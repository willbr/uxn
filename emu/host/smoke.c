/* GUI dependency smoke test: hidden SDL3 window, one Dear ImGui frame. */
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_SDL3
#define CIMGUI_USE_SDLRENDERER3
#include "cimgui.h"
#include "cimgui_impl.h"
#include "imgui_backends.h"

static int fail(const char *what)
{
	fprintf(stderr, "smoke: %s failed: %s\n", what, SDL_GetError());
	return 1;
}

int main(void)
{
	SDL_Window *win;
	SDL_Renderer *ren;
	ImGuiContext *ctx;

	if (!SDL_Init(SDL_INIT_VIDEO)) return fail("SDL_Init");
	win = SDL_CreateWindow("smoke", 320, 240, SDL_WINDOW_HIDDEN);
	if (!win) return fail("SDL_CreateWindow");
	ren = SDL_CreateRenderer(win, NULL);
	if (!ren) return fail("SDL_CreateRenderer");

	ctx = igCreateContext(NULL);
	if (!ctx) return fail("igCreateContext");
	igGetIO_Nil()->IniFilename = NULL; /* no imgui.ini in the cwd */
	if (!ImGui_ImplSDL3_InitForSDLRenderer(win, ren)) return fail("ImGui_ImplSDL3_InitForSDLRenderer");
	if (!ImGui_ImplSDLRenderer3_Init(ren)) return fail("ImGui_ImplSDLRenderer3_Init");

	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	igNewFrame();
	igBegin("smoke", NULL, 0);
	igText("ok");
	igEnd();
	igRender();
	SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
	SDL_RenderClear(ren);
	ImGui_ImplSDLRenderer3_RenderDrawData(igGetDrawData(), ren);
	SDL_RenderPresent(ren);

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	igDestroyContext(ctx);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	puts("smoke ok");
	return 0;
}
