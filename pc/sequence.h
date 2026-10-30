#ifndef POKEBW2_PC_SEQUENCE_H
#define POKEBW2_PC_SEQUENCE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t *data;
    size_t size;
} PcSoundResource;
void PcSequence_Init(void);
void PcSequence_Shutdown(void);
bool PcSequence_Start(unsigned player, PcSoundResource sequence, PcSoundResource bank, const PcSoundResource waves[4],
                      unsigned volume, unsigned priority);
void PcSequence_Stop(unsigned player);
void PcSequence_Pause(unsigned player, bool paused);
bool PcSequence_Active(unsigned player);
uint32_t PcSequence_Tick(unsigned player);
void PcSequence_Volume(unsigned player, unsigned volume);
void PcSequence_Params(unsigned player, int tempo, int pitch, int pan);
void PcSequence_TrackParams(unsigned player, unsigned mask, int tempo, int pitch, int pan, int volume);
void PcSequence_ChannelMask(unsigned player, unsigned mask);
void PcSequence_Modulation(unsigned player, unsigned mask, int depth, int speed);
bool PcSequence_ReplaceWave(PcSoundResource archive, unsigned index, PcSoundResource source, unsigned source_index);
void PcSequence_ClearWaveReplacements(void);
void PcSequence_Render(float *stereo, size_t frames);
#endif
