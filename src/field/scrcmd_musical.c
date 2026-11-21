#include "types.h"
#include "app/ircbattlematch.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/comm_entry_menu.h"
#include "field/event_poke_status.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/musical.h"
#include "field/musical_dressup_sys.h"
#include "field/pdw_postman.h"
#include "field/scrcmd_musical.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/dream_world.h"
#include "save/musical_savedata.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The event that runs a musical
typedef struct {
    GameEvent *event;
    MusicalCommWork *work;
    BOOL online;
} MusicalCallEvent;

typedef struct {
    MusicalShotParam *param;
} MusicalShotHolder;

typedef struct {
    MusicalDressUpParam *param;
} MusicalDressUpEvent;

typedef struct {
    FieldScriptEnv *env;
    MusicalCommWork *work;
} MusicalIrcMatchHolder;

void MusicalCommWork_Create(ScriptWork *work, GameData *gameData);
void MusicalCommWork_Free(MusicalCommWork *work, GameData *gameData);
void MusicalCommWork_BootComm(MusicalCommWork *work, GameSystem *gsys, GameCommSys *comm, u16 value);
void MusicalCommWork_QuitComm(MusicalCommWork *work);
GameEventReturnCode MusicalCallEvent_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode MusicalDressUpEvent_Callback(GameEvent *event, u32 *state, void *data);
void MusicalShotHolder_Free(void *arg);
void MusicalIrcMatch_OnReturn(void *arg);
BOOL ScriptNative_MusicalLeaderMenuWait(VM *vm, void *arg);
BOOL ScriptNative_MusicalSelectLeaderMenuWait(VM *vm, void *arg);
BOOL ScriptNative_MusicalTimingWait(VM *vm, void *arg);
BOOL ScriptNative_MusicalScriptRecvWait(VM *vm, void *arg);
BOOL ScriptNative_MusicalCommBootWait(VM *vm, void *arg);
BOOL ScriptNative_MusicalCommEndWait(VM *vm, void *arg);
BOOL ScriptNative_MusicalPokesRecvWait(VM *vm, void *arg);

// What Scrcmd_Musical_MusicalCall does: plays the show with the party Pokémon at the argument, alone or online, or
// opens the photos of the shows played
enum {
    MUSICAL_CALL_OFFLINE_SHOW = 0,
    MUSICAL_CALL_ONLINE_SHOW = 1,
    MUSICAL_CALL_PHOTO = 2,
};

BOOL Scrcmd_Musical_MusicalCall(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u16 arg;
    u8 kind;
    MusicalCommWork *commWork;
    BOOL online;
    GameEvent *event;
    MusicalCallEvent *data;
    MusicalSave *save;
    MusicalShotHolder *holder;

    ScriptWork_GetFieldWork(work);
    arg = ScriptReadAny(vm, env);
    kind = VM_Read8(vm);
    if (kind <= MUSICAL_CALL_ONLINE_SHOW) {
        commWork = GameData_GetMusicalCommWork(gameData);
        online = kind != MUSICAL_CALL_OFFLINE_SHOW ? TRUE : FALSE;
        event = GameEvent_Create(gsys, NULL, MusicalCallEvent_Callback, sizeof(MusicalCallEvent));
        data = GameEvent_GetData(event);
        MI_CpuFill8(data, 0, sizeof(MusicalCallEvent));
        data->event = MusicalEvent_Create(gsys, gameData, arg, online, commWork);
        data->work = commWork;
        data->online = online;
        ScriptWork_SetPostEvent(work, event);
        GameData_GetFieldSoundSystem(gameData);
        VM_Halt(vm);
        return TRUE;
    }
    save = getMusicalInfoBlkAddress(gameData);
    holder = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalShotHolder), TRUE, "scrcmd_musical.c", 171);
    holder->param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalShotParam), TRUE, "scrcmd_musical.c", 172);
    holder->param->askSave = FALSE;
    holder->param->shot = MusicalSave_GetShot(save);
    holder->param->loadComm = TRUE;
    holder->param->loadData = TRUE;
    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(
                                   gsys, field, OVERLAY_ID(209), &MUSICAL_SHOT_PROC_FUNCTIONS, holder->param,
                                   MusicalShotHolder_Free, holder));
    return TRUE;
}

BOOL Scrcmd_Musical_MusicalCallDressUp(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 slot;
    MusicalSave *save;
    MusicalPoke *poke;
    GameEvent *event;
    MusicalDressUpEvent *data;

    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    save = getMusicalInfoBlkAddress(gameData);
    poke = MusicalSystem_InitPokeFromPkm(PokeParty_GetPkm(GameData_GetParty(gameData), slot), HEAPID_GAMEEVENT);
    event = GameEvent_Create(gsys, NULL, MusicalDressUpEvent_Callback, sizeof(MusicalDressUpEvent));
    data = GameEvent_GetData(event);
    MI_CpuFill8(data, 0, sizeof(MusicalDressUpEvent));
    data->param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalDressUpParam), TRUE, "scrcmd_musical.c", 218);
    data->param->poke = poke;
    data->param->save = save;
    data->param->comm = NULL;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Musical_MusicalGetInfo(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u32 mostIndex;
    u32 fewestIndex;
    u32 firstIndex;
    u8 topIndex;
    u8 secondValue;
    u32 second;
    PokeParty *party;
    u8 kind;
    u16 arg;
    u16 *result;
    MusicalSave *save;
    MusicalEventWork *event;
    MusicalCommWork *commWork;
    u8 i;
    u8 j;
    u8 most;
    u8 count;
    u8 value;
    u8 best;
    MusicalSaveEquip *entries;

    FieldScriptEnv_GetScriptWork(env);
    kind = VM_Read8(vm);
    arg = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    gameData = FieldScriptEnv_GetGameData(env);
    save = getMusicalInfoBlkAddress(gameData);
    event = NULL;
    if (kind >= MUSICAL_INFO_PLAYER_POS) {
        commWork = GameData_GetMusicalCommWork(gameData);
        if (commWork == NULL) {
            *result = 0;
            return FALSE;
        }
        event = commWork->event;
    }
    switch (kind) {
    case MUSICAL_INFO_SHOW_COUNT:
        *result = MusicalSave_GetShowCount(save);
        break;
    case MUSICAL_INFO_WIN_COUNT:
        *result = MusicalSave_GetWinCount(save);
        break;
    case MUSICAL_INFO_LAST_POINTS:
        *result = MusicalSave_GetLastPoints(save);
        break;
    case MUSICAL_INFO_MOST_PROP_KIND:
        best = 0;
        mostIndex = MUSICAL_PROP_KIND_NONE;
        for (i = 0; i < 4; i++) {
            value = MusicalSave_GetPropKindCount(save, i);
            if (value > best) {
                best = value;
                mostIndex = i;
            } else if (value == best) {
                mostIndex = MUSICAL_PROP_KIND_NONE;
            }
        }
        *result = mostIndex;
        break;
    case MUSICAL_INFO_PROGRAM:
        *result = MusicalSave_GetProgram(save);
        break;
    case MUSICAL_INFO_SET_PROGRAM:
        MusicalSave_SetProgram(save, arg);
        break;
    case MUSICAL_INFO_HAS_WORN_PROP:
        *result = 0;
        for (i = 0; i < 4; i++) {
            if (MusicalSave_GetPropKindCount(save, i) != 0) {
                *result = 1;
            }
        }
        break;
    case MUSICAL_INFO_HAS_SHOT:
        if (MusicalSave_GetShot(save)->month == 0) {
            *result = FALSE;
        } else {
            *result = TRUE;
        }
        break;
    case MUSICAL_INFO_LEAST_PROP_KIND:
        best = 0xff;
        fewestIndex = MUSICAL_PROP_KIND_NONE;
        for (i = 0; i < 4; i++) {
            value = MusicalSave_GetPropKindCount(save, i);
            if (value <= best) {
                best = value;
                fewestIndex = i;
            }
        }
        *result = fewestIndex;
        break;
    case MUSICAL_INFO_FIRST_MOST_PROP_KIND:
        best = 0;
        firstIndex = MUSICAL_PROP_KIND_NONE;
        for (i = 0; i < 4; i++) {
            value = MusicalSave_GetPropKindCount(save, i);
            if (value > best) {
                best = value;
                firstIndex = i;
            }
        }
        *result = firstIndex;
        break;
    case MUSICAL_INFO_SECOND_MOST_PROP_KIND:
        topIndex = MUSICAL_PROP_KIND_NONE;
        secondValue = 0;
        most = 0;
        second = MUSICAL_PROP_KIND_NONE;
        for (j = 0; j < 4; j++) {
            value = MusicalSave_GetPropKindCount(save, j);
            if (value > most) {
                if (secondValue == most) {
                    second = MUSICAL_PROP_KIND_NONE;
                } else {
                    secondValue = most;
                    second = topIndex;
                }
                most = value;
                topIndex = j;
            }
        }
        *result = second;
        break;
    case MUSICAL_INFO_CAN_JOIN_COUNT:
        count = 0;
        party = GameData_GetParty(gameData);
        value = PokeParty_GetPkmCount(party);
        for (i = 0; i < value; i++) {
            if (MusicalSystem_CanJoin(PokeParty_GetPkm(party, i)) == TRUE) {
                count++;
            }
        }
        *result = count;
        break;
    case MUSICAL_INFO_CAN_JOIN_POKE:
        if (MusicalSystem_CanJoin(PokeParty_GetPkm(GameData_GetParty(gameData), arg)) == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MUSICAL_INFO_HAS_EQUIP:
        entries = MusicalSave_GetEquips(save);
        *result = 0;
        for (i = 0; i < 8; i++) {
            if (entries[i].pos != MUSICAL_SAVED_EQUIP_NONE) {
                *result = 1;
            }
        }
        break;
    case MUSICAL_INFO_HAS_DIST_PROGRAM:
        if (MusicalSave_HasDistProgram(save) == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MUSICAL_INFO_HAS_PROP:
        if (MusicalSave_HasProp(save, arg) == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MUSICAL_INFO_PLAYER_POS:
        *result = MusicalEvent_GetPlayerPos(event);
        break;
    case MUSICAL_INFO_MAX_POINTS:
        *result = MusicalEvent_GetMaxPoints(event);
        break;
    case MUSICAL_INFO_MIN_POINTS:
        *result = MusicalEvent_GetMinPoints(event);
        break;
    case MUSICAL_INFO_POINTS:
        *result = MusicalEvent_GetPoints(event, arg);
        break;
    case MUSICAL_INFO_ENTRY_INDEX:
        *result = MusicalEvent_GetEntryIndex(event, arg);
        break;
    case MUSICAL_INFO_TOP_KIND:
        *result = MusicalEvent_GetTopPropKind(event);
        break;
    case MUSICAL_INFO_POS_AT_RANK:
        *result = MusicalEvent_GetPosAtRank(event, arg);
        break;
    case MUSICAL_INFO_BEST_PROP_KIND:
        *result = MusicalEvent_GetBestPropKind(event, arg);
        break;
    case MUSICAL_INFO_OWNER_TRAINER_CLASS:
        *result = MusicalEvent_GetOwnerTrainerClass(event, arg);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Musical_MusicalGetAudienceGift(VM *vm, FieldScriptEnv *env) {
    u16 arg = ScriptReadAny(vm, env);
    u8 kind = VM_Read8(vm);
    u16 *result = ScriptReadVar(vm, env);
    MusicalAudienceGift *entry =
        MusicalSave_GetAudienceGift(getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env)), arg);

    switch (kind) {
    case MUSICAL_GIFT_HAS_GIFT:
        // The number the gift gives for each top kind of the show, which no list names and the script only tests for
        // zero
        switch (entry->topKind) {
        case MUSICAL_PROP_KIND_COOL:
            *result = 24;
            break;
        case MUSICAL_PROP_KIND_CUTE:
            *result = 15;
            break;
        case MUSICAL_PROP_KIND_ELEGANT:
            *result = 45;
            break;
        case MUSICAL_PROP_KIND_UNIQUE:
            *result = 12;
            break;
        case MUSICAL_PROP_KIND_NONE:
            *result = 0;
            break;
        }
        break;
    case MUSICAL_GIFT_COMMENT:
        *result = arg + 5;
        break;
    case MUSICAL_GIFT_COMMENT_WITH_PROP:
        *result = arg + 15;
        break;
    case MUSICAL_GIFT_HAS_PROP:
        *result = entry->hasProp;
        break;
    case MUSICAL_GIFT_PROP:
        *result = entry->prop;
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Musical_MusicalAddProp(VM *vm, FieldScriptEnv *env) {
    u16 prop = ScriptReadAny(vm, env);

    MusicalSave_AddProp(getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env)), prop);
    return FALSE;
}

BOOL Scrcmd_Musical_WordSetMusicalInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 kind = VM_Read8(vm);
    u8 wordIndex = VM_Read8(vm);
    u16 arg = ScriptReadAny(vm, env);
    MusicalSave *save;
    MusicalSave *musical;
    MsgData *msgData;
    StrBuf *str;
    u8 i;
    u8 j;
    u8 next;
    u8 tmp;
    u16 messages[3];
    u8 order[4];
    u8 points[4];

    switch (kind) {
    case MUSICAL_WORD_NAME:
        save = getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env));
        str = GFL_StrBufCreate(0x26, heapId);
        GFL_StrBufLoadFixedString(str, MusicalSave_GetName(save), 0x26);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        break;
    case MUSICAL_WORD_PROP_NAME:
    case MUSICAL_WORD_PROP_NAME_ARTICLE:
        if (kind == MUSICAL_WORD_PROP_NAME) {
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MUSICAL_PROP_NAMES, heapId);
        } else {
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_MUSICAL, heapId);
        }
        str = GFL_MsgDataLoadStrbufNew(msgData, arg + 3);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        break;
    case MUSICAL_WORD_PROGRAM_TITLE:
        str = MusicalEvent_GetProgramTitle(GameData_GetMusicalCommWork(gameData)->event, heapId);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        break;
    case MUSICAL_WORD_AUDIENCE:
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MUSICAL_INFO_WORDS, heapId);
        str = GFL_MsgDataLoadStrbufNew(msgData, arg);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        break;
    case MUSICAL_WORD_STYLE:
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MUSICAL_INFO_WORDS, heapId);
        str = GFL_MsgDataLoadStrbufNew(msgData, arg + 4);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, str, 0, 1, 2);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        break;
    case MUSICAL_WORD_OWNER_NAME:
        MusicalEvent_WordSetOwnerName(GameData_GetMusicalCommWork(gameData)->event, arg, wordSet, wordIndex);
        break;
    case MUSICAL_WORD_POKE_NAME:
        MusicalEvent_WordSetPokeName(GameData_GetMusicalCommWork(gameData)->event, arg, wordSet, wordIndex);
        break;
    case MUSICAL_WORD_RANKING:
        musical = getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env));
        for (i = 0; i < 4; i++) {
            order[i] = i;
            points[i] = MusicalSave_GetPropKindCount(musical, i);
        }
        for (i = 1; i < 4; i++) {
            for (j = 0; j < 4 - i; j++) {
                next = j + 1;
                if (points[j] < points[next]) {
                    tmp = points[j];
                    points[j] = points[next];
                    points[next] = tmp;
                    tmp = order[j];
                    order[j] = order[next];
                    order[next] = tmp;
                }
            }
        }
        if (points[0] == points[1] && points[0] == points[2]) {
            messages[0] = 9;
            messages[1] = 8;
        } else {
            messages[0] = order[0] + 4;
            if (points[1] == points[2]) {
                messages[1] = 8;
            } else {
                messages[1] = order[1] + 4;
            }
        }
        if (points[1] == points[3]) {
            messages[2] = order[1] + 4;
        } else if (points[2] == points[3]) {
            messages[2] = order[2] + 4;
        } else {
            messages[2] = order[3] + 4;
        }
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MUSICAL_INFO_WORDS, heapId);
        for (i = 0; i < 3; i++) {
            str = GFL_MsgDataLoadStrbufNew(msgData, messages[i]);
            GFL_WordSetSetStrbuf(wordSet, i, str, 0, 1, 2);
            GFL_StrBufFree(str);
        }
        GFL_MsgDataFree(msgData);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Musical_MusicalClearAudienceGift(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);

    MusicalSave_GetAudienceGift(getMusicalInfoBlkAddress(FieldScriptEnv_GetGameData(env)), index)->hasProp = 0;
    return FALSE;
}

BOOL Scrcmd_Musical_MusicalSelectPoke(VM *vm, FieldScriptEnv *env) {
    u16 *picked = ScriptReadVar(vm, env);
    u16 *index = ScriptReadVar(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    PokeSelectArgs args;

    MI_CpuFill8(&args, 0, sizeof(PokeSelectArgs));
    args.picked = picked;
    args.index = index;
    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKE_STATUS, EventMusicalPokeSelect_Create, &args));
    return TRUE;
}

BOOL Scrcmd_Musical_MusicalComm(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u16 kind = ScriptReadAny(vm, env);
    u16 arg = ScriptReadAny(vm, env);
    u16 *result;
    BOOL lost;
    MusicalCommWork *commWork;
    BOOL allowed;
    s32 i;
    PlayerInfo *info;
    MusicalIrcMatchHolder *holder;

    ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    lost = FALSE;
    commWork = NULL;
    if (kind != MUSICAL_COMM_CREATE && kind != MUSICAL_COMM_CHECK_NET_ERROR) {
        commWork = GameData_GetMusicalCommWork(gameData);
        if (commWork->event != NULL) {
            MusicalEvent_CheckNetError(commWork->event);
            if (MusicalEvent_IsNetError(commWork->event) == TRUE) {
                lost = TRUE;
            }
        }
    }
    switch (kind) {
    case MUSICAL_COMM_CREATE:
        MusicalCommWork_Create(work, gameData);
        break;
    case MUSICAL_COMM_FREE:
        MusicalCommWork_Free(commWork, gameData);
        break;
    case MUSICAL_COMM_BOOT:
        MusicalCommWork_BootComm(commWork, gsys, GSYS_GetGameCommSystem(gsys), arg);
        break;
    case MUSICAL_COMM_QUIT:
        MusicalCommWork_QuitComm(commWork);
        VM_SetNativeCallback(vm, ScriptNative_MusicalCommEndWait);
        return TRUE;
    case MUSICAL_COMM_LEADER_MENU:
        commWork->result = result;
        commWork->menu = CommEntryMenu_Create(GetGameDataPlayerInfo(gameData), field, 2, 4, HEAPID_GAMEEVENT,
                                              COMM_ENTRY_MENU_LEADER, COMM_ENTRY_MENU_MUSICAL, NULL);
        VM_SetNativeCallback(vm, ScriptNative_MusicalLeaderMenuWait);
        return TRUE;
    case MUSICAL_COMM_SELECT_LEADER_MENU:
        commWork->result = result;
        commWork->menu = CommEntryMenu_Create(GetGameDataPlayerInfo(gameData), field, 2, 4, HEAPID_GAMEEVENT,
                                              COMM_ENTRY_MENU_SELECT_LEADER, COMM_ENTRY_MENU_MUSICAL, NULL);
        VM_SetNativeCallback(vm, ScriptNative_MusicalSelectLeaderMenuWait);
        return TRUE;
    case MUSICAL_COMM_SEND_TIMING:
        allowed = TRUE;
        if (commWork->comm != NULL && arg == 0x8e) {
            for (i = 0; i < 4; i++) {
                info = MusComm_GetPlayerInfo(commWork->comm, i);
                if (info != NULL &&
                    (u32)(PlayerInfo_GetVersion(info) - VERSION_WHITE) <= VERSION_BLACK - VERSION_WHITE) {
                    allowed = FALSE;
                }
            }
        }
        if (commWork->comm == NULL || lost) {
            break;
        }
        if (allowed) {
            commWork->value = arg;
            MusComm_SendTiming(commWork->comm, commWork->value);
            VM_SetNativeCallback(vm, ScriptNative_MusicalTimingWait);
            return TRUE;
        }
        break;
    case MUSICAL_COMM_WAIT_SCRIPT:
        if (commWork->comm != NULL && !lost) {
            VM_SetNativeCallback(vm, ScriptNative_MusicalScriptRecvWait);
            return TRUE;
        }
        break;
    case MUSICAL_COMM_WAIT_BOOT:
        VM_SetNativeCallback(vm, ScriptNative_MusicalCommBootWait);
        return TRUE;
    case MUSICAL_COMM_WAIT_POKES:
        if (commWork->comm != NULL && !lost) {
            VM_SetNativeCallback(vm, ScriptNative_MusicalPokesRecvWait);
            return TRUE;
        }
        break;
    case MUSICAL_COMM_START_IRC_MATCH:
        holder = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GAMEEVENT), sizeof(MusicalIrcMatchHolder), TRUE,
                                  "scrcmd_musical.c", 1004);
        holder->env = env;
        holder->work = commWork;
        commWork->result = result;
        commWork->ircMatch.gameData = gameData;
        commWork->ircMatch.musicalNetInit = MusComm_GetNetInitData(gameData);
        commWork->ircMatch.netError = 0;
        if (arg == 0) {
            commWork->ircMatch.result = IRCBATTLEMATCH_MODE_MUSICAL_ARG0;
        } else {
            commWork->ircMatch.result = IRCBATTLEMATCH_MODE_MUSICAL_ARG1;
        }
        ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(
                                       gsys, field, OVERLAY_IRCBATTLEMATCH, &IRCBATTLEMATCH_PROC_FUNCTIONS,
                                       &commWork->ircMatch, MusicalIrcMatch_OnReturn, holder));
        return TRUE;
    case MUSICAL_COMM_IS_LOST:
        if (commWork->comm != NULL && lost == TRUE) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MUSICAL_COMM_IS_CONNECTED:
        if (commWork->comm != NULL) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MUSICAL_COMM_CHECK_NET_ERROR:
    case MUSICAL_COMM_CHECK_NET_ERROR_2:
        if (GFL_NetErrCheck()) {
            GSYS_SetNetErrorPending(gsys, 1);
            *result = 1;
        } else if (commWork != NULL && commWork->ircMatch.netError == 1) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case MUSICAL_COMM_NONE:
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Musical_MusicalIsPropOwned(VM *vm, FieldScriptEnv *env) {
    u16 prop = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    MusicalSave *save = getMusicalInfoBlkAddress(gameData);
    *result = MusicalSave_HasProp(save, prop);
    return FALSE;
}

BOOL Scrcmd_Musical_MusicalGetOwnedPropCount(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    MusicalSave *save = getMusicalInfoBlkAddress(gameData);
    *result = MusicalSave_HasAllProps(save);
    return FALSE;
}

void MusicalCommWork_Create(ScriptWork *work, GameData *gameData) {
    MusicalCommWork *commWork =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalCommWork), TRUE, "scrcmd_musical.c", 1138);

    commWork->event = NULL;
    commWork->comm = NULL;
    GameData_SetMusicalCommWork(gameData, commWork);
    GFL_OvlLoad(OVERLAY_ID(210));
    GFL_OvlLoad(OVERLAY_ID(211));
}

void MusicalCommWork_Free(MusicalCommWork *work, GameData *gameData) {
    GFL_OvlUnload(OVERLAY_ID(211));
    GFL_OvlUnload(OVERLAY_ID(210));
    GFL_HeapFree(work);
    GameData_SetMusicalCommWork(gameData, NULL);
}

void MusicalCommWork_BootComm(MusicalCommWork *work, GameSystem *gsys, GameCommSys *comm, u16 value) {
    MusComm_Boot(HEAPID_GAMEEVENT, gsys, comm, value);
}

void MusicalCommWork_QuitComm(MusicalCommWork *work) {
    if (work->comm != NULL) {
        MusComm_Quit(work->comm);
        work->comm = NULL;
    }
}

// The steps of the musical call's event: it runs the show, ends the communication and frees the work
enum {
    MUSICAL_CALL_STEP_CHAIN_EVENT,
    MUSICAL_CALL_STEP_QUIT_COMM,
    MUSICAL_CALL_STEP_WAIT_COMM_END,
    MUSICAL_CALL_STEP_FREE,
};

GameEventReturnCode MusicalCallEvent_Callback(GameEvent *event, u32 *state, void *arg) {
    MusicalCallEvent *data = arg;
    GameData *gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
    GameCommSys *comm;

    switch (*state) {
    case MUSICAL_CALL_STEP_CHAIN_EVENT:
        GameEvent_ChainNext(event, data->event);
        (*state)++;
        break;
    case MUSICAL_CALL_STEP_QUIT_COMM:
        GameData_GetFieldSoundSystem(gameData);
        MusicalCommWork_QuitComm(data->work);
        (*state)++;
        break;
    case MUSICAL_CALL_STEP_WAIT_COMM_END:
        comm = GSYS_GetGameCommSystem(GameEvent_GetGameSystem(event));
        if (data->online && GameCommSys_BootCheck(comm)) {
            break;
        }
        (*state)++;
        break;
    case MUSICAL_CALL_STEP_FREE:
        if (GFL_NetIsEnded() == TRUE || !data->online) {
            if (data->online == TRUE) {
                GSYS_TryBootGameComm(GameEvent_GetGameSystem(event));
            }
            MusicalCommWork_Free(data->work, gameData);
            (*state)++;
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode MusicalDressUpEvent_Callback(GameEvent *event, u32 *state, void *arg) {
    MusicalDressUpEvent *data = arg;
    GameSystem *gsys = GameEvent_GetGameSystem(event);

    GSYS_GetGameData(gsys);
    GSYS_GetField(gsys);
    // States 0 to 2 and 5 only advance the state, so they keep their numbers: nothing tells them apart
    switch (*state) {
    case 0:
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        (*state)++;
        break;
    case 3:
        GSYS_QueueProc(gsys, OVERLAY_NONE, &MUSICAL_DRESSUP_PROC_FUNCTIONS, data->param);
        (*state)++;
        break;
    case 4:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        (*state)++;
        break;
    case 5:
        (*state)++;
        break;
    case 6:
        GFL_HeapFree(data->param->poke);
        GFL_HeapFree(data->param);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void MusicalShotHolder_Free(void *arg) {
    MusicalShotHolder *holder = arg;

    GFL_HeapFree(holder->param);
}

void MusicalIrcMatch_OnReturn(void *arg) {
    MusicalIrcMatchHolder *holder = arg;
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(holder->env);

    FieldScriptEnv_GetGameData(holder->env);
    GSYS_GetGameCommSystem(gsys);
    if (holder->work->ircMatch.result != IRCBATTLEMATCH_RESULT_QUIT &&
        holder->work->ircMatch.result != IRCBATTLEMATCH_RESULT_BACK) {
        *holder->work->result = 1;
        MusComm_RegisterCommands(holder->work->comm);
    } else {
        *holder->work->result = 0;
    }
}

BOOL ScriptNative_MusicalLeaderMenuWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;
    BOOL done;
    u32 choice;

    FieldScriptEnv_GetScriptWork(env);
    commWork = GameData_GetMusicalCommWork(FieldScriptEnv_GetGameData(env));
    choice = CommEntryMenu_Update(commWork->menu);
    done = FALSE;
    switch (choice) {
    case COMM_ENTRY_MENU_DONE:
        *commWork->result = 0;
        MusComm_UpdateRole(commWork->comm);
        done = TRUE;
        break;
    case COMM_ENTRY_MENU_CANCELLED:
        *commWork->result = 1;
        done = TRUE;
        break;
    case COMM_ENTRY_MENU_ERROR:
        *commWork->result = 2;
        done = TRUE;
        break;
    }
    if (done == TRUE) {
        CommEntryMenu_Free(commWork->menu);
        commWork->menu = NULL;
    }
    return done;
}

BOOL ScriptNative_MusicalSelectLeaderMenuWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;
    BOOL done;

    FieldScriptEnv_GetScriptWork(env);
    commWork = GameData_GetMusicalCommWork(FieldScriptEnv_GetGameData(env));
    done = FALSE;
    switch (CommEntryMenu_Update(commWork->menu)) {
    case COMM_ENTRY_MENU_DONE:
        *commWork->result = 3;
        MusComm_UpdateRole(commWork->comm);
        done = TRUE;
        break;
    case COMM_ENTRY_MENU_CANCELLED:
        *commWork->result = 4;
        done = TRUE;
        break;
    case COMM_ENTRY_MENU_ERROR:
        *commWork->result = 2;
        done = TRUE;
        break;
    }
    if (done == TRUE) {
        CommEntryMenu_Free(commWork->menu);
        commWork->menu = NULL;
    }
    return done;
}

BOOL ScriptNative_MusicalTimingWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    commWork = GameData_GetMusicalCommWork(FieldScriptEnv_GetGameData(env));
    if (commWork->event != NULL) {
        MusicalEvent_CheckNetError(commWork->event);
        if (MusicalEvent_IsNetError(commWork->event) == TRUE) {
            return TRUE;
        }
    }
    if (MusComm_IsTimingReached(commWork->comm, commWork->value) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL ScriptNative_MusicalScriptRecvWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    commWork = GameData_GetMusicalCommWork(FieldScriptEnv_GetGameData(env));
    MusicalEvent_CheckNetError(commWork->event);
    if (MusicalEvent_IsNetError(commWork->event) == TRUE) {
        return TRUE;
    }
    if (MusComm_IsScriptReceived(commWork->comm) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL ScriptNative_MusicalCommBootWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    GameSystem *gsys;
    GameData *gameData;
    GameCommSys *comm;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    comm = GSYS_GetGameCommSystem(gsys);
    commWork = GameData_GetMusicalCommWork(gameData);
    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_MUSICAL) {
        commWork->comm = GameCommSys_GetWork(comm);
        return TRUE;
    }
    return FALSE;
}

BOOL ScriptNative_MusicalCommEndWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    GameSystem *gsys;
    GameData *gameData;
    GameCommSys *comm;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    comm = GSYS_GetGameCommSystem(gsys);
    commWork = GameData_GetMusicalCommWork(gameData);
    if (commWork->event != NULL) {
        MusicalEvent_CheckNetError(commWork->event);
        if (MusicalEvent_IsNetError(commWork->event) == TRUE) {
            return TRUE;
        }
    }
    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL ScriptNative_MusicalPokesRecvWait(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    MusicalCommWork *commWork;

    FieldScriptEnv_GetScriptWork(env);
    commWork = GameData_GetMusicalCommWork(FieldScriptEnv_GetGameData(env));
    MusicalEvent_CheckNetError(commWork->event);
    if (MusicalEvent_IsNetError(commWork->event) == TRUE) {
        return TRUE;
    }
    if (MusComm_AreAllPokesReceived(commWork->comm) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

// The items sent from the Dream World
BOOL Scrcmd_Musical_DreamWorldItems(VM *vm, FieldScriptEnv *env) {
    u16 kind = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(GameData_GetSaveControl(gameData));
    s32 count = DreamWorldSave_CountItems(dreamWorld);
    GameEvent *event;

    if (GSYS_GetField(gsys) == NULL && kind != DREAM_WORLD_ITEMS_HAS_ITEMS) {
        return FALSE;
    }
    switch (kind) {
    case DREAM_WORLD_ITEMS_HAS_ITEMS:
        *result = DreamWorldSave_CountItems(dreamWorld) > 0 ? TRUE : FALSE;
        break;
    case DREAM_WORLD_ITEMS_HAS_UNRECEIVABLE:
        if (PdwPostman_CountUnreceivable(gameData, heapId, dreamWorld) < count) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    case DREAM_WORLD_ITEMS_OPEN_LIST:
        event = PdwPostman_CreateListEvent(gsys);
        ScriptWork_CallEvent(FieldScriptEnv_GetScriptWork(env), event);
        return TRUE;
    case DREAM_WORLD_ITEMS_RECEIVE:
        PdwPostman_ReceiveItems(gameData, heapId, dreamWorld);
        break;
    case DREAM_WORLD_ITEMS_ITEM_NAME:
        loadItemsNameToStrbuf(ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env)), 0,
                              PdwPostman_GetUnreceivableItem(gameData, heapId, dreamWorld, *result));
        break;
    case DREAM_WORLD_ITEMS_COUNT_UNRECEIVABLE:
        *result = PdwPostman_CountUnreceivable(gameData, heapId, dreamWorld);
        break;
    }
    return FALSE;
}
