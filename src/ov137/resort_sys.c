#include "types.h"
#include "constants/arc.h"
#include "constants/flags.h"
#include "constants/script_text_banks.h"
#include "constants/species.h"
#include "constants/version.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/resort.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/rtc.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"
#include "system/resort_work.h"
#include "system/union_view.h"
#include "system/wordset.h"

// The visit priority also indexes the half, normal and double point multipliers
enum {
    RESORT_VISIT_PRIORITY_HALF = 0,
    RESORT_VISIT_PRIORITY_NORMAL = 1,
    RESORT_VISIT_PRIORITY_DOUBLE = 2,
};

#define RESORT_SHOP_MAX_LEVEL 9

// The conversions selected by sMakeKinds
enum {
    RESORT_MAKE_NONE = 0,
    RESORT_MAKE_PERSON_FROM_PERSON = 1,
    RESORT_MAKE_PERSON_FROM_ENTRY = 2,
    RESORT_MAKE_RECORD_FROM_PERSON = 3,
    RESORT_MAKE_RECORD_FROM_ENTRY = 4,
};

// The player follows the eight occupants in the ranking table
#define RESORT_RECORD_PLAYER_INDEX 8

// A visit request tests a shop kind, gender, one of sixteen values, or a ranking page
enum {
    RESORT_VISIT_CONDITION_SHOP_KIND = 0,
    RESORT_VISIT_CONDITION_GENDER = 1,
    RESORT_VISIT_CONDITION_VALUE_0 = 2,
    RESORT_VISIT_CONDITION_VALUE_1 = 3,
    RESORT_VISIT_CONDITION_VALUE_2 = 4,
    RESORT_VISIT_CONDITION_VALUE_3 = 5,
    RESORT_VISIT_CONDITION_VALUE_4 = 6,
    RESORT_VISIT_CONDITION_VALUE_5 = 7,
    RESORT_VISIT_CONDITION_VALUE_6 = 8,
    RESORT_VISIT_CONDITION_VALUE_7 = 9,
    RESORT_VISIT_CONDITION_VALUE_8 = 10,
    RESORT_VISIT_CONDITION_VALUE_9 = 11,
    RESORT_VISIT_CONDITION_VALUE_10 = 12,
    RESORT_VISIT_CONDITION_VALUE_11 = 13,
    RESORT_VISIT_CONDITION_VALUE_12 = 14,
    RESORT_VISIT_CONDITION_VALUE_13 = 15,
    RESORT_VISIT_CONDITION_VALUE_14 = 16,
    RESORT_VISIT_CONDITION_VALUE_15 = 17,
    RESORT_VISIT_CONDITION_LINK_TRADES = 19,
    RESORT_VISIT_CONDITION_NICKNAMES = 20,
    RESORT_VISIT_CONDITION_AVENUE_CUSTOMERS = 21,
    RESORT_VISIT_CONDITION_SHOPPING_MONEY = 22,
    RESORT_VISIT_CONDITION_PASSERSBY = 23,
    RESORT_VISIT_CONDITION_LINK_BATTLES = 24,
    RESORT_VISIT_CONDITION_POKEMON_CAUGHT = 25,
    RESORT_VISIT_CONDITION_EGGS_HATCHED = 26,
    RESORT_VISIT_CONDITION_MEDALS = 27,
};

// The Join Avenue's system (resort_sys.c, from its assert string): the avenue's people and their data, its tables and
// the questions the scripts ask about them. The scripts get a message of the avenue's texts, SCRIPT_TEXT_GLOBAL_10685,
// by the number of the question, a man's or a woman's version by the person's gender

// The kinds of shop that the avenue's ranks unlock, and the people of the avenue
#define RESORT_SHOP_KIND_COUNT 8
#define RESORT_PERSON_COUNT 8

// The base of the messages of the page of a ranking in SCRIPT_TEXT_GLOBAL_10685
#define RESORT_MSG_RANK_PAGE 2173

struct ResortSys {
    ResortSysSetup setup;
    ResortShopData *shopData;
    // The person types: their kinds of shop, what they wear and say
    ResortBinary *types;
    // The avenue's ranks: the popularity they need and the shops they bring
    ResortBinary *ranks;
    ResortBinary *posOffsets;
    // The shops' raffles, which join_ave_raffle_shop draws from
    ResortBinary *raffles;
    // The texts of a trainer type, by column
    ResortBinary *texts;
    // The shops that open at a popularity
    ResortBinary *shopOpenings;
    ResortPersonData **datas;
    ResortSlots *slots;
    ResortWork *work;
};

// A value of a table, by the key before it
typedef struct {
    u16 key;
    u16 value;
} ResortKeyValue;

// What the avenue's ranks unlock, in order: the info's fields of the unlock's flags, pending and given
typedef struct {
    u32 pendingParam;
    u32 unlockedParam;
    u32 id;
    BOOL (*check)(ResortSys *sys, GameData *gameData);
    BOOL flag;
} ResortUnlock;

// The people next to a person at the avenue, then the others, each ended by -1
typedef struct {
    s8 near[8];
    s8 far[8];
} ResortPeopleRow;

// A person of a ranking, and where the ranking puts them
typedef struct {
    u32 index;
    u32 value;
} ResortRankEntry;

// A person of the ranking of ResortSys_GetRecordRank, who may be the one asked about
typedef struct {
    u32 index;
    u32 value;
    BOOL isSelf;
} ResortRankSelf;

static ResortBinary *ResortSys_GetPersonTypes(ResortSys *sys);
static u32 ResortSys_PickByGender(u32 gender, u32 male, u32 female);
static BOOL ResortSys_MatchCondition(u32 type, JoinAvenuePerson *person, ResortSys *sys, u32 *row);
static u32 ResortSys_RollTier(ResortPersonData *data, ResortSys *sys);
static BOOL ResortSys_IsTypeAvailable(ResortSys *sys, ResortBinary *table, u16 row);
static u32 ResortSys_PickTypeOfTier(ResortPersonData *data, ResortSys *sys, u32 tier);
static u32 ResortSys_ChoosePersonType(ResortPersonData *data, ResortSys *sys, GameData *gameData);
static u32 ResortSys_FindUnlockRow(ResortBinary *table, u32 id);
static u32 ResortSys_GetRecordRank(JoinAvenueOccupants *occupants, JoinAvenuePerson *person, u32 page);
static BOOL ResortActor_IsBehindCamera(FieldActor *actor, FieldCamera *camera);
static BOOL ResortUnlock_Always(ResortSys *sys, GameData *gameData);
static BOOL ResortUnlock_FlagSet(ResortSys *sys, GameData *gameData);
static BOOL ResortUnlock_AllPeople(ResortSys *sys, GameData *gameData);
static BOOL ResortSys_GetEmptyData(ResortSys *sys, u32 kind, u32 row, u16 *slot, ResortPersonData **data);
static void ResortEntry_Fill(JoinAvenueEntry *entry, u16 shopKind, ResortBinary *table, u16 row, HeapID heapId);
static void ResortEntry_Setup(ResortSys *sys, GameData *gameData, JoinAvenueEntry *entry, u32 shopKind, u16 row,
                              HeapID heapId);
static void ResortRecord_Setup(ResortSys *sys, JoinAvenueRecord *record, u16 shopKind, u32 type, u16 slot,
                               HeapID heapId);
static void ResortEntry_SetupFromType(ResortSys *sys, JoinAvenueEntry *entry, u16 shopKind, u32 type, HeapID heapId);
static void ResortSys_ResetWork(ResortSys *sys, u32 value);

const u16 RESORT_SYS_UNUSED[2] = { 0, 8 };
// The kinds of data of the occupants
static const u32 sOccupantKinds[1] = { RESORT_DATA_OCCUPANT };
// The shop kinds that need a row of the texts, and the type that row has
static const ResortKeyValue sShopKindTexts[2] = { { 4, 2 }, { 5, 3 } };
// The chance in 100 of a new shop of the kind in a tier
static const s32 sNewShopChances[3] = { 10, 15, 20 };
static const ResortKeyValue sEntryTypes[3] = { { 0, 2 }, { 1, 3 }, { 3, 5 } };
static const u8 sShopUnlockIndexByRank[13] = { 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6 };
// How much a person's shop kind counts for, in 12-bit fixed point
static const s32 sShopKindMultipliers[3] = { 0x800, 0x1000, 0x2000 };
static const ResortKeyValue sRecordTypes[4] = { { 4, 0 }, { 5, 1 }, { 6, 2 }, { 7, 3 } };
// The kinds of people that hide, by the kind of the person that is hidden: the types before a 4
static const u32 sHiddenTypes[2][4] = {
    { RESORT_PERSON_VISITOR, RESORT_PERSON_ENTRY, RESORT_PERSON_TYPE_MAX, RESORT_PERSON_OCCUPANT },
    { RESORT_PERSON_OCCUPANT, RESORT_PERSON_VISITOR, RESORT_PERSON_ENTRY, RESORT_PERSON_TYPE_MAX },
};
static const u8 sShopUnlockOrderByTrainerId[7][7] = {
    { 0, 2, 7, 4, 6, 1, 3 }, { 1, 3, 0, 2, 7, 4, 6 }, { 4, 6, 1, 3, 0, 2, 7 }, { 2, 7, 4, 6, 1, 3, 0 },
    { 3, 0, 2, 7, 4, 6, 1 }, { 6, 1, 3, 0, 2, 7, 4 }, { 7, 4, 6, 1, 3, 0, 2 },
};
// The data that is made by the kind of the data it is made from and the kind it is made as
// clang-format off
static const u32 sMakeKinds[4][4] = {
    {RESORT_MAKE_NONE, RESORT_MAKE_PERSON_FROM_ENTRY, RESORT_MAKE_PERSON_FROM_PERSON, RESORT_MAKE_NONE},
    {RESORT_MAKE_NONE, RESORT_MAKE_NONE, RESORT_MAKE_NONE, RESORT_MAKE_NONE},
    {RESORT_MAKE_NONE, RESORT_MAKE_NONE, RESORT_MAKE_NONE, RESORT_MAKE_NONE},
    {RESORT_MAKE_NONE, RESORT_MAKE_RECORD_FROM_ENTRY, RESORT_MAKE_RECORD_FROM_PERSON, RESORT_MAKE_NONE},
};

// clang-format on
// clang-format off
static const ResortPeopleRow sPeopleRows[RESORT_PERSON_COUNT] = {
    {{1, 2, -1}, {3, 4, 5, 6, 7, -1}}, {{0, 2, 3, -1}, {4, 5, 6, 7, -1}}, {{0, 1, 3, 4, -1}, {5, 6, 7, -1}},
    {{1, 2, 4, 5, -1}, {0, 6, 7, -1}}, {{2, 3, 5, 6, -1}, {0, 1, 7, -1}}, {{3, 4, 6, 7, -1}, {0, 1, 2, -1}},
    {{4, 5, 7, -1}, {0, 1, 2, 3, -1}}, {{5, 6, -1}, {0, 1, 2, 3, 4, -1}},
};

// clang-format on
// clang-format off
static const ResortUnlock sUnlocks[RESORT_SHOP_KIND_COUNT] = {
    {JOIN_AVE_INFO_UNLOCK_PENDING_1, JOIN_AVE_INFO_UNLOCKED_1, 1, ResortUnlock_Always, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_2, JOIN_AVE_INFO_UNLOCKED_2, 2, ResortUnlock_Always, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_4, JOIN_AVE_INFO_UNLOCKED_4, 4, ResortUnlock_Always, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_5, JOIN_AVE_INFO_UNLOCKED_5, 5, ResortUnlock_Always, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_3, JOIN_AVE_INFO_UNLOCKED_3, 3, ResortUnlock_Always, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_6, JOIN_AVE_INFO_UNLOCKED_6, 6, ResortUnlock_FlagSet, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_7, JOIN_AVE_INFO_UNLOCKED_7, 7, ResortUnlock_AllPeople, FALSE},
    {JOIN_AVE_INFO_UNLOCK_PENDING_8, JOIN_AVE_INFO_UNLOCKED_7, 8, ResortUnlock_Always, TRUE},
};

// clang-format on

ResortSys *ResortSys_Create(const ResortSysSetup *setup, ResortWork *resortWork, HeapID heapId) {
    ResortSys *sys = GFL_HeapAllocate(heapId, sizeof(ResortSys), TRUE, "resort_sys.c", 75);

    sys->setup = *setup;
    sys->work = resortWork;
    sys->shopData = ResortShopData_Load(heapId);
    sys->types = ResortBinary_Load(4, 42, heapId);
    sys->ranks = ResortBinary_Load(7, 13, heapId);
    sys->posOffsets = ResortBinary_Load(3, 4, heapId);
    sys->raffles = ResortBinary_Load(8, 22, heapId);
    sys->texts = ResortBinary_Load(9, 23, heapId);
    sys->shopOpenings = ResortBinary_Load(5, 11, heapId);
    sys->datas = ResortDatas_Create(setup, heapId);
    sys->slots = ResortSlots_Create(sys->datas, heapId);
    return sys;
}

void ResortSys_Free(ResortSys *sys) {
    ResortSlots_Free(sys->slots);
    ResortDatas_Free(sys->datas);
    ResortBinary_Free(sys->shopOpenings);
    ResortBinary_Free(sys->texts);
    ResortBinary_Free(sys->raffles);
    ResortBinary_Free(sys->posOffsets);
    ResortBinary_Free(sys->ranks);
    ResortBinary_Free(sys->types);
    ResortShopData_Free(sys->shopData);
    GFL_HeapFree(sys);
}

ResortShopData *ResortSys_GetShopData(ResortSys *sys) {
    return sys->shopData;
}

static ResortBinary *ResortSys_GetPersonTypes(ResortSys *sys) {
    return sys->types;
}

ResortBinary *ResortSys_GetRanks(ResortSys *sys) {
    return sys->ranks;
}

ResortBinary *ResortSys_GetPosOffsets(ResortSys *sys) {
    return sys->posOffsets;
}

ResortBinary *ResortSys_GetRaffles(ResortSys *sys) {
    return sys->raffles;
}

ResortBinary *ResortSys_GetTexts(ResortSys *sys) {
    return sys->texts;
}

ResortBinary *ResortSys_GetShopOpenings(ResortSys *sys) {
    return sys->shopOpenings;
}

ResortPersonData **ResortSys_GetDatas(ResortSys *sys) {
    return sys->datas;
}

ResortSlots *ResortSys_GetSlots(ResortSys *sys) {
    return sys->slots;
}

JoinAvenueOccupants *ResortSys_GetOccupants(ResortSys *sys) {
    return sys->setup.occupants;
}

JoinAvenuePersonList *ResortSys_GetList(ResortSys *sys) {
    return sys->setup.list;
}

JoinAvenuePersonList *ResortSys_GetList2(ResortSys *sys) {
    return sys->setup.list2;
}

JoinAvenueEntryList *ResortSys_GetEntries(ResortSys *sys) {
    return sys->setup.entries;
}

JoinAvenueInfo *ResortSys_GetInfo(ResortSys *sys) {
    return sys->setup.info;
}

ResortWork *ResortSys_GetWork(ResortSys *sys) {
    return sys->work;
}

// The version of a message for the person's gender
static u32 ResortSys_PickByGender(u32 gender, u32 male, u32 female) {
    if (gender != 0) {
        male = female;
    }
    return male;
}

// The answer to a question of the scripts: a message of the avenue's texts for the person asked about, or a number
u16 ResortSys_GetScriptValue(u32 query, ResortSys *sys, ResortPersonData *data, GameData *gameData, WordSet *wordSet,
                             HeapID heapId) {
    ResortPersonData **datas = ResortSys_GetDatas(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 kind;
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    u32 shopKind;
    u32 gender;

    kind = 0;
    shopKind = 0;
    gender = 0;
    if (data != NULL) {
        gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
        kind = (u16)ResortData_GetType(data);
        shopKind = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    }

    switch (query) {
    case RESORT_QUERY_RECENT_ACTIVITY_0:
    case RESORT_QUERY_RECENT_ACTIVITY_1:
    case RESORT_QUERY_RECENT_ACTIVITY_2:
    case RESORT_QUERY_RECENT_ACTIVITY_3: {
        u16 result = 0;
        u32 activity = ResortData_GetParam(data, JOIN_AVE_PARAM_RECENT_ID + query, NULL);
        u16 date = ResortData_GetParam(data, JOIN_AVE_PARAM_RECENT_DATE + query, NULL);

        WordSetNumber(wordSet, 0, (date >> 9) & 0x7f, 2, NUM_PAD_ZERO, TRUE);
        WordSetNumber(wordSet, 2, (date >> 5) & 0xf, 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 3, date & 0x1f, 2, NUM_PAD_NONE, TRUE);
        if (activity != 0) {
            result = activity + ResortSys_PickByGender(gender, 1048, 1081);
        }
        return result;
    }
    case RESORT_QUERY_VISIT_REQUEST: {
        u32 personType;

        if (kind == RESORT_PERSON_ENTRY && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 12);
        }
        personType = ResortData_GetParam(data, JOIN_AVE_PARAM_VISIT_TYPE, NULL);
        if (personType == 0) {
            personType = ResortSys_ChoosePersonType(data, sys, gameData);
            ResortData_SetParam(data, JOIN_AVE_PARAM_VISIT_TYPE, personType);
        }
        if (gender == 0) {
            return ResortBinary_Get(types, personType, 0);
        }
        return ResortBinary_Get(types, personType, 1);
    }
    case RESORT_QUERY_DESIRED_SHOP: {
        u16 shopId = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = ResortData_GetParam(data, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = ResortData_GetParam(data, JOIN_AVE_PARAM_VERSION, NULL);
        u16 keepShopLevel = ResortData_GetParam(data, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, NULL);
        u32 shop = JoinAvenue_ChooseShop(shopId, trainerId, version, keepShopLevel, info, shopData);

        return ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shop), 0) +
               ResortSys_PickByGender(gender, 1120, 1128);
    }
    case RESORT_QUERY_SHOP_DESCRIPTION: {
        u16 shopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(ResortData_GetPerson(data)), 0);

        return ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shopId), 0) +
               ResortSys_PickByGender(gender, 328, 336);
    }
    case RESORT_QUERY_SHOW_MEDAL:
        return ResortSys_PickByGender(gender, 1116, 1117);
    case RESORT_QUERY_NO_MEDAL:
        return ResortSys_PickByGender(gender, 1118, 1119);
    case RESORT_QUERY_MORE_TO_SAY:
        return ResortSys_PickByGender(gender, 1114, 1115);
    case RESORT_QUERY_NO_RECENT_ACTIVITY:
        return ResortSys_PickByGender(gender, 1048, 1081);
    case RESORT_QUERY_ARRIVAL_GREETING: {
        u32 message;
        u32 arrivalMethod = ResortData_GetParam(data, JOIN_AVE_PARAM_ARRIVAL_METHOD, NULL);
        u32 dataCount = ResortData_GetParam(data, JOIN_AVE_PARAM_DATA_COUNT, NULL);
        u32 version = ResortData_GetParam(data, JOIN_AVE_PARAM_VERSION, NULL);

        if (version == VERSION_WHITE2 || version == VERSION_BLACK2) {
            if (arrivalMethod == JOIN_AVE_ARRIVAL_PASSERBY) {
                message = ResortSys_PickByGender(gender, 8, 27);

                if (dataCount == 0) {
                    return ResortSys_PickByGender(gender, 18, 37);
                }
                return message + dataCount - 1;
            } else if (arrivalMethod == JOIN_AVE_ARRIVAL_FUNFEST) {
                return ResortSys_PickByGender(gender, 15, 34);
            } else if (arrivalMethod == JOIN_AVE_ARRIVAL_DREAM_WORLD) {
                u16 species = ResortData_GetParam(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

                if (species == 0) {
                    species = SPECIES_BULBASAUR;
                }
                WordSet_LoadSpeciesName(wordSet, 1, species);
                return ResortSys_PickByGender(gender, 7, 26);
            } else if (arrivalMethod == JOIN_AVE_ARRIVAL_GTS) {
                u16 species = ResortData_GetParam(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

                if (species == 0) {
                    species = SPECIES_BULBASAUR;
                }
                WordSet_LoadSpeciesName(wordSet, 1, species);
                return ResortSys_PickByGender(gender, 6, 25);
            } else if (arrivalMethod == JOIN_AVE_ARRIVAL_NONE) {
                break;
            }
            message = ResortSys_PickByGender(gender, 1, 20);
            return message + arrivalMethod - 1;
        } else if (arrivalMethod == JOIN_AVE_ARRIVAL_GTS) {
            u16 species = ResortData_GetParam(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

            if (species == 0) {
                species = SPECIES_BULBASAUR;
            }
            WordSet_LoadSpeciesName(wordSet, 1, species);
            return ResortSys_PickByGender(gender, 6, 25);
        } else if (arrivalMethod == JOIN_AVE_ARRIVAL_DREAM_WORLD) {
            u16 species = ResortData_GetParam(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

            if (species == 0) {
                species = SPECIES_BULBASAUR;
            }
            WordSet_LoadSpeciesName(wordSet, 1, species);
            return ResortSys_PickByGender(gender, 7, 26);
        }
        return ResortSys_PickByGender(gender, 19, 38);
    }
    case RESORT_QUERY_ORIGIN: {
        u32 country = ResortData_GetParam(data, JOIN_AVE_PARAM_COUNTRY, NULL);
        u32 area = ResortData_GetParam(data, JOIN_AVE_PARAM_AREA, NULL);

        if (country != 0 && area != 0) {
            loadCountryToStrbuf(wordSet, 1, country);
            loadCountryAreaToStrbuf(wordSet, 2, country, area);
            return ResortSys_PickByGender(gender, 40, 40);
        }
        if (country != 0) {
            loadCountryToStrbuf(wordSet, 1, country);
            return ResortSys_PickByGender(gender, 41, 44);
        }
        return ResortSys_PickByGender(gender, 39, 42);
    }
    case RESORT_QUERY_RECRUIT_ACCEPTED:
        return ResortSys_PickByGender(gender, 51, 55);
    case RESORT_QUERY_RECRUIT_THANKS:
        return ResortSys_PickByGender(gender, 52, 56);
    case RESORT_QUERY_RECRUIT_RANK_TOO_LOW:
        return ResortSys_PickByGender(gender, 53, 57);
    case RESORT_QUERY_RECRUIT_REFUSED:
        return ResortSys_PickByGender(gender, 54, 58);
    case RESORT_QUERY_DEPARTURE: {
        if (kind == RESORT_PERSON_ENTRY && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 13);
        }
        return ResortSys_PickByGender(gender, 75, 76);
    }
    case RESORT_QUERY_SOUVENIR:
        return ResortSys_PickByGender(gender, 49, 50);
    case RESORT_QUERY_MESSAGE1: {
        if (kind == RESORT_PERSON_ENTRY && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 22);
        }
        return ResortSys_PickByGender(gender, 47, 48);
    }
    case RESORT_QUERY_THANKS:
        return ResortSys_PickByGender(gender, 45, 46);
    case RESORT_QUERY_PERSON_NAME: {
        if (kind == RESORT_PERSON_ENTRY && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 14);
        }
        return 268;
    }
    case RESORT_QUERY_RECOMMEND_THANKS:
        return ResortSys_PickByGender(gender, 1484, 1485);
    case RESORT_QUERY_CALL_PLAYER:
        return ResortSys_PickByGender(gender, 63, 64);
    case RESORT_QUERY_CALL_PERSON:
        return ResortSys_PickByGender(gender, 65, 66);
    case RESORT_QUERY_CHANGE_SHOP_PROMPT:
        return ResortSys_PickByGender(gender, 67, 68);
    case RESORT_QUERY_CHANGE_SHOP_CONFIRM:
        return ResortSys_PickByGender(gender, 69, 70);
    case RESORT_QUERY_CHANGE_SHOP_ACCEPTED:
        return ResortSys_PickByGender(gender, 71, 72);
    case RESORT_QUERY_KEEP_SHOP:
        return ResortSys_PickByGender(gender, 73, 74);
    case RESORT_QUERY_ASSISTANT_GREETING: {
        switch (shopKind) {
        case 0:
        default:
            return ResortSys_PickByGender(gender, 122, 126);
        case 1:
            return ResortSys_PickByGender(gender, 123, 127);
        case 3:
            return ResortSys_PickByGender(gender, 124, 128);
        case 2:
            return ResortSys_PickByGender(gender, 125, 129);
        }
    }
    case RESORT_QUERY_ASSISTANT_ACKNOWLEDGE: {
        switch (shopKind) {
        case 0:
        default:
            return ResortSys_PickByGender(gender, 130, 134);
        case 1:
            return ResortSys_PickByGender(gender, 131, 135);
        case 3:
            return ResortSys_PickByGender(gender, 132, 136);
        case 2:
            return ResortSys_PickByGender(gender, 133, 137);
        }
    }
    case RESORT_QUERY_AVENUE_RANK: {
        u16 rank = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL);

        if (rank >= ResortBinary_GetLastRangeEnd(ranks)) {
            return ResortSys_PickByGender(gender, 148, 165);
        }
        return ResortSys_PickByGender(gender, 138, 155);
    }
    case RESORT_QUERY_CHANGE_ASSISTANT_ROLE:
        return ResortSys_PickByGender(gender, 142, 159);
    case RESORT_QUERY_REORDER_SHOPS:
        return ResortSys_PickByGender(gender, 143, 160);
    case RESORT_QUERY_CEILING_COLOR_CHANGED:
        return ResortSys_PickByGender(gender, 145, 162);
    case RESORT_QUERY_CHOOSE_CEILING_COLOR:
        return ResortSys_PickByGender(gender, 144, 161);
    case RESORT_QUERY_CHOOSE_RECORD_PAGE:
        return ResortSys_PickByGender(gender, 146, 163);
    case RESORT_QUERY_AVENUE_PROMOTION:
        return ResortSys_PickByGender(gender, 147, 164);
    case RESORT_QUERY_AVENUE_BUSY:
        return ResortSys_PickByGender(gender, 141, 158);
    case RESORT_QUERY_CHANGE_TITLE:
        return ResortSys_PickByGender(gender, 139, 156);
    case RESORT_QUERY_CHANGE_NAME:
        return ResortSys_PickByGender(gender, 140, 157);
    case RESORT_QUERY_CHANGE_GREETING:
        return ResortSys_PickByGender(gender, 153, 170);
    case RESORT_QUERY_CANNOT_REORDER_SHOPS:
        return ResortSys_PickByGender(gender, 154, 171);
    case RESORT_QUERY_SHOP_RANK: {
        u16 level = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(ResortData_GetPerson(data)), 1);

        if (level >= RESORT_SHOP_MAX_LEVEL) {
            return ResortSys_PickByGender(gender, 80, 85);
        }
        return ResortSys_PickByGender(gender, 79, 84);
    }
    case RESORT_QUERY_PROMOTION:
        return ResortSys_PickByGender(gender, 77, 78);
    case RESORT_QUERY_NEW_AVENUE_SHOPKEEPER:
        return 61;
    case RESORT_QUERY_NEW_AVENUE_CUSTOMER:
        return 62;
    case RESORT_QUERY_FAN_GREETING:
        return ResortSys_PickByGender(gender, 59, 60);
    case RESORT_QUERY_RAFFLE_PRIZE:
        return ResortSys_PickByGender(gender, 361, 367);
    case RESORT_QUERY_RAFFLE_LOST:
        return ResortSys_PickByGender(gender, 360, 366);
    case RESORT_QUERY_RAFFLE_ALREADY_PLAYED:
        return ResortSys_PickByGender(gender, 362, 368);
    case RESORT_QUERY_RAFFLE_DRAW:
        return ResortSys_PickByGender(gender, 363, 369);
    case RESORT_QUERY_RAFFLE_CONGRATULATIONS:
        return ResortSys_PickByGender(gender, 364, 370);
    case RESORT_QUERY_RAFFLE_RUNNING:
        return ResortSys_PickByGender(gender, 365, 371);
    case RESORT_QUERY_PAYMENT:
        return ResortSys_PickByGender(gender, 344, 345);
    case RESORT_QUERY_CANNOT_SELECT_EGG:
        return ResortSys_PickByGender(gender, 346, 347);
    case RESORT_QUERY_NOT_AN_EGG:
        return ResortSys_PickByGender(gender, 348, 349);
    case RESORT_QUERY_ALREADY_STRONG:
        return ResortSys_PickByGender(gender, 350, 351);
    case RESORT_QUERY_ALREADY_FRIENDLY:
        return ResortSys_PickByGender(gender, 352, 353);
    case RESORT_QUERY_ALREADY_FRIENDLY_ALT:
        return ResortSys_PickByGender(gender, 354, 355);
    case RESORT_QUERY_MAXIMUM_LEVEL:
        return ResortSys_PickByGender(gender, 356, 357);
    case RESORT_QUERY_UNWELL:
        return ResortSys_PickByGender(gender, 358, 359);
    case RESORT_QUERY_GROOM_DONE:
        return ResortSys_PickByGender(gender, 373, 375);
    case RESORT_QUERY_GROOM_SELECT:
        return ResortSys_PickByGender(gender, 372, 374);
    case RESORT_QUERY_MEAL_DONE:
        return ResortSys_PickByGender(gender, 377, 379);
    case RESORT_QUERY_MEAL_SELECT:
        return ResortSys_PickByGender(gender, 376, 378);
    case RESORT_QUERY_TRAINING_DONE:
        return ResortSys_PickByGender(gender, 381, 383);
    case RESORT_QUERY_TRAINING_SELECT:
        return ResortSys_PickByGender(gender, 380, 382);
    case RESORT_QUERY_EGG_READY:
        return ResortSys_PickByGender(gender, 385, 392);
    case RESORT_QUERY_EGG_SELECT:
        return ResortSys_PickByGender(gender, 384, 391);
    case RESORT_QUERY_EGG_ANY_SECOND:
        return ResortSys_PickByGender(gender, 386, 393);
    case RESORT_QUERY_EGG_SOON:
        return ResortSys_PickByGender(gender, 387, 394);
    case RESORT_QUERY_EGG_CLOSE:
        return ResortSys_PickByGender(gender, 388, 395);
    case RESORT_QUERY_EGG_NOT_CLOSE:
        return ResortSys_PickByGender(gender, 389, 396);
    case RESORT_QUERY_EGG_LONG_TIME:
        return ResortSys_PickByGender(gender, 390, 397);
    case RESORT_QUERY_TRAINER_TEXT_11:
        return ResortSys_GetTrainerTypeText(texts, data, 11);
    case RESORT_QUERY_TRAINER_TEXT_17:
        return ResortSys_GetTrainerTypeText(texts, data, 17);
    case RESORT_QUERY_TRAINER_TEXT_18:
        return ResortSys_GetTrainerTypeText(texts, data, 18);
    case RESORT_QUERY_TRAINER_TEXT_19:
        return ResortSys_GetTrainerTypeText(texts, data, 19);
    case RESORT_QUERY_TRAINER_TEXT_20:
        return ResortSys_GetTrainerTypeText(texts, data, 20);
    case RESORT_QUERY_TRAINER_TEXT_21:
        return ResortSys_GetTrainerTypeText(texts, data, 21);
    case RESORT_QUERY_OCCUPANT_WELCOME_0:
    case RESORT_QUERY_OCCUPANT_WELCOME_1:
    case RESORT_QUERY_OCCUPANT_WELCOME_2:
    case RESORT_QUERY_OCCUPANT_WELCOME_3:
    case RESORT_QUERY_OCCUPANT_WELCOME_4:
    case RESORT_QUERY_OCCUPANT_WELCOME_5:
    case RESORT_QUERY_OCCUPANT_WELCOME_6:
    case RESORT_QUERY_OCCUPANT_WELCOME_7: {
        ResortPersonData *occupant;
        StrBuf *name;
        u32 shopName;
        u16 shopId;

        query -= RESORT_QUERY_OCCUPANT_WELCOME_0;
        occupant = ResortDatas_Get(datas, RESORT_DATA_OCCUPANT, query);
        shopName =
            ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shopData, ResortData_GetPerson(occupant)), 0);
        shopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(ResortData_GetPerson(occupant)), 0);
        name = ResortSys_FormatShopText(shopData, occupant, shopId, heapId);
        GFL_WordSetSetStrbuf(wordSet, 0, name, ResortData_GetParam(occupant, JOIN_AVE_PARAM_GENDER, NULL), 1, 2);
        GFL_StrBufFree(name);
        return shopName + 2165;
    }
    case RESORT_QUERY_RECORD_RANK: {
        u32 message;
        u32 candidates[9] = { 0 };
        s32 count = 0;
        JoinAvenuePerson *person = ResortData_GetPerson(data);
        BOOL hasRecord = FALSE;
        s32 i;
        u32 pick;

        for (i = 0; i < JOIN_AVE_RECORD_COUNT; i++) {
            JoinAvenueRecord *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 2) {
                hasRecord = TRUE;
                break;
            }
        }
        if (!hasRecord) {
            return 0;
        }
        for (i = 0; i <= 8; i++) {
            if (ResortSys_GetRecordRank(occupants, person, i) == 0) {
                candidates[count] = i;
                count++;
            }
        }
        if (count == 0) {
            return 0;
        }
        pick = GFL_RandomLC(count);
        message = ResortSys_PickByGender(gender, 103, 112);
        return message + candidates[pick];
    }
    case RESORT_QUERY_RECOMMEND_SHOP:
        return ResortSys_PickByGender(gender, 89, 96);
    case RESORT_QUERY_RECOMMEND_VISIT:
        return ResortSys_PickByGender(gender, 90, 97);
    case RESORT_QUERY_RECOMMEND_MESSAGE:
        return ResortSys_PickByGender(gender, 91, 98);
    case RESORT_QUERY_RECOMMEND_NEXT_TIME:
        return ResortSys_PickByGender(gender, 92, 99);
    case RESORT_QUERY_GRIN:
        return ResortSys_PickByGender(gender, 93, 100);
    case RESORT_QUERY_SMILE:
        return ResortSys_PickByGender(gender, 94, 101);
    case RESORT_QUERY_BIG_SMILE:
        return ResortSys_PickByGender(gender, 95, 102);
    case RESORT_QUERY_MOTIVATION: {
        u32 level = ResortSys_GetVisitPoints(sys, data);

        if (!ResortSys_IsFull(sys)) {
            return 0;
        }
        if (level >= 20) {
            return ResortSys_PickByGender(gender, 83, 88);
        }
        if (level >= 10) {
            return ResortSys_PickByGender(gender, 82, 87);
        }
        if (level >= 5) {
            return ResortSys_PickByGender(gender, 81, 86);
        }
        return 0;
    }
    }
    return 0;
}

// Whether a person matches one of the conditions of a type of person, and the row of the condition that is chosen: the
// one of priority 2, else the highest
static BOOL ResortSys_MatchCondition(u32 type, JoinAvenuePerson *person, ResortSys *sys, u32 *row) {
    s32 i;
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenueShop *shop = JoinAvenuePerson_GetShop(person);
    const u16 *shopRow = ResortShopData_GetPersonShop(shopData, person);
    u32 shopKind;
    BOOL found;
    s32 best;
    u32 index;

    JoinAvenueShop_GetField(shop, 1);
    ResortShopData_GetShopParam(shopRow, 0);
    shopKind = ResortShopData_GetShopParam(shopRow, 0);
    found = FALSE;
    best = found - 1;
    index = 0;

    for (i = 0; i < 6; i++) {
        u16 condition = ResortBinary_Get(types, type, i * 6 + 6);
        u16 value = ResortBinary_Get(types, type, i * 6 + 7);
        u16 priority = ResortBinary_Get(types, type, i * 6 + 8);
        BOOL matches = FALSE;

        switch (condition) {
        case RESORT_VISIT_CONDITION_SHOP_KIND:
            if (shopKind == value) {
                matches = TRUE;
            }
            break;
        case RESORT_VISIT_CONDITION_GENDER:
            if (value == joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL)) {
                matches = TRUE;
            }
            break;
        case RESORT_VISIT_CONDITION_VALUE_0:
        case RESORT_VISIT_CONDITION_VALUE_1:
        case RESORT_VISIT_CONDITION_VALUE_2:
        case RESORT_VISIT_CONDITION_VALUE_3:
        case RESORT_VISIT_CONDITION_VALUE_4:
        case RESORT_VISIT_CONDITION_VALUE_5:
        case RESORT_VISIT_CONDITION_VALUE_6:
        case RESORT_VISIT_CONDITION_VALUE_7:
        case RESORT_VISIT_CONDITION_VALUE_8:
        case RESORT_VISIT_CONDITION_VALUE_9:
        case RESORT_VISIT_CONDITION_VALUE_10:
        case RESORT_VISIT_CONDITION_VALUE_11:
        case RESORT_VISIT_CONDITION_VALUE_12:
        case RESORT_VISIT_CONDITION_VALUE_13:
        case RESORT_VISIT_CONDITION_VALUE_14:
        case RESORT_VISIT_CONDITION_VALUE_15:
            if (value + 1 ==
                joinAveTextHandler(person, JOIN_AVE_PARAM_VALUE + (u16)(condition - RESORT_VISIT_CONDITION_VALUE_0),
                                   NULL)) {
                matches = TRUE;
            }
            break;
        case RESORT_VISIT_CONDITION_LINK_TRADES:
        case RESORT_VISIT_CONDITION_NICKNAMES:
        case RESORT_VISIT_CONDITION_AVENUE_CUSTOMERS:
        case RESORT_VISIT_CONDITION_SHOPPING_MONEY:
        case RESORT_VISIT_CONDITION_PASSERSBY:
        case RESORT_VISIT_CONDITION_LINK_BATTLES:
        case RESORT_VISIT_CONDITION_POKEMON_CAUGHT:
        case RESORT_VISIT_CONDITION_EGGS_HATCHED:
        case RESORT_VISIT_CONDITION_MEDALS: {
            u16 record = condition - RESORT_VISIT_CONDITION_LINK_TRADES;

            if (value == 0) {
                if (ResortSys_GetRecordRank(occupants, person, record) == 0) {
                    matches = TRUE;
                }
            } else if (record == RESORT_RECORD_MEDALS) {
                if (joinAveTextHandler(person, JOIN_AVE_PARAM_MEDAL_COUNT, NULL) >= value) {
                    matches = TRUE;
                }
            } else if (record < 8) {
                if (joinAveTextHandler(person, JOIN_AVE_PARAM_RECORD1 + record, NULL) >= value) {
                    matches = TRUE;
                }
            }
            break;
        }
        default:
            matches = FALSE;
            break;
        }

        if (matches) {
            found = TRUE;
            if (priority == RESORT_VISIT_PRIORITY_DOUBLE) {
                index = i;
                break;
            }
            if (best < priority) {
                best = priority;
                index = i;
            }
        }
    }

    if (row != NULL) {
        *row = index;
    }
    return found;
}

BOOL ResortSys_GetPersonVisit(ResortPersonData *data, JoinAvenuePerson *person, ResortSys *sys, u16 *message,
                              u16 *shopMessage, u16 *points) {
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u16 personType;
    u16 gender;
    BOOL isShop5;
    u32 multiplier;
    BOOL found;
    u16 kind;
    u16 shopKind;
    u32 index;

    personType = ResortData_GetParam(data, JOIN_AVE_PARAM_VISIT_TYPE, NULL);
    isShop5 = FALSE;
    gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
    multiplier = 0;
    kind = ResortData_GetType(data);
    shopKind = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    if (kind == RESORT_PERSON_ENTRY && shopKind == 5) {
        isShop5 = TRUE;
    }

    found = ResortSys_MatchCondition(personType, person, sys, &index);
    if (joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL) == 0) {
        *shopMessage = ResortBinary_Get(types, personType, index * 6 + 9);
    } else {
        *shopMessage = ResortBinary_Get(types, personType, index * 6 + 10);
    }

    if (found) {
        *points = ResortBinary_Get(types, personType, 4);
        multiplier = ResortBinary_Get(types, personType, index * 6 + 8);
        *points = (*points * sShopKindMultipliers[multiplier]) >> 12;
        if (ResortData_GetType(data) == RESORT_PERSON_ENTRY) {
            switch (ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL)) {
            case 5:
                *points = (*points * 0x2000) >> 12;
                break;
            case 4:
                *points = (*points * 0xc00) >> 12;
                break;
            }
        }
    }

    if (found) {
        if (isShop5) {
            *message = ResortSys_GetTrainerTypeText(texts, data, 15);
        } else {
            multiplier += 251;
            *message = multiplier;
        }
    } else if (isShop5) {
        *message = ResortSys_GetTrainerTypeText(texts, data, 16);
    } else if (gender == 0) {
        *message = 256;
    } else {
        *message = 257;
    }
    return found;
}

// Adds points to the avenue's popularity, and returns what that brought: 0 nothing, 1 a new rank, 2 no more ranks, 3
// a new shop, 4 a new rank with new texts, 5 both
u32 ResortSys_AddPopularity(ResortSys *sys, GameData *gameData, u32 points, u16 *rank) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    BOOL newTexts = FALSE;
    u32 popularity = JoinAvenue_GetParam(info, JOIN_AVE_INFO_POPULARITY, NULL);
    u32 required;
    u32 range;
    u32 nextRange;
    u32 oldMessage;
    u32 shop;
    BOOL newShop;

    *rank = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL);
    if (*rank >= ResortBinary_GetLastRangeEnd(ranks)) {
        return RESORT_POPULARITY_MAX;
    }

    range = ResortBinary_FindRange(ranks, *rank);
    required = ResortBinary_Get(ranks, range, 5);
    if (popularity + points < required) {
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_POPULARITY, popularity + points);
        return RESORT_POPULARITY_NONE;
    }

    (*rank)++;
    JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_RANK, *rank);
    JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_POPULARITY, 0);
    nextRange = ResortBinary_FindRange(ranks, *rank);
    oldMessage = ResortBinary_Get(ranks, range, 6);
    shop = ResortSys_GetUnlockForRank(sys, gameData, *rank);
    if (oldMessage != ResortBinary_Get(ranks, nextRange, 6)) {
        newTexts = TRUE;
    }

    if (shop != 0) {
        if (shop == 8) {
            JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_UNLOCK_PENDING_8, 1);
        }
        JoinAvenueInfo_SetParam(info, shop + JOIN_AVE_INFO_UNLOCK_PENDING_1 - 1, 1);
        newShop = TRUE;
    } else {
        newShop = FALSE;
    }

    if (newShop && newTexts) {
        return RESORT_POPULARITY_UNLOCK_TEXT;
    }
    if (newShop) {
        return RESORT_POPULARITY_UNLOCK;
    }
    return newTexts ? RESORT_POPULARITY_RANK_TEXT : RESORT_POPULARITY_RANK;
}

// Adds experience to a person's shop, and returns what that brought: 0 nothing, 1 a new level, 2 a new level of a
// different kind of shop, 3 the highest level already
u32 ResortSys_AddShopExperience(JoinAvenuePerson *person, ResortSys *sys, u32 points, u16 *level) {
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueShop *shop = JoinAvenuePerson_GetShop(person);
    u32 experience = JoinAvenueShop_GetField(shop, 2);
    const u16 *shopRow = ResortShopData_GetPersonShop(shopData, person);
    u32 needed = ResortShopData_GetShopParam(shopRow, 3);
    u32 kind = ResortShopData_GetShopParam(shopRow, 0);
    u32 column1 = ResortShopData_GetShopParam(shopRow, 1);
    const u16 *newRow;

    *level = JoinAvenueShop_GetField(shop, 1);
    if (*level >= RESORT_SHOP_MAX_LEVEL) {
        return RESORT_EXPERIENCE_MAX;
    }
    if (experience + points < needed) {
        JoinAvenueShop_SetField(shop, 2, experience + points);
        return RESORT_EXPERIENCE_NONE;
    }

    (*level)++;
    JoinAvenueShop_SetField(shop, 1, *level);
    JoinAvenueShop_SetField(shop, 2, 0);
    JoinAvenueShop_SetField(shop, 3, 0);
    JoinAvenueShop_SetField(shop, 0, ResortShopData_FindShop(shopData, kind, *level, column1));
    newRow = ResortShopData_GetPersonShop(shopData, person);
    if (ResortSys_AreAllShopsMaxLevel(sys)) {
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_MAX_LEVEL_WEEK, 1);
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_MAX_LEVEL_DAYS, 0);
    }
    if (ResortShopData_GetShopParam(shopRow, 6) != ResortShopData_GetShopParam(newRow, 6)) {
        return RESORT_EXPERIENCE_LAYOUT;
    }
    return RESORT_EXPERIENCE_LEVEL;
}

u32 ResortSys_GetMaxLevelWeek(ResortSys *sys) {
    return JoinAvenue_GetParam(ResortSys_GetInfo(sys), JOIN_AVE_INFO_MAX_LEVEL_WEEK, NULL);
}

// Whether all of the occupants' shops are at the highest level
BOOL ResortSys_AreAllShopsMaxLevel(ResortSys *sys) {
    ResortPersonData **datas = ResortSys_GetDatas(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortDataIter iter = ResortDatas_StartIter(datas, sOccupantKinds, 1);
    ResortPersonData *data;

    for (;;) {
        data = ResortDatas_NextIter(datas, &iter, sOccupantKinds, 1);
        if (data == NULL) {
            break;
        }
        if (JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(ResortData_GetPerson(data)), 1) != RESORT_SHOP_MAX_LEVEL) {
            return FALSE;
        }
    }
    return TRUE;
}

// Puts a text in the word set: the one of the kind of the first parameter, about the person that the second one
// chooses: the script's own person (0xfe), none (0xff), or the one of a kind of data of the avenue by the third
void ResortSys_SetScriptWord(u32 type, u32 who, u16 index, u32 wordIndex, ResortSys *sys, ResortPerson *person,
                             WordSet *wordSet, GameData *gameData, HeapID heapId) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    JoinAvenuePerson *player = JoinAvenueOccupants_GetPlayerPerson(ResortSys_GetOccupants(sys));
    ResortPersonData *data;
    JoinAvenuePerson *joinPerson;

    switch (who) {
    case RESORT_WORD_NO_PERSON:
        data = NULL;
        break;
    case RESORT_WORD_CURRENT_PERSON:
        data = ResortPerson_GetData(person);
        joinPerson = ResortData_GetPerson(data);
        break;
    default:
        data = ResortDatas_GetByScript(ResortSys_GetDatas(sys), who, index);
        joinPerson = ResortData_GetPerson(data);
        break;
    }

    switch (type) {
    case RESORT_WORD_PERSON_NAME: {
        StrBuf *buf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
        u32 gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 name[8];

        ResortData_GetParam(data, JOIN_AVE_PARAM_NAME, name);
        GFL_StrBufLoadString(buf, name);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_SHOP_NAME: {
        u16 shopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(joinPerson), 0);
        StrBuf *buf = ResortSys_FormatShopText(shopData, data, shopId, heapId);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL), 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_DESIRED_SHOP_NAME: {
        u16 shopId = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = ResortData_GetParam(data, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = ResortData_GetParam(data, JOIN_AVE_PARAM_VERSION, NULL);
        u16 keepShopLevel = ResortData_GetParam(data, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, NULL);
        StrBuf *buf = ResortSys_FormatShopText(
            shopData, data, JoinAvenue_ChooseShop(shopId, trainerId, version, keepShopLevel, info, shopData), heapId);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL), 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_GREETING: {
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 greeting[9];

        ResortData_GetParam(data, JOIN_AVE_PARAM_GREETING, greeting);
        GFL_StrBufLoadString(buf, greeting);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_AVENUE_TITLE: {
        StrBuf *buf = GFL_StrBufCreate(11, HEAPID_TAIL(heapId));
        u32 gender = getTrainerGender(GetGameDataPlayerInfo(gameData));
        u16 name[11];

        JoinAvenue_GetParam(info, JOIN_AVE_INFO_TITLE, name);
        GFL_StrBufLoadString(buf, name);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_MESSAGE1: {
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 message[9];

        ResortData_GetParam(data, JOIN_AVE_PARAM_MESSAGE1, message);
        GFL_StrBufLoadString(buf, message);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_MESSAGE2: {
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 message[9];

        ResortData_GetParam(data, JOIN_AVE_PARAM_MESSAGE2, message);
        GFL_StrBufLoadString(buf, message);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_PLAYER_MESSAGE1: {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = getTrainerGender(playerInfo);
        u16 message[9];

        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE1, message);
        GFL_StrBufLoadString(buf, message);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_PLAYER_MESSAGE2: {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = getTrainerGender(playerInfo);
        u16 message[9];

        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE2, message);
        GFL_StrBufLoadString(buf, message);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_YEAR:
        WordSetNumber(wordSet, wordIndex, ResortData_GetParam(data, JOIN_AVE_PARAM_YEAR, NULL), 2, NUM_PAD_ZERO, TRUE);
        break;
    case RESORT_WORD_MONTH:
        WordSetNumber(wordSet, wordIndex, ResortData_GetParam(data, JOIN_AVE_PARAM_MONTH, NULL), 2, NUM_PAD_NONE, TRUE);
        break;
    case RESORT_WORD_DAY:
        WordSetNumber(wordSet, wordIndex, ResortData_GetParam(data, JOIN_AVE_PARAM_DAY, NULL), 2, NUM_PAD_NONE, TRUE);
        break;
    case RESORT_WORD_SHOP_KIND_DESCRIPTION: {
        u16 shopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(joinPerson), 0);
        u32 shopName = ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shopId), 0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 280);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case RESORT_WORD_DESIRED_SHOP_KIND: {
        u16 shopId = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = ResortData_GetParam(data, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = ResortData_GetParam(data, JOIN_AVE_PARAM_VERSION, NULL);
        u16 keepShopLevel = ResortData_GetParam(data, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, NULL);
        u32 shopName = ResortShopData_GetShopParam(
            ResortShopData_GetShop(shopData,
                                   JoinAvenue_ChooseShop(shopId, trainerId, version, keepShopLevel, info, shopData)),
            0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 280);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case RESORT_WORD_PLAYER_SHOP_KIND: {
        JoinAvenuePerson *occupant = JoinAvenueOccupants_GetPlayerPerson(ResortSys_GetOccupants(sys));
        u32 shopName = ResortShopData_GetShopParam(
            ResortShopData_GetShop(shopData,
                                   (u16)JoinAvenuePerson_GetShopId(occupant, GetGameDataPlayerInfo(gameData))),
            0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 280);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case RESORT_WORD_AVENUE_NAME: {
        StrBuf *buf = GFL_StrBufCreate(21, HEAPID_TAIL(heapId));
        u16 name[21];

        JoinAvenue_GetParam(info, JOIN_AVE_INFO_NAME, name);
        GFL_StrBufLoadString(buf, name);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case RESORT_WORD_PROMOTION_DAYS:
        WordSetNumber(wordSet, wordIndex, 7 - JoinAvenue_GetParam(info, JOIN_AVE_INFO_MAX_LEVEL_DAYS, NULL), 1,
                      NUM_PAD_NONE, TRUE);
        break;
    case RESORT_WORD_SHOP_EXPERIENCE_REMAINING: {
        const u16 *shopRow = ResortShopData_GetPersonShop(shopData, joinPerson);
        JoinAvenueShop *shop = JoinAvenuePerson_GetShop(joinPerson);
        s32 remaining = ResortShopData_GetShopParam(shopRow, 3) - (u16)JoinAvenueShop_GetField(shop, 2);

        if (remaining < 0) {
            remaining = 0;
        }
        WordSetNumber(wordSet, wordIndex, remaining, 5, NUM_PAD_NONE, TRUE);
        break;
    }
    case RESORT_WORD_SHOP_LEVEL: {
        u16 level;

        ResortShopData_GetPersonShop(shopData, joinPerson);
        level = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(joinPerson), 1);
        WordSetNumber(wordSet, wordIndex, level + 1, 4, NUM_PAD_NONE, TRUE);
        break;
    }
    case RESORT_WORD_AVENUE_RANK:
        WordSetNumber(wordSet, wordIndex, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL), 4, NUM_PAD_NONE, TRUE);
        break;
    case RESORT_WORD_AVENUE_RANK_TITLE: {
        u32 message = ResortBinary_Get(
            ranks, ResortBinary_FindRange(ranks, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL)), 6);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, message);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case RESORT_WORD_POPULARITY_REMAINING: {
        u32 rank = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL);
        u32 popularity = JoinAvenue_GetParam(info, JOIN_AVE_INFO_POPULARITY, NULL);
        s32 remaining = ResortBinary_Get(ranks, ResortBinary_FindRange(ranks, rank), 5) - popularity;

        if (remaining < 0) {
            remaining = 0;
        }
        WordSetNumber(wordSet, wordIndex, remaining, 5, NUM_PAD_NONE, TRUE);
        break;
    }
    case RESORT_WORD_SHOP_KIND: {
        u32 shopName = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shopData, joinPerson), 0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 320);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case RESORT_WORD_ARRIVAL_METHOD: {
        u32 arrivalMethod = ResortData_GetParam(data, JOIN_AVE_PARAM_ARRIVAL_METHOD, NULL);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, arrivalMethod + 1488);

        GFL_WordSetSetStrbuf(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    }
}

// The tier of a new person, rolled with the weights of the avenue's rank
static u32 ResortSys_RollTier(ResortPersonData *data, ResortSys *sys) {
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u32 total = 0;
    u32 range = ResortBinary_FindRange(ranks, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL));
    u32 roll = ResortData_Roll(data, 2, 100);
    s32 tier;

    for (tier = 0; tier < 4; tier++) {
        total += ResortBinary_Get(ranks, range, tier + 2);
        if (total > roll) {
            return tier;
        }
    }
    return 0;
}

// Whether a type of person is available at the avenue's popularity
static BOOL ResortSys_IsTypeAvailable(ResortSys *sys, ResortBinary *table, u16 row) {
    u32 unlock = ResortBinary_Get(table, row, 2);
    u16 rank;
    BOOL available;
    ResortBinary *shops;
    JoinAvenueInfo *info;

    switch (unlock) {
    default:
        return FALSE;
    case 0:
        return TRUE;
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
    case 16:
        info = ResortSys_GetInfo(sys);
        shops = ResortSys_GetShopOpenings(sys);
        available = FALSE;
        rank = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL);
        if (rank >= ResortBinary_Get(shops, unlock - 1, 4)) {
            available = TRUE;
        }
        return available;
    }
}

// A random type of person of the tier, among those available
static u32 ResortSys_PickTypeOfTier(ResortPersonData *data, ResortSys *sys, u32 tier) {
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    u32 count = 0;
    u32 row;
    u32 pick;

    for (row = 1; row < ResortBinary_GetRowCount(types); row++) {
        u32 rowTier = ResortBinary_Get(types, row, 3);
        BOOL available = ResortSys_IsTypeAvailable(sys, types, row);

        if (tier == rowTier && available) {
            count++;
        }
    }

    pick = ResortData_Roll(data, 3, count);
    count = 0;
    for (row = 1; row < ResortBinary_GetRowCount(types); row++) {
        u32 rowTier = ResortBinary_Get(types, row, 3);
        BOOL available = ResortSys_IsTypeAvailable(sys, types, row);

        if (tier == rowTier && available) {
            if (count == pick) {
                return row;
            }
            count++;
        }
    }
    return 1;
}

// The type of person that a new person is made, by the tier it rolls
static u32 ResortSys_ChoosePersonType(ResortPersonData *data, ResortSys *sys, GameData *gameData) {
    u32 result;
    u32 tier = ResortSys_RollTier(data, sys);
    BOOL hasPlayerShop = FALSE;
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenuePerson *player = JoinAvenueOccupants_GetPlayerPerson(ResortSys_GetOccupants(sys));
    u32 playerShopKind = ResortShopData_GetShopParam(
        ResortShopData_GetShop(shopData, (u16)JoinAvenuePerson_GetShopId(player, GetGameDataPlayerInfo(gameData))), 0);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    s32 i;

    for (i = 0; i < RESORT_PERSON_COUNT; i++) {
        JoinAvenuePerson *person = JoinAvenueOccupants_GetPerson(occupants, i);

        if (!JoinAvenuePerson_IsEmpty(person)) {
            u32 shopKind;

            JoinAvenuePerson_GetShop(person);
            shopKind = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(ResortSys_GetShopData(sys), person), 0);
            if (shopKind == playerShopKind) {
                hasPlayerShop = TRUE;
                break;
            }
        }
    }

    if (hasPlayerShop) {
        s32 roll = ResortData_Roll(data, 4, 100);

        if (roll < sNewShopChances[tier]) {
            return playerShopKind + 1;
        }
    }

    result = ResortSys_PickTypeOfTier(data, sys, tier);
    if (tier != 0) {
        JoinAvenueOccupants *occupants2;
        BOOL unused;
        u16 type = result;
        u32 row;

        ResortSys_GetPersonTypes(sys);
        occupants2 = ResortSys_GetOccupants(sys);
        unused = TRUE;
        for (i = 0; i < RESORT_PERSON_COUNT; i++) {
            JoinAvenuePerson *person = JoinAvenueOccupants_GetPerson(occupants2, i);

            if (!JoinAvenuePerson_IsEmpty(person) && ResortSys_MatchCondition(type, person, sys, &row)) {
                unused = FALSE;
                break;
            }
        }
        if (unused) {
            result = ResortSys_PickTypeOfTier(data, sys, 0);
        }
    }
    return result;
}

MoneyWin *ResortRecords_CreateWindow(FieldMsgBG *msgBGSys, HeapID heapId) {
    return FieldMsgBG_CreateMoneyWin(msgBGSys, NULL, 1, 1, 30, 22);
}

// Draws a page of the avenue's records: the people with the most of one thing, with their rank and number
void ResortRecords_PrintPage(MoneyWin *window, JoinAvenueOccupants *occupants, FieldMsgBG *msgBGSys, WordSet *wordSet,
                             GameData *gameData, u32 page, HeapID heapId) {
    ResortRankEntry entries[RESORT_PERSON_COUNT + 1];
    s32 total;
    s32 count = 0;
    u32 rank = 0;
    u32 position = 0;
    ResortWork *work = GameData_GetResortWork(gameData);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    StrBuf *nameBuf;
    MsgData *msgData;
    Font *font;
    StrBuf *templateBuf;
    StrBuf *lineBuf;
    s32 i;
    s32 j;

    for (i = 0; i < RESORT_PERSON_COUNT + 1; i++) {
        entries[i].index = -1;
        entries[i].value = -1;
    }

    for (j = RESORT_PERSON_COUNT - 1; j >= 0; j--) {
        JoinAvenuePerson *person = JoinAvenueOccupants_GetPerson(occupants, j);

        if (!JoinAvenuePerson_IsEmpty(person)) {
            if ((JoinAvenuePerson_HasExtraData(person, 1) && page < 4) ||
                (JoinAvenuePerson_HasExtraData(person, 2) && page >= 4) ||
                (JoinAvenuePerson_HasExtraData(person, 0) && page == RESORT_RECORD_MEDALS)) {
                ResortRankEntry *entry = &entries[count];

                entry->index = j;
                if (page < 8) {
                    entry->value = joinAveTextHandler(person, JOIN_AVE_PARAM_RECORD1 + page, NULL);
                } else {
                    entry->value = joinAveTextHandler(person, JOIN_AVE_PARAM_MEDAL_COUNT, NULL);
                }
                if (entry->value != 0) {
                    count++;
                }
            }
        }
    }

    entries[count].index = RESORT_PERSON_COUNT;
    if (page < 8) {
        entries[count].value = ResortWork_Get(work, page);
    } else {
        entries[count].value = MedalBox_GetObtainedCount(SaveControl_GetMedalBox(GameData_GetSaveControl(gameData)), 0);
    }

    for (i = 0; i < count; i++) {
        for (j = count; j > i; j--) {
            ResortRankEntry *upper = &entries[j - 1];
            u32 value = upper->value;
            ResortRankEntry *lower = &entries[j];

            if (value < lower->value) {
                u32 index = upper->index;

                *upper = *lower;
                lower->value = value;
                lower->index = index;
            }
        }
    }

    nameBuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
    templateBuf = GFL_StrBufCreate(128, HEAPID_TAIL(heapId));
    lineBuf = GFL_StrBufCreate(128, HEAPID_TAIL(heapId));
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    font = MsgBGPrinter_GetFont(msgBGSys);
    MsgBGPrinter_ClearMsgWin(window);
    GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + page, lineBuf);
    MsgBGPrinter_MsgWinPrintStr(window, 120 - GFL_FontGetBlockWidth(lineBuf, font, 0) / 2, 0, lineBuf);

    total = count + 1;
    for (i = 0; i < total; i++) {
        ResortRankEntry *entry = &entries[i];
        u32 index = entry->index;
        JoinAvenuePerson *person;
        u32 width;
        u32 y;
        u32 value;
        BOOL shared;

        if (index == RESORT_RECORD_PLAYER_INDEX) {
            person = JoinAvenueOccupants_GetPlayerPerson(occupants);
        } else {
            person = JoinAvenueOccupants_GetPerson(occupants, index);
        }

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + 9, templateBuf);
        WordSetNumber(wordSet, 0, rank + 1, 1, 1, 1);
        GFL_WordSetFormatStrbuf(wordSet, lineBuf, templateBuf);
        width = GFL_FontGetBlockWidth(lineBuf, font, 0);
        y = (i * 2 + 3) * 8;
        MsgBGPrinter_MsgWinPrintStr(window, 40 - width, y, lineBuf);

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + 10, templateBuf);
        if (index == RESORT_RECORD_PLAYER_INDEX) {
            copyVarForText(wordSet, 0, playerInfo);
        } else {
            u32 gender = joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL);
            u16 name[8];

            joinAveTextHandler(person, JOIN_AVE_PARAM_NAME, name);
            GFL_StrBufLoadString(nameBuf, name);
            GFL_WordSetSetStrbuf(wordSet, 0, nameBuf, gender, 1, 2);
        }
        GFL_WordSetFormatStrbuf(wordSet, lineBuf, templateBuf);
        GFL_FontGetBlockWidth(lineBuf, font, 0);
        MsgBGPrinter_MsgWinPrintStr(window, 56, y, lineBuf);

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + RESORT_WORK_MET_FIRST, lineBuf);
        MsgBGPrinter_MsgWinPrintStr(window, 128, y, lineBuf);

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + 12, templateBuf);
        value = entry->value;
        WordSetNumber(wordSet, 0, value, 9, 1, 1);
        GFL_WordSetFormatStrbuf(wordSet, lineBuf, templateBuf);
        MsgBGPrinter_MsgWinPrintStr(window, 216 - GFL_FontGetBlockWidth(lineBuf, font, 0), y, lineBuf);

        shared = TRUE;
        if (i + 1 < total && value == entries[i + 1].value) {
            shared = FALSE;
        }
        position++;
        if (shared) {
            rank = position;
        }
    }

    GFL_MsgDataFree(msgData);
    GFL_StrBufFree(lineBuf);
    GFL_StrBufFree(templateBuf);
    GFL_StrBufFree(nameBuf);
}

// The rank of the avenue's popularity that a tier gets
u32 ResortSys_GetUnlock(ResortSys *sys, GameData *gameData) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u32 i;

    for (i = 0; i < RESORT_SHOP_KIND_COUNT; i++) {
        const ResortUnlock *unlock = &sUnlocks[i];

        if (unlock->check(sys, gameData) && JoinAvenue_GetParam(info, unlock->pendingParam, NULL) != 0) {
            return sUnlocks[i].id;
        }
    }
    return 0;
}

// The row of the table whose column 9 is an unlock
static u32 ResortSys_FindUnlockRow(ResortBinary *table, u32 id) {
    u32 row;

    for (row = 0; row < ResortBinary_GetRowCount(table); row++) {
        if (id == ResortBinary_Get(table, row, 9)) {
            return row;
        }
    }
    return 0;
}

u32 ResortSys_GetUnlockForRank(ResortSys *sys, GameData *gameData, u32 rank) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    BOOL rankReached;
    BOOL blocked;
    BOOL available;
    const ResortUnlock *unlock;
    u32 i;

    for (i = 0; i < RESORT_SHOP_KIND_COUNT; i++) {
        rankReached = FALSE;
        unlock = &sUnlocks[i];
        blocked = FALSE;
        available = FALSE;
        if (rank >= ResortBinary_Get(ranks, ResortSys_FindUnlockRow(ranks, unlock->id), 0)) {
            rankReached = TRUE;
        }
        if (JoinAvenue_GetParam(info, unlock->unlockedParam, NULL) != 0) {
            blocked = FALSE;
            if (unlock->flag == 0) {
                blocked = TRUE;
            }
        }
        if (JoinAvenue_GetParam(info, unlock->pendingParam, NULL) != 0) {
            blocked = TRUE;
        }
        if (unlock->check(sys, gameData)) {
            available = TRUE;
        }
        if (rankReached && available && !blocked) {
            return sUnlocks[i].id;
        }
    }
    return 0;
}

// The rank of a person among the occupants for a record, or 8 if the person has none
static u32 ResortSys_GetRecordRank(JoinAvenueOccupants *occupants, JoinAvenuePerson *person, u32 page) {
    ResortRankSelf entries[RESORT_PERSON_COUNT];
    s32 count;
    u32 position;
    u32 result;
    s32 i;
    s32 j;

    count = 0;
    result = 0;
    position = 0;
    for (i = 0; i < RESORT_PERSON_COUNT; i++) {
        entries[i].index = -1;
        entries[i].value = -1;
        entries[i].isSelf = FALSE;
    }

    for (i = 0; i < RESORT_PERSON_COUNT; i++) {
        JoinAvenuePerson *other = JoinAvenueOccupants_GetPerson(occupants, i);

        if (!JoinAvenuePerson_IsEmpty(other)) {
            if ((JoinAvenuePerson_HasExtraData(other, 1) && page < 4) ||
                (JoinAvenuePerson_HasExtraData(other, 2) && page >= 4 && page < 8) ||
                (JoinAvenuePerson_HasExtraData(other, 0) && page == RESORT_RECORD_MEDALS)) {
                ResortRankSelf *entry = &entries[count];

                entry->index = i;
                if (page == RESORT_RECORD_MEDALS) {
                    entry->value = joinAveTextHandler(other, JOIN_AVE_PARAM_MEDAL_COUNT, NULL);
                } else {
                    entry->value = joinAveTextHandler(other, JOIN_AVE_PARAM_RECORD1 + page, NULL);
                }
                if (person == other) {
                    entry->isSelf = TRUE;
                }
                count++;
            }
        }
    }

    for (i = 0; i < count - 1; i++) {
        for (j = count - 1; j > i; j--) {
            s32 prev = j - 1;

            if (entries[prev].value < entries[j].value) {
                u32 value = entries[prev].value;
                u32 index = entries[prev].index;
#ifdef BUGFIX
                BOOL isSelf = entries[prev].isSelf;

                entries[prev] = entries[j];
                entries[j].value = value;
                entries[j].index = index;
                entries[j].isSelf = isSelf;
#else
                // BUG: only the value and the index are put back, so the person asked about loses isSelf when
                // moved down and the function returns 8. Its callers only test for 0, which it isn't then anyway
                entries[prev] = entries[j];
                entries[j].value = value;
                entries[j].index = index;
#endif
            }
        }
    }

    for (i = 0; i < count; i++) {
        BOOL shared;

        if (entries[i].isSelf) {
            if (entries[i].value == 0) {
                result = 8;
                return result;
            }
            return result;
        }

        shared = TRUE;
        if (i + 1 < count && entries[i].value == entries[i + 1].value) {
            shared = FALSE;
        }
        position++;
        if (shared) {
            result = position;
        }
    }
    return 8;
}

// The column of the texts for a person's trainer type
u32 ResortSys_GetTrainerTypeText(ResortBinary *texts, ResortPersonData *data, u32 column) {
    u32 trainerType = ResortData_GetParam(data, JOIN_AVE_PARAM_TRAINER_TYPE, NULL);
    u32 row;

    for (row = 0; row < ResortBinary_GetRowCount(texts); row++) {
        if (trainerType == ResortBinary_Get(texts, row, 2)) {
            return ResortBinary_Get(texts, row, column);
        }
    }
    return 0;
}

// Hides the people and NPCs who would share the screen with the player's; mode 2 only hides those that are behind
// the camera
void ResortSys_HideOverlappingActors(ResortPeople *people, ResortNPC *npc, Field *field, ResortPerson *person,
                                     u32 mode) {
    FieldPlayer *player = Field_GetPlayer(field);
    FieldCamera *camera = Field_GetCameraSystem(field);
    BOOL cameraOnly = FALSE;
    u32 i;

    if (mode == RESORT_HIDE_CAMERA_FILTERED) {
        cameraOnly = TRUE;
        mode = RESORT_HIDE_PEOPLE;
    }

    for (i = 0; i < ResortPeople_GetCount(people); i++) {
        ResortPerson *other = ResortPeople_GetByIndex(people, i);

        if (other != NULL) {
            u32 type = ResortPerson_GetAttr(other, RESORT_PERSON_ATTR_TYPE);
            const u32 *hidden = sHiddenTypes[mode];
            s32 j;

            for (j = 0; j < 4 && hidden[j] != RESORT_PERSON_TYPE_MAX; j++) {
                if (other != person && type == hidden[j]) {
                    BOOL hide = TRUE;

                    if (cameraOnly) {
                        if (ResortActor_IsBehindCamera(ResortPerson_GetActor(other), camera)) {
                            hide = FALSE;
                        }
                    }
                    if (hide) {
                        ResortPerson_ApplyVisibility(other, RESORT_VISIBILITY_HIDE);
                    }
                    break;
                }
            }
        }
    }

    if (cameraOnly) {
        FieldActor *actor = FieldPlayer_GetActor(player);

        if (!ResortActor_IsBehindCamera(actor, camera)) {
            SetActorHidden(actor, TRUE);
        }
    }

    if (cameraOnly) {
        for (i = 0; i < ResortNPC_GetCount(npc); i++) {
            FieldActor *actor = ResortNPC_GetActor(npc, i);

            if (actor != NULL && !ResortActor_IsBehindCamera(actor, camera)) {
                SetActorHidden(actor, TRUE);
            }
        }
    } else {
        ResortNPC_SetVisible(npc, FALSE);
    }
}

// Whether an actor is behind the camera's eye
static BOOL ResortActor_IsBehindCamera(FieldActor *actor, FieldCamera *camera) {
    VecFx32 pos;
    VecFx32 eye;

    FieldCamera_CoordsGetEye(camera, &eye);
    CopyActorWPos(actor, &pos);
    if (pos.z < eye.z - 128 * FX32_ONE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ResortUnlock_Always(ResortSys *sys, GameData *gameData) {
    return TRUE;
}

static BOOL ResortUnlock_FlagSet(ResortSys *sys, GameData *gameData) {
    return EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960);
}

static BOOL ResortUnlock_AllPeople(ResortSys *sys, GameData *gameData) {
    return JoinAvenueOccupants_CountPeople(ResortSys_GetOccupants(sys)) == RESORT_PERSON_COUNT;
}

ResortSys *ResortSys_Get(Field *field) {
    if (Field_CheckGimmickWorkPassword(field, 1)) {
        return ResortField_GetSys(field);
    }
    return ResortField0_GetSys(field);
}

ResortPerson *ResortSys_GetScriptPerson(FieldScriptEnv *env, ResortPeople *people) {
    return ResortPeople_FindByActor(people, ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env)));
}

// Makes the person of a kind of data of the avenue from another, or gives the data a new shop of its kind. Returns the
// index of the data made, or 0
u32 ResortSys_MakeData(ResortSys *sys, ResortPersonData *data, u32 kind) {
    ResortSlots *slots = ResortSys_GetSlots(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    ResortPersonData *made = NULL;
    u32 row = ResortLayout_GetSlotRow((u16)kind);
    u32 dataKind = ResortData_GetType(data);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u16 slot;

    if (dataKind == kind) {
        JoinAvenuePerson *person = ResortData_GetPerson(data);
        u16 oldShopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(person), 0);
        const u16 *shop = ResortShopData_GetShop(shopData, oldShopId);
        u16 column2 = ResortShopData_GetShopParam(shop, 2);
        u16 column1 = ResortShopData_GetShopParam(shop, 1);
        u16 shopId = joinAveTextHandler(person, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = joinAveTextHandler(person, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = joinAveTextHandler(person, JOIN_AVE_PARAM_VERSION, NULL);
        u16 keepShopLevel = joinAveTextHandler(person, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, NULL);
        u16 newShop;
        u32 status;

        newShop = JoinAvenue_ChooseShop(shopId, trainerId, version, keepShopLevel, info, shopData);
        newShop = ResortShopData_FindShop(
            shopData, ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, newShop), 0), column2, column1);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_SHOP_ID, newShop);
        slot = joinAveTextHandler(person, JOIN_AVE_PARAM_SLOT, NULL);
        status = joinAveTextHandler(person, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, NULL);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, 1);
        JoinAvenuePerson_InitFromPerson(person, person, shopData, slot, row);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_KEEP_SHOP_LEVEL, status);
        return ResortData_GetIndex(data);
    }

    if (ResortSys_GetEmptyData(sys, ResortData_GetMakeKind(kind), row, &slot, &made)) {
        switch (sMakeKinds[kind][dataKind]) {
        case RESORT_MAKE_PERSON_FROM_PERSON:
            JoinAvenuePerson_InitFromPerson(ResortData_GetPerson(made), ResortData_GetPerson(data), shopData, slot,
                                            row);
            break;
        case RESORT_MAKE_PERSON_FROM_ENTRY:
            JoinAvenuePerson_InitFromEntry(ResortData_GetPerson(made), ResortData_GetPerson(data), shopData, slot, row);
            break;
        case RESORT_MAKE_RECORD_FROM_PERSON: {
            JoinAvenueRecord *record = ResortData_GetPerson(made);
            u16 shopKind = ResortData_GetIndex(made);

            JoinAvenueRecord_InitFromPerson(record, ResortData_GetPerson(data), slot, (u16)row, shopKind);
            break;
        }
        case RESORT_MAKE_RECORD_FROM_ENTRY: {
            JoinAvenueRecord *record = ResortData_GetPerson(made);
            u16 shopKind = ResortData_GetIndex(made);

            JoinAvenueRecord_InitFromEntry(record, ResortData_GetPerson(data), slot, (u16)row, shopKind);
            break;
        }
        default:
            return 0;
        }

        ResortSlots_Fill(slots);
        return ResortData_GetIndex(made);
    }
    return 0;
}

// The first empty data of the kind, and the slot it would be placed in; both are returned when given
static BOOL ResortSys_GetEmptyData(ResortSys *sys, u32 kind, u32 row, u16 *slot, ResortPersonData **data) {
    ResortSlots *slots = ResortSys_GetSlots(sys);
    ResortPersonData **datas = ResortSys_GetDatas(sys);
    u16 emptySlot = ResortSlots_FindEmpty(slots, row);
    ResortPersonData *empty = ResortDatas_FindEmpty(datas, kind);

    if (emptySlot == RESORT_SLOT_NONE) {
        return FALSE;
    }
    if (empty == NULL) {
        return FALSE;
    }
    if (slot != NULL) {
        *slot = emptySlot;
    }
    if (data != NULL) {
        *data = empty;
    }
    return TRUE;
}

// The text of a shop for a person, with the person's name in it
StrBuf *ResortSys_FormatShopText(ResortShopData *shops, ResortPersonData *data, u16 shopId, HeapID heapId) {
    const u16 *shop = ResortShopData_GetShop(shops, shopId);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(heapId));
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    StrBuf *src = GFL_MsgDataLoadStrbufNew(msgData, ResortShopData_GetShopParam(shop, 4));
    StrBuf *nameBuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
    StrBuf *dest = GFL_StrBufCreate(64, heapId);
    u32 gender = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);
    u16 name[8];

    ResortData_GetParam(data, JOIN_AVE_PARAM_NAME, name);
    GFL_StrBufLoadString(nameBuf, name);
    GFL_WordSetSetStrbuf(wordSet, 0, nameBuf, gender, 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, dest, src);
    GFL_StrBufFree(nameBuf);
    GFL_StrBufFree(src);
    GFL_MsgDataFree(msgData);
    GFL_WordSetSystemFree(wordSet);
    return dest;
}

// Fills an entry from a row of the texts
static void ResortEntry_Fill(JoinAvenueEntry *entry, u16 shopKind, ResortBinary *table, u16 row, HeapID heapId) {
    u32 nameMessage;
    u32 greetingMessage;
    u32 message1;
    u32 message2;
    u32 gender;
    u32 trainerType;
    u32 trainerId;
    u32 shopId;
    u32 visitType;
    MsgData *msgData;
    StrBuf *buf;
    RTCDate date;

    JoinAvenueEntry_Clear(entry);
    nameMessage = ResortBinary_Get(table, row, 1);
    greetingMessage = ResortBinary_Get(table, row, 7);
    message1 = ResortBinary_Get(table, row, 8);
    message2 = ResortBinary_Get(table, row, 9);
    gender = ResortBinary_Get(table, row, 3);
    trainerType = ResortBinary_Get(table, row, 2);
    trainerId = ResortBinary_Get(table, row, 4);
    shopId = ResortBinary_Get(table, row, 5);
    visitType = ResortBinary_Get(table, row, 6);

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    buf = GFL_MsgDataLoadStrbufNew(msgData, nameMessage);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_NAME, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    buf = GFL_MsgDataLoadStrbufNew(msgData, greetingMessage);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_GREETING, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    buf = GFL_MsgDataLoadStrbufNew(msgData, message1);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_MESSAGE1, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    buf = GFL_MsgDataLoadStrbufNew(msgData, message2);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_MESSAGE2, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_GENDER, gender);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_TRAINER_TYPE, trainerType);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_TRAINER_ID, trainerId);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_SHOP_ID, shopId);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_VISIT_TYPE, visitType);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_SHOP_KIND, shopKind);
    GFL_MsgDataFree(msgData);

    RTC_GetCachedDate(&date);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_YEAR, date.year);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_MONTH, date.month);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_DAY, date.day);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_SEED, GFL_RandomLC(0));
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_VERSION, GAME_VERSION);
    JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_LANGUAGE, 2);
}

// Makes an entry of a kind of shop, for the kind's row of the texts that the avenue's rank picks
void ResortSys_MakeShopEntry(ResortSys *sys, GameData *gameData, JoinAvenueEntry *entry, u32 shopKind, u32 salt,
                             HeapID heapId) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 pick;
    u32 selected;
    u32 type;
    u32 count;
    u32 i;

    JoinAvenueOccupants_GetPlayerPerson(occupants);
    selected = 0xff;
    type = 0xff;
    for (i = 0; i < 2; i++) {
        if (shopKind == sShopKindTexts[i].key) {
            type = sShopKindTexts[i].value;
            break;
        }
    }

    count = 0;
    for (i = 0; i < ResortBinary_GetRowCount(texts); i++) {
        if (type == ResortBinary_Get(texts, i, 0)) {
            count++;
        }
    }

    pick = JoinAvenueInfo_Random(info, salt + 22, count);
    count = 0;
    for (i = 0; i < ResortBinary_GetRowCount(texts); i++) {
        if (type == ResortBinary_Get(texts, i, 0)) {
            if (count == pick) {
                selected = i;
                break;
            }
            count++;
        }
    }

    ResortEntry_Setup(sys, gameData, entry, shopKind, selected, heapId);
}

static void ResortEntry_Setup(ResortSys *sys, GameData *gameData, JoinAvenueEntry *entry, u32 shopKind, u16 row,
                              HeapID heapId) {
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    JoinAvenuePerson *player = JoinAvenueOccupants_GetPlayerPerson(occupants);
    u16 message[64];

    ResortEntry_Fill(entry, shopKind, texts, row, heapId);
    if (shopKind == 4) {
        JoinAvenueEntry_SetParam(
            entry, JOIN_AVE_PARAM_GREETING,
            (u32)TrainerGameInfo_GetGreeting(getTrainerGameInfoAddress(GameData_GetSaveControl(gameData))));
        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE1, message);
        JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_MESSAGE1, (u32)message);
        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE2, message);
        JoinAvenueEntry_SetParam(entry, JOIN_AVE_PARAM_MESSAGE2, (u32)message);
    }
}

static void ResortRecord_Setup(ResortSys *sys, JoinAvenueRecord *record, u16 shopKind, u32 type, u16 slot,
                               HeapID heapId) {
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 row;
    JoinAvenueEntry *entry;

    for (row = 0; row < ResortBinary_GetRowCount(texts); row++) {
        if (type == ResortBinary_Get(texts, row, 0)) {
            break;
        }
    }

    entry = JoinAvenueEntry_Create(HEAPID_TAIL(heapId));
    ResortEntry_Fill(entry, 0, texts, row, heapId);
    JoinAvenueRecord_InitFromEntry(record, entry, slot, 2, shopKind);
    JoinAvenueEntry_Free(entry);
}

static void ResortEntry_SetupFromType(ResortSys *sys, JoinAvenueEntry *entry, u16 shopKind, u32 type, HeapID heapId) {
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 row;

    for (row = 0; row < ResortBinary_GetRowCount(texts); row++) {
        if (type == ResortBinary_Get(texts, row, 0)) {
            break;
        }
    }

    ResortEntry_Fill(entry, shopKind, texts, row, heapId);
}

// Adds an entry (a2 1) or a record (a2 3) of a kind of shop to the avenue
void ResortSys_AddEntryOrRecord(ResortSys *sys, PlayerInfo *playerInfo, u32 mode, u32 shopKind, HeapID heapId) {
    ResortSlots *slots = ResortSys_GetSlots(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    JoinAvenueEntryList *entries = ResortSys_GetEntries(sys);

    ResortSys_GetTexts(sys);
    switch (mode) {
    case RESORT_PERSON_ENTRY: {
        u16 type = 0xff;
        JoinAvenueEntry *entry;
        u32 i;

        for (i = 0; i < 3; i++) {
            if (shopKind == sEntryTypes[i].key) {
                type = sEntryTypes[i].value;
            }
        }
        entry = JoinAvenueEntry_Create(HEAPID_TAIL(heapId));
        ResortEntry_SetupFromType(sys, entry, type, shopKind, heapId);
        if (shopKind == 0) {
            JoinAvenueEntry_SetParam(
                entry, JOIN_AVE_PARAM_SHOP_ID,
                JoinAvenuePerson_GetShopId(JoinAvenueOccupants_GetPlayerPerson(occupants), playerInfo));
        }
        JoinAvenueEntryList_Add(entries, entry);
        JoinAvenueEntry_Free(entry);
        break;
    }
    case RESORT_PERSON_RECORD: {
        u16 type = 0xff;
        JoinAvenueRecord *record;
        u32 i;

        for (i = 0; i < 4; i++) {
            if (shopKind == sRecordTypes[i].key) {
                type = sRecordTypes[i].value;
            }
        }
        record = JoinAvenueRecord_Create(HEAPID_TAIL(heapId));
        ResortRecord_Setup(sys, record, type, shopKind, ResortSlots_FindEmpty(slots, 2), heapId);
        MI_CpuCopy8(record, JoinAvenueOccupants_GetEmptyRecord(occupants), sizeof(JoinAvenueRecord));
        ResortSlots_Fill(slots);
        JoinAvenueRecord_Free(record);
        break;
    }
    }
}

// Picks a person from the table, to let in
void ResortSys_ChooseVisitGroup(ResortSys *sys, ResortPersonData *occupant, ResortPersonData *data) {
    ResortWork *work;
    u32 selected;
    u32 roll;
    u32 total;
    u32 i;
    ResortBinary *table;
    u16 index;

    ResortSys_GetInfo(sys);
    work = ResortSys_GetWork(sys);
    selected = 0xffff;
    roll = ResortData_RollPair(occupant, data, 25, 100);
    total = 0;
    table = ResortBinary_Load(12, 8, HEAPID_GAMEEVENT);
    for (i = 0; i < ResortBinary_GetRowCount(table); i++) {
        total += ResortBinary_Get(table, i, 0);
        if (roll < total) {
            selected = i;
            break;
        }
    }
    if (selected > ResortBinary_GetRowCount(table)) {
        selected = 0;
    }
    ResortBinary_Free(table);
    ResortSys_ResetWork(sys, selected);
    index = ResortData_GetIndex(occupant);
    ResortWork_Set(work, index + RESORT_WORK_MET_FIRST, 1);
}

static void ResortSys_ResetWork(ResortSys *sys, u32 value) {
    ResortWork *work;
    s32 i;

    ResortSys_GetInfo(sys);
    work = ResortSys_GetWork(sys);
    ResortWork_Set(work, RESORT_WORK_GROUP, value);
    ResortWork_Set(work, RESORT_WORK_VISITS, 0);
    for (i = RESORT_WORK_MET_FIRST; i <= RESORT_WORK_MET_LAST; i++) {
        ResortWork_Set(work, i, 0);
    }
}

// Picks which of the people not yet met has something to say, among the people near the one asked about and then the
// others
BOOL ResortSys_PickPersonToMeet(ResortSys *sys, ResortPersonData *occupant, ResortPersonData *data, u32 *out) {
    ResortWork *work = ResortSys_GetWork(sys);
    u16 index;
    s32 count;
    s32 i;
    u32 pick;

    ResortSys_GetInfo(sys);
    index = ResortData_GetIndex(occupant);

    count = 0;
    for (i = 0; sPeopleRows[index].near[i] != -1; i++) {
        u8 id = sPeopleRows[index].near[i];

        if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
            count++;
        }
    }
    if (count > 0) {
        u32 near;

        pick = ResortData_RollPair(occupant, data, 23, count);
        near = 0;

        for (i = 0; sPeopleRows[index].near[i] != -1; i++) {
            u8 id = sPeopleRows[index].near[i];

            if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
                if (near == pick) {
                    *out = id;
                    return TRUE;
                }
                near++;
            }
        }
    }

    count = 0;
    for (i = 0; sPeopleRows[index].far[i] != -1; i++) {
        u8 id = sPeopleRows[index].far[i];

        if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
            count++;
        }
    }
    if (count > 0) {
        u32 near;

        pick = ResortData_RollPair(occupant, data, 24, count);
        near = 0;

        for (i = 0; sPeopleRows[index].far[i] != -1; i++) {
            u8 id = sPeopleRows[index].far[i];

            if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
                if (near == pick) {
                    *out = id;
                    return TRUE;
                }
                near++;
            }
        }
    }
    return FALSE;
}

// The points that a visit of a person brings: those of their trainer type and shop kind for the day of the week
u32 ResortSys_GetVisitPoints(ResortSys *sys, ResortPersonData *data) {
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u32 base;
    RTCDate date;
    u16 shopId;
    u32 shopKind;
    u8 trainerType;
    ResortBinary *table;
    u32 typePoints;
    u32 shopPoints;

    ResortSys_GetWork(sys);
    base = JoinAvenue_GetParam(info, JOIN_AVE_INFO_SEASON, NULL);
    RTC_GetCachedDate(&date);
    shopId = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(ResortData_GetPerson(data)), 0);
    shopKind = ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shopId), 0);
    trainerType = UnionView_FindTrainerType((u8)ResortData_GetParam(data, JOIN_AVE_PARAM_TRAINER_TYPE, NULL));
    if (trainerType >= 8) {
        trainerType -= 8;
    }

    table = ResortBinary_Load(10, 7, HEAPID_GAMEEVENT);
    typePoints = ResortBinary_Get(table, trainerType, date.week);
    ResortBinary_Free(table);
    table = ResortBinary_Load(11, 7, HEAPID_GAMEEVENT);
    shopPoints = ResortBinary_Get(table, base + shopKind * 4, date.week);
    ResortBinary_Free(table);
    return typePoints + shopPoints;
}

// The points of a visit, plus those of the avenue's popularity, from 0 to 100
u32 ResortSys_GetVisitScore(ResortSys *sys, ResortPersonData *data) {
    ResortWork *work = ResortSys_GetWork(sys);
    s16 visit = ResortSys_GetVisitPoints(sys, data);
    u32 day = ResortWork_Get(work, RESORT_WORK_VISITS);
    u32 group = ResortWork_Get(work, RESORT_WORK_GROUP);
    ResortBinary *table = ResortBinary_Load(12, 8, HEAPID_GAMEEVENT);
    s16 bonus = ResortBinary_Get(table, group, day + 1);
    s32 points;

    ResortBinary_Free(table);
    points = visit + bonus;
    if (points > 100) {
        return 100;
    }
    if (points < 0) {
        return 0;
    }
    return points;
}

void ResortSys_MarkMet(ResortSys *sys, ResortPersonData *occupant, u32 index) {
    ResortWork *work = ResortSys_GetWork(sys);

    ResortWork_Set(work, index + RESORT_WORK_MET_FIRST, 1);
    ResortWork_Set(work, RESORT_WORK_VISITS, ResortWork_Get(work, RESORT_WORK_VISITS) + 1);
}

BOOL ResortSys_HasShopKind(ResortSys *sys, GameData *gameData, u32 shopKind) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    s32 trainerIdMod;
    u32 rank;
    u32 i;

    if (shopKind == 5) {
        return JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_6, NULL);
    }

    trainerIdMod = getTrainerID(GetGameDataPlayerInfo(gameData)) % 7;
    rank = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL);
    if (rank >= 13) {
        return 1;
    }
    for (i = 0; i <= rank; i++) {
        if (shopKind == sShopUnlockOrderByTrainerId[trainerIdMod][sShopUnlockIndexByRank[i]]) {
            return 1;
        }
    }
    return 0;
}

// Whether all eight of the avenue's people are there
BOOL ResortSys_IsFull(ResortSys *sys) {
    return JoinAvenueOccupants_CountPeople(ResortSys_GetOccupants(sys)) == RESORT_PERSON_COUNT;
}
