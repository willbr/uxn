/* Unit tests for the uxn core. C89 with libc. */
#include <stdio.h>
#include <string.h>
#include "../core/uxn.h"

static int failures = 0;
static int checks_failed = 0;

#define CHECK(cond) \
	do { if (!(cond)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); checks_failed++; } } while (0)

static Uxn uxn;
static ux_u8 ram[0x10000];
static ux_u8 deo_log[64];
static int deo_n;
static int fault_on_deo;

static ux_u8 test_dei(Uxn *u, ux_u8 port) { return u->dev[port]; }

static void test_deo(Uxn *u, ux_u8 port)
{
	if (deo_n < 64) deo_log[deo_n++] = port;
	if (fault_on_deo) u->fault = 1;
}

static void reset(void)
{
	memset(&uxn, 0, sizeof uxn);
	memset(ram, 0, sizeof ram);
	deo_n = 0;
	uxn.ram = ram;
	uxn.dei = test_dei;
	uxn.deo = test_deo;
}

static int run(const ux_u8 *code, int n, unsigned long max)
{
	reset();
	memcpy(ram + 0x100, code, (size_t)n);
	return uxn_eval(&uxn, 0x0100, max);
}

#define RUN(max) run(code, (int)sizeof code, (max))

static void wst_is(int n, const ux_u8 *want)
{
	int i;
	CHECK(uxn.wst.ptr == n);
	for (i = 0; i < n; i++) CHECK(uxn.wst.dat[i] == want[i]);
}

static void rst_is(int n, const ux_u8 *want)
{
	int i;
	CHECK(uxn.rst.ptr == n);
	for (i = 0; i < n; i++) CHECK(uxn.rst.dat[i] == want[i]);
}

static void lit_add(void)
{
	static const ux_u8 code[] = { 0x80, 0x02, 0x80, 0x03, 0x18, 0x00 };
	static const ux_u8 w[] = { 0x05 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void dup_is_06(void)
{
	static const ux_u8 code[] = { 0x80, 0x07, 0x06, 0x00 };
	static const ux_u8 w[] = { 0x07, 0x07 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(2, w);
}
static void nip_is_03(void)
{
	static const ux_u8 code[] = { 0x80, 0x01, 0x80, 0x02, 0x03, 0x00 };
	static const ux_u8 w[] = { 0x02 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void add_keep(void)
{
	static const ux_u8 code[] = { 0x80, 0x02, 0x80, 0x03, 0x98, 0x00 };
	static const ux_u8 w[] = { 0x02, 0x03, 0x05 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(3, w);
}
static void add2_wraps(void)
{
	static const ux_u8 code[] = { 0xa0, 0xff, 0xff, 0xa0, 0x00, 0x02, 0x38, 0x00 };
	static const ux_u8 w[] = { 0x00, 0x01 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(2, w);
}
static void pop_empty_wraps(void)
{
	static const ux_u8 code[] = { 0x02, 0x00 };
	CHECK(RUN(0) == UXN_BRK);
	CHECK(uxn.wst.ptr == 0xff);
}
static void div_by_zero(void)
{
	static const ux_u8 code[] = { 0x80, 0x05, 0x80, 0x00, 0x1b, 0x00 };
	static const ux_u8 w[] = { 0x00 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void sft_left(void)
{
	static const ux_u8 code[] = { 0x80, 0x01, 0x80, 0x10, 0x1f, 0x00 };
	static const ux_u8 w[] = { 0x02 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void sft_both(void)
{
	static const ux_u8 code[] = { 0x80, 0x34, 0x80, 0x33, 0x1f, 0x00 };
	static const ux_u8 w[] = { 0x30 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void gth(void)
{
	static const ux_u8 code[] = { 0x80, 0x05, 0x80, 0x03, 0x0a, 0x00 };
	static const ux_u8 w[] = { 0x01 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void equ2_one_byte(void)
{
	static const ux_u8 code[] = { 0xa0, 0x12, 0x34, 0xa0, 0x12, 0x34, 0x28, 0x00 };
	static const ux_u8 w[] = { 0x01 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void jci_taken(void)
{
	static const ux_u8 code[] = { 0x80, 0x01, 0x20, 0x00, 0x01, 0x00, 0x80, 0xaa, 0x00 };
	static const ux_u8 w[] = { 0xaa };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
}
static void jci_not_taken(void)
{
	static const ux_u8 code[] = { 0x80, 0x00, 0x20, 0x00, 0x01, 0x00, 0x80, 0xaa, 0x00 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(0, 0);
}
static void jsi_pushes_return(void)
{
	static const ux_u8 code[] = { 0x60, 0x00, 0x01, 0x00, 0x00 };
	static const ux_u8 r[] = { 0x01, 0x03 };
	CHECK(RUN(0) == UXN_BRK);
	rst_is(2, r);
}
static void stz_ldz(void)
{
	static const ux_u8 code[] = { 0x80, 0x42, 0x80, 0x10, 0x11, 0x80, 0x10, 0x10, 0x00 };
	static const ux_u8 w[] = { 0x42 };
	CHECK(RUN(0) == UXN_BRK);
	wst_is(1, w);
	CHECK(ram[0x10] == 0x42);
}
static void deo2_one_callback(void)
{
	static const ux_u8 code[] = { 0xa0, 0x12, 0x34, 0x80, 0x18, 0x37, 0x00 };
	CHECK(RUN(0) == UXN_BRK);
	CHECK(deo_n == 1);
	CHECK(deo_log[0] == 0x19);
	CHECK(uxn.dev[0x18] == 0x12);
	CHECK(uxn.dev[0x19] == 0x34);
}
static void step_limit(void)
{
	static const ux_u8 code[] = { 0x40, 0xff, 0xfd };
	CHECK(RUN(1000) == UXN_LIMIT);
	CHECK(uxn.pc == 0x0100);
}
static void host_fault(void)
{
	static const ux_u8 code[] = { 0x80, 0x01, 0x80, 0x0f, 0x17, 0x80, 0x99, 0x00 };
	int i;
	fault_on_deo = 1;
	CHECK(RUN(0) == UXN_FAULT);
	fault_on_deo = 0;
	for (i = 0; i < uxn.wst.ptr; i++) CHECK(uxn.wst.dat[i] != 0x99);
	CHECK(uxn.pc == 0x0105);
}
static void pc_wraps(void)
{
	static const ux_u8 w[] = { 0x06 };
	reset();
	ram[0xffff] = 0x01;
	uxn.wst.dat[0] = 0x05;
	uxn.wst.ptr = 1;
	CHECK(uxn_eval(&uxn, 0xffff, 1000) == UXN_BRK);
	wst_is(1, w);
}

#define T(f) do { checks_failed = 0; f(); if (checks_failed) { printf("FAIL %s\n", #f); failures++; } } while (0)

int main(void)
{
	int total = 0;
	T(lit_add); total++;
	T(dup_is_06); total++;
	T(nip_is_03); total++;
	T(add_keep); total++;
	T(add2_wraps); total++;
	T(pop_empty_wraps); total++;
	T(div_by_zero); total++;
	T(sft_left); total++;
	T(sft_both); total++;
	T(gth); total++;
	T(equ2_one_byte); total++;
	T(jci_taken); total++;
	T(jci_not_taken); total++;
	T(jsi_pushes_return); total++;
	T(stz_ldz); total++;
	T(deo2_one_callback); total++;
	T(step_limit); total++;
	T(host_fault); total++;
	T(pc_wraps); total++;
	printf("core: %d passed, %d failed\n", total - failures, failures);
	return failures ? 1 : 0;
}
