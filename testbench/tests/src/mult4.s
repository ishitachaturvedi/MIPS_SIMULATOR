.set noreorder
.set noat

li $3,0xFFFFFFFF
li $4,0x1
mult $3,$4
mfhi $2
jr $0
nop			#return 255
