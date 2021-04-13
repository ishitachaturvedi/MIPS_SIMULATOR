###################################################
## MIPS Compilation path

MIPS_CC = mips-linux-gnu-gcc
MIPS_OBJCOPY = mips-linux-gnu-objcopy

# Turn on all warnings, and enable optimisations
MIPS_CPPFLAGS = -Wall -O0 -fno-builtin -march=mips1 -mfp32 -fno-stack-protector

# Avoid standard libraries etc. being brought in, and link statically
MIPS_LDFLAGS = -nostdlib -Wl,-melf32btsmip -march=mips1 -nostartfiles -mno-check-zero-division -Wl,--gpsize=0 -static -Wl,-Bstatic
MIPS_LDFLAGS += -Wl,--build-id=none


# Compile a c file into a s file (added by vgr16)
%.mips.s : %.c
	$(MIPS_CC) $(MIPS_CPPFLAGS) -S $< -o $@

# Compile a c file into a MIPS object file
%.mips.o : %.c
	$(MIPS_CC) $(MIPS_CPPFLAGS) -c $< -o $@
	
# Compile a s file into a MIPS object file
%.mips.o : %.s
	$(MIPS_CC) $(MIPS_CPPFLAGS) -c $< -o $@

# Link a MIPS object file and place it at the locations required in the
# spec using linker.ld
%.mips.elf : %.mips.o
	$(MIPS_CC) $(MIPS_CPPFLAGS) $(MIPS_LDFLAGS) -T src/linker.ld $< -o $@

# Extract just the binary instructions from the object file
%.mips.bin : %.mips.elf
	$(MIPS_OBJCOPY) -O binary --only-section=.text $< $@
	#$(MIPS_OBJCOPY) -O binary $< $@

# For example, if you have testbench/test.c, you can do:
#
# make testbench/test.mips.bin

###################################################
## BASE Simulator

IDIR = include
SIM_DEP = src/main.cpp src/setUp.cpp src/R_functions.cpp src/error.cpp src/J_functions.cpp src/I_functions.cpp src/Decode.cpp src/dumpPipeline.cpp
G++_FLAGS = -Wall -std=c++11 -O1 -I $(IDIR)

# Build the simulation binary
bin/mips_simulator : $(SIM_DEP)
	mkdir -p bin
	g++ $(G++_FLAGS) $(SIM_DEP) -o bin/mips_simulator


# In order to comply with spec
simulator : bin/mips_simulator

###################################################
## 5 stage bypass Simulator

IDIR = include_byp
BYP_SIM_DEP = src_byp/main.cpp src_byp/setUp.cpp src_byp/R_functions.cpp src_byp/error.cpp src_byp/J_functions.cpp src_byp/I_functions.cpp src_byp/Decode.cpp src_byp/dumpPipeline.cpp
G++_FLAGS = -Wall -std=c++11 -O1 -I $(IDIR)

# Build the simulation binary
bin/mips_simulator_byp : $(BYP_SIM_DEP)
	mkdir -p bin
	g++ $(G++_FLAGS) $(BYP_SIM_DEP) -o bin/mips_simulator_byp


# In order to comply with spec
simulator_byp : bin/mips_simulator_byp

###################################################
## 7 stage bypass Simulator with MulDiv unit

IDIR = include
LONG_SIM_DEP = src_long/main.cpp src_long/setUp.cpp src_long/R_functions.cpp src_long/error.cpp src_long/J_functions.cpp src_long/I_functions.cpp src_long/Decode.cpp src_long/dumpPipeline.cpp
G++_FLAGS = -Wall -std=c++11 -O1 -I $(IDIR)

# Build the simulation binary
bin/mips_simulator_long : $(LONG_SIM_DEP)
	mkdir -p bin
	g++ $(G++_FLAGS) $(LONG_SIM_DEP) -o bin/mips_simulator_long


# In order to comply with spec
simulator_long : bin/mips_simulator_long

###################################################
## Testbench

TIDIR = testbench/include
TEST_DEP = testbench/src/testbench.cpp testbench/src/test.cpp testbench/src/io.cpp
SG++_FLAGS = -Wall -std=c++11 -O1 -I $(TIDIR)

bin/testbench : $(TEST_DEP)
	mkdir -p bin
	g++ $(SG++_FLAGS) $(TEST_DEP) -o bin/testbench

testbench : bin/testbench

%.dump: %.mips.elf
	mips-linux-gnu-objdump -d $<
