#ifndef _C64_OPCODES_H_
#define _C64_OPCODES_H_

#include "SYS_Defs.h"

enum OpcodeIllegalStatus
{
	OP_STANDARD,
	OP_ILLEGAL
};

enum OpcodeAddressingMode
{
	ADDR_IMP,								// 1
	ADDR_IMM,	// #$00						// 2
	ADDR_ZP ,	// $00						// 2
	ADDR_ZPX,	// $00,X					// 2
	ADDR_ZPY,	// $00,Y					// 2
	ADDR_IZX,	// ($00,X)					// 2
	ADDR_IZY,	// ($00),Y					// 2
	ADDR_ABS,	// $0000					// 3
	ADDR_ABX,	// $0000,X					// 3
	ADDR_ABY,	// $0000,Y					// 3
	ADDR_IND,	// ($0000)					// 3
	ADDR_REL,	// $0000 (PC-relative)		// 2
	ADDR_UNKNOWN
};

typedef struct opcode_s
{
	OpcodeIllegalStatus isIllegal;
	const char *name;
	OpcodeAddressingMode addressingMode;
	uint8 addressingLength;
	uint8 numCycles;
	bool addCycleOnPageBoundary;
} opcode_t;

// http://www.oxyron.de/html/opcodes02.html
static const opcode_t opcodes[256] =
{
	// x0
	{ OP_STANDARD, "BRK", ADDR_IMP, 1, 7, false },
	{ OP_STANDARD, "ORA", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "SLO", ADDR_IZX, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "ORA", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "ASL", ADDR_ZP , 2, 5, false },
	{ OP_ILLEGAL , "SLO", ADDR_ZP , 2, 5, false },
	{ OP_STANDARD, "PHP", ADDR_IMP, 1, 3, false },
	{ OP_STANDARD, "ORA", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "ASL", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "ANC", ADDR_IMM, 2, 2, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "ORA", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "ASL", ADDR_ABS, 3, 6, false },
	{ OP_ILLEGAL , "SLO", ADDR_ABS, 3, 6, false },
	
	// x1
	{ OP_STANDARD, "BPL", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "ORA", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "SLO", ADDR_IZY, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "ORA", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "ASL", ADDR_ZPX, 2, 6, false },
	{ OP_ILLEGAL , "SLO", ADDR_ZPX, 2, 6, false },
	{ OP_STANDARD, "CLC", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "ORA", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "SLO", ADDR_ABY, 3, 7, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "ORA", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "ASL", ADDR_ABX, 3, 7, false },
	{ OP_ILLEGAL , "SLO", ADDR_ABX, 3, 7, false },
	
	// x2
	{ OP_STANDARD, "JSR", ADDR_ABS, 3, 6, false },
	{ OP_STANDARD, "AND", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "RLA", ADDR_IZX, 2, 8, false },
	{ OP_STANDARD, "BIT", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "AND", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "ROL", ADDR_ZP , 2, 5, false },
	{ OP_ILLEGAL , "RLA", ADDR_ZP , 2, 5, false },
	{ OP_STANDARD, "PLP", ADDR_IMP, 1, 4, false },
	{ OP_STANDARD, "AND", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "ROL", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "ANC", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "BIT", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "AND", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "ROL", ADDR_ABS, 3, 6, false },
	{ OP_ILLEGAL , "RLA", ADDR_ABS, 3, 6, false },
	
	// x3
	{ OP_STANDARD, "BMI", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "AND", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "RLA", ADDR_IZY, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "AND", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "ROL", ADDR_ZPX, 2, 6, false },
	{ OP_ILLEGAL , "RLA", ADDR_ZPX, 2, 6, false },
	{ OP_STANDARD, "SEC", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "AND", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "RLA", ADDR_ABY, 3, 7, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "AND", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "ROL", ADDR_ABX, 3, 7, false },
	{ OP_ILLEGAL , "RLA", ADDR_ABX, 3, 7, false },
	
	// x4
	{ OP_STANDARD, "RTI", ADDR_IMP, 1, 6, false },
	{ OP_STANDARD, "EOR", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "SRE", ADDR_IZX, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "EOR", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "LSR", ADDR_ZP , 2, 5, false },
	{ OP_ILLEGAL , "SRE", ADDR_ZP , 2, 5, false },
	{ OP_STANDARD, "PHA", ADDR_IMP, 1, 3, false },
	{ OP_STANDARD, "EOR", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "LSR", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "ALR", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "JMP", ADDR_ABS, 3, 3, false },
	{ OP_STANDARD, "EOR", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "LSR", ADDR_ABS, 3, 6, false },
	{ OP_ILLEGAL , "SRE", ADDR_ABS, 3, 6, false },
	
	// x5
	{ OP_STANDARD, "BVC", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "EOR", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "SRE", ADDR_IZY, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "EOR", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "LSR", ADDR_ZPX, 2, 6, false },
	{ OP_ILLEGAL , "SRE", ADDR_ZPX, 2, 6, false },
	{ OP_STANDARD, "CLI", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "EOR", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "SRE", ADDR_ABY, 3, 7, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "EOR", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "LSR", ADDR_ABX, 3, 7, false },
	{ OP_ILLEGAL , "SRE", ADDR_ABX, 3, 7, false },
	
	// x6
	{ OP_STANDARD, "RTS", ADDR_IMP, 1, 6, false },
	{ OP_STANDARD, "ADC", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "RRA", ADDR_IZX, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "ADC", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "ROR", ADDR_ZP , 2, 5, false },
	{ OP_ILLEGAL , "RRA", ADDR_ZP , 2, 5, false },
	{ OP_STANDARD, "PLA", ADDR_IMP, 1, 4, false },
	{ OP_STANDARD, "ADC", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "ROR", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "ARR", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "JMP", ADDR_IND, 3, 5, false },
	{ OP_STANDARD, "ADC", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "ROR", ADDR_ABS, 3, 6, false },
	{ OP_ILLEGAL , "RRA", ADDR_ABS, 3, 6, false },
	
	// x7
	{ OP_STANDARD, "BVS", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "ADC", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "RRA", ADDR_IZY, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "ADC", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "ROR", ADDR_ZPX, 2, 6, false },
	{ OP_ILLEGAL , "RRA", ADDR_ZPX, 2, 6, false },
	{ OP_STANDARD, "SEI", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "ADC", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "RRA", ADDR_ABY, 3, 7, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "ADC", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "ROR", ADDR_ABX, 3, 7, false },
	{ OP_ILLEGAL , "RRA", ADDR_ABX, 3, 7, false },
	
	// x8
	{ OP_ILLEGAL , "NOP", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "STA", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "NOP", ADDR_IMM, 2, 2, false },
	{ OP_ILLEGAL , "SAX", ADDR_IZX, 2, 6, false },
	{ OP_STANDARD, "STY", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "STA", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "STX", ADDR_ZP , 2, 3, false },
	{ OP_ILLEGAL , "SAX", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "DEY", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "NOP", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "TXA", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "XAA", ADDR_IMM, 2, 2, false },	//COLOR="#FF0000"
	{ OP_STANDARD, "STY", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "STA", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "STX", ADDR_ABS, 3, 4, false },
	{ OP_ILLEGAL , "SAX", ADDR_ABS, 3, 4, false },
	
	// x9
	{ OP_STANDARD, "BCC", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "STA", ADDR_IZY, 2, 6, false },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "AHX", ADDR_IZY, 2, 6, false }, //COLOR="#0000FF"
	{ OP_STANDARD, "STY", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "STA", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "STX", ADDR_ZPY, 2, 4, false },
	{ OP_ILLEGAL , "SAX", ADDR_ZPY, 2, 4, false },
	{ OP_STANDARD, "TYA", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "STA", ADDR_ABY, 3, 5, false },
	{ OP_STANDARD, "TXS", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "TAS", ADDR_ABY, 3, 5, false }, //COLOR="#0000FF"
	{ OP_ILLEGAL , "SHY", ADDR_ABX, 3, 5, false }, //COLOR="#0000FF"
	{ OP_STANDARD, "STA", ADDR_ABX, 3, 5, false },
	{ OP_ILLEGAL , "SHX", ADDR_ABY, 3, 5, false }, //COLOR="#0000FF"
	{ OP_ILLEGAL , "AHX", ADDR_ABY, 3, 5, false }, //COLOR="#0000FF"
	
	// xA
	{ OP_STANDARD, "LDY", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "LDA", ADDR_IZX, 2, 6, false },
	{ OP_STANDARD, "LDX", ADDR_IMM, 2, 2, false },
	{ OP_ILLEGAL , "LAX", ADDR_IZX, 2, 6, false },
	{ OP_STANDARD, "LDY", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "LDA", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "LDX", ADDR_ZP , 2, 3, false },
	{ OP_ILLEGAL , "LAX", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "TAY", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "LDA", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "TAX", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "LAX", ADDR_IMM, 2, 2, false }, //COLOR="#FF0000"
	{ OP_STANDARD, "LDY", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "LDA", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "LDX", ADDR_ABS, 3, 4, false },
	{ OP_ILLEGAL , "LAX", ADDR_ABS, 3, 4, false },
	
	// xB
	{ OP_STANDARD, "BCS", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "LDA", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "LAX", ADDR_IZY, 2, 5, true  },
	{ OP_STANDARD, "LDY", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "LDA", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "LDX", ADDR_ZPY, 2, 4, false },
	{ OP_ILLEGAL , "LAX", ADDR_ZPY, 2, 4, false },
	{ OP_STANDARD, "CLV", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "LDA", ADDR_ABY, 3, 4, true  },
	{ OP_STANDARD, "TSX", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "LAS", ADDR_ABY, 3, 4, true  },
	{ OP_STANDARD, "LDY", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "LDA", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "LDX", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "LAX", ADDR_ABY, 3, 4, true  },
	
	// xC
	{ OP_STANDARD, "CPY", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "CMP", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "NOP", ADDR_IMM, 2, 2, false },
	{ OP_ILLEGAL , "DCP", ADDR_IZX, 2, 8, false },
	{ OP_STANDARD, "CPY", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "CMP", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "DEC", ADDR_ZP , 2, 5, false },
	{ OP_ILLEGAL , "DCP", ADDR_ZP , 2, 5, false },
	{ OP_STANDARD, "INY", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "CMP", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "DEX", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "AXS", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "CPY", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "CMP", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "DEC", ADDR_ABS, 3, 6, false },
	{ OP_ILLEGAL , "DCP", ADDR_ABS, 3, 6, false },
	
	// xD
	{ OP_STANDARD, "BNE", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "CMP", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "DCP", ADDR_IZY, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "CMP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "DEC", ADDR_ZPX, 2, 6, false },
	{ OP_ILLEGAL , "DCP", ADDR_ZPX, 2, 6, false },
	{ OP_STANDARD, "CLD", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "CMP", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "DCP", ADDR_ABY, 3, 7, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "CMP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "DEC", ADDR_ABX, 3, 7, false },
	{ OP_ILLEGAL , "DCP", ADDR_ABX, 3, 7, false },
	
	// xE
	{ OP_STANDARD, "CPX", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "SBC", ADDR_IZX, 2, 6, false },
	{ OP_ILLEGAL , "NOP", ADDR_IMM, 2, 2, false },
	{ OP_ILLEGAL , "ISC", ADDR_IZX, 2, 8, false },
	{ OP_STANDARD, "CPX", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "SBC", ADDR_ZP , 2, 3, false },
	{ OP_STANDARD, "INC", ADDR_ZP , 2, 5, false },
	{ OP_ILLEGAL , "ISC", ADDR_ZP , 2, 5, false },
	{ OP_STANDARD, "INX", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "SBC", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "SBC", ADDR_IMM, 2, 2, false },
	{ OP_STANDARD, "CPX", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "SBC", ADDR_ABS, 3, 4, false },
	{ OP_STANDARD, "INC", ADDR_ABS, 3, 6, false },
	{ OP_ILLEGAL , "ISC", ADDR_ABS, 3, 6, false },
	
	// xF
	{ OP_STANDARD, "BEQ", ADDR_REL, 2, 2, true  },
	{ OP_STANDARD, "SBC", ADDR_IZY, 2, 5, true  },
	{ OP_ILLEGAL , "KIL", ADDR_IMP, 1, 0, false },
	{ OP_ILLEGAL , "ISC", ADDR_IZY, 2, 8, false },
	{ OP_ILLEGAL , "NOP", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "SBC", ADDR_ZPX, 2, 4, false },
	{ OP_STANDARD, "INC", ADDR_ZPX, 2, 6, false },
	{ OP_ILLEGAL , "ISC", ADDR_ZPX, 2, 6, false },
	{ OP_STANDARD, "SED", ADDR_IMP, 1, 2, false },
	{ OP_STANDARD, "SBC", ADDR_ABY, 3, 4, true  },
	{ OP_ILLEGAL , "NOP", ADDR_IMP, 1, 2, false },
	{ OP_ILLEGAL , "ISC", ADDR_ABY, 3, 7, false },
	{ OP_ILLEGAL , "NOP", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "SBC", ADDR_ABX, 3, 4, true  },
	{ OP_STANDARD, "INC", ADDR_ABX, 3, 7, false },
	{ OP_ILLEGAL , "ISC", ADDR_ABX, 3, 7, false }
};


#endif
