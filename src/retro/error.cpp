#include "retro/error.h"

#include <array>

#include <fmt/format.h>

namespace RETRO {

static std::array<const char*, 14> k_ErrorMessages {
	"SUCCESS: No error",
	"NOT_INIT: A function was called before it's associated module was initialized",
	"ALREADY_INIT: Initialization function called twice",
	"INCOMPATIBLE: API versions between components are incompatible",
	"INPUT_ASSERT: Invalid function parameters, such as a NULL pointer",
	"INPUT_INVALID: An input function parameter is logically invalid",
	"INPUT_NOT_FOUND: The input parameter(s) specified a particular item which was not found",
	"NO_MEMORY: Memory allocation failed",
	"FILES: Error opening, creating, reading, or writing to a file",
	"INTERNAL: logical inconsistency in program code.  Probably a bug.",
	"INVALID_STATE: An operation was requested which is not allowed in the current state",
	"SYSTEM_FAIL: A system function call, such as an SDL or file operation, failed",
	"UNSUPPORTED: Function call is not supported (ie, core not built with debugger)",
	"WRONG_TYPE: A given input type parameter cannot be used for desired operation"
};

Error::Error(retro_error error_code) :
	LoggedRuntimeError{"Retro", GetCoreErrorMessage(error_code)}
{}

Error::Error(const char* function_name, retro_error error_code) :
	LoggedRuntimeError{"Retro", fmt::format("{}: {}", function_name, GetCoreErrorMessage(error_code))}
{}

const char* Error::GetCoreErrorMessage(retro_error error_code)
{
	auto i = static_cast<std::size_t>(error_code);
    return i >= k_ErrorMessages.size() ? "ERROR: Invalid retro_error code" : k_ErrorMessages[i];
}

}
