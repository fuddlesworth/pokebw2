#ifndef POKEBW2_PC_TRAINER_UI_H
#define POKEBW2_PC_TRAINER_UI_H

#include "video.h"

bool PcTrainerUi_IsActive(void);
bool PcTrainerUi_Draw(void);
bool PcTrainerUi_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcTrainerUi_FilterKeys(Uint16 keys);

#endif
