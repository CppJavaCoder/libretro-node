#include "retro/core.h"
#include "retro/error.h"
//#include "sdl/shared_object.h"
#include <SDL2/SDL_loadso.h>

#include <fmt/format.h>

#include <sstream>

//#define LOADFUNC(func) func = m_so.LoadFunction<ptr_##func>(#func)

//Todo, find a C++ way... (I know I'm dumb okay!!!)

#define load_sym(V, S) do {\
    if (!((*(void**)&V) = SDL_LoadFunction(m_retro.handle, #S))) \
        printf("Failed to load symbol '" #S "'': %s", SDL_GetError()); \
	} while (0)
#define load_retro_sym(S) load_sym(m_retro.S, S)

namespace RETRO
{

inline void Checked(const char* function_name, retro_error error_code)
{
//  @TODO fix this thing
//    if (error_code != SUCCESS)
//        throw Error{function_name, error_code};
}

Core::Core() {
    SetEnvironment = NULL;
    SetVideoRefresh = NULL;
    SetInputPoll = NULL;
    SetInputState = NULL;
    SetAudioSample = NULL;
    SetAudioSampleBatch = NULL;
    pause = false;
    currentGame.data = NULL;
    currentGame.meta = "";
    currentGame.path = "";
    currentGame.size = 0;
    gameSize = 0;
    gameData = nullptr;
    changes = false;
}

Core::~Core()
{
    if(gameData != nullptr)
    {
        delete gameData;
        gameData = nullptr;
    }
};

void Core::LoadCore(const char *file)
{
    memset(&m_retro, 0, sizeof(m_retro));
    m_retro.handle = SDL_LoadObject(file);

    if (!m_retro.handle)
        return;

    load_retro_sym(retro_init);
    load_retro_sym(retro_deinit);
    load_retro_sym(retro_api_version);
    load_retro_sym(retro_get_system_info);
    load_retro_sym(retro_get_system_av_info);
    load_retro_sym(retro_set_controller_port_device);
    load_retro_sym(retro_reset);
    load_retro_sym(retro_run);
    load_retro_sym(retro_serialize_size);
    load_retro_sym(retro_serialize);
    load_retro_sym(retro_unserialize);
    load_retro_sym(retro_cheat_set);
    load_retro_sym(retro_cheat_reset);
    load_retro_sym(retro_load_game);
    load_retro_sym(retro_unload_game);
    load_retro_sym(retro_get_region);
    load_retro_sym(retro_get_memory_data);
    load_retro_sym(retro_get_memory_size);

    load_sym(SetEnvironment, retro_set_environment);
    load_sym(SetVideoRefresh, retro_set_video_refresh);
    load_sym(SetInputPoll, retro_set_input_poll);
    load_sym(SetInputState, retro_set_input_state);
    load_sym(SetAudioSample, retro_set_audio_sample);
    load_sym(SetAudioSampleBatch, retro_set_audio_sample_batch);

    //m_retro.retro_init();

    //SetEnvironment(core_environment);
    //SetVideoRefresh(core_video_refresh);
    //SetInputPoll(core_input_poll);
    //SetInputState(core_input_state);
    //SetAudioSample(core_audio_sample);
    //SetAudioSampleBatch(core_audio_sample_batch);
}

void Core::UnloadCore()
{
    SDL_UnloadObject(m_retro.handle);
}

void *Core::GetHandle() const
{
    return m_retro.handle;
}

void Core::Init()
{
    m_retro.retro_init();
}
void Core::Startup(const std::filesystem::path &config,const std::filesystem::path &data)
{
    config_dir = config;
    data_dir = data;
    if(mfile.is_open())
        mfile.close();
    mfile.open(config_dir/".ini");
}
void Core::Deinit()
{
    m_retro.retro_deinit();
}
unsigned Core::GetAPIVersion()
{
    return m_retro.retro_api_version();
}
retro_system_info *Core::GetSystemInfo()
{
    m_retro.retro_get_system_info(&sys_inf);
    return &sys_inf;
}
retro_system_av_info *Core::GetSystemAvInfo()
{
    m_retro.retro_get_system_av_info(&sys_av_inf);
    return &sys_av_inf;
}
void Core::GetSystemInfo(retro_system_info *inf)
{
    m_retro.retro_get_system_info(inf);
}
void Core::GetSystemAvInfo(retro_system_av_info *inf)
{
    m_retro.retro_get_system_av_info(inf);
}
void Core::SetControllerPortDevice(unsigned port, unsigned device)
{
    m_retro.retro_set_controller_port_device(port,device);
}
void Core::Reset()
{
    m_retro.retro_reset();
}
void Core::Run()
{
    if(!pause)
        m_retro.retro_run();
}
size_t Core::SerializeSize()
{
    return m_retro.retro_serialize_size();
}
bool Core::Serialize(void *data, size_t size)
{
    return m_retro.retro_serialize(data,size);
}
bool Core::Unserialize(const void *data, size_t size)
{
    return m_retro.retro_unserialize(data,size);
}
void Core::CheatReset()
{
    m_retro.retro_cheat_reset();
}
void Core::CheatSet(unsigned index, bool enabled, const char *code)
{
    Logger::Log(LogCategory::Debug,std::string("Cheat Code ")+std::to_string(index)+" "+code,std::string("Is ") + (enabled ? "Enabled" : "Disabled"));
    m_retro.retro_cheat_set(index,enabled,code);
}
bool Core::LoadGame(const struct retro_game_info *game)
{
    currentGame.data = game->data;
    currentGame.meta= game->meta;
    currentGame.path = game->path;
    currentGame.size = game->size;
    return m_retro.retro_load_game(game);
}
bool Core::LoadGameSpecial(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    currentGame.data = info->data;
    currentGame.meta = info->meta;
    currentGame.path = info->path;
    currentGame.size = info->size;
    return m_retro.retro_load_game_special(game_type,info,num_info);
}
void Core::UnloadGame()
{
    m_retro.retro_unload_game();
}
unsigned Core::GetRegion()
{
    return m_retro.retro_get_region();
}
retro_game_info *Core::GetGameInfo()
{
    return &currentGame;
}
void Core::SetNeedGameSupport(bool s)
{
    m_retro.supports_no_game = s;
}
bool Core::GetNeedGameSupport()
{
    return m_retro.supports_no_game;
}
void Core::Pause()
{
    pause = true;
}
void Core::Resume()
{
    pause = false;
}

void Core::SetStateSlot(int slot)
{
    //Todo, Implement SaveStates
}
void Core::LoadState()
{
    //Todo, Implement SaveStates
}
void Core::LoadState(int slot)
{
    //Todo, Implement SaveStates
}
void Core::LoadState(const std::filesystem::path& path)
{
    //Todo, Implement SaveStates
}
void Core::SaveState()
{
    //Todo, Implement SaveStates
}
void Core::SaveState(int slot)
{
    //Todo, Implement SaveStates
}
void Core::SaveState(const std::filesystem::path& path)
{
    //Todo, Implement SaveStates
}
void Core::AdvanceFrame()
{
    Run();
}

u8* Core::GetDRAMPtr()
{
    return (u8*)m_retro.retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
}
std::size_t Core::GetDRAMSize()
{
    return m_retro.retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
}
u8* Core::GetROMPtr()
{
    return (u8*)gameData;
}
std::size_t Core::GetROMSize()
{
    return gameSize;
}

u8 Core::RDRAMRead8(u32 addr)
{
    return ((u8*)m_retro.retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM))[addr];
}
u16 Core::RDRAMRead16(u32 addr)
{
    return RDRAMRead8(addr+1) << 8 + RDRAMRead8(addr);
}
u32 Core::RDRAMRead32(u32 addr)
{
    return RDRAMRead16(addr+2) << 16 + RDRAMRead16(addr);
}
u8* Core::RDRAMReadBuffer(u32 addr, std::size_t len)
{
    u8 *buf = new u8[len];
    for(int n=0;n<len;n++)
        buf[n] = RDRAMRead8(addr+n);
    return buf;
}
void Core::RDRAMWrite8(u32 addr, u8 val)
{
    ((u8*)m_retro.retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM))[addr] = val;
}
void Core::RDRAMWrite16(u32 addr, u16 val)
{
    RDRAMWrite8(addr,(u8)val);
    RDRAMWrite8(addr+1,(u8)val);
}
void Core::RDRAMWrite32(u32 addr, u32 val)
{
    RDRAMWrite16(addr,(u8)val);
    RDRAMWrite16(addr+2,(u8)val);
}
void Core::RDRAMWriteBuffer(u32 addr, u8* buf, std::size_t len)
{
    for(int n=0;n<len;n++)
        RDRAMWrite8(addr+n,buf[n]);
}
u8 Core::ROMRead8(u32 addr)
{
    if(gameData != NULL)
        return gameData[addr];
    return 0;
}
u16 Core::ROMRead16(u32 addr)
{
    return ROMRead8(addr+1) << 8 + ROMRead8(addr);
}
u32 Core::ROMRead32(u32 addr)
{
    return ROMRead16(addr+2) << 16 + ROMRead16(addr);
}
u8* Core::ROMReadBuffer(u32 addr, std::size_t len)
{
    u8 *buf = new u8[len];
    for(int n=0;n<len;n++)
        buf[n] = gameData[addr+n];
    return buf;
}
void Core::ROMWrite8(u32 addr, u8 val)
{
    gameData[addr] = val;
}
void Core::ROMWrite16(u32 addr, u16 val)
{
    ROMWrite8(addr,(u8)val);
    ROMWrite8(addr+1,(u8)val);
}
void Core::ROMWrite32(u32 addr, u32 val)
{
    ROMWrite16(addr,(u8)val);
    ROMWrite16(addr+2,(u8)val);
}
void Core::ROMWriteBuffer(u32 addr, u8* buf, std::size_t len)
{
    for(int n=0;n<len;n++)
        ROMWrite8(addr+n,buf[n]);
}

void Core::ContSetInput(u32 controller,unsigned int value)
{
    ctrl_b[controller] = value;
    Logger::Log(LogCategory::Debug,"CORE CTRL ",std::to_string(value));
}
unsigned int Core::ContGetInput(u32 controller)
{
    return ctrl_b[controller];
}
void Core::SetGame(void *data,std::size_t size)
{
    if(gameData != nullptr)
    {
        delete gameData;
        gameData = nullptr;
    }
    gameSize = size;
    gameData = new u8[size];
    gameData = (u8*)data;
}

std::string Core::GetROMHeader(RetroHeader::System sys)
{
    switch(sys)
    {
        case RetroHeader::NES:

        break;
        case RetroHeader::SNES:
            
        break;
        default:
        break;
    }
    return "UNKNOWN";
}
void Core::GetRetroHeader(RetroHeader *hdr)
{
    //Identify the games system
    if(hdr == nullptr)
        return;
    retro_system_info inf;
    Core::GetSystemInfo(&inf);
    
    hdr->name = GetROMHeader(hdr->sys);
}

}
