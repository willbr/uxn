/* Keymap unit tests. Links SDL3 headers only; calls no SDL functions. */
#include <stdio.h>
#include "keymap.h"

static int failed = 0, total = 0, bad_tests = 0;
#define CHECK(c) do { if (!(c)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); failed++; } } while (0)

static void rctrl_toggles_not_A(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	CHECK(keymap_event(&ks, SDLK_RCTRL, SDL_KMOD_RCTRL, 1, &b, &ch) == KEY_TOGGLE_DEBUGGER);
	CHECK(ks.buttons == 0 && b == 0 && ch == 0);
	CHECK(keymap_event(&ks, SDLK_RCTRL, 0, 0, &b, &ch) == KEY_NONE);
	CHECK(ks.buttons == 0);
}

static void lctrl_is_A(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	CHECK(keymap_event(&ks, SDLK_LCTRL, SDL_KMOD_LCTRL, 1, &b, &ch) == KEY_NONE);
	CHECK(ks.buttons == 0x01 && b == 0x01);
	keymap_event(&ks, SDLK_LCTRL, 0, 0, &b, &ch);
	CHECK(ks.buttons == 0 && b == 0x01);
}

static void other_buttons(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LALT, 0, 1, &b, &ch); CHECK(b == 0x02);
	keymap_event(&ks, SDLK_RALT, 0, 1, &b, &ch); CHECK(b == 0x02);
	keymap_event(&ks, SDLK_LSHIFT, 0, 1, &b, &ch); CHECK(b == 0x04);
	keymap_event(&ks, SDLK_RSHIFT, 0, 1, &b, &ch); CHECK(b == 0x04);
	keymap_event(&ks, SDLK_HOME, 0, 1, &b, &ch); CHECK(b == 0x08);
	keymap_event(&ks, SDLK_UP, 0, 1, &b, &ch); CHECK(b == 0x10);
	keymap_event(&ks, SDLK_DOWN, 0, 1, &b, &ch); CHECK(b == 0x20);
	keymap_event(&ks, SDLK_LEFT, 0, 1, &b, &ch); CHECK(b == 0x40);
	keymap_event(&ks, SDLK_RIGHT, 0, 1, &b, &ch); CHECK(b == 0x80);
	CHECK(ks.buttons == 0xfe);
	/* banned-as-shortcut keys do nothing for the ROM either */
	keymap_event(&ks, SDLK_F1, 0, 1, &b, &ch); CHECK(b == 0 && ch == 0);
	keymap_event(&ks, SDLK_PAGEUP, 0, 1, &b, &ch); CHECK(b == 0 && ch == 0);
}

static void tab_is_char_9(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_TAB, 0, 1, &b, &ch);
	CHECK(ch == 0x09 && b == 0);
	keymap_event(&ks, SDLK_TAB, 0, 0, &b, &ch);
	CHECK(ch == 0);
	keymap_event(&ks, SDLK_RETURN, 0, 1, &b, &ch); CHECK(ch == 0x0d);
	keymap_event(&ks, SDLK_ESCAPE, 0, 1, &b, &ch); CHECK(ch == 0x1b);
	keymap_event(&ks, SDLK_BACKSPACE, 0, 1, &b, &ch); CHECK(ch == 0x08);
	keymap_event(&ks, SDLK_DELETE, 0, 1, &b, &ch); CHECK(ch == 0x7f);
}

static void ctrl_letter(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LCTRL, SDL_KMOD_LCTRL, 1, &b, &ch);
	keymap_event(&ks, SDLK_S, SDL_KMOD_LCTRL, 1, &b, &ch);
	CHECK(ch == 's' && ks.buttons == 0x01);
	/* without Ctrl a letter gives no key here (text input supplies it) */
	ks.buttons = 0;
	keymap_event(&ks, SDLK_S, 0, 1, &b, &ch);
	CHECK(ch == 0);
}

static void keymap_release_on_focus_loss(void)
{
	KeyState ks = {0}; ux_u8 b, ch;
	keymap_event(&ks, SDLK_LCTRL, 0, 1, &b, &ch);
	keymap_event(&ks, SDLK_UP, 0, 1, &b, &ch);
	CHECK(keymap_release_all(&ks) == 0x11);
	CHECK(ks.buttons == 0);
	CHECK(keymap_release_all(&ks) == 0);
}

int main(void)
{
	int before;
#define T(f) do { total++; before = failed; f(); if (failed != before) printf("FAIL %s\n", #f); } while (0)
	T(rctrl_toggles_not_A); T(lctrl_is_A); T(other_buttons); T(tab_is_char_9);
	T(ctrl_letter); T(keymap_release_on_focus_loss);
#undef T
	printf("keymap: %d passed, %d failed\n", total - bad_tests, bad_tests);
	return failed ? 1 : 0;
}
