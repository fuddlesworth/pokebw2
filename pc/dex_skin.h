#ifndef POKEBW2_PC_DEX_SKIN_H
#define POKEBW2_PC_DEX_SKIN_H

#include "video.h"

bool PcDexSkin_Init(PcVideo *video);
void PcPartySkin_Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport, Uint32 selected,
                      Uint32 marked, Uint32 menuRow, Uint32 occupied, Uint32 sex, Uint32 hpRows, bool overview,
                      bool overviewData);
void PcDexSkin_Destroy(PcVideo *video);
void PcDexSkin_Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport, Uint32 selectedRow,
                    Uint32 tab, Uint32 filterRow);

#endif
