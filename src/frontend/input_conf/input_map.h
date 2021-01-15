#pragma once

#include "cont_tab.h"
#include <SDL2/SDL.h>
#include "common/types.h"

namespace InputConf
{
    using namespace MapUtil;

    class InputMap
    {
        public:
            InputMap(ContTab *tb);
            ~InputMap();
            std::vector<u8> Update();
            bool IsPlugged();
        private:
            ContTab *ctab;
    };

}