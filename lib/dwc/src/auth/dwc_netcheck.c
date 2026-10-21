#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "dwc/nasfunc.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <errno.h>

// DWC's connection test (dwc_netcheck.c, overlay 11, Thumb): it reaches the test server, logs in to the Nintendo server
// when the console has no Wi-Fi ID yet, and answers a hotspot's login page through the server ("HotSpotResponse" and
// "parse"). The ROM embeds no name; the file and its globals are named as TWL-DWC's, whose dwc_netcheck.c is this code
// in CacaBueno64/ie3ogres's decompilation (lib/TwlDWC_dl/asm/auth/dwc_netcheck.s), and the thread's error exits are
// written as ctrl-cheeb-del/pokeheartgold writes NitroDWC's (lib/NitroDWC/src/auth_netcheck.c)

// The work of the connection test (0x11f4 bytes), whose first 0x1000 bytes are the stack of its thread
typedef struct NasPrWork {
    u8 stack[0x1000];
    int state;
    int result;
    u8 unk_1008[0x100];
    DWCAcMemFuncs memFuncs;
    char *redirectUrl;
    char *hotspotUrl;
    OSThread thread;
    OSMutex lock;
} NasPrWork;

static void DWCi_Netcheck_StartThread(void);
static void DWCi_Netcheck_Thread(void *arg);
static void DWCi_Netcheck_SetError(int state);
static void DWCi_Netcheck_KeepSimpleCheck(void);

// MWCC sorts the file's data by size, and these definitions give the ROM's layout. TWL-DWC has the test server's URL
// as the global DWCnetcheckblurl, after the first-call flag of DWC_SimpleNetcheckAsync, a function that the linker
// dropped from the ROM; as two definitions here the URL comes first, so they are one object
static struct {
    BOOL simpleCheckFirst;
    char *url;
} sConnTest = { TRUE, "http://conntest.nintendowifi.net/" };
// The address that the test reached
CPSInAddr DWCconntestip;
// The test's HTTP client
DWCHttp *DWChttp;
BOOL DWCishotspot;
// The test's work
NasPrWork *DWCnetcheck;
// DWC_SimpleNetcheckAsync's work, and 0x18 bytes that it also used (our name)
void *DWCsimplecheckwork;
// Whether a redirect (302) counts as reaching the network, which nothing sets (the SDK's DWC_Netcheck_SetAllow302)
BOOL DWCallow302;
u8 DWCsimplecheckparam[0x18];
// The request and the login that the test makes
DWCHttpParam DWCnetcheckhttpparam;
DWCAuthParam DWCnetcheckauthparam;

int DWC_Netcheck_Create(const DWCAcMemFuncs *init) {
    DWCHttpAlloc alloc = init->alloc;

    if (DWCnetcheck != NULL) {
        return 4;
    }
    DWCnetcheck = alloc(0, sizeof(NasPrWork));
    if (DWCnetcheck == NULL) {
        return 4;
    }
    MI_CpuFill8(DWCnetcheck, 0, sizeof(NasPrWork));
    DWCnetcheck->result = -99999;
    DWCnetcheck->memFuncs = *init;
    if (DWChttp != NULL) {
        return 4;
    }
    DWChttp = alloc(0, sizeof(DWCHttp));
    if (DWChttp == NULL) {
        return 4;
    }
    DWCishotspot = 0;
    OS_InitMutex(&DWCnetcheck->lock);
    DWCi_Netcheck_StartThread();
    return 0;
}

void DWC_Netcheck_Destroy(void) {
    DWCHttpFree free = DWCnetcheck->memFuncs.free;

    if (DWChttp != NULL) {
        DWC_Http_Destroy(DWChttp);
        free(0, DWChttp, 0);
        DWChttp = NULL;
    }
    DWC_Auth_Destroy();
    if (DWCnetcheck != NULL) {
        if (DWCnetcheck->redirectUrl != NULL) {
            free(0, DWCnetcheck->redirectUrl, 0);
            DWCnetcheck->redirectUrl = NULL;
        }
        if (DWCnetcheck->hotspotUrl != NULL) {
            free(0, DWCnetcheck->hotspotUrl, 0);
            DWCnetcheck->hotspotUrl = NULL;
        }
        free(0, DWCnetcheck, 0);
        DWCnetcheck = NULL;
    }
}

void DWC_Netcheck_Abort(void) {
    if (DWCnetcheck != NULL) {
        if (DWChttp != NULL) {
            DWC_Http_Abort(DWChttp);
        }
        DWC_Auth_Abort();
        if (DWCnetcheck->thread.id != 0 && !OS_IsThreadTerminated(&DWCnetcheck->thread)) {
            OS_JoinThread(&DWCnetcheck->thread);
        }
        DWCnetcheck->result = -7;
    }
}

u32 DWC_Netcheck_GetError(void) {
    int state;

    OS_LockMutex(&DWCnetcheck->lock);
    state = DWCnetcheck->state;
    OS_UnlockMutex(&DWCnetcheck->lock);
    return state;
}

u32 DWC_Netcheck_GetReturnCode(void) {
    return DWCnetcheck->result;
}

static void DWCi_Netcheck_StartThread(void) {
    if (DWCnetcheck->thread.id == 0 || OS_IsThreadTerminated(&DWCnetcheck->thread)) {
        OS_CreateThread(&DWCnetcheck->thread, DWCi_Netcheck_Thread, DWCnetcheck, DWCnetcheck->stack + 0x1000, 0x1000,
                        16);
        OS_WakeupThreadDirect(&DWCnetcheck->thread);
    }
}

static void DWCi_Netcheck_Thread(void *arg) {
    DWCHttpAlloc alloc = DWCnetcheck->memFuncs.alloc;
    DWCHttpFree free = DWCnetcheck->memFuncs.free;
    char *url = NULL;
    char *data = NULL;
    char *waitText = NULL;
    int waitMs;
    int status;
    int length;
    int dataLength;
    int waitLength;
    int returnCode;
    char returnText[4];
    char *location;
    DWCBmWifiIdInfo userInfo;
    NasAuthInfo info;
    DWCAuthResult nasStatus;

    for (;;) {
        DWCnetcheckhttpparam.url = sConnTest.url;
        DWCnetcheckhttpparam.get = 1;
        DWCnetcheckhttpparam.bufferSize = 0x1000;
        DWCnetcheckhttpparam.alloc = alloc;
        DWCnetcheckhttpparam.free = free;
        DWCnetcheckhttpparam.timeout = 20000;
        DWCnetcheck->result = -2;
        if (DWC_Http_Create(DWChttp, &DWCnetcheckhttpparam) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (DWC_Http_FinishHeader(DWChttp) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWChttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWChttp->thread.id != 0 && !OS_IsThreadTerminated(&DWChttp->thread)) {
            OS_JoinThread(&DWChttp->thread);
        }
        if (DWChttp->result == 2) {
            DWCnetcheck->result = -1;
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWChttp->result != 8) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWChttp, 0) != 1) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        status = atol(DWC_Http_GetResult(DWChttp, "httpresult"));
        if (errno == ERANGE) {
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        if (status == 200) {
            DWCconntestip = DWChttp->ip;
        } else if (status == 302) {
            DWCishotspot = 1;
            if (DWCnetcheck->hotspotUrl != NULL) {
                DWCnetcheck->result = -6;
                DWC_Http_Destroy(DWChttp);
                DWCnetcheckhttpparam.url = DWCauthhttpparam.url;
                DWCnetcheckhttpparam.get = 0;
                DWCnetcheckhttpparam.bufferSize = 0x200;
                DWCnetcheckhttpparam.alloc = alloc;
                DWCnetcheckhttpparam.free = free;
                DWCnetcheckhttpparam.timeout = 20000;
                if (strcmp(DWCauthhttpparam.url, "https://nas.nintendowifi.net/ac") != 0) {
                    DWCnetcheckhttpparam.ignoreCa = 1;
                }
                if (DWC_Http_Create(DWChttp, &DWCnetcheckhttpparam) != 0) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(1);
                    goto done;
                }
                if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(DWChttp, &info, 1)) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                if (DWC_Http_Add_PostBase64Item(DWChttp, "action", "message", 7) != 0) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                if (DWC_Http_Add_PostBase64Item(DWChttp, "HotSpotResponse", DWCnetcheck->hotspotUrl,
                                                strlen(DWCnetcheck->hotspotUrl)) != 0) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                free(0, DWCnetcheck->hotspotUrl, 0);
                DWCnetcheck->hotspotUrl = NULL;
                if (DWC_Http_FinishHeader(DWChttp) != 0) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(1);
                    goto done;
                }
                DWC_Http_StartThread(DWChttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
                if (DWChttp->thread.id != 0 && !OS_IsThreadTerminated(&DWChttp->thread)) {
                    OS_JoinThread(&DWChttp->thread);
                }
                if (DWChttp->result == 2) {
                    DWCnetcheck->result = -1;
                } else if (DWChttp->result == 8) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(7);
                    goto done;
                }
                DWC_Http_Destroy(DWChttp);
                DWCi_Netcheck_SetError(3);
                goto done;
            } else {
                location = DWChttp->response.start;
                if (location == NULL) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(2);
                    goto done;
                }
                DWCnetcheck->redirectUrl = alloc(0, strlen(location) + 1);
                if (DWCnetcheck->redirectUrl == NULL) {
                    DWC_Http_Destroy(DWChttp);
                    DWCi_Netcheck_SetError(4);
                    goto done;
                }
                strncpy(DWCnetcheck->redirectUrl, location, strlen(location) + 1);
            }
        } else {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(10);
            goto done;
        }
        DWC_Http_Destroy(DWChttp);
        DWCBm_GetWifiIdInfo(&userInfo);
        if (userInfo.id == 0) {
            DWCnetcheck->result = -3;
            DWCnetcheckauthparam.ingamesn[0] = 0;
            DWCnetcheckauthparam.gsbrcd[0] = 0;
            DWCnetcheckauthparam.alloc = DWCnetcheck->memFuncs.alloc;
            DWCnetcheckauthparam.free = DWCnetcheck->memFuncs.free;
            if (DWC_Auth_Create(&DWCnetcheckauthparam, DWChttp) != 0) {
                DWCi_Netcheck_SetError(5);
                goto done;
            }
            DWC_Auth_Join();
            if (DWC_Auth_GetError() != 0x15) {
                if (DWC_Auth_GetError() == 9) {
                    nasStatus.returnCode = -1;
                } else {
                    DWC_Auth_GetResult(&nasStatus);
                    if (DWCallow302 == 1 && (nasStatus.returnCode == -23302 || DWC_Auth_GetError() == 0xb)) {
                        DWCnetcheck->result = 0;
                        DWC_Auth_Destroy();
                        DWCi_Netcheck_SetError(0xb);
                        goto done;
                    }
                }
                DWCnetcheck->result = nasStatus.returnCode;
                DWC_Auth_Destroy();
                DWCi_Netcheck_SetError(6);
                goto done;
            }
            DWC_Auth_Destroy();
        }
        if (status == 200) {
            DWCnetcheck->result = 0;
            DWCi_Netcheck_SetError(0xb);
            goto done;
        }
        DWCnetcheck->result = -4;
        DWCnetcheckhttpparam.url = DWCauthhttpparam.url;
        DWCnetcheckhttpparam.get = 0;
        DWCnetcheckhttpparam.bufferSize = 0x1000;
        DWCnetcheckhttpparam.alloc = alloc;
        DWCnetcheckhttpparam.free = free;
        DWCnetcheckhttpparam.timeout = 40000;
        if (strcmp(DWCauthhttpparam.url, "https://nas.nintendowifi.net/ac") != 0) {
            DWCnetcheckhttpparam.ignoreCa = 1;
        }
        if (DWC_Http_Create(DWChttp, &DWCnetcheckhttpparam) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(DWChttp, &info, 1) ||
            DWC_Http_Add_PostBase64Item(DWChttp, "action", "parse", 5) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        if (DWC_Http_Add_PostBase64Item(DWChttp, "HTML", DWCnetcheck->redirectUrl, strlen(DWCnetcheck->redirectUrl)) !=
            0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        free(0, DWCnetcheck->redirectUrl, 0);
        DWCnetcheck->redirectUrl = NULL;
        if (DWC_Http_FinishHeader(DWChttp) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWChttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWChttp->thread.id != 0 && !OS_IsThreadTerminated(&DWChttp->thread)) {
            OS_JoinThread(&DWChttp->thread);
        }
        status = DWChttp->result;
        if (status == 2) {
            DWCnetcheck->result = -1;
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (status == 3) {
            DWC_Http_Destroy(DWChttp);
            if (DWCallow302 == 1) {
                DWCnetcheck->result = 0;
                DWCi_Netcheck_SetError(0xb);
            } else {
                DWCi_Netcheck_SetError(3);
            }
            goto done;
        }
        if (status != 8) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWChttp, 0) != 1) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        status = atol(DWC_Http_GetResult(DWChttp, "httpresult"));
        if (errno == ERANGE) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        if (status != 200) {
            DWC_Http_Destroy(DWChttp);
            if (DWCallow302 == 1 && status == 302) {
                DWCnetcheck->result = 0;
                DWCi_Netcheck_SetError(0xb);
            } else {
                DWCi_Netcheck_SetError(2);
            }
            goto done;
        }
        if (DWC_Http_GetBase64DecodedResult(DWChttp, "returncd", returnText, 4) <= 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        returnCode = atol(returnText);
        if (errno == ERANGE) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        if (DWCallow302 == 1 && returnCode == 0x72) {
            DWCnetcheck->result = 0;
            DWCi_Netcheck_SetError(0xb);
            goto done;
        }
        if (returnCode >= 100) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(6);
            goto done;
        }
        length = DWC_Http_GetBase64DecodedResult(DWChttp, "url", NULL, 0);
        if (length <= 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        dataLength = DWC_Http_GetBase64DecodedResult(DWChttp, "data", NULL, 0);
        if (dataLength <= 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        waitLength = DWC_Http_GetBase64DecodedResult(DWChttp, "wait", NULL, 0);
        url = alloc(0, length + 1);
        if (url == NULL) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        data = alloc(0, dataLength + 1);
        if (data == NULL) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        if (waitLength > 0) {
            waitText = alloc(0, waitLength + 1);
            if (waitText == NULL) {
                DWC_Http_Destroy(DWChttp);
                DWCi_Netcheck_SetError(4);
                goto done;
            }
        }
        length = DWC_Http_GetBase64DecodedResult(DWChttp, "url", url, length + 1);
        if (length < 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        url[length] = 0;
        length = DWC_Http_GetBase64DecodedResult(DWChttp, "data", data, dataLength + 1);
        if (length < 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        data[length] = 0;
        waitMs = 0;
        if (waitLength > 0) {
            length = DWC_Http_GetBase64DecodedResult(DWChttp, "wait", waitText, waitLength + 1);
            if (length < 0) {
                DWC_Http_Destroy(DWChttp);
                DWCi_Netcheck_SetError(9);
                goto done;
            }
            waitText[length] = 0;
            waitMs = atol(waitText);
            if (errno == ERANGE) {
                DWC_Http_Destroy(DWChttp);
                DWCi_Netcheck_SetError(9);
                goto done;
            }
            waitMs *= 1000;
            if (waitMs > 180000) {
                waitMs = 180000;
            }
        }
        DWC_Http_Destroy(DWChttp);
        DWCnetcheck->result = -5;
        DWCnetcheckhttpparam.url = url;
        DWCnetcheckhttpparam.get = 0;
        DWCnetcheckhttpparam.bufferSize = 0x1000;
        DWCnetcheckhttpparam.alloc = alloc;
        DWCnetcheckhttpparam.free = free;
        DWCnetcheckhttpparam.timeout = 120000;
        if (DWC_Http_Create(DWChttp, &DWCnetcheckhttpparam) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (DWC_Http_Add_Body(DWChttp, data) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        if (DWC_Http_FinishHeader(DWChttp) != 0) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWChttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWChttp->thread.id != 0 && !OS_IsThreadTerminated(&DWChttp->thread)) {
            OS_JoinThread(&DWChttp->thread);
        }
        if (DWChttp->result == 2) {
            DWCnetcheck->result = -1;
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWChttp->result != 8) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWChttp, 1) != 1) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        location = DWChttp->response.start;
        if (location == NULL) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        DWCnetcheck->hotspotUrl = alloc(0, strlen(location) + 1);
        if (DWCnetcheck->hotspotUrl == NULL) {
            DWC_Http_Destroy(DWChttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        strncpy(DWCnetcheck->hotspotUrl, location, strlen(location) + 1);
        DWC_Http_Destroy(DWChttp);
        OS_Sleep(waitMs);
    }
done:
    if (url != NULL) {
        free(0, url, 0);
    }
    if (data != NULL) {
        free(0, data, 0);
    }
    if (waitText != NULL) {
        free(0, waitText, 0);
    }
}

static void DWCi_Netcheck_SetError(int state) {
    OS_LockMutex(&DWCnetcheck->lock);
    DWCnetcheck->state = state;
    OS_UnlockMutex(&DWCnetcheck->lock);
}

// Stands for DWC_SimpleNetcheckAsync: nothing calls it, so MWCC doesn't emit it, and reading DWCsimplecheckwork and
// DWCsimplecheckparam keeps them in the file's shared .bss where the ROM has them. Its body is a guess
static void DWCi_Netcheck_KeepSimpleCheck(void) {
    if (sConnTest.simpleCheckFirst) {
        sConnTest.simpleCheckFirst = FALSE;
        MI_CpuFill8(DWCsimplecheckparam, 0, sizeof(DWCsimplecheckparam));
        DWCsimplecheckwork = NULL;
    }
}
