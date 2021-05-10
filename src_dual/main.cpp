#include <iostream>
#include "mips.hpp"
#include <string>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <vector>

using namespace std;

#define ALU_PIPE 1
#define MEM_PIPE 2
#define MULDIV_PIPE 3

#define NOP 0x00000000

// Consider two inst A and B

int main(int argc, char* argv[]){

	int CurCycle = 0;
   	try{						//Exception and Error handling

		if(argc != 2){
			std::cerr << "Error: Expected a Binary file a input" << std::endl;
			exit(1);
		}

		// clear contents of pipe state dump file
		std::ofstream ofs;
		ofs.open("pipe_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();
		// clear contents of rob state dump file
		std::ofstream ofs1;
		ofs1.open("rob_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs1.close();
		// clear contents of pipeline diagram file
		ofs1.open("pipe_diagram.out", std::ofstream::out | std::ofstream::trunc);
		ofs1.close();

		// get filename of program
		string fileName(argv[1]);
		// store pc of the next instruction to be issued
		int32_t tempNPC = 0;

		// this flag is turned on when instruction A has been executed
		bool executedA;
		// this flag is turned on when instruction B has been executed
		bool executedB;
		// this data structure keeps track of the next pc and memory state
		State mips_state;
		// this data structure keeps track of the ROB state
		ROBState robState;
		// initialize the ROB
		initROB(robState);

		// datastructure to store the state of the pipeline diagram per cycle
		DiagramState dstate;
		// initialise the pipeline diagram
		initDiagram(dstate);

		// the ALU pipe state for X1 onwards, moves forward one step each cycle
		PipeState pipeStateALU;
		pipeStateALU.pipe_type = ALU_PIPE;	
		// the MEM pipe state for X1 onwards, moves forward one step each cycle
		PipeState pipeStateMEM;	
		pipeStateMEM.pipe_type = MEM_PIPE;
		// the MULDIV pipe state for X1 onwards, moves forward one step each cycle
		PipeState pipeStateMULDIV;	
		pipeStateMULDIV.pipe_type = MULDIV_PIPE;

		// these datastructures store the state for what each pipeline will look like in the next cycle
		PipeState_Next pipeState_NextALU;
		PipeState_Next pipeState_NextMEM;
		PipeState_Next pipeState_NextMULDIV;

		// this datastructure keeps track of the pipe state for IF, ID and X1 stages
		// before the two fetched instructions have been issued
		PipeStateIFID pipeStateIFID;

		// datastructure to store decoded instruction A in IF
		Decode decodeA;
		// datastructure to store decoded instruction B in IF
		Decode decodeB;
		// datastructure to store decoded instruction A in EX
		Decode decodeA_ex;
		// datastructure to store decoded instruction B in EX
		Decode decodeB_ex;

		// stall for 2 extra cycles for LD stalls which are resolved in mem stage
		// IF and ID are stalled when stalling = 1
		int stalling = 0; 

		// Allocate memory for the whole RAM
		mips_state.ram.resize(MEM_SIZE);	

		// flag to check if instr A is a load instruction
		bool is_loadA = false;
		// flag to check if instr A is a store instruction
		bool is_storeA = false;
		// flag to check if instr A can cause mul/div stalls
		bool is_mulDivA = false;
		// flag to check if instr A is a branch instruction
		bool is_branchA = false;
		// flag to check if instr A is a jump instruction
		bool is_jumpA = false;
		// flag to check if instr A is an R-type instruction
		bool is_RA = false;
		// flag to check if instr A is an I-type instruction
		bool is_IA = false;
		// flag to check if instr A is a J-type instruction
		bool is_JA = false;

		// flag to check if instr B is a load instruction
		bool is_loadB = false;
		// flag to check if instr B is a store instruction
		bool is_storeB = false;
		// flag to check if instr A can cause mul/div stalls
		bool is_mulDivB = false;
		// flag to check if instr B is a branch instruction
		bool is_branchB = false;
		// flag to check if instr B is a jump instruction
		bool is_jumpB = false;
		// flag to check if instr B is an R-type instruction
		bool is_RB = false;
		// flag to check if instr B is an I-type instruction
		bool is_IB = false;
		// flag to check if instr B is an R-type instruction
		bool is_JB = false;

		// flag to check if there is a hazard between the two jointly fetched instructions
		bool hazard = false;

		// flag to check if instr A is a non-stalling muld/div instr
		bool is_md_non_stallA = false;
		// flag to check if instr B is a non-stalling muld/div instr
		bool is_md_non_stallB = false;
		
		bool is_exited = false;

		bool is_start = true;

		bool is_branch_jump_B_prev = false;

		// vector of ints to store register state A
		std::vector<int32_t> regA;
		// vector of ints to store register state B
    	std::vector<int32_t> regB;

		// store first fetched instruction
		uint32_t instrA = NOP;
		// store second fetched instruction
		uint32_t instrB = NOP;
		// pc of first fetched instruction
		uint32_t pc_A = 0x1;
		// pc of second fetched instruction
		uint32_t pc_B = 0x1;

		// Passes the instructions to the vector
		setUp(mips_state, fileName);		

		// initilaise the IFID pipeline
		initPipelineIFID(pipeStateIFID);

		// initilaise the ALU, MEM, MULDIV pipelines
		initPipeline(pipeState_NextALU);
		initPipeline(pipeState_NextMEM);
		initPipeline(pipeState_NextMULDIV);

		bool pause_for_jump_branch = false;

		for(;;){

			//register $0 must retain the value zero in every new clock cycle of the processor
			mips_state.reg[0] = 0;
			//every new clock cycle both the executed flags are turned off since no instruction has yet been executed
			executedA = false;		
			executedB = false;

			// Reset the ROB state for each cycle
			robState.cycle = CurCycle;
			robState.commitedA = false;
			robState.commit_instrA = NOP;
			robState.commitedB = false;
			robState.commit_instrB = NOP;

			// If the pipeline is not stalling at the IF stage, fetch and execute next instructions
			// IF, ID, EX, MEM, WB are done in one cycle, the values are propogated down an architectural pipeline 
			if(stalling != 1 && pipeStateIFID.stall_state_IF != 1 && !is_exited && !pause_for_jump_branch)
			{
				flush(cout);
				// fetch instruction A
				instrA = mips_state.ram[mips_state.pc];
				// set instr A pc
				pc_A = mips_state.pc;
				// send instruction A for decode
				decode_inst(instrA,decodeA);
				// store next pc
				tempNPC = mips_state.npc;
				
				//Execute Instruction A
				r_type(mips_state,executedA,decodeA,is_mulDivA,is_jumpA,is_branchA,is_RA,is_md_non_stallA);
				i_type(mips_state,executedA,decodeA,is_loadA,is_storeA,is_branchA,is_IA,is_start);
				j_type(mips_state,executedA,decodeA,is_jumpA,is_JA);

				// store register state for instr A
				regA = mips_state.reg;

				// set next pc
				mips_state.pc = tempNPC;

				// if instr A is not a jump, execute instruction B
				if(!is_branch_jump_B_prev && pc_A!=ADDR_NULL)
				{	
					tempNPC = mips_state.npc; // store next pc
					// fetch instr B
					instrB = mips_state.ram[mips_state.pc];
					// set pc for instr B
					pc_B = mips_state.pc;
					// send instr B for decode
					decode_inst(instrB,decodeB);

					// Execute instruction B
					r_type(mips_state,executedB,decodeB,is_mulDivB,is_jumpB,is_branchB,is_RB,is_md_non_stallB);
					i_type(mips_state,executedB,decodeB,is_loadB,is_storeB,is_branchB,is_IB,is_start);
					j_type(mips_state,executedB,decodeB,is_jumpB,is_JB);

					// set next pc
					mips_state.pc = tempNPC;
				}
				// if instr A is a jump, drop instr B
				else
				{
					instrB = NULL;
					is_mulDivB = false;
					is_jumpB = false;
					is_branchB = false;
					is_RB = false;
					is_md_non_stallB = false;
					is_loadB = false;
					is_storeB = false;
					is_IB = false;
					is_JB = false;
				}

				if(is_branch_jump_B_prev)
					is_branch_jump_B_prev = false;

				if(is_jumpB)
					is_branch_jump_B_prev = true;

				is_start = false;

				regB = mips_state.reg;
			}

			// check for hazards and branches
			checkHazardAndBranch(hazard, is_loadA, is_storeA, is_mulDivA, is_loadB, is_storeB, is_mulDivB, is_jumpA, is_branchA, is_jumpB, is_branchB, is_RA, is_IA, is_JA, is_RB, is_IB, is_JB, decodeA, decodeB, is_md_non_stallA, is_md_non_stallB, instrA, instrB);

			// The instructions just executed are sent down the pipeline. Even though the instructions have executed, they are shown to be in the "IF" stage to the user
			// in the next cycle they will be in the ID stage, then X1 and so on.
			// The line below moves all 4 pipe states (IFID, ALU, MEM, MULDIV) forward by one cycle 
			moveOneCycle(mips_state, pipeStateIFID, pipeStateMULDIV, pipeState_NextMULDIV, pipeStateALU, pipeState_NextALU, pipeStateMEM, pipeState_NextMEM, dstate, executedA, executedB, CurCycle, instrA, instrB, stalling, is_loadA, is_storeA, is_mulDivA, is_loadB, is_storeB, is_mulDivB, is_jumpA, is_branchA, is_jumpB, is_branchB, robState.tail, dstate.num_instrs, hazard, pc_A, pc_B, regA, regB, is_md_non_stallA, is_md_non_stallB, pause_for_jump_branch);

			// ROB Commit
			// If the instructions at the head of the ROB are no longer pending, 
			// send them to the commit and clear that slot by setting it to NOT VALID
			if ((!robState.pending[robState.head]) && (robState.valid[robState.head])) {
				robState.valid[robState.head] = false;
				robState.commitedA = true;
				robState.commit_instrA = robState.instr[robState.head];
				if (robState.head != 15) {
					robState.head += 1;
				} else {
					robState.head = 0;
				}
			}
			//double commit
			if ((!robState.pending[robState.head]) && (robState.valid[robState.head])) {
				robState.valid[robState.head] = false;
				robState.commitedB = true;
				robState.commit_instrB = robState.instr[robState.head];
				if (robState.head != 15) {
					robState.head += 1;
				} else {
					robState.head = 0;
				}
			}

			// get the decoded instructions in EX to allocate slots in the ROB
			decode_inst(pipeStateIFID.exInstrA,decodeA_ex);
			decode_inst(pipeStateIFID.exInstrB,decodeB_ex);

			// if a valid instruction has been issued to the X1 stage, and there is space in the ROB, 
			//then allocate an entry for the issued instruction
			if (pipeStateIFID.ex1A && (pipeStateIFID.exInstrA != NOP) && (robState.valid[robState.tail] != true) && (stalling != 1)) {
				robState.instr[robState.tail] = pipeStateIFID.exInstrA;
				robState.valid[robState.tail] = true;
				robState.pending[robState.tail] = true;
				robState.preg[robState.tail] = decodeA_ex.rt;
				if (robState.tail != 15) {
					robState.tail += 1;
				} else {
					robState.tail = 0;
				}	
			}
			if (pipeStateIFID.ex1B && (pipeStateIFID.exInstrB != NOP) && (robState.valid[robState.tail] != true) && (stalling != 1)) {
				robState.instr[robState.tail] = pipeStateIFID.exInstrB;
				robState.valid[robState.tail] = true;
				robState.pending[robState.tail] = true;
				robState.preg[robState.tail] = decodeB_ex.rt;
				if (robState.tail != 15) {
					robState.tail += 1;
				} else {
					robState.tail = 0;
				}	
			}

			// ROB Fill - set the pending bit of the ROB entry to 0 when the instruction reaches writeback
			if (pipeStateALU.wb_isval) {
				robState.pending[pipeStateALU.rob_fill_slot_wb] = false;
			}
			if (pipeStateMEM.wb_isval) {
				robState.pending[pipeStateMEM.rob_fill_slot_wb] = false;
			}
			if (pipeStateMULDIV.wb_isval) {
				robState.pending[pipeStateMULDIV.rob_fill_slot_wb] = false;
			}

			
			// Pipe Diagram Allocate
			// If valid instructions have been fetched in the IF stage and there is space in the pipe diagram, allocate a 
			// pipe diagram entry for the instruction
			if (pipeStateIFID.IFA && (pipeStateIFID.ifInstrA != NOP) && !dstate.is_full && (CurCycle < (DIAGRAM_CYCLES-1))) {
				dstate.instr[dstate.num_instrs].instr = pipeStateIFID.ifInstrA;
				dstate.instr[dstate.num_instrs].stage[CurCycle] = "IF\t";
				dstate.instr[dstate.num_instrs].done = false;
				
				if (dstate.num_instrs < (DIAGRAM_SIZE-1)) {
					dstate.num_instrs += 1;
				} else {
					dstate.is_full = true;
				}
			}
			if (pipeStateIFID.IFB && (pipeStateIFID.ifInstrB != NOP) && !dstate.is_full && (CurCycle < (DIAGRAM_CYCLES-1))) {
				dstate.instr[dstate.num_instrs].instr = pipeStateIFID.ifInstrB;
				dstate.instr[dstate.num_instrs].stage[CurCycle] = "IF\t";
				dstate.instr[dstate.num_instrs].done = false;
				
				if (dstate.num_instrs < (DIAGRAM_SIZE-1)) {
					dstate.num_instrs += 1;
				} else {
					dstate.is_full = true;
				}
			}
			
			// update the pipe diagram structure by marking the stage that each instruction is in at the current cycle
			// UNCOMMENT TO PRINT PIPELINE DIAGRAM
			//updatePipeDiagram(dstate, pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling, pipeStateIFID);
			
			// if stalling was present in the last cycle, mark it as zero and check again if this cycle will need to be stalled
			if(stalling == 1)
			{
				stalling = 0;
			}

			// compare in all three pipestates
			checkForStall(pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling, pipeStateIFID);

			//UNCOMMENT TO PRINT ROB STATE PER CYCLE
			//dumpROBState(robState);
			// UNCOMMENT TO PRINT PIPELINE DUMP
			//dumpPipeState(pipeStateALU, pipeStateMEM, pipeStateMULDIV, robState, pipeStateIFID,stalling);	

			CurCycle = CurCycle + 1;

			if(pipeStateALU.wbPC == ADDR_NULL){
				// UNCOMMENT TO PRINT PIPELINE DIAGRAM
				//dumpPipeDiagram(dstate);
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
		cout << "ERROR "<<CurCycle << " \n";
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
