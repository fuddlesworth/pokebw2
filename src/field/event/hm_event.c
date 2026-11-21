// The hidden moves' events (a descriptive name). CalcPlayerActionPossibilities is placed here as a guess: it fills the
// PlayerActionPossibilities that this file's checks take and is the only caller of HiddenArea_ContainsUnlessFlag, but a
// file of its own between field_encount_st.c and this one would give the same ROM. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), in English: swan's Hiden (the HMs) is HM
#include "types.h"
#include "app/pokelist.h"
#include "constants/moves.h"
#include "constants/obj_codes.h"
#include "field/event_chatot.h"
#include "field/event_fly.h"
#include "field/event_mapchange.h"
#include "field/event_sweet_scent.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "field/field_skill_map_eff.h"
#include "field/field_status.h"
#include "field/game_beacon_set.h"
#include "field/hidden_event.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "nitro/os.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The places in the Abyssal Ruins where Flash and Strength can be used
static const HiddenArea ABYSSAL_RUINS_FLASH_ROCK_RADIUS = { 14, 26, 1, 2 };
static const HiddenArea ABYSSAL_RUINS_STRENGTH_ROCK_RADIUS = { 40, 26, 1, 2 };

// Four zero bytes that nothing reads, ahead of the Abyssal Ruins areas. Their type and form are a guess: as a
// function-local static, or as two u16s, they give the same ROM
const u32 HM_EVENT_UNUSED = 0;

void CalcPlayerActionPossibilities(Field *field, PlayerActionPossibilities *action) {
    FieldPlayer *player;
    FieldActor *actor;
    u16 objCode;
    MMSys *actorSystem;
    VecFx32 position;
    u32 direction;
    u32 tileInFront;
    u32 tileUnder;
    u16 diveZone;
    s16 x;
    s16 y;
    s16 z;

    MI_CpuFill8(action, 0, sizeof(PlayerActionPossibilities));
    action->zoneId = Field_GetPlayerStateZoneID(field);
    action->gsys = Field_GetGameSystem(field);
    action->field = field;
    player = Field_GetPlayer(field);
    action->exState = FieldPlayer_GetExState(player);
    actor = FieldPlayer_GetActorInFront(player);
    action->actorInFront = actor;
    if (actor != NULL) {
        objCode = FldAct_GetObjCode(actor);
        actorSystem = Field_GetActorSystem(field);
        CopyActorWPos(actor, &position);
        if (IsNPCStrengthRock(objCode) && !IsSavedStrengthRockAtPos(actorSystem, &position)) {
            action->flags |= 1 << HM_EVENT_STRENGTH;
        }
        if (objCode == OBJ_CODE_108) {
            action->flags |= 1 << HM_EVENT_CUT;
        }
    }
    direction = FieldPlayer_GetFaceDir(player);
    tileUnder = FieldPlayer_GetTileTypeUnder(player);
    tileInFront = FieldPlayer_GetTileTypeInDir(player, direction);
    if (CheckSurfHeightAllow(player, direction) == TRUE) {
        action->flags |= 1 << HM_EVENT_SURF;
    }
    if (CheckCanInteractWaterfall(player, tileUnder, tileInFront) == TRUE) {
        action->flags |= 1 << HM_EVENT_WATERFALL;
    }
    if (GetZoneFlagsEnableFlyFrom(action->zoneId) == TRUE) {
        action->flags |= 1 << HM_EVENT_FLY;
        action->flags |= 1 << HM_EVENT_TELEPORT;
    }
    if (GetZoneFlagsEnableEscapeRope(action->zoneId) == TRUE) {
        action->flags |= 1 << HM_EVENT_DIG;
    }
    if (FieldStatus_GetFlashPerms(GameData_GetFieldStatus(GSYS_GetGameData(action->gsys))) & FLD_FLASH_ALLOW) {
        action->flags |= 1 << HM_EVENT_FLASH;
    }
    if (GetAbyssalRuinsDiveZoneID(field, &diveZone) == TRUE) {
        action->flags |= 1 << HM_EVENT_DIVE;
    }
    if (IsZoneAbyssalRuinsInside(action->zoneId)) {
        FieldPlayer_GetGPos(Field_GetPlayer(action->field), &x, &y, &z);
        if (IsZoneAbyssalRuinsFlashRock(action->zoneId) &&
            HiddenArea_ContainsUnlessFlag(&ABYSSAL_RUINS_FLASH_ROCK_RADIUS, x, z, 0xd7, field)) {
            action->flags |= 1 << HM_EVENT_FLASH;
        }
        if (IsZoneAbyssalRuinsStrengthRock(action->zoneId) &&
            HiddenArea_ContainsUnlessFlag(&ABYSSAL_RUINS_STRENGTH_ROCK_RADIUS, x, z, 0xd6, field)) {
            action->flags |= 1 << HM_EVENT_STRENGTH;
        }
    }
    if (!IsZoneNormal(action->zoneId)) {
        action->flags = 0;
    } else {
        action->flags |= 1 << HM_EVENT_SWEET_SCENT;
        action->flags |= 1 << HM_EVENT_CHATTER;
    }
    if (GameData_IsForceSeasonSync(GSYS_GetGameData(action->gsys)) == TRUE) {
        action->flags = 0;
    }
}

// The events of each hidden move, by its kind, and the same in the Abyssal Ruins, where Strength and Flash differ
const HiddenEventSpec HM_EVENTS_NORMAL[HM_EVENT_MAX] = {
    { EventCutCall_Create, EventCutCall_Check },
    { EventSurfCall_Create, EventSurfCall_Check },
    { EventWaterfallCall_Create, EventWaterfallCall_Check },
    { EventStrengthCall_Create, EventStrengthCall_Check },
    { EventFly_Create, EventFly_Check },
    { EventFlash_Create, EventFlash_Check },
    { EventTeleportCall_Create, EventTeleportCall_Check },
    { EventDigCall_Create, EventDigCall_Check },
    { EventSweetScentCall_Create, EventSweetScentCall_Check },
    { EventChatterCall_Create, EventChatterCall_Check },
    { EventDivingCall_Create, EventDivingCall_Check },
};

const HiddenEventSpec HM_EVENTS_RUINS[HM_EVENT_MAX] = {
    { EventCutCall_Create, EventCutCall_Check },
    { EventSurfCall_Create, EventSurfCall_Check },
    { EventWaterfallCall_Create, EventWaterfallCall_Check },
    { EventRuinsStrengthCall_Create, EventStrengthCall_Check },
    { EventFly_Create, EventFly_Check },
    { EventRuinsFlash_Create, EventFlash_Check },
    { EventTeleportCall_Create, EventTeleportCall_Check },
    { EventDigCall_Create, EventDigCall_Check },
    { EventSweetScentCall_Create, EventSweetScentCall_Check },
    { EventChatterCall_Create, EventChatterCall_Check },
    { EventDivingCall_Create, EventDivingCall_Check },
};

u32 CheckAllowHMEvent(u32 kind, PlayerActionPossibilities *context) {
    HiddenCheckFunc check = GetHMEventCheckFunc(context, kind);
    if (check == NULL) {
        return TRUE;
    }
    return check(context);
}

void HiddenEventArgs_Set(HiddenEventArgs *args, u16 partySlot, u16 kind, u32 value) {
    args->partySlot = partySlot;
    args->kind = kind;
    args->value = value;
}

GameEvent *CreateHMEvent(u32 kind, HiddenEventArgs *args, PlayerActionPossibilities *context) {
    HiddenCtorFunc ctor = GetHMEventCtorFunc(context, kind);
    if (ctor == NULL) {
        return NULL;
    }
    return ctor(args, context);
}

BOOL HMEvent_CheckPairFlag(PlayerActionPossibilities *context) {
    return GameData_CheckPairFlag(GSYS_GetGameData(context->gsys));
}

u32 EventCutCall_Check(PlayerActionPossibilities *context) {
    u32 result = FALSE;
    if (HMEvent_CheckContextFlag(context, HM_EVENT_CUT) == 0) {
        result = TRUE;
    }
    return result;
}

GameEvent *EventCutCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;

    event = GameEvent_Create(context->gsys, NULL, EventCutCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

GameEventReturnCode EventCutCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    u16 param;
    ScriptWork *scriptWork;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    param = work->args.partySlot;
    scriptWork = EventScriptCall_Replace(event, 0x2715, 0, 0);
    ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
    return GAMEEVENT_CONTINUE;
}

u32 EventSurfCall_Check(PlayerActionPossibilities *context) {
    u32 state = context->exState;
    if (state == FLD_PLAYER_EXSTATE_SURF) {
        return POKELIST_HM_CHECK_ALREADY_SURFING;
    }
    if (state == FLD_PLAYER_EXSTATE_DIVE) {
        return POKELIST_HM_CHECK_UNAVAILABLE;
    }
    if (HMEvent_CheckContextFlag(context, HM_EVENT_SURF) != 0) {
        if (HMEvent_CheckPairFlag(context) != 0) {
            return POKELIST_HM_CHECK_WITH_PARTNER;
        }
        return POKELIST_HM_CHECK_ALLOWED;
    }
    return POKELIST_HM_CHECK_UNAVAILABLE;
}

GameEvent *EventSurfCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;

    event = GameEvent_Create(context->gsys, NULL, EventSurfCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

GameEventReturnCode EventSurfCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    u16 param;
    ScriptWork *scriptWork;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    param = work->args.partySlot;
    scriptWork = EventScriptCall_Replace(event, 0x2713, 0, 0);
    ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
    return GAMEEVENT_CONTINUE;
}

u32 EventWaterfallCall_Check(PlayerActionPossibilities *context) {
    if (HMEvent_CheckContextFlag(context, HM_EVENT_WATERFALL) != 0) {
        if (HMEvent_CheckPairFlag(context) != 0) {
            return POKELIST_HM_CHECK_WITH_PARTNER;
        }
        return POKELIST_HM_CHECK_ALLOWED;
    }
    return POKELIST_HM_CHECK_UNAVAILABLE;
}

GameEvent *EventWaterfallCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;

    event = GameEvent_Create(context->gsys, NULL, EventWaterfallCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

GameEventReturnCode EventWaterfallCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    u16 param;
    ScriptWork *scriptWork;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    param = work->args.partySlot;
    scriptWork = EventScriptCall_Replace(event, 0x2717, 0, 0);
    ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
    return GAMEEVENT_CONTINUE;
}

u32 EventStrengthCall_Check(PlayerActionPossibilities *context) {
    switch (HMEvent_CheckContextFlag(context, HM_EVENT_STRENGTH)) {
    default:
        return FALSE;
    case FALSE:
        return TRUE;
    }
}

GameEvent *EventStrengthCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;

    event = GameEvent_Create(context->gsys, NULL, EventStrengthCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

GameEventReturnCode EventStrengthCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    u16 param;
    ScriptWork *scriptWork;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    param = work->args.partySlot;
    scriptWork = EventScriptCall_Replace(event, 0x2711, 0, 0);
    ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
    return GAMEEVENT_CONTINUE;
}

u32 EventSweetScentCall_Check(PlayerActionPossibilities *context) {
    switch (HMEvent_CheckContextFlag(context, HM_EVENT_SWEET_SCENT)) {
    default:
        return FALSE;
    case FALSE:
        return TRUE;
    }
}

GameEvent *EventSweetScentCall_Create(HiddenEventArgs *args, PlayerActionPossibilities *context) {
    return EventSweetScent_CreateFromPartySlot(context->gsys, context->field, args->partySlot);
}

GameEvent *EventFly_Create(HiddenEventArgs *args, PlayerActionPossibilities *context) {
    GameBeacon_BroadcastFieldMove(MOVE_FLY);
    return EventFly_CreateMapChangeEvent(context->gsys, context->field, args->value);
}

u32 EventFly_Check(PlayerActionPossibilities *context) {
    if (HMEvent_CheckContextFlag(context, HM_EVENT_FLY) != 0) {
        if (HMEvent_CheckPairFlag(context) != 0) {
            return POKELIST_HM_CHECK_WITH_PARTNER;
        }
        return POKELIST_HM_CHECK_ALLOWED;
    }
    return POKELIST_HM_CHECK_UNAVAILABLE;
}

u32 EventFlash_Check(PlayerActionPossibilities *context) {
    switch (HMEvent_CheckContextFlag(context, HM_EVENT_FLASH)) {
    default:
        return FALSE;
    case FALSE:
        return TRUE;
    }
}

GameEvent *EventFlash_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;

    event = GameEvent_Create(context->gsys, NULL, EventFlash_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

// EventFlash_Callback's steps: the script call, the flash itself, and the wait for it to light
enum {
    EVENT_FLASH_STEP_START_SCRIPT,
    EVENT_FLASH_STEP_START_FLASH,
    EVENT_FLASH_STEP_WAIT_LIT,
};

GameEventReturnCode EventFlash_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    GameData *gameData;
    FieldStatus *status;
    void *flash;
    HeapID heapId;
    ScriptWork *scriptWork;
    u16 param;
    u32 flags;
    FieldStatus *status2;

    gameData = GSYS_GetGameData(work->gsys);
    GameData_GetEventWork(gameData);
    status = GameData_GetFieldStatus(gameData);
    flash = FieldSkillMapEff_GetFlash(Field_GetSkillMapEff(GSYS_GetField(work->gsys)));
    heapId = Field_GetHeapID(GSYS_GetField(work->gsys));
    switch (*state) {
    case EVENT_FLASH_STEP_START_SCRIPT:
        param = work->args.partySlot;
        scriptWork = EventScriptCall_Start(event, 0x2718, NULL, 0, heapId);
        ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
        ++*state;
        break;
    case EVENT_FLASH_STEP_START_FLASH:
        if (!FieldStatus_CheckFlashUsed(status)) {
            status2 = GameData_GetFieldStatus(gameData);
            FieldStatus_SetFlashUsed(status, TRUE);
            GameBeacon_BroadcastFieldMove(MOVE_FLASH);
            FieldFlash_RequestState(flash, FIELD_FLASH_REQUEST_WIDEN);
            flags = FieldStatus_GetFlashPerms(status2);
            flags &= ~FLD_FLASH_ALLOW;
            flags |= FLD_FLASH_ACTIVE;
            FieldStatus_SetFlashPerms(status2, flags);
        } else {
            return GAMEEVENT_DONE;
        }
        ++*state;
        break;
    case EVENT_FLASH_STEP_WAIT_LIT:
        if (FieldFlash_GetState(flash) == FIELD_FLASH_STATE_LIT)
            return GAMEEVENT_DONE;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

u32 EventDigCall_Check(PlayerActionPossibilities *context) {
    if (HMEvent_CheckContextFlag(context, HM_EVENT_DIG) != 0) {
        if (HMEvent_CheckPairFlag(context) != 0)
            return POKELIST_HM_CHECK_WITH_PARTNER;
        return POKELIST_HM_CHECK_ALLOWED;
    }
    return POKELIST_HM_CHECK_UNAVAILABLE;
}

GameEvent *EventDigCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;
    GameBeacon_BroadcastFieldMove(MOVE_DIG);
    event = GameEvent_Create(context->gsys, NULL, EventDigCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

// EventDigCall_Callback's steps: the script call, then the map change
enum {
    EVENT_DIG_STEP_START_SCRIPT,
    EVENT_DIG_STEP_START_MAP_CHANGE,
};

GameEventReturnCode EventDigCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    ScriptWork *scriptWork;
    u16 param;

    switch (*state) {
    case EVENT_DIG_STEP_START_SCRIPT:
        param = work->args.partySlot;
        scriptWork = EventScriptCall_Start(event, 0x271a, NULL, 0, 0x15);
        ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
        ++*state;
        break;
    case EVENT_DIG_STEP_START_MAP_CHANGE:
        GameEvent_Replace(event, EventMapChangeDig_Create(work->gsys));
        break;
    }
    return GAMEEVENT_CONTINUE;
}

u32 EventTeleportCall_Check(PlayerActionPossibilities *context) {
    if (HMEvent_CheckContextFlag(context, HM_EVENT_TELEPORT) != 0) {
        if (HMEvent_CheckPairFlag(context) != 0)
            return POKELIST_HM_CHECK_WITH_PARTNER;
        return POKELIST_HM_CHECK_ALLOWED;
    }
    return POKELIST_HM_CHECK_UNAVAILABLE;
}

GameEvent *EventTeleportCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;
    GameBeacon_BroadcastFieldMove(MOVE_TELEPORT);
    event = GameEvent_Create(context->gsys, NULL, EventTeleportCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

// EventTeleportCall_Callback's steps: the script call, then the map change
enum {
    EVENT_TELEPORT_STEP_START_SCRIPT,
    EVENT_TELEPORT_STEP_START_MAP_CHANGE,
};

GameEventReturnCode EventTeleportCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    ScriptWork *scriptWork;
    u16 param;

    switch (*state) {
    case EVENT_TELEPORT_STEP_START_SCRIPT:
        param = work->args.partySlot;
        scriptWork = EventScriptCall_Start(event, 0x2719, NULL, 0, 0x15);
        ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
        ++*state;
        break;
    case EVENT_TELEPORT_STEP_START_MAP_CHANGE:
        GameEvent_Replace(event, EventMapChangeTeleport_Create(work->gsys));
        break;
    }
    return GAMEEVENT_CONTINUE;
}

u32 EventDivingCall_Check(PlayerActionPossibilities *context) {
    if (HMEvent_CheckContextFlag(context, HM_EVENT_DIVE) != 0) {
        if (HMEvent_CheckPairFlag(context) != 0)
            return POKELIST_HM_CHECK_WITH_PARTNER;
        return POKELIST_HM_CHECK_ALLOWED;
    }
    return POKELIST_HM_CHECK_UNAVAILABLE;
}

GameEvent *EventDivingCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;
    event = GameEvent_Create(context->gsys, NULL, EventDivingCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

GameEventReturnCode EventDivingCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    ScriptWork *scriptWork;
    u16 param;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    param = work->args.partySlot;
    scriptWork = EventScriptCall_Replace(event, 0x271c, 0, 0);
    ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
    return GAMEEVENT_CONTINUE;
}

u32 EventChatterCall_Check(PlayerActionPossibilities *context) {
    switch (HMEvent_CheckContextFlag(context, HM_EVENT_CHATTER)) {
    default:
        return FALSE;
    case FALSE:
        return TRUE;
    }
}

GameEvent *EventChatterCall_Create(HiddenEventArgs *args, PlayerActionPossibilities *context) {
    GameBeacon_BroadcastFieldMove(MOVE_CHATTER);
    return EventChatotRecord_Create(context->gsys, context->field, args->partySlot);
}

// EventRuinsStrengthCall_Callback's steps: the script call, then the script that pushes the boulder
enum {
    EVENT_RUINS_STRENGTH_STEP_START_SCRIPT,
    EVENT_RUINS_STRENGTH_STEP_REPLACE_SCRIPT,
};

GameEventReturnCode EventRuinsStrengthCall_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    ScriptWork *scriptWork;
    u16 param;
    HeapID heapId;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    heapId = Field_GetHeapID(GSYS_GetField(work->gsys));
    switch (*state) {
    case EVENT_RUINS_STRENGTH_STEP_START_SCRIPT:
        param = work->args.partySlot;
        scriptWork = EventScriptCall_Start(event, 0x271e, NULL, 0, heapId);
        ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
        ++*state;
        break;
    case EVENT_RUINS_STRENGTH_STEP_REPLACE_SCRIPT:
        EventScriptCall_Replace(event, 0x28ed, 0, 0);
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventRuinsStrengthCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;
    event = GameEvent_Create(context->gsys, NULL, EventRuinsStrengthCall_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

// EventRuinsFlash_Callback's steps: the script call, then the script that lights the ruins
enum {
    EVENT_RUINS_FLASH_STEP_START_SCRIPT,
    EVENT_RUINS_FLASH_STEP_REPLACE_SCRIPT,
};

GameEventReturnCode EventRuinsFlash_Callback(GameEvent *event, u32 *state, void *data) {
    HiddenEventData *work = data;
    ScriptWork *scriptWork;
    u16 param;
    HeapID heapId;

    GameData_GetEventWork(GSYS_GetGameData(work->gsys));
    heapId = Field_GetHeapID(GSYS_GetField(work->gsys));
    switch (*state) {
    case EVENT_RUINS_FLASH_STEP_START_SCRIPT:
        param = work->args.partySlot;
        scriptWork = EventScriptCall_Start(event, 0x271d, NULL, 0, heapId);
        ScriptWork_SetParams(scriptWork, param, 0, 0, 0);
        ++*state;
        break;
    case EVENT_RUINS_FLASH_STEP_REPLACE_SCRIPT:
        EventScriptCall_Replace(event, 0x28ec, 0, 0);
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventRuinsFlash_Create(HiddenEventArgs *param, PlayerActionPossibilities *context) {
    GameEvent *event;
    HiddenEventData *data;
    event = GameEvent_Create(context->gsys, NULL, EventRuinsFlash_Callback, 0x14);
    data = GameEvent_GetData(event);
    HiddenEventData_Init(data, param, context);
    return event;
}

HiddenCheckFunc GetHMEventCheckFunc(PlayerActionPossibilities *context, u32 kind) {
    if ((s32)kind >= HM_EVENT_MAX) {
        return NULL;
    }
    if (IsZoneAbyssalRuinsInside(context->zoneId)) {
        return HM_EVENTS_RUINS[kind].check;
    }
    return HM_EVENTS_NORMAL[kind].check;
}

HiddenCtorFunc GetHMEventCtorFunc(PlayerActionPossibilities *context, u32 kind) {
    if ((s32)kind >= HM_EVENT_MAX) {
        return NULL;
    }
    if (IsZoneAbyssalRuinsInside(context->zoneId)) {
        return HM_EVENTS_RUINS[kind].create;
    }
    return HM_EVENTS_NORMAL[kind].create;
}

void HiddenEventData_Init(HiddenEventData *data, HiddenEventArgs *param, PlayerActionPossibilities *context) {
    data->magic = 0x19740205;
    data->actor = context->actorInFront;
    data->args = *param;
    data->gsys = context->gsys;
}

u32 HMEvent_CheckContextFlag(PlayerActionPossibilities *context, u32 value) {
    u16 flags = context->flags;
    u32 mask = 1 << value;
    return (flags & mask) ? TRUE : FALSE;
}

BOOL HiddenArea_ContainsUnlessFlag(const HiddenArea *area, u16 x, u16 z, u16 flag, Field *field) {
    GameSystem *gsys;
    GameData *gameData;
    EventWork *eventWork;
    s32 dx;
    s32 dz;

    gsys = Field_GetGameSystem(field);
    gameData = GSYS_GetGameData(gsys);
    eventWork = GameData_GetEventWork(gameData);
    if (EventWork_FlagGet(eventWork, flag)) {
        return FALSE;
    }
    dx = area->x - x;
    dz = area->z - z;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx < area->width) {
        if (dz < 0) {
            dz = -dz;
        }
        if (dz < area->height) {
            return TRUE;
        }
    }
    return FALSE;
}
