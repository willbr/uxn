/* Resumable vector runs, the input queue and booting for the SDL host. See
 * runner.h. Pure C, no SDL. */
#include <stddef.h>
#include "runner.h"

/* The Varvara run hook: every vector Varvara starts (input, console,
 * arguments) is a fresh start, so a breakpoint on its first instruction
 * fires. With no breakpoints this is plain uxn_eval. */
static int runner_hook(Varvara *v, ux_u16 pc, unsigned long max_steps)
{
	Runner *r = (Runner *)v->ctx;

	if (r->sess->nbp > 0) return dbg_run(r->sess, v, pc, max_steps);
	return uxn_eval(&v->u, pc, max_steps);
}

void runner_init(Runner *r, Varvara *v, DebugSession *sess, unsigned long limit)
{
	void *user = r->user;

	r->v = v;
	r->sess = sess;
	r->limit = limit;
	r->busy = 0;
	r->resume_pc = 0;
	iq_init(&r->iq);
	r->boot = RUN_BOOT_DONE;
	r->reset_brk = 0;
	varvara_args_begin(v, &r->args, 0, NULL);
	r->user = user;
	v->run = runner_hook;
	v->ctx = r;
	v->max_steps = limit;
}

unsigned runner_screen_vector(const Runner *r)
{
	return (unsigned)r->v->u.dev[0x20] << 8 | r->v->u.dev[0x21];
}

/* Record how a vector run ended. One that did not finish stays suspended at
 * resume_pc; one that did gets Varvara's deferred port clears, and if it was
 * the reset vector the arguments become due (only after a BRK). */
static void vector_result(Runner *r, int res)
{
	r->busy = (res == UXN_LIMIT);
	if (r->busy) {
		r->resume_pc = r->v->u.pc;
		return;
	}
	varvara_vector_done(r->v);
	if (r->boot == RUN_BOOT_RESET) {
		r->reset_brk = (res == UXN_BRK);
		r->boot = r->reset_brk ? RUN_BOOT_ARGS : RUN_BOOT_DONE;
	}
}

void runner_run(Runner *r, unsigned pc, int fresh)
{
	Varvara *v = r->v;
	int res;

	if (r->sess->nbp > 0) {
		if (fresh) res = dbg_run(r->sess, v, (ux_u16)pc, r->limit);
		else {
			v->u.pc = (ux_u16)pc;
			res = dbg_continue(r->sess, v, r->limit);
		}
	} else {
		res = uxn_eval(&v->u, (ux_u16)pc, r->limit);
	}
	v->u.fault = 0;
	vector_result(r, res);
}

void runner_boot(Runner *r, int n, char **args)
{
	varvara_args_begin(r->v, &r->args, n, args);
	r->boot = RUN_BOOT_RESET;
	r->reset_brk = 0;
	runner_run(r, VARVARA_RESET, 1);
}

/* Deliver owed arguments. Never starts a console vector over a suspended
 * one; a console vector that stops early leaves the machine busy, and the
 * rest are delivered after it completes. */
static void deliver_args(Runner *r)
{
	if (r->busy || r->boot != RUN_BOOT_ARGS) return;
	if (varvara_args_deliver(r->v, &r->args)) {
		r->boot = RUN_BOOT_DONE;
	} else {
		r->busy = 1;
		r->resume_pc = r->v->u.pc;
	}
}

/* Input never runs a vector while another is suspended: an input vector
 * would overwrite u.pc and share the stacks. */
static void dispatch(Runner *r, const InputEv *e)
{
	Varvara *v = r->v;

	switch (e->kind) {
	case IQ_BUTTON: varvara_button(v, (ux_u8)e->a, e->b); break;
	case IQ_KEY: varvara_key(v, (ux_u8)e->a); break;
	case IQ_MOUSE_MOVE: varvara_mouse_move(v, e->a, e->b); break;
	case IQ_MOUSE_BUTTON: varvara_mouse_button(v, (ux_u8)e->a, e->b); break;
	case IQ_SCROLL: varvara_mouse_scroll(v, e->a, e->b); break;
	}
	if (v->limit_hit) {
		r->busy = 1;
		r->resume_pc = v->u.pc;
	}
}

void runner_input(Runner *r, IqKind kind, int a, int b)
{
	InputEv e;

	/* While the debugger has the machine paused, input is dropped. */
	if (r->sess->paused) return;
	e.kind = kind;
	e.a = a;
	e.b = b;
	if (r->busy || r->boot != RUN_BOOT_DONE || r->iq.count) iq_push(&r->iq, e);
	else dispatch(r, &e);
}

void runner_replay(Runner *r, ux_u8 held)
{
	InputEv e;

	deliver_args(r);
	if (r->boot != RUN_BOOT_DONE) return;
	while (!r->busy && iq_pop(&r->iq, &e)) dispatch(r, &e);
	if (!r->busy && r->iq.overflowed) {
		ux_u8 stuck = iq_stuck_buttons(r->v->u.dev[0x82], held);
		int b;

		for (b = 0; b < 8 && !r->busy; b++)
			if (stuck & (1 << b)) {
				InputEv rel;
				rel.kind = IQ_BUTTON; rel.a = 1 << b; rel.b = 0;
				dispatch(r, &rel);
			}
		if (!r->busy) r->iq.overflowed = 0;
	}
}

void runner_tick(Runner *r, ux_u8 held)
{
	int fresh;
	unsigned start;

	if (r->sess->paused) return;
	/* arguments come before the first frame */
	deliver_args(r);
	if (r->sess->paused) return;
	fresh = !r->busy;
	start = fresh ? runner_screen_vector(r) : r->resume_pc;
	r->busy = 0;
	if (start) runner_run(r, start, fresh);
	if (!r->sess->paused) runner_replay(r, held);
}

void runner_step(Runner *r, int over)
{
	Varvara *v = r->v;
	int res;

	r->sess->paused = 1;
	deliver_args(r);
	if (!r->busy) {
		unsigned sv = runner_screen_vector(r);

		if (!sv) return;
		r->busy = 1;
		r->resume_pc = sv;
	}
	v->u.pc = (ux_u16)r->resume_pc;
	res = over ? dbg_step_over(r->sess, v) : dbg_step(r->sess, v);
	v->u.fault = 0;
	vector_result(r, res);
}
