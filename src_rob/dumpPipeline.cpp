#include "dumpPipeline.hpp"
#include <iostream>

static void handleOpZeroInst(uint32_t instr, std::ostream & out_stream);
static void printInstr(uint32_t curInst, std::ostream & pipeState);
static void handleImmInst(uint32_t instr, std::ostream & out_stream);
static string getImmString(uint8_t opcode);
static uint8_t getOpcode(uint32_t instr);
static void handleJInst(uint32_t instr, std::ostream & out_stream);

#define NUM_REGS 32

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

// check for data stalls where data cannot be bypased (load instruction in EX stage)
void checkForStall(PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling)
{
    // Get current instruction in decode
    uint32_t instr_D = NOP;

    if (pipeStateALU.idInstr != NOP) {
        instr_D = pipeStateALU.idInstr; 
    } else if (pipeStateMEM.idInstr != NOP) {
        instr_D = pipeStateMEM.idInstr; 
    } else if (pipeStateMULDIV.idInstr != NOP) {
        instr_D = pipeStateMULDIV.idInstr; 
    }


    // Decode current instruction
    Decode id;
    decode_inst(instr_D, id);

    // Decode Mem instructions;
    Decode ex1MEM;
    Decode ex2MEM;
    decode_inst(pipeStateMEM.ex1Instr, ex1MEM);
    decode_inst(pipeStateMEM.ex2Instr, ex2MEM);

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
    if (
        ((id.rs == ex1MEM.rt || id.rd == ex1MEM.rt) && ex1MEM.rt != 0x0 && pipeStateMEM.ex1_isload && !(pipeStateMEM.ex1Instr == pipeStateMEM.ex2Instr))
        || ((id.rs == ex2MEM.rt || id.rd == ex2MEM.rt) && ex2MEM.rt != 0x0 && pipeStateMEM.ex2_isload && !(pipeStateMEM.ex2Instr == pipeStateMEM.ex3Instr))
    )
    {
        stalling = 1;
        
    }

    // if ex1, ex2, ex3 are mulDiv inst which is being waited on, we stall
    if(
        (((id.rs == ex1MD.rd || id.rt == ex1MD.rd) && ex1MD.rd != 0x0 && pipeStateMULDIV.ex1_isMulDiv && !(pipeStateMULDIV.ex1Instr == pipeStateMULDIV.ex3Instr))
        || ((id.rs == ex2MD.rd || id.rt == ex2MD.rd) && ex2MD.rd != 0x0 && pipeStateMULDIV.ex2_isMulDiv && !(pipeStateMULDIV.ex2Instr == pipeStateMULDIV.ex3Instr))
        || ((id.rs == ex3MD.rd || id.rt == ex3MD.rd) && ex3MD.rd != 0x0 && pipeStateMULDIV.ex3_isMulDiv && !(pipeStateMULDIV.ex3Instr == pipeStateMULDIV.ex4Instr)))
    )
    {
        stalling = 1;
    }
}

//move pipeline one cycle forward, pass values down the pipeline
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, uint32_t instr, int stalling, bool is_load, bool is_store, bool is_mulDiv, uint32_t rob_tail, uint32_t diagram_slot)
{
    std::flush(std::cout);
    if(stalling != 1)
    {   
        // if ALU_PIPE then
        if (pipeState.pipe_type == ALU_PIPE) {

         // Instruction shifting
            pipeState.cycle = CurCycle;
            pipeState.ifInstr = instr;
            pipeState.idInstr = pipeState_Next.idInstr;
            pipeState.ex1Instr = pipeState_Next.ex1Instr;
            pipeState.wbInstr = pipeState_Next.wbInstr;

            pipeState_Next.wbInstr = pipeState_Next.ex1Instr;
            pipeState_Next.ex1Instr = pipeState_Next.idInstr;
            pipeState_Next.idInstr = pipeState.ifInstr;


            //PC setting
            pipeState.ifPC = mips_state.pc;
            pipeState.idPC = pipeState_Next.idPC;
            pipeState.ex1PC = pipeState_Next.ex1PC;
            pipeState.wbPC = pipeState_Next.wbPC;
            
            pipeState_Next.wbPC = pipeState_Next.ex1PC;
            pipeState_Next.ex1PC = pipeState_Next.idPC;
            pipeState_Next.idPC = pipeState.ifPC;

            //reg setting
            pipeState.ifreg = mips_state.reg;
            pipeState.idreg = pipeState_Next.idreg;
            pipeState.ex1reg = pipeState_Next.ex1reg;
            pipeState.wbreg = pipeState_Next.wbreg;
            
            pipeState_Next.wbreg = pipeState_Next.ex1reg;
            pipeState_Next.ex1reg = pipeState_Next.idreg;
            pipeState_Next.idreg = pipeState.ifreg;

            //execute setting

            pipeState.IF = executed;
            pipeState.id = pipeState_Next.id;
            pipeState.ex1 = pipeState_Next.ex1;
            pipeState.wb = pipeState_Next.wb;

            pipeState_Next.wb = pipeState_Next.ex1;
            pipeState_Next.ex1 = pipeState_Next.id;
            pipeState_Next.id = pipeState.IF;

            // ROB fill slots
            pipeState.rob_fill_slot_if = rob_tail;
            pipeState.rob_fill_slot_id = pipeState_Next.rob_fill_slot_id;
            pipeState.rob_fill_slot_ex1 = pipeState_Next.rob_fill_slot_ex1;
            pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

            pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex1;
            pipeState_Next.rob_fill_slot_ex1 = pipeState_Next.rob_fill_slot_id;
            pipeState_Next.rob_fill_slot_id = pipeState.rob_fill_slot_if;


            // iNSTRUCTION Valid
            pipeState.if_isval = (instr != NOP);
            pipeState.id_isval = pipeState_Next.id_isval;
            pipeState.ex1_isval = pipeState_Next.ex1_isval;
            pipeState.wb_isval = pipeState_Next.wb_isval;

            pipeState_Next.wb_isval = pipeState_Next.ex1_isval;
            pipeState_Next.ex1_isval = pipeState_Next.id_isval;
            pipeState_Next.id_isval = pipeState.if_isval;

            // Pipe Diagram fill slots
            pipeState.diagram_slot_if = diagram_slot;
            pipeState.diagram_slot_id = pipeState_Next.diagram_slot_id;
            pipeState.diagram_slot_ex1 = pipeState_Next.diagram_slot_ex1;
            pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

            pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex1;
            pipeState_Next.diagram_slot_ex1 = pipeState_Next.diagram_slot_id;
            pipeState_Next.diagram_slot_id = pipeState.diagram_slot_if;

    
        } else if (pipeState.pipe_type == MEM_PIPE) {

            // Instruction shifting
            pipeState.cycle = CurCycle;
            pipeState.ifInstr = instr;
            pipeState.idInstr = pipeState_Next.idInstr;
            pipeState.ex1Instr = pipeState_Next.ex1Instr;
            pipeState.ex2Instr = pipeState_Next.ex2Instr;
            pipeState.ex3Instr = pipeState_Next.ex3Instr;
            pipeState.wbInstr = pipeState_Next.wbInstr;

            pipeState_Next.wbInstr = pipeState_Next.ex3Instr;
            pipeState_Next.ex3Instr = pipeState_Next.ex2Instr;
            pipeState_Next.ex2Instr = pipeState_Next.ex1Instr;
            pipeState_Next.ex1Instr = pipeState_Next.idInstr;
            pipeState_Next.idInstr = pipeState.ifInstr;

            //PC setting
            pipeState.ifPC = mips_state.pc;
            pipeState.idPC = pipeState_Next.idPC;
            pipeState.ex1PC = pipeState_Next.ex1PC;
            pipeState.ex2PC = pipeState_Next.ex2PC;
            pipeState.ex3PC = pipeState_Next.ex3PC;
            pipeState.wbPC = pipeState_Next.wbPC;
            
            pipeState_Next.wbPC = pipeState_Next.ex3PC;
            pipeState_Next.ex3PC = pipeState_Next.ex2PC;
            pipeState_Next.ex2PC = pipeState_Next.ex1PC;
            pipeState_Next.ex1PC = pipeState_Next.idPC;
            pipeState_Next.idPC = pipeState.ifPC;

            //reg setting
            pipeState.ifreg = mips_state.reg;
            pipeState.idreg = pipeState_Next.idreg;
            pipeState.ex1reg = pipeState_Next.ex1reg;
            pipeState.ex2reg = pipeState_Next.ex2reg;
            pipeState.ex3reg = pipeState_Next.ex3reg;
            pipeState.wbreg = pipeState_Next.wbreg;
            
            pipeState_Next.wbreg = pipeState_Next.ex3reg;
            pipeState_Next.ex3reg = pipeState_Next.ex2reg;
            pipeState_Next.ex2reg = pipeState_Next.ex1reg;
            pipeState_Next.ex1reg = pipeState_Next.idreg;
            pipeState_Next.idreg = pipeState.ifreg;

            //execute setting
            pipeState.IF = executed;
            pipeState.id = pipeState_Next.id;
            pipeState.ex1 = pipeState_Next.ex1;
            pipeState.ex2 = pipeState_Next.ex2;
            pipeState.ex3 = pipeState_Next.ex3;
            pipeState.wb = pipeState_Next.wb;

            pipeState_Next.wb = pipeState_Next.ex3;
            pipeState_Next.ex3 = pipeState_Next.ex2;
            pipeState_Next.ex2 = pipeState_Next.ex1;
            pipeState_Next.ex1 = pipeState_Next.id;
            pipeState_Next.id = pipeState.IF;

            //is_load
            pipeState.if_isload = is_load;
            pipeState.id_isload = pipeState_Next.id_isload;
            pipeState.ex1_isload = pipeState_Next.ex1_isload;
            pipeState.ex2_isload = pipeState_Next.ex2_isload;
            pipeState.ex3_isload = pipeState_Next.ex3_isload;
            
            pipeState_Next.ex3_isload = pipeState_Next.ex2_isload;
            pipeState_Next.ex2_isload = pipeState_Next.ex1_isload;
            pipeState_Next.ex1_isload = pipeState_Next.id_isload;
            pipeState_Next.id_isload = pipeState.if_isload;

            // ROB fill slots
            pipeState.rob_fill_slot_if = rob_tail;
            pipeState.rob_fill_slot_id = pipeState_Next.rob_fill_slot_id;
            pipeState.rob_fill_slot_ex1 = pipeState_Next.rob_fill_slot_ex1;
            pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
            pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
            pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

            pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex3;
            pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
            pipeState_Next.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex1;
            pipeState_Next.rob_fill_slot_ex1 = pipeState_Next.rob_fill_slot_id;
            pipeState_Next.rob_fill_slot_id = pipeState.rob_fill_slot_if;

            // iNSTRUCTION Valid
            pipeState.if_isval = (instr != NOP);
            pipeState.id_isval = pipeState_Next.id_isval;
            pipeState.ex1_isval = pipeState_Next.ex1_isval;
            pipeState.ex2_isval = pipeState_Next.ex2_isval;
            pipeState.ex3_isval = pipeState_Next.ex3_isval;
            pipeState.wb_isval = pipeState_Next.wb_isval;

            pipeState_Next.wb_isval = pipeState_Next.ex3_isval;
            pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
            pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;
            pipeState_Next.ex1_isval = pipeState_Next.id_isval;
            pipeState_Next.id_isval = pipeState.if_isval;

            // Pipe Diagram fill slots
            pipeState.diagram_slot_if = diagram_slot;
            pipeState.diagram_slot_id = pipeState_Next.diagram_slot_id;
            pipeState.diagram_slot_ex1 = pipeState_Next.diagram_slot_ex1;
            pipeState.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex2;
            pipeState.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex3;
            pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

            pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex3;
            pipeState_Next.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex2;
            pipeState_Next.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex1;
            pipeState_Next.diagram_slot_ex1 = pipeState_Next.diagram_slot_id;
            pipeState_Next.diagram_slot_id = pipeState.diagram_slot_if;



        } else if (pipeState.pipe_type == MULDIV_PIPE) {

        
            // Instruction shifting
            pipeState.cycle = CurCycle;
            pipeState.ifInstr = instr;
            pipeState.idInstr = pipeState_Next.idInstr;
            pipeState.ex1Instr = pipeState_Next.ex1Instr;
            pipeState.ex2Instr = pipeState_Next.ex2Instr;
            pipeState.ex3Instr = pipeState_Next.ex3Instr;
            pipeState.ex4Instr = pipeState_Next.ex4Instr;
            pipeState.wbInstr = pipeState_Next.wbInstr;

            pipeState_Next.wbInstr = pipeState_Next.ex4Instr;
            pipeState_Next.ex4Instr = pipeState_Next.ex3Instr;
            pipeState_Next.ex3Instr = pipeState_Next.ex2Instr;
            pipeState_Next.ex2Instr = pipeState_Next.ex1Instr;
            pipeState_Next.ex1Instr = pipeState_Next.idInstr;
            pipeState_Next.idInstr = pipeState.ifInstr;

            //PC setting
            pipeState.ifPC = mips_state.pc;
            pipeState.idPC = pipeState_Next.idPC;
            pipeState.ex1PC = pipeState_Next.ex1PC;
            pipeState.ex2PC = pipeState_Next.ex2PC;
            pipeState.ex3PC = pipeState_Next.ex3PC;
            pipeState.ex4PC = pipeState_Next.ex4PC;
            pipeState.wbPC = pipeState_Next.wbPC;
            
            pipeState_Next.wbPC = pipeState_Next.ex4PC;
            pipeState_Next.ex4PC = pipeState_Next.ex3PC;
            pipeState_Next.ex3PC = pipeState_Next.ex2PC;
            pipeState_Next.ex2PC = pipeState_Next.ex1PC;
            pipeState_Next.ex1PC = pipeState_Next.idPC;
            pipeState_Next.idPC = pipeState.ifPC;

            //reg setting
            pipeState.ifreg = mips_state.reg;
            pipeState.idreg = pipeState_Next.idreg;
            pipeState.ex1reg = pipeState_Next.ex1reg;
            pipeState.ex2reg = pipeState_Next.ex2reg;
            pipeState.ex3reg = pipeState_Next.ex3reg;
            pipeState.ex4reg = pipeState_Next.ex4reg;
            pipeState.wbreg = pipeState_Next.wbreg;
            
            pipeState_Next.wbreg = pipeState_Next.ex4reg;
            pipeState_Next.ex4reg = pipeState_Next.ex3reg;
            pipeState_Next.ex3reg = pipeState_Next.ex2reg;
            pipeState_Next.ex2reg = pipeState_Next.ex1reg;
            pipeState_Next.ex1reg = pipeState_Next.idreg;
            pipeState_Next.idreg = pipeState.ifreg;

            //execute setting

            pipeState.IF = executed;
            pipeState.id = pipeState_Next.id;
            pipeState.ex1 = pipeState_Next.ex1;
            pipeState.ex2 = pipeState_Next.ex2;
            pipeState.ex3 = pipeState_Next.ex3;
            pipeState.ex4 = pipeState_Next.ex4;
            pipeState.wb = pipeState_Next.wb;

            pipeState_Next.wb = pipeState_Next.ex4;
            pipeState_Next.ex4 = pipeState_Next.ex3;
            pipeState_Next.ex3 = pipeState_Next.ex2;
            pipeState_Next.ex2 = pipeState_Next.ex1;
            pipeState_Next.ex1 = pipeState_Next.id;
            pipeState_Next.id = pipeState.IF;

            //is_mulDiv
            pipeState.if_isMulDiv = is_mulDiv;
            pipeState.id_isMulDiv = pipeState_Next.id_isMulDiv;
            pipeState.ex1_isMulDiv = pipeState_Next.ex1_isMulDiv;
            pipeState.ex2_isMulDiv = pipeState_Next.ex2_isMulDiv;
            pipeState.ex3_isMulDiv = pipeState_Next.ex3_isMulDiv;

            pipeState_Next.ex3_isMulDiv = pipeState_Next.ex2_isMulDiv;
            pipeState_Next.ex2_isMulDiv = pipeState_Next.ex1_isMulDiv;
            pipeState_Next.ex1_isMulDiv = pipeState_Next.id_isMulDiv;
            pipeState_Next.id_isMulDiv = pipeState.if_isMulDiv;

            // ROB fill slots
            pipeState.rob_fill_slot_if = rob_tail;
            pipeState.rob_fill_slot_id = pipeState_Next.rob_fill_slot_id;
            pipeState.rob_fill_slot_ex1 = pipeState_Next.rob_fill_slot_ex1;
            pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
            pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
            pipeState.rob_fill_slot_ex4 = pipeState_Next.rob_fill_slot_ex4;
            pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

            pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex4;
            pipeState_Next.rob_fill_slot_ex4 = pipeState_Next.rob_fill_slot_ex3;
            pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
            pipeState_Next.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex1;
            pipeState_Next.rob_fill_slot_ex1 = pipeState_Next.rob_fill_slot_id;
            pipeState_Next.rob_fill_slot_id = pipeState.rob_fill_slot_if;

            // iNSTRUCTION Valid
            pipeState.if_isval = (instr != NOP);
            pipeState.id_isval = pipeState_Next.id_isval;
            pipeState.ex1_isval = pipeState_Next.ex1_isval;
            pipeState.ex2_isval = pipeState_Next.ex2_isval;
            pipeState.ex3_isval = pipeState_Next.ex3_isval;
            pipeState.ex4_isval = pipeState_Next.ex4_isval;
            pipeState.wb_isval = pipeState_Next.wb_isval;

            pipeState_Next.wb_isval = pipeState_Next.ex4_isval;
            pipeState_Next.ex4_isval = pipeState_Next.ex3_isval;
            pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
            pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;
            pipeState_Next.ex1_isval = pipeState_Next.id_isval;
            pipeState_Next.id_isval = pipeState.if_isval;


            // Pipe Diagram fill slots
            pipeState.diagram_slot_if = diagram_slot;
            pipeState.diagram_slot_id = pipeState_Next.diagram_slot_id;
            pipeState.diagram_slot_ex1 = pipeState_Next.diagram_slot_ex1;
            pipeState.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex2;
            pipeState.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex3;
            pipeState.diagram_slot_ex4 = pipeState_Next.diagram_slot_ex4;
            pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

            pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex4;
            pipeState_Next.diagram_slot_ex4 = pipeState_Next.diagram_slot_ex3;
            pipeState_Next.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex2;
            pipeState_Next.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex1;
            pipeState_Next.diagram_slot_ex1 = pipeState_Next.diagram_slot_id;
            pipeState_Next.diagram_slot_id = pipeState.diagram_slot_if;

        } else {
            cerr << "Invalid Pipe Type. Choose between 1: ALU, 2: MEM, 3: MULDIV." << endl;
        }

    }

    if(stalling == 1)
    {
        if (pipeState.pipe_type == ALU_PIPE) {

            pipeState.cycle = CurCycle;
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

            // Pipe Diagram fill slots
            //pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

            //pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex1;

        } else if (pipeState.pipe_type == MEM_PIPE) {

            pipeState.cycle = CurCycle;
            pipeState.wbInstr = pipeState.ex3Instr;
            pipeState.ex3Instr = pipeState.ex2Instr;
            pipeState.ex2Instr = pipeState.ex1Instr;

            pipeState_Next.wbInstr =  pipeState.ex3Instr;
            pipeState_Next.ex3Instr = pipeState.ex2Instr;

            //is_load
            pipeState.ex2_isload = pipeState_Next.ex2_isload;
            pipeState.ex3_isload = pipeState_Next.ex3_isload;
            
            pipeState_Next.ex3_isload = pipeState_Next.ex2_isload;
            pipeState_Next.ex2_isload = pipeState_Next.ex1_isload;

            // ROB fill slots
            pipeState.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex2;
            pipeState.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex3;
            pipeState.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_wb;

            pipeState_Next.rob_fill_slot_wb = pipeState_Next.rob_fill_slot_ex3;
            pipeState_Next.rob_fill_slot_ex3 = pipeState_Next.rob_fill_slot_ex2;
            pipeState_Next.rob_fill_slot_ex2 = pipeState_Next.rob_fill_slot_ex1;


            // iNSTRUCTION Valid
            pipeState.ex2_isval = pipeState_Next.ex2_isval;
            pipeState.ex3_isval = pipeState_Next.ex3_isval;
            pipeState.wb_isval = pipeState_Next.wb_isval;

            pipeState_Next.wb_isval = pipeState_Next.ex3_isval;
            pipeState_Next.ex3_isval = pipeState_Next.ex2_isval;
            pipeState_Next.ex2_isval = pipeState_Next.ex1_isval;

            // Pipe Diagram fill slots
            pipeState.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex2;
            pipeState.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex3;
            pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

            pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex3;
            pipeState_Next.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex2;
            pipeState_Next.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex1;

        } else if (pipeState.pipe_type == MULDIV_PIPE) {

            pipeState.cycle = CurCycle;
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

            // Pipe Diagram fill slots
            pipeState.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex2;
            pipeState.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex3;
            pipeState.diagram_slot_ex4 = pipeState_Next.diagram_slot_ex4;
            pipeState.diagram_slot_wb = pipeState_Next.diagram_slot_wb;

            pipeState_Next.diagram_slot_wb = pipeState_Next.diagram_slot_ex4;
            pipeState_Next.diagram_slot_ex4 = pipeState_Next.diagram_slot_ex3;
            pipeState_Next.diagram_slot_ex3 = pipeState_Next.diagram_slot_ex2;
            pipeState_Next.diagram_slot_ex2 = pipeState_Next.diagram_slot_ex1;
            
        } else {
            cerr << "Invalid Pipe Type. Choose between 1: ALU, 2: MEM, 3: MULDIV." << endl;
        }

    }
}

// initialise pipeline stages
void initPipeline(PipeState_Next &pipeState_Next)
{
    pipeState_Next.idInstr = 0x0;
    pipeState_Next.ex1Instr = 0x0;
    pipeState_Next.ex2Instr = 0x0;
    pipeState_Next.ex3Instr = 0x0;
    pipeState_Next.ex4Instr = 0x0;
    pipeState_Next.wbInstr = 0x0;

    pipeState_Next.idPC = 0x1;
    pipeState_Next.ex1PC = 0x1;
    pipeState_Next.ex2PC = 0x1;
    pipeState_Next.ex3PC = 0x1;
    pipeState_Next.ex4PC = 0x1;
    pipeState_Next.wbPC = 0x1;
    
    pipeState_Next.id = 1;
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
    pipeState_Next.id_isval = false;

}

// initialise the ROB unit
void initROB(ROBState &robState)
{
    robState.cycle = 0;
    robState.head = 0;
    robState.tail = 0;
    robState.commited = false;
    robState.commit_instr = NOP;
    for (int i = 0; i < ROB_SIZE; i += 1) {
        robState.instr[i] = NOP;
        robState.valid[i] = false;
        robState.pending[i] = false;
        robState.preg[i] = 0;
    }
}

// initialise pipeline diagram
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

// update pipeline diagram for each instruction in the pipeline per cycle
void updatePipeDiagram(DiagramState &dstate, PipeState &pipeStateALU, PipeState &pipeStateMEM, PipeState &pipeStateMULDIV, int &stalling)
{
    uint32_t cycle = dstate.cycle;
    if (stalling == 1 && (dstate.cycle < DIAGRAM_CYCLES-1)) {
        if (pipeStateMEM.ex2_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex2].stage[cycle] = "X2-MEM";
        }
        if (pipeStateMEM.ex3_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex3].stage[cycle] = "X3-MEM";
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
        if (pipeStateALU.id_isval) {
            dstate.instr[pipeStateALU.diagram_slot_id].stage[cycle] = "ID\t";
        }
        if (pipeStateALU.ex1_isval) {
            dstate.instr[pipeStateALU.diagram_slot_ex1].stage[cycle] = "X1-ALU";
        }        
        if (pipeStateALU.wb_isval && (pipeStateALU.ex1Instr != pipeStateALU.wbInstr)) {
            dstate.instr[pipeStateALU.diagram_slot_wb].stage[cycle] = "WB-ALU";
        }

        // update diagram for MEM instrs
        if (pipeStateMEM.id_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_id].stage[cycle] = "ID\t";
        }
        if (pipeStateMEM.ex1_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex1].stage[cycle] = "X1-MEM";
        }        
        if (pipeStateMEM.ex2_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex2].stage[cycle] = "X2-MEM";
        }
        if (pipeStateMEM.ex3_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_ex3].stage[cycle] = "X3-MEM";
        }
        if (pipeStateMEM.wb_isval) {
            dstate.instr[pipeStateMEM.diagram_slot_wb].stage[cycle] = "WB-MEM";
        }
        // update diagram for MULDIV instrs
                // update diagram for MEM instrs
        if (pipeStateMULDIV.id_isval) {
            dstate.instr[pipeStateMULDIV.diagram_slot_id].stage[cycle] = "ID\t";
        }
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

// get function name to add to pipeline diagram and pipeline dump
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

// get imm instruction name for pipeline dump and pipeline diagram
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

// get jump instruction name for pipeline dump and pipeline diagram
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

/// output imm instruction name for pipeline dump and pipeline diagram
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

// output reg instruction name for pipeline dump and pipeline diagram
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

// function to print reg, imm, jump instructions for pipeline dump
static void printInstr(uint32_t curInst, ostream & pipeState)
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

// function to output pipeline dump per cycle 
void dumpPipeState(PipeState & stateALU, PipeState & stateMEM, PipeState & stateMULDIV, ROBState & robState)
{

    ofstream pipe_out("pipe_state.out", ios::app);

    if(pipe_out)
    {

        pipe_out << "####################################################" << endl;
        pipe_out << "Cycle: " << stateALU.cycle << endl;
        pipe_out << "####################################################" << endl;
        pipe_out << "|";
        pipe_out << "\t \t  IF \t \t \t  |  \t \t ID  \t \t \t | " << endl;
        pipe_out << "|";
        if (stateALU.if_isval) {
            printInstr(stateALU.ifInstr, pipe_out);
        } else if (stateMEM.if_isval) {
            printInstr(stateMEM.ifInstr, pipe_out);
        } else if (stateMULDIV.if_isval) {
            printInstr(stateMULDIV.ifInstr, pipe_out);
        } else {
            pipe_out << "\t \t   nop\t\t\t ";
        }
        pipe_out << "|";
        if (stateALU.id_isval) {
            printInstr(stateALU.idInstr, pipe_out);
        } else if (stateMEM.id_isval) {
            printInstr(stateMEM.idInstr, pipe_out);
        } else if (stateMULDIV.id_isval) {
            printInstr(stateMULDIV.idInstr, pipe_out);
        }
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
        pipe_out << " \t \t COMMIT \t \t  | " << endl;
        pipe_out << "|";
        if (robState.commited) {
            printInstr(robState.commit_instr, pipe_out);
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

// Dump ROB state per cycle 
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

// function to print pipeline diagram per cycle
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