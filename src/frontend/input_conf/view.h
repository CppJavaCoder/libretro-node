#pragma once

#include "frontend/input_conf/cont_tab.h"
#include "frontend/input_conf/events_tab.h"
#include "frontend/tool_base.h"
#include "retro/fwd.h"

namespace InputConf {

class View : public Frontend::ToolBase {
public:
    View();

    void LoadConfig(RETRO::Core& core);
    void SaveConfig(RETRO::Core& core);
    void DoEvent(const SDL_Event& e);
    void Show(SDL::Window& main_win);

    ContTab *GetContTab(int index);

private:
    std::size_t m_tab_index{};
    std::array<ContTab, 4> m_cont_tabs;
    EventsTab m_events_tab;
};

}
