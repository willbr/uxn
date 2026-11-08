/* Unit tests for the Varvara device layer. C89 plus libc.
 * usage: varvara_test SCRATCH_DIR   (an existing, empty directory) */
#include <stdio.h>
#include <string.h>
#include "../varvara/varvara.h"
#include "../varvara/fs.h"

static int tests_failed = 0;
static int checks_failed = 0;
static const char *scratch;

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
	put_string(&vv, 0x200, "../escape.txt");
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
	put_string(&vv, 0x200, "../rootX/e.txt");
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
	put_string(&vv, 0x200, "../rootX");
	f_name(&vv, 0xb0, 0x200);
	deo2(&vv, 0xb4, 0x500);
	CHECK(success(&vv, 0xb0) == 0);
	make_file("outside.txt", "x");
	put_string(&vv, 0x200, "../outside.txt");
	f_name(&vv, 0xb0, 0x200);
	vv.u.dev[0xb6] = 1;
	vv.u.deo(&vv.u, 0xb6);
	CHECK(success(&vv, 0xb0) == 0);
	CHECK(exists("outside.txt"));

	/* paths that stay inside work, including ones that wander through '..' */
	path_to(p, "root/sub/../in.txt");
	put_string(&vv, 0x200, p);
	f_name(&vv, 0xb0, 0x200);
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
		{ "system_stack_pointers", system_stack_pointers },
		{ "datetime_ports", datetime_ports }
	};
	unsigned i, total = sizeof tests / sizeof tests[0];

	if (argc < 2) {
		fprintf(stderr, "usage: varvara_test SCRATCH_DIR\n");
		return 2;
	}
	scratch = argv[1];
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
