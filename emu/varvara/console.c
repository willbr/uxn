/* Console device (0x10-0x1f). */
#include <stddef.h>
#include "varvara.h"

ux_u8 console_dei(Varvara *v, ux_u8 port)
{
	return v->u.dev[port];
}

void console_deo(Varvara *v, ux_u8 port)
{
	if (!v->on_console) return;
	if (port == 0x18) v->on_console(v->ctx, 1, v->u.dev[port]);
	else if (port == 0x19) v->on_console(v->ctx, 2, v->u.dev[port]);
}

void varvara_console_input(Varvara *v, int c, int type)
{
	unsigned vec = (unsigned)v->u.dev[0x10] << 8 | v->u.dev[0x11];

	v->u.dev[0x12] = (ux_u8)c;
	v->u.dev[0x17] = (ux_u8)type;
	v->limit_hit = 0;
	if (vec) {
		v->limit_hit = varvara_run(v, (ux_u16)vec, v->max_steps) == UXN_LIMIT;
		v->u.fault = 0;
	}
}

const char *console_port_name(ux_u8 port)
{
	switch (port) {
	case 0x10: case 0x11: return "Console/vector";
	case 0x12: return "Console/read";
	case 0x17: return "Console/type";
	case 0x18: return "Console/write";
	case 0x19: return "Console/error";
	}
	return NULL;
}
