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

inline std::string Trim(const std::string &str)
{
    if(str.size() <= 1)
        return str;
    std::string tmp;
    for(int n=1;n<str.size()-2;n++)
        tmp += str[n];
    return tmp;
}


void Core::ConfigSaveFile()
{
    //Checked("ConfigSaveFile", m_ld->ConfigSaveFile());
    if(changes)
    {
        if(mfile.is_open())
            mfile.flush();
    }
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

void Core::ConfigSection::AddToParams(std::string name,std::string val)
{    
    if(val[0] == '/"')
    {
        std::string part = "";
        for(int n=1;n<val.size()-2;n++)
            part += val[n];
        params.push_back({name,part,RetroType::String});
    }
    //If it has a point in it, it is a double
    else if(val.find('.') != std::string::npos)
    {
        params.push_back({name,val,RetroType::Float});
    }
    else if(IsDigit(val))
    {
        params.push_back({name,val,RetroType::Int});
    }
    else if(ToLower(val) == "false" || ToLower(val) == "true")
    {
        params.push_back({name,val,RetroType::Bool});
    }
    else
    {
        params.push_back({name,val,RetroType::Unknown});
    }
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
            while(broke < 150&&in[broke+1] != ']')
            {
                tmp += in[broke+1];
                broke++;
            }
            if(m_name==tmp)
            {
                inSect = true;
            }
        }
        else if(in[0] != '#' && in[0] != ';' && inSect)
        {
            buf = Split(in);

            //The config is setup as
            //VARIABLE = "VALUE"
            //VARIABLE = 22
            //VARIABLE = True
            //VARIABLE = 2.1111
            //Where VARIABLE can be anything and I must Identify the value type

            //Buf should have "VARIABLE" = "Something" which is a total of three items
            if(buf.size() >= 3){

                for(int n=0; n < buf.size()-3;n++)
                    buf[0] = buf[0]+" "+buf[1+n];

                if(buf[buf.size()-1].size() > 0)
                {
                    AddToParams(buf[0],buf[buf.size()-1]);
                    
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
    if(m_core->changes)
    {
        Erase(false);
        m_core->ConfigSaveFile();
        m_core->changes = false;
        //Checked("ConfigSaveSection", m_core->m_ld->ConfigSaveSection(m_name.c_str()));
    }
}

bool Core::ConfigSection::HasUnsavedChanges()
{
    return m_core->changes;
}

void Core::ConfigSection::Erase(bool clprm)
{
    //Checked("ConfigDeleteSection", m_core->m_ld->ConfigDeleteSection(m_name.c_str()));

    if(!m_core->mfile.is_open())
        return;
    
    std::vector<std::string> fileStrBuf;
    m_core->mfile.clear();
    m_core->mfile.seekg(0, std::ios::beg);
    
    std::vector<std::string> buf;
    std::string in;
    bool inSect = false;

    if(clprm)
        params.clear();

    while(std::getline(m_core->mfile,in))
    {
        if(in[0] == '[')
        {
            inSect = false;
            int broke=0;
            std::string tmp;
            //NOSECTIONISGOINGTOBELARGERTHANONEHUNDREDANDFIFTYCHARACTERSANDIFITISWHOEVERMADEITWANTSTHISTONOTWORKANDDESERVESTHECRASHANDTHEHEADERWOULDHAVETOBETHISBIG!
            while(broke < 150&&in[broke+1] != ']')
            {
                tmp += in[broke+1];
                broke++;
            }
            if(m_name==tmp)
            {
                inSect = true;
            }
        }
        if(!inSect)
        {
            fileStrBuf.push_back(in);
        }
        
    }
    m_core->mfile.close();
    m_core->mfile.open(m_core->lastpath,std::ios::trunc|std::ios::in|std::ios::out);

    m_core->mfile.clear();
    m_core->mfile.seekp(0, std::ios::beg);

    for(std::vector<std::string>::iterator n=fileStrBuf.begin();n!=fileStrBuf.end();n++)
    {
        m_core->mfile << *n << "\n";
    }
    
    if(clprm) // I'll have to double check sometime when I'm not so tired and see if it's possible for !m_core->changes == false
        return;

    m_core->mfile << "[" << m_name << "]\n";
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
    {
        m_core->mfile << (*i).name << " = " << (*i).value << "\n";
    }
    

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
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
            return std::atoi((*i).value.c_str());
    //return m_core->m_ld->ConfigGetParamInt(m_handle, name.c_str());
    return 0;
}

int Core::ConfigSection::GetIntOr(const std::string& name, int value)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
            return std::atoi(i->value.c_str());
    //m64p_error ret = m_core->m_ld->ConfigGetParameter(m_handle, name.c_str(), M64TYPE_INT, &v, static_cast<int>(sizeof(int)));
    return value;
}

void Core::ConfigSection::SetInt(const std::string& name, int value)
{
    bool makeOne = true;
    m_core->changes = true;
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
        {
            (*i).value = std::to_string(value);
            makeOne = false;
        }
    if(makeOne)
    {
        AddToParams(name,std::to_string(value));
    }
    //Checked("ConfigSetParameter:SetInt", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_INT, &value));
}

float Core::ConfigSection::GetFloat(const std::string& name)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
            return std::atof(i->value.c_str());
    //return m_core->m_ld->ConfigGetParamFloat(m_handle, name.c_str());
    return 0;
}

float Core::ConfigSection::GetFloatOr(const std::string& name, float value)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
            return std::atof(i->value.c_str());
    //m64p_error ret = m_core->m_ld->ConfigGetParameter(m_handle, name.c_str(), M64TYPE_FLOAT, &v, static_cast<int>(sizeof(float)));
    return value;
}

void Core::ConfigSection::SetFloat(const std::string& name, float value)
{
    bool makeOne = true;
    m_core->changes = true;
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
        {
            (*i).value = std::to_string(value);
            makeOne = false;
        }
    if(makeOne)
    {
        AddToParams(name,std::to_string(value));
    }
    //Checked("ConfigSetParameter:SetFloat", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_FLOAT, &value));
}

bool Core::ConfigSection::GetBool(const std::string& name)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
            return ToLower((*i).value) == "true";
    //return m_core->m_ld->ConfigGetParamBool(m_handle, name.c_str());
    return false;
}

bool Core::ConfigSection::GetBoolOr(const std::string& name, bool value)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
            return ToLower((*i).value) == "true";
    return value;
}

void Core::ConfigSection::SetBool(const std::string& name, bool value)
{
    bool makeOne = true;
    m_core->changes = true;
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
        {
            (*i).value = (value ? "True" : "False");
            makeOne = false;
        }
    if(makeOne)
    {
        AddToParams(name,(value ? "True" : "False"));
    }
    //Checked("ConfigSetParameter:SetBool", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_BOOL, &v));
}

std::string Core::ConfigSection::GetString(const std::string& name)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
    {
        if((*i).name == name)
            return Trim((*i).value);
    }
    //return m_core->m_ld->ConfigGetParamString(m_handle, name.c_str());
    return "";
}

std::string Core::ConfigSection::GetStringOr(const std::string& name, const std::string& value)
{
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
    {
        if((*i).name == name)
        {
            return Trim((*i).value);
        }
    }
    return Trim(value);
}

void Core::ConfigSection::SetString(const std::string& name, const std::string& value)
{
    bool makeOne = true;
    m_core->changes = true;
    for(std::vector<Param>::iterator i = params.begin(); i != params.end(); i++)
        if((*i).name == name)
        {
            (*i).value = '\"'+value+'\"';
            makeOne = false;
        }
    if(makeOne)
    {
        AddToParams(name,'\"'+value+'\"');
    }
    //Checked("ConfigSetParameter:SetString", m_core->m_ld->ConfigSetParameter(m_handle, name.c_str(), M64TYPE_STRING, const_cast<char*>(value.c_str())));
}

Core::ConfigSection Core::ConfigOpenSection(const std::string& name)
{
    return OpenSection("libretro",name);
}

Core::ConfigSection Core::OpenSection(const std::string &fname, const std::string& name)
{
    if(changes)
        ConfigSaveFile();
    changes = false;
    if(lastpath == (config_dir/(fname + ".cfg")).generic_string())
        return {*this,name};
    ConfigSaveFile();
    if(mfile.is_open())
        mfile.close();
    
    lastpath = (config_dir/(fname + ".cfg")).generic_string();
    mfile.open((config_dir/(fname + ".cfg")).generic_string(),std::ios::in|std::ios::out|std::ios::app);
    if(!mfile.is_open())
    {
        changes = true;
        mfile.open((config_dir/(fname + ".cfg")).generic_string(),std::ios::in|std::ios::out|std::ios::app);
    }
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