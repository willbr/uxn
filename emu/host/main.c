/* SDL3 desktop host for the Varvara emulator.
 *
 *   emu.exe [--root DIR] [--zoom 1|2|3] rom.rom [args...]
 *
 * A Host state struct, host_event() for one SDL event, host_frame() for one
 * 60 Hz tick and host_draw() for presentation. Right Ctrl toggles the
 * debugger (debugger.c draws it) and never reaches the ROM. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "varvara.h"
#include "keymap.h"
#include "input_queue.h"
#include "debugger.h"

#define FRAME_NS (1000000000 / 60)
/* Per vector call. Also stored in v->max_steps, which the input vectors use. */
#define STEP_LIMIT 2000000UL

typedef struct Host {
	SDL_Window *win;
	SDL_Renderer *ren;
	SDL_Texture *tex;
	int tex_w, tex_h;
	ux_u8 *rgba;
	int zoom;
	Varvara *v;
	KeyState keys;
	int debugger_open; /* toggled by Right Ctrl */
	DebuggerUI ui;     /* panels, console capture and the DebugSession (ui.sess.paused) */
	char rom_path[1024];
	/* A vector is suspended (at STEP_LIMIT, a breakpoint or a debugger step)
	 * and resumes at resume_pc. This, not v->u.pc, is the resume point: other
	 * code may run uxn_eval on the machine. */
	int busy;
	unsigned resume_pc;
	InputQueue iq;     /* input held back while busy; replayed when the vector ends */
	int quit;
	int exit_code;
	char root[1024];
	int has_root;
	Uint32 open_event; /* custom event: open-ROM dialog finished (data1 = SDL_strdup'd path) */
} Host;

static void on_console(void *ctx, int fd, int c)
{
	debugger_console_put(&((Host *)ctx)->ui, fd, c);
	fputc(c, fd == 2 ? stderr : stdout);
}

static int host_fail(const char *what)
{
	fprintf(stderr, "emu: %s failed: %s\n", what, SDL_GetError());
	return 1;
}

/* ---- machine management ---- */

/* Build a fresh machine from a ROM. On failure returns NULL. argv[0] is the
 * program, argv[1] the ROM, then the ROM's arguments. */
static Varvara *machine_new(const Host *h, const char *rom, int argc, char **argv)
{
	Varvara *v = (Varvara *)malloc(sizeof *v);

	if (!v) return NULL;
	varvara_init(v);
	v->on_console = on_console;
	v->ctx = (void *)h;
	v->max_steps = STEP_LIMIT;
	if ((h->has_root && varvara_set_root(v, h->root)) || varvara_load(v, rom)) {
		varvara_close(v);
		free(v);
		return NULL;
	}
	varvara_boot(v, argc, argv, STEP_LIMIT);
	return v;
}

static void host_set_title(Host *h, const char *rom)
{
	const char *base = strrchr(rom, '/');
	const char *back = strrchr(rom, '\\');
	char title[300];

	if (back && (!base || back > base)) base = back;
	base = base ? base + 1 : rom;
	snprintf(title, sizeof title, "%s - emu", base);
	SDL_SetWindowTitle(h->win, title);
}

/* Replace the running ROM (close, init, load, boot), keeping --root. If the
 * new ROM fails to load the old one keeps running. Returns 0 on success. */
int host_load_rom(Host *h, const char *path)
{
	char *argv[2];
	char keep[sizeof h->rom_path];
	Varvara *nv;

	snprintf(keep, sizeof keep, "%s", path); /* path may be h->rom_path itself */
	argv[0] = (char *)"emu";
	argv[1] = (char *)path;
	nv = machine_new(h, path, 2, argv);
	if (!nv) {
		fprintf(stderr, "emu: cannot load %s\n", path);
		return 1;
	}
	keymap_release_all(&h->keys);
	if (h->v) {
		varvara_close(h->v);
		free(h->v);
	}
	h->v = nv;
	h->busy = 0;
	h->ui.sess.paused = 0; /* breakpoints stay */
	iq_init(&h->iq);
	memcpy(h->rom_path, keep, sizeof keep);
	host_set_title(h, path);
	return 0;
}

static void open_dialog_done(void *userdata, const char *const *files, int filter)
{
	Host *h = (Host *)userdata;
	SDL_Event e;

	(void)filter;
	if (!files) {
		fprintf(stderr, "emu: open dialog failed: %s\n", SDL_GetError());
		return;
	}
	if (!files[0]) return; /* cancelled */
	SDL_zero(e);
	e.type = h->open_event;
	e.user.data1 = SDL_strdup(files[0]);
	if (e.user.data1 && !SDL_PushEvent(&e)) SDL_free(e.user.data1);
}

/* File > Open. Asynchronous (the callback may run on another thread): the
 * chosen ROM arrives as h->open_event and host_event() loads it. Not bound to
 * a key; the menu bar will call it. */
void host_open_rom_dialog(Host *h)
{
	static const SDL_DialogFileFilter filters[] = { { "Uxn ROMs", "rom" }, { "All files", "*" } };

	SDL_ShowOpenFileDialog(open_dialog_done, h, h->win, filters, 2, NULL, false);
}

/* ---- screen texture ---- */

/* Size the window for the screen and, when the debugger is open, its panels. */
static void host_layout(Host *h)
{
	int w, hh;

	if (h->tex_w < 1) return;
	debugger_window_size(h->debugger_open, h->tex_w, h->tex_h, h->zoom, &w, &hh);
	SDL_SetWindowSize(h->win, w, hh);
	h->ui.relayout = 1;
}

/* Recreate the streaming texture and resize the window when the ROM changed
 * the screen size. */
static int host_sync_screen(Host *h)
{
	const Varvara *v = h->v;

	if (v->width < 1 || v->height < 1) return 0; /* allocation failure: keep what we have */
	if (h->tex && v->width == h->tex_w && v->height == h->tex_h) return 0;
	if (h->tex) SDL_DestroyTexture(h->tex);
	free(h->rgba);
	h->tex = SDL_CreateTexture(h->ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
		v->width, v->height);
	h->rgba = (ux_u8 *)malloc((size_t)v->width * (size_t)v->height * 4);
	if (!h->tex || !h->rgba) return host_fail("create screen texture");
	SDL_SetTextureScaleMode(h->tex, SDL_SCALEMODE_NEAREST);
	h->tex_w = v->width;
	h->tex_h = v->height;
	host_layout(h);
	return 0;
}

/* ---- input ---- */

/* ---- running vectors ---- */

static unsigned screen_vector(const Varvara *v)
{
	return (unsigned)v->u.dev[0x20] << 8 | v->u.dev[0x21];
}

static void host_check_halt(Host *h)
{
	if (varvara_halted(h->v)) {
		h->exit_code = varvara_exit_code(h->v);
		h->quit = 1;
	}
}

/* Run a vector from pc until BRK, STEP_LIMIT or (when a breakpoint is set) a
 * breakpoint. Leaves busy / resume_pc describing a vector that did not finish.
 * With no breakpoints this is the plain fast path. */
static void host_run(Host *h, unsigned pc)
{
	Varvara *v = h->v;
	int r;

	if (h->ui.sess.nbp > 0) {
		v->u.pc = (ux_u16)pc;
		r = dbg_continue(&h->ui.sess, v, STEP_LIMIT);
	} else {
		r = uxn_eval(&v->u, (ux_u16)pc, STEP_LIMIT);
	}
	v->u.fault = 0;
	h->busy = (r == UXN_LIMIT);
	if (h->busy) h->resume_pc = v->u.pc;
}

/* Input never runs a vector while another is suspended: an input vector would
 * overwrite u.pc and share the stacks. host_input() runs the call at once, or
 * queues it while busy; host_replay() drains the queue once the vector ends. */
static void host_dispatch(Host *h, const InputEv *e)
{
	Varvara *v = h->v;
	int dbg = h->ui.sess.nbp > 0;

	/* With breakpoints the input function may only run one instruction; the
	 * rest of its vector then goes through host_run, which checks them. A
	 * breakpoint on the vector's first instruction is therefore not seen. */
	v->max_steps = dbg ? 1 : STEP_LIMIT;
	switch (e->kind) {
	case IQ_BUTTON: varvara_button(v, (ux_u8)e->a, e->b); break;
	case IQ_KEY: varvara_key(v, (ux_u8)e->a); break;
	case IQ_MOUSE_MOVE: varvara_mouse_move(v, e->a, e->b); break;
	case IQ_MOUSE_BUTTON: varvara_mouse_button(v, (ux_u8)e->a, e->b); break;
	case IQ_SCROLL: varvara_mouse_scroll(v, e->a, e->b); break;
	}
	v->max_steps = STEP_LIMIT;
	if (v->limit_hit) {
		h->busy = 1;
		h->resume_pc = v->u.pc;
		if (dbg) host_run(h, h->resume_pc);
	}
}

static void host_input(Host *h, IqKind kind, int a, int b)
{
	InputEv e;

	/* While the debugger has the machine paused, input is dropped. */
	if (h->ui.sess.paused) return;
	e.kind = kind;
	e.a = a;
	e.b = b;
	if (h->busy || h->iq.count) iq_push(&h->iq, e);
	else host_dispatch(h, &e);
}

/* Replay queued input in order; stops if a call leaves the machine busy again.
 * After an overflow, release Controller buttons the ROM has down that the
 * host no longer holds, so nothing sticks. */
static void host_replay(Host *h)
{
	InputEv e;

	while (!h->busy && iq_pop(&h->iq, &e)) host_dispatch(h, &e);
	if (!h->busy && h->iq.overflowed) {
		ux_u8 stuck = iq_stuck_buttons(h->v->u.dev[0x82], h->keys.buttons);
		int b;

		for (b = 0; b < 8 && !h->busy; b++)
			if (stuck & (1 << b)) {
				InputEv r;
				r.kind = IQ_BUTTON; r.a = 1 << b; r.b = 0;
				host_dispatch(h, &r);
			}
		if (!h->busy) h->iq.overflowed = 0;
	}
}

/* Send every held Controller button as released. Used on focus loss and when
 * the debugger takes the keyboard, so no button stays stuck in the ROM. */
static void host_release_buttons(Host *h)
{
	ux_u8 mask = keymap_release_all(&h->keys);
	int b;

	for (b = 0; b < 8; b++)
		if (mask & (1 << b)) host_input(h, IQ_BUTTON, 1 << b, 0);
}

/* Leave the paused state. Releases that were dropped while paused are sent
 * now: Controller buttons the ROM has down that the host no longer holds, and
 * any Mouse button the ROM has down (the debugger owns the mouse). */
static void host_unpause(Host *h)
{
	ux_u8 stuck, mouse;
	int b;

	if (!h->ui.sess.paused) return;
	h->ui.sess.paused = 0;
	stuck = iq_stuck_buttons(h->v->u.dev[0x82], h->keys.buttons);
	mouse = (ux_u8)(h->v->u.dev[0x96] & 7);
	for (b = 0; b < 8; b++) {
		if (stuck & (1 << b)) host_input(h, IQ_BUTTON, 1 << b, 0);
		if (b < 3 && (mouse & (1 << b))) host_input(h, IQ_MOUSE_BUTTON, 1 << b, 0);
	}
}

static void host_key(Host *h, const SDL_KeyboardEvent *k)
{
	ux_u8 changed, ch;

	if (k->repeat) {
		/* auto-repeat repeats characters (Backspace, Ctrl+letter), not buttons */
		if (!h->debugger_open && keymap_event(&h->keys, k->key, k->mod, 1, &changed, &ch) == KEY_NONE && ch)
			host_input(h, IQ_KEY, ch, 0);
		return;
	}
	if (keymap_event(&h->keys, k->key, k->mod, k->down, &changed, &ch) == KEY_TOGGLE_DEBUGGER) {
		h->debugger_open = !h->debugger_open;
		host_release_buttons(h);
		host_layout(h);
		if (!h->debugger_open) host_unpause(h); /* a hidden debugger must not hold the ROM */
		return;
	}
	if (h->debugger_open) {
		/* the debugger owns the keyboard, but a release must still reach the ROM */
		if (changed && !k->down) host_input(h, IQ_BUTTON, changed, 0);
		return;
	}
	if (changed) host_input(h, IQ_BUTTON, changed, k->down);
	if (ch) host_input(h, IQ_KEY, ch, 0);
}

static void host_mouse_pos(Host *h, float x, float y)
{
	host_input(h, IQ_MOUSE_MOVE, (int)floorf(x / (float)h->zoom), (int)floorf(y / (float)h->zoom));
}

void host_event(Host *h, const SDL_Event *e)
{
	if (e->type == h->open_event) {
		host_load_rom(h, (const char *)e->user.data1);
		SDL_free(e->user.data1);
		return;
	}
	switch (e->type) {
	case SDL_EVENT_QUIT:
		h->quit = 1;
		break;
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
		host_key(h, &e->key);
		break;
	case SDL_EVENT_TEXT_INPUT:
		if (!h->debugger_open) {
			const unsigned char *p = (const unsigned char *)e->text.text;
			for (; *p; p++) host_input(h, IQ_KEY, *p, 0); /* UTF-8 bytes pass through */
		}
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		host_release_buttons(h);
		break;
	case SDL_EVENT_MOUSE_MOTION:
		if (!h->debugger_open) host_mouse_pos(h, e->motion.x, e->motion.y);
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP: {
		int down = e->button.down;
		ux_u8 mask = e->button.button == SDL_BUTTON_LEFT ? 0x01
			: e->button.button == SDL_BUTTON_MIDDLE ? 0x02
			: e->button.button == SDL_BUTTON_RIGHT ? 0x04 : 0;

		/* a release always reaches the ROM so no mouse button sticks */
		if (h->debugger_open && down) break;
		host_mouse_pos(h, e->button.x, e->button.y);
		host_input(h, IQ_MOUSE_BUTTON, mask, down);
		break;
	}
	case SDL_EVENT_MOUSE_WHEEL:
		if (!h->debugger_open && (e->wheel.integer_x || e->wheel.integer_y))
			host_input(h, IQ_SCROLL, (int)e->wheel.integer_x, (int)e->wheel.integer_y);
		break;
	default:
		break;
	}
	if (varvara_halted(h->v)) {
		h->exit_code = varvara_exit_code(h->v);
		h->quit = 1;
	}
}

/* ---- frame ---- */

/* One 60 Hz tick: run the Screen vector, or resume a suspended vector at
 * resume_pc (varvara_frame always restarts at the vector, so the host calls
 * uxn_eval itself; the stacks live in the Uxn so a resume is exact). When the
 * vector ends, queued input is replayed. Then upload the screen. Returns
 * nonzero on an SDL error. */
int host_frame(Host *h)
{
	Varvara *v = h->v;

	/* Paused in the debugger: nothing runs and queued input waits. */
	if (!h->ui.sess.paused) {
		unsigned start = h->busy ? h->resume_pc : screen_vector(v);

		h->busy = 0;
		if (start) host_run(h, start);
		if (!h->ui.sess.paused) host_replay(h);
	}
	host_check_halt(h);
	if (host_sync_screen(h)) return 1;
	varvara_render(v, h->rgba);
	if (!SDL_UpdateTexture(h->tex, NULL, h->rgba, h->tex_w * 4)) return host_fail("SDL_UpdateTexture");
	fflush(stdout);
	return 0;
}

/* ---- debugger actions ---- */

/* One debugger step. If no vector is suspended, the next one to run is the
 * Screen vector, so stepping walks through the frame. The session runs from
 * resume_pc, which is the only record of where the vector stands. */
static void host_step(Host *h, int over)
{
	Varvara *v = h->v;
	int r;

	h->ui.sess.paused = 1;
	if (!h->busy) {
		unsigned sv = screen_vector(v);

		if (!sv) return;
		h->busy = 1;
		h->resume_pc = sv;
	}
	v->u.pc = (ux_u16)h->resume_pc;
	r = over ? dbg_step_over(&h->ui.sess, v) : dbg_step(&h->ui.sess, v);
	v->u.fault = 0;
	h->busy = (r == UXN_LIMIT);
	if (h->busy) h->resume_pc = v->u.pc;
	host_check_halt(h);
}

static void host_action(Host *h, DebugAction a)
{
	switch (a.kind) {
	case DA_NONE: break;
	case DA_PAUSE: h->ui.sess.paused = 1; break;
	case DA_CONTINUE: host_unpause(h); break;
	case DA_STEP: host_step(h, 0); break;
	case DA_STEP_OVER: host_step(h, 1); break;
	case DA_RESET:
	case DA_RELOAD:
		if (h->rom_path[0]) host_load_rom(h, h->rom_path);
		break;
	case DA_OPEN: host_open_rom_dialog(h); break;
	case DA_ZOOM:
		if (a.zoom >= 1 && a.zoom <= 3 && a.zoom != h->zoom) {
			h->zoom = a.zoom;
			host_layout(h);
		}
		break;
	}
}

void host_draw(Host *h)
{
	SDL_FRect dst;

	SDL_SetRenderDrawColor(h->ren, 0, 0, 0, 255);
	SDL_RenderClear(h->ren);
	dst.x = 0;
	dst.y = h->debugger_open ? DBG_MENU_H : 0;
	dst.w = (float)(h->tex_w * h->zoom);
	dst.h = (float)(h->tex_h * h->zoom);
	SDL_RenderTexture(h->ren, h->tex, NULL, &dst);
	if (h->debugger_open) {
		DebugInfo info;

		info.paused = h->ui.sess.paused;
		info.in_vector = h->busy;
		info.pc = h->busy ? h->resume_pc : screen_vector(h->v);
		info.zoom = h->zoom;
		info.can_reload = h->rom_path[0] != '\0';
		host_action(h, debugger_frame(&h->ui, h->ren, h->v, &info));
	} else if (h->busy) {
		SDL_SetRenderDrawColor(h->ren, 255, 255, 0, 255);
		SDL_RenderDebugText(h->ren, 8, 8, "ROM busy");
	}
	SDL_RenderPresent(h->ren);
}

/* ---- startup ---- */

static void usage(void)
{
	fprintf(stderr, "usage: emu [--root DIR] [--zoom 1|2|3] rom.rom [args...]\n");
}

int main(int argc, char **argv)
{
	Host host;
	Host *h = &host;
	const char *rom;
	char **rom_argv;
	int i, rom_argc, first = 1;
	Uint64 next;

	memset(&host, 0, sizeof host);
	host.zoom = 2;
	while (first < argc && strncmp(argv[first], "--", 2) == 0) {
		if (strcmp(argv[first], "--root") == 0 && first + 1 < argc) {
			snprintf(host.root, sizeof host.root, "%s", argv[first + 1]);
			host.has_root = 1;
			first += 2;
		} else if (strcmp(argv[first], "--zoom") == 0 && first + 1 < argc) {
			host.zoom = atoi(argv[first + 1]);
			if (host.zoom < 1 || host.zoom > 3) { usage(); return 2; }
			first += 2;
		} else {
			usage();
			return 2;
		}
	}
	if (first >= argc) { usage(); return 2; }
	rom = argv[first];
	/* argv for the ROM: [0] program, [1] ROM, then its arguments */
	rom_argc = argc - first + 1;
	rom_argv = (char **)malloc(sizeof(char *) * (size_t)(rom_argc + 1));
	if (!rom_argv) return 1;
	rom_argv[0] = (char *)"emu";
	for (i = first; i < argc; i++) rom_argv[i - first + 1] = argv[i];
	rom_argv[rom_argc] = NULL;

	if (!SDL_Init(SDL_INIT_VIDEO)) return host_fail("SDL_Init");
	host.open_event = SDL_RegisterEvents(1);
	host.v = machine_new(&host, rom, rom_argc, rom_argv);
	if (!host.v) {
		fprintf(stderr, "emu: cannot load %s\n", rom);
		SDL_Quit();
		return 1;
	}
	host.win = SDL_CreateWindow("emu", host.v->width * host.zoom, host.v->height * host.zoom, 0);
	if (!host.win) return host_fail("SDL_CreateWindow");
	host.ren = SDL_CreateRenderer(host.win, NULL);
	if (!host.ren) return host_fail("SDL_CreateRenderer");
	host_set_title(&host, rom);
	if (debugger_init(host.win, host.ren)) return host_fail("debugger_init");
	host.ui.mem_addr = 0x100;
	if (!SDL_StartTextInput(host.win)) return host_fail("SDL_StartTextInput");
	if (varvara_halted(host.v)) {
		host.exit_code = varvara_exit_code(host.v);
		host.quit = 1;
	}

	next = SDL_GetTicksNS();
	while (!host.quit) {
		SDL_Event e;
		Uint64 now;

		while (SDL_PollEvent(&e)) {
			if (host.debugger_open) debugger_event(&e);
			host_event(h, &e);
		}
		if (host.quit) break;
		if (host_frame(h)) { host.exit_code = 1; break; }
		host_draw(h);
		next += FRAME_NS;
		now = SDL_GetTicksNS();
		if (now < next) SDL_DelayNS(next - now);
		else if (now - next > 100000000) next = now; /* far behind: do not spiral */
	}

	fflush(stdout);
	varvara_close(host.v);
	free(host.v);
	free(host.rgba);
	debugger_shutdown();
	if (host.tex) SDL_DestroyTexture(host.tex);
	SDL_DestroyRenderer(host.ren);
	SDL_DestroyWindow(host.win);
	SDL_Quit();
	free(rom_argv);
	return host.exit_code;
}
