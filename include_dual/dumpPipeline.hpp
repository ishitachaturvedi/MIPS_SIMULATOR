#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <inttypes.h>
#include <errno.h>
#include <arpa/inet.h>
#include <vector>
#include "setUp.hpp"
#include "Decode.hpp"

#ifndef DUMPHPP
#define DUMPHPP

using namespace std;

#define ROB_SIZE 16

#define NOP 0x00000000

struct PipeState
{

    uint32_t cycle;
    int pipe_type;

    //Push instructions
    uint32_t ex1Instr;
    uint32_t ex2Instr;
    uint32_t ex3Instr;
    uint32_t ex4Instr;
    uint32_t wbInstr;

    //Push PC
    uint32_t ex1PC;
    uint32_t ex2PC;
    uint32_t ex3PC;
    uint32_t ex4PC;
    uint32_t wbPC;

    //Push Reg
    std::vector<int32_t> ex1reg;
    std::vector<int32_t> ex2reg;
    std::vector<int32_t> ex3reg;
    std::vector<int32_t> ex4reg;
    std::vector<int32_t> wbreg;

    //push execute
    bool ex1;
    bool ex2;
    bool ex3;
    bool ex4;
    bool wb;

    //push load
    bool ex1_isload;
    bool ex2_isload;
    bool ex3_isload;
    bool ex4_isload;

    //push mulDiv
    bool ex1_isMulDiv;
    bool ex2_isMulDiv;
    bool ex3_isMulDiv;

    // ROB Fill slots
    uint32_t rob_fill_slot_ex1;
    uint32_t rob_fill_slot_ex2;
    uint32_t rob_fill_slot_ex3;
    uint32_t rob_fill_slot_ex4;
    uint32_t rob_fill_slot_wb;

    // Instruction is valid - for ROB
    bool ex1_isval;
    bool ex2_isval;
    bool ex3_isval;
    bool ex4_isval;
    bool wb_isval;

    // Pipe Diagram slot
    uint32_t diagram_slot_ex1;
    uint32_t diagram_slot_ex2;
    uint32_t diagram_slot_ex3;
    uint32_t diagram_slot_ex4;
    uint32_t diagram_slot_wb;

};

struct PipeState_Next
{

    //Push instructions
    uint32_t ex1Instr;
    uint32_t ex2Instr;
    uint32_t ex3Instr;
    uint32_t ex4Instr;
    uint32_t wbInstr;
    
    //Push PC
    uint32_t ex1PC;
    uint32_t ex2PC;
    uint32_t ex3PC;
    uint32_t ex4PC;
    uint32_t wbPC;

    //Push Reg
    std::vector<int32_t> ex1reg;
    std::vector<int32_t> ex2reg;
    std::vector<int32_t> ex3reg;
    std::vector<int32_t> ex4reg;
    std::vector<int32_t> wbreg;

    //push execute
    bool ex1;
    bool ex2;
    bool ex3;
    bool ex4;
    bool wb;

    //push load
    bool ex1_isload;
    bool ex2_isload;
    bool ex3_isload;
    bool ex4_isload;

    //push mulDiv
    bool ex1_isMulDiv;
    bool ex2_isMulDiv;
    bool ex3_isMulDiv;

    // ROB Fill slots
    uint32_t rob_fill_slot_ex1;
    uint32_t rob_fill_slot_ex2;
    uint32_t rob_fill_slot_ex3;
    uint32_t rob_fill_slot_ex4;
    uint32_t rob_fill_slot_wb;

    // Instruction is valid - for ROB
    bool ex1_isval;
    bool ex2_isval;
    bool ex3_isval;
    bool ex4_isval;
    bool wb_isval;

    // Pipe Diagram slot
    uint32_t diagram_slot_ex1;
    uint32_t diagram_slot_ex2;
    uint32_t diagram_slot_ex3;
    uint32_t diagram_slot_ex4;
    uint32_t diagram_slot_wb;
};

struct PipeStateIFID
{
    uint32_t cycle;

    //Push instructions
    uint32_t ifInstrA;
    uint32_t ifInstrB;

    uint32_t idInstrA;
    uint32_t idInstrB;

    uint32_t exInstrA;
    uint32_t exInstrB;

    //Push PC
    uint32_t ifPCA;
    uint32_t idPCA;
    uint32_t exPCA;

    uint32_t ifPCB;
    uint32_t idPCB;
    uint32_t exPCB;

    //Push Reg
    std::vector<int32_t> ifregA;
    std::vector<int32_t> idregA;
    std::vector<int32_t> exregA;

    std::vector<int32_t> ifregB;
    std::vector<int32_t> idregB;
    std::vector<int32_t> exregB;

    //push execute
    bool IFA;
    bool IDA;
    bool ex1A;

    bool IFB;
    bool IDB;
    bool ex1B;

    //push load
    bool if_isloadA;
    bool id_isloadA;
    bool ex_isloadA;
    
    bool if_isstoreA;
    bool id_isstoreA;
    bool ex_isstoreA;

    bool if_isloadB;
    bool id_isloadB;
    bool ex_isloadB;

    bool if_isstoreB;
    bool id_isstoreB;
    bool ex_isstoreB;

    //push mulDiv
    bool if_isMulDivA;
    bool id_isMulDivA;
    bool ex_isMulDivA;

    bool if_isMulDivB;
    bool id_isMulDivB;
    bool ex_isMulDivB;

    // ROB Fill slots
    uint32_t rob_fill_slot_ifA;
    uint32_t rob_fill_slot_idA;
    uint32_t rob_fill_slot_exA;

    uint32_t rob_fill_slot_ifB;
    uint32_t rob_fill_slot_idB;
    uint32_t rob_fill_slot_exB;

    // Pipe Diagram slots
    uint32_t diagram_slot_ifA;
    uint32_t diagram_slot_idA;
    uint32_t diagram_slot_exA;

    uint32_t diagram_slot_ifB;
    uint32_t diagram_slot_idB;
    uint32_t diagram_slot_exB;

    // Instruction is valid - for ROB
    bool if_isvalA;
    bool id_isvalA;
    bool ex_isvalA;

    bool if_isvalB;
    bool id_isvalB;
    bool ex_isvalB;

    bool is_jumpA_IF;
    bool is_branchA_IF;
    bool is_jumpA_ID;
    bool is_branchA_ID; 
    bool is_jumpA_EX;  
    bool is_branchA_EX;

    bool is_jumpB_IF;
    bool is_branchB_IF;
    bool is_jumpB_ID;
    bool is_branchB_ID;
    bool is_jumpB_EX;
    bool is_branchB_EX;

    //Hazard
    int is_hazard_IF;
    int is_hazard_ID;
    int is_hazard_EX;

    //Stall state, keeps a note of whether to stall or and for how many cycles for a hazard
    int stall_state_IF;
    int stall_state_ID;
    int stall_state_EX;
};


struct ROBState {

    uint32_t cycle;
    uint32_t instr[ROB_SIZE];
    bool pending[ROB_SIZE];
    bool valid[ROB_SIZE];
    uint32_t preg[ROB_SIZE];
    uint32_t head;
    uint32_t tail;
    bool commited;
    uint32_t commit_instr;
};

struct Instr {

    uint32_t instr;
    string stage[DIAGRAM_CYCLES];
    bool done;
    uint32_t commit_cycle;

};

struct DiagramState {

    uint32_t cycle;
    struct Instr instr[DIAGRAM_SIZE];
    uint32_t num_instrs;
    bool is_full;

};

void dumpPipeState(PipeState & stateALU, PipeState & stateMEM, PipeState & stateMULDIV, ROBState & robState, PipeStateIFID & pipeStateIFID, int stalling);
void dumpROBState(ROBState & robState);
void dumpPipeDiagram(DiagramState & dstate);
void checkHazardAndBranch(bool& hazard, bool is_loadA, bool is_storeA, bool is_mulDivA, bool is_loadB, bool is_storeB, bool is_mulDivB,  bool&is_jumpA, bool&is_branchA, bool&is_jumpB, bool&is_branchB, bool&is_RA, bool&is_IA, bool&is_JA, bool&is_RB, bool&is_IB, bool&is_JB, Decode& decodeA, Decode& decodeB, bool&is_md_non_stallA, bool&is_md_non_stallB, uint32_t instrA, uint32_t instrB);
void moveOneCycle(State &mips_state, PipeStateIFID &pipeStateIFID, PipeState &pipeStateMULDIV, PipeState_Next &pipeState_NextMULDIV, PipeState &pipeStateALU, PipeState_Next &pipeState_NextALU, PipeState &pipeStateMEM, PipeState_Next &pipeState_NextMEM, DiagramState & dstate, bool executedA, bool executedB, int & CurCycle, uint32_t instrA, uint32_t instrB, int& stalling, bool is_loadA, bool is_storeA, bool is_mulDivA, bool is_loadB, bool is_storeB, bool is_mulDivB,  bool&is_jumpA, bool&is_branchA, bool&is_jumpB, bool&is_branchB, uint32_t rob_tail, uint32_t diagram_slot, bool& hazard, uint32_t pc_A, uint32_t pc_B, std::vector<int32_t> regA, std::vector<int32_t> regB, bool&is_md_non_stallA, bool&is_md_non_stallB, bool &pause_for_jump_branch);
void initPipeline(PipeState_Next &pipeState_Next);
void initPipelineIFID(PipeStateIFID &pipeStateIFID);
void initROB(ROBState &robState);
void initDiagram(DiagramState &dstate);
void checkForStall(PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling, PipeStateIFID &pipeStateIFID);
void MoveOneCycleIFID(PipeStateIFID &pipeStateIFID, uint32_t instrA, uint32_t instrB, uint32_t pc_A, uint32_t pc_B, uint32_t &rob_tail, uint32_t& diagram_slot, std::vector<int32_t> regA, std::vector<int32_t> regB, bool& hazard, bool&is_jumpA, bool&is_branchA, bool&is_jumpB, bool&is_branchB, bool is_loadA, bool is_storeA, bool is_mulDivA, bool is_loadB, bool is_storeB, bool is_mulDivB, int executedA, int executedB, int stall_state, bool is_valA, bool is_valB, int& stalling);
void moveStalledALU(PipeState &pipeState, PipeState_Next &pipeState_Next);
void moveStalledMEM(PipeState &pipeState, PipeState_Next &pipeState_Next);
void moveStalledMULDIV(PipeState &pipeState, PipeState_Next &pipeState_Next);
void doROBWork(PipeStateIFID &pipeStateIFID, ROBState & robState, PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV);
void MoveOneCycleIFIDPause(PipeStateIFID &pipeStateIFID, State &mips_state);void updatePipeDiagram(DiagramState &dstate, PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling);
void updatePipeDiagram(DiagramState &dstate, PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling, PipeStateIFID &pipeStateIFID);
void printInstr(uint32_t curInst, std::ostream & pipeState);


#endif /* DUMPHPP */