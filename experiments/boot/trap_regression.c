/* Synthetic cartridge bytes: TRAP #0 is supported, state-switching TRAP #1
 * remains an explicit adapter responsibility. */
#include "m68k_interp.h"
#include "game_spec.h"
#include <stdio.h>
#include <string.h>
M68KState g_cpu;
uint8_t g_rom[0x400000], g_ram[0x10000];
uint8_t *g_rom_ext;
uint32_t g_rom_ext_size;
uint64_t g_native_insn_count;
uint32_t g_cycle_accumulator, g_audio_cycle_counter, g_vblank_threshold = 1000000;
static int rte_pending;
int *g_rte_pending_ptr = &rte_pending;
const GameSpec g_game_spec = {0};
uint8_t m68k_read8(uint32_t a) { return a < sizeof g_rom ? g_rom[a] : g_ram[a & 65535]; }
uint16_t m68k_read16(uint32_t a) { return (uint16_t)((m68k_read8(a) << 8) | m68k_read8(a+1)); }
uint32_t m68k_read32(uint32_t a) { return ((uint32_t)m68k_read16(a) << 16) | m68k_read16(a+2); }
void m68k_write8(uint32_t a,uint8_t v) { if(a < sizeof g_rom) g_rom[a]=v; else g_ram[a&65535]=v; }
void m68k_write16(uint32_t a,uint16_t v) { m68k_write8(a,(uint8_t)(v>>8));m68k_write8(a+1,(uint8_t)v); }
void m68k_write32(uint32_t a,uint32_t v) { m68k_write16(a,(uint16_t)(v>>16));m68k_write16(a+2,(uint16_t)v); }
void glue_check_vblank(void) { }
int genesis_game_instruction_hook(uint32_t pc) { (void)pc; return 0; }
int game_instruction_hook_site(uint32_t pc) { (void)pc; return 0; }
static unsigned trap_vector;
void m68k_trap_vector(uint8_t vec) { trap_vector = vec; }
int main(void) {
    g_cpu.PC=0x200;g_cpu.SR=0x2300;g_cpu.A[7]=0xFF1000;
    m68k_write16(0x200,0x4E40); /* conventional TRAP #0 */
    M68kiStatus result=m68k_interp_step();
    if(result != M68KI_OK || trap_vector != 32 || g_cpu.PC != 0x202) {
        fprintf(stderr,"TRAP #0 failed: status=%d vector=%u PC=%06X\n",
                result,trap_vector,g_cpu.PC);return 1;
    }
    g_cpu.PC=0x204;
    m68k_write16(0x204,0x4E41); /* non-returning game-state transfer */
    result=m68k_interp_step();
    if(result==M68KI_HALT_UNIMPL && g_m68ki_bad_pc==0x204 && g_m68ki_bad_op==0x4E41) {
        puts("PASS: interpreter dispatches TRAP #0 and loudly defers TRAP #1");return 0;
    }
    fprintf(stderr,"TRAP #1 contract changed: status=%d PC=%06X\n",result,g_cpu.PC);return 1;
}
