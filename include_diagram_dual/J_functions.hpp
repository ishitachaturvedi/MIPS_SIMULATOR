#ifndef J_TYPE
#define J_TYPE

#include "mips.hpp"
#include "Decode.hpp"

void j_type(State& mips_state, bool& executed, Decode& decode, bool& is_jump, bool&is_J);


void j(State& mips_state, uint32_t address);
void jal(State& mips_state, uint32_t address);

#endif
