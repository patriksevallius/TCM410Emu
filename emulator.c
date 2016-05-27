#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "opcode.h"

#define FAKEFLASH_START 0x9f000000
#define FAKEFLASH_END   0x9f200000

#define FLASH_START 0x9fc00000
#define FLASH_END   0x9fe00000
#define FLASH_SIZE  0x00200000

#define RAM_START   0x80000000
#define RAM_END     0x82000000
#define RAM_SIZE    0x02000000

#define REG_START   0xfffe0000
#define REG_END     0xffffffff

#define dtrace(...) do { if( debug == 1 ) fprintf(stderr, __VA_ARGS__); } while(0)

struct cpu_state
{
	int reg[32];
	int HI;
	int LO;
	int pc;
	int delayed_jump;
	int jump_pc;
};

static int debug = 0;
static int timer_int = 0;
static int fakeflash_state = 0;

char fakeflash_read(unsigned int vaddr)
{
  char rv = '\0';

  if(fakeflash_state == 1)
    switch(vaddr)
      {
      case 0x9f000001:
	rv = 'P';
	break;
      case 0x9f000003:
	rv = 'R';
	break;
      case 0x9f000005:
	rv = 'I';
	break;
      case 0x9f000021:
	rv = 'Q';
	break;
      case 0x9f000023:
	rv = 'R';
	break;
      case 0x9f000025:
	rv = 'Y';
	break;
    default:
	rv = '\0';
	break;
      }

/*   printf("fake flash read @ 0x%x (0x%x)\n", vaddr, rv); */
  return rv;
}

void fakeflash_write(unsigned int vaddr, short val)
{

  if(vaddr == 0x9f0000aa && val == 0x98)
    fakeflash_state = 1;
  if(vaddr == 0x9f0000aa && val == 0xff)
    fakeflash_state = 0;
/*   printf("fake flash write 0x%x @ 0x%x\n", val, vaddr); */
}

int get_reg_val(unsigned int vaddr)
{
	if(vaddr == 0xfffe0000)
		return 0x33483348;
	else if(vaddr == 0xfffe0003)
		return 0xa0;
	else if(vaddr == 0xFFFE2000)
		return 0x1F00000B;
	else if(vaddr == 0xFFFE2008)
		return 0x1A000008;
	else if(vaddr == 0xFFFE2010)
		return 0x00000000;
	else if(vaddr == 0xFFFE2018)
		return 0x00000000;
	else if(vaddr == 0xFFFE2020)
		return 0x00000000;
	else if(vaddr == 0xFFFE2028)
		return 0x00000000;
	else if(vaddr == 0xFFFE2004)
		return 0x00000019;
	else if(vaddr == 0xFFFE200C)
		return 0x00000019;
	else if(vaddr == 0xFFFE2014)
		return 0x00000000;
	else if(vaddr == 0xFFFE201C)
		return 0x00000000;
	else if(vaddr == 0xFFFE2024)
		return 0x00000000;
	else if(vaddr == 0xFFFE202C)
		return 0x00000000;
	else if(vaddr == 0xFFFE0203)
	  {
	    if(timer_int == 1) {
	      timer_int = 0;
	      return 0xff;
	    }
	    else  
	      return 0x0;
	  }
	else if(vaddr == 0xFFFE0312)
	  return 0xffffffff;
	else
	printf("Registry access b(0x%x)\n", vaddr);

	return 0;
}

int get_instruction(char *flash, char *ram, unsigned int address)
{
	int instruction = 0x0;
	if(address >= FLASH_START && address < FLASH_END)
		instruction = *(int *)(flash+address-FLASH_START);
	else if(address >= RAM_START && address < RAM_END)
		instruction = *(int *)(ram+address-RAM_START);
	instruction = ntohl(instruction);
	return instruction;
}

unsigned int decode_opcode(unsigned int instruction)
{
	return instruction >> 26;
}

unsigned int decode_special_opcode(unsigned int instruction)
{
	return (instruction & 0x3f) | 0x40;
}

unsigned int decode_special_branch_opcode(unsigned int instruction)
{
	return (instruction & 0x1f0000) >>16 | 0x80;
}

int get_jump_address(int instruction, int pc)
{
	instruction = (pc & 0xf0000000) | ((instruction & 0x03ffffff) << 2);
	return instruction;
}

int get_rs(int instruction)
{
	return (instruction & 0x03e00000) >> 21;
}

int get_rt(int instruction)
{
	return (instruction & 0x001f0000) >> 16;
}

int get_rd(int instruction)
{
	return (instruction & 0x0000f800) >> 11;
}

int get_sa(int instruction)
{
	return (instruction & 0x000007c0) >> 6;
}

short get_immediate16(int instruction)
{
	return (instruction & 0x0000ffff);
}

short get_offset(int instruction)
{
	return (instruction & 0x0000ffff);
}

int get_base(int instruction)
{
	return get_rs(instruction);
}

int load_word(unsigned int vaddr, char *ram, char *flash)
{
	int word = 0;

	if(vaddr >= REG_START && vaddr <= REG_END)
		return (int)get_reg_val(vaddr);
	else
		vaddr = vaddr & ~0x20000000;
	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		word = *(int *)(flash+vaddr-FLASH_START);
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		word = *(int *)(ram+vaddr-RAM_START);
	word = ntohl(word);
	return word;
}

void store_word(unsigned int vaddr, int val, char *ram, char *flash)
{
	if(vaddr >= REG_START && vaddr <= REG_END)
		goto error;
	else
		vaddr = vaddr & ~0x20000000;

	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		goto error;
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		*(int *)(ram+vaddr-RAM_START) = htonl(val);

	return;
error:
	(void)0;
/* 	printf("can't write to 0x%x\n", vaddr); */
}

unsigned short load_halfword(unsigned int vaddr, char *ram, char *flash)
{
	short word = 0;

	if(vaddr >= REG_START && vaddr <= REG_END)
		return (short)get_reg_val(vaddr);
	else
		vaddr = vaddr & ~0x20000000;
	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		word = *(short *)(flash+vaddr-FLASH_START);
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		word = *(short *)(ram+vaddr-RAM_START);
	word = ntohs(word);
	return word;
}

void store_halfword(unsigned int vaddr, short val, char *ram, char *flash)
{
	if(vaddr >= REG_START && vaddr <= REG_END)
		goto error;
	else
		vaddr = vaddr & ~0x20000000;
	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		goto error;
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		*(short *)(ram+vaddr-RAM_START) = htons(val);

	if(vaddr >= FAKEFLASH_START && vaddr < FAKEFLASH_END)
		fakeflash_write(vaddr, val);

	
	return;
error:
	(void)0;
/* 	printf("can't write to 0x%x\n", vaddr); */
}

unsigned char load_byte(unsigned int vaddr, char *ram, char *flash)
{
	char byte = 0;

	if(vaddr >= REG_START && vaddr <= REG_END)
		return (char)get_reg_val(vaddr);
	else
		vaddr = vaddr & ~0x20000000;
	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		byte = *(char *)(flash+vaddr-FLASH_START);
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		byte = *(char *)(ram+vaddr-RAM_START);

	if(vaddr >= FAKEFLASH_START && vaddr < FAKEFLASH_END)
		byte = fakeflash_read(vaddr);

	return byte;
}

void store_byte(unsigned int vaddr, char val, char *ram, char *flash)
{
	if(vaddr >= REG_START && vaddr <= REG_END)
		goto error;
	else
		vaddr = vaddr & ~0x20000000;
	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		goto error;
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		*(char *)(ram+vaddr-RAM_START) = val;
	
	return;
error:
	(void)0;
/* 	printf("can't write to 0x%x\n", vaddr); */
}

char *r2rn(int reg)
{
	switch (reg)
	{
	case 0:
		return "$0";
	case 1:
		return "$at";
	case 2:
		return "$v0";
	case 3:
		return "$v1";
	case 4:
		return "$a0";
	case 5:
		return "$a1";
	case 6:
		return "$a2";
	case 7:
		return "$a3";
	case 8:
		return "$t0";
	case 9:
		return "$t1";
	case 10:
		return "$t2";
	case 11:
		return "$t3";
	case 12:
		return "$t4";
	case 13:
		return "$t5";
	case 14:
		return "$t6";
	case 15:
		return "$t7";
	case 16:
		return "$s0";
	case 17:
		return "$s1";
	case 18:
		return "$s2";
	case 19:
		return "$s3";
	case 20:
		return "$s4";
	case 21:
		return "$s5";
	case 22:
		return "$s6";
	case 23:
		return "$s7";
	case 24:
		return "$t8";
	case 25:
		return "$t9";
	case 26:
		return "$k0";
	case 27:
		return "$k1";
	case 28:
		return "$gp";
	case 29:
		return "$sp";
	case 30:
		return "$s8";
	case 31:
		return "$ra";

	}
	return "";
}

#if 0
void dump_regs(struct cpu_state *cpu)
{
  dtrace("$0:  0x%08x $at: 0x%08x $v0: 0x%08x $v1: 0x%08x $a0: 0x%08x $a1: 0x%08x $a2: 0x%08x $a3: 0x%08x\n", cpu->reg[0], cpu->reg[1], cpu->reg[2], cpu->reg[3], cpu->reg[4], cpu->reg[5], cpu->reg[6], cpu->reg[7]);
  dtrace("$t0: 0x%08x $t1: 0x%08x $t2: 0x%08x $t3: 0x%08x $t4: 0x%08x $t5: 0x%08x $t6: 0x%08x $t7: 0x%08x\n", cpu->reg[8], cpu->reg[9], cpu->reg[10], cpu->reg[11], cpu->reg[12], cpu->reg[13], cpu->reg[14], cpu->reg[15]);
  dtrace("$s0: 0x%08x $s1: 0x%08x $s2: 0x%08x $s3: 0x%08x $s4: 0x%08x $s5: 0x%08x $s6: 0x%08x $s7: 0x%08x\n", cpu->reg[16], cpu->reg[17], cpu->reg[18], cpu->reg[19], cpu->reg[20], cpu->reg[21], cpu->reg[22], cpu->reg[23]);
  dtrace("$t8: 0x%08x $t9: 0x%08x $k0: 0x%08x $k1: 0x%08x $gp: 0x%08x $sp: 0x%08x $s8: 0x%08x $ra: 0x%08x\n", cpu->reg[24], cpu->reg[25], cpu->reg[26], cpu->reg[27], cpu->reg[28], cpu->reg[29], cpu->reg[30], cpu->reg[31]);
}
#endif

int *get_address(unsigned int vaddr, char *ram, char *flash)
{
	if(vaddr >= REG_START && vaddr <= REG_END)
		return 0;
	else
		vaddr = vaddr & ~0x20000000;

	if(vaddr >= FLASH_START && vaddr < FLASH_END)
		return (int *)(flash+vaddr-FLASH_START);
	
	if(vaddr >= RAM_START && vaddr < RAM_END)
		return (int *)(ram+vaddr-RAM_START);
	return 0;
}

void execute(struct cpu_state *cpu, char *flash, char *ram)
{
	int instruction;
	int opcode;
	int rs ;
	int rt ;
	int rd ;
	int sa ;
	int base;
	unsigned int vaddr;
	int offset;
	short im16;
	static int count = 0;

	dtrace("0x%x: ", cpu->pc);

	if(cpu->pc == 0x80010000)
	    debug = 1;

	if(cpu->pc == 0x80260A5C)
	    printf("%s", ram+(cpu->reg[5]-RAM_START));

	if(cpu->pc == 0x81f837b0)
	  printf("%c", cpu->reg[4]);

	if(cpu->pc == 0x81f800a8)
	  printf("%c", cpu->reg[4]);
	
	instruction = get_instruction(flash, ram, cpu->pc);
	opcode = decode_opcode(instruction);
	if(opcode == 0)
		opcode = decode_special_opcode(instruction);
	else if(opcode == 1)
		opcode = decode_special_branch_opcode(instruction);

	if(cpu->delayed_jump)
	{
		cpu->pc = cpu->jump_pc;
		cpu->delayed_jump = 0;
		cpu->jump_pc = 0;
	}
	else
		cpu->pc += 4;

	base = get_base(instruction);
	im16 = get_immediate16(instruction);
	offset = get_offset(instruction);
	rd = get_rd(instruction);
	rs = get_rs(instruction);
	rt = get_rt(instruction);
	sa = get_sa(instruction);
	vaddr = cpu->reg[base]+offset;

	switch(opcode)
	{
	case INS_J:     /* 00000010 */
		cpu->jump_pc = get_jump_address(instruction, cpu->pc);
		cpu->delayed_jump = 1;
		dtrace("\tj\t0x%x\n", cpu->jump_pc);
		break;
	case INS_JAL:   /* 00000011 */
		cpu->jump_pc = get_jump_address(instruction, cpu->pc);
		cpu->reg[31] = cpu->pc+4;
		cpu->delayed_jump = 1;
		dtrace("\tjal\t0x%x\n", cpu->jump_pc);
		break;
	case INS_BEQ:   /* 00000100 */
		if(cpu->reg[rt] == cpu->reg[rs])
		{
		  cpu->jump_pc = cpu->pc + ((int)offset << 2);
			cpu->delayed_jump = 1;
		}
		dtrace("\tbeq\t%s, %s, 0x%x\033[100D\33[65C(0x%x == 0x%x)\n",
		       r2rn(rt), r2rn(rs), cpu->pc + ((int)offset << 2), cpu->reg[rt], cpu->reg[rs]);
		break;
	case INS_BNE:   /* 00000101 */
		if(cpu->reg[rt] != cpu->reg[rs])
		{
		  cpu->jump_pc = cpu->pc + ((int)offset << 2);
			cpu->delayed_jump = 1;
		}
		dtrace("\tbne\t%s, %s, 0x%x\033[100D\33[65C(0x%x != 0x%x)\n",
		       r2rn(rt), r2rn(rs), cpu->pc + ((int)offset << 2), cpu->reg[rt], cpu->reg[rs]);
		break;
	case INS_BLEZ:  /* 00000110 */
		if(cpu->reg[rs] <= 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
		}
		dtrace("\tblez\t%s, 0x%x\033[100D\33[65C(0x%x <= 0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
	case INS_BGTZ:  /* 00000111 */
		if(cpu->reg[rs] > 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
		}
		dtrace("\tbgtz\t%s, 0x%x\033[100D\33[65C(0x%x > 0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
	case INS_ADDI:  /* 00001000 */
		cpu->reg[rt] = cpu->reg[rs] + (int)im16;
		dtrace("\taddi\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_ADDIU: /* 00001001 */
		cpu->reg[rt] = cpu->reg[rs] + (int)im16;
		dtrace("\taddiu\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_SLTI:  /* 00001010 */
		cpu->reg[rt] = cpu->reg[rs] < (unsigned int)(int)im16;
		dtrace("\tslti\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_SLTIU: /* 00001011 */
	  cpu->reg[rt] = (unsigned int)cpu->reg[rs] < (int)(unsigned short)im16;
		dtrace("\tsltiu\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_ANDI:  /* 00001100 */
		cpu->reg[rt] = cpu->reg[rs] & (int)(unsigned short)im16;
		dtrace("\tandi\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), (int)(unsigned short)im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_ORI:   /* 00001101 */
		cpu->reg[rt] = cpu->reg[rs] | (int)(unsigned short)im16;
		dtrace("\tori\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), (int)(unsigned short)im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_XORI:  /* 00001110 */
		cpu->reg[rt] = cpu->reg[rs] ^ (int)(unsigned short)im16;
		dtrace("\txori\t%s, %s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), r2rn(rs), (int)(unsigned short)im16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_LUI:   /* 00001111 */
		cpu->reg[rt] = im16 << 16;
		dtrace("\tlui\t%s, 0x%x\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rt), im16<<16, r2rn(rt), cpu->reg[rt]);
		break;
	case INS_COP0:  /* 00010000 */
		/* don't handle at the moment */
		dtrace("\tcop0\n");
		break;
	case INS_COP1:  /* 00010001 */
		printf("\tcop1 not implemented\n");
		exit(1);
		break;
	case INS_COP2:  /* 00010010 */
		printf("\tcop2 not implemented\n");
		exit(1);
		break;
	case INS_NA1:   /* 00010011 */
		printf("\tundefined instruction na1 not implemented\n");
		exit(1);
		break;
	case INS_BEQL:  /* 00010100 */
		if(cpu->reg[rt] == cpu->reg[rs])
		{
		  cpu->jump_pc = cpu->pc + ((int)offset << 2);
		  cpu->delayed_jump = 1;
		}
		else
		  cpu->pc += 4;
		dtrace("\tbeql\t%s, %s, 0x%x\033[100D\33[65C(0x%x != 0x%x)\n",
		       r2rn(rt), r2rn(rs), cpu->pc + ((int)offset << 2), cpu->reg[rt], cpu->reg[rs]);
		break;
	case INS_BNEL:   /* 00010101 */
		if(cpu->reg[rt] != cpu->reg[rs])
		{
		  cpu->jump_pc = cpu->pc + ((int)offset << 2);
		  cpu->delayed_jump = 1;
		}
		else
		  cpu->pc += 4;
		dtrace("\tbnel\t%s, %s, 0x%x\033[100D\33[65C(0x%x != 0x%x)\n",
		       r2rn(rt), r2rn(rs), cpu->pc + ((int)offset << 2), cpu->reg[rt], cpu->reg[rs]);
		break;
	case INS_BLEZL: /* 00010110 */
		if(cpu->reg[rs] <= 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
		}
		else
		  cpu->pc += 4;
		dtrace("\tblezl\t%s, 0x%x\033[100D\33[65C(0x%x <= 0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
		break;
	case INS_BGTZL: /* 00010111 */
		if(cpu->reg[rs] > 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
		}
		else
		  cpu->pc += 4;
		dtrace("\tbgtzl\t%s, 0x%x\033[100D\33[65C(0x%x > 0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
		break;
	case INS_NA2:   /* 00011000 */
		printf("\tundefined instruction na2 not implemented\n");
		exit(1);
		break;
	case INS_NA3:   /* 00011001 */
		printf("\tundefined instruction na3 not implemented\n");
		exit(1);
		break;
	case INS_NA4:   /* 00011010 */
		printf("\tundefined instruction na4 not implemented\n");
		exit(1);
		break;
	case INS_NA5:   /* 00011011 */
		printf("\tundefined instruction na5 not implemented\n");
		exit(1);
		break;
	case INS_NA6:   /* 00011100 */
		printf("\tundefined instruction na6 not implemented\n");
		exit(1);
		break;
	case INS_NA7:   /* 00011101 */
		printf("\tundefined instruction na7 not implemented\n");
		exit(1);
		break;
	case INS_NA8:   /* 00011110 */
		printf("\tundefined instruction na8 not implemented\n");
		exit(1);
		break;
	case INS_NA9:   /* 00011111 */
		printf("\tundefined instruction na9 not implemented\n");
		exit(1);
		break;
	case INS_LB:    /* 00100000 */
		vaddr = cpu->reg[base]+offset;
		cpu->reg[rt] = load_byte(vaddr, ram, flash);
		dtrace("\tlh\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	case INS_LH:    /* 00100001 */
		vaddr = cpu->reg[base]+offset;
		if(vaddr & 0x1)
			dtrace("Exception address error\n");
		cpu->reg[rt] = load_halfword(vaddr, ram, flash);
		dtrace("\tlh\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	case INS_LWL:   /* 00100010 */
	  {
	    char byte;
	    unsigned int word;
	    	vaddr = cpu->reg[base]+(int)offset;
		byte = (vaddr & 0x03);
		word = load_word(vaddr & 0xfffffffc, ram, flash);
		switch(byte)
		  {
		  case 0:
		    break;
		  case 1:
		    word = (cpu->reg[rt] & 0x000000ff) | ((word & 0x00ffffff) << 8);
		    break;
		  case 2:
		    word = (cpu->reg[rt] & 0x0000ffff) | ((word & 0x0000ffff) << 16);
		    break;
		  case 3:
		    word = (cpu->reg[rt] & 0x00ffffff) | ((word & 0x000000ff) << 24);
		    break;
		  }
		cpu->reg[rt] = word;
		dtrace("\tlwl\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	  }
	case INS_LW:    /* 00100011 */
		vaddr = cpu->reg[base]+offset;
		if(vaddr & 0x3)
			dtrace("Exception address error\n");
		cpu->reg[rt] = load_word(vaddr, ram, flash);
		dtrace("\tlw\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	case INS_LBU:   /* 00100100 */
		vaddr = cpu->reg[base]+offset;
		cpu->reg[rt] = (int)load_byte(vaddr, ram, flash);
		dtrace("\tlbu\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	case INS_LHU:   /* 00100101 */
		vaddr = cpu->reg[base]+offset;
		if(vaddr & 0x1)
			dtrace("Exception address error\n");
		cpu->reg[rt] = load_halfword(vaddr, ram, flash);
		dtrace("\tlhu\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	case INS_LWR:   /* 00100110 */
	  {
	    char byte;
	    unsigned int word;
	    	vaddr = cpu->reg[base]+(int)offset;
		byte = (vaddr & 0x03);
		word = load_word(vaddr & 0xfffffffc, ram, flash);
		switch(byte)
		  {
		  case 0:
		    word = (cpu->reg[rt] & 0xffffff00) | ((word & 0xff000000) >> 24);
		    break;
		  case 1:
		    word = (cpu->reg[rt] & 0xffff0000) | ((word & 0xffff0000) >> 16);
		    break;
		  case 2:
		    word = (cpu->reg[rt] & 0xff000000) | ((word & 0xffffff00) >> 8);
		    break;
		  case 3:
		    break;
		  }
		cpu->reg[rt] = word;
		dtrace("\tlwr\t%s, 0x%x(%s)\033[100D\33[65C(%s = 0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), r2rn(rt), cpu->reg[rt], vaddr);
		break;
	  }
	case INS_NA10:   /* 00100111 */
		printf("\tundefined instruction na10 not implemented\n");
		exit(1);
		break;
	case INS_SB:    /* 00101000 */
		vaddr = cpu->reg[base]+offset;
		store_byte(vaddr, cpu->reg[rt], ram, flash);
		dtrace("\tsb\t%s, 0x%x(%s)\033[100D\33[65C(0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), cpu->reg[rt], vaddr);
		break;
	case INS_SH:    /* 00101001 */
		vaddr = cpu->reg[base]+offset;
		if(vaddr & 0x1)
			dtrace("Exception address error\n");
		store_halfword(vaddr, cpu->reg[rt], ram, flash);
		dtrace("\tsh\t%s, 0x%x(%s)\033[100D\33[65C(0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), cpu->reg[rt], vaddr);
		break;
	case INS_SWL:   /* 00101010 */
	  {
		char byte;
		unsigned int word;
	    	vaddr = cpu->reg[base]+(int)offset;
		byte = (vaddr & 0x03);
		word = load_word(vaddr & 0xfffffffc, ram, flash);
		switch(byte)
		  {
		  case 0:
		    word = cpu->reg[rt];
		    break;
		  case 1:
		    word = ((cpu->reg[rt] & 0xffffff00) >>  8) | ((word & 0xff000000));
		    break;
		  case 2:
		    word = ((cpu->reg[rt] & 0xffff0000) >> 16) | ((word & 0xffff0000));
		    break;
		  case 3:
		    word = ((cpu->reg[rt] & 0xff000000) >> 24) | ((word & 0xffffff00));
		    break;
		  }
		store_word(vaddr & 0xfffffffc, word, ram, flash);
		dtrace("\tswl\t%s, 0x%x(%s)\033[100D\33[65C(0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), cpu->reg[rt], vaddr);
		break;
	  }
	case INS_SW:    /* 00101011 */
		vaddr = cpu->reg[base]+offset;
		if(vaddr & 0x3)
			dtrace("Exception address error\n");
		store_word(vaddr, cpu->reg[rt], ram, flash);
		dtrace("\tsw\t%s, 0x%x(%s)\033[100D\33[65C(0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), cpu->reg[rt], vaddr);
		break;
	case INS_NA11:   /* 00101100 */
		printf("\tundefined instruction na11 not implemented\n");
		exit(1);
		break;
	case INS_NA12:   /* 00101101 */
		printf("\tundefined instruction na12 not implemented\n");
		exit(1);
		break;
	case INS_SWR:   /* 00101110 */
	  {
		char byte;
		unsigned int word;
	    	vaddr = cpu->reg[base]+(int)offset;
		byte = (vaddr & 0x03);
		word = load_word(vaddr & 0xfffffffc, ram, flash);
		switch(byte)
		  {
		  case 0:
		    word = ((cpu->reg[rt] & 0x000000ff) << 24) | (word & 0x00ffffff);
		    break;
		  case 1:
		    word = ((cpu->reg[rt] & 0x0000ffff) << 16) | (word & 0x0000ffff);
		    break;
		  case 2:
		    word = ((cpu->reg[rt] & 0x00ffffff) <<  8) | (word & 0x000000ff);
		    break;
		  case 3:
		    word = cpu->reg[rt];
		    break;
		  }
		store_word(vaddr & 0xfffffffc, word, ram, flash);
		dtrace("\tswr\t%s, 0x%x(%s)\033[100D\33[65C(0x%x) @ 0x%x\n",
		       r2rn(rt), offset, r2rn(base), cpu->reg[rt], vaddr);
		break;
	  }
	case INS_CACHE: /* 00101111 */
		/* don't handle at the moment */
		dtrace("\tcache\n");
		break;
	case INS_SLL:   /* 01000000 */
		cpu->reg[rd] = (unsigned int)cpu->reg[rt] << sa;
		if(sa == 0)
		  dtrace("\tnop\n");
		else
		  dtrace("\tsll\t%s, %s, %d\033[100D\33[65C(%s = 0x%x)\n",
			 r2rn(rd), r2rn(rt), sa, r2rn(rd), cpu->reg[rd]);
		break;
	case INS_MOVF:   /* 01000001 */
		printf("\tmovf not implemented\n");
		exit(1);
		break;
	case INS_SRL:   /* 01000010 */
		cpu->reg[rd] = (unsigned int)cpu->reg[rt] >> sa;
		dtrace("\tsrl\t%s, %s, %d\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rt), sa, r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SRA:   /* 01000011 */
		cpu->reg[rd] = cpu->reg[rt] >> sa;
		dtrace("\tsllv\t%s, %s, %d\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rt), sa, r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SLLV:  /* 01000100 */
		cpu->reg[rd] = (unsigned int)cpu->reg[rt] << (cpu->reg[rs] & 0x1f);
		dtrace("\tsllv\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rt), r2rn(rs), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SRLV:  /* 01000110 */
		cpu->reg[rd] = (unsigned int)cpu->reg[rt] >> (cpu->reg[rs] & 0x1f);
		dtrace("\tsrlv\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rt), r2rn(rs), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SRAV:  /* 01000111 */
		cpu->reg[rd] = (int)cpu->reg[rt] >> (cpu->reg[rs] & 0x1f);
		dtrace("\tsrav\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rt), r2rn(rs), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_JR:    /* 01001000 */
		cpu->jump_pc = cpu->reg[rs] & ~0x20000000;
		cpu->delayed_jump = 1;
		dtrace("\tjr\t%s\033[100D\33[65C(0x%x)\n",
		       r2rn(rs), cpu->jump_pc);
		break;
	case INS_JALR:  /* 01001001 */
		cpu->jump_pc = cpu->reg[rs] & ~0x20000000;
		cpu->reg[rd] = cpu->pc+4;
		cpu->delayed_jump = 1;
		dtrace("\tjr\t%s, %s\033[100D\33[65C(0x%x)\n",
		       r2rn(rs), r2rn(rd), cpu->jump_pc);
		break;
	case INS_MFHI:  /* 01010000 */
		cpu->reg[rd] = cpu->HI;
		dtrace("\tmfhi\t%s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_MTHI:  /* 01010001 */
		cpu->HI = cpu->reg[rs];
		dtrace("\tmthi\t%s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rs), "HI", cpu->HI);
		break;
	case INS_MFLO:  /* 01010010 */
		cpu->reg[rd] = cpu->LO;
		dtrace("\tmflo\t%s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_MTLO:  /* 01010011 */
		cpu->LO = cpu->reg[rs];
		dtrace("\tmtlo\t%s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rs), "LO", cpu->LO);
		break;
	case INS_MULT:  /* 01011000 */
		cpu->HI = ((long)cpu->reg[rs] * (long)cpu->reg[rt]) >> 32;
		cpu->LO = ((long)cpu->reg[rs] * (long)cpu->reg[rt]) & 0xffffffff;
		dtrace("\tmult\t%s, %s\033[100D\33[65C(hi = 0x%x lo=0x%x)\n",
		       r2rn(rs), r2rn(rt), cpu->HI, cpu->LO);
		break;
	case INS_MULTU: /* 01011001 */
		cpu->HI = ((unsigned long)(unsigned int)cpu->reg[rs] * (unsigned long)(unsigned int)cpu->reg[rt]) >> 32;
		cpu->LO = ((unsigned long)(unsigned int)cpu->reg[rs] * (unsigned long)(unsigned int)cpu->reg[rt]) & 0xffffffff;
		dtrace("\tmultu\t%s, %s\033[100D\33[65C(hi = 0x%x lo=0x%x)\n",
		       r2rn(rs), r2rn(rt), cpu->HI, cpu->LO);
		break;
	case INS_DIV:   /* 01011010 */
		cpu->LO = cpu->reg[rs] / cpu->reg[rt];
		cpu->HI = cpu->reg[rs] % cpu->reg[rt];
		dtrace("\tdiv\t%s, %s\033[100D\33[65C(hi = 0x%x lo=0x%x)\n",
		       r2rn(rs), r2rn(rt), cpu->HI, cpu->LO);
		break;
	case INS_DIVU:  /* 01011011 */
		cpu->LO = (unsigned int)cpu->reg[rs] / (unsigned int)cpu->reg[rt];
		cpu->HI = (unsigned int)cpu->reg[rs] % (unsigned int)cpu->reg[rt];
		dtrace("\tdivu\t%s, %s\033[100D\33[65C(hi = 0x%x lo=0x%x)\n",
		       r2rn(rs), r2rn(rt), cpu->HI, cpu->LO);
		break;
	case INS_ADDU:  /* 01100001 */
		cpu->reg[rd] = cpu->reg[rs] + cpu->reg[rt];
		dtrace("\taddu\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_ADD:   /* 01100000 */
		cpu->reg[rd] = cpu->reg[rs] + cpu->reg[rt];
		dtrace("\tadd\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SUB:   /* 01100010 */
		cpu->reg[rd] = cpu->reg[rs] - cpu->reg[rt];
		dtrace("\tsub\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SUBU:  /* 01100011 */
		cpu->reg[rd] = cpu->reg[rs] - cpu->reg[rt];
		dtrace("\tsubu\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_AND:   /* 01100100 */
		cpu->reg[rd] = cpu->reg[rs] & cpu->reg[rt];
		dtrace("\tand\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_OR:    /* 01100101 */
		cpu->reg[rd] = cpu->reg[rs] | cpu->reg[rt];
		dtrace("\tor\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_XOR:   /* 01100110 */
		cpu->reg[rd] = cpu->reg[rs] ^ cpu->reg[rt];
		dtrace("\tor\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_NOR:   /* 01100111 */
		cpu->reg[rd] = ~(cpu->reg[rs] | cpu->reg[rt]);
		dtrace("\tnor\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SLT:   /* 01101010 */
		cpu->reg[rd] = cpu->reg[rs] < cpu->reg[rt];
		dtrace("\tslt\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_SLTU:  /* 01101011 */
		cpu->reg[rd] = (unsigned int)cpu->reg[rs] < (unsigned int)cpu->reg[rt];
		dtrace("\tsltu\t%s, %s, %s\033[100D\33[65C(%s = 0x%x)\n",
		       r2rn(rd), r2rn(rs), r2rn(rt), r2rn(rd), cpu->reg[rd]);
		break;
	case INS_BLTZ:  /* 10000000 */
		if(cpu->reg[rs] < 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
			break;
		}
		dtrace("\tbltz\t%s, 0x%x\033[100D\33[65C(0x%x < 0x0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
	case INS_BGEZ:  /* 10000001 */
		if(cpu->reg[rs] >= 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
			break;
		}
		dtrace("\tbgez\t%s, 0x%x\033[100D\33[65C(0x%x < 0x0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
	case INS_BLTZL:  /* 10000000 */
		if(cpu->reg[rs] < 0)
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->delayed_jump = 1;
		}
		else
		  cpu->pc += 4;
		dtrace("\tbltzl\t%s, 0x%x\033[100D\33[65C(0x%x < 0x0)\n",
		       r2rn(rs), cpu->pc + (offset << 2), cpu->reg[rs]);
		break;
	case INS_BAL:   /* 10000010 */
		{
			cpu->jump_pc = cpu->pc + (offset << 2);
			cpu->reg[31] = cpu->pc + 4;
			cpu->delayed_jump = 1;
		}
		dtrace("\tbal\t%s, 0x%x\033[100D\33[65C(0x%x)\n",
		       "$0", cpu->pc + (offset << 2), cpu->jump_pc);
		break;
	default:
		printf("\nunknown instruction at 0x%x opcode(0x%x)\n",
		       cpu->pc-4, decode_opcode(instruction));
		printf("unknown instruction at 0x%x special_opcode(0x%x)\n",
		       cpu->pc-4, decode_special_opcode(instruction));
		printf("unknown instruction at 0x%x special_branch_opcode(0x%x)\n",
		       cpu->pc-4, decode_special_branch_opcode(instruction));
	  exit(0);
	}

	count++;
	if(count % 10000000 == 0)
	  timer_int = 1;
/* 	if(count % 10000 == 0) */
/* 		printf("count: %d\n", count); */
}

void initialize_cpu(struct cpu_state *cpu, int start_address)
{
	int i;

	for(i = 0; i < 32; i++)
	{
		cpu->reg[i] = 0;
	}
	
	cpu->pc = start_address;
	cpu->delayed_jump = 0;
	cpu->jump_pc = 0;
	cpu->HI = 0;
	cpu->LO = 0;
}

int main(void)
{
	int fd;
	char *flash;
	char *ram;
	struct cpu_state cpu;

	flash = malloc(FLASH_SIZE);
	ram = malloc(RAM_SIZE);
	bzero((void *)flash, FLASH_SIZE);
	bzero((void *)ram, RAM_SIZE);

	fd = open("fw.bin", O_RDONLY);

	read(fd, (void *)flash, FLASH_SIZE);

	initialize_cpu(&cpu, FLASH_START);

	while(1)
		execute(&cpu, flash, ram);

	return 0;
}
