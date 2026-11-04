#include "types.h"
#include "battle/battle_proc.h"
#include "battle/btl_setup.h"
#include "field/event_colosseum_battle.h"
#include "field/event_make.h"
#include "field/event_sound.h"
#include "field/field_event.h"
#include "field/union_oneself.h"
#include "gfl/heap.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "system/game_event.h"
#include "system/game_system.h"

// No rule picked yet
#define RULE_NONE 41

typedef struct {
    GameSystem *gsys;
    Field *field;
    BtlSetup setup;
    BattlePlayers *players;
} EventColosseumBattleWork;

// The event's steps: the battle music, the battle, and the field again. Steps 1 and 2 are never used
enum {
    COLOSSEUM_BATTLE_STEP_START,
    COLOSSEUM_BATTLE_STEP_PLAY_BGM = 3,
    COLOSSEUM_BATTLE_STEP_BATTLE,
    COLOSSEUM_BATTLE_STEP_POP_BGM,
    COLOSSEUM_BATTLE_STEP_OPEN_FIELD,
    COLOSSEUM_BATTLE_STEP_FADE_IN,
    COLOSSEUM_BATTLE_STEP_WAIT_BGM,
    COLOSSEUM_BATTLE_STEP_END,
};

static GameEventReturnCode EventColosseumBattle_Callback(GameEvent *event, u32 *state, void *data);
static void SetRuleIfNone(u32 *rule, u32 value);

static GameEventReturnCode EventColosseumBattle_Callback(GameEvent *event, u32 *state, void *data) {
    EventColosseumBattleWork *work = data;
    GameSystem *gsys = work->gsys;
    EventMakeArgs args;

    switch (*state) {
    case COLOSSEUM_BATTLE_STEP_START:
        *state = COLOSSEUM_BATTLE_STEP_PLAY_BGM;
        break;
    case COLOSSEUM_BATTLE_STEP_PLAY_BGM:
        // The battle's music, a u16 at 0x18 of the setup, which battle/btl_setup.h doesn't have yet
        GameEvent_ChainNext(event, EventBattleBGMPlay_Create(gsys, work->setup.fieldSituation.bgm));
        (*state)++;
        break;
    case COLOSSEUM_BATTLE_STEP_BATTLE:
        args.setup = &work->setup;
        args.players = work->players;
        args.markNetErrShown = FALSE;
        GameEvent_ChainNext(event, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(10), eventMakeFunc, &args));
        (*state)++;
        break;
    case COLOSSEUM_BATTLE_STEP_POP_BGM:
        BtlSetup_FreeContents(&work->setup);
        GameEvent_ChainNext(event, EventBGMFadePop_Create(gsys));
        (*state)++;
        break;
    case COLOSSEUM_BATTLE_STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case COLOSSEUM_BATTLE_STEP_FADE_IN:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case COLOSSEUM_BATTLE_STEP_WAIT_BGM:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case COLOSSEUM_BATTLE_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventColosseumBattle_Create(GameSystem *gsys, Field *field, u32 category, ColosseumBattleParam *param) {
    u32 rule = RULE_NONE;
    GameEvent *event;
    EventColosseumBattleWork *work;
    BtlSetup *setup;
    GameData *gameData;

    event = GameEvent_Create(gsys, NULL, EventColosseumBattle_Callback, sizeof(EventColosseumBattleWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = field;
    work->players = &param->players.battle;
    setup = &work->setup;
    switch (category) {
    case UNION_ACTIVITY_SINGLE_NO_LAUNCHER:
        SetRuleIfNone(&rule, 0);
    case UNION_ACTIVITY_SINGLE:
        SetRuleIfNone(&rule, 2);
    case UNION_ACTIVITY_SINGLE_FLAT_NO_LAUNCHER:
        SetRuleIfNone(&rule, 1);
    case UNION_ACTIVITY_SINGLE_FLAT:
        SetRuleIfNone(&rule, 3);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNet1v1Single(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case UNION_ACTIVITY_DOUBLE_NO_LAUNCHER:
        SetRuleIfNone(&rule, 4);
    case UNION_ACTIVITY_DOUBLE:
        SetRuleIfNone(&rule, 6);
    case UNION_ACTIVITY_DOUBLE_FLAT_NO_LAUNCHER:
        SetRuleIfNone(&rule, 5);
    case UNION_ACTIVITY_DOUBLE_FLAT:
        SetRuleIfNone(&rule, 7);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNet1v1Double(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case UNION_ACTIVITY_TRIPLE_NO_LAUNCHER:
        SetRuleIfNone(&rule, 8);
    case UNION_ACTIVITY_TRIPLE:
        SetRuleIfNone(&rule, 10);
    case UNION_ACTIVITY_TRIPLE_FLAT_NO_LAUNCHER:
        SetRuleIfNone(&rule, 9);
    case UNION_ACTIVITY_TRIPLE_FLAT:
        SetRuleIfNone(&rule, 11);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetTriple(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case UNION_ACTIVITY_ROTATION_NO_LAUNCHER:
        SetRuleIfNone(&rule, 12);
    case UNION_ACTIVITY_ROTATION:
        SetRuleIfNone(&rule, 14);
    case UNION_ACTIVITY_ROTATION_FLAT_NO_LAUNCHER:
        SetRuleIfNone(&rule, 13);
    case UNION_ACTIVITY_ROTATION_FLAT:
        SetRuleIfNone(&rule, 15);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetRotation(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case UNION_ACTIVITY_MULTI_NO_LAUNCHER:
        SetRuleIfNone(&rule, 16);
    case UNION_ACTIVITY_MULTI:
        SetRuleIfNone(&rule, 18);
    case UNION_ACTIVITY_MULTI_FLAT_NO_LAUNCHER:
        SetRuleIfNone(&rule, 17);
    case UNION_ACTIVITY_MULTI_FLAT:
        SetRuleIfNone(&rule, 19);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetMultiVsNet(setup, gameData, NetHandle_GetOwn(), 1, param->clientId, HEAPID_GAMEEVENT);
        break;
    default:
        GFL_ASSERT_MSG(FALSE, "play_category = %d\n", category);
        return NULL;
    }
    BtlSetup_SetParty(setup, param->party, 0);
    BtlSetup_ApplyRegulation(setup, param->regulation, HEAPID_GAMEEVENT);
    // The battle's music, as above
    setup->fieldSituation.bgm = param->bgm;
    work->players->rule = rule;
    work->players->unk4C = 0;
    BtlSetup_AllocRecBuffer(setup, 1);
    return event;
}

// Sets the rule, unless one is set
static void SetRuleIfNone(u32 *rule, u32 value) {
    if (*rule == RULE_NONE) {
        *rule = value;
    }
}
