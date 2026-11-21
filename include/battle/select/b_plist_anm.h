#ifndef POKEBW2_BATTLE_SELECT_B_PLIST_ANM_H
#define POKEBW2_BATTLE_SELECT_B_PLIST_ANM_H

#include "types.h"
#include "struct_decls.h"

void BPlistAnm_CutButtonScrn(BPlistWork *work, const void *scrn);
void BPlistAnm_CutPageScrn(BPlistWork *work, const void *scrn);
void BPlistAnm_StartButtonAnm(BPlistWork *work, u8 button);
void BPlistAnm_MainButtonAnm(BPlistWork *work);
void BPlistAnm_PutPageButtons(BPlistWork *work, u8 page);
void BPlistAnm_RestorePalette(BPlistWork *work, u8 page);
void BPlistAnm_PutReturnButton(BPlistWork *work);

#endif // POKEBW2_BATTLE_B_PLIST_ANM_H
