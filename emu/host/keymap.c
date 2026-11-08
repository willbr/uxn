#include "keymap.h"

static ux_u8 button_for(SDL_Keycode key)
{
	switch (key) {
	case SDLK_LCTRL: return 0x01;
	case SDLK_LALT: case SDLK_RALT: return 0x02;
	case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x04;
	case SDLK_HOME: return 0x08;
	case SDLK_UP: return 0x10;
	case SDLK_DOWN: return 0x20;
	case SDLK_LEFT: return 0x40;
	case SDLK_RIGHT: return 0x80;
	default: return 0;
	}
}

static ux_u8 chord_bit(SDL_Keycode key)
{
	switch (key) {
	case SDLK_LCTRL: return KS_LCTRL;
	case SDLK_LALT: return KS_LALT;
	case SDLK_RALT: return KS_RALT;
	default: return 0;
	}
}

/* Right Alt held on a layout with AltGr (also when Windows reports it as
 * Left Ctrl + Right Alt). */
static int altgr_held(const KeyState *ks, SDL_Keymod mod)
{
	return ks->altgr && ((ks->chord & KS_RALT) || (mod & SDL_KMOD_RALT));
}

/* Left Ctrl or Alt held (Right Alt only when it is not AltGr). mod also
 * counts, for a modifier pressed before the window had focus. Right Ctrl
 * never counts: it is the host's. */
static int chord_held(const KeyState *ks, SDL_Keymod mod)
{
	if (altgr_held(ks, mod)) return 0;
	return (ks->chord & (KS_LCTRL | KS_LALT | KS_RALT)) != 0
		|| (mod & (SDL_KMOD_LCTRL | SDL_KMOD_ALT)) != 0;
}

/* The Controller/key byte for a non-button key going down, or 0. */
static ux_u8 char_for(const KeyState *ks, SDL_Keycode key, SDL_Keycode typed, SDL_Keymod mod)
{
	switch (key) {
	case SDLK_TAB: return 0x09;
	case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0d;
	case SDLK_ESCAPE: return 0x1b;
	case SDLK_BACKSPACE: return 0x08;
	case SDLK_DELETE: return 0x7f;
	default: break;
	}
	/* A chord: the character with Shift applied, as uxn11 sends the keysym
	 * for keys below 0x80 (Ctrl+s 's', Ctrl+Shift+V 'V', Ctrl+/ '/') */
	if (!typed) typed = key;
	if (chord_held(ks, mod) && typed >= 0x20 && typed <= 0x7e) return (ux_u8)typed;
	return 0;
}

KeyAction keymap_event(KeyState *ks, SDL_Keycode key, SDL_Keycode typed, SDL_Keymod mod, int down,
                       ux_u8 *button_changed, ux_u8 *ch)
{
	ux_u8 mask, bit;

	*button_changed = 0;
	*ch = 0;
	if (key == SDLK_RCTRL) return down ? KEY_TOGGLE_DEBUGGER : KEY_NONE;
	bit = chord_bit(key);
	if (bit) {
		if (down) ks->chord |= bit;
		else ks->chord &= (ux_u8)~bit;
	}
	if (key == SDLK_RALT && ks->altgr) return KEY_NONE; /* AltGr: not button B */
	mask = button_for(key);
	if (mask) {
		if (down) ks->buttons |= mask;
		else ks->buttons &= (ux_u8)~mask;
		*button_changed = mask;
		return KEY_NONE;
	}
	if (down) *ch = char_for(ks, key, typed, mod);
	return KEY_NONE;
}

void keymap_repeat(const KeyState *ks, SDL_Keycode key, SDL_Keycode typed, SDL_Keymod mod,
                   ux_u8 *button, ux_u8 *ch)
{
	ux_u8 mask;

	*button = 0;
	*ch = 0;
	if (key == SDLK_RCTRL || (key == SDLK_RALT && ks->altgr)) return;
	mask = button_for(key);
	if (mask) {
		*button = (ux_u8)(mask & 0xf8); /* Start and the directions, not A, B, Select */
		return;
	}
	*ch = char_for(ks, key, typed, mod);
}

int keymap_text_allowed(const KeyState *ks, SDL_Keymod mod)
{
	return !chord_held(ks, mod);
}

ux_u8 keymap_release_all(KeyState *ks)
{
	ux_u8 was = ks->buttons;
	ks->buttons = 0;
	ks->chord = 0;
	return was;
}
