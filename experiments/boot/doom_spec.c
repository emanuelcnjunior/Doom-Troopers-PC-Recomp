/* Experimental vector adapter. Addresses are validated ROM metadata only.
 * No game-specific RAM edits, video overrides or yield substitutions. */
#include "game_spec.h"
#include "genesis_runtime.h"
#include "doom_cheats.h"
#include "m68k_interp.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned launcher_cheats;

static void configure_launcher_cheats(void) {
    const char *setting = getenv("DOOM_CHEAT_MASK");
    if (setting) {
        char *end;
        unsigned long value = strtoul(setting, &end, 10);
        if (*setting && !*end && value < (1ul << 30)) launcher_cheats = (unsigned)value;
    }
    doom_launcher_cheats(launcher_cheats, 1);
}
static void entry(void) { recomp_call_addr(0x0568A8u); }
static void vint(void) { recomp_call_addr(0x000200u); }
static void hint(void) { recomp_call_addr(0x00020Au); }
static void post_reset(void) {
    doom_cheats_apply_rom();
    configure_launcher_cheats();
    fprintf(stderr, "[Doom cheats] %s\n", doom_cheats_summary());
}
static void frame_post(uint64_t frame_count) {
    (void)frame_count;
    doom_cheats_apply_ram();
    if (launcher_cheats) doom_launcher_cheats(launcher_cheats, 0);
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
    /* TRAP #1 discards the actor stack and enters a new game state.
     * Transfer it to the same native vector handler used at boot. */
    if (status == M68KI_HALT_UNIMPL && g_m68ki_bad_pc == g_cpu.PC &&
        g_m68ki_bad_op == 0x4e41u) {
        g_cpu.PC += 2;
        m68k_trap_vector(33);
        return 1;
    }
    if (status != M68KI_OK || g_cpu.A[7] != caller_sp + 4u) {
        fprintf(stderr, "Actor scheduler failed at PC=%06X status=%d\n", g_cpu.PC, status);
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
    .call_hblank = hint,
    .on_post_reset = post_reset,
    .on_frame_post = frame_post
};
