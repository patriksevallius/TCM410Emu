#ifndef _OPCODE_H_
#define _OPCODE_H_

#define INS_J     0x2
#define INS_JAL   0x3
#define INS_BEQ   0x4
#define INS_BNE   0x5
#define INS_BLEZ  0x6
#define INS_BGTZ  0x7
#define INS_ADDI  0x8
#define INS_ADDIU 0x9
#define INS_SLTI  0xa
#define INS_SLTIU 0xb
#define INS_ANDI  0xc
#define INS_ORI   0xd
#define INS_XORI  0xe
#define INS_LUI   0xf
#define INS_COP0  0x10
#define INS_COP1  0x11
#define INS_COP2  0x12
#define INS_NA1   0x13
#define INS_BEQL  0x14
#define INS_BNEL  0x15
#define INS_BLEZL 0x16
#define INS_BGTZL 0x17
#define INS_NA2   0x18
#define INS_NA3   0x19
#define INS_NA4   0x1a
#define INS_NA5   0x1b
#define INS_NA6   0x1c
#define INS_NA7   0x1d
#define INS_NA8   0x1e
#define INS_NA9   0x1f
#define INS_LB    0x20
#define INS_LH    0x21
#define INS_LWL   0x22
#define INS_LW    0x23
#define INS_LBU   0x24
#define INS_LHU   0x25
#define INS_LWR   0x26
#define INS_NA10  0x27
#define INS_SB    0x28
#define INS_SH    0x29
#define INS_SWL   0x2a
#define INS_SW    0x2b
#define INS_NA11  0x2c
#define INS_NA12  0x2d
#define INS_SWR   0x2e
#define INS_CACHE 0x2f
#define INS_LL    0x30

#define INS_SLL   0x40
#define INS_MOVF  0x41
#define INS_SRL   0x42
#define INS_SRA   0x43
#define INS_SLLV  0x44

#define INS_SRLV  0x46
#define INS_SRAV  0x47
#define INS_JR    0x48
#define INS_JALR  0x49

#define INS_MFHI  0x50
#define INS_MTHI  0x51
#define INS_MFLO  0x52
#define INS_MTLO  0x53

#define INS_MULT  0x58
#define INS_MULTU 0x59

#define INS_DIV   0x5a
#define INS_DIVU  0x5b

#define INS_ADD   0x60
#define INS_ADDU  0x61
#define INS_SUB   0x62
#define INS_SUBU  0x63
#define INS_AND   0x64
#define INS_OR    0x65
#define INS_XOR   0x66
#define INS_NOR   0x67

#define INS_SLT   0x6a
#define INS_SLTU  0x6b

#define INS_BLTZ  0x80
#define INS_BGEZ  0x81
#define INS_BLTZL 0x82
#define INS_BAL 0x91
#endif /* _OPCODE_H_ */

