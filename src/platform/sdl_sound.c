#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_rwops.h>

#include "deh_str.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_system.h"
#include "m_list.h"
#include "m_misc.h"
#include "memio.h"
#include "w_wad.h"
#include "z_zone.h"

#include "mus2mid/mus2mid.h"

#define CHUNK_SIZE      0x1000
#define CHANNELS_COUNT  8

static boolean    sfx_initialized;
static int        sdl_refcount;
static boolean    use_sfx_prefix;
static sfxinfo_t* channels[CHANNELS_COUNT];

static LIST_DECLARE(sounds);

typedef struct sfx
{
    sfxinfo_t*    sfxinfo;
    Mix_Chunk     chunk;
    list_head_t   list_entry;
    byte          data[];
} sfx_t;

static boolean PlatformSDL_InitSoundSystem(void)
{
    if (!sdl_refcount)
    {
        if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
            I_Printf("Failed to initialize audio: %s\n", SDL_GetError());
            return false;
        }

        if (Mix_OpenAudio(snd_samplerate, AUDIO_S16SYS, 2, CHUNK_SIZE) < 0)
        {
            I_Printf("Failed to initialize mixer: %s\n", Mix_GetError());
            return false;
        }

        SDL_PauseAudio(0);
    }

    ++sdl_refcount;

    return true;
}

static void PlatformSDL_ShutdownSoundSystem(void)
{
    if (sdl_refcount && --sdl_refcount == 0)
    {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

static boolean PlatformSDL_InitSound(boolean use_prefix)
{
    use_sfx_prefix = use_prefix;

    if (!PlatformSDL_InitSoundSystem())
    {
        return false;
    }

    if (Mix_AllocateChannels(CHANNELS_COUNT) != CHANNELS_COUNT)
    {
        I_Printf("Failed to allocate channels: %s\n", Mix_GetError());
        return false;
    }

    return sfx_initialized = true;
}

static void PlatformSDL_ShutdownSound(void)
{
    PlatformSDL_ShutdownSoundSystem();
    sfx_initialized = false;
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

    if (!sfx_initialized || handle < 0 || handle >= CHANNELS_COUNT)
    {
        return;
    }

    left = ((254 - sep) * vol) / 127;
    right = ((sep) * vol) / 127;

    Mix_SetPanning(handle, Clamp(left, 0, 255), Clamp(right, 0, 255));
}

#define DMX_BEGIN_PADDING 16
#define DMX_END_PADDING   16
#define DMX_PADDING       (DMX_BEGIN_PADDING + DMX_END_PADDING)
#define DMX_MAGIC         3

typedef struct dmx
{
    uint16_t magic;
    uint16_t sample_rate;
    uint32_t number_of_samples;
    uint8_t  padding[DMX_BEGIN_PADDING];
    uint8_t  samples[];
    /* uint8_t  padding2[DMX_END_PADDING]; */
} dmx_t;

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
     * low pass filter (if needed)
     */

    dt = 1.0f / snd_samplerate;
    rc = 1.0f / (2 * 3.14f * dmx->sample_rate);
    alpha = dt / (rc + dt);

    if (ratio > 1)
    {
        for (i = 0; i < converted_size / 4; ++i)
        {
            src_index = i / ratio;
            sample = INT16_MIN + (Sint16)(src_data[src_index] | (src_data[src_index] << 8));
            sample = (Sint16)(sample * alpha) + (1 - alpha) * prev_sample;
            converted[i * 2] = converted[i * 2 + 1] = sample;
            prev_sample = sample;
        }
    }
    else
    {
        for (i = 0; i < converted_size / 4; ++i)
        {
            sample = INT16_MIN + (Sint16)(src_data[i] | (src_data[i] << 8));
            converted[i * 2] = converted[i * 2 + 1] = sample;
            prev_sample = sample;
        }
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

    if (!sfx_initialized || channel < 0 || channel >= CHANNELS_COUNT)
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
    if (!sfx_initialized || handle < 0 || handle >= CHANNELS_COUNT)
    {
        return;
    }

    Mix_HaltChannel(handle);

    channels[handle] = NULL;
}

static boolean PlatformSDL_IsSoundPlaying(int handle)
{
    return !sfx_initialized || handle < 0 || handle >= CHANNELS_COUNT
        ? false
        : Mix_Playing(handle);
}

static void PlatformSDL_PrecacheSounds(sfxinfo_t* sounds, int num_sounds)
{
    UNUSED(sounds && num_sounds);
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

typedef struct track
{
    MEMFILE*    memfile;
    Mix_Music*  sdl_music;
    list_head_t list_entry;
} track_t;

static boolean    music_paused;
static int        music_volume;
static LIST_DECLARE(tracks);

static boolean PlatformSDL_InitMusic(void)
{
    if (!PlatformSDL_InitSoundSystem())
    {
        return false;
    }
    Mix_SetSoundFonts("gzdoom.sf2;" DOOM_DATA_DIR "/gzdoom.sf2");
    return true;
}

static void RemoveTrack(track_t* track)
{
    if (track->sdl_music) Mix_FreeMusic(track->sdl_music);
    if (track->memfile) mem_fclose(track->memfile);
    M_ListDel(&track->list_entry);
    free(track);
}

static void PlatformSDL_ShutdownMusic(void)
{
    track_t* tmp;
    Mix_HaltMusic();
    while (!M_ListEmpty(&tracks))
    {
        tmp = M_ListNextEntry(&tracks, track_t, list_entry);
        RemoveTrack(tmp);
    }
    PlatformSDL_ShutdownSoundSystem();
}

static void UpdateMusicVolume(void)
{
    if (music_paused)
    {
        Mix_VolumeMusic(0);
    }
    else
    {
        Mix_VolumeMusic((music_volume * MIX_MAX_VOLUME) / 127);
    }
}

static void PlatformSDL_SetMusicVolume(int volume)
{
    if (volume == music_volume)
    {
        return;
    }
    music_volume = volume;
    UpdateMusicVolume();
}

static void PlatformSDL_PauseMusic(void)
{
    if (music_paused)
    {
        return;
    }
    music_paused = true;
    UpdateMusicVolume();
}

static void PlatformSDL_ResumeMusic(void)
{
    if (!music_paused)
    {
        return;
    }
    music_paused = false;
    UpdateMusicVolume();
}

static boolean ConvertMUS(void* data, int len, MEMFILE** out)
{
    MEMFILE* instream;
    MEMFILE* outstream;

    instream  = mem_fopen_read(data, len);
    outstream = mem_fopen_write();

    if (mus2mid(instream, outstream))
    {
        I_Printf("Failed to convert MUS to MIDI\n");
        goto error;
    }

    mem_fclose(instream);
    *out = outstream;

    return true;

error:
    mem_fclose(outstream);
    mem_fclose(instream);
    *out = NULL;
    return false;
}

static void* PlatformSDL_RegisterSong(void* data, int len)
{
    track_t*   track;
    SDL_RWops* ops = NULL;
    void*      midi_data = NULL;
    size_t     midi_len = 0;

    if (!(track = malloc(sizeof(track_t))))
    {
        goto error;
    }

    track->sdl_music = NULL;
    track->memfile   = NULL;
    M_ListInit(&track->list_entry);

    if (!ConvertMUS(data, len, &track->memfile))
    {
        goto error;
    }

    mem_get_buf(track->memfile, &midi_data, &midi_len);

    if (!(ops = SDL_RWFromConstMem(midi_data, midi_len)))
    {
        I_Printf("Failed to create RW ops for memory: %s\n", SDL_GetError());
        goto error;
    }

    I_RedirectOutputToBuffer_Start();

    if (!(track->sdl_music = Mix_LoadMUSType_RW(ops, MUS_MID, 0)))
    {
        char* buf = I_GetRedirectedOutput();
        I_RedirectOutputToBuffer_End();
        I_Printf("Failed to load song: %s; collected output:\n", Mix_GetError());
        fputs(buf, stderr);
        free(buf);
        goto error;
    }

    I_RedirectOutputToBuffer_End();

    M_ListAddHead(&track->list_entry, &tracks);

    SDL_FreeRW(ops);

    return track;

error:
    if (ops) SDL_FreeRW(ops);
    if (track)
    {
        if (track->memfile) mem_fclose(track->memfile);
        free(track);
    }
    return NULL;
}

static void PlatformSDL_UnregisterSong(void* handle)
{
    track_t* track = handle;
    if (track)
    {
        RemoveTrack(track);
    }
}

static void PlatformSDL_PlaySong(void* handle, boolean looping)
{
    track_t* track = handle;
    if (track && Mix_PlayMusic(track->sdl_music, looping ? -1 : 1))
    {
        I_Printf("Failed to play: %s\n", Mix_GetError());
    }
}

static void PlatformSDL_StopSong(void)
{
    Mix_HaltMusic();
}

static boolean PlatformSDL_IsMusicPlaying(void)
{
    return Mix_PlayingMusic();
}

static void PlatformSDL_Poll(void)
{
}

static music_module_t sdl_music_module = {
    &PlatformSDL_InitMusic,
    &PlatformSDL_ShutdownMusic,
    &PlatformSDL_SetMusicVolume,
    &PlatformSDL_PauseMusic,
    &PlatformSDL_ResumeMusic,
    &PlatformSDL_RegisterSong,
    &PlatformSDL_UnregisterSong,
    &PlatformSDL_PlaySong,
    &PlatformSDL_StopSong,
    &PlatformSDL_IsMusicPlaying,
    &PlatformSDL_Poll
};

struct music_module* I_Platform_GetMusicModule(void)
{
    return &sdl_music_module;
}
