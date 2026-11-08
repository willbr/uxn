/* Screen device (0x20-0x2f), following uxn11.c: pixel and sprite ports,
 * the blend table, flips, 1bpp/2bpp, two layers, auto and resize.
 *
 * uxn11 keeps both layers in one buffer with an 8-pixel margin around the
 * screen; here fg and bg are separate width*height buffers and every write
 * is clipped to the screen. The margin is never shown, so the visible
 * result is the same. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "varvara.h"

#define SCREEN_MAX 1024 /* Review Focus 4: sizes are kept in 1..SCREEN_MAX */

/* uxn11.c blend_lut[mode][0] (the bg column; the fg column holds the same
 * colours shifted into the high bits of its shared buffer). */
static const ux_u8 blend[16][4] = {
	{ 0, 0, 1, 2 }, { 0, 1, 2, 3 }, { 0, 2, 3, 1 }, { 0, 3, 1, 2 },
	{ 1, 0, 1, 2 }, { 1, 1, 2, 3 }, { 1, 2, 3, 1 }, { 1, 3, 1, 2 },
	{ 2, 0, 1, 2 }, { 2, 1, 2, 3 }, { 2, 2, 3, 1 }, { 2, 3, 1, 2 },
	{ 3, 0, 1, 2 }, { 3, 1, 2, 3 }, { 3, 2, 3, 1 }, { 3, 3, 1, 2 }
};

static unsigned peek2(const ux_u8 *d)
{
	return (unsigned)d[0] << 8 | d[1];
}

static void poke2(ux_u8 *d, unsigned v)
{
	d[0] = (ux_u8)(v >> 8);
	d[1] = (ux_u8)v;
}

static int clamp_size(int n)
{
	return n < 1 ? 1 : n > SCREEN_MAX ? SCREEN_MAX : n;
}

/* Like uxn11's screen_resize: a new size gets fresh, cleared layers; the
 * same size keeps them. If allocation fails the old screen stays. Either
 * way Screen/width and Screen/height read back the size in effect. */
static void resize(Varvara *v, int w, int h)
{
	w = clamp_size(w);
	h = clamp_size(h);
	if (w != v->width || h != v->height || !v->fg) {
		size_t n = (size_t)w * (size_t)h;
		ux_u8 *fg = (ux_u8 *)calloc(n, 1);
		ux_u8 *bg = (ux_u8 *)calloc(n, 1);
		if (fg && bg) {
			free(v->fg);
			free(v->bg);
			v->fg = fg;
			v->bg = bg;
			v->width = w;
			v->height = h;
		} else {
			free(fg);
			free(bg);
			fprintf(stderr, "Screen out of memory.\n");
		}
	}
	poke2(&v->u.dev[0x22], (unsigned)v->width);
	poke2(&v->u.dev[0x24], (unsigned)v->height);
}

void screen_init(Varvara *v)
{
	resize(v, 64 * 8, 40 * 8); /* uxn11.c WIDTH x HEIGHT */
}

void screen_free(Varvara *v)
{
	free(v->fg);
	free(v->bg);
	v->fg = v->bg = NULL;
	v->width = v->height = 0;
}

static void draw_pixel(Varvara *v)
{
	const int ctrl = v->u.dev[0x2e];
	ux_u8 *layer = (ctrl & 0x40) ? v->fg : v->bg;
	const ux_u8 color = (ux_u8)(ctrl & 0x3);
	const int w = v->width, h = v->height;
	const int x = v->scr_x < 0 ? 0 : v->scr_x;
	const int y = v->scr_y < 0 ? 0 : v->scr_y;

	if (!layer || x >= w || y >= h) return;
	if (ctrl & 0x80) {
		/* fill from (x,y) towards the bottom-right, or with the flip bits
		 * from the left/top edge up to (but not including) x/y */
		const int x1 = (ctrl & 0x10) ? 0 : x, x2 = (ctrl & 0x10) ? x : w;
		const int y1 = (ctrl & 0x20) ? 0 : y, y2 = (ctrl & 0x20) ? y : h;
		int row;
		for (row = y1; row < y2; row++)
			memset(layer + (size_t)row * (size_t)w + (size_t)x1, color, (size_t)(x2 - x1));
	} else {
		layer[(size_t)y * (size_t)w + (size_t)x] = color;
		if (v->scr_auto_x) v->scr_x += (ctrl & 0x10) ? -1 : 1;
		if (v->scr_auto_y) v->scr_y += (ctrl & 0x20) ? -1 : 1;
	}
}

static void draw_sprite(Varvara *v)
{
	const int ctrl = v->u.dev[0x2f];
	const int flipx = ctrl & 0x10, flipy = ctrl & 0x20;
	/* repeats (Screen/auto length) step along the other axis */
	const int dx = flipx ? -v->scr_auto_y : v->scr_auto_y;
	const int dy = flipy ? -v->scr_auto_x : v->scr_auto_x;
	ux_u8 *layer = (ctrl & 0x40) ? v->fg : v->bg;
	const int is_2bpp = ctrl >> 7;
	const unsigned addr_step = (unsigned)v->scr_auto_a << is_2bpp;
	const ux_u8 *table = blend[ctrl & 0xf];
	const int opaque = (ctrl & 0xf) % 5; /* modes 0, 5, a, f: colour 0 is transparent */
	const int w = v->width, h = v->height;
	int i, x = v->scr_x, y = v->scr_y;
	unsigned addr = v->scr_addr;

	for (i = 0; i <= v->scr_auto_len; i++, x += dx, y += dy, addr = (addr + addr_step) & 0xffff) {
		int j;
		if (!layer || x <= -8 || y <= -8 || x >= w || y >= h) continue;
		for (j = 0; j < 8; j++) {
			const int py = y + j;
			/* sprite data wraps at the end of the 64K page */
			const unsigned a = (addr + (unsigned)(flipy ? 7 - j : j)) & 0xffff;
			const int ch1 = v->ram[a];
			const int ch2 = is_2bpp ? v->ram[(a + 8) & 0xffff] << 1 : 0;
			ux_u8 *row;
			int k;
			if (py < 0 || py >= h) continue;
			row = layer + (size_t)py * (size_t)w;
			for (k = 0; k < 8; k++) {
				const int px = x + k;
				const int bit = flipx ? k : 7 - k;
				const int color = ((ch1 >> bit) & 1) | ((ch2 >> bit) & 2);
				if (px < 0 || px >= w) continue;
				if (opaque || color) row[px] = table[color];
			}
		}
	}
	v->scr_addr = addr;
	if (v->scr_auto_x) v->scr_x += flipx ? -v->scr_auto_x : v->scr_auto_x;
	if (v->scr_auto_y) v->scr_y += flipy ? -v->scr_auto_y : v->scr_auto_y;
}

static int signed16(unsigned v)
{
	return (v & 0x8000) ? (int)v - 0x10000 : (int)v;
}

ux_u8 screen_dei(Varvara *v, ux_u8 port)
{
	/* x, y and addr are registers: the high port fills in the low byte,
	 * which DEI2 then reads raw (as uxn11 does). */
	switch (port) {
	case 0x28: v->u.dev[0x29] = (ux_u8)v->scr_x; return (ux_u8)((unsigned)v->scr_x >> 8);
	case 0x2a: v->u.dev[0x2b] = (ux_u8)v->scr_y; return (ux_u8)((unsigned)v->scr_y >> 8);
	case 0x2c: v->u.dev[0x2d] = (ux_u8)v->scr_addr; return (ux_u8)(v->scr_addr >> 8);
	}
	return v->u.dev[port];
}

void screen_deo(Varvara *v, ux_u8 port)
{
	ux_u8 *d = v->u.dev;

	/* Short ports arrive on their low byte. The vector is read from dev[]
	 * by varvara_frame. */
	switch (port) {
	case 0x23: resize(v, (int)(peek2(&d[0x22]) & 0xfff), v->height); break;
	case 0x25: resize(v, v->width, (int)(peek2(&d[0x24]) & 0xfff)); break;
	case 0x26:
		v->scr_auto_x = (d[0x26] & 0x1) ? 8 : 0;
		v->scr_auto_y = (d[0x26] & 0x2) ? 8 : 0;
		v->scr_auto_a = (d[0x26] & 0x4) ? 8 : 0;
		v->scr_auto_len = d[0x26] >> 4;
		break;
	case 0x29: v->scr_x = signed16(peek2(&d[0x28])); break;
	case 0x2b: v->scr_y = signed16(peek2(&d[0x2a])); break;
	case 0x2d: v->scr_addr = peek2(&d[0x2c]); break;
	case 0x2e: draw_pixel(v); break;
	case 0x2f: draw_sprite(v); break;
	}
}

int varvara_frame(Varvara *v, unsigned long max_steps)
{
	unsigned vec = peek2(&v->u.dev[0x20]);
	int r;

	if (!vec) return 0;
	r = uxn_eval(&v->u, (ux_u16)vec, max_steps);
	v->u.fault = 0;
	return r != UXN_BRK;
}

void varvara_render(const Varvara *v, ux_u8 *rgba)
{
	ux_u8 pal[4][3];
	size_t i, n;
	int c;

	if (!v->fg || !v->bg) return;
	/* System/r, g, b hold one nibble per colour: colour 0 in the high
	 * nibble of the first byte, colour 3 in the low nibble of the second
	 * (uxn11.c system_deo_colorize). */
	for (c = 0; c < 4; c++) {
		const int shift = (c & 1) ? 0 : 4;
		int k;
		for (k = 0; k < 3; k++) {
			const int nib = (v->u.dev[0x08 + 2 * k + c / 2] >> shift) & 0xf;
			pal[c][k] = (ux_u8)(nib * 0x11);
		}
	}
	n = (size_t)v->width * (size_t)v->height;
	for (i = 0; i < n; i++) {
		const ux_u8 *p = pal[v->fg[i] ? v->fg[i] & 3 : v->bg[i] & 3];
		rgba[4 * i] = p[0];
		rgba[4 * i + 1] = p[1];
		rgba[4 * i + 2] = p[2];
		rgba[4 * i + 3] = 0xff;
	}
}

const char *screen_port_name(ux_u8 port)
{
	static const char *const names[16] = {
		"Screen/vector", "Screen/vector", "Screen/width", "Screen/width",
		"Screen/height", "Screen/height", "Screen/auto", NULL,
		"Screen/x", "Screen/x", "Screen/y", "Screen/y",
		"Screen/addr", "Screen/addr", "Screen/pixel", "Screen/sprite"
	};
	return names[port & 0x0f];
}
