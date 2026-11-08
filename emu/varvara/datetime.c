/* Datetime device (0xc0-0xcf): local time, read on demand. */
#include <stddef.h>
#include <string.h>
#include <time.h>
#include "varvara.h"

ux_u8 datetime_dei(Varvara *v, ux_u8 port)
{
	time_t now;
	struct tm *t;
	struct tm zero;
	int x;

	if (port > 0xca) return v->u.dev[port];
	now = time(NULL);
	t = localtime(&now);
	if (t == NULL) {
		memset(&zero, 0, sizeof zero);
		t = &zero;
	}
	switch (port) {
	case 0xc0: x = t->tm_year + 1900; v->u.dev[0xc1] = (ux_u8)x; return (ux_u8)(x >> 8);
	case 0xc2: return (ux_u8)t->tm_mon;
	case 0xc3: return (ux_u8)t->tm_mday;
	case 0xc4: return (ux_u8)t->tm_hour;
	case 0xc5: return (ux_u8)t->tm_min;
	case 0xc6: return (ux_u8)t->tm_sec;
	case 0xc7: return (ux_u8)t->tm_wday;
	case 0xc8: x = t->tm_yday; v->u.dev[0xc9] = (ux_u8)x; return (ux_u8)(x >> 8);
	case 0xca: return (ux_u8)t->tm_isdst;
	}
	return v->u.dev[port];
}

void datetime_deo(Varvara *v, ux_u8 port)
{
	(void)v;
	(void)port;
}

const char *datetime_port_name(ux_u8 port)
{
	static const char *const names[11] = {
		"Datetime/year", "Datetime/year", "Datetime/month", "Datetime/day",
		"Datetime/hour", "Datetime/minute", "Datetime/second", "Datetime/dotw",
		"Datetime/doty", "Datetime/doty", "Datetime/isdst"
	};
	return (port & 0x0f) < 11 ? names[port & 0x0f] : NULL;
}
