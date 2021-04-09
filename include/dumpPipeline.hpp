#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <inttypes.h>
#include <errno.h>
#include <arpa/inet.h>

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
};

struct PipeState_Next
{
    uint32_t idInstr;
    uint32_t exInstr;
    uint32_t memInstr;
    uint32_t wbInstr;
};

void dumpPipeState(PipeState & state);

#endif /* DUMPHPP */