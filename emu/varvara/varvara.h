/* Varvara: the device layer on top of the uxn core. C89 plus libc. */
#ifndef VARVARA_H
#define VARVARA_H

#include <stdio.h>
#include "uxn.h"

struct FsDir;

/* One File device (0xa0 or 0xb0). */
typedef struct VarvaraFile {
	FILE *f;
	struct FsDir *dir;
	int state;     /* 0 idle, 1 file read, 2 file write, 3 dir read, 4 dir write */
	int has_name;  /* File/name has been written */
	unsigned name; /* RAM address of the NUL-terminated path */
} VarvaraFile;

typedef struct Varvara {
	Uxn u;
	ux_u8 ram[0x10000];
	void (*on_console)(void *ctx, int fd, int c); /* fd 1 (write) or 2 (error) */
	void *ctx;
	/* Step limit per uxn_eval call made by the input functions (varvara_button,
	 * varvara_key, varvara_mouse_*, varvara_console_input); 0 = unlimited.
	 * varvara_boot and varvara_frame take their own max_steps argument and do
	 * not use this field. The SDL host sets both to 2,000,000. */
	unsigned long max_steps;
	/* Set by the last input function (button, key, mouse): nonzero if its
	 * vector stopped at max_steps. The vector is then suspended with u.pc
	 * as the resume address; a host may continue it with uxn_eval. */
	int limit_hit;
	/* Optional vector runner. NULL means plain uxn_eval. Every vector the
	 * input functions, varvara_frame and varvara_console_input start goes
	 * through it as a fresh start at pc; a debugger host installs one that
	 * stops at breakpoints (it may return UXN_LIMIT early, leaving u.pc as the
	 * resume address). limit_hit is set when the vector did not finish
	 * (UXN_LIMIT from the step budget or the runner). */
	int (*run)(struct Varvara *v, ux_u16 pc, unsigned long max_steps);
	/* Port clears owed after a vector that stopped early: 1 = Controller/key,
	 * 2 = Mouse scroll. Applied by varvara_vector_done. */
	int pending_clear;
	VarvaraFile file[2];
	ux_u8 *banks; /* banks 1-15 (15 x 64K), NULL if allocation failed; freed by varvara_close */
	int has_root;
	char root[1024]; /* absolute normalized confinement root, when has_root */
	/* Screen (0x20). fg and bg hold one colour index (0-3) per pixel,
	 * width*height bytes each, row-major. The size is kept in 1..1024 x
	 * 1..1024; fg and bg are NULL (and the size 0x0) only if allocation
	 * failed. Freed by varvara_close. */
	int width, height;
	ux_u8 *fg, *bg;
	int scr_x, scr_y;   /* Screen/x, Screen/y registers (signed) */
	unsigned scr_addr;  /* Screen/addr register */
	int scr_auto_x, scr_auto_y; /* Screen/auto: step in pixels (0 or 8) */
	int scr_auto_a;     /* Screen/auto: address step per 1bpp tile (0 or 8) */
	int scr_auto_len;   /* Screen/auto: extra sprites per write (0-15) */
} Varvara;

/* Zero the machine, wire dei/deo, and allocate the memory banks and the
 * default 512x320 screen. Returns 0. */
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
/* Call when a vector that stopped early (limit_hit) has finally finished:
 * clears Controller/key and Mouse scroll, which varvara_key and
 * varvara_mouse_scroll only clear themselves after a vector that completed.
 * Harmless to call at any time. */
void varvara_vector_done(Varvara *v);
/* Run a vector through v->run (or uxn_eval). Internal. */
int varvara_run(Varvara *v, ux_u16 pc, unsigned long max_steps);
int varvara_halted(const Varvara *v);    /* dev[0x0f] != 0 */
int varvara_exit_code(const Varvara *v); /* dev[0x0f] & 0x7f */
/* "Console/write" style name for a device port, or NULL. */
const char *varvara_port_name(ux_u8 port);

/* Confine every File path to dir. Returns 0 if dir exists, nonzero (and no
 * root set) otherwise. Without a root, paths are used as given. */
int varvara_set_root(Varvara *v, const char *dir);
/* Close any open File device handles and free the extra memory banks and
 * the screen layers. Call before exiting or removing files; varvara_init
 * may follow. */
void varvara_close(Varvara *v);

/* Run the Screen vector once, if set (one frame). Returns 0 if there is no
 * vector or it reached BRK, nonzero if it hit max_steps or a fault (a fault
 * is cleared). */
int varvara_frame(Varvara *v, unsigned long max_steps);
/* Composite the screen (fg over bg, fg colour 0 transparent) through the
 * System palette into rgba: width*height*4 bytes, R G B A (A = 255). */
void varvara_render(const Varvara *v, ux_u8 *rgba);
/* Input events. Each stores the port value and runs that device's vector,
 * if set, with v->max_steps (a fault is cleared), as uxn11 does. */
/* Controller/button: 0x01 A (Ctrl), 0x02 B (Alt), 0x04 Select (Shift),
 * 0x08 Start (Home), 0x10 Up, 0x20 Down, 0x40 Left, 0x80 Right. A mask of 0
 * does nothing. */
void varvara_button(Varvara *v, ux_u8 mask, int down);
/* Controller/key: the byte is set for the vector and cleared after it. 0
 * does nothing. */
void varvara_key(Varvara *v, ux_u8 ch);
/* Mouse/x, Mouse/y in screen pixels. */
void varvara_mouse_move(Varvara *v, int x, int y);
/* Mouse/state: 0x01 left, 0x02 middle, 0x04 right. */
void varvara_mouse_button(Varvara *v, ux_u8 mask, int down);
/* One scroll event, in X11/SDL wheel sense (dy > 0 is away from the user).
 * As in uxn11, Mouse/scrollx gets dx and Mouse/scrolly gets -dy for the
 * vector; both are zeroed after it. */
void varvara_mouse_scroll(Varvara *v, int dx, int dy);

/* Per-device handlers, dispatched on (port & 0xf0). Internal. */
ux_u8 system_dei(Varvara *v, ux_u8 port);
void system_deo(Varvara *v, ux_u8 port);
ux_u8 console_dei(Varvara *v, ux_u8 port);
void console_deo(Varvara *v, ux_u8 port);
ux_u8 file_dei(Varvara *v, ux_u8 port);
void file_deo(Varvara *v, ux_u8 port);
ux_u8 datetime_dei(Varvara *v, ux_u8 port);
void datetime_deo(Varvara *v, ux_u8 port);
const char *file_port_name(ux_u8 port);
const char *datetime_port_name(ux_u8 port);
const char *system_port_name(ux_u8 port);
const char *console_port_name(ux_u8 port);
ux_u8 screen_dei(Varvara *v, ux_u8 port);
void screen_deo(Varvara *v, ux_u8 port);
const char *screen_port_name(ux_u8 port);
/* Allocate the default 512x320 screen (called by varvara_init) / free it. */
void screen_init(Varvara *v);
void screen_free(Varvara *v);
const char *controller_port_name(ux_u8 port);
const char *mouse_port_name(ux_u8 port);

#endif
