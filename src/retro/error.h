#pragma once

#include "common/logged_runtime_error.h"

#include <libretro.h>

namespace RETRO {

enum retro_error
{
    SUCCESS=0, NOT_INIT=1, ALREADY_INIT=2, INCOMPATIBLE=3, INPUT_ASSERT=4, INPUT_INVALID=5, INPUT_NOT_FOUND=6,
    NO_MEMORY=7, FILES=8, INTERNAL=9, INVALID_STATE=10, SYSTEM_FAIL=11, UNSUPPORTED=12, WRONG_TYPE=13
};

class Error : public LoggedRuntimeError {
public:
    Error(retro_error error_code);
	Error(const char* function_name, retro_error error_code);

private:
    static const char* GetCoreErrorMessage(retro_error error_code);
};

}
