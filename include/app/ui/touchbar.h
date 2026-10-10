#ifndef POKEBW2_APP_UI_TOUCHBAR_H
#define POKEBW2_APP_UI_TOUCHBAR_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// touchbar.c: The bar of icons at the bottom of the lower screen, such as the return button
typedef struct Ov139TouchBar Ov139TouchBar;

// An icon of the bar: one of the bar's own, or from OV139_TOUCHBAR_ICON_CUSTOM on, one drawn from the caller's
// resources and animations. Pressing the key does what touching it does
#define OV139_TOUCHBAR_ICON_CUSTOM 7

typedef struct {
    u32 icon;
    ClActorPos pos;
    u16 charRes;
    u16 plttRes;
    u16 cellRes;
    u16 anims[3];
    u32 unk14;
    u32 key;
    u32 se;
} Ov139TouchBarItem;

typedef struct {
    Ov139TouchBarItem *items;
    u32 count;
    ClActUnit *unit;
    u32 bg;
    u32 bgPalette;
    u32 objPalette;
    u32 vramType;
    BOOL unk1C;
} Ov139TouchBarSetup;

Ov139TouchBar *func_ov139_02199aa0(const Ov139TouchBarSetup *setup, HeapID heapId);
void func_ov139_02199b5c(Ov139TouchBar *bar);
void func_ov139_02199b90(Ov139TouchBar *bar);
// Whether the touched icon's animation has ended, or -1 when none was touched
u32 func_ov139_02199c08(Ov139TouchBar *bar);
// Whether the return icon was touched
BOOL func_ov139_02199c30(Ov139TouchBar *bar);
// Whether the icons can be touched
void func_ov139_02199c90(Ov139TouchBar *bar, BOOL active);
void func_ov139_02199ce0(Ov139TouchBar *bar, u32 a1);
void func_ov139_02199d08(Ov139TouchBar *bar, u32 icon, BOOL a2);
void func_ov139_02199d18(Ov139TouchBar *bar, u32 icon, BOOL a2);
void func_ov139_02199d48(Ov139TouchBar *bar, u32 icon, BOOL a2);
void func_ov139_02199d74(Ov139TouchBar *bar, u32 a1);

#endif // POKEBW2_APP_UI_TOUCHBAR_H
