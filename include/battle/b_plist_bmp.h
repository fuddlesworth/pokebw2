#ifndef POKEBW2_BATTLE_B_PLIST_BMP_H
#define POKEBW2_BATTLE_B_PLIST_BMP_H

#include "types.h"
#include "struct_decls.h"

void BPlistBmp_Init(BPlistWork *work);
void BPlistBmp_CreatePageWindows(BPlistWork *work, u8 page);
void BPlistBmp_FreePageWindows(BPlistWork *work);
void BPlistBmp_Exit(BPlistWork *work);
void BPlistBmp_DrawPage(BPlistWork *work, u8 page);
void BPlistBmp_PrintChoosePokemonPrompt(BPlistWork *work);
void BPlistBmp_PrintChooseSwitchPositionPrompt(BPlistWork *work);
void BPlistBmp_PrintMessage(BPlistWork *work);
void BPlistBmp_OpenMessage(BPlistWork *work);
void BPlistBmp_SetEmbargoMessage(BPlistWork *work);
void BPlistBmp_FlushWindows(BPlistWork *work);
void BPlistBmp_TransferPage(BPlistWork *work);
void BPlistBmp_DrawInfoFrame(BPlistWork *work);

#endif // POKEBW2_BATTLE_B_PLIST_BMP_H
