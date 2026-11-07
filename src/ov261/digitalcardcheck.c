#include "app/digitalcardcheck.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/event_fishing.h"

enum {
    CARD_CONFIRM_CHOICE_YES = 0,
    CARD_CONFIRM_CHOICE_NO = 1,
};

enum {
    CARD_ACTION_CHOICE_UNREGISTER = 0,
    CARD_ACTION_CHOICE_BACK = 1,
};

enum {
    CARD_BACK_CHOICE_BACK = 0,
};

enum {
    CARD1_SHOW_CARD = 0,
    CARD1_WAIT_CARD = 1,
    CARD1_WAIT_KEY = 2,
    CARD1_SHOW_STATUS = 3,
    CARD1_OPEN_MENU = 4,
    CARD1_WAIT_MENU = 5,
    CARD1_SHOW_WARNING = 6,
    CARD1_OPEN_WARNING_MENU = 7,
    CARD1_WAIT_WARNING_MENU = 8,
    CARD1_SHOW_CONFIRM = 9,
    CARD1_OPEN_CONFIRM_MENU = 10,
    CARD1_WAIT_CONFIRM_MENU = 11,
    CARD1_SHOW_UNREGISTERING = 12,
    CARD1_UNREGISTER = 13,
    CARD1_START_SAVE = 14,
    CARD1_WAIT_SAVE = 15,
    CARD1_SHOW_UNREGISTERED = 16,
    CARD1_HIDE_CARD = 17,
    CARD1_END = 18,
    CARD1_WAIT_MESSAGE = 19,
};

enum {
    CARD2_SHOW_CARD = 0,
    CARD2_WAIT_CARD = 1,
    CARD2_WAIT_KEY = 2,
    CARD2_SHOW_STATUS = 3,
    CARD2_OPEN_MENU = 4,
    CARD2_WAIT_MENU = 5,
    CARD2_SHOW_WARNING = 6,
    CARD2_OPEN_WARNING_MENU = 7,
    CARD2_WAIT_WARNING_MENU = 8,
    CARD2_SHOW_CONFIRM = 9,
    CARD2_OPEN_CONFIRM_MENU = 10,
    CARD2_WAIT_CONFIRM_MENU = 11,
    CARD2_SHOW_UNREGISTERING = 12,
    CARD2_UNREGISTER = 13,
    CARD2_START_SAVE = 14,
    CARD2_WAIT_SAVE = 15,
    CARD2_SHOW_UNREGISTERED = 16,
    CARD2_HIDE_CARD = 17,
    CARD2_END = 18,
    CARD2_WAIT_MESSAGE = 19,
};

enum {
    CARD3_SHOW_CARD = 0,
    CARD3_WAIT_CARD = 1,
    CARD3_WAIT_KEY = 2,
    CARD3_SHOW_STATUS = 3,
    CARD3_WAIT_MESSAGE = 4,
    CARD3_OPEN_MENU = 5,
    CARD3_WAIT_MENU = 6,
    CARD3_HIDE_CARD = 7,
    CARD3_END = 8,
};

enum {
    CARD4_SHOW_CARD = 0,
    CARD4_WAIT_CARD = 1,
    CARD4_WAIT_KEY = 2,
    CARD4_SHOW_STATUS = 3,
    CARD4_WAIT_MESSAGE = 4,
    CARD4_OPEN_MENU = 5,
    CARD4_WAIT_MENU = 6,
    CARD4_HIDE_CARD = 7,
    CARD4_END = 8,
};

enum {
    CARD5_SHOW_CARD = 0,
    CARD5_WAIT_CARD = 1,
    CARD5_WAIT_KEY = 2,
    CARD5_SHOW_STATUS = 3,
    CARD5_WAIT_MESSAGE = 4,
    CARD5_OPEN_MENU = 5,
    CARD5_WAIT_MENU = 6,
    CARD5_HIDE_CARD = 7,
    CARD5_END = 8,
};

enum {
    CARD_NONE_SHOW = 0,
    CARD_NONE_WAIT = 1,
    CARD_NONE_END = 2,
};

enum {
    CARD_MENU_LAYOUT_NARROW = 0,
    CARD_MENU_LAYOUT_WIDE = 1,
};

// The kinds of cards, which field 10 of the entry of the tournament block says
enum {
    CARD_TYPE_1 = 1,
    CARD_TYPE_2,
    CARD_TYPE_3,
    CARD_TYPE_4,
    CARD_TYPE_5,
};

// What the menu holds
enum {
    MENU_YES_NO,
    MENU_BACK,
    MENU_UNREGISTER_BACK,
};

// The three bytes of a digital player ID, the first in the high byte
static inline u32 DigitalCardCheck_PackBytes(u32 high, u32 middle, u32 low) {
    return (high << 24) | ((middle & 0xff) << 16) | ((low & 0xff) << 8);
}

struct DigitalCardCheck {
    // The messages of the digital card (event_fishing.h's)
    MsgData *msgData;
    WordSet *wordSet;
    WifiBattleMatchCard *card;
    WifiBattleMatchMenu *menu;
    WifiBattleMatchTask *task;
    DigitalCardCheckParam param;
    u16 heapId;
};

static void DigitalCardCheck_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType1(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType2(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType3(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType4(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType5(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskNoCard(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_CreateCard(DigitalCardCheck *check);
static void DigitalCardCheck_DeleteCard(DigitalCardCheck *check);
static BOOL DigitalCardCheck_IsCardReady(DigitalCardCheck *check);
static BOOL DigitalCardCheck_IsCardDone(DigitalCardCheck *check);
static void DigitalCardCheck_ShowCard(DigitalCardCheck *check, u32 page);
static void DigitalCardCheck_OpenMenu(DigitalCardCheck *check, u32 kind);
static void DigitalCardCheck_CloseMenu(DigitalCardCheck *check);
static s32 DigitalCardCheck_GetMenuChoice(DigitalCardCheck *check);
static void DigitalCardCheck_Print(DigitalCardCheck *check, u32 msgId, u32 style);
static BOOL DigitalCardCheck_IsPrinted(DigitalCardCheck *check);
static void DigitalCardCheck_SetBGEnabled(DigitalCardCheck *check, u32 enabled);
static void DigitalCardCheck_ClearMessage(DigitalCardCheck *check);

DigitalCardCheck *DigitalCardCheck_Create(const DigitalCardCheckParam *param, HeapID heapId) {
    DigitalCardCheck *check = GFL_HeapAllocate(heapId, sizeof(DigitalCardCheck), FALSE, "digitalcardcheck.c", 155);

    MI_CpuFill8(check, 0, sizeof(DigitalCardCheck));
    check->param = *param;
    check->heapId = heapId;
    WifiBattleMatchRes_LoadBG(check->param.seq, WIFIBATTLEMATCH_BG_CARD, heapId);
    check->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, heapId);
    check->wordSet = GFL_WordSetSystemCreate(8, 64, heapId);
    check->task = WifiBattleMatchTask_Create(check, DigitalCardCheck_TaskStart, heapId);
    return check;
}

void DigitalCardCheck_Delete(DigitalCardCheck *check) {
    WifiBattleMatchTask_Free(check->task);
    DigitalCardCheck_DeleteCard(check);
    GFL_WordSetSystemFree(check->wordSet);
    GFL_MsgDataFree(check->msgData);
    GFL_HeapFree(check);
}

void DigitalCardCheck_Update(DigitalCardCheck *check) {
    WifiBattleMatchTask_Run(check->task);
    if (check->card != NULL) {
        WifiBattleMatchCard_Flush(check->card, check->param.printQueue);
    }
}

BOOL DigitalCardCheck_IsFinished(DigitalCardCheck *check) {
    return WifiBattleMatchTask_IsEnded(check->task);
}

static void DigitalCardCheck_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    TournamentEntry *entry = TournamentBlock_GetEntry(
        getTournamentBlockAddress(GameData_GetSaveControl(check->param.gameData)), check->param.mode);
    u32 field2 = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_CUP_ID);
    u32 cardType = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_UNK15B);

    if (field2 != 0 && cardType == CARD_TYPE_1) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType1);
    } else if (field2 != 0 && cardType == CARD_TYPE_2) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType2);
    } else if (field2 != 0 && cardType == CARD_TYPE_3) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType3);
    } else if (field2 != 0 && cardType == CARD_TYPE_4) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType4);
    } else if (field2 != 0 && cardType == CARD_TYPE_5) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType5);
    } else {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskNoCard);
    }
}

static void DigitalCardCheck_TaskCardType1(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    SaveControl *save;
    BattleBoxSave *box;
    TournamentEntry *entry;
    s32 choice;
    int result;

    switch (*seq) {
    case CARD1_SHOW_CARD:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        *seq = CARD1_WAIT_CARD;
        break;
    case CARD1_WAIT_CARD:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = CARD1_WAIT_KEY;
        }
        break;
    case CARD1_WAIT_KEY:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = CARD1_SHOW_STATUS;
        }
        break;
    case CARD1_SHOW_STATUS:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_ParticipatingCompetitionPleaseRegister, 1);
        *seq = CARD1_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD1_OPEN_MENU);
        break;
    case CARD1_OPEN_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_UNREGISTER_BACK);
        *seq = CARD1_WAIT_MENU;
        break;
    case CARD1_WAIT_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_ACTION_CHOICE_UNREGISTER) {
            *seq = CARD1_SHOW_WARNING;
        } else if (choice == CARD_ACTION_CHOICE_BACK) {
            *seq = CARD1_HIDE_CARD;
        }
        break;
    case CARD1_SHOW_WARNING:
        DigitalCardCheck_Print(check, EventFishing_Text_OnceUnregisteredWillNot_3, 1);
        *seq = CARD1_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD1_OPEN_WARNING_MENU);
        break;
    case CARD1_OPEN_WARNING_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = CARD1_WAIT_WARNING_MENU;
        break;
    case CARD1_WAIT_WARNING_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_CONFIRM_CHOICE_YES) {
            *seq = CARD1_SHOW_CONFIRM;
        } else if (choice == CARD_CONFIRM_CHOICE_NO) {
            *seq = CARD1_HIDE_CARD;
        }
        break;
    case CARD1_SHOW_CONFIRM:
        DigitalCardCheck_Print(check, EventFishing_Text_WillUnregisteredSureAbout_2, 1);
        *seq = CARD1_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD1_OPEN_CONFIRM_MENU);
        break;
    case CARD1_OPEN_CONFIRM_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = CARD1_WAIT_CONFIRM_MENU;
        break;
    case CARD1_WAIT_CONFIRM_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_CONFIRM_CHOICE_YES) {
            *seq = CARD1_SHOW_UNREGISTERING;
        } else if (choice == CARD_CONFIRM_CHOICE_NO) {
            *seq = CARD1_HIDE_CARD;
        }
        break;
    case CARD1_SHOW_UNREGISTERING:
        DigitalCardCheck_Print(check, EventFishing_Text_BeingUnregisteredDontTurn, 2);
        *seq = CARD1_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD1_UNREGISTER);
        break;
    case CARD1_UNREGISTER:
        save = GameData_GetSaveControl(check->param.gameData);
        entry = TournamentBlock_GetEntry(getTournamentBlockAddress(save), check->param.mode);
        box = getBattleBox(save);
        BattleBoxSave_ClearFlags(box, check->param.mode == DIGITAL_CARD_CHECK_WIFI ? 1 : 2);
        TournamentEntry_SetField(entry, TOURNAMENT_FIELD_UNK15B, 4);
        *seq = CARD1_START_SAVE;
        break;
    case CARD1_START_SAVE:
        GameData_StartSave(check->param.gameData);
        *seq = CARD1_WAIT_SAVE;
        break;
    case CARD1_WAIT_SAVE:
        result = GameData_UpdateSave(check->param.gameData);
        if (result == SAVE_RESULT_OK) {
            *seq = CARD1_SHOW_UNREGISTERED;
        } else if (result == SAVE_RESULT_NG) {
            *seq = CARD1_SHOW_UNREGISTERED;
        }
        break;
    case CARD1_SHOW_UNREGISTERED:
        DigitalCardCheck_Print(check, EventFishing_Text_HaveBeenUnregisteredFrom, 1);
        *seq = CARD1_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD1_HIDE_CARD);
        break;
    case CARD1_HIDE_CARD:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = CARD1_END;
        }
        break;
    case CARD1_END:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    case CARD1_WAIT_MESSAGE:
        if (DigitalCardCheck_IsPrinted(check)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void DigitalCardCheck_TaskCardType2(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    SaveControl *save;
    BattleBoxSave *box;
    TournamentEntry *entry;
    s32 choice;
    int result;

    switch (*seq) {
    case CARD2_SHOW_CARD:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, WIFIBATTLEMATCH_CARD_PAGE_REGISTERED);
        *seq = CARD2_WAIT_CARD;
        break;
    case CARD2_WAIT_CARD:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = CARD2_WAIT_KEY;
        }
        break;
    case CARD2_WAIT_KEY:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = CARD2_SHOW_STATUS;
        }
        break;
    case CARD2_SHOW_STATUS:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_ParticipatingCompetitionBattleBox, 1);
        *seq = CARD2_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD2_OPEN_MENU);
        break;
    case CARD2_OPEN_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_UNREGISTER_BACK);
        *seq = CARD2_WAIT_MENU;
        break;
    case CARD2_WAIT_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_ACTION_CHOICE_UNREGISTER) {
            *seq = CARD2_SHOW_WARNING;
        } else if (choice == CARD_ACTION_CHOICE_BACK) {
            *seq = CARD2_HIDE_CARD;
        }
        break;
    case CARD2_SHOW_WARNING:
        DigitalCardCheck_Print(check, EventFishing_Text_OnceUnregisteredWillNot_2, 1);
        *seq = CARD2_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD2_OPEN_WARNING_MENU);
        break;
    case CARD2_OPEN_WARNING_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = CARD2_WAIT_WARNING_MENU;
        break;
    case CARD2_WAIT_WARNING_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_CONFIRM_CHOICE_YES) {
            *seq = CARD2_SHOW_CONFIRM;
        } else if (choice == CARD_CONFIRM_CHOICE_NO) {
            *seq = CARD2_HIDE_CARD;
        }
        break;
    case CARD2_SHOW_CONFIRM:
        DigitalCardCheck_Print(check, EventFishing_Text_WillUnregisteredSureAbout_2, 1);
        *seq = CARD2_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD2_OPEN_CONFIRM_MENU);
        break;
    case CARD2_OPEN_CONFIRM_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = CARD2_WAIT_CONFIRM_MENU;
        break;
    case CARD2_WAIT_CONFIRM_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_CONFIRM_CHOICE_YES) {
            *seq = CARD2_SHOW_UNREGISTERING;
        } else if (choice == CARD_CONFIRM_CHOICE_NO) {
            *seq = CARD2_HIDE_CARD;
        }
        break;
    case CARD2_SHOW_UNREGISTERING:
        DigitalCardCheck_Print(check, EventFishing_Text_BeingUnregisteredDontTurn, 2);
        *seq = CARD2_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD2_UNREGISTER);
        break;
    case CARD2_UNREGISTER:
        save = GameData_GetSaveControl(check->param.gameData);
        entry = TournamentBlock_GetEntry(getTournamentBlockAddress(save), check->param.mode);
        box = getBattleBox(save);
        BattleBoxSave_ClearFlags(box, check->param.mode == DIGITAL_CARD_CHECK_WIFI ? 1 : 2);
        TournamentEntry_SetField(entry, TOURNAMENT_FIELD_UNK15B, 4);
        *seq = CARD2_START_SAVE;
        break;
    case CARD2_START_SAVE:
        GameData_StartSave(check->param.gameData);
        *seq = CARD2_WAIT_SAVE;
        break;
    case CARD2_WAIT_SAVE:
        result = GameData_UpdateSave(check->param.gameData);
        if (result == SAVE_RESULT_OK) {
            *seq = CARD2_SHOW_UNREGISTERED;
        } else if (result == SAVE_RESULT_NG) {
            *seq = CARD2_SHOW_UNREGISTERED;
        }
        break;
    case CARD2_SHOW_UNREGISTERED:
        DigitalCardCheck_Print(check, EventFishing_Text_BattleBoxHasBeen_2, 1);
        *seq = CARD2_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, CARD2_HIDE_CARD);
        break;
    case CARD2_HIDE_CARD:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = CARD2_END;
        }
        break;
    case CARD2_END:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    case CARD2_WAIT_MESSAGE:
        if (DigitalCardCheck_IsPrinted(check)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void DigitalCardCheck_TaskCardType3(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    s32 choice;

    switch (*seq) {
    case CARD3_SHOW_CARD:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        *seq = CARD3_WAIT_CARD;
        break;
    case CARD3_WAIT_CARD:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = CARD3_WAIT_KEY;
        }
        break;
    case CARD3_WAIT_KEY:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = CARD3_SHOW_STATUS;
        }
        break;
    case CARD3_SHOW_STATUS:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_CompetitionHasAlreadyEnded, 1);
        *seq = CARD3_WAIT_MESSAGE;
        break;
    case CARD3_WAIT_MESSAGE:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = CARD3_OPEN_MENU;
        }
        break;
    case CARD3_OPEN_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_BACK);
        *seq = CARD3_WAIT_MENU;
        break;
    case CARD3_WAIT_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_BACK_CHOICE_BACK) {
            *seq = CARD3_HIDE_CARD;
        }
        break;
    case CARD3_HIDE_CARD:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = CARD3_END;
        }
        break;
    case CARD3_END:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskCardType4(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    s32 choice;

    switch (*seq) {
    case CARD4_SHOW_CARD:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        *seq = CARD4_WAIT_CARD;
        break;
    case CARD4_WAIT_CARD:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = CARD4_WAIT_KEY;
        }
        break;
    case CARD4_WAIT_KEY:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = CARD4_SHOW_STATUS;
        }
        break;
    case CARD4_SHOW_STATUS:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_UnregisteredFromCompetition, 1);
        *seq = CARD4_WAIT_MESSAGE;
        break;
    case CARD4_WAIT_MESSAGE:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = CARD4_OPEN_MENU;
        }
        break;
    case CARD4_OPEN_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_BACK);
        *seq = CARD4_WAIT_MENU;
        break;
    case CARD4_WAIT_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_BACK_CHOICE_BACK) {
            *seq = CARD4_HIDE_CARD;
        }
        break;
    case CARD4_HIDE_CARD:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = CARD4_END;
        }
        break;
    case CARD4_END:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskCardType5(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    s32 choice;

    switch (*seq) {
    case CARD5_SHOW_CARD:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        *seq = CARD5_WAIT_CARD;
        break;
    case CARD5_WAIT_CARD:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = CARD5_WAIT_KEY;
        }
        break;
    case CARD5_WAIT_KEY:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = CARD5_SHOW_STATUS;
        }
        break;
    case CARD5_SHOW_STATUS:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_YouveBeenUnregisteredFrom, 1);
        *seq = CARD5_WAIT_MESSAGE;
        break;
    case CARD5_WAIT_MESSAGE:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = CARD5_OPEN_MENU;
        }
        break;
    case CARD5_OPEN_MENU:
        DigitalCardCheck_OpenMenu(check, MENU_BACK);
        *seq = CARD5_WAIT_MENU;
        break;
    case CARD5_WAIT_MENU:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == CARD_BACK_CHOICE_BACK) {
            *seq = CARD5_HIDE_CARD;
        }
        break;
    case CARD5_HIDE_CARD:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = CARD5_END;
        }
        break;
    case CARD5_END:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskNoCard(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;

    switch (*seq) {
    case CARD_NONE_SHOW:
        DigitalCardCheck_Print(check, EventFishing_Text_DontHaveAnyDigital, 1);
        *seq = CARD_NONE_WAIT;
        break;
    case CARD_NONE_WAIT:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = CARD_NONE_END;
        }
        break;
    case CARD_NONE_END:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void DigitalCardCheck_CreateCard(DigitalCardCheck *check) {
    SaveControl *save;
    TournamentBlock *tournament;
    TournamentEntry *entry;
    TournamentTeam *cardData;
    Regulation *regulation;
    RecordSave *record;
    PlayerInfo *playerInfo;
    ClActUnit *link;

    if (check->card != NULL) {
        return;
    }
    save = GameData_GetSaveControl(check->param.gameData);
    tournament = getTournamentBlockAddress(GameData_GetSaveControl(check->param.gameData));
    entry = TournamentBlock_GetEntry(tournament, check->param.mode);
    cardData = TournamentBlock_GetTeam(tournament, check->param.mode);
    regulation = TournamentEntry_GetRegulation(entry);
    record = getRecordBlkAddress(save);
    playerInfo = GetGameDataPlayerInfo(check->param.gameData);
    link = WifiBattleMatchGraphic_GetClActUnit(check->param.net);
    if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
        WifiBattleMatchWifiCardInfo info;
        RndMatchData *results;
        u32 first;

        MI_CpuFill8(&info, 0, sizeof(WifiBattleMatchWifiCardInfo));
        MI_CpuCopy8(TournamentEntry_GetCardData(entry), &info, 0x92);
        results = RecordSave_GetRndMatch(record);
        info.rating = RndMatchData_GetParam(results, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_RATING);
        first = RndMatchData_GetParam(results, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_WIN);
        info.matchCount = first + RndMatchData_GetParam(results, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_LOSE);
        info.look = PlayerInfo_GetUnionView(playerInfo);
        info.startDate = DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_YEAR),
                                                    TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_MONTH),
                                                    TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_DAY));
        info.endDate = DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_YEAR),
                                                  TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_MONTH),
                                                  TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_DAY));
        info.variant = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_VARIANT);
        check->card = WifiBattleMatchWifiCard_Create(&info, 0, playerInfo, link, check->param.seq, check->param.font,
                                                     check->param.printQueue, check->msgData, check->wordSet, cardData,
                                                     1, check->heapId);
    } else {
        WifiBattleMatchLiveCardInfo info;
        LiveMatchData *results;

        MI_CpuFill8(&info, 0, sizeof(WifiBattleMatchLiveCardInfo));
        MI_CpuCopy8(TournamentEntry_GetCardData(entry), &info, 0x92);
        results = RecordSave_GetLiveMatch(record);
        info.wins = LiveMatchData_GetCount(results, LIVEMATCH_COUNT_WON);
        info.losses = LiveMatchData_GetCount(results, LIVEMATCH_COUNT_LOST);
        info.matchCount = LiveMatchData_GetCount(results, LIVEMATCH_COUNT_PLAYED);
        info.maxBattles = Regulation_GetParam(regulation, REGULATION_PARAM_MAX_BATTLES);
        info.look = PlayerInfo_GetUnionView(playerInfo);
        info.startDate = DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_YEAR),
                                                    TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_MONTH),
                                                    TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_DAY));
        info.endDate = DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_YEAR),
                                                  TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_MONTH),
                                                  TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_DAY));
        info.variant = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_VARIANT);
        check->card = WifiBattleMatchLiveCard_Create(&info, playerInfo, link, check->param.seq, check->param.font,
                                                     check->param.printQueue, check->msgData, check->wordSet, cardData,
                                                     1, check->heapId);
    }
    DigitalCardCheck_SetBGEnabled(check, 0);
}

static void DigitalCardCheck_DeleteCard(DigitalCardCheck *check) {
    if (check->card != NULL) {
        if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
            WifiBattleMatchWifiCard_Free(check->card);
        } else {
            WifiBattleMatchLiveCard_Free(check->card);
        }
        check->card = NULL;
    }
    GFL_BGSysClearScr(1);
    GFL_BGSysLoadScr(1);
}

static BOOL DigitalCardCheck_IsCardReady(DigitalCardCheck *check) {
    return WifiBattleMatchCard_SlideIn(check->card);
}

static BOOL DigitalCardCheck_IsCardDone(DigitalCardCheck *check) {
    return WifiBattleMatchCard_SlideOut(check->card);
}

static void DigitalCardCheck_ShowCard(DigitalCardCheck *check, u32 page) {
    if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
        WifiBattleMatchWifiCard_ShowPage(check->card, page, check->msgData, check->param.printQueue, check->param.font,
                                         check->heapId);
    } else {
        WifiBattleMatchLiveCard_ShowPage(check->card, page, check->msgData, check->param.printQueue, check->param.font,
                                         check->heapId);
    }
}

static void DigitalCardCheck_OpenMenu(DigitalCardCheck *check, u32 kind) {
    WifiBattleMatchMenuParam param;
    u32 layout;
    int x, y, width, height;

    if (check->menu != NULL) {
        return;
    }
    MI_CpuFill8(&param, 0, sizeof(WifiBattleMatchMenuParam));
    param.msgData = check->msgData;
    param.font = check->param.font;
    param.printQueue = check->param.printQueue;
    param.bg = 0;
    param.palette = 15;
    param.framePalette = 13;
    param.frameChar = 1;
    switch (kind) {
    case MENU_YES_NO:
        param.msgIds[0] = EventFishing_Text_Yes_4;
        param.msgIds[1] = EventFishing_Text_No_4;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        layout = CARD_MENU_LAYOUT_NARROW;
        param.cursorRow = 1;
        break;
    case MENU_BACK:
        param.msgIds[0] = EventFishing_Text_Back;
        param.count = 1;
        param.canCancel = 1;
        param.cancelValue = 0;
        layout = CARD_MENU_LAYOUT_NARROW;
        break;
    case MENU_UNREGISTER_BACK:
        param.msgIds[0] = EventFishing_Text_Unregister;
        param.msgIds[1] = EventFishing_Text_Back;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        layout = CARD_MENU_LAYOUT_WIDE;
        param.cursorRow = 1;
        break;
    }
    switch (layout) {
    case CARD_MENU_LAYOUT_NARROW:
        width = 12;
        height = param.count * 2;
        x = 19;
        y = 17 - height;
        break;
    case CARD_MENU_LAYOUT_WIDE:
        width = 15;
        height = param.count * 2;
        x = 16;
        y = 17 - height;
        break;
    }
    check->menu = WifiBattleMatchMenu_Create(&param, x, y, width, height, check->heapId);
}

static void DigitalCardCheck_CloseMenu(DigitalCardCheck *check) {
    if (check->menu != NULL) {
        WifiBattleMatchMenu_Free(check->menu);
        check->menu = NULL;
    }
}

static s32 DigitalCardCheck_GetMenuChoice(DigitalCardCheck *check) {
    if (check->menu != NULL) {
        return WifiBattleMatchMenu_Update(check->menu);
    }
    return -1;
}

static void DigitalCardCheck_Print(DigitalCardCheck *check, u32 msgId, u32 style) {
    WifiBattleMatchMsgBox_Print(check->param.msgBox, check->msgData, msgId, style);
}

static BOOL DigitalCardCheck_IsPrinted(DigitalCardCheck *check) {
    return WifiBattleMatchMsgBox_IsDone(check->param.msgBox);
}

static void DigitalCardCheck_SetBGEnabled(DigitalCardCheck *check, u32 enabled) {
    GFL_BGSysSetBGEnabled(0, enabled);
}

static void DigitalCardCheck_ClearMessage(DigitalCardCheck *check) {
    WifiBattleMatchMsgBox_Clear(check->param.msgBox);
}
