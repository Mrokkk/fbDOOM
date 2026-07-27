#ifndef __PLATFORM_PLATFORM_H__
#define __PLATFORM_PLATFORM_H__

#include <stdint.h>

struct color_info
{
    uint8_t offset;
    uint8_t len;
};

typedef struct color_info color_info_t;

struct screen
{
    uint16_t     resx;
    uint16_t     resy;
    uint16_t     bits_per_pixel;
    uint16_t     bytes_per_pixel;
    color_info_t red;
    color_info_t green;
    color_info_t blue;
    color_info_t alpha;
    void*        pixels;
};

typedef struct screen screen_t;

void I_Platform_InitGraphics(screen_t* s);
void I_Platform_ShutdownGraphics(screen_t* s);
void I_Platform_SetWindowTitle(char* title);
void I_Platform_RenderFrame(void);

void I_Platform_InitInput(void);
void I_Platform_ShutdownInput(void);
void I_Platform_ReadEvents(void);

#endif //  __PLATFORM_PLATFORM_H__
