#include "dumpPipeline.hpp"
#include <iostream>
#include <vector>

static void handleOpZeroInst(uint32_t instr, std::ostream & out_stream);
static void handleImmInst(uint32_t instr, std::ostream & out_stream);
static string getImmString(uint8_t opcode);
static uint8_t getOpcode(uint32_t instr);
static void handleJInst(uint32_t instr, std::ostream & out_stream);

#define NUM_REGS 32

#define ALU_PIPE 1
#define MEM_PIPE 2
#define MULDIV_PIPE 3

#define ROB_SIZE 16

#define NOP 0x00000000
#define ADDR_NULL 0x00000000

using namespace std;

//Printing code...
enum OP_IDS
{
   //R-type opcodes...
    OP_ZERO = 0,
    //I-type opcodes...
    OP_ADDI = 0x8,
    OP_ADDIU = 0x9,
    OP_ANDI = 0xc,
    OP_BEQ = 0x4,
    OP_BNE = 0x5,
    OP_BLEZ = 0x6, //added
    OP_BGTZ = 0x7, //added
    OP_LBU = 0x24,
    OP_LB = 0x20,
    OP_LHU = 0x25,
    OP_LH = 0x21,
    OP_LWL = 0x22, //added
    OP_LWR = 0x26, //added
    OP_LUI = 0xf,
    OP_LW = 0x23,
    OP_ORI = 0xd,
    OP_SLTI = 0xa,
    OP_SLTIU = 0xb,
    OP_SB = 0x28,
    OP_SC = 0x38,
    OP_SH = 0x29,
    OP_SW = 0x2b,
    OP_XORI = 0x0E, //added
    OP_BDECODER = 0x01, //added
    //J-type opcodes...
    OP_J = 0x2,
    OP_JAL = 0x3
};

enum FUN_IDS
{
    FUN_ADD = 0x20,
    FUN_ADDU = 0x21,
    FUN_AND = 0x24,
    FUN_JR = 0x08,
    FUN_NOR = 0x27,
    FUN_OR = 0x25,
    FUN_SLT = 0x2a,
    FUN_SLTU = 0x2b,
    FUN_SLL = 0x00,
    FUN_SRL = 0x02,
    FUN_SLLV = 0x04,
    FUN_SRLV = 0x06,
    FUN_SRA = 0x03,
    FUN_SUB = 0x22,
    FUN_SUBU = 0x23,
    FUN_MULT = 0x18,
    FUN_MULTU = 0x19,
    FUN_MFHI = 0x10,
    FUN_MFLO = 0x12,
    FUN_DIV = 0x1a,
    FUN_DIVU = 0x1b,
    FUN_JALR = 0x9, //added
    FUN_MTHI = 0x11,
    FUN_MTLO = 0x13,
    FUN_SRAV= 0x7,
    FUN_XOR = 0x26
};

void checkForStall(PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling, PipeStateIFID &pipeStateIFID)
{
    // Get current instruction in decode
    uint32_t instr_DA = pipeStateIFID.idInstrA;
    uint32_t instr_DB = pipeStateIFID.idInstrB;

    // Decode current instruction
    Decode idA;
    Decode idB;
    decode_inst(instr_DA, idA);
    decode_inst(instr_DB, idB);

    // Decode Mem instructions;
    Decode ex1MEM;
    Decode ex2MEM;
    Decode ex3MEM;
    decode_inst(pipeStateMEM.ex1Instr, ex1MEM);
    decode_inst(pipeStateMEM.ex2Instr, ex2MEM);
    decode_inst(pipeStateMEM.ex2Instr, ex3MEM);

    // Decode MUL DIV instructions;
    Decode ex1MD;
    Decode ex2MD;
    Decode ex3MD;
    Decode ex4MD;
    decode_inst(pipeStateMULDIV.ex1Instr, ex1MD);
    decode_inst(pipeStateMULDIV.ex2Instr, ex2MD);
    decode_inst(pipeStateMULDIV.ex3Instr, ex3MD);
    decode_inst(pipeStateMULDIV.ex4Instr, ex4MD);

    // if ex1 are load inst which is being waited on, we stall
    if(
        (pipeStateIFID.stall_state_EX !=1) && 
        (
            //((idA.rs == ex1MEM.rt || idA.rd == ex1MEM.rt) && ex1MEM.rt != 0x0 && pipeStateMEM.ex1_isload && !(pipeStateMEM.ex1Instr == pipeStateMEM.ex2Instr) && !(instr_DA == pipeStateMEM.ex1Instr))
            //|| ((idB.rs == ex1MEM.rt || idB.rd == ex1MEM.rt) && ex1MEM.rt != 0x0 && pipeStateMEM.ex1_isload && !(pipeStateMEM.ex1Instr == pipeStateMEM.ex2Instr) && !(instr_DA == pipeStateMEM.ex1Instr))

            ((idA.rs == ex1MEM.rt || idA.rd == ex1MEM.rt) && ex1MEM.rt != 0x0 && pipeStateMEM.ex1_isload && !(pipeStateMEM.ex1Instr == pipeStateMEM.ex3Instr) && !(instr_DA == pipeStateMEM.ex1Instr))
            || ((idB.rs == ex1MEM.rt || idB.rd == ex1MEM.rt) && ex1MEM.rt != 0x0 && pipeStateMEM.ex1_isload && !(pipeStateMEM.ex1Instr == pipeStateMEM.ex3Instr) && !(instr_DA == pipeStateMEM.ex1Instr))
            || ((idA.rs == ex2MEM.rt || idA.rd == ex2MEM.rt) && ex2MEM.rt != 0x0 && pipeStateMEM.ex2_isload && !(pipeStateMEM.ex2Instr == pipeStateMEM.ex3Instr) && !(instr_DA == pipeStateMEM.ex2Instr))
            || ((idB.rs == ex2MEM.rt || idB.rd == ex2MEM.rt) && ex2MEM.rt != 0x0 && pipeStateMEM.ex2_isload && !(pipeStateMEM.ex2Instr == pipeStateMEM.ex3Instr) && !(instr_DA == pipeStateMEM.ex2Instr))
        )
    )
    {
        stalling = 1;
    }

    // if ex1, ex2, ex3 are mulDiv inst which is being waited on, we stall
    if(
        (((idA.rs == ex1MD.rd || idA.rt == ex1MD.rd) && ex1MD.rd != 0x0 && pipeStateMULDIV.ex1_isMulDiv && !(pipeStateMULDIV.ex1Instr == pipeStateMULDIV.ex4Instr) && (pipeStateIFID.stall_state_ID !=1))
        || ((idA.rs == ex2MD.rd || idA.rt == ex2MD.rd) && ex2MD.rd != 0x0 && pipeStateMULDIV.ex2_isMulDiv && !(pipeStateMULDIV.ex2Instr == pipeStateMULDIV.ex4Instr))
        || ((idA.rs == ex3MD.rd || idA.rt == ex3MD.rd) && ex3MD.rd != 0x0 && pipeStateMULDIV.ex3_isMulDiv && !(pipeStateMULDIV.ex3Instr == pipeStateMULDIV.ex4Instr)))
        ||(((idB.rs == ex1MD.rd || idB.rt == ex1MD.rd) && ex1MD.rd != 0x0 && pipeStateMULDIV.ex1_isMulDiv && !(pipeStateMULDIV.ex1Instr == pipeStateMULDIV.ex4Instr))
        || ((idB.rs == ex2MD.rd || idB.rt == ex2MD.rd) && ex2MD.rd != 0x0 && pipeStateMULDIV.ex2_isMulDiv && !(pipeStateMULDIV.ex2Instr == pipeStateMULDIV.ex4Instr))
        || ((idB.rs == ex3MD.rd || idB.rt == ex3MD.rd) && ex3MD.rd != 0x0 && pipeStateMULDIV.ex3_isMulDiv && !(pipeStateMULDIV.ex3Instr == pipeStateMULDIV.ex4Instr)))
    )
    {
        stalling = 1;
    }
}

void checkHazardAndBranch(bool& hazard, bool is_loadA, bool is_storeA, bool is_mulDivA, bool is_loadB, bool is_storeB, bool is_mulDivB,  bool&is_jumpA, bool&is_branchA, bool&is_jumpB, bool&is_branchB, bool&is_RA, bool&is_IA, bool&is_JA, bool&is_RB, bool&is_IB, bool&is_JB, Decode& decodeA, Decode& decodeB, bool&is_md_non_stallA, bool&is_md_non_stallB, uint32_t instrA, uint32_t instrB)
{

    // CHECK FOR STR HAZARDS
    bool str_hazard = false;
    if((!is_jumpA && !is_branchA) && (instrA!=NOP && instrB!= NOP))
    {
        if(
            ((is_mulDivA||is_md_non_stallA) && (is_mulDivB||is_md_non_stallB))
            || ((is_loadA || is_storeA) && (is_loadB || is_storeB))
            || ((!is_mulDivA && !is_loadA && !is_storeA) && (!is_mulDivB && !is_loadB && !is_storeB))
        )
        {
            str_hazard = true;
        }
    }

    //CHECK FOR DATA STALLS
    bool mem_hazard = false;
    if(!is_jumpA && !is_branchA)
    {
        if(is_RA)
        {
            if(is_RB)
            {
                if
                ((decodeA.rd == decodeB.rs || decodeA.rd == decodeB.rt) && decodeA.rd!= 0x0)
                    mem_hazard = true;
            }
            else if(is_IB and !is_storeB)
            {
                if
                ((decodeA.rd == decodeB.rs) && decodeA.rd!= 0x0)
                    mem_hazard = true;
            }
        }
        if(is_IA)
        {
            if(is_RB)
            {
                if
                ((decodeA.rt == decodeB.rs || decodeA.rt == decodeB.rt) && decodeA.rt!= 0x0)
                    mem_hazard = true;
            }
            else if(is_IB and !is_storeB)
            {
                if
                ((decodeA.rt == decodeB.rs) && decodeA.rt!= 0x0)
                    mem_hazard = true;
            }
            else if(is_IB and is_storeB)
            {
                if
                ((decodeA.rt == decodeB.rt) && decodeA.rt!= 0x0)
                    mem_hazard = true;
            }
        }
    }

    //add branch delay slot
    bool delay = false;
    if(is_jumpA || is_branchA)
    {
        delay = true;
    }

    hazard = mem_hazard || str_hazard || delay;
}

void moveOneCycleMULDIV(PipeState &pipeState, PipeState_Next &pipeState_Next, uint32_t instr, bool executed, bool is_mulDiv, uint32_t rob_tail, uint32_t diagram_slot, uint32_t PC, std::vector<int32_t> reg, bool valid)
{
    // Instruction shifting
    pipeState.ex1Instr = instr;
    pipeState.ex2Instr = pipeState_Next.ex2Instr;
    pipeState.ex3Instr = pipeState_Next.ex3Instr;
    pipeState.ex4Instr = pipeState_Next.ex4Instr;
    pipeState.wbInstr = pipeState_Next.wbInstr;

    pipeState_Next.wbInstr = pipeState_Next.ex4Instr;
    pipeState_Next.ex4Instr = pipeState_Next.ex3Instr;
    pipeState_Next.ex3Instr = pipeState_Next.ex2Instr;
    pipeState_Next.ex2Instr = instr;

    //PC setting
    pipeState.ex1PC = PC;
    pipeState.ex2PC = pipeState_Next.ex2PC;
    pipeState.ex3PC = pipeState_Next.ex3PC;
    pipeState.ex4PC = pipeState_Next.ex4PC;
    pipeState.wbPC = pipeState_Next.wbPC;
    
    pipeState_Next.wbPC = pipeState_Next.ex4PC;
    pipeState_Next.ex4PC = pipeState_Next.ex3PC;
    pipeState_Next.ex3PC = pipeState_Next.ex2PC;
    pipeState_Next.ex2PC = PC;

    //reg setting
    pipeState.ex1reg = reg;
    pipeState.ex2reg = pipeState_Next.ex2reg;
    pipeState.ex3reg = pipeState_Next.ex3reg;
    pipeState.ex4reg = pipeState_Next.ex4reg;
    pipeState.wbreg = pipeState_Next.wbreg;
    
    pipeState_Next.wbreg = pipeState_Next.ex4reg;
    pipeState_Next.ex4reg = pipeState_Next.ex3reg;
    pipeState_Next.ex3reg = pipeState_Next.ex2reg;
    pipeState_Next.ex2reg = reg;

    //execute setting

    pipeState.ex1 = executed;
    pipeState.ex2 = pipeState_Next.ex2;
    pipeState.ex3 = pipeState_Next.ex3;
    pipeState.ex4 = pipeState_Next.ex4;
    pipeState.wb = pipeState_Next.wb;

    pipeState_Next.wb = pipeState_Next.ex4;
    pipeState_Next.ex4 = pipeState_Next.ex3;
    pipeState_Next.ex3 = pipeState_Next.ex2;
    pipeState_Next.ex2 = executed;

    //is_mulDiv
    pipeState.ex1_isMulDiv = is_mulDiv;
    pipeState.ex2_isMulDiv = pipeState_Next.ex2_isMulDiv;
    pipeState.ex3_isMulDiv = pipeState_Next.ex3_isMulDiv;

    pipeState_Next.ex3_isMulDiv = pipeState_Next.ex2_isMulDiv;
    pipeState_Next.ex2_isMulDiv = is_mulDiv;

    // ROB fill slots
    pipeState.rob_fill_slot_ex1 = rob_tail;
    pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
    pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
    pipeState.rob_fill_slot_ex4 = pipeState_Next.rob_fill_slot_ex4;
    pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex4;
    pipeState_Next.rob_fill_slot_ex4 = pipeState_Next.rob_fill_slot_ex3;
    pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
    pipeState_Next.rob_fill_slot_ex2 = rob_tail;

    // Pipe Diagram slots
    pipeState.diagram_slot_ex1 = diagram_slot;
    pipeState.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex2;
    pipeState.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex3;
    pipeState.diagram_slot_ex4 = pipeState_Next.diagram_slot_ex4;
    pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

    pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex4;
    pipeState_Next.diagram_slot_ex4 = pipeState_Next.diagram_slot_ex3;
    pipeState_Next.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex2;
    pipeState_Next.diagram_slot_ex2 = diagram_slot;


    // INSTRUCTION Valid
    pipeState.ex1_isval = valid;
    pipeState.ex2_isval = pipeState_Next.ex2_isval;
    pipeState.ex3_isval = pipeState_Next.ex3_isval;
    pipeState.ex4_isval = pipeState_Next.ex4_isval;
    pipeState.wb_isval = pipeState_Next.wb_isval;

    pipeState_Next.wb_isval = pipeState_Next.ex4_isval;
    pipeState_Next.ex4_isval = pipeState_Next.ex3_isval;
    pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
    pipeState_Next.ex2_isval = valid;
}

void moveOneCycleALU(PipeState &pipeState, PipeState_Next &pipeState_Next, uint32_t instr, bool executed, uint32_t rob_tail, uint32_t diagram_slot, uint32_t PC, std::vector<int32_t> reg, bool valid)
{

    // Instruction shifting
    pipeState.ex1Instr = instr;
    pipeState.wbInstr = pipeState_Next.wbInstr;

    pipeState_Next.wbInstr = instr;

    //PC setting
    pipeState.ex1PC = PC;
    pipeState.wbPC = pipeState_Next.wbPC;
    
    pipeState_Next.wbPC = PC;

    //reg setting
    pipeState.ex1reg = reg;
    pipeState.wbreg = pipeState_Next.wbreg;
    
    pipeState_Next.wbreg = reg;

    //execute setting

    pipeState.ex1 = executed;
    pipeState.wb = pipeState_Next.wb;

    pipeState_Next.wb = executed;

    // ROB fill slots
    pipeState.rob_fill_slot_ex1 = rob_tail;
    pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    pipeState_Next.rob_fill_slot_wb = rob_tail;

    // Pipe Diagram slots
    pipeState.diagram_slot_ex1 = diagram_slot;
    pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

    pipeState_Next.diagram_slot_wb = diagram_slot;

    // iNSTRUCTION Valid
    pipeState.ex1_isval = valid;
    pipeState.wb_isval = pipeState_Next.wb_isval;

    pipeState_Next.wb_isval = valid;
}

void moveOneCycleMEM(PipeState &pipeState, PipeState_Next &pipeState_Next, uint32_t instr, bool executed, bool is_load, uint32_t rob_tail, uint32_t diagram_slot, uint32_t PC, std::vector<int32_t> reg, bool valid)
{
    // Instruction shifting
    pipeState.ex1Instr = instr;
    pipeState.ex2Instr = pipeState_Next.ex2Instr;
    pipeState.ex3Instr = pipeState_Next.ex3Instr;
    pipeState.wbInstr = pipeState_Next.wbInstr;

    pipeState_Next.wbInstr = pipeState_Next.ex3Instr;
    pipeState_Next.ex3Instr = pipeState_Next.ex2Instr;
    pipeState_Next.ex2Instr = instr;

    //PC setting
    pipeState.ex1PC = PC;
    pipeState.ex2PC = pipeState_Next.ex2PC;
    pipeState.ex3PC = pipeState_Next.ex3PC;
    pipeState.wbPC = pipeState_Next.wbPC;
    
    pipeState_Next.wbPC = pipeState_Next.ex3PC;
    pipeState_Next.ex3PC = pipeState_Next.ex2PC;
    pipeState_Next.ex2PC = PC;

    //reg setting
    pipeState.ex1reg = reg;
    pipeState.ex2reg = pipeState_Next.ex2reg;
    pipeState.ex3reg = pipeState_Next.ex3reg;
    pipeState.wbreg = pipeState_Next.wbreg;
    
    pipeState_Next.wbreg = pipeState_Next.ex3reg;
    pipeState_Next.ex3reg = pipeState_Next.ex2reg;
    pipeState_Next.ex2reg = reg;

    //execute setting

    pipeState.ex1 = executed;
    pipeState.ex2 = pipeState_Next.ex2;
    pipeState.ex3 = pipeState_Next.ex3;
    pipeState.wb = pipeState_Next.wb;

    pipeState_Next.wb = pipeState_Next.ex3;
    pipeState_Next.ex3 = pipeState_Next.ex2;
    pipeState_Next.ex2 = executed;

    //is_load
    pipeState.ex1_isload = is_load;
    pipeState.ex2_isload = pipeState_Next.ex2_isload;
    pipeState.ex3_isload = pipeState_Next.ex3_isload;
    
    pipeState_Next.ex3_isload = pipeState_Next.ex2_isload;
    pipeState_Next.ex2_isload = is_load;

    // ROB fill slots
    pipeState.rob_fill_slot_ex1 = rob_tail;
    pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
    pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
    pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex3;
    pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
    pipeState_Next.rob_fill_slot_ex2 = rob_tail;

    // iNSTRUCTION Valid
    pipeState.ex1_isval = valid;
    pipeState.ex2_isval = pipeState_Next.ex2_isval;
    pipeState.ex3_isval = pipeState_Next.ex3_isval;
    pipeState.wb_isval = pipeState_Next.wb_isval;

    pipeState_Next.wb_isval = pipeState_Next.ex3_isval;
    pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
    pipeState_Next.ex2_isval = valid;
}

void MoveOneCycleIFIDPause(PipeStateIFID &pipeStateIFID, State &mips_state)
{
    pipeStateIFID.stall_state_IF = 0;
    pipeStateIFID.ifInstrA = mips_state.ram[pipeStateIFID.idPCA+2];
    pipeStateIFID.ifInstrB = mips_state.ram[pipeStateIFID.idPCB+2];
    pipeStateIFID.ifPCA = pipeStateIFID.idPCA+2;
    pipeStateIFID.ifPCB = pipeStateIFID.idPCB+2;

    pipeStateIFID.is_hazard_IF = 0;
    pipeStateIFID.is_jumpA_IF = 0;
    pipeStateIFID.is_branchA_IF = 0;
    pipeStateIFID.is_jumpB_IF = 0;
    pipeStateIFID.is_branchB_IF = 0;
    pipeStateIFID.if_isMulDivA = 0;
    pipeStateIFID.if_isMulDivB = 0;
    pipeStateIFID.if_isloadA = 0;
    pipeStateIFID.if_isloadB = 0;
    pipeStateIFID.if_isstoreA = 0;
    pipeStateIFID.if_isstoreB = 0;
    pipeStateIFID.IFA = 0;
    pipeStateIFID.IFB = 0;
    pipeStateIFID.if_isvalA = 0;
    pipeStateIFID.if_isvalB = 0;
}

void MoveOneCycleIFID(PipeStateIFID &pipeStateIFID, DiagramState & dstate, uint32_t instrA, uint32_t instrB, uint32_t pc_A, uint32_t pc_B, uint32_t &rob_tail, uint32_t diagram_slot, std::vector<int32_t> regA, std::vector<int32_t> regB, bool& hazard, bool&is_jumpA, bool&is_branchA, bool&is_jumpB, bool&is_branchB, bool is_loadA, bool is_storeA, bool is_mulDivA, bool is_loadB, bool is_storeB, bool is_mulDivB, int executedA, int executedB, int stall_state, bool is_valA, bool is_valB, int& stalling, int CurCycle)
{

    //pass stall stage
    //if(stalling != 1)
    {
        pipeStateIFID.stall_state_EX = pipeStateIFID.stall_state_ID;
        pipeStateIFID.stall_state_ID = pipeStateIFID.stall_state_IF;
        pipeStateIFID.stall_state_IF = stall_state;
    }

    //pass instructions
    if ( pipeStateIFID.stall_state_EX == 0 && stalling != 1)
    {
        pipeStateIFID.exInstrA = pipeStateIFID.idInstrA;
        pipeStateIFID.idInstrA = pipeStateIFID.ifInstrA;
        pipeStateIFID.ifInstrA = instrA;
        
        pipeStateIFID.exInstrB = pipeStateIFID.idInstrB;
        pipeStateIFID.idInstrB = pipeStateIFID.ifInstrB;
        pipeStateIFID.ifInstrB = instrB;

        //pass ROB tail
        pipeStateIFID.rob_fill_slot_exA = rob_tail;

        uint32_t rob_tailB;
        if(rob_tail < ROB_SIZE-1)
            rob_tail = rob_tail + 1;
        else
            rob_tail = 0;

        pipeStateIFID.rob_fill_slot_exB = rob_tail;

        if(CurCycle > 2)
        {
            pipeStateIFID.ex1A = true;
            pipeStateIFID.ex1B = true;

            pipeStateIFID.ex_isvalA = true;
            pipeStateIFID.ex_isvalB = true;
        }

    }

    else if ( pipeStateIFID.stall_state_EX == 2 && stalling != 1)
    {
        pipeStateIFID.exInstrA = pipeStateIFID.idInstrA;
        pipeStateIFID.ifInstrA = instrA;
        pipeStateIFID.idInstrA = NOP;
        
        pipeStateIFID.exInstrB = NOP;
        pipeStateIFID.ifInstrB = instrB;

        pipeStateIFID.rob_fill_slot_exA = rob_tail;

        pipeStateIFID.ex1A = true;
        pipeStateIFID.ex1B = false;

        pipeStateIFID.diagram_slot_exA = diagram_slot;

        pipeStateIFID.ex_isvalA = true;
        pipeStateIFID.ex_isvalB = false;

    }

    else if ( pipeStateIFID.stall_state_EX == 1 && stalling != 1)
    {
        pipeStateIFID.exInstrA = NOP;
        pipeStateIFID.idInstrA = pipeStateIFID.ifInstrA;
        pipeStateIFID.ifInstrA = instrA;
        
        pipeStateIFID.exInstrB = pipeStateIFID.idInstrB;
        pipeStateIFID.idInstrB = pipeStateIFID.ifInstrB;
        pipeStateIFID.ifInstrB = instrB;

        pipeStateIFID.rob_fill_slot_exB = rob_tail;

        pipeStateIFID.ex1A = false;
        pipeStateIFID.ex1B = true;

        pipeStateIFID.diagram_slot_exB = diagram_slot;

        pipeStateIFID.ex_isvalA = false;
        pipeStateIFID.ex_isvalB = true;
    }

    {
        pipeStateIFID.exPCA = pipeStateIFID.idPCA;
        pipeStateIFID.idPCA = pipeStateIFID.ifPCA;
        pipeStateIFID.ifPCA = pc_A;

        pipeStateIFID.exPCB = pipeStateIFID.idPCB;
        pipeStateIFID.idPCB = pipeStateIFID.ifPCB;
        pipeStateIFID.ifPCB = pc_B;
    }

    // Pass register states
    pipeStateIFID.exregA = pipeStateIFID.idregA;
    pipeStateIFID.idregA = pipeStateIFID.ifregA;
    pipeStateIFID.ifregA = regA;

    pipeStateIFID.exregB = pipeStateIFID.idregB;
    pipeStateIFID.idregB = pipeStateIFID.ifregB;
    pipeStateIFID.ifregB = regB;

    //pass hazard from ID to EX
    if(stalling != 1)
    {
        pipeStateIFID.is_hazard_EX = pipeStateIFID.is_hazard_ID;

        pipeStateIFID.is_jumpA_EX = pipeStateIFID.is_jumpA_ID;

        pipeStateIFID.is_branchA_EX = pipeStateIFID.is_branchA_ID;

        pipeStateIFID.is_jumpB_EX = pipeStateIFID.is_jumpB_ID;

        pipeStateIFID.is_branchB_EX = pipeStateIFID.is_branchB_ID;

        pipeStateIFID.ex_isMulDivA = pipeStateIFID.id_isMulDivA;

        pipeStateIFID.ex_isMulDivB = pipeStateIFID.id_isMulDivB;

        pipeStateIFID.ex_isloadA = pipeStateIFID.id_isloadA;

        pipeStateIFID.ex_isloadB = pipeStateIFID.id_isloadB;

        pipeStateIFID.ex_isstoreA = pipeStateIFID.id_isstoreA;

        pipeStateIFID.ex_isstoreB = pipeStateIFID.id_isstoreB;
    }

    //pass hazard from IF to ID
    if(pipeStateIFID.stall_state_ID != 1 && stalling != 1)
    {
        pipeStateIFID.is_hazard_ID = pipeStateIFID.is_hazard_IF;

        pipeStateIFID.is_jumpA_ID = pipeStateIFID.is_jumpA_IF;
        
        pipeStateIFID.is_branchA_ID = pipeStateIFID.is_branchA_IF;

        pipeStateIFID.is_jumpB_ID = pipeStateIFID.is_jumpB_IF;

        pipeStateIFID.is_branchB_ID = pipeStateIFID.is_branchB_IF;

        //Pass MULDIV, LOAD/STORE ETC
        pipeStateIFID.id_isMulDivA = pipeStateIFID.if_isMulDivA;

        pipeStateIFID.id_isMulDivB = pipeStateIFID.if_isMulDivB;

        pipeStateIFID.id_isloadA = pipeStateIFID.if_isloadA;

        pipeStateIFID.id_isloadB = pipeStateIFID.if_isloadB;

        pipeStateIFID.id_isstoreA = pipeStateIFID.if_isstoreA;

        pipeStateIFID.id_isstoreB = pipeStateIFID.if_isstoreB;
    
    }

    if(pipeStateIFID.stall_state_EX != 1)
    {
        //change if only when not stalling front end
    
        pipeStateIFID.is_hazard_IF = hazard;
        pipeStateIFID.is_jumpA_IF = is_jumpA;
        pipeStateIFID.is_branchA_IF = is_branchA;
        pipeStateIFID.is_jumpB_IF = is_jumpB;
        pipeStateIFID.is_branchB_IF = is_branchB;
        pipeStateIFID.if_isMulDivA = is_mulDivA;
        pipeStateIFID.if_isMulDivB = is_mulDivB;
        pipeStateIFID.if_isloadA = is_loadA;
        pipeStateIFID.if_isloadB = is_loadB;
        pipeStateIFID.if_isstoreA = is_storeA;
        pipeStateIFID.if_isstoreB = is_storeB;
        pipeStateIFID.IFA = executedA;
        pipeStateIFID.IFB = executedB;
    }

    // if (stalling == 1)
    // {
    //     pipeStateIFID.exInstrA = NOP;
    //     pipeStateIFID.exInstrB = NOP;
    // }

}

void moveStalledALU(PipeState &pipeState, PipeState_Next &pipeState_Next)
{
    pipeState.wbInstr = pipeState.ex1Instr;

    pipeState_Next.wbInstr =  pipeState.ex1Instr;

    // ROB fill slots
    pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex1;

    // iNSTRUCTION Valid
    pipeState.ex2_isval = pipeState_Next.ex2_isval;
    pipeState.wb_isval = pipeState_Next.wb_isval;

    pipeState_Next.wb_isval = pipeState_Next.ex2_isval;
    pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;
}

void moveStalledMEM(PipeState &pipeState, PipeState_Next &pipeState_Next)
{
    pipeState.wbInstr = pipeState.ex3Instr;
    pipeState.ex3Instr = pipeState.ex2Instr;
    pipeState.ex2Instr = pipeState.ex1Instr;

    //pipeState_Next.wbInstr =  pipeState.ex2Instr;

    pipeState_Next.wbInstr =  pipeState.ex3Instr;
    pipeState_Next.ex3Instr = pipeState.ex2Instr;

    // ROB fill slots
    // pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
    // pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    // pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex2;
    // pipeState_Next.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex1;

    pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
    pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
    pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex3;
    pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
    pipeState_Next.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex1;

    // iNSTRUCTION Valid
    // pipeState.ex2_isval = pipeState_Next.ex2_isval;
    // pipeState.wb_isval = pipeState_Next.wb_isval;

    // pipeState_Next.wb_isval = pipeState_Next.ex2_isval;
    // pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;

    pipeState.ex2_isval = pipeState_Next.ex2_isval;
    pipeState.ex3_isval = pipeState_Next.ex3_isval;
    pipeState.wb_isval = pipeState_Next.wb_isval;

    pipeState_Next.wb_isval = pipeState_Next.ex3_isval;
    pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
    pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;
}

void moveStalledMULDIV(PipeState &pipeState, PipeState_Next &pipeState_Next)
{
    pipeState.wbInstr = pipeState.ex4Instr;
    pipeState.ex4Instr = pipeState.ex3Instr;
    pipeState.ex3Instr = pipeState.ex2Instr;
    pipeState.ex2Instr = pipeState.ex1Instr;

    pipeState_Next.wbInstr =  pipeState.ex4Instr;
    pipeState_Next.ex4Instr =  pipeState.ex3Instr;
    pipeState_Next.ex3Instr = pipeState.ex2Instr;


    // ROB fill slots
    pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
    pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
    pipeState.rob_fill_slot_ex4 = pipeState_Next.rob_fill_slot_ex4;
    pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

    pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex4;
    pipeState_Next.rob_fill_slot_ex4 = pipeState_Next.rob_fill_slot_ex3;
    pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
    pipeState_Next.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex1;


    // iNSTRUCTION Valid
    pipeState.ex2_isval = pipeState_Next.ex2_isval;
    pipeState.ex3_isval = pipeState_Next.ex3_isval;
    pipeState.ex4_isval = pipeState_Next.ex4_isval;
    pipeState.wb_isval = pipeState_Next.wb_isval;

    pipeState_Next.wb_isval = pipeState_Next.ex4_isval;
    pipeState_Next.ex4_isval = pipeState_Next.ex3_isval;
    pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
    pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;
}

void moveOneCycle(State &mips_state, PipeStateIFID &pipeStateIFID, PipeState &pipeStateMULDIV, PipeState_Next &pipeState_NextMULDIV, PipeState &pipeStateALU, PipeState_Next &pipeState_NextALU, PipeState &pipeStateMEM, PipeState_Next &pipeState_NextMEM, DiagramState & dstate, bool executedA, bool executedB, int &CurCycle, uint32_t instrA, uint32_t instrB, int& stalling, bool is_loadA, bool is_storeA, bool is_mulDivA, bool is_loadB, bool is_storeB, bool is_mulDivB,  bool&is_jumpA, bool&is_branchA, bool&is_jumpB, bool&is_branchB, uint32_t rob_tail, uint32_t diagram_slot, bool& hazard, uint32_t pc_A, uint32_t pc_B, std::vector<int32_t> regA, std::vector<int32_t> regB, bool&is_md_non_stallA, bool&is_md_non_stallB, bool & pause_for_jump_branch)
{
    //ofstream piper("pipe_test.out", ios::app);
    uint32_t instrALU = NOP;
    uint32_t instrMEM = NOP;
    uint32_t instrMULDIV = NOP;

    int ALU_DONE = 0;
    int MEM_DONE = 0;
    int MULDIV_DONE = 0;

    bool executed = pipeStateIFID.ex1A || pipeStateIFID.ex1B;   

    bool is_MulDiv = false;
    bool is_load = false;

    uint32_t rob_tail_ALU = 0;
    uint32_t rob_tail_MEM = 0;
    uint32_t rob_tail_MULDIV = 0;

    uint32_t diagram_slot_ALU = 0;
    uint32_t diagram_slot_MEM = 0;
    uint32_t diagram_slot_MULDIV = 0;    

    uint32_t pc_MULDIV = 1;
    uint32_t pc_MEM = 1;
    uint32_t pc_ALU = 1;

    std::vector<int32_t> reg_MULDIV;
    std::vector<int32_t> reg_MEM;
    std::vector<int32_t> reg_ALU;

    bool valid_MULDIV = false;
    bool valid_MEM = false;
    bool valid_ALU = false;

    is_mulDivA = is_mulDivA || is_md_non_stallA;
    is_mulDivB = is_mulDivB || is_md_non_stallB;

    int stall_state = 0;

    if(pipeStateIFID.stall_state_IF != 0)
    {
        stall_state = pipeStateIFID.stall_state_IF - 1;
        executed = true;
    }

    if(stall_state == 0 && stalling != 1)
    {
        if (hazard)
        {
            stall_state = 2;
        }
        executed = true;
    }

    //instr is VALID for ROB if non NOP and B is valid if A has no jumps or taken branches
    bool is_valA = (executedA && (instrA != NOP));
    bool is_valB = (executedB && (instrB != NOP) && !is_jumpA &&!is_branchA); 

    MoveOneCycleIFID(pipeStateIFID, dstate, instrA, instrB, pc_A, pc_B, rob_tail, diagram_slot, regA, regB, hazard, is_jumpA, is_branchA, is_jumpB, is_branchB, is_loadA, is_storeA, is_mulDivA, is_loadB, is_storeB, is_mulDivB, executedA, executedB, stall_state, is_valA, is_valB, stalling, CurCycle);

    if(pause_for_jump_branch)
    {
        pause_for_jump_branch = false;
    }

    bool isALU_B = !pipeStateIFID.ex_isMulDivB && !(pipeStateIFID.ex_isloadB || pipeStateIFID.ex_isstoreB);
    bool isALU_A = !pipeStateIFID.ex_isMulDivA && !(pipeStateIFID.ex_isloadA || pipeStateIFID.ex_isstoreA) && !(pipeStateIFID.exInstrA==NOP && (pipeStateIFID.exInstrB!=NOP || pipeStateIFID.exPCB == ADDR_NULL) && isALU_B);

    //initialise with latest value of registers
    reg_MULDIV = pipeStateIFID.exregB;
    reg_MEM = pipeStateIFID.exregB;
    reg_ALU = pipeStateIFID.exregB;

    pipeStateIFID.cycle = CurCycle;

    if(stalling != 1)
    {
        //if no hazards
        if(pipeStateIFID.stall_state_EX == 0)
        {
            //if a jump, only instA will set a value
            // set inst A
            if (pipeStateIFID.ex_isMulDivA && MULDIV_DONE == 0)
            {
                instrMULDIV = pipeStateIFID.exInstrA;
                is_MulDiv = pipeStateIFID.ex_isMulDivA;
                rob_tail_MULDIV = pipeStateIFID.rob_fill_slot_exA;
                diagram_slot_MULDIV = pipeStateIFID.diagram_slot_exA;
                pc_MULDIV = pipeStateIFID.exPCA;
                reg_MULDIV = pipeStateIFID.exregA;
                valid_MULDIV = pipeStateIFID.ex_isvalA;
                MULDIV_DONE = 1;
            }
            else if ((pipeStateIFID.ex_isloadA || pipeStateIFID.ex_isstoreA) && MEM_DONE == 0) // if is_load or is_store, send down pipe2
            {
                instrMEM = pipeStateIFID.exInstrA;
                rob_tail_MEM = pipeStateIFID.rob_fill_slot_exA;
                diagram_slot_MEM = pipeStateIFID.diagram_slot_exA;
                is_load = pipeStateIFID.ex_isloadA;
                pc_MEM = pipeStateIFID.exPCA;
                reg_MEM = pipeStateIFID.exregA;
                valid_MEM = pipeStateIFID.ex_isvalA;
                MEM_DONE = 1;
            }
            else if(isALU_A && ALU_DONE == 0) // otherwise send down pipe 1
            {
                instrALU = pipeStateIFID.exInstrA;
                rob_tail_ALU = pipeStateIFID.rob_fill_slot_exA;
                diagram_slot_ALU = pipeStateIFID.diagram_slot_exA;
                pc_ALU = pipeStateIFID.exPCA;
                reg_ALU = pipeStateIFID.exregA;
                valid_ALU = pipeStateIFID.ex_isvalA;
                ALU_DONE = 1;
            }

            if(!pipeStateIFID.is_jumpA_ID)
            {
                if (pipeStateIFID.ex_isMulDivB && MULDIV_DONE == 0)
                {
                    instrMULDIV = pipeStateIFID.exInstrB;
                    is_MulDiv = pipeStateIFID.ex_isMulDivB;
                    rob_tail_MULDIV = pipeStateIFID.rob_fill_slot_exB;
                    diagram_slot_MULDIV = pipeStateIFID.diagram_slot_exB;
                    pc_MULDIV = pipeStateIFID.exPCB;
                    reg_MULDIV = pipeStateIFID.exregB;
                    valid_MULDIV = pipeStateIFID.ex_isvalB;
                    MULDIV_DONE = 1;
                }
                else if ((pipeStateIFID.ex_isloadB || pipeStateIFID.ex_isstoreB) && MEM_DONE == 0) // if is_load or is_store, send down pipe2
                {
                    instrMEM = pipeStateIFID.exInstrB;
                    rob_tail_MEM = pipeStateIFID.rob_fill_slot_exB;
                    diagram_slot_MEM = pipeStateIFID.diagram_slot_exB;
                    is_load = pipeStateIFID.ex_isloadB;
                    pc_MEM = pipeStateIFID.exPCB;
                    reg_MEM = pipeStateIFID.exregB;
                    valid_MEM = pipeStateIFID.ex_isvalB;
                    MEM_DONE = 1;
                }
                else if (isALU_B && ALU_DONE == 0)// otherwise send down pipe 1
                {
                    instrALU = pipeStateIFID.exInstrB;
                    rob_tail_ALU = pipeStateIFID.rob_fill_slot_exB;
                    diagram_slot_ALU = pipeStateIFID.diagram_slot_exB;
                    pc_ALU = pipeStateIFID.exPCB;
                    reg_ALU = pipeStateIFID.exregB;
                    valid_ALU = pipeStateIFID.ex_isvalB;
                    ALU_DONE = 1;
                }
            }
        }

        if(pipeStateIFID.stall_state_EX == 2 )
        {
            if (pipeStateIFID.ex_isMulDivA && MULDIV_DONE == 0)
            {
                instrMULDIV = pipeStateIFID.exInstrA;
                is_MulDiv = pipeStateIFID.ex_isMulDivA;
                rob_tail_MULDIV = pipeStateIFID.rob_fill_slot_exA;
                diagram_slot_MULDIV = pipeStateIFID.diagram_slot_exA;
                pc_MULDIV = pipeStateIFID.exPCA;
                reg_MULDIV = pipeStateIFID.exregA;
                valid_MULDIV = pipeStateIFID.ex_isvalA;
                MULDIV_DONE = 1;
            }
            else if ((pipeStateIFID.ex_isloadA || pipeStateIFID.ex_isstoreA) && MEM_DONE == 0) // if is_load or is_store, send down pipe2
            {
                instrMEM = pipeStateIFID.exInstrA;
                rob_tail_MEM = pipeStateIFID.rob_fill_slot_exA;
                diagram_slot_MEM = pipeStateIFID.diagram_slot_exA;
                is_load = pipeStateIFID.ex_isloadA;
                pc_MEM = pipeStateIFID.exPCA;
                reg_MEM = pipeStateIFID.exregA;
                valid_MEM = pipeStateIFID.ex_isvalA;
                MEM_DONE = 1;
            }
            else if(isALU_A && ALU_DONE == 0) // otherwise send down pipe 1
            {
                instrALU = pipeStateIFID.exInstrA;
                rob_tail_ALU = pipeStateIFID.rob_fill_slot_exA;
                diagram_slot_ALU = pipeStateIFID.diagram_slot_exA;
                pc_ALU = pipeStateIFID.exPCA;
                reg_ALU = pipeStateIFID.exregA;
                valid_ALU = pipeStateIFID.ex_isvalA;
                ALU_DONE = 1;
            }
        }
        // now update only B
        if(pipeStateIFID.stall_state_EX == 1 )
        {
            if (pipeStateIFID.ex_isMulDivB && MULDIV_DONE == 0)
            {
                instrMULDIV = pipeStateIFID.exInstrB;
                is_MulDiv = pipeStateIFID.ex_isMulDivB;
                rob_tail_MULDIV = pipeStateIFID.rob_fill_slot_exB;
                diagram_slot_MULDIV = pipeStateIFID.diagram_slot_exB;
                pc_MULDIV = pipeStateIFID.exPCB;
                reg_MULDIV = pipeStateIFID.exregB;
                valid_MULDIV = pipeStateIFID.ex_isvalB;
                MULDIV_DONE = 1;
            }
            else if ((pipeStateIFID.ex_isloadB || pipeStateIFID.ex_isstoreB) && MEM_DONE == 0) // if is_load or is_store, send down pipe2
            {
                instrMEM = pipeStateIFID.exInstrB;
                rob_tail_MEM = pipeStateIFID.rob_fill_slot_exB;
                diagram_slot_MEM = pipeStateIFID.diagram_slot_exB;
                is_load = pipeStateIFID.ex_isloadB;
                pc_MEM = pipeStateIFID.exPCB;
                reg_MEM = pipeStateIFID.exregB;
                valid_MEM = pipeStateIFID.ex_isvalB;
                MEM_DONE = 1;
            }
            else if (isALU_B && ALU_DONE == 0)// otherwise send down pipe 1
            {
                instrALU = pipeStateIFID.exInstrB;
                rob_tail_ALU = pipeStateIFID.rob_fill_slot_exB;
                diagram_slot_ALU = pipeStateIFID.diagram_slot_exB;
                pc_ALU = pipeStateIFID.exPCB;
                reg_ALU = pipeStateIFID.exregB;
                valid_ALU = pipeStateIFID.ex_isvalB;
                ALU_DONE = 1;
            }
        }
        // update all pipes for ex and later stages
        moveOneCycleMULDIV(pipeStateMULDIV, pipeState_NextMULDIV, instrMULDIV, executed, is_MulDiv, rob_tail_MULDIV, diagram_slot_MULDIV, pc_MULDIV, reg_MULDIV, valid_MULDIV);
        moveOneCycleALU(pipeStateALU, pipeState_NextALU, instrALU, executed, rob_tail_ALU, diagram_slot_ALU, pc_ALU, reg_ALU, valid_ALU);
        moveOneCycleMEM(pipeStateMEM, pipeState_NextMEM, instrMEM, executed, is_load, rob_tail_MEM, diagram_slot_MEM, pc_MEM, reg_MEM, valid_MEM);

    }
    //Dont make any updates if stall is 1. Just move post ex1 everything one cycle up
    else
    {
        moveStalledMULDIV(pipeStateMULDIV, pipeState_NextMULDIV);
        moveStalledALU(pipeStateALU, pipeState_NextALU);
        moveStalledMEM(pipeStateMEM, pipeState_NextMEM);
    }
}

void initPipeline(PipeState_Next &pipeState_Next)
{
    pipeState_Next.ex1Instr = 0x0;
    pipeState_Next.ex2Instr = 0x0;
    pipeState_Next.ex3Instr = 0x0;
    pipeState_Next.ex4Instr = 0x0;
    pipeState_Next.wbInstr = 0x0;

    pipeState_Next.ex1PC = 0x1;
    pipeState_Next.ex2PC = 0x1;
    pipeState_Next.ex3PC = 0x1;
    pipeState_Next.ex4PC = 0x1;
    pipeState_Next.wbPC = 0x1;
    
    pipeState_Next.ex1 = 1;
    pipeState_Next.ex2 = 1;
    pipeState_Next.ex3 = 1;
    pipeState_Next.ex4 = 1;
    pipeState_Next.wb = 1;

    pipeState_Next.wb_isval = false;
    pipeState_Next.ex4_isval = false;
    pipeState_Next.ex3_isval = false;
    pipeState_Next.ex2_isval = false;
    pipeState_Next.ex1_isval = false;

}

void initPipelineIFID(PipeStateIFID &pipeStateIFID)
{
    pipeStateIFID.cycle = 0;

    pipeStateIFID.ifInstrA = NOP;
    pipeStateIFID.ifInstrB = NOP;

    pipeStateIFID.idInstrA = NOP;
    pipeStateIFID.idInstrB = NOP;

    pipeStateIFID.exInstrA = NOP;
    pipeStateIFID.exInstrB = NOP;

    //Push PC
    pipeStateIFID.ifPCA = 0x1;
    pipeStateIFID.idPCA = 0x1;
    pipeStateIFID.exPCA = 0x1;

    pipeStateIFID.ifPCB = 0x1;
    pipeStateIFID.idPCB = 0x1;
    pipeStateIFID.exPCB = 0x1;

    //push execute
    pipeStateIFID.IFA = 1;
    pipeStateIFID.IDA = 1;
    pipeStateIFID.ex1A = 1;

    pipeStateIFID.IFB = 0;
    pipeStateIFID.IDB = 0;
    pipeStateIFID.ex1B = 0;

    // Instruction is valid - for ROB
    pipeStateIFID.if_isvalA  = false;
    pipeStateIFID.id_isvalA = false;
    pipeStateIFID.ex_isvalA = false;

    pipeStateIFID.if_isvalB = false;
    pipeStateIFID.id_isvalB = false;
    pipeStateIFID.ex_isvalB = false;

    //Hazard
    pipeStateIFID.is_hazard_IF = 0;
    pipeStateIFID.is_hazard_ID = 0;
    pipeStateIFID.is_hazard_EX = 0;

    //Stall state, keeps a note of whether to stall or and for how many cycles for a hazard
    pipeStateIFID.stall_state_EX = 0;
    pipeStateIFID.stall_state_ID = 0;
    pipeStateIFID.stall_state_IF = 0;

    pipeStateIFID.rob_fill_slot_ifA = 0;
    pipeStateIFID.rob_fill_slot_idA = 0;
    pipeStateIFID.rob_fill_slot_exA = 0;

    pipeStateIFID.rob_fill_slot_ifB = 0;
    pipeStateIFID.rob_fill_slot_idB = 0;
    pipeStateIFID.rob_fill_slot_exB = 0;

    pipeStateIFID.is_jumpA_IF = 0;
    pipeStateIFID.is_jumpA_ID = 0;
    pipeStateIFID.is_jumpA_EX = 0;

    pipeStateIFID.is_jumpB_IF = 0;
    pipeStateIFID.is_jumpB_ID = 0;
    pipeStateIFID.is_jumpB_EX = 0;

    pipeStateIFID.is_branchA_IF = 0;
    pipeStateIFID.is_branchA_ID = 0;
    pipeStateIFID.is_branchA_EX = 0;

    pipeStateIFID.is_branchB_IF = 0;
    pipeStateIFID.is_branchB_ID = 0;
    pipeStateIFID.is_branchB_EX = 0;

}

void initROB(ROBState &robState)
{
    robState.cycle = 0;
    robState.head = 0;
    robState.tail = 0;
    robState.commitedA = false;
    robState.commitedB = false;
    robState.commit_instrA = NOP;
    robState.commit_instrB = NOP;
    for (int i = 0; i < ROB_SIZE; i += 1) {
        robState.instr[i] = NOP;
        robState.valid[i] = false;
        robState.pending[i] = false;
        robState.preg[i] = 0;
    }

}


void initDiagram(DiagramState &dstate)
{
    dstate.cycle = 0;
    dstate.num_instrs = 0;
    dstate.is_full = false;
    for (int i = 0; i < DIAGRAM_SIZE; i += 1) {
        dstate.instr[i].instr = NOP;
        dstate.instr[i].done = true;
        dstate.instr[i].commit_cycle = 0;
        for (int j = 0; j < DIAGRAM_CYCLES; j += 1) {
            dstate.instr[i].stage[j] = "------";
        }
    }
}

void updatePipeDiagram(DiagramState &dstate, PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling, PipeStateIFID &pipeStateIFID)
{
    uint32_t cycle = dstate.cycle;
    if (stalling == 1 && (dstate.cycle < DIAGRAM_CYCLES-1)) {
        if (pipeStateMEM.ex2_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex2].stage[cycle] = "X2-MEM";
        }
        if (pipeStateMULDIV.ex2_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex2].stage[cycle] = "X2-MDV";
        }
        if (pipeStateMULDIV.ex3_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex3].stage[cycle] = "X3-MDV";
        }
        if (pipeStateMULDIV.ex4_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex4].stage[cycle] = "X4-MDV";
        }
        if (pipeStateMULDIV.wb_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_wb].stage[cycle] = "WB-MDV";
        }
        
    } else if ((stalling == 0) && (dstate.cycle < DIAGRAM_CYCLES-1) ) {
        // update diagram from ALU instrs
        if (pipeStateIFID.id_isvalA) {
            dstate.instr[pipeStateIFID.diagram_slot_idA].stage[cycle] = "ID\t";
        }
        if (pipeStateIFID.id_isvalB) {
            dstate.instr[pipeStateIFID.diagram_slot_idB].stage[cycle] = "ID\t";
        }
        if (pipeStateALU.ex1_isval) {
            dstate.instr[pipeStateALU.diagram_slot_ex1].stage[cycle] = "X1-ALU";
        }        
        if (pipeStateALU.wb_isval && (pipeStateALU.ex1Instr != pipeStateALU.wbInstr)) {
            dstate.instr[pipeStateALU.diagram_slot_wb].stage[cycle] = "WB-ALU";
        }

        // update diagram for MEM instrs
        if (pipeStateMEM.ex1_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex1].stage[cycle] = "X1-MEM";
        }        
        if (pipeStateMEM.ex2_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex2].stage[cycle] = "X2-MEM";
        }
        if (pipeStateMEM.wb_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_wb].stage[cycle] = "WB-MEM";
        }
        // update diagram for MULDIV instrs
                // update diagram for MEM instrs
        if (pipeStateMULDIV.ex1_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex1].stage[cycle] = "X1-MDV";
        }        
        if (pipeStateMULDIV.ex2_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex2].stage[cycle] = "X2-MDV";
        }
        if (pipeStateMULDIV.ex3_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex3].stage[cycle] = "X3-MDV";
        }
        if (pipeStateMULDIV.ex4_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_ex4].stage[cycle] = "X4-MDV";
        }                
        if (pipeStateMULDIV.wb_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_wb].stage[cycle] = "WB-MDV";
        }

    }
    dstate.cycle += 1;


}

//Byte's the smallest thing that can hold the opcode...
static uint8_t getOpcode(uint32_t instr)
{
    return (uint8_t)((instr >> 26) & 0x3f);
}

static const string regNames[NUM_REGS] = {
    "$zero",
    "$at",
    "$v0",
    "$v1",
    "$a0",
    "$a1",
    "$a2",
    "$a3",
    "$t0",
    "$t1",
    "$t2",
    "$t3",
    "$t4",
    "$t5",
    "$t6",
    "$t7",
    "$s0",
    "$s1",
    "$s2",
    "$s3",
    "$s4",
    "$s5",
    "$s6",
    "$s7",
    "$t8",
    "$t9",
    "$k0",
    "$k1",
    "$gp",
    "$sp",
    "$fp",
    "$ra",
};

static string getFunString(uint8_t funct)
{
    switch(funct)
    {
        case FUN_ADD:
            return "add";
        case FUN_ADDU:
            return "addu";
        case FUN_AND:
            return "and";
        case FUN_NOR:
            return "nor";
        case FUN_OR:
            return "or";
        case FUN_SLT:
            return "slt";
        case FUN_SLTU:
            return "sltu";
        case FUN_SLL:
            return "sll";
        case FUN_SRL:
            return "srl";
        case FUN_SLLV:
            return "sllv";
        case FUN_SRLV:
            return "srlv";
        case FUN_SRA:
            return "sra";
        case FUN_SUB:
            return "sub";
        case FUN_SUBU:
            return "subu";
        case FUN_MULT:
            return "mult";
        case FUN_MULTU:
            return "multu";
        case FUN_MFHI:
            return "mfhi";
        case FUN_MFLO:
            return "mflo";
        case FUN_DIV:
            return "div";
        case FUN_DIVU:
            return "divu";
        case FUN_JALR:
            return "jalr";
        case FUN_JR:
            return "jr";
        case FUN_MTHI:
            return "mthi";
        case FUN_MTLO:
            return "mtlo";
        case FUN_SRAV:
            return "srav";
        case FUN_XOR:
            return "xor";
        default:
            return "ILLEGAL";
    }
}

static string getImmString(uint8_t opcode)
{
    switch(opcode)
    {
        case OP_ADDI:
            return "addi";
        case OP_ADDIU:
            return "addiu";
        case OP_ANDI:
            return "andi";
        case OP_BEQ:
            return "beq";
        case OP_BNE:
            return "bne";
        case OP_LBU:
            return "lbu";
        case OP_LB:
            return "lb";
        case OP_LHU:
            return "lhu";
        case OP_LH:
            return "lh";
        case OP_LUI:
            return "lui";
        case OP_LW:
            return "lw";
        case OP_ORI:
            return "ori";
        case OP_SLTI:
            return "slti";
        case OP_SLTIU:
            return "sltiu";
        case OP_SB:
            return "sb";
        case OP_SC:
            return "sc";
        case OP_SH:
            return "sh";
        case OP_SW:
            return "sw";
        case OP_BLEZ:
            return "blez";
        case OP_BGTZ:
            return "bgtz";
        case OP_LWL:
            return "lwl";
        case OP_LWR:
            return "lwr";
        case OP_XORI:
            return "xori";
        default:
            //This should never happen.
            return "ILLEGAL";
    }
}

static void handleJInst(uint32_t instr, ostream & out_stream)
{
    uint8_t opcode = (instr >> 26) & 0x3f;
    uint32_t addr = instr & 0x3ffffff;

    ostringstream sb;

    switch(opcode)
    {
        case OP_JAL:
            sb << " jal " << hex << "0x" << addr << " ";
            break;
        case OP_J:
            sb << " j " << hex << "0x" << addr << " ";
            break;
    }

    out_stream << left << setw(25) << sb.str();
}

static void handleImmInst(uint32_t instr, ostream & out_stream)
{
    uint8_t opcode = (instr >> 26) & 0x3f;
    uint8_t rs = (instr >> 21) & 0x1f;
    uint8_t rt = (instr >> 16) & 0x1f;
    uint16_t imm = instr & 0xffff;
    //Sign extend the immediate...
    //uint32_t seImm = static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(imm)));
    //uint32_t zeImm = imm;

    //int ret = 0;
    //uint32_t value = 0;

    string opString = getImmString(opcode);

    ostringstream sb;

    switch(opcode)
    {
        case OP_ADDI:
        case OP_ADDIU:
        case OP_ANDI:
        case OP_ORI:
        case OP_SLTI:
        case OP_SLTIU:
        case OP_XORI:
            sb << " " << opString << " " << regNames[rt] << ", " << regNames[rs] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " ";
            break;
        case OP_BEQ:
        case OP_BNE:
        case OP_BLEZ:
        case OP_BGTZ:
            sb << " " << opString << " " << regNames[rs] << ", " << regNames[rt] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " ";
            break;
        case OP_LBU:
        case OP_LB:
        case OP_LHU:
        case OP_LW:
        case OP_LWL:
        case OP_LWR:
        case OP_SB:
        case OP_SC:
        case OP_SH:
        case OP_SW:
            sb << " " << opString << " " << regNames[rt] << ", " << static_cast<int16_t>(imm) << "(" << regNames[rs] << ") ";
            break;
        case OP_LUI:
            sb << " " << opString << " " << regNames[rt] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " ";
            break;
        case OP_BDECODER:
            switch(rt)
            {
                case 0x00000001:
                    sb << " " << "bgez" << " " << regNames[rt] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " ";
                    break;
                case 0x00000011:
                    sb << " " << "bgezal" << " " << regNames[rt] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " ";
                    break;
                case 0x00000000:
                    sb << " " << "bltz" << " " << regNames[rt] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " ";
                    break;
                case 0x00000010:
                    sb << " " << "bltzal" << " " << regNames[rt] << ", " << hex << "0x" << static_cast<uint32_t>(imm) << " "; 
                    break;
            }
            break;
        default:
            //This should never happen.
            sb << "ILLEGAL";
            break;
    }

    out_stream << left << setw(25) << sb.str();
}

static void handleOpZeroInst(uint32_t instr, ostream & out_stream)
{
    uint8_t rs = (instr >> 21) & 0x1f;
    uint8_t rt = (instr >> 16) & 0x1f;
    uint8_t rd = (instr >> 11) & 0x1f;
    uint8_t shamt = (instr >> 6) & 0x1f;
    uint8_t funct = instr & 0x3f;

    string funName = getFunString(funct);

    ostringstream sb;

    if(funct == FUN_JR)
    {
        sb << " " << funName << " " << regNames[rs] << " ";
    }
    else if(funct == FUN_SLL || funct == FUN_SRL)
    {
        if(instr == 0x0)
        {
            sb << " nop ";
        }
        else
        {
            sb << " " << funName << " " << regNames[rd] << ", " << regNames[rt] << ", " << static_cast<uint32_t>(shamt) << " ";
        }
    }
    else
    {
        sb << " " << funName << " " << regNames[rd] << ", " << regNames[rs] << ", " << regNames[rt] << " ";
    }

    out_stream << left << setw(25) << sb.str();
}

void printInstr(uint32_t curInst, ostream & pipeState)
{
    if(curInst == 0xfeedfeed)
    {
        pipeState << left << setw(25) << " HALT ";
        return;
    }
    else if(curInst == 0xdeefdeef)
    {
        pipeState << left << setw(25) << " UNKNOWN ";
        return;
    }

    switch(getOpcode(curInst))
    {
        //Everything with a zero opcode...
        case OP_ZERO:
            handleOpZeroInst(curInst, pipeState);
            break;
        case OP_ADDI:
        case OP_ADDIU:
        case OP_ANDI:
        case OP_BEQ:
        case OP_BNE:
        case OP_LBU:
        case OP_LB:
        case OP_LHU:
        case OP_LH:
        case OP_LUI:
        case OP_LW:
        case OP_ORI:
        case OP_SLTI:
        case OP_SLTIU:
        case OP_SB:
        case OP_SC:
        case OP_SH:
        case OP_SW:
        case OP_BLEZ:
        case OP_BGTZ:
        case OP_LWL:
        case OP_LWR:
        case OP_XORI:
        case OP_BDECODER:
            handleImmInst(curInst, pipeState);
            break;
        case OP_J:
        case OP_JAL:
            handleJInst(curInst, pipeState);
            break;
        default:
            //Illegal instruction. Trigger an exception.
            //Note: Since we catch illegal instructions here, the "handle"
            //instructions don't need to check for illegal instructions.
            //except for the case with a 0 opcode and illegal function.
            pipeState << left << setw(25) << " ILLEGAL ";
    }
}

void dumpPipeState(PipeState & stateALU, PipeState & stateMEM, PipeState & stateMULDIV, ROBState & robState, PipeStateIFID & pipeStateIFID, int stalling)
{

    ofstream pipe_out("pipe_state.out", ios::app);

    if(pipe_out)
    {

        pipe_out << "####################################################" << endl;
        pipe_out << "Cycle: " << pipeStateIFID.cycle << " IF : "<< pipeStateIFID.stall_state_IF <<" ID : " << pipeStateIFID.stall_state_ID <<" EX : " << pipeStateIFID.stall_state_EX <<endl;
        pipe_out << "####################################################" << endl;
        pipe_out << "|";
        pipe_out << "\t  IF \t \t   |\t  ID \t \t   | " << endl;

        pipe_out << "| ";
        printInstr(pipeStateIFID.ifInstrA, pipe_out);
        pipe_out << "|";
        printInstr(pipeStateIFID.idInstrA, pipe_out);
        pipe_out << "|" << endl;

        pipe_out << "| ";
        printInstr(pipeStateIFID.ifInstrB, pipe_out);
        pipe_out << "|";
        printInstr(pipeStateIFID.idInstrB, pipe_out);
        pipe_out << "|" << endl;

        pipe_out << "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
        pipe_out << "ALU Pipe: " << endl;
        pipe_out << "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
        pipe_out << "|";
        printInstr(stateALU.ex1Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateALU.wbInstr, pipe_out);
        pipe_out << "|" << endl;
        pipe_out << "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;   
        pipe_out << "MEM Pipe: " << endl; 
        pipe_out << "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
        pipe_out << "|";
        printInstr(stateMEM.ex1Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMEM.ex2Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMEM.ex3Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMEM.wbInstr, pipe_out);
        pipe_out << "|" << endl;
        pipe_out << "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
        pipe_out << "MUL/DIV Pipe: " << endl;
        pipe_out << "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
        pipe_out << "|";
        printInstr(stateMULDIV.ex1Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMULDIV.ex2Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMULDIV.ex3Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMULDIV.ex4Instr, pipe_out);
        pipe_out << "|";
        printInstr(stateMULDIV.wbInstr, pipe_out);
        pipe_out << "|" << endl;
        pipe_out << "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
           pipe_out << "|";
        pipe_out << " \t \t COMMITA\t \t  | " ;
        pipe_out << " \t \t COMMITB\t \t  | " << endl;
        pipe_out << "|";
        if (robState.commitedA) {
            printInstr(robState.commit_instrA, pipe_out);
        } else {
            pipe_out << "\t \t   nop\t\t\t";
        }
        pipe_out << "|";
        if (robState.commitedB) {
            printInstr(robState.commit_instrA, pipe_out);
        } else {
            pipe_out << "\t \t   nop\t\t\t";
        }
        pipe_out << "|" << endl;
    }
    else
    {
        cerr << "Could not open pipe state file!" << endl;
    }
}

void dumpROBState(ROBState & robState)
{

    ofstream rob_out("rob_state.out", ios::app);

    if(rob_out)
    {

        rob_out << "####################################################" << endl;
        rob_out << "Cycle: " << robState.cycle << "\t" << "Head: " << robState.head << "\t"  << "Tail: " << robState.tail << endl;
        rob_out << "####################################################" << endl;
        rob_out << "\t |" << "Val" << "|" << "Instruction \t \t \t \t" << "|" << "Pending: \t" << "|" << "Physical Reg: \t" << "|" << endl;
        rob_out << "---------------------------------------------------------------------------------------------------" << endl;

        for (uint32_t i = 0; i < ROB_SIZE; i += 1) {
            rob_out << i << "\t|" << robState.valid[i] << "\t|";
            printInstr(robState.instr[i], rob_out);
            rob_out << "\t|" << robState.pending[i] << "\t \t \t |" << robState.preg[i] << "\t \t \t \t |";
            if (i == robState.head) {
                rob_out << "\t \t \t \t <-------  HEAD";
            }
            if (i == robState.tail) {
                rob_out << "\t \t \t \t <-------  TAIL";
            }
            rob_out << "\t" << endl;
            rob_out << "---------------------------------------------------------------------------------------------------" << endl;

        }
    }
    else
    {
        cerr << "Could not open pipe state file!" << endl;
    }
}

void dumpPipeDiagram(DiagramState & dstate)
{

    ofstream diagram_out("pipe_diagram.out", ios::app);

    if(diagram_out)
    {
        diagram_out  << "\t|" << left << setw(27) << "Instruction " << "|"; 
        for (int j = 0; j < DIAGRAM_CYCLES; j += 1) {
            diagram_out  << j << "\t    |"; 
        }
        diagram_out << "|" << endl;
        diagram_out << "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;

        for (int i = 0; i < DIAGRAM_SIZE; i += 1) {
            diagram_out << i << "\t|";
            printInstr(dstate.instr[i].instr, diagram_out);
            for (int j = 0; j < DIAGRAM_CYCLES; j += 1) {
                diagram_out  << "\t|" << dstate.instr[i].stage[j]; 
            }
            diagram_out  << "\t|" << endl;
        }
        diagram_out << "---------------------------------------------------------------------------------------------------" << endl;

    }
    else
    {
        cerr << "Could not open pipe diagram file!" << endl;
    }
}