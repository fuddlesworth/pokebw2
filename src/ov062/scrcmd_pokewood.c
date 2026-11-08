#include "types.h"
#include "battle/btl_setup.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/obj_codes.h"
#include "constants/text_banks.h"
#include "constants/trainer_classes.h"
#include "field/event_pokewood.h"
#include "field/field_script.h"
#include "field/pokewood_system.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "save/pokewood.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The script plugin of Pokéstar Studios (plugin 10), commands from 1000

#define POKEWOOD_MSG_FILE TEXT_BANK_POKEWOOD_LINES

enum {
    POKEWOOD_MOVIE_FIELD_DIRECTOR = 0,
    POKEWOOD_MOVIE_FIELD_MALE_FILM_TRAINER_CLASS = 1,
    POKEWOOD_MOVIE_FIELD_FEMALE_FILM_TRAINER_CLASS = 2,
    POKEWOOD_MOVIE_FIELD_SCORE_MULTIPLIER = 3,
};

enum {
    POKEWOOD_UNLOCK_NOTICE_STANDARD = 0,
    POKEWOOD_UNLOCK_NOTICE_EVENT_FLAG = 1,
    POKEWOOD_UNLOCK_NOTICE_MIN_GRADE = 2,
    POKEWOOD_UNLOCK_NOTICE_HIGH_SCORE = 3,
    POKEWOOD_UNLOCK_NOTICE_ALL_PREVIOUS_HITS = 4,
    POKEWOOD_UNLOCK_NOTICE_FIRST_MOVIE_WATCHED = 5,
    POKEWOOD_UNLOCK_NOTICE_HIT_AND_EVENT_FLAG = 6,
};

#define POKEWOOD_FAN_GIFT_GROUP_NONE 0xff

static u16 Pokewood_GetTrainerObjCode(u16 trainerClass) {
    static const struct {
        u16 trainerClass;
        u16 objCode;
    } sTrainerObjCodes[] = {
        { TRAINER_CLASS_GENTLEMAN, OBJ_CODE_GENTLEMAN },
        { TRAINER_CLASS_SCIENTIST_F_46, OBJ_CODE_SCIENTIST_F_46 },
        { TRAINER_CLASS_SABRINA, OBJ_CODE_256 },
        { TRAINER_CLASS_SCIENTIST_M_52, OBJ_CODE_SCIENTIST_M_52 },
        { TRAINER_CLASS_GUY_IN_SUIT_223, OBJ_CODE_346 },
        { TRAINER_CLASS_MAID, OBJ_CODE_65 },
        { TRAINER_CLASS_BRYCEN, OBJ_CODE_91 },
        { TRAINER_CLASS_ACE_TRAINER_M, OBJ_CODE_ACE_TRAINER_M },
        { TRAINER_CLASS_ACE_TRAINER_F, OBJ_CODE_ACE_TRAINER_F },
        { TRAINER_CLASS_JANITOR, OBJ_CODE_JANITOR },
        { TRAINER_CLASS_CLERK_F, OBJ_CODE_CLERK_F },
        { TRAINER_CLASS_GIRL_IN_SUIT_224, OBJ_CODE_346 },
        { TRAINER_CLASS_PRESCHOOLER_F, OBJ_CODE_PRESCHOOLER_F },
        { TRAINER_CLASS_CLERK_M_67, OBJ_CODE_CLERK_M_67 },
        { TRAINER_CLASS_WORKER_26, OBJ_CODE_WORKER_26 },
    };
    u32 i;

    for (i = 0; i < NELEMS(sTrainerObjCodes); i++) {
        if (trainerClass == sTrainerObjCodes[i].trainerClass) {
            return sTrainerObjCodes[i].objCode;
        }
    }
    return OBJ_CODE_YOUNGSTER;
}

static u16 Pokewood_GetDirectorObjCode(u8 director) {
    static const u16 sDirectorObjCodes[] = { OBJ_CODE_ARTIST, OBJ_CODE_83, OBJ_CODE_25, OBJ_CODE_GENTLEMAN,
                                             OBJ_CODE_303 };

    if (director >= NELEMS(sDirectorObjCodes)) {
        director = 0;
    }
    return sDirectorObjCodes[director];
}

static PokewoodMovie *Pokewood_LoadMovie(HeapID heapId, u32 movie) {
    return GFL_ArcSysReadHeapNew(ARCID_POKEWOOD_MOVIE, movie, heapId);
}

static u16 Pokewood_GetMovieField(u32 movie, u32 field) {
    u16 value = 0;
    PokewoodMovie *data = Pokewood_LoadMovie(HEAPID_GAMEEVENT, movie);

    switch (field) {
    case POKEWOOD_MOVIE_FIELD_DIRECTOR:
        value = data->director;
        break;
    case POKEWOOD_MOVIE_FIELD_MALE_FILM_TRAINER_CLASS:
        value = data->maleFilmTrainerClass;
        break;
    case POKEWOOD_MOVIE_FIELD_FEMALE_FILM_TRAINER_CLASS:
        value = data->femaleFilmTrainerClass;
        break;
    case POKEWOOD_MOVIE_FIELD_SCORE_MULTIPLIER:
        value = data->scoreMultiplier;
        break;
    }
    GFL_HeapFree(data);
    return value;
}

static PokewoodSave *Pokewood_GetSave(FieldScriptEnv *env) {
    return PokewoodSave_Get(FieldScriptEnv_GetGameData(env));
}

static PokewoodSystem *Pokewood_GetSystem(FieldScriptEnv *env) {
    return *GameData_GetPokewoodSystemPtr(FieldScriptEnv_GetGameData(env));
}

static BOOL Scrcmd_Pokewood_Create(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokewoodSave *save = PokewoodSave_Get(gameData);
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(gameData);

    *sys = PokewoodSystem_Create(HEAPID_GAMEEVENT);
    GFL_OvlLoad(OVERLAY_EVENT_POKEWOOD);
    EventPokewood_UnlockMovies(save, gameData, 0, 0, 0);
    GFL_OvlUnload(OVERLAY_EVENT_POKEWOOD);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_Free(VM *vm, FieldScriptEnv *env) {
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(FieldScriptEnv_GetGameData(env));

    PokewoodSystem_Free(*sys);
    *sys = NULL;
    return FALSE;
}

static BOOL Scrcmd_Pokewood_Film(VM *vm, FieldScriptEnv *env) {
    PokewoodSystem *sys = Pokewood_GetSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u32 args[2];

    args[0] = ScriptReadAny(vm, env);
    args[1] = ScriptReadAny(vm, env);
    PokewoodSystem_SetResultVar(sys, ScriptReadVar(vm, env));
    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateFilmEvent, args));
    return TRUE;
}

static BOOL Scrcmd_Pokewood_SelectCast(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateCastSelectEvent, sys));
    PokewoodSystem_SetResultVar(sys, var);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_SetUsePlayerCast(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    PokewoodSystem_SetUsePlayerCast(sys, ScriptReadAny(vm, env));
    return TRUE;
}

static BOOL Scrcmd_Pokewood_WatchSlot(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    u32 arg = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateWatchSlotEvent, &arg));
    PokewoodSystem_SetResultVar(sys, var);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_WatchBlock(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateWatchBlockEvent, NULL));
    PokewoodSystem_SetResultVar(sys, NULL);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_PickMovie(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(gameData);
    PkwdBookParam *param = PokewoodSetup_CreateBookParam(heapId, gsys, var);
    GameEvent *event =
        GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateMoviePickerEvent, param);

    PokewoodSetup_FreeBookParam(param);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_OpenMenu(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 mode = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    PkwdSelectScreenParam *param = PokewoodSetup_CreateMenuParam(heapId, gsys, mode, var);
    GameEvent *event =
        GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateMenuEvent, param);

    PokewoodSetup_FreeMenuParam(param);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Whether the downloaded movie of a slot loads
static BOOL Pokewood_CheckDownloadedMovie(FieldScriptEnv *env, u32 slot) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BOOL loads = FALSE;

    allocatePokewoodBlk(heapId);
    if (Pokewood_LoadDownloadedMovie(save, heapId, slot) == 1 && Pokewood_IsLoadedMovieValid() == TRUE) {
        loads = TRUE;
    }
    freeAndClearPokewoodBlk();
    return loads;
}

// How many slots have a downloaded movie that loads
static u32 Pokewood_CountDownloadedMovies(FieldScriptEnv *env) {
    u32 slot;
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u32 count = 0;

    allocatePokewoodBlk(heapId);
    for (slot = 0; slot < 8; slot++) {
        if (Pokewood_LoadDownloadedMovie(save, heapId, slot) == 1 && Pokewood_IsLoadedMovieValid() == TRUE) {
            count++;
        }
    }
    freeAndClearPokewoodBlk();
    return count;
}

// A field of the downloaded movie of a slot, and 0 when it doesn't load
static u32 Pokewood_GetDownloadedMovieField(FieldScriptEnv *env, u32 slot, u32 field, u32 fieldArg) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u32 value;

    allocatePokewoodBlk(heapId);
    if (Pokewood_LoadDownloadedMovie(save, heapId, slot) == 1) {
        value = Pokewood_GetLoadedMovieField(field, fieldArg);
    } else {
        value = 0;
    }
    freeAndClearPokewoodBlk();
    return value;
}

static BOOL Scrcmd_Pokewood_CheckDownloadedMovie(VM *vm, FieldScriptEnv *env) {
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_CheckDownloadedMovie(env, slot);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetDownloadedMovieResult(VM *vm, FieldScriptEnv *env) {
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    switch (Pokewood_GetDownloadedMovieField(env, slot, POKEWOOD_BLOCK_FIELD_ENDING, 0)) {
    case STUDIO_ENDING_FLOP:
    default:
        *var = STUDIO_ENDING_FLOP;
        break;
    case STUDIO_ENDING_HIT:
        *var = STUDIO_ENDING_HIT;
        break;
    case STUDIO_ENDING_STRANGE:
        *var = STUDIO_ENDING_STRANGE;
        break;
    }
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetMessageWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);
    u16 message = ScriptReadAny(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u32 msgId = message + 42;
    StrBuf *strbuf = ScriptWork_GetAltStrBuf(work);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, POKEWOOD_MSG_FILE, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, msgId, strbuf);
        GFL_MsgDataFree(msgData);
    }
    GFL_WordSetSetStrbuf(wordSet, index, strbuf, 2, 1, 2);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetMovieFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 list = ScriptReadAny(vm, env);
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = PokewoodSave_CheckFlag(save, list, movie);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetMovieFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 list = ScriptReadAny(vm, env);
    u16 movie = ScriptReadAny(vm, env);

    PokewoodSave_SetFlag(save, list, movie);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_CountMovieFlags(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 list = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = PokewoodSave_CountFlags(save, list);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_CountDownloadedMovies(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_CountDownloadedMovies(env);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetNewUnlockKind(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    u16 *doneVar = ScriptReadVar(vm, env);

    *doneVar = FALSE;
    if (PokewoodSave_CheckFlag(save, POKEWOOD_FLAG_LIST_UNLOCKED, movie) == TRUE &&
        PokewoodSave_CheckFlag(save, POKEWOOD_FLAG_LIST_ANNOUNCED, movie) == FALSE) {
        PokewoodSave_SetFlag(save, POKEWOOD_FLAG_LIST_ANNOUNCED, movie);
        GFL_OvlLoad(OVERLAY_EVENT_POKEWOOD);
        switch (EventPokewood_GetUnlockKind(movie)) {
        case POKEWOOD_UNLOCK_NONE:
        case POKEWOOD_UNLOCK_PREVIOUS_HIT:
        default:
            *var = POKEWOOD_UNLOCK_NOTICE_STANDARD;
            break;
        case POKEWOOD_UNLOCK_EVENT_FLAG:
            *var = POKEWOOD_UNLOCK_NOTICE_EVENT_FLAG;
            break;
        case POKEWOOD_UNLOCK_MIN_GRADE:
            *var = POKEWOOD_UNLOCK_NOTICE_MIN_GRADE;
            break;
        case POKEWOOD_UNLOCK_HIGH_SCORE:
            *var = POKEWOOD_UNLOCK_NOTICE_HIGH_SCORE;
            break;
        case POKEWOOD_UNLOCK_ALL_PREVIOUS_HITS:
            *var = POKEWOOD_UNLOCK_NOTICE_ALL_PREVIOUS_HITS;
            break;
        case POKEWOOD_UNLOCK_FIRST_MOVIE_WATCHED:
            *var = POKEWOOD_UNLOCK_NOTICE_FIRST_MOVIE_WATCHED;
            break;
        case POKEWOOD_UNLOCK_HIT_AND_EVENT_FLAG:
            *var = POKEWOOD_UNLOCK_NOTICE_HIT_AND_EVENT_FLAG;
            break;
        }
        GFL_OvlUnload(OVERLAY_EVENT_POKEWOOD);
        *doneVar = TRUE;
    }
    return FALSE;
}

static BOOL Scrcmd_Pokewood_TestAndSetLevelBit(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSave *save = Pokewood_GetSave(env);
    u32 level = PokewoodSave_GetLevelOfPoints(PokewoodSave_GetLevelPoints(save));

    *var = PokewoodSave_GetLevelBit(save, level);
    PokewoodSave_SetLevelBit(save, level, 1);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetSlotFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = PokewoodSave_IsSlotWatched(save, slot);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetSlotFlag(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);

    PokewoodSave_SetSlotWatched(save, ScriptReadAny(vm, env), 1);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetDownloadedMovieStudioCast(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 slot = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_GetDownloadedMovieField(env, slot, POKEWOOD_BLOCK_FIELD_STUDIO_CAST, 0);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_GetMovieActorObjCodes(VM *vm, FieldScriptEnv *env) {
    u8 playerGender = getTrainerGender(GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env)));
    u16 movie = ScriptReadAny(vm, env);
    u16 *trainerObjCodeVar = ScriptReadVar(vm, env);
    u16 *directorObjCodeVar = ScriptReadVar(vm, env);

    *trainerObjCodeVar = Pokewood_GetTrainerObjCode(
        Pokewood_GetMovieField(movie, playerGender == GENDER_MALE ? POKEWOOD_MOVIE_FIELD_MALE_FILM_TRAINER_CLASS
                                                                  : POKEWOOD_MOVIE_FIELD_FEMALE_FILM_TRAINER_CLASS));
    *directorObjCodeVar = Pokewood_GetDirectorObjCode(Pokewood_GetMovieField(movie, POKEWOOD_MOVIE_FIELD_DIRECTOR));
    return FALSE;
}

static BOOL Scrcmd_Pokewood_SetSaveSlot(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokewoodSystem *sys = Pokewood_GetSystem(env);

    PokewoodSystem_SetSaveSlot(sys, ScriptReadAny(vm, env));
    return TRUE;
}

static BOOL Scrcmd_Pokewood_GetMovieDirector(VM *vm, FieldScriptEnv *env) {
    u16 movie = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = Pokewood_GetMovieField(movie, POKEWOOD_MOVIE_FIELD_DIRECTOR);
    return FALSE;
}

static BOOL Scrcmd_Pokewood_HasAllHitEndings(VM *vm, FieldScriptEnv *env) {
    PokewoodSave *save = Pokewood_GetSave(env);
    u16 *var = ScriptReadVar(vm, env);

    if (PokewoodSave_CountFlags(save, POKEWOOD_FLAG_LIST_HIT_ENDING) == POKEWOOD_MOVIE_COUNT) {
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

static BOOL Scrcmd_Pokewood_OpenRecordMonitor(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 *var = ScriptReadVar(vm, env);
    PokewoodSystem **sys = GameData_GetPokewoodSystemPtr(gameData);
    PkwdRecordMonitorParam *param = PokewoodSetup_CreateRecordMonitorParam(heapId, gsys);
    GameEvent *event =
        GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_POKEWOOD, EventPokewood_CreateScreenEvent, param);

    PokewoodSetup_FreeRecordMonitorParam(param);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Pokewood_GetFanGift(VM *vm, FieldScriptEnv *env) {
    static const u16 sGiftColumns[][3] = { { 3, 2, 1 }, { 3, 2, 1 }, { 3, 2, 0 }, { 2, 1, 0 } };
    static const u16 sGiftItems[][4] = {
        { ITEM_PRETTY_WING, ITEM_LAVA_COOKIE, ITEM_LEMONADE, ITEM_FRESH_WATER },
        { ITEM_MAX_REVIVE, ITEM_HYPER_POTION, ITEM_BERRY_JUICE, ITEM_POTION },
        { ITEM_NUGGET, ITEM_MOOMOO_MILK, ITEM_ESCAPE_ROPE, ITEM_FAVORED_MAIL },
        { ITEM_STAR_PIECE, ITEM_OLD_GATEAU, ITEM_RAGE_CANDY_BAR, ITEM_SWEET_HEART },
        { ITEM_FULL_RESTORE, ITEM_BIG_PEARL, ITEM_ENERGY_ROOT, ITEM_ENERGY_POWDER },
    };
    static const u16 sGiftQuantities[][4] = {
        { 1, 5, 5, 1 }, { 1, 5, 1, 1 }, { 1, 5, 1, 1 }, { 1, 5, 1, 1 }, { 1, 1, 1, 1 },
    };
    static const u16 sGiftGroups[] = {
        0,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        1,
        2,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        2,
        2,
        3,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        3,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        3,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        3,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        POKEWOOD_FAN_GIFT_GROUP_NONE,
        4,
        4,
    };
    u16 slot = ScriptReadAny(vm, env);
    u16 fanIndex = ScriptReadAny(vm, env);
    u16 *itemVar = ScriptReadVar(vm, env);
    u16 *quantityVar = ScriptReadVar(vm, env);
    u16 ending = Pokewood_GetDownloadedMovieField(env, slot, POKEWOOD_BLOCK_FIELD_ENDING, 0);
    u16 multiplierIndex =
        Pokewood_GetMovieField((u16)Pokewood_GetDownloadedMovieField(env, slot, POKEWOOD_BLOCK_FIELD_MOVIE, 0),
                               POKEWOOD_MOVIE_FIELD_SCORE_MULTIPLIER) /
            2 -
        1;
    u16 column = sGiftColumns[multiplierIndex][ending];
    u16 row = sGiftGroups[fanIndex];

    if (row != POKEWOOD_FAN_GIFT_GROUP_NONE) {
        *itemVar = sGiftItems[row][column];
        *quantityVar = sGiftQuantities[row][column];
    } else {
        *itemVar = ITEM_NONE;
        *quantityVar = 0;
    }
    return FALSE;
}

const FieldScriptCommand POKEWOOD_SCRIPT_COMMANDS[] = {
    Scrcmd_Pokewood_Film,
    Scrcmd_Pokewood_WatchSlot,
    Scrcmd_Pokewood_PickMovie,
    Scrcmd_Pokewood_OpenMenu,
    Scrcmd_Pokewood_SetSaveSlot,
    Scrcmd_Pokewood_SetMessageWord,
    Scrcmd_Pokewood_GetMovieFlag,
    Scrcmd_Pokewood_SetMovieFlag,
    Scrcmd_Pokewood_CountDownloadedMovies,
    Scrcmd_Pokewood_CheckDownloadedMovie,
    Scrcmd_Pokewood_GetDownloadedMovieResult,
    Scrcmd_Pokewood_Create,
    Scrcmd_Pokewood_Free,
    Scrcmd_Pokewood_SelectCast,
    Scrcmd_Pokewood_SetUsePlayerCast,
    Scrcmd_Pokewood_GetNewUnlockKind,
    Scrcmd_Pokewood_TestAndSetLevelBit,
    Scrcmd_Pokewood_GetSlotFlag,
    Scrcmd_Pokewood_GetDownloadedMovieStudioCast,
    Scrcmd_Pokewood_GetMovieActorObjCodes,
    Scrcmd_Pokewood_GetMovieDirector,
    Scrcmd_Pokewood_HasAllHitEndings,
    Scrcmd_Pokewood_SetSlotFlag,
    Scrcmd_Pokewood_OpenRecordMonitor,
    Scrcmd_Pokewood_GetFanGift,
    Scrcmd_Pokewood_CountMovieFlags,
    Scrcmd_Pokewood_WatchBlock,
    (FieldScriptCommand)0xFFFFFFFF,
};
