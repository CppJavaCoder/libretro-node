#include "common/logger.h"
#include "common/message_box.h"
#include "frontend/app.h"
#include "addon/param.h"
#include "addon/safe_call.h"

#include "frontend/retro/sprite.h"

#include <array>
#include <atomic>
#include <unordered_map>

#include <fmt/format.h>

using namespace Param;

namespace Addon::Frontend::Sprite {

inline ::Frontend::App& GetApp()
{
    return ::Frontend::App::GetInstance();
}
static u32 spr_count = 0;
Napi::Value FromImage(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        //int val = GetApp().CreateSprite(AsStrUtf8(info[0]),AsU32(info[1]),AsU32(info[2]),AsU32(info[3]),AsU32(info[4]));
        std::vector<std::string> prm;
        prm.push_back(AsStrUtf8(info[0]));
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        iprm.push_back(AsU32(info[2]));
        iprm.push_back(AsU32(info[3]));
        iprm.push_back(AsU32(info[4]));
        GetApp().PushBackCommand("FromImage",spr_count++,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value FromSurface(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromS32(info.Env(),GetApp().CreateSprite((SDL_Surface*)AsPtr(info[0]),AsU32(info[1]),AsU32(info[2]),AsU32(info[3]),AsU32(info[4])));
    });
}
Napi::Value FromBuffer(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromS32(info.Env(),GetApp().CreateSprite((SDL_Surface*)AsPtr(info[0]),AsU32(info[1]),AsU32(info[2]),AsU32(info[3]),AsU32(info[4]),AsU32(info[5])));
    });
}
Napi::Value FromSprite(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[0]));
        GetApp().PushBackCommand("FromSprite",spr_count++,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value Remove(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        GetApp().PushBackCommand("Remove",id,prm,iprm);
        return info.Env().Undefined();
    });
}

Napi::Value SetFrame(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        GetApp().PushBackCommand("SetFrame",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value Animate(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        iprm.push_back(AsU32(info[2]));
        iprm.push_back(AsU32(info[3]));
        GetApp().PushBackCommand("Animate",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value SetPos(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        iprm.push_back(AsU32(info[2]));
        GetApp().PushBackCommand("SetPos",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value ReplaceColor(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        iprm.push_back(AsU32(info[2]));
        iprm.push_back(AsU32(info[3]));
        iprm.push_back(AsU32(info[4]));
        iprm.push_back(AsU32(info[5]));
        iprm.push_back(AsU32(info[6]));
        GetApp().PushBackCommand("ReplaceColor",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value GetX(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromS32(info.Env(),GetApp().GetSprite(AsU32(info[0]))->GetX());
    });
}
Napi::Value GetY(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromS32(info.Env(),GetApp().GetSprite(AsU32(info[0]))->GetY());
    });
}
Napi::Value GetW(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromS32(info.Env(),GetApp().GetSprite(AsU32(info[0]))->GetW());
    });
}
Napi::Value GetH(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromS32(info.Env(),GetApp().GetSprite(AsU32(info[0]))->GetH());
    });
}
//Saving for internal use, am not letting this through the binding
Napi::Value Draw(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return info.Env().Undefined();
    });
}
Napi::Value SetHFlip(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        GetApp().PushBackCommand("SetHFlip",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value SetVFlip(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        GetApp().PushBackCommand("SetVFlip",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value SetFG(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsBool(info[1]) ? 1 : 0);
        GetApp().PushBackCommand("SetFG",id,prm,iprm);
        return info.Env().Undefined();
    });
}
Napi::Value GetFG(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        return FromBool(info.Env(),GetApp().GetSprite(AsU32(info[0]))->GetFG());
    });
}
Napi::Value SetClip(const Napi::CallbackInfo& info)
{
    return SafeCall(info.Env(), [&info]() {
        int id = AsU32(info[0]);
        std::vector<std::string> prm;
        std::vector<int> iprm;
        iprm.push_back(AsU32(info[1]));
        iprm.push_back(AsU32(info[2]));
        iprm.push_back(AsU32(info[3]));
        iprm.push_back(AsU32(info[4]));
        GetApp().PushBackCommand("SetClip",id,prm,iprm);
        return info.Env().Undefined();
    });
}

Napi::Object BuildExports(Napi::Env env, Napi::Object exports)
{
    exports.Set("fromImage", Napi::Function::New(env, FromImage));
    exports.Set("fromSurface", Napi::Function::New(env, FromSurface));
    exports.Set("fromBuffer", Napi::Function::New(env, FromBuffer));
    exports.Set("fromSprite", Napi::Function::New(env, FromSprite));

    exports.Set("remove", Napi::Function::New(env, Remove));
    exports.Set("setFrame", Napi::Function::New(env, SetFrame));
    exports.Set("animate", Napi::Function::New(env, Animate));
    exports.Set("setPos", Napi::Function::New(env, SetPos));
    exports.Set("replaceColor", Napi::Function::New(env, ReplaceColor));
    exports.Set("replaceColour", Napi::Function::New(env, ReplaceColor));
    exports.Set("getX", Napi::Function::New(env, GetX));
    exports.Set("getY", Napi::Function::New(env, GetY));
    exports.Set("getW", Napi::Function::New(env, GetW));
    exports.Set("getH", Napi::Function::New(env, GetH));
    exports.Set("setHFlip", Napi::Function::New(env, SetHFlip));
    exports.Set("setVFlip", Napi::Function::New(env, SetVFlip));
    exports.Set("setFG", Napi::Function::New(env, SetFG));
    exports.Set("getFG", Napi::Function::New(env, GetFG));
    exports.Set("setClip", Napi::Function::New(env, SetClip));

    return exports;
}

}