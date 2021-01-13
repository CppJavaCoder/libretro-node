#pragma once

#include "common/types.h"
#include "retro/fwd.h"

#include <array>

#include <libretro.h>

namespace Frontend {

typedef union {
    unsigned int Value;
    struct {
        unsigned R_DPAD       : 1;
        unsigned L_DPAD       : 1;
        unsigned D_DPAD       : 1;
        unsigned U_DPAD       : 1;
        unsigned START_BUTTON : 1;
        unsigned Z_TRIG       : 1;
        unsigned B_BUTTON     : 1;
        unsigned A_BUTTON     : 1;

        unsigned R_CBUTTON    : 1;
        unsigned L_CBUTTON    : 1;
        unsigned D_CBUTTON    : 1;
        unsigned U_CBUTTON    : 1;
        unsigned R_TRIG       : 1;
        unsigned L_TRIG       : 1;
        unsigned Reserved1    : 1;
        unsigned Reserved2    : 1;

        signed   X_AXIS       : 8;
        signed   Y_AXIS       : 8;
    };
} BUTTONS;

class Input {
public:
    void Update(RETRO::Core& core);

    f32 GetAxis(u32 cont, u32 axis);
    void SetAxis(u32 cont, u32 axis, f32 value);
    void SetAxisVi(u32 cont, u32 axis, f32 value);
    bool GetButton(u32 cont, u32 button);
    bool GetButtonDown(u32 cont, u32 button);
    bool GetButtonUp(u32 cont, u32 button);
    void SetButton(u32 cont, u32 button, bool pressed);
    void SetButtonDown(u32 cont, u32 button);

private:
    static constexpr unsigned NumCont = 4;
    static constexpr u32 NumButtons = 14;
    static constexpr u32 NumAxis = 2;

    using ContState = std::array<BUTTONS, NumCont>;

    ContState m_curr{};
    ContState m_prev{};
    ContState m_btn_down{};
    ContState m_btn_up{};
    ContState m_persist{};
    ContState m_vi{};
};

}
