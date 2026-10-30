#include "sequence.h"
#include "audio.h"
#include <SDL3/SDL.h>
#include <nitro/snd/common/bank.h>
#include <nitro/snd/common/util.h>
#include <nitro/snd/common/work.h>

// The interpreter owns notes, tempo, ADSR, modulation and track control. This backend supplies sample channels.
SNDWork SNDi_Work;
static SNDSharedWork shared;
SNDSharedWork *SNDi_SharedWork = &shared;
typedef struct {
    SNDBankData header;
    PcSoundResource file, waves[4];
} PcSequenceBank;
static PcSequenceBank banks[SND_PLAYER_NUM];
typedef struct {
    float *samples;
    size_t count, loopStart;
    double position, step;
    float gain, pan;
    bool active, loop;
} PcSoundChannel;
static PcSoundChannel channels[SND_CHANNEL_NUM];
static double tickPhase;
typedef struct PcWaveReplacement {
    struct PcWaveReplacement *next;
    const uint8_t *archive;
    unsigned index;
    PcSoundResource wave;
} PcWaveReplacement;
static PcWaveReplacement *wave_replacements;

void PcSequence_ClearWaveReplacements(void) {
    while (wave_replacements) {
        PcWaveReplacement *next = wave_replacements->next;
        SDL_free(wave_replacements);
        wave_replacements = next;
    }
}

uint32_t PcSequence_Tick(unsigned player) {
    return player < SND_PLAYER_NUM ? shared.player[player].tickCounter : 0;
}

static uint16_t Read16(const uint8_t *p) {
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static uint32_t Read32(const uint8_t *p) {
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static bool Contains(PcSoundResource resource, size_t offset, size_t size) {
    return resource.data && offset <= resource.size && size <= resource.size - offset;
}
bool PcSequence_ReplaceWave(PcSoundResource archive, unsigned index, PcSoundResource source, unsigned source_index) {
    if (!Contains(archive, 56, 4) || index >= Read32(archive.data + 56) || !Contains(source, 56, 4) ||
        source_index >= Read32(source.data + 56) || !Contains(source, 60 + (size_t)source_index * 4, 4))
        return false;
    size_t offset = Read32(source.data + 60 + source_index * 4);
    if (!Contains(source, offset, 12))
        return false;
    PcWaveReplacement *replacement = wave_replacements;
    while (replacement && (replacement->archive != archive.data || replacement->index != index))
        replacement = replacement->next;
    if (!replacement) {
        replacement = SDL_malloc(sizeof(*replacement));
        if (!replacement)
            return false;
        *replacement = (PcWaveReplacement){ wave_replacements, archive.data, index, { 0 } };
        wave_replacements = replacement;
    }
    replacement->wave = (PcSoundResource){ source.data + offset, source.size - offset };
    return true;
}

BOOL SND_ReadInstData(const SNDBankData *header, int program, int key, SNDInstData *instrument) {
    const PcSequenceBank *bank = (const PcSequenceBank *)header;
    PcSoundResource file = bank->file;
    if (program < 0 || key < 0 || key > 127 || !Contains(file, 56, 4) || (uint32_t)program >= Read32(file.data + 56) ||
        !Contains(file, 60 + (size_t)program * 4, 4))
        return FALSE;
    uint32_t entry = Read32(file.data + 60 + program * 4);
    unsigned type = entry & 255;
    size_t offset = entry >> 8;
    if (type == SND_INST_DRUM_SET) {
        if (!Contains(file, offset, 2) || key < file.data[offset] || key > file.data[offset + 1])
            return FALSE;
        offset += 2 + (size_t)(key - file.data[offset]) * 12;
        if (!Contains(file, offset, 12))
            return FALSE;
        type = file.data[offset];
        offset += 2;
    } else if (type == SND_INST_KEY_SPLIT) {
        if (!Contains(file, offset, 8))
            return FALSE;
        unsigned region = 0;
        while (region < 8 && key > file.data[offset + region])
            region++;
        if (region == 8 || !Contains(file, offset + 8 + region * 12, 12))
            return FALSE;
        offset += 8 + region * 12;
        type = file.data[offset];
        offset += 2;
    }
    if (type < SND_INST_PCM || type > SND_INST_NOISE || !Contains(file, offset, 10))
        return FALSE;
    const uint8_t *p = file.data + offset;
    instrument->type = (u8)type;
    instrument->param = (SNDInstParam){ { Read16(p), Read16(p + 2) }, p[4], p[5], p[6], p[7], p[8], p[9] };
    return TRUE;
}

BOOL SND_NoteOn(SNDExChannel *channel, int key, int velocity, s32 length, const SNDBankData *header,
                const SNDInstData *instrument) {
    const PcSequenceBank *bank = (const PcSequenceBank *)header;
    const SNDInstParam *param = &instrument->param;
    int release = param->release;
    if (release == 255) {
        length = -1;
        release = 0;
    }
    BOOL started = FALSE;
    if (instrument->type == SND_INST_PCM && param->wave[1] < 4) {
        PcSoundResource wave = bank->waves[param->wave[1]];
        size_t index = param->wave[0];
        if (!Contains(wave, 56, 4) || index >= Read32(wave.data + 56) || !Contains(wave, 60 + index * 4, 4))
            return FALSE;
        size_t offset = Read32(wave.data + 60 + index * 4);
        for (PcWaveReplacement *replacement = wave_replacements; replacement; replacement = replacement->next) {
            if (replacement->archive == wave.data && replacement->index == index) {
                wave = replacement->wave;
                offset = 0;
                break;
            }
        }
        if (!Contains(wave, offset, 12))
            return FALSE;
        SNDWaveParam shape;
        SDL_memcpy(&shape, wave.data + offset, sizeof(shape));
        size_t bytes = ((size_t)shape.loopstart + shape.looplen) * 4;
        if (!Contains(wave, offset + 12, bytes) || shape.format > 2 || (shape.format == 2 && bytes < 4))
            return FALSE;
        started = SND_StartExChannelPcm(channel, &shape, wave.data + offset + 12, length);
    } else if (instrument->type == SND_INST_PSG) {
        started = SND_StartExChannelPsg(channel, (SNDDuty)param->wave[0], length);
    } else if (instrument->type == SND_INST_NOISE) {
        started = SND_StartExChannelNoise(channel, length);
    }
    if (!started)
        return FALSE;
    channel->key = (u8)key;
    channel->original_key = param->original_key;
    channel->velocity = (u8)velocity;
    channel->init_pan = (s8)(param->pan - 64);
    SND_SetExChannelAttack(channel, param->attack);
    SND_SetExChannelDecay(channel, param->decay);
    SND_SetExChannelSustain(channel, param->sustain);
    SND_SetExChannelRelease(channel, release);
    return TRUE;
}

void SND_SetChannelVolume7(int channel, int volume, SNDChannelDataShift shift) {
    static const unsigned divisors[] = { 1, 2, 4, 16 };
    channels[channel].gain = (float)SDL_clamp(volume, 0, 127) / (127 * divisors[(unsigned)shift & 3]);
}
void SND_SetChannelPan7(int channel, int pan) {
    channels[channel].pan = (float)SDL_clamp(pan, 0, 127) / 127;
}
void SND_SetChannelTimer7(int channel, int timer) {
    channels[channel].step = 16756991.0 / (SDL_max(timer, 16) * 48000.0);
}
void SND_StartChannel7(int channel) {
    channels[channel].active = channels[channel].samples && channels[channel].count;
}
void SND_StopChannel7(int channel, s32 flags) {
    (void)flags;
    channels[channel].active = false;
}
BOOL SND_IsChannelActive7(int channel) {
    return channels[channel].active;
}
static bool AllocateChannel(int index, size_t count, int volume, SNDChannelDataShift shift, int timer, int pan) {
    PcSoundChannel *channel = &channels[index];
    SDL_free(channel->samples);
    SDL_zero(*channel);
    channel->samples = SDL_calloc(count, sizeof(float));
    if (!channel->samples)
        return false;
    channel->count = count;
    SND_SetChannelVolume7(index, volume, shift);
    SND_SetChannelTimer7(index, timer);
    SND_SetChannelPan7(index, pan);
    return true;
}
void SND_SetupChannelPcm7(int index, const void *data, SNDWaveFormat format, SNDChannelLoop loop, int loopStart,
                          int loopLength, int volume, SNDChannelDataShift shift, int timer, int pan) {
    const uint8_t *bytes = data;
    size_t size = ((size_t)loopStart + loopLength) * 4;
    size_t count = format == 0 ? size : format == 1 ? size / 2 : (size - 4) * 2;
    if (!AllocateChannel(index, count, volume, shift, timer, pan))
        return;
    PcSoundChannel *channel = &channels[index];
    channel->loop = loop == SND_CHANNEL_LOOP_REPEAT;
    channel->loopStart = format == 0 ? loopStart * 4 : format == 1 ? loopStart * 2 : SDL_max(loopStart * 8 - 8, 0);
    int sample = format == 2 ? (int16_t)Read16(bytes) : 0;
    int predictor = format == 2 ? SDL_clamp(bytes[2] & 127, 0, 88) : 0;
    for (size_t i = 0; i < count; i++) {
        int value = format == 0   ? (int8_t)bytes[i] * 256
                    : format == 1 ? (int16_t)Read16(bytes + i * 2)
                                  : PcAudio_DecodeSample((bytes[4 + i / 2] >> ((i & 1) * 4)) & 15, &sample, &predictor);
        channel->samples[i] = (float)value / 32768;
    }
}
void SND_SetupChannelPsg7(int index, SNDDuty duty, int volume, SNDChannelDataShift shift, int timer, int pan) {
    if (!AllocateChannel(index, 8, volume, shift, timer, pan))
        return;
    channels[index].loop = true;
    for (unsigned i = 0; i < 8; i++)
        channels[index].samples[i] = i <= (unsigned)duty ? 1 : -1;
}
void SND_SetupChannelNoise7(int index, int volume, SNDChannelDataShift shift, int timer, int pan) {
    if (!AllocateChannel(index, 32767, volume, shift, timer, pan))
        return;
    channels[index].loop = true;
    unsigned noise = 0x7fff;
    for (unsigned i = 0; i < 32767; i++) {
        channels[index].samples[i] = noise & 1 ? -1 : 1;
        noise = (noise >> 1) | (((noise ^ (noise >> 1)) & 1) << 14);
    }
}

void PcSequence_Init(void) {
    SDL_zero(SNDi_Work);
    SDL_zero(shared);
    for (unsigned p = 0; p < SND_PLAYER_NUM; p++)
        for (unsigned v = 0; v < SND_PLAYER_VARIABLE_NUM; v++)
            shared.player[p].variable[v] = -1;
    for (unsigned v = 0; v < SND_GLOBAL_VARIABLE_NUM; v++)
        shared.globalVariable[v] = -1;
    SND_SeqInit();
    SND_ExChannelInit();
    tickPhase = 0;
}
void PcSequence_Stop(unsigned player) {
    if (player < SND_PLAYER_NUM)
        SND_StopSeq7(player);
}
void PcSequence_Pause(unsigned player, bool paused) {
    if (player < SND_PLAYER_NUM)
        SND_PauseSeq7(player, paused);
}
void PcSequence_Shutdown(void) {
    PcSequence_ClearWaveReplacements();
    for (unsigned player = 0; player < SND_PLAYER_NUM; player++)
        PcSequence_Stop(player);
    for (unsigned i = 0; i < SND_CHANNEL_NUM; i++) {
        SDL_free(channels[i].samples);
        SDL_zero(channels[i]);
    }
}
bool PcSequence_Start(unsigned player, PcSoundResource sequence, PcSoundResource bank, const PcSoundResource waves[4],
                      unsigned volume, unsigned priority) {
    if (player >= SND_PLAYER_NUM || !Contains(sequence, 0, 28) || SDL_memcmp(sequence.data, "SSEQ", 4) ||
        !Contains(bank, 0, 60) || SDL_memcmp(bank.data, "SBNK", 4))
        return false;
    size_t offset = Read32(sequence.data + 24);
    if (offset >= sequence.size)
        return false;
    PcSequence_Stop(player);
    banks[player].file = bank;
    SDL_memcpy(banks[player].waves, waves, sizeof(banks[player].waves));
    SND_StartSeq7(player, sequence.data + offset, 0, &banks[player].header);
    SNDi_Work.player[player].volume = (u8)volume;
    SNDi_Work.player[player].prio = (u8)priority;
    return true;
}
bool PcSequence_Active(unsigned player) {
    return player < SND_PLAYER_NUM && SNDi_Work.player[player].active_flag;
}
void PcSequence_ChannelMask(unsigned player, unsigned mask) {
    if (player < SND_PLAYER_NUM) {
        SND_SetTrackMute7(player, 0xffff, SND_SEQ_MUTE_OFF);
        SND_SetTrackMute7(player, mask ^ 0xffff, SND_SEQ_MUTE_NO_STOP);
    }
}
void PcSequence_Volume(unsigned player, unsigned volume) {
    if (player < SND_PLAYER_NUM)
        SNDi_Work.player[player].volume = (u8)SDL_min(volume, 127);
}
void PcSequence_Modulation(unsigned player, unsigned mask, int depth, int speed) {
    if (player >= SND_PLAYER_NUM)
        return;
    SNDPlayer *p = &SNDi_Work.player[player];
    for (unsigned i = 0; i < SND_TRACK_NUM_PER_PLAYER; i++) {
        if (!(mask & (1u << i)) || p->tracks[i] == SND_INVALID_TRACK_INDEX)
            continue;
        SNDTrack *track = &SNDi_Work.track[p->tracks[i]];
        if (depth >= 0)
            track->mod.depth = (u8)SDL_min(depth, 127);
        if (speed >= 0)
            track->mod.speed = (u8)SDL_min(speed, 127);
    }
}
void PcSequence_Params(unsigned player, int tempo, int pitch, int pan) {
    PcSequence_TrackParams(player, 0xffff, tempo, pitch, pan, -1);
}
void PcSequence_TrackParams(unsigned player, unsigned mask, int tempo, int pitch, int pan, int volume) {
    if (player >= SND_PLAYER_NUM)
        return;
    SNDPlayer *p = &SNDi_Work.player[player];
    if (tempo >= 0)
        p->tempo_ratio = (u16)tempo;
    for (unsigned i = 0; i < SND_TRACK_NUM_PER_PLAYER; i++) {
        if (!(mask & (1u << i)) || p->tracks[i] == SND_INVALID_TRACK_INDEX)
            continue;
        SNDTrack *track = &SNDi_Work.track[p->tracks[i]];
        if (volume >= 0)
            track->extFader = SND_CalcDecibel(SDL_min(volume, 127));
        if (pitch != -1)
            track->ext_pitch = (s16)pitch;
        if (pan != -1)
            track->ext_pan = (s8)SDL_clamp(pan, -64, 63);
    }
}
void PcSequence_Render(float *stereo, size_t frames) {
    for (size_t frame = 0; frame < frames; frame++) {
        if (tickPhase <= 0) {
            SND_SeqMain(TRUE);
            SND_ExChannelMain(TRUE);
            SND_UpdateExChannel();
            tickPhase += 48000.0 * 64 * 2728 / 33513982;
        }
        tickPhase--;
        float left = 0, right = 0;
        for (unsigned index = 0; index < SND_CHANNEL_NUM; index++) {
            PcSoundChannel *channel = &channels[index];
            if (!channel->active)
                continue;
            if (channel->position >= channel->count) {
                if (!channel->loop || channel->loopStart >= channel->count) {
                    channel->active = false;
                    continue;
                }
                channel->position = channel->loopStart + SDL_fmod(channel->position - channel->loopStart,
                                                                  channel->count - channel->loopStart);
            }
            float value = channel->samples[(size_t)channel->position] * channel->gain * 0.25f;
            left += value * (1 - channel->pan);
            right += value * channel->pan;
            channel->position += channel->step;
        }
        stereo[frame * 2] = SDL_clamp(left, -1, 1);
        stereo[frame * 2 + 1] = SDL_clamp(right, -1, 1);
    }
}
