#pragma once

#include <napi.h>

namespace Addon::RETRO::Config {

Napi::Object BuildExports(Napi::Env env, Napi::Object exports);

}
