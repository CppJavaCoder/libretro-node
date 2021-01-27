#include "sprite.h"
#include "common/logger.h"

//#include "frontend/app.h"

namespace RETRO
{

    Sprite::Sprite(SDL_Renderer *r) : rnd(r), txt(NULL), srf(NULL)
    {
        tick_count = frame_current = 0;
        clip = pos = {0,0,0,0};
        free_surf = false;
        isFG = true;
        vflip = hflip = false;
        par = NULL;
        fname = "";
    }
    void Sprite::Copy(const Sprite *spr)
    {
        Logger::Log(LogCategory::Debug,"Mark","1");
        //srf = spr.srf;
        //txt = spr.txt;
        par = (Sprite*)spr;
        cols = spr->cols;
        rows = spr->rows;
        pos.x = spr->pos.x;
        pos.y = spr->pos.y;
        pos.w = spr->pos.w;
        pos.h = spr->pos.h;
        Logger::Log(LogCategory::Debug,"Mark","2");
        frame_current = spr->frame_current;
        tick_count = spr->tick_count;
        free_surf = false;
        hflip = spr->hflip;
        vflip = spr->vflip;
        isFG = spr->isFG;
        clip = spr->clip;
        Logger::Log(LogCategory::Debug,"Mark","3");
    }
    Sprite::~Sprite()
    {
        Logger::Log(LogCategory::Debug,"Sprite","Destructor Called");
        rnd = NULL;
        if(txt != NULL)
        {
            SDL_DestroyTexture(txt);
            txt = NULL;
        }
        if(srf != NULL)
        {
            if(free_surf)
                SDL_FreeSurface(srf);
            srf = NULL;
        }
        par = NULL;
    }
    bool Sprite::LoadFromImage(std::string img,int w,int h,int cols,int rows)
    {
        fname = img;
        free_surf = false;
        if(free_surf && srf)
        {
            SDL_FreeSurface(srf);
            srf = NULL;
        }
        srf = SDL_LoadBMP(img.c_str());
        if(!srf)
            return false;
        SDL_SetColorKey(srf,1,SDL_MapRGB(srf->format,255,0,255));
        free_surf = false;
        bool ret = LoadFromSurface(srf,w,h,cols,rows);
        free_surf = true;
        return ret;
    }
    bool Sprite::LoadFromSurface(SDL_Surface *s,int w,int h,int cols,int rows)
    {
        if(free_surf && srf)
        {
            SDL_FreeSurface(srf);
            srf = NULL;
        }
        free_surf = false;

        SDL_SetColorKey(s,1,SDL_MapRGB(s->format,255,0,255));
        if(txt != NULL)
            SDL_DestroyTexture(txt);
        txt = SDL_CreateTextureFromSurface(rnd,s);
        if(txt == NULL)
            return false;
        pos.w = w;
        pos.h = h;
        this->cols = cols;
        this->rows = rows;

        return true;        
    }
    bool Sprite::LoadFromBuffer(void *pixels,int pitch,int w,int h,int cols,int rows)
    {
        srf = SDL_CreateRGBSurfaceFrom(pixels,cols*w,rows*h,24,pitch,0,0,0,0);
        if(srf)
        {
            SDL_SetColorKey(srf,1,SDL_MapRGB(srf->format,255,0,255));
            free_surf = false;
            bool res = LoadFromSurface(srf,w,h,cols,rows);
            free_surf = false;
            return res;
        }
        return false;
    }
    void Sprite::SetFrame(int frm)
    {
        frame_current = frm;
    }
    void Sprite::Animate(int from, int to, int time)
    {
        Sint8 val = 0;
        if(from<to)
        {
            val = 1;
            if(frame_current < from)
                frame_current = from;
            else if(frame_current > to)
                frame_current = to;
        }
        else
        {
            val = -1;
            if(frame_current > from)
                frame_current = from;
            else if(frame_current < to)
                frame_current = to;
        }

        if(++tick_count > time)
        {
            tick_count = 0;
            frame_current += val;
            if(from<to)
            {
                if(frame_current < from)
                    frame_current = from;
                else if(frame_current > to)
                    frame_current = to;
            }
            else
            {
                if(frame_current > from)
                    frame_current = from;
                else if(frame_current < to)
                    frame_current = to;
            }
        }
    }
    void Sprite::SetPos(int x,int y)
    {
        pos.x = x;
        pos.y = y;
    }
    void Sprite::SetClip(int x,int y,int w,int h)
    {
        clip.x=x;
        clip.y=y;
        clip.h=h;
        clip.w=w;
    }
    void Sprite::ReplaceColor(int r1,int g1,int b1,int r2,int g2,int b2)
    {
        for(int n=0;n<srf->w*srf->h;n++)
        {
            Uint8 r,g,b;
            SDL_GetRGB(((Uint32*)srf->pixels)[n],srf->format,&r,&g,&b);
            ((Uint32*)srf->pixels)[n] = SDL_MapRGB(srf->format,r2,g2,b2);
        }
    }
    int Sprite::GetX()
    {
        return pos.x;
    }
    int Sprite::GetY()
    {
        return pos.y;
    }
    int Sprite::GetW()
    {
        return pos.w;
    }
    int Sprite::GetH()
    {
        return pos.h;
    }
    bool Sprite::Draw()
    {
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        int angle = 0;
        if(vflip)
            flip = SDL_FLIP_VERTICAL;
        else if(hflip)
            flip = SDL_FLIP_HORIZONTAL;
        else if(vflip && hflip)
        {
            flip = SDL_FLIP_HORIZONTAL;
            angle = 180;
        }
        SDL_Rect a = {((frame_current%cols)*pos.w)+clip.x,((frame_current/cols)*pos.h)+clip.y,pos.w+clip.w,pos.h+clip.h};
        SDL_Rect b = {pos.x+clip.x,pos.y+clip.y,pos.w+clip.w,pos.h+clip.h};
        if(par != NULL && par->rnd != NULL && par->txt != NULL)
        {
            SDL_RenderCopyEx(par->rnd,par->txt,&a,&b,angle,NULL,flip);
        }
        else if(rnd != NULL && txt != NULL)
        {
            SDL_RenderCopyEx(rnd,txt,&a,&b,angle,NULL,flip);
            SDL_SaveBMP(srf,"mods/character.bmp");
        }
        else
        {
            if(!rnd)
                Logger::Log(LogCategory::Error,"Sprite","No Renderer!");
            if(!txt)
            {
                Logger::Log(LogCategory::Error,"Sprite","No Texture!");
            }
            
            Logger::Log(LogCategory::Error,"Sprite","ERROR!");
                return false;
        }
        return true;
    }
    void Sprite::SetVFlip(bool flip)
    {
        vflip = flip;
    }
    void Sprite::SetHFlip(bool flip)
    {
        hflip = flip;
    }
    void Sprite::SetFG(bool fg)
    {
        isFG = fg;
    }
    bool Sprite::GetFG()
    {
        return isFG;
    }
    void Sprite::RunCommand(Command *c)
    {
        if(c->name == "LoadFromImage")
            LoadFromImage(c->param[0],c->iparam[0],c->iparam[1],c->iparam[2],c->iparam[4]);
        else if(c->name == "SetFrame")
            SetFrame(c->iparam[0]);
        else if(c->name == "Animate")
            Animate(c->iparam[0],c->iparam[1],c->iparam[2]);
        else if(c->name == "SetPos")
            SetPos(c->iparam[0],c->iparam[1]);
        else if(c->name == "ReplaceColor")
            ReplaceColor(c->iparam[0],c->iparam[1],c->iparam[2],c->iparam[3],c->iparam[4],c->iparam[5]);
        else if(c->name == "SetHFlip")
            SetHFlip(c->iparam[0]);
        else if(c->name == "SetVFlip")
            SetVFlip(c->iparam[0]);
        else if(c->name == "SetFG")
            SetFG(c->iparam[0]);
        else if(c->name == "SetClip")
            SetClip(c->iparam[0],c->iparam[1],c->iparam[2],c->iparam[3]);
        else
            Logger::Log(LogCategory::Info,"Unidentified Command",c->name);
    }
    void Sprite::Reload(SDL_Renderer *r)
    {
        rnd = r;
        if(fname != "")
        {
            LoadFromImage(fname,pos.w,pos.h,cols,rows);
        }
        else
        {
            bool olfs = free_surf;
            free_surf = false;
            LoadFromSurface(srf,pos.w,pos.h,cols,rows);
            free_surf = olfs;
        }
    }
}