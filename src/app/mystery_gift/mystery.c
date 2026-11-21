#include "types.h"
#include "app/mystery/mystery_album.h"
#include "app/mystery/mystery_check.h"
#include "app/mystery/mystery_gift_data.h"
#include "app/mystery/mystery_graphic.h"
#include "app/mystery/mystery_net.h"
#include "app/mystery/mystery_util.h"
#include "app/mystery_gift.h"
#include "app/wifi_login.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/particle.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "pml/item.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/mystery_gift.h"
#include "save/player_info.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/version.h"
#include "system/wordset.h"

enum {
    TOP_CHOICE_RECEIVE = 0,
    TOP_CHOICE_ALBUM = 1,
    TOP_CHOICE_INFO = 2,
    TOP_CHOICE_QUIT = 3,
    TOP_CHOICE_MAX = 4,
};

enum {
    RECEIVE_CHOICE_WIRELESS = 0,
    RECEIVE_CHOICE_WIFI = 1,
    RECEIVE_CHOICE_INFRARED = 2,
    RECEIVE_CHOICE_BACK = 3,
    RECEIVE_CHOICE_MAX = 4,
};

enum {
    ABOUT_CHOICE_GIFT = 0,
    ABOUT_CHOICE_ALBUM = 1,
    ABOUT_CHOICE_FULL_ALBUM = 2,
    ABOUT_CHOICE_BACK = 3,
    ABOUT_CHOICE_MAX = 4,
};

enum {
    RECEIVED_CHOICE_CARD = 0,
    RECEIVED_CHOICE_MAX = 1,
};

enum {
    FADE_IN_STATE_START = 0,
    FADE_IN_STATE_WAIT = 1,
    FADE_IN_STATE_FINISH = 2,
};

enum {
    FADE_OUT_STATE_START = 0,
    FADE_OUT_STATE_WAIT = 1,
    FADE_OUT_STATE_FINISH = 2,
};

enum {
    TOP_STATE_SHOW_MENU = 0,
    TOP_STATE_WAIT_MENU = 1,
    TOP_STATE_SHOW_RECEIVE_PROMPT = 2,
    TOP_STATE_OPEN_RECEIVE_CONFIRM = 3,
    TOP_STATE_WAIT_RECEIVE_CONFIRM = 4,
    TOP_STATE_START_WIRELESS = 5,
    TOP_STATE_WAIT_WIRELESS = 6,
    TOP_STATE_SHOW_METHOD_MENU = 7,
    TOP_STATE_WAIT_METHOD_MENU = 8,
    TOP_STATE_SHOW_METHOD_PROMPT = 9,
    TOP_STATE_OPEN_METHOD_CONFIRM = 10,
    TOP_STATE_WAIT_METHOD_CONFIRM = 11,
    TOP_STATE_STOP_WIRELESS = 12,
    TOP_STATE_WAIT_WIRELESS_STOP = 13,
    TOP_STATE_STOP_CANCELLED_WIRELESS = 14,
    TOP_STATE_WAIT_CANCELLED_STOP = 15,
    TOP_STATE_SHOW_FULL_PROMPT = 16,
    TOP_STATE_OPEN_FULL_CONFIRM = 17,
    TOP_STATE_WAIT_FULL_CONFIRM = 18,
    TOP_STATE_WAIT_MESSAGE = 19,
};

enum {
    ABOUT_STATE_SHOW_MENU = 0,
    ABOUT_STATE_WAIT_MENU = 2,
    ABOUT_STATE_WAIT_MESSAGE = 3,
};

enum {
    RECEIVE_STATE_START = 0,
    RECEIVE_STATE_WAIT_DATA = 1,
    RECEIVE_STATE_SHOW_STOPPING = 2,
    RECEIVE_STATE_WAIT_STOPPING = 3,
    RECEIVE_STATE_RETURN_TOP = 4,
    RECEIVE_STATE_SHOW_CARD = 5,
    RECEIVE_STATE_WAIT_CARD = 6,
    RECEIVE_STATE_OPEN_ACCEPT_CONFIRM = 7,
    RECEIVE_STATE_WAIT_ACCEPT_CONFIRM = 8,
    RECEIVE_STATE_OPEN_CANCEL_CONFIRM = 9,
    RECEIVE_STATE_WAIT_CANCEL_CONFIRM = 10,
    RECEIVE_STATE_SHOW_ALREADY_RECEIVED = 11,
    RECEIVE_STATE_SHOW_INVALID = 12,
    RECEIVE_STATE_CLEANUP = 13,
    RECEIVE_STATE_WAIT_MESSAGE = 14,
};

enum {
    RECEIVED_STATE_START_EFFECT = 0,
    RECEIVED_STATE_ENABLE_EFFECT_BG = 1,
    RECEIVED_STATE_WAIT_MESSAGE = 2,
    RECEIVED_STATE_RUN_EFFECT = 3,
    RECEIVED_STATE_START_SAVE = 4,
    RECEIVED_STATE_WAIT_SAVE = 5,
    RECEIVED_STATE_START_FADE_BACK = 6,
    RECEIVED_STATE_RUN_FADE_BACK = 7,
    RECEIVED_STATE_SHOW_SAVED = 8,
    RECEIVED_STATE_WAIT_SAVED = 9,
    RECEIVED_STATE_CLEANUP_EFFECT = 10,
    RECEIVED_STATE_RESTORE_MENU_BG = 11,
    RECEIVED_STATE_CREATE_ALBUM = 12,
    RECEIVED_STATE_OPEN_ALBUM = 13,
    RECEIVED_STATE_WAIT_ALBUM = 14,
    RECEIVED_STATE_WAIT_KEY = 15,
    RECEIVED_STATE_CREATE_KEY_HINT = 16,
    RECEIVED_STATE_WAIT_KEY_HINT = 17,
    RECEIVED_STATE_FINISH = 18,
};

enum {
    ALBUM_SEQ_STATE_FADE_OUT = 0,
    ALBUM_SEQ_STATE_CREATE_VIEW = 1,
    ALBUM_SEQ_STATE_FADE_IN = 2,
    ALBUM_SEQ_STATE_RUN_VIEW = 3,
    ALBUM_SEQ_STATE_FADE_OUT_VIEW = 4,
    ALBUM_SEQ_STATE_RESTORE_MENU = 5,
    ALBUM_SEQ_STATE_FADE_IN_MENU = 6,
    ALBUM_SEQ_STATE_RETURN_TOP = 7,
    ALBUM_SEQ_STATE_WAIT_FADE = 8,
};

enum {
    FULL_ALBUM_SEQ_STATE_FADE_OUT = 0,
    FULL_ALBUM_SEQ_STATE_CREATE_VIEW = 1,
    FULL_ALBUM_SEQ_STATE_FADE_IN = 2,
    FULL_ALBUM_SEQ_STATE_RUN_VIEW = 3,
    FULL_ALBUM_SEQ_STATE_FADE_OUT_VIEW = 4,
    FULL_ALBUM_SEQ_STATE_RESTORE_MENU = 5,
    FULL_ALBUM_SEQ_STATE_FADE_IN_MENU = 6,
    FULL_ALBUM_SEQ_STATE_RESUME_RECEIVE = 7,
    FULL_ALBUM_SEQ_STATE_WAIT_FADE = 8,
};

enum {
    WIFI_LOGIN_STATE_FADE_OUT = 0,
    WIFI_LOGIN_STATE_WAIT_FADE_OUT = 1,
    WIFI_LOGIN_STATE_FREE_GRAPHICS = 2,
    WIFI_LOGIN_STATE_QUEUE_LOGIN = 3,
    WIFI_LOGIN_STATE_RESUME = 4,
    WIFI_LOGIN_STATE_CREATE_GRAPHICS = 5,
    WIFI_LOGIN_STATE_FADE_IN = 6,
    WIFI_LOGIN_STATE_WAIT_FADE_IN = 7,
    WIFI_LOGIN_STATE_CHECK_RESULT = 8,
};

enum {
    WIRELESS_OFF_STATE_SHOW_MESSAGE = 0,
    WIRELESS_OFF_STATE_WAIT_MESSAGE = 1,
};

enum {
    MYSTERY_BG_MENU = 0,
    MYSTERY_BG_CARD_TEXT = 1,
    MYSTERY_BG_EFFECT = 2,
};

enum {
    MYSTERY_SCROLL_SLOW = 0,
    MYSTERY_SCROLL_FAST = 1,
};

enum {
    MYSTERY_EFFECT_FALL = 0,
    MYSTERY_EFFECT_FADE_BACK = 1,
};

enum {
    FALL_STATE_LOAD_BG = 0,
    FALL_STATE_SHOW_BG = 1,
    FALL_STATE_FADE_BG = 2,
    FALL_STATE_WAIT_FALL = 3,
    FALL_STATE_FALL_ICON = 4,
    FALL_STATE_WAIT_END = 5,
    FALL_STATE_END = 6,
};

enum {
    SPECIAL_FALL_STATE_LOAD_PARTICLES = 0,
    SPECIAL_FALL_STATE_SHOW_BG = 1,
    SPECIAL_FALL_STATE_FADE_BG = 2,
    SPECIAL_FALL_STATE_START_PARTICLES = 3,
    SPECIAL_FALL_STATE_SHOW_ICON = 4,
    SPECIAL_FALL_STATE_FADE_ICON = 5,
    SPECIAL_FALL_STATE_WAIT_PARTICLES = 6,
    SPECIAL_FALL_STATE_WAIT_ICON = 7,
    SPECIAL_FALL_STATE_WAIT_PALETTE = 8,
    SPECIAL_FALL_STATE_FADE_PALETTE = 9,
    SPECIAL_FALL_STATE_WAIT_END = 10,
    SPECIAL_FALL_STATE_END = 11,
};

enum {
    FADE_BACK_STATE_START = 0,
    FADE_BACK_STATE_FADE = 1,
    FADE_BACK_STATE_END = 2,
};

// Mystery Gift, from the start menu: receiving gifts by wireless, infrared or Wi-Fi, the effect of a received gift
// and the album of cards. Our names; swan has none for this overlay

// The menus of Mystery_CreateList
enum {
    LIST_TOP,
    LIST_RECEIVE,
    LIST_RECEIVED,
    LIST_ABOUT,
};

// How a gift is received, from the receive menu
enum {
    RECEIVE_WIRELESS,
    RECEIVE_WIFI,
    RECEIVE_INFRARED,
};

// The BGs' graphics: the main screens', the window of a received card's text, and the effect's
typedef struct {
    HeapID heapId;
} MysteryBg;

// The falling stars on both screens, the menu's cursor and the hint of the receive effect, and the colors the
// cursor's palette cycles through
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
    // The cursor, the stars, then the hint
    ClActor *actors[35];
    u16 palFrame;
    u16 colors[16];
    u16 toColors[16];
    u16 fromColors[16];
} MysteryActors;

#define STAR_COUNT 32
enum {
    ACTOR_CURSOR,
    ACTOR_STARS,
    ACTOR_HINT = 34,
};

// The particles of a special gift's effect
typedef struct {
    ParticleSystem *sys;
    u32 unk4;
    void *work;
    BOOL active;
} MysteryParticle;

typedef struct MysteryBgScroll MysteryBgScroll;

// Scrolls the BGs behind the menus and moves the stars with them
struct MysteryBgScroll {
    BOOL active;
    BOOL enabled;
    u16 frame;
    fx32 pos;
    fx32 speed;
    // The speed when it last changed, which the speed eases from
    fx32 startSpeed;
    fx32 starY[STAR_COUNT];
    fx32 starSpeed[STAR_COUNT];
    MysteryActors *actors;
    void (*func)(MysteryBgScroll *scroll);
    u32 unk120;
    HeapID heapId;
};

typedef struct MysteryEffect MysteryEffect;

// The effect of a received gift: its Pokémon, item or other icon falls from the top screen, and the album opens
struct MysteryEffect {
    HeapID heapId;
    ClActor *actor;
    BOOL done;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u32 frame;
    u32 subFrame;
    u32 state;
    MysteryBg *bg;
    MysteryBgScroll *scroll;
    void (*func)(MysteryEffect *effect);
    u16 blend;
    u16 bgFadeColors[16];
    u16 subFadeColors[16];
    // The palette of the gift's kind, and the normal one
    u16 kindColors[16];
    u16 baseColors[16];
    u16 objFadeColors[16];
    u16 whiteColors[16];
    u16 objColors[16];
    const MysteryGiftRecvData *recv;
    MysteryParticle particle;
    // Where the icon stops, from the bottom of the Pokémon's sprite
    s16 height;
    s16 offsetX;
};

// "Press A" shown as an OAM text over the album
typedef struct {
    BmpOamSys *bmpOam;
    MysteryOamText *text;
    const MysteryActors *actors;
} MysteryOamMsg;

typedef struct {
    MysteryAlbum *album;
    MysteryCardRes *cardRes;
    MysteryCardView *cardView;
    MysteryEffect effect;
    MysteryBgScroll scroll;
    MysteryYesNo *yesNo;
    MysteryOamMsg oamMsg;
    MysteryList *list;
    u32 listCursors[4];
    MysteryMsgWin *msgWin;
    MysteryTextWin *unk288;
    MysteryTextWin *cardWin;
    MysterySeq *seq;
    MysteryActors actors;
    MysteryBg bg;
    MysteryGraphic *graphic;
    Font *font;
    PrintQueue *queue;
    MsgData *scriptMsgData;
    MsgData *msgData;
    WordSet *wordSet;
    u32 receiveMode;
    MysteryNet *net;
    void *param;
    GameData *gameData;
    MysteryGiftSave *giftSave;
    WifiLoginParam *loginParam;
    u32 wait;
    // Set when the album was full and has a free slot again, to go on with receiving
    BOOL resumeReceive;
    MysteryGiftRecvData recv;
} MysteryWork;

static BOOL MysteryProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MysteryProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MysteryProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static void MysteryBg_Init(MysteryBg *bg, HeapID heapId);
static void MysteryBg_Exit(MysteryBg *bg);
static void MysteryBg_Load(MysteryBg *bg, u32 mode);
static void MysteryActors_Init(MysteryActors *actors, ClActUnit *unit, HeapID heapId);
static void MysteryActors_Exit(MysteryActors *actors);
static ClActor *MysteryActors_GetActor(const MysteryActors *actors, u32 index);
static void MysteryActors_LoadPalette(MysteryActors *actors, HeapID heapId);
static void MysteryActors_Update(MysteryActors *actors);
static void MysteryActors_ResetPalAnim(MysteryActors *actors);
static void MysteryParticle_Init(MysteryParticle *particle, HeapID heapId);
static void MysteryParticle_Exit(MysteryParticle *particle);
static void MysteryParticle_Load(MysteryParticle *particle, u32 arcId, u32 fileId, HeapID heapId);
static void MysteryParticle_Emit(MysteryParticle *particle, int emitter);
static BOOL MysteryParticle_IsActive(MysteryParticle *particle);
static void MysterySeq_Start(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_FadeIn(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_FadeOut(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_Top(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_About(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_Receive(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_Received(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_Album(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_AlbumFull(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_WifiLogin(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_Exit(MysterySeq *seq, u32 *state, void *work);
static void MysterySeq_WirelessOff(MysterySeq *seq, u32 *state, void *work);
static void Mystery_CreateYesNo(MysteryWork *wk, u32 unused, HeapID heapId);
static void Mystery_DeleteYesNo(MysteryWork *wk);
static void Mystery_CreateList(MysteryWork *wk, u32 type, HeapID heapId);
static void Mystery_DeleteList(MysteryWork *wk);
static u32 Mystery_UpdateList(MysteryWork *wk);
static void Mystery_OnListMove(void *work);
static void Mystery_CreateCardWin(MysteryWork *wk, HeapID heapId);
static void Mystery_DeleteCardWin(MysteryWork *wk);
static void MysteryEffect_Init(MysteryEffect *effect, ClActUnit *unit, const MysteryGiftRecvData *recv,
                               GameData *gameData, MysteryActors *actors, MysteryBg *bg, MysteryBgScroll *scroll,
                               HeapID heapId);
static void MysteryEffect_Start(MysteryEffect *effect, u32 mode);
static void MysteryEffect_Exit(MysteryEffect *effect);
static void MysteryEffect_Main(MysteryEffect *effect);
static BOOL MysteryEffect_IsEnd(MysteryEffect *effect);
static void MysteryEffect_Fall(MysteryEffect *effect);
static void MysteryEffect_FallSpecial(MysteryEffect *effect);
static void MysteryEffect_FadeBack(MysteryEffect *effect);
static void MysteryBgScroll_Init(MysteryBgScroll *scroll, MysteryActors *actors, HeapID heapId);
static void MysteryBgScroll_Exit(MysteryBgScroll *scroll);
static void MysteryBgScroll_Main(MysteryBgScroll *scroll);
static void MysteryBgScroll_SetSpeed(MysteryBgScroll *scroll, u32 mode);
static void MysteryBgScroll_SetEnabled(MysteryBgScroll *scroll, BOOL enabled);
static void MysteryBgScroll_Slow(MysteryBgScroll *scroll);
static void MysteryBgScroll_Fast(MysteryBgScroll *scroll);
static void MysteryOamMsg_Init(MysteryOamMsg *msg, const MysteryActors *actors, ClActUnit *unit, PrintQueue *queue,
                               MsgData *msgData, u32 msgId, Font *font, HeapID heapId);
static void MysteryOamMsg_Exit(MysteryOamMsg *msg);
static BOOL MysteryOamMsg_Update(MysteryOamMsg *msg);
static u32 Mystery_GetSpriteBottom(BoxPkm *pkm, HeapID heapId);
static BOOL Mystery_IsRecvDataValid(const MysteryGiftRecvData *recv, u32 errors);

// The BG palettes of the main and of the sub screen. No code loads from it: its reads have constant indices
static const u8 sBgPalTypes[2] = { PALTYPE_MAIN_BG, PALTYPE_SUB_BG };

// The palette of the stars and the effect for each kind of gift
static const u16 sKindPalettes[7] = { 0, 1, 2, 6, 6, 3, 0 };

const GameProcFunctions MYSTERY_GIFT_PROC_FUNCTIONS = {
    MysteryProc_Init,
    MysteryProc_Main,
    MysteryProc_Exit,
};

static BOOL MysteryProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MysteryWork *wk;

    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MYSTERY, 0xa0000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MysteryWork), HEAPID_MYSTERY);
    MI_CpuFill8(wk, 0, sizeof(MysteryWork));
    wk->param = param;
    wk->gameData = GameData_Create(HEAPID_MYSTERY);
    wk->giftSave = mysteryGiftBlock(GameData_GetSaveControl(wk->gameData), 1, HEAPID_MYSTERY);
    wk->graphic = MysteryGraphic_Create(1, HEAPID_MYSTERY);
    MysteryBg_Init(&wk->bg, HEAPID_MYSTERY);
    MysteryActors_Init(&wk->actors, MysteryGraphic_GetClactUnit(wk->graphic), HEAPID_MYSTERY);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_MYSTERY);
    wk->queue = PrintQueue_CreateDefault(HEAPID_MYSTERY);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MYSTERY_GIFT, HEAPID_MYSTERY);
    wk->scriptMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, HEAPID_MYSTERY);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_MYSTERY);
    wk->seq = MysterySeq_Create(wk, MysterySeq_Start, HEAPID_MYSTERY);
    wk->msgWin = MysteryMsgWin_Create(1, 15, wk->queue, wk->font, HEAPID_MYSTERY);
    MysteryMsgWin_DrawFrame(wk->msgWin, 10, 14);
    wk->net = MysteryNet_Create(GameData_GetSaveControl(wk->gameData), HEAPID_MYSTERY);
    MysteryBgScroll_Init(&wk->scroll, &wk->actors, HEAPID_MYSTERY);
    MysteryBgScroll_SetSpeed(&wk->scroll, MYSTERY_SCROLL_SLOW);
    MysteryBgScroll_SetEnabled(&wk->scroll, TRUE);
    GFL_SndBGMPlay(SEQ_BGM_WIFI_PRESENT, 0xffff);
    return TRUE;
}

static BOOL MysteryProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MysteryWork *wk = work;

    MysteryBgScroll_Exit(&wk->scroll);
    MysteryNet_Delete(wk->net);
    if (wk->msgWin != NULL) {
        MysteryMsgWin_Delete(wk->msgWin);
    }
    MysterySeq_Delete(wk->seq);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->scriptMsgData);
    GFL_MsgDataFree(wk->msgData);
    PrintQueue_Free(wk->queue);
    GFL_FontFree(wk->font);
    MysteryActors_Exit(&wk->actors);
    MysteryBg_Exit(&wk->bg);
    MysteryGraphic_Delete(wk->graphic);
    MysteryGiftSave_Free(wk->giftSave);
    GameData_Free(wk->gameData);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MYSTERY);
    OS_ResetSystem(0);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_SndBGMStopImmediate();
    return TRUE;
}

static BOOL MysteryProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MysteryWork *wk = work;

    MysteryNet_Main(wk->net);
    MysterySeq_Main(wk->seq);
    if (wk->graphic != NULL) {
        MysteryGraphic_Update(wk->graphic);
        MysteryGraphic_BeginFrame3D(wk->graphic);
        MysteryGraphic_EndFrame3D(wk->graphic);
    }
    MysteryBgScroll_Main(&wk->scroll);
    PrintQueue_Main(wk->queue);
    if (wk->msgWin != NULL) {
        MysteryMsgWin_Update(wk->msgWin);
    }
    if (wk->cardWin != NULL) {
        MysteryTextWin_Update(wk->cardWin);
    }
    if (wk->unk288 != NULL) {
        MysteryTextWin_Update(wk->unk288);
    }
    if (wk->list != NULL) {
        MysteryList_UpdatePrint(wk->list);
    }
    if (MysterySeq_IsEnd(wk->seq)) {
        return TRUE;
    }
    return FALSE;
}

static void MysteryBg_Init(MysteryBg *bg, HeapID heapId) {
    MI_CpuFill8(bg, 0, sizeof(MysteryBg));
    bg->heapId = heapId;
    MysteryBg_Load(bg, MYSTERY_BG_MENU);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

static void MysteryBg_Exit(MysteryBg *bg) {
}

static void MysteryBg_Load(MysteryBg *bg, u32 mode) {
    HeapID heapId = HEAPID_TAIL(bg->heapId);
    ArcTool *arc;

    switch (mode) {
    case MYSTERY_BG_MENU:
        arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, sBgPalTypes[0], 0, 0, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, sBgPalTypes[1], 0, 0x80, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 7, sBgPalTypes[1], 0x20, 0x20, heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 3, 0, 0, FALSE, heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 7, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 3, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 7, 0, 0, FALSE, heapId);
        GFL_ArcToolFree(arc);
        arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 5, sBgPalTypes[0], 15 * 0x20, 0x20, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 5, sBgPalTypes[1], 15 * 0x20, 0x20, heapId);
        GFL_ArcToolFree(arc);
        LoadSysMsgBox(1, 10, 14, 0, heapId);
        LoadSysMsgBox(0, 1, 13, 0, heapId);
        GFL_BGSysClearCharCore(1, 0x20, 0, heapId);
        GFL_BGSysClearCharCore(0, 0x20, 0, heapId);
        break;
    case MYSTERY_BG_CARD_TEXT:
        arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, sBgPalTypes[1], 0, 0xc0, heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 6, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 21, 6, 0, 0, FALSE, heapId);
        GFL_ArcToolFree(arc);
        break;
    case MYSTERY_BG_EFFECT:
        arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 11, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 28, 2, 0, 0, FALSE, heapId);
        GFL_ArcToolFree(arc);
        break;
    }
}

static void MysteryActors_Init(MysteryActors *actors, ClActUnit *unit, HeapID heapId) {
    ArcTool *arc;
    NNSG2dPaletteData *palette;
    void *buffer;
    u16 *colors;
    ClActorSetup setup;
    int i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, heapId);
    actors->palette = ClActSys_LoadPltt(arc, 8, CLACT_VRAM_BOTH, 0, heapId);
    actors->cellAnims = ClActSys_LoadCellAnims(arc, 34, 37, heapId);
    actors->chars = ClActSys_LoadChars(arc, 16, FALSE, CLACT_VRAM_BOTH, heapId);
    buffer = GFL_G2DIOReadNCLRArc(arc, 8, &palette, heapId);
    colors = palette->pRawData;
    MI_CpuCopy8(colors, actors->fromColors, sizeof(actors->fromColors));
    colors += 16;
    MI_CpuCopy8(colors, actors->toColors, sizeof(actors->toColors));
    GFL_HeapFree(buffer);
    GFL_ArcToolFree(arc);
    MI_CpuFill8(&setup, 0, sizeof(setup));
    actors->actors[ACTOR_CURSOR] =
        ClActor_Create(unit, actors->chars, actors->palette, actors->cellAnims, &setup, 0, heapId);
    ClActor_SetVisible(actors->actors[ACTOR_CURSOR], FALSE);
    for (i = 0; i < STAR_COUNT; i++) {
        setup.x = GFL_RandomLCAlt(256);
        setup.y = GFL_RandomLCAlt(256) - 272;
        setup.bgPriority = 3;
        setup.sequence = GFL_RandomLCAlt(6) + 3;
        actors->actors[ACTOR_STARS + i] =
            ClActor_Create(unit, actors->chars, actors->palette, actors->cellAnims, &setup, 0, heapId);
        ClActor_SetVisible(actors->actors[ACTOR_STARS + i], FALSE);
    }
    setup.x = 128;
    setup.y = 177;
    setup.bgPriority = 0;
    setup.priority = 1;
    setup.sequence = 9;
    actors->actors[ACTOR_HINT] =
        ClActor_Create(unit, actors->chars, actors->palette, actors->cellAnims, &setup, 0, heapId);
    ClActor_SetVisible(actors->actors[ACTOR_HINT], FALSE);
}

static void MysteryActors_Exit(MysteryActors *actors) {
    int i;

    for (i = 0; i < 35; i++) {
        if (actors->actors[i] != NULL) {
            ClActor_Delete(actors->actors[i]);
        }
    }
    ClActSys_FreePltt(actors->palette);
    ClActSys_FreeChars(actors->chars);
    ClActSys_FreeCellAnims(actors->cellAnims);
}

static ClActor *MysteryActors_GetActor(const MysteryActors *actors, u32 index) {
    return actors->actors[index];
}

static void MysteryActors_LoadPalette(MysteryActors *actors, HeapID heapId) {
    NNSG2dPaletteData *palette;
    void *buffer = GFL_G2DIOReadNCLR(ARCID_MYSTERY, 8, &palette, heapId);

    ClActSys_ReplacePlttData(actors->palette, palette, 7);
    GFL_HeapFree(buffer);
}

static void MysteryActors_Update(MysteryActors *actors) {
    int i;

    if (actors->palFrame + 0x400 >= 0x10000) {
        actors->palFrame = actors->palFrame + 0x400 - 0x10000;
    } else {
        actors->palFrame += 0x400;
    }
    for (i = 0; i < 5; i++) {
        MysteryPal_BlendOne(14, &actors->colors[i], actors->palFrame, 0, i, actors->toColors[i], actors->fromColors[i]);
    }
}

static void MysteryActors_ResetPalAnim(MysteryActors *actors) {
    actors->palFrame = 0;
}

static void MysteryParticle_Init(MysteryParticle *particle, HeapID heapId) {
    MI_CpuFill8(particle, 0, sizeof(MysteryParticle));
    particle->active = TRUE;
    particle->work = GFL_HeapAllocate(heapId, 0x4800, FALSE, "mystery.c", 1154);
    particle->sys = Particle_CreateSystem(particle->work, 0x4800, TRUE, heapId);
}

static void MysteryParticle_Exit(MysteryParticle *particle) {
    GFL_HeapFree(particle->work);
    MI_CpuFill8(particle, 0, sizeof(MysteryParticle));
}

static void MysteryParticle_Load(MysteryParticle *particle, u32 arcId, u32 fileId, HeapID heapId) {
    Particle_SetResource(particle->sys, Particle_ReadResource(arcId, fileId, heapId), TRUE, NULL);
}

static void MysteryParticle_Emit(MysteryParticle *particle, int emitter) {
    Particle_CreateEmitterWithCallback(particle->sys, emitter, NULL, particle);
}

static BOOL MysteryParticle_IsActive(MysteryParticle *particle) {
    return particle->active;
}

static void MysterySeq_Start(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;

    MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0, MYSTERY_PRINT_QUEUE);
    Mystery_CreateList(wk, LIST_TOP, HEAPID_MYSTERY);
    MysterySeq_SetNext(seq, MysterySeq_FadeIn);
}

static void MysterySeq_FadeIn(MysterySeq *seq, u32 *state, void *work) {
    switch (*state) {
    case FADE_IN_STATE_START:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *state = FADE_IN_STATE_WAIT;
        break;
    case FADE_IN_STATE_WAIT:
        if (!GFL_FadeIsRunning()) {
            *state = FADE_IN_STATE_FINISH;
        }
        break;
    case FADE_IN_STATE_FINISH:
        MysterySeq_SetNext(seq, MysterySeq_Top);
        break;
    }
}

static void MysterySeq_FadeOut(MysterySeq *seq, u32 *state, void *work) {
    switch (*state) {
    case FADE_OUT_STATE_START:
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 0, 16, 0);
        GFL_SndBGMFadeOut(8);
        *state = FADE_OUT_STATE_WAIT;
        break;
    case FADE_OUT_STATE_WAIT:
        if (!GFL_FadeIsRunning()) {
            *state = FADE_OUT_STATE_FINISH;
        }
        break;
    case FADE_OUT_STATE_FINISH:
        MysterySeq_SetNext(seq, MysterySeq_Exit);
        break;
    }
}

static void MysterySeq_Top(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;
    u32 ret;

    switch (*state) {
    case TOP_STATE_SHOW_MENU:
        if (wk->resumeReceive) {
            wk->resumeReceive = FALSE;
            *state = TOP_STATE_SHOW_RECEIVE_PROMPT;
            break;
        }
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0, MYSTERY_PRINT_QUEUE);
        if (wk->list == NULL) {
            Mystery_CreateList(wk, LIST_TOP, HEAPID_MYSTERY);
        }
        *state = TOP_STATE_WAIT_MENU;
        break;
    case TOP_STATE_WAIT_MENU:
        ret = Mystery_UpdateList(wk);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        if (ret == TOP_CHOICE_RECEIVE) {
            // With the album full, a card can only be thrown away to make room once its gift was picked up
            BOOL noneDelivered = TRUE;
            BOOL full;
            u32 i;

            Mystery_DeleteList(wk);
            full = MysteryGiftSave_HasFreeSlot(wk->giftSave) == FALSE ? TRUE : FALSE;
            if (full) {
                for (i = 0; i < MysteryGiftSave_GetCardMax(wk->giftSave); i++) {
                    if (MysteryGiftSave_IsCardDelivered(wk->giftSave, i)) {
                        noneDelivered = FALSE;
                    }
                }
            }
            if (full) {
                if (noneDelivered) {
                    MysteryMsgWin_Print(wk->msgWin, wk->msgData,
                                        MysteryGiftSave_GetKind(wk->giftSave) == MG_DATA_OUTSIDE ? 0x43 : 0x3a,
                                        MYSTERY_PRINT_STREAM);
                    MysterySeq_SetReturn(seq, TOP_STATE_SHOW_MENU);
                    *state = TOP_STATE_WAIT_MESSAGE;
                } else {
                    *state = TOP_STATE_SHOW_FULL_PROMPT;
                }
            } else if (!isWirelessEnabled()) {
                MysterySeq_SetNext(seq, MysterySeq_WirelessOff);
            } else {
                *state = TOP_STATE_SHOW_RECEIVE_PROMPT;
            }
        } else if (ret == TOP_CHOICE_ALBUM) {
            Mystery_DeleteList(wk);
            MysterySeq_SetNext(seq, MysterySeq_Album);
        } else if (ret == TOP_CHOICE_INFO) {
            Mystery_DeleteList(wk);
            MysterySeq_SetNext(seq, MysterySeq_About);
        } else if (ret == TOP_CHOICE_QUIT) {
            MysterySeq_SetNext(seq, MysterySeq_FadeOut);
        }
        break;
    case TOP_STATE_SHOW_RECEIVE_PROMPT:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 4, MYSTERY_PRINT_STREAM);
        MysterySeq_SetReturn(seq, TOP_STATE_OPEN_RECEIVE_CONFIRM);
        *state = TOP_STATE_WAIT_MESSAGE;
        break;
    case TOP_STATE_OPEN_RECEIVE_CONFIRM:
        Mystery_CreateYesNo(wk, 0, HEAPID_MYSTERY);
        *state = TOP_STATE_WAIT_RECEIVE_CONFIRM;
        break;
    case TOP_STATE_WAIT_RECEIVE_CONFIRM:
        ret = MysteryYesNo_Update(wk->yesNo);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteYesNo(wk);
        if (ret == MYSTERY_CONFIRM_YES) {
            *state = TOP_STATE_START_WIRELESS;
        } else if (ret == MYSTERY_CONFIRM_NO) {
            *state = TOP_STATE_SHOW_MENU;
        }
        break;
    case TOP_STATE_START_WIRELESS:
        MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_WIRELESS_START);
        *state = TOP_STATE_WAIT_WIRELESS;
        break;
    case TOP_STATE_WAIT_WIRELESS:
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_WIRELESS_READY) {
            *state = TOP_STATE_SHOW_METHOD_MENU;
        }
        break;
    case TOP_STATE_SHOW_METHOD_MENU:
        Mystery_DeleteList(wk);
        Mystery_CreateList(wk, LIST_RECEIVE, HEAPID_MYSTERY);
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 7, MYSTERY_PRINT_QUEUE);
        *state = TOP_STATE_WAIT_METHOD_MENU;
        break;
    case TOP_STATE_WAIT_METHOD_MENU:
        if (MysteryNet_GetBeaconFlags(wk->net) & 0x20) {
            MysteryList_SetItemGrayed(wk->list, 0, TRUE);
        } else {
            MysteryList_SetItemGrayed(wk->list, 0, FALSE);
        }
        ret = Mystery_UpdateList(wk);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteList(wk);
        switch (ret) {
        case RECEIVE_CHOICE_WIRELESS:
            wk->receiveMode = RECEIVE_WIRELESS;
            *state = TOP_STATE_SHOW_METHOD_PROMPT;
            break;
        case RECEIVE_CHOICE_WIFI:
            wk->receiveMode = RECEIVE_WIFI;
            *state = TOP_STATE_STOP_WIRELESS;
            break;
        case RECEIVE_CHOICE_INFRARED:
            wk->receiveMode = RECEIVE_INFRARED;
            *state = TOP_STATE_SHOW_METHOD_PROMPT;
            break;
        case RECEIVE_CHOICE_BACK:
            *state = TOP_STATE_STOP_CANCELLED_WIRELESS;
            break;
        }
        break;
    case TOP_STATE_SHOW_METHOD_PROMPT:
        switch (wk->receiveMode) {
        case RECEIVE_WIRELESS:
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x11, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, TOP_STATE_OPEN_METHOD_CONFIRM);
            *state = TOP_STATE_WAIT_MESSAGE;
            break;
        case RECEIVE_WIFI:
            break;
        case RECEIVE_INFRARED:
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x17, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, TOP_STATE_OPEN_METHOD_CONFIRM);
            *state = TOP_STATE_WAIT_MESSAGE;
            break;
        }
        break;
    case TOP_STATE_OPEN_METHOD_CONFIRM:
        Mystery_CreateYesNo(wk, 0, HEAPID_MYSTERY);
        *state = TOP_STATE_WAIT_METHOD_CONFIRM;
        break;
    case TOP_STATE_WAIT_METHOD_CONFIRM:
        ret = MysteryYesNo_Update(wk->yesNo);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteYesNo(wk);
        if (ret == MYSTERY_CONFIRM_YES) {
            *state = TOP_STATE_STOP_WIRELESS;
        } else if (ret == MYSTERY_CONFIRM_NO) {
            *state = TOP_STATE_SHOW_METHOD_MENU;
        }
        break;
    case TOP_STATE_STOP_WIRELESS:
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_WIRELESS_READY) {
            MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_WIRELESS_END);
            *state = TOP_STATE_WAIT_WIRELESS_STOP;
        }
        break;
    case TOP_STATE_WAIT_WIRELESS_STOP:
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_IDLE) {
            if (wk->receiveMode == RECEIVE_WIFI) {
                MysterySeq_SetNext(seq, MysterySeq_WifiLogin);
            } else {
                MysterySeq_SetNext(seq, MysterySeq_Receive);
            }
        }
        break;
    case TOP_STATE_STOP_CANCELLED_WIRELESS:
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_WIRELESS_READY) {
            MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_WIRELESS_END);
            *state = TOP_STATE_WAIT_CANCELLED_STOP;
        } else {
            *state = TOP_STATE_SHOW_MENU;
        }
        break;
    case TOP_STATE_WAIT_CANCELLED_STOP:
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_IDLE) {
            *state = TOP_STATE_SHOW_MENU;
        }
        break;
    case TOP_STATE_SHOW_FULL_PROMPT:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x3e, MYSTERY_PRINT_STREAM);
        MysterySeq_SetReturn(seq, TOP_STATE_OPEN_FULL_CONFIRM);
        *state = TOP_STATE_WAIT_MESSAGE;
        break;
    case TOP_STATE_OPEN_FULL_CONFIRM:
        Mystery_CreateYesNo(wk, 0, HEAPID_MYSTERY);
        *state = TOP_STATE_WAIT_FULL_CONFIRM;
        break;
    case TOP_STATE_WAIT_FULL_CONFIRM:
        ret = MysteryYesNo_Update(wk->yesNo);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteYesNo(wk);
        if (ret == MYSTERY_CONFIRM_YES) {
            MysterySeq_SetNext(seq, MysterySeq_AlbumFull);
        } else if (ret == MYSTERY_CONFIRM_NO) {
            *state = TOP_STATE_SHOW_MENU;
        }
        break;
    case TOP_STATE_WAIT_MESSAGE:
        if (MysteryMsgWin_IsDone(wk->msgWin)) {
            MysterySeq_Return(seq);
        }
        break;
    }
}

static void MysterySeq_About(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;
    u32 ret;

    switch (*state) {
    case ABOUT_STATE_SHOW_MENU:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x2f, MYSTERY_PRINT_QUEUE);
        Mystery_CreateList(wk, LIST_ABOUT, HEAPID_MYSTERY);
        *state = ABOUT_STATE_WAIT_MENU;
        break;
    case ABOUT_STATE_WAIT_MENU:
        ret = Mystery_UpdateList(wk);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteList(wk);
        if (ret == ABOUT_CHOICE_GIFT) {
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x35, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, ABOUT_STATE_SHOW_MENU);
            *state = ABOUT_STATE_WAIT_MESSAGE;
        } else if (ret == ABOUT_CHOICE_ALBUM) {
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x36, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, ABOUT_STATE_SHOW_MENU);
            *state = ABOUT_STATE_WAIT_MESSAGE;
        } else if (ret == ABOUT_CHOICE_FULL_ALBUM) {
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x37, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, ABOUT_STATE_SHOW_MENU);
            *state = ABOUT_STATE_WAIT_MESSAGE;
        } else if (ret == ABOUT_CHOICE_BACK) {
            MysterySeq_SetNext(seq, MysterySeq_Top);
        }
        break;
    case ABOUT_STATE_WAIT_MESSAGE:
        if (MysteryMsgWin_IsDone(wk->msgWin)) {
            MysterySeq_Return(seq);
        }
        break;
    }
}

static void MysterySeq_Receive(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;
    u32 ret;
    BOOL cancel;
    RTCDate date;

    switch (*state) {
    case RECEIVE_STATE_START:
        switch (wk->receiveMode) {
        case RECEIVE_WIFI:
            MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_WIFI);
            break;
        case RECEIVE_WIRELESS:
            MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_BEACON_START);
            break;
        case RECEIVE_INFRARED:
            MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_IRC_START);
            break;
        }
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x1a, MYSTERY_PRINT_QUEUE);
        wk->wait = 0;
        *state = RECEIVE_STATE_WAIT_DATA;
        break;
    case RECEIVE_STATE_WAIT_DATA:
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_IDLE) {
            ret = MysteryNet_GetRecvData(wk->net, &wk->recv, sizeof(wk->recv));
            if (ret == MYSTERY_NET_RECV_OK) {
                if (Mystery_IsRecvDataValid(
                        &wk->recv, MysteryCheck_CountErrors(&wk->recv, wk->gameData, HEAPID_TAIL(HEAPID_MYSTERY)))) {
                    *state = RECEIVE_STATE_SHOW_CARD;
                } else {
                    *state = RECEIVE_STATE_SHOW_INVALID;
                }
            } else if (ret == MYSTERY_NET_RECV_ERROR) {
                *state = RECEIVE_STATE_SHOW_STOPPING;
            }
        }
        switch (MysteryNet_GetError(wk->net)) {
        case MYSTERY_NET_ERROR_FATAL:
        case MYSTERY_NET_ERROR_DISCONNECT:
            MysteryNet_ClearError(wk->net);
            MysterySeq_SetNext(seq, MysterySeq_Top);
            break;
        }
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) || wk->wait++ > 7200) {
            cancel = FALSE;
            switch (wk->receiveMode) {
            case RECEIVE_WIFI:
                if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_WIFI) {
                    MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_WIFI_CANCEL);
                    cancel = TRUE;
                }
                break;
            case RECEIVE_WIRELESS:
                if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_BEACON_WAIT) {
                    MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_BEACON_END);
                    cancel = TRUE;
                }
                break;
            case RECEIVE_INFRARED:
                if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_IRC_WAIT) {
                    MysteryNet_ChangeState(wk->net, MYSTERY_NET_STATE_IRC_END);
                    cancel = TRUE;
                }
                break;
            }
            if (cancel) {
                *state = RECEIVE_STATE_SHOW_STOPPING;
                wk->wait = 0;
            }
        }
        break;
    case RECEIVE_STATE_SHOW_STOPPING:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x39, MYSTERY_PRINT_WAIT_ICON);
        MysterySeq_SetReturn(seq, RECEIVE_STATE_WAIT_STOPPING);
        *state = RECEIVE_STATE_WAIT_MESSAGE;
        break;
    case RECEIVE_STATE_WAIT_STOPPING:
        wk->wait++;
        if (MysteryNet_GetState(wk->net) == MYSTERY_NET_STATE_IDLE && wk->wait > 30) {
            wk->wait = 0;
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x3c, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, RECEIVE_STATE_RETURN_TOP);
            *state = RECEIVE_STATE_WAIT_MESSAGE;
        }
        break;
    case RECEIVE_STATE_RETURN_TOP:
        MysterySeq_SetNext(seq, MysterySeq_Top);
        break;
    case RECEIVE_STATE_SHOW_CARD:
        Mystery_CreateList(wk, LIST_RECEIVED, HEAPID_MYSTERY);
        Mystery_CreateCardWin(wk, HEAPID_MYSTERY);
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x1b, MYSTERY_PRINT_QUEUE);
        *state = RECEIVE_STATE_WAIT_CARD;
        break;
    case RECEIVE_STATE_WAIT_CARD:
        cancel = FALSE;
        ret = Mystery_UpdateList(wk);
        if (ret != MYSTERY_MENU_NONE) {
            if (ret == RECEIVED_CHOICE_CARD) {
                MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x1f, MYSTERY_PRINT_STREAM);
                MysterySeq_SetReturn(seq, RECEIVE_STATE_OPEN_ACCEPT_CONFIRM);
                *state = RECEIVE_STATE_WAIT_MESSAGE;
            } else if (ret == MYSTERY_MENU_CANCEL) {
                cancel = TRUE;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            cancel = TRUE;
        }
        if (cancel) {
            MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x1c, MYSTERY_PRINT_STREAM);
            MysterySeq_SetReturn(seq, RECEIVE_STATE_OPEN_CANCEL_CONFIRM);
            *state = RECEIVE_STATE_WAIT_MESSAGE;
        }
        break;
    case RECEIVE_STATE_OPEN_ACCEPT_CONFIRM:
        Mystery_CreateYesNo(wk, 0, HEAPID_MYSTERY);
        *state = RECEIVE_STATE_WAIT_ACCEPT_CONFIRM;
        break;
    case RECEIVE_STATE_WAIT_ACCEPT_CONFIRM:
        ret = MysteryYesNo_Update(wk->yesNo);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteYesNo(wk);
        if (ret == MYSTERY_CONFIRM_YES) {
            Mystery_DeleteList(wk);
            if (wk->recv.gift.once && MysteryGiftSave_IsIdReceived(wk->giftSave, wk->recv.gift.id)) {
                *state = RECEIVE_STATE_SHOW_ALREADY_RECEIVED;
                break;
            }
            RTC_GetCachedDate(&date);
            wk->recv.gift.date = ((date.year + 2000) << 16) | ((date.month & 0xff) << 8) | (date.day & 0xff);
            MysteryGiftSave_AddCard(wk->giftSave, &wk->recv.gift);
            MysteryGiftSave_SetIdReceived(wk->giftSave, wk->recv.gift.id);
            MysterySeq_SetNext(seq, MysterySeq_Received);
        } else if (ret == MYSTERY_CONFIRM_NO) {
            *state = RECEIVE_STATE_SHOW_CARD;
        }
        break;
    case RECEIVE_STATE_OPEN_CANCEL_CONFIRM:
        Mystery_CreateYesNo(wk, 0, HEAPID_MYSTERY);
        *state = RECEIVE_STATE_WAIT_CANCEL_CONFIRM;
        break;
    case RECEIVE_STATE_WAIT_CANCEL_CONFIRM:
        ret = MysteryYesNo_Update(wk->yesNo);
        if (ret == MYSTERY_MENU_NONE) {
            break;
        }
        Mystery_DeleteYesNo(wk);
        if (ret == MYSTERY_CONFIRM_YES) {
            Mystery_DeleteList(wk);
            Mystery_DeleteCardWin(wk);
            MysterySeq_SetNext(seq, MysterySeq_Top);
        } else if (ret == MYSTERY_CONFIRM_NO) {
            *state = RECEIVE_STATE_SHOW_CARD;
        }
        break;
    case RECEIVE_STATE_SHOW_ALREADY_RECEIVED:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x3b, MYSTERY_PRINT_STREAM);
        MysterySeq_SetReturn(seq, RECEIVE_STATE_CLEANUP);
        *state = RECEIVE_STATE_WAIT_MESSAGE;
        break;
    case RECEIVE_STATE_SHOW_INVALID:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x41, MYSTERY_PRINT_STREAM);
        MysterySeq_SetReturn(seq, RECEIVE_STATE_CLEANUP);
        *state = RECEIVE_STATE_WAIT_MESSAGE;
        break;
    case RECEIVE_STATE_CLEANUP:
        Mystery_DeleteList(wk);
        Mystery_DeleteCardWin(wk);
        MysterySeq_SetNext(seq, MysterySeq_Top);
        break;
    case RECEIVE_STATE_WAIT_MESSAGE:
        if (MysteryMsgWin_IsDone(wk->msgWin)) {
            MysterySeq_Return(seq);
        }
        break;
    }
}

static void MysterySeq_Received(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;

    switch (*state) {
    case RECEIVED_STATE_START_EFFECT:
        MysteryGraphic_Start3D(wk->graphic);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(3, TRUE);
        MysteryEffect_Init(&wk->effect, MysteryGraphic_GetClactUnit(wk->graphic), &wk->recv, wk->gameData, &wk->actors,
                           &wk->bg, &wk->scroll, HEAPID_MYSTERY);
        MysteryEffect_Start(&wk->effect, MYSTERY_EFFECT_FALL);
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x22, MYSTERY_PRINT_WAIT_ICON);
        *state = RECEIVED_STATE_ENABLE_EFFECT_BG;
        break;
    case RECEIVED_STATE_ENABLE_EFFECT_BG:
        GFL_BGSysSetBGEnabled(0, TRUE);
        *state = RECEIVED_STATE_WAIT_MESSAGE;
        break;
    case RECEIVED_STATE_WAIT_MESSAGE:
        if (MysteryMsgWin_IsDone(wk->msgWin)) {
            *state = RECEIVED_STATE_RUN_EFFECT;
        }
        break;
    case RECEIVED_STATE_RUN_EFFECT:
        MysteryEffect_Main(&wk->effect);
        if (MysteryEffect_IsEnd(&wk->effect)) {
            *state = RECEIVED_STATE_START_SAVE;
        }
        break;
    case RECEIVED_STATE_START_SAVE:
        MysteryGiftSave_StartSave(wk->giftSave, wk->gameData);
        *state = RECEIVED_STATE_WAIT_SAVE;
        break;
    case RECEIVED_STATE_WAIT_SAVE:
        if (MysteryGiftSave_SaveStep(wk->giftSave, wk->gameData) == SAVE_RESULT_OK) {
            *state = RECEIVED_STATE_START_FADE_BACK;
        }
        break;
    case RECEIVED_STATE_START_FADE_BACK:
        MysteryEffect_Start(&wk->effect, MYSTERY_EFFECT_FADE_BACK);
        *state = RECEIVED_STATE_RUN_FADE_BACK;
        break;
    case RECEIVED_STATE_RUN_FADE_BACK:
        MysteryEffect_Main(&wk->effect);
        if (MysteryEffect_IsEnd(&wk->effect)) {
            *state = RECEIVED_STATE_SHOW_SAVED;
        }
        break;
    case RECEIVED_STATE_SHOW_SAVED:
        MysteryMsgWin_Print(wk->msgWin, wk->msgData, 0x23, MYSTERY_PRINT_STREAM);
        *state = RECEIVED_STATE_WAIT_SAVED;
        break;
    case RECEIVED_STATE_WAIT_SAVED:
        if (MysteryMsgWin_IsDone(wk->msgWin)) {
            *state = RECEIVED_STATE_CLEANUP_EFFECT;
        }
        break;
    case RECEIVED_STATE_CLEANUP_EFFECT:
        MysteryEffect_Exit(&wk->effect);
        MysteryMsgWin_Delete(wk->msgWin);
        wk->msgWin = NULL;
        Mystery_DeleteList(wk);
        G2_BlendNone();
        MysteryGraphic_End3D(wk->graphic);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(3, TRUE);
        *state = RECEIVED_STATE_RESTORE_MENU_BG;
        break;
    case RECEIVED_STATE_RESTORE_MENU_BG:
        GFL_BGSysSetBGEnabled(0, TRUE);
        *state = RECEIVED_STATE_CREATE_ALBUM;
        break;
    case RECEIVED_STATE_CREATE_ALBUM: {
        MysteryCardResSetup setup;

        GFL_BGSysSetBGEnabled(1, FALSE);
        MI_CpuFill8(&setup, 0, sizeof(setup));
        setup.bg = 2;
        setup.textBg = 0;
        setup.bgPalette = 8;
        setup.textPalette = 15;
        setup.iconPalette = 10;
        setup.pokePalette = 13;
        setup.unit = MysteryGraphic_GetClactUnit(wk->graphic);
        setup.giftSave = wk->giftSave;
        setup.msgData = wk->msgData;
        setup.font = wk->font;
        setup.queue = wk->queue;
        setup.wordSet = wk->wordSet;
        wk->cardRes = MysteryCardRes_Create(&setup, HEAPID_MYSTERY);
        wk->album = MysteryAlbum_CreateReceived(&wk->recv.gift, wk->cardRes, wk->gameData, HEAPID_MYSTERY);
        MysteryAlbum_SetVisible(wk->album, FALSE);
        *state = RECEIVED_STATE_OPEN_ALBUM;
        break;
    }
    case RECEIVED_STATE_OPEN_ALBUM:
        MysteryAlbum_Main(wk->album);
        MysteryAlbum_StartOpen(wk->album);
        *state = RECEIVED_STATE_WAIT_ALBUM;
        break;
    case RECEIVED_STATE_WAIT_ALBUM:
        MysteryAlbum_Main(wk->album);
        if (MysteryAlbum_IsOpened(wk->album)) {
            wk->wait = 0;
            *state = RECEIVED_STATE_WAIT_KEY;
        }
        break;
    case RECEIVED_STATE_WAIT_KEY:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            *state = RECEIVED_STATE_FINISH;
        }
        if (wk->wait++ > 240) {
            wk->wait = 0;
            *state = RECEIVED_STATE_CREATE_KEY_HINT;
        }
        break;
    case RECEIVED_STATE_CREATE_KEY_HINT:
        MysteryOamMsg_Init(&wk->oamMsg, &wk->actors, MysteryGraphic_GetClactUnit(wk->graphic), wk->queue, wk->msgData,
                           0x4c, wk->font, HEAPID_MYSTERY);
        MysteryOamMsg_Update(&wk->oamMsg);
        *state = RECEIVED_STATE_WAIT_KEY_HINT;
        break;
    case RECEIVED_STATE_WAIT_KEY_HINT:
        MysteryOamMsg_Update(&wk->oamMsg);
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            MysteryOamMsg_Exit(&wk->oamMsg);
            *state = RECEIVED_STATE_FINISH;
        }
        break;
    case RECEIVED_STATE_FINISH:
        MysterySeq_SetNext(seq, MysterySeq_FadeOut);
        break;
    }
}

static void MysterySeq_Album(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;
    MysteryCardViewSetup setup;

    switch (*state) {
    case ALBUM_SEQ_STATE_FADE_OUT:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, -1);
        MysterySeq_SetReturn(seq, ALBUM_SEQ_STATE_CREATE_VIEW);
        *state = ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case ALBUM_SEQ_STATE_CREATE_VIEW:
        MysteryBgScroll_SetEnabled(&wk->scroll, FALSE);
        Mystery_DeleteList(wk);
        MysteryMsgWin_Delete(wk->msgWin);
        wk->msgWin = NULL;
        MI_CpuFill8(&setup, 0, sizeof(setup));
        setup.mode = MYSTERY_CARD_VIEW_MODE_ALBUM;
        setup.unit = MysteryGraphic_GetClactUnit(wk->graphic);
        setup.giftSave = wk->giftSave;
        setup.font = wk->font;
        setup.queue = wk->queue;
        setup.wordSet = wk->wordSet;
        setup.msgData = wk->msgData;
        setup.gameData = wk->gameData;
        wk->cardView = MysteryCardView_Create(&setup, HEAPID_MYSTERY);
        *state = ALBUM_SEQ_STATE_FADE_IN;
        break;
    case ALBUM_SEQ_STATE_FADE_IN:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, -1);
        MysterySeq_SetReturn(seq, ALBUM_SEQ_STATE_RUN_VIEW);
        *state = ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case ALBUM_SEQ_STATE_RUN_VIEW:
        MysteryCardView_Main(wk->cardView);
        if (MysteryCardView_IsEnd(wk->cardView)) {
            *state = ALBUM_SEQ_STATE_FADE_OUT_VIEW;
        }
        break;
    case ALBUM_SEQ_STATE_FADE_OUT_VIEW:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        MysterySeq_SetReturn(seq, ALBUM_SEQ_STATE_RESTORE_MENU);
        *state = ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case ALBUM_SEQ_STATE_RESTORE_MENU:
        MysteryBgScroll_SetEnabled(&wk->scroll, TRUE);
        MysteryCardView_Delete(wk->cardView);
        wk->cardView = NULL;
        MysteryBg_Load(&wk->bg, MYSTERY_BG_MENU);
        MysteryActors_LoadPalette(&wk->actors, HEAPID_MYSTERY);
        wk->msgWin = MysteryMsgWin_Create(1, 15, wk->queue, wk->font, HEAPID_MYSTERY);
        MysteryMsgWin_DrawFrame(wk->msgWin, 10, 14);
        Mystery_CreateList(wk, LIST_TOP, HEAPID_MYSTERY);
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 4, 0x18, 15, 3);
        *state = ALBUM_SEQ_STATE_FADE_IN_MENU;
        break;
    case ALBUM_SEQ_STATE_FADE_IN_MENU:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        MysterySeq_SetReturn(seq, ALBUM_SEQ_STATE_RETURN_TOP);
        *state = ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case ALBUM_SEQ_STATE_RETURN_TOP:
        MysterySeq_SetNext(seq, MysterySeq_Top);
        break;
    case ALBUM_SEQ_STATE_WAIT_FADE:
        if (!GFL_FadeIsRunning()) {
            MysterySeq_Return(seq);
        }
        break;
    }
    if (wk->cardView != NULL) {
        MysteryCardView_Draw(wk->cardView);
    }
}

static void MysterySeq_AlbumFull(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;
    MysteryCardViewSetup setup;

    switch (*state) {
    case FULL_ALBUM_SEQ_STATE_FADE_OUT:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, -1);
        MysterySeq_SetReturn(seq, FULL_ALBUM_SEQ_STATE_CREATE_VIEW);
        *state = FULL_ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case FULL_ALBUM_SEQ_STATE_CREATE_VIEW:
        MysteryBgScroll_SetEnabled(&wk->scroll, FALSE);
        Mystery_DeleteList(wk);
        MysteryMsgWin_Delete(wk->msgWin);
        wk->msgWin = NULL;
        MI_CpuFill8(&setup, 0, sizeof(setup));
        setup.mode = MYSTERY_CARD_VIEW_MODE_MAKE_ROOM;
        setup.unit = MysteryGraphic_GetClactUnit(wk->graphic);
        setup.giftSave = wk->giftSave;
        setup.font = wk->font;
        setup.queue = wk->queue;
        setup.wordSet = wk->wordSet;
        setup.msgData = wk->msgData;
        setup.gameData = wk->gameData;
        wk->cardView = MysteryCardView_Create(&setup, HEAPID_MYSTERY);
        *state = FULL_ALBUM_SEQ_STATE_FADE_IN;
        break;
    case FULL_ALBUM_SEQ_STATE_FADE_IN:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, -1);
        MysterySeq_SetReturn(seq, FULL_ALBUM_SEQ_STATE_RUN_VIEW);
        *state = FULL_ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case FULL_ALBUM_SEQ_STATE_RUN_VIEW:
        MysteryCardView_Main(wk->cardView);
        if (MysteryCardView_IsEnd(wk->cardView)) {
            *state = FULL_ALBUM_SEQ_STATE_FADE_OUT_VIEW;
        }
        break;
    case FULL_ALBUM_SEQ_STATE_FADE_OUT_VIEW:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        MysterySeq_SetReturn(seq, FULL_ALBUM_SEQ_STATE_RESTORE_MENU);
        *state = FULL_ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case FULL_ALBUM_SEQ_STATE_RESTORE_MENU:
        MysteryBgScroll_SetEnabled(&wk->scroll, TRUE);
        MysteryCardView_Delete(wk->cardView);
        wk->cardView = NULL;
        MysteryBg_Load(&wk->bg, MYSTERY_BG_MENU);
        MysteryActors_LoadPalette(&wk->actors, HEAPID_MYSTERY);
        wk->msgWin = MysteryMsgWin_Create(1, 15, wk->queue, wk->font, HEAPID_MYSTERY);
        MysteryMsgWin_DrawFrame(wk->msgWin, 10, 14);
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 4, 0x18, 15, 3);
        *state = FULL_ALBUM_SEQ_STATE_FADE_IN_MENU;
        break;
    case FULL_ALBUM_SEQ_STATE_FADE_IN_MENU:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        MysterySeq_SetReturn(seq, FULL_ALBUM_SEQ_STATE_RESUME_RECEIVE);
        *state = FULL_ALBUM_SEQ_STATE_WAIT_FADE;
        break;
    case FULL_ALBUM_SEQ_STATE_RESUME_RECEIVE:
        wk->resumeReceive = MysteryGiftSave_HasFreeSlot(wk->giftSave);
        MysterySeq_SetNext(seq, MysterySeq_Top);
        break;
    case FULL_ALBUM_SEQ_STATE_WAIT_FADE:
        if (!GFL_FadeIsRunning()) {
            MysterySeq_Return(seq);
        }
        break;
    }
    if (wk->cardView != NULL) {
        MysteryCardView_Draw(wk->cardView);
    }
}

static void MysterySeq_WifiLogin(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;

    switch (MysteryNet_GetError(wk->net)) {
    case MYSTERY_NET_ERROR_FATAL:
    case MYSTERY_NET_ERROR_DISCONNECT:
        MysteryNet_ClearError(wk->net);
        MysterySeq_SetNext(seq, MysterySeq_Top);
        break;
    }
    switch (*state) {
    case WIFI_LOGIN_STATE_FADE_OUT:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        *state = WIFI_LOGIN_STATE_WAIT_FADE_OUT;
        break;
    case WIFI_LOGIN_STATE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            MysteryBgScroll_SetEnabled(&wk->scroll, FALSE);
            *state = WIFI_LOGIN_STATE_FREE_GRAPHICS;
        }
        break;
    case WIFI_LOGIN_STATE_FREE_GRAPHICS:
        if (wk->msgWin != NULL) {
            MysteryMsgWin_Delete(wk->msgWin);
            wk->msgWin = NULL;
        }
        MysteryActors_Exit(&wk->actors);
        MysteryBg_Exit(&wk->bg);
        MysteryGraphic_Delete(wk->graphic);
        wk->graphic = NULL;
        *state = WIFI_LOGIN_STATE_QUEUE_LOGIN;
        break;
    case WIFI_LOGIN_STATE_QUEUE_LOGIN:
        wk->loginParam = GFL_HeapAllocate(HEAPID_MYSTERY, sizeof(WifiLoginParam), FALSE, "mystery.c", 2666);
        MI_CpuFill8(wk->loginParam, 0, sizeof(WifiLoginParam));
        wk->loginParam->gameData = wk->gameData;
        wk->loginParam->displayStyle = WIFILOGIN_DISP_STYLE_PDWACC;
        wk->loginParam->messageStyle = WIFILOGIN_MESSAGE_STYLE_GSYNC;
        wk->loginParam->buffer = NULL;
        wk->loginParam->keepBgm = 0;
        wk->loginParam->serviceId = WIFILOGIN_SERVICE_MYSTERY_GIFT;
        if (MysteryGiftSave_GetKind(wk->giftSave) == MG_DATA_OUTSIDE) {
            wk->loginParam->mode = WIFILOGIN_MODE_NO_SAVE;
        } else {
            wk->loginParam->mode = WIFILOGIN_MODE_NORMAL;
        }
        GCTX_ProcMgrQueueProc(OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, wk->loginParam);
        *state = WIFI_LOGIN_STATE_CREATE_GRAPHICS;
        break;
    case WIFI_LOGIN_STATE_CREATE_GRAPHICS:
        wk->graphic = MysteryGraphic_Create(1, HEAPID_MYSTERY);
        MysteryBg_Init(&wk->bg, HEAPID_MYSTERY);
        MysteryActors_Init(&wk->actors, MysteryGraphic_GetClactUnit(wk->graphic), HEAPID_MYSTERY);
        wk->msgWin = MysteryMsgWin_Create(1, 15, wk->queue, wk->font, HEAPID_MYSTERY);
        MysteryMsgWin_DrawFrame(wk->msgWin, 10, 14);
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 4, 0x18, 15, 3);
        GFL_NetSetIconScreen(TRUE, HEAPID_MYSTERY);
        GFL_SndBGMPlay(SEQ_BGM_WIFI_PRESENT, 0xffff);
        MysteryBgScroll_SetEnabled(&wk->scroll, TRUE);
        *state = WIFI_LOGIN_STATE_RESUME;
        break;
    case WIFI_LOGIN_STATE_RESUME:
        *state = WIFI_LOGIN_STATE_FADE_IN;
        break;
    case WIFI_LOGIN_STATE_FADE_IN:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *state = WIFI_LOGIN_STATE_WAIT_FADE_IN;
        break;
    case WIFI_LOGIN_STATE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            *state = WIFI_LOGIN_STATE_CHECK_RESULT;
        }
        break;
    case WIFI_LOGIN_STATE_CHECK_RESULT:
        if (wk->loginParam->result == WIFILOGIN_RESULT_OK) {
            MysterySeq_SetNext(seq, MysterySeq_Receive);
        } else {
            MysterySeq_SetNext(seq, MysterySeq_Top);
        }
        GFL_HeapFree(wk->loginParam);
        break;
    }
}

static void MysterySeq_Exit(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;

    Mystery_DeleteYesNo(wk);
    Mystery_DeleteList(wk);
    Mystery_DeleteCardWin(wk);
    if (wk->album != NULL) {
        MysteryAlbum_Delete(wk->album);
        MysteryCardRes_Delete(wk->cardRes);
    }
    MysterySeq_End(seq);
}

static void MysterySeq_WirelessOff(MysterySeq *seq, u32 *state, void *work) {
    MysteryWork *wk = work;

    switch (*state) {
    case WIRELESS_OFF_STATE_SHOW_MESSAGE:
        MysteryMsgWin_Print(wk->msgWin, wk->scriptMsgData, 0x16, MYSTERY_PRINT_STREAM);
        *state = WIRELESS_OFF_STATE_WAIT_MESSAGE;
        break;
    case WIRELESS_OFF_STATE_WAIT_MESSAGE:
        if (MysteryMsgWin_IsDone(wk->msgWin)) {
            MysterySeq_SetNext(seq, MysterySeq_Top);
        }
        break;
    }
}

static void Mystery_CreateYesNo(MysteryWork *wk, u32 unused, HeapID heapId) {
    MysteryYesNoSetup setup;

    if (wk->yesNo == NULL) {
        MI_CpuFill8(&setup, 0, sizeof(setup));
        setup.msgData = wk->msgData;
        setup.font = wk->font;
        setup.queue = wk->queue;
        setup.msgIds[MYSTERY_CONFIRM_YES] = 5;
        setup.msgIds[MYSTERY_CONFIRM_NO] = 6;
        setup.count = MYSTERY_CONFIRM_MAX;
        setup.bg = 0;
        setup.palette = 15;
        setup.framePalette = 14;
        setup.frameChar = 1;
        wk->yesNo = MysteryYesNo_Create(&setup, heapId);
    }
}

static void Mystery_DeleteYesNo(MysteryWork *wk) {
    if (wk->yesNo != NULL) {
        MysteryYesNo_Delete(wk->yesNo);
        wk->yesNo = NULL;
    }
}

static void Mystery_CreateList(MysteryWork *wk, u32 type, HeapID heapId) {
    MysteryListSetup setup;
    ArcTool *arc;
    int i;

    if (wk->list == NULL) {
        MI_CpuFill8(&setup, 0, sizeof(setup));
        setup.font = wk->font;
        setup.queue = wk->queue;
        setup.cursor = MysteryActors_GetActor(&wk->actors, ACTOR_CURSOR);
        setup.bg = 1;
        setup.palette = 15;
        setup.unk28 = 2;
        setup.bgPalette = 4;
        setup.onMove = Mystery_OnListMove;
        setup.work = wk;
        setup.offsetY = 0;
        setup.offsetX = 0;
        setup.cursorSequence = 0;
        setup.cursorPos = &wk->listCursors[type];
        switch (type) {
        case LIST_TOP:
            setup.msgData = wk->msgData;
            setup.items[TOP_CHOICE_RECEIVE] = 1;
            setup.items[TOP_CHOICE_ALBUM] = 2;
            setup.items[TOP_CHOICE_INFO] = 0x34;
            setup.items[TOP_CHOICE_QUIT] = 3;
            setup.count = TOP_CHOICE_MAX;
            break;
        case LIST_RECEIVE:
            setup.msgData = wk->msgData;
            setup.items[RECEIVE_CHOICE_WIRELESS] = 8;
            setup.items[RECEIVE_CHOICE_WIFI] = 9;
            setup.items[RECEIVE_CHOICE_INFRARED] = 10;
            setup.items[RECEIVE_CHOICE_BACK] = 11;
            setup.count = RECEIVE_CHOICE_MAX;
            break;
        case LIST_RECEIVED:
            setup.msgData = NULL;
            setup.items[RECEIVED_CHOICE_CARD] = (u32)GFL_StrBufCreate(37, heapId);
            GFL_StrBufLoadFixedString((StrBuf *)setup.items[RECEIVED_CHOICE_CARD], wk->recv.gift.title, 37);
            setup.count = RECEIVED_CHOICE_MAX;
            setup.offsetY = -1;
            setup.offsetX = -2;
            setup.cursorSequence = 10;
            break;
        case LIST_ABOUT:
            setup.msgData = wk->msgData;
            setup.items[ABOUT_CHOICE_GIFT] = 0x30;
            setup.items[ABOUT_CHOICE_ALBUM] = 0x31;
            setup.items[ABOUT_CHOICE_FULL_ALBUM] = 0x32;
            setup.items[ABOUT_CHOICE_BACK] = 0x33;
            setup.count = ABOUT_CHOICE_MAX;
            break;
        }
        arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0, 0xa0, heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 2, 0, 0, FALSE, heapId);
        if (type == LIST_RECEIVED) {
            loadBGScrToVramByNarcNoReserveNegAlign(ARCID_MYSTERY, 30, 2, 0, 0, FALSE, heapId);
        } else {
            loadBGScrToVramByNarcNoReserveNegAlign(ARCID_MYSTERY, 29, 2, 0, 0, FALSE, heapId);
        }
        GFL_ArcToolFree(arc);
        GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 24, 4);
        GFL_BGSysLoadScr(2);
        GFL_BGSysSetBGEnabled(2, TRUE);
        wk->list = MysteryList_Create(&setup, heapId);
        if (setup.msgData == NULL) {
            for (i = 0; i < 4; i++) {
                if (setup.items[i] != 0) {
                    GFL_StrBufFree((StrBuf *)setup.items[i]);
                }
            }
        }
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 4, 0x18, 15, 3);
        MysteryActors_ResetPalAnim(&wk->actors);
    }
}

static void Mystery_DeleteList(MysteryWork *wk) {
    if (wk->list != NULL) {
        GFL_BGSysClearScr(2);
        GFL_BGSysLoadScr(2);
        GFL_BGSysSetBGEnabled(2, FALSE);
        MysteryList_Delete(wk->list);
        wk->list = NULL;
    }
}

static u32 Mystery_UpdateList(MysteryWork *wk) {
    u32 ret = MysteryList_Update(wk->list);

    MysteryActors_Update(&wk->actors);
    return ret;
}

static void Mystery_OnListMove(void *work) {
    MysteryWork *wk = work;

    MysteryActors_ResetPalAnim(&wk->actors);
}

static void Mystery_CreateCardWin(MysteryWork *wk, HeapID heapId) {
    if (wk->cardWin == NULL) {
        MysteryTextWinEntry entries[2] = {
            { 1, 2, 30, 2, 0, NULL, TRUE, 0, 0, PRINT_COLOR(15, 2, 0) },
            { 2, 6, 28, 15, 0, NULL, FALSE, 0, 4, PRINT_COLOR(15, 2, 0) },
        };

        entries[0].str = GFL_StrBufCreate(37, heapId);
        GFL_StrBufLoadFixedString(entries[0].str, wk->recv.gift.title, 37);
        entries[1].str = GFL_StrBufCreate(253, heapId);
        GFL_StrBufLoadFixedString(entries[1].str, wk->recv.text, 253);
        wk->cardWin = MysteryTextWin_Create(0, entries, 2, 4, 15, wk->queue, wk->msgData, wk->font, heapId);
        GFL_StrBufFree(entries[0].str);
        GFL_StrBufFree(entries[1].str);
        MysteryBg_Load(&wk->bg, MYSTERY_BG_CARD_TEXT);
        GFL_BGSysLoadScr(6);
        GFL_BGSysSetBGEnabled(6, TRUE);
        G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, 4, 0x18, 15, 3);
    }
}

static void Mystery_DeleteCardWin(MysteryWork *wk) {
    if (wk->cardWin != NULL) {
        G2S_BlendNone();
        MysteryTextWin_Clear(wk->cardWin);
        MysteryTextWin_Delete(wk->cardWin);
        wk->cardWin = NULL;
        GFL_BGSysLoadScr(4);
        GFL_BGSysSetBGEnabled(6, FALSE);
    }
}

static void MysteryEffect_Init(MysteryEffect *effect, ClActUnit *unit, const MysteryGiftRecvData *recv,
                               GameData *gameData, MysteryActors *actors, MysteryBg *bg, MysteryBgScroll *scroll,
                               HeapID heapId) {
    u32 palNo;
    NNSG2dPaletteData *palette;
    void *buffer;
    u16 *colors;
    int i;
    PartyPkm *pkm;
    ArcTool *arc;
    s16 height;
    ClActorSetup setup;

    MI_CpuFill8(effect, 0, sizeof(MysteryEffect));
    effect->scroll = scroll;
    effect->bg = bg;
    effect->heapId = heapId;
    effect->recv = recv;
    effect->height = 0;
    effect->offsetX = 0;
    if (recv->special) {
        palNo = 3;
    } else {
        palNo = sKindPalettes[recv->gift.kind];
    }
    buffer = GFL_G2DIOReadNCLR(ARCID_MYSTERY, 2, &palette, heapId);
    colors = palette->pRawData;
    MI_CpuCopy8(colors, effect->baseColors, sizeof(effect->baseColors));
    MI_CpuCopy8(colors + palNo * 16, effect->kindColors, sizeof(effect->kindColors));
    GFL_HeapFree(buffer);
    for (i = 0; i < STAR_COUNT; i++) {
        ClActor_SetPalette(MysteryActors_GetActor(actors, ACTOR_STARS + i), (u8)palNo, 0);
    }
    switch (recv->gift.kind) {
    case MYSTERY_GIFT_KIND_NONE:
        break;
    case MYSTERY_GIFT_KIND_POKEMON:
        pkm = Mystery_CreateGiftPokemon(&recv->gift, HEAPID_TAIL(heapId), gameData);
        arc = MakePokeGraArcHandle(heapId);
        effect->palette =
            PokeGra_LoadClActPaletteByBoxData(arc, PokeParty_ToBoxPkm(pkm), POKEGRA_DIR_FRONT, 0, 0x1c0, heapId);
        effect->cellAnims =
            PokeGra_LoadClActCellAnimsByBoxData(PokeParty_ToBoxPkm(pkm), POKEGRA_DIR_FRONT, 2, 0, heapId);
        effect->chars = PokeGra_LoadClActCharsByBoxData(arc, PokeParty_ToBoxPkm(pkm), POKEGRA_DIR_FRONT, 0, heapId);
        effect->height = Mystery_GetSpriteBottom(PokeParty_ToBoxPkm(pkm), heapId) - 48;
        effect->height = MATH_CLAMP(effect->height, 0, 48);
        GFL_ArcToolFree(arc);
        GFL_HeapFree(pkm);
        break;
    case MYSTERY_GIFT_KIND_ITEM:
        arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, heapId);
        effect->palette = ClActSys_LoadPltt(arc, GetItemGraphicsDatID(recv->gift.value, ITEM_FILE_ICON_PLTT),
                                            CLACT_VRAM_MAIN, 0x1c0, heapId);
        effect->cellAnims = ClActSys_LoadCellAnims(arc, 1, 0, heapId);
        effect->chars = ClActSys_LoadChars(arc, GetItemGraphicsDatID(recv->gift.value, ITEM_FILE_ICON_CHAR), FALSE,
                                           CLACT_VRAM_MAIN, heapId);
        GFL_ArcToolFree(arc);
        effect->offsetX = 4;
        effect->height = 8;
        break;
    case MYSTERY_GIFT_KIND_PASS_POWER:
    case MYSTERY_GIFT_KIND_ONE_SHOT:
        arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, heapId);
        effect->palette = ClActSys_LoadPltt(arc, 3, CLACT_VRAM_MAIN, 0x1c0, heapId);
        effect->cellAnims = ClActSys_LoadCellAnims(arc, 33, 36, heapId);
        effect->chars = ClActSys_LoadChars(arc, 12, FALSE, CLACT_VRAM_MAIN, heapId);
        GFL_ArcToolFree(arc);
        effect->height = 8;
        break;
    }
    if (recv->special) {
        MI_CpuCopy8((void *)(HW_OBJ_PLTT + 14 * 0x20), effect->objColors, sizeof(effect->objColors));
        MIi_CpuClear16(0x7fff, (void *)(HW_OBJ_PLTT + 14 * 0x20), 0x20);
        MI_CpuCopy8((void *)(HW_OBJ_PLTT + 14 * 0x20), effect->whiteColors, sizeof(effect->whiteColors));
    }
    MI_CpuFill8(&setup, 0, sizeof(setup));
    setup.x = effect->offsetX + 128;
    setup.y = -96;
    setup.bgPriority = 2;
    effect->actor = ClActor_Create(unit, effect->chars, effect->palette, effect->cellAnims, &setup, 0, heapId);
    ClActor_SetVisible(effect->actor, FALSE);
    if (recv->special) {
        ClActor_SetObjMode(effect->actor, 1);
    }
}

static void MysteryEffect_Start(MysteryEffect *effect, u32 mode) {
    effect->done = FALSE;
    effect->state = 0;
    effect->frame = 0;
    switch (mode) {
    case MYSTERY_EFFECT_FALL:
        if (effect->recv->special) {
            effect->func = MysteryEffect_FallSpecial;
        } else {
            effect->func = MysteryEffect_Fall;
        }
        break;
    case MYSTERY_EFFECT_FADE_BACK:
        effect->func = MysteryEffect_FadeBack;
        break;
    }
    GX_SetVisibleWnd(GX_WNDMASK_W0);
    G2_SetWnd0InsidePlane(15, TRUE);
    G2_SetWndOutsidePlane(31, TRUE);
    G2_SetWnd0Position(0, 168, 255, 192);
}

static void MysteryEffect_Exit(MysteryEffect *effect) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    if (MysteryParticle_IsActive(&effect->particle)) {
        MysteryParticle_Exit(&effect->particle);
    }
    GFL_BGSysSetBGEnabled(2, FALSE);
    ClActor_Delete(effect->actor);
    ClActSys_FreePltt(effect->palette);
    ClActSys_FreeChars(effect->chars);
    ClActSys_FreeCellAnims(effect->cellAnims);
    MI_CpuFill8(effect, 0, sizeof(MysteryEffect));
}

static void MysteryEffect_Main(MysteryEffect *effect) {
    if (effect->func != NULL) {
        effect->func(effect);
        if (effect->done) {
            effect->func = NULL;
        }
    }
}

static BOOL MysteryEffect_IsEnd(MysteryEffect *effect) {
    return effect->done;
}

static inline void Mystery_SetBlendAlpha(s16 ev1, s16 ev2) {
    reg_G2_BLDALPHA = ev1 | (ev2 << 8);
}

static void MysteryEffect_Fall(MysteryEffect *effect) {
    u32 alpha;
    s16 y;

    switch (effect->state) {
    case FALL_STATE_LOAD_BG:
        GFL_BGSysSetBGEnabled(2, FALSE);
        MysteryBg_Load(effect->bg, MYSTERY_BG_EFFECT);
        effect->state = FALL_STATE_SHOW_BG;
        break;
    case FALL_STATE_SHOW_BG:
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 4, 8, 0, 16);
        GFL_BGSysSetBGEnabled(2, TRUE);
        effect->frame = 0;
        effect->state = FALL_STATE_FADE_BG;
        break;
    case FALL_STATE_FADE_BG:
        alpha = effect->frame * 16 / 120;
        Mystery_SetBlendAlpha(alpha, 16 - alpha);
        effect->blend = effect->frame * 0x7fff / 120;
        MysteryPal_Blend(15, effect->bgFadeColors, effect->blend, 0, effect->kindColors, effect->baseColors);
        MysteryPal_Blend(31, effect->subFadeColors, effect->blend, 0, effect->kindColors, effect->baseColors);
        if (effect->frame++ > 120) {
            G2_BlendNone();
            MysteryBgScroll_SetSpeed(effect->scroll, MYSTERY_SCROLL_FAST);
            effect->frame = 0;
            effect->state = FALL_STATE_WAIT_FALL;
        }
        break;
    case FALL_STATE_WAIT_FALL:
        if (effect->frame++ > 180) {
            effect->frame = 0;
            effect->state = FALL_STATE_FALL_ICON;
        }
        break;
    case FALL_STATE_FALL_ICON:
        y = (214 - effect->height) * effect->frame / 60 - 96;
        ClActor_SetSurfacePosAxis(effect->actor, y, 0, 1);
        if (y >= -48 && y <= 192) {
            ClActor_SetVisible(effect->actor, TRUE);
        }
        if (effect->frame++ >= 60) {
            GFL_SndSEPlay(SEQ_SE_SYS_50);
            MysteryBgScroll_SetSpeed(effect->scroll, MYSTERY_SCROLL_SLOW);
            effect->frame = 0;
            effect->state = FALL_STATE_WAIT_END;
        }
        break;
    case FALL_STATE_WAIT_END:
        if (effect->frame++ > 60) {
            effect->frame = 0;
            effect->state = FALL_STATE_END;
        }
        break;
    case FALL_STATE_END:
        effect->done = TRUE;
        break;
    }
}

static void MysteryEffect_FallSpecial(MysteryEffect *effect) {
    u32 alpha;
    ClActorPos pos;

    switch (effect->state) {
    case SPECIAL_FALL_STATE_LOAD_PARTICLES:
        MysteryParticle_Init(&effect->particle, effect->heapId);
        MysteryParticle_Load(&effect->particle, ARCID_MYSTERY, 38, effect->heapId);
        GFL_BGSysSetBGEnabled(2, FALSE);
        MysteryBg_Load(effect->bg, MYSTERY_BG_EFFECT);
        effect->state = SPECIAL_FALL_STATE_SHOW_BG;
        break;
    case SPECIAL_FALL_STATE_SHOW_BG:
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 4, 8, 0, 16);
        GFL_BGSysSetBGEnabled(2, TRUE);
        effect->frame = 0;
        effect->state = SPECIAL_FALL_STATE_FADE_BG;
        break;
    case SPECIAL_FALL_STATE_FADE_BG:
        alpha = effect->frame * 16 / 120;
        Mystery_SetBlendAlpha(alpha, 16 - alpha);
        effect->blend = effect->frame * 0x7fff / 120;
        MysteryPal_Blend(15, effect->bgFadeColors, effect->blend, 0, effect->kindColors, effect->baseColors);
        MysteryPal_Blend(31, effect->subFadeColors, effect->blend, 0, effect->kindColors, effect->baseColors);
        if (effect->frame++ > 120) {
            G2_BlendNone();
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_START_PARTICLES;
        }
        break;
    case SPECIAL_FALL_STATE_START_PARTICLES:
        if (effect->frame++ > 180) {
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_WAIT_PARTICLES;
            G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 1, 31, 0, 16);
            MysteryParticle_Emit(&effect->particle, 0);
            MysteryParticle_Emit(&effect->particle, 1);
            MysteryParticle_Emit(&effect->particle, 2);
            GFL_SndSEPlay(SEQ_SE_SYS_88);
        }
        break;
    case SPECIAL_FALL_STATE_WAIT_PARTICLES:
        if (effect->frame++ > 120) {
            GFL_SndSEPlay(SEQ_SE_SYS_89);
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_SHOW_ICON;
            MysteryBgScroll_SetSpeed(effect->scroll, MYSTERY_SCROLL_FAST);
        }
        break;
    case SPECIAL_FALL_STATE_SHOW_ICON:
        pos.x = 128;
        pos.y = 118 - effect->height;
        ClActor_SetSurfacePos(effect->actor, &pos, 0);
        ClActor_SetVisible(effect->actor, TRUE);
        effect->subFrame = 0;
        effect->state = SPECIAL_FALL_STATE_FADE_ICON;
        break;
    case SPECIAL_FALL_STATE_FADE_ICON:
        alpha = effect->subFrame * 16 / 10;
        Mystery_SetBlendAlpha(alpha, 16 - alpha);
        effect->frame++;
        if (effect->subFrame++ > 10) {
            effect->subFrame = 0;
            effect->state = SPECIAL_FALL_STATE_WAIT_ICON;
        }
        break;
    case SPECIAL_FALL_STATE_WAIT_ICON:
        if (effect->frame++ > 90) {
            G2_BlendNone();
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_WAIT_PALETTE;
        }
        break;
    case SPECIAL_FALL_STATE_WAIT_PALETTE:
        if (effect->frame++ > 10) {
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_FADE_PALETTE;
        }
        break;
    case SPECIAL_FALL_STATE_FADE_PALETTE:
        effect->blend = effect->frame * 0x7fff / 120;
        MysteryPal_Blend(14, effect->objFadeColors, effect->blend, 14, effect->objColors, effect->whiteColors);
        if (effect->frame++ > 120) {
            MysteryBgScroll_SetSpeed(effect->scroll, MYSTERY_SCROLL_SLOW);
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_WAIT_END;
        }
        break;
    case SPECIAL_FALL_STATE_WAIT_END:
        if (effect->frame++ > 60) {
            GFL_SndSEPlay(SEQ_SE_SYS_50);
            effect->frame = 0;
            effect->state = SPECIAL_FALL_STATE_END;
        }
        break;
    case SPECIAL_FALL_STATE_END:
        G2_BlendNone();
        effect->done = TRUE;
        break;
    }
}

static void MysteryEffect_FadeBack(MysteryEffect *effect) {
    switch (effect->state) {
    case FADE_BACK_STATE_START:
        effect->state = FADE_BACK_STATE_FADE;
        break;
    case FADE_BACK_STATE_FADE:
        effect->blend = effect->frame * 0x7fff / 60;
        MysteryPal_Blend(15, effect->bgFadeColors, effect->blend, 0, effect->baseColors, effect->kindColors);
        MysteryPal_Blend(31, effect->subFadeColors, effect->blend, 0, effect->baseColors, effect->kindColors);
        if (effect->frame++ > 60) {
            effect->frame = 0;
            effect->state = FADE_BACK_STATE_END;
        }
        break;
    case FADE_BACK_STATE_END:
        effect->done = TRUE;
        break;
    }
}

static void MysteryBgScroll_Init(MysteryBgScroll *scroll, MysteryActors *actors, HeapID heapId) {
    int i;
    ClActorPos pos;

    MI_CpuFill8(scroll, 0, sizeof(MysteryBgScroll));
    scroll->speed = -FX32_CONST(0.2);
    scroll->actors = actors;
    scroll->startSpeed = -FX32_CONST(0.2);
    scroll->heapId = heapId;
    for (i = 0; i < STAR_COUNT; i++) {
        ClActor_GetPos(MysteryActors_GetActor(scroll->actors, ACTOR_STARS + i), &pos);
        scroll->starY[i] = pos.y << FX32_SHIFT;
        scroll->starSpeed[i] = GFL_RandomLCAlt(FX32_ONE) + FX32_CONST(1.6);
    }
}

static void MysteryBgScroll_Exit(MysteryBgScroll *scroll) {
    MI_CpuFill8(scroll, 0, sizeof(MysteryBgScroll));
}

static void MysteryBgScroll_Main(MysteryBgScroll *scroll) {
    if (scroll->enabled && scroll->active && scroll->func != NULL) {
        scroll->func(scroll);
    }
}

static void MysteryBgScroll_SetSpeed(MysteryBgScroll *scroll, u32 mode) {
    scroll->active = TRUE;
    scroll->frame = 0;
    scroll->startSpeed = scroll->speed;
    switch (mode) {
    case MYSTERY_SCROLL_SLOW:
        scroll->func = MysteryBgScroll_Slow;
        break;
    case MYSTERY_SCROLL_FAST:
        scroll->func = MysteryBgScroll_Fast;
        break;
    }
}

static void MysteryBgScroll_SetEnabled(MysteryBgScroll *scroll, BOOL enabled) {
    scroll->enabled = enabled;
}

static void MysteryBgScroll_Slow(MysteryBgScroll *scroll) {
    int i;
    ClActor *actor;
    ClActorPos pos;

    if (scroll->speed != -FX32_CONST(0.2)) {
        scroll->speed = scroll->startSpeed + (-FX32_CONST(0.2) - scroll->startSpeed) * scroll->frame / 60;
        if (scroll->frame++ > 60) {
            scroll->speed = -FX32_CONST(0.2);
        }
    }
    for (i = 0; i < STAR_COUNT; i++) {
        actor = MysteryActors_GetActor(scroll->actors, ACTOR_STARS + i);
        if (ClActor_IsVisible(actor)) {
            scroll->starY[i] += scroll->starSpeed[i];
            ClActor_GetPos(actor, &pos);
            pos.y = scroll->starY[i] >> FX32_SHIFT;
            ClActor_SetPos(actor, &pos);
            if (pos.y >= 0 && pos.y <= 208) {
                ClActor_SetVisible(actor, TRUE);
            } else {
                ClActor_SetVisible(actor, FALSE);
            }
        }
    }
    scroll->pos += scroll->speed;
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_Y, scroll->pos >> FX32_SHIFT);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, scroll->pos >> FX32_SHIFT);
}

static void MysteryBgScroll_Fast(MysteryBgScroll *scroll) {
    int i;
    ClActor *actor;
    ClActorPos pos;

    if (scroll->speed != -FX32_CONST(4)) {
        scroll->speed = scroll->startSpeed + (-FX32_CONST(4) - scroll->startSpeed) * scroll->frame / 60;
        if (scroll->frame++ > 60) {
            scroll->speed = -FX32_CONST(4);
        }
    }
    for (i = 0; i < STAR_COUNT; i++) {
        scroll->starY[i] += scroll->starSpeed[i];
        if (scroll->starY[i] >= FX32_CONST(208)) {
            scroll->starY[i] = -FX32_CONST(16);
        }
        actor = MysteryActors_GetActor(scroll->actors, ACTOR_STARS + i);
        ClActor_GetPos(actor, &pos);
        pos.y = scroll->starY[i] >> FX32_SHIFT;
        ClActor_SetPos(actor, &pos);
        if (pos.y >= 0 && pos.y <= 208) {
            ClActor_SetVisible(actor, TRUE);
        } else {
            ClActor_SetVisible(actor, FALSE);
        }
    }
    scroll->pos += scroll->speed;
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_Y, scroll->pos >> FX32_SHIFT);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, scroll->pos >> FX32_SHIFT);
}

static void MysteryOamMsg_Init(MysteryOamMsg *msg, const MysteryActors *actors, ClActUnit *unit, PrintQueue *queue,
                               MsgData *msgData, u32 msgId, Font *font, HeapID heapId) {
    ClActorSetup setup;

    MI_CpuFill8(msg, 0, sizeof(MysteryOamMsg));
    msg->actors = actors;
    msg->bmpOam = BmpOam_Init(heapId, unit);
    MI_CpuFill8(&setup, 0, sizeof(setup));
    setup.x = 96;
    setup.y = 169;
    setup.priority = 0;
    setup.bgPriority = 0;
    msg->text = MysteryOamText_Create(&setup, 8, 2, actors->palette, 3, 0, msg->bmpOam, queue, heapId);
    MysteryOamText_SetColor(msg->text, PRINT_COLOR(1, 3, 0));
    MysteryOamText_SetAlign(msg->text, 0, 0, 1);
    MysteryOamText_Print(msg->text, msgData, msgId, font);
    ClActor_SetVisible(MysteryActors_GetActor(actors, ACTOR_HINT), TRUE);
}

static void MysteryOamMsg_Exit(MysteryOamMsg *msg) {
    MysteryOamText_Clear(msg->text);
    ClActor_SetVisible(MysteryActors_GetActor(msg->actors, ACTOR_HINT), FALSE);
    MysteryOamText_Delete(msg->text);
    BmpOam_Exit(msg->bmpOam);
    MI_CpuFill8(msg, 0, sizeof(MysteryOamMsg));
}

static BOOL MysteryOamMsg_Update(MysteryOamMsg *msg) {
    return MysteryOamText_Update(msg->text);
}

// The lowest row of the Pokémon's front sprite that has a pixel
static u32 Mystery_GetSpriteBottom(BoxPkm *pkm, HeapID heapId) {
    NNSG2dCharacterData *charData;
    void *buffer;
    u32 bottom = 48;
    BOOL found = FALSE;
    int row;
    int col;
    int y;
    u32 *tile;

    buffer = LoadSingleCellSpindaGraphicsByBoxData(&charData, pkm, POKEGRA_DIR_FRONT, HEAPID_TAIL(heapId));
    PokeGra_CellCharsToImage(charData, HEAPID_TAIL(heapId));
    for (row = 11; row >= 0; row--) {
        for (col = 0; col < 12; col++) {
            tile = (u32 *)charData->pRawData + (row * 12 + col) * 8;
            for (y = 7; y >= 0; y--) {
                if (tile[y] != 0) {
                    if (bottom < y + row * 8) {
                        bottom = y + row * 8;
                    }
                    found = TRUE;
                }
            }
        }
        if (found) {
            break;
        }
    }
    GFL_HeapFree(buffer);
    return bottom;
}

static BOOL Mystery_IsRecvDataValid(const MysteryGiftRecvData *recv, u32 errors) {
    if (MysteryGiftRecvData_CheckCRC(recv) && errors == 0 &&
        (recv->versions == 0 || (recv->versions & (1 << GAME_VERSION)))) {
        return TRUE;
    }
    return FALSE;
}
