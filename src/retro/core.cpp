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
    //gameSize = 0;
    //gameData = nullptr;
    changes = false;
    lastpath = "Shhhh, It's a secret to everyone!";
    save_dir = "";
    saveSlot = 0;
    memset(&m_retro, 0, sizeof(m_retro));
    for(int n=0;n<9;n++)
        states.push_back(nullptr);
    
    ginf.data = NULL;
    ginf.path = "";
    ginf.meta = "";
    ginf.size = 0;
    gameLoaded = false;
}

Core::~Core()
{
    for(std::vector<u8*>::iterator i = states.begin(); i != states.end(); i++)
        if(*i != nullptr)
        {
            delete [] *i;
            *i = nullptr;
        }
};
void Core::ReLoadGame()
{
    LoadGame(ginf.path);
}
bool Core::GameLoaded()
{
    return gameLoaded;
}
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
    SetSaveDir(data_dir);
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
bool Core::LoadGame(const std::string &s)
{
    //Todo, Move most of this into Core
    errno = 0;
    std::ifstream mfile;
    mfile.exceptions(0);
    mfile.open(s,std::ios::in|std::ios::binary);
    if(!mfile.is_open() || !mfile.good())
    {
        Logger::Log(LogCategory::Fatal,"Retro",std::string("Failed loading file ") + std::strerror(errno));
        return false;
    }

    ginf.data = NULL;
    ginf.path = s.c_str();
    ginf.meta = "";
    ginf.size = 0;

    mfile.seekg(std::ios::beg,0);

    std::vector<char> bytes(std::istreambuf_iterator<char>(mfile),(std::istreambuf_iterator<char>()));
    mfile.close();

    gameData.assign(std::begin(bytes),std::end(bytes));
    ginf.data = gameData.data();
    ginf.size = gameData.size();
    if(!ginf.data)
    {
        Logger::Log(LogCategory::Fatal,"Retro",std::string("Failed allocating memory!"));
        return false;
    }

    //for(u32 n=0; n < game->size; n++)
    //    gameData.push_back(((u8*)game->data)[n]);
    gameLoaded = m_retro.retro_load_game(&ginf);
    return gameLoaded;
}
bool Core::LoadGameSpecial(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
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
    ginf.data = gameData.data();
    ginf.size = gameData.size();
    ginf.path = "";
    ginf.meta = "";
    return &ginf;
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
    saveSlot = slot;
}
void Core::LoadState()
{
    //Todo, Implement SaveStates
    LoadState(saveSlot);
}
void Core::LoadState(int slot)
{
    //Todo, Implement SaveStates
    if(slot >= states.size())
        return;
    if(states[slot] == nullptr)
        return;
    Logger::Log(LogCategory::Info,"SaveState",std::string("Loading from ") + std::to_string(slot));
    u8 *dat = states[slot];
    m_retro.retro_unserialize(states[slot],m_retro.retro_serialize_size());
}
void Core::LoadState(const std::filesystem::path& path)
{
    //Todo, Implement SaveStates
    if(path.generic_string() == "")
        return;
    Logger::Log(LogCategory::Info,"LoadState",std::string("Loading from ") + (path/"LibretroSaveState.ss").generic_string());
    std::fstream mfile((path/"LibretroSaveState.ss").generic_string(),std::ios::in|std::ios::binary|std::ios::trunc);
    for(int n=0;n<4;n++)
    {
        char *buf;
        std::size_t size = m_retro.retro_get_memory_size(n);
        mfile.read(buf,size);
        u8 *dat = (u8*)buf;
        RDRAMWriteBuffer(0x0,dat,m_retro.retro_get_memory_size(n),n);
    }
    mfile.close();
}
void Core::SaveState()
{
    //Todo, Implement SaveStates
    SaveState(saveSlot);
}
void Core::SaveState(int slot)
{
    //Todo, Implement SaveStates
    if(slot >= states.size())
        return;
    Logger::Log(LogCategory::Info,"SaveState",std::string("Saving to ") + std::to_string(slot));
    if(states[slot] != nullptr)
        delete [] states[slot];
    states[slot] = new u8[m_retro.retro_serialize_size()];
    m_retro.retro_serialize(states[slot],m_retro.retro_serialize_size());
}
void Core::SaveState(const std::filesystem::path& path)
{
    //Todo, Implement SaveStates
    if(path.generic_string() == "")
        return;
    Logger::Log(LogCategory::Info,"SaveState",std::string("Saving to ") + (path/"LibretroSaveState.ss").generic_string());
    std::fstream mfile((path/"LibretroSaveState.ss").generic_string(),std::ios::out|std::ios::binary|std::ios::trunc);
    void *data;
    m_retro.retro_serialize(data,m_retro.retro_serialize_size());
    mfile.write((char*)data,m_retro.retro_serialize_size());

    mfile.close();
}
const std::filesystem::path Core::GetSaveDir()
{
    return save_dir;
}
void Core::SetSaveDir(const std::filesystem::path &path)
{
    save_dir = path;
}
void Core::AdvanceFrame()
{
    Run();
}
bool Core::SaveGameData()
{
    if(save_dir == "")
        return false;
    std::ofstream mfile(save_dir/"SaveGameData.dat",std::ios::out|std::ios::trunc|std::ios::binary);

    if(!mfile.is_open())
        return false;

    mfile.seekp(0,std::ios::beg);

    mfile.write((char*)RDRAMReadBuffer(0,m_retro.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM),RETRO_MEMORY_SAVE_RAM),m_retro.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM));

    mfile.close();
    return true;
}
bool Core::LoadGameData()
{
    if(save_dir == "")
        return false;
    Logger::Log(LogCategory::Debug,"Inner Marker","1");
    std::ifstream mfile;
    mfile.exceptions(0);
    mfile.open(save_dir/"SaveGameData.dat",std::ios::in|std::ios::binary);

    Logger::Log(LogCategory::Debug,"Inner Marker","2");
    if(!mfile.is_open() || !mfile.good())
        return false;
    
    Logger::Log(LogCategory::Debug,"Inner Marker","3");
    std::size_t size;
    mfile.seekg(0,std::ios::end);
    size = mfile.tellg();
    mfile.seekg(0,std::ios::beg);

    Logger::Log(LogCategory::Debug,"Inner Marker","4");
    void *dat = SDL_malloc(size);
    mfile.read((char*)dat,size);

    Logger::Log(LogCategory::Debug,"Inner Marker","5");
    RDRAMWriteBuffer(0x0,(u8*)dat,size,RETRO_MEMORY_SAVE_RAM);
    
    Logger::Log(LogCategory::Debug,"Inner Marker","6");
    if(dat)
    {
        SDL_free(dat);
        dat = NULL;
    }

    Logger::Log(LogCategory::Debug,"Inner Marker","7");
    mfile.close();
    return true;
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
    return gameData.data();
}
std::size_t Core::GetROMSize()
{
    return gameData.size();//gameSize;
}

u8 Core::RDRAMRead8(u32 addr,int type)
{
    return ((u8*)m_retro.retro_get_memory_data(type))[addr];
}
u16 Core::RDRAMRead16(u32 addr,int type)
{
    return RDRAMRead8(addr+1,type) << 8 + RDRAMRead8(addr,type);
}
u32 Core::RDRAMRead32(u32 addr,int type)
{
    return RDRAMRead16(addr+2,type) << 16 + RDRAMRead16(addr,type);
}
u8* Core::RDRAMReadBuffer(u32 addr, std::size_t len,int type)
{
    u8 *buf = new u8[len];
    for(u32 n=0;n<len;n++)
        buf[n] = RDRAMRead8(addr+n,type);
    return buf;
}
void Core::RDRAMWrite8(u32 addr, u8 val,int type)
{
    ((u8*)m_retro.retro_get_memory_data(type))[addr] = val;
}
void Core::RDRAMWrite16(u32 addr, u16 val,int type)
{
    RDRAMWrite8(addr,(u8)val,type);
    RDRAMWrite8(addr+1,(u8)val,type);
}
void Core::RDRAMWrite32(u32 addr, u32 val,int type)
{
    RDRAMWrite16(addr,(u8)val,type);
    RDRAMWrite16(addr+2,(u8)val,type);
}
void Core::RDRAMWriteBuffer(u32 addr, u8* buf, std::size_t len,int type)
{
    for(u32 n=0;n<len;n++)
        RDRAMWrite8(addr+n,buf[n],type);
}
u8 Core::ROMRead8(u32 addr)
{
    if(gameData.size() > addr)
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
    for(u32 n=0;n<len;n++)
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
    for(u32 n=0;n<len;n++)
        ROMWrite8(addr+n,buf[n]);
}

void Core::ContSetInput(u32 controller,unsigned int value)
{
    ctrl_b[controller] = value;
    //Logger::Log(LogCategory::Debug,"ContSetInput",std::string("Controller ") + std::to_string(controller) + " Value "+std::to_string(value));
}
unsigned int Core::ContGetInput(u32 controller)
{
    //Logger::Log(LogCategory::Debug,"ContGetInput",std::string("Controller ") + std::to_string(controller) + " Value "+std::to_string(ctrl_b[controller]));
    return ctrl_b[controller];
}
/*
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
}*/

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
