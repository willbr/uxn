/* Runner tests: the SDL host's resumable vector path, run headlessly.
 * usage: runner_test LEFT_ROM   (build/ref/left.rom) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runner.h"

#define STEP_LIMIT 2000000UL /* host/main.c */

static int checks_failed = 0;
#define CHECK(c) do { if (!(c)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); checks_failed++; } } while (0)

static const char *left_rom;
static Varvara vv;
static DebugSession sess;
static Runner rn;

static unsigned vec(unsigned port)
{
	return (unsigned)vv.u.dev[port] << 8 | vv.u.dev[port + 1];
}

static void fresh(unsigned long limit)
{
	varvara_close(&vv);
	varvara_init(&vv);
	memset(&sess, 0, sizeof sess);
	memset(&rn, 0, sizeof rn);
	runner_init(&rn, &vv, &sess, limit);
}

static int ram_has(const char *s)
{
	size_t n = strlen(s), i;
	for (i = 0; i + n <= 0x10000; i++)
		if (memcmp(vv.ram + i, s, n) == 0) return 1;
	return 0;
}

/* Tick until the boot is over and nothing is suspended. Returns the ticks
 * that resumed the reset vector. */
static int tick_until_idle(int max_ticks, int *ticks_out)
{
	int t, reset_resumes = 0;
	for (t = 0; t < max_ticks && (rn.busy || rn.boot != RUN_BOOT_DONE); t++) {
		if (rn.boot == RUN_BOOT_RESET && rn.busy) reset_resumes++;
		runner_tick(&rn, 0);
	}
	if (ticks_out) *ticks_out = t;
	return reset_resumes;
}

/* Finding 1: Left's reset vector takes more than STEP_LIMIT steps. Through
 * the plain one-shot boot it is cut off with no input vectors; through the
 * runner it completes over several ticks, then gets its argument. */
static int left_boot_resumes(void)
{
	static char arg0[] = "notes-runner-test.txt";
	char *args[1];
	char *argv[3];
	int resumes, ticks;

	args[0] = arg0;
	/* the one-shot boot with the host's limit stops early: the bug */
	fresh(STEP_LIMIT);
	CHECK(varvara_load(&vv, left_rom) == 0);
	argv[0] = (char *)"emu"; argv[1] = (char *)left_rom; argv[2] = arg0;
	CHECK(varvara_boot(&vv, 3, argv, STEP_LIMIT) != 0);
	CHECK(vec(0x90) == 0); /* no Mouse vector yet: Left would get no input */

	fresh(STEP_LIMIT);
	CHECK(varvara_load(&vv, left_rom) == 0);
	runner_boot(&rn, 1, args);
	CHECK(rn.boot == RUN_BOOT_RESET && rn.busy); /* cut off at the limit, suspended */
	CHECK(vv.u.dev[0x17] == 1);                  /* Console/type: has arguments */
	/* input arriving during the boot waits for the reset and the arguments */
	runner_input(&rn, IQ_MOUSE_MOVE, 10, 10);
	CHECK(rn.iq.count == 1);
	resumes = tick_until_idle(1000, &ticks);
	CHECK(resumes >= 1);
	CHECK(rn.reset_brk);
	CHECK(rn.boot == RUN_BOOT_DONE && !rn.busy);
	CHECK(vec(0x90) != 0); /* Mouse vector */
	CHECK(vec(0x10) != 0); /* Console vector */
	CHECK(vec(0x80) != 0); /* Controller vector */
	CHECK(ram_has(arg0));  /* Left took the argument as its file name */
	CHECK(rn.iq.count == 0);
	CHECK(vv.u.dev[0x92] == 0 && vv.u.dev[0x93] == 10); /* the queued move was replayed */
	CHECK(!varvara_halted(&vv));
	printf("  left: reset resumed %d times, idle after %d ticks\n", resumes, ticks);
	return checks_failed == 0;
}

/* A ROM whose console vector runs ~530 steps and appends Console/read to
 * 0x3000 + [0x00]. */
static void load_slow_console_rom(void)
{
	static const ux_u8 reset[] = {
		0xa0, 0x02, 0x00, 0x80, 0x10, 0x37, 0x00 /* ;0200 .Console/vector DEO2 BRK */
	};
	static const ux_u8 con[] = {
		0x80, 0x80,             /* #80 */
		0x80, 0x01, 0x19,       /* &l #01 SUB */
		0x06, 0x20, 0xff, 0xf9, /* DUP ?&l */
		0x02,                   /* POP */
		0x80, 0x12, 0x16,       /* .Console/read DEI */
		0x80, 0x00, 0x10,       /* #00 LDZ */
		0x80, 0x30, 0x04, 0x15, /* #30 SWP STA */
		0x80, 0x00, 0x10, 0x01, 0x80, 0x00, 0x11, /* #00 LDZ INC #00 STZ */
		0x00
	};
	memcpy(vv.ram + 0x100, reset, sizeof reset);
	memcpy(vv.ram + 0x200, con, sizeof con);
}

/* Minor 5: an argument byte whose console vector hits the limit is
 * suspended and resumed, and the rest follow once it completes: every byte
 * once, in order, never a new console vector over a suspended one. */
static int args_resume_per_byte(void)
{
	static char a0[] = "ab", a1[] = "cd";
	char *args[2];
	int ticks;

	args[0] = a0; args[1] = a1;
	fresh(100);
	load_slow_console_rom();
	runner_boot(&rn, 2, args);
	CHECK(rn.reset_brk && rn.boot == RUN_BOOT_ARGS && !rn.busy);
	runner_tick(&rn, 0);
	CHECK(rn.busy && rn.boot == RUN_BOOT_ARGS); /* first byte suspended */
	CHECK(vv.ram[0x00] == 0);
	tick_until_idle(1000, &ticks);
	CHECK(rn.boot == RUN_BOOT_DONE && !rn.busy);
	CHECK(vv.ram[0x00] == 6);
	CHECK(memcmp(vv.ram + 0x3000, "ab\ncd\n", 6) == 0);
	CHECK(vv.u.dev[0x17] == 4); /* last argument end */
	CHECK(ticks > 6);

	/* varvara_boot with a limit stops at the first byte that hits it */
	fresh(0);
	load_slow_console_rom();
	{
		char *argv[4];
		argv[0] = (char *)"x"; argv[1] = (char *)"x.rom"; argv[2] = a0; argv[3] = a1;
		CHECK(varvara_boot(&vv, 4, argv, 100) != 0);
		CHECK(vv.ram[0x00] == 0);
		fresh(0);
		load_slow_console_rom();
		CHECK(varvara_boot(&vv, 4, argv, 0) == 0);
		CHECK(vv.ram[0x00] == 6 && memcmp(vv.ram + 0x3000, "ab\ncd\n", 6) == 0);
	}
	return checks_failed == 0;
}

/* A breakpoint on the reset vector's entry fires before anything runs, and
 * the reset then completes from the debugger. */
static int reset_entry_breakpoint(void)
{
	static char a0[] = "z";
	char *args[1];
	int ticks;

	args[0] = a0;
	fresh(100);
	load_slow_console_rom();
	dbg_toggle_breakpoint(&sess, VARVARA_RESET);
	runner_boot(&rn, 1, args);
	CHECK(sess.paused && rn.busy && rn.resume_pc == VARVARA_RESET);
	CHECK(vec(0x10) == 0); /* nothing ran */
	runner_tick(&rn, 0);   /* paused: nothing runs */
	CHECK(vec(0x10) == 0);
	runner_step(&rn, 0);   /* one instruction: LIT2 0200 */
	CHECK(rn.busy && rn.resume_pc == VARVARA_RESET + 3);
	sess.paused = 0;
	tick_until_idle(1000, &ticks);
	CHECK(rn.reset_brk && rn.boot == RUN_BOOT_DONE);
	CHECK(vv.ram[0x00] == 2 && memcmp(vv.ram + 0x3000, "z\n", 2) == 0);
	return checks_failed == 0;
}

/* Input while paused is dropped; a reset that faults gets no arguments. */
static int input_during_boot_queued(void)
{
	static char a0[] = "q";
	char *args[1];
	static const ux_u8 reset[] = {
		0xa0, 0x02, 0x00, 0x80, 0x10, 0x37,     /* ;0200 .Console/vector DEO2 */
		0xa0, 0x03, 0x00, 0x80, 0x80, 0x37,     /* ;0300 .Controller/vector DEO2 */
		0x00
	};
	static const ux_u8 ctl[] = {
		0x80, 0x83, 0x16, 0x80, 0x01, 0x10,     /* .Controller/key DEI #01 LDZ */
		0x80, 0x31, 0x04, 0x15,                 /* #31 SWP STA */
		0x80, 0x01, 0x10, 0x01, 0x80, 0x01, 0x11, /* count at 0x01 */
		0x00
	};

	args[0] = a0;
	fresh(100);
	load_slow_console_rom();
	memcpy(vv.ram + 0x100, reset, sizeof reset);
	memcpy(vv.ram + 0x300, ctl, sizeof ctl);
	runner_boot(&rn, 1, args);
	CHECK(rn.boot == RUN_BOOT_ARGS);
	runner_input(&rn, IQ_KEY, 'k', 0); /* owed arguments: queued, not run */
	CHECK(rn.iq.count == 1 && vv.ram[0x01] == 0);
	sess.paused = 1;
	runner_input(&rn, IQ_KEY, 'x', 0); /* paused: dropped */
	CHECK(rn.iq.count == 1);
	sess.paused = 0;
	tick_until_idle(1000, NULL);
	CHECK(memcmp(vv.ram + 0x3000, "q\n", 2) == 0);
	CHECK(vv.ram[0x01] == 1 && vv.ram[0x3100] == 'k'); /* after the arguments */
	return checks_failed == 0;
}

int main(int argc, char **argv)
{
	static const struct { const char *name; int (*fn)(void); } tests[] = {
		{ "left_boot_resumes", left_boot_resumes },
		{ "args_resume_per_byte", args_resume_per_byte },
		{ "reset_entry_breakpoint", reset_entry_breakpoint },
		{ "input_during_boot_queued", input_during_boot_queued }
	};
	unsigned i, total = sizeof tests / sizeof tests[0];
	int failed = 0;

	if (argc < 2) {
		fprintf(stderr, "usage: runner_test LEFT_ROM\n");
		return 2;
	}
	left_rom = argv[1];
	for (i = 0; i < total; i++) {
		checks_failed = 0;
		if (!tests[i].fn()) {
			printf("FAIL %s\n", tests[i].name);
			failed++;
		}
	}
	varvara_close(&vv);
	printf("runner: %u passed, %d failed\n", total - (unsigned)failed, failed);
	return failed != 0;
}
