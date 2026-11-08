/* Keymap unit tests. Links SDL3 headers only; calls no SDL functions. */
#include <stdio.h>
#include "keymap.h"
#include "input_queue.h"

static int failed = 0, total = 0, bad_tests = 0;
#define CHECK(c) do { if (!(c)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); failed++; } } while (0)

static void rctrl_toggles_not_A(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	CHECK(keymap_event(&ks, SDLK_RCTRL, 0, SDL_KMOD_RCTRL, 1, &b, &ch) == KEY_TOGGLE_DEBUGGER);
	CHECK(ks.buttons == 0 && b == 0 && ch == 0);
	CHECK(keymap_event(&ks, SDLK_RCTRL, 0, 0, 0, &b, &ch) == KEY_NONE);
	CHECK(ks.buttons == 0);
}

static void lctrl_is_A(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	CHECK(keymap_event(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, 1, &b, &ch) == KEY_NONE);
	CHECK(ks.buttons == 0x01 && b == 0x01);
	keymap_event(&ks, SDLK_LCTRL, 0, 0, 0, &b, &ch);
	CHECK(ks.buttons == 0 && b == 0x01);
}

static void other_buttons(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LALT, 0, 0, 1, &b, &ch); CHECK(b == 0x02);
	keymap_event(&ks, SDLK_RALT, 0, 0, 1, &b, &ch); CHECK(b == 0x02);
	keymap_event(&ks, SDLK_LSHIFT, 0, 0, 1, &b, &ch); CHECK(b == 0x04);
	keymap_event(&ks, SDLK_RSHIFT, 0, 0, 1, &b, &ch); CHECK(b == 0x04);
	keymap_event(&ks, SDLK_HOME, 0, 0, 1, &b, &ch); CHECK(b == 0x08);
	keymap_event(&ks, SDLK_UP, 0, 0, 1, &b, &ch); CHECK(b == 0x10);
	keymap_event(&ks, SDLK_DOWN, 0, 0, 1, &b, &ch); CHECK(b == 0x20);
	keymap_event(&ks, SDLK_LEFT, 0, 0, 1, &b, &ch); CHECK(b == 0x40);
	keymap_event(&ks, SDLK_RIGHT, 0, 0, 1, &b, &ch); CHECK(b == 0x80);
	CHECK(ks.buttons == 0xfe);
	/* banned-as-shortcut keys do nothing for the ROM either */
	keymap_event(&ks, SDLK_F1, 0, 0, 1, &b, &ch); CHECK(b == 0 && ch == 0);
	keymap_event(&ks, SDLK_PAGEUP, 0, 0, 1, &b, &ch); CHECK(b == 0 && ch == 0);
}

static void tab_is_char_9(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_TAB, 0, 0, 1, &b, &ch);
	CHECK(ch == 0x09 && b == 0);
	keymap_event(&ks, SDLK_TAB, 0, 0, 0, &b, &ch);
	CHECK(ch == 0);
	keymap_event(&ks, SDLK_RETURN, 0, 0, 1, &b, &ch); CHECK(ch == 0x0d);
	keymap_event(&ks, SDLK_ESCAPE, 0, 0, 1, &b, &ch); CHECK(ch == 0x1b);
	keymap_event(&ks, SDLK_BACKSPACE, 0, 0, 1, &b, &ch); CHECK(ch == 0x08);
	keymap_event(&ks, SDLK_DELETE, 0, 0, 1, &b, &ch); CHECK(ch == 0x7f);
}

static void ctrl_letter(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	keymap_event(&ks, SDLK_S, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 's' && ks.buttons == 0x01);
	/* without Ctrl a letter gives no key here (text input supplies it) */
	ks.buttons = 0; ks.chord = 0;
	keymap_event(&ks, SDLK_S, 0, 0, 1, &b, &ch);
	CHECK(ch == 0);
}

/* Finding 2: any printable chord reaches the ROM with Shift applied, as
 * uxn11 sends the keysym for keys below 0x80. */
static void ctrl_slash(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	keymap_event(&ks, SDLK_SLASH, '/', SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 0x2f && b == 0);
	keymap_event(&ks, SDLK_SLASH, '/', SDL_KMOD_LCTRL, 0, &b, &ch);
	CHECK(ch == 0);
}

static void ctrl_shift_v(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	keymap_event(&ks, SDLK_LSHIFT, 0, SDL_KMOD_LCTRL | SDL_KMOD_LSHIFT, 1, &b, &ch);
	CHECK(ks.buttons == 0x05);
	keymap_event(&ks, SDLK_V, 'V', SDL_KMOD_LCTRL | SDL_KMOD_LSHIFT, 1, &b, &ch);
	CHECK(ch == 0x56);
	keymap_event(&ks, SDLK_D, 'D', SDL_KMOD_LCTRL | SDL_KMOD_LSHIFT, 1, &b, &ch);
	CHECK(ch == 0x44);
	/* Ctrl alone: lowercase, as before */
	keymap_event(&ks, SDLK_LSHIFT, 0, SDL_KMOD_LCTRL, 0, &b, &ch);
	keymap_event(&ks, SDLK_V, 'v', SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 0x76);
	/* typed 0: falls back to the key */
	keymap_event(&ks, SDLK_V, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 0x76);
}

static void alt_letter(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LALT, 0, SDL_KMOD_LALT, 1, &b, &ch);
	CHECK(b == 0x02);
	keymap_event(&ks, SDLK_A, 'a', SDL_KMOD_LALT, 1, &b, &ch);
	CHECK(ch == 0x61 && (ks.buttons & 0x02));
	/* on a layout without AltGr, Right Alt is Alt too */
	ks.buttons = 0; ks.chord = 0;
	keymap_event(&ks, SDLK_RALT, 0, SDL_KMOD_RALT, 1, &b, &ch);
	CHECK(b == 0x02);
	keymap_event(&ks, SDLK_A, 'a', SDL_KMOD_RALT, 1, &b, &ch);
	CHECK(ch == 0x61);
	/* no chord: printable keys come from text input */
	ks.buttons = 0; ks.chord = 0;
	keymap_event(&ks, SDLK_A, 'a', 0, 1, &b, &ch);
	CHECK(ch == 0);
	/* a chord never sends a non-printable typed code */
	keymap_event(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	keymap_event(&ks, SDLK_F1, SDLK_F1, SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 0);
	keymap_event(&ks, SDLK_A, 0xe9, SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 0);
}

/* Ctrl+Space: keymap sends the space; the TEXT_INPUT " " SDL also sends for
 * it is dropped, so the ROM gets it once. */
static void ctrl_space_once(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	int delivered = 0;
	keymap_event(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, 1, &b, &ch);
	keymap_event(&ks, SDLK_SPACE, ' ', SDL_KMOD_LCTRL, 1, &b, &ch);
	if (ch == 0x20) delivered++;
	if (keymap_text_allowed(&ks, SDL_KMOD_LCTRL)) delivered++; /* TEXT_INPUT " " */
	CHECK(delivered == 1);
	/* the modifier held before the window had focus counts too */
	CHECK(!keymap_text_allowed(&(KeyState){0}, SDL_KMOD_LALT));
	keymap_event(&ks, SDLK_LCTRL, 0, 0, 0, &b, &ch);
	CHECK(keymap_text_allowed(&ks, 0));
	/* Shift alone is no chord */
	CHECK(keymap_text_allowed(&ks, SDL_KMOD_LSHIFT));
}

/* AltGr (Right Alt on a layout that has it): the character comes from text
 * input only; no chord, no button A or B. */
static void altgr_types_text(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	ks.altgr = 1;
	keymap_event(&ks, SDLK_RALT, 0, SDL_KMOD_RALT, 1, &b, &ch);
	CHECK(b == 0 && ks.buttons == 0);
	keymap_event(&ks, SDLK_Q, 'q', SDL_KMOD_RALT, 1, &b, &ch);
	CHECK(ch == 0 && b == 0);
	CHECK(keymap_text_allowed(&ks, SDL_KMOD_RALT)); /* TEXT_INPUT "@" reaches the ROM */
	/* with a Left Ctrl in the modifiers too (Windows' AltGr = LCtrl + RAlt) */
	keymap_event(&ks, SDLK_Q, 'q', SDL_KMOD_LCTRL | SDL_KMOD_RALT, 1, &b, &ch);
	CHECK(ch == 0);
	CHECK(keymap_text_allowed(&ks, SDL_KMOD_LCTRL | SDL_KMOD_RALT));
	keymap_event(&ks, SDLK_RALT, 0, 0, 0, &b, &ch);
	CHECK(b == 0 && ks.buttons == 0 && ks.chord == 0);
	/* Left Alt is still B */
	keymap_event(&ks, SDLK_LALT, 0, SDL_KMOD_LALT, 1, &b, &ch);
	CHECK(b == 0x02);
}

static void rctrl_is_no_chord(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	CHECK(keymap_event(&ks, SDLK_RCTRL, 0, SDL_KMOD_RCTRL, 1, &b, &ch) == KEY_TOGGLE_DEBUGGER);
	keymap_event(&ks, SDLK_S, 's', SDL_KMOD_RCTRL, 1, &b, &ch);
	CHECK(ch == 0 && ks.buttons == 0);
	keymap_repeat(&ks, SDLK_RCTRL, 0, SDL_KMOD_RCTRL, &b, &ch); /* a held Right Ctrl repeats */
	CHECK(b == 0 && ch == 0 && ks.buttons == 0);
}

/* Finding 3: a held arrow or Home re-runs the Controller vector on each
 * repeat; modifier buttons do not. */
static void repeat_directions(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_UP, 0, 0, 1, &b, &ch);
	keymap_repeat(&ks, SDLK_UP, 0, 0, &b, &ch); CHECK(b == 0x10 && ch == 0);
	keymap_repeat(&ks, SDLK_DOWN, 0, 0, &b, &ch); CHECK(b == 0x20);
	keymap_repeat(&ks, SDLK_LEFT, 0, 0, &b, &ch); CHECK(b == 0x40);
	keymap_repeat(&ks, SDLK_RIGHT, 0, 0, &b, &ch); CHECK(b == 0x80);
	keymap_repeat(&ks, SDLK_HOME, 0, 0, &b, &ch); CHECK(b == 0x08);
	keymap_repeat(&ks, SDLK_LCTRL, 0, SDL_KMOD_LCTRL, &b, &ch); CHECK(b == 0 && ch == 0);
	keymap_repeat(&ks, SDLK_LALT, 0, SDL_KMOD_LALT, &b, &ch); CHECK(b == 0 && ch == 0);
	keymap_repeat(&ks, SDLK_LSHIFT, 0, SDL_KMOD_LSHIFT, &b, &ch); CHECK(b == 0 && ch == 0);
	keymap_repeat(&ks, SDLK_BACKSPACE, 0, 0, &b, &ch); CHECK(b == 0 && ch == 0x08);
	CHECK(ks.buttons == 0x10);
	ks.chord = KS_LCTRL;
	keymap_repeat(&ks, SDLK_S, 's', SDL_KMOD_LCTRL, &b, &ch); CHECK(ch == 's');
}

static void keymap_release_on_focus_loss(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LCTRL, 0, 0, 1, &b, &ch);
	keymap_event(&ks, SDLK_UP, 0, 0, 1, &b, &ch);
	CHECK(keymap_release_all(&ks) == 0x11);
	CHECK(ks.buttons == 0);
	CHECK(keymap_release_all(&ks) == 0);
}

static InputEv ev(int a)
{
	InputEv e;
	e.kind = IQ_KEY; e.a = a; e.b = 0;
	return e;
}

static void queue_fifo_order(void)
{
	InputQueue q; InputEv e; int i;
	iq_init(&q);
	for (i = 0; i < 5; i++) CHECK(iq_push(&q, ev(i)));
	for (i = 0; i < 5; i++) { CHECK(iq_pop(&q, &e)); CHECK(e.a == i); }
	CHECK(!iq_pop(&q, &e));
	/* wraps around */
	for (i = 0; i < 3 * IQ_CAP; i++) { CHECK(iq_push(&q, ev(i))); CHECK(iq_pop(&q, &e) && e.a == i); }
	CHECK(!q.overflowed);
}

static void queue_overflow_drops(void)
{
	InputQueue q; InputEv e; int i;
	iq_init(&q);
	for (i = 0; i < IQ_CAP; i++) CHECK(iq_push(&q, ev(i)));
	CHECK(!q.overflowed);
	CHECK(!iq_push(&q, ev(999)));
	CHECK(q.overflowed && q.count == IQ_CAP);
	for (i = 0; i < IQ_CAP; i++) { CHECK(iq_pop(&q, &e)); CHECK(e.a == i); }
	CHECK(!iq_pop(&q, &e));
}

static void queue_stuck_release_after_overflow(void)
{
	/* the ROM saw A and Up go down; the host has since seen Up released (dropped) */
	CHECK(iq_stuck_buttons(0x11, 0x01) == 0x10);
	CHECK(iq_stuck_buttons(0x11, 0x11) == 0);
	CHECK(iq_stuck_buttons(0x00, 0x11) == 0);
	CHECK(iq_stuck_buttons(0x11, 0x00) == 0x11);
}

int main(void)
{
	int before;
#define T(f) do { total++; before = failed; f(); if (failed != before) { printf("FAIL %s\n", #f); bad_tests++; } } while (0)
	T(rctrl_toggles_not_A); T(lctrl_is_A); T(other_buttons); T(tab_is_char_9);
	T(ctrl_letter); T(ctrl_slash); T(ctrl_shift_v); T(alt_letter); T(ctrl_space_once);
	T(altgr_types_text); T(rctrl_is_no_chord); T(repeat_directions);
	T(keymap_release_on_focus_loss);
	T(queue_fifo_order); T(queue_overflow_drops); T(queue_stuck_release_after_overflow);
#undef T
	printf("keymap: %d passed, %d failed\n", total - bad_tests, bad_tests);
	return failed ? 1 : 0;
}
