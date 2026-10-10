#ifndef POKEBW2_APP_UI_UI_SCENE_H
#define POKEBW2_APP_UI_UI_SCENE_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"

// Overlay 139 holds the parts of the lower screen that applications share, each its own original file: OBJ
// resources and their actors (ui_scene.c), the bar of icons at the bottom (touchbar.c), printing (print_msg.c), a
// search of message files (msgsearch.c), the menu of two choices (yesno_menu.c) and a scrolling list (frame_list.c).
// Load it with GFL_OvlLoad(OVERLAY_APP_UI) before using any of them
#define OVERLAY_APP_UI OVERLAY_ID(139)

// ui_scene.c: The OBJ resources of a set of files, and where to load them from
typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u32 vramType;
} Ov139ObjRes;

typedef struct {
    u32 vramType;
    // Bit 0 loads the whole palette, bit 1 means the characters are compressed
    u32 flags;
    u32 arcId;
    u32 paletteFile;
    u32 charFile;
    u32 cellFile;
    u32 animFile;
    u8 paletteOffset;
    u8 paletteStart;
    u8 paletteCount;
} Ov139ObjResSetup;

void func_ov139_021999c8(Ov139ObjRes *res, const Ov139ObjResSetup *setup, ClActUnit *unit, HeapID heapId);
void func_ov139_02199a44(Ov139ObjRes *res);
// Creates an actor of the resources at (x, y), playing an animation
ClActor *func_ov139_02199a5c(Ov139ObjRes *res, ClActUnit *unit, u8 x, u8 y, u8 anim, HeapID heapId);

#endif // POKEBW2_APP_UI_UI_SCENE_H
