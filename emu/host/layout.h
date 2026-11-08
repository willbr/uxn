/* Window layout: where the Varvara screen goes and how big the window is.
 * Pure C, no SDL calls, so it is unit-tested (tests/layout_test.c). */
#ifndef LAYOUT_H
#define LAYOUT_H

/* With the debugger open the screen sits below the menu bar, the panels in a
 * column to its right and under it. */
#define DBG_MENU_H 20
#define DBG_PANEL_W 560
#define DBG_LEFT_MIN_W 480 /* the Memory panel needs this much width under a small screen */
#define DBG_BOTTOM_H 260
#define DBG_MIN_H 700

/* The screen's place in the window, in render pixels. */
typedef struct LayoutView {
	float x, y, w, h;
	float scale;
} LayoutView;

/* Window size for a sw x sh screen at a whole-number zoom. */
void layout_window_size(int debugger_open, int sw, int sh, int zoom, int *w, int *h);

/* The largest zoom from want down to 1 whose window fits in max_w x max_h;
 * 1 when even that does not fit. */
int layout_fit_zoom(int debugger_open, int sw, int sh, int want, int max_w, int max_h);

/* Fit a sw x sh screen into a win_w x win_h window, keeping its aspect.
 * With the debugger open it fills the area left of and above the panels,
 * top-left aligned; otherwise it is centred. integer: a whole-number scale
 * (crisp pixels) whenever 1x fits; below 1x the screen always shrinks
 * smoothly so none of it is cut off. */
LayoutView layout_view(int debugger_open, int sw, int sh, int win_w, int win_h, int integer);

#endif
