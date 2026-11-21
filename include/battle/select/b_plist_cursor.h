#ifndef POKEBW2_BATTLE_SELECT_B_PLIST_CURSOR_H
#define POKEBW2_BATTLE_SELECT_B_PLIST_CURSOR_H

#include "types.h"
#include "struct_decls.h"

// The battle party list's key cursor (b_plist_cursor.c, our name)

void BPlistCursor_Create(BPlistWork *work, u8 page, int pos);
void BPlistCursor_Delete(BPlistWork *work);
void BPlistCursor_ChangePage(BPlistWork *work, u8 page, int pos);

#endif // POKEBW2_BATTLE_B_PLIST_CURSOR_H
