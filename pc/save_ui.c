#include "save_ui.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "field/event_save.h"
#include "field/player_state.h"
#include "field/report.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/rtc_cache.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "gx_backend.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/playtime.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/app_taskmenu.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "ui_assets.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static GameSystem *owner;
static Font *font;
static u16 location[128];
static PlayTime lastSaved;
static bool hasLastSave, hasDex, pendingContinue;
static u32 seenCount;
static Uint16 pendingKeys;
static s32 pendingChoice = -1;

bool PcSaveUi_IsActive(void) {
    return Report_IsNativeActive();
}

static void LoadDetails(ReportNativeState *state) {
    GameData *game = GSYS_GetGameData(state->gameSystem);
    SaveControl *save = GameData_GetSaveControl(game);
    u16 zone = PlayerState_GetZoneID(GSYS_GetPlayerState(state->gameSystem));
    if (IsZoneJoinAvenue(zone) || IsZoneJoinAvenueSubZone(zone)) {
        JoinAvenueInfo *avenue = JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(save));
        const u16 *name = (const u16 *)JoinAvenue_GetParam(avenue, JOIN_AVE_INFO_NAME, 0);
        u32 i = 0;
        while (i + 1 < SDL_arraysize(location) && name[i] != 0xffff) {
            location[i] = name[i];
            i++;
        }
        location[i] = 0xffff;
    } else {
        MsgData *names = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, HEAPID_USER);
        GFL_MsgDataLoadRawStr(names, ZoneData_GetPlaceNameID(zone), location, SDL_arraysize(location));
        GFL_MsgDataFree(names);
    }
    hasLastSave = !SaveControl_IsNewGameOverSave(save) && SaveControl_IsDataAlreadyPresent(save);
    lastSaved = *SaveControl_GetPlayTime(save);
    hasDex = EventWork_FlagGet(GameData_GetEventWork(game), 0x962);
    seenCount = hasDex ? countSeenDexPokes(GameData_GetPokedex(game), HEAPID_USER) : 0;
}

static void Sidebar(GameData *game) {
    char text[64];
    RTCDate date;
    RTCTime time;
    RTC_GetCachedDateTime(&date, &time);
    PcUi_RoleCenteredText(font, GetPlayerName(GetGameDataPlayerInfo(game)), 54, 150, 368, 194, PC_UI_DEFAULT_COLOR,
                          PC_UI_TEXT_HERO_VALUE);
    PcUi_LineCenteredLabel(font, "YOUR ADVENTURE", 54, 195, 368, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    SDL_snprintf(text, sizeof(text), "%04u-%02u-%02u  %02u:%02u", date.year + 2000, date.month, date.day, time.hour,
                 time.minute);
    PcUi_LineCenteredLabel(font, text, 54, 234, 368, PC_UI_WHITE, PC_UI_TEXT_STAT_LABEL);
    PcUi_LineLabel(font, "CURRENT LOCATION", 58, 280, 367, PC_UI_CYAN, PC_UI_TEXT_STAT_LABEL);
    PcUi_RoleParagraph(font, location, 58, 312, 367, 375, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "POKÉMON PARTY", 58, 393, 367, PC_UI_CYAN, PC_UI_TEXT_STAT_LABEL);
    PokeParty *party = GameData_GetParty(game);
    for (int i = 0; i < PARTY_SIZE; i++) {
        float x = 58 + (i % 3) * 104, y = 421 + (i / 3) * 100;
        if (i < PokeParty_GetPkmCount(party)) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);
            PcUi_PokemonIcon(pkm, i, x + 13, y, 64);
            if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL))
                SDL_strlcpy(text, "Egg", sizeof(text));
            else
                SDL_snprintf(text, sizeof(text), "Lv. %u", PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL));
            PcUi_LineCenteredLabel(font, text, x, y + 66, x + 90, PC_UI_WHITE, PC_UI_TEXT_CAPTION);
        } else {
            PcUi_LineCenteredLabel(font, "—", x, y + 28, x + 90, PC_UI_DISABLED, PC_UI_TEXT_STAT_VALUE);
        }
    }
}

static void Summary(GameData *game, bool saved) {
    char text[64];
    PcUi_LineLabel(font, "PROGRESS SUMMARY", 453, 177, 1200, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    PlayTime *time = GameData_GetPlayTime(game);
    SDL_snprintf(text, sizeof(text), "%u:%02u", PlayTime_GetHours(time), PlayTime_GetMinutes(time));
    PcUi_StatCard(font, "PLAY TIME", text, (PcUiRect){ 464, 222, 338, 68 });
    SDL_snprintf(text, sizeof(text), "%u / 8", getBadgeCount(getTrainerCardDataBlkAddress(game)));
    PcUi_StatCard(font, "GYM BADGES", text, (PcUiRect){ 858, 222, 362, 68 });
    if (hasDex)
        SDL_snprintf(text, sizeof(text), "%u", seenCount);
    else
        SDL_strlcpy(text, "—", sizeof(text));
    PcUi_StatCard(font, "POKÉDEX SEEN", text, (PcUiRect){ 464, 314, 338, 68 });
    PcUi_LineLabel(font, "LAST SAVED", 858, 314, 1220, PC_UI_INK_MUTED, PC_UI_TEXT_LABEL);
    PlayTime *stamp = saved ? SaveControl_GetPlayTime(GameData_GetSaveControl(game)) : &lastSaved;
    if (hasLastSave || saved) {
        SDL_snprintf(text, sizeof(text), "%04u-%02u-%02u", PlayTime_GetLastSavedYear(stamp) + 2000,
                     PlayTime_GetLastSavedMonth(stamp), PlayTime_GetLastSavedDay(stamp));
        PcUi_LineLabel(font, text, 858, 341, 1220, PC_UI_INK, PC_UI_TEXT_VALUE);
        SDL_snprintf(text, sizeof(text), "%02u:%02u", PlayTime_GetLastSavedHour(stamp),
                     PlayTime_GetLastSavedMinute(stamp));
        PcUi_LineLabel(font, text, 858, 374, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_STAT_LABEL);
    } else {
        PcUi_LineLabel(font, "Not saved yet", 858, 345, 1220, PC_UI_INK, PC_UI_TEXT_BODY);
    }
}

static void Button(const char *text, float x, float width, bool selected, bool enabled, PcUiAction action) {
    PcUi_CenteredButtonLabel(font, text, (PcUiRect){ x, 579, width, 36 }, selected, enabled, action);
}

bool PcSaveUi_Draw(void) {
    ReportNativeState state;
    if (!Report_GetNativeState(&state)) {
        owner = NULL;
        return false;
    }
    if (owner != state.gameSystem) {
        owner = state.gameSystem;
        LoadDetails(&state);
    }
    PcUi_BeginControls(state.gameSystem, state.phase);
    font = state.font;
    PcGx.vertex_count = 0;
    PcGx.composite_scene = false;
    if (!PcUi_Begin()) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Unable to load the bundled save-screen font", sizeof(PcGx.error));
        return true;
    }
    PcMenuSkin_Add(PC_MENU_HEADER, 0, 0, PC_UI_WIDTH, PC_UI_HEADER_HEIGHT, 0);
    PcMenuSkin_Add(PC_MENU_SAVE, 0, PC_UI_HEADER_HEIGHT, PC_UI_WIDTH, PC_UI_BODY_HEIGHT, 0);
    PcMenuSkin_Add(PC_MENU_FOOTER, 0, PC_UI_FOOTER_TOP, PC_UI_WIDTH, PC_UI_FOOTER_HEIGHT, 0);
    PcUiFrame_Header(font, "SAVE", "ADVENTURE");
    PcUi_LineLabel(font, "SAVE YOUR PROGRESS", 443, 111, 1220, PC_UI_CYAN, PC_UI_TEXT_SECTION);
    bool writing = state.phase == REPORT_NATIVE_WRITING;
    bool saved = state.phase == REPORT_NATIVE_SAVED;
    GameData *game = GSYS_GetGameData(state.gameSystem);
    Sidebar(game);
    Summary(game, saved);
    const char *heading = state.phase == REPORT_NATIVE_OPENING   ? "GETTING READY"
                          : state.phase == REPORT_NATIVE_CONFIRM ? "READY TO SAVE"
                          : writing                              ? "SAVING YOUR ADVENTURE"
                          : saved                                ? "GAME SAVED"
                          : state.phase == REPORT_NATIVE_BLOCKED ? "SAVE UNAVAILABLE"
                                                                 : "COULDN'T SAVE";
    u32 color = state.phase == REPORT_NATIVE_ERROR || state.phase == REPORT_NATIVE_BLOCKED ? 0xff4559 : PC_UI_CYAN;
    PcUi_LineLabel(font, heading, 465, 455, 1218, color, PC_UI_TEXT_BODY);
    if (writing) {
        PcUi_LineLabel(font, "Please wait until saving is complete.", 465, 502, 1218, PC_UI_DEFAULT_COLOR,
                       PC_UI_TEXT_BODY);
        u32 progress = Report_GetNativeProgress();
        for (u32 i = 0; i < 10; i++)
            PcMenuSkin_Add(PC_MENU_SAVE_PROGRESS, 465 + i * 75, 580, 64, 14, i < progress);
    } else {
        if (state.phase == REPORT_NATIVE_BLOCKED) {
            PcUi_LineLabel(font, "A saved adventure already exists.", 465, 497, 1218, PC_UI_DEFAULT_COLOR,
                           PC_UI_TEXT_BODY);
            PcUi_LineLabel(font, "This new game cannot overwrite it.", 465, 530, 1218, PC_UI_DEFAULT_COLOR,
                           PC_UI_TEXT_BODY);
        } else {
            PcUi_RoleParagraph(font, state.message, 465, 497, 1218, 560, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        }
        if (state.phase == REPORT_NATIVE_CONFIRM) {
            AppTaskMenuNativeState menu;
            bool available = AppTaskMenu_GetNativeState(state.menu, &menu);
            Button("SAVE GAME", 462, 355, available && menu.cursor == 0, available && menu.ready,
                   (PcUiAction){ PC_UI_ACTION_MENU, 0 });
            Button("CANCEL", 850, 368, available && menu.cursor == 1, available && menu.ready,
                   (PcUiAction){ PC_UI_ACTION_MENU, 1 });
        } else if (saved || state.phase == REPORT_NATIVE_ERROR || state.phase == REPORT_NATIVE_BLOCKED) {
            Button("CONTINUE", 850, 368, true, true, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
        }
    }
    PcUiFrame_Footer(font, writing ? "Saving in progress" : "Arrows: Choose   Z: Confirm   X: Back", "Select", false,
                     !writing);
    return true;
}

Uint16 PcSaveUi_FilterKeys(Uint16 keys) {
    ReportNativeState state;
    if (!Report_GetNativeState(&state)) {
        pendingKeys = 0;
        pendingChoice = -1;
        pendingContinue = false;
        owner = NULL;
        return keys;
    }
    keys |= pendingKeys;
    pendingKeys = 0;
    AppTaskMenuNativeState menu;
    if (pendingChoice >= 0 && AppTaskMenu_GetNativeState(state.menu, &menu) && menu.ready) {
        AppTaskMenu_SetCursorPos(state.menu, pendingChoice);
        keys |= PAD_BUTTON_A;
    }
    pendingChoice = -1;
    if (keys & PAD_KEY_LEFT)
        keys |= PAD_KEY_UP;
    if (keys & PAD_KEY_RIGHT)
        keys |= PAD_KEY_DOWN;
    GCTX_HIDSetUsingTouch(FALSE);
    if (state.phase == REPORT_NATIVE_WRITING || state.phase == REPORT_NATIVE_OPENING)
        return 0;
    if (state.phase == REPORT_NATIVE_SAVED || state.phase == REPORT_NATIVE_ERROR ||
        state.phase == REPORT_NATIVE_BLOCKED) {
        pendingContinue |= (keys & (PAD_BUTTON_A | PAD_BUTTON_B)) != 0;
        if (state.ready && pendingContinue) {
            pendingContinue = false;
            return PAD_BUTTON_A;
        }
        return 0;
    }
    return keys & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_BUTTON_A | PAD_BUTTON_B);
}

bool PcSaveUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    if (!PcSaveUi_IsActive())
        return false;
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (!event->key.repeat) {
            switch (event->key.scancode) {
            case SDL_SCANCODE_Z:
            case SDL_SCANCODE_RETURN:
                pendingKeys |= PAD_BUTTON_A;
                break;
            case SDL_SCANCODE_X:
            case SDL_SCANCODE_ESCAPE:
                pendingKeys |= PAD_BUTTON_B;
                break;
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_LEFT:
                pendingKeys |= PAD_KEY_UP;
                break;
            case SDL_SCANCODE_DOWN:
            case SDL_SCANCODE_RIGHT:
                pendingKeys |= PAD_KEY_DOWN;
                break;
            default:
                break;
            }
        }
        return true;
    }
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT)
        return false;
    ReportNativeState state;
    Report_GetNativeState(&state);
    PcUiAction action = PcUi_PointerAction(video, event, state.gameSystem, state.phase);
    if (action.kind == PC_UI_ACTION_KEY)
        pendingKeys |= action.value;
    else if (action.kind == PC_UI_ACTION_MENU)
        pendingChoice = action.value;
    return true;
}
