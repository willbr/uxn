/* Varvara: the device layer on top of the uxn core. C89 plus libc. */
#ifndef VARVARA_H
#define VARVARA_H

#include "uxn.h"

typedef struct Varvara {
	Uxn u;
	ux_u8 ram[0x10000];
	void (*on_console)(void *ctx, int fd, int c); /* fd 1 (write) or 2 (error) */
	void *ctx;
	unsigned long max_steps; /* per uxn_eval call; 0 = unlimited */
	/* device state added by later tasks */
} Varvara;

/* Zero the machine and wire dei/deo. Returns 0. */
int varvara_init(Varvara *v);
/* Load a ROM at 0x0100. Returns 0, or nonzero after printing one line to
 * stderr (missing, empty, or longer than 0xff00 bytes). */
int varvara_load(Varvara *v, const char *path);
/* Run the ROM's reset vector, then deliver console arguments, with argc/argv
 * laid out as for uxnmin: argv[0] is the program, argv[1] the ROM, arguments
 * start at argv[2]. Returns 0 if the reset vector reached BRK, nonzero if it
 * hit max_steps or a fault (a fault is cleared). */
int varvara_boot(Varvara *v, int argc, char **argv, unsigned long max_steps);
/* Deliver one console event: type 1 stdin byte, 2 argument byte, 3 argument
 * end, 4 last argument end / end of input. Runs the console vector if set. */
void varvara_console_input(Varvara *v, int c, int type);
int varvara_halted(const Varvara *v);    /* dev[0x0f] != 0 */
int varvara_exit_code(const Varvara *v); /* dev[0x0f] & 0x7f */
/* "Console/write" style name for a device port, or NULL. */
const char *varvara_port_name(ux_u8 port);

/* Per-device handlers, dispatched on (port & 0xf0). Internal. */
ux_u8 system_dei(Varvara *v, ux_u8 port);
void system_deo(Varvara *v, ux_u8 port);
ux_u8 console_dei(Varvara *v, ux_u8 port);
void console_deo(Varvara *v, ux_u8 port);
const char *system_port_name(ux_u8 port);
const char *console_port_name(ux_u8 port);

#endif
