#include "R_functions.hpp"
#include <iostream>
using namespace std;

void r_type(State& mips_state, bool& executed, Decode& decode, bool &is_mulDiv, bool &is_jump, bool &is_branch, bool&is_R, bool &is_md_non_stall){

	is_R = false;
	is_jump = false;
	is_mulDiv = false;
	is_md_non_stall = false;	
	is_branch = false; 
	if(!executed && decode.opcode_R == 0x00000000){
		is_jump = false;
		is_mulDiv = false;
		is_md_non_stall = false;	
		is_branch = false; 
		is_R = true;
		switch(decode.funct_field){
			
			case 0x00000009:
					jalr(mips_state, decode.rs, decode.rt, decode.rd);
					is_jump = true;
					executed = true;
					return;			
			case 0x00000020:
					add(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;			
			case 0x00000021:
					addu(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x00000024:
					And(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x00000008:
					jr(mips_state,decode.rs);
					is_jump = true;
					executed = true;
					return;
			case 0x00000025:
					Or(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x0000002A:
					slt(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x0000002B:
					sltu(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x00000000:
					sll(mips_state, decode.rt, decode.shamt_field, decode.rd);
					executed = true;
					return;
			case 0x00000002:
					srl(mips_state, decode.rt, decode.shamt_field, decode.rd);
					executed = true;
					return;
			case 0x00000022:
					sub(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x00000023:
					subu(mips_state, decode.rs, decode.rt, decode.rd);
					executed = true;
					return;
			case 0x0000001A:
					div(mips_state, decode.rt, decode.rs);
					executed = true;
					is_md_non_stall = true;
					return;
			case 0x0000001B:
					divu(mips_state, decode.rt, decode.rs);
					executed = true;
					is_md_non_stall = true;
					return;
			case 0x00000010:
					mfhi(mips_state, decode.rd);
					executed = true;
					is_mulDiv = true;
					return;
			case 0x00000012:
					mflo(mips_state, decode.rd);
					executed = true;
					is_mulDiv = true;
					return;
			case 0x00000018:
					mult(mips_state, decode.rt, decode.rs);
					executed = true;
					is_md_non_stall = true;
					return;
			case 0x00000019:
					multu(mips_state, decode.rt, decode.rs);
					executed = true;
					is_md_non_stall = true;
					return;
			case 0x00000003:
					sra(mips_state, decode.rt, decode.shamt_field, decode.rd);
					executed = true;
					return;
			case 0x00000011:
					mthi(mips_state, decode.rs);
					executed = true;
					return;
			case 0x00000013:
					mtlo(mips_state, decode.rs);
					executed = true;
					return;
			case 0x00000004:
					sllv(mips_state,decode.rt,decode.rs,decode.rd);
					executed = true;
					return;
			case 0x00000007:
					srav(mips_state,decode.rt,decode.rs,decode.rd);
					executed = true;
					return;
			case 0x00000006:
					srlv(mips_state,decode.rt,decode.rs,decode.rd);
					executed = true;
					return;
			case 0x00000026:
					Xor(mips_state,decode.rt,decode.rs,decode.rd);
					executed = true;
					return;
			default:
					is_R = false;
					executed = false;
					return;
		}
	}
}

void Xor(State& mips_state,uint32_t rt,uint32_t rs,uint32_t rd){
	mips_state.reg[rd] = (mips_state.reg[rs] ^ mips_state.reg[rt]);
	++mips_state.npc;
}

void addu(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	rs = static_cast<uint32_t>(mips_state.reg[rs]);
	rt = static_cast<uint32_t>(mips_state.reg[rt]);
	mips_state.reg[rd] = static_cast<uint32_t>(rs + rt) ;

	++mips_state.npc;
}

void add(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	int32_t temp1 = mips_state.reg[rs];
	int32_t temp2 = mips_state.reg[rt];
	int32_t result = temp1 + temp2;
	 	if (((temp1 < 0 ) && (temp2 < 0) && (result >= 0)) || ((temp1 > 0) && (temp2 > 0) && (result <= 0))){
			throw (static_cast<int>(Exception::ARITHMETIC));
		}
		else {
			mips_state.reg[rd] = temp1 + temp2;
		}
	 	++mips_state.npc;
}


void And(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	mips_state.reg[rd] = mips_state.reg[rs] & mips_state.reg[rt];

	++mips_state.npc;
}

void jr(State& mips_state, uint32_t rs){
	if(mips_state.reg[rs] % 4 != 0){
		throw (static_cast<int>(Exception::MEMORY));
	}
	else{
		mips_state.npc = mips_state.reg[rs] / 4;
	}
}	

void Or(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	mips_state.reg[rd] = (mips_state.reg[rs] | mips_state.reg[rt]);

	++mips_state.npc;
}

void slt(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	int32_t temp1 = mips_state.reg[rs];
	int32_t temp2 = mips_state.reg[rt];
	if(temp1 < temp2){
		mips_state.reg[rd] = 1;
	}
	else{
		mips_state.reg[rd] = 0;
	}
	++mips_state.npc;
}

void sltu(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	rs = static_cast<uint32_t>(mips_state.reg[rs]);
	rt = static_cast<uint32_t>(mips_state.reg[rt]);
	if((0|rs) < (0|rt)) {
		mips_state.reg[rd] = ((0|rs) < (0|rt)) | 1;
	}
	else {
		mips_state.reg[rd] = 0;
	}

	++mips_state.npc;
}

void sll(State& mips_state, uint32_t rt, uint32_t shamt_field, uint32_t rd){
	mips_state.reg[rd] = (mips_state.reg[rt] << shamt_field);

	++mips_state.npc;
}

void srl(State& mips_state, uint32_t rt, uint32_t shamt_field, uint32_t rd){
	uint32_t result = static_cast<uint32_t>(mips_state.reg[rt]);
	mips_state.reg[rd] = static_cast<uint32_t>(result >> shamt_field);

	++mips_state.npc;
}

void sub(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	int32_t temp1 = mips_state.reg[rs];
	int32_t temp2 = mips_state.reg[rt];
	int32_t result = temp1 - temp2;
		if (((temp1 < 0 ) && (temp2 > 0) && (result >= 0)) || ((temp1 > 0) && (temp2 < 0) && (result <= 0))){
			throw (static_cast<int>(Exception::ARITHMETIC));
		}
		else {
			mips_state.reg[rd] =  temp1 - temp2;
		}

		++mips_state.npc;
}

void subu(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){
	mips_state.reg[rd] =  static_cast<uint32_t>(static_cast<uint32_t>(mips_state.reg[rs]) - static_cast<uint32_t>(mips_state.reg[rt]));

	++mips_state.npc;
}

void div(State& mips_state, uint32_t rt, uint32_t rs){
	int32_t dividend = mips_state.reg[rs];
	int32_t divisor = mips_state.reg[rt];

	//Dividing by zero is undefined
	if(divisor == 0){
		++mips_state.npc;
		return;
	}
	mips_state.Hi = dividend % divisor;
	mips_state.Lo = dividend / divisor;

	++mips_state.npc;
}

void divu(State& mips_state, uint32_t rt, uint32_t rs){
	uint32_t dividend = static_cast<uint32_t>(mips_state.reg[rs]);
	uint32_t divisor = static_cast<uint32_t>(mips_state.reg[rt]);

	//Dividing by zero is undefined
	if(divisor == 0){
		++mips_state.npc;
		return;
	}

	mips_state.Hi = static_cast<uint32_t>(dividend % divisor);
	mips_state.Lo = static_cast<uint32_t>(dividend / divisor);

	++mips_state.npc;
}

void mfhi(State& mips_state, uint32_t rd){
	mips_state.reg[rd] = mips_state.Hi;

	++mips_state.npc;
}

void mflo(State& mips_state, uint32_t rd){
	mips_state.reg[rd] = mips_state.Lo;

	++mips_state.npc;
}

void mult(State& mips_state, uint32_t rt, uint32_t rs){
	int64_t temp1 = mips_state.reg[rs];
	int64_t temp2 = mips_state.reg[rt];
	int64_t result  = temp1 * temp2;
	mips_state.Hi = (result >> 32);
	mips_state.Lo = (result << 32) >> 32;

	++mips_state.npc;
}

void multu(State& mips_state, uint32_t rt, uint32_t rs){
	uint64_t temp1 = (uint32_t)mips_state.reg[rs];
	uint64_t temp2 = (uint32_t)mips_state.reg[rt];
	uint64_t result = temp1 * temp2;
	mips_state.Hi = (result >> 32);
	mips_state.Lo = (result << 32) >> 32;

	++mips_state.npc;
}

void sra(State& mips_state, uint32_t rt, uint32_t shamt_field, uint32_t rd){
	int32_t temp = mips_state.reg[rt];
	mips_state.reg[rd] = (temp >> shamt_field);
	++mips_state.npc;
}

void mthi(State& mips_state,uint32_t rs){
	mips_state.Hi = mips_state.reg[rs];

	++mips_state.npc;
}

void mtlo(State& mips_state,uint32_t rs){
	mips_state.Lo = mips_state.reg[rs];

	++mips_state.npc;
}

void sllv(State& mips_state,uint32_t rt, uint32_t rs, uint32_t rd){
	mips_state.reg[rd] = (mips_state.reg[rt] << mips_state.reg[rs]);

	++mips_state.npc;
}

void srav(State& mips_state, uint32_t rt, uint32_t rs, uint32_t rd){
	mips_state.reg[rd] = ((int32_t)mips_state.reg[rt] >> mips_state.reg[rs]);

	++mips_state.npc;
}

void srlv(State& mips_state, uint32_t rt, uint32_t rs, uint32_t rd){
	uint32_t temp = mips_state.reg[rt];
	mips_state.reg[rd]  = (static_cast<uint32_t>(temp) >> mips_state.reg[rs]);

	++mips_state.npc;
}

void jalr(State& mips_state, uint32_t rs, uint32_t rt, uint32_t rd){

	mips_state.reg[rd] = (mips_state.pc * 4) + 8;
	mips_state.npc = mips_state.reg[rs] / 4;

	if(mips_state.reg[rs] % 4 != 0){
		//Have to Execute the delayed instruction and then thrown the Memory
		mips_state.npc = ADDR_DATA;
	}


}
