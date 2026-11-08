// The Union Room's commands (a menu's choice, such as a trade or a battle), each run as a field event. The file's name
// is a guess from the UnionSystem's subproc, which the ROM's asserts name: the ROM only shows that it is a file of its
// own, by the assert file name ("") that its asserts share
#include "types.h"
#include "app/comm_btl_demo.h"
#include "app/p_status.h"
#include "app/pms_select.h"
#include "app/pokelist.h"
#include "app/pokemon_trade.h"
#include "constants/zones.h"
#include "demo/evolution_demo.h"
#include "field/colosseum.h"
#include "field/event_colosseum_battle.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_controller.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "field/union_app.h"
#include "field/union_comm.h"
#include "field/union_main.h"
#include "field/union_msg.h"
#include "field/union_oneself.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_sync.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// What a command's handler gets: the GameSystem, the UnionSystem, the Field, the command's argument, the event that
// runs it, a place for an event to run before the next call, and the step, which starts at 0. It returns whether it is
// done
typedef BOOL (*UnionCommandFunc)(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                 GameEvent **next, u8 *step);

// A row of the command table: what a command does, and the activity that this machine is in while it does:
// UNION_ACTIVITY_NONE leaves it as it is
typedef struct {
    UnionCommandFunc func;
    u8 startActivity;
    u8 endActivity;
} UnionCommandInfo;

// The event that runs a command
typedef struct {
    GameSystem *gsys;
    Field *field;
    UnionSystem *unisys;
} UnionCommandEvent;

static GameEventReturnCode UnionMain_RunCommand(GameEvent *event, u32 *seq, void *work);
static BOOL UnionCommand_TrainerCard(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                     GameEvent **next, u8 *step);
static BOOL UnionCommand_Trade(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step);
static BOOL UnionCommand_App(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                             GameEvent **next, u8 *step);
static BOOL UnionCommand_WarpA(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step);
static BOOL UnionCommand_WarpB(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step);
static BOOL UnionCommand_WarpRoom(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                  GameEvent **next, u8 *step);
static BOOL UnionCommand_Colosseum(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                   GameEvent **next, u8 *step);
static BOOL Colosseum_CommandBattle(GameSystem *gsys, UnionSystem *unisys, Field *field, void *param, GameEvent *event,
                                    GameEvent **next, u8 *step);
static BOOL Colosseum_CommandPmsSelect(GameSystem *gsys, UnionSystem *unisys, Field *field, void *param,
                                       GameEvent *event, GameEvent **next, u8 *step);

GameEvent *UnionCommand_CreateEvent(GameSystem *gsys, Field *field, UnionSystem *unisys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, UnionMain_RunCommand, sizeof(UnionCommandEvent));
    UnionCommandEvent *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    data->unisys = unisys;
    unisys->subproc.active = TRUE;
    return event;
}

// The commands, by the id that UnionCommand_Set starts one with. Each is run as an event by UnionMain_RunCommand:
// 1 the trainer card, 2 a trade, 3 and 4 the app's procs, 5 to 20 and 21 to 24 the warps into a room of a seat, 25 a
// warp, 26 the colosseum's party select, 27 the colosseum's battle, 28 the trainer card again, and 29 the phrase select
static const UnionCommandInfo sUnionCommands[] = {
    { NULL, 0, 0 },
    { UnionCommand_TrainerCard, UNION_ACTIVITY_CARD, UNION_ACTIVITY_TALKING },
    { UnionCommand_Trade, UNION_ACTIVITY_TRADE, UNION_ACTIVITY_TALKING },
    { UnionCommand_App, UNION_ACTIVITY_DRAW, 0 },
    { UnionCommand_App, UNION_ACTIVITY_SPIN_TRADE, 0 },
    { UnionCommand_WarpA, UNION_ACTIVITY_SINGLE, UNION_ACTIVITY_SINGLE },
    { UnionCommand_WarpA, UNION_ACTIVITY_SINGLE_NO_LAUNCHER, UNION_ACTIVITY_SINGLE_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_SINGLE_FLAT, UNION_ACTIVITY_SINGLE_FLAT },
    { UnionCommand_WarpA, UNION_ACTIVITY_SINGLE_FLAT_NO_LAUNCHER, UNION_ACTIVITY_SINGLE_FLAT_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_DOUBLE, UNION_ACTIVITY_DOUBLE },
    { UnionCommand_WarpA, UNION_ACTIVITY_DOUBLE_NO_LAUNCHER, UNION_ACTIVITY_DOUBLE_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_DOUBLE_FLAT, UNION_ACTIVITY_DOUBLE_FLAT },
    { UnionCommand_WarpA, UNION_ACTIVITY_DOUBLE_FLAT_NO_LAUNCHER, UNION_ACTIVITY_DOUBLE_FLAT_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_TRIPLE, UNION_ACTIVITY_TRIPLE },
    { UnionCommand_WarpA, UNION_ACTIVITY_TRIPLE_NO_LAUNCHER, UNION_ACTIVITY_TRIPLE_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_TRIPLE_FLAT, UNION_ACTIVITY_TRIPLE_FLAT },
    { UnionCommand_WarpA, UNION_ACTIVITY_TRIPLE_FLAT_NO_LAUNCHER, UNION_ACTIVITY_TRIPLE_FLAT_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_ROTATION, UNION_ACTIVITY_ROTATION },
    { UnionCommand_WarpA, UNION_ACTIVITY_ROTATION_NO_LAUNCHER, UNION_ACTIVITY_ROTATION_NO_LAUNCHER },
    { UnionCommand_WarpA, UNION_ACTIVITY_ROTATION_FLAT, UNION_ACTIVITY_ROTATION_FLAT },
    { UnionCommand_WarpA, UNION_ACTIVITY_ROTATION_FLAT_NO_LAUNCHER, UNION_ACTIVITY_ROTATION_FLAT_NO_LAUNCHER },
    { UnionCommand_WarpB, UNION_ACTIVITY_MULTI, UNION_ACTIVITY_MULTI },
    { UnionCommand_WarpB, UNION_ACTIVITY_MULTI_NO_LAUNCHER, UNION_ACTIVITY_MULTI_NO_LAUNCHER },
    { UnionCommand_WarpB, UNION_ACTIVITY_MULTI_FLAT, UNION_ACTIVITY_MULTI_FLAT },
    { UnionCommand_WarpB, UNION_ACTIVITY_MULTI_FLAT_NO_LAUNCHER, UNION_ACTIVITY_MULTI_FLAT_NO_LAUNCHER },
    { UnionCommand_WarpRoom, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { UnionCommand_Colosseum, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { Colosseum_CommandBattle, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { UnionCommand_TrainerCard, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { Colosseum_CommandPmsSelect, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
};

enum {
    UNION_COMMAND_EVENT_STEP_BEGIN = 0,
    UNION_COMMAND_EVENT_STEP_RUN = 1,
    UNION_COMMAND_EVENT_STEP_END = 2,
};

static GameEventReturnCode UnionMain_RunCommand(GameEvent *event, u32 *seq, void *work) {
    UnionCommandEvent *data = work;
    GameSystem *gsys = data->gsys;
    UnionSystem *unisys = data->unisys;
    UnionSubproc *subproc = &unisys->subproc;
    GameEvent *next = NULL;
    BOOL done;

    switch (*seq) {
    case UNION_COMMAND_EVENT_STEP_BEGIN:
        UnionMsg_FreeAll(unisys);
        if (sUnionCommands[subproc->id].startActivity != UNION_ACTIVITY_NONE) {
            UnionComm_SetSelf(unisys, UNION_SELF_ACTIVITY, sUnionCommands[subproc->id].startActivity);
        }
        (*seq)++;
        // fallthrough
    case UNION_COMMAND_EVENT_STEP_RUN:
        done = sUnionCommands[subproc->id].func(gsys, unisys, data->field, subproc->arg, event, &next, &subproc->step);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        } else if (done == TRUE) {
            (*seq)++;
        }
        break;
    case UNION_COMMAND_EVENT_STEP_END:
        if (sUnionCommands[subproc->id].endActivity != UNION_ACTIVITY_NONE) {
            UnionComm_SetSelf(unisys, UNION_SELF_ACTIVITY, sUnionCommands[subproc->id].endActivity);
        }
        subproc->id = UNION_SUBPROC_ID_NULL;
        subproc->arg = NULL;
        subproc->step = 0;
        subproc->active = FALSE;
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void UnionCommand_Set(UnionSystem *unisys, u32 id, void *arg) {
    GFL_ASSERT(unisys->subproc.id == UNION_SUBPROC_ID_NULL && unisys->subproc.active == FALSE);
    unisys->subproc.id = id;
    unisys->subproc.arg = arg;
    unisys->subproc.step = 0;
}

BOOL UnionCommand_IsActive(UnionSystem *unisys) {
    if (unisys->subproc.id == UNION_SUBPROC_ID_NULL && unisys->subproc.active == FALSE) {
        return FALSE;
    }
    return TRUE;
}

enum {
    UNION_ONESHOT_STEP_START = 0,
};

static BOOL UnionCommand_TrainerCard(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                     GameEvent **next, u8 *step) {
    if (*step == UNION_ONESHOT_STEP_START) {
        *next =
            EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(186), &TRCARD_SYS_VIEW_PROC_FUNCTIONS, arg);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

enum {
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_START_TRADE,
    STEP_WAIT_TRADE,
    STEP_START_EVOLVE,
    STEP_WAIT_EVOLVE,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_END,
};

static BOOL UnionCommand_Trade(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step) {
    PokemonTradeParam *trade = arg;
    EvolutionDemoParam *evolution;

    switch (*step) {
    case STEP_FADE_OUT:
        *next = CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0);
        (*step)++;
        break;
    case STEP_CLOSE_FIELD:
        *next = CreateFieldCloseEvent(gsys, field);
        (*step)++;
        break;
    case STEP_START_TRADE:
        GSYS_QueueProc(gsys, OVERLAY_POKEMONTRADE, &POKEMONTRADE_UNION_PROC_FUNCTIONS, trade);
        (*step)++;
        break;
    case STEP_WAIT_TRADE:
        if (GSYS_GetProcMgrState(gsys) != PROCMGR_DONE) {
            break;
        }
        if (trade->next == TRADE_NEXT_EVOLVE) {
            *step = STEP_START_EVOLVE;
        } else {
            *step = STEP_OPEN_FIELD;
        }
        break;
    case STEP_START_EVOLVE:
        evolution = trade->evolution;
        evolution->gameData = trade->gameData;
        evolution->party = trade->party;
        evolution->species = trade->evolveSpecies;
        evolution->partyIndex = 0;
        evolution->method = trade->evolveMethod;
        evolution->unkC = 1;
        evolution->canCancel = FALSE;
        trade->evolution = evolution;
        GSYS_QueueProc(gsys, OVERLAY_EVOLUTION_DEMO, &EVOLUTION_DEMO_PROC_FUNCTIONS, evolution);
        (*step)++;
        break;
    case STEP_WAIT_EVOLVE:
        if (GSYS_GetProcMgrState(gsys) != PROCMGR_DONE) {
            break;
        }
        if (GFL_NetErrCheck()) {
            *step = STEP_OPEN_FIELD;
        } else {
            trade->next = TRADE_NEXT_EVOLVE;
            *step = STEP_START_TRADE;
        }
        break;
    case STEP_OPEN_FIELD:
        *next = EventFieldOpen_CreateHeadless(gsys);
        *step = STEP_FADE_IN;
        break;
    case STEP_FADE_IN:
        *next = CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0);
        *step = STEP_END;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

enum {
    UNION_APP_COMMAND_STEP_FADE_OUT = 0,
    UNION_APP_COMMAND_STEP_CLOSE_FIELD = 1,
    UNION_APP_COMMAND_STEP_CREATE_UNION_APP = 2,
    UNION_APP_COMMAND_STEP_SEND_INITIAL_SYNC = 3,
    UNION_APP_COMMAND_STEP_WAIT_INITIAL_SYNC = 4,
    UNION_APP_COMMAND_STEP_ENQUEUE = 5,
    UNION_APP_COMMAND_STEP_WAIT_TURN = 6,
    UNION_APP_COMMAND_STEP_REQUEST_STATUS = 7,
    UNION_APP_COMMAND_STEP_WAIT_MEMBERS = 8,
    UNION_APP_COMMAND_STEP_SEND_PROFILE = 9,
    UNION_APP_COMMAND_STEP_WAIT_PROFILES = 10,
    UNION_APP_COMMAND_STEP_CONFIRM_PROFILES = 11,
    UNION_APP_COMMAND_STEP_WAIT_CONFIRM_SYNC = 12,
    UNION_APP_COMMAND_STEP_START_PROC = 13,
    UNION_APP_COMMAND_STEP_WAIT_PROC = 14,
    UNION_APP_COMMAND_STEP_WAIT_BGM = 15,
    UNION_APP_COMMAND_STEP_FREE_UNION_APP = 16,
    UNION_APP_COMMAND_STEP_OPEN_FIELD = 17,
    UNION_APP_COMMAND_STEP_FADE_IN = 18,
    UNION_APP_COMMAND_STEP_END = 19,
};

static BOOL UnionCommand_App(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                             GameEvent **next, u8 *step) {
    UnionSelf *self = &unisys->situ;
    UnionSubproc *subproc = &unisys->subproc;
    UnionProcParam *param;
    GameEvent *chain;
    u32 joined;

    if (GFL_NetErrCheck() && *step > UNION_APP_COMMAND_STEP_CREATE_UNION_APP &&
        *step <= UNION_APP_COMMAND_STEP_START_PROC) {
        *step = UNION_APP_COMMAND_STEP_FREE_UNION_APP;
    }
    if (unisys->alloc.uniapp != NULL) {
        UnionApp_Update(unisys->alloc.uniapp, unisys);
    }
    switch (*step) {
    case UNION_APP_COMMAND_STEP_FADE_OUT:
        chain = CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_APP_COMMAND_STEP_CLOSE_FIELD:
        chain = CreateFieldCloseEvent(gsys, field);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_APP_COMMAND_STEP_CREATE_UNION_APP:
        GFL_OvlLoad(OVERLAY_ID(69));
        unisys->alloc.uniapp = UnionApp_Create(unisys, 3, 5, GetGameDataPlayerInfo(unisys->param->gameData));
        if (self->mycomm.joinedRecruit == 1) {
            *step = UNION_APP_COMMAND_STEP_ENQUEUE;
        } else {
            (*step)++;
        }
        break;
    case UNION_APP_COMMAND_STEP_SEND_INITIAL_SYNC:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 9, 0x14);
        (*step)++;
        break;
    case UNION_APP_COMMAND_STEP_WAIT_INITIAL_SYNC:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 9, 0x14) == TRUE) {
            *step = UNION_APP_COMMAND_STEP_REQUEST_STATUS;
        }
        break;
    case UNION_APP_COMMAND_STEP_ENQUEUE:
        if (UnionMain_SendEnqueue() == TRUE) {
            (*step)++;
        }
        break;
    case UNION_APP_COMMAND_STEP_WAIT_TURN:
        if (UnionApp_IsTurnReceived(unisys->alloc.uniapp) == TRUE) {
            *step = UNION_APP_COMMAND_STEP_REQUEST_STATUS;
        }
        break;
    case UNION_APP_COMMAND_STEP_REQUEST_STATUS:
        if (UnionMain_SendRequestStatus() == TRUE) {
            (*step)++;
        }
        break;
    case UNION_APP_COMMAND_STEP_WAIT_MEMBERS:
        if (UnionApp_HasMembers(unisys->alloc.uniapp) == TRUE) {
            (*step)++;
        }
        break;
    case UNION_APP_COMMAND_STEP_SEND_PROFILE:
        joined = UnionApp_GetJoined(unisys->alloc.uniapp);
        if (UnionMain_SendAppProfile(joined, GetGameDataPlayerInfo(unisys->param->gameData)) == TRUE) {
            (*step)++;
        }
        break;
    case UNION_APP_COMMAND_STEP_WAIT_PROFILES:
        if (UnionApp_AllProfilesReceived(unisys->alloc.uniapp) == TRUE) {
            (*step)++;
        }
        break;
    case UNION_APP_COMMAND_STEP_CONFIRM_PROFILES:
        if (self->mycomm.joinedRecruit == 1) {
            joined = UnionApp_GetJoined(unisys->alloc.uniapp);
            if (UnionMain_SendConfirmProfile(joined, GetGameDataPlayerInfo(unisys->param->gameData)) == TRUE) {
                UnionApp_JoinSelf(unisys->alloc.uniapp);
                *step = UNION_APP_COMMAND_STEP_START_PROC;
            }
        } else {
            UnionComm_SetCommandBase(self->activity);
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 10, 0x14);
            *step = UNION_APP_COMMAND_STEP_WAIT_CONFIRM_SYNC;
        }
        break;
    case UNION_APP_COMMAND_STEP_WAIT_CONFIRM_SYNC:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 10, 0x14) == TRUE) {
            GFL_NetSetAcceptConnect(NetHandle_GetOwn(), TRUE);
            *step = UNION_APP_COMMAND_STEP_START_PROC;
        }
        break;
    case UNION_APP_COMMAND_STEP_START_PROC:
        GFL_NetSetDisconnectError(FALSE);
        if (self->activity == UNION_ACTIVITY_DRAW) {
            param = subproc->arg;
            param->app = unisys->alloc.uniapp;
            GSYS_QueueProc(gsys, OVERLAY_ID(70), &DRAWING_PROC_FUNCTIONS, param);
        } else {
            param = subproc->arg;
            param->app = unisys->alloc.uniapp;
            GSYS_QueueProc(gsys, OVERLAY_ID(216), &SPIN_TRADE_PROC_FUNCTIONS, param);
        }
        (*step)++;
        break;
    case UNION_APP_COMMAND_STEP_WAIT_PROC:
        if (GSYS_GetProcMgrState(gsys) != PROCMGR_DONE) {
            break;
        }
        if (self->activity == UNION_ACTIVITY_SPIN_TRADE) {
            param = subproc->arg;
            if (param->spinTradeStarted == 1) {
                GFL_SndBGMRequestChange((u16)GetMapBGMIDByPlayerState2(unisys->param->gameData, ZONE_UNION_ROOM,
                                                                       GameData_GetSeason(unisys->param->gameData)),
                                        0xffff, 6, 0x3c);
                *step = UNION_APP_COMMAND_STEP_WAIT_BGM;
            } else {
                *step = UNION_APP_COMMAND_STEP_FREE_UNION_APP;
            }
        } else {
            *step = UNION_APP_COMMAND_STEP_FREE_UNION_APP;
        }
        break;
    case UNION_APP_COMMAND_STEP_WAIT_BGM:
        if (GFL_SndBGMIsFading()) {
            break;
        }
        *step = UNION_APP_COMMAND_STEP_FREE_UNION_APP;
        break;
    case UNION_APP_COMMAND_STEP_FREE_UNION_APP:
        if (unisys->alloc.uniapp != NULL) {
            UnionApp_Free(unisys->alloc.uniapp);
            unisys->alloc.uniapp = NULL;
            GFL_OvlUnload(OVERLAY_ID(69));
        }
        *step = UNION_APP_COMMAND_STEP_OPEN_FIELD;
        break;
    case UNION_APP_COMMAND_STEP_OPEN_FIELD:
        chain = EventFieldOpen_CreateHeadless(gsys);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_APP_COMMAND_STEP_FADE_IN:
        chain = CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_APP_COMMAND_STEP_END:
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL UnionCommand_WarpA(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step) {
    VecFx32 pos;

    if (*step == UNION_ONESHOT_STEP_START) {
        pos.x = (GFL_NetGetHandleNetId(NetHandle_GetOwn()) * 16 + 88) << FX32_SHIFT;
        pos.y = 0;
        pos.z = FX32_CONST(152);
        *next = EventMapChangeWarp_CreateGrid(gsys, field, ZONE_UNION_ROOM_COLOSSEUM, &pos, 0, FALSE);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL UnionCommand_WarpB(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step) {
    VecFx32 pos;

    if (*step == UNION_ONESHOT_STEP_START) {
        pos.x = (GFL_NetGetHandleNetId(NetHandle_GetOwn()) * 16 + 88) << FX32_SHIFT;
        pos.y = 0;
        pos.z = FX32_CONST(152);
        *next = EventMapChangeWarp_CreateGrid(gsys, field, ZONE_UNION_ROOM_COLOSSEUM_2, &pos, 0, FALSE);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL UnionCommand_WarpRoom(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                  GameEvent **next, u8 *step) {
    VecFx32 pos;

    if (*step == UNION_ONESHOT_STEP_START) {
        pos.x = FX32_CONST(184);
        pos.y = 0;
        pos.z = FX32_CONST(232);
        *next = EventMapChangeWarp_CreateGrid(gsys, field, ZONE_UNION_ROOM, &pos, 0, FALSE);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

enum {
    UNION_COLOSSEUM_COMMAND_STEP_FADE_OUT = 0,
    UNION_COLOSSEUM_COMMAND_STEP_CLOSE_FIELD = 1,
    UNION_COLOSSEUM_COMMAND_STEP_INIT_SYNC = 2,
    UNION_COLOSSEUM_COMMAND_STEP_START_PARTY_LIST = 3,
    UNION_COLOSSEUM_COMMAND_STEP_WAIT_PARTY_LIST = 4,
    UNION_COLOSSEUM_COMMAND_STEP_START_SUMMARY = 5,
    UNION_COLOSSEUM_COMMAND_STEP_WAIT_SUMMARY = 6,
    UNION_COLOSSEUM_COMMAND_STEP_EXIT_SYNC = 7,
    UNION_COLOSSEUM_COMMAND_STEP_BUILD_PARTY = 8,
    UNION_COLOSSEUM_COMMAND_STEP_WAIT_PARTY_SYNC = 9,
    UNION_COLOSSEUM_COMMAND_STEP_SEND_PARTY = 10,
    UNION_COLOSSEUM_COMMAND_STEP_WAIT_PARTIES = 11,
    UNION_COLOSSEUM_COMMAND_STEP_START_BATTLE = 12,
    UNION_COLOSSEUM_COMMAND_STEP_UPDATE_FRIEND_RECORDS = 13,
    UNION_COLOSSEUM_COMMAND_STEP_OPEN_FIELD = 14,
    UNION_COLOSSEUM_COMMAND_STEP_FADE_IN = 15,
    UNION_COLOSSEUM_COMMAND_STEP_END = 16,
    UNION_COLOSSEUM_COMMAND_STEP_NET_ERROR = 17,
};

static BOOL UnionCommand_Colosseum(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                   GameEvent **next, u8 *step) {
    UnionColosseumParam *param = arg;
    UnionColosseum *colosseum = unisys->colosseum_sys;
    UnionSelf *self = &unisys->situ;
    PStatusParam *status = &param->status;
    ColosseumBattleParam *battle = param->battle;
    u32 selfId = GFL_NetGetHandleNetId(NetHandle_GetOwn());
    PokeParty *tmpParty;
    PokeParty *party;
    GameEvent *chain;
    WifiList *wifiList;
    u32 friendIndex;
    int count;
    u32 isRedTeam;
    int i;

    if (*step > UNION_COLOSSEUM_COMMAND_STEP_INIT_SYNC && *step < UNION_COLOSSEUM_COMMAND_STEP_EXIT_SYNC) {
        NetSync_Update((NetSyncWork *)param);
    }
    switch (*step) {
    case UNION_COLOSSEUM_COMMAND_STEP_FADE_OUT:
        chain = CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_CLOSE_FIELD:
        chain = CreateFieldCloseEvent(gsys, field);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_INIT_SYNC:
        GFL_OvlLoad(OVERLAY_ID(164));
        NetSync_Init((NetSyncWork *)param);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_START_PARTY_LIST:
        GSYS_QueueProc(gsys, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, param);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_WAIT_PARTY_LIST:
        if (GSYS_GetProcMgrState(gsys) != PROCMGR_DONE) {
            break;
        }
        if (param->list.result == POKELIST_RESULT_SUMMARY) {
            *step = UNION_COLOSSEUM_COMMAND_STEP_START_SUMMARY;
        } else {
            *step = UNION_COLOSSEUM_COMMAND_STEP_EXIT_SYNC;
        }
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_START_SUMMARY:
        status->party = param->list.party;
        status->trainerData = param->list.trainerData;
        status->gameData = GSYS_GetGameData(gsys);
        status->dataType = PSTATUS_DATA_PARTY;
        status->partyCount = PokeParty_GetPkmCount(param->list.party);
        status->mode = PSTATUS_MODE_NORMAL;
        status->partyIndex = param->list.index;
        status->page = PSTATUS_PAGE_INFO;
        status->isNationalDex = PokeDex_IsNationalObtained(GameData_GetPokedex(status->gameData));
        GSYS_QueueProc(gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, status);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_WAIT_SUMMARY:
        if (GSYS_GetProcMgrState(gsys) != PROCMGR_DONE) {
            break;
        }
        param->list.index = status->partyIndex;
        *step = UNION_COLOSSEUM_COMMAND_STEP_START_PARTY_LIST;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_EXIT_SYNC:
        NetSync_Exit((NetSyncWork *)param);
        GFL_OvlUnload(OVERLAY_ID(164));
        *step = UNION_COLOSSEUM_COMMAND_STEP_BUILD_PARTY;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_BUILD_PARTY:
        if (GFL_NetErrCheck()) {
            *step = UNION_COLOSSEUM_COMMAND_STEP_NET_ERROR;
            break;
        }
        GFL_ASSERT_MSG(param->list.result == POKELIST_RESULT_DONE, "plist->ret_mode \x95\x73\x90\xb3 %d\n",
                       param->list.result);
        GFL_ASSERT_MSG(param->list.index == POKELIST_CURSOR_CONFIRM, "ret_sel=%d\n", param->list.index);
        party = colosseum->party[selfId];
        tmpParty = PokeParty_Create(HEAPID_UNION);
        PokeParty_Copy(party, tmpParty);
        PokeParty_Init(party);
        for (i = 0; i < 6; i++) {
            if (param->list.picked[i] == 0) {
                break;
            }
            PokeParty_AddPkm(party, PokeParty_GetPkm(tmpParty, param->list.picked[i] - 1));
        }
        GFL_HeapFree(tmpParty);
        Colosseum_ClearParties(colosseum, TRUE);
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 22, 0x14);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_WAIT_PARTY_SYNC:
        if (GFL_NetErrCheck()) {
            *step = UNION_COLOSSEUM_COMMAND_STEP_NET_ERROR;
            break;
        }
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 22, 0x14) == TRUE) {
            (*step)++;
        }
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_SEND_PARTY:
        if (GFL_NetErrCheck()) {
            *step = UNION_COLOSSEUM_COMMAND_STEP_NET_ERROR;
            break;
        }
        if (Colosseum_SendParty(colosseum->party[selfId]) == TRUE) {
            (*step)++;
        }
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_WAIT_PARTIES:
        if (GFL_NetErrCheck()) {
            *step = UNION_COLOSSEUM_COMMAND_STEP_NET_ERROR;
            break;
        }
        if (Colosseum_AreAllPartiesReceived(colosseum) == TRUE) {
            (*step)++;
        }
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_START_BATTLE:
        battle->party = colosseum->party[selfId];
        battle->clientId = colosseum->positions[selfId];
        battle->regulation = unisys->alloc.regulation;
        if (selfId == 0 || (colosseum->positions[0] & 2) == (colosseum->positions[selfId] & 2)) {
            battle->bgm = 0x489;
        } else {
            battle->bgm = 0x48a;
        }
        Colosseum_BuildBattlePlayers(colosseum, gsys, &battle->players, HEAPID_UNION);
        *next = EventColosseumBattle_Create(gsys, field, self->activity, battle);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_UPDATE_FRIEND_RECORDS:
        if (!GFL_NetErrCheck()) {
            wifiList = GameData_GetWifiList(GSYS_GetGameData(gsys));
            count = GFL_NetGetNegotiatedCount();
            isRedTeam = colosseum->positions[selfId] & 1;
            for (i = 0; i < count; i++) {
                if (i == selfId) {
                    continue;
                }
                if (WifiList_FindFriendByPlayerInfo(wifiList, &colosseum->members[i].info, &friendIndex) != TRUE) {
                    continue;
                }
                if (isRedTeam == (colosseum->positions[i] & 1)) {
                    WifiList_SetFriendLastDateToNow(wifiList, friendIndex);
                } else {
                    switch (battle->players.battle.result) {
                    case COMM_BTL_DEMO_RESULT_WIN:
                        WifiList_AddFriendRecord(wifiList, friendIndex, 1, 0, 0);
                        break;
                    case COMM_BTL_DEMO_RESULT_LOSE:
                        WifiList_AddFriendRecord(wifiList, friendIndex, 0, 1, 0);
                        break;
                    case COMM_BTL_DEMO_RESULT_DRAW:
                        WifiList_SetFriendLastDateToNow(wifiList, friendIndex);
                        break;
                    }
                }
            }
        }
        Colosseum_ClearPlayers(&battle->players);
        *step = UNION_COLOSSEUM_COMMAND_STEP_END;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_OPEN_FIELD:
        chain = EventFieldOpen_CreateHeadless(gsys);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case UNION_COLOSSEUM_COMMAND_STEP_FADE_IN:
        chain = CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    default:
        return TRUE;
    case UNION_COLOSSEUM_COMMAND_STEP_NET_ERROR:
        *step = UNION_COLOSSEUM_COMMAND_STEP_OPEN_FIELD;
        break;
    }
    return FALSE;
}

static BOOL Colosseum_CommandBattle(GameSystem *gsys, UnionSystem *unisys, Field *field, void *param, GameEvent *event,
                                    GameEvent **next, u8 *step) {
    UnionSelf *self = &unisys->situ;

    if (*step == UNION_ONESHOT_STEP_START) {
        *next = EventColosseumBattle_Create(gsys, field, self->activity, param);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL Colosseum_CommandPmsSelect(GameSystem *gsys, UnionSystem *unisys, Field *field, void *param,
                                       GameEvent *event, GameEvent **next, u8 *step) {
    if (*step == UNION_ONESHOT_STEP_START) {
        *next = EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(185), &PMS_SELECT_PROC_FUNCTIONS, param);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}
