/* Unit tests for the Varvara device layer. C89 plus libc.
 * usage: varvara_test SCRATCH_DIR   (an existing, empty directory) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../varvara/varvara.h"
#include "../varvara/fs.h"

static int tests_failed = 0;
static int checks_failed = 0;
static const char *scratch;
static char scratch_abs[1024];

#define CHECK(cond) \
	do { if (!(cond)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); checks_failed++; } } while (0)

static Varvara vv;

static void fresh(void)
{
	varvara_close(&vv);
	varvara_init(&vv);
}

static void path_to(char *out, const char *rel)
{
	sprintf(out, "%s/%s", scratch, rel);
}

static void put_string(Varvara *v, unsigned addr, const char *s)
{
	memcpy(v->ram + addr, s, strlen(s) + 1);
}

static void poke_port(Varvara *v, unsigned port, unsigned val)
{
	v->u.dev[port] = (ux_u8)val;
}

/* DEO2 semantics: store both bytes, then call deo for the low port. */
static void deo2(Varvara *v, unsigned port, unsigned val)
{
	v->u.dev[port] = (ux_u8)(val >> 8);
	v->u.dev[port + 1] = (ux_u8)val;
	v->u.deo(&v->u, (ux_u8)(port + 1));
}

static unsigned success(const Varvara *v, unsigned base)
{
	return (unsigned)v->u.dev[base + 2] << 8 | v->u.dev[base + 3];
}

static void f_name(Varvara *v, unsigned base, unsigned addr)
{
	deo2(v, base + 8, addr);
}

static void f_length(Varvara *v, unsigned base, unsigned len)
{
	deo2(v, base + 0xa, len);
}

static void f_read(Varvara *v, unsigned base, unsigned addr)
{
	deo2(v, base + 0xc, addr);
}

static void f_write(Varvara *v, unsigned base, unsigned addr)
{
	deo2(v, base + 0xe, addr);
}

static void make_file(const char *rel, const char *text)
{
	char p[1024];
	FILE *f;

	path_to(p, rel);
	f = fs_fopen(p, "wb");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

static int exists(const char *rel)
{
	char p[1024];
	unsigned long size;
	int is_dir;

	path_to(p, rel);
	return fs_stat(p, &size, &is_dir) == 0;
}

static int root_confines(void)
{
	char p[1024];
	char abs_out[1024];

	fs_mkdir((path_to(p, "root"), p));
	fs_mkdir((path_to(p, "rootX"), p));
	make_file("root/x.txt", "hello");

	fresh();
	path_to(p, "root");
	CHECK(varvara_set_root(&vv, p) == 0);

	/* reading inside works */
	path_to(p, "root/x.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xa0, 0x200);
	f_length(&vv, 0xa0, 16);
	f_read(&vv, 0xa0, 0x400);
	CHECK(success(&vv, 0xa0) == 5);
	CHECK(memcmp(vv.ram + 0x400, "hello", 5) == 0);

	/* a relative path escaping the root is rejected, and nothing is created */
	path_to(p, "root/../escape.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	f_length(&vv, 0xb0, 3);
	put_string(&vv, 0x300, "abc");
	f_write(&vv, 0xb0, 0x300);
	CHECK(success(&vv, 0xb0) == 0);
	CHECK(!exists("escape.txt"));

	/* a sibling whose name starts with the root's name is outside it */
	path_to(p, "rootX/e.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	f_write(&vv, 0xb0, 0x300);
	CHECK(success(&vv, 0xb0) == 0);
	CHECK(!exists("rootX/e.txt"));
	path_to(p, "root/../rootX/e.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	f_write(&vv, 0xb0, 0x300);
	CHECK(success(&vv, 0xb0) == 0);
	CHECK(!exists("rootX/e.txt"));

	/* an absolute path outside is rejected too */
	path_to(abs_out, "escape2.txt");
	put_string(&vv, 0x200, abs_out);
	f_name(&vv, 0xb0, 0x200);
	f_write(&vv, 0xb0, 0x300);
	CHECK(success(&vv, 0xb0) == 0);
	CHECK(!exists("escape2.txt"));

	/* stat and delete are confined as well */
	path_to(p, "root/../rootX");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	f_length(&vv, 0xb0, 4);
	deo2(&vv, 0xb4, 0x500);
	CHECK(success(&vv, 0xb0) == 0);
	make_file("outside.txt", "x");
	CHECK(exists("outside.txt"));
	path_to(p, "root/../outside.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	vv.u.dev[0xb6] = 1;
	vv.u.deo(&vv.u, 0xb6);
	CHECK(success(&vv, 0xb0) == 0);
	CHECK(exists("outside.txt"));

	/* control: the same delete on a file inside the root succeeds */
	make_file("root/del.txt", "x");
	path_to(p, "root/del.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	vv.u.dev[0xb6] = 1;
	vv.u.deo(&vv.u, 0xb6);
	CHECK(success(&vv, 0xb0) == 1);
	CHECK(!exists("root/del.txt"));

	/* paths that stay inside work, including ones that wander through '..' */
	path_to(p, "root/sub/../in.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
	f_length(&vv, 0xb0, 3);
	f_write(&vv, 0xb0, 0x300);
	CHECK(success(&vv, 0xb0) == 3);
	CHECK(exists("root/in.txt"));

	varvara_close(&vv);
	return checks_failed == 0;
}

static int root_must_exist(void)
{
	char p[1024];

	fresh();
	path_to(p, "no-such-dir");
	CHECK(varvara_set_root(&vv, p) != 0);
	CHECK(!vv.has_root);
	path_to(p, "root/x.txt");
	CHECK(varvara_set_root(&vv, p) != 0); /* a file is not a directory */
	return checks_failed == 0;
}

static int file_name_unterminated(void)
{
	fresh();
	memset(vv.ram + 0xf000, 'a', 0x1000); /* no NUL before the end of RAM */
	f_name(&vv, 0xa0, 0xf000);
	f_length(&vv, 0xa0, 4);
	f_read(&vv, 0xa0, 0x400);
	CHECK(success(&vv, 0xa0) == 0);
	f_write(&vv, 0xa0, 0x400);
	CHECK(success(&vv, 0xa0) == 0);
	deo2(&vv, 0xa4, 0x500);
	CHECK(success(&vv, 0xa0) == 0);
	return checks_failed == 0;
}

static int file_no_name(void)
{
	fresh();
	f_length(&vv, 0xa0, 4);
	f_read(&vv, 0xa0, 0x400);
	CHECK(success(&vv, 0xa0) == 0);
	return checks_failed == 0;
}

static int dir_cutoff(void)
{
	char p[1024];
	char name[32];
	int i;
	unsigned n;

	fs_mkdir((path_to(p, "many"), p));
	for (i = 0; i < 20; i++) {
		sprintf(name, "many/file%02d.txt", i);
		make_file(name, "x");
	}
	fresh();
	path_to(p, "many");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xa0, 0x200);
	memset(vv.ram + 0x1000, 0x7e, 0x100);
	f_length(&vv, 0xa0, 0x30);
	f_read(&vv, 0xa0, 0x1000);
	n = success(&vv, 0xa0);
	CHECK(n == 0x30);
	CHECK(vv.ram[0x1000 + 4] == '\t');
	CHECK(memcmp(vv.ram + 0x1000, "0001\tfile", 9) == 0);
	CHECK(vv.ram[0x1000 + 0x30] == 0x7e); /* nothing written past the length */
	CHECK(vv.ram[0x1000 + 0x2f] != 0x7e);
	/* a length of 0 writes nothing */
	fresh();
	memset(vv.ram + 0x1000, 0x7e, 0x10);
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xa0, 0x200);
	f_length(&vv, 0xa0, 0);
	f_read(&vv, 0xa0, 0x1000);
	CHECK(success(&vv, 0xa0) == 0);
	CHECK(vv.ram[0x1000] == 0x7e);
	varvara_close(&vv);
	return checks_failed == 0;
}

static int read_clamps_to_ram(void)
{
	fresh();
	make_file("clamp.txt", "0123456789");
	{
		char p[1024];
		path_to(p, "clamp.txt");
		put_string(&vv, 0x200, p);
	}
	f_name(&vv, 0xa0, 0x200);
	f_length(&vv, 0xa0, 10);
	f_read(&vv, 0xa0, 0xfffc);
	CHECK(success(&vv, 0xa0) == 4);
	CHECK(memcmp(vv.ram + 0xfffc, "0123", 4) == 0);
	varvara_close(&vv);
	return checks_failed == 0;
}

static int system_expansion(void)
{
	fresh();
	/* fill 4 bytes at 0x1000 with 0x55, command block at the very end of RAM */
	vv.ram[0xfff0] = 0;
	vv.ram[0xfff1] = 0;
	vv.ram[0xfff2] = 4;
	vv.ram[0xfff3] = 0;
	vv.ram[0xfff4] = 0;
	vv.ram[0xfff5] = 0x10;
	vv.ram[0xfff6] = 0;
	vv.ram[0xfff7] = 0x55;
	deo2(&vv, 0x02, 0xfff0);
	CHECK(vv.ram[0x1000] == 0x55 && vv.ram[0x1003] == 0x55 && vv.ram[0x1004] == 0);
	/* a command block whose fields run past the end of RAM must not crash */
	vv.ram[0xffff] = 0;
	deo2(&vv, 0x02, 0xffff);
	return checks_failed == 0;
}

static void exp_cmd(Varvara *v, unsigned at, unsigned cmd, unsigned len,
	unsigned bank, unsigned addr, unsigned a, unsigned b)
{
	/* fill: a = value. copy: a = destination bank, b = destination address */
	ux_u8 *m = v->ram + at;
	m[0] = (ux_u8)cmd; m[1] = (ux_u8)(len >> 8); m[2] = (ux_u8)len;
	m[3] = (ux_u8)(bank >> 8); m[4] = (ux_u8)bank;
	m[5] = (ux_u8)(addr >> 8); m[6] = (ux_u8)addr;
	if (cmd == 0) {
		m[7] = (ux_u8)a;
	} else {
		m[7] = (ux_u8)(a >> 8); m[8] = (ux_u8)a;
		m[9] = (ux_u8)(b >> 8); m[10] = (ux_u8)b;
	}
	deo2(v, 0x02, at);
}

static int system_banks(void)
{
	fresh();
	memcpy(vv.ram + 0x1000, "ABCD", 4);
	/* copy bank 0 -> bank 1, fill bank 2, then copy both back */
	exp_cmd(&vv, 0x2000, 1, 4, 0, 0x1000, 1, 0x0020);
	exp_cmd(&vv, 0x2000, 0, 3, 2, 0x0010, 0x77, 0);
	exp_cmd(&vv, 0x2000, 1, 4, 1, 0x0020, 0, 0x3000);
	exp_cmd(&vv, 0x2000, 2, 3, 2, 0x0010, 0, 0x3100);
	CHECK(memcmp(vv.ram + 0x3000, "ABCD", 4) == 0);
	CHECK(memcmp(vv.ram + 0x3100, "www", 3) == 0);
	CHECK(vv.ram[0x3103] == 0);
	/* bank 0 itself is unchanged by writes to other banks */
	CHECK(vv.ram[0x1000] == 'A' && vv.ram[0x0010] == 0);
	/* copy within one bank, overlapping */
	exp_cmd(&vv, 0x2000, 1, 4, 1, 0x0020, 1, 0x0022);
	exp_cmd(&vv, 0x2000, 1, 4, 1, 0x0020, 0, 0x3200);
	CHECK(memcmp(vv.ram + 0x3200, "ABAB", 4) == 0);
	/* banks beyond 15 do nothing */
	exp_cmd(&vv, 0x2000, 1, 4, 0, 0x1000, 16, 0x0000);
	exp_cmd(&vv, 0x2000, 0, 4, 16, 0x1000, 0x55, 0);
	CHECK(vv.ram[0x1000] == 'A' && vv.ram[0x0000] == 0);
	/* a copy that runs off the end of a bank is clamped */
	exp_cmd(&vv, 0x2000, 1, 0x20, 0, 0x1000, 1, 0xfff8);
	exp_cmd(&vv, 0x2000, 1, 8, 1, 0xfff8, 0, 0x3300);
	CHECK(memcmp(vv.ram + 0x3300, "ABCD", 4) == 0);
	varvara_close(&vv);
	return checks_failed == 0;
}

static int system_stack_pointers(void)
{
	fresh();
	vv.u.wst.ptr = 7;
	vv.u.rst.ptr = 9;
	CHECK(vv.u.dei(&vv.u, 0x04) == 7);
	CHECK(vv.u.dei(&vv.u, 0x05) == 9);
	poke_port(&vv, 0x04, 3);
	vv.u.deo(&vv.u, 0x04);
	poke_port(&vv, 0x05, 4);
	vv.u.deo(&vv.u, 0x05);
	CHECK(vv.u.wst.ptr == 3 && vv.u.rst.ptr == 4);
	return checks_failed == 0;
}

static int datetime_ports(void)
{
	unsigned year;

	fresh();
	year = (unsigned)vv.u.dei(&vv.u, 0xc0) << 8;
	year |= vv.u.dev[0xc1];
	CHECK(year >= 2025 && year < 2200);
	CHECK(vv.u.dei(&vv.u, 0xc2) < 12);
	CHECK(vv.u.dei(&vv.u, 0xc3) >= 1 && vv.u.dei(&vv.u, 0xc3) <= 31);
	CHECK(vv.u.dei(&vv.u, 0xc4) < 24);
	CHECK(vv.u.dei(&vv.u, 0xc7) < 7);
	return checks_failed == 0;
}

/* Byte DEO: store, then call deo for that port. */
static void deo1(Varvara *v, unsigned port, unsigned val)
{
	v->u.dev[port] = (ux_u8)val;
	v->u.deo(&v->u, (ux_u8)port);
}

/* DEI2 semantics: dei on the high port, then the low byte raw. */
static unsigned dei2(Varvara *v, unsigned port)
{
	unsigned hi = v->u.dei(&v->u, (ux_u8)port);
	return hi << 8 | v->u.dev[port + 1];
}

/* Copies a hand-assembled program or data to addr. */
static void put_prog(Varvara *v, unsigned addr, const ux_u8 *code, size_t n)
{
	memcpy(v->ram + addr, code, n);
}

static void screen_at(Varvara *v, int x, int y)
{
	deo2(v, 0x28, (unsigned)x & 0xffff);
	deo2(v, 0x2a, (unsigned)y & 0xffff);
}

static int screen_default_size(void)
{
	fresh();
	CHECK(vv.width == 512 && vv.height == 320); /* uxn11.c WIDTH/HEIGHT, 64x40 tiles */
	CHECK(vv.fg != NULL && vv.bg != NULL);
	CHECK(dei2(&vv, 0x22) == 512);
	CHECK(dei2(&vv, 0x24) == 320);
	return checks_failed == 0;
}

/* 2bpp tile: colour 0 at (0,0), 1 at (7,0), 2 at (0,7), 3 at (7,7). */
static const ux_u8 blend_tile[16] = {
	0x01, 0, 0, 0, 0, 0, 0, 0x01, /* plane 1 (low bit) */
	0, 0, 0, 0, 0, 0, 0, 0x81     /* plane 2 (high bit) */
};

static int sprite_blend_table(void)
{
	/* The layer is pre-filled with a colour that differs from what the mode
	 * would write for colour 0, so a transparent pixel shows as such. */
	static const ux_u8 prefill[16] = { 1, 1, 1, 1, 0, 0, 0, 0, 3, 3, 3, 3, 2, 2, 2, 2 };
	/* Expected layer values at (0,0) (7,0) (0,7) (7,7), i.e. for tile
	 * colours 0, 1, 2, 3, for blend modes 0x0-0xf. Worked out by hand from
	 * blend_lut[mode][0] in vendor/ref/uxn11/src/uxn11.c lines 359-375 (the
	 * fg column, [mode][1], is the same values shifted into the high bits).
	 * Modes 0x0, 0x5, 0xa and 0xf leave colour 0 transparent (opaque_mask =
	 * blend % 5, uxn11.c line 525, tested at line 540), so (0,0) keeps the
	 * prefill in those rows. */
	static const ux_u8 expect[16][4] = {
		{ 1, 0, 1, 2 }, /* 0: transparent, prefill 1 */
		{ 0, 1, 2, 3 },
		{ 0, 2, 3, 1 },
		{ 0, 3, 1, 2 },
		{ 1, 0, 1, 2 },
		{ 0, 1, 2, 3 }, /* 5: transparent, prefill 0 */
		{ 1, 2, 3, 1 },
		{ 1, 3, 1, 2 },
		{ 2, 0, 1, 2 },
		{ 2, 1, 2, 3 },
		{ 3, 2, 3, 1 }, /* a: transparent, prefill 3 */
		{ 2, 3, 1, 2 },
		{ 3, 0, 1, 2 },
		{ 3, 1, 2, 3 },
		{ 3, 2, 3, 1 },
		{ 2, 3, 1, 2 }  /* f: transparent, prefill 2 */
	};
	int mode, layer;

	for (layer = 0; layer < 2; layer++) {
		for (mode = 0; mode < 16; mode++) {
			const ux_u8 *l, *other;
			int w;
			fresh();
			w = vv.width;
			l = layer ? vv.fg : vv.bg;
			other = layer ? vv.bg : vv.fg;
			put_prog(&vv, 0x1000, blend_tile, sizeof blend_tile);
			screen_at(&vv, 0, 0);
			deo1(&vv, 0x2e, 0x80u | (layer ? 0x40u : 0u) | prefill[mode]); /* fill the layer */
			deo2(&vv, 0x2c, 0x1000);
			deo1(&vv, 0x2f, 0x80u | (layer ? 0x40u : 0u) | (unsigned)mode);
			if (l[0] != expect[mode][0] || l[7] != expect[mode][1]
			    || l[7 * w] != expect[mode][2] || l[7 * w + 7] != expect[mode][3]) {
				printf("blend %s mode %x: got %d %d %d %d, wanted %d %d %d %d\n",
					layer ? "fg" : "bg", mode, l[0], l[7], l[7 * w], l[7 * w + 7],
					expect[mode][0], expect[mode][1], expect[mode][2], expect[mode][3]);
				checks_failed++;
			}
			CHECK(l[8] == prefill[mode] && l[8 * w] == prefill[mode]); /* only 8x8 touched */
			CHECK(other[0] == 0 && other[7 * w + 7] == 0); /* the other layer untouched */
		}
	}
	return checks_failed == 0;
}

static int sprite_flips(void)
{
	static const ux_u8 corner[8] = { 0x80, 0, 0, 0, 0, 0, 0, 0 }; /* 1bpp, top-left pixel */
	static const int want[4][2] = { { 8, 8 }, { 15, 8 }, { 8, 15 }, { 15, 15 } };
	int f, x, y, w;

	for (f = 0; f < 4; f++) {
		int lit = 0;
		fresh();
		w = vv.width;
		put_prog(&vv, 0x1000, corner, 8);
		screen_at(&vv, 8, 8);
		deo2(&vv, 0x2c, 0x1000);
		deo1(&vv, 0x2f, (unsigned)(f << 4) | 0x01); /* 1bpp, bg, blend 1, flip bits */
		for (y = 0; y < 24; y++)
			for (x = 0; x < 24; x++) lit += vv.bg[y * w + x] != 0;
		CHECK(lit == 1);
		CHECK(vv.bg[want[f][1] * w + want[f][0]] == 1);
	}
	return checks_failed == 0;
}

static int sprite_auto_repeat(void)
{
	static const ux_u8 tiles[16] = {
		0x80, 0, 0, 0, 0, 0, 0, 0, /* 0x1000: top-left */
		0x01, 0, 0, 0, 0, 0, 0, 0  /* 0x1008: top-right */
	};
	int w;

	fresh();
	w = vv.width;
	put_prog(&vv, 0x1000, tiles, sizeof tiles);
	screen_at(&vv, 0, 0);
	deo2(&vv, 0x2c, 0x1000);
	deo1(&vv, 0x26, 0x15); /* length 1 (two sprites), auto x, auto addr */
	deo1(&vv, 0x2f, 0x01);
	/* with auto x, the repeats go down (uxn11.c line 516: dy = rDX) */
	CHECK(vv.bg[0] == 1);
	CHECK(vv.bg[8 * w + 7] == 1);
	CHECK(vv.bg[8] == 0);
	/* after the write x moved by 8 and addr by two 1bpp tiles */
	CHECK(dei2(&vv, 0x28) == 8);
	CHECK(dei2(&vv, 0x2a) == 0);
	CHECK(dei2(&vv, 0x2c) == 0x1010);
	/* a negative x reads back as a 16-bit value */
	screen_at(&vv, -3, 0);
	CHECK(dei2(&vv, 0x28) == 0xfffd);
	/* 2bpp tiles step the address by 16, and the address wraps at 64K */
	deo2(&vv, 0x2c, 0xfff8);
	deo1(&vv, 0x2f, 0x81);
	CHECK(dei2(&vv, 0x2c) == 0x0018);
	return checks_failed == 0;
}

static int pixel_modes(void)
{
	int w, h;

	fresh();
	w = vv.width;
	h = vv.height;
	/* single pixels: auto x moves right */
	deo1(&vv, 0x26, 0x01);
	screen_at(&vv, 0, 1);
	deo1(&vv, 0x2e, 0x01);
	deo1(&vv, 0x2e, 0x42);
	CHECK(vv.bg[w] == 1 && vv.fg[w + 1] == 2 && vv.fg[w] == 0);
	CHECK(dei2(&vv, 0x28) == 2);
	/* a negative x draws at 0 (uxn11.c line 479) */
	screen_at(&vv, -5, 3);
	deo1(&vv, 0x2e, 0x03);
	CHECK(vv.bg[3 * w] == 3);
	/* an off-screen pixel draws nothing and does not move */
	screen_at(&vv, w, 0);
	deo1(&vv, 0x2e, 0x03);
	CHECK(dei2(&vv, 0x28) == (unsigned)w);
	deo1(&vv, 0x26, 0x00);
	/* fill towards bottom-right covers x..w-1, y..h-1 */
	screen_at(&vv, w - 2, h - 1);
	deo1(&vv, 0x2e, 0x82);
	CHECK(vv.bg[(h - 1) * w + w - 1] == 2 && vv.bg[(h - 1) * w + w - 2] == 2);
	CHECK(vv.bg[(h - 1) * w + w - 3] == 0 && vv.bg[(h - 2) * w + w - 1] == 0);
	/* fill flipped in x and y covers 0..x-1, 0..y-1 (uxn11.c lines 485-490) */
	fresh();
	screen_at(&vv, 3, 2);
	deo1(&vv, 0x2e, 0xb1);
	CHECK(vv.bg[0] == 1 && vv.bg[1 * w + 2] == 1);
	CHECK(vv.bg[2 * w + 2] == 0 && vv.bg[1 * w + 3] == 0);
	return checks_failed == 0;
}

static int screen_render_composite(void)
{
	ux_u8 rgba[16];
	static const ux_u8 want[16] = {
		0x22, 0x66, 0xaa, 0xff, /* bg 1, fg 0: bg shows */
		0x33, 0x77, 0xbb, 0xff, /* bg 1, fg 2: fg wins */
		0x11, 0x55, 0x99, 0xff, /* both 0: colour 0 */
		0x44, 0x88, 0xcc, 0xff  /* fg 3 */
	};

	fresh();
	deo2(&vv, 0x08, 0x1234);
	deo2(&vv, 0x0a, 0x5678);
	deo2(&vv, 0x0c, 0x9abc);
	deo2(&vv, 0x22, 4);
	deo2(&vv, 0x24, 1);
	CHECK(vv.width == 4 && vv.height == 1);
	if (vv.width != 4 || vv.height != 1) return 0;
	vv.bg[0] = 1;
	vv.bg[1] = 1; vv.fg[1] = 2;
	vv.fg[3] = 3;
	varvara_render(&vv, rgba);
	CHECK(memcmp(rgba, want, 16) == 0);
	return checks_failed == 0;
}

/* Review Focus 4: sizes stay in 1..1024, and drawing and rendering at the
 * clamped edges stay inside the buffers. */
static int screen_resize_clamped(void)
{
	static const ux_u8 to_zero[] = {
		0xa0, 0x00, 0x00, 0x80, 0x22, 0x37, /* #0000 .Screen/width DEO2 */
		0xa0, 0x00, 0x00, 0x80, 0x24, 0x37, /* #0000 .Screen/height DEO2 */
		0x00
	};
	static const ux_u8 to_max[] = {
		0xa0, 0xff, 0xff, 0x80, 0x22, 0x37, /* #ffff .Screen/width DEO2 */
		0xa0, 0xff, 0xff, 0x80, 0x24, 0x37, /* #ffff .Screen/height DEO2 */
		0x80, 0x22, 0x36, 0xa0, 0x30, 0x00, 0x35, /* .Screen/width DEI2 ;3000 STA2 */
		0x80, 0x24, 0x36, 0xa0, 0x30, 0x02, 0x35, /* .Screen/height DEI2 ;3002 STA2 */
		0x00
	};
	static const ux_u8 solid[16] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	ux_u8 *rgba;
	size_t n, i;
	int ok = 1;

	fresh();
	put_prog(&vv, 0x100, to_zero, sizeof to_zero);
	CHECK(uxn_eval(&vv.u, 0x100, 1000) == UXN_BRK);
	CHECK(vv.width == 1 && vv.height == 1);
	CHECK(dei2(&vv, 0x22) == 1 && dei2(&vv, 0x24) == 1);
	put_prog(&vv, 0x100, to_max, sizeof to_max);
	CHECK(uxn_eval(&vv.u, 0x100, 1000) == UXN_BRK);
	CHECK(vv.width == 1024 && vv.height == 1024);
	CHECK(vv.ram[0x3000] == 0x04 && vv.ram[0x3001] == 0x00);
	CHECK(vv.ram[0x3002] == 0x04 && vv.ram[0x3003] == 0x00);
	if (vv.width != 1024 || vv.height != 1024 || !vv.fg) return 0;
	/* 0x1005 & 0xfff is 5, as uxn11 masks; 0x0500 is clamped to 1024 */
	deo2(&vv, 0x22, 0x1005);
	CHECK(vv.width == 5);
	deo2(&vv, 0x22, 0x0500);
	CHECK(vv.width == 1024);

	/* draw at and across the far edges */
	put_prog(&vv, 0x1000, solid, sizeof solid);
	deo2(&vv, 0x2c, 0x1000);
	screen_at(&vv, 1023, 1023);
	deo1(&vv, 0x2e, 0x03);
	deo1(&vv, 0x2e, 0xc2);
	screen_at(&vv, 1020, 1020);
	deo1(&vv, 0x2f, 0xc5);
	screen_at(&vv, -4, -4);
	deo1(&vv, 0x2f, 0x85);
	screen_at(&vv, 1023, 1023);
	deo1(&vv, 0x26, 0xf3); /* 16 sprites, auto x and y: runs off the edge */
	deo1(&vv, 0x2f, 0x85);
	CHECK(vv.fg[1023 * 1024 + 1023] == 3);
	CHECK(vv.bg[0] == 3 && vv.bg[3 * 1024 + 3] == 3 && vv.bg[4 * 1024 + 4] == 0);

	/* render into an exact-size buffer followed by guard bytes */
	n = (size_t)vv.width * (size_t)vv.height * 4;
	CHECK(n == 1024u * 1024u * 4u);
	rgba = (ux_u8 *)malloc(n + 64);
	CHECK(rgba != NULL);
	if (rgba) {
		memset(rgba + n, 0xa5, 64);
		varvara_render(&vv, rgba);
		for (i = n; i < n + 64; i++) if (rgba[i] != 0xa5) ok = 0;
		CHECK(ok);
		free(rgba);
	}

	/* a resize to a new size clears the layers; the same size keeps them */
	deo2(&vv, 0x22, 1024);
	CHECK(vv.fg[1023 * 1024 + 1023] == 3);
	deo2(&vv, 0x22, 16);
	deo2(&vv, 0x24, 16);
	CHECK(vv.width == 16 && vv.height == 16);
	CHECK(vv.fg[15 * 16 + 15] == 0 && vv.bg[0] == 0);
	varvara_close(&vv);
	CHECK(vv.fg == NULL && vv.bg == NULL);
	return checks_failed == 0;
}

static int screen_frame_vector(void)
{
	static const ux_u8 boot[] = {
		0xa0, 0x02, 0x00, 0x80, 0x20, 0x37, 0x00 /* ;0200 .Screen/vector DEO2 BRK */
	};
	static const ux_u8 count[] = {
		0x80, 0x10, 0x90, 0x01, 0x04, 0x11, 0x00 /* #10 LDZk INC SWP STZ BRK */
	};
	static const ux_u8 spin[] = { 0x40, 0xff, 0xfd }; /* JMI to itself */

	fresh();
	CHECK(varvara_frame(&vv, 1000) == 0); /* no vector: nothing runs */
	put_prog(&vv, 0x100, boot, sizeof boot);
	put_prog(&vv, 0x200, count, sizeof count);
	CHECK(uxn_eval(&vv.u, 0x100, 1000) == UXN_BRK);
	CHECK(varvara_frame(&vv, 1000) == 0);
	CHECK(varvara_frame(&vv, 1000) == 0);
	CHECK(vv.ram[0x10] == 2);
	put_prog(&vv, 0x200, spin, sizeof spin);
	CHECK(varvara_frame(&vv, 1000) != 0);
	CHECK(vv.u.fault == 0);
	return checks_failed == 0;
}

static int controller_vector_called(void)
{
	static const ux_u8 boot[] = {
		0xa0, 0x02, 0x00, 0x80, 0x80, 0x37, 0x00 /* ;0200 .Controller/vector DEO2 BRK */
	};
	static const ux_u8 vec[] = {
		0x80, 0x82, 0x16, 0xa0, 0x30, 0x00, 0x15, /* .Controller/button DEI ;3000 STA */
		0x80, 0x83, 0x16, 0xa0, 0x30, 0x01, 0x15, /* .Controller/key DEI ;3001 STA */
		0x80, 0x10, 0x90, 0x01, 0x04, 0x11,       /* count events at 0x10 */
		0x00
	};

	fresh();
	put_prog(&vv, 0x100, boot, sizeof boot);
	put_prog(&vv, 0x200, vec, sizeof vec);
	CHECK(uxn_eval(&vv.u, 0x100, 1000) == UXN_BRK);
	varvara_button(&vv, 0x01, 1);
	CHECK(vv.u.dev[0x82] == 0x01);
	CHECK(vv.ram[0x3000] == 0x01 && vv.ram[0x10] == 1);
	varvara_button(&vv, 0x10, 1);
	CHECK(vv.ram[0x3000] == 0x11 && vv.ram[0x10] == 2);
	varvara_button(&vv, 0x01, 0);
	CHECK(vv.ram[0x3000] == 0x10 && vv.ram[0x10] == 3);
	varvara_button(&vv, 0x00, 1); /* no bits: no event */
	CHECK(vv.ram[0x10] == 3);
	varvara_key(&vv, 'a');
	CHECK(vv.ram[0x3001] == 'a' && vv.ram[0x3000] == 0x10 && vv.ram[0x10] == 4);
	CHECK(vv.u.dev[0x83] == 0); /* cleared after the vector */
	varvara_key(&vv, 0); /* no event */
	CHECK(vv.ram[0x10] == 4);
	return checks_failed == 0;
}

static int mouse_vector_called(void)
{
	static const ux_u8 boot[] = {
		0xa0, 0x02, 0x00, 0x80, 0x90, 0x37, 0x00 /* ;0200 .Mouse/vector DEO2 BRK */
	};
	static const ux_u8 vec[] = {
		0x80, 0x92, 0x36, 0xa0, 0x30, 0x00, 0x35, /* .Mouse/x DEI2 ;3000 STA2 */
		0x80, 0x94, 0x36, 0xa0, 0x30, 0x02, 0x35, /* .Mouse/y DEI2 ;3002 STA2 */
		0x80, 0x96, 0x16, 0xa0, 0x30, 0x04, 0x15, /* .Mouse/state DEI ;3004 STA */
		0x80, 0x9a, 0x36, 0xa0, 0x30, 0x05, 0x35, /* .Mouse/scrollx DEI2 ;3005 STA2 */
		0x80, 0x9c, 0x36, 0xa0, 0x30, 0x07, 0x35, /* .Mouse/scrolly DEI2 ;3007 STA2 */
		0x80, 0x10, 0x90, 0x01, 0x04, 0x11,       /* count events at 0x10 */
		0x00
	};
	const ux_u8 *r = vv.ram + 0x3000;

	fresh();
	put_prog(&vv, 0x100, boot, sizeof boot);
	put_prog(&vv, 0x200, vec, sizeof vec);
	CHECK(uxn_eval(&vv.u, 0x100, 1000) == UXN_BRK);
	varvara_mouse_move(&vv, 5, 300);
	CHECK(r[0] == 0x00 && r[1] == 0x05 && r[2] == 0x01 && r[3] == 0x2c && vv.ram[0x10] == 1);
	varvara_mouse_button(&vv, 0x01, 1);
	varvara_mouse_button(&vv, 0x04, 1);
	CHECK(r[4] == 0x05);
	varvara_mouse_button(&vv, 0x01, 0);
	CHECK(r[4] == 0x04 && vv.ram[0x10] == 4);
	/* scroll: y is negated for the vector, and both are zeroed after it */
	varvara_mouse_scroll(&vv, 0, 1);
	CHECK(r[5] == 0x00 && r[6] == 0x00 && r[7] == 0xff && r[8] == 0xff);
	CHECK(vv.u.dev[0x9c] == 0 && vv.u.dev[0x9d] == 0);
	varvara_mouse_scroll(&vv, 1, -1);
	CHECK(r[5] == 0x00 && r[6] == 0x01 && r[7] == 0x00 && r[8] == 0x01);
	CHECK(vv.u.dev[0x9a] == 0 && vv.u.dev[0x9b] == 0 && vv.ram[0x10] == 6);
	CHECK(r[0] == 0x00 && r[1] == 0x05); /* position kept */
	return checks_failed == 0;
}

int main(int argc, char **argv)
{
	static const struct { const char *name; int (*fn)(void); } tests[] = {
		{ "root_confines", root_confines },
		{ "root_must_exist", root_must_exist },
		{ "file_name_unterminated", file_name_unterminated },
		{ "file_no_name", file_no_name },
		{ "dir_cutoff", dir_cutoff },
		{ "read_clamps_to_ram", read_clamps_to_ram },
		{ "system_expansion", system_expansion },
		{ "system_banks", system_banks },
		{ "system_stack_pointers", system_stack_pointers },
		{ "datetime_ports", datetime_ports },
		{ "screen_default_size", screen_default_size },
		{ "sprite_blend_table", sprite_blend_table },
		{ "sprite_flips", sprite_flips },
		{ "sprite_auto_repeat", sprite_auto_repeat },
		{ "pixel_modes", pixel_modes },
		{ "screen_render_composite", screen_render_composite },
		{ "screen_resize_clamped", screen_resize_clamped },
		{ "screen_frame_vector", screen_frame_vector },
		{ "controller_vector_called", controller_vector_called },
		{ "mouse_vector_called", mouse_vector_called }
	};
	unsigned i, total = sizeof tests / sizeof tests[0];

	if (argc < 2) {
		fprintf(stderr, "usage: varvara_test SCRATCH_DIR\n");
		return 2;
	}
	if (fs_fullpath(argv[1], scratch_abs, sizeof scratch_abs) != 0) {
		fprintf(stderr, "varvara_test: bad scratch dir\n");
		return 2;
	}
	scratch = scratch_abs;
	for (i = 0; i < total; i++) {
		checks_failed = 0;
		if (!tests[i].fn()) {
			printf("FAIL %s\n", tests[i].name);
			tests_failed++;
		}
	}
	printf("varvara: %u passed, %d failed\n", total - (unsigned)tests_failed, tests_failed);
	return tests_failed != 0;
}
