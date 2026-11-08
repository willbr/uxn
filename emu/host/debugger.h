/* cimgui debugger panels over a DebugSession. The host owns the machine and
 * the run state; debugger_frame() only draws and reports what was clicked. */
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <SDL3/SDL.h>
#include "varvara.h"
#include "session.h"

#include "layout.h"

#define DBG_CON_SIZE 16384

typedef enum {
	DA_NONE, DA_PAUSE, DA_CONTINUE, DA_STEP, DA_STEP_OVER,
	DA_RESET, DA_OPEN, DA_RELOAD, DA_ZOOM, DA_INTEGER
} DebugActionKind;

typedef struct DebugAction {
	DebugActionKind kind;
	int zoom; /* DA_ZOOM: 1..3 */
} DebugAction;

/* What the panels show, filled in by the host each frame. */
typedef struct DebugInfo {
	int paused;
	int in_vector;   /* a vector is suspended at pc (otherwise pc is the next Screen vector) */
	unsigned pc;
	int zoom;        /* the Zoom menu's choice (sets the window size) */
	int integer;     /* integer scaling on */
	LayoutView view; /* where the screen is drawn this frame */
	int win_w, win_h; /* render output size */
	int can_reload;  /* a ROM path is known */
} DebugInfo;

typedef struct DebuggerUI {
	DebugSession sess;
	char con[DBG_CON_SIZE]; /* captured Console output (stdout and stderr), newest last */
	int con_len;
	int relayout;           /* force the default panel positions for one frame */
	unsigned mem_addr;
	char mem_text[8];
} DebuggerUI;


/* Creates the imgui context and backends. Returns 0 on success. */
int debugger_init(SDL_Window *win, SDL_Renderer *ren);
void debugger_shutdown(void);
void debugger_event(const SDL_Event *e);
/* Append one Console byte to the captured output. */
void debugger_console_put(DebuggerUI *ui, int fd, int c);
/* Draw one imgui frame onto ren (after the Varvara screen). */
DebugAction debugger_frame(DebuggerUI *ui, SDL_Renderer *ren, const Varvara *v, const DebugInfo *info);

#endif
