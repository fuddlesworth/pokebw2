#include "app/local_tvt/local_tvt_person_doctor.h"
#include "types.h"
#include "app/local_tvt/local_tvt_calldata.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/local_tvt/local_tvt_person.h"
#include "app/local_tvt/local_tvt_util.h"
#include "constants/arc.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The steps of LocalTvtDoctor_Pokedex
enum {
    LOCAL_TVT_DOCTOR_POKEDEX_COUNT,
    LOCAL_TVT_DOCTOR_POKEDEX_WAIT_COUNT,
    LOCAL_TVT_DOCTOR_POKEDEX_JINGLE,
    LOCAL_TVT_DOCTOR_POKEDEX_WAIT_JINGLE,
    LOCAL_TVT_DOCTOR_POKEDEX_FINISH_COUNT,
    LOCAL_TVT_DOCTOR_POKEDEX_EVALUATE,
    LOCAL_TVT_DOCTOR_POKEDEX_WAIT_EVALUATION,
    LOCAL_TVT_DOCTOR_POKEDEX_AWARD,
    LOCAL_TVT_DOCTOR_POKEDEX_WAIT_AWARD,
    LOCAL_TVT_DOCTOR_POKEDEX_FINISH,
};

// The party selection and explanation of a selected Pokemon
enum {
    LOCAL_TVT_DOCTOR_EXPLAIN_PROMPT,
    LOCAL_TVT_DOCTOR_EXPLAIN_WAIT_PROMPT,
    LOCAL_TVT_DOCTOR_EXPLAIN_DELAY,
    LOCAL_TVT_DOCTOR_EXPLAIN_OPEN_PARTY,
    LOCAL_TVT_DOCTOR_EXPLAIN_SELECT,
    LOCAL_TVT_DOCTOR_EXPLAIN_CANCEL,
    LOCAL_TVT_DOCTOR_EXPLAIN_EXPLAIN,
    LOCAL_TVT_DOCTOR_EXPLAIN_WAIT_EXPLANATION,
};

// A message starts, waits for its face, then finishes
enum {
    LOCAL_TVT_DOCTOR_TALK_START,
    LOCAL_TVT_DOCTOR_TALK_WAIT,
    LOCAL_TVT_DOCTOR_TALK_FINISH,
};

// What the Professor says in a call on the Xtransceiver, named after the ROM's "local_tvt_person_doctor.c": how a
// Pokémon of the party evolves, and how the Pokédex is coming along

struct LocalTvtDoctor {
    int step;
    u32 timer;
    LocalTvtPokeSelect *select;
    GameData *gameData;
    MsgData *evolutionMsgData;
    MsgData *pokedexMsgData;
    ArcTool *evolutionFile;
    u16 heapId;
    // What the evaluation of the Pokédex says after the count, the count and the jingle
    u16 pokedexMsg;
    u16 pokedexCount;
    u16 pokedexJingle;
    // Whether the Pokédex has earned an award the player hasn't collected
    BOOL award;
};

// The message for a species, item or move
struct LocalTvtEvolutionMsg {
    u16 key;
    u16 messageId;
};

static LocalTvtDoctor *LocalTvtDoctor_Create(GameData *gameData, HeapID heapId);
static void LocalTvtDoctor_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtDoctor_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtDoctor_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtDoctor_Evolution(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtDoctor_Pokedex(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtDoctor_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static u16 LocalTvtDoctor_GetEvolutionMsg(ArcTool *evolutionFile, u16 species);
static BOOL LocalTvtDoctor_FindEvolutionMsg(const LocalTvtEvolutionMsg *table, u32 count, u32 key, u16 *messageId);
static void LocalTvtDoctor_EvaluatePokedex(PokeDexSave *pokedex, EventWork *eventWork, HeapID heapId, u16 *startMsg,
                                           u16 *messageId, u16 *count, u16 *jingle, BOOL *award);

static const LocalTvtEvolutionMsg sLocalTvtLevelItemMsgs[] = {
    { ITEM_RAZOR_FANG, 40 },
    { ITEM_RAZOR_CLAW, 41 },
};

static const LocalTvtEvolutionMsg sLocalTvtLevelMoveMsgs[] = {
    { MOVE_MIMIC, 50 },
    { MOVE_DOUBLE_HIT, 51 },
    { MOVE_ROLLOUT, 52 },
    { MOVE_ANCIENT_POWER, 53 },
};

static const LocalTvtEvolutionMsg sLocalTvtItemMsgs[] = {
    { ITEM_THUNDERSTONE, 42 }, { ITEM_SUN_STONE, 43 },  { ITEM_MOON_STONE, 44 },  { ITEM_FIRE_STONE, 45 },
    { ITEM_WATER_STONE, 46 },  { ITEM_LEAF_STONE, 47 }, { ITEM_SHINY_STONE, 48 }, { ITEM_DUSK_STONE, 49 },
};

// clang-format off
static const LocalTvtEvolutionMsg sLocalTvtTradeItemMsgs[] = {
    { ITEM_UP_GRADE, 29 },     { ITEM_KINGS_ROCK, 30 },   { ITEM_DEEP_SEA_TOOTH, 31 }, { ITEM_DEEP_SEA_SCALE, 32 },
    { ITEM_METAL_COAT, 33 },   { ITEM_DRAGON_SCALE, 34 }, { ITEM_PROTECTOR, 35 },      { ITEM_ELECTIRIZER, 36 },
    { ITEM_MAGMARIZER, 37 },   { ITEM_DUBIOUS_DISC, 38 }, { ITEM_REAPER_CLOTH, 39 },
};

// clang-format on

// The species that evolve in more than one way
static const LocalTvtEvolutionMsg sLocalTvtSpeciesMsgs[] = {
    { SPECIES_GLOOM, 54 },   { SPECIES_POLIWHIRL, 55 }, { SPECIES_SLOWPOKE, 56 }, { SPECIES_EEVEE, 57 },
    { SPECIES_TYROGUE, 58 }, { SPECIES_WURMPLE, 59 },   { SPECIES_KIRLIA, 60 },   { SPECIES_NINCADA, 61 },
    { SPECIES_SNORUNT, 62 }, { SPECIES_CLAMPERL, 63 },  { SPECIES_BURMY, 64 },
};

void LocalTvtDoctor_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId) {
    MI_CpuFill8(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtDoctor_Create(gameData, heapId);
    funcs->msgData = msgData;
    funcs->free = LocalTvtDoctor_Free;
    funcs->greet = LocalTvtDoctor_Greet;
    funcs->topics[0] = LocalTvtDoctor_Chat;
    funcs->topicNames[0] = 9;
    funcs->topics[1] = LocalTvtDoctor_Evolution;
    funcs->topicNames[1] = 10;
    funcs->topics[2] = LocalTvtDoctor_Pokedex;
    funcs->topicNames[2] = 11;
    funcs->topics[3] = LocalTvtDoctor_Bye;
    funcs->topicNames[3] = 12;
    funcs->topicCount = 4;
    funcs->menuMsg = 14;
}

static LocalTvtDoctor *LocalTvtDoctor_Create(GameData *gameData, HeapID heapId) {
    LocalTvtDoctor *doctor = GFL_HeapAllocate(heapId, sizeof(LocalTvtDoctor), TRUE, "local_tvt_person_doctor.c", 344);

    doctor->heapId = heapId;
    doctor->gameData = gameData;
    doctor->evolutionMsgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_LOCAL_TVT_PERSON_DOCTOR, heapId);
    doctor->pokedexMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10380, heapId);
    doctor->evolutionFile = loadEvolutionFile(heapId);
    return doctor;
}

static void LocalTvtDoctor_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtDoctor *doctor = work;

    GFL_ArcToolFree(doctor->evolutionFile);
    GFL_MsgDataFree(doctor->pokedexMsgData);
    GFL_MsgDataFree(doctor->evolutionMsgData);
    GFL_HeapFree(doctor);
}

static BOOL LocalTvtDoctor_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtDoctor *doctor = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (doctor->step) {
    case LOCAL_TVT_DOCTOR_TALK_START: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_DOCTOR));
        doctor->step++;
        break;
    }
    case LOCAL_TVT_DOCTOR_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_TALK_FINISH:
        doctor->step = LOCAL_TVT_DOCTOR_TALK_START;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtDoctor_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtDoctor *doctor = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (doctor->step) {
    case LOCAL_TVT_DOCTOR_TALK_START: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_DOCTOR));
        doctor->step++;
        break;
    }
    case LOCAL_TVT_DOCTOR_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_TALK_FINISH:
        doctor->step = LOCAL_TVT_DOCTOR_TALK_START;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtDoctor_Evolution(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtDoctor *doctor = work;
    u16 messageId = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (doctor->step) {
    case LOCAL_TVT_DOCTOR_EXPLAIN_PROMPT:
        LocalTvtFace_Talk(face, LocalTvtMessage_GetTalkMsgData(setup->message), 13);
        doctor->step++;
        break;
    case LOCAL_TVT_DOCTOR_EXPLAIN_WAIT_PROMPT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->timer = 0;
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_EXPLAIN_DELAY:
        if (doctor->timer++ > 15) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_EXPLAIN_OPEN_PARTY:
        if (doctor->select != NULL) {
            LocalTvtPokeSelect_SetActive(doctor->select, TRUE);
            LocalTvtPokeSelect_Restart(doctor->select);
        } else {
            LocalTvtPokeSelectSetup selectSetup;

            MI_CpuFill8(&selectSetup, 0, sizeof(selectSetup));
            selectSetup.message = setup->message;
            selectSetup.unit = setup->unit;
            selectSetup.party = GameData_GetParty(doctor->gameData);
            doctor->select = LocalTvtPokeSelect_Create(&selectSetup, doctor->heapId);
        }
        doctor->step++;
        break;
    case LOCAL_TVT_DOCTOR_EXPLAIN_SELECT:
        LocalTvtPokeSelect_Main(doctor->select);
        switch (LocalTvtPokeSelect_GetResult(doctor->select)) {
        case LOCAL_TVT_POKE_SELECT_DECIDE:
            LocalTvtPokeSelect_SetActive(doctor->select, FALSE);
            doctor->step = LOCAL_TVT_DOCTOR_EXPLAIN_EXPLAIN;
            break;
        case LOCAL_TVT_POKE_SELECT_CANCEL:
            doctor->step = LOCAL_TVT_DOCTOR_EXPLAIN_CANCEL;
            break;
        }
        break;
    case LOCAL_TVT_DOCTOR_EXPLAIN_CANCEL:
        LocalTvtPokeSelect_Free(doctor->select);
        doctor->select = NULL;
        doctor->step = LOCAL_TVT_DOCTOR_EXPLAIN_PROMPT;
        return TRUE;
    case LOCAL_TVT_DOCTOR_EXPLAIN_EXPLAIN: {
        PartyPkm *pkm = LocalTvtPokeSelect_GetPkm(doctor->select);

        messageId =
            LocalTvtDoctor_GetEvolutionMsg(doctor->evolutionFile, PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL));
        LocalTvtMessage_SetPlayerName(setup->message, doctor->gameData);
        LocalTvtMessage_SetPokemonNickname(setup->message, pkm, 1);
        LocalTvtFace_Talk(face, doctor->evolutionMsgData, messageId);
        doctor->step++;
        break;
    }
    case LOCAL_TVT_DOCTOR_EXPLAIN_WAIT_EXPLANATION:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->step = LOCAL_TVT_DOCTOR_EXPLAIN_PROMPT;
        }
        break;
    }
    return FALSE;
}

static BOOL LocalTvtDoctor_Pokedex(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtDoctor *doctor = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (doctor->step) {
    case LOCAL_TVT_DOCTOR_POKEDEX_COUNT: {
        PokeDexSave *pokedex = GameData_GetPokedex(doctor->gameData);
        EventWork *eventWork = getConstDataBlock(GameData_GetSaveControl(doctor->gameData));
        u16 startMsg;

        LocalTvtDoctor_EvaluatePokedex(pokedex, eventWork, doctor->heapId, &startMsg, &doctor->pokedexMsg,
                                       &doctor->pokedexCount, &doctor->pokedexJingle, &doctor->award);
        LocalTvtMessage_SetNumber(setup->message, doctor->pokedexCount, 3, 0);
        LocalTvtFace_TalkKeyWait(face, doctor->pokedexMsgData, startMsg);
        doctor->step++;
        break;
    }
    case LOCAL_TVT_DOCTOR_POKEDEX_WAIT_COUNT:
        LocalTvtFace_UpdateTalk(face, setup->chara);
        if (LocalTvtMessage_IsTalkDone(setup->message)) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_JINGLE:
        GFL_SndBGMPush();
        GFL_SndBGMPlay(doctor->pokedexJingle, 0xffff);
        doctor->step++;
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_WAIT_JINGLE:
        if (!GFL_SndBGMIsPlaying()) {
            LocalTvtMessage_AllowTalkEnd(setup->message);
            GFL_SndBGMPop();
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_FINISH_COUNT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_EVALUATE:
        LocalTvtFace_Talk(face, doctor->pokedexMsgData, doctor->pokedexMsg);
        doctor->step++;
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_WAIT_EVALUATION:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            if (doctor->award) {
                doctor->step++;
            } else {
                doctor->step = LOCAL_TVT_DOCTOR_POKEDEX_FINISH;
            }
        }
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_AWARD:
        LocalTvtFace_TalkKeyWait(face, doctor->pokedexMsgData, 34);
        doctor->step++;
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_WAIT_AWARD:
        if (LocalTvtMessage_IsTalkDone(setup->message)) {
            LocalTvtMessage_AllowTalkEnd(setup->message);
        }
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_POKEDEX_FINISH:
        doctor->step = LOCAL_TVT_DOCTOR_POKEDEX_COUNT;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtDoctor_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtDoctor *doctor = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (doctor->step) {
    case LOCAL_TVT_DOCTOR_TALK_START: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_DOCTOR));
        doctor->step++;
        break;
    }
    case LOCAL_TVT_DOCTOR_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            doctor->step++;
        }
        break;
    case LOCAL_TVT_DOCTOR_TALK_FINISH:
        doctor->step = LOCAL_TVT_DOCTOR_TALK_START;
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}

// The message of how a species evolves: one of its own, one for what its way to evolve needs, or the message of
// the way itself
static u16 LocalTvtDoctor_GetEvolutionMsg(ArcTool *evolutionFile, u16 species) {
    u16 messageId = 0;
    int method;
    int param;

    if (LocalTvtDoctor_FindEvolutionMsg(sLocalTvtSpeciesMsgs, NELEMS(sLocalTvtSpeciesMsgs), species, &messageId)) {
        return messageId;
    }
    method = getEvolutionInfoParam(evolutionFile, species, 0, 0);
    param = getEvolutionInfoParam(evolutionFile, species, 0, 1);
    switch (method) {
    case EVO_METHOD_TRADE_WITH_ITEM:
        LocalTvtDoctor_FindEvolutionMsg(sLocalTvtTradeItemMsgs, NELEMS(sLocalTvtTradeItemMsgs), param, &messageId);
        break;
    case EVO_METHOD_ITEM:
        LocalTvtDoctor_FindEvolutionMsg(sLocalTvtItemMsgs, NELEMS(sLocalTvtItemMsgs), param, &messageId);
        break;
    case EVO_METHOD_HELD_ITEM_NIGHT:
        LocalTvtDoctor_FindEvolutionMsg(sLocalTvtLevelItemMsgs, NELEMS(sLocalTvtLevelItemMsgs), param, &messageId);
        break;
    case EVO_METHOD_KNOWS_MOVE:
        LocalTvtDoctor_FindEvolutionMsg(sLocalTvtLevelMoveMsgs, NELEMS(sLocalTvtLevelMoveMsgs), param, &messageId);
        break;
    default:
        messageId = method;
        break;
    }
    return messageId;
}

static BOOL LocalTvtDoctor_FindEvolutionMsg(const LocalTvtEvolutionMsg *table, u32 count, u32 key, u16 *messageId) {
    u32 i;

    for (i = 0; i < count; i++) {
        const LocalTvtEvolutionMsg *entry = &table[i];

        if (entry->key == key) {
            *messageId = entry->messageId;
            return TRUE;
        }
    }
    *messageId = 0;
    return FALSE;
}

static void LocalTvtDoctor_EvaluatePokedex(PokeDexSave *pokedex, EventWork *eventWork, HeapID heapId, u16 *startMsg,
                                           u16 *messageId, u16 *count, u16 *jingle, BOOL *award) {
    u16 evalMsg;
    u16 evalCount;
    u16 evalJingle;

    *award = FALSE;
    // The national Pokédex once the regional one has seen everything
    if (EventWork_FlagGet(eventWork, EVENT_FLAG_0x008d)) {
        *startMsg = 2;
        PokeDex_GetEvaluationParams(pokedex, 3, heapId, &evalMsg, &evalCount, &evalJingle);
    } else {
        *startMsg = 1;
        PokeDex_GetEvaluationParams(pokedex, 2, heapId, &evalMsg, &evalCount, &evalJingle);
        if (PokeDex_IsSeenAllUnova(pokedex, heapId)) {
            EventWork_FlagSet(eventWork, EVENT_FLAG_0x008d);
        }
    }
    if (PokeDex_IsSeenAllUnova(pokedex, heapId) && !EventWork_FlagGet(eventWork, EVENT_FLAG_0x0088)) {
        *award = TRUE;
    }
    if (PokeDex_IsCompleteUnova(pokedex, heapId) && !EventWork_FlagGet(eventWork, EVENT_FLAG_0x0089)) {
        *award = TRUE;
    }
    if (PokeDex_IsCompleteNational(pokedex) && !EventWork_FlagGet(eventWork, EVENT_FLAG_0x008a)) {
        *award = TRUE;
    }
    if (messageId != NULL) {
        *messageId = evalMsg;
    }
    if (count != NULL) {
        *count = evalCount;
    }
    if (jingle != NULL) {
        *jingle = evalJingle;
    }
}
