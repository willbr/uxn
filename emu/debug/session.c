#include "session.h"

void dbg_toggle_breakpoint(DebugSession *s, ux_u16 addr)
{
	ux_u8 bit = (ux_u8)(1 << (addr & 7));

	if (s->bp[addr >> 3] & bit) {
		s->bp[addr >> 3] &= (ux_u8)~bit;
		s->nbp--;
	} else {
		s->bp[addr >> 3] |= bit;
		s->nbp++;
	}
}

int dbg_has_breakpoint(const DebugSession *s, ux_u16 addr)
{
	return (s->bp[addr >> 3] >> (addr & 7)) & 1;
}

int dbg_step(DebugSession *s, Varvara *v)
{
	s->paused = 1;
	return uxn_eval(&v->u, v->u.pc, 1);
}

int dbg_step_over(DebugSession *s, Varvara *v)
{
	Uxn *u = &v->u;
	ux_u8 op = u->ram[u->pc];
	UxStack *st;
	ux_u8 ptr;
	ux_u16 ret;
	unsigned long n;
	int r = UXN_LIMIT;

	if (op != 0x60 && (op & 0x1f) != 0x0e) return dbg_step(s, v);
	/* The call pushes its return address on the return stack, or on the
	 * working stack for JSRr; it is back to its old height when the callee
	 * returns. */
	st = (op != 0x60 && (op & 0x40)) ? &u->wst : &u->rst;
	ptr = st->ptr;
	ret = (ux_u16)(u->pc + (op == 0x60 ? 3 : 1));
	s->paused = 1;
	r = uxn_eval(u, u->pc, 1);
	for (n = 1; r == UXN_LIMIT; n++) {
		if (u->pc == ret && st->ptr == ptr) return UXN_LIMIT;
		if (n >= DBG_STEP_OVER_LIMIT) return UXN_LIMIT;
		if (dbg_has_breakpoint(s, u->pc)) return UXN_LIMIT;
		r = uxn_eval(u, u->pc, 1);
	}
	return r;
}

int dbg_continue(DebugSession *s, Varvara *v, unsigned long max_steps)
{
	Uxn *u = &v->u;
	unsigned long n = 0;
	int r;

	for (;;) {
		r = uxn_eval(u, u->pc, 1);
		if (r != UXN_LIMIT) return r;
		n++;
		if (dbg_has_breakpoint(s, u->pc)) {
			s->paused = 1;
			return UXN_LIMIT;
		}
		if (max_steps && n >= max_steps) return UXN_LIMIT;
	}
}

int dbg_run(DebugSession *s, Varvara *v, ux_u16 pc, unsigned long max_steps)
{
	v->u.pc = pc;
	if (dbg_has_breakpoint(s, pc)) {
		s->paused = 1;
		return UXN_LIMIT;
	}
	return dbg_continue(s, v, max_steps);
}
