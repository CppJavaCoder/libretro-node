#pragma once

#include <napi.h>

namespace Addon::Frontend::Sprite {

Napi::Object BuildExports(Napi::Env env, Napi::Object exports);

}