#include "dumpPipeline.hpp"

static void handleOpZeroInst(uint32_t instr, std::ostream & out_stream);
static void printInstr(uint32_t curInst, std::ostream & pipeState);
static void handleImmInst(uint32_t instr, std::ostream & out_stream);
static string getImmString(uint8_t opcode);
static uint8_t getOpcode(uint32_t instr);
static void handleJInst(uint32_t instr, std::ostream & out_stream);

#define NUM_REGS 32

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


void checkForStall(PipeState &pipeState, bool is_load, int &stalling)
{
    Decode id;
    Decode ex;
    decode_inst(pipeState.idInstr, id);
    decode_inst(pipeState.exInstr, ex);

    if((id.rs == ex.rt || id.rd == ex.rt) && ex.rt != 0x0 && is_load && stalling == 0)
    {
        stalling = 1;
    }
}

//move pipeline one cycle forward
void moveOneCycle(State &mips_state, PipeState &pipeState, PipeState_Next &pipeState_Next, int executed, int CurCycle, int stalling, bool is_load)
{
    if(stalling != 2)
    {    
        pipeState.cycle = CurCycle;
        pipeState.ifInstr = mips_state.ram[mips_state.pc];
        pipeState.idInstr = pipeState_Next.idInstr;
        pipeState.exInstr = pipeState_Next.exInstr;
        pipeState.memInstr = pipeState_Next.memInstr;
        pipeState.wbInstr = pipeState_Next.wbInstr;

        pipeState_Next.wbInstr = pipeState_Next.memInstr;
        pipeState_Next.memInstr = pipeState_Next.exInstr;
        pipeState_Next.exInstr = pipeState_Next.idInstr;
        pipeState_Next.idInstr = pipeState.ifInstr;

        //PC setting
        pipeState.ifPC = mips_state.pc;
        pipeState.idPC = pipeState_Next.idPC;
        pipeState.exPC = pipeState_Next.exPC;
        pipeState.memPC = pipeState_Next.memPC;
        pipeState.wbPC = pipeState_Next.wbPC;
        
        pipeState_Next.wbPC = pipeState_Next.memPC;
        pipeState_Next.memPC = pipeState_Next.exPC;
        pipeState_Next.exPC = pipeState_Next.idPC;
        pipeState_Next.idPC = pipeState.ifPC;

        //reg setting
        pipeState.ifreg = mips_state.reg;
        pipeState.idreg = pipeState_Next.idreg;
        pipeState.exreg = pipeState_Next.exreg;
        pipeState.memreg = pipeState_Next.memreg;
        pipeState.wbreg = pipeState_Next.wbreg;
        
        pipeState_Next.wbreg = pipeState_Next.memreg;
        pipeState_Next.memreg = pipeState_Next.exreg;
        pipeState_Next.exreg = pipeState_Next.idreg;
        pipeState_Next.idreg = pipeState.ifreg;

        //execute setting
        pipeState.ex = executed;
        pipeState.mem = pipeState_Next.mem;
        pipeState.wb = pipeState_Next.wb;

        pipeState_Next.mem = pipeState.ex;
        pipeState_Next.wb = pipeState_Next.mem;

        //is_load
        pipeState.if_isload = is_load;
        pipeState.id_isload = pipeState_Next.id_isload;
        pipeState.ex_isload = pipeState_Next.ex_isload;

        pipeState_Next.ex_isload = pipeState_Next.id_isload;
        pipeState_Next.id_isload = pipeState.if_isload;
    }

    if(stalling == 2)
    {
        pipeState.cycle = CurCycle;
        pipeState.wbInstr = pipeState.memInstr;
        pipeState.memInstr = pipeState.exInstr;
        pipeState_Next.wbInstr =  pipeState.memInstr;
    }
}

void initPipeline(PipeState_Next &pipeState_Next)
{
    pipeState_Next.idInstr = 0x0;
    pipeState_Next.exInstr = 0x0;
    pipeState_Next.memInstr = 0x0;
    pipeState_Next.wbInstr = 0x0;
    pipeState_Next.idPC = 0x1;
    pipeState_Next.exPC = 0x1;
    pipeState_Next.memPC = 0x1;
    pipeState_Next.wbPC = 0x1;
    pipeState_Next.wb = 1;
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

void dumpPipeState(PipeState & state)
{

    ofstream pipe_out("pipe_state.out", ios::app);

    if(pipe_out)
    {
        pipe_out << "Cycle: " << state.cycle << endl;
        pipe_out << "-----------------------------------------------------------------------------------------------------------------------------------" << endl;
        pipe_out << "|";
        printInstr(state.ifInstr, pipe_out);
        pipe_out << "|";
        printInstr(state.idInstr, pipe_out);
        pipe_out << "|";
        printInstr(state.exInstr, pipe_out);
        pipe_out << "|";
        printInstr(state.memInstr, pipe_out);
        pipe_out << "|";
        printInstr(state.wbInstr, pipe_out);
        pipe_out << "|" << endl;
        pipe_out << "-----------------------------------------------------------------------------------------------------------------------------------" << endl;
    }
    else
    {
        cerr << "Could not open pipe state file!" << endl;
    }
}