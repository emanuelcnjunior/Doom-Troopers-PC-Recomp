/* Experimental vector adapter. Addresses are validated ROM metadata only.
 * No game-specific RAM edits, video overrides or yield substitutions. */
#include "game_spec.h"
#include "genesis_runtime.h"
#include "doom_cheats.h"
#include "m68k_interp.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned launcher_cheats;
static int transition_trace;
static uint64_t doom_frame_count;
static uint32_t last_game_pc;
static uint32_t last_game_sp;
static void post_reset(void);
static void frame_post(uint64_t frame_count);

typedef struct DoomTransitionState {
    uint32_t callback;
    uint16_t mode;
    uint16_t stage;
    uint16_t substate;
    uint8_t flags;
    uint8_t trap_state;
    uint8_t trap_count;
} DoomTransitionState;

static DoomTransitionState read_transition_state(void) {
    DoomTransitionState state;
    state.callback = m68k_read32(0x00ff0006u) & 0x00ffffffu;
    state.mode = m68k_read16(0x00ff002au);
    state.stage = m68k_read16(0x00ff002cu);
    state.substate = m68k_read16(0x00ff002eu);
    state.flags = m68k_read8(0x00ff0032u);
    state.trap_state = m68k_read8(0x00ff0094u);
    state.trap_count = m68k_read8(0x00ff0095u);
    return state;
}

static void log_transition_state(uint64_t frame_count) {
    static DoomTransitionState previous;
    static int initialized;
    DoomTransitionState current = read_transition_state();
    if (!transition_trace) return;
    if (!initialized || current.callback != previous.callback ||
        current.stage != previous.stage || current.substate != previous.substate ||
        current.trap_state != previous.trap_state || current.trap_count != previous.trap_count) {
        fprintf(stderr,
                "[DOOM_TRANSITION] frame=%llu game_pc=%06X game_sp=%08X callback=%06X "
                "ram_002A=%04X ram_002C=%04X ram_002E=%04X ram_0032=%02X "
                "ram_0094=%02X ram_0095=%02X\n",
                (unsigned long long)frame_count, last_game_pc, last_game_sp,
                current.callback, current.mode, current.stage, current.substate,
                current.flags, current.trap_state, current.trap_count);
        previous = current;
        initialized = 1;
    }
}

static void configure_launcher_cheats(void) {
    const char *setting = getenv("DOOM_CHEAT_MASK");
    if (setting) {
        char *end;
        unsigned long value = strtoul(setting, &end, 10);
        if (*setting && !*end && value < (1ul << 30)) launcher_cheats = (unsigned)value;
    }
    doom_launcher_cheats(launcher_cheats, 1);
}
static void entry(void) {
    /* The pinned runner declares lifecycle hooks but does not invoke them. */
    post_reset();
    recomp_call_addr(0x0568A8u);
}
static void vint(void) {
    recomp_call_addr(0x000200u);
    frame_post(++doom_frame_count);
}
static void hint(void) { recomp_call_addr(0x00020Au); }
static void post_reset(void) {
    const char *trace = getenv("DOOM_TRANSITION_TRACE");
    transition_trace = trace && *trace && *trace != '0';
    doom_cheats_apply_rom();
    configure_launcher_cheats();
    fprintf(stderr, "[Doom cheats] %s\n", doom_cheats_summary());
}
static void frame_post(uint64_t frame_count) {
    doom_cheats_apply_ram();
    if (launcher_cheats) doom_launcher_cheats(launcher_cheats, 0);
    log_transition_state(frame_count);
}
/* Actor actions can resume a different saved stack. Execute the scheduler
 * through its actual 68000 return to the frame loop, including its callees. */
static int run_actor_scheduler(uint32_t pc) {
    static int interpreting;
    /* ROM cheats use the CPU interpreter so patched instructions are fetched live. */
    if ((launcher_cheats & ~0x103fu) || pc != 0x000358u || interpreting) return 0;
    uint32_t caller_sp = g_cpu.A[7];
    uint32_t return_pc = m68k_read32(caller_sp) & 0xffffffu;
    if (return_pc != 0x054ee0u) return 0;
    interpreting = 1;
    M68kiStatus status = m68k_interp_run(pc, return_pc);
    interpreting = 0;
    last_game_pc = g_cpu.PC & 0xffffffu;
    last_game_sp = g_cpu.A[7];
    /* TRAP #1 discards the actor stack and enters a new game state.
     * Transfer it to the same native vector handler used at boot. */
    if (status == M68KI_HALT_UNIMPL && g_m68ki_bad_pc == g_cpu.PC &&
        g_m68ki_bad_op == 0x4e41u) {
        if (transition_trace)
            fprintf(stderr, "[DOOM_SCHEDULER] state TRAP #1 pc=%06X sp=%08X\n",
                    g_cpu.PC, g_cpu.A[7]);
        g_cpu.PC += 2;
        m68k_trap_vector(33);
        return 1;
    }
    if (status != M68KI_OK || g_cpu.A[7] != caller_sp + 4u) {
        fprintf(stderr,
                "[DOOM_SCHEDULER][FATAL] pc=%06X status=%d opcode=%04X "
                "bad_pc=%06X caller_sp=%08X current_sp=%08X return=%06X\n",
                g_cpu.PC & 0xffffffu, status, g_m68ki_bad_op,
                g_m68ki_bad_pc & 0xffffffu, caller_sp, g_cpu.A[7], return_pc);
        fflush(stderr);
        abort();
    }
    /* The generated JSR owns the final pop. */
    g_cpu.A[7] = caller_sp;
    return 1;
}

const GameSpec g_game_spec = {
    .display_name = "Doom Troopers boot experiment",
    .short_name = "DOOM-PROBE",
    .expected_rom_crc32 = 0x11194414u,
    .expected_rom_size = 2097152u,
    .tier3_floor_default = 1,
    .instruction_hook = run_actor_scheduler,
    .call_entry_point = entry,
    .call_vblank = vint,
    .call_hblank = hint
};
