#ifndef DECODEHPP
#define DECODEHPP

#include "setUp.hpp"
#include "error.hpp"

struct Decode
{
    uint32_t opcode;
    uint32_t opcode_R;
    uint32_t rs;
    uint32_t rt;
    int32_t immediate;
    int32_t SignExtImm;;
    int32_t address;
    uint32_t funct_field;
    uint32_t shamt_field;
    uint32_t rd;
};

void decode_inst(uint32_t instr, Decode& decode);

#endif /* DECODE */