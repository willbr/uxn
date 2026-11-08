/* System device (0x00-0x0f). Only halt (state, 0x0f) is implemented so far. */
#include <stddef.h>
#include "varvara.h"

ux_u8 system_dei(Varvara *v, ux_u8 port)
{
	return v->u.dev[port];
}

void system_deo(Varvara *v, ux_u8 port)
{
	/* The core already stored the byte in dev[]. Like uxnmin, halting does
	 * not abort the vector that wrote it; the host checks varvara_halted. */
	(void)v;
	(void)port;
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
