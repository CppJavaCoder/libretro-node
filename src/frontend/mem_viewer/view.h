#pragma once

#include "frontend/tool_base.h"
#include "retro/fwd.h"

struct ImFont;

namespace MemViewer {

class MemViewerImpl;

class View : public Frontend::ToolBase {
public:
    View(RETRO::Core& core);
    ~View();

    void Show(SDL::Window& main_win, ImFont* font_mono);

private:
    std::unique_ptr<MemViewerImpl> m_impl;
};

}
