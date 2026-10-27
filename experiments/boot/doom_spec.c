/* Experimental vector adapter. Addresses are validated ROM metadata only.
 * No game-specific RAM edits, video overrides or yield substitutions. */
#include "game_spec.h"
#include "genesis_runtime.h"
static void entry(void) { recomp_call_addr(0x0568A8u); }
static void vint(void) { recomp_call_addr(0x000200u); }
static void hint(void) { recomp_call_addr(0x00020Au); }
const GameSpec g_game_spec = {
    .display_name = "Doom Troopers boot experiment",
    .short_name = "DOOM-PROBE",
    .expected_rom_crc32 = 0x11194414u,
    .expected_rom_size = 2097152u,
    .tier3_floor_default = 1,
    .call_entry_point = entry,
    .call_vblank = vint,
    .call_hblank = hint
};
