#ifndef POKEBW2_PC_AUDIO_H
#define POKEBW2_PC_AUDIO_H
#include <stdbool.h>
#include <stdint.h>
int16_t PcAudio_DecodeSample(int nibble, int *sample, int *index);
bool PcAudio_Init(const char *root);
void PcAudio_Update(void);
void PcAudio_Shutdown(void);
#endif
