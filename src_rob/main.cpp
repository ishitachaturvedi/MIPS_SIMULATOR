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

		// clear contents of pipedump and pipediagram
		std::ofstream ofs;
		ofs.open("pipe_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();
		ofs.open("pipe_diagram.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();
		ofs.open("rob_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();

		string fileName(argv[1]);
		// store pc of the next instruction to be issued
		int32_t tempNPC = 0;
		//this flag is turned on when an instruction of one of the 3 types has been executed
		bool executed;				
		// this data structure keeps note of the next pc and the state of the memory	
		State mips_state;
		// this data structure keeps note of the state of the ROB
		ROBState robState;
		// initialise the ROB
		initROB(robState);

		// datastructure to store the state of the pipeline diagram per cycle
		DiagramState dstate;
		// initialise the pipeline diagram
		initDiagram(dstate);

		// this datastructure stores the ALU pipeline state and moves it forward one step for each cycle
		PipeState pipeStateALU;
		pipeStateALU.pipe_type = ALU_PIPE;	
		// this datastructure stores the MEM pipeline state and moves it forward one step for each cycle
		PipeState pipeStateMEM;	
		pipeStateMEM.pipe_type = MEM_PIPE;
		// this datastructure stores the MULDIV pipeline state and moves it forward one step for each cycle
		PipeState pipeStateMULDIV;	
		pipeStateMULDIV.pipe_type = MULDIV_PIPE;

		// this datastructure stores the state for what the pipeline will look like in the next cycle
		PipeState_Next pipeState_NextALU;
		PipeState_Next pipeState_NextMEM;
		PipeState_Next pipeState_NextMULDIV;

		// datastructure to store decoded instruction
		Decode decode;

		// store ALU instruction
		uint32_t instrALU = NOP;
		// store MEM instruction
		uint32_t instrMEM = NOP;
		// store MULDIV instruction
		uint32_t instrMULDIV = NOP;
	
		//stall for 1 extra cycle for LD stalls which are resolved in mem stage
		// IF and ID are stalled when stalling = 1
		int stalling = 0; //stall for 1 extra cycle for LD stalls which are resolved in mem stage

		//This will allocate memory for the whole RAM
		mips_state.ram.resize(MEM_SIZE);	

		// is_load checks if instruction is a load instruction
		bool is_load = false;
		// is_load checks if instruction is a store instruction
		bool is_store = false;
		// is_load checks if instruction is a MULDIV instruction
		bool is_mulDiv = false;
		// is_load checks if instruction is a mflo or mfhi instruction
		bool is_md_non_stall = false;

		// Passes the instructions to the vector
		setUp(mips_state, fileName);		

		// initilaise the ALU, MEM, MULDIV pipelines
		initPipeline(pipeState_NextALU);
		initPipeline(pipeState_NextMEM);
		initPipeline(pipeState_NextMULDIV);

		for(;;){

			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executed = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed

			// Fetch the instruction to be executed
			uint32_t instr = mips_state.ram[mips_state.pc];

			// initialise ROB commit state per cycle
			robState.cycle = CurCycle;
			robState.commited = false;
			robState.commit_instr = NOP;

			//Send Instruction for Decode
			decode_inst(instr,decode);

			// If the pipeline is not stalling at IF and ID stage because of load stalls, execute the fetched instruction
			// IF, ID, EX, MEM, WB are done in one cycle, the values are propogated down an architectural pipeline 
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

			// check if is_mulDiv, send down pipe3 and send noop down pipe 1 and 2

			// if is_load or is_store, send down pipe2
			// else send down pipe 1

			// The instruction just executed is now sent down the pipeline. Even though it has executed it is shown to be in the "IF" stage to the user
			// in the next cycle it will be in the ID stage, then EX and finally WB
			// Move ALU, MEM and MULDIV one cycle forward
			moveOneCycle(mips_state, pipeStateALU, pipeState_NextALU, executed, CurCycle, instrALU, stalling, is_load, is_store, is_mulDiv, robState.tail, dstate.num_instrs);
			moveOneCycle(mips_state, pipeStateMEM, pipeState_NextMEM, executed, CurCycle, instrMEM, stalling, is_load, is_store, is_mulDiv, robState.tail, dstate.num_instrs);
			moveOneCycle(mips_state, pipeStateMULDIV, pipeState_NextMULDIV, executed, CurCycle, instrMULDIV, stalling, is_load, is_store, is_mulDiv, robState.tail, dstate.num_instrs);

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

			// Pipe Diagram Allocate
			
			if (executed && (instr != NOP) && !dstate.is_full && (CurCycle < DIAGRAM_CYCLES)) {
				dstate.instr[dstate.num_instrs].instr = instr;
				dstate.instr[dstate.num_instrs].stage[CurCycle] = "IF\t";
				dstate.instr[dstate.num_instrs].done = false;
				
				if (dstate.num_instrs < DIAGRAM_SIZE) {
					dstate.num_instrs += 1;
				} else {
					dstate.is_full = true;
				}
			}

			updatePipeDiagram(dstate, pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling);
			
			// if stalling was present in the last cycle, mark it as zero and check again if this cycle will need to be stalled
			if(stalling == 1)
			{
				stalling = 0;
			}

			// check if there needs to be a stall in the next cycle for a load RAW dependece
			checkForStall(pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling);

			dumpROBState(robState);
			dumpPipeState(pipeStateALU, pipeStateMEM, pipeStateMULDIV, robState);

			CurCycle = CurCycle + 1;

			// update the present pc to the NPC only if there is no stall
			if(stalling !=1)
			{
				mips_state.pc = tempNPC;
			}

			// check if the end of program has reached and exit the simulator
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
