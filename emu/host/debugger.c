/* cimgui debugger: menu bar, CPU, disassembly, memory and console panels.
 * Draws state and reports clicks as a DebugAction; the host acts on them.
 *
 * Shortcuts (only while an imgui window has focus; Ctrl + letter, so no
 * function keys, Insert, Page Up/Down, End or arrows):
 *   Ctrl+S step   Ctrl+N step over   Ctrl+G continue   Ctrl+P pause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_SDL3
#define CIMGUI_USE_SDLRENDERER3
#include "cimgui.h"
#include "cimgui_impl.h"
#include "imgui_backends.h"

#include "debugger.h"
#include "disasm.h"

static ImGuiContext *g_ctx;

static ImVec2 v2(float x, float y)
{
	ImVec2 v;
	v.x = x;
	v.y = y;
	return v;
}

static ImVec4 v4(float x, float y, float z, float w)
{
	ImVec4 v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

int debugger_init(SDL_Window *win, SDL_Renderer *ren)
{
	g_ctx = igCreateContext(NULL);
	if (!g_ctx) return 1;
	igGetIO_Nil()->IniFilename = NULL; /* no imgui.ini in the cwd */
	if (!ImGui_ImplSDL3_InitForSDLRenderer(win, ren)) return 1;
	if (!ImGui_ImplSDLRenderer3_Init(ren)) return 1;
	return 0;
}

void debugger_shutdown(void)
{
	if (!g_ctx) return;
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	igDestroyContext(g_ctx);
	g_ctx = NULL;
}

void debugger_event(const SDL_Event *e)
{
	if (g_ctx) ImGui_ImplSDL3_ProcessEvent(e);
}

void debugger_window_size(int open, int sw, int sh, int zoom, int *w, int *h)
{
	*w = sw * zoom;
	*h = sh * zoom;
	if (open) {
		if (*w < DBG_LEFT_MIN_W) *w = DBG_LEFT_MIN_W;
		*w += DBG_PANEL_W;
		*h += DBG_MENU_H + DBG_BOTTOM_H;
		if (*h < DBG_MIN_H) *h = DBG_MIN_H;
	}
}

void debugger_console_put(DebuggerUI *ui, int fd, int c)
{
	(void)fd;
	if (ui->con_len >= DBG_CON_SIZE - 1) {
		int keep = DBG_CON_SIZE / 2;
		memmove(ui->con, ui->con + ui->con_len - keep, (size_t)keep);
		ui->con_len = keep;
	}
	ui->con[ui->con_len++] = (char)c;
	ui->con[ui->con_len] = '\0';
}

static void place(const DebuggerUI *ui, float x, float y, float w, float h)
{
	ImGuiCond cond = ui->relayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

	igSetNextWindowPos(v2(x, y), cond, v2(0, 0));
	igSetNextWindowSize(v2(w, h), cond);
}

static void stack_text(const char *label, const UxStack *st)
{
	char buf[256 * 3 + 1];
	int i, n = 0;

	for (i = 0; i < st->ptr; i++) n += sprintf(buf + n, "%02x ", (unsigned)st->dat[i]);
	buf[n] = '\0';
	igText("%s (%d):", label, (int)st->ptr);
	igSameLine(0, -1);
	igTextWrapped("%s", n ? buf : "empty");
}

static void cpu_panel(DebuggerUI *ui, const Varvara *v, const DebugInfo *info, DebugAction *act)
{
	igText("PC %04x   %s", info->pc, info->paused ? "paused" : "running");
	if (info->in_vector) igText("A vector is suspended here.");
	else igText("No vector running: PC is the Screen vector.");
	igSeparator();
	igBeginDisabled(info->paused);
	if (igButton("Pause (Ctrl+P)", v2(0, 0))) act->kind = DA_PAUSE;
	igEndDisabled();
	igSameLine(0, -1);
	igBeginDisabled(!info->paused);
	if (igButton("Continue (Ctrl+G)", v2(0, 0))) act->kind = DA_CONTINUE;
	igEndDisabled();
	if (igButton("Step (Ctrl+S)", v2(0, 0))) act->kind = DA_STEP;
	igSameLine(0, -1);
	if (igButton("Step over (Ctrl+N)", v2(0, 0))) act->kind = DA_STEP_OVER;
	igSameLine(0, -1);
	if (igButton("Reset", v2(0, 0))) act->kind = DA_RESET;
	igSeparator();
	stack_text("Working stack", &v->u.wst);
	stack_text("Return stack", &v->u.rst);
	igSeparator();
	igText("%d breakpoint(s)", ui->sess.nbp);
	igSameLine(0, -1);
	if (igButton("Clear all", v2(0, 0))) {
		memset(ui->sess.bp, 0, sizeof ui->sess.bp);
		ui->sess.nbp = 0;
	}
}

#define DIS_LINES 32

static void disasm_panel(DebuggerUI *ui, const Varvara *v, const DebugInfo *info)
{
	ux_u16 pc = (ux_u16)info->pc;
	ux_u16 base = (ux_u16)(pc - 10), a;
	int i, seen = 0;

	/* Start a little before PC. Code can't be disassembled backwards, so if
	 * this start never lands on PC, start at PC itself. */
	for (a = base, i = 0; i < 16; i++) {
		char t[32];

		if (a == pc) { seen = 1; break; }
		a = (ux_u16)(a + uxn_disasm(v->ram, a, t, sizeof t));
	}
	if (!seen) base = pc;
	igText("Click a line to toggle a breakpoint.");
	for (a = base, i = 0; i < DIS_LINES; i++) {
		char text[32], bytes[16], line[96];
		int len = uxn_disasm(v->ram, a, text, sizeof text), k, bp = dbg_has_breakpoint(&ui->sess, a);

		bytes[0] = '\0';
		for (k = 0; k < len; k++) sprintf(bytes + k * 3, "%02x ", (unsigned)v->ram[(ux_u16)(a + k)]);
		snprintf(line, sizeof line, "%c %04x  %-9s %s", bp ? '*' : ' ', (unsigned)a, bytes, text);
		if (bp) igPushStyleColor_Vec4(ImGuiCol_Text, v4(1.0f, 0.35f, 0.35f, 1.0f));
		if (igSelectable_Bool(line, a == pc, 0, v2(0, 0))) dbg_toggle_breakpoint(&ui->sess, a);
		if (bp) igPopStyleColor(1);
		a = (ux_u16)(a + len);
	}
}

static void ram_tab(DebuggerUI *ui, const Varvara *v, const DebugInfo *info)
{
	int row, col;

	if (igInputText("Address", ui->mem_text, sizeof ui->mem_text,
			ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL))
		ui->mem_addr = (unsigned)strtoul(ui->mem_text, NULL, 16) & 0xfff0;
	igSameLine(0, -1);
	if (igButton("-100", v2(0, 0))) ui->mem_addr = (ui->mem_addr - 0x100) & 0xfff0;
	igSameLine(0, -1);
	if (igButton("+100", v2(0, 0))) ui->mem_addr = (ui->mem_addr + 0x100) & 0xfff0;
	igSameLine(0, -1);
	if (igButton("Go to PC", v2(0, 0))) ui->mem_addr = info->pc & 0xfff0;
	for (row = 0; row < 32; row++) {
		char line[16 * 3 + 32];
		unsigned a = (ui->mem_addr + (unsigned)row * 16) & 0xffff;
		int n = sprintf(line, "%04x  ", a);

		for (col = 0; col < 16; col++) n += sprintf(line + n, "%02x ", (unsigned)v->ram[(a + (unsigned)col) & 0xffff]);
		line[n++] = ' ';
		for (col = 0; col < 16; col++) {
			unsigned c = v->ram[(a + (unsigned)col) & 0xffff];
			line[n++] = (c >= 32 && c < 127) ? (char)c : '.';
		}
		line[n] = '\0';
		igTextUnformatted(line, NULL);
	}
}

static void device_tab(const Varvara *v)
{
	int d, p;

	for (d = 0; d < 16; d++) {
		char line[16 * 3 + 8];
		int n = sprintf(line, "%02x: ", d * 16);

		for (p = 0; p < 16; p++) n += sprintf(line + n, "%02x ", (unsigned)v->u.dev[d * 16 + p]);
		igTextUnformatted(line, NULL);
		for (p = 0; p < 16; p++) {
			const char *name = varvara_port_name((ux_u8)(d * 16 + p));

			if (name) igText("    %02x %-22s %02x", d * 16 + p, name, (unsigned)v->u.dev[d * 16 + p]);
		}
	}
}

DebugAction debugger_frame(DebuggerUI *ui, SDL_Renderer *ren, const Varvara *v, const DebugInfo *info)
{
	DebugAction act;
	ImGuiIO *io;
	int focused = 0, z, win_w, win_h, scr_w, bottom, rh;

	act.kind = DA_NONE;
	act.zoom = 0;
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	igNewFrame();
	io = igGetIO_Nil();

	debugger_window_size(1, v->width, v->height, info->zoom, &win_w, &win_h);
	scr_w = v->width * info->zoom;
	if (scr_w < DBG_LEFT_MIN_W) scr_w = DBG_LEFT_MIN_W;
	bottom = DBG_MENU_H + v->height * info->zoom;

	if (igBeginMainMenuBar()) {
		if (igBeginMenu("File", true)) {
			if (igMenuItem_Bool("Open ROM...", NULL, false, true)) act.kind = DA_OPEN;
			if (igMenuItem_Bool("Reload", NULL, false, info->can_reload)) act.kind = DA_RELOAD;
			igEndMenu();
		}
		if (igBeginMenu("Zoom", true)) {
			for (z = 1; z <= 3; z++) {
				char label[8];

				sprintf(label, "%dx", z);
				if (igMenuItem_Bool(label, NULL, info->zoom == z, true)) {
					act.kind = DA_ZOOM;
					act.zoom = z;
				}
			}
			igEndMenu();
		}
		igEndMainMenuBar();
	}

	place(ui, (float)scr_w, DBG_MENU_H, DBG_PANEL_W, 240);
	if (igBegin("CPU", NULL, 0)) {
		if (igIsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) focused = 1;
		cpu_panel(ui, v, info, &act);
	}
	igEnd();

	rh = win_h - DBG_MENU_H - 240 - 190;
	place(ui, (float)scr_w, DBG_MENU_H + 240, DBG_PANEL_W, (float)rh);
	if (igBegin("Disassembly", NULL, 0)) {
		if (igIsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) focused = 1;
		disasm_panel(ui, v, info);
	}
	igEnd();

	place(ui, (float)scr_w, (float)(win_h - 190), DBG_PANEL_W, 190);
	if (igBegin("Console", NULL, 0)) {
		int at_bottom = igGetScrollY() >= igGetScrollMaxY() - 1.0f;

		if (igIsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) focused = 1;
		if (igButton("Clear", v2(0, 0))) {
			ui->con_len = 0;
			ui->con[0] = '\0';
		}
		igTextUnformatted(ui->con, ui->con + ui->con_len);
		if (at_bottom) igSetScrollHereY(1.0f);
	}
	igEnd();

	place(ui, 0, (float)bottom, (float)scr_w, (float)(win_h - bottom));
	if (igBegin("Memory", NULL, 0)) {
		if (igIsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) focused = 1;
		if (igBeginTabBar("memtabs", 0)) {
			if (igBeginTabItem("RAM", NULL, 0)) {
				ram_tab(ui, v, info);
				igEndTabItem();
			}
			if (igBeginTabItem("Devices", NULL, 0)) {
				device_tab(v);
				igEndTabItem();
			}
			igEndTabBar();
		}
	}
	igEnd();
	ui->relayout = 0;

	if (focused && io->KeyCtrl && !io->KeyShift && !io->KeyAlt && act.kind == DA_NONE) {
		if (igIsKeyPressed_Bool(ImGuiKey_S, false)) act.kind = DA_STEP;
		else if (igIsKeyPressed_Bool(ImGuiKey_N, false)) act.kind = DA_STEP_OVER;
		else if (igIsKeyPressed_Bool(ImGuiKey_G, false)) act.kind = DA_CONTINUE;
		else if (igIsKeyPressed_Bool(ImGuiKey_P, false)) act.kind = DA_PAUSE;
	}

	igRender();
	ImGui_ImplSDLRenderer3_RenderDrawData(igGetDrawData(), ren);
	return act;
}
