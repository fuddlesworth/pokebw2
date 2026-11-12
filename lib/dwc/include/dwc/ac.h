#ifndef POKEBW2_DWC_AC_H
#define POKEBW2_DWC_AC_H

#include "types.h"
#include "dwc/bm.h"
#include "dwc/nasfunc.h"
#include "nitro/wm.h"
#include "nitrowifi/soc.h"
#include "nitrowifi/wcm.h"
#include "twl/nwm.h"

// DWC's auto-connect (TWL-DWC 5.3, ARM9 main; the ac/ files of lib/dwc), which finds an access point to join and
// brings the wireless manager, the socket layer and the connection up. The ROM names none of it; the names of the
// types, their fields and the functions are NitroDWC's own, as its libraries' symbols and ntrtwl's reconstruction of
// NitroDWC have them, where the code shows them, and ours elsewhere (the DWCAc_ ones and the fields that the DSi
// added).
//
// Auto-connect keeps five blocks of memory, which DWCi_AC_GetMemPtr gives out by the flag of each; a table of five
// pointers in the BSS holds them, and a bit of the byte at offset 8 of the first block says which of them are
// allocated. The first block is the control block, ACHOLD; the one of 0x14d8 bytes is the search's work.

// The flags DWCi_AC_GetMemPtr takes
#define DWC_AC_WORK_MAIN 0x01       // the control block: 0xcc bytes
#define DWC_AC_WORK_WCM 0x02        // the wireless connection manager's work: 0x4800 bytes
#define DWC_AC_WORK_SOC_CONFIG 0x04 // the SOCConfig that SOC_Startup is given (dwc_ac_soc.c): 0x58 bytes
#define DWC_AC_WORK_MEMFUNCS 0x08   // the connection test's DWCNetcheckParam: 0xc bytes
#define DWC_AC_WORK_SEARCH 0x10     // the search's work: 0x14d8 bytes

// The allocator and the deallocator that auto-connect is given, the same that DWC gives to the library. The name is
// always 1 here. The SDK's DWCACAlloc and DWCACFree (NitroDWC's base/dwc_connectinet.h)
typedef void *(*DWCACAlloc)(u32 name, s32 size);
typedef void (*DWCACFree)(u32 name, void *ptr, s32 size);

// What DWC_AC_Create is given, 0x14 bytes
typedef struct DWCACConfig {
    DWCACAlloc alloc;
    DWCACFree free;
    u8 dmaNo;                  // goes to ACWORK.dmaNo
    u8 powerMode;              // 0x09; ACWORK.radioOn
    u8 mode;                   // 0x0a; ACWORK.mode
    u8 skipConnectionTest : 4; // 0x0b; ACWORK.skipConnectionTest
    u8 rssiPacked : 4;         // ACWORK.rssiPacked
    s32 rwin;                  // ACWORK.rwin, the receive window of SOC on a DSi
    u8 ignoreEula;             // 0x10; ACWORK.ignoreEula
    u8 unk_11[3];
} DWCACConfig;

// The phase that the control block's byte at 9 holds, which DWCi_AC_SetPhase writes and DWC_AC_Process dispatches on:
// each phase has a handler in one of the ac files. The SDK's DWC_AC_GetStatus reports them as its own few values, the
// DWC_AC_STATE_* of NitroDWC's ac/dwc_ac.h, which are not these
enum {
    DWC_AC_PHASE_NULL = 0,              // before the control block exists
    DWC_AC_PHASE_START,                 // dwc_ac_soc.c's DWCi_AC_Start: the wireless manager starts
    DWC_AC_PHASE_SEARCH_START,          // dwc_ac_search.c's SearchStart: measures the channels, starts the scan
    DWC_AC_PHASE_SEARCH_AROUND,         // SearchAround: a round's channels for any access point
    DWC_AC_PHASE_SEARCH_DIFFER_CHANNEL, // SearchDifferChannel: each candidate on its own channel
    DWC_AC_PHASE_SEARCH_STEALTH,        // SearchStealth: each target's SSID where a beacon came with no SSID
    DWC_AC_PHASE_SEARCH_END,            // SearchEndCheck: the end of the search
    DWC_AC_PHASE_CONNECT_START,         // dwc_ac_connect.c's ConnectStart: the candidate's key and its fitness
    DWC_AC_PHASE_CONNECT,               // ConnectAP: the connection to the candidate
    DWC_AC_PHASE_CONNECT_RETRY,         // dwc_ac_search.c's DWCi_AC_ConnectRetryAP: the end of the connection to it
    DWC_AC_PHASE_TEST_START,            // dwc_ac_soc.c's ConnectTestStart: SOC starts
    DWC_AC_PHASE_TEST_RETRY,            // ConnectTestRetry: the sockets go down again
    DWC_AC_PHASE_TEST_ADDRESS,          // GetIPAddress: waits for the address from DHCP
    DWC_AC_PHASE_TEST_CREATE,           // ConnectTestCreate: the connection test starts
    DWC_AC_PHASE_TEST_PROCESS,          // ConnectTestProcess: the connection test runs
    DWC_AC_PHASE_TEST_END,              // ConnectTestEnd: the connection is noted
    DWC_AC_PHASE_COMPLETE,              // the connection is up
    DWC_AC_PHASE_ERROR,                 // dwc_ac_result.c's DWCi_AC_Error: the teardown
    DWC_AC_PHASE_FAILED,                // the teardown is over: the failure
};

// What DWCi_AC_SetError notes (ACHOLD.error), which DWCi_AC_GetResult turns into the code that the game is told
enum {
    DWC_AC_ERROR_WCM = 0,         // the wireless manager reported one
    DWC_AC_ERROR_WCM_START,       // WCM_StartupAsync would not start it
    DWC_AC_ERROR_SOC_START,       // SOC_Startup would not start the sockets
    DWC_AC_ERROR_NETCHECK_CREATE, // DWC_Netcheck_Create would not start the connection test
    DWC_AC_ERROR_IRREGULAR,       // the wireless manager ended without being asked to
    DWC_AC_ERROR_NO_AP,           // the search ended without finding an access point
    DWC_AC_ERROR_NO_INET,         // the search ended with every candidate tried
    DWC_AC_ERROR_EULA,            // the DSi's license agreement has not been agreed to
    DWC_AC_ERROR_WIRELESS_OFF,    // the settings or the parental controls forbid the wireless connection
};

// The kind of access point that ACHOLD.type and ACHOLD.overrideType hold: NitroDWC's DWC_AC_AP_TYPE (its ac/dwc_ac.h),
// with the DSi's own kinds after NINTENDOSPOT at 10 to 12. DWC_AC_AP_TYPE_FALSE is "none"
enum {
    DWC_AC_AP_TYPE_USER1 = 0,
    DWC_AC_AP_TYPE_USER2,
    DWC_AC_AP_TYPE_USER3,
    DWC_AC_AP_TYPE_USB,
    DWC_AC_AP_TYPE_SHOP,
    DWC_AC_AP_TYPE_FREESPOT,
    DWC_AC_AP_TYPE_WAYPORT,
    DWC_AC_AP_TYPE_NINTENDOWFC,
    DWC_AC_AP_TYPE_NINTENDOSPOT,
    DWC_AC_AP_TYPE_UNKNOWN = 99,
    DWC_AC_AP_TYPE_FALSE = 0xff
};

// The control block, 0xcc bytes. Only the bytes that the files seen so far touch are named
typedef struct ACHOLD {
    DWCACAlloc alloc;
    DWCACFree free; // lets a block go: DWCi_AC_FreeAll calls it with 1, the block and its size
    u8 allocState;  // 0x08; a bit for each of the blocks by its flag
    u8 phase;       // 0x09; what DWC_AC_Process dispatches on
    u8 phaseError;  // 0x0a; the state that DWCi_AC_SetError found when it was told of an error; the teardown starts
                    // there
    u8 findAP : 4;  // the search sets it to 1 whenever the wireless manager reports an access point
    u8 addressInUse : 4; // 1 when the connection test failed with CPS saying that the address is in use (dwc_ac_soc.c)
    s32 error;           // 0x0c; what DWCi_AC_SetError was told
    u32 errorTest;       // the result of the connection test of overlay 11 (DWC_Netcheck_GetReturnCode), kept by
                         // dwc_ac_soc.c
    u8 endState;         // 0x14; ACFL.result of the candidate that the connection works on
    u8 endType;          // 0x15; the type (DWCi_ConvConnectAPType) of that candidate's target
    u8 endSequence;      // 0x16; the highest state that DWCi_AC_SetPhase was given, from 1
    u8 type;             // 0x17; the type of the access point that the connection is with, 0xff for none
    u8 apSpotInfo[10];   // 0x18; what an NDWCSHAP's SSID tells (DWCi_AC_GetPostalCode), or the 10 bytes that
                         // DWC_AC_SetSpecifyApEx got
    u8 overrideType;     // 0x22; the type that DWC_AC_SetSpecifyApEx got: nonzero when the game gave the access point
    u8 keyMode; // 0x23; the key mode of the setting: the WCM mode of WPA, the WEP mode, or 2 for NWCUSBAP and NDWCSHAP
    // 0x24; the proxy of the connection's setting, which DWCAc_NoteKeyMode copies from the DSi setting
    u8 proxyMode;       // 0x24
    u8 proxyAuthType;   // 0x25
    u16 proxyPort;      // 0x26
    u8 proxyHost[0x64]; // 0x28
    u8 proxyId[0x20];   // 0x8c; the setting's first 32-byte identifier (base.ap.unk0)
    u8 proxyPass[0x20]; // 0xac; and its second (base.ap.unk20)
} ACHOLD;

// What a search looks for, one entry per kind of access point: the SSID to look for and the id that an access point
// with that SSID gets. There are DWC_AC_TARGET_MAX of them
typedef struct ACSL {
    u8 find : 4;      // 1 once the search has seen an access point with this SSID
    u8 duplicate : 4; // 1 for a later target of the SSID that is still to be tried on the access point that took one
    u8 type;
    u8 unk_02; // no writer or reader
    u8 length;
    char ssid[32];
} ACSL;

#define DWC_AC_TARGET_MAX 21

// The ids of the targets that the search makes from the stored settings (MakeUserList), which are also the ids that the
// connection reads the keys by (dwc_ac_connect.c's GetWepKey): for each of the DS's three settings and each of the
// DSi's, the SSID of the setting, the SSID that goes with its 40-bit WEP key of an AOSS setup, and, for the DSi's, the
// two SSIDs that go with its WPA keys
enum {
    DWC_AC_ID_USER1 = 0,
    DWC_AC_ID_USER2,
    DWC_AC_ID_USER3,
    DWC_AC_ID_USER1_AOSS, // 3; the SSID that goes with the AOSS setup's 40-bit key
    DWC_AC_ID_USER2_AOSS,
    DWC_AC_ID_USER3_AOSS,
    DWC_AC_ID_DSI_USER1, // 6
    DWC_AC_ID_DSI_USER2,
    DWC_AC_ID_DSI_USER3,
    DWC_AC_ID_DSI_USER1_AOSS, // 9
    DWC_AC_ID_DSI_USER2_AOSS,
    DWC_AC_ID_DSI_USER3_AOSS,
    DWC_AC_ID_DSI_USER1_WPA, // 12; the SSID that goes with the WPA key of the mode 4
    DWC_AC_ID_DSI_USER2_WPA,
    DWC_AC_ID_DSI_USER3_WPA,
    DWC_AC_ID_DSI_USER1_WPA2, // 15; and the one of the mode 6
    DWC_AC_ID_DSI_USER2_WPA2,
    DWC_AC_ID_DSI_USER3_WPA2,
};

// The ids of the access points that are told by their SSIDs: one that starts with "NWCUSBAP", one that decodes to
// "NDWCSHAP", one that starts with "FREESPOT" (dwc_ac_aplist.c's DWCi_AC_CheckFreespot) and one that is "NINTENDOWFC".
// The ids 0x13 to 0x16 are those of a kind that the candidates sort apart from the others; 0x15 is not known
#define DWC_AC_ID_NWCUSBAP 0x12
#define DWC_AC_ID_NDWCSHAP 0x13
#define DWC_AC_ID_FREESPOT 0x14
#define DWC_AC_ID_NINTENDOWFC 0x16
#define DWC_AC_ID_HOTSPOT_FIRST 0x13
#define DWC_AC_ID_HOTSPOT_LAST 0x16
#define DWC_AC_ID_IS_HOTSPOT(id) ((id) >= DWC_AC_ID_HOTSPOT_FIRST && (id) <= DWC_AC_ID_HOTSPOT_LAST)

// What the search keeps of the access point that it found for a target, next to the description of its beacon
// (ACWORK.candidates). The best of them come first
typedef struct ACFL {
    u8 state;       // the result of the connection to it: 0 while untried, else DWC_AC_RESULT_*
    u8 type;        // that of the target it matched
    u8 rssi;        // the signal's strength, the best that the beacons showed
    u8 channel : 7; // what AddList was told when the best beacon came in: the round or the channel
    u8 find : 1;    // set when the beacon was seen again on its channel
} ACFL;

// What ACFL.result says once the connection to an access point is over, or when it cannot be tried
#define DWC_AC_RESULT_GAVE_UP 1   // every attempt failed, or the sockets did not come up in time
#define DWC_AC_RESULT_LOST 2      // the connection to the access point was lost after it had come up
#define DWC_AC_RESULT_WRONG_KEY 3 // the key at hand does not fit the access point, or the access point refused it
#define DWC_AC_RESULT_FULL 4      // the access point had no room for another station

// The candidates that are kept, and one more as a place for the one that is being added
#define DWC_AC_CANDIDATE_MAX 10

// The connection settings that the system keeps, as auto-connect gets them in the search's work: the DS keeps three
// pages of 0x100 bytes (dwc/bm.h's DWCMemPage), and the DSi three more of 0x200, which go on with the passphrases of
// WPA
typedef struct DWCAcDsiSetting {
    DWCMemPage base;     // 0x0000; NitroDWC's NCFGSlotInfo
    u8 passphrase[0x20]; // 0x0100; NitroDWC's NCFGApInfoEx: the key for the WCM mode that wpaMode says, and for 4
    u8 pmk[0x20];        // 0x0120; and the key for the mode 6
    u8 ssid[2][0x20];    // 0x0140; the SSIDs that go with the two keys
    u8 ssidLength0;      // 0x0180
    u8 wpaMode;          // 0x0181; the mode that WCM_ConnectAsync wants of the setting's WPA
    u8 proxyMode;        // 0x0182; nonzero for a proxy, whose connection test is skipped
    u8 proxyAuthType;    // 0x0183
    u8 proxyHost[0x64];  // 0x0184
    u16 proxyPort;       // 0x01e8
    u8 rsvEx[6];         // 0x01ea
    u8 rsv[0x0e];        // 0x01f0; the two runs of reserved bytes of the second half
    u16 crcEx;           // 0x01fe; the CRC16 of the second half
} DWCAcDsiSetting;

// What WCM_ConnectAsync is given for the key: the mode (WCM's WEP modes 1 to 3, WPA 4 to 7, 0 for an open network),
// for WPA the length of the passphrase, and the key
typedef struct DWCAcKeyDesc {
    u8 mode;
    u8 length;
    u8 key[0x50];
} DWCAcKeyDesc;

// What ACWORK.mode asks the search to look for: everything, one of the DS's three settings (MakeUserList), one of the
// special access points alone (MakeAroundList, MakeStealthList, DWCi_AC_CheckNintendoSSID, DWCi_AC_CheckFreespot), or
// one of the DSi's three settings (0xb is the first)
enum {
    DWC_AC_MODE_ALL = 0,         // everything
    DWC_AC_MODE_USER1,           // the DS's first setting alone
    DWC_AC_MODE_USER2,           // its second
    DWC_AC_MODE_USER3,           // and its third
    DWC_AC_MODE_NWCUSBAP,        // an NWCUSBAP alone
    DWC_AC_MODE_NDWCSHAP,        // an NDWCSHAP alone
    DWC_AC_MODE_FREESPOT,        // FREESPOT alone
    DWC_AC_MODE_NINTENDOWFC = 8, // NINTENDOWFC alone
    DWC_AC_MODE_DSI_USER1 = 0xb, // the DSi's first setting alone
    DWC_AC_MODE_DSI_USER2,       // its second
    DWC_AC_MODE_DSI_USER3,       // and its third
};

// Why the connection to a candidate failed (ACWORK.failureReason), from the status code that the access point gave
// (dwc_ac_wcm.c's DWCi_AC_CallBackWCM): an authentication algorithm that it does not support, a challenge that failed,
// or that it cannot take another station; any other is OTHER
enum {
    DWC_AC_FAILURE_AUTH_ALG = 1,
    DWC_AC_FAILURE_CHALLENGE,
    DWC_AC_FAILURE_FULL,
    DWC_AC_FAILURE_OTHER,
};

// The step of DWCi_AC_MakeSearchList: the list of the first search (MakeAroundList), the candidates that are still to
// visit (MakeDifferChannelList), or the targets of the second search (MakeStealthList)
enum {
    DWC_AC_LIST_AROUND = 0,
    DWC_AC_LIST_DIFFER_CHANNEL,
    DWC_AC_LIST_STEALTH,
};

// The search's work, 0x14d8 bytes
typedef struct ACWORK {
    DWCMemPage userInfo[3];                      // 0x0000
    DWCAcDsiSetting dsiSettings[3];              // 0x0300
    ACSL searchList[DWC_AC_TARGET_MAX];          // 0x0900
    ACFL findList[DWC_AC_CANDIDATE_MAX + 1];     // 0x0bf4
    WMBssDesc bssDesc[DWC_AC_CANDIDATE_MAX + 1]; // 0x0c20
    // 0x1460; the system timer when the step at hand began: a scan started, or the connection came up
    u64 timeOut;
    DWCAcKeyDesc wepKey; // 0x1468
    u8 dmaNo;            // 0x14ba; the DMA channel that WCM_StartupAsync gets in WCMConfig.dmano (dwc_ac_soc.c)
    u8 radioOn : 2;      // 0x14bb; 1 to connect with the radio always on, else in the wireless manager's power saving
    u8 sharedKey : 2;    // 1 to authenticate with a shared key, else open; the connection drops it when that fails
    u8 round : 4;        // how many times the search went on to its second round (dwc_ac_search.c's SearchAround2ndLap)
    // 0x14bc; what the search looks for (DWC_AC_MODE)
    u8 mode : 4;
    u8 skipConnectionTest : 1; // 1 to skip the connection test (dwc_ac_soc.c); from DWCACConfig
    u8 rssiPacked : 1; // the beacon's signal is in the DSi's form, a quarter of what it is in the DS's, and 0x19 less
    u8 sameSsid : 1;   // the access point matched more than one target, which are tried on its next visits
    u8 preferWpa : 1;  // the candidates with WPA come before those without
    u8 connectApType;  // 0x14bd; the id of the target that the access point at hand was settled to
    u8 resumeState;    // 0x14be; the search state that the connection to a candidate came from and goes back to
    u8 searchListNo;   // 0x14bf; the index of the target or the candidate that the state at hand is working on
    u8 searchListNum;  // 0x14c0; how many targets there are, or in the round of the candidates still to visit, how many
                       // candidates
    // 0x14c1; the channel that the search is on, as an index (the channel less 1), -1 when it has none left
    s8 tag;
    u8 candidateCount;  // 0x14c2
    u8 connectNo;       // 0x14c3; the candidate that the connection works on
    u8 failureReason;   // 0x14c4; why the last connection failed (DWC_AC_FAILURE)
    u8 attempts;        // 0x14c5; how many times the connection to the candidate has been tried; in the search's third
                        // state, how many of the channels in hiddenMask it is past
    u16 stealthChannel; // 0x14c6; a bit for each channel (by its index) where a beacon came with no SSID
    s32 rwin;           // 0x14c8; the receive window that SOC is started with on a DSi (dwc_ac_soc.c), from DWCACConfig
    u8 ignoreEula;      // 0x14cc; nonzero lets the connection go on without the DSi's license agreement
    u8 unk_14cd;
    u16 channelMax; // 0x14ce; the highest channel that the wireless manager allows, as a number from 1
    u16 unk_14d0;
    u16 scanTime;     // 0x14d2; how many milliseconds a scan with no SSID stays on a channel: 300 or, on a DSi, 278
    u16 directedTime; // 0x14d4; the same for a scan of one SSID or one access point: 150 or, on a DSi, 203
    u8 unk_14d6[2];
} ACWORK;

// dwc_ac.c
// Creates auto-connect from the allocator and the settings of the param: the control block and the four more blocks,
// the settings that it works from, and the wireless manager. FALSE if the wireless manager does not start
BOOL DWC_AC_Create(DWCACConfig *param);
// Runs the state at hand once and moves to the next. 1 once the connection is up (2 if CheckDuplicate finds
// another candidate like the one connected to), -1 when it has failed for good, else 0
s32 DWC_AC_Process(void);
// What the state is, in a few values: 0 before the search, 1 searching, 2 connecting, 3 the sockets coming up, 4 going
// on to another candidate or to the teardown, 5 connected; for a failure, the error code of dwc_ac_result.c's
// DWCi_AC_GetResult
s32 DWC_AC_GetStatus(void);
// The type of the access point that the connection is with (ACHOLD.type), 0xff while there is none
u8 DWC_AC_GetApType(void);
// Copies the access point's 10 bytes of spot information, if the connection is with one that has them. TRUE if so
BOOL DWC_AC_GetApSpotInfo(u8 *spotInfo);
// The key mode of the setting that the connection is made with (ACHOLD.keyMode)
u8 DWCAc_GetKeyMode(void);
// Takes auto-connect down, a step at a time. TRUE once it is down, with every block let go
BOOL DWC_AC_Destroy(void);
// Gives the connection to make by hand: an SSID, a WEP key with its mode (the length it has), the MTU of the
// first setting, the spot information if any, and the type that the access point is given
void DWC_AC_SetSpecifyApEx(const u8 *ssid, const u8 *key, u32 wepMode, u16 mtu, const u8 *spotInfo, u32 overrideType);
// Whether the SSID of the given length is that of an NDWCSHAP
BOOL DWC_AC_CheckWiFiStation(const u8 *ssid, u32 length);
// Replaces the connection of the DS's setting `index` with the 0xf0 bytes of setting
void DWCi_AC_InsertApInfo(u32 index, const void *setting);
// Stops the search from preferring the access points with WPA
void DWCAc_ClearPreferWpa(void);
// The block of memory that a flag of DWC_AC_WORK_* names, NULL while it is not allocated
void *DWCi_AC_GetMemPtr(u32 flag);
// The state of the control block, 0 while there is none
u8 DWCi_AC_GetPhase(void);
// Notes an error, as the control block's error and, as its failed state, the state it is in
void DWCi_AC_SetError(s32 error);
// The error that was noted last
s32 DWCi_AC_GetError(void);
// Notes the type of the access point that the connection is with, and its spot information if it has some
void DWCi_AC_SetApType(u8 id);
// Notes the key mode of the setting that the target `id` is, and a copy of the DSi setting's details
void DWCAc_NoteKeyMode(u8 id);
// The type of the access point of a target id: 0 to 2 the DS's settings, 3 to 5 NWCUSBAP, NDWCSHAP and FREESPOT, 10 to
// 12 the DSi's
u8 DWCi_ConvConnectAPType(u8 id);

// dwc_ac_connect.c
// The handler of the states 7 and 8, which settle the key for the candidate and then connect to it: the state that
// follows
u8 DWCi_AC_ConnectAP(void);

// dwc_ac_search.c
// Notes that a beacon with no SSID came in on the channel (its number, from 1), so that the search's third pass looks
// on it
void DWCi_AC_SetStealthChannel(u32 channel);
// The handler of the states 2 to 6, the search: the state that follows
u8 DWCi_AC_SearchAP(void);
// The handler of state 9, the end of the connection to a candidate: back to the search, or to the next target of the
// candidate; the state that follows
u8 DWCi_AC_ConnectRetryAP(void);

// dwc_ac_soc.c
// The handler of state 1 (the wireless manager's start), and the one of the states 10 to 15 (the sockets coming up, the
// connection kept): the state that follows
u8 DWCi_AC_Start(void);
u8 DWCi_AC_ConnectTest(void);

// dwc_ac_result.c
// The teardown of the state 17, from the state the failure came in: 18 once it is over, else 17
u8 DWCi_AC_Error(void);
// The error code that is told to the game, from the error that DWCi_AC_SetError noted
s32 DWCi_AC_GetResult(void);

// dwc_ac_aplist.c
// Makes the list that a step of the search works from: step 0 the targets of the first search (the stored settings and
// FREESPOT), step 1 the candidates still to visit, step 2 the targets of the second (the stored settings, NWCUSBAP and
// NINTENDOWFC). FALSE if steps 1 and 2 leave nothing to look for
BOOL DWCi_AC_MakeSearchList(u32 step);
// The id of FREESPOT if the SSID of the beacon is that and the search takes it, else 0
int DWCi_AC_CheckFreespot(const WMBssDesc *bss);
// How many targets there are: the first with an SSID of no length ends them, at DWC_AC_TARGET_MAX at most
u8 DWCAc_CountTargets(void);

// dwc_ac_wepkey.c
// Makes the 13-byte WEP key of an access point whose SSID starts with "NWCUSBAP", from the SSID alone
void DWCi_AC_GetNintendoUSBWepKey(const u8 *ssid, u8 *key);
// Makes the 13-byte WEP key of an access point whose SSID, decoded, starts with "NDWCSHAP", from the SSID alone
void DWCi_AC_GetNintendoShopWepKey(const u8 *ssid, u8 *key);
// Whether the SSID starts with "NWCUSBAP"
BOOL DWCi_AC_CheckNintendoUsbAP(const u8 *ssid);
// Whether the 32 characters of the SSID, decoded, start with "NDWCSHAP"
BOOL DWCi_AC_CheckNintendoShopAP(const u8 *ssid);
// Decodes an NDWCSHAP's SSID into its 10 bytes of spot information; TRUE if it holds the 8 bytes "NDWCSHAP" and the 10
// are printable
BOOL DWCi_AC_GetPostalCode(const u8 *ssid, u8 *spotInfo);

// dwc_ac_wcm.c
// The id of the kind of access point that an unmatched beacon with a 32-byte SSID is, from its SSID and the kind that
// the search wants: DWC_AC_ID_NWCUSBAP or DWC_AC_ID_NDWCSHAP, or -1 for none
int DWCi_AC_CheckNintendoSSID(const WMBssDesc *bss);
// Whether the access point can be taken for a connection with WPA: FALSE only on a DSi, for a beacon that has neither
// the RSN element nor the WPA vendor element
BOOL DWCAc_CheckWpa(const WMBssDesc *bss);
// The wireless manager's notification callback during the search. It takes in the access points that are found and
// notes why a connection failed
void DWCi_AC_CallBackWCM(WCMNotice *notice);

// dwc_ac_cleanup.c
// Steps the teardown whose state is *state (ACHOLD.failedState): the sockets down first, if the state is over 10,
// and then the wireless manager to idle. TRUE when it is over, with *state 0, or has failed, with *state 0x12
BOOL DWCi_AC_CloseNetwork(u8 *state);

#endif // POKEBW2_DWC_AC_H
