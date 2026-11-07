#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/version.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/resort.h"
#include "field/scrcmd_resort.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"
#include "system/resort_work.h"
#include "system/vm.h"
#include "system/wordset.h"

// The script plugin of the Join Avenue (plugin 8), commands from 1000. Its command table is in overlay 58, so that
// the shops' overlay 60 can take this overlay's place

// The records window lifecycle; allocation clears the initial state to CREATE.
enum {
    RESORT_RECORDS_STATE_CREATE = 0,
    RESORT_RECORDS_STATE_PRINT = 1,
    RESORT_RECORDS_STATE_WAIT_PRINT = 2,
    RESORT_RECORDS_STATE_INPUT = 3,
    RESORT_RECORDS_STATE_CLOSE = 4,
    RESORT_RECORDS_STATE_FINISH = 5,
};

// The page of records that Scrcmd_Resort_ShowRecords shows, while the script waits
typedef struct {
    MoneyWin *window;
    s16 page;
    u16 state;
    u16 *var;
} ResortRecordsWork;

// By version: Black 2's row, then White 2's
static const u16 sPlayerShopIdsByVersion[2][8] = {
    { 161, 171, 181, 191, 201, 211, 221, 231 },
    { 241, 251, 261, 271, 281, 291, 301, 311 },
};

static ResortPeople *ResortCmd_GetPeople(Field *field);
static u16 ResortCmd_GetActorUID(FieldScriptEnv *env);
static ResortNPC *ResortCmd_GetNPC(Field *field);
static int ResortCmd_ApplyEffortChange(int value, int others, int change);

BOOL Scrcmd_Resort_StartTalk(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ResortPerson *person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(gsys)));

    ResortPerson_SetParam(person, JOIN_AVE_PARAM_TALKING, 1);
    ResortPerson_SetTalking(person, 1);
    if (checkForMidnight(GSYS_GetGameData(gsys)) && ResortPerson_GetAttr(person, 0) == 0) {
        JoinAvenueShop_SetField(ResortPerson_GetShop(person), 3, -1);
    }
    return FALSE;
}

BOOL Scrcmd_Resort_EndTalk(VM *vm, FieldScriptEnv *env) {
    ResortPerson *person =
        ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));

    if (person != NULL) {
        ResortPerson_SetParam(person, JOIN_AVE_PARAM_TALKING, 0);
        ResortPerson_SetTalking(person, 0);
    }
    return FALSE;
}

// Sets a variable to one of 65 things about the avenue and the person the script is about
BOOL Scrcmd_Resort_GetValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u32 which = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    ResortSys *sys = ResortSys_Get(field);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortPeople *people = ResortCmd_GetPeople(field);
    void *shops = ResortSys_GetShopData(sys);
    void *table = ResortSys_GetRanks(sys);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);
    ResortPersonData *data = NULL;
    GameData *gameData = GSYS_GetGameData(gsys);
    void *texts;
    int i;

    GetGameDataPlayerInfo(gameData);
    SaveControl_GetMedalBox(GameData_GetSaveControl(gameData));
    texts = ResortSys_GetTexts(sys);
    if (person != NULL) {
        data = ResortPerson_GetData(person);
    }
    switch (which) {
    case RESORT_VALUE_ACTOR_UID:
        *var = (u16)ResortPerson_GetParam(person, JOIN_AVE_PARAM_SLOT, NULL) + 80;
        break;
    case 1: {
        JoinAvenueRecord *a = JoinAvenueOccupants_GetEmptyRecord(occupants);
        u32 b = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_5, 0);

        *var = a == NULL && b != 0 ? TRUE : FALSE;
        break;
    }
    case RESORT_VALUE_CAN_CHANGE_SHOP: {
        u16 a = ResortPerson_GetParam(person, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 b = ResortPerson_GetParam(person, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 c = ResortPerson_GetParam(person, JOIN_AVE_PARAM_VERSION, NULL);
        u16 d = ResortPerson_GetParam(person, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, NULL);
        u16 id1 = JoinAvenue_ChooseShop(a, b, c, d, info, shops);
        u16 id2 = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);
        const u16 *row1 = ResortShopData_GetShop(shops, id1);
        const u16 *row2 = ResortShopData_GetShop(shops, id2);
        u16 kind1 = ResortShopData_GetShopParam(row1, 0);
        u16 kind2 = ResortShopData_GetShopParam(row2, 0);
        u32 shopChangeBlocked = ResortPerson_GetParam(person, JOIN_AVE_PARAM_SHOP_CHANGE_BLOCKED, NULL);
        u32 shopKind5Unlocked = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_6, 0);

        if (kind1 == 5 && shopKind5Unlocked == 0) {
            shopChangeBlocked = 1;
        }
        *var = kind1 != kind2 && shopChangeBlocked == 0 ? TRUE : FALSE;
        break;
    }
    case RESORT_VALUE_RAFFLE_SHOP_ITEM:
        if (ResortPerson_GetParam(person, JOIN_AVE_PARAM_RAFFLED, NULL) == 0) {
            *var = ResortData_RaffleShopItem(data, ResortSys_GetRaffles(sys), ResortSys_GetRanks(sys), info);
            ResortPerson_SetParam(person, JOIN_AVE_PARAM_RAFFLED, 1);
        } else {
            *var = 0;
        }
        break;
    case RESORT_VALUE_NO_OCCUPANTS:
        *var = JoinAvenueOccupants_CountPeople(occupants) == 0 ? TRUE : FALSE;
        break;
    case 7:
        *var = ResortPerson_GetParam(person, JOIN_AVE_PARAM_UNK_32, NULL);
        break;
    case RESORT_VALUE_PERSON_SHOP_KIND:
        *var = ResortPerson_GetParam(person, JOIN_AVE_PARAM_SHOP_KIND, NULL);
        break;
    case RESORT_VALUE_SHOP_FLAG:
        *var = JoinAvenueShop_GetFlag(JoinAvenuePerson_GetShop(ResortData_GetPerson(data)), 0);
        break;
    case RESORT_VALUE_ALL_OCCUPANTS:
        *var = JoinAvenueOccupants_CountPeople(occupants) == 8 ? TRUE : FALSE;
        break;
    case RESORT_VALUE_CURRENT_SHOP_KIND: {
        u16 id = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);

        *var = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 0);
        break;
    }
    case RESORT_VALUE_RECENT_ACTIVITY_COUNT: {
        BOOL flag = JoinAvenuePerson_HasExtraData(ResortPerson_GetJoinAvenuePerson(person), 4);
        JoinAvenuePersonParam param;

        *var = 0;
        if (flag) {
            for (param = 80; param <= 83; param++) {
                if (ResortPerson_GetParam(person, param, NULL) != 0) {
                    (*var)++;
                }
            }
        }
        break;
    }
    case RESORT_VALUE_HAS_MEDAL: {
        BOOL flag = JoinAvenuePerson_HasExtraData(ResortPerson_GetJoinAvenuePerson(person), 0);
        u32 lastMedal = ResortPerson_GetParam(person, JOIN_AVE_PARAM_LAST_MEDAL, NULL);

        *var = flag && lastMedal != 255 ? TRUE : FALSE;
        break;
    }
    case RESORT_VALUE_RANK_COMPARISON: {
        u32 a = ResortPerson_GetParam(person, JOIN_AVE_PARAM_RANK, NULL);
        u32 b = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0);

        if (a == 0) {
            *var = 2;
        } else if (a <= b) {
            *var = 0;
        } else {
            *var = 1;
        }
        break;
    }
    case RESORT_VALUE_HAS_SHOP_KIND_0:
        *var = ResortSys_HasShopKind(sys, gameData, 0);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_1:
        *var = ResortSys_HasShopKind(sys, gameData, 1);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_3:
        *var = ResortSys_HasShopKind(sys, gameData, 3);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_2:
        *var = ResortSys_HasShopKind(sys, gameData, 2);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_4:
        *var = ResortSys_HasShopKind(sys, gameData, 4);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_5:
        *var = ResortSys_HasShopKind(sys, gameData, 5);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_6:
        *var = ResortSys_HasShopKind(sys, gameData, 6);
        break;
    case RESORT_VALUE_HAS_SHOP_KIND_7:
        *var = ResortSys_HasShopKind(sys, gameData, 7);
        break;
    case RESORT_VALUE_OCCUPANT_0_PRESENT:
    case RESORT_VALUE_OCCUPANT_1_PRESENT:
    case RESORT_VALUE_OCCUPANT_2_PRESENT:
    case RESORT_VALUE_OCCUPANT_3_PRESENT:
    case RESORT_VALUE_OCCUPANT_4_PRESENT:
    case RESORT_VALUE_OCCUPANT_5_PRESENT:
    case RESORT_VALUE_OCCUPANT_6_PRESENT:
    case RESORT_VALUE_OCCUPANT_7_PRESENT:
        which -= RESORT_VALUE_OCCUPANT_0_PRESENT;
        *var = JoinAvenuePerson_IsEmpty(JoinAvenueOccupants_GetPerson(occupants, which)) == FALSE ? TRUE : FALSE;
        break;
    case 33:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_4, 0);
        break;
    case 34:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_3, 0);
        break;
    case RESORT_VALUE_SHOP_KIND_0_RECORD_GENDER: {
        u32 value = 0;

        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 0) {
                value = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_GENDER, NULL);
                break;
            }
        }
        *var = value;
        break;
    }
    case RESORT_VALUE_SHOP_KIND_2_RECORD_GENDER: {
        u32 value = 0;

        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 2) {
                value = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_GENDER, NULL);
                break;
            }
        }
        *var = value;
        break;
    }
    case RESORT_VALUE_SHOP_KIND_0_RECORD_INDEX:
        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 0) {
                *var = i;
                break;
            }
        }
        break;
    case RESORT_VALUE_SHOP_KIND_2_RECORD_INDEX:
        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 2) {
                *var = i;
                break;
            }
        }
        break;
    case 39:
        *var = ResortSys_GetTrainerTypeText(texts, data, 10);
        break;
    case RESORT_VALUE_DATA_INDEX:
        *var = ResortData_GetIndex(data);
        break;
    case RESORT_VALUE_PERSON_TYPE:
        *var = ResortPerson_GetAttr(person, RESORT_PERSON_ATTR_TYPE);
        break;
    case RESORT_VALUE_OCCUPANT_0_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_1_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_2_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_3_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_4_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_5_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_6_ACTOR_UID:
    case RESORT_VALUE_OCCUPANT_7_ACTOR_UID:
        which -= RESORT_VALUE_OCCUPANT_0_ACTOR_UID;
        *var = (u16)joinAveTextHandler(JoinAvenueOccupants_GetPerson(occupants, which), JOIN_AVE_PARAM_SLOT, NULL) + 80;
        break;
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59: {
        u16 id;

        which -= 52;
        id = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(JoinAvenueOccupants_GetPerson(occupants, which)), 0);
        *var = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 5);
        break;
    }
    case RESORT_VALUE_IS_ENTRY_SHOP_KIND_5: {
        u16 kind = ResortData_GetType(data);
        u16 shopKind = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);

        *var = kind == RESORT_PERSON_ENTRY && shopKind == 5 ? TRUE : FALSE;
        break;
    }
    case 61:
        *var = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0)),
                                11);
        break;
    case 62:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_7, 0);
        break;
    case 63:
        *var = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0)),
                                6) -
               1753;
        break;
    case RESORT_VALUE_OCCUPANT_COUNT:
        *var = JoinAvenueOccupants_CountPeople(occupants);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_GetScriptValue(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 query = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    ResortPeople *people = ResortCmd_GetPeople(field);
    HeapID heapId = Field_GetHeapID(field);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    ResortSys *sys = ResortSys_Get(field);
    ResortPersonData *data = NULL;
    GameData *gameData = GSYS_GetGameData(gsys);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);

    if (person != NULL) {
        data = ResortPerson_GetData(person);
    }
    *var = ResortSys_GetScriptValue(query, sys, data, gameData, wordSet, heapId);
    return FALSE;
}

BOOL Scrcmd_Resort_MakeData(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 mode = VM_Read16(vm);
    u16 *var = ScriptReadVar(vm, env);
    ResortSys *sys = ResortSys_Get(field);
    ResortPerson *person;
    ResortPersonData *data;

    ResortSys_GetSlots(sys);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    data = ResortPerson_GetData(person);
    switch (mode) {
    case RESORT_MAKE_DATA_OCCUPANT_MOVE_PLAYER: {
        VecFx32 pos;
        u16 x;
        u16 z;
        u16 dir;
        FieldPlayer *player;

        *var = ResortSys_MakeData(sys, data, RESORT_PERSON_OCCUPANT);
        player = Field_GetPlayer(field);
        ResortPerson_MoveToHome(person);
        ResortPerson_GetFacingPos(person, &x, &z, &dir);
        x = x <= 11 ? 11 : 19;
        FieldPlayer_GetWPos(player, &pos);
        pos.x = x * (16 * FX32_ONE) + 8 * FX32_ONE;
        pos.z = z * (16 * FX32_ONE) + 8 * FX32_ONE;
        FieldPlayer_SetWPos(player, &pos);
        FieldPlayer_SetDirection(player, dir);
        break;
    }
    case RESORT_MAKE_DATA_OCCUPANT:
        *var = ResortSys_MakeData(sys, data, RESORT_PERSON_OCCUPANT);
        break;
    case RESORT_MAKE_DATA_RECORD:
        *var = ResortSys_MakeData(sys, data, RESORT_PERSON_RECORD);
        break;
    }
    ResortPerson_SetParam(person, JOIN_AVE_PARAM_SHOP_CHANGE_BLOCKED, 1);
    return FALSE;
}

BOOL Scrcmd_Resort_SetWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    HeapID heapId = Field_GetHeapID(field);
    u16 type = VM_Read16(vm);
    u16 who = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u16 wordIndex = VM_Read16(vm);
    ResortSys *sys = ResortSys_Get(field);
    ResortPerson *person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));

    ResortSys_SetScriptWord(type, who, index, wordIndex, sys, person, wordSet, GSYS_GetGameData(gsys), heapId);
    return FALSE;
}

BOOL Scrcmd_Resort_RemovePerson(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortSys *sys;
    ResortPeople *people;
    void *datas;
    void *slots;
    u16 mode;
    int which;
    u16 index;
    ResortPerson *person;
    ResortPersonData *data;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    sys = ResortSys_Get(field);
    people = ResortCmd_GetPeople(field);
    datas = ResortSys_GetDatas(sys);
    ResortSys_GetOccupants(sys);
    slots = ResortSys_GetSlots(sys);
    mode = ScriptReadAny(vm, env);
    which = ScriptReadAny(vm, env);
    index = ScriptReadAny(vm, env);
    person = NULL;
    data = NULL;
    switch (which) {
    case RESORT_REMOVE_CURRENT_PERSON:
        person = ResortSys_GetScriptPerson(env, people);
        data = ResortPerson_GetData(person);
        break;
    case RESORT_PERSON_OCCUPANT:
    case RESORT_PERSON_ENTRY:
    case RESORT_PERSON_VISITOR:
    case RESORT_PERSON_RECORD:
        data = ResortDatas_GetByScript(datas, which, index);
        person = ResortPeople_FindByData(people, data);
        break;
    }
    if (person != NULL) {
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            ResortField_RemoveWalker(field, person);
        }
        ResortPeople_Delete(people, person);
    }
    if (data != NULL) {
        if (mode == 0) {
            ResortData_Clear(data);
        } else {
            ResortData_SetParam(data, JOIN_AVE_PARAM_SLOT, 0);
            ResortData_SetParam(data, JOIN_AVE_PARAM_ROW, 0);
        }
    }
    ResortSlots_Fill(slots);
    return FALSE;
}

BOOL Scrcmd_Resort_SaveShopId(VM *vm, FieldScriptEnv *env) {
    ResortPerson *person;
    u16 value;

    FieldScriptEnv_GetScriptWork(env);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    value = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);
    ResortPerson_SetParam(person, JOIN_AVE_PARAM_SHOP_ID, value);
    return FALSE;
}

BOOL Scrcmd_Resort_RaffleShopItem(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortSys *sys;
    void *shops;
    void *table;
    u16 index;
    u16 *prizeValueOut;
    u16 *prizeNumberOut;
    ResortPerson *person;
    ResortPersonData *data;
    const u16 *row;
    u16 id;
    u32 prize;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    sys = ResortSys_Get(field);
    shops = ResortSys_GetShopData(sys);
    table = ResortSys_GetRaffles(sys);
    index = ScriptReadAny(vm, env);
    prizeValueOut = ScriptReadVar(vm, env);
    prizeNumberOut = ScriptReadVar(vm, env);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    data = ResortPerson_GetData(person);
    *prizeNumberOut = 0;
    row = ResortShopData_GetGoods(shops, ResortShopData_GetPersonShop(shops, ResortPerson_GetJoinAvenuePerson(person)),
                                  0);
    id = ResortShopData_GetGoodsParam(row, 6);
    ResortShopData_GetGoodsParam(row, 5);
    prize = join_ave_raffle_shop(table, id, ResortData_Roll(data, index + 5, ResortBinary_Get(table, id, 0)));
    *prizeNumberOut = prize + 1;
    if (prize != RESORT_RAFFLE_PRIZE_COUNT) {
        *prizeValueOut = ResortBinary_Get(table, id, prize * 2 + 3);
    }
    return FALSE;
}

// Sets a Pokémon's friendship
BOOL Scrcmd_Resort_SetFriendship(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 slot;
    u16 value;
    u16 *var;
    PartyPkm *pkm;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    value = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    if (PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL) == 255) {
        *var = FALSE;
    } else {
        setPkmBattleData(pkm, PKM_PARAM_HAPPINESS, value);
        *var = TRUE;
    }
    return FALSE;
}

// Raises a Pokémon's level, up to 100
BOOL Scrcmd_Resort_RaiseLevel(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 slot;
    s16 levels;
    u16 *var;
    PartyPkm *pkm;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    levels = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    if (PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL) < 100) {
        u32 level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
        u32 species;
        u32 exp;

        level += levels;
        if (level > 100) {
            level = 100;
        }
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        exp = PML_UtilGetPkmLvExp(species, PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), level);
        JoinAvenue_RecordActivity(9);
        PokeParty_SetParam(pkm, PKM_PARAM_EXP, exp);
        PokeParty_RecalcStats(pkm);
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Raises or, for stats from 6, lowers one of a Pokémon's effort values
BOOL Scrcmd_Resort_ChangeEffortValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 slot;
    u16 stat;
    s32 change;
    u16 *var;
    PartyPkm *pkm;
    int total;
    int current;
    int i;
    int value;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    stat = VM_Read16(vm);
    change = (s16)ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    if (stat >= 6) {
        stat -= 6;
        change *= -1;
    }
    total = 0;
    for (i = 0; i < 6; i++) {
        total += PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
        if (i == stat) {
            current = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
        }
    }
    value = ResortCmd_ApplyEffortChange(current, total - current, change);
    if (value != -1) {
        PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + stat, value);
        PokeParty_RecalcStats(pkm);
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Shortens the steps an egg of the party needs to hatch, by the shop's rate
BOOL Scrcmd_Resort_ShortenEggSteps(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    u16 index;
    u16 slot;
    u16 *var;
    ResortPerson *person;
    ResortPersonData *data;
    u32 rate;
    void *shops;
    void *table;
    const u16 *row;
    u16 id;
    u32 prize;
    PartyPkm *pkm;
    u32 species;
    u32 form;
    PersonalData *personal;
    u32 cycles;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    index = ScriptReadAny(vm, env);
    slot = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    data = ResortPerson_GetData(person);
    rate = 0;
    shops = ResortSys_GetShopData(sys);
    table = ResortSys_GetRaffles(sys);
    row = ResortShopData_GetGoods(shops, ResortShopData_GetPersonShop(shops, ResortPerson_GetJoinAvenuePerson(person)),
                                  index);
    id = ResortShopData_GetGoodsParam(row, 6);
    ResortShopData_GetGoodsParam(row, 5);
    prize = join_ave_raffle_shop(table, id, ResortData_Roll(data, index + 5, ResortBinary_Get(table, id, 0)));
    if (prize != RESORT_RAFFLE_PRIZE_COUNT) {
        rate = ResortBinary_Get(table, id, prize * 2 + 3);
    }
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    personal = PML_PersonalLoad(species, form, Field_GetHeapID(field));
    cycles = PML_PersonalGetParam(personal, PERSONAL_HATCH_CYCLES);
    PML_PersonalFree(personal);
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
        int steps = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
        u32 left = 0;

        steps = steps - (int)(cycles * rate) / 100;
        if (steps > 0) {
            left = steps;
        }
        PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, left);
        *var = left;
    } else {
        *var = 200;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_SetShopFlag(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    JoinAvenueShop_SetFlag(JoinAvenuePerson_GetShop(ResortPerson_GetJoinAvenuePerson(ResortSys_GetScriptPerson(
                               env, ResortCmd_GetPeople(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)))))),
                           0, TRUE);
    return FALSE;
}

// Takes ten times the amount from the player's money, and sets a flag of the person
BOOL Scrcmd_Resort_PayAndSetShopFlag(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    ResortPeople *people = ResortCmd_GetPeople(field);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 amount = ScriptReadAny(vm, env);
    u16 bit = ScriptReadAny(vm, env);
    u32 cost = amount * 10;
    GameRecords *records;
    TrainerCardSave *card;

    card = getTrainerCardDataBlkAddress(gameData);
    subCashFromTotal(card, cost);
    records = GameData_GetRecords(gameData);
    RecordAdd(records, 0x16, cost);
    RecordAddOne(records, 0x15);
    WordSetNumber(wordSet, 1, cost, 6, 0, 1);
    JoinAvenueShop_SetFlag(JoinAvenuePerson_GetShop(ResortPerson_GetJoinAvenuePerson(person)), bit, TRUE);
    return FALSE;
}

BOOL Scrcmd_Resort_GetPersonVisit(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortPerson *person;
    ResortSys *sys;
    JoinAvenueOccupants *occupants;
    u16 index;
    u16 *foundOut;
    u16 *messageOut;
    u16 *shopMessageOut;
    u16 *pointsOut;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    sys = ResortSys_Get(field);
    occupants = ResortSys_GetOccupants(sys);
    Field_GetHeapID(field);
    index = ScriptReadAny(vm, env);
    foundOut = ScriptReadVar(vm, env);
    messageOut = ScriptReadVar(vm, env);
    shopMessageOut = ScriptReadVar(vm, env);
    pointsOut = ScriptReadVar(vm, env);
    *foundOut = ResortSys_GetPersonVisit(ResortPerson_GetData(person), JoinAvenueOccupants_GetPerson(occupants, index),
                                         sys, messageOut, shopMessageOut, pointsOut);
    return FALSE;
}

BOOL Scrcmd_Resort_AddShopExperience(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    JoinAvenueOccupants *occupants;
    u16 index;
    u16 points;
    u16 *var;
    u16 *levelOut;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    sys = ResortSys_Get(field);
    occupants = ResortSys_GetOccupants(sys);
    index = ScriptReadAny(vm, env);
    points = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    levelOut = ScriptReadVar(vm, env);
    *var = ResortSys_AddShopExperience(JoinAvenueOccupants_GetPerson(occupants, index), sys, points, levelOut);
    return FALSE;
}

BOOL Scrcmd_Resort_AddPopularity(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    ResortSys *sys;
    u16 points;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    sys = ResortSys_Get(field);
    ResortSys_GetOccupants(sys);
    points = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    *var = ResortSys_AddPopularity(sys, gameData, points, ScriptReadVar(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_SetOccupantsLocked(VM *vm, FieldScriptEnv *env) {
    JoinAvenueOccupants *occupants;

    FieldScriptEnv_GetScriptWork(env);
    occupants = ResortSys_GetOccupants(ResortSys_Get(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    JoinAvenueOccupants_SetLocked(occupants, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_HideActors(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortPeople *people;
    void *npc;
    ResortPerson *person;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    people = ResortCmd_GetPeople(field);
    npc = ResortCmd_GetNPC(field);
    person = ResortSys_GetScriptPerson(env, people);
    switch (ScriptReadAny(vm, env)) {
    case RESORT_HIDE_ACTORS_EXCEPT_SCRIPT_PERSON:
        ResortSys_HideOverlappingActors(people, npc, field, person, RESORT_HIDE_WALKERS);
        break;
    case RESORT_HIDE_ACTORS_ALL_OVERLAPPING:
        ResortSys_HideOverlappingActors(people, npc, field, NULL, RESORT_HIDE_WALKERS);
        break;
    case RESORT_HIDE_ACTORS_PEOPLE:
        ResortSys_HideOverlappingActors(people, npc, field, NULL, RESORT_HIDE_PEOPLE);
        break;
    case RESORT_HIDE_ACTORS_CAMERA_FILTERED:
        ResortSys_HideOverlappingActors(people, npc, field, NULL, RESORT_HIDE_CAMERA_FILTERED);
        break;
    case RESORT_HIDE_ACTORS_RESTORE:
        ResortPeople_ApplyVisibility(people);
        ResortNPC_SetVisible(npc, 1);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_Visit(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    ResortPersonData *data;
    ResortWork *resortWork;
    void *texts;
    void *datas;
    u16 argument;
    u16 mode;
    u16 *var;
    u16 *targetIndexOut;
    u16 kind;
    u16 shopKind;
    BOOL flag;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    ResortSys_GetInfo(sys);
    data = ResortPerson_GetData(ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field)));
    resortWork = ResortSys_GetWork(sys);
    texts = ResortSys_GetTexts(sys);
    datas = ResortSys_GetDatas(sys);
    argument = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    targetIndexOut = ScriptReadVar(vm, env);
    kind = ResortData_GetType(data);
    shopKind = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    flag = FALSE;
    if (kind == RESORT_PERSON_ENTRY && shopKind == 5) {
        flag = TRUE;
    }
    switch (mode) {
    case RESORT_VISIT_CHECK: {
        ResortPersonData *other = ResortDatas_Get(datas, RESORT_DATA_OCCUPANT, argument);
        u16 visitCount = ResortWork_Get(ResortSys_GetWork(sys), RESORT_WORK_VISITS);
        u32 score;
        u32 value;
        u32 limit;

        ResortSys_GetOccupants(sys);
        score = ResortData_RollPair(other, data, 27, 100);
        if (checkForMidnight(GSYS_GetGameData(gsys))) {
            *var = 1;
            return FALSE;
        }
        if (flag) {
            *var = 1;
            return FALSE;
        }
        if (!ResortSys_IsFull(sys)) {
            *var = 1;
            return FALSE;
        }
        if (!ResortSys_PickPersonToMeet(sys, other, data, &value)) {
            *var = 1;
            return FALSE;
        }
        if (visitCount == 0) {
            limit = ResortSys_GetVisitPoints(sys, other);
        } else {
            limit = ResortSys_GetVisitScore(sys, other);
        }
        if (score < limit) {
            *var = 0;
        } else if (score < limit + limit / 2) {
            *var = 2;
        } else {
            *var = 1;
        }
        *targetIndexOut = value;
        return FALSE;
    }
    case RESORT_VISIT_SCALE_POINTS:
        *var = (argument * (((u16)ResortWork_Get(resortWork, RESORT_WORK_VISITS) + 1) * (FX32_ONE / 4) + FX32_ONE)) >>
               FX32_SHIFT;
        break;
    case RESORT_VISIT_ROLL_ACCEPTANCE:
        *var = ResortData_Roll(data, 26, 100) < 85 ? TRUE : FALSE;
        break;
    case RESORT_VISIT_GET_MESSAGE: {
        u16 gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);

        if (argument != 0) {
            if (flag) {
                *var = ResortSys_GetTrainerTypeText(texts, data, 15);
            } else {
                *var = 253;
            }
        } else if (flag) {
            *var = ResortSys_GetTrainerTypeText(texts, data, 16);
        } else if (gender == GENDER_MALE) {
            *var = 256;
        } else {
            *var = 257;
        }
        break;
    }
    case RESORT_VISIT_CHOOSE_GROUP:
        ResortSys_ChooseVisitGroup(sys, ResortDatas_Get(datas, RESORT_DATA_OCCUPANT, argument), data);
        break;
    case RESORT_VISIT_MARK_MET:
        ResortSys_MarkMet(sys, ResortDatas_Get(datas, RESORT_DATA_OCCUPANT, argument), argument);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_CheckRank100Unlock(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    ResortSys *sys;
    JoinAvenueInfo *info;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    sys = ResortSys_Get(GSYS_GetField(gsys));
    info = ResortSys_GetInfo(sys);
    var = ScriptReadVar(vm, env);
    *var = FALSE;
    if (JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0) >= 100) {
        u32 result = ResortSys_GetUnlockForRank(sys, gameData, 100);

        if (result != 0 && result != 8) {
            JoinAvenueInfo_SetParam(info, result + JOIN_AVE_INFO_UNLOCK_PENDING_1 - 1, 1);
            *var = TRUE;
        }
    }
    return FALSE;
}

BOOL Scrcmd_Resort_NameEntry(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 target = VM_Read16(vm);
    u16 useCurrent = VM_Read16(vm);

    ScriptWork_CallEvent(work, JoinAvenueNameEntry_Create(target, gsys, 0, useCurrent));
    return TRUE;
}

BOOL Scrcmd_Resort_MarkRaffled(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    ResortPerson_SetParam(ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(gsys))),
                          JOIN_AVE_PARAM_RAFFLED, 1);
    return FALSE;
}

BOOL Scrcmd_Resort_GetNPCValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    void *npc;
    u16 id;
    u16 mode;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    npc = ResortCmd_GetNPC(GSYS_GetField(gsys));
    id = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    switch (mode) {
    case RESORT_NPC_VALUE_ACTOR_UID:
        *var = ResortCmd_GetActorUID(env);
        break;
    case RESORT_NPC_VALUE_ACTOR_INDEX:
        *var = ResortCmd_GetActorUID(env) - 148;
        break;
    case RESORT_NPC_VALUE_TEXT_COLUMN_5:
        *var = ResortNPC_GetTextEntry(npc, id - 148, 5);
        break;
    case RESORT_NPC_VALUE_TEXT_COLUMN_6:
        *var = ResortNPC_GetTextEntry(npc, id - 148, 6);
        break;
    case RESORT_NPC_VALUE_TEXT_0:
    case RESORT_NPC_VALUE_TEXT_1:
    case RESORT_NPC_VALUE_TEXT_2:
    case RESORT_NPC_VALUE_TEXT_3:
    case RESORT_NPC_VALUE_TEXT_4:
    case RESORT_NPC_VALUE_TEXT_5:
    case RESORT_NPC_VALUE_TEXT_6:
    case RESORT_NPC_VALUE_TEXT_7:
    case RESORT_NPC_VALUE_TEXT_8:
    case RESORT_NPC_VALUE_TEXT_9:
    case RESORT_NPC_VALUE_TEXT_10:
    case RESORT_NPC_VALUE_TEXT_11:
        mode -= RESORT_NPC_VALUE_TEXT_0;
        *var = ResortNPC_GetTextEntry(npc, id - 148, mode + 7);
        break;
    case RESORT_NPC_VALUE_TEXT_0_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_1_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_2_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_3_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_4_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_5_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_6_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_7_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_8_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_9_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_10_OFFSET_59:
    case RESORT_NPC_VALUE_TEXT_11_OFFSET_59:
        mode -= RESORT_NPC_VALUE_TEXT_0_OFFSET_59;
        *var = ResortNPC_GetTextEntry(npc, id - 148, mode + 7) + 59;
        break;
    case RESORT_NPC_VALUE_TEXT_COLUMN_0:
    case RESORT_NPC_VALUE_TEXT_COLUMN_1:
    case RESORT_NPC_VALUE_TEXT_COLUMN_2:
    case RESORT_NPC_VALUE_TEXT_COLUMN_3:
    case RESORT_NPC_VALUE_TEXT_COLUMN_4:
        mode -= RESORT_NPC_VALUE_TEXT_COLUMN_0;
        *var = ResortNPC_GetTextEntry(npc, id - 148, mode);
        break;
    case RESORT_NPC_VALUE_TEXT_ROW:
        *var = ResortNPC_GetTextRow(npc, id - 148);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_SetOccupantWords(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    Field *field;
    u16 param;
    u16 n;
    u16 *var;
    JoinAvenueOccupants *occupants;
    int count;
    HeapID heapId;
    int i;
    JoinAvenuePerson *person;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    param = ScriptReadAny(vm, env);
    n = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    occupants =
        getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    count = 0;
    heapId = Field_GetHeapID(field);
    *var = 0;
    for (i = 7; i >= 0; i--) {
        person = JoinAvenueOccupants_GetPerson(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(person) && joinAveTextHandler(person, param + JOIN_AVE_PARAM_VALUE, NULL) != 0) {
            count++;
        }
    }
    if (count == 0) {
        return FALSE;
    }
    if (n >= count) {
        return FALSE;
    }
    i = 7;
    while (TRUE) {
        person = JoinAvenueOccupants_GetPerson(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(person)) {
            u32 value = joinAveTextHandler(person, param + JOIN_AVE_PARAM_VALUE, NULL);

            if (value != 0) {
                if (n == 0) {
                    WordSet *wordSet = ScriptWork_GetWordSet(work);
                    StrBuf *strbuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
                    u32 gender = joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL);
                    u16 text[64];

                    joinAveTextHandler(person, JOIN_AVE_PARAM_NAME, text);
                    GFL_StrBufLoadString(strbuf, text);
                    GFL_WordSetSetStrbuf(wordSet, 0, strbuf, gender, 1, 2);
                    GFL_StrBufFree(strbuf);
                    strbuf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
                    joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE1, text);
                    GFL_StrBufLoadString(strbuf, text);
                    GFL_WordSetSetStrbuf(wordSet, 1, strbuf, 2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    strbuf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
                    joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE2, text);
                    GFL_StrBufLoadString(strbuf, text);
                    GFL_WordSetSetStrbuf(wordSet, 2, strbuf, 2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    *var = value;
                    break;
                }
                n--;
            }
        }
        if (--i < 0) {
            i = 7;
        }
    }
    return FALSE;
}

BOOL Scrcmd_Resort_SetNPCWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field;
    u16 id;
    u16 column;
    u16 index;
    WordSet *wordSet;
    HeapID heapId;
    void *npc;
    MsgData *msgData;
    StrBuf *strbuf;

    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    id = ScriptReadAny(vm, env);
    column = ScriptReadAny(vm, env);
    index = VM_Read16(vm);
    wordSet = ScriptWork_GetWordSet(work);
    heapId = Field_GetHeapID(field);
    npc = ResortCmd_GetNPC(field);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, ResortNPC_GetTextEntry(npc, id, column + 7));
    GFL_WordSetSetStrbuf(wordSet, index, strbuf, 2, 1, 2);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

// Puts things about one of the avenue's people in the script's WordSet, and sets a variable to the message that
// tells them
BOOL Scrcmd_Resort_DescribeOccupant(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    Field *field;
    JoinAvenueOccupants *occupants;
    WordSet *wordSet;
    HeapID heapId;
    u16 index;
    u16 value;
    BOOL found;
    u16 number;
    // NTH_SHOP_COUNT_0 is the first nonzero shop count; zero-count kinds are skipped.
    enum {
        RESORT_DESCRIBE_START_DATE = 0,
        RESORT_DESCRIBE_HALL_OF_FAME_DATE = 1,
        RESORT_DESCRIBE_PLAY_TIME = 2,
        RESORT_DESCRIBE_DEX_CAUGHT = 3,
        RESORT_DESCRIBE_SPECIES = 5,
        RESORT_DESCRIBE_LOCATION = 6,
        RESORT_DESCRIBE_JOB = 7,
        RESORT_DESCRIBE_HOBBY = 8,
        RESORT_DESCRIBE_AVENUE_RANK = 9,
        RESORT_DESCRIBE_HAS_SHOP_COUNTS = 10,
        RESORT_DESCRIBE_SHOP_KIND_COUNT = 11,
        RESORT_DESCRIBE_TOTAL_SHOP_COUNT = 12,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_0 = 13,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_1 = 14,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_2 = 15,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_3 = 16,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_4 = 17,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_5 = 18,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_6 = 19,
        RESORT_DESCRIBE_NTH_SHOP_COUNT_7 = 20,
    } mode;
    MsgData *msgData;
    BOOL any;
    u16 *var;
    JoinAvenuePerson *person;
    u16 date;
    u16 a;
    u16 b;
    u32 total;
    JoinAvenuePersonParam param;
    int i;
    u32 n;
    u32 count;
    StrBuf *strbuf;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortCmd_GetNPC(field);
    occupants =
        getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    wordSet = ScriptWork_GetWordSet(work);
    heapId = Field_GetHeapID(field);
    index = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    person = JoinAvenueOccupants_GetPerson(occupants, index);
    switch (mode) {
    case RESORT_DESCRIBE_START_DATE:
        date = joinAveTextHandler(person, JOIN_AVE_PARAM_START_DATE, NULL);
        if (date == 0) {
            *var = 1725;
            break;
        }
        WordSetNumber(wordSet, 1, (date >> 9) & 0x7f, 2, 2, 1);
        WordSetNumber(wordSet, 2, (date >> 5) & 0xf, 2, 0, 1);
        WordSetNumber(wordSet, 3, date & 0x1f, 2, 0, 1);
        *var = 1724;
        break;
    case RESORT_DESCRIBE_HALL_OF_FAME_DATE:
        date = joinAveTextHandler(person, JOIN_AVE_PARAM_HALL_OF_FAME_DATE, NULL);
        if (date == 0) {
            *var = 1727;
            break;
        }
        WordSetNumber(wordSet, 1, (date >> 9) & 0x7f, 2, 2, 1);
        WordSetNumber(wordSet, 2, (date >> 5) & 0xf, 2, 0, 1);
        WordSetNumber(wordSet, 3, date & 0x1f, 2, 0, 1);
        *var = 1726;
        break;
    case RESORT_DESCRIBE_PLAY_TIME:
        a = joinAveTextHandler(person, JOIN_AVE_PARAM_PLAY_HOURS, NULL);
        b = joinAveTextHandler(person, JOIN_AVE_PARAM_PLAY_MINUTES, NULL);
        if (a == 0 && b == 0) {
            *var = 1729;
            break;
        }
        WordSetNumber(wordSet, 1, a, 3, 0, 1);
        WordSetNumber(wordSet, 2, b, 3, 0, 1);
        *var = 1728;
        break;
    case RESORT_DESCRIBE_DEX_CAUGHT:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_DEX_CAUGHT, NULL);
        if (value == 0) {
            *var = 1731;
            break;
        }
        WordSetNumber(wordSet, 1, value, 3, 0, 1);
        *var = 1730;
        break;
    case RESORT_DESCRIBE_SPECIES:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_SPECIES, NULL);
        if (value == 0) {
            *var = 1733;
            break;
        }
        WordSet_LoadSpeciesName(wordSet, 1, value);
        *var = 1732;
        break;
    case RESORT_DESCRIBE_LOCATION:
        a = joinAveTextHandler(person, JOIN_AVE_PARAM_COUNTRY, NULL);
        b = joinAveTextHandler(person, JOIN_AVE_PARAM_AREA, NULL);
        if (a == 0 && b == 0) {
            *var = 1737;
        } else if (b == 0) {
            loadCountryToStrbuf(wordSet, 1, a);
            *var = 1736;
        } else {
            loadCountryToStrbuf(wordSet, 1, a);
            loadCountryAreaToStrbuf(wordSet, 2, a, b);
            *var = 1735;
        }
        break;
    case RESORT_DESCRIBE_JOB:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_JOB, NULL);
        if (value == 0) {
            *var = 1739;
            break;
        }
        loadJobAnswerToStrbuf(wordSet, 1, value);
        *var = 1738;
        break;
    case RESORT_DESCRIBE_HOBBY:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_HOBBY, NULL);
        if (value == 0) {
            *var = 1741;
            break;
        }
        loadHobbyNameToStrbuf(wordSet, 1, value);
        *var = 1740;
        break;
    case RESORT_DESCRIBE_AVENUE_RANK:
        number = joinAveTextHandler(person, JOIN_AVE_PARAM_RANK, NULL);
        found = FALSE;
        if (number != 0) {
            if (number == 1) {
                for (param = JOIN_AVE_PARAM_SHOP_COUNT; param <= 55; param++) {
                    if (joinAveTextHandler(person, param, NULL) != 0) {
                        found = TRUE;
                        break;
                    }
                }
            } else {
                found = TRUE;
            }
        }
        if (!found) {
            *var = 1743;
            break;
        }
        WordSetNumber(wordSet, 1, number, 3, 0, 1);
        *var = 1742;
        break;
    case RESORT_DESCRIBE_HAS_SHOP_COUNTS:
        total = 0;
        for (i = 0; i < 8; i++) {
            total += joinAveTextHandler(person, i + JOIN_AVE_PARAM_SHOP_COUNT, NULL);
        }
        any = FALSE;
        if (total != 0) {
            any = TRUE;
        }
        *var = any;
        break;
    case RESORT_DESCRIBE_SHOP_KIND_COUNT:
        count = 0;
        for (i = 0; i < 8; i++) {
            if (joinAveTextHandler(person, i + JOIN_AVE_PARAM_SHOP_COUNT, NULL) != 0) {
                count++;
            }
        }
        *var = count;
        break;
    case RESORT_DESCRIBE_TOTAL_SHOP_COUNT:
        total = 0;
        for (i = 0; i < 8; i++) {
            total += joinAveTextHandler(person, i + JOIN_AVE_PARAM_SHOP_COUNT, NULL);
        }
        WordSetNumber(wordSet, 2, total, 1, 0, 1);
        *var = 1745;
        break;
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_0:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_1:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_2:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_3:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_4:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_5:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_6:
    case RESORT_DESCRIBE_NTH_SHOP_COUNT_7:
        mode -= RESORT_DESCRIBE_NTH_SHOP_COUNT_0;
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        n = 0;
        for (i = 0; i < 8; i++) {
            u32 target = joinAveTextHandler(person, i + JOIN_AVE_PARAM_SHOP_COUNT, NULL);
            int wanted = mode + 1;

            if (target != 0 && wanted == ++n) {
                strbuf = GFL_MsgDataLoadStrbufNew(msgData, i + 280);
                GFL_WordSetSetStrbuf(wordSet, 1, strbuf, 2, 1, 2);
                WordSetNumber(wordSet, 2, target, 1, 0, 1);
                GFL_StrBufFree(strbuf);
                break;
            }
        }
        GFL_MsgDataFree(msgData);
        if (mode + 1 < n) {
            *var = 1747;
        } else {
            *var = 1746;
        }
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_GetGoodsValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    void *shops;
    ResortPerson *person;
    const u16 *row;
    u16 index;
    u16 mode;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    shops = ResortSys_GetShopData(ResortSys_Get(field));
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    row = ResortShopData_GetPersonShop(shops, ResortPerson_GetJoinAvenuePerson(person));
    index = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    if (mode == 0) {
        const u16 *item = ResortShopData_GetGoods(shops, row, index);

        if (ResortPerson_GetParam(person, JOIN_AVE_PARAM_GENDER, NULL) == 0) {
            *var = ResortShopData_GetGoodsParam(item, 8);
        } else {
            *var = ResortShopData_GetGoodsParam(item, 9);
        }
    }
    return FALSE;
}

// Sets a field of the player's own entry or the avenue's info
BOOL Scrcmd_Resort_SetOwnValue(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u32 param = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    JoinAvenuePerson *own = JoinAvenueOccupants_GetPlayerPerson(getAddressOfBeginningOfOccupants(joinAvenue));
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);

    switch (param) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        JoinAvenuePerson_SetParam(own, (u16)param + JOIN_AVE_PARAM_VALUE, value);
        break;
    case RESORT_OWN_SHOP_ID:
        JoinAvenuePerson_SetParam(own, JOIN_AVE_PARAM_SHOP_ID,
                                  sPlayerShopIdsByVersion[VERSION_BLACK2 - GAME_VERSION][value]);
        break;
    case RESORT_OWN_SEASON:
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_SEASON, value);
        break;
    case RESORT_OWN_UNOPENED:
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_UNOPENED, value);
        if (value == 0) {
            ResortField_RefreshPeople(field);
        }
        break;
    case RESORT_OWN_CONFIRM_UNLOCK:
        JoinAvenueInfo_SetParam(info, value + JOIN_AVE_INFO_UNLOCK_PENDING_1 - 1, 0);
        JoinAvenueInfo_SetParam(info, value + JOIN_AVE_INFO_UNLOCKED_1 - 1, 1);
        break;
    case 22:
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_UNK_DC_13, value);
        break;
    case RESORT_OWN_ENTER_EVENT_MODE:
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            ResortField_EnterEventMode(field);
        }
        break;
    case RESORT_OWN_BUBBLES_ACTIVE:
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            ResortField_SetBubblesActive(field, value);
        }
        break;
    case RESORT_OWN_PARENT_ACTOR:
        ScriptWork_SetParentActor(work, FindFieldActor(GameData_GetMMSys(gameData), value));
        break;
    }
    return FALSE;
}

// Sets a variable to a field of the player's own entry or the avenue's info
BOOL Scrcmd_Resort_GetOwnValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    u32 param;
    u16 *var;
    JoinAvenueSave *joinAvenue;
    JoinAvenueOccupants *occupants;
    JoinAvenueInfo *info;
    JoinAvenuePerson *own;
    ResortSys *sys;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    param = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    info = JoinAvenue_GetInfo(joinAvenue);
    own = JoinAvenueOccupants_GetPlayerPerson(occupants);
    sys = ResortSys_Get(field);
    switch (param) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        *var = joinAveTextHandler(own, (u16)param + JOIN_AVE_PARAM_VALUE, NULL);
        break;
    case RESORT_OWN_SHOP_ID:
        *var = JoinAvenuePerson_GetShopId(own, GetGameDataPlayerInfo(gameData));
        break;
    case RESORT_OWN_MAX_LEVEL_WEEK:
        *var = ResortSys_GetMaxLevelWeek(sys);
        break;
    case RESORT_OWN_UNLOCK:
        *var = ResortSys_GetUnlock(sys, gameData);
        break;
    case 22:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNK_DC_13, 0);
        break;
    case RESORT_OWN_SHOP_KIND_5_UNLOCKED:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_6, 0);
        break;
    case RESORT_OWN_SEASON:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_SEASON, 0);
        break;
    case RESORT_OWN_IS_FULL:
        *var = ResortSys_IsFull(sys);
        break;
    case RESORT_OWN_ALL_SHOPS_MAX_LEVEL:
        *var = ResortSys_AreAllShopsMaxLevel(sys);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_StartSceneAnim(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortField_StartSceneAnim(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_IsSceneAnimDone(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    u16 animation;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    animation = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    *var = ResortField_IsSceneAnimDone(field, animation);
    return FALSE;
}

BOOL Scrcmd_Resort_EndSceneAnim(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortField_EndSceneAnim(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_AddEntryOrRecord(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    ResortSys *sys;
    HeapID heapId;
    PlayerInfo *playerInfo;
    u16 mode;
    u16 shopKind;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    heapId = Field_GetHeapID(field);
    playerInfo = GetGameDataPlayerInfo(gameData);
    mode = ScriptReadAny(vm, env);
    shopKind = ScriptReadAny(vm, env);
    ResortSys_AddEntryOrRecord(sys, playerInfo, mode, shopKind, heapId);
    if (shopKind <= 1) {
        ResortField_MarkPeopleChanged(field);
    }
    return FALSE;
}

static BOOL ResortCmd_UpdateRecords(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    ResortSys *sys = ResortSys_Get(field);
    void **ptr = ScriptWork_GetUserHeapPtr(work);
    HeapID heapId = Field_GetHeapID(field);
    ResortRecordsWork *wk = *ptr;
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    switch (wk->state) {
    case RESORT_RECORDS_STATE_CREATE:
        wk->window = ResortRecords_CreateWindow(msgBGSys, heapId);
        wk->state++;
        break;
    case RESORT_RECORDS_STATE_PRINT:
        ResortWork_UpdateRecords(GameData_GetResortWork(gameData), GameData_GetSaveControl(gameData));
        ResortRecords_PrintPage(wk->window, ResortSys_GetOccupants(sys), msgBGSys, wordSet, gameData, wk->page, heapId);
        wk->state++;
        break;
    case RESORT_RECORDS_STATE_WAIT_PRINT:
        if (MsgBGPrinter_IsMsgWinTransDone(wk->window)) {
            GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(MsgBGPrinter_GetMsgWinBmpWin(wk->window)));
            wk->state++;
        }
        break;
    case RESORT_RECORDS_STATE_INPUT:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(1361);
            wk->state = RESORT_RECORDS_STATE_CLOSE;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            GFL_SndSEPlay(1352);
            wk->page++;
            wk->page %= 9;
            wk->state = RESORT_RECORDS_STATE_PRINT;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            GFL_SndSEPlay(1352);
            wk->page--;
            wk->page = wk->page < 0 ? 8 : wk->page;
            wk->state = RESORT_RECORDS_STATE_PRINT;
        }
        break;
    case RESORT_RECORDS_STATE_CLOSE:
        MsgBGPrinter_ClearMsgWin(wk->window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(MsgBGPrinter_GetMsgWinBmpWin(wk->window)));
        MsgBGPrinter_DeleteMsgWin(wk->window);
        wk->state++;
        break;
    case RESORT_RECORDS_STATE_FINISH:
        *wk->var = wk->page;
        GFL_HeapFree(wk);
        *ptr = NULL;
        return TRUE;
    }
    return FALSE;
}

// Shows the avenue's records, from the page, and sets a variable to the page it closes on
BOOL Scrcmd_Resort_ShowRecords(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field;
    HeapID heapId;
    void **ptr;
    u16 page;
    u16 *var;
    ResortRecordsWork *wk;

    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortSys_GetSlots(ResortSys_Get(field));
    heapId = Field_GetHeapID(field);
    ptr = ScriptWork_GetUserHeapPtr(work);
    page = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    wk = GFL_HeapAllocate(heapId, sizeof(ResortRecordsWork), TRUE, "scrcmd_resort.c", 2716);
    wk->page = page;
    wk->var = var;
    *ptr = wk;
    VM_SetNativeCallback(vm, ResortCmd_UpdateRecords);
    return TRUE;
}

BOOL Scrcmd_Resort_AddRecordPerson(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    ResortPeople *people;
    void *datas;
    u16 zoneId;
    u32 index;
    u16 *var;
    ResortPersonSource key;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    people = ResortCmd_GetPeople(field);
    datas = ResortSys_GetDatas(sys);
    zoneId = Field_GetPlayerStateZoneID(field);
    index = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    MI_CpuFill8(&key, 0, sizeof(key));
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
        key.data = ResortDatas_Get(datas, RESORT_DATA_RECORD, index);
        key.zone = ResortLayout_GetZoneIndex(zoneId);
        *var = (u16)ResortPerson_GetParam(ResortPeople_AddAtEmptySlot(people, &key), JOIN_AVE_PARAM_SLOT, NULL) + 80;
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_RecreatePeople(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    void *datas;
    ResortPeople *people;
    u32 index;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    datas = ResortSys_GetDatas(ResortSys_Get(field));
    people = ResortCmd_GetPeople(field);
    index = ScriptReadAny(vm, env);
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        ResortPeople_Recreate(people,
                              ResortPeople_FindByJoinAvenuePerson(
                                  people, ResortData_GetPerson(ResortDatas_Get(datas, RESORT_DATA_OCCUPANT, index))));
        break;
    case RESORT_RECREATE_RECORD_PEOPLE: {
        u32 i;

        for (i = 0; i < ResortPeople_GetCount(people); i++) {
            ResortPerson *person = ResortPeople_GetByIndex(people, i);

            if (person != NULL && ResortPerson_GetAttr(person, RESORT_PERSON_ATTR_TYPE) == RESORT_PERSON_RECORD) {
                ResortPeople_Recreate(people, person);
            }
        }
        break;
    }
    }
    return FALSE;
}

static ResortPeople *ResortCmd_GetPeople(Field *field) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        return ResortField0_GetPeople(field);
    }
    if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = ResortField_GetPeople(field);
    }
    return people;
}

// The UID of the script's actor
static u16 ResortCmd_GetActorUID(FieldScriptEnv *env) {
    return GetActorUID(ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env)));
}

static ResortNPC *ResortCmd_GetNPC(Field *field) {
    void *npc;

    if (Field_CheckGimmickWorkPassword(field, 1)) {
        return ResortField_GetNPC(field);
    }
    npc = NULL;
    if (Field_CheckGimmickWorkPassword(field, 0)) {
        npc = ResortField0_GetNPC(field);
    }
    return npc;
}

// Changes an effort value, keeping it to 252 and the total to 510, or returns -1 if it can't change
static int ResortCmd_ApplyEffortChange(int value, int others, int change) {
    if (value == 0 && change < 0) {
        return -1;
    }
    if (value == 255 && change > 0) {
        return -1;
    }
    if (value + others >= 510 && change > 0) {
        return -1;
    }
    if (value >= 252 && change > 0) {
        return -1;
    }
    if (value + change >= 252) {
        change = 252 - value;
    }
    value += change;
    if (value < 0) {
        value = 0;
    }
    if (value + others > 510) {
        value = 510 - others;
    }
    return value;
}
