#include <iostream>
#include "mips.hpp"
#include <string>
#include <cstdint>

using namespace std;

int main(int argc, char* argv[]){

   try{						//Exception and Error handling

	if(argc != 2){
		std::cerr << "Error: Expected a Binary file a input" << std::endl;
		exit(1);
	}

	string fileName(argv[1]);
	int32_t tempNPC;
	bool executed;				//this flag is turned on when an instruction of one of the 3 types has been executed
	bool is_load; //is inst load
	State mips_state;	
	PipeState pipeState;	
	PipeState_Next pipeState_Next;
	Decode decode;

	initPipeline(pipeState_Next); // initialise the structure

	mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

	int CurCycle = 0;

	
	setUp(mips_state, fileName);		//Passes the instructions to the vector

	for(;;){

		checkExec(mips_state.reg, mips_state.pc); //checks if the address is in the executable range
		mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
		executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed
		tempNPC = mips_state.npc;	//Since the instruction that will be executed will change the npc it needs to be stored

		uint32_t instr = mips_state.ram[mips_state.pc];

		decode_inst(instr,decode);

		r_type(mips_state,executed,decode);
		i_type(mips_state,executed,decode,is_load);
		j_type(mips_state,executed,decode);

		moveOneCycle(mips_state, pipeState, pipeState_Next, executed, CurCycle);

		//dumpPipeState(pipeState);
		CurCycle = CurCycle + 1;

		if(!executed){			//if no instruction from the 3 types has executed at this stage (ie.false), then the binary must be invalid or an unknown error occurred
			throw (static_cast<int>(Exception::INSTRUCTION));
		}		
		
		mips_state.pc = tempNPC;	//Set the value of pc (the address of the next instruction that is going to execute) to the
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
