#include <iostream>
#include "mips.hpp"
#include <string>
#include <cstdint>
#include <fstream>
#include <iomanip>

using namespace std;

#define ALU_PIPE 1
#define MEM_PIPE 2
#define MULDIV_PIPE 3

#define NOP 0x00000000



int main(int argc, char* argv[]){

	int CurCycle = 0;
   	try{						//Exception and Error handling

		if(argc != 2){
			std::cerr << "Error: Expected a Binary file a input" << std::endl;
			exit(1);
		}

		// clear contents of file
		std::ofstream ofs;
		ofs.open("pipe_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();

		string fileName(argv[1]);
		int32_t tempNPC = 0;
		bool executed;				//this flag is turned on when an instruction of one of the 3 types has been executed
		State mips_state;

		ROBState robState;
		initROB(robState);

		DiagramState dstate;
		initDiagram(dstate);

		PipeState pipeStateALU;
		pipeStateALU.pipe_type = ALU_PIPE;	
		PipeState pipeStateMEM;	
		pipeStateMEM.pipe_type = MEM_PIPE;
		PipeState pipeStateMULDIV;	
		pipeStateMULDIV.pipe_type = MULDIV_PIPE;

		PipeState_Next pipeState_NextALU;
		PipeState_Next pipeState_NextMEM;
		PipeState_Next pipeState_NextMULDIV;

		Decode decode;

		uint32_t instrALU = NOP;
		uint32_t instrMEM = NOP;
		uint32_t instrMULDIV = NOP;
	

		int stalling = 0; //stall for 1 extra cycle for LD stalls which are resolved in mem stage

		mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

		bool is_load = false;
		bool is_store = false;
		bool is_mulDiv = false;
		bool is_md_non_stall = false;

		setUp(mips_state, fileName);		//Passes the instructions to the vector

		initPipeline(pipeState_NextALU);
		initPipeline(pipeState_NextMEM);
		initPipeline(pipeState_NextMULDIV);

		for(;;){

			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed

			uint32_t instr = mips_state.ram[mips_state.pc];

			robState.cycle = CurCycle;
			robState.commited = false;
			robState.commit_instr = NOP;

			//Send Instruction for Decode
			decode_inst(instr,decode);

			// Execute if not stalling
			if(stalling != 1)
			{
				tempNPC = mips_state.npc;
				//Execute Instructions
				r_type(mips_state,executed,decode,is_mulDiv, is_md_non_stall);
				i_type(mips_state,executed,decode,is_load, is_store);
				j_type(mips_state,executed,decode);

				if (is_mulDiv || is_md_non_stall) { // check if is_mulDiv, send down pipe3 and send nop down pipe 1 and 2
					instrMULDIV = instr;
					instrALU = NOP;
					instrMEM = NOP;

				} else if (is_load || is_store) { // if is_load or is_store, send down pipe2
					instrMEM = instr;
					instrALU = NOP;
					instrMULDIV = NOP;

				} else { // otherwise send down pipe 1
					instrMEM = NOP;
					instrALU = instr;
					instrMULDIV = NOP;				
				}
			}
			
			moveOneCycle(mips_state, pipeStateALU, pipeState_NextALU, executed, CurCycle, instrALU, stalling, is_load, is_store, is_mulDiv, robState.tail);
			moveOneCycle(mips_state, pipeStateMEM, pipeState_NextMEM, executed, CurCycle, instrMEM, stalling, is_load, is_store, is_mulDiv, robState.tail);
			moveOneCycle(mips_state, pipeStateMULDIV, pipeState_NextMULDIV, executed, CurCycle, instrMULDIV, stalling, is_load, is_store, is_mulDiv, robState.tail);

			// ROB Commit
			if ((!robState.pending[robState.head]) && (robState.valid[robState.head])) {
				robState.commited = true;
				robState.commit_instr = robState.instr[robState.head];
				robState.valid[robState.head] = false;
				if (robState.head != 15) {
					robState.head += 1;
				} else {
					robState.head = 0;
				}
			}

			// ROB Allocate
			if (executed && (instr != NOP) && (robState.valid[robState.tail] != true)) {
				robState.instr[robState.tail] = instr;
				robState.valid[robState.tail] = true;
				robState.pending[robState.tail] = true;
				robState.preg[robState.tail] = decode.rt;
				if (robState.tail != 15) {
					robState.tail += 1;
				} else {
					robState.tail = 0;
				}
				
			}

			// ROB Fill
			if (pipeStateALU.wb_isval) {
				robState.pending[pipeStateALU.rob_fill_slot_wb] = false;
				//std::cout << "Cycle: " << CurCycle << " -- " << "Filling Instruction from ALU pipe: " << endl;
			}
			if (pipeStateMEM.wb_isval) {
				robState.pending[pipeStateMEM.rob_fill_slot_wb] = false;
				//std::cout << "Cycle: " << CurCycle << " -- " << "Filling Instruction from MEM pipe:"  << endl;
			}
			if (pipeStateMULDIV.wb_isval) {
				robState.pending[pipeStateMULDIV.rob_fill_slot_wb] = false;
				//std::cout << "Cycle: " << CurCycle << " -- " << "Filling Instruction from MULDIV pipe: " << endl;
			}

			if(stalling == 1)
			{
				stalling = 0;
			}


			// compare in all three pipestates
			checkForStall(pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling);

			//dumpROBState(robState);
			//dumpPipeState(pipeStateALU, pipeStateMEM, pipeStateMULDIV, robState);	



			CurCycle = CurCycle + 1;

			if(stalling !=1)
			{
				mips_state.pc = tempNPC;
			}

			/*
			if(pipeStateALU.wbPC == ADDR_NULL){
				std::cout << "Dumping Pipe Diagram" << endl;
				dumpPipeDiagram(dstate);

			}
			*/
			checkExit(pipeStateALU.wbreg, pipeStateALU.wbPC,CurCycle);

			if(!pipeStateALU.wb){
				throw (static_cast<int>(Exception::INSTRUCTION));
			}		
			
		};
    }
//
	catch (const int EXIT_CODE){		//Exceptions and Errors are caught here
		cout << CurCycle << " \n";
		switch(EXIT_CODE){
			case 0xFFFFFFF6:
				std::exit(static_cast<int>(Exception::ARITHMETIC));
			case 0xFFFFFFF5:
				std::exit(static_cast<int>(Exception::MEMORY));
			case 0xFFFFFFF4:
				std::exit(static_cast<int>(Exception::INSTRUCTION));
			case 0xFFFFFFEC:
				std::exit(static_cast<int>(Error::IO));
			default: ;
		}
	}	
	catch(...){				//If an error ocurrs that is of none of the defined above, then it is of unknown nature
		std::exit(static_cast<int>(Error::INTERNAL));
	}

}
