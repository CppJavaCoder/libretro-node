#pragma once

#include <libretro.h>

#include "common/types.h"
#include "retro/version.h"

#include <filesystem>
#include <fstream>
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
    void Startup(const std::filesystem::path &config,const std::filesystem::path &data);
    void Deinit();
    unsigned GetAPIVersion();
    retro_system_info *GetSystemInfo();
	retro_system_av_info *GetSystemAvInfo();
    void GetSystemInfo(retro_system_info *inf);
	void GetSystemAvInfo(retro_system_av_info *inf);
	void SetControllerPortDevice(unsigned port, unsigned device);
	void Reset(void);
	void Run(void);
	size_t SerializeSize(void);
	bool Serialize(void *data, size_t size);
	bool Unserialize(const void *data, size_t size);
	void CheatReset(void);
	void CheatSet(unsigned index, bool enabled, const char *code);
	bool LoadGame(const std::string &s);
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

    u8 RDRAMRead8(u32 addr,int type = RETRO_MEMORY_SYSTEM_RAM);
    u16 RDRAMRead16(u32 addr,int type = RETRO_MEMORY_SYSTEM_RAM);
    u32 RDRAMRead32(u32 addr,int type = RETRO_MEMORY_SYSTEM_RAM);
    u8* RDRAMReadBuffer(u32 addr, std::size_t len,int type = RETRO_MEMORY_SYSTEM_RAM);
    void RDRAMWrite8(u32 addr, u8 val,int type = RETRO_MEMORY_SYSTEM_RAM);
    void RDRAMWrite16(u32 addr, u16 val,int type = RETRO_MEMORY_SYSTEM_RAM);
    void RDRAMWrite32(u32 addr, u32 val,int type = RETRO_MEMORY_SYSTEM_RAM);
    void RDRAMWriteBuffer(u32 addr, u8* buf, std::size_t len,int type = RETRO_MEMORY_SYSTEM_RAM);
    u8 ROMRead8(u32 addr);
    u16 ROMRead16(u32 addr);
    u32 ROMRead32(u32 addr);
    u8* ROMReadBuffer(u32 addr, std::size_t len);
    void ROMWrite8(u32 addr, u8 val);
    void ROMWrite16(u32 addr, u16 val);
    void ROMWrite32(u32 addr, u32 val);
    void ROMWriteBuffer(u32 addr, u8* buf, std::size_t len);

    void ContSetInput(u32 controller,unsigned int value);
    unsigned int ContGetInput(u32 controller);

    //void SetGame(void *data,std::size_t size);

    void ConfigSaveFile();
    bool ConfigHasUnsavedChanges();
    std::vector<std::string> ConfigListSections();

    struct RetroHeader{
        enum System
        {
            NES, SNES, GENESIS, UNKNOWN
        };
        System sys;
        std::string name;
    };

    std::string GetROMHeader();
    void GetRetroHeader(RetroHeader *hdr);

    const std::filesystem::path GetSaveDir();
    void SetSaveDir(const std::filesystem::path &path);

    bool SaveGameData();
    bool LoadGameData();

    class ConfigSection {
        friend Core;

    public:

        enum RetroType
        {
            Int, Float, Bool, String, Unknown
        };

        struct Param {
            std::string name, value;
            RetroType type;
        };

        std::string GetName();
        std::vector<Param> ListParams();
        void Save();
        bool HasUnsavedChanges();
        void Erase(bool clprm = true);
        void RevertChanges();

        std::string GetHelp(const std::string& name);
        void SetHelp(const std::string& name, const std::string& help);
        RetroType GetType(const std::string& name);

        void SetDefaultInt(const std::string& name, int value, const std::string& help = "");
        void SetDefaultFloat(const std::string& name, float value, const std::string& help = "");
        void SetDefaultBool(const std::string& name, bool value, const std::string& help = "");
        void SetDefaultString(const std::string& name, const std::string& value, const std::string& help = "");

        int GetInt(const std::string& name);
        int GetIntOr(const std::string& name, int value);
        void SetInt(const std::string& name, int value);
        float GetFloat(const std::string& name);
        float GetFloatOr(const std::string& name, float value);
        void SetFloat(const std::string& name, float value);
        bool GetBool(const std::string& name);
        bool GetBoolOr(const std::string& name, bool value);
        void SetBool(const std::string& name, bool value);
        std::string GetString(const std::string& name);
        std::string GetStringOr(const std::string& name, const std::string& value);
        void SetString(const std::string& name, const std::string& value);

        ConfigSection(Core& core, const std::string& name);
    private:
        void AddToParams(std::string name,std::string val);
        Core* m_core;
        std::string m_name;
        std::vector<Param> params;
    };

    ConfigSection ConfigOpenSection(const std::string& name);
    ConfigSection OpenSection(const std::string &fname, const std::string& name);

    std::filesystem::path GetSharedDataFilePath(const std::string& file);
    std::filesystem::path GetUserConfigPath();
    std::filesystem::path GetUserDataPath();
    std::filesystem::path GetUserCachePath();

    bool GameLoaded();
    void ReLoadGame();

private:
    RetroHeader::System sys;
    struct retro_game_info ginf;
    bool support_no_game;
    bool pause, gameLoaded;
    std::vector<u8> gameData;
    //std::size_t gameSize;

    int saveSlot;
    std::vector<u8*> states;

    bool changes;
    std::fstream mfile;

    std::filesystem::path config_dir,data_dir,save_dir;
    std::string lastpath;
    //std::fstream config_file,data_file;

    unsigned int ctrl_b[4];

    retro_system_info sys_inf;
    retro_system_av_info sys_av_inf;

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
