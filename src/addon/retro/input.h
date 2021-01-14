#pragma once

#include <napi.h>

namespace Addon::RETRO::Input {

Napi::Object BuildExports(Napi::Env env, Napi::Object exports);

}
