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

KeyAction keymap_event(KeyState *ks, SDL_Keycode key, SDL_Keymod mod, int down,
                       ux_u8 *button_changed, ux_u8 *ch)
{
	ux_u8 mask;

	*button_changed = 0;
	*ch = 0;
	if (key == SDLK_RCTRL) return down ? KEY_TOGGLE_DEBUGGER : KEY_NONE;
	mask = button_for(key);
	if (mask) {
		if (down) ks->buttons |= mask;
		else ks->buttons &= (ux_u8)~mask;
		*button_changed = mask;
		return KEY_NONE;
	}
	if (!down) return KEY_NONE;
	switch (key) {
	case SDLK_TAB: *ch = 0x09; break;
	case SDLK_RETURN: case SDLK_KP_ENTER: *ch = 0x0d; break;
	case SDLK_ESCAPE: *ch = 0x1b; break;
	case SDLK_BACKSPACE: *ch = 0x08; break;
	case SDLK_DELETE: *ch = 0x7f; break;
	default:
		/* Left Ctrl + letter: the lowercase letter, as uxn11 sends it */
		if (key >= 'a' && key <= 'z' && ((ks->buttons & 0x01) || (mod & SDL_KMOD_LCTRL)))
			*ch = (ux_u8)key;
		break;
	}
	return KEY_NONE;
}

ux_u8 keymap_release_all(KeyState *ks)
{
	ux_u8 was = ks->buttons;
	ks->buttons = 0;
	return was;
}
