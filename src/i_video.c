/*
 * Copyright (C) 1993-1996 by id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  DOOM graphics
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_misc.h"
#include "tables.h"
#include "z_zone.h"

#include "platform/platform.h"

static screen_t screen;
static int scaling = 1;
static size_t left_x_offset, right_x_offset, y_offset_bytes;
static void (*I_DrawScreen)(void);

/* The screen buffer; this is modified to draw things to the screen */
byte *I_VideoBuffer = NULL;

/* If true, game is running as a screensaver */
boolean screensaver_mode = false;

/* Flag indicating whether the screen is currently visible:
 * when the screen isnt visible, don't render the screen */
boolean screenvisible = false;

/* Gamma correction level to use */
int usegamma = 0;

typedef union color_table
{
    uint8_t  width1[256];
    uint16_t width2[256];
    uint32_t width3[256];
    uint32_t width4[256];
} color_table_t;

static color_table_t color_table;

#define DEFINE_DRAWSCREEN_SOFTWARESCALING(name, type, table, ...) \
    static void I_CMapTo_##table(type *out, uint8_t *in, int in_pixels) \
    { \
        int i, j; \
        uint32_t color; \
        for (i = 0; i < in_pixels; i++) \
        { \
            color = color_table.table[*in]; \
            for (j = 0; j < scaling; j++) \
            { \
                __VA_ARGS__; \
            } \
            in++; \
        } \
    } \
    static void name(void) \
    { \
        int i; \
        int y; \
        uint8_t *line_in, *line_out; \
        line_in  = (uint8_t*)I_VideoBuffer; \
        line_out = (uint8_t*)screen.pixels + y_offset_bytes; \
        y = SCREENHEIGHT; \
        while (y--) \
        { \
            for (i = 0; i < scaling; i++) \
            { \
                line_out += left_x_offset; \
                I_CMapTo_##table((type*)line_out, line_in, SCREENWIDTH); \
                line_out += SCREENWIDTH * scaling * screen.bytes_per_pixel + right_x_offset; \
            } \
            line_in += SCREENWIDTH; \
        } \
    }

#define DEFINE_DRAWSCREEN_NOSCALING(name, type, table, ...) \
    static void I_CMapTo_##table##_NoScaling(type *out, uint8_t *in, int in_pixels) \
    { \
        int i; \
        uint32_t color; \
        for (i = 0; i < in_pixels; i++) \
        { \
            color = color_table.table[*in]; \
            __VA_ARGS__; \
            in++; \
        } \
    } \
    static void name(void) \
    { \
        int y; \
        uint8_t *line_in, *line_out; \
        line_in  = (uint8_t*)I_VideoBuffer; \
        line_out = (uint8_t*)screen.pixels + y_offset_bytes; \
        y = SCREENHEIGHT; \
        while (y--) \
        { \
            line_out += left_x_offset; \
            I_CMapTo_##table##_NoScaling((type*)line_out, line_in, SCREENWIDTH); \
            line_out += SCREENWIDTH * screen.bytes_per_pixel + right_x_offset; \
            line_in += SCREENWIDTH; \
        } \
    }

DEFINE_DRAWSCREEN_SOFTWARESCALING(I_DrawScreen_4Bytes_SoftwareScaling, uint32_t, width4, *out++ = color);
DEFINE_DRAWSCREEN_SOFTWARESCALING(I_DrawScreen_3Bytes_SoftwareScaling, uint8_t,  width3,  memcpy(out, &color, 3); out += 3);
DEFINE_DRAWSCREEN_SOFTWARESCALING(I_DrawScreen_2Bytes_SoftwareScaling, uint16_t, width2, *out++ = color);
DEFINE_DRAWSCREEN_SOFTWARESCALING(I_DrawScreen_1Bytes_SoftwareScaling, uint8_t,  width1, *out++ = color);

DEFINE_DRAWSCREEN_NOSCALING(I_DrawScreen_4Bytes_NoScaling, uint32_t, width4, *out++ = color);
DEFINE_DRAWSCREEN_NOSCALING(I_DrawScreen_3Bytes_NoScaling, uint8_t,  width3,  memcpy(out, &color, 3); out += 3);
DEFINE_DRAWSCREEN_NOSCALING(I_DrawScreen_2Bytes_NoScaling, uint16_t, width2, *out++ = color);
DEFINE_DRAWSCREEN_NOSCALING(I_DrawScreen_1Bytes_NoScaling, uint8_t,  width1, *out++ = color);

static void I_ScreenRecalculate(void)
{
    int i, max_scaling;

    max_scaling = MIN(screen.resx / SCREENWIDTH, screen.resy / SCREENHEIGHT);

    i = M_CheckParmWithArgs("-scaling", 1);
    if (i > 0)
    {
        i = atoi(myargv[i + 1]);
        if (i > max_scaling)
        {
            scaling = max_scaling;
            I_Printf("Scaling too big: %d, using max value %d\n", i, max_scaling);
        }
        else
        {
            scaling = i;
            I_Printf("Scaling factor: %d\n", scaling);
        }
    }
    else
    {
        scaling = max_scaling;
        I_Printf("Auto-scaling factor: %d\n", scaling);
    }

    if (scaling == 1)
    {
        switch (screen.bytes_per_pixel)
        {
            case 4:
                I_DrawScreen = &I_DrawScreen_4Bytes_NoScaling;
                break;
            case 3:
                I_DrawScreen = &I_DrawScreen_3Bytes_NoScaling;
                break;
            case 2:
                I_DrawScreen = &I_DrawScreen_2Bytes_NoScaling;
                break;
            case 1:
                I_DrawScreen = &I_DrawScreen_1Bytes_NoScaling;
                break;
            default:
                I_Error("Unsupported screen depth: %u\n", screen.bits_per_pixel);
        }
    }
    else
    {
        switch (screen.bytes_per_pixel)
        {
            case 4:
                I_DrawScreen = &I_DrawScreen_4Bytes_SoftwareScaling;
                break;
            case 3:
                I_DrawScreen = &I_DrawScreen_3Bytes_SoftwareScaling;
                break;
            case 2:
                I_DrawScreen = &I_DrawScreen_2Bytes_SoftwareScaling;
                break;
            case 1:
                I_DrawScreen = &I_DrawScreen_1Bytes_SoftwareScaling;
                break;
            default:
                I_Error("Unsupported screen depth: %u\n", screen.bits_per_pixel);
        }
    }

    left_x_offset   = ((screen.resx - (SCREENWIDTH  * scaling))  * screen.bytes_per_pixel) / 2;
    right_x_offset  = ((screen.resx - (SCREENWIDTH  * scaling))  * screen.bytes_per_pixel) - left_x_offset;
    y_offset_bytes  = (((screen.resy - (SCREENHEIGHT * scaling)) * screen.bytes_per_pixel) / 2) * screen.resx;
}

void I_InitGraphics(void)
{
    int i, max_scaling;

    I_AtExit(I_ShutdownGraphics, true);

    I_Platform_InitGraphics(&screen);

    if (!screen.red.len && !screen.green.len && !screen.blue.len)
    {
        I_Error("Unusable platform screen: invalid colors bit lenghts\n");
    }

    I_Printf("DOOM screen size: %d x %d\n", SCREENWIDTH, SCREENHEIGHT);
    I_Printf("Platform screen: %d x %d x %d\n", screen.resx, screen.resy, screen.bits_per_pixel);

    max_scaling = MIN(screen.resx / SCREENWIDTH, screen.resy / SCREENHEIGHT);

    i = M_CheckParmWithArgs("-scaling", 1);
    if (i > 0)
    {
        i = atoi(myargv[i + 1]);
        if (i > max_scaling)
        {
            scaling = max_scaling;
            I_Printf("Scaling too big: %d, using max value %d\n", i, max_scaling);
        }
        else
        {
            scaling = i;
            I_Printf("Scaling factor: %d\n", scaling);
        }
    }
    else
    {
        scaling = max_scaling;
        I_Printf("Auto-scaling factor: %d\n", scaling);
    }

    if (scaling == 1)
    {
        switch (screen.bytes_per_pixel)
        {
            case 4:
                I_DrawScreen = &I_DrawScreen_4Bytes_NoScaling;
                break;
            case 3:
                I_DrawScreen = &I_DrawScreen_3Bytes_NoScaling;
                break;
            case 2:
                I_DrawScreen = &I_DrawScreen_2Bytes_NoScaling;
                break;
            case 1:
                I_DrawScreen = &I_DrawScreen_1Bytes_NoScaling;
                break;
            default:
                I_Error("Unsupported screen depth: %u\n", screen.bits_per_pixel);
        }
    }
    else
    {
        switch (screen.bytes_per_pixel)
        {
            case 4:
                I_DrawScreen = &I_DrawScreen_4Bytes_SoftwareScaling;
                break;
            case 3:
                I_DrawScreen = &I_DrawScreen_3Bytes_SoftwareScaling;
                break;
            case 2:
                I_DrawScreen = &I_DrawScreen_2Bytes_SoftwareScaling;
                break;
            case 1:
                I_DrawScreen = &I_DrawScreen_1Bytes_SoftwareScaling;
                break;
            default:
                I_Error("Unsupported screen depth: %u\n", screen.bits_per_pixel);
        }
    }

    left_x_offset   = ((screen.resx - (SCREENWIDTH  * scaling))  * screen.bytes_per_pixel) / 2;
    right_x_offset  = ((screen.resx - (SCREENWIDTH  * scaling))  * screen.bytes_per_pixel) - left_x_offset;
    y_offset_bytes  = (((screen.resy - (SCREENHEIGHT * scaling)) * screen.bytes_per_pixel) / 2) * screen.resx;

    I_VideoBuffer = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);

    screenvisible = true;
}

void I_ShutdownGraphics(void)
{
    I_Platform_ShutdownGraphics(&screen);
    if (I_VideoBuffer)
    {
        Z_Free(I_VideoBuffer);
    }
}

WEAK void I_StartFrame(void)
{
}

WEAK void I_UpdateNoBlit(void)
{
}

WEAK void I_FinishUpdate(void)
{
    I_DrawScreen();
    I_Platform_RenderFrame();
}

void I_ResetScreen(uint16_t resx, uint16_t resy, void* pixels)
{
    screen.resx = resx;
    screen.resy = resy;
    screen.pixels = pixels;

    I_ScreenRecalculate();
}

void I_ReadScreen(byte* scr)
{
    memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

#define MASK(len) ((1 << (len)) - 1)

void I_SetPalette(byte* palette)
{
    int i;
    uint32_t red, green, blue, alpha = 0xff;

    uint8_t red_offset = screen.red.offset;
    uint8_t red_len = screen.red.len;
    uint8_t red_mask = MASK(red_len);

    uint8_t green_offset = screen.green.offset;
    uint8_t green_len = screen.green.len;
    uint8_t green_mask = MASK(green_len);

    uint8_t blue_offset = screen.blue.offset;
    uint8_t blue_len = screen.blue.len;
    uint8_t blue_mask = MASK(blue_len);

    uint8_t alpha_offset = screen.alpha.offset;
    uint8_t alpha_len = screen.alpha.len;
    uint8_t alpha_mask = MASK(alpha_len);

#define COLOR(c) \
    (((c) >> (8 - c##_len) & c##_mask) << c##_offset)

    for (i = 0; i < 256; ++i)
    {
        red   = gammatable[usegamma][*palette++];
        green = gammatable[usegamma][*palette++];
        blue  = gammatable[usegamma][*palette++];

        switch (screen.bytes_per_pixel)
        {
            case 4:
                color_table.width4[i] = COLOR(red) | COLOR(green) | COLOR(blue);
                if (alpha_len)
                {
                    color_table.width4[i] |= COLOR(alpha);
                }
                break;
            case 3:
                color_table.width3[i] = COLOR(red) | COLOR(green) | COLOR(blue);
                break;
            case 2:
                color_table.width2[i] = COLOR(red) | COLOR(green) | COLOR(blue);
                break;
            case 1:
                color_table.width1[i] = COLOR(red) | COLOR(green) | COLOR(blue);
                break;
            default:
                return;
        }
    }
}

#define GFX_RGB565(r, g, b)     ((((r & 0xF8) >> 3) << 11) | (((g & 0xFC) >> 2) << 5) | ((b & 0xF8) >> 3))
#define GFX_RGB565_R(color)     ((0xF800 & color) >> 11)
#define GFX_RGB565_G(color)     ((0x07E0 & color) >> 5)
#define GFX_RGB565_B(color)     (0x001F & color)

/* Given an RGB value, find the closest matching palette index. */
int I_GetPaletteIndex(int r, int g, int b)
{
#if 0
    typedef struct
    {
        byte r;
        byte g;
        byte b;
    } col_t;

    // FIXME: empty
    static uint16_t rgb565_palette[256];

    int best, best_diff, diff;
    int i;
    col_t color;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        color.r = GFX_RGB565_R(rgb565_palette[i]);
        color.g = GFX_RGB565_G(rgb565_palette[i]);
        color.b = GFX_RGB565_B(rgb565_palette[i]);

        diff = (r - color.r) * (r - color.r)
             + (g - color.g) * (g - color.g)
             + (b - color.b) * (b - color.b);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }
    return best;
#else
    UNUSED(r && g && b);
    return 0;
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

void I_SetWindowTitle(char *title)
{
    I_Platform_SetWindowTitle(title);
}

void I_GraphicsCheckCommandLine(void)
{
}

void I_EnableLoadingDisk(void)
{
}

void I_BindVideoVariables(void)
{
}

void I_DisplayFPSDots(boolean dots_on)
{
    UNUSED(dots_on);
}

void I_CheckIsScreensaver(void)
{
}
