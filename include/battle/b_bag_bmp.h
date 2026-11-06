#ifndef POKEBW2_BATTLE_B_BAG_BMP_H
#define POKEBW2_BATTLE_B_BAG_BMP_H

#include "types.h"
#include "struct_decls.h"

// The two template messages for each slot
enum {
    BBAG_SLOT_MESSAGE_NAME,
    BBAG_SLOT_MESSAGE_COUNT,
};

#define BBAG_WINDOW_LIST_END 0xff

void BBagBmp_Init(BBagWork *work);
void BBagBmp_Exit(BBagWork *work);
void BBagBmp_DrawPage(BBagWork *work, u8 page);
void BBagBmp_DrawSlots(BBagWork *work);
void BBagBmp_PrintPageNumber(BBagWork *work);
void BBagBmp_OpenMessage(BBagWork *work);
void BBagBmp_FlushWindows(BBagWork *work);
void BBagBmp_TransferPage(BBagWork *work);

#endif // POKEBW2_BATTLE_B_BAG_BMP_H
