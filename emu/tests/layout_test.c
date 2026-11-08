/* Window layout unit tests: pure C, no SDL. */
#include <stdio.h>
#include "layout.h"

static int failed = 0, total = 0, bad_tests = 0;
#define CHECK(c) do { if (!(c)) { printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); failed++; } } while (0)

/* Left asks for 1360x716; 2x (2720x1432) is wider than a 2560x1440 display. */
static void left_2x_falls_back_to_1x(void)
{
	CHECK(layout_fit_zoom(0, 1360, 716, 2, 2560, 1390) == 1);
	CHECK(layout_fit_zoom(0, 512, 320, 2, 2560, 1390) == 2);
	CHECK(layout_fit_zoom(0, 512, 320, 3, 2560, 1390) == 3);
	CHECK(layout_fit_zoom(0, 4000, 4000, 3, 2560, 1390) == 1); /* nothing fits: 1 */
	CHECK(layout_fit_zoom(1, 1360, 716, 1, 2560, 1390) == 1);
}

static void integer_scale_is_whole(void)
{
	LayoutView r = layout_view(0, 512, 320, 1300, 700, 1); /* fits 2.18x */
	CHECK(r.scale == 2 && r.w == 1024 && r.h == 640);
	CHECK(r.x == 138 && r.y == 30); /* centred */
	r = layout_view(0, 512, 320, 1300, 700, 0);
	CHECK(r.scale > 2.18f && r.scale < 2.19f && r.h == 700);
}

static void below_1x_shrinks_even_with_integer(void)
{
	LayoutView r = layout_view(0, 1360, 716, 1000, 700, 1);
	CHECK(r.scale < 1 && r.w <= 1000 && r.h <= 700);
	CHECK(r.w > 999); /* width-bound */
}

static void debugger_area_top_left(void)
{
	int w, h;
	LayoutView r;

	layout_window_size(1, 512, 320, 2, &w, &h);
	r = layout_view(1, 512, 320, w, h, 1);
	CHECK(r.x == 0 && r.y == DBG_MENU_H && r.scale == 2);
}

static void degenerate_sizes(void)
{
	LayoutView r = layout_view(1, 512, 320, 100, 100, 1); /* window smaller than the panels */
	CHECK(r.w == 0 && r.h == 0 && r.scale == 1);
	r = layout_view(0, 0, 0, 800, 600, 1);
	CHECK(r.w == 0 && r.scale == 1);
}

int main(void)
{
	int before;
#define T(f) do { total++; before = failed; f(); if (failed != before) { printf("FAIL %s\n", #f); bad_tests++; } } while (0)
	T(left_2x_falls_back_to_1x); T(integer_scale_is_whole); T(below_1x_shrinks_even_with_integer);
	T(debugger_area_top_left); T(degenerate_sizes);
#undef T
	printf("layout: %d passed, %d failed\n", total - bad_tests, bad_tests);
	return failed ? 1 : 0;
}
