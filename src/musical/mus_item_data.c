#include "types.h"
#include "app/musical/mus_item_data.h"
#include "app/musical/mus_poke_draw.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"

// Overlay 210's mus_item_data.c, named after its string: the table of the musical's props

// The file of ARCID_MUSICAL_ITEM that holds the table, after the props' textures
#define MUS_ITEM_DATA_FILE 101

MusItemData *MusItemData_Init(HeapID heapId) {
    MusItemData *items = GFL_HeapAllocate(heapId, sizeof(MusItemData), FALSE, "mus_item_data.c", 71);

    items->data = GFL_ArcSysReadHeapNewLZ(ARCID_MUSICAL_ITEM, MUS_ITEM_DATA_FILE, FALSE, heapId);
    items->items = items->data;
    return items;
}

void MusItemData_Free(MusItemData *items) {
    GFL_HeapFree(items->data);
    GFL_HeapFree(items);
}

MusicalItemData *MusItemData_GetItem(MusItemData *items, u16 itemId) {
    return &items->items[itemId];
}

void MusItemData_GetOffset(MusicalItemData *item, s32 *offset) {
    offset[0] = item->offsetX;
    offset[1] = item->offsetY;
}

u32 MusItemData_GetTexSize(MusicalItemData *item) {
    return item->texSize;
}

BOOL MusItemData_CanEquipAtPos(MusicalItemData *item, s32 pos) {
    switch (pos) {
    case MUS_POKE_DRAW_EQUIP_EAR:
    case MUS_POKE_DRAW_EQUIP_EAR_2:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_EAR) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_HEAD:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_HEAD) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_FACE:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_FACE) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_FACE_2:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_FACE_2) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_BODY:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_BODY) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_WAIST:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_WAIST) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_HAND_R:
    case MUS_POKE_DRAW_EQUIP_HAND_L:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_ARM) {
            return TRUE;
        }
        break;
    case MUS_POKE_DRAW_EQUIP_MAX:
        return FALSE;
    }
    return FALSE;
}

BOOL MusItemData_CanEquipInGroup(MusicalItemData *item, s32 group) {
    switch (group) {
    case DUP_FITTING_CATEGORY_EAR:
    case DUP_FITTING_CATEGORY_EAR_2:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_EAR) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_HEAD:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_HEAD) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_FACE:
        if (item->flags & (MUS_ITEM_FLAG_EQUIP_FACE | MUS_ITEM_FLAG_EQUIP_FACE_2)) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_BODY:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_BODY) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_WAIST:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_WAIST) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_ARM:
    case DUP_FITTING_CATEGORY_ARM_2:
        if (item->flags & MUS_ITEM_FLAG_EQUIP_ARM) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_ORIGINAL:
        return FALSE;
    }
    return FALSE;
}

BOOL MusItemData_IsCategoryOfGroup(MusicalItemData *item, s32 group) {
    switch (group) {
    case DUP_FITTING_CATEGORY_EAR:
    case DUP_FITTING_CATEGORY_EAR_2:
        if (item->category == MUS_ITEM_CATEGORY_EAR) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_HEAD:
        if (item->category == MUS_ITEM_CATEGORY_HEAD) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_FACE:
        if (item->category == MUS_ITEM_CATEGORY_FACE || item->category == MUS_ITEM_CATEGORY_FACE_2) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_BODY:
        if (item->category == MUS_ITEM_CATEGORY_BODY) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_WAIST:
        if (item->category == MUS_ITEM_CATEGORY_WAIST) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_ARM:
    case DUP_FITTING_CATEGORY_ARM_2:
        if (item->category == MUS_ITEM_CATEGORY_ARM) {
            return TRUE;
        }
        break;
    case DUP_FITTING_CATEGORY_ORIGINAL:
        return FALSE;
    }
    return FALSE;
}

BOOL MusItemData_UsesLowerDrawDepth(MusicalItemData *item) {
    if (item->flags & MUS_ITEM_FLAG_LOWER_DRAW_DEPTH) {
        return TRUE;
    }
    return FALSE;
}

BOOL MusItemData_UsesMarkerFlip(MusicalItemData *item) {
    if (item->flags & MUS_ITEM_FLAG_MARKER_FLIP) {
        return TRUE;
    }
    return FALSE;
}

s32 MusItemData_GetPosGroup(s32 pos) {
    switch (pos) {
    case MUS_POKE_DRAW_EQUIP_EAR:
        return DUP_FITTING_CATEGORY_EAR;
    case MUS_POKE_DRAW_EQUIP_EAR_2:
        return DUP_FITTING_CATEGORY_EAR_2;
    case MUS_POKE_DRAW_EQUIP_HEAD:
        return DUP_FITTING_CATEGORY_HEAD;
    case MUS_POKE_DRAW_EQUIP_FACE:
    case MUS_POKE_DRAW_EQUIP_FACE_2:
        return DUP_FITTING_CATEGORY_FACE;
    case MUS_POKE_DRAW_EQUIP_BODY:
        return DUP_FITTING_CATEGORY_BODY;
    case MUS_POKE_DRAW_EQUIP_WAIST:
        return DUP_FITTING_CATEGORY_WAIST;
    case MUS_POKE_DRAW_EQUIP_HAND_R:
        return DUP_FITTING_CATEGORY_ARM;
    case MUS_POKE_DRAW_EQUIP_HAND_L:
        return DUP_FITTING_CATEGORY_ARM_2;
    case MUS_POKE_DRAW_EQUIP_MAX:
        return DUP_FITTING_CATEGORY_ORIGINAL;
    }
    return 9;
}

u32 MusItemData_GetKind(MusItemData *items, u16 itemId) {
    return MusItemData_GetItem(items, itemId)->kind;
}

u8 MusItemData_GetEffect(MusItemData *items, u16 itemId) {
    return MusItemData_GetItem(items, itemId)->effect;
}

u8 MusItemData_GetCategory(MusItemData *items, u16 itemId) {
    return MusItemData_GetItem(items, itemId)->category;
}
