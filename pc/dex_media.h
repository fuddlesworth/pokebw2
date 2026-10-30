#ifndef POKEBW2_PC_DEX_MEDIA_H
#define POKEBW2_PC_DEX_MEDIA_H
#include "types.h"
#include <stdbool.h>

void PcDexMedia_Free(void);
void PcDexMedia_Update(void);
void PcDexMedia_SetPokemon(unsigned slot, u16 species, u32 form, u32 sex, u32 rare, u32 personality, bool egg,
                           bool back);
void PcDexMedia_Play(bool play);
bool PcDexMedia_DrawPokemon(unsigned slot, float center, float floor, float width, float height);
const u16 *PcDexMedia_Map(void);
void PcDexMedia_HighlightArea(u16 *pixels, u16 animation, int left, int top, u32 color);

#endif
