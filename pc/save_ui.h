#ifndef POKEBW2_PC_SAVE_UI_H
#define POKEBW2_PC_SAVE_UI_H

#include "video.h"

bool PcSaveUi_IsActive(void);
bool PcSaveUi_Draw(void);
bool PcSaveUi_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcSaveUi_FilterKeys(Uint16 keys);

#endif
