#include <iostream>
#include "mips.hpp"
#include <string>
#include <cstdint>
#include <fstream>
#include <iomanip>

using namespace std;

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
		PipeState pipeState;	
		PipeState_Next pipeState_Next;
		Decode decode;

		int stalling = 0; //stall for 1 extra cycle for LD stalls which are resolved in mem stage


		mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

		bool is_load = false;
		bool ex_isload = false;

		setUp(mips_state, fileName);		//Passes the instructions to the vector

		initPipeline(pipeState_Next);

		for(;;){

			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed

			uint32_t instr = mips_state.ram[mips_state.pc];

			//Send Instruction for Decode
			decode_inst(instr,decode);

			if(stalling !=2)
			{
				tempNPC = mips_state.npc;
			}

			// Execute if not stalling
			if(stalling != 2)
			{
				//Execute Instructions
				r_type(mips_state,executed,decode);
				i_type(mips_state,executed,decode,is_load);
				j_type(mips_state,executed,decode);
			}

			if(stalling == 3)
			{
				stalling = 0;
			}

			moveOneCycle(mips_state, pipeState, pipeState_Next, executed, CurCycle, stalling, is_load);

			ex_isload = pipeState.ex_isload;
			
			//dumpPipeState(pipeState);

			checkForStall(pipeState, ex_isload, stalling);

			CurCycle = CurCycle + 1;

			if(stalling !=1)
			{
				mips_state.pc = tempNPC;
			}

			if(stalling != 0)
			{
				stalling = stalling + 1;
			}

			checkExit(pipeState.wbreg, pipeState.wbPC,CurCycle);

			
			if(!pipeState.wb){
				throw (static_cast<int>(Exception::INSTRUCTION));
			}		
			
		};

    }

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
