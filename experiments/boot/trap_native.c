/* Architectural checks for independently authored native TRAP/RTE fixture. */
#include "trap.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned char memory[1024];
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"failed line %d\n",__LINE__);exit(1); } } while(0)
static M68kStepBusResult r8(void*u,uint32_t a,uint8_t*v) { (void)u;if(a>=1024)return M68K_BUS_FAULT;*v=memory[a];return M68K_BUS_OK; }
static M68kStepBusResult r16(void*u,uint32_t a,uint16_t*v) { (void)u;if(a>=1023)return M68K_BUS_FAULT;*v=(uint16_t)((memory[a]<<8)|memory[a+1]);return M68K_BUS_OK; }
static M68kStepBusResult w8(void*u,uint32_t a,uint8_t v) { (void)u;if(a>=1024)return M68K_BUS_FAULT;memory[a]=v;return M68K_BUS_OK; }
static M68kStepBusResult w16(void*u,uint32_t a,uint16_t v) { (void)u;if(a>=1023)return M68K_BUS_FAULT;memory[a]=(uint8_t)(v>>8);memory[a+1]=(uint8_t)v;return M68K_BUS_OK; }
int main(int argc,char**argv) {
    CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f);CHECK(fread(memory,1,1024,f)==1024);fclose(f);
    M68kStepContext c;M68kStepBus bus={0};bus.read8=r8;bus.read16=r16;bus.fetch16=r16;bus.write8=w8;bus.write16=w16;
    m68k_step_init(&c,&bus,NULL);CHECK(m68k_step_reset(&c)==M68K_STEP_OK);
    CHECK(m68k_step_module_execute(&c,&trap_module)==M68K_STEP_OK);CHECK(c.state.regs.D[0]==5);
    uint16_t sr=c.state.regs.SR;
    CHECK(m68k_step_module_execute(&c,&trap_module)==M68K_STEP_OK);
    CHECK(c.state.regs.PC==0x300 && c.state.regs.A[7]==0x3EA && c.state.exception_vector==33);
    CHECK(m68k_step_read16(&c,0x3EA)==sr && m68k_step_read32(&c,0x3EC)==0x204);
    CHECK(m68k_step_module_execute(&c,&trap_module)==M68K_STEP_OK);CHECK(c.state.regs.D[0]==6);
    CHECK(m68k_step_module_execute(&c,&trap_module)==M68K_STEP_OK);
    CHECK(c.state.regs.PC==0x204 && c.state.regs.A[7]==0x3F0 && c.state.regs.SR==sr);
    CHECK(m68k_step_module_execute(&c,&trap_module)==M68K_STEP_OK);
    CHECK(c.state.regs.PC==0x206 && c.state.instructions==5 && c.state.cycles==70);
    puts("PASS: 5 native operations, TRAP #1 frame/RTE, 70 nominal cycles. No game/runtime validation.");return 0;
}
