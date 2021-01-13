#pragma once

#include <napi.h>

namespace Addon::RETRO {

Napi::Object BuildExports(Napi::Env env, Napi::Object exports);

}
