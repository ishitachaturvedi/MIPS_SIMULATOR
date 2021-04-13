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
    uint32_t cycle;
    uint32_t ifInstr;
    uint32_t idInstr;
    uint32_t exInstr;
    uint32_t memInstr;
    uint32_t wbInstr;
    uint32_t ifPC;
    uint32_t idPC;
    uint32_t exPC;
    uint32_t memPC;
    uint32_t wbPC;

    std::vector<int32_t> ifreg;
    std::vector<int32_t> idreg;
    std::vector<int32_t> exreg;
    std::vector<int32_t> memreg;
    std::vector<int32_t> wbreg;

    bool ex;
    bool mem;
    bool wb;

    bool if_isload;
    bool id_isload;
    bool ex_isload;
};

struct PipeState_Next
{
    uint32_t idInstr;
    uint32_t exInstr;
    uint32_t memInstr;
    uint32_t wbInstr;
    uint32_t idPC;
    uint32_t exPC;
    uint32_t memPC;
    uint32_t wbPC;

    std::vector<int32_t> idreg;
    std::vector<int32_t> exreg;
    std::vector<int32_t> memreg;
    std::vector<int32_t> wbreg;

    bool mem;
    bool wb;

    bool id_isload;
    bool ex_isload;

};

void dumpPipeState(PipeState & state);
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, int stalling, bool is_load);
void initPipeline(PipeState_Next &pipeState_Next);
void checkForStall(PipeState & pipeState, bool is_load, int &stalling);

#endif /* DUMPHPP */