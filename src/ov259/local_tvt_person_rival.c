#include "app/local_tvt/local_tvt_person_rival.h"
#include "types.h"
#include "app/local_tvt/local_tvt_calldata.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/local_tvt/local_tvt_person.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// A message starts, waits for its face, then finishes
enum {
    LOCAL_TVT_RIVAL_TALK_START,
    LOCAL_TVT_RIVAL_TALK_WAIT,
    LOCAL_TVT_RIVAL_TALK_FINISH,
};

// What the rival says in a call on the Xtransceiver, named after the ROM's "local_tvt_person_rival.c"

struct LocalTvtRival {
    int step;
    u32 unk4;
    GameData *gameData;
    u16 heapId;
};

static LocalTvtRival *LocalTvtRival_Create(GameData *gameData, HeapID heapId);
static void LocalTvtRival_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Extra(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

void LocalTvtRival_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId) {
    MI_CpuFill8(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtRival_Create(gameData, heapId);
    funcs->msgData = msgData;
    funcs->free = LocalTvtRival_Free;
    funcs->greet = LocalTvtRival_Greet;
    funcs->topics[0] = LocalTvtRival_Chat;
    funcs->topicNames[0] = 15;
    funcs->topics[1] = LocalTvtRival_Extra;
    funcs->topicNames[1] = 16;
    funcs->topics[2] = LocalTvtRival_Bye;
    funcs->topicNames[2] = 17;
    funcs->topicCount = 3;
    funcs->menuMsg = 18;
}

static LocalTvtRival *LocalTvtRival_Create(GameData *gameData, HeapID heapId) {
    LocalTvtRival *rival = GFL_HeapAllocate(heapId, sizeof(LocalTvtRival), TRUE, "local_tvt_person_rival.c", 108);

    rival->heapId = heapId;
    rival->gameData = gameData;
    return rival;
}

static void LocalTvtRival_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    GFL_HeapFree(work);
}

static BOOL LocalTvtRival_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case LOCAL_TVT_RIVAL_TALK_START:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    case LOCAL_TVT_RIVAL_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case LOCAL_TVT_RIVAL_TALK_FINISH:
        rival->step = LOCAL_TVT_RIVAL_TALK_START;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtRival_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case LOCAL_TVT_RIVAL_TALK_START:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    case LOCAL_TVT_RIVAL_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case LOCAL_TVT_RIVAL_TALK_FINISH:
        rival->step = LOCAL_TVT_RIVAL_TALK_START;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtRival_Extra(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case LOCAL_TVT_RIVAL_TALK_START:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetExtraMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    case LOCAL_TVT_RIVAL_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case LOCAL_TVT_RIVAL_TALK_FINISH:
        rival->step = LOCAL_TVT_RIVAL_TALK_START;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtRival_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case LOCAL_TVT_RIVAL_TALK_START: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    }
    case LOCAL_TVT_RIVAL_TALK_WAIT:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case LOCAL_TVT_RIVAL_TALK_FINISH:
        rival->step = LOCAL_TVT_RIVAL_TALK_START;
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}
