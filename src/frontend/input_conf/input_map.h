#pragma once

#include "cont_tab.h"
#include "events_tab.h"
#include <SDL2/SDL.h>
#include "common/types.h"

namespace InputConf
{
    using namespace MapUtil;

    class InputMap
    {
        public:
            InputMap(ContTab *tb);
            InputMap(EventsTab *tb);
            ~InputMap();
            std::vector<u8> Update();
            bool IsPlugged();
        private:
            std::vector<u8> UpdateCtab();
            std::vector<u8> UpdateEtab();
            
            ContTab *ctab;
            EventsTab *etab;
    };

}