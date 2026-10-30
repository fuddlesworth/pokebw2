#include "trainer_ui.h"
#include "app/pms_select.h"
#include "app/trainer_card.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "gx_backend.h"
#include "save/playtime.h"
#include "save/timesig_savedata.h"
#include "system/app_taskmenu.h"
#include "system/game_data.h"
#include "system/pms_data.h"
#include "system/union_view.h"
#include "system/wordset.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"

// The trainer card keeps its original process and save data. Only its presentation and pointer coordinates change.
enum { PAGE_PROFILE, PAGE_RECORDS, PAGE_BADGES, PAGE_SIGNATURE, PAGE_COUNT };
enum { BADGE_WIDTH = 32, BADGE_HEIGHT = 64 };
static const PcUiRect signatureRect = { 530, 210, 576, 192 };
enum { ACTION_PROFILE = 1, ACTION_PEN, ACTION_ERASER, ACTION_CLEAR, ACTION_UNDO };
static const char *const tabs[] = { "PROFILE", "RECORDS", "BADGES", "SIGNATURE" };
static const char *const badgeNames[] = { "BASIC", "TOXIC", "INSECT", "BOLT", "QUAKE", "JET", "LEGEND", "WAVE" };
static TrainerCardWork *owner;
static Font *font;
static u16 pictures[256 * 256], badgePixels[8][BADGE_WIDTH * BADGE_HEIGHT];
static bool badgesLoaded, erase, signing;
static u8 undoSign[TRCARD_SIGN_PIXELS];
static bool canUndo;
static u32 page, recordCursor, recordStart, badgeCursor, profileCursor;
static Uint16 pendingKeys, previousKeys;
static s32 pendingPage = -1, pendingRow = -1, pendingMenu = -1;
static u16 className[128], personality[128], greeting[256];
static u32 classId = ~0u, personalityId = ~0u;
static PMSData greetingData;
static bool greetingLoaded;

bool PcTrainerUi_IsActive(void) {
    return TrainerCard_GetNativeWork() || PMSSelect_IsNativeActive();
}

static void Frame(PcMenuSkinAsset body, const char *section, const char *hint) {
    PcUiFrame_Draw(font, body, "TRAINER", section, hint);
}

static void Button(const char *text, float x, float y, float width, bool selected, int action) {
    PcUi_CenteredButtonLabel(font, text, (PcUiRect){ x, y, width, 36 }, selected, true,
                             (PcUiAction){ action ? PC_UI_ACTION_CUSTOM : PC_UI_ACTION_NONE, action });
}

static void LoadBadges(void) {
    if (badgesLoaded)
        return;
    NNSG2dCharacterData *chars;
    NNSG2dPaletteData *palette;
    NNSG2dCellDataBank *cells;
    NNSG2dAnimBankData *anims;
    void *charFile = GFL_G2DIOReadOBJNCGR(ARCID_TRAINER_CARD_GRA, 31, FALSE, &chars, HEAPID_USER);
    void *paletteFile = GFL_G2DIOReadNCLR(ARCID_TRAINER_CARD_GRA, 4, &palette, HEAPID_USER);
    void *cellFile = GFL_G2DIOReadNCER(ARCID_TRAINER_CARD_GRA, 52, FALSE, &cells, HEAPID_USER);
    void *animFile = GFL_G2DIOReadNANR(ARCID_TRAINER_CARD_GRA, 54, FALSE, &anims, HEAPID_USER);
    if (charFile && paletteFile && cellFile && animFile) {
        const u8 *tiles = chars->pRawData;
        const u16 *colors = palette->pRawData;
        for (u32 badge = 0; badge < 8; badge++) {
            const NNSG2dAnimSequenceData *sequence = NNS_G2dGetAnimSequenceByIdx(anims, badge);
            if (!sequence || !sequence->numFrames)
                continue;
            const NNSG2dCellData *cell =
                NNS_G2dGetCellDataByIdx(cells, *(const u16 *)sequence->pAnmFrameArray[0].pContent);
            for (u32 i = 0; cell && i < cell->numOAMAttrs; i++) {
                GXOamAttr oam;
                NNS_G2dCopyCellAsOamAttr(cell, i, &oam);
                GXOamShape shape = (GXOamShape)(oam.attr0 | ((u32)oam.attr1 << 16));
                int width = NNS_G2dGetOamSizeX(&shape), height = NNS_G2dGetOamSizeY(&shape);
                int ox = oam.attr1 & 511, oy = oam.attr0 & 255;
                if (ox >= 256)
                    ox -= 512;
                if (oy >= 128)
                    oy -= 256;
                u32 base = (oam.attr2 & 1023) * (32u << cells->mappingMode);
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int dx = BADGE_WIDTH / 2 + ox + x, dy = BADGE_HEIGHT / 2 + oy + y;
                        if (dx < 0 || dx >= BADGE_WIDTH || dy < 0 || dy >= BADGE_HEIGHT)
                            continue;
                        int sx = oam.attr1 & 0x1000 ? width - 1 - x : x;
                        int sy = oam.attr1 & 0x2000 ? height - 1 - y : y;
                        u32 offset = base + ((sy / 8) * (width / 8) + sx / 8) * 32 + (sy & 7) * 4 + (sx & 7) / 2;
                        if (offset >= chars->szByte)
                            continue;
                        u32 index = (tiles[offset] >> ((sx & 1) * 4)) & 15;
                        u32 color = (oam.attr2 >> 12) * 16 + index;
                        if (index && color * 2 < palette->szByte)
                            badgePixels[badge][dy * BADGE_WIDTH + dx] = colors[color] | 0x8000;
                    }
                }
            }
        }
        badgesLoaded = true;
    }
    if (charFile)
        GFL_HeapFree(charFile);
    if (paletteFile)
        GFL_HeapFree(paletteFile);
    if (cellFile)
        GFL_HeapFree(cellFile);
    if (animFile)
        GFL_HeapFree(animFile);
}

static void LoadPortrait(TrainerCardWork *wk) {
    SDL_zero(pictures);
    const u8 *tiles = wk->charData->pRawData;
    const u16 *screen = (const u16 *)wk->scrData->rawData;
    const u16 *colors = wk->palData->pRawData;
    for (u32 y = 0; y < 88; y++) {
        for (u32 x = 0; x < 80; x++) {
            u32 sx = x + 168, sy = y + 16;
            u16 entry = screen[(sy / 8) * (wk->scrData->screenWidth / 8) + sx / 8];
            u32 tx = entry & 0x400 ? 7 - (sx & 7) : sx & 7;
            u32 ty = entry & 0x800 ? 7 - (sy & 7) : sy & 7;
            bool indexed256 = wk->charData->pixelFmt == GX_TEXFMT_PLTT256;
            u32 offset = indexed256 ? (entry & 1023) * 64 + ty * 8 + tx : (entry & 1023) * 32 + ty * 4 + tx / 2;
            if (offset >= wk->charData->szByte)
                continue;
            u32 index = indexed256 ? tiles[offset] : (tiles[offset] >> ((tx & 1) * 4)) & 15;
            u32 color = (entry >> 12) * (indexed256 ? 256 : 16) + index;
            if (index && color * 2 < wk->palData->szByte)
                pictures[y * 256 + x] = colors[color] | 0x8000;
        }
    }
    LoadBadges();
    for (u32 badge = 0; badge < 8; badge++)
        for (u32 y = 0; y < BADGE_HEIGHT; y++)
            SDL_memcpy(pictures + (96 + y) * 256 + badge * BADGE_WIDTH, badgePixels[badge] + y * BADGE_WIDTH,
                       BADGE_WIDTH * sizeof(u16));
}

static void Picture(float x, float y, float width, float height, u32 u, u32 v, u32 sw, u32 sh, float tint) {
    u32 texture = (7u << 26) | (5u << 23) | (5u << 20);
    PcUi_Quad(x, y, width, height, -0.45f, tint, tint, tint, texture, u, v, sw, sh);
}

static void RefreshText(TrainerCardWork *wk) {
    u32 currentClass = TimeSigSave_IsUnionViewChanged(getTimeSigSaveBlock(wk->param->gameData))
                           ? UnionView_GetClassNameId(wk->data->unionView)
                           : 0;
    if (classId != currentClass) {
        MsgData *bank = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TRAINER_CLASS_NAMES, HEAPID_USER);
        GFL_MsgDataLoadRawStr(bank, currentClass, className, SDL_arraysize(className));
        GFL_MsgDataFree(bank);
        classId = currentClass;
    }
    if (personalityId != wk->data->personality) {
        GFL_MsgDataLoadRawStr(wk->msgData, 35 + wk->data->personality, personality, SDL_arraysize(personality));
        personalityId = wk->data->personality;
    }
    if (!greetingLoaded || !PMSData_Equals(&greetingData, &wk->data->greeting)) {
        StrBuf *text = PMSData_ToString(&wk->data->greeting, HEAPID_USER);
        GFL_StrBufStoreString(text, greeting, SDL_arraysize(greeting));
        GFL_StrBufFree(text);
        greetingData = wk->data->greeting;
        greetingLoaded = true;
    }
}

static void Sidebar(TrainerCardWork *wk) {
    char text[80];
    PcUi_RoleCenteredText(font, wk->data->name, 54, 150, 368, 194, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_HERO_VALUE);
    PcUi_LineCenteredLabel(font, "POKÉMON TRAINER", 54, 195, 368, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    Picture(91, 235, 240, 264, 0, 0, 80, 88, 1);
    SDL_snprintf(text, sizeof(text), "ID No. %05u", wk->data->trainerId);
    PcUi_LineCenteredLabel(font, text, 54, 532, 368, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    u32 count = 0;
    for (u32 i = 0; i < 8; i++)
        count += (wk->data->badges >> i) & 1;
    SDL_snprintf(text, sizeof(text), "%u / 8 BADGES", count);
    PcUi_LineCenteredLabel(font, text, 54, 571, 368, PC_UI_CYAN, PC_UI_TEXT_BODY);
    SDL_snprintf(text, sizeof(text), "%u / 5 CARD STARS", wk->data->stars);
    PcUi_LineCenteredLabel(font, text, 54, 603, 368, PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
}

static void Profile(TrainerCardWork *wk) {
    char text[80];
    TrainerCardData *data = wk->data;
    PcUi_LineLabel(font, "TRAINER PROFILE", 453, 177, 1200, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    SDL_snprintf(text, sizeof(text), "$%u", data->money);
    PcUi_StatCard(font, "MONEY", text, (PcUiRect){ 464, 222, 338, 68 });
    if (data->hasPokedex)
        SDL_snprintf(text, sizeof(text), "%u", data->seenCount);
    else
        SDL_strlcpy(text, "—", sizeof(text));
    PcUi_StatCard(font, "POKÉDEX SEEN", text, (PcUiRect){ 858, 222, 362, 68 });
    SDL_snprintf(text, sizeof(text), "%u:%02u", data->playTime ? PlayTime_GetHours(data->playTime) : data->playHours,
                 data->playTime ? PlayTime_GetMinutes(data->playTime) : data->playMinutes);
    PcUi_StatCard(font, "PLAY TIME", text, (PcUiRect){ 464, 309, 338, 68 });
    SDL_snprintf(text, sizeof(text), "%04u-%02u-%02u", data->startYear + 2000, data->startMonth, data->startDay);
    PcUi_StatCard(font, "ADVENTURE STARTED", text, (PcUiRect){ 858, 309, 362, 68 });
    PcUi_LineLabel(font, "GREETING", 465, 455, 1010, PC_UI_CYAN, PC_UI_TEXT_BODY);
    if (data->canEdit)
        Button("EDIT", 1076, 451, 142, profileCursor == 0, ACTION_PROFILE);
    PcUi_RoleParagraph(font, greeting, 465, 493, 1220, 552, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "CLASS", 466, 559, 797, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CAPTION);
    PcUi_LineLabel(font, "NATURE", 854, 559, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CAPTION);
    if (data->canEdit) {
        PcUi_SkinControl(PC_MENU_BUTTON, (PcUiRect){ 462, 580, 355, 36 }, profileCursor == 1,
                         (PcUiAction){ PC_UI_ACTION_MENU, 1 });
        PcUi_SkinControl(PC_MENU_BUTTON, (PcUiRect){ 850, 580, 368, 36 }, profileCursor == 2,
                         (PcUiAction){ PC_UI_ACTION_MENU, 2 });
    }
    PcUi_RoleCenteredText(font, className, 474, 586, 803, 620,
                          profileCursor == 1 && data->canEdit ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
    PcUi_RoleCenteredText(font, personality, 862, 586, 1204, 620,
                          profileCursor == 2 && data->canEdit ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
}

static u32 RecordRows(TrainerCardWork *wk, u8 *rows) {
    u32 count = 0;
    for (u32 i = 0; i < wk->rowCount; i++) {
        u32 row = wk->rows[i];
        if (row != 3 && row != 5)
            rows[count++] = row;
    }
    return count;
}

static void Record(TrainerCardWork *wk, u32 row, u16 *label, char *value, size_t size) {
    static const u8 labels[] = {
        5, 6, 7, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 22, 23, 19, 24, 25, 26, 20, 21
    };
    TrainerCardData *data = wk->data, *own = wk->ownData;
    GFL_StrBufStoreString(wk->labels[labels[row]], label, 128);
    u32 number = 0;
    switch (row) {
    case 0:
        SDL_snprintf(value, size, "%05u", data->trainerId);
        return;
    case 1:
        SDL_snprintf(value, size, "%u:%02u", data->playTime ? PlayTime_GetHours(data->playTime) : data->playHours,
                     data->playTime ? PlayTime_GetMinutes(data->playTime) : data->playMinutes);
        return;
    case 2:
        SDL_snprintf(value, size, "%04u-%02u-%02u", data->startYear + 2000, data->startMonth, data->startDay);
        return;
    case 4:
        if (data->clearMonth)
            SDL_snprintf(value, size, "%04u-%02u-%02u %02u:%02u", data->clearYear + 2000, data->clearMonth,
                         data->clearDay, data->clearHour, data->clearMinute);
        else
            SDL_strlcpy(value, "Not yet entered", size);
        return;
    case 6:
        number = data->timesLinked;
        break;
    case 7:
        number = data->linkBattles;
        break;
    case 8:
        number = data->linkBattleWins;
        break;
    case 9:
        number = data->linkBattleLosses;
        break;
    case 10:
        number = data->linkTrades;
        break;
    case 11:
        number = data->wildEncounters;
        break;
    case 12:
        number = data->trainersBattled;
        break;
    case 13:
        number = data->passerbyCount;
        break;
    case 14:
        number = data->feelingChecks;
        break;
    case 15:
        number = data->passPowerCount;
        break;
    case 16:
        number = own->funfestMissions;
        break;
    case 17:
        number = own->movieShoots;
        break;
    case 18:
        number = data->musicalPerformances;
        break;
    case 19:
        number = own->pwtWins;
        break;
    case 20:
    case 21: {
        StrBuf *name = GFL_StrBufCreate(24, wk->heapId);
        GFL_StrBufLoadFixedString(name, own->avenueName, 23);
        GFL_WordSetSetStrbuf(wk->wordSet, 7, name, 0, 0, 0);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->tmpBuf, wk->labels[labels[row]]);
        GFL_StrBufStoreString(wk->tmpBuf, label, 128);
        GFL_StrBufFree(name);
        if (row == 20) {
            value[0] = 0;
            return;
        }
        number = own->recommendedCustomers;
        break;
    }
    case 22:
        number = data->palParkHighScore;
        break;
    case 23:
        number = data->battleTestHighScore;
        break;
    }
    SDL_snprintf(value, size, "%u", number);
}

static void Records(TrainerCardWork *wk) {
    u8 rows[TRCARD_ROW_MAX];
    u32 count = RecordRows(wk, rows);
    if (!count)
        return;
    recordCursor = SDL_min(recordCursor, count - 1);
    if (recordCursor < recordStart)
        recordStart = recordCursor;
    if (recordCursor >= recordStart + 5)
        recordStart = recordCursor - 4;
    PcUi_LineLabel(font, "ADVENTURE RECORDS", 453, 177, 1200, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    for (u32 i = 0; i < 5 && recordStart + i < count; i++) {
        u16 label[128];
        char value[80];
        u32 index = recordStart + i;
        Record(wk, rows[index], label, value, sizeof(value));
        PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ 447, 212 + i * 38, 766, 34 }, index == recordCursor,
                         (PcUiAction){ PC_UI_ACTION_ROW, (int)index });
        PcUi_LineText(font, label, 464, 218 + i * 38, 934, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_STAT_VALUE);
        PcUi_RightLabel(font, value, 946, 218 + i * 38, 1202, 248 + i * 38, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_STAT_VALUE);
    }
    u16 label[128];
    char value[80];
    Record(wk, rows[recordCursor], label, value, sizeof(value));
    PcUi_RoleParagraph(font, label, 465, 460, 1218, 515, PC_UI_CYAN, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, value, 465, 518, 1218, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CARD_TITLE);
    SDL_snprintf(value, sizeof(value), "%u / %u RECORDS", recordCursor + 1, count);
    PcUi_LineLabel(font, value, 465, 586, 1000, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    PcUi_LineLabel(font, "Up / Down to browse", 928, 586, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
}

static void Badges(TrainerCardWork *wk) {
    PcUi_LineLabel(font, "UNOVA GYM BADGES", 453, 177, 1200, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    for (u32 i = 0; i < 8; i++) {
        float x = 455 + (i % 4) * 191, y = 216 + (i / 4) * 96;
        bool obtained = (wk->data->badges & (1u << i)) != 0;
        PcUi_SkinControl(PC_MENU_TRAINER_BADGE, (PcUiRect){ x, y, 179, 84 }, i == badgeCursor,
                         (PcUiAction){ PC_UI_ACTION_ROW, (int)i });
        Picture(x + 16, y + 10, BADGE_WIDTH, BADGE_HEIGHT, i * BADGE_WIDTH, 96, BADGE_WIDTH, BADGE_HEIGHT,
                obtained ? 1 : 0.25f);
        PcUi_LineLabel(font, badgeNames[i], x + 66, y + 24, x + 174, PC_UI_INK, PC_UI_TEXT_CAPTION);
        PcUi_LineLabel(font, obtained ? "EARNED" : "UNOBTAINED", x + 66, y + 48, x + 174, PC_UI_INK_MUTED,
                       PC_UI_TEXT_KEY_HINT);
    }
    bool obtained = (wk->data->badges & (1u << badgeCursor)) != 0;
    char text[80];
    SDL_snprintf(text, sizeof(text), "%s BADGE", badgeNames[badgeCursor]);
    PcUi_LineLabel(font, text, 465, 455, 1218, PC_UI_CYAN, PC_UI_TEXT_VALUE);
    if (obtained) {
        TimeSigBadgeDate date = TimeSigSave_GetBadgeDate(getTimeSigSaveBlock(wk->param->gameData), badgeCursor);
        SDL_snprintf(text, sizeof(text), "Obtained %04u-%02u-%02u", date.year + 2000, date.month, date.day);
        PcUi_LineLabel(font, text, 465, 492, 1218, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        u16 description[256];
        GFL_MsgDataLoadRawStr(wk->msgData, 90 + badgeCursor * 3, description, SDL_arraysize(description));
        PcUi_RoleParagraph(font, description, 465, 532, 1218, 580, PC_UI_WHITE, PC_UI_TEXT_LABEL);
        GFL_MsgDataLoadRawStr(wk->msgData, 65 + badgeCursor * 3, description, SDL_arraysize(description));
        PcUi_LineText(font, description, 465, 588, 1218, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    } else {
        PcUi_LineLabel(font, "This badge has not been obtained yet.", 465, 500, 1218, PC_UI_DEFAULT_COLOR,
                       PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Defeat its Gym Leader to add it to your card.", 465, 550, 1218, PC_UI_MUTED,
                       PC_UI_TEXT_STAT_VALUE);
    }
}

static void Signature(TrainerCardWork *wk) {
    PcUi_LineLabel(font, "TRAINER SIGNATURE", 453, 177, 1200, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    PcUi_SkinControl(PC_MENU_SIGNATURE_PAD, signatureRect, 0, (PcUiAction){ PC_UI_ACTION_NONE, 0 });
    for (u32 y = 0; y < 64; y++)
        for (u32 x = 0; x < 192; x++) {
            u32 source = (y / 8 * 24 + x / 8) * 64 + (y & 7) * 8 + (x & 7);
            pictures[(160 + y) * 256 + x] = wk->signPixels[source] ? 0x98c4 : 0;
        }
    Picture(signatureRect.x, signatureRect.y, signatureRect.width, signatureRect.height, 0, 160, 192, 64, 1);
    PcUi_LineLabel(font, "MAKE IT YOURS", 465, 455, 1218, PC_UI_CYAN, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "Draw with the mouse inside the signature box.", 465, 493, 1218, PC_UI_DEFAULT_COLOR,
                   PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "Your signature is kept when you close this screen.", 465, 526, 1218, PC_UI_DEFAULT_COLOR,
                   PC_UI_TEXT_LABEL);
    if (wk->data->canEdit) {
        Button("PEN", 462, 579, 168, !erase, ACTION_PEN);
        Button("ERASER", 648, 579, 168, erase, ACTION_ERASER);
        Button("CLEAR", 834, 579, 168, false, ACTION_CLEAR);
        Button(canUndo ? "UNDO" : "UNDO —", 1020, 579, 198, false, canUndo ? ACTION_UNDO : 0);
    }
}

static void DrawGreetings(PMSSelectNativeState *state) {
    font = state->font;
    PcUi_BeginControls(state->font, (state->menu ? PAGE_COUNT + 1 : PAGE_COUNT));
    Frame(PC_MENU_MAIL, "GREETING", "Up/Down: Browse   Z: Choose or edit a phrase");
    PcUi_LineLabel(font, "SAVED GREETINGS", 59, 112, 812, PC_UI_CYAN, PC_UI_TEXT_SECTION);
    for (u32 i = 0; i < 3 && state->top + i < state->count; i++) {
        float y = 184 + i * 133;
        bool selected = state->cursor == state->top + i;
        PcUi_SkinControl(
            PC_MENU_GREETING_ROW, (PcUiRect){ 59, y, 757, 112 }, selected,
            (PcUiAction){ state->ready && !state->menu ? PC_UI_ACTION_ROW : PC_UI_ACTION_NONE, (int)(state->top + i) });
        char number[40];
        SDL_snprintf(number, sizeof(number), "%02u", state->top + i + 1);
        PcUi_LineLabel(font, number, 76, y + 13, 126, PC_UI_INK, PC_UI_TEXT_STAT_LABEL);
        if (state->empty[i])
            PcUi_LineLabel(font, "Create a new greeting", 142, y + 30, 793, PC_UI_INK, PC_UI_TEXT_VALUE);
        else
            PcUi_RoleParagraph(font, state->phrases[i], 142, y + 16, 793, y + 104, PC_UI_INK, PC_UI_TEXT_BODY);
    }
    PcUi_LineLabel(font, "GREETING", 905, 177, 1218, PC_UI_CYAN, PC_UI_TEXT_SECTION);
    PcUi_LineLabel(font, "Choose a saved phrase", 905, 220, 1218, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
    PcUi_LineLabel(font, "or create your own.", 905, 249, 1218, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
    AppTaskMenuNativeState menu;
    if (AppTaskMenu_GetNativeState(state->menu, &menu)) {
        for (u32 i = 0; i < menu.count; i++) {
            PcUi_SkinControl(PC_MENU_BUTTON, (PcUiRect){ 900, 352 + i * 48, 318, 40 }, i == menu.cursor,
                             (PcUiAction){ menu.ready ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, (int)i });
            PcUi_RoleCenteredText(font, menu.labels[i], 912, 361 + i * 48, 1204, 396 + i * 48,
                                  i == menu.cursor ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_BODY);
        }
    }
}

bool PcTrainerUi_Draw(void) {
    TrainerCardWork *wk = TrainerCard_GetNativeWork();
    PMSSelectNativeState phrases;
    if (!wk) {
        owner = NULL;
        if (!PMSSelect_GetNativeState(&phrases))
            return false;
    }
    PcGx.vertex_count = 0;
    PcGx.composite_scene = false;
    if (!PcUi_Begin()) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Unable to load the bundled trainer font", sizeof(PcGx.error));
        return true;
    }
    if (!wk) {
        DrawGreetings(&phrases);
        return true;
    }
    if (owner != wk) {
        owner = wk;
        page = wk->side ? PAGE_RECORDS : PAGE_PROFILE;
        recordCursor = recordStart = badgeCursor = profileCursor = 0;
        classId = personalityId = ~0u;
        greetingLoaded = false;
        erase = signing = canUndo = false;
        LoadPortrait(wk);
    }
    PcUi_BeginControls(wk, page);
    font = wk->font;
    RefreshText(wk);
    Frame(PC_MENU_TRAINER, "CARD",
          page == PAGE_SIGNATURE ? "Q/W: Tabs   E: Pen/Eraser   Mouse: Sign"
                                 : "Q/W: Tabs   Arrows: Browse   Z: Select");
    for (u32 i = 0; i < PAGE_COUNT; i++) {
        float x = 418 + 214 * i;
        PcUi_Tab(font, tabs[i], (PcUiRect){ x, 101, 203, 43 }, page == i, (PcUiAction){ PC_UI_ACTION_TAB, (int)i });
    }
    Sidebar(wk);
    if (page == PAGE_PROFILE)
        Profile(wk);
    else if (page == PAGE_RECORDS)
        Records(wk);
    else if (page == PAGE_BADGES)
        Badges(wk);
    else
        Signature(wk);
    SDL_memcpy(PcGx.textures, pictures, sizeof(pictures));
    return true;
}

static void ProfileAction(void) {
    static const TrainerCardNativeAction actions[] = { TRCARD_NATIVE_GREETING, TRCARD_NATIVE_APPEARANCE,
                                                       TRCARD_NATIVE_PERSONALITY };
    TrainerCard_NativeAction(actions[profileCursor]);
}

Uint16 PcTrainerUi_FilterKeys(Uint16 keys) {
    if (!PcTrainerUi_IsActive()) {
        pendingKeys = previousKeys = 0;
        pendingPage = pendingRow = pendingMenu = -1;
        owner = NULL;
        return keys;
    }
    keys |= pendingKeys;
    pendingKeys = 0;
    Uint16 pressed = keys & ~previousKeys;
    previousKeys = keys;
    TrainerCardWork *wk = TrainerCard_GetNativeWork();
    if (!wk) {
        PMSSelectNativeState state;
        if (PMSSelect_GetNativeState(&state)) {
            if (pendingRow >= 0 && PMSSelect_NativeSelect(pendingRow))
                keys |= PAD_BUTTON_A;
            AppTaskMenuNativeState menu;
            if (pendingMenu >= 0 && AppTaskMenu_GetNativeState(state.menu, &menu) && menu.ready &&
                (u32)pendingMenu < menu.count) {
                AppTaskMenu_SetCursorPos(state.menu, pendingMenu);
                keys |= PAD_BUTTON_A;
            }
        }
        pendingRow = pendingMenu = -1;
        GCTX_HIDSetUsingTouch(FALSE);
        return keys;
    }
    if (!TrainerCard_NativeReady())
        return 0;
    if (pressed & PAD_BUTTON_R)
        pendingPage = (page + 1) % PAGE_COUNT;
    if (pressed & PAD_BUTTON_L)
        pendingPage = (page + PAGE_COUNT - 1) % PAGE_COUNT;
    if (pendingPage >= 0 && pendingPage < PAGE_COUNT) {
        page = pendingPage;
        signing = false;
        TrainerCard_NativeSign(0, 0, FALSE, FALSE);
    }
    pendingPage = -1;
    if (page == PAGE_PROFILE) {
        if (pressed & (PAD_KEY_DOWN | PAD_KEY_RIGHT))
            profileCursor = (profileCursor + 1) % 3;
        if (pressed & (PAD_KEY_UP | PAD_KEY_LEFT))
            profileCursor = (profileCursor + 2) % 3;
        if (pressed & PAD_BUTTON_A)
            ProfileAction();
    } else if (page == PAGE_RECORDS) {
        u8 rows[TRCARD_ROW_MAX];
        u32 count = RecordRows(wk, rows);
        if ((pressed & PAD_KEY_DOWN) && recordCursor + 1 < count)
            recordCursor++;
        if ((pressed & PAD_KEY_UP) && recordCursor)
            recordCursor--;
        if (pendingRow >= 0 && (u32)pendingRow < count)
            recordCursor = pendingRow;
    } else if (page == PAGE_BADGES) {
        if (pressed & PAD_KEY_RIGHT)
            badgeCursor = (badgeCursor + 1) % 8;
        if (pressed & PAD_KEY_LEFT)
            badgeCursor = (badgeCursor + 7) % 8;
        if (pressed & (PAD_KEY_DOWN | PAD_KEY_UP))
            badgeCursor = (badgeCursor + 4) % 8;
        if (pendingRow >= 0 && pendingRow < 8)
            badgeCursor = pendingRow;
    }
    pendingRow = -1;
    GCTX_HIDSetUsingTouch(FALSE);
    return keys & PAD_BUTTON_B;
}

bool PcTrainerUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    if (!PcTrainerUi_IsActive())
        return false;
    TrainerCardWork *wk = TrainerCard_GetNativeWork();
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_MOUSE_WHEEL) {
        pendingKeys |= PcUi_EventKey(event);
        if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
            if (event->key.scancode == SDL_SCANCODE_TAB)
                pendingKeys |= PAD_BUTTON_R;
            if (event->key.scancode == SDL_SCANCODE_E && wk && page == PAGE_SIGNATURE)
                erase = !erase;
        }
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
        signing = false;
        TrainerCard_NativeSign(0, 0, FALSE, FALSE);
        return true;
    }
    bool down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT;
    bool motion = event->type == SDL_EVENT_MOUSE_MOTION && signing;
    if (!down && !motion)
        return false;
    float x, y;
    if (!PcVideo_MapPointer(video, motion ? event->motion.x : event->button.x,
                            motion ? event->motion.y : event->button.y, &x, &y))
        return true;
    x *= (float)PC_UI_WIDTH / PC_CANVAS_WIDTH;
    y *= (float)PC_UI_HEIGHT / PC_CANVAS_HEIGHT;
    if (wk && page == PAGE_SIGNATURE && wk->data->canEdit &&
        PcUi_Contains((PcUiShape){ .rect = signatureRect }, x, y)) {
        if (down) {
            SDL_memcpy(undoSign, wk->signPixels, sizeof(undoSign));
            canUndo = true;
            signing = true;
        }
        TrainerCard_NativeSign((x - signatureRect.x) * 192 / signatureRect.width,
                               (y - signatureRect.y) * 64 / signatureRect.height, signing, erase);
        return true;
    }
    if (motion) {
        TrainerCard_NativeSign(0, 0, FALSE, FALSE);
        return true;
    }
    PMSSelectNativeState state;
    if (!wk && !PMSSelect_GetNativeState(&state))
        return true;
    PcUiAction action = PcUi_PointerAction(video, event, wk ? (void *)wk : (void *)state.font,
                                           wk           ? page
                                           : state.menu ? PAGE_COUNT + 1
                                                        : PAGE_COUNT);
    switch (action.kind) {
    case PC_UI_ACTION_KEY:
        pendingKeys |= action.value;
        break;
    case PC_UI_ACTION_TAB:
        pendingPage = action.value;
        break;
    case PC_UI_ACTION_ROW:
        pendingRow = action.value;
        break;
    case PC_UI_ACTION_MENU:
        if (wk) {
            profileCursor = action.value;
            ProfileAction();
        } else
            pendingMenu = action.value;
        break;
    case PC_UI_ACTION_CUSTOM:
        if (!wk || !wk->data->canEdit)
            break;
        switch (action.value) {
        case ACTION_PROFILE:
            profileCursor = 0;
            ProfileAction();
            break;
        case ACTION_PEN:
            erase = false;
            break;
        case ACTION_ERASER:
            erase = true;
            break;
        case ACTION_CLEAR:
            SDL_memcpy(undoSign, wk->signPixels, sizeof(undoSign));
            canUndo = true;
            SDL_memset(wk->signPixels, 0, sizeof(wk->signPixels));
            break;
        case ACTION_UNDO:
            if (canUndo) {
                SDL_memcpy(wk->signPixels, undoSign, sizeof(undoSign));
                canUndo = false;
            }
            break;
        }
        break;
    default:
        break;
    }
    return true;
}
