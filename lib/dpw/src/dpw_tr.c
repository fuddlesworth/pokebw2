#include "dpw/dpw_tr.h"
#include "dpw_internal.h"
#include "dwc/dwc_encsession.h"
#include "nitro/os.h"

// The Global Trade Station's client, see dpw_tr.h. Every call fills the request that the game statistics client sends
// to one page of the server, and DpwTr's process runs it a frame at a time. The state is even while a call waits for
// its answer, and the answer of the server (a code, and for some calls the Pokémon) ends the call with a result: 0 or
// more when it went well, and a negative number for the ways it failed. A call that could not start ends in state 0x18
// with the result -13, and the other states after an answer or a failure of the game statistics client are 0x18 too,
// until the game asks for the end (DpwTr_IsFinished). The few bytes copied into the request and from the head of the
// answer go through the compiler's built-in memcpy (__memcpy), which copies a constant size in place; the larger copies
// call memcpy

// Where a call is: the state of the call that is out, one more while its cancel waits for the session to end,
// DPW_TR_STATE_IDLE when none is out, DPW_TR_STATE_DONE when one has ended with its result, and
// DPW_TR_STATE_UNINITIALIZED after DpwTr_End
enum {
    DPW_TR_STATE_UNINITIALIZED = 0,
    DPW_TR_STATE_IDLE = 1,
    DPW_TR_STATE_UPLOAD = 2,
    DPW_TR_STATE_UPLOAD_CANCELED = 3,
    DPW_TR_STATE_UPLOAD_FINISH = 4,
    DPW_TR_STATE_UPLOAD_FINISH_CANCELED = 5,
    DPW_TR_STATE_DOWNLOAD = 6,
    DPW_TR_STATE_DOWNLOAD_CANCELED = 7,
    DPW_TR_STATE_TRADE_RESULT = 8,
    DPW_TR_STATE_TRADE_RESULT_CANCELED = 9,
    DPW_TR_STATE_DELETE_FINISH = 10,
    DPW_TR_STATE_DELETE_FINISH_CANCELED = 11,
    DPW_TR_STATE_RETURN_FINISH = 12,
    DPW_TR_STATE_RETURN_FINISH_CANCELED = 13,
    DPW_TR_STATE_SEARCH = 14,
    DPW_TR_STATE_SEARCH_CANCELED = 15,
    DPW_TR_STATE_EXCHANGE = 16,
    DPW_TR_STATE_EXCHANGE_CANCELED = 17,
    DPW_TR_STATE_EXCHANGE_FINISH = 18,
    DPW_TR_STATE_EXCHANGE_FINISH_CANCELED = 19,
    DPW_TR_STATE_SERVER_STATE = 20,
    DPW_TR_STATE_SERVER_STATE_CANCELED = 21,
    DPW_TR_STATE_SET_PROFILE = 22,
    DPW_TR_STATE_SET_PROFILE_CANCELED = 23,
    DPW_TR_STATE_DONE = 0x18,
};

// The code in the first byte of a page's answer, where the result the library keeps from it has a name: 1 success (and
// the info and setProfile pages' normal state), 2 already deposited, 3 not found, 4 not traded (the trade result
// page), 5 already traded, 7 crowded and 14 the server in maintenance. The pages' answers 6 and 8 to 13 give results
// that have no name, so they stay numbers
enum {
    DPW_TR_ANSWER_SUCCESS = 1,
    DPW_TR_ANSWER_ALREADY_DEPOSITED = 2,
    DPW_TR_ANSWER_NOT_FOUND = 3,
    DPW_TR_ANSWER_NOT_TRADED = 4,
    DPW_TR_ANSWER_ALREADY_TRADED = 5,
    DPW_TR_ANSWER_CROWDED = 7,
    DPW_TR_ANSWER_SERVER_MAINTENANCE = 14,
};

// The state of the library: where the call is, what it ended with, the player's ID and friend key, and the request
// that is sent: the Pokémon, the signature that goes with it and the size of the signature. The answer of the server
// starts with two codes
typedef struct DpwTrWork {
    int state;
    int result;
    s32 pid;
    DpwFriendKey friendKey;
    u8 request[0x128];
    u32 requestId;
    u8 signature[0x80];
    u32 signatureSize;
    u8 code[2];
    void *resultData;
    // Zeroed with the static, but never read or written: nothing shows what it holds
    u32 unk1CC;
} DpwTrWork;

static int DpwTrMapGamestatsError(DWCErrorType group);

static DpwTrWork sDpwTrWork;
static int sDpwTrServerType;

void DpwTr_Init(s32 pid, u64 friendKey, int server) {
    sDpwTrWork.state = DPW_TR_STATE_IDLE;
    sDpwTrWork.result = 0;
    sDpwTrWork.pid = pid;
    sDpwTrWork.friendKey.low = (u32)friendKey;
    sDpwTrWork.friendKey.high = friendKey >> 32;
    switch (server) {
    case GAMESTATS_SERVER_LIVE:
        sDpwTrServerType = GAMESTATS_SERVER_LIVE;
        break;
    case GAMESTATS_SERVER_DEVELOPMENT:
        sDpwTrServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    case GAMESTATS_SERVER_TEST:
        sDpwTrServerType = GAMESTATS_SERVER_TEST;
        break;
    default:
        sDpwTrServerType = GAMESTATS_SERVER_DEVELOPMENT;
        break;
    }
}

void DpwTr_Process(void) {
    const u8 *answer;
    int size;
    int type;
    DWCErrorType group;

    switch (sDpwTrWork.state) {
    case DPW_TR_STATE_UPLOAD:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.code, answer, sizeof(sDpwTrWork.code));
            switch (sDpwTrWork.code[0]) {
            case DPW_TR_ANSWER_SUCCESS:
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
                break;
            case DPW_TR_ANSWER_ALREADY_DEPOSITED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_DEPOSITED;
                break;
            case DPW_TR_ANSWER_NOT_FOUND:
                sDpwTrWork.result = DPW_TR_RESULT_NOT_FOUND;
                break;
            case DPW_TR_ANSWER_CROWDED:
                sDpwTrWork.result = DPW_TR_RESULT_CROWDED;
                break;
            case 12:
                sDpwTrWork.result = -6;
                break;
            case 13:
                sDpwTrWork.result = -7;
                break;
            case 8:
                sDpwTrWork.result = -8;
                break;
            case 9:
                sDpwTrWork.result = -9;
                break;
            case 10:
                sDpwTrWork.result = -10;
                break;
            case 11:
                sDpwTrWork.result = -11;
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            default:
                sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_UPLOAD_FINISH:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.code, answer, sizeof(sDpwTrWork.code));
            switch (sDpwTrWork.code[0]) {
            case DPW_TR_ANSWER_SUCCESS:
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
                break;
            case DPW_TR_ANSWER_ALREADY_DEPOSITED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_DEPOSITED;
                break;
            case DPW_TR_ANSWER_NOT_FOUND:
                sDpwTrWork.result = DPW_TR_RESULT_NOT_FOUND;
                break;
            case DPW_TR_ANSWER_ALREADY_TRADED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_TRADED;
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            default:
                sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_DOWNLOAD:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            memcpy(sDpwTrWork.resultData, answer, sizeof(Dpw_Tr_Data));
            if (size == sizeof(Dpw_Tr_Data)) {
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case DPW_TR_ANSWER_ALREADY_TRADED:
                    sDpwTrWork.result = DPW_TR_RESULT_ALREADY_TRADED;
                    break;
                case DPW_TR_ANSWER_NOT_FOUND:
                    sDpwTrWork.result = DPW_TR_RESULT_NOT_FOUND;
                    break;
                case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                    sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                    break;
                default:
                    sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_TRADE_RESULT:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            memcpy(sDpwTrWork.resultData, answer, sizeof(Dpw_Tr_Data));
            if (size == sizeof(Dpw_Tr_Data)) {
                sDpwTrWork.result = DPW_TR_TRADE_TRADED;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case DPW_TR_ANSWER_ALREADY_TRADED:
                    sDpwTrWork.result = DPW_TR_RESULT_ALREADY_TRADED;
                    break;
                case DPW_TR_ANSWER_NOT_FOUND:
                    sDpwTrWork.result = DPW_TR_RESULT_NOT_FOUND;
                    break;
                case DPW_TR_ANSWER_NOT_TRADED:
                    sDpwTrWork.result = DPW_TR_TRADE_NOT_TRADED;
                    break;
                case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                    sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                    break;
                default:
                    sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_DELETE_FINISH:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.code, answer, sizeof(sDpwTrWork.code));
            switch (sDpwTrWork.code[0]) {
            case DPW_TR_ANSWER_SUCCESS:
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
                break;
            case DPW_TR_ANSWER_ALREADY_TRADED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_TRADED;
                break;
            case DPW_TR_ANSWER_NOT_FOUND:
                sDpwTrWork.result = DPW_TR_RESULT_NOT_FOUND;
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            default:
                sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_RETURN_FINISH:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.code, answer, sizeof(sDpwTrWork.code));
            switch (sDpwTrWork.code[0]) {
            case DPW_TR_ANSWER_SUCCESS:
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
                break;
            case DPW_TR_ANSWER_ALREADY_TRADED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_TRADED;
                break;
            case DPW_TR_ANSWER_ALREADY_DEPOSITED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_DEPOSITED;
                break;
            case DPW_TR_ANSWER_NOT_FOUND:
                sDpwTrWork.result = DPW_TR_RESULT_NOT_FOUND;
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            default:
                sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_SEARCH:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.resultData, answer, sizeof(sDpwTrWork.code));
            switch (*(u8 *)sDpwTrWork.resultData) {
            case DPW_TR_ANSWER_SUCCESS:
                if ((u32)(size - 2) >= sizeof(Dpw_Tr_Data)) {
                    sDpwTrWork.result = (u32)(size - 2) / sizeof(Dpw_Tr_Data);
                    memcpy(sDpwTrWork.resultData, answer + 2, size - 2);
                } else if (size == sizeof(sDpwTrWork.code)) {
                    sDpwTrWork.result = 0;
                }
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            default:
                sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_EXCHANGE:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            memcpy(sDpwTrWork.resultData, answer, sizeof(Dpw_Tr_Data));
            if (size == sizeof(Dpw_Tr_Data)) {
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case DPW_TR_ANSWER_ALREADY_DEPOSITED:
                    sDpwTrWork.result = DPW_TR_RESULT_ALREADY_DEPOSITED;
                    break;
                case 12:
                    sDpwTrWork.result = -6;
                    break;
                case 13:
                    sDpwTrWork.result = -7;
                    break;
                case 8:
                    sDpwTrWork.result = -8;
                    break;
                case 9:
                    sDpwTrWork.result = -9;
                    break;
                case 10:
                    sDpwTrWork.result = -10;
                    break;
                case 11:
                    sDpwTrWork.result = -11;
                    break;
                case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                    sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                    break;
                default:
                    sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_EXCHANGE_FINISH:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.code, answer, sizeof(sDpwTrWork.code));
            switch (sDpwTrWork.code[0]) {
            case DPW_TR_ANSWER_SUCCESS:
                sDpwTrWork.result = DPW_TR_RESULT_SUCCESS;
                break;
            case DPW_TR_ANSWER_ALREADY_DEPOSITED:
                sDpwTrWork.result = DPW_TR_RESULT_ALREADY_DEPOSITED;
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_SERVER_STATE:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.code, answer, sizeof(sDpwTrWork.code));
            switch (sDpwTrWork.code[0]) {
            case DPW_TR_ANSWER_SUCCESS:
                sDpwTrWork.result = 0;
                break;
            case 6:
                sDpwTrWork.result = 1;
                break;
            case DPW_TR_ANSWER_CROWDED:
                sDpwTrWork.result = 2;
                break;
            case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                break;
            default:
                sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                break;
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_SET_PROFILE:
        switch (DWCi_EncSessionProcess()) {
        case GAMESTATS_STATE_FAILED:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            DWC_GetLastErrorEx(&type, &group);
            sDpwTrWork.result = DpwTrMapGamestatsError(group);
            DWCi_EncSessionShutdown();
            break;
        case GAMESTATS_STATE_DONE:
            sDpwTrWork.state = DPW_TR_STATE_DONE;
            answer = DWCi_EncSessionGetResponse(&size);
            __memcpy(sDpwTrWork.resultData, answer, sizeof(sDpwTrWork.code));
            if (size == sizeof(Dpw_Common_ProfileResult)) {
                sDpwTrWork.result = 0;
            } else {
                switch (*(u8 *)sDpwTrWork.resultData) {
                case DPW_TR_ANSWER_SUCCESS:
                    sDpwTrWork.result = 0;
                    break;
                case 6:
                    sDpwTrWork.result = 1;
                    break;
                case DPW_TR_ANSWER_CROWDED:
                    sDpwTrWork.result = 2;
                    break;
                case DPW_TR_ANSWER_SERVER_MAINTENANCE:
                    sDpwTrWork.result = DPW_TR_RESULT_SERVER_MAINTENANCE;
                    break;
                default:
                    sDpwTrWork.result = DPW_TR_RESULT_ERROR;
                    break;
                }
            }
            DWCi_EncSessionShutdown();
            break;
        }
        break;
    case DPW_TR_STATE_UNINITIALIZED:
    case DPW_TR_STATE_IDLE:
    case DPW_TR_STATE_DONE:
        break;
    case DPW_TR_STATE_UPLOAD_CANCELED:
    case DPW_TR_STATE_UPLOAD_FINISH_CANCELED:
    case DPW_TR_STATE_DOWNLOAD_CANCELED:
    case DPW_TR_STATE_TRADE_RESULT_CANCELED:
    case DPW_TR_STATE_DELETE_FINISH_CANCELED:
    case DPW_TR_STATE_RETURN_FINISH_CANCELED:
    case DPW_TR_STATE_SEARCH_CANCELED:
    case DPW_TR_STATE_EXCHANGE_CANCELED:
    case DPW_TR_STATE_EXCHANGE_FINISH_CANCELED:
    case DPW_TR_STATE_SERVER_STATE_CANCELED:
    case DPW_TR_STATE_SET_PROFILE_CANCELED:
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_CANCELED;
        DWCi_EncSessionShutdown();
        break;
    }
}

void DpwTr_End(void) {
    sDpwTrWork.state = DPW_TR_STATE_UNINITIALIZED;
}

BOOL DpwTr_IsFinished(void) {
    switch (sDpwTrWork.state) {
    case DPW_TR_STATE_IDLE:
        return TRUE;
    case DPW_TR_STATE_DONE:
        sDpwTrWork.state = DPW_TR_STATE_IDLE;
        return TRUE;
    default:
        return FALSE;
    }
}

s32 DpwTr_GetResult(void) {
    return sDpwTrWork.result;
}

void DpwTr_Upload(const Dpw_Tr_Data *data, const void *sign, u32 size) {
    memcpy(sDpwTrWork.request, data, sizeof(Dpw_Tr_Data));
    sDpwTrWork.requestId = 0;
    memcpy(sDpwTrWork.signature, sign, size);
    sDpwTrWork.signatureSize = size;
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/post.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_UPLOAD;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_UploadFinish(void) {
    __memcpy(sDpwTrWork.request, &sDpwTrWork.friendKey, sizeof(sDpwTrWork.friendKey));
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/post_finish.asp", sDpwTrWork.pid, sDpwTrWork.request, 8, NULL) ==
        0) {
        sDpwTrWork.state = DPW_TR_STATE_UPLOAD_FINISH;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_Download(Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/get.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_DOWNLOAD;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_GetTradeResult(Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/result.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) ==
        0) {
        sDpwTrWork.state = DPW_TR_STATE_TRADE_RESULT;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_DeleteFinish(void) {
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/delete.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) ==
        0) {
        sDpwTrWork.state = DPW_TR_STATE_DELETE_FINISH;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_ReturnFinish(void) {
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/return.asp", sDpwTrWork.pid, sDpwTrWork.request, 0, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_RETURN_FINISH;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

// Cancels the call that is running, and waits for the next state
void DpwTr_Cancel(void) {
    switch (sDpwTrWork.state) {
    case DPW_TR_STATE_UPLOAD:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_UPLOAD_CANCELED;
        break;
    case DPW_TR_STATE_UPLOAD_FINISH:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_UPLOAD_FINISH_CANCELED;
        break;
    case DPW_TR_STATE_DOWNLOAD:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_DOWNLOAD_CANCELED;
        break;
    case DPW_TR_STATE_TRADE_RESULT:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_TRADE_RESULT_CANCELED;
        break;
    case DPW_TR_STATE_DELETE_FINISH:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_DELETE_FINISH_CANCELED;
        break;
    case DPW_TR_STATE_RETURN_FINISH:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_RETURN_FINISH_CANCELED;
        break;
    case DPW_TR_STATE_SEARCH:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_SEARCH_CANCELED;
        break;
    case DPW_TR_STATE_EXCHANGE:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_EXCHANGE_CANCELED;
        break;
    case DPW_TR_STATE_EXCHANGE_FINISH:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_EXCHANGE_FINISH_CANCELED;
        break;
    case DPW_TR_STATE_SERVER_STATE:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_SERVER_STATE_CANCELED;
        break;
    case DPW_TR_STATE_SET_PROFILE:
        DWCi_EncSessionCancel();
        sDpwTrWork.state = DPW_TR_STATE_SET_PROFILE_CANCELED;
        break;
    }
}

void DpwTr_Search(const Dpw_Tr_PokemonSearchData *search, s32 maxNum, Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    __memcpy(sDpwTrWork.request, search, sizeof(*search));
    sDpwTrWork.request[6] = maxNum;
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/search.asp", sDpwTrWork.pid, sDpwTrWork.request, 7, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_SEARCH;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_SearchEx(const Dpw_Tr_PokemonSearchDataEx *search, Dpw_Tr_Data *result) {
    sDpwTrWork.resultData = result;
    __memcpy(sDpwTrWork.request, search, sizeof(*search));
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/search.asp", sDpwTrWork.pid, sDpwTrWork.request, 8, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_SEARCH;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_Exchange(s32 id, const Dpw_Tr_Data *upload, Dpw_Tr_Data *result, const void *sign, u32 size) {
    sDpwTrWork.resultData = result;
    memcpy(sDpwTrWork.request, upload, sizeof(Dpw_Tr_Data));
    sDpwTrWork.requestId = id;
    memcpy(sDpwTrWork.signature, sign, size);
    sDpwTrWork.signatureSize = size;
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/exchange.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x1b0, NULL) ==
        0) {
        sDpwTrWork.state = DPW_TR_STATE_EXCHANGE;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_ExchangeFinish(void) {
    __memcpy(sDpwTrWork.request, &sDpwTrWork.friendKey, sizeof(sDpwTrWork.friendKey));
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/exchange_finish.asp", sDpwTrWork.pid, sDpwTrWork.request, 8,
                                NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_EXCHANGE_FINISH;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_GetServerState(void) {
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/worldexchange/info.asp", sDpwTrWork.pid, sDpwTrWork.request, 0, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_SERVER_STATE;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

void DpwTr_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result) {
    OS_GetMacAddress(profile->macAddress);
    memcpy(sDpwTrWork.request, profile, 0x64);
    sDpwTrWork.resultData = result;
    DWCi_EncSessionInitialize(sDpwTrServerType, DPW_GAMESTATS_KEY);
    if (DWCi_EncSessionGetAsync("/web/common/setProfile.asp", sDpwTrWork.pid, sDpwTrWork.request, 0x64, NULL) == 0) {
        sDpwTrWork.state = DPW_TR_STATE_SET_PROFILE;
    } else {
        sDpwTrWork.state = DPW_TR_STATE_DONE;
        sDpwTrWork.result = DPW_TR_RESULT_ERROR;
        DWCi_EncSessionShutdown();
    }
}

// The result of a failure of the game statistics client, from its group of errors
static int DpwTrMapGamestatsError(DWCErrorType group) {
    int result;

    switch (group) {
    case DWC_ETYPE_NO_ERROR:
    case DWC_ETYPE_LIGHT:
    case DWC_ETYPE_SHOW_ERROR:
        result = -15;
        break;
    case DWC_ETYPE_SHUTDOWN_FM:
        result = -14;
        break;
    case DWC_ETYPE_SHUTDOWN_ND:
        result = -14;
        break;
    case DWC_ETYPE_SHUTDOWN_GHTTP:
        result = -15;
        break;
    case DWC_ETYPE_DISCONNECT:
        result = -14;
        break;
    case DWC_ETYPE_FATAL:
        result = DPW_TR_RESULT_ERROR;
        break;
    default:
        result = -14;
        break;
    }
    if (result != DPW_TR_RESULT_ERROR) {
        if (DWC_UpdateConnection()) {
            result = -14;
        }
        DWC_ClearError();
    }
    return result;
}
