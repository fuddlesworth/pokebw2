#ifndef POKEBW2_APP_UI_FRAME_LIST_H
#define POKEBW2_APP_UI_FRAME_LIST_H

#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/touchpanel.h"
#include "system/printsys.h"

// frame_list.c: A list of items that scrolls, with a scroll bar and arrows
typedef struct Ov139List Ov139List;

// What func_ov139_0219b2e0 returns besides the position of the item chosen
#define OV139_LIST_NONE (-1)

// A rectangle of the list's rows and buttons
typedef struct {
    TouchRect rect;
    u32 unk4;
} Ov139ListTouch;

// What the list calls to print an item into its row's window at y, when the cursor moves to an item, and when the
// items scroll by delta pixels
typedef struct {
    void (*print)(void *work, u32 index, PrintWindow *window, s16 y);
    void (*select)(void *work, u32 index);
    void (*scroll)(void *work, s16 delta);
} Ov139ListCallbacks;

typedef struct {
    u8 unk0[20];
    u16 count;
    u16 unk16;
    u8 cursorPos;
    u8 unk19;
    u16 scroll;
    const Ov139ListTouch *touch;
    const Ov139ListCallbacks *callbacks;
    void *work;
} Ov139ListSetup;

Ov139List *func_ov139_0219af1c(const Ov139ListSetup *setup, HeapID heapId);
void func_ov139_0219b138(Ov139List *list);
// Adds an item
void func_ov139_0219b1b4(Ov139List *list, u32 type, u32 value);
// Loads the screen of the list's frame, and its palette
void func_ov139_0219b1e0(Ov139List *list, ArcTool *arc, u32 fileId, BOOL compressed, u32 index);
void func_ov139_0219b21c(Ov139List *list, ArcTool *arc, u32 fileId, BOOL compressed, u32 a4, u16 a5, u8 a6);
void func_ov139_0219b27c(Ov139List *list, ArcTool *arc, u32 fileId, u32 palette, u32 count);
// Whether the list is still drawing its items
BOOL func_ov139_0219b294(Ov139List *list);
u32 func_ov139_0219b2e0(Ov139List *list);
// Where the scroll bar goes, from its y
int func_ov139_0219c324(Ov139List *list, int y);
PrintQueue *func_ov139_0219cc18(Ov139List *list);
// An item's value
u32 func_ov139_0219cc1c(Ov139List *list, u32 index);
// The item under the cursor, the cursor's row, and the first row shown
int func_ov139_0219cc28(Ov139List *list);
s16 func_ov139_0219cc34(Ov139List *list);
s16 func_ov139_0219cc3c(Ov139List *list);
// Whether the list can scroll down
BOOL func_ov139_0219cc44(Ov139List *list);
void func_ov139_0219cc58(Ov139List *list, int pos);
void func_ov139_0219cc90(Ov139List *list);
void func_ov139_0219ccb0(Ov139List *list, u32 a1);
void func_ov139_0219ccc8(Ov139List *list, u32 a1);
void func_ov139_0219ccd0(Ov139List *list, int a1);
u32 func_ov139_0219cd0c(Ov139List *list);

#endif // POKEBW2_APP_UI_FRAME_LIST_H
