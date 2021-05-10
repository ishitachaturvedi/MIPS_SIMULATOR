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

    bool IF;
    bool id;
    bool ex;
    bool mem;
    bool wb;

    bool if_isload;
    bool id_isload;
    bool ex_isload;

    // Instruction is valid
    bool if_isval;
    bool id_isval;
    bool ex_isval;
    bool mem_isval;
    bool wb_isval;

    // Pipe Diagram slot
    uint32_t diagram_slot_if;
    uint32_t diagram_slot_id;
    uint32_t diagram_slot_ex;
    uint32_t diagram_slot_mem;
    uint32_t diagram_slot_wb;
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

    bool id;
    bool ex;
    bool mem;
    bool wb;

    bool id_isload;
    bool ex_isload;

    // Instruction is valid - for ROB
    bool if_isval;
    bool id_isval;
    bool ex_isval;
    bool mem_isval;
    bool wb_isval;

    // Pipe Diagram slot
    uint32_t diagram_slot_id;
    uint32_t diagram_slot_ex;
    uint32_t diagram_slot_mem;
    uint32_t diagram_slot_wb;

};

struct Instr {

    uint32_t instr;
    std::string stage[DIAGRAM_CYCLES];
    bool done;
    uint32_t commit_cycle;

};

struct DiagramState {

    uint32_t cycle;
    struct Instr instr[DIAGRAM_SIZE];
    uint32_t num_instrs;
    bool is_full;

};

void dumpPipeState(PipeState & state);
void dumpPipeDiagram(DiagramState & dstate);
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, uint32_t instr, int stalling, bool is_load, uint32_t diagram_slot);
void initPipeline(PipeState_Next &pipeState_Next);
void initDiagram(DiagramState &dstate);
void checkForStall(PipeState & pipeState, bool is_load, int &stalling);
void updatePipeDiagram(DiagramState &dstate, PipeState &pipeState, int &stalling);


#endif /* DUMPHPP */