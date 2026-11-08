/* Controller (0x80-0x8f) and Mouse (0x90-0x9f) devices. The ports are plain
 * storage; the host reports events through these functions, which store the
 * port values and run the device vector the way uxn11.c does. */
#include <stddef.h>
#include "varvara.h"

static void run_vector(Varvara *v, unsigned port)
{
	unsigned vec = (unsigned)v->u.dev[port] << 8 | v->u.dev[port + 1];

	if (vec) {
		uxn_eval(&v->u, (ux_u16)vec, v->max_steps);
		v->u.fault = 0;
	}
}

static void poke2(ux_u8 *d, unsigned v)
{
	d[0] = (ux_u8)(v >> 8);
	d[1] = (ux_u8)v;
}

void varvara_button(Varvara *v, ux_u8 mask, int down)
{
	if (!mask) return;
	if (down) v->u.dev[0x82] |= mask;
	else v->u.dev[0x82] &= (ux_u8)~mask;
	run_vector(v, 0x80);
}

void varvara_key(Varvara *v, ux_u8 ch)
{
	if (!ch) return;
	v->u.dev[0x83] = ch;
	run_vector(v, 0x80);
	v->u.dev[0x83] = 0;
}

void varvara_mouse_move(Varvara *v, int x, int y)
{
	poke2(&v->u.dev[0x92], (unsigned)x & 0xffff);
	poke2(&v->u.dev[0x94], (unsigned)y & 0xffff);
	run_vector(v, 0x90);
}

/* Unlike the controller, uxn11 runs the mouse vector even for an empty mask. */
void varvara_mouse_button(Varvara *v, ux_u8 mask, int down)
{
	if (down) v->u.dev[0x96] |= mask;
	else v->u.dev[0x96] &= (ux_u8)~mask;
	run_vector(v, 0x90);
}

void varvara_mouse_scroll(Varvara *v, int dx, int dy)
{
	poke2(&v->u.dev[0x9a], (unsigned)dx & 0xffff);
	poke2(&v->u.dev[0x9c], (0u - (unsigned)dy) & 0xffff);
	run_vector(v, 0x90);
	poke2(&v->u.dev[0x9a], 0);
	poke2(&v->u.dev[0x9c], 0);
}

const char *controller_port_name(ux_u8 port)
{
	switch (port & 0x0f) {
	case 0x0: case 0x1: return "Controller/vector";
	case 0x2: return "Controller/button";
	case 0x3: return "Controller/key";
	}
	return NULL;
}

const char *mouse_port_name(ux_u8 port)
{
	switch (port & 0x0f) {
	case 0x0: case 0x1: return "Mouse/vector";
	case 0x2: case 0x3: return "Mouse/x";
	case 0x4: case 0x5: return "Mouse/y";
	case 0x6: return "Mouse/state";
	case 0xa: case 0xb: return "Mouse/scrollx";
	case 0xc: case 0xd: return "Mouse/scrolly";
	}
	return NULL;
}
