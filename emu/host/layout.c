#include "layout.h"

void layout_window_size(int debugger_open, int sw, int sh, int zoom, int *w, int *h)
{
	*w = sw * zoom;
	*h = sh * zoom;
	if (debugger_open) {
		if (*w < DBG_LEFT_MIN_W) *w = DBG_LEFT_MIN_W;
		*w += DBG_PANEL_W;
		*h += DBG_MENU_H + DBG_BOTTOM_H;
		if (*h < DBG_MIN_H) *h = DBG_MIN_H;
	}
}

int layout_fit_zoom(int debugger_open, int sw, int sh, int want, int max_w, int max_h)
{
	int z, w, h;

	for (z = want; z > 1; z--) {
		layout_window_size(debugger_open, sw, sh, z, &w, &h);
		if (w <= max_w && h <= max_h) break;
	}
	return z < 1 ? 1 : z;
}

LayoutView layout_view(int debugger_open, int sw, int sh, int win_w, int win_h, int integer)
{
	LayoutView r;
	float aw = (float)win_w, ah = (float)win_h, s;

	if (debugger_open) {
		aw -= DBG_PANEL_W;
		ah -= DBG_MENU_H + DBG_BOTTOM_H;
	}
	if (sw < 1 || sh < 1 || aw < 1 || ah < 1) {
		r.x = r.y = r.w = r.h = 0;
		r.scale = 1;
		return r;
	}
	s = aw / (float)sw;
	if (ah / (float)sh < s) s = ah / (float)sh;
	if (integer && s >= 1) s = (float)(int)s;
	r.scale = s;
	r.w = (float)sw * s;
	r.h = (float)sh * s;
	if (debugger_open) {
		r.x = 0;
		r.y = DBG_MENU_H;
	} else {
		r.x = (float)(int)((aw - r.w) / 2);
		r.y = (float)(int)((ah - r.h) / 2);
	}
	return r;
}
