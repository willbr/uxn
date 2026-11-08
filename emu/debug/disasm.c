#include <stdio.h>
#include <string.h>
#include "disasm.h"

static const char *const names[32] = {
	"BRK", "INC", "POP", "NIP", "SWP", "ROT", "DUP", "OVR",
	"EQU", "NEQ", "GTH", "LTH", "JMP", "JCN", "JSR", "STH",
	"LDZ", "STZ", "LDR", "STR", "LDA", "STA", "DEI", "DEO",
	"ADD", "SUB", "MUL", "DIV", "AND", "ORA", "EOR", "SFT"
};

int uxn_disasm(const ux_u8 *ram, ux_u16 addr, char *out, int outlen)
{
	ux_u8 op = ram[addr];
	char text[32];
	int len = 1;

	switch (op) {
	case 0x00: strcpy(text, "BRK"); break;
	case 0x20: case 0x40: case 0x60: {
		ux_u16 off = (ux_u16)((ram[(ux_u16)(addr + 1)] << 8) | ram[(ux_u16)(addr + 2)]);
		ux_u16 target = (ux_u16)(addr + 3 + off); /* signed offset, wraps mod 64K */
		sprintf(text, "%s %04X", op == 0x20 ? "JCI" : op == 0x40 ? "JMI" : "JSI", (unsigned)target);
		len = 3;
		break;
	}
	case 0x80: case 0xc0:
		sprintf(text, "LIT%s %02x", op == 0xc0 ? "r" : "", (unsigned)ram[(ux_u16)(addr + 1)]);
		len = 2;
		break;
	case 0xa0: case 0xe0:
		sprintf(text, "LIT2%s %02x%02x", op == 0xe0 ? "r" : "",
			(unsigned)ram[(ux_u16)(addr + 1)], (unsigned)ram[(ux_u16)(addr + 2)]);
		len = 3;
		break;
	default:
		sprintf(text, "%s%s%s%s", names[op & 0x1f], (op & 0x20) ? "2" : "",
			(op & 0x80) ? "k" : "", (op & 0x40) ? "r" : "");
		break;
	}
	if (outlen > 0) {
		strncpy(out, text, (size_t)outlen - 1);
		out[outlen - 1] = '\0';
	}
	return len;
}
