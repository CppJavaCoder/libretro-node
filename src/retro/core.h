#pragma once

#include <libretro.h>

#include "common/types.h"
#include "retro/version.h"

#include <filesystem>
#include <string>
#include <vector>


namespace RETRO
{

class Core {
public:

    Core();
    ~Core();

    void LoadCore(const char *file);
    void UnloadCore();
    void *GetHandle() const;

    void Init();
    void Deinit();
    unsigned GetAPIVersion();
    retro_system_info *GetSystemInfo();
	retro_system_av_info *GetSystemAvInfo();
    retro_system_info *GetSystemInfo(retro_system_info *inf);
	retro_system_av_info *GetSystemAvInfo(retro_system_av_info *inf);
	void SetControllerPortDevice(unsigned port, unsigned device);
	void Reset(void);
	void Run(void);
	size_t SerializeSize(void);
	bool Serialize(void *data, size_t size);
	bool Unserialize(const void *data, size_t size);
	void CheatReset(void);
	void CheatSet(unsigned index, bool enabled, const char *code);
	bool LoadGame(const struct retro_game_info *game);
	bool LoadGameSpecial(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void UnloadGame();
	unsigned GetRegion(void);
	retro_game_info *GetGameInfo();

	void SetNeedGameSupport(bool s);
	bool GetNeedGameSupport();

    void (*SetEnvironment)(retro_environment_t);
    void (*SetVideoRefresh)(retro_video_refresh_t);
    void (*SetInputPoll)(retro_input_poll_t);
    void (*SetInputState)(retro_input_state_t);
    void (*SetAudioSample)(retro_audio_sample_t);
    void (*SetAudioSampleBatch)(retro_audio_sample_batch_t);

    void Pause();
    void Resume();

    void SetStateSlot(int slot);
    void LoadState();
    void LoadState(int slot);
    void LoadState(const std::filesystem::path& path);
    void SaveState();
    void SaveState(int slot);
    void SaveState(const std::filesystem::path& path);
    void AdvanceFrame();

    u8* GetDRAMPtr();
    std::size_t GetDRAMSize();
    u8* GetROMPtr();
    std::size_t GetROMSize();

    u8 RDRAMRead8(u32 addr);
    u16 RDRAMRead16(u32 addr);
    u32 RDRAMRead32(u32 addr);
    u8* RDRAMReadBuffer(u32 addr, std::size_t len);
    void RDRAMWrite8(u32 addr, u8 val);
    void RDRAMWrite16(u32 addr, u16 val);
    void RDRAMWrite32(u32 addr, u32 val);
    void RDRAMWriteBuffer(u32 addr, u8* buf, std::size_t len);
    u8 ROMRead8(u32 addr);
    u16 ROMRead16(u32 addr);
    u32 ROMRead32(u32 addr);
    u8* ROMReadBuffer(u32 addr, std::size_t len);
    void ROMWrite8(u32 addr, u8 val);
    void ROMWrite16(u32 addr, u16 val);
    void ROMWrite32(u32 addr, u32 val);
    void ROMWriteBuffer(u32 addr, u8* buf, std::size_t len);

    void ContSetInput(u32 button,unsigned int value);
    unsigned int ContGetInput(u32 button);

    void SetGame(void *data,std::size_t size);

private:
    bool support_no_game;
    bool pause;
    struct retro_game_info currentGame;
    u8 *gameData;
    std::size_t gameSize;

    //Honestly... I'm stupid and have no idea how big this should be
    u32 ctrl_b[128];

    struct {
        void *handle;
        bool initialized;
        bool supports_no_game;

        void (*retro_init)(void);
        void (*retro_deinit)(void);
        unsigned (*retro_api_version)(void);
        void (*retro_get_system_info)(struct retro_system_info *info);
        void (*retro_get_system_av_info)(struct retro_system_av_info *info);
        void (*retro_set_controller_port_device)(unsigned port, unsigned device);
        void (*retro_reset)(void);
        void (*retro_run)(void);
        size_t (*retro_serialize_size)(void);
        bool (*retro_serialize)(void *data, size_t size);
        bool (*retro_unserialize)(const void *data, size_t size);
        void (*retro_cheat_reset)(void);
        void (*retro_cheat_set)(unsigned index, bool enabled, const char *code);
        bool (*retro_load_game)(const struct retro_game_info *game);
        bool (*retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
        void (*retro_unload_game)(void);
        unsigned (*retro_get_region)(void);
        void *(*retro_get_memory_data)(unsigned id);
        size_t (*retro_get_memory_size)(unsigned id);
    }m_retro;
};

}
