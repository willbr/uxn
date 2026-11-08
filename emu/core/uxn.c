/* uxn CPU core: see uxn.h. The only include is the core's own header. */
#include "uxn.h"

/* Stack primitives. Stack pointers are bytes that wrap at 256 (no checks). */
static void push8(UxStack *s, unsigned long v)
{
	s->dat[s->ptr] = (ux_u8)(v & 0xff);
	s->ptr = (ux_u8)((s->ptr + 1) & 0xff);
}

static unsigned long pop8(UxStack *s)
{
	s->ptr = (ux_u8)((s->ptr - 1) & 0xff);
	return (unsigned long)(s->dat[s->ptr] & 0xff);
}

/* Shorts are big-endian on the stack: high byte deeper. */
static void push(UxStack *s, int two, unsigned long v)
{
	if (two) push8(s, (v >> 8) & 0xff);
	push8(s, v & 0xff);
}

static unsigned long pop(UxStack *s, int two)
{
	unsigned long lo = pop8(s);
	if (two) return (pop8(s) << 8) | lo;
	return lo;
}

/* Memory helpers. Addresses are always masked to 16 bits (or 8 for zero page). */
static unsigned long peek(Uxn *u, unsigned long a, int two, unsigned long mask)
{
	unsigned long v = (unsigned long)(UXN_PEEK(u, a & mask) & 0xff);
	if (two) v = (v << 8) | (unsigned long)(UXN_PEEK(u, (a + 1) & mask) & 0xff);
	return v;
}

static void poke(Uxn *u, unsigned long a, int two, unsigned long v, unsigned long mask)
{
	if (two) {
		UXN_POKE(u, a & mask, (v >> 8) & 0xff);
		UXN_POKE(u, (a + 1) & mask, v & 0xff);
	} else {
		UXN_POKE(u, a & mask, v & 0xff);
	}
}

/* 8-bit signed offset as a 16-bit two's complement value. */
static unsigned long sext8(unsigned long a)
{
	return (a & 0x80) ? (a | 0xff00) : (a & 0xff);
}

int uxn_eval(Uxn *u, ux_u16 start, unsigned long max_steps)
{
	unsigned long pc = (unsigned long)start & 0xffff;
	unsigned long left = max_steps;
	unsigned long a, b, c, m;
	unsigned int op, base;
	int two, ret, keep;
	UxStack *s, *o;
	ux_u8 k;

	for (;;) {
		if (max_steps) {
			if (left == 0) {
				u->pc = (ux_u16)pc;
				return UXN_LIMIT;
			}
			left--;
		}
		op = (unsigned int)(UXN_PEEK(u, pc) & 0xff);
		pc = (pc + 1) & 0xffff;
		base = op & 0x1f;

		if (base == 0) {
			switch (op) {
			case 0x00: /* BRK */
				u->pc = (ux_u16)pc;
				return UXN_BRK;
			case 0x20: /* JCI */
				a = pop8(&u->wst);
				b = peek(u, pc, 1, 0xffff);
				pc = (pc + 2) & 0xffff;
				if (a) pc = (pc + b) & 0xffff;
				break;
			case 0x40: /* JMI */
				b = peek(u, pc, 1, 0xffff);
				pc = (pc + 2) & 0xffff;
				pc = (pc + b) & 0xffff;
				break;
			case 0x60: /* JSI */
				b = peek(u, pc, 1, 0xffff);
				pc = (pc + 2) & 0xffff;
				push(&u->rst, 1, pc);
				pc = (pc + b) & 0xffff;
				break;
			default: /* 80 LIT, a0 LIT2, c0 LITr, e0 LIT2r */
				s = (op & 0x40) ? &u->rst : &u->wst;
				two = (op & 0x20) != 0;
				a = peek(u, pc, two, 0xffff);
				pc = (pc + (two ? 2 : 1)) & 0xffff;
				push(s, two, a);
				break;
			}
			continue;
		}

		two = (op & 0x20) != 0;
		ret = (op & 0x40) != 0;
		keep = (op & 0x80) != 0;
		s = ret ? &u->rst : &u->wst;
		o = ret ? &u->wst : &u->rst;
		k = s->ptr;
		m = two ? 0xffff : 0xff;

		switch (base) {
		case 0x01: /* INC */
			a = pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, (a + 1) & m);
			break;
		case 0x02: /* POP */
			pop(s, two);
			if (keep) s->ptr = k;
			break;
		case 0x03: /* NIP */
			a = pop(s, two);
			pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, a);
			break;
		case 0x04: /* SWP */
			a = pop(s, two);
			b = pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, a);
			push(s, two, b);
			break;
		case 0x05: /* ROT */
			a = pop(s, two);
			b = pop(s, two);
			c = pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, b);
			push(s, two, a);
			push(s, two, c);
			break;
		case 0x06: /* DUP */
			a = pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, a);
			push(s, two, a);
			break;
		case 0x07: /* OVR */
			a = pop(s, two);
			b = pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, b);
			push(s, two, a);
			push(s, two, b);
			break;
		case 0x08: /* EQU */
		case 0x09: /* NEQ */
		case 0x0a: /* GTH */
		case 0x0b: /* LTH */
			a = pop(s, two);
			b = pop(s, two);
			if (keep) s->ptr = k;
			if (base == 0x08) c = (b == a);
			else if (base == 0x09) c = (b != a);
			else if (base == 0x0a) c = (b > a);
			else c = (b < a);
			push8(s, c); /* one byte even in short mode */
			break;
		case 0x0c: /* JMP */
			a = pop(s, two);
			if (keep) s->ptr = k;
			pc = two ? a : (pc + sext8(a)) & 0xffff;
			break;
		case 0x0d: /* JCN */
			a = pop(s, two);
			b = pop8(s);
			if (keep) s->ptr = k;
			if (b) pc = two ? a : (pc + sext8(a)) & 0xffff;
			break;
		case 0x0e: /* JSR */
			a = pop(s, two);
			if (keep) s->ptr = k;
			push(o, 1, pc);
			pc = two ? a : (pc + sext8(a)) & 0xffff;
			break;
		case 0x0f: /* STH */
			a = pop(s, two);
			if (keep) s->ptr = k;
			push(o, two, a);
			break;
		case 0x10: /* LDZ */
			a = pop8(s);
			if (keep) s->ptr = k;
			push(s, two, peek(u, a, two, 0xff));
			break;
		case 0x11: /* STZ */
			a = pop8(s);
			b = pop(s, two);
			if (keep) s->ptr = k;
			poke(u, a, two, b, 0xff);
			break;
		case 0x12: /* LDR */
			a = pop8(s);
			if (keep) s->ptr = k;
			push(s, two, peek(u, (pc + sext8(a)) & 0xffff, two, 0xffff));
			break;
		case 0x13: /* STR */
			a = pop8(s);
			b = pop(s, two);
			if (keep) s->ptr = k;
			poke(u, (pc + sext8(a)) & 0xffff, two, b, 0xffff);
			break;
		case 0x14: /* LDA */
			a = pop(s, 1);
			if (keep) s->ptr = k;
			push(s, two, peek(u, a, two, 0xffff));
			break;
		case 0x15: /* STA */
			a = pop(s, 1);
			b = pop(s, two);
			if (keep) s->ptr = k;
			poke(u, a, two, b, 0xffff);
			break;
		case 0x16: /* DEI */
			a = pop8(s);
			if (keep) s->ptr = k;
			if (u->dei) b = (unsigned long)(u->dei(u, (ux_u8)a) & 0xff);
			else b = (unsigned long)(u->dev[a] & 0xff);
			if (two) b = (b << 8) | (unsigned long)(u->dev[(a + 1) & 0xff] & 0xff);
			push(s, two, b);
			if (u->fault) {
				u->pc = (ux_u16)pc;
				return UXN_FAULT;
			}
			break;
		case 0x17: /* DEO */
			a = pop8(s);
			b = pop(s, two);
			if (keep) s->ptr = k;
			if (two) {
				u->dev[a] = (ux_u8)((b >> 8) & 0xff);
				a = (a + 1) & 0xff;
			}
			u->dev[a] = (ux_u8)(b & 0xff);
			if (u->deo) u->deo(u, (ux_u8)a);
			if (u->fault) {
				u->pc = (ux_u16)pc;
				return UXN_FAULT;
			}
			break;
		case 0x18: /* ADD */
		case 0x19: /* SUB */
		case 0x1a: /* MUL */
		case 0x1b: /* DIV */
		case 0x1c: /* AND */
		case 0x1d: /* ORA */
		case 0x1e: /* EOR */
			a = pop(s, two);
			b = pop(s, two);
			if (keep) s->ptr = k;
			switch (base) {
			case 0x18: c = b + a; break;
			case 0x19: c = b - a; break;
			case 0x1a: c = b * a; break;
			case 0x1b: c = UXN_DIV(b, a); break;
			case 0x1c: c = b & a; break;
			case 0x1d: c = b | a; break;
			default: c = b ^ a; break;
			}
			push(s, two, c & m);
			break;
		default: /* 0x1f SFT */
			a = pop8(s);
			b = pop(s, two);
			if (keep) s->ptr = k;
			push(s, two, ((b >> (a & 0xf)) << (a >> 4)) & m);
			break;
		}
	}
}
