#include "J_functions.hpp"
#include <iostream>
using namespace std;

// execute Jump instruction for this cycle
void j_type(State& mips_state, bool& executed, Decode& decode){
	if(!executed){
	
		switch(decode.opcode) {
			case 0x00000002:
				j(mips_state, decode.address);
				executed = true;
				return;
			case 0x00000003:
				jal(mips_state, decode.address);
				executed = true;
				return;
			default:
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
