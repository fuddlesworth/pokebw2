#include "audio.h"
#include "../lib/nnsys/src/snd/snd_internal.h"
#include "constants/sound.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "microphone.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "sequence.h"
#include "settings.h"
#include <SDL3/SDL.h>
#include <limits.h>

// SDL consumes decoded samples. Archive lookup and the game's stream/cry lifetimes remain in NNS/GFL.
static NNSSndArc archive;
static u8 *archive_data;
static size_t archive_size;
static MATHRandContext32 random_context;
static SDL_AudioStream *sequence_stream;
static unsigned sequence_volume[16];
static unsigned bgm_player;
static unsigned bgm_ids[16];
static unsigned sequence_ids[16];
static NNSSndHandle player_handles[16];
static unsigned bgm_stack[5], bgm_depth;
static NNSSndSeqPlayer bgm_handles[16];
static void *sound_heap_memory;
static NNSSndHeapHandle sound_heap;
static NNSSndHandle bgm_handle;
static struct {
    bool active;
    u32 sequence;
    u16 mask, fade_in;
} bgm_change;
static unsigned bgm_fade_frames, bgm_fade_total;
static bool bgm_fade_in;
static float sequence_peak;
static float master_volume = 1.0f;
static bool mono_output;

struct NNSSndWaveOut {
    SDL_AudioStream *stream;
    double frames;
    bool allocated;
};
static struct NNSSndWaveOut voices[16];

struct NNSSndStrmPlayer {
    SDL_AudioStream *stream;
    s16 *samples;
    u32 sample_count, loop_start, rate, position;
    u32 fade_frames, fade_total;
    unsigned channels;
    float volume;
    bool loop, playing;
};
static struct NNSSndStrmPlayer music;

void NNS_SndSetMasterVolume(int volume) {
    master_volume = SDL_clamp(volume, 0, 127) / 127.0f;
    if (sequence_stream)
        SDL_SetAudioDeviceGain(SDL_GetAudioStreamDevice(sequence_stream),
                               master_volume * PcSettings_Get()->volume / 10.0f);
    if (music.stream)
        SDL_SetAudioDeviceGain(SDL_GetAudioStreamDevice(music.stream),
                               master_volume * PcSettings_Get()->volume / 10.0f);
    for (unsigned i = 0; i < SDL_arraysize(voices); i++) {
        if (voices[i].stream)
            SDL_SetAudioDeviceGain(SDL_GetAudioStreamDevice(voices[i].stream),
                                   master_volume * PcSettings_Get()->volume / 10.0f);
    }
}

static u16 Read16(const u8 *data) {
    return (u16)(data[0] | (u16)data[1] << 8);
}
static u32 Read32(const u8 *data) {
    return data[0] | (u32)data[1] << 8 | (u32)data[2] << 16 | (u32)data[3] << 24;
}
static bool InArchive(size_t offset, size_t size) {
    return offset <= archive_size && size <= archive_size - offset;
}

bool PcAudio_Init(const char *root) {
    char *path = NULL;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO) || SDL_asprintf(&path, "%s/swan_sound_data.sdat", root) < 0)
        return false;
    archive_data = SDL_LoadFile(path, &archive_size);
    if (!archive_data || archive_size < sizeof(archive.header) || SDL_memcmp(archive_data, "SDAT", 4) != 0) {
        SDL_SetError("Invalid sound archive");
        goto fail;
    }
    SDL_memcpy(&archive.header, archive_data, sizeof(archive.header));
    u32 info = Read32(archive_data + 24), fat = Read32(archive_data + 32);
    if (!InArchive(info, archive.header.infoSize) || archive.header.infoSize < sizeof(NNSSndArcInfo) ||
        !InArchive(fat, 12)) {
        SDL_SetError("Invalid SDAT tables");
        goto fail;
    }
    u32 count = Read32(archive_data + fat + 8);
    if (count > (archive_size - fat - 12) / 16) {
        SDL_SetError("Invalid SDAT file count");
        goto fail;
    }
    archive.fat = SDL_calloc(1, sizeof(*archive.fat) + count * sizeof(NNSSndArcFileInfo));
    if (!archive.fat) {
        goto fail;
    }
    archive.fat->count = count;
    for (u32 i = 0; i < count; i++) {
        NNSSndArcFileInfo *file = &archive.fat->files[i];
        file->offset = Read32(archive_data + fat + 12 + i * 16);
        file->size = Read32(archive_data + fat + 16 + i * 16);
        if (!InArchive(file->offset, file->size)) {
            SDL_SetError("Invalid SDAT file extent");
            goto fail;
        }
        file->mem = archive_data + file->offset;
    }
    archive.info = (NNSSndArcInfo *)(archive_data + info);
    FS_InitFile(&archive.file);
    bool opened = FS_OpenFile(&archive.file, "swan_sound_data.sdat");
    SDL_free(path);
    path = NULL;
    if (!opened)
        goto fail;
    archive.file_open = TRUE;
    NNS_SndArcSetCurrent(&archive);
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 48000 };
    sequence_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (sequence_stream)
        SDL_SetAudioDeviceGain(SDL_GetAudioStreamDevice(sequence_stream),
                               master_volume * PcSettings_Get()->volume / 10.0f);
    if (!sequence_stream || !SDL_ResumeAudioStreamDevice(sequence_stream))
        goto fail;
    PcSequence_Init();
    sequence_peak = 0;
    MATH_InitRand32(&random_context, 0);
    PokeVoice_Init(0, getResIDFunc, getWaveInfoFunc);
    return true;
fail:
    SDL_free(path);
    PcAudio_Shutdown();
    return false;
}

static void FreeMusic(void) {
    if (music.stream)
        SDL_DestroyAudioStream(music.stream);
    SDL_free(music.samples);
    SDL_zero(music);
}

void PcAudio_Shutdown(void) {
    PcMic_Shutdown();
    if (sequence_peak > 0)
        SDL_Log("Rendered sequence audio; peak %.6f", sequence_peak);
    PcSequence_Shutdown();
    if (sequence_stream)
        SDL_DestroyAudioStream(sequence_stream);
    sequence_stream = NULL;
    FreeMusic();
    for (unsigned i = 0; i < SDL_arraysize(voices); i++)
        NNS_SndWaveOutFreeChannel(&voices[i]);
    if (sound_heap)
        NNS_SndHeapDestroy(sound_heap);
    sound_heap = NULL;
    SDL_free(sound_heap_memory);
    sound_heap_memory = NULL;
    NNS_SndArcSetCurrent(NULL);
    if (archive.file_open)
        FS_CloseFile(&archive.file);
    SDL_free(archive.fat);
    SDL_free(archive_data);
    archive_data = NULL;
    archive_size = 0;
    SDL_zero(archive);
}

// IMA ADPCM, the same predictor and index bounds used by the recovered NNS stream decoder.
int16_t PcAudio_DecodeSample(int nibble, int *sample, int *index) {
    static const s16 steps[] = { 7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,
                                 23,    25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,
                                 73,    80,    88,    97,    107,   118,   130,   143,   157,   173,   190,   209,
                                 230,   253,   279,   307,   337,   371,   408,   449,   494,   544,   598,   658,
                                 724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
                                 2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
                                 7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
                                 22385, 24623, 27086, 29794, 32767 };
    static const int changes[] = { -1, -1, -1, -1, 2, 4, 6, 8 };
    int step = steps[*index];
    int delta =
        (step >> 3) + ((nibble & 4) ? step : 0) + ((nibble & 2) ? step >> 1 : 0) + ((nibble & 1) ? step >> 2 : 0);
    *sample = SDL_clamp(*sample + ((nibble & 8) ? -delta : delta), -32768, 32767);
    *index = SDL_clamp(*index + changes[nibble & 7], 0, 88);
    return (s16)*sample;
}

static bool LoadStream(int number, u32 offset) {
    const NNSSndArcStrmInfo *info = NNS_SndArcGetStrmInfo(number);
    if (!info)
        return SDL_SetError("Missing sound stream %d", number);
    const u8 *data = NNS_SndArcGetFileAddress(info->fileId);
    u32 size = NNS_SndArcGetFileSize(info->fileId);
    if (!data || size < 64 || SDL_memcmp(data, "STRM", 4) != 0)
        return SDL_SetError("Invalid STRM header");
    unsigned format = data[24], channels = data[26];
    u32 count = Read32(data + 36), blocks = Read32(data + 44);
    u32 block_size = Read32(data + 48), block_samples = Read32(data + 52);
    if (format > 2 || channels == 0 || channels > 2 || !Read16(data + 28) || !count || !blocks ||
        count > INT_MAX / (channels * sizeof(s16)))
        return SDL_SetError("Unsupported STRM dimensions");
    music.samples = SDL_malloc((size_t)count * channels * sizeof(s16));
    if (!music.samples)
        return false;
    u32 written = 0;
    for (u32 block = 0; block < blocks; block++) {
        u32 bytes = block + 1 == blocks ? Read32(data + 56) : block_size;
        u32 samples = block + 1 == blocks ? Read32(data + 60) : block_samples;
        u64 start = Read32(data + 40) + (u64)block * block_size * channels;
        u64 needed = format == 0 ? samples : format == 1 ? (u64)samples * 2 : 4 + ((u64)samples + 1) / 2;
        if (written > count || samples > count - written || needed > bytes || start > size ||
            (u64)bytes * channels > size - start)
            return SDL_SetError("Invalid STRM block");
        for (unsigned channel = 0; channel < channels; channel++) {
            const u8 *source = data + start + channel * bytes;
            int sample = format == 2 ? (s16)Read16(source) : 0;
            int index = format == 2 ? source[2] : 0;
            if (index > 88)
                return SDL_SetError("Invalid STRM ADPCM predictor");
            for (u32 i = 0; i < samples; i++) {
                s16 value = format == 0 ? (s8)source[i] * 256
                            : format == 1
                                ? (s16)Read16(source + i * 2)
                                : PcAudio_DecodeSample((source[4 + i / 2] >> ((i & 1) * 4)) & 15, &sample, &index);
                music.samples[(written + i) * channels + channel] = value;
            }
        }
        written += samples;
    }
    if (written != count)
        return SDL_SetError("STRM sample count mismatch");
    music.rate = Read16(data + 28);
    music.channels = channels;
    music.sample_count = count;
    music.loop_start = Read32(data + 32);
    music.loop = data[25] != 0;
    music.position = (u64)offset * music.rate / 1000;
    if (music.position >= count || (music.loop && music.loop_start >= count))
        return SDL_SetError("Invalid STRM loop or start position");
    music.volume = info->volume / 127.0f;
    SDL_AudioSpec spec = { .format = SDL_AUDIO_S16, .channels = (int)channels, .freq = (int)music.rate };
    music.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (music.stream)
        SDL_SetAudioDeviceGain(SDL_GetAudioStreamDevice(music.stream),
                               master_volume * PcSettings_Get()->volume / 10.0f);
    return music.stream && SDL_SetAudioStreamGain(music.stream, music.volume);
}

void PcAudio_Update(void) {
    static int appliedVolume = -1;
    const PcSettings *settings = PcSettings_Get();
    if (appliedVolume != settings->volume) {
        appliedVolume = settings->volume;
        NNS_SndSetMasterVolume((int)(master_volume * 127.0f + 0.5f));
    }
    if (settings->gameValid)
        mono_output = settings->sound != 0;
    PcMic_Update();
    if (sequence_stream) {
        if (bgm_fade_frames) {
            bgm_fade_frames--;
            unsigned elapsed = bgm_fade_in ? bgm_fade_total - bgm_fade_frames : bgm_fade_frames;
            PcSequence_Volume(bgm_player, sequence_volume[bgm_player] * elapsed / bgm_fade_total);
            if (!bgm_fade_frames && !bgm_fade_in)
                PcSequence_Stop(bgm_player);
        }
        if (bgm_change.active && !bgm_fade_frames) {
            u16 fade_in = bgm_change.fade_in;
            GFL_SndBGMPlay(bgm_change.sequence, bgm_change.mask);
            GFL_SndBGMFadeIn(fade_in);
        }
        float samples[800 * 2];
        PcSequence_Render(samples, 800);
        if (mono_output) {
            for (unsigned i = 0; i < 800; i++) {
                float sample = (samples[i * 2] + samples[i * 2 + 1]) * 0.5f;
                samples[i * 2] = samples[i * 2 + 1] = sample;
            }
        }
        for (unsigned i = 0; i < SDL_arraysize(samples); i++)
            sequence_peak = SDL_max(sequence_peak, SDL_fabsf(samples[i]));
        // Bounded fast-forward tests still advance the sequence clock without building an unlimited audio queue.
        if (SDL_GetAudioStreamQueued(sequence_stream) < 48000 * 2 * (int)sizeof(float) / 4 &&
            !SDL_PutAudioStreamData(sequence_stream, samples, sizeof(samples)))
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Sequence output: %s", SDL_GetError());
    }
    for (unsigned i = 0; i < SDL_arraysize(voices); i++)
        if (voices[i].frames > 0)
            voices[i].frames--;
    PokeVoice_Update();
    if (!music.playing)
        return;
    if (music.fade_frames) {
        music.fade_frames--;
        SDL_SetAudioStreamGain(music.stream, music.volume * music.fade_frames / music.fade_total);
        if (!music.fade_frames) {
            music.playing = false;
            SDL_ClearAudioStream(music.stream);
            return;
        }
    }
    if (SDL_GetAudioStreamQueued(music.stream) < 4096) {
        if (music.position == music.sample_count) {
            if (music.loop)
                music.position = music.loop_start;
            else {
                if (SDL_GetAudioStreamQueued(music.stream) == 0 && SDL_GetAudioStreamAvailable(music.stream) == 0)
                    music.playing = false;
                return;
            }
        }
        u32 samples = SDL_min(4096u, music.sample_count - music.position);
        const s16 *source = music.samples + music.position * music.channels;
        s16 mono[4096 * 2];
        if (mono_output && music.channels == 2) {
            for (u32 i = 0; i < samples; i++)
                mono[i * 2] = mono[i * 2 + 1] = (source[i * 2] + source[i * 2 + 1]) / 2;
            source = mono;
        }
        if (!SDL_PutAudioStreamData(music.stream, source, samples * music.channels * sizeof(s16))) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Stream playback: %s", SDL_GetError());
            music.playing = false;
            return;
        }
        music.position += samples;
        if (music.position == music.sample_count)
            SDL_FlushAudioStream(music.stream);
    }
}

NNSSndHeapHandle GFL_SndGetHeap(void) {
    if (!sound_heap) {
        sound_heap_memory = SDL_malloc(0x140000);
        if (sound_heap_memory)
            sound_heap = NNS_SndHeapCreate(sound_heap_memory, 0x140000);
        if (!sound_heap)
            GFL_DebugAssertFailEx(__FILE__, __LINE__, "Creating native sound heap");
    }
    return sound_heap;
}
BOOL NNS_SndArcLoadGroup(int groupNo, NNSSndHeapHandle heap) {
    const NNSSndArcGroupInfo *group = NNS_SndArcGetGroupInfo(groupNo);
    (void)heap;
    if (!group)
        return FALSE;
    // PcAudio_Init makes every archive file resident; group loads validate those entries.
    for (u32 i = 0; i < group->count; i++) {
        const NNSSndArcGroupItem *item = &group->item[i];
        u32 file;
        switch (item->type) {
        case 0: {
            const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(item->index);
            if (!info)
                return FALSE;
            file = info->fileId;
            break;
        }
        case 1: {
            const NNSSndArcBankInfo *info = NNS_SndArcGetBankInfo(item->index);
            if (!info)
                return FALSE;
            file = info->fileId;
            break;
        }
        case 2: {
            const NNSSndArcWaveArcInfo *info = NNS_SndArcGetWaveArcInfo(item->index);
            if (!info)
                return FALSE;
            file = info->fileId;
            break;
        }
        case 3: {
            const NNSSndArcSeqArcInfo *info = NNS_SndArcGetSeqArcInfo(item->index);
            if (!info)
                return FALSE;
            file = info->fileId;
            break;
        }
        default:
            return FALSE;
        }
        if (!NNS_SndArcGetFileAddress(file))
            return FALSE;
    }
    return TRUE;
}
BOOL GFL_SndIsLoadThreadRunning(void) {
    return FALSE;
}
static PcSoundResource SoundFile(u32 id) {
    return (PcSoundResource){ NNS_SndArcGetFileAddress(id), NNS_SndArcGetFileSize(id) };
}
static void StartSequence(u32 number, int player) {
    const NNSSndArcSeqInfo *seq = NNS_SndArcGetSeqInfo(number);
    const NNSSndArcBankInfo *bank = seq ? NNS_SndArcGetBankInfo(seq->param.bankNo) : NULL;
    if (!seq || !bank)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Missing sequence/bank %u", number);
    if (player < 0)
        player = seq->param.playerNo;
    PcSoundResource waves[4] = { { 0 } };
    for (unsigned i = 0; i < 4; i++) {
        if (bank->waveArcNo[i] == NNS_SND_ARC_INVALID_WAVEARC_NO)
            continue;
        const NNSSndArcWaveArcInfo *wave = NNS_SndArcGetWaveArcInfo(bank->waveArcNo[i]);
        if (wave)
            waves[i] = SoundFile(wave->fileId);
    }
    if (!PcSequence_Start(player, SoundFile(seq->fileId), SoundFile(bank->fileId), waves, seq->param.volume,
                          seq->param.playerPrio))
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Starting sequence %u on player %d", number, player);
    sequence_volume[player] = seq->param.volume;
    sequence_ids[player] = number;
    SDL_Log("Playing original SSEQ %u on player %d", number, player);
}
void GFL_SndBGMPlay(u32 bgm, u32 channelMask) {
    bgm_change.active = false;
    PcSequence_Stop(bgm_player);
    bgm_player = bgm_depth ? 6 + bgm_depth : 0;
    bgm_fade_frames = 0;
    bgm_ids[bgm_player] = bgm;
    StartSequence(bgm, bgm_player);
    PcSequence_ChannelMask(bgm_player, channelMask);
}
void GFL_SndBGMStop(u32 bgm) {
    bgm_change.active = false;
    PcSequence_Stop(bgm_player);
    bgm_player = bgm_depth ? 6 + bgm_depth : 6;
    bgm_fade_frames = 0;
    bgm_ids[bgm_player] = bgm;
    StartSequence(bgm, bgm_player);
}
void GFL_SndSEPlay(u32 se) {
    StartSequence(se, -1);
}
void GFL_SEPlayKeepVol(u32 se, s32 player) {
    if (player < 0 || player >= 5)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid sound-effect player %d", player);
    unsigned volume = sequence_volume[player + 1];
    StartSequence(se, player + 1);
    sequence_volume[player + 1] = volume;
    PcSequence_Volume(player + 1, volume);
}
BOOL GFL_SndIsPlaying(u32 seq) {
    for (unsigned i = 0; i < SDL_arraysize(sequence_ids); i++) {
        if (sequence_ids[i] == seq && PcSequence_Active(i))
            return TRUE;
    }
    return FALSE;
}
u32 GFL_SndBGMGetTick(void) {
    return PcSequence_Tick(bgm_player);
}
void GFL_SndBGMStopSeq(void) {
    GFL_SndBGMStopImmediate();
}
void GFL_SndBGMClearLoadedData(void) {
    u32 bank = NNS_SndArcGetBankInfo(0x3f2)->fileId;
    u32 sequence = NNS_SndArcGetSeqInfo(SEQ_BGM_SHINKA)->fileId;
    // Restore resident SDAT data after the musical's temporary sequence and bank are released.
    NNS_SndArcSetFileAddress(bank, archive_data + NNS_SndArcGetFileOffset(bank));
    NNS_SndArcSetFileAddress(sequence, archive_data + NNS_SndArcGetFileOffset(sequence));
    u32 fat = Read32(archive_data + 32);
    archive.fat->files[bank].size = Read32(archive_data + fat + 16 + bank * 16);
    archive.fat->files[sequence].size = Read32(archive_data + fat + 16 + sequence * 16);
    PcSequence_ClearWaveReplacements();
}
BOOL GFL_SndBGMSetLoadedData(void *seqData, void *bankData, u32 seq) {
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(seq);
    const NNSSndArcBankInfo *bank = info ? NNS_SndArcGetBankInfo(info->param.bankNo) : NULL;
    if (!bank || !seqData || !bankData)
        return FALSE;
    NNS_SndArcSetFileAddress(info->fileId, seqData);
    NNS_SndArcSetFileAddress(bank->fileId, bankData);
    archive.fat->files[info->fileId].size = Read32((u8 *)seqData + 8);
    archive.fat->files[bank->fileId].size = Read32((u8 *)bankData + 8);
    bgm_ids[bgm_player] = seq;
    return TRUE;
}
void GFL_SndBGMStartSeq(u16 seq) {
    StartSequence(seq, bgm_player);
    bgm_ids[bgm_player] = seq;
}
BOOL GFL_SndReplaceWave(u16 waveArc, u16 index, const SNDWaveArc *source, u16 sourceIndex) {
    const NNSSndArcWaveArcInfo *info = NNS_SndArcGetWaveArcInfo(waveArc);
    if (!info || !source)
        return FALSE;
    PcSoundResource resource = { (const u8 *)source, Read32((const u8 *)source + 8) };
    return PcSequence_ReplaceWave(SoundFile(info->fileId), index, resource, sourceIndex);
}
BOOL GFL_IRQCartDataTransferIsEnabled(void) {
    // Native archive reads complete synchronously, without cartridge DMA interrupts.
    return FALSE;
}
NNSSndHandle *GFL_SndPlayerGetHandle(s32 player) {
    if (player < 0 || player >= 5)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid sound-effect player %d", player);
    bgm_handles[player + 1].playerNo = player + 1;
    player_handles[player + 1].player = &bgm_handles[player + 1];
    return &player_handles[player + 1];
}
void NNS_SndPlayerSetPlayerVolume(int playerNo, int volume) {
    if (playerNo >= 0 && playerNo < 16) {
        sequence_volume[playerNo] = SDL_clamp(volume, 0, 127);
        PcSequence_Volume(playerNo, sequence_volume[playerNo]);
    }
}
void NNS_SndPlayerSetVolume(NNSSndHandle *handle, int volume) {
    if (handle && handle->player)
        NNS_SndPlayerSetPlayerVolume(handle->player->playerNo, volume);
}
void GFL_SndPlayerSetVolumeEx(u32 volume, u32 playerMask) {
    for (unsigned i = 0; i < 16; i++) {
        if (playerMask & (1u << i))
            NNS_SndPlayerSetPlayerVolume(i, volume);
    }
}
void GFL_SndPlayerSetMuteStateEx(BOOL on, u32 playerMask) {
    GFL_SndPlayerSetVolumeEx(on ? 127 : 0, playerMask);
}
void GFL_SndSetStereo(BOOL stereo) {
    mono_output = stereo != TRUE;
}
void GFL_SndDestroyHeap(void) {
    // The archive remains resident, but no voices may keep references across the game's sound reset.
    for (unsigned i = 0; i < 16; i++) {
        PcSequence_Stop(i);
        NNS_SndWaveOutStop(&voices[i]);
    }
    if (music.stream)
        SDL_ClearAudioStream(music.stream);
    music.playing = false;
    bgm_change.active = false;
    bgm_fade_frames = 0;
    if (sound_heap)
        NNS_SndHeapClear(sound_heap);
}
void GFL_SndInit(void) {
    GFL_SndDestroyHeap();
    PcSequence_Init();
    SDL_zero(sequence_ids);
    SDL_zero(bgm_ids);
    SDL_zero(player_handles);
    bgm_handle.player = NULL;
    bgm_depth = 0;
    bgm_player = 0;
    for (unsigned i = 0; i < 16; i++)
        sequence_volume[i] = 127;
    NNS_SndSetMasterVolume(127);
}
void GFL_SndSEPlayEx(u32 se, u32 volume) {
    GFL_SndSEPlay(se);
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(se);
    PcSequence_Volume(info->param.playerNo, volume);
}
u32 GFL_SndBGMGetID(void) {
    return bgm_ids[bgm_player];
}
NNSSndHandle *GFL_SndBGMGetHandle(void) {
    bgm_handles[bgm_player].playerNo = bgm_player;
    bgm_handle.player = &bgm_handles[bgm_player];
    return &bgm_handle;
}
void NNS_SndPlayerSetTrackPitch(NNSSndHandle *handle, u16 trackBitMask, int pitch) {
    if (handle && handle->player)
        PcSequence_TrackParams(handle->player->playerNo, trackBitMask, -1, pitch, -1, -1);
}
void NNS_SndPlayerSetTrackModDepth(NNSSndHandle *handle, u16 trackBitMask, int value) {
    if (handle && handle->player)
        PcSequence_Modulation(handle->player->playerNo, trackBitMask, value, -1);
}
void NNS_SndPlayerSetTrackModSpeed(NNSSndHandle *handle, u16 trackBitMask, int value) {
    if (handle && handle->player)
        PcSequence_Modulation(handle->player->playerNo, trackBitMask, -1, value);
}
void GFL_SndBGMRequestChange(u32 bgm, u16 channelMask, s32 fadeOutFrames, s32 fadeInFrames) {
    GFL_SndBGMFadeOut((u16)SDL_clamp(fadeOutFrames, 0, 65535));
    bgm_change.active = true;
    bgm_change.sequence = bgm;
    bgm_change.mask = channelMask;
    bgm_change.fade_in = (u16)SDL_clamp(fadeInFrames, 0, 65535);
}
void GFL_SndSetSeqVerifyCallback(BOOL (*callback)(u32 seq)) {
    // Native archive reads finish synchronously, so playback never needs a load-thread gate.
    (void)callback;
}
void GFL_SndBGMSetPaused(BOOL paused) {
    PcSequence_Pause(bgm_player, paused);
}
void GFL_SndBGMSetVolume(u16 trackMask, s32 volume) {
    PcSequence_TrackParams(bgm_player, trackMask, -1, -1, -1, volume);
}
void GFL_SndBGMSetParams(u16 trackMask, s32 tempoRatio, s32 pitch, s32 pan) {
    PcSequence_TrackParams(bgm_player, trackMask, tempoRatio, pitch, pan, -1);
}
void GFL_SndBGMPush(void) {
    if (bgm_depth == SDL_arraysize(bgm_stack))
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "BGM stack overflow");
    PcSequence_Pause(bgm_player, true);
    bgm_stack[bgm_depth++] = bgm_player;
    bgm_player = 6 + bgm_depth;
    bgm_ids[bgm_player] = 0;
    bgm_fade_frames = 0;
}
void GFL_SndBGMPop(void) {
    if (!bgm_depth)
        return;
    PcSequence_Stop(bgm_player);
    bgm_player = bgm_stack[--bgm_depth];
    bgm_fade_frames = 0;
}
BOOL GFL_SndBGMLoadStep(u32 seq, u32 *step, BOOL start) {
    // Files are resident; retain the original start/poll protocol for field transitions.
    if (start) {
        GFL_SndBGMStopImmediate();
        *step = 0;
        return FALSE;
    }
    // The verify callback only restricts playback during asynchronous loading;
    // native loading has already finished before a sequence can be started.
    GFL_SndBGMPlay(seq, 0xffff);
    *step = 3;
    return TRUE;
}
void GFL_SndUpdate(void) {
    PcAudio_Update();
}
void GFL_SndBGMStopImmediate(void) {
    bgm_change.active = false;
    bgm_fade_frames = 0;
    PcSequence_Stop(bgm_player);
    bgm_ids[bgm_player] = 0;
}
BOOL GFL_SndBGMIsPlaying(void) {
    return PcSequence_Active(bgm_player);
}
BOOL GFL_SndBGMIsFading(void) {
    return bgm_fade_frames != 0;
}
void GFL_SndBGMFadeOut(u16 frames) {
    bgm_fade_in = false;
    bgm_fade_frames = bgm_fade_total = frames;
    if (!frames)
        GFL_SndBGMStopImmediate();
}
void GFL_SndBGMFadeIn(u16 frames) {
    bgm_fade_in = true;
    bgm_fade_frames = bgm_fade_total = frames;
    PcSequence_Volume(bgm_player, frames ? 0 : sequence_volume[bgm_player]);
}
s32 GFL_SndSeqGetPlayerIndex(u32 sequence) {
    if (sequence < SEQ_SE_DUMMY || sequence >= SEQ_SE_END)
        return 1;
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(sequence);
    return info ? (s32)info->param.playerNo - 1 : -1;
}
void GFL_SndPlayerSetVolume(s32 player, u32 volume) {
    if (player >= 0 && player < 5)
        PcSequence_Volume(player + 1, volume);
}
void GFL_SndPlayerSetParams(s32 player, s32 tempo, s32 pitch, s32 pan) {
    if (player >= 0 && player < 5)
        PcSequence_Params(player + 1, tempo, pitch, pan);
}
void GFL_SndPlayerStop(s32 player) {
    if (player >= 0 && player < 5)
        PcSequence_Stop(player + 1);
}
void GFL_SndStop(void) {
    for (unsigned player = 1; player <= 5; player++)
        PcSequence_Stop(player);
}
BOOL GFL_SndPlayerIsActiveAny(void) {
    for (unsigned i = 1; i <= 5; i++)
        if (PcSequence_Active(i))
            return TRUE;
    return FALSE;
}
BOOL GFL_SndPlayerIsActive(s32 player) {
    return player >= 0 && player < 5 && PcSequence_Active(player + 1);
}
u32 GFL_RandomLC(u32 max) {
    return MATH_Rand32(&random_context, max);
}
u32 GFL_RandomLCAlt(u32 max) {
    random_context.x = random_context.mul * random_context.x + random_context.add;
    return (u32)(((random_context.x >> 32) * max) >> 32);
}
u32 GFL_SndBGMStack_Preload(const u32 *seqs, u32 count) {
    // The native archive is resident for the session, including sequence banks and wave archives.
    for (u32 i = 0; i < count; i++) {
        if (!NNS_SndArcGetSeqInfo(seqs[i]))
            GFL_DebugAssertFailEx(__FILE__, __LINE__, "Missing sequence %u", seqs[i]);
    }
    return 0;
}
void GFL_SndBGMStack_Unload(u32 handle) {
    (void)handle;
}
void OS_Sleep(u32 milliseconds) {
    SDL_Delay(milliseconds);
}
void NNS_SndArcStrmInit(u32 priority, NNSSndHeapHandle heap) {
    (void)priority;
    (void)heap;
}
BOOL NNS_SndArcStrmAllocChannel(int player) {
    return player == 0;
}
void NNS_SndArcStrmFreeChannel(int player) {
    if (player == 0)
        FreeMusic();
}
void NNS_SndStrmHandleInit(NNSSndStrmHandle *handle) {
    handle->player = NULL;
}
void NNS_SndStrmHandleRelease(NNSSndStrmHandle *handle) {
    handle->player = NULL;
}
BOOL NNS_SndArcStrmStart(NNSSndStrmHandle *handle, int number, u32 offset) {
    FreeMusic();
    handle->player = NULL;
    if (!LoadStream(number, offset)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Starting title audio: %s", SDL_GetError());
        FreeMusic();
        return FALSE;
    }
    handle->player = &music;
    NNS_SndArcStrmStartPrepared(handle);
    SDL_Log("Playing original STRM %d: %u Hz, %u channels", number, music.rate, music.channels);
    return TRUE;
}
void NNS_SndArcStrmStartPrepared(NNSSndStrmHandle *handle) {
    if (handle->player && music.stream) {
        music.playing = true;
        SDL_ResumeAudioStreamDevice(music.stream);
        PcAudio_Update();
    }
}
void NNS_SndArcStrmStop(NNSSndStrmHandle *handle, int frames) {
    if (!handle->player)
        return;
    music.fade_total = music.fade_frames = frames > 0 ? (u32)frames : 0;
    if (frames <= 0) {
        music.playing = false;
        if (music.stream)
            SDL_ClearAudioStream(music.stream);
    }
}
u32 NNS_SndArcStrmGetCurrentPlayingPos(NNSSndStrmHandle *handle) {
    return handle->player && music.playing ? 1 + (u64)music.position * 1000 / music.rate : 0;
}

NNSSndWaveOutHandle NNS_SndWaveOutAllocChannel(int channel) {
    if (channel < 0 || channel >= (int)SDL_arraysize(voices) || voices[channel].allocated)
        return NULL;
    voices[channel].allocated = true;
    return &voices[channel];
}
void NNS_SndWaveOutFreeChannel(NNSSndWaveOutHandle handle) {
    if (!handle)
        return;
    if (handle->stream)
        SDL_DestroyAudioStream(handle->stream);
    SDL_zero(*handle);
}
BOOL NNS_SndWaveOutStart(NNSSndWaveOutHandle handle, NNSSndWaveFormat format, const void *data, BOOL loop,
                         int loop_start, int samples, int rate, int volume, int speed, int pan) {
    (void)loop_start;
    if (!handle || !data || samples <= 0 || samples > INT_MAX / 8 || rate <= 0 || speed <= 0 || loop)
        return FALSE;
    if (format != NNS_SND_WAVE_FORMAT_PCM8 && format != NNS_SND_WAVE_FORMAT_PCM16)
        return FALSE;
    float *stereo = SDL_malloc((size_t)samples * 2 * sizeof(float));
    if (!stereo)
        return FALSE;
    float right = mono_output ? 0.5f : SDL_clamp(pan, 0, 127) / 127.0f;
    for (int i = 0; i < samples; i++) {
        float sample =
            format == NNS_SND_WAVE_FORMAT_PCM8 ? ((const s8 *)data)[i] / 128.0f : ((const s16 *)data)[i] / 32768.0f;
        stereo[i * 2] = sample * (1 - right);
        stereo[i * 2 + 1] = sample * right;
    }
    SDL_AudioSpec spec = { .format = SDL_AUDIO_F32, .channels = 2, .freq = rate };
    if (handle->stream)
        SDL_DestroyAudioStream(handle->stream);
    handle->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (handle->stream)
        SDL_SetAudioDeviceGain(SDL_GetAudioStreamDevice(handle->stream),
                               master_volume * PcSettings_Get()->volume / 10.0f);
    bool result = handle->stream && SDL_SetAudioStreamGain(handle->stream, volume / 127.0f) &&
                  SDL_SetAudioStreamFrequencyRatio(handle->stream, speed / 32768.0f) &&
                  SDL_PutAudioStreamData(handle->stream, stereo, samples * 2 * sizeof(float)) &&
                  SDL_FlushAudioStream(handle->stream) && SDL_ResumeAudioStreamDevice(handle->stream);
    SDL_free(stereo);
    handle->frames = result ? (double)samples * 60 * 32768 / rate / speed : 0;
    return result;
}
BOOL NNS_SndWaveOutIsPlaying(NNSSndWaveOutHandle handle) {
    return handle && handle->frames > 0;
}
void NNS_SndWaveOutStop(NNSSndWaveOutHandle handle) {
    if (handle && handle->stream) {
        SDL_ClearAudioStream(handle->stream);
        handle->frames = 0;
    }
}
void NNS_SndWaveOutWaitForChannelStop(NNSSndWaveOutHandle handle) {
    NNS_SndWaveOutStop(handle);
}
void NNS_SndWaveOutSetVolume(NNSSndWaveOutHandle handle, int volume) {
    if (handle && handle->stream)
        SDL_SetAudioStreamGain(handle->stream, SDL_clamp(volume, 0, 127) / 127.0f);
}
void NNS_SndWaveOutSetSpeed(NNSSndWaveOutHandle handle, int speed) {
    if (handle && handle->stream)
        SDL_SetAudioStreamFrequencyRatio(handle->stream, speed / 32768.0f);
}
