// The Trial House's events: picking the Pokémon to enter, the battle, its statistics and the results screen. Function
// names from swan; the file's name is descriptive
#include "types.h"
#include "app/pokelist.h"
#include "app/th_award.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "field/battle_facility.h"
#include "field/event_battle.h"
#include "field/fld_btl_inst_event.h"
#include "field/trial_house.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/trial_house.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 unk08;
    // Set to whether Pokémon were picked
    u16 *result;
    // The Battle Test's type, TRIAL_HOUSE_BATTLE_*
    u32 battleType;
    u32 regulationId;
    PokeParty *party;
    // The Battle Box's party, made for the screen
    PokeParty *battleBoxParty;
    u32 choice;
    u32 screenResult;
    u8 picked[6];
    TrialHouseWork *work;
} TrialHousePokeSelectData;

typedef struct {
    GameSystem *gsys;
    Field *field;
    ThAwardParam param;
} TrialHouseResultsData;

static GameEventReturnCode EventTrialHouseTeamSelect_Callback(GameEvent *event, u32 *state, void *work);
static GameEventReturnCode EventTrialHouseResults_Callback(GameEvent *event, u32 *state, void *work);

// The battle type of the work, which MWCC loads before the other arguments only when it goes through a variable of an
// enum type
typedef enum {
    TRIAL_HOUSE_BATTLE_SINGLE,
    TRIAL_HOUSE_BATTLE_DOUBLE,
} TrialHouseBattleType;

// The regulations a Battle Test asks for, the single and the double: the Battle Subway's ids 20 and 21
// (BSUBWAY_RULE_*), which the scripts pass and CheckPartyRegulation checks
enum {
    TRIAL_HOUSE_REGULATION_SINGLE = 20,
    TRIAL_HOUSE_REGULATION_DOUBLE = 21,
};

GameEvent *CallTrialHouseTeamSelect(GameSystem *gsys, TrialHouseWork *work, u32 mode, BOOL battleBox, u16 *result) {
    GameEvent *event;
    GameData *gameData = GSYS_GetGameData(gsys);
    TrialHousePokeSelectData *data;

    event = GameEvent_Create(gsys, NULL, EventTrialHouseTeamSelect_Callback, sizeof(TrialHousePokeSelectData));
    data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = GSYS_GetField(gsys);
    data->result = result;
    data->work = work;
    if (mode == TRIAL_HOUSE_REGULATION_SINGLE || mode == TRIAL_HOUSE_REGULATION_DOUBLE) {
        if (mode == TRIAL_HOUSE_REGULATION_SINGLE) {
            data->battleType = TRIAL_HOUSE_BATTLE_SINGLE;
            data->regulationId = TRIAL_HOUSE_REGULATION_SINGLE;
        } else {
            data->battleType = TRIAL_HOUSE_BATTLE_DOUBLE;
            data->regulationId = TRIAL_HOUSE_REGULATION_DOUBLE;
        }
    } else {
        data->battleType = TRIAL_HOUSE_BATTLE_SINGLE;
        data->regulationId = TRIAL_HOUSE_REGULATION_SINGLE;
    }
    if (battleBox == FALSE) {
        data->battleBoxParty = NULL;
        data->party = GameData_GetParty(gameData);
    } else {
        data->party = data->battleBoxParty =
            convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(gameData)), HEAPID_TRIAL_HOUSE);
    }
    return event;
}

// EventTrialHouseTeamSelect_Callback's steps: the party screen, and its result
enum {
    TRIAL_HOUSE_TEAM_SELECT_STEP_PARTY_SCREEN,
    TRIAL_HOUSE_TEAM_SELECT_STEP_RESULT,
};

static GameEventReturnCode EventTrialHouseTeamSelect_Callback(GameEvent *event, u32 *state, void *work) {
    TrialHousePokeSelectData *data = work;
    GameSystem *gsys = data->gsys;

    switch (*state) {
    case TRIAL_HOUSE_TEAM_SELECT_STEP_PARTY_SCREEN:
        GameEvent_ChainNext(event,
                            EventBtlInstPokeSelect_Create(gsys, data->battleType, POKELIST_MODE_BATTLE_INSTITUTE,
                                                          data->regulationId, data->party, data->picked, &data->choice,
                                                          &data->screenResult, data->work->party));
        *state = 1;
        break;
    case TRIAL_HOUSE_TEAM_SELECT_STEP_RESULT:
        if (data->battleBoxParty != NULL) {
            GFL_HeapFree(data->battleBoxParty);
        }
        if (data->screenResult != 0 || data->choice == POKELIST_CURSOR_CANCEL ||
            data->choice == POKELIST_CURSOR_CLOSE) {
            *data->result = FALSE;
        } else {
            *data->result = TRUE;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *CallTrialHouseBattle(GameSystem *gsys, TrialHouseWork *work) {
    Field *field = GSYS_GetField(gsys);
    TrialHouseBattleType type = work->battleType;
    BtlSetup *setup = SetupTrialHouseBattle(gsys, work->party, type, &work->trainer, NULL, work->capacity);

    return CreateTrialHouseBattleEvent(gsys, field, setup);
}

void SyncTrialHouseWkStatsFromBattle(TrialHouseWork *work, BtlSetup *setup) {
    PokeParty *party;
    int count;
    int totalHp;
    int i;

    work->stats[0] += (u16)(setup->turnCount + 1);
    work->stats[1] += setup->switchCount;
    work->stats[2] += setup->playerImmuneHitCount;
    work->stats[3] += setup->playerSuperEffectiveHitCount;
    work->stats[4] += setup->playerNotVeryEffectiveHitCount;
    work->stats[5] += setup->foeImmuneHitCount;
    work->stats[6] += setup->foeNotVeryEffectiveHitCount;
    work->stats[7] += setup->won;
    work->stats[8] += setup->foeFaintedCount;
    work->stats[9] += setup->playerFaintedCount;
    work->stats[11] += setup->usedMoveKindCount;
    party = setup->party[0];
    count = PokeParty_GetPkmCount(party);
    totalHp = 0;
    for (i = 0; i < count; i++) {
        totalHp += PokeParty_GetParam(PokeParty_GetPkm(party, i), PKM_PARAM_MAX_HP, NULL);
    }
    work->stats[10] += (u16)(setup->playerHp * 100 / totalHp);
}

GameEvent *CallTrialHouseResults(GameSystem *gsys, u32 downloaded, u32 playJingle) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventTrialHouseResults_Callback, sizeof(TrialHouseResultsData));
    TrialHouseResultsData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = GSYS_GetField(gsys);
    data->param.gender = getTrainerGender(GetGameDataPlayerInfo(gameData));
    data->param.save = TrialHouseSave_Get(GameData_GetSaveControl(gameData));
    data->param.downloaded = downloaded;
    data->param.playJingle = playJingle;
    return event;
}

// EventTrialHouseResults_Callback's steps: the results screen, and its end
enum {
    TRIAL_HOUSE_RESULTS_STEP_SCREEN,
    TRIAL_HOUSE_RESULTS_STEP_END,
};

static GameEventReturnCode EventTrialHouseResults_Callback(GameEvent *event, u32 *state, void *work) {
    TrialHouseResultsData *data = GameEvent_GetData(event);

    switch (*state) {
    case TRIAL_HOUSE_RESULTS_STEP_SCREEN:
        GSYS_QueueProcAsEvent(event, OVERLAY_TH_AWARD, &TH_AWARD_PROC_FUNCTIONS, &data->param);
        (*state)++;
        break;
    case TRIAL_HOUSE_RESULTS_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
