/* uxn disassembler. C89 plus libc. */
#ifndef DISASM_H
#define DISASM_H

#include "uxn.h"

/* Disassembles the instruction at addr. Returns its length (1..3) and writes
 * NUL-terminated text to out (truncated to outlen, which must be >= 1).
 * Mode suffixes come in the order 2, k, r (ADD2kr). LIT shows its operand
 * (LIT 12, LIT2 1234); JCI, JMI and JSI show the absolute target (JSI 0104).
 * Operand bytes wrap around the 64K address space. */
int uxn_disasm(const ux_u8 *ram, ux_u16 addr, char *out, int outlen);

#endif
