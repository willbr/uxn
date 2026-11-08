#include "input_queue.h"

void iq_init(InputQueue *q)
{
	q->head = 0;
	q->count = 0;
	q->overflowed = 0;
}

int iq_push(InputQueue *q, InputEv ev)
{
	if (q->count >= IQ_CAP) {
		q->overflowed = 1;
		return 0;
	}
	q->ev[(q->head + q->count) % IQ_CAP] = ev;
	q->count++;
	return 1;
}

int iq_pop(InputQueue *q, InputEv *ev)
{
	if (!q->count) return 0;
	*ev = q->ev[q->head];
	q->head = (q->head + 1) % IQ_CAP;
	q->count--;
	return 1;
}

ux_u8 iq_stuck_buttons(ux_u8 rom, ux_u8 held)
{
	return (ux_u8)(rom & ~held);
}
