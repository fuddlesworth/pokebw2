// NitroWiFi's WCM, its backend for the DSi's wireless (mode 2), in the LTD autoload's .ltdmain: the steps that
// wcm_system.c's calls take through TwlSDK's NWM where the DS's take them through WM, and NWM's callbacks, which move
// WCM's state machine and notify WCM's user. It mirrors wcm_wmif.c, the WM backend, step for step. The ROM names none
// of it: the file's name is descriptive (TwlSDK's libwcm calls the object nwmif.o), and the functions have the names of
// the TwlWiFi symbols that Black 1's decompilation (aletheia) recovered from that object, in the same order.
// WCMi_NwmifSetDCFData and its callbacks keep the DCF of the WM backend's names, though NWM sends a frame.
#include "nitrowifi/wcm_dsi.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"
#include "twl/nwm.h"
#include "wcm_internal.h"

// The index of the command 8's connection, from the access point's capability bits
static u8 sConnectIndex;

static void WcmNwmReset(void);
static void WcmNwmcbCommon(void *arg);
static void WcmNwmcbScanEx(void *arg);
static void WcmNwmcbConnect(void *arg);
static void WcmNwmcbDisconnect(void *arg);
static void WcmNwmcbSetRecvFrmBuf(void *arg);
static void WcmNwmcbUnsetRecvFrmBuf(void *arg);
static void WcmEditScanExParam(const u8 *bssid, const u8 *essid, u32 option);
static void WcmNwmcbSetDCFData(void *arg);
static void WcmNwmcbSetDCFDataAsync(void *arg);

// clang-format off
// TwlSDK's section for the DSi-only code that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on
#pragma section LTDMAIN begin

// The startup's first step: sets NWM up in the work and loads the device
s32 WCMi_NwmifStartupAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (NWM_Init(work->nwmSystemBuffer, 0x3000, work->dmaNo)) {
    case NWM_RETCODE_SUCCESS:
        break;
    case NWM_RETCODE_ILLEGAL_STATE:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    case NWM_RETCODE_NWM_DISABLE:
        return 5;
    case NWM_RETCODE_INVALID_PARAM:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    work->nwmConnectionFailureReason = 0;
    switch (NWM_LoadDevice(WcmNwmcbCommon)) {
    case NWM_RETCODE_OPERATING:
        WCMi_SetPhase(WCM_PHASE_WAIT_TO_IDLE);
        work->pendingNotify = WCM_NOTIFY_STARTUP;
        break;
    case NWM_RETCODE_FIFO_ERROR:
        WCMi_SetPhase(WCM_PHASE_IRREGULAR);
        return 1;
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    return 3;
}

s32 WCMi_NwmifCleanupAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (NWM_Close(WcmNwmcbCommon)) {
    case NWM_RETCODE_OPERATING:
        WCMi_SetPhase(WCM_PHASE_IDLE_TO_WAIT);
        work->pendingNotify = WCM_NOTIFY_CLEANUP;
        break;
    case NWM_RETCODE_FIFO_ERROR:
        return 4;
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    return 3;
}

s32 WCMi_NwmifBeginSearchAsync(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (work->state) {
    case WCM_PHASE_IDLE_TO_SEARCH:
        WcmEditScanExParam(bssid, essid, option);
        return 2;
    case WCM_PHASE_SEARCH:
        WcmEditScanExParam(bssid, essid, option);
        return 0;
    case WCM_PHASE_IDLE:
        break;
    default:
        return 1;
    }
    WcmEditScanExParam(bssid, essid, option);
    DC_InvalidateRange(work->nwmScanRequest.scanBuf, work->nwmScanRequest.scanBufSize);
    work->nwmScanCount++;
    switch (NWM_StartScanEx(WcmNwmcbScanEx, &work->nwmScanRequest)) {
    case NWM_RETCODE_OPERATING:
        WCMi_SetPhase(WCM_PHASE_IDLE_TO_SEARCH);
        work->pendingNotify = WCM_NOTIFY_BEGIN_SEARCH;
        break;
    case NWM_RETCODE_FIFO_ERROR:
        return 4;
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    return 3;
}

s32 WCMi_NwmifConnectAsync(void) {
    NWMWpaParam pmk;
    WCMWork *work = WCMi_GetSystemWork();
    s32 result;

    switch (work->connectionStorage.mode) {
    case WCM_PRIVACYMODE_NONE:
    case WCM_PRIVACYMODE_40:
    case WCM_PRIVACYMODE_104:
    case WCM_PRIVACYMODE_128: {
        u16 authMode = 0; // Open system, or 1 for a shared key

        if ((work->searchOption & WCM_OPTION_AUTH_SHAREDKEY) == WCM_OPTION_AUTH_SHAREDKEY) {
            authMode = 1;
        }
        result = NWM_SetWEPKey(WcmNwmcbCommon, work->connectionStorage.mode, work->connectionStorage.argument,
                               work->connectionStorage.data, authMode);
        break;
    }
    case WCM_PRIVACYMODE_WPA_TKIP:
    case WCM_PRIVACYMODE_WPA2_TKIP:
    case WCM_PRIVACYMODE_WPA_AES:
    case WCM_PRIVACYMODE_WPA2_AES:
        MI_CpuCopy8(work->connectionStorage.data, pmk.psk, sizeof(pmk.psk));
        pmk.auth = work->connectionStorage.mode;
        result = NWM_SetWPAPSK(WcmNwmcbCommon, &pmk);
        break;
    }
    switch (result) {
    case NWM_RETCODE_OPERATING:
        WCMi_SetPhase(WCM_PHASE_IDLE_TO_DCF);
        work->pendingNotify = WCM_NOTIFY_CONNECT;
        break;
    case NWM_RETCODE_FIFO_ERROR:
        return 4;
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    return 3;
}

s32 WCMi_NwmifDisconnectAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (NWM_UnsetReceivingFrameBuffer(WcmNwmcbUnsetRecvFrmBuf, 0)) {
    case NWM_RETCODE_OPERATING:
        WCMi_SetPhase(WCM_PHASE_DCF_TO_IDLE);
        work->pendingNotify = WCM_NOTIFY_DISCONNECT;
        break;
    case NWM_RETCODE_FIFO_ERROR:
        return 4;
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    return 3;
}

// Brings NWM back down from the state it is in, one command at a time
s32 WCMi_NwmifTerminateAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();
    s32 result;

    switch (NWM_GetState()) {
    case NWM_STATE_INITIALIZED:
        result = NWM_End();
        if (result == 0) {
            WCMi_SetPhase(WCM_PHASE_WAIT);
            work->pendingNotify = WCM_NOTIFY_COMMON;
            return 0;
        }
        break;
    case NWM_STATE_LOADED:
        result = NWM_UnloadDevice(WcmNwmcbCommon);
        break;
    case NWM_STATE_DISCONNECTED:
        result = NWM_Close(WcmNwmcbCommon);
        break;
    case NWM_STATE_INFRA_CONNECTED:
        result = NWM_Disconnect(WcmNwmcbDisconnect);
        break;
    default:
        work->resetPending = 1;
        result = 4;
        break;
    }
    switch (result) {
    case NWM_RETCODE_OPERATING:
        WCMi_SetPhase(WCM_PHASE_TERMINATING);
        work->pendingNotify = WCM_NOTIFY_TERMINATE;
        break;
    case NWM_RETCODE_FIFO_ERROR:
        return 4;
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        return 7;
    }
    return 3;
}

s32 WCMi_NwmifSetDCFData(const u8 *dst, const void *buffer, u16 byteLength, void (*completion)(s32 result)) {
    WCMCpsifWork *global = WCMi_GetCpsifWork();
    NWMCallbackFunc callback;

    if (completion == NULL) {
        callback = WcmNwmcbSetDCFData;
    } else {
        global->sendCompletion = completion;
        callback = WcmNwmcbSetDCFDataAsync;
    }
    switch (NWM_SendFrame(callback, dst, buffer, byteLength)) {
    case NWM_RETCODE_OPERATING:
        break;
    case NWM_RETCODE_ILLEGAL_STATE:
    case NWM_RETCODE_INVALID_PARAM:
    case NWM_RETCODE_FIFO_ERROR:
    default:
        return -5;
    }
    if (completion == NULL) {
        OS_SleepThread(&global->sendQueue);
        switch (global->sendStatus) {
        case NWM_RETCODE_SUCCESS:
            break;
        case NWM_RETCODE_FAILED:
        default:
            return -5;
        }
    }
    return byteLength;
}

// The channels that the DSi's wireless allows, from bit 0 for channel 1
s32 WCMi_NwmifGetAllowedChannel(void) {
    return (u16)(NWM_GetAllowedChannel() >> 1);
}

// What wcm_wmif.c's reset request and its completion do, at once, since NWM has no reset command: ends the operation of
// the state WCM is in
static void WcmNwmReset(void) {
    WCMWork *work = WCMi_GetSystemWork();

    if (work->resetPending == 0) {
        // The reset is requested and completes here
        work->resetPending = 1;
        work->resetPending = 0;
        work->nwmAssociationId = 0;
        switch (work->state) {
        case WCM_PHASE_IDLE_TO_SEARCH:
        case WCM_PHASE_SEARCH:
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_FAILURE, NULL, 0, 0x29b);
            break;
        case WCM_PHASE_SEARCH_TO_IDLE:
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_SUCCESS, NULL, 0, 0x2a1);
            break;
        case WCM_PHASE_IDLE_TO_DCF: {
            u16 reason = work->nwmConnectionFailureReason;

            work->nwmConnectionFailureReason = 0;
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_FAILURE, &work->connectBss, reason, 0x2ab);
            break;
        }
        case WCM_PHASE_DCF:
        case WCM_PHASE_IRREGULAR:
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_SUCCESS, &work->connectBss, 1, 0x2b3);
            break;
        case WCM_PHASE_DCF_TO_IDLE:
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_SUCCESS, &work->connectBss, 0, 0x2ba);
            break;
        case WCM_PHASE_TERMINATING:
            switch (NWM_Close(WcmNwmcbCommon)) {
            case NWM_RETCODE_OPERATING:
                break;
            case NWM_RETCODE_FIFO_ERROR:
                WCMi_SetPhase(WCM_PHASE_IRREGULAR);
                WCMi_Notify(WCM_RESULT_FAILURE, NULL, 0, 0x2c8);
                break;
            case NWM_RETCODE_ILLEGAL_STATE:
            default:
                WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
                WCMi_Notify(WCM_RESULT_FATAL_ERROR, NULL, 0, 0x2d1);
                break;
            }
            break;
        default:
            WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
            WCMi_Notify(WCM_RESULT_FATAL_ERROR, NULL, work->state, 0x2d7);
            break;
        }
    }
}

// The callback of NWM's device and connection commands, which takes the startup, the cleanup and the connection on
// to their next command
static void WcmNwmcbCommon(void *arg) {
    const NWMCallback *event = arg;
    s32 result = 13;
    WCMWork *work = WCMi_GetSystemWork();

    switch (event->retcode) {
    case NWM_RETCODE_SUCCESS:
        switch (event->apiid) {
        case NWM_APIID_LOAD_DEVICE:
            result = NWM_Open(WcmNwmcbCommon);
            break;
        case NWM_APIID_UNLOAD_DEVICE:
            result = NWM_End();
            switch (result) {
            case NWM_RETCODE_SUCCESS:
                WCMi_SetPhase(WCM_PHASE_WAIT);
                WCMi_Notify(WCM_RESULT_SUCCESS, NULL, 0, 0x300);
                break;
            case NWM_RETCODE_NWM_DISABLE:
            default:
                WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
                WCMi_Notify(WCM_RESULT_FATAL_ERROR, NULL, 0, 0x309);
                break;
            }
            return;
        case NWM_APIID_OPEN: {
            u8 mac[6];

            OS_GetMacAddress(mac);
            WCMi_CpsifSetMacAddress(mac);
        }
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_SUCCESS, NULL, 0, 0x316);
            return;
        case NWM_APIID_CLOSE:
            result = NWM_UnloadDevice(WcmNwmcbCommon);
            break;
        case NWM_APIID_SET_WEPKEY:
        case NWM_APIID_SET_WPA_PSK:
            result = NWM_SetPowerSaveMode(WcmNwmcbCommon,
                                          (work->searchOption & WCM_OPTION_POWER_ACTIVE) == WCM_OPTION_POWER_ACTIVE
                                              ? NWM_POWERMODE_ACTIVE
                                              : NWM_POWERMODE_STANDARD);
            break;
        case NWM_APIID_SET_PS_MODE:
            result =
                NWM_SetReceivingFrameBuffer(WcmNwmcbSetRecvFrmBuf, work->ioBuffer.nwm, sizeof(work->ioBuffer.nwm), 0);
            break;
        }
        switch (result) {
        case NWM_RETCODE_OPERATING:
            break;
        case NWM_RETCODE_FIFO_ERROR:
            WCMi_SetPhase(WCM_PHASE_IRREGULAR);
            WCMi_Notify(WCM_RESULT_FAILURE, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0,
                        0x34a);
            break;
        case NWM_RETCODE_ILLEGAL_STATE:
        default:
            WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
            WCMi_Notify(WCM_RESULT_FATAL_ERROR, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0,
                        0x353);
            break;
        }
        break;
    case NWM_RETCODE_FAILED:
        WCMi_SetPhase(WCM_PHASE_IRREGULAR);
        WCMi_Notify(WCM_RESULT_FAILURE, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x372);
        break;
    case NWM_RETCODE_SDIO_ERROR:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        WCMi_Notify(WCM_RESULT_FATAL_ERROR, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0,
                    0x378);
        break;
    case NWM_RETCODE_ILLEGAL_STATE:
    case NWM_RETCODE_NWM_DISABLE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        WCMi_Notify(WCM_RESULT_FATAL_ERROR, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0,
                    0x385);
        break;
    }
}

// The scan's callback: reports the access points found and scans the next channel, until the search stops
static void WcmNwmcbScanEx(void *arg) {
    const NWMStartScanCallback *event = arg;
    s32 result = 13;
    WCMWork *work = WCMi_GetSystemWork();

    switch (event->header.retcode) {
    case NWM_RETCODE_SUCCESS:
        if (work->state == WCM_PHASE_IDLE_TO_SEARCH) {
            WCMi_SetPhase(WCM_PHASE_SEARCH);
            WCMi_Notify(WCM_RESULT_SUCCESS, NULL, 0, 0x3a3);
        }
        switch (work->state) {
        case WCM_PHASE_SEARCH:
            work->pendingNotify = WCM_NOTIFY_FOUND_AP;
            if (event->bssDescCount != 0) {
                int i;

                DC_InvalidateRange(work->nwmScanRequest.scanBuf, work->nwmScanRequest.scanBufSize);
                for (i = 0; i < event->bssDescCount; i++) {
                    WCMi_EntryApList((const WMBssDesc *)event->bssDesc[i], event->linkLevel[i]);
                    WCMi_NotifyEx(WCM_NOTIFY_FOUND_AP, WCM_RESULT_SUCCESS, (s32)event->bssDesc[i], arg, 0x3b9);
                }
            }
            if ((work->searchOption & 0xc00000) == 0xc00000) {
                u32 count = MATH_CountPopulation(work->searchOption & 0x3ffe);

                if (count > 0) {
                    if (work->nwmScanCount % count == 0) {
                        WCMi_NotifyEx(WCM_NOTIFY_SEARCH_AROUND, WCM_RESULT_SUCCESS, work->nwmScanCount, NULL, 0x3c7);
                    }
                }
            }
            work->nwmScanRequest.channelList =
                (u16)((1 << WCMi_GetNextScanChannel((u16)(32 - MATH_CountLeadingZerosFunc(event->channelList)))) >> 1);
            DC_StoreRange(work->nwmScanRequest.scanBuf, work->nwmScanRequest.scanBufSize);
            work->nwmScanCount++;
            result = NWM_StartScanEx(WcmNwmcbScanEx, &work->nwmScanRequest);
            break;
        case WCM_PHASE_SEARCH_TO_IDLE:
            result = 2;
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_SUCCESS, NULL, 0, 0x3d7);
            break;
        case WCM_PHASE_TERMINATING:
            WcmNwmReset();
            return;
        }
        switch (result) {
        case NWM_RETCODE_OPERATING:
            break;
        case NWM_RETCODE_FIFO_ERROR:
            WCMi_SetPhase(WCM_PHASE_IRREGULAR);
            WCMi_Notify(WCM_RESULT_FAILURE, NULL, 0, 0x3f3);
            break;
        case NWM_RETCODE_ILLEGAL_STATE:
        default:
            WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
            WCMi_Notify(WCM_RESULT_FATAL_ERROR, NULL, 0, 0x3fc);
            break;
        }
        break;
    case NWM_RETCODE_FAILED:
        WcmNwmReset();
        break;
    case NWM_RETCODE_ILLEGAL_STATE:
    case NWM_RETCODE_NWM_DISABLE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        WCMi_Notify(WCM_RESULT_FATAL_ERROR, NULL, 0, 0x40d);
        break;
    }
}

// The callback of the command 8: the connection made, or why it failed
static void WcmNwmcbConnect(void *arg) {
    const NWMConnectCallback *event = arg;
    WCMWork *work = WCMi_GetSystemWork();
    u16 nwmState = NWM_GetState();

    switch (event->header.retcode) {
    case NWM_RETCODE_SUCCESS:
        if (nwmState == NWM_STATE_INFRA_CONNECTED) {
            if (work->state == WCM_PHASE_IRREGULAR) {
                WCMi_SetPhase(WCM_PHASE_IDLE_TO_DCF);
                WcmNwmReset();
                return;
            }
            work->nwmAssociationId = event->aid;
            WCMi_SetPhase(WCM_PHASE_DCF);
            WCMi_Notify(WCM_RESULT_SUCCESS, &work->connectBss, 0, 0x436);
        } else {
            WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
            WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, 0, 0x43c);
        }
        break;
    case NWM_RETCODE_FAILED:
        switch (event->reason) {
        case 1:
        case 2:
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
            switch (work->state) {
            case WCM_PHASE_DCF:
            case WCM_PHASE_DCF_TO_IDLE:
                work->pendingNotify = WCM_NOTIFY_DISCONNECT;
            case WCM_PHASE_IDLE_TO_DCF:
                work->nwmAssociationId = 0;
                switch (event->reason) {
                case 5:
                    work->nwmConnectionFailureReason = 13;
                    break;
                case 10:
                case 11:
                    work->nwmConnectionFailureReason = 15;
                    break;
                case 7:
                    work->nwmConnectionFailureReason = 17;
                    break;
                }
            case WCM_PHASE_IRREGULAR:
                WcmNwmReset();
                break;
            }
            break;
        case 3:
            if (work->state == WCM_PHASE_IDLE_TO_DCF) {
                work->nwmAssociationId = 0;
                WcmNwmReset();
            }
            break;
        default:
            WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
            WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, event->reason, 0x481);
            break;
        }
        break;
    case NWM_RETCODE_INVALID_PARAM:
        WCMi_SetPhase(WCM_PHASE_IDLE_TO_DCF);
        WcmNwmReset();
        break;
    case NWM_RETCODE_ILLEGAL_STATE:
    case NWM_RETCODE_NWM_DISABLE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, 0, 0x495);
        break;
    }
}

// The disconnection's callback
static void WcmNwmcbDisconnect(void *arg) {
    const NWMCallback *event = arg;
    WCMWork *work = WCMi_GetSystemWork();

    switch (event->retcode) {
    case NWM_RETCODE_SUCCESS:
        if (work->state == WCM_PHASE_IRREGULAR) {
            WCMi_SetPhase(WCM_PHASE_DCF_TO_IDLE);
            WcmNwmReset();
        } else if (work->state == WCM_PHASE_TERMINATING) {
            WcmNwmReset();
        } else {
            work->nwmAssociationId = 0;
            WCMi_SetPhase(WCM_PHASE_IDLE);
            WCMi_Notify(WCM_RESULT_SUCCESS, &work->connectBss, 0, 0x4bd);
        }
        break;
    case NWM_RETCODE_FAILED:
    case NWM_RETCODE_ILLEGAL_STATE:
        WCMi_SetPhase(WCM_PHASE_DCF_TO_IDLE);
        WcmNwmReset();
        break;
    case NWM_RETCODE_NWM_DISABLE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, 0, 0x4cf);
        break;
    }
}

// The callback of the receive buffer: once it is set, connects to the access point; then hands WCM each frame received
static void WcmNwmcbSetRecvFrmBuf(void *arg) {
    const NWMReceivingFrameCallback *event = arg;
    WCMWork *work = WCMi_GetSystemWork();

    switch (event->header.retcode) {
    case NWM_RETCODE_SUCCESS:
        if (work->state == WCM_PHASE_IRREGULAR) {
            WCMi_SetPhase(WCM_PHASE_IDLE_TO_DCF);
            WcmNwmReset();
            return;
        }
        if (work->connectBss.capaInfo & 2) {
            sConnectIndex = 1;
        } else {
            sConnectIndex = 0;
        }
        switch (NWM_Connect(WcmNwmcbConnect, sConnectIndex, (const NWMBssDesc *)&work->connectBss)) {
        case NWM_RETCODE_OPERATING:
            break;
        case NWM_RETCODE_FIFO_ERROR:
            WCMi_SetPhase(WCM_PHASE_IRREGULAR);
            WCMi_Notify(WCM_RESULT_FAILURE, &work->connectBss, 0, 0x506);
            break;
        case NWM_RETCODE_ILLEGAL_STATE:
        default:
            WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
            WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, 0, 0x50f);
            break;
        }
        break;
    case NWM_RETCODE_RECV_IND: {
        s32 rssi;

        DC_InvalidateRange(event->recvBuf, 0x620);
        rssi = event->rssi;
        if (rssi < 0) {
            rssi = 0;
        }
        if (rssi > 0x100) {
            rssi = 0xff;
        }
        WCMi_ShelterRssi((u8)rssi);
        WCMi_CpsifRecvCallback(event->recvBuf->sa, event->recvBuf->da, event->recvBuf->frame, event->length - 14);
        break;
    }
    }
}

// The callback of the command 0xc, which readies the disconnection: disconnects
static void WcmNwmcbUnsetRecvFrmBuf(void *arg) {
    const NWMCallback *event = arg;
    WCMWork *work = WCMi_GetSystemWork();

    switch (event->retcode) {
    case NWM_RETCODE_SUCCESS:
        if (work->state == WCM_PHASE_IRREGULAR) {
            WCMi_SetPhase(WCM_PHASE_DCF_TO_IDLE);
            WcmNwmReset();
        } else {
            switch (NWM_Disconnect(WcmNwmcbDisconnect)) {
            case NWM_RETCODE_OPERATING:
                WCMi_SetPhase(WCM_PHASE_DCF_TO_IDLE);
                work->pendingNotify = WCM_NOTIFY_DISCONNECT;
                break;
            case NWM_RETCODE_FIFO_ERROR:
                WCMi_SetPhase(WCM_PHASE_IRREGULAR);
                WCMi_Notify(WCM_RESULT_FAILURE, &work->connectBss, 0, 0x555);
                break;
            case NWM_RETCODE_ILLEGAL_STATE:
                WCMi_SetPhase(WCM_PHASE_DCF_TO_IDLE);
                WcmNwmReset();
                break;
            default:
                WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
                WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, 0, 0x562);
                break;
            }
        }
        break;
    case NWM_RETCODE_FAILED:
    case NWM_RETCODE_ILLEGAL_STATE:
    default:
        WCMi_SetPhase(WCM_PHASE_FATAL_ERROR);
        WCMi_Notify(WCM_RESULT_FATAL_ERROR, &work->connectBss, 0, 0x56e);
        break;
    }
}

// Fills in the scan request of a search
static void WcmEditScanExParam(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCMi_GetSystemWork();
    u16 channelTime;

    WCM_UpdateOption(option);
    work->nwmScanRequest.scanBuf = (NWMBssDesc *)work->ioBuffer.nwm;
    work->nwmScanRequest.scanBufSize = sizeof(work->ioBuffer.nwm);
    work->nwmScanRequest.channelList = (u16)((1 << WCMi_GetNextScanChannel(0)) >> 1);
    work->nwmScanRequest.scanType = (work->searchOption & 0x300000) == 0x300000;
    channelTime = work->scanChannelTime;
    if (channelTime == 0) {
        channelTime = NWM_GetDispersionScanPeriod(work->nwmScanRequest.scanType);
    }
    work->nwmScanRequest.channelDwellTime = channelTime;
    if (bssid == NULL) {
        MI_CpuCopy8(WCM_Bssid_Any, work->nwmScanRequest.bssid, 6);
    } else {
        MI_CpuCopy8(bssid, work->nwmScanRequest.bssid, 6);
    }
    if (essid == NULL || essid == WCM_Essid_Any) {
        MI_CpuCopy8(WCM_Essid_Any, work->nwmScanRequest.ssid, 0x20);
        work->nwmScanRequest.ssidLength = 0;
        work->nwmScanRequest.ssidMatchLength = 0;
    } else {
        s32 i;

        MI_CpuCopy8(essid, work->nwmScanRequest.ssid, 0x20);
        for (i = 0; i < 0x20; i++) {
            if (essid[i] == 0) {
                break;
            }
        }
        work->nwmScanRequest.ssidLength = (u16)i;
        work->nwmScanRequest.ssidMatchLength = (u16)i;
    }
    work->nwmScanCount = 0;
}

// The callback of a frame sent while the sender waits for it
static void WcmNwmcbSetDCFData(void *arg) {
    WCMCpsifWork *global = WCMi_GetCpsifWork();
    const NWMCallback *event = arg;

    if (event->apiid != NWM_APIID_SEND_FRAME) {
        return;
    }
    global->sendStatus = event->retcode;
    if (event->retcode == 0) {
        WCMi_ResetKeepAliveAlarm();
    }
    OS_WakeupThread(&global->sendQueue);
}

// The callback of a frame sent with a completion function, which it runs
static void WcmNwmcbSetDCFDataAsync(void *arg) {
    WCMCpsifWork *global = WCMi_GetCpsifWork();
    const NWMCallback *event = arg;
    s32 status;
    void (*completion)(s32);

    if (event->apiid != NWM_APIID_SEND_FRAME) {
        return;
    }
    global->sendStatus = event->retcode;
    if (event->retcode == 0) {
        WCMi_ResetKeepAliveAlarm();
    }
    switch (global->sendStatus) {
    case NWM_RETCODE_SUCCESS:
        status = 0;
        break;
    case NWM_RETCODE_FAILED:
    default:
        status = -5;
        break;
    }
    completion = global->sendCompletion;
    global->sendCompletion = NULL;
    WCMi_CpsifUnlockMutexInIRQ(&global->mutex);
    completion(status);
}

#pragma section LTDMAIN end
