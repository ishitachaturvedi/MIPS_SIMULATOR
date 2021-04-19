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
    uint32_t memInstr;
    uint32_t wbInstr;

    //Push PC
    uint32_t ifPC;
    uint32_t idPC;
    uint32_t ex1PC;
    uint32_t ex2PC;
    uint32_t ex3PC;
    uint32_t ex4PC;
    uint32_t memPC;
    uint32_t wbPC;

    //Push Reg
    std::vector<int32_t> ifreg;
    std::vector<int32_t> idreg;
    std::vector<int32_t> ex1reg;
    std::vector<int32_t> ex2reg;
    std::vector<int32_t> ex3reg;
    std::vector<int32_t> ex4reg;
    std::vector<int32_t> memreg;
    std::vector<int32_t> wbreg;

    //push execute
    bool IF;
    bool id;
    bool ex1;
    bool ex2;
    bool ex3;
    bool ex4;
    bool mem;
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

};

struct PipeState_Next
{

    //Push instructions
    uint32_t idInstr;
    uint32_t ex1Instr;
    uint32_t ex2Instr;
    uint32_t ex3Instr;
    uint32_t ex4Instr;
    uint32_t memInstr;
    uint32_t wbInstr;
    
    //Push PC
    uint32_t idPC;
    uint32_t ex1PC;
    uint32_t ex2PC;
    uint32_t ex3PC;
    uint32_t ex4PC;
    uint32_t memPC;
    uint32_t wbPC;

    //Push Reg
    std::vector<int32_t> idreg;
    std::vector<int32_t> ex1reg;
    std::vector<int32_t> ex2reg;
    std::vector<int32_t> ex3reg;
    std::vector<int32_t> ex4reg;
    std::vector<int32_t> memreg;
    std::vector<int32_t> wbreg;

    //push execute
    bool id;
    bool ex1;
    bool ex2;
    bool ex3;
    bool ex4;
    bool mem;
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
};

struct ROBState {

    uint32_t cycle;
    uint32_t instr[ROB_SIZE];
    bool pending[ROB_SIZE];
    bool valid[ROB_SIZE];
    uint32_t preg[ROB_SIZE];
    uint32_t head;
    uint32_t tail;

};

void dumpPipeState(PipeState & stateALU, PipeState & stateMEM, PipeState & stateMULDIV);
void dumpROBState(ROBState & robState);
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, uint32_t instr, int stalling, bool is_load, bool is_store, bool is_mulDiv, uint32_t rob_tail);
void initPipeline(PipeState_Next &pipeState_Next);
void initROB(ROBState &robState);
void checkForStall(PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling);

#endif /* DUMPHPP */