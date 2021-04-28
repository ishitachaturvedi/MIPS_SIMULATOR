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
#define DIAGRAM_SIZE 50
#define DIAGRAM_CYCLES 50


struct PipeState
{

    uint32_t cycle;
    int pipe_type;

    //Push instructions
    uint32_t ifInstr;
    uint32_t idInstr;
    uint32_t ex1Instr;
    uint32_t ex2Instr;
    uint32_t ex3Instr;
    uint32_t ex4Instr;
    uint32_t wbInstr;

    //Push PC
    uint32_t ifPC;
    uint32_t idPC;
    uint32_t ex1PC;
    uint32_t ex2PC;
    uint32_t ex3PC;
    uint32_t ex4PC;
    uint32_t wbPC;

    //Push Reg
    std::vector<int32_t> ifreg;
    std::vector<int32_t> idreg;
    std::vector<int32_t> ex1reg;
    std::vector<int32_t> ex2reg;
    std::vector<int32_t> ex3reg;
    std::vector<int32_t> ex4reg;
    std::vector<int32_t> wbreg;

    //push execute
    bool IF;
    bool id;
    bool ex1;
    bool ex2;
    bool ex3;
    bool ex4;
    bool wb;

    //push load
    bool if_isload;
    bool id_isload;
    bool ex1_isload;
    bool ex2_isload;
    bool ex3_isload;
    bool ex4_isload;

    //push mulDiv
    bool if_isMulDiv;
    bool id_isMulDiv;
    bool ex1_isMulDiv;
    bool ex2_isMulDiv;
    bool ex3_isMulDiv;

    // ROB Fill slots
    uint32_t rob_fill_slot_if;
    uint32_t rob_fill_slot_id;
    uint32_t rob_fill_slot_ex1;
    uint32_t rob_fill_slot_ex2;
    uint32_t rob_fill_slot_ex3;
    uint32_t rob_fill_slot_ex4;
    uint32_t rob_fill_slot_wb;

    // Instruction is valid - for ROB
    bool if_isval;
    bool id_isval;
    bool ex1_isval;
    bool ex2_isval;
    bool ex3_isval;
    bool ex4_isval;
    bool wb_isval;

};

struct PipeState_Next
{

    //Push instructions
    uint32_t idInstr;
    uint32_t ex1Instr;
    uint32_t ex2Instr;
    uint32_t ex3Instr;
    uint32_t ex4Instr;
    uint32_t wbInstr;
    
    //Push PC
    uint32_t idPC;
    uint32_t ex1PC;
    uint32_t ex2PC;
    uint32_t ex3PC;
    uint32_t ex4PC;
    uint32_t wbPC;

    //Push Reg
    std::vector<int32_t> idreg;
    std::vector<int32_t> ex1reg;
    std::vector<int32_t> ex2reg;
    std::vector<int32_t> ex3reg;
    std::vector<int32_t> ex4reg;
    std::vector<int32_t> wbreg;

    //push execute
    bool id;
    bool ex1;
    bool ex2;
    bool ex3;
    bool ex4;
    bool wb;

    //push load
    bool id_isload;
    bool ex1_isload;
    bool ex2_isload;
    bool ex3_isload;
    bool ex4_isload;

    //push mulDiv
    bool id_isMulDiv;
    bool ex1_isMulDiv;
    bool ex2_isMulDiv;
    bool ex3_isMulDiv;

    // ROB Fill slots
    uint32_t rob_fill_slot_id;
    uint32_t rob_fill_slot_ex1;
    uint32_t rob_fill_slot_ex2;
    uint32_t rob_fill_slot_ex3;
    uint32_t rob_fill_slot_ex4;
    uint32_t rob_fill_slot_wb;

    // Instruction is valid - for ROB
    bool if_isval;
    bool id_isval;
    bool ex1_isval;
    bool ex2_isval;
    bool ex3_isval;
    bool ex4_isval;
    bool wb_isval;
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

    uint32_t cycles;
    struct Instr instr[DIAGRAM_SIZE];
    uint32_t num_instrs;
    bool is_full;

};

void dumpPipeState(PipeState & stateALU, PipeState & stateMEM, PipeState & stateMULDIV, ROBState & robState);
void dumpROBState(ROBState & robState);
void dumpPipeDiagram(DiagramState & dstate);
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, uint32_t instr, int stalling, bool is_load, bool is_store, bool is_mulDiv, uint32_t rob_tail);
void initPipeline(PipeState_Next &pipeState_Next);
void initROB(ROBState &robState);
void initDiagram(DiagramState &dstate);
void checkForStall(PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling);

#endif /* DUMPHPP */