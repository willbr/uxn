/* How the SDL host runs one machine's vectors, without SDL: a vector that
 * uses up the step limit (or stops at a breakpoint) stays suspended and is
 * resumed on later ticks; input that arrives meanwhile is queued and replayed
 * in order once it finishes. Booting goes through the same path: the reset
 * vector is resumable, and the console arguments are delivered only after it
 * reaches BRK, before any queued input.
 *
 * Pure C (C89 subset), no SDL, so it is tested headlessly (tests/runner_test.c). */
#ifndef RUNNER_H
#define RUNNER_H

#include "varvara.h"
#include "session.h"
#include "input_queue.h"

typedef struct Runner {
	Varvara *v;
	DebugSession *sess;  /* breakpoints and the paused flag; not owned */
	unsigned long limit; /* steps per vector run (the host's STEP_LIMIT) */
	/* A vector is suspended (at the limit, a breakpoint or a debugger step)
	 * and resumes at resume_pc. This, not v->u.pc, is the resume point. */
	int busy;
	unsigned resume_pc;
	InputQueue iq;       /* input held back while busy or booting */
	/* RUN_BOOT_RESET while the reset vector runs, RUN_BOOT_ARGS while the
	 * console arguments are owed, RUN_BOOT_DONE after. */
	int boot;
	int reset_brk;       /* the reset vector reached BRK (not a fault) */
	VarvaraArgs args;
	void *user;          /* for the host (its Console callback finds the Host here) */
} Runner;

enum { RUN_BOOT_DONE, RUN_BOOT_RESET, RUN_BOOT_ARGS };

/* Attach r to v: v->run, v->ctx (= r) and v->max_steps (= limit) are set.
 * The rest of r is cleared, except user. */
void runner_init(Runner *r, Varvara *v, DebugSession *sess, unsigned long limit);
/* Start the machine: set up the n arguments (not copied; they must outlive
 * the boot) and run the reset vector from VARVARA_RESET as a fresh start, so
 * a breakpoint there fires. It may be left suspended (busy). The arguments
 * are delivered by a later runner_tick or runner_step. */
void runner_boot(Runner *r, int n, char **args);
/* Run a vector from pc until BRK, the limit or a breakpoint. fresh: pc is a
 * vector's entry; otherwise it resumes a suspended vector. */
void runner_run(Runner *r, unsigned pc, int fresh);
/* One input call (see InputEv). Dropped while paused; queued while busy,
 * booting or while older input is still queued; otherwise run now. */
void runner_input(Runner *r, IqKind kind, int a, int b);
/* Deliver owed arguments, then queued input, in order, until something
 * leaves the machine busy. held: Controller buttons the host still holds
 * (after a queue overflow, buttons the ROM has down but the host does not
 * are released). */
void runner_replay(Runner *r, ux_u8 held);
/* One 60 Hz tick, unless paused: deliver owed arguments, then resume the
 * suspended vector or run the Screen vector, then replay queued input. */
void runner_tick(Runner *r, ux_u8 held);
/* One debugger step (over: step over a call). Pauses. With nothing
 * suspended, owed arguments are delivered first (not stepped), and then the
 * next vector to step through is the Screen vector. */
void runner_step(Runner *r, int over);
/* The Screen vector (0 if none). */
unsigned runner_screen_vector(const Runner *r);

#endif
