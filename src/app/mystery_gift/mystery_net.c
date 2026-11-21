#include "types.h"
#include "app/mystery/mystery_net.h"
#include "constants/version.h"
#include "dwc/dwc.h"
#include "dwc/nd.h"
#include "field/delivery_beacon.h"
#include "field/delivery_irc.h"
#include "gfl/dwc_rap.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "save/mystery_gift.h"
#include "system/version.h"

enum {
    WIRELESS_START_STATE_INIT = 0,
    WIRELESS_START_STATE_WAIT_INIT = 1,
    WIRELESS_START_STATE_READY = 2,
};

enum {
    WIRELESS_END_STATE_DESTROY_WORK = 0,
    WIRELESS_END_STATE_EXIT_NET = 1,
    WIRELESS_END_STATE_WAIT_EXIT = 2,
    WIRELESS_END_STATE_IDLE = 3,
};

enum {
    BEACON_START_STATE_INIT = 0,
    BEACON_START_STATE_WAIT_INIT = 1,
    BEACON_START_STATE_READY = 2,
};

enum {
    BEACON_END_STATE_END = 0,
    BEACON_END_STATE_WAIT_END = 1,
    BEACON_END_STATE_IDLE = 2,
};

enum {
    IRC_START_STATE_INIT = 0,
    IRC_START_STATE_WAIT_INIT = 1,
    IRC_START_STATE_READY = 2,
};

enum {
    IRC_END_STATE_END = 0,
    IRC_END_STATE_WAIT_END = 1,
    IRC_END_STATE_IDLE = 2,
};

enum {
    DOWNLOAD_STATE_INIT = 0,
    DOWNLOAD_STATE_SET_ATTR = 1,
    DOWNLOAD_STATE_FILTER_FILES = 2,
    DOWNLOAD_STATE_GET_FILE_LIST = 3,
    DOWNLOAD_STATE_SELECT_FILE = 4,
    DOWNLOAD_STATE_GET_FILE = 5,
    DOWNLOAD_STATE_WAIT_FILE = 6,
    DOWNLOAD_STATE_CANCEL = 7,
    DOWNLOAD_STATE_CLEANUP = 8,
    DOWNLOAD_STATE_END = 9,
};

enum {
    WIFI_END_STATE_EXIT_NET = 0,
    WIFI_END_STATE_WAIT_EXIT = 1,
    WIFI_END_STATE_IDLE = 2,
};

// Mystery Gift's receiving of gifts: by local wireless beacons and by infrared through overlay 12's delivery_beacon.c
// and delivery_irc.c, and by Wi-Fi through NitroDWC's download library. Our names; swan has none for this overlay

#define MYSTERY_NET_FILE_MAX 10
#define MYSTERY_NET_BUFFER_SIZE 0x1000

typedef struct MysteryNetSeq MysteryNetSeq;

typedef void (*MysteryNetSeqFunc)(MysteryNetSeq *seq, int *state, void *work);

struct MysteryNetSeq {
    MysteryNetSeqFunc func;
    BOOL end;
    int state;
    void *work;
};

// The states of a Wi-Fi download that wait for the library
#define DOWNLOAD_WAIT 100
#define DOWNLOAD_WAIT_CLEANUP 200

typedef struct {
    DWCNdFileInfo files[MYSTERY_NET_FILE_MAX];
    BOOL available[MYSTERY_NET_FILE_MAX];
    // Where a wait goes on to
    u32 nextState;
    BOOL cancel;
    BOOL cancelled;
    int count;
    u32 percent;
    u32 received;
    u32 contentLength;
    u32 selected;
    void *buffer;
    u32 timeout;
    BOOL initialized;
} MysteryNetDownload;

struct MysteryNet {
    MysteryNetSeq seq;
    WihDwcWork *wifiWork;
    u8 info[0x20];
    u8 unk34[0x20];
    u32 beaconFlags;
    HeapID heapId;
    BOOL exited;
    MysteryNetDownload download;
    SaveControl *save;
    DeliveryBeaconWork *beacon;
    DeliveryIrcWork *irc;
    u32 unk7A0;
    u8 buffer[MYSTERY_NET_BUFFER_SIZE];
    u32 result;
};

// What the download library's callback last gave
typedef struct {
    BOOL done;
    BOOL ready;
    u32 unk8;
    u32 error;
} MysteryNetNdState;

// A hexadecimal digit and its value
typedef struct {
    s8 c;
    s8 value;
} MysteryHexDigit;

static u32 MysteryNet_CheckErrorWifi(MysteryNet *net);
static u32 MysteryNet_CheckErrorWireless(MysteryNet *net);
static u32 MysteryNet_CheckErrorIrc(MysteryNet *net);
static void MysteryNet_Idle(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WirelessStart(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WirelessEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WirelessReady(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_BeaconStart(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_BeaconEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_BeaconWait(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_IrcStart(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_IrcEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_IrcWait(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WifiDownload(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WifiEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNetDownload_Wait(MysteryNetDownload *download, int *state, u32 nextState);
static void MysteryNetDownload_Cleanup(MysteryNetDownload *download, u32 error, int *state, u32 nextState,
                                       int failState);
static void MysteryNetSeq_Init(MysteryNetSeq *seq, void *work, MysteryNetSeqFunc func);
static void MysteryNetSeq_Exit(MysteryNetSeq *seq);
static void MysteryNetSeq_Main(MysteryNetSeq *seq);
static void MysteryNetSeq_SetNext(MysteryNetSeq *seq, MysteryNetSeqFunc func);
static BOOL MysteryNetSeq_IsCurrent(MysteryNetSeq *seq, MysteryNetSeqFunc func);
static void *MysteryNet_GetInfo(void *work);
static int MysteryNet_GetInfoSize(void *work);
static BOOL MysteryNet_IsSameInfo(u32 a, u32 b);
static void MysteryNet_OnExit(void *work);
static void MysteryNet_OnNdEvent(DWCNdCallbackReason reason, DWCNdError error, int serverError);
static void MysteryNet_SetNdDone(void);
static u32 Mystery_ParseHex(const char *str);
static BOOL Mystery_IsZero(const u32 *data, u32 size);
static u32 MysteryNet_OnDwcEvent(void *work, int code, int errorType, int error);

static const MysteryHexDigit sHexDigits[] = {
    { '0', 0 },  { '1', 1 },  { '2', 2 },  { '3', 3 },  { '4', 4 },  { '5', 5 },  { '6', 6 },   { '7', 7 },
    { '8', 8 },  { '9', 9 },  { 'A', 10 }, { 'B', 11 }, { 'C', 12 }, { 'D', 13 }, { 'E', 14 },  { 'F', 15 },
    { 'a', 10 }, { 'b', 11 }, { 'c', 12 }, { 'd', 13 }, { 'e', 14 }, { 'f', 15 }, { '\0', -1 },
};

static const GFLNetInitData sNetInit = {
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    MysteryNet_GetInfo,
    MysteryNet_GetInfoSize,
    MysteryNet_IsSameInfo,
    NULL,
    NULL,
    NULL,
    NULL,
    { 0 },
    NULL,
    NULL,
    0,
    { 1, 0, 0, 0, 0x80, 0x13, 0, 0 },
    HEAPID_USER,
    0xd,
    0xf,
    0xd,
    0xf0,
    0,
    1,
    0x58,
    4,
    1,
    0,
    GFL_NET_TYPE_WIRELESS,
    0,
    9,
    { 0x2c, 1, 0, 0 },
    0x1f4,
    0,
};

static MysteryNetNdState sNdState;

MysteryNet *MysteryNet_Create(SaveControl *save, HeapID heapId) {
    MysteryNet *net = GFL_HeapAllocate(heapId, sizeof(MysteryNet), FALSE, "mystery_net.c", 315);

    MI_CpuFill8(net, 0, sizeof(MysteryNet));
    net->heapId = heapId;
    net->save = save;
    MysteryNetSeq_Init(&net->seq, net, MysteryNet_Idle);
    return net;
}

void MysteryNet_Delete(MysteryNet *net) {
    MysteryNetSeq_Exit(&net->seq);
    GFL_HeapFree(net);
}

void MysteryNet_Main(MysteryNet *net) {
    MysteryNetSeq_Main(&net->seq);
}

void MysteryNet_ChangeState(MysteryNet *net, u32 state) {
    switch (state) {
    case MYSTERY_NET_STATE_WIRELESS_START:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_Idle)) {
            MysteryNetSeq_SetNext(&net->seq, MysteryNet_WirelessStart);
        }
        break;
    case MYSTERY_NET_STATE_WIRELESS_END:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessReady)) {
            MysteryNetSeq_SetNext(&net->seq, MysteryNet_WirelessEnd);
        }
        break;
    case MYSTERY_NET_STATE_WIFI:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_Idle)) {
            MysteryNetSeq_SetNext(&net->seq, MysteryNet_WifiDownload);
        }
        break;
    case MYSTERY_NET_STATE_WIFI_CANCEL:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WifiDownload)) {
            net->download.cancel = TRUE;
        }
        break;
    case MYSTERY_NET_STATE_WIFI_END:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_WifiEnd);
        break;
    case MYSTERY_NET_STATE_BEACON_START:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_BeaconStart);
        break;
    case MYSTERY_NET_STATE_BEACON_END:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_BeaconEnd);
        break;
    case MYSTERY_NET_STATE_IRC_START:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_IrcStart);
        break;
    case MYSTERY_NET_STATE_IRC_END:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_IrcEnd);
        break;
    case MYSTERY_NET_STATE_IDLE:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_Idle);
        break;
    }
}

u32 MysteryNet_GetState(MysteryNet *net) {
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_Idle)) {
        return MYSTERY_NET_STATE_IDLE;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessStart)) {
        return MYSTERY_NET_STATE_WIRELESS_START;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessReady)) {
        return MYSTERY_NET_STATE_WIRELESS_READY;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessEnd)) {
        return MYSTERY_NET_STATE_WIRELESS_END;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WifiDownload)) {
        return MYSTERY_NET_STATE_WIFI;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WifiEnd)) {
        return MYSTERY_NET_STATE_WIFI_END;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_BeaconStart)) {
        return MYSTERY_NET_STATE_BEACON_START;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_BeaconWait)) {
        return MYSTERY_NET_STATE_BEACON_WAIT;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_BeaconEnd)) {
        return MYSTERY_NET_STATE_BEACON_END;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_IrcStart)) {
        return MYSTERY_NET_STATE_IRC_START;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_IrcWait)) {
        return MYSTERY_NET_STATE_IRC_WAIT;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_IrcEnd)) {
        return MYSTERY_NET_STATE_IRC_END;
    }
    return MYSTERY_NET_STATE_IDLE;
}

u32 MysteryNet_GetBeaconFlags(MysteryNet *net) {
    return net->beaconFlags;
}

u32 MysteryNet_GetRecvData(MysteryNet *net, void *buffer, u32 size) {
    if (net->result == MYSTERY_NET_RECV_OK) {
        MI_CpuCopy8(net->buffer, buffer, size);
        return MYSTERY_NET_RECV_OK;
    }
    return net->result;
}

u32 MysteryNet_GetError(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetIsInitialized()) {
        switch (GFL_NetGetInitData()->bNetType) {
        case GFL_NET_TYPE_WIRELESS:
        case GFL_NET_TYPE_WIRELESS_LISTEN:
            error = MysteryNet_CheckErrorWireless(net);
            break;
        case GFL_NET_TYPE_WIFI:
            error = MysteryNet_CheckErrorWifi(net);
            break;
        case GFL_NET_TYPE_IRC:
            error = MysteryNet_CheckErrorIrc(net);
            break;
        }
    }
    return error;
}

void MysteryNet_ClearError(MysteryNet *net) {
    if (net->wifiWork != NULL) {
        WihDwc_Destroy(net->wifiWork);
        net->wifiWork = NULL;
    }
    MysteryNet_ChangeState(net, MYSTERY_NET_STATE_IDLE);
    GFL_NetErrClear();
}

static u32 MysteryNet_CheckErrorWifi(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetErrCheck()) {
        switch (NetState_GetErrorInfo()->errorType) {
        case DWC_ETYPE_LIGHT:
        case DWC_ETYPE_SHOW_ERROR:
        case DWC_ETYPE_SHUTDOWN_FM:
        case DWC_ETYPE_SHUTDOWN_GHTTP:
        case DWC_ETYPE_SHUTDOWN_ND:
        case DWC_ETYPE_DISCONNECT:
            if (net->download.initialized == TRUE) {
                return error;
            }
            NetState_ClearWifiError();
            DWC_ClearError();
            GFL_NetErrShutdown();
            GFL_NetErrShowScreen();
            GFL_NetErrClear();
            error = MYSTERY_NET_ERROR_FATAL;
            break;
        case DWC_ETYPE_FATAL:
            NetState_ClearWifiError();
            GFL_NetErrAbort();
            error = MYSTERY_NET_ERROR_ABORT;
            break;
        }
    }
    return error;
}

static u32 MysteryNet_CheckErrorWireless(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetErrCheck()) {
        GFL_NetErrShutdown();
        GFL_NetErrShowScreen();
        GFL_NetErrClear();
        error = MYSTERY_NET_ERROR_FATAL;
    }
    return error;
}

static u32 MysteryNet_CheckErrorIrc(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetErrCheck()) {
        GFL_NetErrShutdown();
        GFL_NetErrShowScreen();
        GFL_NetErrClear();
        error = MYSTERY_NET_ERROR_FATAL;
    }
    return error;
}

static void MysteryNet_Idle(MysteryNetSeq *seq, int *state, void *work) {
}

static void MysteryNet_WirelessStart(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case WIRELESS_START_STATE_INIT:
        GFL_NetInit(&sNetInit, NULL, net);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
        *state = WIRELESS_START_STATE_WAIT_INIT;
        break;
    case WIRELESS_START_STATE_WAIT_INIT:
        if (GFL_NetIsInitialized()) {
            GFL_NetSetIconScreen(TRUE, net->heapId);
            net->wifiWork = WihDwc_Create(4, net->heapId);
            GFL_NetStartScan();
            *state = WIRELESS_START_STATE_READY;
        }
        break;
    case WIRELESS_START_STATE_READY:
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
        MysteryNetSeq_SetNext(seq, MysteryNet_WirelessReady);
        break;
    }
}

static void MysteryNet_WirelessEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case WIRELESS_END_STATE_DESTROY_WORK:
        WihDwc_Destroy(net->wifiWork);
        net->wifiWork = NULL;
        *state = WIRELESS_END_STATE_EXIT_NET;
        break;
    case WIRELESS_END_STATE_EXIT_NET:
        if (GFL_NetExit(MysteryNet_OnExit)) {
            *state = WIRELESS_END_STATE_WAIT_EXIT;
        } else {
            *state = WIRELESS_END_STATE_IDLE;
        }
        break;
    case WIRELESS_END_STATE_WAIT_EXIT:
        if (net->exited) {
            net->exited = FALSE;
            *state = WIRELESS_END_STATE_IDLE;
        }
        break;
    case WIRELESS_END_STATE_IDLE:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNet_WirelessReady(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    WihDwc_TickTimers();
    net->beaconFlags = WihDwc_GetStatus(NULL);
}

static void MysteryNet_BeaconStart(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    DeliveryInit init;

    switch (*state) {
    case BEACON_START_STATE_INIT:
        MI_CpuFill8(&init, 0, sizeof(DeliveryInit));
        init.code = 9;
        init.data[0].datasize = sizeof(MysteryGiftRecvData);
        init.data[0].pData = net->buffer;
        init.data[0].region = region;
        init.data[0].mask = 1 << GAME_VERSION;
        init.dataNum = 1;
        init.flag4 = 0;
        init.heapId = net->heapId;
        net->beacon = DeliveryBeacon_Init(&init);
        DeliveryBeacon_RecvStart(net->beacon);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
        net->result = MYSTERY_NET_RECV_NONE;
        *state = BEACON_START_STATE_WAIT_INIT;
        break;
    case BEACON_START_STATE_WAIT_INIT:
        if (GFL_NetIsInitialized()) {
            GFL_NetSetIconScreen(TRUE, net->heapId);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
            *state = BEACON_START_STATE_READY;
        }
        break;
    case BEACON_START_STATE_READY:
        MysteryNetSeq_SetNext(seq, MysteryNet_BeaconWait);
        break;
    }
}

static void MysteryNet_BeaconEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case BEACON_END_STATE_END:
        DeliveryBeacon_End(net->beacon);
        *state = BEACON_END_STATE_WAIT_END;
        break;
    case BEACON_END_STATE_WAIT_END:
        if (GFL_NetIsNotInitialized()) {
            *state = BEACON_END_STATE_IDLE;
        }
        break;
    case BEACON_END_STATE_IDLE:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNet_BeaconWait(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    DeliveryBeacon_Main(net->beacon);
    if (DeliveryBeacon_IsReceiveDone(net->beacon)) {
        net->result = MYSTERY_NET_RECV_OK;
        MysteryNetSeq_SetNext(seq, MysteryNet_BeaconEnd);
    }
}

static void MysteryNet_IrcStart(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    DeliveryIrcInit init;

    switch (*state) {
    case IRC_START_STATE_INIT:
        MI_CpuFill8(&init, 0, sizeof(DeliveryIrcInit));
        init.code = 9;
        init.data[0].datasize = sizeof(MysteryGiftRecvData);
        init.data[0].pData = net->buffer;
        init.unk04 = 0;
        init.heapId = net->heapId;
        init.data[0].region = region;
        init.dataNum = 1;
        init.data[0].mask = 1 << GAME_VERSION;
        net->irc = DeliveryIrc_Init(&init);
        DeliveryIrc_RecvStart(net->irc);
        net->result = MYSTERY_NET_RECV_NONE;
        *state = IRC_START_STATE_WAIT_INIT;
        break;
    case IRC_START_STATE_WAIT_INIT:
        if (GFL_NetIsInitialized()) {
            GFL_NetSetIconScreen(TRUE, net->heapId);
            *state = IRC_START_STATE_READY;
        }
        break;
    case IRC_START_STATE_READY:
        MysteryNetSeq_SetNext(seq, MysteryNet_IrcWait);
        break;
    }
}

static void MysteryNet_IrcEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case IRC_END_STATE_END:
        DeliveryIrc_End(net->irc);
        *state = IRC_END_STATE_WAIT_END;
        break;
    case IRC_END_STATE_WAIT_END:
        if (GFL_NetIsNotInitialized()) {
            *state = IRC_END_STATE_IDLE;
        }
        break;
    case IRC_END_STATE_IDLE:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNet_IrcWait(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    u8 ret;

    DeliveryIrc_Main(net->irc);
    ret = DeliveryIrc_GetResult(net->irc);
    if (ret != DELIVERY_IRC_RESULT_RUNNING) {
        switch (ret) {
        case DELIVERY_IRC_RESULT_SUCCESS:
            net->result = MYSTERY_NET_RECV_OK;
            break;
        case DELIVERY_IRC_RESULT_DAMAGED:
        case DELIVERY_IRC_RESULT_NO_DATA:
            net->result = MYSTERY_NET_RECV_ERROR;
            break;
        }
        MysteryNetSeq_SetNext(seq, MysteryNet_IrcEnd);
    }
}

static void MysteryNet_WifiDownload(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    MysteryNetDownload *download = &net->download;
    int i;

    switch (*state) {
    case DOWNLOAD_STATE_INIT:
        DWCRap_SetEventFunc(MysteryNet_OnDwcEvent, download);
        MI_CpuFill8(download, 0, sizeof(MysteryNetDownload));
        download->cancel = FALSE;
        download->cancelled = FALSE;
        download->selected = 0;
        download->buffer = net->buffer;
        net->result = MYSTERY_NET_RECV_NONE;
        if (!DWC_NdInitAsync(MysteryNet_OnNdEvent, "IRAO", "WX9x7Zh6J3aBC4zQ")) {
            net->result = MYSTERY_NET_RECV_ERROR;
            *state = DOWNLOAD_STATE_END;
            return;
        }
        download->initialized = TRUE;
        MysteryNetDownload_Wait(download, state, DOWNLOAD_STATE_SET_ATTR);
        break;
    case DOWNLOAD_STATE_SET_ATTR:
        if (download->cancelled) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, DOWNLOAD_STATE_CANCEL, DOWNLOAD_STATE_CANCEL);
            return;
        }
        if (!DWC_NdSetAttr("MYSTERY_E", "", "")) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, DOWNLOAD_STATE_END, DOWNLOAD_STATE_END);
            return;
        }
        *state = DOWNLOAD_STATE_GET_FILE_LIST;
        break;
    case DOWNLOAD_STATE_GET_FILE_LIST:
        if (!DWC_NdGetFileListAsync(download->files, 0, MYSTERY_NET_FILE_MAX)) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, DOWNLOAD_STATE_END, DOWNLOAD_STATE_END);
            return;
        }
        MysteryNetDownload_Wait(download, state, DOWNLOAD_STATE_FILTER_FILES);
        break;
    case DOWNLOAD_STATE_FILTER_FILES:
        for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
            if (!Mystery_IsZero((u32 *)&download->files[i], sizeof(DWCNdFileInfo))) {
                if (Mystery_ParseHex(download->files[i].param2) & (1 << GAME_VERSION)) {
                    download->available[i] = TRUE;
                } else {
                    download->available[i] = FALSE;
                }
            } else {
                download->available[i] = FALSE;
            }
        }
        download->count = 0;
        for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
            if (download->available[i]) {
                download->count++;
            }
        }
        *state = DOWNLOAD_STATE_SELECT_FILE;
        break;
    case DOWNLOAD_STATE_SELECT_FILE:
        if (download->count == 1) {
            for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
                if (download->available[i]) {
                    download->selected = i;
                    break;
                }
            }
        } else if (download->count > 1) {
            u32 n = 0;
            u32 pick = GFL_RandomMTRange(download->count);

            for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
                if (download->available[i]) {
                    if (n == pick) {
                        download->selected = i;
                        break;
                    }
                    n++;
                }
            }
            download->selected = i;
        } else {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 1, state, DOWNLOAD_STATE_CLEANUP, DOWNLOAD_STATE_CLEANUP);
            return;
        }
        *state = DOWNLOAD_STATE_GET_FILE;
        break;
    case DOWNLOAD_STATE_GET_FILE:
        if (!DWC_NdGetFileAsync(&download->files[download->selected], download->buffer, MYSTERY_NET_BUFFER_SIZE)) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, DOWNLOAD_STATE_END, DOWNLOAD_STATE_END);
            return;
        }
        download->percent = 0;
        sNdState.ready = FALSE;
        sNdState.error = 0;
        download->timeout = 0;
        *state = DOWNLOAD_STATE_WAIT_FILE;
        break;
    case DOWNLOAD_STATE_WAIT_FILE:
        NdReportResults();
        if (download->timeout++ > 7200) {
            NetState_RecordError(0, 0, 0, 1013);
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, DOWNLOAD_STATE_CANCEL, DOWNLOAD_STATE_CANCEL);
        }
        if (!sNdState.ready) {
            if (download->cancel) {
                net->result = MYSTERY_NET_RECV_ERROR;
                MysteryNetDownload_Cleanup(download, 3, state, DOWNLOAD_STATE_CANCEL, DOWNLOAD_STATE_CANCEL);
                return;
            }
            if (DWC_NdGetProgress(&download->received, &download->contentLength) == TRUE) {
                u32 percent = download->received * 100 / download->contentLength;

                if (download->percent != percent) {
                    download->percent = percent;
                }
            }
        } else if (sNdState.error != 0) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, DOWNLOAD_STATE_END, DOWNLOAD_STATE_END);
        } else if (!download->cancelled) {
            net->result = MYSTERY_NET_RECV_OK;
            MysteryNetDownload_Cleanup(download, 1, state, DOWNLOAD_STATE_CLEANUP, DOWNLOAD_STATE_CLEANUP);
        } else {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, DOWNLOAD_STATE_CLEANUP, DOWNLOAD_STATE_CLEANUP);
        }
        break;
    case DOWNLOAD_STATE_CANCEL:
        DWC_NdCancelAsync();
        *state = DOWNLOAD_STATE_CLEANUP;
        break;
    case DOWNLOAD_STATE_CLEANUP:
        *state = DOWNLOAD_STATE_END;
        break;
    case DOWNLOAD_STATE_END:
        download->initialized = FALSE;
        DWCRap_SetEventFunc(NULL, NULL);
        MysteryNetSeq_SetNext(seq, MysteryNet_WifiEnd);
        break;
    case DOWNLOAD_WAIT:
        NdReportResults();
        if (download->timeout++ > 7200) {
            NetState_RecordError(0, 0, 0, 1013);
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, DOWNLOAD_STATE_END, DOWNLOAD_STATE_END);
        }
        if (sNdState.ready) {
            sNdState.ready = FALSE;
            if (sNdState.error != 0) {
                net->result = MYSTERY_NET_RECV_ERROR;
                MysteryNetDownload_Cleanup(download, 3, state, DOWNLOAD_STATE_END, DOWNLOAD_STATE_END);
                return;
            }
            *state = download->nextState;
            return;
        }
        if (download->cancel) {
            download->cancelled = TRUE;
        }
        break;
    case DOWNLOAD_WAIT_CLEANUP:
        NdReportResults();
        if (download->timeout++ > 7200) {
            NetState_RecordError(0, 0, 0, 1013);
            *state = download->nextState;
        }
        if (sNdState.done) {
            sNdState.done = FALSE;
            *state = download->nextState;
            return;
        }
        if (download->cancel) {
            download->cancelled = TRUE;
        }
        break;
    }
}

static void MysteryNet_WifiEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case WIFI_END_STATE_EXIT_NET:
        if (GFL_NetExit(NULL)) {
            *state = WIFI_END_STATE_WAIT_EXIT;
        } else {
            *state = WIFI_END_STATE_IDLE;
        }
        break;
    case WIFI_END_STATE_WAIT_EXIT:
        if (!GFL_NetIsInitialized()) {
            net->exited = FALSE;
            *state = WIFI_END_STATE_IDLE;
        }
        break;
    case WIFI_END_STATE_IDLE:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNetDownload_Wait(MysteryNetDownload *download, int *state, u32 nextState) {
    sNdState.ready = FALSE;
    sNdState.error = 0;
    download->nextState = nextState;
    download->timeout = 0;
    *state = DOWNLOAD_WAIT;
}

// Ends the download library, then goes on to nextState, or to failState at once if it can't
static void MysteryNetDownload_Cleanup(MysteryNetDownload *download, u32 error, int *state, u32 nextState,
                                       int failState) {
    sNdState.done = FALSE;
    sNdState.error = error;
    download->nextState = nextState;
    download->timeout = 0;
    *state = DOWNLOAD_WAIT_CLEANUP;
    if (!DWC_NdCleanupAsync()) {
        *state = failState;
    }
}

static void MysteryNetSeq_Init(MysteryNetSeq *seq, void *work, MysteryNetSeqFunc func) {
    MI_CpuFill8(seq, 0, sizeof(MysteryNetSeq));
    seq->work = work;
    MysteryNetSeq_SetNext(seq, func);
}

static void MysteryNetSeq_Exit(MysteryNetSeq *seq) {
    MI_CpuFill8(seq, 0, sizeof(MysteryNetSeq));
}

static void MysteryNetSeq_Main(MysteryNetSeq *seq) {
    if (!seq->end) {
        seq->func(seq, &seq->state, seq->work);
    }
}

static void MysteryNetSeq_SetNext(MysteryNetSeq *seq, MysteryNetSeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

static BOOL MysteryNetSeq_IsCurrent(MysteryNetSeq *seq, MysteryNetSeqFunc func) {
    if (seq->func == func) {
        return TRUE;
    }
    return FALSE;
}

static void *MysteryNet_GetInfo(void *work) {
    MysteryNet *net = work;

    return net->info;
}

static int MysteryNet_GetInfoSize(void *work) {
    return sizeof(((MysteryNet *)NULL)->info);
}

static BOOL MysteryNet_IsSameInfo(u32 a, u32 b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

static void MysteryNet_OnExit(void *work) {
    MysteryNet *net = work;

    net->exited = TRUE;
}

static void MysteryNet_OnNdEvent(DWCNdCallbackReason reason, DWCNdError error, int serverError) {
    sNdState.ready = TRUE;
    sNdState.error = error;
    MysteryNet_SetNdDone();
}

static void MysteryNet_SetNdDone(void) {
    sNdState.done = TRUE;
}

static u32 Mystery_ParseHex(const char *str) {
    u32 i;
    s32 digit;
    u32 value = 0;

    while (TRUE) {
        digit = -1;
        for (i = 0; i < NELEMS(sHexDigits); i++) {
            if (str[0] == sHexDigits[i].c) {
                digit = sHexDigits[i].value;
                break;
            }
        }
        if (digit == -1) {
            break;
        }
        value <<= 4;
        value += digit;
        str++;
    }
    return value;
}

static BOOL Mystery_IsZero(const u32 *data, u32 size) {
    u32 i;

    for (i = 0; i < size / 4; i++) {
        if (data[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 MysteryNet_OnDwcEvent(void *work, int code, int errorType, int error) {
    MysteryNetDownload *download = work;

    switch (errorType) {
    case DWC_ETYPE_SHUTDOWN_FM:
    case DWC_ETYPE_SHUTDOWN_ND:
    case DWC_ETYPE_DISCONNECT:
        return download->initialized;
    }
    return 0;
}
