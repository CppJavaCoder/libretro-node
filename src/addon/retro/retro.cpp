#include "addon/retro/retro.h"
#include "addon/retro/memory.h"
#include "addon/safe_call.h"
#include "addon/param.h"
#include "frontend/app.h"
#include "common/file_util.h"

#include <fmt/format.h>

#include <SDL2/SDL_endian.h>
#include <SDL2/SDL.h>

using namespace Param;

namespace Addon::RETRO {

Napi::Value LoadCore(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().LoadCore(AsStrUtf8(info[0]).c_str());
        return info.Env().Undefined();
    });
}
Napi::Value UnloadCore(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().UnloadCore();
        return info.Env().Undefined();
    });
}

Napi::Value Init(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().Init();
        return info.Env().Undefined();
    });
}
Napi::Value Deinit(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().Deinit();
        return info.Env().Undefined();
    });
}
Napi::Value GetAPIVersion(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromU32(info.Env(),Frontend::App::GetInstance().GetCore().GetAPIVersion());
    });
}
Napi::Value GetSystemInfo(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        retro_system_info *inf;
        Frontend::App::GetInstance().GetCore().GetSystemInfo(inf);
        return FromPtr(info.Env(),(void*)inf);
    });
}
Napi::Value GetSystemAvInfo(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        retro_system_av_info *inf;
        Frontend::App::GetInstance().GetCore().GetSystemAvInfo(inf);
        return FromPtr(info.Env(),(void*)inf);
    });
}
Napi::Value SetControllerPortDevice(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().SetControllerPortDevice(AsU32(info[0]),AsU32(info[1]));
        return info.Env().Undefined();
    });
}
Napi::Value Reset(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().Reset();
        return info.Env().Undefined();
    });
}
Napi::Value Run(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().Run();
        return info.Env().Undefined();
    });
}
Napi::Value SerializeSize(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromU32(info.Env(),Frontend::App::GetInstance().GetCore().SerializeSize());
    });
}
Napi::Value Serialize(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromBool(info.Env(),Frontend::App::GetInstance().GetCore().Serialize(AsPtr(info[0]),AsU32(info[1])));
    });
}
Napi::Value Unserialize(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromBool(info.Env(),Frontend::App::GetInstance().GetCore().Unserialize(AsPtr(info[0]),AsU32(info[1])));
    });
}
Napi::Value CheatReset(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().CheatReset();
        return info.Env().Undefined();
    });
}
Napi::Value CheatSet(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().CheatSet(AsU32(info[0]),AsBool(info[1]),AsStrUtf8(info[2]).c_str());
        return info.Env().Undefined();
    });
}
Napi::Value LoadGame(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        retro_game_info inf;

        inf.path = AsStrUtf8(info[0]).c_str();
        inf.meta = "";
        inf.data = NULL;
        inf.size = 0;

        if (inf.path) {
                SDL_RWops *file = SDL_RWFromFile(inf.path, "rb");
                Sint64 size;

                size = SDL_RWsize(file);

            inf.size = size;
            inf.data = SDL_malloc(inf.size);


            SDL_RWread(file, (void*)inf.data, inf.size, 1);

            Frontend::App::GetInstance().GetCore().SetGame((u8*)inf.data,inf.size);

            SDL_RWclose(file);
        }

        return FromBool(info.Env(),Frontend::App::GetInstance().GetCore().LoadGame(&inf));
    });
}
Napi::Value LoadGameSpecial(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        retro_game_info inf;

        inf.path = AsStrUtf8(info[1]).c_str();
        inf.meta = "";
        inf.data = NULL;
        inf.size = 0;

        if (inf.path) {
                SDL_RWops *file = SDL_RWFromFile(inf.path, "rb");
                Sint64 size;

                size = SDL_RWsize(file);

            inf.size = size;
            inf.data = SDL_malloc(inf.size);


            SDL_RWread(file, (void*)inf.data, inf.size, 1);

            SDL_RWclose(file);
        }

        return FromBool(info.Env(),Frontend::App::GetInstance().GetCore().LoadGameSpecial(AsU32(info[0]),&inf,AsU32(info[2])));
    });
}
Napi::Value UnloadGame(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().UnloadGame();
        return info.Env().Undefined();
    });
}

Napi::Value GetRegion(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromU32(info.Env(),Frontend::App::GetInstance().GetCore().GetRegion());
    });
}

Napi::Value Pause(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().Pause();
        return info.Env().Undefined();
    });
}

Napi::Value Resume(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().Resume();
        return info.Env().Undefined();
    });
}

Napi::Value AdvanceFrame(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        Frontend::App::GetInstance().GetCore().AdvanceFrame();
        return info.Env().Undefined();
    });
}

Napi::Value GetGameInfo(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromPtr(info.Env(),(void*)Frontend::App::GetInstance().GetCore().GetGameInfo());
    });
}

Napi::Object BuildExports(Napi::Env env, Napi::Object exports)
{
    exports.Set("Memory", Memory::BuildExports(env, Napi::Object::New(env)));

    exports.Set("loadCore", Napi::Function::New(env, LoadCore));
    exports.Set("unloadCore", Napi::Function::New(env, UnloadCore));

    exports.Set("loadGame", Napi::Function::New(env, LoadGame));
    exports.Set("loadGameSpecial", Napi::Function::New(env, LoadGameSpecial));
    exports.Set("unloadGame", Napi::Function::New(env, UnloadGame));

    exports.Set("getAPIVersion", Napi::Function::New(env, GetAPIVersion));
    exports.Set("getSystemInfo", Napi::Function::New(env, GetSystemInfo));
    exports.Set("getSystemAvInfo", Napi::Function::New(env, GetSystemAvInfo));
    exports.Set("setControllerPortDevice", Napi::Function::New(env, SetControllerPortDevice));
    exports.Set("reset", Napi::Function::New(env, Reset));
    exports.Set("run", Napi::Function::New(env, Run));
    exports.Set("serializeSize", Napi::Function::New(env, SerializeSize));
    exports.Set("serialize", Napi::Function::New(env, Serialize));
    exports.Set("unserialize", Napi::Function::New(env, Unserialize));
    exports.Set("cheatReset", Napi::Function::New(env, CheatReset));
    exports.Set("cheatSet", Napi::Function::New(env, CheatSet));
    exports.Set("getRegion", Napi::Function::New(env, GetRegion));
    exports.Set("pause", Napi::Function::New(env, Pause));
    exports.Set("resume", Napi::Function::New(env, Resume));
    exports.Set("advanceFrame", Napi::Function::New(env, AdvanceFrame));

    exports.Set("getGameInfo", Napi::Function::New(env, GetGameInfo));

    return exports;
}

}
