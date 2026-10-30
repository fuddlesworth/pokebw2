#ifndef POKEBW2_PC_BATTLE_MENU_H
#define POKEBW2_PC_BATTLE_MENU_H

#include "video.h"

bool PcBattleMenu_IsActive(void);
bool PcBattleMenu_Draw(void);
bool PcBattleMenu_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcBattleMenu_FilterKeys(Uint16 keys);

#endif
