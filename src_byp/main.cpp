// 5-stage fully bypassed MIPS processor

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

		// clear contents of pipedump and pipediagram
		std::ofstream ofs;
		ofs.open("pipe_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();
		ofs.open("pipe_diagram.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();

		string fileName(argv[1]);
		// store pc of the next instruction to be issued
		int32_t tempNPC = 1;
		//this flag is turned on when an instruction of one of the 3 types has been executed
		bool executed;	
		// this data structure keeps note of the next pc and the state of the memory		
		State mips_state;
		// this datastructure stores the pipeline state and moves it forward one step for each cycle
		PipeState pipeState;	
		// this datastructure stores the state for what the pipeline will look like in the next cycle
		PipeState_Next pipeState_Next;
		// datastructure to store decoded instruction
		Decode decode;

		// datastructure to store the state of the pipeline diagram per cycle
		DiagramState dstate;

		// initialise the pipeline diagram
		initDiagram(dstate);

		//stall for 1 extra cycle for LD stalls which are resolved in mem stage
		// IF and ID are stalled when stalling = 1
		int stalling = 0; 

		//This will allocate memory for the whole RAM
		mips_state.ram.resize(MEM_SIZE);	

		// is_load checks if instruction is a load instruction
		bool is_load = false;
		bool ex_isload = false;

		//Passes the instructions to the vector
		setUp(mips_state, fileName);		

		// initilaise the pipeline
		initPipeline(pipeState_Next);

		for(;;){

			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed

			// Fetch the instruction to be executed
			uint32_t instr = mips_state.ram[mips_state.pc];

			//Send Instruction for Decode
			decode_inst(instr,decode);

			uint32_t instr_executed = NOP;

			// If the pipeline is not stalling at IF and ID stage because of load stalls, execute the fetched instruction
			// IF, ID, EX, MEM, WB are done in one cycle, the values are propogated down an architectural pipeline 
			if(stalling != 1)
			{
				tempNPC = mips_state.npc;
				//Execute Instructions
				r_type(mips_state,executed,decode);
				i_type(mips_state,executed,decode,is_load);
				j_type(mips_state,executed,decode);
				instr_executed = instr;
			}

			// The instruction just executed is now sent down the pipeline. Even though it has executed it is shown to be in the "IF" stage to the user
			// in the next cycle it will be in the ID stage, then EX and finally WB
			moveOneCycle(mips_state, pipeState, pipeState_Next, executed, CurCycle, instr_executed, stalling, is_load, dstate.num_instrs);

			// Pipe Diagram Allocate
			if (executed && (instr != NOP) && !dstate.is_full && (CurCycle < DIAGRAM_CYCLES)) {
				dstate.instr[dstate.num_instrs].instr = instr;
				dstate.instr[dstate.num_instrs].stage[CurCycle] = "IF ";
				dstate.instr[dstate.num_instrs].done = false;
				
				if (dstate.num_instrs < DIAGRAM_SIZE) {
					dstate.num_instrs += 1;
				} else {
					dstate.is_full = true;
				}
			}

			// update the pipeline diagram for the new instruction
			// UNCOMMENT THIS TO PRINT PIPELINE DIAGRAM
			//updatePipeDiagram(dstate, pipeState, stalling);

			// if stalling was present in the last cycle, mark it as zero and check again if this cycle will need to be stalled
			if(stalling == 1)
			{
				stalling = 0;
			}

			ex_isload = pipeState.ex_isload;
			
			//update the pipeline dump
			// UNCOMMENT THIS TO PRINT PIPELINE DUMP
			//dumpPipeState(pipeState);

			// check if there needs to be a stall in the next cycle for a load RAW dependece
			checkForStall(pipeState, ex_isload, stalling);

			CurCycle = CurCycle + 1;

			// update the present pc to the NPC only if there is no stall
			if(stalling !=1)
			{
				mips_state.pc = tempNPC;
			}
			
			// if end of program has been reached, dump the pipeline diagram
			if(pipeState.wbPC == ADDR_NULL){
				//std::cout << "Dumping Pipe Diagram" << endl;
				// UNCOMMENT THIS TO PRINT PIPELINE DIAGRAM
				//dumpPipeDiagram(dstate);
			}
			
			// check if the end of program has reached and exit the simulator
			checkExit(pipeState.wbreg, pipeState.wbPC,CurCycle);

			// if the writeback stage has an illegal instruction, throw instruction exception
			if(!pipeState.wb){
				throw (static_cast<int>(Exception::INSTRUCTION));
			}		
			
		}

    }

	// catch Exceptions and errors
	catch (const int EXIT_CODE)
	{
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
