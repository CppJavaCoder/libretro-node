#include "input_map.h"

#include "map_util.h"
#include "common/logger.h"

#include <vector>

namespace InputConf
{

    InputMap::InputMap(ContTab *t) : ctab(t)
    {}

    std::vector<u8> InputMap::Update()
    {
        Uint8 *keybuf = (Uint8*)SDL_GetKeyboardState(NULL);

        std::vector<u8> output;
        for(int n = 0; n < MapIndex_Count; n++)
            output.push_back(0);

        //None, Key, JoyButton, JoyHat, JoyAxis
        for (int i = 0; i < MapIndex_Count; ++i)
            switch(ctab->m_map[i].type)
            {
                default:
                break;
                case MapType::Key:
                    if(keybuf[ctab->m_map[i].key.scancode])
                        output[i] = 1;
                break;
                case MapType::JoyHat:
                    if(ctab->m_joystick.GetHat(ctab->m_map[i].jhat.hat)==ctab->m_map[i].jhat.raw_dir)
                        output[i] = 1;
                break;
                case MapType::JoyButton:
                    if(ctab->m_joystick.GetButton(ctab->m_map[i].jbutton.button))
                        output[i] = 1;
                break;
                case MapType::JoyAxis:
                    if(ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis))
                        output[i] = ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis);
                break;
            }
        return output;
    }

}