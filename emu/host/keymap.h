/* Keyboard to Varvara Controller mapping. Needs the SDL3 headers only; calls
 * no SDL functions (the host passes in what it asked SDL). Right Ctrl is
 * reserved for the host (debugger toggle) and never reaches the ROM. */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL3/SDL_keycode.h>
#include "uxn.h"

/* KeyState.chord bits: the chord keys held down. */
#define KS_LCTRL 0x01
#define KS_LALT  0x02
#define KS_RALT  0x04

typedef struct KeyState {
	ux_u8 buttons; /* Controller buttons held */
	ux_u8 chord;   /* KS_* keys held */
	/* Set by the host when the keyboard layout has AltGr (Right Alt types
	 * characters, e.g. AltGr+Q = '@' on a German layout). Right Alt is then
	 * AltGr: not button B and never a chord; its characters come from text
	 * input. Otherwise Right Alt is Alt, as in uxn11. */
	int altgr;
} KeyState;
typedef enum { KEY_NONE, KEY_TOGGLE_DEBUGGER } KeyAction;

/* Updates *ks for one key event (not an auto-repeat; see keymap_repeat).
 * typed is the key's character with only Shift applied (the host passes
 * SDL_GetKeyFromScancode(scancode, mod & SDL_KMOD_SHIFT, false); 0 means use
 * key). Sets *button_changed to the Controller button mask that changed (0
 * if none) and *ch to the Controller/key byte to send (0 if none; only ever
 * set on key down):
 *   - Tab 0x09, Return/Enter 0x0d, Escape 0x1b, Backspace 0x08, Delete 0x7f;
 *   - while Left Ctrl or Alt is held (a chord), typed, if 0x20..0x7e, as
 *     uxn11 sends the keysym: Ctrl+s 's', Ctrl+Shift+V 'V', Ctrl+/ '/',
 *     Alt+a 'a'. Not while AltGr is held.
 * Other printable characters come from SDL_EVENT_TEXT_INPUT (see
 * keymap_text_allowed). */
KeyAction keymap_event(KeyState *ks, SDL_Keycode key, SDL_Keycode typed, SDL_Keymod mod, int down,
                       ux_u8 *button_changed, ux_u8 *ch);
/* For an auto-repeated key down: what to send again. *button gets the
 * direction or Start (Home) button the key holds (a repeat runs the
 * Controller vector again, as uxn11 does on X11 autorepeat); modifier
 * buttons (A, B, Select) are never repeated. *ch gets the byte a key down
 * would send. Right Ctrl does nothing. ks is not changed. */
void keymap_repeat(const KeyState *ks, SDL_Keycode key, SDL_Keycode typed, SDL_Keymod mod,
                   ux_u8 *button, ux_u8 *ch);
/* Whether SDL_EVENT_TEXT_INPUT should reach the ROM: not while a chord
 * modifier is held (mod: SDL_GetModState()), since keymap_event already
 * sent that chord's character (Ctrl+Space would otherwise arrive twice).
 * AltGr text is allowed. */
int keymap_text_allowed(const KeyState *ks, SDL_Keymod mod);
/* Clears every held button and chord key; returns the button mask that was down. */
ux_u8 keymap_release_all(KeyState *ks);

#endif
