#include "Decode.hpp"
using namespace std;
#include <iostream>

void decode_inst(uint32_t instr, Decode& decode)
{
    decode.opcode = (instr & 0xFC000000) >> 26;
    decode.opcode_R = (instr & 0xFC000000) >> 26 & 0x0000003F;
    decode.rs = (instr & 0x03E00000) >> 21;
    decode.rt = (instr & 0x001F0000) >> 16;
    decode.rd = (instr & 0x0000F800) >> 11;
    decode.immediate = instr & 0x0000FFFF;
    decode.address = instr & 0x03FFFFFF;
    decode.funct_field = instr & 0x0000003F;
    decode.shamt_field = (instr & 0x000007C0) >> 6;

    if(decode.immediate >> 15)
    {
        decode.SignExtImm = decode.immediate | 0xFFFF0000;
    }
    else
    {
        decode.SignExtImm = decode.immediate;
    }
}