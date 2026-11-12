// Events that send requests to the field's sound system. The file's name is descriptive, a guess: the ROM has no
// string for it. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except EventSoundWork,
// EventWaitFieldSound_Create and EventWaitFieldSound_Callback

#include "types.h"
#include "constants/sound.h"
#include "field/event_sound.h"
#include "field/field_sound.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The work of every event in this file; each uses only some of it
typedef struct {
    GameSystem *gsys;
    FieldSound *fieldSound;
    u32 bgm;
    u16 fadeInFrames;
    u16 fadeOutFrames;
} EventSoundWork;

static GameEventReturnCode EventBGMFadeIn_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMFadeOut_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPushWait_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPushBGMFinish_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPop_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPopAll_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPlayPushEx_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventMEPlay_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMChange_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMFadeStop_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPlay_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPlayEx_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode ChangeBGMToFieldSilence_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMPlayPush_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBattleBGMPlay_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventWaitFieldSound_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBGMFadeWait_Callback(GameEvent *event, u32 *state, void *data);

// The BGM fade-in event's steps: send the fade, then finish
enum {
    EVENT_BGM_FADE_IN_SEQ_SEND_FADE,
    EVENT_BGM_FADE_IN_SEQ_DONE,
};

static GameEventReturnCode EventBGMFadeIn_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_FADE_IN_SEQ_SEND_FADE:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_IN, 0, 0, work->fadeInFrames);
        (*state)++;
        break;
    case EVENT_BGM_FADE_IN_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMFadeIn_Create(GameSystem *gsys, u32 fadeInFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMFadeIn_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeInFrames = fadeInFrames;
    return event;
}

// The BGM fade-out event's steps: send the fade, then finish
enum {
    EVENT_BGM_FADE_OUT_SEQ_SEND_FADE,
    EVENT_BGM_FADE_OUT_SEQ_DONE,
};

static GameEventReturnCode EventBGMFadeOut_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_FADE_OUT_SEQ_SEND_FADE:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_OUT, 0, work->fadeOutFrames, 0);
        (*state)++;
        break;
    case EVENT_BGM_FADE_OUT_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMFadeOut_Create(GameSystem *gsys, u32 fadeOutFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMFadeOut_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The BGM push event's steps: push the BGM, wait for it to fade, then finish
enum {
    EVENT_BGM_PUSH_WAIT_SEQ_PUSH,
    EVENT_BGM_PUSH_WAIT_SEQ_WAIT,
    EVENT_BGM_PUSH_WAIT_SEQ_DONE,
};

static GameEventReturnCode EventBGMPushWait_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_PUSH_WAIT_SEQ_PUSH:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PUSH, 0, work->fadeOutFrames, 0);
        (*state)++;
        break;
    case EVENT_BGM_PUSH_WAIT_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            (*state)++;
        }
        break;
    case EVENT_BGM_PUSH_WAIT_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPushWait_Create(GameSystem *gsys, u32 fadeOutFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPushWait_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The pushed BGM's finish: pop it, wait for the fade, then finish
enum {
    EVENT_PUSH_BGM_FINISH_SEQ_POP,
    EVENT_PUSH_BGM_FINISH_SEQ_WAIT,
    EVENT_PUSH_BGM_FINISH_SEQ_DONE,
};

static GameEventReturnCode EventPushBGMFinish_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_PUSH_BGM_FINISH_SEQ_POP:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_POP, 0, work->fadeOutFrames, work->fadeInFrames);
        (*state)++;
        break;
    case EVENT_PUSH_BGM_FINISH_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            (*state)++;
        }
        break;
    case EVENT_PUSH_BGM_FINISH_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventPushBGMFinish_Create(GameSystem *gsys, u32 fadeOutFrames, u32 fadeInFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPushBGMFinish_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeInFrames = fadeInFrames;
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The BGM pop event's steps: pop the BGM, then finish
enum {
    EVENT_BGM_POP_SEQ_POP,
    EVENT_BGM_POP_SEQ_DONE,
};

static GameEventReturnCode EventBGMPop_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_POP_SEQ_POP:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_POP, 0, work->fadeOutFrames, work->fadeInFrames);
        (*state)++;
        break;
    case EVENT_BGM_POP_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPop_CreateEx(GameSystem *gsys, u32 fadeOutFrames, u32 fadeInFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPop_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeInFrames = fadeInFrames;
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The BGM pop-all event's steps: pop all but the last, then finish
enum {
    EVENT_BGM_POP_ALL_SEQ_POP_ALL,
    EVENT_BGM_POP_ALL_SEQ_DONE,
};

static GameEventReturnCode EventBGMPopAll_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;
    s32 depth;
    s32 i;

    switch (*state) {
    case EVENT_BGM_POP_ALL_SEQ_POP_ALL:
        depth = FieldSnd_BGMGetStackIndex(fieldSound);
        if (depth > 0) {
            // Pops all but the last at once, and fades in the BGM under them
            for (i = 0; i < depth - 1; i++) {
                FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_POP, 0, 0, 0);
            }
            FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_POP, 0, 0, work->fadeInFrames);
        }
        (*state)++;
        break;
    case EVENT_BGM_POP_ALL_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPopAll_Create(GameSystem *gsys, u32 fadeInFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPopAll_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeInFrames = fadeInFrames;
    return event;
}

// The BGM play event's steps: push the old BGM and change to the new one, then finish
enum {
    EVENT_BGM_PLAY_PUSH_EX_SEQ_PUSH_AND_PLAY,
    EVENT_BGM_PLAY_PUSH_EX_SEQ_DONE,
};

static GameEventReturnCode EventBGMPlayPushEx_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_PLAY_PUSH_EX_SEQ_PUSH_AND_PLAY:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PUSH, 0, work->fadeOutFrames, 0);
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_CHANGE, work->bgm, 0, work->fadeInFrames);
        (*state)++;
        break;
    case EVENT_BGM_PLAY_PUSH_EX_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPlayPushEx_Create(GameSystem *gsys, u32 bgm, u32 fadeOutFrames, u32 fadeInFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPlayPushEx_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = bgm;
    work->fadeInFrames = fadeInFrames;
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The music effect's steps: push the BGM and play the effect, then finish
enum {
    EVENT_ME_PLAY_SEQ_PLAY,
    EVENT_ME_PLAY_SEQ_DONE,
};

static GameEventReturnCode EventMEPlay_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_ME_PLAY_SEQ_PLAY:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PUSH, 0, 6, 0);
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PLAY, work->bgm, 0, 0);
        (*state)++;
        break;
    case EVENT_ME_PLAY_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventMEPlay_Create(GameSystem *gsys, u32 meId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMEPlay_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = meId;
    work->fadeOutFrames = 0;
    return event;
}

// The BGM change event's steps: change the BGM, then finish
enum {
    EVENT_BGM_CHANGE_SEQ_CHANGE,
    EVENT_BGM_CHANGE_SEQ_DONE,
};

static GameEventReturnCode EventBGMChange_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_CHANGE_SEQ_CHANGE:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_CHANGE, work->bgm, work->fadeOutFrames, work->fadeInFrames);
        (*state)++;
        break;
    case EVENT_BGM_CHANGE_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMChange_Create(GameSystem *gsys, u32 bgm, u32 fadeOutFrames, u32 fadeInFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMChange_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = bgm;
    work->fadeInFrames = fadeInFrames;
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The BGM fade-stop event's steps: fade out, wait for it, release the field's sound, then finish
enum {
    EVENT_BGM_FADE_STOP_SEQ_FADE_OUT,
    EVENT_BGM_FADE_STOP_SEQ_WAIT,
    EVENT_BGM_FADE_STOP_SEQ_RELEASE,
    EVENT_BGM_FADE_STOP_SEQ_DONE,
};

static GameEventReturnCode EventBGMFadeStop_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    GameData *gameData = GSYS_GetGameData(work->gsys);
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_FADE_STOP_SEQ_FADE_OUT:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_OUT, 0, work->fadeOutFrames, 0);
        (*state)++;
        break;
    case EVENT_BGM_FADE_STOP_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            (*state)++;
        }
        break;
    case EVENT_BGM_FADE_STOP_SEQ_RELEASE:
        FieldSnd_Release(fieldSound, gameData);
        (*state)++;
        break;
    case EVENT_BGM_FADE_STOP_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The BGM play event's steps: fade out and play the BGM, then wait for it to start
enum {
    EVENT_BGM_PLAY_SEQ_FADE_OUT_AND_PLAY,
    EVENT_BGM_PLAY_SEQ_WAIT,
};

static GameEventReturnCode EventBGMPlay_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_PLAY_SEQ_FADE_OUT_AND_PLAY:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_OUT, 0, 6, 0);
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PLAY, work->bgm, 0, 0);
        (*state)++;
        break;
    case EVENT_BGM_PLAY_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPlay_Create(GameSystem *gsys, u32 bgm) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPlay_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = bgm;
    return event;
}

// The BGM play event's steps: fade out for the given frames and play the BGM, then wait for it
enum {
    EVENT_BGM_PLAY_EX_SEQ_FADE_OUT_AND_PLAY,
    EVENT_BGM_PLAY_EX_SEQ_WAIT,
};

static GameEventReturnCode EventBGMPlayEx_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_PLAY_EX_SEQ_FADE_OUT_AND_PLAY:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_OUT, 0, work->fadeOutFrames, 0);
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PLAY, work->bgm, 0, 0);
        (*state)++;
        break;
    case EVENT_BGM_PLAY_EX_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPlayEx_Create(GameSystem *gsys, u32 bgm, u32 fadeOutFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPlayEx_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = bgm;
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

static GameEventReturnCode ChangeBGMToFieldSilence_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    if (*state == 0) {
        FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_OUT, 0, work->fadeOutFrames, 0);
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PLAY, SEQ_BGM_SILENCE_FIELD, 0, 0);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *CreateBGMFadeOutEvent(GameSystem *gsys, u32 fadeOutFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, ChangeBGMToFieldSilence_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The BGM play event's steps: push the old BGM and play the new one, then wait for it
enum {
    EVENT_BGM_PLAY_PUSH_SEQ_PUSH_AND_PLAY,
    EVENT_BGM_PLAY_PUSH_SEQ_WAIT,
};

static GameEventReturnCode EventBGMPlayPush_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_PLAY_PUSH_SEQ_PUSH_AND_PLAY:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PUSH, 0, 6, 0);
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PLAY, work->bgm, 0, 0);
        (*state)++;
        break;
    case EVENT_BGM_PLAY_PUSH_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMPlayPush_Create(GameSystem *gsys, u32 bgm) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPlayPush_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = bgm;
    return event;
}

GameEvent *EventBGMFadeStop_Create(GameSystem *gsys, u16 fadeOutFrames) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMFadeStop_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeOutFrames = fadeOutFrames;
    return event;
}

// The battle BGM's steps: keep or fade out the field's BGM, wait, play the battle's, then finish
enum {
    EVENT_BATTLE_BGM_PLAY_SEQ_PUSH_OR_FADE,
    EVENT_BATTLE_BGM_PLAY_SEQ_WAIT,
    EVENT_BATTLE_BGM_PLAY_SEQ_PLAY,
    EVENT_BATTLE_BGM_PLAY_SEQ_DONE,
};

static GameEventReturnCode EventBattleBGMPlay_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BATTLE_BGM_PLAY_SEQ_PUSH_OR_FADE:
        // Keeps the field's BGM to come back to, unless one is already pushed
        if (FieldSnd_BGMGetStackIndex(fieldSound) == 0) {
            FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PUSH, 0, 6, 0);
        } else {
            FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_OUT, 0, 6, 0);
        }
        (*state)++;
        break;
    case EVENT_BATTLE_BGM_PLAY_SEQ_WAIT:
        if (FieldSnd_IsBusy(fieldSound) == FALSE) {
            (*state)++;
        }
        break;
    case EVENT_BATTLE_BGM_PLAY_SEQ_PLAY:
        FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PLAY, work->bgm, 0, 0);
        (*state)++;
        break;
    case EVENT_BATTLE_BGM_PLAY_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBattleBGMPlay_Create(GameSystem *gsys, u32 bgm) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleBGMPlay_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->bgm = bgm;
    return event;
}

GameEvent *EventBGMFadePop_Create(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMPop_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    work->fadeInFrames = 60;
    work->fadeOutFrames = 30;
    return event;
}

// The field sound wait's steps: wait for the BGM to finish changing, then finish
enum {
    EVENT_WAIT_FIELD_SOUND_SEQ_WAIT,
    EVENT_WAIT_FIELD_SOUND_SEQ_DONE,
};

static GameEventReturnCode EventWaitFieldSound_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_WAIT_FIELD_SOUND_SEQ_WAIT:
        if (FieldSnd_IsBGMChanging(fieldSound) == FALSE) {
            (*state)++;
        }
        break;
    case EVENT_WAIT_FIELD_SOUND_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWaitFieldSound_Create(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWaitFieldSound_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    return event;
}

// The BGM fade wait's steps: wait for the pop to finish, then finish
enum {
    EVENT_BGM_FADE_WAIT_SEQ_WAIT,
    EVENT_BGM_FADE_WAIT_SEQ_DONE,
};

static GameEventReturnCode EventBGMFadeWait_Callback(GameEvent *event, u32 *state, void *data) {
    EventSoundWork *work = data;
    FieldSound *fieldSound = work->fieldSound;

    switch (*state) {
    case EVENT_BGM_FADE_WAIT_SEQ_WAIT:
        if (FieldSnd_IsPopDone(fieldSound) == TRUE) {
            (*state)++;
        }
        break;
    case EVENT_BGM_FADE_WAIT_SEQ_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBGMFadeWait_Create(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBGMFadeWait_Callback, sizeof(EventSoundWork));
    EventSoundWork *work = GameEvent_GetData(event);
    work->fieldSound = GameData_GetFieldSoundSystem(gameData);
    return event;
}
