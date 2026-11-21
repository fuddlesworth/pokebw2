// The Battle Subway's events: picking the Pokémon to enter, the battle, the trainers' and leaders' messages, and its
// records screen. The name is descriptive
#include "types.h"
#include "app/bsubway_records.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "battle/battle_proc.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/bsubway_scr.h"
#include "field/encounter.h"
#include "field/event_battle.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_event.h"
#include "field/field_msgbg.h"
#include "field/scrcmd_msg.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/bsubway_save.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/pms.h"

#define SEQ_SE_MESSAGE 0x547
// The timing the partners synchronize at before the battle
#define BSUBWAY_COMM_TIMING_BATTLE 200
// The number of messages TEXT_BANK_BATTLE_SUBWAY_TRAINER_MESSAGES holds: the train's Trainers' messages (942)
#define BSUBWAY_TRAINER_MESSAGE_COUNT 0x3ae

typedef struct {
    GameSystem *gsys;
    Field *field;
    PStatusParam summaryParam;
    PokeListParam partyParam;
    u16 *result;
    u16 *choice;
    u8 *picked;
    u32 unkE4;
} BSubwayPokeSelectData;

typedef struct {
    GameSystem *gsys;
    u32 unk04;
    VecFx32 pos;
    ActorMsgWin *msgWin;
    StrBuf *strbuf;
    u16 actorId;
    u8 unk1E[0xca];
} BSubwayTrainerMsgData;

typedef struct {
    Field *field;
    GameSystem *gsys;
    BSubwayRecordsParam param;
} BSubwayRecordsData;

typedef struct {
    GameProcManager *procManager;
} BSubwayCommBattleWork;

typedef struct {
    GameData *gameData;
    BtlSetup *setup;
    void *nameInputEncounterWork;
} BSubwayCommBattleParam;

typedef struct {
    GameSystem *gsys;
    BtlSetup *setup;
    void *nameInputEncounterWork;
    BSubwayCommBattleParam param;
    u8 unk18[0x24];
} BSubwayCommBattleData;

typedef struct {
    GameSystem *gsys;
    Field *field;
    BtlSetup *setup;
    void *nameInputEncounterWork;
} BSubwayBattleData;

typedef struct {
    GameSystem *gsys;
    u32 unk04;
    VecFx32 pos;
    ActorMsgWin *msgWin;
    StrBuf *strbuf;
} BSubwayLeaderMsgData;

static BOOL BSubwayCommBattleProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BSubwayCommBattleProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BSubwayCommBattleProc_Main(GameProc *proc, u32 *state, void *param, void *work);

static const GameProcFunctions BSUBWAY_COMM_BATTLE_PROC_FUNCTIONS = {
    BSubwayCommBattleProc_Init,
    BSubwayCommBattleProc_Main,
    BSubwayCommBattleProc_Exit,
};

static GameEvent *EventBSubwayCommBattle_CreateFromScrWork(BSubwayScrWork *bsw, GameSystem *gsys, Field *field);

// EventBSubwayPokeSelect_Callback's steps: the field's fade out and close, the party screen, the field's opening and
// fade in, which read the picks and result, and the end
enum {
    BSUBWAY_POKE_SELECT_STEP_FADE_OUT,
    BSUBWAY_POKE_SELECT_STEP_CLOSE_FIELD,
    BSUBWAY_POKE_SELECT_STEP_POKE_LIST,
    BSUBWAY_POKE_SELECT_STEP_OPEN_FIELD,
    BSUBWAY_POKE_SELECT_STEP_FADE_IN,
    BSUBWAY_POKE_SELECT_STEP_END,
};

static GameEventReturnCode EventBSubwayPokeSelect_Callback(GameEvent *event, u32 *state, void *work) {
    BSubwayPokeSelectData *data = work;
    GameSystem *gsys = data->gsys;

    switch (*state) {
    case BSUBWAY_POKE_SELECT_STEP_FADE_OUT:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceOutTransitionDefault(gsys, data->field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case BSUBWAY_POKE_SELECT_STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, data->field));
        (*state)++;
        break;
    case BSUBWAY_POKE_SELECT_STEP_POKE_LIST:
        GameEvent_ChainNext(event, EventPokeList_Create(gsys, data->field, &data->partyParam, &data->summaryParam));
        (*state)++;
        break;
    case BSUBWAY_POKE_SELECT_STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case BSUBWAY_POKE_SELECT_STEP_FADE_IN:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, data->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        MI_CpuCopy8(data->partyParam.picked, data->picked, sizeof(data->partyParam.picked));
        *data->choice = data->partyParam.index;
        *data->result = data->partyParam.result;
        GFL_HeapFree(data->partyParam.regulation);
        (*state)++;
        break;
    case BSUBWAY_POKE_SELECT_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static inline void InitSummaryParam(BSubwayPokeSelectData *data, PokeParty *party, GameData *gameData) {
    PStatusParam *param = &data->summaryParam;
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);

    MI_CpuFill8(param, 0, sizeof(PStatusParam));
    param->party = party;
    param->dataType = PSTATUS_DATA_PARTY;
    param->partyCount = PokeParty_GetPkmCount(party);
    param->mode = PSTATUS_MODE_NORMAL;
    param->page = PSTATUS_PAGE_INFO;
    param->gameData = gameData;
    param->isNationalDex = PokeDex_IsNationalObtained(pokedex);
}

GameEvent *EventBSubwayPokeSelect_Create(BSubwayScrWork *bsw, GameSystem *gsys, u8 rental) {
    Field *field;
    GameEvent *event;
    GameData *gameData;
    BSubwayPokeSelectData *data;
    PokeParty *party;
    u32 regulationId;
    PokeListParam *partyParam;
    u32 mode;

    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    event = GameEvent_Create(gsys, NULL, EventBSubwayPokeSelect_Callback, sizeof(BSubwayPokeSelectData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->picked = bsw->memberChoices;
    data->choice = &bsw->pokeSelectIndex;
    data->result = &bsw->pokeSelectResult;
    if (rental == FALSE) {
        party = GameData_GetParty(gameData);
    } else {
        party = bsw->allocatedBuffer;
    }
    mode = 0;
    regulationId = 20;
    partyParam = &data->partyParam;
    switch (bsw->playMode) {
    case BSUBWAY_MODE_DOUBLE:
    case BSUBWAY_MODE_SUPER_DOUBLE:
        regulationId = 21;
        mode = 1;
        break;
    case BSUBWAY_MODE_MULTI:
    case BSUBWAY_MODE_COMM_MULTI:
    case BSUBWAY_MODE_SUPER_MULTI:
    case BSUBWAY_MODE_SUPER_COMM_MULTI:
        regulationId = 22;
        mode = 2;
        break;
    }
    PokeListParam_Setup(partyParam, gameData, POKELIST_MODE_BATTLE_TEAM_CANCEL, party);
    partyParam->regulation = PokeRegulation_LoadNew(regulationId, HEAPID_GAMEEVENT);
    partyParam->unk48 = mode;
    InitSummaryParam(data, party, gameData);
    return event;
}

GameEvent *EventBSubwayBattle_Create(BSubwayScrWork *bsw, GameSystem *gsys, Field *field) {
    BattleRec_Init(HEAPID_GAMEEVENT);
    bsw->btlSetup = BsubwayScr_CreateBtlSetup(bsw, gsys);
    if (bsw->playMode == BSUBWAY_MODE_COMM_MULTI || bsw->playMode == BSUBWAY_MODE_SUPER_COMM_MULTI) {
        return EventBSubwayCommBattle_CreateFromScrWork(bsw, gsys, field);
    }
    return EventBSubwayBattleCall_Create(gsys, field, bsw->btlSetup);
}

// The steps of the trainer's and the leader's message events: the message, the input that closes it, and its close
enum {
    BSUBWAY_MSG_STEP_PRINT,
    BSUBWAY_MSG_STEP_WAIT_INPUT,
    BSUBWAY_MSG_STEP_WAIT_CLOSE,
};

static GameEventReturnCode EventBSubwayTrainerMsg_Callback(GameEvent *event, u32 *state, void *work) {
    BSubwayTrainerMsgData *data = work;

    switch (*state) {
    case BSUBWAY_MSG_STEP_PRINT:
        if (!MsgBGPrinter_TalkWinMain(data->msgWin)) {
            break;
        }
        (*state)++;
        // fallthrough
    case BSUBWAY_MSG_STEP_WAIT_INPUT:
        MsgBGPrinter_TalkWinDrawCursor(data->msgWin);
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            MsgBGPrinter_DeleteTalkWin(data->msgWin);
            (*state)++;
        }
        break;
    case BSUBWAY_MSG_STEP_WAIT_CLOSE:
        if (MsgBGPrinter_WaitTalkWinClose(data->msgWin) == TRUE) {
            GFL_StrBufFree(data->strbuf);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBSubwayTrainerMsg_Create(BSubwayScrWork *bsw, GameSystem *gsys, int index, u16 actorId, u8 winPos) {
    GameEvent *event;
    GameData *gameData;
    FieldMsgBG *msgBGSys;
    Field *field;
    BSubwayTrainerMsgData *data;
    PMSData *message;
    MsgData *msgData;
    u16 messageId;
    FieldCamera *camera;
    VecFx32 offset;
    u32 winX;
    u32 winY;

    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    msgBGSys = Field_GetMsgBGSys(field);
    event = GameEvent_Create(gsys, NULL, EventBSubwayTrainerMsg_Callback, sizeof(BSubwayTrainerMsgData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->actorId = actorId;
    message = &bsw->trainers[index].message;
    if (message->type == PMS_WORD_NULL) {
        data->strbuf = GFL_StrBufCreate(0x300, HEAPID_GAMEEVENT);
        msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BATTLE_SUBWAY_TRAINER_MESSAGES, HEAPID_GAMEEVENT);
        messageId = bsw->trainers[index].message.id;
        if (messageId >= BSUBWAY_TRAINER_MESSAGE_COUNT) {
            messageId = 0;
        }
        GFL_MsgDataLoadStrbuf(msgData, messageId, data->strbuf);
        GFL_MsgDataFree(msgData);
    } else {
        data->strbuf = PMSData_ToString(message, HEAPID_GAMEEVENT);
    }
    CopyActorWPos(FindFieldActor(GameData_GetMMSys(gameData), data->actorId), &data->pos);
    camera = Field_GetCameraSystem(field);
    ActorMsgWin_GetBalloonOffset(&data->pos, &offset, FieldCamera_GetG3DCamera(camera), camera, winPos);
    data->pos.x += offset.x;
    data->pos.y += offset.y;
    data->pos.z += offset.z;
    ActorMsgWin_GetBalloonTailParams(winPos, &winX, &winY);
    data->msgWin = ActorMsgWin_CheckAndCreate(msgBGSys, winX, &data->pos, data->strbuf, 0, winY);
    return event;
}

// EventBSubwayRecords_Callback's steps: the records screen, and its end
enum {
    BSUBWAY_RECORDS_STEP_SCREEN,
    BSUBWAY_RECORDS_STEP_END,
};

static GameEventReturnCode EventBSubwayRecords_Callback(GameEvent *event, u32 *state, void *work) {
    BSubwayRecordsData *data = work;
    void *bsubwaySave;

    switch (*state) {
    case BSUBWAY_RECORDS_STEP_SCREEN:
        data->param.gameData = GSYS_GetGameData(data->gsys);
        bsubwaySave = GameData_GetBSubwayWifiData(data->param.gameData);
        data->param.rank = BSubwayWifiData_GetLeaderRank(bsubwaySave);
        data->param.trainNumber = BSubwayWifiData_GetLeaderRoom(bsubwaySave);
        GameEvent_ChainNext(event,
                            EventFieldSubprocessTransition_Create(data->gsys, data->field, OVERLAY_ID(141),
                                                                  &BSUBWAY_RECORDS_PROC_FUNCTIONS, &data->param));
        (*state)++;
        break;
    case BSUBWAY_RECORDS_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBSubwayRecords_Create(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBSubwayRecords_Callback, sizeof(BSubwayRecordsData));
    BSubwayRecordsData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    return event;
}

static BOOL BSubwayCommBattleProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BSubwayCommBattleWork *procWork;

    GFL_HeapCreateChild(HEAPID_USER, 0x76, 0x4000);
    procWork = GFL_ProcInitSubsystem(proc, sizeof(BSubwayCommBattleWork), 0x76);
    procWork->procManager = CreateGameProcManager(0x76);
    return TRUE;
}

static BOOL BSubwayCommBattleProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BSubwayCommBattleWork *procWork = work;

    FreeGameProcManager(procWork->procManager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(0x76);
    return TRUE;
}

// BSubwayCommBattleProc_Main's steps: overlay 167 and its commands, the timing the partners meet at, the battle,
// and the overlay's unload
enum {
    BSUBWAY_COMM_BATTLE_PROC_STEP_LOAD,
    BSUBWAY_COMM_BATTLE_PROC_STEP_WAIT_TIMING,
    BSUBWAY_COMM_BATTLE_PROC_STEP_BATTLE,
    BSUBWAY_COMM_BATTLE_PROC_STEP_WAIT_BATTLE,
    BSUBWAY_COMM_BATTLE_PROC_STEP_UNLOAD,
    BSUBWAY_COMM_BATTLE_PROC_STEP_END,
};

static BOOL BSubwayCommBattleProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BSubwayCommBattleParam *battleParam = param;
    BSubwayCommBattleWork *procWork = work;
    BOOL running = GFL_ProcMgrUpdate(procWork->procManager);

    switch (*state) {
    case BSUBWAY_COMM_BATTLE_PROC_STEP_LOAD:
        GFL_OvlLoad(OVERLAY_ID(167));
        NetCommand_AddTable(0x100, BTL_NET_COMMANDS, 9, NULL);
        GFL_NetSendTiming(NetHandle_GetOwn(), BSUBWAY_COMM_TIMING_BATTLE);
        *state = 1;
        break;
    case BSUBWAY_COMM_BATTLE_PROC_STEP_WAIT_TIMING:
        if (GFL_NetErrCheck()) {
            *state = 4;
        } else if (GFL_NetIsTimingRecv(NetHandle_GetOwn(), BSUBWAY_COMM_TIMING_BATTLE)) {
            *state = 2;
        }
        break;
    case BSUBWAY_COMM_BATTLE_PROC_STEP_BATTLE:
        QueueGameProc(procWork->procManager, -1, &BATTLE_PROC_FUNCTIONS, battleParam->setup);
        *state = 3;
        break;
    case BSUBWAY_COMM_BATTLE_PROC_STEP_WAIT_BATTLE:
        if (running != TRUE) {
            *state = 4;
        }
        break;
    case BSUBWAY_COMM_BATTLE_PROC_STEP_UNLOAD:
        NetCommand_RemoveTable(0x100);
        GFL_OvlUnload(OVERLAY_ID(167));
        *state = 5;
        break;
    case BSUBWAY_COMM_BATTLE_PROC_STEP_END:
        return TRUE;
    }
    return FALSE;
}

// EventBSubwayCommBattleProc_Callback's steps: the battle's proc, and its end
enum {
    BSUBWAY_COMM_BATTLE_STEP_QUEUE_PROC,
    BSUBWAY_COMM_BATTLE_STEP_WAIT_PROC,
};

static GameEventReturnCode EventBSubwayCommBattleProc_Callback(GameEvent *event, u32 *state, void *work) {
    BSubwayCommBattleData *data = work;
    GameSystem *gsys = data->gsys;
    BSubwayCommBattleParam *param;

    switch (*state) {
    case BSUBWAY_COMM_BATTLE_STEP_QUEUE_PROC:
        param = &data->param;
        data->param.gameData = GSYS_GetGameData(gsys);
        param->setup = data->setup;
        param->nameInputEncounterWork = data->nameInputEncounterWork;
        GSYS_QueueProc(gsys, -1, &BSUBWAY_COMM_BATTLE_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_WAIT_PROC:
        if (GSYS_GetProcMgrState(gsys) == FALSE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventBSubwayCommBattleProc_Create(GameSystem *gsys, BtlSetup *setup, void *nameInputEncounterWork) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBSubwayCommBattleProc_Callback, sizeof(BSubwayCommBattleData));
    BSubwayCommBattleData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->setup = setup;
    data->nameInputEncounterWork = nameInputEncounterWork;
    return event;
}

// EventBSubwayCommBattle_Callback's steps: the actors, the BGM, the encounter's effect, the field's close, the
// battle, the BGM's return, the field's opening, and the fade in
enum {
    BSUBWAY_COMM_BATTLE_STEP_PREPARE_ACTORS,
    BSUBWAY_COMM_BATTLE_STEP_PLAY_BGM,
    BSUBWAY_COMM_BATTLE_STEP_ENC_EFFECT,
    BSUBWAY_COMM_BATTLE_STEP_CLOSE_FIELD,
    BSUBWAY_COMM_BATTLE_STEP_BATTLE,
    BSUBWAY_COMM_BATTLE_STEP_FADE_BGM,
    BSUBWAY_COMM_BATTLE_STEP_OPEN_FIELD,
    BSUBWAY_COMM_BATTLE_STEP_WAIT_BGM,
    BSUBWAY_COMM_BATTLE_STEP_FADE_IN,
    BSUBWAY_COMM_BATTLE_STEP_END,
};

static GameEventReturnCode EventBSubwayCommBattle_Callback(GameEvent *event, u32 *state, void *work) {
    BSubwayBattleData *data = work;
    GameSystem *gsys = data->gsys;
    Field *field = data->field;

    switch (*state) {
    case BSUBWAY_COMM_BATTLE_STEP_PREPARE_ACTORS:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_PLAY_BGM:
        GameEvent_ChainNext(event, EventBattleBGMPlay_Create(gsys, data->setup->fieldSituation.bgm));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_ENC_EFFECT:
        EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
        EncEff_StartEvent(Field_GetEncEff(data->field), event, 9);
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_BATTLE:
        GameEvent_ChainNext(event, EventBSubwayCommBattleProc_Create(gsys, data->setup, data->nameInputEncounterWork));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_FADE_BGM:
        GameEvent_ChainNext(event, EventBGMFadePop_Create(gsys));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_WAIT_BGM:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_FADE_IN:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case BSUBWAY_COMM_BATTLE_STEP_END:
        return GAMEEVENT_DONE;
    default:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventBSubwayCommBattle_Create(GameSystem *gsys, Field *field, BtlSetup *setup,
                                                void *nameInputEncounterWork) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBSubwayCommBattle_Callback, sizeof(BSubwayBattleData));
    BSubwayBattleData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->setup = setup;
    data->nameInputEncounterWork = nameInputEncounterWork;
    data->field = field;
    return event;
}

static GameEvent *EventBSubwayCommBattle_CreateFromScrWork(BSubwayScrWork *bsw, GameSystem *gsys, Field *field) {
    return EventBSubwayCommBattle_Create(gsys, field, bsw->btlSetup, BsubwayScr_GetNameInputEncounterWork(bsw));
}

static GameEventReturnCode EventBSubwayLeaderMsg_Callback(GameEvent *event, u32 *state, void *work) {
    BSubwayLeaderMsgData *data = work;

    switch (*state) {
    case BSUBWAY_MSG_STEP_PRINT:
        if (MsgBGPrinter_TalkWinMain(data->msgWin) == TRUE) {
            (*state)++;
        }
        break;
    case BSUBWAY_MSG_STEP_WAIT_INPUT:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            MsgBGPrinter_DeleteTalkWin(data->msgWin);
            (*state)++;
        }
        break;
    case BSUBWAY_MSG_STEP_WAIT_CLOSE:
        if (MsgBGPrinter_WaitTalkWinClose(data->msgWin) == TRUE) {
            GFL_StrBufFree(data->strbuf);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBSubwayLeaderMsg_Create(GameSystem *gsys, u16 index, u16 actorId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBSubwayLeaderMsg_Callback, sizeof(BSubwayLeaderMsgData));
    BSubwayLeaderMsgData *data = GameEvent_GetData(event);
    MMSys *actorSystem;
    FieldActor *player;
    u8 winPos;
    FieldCamera *camera;
    VecFx32 offset;
    BSubwayWifiLeader *leaders;
    u32 winX;
    u32 winY;

    data->gsys = gsys;
    actorSystem = GameData_GetMMSys(gameData);
    player = FindPlayerFieldActor(actorSystem);
    CopyActorWPos(FindFieldActor(actorSystem, actorId), &data->pos);
    winPos = ActorMsgWin_CalcWinPosAuto(player, &data->pos);
    camera = Field_GetCameraSystem(field);
    ActorMsgWin_GetBalloonOffset(&data->pos, &offset, FieldCamera_GetG3DCamera(camera), camera, winPos);
    data->pos.x += offset.x;
    data->pos.y += offset.y;
    data->pos.z += offset.z;
    leaders = BSubwayWifiData_AllocLeaders(
        SaveControl_GetBlockPtr(GameData_GetSaveControl(gameData), SAVE_BLOCK_BSUBWAY_3A), HEAPID_GAMEEVENT);
    data->strbuf = PMSData_ToString(&leaders[index].message, HEAPID_GAMEEVENT);
    GFL_HeapFree(leaders);
    ActorMsgWin_GetBalloonTailParams(winPos, &winX, &winY);
    data->msgWin = ActorMsgWin_CheckAndCreate(msgBGSys, winX, &data->pos, data->strbuf, 0, winY);
    return event;
}
