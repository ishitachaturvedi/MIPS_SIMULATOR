// original code we started with 
// there is no pipelining, each instruction takes 1 cycle to complete

#include <iostream>
#include "mips.hpp"
#include <string>
#include <cstdint>

using namespace std;

int main(int argc, char* argv[]){
	
	int CurCycle = 0;
   try{						//Exception and Error handling

	if(argc != 2){
		std::cerr << "Error: Expected a Binary file a input" << std::endl;
		exit(1);
	}

	string fileName(argv[1]);
	int32_t tempNPC;
	bool executed;				//this flag is turned on when an instruction of one of the 3 types has been executed
	State mips_state;	

	mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

	
	setUp(mips_state, fileName);		//Passes the instructions to the vector

	for(;;){

		if(mips_state.pc == ADDR_NULL) {
			std::cout << "Cycle Count: " << CurCycle << endl;
		}
		checkExec(mips_state.reg, mips_state.pc); //checks if the address is in the executable range
		mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
		executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed
		tempNPC = mips_state.npc;	//Since the instruction that will be executed will change the npc it needs to be stored

		// execute the instruction
		r_type(mips_state,executed);
		i_type(mips_state,executed);
		j_type(mips_state,executed);

		if(!executed){			//if no instruction from the 3 types has executed at this stage (ie.false), then the binary must be invalid or an unknown error occurred
			throw (static_cast<int>(Exception::INSTRUCTION));
		}		
		
		mips_state.pc = tempNPC;	//Set the value of pc (the address of the next instruction that is going to execute) to the
						//original value of npc

		CurCycle += 1;
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
