#ifndef POKEBW2_APP_MUSICAL_MUS_ITEM_DATA_H
#define POKEBW2_APP_MUSICAL_MUS_ITEM_DATA_H

// Overlay 210's mus_item_data.c (named after its string): the table of the musical's props, file 101 of
// ARCID_MUSICAL_ITEM, with each prop's texture size, offset, the positions it can be worn at and its effect

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The active prop record categories, named by the category text; the two face records share a displayed name
enum {
    MUS_ITEM_CATEGORY_EAR = 0,
    MUS_ITEM_CATEGORY_HEAD = 1,
    MUS_ITEM_CATEGORY_FACE = 2,
    MUS_ITEM_CATEGORY_FACE_2 = 3,
    MUS_ITEM_CATEGORY_BODY = 4,
    MUS_ITEM_CATEGORY_WAIST = 5,
    MUS_ITEM_CATEGORY_ARM = 6,
};

// Combinable equip flags and numeric depth/marker-flip choices
#define MUS_ITEM_FLAG_EQUIP_EAR 0x1
#define MUS_ITEM_FLAG_EQUIP_HEAD 0x2
#define MUS_ITEM_FLAG_EQUIP_FACE 0x4
#define MUS_ITEM_FLAG_EQUIP_FACE_2 0x8
#define MUS_ITEM_FLAG_EQUIP_BODY 0x10
#define MUS_ITEM_FLAG_EQUIP_WAIST 0x20
#define MUS_ITEM_FLAG_EQUIP_ARM 0x40
#define MUS_ITEM_FLAG_LOWER_DRAW_DEPTH 0x80
#define MUS_ITEM_FLAG_MARKER_FLIP 0x200

// The kinds of prop, which the category messages are named after; the second of the ears and of the arms is its own
enum {
    DUP_FITTING_CATEGORY_EAR,
    DUP_FITTING_CATEGORY_EAR_2,
    DUP_FITTING_CATEGORY_HEAD,
    DUP_FITTING_CATEGORY_FACE,
    DUP_FITTING_CATEGORY_BODY,
    DUP_FITTING_CATEGORY_WAIST,
    DUP_FITTING_CATEGORY_ARM,
    DUP_FITTING_CATEGORY_ARM_2,
};

// The category that shows the props in the order they are kept in
#define DUP_FITTING_CATEGORY_ORIGINAL 8

// The kinds of prop, which the program gives points for and which each bring their own audience: the four words of
// musical_info_words.txt that MusicalInfoWords_Text_Cool starts, and none, which a show with no points of a kind has
enum {
    MUSICAL_PROP_KIND_COOL,
    MUSICAL_PROP_KIND_CUTE,
    MUSICAL_PROP_KIND_ELEGANT,
    MUSICAL_PROP_KIND_UNIQUE,
    MUSICAL_PROP_KIND_NONE = 4,
};

// A prop's entry in the table, 12 bytes
typedef struct MusicalItemData {
    // The BillboardActor size of its texture
    u32 texSize;
    // Its offset from where it is worn
    s8 offsetX;
    s8 offsetY;
    // The positions it can be worn at
    u16 flags;
    // The kind of position it is worn at
    u8 category;
    // The kind of prop it is, which the program gives points for
    u8 kind;
    // The kind of its effect when it is used
    u8 effect;
} MusicalItemData;

// The loaded table
typedef struct {
    void *data;
    MusicalItemData *items;
} MusItemData;

MusItemData *MusItemData_Init(HeapID heapId);
void MusItemData_Free(MusItemData *items);
MusicalItemData *MusItemData_GetItem(MusItemData *items, u16 itemId);
// The prop's offset from where it is worn, x then y
void MusItemData_GetOffset(MusicalItemData *item, s32 *offset);
// The BillboardActor size of its texture
u32 MusItemData_GetTexSize(MusicalItemData *item);
// Whether the prop can be worn at a position, by its flags
BOOL MusItemData_CanEquipAtPos(MusicalItemData *item, s32 pos);
BOOL MusItemData_CanEquipInGroup(MusicalItemData *item, s32 group);
// Whether the prop record category matches a conflict/sort group
BOOL MusItemData_IsCategoryOfGroup(MusicalItemData *item, s32 group);
// Whether it selects the lower numeric draw depth or follows a mirrored marker
BOOL MusItemData_UsesLowerDrawDepth(MusicalItemData *item);
BOOL MusItemData_UsesMarkerFlip(MusicalItemData *item);
s32 MusItemData_GetPosGroup(s32 pos);
u32 MusItemData_GetKind(MusItemData *items, u16 itemId);
// The kind of a prop's effect when it is used
u8 MusItemData_GetEffect(MusItemData *items, u16 itemId);
u8 MusItemData_GetCategory(MusItemData *items, u16 itemId);

#endif // POKEBW2_APP_MUSICAL_MUS_ITEM_DATA_H
