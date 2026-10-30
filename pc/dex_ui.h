#ifndef POKEBW2_PC_DEX_UI_H
#define POKEBW2_PC_DEX_UI_H

#include "video.h"

typedef enum { PC_DEX_ENTRY, PC_DEX_AREA, PC_DEX_FORMS, PC_DEX_TAB_COUNT } PcDexTab;

void PcDexUi_Draw(void);
bool PcDexUi_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcDexUi_FilterKeys(Uint16 keys);
bool PcDexUi_IsActive(void);

#endif
