#include "common/logged_runtime_error.h"
#include "frontend/cheat_conf/view.h"
#include "frontend/fonts/fonts.h"
#include "frontend/gfx/texture.h"
#include "frontend/input_conf/view.h"
#include "frontend/mem_viewer/view.h"
#include "frontend/app.h"
#include "frontend/imgui_sdl.h"
//#include "frontend/glinfo.h"
#include "frontend/vidext.h"
#include "imgui/imgui.h"
#include "sdl/surface.h"

#include "retro/cheat.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <fmt/format.h>
//#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

//It bothers me that I have to do this
#include "retro/retro_bound.cpp"
#include "retro/sprite.cpp"

namespace Frontend {

inline bool HigherSprite(RETRO::Sprite& a,RETRO::Sprite& b)
{
    return a.GetY() > b.GetY();
}

App::App()
{
    SDL_SetMainReady();

    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    m_sdl_init.sdl.Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    m_sdl_init.img.Init(IMG_INIT_PNG | IMG_INIT_JPG);
    m_sdl_init.ttf.Init();

    resized = false;
    
    for(int n = 0; n < 4; n++)
        map[n] = nullptr;
}

App::~App()
{
    for(int n = 0; n < 4; n++)
        if(map[n] != nullptr)
        {
            delete map[n];
            map[n] = nullptr;
        }
    
    for(std::vector<RETRO::Sprite*>::iterator i=spr.begin();i!=spr.end();i++)
    {
        if((*i) != nullptr)
        {
            delete *i;
            *i = nullptr;
        }
    }
    spr.clear();

    m_sdl_init.ttf.Quit();
    m_sdl_init.img.Quit();
    m_sdl_init.sdl.Quit();

}
//I'm not using opengl
/*
void App::PrintOpenGLInfo()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "PrintOpenGLInfo");
    SDL::GLContext context{m_video.window};

    Logger::Log(LogCategory::Debug, "OpenGL", fmt::format("Initial context: {}", GetGLContextInfo()));

    Logger::Log(LogCategory::Debug, "OpenGL", fmt::format("Vendor: {}", SafeGLGetString(GL_VENDOR)));
    Logger::Log(LogCategory::Debug, "OpenGL", fmt::format("Renderer: {}", SafeGLGetString(GL_RENDERER)));
    Logger::Log(LogCategory::Debug, "OpenGL", fmt::format("Version: {}", SafeGLGetString(GL_VERSION)));
    Logger::Log(LogCategory::Debug, "OpenGL", fmt::format("Shading language version: {}", SafeGLGetString(GL_SHADING_LANGUAGE_VERSION)));
}*/

void App::InitVideo(const StartInfo& info)
{
    m_video.window = {info.window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, info.window_width, info.window_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN};

    video_init();

    m_video.imgui.Initialize(m_video.window);

    auto font_atlas = ImGui::GetIO().Fonts;
    m_fonts.base = font_atlas->AddFontFromMemoryCompressedTTF(DroidSans_compressed_data, DroidSans_compressed_size, 18);
    m_fonts.mono = font_atlas->AddFontFromMemoryCompressedTTF(DroidSansMono_compressed_data, DroidSansMono_compressed_size, 18);
    m_video.imgui.RebuildFontAtlas();

    Frontend::App::GetInstance().NewVIHandler();
    Logger::Log(LogCategory::Debug, "VideoInit", "Exit");
}

void App::DeinitVideo()
{
    m_video.imgui.Deinitialize();
    m_video.window = {};
}

void App::InitFrameCapture()
{
    //Not needed
}

inline void DebugLog(void*, int level, const char* message)
{
    //Todo implement
}

struct AppCallbacks {
    static void NewFrameHandler()
    {
        App::GetInstance().NewFrameHandler();
    }

    static void NewVIHandler()
    {
        App::GetInstance().NewVIHandler();
    }

    static void ResetHandler(int do_hard_reset)
    {
        App::GetInstance().ResetHandler(do_hard_reset);
    }

    static void PauseLoopHandler()
    {
        App::GetInstance().PauseLoopHandler();
    }

    static void CoreEventHandler(int event)
    {
        App::GetInstance().CoreEventHandler(event);
    }

    static void CoreStateChangedHandler(void*, int param_type, int new_value)
    {
        App::GetInstance().CoreStateChangedHandler(param_type, new_value);
    }

    static void DebugInitHandler()
    {
        App::GetInstance().DebugInitHandler();
    }

    static void DebugUpdateHandler(unsigned pc)
    {
        App::GetInstance().DebugUpdateHandler(pc);
    }
};

void App::InitEmu(const StartInfo& info)
{
    m_emu.core.LoadCore(std::string(info.retrolib.generic_string() + ".dll").c_str());
    m_emu.core.Startup(info.config_dir,info.data_dir);

    m_emu.core.SetEnvironment(core_environment);
    m_emu.core.SetVideoRefresh(core_video_refresh);
    m_emu.core.SetInputPoll(core_input_poll);
    m_emu.core.SetInputState(core_input_state);
    m_emu.core.SetAudioSample(core_audio_sample);
    m_emu.core.SetAudioSampleBatch(core_audio_sample_batch);

    m_emu.core.Init();

    m_emu.data_dir = info.data_dir;
    m_emu.core_init = true;

    // Configure the player input devices.
    m_emu.core.SetControllerPortDevice(0, RETRO_DEVICE_JOYPAD);
    m_emu.core.SetControllerPortDevice(1, RETRO_DEVICE_JOYPAD);
    m_emu.core.SetControllerPortDevice(2, 0xFF);
    m_emu.core.SetControllerPortDevice(3, 0xFF);
    m_emu.core.SetControllerPortDevice(4, 0xFF);
    m_emu.core.SetControllerPortDevice(5, 0xFF);
    m_emu.core.SetControllerPortDevice(6, 0xFF);
    m_emu.core.SetControllerPortDevice(7, 0xFF);

	//struct retro_system_info system = {0};
	//m_emu.core.GetSystemInfo(&system);

    struct retro_system_av_info av = {0};
	m_emu.core.GetSystemAvInfo(&av);

	audio_init(av.timing.sample_rate);
}

void App::DeinitEmu()
{
    audio_deinit();

    variables_free();

    m_emu.core.Deinit();
    m_emu.core.UnloadGame();
    m_emu.core.UnloadCore();

    m_emu.core_init = false;
    m_emu.debug_init = false;
    m_emu.started = false;
}

void App::DoEvents()
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        m_video.imgui.ProcessEvent(e);

        if (m_emu.core_init) {
            if(e.type == SDL_QUIT)
            {
                core_stop();
                OnWindowClosing.fire();
            }
            else if (e.type == SDL_WINDOWEVENT && e.window.windowID == m_video.window.GetId()) {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    OnWindowClosing.fire();
                    core_stop();
                }
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				    resized = true;
				}
            }
//          Is for Mupen
//            else if (!m_any_item_active && e.key.windowID == m_video.window.GetId() && m_video.window.HasInputFocus()) {
//                if (e.type == SDL_KEYDOWN)
//                    ;//m_emu.core.InputKeyDown(e.key.keysym.mod, e.key.keysym.scancode);
//                else if (e.type == SDL_KEYUP)
//                    ;//m_emu.core.InputKeyUp(e.key.keysym.mod, e.key.keysym.scancode);
//            }
        }

        if (m_util_win.input_conf)
        {
            m_util_win.input_conf->DoEvent(e);
        }
    }
}

void App::LoadCheats()
{
    static const std::string k_NullCrc = "00000000-00000000-0:00";

    static const auto init_block = [](RETRO::Cheat::Block* block) {
        block->crc = k_NullCrc;
        block->good_name = "None";
    };

    try {
        m_cheats.map = RETRO::Cheat::Load(m_emu.data_dir / "libretrocheat.txt");
    }
    catch (const std::exception& e) {
        Logger::Log(LogCategory::Warn, "Retro frontend", fmt::format("Cheats not loaded. {}", e.what()));
    }

    try {
        m_cheats.user_map = RETRO::Cheat::Load(m_emu.data_dir / "libretro_user.txt");
    }
    catch (const std::exception& e) {
        Logger::Log(LogCategory::Warn, "Retro frontend", fmt::format("User cheats not loaded. {}", e.what()));
    }

    m_cheats.block = &m_cheats.map[k_NullCrc];
    init_block(m_cheats.block);
    m_cheats.user_block = &m_cheats.user_map[k_NullCrc];
    init_block(m_cheats.user_block);
}

void App::SaveCheats()
{
    try {
        RETRO::Cheat::Save(m_emu.data_dir / "libretro_user.txt", m_cheats.user_map);
    }
    catch (const std::exception& e) {
        Logger::Log(LogCategory::Error, "Retro frontend", fmt::format("User cheats not saved. {}", e.what()));
    }
}

void App::LoadROMCheats()
{
    // Yet to be implemented
    
    RETRO::Core::RetroHeader header;
    m_emu.core.GetRetroHeader(&header);
    //m64p_rom_settings settings;
    //m_emu.core.GetROMSettings(&settings);

    //auto crc = !m_cheats.crc.empty() ? m_cheats.crc :
        //fmt::format("{:08X}-{:08X}-C:{:02X}", "CRC1", "CRC2", 0xff/*header.Country_code & 0xff*/);

    std::string crc = "Zelda";

    if (auto it = m_cheats.map.find(crc); it != m_cheats.map.end()) {
        m_cheats.block = &it->second;
    }
    else {
        m_cheats.block = &m_cheats.map[crc];
        m_cheats.block->crc = crc;
        m_cheats.block->good_name = "Zelda";
    }

    if (auto it = m_cheats.user_map.find(crc); it != m_cheats.user_map.end()) {
        m_cheats.user_block = &it->second;
    }
    else {
        m_cheats.user_block = &m_cheats.user_map[crc];
        m_cheats.user_block->crc = crc;
        m_cheats.user_block->good_name = "Zelda";
    }
}

void App::InitUtilWindows()
{
    m_util_win.input_conf = std::make_unique<InputConf::View>();
    m_util_win.input_conf->LoadConfig(m_emu.core);
    m_util_win.input_conf->OnClosed.connect<&App::InputConfigClosedHandler>(this);

    for(int n=0;n<4;n++)
    {
        if(map[n]!=nullptr)
            delete map[n];
        map[n] = new InputConf::InputMap(m_util_win.input_conf->GetContTab(n));
    }

    m_util_win.cheat_conf = std::make_unique<CheatConf::View>(m_emu.core, *m_cheats.block, *m_cheats.user_block);

    m_util_win.mem_viewer = std::make_unique<MemViewer::View>(m_emu.core);
}

void App::DeinitUtilWindows()
{
    for(int n=0;n<4;n++)
        if(map[n]!=nullptr)
        {
            delete map[n];
            map[n] = nullptr;
        }

    m_util_win.input_conf->OnClosed.disconnect<&App::InputConfigClosedHandler>(this);
    m_util_win.input_conf->SaveConfig(m_emu.core);
    m_util_win.input_conf.reset();

    // Currently Crashes
    //m_util_win.cheat_conf->DisableAllEntries();
    m_util_win.cheat_conf.reset();

    m_util_win.mem_viewer.reset();
}

void App::InputConfigClosedHandler()
{
    m_util_win.input_conf->SaveConfig(m_emu.core);
    // No plugins for our core
    //m_emu.core.ResetInputPlugin();
}

void App::ShowUtilWindows()
{
    // ImGui::GetIO().ConfigViewportsNoDecoration = false;

    if (m_util_win.input_conf)
        m_util_win.input_conf->Show(m_video.window);

    if (m_util_win.cheat_conf)
        m_util_win.cheat_conf->Show(m_video.window, m_fonts.mono);

    if (m_util_win.mem_viewer)
        m_util_win.mem_viewer->Show(m_video.window, m_fonts.mono);

    // ImGui::GetIO().ConfigViewportsNoDecoration = true;
}

void App::Startup(const StartInfo& info)
{
    inf = info;
    InitVideo(info);
    // Not currently needed afaik
    //InitFrameCapture();
    Logger::Log(LogCategory::Debug, "Startup", "InitEmu");
    InitEmu(info);
    Logger::Log(LogCategory::Debug, "Startup", "Done");
    CoreStartedHandler();
    LoadCheats();

    CreateResourcesNextVi();
    Logger::Log(LogCategory::Debug, "Startup", "Exit");
}

void App::Shutdown()
{
    SaveCheats();
    DeinitVideo();
    DeinitEmu();
}

void App::Execute()
{
    m_emu.execute = std::async(std::launch::async,[this]() {
    try{
        Logger::Log(LogCategory::Info,"Marker","0");
        ImGuiSDL::Initialize(renderer_get(),inf.window_width,inf.window_height);

        Logger::Log(LogCategory::Info,"Marker","1");      
        //while(!m_emu.core.GameLoaded());
        Logger::Log(LogCategory::Info,"Marker","3");
        m_emu.core.LoadGameData();
        Logger::Log(LogCategory::Info,"Marker","4");
        while(core_is_running())
        {
            for(std::vector<RETRO::Sprite::Command>::iterator i = cmd.begin(); i != cmd.end(); i++)
            {
                Logger::Log(LogCategory::Info,std::string("Command ") + std::to_string((*i).id),(*i).name);
                if((*i).id < spr.size())
                    spr[(*i).id]->RunCommand(&(*i));
                else
                {
                    if((*i).name == "FromImage")
                    {
                        Logger::Log(LogCategory::Info,"Command",(*i).name);
                        CreateSprite((*i).param[0], (*i).iparam[0], (*i).iparam[1], (*i).iparam[2], (*i).iparam[3]);
                    }
                    else if((*i).name == "FromSprite")
                    {
                        Logger::Log(LogCategory::Debug,"Command",(*i).name);
                        CreateSprite((*i).iparam[0]);
                    }
                    else
                    {
                        Logger::Log(LogCategory::Info,"Command",std::string("Unidentified Command ")+(*i).name);
                    }
                    
                }
                
            }
            cmd.clear();

        Logger::Log(LogCategory::Debug, "Joe's debug", "Dang it A");
            m_emu.core.Run();
        Logger::Log(LogCategory::Debug, "Joe's debug", "Dang it B");
            core_refresh();
        }
        m_emu.core.SaveGameData();
        ImGuiSDL::Deinitialize();
        video_deinit();
        CoreStoppedHandler();
    }catch(const std::exception& e)
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "Dang it A");
        std::exit(EXIT_FAILURE);
    }catch(...)
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "Dang it B");
        std::exit(EXIT_FAILURE);
    }
    });
}

void App::Stop()
{
    //m_emu.core.Stop();
    m_emu.stopping = true;
}

VideoOutputInfo App::GetVideoOutputInfo()
{
    //Everything about this function will have to be revisited
    //Logger::Log(LogCategory::Debug, "Joe's debug", "GetVideoOutputInfo");
    //if (m_emu.core.GetEmuState() == M64EMU_STOPPED)
    //    return {};

    VideoOutputInfo info{};

    //auto size = m_emu.core.GetVideoSize();
    //info.screen_width = static_cast<float>(size.uiWidth);
    //info.screen_height = static_cast<float>(size.uiHeight);

    retro_system_av_info rsai = *m_emu.core.GetSystemAvInfo();
    info.screen_width = rsai.geometry.base_width;
    info.screen_height = rsai.geometry.base_height;

    switch (m_emu.gfx_aspect) {
    default:
    case 0: // stretch
    case 3: // adjust
        info.width = info.screen_width;
        info.height = info.screen_height;
        break;
    case 1: // force 4:3
        if (info.screen_width * 3 / 4 > info.screen_height) {
			info.height = info.screen_height;
			info.width = info.screen_height * 4 / 3;
            info.left = (info.screen_width - info.width) / 2;
		}
        else if (info.screen_height * 4 / 3 > info.screen_width) {
			info.width = info.screen_width;
			info.height = info.screen_width * 3 / 4;
            info.top = (info.screen_height - info.height) / 2;
		}
        else {
			info.width = info.screen_width;
			info.height = info.screen_height;
		}
        break;
    case 2: // force 16:9
        if (info.screen_width * 9 / 16 > info.screen_height) {
			info.height = info.screen_height;
			info.width = info.screen_height * 16 / 9;
            info.left = (info.screen_width - info.width) / 2;
		}
        else if (info.screen_height * 16 / 9 > info.screen_width) {
			info.width = info.screen_width;
			info.height = info.screen_width * 9 / 16;
            info.top = (info.screen_height - info.height) / 2;
		}
        else {
			info.width = info.screen_width;
			info.height = info.screen_height;
		}
        break;
    }

    return info;
}

void App::UpdateVideoOutputSize()
{
    //Might as well throw this function into the TRASH
    //if (m_emu.core.GetEmuState() != M64EMU_RUNNING)
    //    return;

    //auto ws = ImGui::GetMainViewport()->GetWorkSize();
    //auto wsw = static_cast<int>(ws.x) / 4 * 4;
    //auto wsh = static_cast<int>(ws.y);

	//if (static_cast<int>(size.uiWidth) != wsw || static_cast<int>(size.uiHeight) != wsh)
    //    m_emu.core.GfxResizeOutput(static_cast<int>(wsw), static_cast<int>(wsh));
}

void App::ToggleFullScreen()
{
    //Logger::Log(LogCategory::Debug, "Joe's debug", "ToggleFullScreen");
    m_video.window.ToggleFullScreen();
}

void App::CaptureFrame()
{
    //Logger::Log(LogCategory::Debug, "Joe's debug", "CaptureFrame");
    //if (m_emu.core.GetEmuState() == M64EMU_STOPPED)
    //    return;

    //auto size = m_emu.core.GetVideoSize();

    retro_system_av_info rsai = *m_emu.core.GetSystemAvInfo();
    u32 w = rsai.geometry.base_width;
    u32 h = rsai.geometry.base_height;
    retro_2d_size size = {w,h};

	//if (static_cast<int>(size.uiWidth) != wsw || static_cast<int>(size.uiHeight) != wsh)
    //    m_emu.core.GfxResizeOutput(static_cast<int>(wsw), static_cast<int>(wsh));

    m_capture.width = static_cast<int>(size.uiWidth);
    m_capture.height = static_cast<int>(size.uiHeight);

    m_capture.pixels.resize(m_capture.width * m_capture.height * 3);
    //glReadBuffer(GL_BACK);
    //glReadPixels(0, 0, m_capture.width, m_capture.height, GL_RGB, GL_UNSIGNED_BYTE, m_capture.pixels.data());

    //glBindTexture(GL_TEXTURE_2D, m_capture.texture_id);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_capture.width, m_capture.height, 0,
    //    GL_RGB, GL_UNSIGNED_BYTE, m_capture.pixels.data());
}

void App::DispatchCoreEvents()
{
//    Logger::Log(LogCategory::Debug, "Joe's debug", "DispatchCoreEvents");
    std::lock_guard lock{m_emu.events_mutex};

    if (!m_emu.events.empty()) {
        for (auto& e : m_emu.events)
            OnCoreEvent.fire(e & 0xff, (e >> 8) & 0xff);

        m_emu.events.clear();
    }
}

void App::DispatchCoreStates()
{
    //Logger::Log(LogCategory::Debug, "Joe's debug", "DispatchCoreStates");
    std::lock_guard lock{m_emu.states_mutex};

    if (!m_emu.states.empty()) {
        for (const auto& p : m_emu.states)
            OnCoreStateChanged.fire(p.first, p.second);

        m_emu.states.clear();
    }
}

void App::DispatchEvents()
{
    DoEvents();

    DispatchCoreEvents();
    DispatchCoreStates();

    if (m_emu.notify_started) {
        OnCoreStarted.fire();
        m_emu.notify_started = false;
    }

    if (m_emu.notify_reset) {
        OnCoreReset.fire(m_emu.notify_reset == 2);
        m_emu.notify_reset = 0;
    }

    if (m_emu.stopping && m_emu.execute.wait_for(std::chrono::milliseconds{1}) == std::future_status::ready) {
        OnCoreStopped.fire();
        m_emu.stopping = false;
    }
}

void App::DestroyTextureLater(u32 id)
{
    //More uneeded functions to revisit
    //Logger::Log(LogCategory::Debug, "Joe's debug", "DestroyTextureLater");
    std::lock_guard lock{m_tex_to_destroy_mutex};
    m_tex_to_destroy.push_back(id);
}

void App::DestroyTextures()
{
    //More uneeded functions to revisit
    //Logger::Log(LogCategory::Debug, "Joe's debug", "DestroyTextures");
    std::lock_guard lock{m_tex_to_destroy_mutex};

    if (m_tex_to_destroy.empty())
        return;

    for (auto id : m_tex_to_destroy)
        Gfx::Texture::Destroy(id);

    m_tex_to_destroy.clear();
}

void App::BindingBeforeCreateResources()
{
    //Hmmmm, feels like something should be here
    //Logger::Log(LogCategory::Debug, "Joe's debug", "BindingBeforeCreateResources");
    //m_video.gl_context.MakeCurrent(m_video.window);
}

void App::BindingAfterCreateResources()
{
    //Logger::Log(LogCategory::Debug, "Joe's debug", "BindingAfterCreateResources");
    //SDL::GLContext::MakeCurrentNone();
}

void App::BindingBeforeRender()
{
    //SDL_UpdateTexture(texture_get_screen(),NULL,screen_data(),screen_pitch());
    if(!texture_get_screen() || !renderer_get())
        return;

    if (m_fonts.rebuild_atlas)
    {
        m_video.imgui.RebuildFontAtlas();
        m_fonts.rebuild_atlas = false;
    }

    if(resized)
    {
        ImGuiSDL::Deinitialize();
        ImGuiSDL::Initialize(renderer_get(),m_video.window.GetWidth(),m_video.window.GetHeight());
        resized = false;
    }

    //if(spr.size() >= 2)
      //  std::sort(spr.begin(),spr.end(),&HigherSprite);

    SDL_Rect a = {0,0,screen_w(),screen_h()};
    if(a.w>1&&a.h>1)
        while(a.w*2 < Frontend::App::GetInstance().GetMainWindow().GetWidth() && a.h*2 < Frontend::App::GetInstance().GetMainWindow().GetHeight())
        {
            a.w*=2;
            a.h*=2;
        };
    a.x += Frontend::App::GetInstance().GetMainWindow().GetWidth()/2;
    a.x -= a.w/2;
    a.y += Frontend::App::GetInstance().GetMainWindow().GetHeight()/2;
    a.y -= a.h/2;

    SDL_SetRenderTarget(renderer_get(),NULL);
    SDL_SetRenderDrawColor(renderer_get(),64,64,64,255);
    SDL_RenderFillRect(renderer_get(),NULL);

    if(SDL_SetRenderTarget(renderer_get(),texture_get())!=0)
        Logger::Log(LogCategory::Info,"ARGH",std::string("WHY!?!? ") + SDL_GetError());
    /*  This is not possible until the day you can set Color Keys to streamed textures, alternating to plan B,
    Sadly it will only work for some games D; Luckily, games at/after SNES era, can probably make their own puppets
    games in the NES era, will look alrightish, with these sprites slapped on
    for(std::vector<RETRO::Sprite*>::iterator i=spr.begin();i!=spr.end();i++)
        if(!(*i)->GetFG())
        {
            Logger::Log(LogCategory::Info,"Drawing Sprite BG","Sprite");
            (*i)->Draw();
        }*/
    SDL_RenderCopy(renderer_get(),texture_get_screen(),NULL,NULL);
    int xy=0;
    for(std::vector<RETRO::Sprite*>::iterator i=spr.begin();i!=spr.end();i++)
        if((*i)->GetFG())
        {
            xy++;
            Logger::Log(LogCategory::Info,"Drawing Sprite FG",std::string("Sprite ") + std::to_string(xy));
            (*i)->Draw();
        }
    
    SDL_SetRenderTarget(renderer_get(),NULL);
    core_render(a);

    m_video.imgui.NewFrame(m_video.window);

    ImGui::PushFont(m_fonts.base);

    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkPos(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->GetWorkSize());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.01f, 0.01f});
    ImGui::Begin("##main_overlay", nullptr,  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
    ImGui::PopStyleVar();
}

void App::BindingAfterRender()
{
    if(!texture_get_screen() || !renderer_get())
        return;
    m_any_item_active = ImGui::IsAnyItemActive();
    ImGui::End();

    ShowUtilWindows();

    ImGui::PopFont();

    m_video.imgui.EndFrame();

    //m_video.gl_context.MakeCurrent(m_video.window);
    //m_video.window.Swap();
    //SDL::GLContext::MakeCurrentNone();
    ++m_emu.elapsed_frames;
    
    SDL_SetRenderTarget(renderer_get(),NULL);
    core_present();
}

void App::CoreStartedHandler()
{
    m_emu.elapsed_frames = 0;
    //m_emu.gfx_aspect = m_emu.core.ConfigOpenSection("Video-GLideN64").GetIntOr("AspectRatio", 1);

    LoadROMCheats();
    InitUtilWindows();

    m_video.window.Show();
    m_video.window.Raise();
}

void App::CoreStoppedHandler()
{
    m_video.window.Hide();
    DeinitUtilWindows();
    OnCoreStopped.fire();
}

void App::CreateResourcesHandler()
{
    //auto bound_tex = Gfx::Texture::GetCurrentBound();
    //SDL::GLContext::MakeCurrentNone();

    OnCreateResources.fire();

    //m_video.gl_context.MakeCurrent(m_video.window);
    //glBindTexture(GL_TEXTURE_2D, bound_tex > 0 ? bound_tex : 0);
}

void App::NewFrameHandler()
{
    ++m_emu.elapsed_frames;
    OnNewFrame.fire();
}

void App::NewVIHandler()
{
    //DestroyTextures();

    //Crashes
    if (m_create_res_next) {
        CreateResourcesHandler();
        m_create_res_next = false;
    }

    /*if (m_take_shot) {
        m_take_shot = false;

        try {
            TakeScreenshot();
        }
        catch (const std::exception& e) {
            Logger::Log(LogCategory::Error, "Retro frontend", fmt::format("Screenshot not taken. {}", e.what()));
        }
    }*/

    //Not needed atm
    m_input.Update(GetCore());
    OnNewVI.fire();
}

void App::ResetHandler(bool do_hard_reset)
{
    m_emu.notify_reset = 1 + do_hard_reset;
}

void App::PauseLoopHandler()
{
    auto ticks = SDL_GetTicks();

    NewVIHandler();
    SwapHandler();

    if (SDL_GetTicks() - ticks < 16)
        SDL_Delay(16 - (SDL_GetTicks() - ticks));
}

void App::CoreEventHandler(int event)
{
    if (!HasEmuInputFocus())
        return;

    std::lock_guard lock{m_emu.events_mutex};
    m_emu.events.push_back(event);
}

void App::CoreStateChangedHandler(int param_type, int new_value)
{
    if (!m_emu.started && param_type == 1 && new_value == 2)
        m_emu.started = m_emu.notify_started = true;

    std::lock_guard lock{m_emu.states_mutex};
    m_emu.states.push_back(std::make_pair(param_type, new_value));
}

void App::DebugInitHandler()
{
    m_emu.debug_init = true;
    OnDebugInit.fire();
}

void App::DebugUpdateHandler(unsigned pc)
{
    OnDebugUpdate.fire(pc);
}

void App::SwapHandler()
{
    //auto bound_tex = Gfx::Texture::GetCurrentBound();
    //SDL::GLContext::MakeCurrentNone();

    OnViUpdate.fire();

    //if (m_emu.core.GetEmuState() != M64EMU_PAUSED)
    //m_video.gl_context.MakeCurrent(m_video.window);

    //glBindTexture(GL_TEXTURE_2D, bound_tex > 0 ? bound_tex : 0);
    //UpdateVideoOutputSize();
}

std::filesystem::path App::GetScreenshotPath()
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    //m64p_rom_settings settings;
    //m_emu.core.GetROMSettings(&settings);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H-%M-%S]") << " (" << m_emu.elapsed_frames << ") " << "ML64-LIBRETRO"/*settings.goodname*/ << ".png";
    auto filename = oss.str();

    //auto cache_path = m_emu.core.GetUserCachePath();
    //auto screenshot_path = std::filesystem::path{m_emu.core.ConfigOpenSection("Core").GetStringOr("ScreenshotPath", "")};

    //return screenshot_path.empty() ? cache_path / "screenshot" / filename : screenshot_path / filename;
    return filename;
}

void App::TakeScreenshot()
{
    auto screenshot_path = GetScreenshotPath();
    auto parent_path = screenshot_path.parent_path();

    if (!std::filesystem::exists(parent_path))
        std::filesystem::create_directory(parent_path);

    //auto width = m_video.window.GetWidth();
    //auto height = m_video.window.GetHeight();
    //const std::size_t pitch = width * 4;
    //std::vector<u8> buf(width * height * 4);
    //std::vector<u8> stride(pitch);

    //glReadBuffer(GL_BACK);
    //glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());

    /*for (int i{}; i < height / 2; ++i) {
        auto line_start = buf.data() + i * pitch;
        auto line_end = buf.data() + (height - 1 - i) * pitch;

        std::memcpy(stride.data(), line_start, pitch);
        std::memcpy(line_start, line_end, pitch);
        std::memcpy(line_end, stride.data(), pitch);
    }

    SDL::Surface s{buf.data(), width, height, 32, static_cast<int>(pitch), SDL_PIXELFORMAT_ABGR8888};
    s.SavePNG(GetScreenshotPath());*/
}
InputConf::InputMap *App::GetInputMap(int n)
{
    return map[n];
}

int App::CreateSprite(std::string fname,int w,int h,int cols,int rows)
{
    Logger::Log(LogCategory::Info,"The callback 1","Start!");
    int index = spr.size();
    spr.push_back(new RETRO::Sprite(renderer_get()));
    if(!spr[index]->LoadFromImage(fname,w,h,cols,rows))
    {
        Logger::Log(LogCategory::Error,"Failed to load sprite","Boo!");
        spr.pop_back();
        return -1;
    }
    Logger::Log(LogCategory::Info,"Loaded Sprite","Yay!");
    return index;
}
int App::CreateSprite(SDL_Surface *srf,int w,int h,int cols,int rows)
{
    Logger::Log(LogCategory::Info,"The callback 2","Start!");
    int index = spr.size();
    spr.push_back(new RETRO::Sprite(renderer_get()));
    if(!spr[index]->LoadFromSurface(srf,w,h,cols,rows))
    {
        spr.pop_back();
        return -1;
    }
    return index;
}
int App::CreateSprite(void *pixels,int pitch,int w,int h,int cols,int rows)
{
    Logger::Log(LogCategory::Info,"The callback 3","Start!");
    int index = spr.size();
    spr.push_back(new RETRO::Sprite(renderer_get()));
    if(!spr[index]->LoadFromBuffer(pixels,pitch,w,h,cols,rows))
    {
        spr.pop_back();
        return -1;
    }
    return index;
}
int App::CreateSprite(int index)
{
    int nindex = spr.size();
    spr.push_back(new RETRO::Sprite());
    spr[nindex]->Copy(spr[index]);
    Logger::Log(LogCategory::Debug,"Copying Sprite " + std::to_string(index),"To sprite " + std::to_string(nindex));
    return nindex;
}
RETRO::Sprite *App::GetSprite(int index)
{
    if(index >= 0 && index < spr.size())
        return spr[index];
    return NULL;
}
void App::RemoveSprite(int index)
{
    if(index >= 0 && index < spr.size())
    {
        delete spr[index];
        spr[index] = nullptr;
        for(int n=index;n<spr.size()-2;n++)
            spr[n] = spr[n+1];
        spr.pop_back();
    }
}
void App::PushBackCommand(std::string name,int id,std::vector<std::string> values,std::vector<int> ivalues)
{
    cmd.push_back({name,values,ivalues,id});
}

}