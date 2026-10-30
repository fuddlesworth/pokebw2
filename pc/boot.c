#include "boot.h"
#include "app/bag.h"
#include "app/boot_screens.h"
#include "app/cdemo.h"
#include "app/config_panel.h"
#include "app/demo3d.h"
#include "app/event_battle_return.h"
#include "app/game_start.h"
#include "app/name_entry.h"
#include "app/pms_select.h"
#include "app/pokedex.h"
#include "app/pokedex_list.h"
#include "app/pokedex_registration.h"
#include "app/pokelist.h"
#include "app/psel.h"
#include "app/start_menu.h"
#include "app/title.h"
#include "app/townmap.h"
#include "audio.h"
#include "battle/battle_proc.h"
#include "battle_ui.h"
#include "demo/evolution_demo.h"
#include "demo/intro.h"
#include "dex_ui.h"
#include "field/event_season_banner.h"
#include "field/field.h"
#include "field/field_3dci.h"
#include "field/field_menu.h"
#include "field/mailsys.h"
#include "field/trcard_sys.h"
#include "filesystem.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gx_backend.h"
#include "menu_ui.h"
#include "nitro/host_gx.h"
#include "nitro/os.h"
#include "nnsys/gfd.h"
#include "party_ui.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save.h"
#include "save/config.h"
#include "save/high_link.h"
#include "settings.h"
#include "system/boot_proc.h"
#include "system/brightness.h"
#include "system/game_beacon.h"
#include "system/game_system.h"
#include "system/gfl_use.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "timing.h"
#include "ui_components.h"
#include "ui_draw.h"
#include <SDL3/SDL.h>

static GameProcManager *manager;
static char *paths[308];
static SystemUI input;
static bool mouseHeld, touchPressed;
static bool completed;
u32 PcOsIrqCheck;
u32 PcOsVBlankCount;
static u64 ticks;
static bool traceDex;
static Uint64 dexOpenTime, dexOpenTick;
static char *replayData;
static const char *replayCursor;
static unsigned long long replayFrame;
static unsigned replayKeys, replayNextKeys;
static TCBManager *vblank, *hblank;
static NNSGfdVramTransferTask vramTransfers[48];
static PcGxVertex *fieldVertices;
static u32 fieldVertexCount;
static u32 fieldVertexCapacity;
static u32 fieldSparseFrames;
static u16 pendingBackKeys;

bool PcBoot_Start(const char *root, PcBootEntry entry) {
    ticks = 0;
    pendingBackKeys = 0;
    traceDex = SDL_getenv("POKEBW2_TRACE_DEX") != NULL;
    dexOpenTime = 0;
    PcOsVBlankCount = 0;
    input.frameCount = 0;
    PcFs_SetRoot(root);
    NNS_GfdInitVramTransferManager(vramTransfers, SDL_arraysize(vramTransfers));
    const char *replayPath = SDL_getenv("POKEBW2_INPUT_REPLAY");
    if (replayPath) {
        replayData = SDL_LoadFile(replayPath, NULL);
        if (!replayData)
            return false;
        replayCursor = replayData;
        replayFrame = 0;
        replayKeys = replayNextKeys = 0;
    }
    GFL_HeapCreateChild(HEAPID_SYSTEM, HEAPID_USER, 0x1a3e00);
    initTableArea(HEAPID_SYSTEM);
    input.keypad = GFL_HIDCreateKeypadManager(HEAPID_SYSTEM);
    input.updateRate = PC_GAME_HZ;
    for (unsigned i = 0; i < SDL_arraysize(paths); i++) {
        if (SDL_asprintf(&paths[i], "a/%u/%u/%u", i / 100, (i / 10) % 10, i % 10) < 0)
            return false;
    }
    GFL_ArcSysInit((const char **)paths, SDL_arraysize(paths));
    PML_SpeciesNamesResidentInit(HEAPID_SYSTEM);
    PML_PersonalInit(HEAPID_SYSTEM);
    GFL_FadeCreate(HEAPID_SYSTEM);
    GFL_TextRndInit(HEAPID_SYSTEM);
    RTCCache_Init();
    GFL_NetHeapInit();
    GFL_NetErrInit(HEAPID_NET);
    initKeyBlk();
    GameBeaconSys_Create(HEAPID_USER);
    PassPower_Init();
    if (!PcAudio_Init(root))
        return false;
    vblank =
        GFL_TCBMgrCreate(32, GFL_HeapAllocate(HEAPID_SYSTEM, GFL_TCBMgrCalcAllocSize(32), TRUE, __FILE__, __LINE__));
    hblank =
        GFL_TCBMgrCreate(32, GFL_HeapAllocate(HEAPID_SYSTEM, GFL_TCBMgrCalcAllocSize(32), TRUE, __FILE__, __LINE__));
    GCTX_ProcMgrInit(HEAPID_SYSTEM);
    manager = GCTX_ProcMgrGet();
    completed = false;
    if (entry >= PC_BOOT_MENU && !PcSave_Init())
        return false;
    const GameProcFunctions *process = entry == PC_BOOT_NEW_GAME ? &NEW_GAME_PROC_FUNCTIONS
                                       : entry == PC_BOOT_MENU   ? &START_MENU_PROC_FUNCTIONS
                                       : entry == PC_BOOT_TITLE  ? &TITLE_PROC_FUNCTIONS
                                                                 : &EVENT_TITLE_FUNCS;
    QueueGameProc(manager, OVERLAY_NONE, process, NULL);
    return true;
}
void PcBoot_Update(void) {
    const GameProcFunctions *current = GCTX_ProcMgrCurrentFunctions();
    BOOL battleScene = current == &BATTLE_PROC_FUNCTIONS;
    PcGx.wide_scene = current == &FIELDMAP_PROC_FUNCS || current == &gDemo3DProcFunctions || battleScene;
    PcGx.projection_scale_x = battleScene ? 138.0f / 288.0f : 0.75f;
    bool mouseDown = !PcBattleUi_IsActive() && !PcDexUi_IsActive() && !PcPartyUi_IsActive() && !PcMenuUi_IsActive() &&
                     (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK) != 0;
    touchPressed = mouseDown && !mouseHeld;
    mouseHeld = mouseDown;
    GCTX_HIDUpdateKeypad();
    input.frameCount++;
    ticks++;
    PcOsVBlankCount++;
    PcAudio_Update();
    RTCCache_Update();
    GFL_NetErrUpdate();
    GameBeaconSys_Update();
    PassPower_Update();
    BrightnessController_Update();
    GFL_FadeUpdate();
    GFL_WipeMain();
    PC_GX_REG16(0x04000006) = 192;
    GFL_VBlankExecCallback();
    NNS_GfdDoVramTransfer();
    GFL_TCBMgrUpdate(vblank);
    GFL_BGSysUpdate();
    PC_GX_REG16(0x04000006) = 0;
    if (!completed)
        completed = GFL_ProcMgrUpdate(manager) == 0;
}
void PcBoot_Stop(void) {
    // Closing the host session is not a game event. Never run scripts or synthesize input during teardown.
    // Audio and save handles must close while their game-heap allocations still exist.
    PcAudio_Shutdown();
    PcSave_Shutdown();
    manager = NULL;
    completed = true;
    GameBeaconSys = NULL;
    GFL_ArcSysInit(NULL, 0);
    PcFs_SetRoot(NULL);
    SDL_free(replayData);
    replayData = NULL;
    replayCursor = NULL;
    SDL_free(fieldVertices);
    fieldVertices = NULL;
    fieldVertexCount = fieldVertexCapacity = 0;
    fieldSparseFrames = 0;
    for (unsigned i = 0; i < SDL_arraysize(paths); i++) {
        SDL_free(paths[i]);
        paths[i] = NULL;
    }
}
void PcBoot_Draw(void) {
    PcViewport scene = { 48, 0, 288, 216 };
    BOOL naming = GCTX_ProcMgrCurrentFunctions() == &NAME_ENTRY_PROC_FUNCTIONS;
    if (naming)
        scene = (PcViewport){ 128, 12, 256, 192 };
    GFL_G3DSysCheckSwapBuffers();
    BOOL fieldScene = GCTX_ProcMgrCurrentFunctions() == &FIELDMAP_PROC_FUNCS;
    BOOL fieldReady = fieldScene && g_GameSystem != NULL && GSYS_GetField(g_GameSystem) != NULL &&
                      Field_CheckMapLoadFinished(GSYS_GetField(g_GameSystem));
    if (fieldReady) {
        // Map handoffs can submit a nearly empty 3D list for one tick; keep the last full field until it recovers.
        if (PcGx.vertex_count &&
            (!fieldVertexCount || PcGx.vertex_count * 2 >= fieldVertexCount || fieldSparseFrames >= 2)) {
            fieldSparseFrames = 0;
            if (PcGx.vertex_count > fieldVertexCapacity) {
                PcGxVertex *vertices = SDL_realloc(fieldVertices, PcGx.vertex_count * sizeof(*fieldVertices));
                if (vertices) {
                    fieldVertices = vertices;
                    fieldVertexCapacity = PcGx.vertex_count;
                }
            }
            if (PcGx.vertex_count <= fieldVertexCapacity) {
                fieldVertexCount = PcGx.vertex_count;
                SDL_memcpy(fieldVertices, PcGx.vertices, fieldVertexCount * sizeof(*fieldVertices));
            }
        } else if (fieldVertexCount) {
            if (PcGx.vertex_count)
                fieldSparseFrames++;
            SDL_memcpy(PcGx.vertices, fieldVertices, fieldVertexCount * sizeof(*fieldVertices));
            PcGx.vertex_count = fieldVertexCount;
        }
    } else {
        fieldVertexCount = 0;
        fieldSparseFrames = 0;
    }
    PcGx.composite_scene = true;
    PcGx.scene_viewport = PcGx.wide_scene ? (PcViewport){ 0, 0, PC_CANVAS_WIDTH, PC_CANVAS_HEIGHT } : scene;
    BOOL battleScene = PcBattleUi_IsActive();
    if (battleScene) {
        PcGx.scene_viewport = (PcViewport){ 0, 0, PC_CANVAS_WIDTH, 138 };
        // BG0 is the arena. Messages and gauges are composed from their original live state below.
    } else if (fieldScene && PcGx_WideCaptureActive()) {
        PcGx_DrawBackgroundLayers(0, scene, 256, 192, 15 & ~(1u << 2));
        u32 first = PcGx.vertex_count;
        PcGx_DrawBackgroundLayers(0, (PcViewport){ 0, 0, PC_CANVAS_WIDTH, PC_CANVAS_HEIGHT }, PC_CANVAS_WIDTH,
                                  PC_CANVAS_HEIGHT, 1u << 2);
        u32 blend = PcGxRegisters[0].words[20];
        if ((blend & (REG_G2_BLDCNT_EFFECT_MASK | GX_BLEND_PLANEMASK_BG2)) ==
            (REG_G2_BLDCNT_EFFECT_ALPHA | GX_BLEND_PLANEMASK_BG2)) {
            float alpha = SDL_min((blend >> 16) & 31u, 16u) / 16.0f;
            for (u32 i = first; i < PcGx.vertex_count; i++)
                PcGx.vertices[i].color[3] = alpha;
        }
        for (u32 i = first; i < PcGx.vertex_count; i++) {
            PcGx.vertices[i].polygon |= PC_GX_POLYGON_WIDE_CAPTURE;
            PcGx.vertices[i].clip[2] = -0.99f;
        }
    } else if (fieldScene && Fld3DCi_IsNativeSceneActive()) {
        PcGx_DrawBackgroundLayers(0, (PcViewport){ 0, 0, PC_CANVAS_WIDTH, PC_CANVAS_HEIGHT }, 256, 192, 15);
    } else if (fieldScene)
        PcGx_DrawBackgroundLayers(0, scene, 256, 192, 15);
    else
        PcGx_DrawBackgroundsInViewport(0, scene, 256, 192);
    bool seasonBanner = EventSeasonBanner_IsNativeActive();
    bool placeBanner = fieldReady && FieldPlaceName_IsNativeActive(Field_GetPlaceName(GSYS_GetField(g_GameSystem)));
    if (seasonBanner || placeBanner) {
        // Repeat the location pattern into the margins; the season splash extends its plain edge pixels.
        float edgeWidth = seasonBanner ? 0 : scene.x * 256.0f / scene.width;
        PcGx_DrawBackgroundRegion(0, (PcViewport){ 0, 0, scene.x, PC_CANVAS_HEIGHT },
                                  (SDL_FRect){ seasonBanner ? 0 : 256 - edgeWidth, 0, edgeWidth, 192 }, 1u << 3);
        unsigned right = scene.x + scene.width;
        PcGx_DrawBackgroundRegion(0, (PcViewport){ right, 0, PC_CANVAS_WIDTH - right, PC_CANVAS_HEIGHT },
                                  (SDL_FRect){ seasonBanner ? 255 : 0, 0, edgeWidth, 192 }, 1u << 3);
    }
    if (!battleScene) {
        if (placeBanner && !seasonBanner)
            PcGx_DrawBannerObjects(0, scene);
        else
            PcGx_DrawObjects(0, scene);
    }
    if (FieldMenu_IsOpenNative() || GCTX_ProcMgrCurrentFunctions() == &CONFIG_PANEL_PROC_FUNCTIONS ||
        GCTX_ProcMgrCurrentFunctions() == &PselProcFunctions ||
        GCTX_ProcMgrCurrentFunctions() == &PSTATUS_PROC_FUNCTIONS ||
        GCTX_ProcMgrCurrentFunctions() == &BAG_PROC_FUNCTIONS) {
        PcViewport menu = { 208, 18, 160, 120 };
        PcGx_DrawBackgroundsInViewport(1, menu, 256, 192);
        PcGx_DrawObjects(1, menu);
    }
    if (naming) {
        PcViewport prompt = { 8, 48, 120, 90 };
        PcGx_DrawBackgroundsInViewport(1, prompt, 256, 192);
        PcGx_DrawObjects(1, prompt);
    }
    if (GCTX_ProcMgrCurrentFunctions() == &TITLE_PROC_FUNCTIONS) {
        // The original lower-screen logo and prompt become an overlay on the animated title scene.
        u32 first = PcGx.vertex_count;
        PcGx_DrawBackgroundLayers(1, scene, 256, 192, (1u << 1) | (1u << 3));
        PcGx_DrawObjects(1, scene);
        for (u32 i = first; i < PcGx.vertex_count; i++)
            PcGx.vertices[i].clip[2] -= 0.4f;
    }
    if (battleScene && PcBattleUi_Draw())
        return;
    bool partyDrawn = PcPartyUi_Draw();
    bool menuDrawn = PcMenuUi_Draw();
    if (!partyDrawn && !menuDrawn)
        PcDexUi_Draw();
    if (dexOpenTime && PcGx.dex_ui) {
        SDL_Log("Pokédex first frame: %.1f ms, %llu game ticks", (SDL_GetTicksNS() - dexOpenTime) / 1000000.0,
                (unsigned long long)(ticks - dexOpenTick));
        dexOpenTime = 0;
    }
    int wipeType;
    float wipeProgress;
    if (fieldScene && GFL_WipeGetHostState(&wipeType, &wipeProgress)) {
        u32 first = PcGx.vertex_count;
        PcUi_Quad(0, 0, PC_UI_WIDTH, PC_UI_HEIGHT, -0.99f, 0, 0, 0, 0, 0, 0, 1, 1);
        for (u32 i = first; i < PcGx.vertex_count; i++)
            PcGx.vertices[i].polygon |= PC_GX_POLYGON_WIPE;
    }
}
u64 OS_GetTick(void) {
    return (ticks + 1) * OS_SYSTEM_CLOCK / (64 * PC_GAME_HZ);
}
TCB *GFL_VBlankTCBAdd(TCBFunc func, void *data, u32 priority) {
    return GFL_TCBMgrAddTask(vblank, func, data, priority);
}
TCB *GFL_HBlankTCBAdd(TCBFunc func, void *data, u32 priority) {
    return GFL_TCBMgrAddTask(hblank, func, data, priority);
}
TCBManager *GFL_VBlankGetTCBMgr(void) {
    return vblank;
}
static u16 MapNativeUiKeys(u16 keys) {
    keys |= pendingBackKeys;
    pendingBackKeys = 0;
    return PcBattleUi_FilterKeys(PcMenuUi_FilterKeys(PcPartyUi_FilterKeys(PcDexUi_FilterKeys(keys))));
}

bool PcBoot_HandleEvent(const SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_ESCAPE) {
        if (!event->key.repeat)
            pendingBackKeys |= PAD_BUTTON_B;
        return true;
    }
    return false;
}

u16 PcPad_Read(void) {
    if (replayData) {
        while (replayCursor && ticks >= replayFrame) {
            replayKeys = replayNextKeys;
            if (!*replayCursor) {
                replayCursor = NULL;
                break;
            }
            int consumed = 0;
            unsigned long long frame;
            unsigned keys;
            if (SDL_sscanf(replayCursor, "%llu %x%n", &frame, &keys, &consumed) != 2 || !consumed ||
                frame < replayFrame || keys > 0xfff)
                GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid input replay entry");
            replayCursor += consumed;
            while (*replayCursor == '\n' || *replayCursor == '\r' || *replayCursor == ' ')
                replayCursor++;
            replayFrame = frame;
            replayNextKeys = keys;
        }
        if (replayCursor)
            return MapNativeUiKeys((u16)replayKeys);
    }
    u16 pad = PcUi_KeyboardPad(PcMenuUi_IsActive() || PcDexUi_IsActive() || PcPartyUi_IsActive() ||
                               PcBattleUi_IsActive() || FieldMenu_IsOpenNative());
    if (FieldMenu_IsOpenNative() && SDL_GetKeyboardState(NULL)[SDL_SCANCODE_ESCAPE])
        pad |= PAD_BUTTON_B;
    return MapNativeUiKeys(pad);
}
SystemUI *GCTX_HIDGetInstance(void) {
    return &input;
}
void GCTX_HIDUnblockSoftReset(u8 flags) {
    input.softResetBlock &= ~flags;
}
void GCTX_HIDBlockSoftReset(u8 flags) {
    input.softResetBlock |= flags;
}
BOOL GCTX_HIDIsSoftResetBlocked(u8 flags) {
    return (input.softResetBlock & flags) != 0;
}
void GCTX_HIDResetFrameCount(void) {
    input.frameCount = 0;
}
void GCTX_HIDChangeFPS(u8 rate) {
    input.updateRate = rate;
    GFL_HIDClearKeypadState(&input);
    mouseHeld = touchPressed = false;
}
void GCTX_HIDSetLidCallbacks(UICallback close, UICallback open, void *work) {
    input.lidCloseCallback = close;
    input.lidOpenCallback = open;
    input.lidWork = work;
}
void GCTX_HIDUpdate(void) {
    SDL_PumpEvents();
    GCTX_HIDUpdateKeypad();
    input.frameCount++;
}
void OS_WaitIrq(BOOL clear, OSIrqMask interrupts) {
    (void)clear;
    if (interrupts != OS_IE_V_BLANK)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Unsupported native interrupt wait %u", interrupts);
    SDL_Delay(16);
    PcOsVBlankCount++;
    PC_GX_REG16(0x04000006) = 192;
    // Hardware captures the armed display on VBlank, including during synchronous waits.
    PcGx_Capture();
    GFL_VBlankExecCallback();
    NNS_GfdDoVramTransfer();
    GFL_TCBMgrUpdate(vblank);
    GFL_BGSysUpdate();
    PC_GX_REG16(0x04000006) = 0;
}
void OS_WaitInterrupt(BOOL clear, OSIrqMask interrupts) {
    OS_WaitIrq(clear, interrupts);
}
void OS_WaitVBlankIntr(void) {
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
}
KeypadManager *GFL_HIDGetKeypadManager(SystemUI *ui) {
    return ui->keypad;
}
u8 GCTX_HIDGetUpdateRate(void) {
    return input.updateRate;
}
void GCTX_HIDSetUpdateRate(u8 rate) {
    input.updateRate = rate;
}
BOOL GCTX_HIDIsLidClosed(void) {
    return FALSE;
}
BOOL GetIsShouldUICallback(void) {
    return FALSE;
}
void SetUICallbackStatic(UICallback callback, void *work) {
    input.lowBatteryCallback = callback;
    input.lowBatteryWork = work;
}
void GCTX_HIDSetSleepCheckCallback(UISleepCheckCallback callback, void *work) {
    input.sleepCheckCallback = callback;
    input.sleepCheckWork = work;
}
void GCTX_HIDSetSleepCallback(UICallback callback, void *work) {
    input.sleepCallback = callback;
    input.sleepWork = work;
}
void GCTX_HIDSetWakeCallback(UICallback callback, void *work) {
    input.wakeCallback = callback;
    input.sleepWork = work;
}
UICallback GCTX_HIDGetLidCloseCallback(void) {
    return input.lidCloseCallback;
}
int GCTX_HIDGetBatteryLevel(void) {
    int percent = 100;
    SDL_PowerState state = SDL_GetPowerInfo(NULL, &percent);
    if (state == SDL_POWERSTATE_ON_BATTERY && percent >= 0)
        return (percent * 5 + 99) / 100;
    return 5;
}
void GCTX_HIDSetSoftResetCallback(SoftResetCallback callback, void *work) {
    input.softResetCallback = callback;
    input.softResetWork = work;
}
BOOL GFL_HIDIsTouchPressed(void) {
    return touchPressed;
}
BOOL GFL_HIDIsTouchHeld(void) {
    return mouseHeld;
}
BOOL GCTX_HIDIsUsingTouch(void) {
    return input.usingTouch;
}
void GCTX_HIDSetUsingTouch(BOOL touch) {
    input.usingTouch = touch;
}
BOOL GFL_HIDGetTouchPressedPoint(u32 *x, u32 *y) {
    if (!touchPressed)
        return FALSE;
    return GFL_HIDGetTouchHeldPoint(x, y);
}
BOOL GFL_HIDGetTouchHeldPoint(u32 *x, u32 *y) {
    if (!mouseHeld)
        return FALSE;
    SDL_Window *window = SDL_GetMouseFocus();
    int width, height;
    float mouseX, mouseY, canvasX, canvasY;
    if (!window || !SDL_GetWindowSize(window, &width, &height) || width <= 0 || height <= 0)
        return FALSE;
    SDL_GetMouseState(&mouseX, &mouseY);
    int pixelWidth, pixelHeight;
    if (!SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight) || pixelWidth <= 0 || pixelHeight <= 0)
        return FALSE;
    if (!PcViewport_MapPointer(PcSettings_Viewport(pixelWidth, pixelHeight), mouseX * pixelWidth / width,
                               mouseY * pixelHeight / height, &canvasX, &canvasY))
        return FALSE;
    if (GCTX_ProcMgrCurrentFunctions() == &NAME_ENTRY_PROC_FUNCTIONS) {
        if (canvasX < 128 || canvasX >= 384 || canvasY < 12 || canvasY >= 204)
            return FALSE;
        *x = (u32)(canvasX - 128);
        *y = (u32)(canvasY - 12);
        return TRUE;
    }
    if (canvasX < 48 || canvasX >= 336)
        return FALSE;
    *x = (u32)((canvasX - 48) * 256 / 288);
    *y = (u32)(canvasY * 192 / 216);
    return TRUE;
}
s32 GFL_HIDGetTouchRectAt(const TouchRect *rects, u32 x, u32 y) {
    for (s32 i = 0; rects[i].top != TOUCH_RECT_END; i++) {
        const TouchRect *rect = &rects[i];
        if (rect->top == TOUCH_RECT_SKIP)
            continue;
        if (rect->top == TOUCH_RECT_CIRCLE) {
            // Circle records store center X, center Y, radius in the remaining three bytes.
            u32 dx = rect->bottom - x;
            u32 dy = rect->left - y;
            if (dx * dx + dy * dy < (u32)rect->right * rect->right)
                return i;
        } else if (x - rect->left <= (u32)(rect->right - rect->left) &&
                   y - rect->top <= (u32)(rect->bottom - rect->top)) {
            return i;
        }
    }
    return TOUCH_RECT_NONE;
}
s32 GFL_HIDGetTouchHeldRect(const TouchRect *rects) {
    u32 x, y;
    return GFL_HIDGetTouchHeldPoint(&x, &y) ? GFL_HIDGetTouchRectAt(rects, x, y) : TOUCH_RECT_NONE;
}
s32 GFL_HIDGetTouchPressedRect(const TouchRect *rects) {
    u32 x, y;
    return GFL_HIDGetTouchPressedPoint(&x, &y) ? GFL_HIDGetTouchRectAt(rects, x, y) : TOUCH_RECT_NONE;
}
const GameProcFunctions *PcProc_Find(const char *name) {
    SDL_Log("Starting original application %s", name);
    if (SDL_strcmp(name, "gDemo3DProcFunctions") == 0)
        return &gDemo3DProcFunctions;
    if (SDL_strcmp(name, "CONTINUE_PROC_FUNCTIONS") == 0)
        return &CONTINUE_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "CONFIG_PANEL_PROC_FUNCTIONS") == 0)
        return &CONFIG_PANEL_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "FIELDMAP_PROC_FUNCS") == 0)
        return &FIELDMAP_PROC_FUNCS;
    if (SDL_strcmp(name, "BATTLE_PROC_FUNCTIONS") == 0)
        return &BATTLE_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "EVENT_BATTLE_RETURN_PROC_FUNCTIONS") == 0)
        return &EVENT_BATTLE_RETURN_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "POKEDEX_REGISTRATION_PROC_FUNCTIONS") == 0)
        return &POKEDEX_REGISTRATION_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "EVOLUTION_DEMO_PROC_FUNCTIONS") == 0)
        return &EVOLUTION_DEMO_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "GAMESYSTEM_PROC_FUNCTIONS") == 0)
        return &GAMESYSTEM_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "INTRO_PROC_FUNCTIONS") == 0)
        return &INTRO_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "NAME_ENTRY_PROC_FUNCTIONS") == 0)
        return &NAME_ENTRY_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "NEW_GAME_PROC_FUNCTIONS") == 0)
        return &NEW_GAME_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "PselProcFunctions") == 0)
        return &PselProcFunctions;
    if (SDL_strcmp(name, "POKELIST_PROC_FUNCTIONS") == 0)
        return &POKELIST_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "PSTATUS_PROC_FUNCTIONS") == 0)
        return &PSTATUS_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "BAG_PROC_FUNCTIONS") == 0)
        return &BAG_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "MAILSYS_PROC_FUNCTIONS") == 0)
        return &MAILSYS_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "TOWNMAP_PROC_FUNCTIONS") == 0)
        return &TOWNMAP_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "TRCARD_SYS_PROC_FUNCTIONS") == 0)
        return &TRCARD_SYS_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "PMS_SELECT_PROC_FUNCTIONS") == 0)
        return &PMS_SELECT_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "POKEDEX_PROC_FUNCTIONS") == 0) {
        if (traceDex) {
            dexOpenTime = SDL_GetTicksNS();
            dexOpenTick = ticks;
        }
        return &POKEDEX_PROC_FUNCTIONS;
    }
    if (SDL_strcmp(name, "START_MENU_PROC_FUNCTIONS") == 0) {
        if (!PcSave_Init())
            GFL_DebugAssertFailEx(__FILE__, __LINE__, "Opening native save: %s", SDL_GetError());
        return &START_MENU_PROC_FUNCTIONS;
    }
    if (SDL_strcmp(name, "TITLE_PROC_FUNCTIONS") == 0)
        return &TITLE_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "BOOT_SCREENS_PROC_FUNCTIONS") == 0)
        return &BOOT_SCREENS_PROC_FUNCTIONS;
    if (SDL_strcmp(name, "CDEMO_PROC_FUNCTIONS") == 0)
        return &CDEMO_PROC_FUNCTIONS;
    GFL_DebugAssertFailEx(__FILE__, __LINE__, "Native application %s has not been linked", name);
    return NULL;
}
BOOL GFL_OvlLoad(u32 overlayId) {
    // Native function references are resolved by the linker; overlay code stays resident.
    // Application entry points are checked separately by PcProc_Find.
    (void)overlayId;
    return TRUE;
}
void GFL_OvlUnload(u32 overlayId) {
    (void)overlayId;
}
void OS_Halt(void) {
    GFL_DebugAssertFailEx(__FILE__, __LINE__, "Unexpected halt in native process manager");
}
void OS_Terminate(void) {
    GFL_DebugAssertFailEx(__FILE__, __LINE__, "Original game requested termination");
}
