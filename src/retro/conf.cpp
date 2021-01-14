#include "retro/core.h"
#include "retro/error.h"
//#include "sdl/shared_object.h"
#include <SDL2/SDL_loadso.h>

#include <fmt/format.h>

#include <sstream>

namespace RETRO{

inline std::vector<std::string> Split(const std::string &str)
{
    std::vector<std::string> buf;
    std::stringstream ss(str);
    std::string tmp;
    while(ss >> tmp)
        buf.push_back(tmp);
    return buf;
}

inline bool IsDigit(const std::string &str)
{
    for(int n=0;n<str.size()-1;n++)
        if(!isdigit(str[n]))
            return false;
    return true;
}

inline std::string ToLower(const std::string &str)
{
    std::string low;
    for(int n=0;n<str.size()-1;n++)
        low += tolower(str[n]);
    return low;
}


void Core::ConfigSaveFile()
{
    //Checked("ConfigSaveFile", m_ld->ConfigSaveFile());
    if(mfile.is_open())
        mfile.flush();
}

bool Core::ConfigHasUnsavedChanges()
{
    //return m_ld->ConfigHasUnsavedChanges(nullptr);
    //Can't be right
    //return this->ConfigOpenSection(m_name).HasUnsavedChanges();
    return changes;
}

std::vector<std::string> Core::ConfigListSections()
{
    //std::vector<std::string> v;
    //Checked("ConfigListSections", m_ld->ConfigListSections(&v, [](void* context, const char* section_name) {
    //    static_cast<std::vector<std::string>*>(context)->push_back(section_name);
    //}));

    std::vector<std::string> v;
    return v;
}

Core::ConfigSection::ConfigSection(Core& core, const std::string& name) :
    m_core{&core},
    m_name{name}
{
    if(!m_core->mfile.is_open())
        return;
    
    m_core->mfile.seekg(0);
    
    std::vector<std::string> buf;
    std::string in;
    bool inSect = false;
    while(std::getline(m_core->mfile,in))
    {
        if(in[0] == '[')
        {
            inSect = false;
            int broke=0;
            std::string tmp;
            //NOSECTIONISGOINGTOBELARGERTHANONEHUNDREDANDFIFTYCHARACTERSANDIFITISWHOEVERMADEITWANTSTHISTONOTWORKANDDESERVESTHECRASHANDTHEHEADERWOULDHAVETOBETHISBIG!
            while(broke++ < 150&&in[broke+1] != ']')
            {
                tmp += in[broke+1];
            }
            if(m_name==tmp)
                inSect = true;
        }
        else if(in[0] != '#' && inSect)
        {
            buf = Split(in);

            //The config is setup as
            //VARIABLE = "VALUE"
            //VARIABLE = 22
            //VARIABLE = True
            //VARIABLE = 2.1111
            //Where VARIABLE can be anything and I must Identify the value type

            //Buf should have "VARIABLE" = "Something" which is a total of three items
            if(buf.size() == 3)
                if(buf[2].size() > 1)
                {
                    if(buf[2][0] == '/"')
                    {
                        std::string part;
                        for(int n=1;n<buf[2].size()-2;n++)
                            part += buf[2][n];
                        params.push_back({buf[0],buf[2],RetroType::String});
                    }
                    //If it has a point in it, it is a double
                    else if(buf[2].find('.') != std::string::npos)
                    {
                        params.push_back({buf[0],buf[2],RetroType::Float});
                    }
                    else if(IsDigit(buf[2]))
                    {
                        params.push_back({buf[0],buf[2],RetroType::Int});
                    }
                    else if(ToLower(buf[2]) == "false" || ToLower(buf[2]) == "true")
                    {
                        params.push_back({buf[0],buf[2],RetroType::Bool});
                    }
                    else
                    {
                        params.push_back({buf[0],buf[2],RetroType::Unknown});
                    }
                    
                }
        }

    }
}

std::string Core::ConfigSection::GetName()
{
    return m_name;
}

std::vector<Core::ConfigSection::Param> Core::ConfigSection::ListParams()
{
    /*
    std::vector<Param> v;
    Checked("ConfigListParameters", m_core->m_ld->ConfigListParameters(m_handle, &v,
        [](void* context, const char* param_name, m64p_type param_type) {
            static_cast<std::vector<Param>*>(context)->push_back({param_name, param_type});
        }));
    return v;
    */

    return params;

}

void Core::ConfigSection::Save()
{


    m_core->ConfigSaveFile();
    //Checked("ConfigSaveSection", m_core->m_ld->ConfigSaveSection(m_name.c_str()));
}

bool Core::ConfigSection::HasUnsavedChanges()
{
    return m_core->changes;
}

void Core::ConfigSection::Erase()
{
    //Checked("ConfigDeleteSection", m_core->m_ld->ConfigDeleteSection(m_name.c_str()));

    if(!m_core->mfile.is_open())
        return;
    
    std::vector<std::string> fileStrBuf;
    m_core->mfile.seekg(0);
    
    std::vector<std::string> buf;
    std::string in;
    bool inSect = false;

    while(std::getline(m_core->mfile,in))
    {
        if(in[0] == '[')
        {
            inSect = false;
            int broke=0;
            std::string tmp;
            //NOSECTIONISGOINGTOBELARGERTHANONEHUNDREDANDFIFTYCHARACTERSANDIFITISWHOEVERMADEITWANTSTHISTONOTWORKANDDESERVESTHECRASHANDTHEHEADERWOULDHAVETOBETHISBIG!
            while(broke++ < 150&&in[broke+1] != ']')
            {
                tmp += in[broke+1];
            }
            if(m_name==tmp)
                inSect = true;
        }
        if(!inSect)
        {
            fileStrBuf.push_back(in);
        }
        else
        {
            m_core->changes = true;
            params.clear();
        }
        
    }
    m_core->mfile.close();
    m_core->mfile.open(m_name,std::ios::trunc|std::ios::in|std::ios::out);
    for(std::vector<std::string>::iterator n=fileStrBuf.begin();n!=fileStrBuf.end();n++)
        m_core->mfile << *n;

}

void Core::ConfigSection::RevertChanges()
{
    //Checked("ConfigRevertChanges", m_core->m_ld->ConfigRevertChanges(m_name.c_str()));
}

std::string Core::ConfigSection::GetHelp(const std::string& name)
{
    //return m_core->m_ld->ConfigGetParameterHelp(m_handle, name.c_str());
    return "";
}

void Core::ConfigSection::SetHelp(const std::string& name, const std::string& help)
{
    //Checked("ConfigSetParameterHelp", m_core->m_ld->ConfigSetParameterHelp(m_handle, name.c_str(), help.c_str()));
}

Core::ConfigSection::RetroType Core::ConfigSection::GetType(const std::string& name)
{
    std::vector<Param> prm = ListParams();

    //Checked("ConfigGetParameterType", m_core->m_ld->ConfigGetParameterType(m_handle, name.c_str(), &v));

    for(std::vector<Param>::iterator i = prm.begin(); i != prm.end(); i++)
        if((&(*i))->name == name)
            return (&(*i))->type;

    return RetroType::Unknown;
}

void Core::ConfigSection::SetDefaultInt(const std::string& name, int value, const std::string& help)
{
    //Checked("ConfigSetDefaultInt", m_core->m_ld->ConfigSetDefaultInt(m_handle, name.c_str(), value, help.c_str()));
}

void Core::ConfigSection::SetDefaultFloat(const std::string& name, float value, const std::string& help)
{
    //Checked("ConfigSetDefaultFloat", m_core->m_ld->ConfigSetDefaultFloat(m_handle, name.c_str(), value, help.c_str()));
}

void Core::ConfigSection::SetDefaultBool(const std::string& name, bool value, const std::string& help)
{
    //Checked("ConfigSetDefaultBool", m_core->m_ld->ConfigSetDefaultBool(m_handle, name.c_str(), value, help.c_str()));
}

void Core::ConfigSection::SetDefaultString(const std::string& name, const std::string& value, const std::string& help)
{
    //Checked("ConfigSetDefaultString", m_core->m_ld->ConfigSetDefaultString(m_handle, name.c_str(), value.c_str(), help.c_str()));
}

int Core::ConfigSection::GetInt(const std::string& name)
{
    //return m_core->m_ld->ConfigGetParamInt(m_handle, name.c_str());
    return 0;
}

int Core::ConfigSection::GetIntOr(const std::string& name, int value)
{
    int v;
    //m64p_error ret = m_core->m_ld->ConfigGetParameter(m_handle, name.c_str(), M64TYPE_INT, &v, static_cast<int>(sizeof(int)));
    bool ret = false;
    return ret == true ? v : value;
}

void Core::ConfigSection::SetInt(const std::string& name, int value)
{
    //Checked("ConfigSetParameter:SetInt", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_INT, &value));
}

float Core::ConfigSection::GetFloat(const std::string& name)
{
    //return m_core->m_ld->ConfigGetParamFloat(m_handle, name.c_str());
    return 0;
}

float Core::ConfigSection::GetFloatOr(const std::string& name, float value)
{
    float v;
    //m64p_error ret = m_core->m_ld->ConfigGetParameter(m_handle, name.c_str(), M64TYPE_FLOAT, &v, static_cast<int>(sizeof(float)));
    bool ret = false;
    return ret == true ? v : value;
}

void Core::ConfigSection::SetFloat(const std::string& name, float value)
{
    //Checked("ConfigSetParameter:SetFloat", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_FLOAT, &value));
}

bool Core::ConfigSection::GetBool(const std::string& name)
{
    //return m_core->m_ld->ConfigGetParamBool(m_handle, name.c_str());
    return false;
}

bool Core::ConfigSection::GetBoolOr(const std::string& name, bool value)
{
    int v;
    //m64p_error ret = m_core->m_ld->ConfigGetParameter(m_handle, name.c_str(), M64TYPE_BOOL, &v, static_cast<int>(sizeof(int)));
    bool ret = false;
    return ret == true ? v : value;
}

void Core::ConfigSection::SetBool(const std::string& name, bool value)
{
    int v = value;
    //Checked("ConfigSetParameter:SetBool", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_BOOL, &v));
}

std::string Core::ConfigSection::GetString(const std::string& name)
{
    //return m_core->m_ld->ConfigGetParamString(m_handle, name.c_str());
    return "";
}

std::string Core::ConfigSection::GetStringOr(const std::string& name, const std::string& value)
{
    char v[200];
    //m64p_error ret = m_core->m_ld->ConfigGetParameter(m_handle, name.c_str(), M64TYPE_STRING, v, 200);
    bool ret = false;
    return ret == true ? v : value;
}

void Core::ConfigSection::SetString(const std::string& name, const std::string& value)
{
    //Checked("ConfigSetParameter:SetString", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_STRING, const_cast<char*>(value.c_str())));
}

Core::ConfigSection Core::ConfigOpenSection(const std::string& name)
{
    return {*this,name};
}

std::filesystem::path Core::GetSharedDataFilePath(const std::string& file)
{
    //auto v = m_ld->ConfigGetSharedDataFilepath(file.c_str());
    //return v ? v : "";
    return std::filesystem::path(file);
}

std::filesystem::path Core::GetUserConfigPath()
{
    //return m_ld->ConfigGetUserConfigPath();
    return config_dir;
}

std::filesystem::path Core::GetUserDataPath()
{
    //return m_ld->ConfigGetUserDataPath();
    return data_dir;
}

std::filesystem::path Core::GetUserCachePath()
{
    //return m_ld->ConfigGetUserCachePath();
    return data_dir;
}

}