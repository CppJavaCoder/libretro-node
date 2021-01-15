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

#include <fmt/format.h>
//#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

static SDL_Renderer *g_rnd = NULL;
static SDL_Texture *g_txt = NULL;

static SDL_AudioDeviceID g_pcm = 0;
static struct retro_frame_time_callback runloop_frame_time;
static retro_usec_t runloop_frame_time_last = 0;
static const uint8_t *g_kbd = NULL;
static struct retro_audio_callback audio_callback;
static Uint8 g_format = 0;

static float g_scale = 3;
static bool running = true;

static bool resized = false;

static struct retro_variable *g_vars = NULL;



struct keymap {
	unsigned k;
	unsigned rk;
};

/*
static struct keymap g_binds[] = {
    { SDL_SCANCODE_X, RETRO_DEVICE_ID_JOYPAD_A },
    { SDL_SCANCODE_Z, RETRO_DEVICE_ID_JOYPAD_B },
    { SDL_SCANCODE_A, RETRO_DEVICE_ID_JOYPAD_Y },
    { SDL_SCANCODE_S, RETRO_DEVICE_ID_JOYPAD_X },
    { SDL_SCANCODE_UP, RETRO_DEVICE_ID_JOYPAD_UP },
    { SDL_SCANCODE_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN },
    { SDL_SCANCODE_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT },
    { SDL_SCANCODE_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT },
    { SDL_SCANCODE_RETURN, RETRO_DEVICE_ID_JOYPAD_START },
    { SDL_SCANCODE_BACKSPACE, RETRO_DEVICE_ID_JOYPAD_SELECT },
    { SDL_SCANCODE_Q, RETRO_DEVICE_ID_JOYPAD_L },
    { SDL_SCANCODE_W, RETRO_DEVICE_ID_JOYPAD_R },
    { 0, 0 }
};*/

static unsigned g_joy[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };

static void die(const char *fmt, ...) {
	char buffer[4096];

	va_list va;
	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

    Logger::Log(LogCategory::Error, "[Libretro]", buffer);

	exit(EXIT_FAILURE);
}

static bool video_set_pixel_format(unsigned format) {
	switch (format) {
	    //WARNING, untested
        case RETRO_PIXEL_FORMAT_0RGB1555:
            g_format = SDL_PIXELFORMAT_RGB555;
            break;
        case RETRO_PIXEL_FORMAT_XRGB8888:
            g_format = SDL_PIXELFORMAT_RGBX8888;
            break;
        case RETRO_PIXEL_FORMAT_RGB565:
            g_format = SDL_PIXELFORMAT_RGB565;
            break;
        default:
            die("Unknown pixel type %u", format);
	}

	return true;
}

static void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch) {
    if (data && data != RETRO_HW_FRAME_BUFFER_VALID) {
        SDL_UpdateTexture(g_txt,NULL,(void*)data,pitch);
        Frontend::App::GetInstance().SwapHandler();
        Frontend::App::GetInstance().NewFrameHandler();
	}
}

static void audio_init(int frequency) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;

    SDL_zero(desired);
    SDL_zero(obtained);

    desired.format = AUDIO_S16;
    desired.freq   = frequency;
    desired.channels = 2;
    desired.samples = 4096;

    g_pcm = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!g_pcm)
        die("Failed to open playback device: %s", SDL_GetError());

    SDL_PauseAudioDevice(g_pcm, 0);

    // Let the core know that the audio device has been initialized.
    if (audio_callback.set_state) {
        audio_callback.set_state(true);
    }
}


static void audio_deinit() {
    SDL_CloseAudioDevice(g_pcm);
}

static size_t audio_write(const int16_t *buf, unsigned frames) {
    SDL_QueueAudio(g_pcm, buf, sizeof(*buf) * frames * 2);
    return frames;
}


static void core_log(enum retro_log_level level, const char *fmt, ...) {
	char buffer[4096] = {0};
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (level == 0)
		return;

	switch(level)
	{
	    default:
            Logger::Log(LogCategory::Debug, "[Libretro]", buffer);
            break;
	    case 0:
            Logger::Log(LogCategory::Debug, "[Libretro]", buffer);
            break;
	    case 1:
            Logger::Log(LogCategory::Info, "[Libretro]", buffer);
            break;
	    case 2:
            Logger::Log(LogCategory::Warn, "[Libretro]", buffer);
            break;
	    case 3:
            Logger::Log(LogCategory::Error, "[Libretro]", buffer);
            break;
	}

	if (level == RETRO_LOG_ERROR)
		exit(EXIT_FAILURE);
}

static bool core_environment(unsigned cmd, void *data) {
	switch (cmd) {
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO: {
        const struct retro_subsystem_info *inf = (const struct retro_subsystem_info *)data;
        Logger::Log(LogCategory::Info,"Core Env",std::string("Desc ")+inf->desc);
        Logger::Log(LogCategory::Info,"Core Env",std::string("ID ")+std::to_string(inf->id));
        Logger::Log(LogCategory::Info,"Core Env",std::string("Ident ")+inf->ident);
        Logger::Log(LogCategory::Info,"Core Env",std::string("Num Roms ")+std::to_string(inf->num_roms));
        const struct retro_subsystem_rom_info *rom_inf = inf->roms;
        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM info block extract ")+(rom_inf->block_extract ? "True" : "False"));
        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM info desc ")+rom_inf->desc);
        const struct retro_subsystem_memory_info *mem_inf = rom_inf->memory;
        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM info need fullpath ")+(rom_inf->need_fullpath ? "True" : "False"));
        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM info num memory ")+std::to_string(rom_inf->num_memory));
        Logger::Log(LogCategory::Info,"Core Env",std::string("Rom info required")+(rom_inf->required ? "True" : "False"));
        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM info valid extentions ")+rom_inf->valid_extensions);

        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM memory info extentions ")+mem_inf->extension);
        Logger::Log(LogCategory::Info,"Core Env",std::string("ROM memory info type ")+std::to_string(mem_inf->type));

        return true;
    }
    case RETRO_ENVIRONMENT_SET_VARIABLES: {
        const struct retro_variable *vars = (const struct retro_variable *)data;
        size_t num_vars = 0;

        for (const struct retro_variable *v = vars; v->key; ++v) {
            num_vars++;
        }

        g_vars = (struct retro_variable*)calloc(num_vars + 1, sizeof(*g_vars));
        for (unsigned i = 0; i < num_vars; ++i) {
            const struct retro_variable *invar = &vars[i];
            struct retro_variable *outvar = &g_vars[i];

            const char *semicolon = strchr(invar->value, ';');
            const char *first_pipe = strchr(invar->value, '|');

            SDL_assert(semicolon && *semicolon);
            semicolon++;
            while (isspace(*semicolon))
                semicolon++;

            if (first_pipe) {
                outvar->value = (char*)malloc((first_pipe - semicolon) + 1);
                memcpy((char*)outvar->value, semicolon, first_pipe - semicolon);
                ((char*)outvar->value)[first_pipe - semicolon] = '\0';
            } else {
                outvar->value = strdup(semicolon);
            }

            outvar->key = strdup(invar->key);
            SDL_assert(outvar->key && outvar->value);
        }

        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *var = (struct retro_variable *)data;

        if (!g_vars)
            return false;

        for (const struct retro_variable *v = g_vars; v->key; ++v) {
            if (strcmp(var->key, v->key) == 0) {
                var->value = v->value;
                break;
            }
        }

        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
        bool *bval = (bool*)data;
		*bval = false;
        return true;
    }
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
		struct retro_log_callback *cb = (struct retro_log_callback *)data;
		cb->log = core_log;
        return true;
	}
	case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
		bool *bval = (bool*)data;
		*bval = true;
        return true;
    }
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
		const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;

		if (*fmt > RETRO_PIXEL_FORMAT_RGB565)
			return false;

		return video_set_pixel_format(*fmt);
	}
    case RETRO_ENVIRONMENT_SET_HW_RENDER: {
        //Todo learn opengl so I can properly add functionality like this
        //struct retro_hw_render_callback *hw = (struct retro_hw_render_callback*)data;
        //hw->get_current_framebuffer = core_get_current_framebuffer;
        //hw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
        //g_video.hw = *hw;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
        const struct retro_frame_time_callback *frame_time =
            (const struct retro_frame_time_callback*)data;
        runloop_frame_time = *frame_time;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
        struct retro_audio_callback *audio_cb = (struct retro_audio_callback*)data;
        audio_callback = *audio_cb;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char**)data;
        *dir = ".";
        return true;
    }
    case RETRO_ENVIRONMENT_SET_GEOMETRY: {
        const struct retro_game_geometry *geom = (const struct retro_game_geometry *)data;
        //geom->base_width; This is what our texture should be
        //geom->base_height;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
        Frontend::App::GetInstance().GetCore().SetNeedGameSupport(*(bool*)data);
        return true;
    }
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
        int *value = (int*)data;
        *value = 1 << 0 | 1 << 1;
        return true;
    }
	default:
		core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
		return false;
	}

    return false;
}


static void core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    video_refresh(data, width, height, pitch);
}


static void core_input_poll(void) {
	int i;
    std::vector<u8> input = Frontend::App::GetInstance().GetInputMap(0)->Update();

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_ESCAPE])
        running = false;

    //g_binds[0].k = Frontend::App::GetInstance().GetInput().GetButton(0,0);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_B,(bool)input[InputConf::MapUtil::MapIndex_B]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_X,(bool)input[InputConf::MapUtil::MapIndex_X]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_Y,(bool)input[InputConf::MapUtil::MapIndex_Y]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_DOWN,(bool)input[InputConf::MapUtil::MapIndex_YAxisDown]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_UP,(bool)input[InputConf::MapUtil::MapIndex_YAxisUp]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_LEFT,(bool)input[InputConf::MapUtil::MapIndex_XAxisLeft]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_RIGHT,(bool)input[InputConf::MapUtil::MapIndex_XAxisRight]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_START,(bool)input[InputConf::MapUtil::MapIndex_Start]);
    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_SELECT,(bool)input[InputConf::MapUtil::MapIndex_Select]);
//    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);
//    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);
//    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);

    Frontend::App::GetInstance().GetInput().Update(Frontend::App::GetInstance().GetCore());

	for (i = 0; i < RETRO_DEVICE_ID_JOYPAD_R3-1; i++)
    {
        if(i == RETRO_DEVICE_ID_JOYPAD_RIGHT
        || i == RETRO_DEVICE_ID_JOYPAD_LEFT
        || i == RETRO_DEVICE_ID_JOYPAD_DOWN
        || i == RETRO_DEVICE_ID_JOYPAD_UP)
            g_joy[i] = Frontend::App::GetInstance().GetInput().GetButton(0,i);
        else
            g_joy[i] = Frontend::App::GetInstance().GetInput().GetButtonDown(0,i);
        if(g_joy[i])
            Logger::Log(LogCategory::Info,"Ctrl","g_joy " + std::to_string(i));
    }
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	if (port || index || device != RETRO_DEVICE_JOYPAD)
		return 0;

	return g_joy[id];//Frontend::App::GetInstance().GetInput().GetButton(port,id);
}

static void core_audio_sample(int16_t left, int16_t right) {
	int16_t buf[2] = {left, right};
	audio_write(buf, 1);
}

static size_t core_audio_sample_batch(const int16_t *data, size_t frames) {
	return audio_write(data, frames);
}

retro_time_t cpu_features_get_time_usec(void) {
    return (retro_time_t)SDL_GetTicks();
}

static void noop() {}

namespace Frontend {

App::App()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "App");

    SDL_SetMainReady();

    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    m_sdl_init.sdl.Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    m_sdl_init.img.Init(IMG_INIT_PNG | IMG_INIT_JPG);
    m_sdl_init.ttf.Init();

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
    Logger::Log(LogCategory::Debug, "Joe's debug", "InitVideo");
    m_video.window = {info.window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, info.window_width, info.window_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN};

    g_rnd = SDL_CreateRenderer(m_video.window.Get(),0,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(g_txt == NULL)
        g_txt = SDL_CreateTexture(g_rnd,SDL_GetWindowPixelFormat(m_video.window.Get()),SDL_TEXTUREACCESS_TARGET,256,232);

    m_video.imgui.Initialize(m_video.window);

    auto font_atlas = ImGui::GetIO().Fonts;
    m_fonts.base = font_atlas->AddFontFromMemoryCompressedTTF(DroidSans_compressed_data, DroidSans_compressed_size, 18);
    m_fonts.mono = font_atlas->AddFontFromMemoryCompressedTTF(DroidSansMono_compressed_data, DroidSansMono_compressed_size, 18);
    m_video.imgui.RebuildFontAtlas();

    Frontend::App::GetInstance().NewVIHandler();
}

void App::DeinitVideo()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitVideo");
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
        Logger::Log(LogCategory::Debug, "Joe's debug", "ResetHandler");
        App::GetInstance().ResetHandler(do_hard_reset);
    }

    static void PauseLoopHandler()
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "PauseLoopHandler");
        App::GetInstance().PauseLoopHandler();
    }

    static void CoreEventHandler(int event)
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "CoreEventHandler");
        App::GetInstance().CoreEventHandler(event);
    }

    static void CoreStateChangedHandler(void*, int param_type, int new_value)
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "CoreStateChangedHandler");
        App::GetInstance().CoreStateChangedHandler(param_type, new_value);
    }

    static void DebugInitHandler()
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "DebugInitHandler");
        App::GetInstance().DebugInitHandler();
    }

    static void DebugUpdateHandler(unsigned pc)
    {
        Logger::Log(LogCategory::Debug, "Joe's debug", "DebugUpdateHandler");
        App::GetInstance().DebugUpdateHandler(pc);
    }
};

void App::InitEmu(const StartInfo& info)
{
    Logger::Log(LogCategory::Debug, "Joe's debug", std::string("InitEmu ") + std::string(info.retrolib.generic_string() + ".dll").c_str());

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

	//struct retro_system_info system = {0};
	//m_emu.core.GetSystemInfo(&system);

    struct retro_system_av_info av = {0};
	m_emu.core.GetSystemAvInfo(&av);

	audio_init(av.timing.sample_rate);
}

void App::DeinitEmu()
{
    audio_deinit();

    if (g_vars) {
        for (const struct retro_variable *v = g_vars; v->key; ++v) {
            free((char*)v->key);
            free((char*)v->value);
        }
        free(g_vars);
    }
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
                running = false;
                OnWindowClosing.fire();
            }
            else if (e.type == SDL_WINDOWEVENT && e.window.windowID == m_video.window.GetId()) {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    OnWindowClosing.fire();
                    running = false;
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "LoadCheats");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "SaveCheats");
    try {
        RETRO::Cheat::Save(m_emu.data_dir / "libretro_user.txt", m_cheats.user_map);
    }
    catch (const std::exception& e) {
        Logger::Log(LogCategory::Error, "Retro frontend", fmt::format("User cheats not saved. {}", e.what()));
    }
}

void App::LoadROMCheats()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "LoadROMCheats");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "InitUtilWindows");
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

    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitUtilWindows1");
    m_util_win.input_conf->OnClosed.disconnect<&App::InputConfigClosedHandler>(this);
    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitUtilWindows2");
    m_util_win.input_conf->SaveConfig(m_emu.core);
    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitUtilWindows3");
    m_util_win.input_conf.reset();

    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitUtilWindows4");
    // Currently Crashes
    //m_util_win.cheat_conf->DisableAllEntries();
    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitUtilWindows5");
    m_util_win.cheat_conf.reset();

    Logger::Log(LogCategory::Debug, "Joe's debug", "DeinitUtilWindows6");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "Startup");
    InitVideo(info);
    // Not currently needed afaik
    //InitFrameCapture();
    InitEmu(info);
    CoreStartedHandler();
    LoadCheats();

        CreateResourcesNextVi();

        // Update the game loop timer.
        if (runloop_frame_time.callback) {
            retro_time_t current = cpu_features_get_time_usec();
            retro_time_t delta = current - runloop_frame_time_last;

            if (!runloop_frame_time_last)
                delta = runloop_frame_time.reference;
            runloop_frame_time_last = current;
            runloop_frame_time.callback(delta * 1000);
        }

        // Ask the core to emit the audio.
        if (audio_callback.callback) {
            audio_callback.callback();
        }
}

void App::Shutdown()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "Shutdown");
    SaveCheats();
    DeinitVideo();
    DeinitEmu();
    Logger::Log(LogCategory::Debug, "Joe's debug", "Shutdown End");
}

void App::Execute()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "Execute");

    m_emu.execute = std::async(std::launch::async,[this]() {
    try{
        ImGuiSDL::Initialize(g_rnd,inf.window_width,inf.window_height);
        while(running){
            m_emu.core.Run();
        }
        ImGuiSDL::Deinitialize();
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "Stop");
    //m_emu.core.Stop();
    m_emu.stopping = true;
}

VideoOutputInfo App::GetVideoOutputInfo()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "GetVideoOutputInfo");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "UpdateVideoOutputSize");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "ToggleFullScreen");
    m_video.window.ToggleFullScreen();
}

void App::CaptureFrame()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "CaptureFrame");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "DestroyTextureLater");
    std::lock_guard lock{m_tex_to_destroy_mutex};
    m_tex_to_destroy.push_back(id);
}

void App::DestroyTextures()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "DestroyTextures");
    std::lock_guard lock{m_tex_to_destroy_mutex};

    if (m_tex_to_destroy.empty())
        return;

    for (auto id : m_tex_to_destroy)
        Gfx::Texture::Destroy(id);

    m_tex_to_destroy.clear();
}

void App::BindingBeforeCreateResources()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "BindingBeforeCreateResources");
    //m_video.gl_context.MakeCurrent(m_video.window);
}

void App::BindingAfterCreateResources()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "BindingAfterCreateResources");
    //SDL::GLContext::MakeCurrentNone();
}

void App::BindingBeforeRender()
{
    SDL_Rect a;
    a.x = 0;
    a.y = 0;
    a.w = 256;
    a.h = 232;
    int width,height;
    while(a.w*2 < m_video.window.GetWidth() && a.h*2 < m_video.window.GetHeight())
    {
        a.w*=2;
        a.h*=2;
    };
    a.x += m_video.window.GetWidth()/2;
    a.x -= a.w/2;
    a.y += m_video.window.GetHeight()/2;
    a.y -= a.h/2;
    m_video.window.GetHeight();

    //m_video.gl_context.MakeCurrent(m_video.window);

    //if (m_emu.core.GetEmuState() == M64EMU_PAUSED) {
    //    glScissor(0, 0, m_video.window.GetWidth(), m_video.window.GetHeight());
    //    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //    glClear(GL_COLOR_BUFFER_BIT);
    //}

    if (m_fonts.rebuild_atlas) {
        m_video.imgui.RebuildFontAtlas();
        m_fonts.rebuild_atlas = false;
    }

    if(resized)
    {
        ImGuiSDL::Deinitialize();
        ImGuiSDL::Initialize(g_rnd,m_video.window.GetWidth(),m_video.window.GetHeight());
        resized = false;
    }


    SDL_SetRenderTarget(g_rnd,NULL);
    SDL_SetRenderDrawColor(g_rnd,64,64,64,255);
    SDL_RenderFillRect(g_rnd,NULL);
    SDL_RenderCopy(g_rnd,g_txt,NULL,&a);

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
    m_any_item_active = ImGui::IsAnyItemActive();
    ImGui::End();

    ShowUtilWindows();

    ImGui::PopFont();

    m_video.imgui.EndFrame();

    //m_video.gl_context.MakeCurrent(m_video.window);
    //m_video.window.Swap();
    //SDL::GLContext::MakeCurrentNone();
    ++m_emu.elapsed_frames;

    SDL_RenderPresent(g_rnd);
}

void App::CoreStartedHandler()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "CoreStartedHandler");
    m_emu.elapsed_frames = 0;
    //m_emu.gfx_aspect = m_emu.core.ConfigOpenSection("Video-GLideN64").GetIntOr("AspectRatio", 1);

    LoadROMCheats();
    InitUtilWindows();

    m_video.window.Show();
    m_video.window.Raise();
}

void App::CoreStoppedHandler()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "CoreStoppedHandler");
    m_video.window.Hide();
    DeinitUtilWindows();
    OnCoreStopped.fire();
}

void App::CreateResourcesHandler()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "CreateResourcesHandler");
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
    //Logger::Log(LogCategory::Debug, "Joe's debug", "NewVIHandler");
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
    Logger::Log(LogCategory::Debug, "Joe's debug", "ResetHandler");
    m_emu.notify_reset = 1 + do_hard_reset;
}

void App::PauseLoopHandler()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "PauseLoopHandler");
    auto ticks = SDL_GetTicks();

    NewVIHandler();
    SwapHandler();

    if (SDL_GetTicks() - ticks < 16)
        SDL_Delay(16 - (SDL_GetTicks() - ticks));
}

void App::CoreEventHandler(int event)
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "CoreEventHandler");
    if (!HasEmuInputFocus())
        return;

    std::lock_guard lock{m_emu.events_mutex};
    m_emu.events.push_back(event);
}

void App::CoreStateChangedHandler(int param_type, int new_value)
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "CoreStateChangedHandler");
    if (!m_emu.started && param_type == 1 && new_value == 2)
        m_emu.started = m_emu.notify_started = true;

    std::lock_guard lock{m_emu.states_mutex};
    m_emu.states.push_back(std::make_pair(param_type, new_value));
}

void App::DebugInitHandler()
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "DebugInitHandler");
    m_emu.debug_init = true;
    OnDebugInit.fire();
}

void App::DebugUpdateHandler(unsigned pc)
{
    Logger::Log(LogCategory::Debug, "Joe's debug", "DebugUpdateHandler");
    OnDebugUpdate.fire(pc);
}

void App::SwapHandler()
{
    //Logger::Log(LogCategory::Debug, "Joe's debug", "SwapHandler");
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

}
