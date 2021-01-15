#include "input_map.h"

#include "map_util.h"
#include "common/logger.h"

#include <vector>

namespace InputConf
{

    InputMap::InputMap(ContTab *t) : ctab(t)
    {}
    InputMap::~InputMap()
    {ctab=nullptr;}

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
                    if(ctab->m_map[i].jhat.dir == JoyHatDir::Right && ctab->m_joystick.GetHat(ctab->m_map[i].jhat.hat) & SDL_HAT_RIGHT)
                        output[i] = 1;
                    if(ctab->m_map[i].jhat.dir == JoyHatDir::Left && ctab->m_joystick.GetHat(ctab->m_map[i].jhat.hat) & SDL_HAT_LEFT)
                        output[i] = 1;
                    if(ctab->m_map[i].jhat.dir == JoyHatDir::Down && ctab->m_joystick.GetHat(ctab->m_map[i].jhat.hat) & SDL_HAT_DOWN)
                        output[i] = 1;
                    if(ctab->m_map[i].jhat.dir == JoyHatDir::Up && ctab->m_joystick.GetHat(ctab->m_map[i].jhat.hat) & SDL_HAT_UP)
                        output[i] = 1;
                break;
                case MapType::JoyButton:
                    if(ctab->m_joystick.GetButton(ctab->m_map[i].jbutton.button))
                        output[i] = 1;
                break;
                case MapType::JoyAxis:
                    bool in_range = ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis) < ctab->m_joy_init_axes[ctab->m_map[i].jaxis.axis] - 18000
                        || ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis) > ctab->m_joy_init_axes[ctab->m_map[i].jaxis.axis] + 18000;
                    if(in_range && ctab->m_map[i].jaxis.dir == MapUtil::JoyAxisDir::Positive && ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis))
                    {
                        int value = ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis)/140;
                        if(ctab->m_map[i].jaxis.dir == MapUtil::JoyAxisDir::Positive)
                        {
                            if(value > 0)
                                output[i] = value;
                            else
                                output[i] = 0;
                        }else
                        {
                            if(value < 0)
                                output[i] = abs(value);
                            else
                                output[i] = 0;                            
                        }
                        
                    }
                    else if(in_range && ctab->m_map[i].jaxis.dir == MapUtil::JoyAxisDir::Negative && ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis))
                    {
                        Logger::Log(LogCategory::Debug,"CTRL","Negative");
                        int value = -ctab->m_joystick.GetAxis(ctab->m_map[i].jaxis.axis)/129;
                        if(value > 0)
                            output[i] = value;
                        else
                            output[i] = 0;
                    }
                break;
            }
        return output;
    }
    bool InputMap::IsPlugged()
    {
        if(ctab == nullptr)
            return false;
        return ctab->m_plugged;
    }

}