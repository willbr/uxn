/* Keyboard to Varvara Controller mapping. Needs the SDL3 headers only; calls
 * no SDL functions. Right Ctrl is reserved for the host (debugger toggle) and
 * never reaches the ROM. */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL3/SDL_keycode.h>
#include "uxn.h"

typedef struct KeyState { ux_u8 buttons; } KeyState;
typedef enum { KEY_NONE, KEY_TOGGLE_DEBUGGER } KeyAction;

/* Updates *ks for one key event. Sets *button_changed to the Controller
 * button mask that changed (0 if none) and *ch to the Controller/key byte
 * to send (0 if none; only ever set on key down). Printable characters are
 * not produced here: they come from SDL_EVENT_TEXT_INPUT. */
KeyAction keymap_event(KeyState *ks, SDL_Keycode key, SDL_Keymod mod, int down,
                       ux_u8 *button_changed, ux_u8 *ch);
/* Clears every held button; returns the mask that was down. */
ux_u8 keymap_release_all(KeyState *ks);

#endif
