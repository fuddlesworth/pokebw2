// wifibattlematch_battle.c, in overlay 290 (a guessed name; the file has no string): the proc that runs a battle of
// the Battle Competition, for wifibattlematch_sys.c's battle step

#include "types.h"
#include "app/battle_championship_flow.h"
#include "app/comm_btl_demo.h"
#include "app/livebattlematch_irc.h"
#include "app/ov306.h"
#include "app/wifi_login.h"
#include "app/wifibattlematch.h"
#include "app/wifibattlematch_battle.h"
#include "app/wifibattlematch_link.h"
#include "app/wifibattlematch_subproc.h"
#include "battle/battle_proc.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_net.h"
#include "battle/btl_result.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "dwc/dwc.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"

// The heap of the proc that runs the battle
#define HEAPID_WIFIBATTLEMATCH_BATTLE 0x58

// The battle wrapper's steps; the unassigned no-op state 13 stays unexplained.
enum {
    BATTLE_START_INTRO = 0,
    BATTLE_WAIT_INTRO = 1,
    BATTLE_START_SYNC = 2,
    BATTLE_WAIT_SYNC = 3,
    BATTLE_START_PROC = 4,
    BATTLE_WAIT_PROC = 5,
    BATTLE_ADVANCE_TO_RESULT_SYNC = 6,
    BATTLE_START_RESULT_SYNC = 7,
    BATTLE_WAIT_RESULT_SYNC = 8,
    BATTLE_START_RESULT_ACK = 9,
    BATTLE_WAIT_RESULT_ACK = 10,
    BATTLE_START_RESULT_DEMO = 11,
    BATTLE_WAIT_RESULT_DEMO = 12,
    BATTLE_END = 14,
    BATTLE_WAIT_ERROR = 15,
};

// The battle proc's work
typedef struct {
    GameProcManager *manager;
    u32 timer;
    BOOL ov167Loaded;
    BOOL commandsSet;
} WifiBattleMatchBattleWork;

static BOOL WifiBattleMatchSys_BattleProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchSys_BattleProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchSys_BattleProcMain(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions WIFIBATTLEMATCH_BATTLE_PROC_FUNCTIONS = {
    WifiBattleMatchSys_BattleProcInit,
    WifiBattleMatchSys_BattleProcMain,
    WifiBattleMatchSys_BattleProcExit,
};

static BOOL WifiBattleMatchSys_BattleProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchBattleParam *battle = param;
    WifiBattleMatchBattleWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_BATTLE, 0x150);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchBattleWork), HEAPID_WIFIBATTLEMATCH_BATTLE);
    MI_CpuFill8(wk, 0, sizeof(WifiBattleMatchBattleWork));
    battle->result = WIFIBATTLEMATCH_BATTLE_RESULT_FINISHED;
    wk->manager = CreateGameProcManager(HEAPID_WIFIBATTLEMATCH_BATTLE);
    GFL_SndBGMFadeOut(0x10);
    return TRUE;
}

static BOOL WifiBattleMatchSys_BattleProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchBattleWork *wk = work;

    if (wk->ov167Loaded) {
        GFL_OvlUnload(OVERLAY_ID(167));
        wk->ov167Loaded = FALSE;
    }
    if (wk->commandsSet) {
        NetCommand_RemoveTable(0x100);
        wk->commandsSet = FALSE;
    }
    FreeGameProcManager(wk->manager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_BATTLE);
    return TRUE;
}

static BOOL WifiBattleMatchSys_BattleProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchBattleParam *battle = param;
    WifiBattleMatchBattleWork *wk = work;
    int running = GFL_ProcMgrUpdate(wk->manager);
    BOOL queue;
    int last;
    u32 first;
    u32 second;
    int i;
    int j;
    u32 client;
    u32 source;

    switch (*state) {
    case BATTLE_START_INTRO:
        GFL_SndBGMPlay(battle->setup->fieldSituation.bgm, SND_CHANNEL_MASK_ALL);
        battle->players->base.demoType = COMM_BTL_DEMO_TYPE_SINGLE_START;
        QueueGameProc(wk->manager, OVERLAY_ID(305), &CommBtlDemoProc, battle->players);
        running = PROCMGR_PROC_ENDED;
        *state = BATTLE_WAIT_INTRO;
        break;
    case BATTLE_WAIT_INTRO:
        if (running == PROCMGR_RUNNING) {
            break;
        }
        *state = BATTLE_START_SYNC;
        break;
    case BATTLE_START_SYNC:
        GFL_OvlLoad(OVERLAY_ID(167));
        wk->ov167Loaded = TRUE;
        NetCommand_AddTable(0x100, BTL_NET_COMMANDS, 9, NULL);
        wk->commandsSet = TRUE;
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0xc8, 0x24);
        *state = BATTLE_WAIT_SYNC;
        break;
    case BATTLE_WAIT_SYNC:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0xc8, 0x24)) {
            *state = BATTLE_START_PROC;
        }
        break;
    case BATTLE_START_PROC:
        QueueGameProc(wk->manager, OVERLAY_NONE, &BATTLE_PROC_FUNCTIONS, battle->setup);
        running = PROCMGR_PROC_ENDED;
        *state = BATTLE_WAIT_PROC;
        break;
    case BATTLE_WAIT_PROC:
        if (running == PROCMGR_RUNNING) {
            break;
        }
        *state = BATTLE_ADVANCE_TO_RESULT_SYNC;
        break;
    case BATTLE_ADVANCE_TO_RESULT_SYNC:
        *state = BATTLE_START_RESULT_SYNC;
        break;
    case BATTLE_START_RESULT_SYNC:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0xd2, 0x24);
        wk->timer = 0;
        *state = BATTLE_WAIT_RESULT_SYNC;
        break;
    case BATTLE_WAIT_RESULT_SYNC:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0xd2, 0x24)) {
            NetCommand_RemoveTable(0x100);
            wk->commandsSet = FALSE;
            GFL_OvlUnload(OVERLAY_ID(167));
            wk->ov167Loaded = FALSE;
            *state = BATTLE_START_RESULT_ACK;
        }
        if (GFL_NetIsInitialized()) {
            if (GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC) {
                if (wk->timer++ > 600) {
                    NetCommand_RemoveTable(0x100);
                    wk->commandsSet = FALSE;
                    GFL_OvlUnload(OVERLAY_ID(167));
                    wk->ov167Loaded = FALSE;
                    *state = BATTLE_START_RESULT_ACK;
                    wk->timer = 0;
                }
            }
        }
        break;
    case BATTLE_START_RESULT_ACK:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0xd3, 0x24);
        *state = BATTLE_WAIT_RESULT_ACK;
        wk->timer = 0;
        break;
    case BATTLE_WAIT_RESULT_ACK:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0xd3, 0x24)) {
            *state = BATTLE_START_RESULT_DEMO;
        }
        if (GFL_NetIsInitialized()) {
            if (GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC) {
                if (wk->timer++ > 600) {
                    *state = BATTLE_START_RESULT_DEMO;
                    wk->timer = 0;
                }
            }
        }
        break;
    case BATTLE_START_RESULT_DEMO:
        queue = TRUE;
        if (battle->setup->fieldSituation.multiMode == 0) {
            battle->players->base.demoType = COMM_BTL_DEMO_TYPE_SINGLE_END;
        } else {
            battle->players->base.demoType = COMM_BTL_DEMO_TYPE_MULTI_END;
        }
        switch (battle->setup->result) {
        case BTL_RESULT_VICTORY:
        case BTL_RESULT_FLEE_FOE:
            battle->players->base.result = COMM_BTL_DEMO_RESULT_WIN;
            break;
        case BTL_RESULT_DEFEAT:
        case BTL_RESULT_FLEE:
            battle->players->base.result = COMM_BTL_DEMO_RESULT_LOSE;
            break;
        case BTL_RESULT_DRAW:
            battle->players->base.result = COMM_BTL_DEMO_RESULT_DRAW;
            break;
        case BTL_RESULT_6:
            wk->timer = 0;
            queue = FALSE;
            *state = BATTLE_WAIT_ERROR;
            break;
        default:
            battle->players->base.result = COMM_BTL_DEMO_RESULT_DRAW;
            break;
        }
        if (battle->setup->fieldSituation.multiMode == 0) {
            last = 1;
            first = 0;
            second = 1;
        } else {
            last = 3;
            first = 0;
            second = 2;
        }
        client = battle->setup->fieldSituation.playerClientId;
        for (i = 0; i <= last; i++) {
            if ((i & 1) == (client & 1)) {
                if (client & 1) {
                    source = (i & 2) + 1;
                } else {
                    source = i & 2;
                }
                for (j = 0; j < 6; j++) {
                    battle->players->base.players[first + (i >> 1)].order[j] = battle->setup->monStatus[source][j];
                }
            } else {
                if (client & 1) {
                    source = i & 2;
                } else {
                    source = (i & 2) + 1;
                }
                for (j = 0; j < 6; j++) {
                    battle->players->base.players[second + (i >> 1)].order[j] = battle->setup->monStatus[source][j];
                }
            }
        }
        if (queue) {
            QueueGameProc(wk->manager, OVERLAY_ID(305), &CommBtlDemoProc, battle->players);
            running = PROCMGR_PROC_ENDED;
            *state = BATTLE_WAIT_RESULT_DEMO;
        }
        break;
    case BATTLE_WAIT_RESULT_DEMO:
        if (running == PROCMGR_RUNNING) {
            break;
        }
        *state = BATTLE_END;
        break;
    case 13:
        break;
    case BATTLE_END:
        return TRUE;
    case BATTLE_WAIT_ERROR:
        if (wk->timer++ > 1800) {
            GFL_NetErrMarkShown();
            battle->result = WIFIBATTLEMATCH_BATTLE_RESULT_ERROR;
            return TRUE;
        }
        break;
    }
    if (GFL_NetIsInitialized() && running != PROCMGR_RUNNING) {
        if (GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC) {
            if (GFL_NetErrCheck()) {
                GFL_NetErrMarkShown();
                battle->result = WIFIBATTLEMATCH_BATTLE_RESULT_IRC_ERROR;
                return TRUE;
            }
        } else {
            switch (DWCRapCommon_CheckError(TRUE, TRUE)) {
            case DWCRAP_RESULT_OK:
                break;
            case DWCRAP_RESULT_DISCONNECTED:
            case DWCRAP_RESULT_FAILED:
            case DWCRAP_RESULT_NET_ERROR:
            default:
                battle->result = WIFIBATTLEMATCH_BATTLE_RESULT_ERROR;
                return TRUE;
            case DWCRAP_RESULT_TIMEOUT:
                battle->result = WIFIBATTLEMATCH_BATTLE_RESULT_WIFI_TIMEOUT;
                return TRUE;
            }
        }
    }
    return FALSE;
}
