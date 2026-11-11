#include "dpw/dpw_bt.h"
#include "dpw_internal.h"
#include "dwc/dwc_encsession.h"
#include "nitro/os.h"

// The Battle Subway's client, see dpw_bt.h. It works like dpw_tr.c: the states are even while a request is out and the
// answer of the server decides the result, which is 0 or more when it went well and negative otherwise. The states
// end at 0xc, which is also where a request that failed stops. As there, the copies of a few bytes go through the
// compiler's built-in memcpy (__memcpy), which copies a constant size in place

// Where a request is: the state of the request that is out, one more while its cancel waits for the session to end,
// DPW_BT_STATE_IDLE when none is out, DPW_BT_STATE_DONE when one has ended with its result, and
// DPW_BT_STATE_UNINITIALIZED after DpwBt_End
enum {
    DPW_BT_STATE_UNINITIALIZED = 0,
    DPW_BT_STATE_IDLE = 1,
    DPW_BT_STATE_GET_ROOM_NUMBER = 2,
    DPW_BT_STATE_GET_ROOM_NUMBER_CANCELED = 3,
    DPW_BT_STATE_DOWNLOAD = 4,
    DPW_BT_STATE_DOWNLOAD_CANCELED = 5,
    DPW_BT_STATE_UPLOAD = 6,
    DPW_BT_STATE_UPLOAD_CANCELED = 7,
    DPW_BT_STATE_GET_SERVER_STATE = 8,
    DPW_BT_STATE_GET_SERVER_STATE_CANCELED = 9,
    DPW_BT_STATE_SET_PROFILE = 10,
    DPW_BT_STATE_SET_PROFILE_CANCELED = 11,
    DPW_BT_STATE_DONE = 12,
};

// The code in the first byte of a page's answer: success, and 2, 3 and 4 for the failures that the library keeps as
// DPW_BT_RESULT_INVALID_NUMBER, DPW_BT_SERVER_STATE_MAINTENANCE and DPW_BT_RESULT_CROWDED. The answer 5 gives the
// unnamed result -2, so it stays a number
enum {
    DPW_BT_ANSWER_SUCCESS = 1,
    DPW_BT_ANSWER_INVALID_NUMBER = 2,
    DPW_BT_ANSWER_MAINTENANCE = 3,
    DPW_BT_ANSWER_CROWDED = 4,
};

// The size of the players and leaders of a room, which the download page answers with
#define DPW_BT_DOWNLOAD_SIZE 0xa8c

// The team that is uploaded, with what the server needs to know about it. It is built here and then copied to the
// request, the same bytes as the request holds
typedef struct DpwBtUpload {
    u8 team[0xf0];
    u8 kind;
    u8 room;
    u8 win;
    // Zeroed with the static and copied to the request, but never read or written: nothing shows what it holds
    u8 unkF3[5];
    DpwFriendKey key;
    u8 sign[0x80];
    u32 id;
} DpwBtUpload;

typedef struct DpwBtWork {
    int state;
    int result;
    s32 pid;
    DpwFriendKey friendKey;
    u8 request[0x184];
    u8 code[2];
    void *resultData;
    // Zeroed with the static, but never read or written: nothing shows what it holds
    u32 unk1A0;
} DpwBtWork;

static int DpwBtMapGamestatsError(DWCErrorType group);

static DpwBtWork sDpwBtWork;
static DpwBtUpload sDpwBtUpload;
static int sDpwBtServerType;

void DpwBt_Init(s32 pid, u64 friendKey, int server) {
    sDpwBtWork.state = DPW_BT_STATE_IDLE;
    sDpwBtWork.result = 0;
    sDpwBtWork.pid = pid;
    sDpwBtWork.friendKey.low = (u32)friendKey;
    sDpwBtWork.friendKey.high = friendKey >> 32;
    switch (server) {
    case GAMESTATS_SERVER_LIVE:
        sDpwBtServerType = GAMESTATS_SERVER_LIVE;
        break;
    case GAMESTATS_SERVER_DEVELOPMENT:
        sDpwBtServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    case GAMESTATS_SERVER_TEST:
        sDpwBtServerType = GAMESTATS_SERVER_TEST;
        break;
    default:
        sDpwBtServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    }
}

void DpwBt_Process(void) {
    const u8 *answer;
    int size;
    int type;
    DWCErrorType group;

    switch (sDpwBtWork.state) {
    case DPW_BT_STATE_GET_ROOM_NUMBER:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.code, answer, sizeof(sDpwBtWork.code));
            if (size != 0) {
                sDpwBtWork.result = sDpwBtWork.code[0];
            } else {
                sDpwBtWork.result = -2;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_BT_STATE_DOWNLOAD:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            memcpy(sDpwBtWork.resultData, answer, DPW_BT_DOWNLOAD_SIZE);
            if (size == DPW_BT_DOWNLOAD_SIZE) {
                sDpwBtWork.result = 0;
            } else {
                switch (*(u8 *)sDpwBtWork.resultData) {
                case DPW_BT_ANSWER_INVALID_NUMBER:
                    sDpwBtWork.result = DPW_BT_RESULT_INVALID_NUMBER;
                    break;
                case 5:
                    sDpwBtWork.result = -2;
                    break;
                default:
                    sDpwBtWork.result = DPW_BT_RESULT_ERROR;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_BT_STATE_UPLOAD:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.code, answer, sizeof(sDpwBtWork.code));
            switch (sDpwBtWork.code[0]) {
            case DPW_BT_ANSWER_SUCCESS:
                sDpwBtWork.result = 0;
                break;
            case DPW_BT_ANSWER_INVALID_NUMBER:
                sDpwBtWork.result = DPW_BT_RESULT_INVALID_NUMBER;
                break;
            case DPW_BT_ANSWER_CROWDED:
                sDpwBtWork.result = DPW_BT_RESULT_CROWDED;
                break;
            case 5:
                sDpwBtWork.result = -2;
                break;
            default:
                sDpwBtWork.result = DPW_BT_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_BT_STATE_GET_SERVER_STATE:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.code, answer, sizeof(sDpwBtWork.code));
            switch (sDpwBtWork.code[0]) {
            case DPW_BT_ANSWER_SUCCESS:
                sDpwBtWork.result = 0;
                break;
            case DPW_BT_ANSWER_MAINTENANCE:
                sDpwBtWork.result = DPW_BT_SERVER_STATE_MAINTENANCE;
                break;
            case DPW_BT_ANSWER_CROWDED:
                sDpwBtWork.result = DPW_BT_SERVER_STATE_CROWDED;
                break;
            case 5:
                sDpwBtWork.result = -2;
                break;
            default:
                sDpwBtWork.result = DPW_BT_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_BT_STATE_SET_PROFILE:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwBtWork.result = DpwBtMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwBtWork.state = DPW_BT_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwBtWork.resultData, answer, sizeof(Dpw_Common_ProfileResult));
            if (size == sizeof(Dpw_Common_ProfileResult)) {
                sDpwBtWork.result = 0;
            } else {
                switch (*(u8 *)sDpwBtWork.resultData) {
                case DPW_BT_ANSWER_SUCCESS:
                    sDpwBtWork.result = 0;
                    break;
                case DPW_BT_ANSWER_MAINTENANCE:
                    sDpwBtWork.result = DPW_BT_SERVER_STATE_MAINTENANCE;
                    break;
                case DPW_BT_ANSWER_CROWDED:
                    sDpwBtWork.result = DPW_BT_SERVER_STATE_CROWDED;
                    break;
                case 5:
                    sDpwBtWork.result = -2;
                    break;
                default:
                    sDpwBtWork.result = DPW_BT_RESULT_ERROR;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_BT_STATE_GET_ROOM_NUMBER_CANCELED:
    case DPW_BT_STATE_DOWNLOAD_CANCELED:
    case DPW_BT_STATE_UPLOAD_CANCELED:
    case DPW_BT_STATE_GET_SERVER_STATE_CANCELED:
    case DPW_BT_STATE_SET_PROFILE_CANCELED:
        sDpwBtWork.state = DPW_BT_STATE_DONE;
        sDpwBtWork.result = DPW_BT_RESULT_CANCELED;
        DWCi_EncSessionShutdown();
        break;
    case DPW_BT_STATE_UNINITIALIZED:
    case DPW_BT_STATE_IDLE:
    case DPW_BT_STATE_DONE:
        break;
    }
}

void DpwBt_End(void) {
    sDpwBtWork.state = DPW_BT_STATE_UNINITIALIZED;
}

BOOL DpwBt_IsFinished(void) {
    switch (sDpwBtWork.state) {
    case DPW_BT_STATE_IDLE:
        return TRUE;
    case DPW_BT_STATE_DONE:
        sDpwBtWork.state = DPW_BT_STATE_IDLE;
        return TRUE;
    default:
        return FALSE;
    }
}

s32 DpwBt_GetResult(void) {
    return sDpwBtWork.result;
}

void DpwBt_GetRoomNumber(int kind) {
    sDpwBtWork.request[0] = kind - 1;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/roomnum.asp", sDpwBtWork.pid, sDpwBtWork.request, 1, NULL) == 0) {
        sDpwBtWork.state = DPW_BT_STATE_GET_ROOM_NUMBER;
    } else {
        sDpwBtWork.state = DPW_BT_STATE_DONE;
        sDpwBtWork.result = DPW_BT_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_Download(int kind, int room, void *result) {
    sDpwBtWork.resultData = result;
    sDpwBtWork.request[0] = kind - 1;
    sDpwBtWork.request[1] = room - 1;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/download.asp", sDpwBtWork.pid, sDpwBtWork.request, 2, NULL) == 0) {
        sDpwBtWork.state = DPW_BT_STATE_DOWNLOAD;
    } else {
        sDpwBtWork.state = DPW_BT_STATE_DONE;
        sDpwBtWork.result = DPW_BT_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_Upload(int kind, int room, u8 win, const void *team, const void *sign, u32 id) {
    memcpy(sDpwBtUpload.team, team, 0xf0);
    sDpwBtUpload.kind = kind - 1;
    sDpwBtUpload.room = room - 1;
    sDpwBtUpload.win = win;
    __memcpy(&sDpwBtUpload.key, &sDpwBtWork.friendKey, sizeof(sDpwBtUpload.key));
    memcpy(sDpwBtUpload.sign, sign, 0x80);
    sDpwBtUpload.id = id;
    *(DpwBtUpload *)sDpwBtWork.request = sDpwBtUpload;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/upload.asp", sDpwBtWork.pid, sDpwBtWork.request, 0x184, NULL) == 0) {
        sDpwBtWork.state = DPW_BT_STATE_UPLOAD;
    } else {
        sDpwBtWork.state = DPW_BT_STATE_DONE;
        sDpwBtWork.result = DPW_BT_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_Cancel(void) {
    switch (sDpwBtWork.state) {
    case DPW_BT_STATE_GET_ROOM_NUMBER:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = DPW_BT_STATE_GET_ROOM_NUMBER_CANCELED;
        break;
    case DPW_BT_STATE_DOWNLOAD:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = DPW_BT_STATE_DOWNLOAD_CANCELED;
        break;
    case DPW_BT_STATE_UPLOAD:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = DPW_BT_STATE_UPLOAD_CANCELED;
        break;
    case DPW_BT_STATE_GET_SERVER_STATE:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = DPW_BT_STATE_GET_SERVER_STATE_CANCELED;
        break;
    case DPW_BT_STATE_SET_PROFILE:
        DWCi_EncSessionCancel();
        sDpwBtWork.state = DPW_BT_STATE_SET_PROFILE_CANCELED;
        break;
    }
}

void DpwBt_GetServerState(void) {
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/battletower/info.asp", sDpwBtWork.pid, sDpwBtWork.request, 0, NULL) == 0) {
        sDpwBtWork.state = DPW_BT_STATE_GET_SERVER_STATE;
    } else {
        sDpwBtWork.state = DPW_BT_STATE_DONE;
        sDpwBtWork.result = DPW_BT_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwBt_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result) {
    OS_GetMacAddress(profile->macAddress);
    memcpy(sDpwBtWork.request, profile, 0x64);
    sDpwBtWork.resultData = result;
    DWCi_EncSessionInitialize(sDpwBtServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/common/setProfile.asp", sDpwBtWork.pid, sDpwBtWork.request, 0x64, NULL) == 0) {
        sDpwBtWork.state = DPW_BT_STATE_SET_PROFILE;
    } else {
        sDpwBtWork.state = DPW_BT_STATE_DONE;
        sDpwBtWork.result = DPW_BT_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

// The result of a failure of the game statistics client, from its group of errors
static int DpwBtMapGamestatsError(DWCErrorType group) {
    int result;

    switch (group) {
    case DWC_ETYPE_NO_ERROR:
    case DWC_ETYPE_LIGHT:
    case DWC_ETYPE_SHOW_ERROR:
        result = -7;
        break;
    case DWC_ETYPE_SHUTDOWN_FM:
        result = -6;
        break;
    case DWC_ETYPE_SHUTDOWN_ND:
        result = -6;
        break;
    case DWC_ETYPE_SHUTDOWN_GHTTP:
        result = -7;
        break;
    case DWC_ETYPE_DISCONNECT:
        result = -6;
        break;
    case DWC_ETYPE_FATAL:
        result = DPW_BT_RESULT_ERROR;
        break;
    default:
        result = -6;
        break;
    }
    if (result != DPW_BT_RESULT_ERROR) {
        if (DWC_UpdateConnection()) {
            result = -6;
        }
        DWC_ClearError();
    }
    return result;
}
