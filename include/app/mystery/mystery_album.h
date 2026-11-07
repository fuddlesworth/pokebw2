#ifndef POKEBW2_APP_MYSTERY_MYSTERY_ALBUM_H
#define POKEBW2_APP_MYSTERY_MYSTERY_ALBUM_H

// Mystery Gift's cards: the album of saved cards, and the view of one card (ov197, mystery_album.c). Our names; swan
// has none for this overlay

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "save/mystery_gift.h"
#include "struct_decls.h"
#include "system/printsys.h"

enum {
    MYSTERY_CARD_VIEW_MODE_ALBUM = 0,
    MYSTERY_CARD_VIEW_MODE_MAKE_ROOM = 1,
};

typedef struct {
    // A MYSTERY_CARD_VIEW_MODE_*
    u32 mode;
    u32 unk4;
    ClActUnit *unit;
    MysteryGiftSave *giftSave;
    Font *font;
    PrintQueue *queue;
    WordSet *wordSet;
    MsgData *msgData;
    GameData *gameData;
} MysteryCardViewSetup;

// The card's BGs and palettes, on the main screen when its BG is below 4
typedef struct {
    // The card's BG, which also gives its Pokémon's BG priority
    u32 bg;
    u32 textBg;
    u32 bgPalette;
    u32 textPalette;
    // The OBJ palettes of the gift's icon and of the gift's Pokémon
    u32 iconPalette;
    u32 pokePalette;
    ClActUnit *unit;
    MysteryGiftSave *giftSave;
    MsgData *msgData;
    Font *font;
    PrintQueue *queue;
    WordSet *wordSet;
} MysteryCardResSetup;

MysteryCardView *MysteryCardView_Create(const MysteryCardViewSetup *setup, HeapID heapId);
void MysteryCardView_Delete(MysteryCardView *view);
void MysteryCardView_Main(MysteryCardView *view);
void MysteryCardView_Draw(MysteryCardView *view);
BOOL MysteryCardView_IsEnd(MysteryCardView *view);

MysteryCardRes *MysteryCardRes_Create(const MysteryCardResSetup *setup, HeapID heapId);
void MysteryCardRes_Delete(MysteryCardRes *res);

// A card drawn with the BGs and actors of res: its text, its icon, and the Pokémon of a Pokémon gift
MysteryAlbum *MysteryAlbum_CreateReceived(MysteryGift *gift, MysteryCardRes *res, GameData *gameData, HeapID heapId);
void MysteryAlbum_Delete(MysteryAlbum *album);
void MysteryAlbum_Main(MysteryAlbum *album);
// Brings a Pokémon gift's Pokémon out of the card
void MysteryAlbum_StartOpen(MysteryAlbum *album);
BOOL MysteryAlbum_IsOpened(MysteryAlbum *album);
// Draws the card at the next VBlank, with its BG's graphics, or only its palette when paletteOnly is set
void MysteryAlbum_SetVisible(MysteryAlbum *album, BOOL paletteOnly);

#endif // POKEBW2_APP_MYSTERY_MYSTERY_ALBUM_H
