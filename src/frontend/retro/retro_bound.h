#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <libretro.h>
#include "common/types.h"
#include "frontend/app.h"

static void video_init(void);
static void audio_init(int frequency);
static void audio_deinit();
static SDL_Renderer *renderer_get(void);
static SDL_Texture *texture_get(void);
static SDL_Texture *texture_get_screen(void);
static void variables_free(void);
static void video_deinit(void);
static u32 format_get(void);

Uint32 screen_w(void);
Uint32 screen_h(void);

static void core_stop(void);
static bool core_environment(unsigned cmd, void *data);
static void core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);
static void core_input_poll(void);
static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id);
static void core_audio_sample(int16_t left, int16_t right);
static size_t core_audio_sample_batch(const int16_t *data, size_t frames);
retro_time_t cpu_features_get_time_usec(void);
static void core_render(const SDL_Rect &a);
static void core_present(void);
static void core_refresh(void);
static bool core_is_running(void);
