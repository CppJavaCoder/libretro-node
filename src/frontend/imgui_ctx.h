#pragma once

#include "sdl/fwd.h"
#include <SDL2/SDL.h>
struct ImGuiContext;
union SDL_Event;

namespace Frontend {

class ImGuiCtx {
public:
    ImGuiCtx() = default;
    ~ImGuiCtx();
    ImGuiCtx(const ImGuiCtx& other) = delete;
    ImGuiCtx& operator=(const ImGuiCtx& other) = delete;
    ImGuiCtx(ImGuiCtx&& other) = delete;
    ImGuiCtx& operator=(ImGuiCtx&& other) = delete;

    void Initialize(SDL::Window& window);
    void Deinitialize();
    bool ProcessEvent(const SDL_Event& e);
    void NewFrame(SDL::Window& window);
    void EndFrame();
    void RebuildFontAtlas();
    void ApplyStyle();

private:
    bool m_init{};
    ImGuiContext* m_ctx{};

    void InitContext(SDL::Window& window);
    void DeinitContext();
    void InitBackends(SDL::Window& window);
    void DeinitBackends();
};

}
