#include "J_functions.hpp"
#include <iostream>
using namespace std;


void j_type(State& mips_state, bool& executed, Decode& decode, bool& is_jump, bool&is_J){
	is_J = false;
	if(!executed){
		// int32_t instr = mips_state.ram[mips_state.pc];
		// int32_t address = instr & 0x03FFFFFF;
		// int32_t opcode = (instr & 0xFC000000) >> 26;
		is_J = true;
		is_jump = false;
		switch(decode.opcode) {
			case 0x00000002:
				j(mips_state, decode.address);
				is_jump = true;
				executed = true;
				return;
			case 0x00000003:
				jal(mips_state, decode.address);
				executed = true;
				is_jump = true;
				return;
			default:
				is_J = false;
				executed = false;
				return;
		}
	}
}


void j(State& mips_state, uint32_t address){
	mips_state.npc = ((mips_state.pc & 0x3F000000) | address);
}

void jal(State& mips_state, uint32_t address){
	mips_state.reg[31] = (mips_state.npc) * 4 + 4;
	mips_state.npc = ((mips_state.pc & 0x3F000000) | address);
}
