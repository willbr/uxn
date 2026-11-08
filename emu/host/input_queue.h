/* Bounded FIFO of already-mapped input calls, for use while a vector is
 * suspended at the step limit. Pure C, no SDL. */
#ifndef INPUT_QUEUE_H
#define INPUT_QUEUE_H

#include "uxn.h"

#define IQ_CAP 256

typedef enum { IQ_BUTTON, IQ_KEY, IQ_MOUSE_MOVE, IQ_MOUSE_BUTTON, IQ_SCROLL } IqKind;

typedef struct InputEv {
	IqKind kind;
	int a, b; /* BUTTON: mask, down. KEY: byte. MOVE: x, y. MOUSE_BUTTON: mask, down. SCROLL: dx, dy */
} InputEv;

typedef struct InputQueue {
	InputEv ev[IQ_CAP];
	int head, count;
	int overflowed; /* an event was dropped since the flag was last cleared */
} InputQueue;

void iq_init(InputQueue *q);
/* Returns 1 if queued, 0 if the queue was full (event dropped, overflowed set). */
int iq_push(InputQueue *q, InputEv ev);
/* Returns 1 and fills *ev, or 0 if empty. */
int iq_pop(InputQueue *q, InputEv *ev);
/* Controller buttons the ROM has down (rom) that the host no longer holds. */
ux_u8 iq_stuck_buttons(ux_u8 rom, ux_u8 held);

#endif
