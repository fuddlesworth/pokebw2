#include "types.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/pms.h"
#include "system/vm.h"

// The script command that lets the player pick a phrase (a descriptive name). Scrcmd_PhraseSelect_CallPhraseSelect is
// swan's name

// CallPhraseSelect's mode: the C-Gear phrase it starts from and saves to (the C-Gear's phrases[1] to phrases[4]), or
// the one or two words the player types. gear_station.s picks the C-Gear phrase from the menu whose text labels them
// INTRODUCTION, WHEN YOU WIN, WHEN YOU LOSE and WHEN YOU BECOME NO. 1; icirrus_city_4.s passes PHRASE_SELECT_WORD.
enum {
    PHRASE_SELECT_CGEAR_INTRODUCTION,
    PHRASE_SELECT_CGEAR_WHEN_WIN,
    PHRASE_SELECT_CGEAR_WHEN_LOSE,
    PHRASE_SELECT_CGEAR_WHEN_NO_1,
    PHRASE_SELECT_WORD,
    PHRASE_SELECT_DOUBLE_WORD,
};

// EventPhraseSelect_Callback's steps: the input screen, and reading what it returns
enum {
    PHRASE_SELECT_STEP_INPUT,
    PHRASE_SELECT_STEP_READ_RESULT,
};

typedef struct {
    u32 mode;
    u16 *result;
    u16 *result2;
    u16 *decided;
    PMSIParam *pms;
    // Which of the C-Gear's phrases the select starts from and keeps, when it does
    u32 cgearIndex;
    BOOL useCGear;
} PhraseSelectEvent;

static GameEventReturnCode EventPhraseSelect_Callback(GameEvent *event, u32 *state, void *data);

BOOL Scrcmd_PhraseSelect_CallPhraseSelect(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 mode = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u16 *result2 = ScriptReadVar(vm, env);
    u16 *decided = ScriptReadVar(vm, env);
    u32 kind = PMSI_MODE_WORD;
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPhraseSelect_Callback, sizeof(PhraseSelectEvent));
    PhraseSelectEvent *data = GameEvent_GetData(event);
    PMSData sentence;

    MI_CpuFill8(data, 0, sizeof(PhraseSelectEvent));
    data->mode = mode;
    data->result = result;
    data->result2 = result2;
    data->decided = decided;
    *decided = FALSE;
    switch (mode) {
    case PHRASE_SELECT_CGEAR_INTRODUCTION:
        data->cgearIndex = 1;
        data->useCGear = TRUE;
        kind = PMSI_MODE_SENTENCE;
        break;
    case PHRASE_SELECT_CGEAR_WHEN_WIN:
        data->cgearIndex = 2;
        data->useCGear = TRUE;
        kind = PMSI_MODE_SENTENCE;
        break;
    case PHRASE_SELECT_CGEAR_WHEN_LOSE:
        data->cgearIndex = 3;
        data->useCGear = TRUE;
        kind = PMSI_MODE_SENTENCE;
        break;
    case PHRASE_SELECT_CGEAR_WHEN_NO_1:
        data->cgearIndex = 4;
        data->useCGear = TRUE;
        kind = PMSI_MODE_SENTENCE;
        break;
    case PHRASE_SELECT_WORD:
        data->useCGear = FALSE;
        break;
    case PHRASE_SELECT_DOUBLE_WORD:
        data->useCGear = FALSE;
        kind = PMSI_MODE_DOUBLE_WORD;
        break;
    default:
        data->pms = NULL;
        ScriptWork_CallEvent(work, event);
        return TRUE;
    }
    data->pms = PMSIParam_Create(kind, 0, 0, 0, GameData_GetSaveControl(gameData), HEAPID_USER);
    if (data->useCGear) {
        CGearPhrases_Get(getCGearDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), data->cgearIndex,
                         &sentence);
        PMSIParam_SetSentence(data->pms, &sentence);
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode EventPhraseSelect_Callback(GameEvent *event, u32 *state, void *data_) {
    PhraseSelectEvent *data = GameEvent_GetData(event);
    PMSIParam *pms = data->pms;
    u16 words[2];
    PMSData sentence;

    switch (*state) {
    case PHRASE_SELECT_STEP_INPUT:
        if (pms == NULL) {
            return GAMEEVENT_DONE;
        }
        GSYS_QueueProcAsEvent(event, OVERLAY_ID(185), &PMS_INPUT_PROC_FUNCTIONS, pms);
        (*state)++;
        break;
    case PHRASE_SELECT_STEP_READ_RESULT:
        if (PMSIParam_IsCanceled(pms)) {
            *data->decided = FALSE;
        } else if (data->mode == PHRASE_SELECT_DOUBLE_WORD) {
            PMSIParam_GetWords(pms, words);
            *data->result = words[0];
            *data->result2 = words[1];
            *data->decided = TRUE;
        } else if (data->mode == PHRASE_SELECT_WORD) {
            *data->result = PMSIParam_GetWord(pms);
            *data->result2 = 0;
            *data->decided = TRUE;
        } else {
            PMSIParam_GetSentence(pms, &sentence);
            if (data->useCGear) {
                CGearPhrases_Set(
                    getCGearDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(GameEvent_GetGameSystem(event)))),
                    data->cgearIndex, &sentence);
            }
            *data->result = sentence.words[0];
            *data->result2 = sentence.words[1];
            *data->decided = TRUE;
        }
        PMSIParam_Free(data->pms);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
