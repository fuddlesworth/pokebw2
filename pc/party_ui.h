#ifndef POKEBW2_PC_PARTY_UI_H
#define POKEBW2_PC_PARTY_UI_H

#include "video.h"

typedef enum { PC_PARTY_PARTY, PC_PARTY_OVERVIEW } PcPartyTab;

bool PcPartyUi_IsActive(void);
bool PcPartyUi_Draw(void);
bool PcPartyUi_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcPartyUi_FilterKeys(Uint16 keys);

#endif
