/* Debug session: breakpoints and single-stepping over a Varvara machine.
 * C89 plus libc. The functions run from v->u.pc and leave the resume address
 * there; the caller must not run anything else on the machine in between. */
#ifndef SESSION_H
#define SESSION_H

#include "varvara.h"

typedef struct DebugSession {
	int paused;
	ux_u8 bp[0x10000 / 8];
	int nbp; /* number of breakpoints set */
} DebugSession;

/* dbg_step_over gives up after this many instructions (UXN_LIMIT). */
#define DBG_STEP_OVER_LIMIT 2000000UL

void dbg_toggle_breakpoint(DebugSession *s, ux_u16 addr);
int  dbg_has_breakpoint(const DebugSession *s, ux_u16 addr);
/* All return a UXN_* code. u->fault is left as the core set it; the caller
 * clears it. dbg_step and dbg_step_over set s->paused. */
int  dbg_step(DebugSession *s, Varvara *v);       /* one instruction */
int  dbg_step_over(DebugSession *s, Varvara *v);  /* JSR/JSR2/JSI: run until return */
/* Runs from v->u.pc for up to max_steps instructions (0 = unlimited), always
 * executing the first one, and stops before an instruction at a breakpoint:
 * that returns UXN_LIMIT with s->paused = 1. Running out of steps also returns
 * UXN_LIMIT, with s->paused unchanged. */
int  dbg_continue(DebugSession *s, Varvara *v, unsigned long max_steps);

#endif
