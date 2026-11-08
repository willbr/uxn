/* C declarations for the Dear ImGui SDLRenderer3 backend.
 * cimgui_impl.h (vendored, 2a6a93f) has no SDLRenderer3 section, so we declare
 * the functions we use here. They are compiled with IMGUI_IMPL_API=extern "C",
 * so the symbols are unmangled. Include after cimgui.h and cimgui_impl.h. */
#ifndef IMGUI_BACKENDS_H
#define IMGUI_BACKENDS_H

bool ImGui_ImplSDLRenderer3_Init(SDL_Renderer *renderer);
void ImGui_ImplSDLRenderer3_Shutdown(void);
void ImGui_ImplSDLRenderer3_NewFrame(void);
void ImGui_ImplSDLRenderer3_RenderDrawData(ImDrawData *draw_data, SDL_Renderer *renderer);

#endif
