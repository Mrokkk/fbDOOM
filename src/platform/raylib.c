#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#define __DOOMKEYS__
#include "d_event.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_misc.h"
#include "platform/pixel_format.h"
#include "platform/platform.h"
#include "platform/rl_doomkeys.h"

static boolean showfps;
static Image screen_image;
static Texture2D screen_texture;
static int resx, resy;

void I_Platform_InitGraphics(screen_t* s)
{
    int i, fps = 60;

    SetConfigFlags(FLAG_WINDOW_MAXIMIZED | FLAG_WINDOW_RESIZABLE);
    InitWindow(1024, 768, "fbdoom");
    SetWindowState(FLAG_WINDOW_MAXIMIZED | FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_NULL);

    showfps = !!M_CheckParm("-showfps");

    i = M_CheckParmWithArgs("-fps", 1);
    if (i > 0)
    {
        i = atoi(myargv[i + 1]);
        if (i < 0)
        {
            I_Printf("Incorrect FPS setting: %d\n", i);
        }
        else
        {
            fps = i;
        }
    }

    I_Printf("Target FPS: %d\n", fps);

    SetTargetFPS(fps);

    screen_image = GenImageColor(SCREENWIDTH, SCREENHEIGHT, BLACK);
    screen_texture = LoadTextureFromImage(screen_image);

    s->resx   = SCREENWIDTH;
    s->resy   = SCREENHEIGHT;
    s->pixels = screen_image.data;

    SET_PIXEL_FORMAT_R8B8G8A8(s);
}

void I_Platform_ShutdownGraphics(screen_t* s)
{
    UNUSED(s);
    CloseWindow();
}

void I_Platform_SetWindowTitle(char* title)
{
    SetWindowTitle(title);
}

void I_Platform_RenderFrame(void)
{
    I_Platform_ReadEvents();

    UpdateTexture(screen_texture, screen_image.data);

    BeginDrawing();
    {
        ClearBackground(BLACK);

        resx = GetScreenWidth();
        resy = GetScreenHeight();
        float scalex = (float)(resx) / SCREENWIDTH;
        float scaley = (float)(resy) / SCREENHEIGHT;
        float scale = MIN(scalex, scaley);
        float posx = (resx - scale * SCREENWIDTH) / 2;
        float posy = (resy - scale * SCREENHEIGHT) / 2;

        DrawTextureEx(screen_texture, (Vector2){posx, posy}, 0.0f, scale, WHITE);

        if (showfps)
        {
            DrawFPS(resx - 100, 20);
        }
    }
    EndDrawing();
}

static int initialized = 0;
static char key_state[348];
static char mouse_button_state[3];

void I_Platform_InitInput(void)
{
    HideCursor();
    initialized = 1;
}

void I_Platform_ShutdownInput(void)
{
}

static int I_Platform_ConvertToDoomKey(int key)
{
    switch (key)
    {
        case KEY_LEFT_CONTROL:
        case KEY_RIGHT_CONTROL: return D_KEY_FIRE;
        case KEY_LEFT_SHIFT:
        case KEY_RIGHT_SHIFT:   return D_KEY_RSHIFT;
        case KEY_LEFT_ALT:      return D_KEY_RALT;
        case KEY_RIGHT_ALT:     return D_KEY_LALT;
        case KEY_ESCAPE:        return D_KEY_ESCAPE;
        case KEY_SPACE:         return D_KEY_USE;
        case KEY_ENTER:         return D_KEY_ENTER;
        case KEY_BACKSPACE:     return D_KEY_BACKSPACE;
        case KEY_UP:            return D_KEY_UPARROW;
        case KEY_DOWN:          return D_KEY_DOWNARROW;
        case KEY_LEFT:          return D_KEY_LEFTARROW;
        case KEY_RIGHT:         return D_KEY_RIGHTARROW;
        case KEY_TAB:           return D_KEY_TAB;

#define FN_KEY(k) case KEY_F##k: return D_KEY_F##k
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
            if (isprint(key))
            {
                return tolower(key);
            }
            return 0;
    }
}

void I_Platform_ReadEvents(void)
{
    size_t i;
    event_t event;
    boolean pressed, changed = false;
    Vector2 mouse_pos;

    if (!initialized)
    {
        return;
    }

    if (WindowShouldClose())
    {
        event.type = ev_quit;
        event.data1 = 0;
        event.data2 = 0;
        D_PostEvent(&event);
        return;
    }

    for (i = 0; i < arrlen(key_state); ++i)
    {
        pressed = IsKeyDown(i);

        if (!key_state[i] && pressed)
        {
            key_state[i] = 1;
            event.type = ev_keydown;
            event.data1 = I_Platform_ConvertToDoomKey(i);
            event.data2 = 0;
            if (event.data1)
            {
                D_PostEvent(&event);
            }
        }
        else if (key_state[i] && !pressed)
        {
            key_state[i] = 0;
            event.type = ev_keyup;
            event.data1 = I_Platform_ConvertToDoomKey(i);
            event.data2 = 0;
            if (event.data1)
            {
                D_PostEvent(&event);
            }
        }
    }

    mouse_pos = GetMouseDelta();

    pressed = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    if (!mouse_button_state[0] && pressed)
    {
        mouse_button_state[0] = 1;
        changed = true;
    }
    else if (mouse_button_state[0] && !pressed)
    {
        mouse_button_state[0] = 0;
        changed = true;
    }

    if (mouse_pos.x || changed)
    {
        event.type = ev_mouse;
        event.data1 = mouse_button_state[0];
        event.data2 = mouse_pos.x * 5;
        event.data3 = 0;
        D_PostEvent(&event);
    }

    SetMousePosition(resx / 2, resy / 2);
}

struct music_module* I_Platform_GetMusicModule(void)
{
    return NULL;
}

struct sound_module* I_Platform_GetSoundModule(void)
{
    return NULL;
}
