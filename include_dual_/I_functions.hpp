#ifndef I_TYPEHPP
#define I_TYPEHPP

#include "setUp.hpp"
#include "error.hpp"
#include "Decode.hpp"

void i_type(State& mips_state, bool& executed, Decode& decode, bool& is_load, bool& is_store, bool& is_branch, bool&is_I, bool is_start);

void addi(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void addiu(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void andi(State& mips_state, uint32_t rs, uint32_t rt, int32_t immediate);
void beq(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm, bool& is_branch, bool is_start);
void bne(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm, bool& is_branch, bool is_start);
void lbu(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void lb(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void lhu(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void lh(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void lui(State& mips_state, uint32_t rs, uint32_t rt, int32_t immediate);
void lw(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void lwl(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void lwr(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void ori(State& mips_state, uint32_t rs, uint32_t rt, int32_t immediate);
void xori(State& mips_state, uint32_t rs, uint32_t rt, int32_t immediate);
void slti(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void sltiu(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void sb(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void sh(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void sw(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm);
void bgez(State& mips_state, uint32_t rs, int32_t SignExtImm, bool& is_branch, bool is_start);
void bgezal(State& mips_state, uint32_t rs, int32_t SignExtImm, bool& is_branch, bool is_start);
void bgtz(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm, bool& is_branch, bool is_start);
void blez(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm, bool& is_branch, bool is_start);
void bltz(State& mips_state, uint32_t rs, int32_t SignExtImm, bool& is_branch, bool is_start);
void bltzal(State& mips_state, uint32_t rs, int32_t SignExtImm, bool& is_branch, bool is_start);
void bdecoder(State& mips_state, uint32_t rs, uint32_t rt, int32_t SignExtImm, bool& is_branch, bool is_start);

#endif
