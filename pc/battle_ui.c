#include "battle_ui.h"
#include "battle/battle_proc.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btlv.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_gauge.h"
#include "battle/btlv_input.h"
#include "battle_menu.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gx_backend.h"
#include "pml/poke_party.h"
#include "pml/move.h"
#include "ui_assets.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static BtlvCore *owner;
static MsgData *moveNames;
static Uint16 pendingKeys, previousKeys;
static int cursor, pendingChoice = -1;
static u32 previousScreen, heldFrames;

static bool Current(BtlvNativeState *state) {
    return GCTX_ProcMgrCurrentFunctions() == &BATTLE_PROC_FUNCTIONS && BtlvCore_GetNativeState(state);
}

bool PcBattleUi_IsActive(void) {
    BtlvNativeState state;
    return Current(&state);
}

static bool Commands(u32 screen) {
    return screen == 1 || screen == 2;
}

static bool ChoiceExists(const BtlvNativeState *state, int choice) {
    return choice >= 4 || (state->mon && choice >= 0 && (u32)choice < GetBattleMonMoveCount(state->mon));
}

static void Reset(const BtlvNativeState *state) {
    if (owner == state->core)
        return;
    owner = state->core;
    cursor = 0;
    pendingChoice = -1;
    previousKeys = pendingKeys = 0;
    previousScreen = heldFrames = 0;
}

static void Submit(const BtlvNativeState *state, const BtlvInputNativeState *input) {
    if (pendingChoice < 0 || !input->ready || !Commands(input->screen))
        return;
    if (pendingChoice < 4) {
        if (input->screen == 1) {
            if (!BtlvInput_PressNativeButton(state->input, 1, 0))
                pendingChoice = -1;
        } else {
            BtlvInput_PressNativeButton(state->input, 2, pendingChoice);
            pendingChoice = -1;
        }
    } else if (input->screen == 2) {
        if (BtlvInput_PressNativeButton(state->input, 2, 4))
            pendingChoice = -1;
    } else {
        BtlvInput_PressNativeButton(state->input, 1, pendingChoice - 3);
        pendingChoice = -1;
    }
}

Uint16 PcBattleUi_FilterKeys(Uint16 keys) {
    keys = PcBattleMenu_FilterKeys(keys);
    if (PcBattleMenu_IsActive())
        return keys;
    BtlvNativeState state;
    BtlvInputNativeState input;
    if (!Current(&state)) {
        owner = NULL;
        pendingChoice = -1;
        previousKeys = pendingKeys = 0;
        return keys;
    }
    Reset(&state);
    keys |= pendingKeys;
    pendingKeys = 0;
    if (keys & PAD_BUTTON_START)
        keys = (keys & ~PAD_BUTTON_START) | PAD_BUTTON_A;
    if (!BtlvInput_GetNativeState(state.input, &input))
        return keys;
    u16 pressed = keys & ~previousKeys;
    u16 directions = keys & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT);
    if (directions != (previousKeys & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT)))
        heldFrames = 0;
    previousKeys = keys;
    if (input.screen != previousScreen && (input.screen == 1 || !Commands(input.screen))) {
        pendingChoice = -1;
        cursor = 0;
    }
    previousScreen = input.screen;
    if (Commands(input.screen)) {
        if (!ChoiceExists(&state, cursor))
            cursor = 4;
        if (directions && (heldFrames == 0 || (heldFrames >= 20 && heldFrames % 5 == 0))) {
            do {
                if (directions & PAD_KEY_RIGHT)
                    cursor = cursor >= 4 ? (cursor == 6 ? 0 : cursor + 1) : (cursor & 1 ? 4 : cursor + 1);
                else if (directions & PAD_KEY_LEFT)
                    cursor = cursor >= 4 ? (cursor == 4 ? 1 : cursor - 1) : (cursor & 1 ? cursor - 1 : 6);
                else if (directions & PAD_KEY_DOWN)
                    cursor = cursor >= 4 ? 0 : (cursor < 2 ? cursor + 2 : 4);
                else
                    cursor = cursor >= 4 ? 2 : (cursor >= 2 ? cursor - 2 : 4);
            } while (!ChoiceExists(&state, cursor));
        }
        if ((pressed & PAD_BUTTON_A) && pendingChoice < 0 && input.ready)
            pendingChoice = cursor;
        if (pressed & PAD_BUTTON_B) {
            pendingChoice = -1;
            cursor = 0;
            if (input.screen == 2)
                BtlvInput_PressNativeButton(state.input, 2, 4);
        }
        Submit(&state, &input);
        heldFrames++;
        return keys & ~(PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT | PAD_BUTTON_A | PAD_BUTTON_B);
    }
    return keys;
}

bool PcBattleUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    if (PcBattleMenu_IsActive())
        return PcBattleMenu_HandleEvent(video, event);
    BtlvNativeState state;
    BtlvInputNativeState input;
    if (!Current(&state))
        return false;
    Reset(&state);
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_MOUSE_WHEEL) {
        pendingKeys |= PcUi_EventKey(event);
        return true;
    }
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT)
        return false;
    if (!BtlvInput_GetNativeState(state.input, &input))
        return true;
    PcUiAction action = PcUi_PointerAction(video, event, state.core, input.screen);
    if (action.kind == PC_UI_ACTION_KEY)
        pendingKeys |= action.value;
    else if (action.kind == PC_UI_ACTION_MENU && Commands(input.screen) && ChoiceExists(&state, action.value)) {
        cursor = pendingChoice = action.value;
    } else if (action.kind == PC_UI_ACTION_ROW && input.screen == 4)
        BtlvInput_PressNativeButton(state.input, 4, action.value);
    return true;
}

static void Footer(Font *font, bool choosing) {
    float right = 1252;
    if (!choosing) {
        PcUiFrame_KeyHint(font, "Enter", "Continue", right, PC_UI_FOOTER_KEY_CENTER, 54, PAD_BUTTON_A);
        return;
    }
    right =
        PcUiFrame_KeyHint(font, "Esc", "Back", right, PC_UI_FOOTER_KEY_CENTER, 42, PAD_BUTTON_B) - PC_UI_KEY_GROUP_GAP;
    right = PcUiFrame_KeyHint(font, "Enter", "Confirm", right, PC_UI_FOOTER_KEY_CENTER, 54, PAD_BUTTON_A) -
            PC_UI_KEY_GROUP_GAP;
    PcUiFrame_DirectionHint(font, "Select", right, PC_UI_FOOTER_KEY_CENTER);
}

static void Message(const BtlvNativeState *state, bool choosing, bool decision) {
    u16 text[1024];
    u32 out = 0;
    for (u32 i = 0; i < 1023 && state->message[i] != 0xffff && out + 7 < SDL_arraysize(text); i++) {
        // The DS font's paired PK/MN ligature becomes the full name in the native font.
        if (state->message[i] == 0x2486 && state->message[i + 1] == 0x2487) {
            const u16 name[] = { 'P', 'o', 'k', 0xe9, 'm', 'o', 'n' };
            for (u32 j = 0; j < SDL_arraysize(name); j++)
                text[out++] = name[j];
            i++;
        } else {
            text[out++] = state->message[i];
        }
    }
    text[out] = 0xffff;
    if (choosing) {
        PcUiTextFit fit = PcUi_FitText(state->font, text, PC_UI_TEXT_VALUE, 534);
        PcUi_RoleCenteredText(state->font, text, 46, 518 - PC_UI_FONT_CELL / 2 * fit.scale, 580, 556,
                              PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    } else {
        PcUi_RoleParagraph(state->font, text, 55, 508,
                           state->statsMode ? 568
                           : decision       ? 770
                                            : 1210,
                           625, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    }
}

static void DrawGauge(const BtlvNativeState *state, u32 pos) {
    BtlvGaugeNativeState gauge;
    if (!BtlvEffect_GetWork() || !BtlvGauge_GetNativeState(BtlvEffect_GetGauge(), pos, &gauge))
        return;
    BattleMon *mon = BtlvCore_GetMonAtViewPos(state->core, pos);
    if (!mon)
        return;
    bool player = !(pos & 1);
    float x = player ? 810 : 0, y = player ? 330 : 54;
    PcMenuSkin_Add(player ? PC_BATTLE_PLAYER_HP : PC_BATTLE_ENEMY_HP, x, y, player ? 470 : 486, player ? 115 : 76, 0);
    u16 name[32];
    PokeParty_GetParam(BattleMon_GetViewSrcData(mon), PKM_PARAM_NICKNAME_RAW, name);
    float nameRight = x + 87 + (gauge.sex < 2 ? 204 : 236);
    PcUiTextFit nameFit = PcUi_FitText(state->font, name, PC_UI_TEXT_CARD_TITLE, nameRight - x - 87);
    PcUi_RoleText(state->font, name, x + 87, y + 34 - PC_UI_FONT_BASELINE * nameFit.scale, nameRight, y + 40,
                  PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CARD_TITLE);
    if (gauge.sex < 2) {
        float signX = x + 87 + nameFit.width + 6;
        PcUi_BattleGender(gauge.sex, signX, y + 13, 20);
    }
    PcUi_LineLabel(state->font, "Lv.", x + 334, y + 34 - PC_UI_FONT_BASELINE * PcUi_Style(PC_UI_TEXT_LABEL)->scale,
                   x + 369, PC_UI_WARNING, PC_UI_TEXT_LABEL);
    char text[64];
    SDL_snprintf(text, sizeof(text), "%u", gauge.level);
    PcUi_LineLabel(state->font, text, x + 374, y + 34 - PC_UI_FONT_BASELINE * PcUi_Style(PC_UI_TEXT_CARD_TITLE)->scale,
                   x + 431, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CARD_TITLE);
    float barX = x + (player ? 206 : 198), barY = y + (player ? 45 : 46);
    float fill = gauge.maxHp ? SDL_clamp((float)gauge.hp / gauge.maxHp, 0, 1) : 0;
    u32 color = fill > .5f ? 0x40d52b : fill > .2f ? 0xf3c840 : 0xef4f45;
    PcUi_Solid(barX, barY, (player ? 195 : 207) * fill, player ? 14 : 13, color, 1);
    float hpWidth = PcUi_RoleLabelWidth(state->font, "HP", PC_UI_TEXT_BODY);
    PcUi_LineLabel(state->font, "HP", barX - 12 - hpWidth,
                   barY + (player ? 7 : 6.5f) - PC_UI_FONT_CELL / 2 * PcUi_Style(PC_UI_TEXT_BODY)->scale, barX - 10,
                   PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    if (player) {
        SDL_snprintf(text, sizeof(text), "%d / %d", gauge.hp, gauge.maxHp);
        float width = PcUi_RoleLabelWidth(state->font, text, PC_UI_TEXT_VALUE);
        PcUi_LineLabel(state->font, text, x + 403 - width, y + 62, x + 405, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
        float expWidth = PcUi_RoleLabelWidth(state->font, "EXP", PC_UI_TEXT_LABEL);
        PcUi_LineLabel(state->font, "EXP", x + 111 - expWidth,
                       y + 100.5f - PC_UI_FONT_CELL / 2 * PcUi_Style(PC_UI_TEXT_LABEL)->scale, x + 113, 0x31b4e7,
                       PC_UI_TEXT_LABEL);
        float exp = gauge.expMax ? SDL_clamp((float)gauge.exp / gauge.expMax, 0, 1) : 0;
        PcUi_Solid(x + 123, y + 97, 280 * exp, 7, 0x29b8e8, 1);
    }
}

bool PcBattleUi_Draw(void) {
    if (PcBattleMenu_Draw())
        return true;
    BtlvNativeState state;
    BtlvInputNativeState input = { 0 };
    if (!Current(&state) || !PcUi_Begin())
        return false;
    Reset(&state);
    PcGx.battle_ui = true;
    if (!moveNames)
        moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_SYSTEM);
    BtlvInput_GetNativeState(state.input, &input);
    PcUi_BeginControls(state.core, input.screen);
    bool choosing = Commands(input.screen);
    PcMenuSkin_Add(choosing ? PC_BATTLE_PANEL : PC_BATTLE_NOTICE, 0, 460, 1280, 260, 0);
    DrawGauge(&state, 1);
    DrawGauge(&state, 0);
    Message(&state, choosing, input.screen == 4);
    if (!choosing) {
        if (input.screen == 4) {
            for (u32 i = 0; i < 2; i++) {
                PcUi_SkinControl(PC_MENU_BUTTON, (PcUiRect){ 820, 496 + i * 64, 410, 56 }, input.cursor == i,
                                 (PcUiAction){ PC_UI_ACTION_ROW, (int)i });
                PcUi_RoleCenteredText(state.font, input.choices[i], 842, 509 + i * 64, 1200, 550 + i * 64,
                                      input.cursor == i ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_VALUE);
            }
            Footer(state.font, true);
            return true;
        }
        PcUi_Control((PcUiShape){ .rect = { 0, 460, 1280, 190 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
        if (state.statsMode) {
            const char *names[] = { "HP", "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed" };
            const u16 values[] = { state.stats.hp,       state.stats.attack,    state.stats.defense,
                                   state.stats.spAttack, state.stats.spDefense, state.stats.speed };
            for (u32 i = 0; i < SDL_arraysize(values); i++) {
                float x = 656 + (i / 3) * 280, y = 499 + (i % 3) * 44;
                char value[16];
                SDL_snprintf(value, sizeof(value), "%s%u", state.statsMode == 1 ? "+" : "", values[i]);
                PcUi_StatRow(state.font, names[i], value, (PcUiRect){ x, y, 264, 32 }, false);
            }
        }
        Footer(state.font, false);
        return true;
    }
    const char *actions[] = { "Bag", "Pokémon", "Run" };
    const float actionX[] = { 22, 195, 410 }, actionW[] = { 160, 203, 156 };
    for (u32 i = 0; i < 3; i++) {
        PcUi_SkinControl(PC_BATTLE_ACTION, (PcUiRect){ actionX[i], 576, actionW[i], 72 }, cursor == (int)i + 4,
                         (PcUiAction){ PC_UI_ACTION_MENU, (int)i + 4 });
        PcUiTextFit fit = PcUi_FitLabel(state.font, actions[i], PC_UI_TEXT_CARD_TITLE, actionW[i] - 88);
        float left = actionX[i] + (actionW[i] - 56 - fit.width) * 0.5f;
        PcMenuSkin_Add(PC_BATTLE_ICON, left, 591, 42, 42, 17 + i);
        PcUi_RoleLabel(state.font, actions[i], left + 56, 612 - PC_UI_FONT_CELL / 2 * fit.scale, left + 56 + fit.width,
                       640, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CARD_TITLE);
    }
    for (u32 i = 0; i < 4; i++) {
        float x = 647 + (i & 1) * 312, y = 475 + (i >> 1) * 84;
        bool selected = Commands(input.screen) && cursor == (int)i;
        PcUi_SkinControl(PC_BATTLE_MOVE, (PcUiRect){ x, y, 304, 80 }, selected,
                         (PcUiAction){ ChoiceExists(&state, i) ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, (int)i });
        u8 pp = 0, maxPP = 0;
        u16 move =
            state.mon && i < GetBattleMonMoveCount(state.mon) ? BattleMon_GetMoveInfo(state.mon, i, &pp, &maxPP) : 0;
        if (!move) {
            PcUi_RoleCenteredLabel(state.font, "—", x + 20, y + 27, x + 284, y + 70, PC_UI_INK_MUTED,
                                   PC_UI_TEXT_CARD_TITLE);
            continue;
        }
        u16 name[128];
        GFL_MsgDataLoadRawStr(moveNames, move, name, SDL_arraysize(name));
        PcUi_RoleText(state.font, name, x + 65, y + 13, x + 288, y + 44, pp ? PC_UI_WHITE : PC_UI_MUTED,
                      PC_UI_TEXT_VALUE);
        u32 type = PML_MoveGetParam(move, MOVE_PARAM_TYPE);
        PcMenuSkin_Add(PC_BATTLE_ICON, x + 15, y + 19, 42, 42, type);
        PcUi_TypeBadge(type, i, 2, x + 65, y + 46, 76, 23);
        char text[48];
        SDL_snprintf(text, sizeof(text), "PP %u/%u", pp, maxPP);
        PcUi_LineLabel(state.font, text, x + 156, y + 45, x + 290, pp ? PC_UI_WHITE : PC_UI_DANGER, PC_UI_TEXT_BODY);
        if (selected)
            PcMenuSkin_Add(PC_BATTLE_CURSOR, x - 22, y + 18, 32, 44, 0);
    }
    Footer(state.font, true);
    return true;
}
