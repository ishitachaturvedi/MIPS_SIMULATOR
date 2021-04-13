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

struct PipeState
{
    //Push instructions
    uint32_t cycle;
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
};

void dumpPipeState(PipeState & state);
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, int stalling, bool is_load, bool is_mulDiv);
void initPipeline(PipeState_Next &pipeState_Next);
void checkForStall(PipeState & pipeState, int &stalling);

#endif /* DUMPHPP */