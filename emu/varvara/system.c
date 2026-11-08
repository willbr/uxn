/* System device (0x00-0x0f): expansion, stack pointers, palette, debug, halt. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "varvara.h"

/* Memory is one 64K bank, so commands naming any other bank do nothing, as
 * uxn11 does for banks beyond its own. Reads of the command block wrap at
 * the end of RAM instead of running off it. */
static unsigned rd8(const Varvara *v, unsigned a)
{
	return v->ram[a & 0xffff];
}

static unsigned rd16(const Varvara *v, unsigned a)
{
	return rd8(v, a) << 8 | rd8(v, a + 1);
}

static void expansion(Varvara *v)
{
	unsigned exp, cmd, length, bank, addr;

	exp = (unsigned)v->u.dev[2] << 8 | v->u.dev[3];
	cmd = rd8(v, exp);
	length = rd16(v, exp + 1);
	bank = rd16(v, exp + 3);
	addr = rd16(v, exp + 5);
	if (length > 0x10000 - addr) length = 0x10000 - addr;
	if (cmd == 0x0) {
		if (bank == 0) memset(v->ram + addr, (int)rd8(v, exp + 7), length);
	} else if (cmd == 1 || cmd == 2) {
		unsigned dst_bank = rd16(v, exp + 7);
		unsigned dst_addr = rd16(v, exp + 9);
		if (length > 0x10000 - dst_addr) length = 0x10000 - dst_addr;
		if (bank == 0 && dst_bank == 0)
			memmove(v->ram + dst_addr, v->ram + addr, length);
	} else {
		fprintf(stderr, "Unknown command: %02x\n", rd8(v, exp));
	}
}

static void print_stack(const char *name, const UxStack *s)
{
	unsigned i;
	ux_u8 p = s->ptr;

	fprintf(stderr, "%s%c", name, (int)p - 8 ? ' ' : '|');
	for (i = (ux_u8)(p - 8); (ux_u8)i != p; i = (ux_u8)(i + 1))
		fprintf(stderr, "%02x%c", s->dat[i], i == 0xff ? '|' : ' ');
	fprintf(stderr, "<%02x\n", p);
}

ux_u8 system_dei(Varvara *v, ux_u8 port)
{
	if (port == 0x04) return v->u.wst.ptr;
	if (port == 0x05) return v->u.rst.ptr;
	return v->u.dev[port];
}

void system_deo(Varvara *v, ux_u8 port)
{
	/* The core already stored the byte in dev[]. Like uxnmin, halting (state,
	 * 0x0f) does not abort the vector that wrote it; the host checks
	 * varvara_halted. Short ports arrive on their low byte. The palette
	 * (0x08-0x0d) is only stored; there is no screen to colorize. */
	switch (port) {
	case 0x03: expansion(v); break;
	case 0x04: v->u.wst.ptr = v->u.dev[4]; break;
	case 0x05: v->u.rst.ptr = v->u.dev[5]; break;
	case 0x0e:
		if (v->u.dev[0x0e] & 1) {
			print_stack("WST", &v->u.wst);
			print_stack("RST", &v->u.rst);
		}
		break;
	}
}

const char *system_port_name(ux_u8 port)
{
	static const char *const names[16] = {
		"System/vector", "System/vector", "System/expansion", "System/expansion",
		"System/wst", "System/rst", "System/metadata", "System/metadata",
		"System/red", "System/red", "System/green", "System/green",
		"System/blue", "System/blue", "System/debug", "System/state"
	};
	return names[port & 0x0f];
}
