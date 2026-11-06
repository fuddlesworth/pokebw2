#ifndef POKEBW2_APP_POKELIST_H
#define POKEBW2_APP_POKELIST_H

#include "types.h"
#include "app/p_status.h"
#include "field/player_action.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/proc.h"
#include "gfl/touchpanel.h"
#include "save/resh_zek.h"
#include "struct_decls.h"
#include "system/printsys.h"

// Overlay 165's party list, which EventPokeList_Create runs, showing overlay 207's summary screen for a Pokémon
// when asked to. Its files: pokelist.c (the proc), plist_sys.c (the screen), plist_plate.c (a Pokémon's plate),
// plist_message.c, plist_menu.c, plist_item.c (using an item), plist_battle.c (the partners' teams and the timer of
// a link battle's team selection), plist_demo.c (form changes) and status_rcv.c (an item's effect on a Pokémon)

// A partner's team, shown beside the player's in a multi battle's selection
typedef struct {
    PokeParty *party;
    u16 *name;
    u8 gender;
} PokeListPartnerParam;

// 0xa8 bytes, which PokeListParam_Setup (plist_data_setup.c) fills in
typedef struct {
    PokeParty *party;
    BagSave *bag;
    MailData *mailbox;
    TrainerDataSave *trainerData;
    // The Pokémon fused with Kyurem (getReshZekBlkAddress)
    ReshZekSave *reshZek;
    Regulation *regulation;
    void *extraRule;
    PokeDexSave *pokedex;
    ShortcutSave *shortcut;
    TrainerCardSave *trainerCard;
    PlayerInfo *playerInfo;
    PlayerActionPossibilities action;
    u16 zoneId;
    // What the screen is for
    int mode;
    u32 unk48;
    // The Pokémon whose summary to show, when result is 1
    u32 index;
    // 0 once the screen is done, and 1 to show a summary
    u32 result;
    u16 item;
    u16 move;
    // The move slot to teach the move to
    u8 moveSlot;
    // The party slots, from 1, of the Pokémon picked
    u8 picked[6];
    // Where to go on in the moves a Pokémon learns at its new level
    u32 learnIndex;
    u16 unk64;
    u32 unk68;
    u16 mailboxSlot;
    // A bit for each Pokémon of the party that can learn the move on offer
    u8 learnable;
    u8 season;
    // Seconds left to pick, counted down while timerEnabled is set
    u16 timeLeft;
    // The state and error of overlay 164's sync (net_sync.c), which takes the parameters as its NetSyncWork, and
    // the list ends once forceExit is 1
    u8 unk72;
    u8 forceExit;
    u32 partnerLayout;
    PokeListPartnerParam partners[3];
    BOOL showPartners;
    BOOL timerEnabled;
    u8 battleMsg;
    // Whether key item 0x1e is registered to Y, and whether it is to be, once the screen is done
    u8 keyItemRegistered;
    u8 unkA6[2];
} PokeListParam;

// The party-icon and held-item resource sequences shared with the partner teams
enum {
    POKELIST_PARTY_ICON_ANM_FAINTED = 0,
    POKELIST_PARTY_ICON_ANM_DEFAULT = 1,
    POKELIST_PARTY_ICON_ANM_HP_GREEN = 2,
    POKELIST_PARTY_ICON_ANM_HP_YELLOW = 3,
    POKELIST_PARTY_ICON_ANM_HP_RED = 4,
    POKELIST_PARTY_ICON_ANM_STATUS = 5,
};

enum {
    POKELIST_HELD_ITEM_ANM_ITEM = 0,
    POKELIST_HELD_ITEM_ANM_MAIL = 1,
};

// Offsets from BtlMain11_Text_WontHaveAnyEffect in the item refusal messages
enum {
    POKELIST_ITEM_MESSAGE_NO_EFFECT,
    POKELIST_ITEM_MESSAGE_FAINTED,
    POKELIST_ITEM_MESSAGE_NO_FUSION_SPACE,
};

// The moves to teach that stand for a set of moves: the ultimate moves, and the pledges
#define POKELIST_MOVE_ULTIMATE 0xfffe
#define POKELIST_MOVE_PLEDGE 0xfffd

#define POKELIST_PLATE_COUNT 6

// What the party list is opened for
enum {
    POKELIST_MODE_FIELD = 0,
    POKELIST_MODE_BATTLE_TEAM = 1,
    POKELIST_MODE_SELECT = 3,
    POKELIST_MODE_USE_ITEM = 5,
    POKELIST_MODE_TEACH_TM = 6,
    POKELIST_MODE_REPLACE_TM_MOVE = 7,
    POKELIST_MODE_REPLACE_LEVEL_MOVE = 8,
    POKELIST_MODE_GIVE_ITEM = 9,
    POKELIST_MODE_GIVE_FROM_BAG = 10,
    POKELIST_MODE_GIVE_ITEM_EXIT = 11,
    POKELIST_MODE_GIVE_ITEM_RETURN = 12,
    POKELIST_MODE_GIVE_MAIL = 14,
    POKELIST_MODE_EVOLUTION_ITEM = 16,
    POKELIST_MODE_DAY_CARE = 18,
    POKELIST_MODE_SELECT_EGG = 21,
    POKELIST_MODE_BATTLE_TEAM_CANCEL = 22,
    POKELIST_MODE_BATTLE_INSTITUTE = 23,
    POKELIST_MODE_MOVE_TUTOR = 24,
    POKELIST_MODE_MUSICAL = 25,
    POKELIST_MODE_NETWORK_TEAM = 26,
    POKELIST_MODE_SELECT_EGG_PROMPT = 27,
};

// Which other trainers' teams are shown beside the player's
enum {
    POKELIST_PARTNER_LAYOUT_ONE_TEAM = 1,
    POKELIST_PARTNER_LAYOUT_THREE_TEAMS = 2,
};

// The party list screen states
enum {
    POKELIST_STATE_FADE_IN = 0,
    POKELIST_STATE_WAIT_WIPE = 1,
    POKELIST_STATE_SELECT = 2,
    POKELIST_STATE_SELECT_SWAP = 3,
    POKELIST_STATE_SELECT_HP_SHARE = 4,
    POKELIST_STATE_SWAP = 5,
    POKELIST_STATE_MENU = 6,
    POKELIST_STATE_MESSAGE = 7,
    POKELIST_STATE_IDLE = 8,
    POKELIST_STATE_SUBMENU = 9,
    POKELIST_STATE_HP_CHANGE_START = 10,
    POKELIST_STATE_HP_CHANGE = 11,
    POKELIST_STATE_WAIT_TEAM_BUTTON = 12,
    POKELIST_STATE_LEVEL_UP = 13,
    POKELIST_STATE_DEMO_START = 14,
    POKELIST_STATE_DEMO_UPDATE = 15,
    POKELIST_STATE_DEMO_END = 16,
    POKELIST_STATE_SYNC_START = 17,
    POKELIST_STATE_SYNC_WAIT = 18,
    POKELIST_STATE_EXIT_WAIT_BUTTON = 19,
    POKELIST_STATE_EXIT_FADE = 20,
    POKELIST_STATE_EXIT_WAIT_WIPE = 21,
    POKELIST_STATE_SELECT_ITEM_SWAP = 22,
    POKELIST_STATE_SELECT_FUSION = 23,
};

// Selection, menu and swap substates
enum {
    POKELIST_SUBSTATE_INIT = 0,
    POKELIST_SUBSTATE_READ_INPUT = 1,
    POKELIST_SUBSTATE_HANDLE_INPUT = 2,
    POKELIST_SUBSTATE_SWAP_OUT = 3,
    POKELIST_SUBSTATE_SWAP_IN = 4,
};

// The party list input actions
enum {
    POKELIST_INPUT_CONFIRM = 0,
    POKELIST_INPUT_CANCEL = 1,
    POKELIST_INPUT_CLOSE = 2,
    POKELIST_INPUT_SELECT_POKEMON = 3,
    POKELIST_INPUT_NONE = 4,
};

// Cursor and returned button positions after the party slots
enum {
    POKELIST_CURSOR_CONFIRM = 6,
    POKELIST_CURSOR_CANCEL = 7,
    POKELIST_CURSOR_CLOSE = 8,
    POKELIST_CURSOR_NONE = 9,
};

// The menu entries and expanded move slots
enum {
    POKELIST_MENU_SUMMARY = 0,
    POKELIST_MENU_FIELD_MOVES = 1,
    POKELIST_MENU_ITEM_MOVES = 2,
    POKELIST_MENU_SWITCH = 3,
    POKELIST_MENU_ITEM = 4,
    POKELIST_MENU_MAIL = 5,
    POKELIST_MENU_BACK = 6,
    POKELIST_MENU_SELECT = 7,
    POKELIST_MENU_BATTLE_ENTRY = 8,
    POKELIST_MENU_GIVE = 9,
    POKELIST_MENU_TAKE = 10,
    POKELIST_MENU_CHOOSE = 11,
    POKELIST_MENU_READ_MAIL = 12,
    POKELIST_MENU_TAKE_MAIL = 13,
    POKELIST_MENU_YES = 14,
    POKELIST_MENU_NO = 15,
    POKELIST_MENU_MOVE1 = 16,
    POKELIST_MENU_MOVE2 = 17,
    POKELIST_MENU_MOVE3 = 18,
    POKELIST_MENU_MOVE4 = 19,
    POKELIST_MENU_ENTER = 20,
    POKELIST_MENU_WITHDRAW = 21,
    POKELIST_MENU_MOVE_ITEM = 22,
    POKELIST_MENU_MOVE_MAIL = 23,
    POKELIST_MENU_NONE = 25,
    POKELIST_MENU_END = 16,
};

// What the party list returns to its caller
enum {
    POKELIST_RESULT_DONE = 0,
    POKELIST_RESULT_SUMMARY = 1,
    POKELIST_RESULT_GIVE_ITEM = 3,
    POKELIST_RESULT_REPLACE_TM_MOVE = 4,
    POKELIST_RESULT_REPLACE_LEVEL_MOVE = 5,
    POKELIST_RESULT_WRITE_MAIL = 6,
    POKELIST_RESULT_READ_MAIL = 7,
    POKELIST_RESULT_EVOLVE_ITEM = 8,
    POKELIST_RESULT_EVOLVE_LEVEL = 9,
    POKELIST_RESULT_ITEM_DONE = 10,
    POKELIST_RESULT_CUT = 11,
    POKELIST_RESULT_SURF = 12,
    POKELIST_RESULT_WATERFALL = 13,
    POKELIST_RESULT_STRENGTH = 14,
    POKELIST_RESULT_FLY = 15,
    POKELIST_RESULT_FLASH = 16,
    POKELIST_RESULT_TELEPORT = 17,
    POKELIST_RESULT_DIG = 18,
    POKELIST_RESULT_SWEET_SCENT = 19,
    POKELIST_RESULT_CHATTER = 20,
    POKELIST_RESULT_DIVE = 21,
    POKELIST_RESULT_MILK_DRINK = 22,
    POKELIST_RESULT_SOFTBOILED = 23,
};

// Whether a move can be taught
enum {
    POKELIST_LEARN_EMPTY_SLOT = 0,
    POKELIST_LEARN_REPLACE_MOVE = 1,
    POKELIST_LEARN_UNABLE = 2,
    POKELIST_LEARN_KNOWN = 3,
};

// Whether a Pokemon may join the battle team
enum {
    POKELIST_ENTRY_CHECK_ALLOWED = 0,
    POKELIST_ENTRY_CHECK_REGULATION = 1,
    POKELIST_ENTRY_CHECK_SAME_SPECIES = 2,
    POKELIST_ENTRY_CHECK_SAME_ITEM = 3,
    POKELIST_ENTRY_CHECK_FULL = 4,
};

// What the item does on the party list
enum {
    POKELIST_ITEM_EFFECT_NONE = 0,
    POKELIST_ITEM_EFFECT_REVIVE_ALL = 1,
    POKELIST_ITEM_EFFECT_LEVEL_UP = 2,
    POKELIST_ITEM_EFFECT_CURE_SLEEP = 3,
    POKELIST_ITEM_EFFECT_CURE_POISON = 4,
    POKELIST_ITEM_EFFECT_CURE_BURN = 5,
    POKELIST_ITEM_EFFECT_CURE_FREEZE = 6,
    POKELIST_ITEM_EFFECT_CURE_PARALYSIS = 7,
    POKELIST_ITEM_EFFECT_CURE_CONFUSION = 8,
    POKELIST_ITEM_EFFECT_CURE_ALL_STATUS = 9,
    POKELIST_ITEM_EFFECT_CURE_INFATUATION = 10,
    POKELIST_ITEM_EFFECT_RESTORE_HP = 11,
    POKELIST_ITEM_EFFECT_REVIVE = 12,
    POKELIST_ITEM_EFFECT_RAISE_HP_EV = 13,
    POKELIST_ITEM_EFFECT_RAISE_ATTACK_EV = 14,
    POKELIST_ITEM_EFFECT_RAISE_DEFENSE_EV = 15,
    POKELIST_ITEM_EFFECT_RAISE_SPEED_EV = 16,
    POKELIST_ITEM_EFFECT_RAISE_SP_ATTACK_EV = 17,
    POKELIST_ITEM_EFFECT_RAISE_SP_DEFENSE_EV = 18,
    POKELIST_ITEM_EFFECT_LOWER_HP_EV = 19,
    POKELIST_ITEM_EFFECT_LOWER_ATTACK_EV = 20,
    POKELIST_ITEM_EFFECT_LOWER_DEFENSE_EV = 21,
    POKELIST_ITEM_EFFECT_LOWER_SPEED_EV = 22,
    POKELIST_ITEM_EFFECT_LOWER_SP_ATTACK_EV = 23,
    POKELIST_ITEM_EFFECT_LOWER_SP_DEFENSE_EV = 24,
    POKELIST_ITEM_EFFECT_EVOLVE = 25,
    POKELIST_ITEM_EFFECT_PP_UP = 26,
    POKELIST_ITEM_EFFECT_PP_MAX = 27,
    POKELIST_ITEM_EFFECT_RESTORE_PP = 28,
    POKELIST_ITEM_EFFECT_UNUSABLE = 29,
};

// The form change being shown
enum {
    POKELIST_DEMO_NONE = 0,
    POKELIST_DEMO_GIRATINA_ORIGIN = 1,
    POKELIST_DEMO_GIRATINA_ALTERED = 2,
    POKELIST_DEMO_SHAYMIN_SKY = 3,
    POKELIST_DEMO_KYUREM_FUSE = 4,
    POKELIST_DEMO_KYUREM_SEPARATE = 5,
    POKELIST_DEMO_TOGGLE_THERIAN = 6,
};

// One-Pokemon form-change steps
enum {
    POKELIST_FORM_STEP_START = 0,
    POKELIST_FORM_STEP_WAIT_DELAY = 1,
    POKELIST_FORM_STEP_WAIT_EFFECT = 2,
    POKELIST_FORM_STEP_WAIT_CRY = 3,
    POKELIST_DEMO_STEP_START = POKELIST_FORM_STEP_START,
};

// Kyurem fusion and separation steps
enum {
    POKELIST_FUSION_STEP_START = 0,
    POKELIST_FUSION_STEP_WAIT_DELAY = 1,
    POKELIST_FUSION_STEP_APPLY = 2,
    POKELIST_FUSION_STEP_WAIT_TEXT = 3,
    POKELIST_FUSION_STEP_WAIT_CRY = 4,
    POKELIST_FUSION_STEP_WAIT_FADE = 5,
};

// The level-up display and move-learning steps
enum {
    POKELIST_LEVELUP_DRAW_GAINS = 0,
    POKELIST_LEVELUP_TRANSFER_GAINS = 1,
    POKELIST_LEVELUP_WAIT_GAINS = 2,
    POKELIST_LEVELUP_DRAW_STATS = 3,
    POKELIST_LEVELUP_TRANSFER_STATS = 4,
    POKELIST_LEVELUP_WAIT_STATS = 5,
    POKELIST_LEVELUP_LEARN_MOVES = 6,
    POKELIST_LEVELUP_CHECK_EVOLUTION = 7,
};

// The checks for using the DNA Splicers
enum {
    POKELIST_FUSION_CHECK_CAN_FUSE = 0,
    POKELIST_FUSION_CHECK_CAN_SEPARATE = 1,
    POKELIST_FUSION_CHECK_NOT_KYUREM = 2,
    POKELIST_FUSION_CHECK_FAINTED = 3,
    POKELIST_FUSION_CHECK_WRONG_FORM = 4,
    POKELIST_FUSION_CHECK_PARTY_FULL = 5,
};

// The fusion partner, or why it cannot be used
enum {
    POKELIST_FUSION_PARTNER_RESHIRAM = 0,
    POKELIST_FUSION_PARTNER_ZEKROM = 1,
    POKELIST_FUSION_PARTNER_SEPARATE = 2,
    POKELIST_FUSION_PARTNER_WRONG_SPECIES = 3,
    POKELIST_FUSION_PARTNER_FAINTED = 4,
    POKELIST_FUSION_PARTNER_EGG = 5,
};

// Why a field move cannot be used
enum {
    POKELIST_HM_CHECK_ALLOWED = 0,
    POKELIST_HM_CHECK_UNAVAILABLE = 1,
    POKELIST_HM_CHECK_BADGE_REQUIRED = 2,
    POKELIST_HM_CHECK_WITH_PARTNER = 3,
    POKELIST_HM_CHECK_ALREADY_SURFING = 4,
};

// The party plate palettes
enum {
    POKELIST_PLATE_PALETTE_NORMAL = 3,
    POKELIST_PLATE_PALETTE_FAINTED = 4,
    POKELIST_PLATE_PALETTE_MARKED = 5,
    POKELIST_PLATE_PALETTE_SELECTED = 6,
    POKELIST_PLATE_PALETTE_SELECTED_FAINTED = 7,
    POKELIST_PLATE_PALETTE_MARKED_SELECTED = 8,
};

// The message control-code events this screen handles
enum {
    POKELIST_MESSAGE_EVENT_LEVEL_UP = 2,
    POKELIST_MESSAGE_EVENT_SOUND = 3,
    POKELIST_MESSAGE_EVENT_WAIT_LEVEL_UP = 5,
    POKELIST_MESSAGE_EVENT_WAIT_SOUND = 6,
};

// The message window's shapes
enum {
    POKELIST_MESSAGE_WINDOW_SHORT,
    POKELIST_MESSAGE_WINDOW_SHORTER,
    POKELIST_MESSAGE_WINDOW_MENU,
    POKELIST_MESSAGE_WINDOW_WIDE,
    POKELIST_MESSAGE_WINDOW_NONE,
};

// A plate's entry in a battle's selection, when it is not one of the order's places from 0
enum {
    POKELIST_ENTRY_ABLE = 6,
    POKELIST_ENTRY_UNABLE,
    // Not a battle's selection, or an empty slot
    POKELIST_ENTRY_EMPTY,
};
#define POKELIST_CL_RES_COUNT 23
#define CL_RES_NONE 0xffffffff
// The palettes, characters and cell animations in PokeListWork's clResources
#define CL_RES_PLTT(i) (i)
#define CL_RES_CHAR(i) (8 + (i))
#define CL_RES_CELL(i) (15 + (i))
#define CL_RES_PLTT_COUNT 8
#define CL_RES_CHAR_COUNT 7
#define CL_RES_CELL_COUNT 8

// The list's work, which every file of the overlay shares
struct PokeListWork {
    u16 heapId;
    TCB *vblankTask;
    BOOL touch;
    u8 state;
    // The state to go to once the wipe is done
    u8 nextState;
    u8 subState;
    u8 input;
    // Holds the glow of the selected plate
    BOOL glowPaused;
    BOOL showCloseAndShortcut;
    // The modes restored when the list ends
    BOOL wasTutorMode;
    BOOL wasMusicalMode;
    BOOL wasNetworkTeamMode;
    BOOL playHealSe;
    BOOL allowEggs;
    BOOL wasEggPromptMode;
    // 0 to 5 for the plates, then the buttons
    s32 cursorPos;
    // 9 when nothing is selected
    s32 selectPos;
    s32 selectPos2;
    PartyPkm *pkm;
    u32 menuItem;
    // The palettes that glow, as loaded, and as blended
    u16 baseColors[3][16];
    u16 colors[3][16];
    u8 flashTimer;
    u16 glowAngle;
    u8 swapStep;
    u8 enteredCount;
    AppTaskMenuWin *buttons[2];
    u16 unk114;
    u16 unk116;
    u8 waitTimer;
    BOOL decided;
    u16 prevHp;
    s8 hpStep;
    // Called once a plate's HP is done changing
    void (*hpDoneFunc)(PokeListWork *wk);
    // The stats before a level up
    u16 prevStats[6];
    BmpWin *statsWindow;
    MsgData *msgData;
    Font *font;
    Font *smallFont;
    PrintQueue *printQueue;
    BOOL msgWaitInput;
    // Called once a message is done
    void (*msgDoneFunc)(PokeListWork *wk);
    // Called with the item picked in the menu
    void (*menuFunc)(PokeListWork *wk, u32 item);
    PokeListMessage *message;
    PokeListMenu *menu;
    void *plateScreenFile;
    NNSG2dScreenData *plateScreen;
    PokeListPlate *plates[POKELIST_PLATE_COUNT];
    // Palettes, then characters, then cell animations
    u32 clResources[POKELIST_CL_RES_COUNT];
    ClActUnit *actorUnit;
    ClActor *cursor;
    ClActor *subCursor;
    ClActor *backButton;
    ClActor *closeButton;
    ClActor *shortcutMark;
    ClActor *pressedButton;
    ClActUnit *partnerUnit;
    PokeListPartner *partners[3];
    BmpWin *timerWindows[2];
    BOOL timerWindowDirty[2];
    u8 shownBattleMsg;
    u8 shownTime;
    u16 timerBaseColors[16];
    u16 timerColors[16];
    u16 timerBlink;
    BOOL timeUp;
    AppTaskMenuRes *taskMenuRes;
    G3DCamera *camera;
    void *particleHeaps[2];
    ParticleSystem *particles[2];
    // The form change shown
    u32 demo;
    u8 demoTimer;
    u8 demoLength;
    u8 demoStep;
    u8 demoForm;
    u16 demoSe;
    u32 voice;
    // Whether PokeList_TimeUp waits for network sync before exit; this overlay leaves it FALSE.
    BOOL syncOnTimeUp;
    u32 unk288;
    PokeListParam *param;
};

extern GameProcFunctions POKELIST_PROC_FUNCTIONS;

void PokeListParam_Setup(PokeListParam *param, GameData *gameData, u32 mode, PokeParty *party);
// Allocates the parameters and fills them in with PokeListParam_Setup
PokeListParam *PokeListParam_Create(GameData *gameData, u32 mode, PokeParty *party, HeapID heapId);
GameEvent *EventPokeList_Create(GameSystem *gsys, Field *field, PokeListParam *param, PStatusParam *summaryParam);

// plist_sys.c
BOOL PokeList_Init(PokeListWork *wk);
BOOL PokeList_Exit(PokeListWork *wk);
BOOL PokeList_Main(PokeListWork *wk);
// BG 0 as a text BG, or as the 3D BG with the camera and particles of plist_demo.c's form changes
void PokeList_CreateBG0(PokeListWork *wk);
void PokeList_Init3D(PokeListWork *wk);
void PokeList_ReleaseBG0(PokeListWork *wk);
void PokeList_Exit3D(PokeListWork *wk);
void PokeList_ShowMessage(PokeListWork *wk, u32 msgId, BOOL waitInput, void (*doneFunc)(PokeListWork *wk));
u32 PokeList_GetHMResult(PartyPkm *pkm, u8 slot);
BOOL PokeList_IsBattle(PokeListWork *wk);
u32 PokeList_CheckLearnMove(PokeListWork *wk, PartyPkm *pkm, u8 pos);
BOOL PokeList_CanEvolveWithItem(PokeListWork *wk, PartyPkm *pkm, u16 item);
void PokeList_PrintString(PokeListWork *wk, BmpWin *window, u16 msgId, u16 x, s16 y, u16 color);
void PokeList_PrintStringSmall(PokeListWork *wk, BmpWin *window, u32 msgId, int x, s16 y, u16 color);
void PokeList_DrawStringSmall(PokeListWork *wk, BmpWin *window, u32 msgId, int x, s16 y, u16 color);
void PokeList_PrintWordSetString(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                 u16 color);
void PokeList_PrintWordSetStringSmall(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                      u16 color);
void PokeList_DrawWordSetStringSmall(PokeListWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, s16 x, s16 y,
                                     u16 color);
void PokeList_MessageDoneSelect(PokeListWork *wk);
void PokeList_MessageDoneExit(PokeListWork *wk);
void PokeList_MessageDoneItem(PokeListWork *wk);
void PokeList_AskReplaceMove(PokeListWork *wk);
void PokeList_TimeUp(PokeListWork *wk);

// plist_plate.c
PokeListPlate *PokeListPlate_Create(PokeListWork *wk, u8 index, PartyPkm *pkm);
PokeListPlate *PokeListPlate_CreateEmpty(PokeListWork *wk, u8 index);
void PokeListPlate_Free(PokeListWork *wk, PokeListPlate *plate);
BOOL PokeListPlate_IsPrinting(PokeListPlate *plate);
void PokeListPlate_Update(PokeListWork *wk, PokeListPlate *plate);
void PokeListPlate_SetSelected(PokeListWork *wk, PokeListPlate *plate, BOOL selected);
void PokeListPlate_SetPalette(PokeListWork *wk, PokeListPlate *plate, u32 palette);
void PokeListPlate_DrawSlid(PokeListWork *wk, PokeListPlate *plate, int step);
void PokeListPlate_ClearSlid(PokeListWork *wk, PokeListPlate *plate, int step);
void PokeListPlate_SetPkm(PokeListWork *wk, PokeListPlate *plate, PartyPkm *pkm, int step);
void PokeListPlate_Redraw(PokeListWork *wk, PokeListPlate *plate);
void PokeListPlate_StartHpChange(PokeListWork *wk, PokeListPlate *plate);
BOOL PokeListPlate_UpdateHpChange(PokeListWork *wk, PokeListPlate *plate);
BOOL PokeListPlate_IsValid(PokeListWork *wk, PokeListPlate *plate);
void PokeListPlate_GetCursorPos(PokeListWork *wk, PokeListPlate *plate, ClActorPos *pos);
void PokeListPlate_GetTouchRect(PokeListWork *wk, PokeListPlate *plate, TouchRect *rect);
int PokeListPlate_GetEntry(PokeListPlate *plate);
void PokeListPlate_SetEntry(PokeListWork *wk, PokeListPlate *plate, int entry);
u16 PokeListPlate_GetHp(PokeListWork *wk, PokeListPlate *plate);
BOOL PokeListPlate_IsEgg(PokeListWork *wk, PokeListPlate *plate);
u32 PokeListPlate_CheckEntry(PokeListWork *wk, PokeListPlate *plate);

// plist_message.c
PokeListMessage *PokeListMessage_Create(PokeListWork *wk);
void PokeListMessage_Free(PokeListWork *wk, PokeListMessage *msg);
void PokeListMessage_Update(PokeListWork *wk, PokeListMessage *msg);
// windowType is a POKELIST_MESSAGE_WINDOW_*
void PokeListMessage_Open(PokeListWork *wk, PokeListMessage *msg, u32 windowType);
void PokeListMessage_Close(PokeListWork *wk, PokeListMessage *msg);
BOOL PokeListMessage_IsOpen(PokeListWork *wk, PokeListMessage *msg);
void PokeListMessage_Print(PokeListWork *wk, PokeListMessage *msg, u32 msgId);
void PokeListMessage_PrintStream(PokeListWork *wk, PokeListMessage *msg, u32 msgId, BOOL waitInput);
BOOL PokeListMessage_IsDone(PokeListWork *wk, PokeListMessage *msg);
// The word set that the messages printed are expanded with, and its words
void PokeListMessage_CreateWordSet(PokeListWork *wk, PokeListMessage *msg);
void PokeListMessage_FreeWordSet(PokeListWork *wk, PokeListMessage *msg);
void PokeListMessage_SetPkmName(PokeListWork *wk, PokeListMessage *msg, u32 index, PartyPkm *pkm);
void PokeListMessage_SetItemName(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 item);
void PokeListMessage_SetItemTextName(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 item);
void PokeListMessage_SetMoveName(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 move);
void PokeListMessage_SetStatName(PokeListWork *wk, PokeListMessage *msg, u32 index, u32 stat);
void PokeListMessage_SetNumber(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 number, u8 digits);
void PokeListMessage_SetString(PokeListWork *wk, PokeListMessage *msg, u32 index, const StrBuf *str, u32 gender);
void PokeListMessage_LoadFrame(PokeListWork *wk, PokeListMessage *msg);
void PokeListMessage_ShowWaitIcon(PokeListWork *wk, PokeListMessage *msg);
void PokeListMessage_DrawKeyCursor(PokeListWork *wk, PokeListMessage *msg);

// plist_menu.c
PokeListMenu *PokeListMenu_Create(PokeListWork *wk);
void PokeListMenu_Free(PokeListWork *wk, PokeListMenu *menu);
void PokeListMenu_Open(PokeListWork *wk, PokeListMenu *menu, const u32 *items);
void PokeListMenu_OpenYesNo(PokeListWork *wk, PokeListMenu *menu);
void PokeListMenu_Close(PokeListWork *wk, PokeListMenu *menu);
void PokeListMenu_Update(PokeListWork *wk, PokeListMenu *menu);
u32 PokeListMenu_GetPicked(PokeListWork *wk, PokeListMenu *menu);
AppTaskMenuWin *PokeListMenu_CreateButton(PokeListWork *wk, PokeListMenu *menu, u32 msgId, u8 x, u8 y, BOOL isBack);
void PokeListMenu_FreeButton(AppTaskMenuWin *button);
void PokeListMenu_UpdateButton(AppTaskMenuWin *button);
void PokeListMenu_SetButtonActive(AppTaskMenuWin *button, BOOL active);
void PokeListMenu_SetButtonPressed(AppTaskMenuWin *button, BOOL pressed);

// plist_item.c
BOOL PokeList_IsItemForMove(PokeListWork *wk, u16 item);
BOOL PokeList_IsItemForParty(PokeListWork *wk, u16 item);
s32 PokeList_FindItemTarget(PokeListWork *wk);
u32 PokeList_GetItemMenuMessage(PokeListWork *wk, u16 item);
void PokeList_UpdateArceusForm(PokeListWork *wk, PartyPkm *pkm, u16 item);
void PokeList_UpdateGenesectForm(PokeListWork *wk, PartyPkm *pkm, u16 item);
void PokeList_ShowItemUselessExit(PokeListWork *wk);
void PokeList_ShowItemUseless(PokeListWork *wk);
void PokeList_ShowItemMessageSelect(PokeListWork *wk, u32 offset);
u32 PokeList_ShowItemResult(PokeListWork *wk, u32 move);
void PokeList_LearnMessageDone(PokeListWork *wk);
void PokeList_UpdateLevelUp(PokeListWork *wk);

// plist_battle.c
void PokeListBattle_Init(PokeListWork *wk);
void PokeListBattle_Exit(PokeListWork *wk);
void PokeListBattle_Update(PokeListWork *wk);
void PokeListBattle_ShowMessage(PokeListWork *wk);

// plist_demo.c
// The form change shown, wk->demo: 1 Giratina's Origin Forme, 2 its Altered Forme, 3 Shaymin's Sky Forme, 4 Kyurem
// fusing, 5 Kyurem separating, 6 the Therian or Incarnate Forme of Tornadus, Thundurus or Landorus
void PokeListDemo_Start(PokeListWork *wk);
void PokeListDemo_End(PokeListWork *wk);
void PokeListDemo_Update(PokeListWork *wk);
BOOL PokeListDemo_CanBecomeOrigin(PokeListWork *wk, PartyPkm *pkm);
void PokeListDemo_SetOrigin(PokeListWork *wk, PartyPkm *pkm);
BOOL PokeListDemo_CanBecomeAltered(PokeListWork *wk, PartyPkm *pkm);
void PokeListDemo_SetAltered(PokeListWork *wk, PartyPkm *pkm);
BOOL PokeListDemo_CanBecomeSky(PokeListWork *wk, PartyPkm *pkm);
void PokeListDemo_SetSky(PokeListWork *wk, PartyPkm *pkm);
// 0 if the DNA Splicers can fuse the Pokémon, 2 if it isn't Kyurem, 3 if it has fainted and 4 if it is fused
u32 PokeListDemo_CheckFuse(PokeListWork *wk, PartyPkm *pkm);
// 1 if the DNA Splicers can separate the Pokémon, 2 if it isn't Kyurem, 4 if it isn't fused and 5 if the party is
// full
u32 PokeListDemo_CheckSeparate(PokeListWork *wk, PartyPkm *pkm);
BOOL PokeListDemo_CanBecomeTherian(PokeListWork *wk, PartyPkm *pkm);
void PokeListDemo_ToggleTherian(PokeListWork *wk, PartyPkm *pkm);

// status_rcv.c
// Whether the item would do anything to the Pokémon, pos being the move slot for items that act on one move
BOOL StatusRcv_CanUseItem(PartyPkm *pkm, u16 item, u16 pos, u32 heapId);
// Uses the item on the Pokémon, and whether it changed anything
BOOL StatusRcv_UseItem(PartyPkm *pkm, u16 item, u16 pos, u16 zoneId, u32 heapId);

#endif // POKEBW2_APP_POKELIST_H
