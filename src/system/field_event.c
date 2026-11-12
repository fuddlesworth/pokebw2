#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "field/event_game_clear.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "gfl/heap.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The events that close and open the field around another screen: closing it, opening it again, running a proc as a
// field subprocess with or without the map's fade, and the party list with its summary screen. The name is a guess,
// since the ROM has no string of this file.

typedef struct {
    GameSystem *gsys;
    Field *field;
    GameData *gameData;
    // Whether the field's glue overlay is unloaded once the field is closed
    BOOL unloadGlue;
} EventFieldCloseWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    GameData *gameData;
    u32 unk0C;
} EventFieldOpenWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    s32 overlayId;
    const GameProcFunctions *functions;
    void *param;
} EventFieldSubprocessWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    s32 overlayId;
    const GameProcFunctions *functions;
    void *param;
    void (*callback)(void *work);
    void *work;
} EventFieldSubprocessCallbackWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    PokeListParam *param;
    PStatusParam *summaryParam;
} EventPokeListWork;

static GameEventReturnCode EventFieldClose_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldCloseKeepSound_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldOpen_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessTransition_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessCall_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessTransitionWithCallback_Callback(GameEvent *event, u32 *state,
                                                                               void *data);
static GameEventReturnCode EventFieldSubprocessCallWithCallback_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPokeList_Callback(GameEvent *event, u32 *state, void *data);

// The field-closing event's steps: ask the field to close, then wait for its procs and its sound
enum {
    EVENT_FIELD_CLOSE_SEQ_REQUEST_CLOSE,
    EVENT_FIELD_CLOSE_SEQ_WAIT_CLOSED,
};

static GameEventReturnCode EventFieldClose_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldCloseWork *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(wk->gameData);

    switch (*state) {
    case EVENT_FIELD_CLOSE_SEQ_REQUEST_CLOSE:
        Field_RequestClose(field);
        (*state)++;
        break;
    case EVENT_FIELD_CLOSE_SEQ_WAIT_CLOSED:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        if (FieldSnd_IsBusy(fieldSound) == TRUE) {
            break;
        }
        if (wk->unloadGlue) {
            UnloadFieldGlueOverlay();
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *CreateFieldCloseEvent(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldClose_Callback, sizeof(EventFieldCloseWork));
    EventFieldCloseWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->unloadGlue = TRUE;
    return event;
}

// The same without the sound: ask the field to close, then wait for its procs
enum {
    EVENT_FIELD_CLOSE_KEEP_SOUND_SEQ_REQUEST_CLOSE,
    EVENT_FIELD_CLOSE_KEEP_SOUND_SEQ_WAIT_CLOSED,
};

// The same without waiting for the field's sound
static GameEventReturnCode EventFieldCloseKeepSound_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldCloseWork *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(wk->gameData);

    switch (*state) {
    case EVENT_FIELD_CLOSE_KEEP_SOUND_SEQ_REQUEST_CLOSE:
        Field_RequestClose(field);
        (*state)++;
        break;
    case EVENT_FIELD_CLOSE_KEEP_SOUND_SEQ_WAIT_CLOSED:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        if (wk->unloadGlue) {
            UnloadFieldGlueOverlay();
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Closes the field without waiting for its sound, and leaves the field's glue overlay loaded
GameEvent *EventFieldCloseKeepSound_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldCloseKeepSound_Callback, sizeof(EventFieldCloseWork));
    EventFieldCloseWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->unloadGlue = FALSE;
    return event;
}

// The field-opening event's steps: wait for the last proc, start the field, then wait for the map
enum {
    EVENT_FIELD_OPEN_SEQ_START_FIELD,
    EVENT_FIELD_OPEN_SEQ_WAIT_MAP_LOAD,
    EVENT_FIELD_OPEN_SEQ_DONE,
};

static GameEventReturnCode EventFieldOpen_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldOpenWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case EVENT_FIELD_OPEN_SEQ_START_FIELD:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        GSYS_StartFieldmapProc(gsys);
        (*state)++;
        break;
    case EVENT_FIELD_OPEN_SEQ_WAIT_MAP_LOAD:
        if (!Field_CheckMapLoadFinished(GSYS_GetField(gsys))) {
            break;
        }
        (*state)++;
        break;
    case EVENT_FIELD_OPEN_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Opens the field after CreateFieldCloseEvent, loading the field's glue overlay again
GameEvent *EventFieldOpen_CreateHeadless(GameSystem *gsys) {
    Field *field = NULL;
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldOpen_Callback, sizeof(EventFieldOpenWork));
    EventFieldOpenWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    LoadFieldGlueOverlay();
    return event;
}

// Opens the field after EventFieldCloseKeepSound_Create
GameEvent *EventFieldOpen_Create(GameSystem *gsys) {
    Field *field = NULL;
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldOpen_Callback, sizeof(EventFieldOpenWork));
    EventFieldOpenWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    return event;
}

// The fading subprocess's steps: fade the map out, run the proc, then fade the map in
enum {
    EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_FADE_OUT,
    EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_CALL_SUBPROCESS,
    EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_FADE_IN,
    EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_DONE,
};

static GameEventReturnCode EventFieldSubprocessTransition_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_FADE_OUT:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_CALL_SUBPROCESS:
        GameEvent_ChainNext(event,
                            EventFieldSubprocessCall_Create(gsys, wk->field, wk->overlayId, wk->functions, wk->param));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_FADE_IN:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_TRANSITION_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The subprocess's steps: close the field, run the proc, then open the field again
enum {
    EVENT_FIELD_SUBPROCESS_CALL_SEQ_CLOSE_FIELD,
    EVENT_FIELD_SUBPROCESS_CALL_SEQ_QUEUE_PROC,
    EVENT_FIELD_SUBPROCESS_CALL_SEQ_WAIT_PROC,
    EVENT_FIELD_SUBPROCESS_CALL_SEQ_OPEN_FIELD,
    EVENT_FIELD_SUBPROCESS_CALL_SEQ_DONE,
};

static GameEventReturnCode EventFieldSubprocessCall_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case EVENT_FIELD_SUBPROCESS_CALL_SEQ_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, wk->field));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_CALL_SEQ_QUEUE_PROC:
        GSYS_QueueProc(gsys, wk->overlayId, wk->functions, wk->param);
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_CALL_SEQ_WAIT_PROC:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_CALL_SEQ_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_CALL_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Runs the proc as a field subprocess, fading the map out before and in after
GameEvent *EventFieldSubprocessTransition_Create(GameSystem *gsys, Field *field, s32 overlayId,
                                                 const GameProcFunctions *functions, void *param) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventFieldSubprocessTransition_Callback, sizeof(EventFieldSubprocessWork));
    EventFieldSubprocessWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    return event;
}

GameEvent *EventFieldSubprocessCall_Create(GameSystem *gsys, Field *field, s32 overlayId,
                                           const GameProcFunctions *functions, void *param) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventFieldSubprocessCall_Callback, sizeof(EventFieldSubprocessWork));
    EventFieldSubprocessWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    return event;
}

// The fading subprocess's steps with a callback: run the transition, call it, then free its work
enum {
    EVENT_FIELD_SUBPROCESS_TRANSITION_CALLBACK_SEQ_RUN_TRANSITION,
    EVENT_FIELD_SUBPROCESS_TRANSITION_CALLBACK_SEQ_CALLBACK,
    EVENT_FIELD_SUBPROCESS_TRANSITION_CALLBACK_SEQ_FREE_WORK,
};

static GameEventReturnCode EventFieldSubprocessTransitionWithCallback_Callback(GameEvent *event, u32 *state,
                                                                               void *data) {
    EventFieldSubprocessCallbackWork *wk = data;

    switch (*state) {
    case EVENT_FIELD_SUBPROCESS_TRANSITION_CALLBACK_SEQ_RUN_TRANSITION:
        GameEvent_ChainNext(
            event, EventFieldSubprocessTransition_Create(wk->gsys, wk->field, wk->overlayId, wk->functions, wk->param));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_TRANSITION_CALLBACK_SEQ_CALLBACK:
        if (wk->callback != NULL) {
            wk->callback(wk->work);
        }
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_TRANSITION_CALLBACK_SEQ_FREE_WORK:
        if (wk->work != NULL) {
            GFL_HeapFree(wk->work);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The subprocess's steps with a callback: run the subprocess, call it, then free its work
enum {
    EVENT_FIELD_SUBPROCESS_CALL_CALLBACK_SEQ_RUN_SUBPROCESS,
    EVENT_FIELD_SUBPROCESS_CALL_CALLBACK_SEQ_CALLBACK,
    EVENT_FIELD_SUBPROCESS_CALL_CALLBACK_SEQ_FREE_WORK,
};

static GameEventReturnCode EventFieldSubprocessCallWithCallback_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessCallbackWork *wk = data;

    switch (*state) {
    case EVENT_FIELD_SUBPROCESS_CALL_CALLBACK_SEQ_RUN_SUBPROCESS:
        GameEvent_ChainNext(
            event, EventFieldSubprocessCall_Create(wk->gsys, wk->field, wk->overlayId, wk->functions, wk->param));
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_CALL_CALLBACK_SEQ_CALLBACK:
        if (wk->callback != NULL) {
            wk->callback(wk->work);
        }
        (*state)++;
        break;
    case EVENT_FIELD_SUBPROCESS_CALL_CALLBACK_SEQ_FREE_WORK:
        if (wk->work != NULL) {
            GFL_HeapFree(wk->work);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFieldSubprocessTransition_CreateWithCallback(GameSystem *gsys, Field *field, u32 overlayId,
                                                             const GameProcFunctions *functions, void *param,
                                                             void (*callback)(void *work), void *work) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldSubprocessTransitionWithCallback_Callback,
                                        sizeof(EventFieldSubprocessCallbackWork));
    EventFieldSubprocessCallbackWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    wk->callback = callback;
    wk->work = work;
    return event;
}

GameEvent *EventFieldSubprocessCall_CreateWithCallback(GameSystem *gsys, Field *field, s32 overlayId,
                                                       const GameProcFunctions *functions, void *param,
                                                       void (*callback)(void *work), void *work) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldSubprocessCallWithCallback_Callback,
                                        sizeof(EventFieldSubprocessCallbackWork));
    EventFieldSubprocessCallbackWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    wk->callback = callback;
    wk->work = work;
    return event;
}

// The party list's steps: open the list, then the summary screen of the Pokemon picked, until the list is done
enum {
    EVENT_POKE_LIST_SEQ_OPEN_LIST,
    EVENT_POKE_LIST_SEQ_WAIT_LIST,
    EVENT_POKE_LIST_SEQ_CHECK_RESULT,
    EVENT_POKE_LIST_SEQ_OPEN_SUMMARY,
    EVENT_POKE_LIST_SEQ_WAIT_SUMMARY,
    EVENT_POKE_LIST_SEQ_DONE,
};

// Switches between the party list and the summary screen until the list is done. The field is closed already.
static GameEventReturnCode EventPokeList_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeListWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case EVENT_POKE_LIST_SEQ_OPEN_LIST:
        GSYS_QueueProc(gsys, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, wk->param);
        *state = 1;
        break;
    case EVENT_POKE_LIST_SEQ_WAIT_LIST:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        *state = 2;
        break;
    case EVENT_POKE_LIST_SEQ_CHECK_RESULT:
        if (wk->param->result == 0) {
            *state = 5;
        } else if (wk->param->result == 1) {
            wk->summaryParam->partyIndex = wk->param->index;
            *state = 3;
        } else {
            *state = 5;
        }
        break;
    case EVENT_POKE_LIST_SEQ_OPEN_SUMMARY:
        GSYS_QueueProc(gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, wk->summaryParam);
        *state = 4;
        break;
    case EVENT_POKE_LIST_SEQ_WAIT_SUMMARY:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        wk->param->index = wk->summaryParam->partyIndex;
        *state = 0;
        break;
    case EVENT_POKE_LIST_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventPokeList_Create(GameSystem *gsys, Field *field, PokeListParam *param, PStatusParam *summaryParam) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPokeList_Callback, sizeof(EventPokeListWork));
    EventPokeListWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->param = param;
    wk->summaryParam = summaryParam;
    return event;
}
