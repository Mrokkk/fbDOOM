#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_mixer.h>

#include "deh_str.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_system.h"
#include "m_list.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#define CHUNK_SIZE      0x1000
#define CHANNELS_COUNT  8

static boolean initialized;
static boolean use_sfx_prefix;
static sfxinfo_t* channels[CHANNELS_COUNT];

static LIST_DECLARE(sounds);

typedef struct sfx
{
    sfxinfo_t*    sfxinfo;
    Mix_Chunk     chunk;
    list_head_t   list_entry;
    byte          data[];
} sfx_t;

static boolean PlatformSDL_InitSound(boolean use_prefix)
{
    use_sfx_prefix = use_prefix;

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        I_Printf("Failed to initialize audio: %s\n", SDL_GetError());
        return false;
    }

    snd_samplerate = 44100;

    if (Mix_OpenAudio(snd_samplerate, AUDIO_S16SYS, 2, CHUNK_SIZE) < 0)
    {
        I_Printf("Failed to initialize mixer: %s\n", Mix_GetError());
        return false;
    }

    if (Mix_AllocateChannels(CHANNELS_COUNT) != CHANNELS_COUNT)
    {
        I_Printf("Failed to allocate channels: %s\n", Mix_GetError());
        return false;
    }

    SDL_PauseAudio(0);

    return initialized = true;
}

static void PlatformSDL_ShutdownSound(void)
{
    if (!initialized)
    {
        return;
    }

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    initialized = false;
}

static int PlatformSDL_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char buf[10];

    if (sfx->link != NULL)
    {
        sfx = sfx->link;
    }

    if (use_sfx_prefix)
    {
        M_snprintf(buf, sizeof(buf), "ds%s", DEH_String(sfx->name));
    }
    else
    {
        M_StringCopy(buf, DEH_String(sfx->name), sizeof(buf));
    }

    return W_CheckNumForName(buf);
}

static void PlatformSDL_UpdateSound(void)
{
}

static int Clamp(int value, int min, int max)
{
    if (value < min)
    {
        value = min;
    }
    else if (value > max)
    {
        value = max;
    }
    return value;
}

static void PlatformSDL_UpdateSoundParams(int handle, int vol, int sep)
{
    int left, right;

    if (!initialized || handle < 0 || handle >= CHANNELS_COUNT)
    {
        return;
    }

    left = ((254 - sep) * vol) / 127;
    right = ((sep) * vol) / 127;

    Mix_SetPanning(handle, Clamp(left, 0, 255), Clamp(right, 0, 255));
}

typedef struct dmx
{
    uint16_t magic;
    uint16_t sample_rate;
    uint32_t number_of_samples;
    uint8_t  padding[16];
    uint8_t  samples[];
    /* uint8_t  padding2[16]; */
} dmx_t;

#define DMX_PADDING 32
#define DMX_MAGIC 3

static sfx_t* LoadDmx(sfxinfo_t* sfxinfo, dmx_t* dmx)
{
    Sint16*  converted;
    size_t   i, src_size, converted_size;
    sfx_t*   sfx;
    uint8_t  ratio;
    uint32_t src_index;
    uint8_t* src_data;
    float    rc, dt, alpha;
    Sint16   sample, prev_sample = 0;

    src_data = dmx->samples;
    src_size = dmx->number_of_samples - DMX_PADDING;

    if (snd_samplerate % dmx->sample_rate)
    {
        I_Printf("Cannot convert sfx with sample rate: %u\n", dmx->sample_rate);
        return NULL;
    }

    ratio = snd_samplerate / dmx->sample_rate;
    converted_size = ((src_size * snd_samplerate) / dmx->sample_rate) * 4;

    if (!(sfx = malloc(sizeof(*sfx) + converted_size)))
    {
        return NULL;
    }

    sfx->sfxinfo         = sfxinfo;
    sfx->chunk.allocated = 1;
    sfx->chunk.abuf      = (Uint8*)(sfx + 1);
    sfx->chunk.alen      = converted_size;
    sfx->chunk.volume    = SDL_MIX_MAXVOLUME;
    M_ListInit(&sfx->list_entry);

    sfxinfo->driver_data = sfx;

    converted = (Sint16*)sfx->chunk.abuf;

    /*
     * Convert u8 -> s16, 1 ch -> 2 ch and apply
     * low pass filter
     */

    dt = 1.0f / snd_samplerate;
    rc = 1.0f / (2 * 3.14f * dmx->sample_rate);
    alpha = dt / (rc + dt);

    for (i = 0; i < converted_size / 4; ++i)
    {
        src_index = i / ratio;
        sample = INT16_MIN + (Sint16)(src_data[src_index] | (src_data[src_index] << 8));
        sample = (Sint16)(sample * alpha) + (1 - alpha) * prev_sample;
        converted[i * 2] = converted[i * 2 + 1] = sample;
        prev_sample = sample;
    }

    return sfx;
}

static void LoadSfx(sfxinfo_t* sfxinfo)
{
    int    lumpnum;
    size_t lumplen;
    dmx_t* dmx;
    sfx_t* sfx;

    lumpnum = sfxinfo->lumpnum;
    dmx     = W_CacheLumpNum(lumpnum, PU_STATIC);
    lumplen = W_LumpLength(lumpnum);

    if (!dmx || lumplen < sizeof(dmx_t) || dmx->magic != DMX_MAGIC)
    {
        return;
    }

    if (dmx->number_of_samples > lumplen - 8 || dmx->number_of_samples <= 48)
    {
        return;
    }

    if ((sfx = LoadDmx(sfxinfo, dmx)))
    {
        M_ListAddHead(&sfx->list_entry, &sounds);
    }

    W_ReleaseLumpNum(lumpnum);
}

static int PlatformSDL_StartSound(sfxinfo_t* sfxinfo, int channel, int vol, int sep)
{
    sfx_t* sfx;

    if (!initialized || channel < 0 || channel >= CHANNELS_COUNT)
    {
        return -1;
    }

    sfx = sfxinfo->driver_data;

    if (!sfx)
    {
        LoadSfx(sfxinfo);
        sfx = sfxinfo->driver_data;
        if (!sfx)
        {
            return -1;
        }
    }

    Mix_PlayChannelTimed(channel, &sfx->chunk, 0, -1);

    channels[channel] = sfxinfo;

    PlatformSDL_UpdateSoundParams(channel, vol, sep);

    return channel;
}

static void PlatformSDL_StopSound(int handle)
{
    if (!initialized || handle < 0 || handle >= CHANNELS_COUNT)
    {
        return;
    }

    Mix_HaltChannel(handle);

    channels[handle] = NULL;
}

static boolean PlatformSDL_IsSoundPlaying(int handle)
{
    return !initialized || handle < 0 || handle >= CHANNELS_COUNT
        ? false
        : Mix_Playing(handle);
}

static void PlatformSDL_PrecacheSounds(sfxinfo_t* sounds, int num_sounds)
{
}

static sound_module_t sdl_sound_module = {
    &PlatformSDL_InitSound,
    &PlatformSDL_ShutdownSound,
    &PlatformSDL_GetSfxLumpNum,
    &PlatformSDL_UpdateSound,
    &PlatformSDL_UpdateSoundParams,
    &PlatformSDL_StartSound,
    &PlatformSDL_StopSound,
    &PlatformSDL_IsSoundPlaying,
    &PlatformSDL_PrecacheSounds
};

sound_module_t* I_Platform_GetSoundModule(void)
{
    return &sdl_sound_module;
}
