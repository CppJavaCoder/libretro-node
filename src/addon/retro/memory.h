#pragma once

#include <napi.h>

namespace Addon::RETRO::Memory {

Napi::Object BuildExports(Napi::Env env, Napi::Object exports);

}
