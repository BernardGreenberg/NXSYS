#pragma once

enum MACH_OP {      MOP_AND, MOP_OR, MOP_JZ, MOP_JNZ, MOP_RETF, MOP_NOJUMP,
		    MOP_JMP, MOP_TST, MOP_JMPL, MOP_XOR, MOP_LDAL, MOP_CLZ,
		    MOP_STZ, MOP_RET, MOP_RPUSH, MOP_RPOP, MOP_LDBLI8,
		    MOP_LOADWD, MOP_CALLIND, MOP_MOVZX8, MOP_LEAVE, MOP_RRET,
		    MOP_SETNZ};

struct OPDEF {
    const char * mnemonic;
    char unsigned opcode;
    short flags;
};

#define OPF_8BIT       0x0001
#define OPF_RVOPD      0x0002
#define OPF_NOREGOP    0x0004
#define OPF_0F         0x0008

#define INTEL_OP_INFO_DATA { \
    {"and",	0x24,	OPF_8BIT}, \
    {"or",	0x26,	OPF_8BIT}, \
    {"jz",	0x74,	0}, \
    {"jnz",	0x75,	0},  \
    {"retf",	0xCB,	0},  \
    {"nojump",	0x00,	0},  \
    {"jmp",	0xEB,	0},  \
    {"test",	0x84,	OPF_8BIT|OPF_RVOPD},	/* MOP_TST (BL) */ \
    {"jmp",	0xE9,	0},		/* JUMP LONG  */ \
    {"xor",	0x34,	OPF_8BIT}, \
    {"mov",	0x8A,	OPF_8BIT},	/* MOP_LDAL */ \
    {"and",	0x24,	OPF_8BIT},	/* MOP_CLZ */ \
    {"or",	0x0C,	OPF_8BIT},	/* MOP_STZ */ \
    {"ret",	0xC3,	0},		/* MOP_RET */ \
    {"push",	0x50,	0},		/* MOP_RPUSH */ \
    {"pop",	0x58,	0},		/* MOP_RPOP */ \
    {"mov",	0xB3,	OPF_8BIT},	/* MOP LDBLI8 */ \
    {"mov",	0x8B,	0},		/* MOP_LOADWD */ \
    {"call",	0xFF,	OPF_NOREGOP},	/* MOP_CALLIND */ \
    {"movzx",	0xB6,	OPF_0F|OPF_8BIT},/* MOP_MOVZX8 */ \
    {"leave",	0xC9,	0},              /* MOP_LEAVE */ \
    {"ret",	0xC2,	0}, 		/* MOP_RRET */\
    {"setnz",   0x95,   OPF_0F|OPF_8BIT|OPF_NOREGOP} \
}


