#ifndef POKEBW2_APP_UI_YESNO_MENU_H
#define POKEBW2_APP_UI_YESNO_MENU_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "system/printsys.h"

// yesno_menu.c: the menu of two choices with which the evolution demo asks about learning a move
typedef struct TwoChoiceMenu TwoChoiceMenu;

// What func_ov139_0219ae78 returns
#define TWO_CHOICE_MENU_SECOND 0
#define TWO_CHOICE_MENU_FIRST 1
#define TWO_CHOICE_MENU_NONE 2

TwoChoiceMenu *func_ov139_0219a584(HeapID heapId, u32 a1, u32 a2, u32 a3, u32 a4, ClActUnit *unit, Font *font,
                                   PrintQueue *queue, u32 a8);
void func_ov139_0219a864(TwoChoiceMenu *menu);
// Opens the menu with the two choices
void func_ov139_0219a8bc(TwoChoiceMenu *menu, StrBuf *first, StrBuf *second);
void func_ov139_0219aaa4(TwoChoiceMenu *menu);
void func_ov139_0219ab40(TwoChoiceMenu *menu);
u32 func_ov139_0219ae78(TwoChoiceMenu *menu);

#endif // POKEBW2_APP_UI_YESNO_MENU_H
