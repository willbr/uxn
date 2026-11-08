/* Unit tests for the disassembler and the debug session. C89 plus libc. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../varvara/varvara.h"
#include "../debug/disasm.h"
#include "../debug/session.h"

static int checks_failed = 0;
#define CHECK(c) do { if (!(c)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); checks_failed++; } } while (0)

static Varvara *vv;

static void fresh(const ux_u8 *code, int n)
{
	if (vv) { varvara_close(vv); free(vv); }
	vv = (Varvara *)malloc(sizeof *vv);
	varvara_init(vv);
	memcpy(vv->ram + 0x100, code, (size_t)n);
	vv->u.pc = 0x100;
}

static int dis(const ux_u8 *code, int n, ux_u16 at, char *out)
{
	ux_u8 ram[0x10000];
	memset(ram, 0, sizeof ram);
	memcpy(ram + at, code, (size_t)n);
	return uxn_disasm(ram, at, out, 32);
}

static void disasm_formats(void)
{
	char s[32];
	static const ux_u8 add[] = { 0x18 }, addk2[] = { 0xb8 }, lit[] = { 0x80, 0x12 };
	static const ux_u8 lit2[] = { 0xa0, 0x12, 0x34 }, jsi[] = { 0x60, 0x00, 0x01 };
	static const ux_u8 brk[] = { 0x00 }, add2kr[] = { 0xf8 }, jci[] = { 0x20, 0xff, 0xfd };
	static const ux_u8 litr[] = { 0xc0, 0xab }, lit2r[] = { 0xe0, 0x01, 0x02 };

	CHECK(dis(add, 1, 0x100, s) == 1 && strcmp(s, "ADD") == 0);
	CHECK(dis(addk2, 1, 0x100, s) == 1 && strcmp(s, "ADD2k") == 0);
	CHECK(dis(add2kr, 1, 0x100, s) == 1 && strcmp(s, "ADD2kr") == 0);
	CHECK(dis(lit, 2, 0x100, s) == 2 && strcmp(s, "LIT 12") == 0);
	CHECK(dis(lit2, 3, 0x100, s) == 3 && strcmp(s, "LIT2 1234") == 0);
	CHECK(dis(litr, 2, 0x100, s) == 2 && strcmp(s, "LITr ab") == 0);
	CHECK(dis(lit2r, 3, 0x100, s) == 3 && strcmp(s, "LIT2r 0102") == 0);
	CHECK(dis(jsi, 3, 0x100, s) == 3 && strcmp(s, "JSI 0104") == 0);
	CHECK(dis(jci, 3, 0x100, s) == 3 && strcmp(s, "JCI 0100") == 0);
	CHECK(dis(brk, 1, 0x100, s) == 1 && strcmp(s, "BRK") == 0);
	/* truncation keeps the NUL */
	{
		ux_u8 ram[0x10000];
		char t[4];
		memset(ram, 0, sizeof ram);
		ram[0x100] = 0xa0;
		CHECK(uxn_disasm(ram, 0x100, t, 4) == 3 && strcmp(t, "LIT") == 0);
	}
	/* operands wrap around the end of memory */
	{
		ux_u8 ram[0x10000];
		char t[32];
		memset(ram, 0, sizeof ram);
		ram[0xffff] = 0xa0; ram[0x0000] = 0xab; ram[0x0001] = 0xcd;
		CHECK(uxn_disasm(ram, 0xffff, t, 32) == 3 && strcmp(t, "LIT2 abcd") == 0);
	}
}

static void step_over_jsi(void)
{
	static const ux_u8 code[] = { 0x60, 0x00, 0x01, 0x00, 0x80, 0x07, 0x6c };
	DebugSession s;

	memset(&s, 0, sizeof s);
	fresh(code, sizeof code);
	CHECK(dbg_step_over(&s, vv) == UXN_LIMIT);
	CHECK(vv->u.pc == 0x0103);
	CHECK(vv->u.wst.ptr == 1 && vv->u.wst.dat[0] == 0x07);
	CHECK(vv->u.rst.ptr == 0);
	/* a non-call steps once */
	fresh(code + 4, 3);
	memset(&s, 0, sizeof s);
	CHECK(dbg_step_over(&s, vv) == UXN_LIMIT && vv->u.pc == 0x102);
}

static void step_over_jsr(void)
{
	/* LIT2 0105 JSR2 BRK . LIT 09 JMP2r : JSR2 at 0x103 calls 0x105 */
	static const ux_u8 code[] = { 0xa0, 0x01, 0x05, 0x2e, 0x00, 0x80, 0x09, 0x6c };
	DebugSession s;

	memset(&s, 0, sizeof s);
	fresh(code, sizeof code);
	CHECK(dbg_step(&s, vv) == UXN_LIMIT && vv->u.pc == 0x103);
	CHECK(s.paused == 1);
	CHECK(dbg_step_over(&s, vv) == UXN_LIMIT && vv->u.pc == 0x104);
	CHECK(vv->u.wst.ptr == 1 && vv->u.wst.dat[0] == 0x09 && vv->u.rst.ptr == 0);
	CHECK(dbg_step(&s, vv) == UXN_BRK);
}

static void step_over_runaway(void)
{
	/* callee at 0x104 loops forever: JMP back via JMI -1 */
	static const ux_u8 code[] = { 0x60, 0x00, 0x01, 0x00, 0x40, 0xff, 0xfd };
	DebugSession s;

	memset(&s, 0, sizeof s);
	fresh(code, sizeof code);
	CHECK(dbg_step_over(&s, vv) == UXN_LIMIT);
	CHECK(s.paused == 1);
	CHECK(vv->u.pc == 0x104);
}

static void breakpoint_stops(void)
{
	static const ux_u8 code[] = { 0x80, 0x01, 0x80, 0x02, 0x18, 0x00 };
	DebugSession s;

	memset(&s, 0, sizeof s);
	fresh(code, sizeof code);
	dbg_toggle_breakpoint(&s, 0x0102);
	CHECK(dbg_has_breakpoint(&s, 0x0102) && !dbg_has_breakpoint(&s, 0x0103));
	CHECK(dbg_continue(&s, vv, 1000) == UXN_LIMIT);
	CHECK(vv->u.pc == 0x0102 && s.paused == 1);
	/* continuing from a breakpoint runs past it */
	CHECK(dbg_continue(&s, vv, 1000) == UXN_BRK);
	CHECK(vv->u.wst.ptr == 1 && vv->u.wst.dat[0] == 0x03);
	/* toggle off */
	dbg_toggle_breakpoint(&s, 0x0102);
	CHECK(!dbg_has_breakpoint(&s, 0x0102) && s.nbp == 0);
	/* without a breakpoint, the step budget bounds the run and does not pause */
	fresh(code, sizeof code);
	memset(&s, 0, sizeof s);
	CHECK(dbg_continue(&s, vv, 2) == UXN_LIMIT && vv->u.pc == 0x0104 && s.paused == 0);
}

static DebugSession *hook_sess;
static int bp_hook(Varvara *v, ux_u16 pc, unsigned long max_steps)
{
	return dbg_run(hook_sess, v, pc, max_steps);
}

static void entry_breakpoint(void)
{
	static const ux_u8 code[] = { 0x80, 0x01, 0x00 };
	DebugSession s;

	memset(&s, 0, sizeof s);
	fresh(code, sizeof code);
	dbg_toggle_breakpoint(&s, 0x0100);
	CHECK(dbg_run(&s, vv, 0x0100, 1000) == UXN_LIMIT);
	CHECK(s.paused == 1 && vv->u.pc == 0x0100 && vv->u.wst.ptr == 0);
	/* continuing from it executes that instruction and proceeds */
	CHECK(dbg_continue(&s, vv, 1000) == UXN_BRK);
	CHECK(vv->u.wst.ptr == 1 && vv->u.wst.dat[0] == 0x01);
}

/* Controller vector at 0x100: LIT 83, DEI, BRK, with a breakpoint at the DEI. */
static void setup_key_vector(DebugSession *s)
{
	static const ux_u8 code[] = { 0x80, 0x83, 0x16, 0x00 };

	memset(s, 0, sizeof *s);
	fresh(code, sizeof code);
	vv->u.dev[0x80] = 0x01;
	vv->u.dev[0x81] = 0x00;
	hook_sess = s;
	vv->run = bp_hook;
	vv->max_steps = 100000;
}

static void key_clear_after_breakpoint(void)
{
	DebugSession s;

	setup_key_vector(&s);
	dbg_toggle_breakpoint(&s, 0x0102);
	varvara_key(vv, 'A');
	CHECK(vv->limit_hit == 1 && s.paused == 1 && vv->u.pc == 0x0102);
	CHECK(vv->u.dev[0x83] == 'A'); /* not cleared while the vector is suspended */
	CHECK(dbg_continue(&s, vv, 1000) == UXN_BRK);
	CHECK(vv->u.wst.ptr == 1 && vv->u.wst.dat[0] == 0x41);
	varvara_vector_done(vv);
	CHECK(vv->u.dev[0x83] == 0 && vv->pending_clear == 0);
	/* breakpoint at the vector's entry stops before any instruction */
	setup_key_vector(&s);
	dbg_toggle_breakpoint(&s, 0x0100);
	varvara_key(vv, 'B');
	CHECK(vv->limit_hit == 1 && vv->u.pc == 0x0100 && vv->u.wst.ptr == 0);
	CHECK(dbg_continue(&s, vv, 1000) == UXN_BRK && vv->u.wst.dat[0] == 0x42);
	varvara_vector_done(vv);
	CHECK(vv->u.dev[0x83] == 0);
}

static void key_clear_after_step_limit(void)
{
	DebugSession s;

	setup_key_vector(&s);
	vv->run = NULL; /* plain uxn_eval */
	vv->max_steps = 1;
	varvara_key(vv, 'A');
	CHECK(vv->limit_hit == 1 && vv->u.dev[0x83] == 'A');
	CHECK(uxn_eval(&vv->u, vv->u.pc, 100) == UXN_BRK);
	CHECK(vv->u.wst.ptr == 1 && vv->u.wst.dat[0] == 0x41);
	varvara_vector_done(vv);
	CHECK(vv->u.dev[0x83] == 0);
	/* a vector that completes clears at once */
	vv->max_steps = 100;
	varvara_key(vv, 'C');
	CHECK(vv->limit_hit == 0 && vv->u.dev[0x83] == 0);
}

static void scroll_clear_after_breakpoint(void)
{
	static const ux_u8 code[] = { 0x80, 0x9b, 0x16, 0x00 };
	DebugSession s;

	memset(&s, 0, sizeof s);
	fresh(code, sizeof code);
	vv->u.dev[0x90] = 0x01;
	hook_sess = &s;
	vv->run = bp_hook;
	vv->max_steps = 1000;
	dbg_toggle_breakpoint(&s, 0x0102);
	varvara_mouse_scroll(vv, 3, 0);
	CHECK(vv->limit_hit == 1 && vv->u.dev[0x9b] == 3);
	CHECK(dbg_continue(&s, vv, 1000) == UXN_BRK && vv->u.wst.dat[0] == 3);
	varvara_vector_done(vv);
	CHECK(vv->u.dev[0x9b] == 0);
}

int main(void)
{
	int before, total = 0, bad = 0;
#define RUN(f) do { before = checks_failed; total++; f(); if (checks_failed != before) { bad++; printf("FAIL %s\n", #f); } } while (0)
	RUN(disasm_formats);
	RUN(step_over_jsi);
	RUN(step_over_jsr);
	RUN(step_over_runaway);
	RUN(breakpoint_stops);
	RUN(entry_breakpoint);
	RUN(key_clear_after_breakpoint);
	RUN(key_clear_after_step_limit);
	RUN(scroll_clear_after_breakpoint);
	if (vv) { varvara_close(vv); free(vv); }
	printf("debug: %d passed, %d failed\n", total - bad, bad);
	return bad != 0;
}
