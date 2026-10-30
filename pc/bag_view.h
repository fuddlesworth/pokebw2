#ifndef POKEBW2_PC_BAG_VIEW_H
#define POKEBW2_PC_BAG_VIEW_H
#include "gfl/msg.h"
#include "ui_components.h"

typedef struct {
    u32 item;
    bool valid;
    u16 description[512];
} PcBagViewCache;
typedef struct {
    u32 item, count;
    const char *label;
    int value;
    bool selected, enabled;
} PcBagViewRow;
typedef struct {
    Font *font;
    MsgData *itemNames;
    const char *section, *hint, *heading;
    const char *const *pockets;
    u32 pocketCount, selectedPocket;
    bool tabsEnabled, showQuantity;
    PcUiActionKind rowAction;
    PcBagViewRow rows[6];
    u32 rowCount, selectedItem, selectedCount;
    bool pocketList;
} PcBagView;
void PcBagView_Draw(const PcBagView *view, PcBagViewCache *cache);
#endif
