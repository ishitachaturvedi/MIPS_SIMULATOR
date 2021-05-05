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
SIM_DEP = src/main.cpp src/setUp.cpp src/R_functions.cpp src/error.cpp src/J_functions.cpp src/I_functions.cpp
G++_FLAGS = -Wall -std=c++11 -O1 -I $(IDIR)

# Build the simulation binary
bin/mips_simulator : $(SIM_DEP)
	mkdir -p bin
	g++ $(G++_FLAGS) $(SIM_DEP) -o bin/mips_simulator


# In order to comply with spec
simulator : bin/mips_simulator

###################################################
## 5 stage bypass Simulator

BYP_IDIR = include_byp
BYP_SIM_DEP = src_byp/main.cpp src_byp/setUp.cpp src_byp/R_functions.cpp src_byp/error.cpp src_byp/J_functions.cpp src_byp/I_functions.cpp src_byp/Decode.cpp src_byp/dumpPipeline.cpp
BYP_G++_FLAGS = -Wall -std=c++11 -O1 -I include_byp

# Build the simulation binary
bin/mips_simulator_byp : $(BYP_SIM_DEP)
	mkdir -p bin
	g++ $(BYP_G++_FLAGS) $(BYP_SIM_DEP) -o bin/mips_simulator_byp

# In order to comply with spec
simulator_byp : bin/mips_simulator_byp

###################################################
## 7 stage bypass Simulator with MulDiv unit

LONG_IDIR = include_long
LONG_SIM_DEP = src_long/main.cpp src_long/setUp.cpp src_long/R_functions.cpp src_long/error.cpp src_long/J_functions.cpp src_long/I_functions.cpp src_long/Decode.cpp src_long/dumpPipeline.cpp
LONG_G++_FLAGS = -Wall -std=c++11 -O1 -I $(LONG_IDIR)

# Build the simulation binary
bin/mips_simulator_long : $(LONG_SIM_DEP)
	mkdir -p bin
	g++ $(LONG_G++_FLAGS) $(LONG_SIM_DEP) -o bin/mips_simulator_long


# In order to comply with spec
simulator_long : bin/mips_simulator_long


###################################################
## 3-pipe (ALU/MEM/MULDIV) bypass Simulator

3PIPE_IDIR = include_3pipe
3PIPE_SIM_DEP = src_3pipe/main.cpp src_3pipe/setUp.cpp src_3pipe/R_functions.cpp src_3pipe/error.cpp src_3pipe/J_functions.cpp src_3pipe/I_functions.cpp src_3pipe/Decode.cpp src_3pipe/dumpPipeline.cpp
3PIPE_G++_FLAGS = -Wall -std=c++11 -O1 -I $(3PIPE_IDIR)

# Build the simulation binary
bin/mips_simulator_3pipe : $(3PIPE_SIM_DEP)
	mkdir -p bin
	g++ $(3PIPE_G++_FLAGS) $(3PIPE_SIM_DEP) -o bin/mips_simulator_3pipe


# In order to comply with spec
simulator_3pipe : bin/mips_simulator_3pipe

###################################################
## ROB 3-pipe (ALU/MEM/MULDIV) bypass Simulator

ROB_IDIR = include_rob
ROB_SIM_DEP = src_rob/main.cpp src_rob/setUp.cpp src_rob/R_functions.cpp src_rob/error.cpp src_rob/J_functions.cpp src_rob/I_functions.cpp src_rob/Decode.cpp src_rob/dumpPipeline.cpp
ROB_G++_FLAGS = -Wall -std=c++11 -O1 -I $(ROB_IDIR)

# Build the simulation binary
bin/mips_simulator_rob : $(ROB_SIM_DEP)
	mkdir -p bin
	g++ $(ROB_G++_FLAGS) $(ROB_SIM_DEP) -o bin/mips_simulator_rob


# In order to comply with spec
simulator_rob : bin/mips_simulator_rob

###################################################
## Dual issue-single commit ROB 3-pipe (ALU/MEM/MULDIV) bypass Simulator

DUAL_IDIR = include_dual
DUAL_SIM_DEP = src_dual/main.cpp src_dual/setUp.cpp src_dual/R_functions.cpp src_dual/error.cpp src_dual/J_functions.cpp src_dual/I_functions.cpp src_dual/Decode.cpp src_dual/dumpPipeline.cpp
DUAL_G++_FLAGS = -Wall -std=c++11 -O1 -I $(DUAL_IDIR)

# Build the simulation binary
bin/mips_simulator_dual : $(DUAL_SIM_DEP)
	mkdir -p bin
	g++ $(DUAL_G++_FLAGS) $(DUAL_SIM_DEP) -o bin/mips_simulator_dual


# In order to comply with spec
simulator_dual : bin/mips_simulator_dual

###################################################
## Dual issue-single commit ROB 3-pipe (ALU/MEM/MULDIV) bypass Simulator

DIAGRAM_IDIR = include_diagram
DIAGRAM_SIM_DEP = src_diagram/main.cpp src_diagram/setUp.cpp src_diagram/R_functions.cpp src_diagram/error.cpp src_diagram/J_functions.cpp src_diagram/I_functions.cpp src_diagram/Decode.cpp src_diagram/dumpPipeline.cpp
DIAGRAM_G++_FLAGS = -Wall -std=c++11 -O1 -I $(DIAGRAM_IDIR)

# Build the simulation binary
bin/mips_simulator_diagram : $(DIAGRAM_SIM_DEP)
	mkdir -p bin
	g++ $(DIAGRAM_G++_FLAGS) $(DIAGRAM_SIM_DEP) -o bin/mips_simulator_diagram


# In order to comply with spec
simulator_diagram : bin/mips_simulator_diagram

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
