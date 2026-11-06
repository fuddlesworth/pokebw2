#include "app/pokelist.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "field/hidden_event.h"
#include "field/musical.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_sync.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/join_avenue.h"
#include "save/trainer_card.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/btl_main_11.h"

// The party list's screen: its graphics, the cursor, and what each mode does with the Pokémon picked. The ROM
// doesn't name this file; it is named for the list's system, which pokelist.c's proc runs

// The common touch bar and the party cursor use separate animation resources
enum {
    TOUCHBAR_ANM_CLOSE = 0,
    TOUCHBAR_ANM_BACK = 1,
    TOUCHBAR_ANM_SHORTCUT_OFF = 6,
    TOUCHBAR_ANM_SHORTCUT_ON = 7,
    TOUCHBAR_ANM_CLOSE_PRESS = 8,
    TOUCHBAR_ANM_BACK_PRESS = 9,
};

enum {
    CURSOR_ANM_FIRST,
    CURSOR_ANM_OTHER,
    CURSOR_ANM_SUB_FIRST,
    CURSOR_ANM_SUB_OTHER,
};

#define TM_INDEX_NONE 0xff

static void PokeList_UpdateGlow(PokeListWork *wk);
static void PokeList_VBlank(TCB *tcb, void *data);
static void PokeList_InitGraphics(PokeListWork *wk);
static void PokeList_ExitGraphics(PokeListWork *wk);
static void PokeList_CreateBG(const BGSetup *setup, u8 bg, u8 mode);
static void PokeList_CreateActors(PokeListWork *wk);
static void PokeList_FreeActors(PokeListWork *wk);
static void PokeList_InitText(PokeListWork *wk);
static void PokeList_ExitText(PokeListWork *wk);
static void PokeList_LoadResources(PokeListWork *wk);
static void PokeList_FreeResources(PokeListWork *wk);
static void PokeList_InitMode(PokeListWork *wk);
static void PokeList_ShowModeMessage(PokeListWork *wk);
static void PokeList_SelectPokemon(PokeListWork *wk);
static void PokeList_OpenMenu(PokeListWork *wk);
static void PokeList_UpdateSelect(PokeListWork *wk);
static void PokeList_RefreshCursor(PokeListWork *wk);
static void PokeList_HandleSelectionInput(PokeListWork *wk);
static void PokeList_HandleSwapSelection(PokeListWork *wk);
static int PokeList_CheckFusionPartner(PokeListWork *wk, u8 pos);
static void PokeList_HandleFusionSelection(PokeListWork *wk);
static void PokeList_HandleHpShareSelection(PokeListWork *wk);
static void PokeList_ConfirmBattleTeam(PokeListWork *wk);
static void PokeList_MoveHeldItem(PokeListWork *wk, PartyPkm *from, PartyPkm *to, u16 item);
static void PokeList_SwapHeldItems(PokeListWork *wk, PartyPkm *pkmA, PartyPkm *pkmB, u16 itemA, u16 itemB);
static void PokeList_UpdateSwappedItemForms(PartyPkm *pkmA, PartyPkm *pkmB);
static void PokeList_ShowItemSwapError(PokeListWork *wk, u32 msgId);
static void PokeList_HandleItemSwapSelection(PokeListWork *wk);
static void PokeList_ReadSelectionInput(PokeListWork *wk);
static void PokeList_HandleConfirmKey(PokeListWork *wk);
static void PokeList_ReadCursorKeys(PokeListWork *wk);
static void PokeList_HandleTouch(PokeListWork *wk);
static void PokeList_ShowCursor(PokeListWork *wk, s32 pos);
static void PokeList_ShowSubCursor(PokeListWork *wk, s32 pos);
static void PokeList_UpdateMenu(PokeListWork *wk);
static void PokeList_BeginMenu(PokeListWork *wk);
static void PokeList_EndMenu(PokeListWork *wk);
static void PokeList_DoMenuItem(PokeListWork *wk);
static void PokeList_WaitMessage(PokeListWork *wk);
static void PokeList_UpdateSubMenu(PokeListWork *wk);
static void PokeList_OpenSubMenu(PokeListWork *wk, void (*func)(PokeListWork *wk, u32 item));
static void PokeList_UpdateSwap(PokeListWork *wk);
static void PokeList_StartSwap(PokeListWork *wk);
static void PokeList_EndSwap(PokeListWork *wk);
static void PokeList_AnswerReplaceMove(PokeListWork *wk, u32 item);
static void PokeList_AskCancelLearning(PokeListWork *wk);
static void PokeList_AnswerCancelLearning(PokeListWork *wk, u32 item);
static void PokeList_AnswerSwapItem(PokeListWork *wk, u32 item);
static void PokeList_ShareHpEnd(PokeListWork *wk);
static void PokeList_AnswerTakeMail(PokeListWork *wk, u32 item);
static void PokeList_AskDeleteMail(PokeListWork *wk);
static void PokeList_AnswerDeleteMail(PokeListWork *wk, u32 item);
static void PokeList_SetMove(PokeListWork *wk, PartyPkm *pkm, u8 slot);
static void PokeList_SubFromBag(PokeListWork *wk, u16 item);
static void PokeList_AddToBag(PokeListWork *wk, u16 item);
static BOOL PokeList_HasBattleCancelButton(PokeListWork *wk);
static BOOL PokeList_HasBackButton(PokeListWork *wk);
static void PokeList_LearnMove(PokeListWork *wk, PartyPkm *pkm);
static void PokeList_RaiseFriendship(PokeListWork *wk, PartyPkm *pkm);
static void PokeList_LearnInSlot(PokeListWork *wk);
static void PokeList_AskSwapItem(PokeListWork *wk);
static void PokeList_ShareHpDone(PokeListWork *wk);
static void PokeList_AskTakeMail(PokeListWork *wk);
static void PokeList_MessageDoneDemo(PokeListWork *wk);
static void PokeList_SetHeldItem(PokeListWork *wk, PartyPkm *pkm, u16 item);
static u16 PokeList_GetBagCount(PokeListWork *wk, u16 item);

// BG 0's setup, which PokeList_CreateBG0 builds, again after plist_demo.c's 3D demo. The other BGs' setups are statics
// of PokeList_InitGraphics; this one is declared ahead of them, and its place there sets its place in .rodata
static const BGSetup sBG0Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xf, 6, 0x8000, 0, 0, 0, 0 };

// The moves outside battle, by the index of their check in CheckAllowHMEvent
static const u16 sHMMoves[] = {
    MOVE_CUT, MOVE_SURF,        MOVE_WATERFALL, MOVE_STRENGTH, MOVE_FLY,        MOVE_FLASH,      MOVE_TELEPORT,
    MOVE_DIG, MOVE_SWEET_SCENT, MOVE_CHATTER,   MOVE_DIVE,     MOVE_MILK_DRINK, MOVE_SOFTBOILED,
};

BOOL PokeList_Init(PokeListWork *wk) {
    u8 count = PokeParty_GetPkmCount(wk->param->party);
    u8 i;

    wk->touch = GCTX_HIDIsUsingTouch();
    wk->menuItem = POKELIST_MENU_NONE;
    wk->state = POKELIST_STATE_FADE_IN;
    wk->subState = POKELIST_SUBSTATE_INIT;
    wk->cursorPos = 0;
    wk->pkm = NULL;
    wk->selectPos = POKELIST_CURSOR_NONE;
    wk->selectPos2 = POKELIST_CURSOR_NONE;
    wk->showCloseAndShortcut = FALSE;
    wk->glowPaused = FALSE;
    wk->enteredCount = 0;
    wk->flashTimer = 0;
    wk->glowAngle = 0;
    wk->unk114 = 0;
    wk->waitTimer = 0;
    wk->hpDoneFunc = NULL;
    wk->statsWindow = NULL;
    wk->pressedButton = NULL;
    wk->buttons[0] = NULL;
    wk->buttons[1] = NULL;
    wk->decided = FALSE;
    wk->demo = POKELIST_DEMO_NONE;
    wk->timeUp = FALSE;
    wk->syncOnTimeUp = FALSE;
    wk->param->forceExit = FALSE;
    wk->allowEggs = TRUE;
    wk->playHealSe = FALSE;
    for (i = 0; i < POKELIST_CL_RES_COUNT; i++) {
        wk->clResources[i] = CL_RES_NONE;
    }
    wk->wasMusicalMode = wk->param->mode == POKELIST_MODE_MUSICAL ? TRUE : FALSE;
    wk->wasEggPromptMode = wk->param->mode == POKELIST_MODE_SELECT_EGG_PROMPT ? TRUE : FALSE;
    if (wk->param->mode == POKELIST_MODE_MOVE_TUTOR) {
        wk->param->mode = POKELIST_MODE_SELECT;
        wk->wasTutorMode = TRUE;
    } else {
        wk->wasTutorMode = FALSE;
    }
    if (wk->param->mode == POKELIST_MODE_NETWORK_TEAM) {
        wk->param->mode = POKELIST_MODE_BATTLE_TEAM;
        wk->wasNetworkTeamMode = TRUE;
    } else {
        wk->wasNetworkTeamMode = FALSE;
    }
    if (wk->param->mode == POKELIST_MODE_TEACH_TM && wk->param->move == 0) {
        wk->param->move = PML_ItemGetTMMoveID(wk->param->item);
    }
    PokeList_InitGraphics(wk);
    PokeList_LoadResources(wk);
    PokeList_InitText(wk);
    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        if (i < count) {
            PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, i);

            wk->plates[i] = PokeListPlate_Create(wk, i, pkm);
        } else {
            wk->plates[i] = PokeListPlate_CreateEmpty(wk, i);
        }
        if (PokeListPlate_GetEntry(wk->plates[i]) <= 5) {
            wk->enteredCount++;
        }
    }
    wk->taskMenuRes = AppTaskMenuRes_Create(0, 1, wk->font, wk->printQueue, wk->heapId);
    PokeList_CreateActors(wk);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListBattle_Init(wk);
    }
    wk->vblankTask = GFL_VBlankTCBAdd(PokeList_VBlank, wk, 8);
    wk->message = PokeListMessage_Create(wk);
    wk->menu = PokeListMenu_Create(wk);
    PokeList_InitMode(wk);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysQueueScrLoad(7);
    GFL_NetSetIconScreen(TRUE, wk->heapId);
    return TRUE;
}

BOOL PokeList_Exit(PokeListWork *wk) {
    u8 i;

    GFL_TCBRemove(wk->vblankTask);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListMenu_FreeButton(wk->buttons[0]);
        PokeListMenu_FreeButton(wk->buttons[1]);
        wk->buttons[0] = NULL;
        wk->buttons[1] = NULL;
    }
    PokeListMenu_Free(wk, wk->menu);
    PokeListMessage_Free(wk, wk->message);
    if (wk->statsWindow != NULL) {
        BmpWin_Free(wk->statsWindow);
    }
    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        PokeListPlate_Free(wk, wk->plates[i]);
    }
    AppTaskMenuRes_Free(wk->taskMenuRes);
    PokeList_ExitText(wk);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListBattle_Exit(wk);
    }
    PokeList_FreeActors(wk);
    PokeList_FreeResources(wk);
    PokeList_ExitGraphics(wk);
    if (wk->wasTutorMode == TRUE) {
        wk->param->mode = POKELIST_MODE_MOVE_TUTOR;
    }
    if (wk->wasMusicalMode == TRUE) {
        wk->param->mode = POKELIST_MODE_MUSICAL;
    }
    if (wk->wasEggPromptMode == TRUE) {
        wk->param->mode = POKELIST_MODE_SELECT_EGG_PROMPT;
    }
    if (wk->wasNetworkTeamMode == TRUE) {
        wk->param->mode = POKELIST_MODE_NETWORK_TEAM;
    }
    GCTX_HIDSetUsingTouch(wk->touch);
    return TRUE;
}

BOOL PokeList_Main(PokeListWork *wk) {
    u8 i;

    switch (wk->state) {
    case POKELIST_STATE_FADE_IN:
        GFL_WipeSet(WIPE_MODE_SUB_FIRST, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
        wk->state = POKELIST_STATE_WAIT_WIPE;
        break;
    case POKELIST_STATE_WAIT_WIPE:
        if (GFL_WipeIsFinished() == TRUE) {
            wk->state = wk->nextState;
        }
        break;
    case POKELIST_STATE_SELECT:
    case POKELIST_STATE_SELECT_SWAP:
    case POKELIST_STATE_SELECT_HP_SHARE:
    case POKELIST_STATE_SELECT_ITEM_SWAP:
    case POKELIST_STATE_SELECT_FUSION:
        PokeList_UpdateSelect(wk);
        break;
    case POKELIST_STATE_SWAP:
        PokeList_UpdateSwap(wk);
        break;
    case POKELIST_STATE_MENU:
        PokeList_UpdateMenu(wk);
        break;
    case POKELIST_STATE_MESSAGE:
        PokeList_WaitMessage(wk);
        break;
    case POKELIST_STATE_IDLE:
        break;
    case POKELIST_STATE_SUBMENU:
        PokeList_UpdateSubMenu(wk);
        break;
    case POKELIST_STATE_HP_CHANGE_START:
        PokeListPlate_StartHpChange(wk, wk->plates[wk->cursorPos]);
        if (wk->playHealSe == TRUE) {
            wk->playHealSe = FALSE;
            GFL_SndSEPlay(SEQ_SE_RECOVERY);
        }
        wk->state = POKELIST_STATE_HP_CHANGE;
        // fallthrough
    case POKELIST_STATE_HP_CHANGE:
        if (PokeListPlate_UpdateHpChange(wk, wk->plates[wk->cursorPos]) == TRUE) {
            if (wk->hpDoneFunc != NULL) {
                wk->hpDoneFunc(wk);
            } else {
                wk->state = POKELIST_STATE_EXIT_WAIT_WIPE;
            }
        }
        break;
    case POKELIST_STATE_WAIT_TEAM_BUTTON:
        wk->waitTimer++;
        if (wk->waitTimer > 16) {
            PokeListMenu_SetButtonPressed(wk->buttons[0], 0);
            if (wk->wasNetworkTeamMode == TRUE) {
                // Overlay 164's sync takes the parameters as its work
                if (NetSync_Send((NetSyncWork *)wk->param, 0, 0) == TRUE) {
                    wk->state = POKELIST_STATE_SYNC_START;
                    PokeListMenu_FreeButton(wk->buttons[0]);
                    PokeListMenu_FreeButton(wk->buttons[1]);
                    wk->buttons[0] = NULL;
                    wk->buttons[1] = NULL;
                    PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
                    PokeListMessage_Print(wk, wk->message, BtlMain11_Text_CommunicatingPleaseWait);
                    PokeListMessage_ShowWaitIcon(wk, wk->message);
                }
            } else {
                wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
            }
        }
        break;
    case POKELIST_STATE_LEVEL_UP:
        PokeList_UpdateLevelUp(wk);
        break;
    case POKELIST_STATE_DEMO_START:
        PokeListDemo_Start(wk);
        break;
    case POKELIST_STATE_DEMO_UPDATE:
        PokeListDemo_Update(wk);
        break;
    case POKELIST_STATE_DEMO_END:
        PokeListDemo_End(wk);
        break;
    case POKELIST_STATE_SYNC_START:
        NetSync_StartTiming((u32)wk->param, 2);
        wk->state = POKELIST_STATE_SYNC_WAIT;
        break;
    case POKELIST_STATE_SYNC_WAIT:
        if (NetSync_IsTimingDone((NetSyncWork *)wk->param, 2) == TRUE) {
            wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        }
        break;
    case POKELIST_STATE_EXIT_FADE:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
        wk->state = POKELIST_STATE_EXIT_WAIT_WIPE;
        break;
    case POKELIST_STATE_EXIT_WAIT_BUTTON:
        if (wk->pressedButton == NULL || ClActor_IsAnimActive(wk->pressedButton) == FALSE) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
            wk->state = POKELIST_STATE_EXIT_WAIT_WIPE;
        }
        break;
    case POKELIST_STATE_EXIT_WAIT_WIPE:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }
    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        PokeListPlate_Update(wk, wk->plates[i]);
    }
    PokeListMessage_Update(wk, wk->message);
    PokeList_UpdateGlow(wk);
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListBattle_Update(wk);
        PokeListMenu_UpdateButton(wk->buttons[0]);
        PokeListMenu_UpdateButton(wk->buttons[1]);
    }
    PrintQueue_Main(wk->printQueue);
    ClActSys_Draw();
    return FALSE;
}

// Blends the plates' palettes toward white while a plate flashes, and makes the cursor's plate glow
static void PokeList_UpdateGlow(PokeListWork *wk) {
    MIi_CpuCopy16(wk->baseColors, wk->colors, sizeof(wk->colors));
    if (wk->flashTimer != 0) {
        u8 level;
        u8 i;
        u8 j;

        wk->flashTimer--;
        level = wk->flashTimer;
        if (level > 6) {
            level = 12 - level;
        }
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 16; j++) {
                if (j < 2 || j > 4) {
                    u16 color = wk->baseColors[i][j];
                    u8 g = (color & 0x3e0) >> 5;
                    u8 b = (color & 0x7c00) >> 10;
                    u8 r = color & 0x1f;

                    r = r + level * 2 > 31 ? 31 : r + level * 2;
                    g = g + level * 2 > 31 ? 31 : g + level * 2;
                    b = b + level * 2 > 31 ? 31 : b + level * 2;
                    wk->colors[i][j] = GX_RGB(r, g, b);
                }
            }
        }
    }
    {
        // The glow angle's step. Its reads are folded, but a function's static constant stays in .rodata
        static const u16 angleStep = 0x400;
        u16 glowColors[3] = { 0x7ff4, 0x7fff, 0x7be9 };
        u16 baseColors[3] = { 0x4a0a, 0x5ef7, 0x4140 };
        u8 i;
        u8 j;
        fx32 ratio;

        if (wk->glowPaused == FALSE) {
            if (wk->glowAngle + angleStep >= 0x10000) {
                wk->glowAngle = wk->glowAngle + angleStep - 0x10000;
            } else {
                wk->glowAngle = wk->glowAngle + angleStep;
            }
        } else {
            wk->glowAngle = 0xc000;
        }
        ratio = (FX_SinIdx(wk->glowAngle) + FX32_ONE) / 2;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                u16 glow = glowColors[j];
                u16 base = baseColors[j];
                u8 glowR = glow & 0x1f;
                u8 glowG = (glow & 0x3e0) >> 5;
                u8 glowB = (glow & 0x7c00) >> 10;
                u8 baseR = base & 0x1f;
                u8 baseG = (base & 0x3e0) >> 5;
                u8 baseB = (base & 0x7c00) >> 10;
                s8 dr = (baseR - glowR) * ratio >> FX32_SHIFT;
                s8 dg = (baseG - glowG) * ratio >> FX32_SHIFT;
                s8 db = (baseB - glowB) * ratio >> FX32_SHIFT;
                u8 r = MATH_CLAMP(glowR + dr, 0, 31);
                u8 g = MATH_CLAMP(glowG + dg, 0, 31);
                u8 b = MATH_CLAMP(glowB + db, 0, 31);

                wk->colors[i][2 + j] = GX_RGB(r, g, b);
            }
        }
    }
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 0xc0, wk->colors, sizeof(wk->colors));
}

static void PokeList_VBlank(TCB *tcb, void *data) {
    PokeListWork *wk = data;

    if (wk->glowPaused == TRUE) {
        GX_SetVisibleWnd(GX_WNDMASK_W0);
    } else {
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
    }
    ClActSys_VBlank();
}

static void PokeList_InitGraphics(PokeListWork *wk) {
    static const BGSysLCDConfig sLCDConfig = { 1, 0, 0, 0 };
    static const BGSetup sBG1Setup = { 0, 0, 0x1000, 0, 3, GX_BG_COLORMODE_16, 0xa, 2, 0x8000, 0, 1, 0, 0 };
    static const BGSetup sBG2Setup = { 0, 0, 0x1000, 0, 3, GX_BG_COLORMODE_16, 0xc, 4, 0x8000, 0, 2, 1, 0 };
    static const BGSetup sBG3Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xe, 0, 0x5000, 1, 3, 0, 0 };
    static const BGSetup sBG4Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xf, 6, 0x8000, 1, 0, 0, 0 };
    static const BGSetup sBG5Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xe, 4, 0x8000, 1, 1, 0, 0 };
    static const BGSetup sBG6Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xd, 2, 0x8000, 1, 2, 0, 0 };
    static const BGSetup sBG7Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xc, 0, 0x6000, 1, 3, 0, 0 };
    BGSysVRAMConfig vramConfig = {
        GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_NONE,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
    };
    ClActSysSetup clactSetup;

    if (PokeList_IsBattle(wk) == TRUE) {
        vramConfig.objSub = GX_VRAM_SUB_OBJ_128_D;
        vramConfig.texture = GX_VRAM_TEX_NONE;
    } else {
        vramConfig.objSub = GX_VRAM_SUB_OBJ_16_I;
    }
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GFL_BGSysSetVRAMBanks(&vramConfig);
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    PokeList_CreateBG(&sBG3Setup, 3, 0);
    PokeList_CreateBG(&sBG2Setup, 2, 0);
    PokeList_CreateBG(&sBG1Setup, 1, 0);
    PokeList_CreateBG(&sBG7Setup, 7, 0);
    PokeList_CreateBG(&sBG6Setup, 6, 0);
    PokeList_CreateBG(&sBG5Setup, 5, 0);
    PokeList_CreateBG(&sBG4Setup, 4, 0);
    PokeList_CreateBG0(wk);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, GX_PLANEMASK_BG3, 16, 10);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 16, 10);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, 128);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, 128);
    clactSetup = gClActSysSetupSplitScreens;
    ClActSys_Create(&clactSetup, &vramConfig, wk->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    NNS_GfdClearVramTransferManagerTask();
}

void PokeList_CreateBG0(PokeListWork *wk) {
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
    PokeList_CreateBG(&sBG0Setup, 0, 0);
}

void PokeList_Init3D(PokeListWork *wk) {
    static const VecFx32 sCameraPos = { 0, 0, FX32_CONST(128) };
    static const VecFx32 sCameraUp = { 0, FX32_ONE, 0 };
    static const VecFx32 sCameraTarget = { 0, 0, 0 };
    // Nothing reads it, but the original's .rodata has these 16 zero bytes after the LCD config. Its type and name are
    // a guess: the eight colors that G3X_SetEdgeColorTable takes, for the edge marking turned on below
    static const GXRgb sEdgeColors[8] = { 0 };

    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, wk->heapId, NULL);
    G2_SetBG0Priority(0);
    G3X_EdgeMarking(TRUE);
    G3X_AntiAlias(TRUE);
    G3X_AlphaBlend(TRUE);
    wk->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, 0, 0x50d7, 0, 0x6bca, FX32_ONE, FX32_CONST(128), 0,
                                     &sCameraPos, &sCameraUp, &sCameraTarget, wk->heapId);
    GFL_G3DCameraFlush(wk->camera);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2,
                       GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 16, 10);
    Particle_Init(wk->heapId);
}

static void PokeList_ExitGraphics(PokeListWork *wk) {
    NNS_GfdClearVramTransferManagerTask();
    ClActSys_Delete();
    PokeList_ReleaseBG0(wk);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

void PokeList_ReleaseBG0(PokeListWork *wk) {
    GFL_BGSysReleaseBG(0);
}

void PokeList_Exit3D(PokeListWork *wk) {
    Particle_Free();
    GFL_G3DCameraFree(wk->camera);
    GFL_G3DSysFree();
    GFL_BGSysReleaseBG(0);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, GX_PLANEMASK_BG3, 16, 10);
}

static void PokeList_CreateBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void PokeList_CreateActors(PokeListWork *wk) {
    ClActorSetup cursorSetup;
    ClActorSetup setup;

    wk->actorUnit = ClActUnit_Create(10, 8, wk->heapId);
    ClActUnit_UseSystemRenderer(wk->actorUnit);
    cursorSetup.x = 0;
    cursorSetup.y = 0;
    cursorSetup.sequence = CURSOR_ANM_FIRST;
    cursorSetup.priority = 10;
    cursorSetup.bgPriority = 3;
    wk->cursor = ClActor_Create(wk->actorUnit, wk->clResources[CL_RES_CHAR(0)], wk->clResources[CL_RES_PLTT(0)],
                                wk->clResources[CL_RES_CELL(0)], &cursorSetup, 0, wk->heapId);
    wk->subCursor = ClActor_Create(wk->actorUnit, wk->clResources[CL_RES_CHAR(0)], wk->clResources[CL_RES_PLTT(0)],
                                   wk->clResources[CL_RES_CELL(0)], &cursorSetup, 0, wk->heapId);
    ClActor_SetAutoAnim(wk->cursor, TRUE);
    ClActor_SetAutoAnim(wk->subCursor, TRUE);
    ClActor_SetVisible(wk->cursor, FALSE);
    ClActor_SetVisible(wk->subCursor, FALSE);
    setup.x = 224;
    setup.y = 168;
    setup.sequence = TOUCHBAR_ANM_BACK;
    setup.priority = 0;
    setup.bgPriority = 0;
    wk->backButton = ClActor_Create(wk->actorUnit, wk->clResources[CL_RES_CHAR(2)], wk->clResources[CL_RES_PLTT(1)],
                                    wk->clResources[CL_RES_CELL(2)], &setup, 0, wk->heapId);
    ClActor_SetAutoAnim(wk->backButton, TRUE);
    setup.x = 204;
    setup.sequence = TOUCHBAR_ANM_CLOSE;
    wk->closeButton = ClActor_Create(wk->actorUnit, wk->clResources[CL_RES_CHAR(2)], wk->clResources[CL_RES_PLTT(1)],
                                     wk->clResources[CL_RES_CELL(2)], &setup, 0, wk->heapId);
    ClActor_SetAutoAnim(wk->closeButton, TRUE);
    setup.y = 172;
    setup.x = 184;
    setup.sequence = TOUCHBAR_ANM_SHORTCUT_OFF;
    wk->shortcutMark = ClActor_Create(wk->actorUnit, wk->clResources[CL_RES_CHAR(2)], wk->clResources[CL_RES_PLTT(1)],
                                      wk->clResources[CL_RES_CELL(2)], &setup, 0, wk->heapId);
    ClActor_SetAutoAnim(wk->shortcutMark, TRUE);
}

static void PokeList_FreeActors(PokeListWork *wk) {
    ClActor_Delete(wk->backButton);
    ClActor_Delete(wk->closeButton);
    ClActor_Delete(wk->shortcutMark);
    ClActor_Delete(wk->cursor);
    ClActor_Delete(wk->subCursor);
    ClActUnit_Delete(wk->actorUnit);
}

static void PokeList_InitText(PokeListWork *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_11, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1c0, 0x20, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    wk->printQueue = PrintQueue_Create(0x1800, wk->heapId);
    GFL_TextRndResetColorIndexLUT();
}

static void PokeList_ExitText(PokeListWork *wk) {
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->smallFont);
    GFL_FontFree(wk->font);
}

static void PokeList_LoadResources(PokeListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PLIST, wk->heapId);
    ArcTool *iconArc;
    ArcTool *commonArc;

    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0, 0, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0x1a0, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0xf, 3, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x14, 3, 0, 0, FALSE, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0xc, 2, 0, 0, FALSE, wk->heapId);
    wk->plateScreenFile = GFL_G2DIOReadNSCRArc(arc, 0x12, FALSE, &wk->plateScreen, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, 4, 0, 0, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0xe, 7, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x13, 7, 0, 0, FALSE, wk->heapId);
    wk->clResources[CL_RES_PLTT(0)] = ClActSys_LoadPlttRange(arc, 4, 0, 0, 0, 3, wk->heapId);
    wk->clResources[CL_RES_CHAR(0)] = ClActSys_LoadChars(arc, 0xb, FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CHAR(1)] = ClActSys_LoadChars(arc, 8, FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(0)] = ClActSys_LoadCellAnims(arc, 0x17, 0x1a, wk->heapId);
    wk->clResources[CL_RES_CELL(1)] = ClActSys_LoadCellAnims(arc, 0x16, 0x19, wk->heapId);
    GFL_ArcToolFree(arc);

    iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);
    wk->clResources[CL_RES_PLTT(6)] = ClActSys_LoadPlttEx(iconArc, PokeIcon_GetPaletteFile(), 0, 0xc0, wk->heapId);
    wk->clResources[CL_RES_CELL(7)] =
        ClActSys_LoadCellAnims(iconArc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), wk->heapId);
    GFL_ArcToolFree(iconArc);

    commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    // BUG: The BG of the common bar is loaded from the closed handle of the first archive, rather than the common
    // archive's, which works as long as the handle's memory is given to the new handle
#ifdef BUGFIX
    GFL_G2DIOLoadArcNCLRDefault(commonArc, AppMenuCommon_GetBarBgPaletteFile(), 0, 0x160, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(commonArc, AppMenuCommon_GetBarBgCharFile(), 1, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(commonArc, AppMenuCommon_GetStatusExtraFileId(), 1, 0, 0, FALSE, wk->heapId);
#else
    GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), 0, 0x160, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, AppMenuCommon_GetBarBgCharFile(), 1, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, AppMenuCommon_GetStatusExtraFileId(), 1, 0, 0, FALSE, wk->heapId);
#endif
    GFL_BGSysSetScrPaletteNo(1, 0, 0, 0x40, 0x20, 0xb);
    wk->clResources[CL_RES_PLTT(5)] =
        ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetHpBarPaletteFile(), 0, 0xa0, 0, 1, wk->heapId);
    wk->clResources[CL_RES_CHAR(6)] =
        ClActSys_LoadChars(commonArc, AppMenuCommon_GetHpBarCharFile(2), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(6)] = ClActSys_LoadCellAnims(commonArc, AppMenuCommon_GetHpBarCellFile(2),
                                                             AppMenuCommon_GetHpBarAnimFile(2), wk->heapId);
    wk->clResources[CL_RES_PLTT(1)] =
        ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetTouchBarPaletteFile(), 0, 0x120, 0, 3, wk->heapId);
    wk->clResources[CL_RES_CHAR(2)] =
        ClActSys_LoadChars(commonArc, AppMenuCommon_GetTouchBarCharFile(), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(2)] = ClActSys_LoadCellAnims(commonArc, AppMenuCommon_GetTouchBarCellFile(2),
                                                             AppMenuCommon_GetTouchBarAnimFile(2), wk->heapId);
    wk->clResources[CL_RES_PLTT(2)] =
        ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetItemIconPaletteFile(), 0, 0x60, 0, 1, wk->heapId);
    wk->clResources[CL_RES_CHAR(3)] =
        ClActSys_LoadChars(commonArc, AppMenuCommon_GetItemIconCharFile(), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(3)] = ClActSys_LoadCellAnims(commonArc, AppMenuCommon_GetItemIconCellFile(2),
                                                             AppMenuCommon_GetItemIconAnimFile(2), wk->heapId);
    wk->clResources[CL_RES_PLTT(4)] =
        ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetStatusIconPaletteFile(), 0, 0x80, 0, 1, wk->heapId);
    wk->clResources[CL_RES_CHAR(5)] =
        ClActSys_LoadChars(commonArc, AppMenuCommon_GetStatusIconCharFile(), FALSE, 0, wk->heapId);
    wk->clResources[CL_RES_CELL(5)] = ClActSys_LoadCellAnims(commonArc, AppMenuCommon_GetStatusIconCellFile(2),
                                                             AppMenuCommon_GetStatusIconAnimFile(2), wk->heapId);
    GFL_ArcToolFree(commonArc);
    MIi_CpuCopy16((void *)(HW_BG_PLTT + 0xc0), wk->baseColors, sizeof(wk->baseColors));
}

static void PokeList_FreeResources(PokeListWork *wk) {
    u8 i;

    GFL_HeapFree(wk->plateScreenFile);
    for (i = 0; i < CL_RES_PLTT_COUNT; i++) {
        if (wk->clResources[i] != CL_RES_NONE) {
            ClActSys_FreePltt(wk->clResources[i]);
        }
    }
    for (i = CL_RES_CHAR(0); i < CL_RES_CHAR(CL_RES_CHAR_COUNT); i++) {
        if (wk->clResources[i] != CL_RES_NONE) {
            ClActSys_FreeChars(wk->clResources[i]);
        }
    }
    for (i = CL_RES_CELL(0); i < CL_RES_CELL(CL_RES_CELL_COUNT); i++) {
        if (wk->clResources[i] != CL_RES_NONE) {
            ClActSys_FreeCellAnims(wk->clResources[i]);
        }
    }
}

static void PokeList_InitMode(PokeListWork *wk) {
    switch (wk->param->mode) {
    case POKELIST_MODE_BATTLE_TEAM:
        wk->buttons[0] = PokeListMenu_CreateButton(wk, wk->menu, BtlMain11_Text_Confirm, 22, 21, 0);
        PokeList_ShowModeMessage(wk);
        wk->nextState = POKELIST_STATE_SELECT;
        break;
    case POKELIST_MODE_BATTLE_TEAM_CANCEL:
    case POKELIST_MODE_BATTLE_INSTITUTE:
        wk->buttons[0] = PokeListMenu_CreateButton(wk, wk->menu, BtlMain11_Text_Confirm, 12, 21, 0);
        wk->buttons[1] = PokeListMenu_CreateButton(wk, wk->menu, BtlMain11_Text_Cancel, 22, 21, 1);
        PokeList_ShowModeMessage(wk);
        wk->nextState = POKELIST_STATE_SELECT;
        break;
    case POKELIST_MODE_FIELD:
    case POKELIST_MODE_TEACH_TM:
    case POKELIST_MODE_GIVE_ITEM:
    case POKELIST_MODE_GIVE_MAIL:
    case POKELIST_MODE_EVOLUTION_ITEM:
    case POKELIST_MODE_DAY_CARE:
    case POKELIST_MODE_SELECT_EGG:
    case POKELIST_MODE_MUSICAL:
    case POKELIST_MODE_SELECT_EGG_PROMPT:
        PokeList_ShowModeMessage(wk);
        wk->nextState = POKELIST_STATE_SELECT;
        break;
    case POKELIST_MODE_SELECT:
        wk->showCloseAndShortcut = FALSE;
        PokeList_ShowModeMessage(wk);
        wk->nextState = POKELIST_STATE_SELECT;
        break;
    case POKELIST_MODE_USE_ITEM:
        if (PokeList_IsItemForParty(wk, wk->param->item) == TRUE) {
            s32 pos = PokeList_FindItemTarget(wk);

            if (pos != -1) {
                wk->cursorPos = pos;
                wk->pkm = PokeParty_GetPkm(wk->param->party, pos);
                PokeList_ShowItemResult(wk, 0);
                StatusRcv_UseItem(wk->pkm, wk->param->item, 0, wk->param->zoneId, wk->heapId);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                wk->playHealSe = TRUE;
                wk->nextState = wk->state;
                wk->state = POKELIST_STATE_FADE_IN;
                PokeList_SubFromBag(wk, wk->param->item);
            } else {
                PokeList_ShowItemUselessExit(wk);
                wk->nextState = wk->state;
                wk->state = POKELIST_STATE_FADE_IN;
            }
        } else if (PokeList_GetBagCount(wk, wk->param->item)) {
            PokeList_ShowModeMessage(wk);
            wk->nextState = POKELIST_STATE_SELECT;
        } else {
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            PokeListMessage_Close(wk, wk->message);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetItemName(wk, wk->message, 0, wk->param->item);
            PokeList_ShowMessage(wk, BtlMain11_Text_UsedLast, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            wk->nextState = wk->state;
            wk->state = POKELIST_STATE_FADE_IN;
        }
        break;
    case POKELIST_MODE_REPLACE_TM_MOVE:
    case POKELIST_MODE_REPLACE_LEVEL_MOVE:
        wk->cursorPos = wk->param->index;
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->param->index);
        if (wk->param->moveSlot < 4) {
            u32 move = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + wk->param->moveSlot, NULL);

            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, move);
            PokeList_ShowMessage(wk, BtlMain11_Text_12TaDa, TRUE, PokeList_LearnInSlot);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, BtlMain11_Text_GiveUpLearningMove, FALSE, PokeList_AskCancelLearning);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        wk->nextState = POKELIST_STATE_MESSAGE;
        wk->state = POKELIST_STATE_FADE_IN;
        wk->showCloseAndShortcut = FALSE;
        PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
        break;
    case POKELIST_MODE_GIVE_FROM_BAG:
        wk->cursorPos = wk->param->index;
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->param->index);
        if (wk->param->item == 0) {
            wk->param->mode = POKELIST_MODE_FIELD;
            PokeList_ShowModeMessage(wk);
            wk->nextState = POKELIST_STATE_SELECT;
        } else {
            u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

            if (heldItem == 0) {
                PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                PokeListMessage_CreateWordSet(wk, wk->message);
                PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
                PokeListMessage_SetItemName(wk, wk->message, 1, wk->param->item);
                PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
                PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
                if (PokeListDemo_CanBecomeOrigin(wk, wk->pkm) == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, BtlMain11_Text_GivenHold, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = POKELIST_DEMO_GIRATINA_ORIGIN;
                } else {
                    PokeList_ShowMessage(wk, BtlMain11_Text_GivenHold, TRUE, PokeList_MessageDoneSelect);
                }
                PokeListMessage_FreeWordSet(wk, wk->message);
                wk->param->mode = POKELIST_MODE_FIELD;
            } else {
                PokeListMessage_CreateWordSet(wk, wk->message);
                PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
                PokeListMessage_SetItemTextName(wk, wk->message, 1, heldItem);
                PokeList_ShowMessage(wk, BtlMain11_Text_AlreadyHoldingWouldLike, FALSE, PokeList_AskSwapItem);
                PokeListMessage_FreeWordSet(wk, wk->message);
            }
        }
        wk->nextState = POKELIST_STATE_MESSAGE;
        wk->state = POKELIST_STATE_FADE_IN;
        wk->showCloseAndShortcut = TRUE;
        PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
        break;
    case POKELIST_MODE_GIVE_ITEM_EXIT:
    case POKELIST_MODE_GIVE_ITEM_RETURN: {
        u32 heldItem;
        u32 msg;

        wk->cursorPos = wk->param->index;
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->param->index);
        heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);
        PokeListMessage_CreateWordSet(wk, wk->message);
        if (heldItem == 0) {
            msg = BtlMain11_Text_GivenHold;
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, wk->param->item);
        } else {
            msg = BtlMain11_Text_TakenReplaced;
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, heldItem);
            PokeListMessage_SetItemName(wk, wk->message, 2, wk->param->item);
        }
        PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
        PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
        PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
        PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
        if (PokeListDemo_CanBecomeAltered(wk, wk->pkm) == TRUE) {
            PokeListDemo_SetAltered(wk, wk->pkm);
            PokeList_ShowMessage(wk, msg, TRUE, PokeList_MessageDoneDemo);
            wk->demo = POKELIST_DEMO_GIRATINA_ALTERED;
        } else if (wk->param->mode == POKELIST_MODE_GIVE_ITEM_EXIT) {
            PokeList_ShowMessage(wk, msg, TRUE, PokeList_MessageDoneExit);
        } else {
            PokeList_ShowMessage(wk, msg, TRUE, PokeList_MessageDoneSelect);
        }
        PokeListMessage_FreeWordSet(wk, wk->message);
        if (wk->param->mode == POKELIST_MODE_GIVE_ITEM_EXIT) {
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
        } else {
            wk->param->mode = POKELIST_MODE_FIELD;
        }
        wk->nextState = POKELIST_STATE_MESSAGE;
        wk->state = POKELIST_STATE_FADE_IN;
        wk->showCloseAndShortcut = TRUE;
        break;
    }
    }
}

static void PokeList_ShowModeMessage(PokeListWork *wk) {
    GFL_BGSysLoadScr(0);
    switch (wk->param->mode) {
    case POKELIST_MODE_FIELD:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_ChoosePokemon);
        wk->showCloseAndShortcut = TRUE;
        break;
    case POKELIST_MODE_BATTLE_TEAM:
        PokeListBattle_ShowMessage(wk);
        ClActor_SetVisible(wk->backButton, FALSE);
        wk->showCloseAndShortcut = FALSE;
        break;
    case POKELIST_MODE_BATTLE_TEAM_CANCEL:
    case POKELIST_MODE_BATTLE_INSTITUTE:
        wk->showCloseAndShortcut = FALSE;
        break;
    case POKELIST_MODE_USE_ITEM:
    case POKELIST_MODE_EVOLUTION_ITEM:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_UseWhichPokemon);
        wk->showCloseAndShortcut = FALSE;
        wk->allowEggs = FALSE;
        break;
    case POKELIST_MODE_GIVE_ITEM:
    case POKELIST_MODE_GIVE_MAIL:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_GiveWhichPokemon);
        wk->showCloseAndShortcut = FALSE;
        wk->allowEggs = FALSE;
        break;
    case POKELIST_MODE_TEACH_TM:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_TeachWhichPokemon);
        wk->showCloseAndShortcut = FALSE;
        wk->allowEggs = FALSE;
        break;
    case POKELIST_MODE_DAY_CARE:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_DepositWhichPokemon);
        wk->showCloseAndShortcut = FALSE;
        break;
    case POKELIST_MODE_SELECT:
    case POKELIST_MODE_MUSICAL:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_ChoosePokemon);
        wk->showCloseAndShortcut = FALSE;
        break;
    case POKELIST_MODE_SELECT_EGG:
    case POKELIST_MODE_SELECT_EGG_PROMPT:
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_PleaseSelectEgg);
        ClActor_SetVisible(wk->backButton, FALSE);
        wk->showCloseAndShortcut = FALSE;
        break;
    }
    PokeList_RefreshCursor(wk);
}

// A Pokémon was picked: what happens depends on the mode
static void PokeList_SelectPokemon(PokeListWork *wk) {
    if (wk->param->forceExit == TRUE) {
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        return;
    }
    switch (wk->param->mode) {
    case POKELIST_MODE_FIELD:
    case POKELIST_MODE_BATTLE_TEAM:
    case POKELIST_MODE_DAY_CARE:
    case POKELIST_MODE_SELECT_EGG:
    case POKELIST_MODE_BATTLE_TEAM_CANCEL:
    case POKELIST_MODE_BATTLE_INSTITUTE:
    case POKELIST_MODE_MUSICAL:
        PokeList_BeginMenu(wk);
        GFL_BGSysLoadScr(0);
        PokeList_OpenMenu(wk);
        break;
    case POKELIST_MODE_USE_ITEM:
        if (PokeList_IsItemForMove(wk, wk->param->item) == TRUE) {
            PokeList_BeginMenu(wk);
            PokeList_OpenMenu(wk);
        } else if (StatusRcv_CanUseItem(wk->pkm, wk->param->item, 0, wk->heapId) == TRUE) {
            u32 result = PokeList_ShowItemResult(wk, 0);
            BOOL used = StatusRcv_UseItem(wk->pkm, wk->param->item, 0, wk->param->zoneId, wk->heapId);

            if (result == POKELIST_ITEM_EFFECT_LEVEL_UP) {
                if (used) {
                    JoinAvenue_RecordActivity(9);
                }
                PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], wk->pkm, 0);
            } else {
                if (result == POKELIST_ITEM_EFFECT_LOWER_HP_EV || result == POKELIST_ITEM_EFFECT_RAISE_HP_EV) {
                    PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], wk->pkm, 0);
                } else {
                    PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                }
                GFL_SndSEPlay(SEQ_SE_RECOVERY);
            }
            PokeList_SubFromBag(wk, wk->param->item);
        } else if (wk->param->item == ITEM_GRACIDEA && PokeListDemo_CanBecomeSky(wk, wk->pkm) == TRUE) {
            PokeListDemo_SetSky(wk, wk->pkm);
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            wk->state = POKELIST_STATE_DEMO_START;
            wk->demo = POKELIST_DEMO_SHAYMIN_SKY;
        } else if (wk->param->item == ITEM_REVEAL_GLASS && PokeListDemo_CanBecomeTherian(wk, wk->pkm) == TRUE &&
                   isOneShotDRObtained(wk->param->trainerCard, 6, wk->param->playerInfo) == TRUE) {
            PokeListDemo_ToggleTherian(wk, wk->pkm);
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            wk->state = POKELIST_STATE_DEMO_START;
            wk->demo = POKELIST_DEMO_TOGGLE_THERIAN;
        } else if (wk->param->item == ITEM_DNA_SPLICERS_FUSE) {
            u32 result = PokeListDemo_CheckFuse(wk, wk->pkm);

            if (result == POKELIST_FUSION_CHECK_NOT_KYUREM || result == POKELIST_FUSION_CHECK_WRONG_FORM) {
                PokeList_ShowItemUseless(wk);
            } else if (result == POKELIST_FUSION_CHECK_FAINTED) {
                PokeList_ShowItemMessageSelect(wk, POKELIST_ITEM_MESSAGE_FAINTED);
            } else if (result == POKELIST_FUSION_CHECK_CAN_FUSE) {
                PokeList_ShowCursor(wk, wk->cursorPos);
                PokeList_ShowSubCursor(wk, wk->cursorPos);
                wk->selectPos = wk->cursorPos;
                wk->param->result = POKELIST_RESULT_ITEM_DONE;
                wk->state = POKELIST_STATE_SELECT_FUSION;
                wk->showCloseAndShortcut = FALSE;
                PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
                PokeListMessage_Print(wk, wk->message, BtlMain11_Text_FuseWhichPokemon);
                PokeList_RefreshCursor(wk);
            }
        } else if (wk->param->item == ITEM_DNA_SPLICERS_SEPARATE) {
            u32 result = PokeListDemo_CheckSeparate(wk, wk->pkm);

            if (result == POKELIST_FUSION_CHECK_PARTY_FULL) {
                PokeList_ShowItemMessageSelect(wk, POKELIST_ITEM_MESSAGE_NO_FUSION_SPACE);
            } else if (result == POKELIST_FUSION_CHECK_NOT_KYUREM || result == POKELIST_FUSION_CHECK_WRONG_FORM) {
                PokeList_ShowItemUseless(wk);
            } else {
                wk->selectPos = wk->cursorPos;
                wk->param->result = POKELIST_RESULT_ITEM_DONE;
                wk->state = POKELIST_STATE_DEMO_START;
                wk->demo = POKELIST_DEMO_KYUREM_SEPARATE;
                wk->demoForm = POKELIST_FUSION_PARTNER_SEPARATE;
            }
        } else {
            PokeList_ShowItemUseless(wk);
        }
        break;
    case POKELIST_MODE_EVOLUTION_ITEM:
        if (PokeList_CanEvolveWithItem(wk, wk->pkm, wk->param->item)) {
            PokeList_SubFromBag(wk, wk->param->item);
            wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
            wk->param->index = wk->cursorPos;
            wk->param->result = POKELIST_RESULT_EVOLVE_ITEM;
        } else {
            PokeList_ShowMessage(wk, BtlMain11_Text_WontHaveAnyEffect, TRUE, PokeList_MessageDoneExit);
        }
        break;
    case POKELIST_MODE_GIVE_ITEM: {
        u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        if (heldItem == 0) {
            if (PML_ItemIsMail(wk->param->item) == TRUE) {
                wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
                wk->param->index = wk->cursorPos;
                wk->param->result = POKELIST_RESULT_WRITE_MAIL;
            } else {
                wk->param->result = POKELIST_RESULT_ITEM_DONE;
                PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                PokeListMessage_CreateWordSet(wk, wk->message);
                PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
                PokeListMessage_SetItemName(wk, wk->message, 1, wk->param->item);
                PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
                PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
                if (PokeListDemo_CanBecomeOrigin(wk, wk->pkm) == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, BtlMain11_Text_GivenHold, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = POKELIST_DEMO_GIRATINA_ORIGIN;
                } else {
                    PokeList_ShowMessage(wk, BtlMain11_Text_GivenHold, TRUE, PokeList_MessageDoneExit);
                }
                PokeListMessage_FreeWordSet(wk, wk->message);
            }
        } else if (PML_ItemIsMail(heldItem) == TRUE) {
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            PokeList_ShowMessage(wk, BtlMain11_Text_MustRemoveMailBefore, TRUE, PokeList_MessageDoneExit);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemTextName(wk, wk->message, 1, heldItem);
            PokeList_ShowMessage(wk, BtlMain11_Text_AlreadyHoldingWouldLike, FALSE, PokeList_AskSwapItem);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    }
    case POKELIST_MODE_GIVE_MAIL:
        if (PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL) == 0) {
            MailData *mail = MailBox_CopyMail(wk->param->mailbox, 0, wk->param->mailboxSlot, wk->heapId);

            PokeParty_SetParam(wk->pkm, PKM_PARAM_MAIL, (u32)mail);
            GFL_HeapFree(mail);
            PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, wk->param->item);
            MailBox_ResetMail(wk->param->mailbox, 0, wk->param->mailboxSlot);
            PokeList_ShowMessage(wk, BtlMain11_Text_MailTransferredFromMailbox, TRUE, PokeList_MessageDoneExit);
        } else {
            PokeList_ShowMessage(wk, BtlMain11_Text_PokemonHoldingItemCannot, TRUE, PokeList_MessageDoneExit);
        }
        break;
    case POKELIST_MODE_TEACH_TM:
        switch (PokeList_CheckLearnMove(wk, wk->pkm, wk->param->index)) {
        case POKELIST_LEARN_EMPTY_SLOT:
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, BtlMain11_Text_Learned, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            PokeList_LearnMove(wk, wk->pkm);
            PokeList_RaiseFriendship(wk, wk->pkm);
            break;
        case POKELIST_LEARN_REPLACE_MOVE:
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, BtlMain11_Text_WantsLearnMoveHowever, FALSE, PokeList_AskReplaceMove);
            PokeListMessage_FreeWordSet(wk, wk->message);
            break;
        case POKELIST_LEARN_UNABLE:
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, BtlMain11_Text_NotCompatibleCantLearned, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            break;
        case POKELIST_LEARN_KNOWN:
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, BtlMain11_Text_AlreadyKnows, TRUE, PokeList_MessageDoneExit);
            PokeListMessage_FreeWordSet(wk, wk->message);
            break;
        }
        break;
    case POKELIST_MODE_SELECT:
    case POKELIST_MODE_SELECT_EGG_PROMPT:
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = wk->cursorPos;
        wk->param->result = POKELIST_RESULT_DONE;
        break;
    default:
        PokeList_BeginMenu(wk);
        PokeList_OpenMenu(wk);
        break;
    }
}

// Opens the menu of what to do with the Pokémon picked
static void PokeList_OpenMenu(PokeListWork *wk) {
    u32 items[8];

    GFL_BGSysLoadScr(0);
    switch (wk->param->mode) {
    case POKELIST_MODE_FIELD:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = POKELIST_MENU_SUMMARY;
            items[1] = POKELIST_MENU_SWITCH;
            items[2] = POKELIST_MENU_BACK;
            items[3] = POKELIST_MENU_END;
        } else {
            items[0] = POKELIST_MENU_SUMMARY;
            items[1] = POKELIST_MENU_FIELD_MOVES;
            items[2] = POKELIST_MENU_SWITCH;
            items[3] = POKELIST_MENU_ITEM;
            items[4] = POKELIST_MENU_BACK;
            items[5] = POKELIST_MENU_END;
            if (PML_ItemIsMail(PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL)) == TRUE) {
                items[3] = POKELIST_MENU_MAIL;
            }
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_What);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case POKELIST_MODE_BATTLE_TEAM:
    case POKELIST_MODE_BATTLE_TEAM_CANCEL:
    case POKELIST_MODE_BATTLE_INSTITUTE:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = POKELIST_MENU_SUMMARY;
            items[1] = POKELIST_MENU_BACK;
            items[2] = POKELIST_MENU_END;
        } else {
            items[0] = POKELIST_MENU_BATTLE_ENTRY;
            items[1] = POKELIST_MENU_SUMMARY;
            items[2] = POKELIST_MENU_BACK;
            items[3] = POKELIST_MENU_END;
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_What);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case POKELIST_MODE_USE_ITEM:
        items[0] = POKELIST_MENU_ITEM_MOVES;
        items[1] = POKELIST_MENU_BACK;
        items[2] = POKELIST_MENU_END;
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, PokeList_GetItemMenuMessage(wk, wk->param->item));
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case POKELIST_MODE_DAY_CARE:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = POKELIST_MENU_SUMMARY;
            items[1] = POKELIST_MENU_BACK;
            items[2] = POKELIST_MENU_END;
        } else {
            items[0] = POKELIST_MENU_SELECT;
            items[1] = POKELIST_MENU_SUMMARY;
            items[2] = POKELIST_MENU_BACK;
            items[3] = POKELIST_MENU_END;
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_What);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case POKELIST_MODE_SELECT_EGG:
        if (PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
            items[0] = POKELIST_MENU_CHOOSE;
            items[1] = POKELIST_MENU_SUMMARY;
            items[2] = POKELIST_MENU_BACK;
            items[3] = POKELIST_MENU_END;
            PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
            PokeListMessage_Print(wk, wk->message, BtlMain11_Text_WhatWillEgg);
        } else {
            items[0] = POKELIST_MENU_SUMMARY;
            items[1] = POKELIST_MENU_BACK;
            items[2] = POKELIST_MENU_END;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
            PokeListMessage_Print(wk, wk->message, BtlMain11_Text_What);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    case POKELIST_MODE_MUSICAL:
        if (MusicalSystem_CanJoin(wk->pkm) == TRUE) {
            items[0] = POKELIST_MENU_CHOOSE;
            items[1] = POKELIST_MENU_SUMMARY;
            items[2] = POKELIST_MENU_BACK;
            items[3] = POKELIST_MENU_END;
        } else {
            items[0] = POKELIST_MENU_SUMMARY;
            items[1] = POKELIST_MENU_BACK;
            items[2] = POKELIST_MENU_END;
        }
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_What);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    default:
        items[0] = POKELIST_MENU_SUMMARY;
        items[1] = POKELIST_MENU_BACK;
        items[2] = POKELIST_MENU_END;
        break;
    }
    PokeListMenu_Open(wk, wk->menu, items);
}

static void PokeList_UpdateSelect(PokeListWork *wk) {
    switch (wk->subState) {
    case POKELIST_SUBSTATE_INIT:
        wk->subState = POKELIST_SUBSTATE_READ_INPUT;
        wk->input = POKELIST_INPUT_NONE;
        break;
    case POKELIST_SUBSTATE_READ_INPUT:
        PokeList_ReadSelectionInput(wk);
        break;
    case POKELIST_SUBSTATE_HANDLE_INPUT:
        switch (wk->state) {
        case POKELIST_STATE_SELECT_SWAP:
            PokeList_HandleSwapSelection(wk);
            break;
        case POKELIST_STATE_SELECT_HP_SHARE:
            PokeList_HandleHpShareSelection(wk);
            break;
        case POKELIST_STATE_SELECT_ITEM_SWAP:
            PokeList_HandleItemSwapSelection(wk);
            break;
        case POKELIST_STATE_SELECT_FUSION:
            PokeList_HandleFusionSelection(wk);
            break;
        default:
            PokeList_HandleSelectionInput(wk);
            break;
        }
        break;
    }
}

static void PokeList_RefreshCursor(PokeListWork *wk) {
    wk->cursorPos = wk->param->index;
    wk->subState = POKELIST_SUBSTATE_INIT;
    wk->menuItem = POKELIST_MENU_NONE;
    if (PokeList_IsBattle(wk) == TRUE) {
        PokeListMenu_SetButtonActive(wk->buttons[0], FALSE);
        PokeListMenu_SetButtonActive(wk->buttons[1], FALSE);
    }
    if (wk->touch == FALSE) {
        if (wk->cursorPos <= 5) {
            PokeList_ShowCursor(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
        } else {
            PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
        }
    } else {
        ClActor_SetVisible(wk->cursor, FALSE);
        if (wk->cursorPos <= 5) {
            if (wk->state == POKELIST_STATE_SELECT_SWAP || wk->state == POKELIST_STATE_SELECT_ITEM_SWAP ||
                wk->state == POKELIST_STATE_SELECT_FUSION) {
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
            } else {
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            }
        } else {
            PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], FALSE);
        }
    }
    ClActor_SetVisible(wk->closeButton, wk->showCloseAndShortcut);
    ClActor_SetVisible(wk->shortcutMark, wk->showCloseAndShortcut);
    ClActor_SetSequence(wk->shortcutMark, wk->param->keyItemRegistered + TOUCHBAR_ANM_SHORTCUT_OFF);
    if (PokeList_IsBattle(wk) == TRUE || PokeList_HasBackButton(wk) == FALSE) {
        ClActor_SetVisible(wk->backButton, FALSE);
    } else {
        ClActor_SetVisible(wk->backButton, TRUE);
    }
}

static void PokeList_HandleSelectionInput(PokeListWork *wk) {
    u8 i;

    PokeListMessage_Close(wk, wk->message);
    if (PokeList_IsBattle(wk) == TRUE) {
        GFL_BGSysFillScrArea(0, 0, 0, 21, 32, 3, BGSYS_FILL_KEEP_PALETTE);
        for (i = 0; i < 6; i++) {
            wk->param->picked[i] = 0;
        }
        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            int entry = PokeListPlate_GetEntry(wk->plates[i]);

            if (entry <= 5) {
                wk->param->picked[entry] = i + 1;
            }
        }
    }
    switch (wk->input) {
    case POKELIST_INPUT_SELECT_POKEMON:
        ClActor_SetVisible(wk->cursor, FALSE);
        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        wk->param->index = wk->cursorPos;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        PokeList_SelectPokemon(wk);
        break;
    case POKELIST_INPUT_CONFIRM:
        if (PokeList_IsBattle(wk) == TRUE) {
            PokeList_ConfirmBattleTeam(wk);
        } else {
            wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
            wk->param->index = POKELIST_CURSOR_CONFIRM;
            wk->param->result = POKELIST_RESULT_DONE;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case POKELIST_INPUT_CANCEL:
        if (wk->cursorPos <= 5) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            ClActor_SetVisible(wk->cursor, FALSE);
        }
        if (PokeList_IsBattle(wk) == TRUE) {
            PokeListMenu_SetButtonActive(wk->buttons[0], FALSE);
            PokeListMenu_SetButtonPressed(wk->buttons[1], 1);
            wk->state = POKELIST_STATE_WAIT_TEAM_BUTTON;
            wk->cursorPos = POKELIST_CURSOR_CANCEL;
        } else {
            wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
            wk->pressedButton = wk->backButton;
            ClActor_SetSequence(wk->pressedButton, TOUCHBAR_ANM_BACK_PRESS);
        }
        wk->param->index = POKELIST_CURSOR_CANCEL;
        wk->param->result = POKELIST_RESULT_DONE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        break;
    case POKELIST_INPUT_CLOSE:
        if (wk->cursorPos <= 5) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            ClActor_SetVisible(wk->cursor, FALSE);
        }
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = POKELIST_CURSOR_CLOSE;
        wk->param->result = POKELIST_RESULT_DONE;
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->pressedButton = wk->closeButton;
        ClActor_SetSequence(wk->pressedButton, TOUCHBAR_ANM_CLOSE_PRESS);
        break;
    }
}

static void PokeList_HandleSwapSelection(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    if (wk->input == POKELIST_INPUT_SELECT_POKEMON && wk->cursorPos == wk->selectPos) {
        wk->input = POKELIST_INPUT_CANCEL;
    }
    switch (wk->input) {
    case POKELIST_INPUT_SELECT_POKEMON:
        PokeList_StartSwap(wk);
        break;
    case POKELIST_INPUT_CANCEL:
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowCursor(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
        ClActor_SetVisible(wk->subCursor, FALSE);
        wk->selectPos = POKELIST_CURSOR_NONE;
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        break;
    }
}

static int PokeList_CheckFusionPartner(PokeListWork *wk, u8 pos) {
    PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, pos);
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);

    if (PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL) != 0 || PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != 0) {
        return POKELIST_FUSION_PARTNER_EGG;
    }
    if (species == SPECIES_RESHIRAM) {
        if (hp == 0) {
            return POKELIST_FUSION_PARTNER_FAINTED;
        }
        return POKELIST_FUSION_PARTNER_RESHIRAM;
    }
    if (species == SPECIES_ZEKROM) {
        if (hp == 0) {
            return POKELIST_FUSION_PARTNER_FAINTED;
        }
        return POKELIST_FUSION_PARTNER_ZEKROM;
    }
    return POKELIST_FUSION_PARTNER_WRONG_SPECIES;
}

static void PokeList_HandleFusionSelection(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    if (wk->input != POKELIST_INPUT_SELECT_POKEMON || wk->cursorPos == wk->selectPos) {
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowCursor(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
        ClActor_SetVisible(wk->subCursor, FALSE);
        wk->selectPos = POKELIST_CURSOR_NONE;
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else {
        int form;

        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        form = PokeList_CheckFusionPartner(wk, wk->cursorPos);
        if (form == POKELIST_FUSION_PARTNER_RESHIRAM || form == POKELIST_FUSION_PARTNER_ZEKROM) {
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            wk->state = POKELIST_STATE_DEMO_START;
            wk->demo = POKELIST_DEMO_KYUREM_FUSE;
            wk->demoForm = form;
        } else {
            PokeList_ShowMessage(wk, form + BtlMain11_Text_WontHaveAnyEffect, TRUE, PokeList_MessageDoneSelect);
            PokeList_ShowCursor(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            ClActor_SetVisible(wk->subCursor, FALSE);
            wk->param->index = wk->cursorPos;
        }
    }
}

// Shares a fifth of the user's HP with the Pokémon picked, as Soft-Boiled and Milk Drink do outside battle
static void PokeList_HandleHpShareSelection(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    if (wk->input == POKELIST_INPUT_SELECT_POKEMON) {
        u32 hp;
        u32 maxHp;

        wk->pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        hp = PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL);
        maxHp = PokeParty_GetParam(wk->pkm, PKM_PARAM_MAX_HP, NULL);
        if (wk->cursorPos == wk->selectPos2 || hp == 0 || maxHp == hp) {
            PokeList_ShowMessage(wk, BtlMain11_Text_CantUsedPokemon, TRUE, PokeList_MessageDoneSelect);
            PokeList_ShowCursor(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos2], FALSE);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            ClActor_SetVisible(wk->subCursor, FALSE);
            wk->param->index = wk->cursorPos;
        } else {
            PartyPkm *user;
            u32 userMaxHp;
            u32 userHp;
            u32 amount;

            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            wk->pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
            wk->state = POKELIST_STATE_HP_CHANGE_START;
            wk->prevHp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);
            wk->hpDoneFunc = PokeList_ShareHpDone;
            PokeList_ShowCursor(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos2], FALSE);
            ClActor_SetVisible(wk->subCursor, FALSE);
            user = PokeParty_GetPkm(wk->param->party, wk->selectPos2);
            userMaxHp = PokeParty_GetParam(user, PKM_PARAM_MAX_HP, NULL);
            userHp = PokeParty_GetParam(user, PKM_PARAM_HP, NULL);
            amount = userMaxHp / 5;
            if (hp + amount > maxHp) {
                PokeParty_SetParam(user, PKM_PARAM_HP, userHp - (maxHp - hp));
            } else {
                PokeParty_SetParam(user, PKM_PARAM_HP, userHp - amount);
            }
            wk->param->index = wk->cursorPos;
            wk->cursorPos = wk->selectPos2;
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            GFL_SndSEPlay(SEQ_SE_RECOVERY);
        }
    } else {
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowCursor(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos2], FALSE);
        ClActor_SetVisible(wk->subCursor, FALSE);
        wk->selectPos2 = POKELIST_CURSOR_NONE;
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
}

// Decides the team in a battle's selection, if the regulation allows it
static void PokeList_ConfirmBattleTeam(PokeListWork *wk) {
    Regulation *regulation = wk->param->regulation;

    if (wk->cursorPos != POKELIST_CURSOR_CONFIRM) {
        if (wk->cursorPos <= 5) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            ClActor_SetVisible(wk->cursor, FALSE);
        }
        wk->cursorPos = POKELIST_CURSOR_CONFIRM;
    }
    wk->param->index = POKELIST_CURSOR_CONFIRM;
    {
        u8 picked[6] = { 0 };
        u8 i;
        u32 result;

        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            int entry = PokeListPlate_GetEntry(wk->plates[i]);

            if (entry <= 5) {
                picked[entry] = i + 1;
            }
        }
        result = PokeRegulation_CheckPicked(regulation, wk->param->party, picked);
        if (regulation->minCount > wk->enteredCount) {
            PokeList_ShowMessage(wk, regulation->minCount + BtlMain11_Text_EggCannotHoldItem, TRUE,
                                 PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (regulation->maxCount < wk->enteredCount) {
            PokeList_ShowMessage(wk, regulation->maxCount + BtlMain11_Text_SixPokemonNeeded, TRUE,
                                 PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (result == REGULATION_RESULT_LEVEL_TOTAL) {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetNumber(wk, wk->message, 0, regulation->maxLevelTotal, 3);
            PokeList_ShowMessage(wk, BtlMain11_Text_TotalAllPartyMembers, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else if (result == REGULATION_RESULT_REQUIRED) {
            PokeList_ShowMessage(wk, BtlMain11_Text_PokemonWhichMustParticipate, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            PokeListMenu_SetButtonActive(wk->buttons[1], FALSE);
            PokeListMenu_SetButtonPressed(wk->buttons[0], 1);
            wk->state = POKELIST_STATE_WAIT_TEAM_BUTTON;
            wk->param->index = POKELIST_CURSOR_CONFIRM;
            wk->param->result = POKELIST_RESULT_DONE;
            wk->decided = TRUE;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
}

// Moves an item from one Pokémon to another, with its mail
static void PokeList_MoveHeldItem(PokeListWork *wk, PartyPkm *from, PartyPkm *to, u16 item) {
    if (PML_ItemIsMail(item) == FALSE) {
        PokeList_SetHeldItem(wk, from, 0);
        PokeList_SetHeldItem(wk, to, item);
    } else {
        MailData *mail = CreateMailData(wk->heapId);
        MailData *blank = CreateMailData(wk->heapId);

        PokeParty_GetParam(from, PKM_PARAM_MAIL, mail);
        PokeParty_SetParam(to, PKM_PARAM_MAIL, (u32)mail);
        PokeParty_SetParam(to, PKM_PARAM_ITEM, item);
        PokeParty_SetParam(from, PKM_PARAM_MAIL, (u32)blank);
        PokeParty_SetParam(from, PKM_PARAM_ITEM, 0);
        GFL_HeapFree(blank);
        GFL_HeapFree(mail);
    }
}

// Swaps the items of two Pokémon, with their mail
static void PokeList_SwapHeldItems(PokeListWork *wk, PartyPkm *pkmA, PartyPkm *pkmB, u16 itemA, u16 itemB) {
    MailData *mailA = CreateMailData(wk->heapId);
    MailData *mailB = CreateMailData(wk->heapId);
    MailData *blank = CreateMailData(wk->heapId);

    if (PML_ItemIsMail(itemA) == FALSE && PML_ItemIsMail(itemB) == FALSE) {
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
    } else if (PML_ItemIsMail(itemA) == TRUE && PML_ItemIsMail(itemB) == FALSE) {
        PokeParty_GetParam(pkmA, PKM_PARAM_MAIL, mailA);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
        PokeParty_SetParam(pkmB, PKM_PARAM_MAIL, (u32)mailA);
        PokeParty_SetParam(pkmA, PKM_PARAM_MAIL, (u32)blank);
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
    } else if (PML_ItemIsMail(itemA) == FALSE && PML_ItemIsMail(itemB) == TRUE) {
        PokeParty_GetParam(pkmB, PKM_PARAM_MAIL, mailB);
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
        PokeParty_SetParam(pkmA, PKM_PARAM_MAIL, (u32)mailB);
        PokeParty_SetParam(pkmB, PKM_PARAM_MAIL, (u32)blank);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
    } else if (PML_ItemIsMail(itemA) == TRUE && PML_ItemIsMail(itemB) == TRUE) {
        PokeParty_GetParam(pkmA, PKM_PARAM_MAIL, mailA);
        PokeParty_GetParam(pkmB, PKM_PARAM_MAIL, mailB);
        PokeParty_SetParam(pkmA, PKM_PARAM_ITEM, itemB);
        PokeParty_SetParam(pkmA, PKM_PARAM_MAIL, (u32)mailB);
        PokeParty_SetParam(pkmB, PKM_PARAM_ITEM, itemA);
        PokeParty_SetParam(pkmB, PKM_PARAM_MAIL, (u32)mailA);
    }
    GFL_HeapFree(blank);
    GFL_HeapFree(mailB);
    GFL_HeapFree(mailA);
}

// Updates the forms that two Pokémon's new items give them
static void PokeList_UpdateSwappedItemForms(PartyPkm *pkmA, PartyPkm *pkmB) {
    u16 items[2];
    PartyPkm *pkms[2];
    u16 i;

    pkms[0] = pkmA;
    pkms[1] = pkmB;
    items[0] = PokeParty_GetParam(pkmA, PKM_PARAM_ITEM, NULL);
    items[1] = PokeParty_GetParam(pkmB, PKM_PARAM_ITEM, NULL);
    for (i = 0; i < 2; i++) {
        // BUG: The first Pokémon's stats are calculated again each time, and the second's never are
#ifdef BUGFIX
        PokeParty_RecalcStats(pkms[i]);
#else
        PokeParty_RecalcStats(pkmA);
#endif
        PokeList_UpdateArceusForm(NULL, pkms[i], items[i]);
        PokeList_UpdateGenesectForm(NULL, pkms[i], items[i]);
    }
}

static void PokeList_ShowItemSwapError(PokeListWork *wk, u32 msgId) {
    GFL_SndSEPlay(SEQ_SE_BEEP);
    PokeList_ShowCursor(wk, wk->cursorPos);
    PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
    PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
    ClActor_SetVisible(wk->subCursor, FALSE);
    PokeList_ShowMessage(wk, msgId, TRUE, PokeList_MessageDoneSelect);
}

// Gives the item of the Pokémon picked first to the one picked now, swapping their items
static void PokeList_HandleItemSwapSelection(PokeListWork *wk) {
    if (wk->input == POKELIST_INPUT_SELECT_POKEMON) {
        PartyPkm *src = PokeParty_GetPkm(wk->param->party, wk->selectPos);
        PartyPkm *dst = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        u16 srcSpecies = PokeParty_GetParam(src, PKM_PARAM_LEGAL_SPECIES, NULL);
        u16 dstSpecies = PokeParty_GetParam(dst, PKM_PARAM_LEGAL_SPECIES, NULL);
        u16 srcItem = PokeParty_GetParam(src, PKM_PARAM_ITEM, NULL);
        u16 dstItem = PokeParty_GetParam(dst, PKM_PARAM_ITEM, NULL);

        PokeListMessage_Close(wk, wk->message);
        if ((dstSpecies == SPECIES_GIRATINA && srcItem == ITEM_GRISEOUS_ORB) ||
            (srcSpecies == SPECIES_GIRATINA && dstItem == ITEM_GRISEOUS_ORB)) {
            PokeList_ShowItemSwapError(wk, BtlMain11_Text_GriseousOrbCannotMoved);
        } else if (dstSpecies == SPECIES_GIRATINA && dstItem == ITEM_GRISEOUS_ORB) {
            PokeList_ShowItemSwapError(wk, BtlMain11_Text_GriseousOrbCannotMoved_2);
        } else if (dstSpecies == SPECIES_EGG) {
            PokeList_ShowItemSwapError(wk, BtlMain11_Text_EggCannotHoldItem);
        } else {
            if (dstItem == 0) {
                PokeList_MoveHeldItem(wk, src, dst, srcItem);
                PokeListMessage_SetPkmName(wk, wk->message, 0, dst);
            } else {
                PokeList_SwapHeldItems(wk, src, dst, srcItem, dstItem);
                PokeListMessage_SetPkmName(wk, wk->message, 0, src);
                PokeListMessage_SetPkmName(wk, wk->message, 2, dst);
                PokeListMessage_SetItemName(wk, wk->message, 3, dstItem);
            }
            PokeList_UpdateSwappedItemForms(src, dst);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeListPlate_Redraw(wk, wk->plates[wk->selectPos]);
            PokeList_ShowCursor(wk, wk->cursorPos);
            PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            ClActor_SetVisible(wk->subCursor, FALSE);
            if (dstItem == 0) {
                PokeList_ShowMessage(wk, BtlMain11_Text_GivenHold, TRUE, PokeList_MessageDoneSelect);
            } else {
                PokeList_ShowMessage(wk, BtlMain11_Text_SSWereSwapped, TRUE, PokeList_MessageDoneSelect);
            }
        }
        PokeListMessage_FreeWordSet(wk, wk->message);
    } else {
        PokeListMessage_Close(wk, wk->message);
        PokeListMessage_FreeWordSet(wk, wk->message);
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowCursor(wk, wk->cursorPos);
        PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
        PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
        ClActor_SetVisible(wk->subCursor, FALSE);
        // BUG: The selection of the Pokémon giving its item is kept, and that of Soft-Boiled's user is cleared instead
#ifdef BUGFIX
        wk->selectPos = POKELIST_CURSOR_NONE;
#else
        wk->selectPos2 = POKELIST_CURSOR_NONE;
#endif
        wk->param->index = wk->cursorPos;
        PokeList_ShowModeMessage(wk);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
}

static void PokeList_ReadSelectionInput(PokeListWork *wk) {
    PokeList_ReadCursorKeys(wk);
    if (wk->input == POKELIST_INPUT_NONE) {
        PokeList_HandleTouch(wk);
    }
    if (wk->input != POKELIST_INPUT_NONE) {
        wk->subState = POKELIST_SUBSTATE_HANDLE_INPUT;
    }
    if (wk->param->forceExit == TRUE) {
        wk->subState = POKELIST_SUBSTATE_HANDLE_INPUT;
        wk->input = POKELIST_INPUT_CANCEL;
    }
}

// A was pressed
static void PokeList_HandleConfirmKey(PokeListWork *wk) {
    if (wk->cursorPos == POKELIST_CURSOR_CONFIRM) {
        wk->input = POKELIST_INPUT_CONFIRM;
    } else if (wk->cursorPos == POKELIST_CURSOR_CANCEL) {
        wk->input = POKELIST_INPUT_CANCEL;
    } else if (wk->allowEggs == FALSE && PokeListPlate_IsEgg(wk, wk->plates[wk->cursorPos]) == TRUE) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
    } else if (wk->state == POKELIST_STATE_SELECT_ITEM_SWAP && wk->cursorPos == wk->selectPos) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
    } else {
        wk->input = POKELIST_INPUT_SELECT_POKEMON;
    }
}

// Moves the cursor with the keys, and takes A, B, X and Y
static void PokeList_ReadCursorKeys(PokeListWork *wk) {
    static const u8 sUpFromButtons[] = { 0, 1, 1, 3, 3, 5 };
    static const u8 sUpFromButtonsTwo[] = { 0, 0, 2, 2, 4, 4 };
    u32 trg = GCTX_HIDGetPressedKeys();
    u32 rep = GCTX_HIDGetTypedKeys();

    if (wk->touch == TRUE && ClActor_IsVisible(wk->cursor) == FALSE) {
        if (trg != 0 && !(trg & (PAD_BUTTON_B | PAD_BUTTON_X))) {
            if (wk->cursorPos <= 5) {
                PokeList_ShowCursor(wk, wk->cursorPos);
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
            } else {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                ClActor_SetVisible(wk->cursor, FALSE);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->touch = FALSE;
        }
    } else {
        s32 dir = 0;

        if (trg != 0) {
            wk->touch = FALSE;
        }
        if (rep & PAD_KEY_UP) {
            dir = -2;
        } else if (rep & PAD_KEY_DOWN) {
            dir = 2;
        } else if (rep & PAD_KEY_LEFT) {
            dir = -1;
        } else if (rep & PAD_KEY_RIGHT) {
            dir = 1;
        } else if (trg & PAD_BUTTON_A) {
            PokeList_HandleConfirmKey(wk);
        }
        if (dir != 0) {
            s32 prevPos = wk->cursorPos;
            BOOL done = FALSE;
            s32 last;

            if (PokeList_IsBattle(wk) == TRUE) {
                if (PokeList_HasBattleCancelButton(wk) == TRUE) {
                    last = POKELIST_CURSOR_CANCEL;
                } else {
                    last = POKELIST_CURSOR_CONFIRM;
                }
            } else {
                last = 5;
            }
            if ((wk->cursorPos == POKELIST_CURSOR_CONFIRM || wk->cursorPos == POKELIST_CURSOR_CANCEL) &&
                (rep & (PAD_KEY_UP | PAD_KEY_DOWN))) {
                u8 count = PokeParty_GetPkmCount(wk->param->party);

                if (rep & PAD_KEY_UP) {
                    if (wk->cursorPos == POKELIST_CURSOR_CANCEL || PokeList_HasBattleCancelButton(wk) == FALSE) {
                        wk->cursorPos = sUpFromButtons[count - 1];
                    } else {
                        wk->cursorPos = sUpFromButtonsTwo[count - 1];
                    }
                } else if (wk->cursorPos == POKELIST_CURSOR_CANCEL || PokeList_HasBattleCancelButton(wk) == FALSE) {
                    if (count > 1) {
                        wk->cursorPos = 1;
                    } else {
                        wk->cursorPos = 0;
                    }
                } else {
                    wk->cursorPos = 0;
                }
            } else {
                do {
                    s32 next = wk->cursorPos + dir;

                    if (next > last) {
                        if (last == POKELIST_CURSOR_CONFIRM && dir > 1 && prevPos != POKELIST_CURSOR_CONFIRM) {
                            wk->cursorPos = last;
                        } else {
                            wk->cursorPos = next - (last + 1);
                        }
                    } else if (next < 0) {
                        if (last == POKELIST_CURSOR_CONFIRM && dir < 1) {
                            wk->cursorPos = last;
                        } else {
                            wk->cursorPos = wk->cursorPos + (last + 1) + dir;
                        }
                    } else {
                        wk->cursorPos += dir;
                    }
                    if (wk->cursorPos > 5 || PokeListPlate_IsValid(wk, wk->plates[wk->cursorPos]) == TRUE) {
                        done = TRUE;
                    }
                } while (done == FALSE);
            }
            if (prevPos != wk->cursorPos) {
                if (wk->cursorPos <= 5) {
                    PokeList_ShowCursor(wk, wk->cursorPos);
                    PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
                } else {
                    PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                    ClActor_SetVisible(wk->cursor, FALSE);
                }
                if (prevPos <= 5) {
                    PokeListPlate_SetSelected(wk, wk->plates[prevPos], FALSE);
                } else {
                    PokeListMenu_SetButtonActive(wk->buttons[(u8)(prevPos - 6)], FALSE);
                }
                wk->flashTimer = 12;
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    }
    if (trg & PAD_BUTTON_B) {
        if ((PokeList_IsBattle(wk) == FALSE || PokeList_HasBattleCancelButton(wk) == TRUE) &&
            PokeList_HasBackButton(wk) == TRUE) {
            wk->input = POKELIST_INPUT_CANCEL;
            wk->pressedButton = wk->backButton;
            ClActor_SetSequence(wk->pressedButton, TOUCHBAR_ANM_BACK_PRESS);
            wk->touch = FALSE;
        }
    } else if (trg & PAD_BUTTON_X) {
        if (wk->showCloseAndShortcut == TRUE) {
            wk->input = POKELIST_INPUT_CLOSE;
            wk->pressedButton = wk->closeButton;
            ClActor_SetSequence(wk->pressedButton, TOUCHBAR_ANM_CLOSE_PRESS);
            wk->touch = FALSE;
        }
    } else if (trg & PAD_BUTTON_Y) {
        if (wk->showCloseAndShortcut == TRUE) {
            wk->param->keyItemRegistered ^= 1;
            ClActor_SetSequence(wk->shortcutMark, wk->param->keyItemRegistered + TOUCHBAR_ANM_SHORTCUT_OFF);
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            wk->touch = FALSE;
        }
    }
}

// Takes a touch of a plate or a button
static void PokeList_HandleTouch(PokeListWork *wk) {
    u8 slots[POKELIST_PLATE_COUNT];
    TouchRect plateRects[POKELIST_PLATE_COUNT + 1];
    u8 count = 0;
    u8 i;
    s32 hit;

    for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
        if (PokeListPlate_IsValid(wk, wk->plates[i]) == TRUE) {
            slots[count] = i;
            PokeListPlate_GetTouchRect(wk, wk->plates[i], &plateRects[count]);
            count++;
        }
    }
    plateRects[count].top = TOUCH_RECT_END;
    hit = GFL_HIDGetTouchPressedRect(plateRects);
    if (hit != TOUCH_RECT_NONE) {
        if (wk->allowEggs == FALSE && hit <= 5 && PokeListPlate_IsEgg(wk, wk->plates[hit]) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (wk->state == POKELIST_STATE_SELECT_ITEM_SWAP && hit <= 5 && hit == wk->selectPos) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            s32 prevPos;

            wk->input = POKELIST_INPUT_SELECT_POKEMON;
            prevPos = wk->cursorPos;
            wk->cursorPos = slots[hit];
            if (prevPos <= 5) {
                PokeListPlate_SetSelected(wk, wk->plates[prevPos], FALSE);
            } else {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(prevPos - 6)], FALSE);
            }
            if (wk->cursorPos <= 5) {
                PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], TRUE);
            } else {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                ClActor_SetVisible(wk->cursor, FALSE);
            }
            wk->flashTimer = 12;
        }
        wk->touch = TRUE;
    }
    if (hit == TOUCH_RECT_NONE) {
        TouchRect buttonRects[4] = {
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { TOUCH_RECT_END, 0, 0, 0 },
        };

        if (PokeList_IsBattle(wk) == TRUE) {
            if (PokeList_HasBattleCancelButton(wk) == TRUE) {
                buttonRects[1].top = 168;
                buttonRects[1].bottom = 192;
                buttonRects[1].left = 176;
                buttonRects[1].right = 255;
                buttonRects[0].top = 168;
                buttonRects[0].bottom = 192;
                buttonRects[0].left = 96;
                buttonRects[0].right = 176;
            } else {
                buttonRects[0].top = 168;
                buttonRects[0].bottom = 192;
                buttonRects[0].left = 176;
                buttonRects[0].right = 255;
            }
        } else {
            if (PokeList_HasBackButton(wk) == TRUE) {
                buttonRects[1].top = 168;
                buttonRects[1].bottom = 192;
                buttonRects[1].left = 224;
                buttonRects[1].right = 248;
            }
            if (wk->showCloseAndShortcut == TRUE) {
                buttonRects[2].top = 168;
                buttonRects[2].bottom = 192;
                buttonRects[2].left = 208;
                buttonRects[2].right = 232;
            }
        }
        hit = GFL_HIDGetTouchPressedRect(buttonRects);
        if (hit != TOUCH_RECT_NONE) {
            wk->input = hit;
            wk->touch = TRUE;
            if (hit == POKELIST_INPUT_CANCEL) {
                ClActor_SetSequence(wk->backButton, TOUCHBAR_ANM_BACK_PRESS);
            }
        }
    }
    if (hit == TOUCH_RECT_NONE && wk->showCloseAndShortcut == TRUE) {
        TouchRect markRects[2] = {
            { 0, 0, 0, 0 },
            { TOUCH_RECT_END, 0, 0, 0 },
        };

        markRects[0].top = 168;
        markRects[0].bottom = 192;
        markRects[0].left = 184;
        markRects[0].right = 208;
        if (GFL_HIDGetTouchPressedRect(markRects) != TOUCH_RECT_NONE) {
            wk->param->keyItemRegistered ^= 1;
            ClActor_SetSequence(wk->shortcutMark, wk->param->keyItemRegistered + TOUCHBAR_ANM_SHORTCUT_OFF);
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            wk->touch = TRUE;
        }
    }
}

// Shows the cursor on a plate
static void PokeList_ShowCursor(PokeListWork *wk, s32 pos) {
    ClActorPos actorPos;

    PokeListPlate_GetCursorPos(wk, wk->plates[pos], &actorPos);
    ClActor_SetSurfacePos(wk->cursor, &actorPos, 0);
    ClActor_SetVisible(wk->cursor, TRUE);
    ClActor_SetBgPriority(wk->cursor, 3);
    if (pos == 0) {
        ClActor_SetSequence(wk->cursor, CURSOR_ANM_FIRST);
    } else {
        ClActor_SetSequence(wk->cursor, CURSOR_ANM_OTHER);
    }
}

// Shows the second cursor on a plate, which marks the Pokémon picked first
static void PokeList_ShowSubCursor(PokeListWork *wk, s32 pos) {
    ClActorPos actorPos;

    PokeListPlate_GetCursorPos(wk, wk->plates[pos], &actorPos);
    ClActor_SetSurfacePos(wk->subCursor, &actorPos, 0);
    ClActor_SetVisible(wk->subCursor, TRUE);
    ClActor_SetBgPriority(wk->subCursor, 2);
    if (pos == 0) {
        ClActor_SetSequence(wk->subCursor, CURSOR_ANM_SUB_FIRST);
    } else {
        ClActor_SetSequence(wk->subCursor, CURSOR_ANM_SUB_OTHER);
    }
}

static void PokeList_UpdateMenu(PokeListWork *wk) {
    switch (wk->subState) {
    case POKELIST_SUBSTATE_INIT:
        PokeList_BeginMenu(wk);
        wk->subState = POKELIST_SUBSTATE_READ_INPUT;
        break;
    case POKELIST_SUBSTATE_READ_INPUT: {
        u32 item;

        PokeListMenu_Update(wk, wk->menu);
        item = PokeListMenu_GetPicked(wk, wk->menu);
        if (item != POKELIST_MENU_NONE) {
            wk->menuItem = item;
            wk->subState = POKELIST_SUBSTATE_HANDLE_INPUT;
        }
        if (wk->param->forceExit == TRUE) {
            wk->subState = POKELIST_SUBSTATE_HANDLE_INPUT;
        }
        break;
    }
    case POKELIST_SUBSTATE_HANDLE_INPUT:
        PokeList_EndMenu(wk);
        PokeList_DoMenuItem(wk);
        break;
    }
}

// Dims the screen but the menu, for the menu to open
static void PokeList_BeginMenu(PokeListWork *wk) {
    wk->state = POKELIST_STATE_MENU;
    wk->subState = POKELIST_SUBSTATE_INIT;
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                            8);
    G2_SetWnd0Position(224, 168, 248, 192);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          FALSE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          TRUE);
    wk->glowPaused = TRUE;
    ClActor_SetVisible(wk->closeButton, FALSE);
    ClActor_SetVisible(wk->backButton, FALSE);
    ClActor_SetVisible(wk->shortcutMark, FALSE);
    GFL_BGSysQueueScrLoad(0);
    GCTX_HIDSetUsingTouch(wk->touch);
}

static void PokeList_EndMenu(PokeListWork *wk) {
    PokeListMenu_Close(wk, wk->menu);
    PokeListMessage_Close(wk, wk->message);
    if (PokeList_IsBattle(wk) == TRUE || PokeList_HasBackButton(wk) == FALSE) {
        ClActor_SetVisible(wk->backButton, FALSE);
    } else {
        ClActor_SetVisible(wk->backButton, TRUE);
    }
    if (wk->menuItem != POKELIST_MENU_ITEM && wk->menuItem != POKELIST_MENU_MAIL) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, GX_PLANEMASK_BG3, 16, 10);
        wk->glowPaused = FALSE;
    }
    GFL_BGSysFillScrAsync(0, 0);
    wk->touch = GCTX_HIDIsUsingTouch();
}

// Does what the menu item picked does
static void PokeList_DoMenuItem(PokeListWork *wk) {
    if (wk->param->forceExit == TRUE) {
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        return;
    }
    switch (wk->menuItem) {
    case POKELIST_MENU_BACK:
        PokeList_ShowCursor(wk, wk->cursorPos);
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowModeMessage(wk);
        break;
    case POKELIST_MENU_SWITCH:
        PokeList_ShowCursor(wk, wk->cursorPos);
        PokeList_ShowSubCursor(wk, wk->cursorPos);
        wk->selectPos = wk->cursorPos;
        wk->state = POKELIST_STATE_SELECT_SWAP;
        wk->pkm = NULL;
        wk->showCloseAndShortcut = FALSE;
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_MoveWhere);
        PokeList_RefreshCursor(wk);
        break;
    case POKELIST_MENU_ITEM: {
        PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, wk->cursorPos);
        u16 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
        u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        u32 itemMenus[2][5] = {
            { POKELIST_MENU_GIVE, POKELIST_MENU_TAKE, POKELIST_MENU_MOVE_ITEM, POKELIST_MENU_BACK, POKELIST_MENU_END },
            { POKELIST_MENU_GIVE, POKELIST_MENU_TAKE, POKELIST_MENU_BACK, POKELIST_MENU_END },
        };

        PokeList_BeginMenu(wk);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_WhatItem);
        if (item == ITEM_GRISEOUS_ORB && species == SPECIES_GIRATINA) {
            PokeListMenu_Open(wk, wk->menu, itemMenus[1]);
        } else {
            PokeListMenu_Open(wk, wk->menu, itemMenus[0]);
        }
        GFL_BGSysLoadScr(0);
        break;
    }
    case POKELIST_MENU_MAIL: {
        u32 mailMenu[5] = { POKELIST_MENU_READ_MAIL, POKELIST_MENU_TAKE_MAIL, POKELIST_MENU_MOVE_MAIL,
                            POKELIST_MENU_BACK, POKELIST_MENU_END };

        PokeList_BeginMenu(wk);
        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_MENU);
        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_WhatItem);
        PokeListMenu_Open(wk, wk->menu, mailMenu);
        GFL_BGSysLoadScr(0);
        break;
    }
    case POKELIST_MENU_SUMMARY:
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = wk->cursorPos;
        wk->param->result = POKELIST_RESULT_SUMMARY;
        break;
    case POKELIST_MENU_ENTER: {
        Regulation *regulation = wk->param->regulation;
        BOOL full = FALSE;
        u32 result = PokeListPlate_CheckEntry(wk, wk->plates[wk->cursorPos]);

        if (result == POKELIST_ENTRY_CHECK_ALLOWED) {
            PokeListPlate_SetEntry(wk, wk->plates[wk->cursorPos], wk->enteredCount);
            wk->enteredCount++;
            PokeList_ShowModeMessage(wk);
            if (wk->enteredCount == regulation->maxCount) {
                full = TRUE;
            }
            wk->state = POKELIST_STATE_SELECT;
        } else if (result == POKELIST_ENTRY_CHECK_SAME_SPECIES) {
            PokeList_ShowMessage(wk, BtlMain11_Text_IdenticalPokemonNotPermitted, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (result == POKELIST_ENTRY_CHECK_SAME_ITEM) {
            PokeList_ShowMessage(wk, BtlMain11_Text_SomePokemonHoldingIdentical, TRUE, PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else if (result == POKELIST_ENTRY_CHECK_FULL) {
            PokeList_ShowMessage(wk, regulation->maxCount + BtlMain11_Text_SixPokemonNeeded, TRUE,
                                 PokeList_MessageDoneSelect);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        wk->pkm = NULL;
        if (full == TRUE) {
            PokeListPlate_SetSelected(wk, wk->plates[wk->cursorPos], FALSE);
            wk->cursorPos = POKELIST_CURSOR_CONFIRM;
            if (wk->touch == FALSE) {
                PokeListMenu_SetButtonActive(wk->buttons[(u8)(wk->cursorPos - 6)], TRUE);
                ClActor_SetVisible(wk->cursor, FALSE);
            }
        } else if (wk->touch == FALSE) {
            PokeList_ShowCursor(wk, wk->cursorPos);
        }
        break;
    }
    case POKELIST_MENU_WITHDRAW: {
        int entry = PokeListPlate_GetEntry(wk->plates[wk->cursorPos]);
        u8 i;

        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            int other = PokeListPlate_GetEntry(wk->plates[i]);

            if (other <= 5 && other > entry) {
                PokeListPlate_SetEntry(wk, wk->plates[i], other - 1);
            }
        }
        PokeListPlate_SetEntry(wk, wk->plates[wk->cursorPos], POKELIST_ENTRY_ABLE);
        wk->enteredCount--;
        if (wk->touch == FALSE) {
            PokeList_ShowCursor(wk, wk->cursorPos);
        }
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowModeMessage(wk);
        break;
    }
    case POKELIST_MENU_TAKE: {
        u32 item = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        PokeList_UpdateArceusForm(wk, wk->pkm, 0);
        PokeList_UpdateGenesectForm(wk, wk->pkm, 0);
        if (item == 0) {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeList_ShowMessage(wk, BtlMain11_Text_IsntHoldingAnything, TRUE, PokeList_MessageDoneSelect);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else if (PokeList_GetBagCount(wk, item) == 999) {
            PokeList_ShowMessage(wk, BtlMain11_Text_BagFull, TRUE, PokeList_MessageDoneSelect);
        } else {
            PokeList_SetHeldItem(wk, wk->pkm, 0);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, item);
            if (PokeListDemo_CanBecomeAltered(wk, wk->pkm) == TRUE) {
                PokeListDemo_SetAltered(wk, wk->pkm);
                PokeList_ShowMessage(wk, BtlMain11_Text_ReceivedFrom, TRUE, PokeList_MessageDoneDemo);
                wk->demo = POKELIST_DEMO_GIRATINA_ALTERED;
            } else {
                PokeList_ShowMessage(wk, BtlMain11_Text_ReceivedFrom, TRUE, PokeList_MessageDoneSelect);
            }
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    }
    case POKELIST_MENU_MOVE_ITEM:
    case POKELIST_MENU_MOVE_MAIL: {
        u32 item = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        PokeList_UpdateArceusForm(wk, wk->pkm, 0);
        PokeList_UpdateGenesectForm(wk, wk->pkm, 0);
        PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
        if (item == 0) {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeList_ShowMessage(wk, BtlMain11_Text_IsntHoldingAnything, TRUE, PokeList_MessageDoneSelect);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetItemName(wk, wk->message, 1, item);
            PokeList_ShowCursor(wk, wk->cursorPos);
            PokeList_ShowSubCursor(wk, wk->cursorPos);
            wk->selectPos = wk->cursorPos;
            wk->state = POKELIST_STATE_SELECT_ITEM_SWAP;
            wk->pkm = NULL;
            wk->showCloseAndShortcut = FALSE;
            PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
            PokeListMessage_Print(wk, wk->message, BtlMain11_Text_MoveWhere);
            PokeList_RefreshCursor(wk);
        }
        break;
    }
    case POKELIST_MENU_CHOOSE:
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = wk->cursorPos;
        wk->param->result = POKELIST_RESULT_DONE;
        break;
    case POKELIST_MENU_GIVE:
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = wk->cursorPos;
        wk->param->result = POKELIST_RESULT_GIVE_ITEM;
        break;
    case POKELIST_MENU_READ_MAIL:
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = wk->cursorPos;
        wk->param->result = POKELIST_RESULT_READ_MAIL;
        break;
    case POKELIST_MENU_TAKE_MAIL:
        PokeList_ShowMessage(wk, BtlMain11_Text_SendRemovedMailPc, FALSE, PokeList_AskTakeMail);
        break;
    case POKELIST_MENU_SELECT:
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
        wk->param->index = wk->cursorPos;
        wk->param->result = POKELIST_RESULT_DONE;
        break;
    case POKELIST_MENU_MOVE1:
    case POKELIST_MENU_MOVE2:
    case POKELIST_MENU_MOVE3:
    case POKELIST_MENU_MOVE4:
        if (wk->param->mode == POKELIST_MODE_USE_ITEM) {
            if (StatusRcv_CanUseItem(wk->pkm, wk->param->item, wk->menuItem - POKELIST_MENU_MOVE1, wk->heapId) ==
                TRUE) {
                u32 move = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + wk->menuItem - POKELIST_MENU_MOVE1, NULL);

                GFL_BGSysLoadScr(0);
                PokeList_ShowItemResult(wk, move);
                StatusRcv_UseItem(wk->pkm, wk->param->item, wk->menuItem - POKELIST_MENU_MOVE1, wk->param->zoneId,
                                  wk->heapId);
                PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
                PokeList_SubFromBag(wk, wk->param->item);
                GFL_SndSEPlay(SEQ_SE_RECOVERY);
            } else {
                PokeList_ShowItemUseless(wk);
            }
        } else {
            u32 hm = PokeList_GetHMResult(wk->pkm, wk->menuItem - POKELIST_MENU_MOVE1);

            if (hm == POKELIST_RESULT_MILK_DRINK || hm == POKELIST_RESULT_SOFTBOILED) {
                if (IsZoneNormal(wk->param->zoneId) == FALSE) {
                    PokeList_ShowMessage(wk, BtlMain11_Text_CantUseHere, TRUE, PokeList_MessageDoneSelect);
                } else {
                    u32 maxHp = PokeParty_GetParam(wk->pkm, PKM_PARAM_MAX_HP, NULL);
                    u32 hp = PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL);

                    if (hp > maxHp / 5) {
                        PokeList_ShowCursor(wk, wk->cursorPos);
                        PokeList_ShowSubCursor(wk, wk->cursorPos);
                        wk->selectPos2 = wk->cursorPos;
                        wk->pkm = NULL;
                        wk->state = POKELIST_STATE_SELECT_HP_SHARE;
                        wk->showCloseAndShortcut = FALSE;
                        PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORT);
                        PokeListMessage_Print(wk, wk->message, BtlMain11_Text_UseWhichPokemon_2);
                        PokeList_RefreshCursor(wk);
                    } else {
                        PokeList_ShowMessage(wk, BtlMain11_Text_NotEnoughHp, TRUE, PokeList_MessageDoneSelect);
                    }
                }
            } else {
                switch (CheckAllowHMEvent(hm - POKELIST_RESULT_CUT, &wk->param->action)) {
                case POKELIST_HM_CHECK_ALLOWED:
                    wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
                    wk->param->index = wk->cursorPos;
                    wk->param->result = hm;
                    break;
                case POKELIST_HM_CHECK_UNAVAILABLE:
                    PokeList_ShowMessage(wk, BtlMain11_Text_CantUseHere, TRUE, PokeList_MessageDoneSelect);
                    break;
                case POKELIST_HM_CHECK_BADGE_REQUIRED:
                    PokeList_ShowMessage(wk, BtlMain11_Text_CantUsedUntilNew, TRUE, PokeList_MessageDoneSelect);
                    break;
                case POKELIST_HM_CHECK_WITH_PARTNER:
                    PokeListMessage_CreateWordSet(wk, wk->message);
                    PokeListMessage_SetMoveName(wk, wk->message, 0, *(sHMMoves + hm - POKELIST_RESULT_CUT));
                    PokeList_ShowMessage(wk, BtlMain11_Text_CannotUsedWhenSomeone, TRUE, PokeList_MessageDoneSelect);
                    PokeListMessage_FreeWordSet(wk, wk->message);
                    break;
                case POKELIST_HM_CHECK_ALREADY_SURFING:
                    PokeList_ShowMessage(wk, BtlMain11_Text_YoureAlreadySurfing, TRUE, PokeList_MessageDoneSelect);
                    break;
                }
            }
        }
        break;
    default:
        PokeList_ShowCursor(wk, wk->cursorPos);
        wk->pkm = NULL;
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowModeMessage(wk);
        break;
    }
}

// Waits for the message to be read
static void PokeList_WaitMessage(PokeListWork *wk) {
    if (PokeListMessage_IsDone(wk, wk->message) == TRUE) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            wk->touch = FALSE;
        }
        if (GFL_HIDIsTouchPressed() == TRUE) {
            wk->touch = TRUE;
        }
        wk->msgDoneFunc(wk);
    }
}

void PokeList_ShowMessage(PokeListWork *wk, u32 msgId, BOOL waitInput, void (*doneFunc)(PokeListWork *wk)) {
    wk->state = POKELIST_STATE_MESSAGE;
    wk->msgWaitInput = waitInput;
    wk->msgDoneFunc = doneFunc;
    PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_WIDE);
    PokeListMessage_PrintStream(wk, wk->message, msgId, waitInput);
    ClActor_SetVisible(wk->backButton, FALSE);
    ClActor_SetVisible(wk->closeButton, FALSE);
    ClActor_SetVisible(wk->shortcutMark, FALSE);
}

static void PokeList_UpdateSubMenu(PokeListWork *wk) {
    u32 item;

    PokeListMenu_Update(wk, wk->menu);
    item = PokeListMenu_GetPicked(wk, wk->menu);
    if (item != POKELIST_MENU_NONE) {
        wk->menuFunc(wk, item);
        PokeListMenu_Close(wk, wk->menu);
    }
}

static void PokeList_OpenSubMenu(PokeListWork *wk, void (*func)(PokeListWork *wk, u32 item)) {
    wk->state = POKELIST_STATE_SUBMENU;
    wk->menuFunc = func;
    PokeListMenu_OpenYesNo(wk, wk->menu);
}

// Slides the plates of the two Pokémon being swapped out, swaps them, and slides them back in
static void PokeList_UpdateSwap(PokeListWork *wk) {
    switch (wk->subState) {
    case POKELIST_SUBSTATE_SWAP_OUT:
        PokeListPlate_ClearSlid(wk, wk->plates[wk->cursorPos], wk->swapStep);
        PokeListPlate_ClearSlid(wk, wk->plates[wk->selectPos], wk->swapStep);
        wk->swapStep++;
        PokeListPlate_DrawSlid(wk, wk->plates[wk->cursorPos], wk->swapStep);
        PokeListPlate_DrawSlid(wk, wk->plates[wk->selectPos], wk->swapStep);
        if (wk->swapStep >= 16) {
            PokeParty_SwapPkms(wk->param->party, wk->cursorPos, wk->selectPos, wk->heapId);
            PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], PokeParty_GetPkm(wk->param->party, wk->cursorPos),
                                 wk->swapStep);
            PokeListPlate_SetPkm(wk, wk->plates[wk->selectPos], PokeParty_GetPkm(wk->param->party, wk->selectPos),
                                 wk->swapStep);
            wk->subState = POKELIST_SUBSTATE_SWAP_IN;
            GFL_SndSEPlay(SEQ_SE_SYS_03);
        }
        break;
    case POKELIST_SUBSTATE_SWAP_IN:
        PokeListPlate_ClearSlid(wk, wk->plates[wk->cursorPos], wk->swapStep);
        PokeListPlate_ClearSlid(wk, wk->plates[wk->selectPos], wk->swapStep);
        wk->swapStep--;
        PokeListPlate_DrawSlid(wk, wk->plates[wk->cursorPos], wk->swapStep);
        PokeListPlate_DrawSlid(wk, wk->plates[wk->selectPos], wk->swapStep);
        if (wk->swapStep == 0) {
            PokeList_EndSwap(wk);
        }
        break;
    }
}

static void PokeList_StartSwap(PokeListWork *wk) {
    wk->state = POKELIST_STATE_SWAP;
    wk->subState = POKELIST_SUBSTATE_SWAP_OUT;
    wk->swapStep = 0;
    GFL_SndSEPlay(SEQ_SE_SYS_02);
    ClActor_SetVisible(wk->cursor, FALSE);
    ClActor_SetVisible(wk->subCursor, FALSE);
}

static void PokeList_EndSwap(PokeListWork *wk) {
    wk->pkm = NULL;
    wk->state = POKELIST_STATE_SELECT;
    PokeList_ShowCursor(wk, wk->cursorPos);
    PokeListPlate_SetSelected(wk, wk->plates[wk->selectPos], FALSE);
    ClActor_SetVisible(wk->subCursor, FALSE);
    wk->selectPos = POKELIST_CURSOR_NONE;
    wk->param->index = wk->cursorPos;
    PokeList_ShowModeMessage(wk);
}

// The result that a move used outside battle ends the list with, or 0 for a move that can't be
u32 PokeList_GetHMResult(PartyPkm *pkm, u8 slot) {
    switch (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL)) {
    case MOVE_CUT:
        return POKELIST_RESULT_CUT;
    case MOVE_FLY:
        return POKELIST_RESULT_FLY;
    case MOVE_SURF:
        return POKELIST_RESULT_SURF;
    case MOVE_STRENGTH:
        return POKELIST_RESULT_STRENGTH;
    case MOVE_WATERFALL:
        return POKELIST_RESULT_WATERFALL;
    case MOVE_FLASH:
        return POKELIST_RESULT_FLASH;
    case MOVE_TELEPORT:
        return POKELIST_RESULT_TELEPORT;
    case MOVE_DIG:
        return POKELIST_RESULT_DIG;
    case MOVE_SWEET_SCENT:
        return POKELIST_RESULT_SWEET_SCENT;
    case MOVE_CHATTER:
        return POKELIST_RESULT_CHATTER;
    case MOVE_DIVE:
        return POKELIST_RESULT_DIVE;
    case MOVE_MILK_DRINK:
        return POKELIST_RESULT_MILK_DRINK;
    case MOVE_SOFTBOILED:
        return POKELIST_RESULT_SOFTBOILED;
    }
    return POKELIST_RESULT_DONE;
}

BOOL PokeList_IsBattle(PokeListWork *wk) {
    if (wk->param->mode == POKELIST_MODE_BATTLE_TEAM || wk->param->mode == POKELIST_MODE_BATTLE_TEAM_CANCEL ||
        wk->param->mode == POKELIST_MODE_BATTLE_INSTITUTE) {
        return TRUE;
    }
    return FALSE;
}

// Whether the cancel button is shown next to the battle's decide button
static BOOL PokeList_HasBattleCancelButton(PokeListWork *wk) {
    if (wk->param->mode == POKELIST_MODE_BATTLE_TEAM) {
        return FALSE;
    }
    return TRUE;
}

static BOOL PokeList_HasBackButton(PokeListWork *wk) {
    if (wk->param->mode == POKELIST_MODE_SELECT_EGG) {
        return FALSE;
    }
    return TRUE;
}

// Whether a Pokémon can learn the move to teach: 0 into an empty slot, 1 in place of a move, 2 not at all, and 3 if
// it knows it already
u32 PokeList_CheckLearnMove(PokeListWork *wk, PartyPkm *pkm, u8 pos) {
    u8 i;
    BOOL hasEmptySlot = FALSE;
    u32 ret;

    if (wk->wasTutorMode == TRUE) {
        u32 moveA = wk->param->move;
        u32 moveB;
        u32 moveC;

        if (moveA == POKELIST_MOVE_ULTIMATE) {
            moveA = MOVE_FRENZY_PLANT;
            moveB = MOVE_BLAST_BURN;
            moveC = MOVE_HYDRO_CANNON;
        } else if (moveA == POKELIST_MOVE_PLEDGE) {
            moveA = MOVE_GRASS_PLEDGE;
            moveB = MOVE_FIRE_PLEDGE;
            moveC = MOVE_WATER_PLEDGE;
        } else {
            moveB = moveA;
            moveC = moveA;
        }
        for (i = 0; i < 4; i++) {
            u32 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

            if (move == moveA || move == moveB || move == moveC) {
                return POKELIST_LEARN_KNOWN;
            }
            if (move == 0) {
                hasEmptySlot = TRUE;
            }
        }
        if (wk->param->learnable & (1 << pos)) {
            if (hasEmptySlot == TRUE) {
                return POKELIST_LEARN_EMPTY_SLOT;
            }
            return POKELIST_LEARN_REPLACE_MOVE;
        }
        return POKELIST_LEARN_UNABLE;
    }
    for (i = 0; i < 4; i++) {
        u32 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

        if (move == wk->param->move) {
            return POKELIST_LEARN_KNOWN;
        }
        if (move == 0) {
            hasEmptySlot = TRUE;
        }
    }
    if (wk->param->item != 0) {
        u8 tm = PML_ItemGetTMBitMask(wk->param->item);

        if (tm != TM_INDEX_NONE && canPkmLearnTM_Wrapper(pkm, tm) == TRUE) {
            ret = POKELIST_LEARN_EMPTY_SLOT;
            if (hasEmptySlot != TRUE) {
                ret = POKELIST_LEARN_REPLACE_MOVE;
            }
            return ret;
        }
        return POKELIST_LEARN_UNABLE;
    }
    ret = POKELIST_LEARN_EMPTY_SLOT;
    if (hasEmptySlot != TRUE) {
        ret = POKELIST_LEARN_REPLACE_MOVE;
    }
    return ret;
}

BOOL PokeList_CanEvolveWithItem(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    return CheckEvolveSpecies(wk->param->party, pkm, EVO_CHECK_ITEM, item, wk->param->season, NULL, wk->heapId);
}

void PokeList_PrintString(PokeListWork *wk, BmpWin *window, u16 msgId, u16 x, s16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(window), x, y, str, wk->font, color);
    GFL_StrBufFree(str);
}

void PokeList_PrintStringSmall(PokeListWork *wk, BmpWin *window, u32 msgId, int x, s16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(window), x, y, str, wk->smallFont, color);
    GFL_StrBufFree(str);
}

// Draws a string at once, without the print queue
void PokeList_DrawStringSmall(PokeListWork *wk, BmpWin *window, u32 msgId, int x, s16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), x, y, str, wk->smallFont, color);
    GFL_StrBufFree(str);
}

void PokeList_PrintWordSetString(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                 u16 color) {
    StrBuf *buf = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, buf, str);
    PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(window), x, y, buf, wk->font, color);
    GFL_StrBufFree(str);
    GFL_StrBufFree(buf);
}

void PokeList_PrintWordSetStringSmall(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                      u16 color) {
    StrBuf *buf = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, buf, str);
    PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(window), x, y, buf, wk->smallFont, color);
    GFL_StrBufFree(str);
    GFL_StrBufFree(buf);
}

void PokeList_DrawWordSetStringSmall(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                     u16 color) {
    StrBuf *buf = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, buf, str);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), x, y, buf, wk->smallFont, color);
    GFL_StrBufFree(str);
    GFL_StrBufFree(buf);
}

// Called once a message is read: back to picking a Pokémon
void PokeList_MessageDoneSelect(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    wk->state = POKELIST_STATE_SELECT;
    PokeList_ShowModeMessage(wk);
}

// Called once a message is read: the list ends
void PokeList_MessageDoneExit(PokeListWork *wk) {
    wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
}

// Called once a message about using an item is read: back to picking a Pokémon, if there are items left
void PokeList_MessageDoneItem(PokeListWork *wk) {
    if (PokeList_GetBagCount(wk, wk->param->item)) {
        PokeListMessage_Close(wk, wk->message);
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowModeMessage(wk);
    } else {
        PokeListMessage_Close(wk, wk->message);
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetItemName(wk, wk->message, 0, wk->param->item);
        PokeList_ShowMessage(wk, BtlMain11_Text_UsedLast, TRUE, PokeList_MessageDoneExit);
        PokeListMessage_FreeWordSet(wk, wk->message);
    }
}

// Asks whether to replace an old move with the new one
void PokeList_AskReplaceMove(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerReplaceMove);
}

static void PokeList_AnswerReplaceMove(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == POKELIST_MENU_YES) {
        if (wk->param->mode == POKELIST_MODE_USE_ITEM || wk->param->mode == POKELIST_MODE_REPLACE_LEVEL_MOVE) {
            wk->param->result = POKELIST_RESULT_REPLACE_LEVEL_MOVE;
        } else {
            wk->param->result = POKELIST_RESULT_REPLACE_TM_MOVE;
        }
        PokeList_ShowMessage(wk, BtlMain11_Text_WhichMoveShouldForgotten, TRUE, PokeList_MessageDoneExit);
    } else {
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
        PokeList_ShowMessage(wk, BtlMain11_Text_GiveUpLearningMove, FALSE, PokeList_AskCancelLearning);
        PokeListMessage_FreeWordSet(wk, wk->message);
    }
}

// Asks whether to give up learning the new move
static void PokeList_AskCancelLearning(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerCancelLearning);
}

static void PokeList_AnswerCancelLearning(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == POKELIST_MENU_YES) {
        wk->param->result = POKELIST_RESULT_ITEM_DONE;
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
        if (wk->param->mode == POKELIST_MODE_USE_ITEM || wk->param->mode == POKELIST_MODE_REPLACE_LEVEL_MOVE) {
            PokeList_ShowMessage(wk, BtlMain11_Text_DidNotLearn, TRUE, PokeList_LearnMessageDone);
        } else {
            PokeList_ShowMessage(wk, BtlMain11_Text_DidNotLearn, TRUE, PokeList_MessageDoneExit);
        }
        PokeListMessage_FreeWordSet(wk, wk->message);
    } else {
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
        PokeList_ShowMessage(wk, BtlMain11_Text_WantsLearnMoveHowever, FALSE, PokeList_AskReplaceMove);
        PokeListMessage_FreeWordSet(wk, wk->message);
    }
}

// The move was forgotten: the new one takes its slot
static void PokeList_LearnInSlot(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
    if (wk->param->mode == POKELIST_MODE_USE_ITEM || wk->param->mode == POKELIST_MODE_REPLACE_LEVEL_MOVE) {
        PokeList_ShowMessage(wk, BtlMain11_Text_Learned, TRUE, PokeList_LearnMessageDone);
    } else {
        PokeList_ShowMessage(wk, BtlMain11_Text_Learned, TRUE, PokeList_MessageDoneExit);
        PokeList_RaiseFriendship(wk, wk->pkm);
        wk->param->result = POKELIST_RESULT_ITEM_DONE;
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
    PokeList_SetMove(wk, wk->pkm, wk->param->moveSlot);
}

// Asks whether to swap the item held for the one given
static void PokeList_AskSwapItem(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerSwapItem);
}

static void PokeList_AnswerSwapItem(PokeListWork *wk, u32 item) {
    if (item == POKELIST_MENU_YES) {
        u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        if (PokeList_GetBagCount(wk, heldItem) == 999) {
            if (wk->param->mode == POKELIST_MODE_GIVE_FROM_BAG) {
                wk->param->mode = POKELIST_MODE_FIELD;
                PokeListMessage_Close(wk, wk->message);
                PokeList_ShowMessage(wk, BtlMain11_Text_BagFull, TRUE, PokeList_MessageDoneSelect);
            } else {
                wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
                PokeListMessage_Close(wk, wk->message);
                PokeList_ShowMessage(wk, BtlMain11_Text_BagFull, TRUE, PokeList_MessageDoneExit);
            }
        } else if (PML_ItemIsMail(wk->param->item) == TRUE) {
            wk->param->index = wk->cursorPos;
            wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
            wk->param->result = POKELIST_RESULT_WRITE_MAIL;
        } else {
            BOOL griseousForm;
            BOOL plateForm;

            PokeListMessage_Close(wk, wk->message);
            PokeList_SetHeldItem(wk, wk->pkm, wk->param->item);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeList_UpdateArceusForm(wk, wk->pkm, wk->param->item);
            PokeList_UpdateGenesectForm(wk, wk->pkm, wk->param->item);
            griseousForm = PokeListDemo_CanBecomeOrigin(wk, wk->pkm);
            plateForm = PokeListDemo_CanBecomeAltered(wk, wk->pkm);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetItemName(wk, wk->message, 1, heldItem);
            PokeListMessage_SetItemName(wk, wk->message, 2, wk->param->item);
            if (wk->param->mode == POKELIST_MODE_GIVE_FROM_BAG) {
                wk->param->mode = POKELIST_MODE_FIELD;
                if (griseousForm == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, BtlMain11_Text_TakenReplaced, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = POKELIST_DEMO_GIRATINA_ORIGIN;
                } else if (plateForm == TRUE) {
                    PokeListDemo_SetAltered(wk, wk->pkm);
                    PokeList_ShowMessage(wk, BtlMain11_Text_TakenReplaced, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = POKELIST_DEMO_GIRATINA_ALTERED;
                } else {
                    PokeList_ShowMessage(wk, BtlMain11_Text_TakenReplaced, TRUE, PokeList_MessageDoneSelect);
                }
            } else {
                wk->param->result = POKELIST_RESULT_ITEM_DONE;
                if (griseousForm == TRUE) {
                    PokeListDemo_SetOrigin(wk, wk->pkm);
                    PokeList_ShowMessage(wk, BtlMain11_Text_TakenReplaced, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = POKELIST_DEMO_GIRATINA_ORIGIN;
                } else if (plateForm == TRUE) {
                    PokeListDemo_SetAltered(wk, wk->pkm);
                    PokeList_ShowMessage(wk, BtlMain11_Text_TakenReplaced, TRUE, PokeList_MessageDoneDemo);
                    wk->demo = POKELIST_DEMO_GIRATINA_ALTERED;
                } else {
                    PokeList_ShowMessage(wk, BtlMain11_Text_TakenReplaced, TRUE, PokeList_MessageDoneExit);
                }
            }
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
    } else if (wk->param->mode == POKELIST_MODE_GIVE_FROM_BAG) {
        wk->param->mode = POKELIST_MODE_FIELD;
        PokeListMessage_Close(wk, wk->message);
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowModeMessage(wk);
    } else {
        wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
    }
}

// The user's HP has gone down: the HP it shares is given to the Pokémon picked, whose HP goes up next
static void PokeList_ShareHpDone(PokeListWork *wk) {
    PartyPkm *user = PokeParty_GetPkm(wk->param->party, wk->selectPos2);
    // Read but not used
    u32 userHp = PokeParty_GetParam(user, PKM_PARAM_HP, NULL);
    u32 userMaxHp = PokeParty_GetParam(user, PKM_PARAM_MAX_HP, NULL);
    PartyPkm *target = PokeParty_GetPkm(wk->param->party, wk->param->index);
    u32 hp = PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL);
    u32 maxHp = PokeParty_GetParam(wk->pkm, PKM_PARAM_MAX_HP, NULL);

    wk->cursorPos = wk->param->index;
    if (hp + userMaxHp / 5 > maxHp) {
        PokeParty_SetParam(target, PKM_PARAM_HP, maxHp);
    } else {
        PokeParty_SetParam(target, PKM_PARAM_HP, hp + userMaxHp / 5);
    }
    wk->hpDoneFunc = PokeList_ShareHpEnd;
    wk->state = POKELIST_STATE_HP_CHANGE_START;
    wk->playHealSe = TRUE;
}

static void PokeList_ShareHpEnd(PokeListWork *wk) {
    u16 hp = PokeListPlate_GetHp(wk, wk->plates[wk->param->index]);

    wk->cursorPos = wk->param->index;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    if (wk->prevHp != 0) {
        PokeListMessage_SetNumber(wk, wk->message, 1, hp - wk->prevHp, 3);
        PokeList_ShowMessage(wk, BtlMain11_Text_SHpRestoredBy, TRUE, PokeList_MessageDoneSelect);
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
}

// Asks whether to take the mail
static void PokeList_AskTakeMail(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerTakeMail);
}

static void PokeList_AnswerTakeMail(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == POKELIST_MENU_YES) {
        s32 slot = MailBox_GetFreeSlot(wk->param->mailbox, 0);

        if (slot >= 0) {
            MailData *mail = CreateMailData(wk->heapId);

            PokeParty_GetParam(wk->pkm, PKM_PARAM_MAIL, mail);
            MailBox_SetMail(wk->param->mailbox, 0, slot, mail);
            GFL_HeapFree(mail);
            PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, 0);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeList_ShowMessage(wk, BtlMain11_Text_MailSentPc, TRUE, PokeList_MessageDoneSelect);
        } else {
            PokeList_ShowMessage(wk, BtlMain11_Text_PcsMailboxFull, TRUE, PokeList_MessageDoneSelect);
        }
    } else {
        PokeList_ShowMessage(wk, BtlMain11_Text_IfMailRemovedIts, FALSE, PokeList_AskDeleteMail);
    }
}

// Asks whether to take the mail anyway, which loses its message
static void PokeList_AskDeleteMail(PokeListWork *wk) {
    PokeList_OpenSubMenu(wk, PokeList_AnswerDeleteMail);
}

static void PokeList_AnswerDeleteMail(PokeListWork *wk, u32 item) {
    PokeListMessage_Close(wk, wk->message);
    if (item == POKELIST_MENU_YES) {
        u32 heldItem = PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL);

        if (PokeList_GetBagCount(wk, heldItem) == 999) {
            PokeList_ShowMessage(wk, BtlMain11_Text_BagFull, TRUE, PokeList_MessageDoneSelect);
        } else {
            PokeList_AddToBag(wk, heldItem);
            PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, 0);
            PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
            PokeList_ShowMessage(wk, BtlMain11_Text_MailTakenFromPokemon, TRUE, PokeList_MessageDoneSelect);
        }
    } else {
        PokeListMessage_Close(wk, wk->message);
        wk->state = POKELIST_STATE_SELECT;
        PokeList_ShowModeMessage(wk);
    }
}

// Called once a message is read: the form change is shown
static void PokeList_MessageDoneDemo(PokeListWork *wk) {
    PokeListMessage_Close(wk, wk->message);
    wk->state = POKELIST_STATE_DEMO_START;
}

// The time to pick a team ran out
void PokeList_TimeUp(PokeListWork *wk) {
    if (wk->timeUp == FALSE && wk->state != POKELIST_STATE_EXIT_WAIT_BUTTON &&
        wk->state != POKELIST_STATE_EXIT_WAIT_WIPE && wk->state != POKELIST_STATE_FADE_IN &&
        wk->state != POKELIST_STATE_WAIT_WIPE) {
        if (wk->state == POKELIST_STATE_MENU) {
            PokeList_EndMenu(wk);
        }
        wk->timeUp = TRUE;
        wk->state = wk->syncOnTimeUp == TRUE ? POKELIST_STATE_SYNC_START : POKELIST_STATE_EXIT_FADE;
        wk->param->index = POKELIST_CURSOR_CONFIRM;
        wk->param->result = POKELIST_RESULT_DONE;
        if (wk->decided == FALSE) {
            Regulation *regulation;
            u8 i;

            for (i = 0; i < 6; i++) {
                wk->param->picked[i] = 0;
            }
            regulation = wk->param->regulation;
            if (PokeRegulation_PickTeam(regulation, wk->param->party, wk->param->picked) == 0) {
                for (i = 0; i < 6; i++) {
                    wk->param->picked[i] = 0;
                }
                for (i = 0; i < regulation->minCount; i++) {
                    wk->param->picked[i] = i + 1;
                }
            }
        }
    }
}

static void PokeList_LearnMove(PokeListWork *wk, PartyPkm *pkm) {
    if (wk->param->mode == POKELIST_MODE_USE_ITEM || wk->param->mode == POKELIST_MODE_REPLACE_LEVEL_MOVE) {
        PokeParty_LearnMove(pkm, wk->param->move);
    } else {
        PokeParty_LearnMoveTutor(pkm, wk->param->move);
    }
}

static void PokeList_SetMove(PokeListWork *wk, PartyPkm *pkm, u8 slot) {
    if (wk->param->mode == POKELIST_MODE_USE_ITEM || wk->param->mode == POKELIST_MODE_REPLACE_LEVEL_MOVE) {
        PokeParty_SetMove(pkm, wk->param->move, slot);
    } else {
        PokeParty_SetMoveKeepPP(pkm, wk->param->move, slot);
    }
}

static void PokeList_RaiseFriendship(PokeListWork *wk, PartyPkm *pkm) {
    FriendshipManagerCalc(pkm, 7, wk->param->zoneId, wk->heapId);
}

// Gives a Pokémon an item from the bag, putting the item it held back in the bag
static void PokeList_SetHeldItem(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    u32 heldItem = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);

    if (heldItem != 0) {
        PokeList_AddToBag(wk, heldItem);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, item);
    if (item != 0) {
        PokeList_SubFromBag(wk, item);
    }
}

static u16 PokeList_GetBagCount(PokeListWork *wk, u16 item) {
    return BagSave_GetItemCountByID(wk->param->bag, item, wk->heapId);
}

static void PokeList_SubFromBag(PokeListWork *wk, u16 item) {
    BagSave_SubItem(wk->param->bag, item, 1, wk->heapId);
}

static void PokeList_AddToBag(PokeListWork *wk, u16 item) {
    BagSave_AddItem(wk->param->bag, item, 1, wk->heapId);
}
