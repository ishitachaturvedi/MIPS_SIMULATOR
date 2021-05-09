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

		// clear contents of file
		std::ofstream ofs;
		ofs.open("pipe_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs.close();
		std::ofstream ofs1;
		ofs1.open("rob_state.out", std::ofstream::out | std::ofstream::trunc);
		ofs1.close();
		ofs1.open("pipe_diagram.out", std::ofstream::out | std::ofstream::trunc);
		ofs1.close();

		string fileName(argv[1]);
		int32_t tempNPC = 0;

		bool executedA;				//this flag is turned on when an instruction of one of the 3 types has been executed
		bool executedB;

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

		PipeStateIFID pipeStateIFID;

		Decode decodeA;
		Decode decodeB;

		Decode decodeA_ex;
		Decode decodeB_ex;

		int stalling = 0; //stall for 1 extra cycle for LD stalls which are resolved in mem stage

		mips_state.ram.resize(MEM_SIZE);	//This will allocate memory for the whole RAM

		bool is_loadA = false;
		bool is_storeA = false;
		bool is_mulDivA = false;
		bool is_branchA = false;
		bool is_jumpA = false;
		bool is_RA = false;
		bool is_IA = false;
		bool is_JA = false;

		bool is_loadB = false;
		bool is_storeB = false;
		bool is_mulDivB = false;
		bool is_branchB = false;
		bool is_jumpB = false;
		bool is_RB = false;
		bool is_IB = false;
		bool is_JB = false;
		bool hazard = false;

		bool is_md_non_stallA = false;
		bool is_md_non_stallB = false;

		bool is_exited = false;

		bool is_start = true;

		bool is_branch_jump_B_prev = false;

		std::vector<int32_t> regA;
    	std::vector<int32_t> regB;

		uint32_t instrA = NOP;
		uint32_t instrB = NOP;
		uint32_t pc_A = 0x1;
		uint32_t pc_B = 0x1;

		setUp(mips_state, fileName);		//Passes the instructions to the vector

		initPipelineIFID(pipeStateIFID);

		initPipeline(pipeState_NextALU);
		initPipeline(pipeState_NextMEM);
		initPipeline(pipeState_NextMULDIV);

		bool pause_for_jump_branch = false;

		for(;;){

			mips_state.reg[0] = 0;		//register $0 must retain the value zero in every new clock cycle of the processor
			executedA = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed
			executedB = false;		//every new clock cycle the flag is turned off since no instruction has yet been executed

			robState.cycle = CurCycle;
			robState.commited = false;
			robState.commit_instr = NOP;

			// Execute if not stalling
			if(stalling != 1 && pipeStateIFID.stall_state_IF != 1 && !is_exited && !pause_for_jump_branch)
			{
				flush(cout);
				instrA = mips_state.ram[mips_state.pc];
				pc_A = mips_state.pc;
				decode_inst(instrA,decodeA);
				tempNPC = mips_state.npc;
				
				//Execute Instructions
				r_type(mips_state,executedA,decodeA,is_mulDivA,is_jumpA,is_branchA,is_RA,is_md_non_stallA);
				i_type(mips_state,executedA,decodeA,is_loadA,is_storeA,is_branchA,is_IA,is_start);
				j_type(mips_state,executedA,decodeA,is_jumpA,is_JA);

				regA = mips_state.reg;

				mips_state.pc = tempNPC;

				if(!is_branch_jump_B_prev && pc_A!=ADDR_NULL)
				{	
					tempNPC = mips_state.npc;
					instrB = mips_state.ram[mips_state.pc];
					pc_B = mips_state.pc;
					decode_inst(instrB,decodeB);
					r_type(mips_state,executedB,decodeB,is_mulDivB,is_jumpB,is_branchB,is_RB,is_md_non_stallB);
					i_type(mips_state,executedB,decodeB,is_loadB,is_storeB,is_branchB,is_IB,is_start);
					j_type(mips_state,executedB,decodeB,is_jumpB,is_JB);
					mips_state.pc = tempNPC;
				}
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

			checkHazardAndBranch(hazard, is_loadA, is_storeA, is_mulDivA, is_loadB, is_storeB, is_mulDivB, is_jumpA, is_branchA, is_jumpB, is_branchB, is_RA, is_IA, is_JA, is_RB, is_IB, is_JB, decodeA, decodeB, is_md_non_stallA, is_md_non_stallB, instrA, instrB);

			moveOneCycle(mips_state, pipeStateIFID, pipeStateMULDIV, pipeState_NextMULDIV, pipeStateALU, pipeState_NextALU, pipeStateMEM, pipeState_NextMEM, dstate, executedA, executedB, CurCycle, instrA, instrB, stalling, is_loadA, is_storeA, is_mulDivA, is_loadB, is_storeB, is_mulDivB, is_jumpA, is_branchA, is_jumpB, is_branchB, robState.tail, dstate.num_instrs, hazard, pc_A, pc_B, regA, regB, is_md_non_stallA, is_md_non_stallB, pause_for_jump_branch);

			// ROB Commit
			if ((!robState.pending[robState.head]) && (robState.valid[robState.head])) {
				robState.valid[robState.head] = false;
				robState.commited = true;
				robState.commit_instr = robState.instr[robState.head];
				if (robState.head != 15) {
					robState.head += 1;
				} else {
					robState.head = 0;
				}
			}
			//double commit
			if ((!robState.pending[robState.head]) && (robState.valid[robState.head])) {
				robState.valid[robState.head] = false;
				robState.commited = true;
				robState.commit_instr = robState.instr[robState.head];
				if (robState.head != 15) {
					robState.head += 1;
				} else {
					robState.head = 0;
				}
			}

			decode_inst(pipeStateIFID.exInstrA,decodeA_ex);
			decode_inst(pipeStateIFID.exInstrB,decodeB_ex);

			if (pipeStateIFID.ex1A && (pipeStateIFID.exInstrA != NOP) && (robState.valid[robState.tail] != true)) {
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
			if (pipeStateIFID.ex1B && (pipeStateIFID.exInstrB != NOP) && (robState.valid[robState.tail] != true)) {
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

			// ROB Fill
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
			
			updatePipeDiagram(dstate, pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling, pipeStateIFID);
			

			if(stalling == 1)
			{
				stalling = 0;
			}

			// compare in all three pipestates
			checkForStall(pipeStateALU, pipeStateMEM, pipeStateMULDIV, stalling, pipeStateIFID);

			dumpROBState(robState);
			dumpPipeState(pipeStateALU, pipeStateMEM, pipeStateMULDIV, robState, pipeStateIFID,stalling);	

			CurCycle = CurCycle + 1;

			if(pipeStateALU.wbPC == ADDR_NULL){
				std::cout << "Dumping Pipe Diagram" << endl;
				dumpPipeDiagram(dstate);
			}
			
			checkExit(pipeStateALU.wbreg, pipeStateALU.wbPC,CurCycle);

			if(!pipeStateALU.wb){
				cout << " I AM ERROR\n";
				throw (static_cast<int>(Exception::INSTRUCTION));
			}		
			
		};

    }
//
	catch (const int EXIT_CODE){		//Exceptions and Errors are caught here
		cout << "ERROR "<<CurCycle << " \n";
		switch(EXIT_CODE){
			case 0xFFFFFFF6:
				cout << "ARITHMETIC\n";
				std::exit(static_cast<int>(Exception::ARITHMETIC));
			case 0xFFFFFFF5:
				cout << "MEMORY\n";
				std::exit(static_cast<int>(Exception::MEMORY));
			case 0xFFFFFFF4:
				cout << "INSTRUCTION\n";
				std::exit(static_cast<int>(Exception::INSTRUCTION));
			case 0xFFFFFFEC:
				cout << "IO\n";
				std::exit(static_cast<int>(Error::IO));
			default: ;
		}
	}	
	catch(...){				//If an error ocurrs that is of none of the defined above, then it is of unknown nature
		std::exit(static_cast<int>(Error::INTERNAL));
	}

}
