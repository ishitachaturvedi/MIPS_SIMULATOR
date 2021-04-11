#include <iostream>
#include "mips.hpp"
#include <string>
#include <cstdint>
#include <fstream>
#include <iomanip>

int main(int argc, char* argv[]){

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
		PipeState pipeState;	
		PipeState_Next pipeState_Next;
		Decode decode;
		Decode decode_ex;
		decode_ex.rd = 0x0;

		int CurCycle = 0;
		int stalling = 0; //stall for 1 extra cycle for LD stalls which are resolved in mem stage


		mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

		bool is_load;

		setUp(mips_state, fileName);		//Passes the instructions to the vector

		initPipeline(pipeState_Next);

		for(;;){

			//checkExec(mips_state.reg, mips_state.pc); //checks if the address is in the executable range
			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed
			tempNPC = mips_state.npc;	//Since the instruction that will be executed will change the npc it needs to be stored

			uint32_t instr = mips_state.ram[mips_state.pc];

			//Send Instruction for Decode
			decode_inst(instr,decode);

			checkForStall(decode, decode_ex,is_load,stalling);

			if(stalling != 0)
			{
				stalling = stalling + 1;
			}

			if(stalling == 3)
			{
				stalling = 0;
			}

			if(stalling == 0)
			{
				//Execute Instructions
				r_type(mips_state,executed,decode);
				i_type(mips_state,executed,decode,is_load);
				j_type(mips_state,executed,decode);
				moveOneCycle(mips_state, pipeState, pipeState_Next, executed, CurCycle);
				decode_ex = decode;
				mips_state.pc = tempNPC;	//Set the value of pc (the address of the next instruction that is going to execute) to the
			}

			dumpPipeState(pipeState);
			CurCycle = CurCycle + 1;

			checkExit(pipeState.wbreg, pipeState.wbPC);

			
			//if(!executed){			//if no instruction from the 3 types has executed at this stage (ie.false), then the binary must be invalid or an unknown error occurred
			if(!pipeState.wb){
				throw (static_cast<int>(Exception::INSTRUCTION));
			}		
			
			//mips_state.pc = tempNPC;	//Set the value of pc (the address of the next instruction that is going to execute) to the
							//original value of npc

		};

    }

	catch (const int EXIT_CODE){		//Exceptions and Errors are caught here
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
