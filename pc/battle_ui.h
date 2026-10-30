#ifndef POKEBW2_PC_BATTLE_UI_H
#define POKEBW2_PC_BATTLE_UI_H

#include "video.h"

bool PcBattleUi_IsActive(void);
bool PcBattleUi_Draw(void);
bool PcBattleUi_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcBattleUi_FilterKeys(Uint16 keys);

#endif
