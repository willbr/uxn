/* uxn CPU core. Portable ISO C89, freestanding: no includes, no libc, no
 * globals. The host zeroes a Uxn, sets ram (65536 bytes), dei, deo and user,
 * loads the ROM at 0x0100, and calls uxn_eval(). There is no init function. */
#ifndef UXN_H
#define UXN_H

typedef unsigned char ux_u8;
typedef unsigned short ux_u16;

typedef struct UxStack {
	ux_u8 dat[256];
	ux_u8 ptr;
} UxStack;

typedef struct Uxn {
	ux_u8 *ram;
	UxStack wst, rst;
	ux_u8 dev[256];
	ux_u8 (*dei)(struct Uxn *, ux_u8 port);
	void (*deo)(struct Uxn *, ux_u8 port);
	void *user;
	ux_u16 pc;
	ux_u8 fault;
} Uxn;

/* uxn_eval return values. On every return u->pc is set:
 *   UXN_BRK   : BRK executed; u->pc is the address after the BRK byte.
 *   UXN_LIMIT : step budget exhausted; u->pc is the next instruction to run.
 *   UXN_FAULT : u->fault was nonzero after a dei/deo callback; u->pc is the
 *               next instruction to run (u->fault is left set). */
#define UXN_BRK 0
#define UXN_LIMIT 1
#define UXN_FAULT 2

/* Runs from pc. max_steps is the maximum number of instructions executed;
 * 0 means unlimited. */
int uxn_eval(Uxn *u, ux_u16 pc, unsigned long max_steps);

/* Overridable memory/arithmetic hooks. Arguments are already masked: a is a
 * valid 16-bit address and v is 0..255. */
#ifndef UXN_PEEK
#define UXN_PEEK(u, a) ((u)->ram[(a)])
#endif
#ifndef UXN_POKE
#define UXN_POKE(u, a, v) ((u)->ram[(a)] = (ux_u8)(v))
#endif
#ifndef UXN_DIV
#define UXN_DIV(a, b) ((b) ? (a) / (b) : 0)
#endif

#endif
