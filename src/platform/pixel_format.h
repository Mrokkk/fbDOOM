#ifndef __PLATFORM_PIXEL_FORMAT_H__
#define __PLATFORM_PIXEL_FORMAT_H__

#define SET_PIXEL_FORMAT_R8B8G8A8(screen) \
    do \
    { \
        (screen)->bits_per_pixel  = 32; \
        (screen)->bytes_per_pixel = 4;  \
        (screen)->red.offset      = 0;  \
        (screen)->red.len         = 8;  \
        (screen)->green.offset    = 8;  \
        (screen)->green.len       = 8;  \
        (screen)->blue.offset     = 16; \
        (screen)->blue.len        = 8;  \
        (screen)->alpha.offset    = 24; \
        (screen)->alpha.len       = 8;  \
    } \
    while (0)

#define SET_PIXEL_FORMAT_G8B8R8A8(screen) \
    do \
    { \
        (screen)->bits_per_pixel  = 32; \
        (screen)->bytes_per_pixel = 4;  \
        (screen)->red.offset      = 16; \
        (screen)->red.len         = 8;  \
        (screen)->green.offset    = 8;  \
        (screen)->green.len       = 8;  \
        (screen)->blue.offset     = 0;  \
        (screen)->blue.len        = 8;  \
        (screen)->alpha.offset    = 24; \
        (screen)->alpha.len       = 8;  \
    } \
    while (0)

#endif // __PLATFORM_PIXEL_FORMAT_H__
