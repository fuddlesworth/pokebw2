#ifndef POKEBW2_APP_P_STATUS_H
#define POKEBW2_APP_P_STATUS_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pokémon summary screen, overlay 207, which the party and box screens show and the evolution demo runs to pick
// a move to forget for a new one. The parameter's field names are ours

#define OVERLAY_PSTATUS OVERLAY_ID(207)

// What PStatusParam.party points to
enum {
    PSTATUS_DATA_PARTY_PKM,
    PSTATUS_DATA_PARTY,
    // An array of BoxPkm
    PSTATUS_DATA_BOX,
};

// What the screen is shown for
enum {
    PSTATUS_MODE_NORMAL,
    // Set by the Global Trade Station and the debug start; the summary screen treats it as PSTATUS_MODE_NORMAL
    PSTATUS_MODE_1,
    // Pick a move to forget for PStatusParam.move
    PSTATUS_MODE_FORGET_MOVE,
    // The same, where HMs can be forgotten too
    PSTATUS_MODE_FORGET_HM,
    // The markings can't be changed
    PSTATUS_MODE_LOCK_MARKINGS = 6,
};

// The pages
enum {
    PSTATUS_PAGE_INFO,
    PSTATUS_PAGE_SKILL,
    PSTATUS_PAGE_RIBBON,
    PSTATUS_PAGE_FORGET,
};

// How the screen was left
enum {
    PSTATUS_RESULT_FORGET,
    PSTATUS_RESULT_BACK,
    // X, back to the field
    PSTATUS_RESULT_CLOSE,
};

typedef struct {
    // A PartyPkm, a PokeParty or an array of BoxPkm, by dataType
    void *party;
    TrainerDataSave *trainerData;
    GameData *gameData;
    u8 dataType;
    u8 mode;
    u8 partyCount;
    u8 partyIndex;
    // The first page shown
    u8 page;
    // The slot of the move to forget
    u8 slot;
    u8 result;
    u16 move;
    BOOL isNationalDex;
    u8 unk1C[4];
    // X closes the menu and Y registers a page to the shortcut menu
    BOOL fromFieldMenu;
    // Leave the screen at once
    BOOL forceExit;
} PStatusParam;

extern GameProcFunctions PSTATUS_PROC_FUNCTIONS;

#ifdef POKEBW2_PC
typedef struct {
    Font *font;
    PartyPkm *pkm;
    PStatusParam *param;
    u32 page;
    u32 partyIndex;
    BOOL ready;
    BOOL browsing;
    u32 moveCursor;
    u32 swapSource;
    BOOL swapping;
    BOOL confirming;
    u16 moveMessage[256];
    u16 memo[3][256];
} PStatusNativeState;
PStatusWork *PStatus_GetNativeWork(void);
BOOL PStatus_GetNativeState(PStatusNativeState *state);
BOOL PStatus_SetNativePage(u32 page);
BOOL PStatus_SetNativeMove(u32 slot);
void PStaSkill_GetNativeState(PStaSkillWork *skill, PStatusNativeState *state);
void PStaInfo_GetNativeMemo(PStaInfoWork *info, PStatusNativeState *state);
u32 PStatus_GetNativeRibbon(u32 index, u16 *name, u32 nameSize, u16 *description, u32 descriptionSize, u32 *icon,
                            u32 *palette);
#endif

#endif // POKEBW2_APP_P_STATUS_H
