#include "retro_bound.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "common/logger.h"
#include "sdl/window.h"

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

static unsigned g_joy[4][(RETRO_DEVICE_ID_JOYPAD_R3+1)] = { 0 };

static void die(const char *fmt, ...) {
	char buffer[4096];

	va_list va;
	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

    Logger::Log(LogCategory::Error, "[Libretro]", buffer);

	exit(EXIT_FAILURE);
}

static void video_init(void)
{
    g_rnd = SDL_CreateRenderer(Frontend::App::GetInstance().GetMainWindow().Get(),0,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(g_txt == NULL)
        g_txt = SDL_CreateTexture(g_rnd,SDL_GetWindowPixelFormat(Frontend::App::GetInstance().GetMainWindow().Get()),SDL_TEXTUREACCESS_TARGET,256,232);
}

static SDL_Renderer *renderer_get(void)
{
    return g_rnd;
}

static void core_stop(void)
{
    running = false;
}

static void variables_free(void)
{
    if (g_vars) {
        for (const struct retro_variable *v = g_vars; v->key; ++v) {
            free((char*)v->key);
            free((char*)v->value);
        }
        free(g_vars);
    }
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
//    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: {
//        return true;
//    }
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

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_ESCAPE])
        running = false;

    for(int n = 0; n < 4; n++)
    {
        std::vector<u8> input = Frontend::App::GetInstance().GetInputMap(n)->Update();

        for(int i = 0; i < InputConf::MapUtil::MapIndex_Count; i++)
            Frontend::App::GetInstance().GetInput().SetButton(n,i,false);
        //g_binds[0].k = Frontend::App::GetInstance().GetInput().GetButton(0,0);
        if(Frontend::App::GetInstance().GetInputMap(n)->IsPlugged())
        {
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]||(bool)input[InputConf::MapUtil::MapIndex_CRight]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_B,(bool)input[InputConf::MapUtil::MapIndex_B]||(bool)input[InputConf::MapUtil::MapIndex_CDown]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_DOWN,(bool)input[InputConf::MapUtil::MapIndex_YAxisDown]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_UP,(bool)input[InputConf::MapUtil::MapIndex_YAxisUp]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_LEFT,(bool)input[InputConf::MapUtil::MapIndex_XAxisLeft]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_RIGHT,(bool)input[InputConf::MapUtil::MapIndex_XAxisRight]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_START,(bool)input[InputConf::MapUtil::MapIndex_Start]);
            Frontend::App::GetInstance().GetInput().SetButton(n,RETRO_DEVICE_ID_JOYPAD_SELECT,(bool)input[InputConf::MapUtil::MapIndex_TrigZ]||(bool)input[InputConf::MapUtil::MapIndex_TrigR]||(bool)input[InputConf::MapUtil::MapIndex_TrigL]);
        //    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);
        //    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);
        //    Frontend::App::GetInstance().GetInput().SetButton(0,RETRO_DEVICE_ID_JOYPAD_A,(bool)input[InputConf::MapUtil::MapIndex_A]);
        }

        Frontend::App::GetInstance().GetInput().Update(Frontend::App::GetInstance().GetCore());

        for (int i = 0; i < RETRO_DEVICE_ID_JOYPAD_R3-1; i++)
        {
            if(i == RETRO_DEVICE_ID_JOYPAD_RIGHT
            || i == RETRO_DEVICE_ID_JOYPAD_LEFT
            || i == RETRO_DEVICE_ID_JOYPAD_DOWN
            || i == RETRO_DEVICE_ID_JOYPAD_UP)
                g_joy[n][i] = Frontend::App::GetInstance().GetInput().GetButton(n,i);
            else
                g_joy[n][i] = Frontend::App::GetInstance().GetInput().GetButtonDown(n,i);
        }
    }
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	if (device != RETRO_DEVICE_JOYPAD)
		return 0;
	return g_joy[port][id];
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

static void core_render(void)
{
    SDL_Rect a;
    a.x = 0;
    a.y = 0;
    a.w = 256;
    a.h = 232;
    while(a.w*2 < Frontend::App::GetInstance().GetMainWindow().GetWidth() && a.h*2 < Frontend::App::GetInstance().GetMainWindow().GetHeight())
    {
        a.w*=2;
        a.h*=2;
    };
    a.x += Frontend::App::GetInstance().GetMainWindow().GetWidth()/2;
    a.x -= a.w/2;
    a.y += Frontend::App::GetInstance().GetMainWindow().GetHeight()/2;
    a.y -= a.h/2;
    Frontend::App::GetInstance().GetMainWindow().GetHeight();

    SDL_SetRenderTarget(g_rnd,NULL);
    SDL_SetRenderDrawColor(g_rnd,64,64,64,255);
    SDL_RenderFillRect(g_rnd,NULL);
    SDL_RenderCopy(g_rnd,g_txt,NULL,&a);
}
static void core_present(void)
{
    SDL_RenderPresent(g_rnd);
}

static void core_refresh(void)
{
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

static bool core_is_running(void)
{
    return running;
}