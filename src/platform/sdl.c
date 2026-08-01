#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>

#include "d_event.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_misc.h"
#include "platform/platform.h"

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static uint32_t* image;

static void CalculateLen(unsigned long mask, uint8_t* out_len)
{
    uint8_t len;
    uint8_t offset;
    for (offset = 0; (mask & 1) == 0; mask >>= 1, ++offset);
    for (len = 0; mask; mask >>= 1, ++len);
    *out_len = len;
}

static void SetPixelFormat(screen_t* s, SDL_PixelFormat* f)
{
    uint8_t len;

    CalculateLen(f->Rmask, &len);
    s->red.offset   = f->Rshift;
    s->red.len      = len;

    CalculateLen(f->Gmask, &len);
    s->green.offset = f->Gshift;
    s->green.len    = len;

    CalculateLen(f->Bmask, &len);
    s->blue.offset  = f->Bshift;
    s->blue.len     = len;

    s->bits_per_pixel  = f->BitsPerPixel;
    s->bytes_per_pixel = f->BytesPerPixel;
}

void I_Platform_InitGraphics(screen_t* s)
{
    Uint32           flags = 0;
    Uint32           pixel_format_id;
    SDL_RendererInfo info;
    SDL_PixelFormat* pixel_format;

    I_ErrorWhen(SDL_Init(SDL_INIT_VIDEO) < 0,
        "Failed to initialize the SDL2 library: %s\n", SDL_GetError());

    if (M_CheckParm("-fullscreen"))
    {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    else if (M_CheckParm("-maximized"))
    {
        flags |= SDL_WINDOW_MAXIMIZED;
    }

    SDL_ShowCursor(SDL_DISABLE);

    window = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREENWIDTH * 3,
        SCREENHEIGHT * 3,
        flags);

    I_ErrorWhen(!window, "Failed to create window: %s\n", SDL_GetError());

    pixel_format_id = SDL_GetWindowPixelFormat(window);

    I_ErrorWhen(!(pixel_format = SDL_AllocFormat(pixel_format_id)),
        "Failed to allocate pixel format: %s\n", SDL_GetError());

    I_ErrorWhen(!(renderer = SDL_CreateRenderer(window, -1, 0)),
        "Failed to create renderer: %s\n", SDL_GetError());

    I_ErrorWhen(SDL_GetRendererInfo(renderer, &info) < 0,
        "Failed to read renderer info: %s\n", SDL_GetError());

    I_Printf("Renderer: %s\n", info.name);

    texture = SDL_CreateTexture(
        renderer,
        pixel_format_id,
        SDL_TEXTUREACCESS_TARGET,
        SCREENWIDTH,
        SCREENHEIGHT);

    I_ErrorWhen(!texture, "Failed to create texture\n");

    I_ErrorWhen(!(image = calloc(SCREENWIDTH * SCREENHEIGHT, sizeof(*image))),
        "Failed to allocate memory for framebuffer\n");

    s->resx   = SCREENWIDTH;
    s->resy   = SCREENHEIGHT;
    s->pixels = image;

    SetPixelFormat(s, pixel_format);

    SDL_FreeFormat(pixel_format);
}

void I_Platform_ShutdownGraphics(screen_t* s)
{
    if (window) SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    UNUSED(s);
}

void I_Platform_SetWindowTitle(char* title)
{
    if (window)
    {
        SDL_SetWindowTitle(window, title);
    }
}

void I_Platform_RenderFrame(void)
{
    SDL_Rect dest;

    int resx, resy;

    SDL_GetWindowSize(window, &resx, &resy);

    float scalex = (float)(resx) / SCREENWIDTH;
    float scaley = (float)(resy) / SCREENHEIGHT;
    float scale = MIN(scalex, scaley);
    float posx = (resx - scale * SCREENWIDTH) / 2;
    float posy = (resy - scale * SCREENHEIGHT) / 2;

    dest.x = posx;
    dest.y = posy;
    dest.w = scale * SCREENWIDTH;
    dest.h = scale * SCREENHEIGHT;

    SDL_UpdateTexture(texture, NULL, image, SCREENWIDTH * sizeof(*image));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_RenderPresent(renderer);
}

void I_Platform_InitInput(void)
{
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void I_Platform_ShutdownInput(void)
{
}

static uint8_t I_Platform_ConvertToDoomKey(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_LCTRL:
        case SDLK_RCTRL:     return KEY_FIRE;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:    return KEY_RSHIFT;
        case SDLK_LALT:      return KEY_LALT;
        case SDLK_RALT:      return KEY_RALT;
        case SDLK_ESCAPE:    return KEY_ESCAPE;
        case SDLK_SPACE:     return KEY_USE;
        case SDLK_RETURN:    return KEY_ENTER;
        case SDLK_BACKSPACE: return KEY_BACKSPACE;
        case SDLK_UP:        return KEY_UPARROW;
        case SDLK_DOWN:      return KEY_DOWNARROW;
        case SDLK_LEFT:      return KEY_LEFTARROW;
        case SDLK_RIGHT:     return KEY_RIGHTARROW;
        case SDLK_TAB:       return KEY_TAB;

#define FN_KEY(k) case SDLK_F##k: return KEY_F##k
        FN_KEY(1);
        FN_KEY(2);
        FN_KEY(3);
        FN_KEY(4);
        FN_KEY(5);
        FN_KEY(6);
        FN_KEY(7);
        FN_KEY(8);
        FN_KEY(9);
        FN_KEY(10);
        FN_KEY(11);
        FN_KEY(12);

        default:
            if (key < 127 && isprint(key))
            {
                return tolower(key);
            }
            return 0;
    }
}

static int buttons;

void I_Platform_ReadEvents(void)
{
    SDL_Event e;
    event_t event;

    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                event.type = ev_quit;
                event.data1 = 0;
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                event.type = e.type == SDL_KEYUP ? ev_keyup : ev_keydown;
                event.data1 = I_Platform_ConvertToDoomKey(e.key.keysym.sym);
                event.data2 = 0;
                if (event.data1)
                {
                    D_PostEvent(&event);
                }
                break;

            case SDL_MOUSEMOTION:
                event.type = ev_mouse;
                event.data1 = buttons;
                event.data2 = e.motion.xrel * 2;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case SDL_MOUSEBUTTONDOWN:
                event.type = ev_mouse;
                event.data1 = buttons |= SDL_BUTTON(e.button.button);
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case SDL_MOUSEBUTTONUP:
                event.type = ev_mouse;
                event.data1 = buttons &= ~SDL_BUTTON(e.button.button);
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;
        }
    }
}

struct music_module* I_Platform_GetMusicModule(void)
{
    return NULL;
}

struct sound_module* I_Platform_GetSoundModule(void)
{
    return NULL;
}
