#ifndef POKEBW2_BATTLE_SELECT_B_BAG_ITEM_H
#define POKEBW2_BATTLE_SELECT_B_BAG_ITEM_H

#include "types.h"
#include "struct_decls.h"

BOOL BBagItem_CheckLastItem(BBagWork *work);
void BBagItem_SetCursorToLastItem(BBagWork *work);
void BBagItem_MakePocketLists(BBagWork *work);
void BBagItem_MakeShooterList(BBagWork *work);
void BBagItem_MakeDemoList(BBagWork *work);
u16 BBagItem_GetSlotItem(BBagWork *work, int slot);
u16 BBagItem_GetShooterCost(u16 item);

#endif // POKEBW2_BATTLE_B_BAG_ITEM_H
