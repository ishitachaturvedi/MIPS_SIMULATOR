#include "error.hpp"
#include <cstdlib> //for std::exit
#include <iostream>

using namespace std;

// Check for any Memory Exceptions during execution
void checkExec(const std::vector<int32_t>& reg, uint32_t addr){
	if(((addr < ADDR_INSTR) || (addr > (ADDR_INSTR + ADDR_INSTR_L - 1))) && (addr != ADDR_NULL)){
		throw (static_cast<int>(Exception::MEMORY));
	}
}	

// check if program has exited normally, without any exception
void checkExit(const std::vector<int32_t>& reg, uint32_t addr, int CurCycle)
{
	if(addr == ADDR_NULL){
		uint32_t out = reg[2] & 0x000000FF;
		cout << "NORMAL EXIT "<<CurCycle << " \n";
		std::exit(out);
	}
}

// Check for any Memory Exceptions during Read from memory
void checkRead(uint32_t addr){
	if(((addr < ADDR_INSTR) || (addr > (ADDR_INSTR + ADDR_INSTR_L - 1)))
		&& ((addr < ADDR_DATA) || (addr > (ADDR_DATA + ADDR_DATA_L - 1)))
		&& (addr != ADDR_GETC)){
			throw (static_cast<int>(Exception::MEMORY));
		}
   }	

// Check for any Memory Exceptions during Write to memory
void checkWrite(uint32_t addr){
	if(((addr < ADDR_DATA || (addr > (ADDR_DATA + ADDR_DATA_L - 1)))) && (addr != ADDR_PUTC)){
		throw (static_cast<int>(Exception::MEMORY));
	}
}

// Read from input or throw IO exception
char readChar(){
	char c;
	c = std::getchar();
	if(std::cin.eof()){
		return 0xFF;
	}
	if(!std::cin.good()){
		throw (static_cast<int>(Error::IO));
	}
	return c;
 }	

// Write to output or throw IO exception
void writeChar(char c){
	//std::putchar(c);
	if(!std::cout.good()){
		throw (static_cast<int>(Error::IO));
	}
  }	
