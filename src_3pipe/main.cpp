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
		int32_t tempNPC;
		bool executed;				//this flag is turned on when an instruction of one of the 3 types has been executed
		State mips_state;


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

		uint32_t instrALU;
		uint32_t instrMEM;
		uint32_t instrMULDIV;



		int stalling = 0; //stall for 1 extra cycle for LD stalls which are resolved in mem stage



		mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

		bool is_load = false;
		bool is_store = false;
		bool is_mulDiv = false;

		setUp(mips_state, fileName);		//Passes the instructions to the vector

		initPipeline(pipeState_NextALU);
		initPipeline(pipeState_NextMEM);
		initPipeline(pipeState_NextMULDIV);

		for(;;){

			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed

			uint32_t instr = mips_state.ram[mips_state.pc];

			//Send Instruction for Decode
			decode_inst(instr,decode);

			// Execute if not stalling
			if(stalling != 1)
			{
				tempNPC = mips_state.npc;
				//Execute Instructions
				r_type(mips_state,executed,decode,is_mulDiv);
				i_type(mips_state,executed,decode,is_load, is_store);
				j_type(mips_state,executed,decode);
			}

			// check if is_mulDiv, send down pipe3 and send noop down pipe 1 and 2
			if (is_mulDiv) {
				instrMULDIV = instr;
				instrALU = NOP;
				instrMEM = NOP;

			} else if (is_load || is_store) {
				instrMEM = instr;
				instrALU = NOP;
				instrMULDIV = NOP;

			} else {
				instrMEM = instr;
				instrALU = NOP;
				instrMULDIV = NOP;				
			}
			// if is_load or is_store, send down pipe2
			// else send down pipe 1

			moveOneCycle(mips_state, pipeStateALU, pipeState_NextALU, executed, CurCycle, instrALU, stalling, is_load, is_store, is_mulDiv);
			moveOneCycle(mips_state, pipeStateMEM, pipeState_NextMEM, executed, CurCycle, instrMEM, stalling, is_load, is_store, is_mulDiv);
			moveOneCycle(mips_state, pipeStateMULDIV, pipeState_NextMULDIV, executed, CurCycle, instrMULDIV, stalling, is_load, is_store, is_mulDiv);

			if(stalling == 1)
			{
				stalling = 0;
			}

			//dumpPipeState(pipeStateALU, pipeStateMEM, pipeStateMULDIV);

			// compare in all three pipestates
			checkForStall(instr, pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling);

			CurCycle = CurCycle + 1;

			if(stalling !=1)
			{
				mips_state.pc = tempNPC;
			}

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
