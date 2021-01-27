#ifndef SPRITE_H
#define SPRITE_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <iostream>

namespace RETRO
{

    class Sprite
    {
        public:
            Sprite(SDL_Renderer *r = NULL);
            void Copy(const Sprite*);
            ~Sprite();
            bool LoadFromImage(std::string img,int w,int h,int cols,int rows);
            bool LoadFromSurface(SDL_Surface *s,int w,int h,int cols,int rows);
            bool LoadFromBuffer(void *pixels,int pitch,int w,int h,int cols,int rows);
            bool FromSprite(Sprite *s);
            void SetFrame(int frm);
            void Animate(int from, int to, int time);
            void SetPos(int x,int y);
            void SetClip(int x,int y,int w,int h);
            void ReplaceColor(int r1,int g1,int b1,int r2,int g2,int b2);
            int GetX();
            int GetY();
            int GetW();
            int GetH();
            bool Draw();
            void SetHFlip(bool flip);
            void SetVFlip(bool flip);
            void SetFG(bool fg);
            bool GetFG();

            void Reload(SDL_Renderer *r);

            struct Command
            {
                std::string name;
                std::vector<std::string> param;
                std::vector<int> iparam;
                int id;
            };

            void RunCommand(Command *c);

        private:
            SDL_Renderer *rnd;
            SDL_Surface *srf;
            SDL_Texture *txt;
            int frame_current;
            int tick_count;
            SDL_Rect pos,clip;
            int cols,rows;
            bool free_surf;
            bool hflip,vflip;
            bool isFG;
            Sprite *par;
            int xoff,yoff;
            std::string fname;
    };

}
#endif