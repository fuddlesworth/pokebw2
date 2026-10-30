#include "dex_ui.h"
#include "app/townmap/townmap_data.h"
#include "app/pokedex_common.h"
#include "app/pokedex_detail.h"
#include "app/pokedex_info.h"
#include "app/pokedex_search_engine.h"
#include "app/pokedex_list.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "dex_media.h"
#include "field/townmap_util.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gx_backend.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "pml/species_names.h"
#include "save/medal_box.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "ui_assets.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"
#include <SDL3/SDL.h>

// The form-name chain used by pokedex_detail_form.c and pokedex_info.c.
enum { DEX_FORM_NAME_ARC = 97 };
enum {
    DEX_SEARCH,
    DEX_FILTERS,
    DEX_LANGUAGE,
    DEX_ANIMATION,
    DEX_BACK_VIEW,
    DEX_COMPARE,
    DEX_MAP,
    DEX_HABITATS,
    DEX_AREA_PAGE,
    DEX_RESET_FILTERS
};
static unsigned ControlPhase(void);
static const PcUiRect mapRect = { 407, 220, 358.4f, 235.2f };
static u16 iconSpecies[9];
static u16 iconPixels[9][32 * 32];

static MsgData *categories;
static MsgData *entries;
static MsgData *translatedNames;
static MsgData *formNames;
static u16 *formNameTable;
static u32 language = 6, loadedLanguage = 0xff;
static bool backView, animationPlaying, compareForms, showWave;
static float cryWave[192], cryDuration;
static Uint64 cryStarted;
static u16 artwork[96 * 96];
static u16 artworkSpecies;
static bool artworkReady;
static u32 artworkLeft, artworkTop, artworkWidth, artworkHeight;
static u16 footprintPixels[16 * 16];
static u16 footprintSpecies;
static u32 footprintForm;
static u32 cryHandle;
static bool cryOwned;
static const u16 *sourceList;
static u16 browseList[649], dexNumbers[650];
static u32 browseCount, seenCount, caughtCount;
static PokedexSearchCriteria criteria, draftCriteria;
static bool filtersOpen;
static u32 filterRow;
static u16 results[649];
static u32 resultCount, selected, listStart;
static PcDexTab activeTab;
static char search[48];
static bool searching;
static SDL_Window *textWindow;
static u16 previousKeys, pendingKeys;
static bool backRequested;
static u32 heldFrames;
static u32 season, areaPage;
static u16 areaZones[85];
static u8 areaKinds[85];
static u32 areaCount;
static MsgData *placeNames;
static struct {
    u32 form, sex, rare;
} variants[112];
static u32 variantCount, variantIndex;
static u16 detailsSpecies;
static void *townmap;
static PokedexHabitatListParam *habitatList;
static u16 mapPlace = TOWNMAP_PLACE_NONE;
static bool habitatsMode;

static void PlayCry(const PokedexNativeListState *state);
static void ChangeFilter(const PokedexNativeListState *state, int direction);
static void ChangeLanguage(const PokedexNativeListState *state);
static void PickMapPlace(const PokedexNativeListState *state, float x, float y);
static bool MapPlaceVisible(const PokedexNativeListState *state, u16 place);

static void StopCry(void) {
    if (cryOwned) {
        PokeVoice_Release(cryHandle);
        cryOwned = false;
    }
}

bool PcDexUi_IsActive(void) {
    PokedexNativeListState state;
    return PokedexList_GetNativeState(&state) && state.count;
}

static void EndSearch(void) {
    searching = false;
    if (textWindow)
        SDL_StopTextInput(textWindow);
}

static void FilterResults(const PokedexNativeListState *state) {
    u16 old = resultCount && selected < resultCount ? results[selected] : PokeDex_GetLastSpecies(state->pokedex);
    u16 *filtered = NULL;
    u16 count = 0;
    bool advanced = criteria.order || criteria.initial != 0xff || criteria.type1 != 0xff || criteria.type2 != 0xff ||
                    criteria.color != 0xff || criteria.shape != 0xff;
    if (advanced) {
        PokedexSearchEngine *engine = PokedexSearchEngine_Create(state->pokedex, &criteria, HEAPID_USER);
        while (!PokedexSearchEngine_Step(engine, HEAPID_USER, &count, &filtered)) {
        }
        PokedexSearchEngine_Free(engine);
    }
    resultCount = 0;
    selected = 0;
    u32 total = advanced ? count : browseCount;
    for (u32 i = 0; i < total && resultCount < SDL_arraysize(results); i++) {
        u16 species = advanced ? filtered[i] : browseList[i];
        u16 wide[32];
        char name[32];
        char number[16];
        bool match = !search[0];
        if (!match && PokeDex_IsSeen(state->pokedex, species)) {
            GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, species, wide, SDL_arraysize(wide));
            u32 n = 0;
            while (n + 1 < sizeof(name) && wide[n] && wide[n] != 0xffff) {
                name[n] = wide[n] < 128 ? (char)wide[n] : '?';
                n++;
            }
            name[n] = 0;
            SDL_snprintf(number, sizeof(number), "%03u", dexNumbers[species]);
            match = SDL_strcasestr(name, search) || SDL_strstr(number, search);
        }
        if (match) {
            if (species == old)
                selected = resultCount;
            results[resultCount++] = species;
        }
    }
    if (filtered)
        GFL_HeapFree(filtered);
    detailsSpecies = 0;
}

static void BuildBrowseList(const PokedexNativeListState *state) {
    u16 *list;
    u32 count = PokedexSearchEngine_LoadList(criteria.regional, HEAPID_USER, &list);
    u16 *regionalNumbers = criteria.regional ? PML_PersonalLoadRegionalDexTable(HEAPID_USER, NULL) : NULL;
    browseCount = 0;
    seenCount = caughtCount = 0;
    SDL_zero(dexNumbers);
    u32 next = 0;
    for (u32 i = 0; i < count && i < SDL_arraysize(browseList); i++) {
        u16 species = list[i];
        bool seen = PokeDex_IsSeen(state->pokedex, species);
        dexNumbers[species] = regionalNumbers ? regionalNumbers[species] : species;
        seenCount += seen;
        caughtCount += PokeDex_IsCaught(state->pokedex, species);
        if (regionalNumbers && !dexNumbers[species] && !seen)
            continue;
        browseList[next++] = species;
        if (seen)
            browseCount = next;
    }
    if (regionalNumbers)
        GFL_HeapFree(regionalNumbers);
    GFL_HeapFree(list);
    FilterResults(state);
}

static void OpenFilters(void) {
    EndSearch();
    draftCriteria = criteria;
    filterRow = 0;
    filtersOpen = true;
}

static void ApplyFilters(const PokedexNativeListState *state) {
    criteria = draftCriteria;
    PokeDex_SetMode(state->pokedex, criteria.regional ? DEX_REGIONAL : DEX_NATIONAL);
    filtersOpen = false;
    BuildBrowseList(state);
}

static void ChangeFilter(const PokedexNativeListState *state, int direction) {
    static const u8 counts[] = { 2, 6, 26, 17, 17, 10, 14 };
    u8 *values[] = { &draftCriteria.regional, &draftCriteria.order, &draftCriteria.initial, &draftCriteria.type1,
                     &draftCriteria.type2,    &draftCriteria.color, &draftCriteria.shape };
    if (filterRow == 0 && !PokeDex_IsNationalObtained(state->pokedex))
        return;
    int value = *values[filterRow] == 0xff ? -1 : *values[filterRow];
    int first = filterRow < 2 ? 0 : -1;
    value += direction;
    if (value >= counts[filterRow])
        value = first;
    if (value < first)
        value = counts[filterRow] - 1;
    *values[filterRow] = value < 0 ? 0xff : (u8)value;
}

static bool CurrentState(PokedexNativeListState *state) {
    if (!PokedexList_GetNativeState(state) || !state->count)
        return false;
    if (sourceList != state->species) {
        sourceList = state->species;
        search[0] = 0;
        activeTab = PC_DEX_ENTRY;
        selected = resultCount = 0;
        previousKeys = pendingKeys = heldFrames = 0;
        season = areaPage = 0;
        PokedexCommon_InitCriteria(state->pokedex, &criteria);
        filtersOpen = false;
        language = 6;
        backView = animationPlaying = compareForms = showWave = false;
        BuildBrowseList(state);
    }
    return true;
}

static void ChangeSelection(int direction) {
    StopCry();
    if (resultCount) {
        if (direction < 0 && selected)
            selected--;
        if (direction > 0 && selected + 1 < resultCount)
            selected++;
    }
}

static void ChangeDetail(int direction) {
    if (activeTab == PC_DEX_FORMS && variantCount) {
        StopCry();
        variantIndex = (variantIndex + variantCount + direction) % variantCount;
        artworkReady = false;
    } else if (activeTab == PC_DEX_AREA) {
        season = (season + 4 + direction) % 4;
        detailsSpecies = 0;
    }
}

static unsigned ControlPhase(void) {
    return activeTab + (filtersOpen ? PC_DEX_TAB_COUNT : 0);
}

Uint16 PcDexUi_FilterKeys(Uint16 keys) {
    PokedexNativeListState state;
    if (!CurrentState(&state))
        return keys;
    // Keep a quick press until a simulation tick consumes it, even if SDL also saw its release.
    keys |= pendingKeys;
    pendingKeys = 0;
    if (backRequested) {
        keys |= PAD_BUTTON_B;
        backRequested = false;
    }
    u16 pressed = keys & ~previousKeys;
    heldFrames = keys && keys == previousKeys ? heldFrames + 1 : 0;
    previousKeys = keys;
    if (filtersOpen) {
        if (pressed & PAD_BUTTON_B)
            filtersOpen = false;
        else if (pressed & PAD_BUTTON_A)
            ApplyFilters(&state);
        else if (pressed || (heldFrames >= 18 && heldFrames % 5 == 0)) {
            if (keys & PAD_KEY_UP)
                filterRow = (filterRow + 6) % 7;
            if (keys & PAD_KEY_DOWN)
                filterRow = (filterRow + 1) % 7;
            if (keys & PAD_KEY_LEFT)
                ChangeFilter(&state, -1);
            if (keys & PAD_KEY_RIGHT)
                ChangeFilter(&state, 1);
        }
        return 0;
    }
    if (searching)
        return 0;
    if (pressed & PAD_BUTTON_X) {
        OpenFilters();
        return 0;
    }
    if (pressed & PAD_BUTTON_START) {
        searching = true;
        if (textWindow)
            SDL_StartTextInput(textWindow);
    }
    if (pressed & PAD_BUTTON_L)
        activeTab = (activeTab + PC_DEX_TAB_COUNT - 1) % PC_DEX_TAB_COUNT;
    if (pressed & PAD_BUTTON_R)
        activeTab = (activeTab + 1) % PC_DEX_TAB_COUNT;
    if (pressed & PAD_BUTTON_A)
        PlayCry(&state);
    if ((pressed & PAD_BUTTON_Y) && activeTab == PC_DEX_AREA && townmap) {
        for (u32 i = 1; i <= TOWNMAP_PLACE_COUNT; i++) {
            u16 place = mapPlace == TOWNMAP_PLACE_NONE ? i - 1 : (mapPlace + i) % TOWNMAP_PLACE_COUNT;
            if (MapPlaceVisible(&state, place)) {
                mapPlace = place;
                areaPage = 0;
                break;
            }
        }
    }
    if ((pressed & PAD_BUTTON_SELECT) && activeTab == PC_DEX_AREA)
        areaPage++;
    if (pressed & PAD_BUTTON_B) {
        if (activeTab != PC_DEX_ENTRY) {
            activeTab = PC_DEX_ENTRY;
        } else if (search[0]) {
            search[0] = 0;
            FilterResults(&state);
        } else {
            StopCry();
            return PAD_BUTTON_B;
        }
    }
    if (pressed || (heldFrames >= 18 && heldFrames % 5 == 0)) {
        if (keys & PAD_KEY_UP)
            ChangeSelection(-1);
        if (keys & PAD_KEY_DOWN)
            ChangeSelection(1);
        if (keys & PAD_KEY_LEFT) {
            if (activeTab == PC_DEX_ENTRY)
                ChangeSelection(-1);
            else
                ChangeDetail(-1);
        }
        if (keys & PAD_KEY_RIGHT) {
            if (activeTab == PC_DEX_ENTRY)
                ChangeSelection(1);
            else
                ChangeDetail(1);
        }
    }
    return 0;
}

bool PcDexUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    PokedexNativeListState state;
    if (!CurrentState(&state))
        return false;
    textWindow = video->window;
    if (event->type == SDL_EVENT_TEXT_INPUT && searching) {
        SDL_strlcat(search, event->text.text, sizeof(search));
        FilterResults(&state);
        return true;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_Scancode key = event->key.scancode;
        if (!searching && !event->key.repeat) {
            switch (key) {
            case SDL_SCANCODE_Z:
                pendingKeys |= PAD_BUTTON_A;
                break;
            case SDL_SCANCODE_X:
                pendingKeys |= PAD_BUTTON_B;
                break;
            case SDL_SCANCODE_S:
                pendingKeys |= PAD_BUTTON_X;
                break;
            case SDL_SCANCODE_A:
                pendingKeys |= PAD_BUTTON_Y;
                break;
            case SDL_SCANCODE_Q:
                pendingKeys |= PAD_BUTTON_L;
                break;
            case SDL_SCANCODE_W:
                pendingKeys |= PAD_BUTTON_R;
                break;
            case SDL_SCANCODE_UP:
                pendingKeys |= PAD_KEY_UP;
                break;
            case SDL_SCANCODE_DOWN:
                pendingKeys |= PAD_KEY_DOWN;
                break;
            case SDL_SCANCODE_LEFT:
                pendingKeys |= PAD_KEY_LEFT;
                break;
            case SDL_SCANCODE_RIGHT:
                pendingKeys |= PAD_KEY_RIGHT;
                break;
            case SDL_SCANCODE_RETURN:
                pendingKeys |= PAD_BUTTON_START;
                break;
            case SDL_SCANCODE_BACKSPACE:
                pendingKeys |= PAD_BUTTON_SELECT;
                break;
            default:
                break;
            }
        }
        if (filtersOpen) {
            if (key == SDL_SCANCODE_ESCAPE)
                filtersOpen = false;
            else if (key == SDL_SCANCODE_R && !event->key.repeat) {
                PokedexCommon_InitCriteria(state.pokedex, &draftCriteria);
                filterRow = 0;
            }
            return true;
        }
        if (key == SDL_SCANCODE_ESCAPE && !searching) {
            backRequested = true;
            return true;
        }
        if (searching) {
            if (key == SDL_SCANCODE_BACKSPACE && search[0]) {
                size_t n = SDL_strlen(search);
                do {
                    n--;
                } while (n && ((unsigned char)search[n] & 0xc0) == 0x80);
                search[n] = 0;
                FilterResults(&state);
            } else if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_RETURN) {
                EndSearch();
                previousKeys = 0xffff;
            }
            return true;
        }
        if (!filtersOpen && !event->key.repeat) {
            if (key == SDL_SCANCODE_L && activeTab == PC_DEX_ENTRY) {
                ChangeLanguage(&state);
                return true;
            }
            if (key == SDL_SCANCODE_SPACE && activeTab == PC_DEX_FORMS) {
                animationPlaying = !animationPlaying;
                PcDexMedia_Play(animationPlaying);
                return true;
            }
            if (key == SDL_SCANCODE_V && activeTab == PC_DEX_FORMS) {
                backView = !backView;
                return true;
            }
            if (key == SDL_SCANCODE_C) {
                if (activeTab == PC_DEX_FORMS)
                    compareForms = variantCount > 1 && !compareForms;
                else
                    showWave = !showWave;
                return true;
            }
            if (key == SDL_SCANCODE_H && activeTab == PC_DEX_AREA) {
                habitatsMode = !habitatsMode;
                return true;
            }
        }
        if (key == SDL_SCANCODE_TAB) {
            if (!event->key.repeat)
                activeTab = (activeTab + ((event->key.mod & SDL_KMOD_SHIFT) ? 2 : 1)) % PC_DEX_TAB_COUNT;
            return true;
        }
        if (key == SDL_SCANCODE_SLASH || (key == SDL_SCANCODE_F && (event->key.mod & SDL_KMOD_CTRL))) {
            searching = true;
            SDL_StartTextInput(video->window);
            return true;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        u16 key = PcUi_EventKey(event);
        if (key) {
            if (filtersOpen)
                filterRow = (filterRow + (key == PAD_KEY_UP ? 6 : 1)) % 7;
            else
                ChangeSelection(key == PAD_KEY_UP ? -1 : 1);
        }
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        PcUiAction action = PcUi_PointerAction(video, event, sourceList, ControlPhase());
        if (action.kind == PC_UI_ACTION_NONE)
            return true;
        if (action.kind != PC_UI_ACTION_CUSTOM || action.value != DEX_SEARCH)
            EndSearch();
        switch (action.kind) {
        case PC_UI_ACTION_KEY:
            if (filtersOpen) {
                if (action.value == PAD_BUTTON_A)
                    ApplyFilters(&state);
                if (action.value == PAD_BUTTON_B)
                    filtersOpen = false;
            } else if (action.value == PAD_BUTTON_B)
                backRequested = true;
            else if (action.value == PAD_BUTTON_A)
                PlayCry(&state);
            else if (action.value == PAD_KEY_UP || action.value == PAD_KEY_DOWN) {
                int direction = action.value == PAD_KEY_UP ? -1 : 1;
                if (activeTab == PC_DEX_ENTRY)
                    ChangeSelection(direction);
                else
                    ChangeDetail(direction);
            } else
                pendingKeys |= action.value;
            break;
        case PC_UI_ACTION_TAB:
            activeTab = action.value;
            break;
        case PC_UI_ACTION_ROW:
            if ((u32)action.value < resultCount) {
                StopCry();
                selected = action.value;
            }
            break;
        case PC_UI_ACTION_MENU:
            if ((u32)action.value < variantCount) {
                StopCry();
                variantIndex = action.value;
                artworkReady = false;
            }
            break;
        case PC_UI_ACTION_MARK:
            filterRow = action.value / 2;
            ChangeFilter(&state, action.value % 2 ? 1 : -1);
            break;
        case PC_UI_ACTION_CUSTOM:
            switch (action.value) {
            case DEX_SEARCH:
                searching = true;
                SDL_StartTextInput(video->window);
                break;
            case DEX_FILTERS:
                OpenFilters();
                break;
            case DEX_LANGUAGE:
                ChangeLanguage(&state);
                break;
            case DEX_ANIMATION:
                animationPlaying = !animationPlaying;
                PcDexMedia_Play(animationPlaying);
                break;
            case DEX_BACK_VIEW:
                backView = !backView;
                break;
            case DEX_COMPARE:
                compareForms = variantCount > 1 && !compareForms;
                break;
            case DEX_MAP: {
                float x, y;
                if (PcVideo_MapPointer(video, event->button.x, event->button.y, &x, &y))
                    PickMapPlace(&state, x * PC_UI_WIDTH / PC_CANVAS_WIDTH, y * PC_UI_HEIGHT / PC_CANVAS_HEIGHT);
                break;
            }
            case DEX_HABITATS:
                habitatsMode = !habitatsMode;
                break;
            case DEX_AREA_PAGE:
                if (habitatsMode)
                    areaPage++;
                else if (areaCount > 5)
                    areaPage = (areaPage + 1) % ((areaCount + 4) / 5);
                break;
            case DEX_RESET_FILTERS:
                PokedexCommon_InitCriteria(state.pokedex, &draftCriteria);
                filterRow = 0;
                break;
            }
            break;
        default:
            break;
        }
        return true;
    }
    return false;
}
static void DrawMeasurement(Font *font, const char *label, const char *value, const char *unit, float left) {
    const float baseline = 391;
    // Units occupy a separate column, so their length cannot shift numeric alignment.
    PcUi_RoleLabel(font, label, left + 23, baseline - 18, left + 98, 408, PC_UI_INK, PC_UI_TEXT_STAT_LABEL);
    PcUiTextFit fit = PcUi_FitLabel(font, value, PC_UI_TEXT_VALUE, 58);
    PcUi_RightLabel(font, value, left + 93, baseline - PC_UI_FONT_BASELINE * fit.scale, left + 151, 408, PC_UI_INK,
                    PC_UI_TEXT_VALUE);
    PcUi_RoleLabel(font, unit, left + 161, baseline - 18, left + 192, 408, PC_UI_INK, PC_UI_TEXT_STAT_LABEL);
}

static void DrawFilters(Font *font, PokeDexSave *pokedex) {
    static const char *const labels[] = { "Pokédex", "Order", "Initial", "Type 1", "Type 2", "Color", "Shape" };
    static const char *const orders[] = { "Number",         "A to Z",        "Heavy to Light",
                                          "Light to Heavy", "Tall to Short", "Short to Tall" };
    static const char *const types[] = { "Normal",   "Fighting", "Flying", "Poison", "Ground", "Rock",
                                         "Bug",      "Ghost",    "Steel",  "Fire",   "Water",  "Grass",
                                         "Electric", "Psychic",  "Ice",    "Dragon", "Dark" };
    static const char *const colors[] = { "Red",   "Blue",   "Yellow", "Green", "Black",
                                          "Brown", "Purple", "Gray",   "White", "Pink" };
    static const char *const shapes[] = { "Head",
                                          "Serpentine",
                                          "Fins",
                                          "Arms",
                                          "Base",
                                          "Biped / Tail",
                                          "Head / Legs",
                                          "Quadruped",
                                          "Wings",
                                          "Tentacles",
                                          "Multiple bodies",
                                          "Biped",
                                          "Insect wings",
                                          "Insect body" };
    static const float rows[] = { 258, 300, 342, 384, 453, 495, 537 };
    char initial[2] = { draftCriteria.initial < 26 ? 'A' + draftCriteria.initial : ' ', 0 };
    const char *values[] = { draftCriteria.regional ? "Unova" : "National",
                             orders[draftCriteria.order],
                             draftCriteria.initial == 0xff ? "Any" : initial,
                             draftCriteria.type1 == 0xff ? "Any" : types[draftCriteria.type1],
                             draftCriteria.type2 == 0xff ? "Any" : types[draftCriteria.type2],
                             draftCriteria.color == 0xff ? "Any" : colors[draftCriteria.color],
                             draftCriteria.shape == 0xff ? "Any" : shapes[draftCriteria.shape] };
    PcUi_RoleLabel(font, "FILTERS", 893, 171, 1245, 220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_PANEL_TITLE);
    PcUi_RoleLabel(font, "Refine the recorded Pokémon", 850, 219, 1245, 247, PC_UI_INK, PC_UI_TEXT_LABEL);
    for (u32 row = 0; row < SDL_arraysize(rows); row++) {
        for (u32 direction = 0; direction < 2; direction++)
            PcUi_Control((PcUiShape){ .rect = { direction ? 1030 : 848, rows[row] - 3, direction ? 218 : 182, 32 } },
                         (PcUiAction){ PC_UI_ACTION_MARK, (int)(row * 2 + direction) });
        u32 color = row < 4 ? PC_UI_INK : PC_UI_WHITE;
        if (row == filterRow)
            color = row < 4 ? PC_UI_LINK : PC_UI_CYAN;
        PcUi_RoleLabel(font, labels[row], 862, rows[row], 1000, rows[row] + 32, color, PC_UI_TEXT_STAT_VALUE);
        PcUi_RoleLabel(font, "<", 1008, rows[row], 1028, rows[row] + 32, color, PC_UI_TEXT_STAT_VALUE);
        PcUi_RoleLabel(font, values[row], 1034, rows[row], 1218, rows[row] + 32, color, PC_UI_TEXT_STAT_VALUE);
        PcUi_RoleLabel(font, ">", 1224, rows[row], 1244, rows[row] + 32, color, PC_UI_TEXT_STAT_VALUE);
    }
    PcUi_Control((PcUiShape){ .rect = { 850, 590, 180, 35 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_RESET_FILTERS });
    PcUi_Control((PcUiShape){ .rect = { 1105, 590, 135, 35 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
    PcUi_RoleLabel(font, "R: Reset", 850, 593, 1030, 625, PC_UI_MUTED, PC_UI_TEXT_BODY);
    PcUi_RoleLabel(font, "Z: Apply", 1105, 593, 1240, 625, PC_UI_CYAN, PC_UI_TEXT_BODY);
    if (!PokeDex_IsNationalObtained(pokedex))
        PcUi_RoleLabel(font, "National Dex locked", 463, 578, 773, 610, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
}

static void LoadDetails(const PokedexNativeListState *state, u16 species) {
    if (detailsSpecies == species)
        return;
    detailsSpecies = species;
    static const u8 languageIds[] = { 1, 3, 5, 4, 7, 8 };
    if (language < 6 && (species > 493 || !PokeDex_HasLanguage(state->pokedex, species, languageIds[language])))
        language = 6;
    StopCry();
    SDL_zero(cryWave);
    cryDuration = 0;
    artworkReady = false;
    variantCount = variantIndex = areaPage = areaCount = 0;
    if (!PokeDex_IsSeen(state->pokedex, species))
        return;
    PokedexList_SetNativeSpecies(species);
    u32 sex = 0, rare = 0, form = 0;
    PokeDex_GetShownSexRareForm(state->pokedex, species, &sex, &rare, &form, HEAPID_USER);
    variants[0].sex = sex;
    variants[0].rare = rare;
    variants[0].form = form;
    variantCount = 1;
    u32 ratio = PML_PersonalGetParamSingle(species, 0, PERSONAL_SEX_RATIO);
    u32 forms = getNumberOfForms(species);
    for (u32 f = 0; f < forms; f++) {
        for (u32 r = 0; r < 2; r++) {
            for (u32 s = 0; s < 3; s++) {
                if ((ratio == 255 && s != 2) || (ratio != 255 && s == 2) || (ratio == 0 && s != 0) ||
                    (ratio == 254 && s != 1) || !PokeDex_HasSeenVariant(state->pokedex, species, s, r, f))
                    continue;
                bool duplicate = false;
                for (u32 i = 0; i < variantCount; i++) {
                    if (variants[i].form == f && variants[i].rare == r && (forms > 1 || variants[i].sex == s))
                        duplicate = true;
                }
                if (!duplicate && variantCount < SDL_arraysize(variants)) {
                    variants[variantCount].sex = s;
                    variants[variantCount].rare = r;
                    variants[variantCount++].form = f;
                }
            }
        }
    }
    areaCount = PokedexDetailMap_GetNativeHabitats(species, season, areaZones, areaKinds, SDL_arraysize(areaZones));
}

static void DrawWave(Font *font) {
    if (!showWave || activeTab == PC_DEX_AREA || filtersOpen)
        return;
    float elapsed = cryDuration ? (SDL_GetTicksNS() - cryStarted) / 1000000000.0f : 0;
    elapsed = SDL_min(elapsed, cryDuration);
    for (u32 i = 0; i < SDL_arraysize(cryWave); i++) {
        float amplitude = SDL_max(1.0f, cryWave[i] * 32);
        bool passed = cryDuration > 0 && i / 192.0f <= elapsed / cryDuration;
        PcUi_Solid(441 + i * 1.5f, 551 - amplitude / 2, 1, amplitude, passed ? PC_UI_CYAN : PC_UI_INK_MUTED, 1);
    }
    char text[64];
    SDL_snprintf(text, sizeof(text), "Cry %.2f / %.2f s", elapsed, cryDuration);
    PcUi_RoleLabel(font, text, 477, 570, 757, 600, PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
}

static bool MapPlaceVisible(const PokedexNativeListState *state, u16 place) {
    u16 flag = TownMapData_GetParam(townmap, place, TOWNMAP_PARAM_FLAG);
    return flag == TOWNMAP_NO_FLAG || TownMap_CheckFlag(state->gameData, flag);
}

static void PickMapPlace(const PokedexNativeListState *state, float x, float y) {
    if (!townmap)
        return;
    float nearest = 24 * 24;
    u16 place = TOWNMAP_PLACE_NONE;
    for (u16 i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (!MapPlaceVisible(state, i))
            continue;
        float dx = mapRect.x + TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_X) * 1.4f - x;
        float dy = mapRect.y + TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_Y) * 1.4f - y;
        if (dx * dx + dy * dy < nearest) {
            nearest = dx * dx + dy * dy;
            place = i;
        }
    }
    mapPlace = place;
    areaPage = 0;
}

static void DrawHabitatMap(const PokedexNativeListState *state) {
    if (!townmap) {
        townmap = TownMapData_Load(HEAPID_USER);
        habitatList = GFL_HeapAllocate(HEAPID_USER, sizeof(*habitatList), FALSE, __FILE__, __LINE__);
        SDL_memset(habitatList, 0, sizeof(*habitatList));
        habitatList->gameData = state->gameData;
        PokedexCommon_BuildHabitatList(SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(state->gameData)),
                                     habitatList, FALSE);
    }
    u32 visibleCount = 0;
    for (u32 i = 0; i < areaCount; i++) {
        for (u16 place = 0; place < TOWNMAP_PLACE_COUNT; place++) {
            if (TownMapData_GetParam(townmap, place, TOWNMAP_PARAM_ZONE) == areaZones[i] &&
                MapPlaceVisible(state, place)) {
                areaZones[visibleCount] = areaZones[i];
                areaKinds[visibleCount++] = areaKinds[i];
                break;
            }
        }
    }
    areaCount = visibleCount;
    const u16 *pixels = PcDexMedia_Map();
    if (pixels) {
        SDL_memcpy(PcGx.textures, pixels, 256 * 256 * sizeof(u16));
        u32 texture = (7u << 26) | (5u << 23) | (5u << 20);
        PcUi_Quad(mapRect.x, mapRect.y, mapRect.width, mapRect.height, -0.4f, 1, 1, 1, texture, 0, 0, 256, 168);
    }
    PcUi_Control((PcUiShape){ .rect = mapRect }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_MAP });
    PcUi_Control((PcUiShape){ .rect = { 460, 548, 305, 29 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_Y });
    PcUi_Control((PcUiShape){ .rect = { 460, 580, 305, 29 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_HABITATS });
    PcUi_RoleCenteredLabel(state->font, "UNOVA HABITATS", 407, 171, 765.4f, 208, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    for (u16 i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (!MapPlaceVisible(state, i))
            continue;
        u16 zone = TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_ZONE);
        bool found = false;
        for (u32 j = 0; j < areaCount; j++)
            found |= areaZones[j] == zone;
        u32 color = found ? PC_UI_CYAN : PC_UI_MUTED;
        if (habitatsMode) {
            found = false;
            for (u32 j = 0; j < habitatList->count; j++) {
                if (habitatList->areas[j].zone == zone) {
                    found = true;
                    color = habitatList->areas[j].complete ? PC_UI_SUCCESS : PC_UI_WARNING;
                }
            }
        }
        float x = mapRect.x + TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_X) * 1.4f;
        float y = mapRect.y + TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_Y) * 1.4f;
        if (found && pixels)
            PcDexMedia_HighlightArea((u16 *)PcGx.textures, TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_AREA_ANIM),
                                     TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_AREA_X),
                                     TownMapData_GetParam(townmap, i, TOWNMAP_PARAM_AREA_Y), color);
        PcUi_Solid(x - 2, y - 2, 4, 4, color, 1);
        if (i == mapPlace) {
            PcUi_Solid(x - 7, y - 7, 14, 2, 0xffffff, 1);
            PcUi_Solid(x - 7, y + 5, 14, 2, 0xffffff, 1);
            PcUi_Solid(x - 7, y - 5, 2, 10, 0xffffff, 1);
            PcUi_Solid(x + 5, y - 5, 2, 10, 0xffffff, 1);
        }
    }
    if (mapPlace != TOWNMAP_PLACE_NONE) {
        u16 text[80];
        u16 zone = TownMapData_GetParam(townmap, mapPlace, TOWNMAP_PARAM_ZONE);
        GFL_MsgDataLoadRawStr(placeNames, ZoneData_GetPlaceNameID(zone), text, SDL_arraysize(text));
        PcUi_RoleCenteredText(state->font, text, 407, 513, 765.4f, 545, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    } else
        PcUi_RoleCenteredLabel(state->font, "Click a location", 407, 513, 765.4f, 545, PC_UI_MUTED, PC_UI_TEXT_BODY);
    PcUi_RoleLabel(state->font, "A:", 460, 548, 488, 577, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    PcUi_RoleLabel(state->font, "Next location", 492, 548, 765.4f, 577, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    PcUi_RoleLabel(state->font, "H:", 460, 580, 488, 609, PC_UI_CYAN, PC_UI_TEXT_LABEL);
    PcUi_RoleLabel(state->font, habitatsMode ? "Species habitats" : "Habitat completion", 492, 580, 765.4f, 609,
                   PC_UI_CYAN, PC_UI_TEXT_LABEL);
}

static void ChangeLanguage(const PokedexNativeListState *state) {
    if (!resultCount)
        return;
    static const u8 ids[] = { 1, 3, 5, 4, 7, 8 };
    u16 species = results[selected];
    for (u32 i = 1; i <= 7; i++) {
        u32 next = (language + i) % 7;
        if (next == 6 || (species <= 493 && PokeDex_HasLanguage(state->pokedex, species, ids[next]))) {
            language = next;
            return;
        }
    }
}

static void LoadLanguage(void) {
    if (loadedLanguage == language)
        return;
    if (translatedNames)
        GFL_MsgDataFree(translatedNames);
    if (categories)
        GFL_MsgDataFree(categories);
    if (entries)
        GFL_MsgDataFree(entries);
    u16 namesBank, categoryBank, entryBank;
    PokedexInfo_GetNativeLanguageBanks(language, &namesBank, &categoryBank, &entryBank);
    translatedNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, namesBank, HEAPID_USER);
    categories = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, categoryBank, HEAPID_USER);
    entries = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, entryBank, HEAPID_USER);
    loadedLanguage = language;
}

static void PlayCry(const PokedexNativeListState *state) {
    if (!resultCount || selected >= resultCount)
        return;
    u16 species = results[selected];
    if (!PokeDex_IsSeen(state->pokedex, species))
        return;
    LoadDetails(state, species);
    StopCry();
    cryHandle = PokeVoice_Play(species, variants[variantIndex].form, 64, 0, 0, 0, 0, NULL);
    cryOwned = true;
    cryStarted = SDL_GetTicksNS();
    u32 count = PokeVoice_GetSampleCount(cryHandle);
    const s8 *samples = PokeVoice_GetSamples(cryHandle);
    cryDuration = count * 32768.0f / PokeVoice_GetSampleRate(cryHandle) / PokeVoice_GetSpeed(cryHandle);
    for (u32 column = 0; column < SDL_arraysize(cryWave); column++) {
        float peak = 0;
        for (u32 i = column * count / SDL_arraysize(cryWave);
             samples && i < (column + 1) * count / SDL_arraysize(cryWave); i++)
            peak = SDL_max(peak, SDL_abs(samples[i]) / 128.0f);
        cryWave[column] = peak;
    }
    showWave = true;
    if (SDL_getenv("POKEBW2_TRACE_DEX"))
        SDL_Log("Pokédex cry: species=%u form=%u samples=%u playing=%d", species, variants[variantIndex].form,
                PokeVoice_GetSampleCount(cryHandle), PokeVoice_IsPlaying(cryHandle));
}

static void DrawExtraPanel(const PokedexNativeListState *state, u16 species) {
    Font *font = state->font;
    char text[96];
    if (activeTab == PC_DEX_AREA) {
        PcUi_Control((PcUiShape){ .rect = { 850, 445, 388, 185 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_AREA_PAGE });
        static const char *const seasons[] = { "SPRING", "SUMMER", "AUTUMN", "WINTER" };
        PcUi_RoleLabel(font, "HABITAT", 893, 257, 1248, 292, PC_UI_INK, PC_UI_TEXT_VALUE);
        PcUi_RoleLabel(font, seasons[season], 893, 302, 1248, 338, PC_UI_INK, PC_UI_TEXT_VALUE);
        PcUi_RoleLabel(font, "Left / Right: change season", 870, 366, 1248, 402, PC_UI_INK, PC_UI_TEXT_LABEL);
        if (habitatsMode) {
            if (!PokeDex_IsHabitatListEnabled(state->pokedex)) {
                PcUi_RoleLabel(font, "Habitat list not yet unlocked.", 850, 449, 1238, 490, PC_UI_DEFAULT_COLOR,
                               PC_UI_TEXT_BODY);
                return;
            }
            u16 zone =
                mapPlace == TOWNMAP_PLACE_NONE ? 0xffff : TownMapData_GetParam(townmap, mapPlace, TOWNMAP_PARAM_ZONE);
            for (u32 i = 0; habitatList && i < habitatList->count; i++) {
                const PokedexHabitatArea *area = &habitatList->areas[i];
                if (area->zone != zone)
                    continue;
                u32 seen = 0, caught = 0, total = 0;
                for (u32 j = 0; j < SDL_arraysize(area->species) && area->species[j].species; j++) {
                    bool present = false;
                    for (u32 kind = 0; kind < 3; kind++)
                        present |= area->species[j].present[season][kind] != 0;
                    if (!present)
                        continue;
                    u16 entry[32];
                    if (total >= areaPage * 4 && total < areaPage * 4 + 4) {
                        u32 row = total - areaPage * 4;
                        if (PokeDex_IsSeen(state->pokedex, area->species[j].species)) {
                            GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, area->species[j].species, entry,
                                                  SDL_arraysize(entry));
                            PcUi_RoleText(font, entry, 850, 450 + row * 28, 1238, 483 + row * 28,
                                          PokeDex_IsCaught(state->pokedex, area->species[j].species) ? PC_UI_SUCCESS
                                                                                                     : PC_UI_WHITE,
                                          PC_UI_TEXT_BODY);
                        } else
                            PcUi_RoleLabel(font, "?????", 850, 450 + row * 28, 1238, 483 + row * 28, PC_UI_MUTED,
                                           PC_UI_TEXT_BODY);
                    }
                    total++;
                    seen += PokeDex_IsSeen(state->pokedex, area->species[j].species);
                    caught += PokeDex_IsCaught(state->pokedex, area->species[j].species);
                }
                if (areaPage * 4 >= total)
                    areaPage = 0;
                SDL_snprintf(text, sizeof(text), "Seen %u/%u   Caught %u/%u", seen, total, caught, total);
                PcUi_RoleLabel(font, text, 850, 572, 1238, 605, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
                SDL_snprintf(text, sizeof(text), "%s  Page %u/%u", area->complete ? "Complete" : "In progress",
                             areaPage + 1, SDL_max(1u, (total + 3) / 4));
                PcUi_RoleLabel(font, text, 850, 602, 1210, 630, area->complete ? PC_UI_SUCCESS : PC_UI_CYAN,
                               PC_UI_TEXT_STAT_LABEL);
                return;
            }
            PcUi_RoleLabel(font, "Choose a visited habitat", 850, 449, 1238, 490, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
            PcUi_RoleLabel(font, "on the map.", 850, 482, 1238, 523, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
            return;
        }
        if (!areaCount) {
            PcUi_RoleLabel(font, "Area unknown.", 850, 449, 1238, 490, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
            PcUi_RoleLabel(font, "No wild habitat recorded", 850, 490, 1238, 530, PC_UI_DEFAULT_COLOR,
                           PC_UI_TEXT_VALUE);
            PcUi_RoleLabel(font, "for this season.", 850, 518, 1238, 558, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
        } else {
            for (u32 row = 0; row < 5 && areaPage * 5 + row < areaCount; row++) {
                u32 i = areaPage * 5 + row;
                u16 name[80];
                GFL_MsgDataLoadRawStr(placeNames, ZoneData_GetPlaceNameID(areaZones[i]), name, SDL_arraysize(name));
                PcUi_RoleText(font, name, 850, 449 + row * 26, 1238, 480 + row * 26, PC_UI_DEFAULT_COLOR,
                              PC_UI_TEXT_VALUE);
            }
            if (areaCount > 5) {
                SDL_snprintf(text, sizeof(text), "%u / %u  Click for next page", areaPage + 1, (areaCount + 4) / 5);
                PcUi_RoleLabel(font, text, 850, 592, 1210, 623, PC_UI_MUTED, PC_UI_TEXT_VALUE);
            }
        }
    } else {
        if (!formNames) {
            formNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEDEX_DETAIL_FORM, HEAPID_USER);
            formNameTable = GFL_ArcSysReadHeapNew(DEX_FORM_NAME_ARC, 0, HEAPID_USER);
        }
        SDL_snprintf(text, sizeof(text), "APPEARANCE %u / %u", variantIndex + 1, variantCount);
        PcUi_RoleLabel(font, text, 893, 257, 1248, 294, PC_UI_INK, PC_UI_TEXT_BODY);
        for (u32 i = 0; i < 3; i++)
            PcUi_Control((PcUiShape){ .rect = { 850, 295 + i * 38, 398, 38 } },
                         (PcUiAction){ i != 2 || variantCount > 1 ? PC_UI_ACTION_CUSTOM : PC_UI_ACTION_NONE,
                                       DEX_ANIMATION + (int)i });
        PcUi_RoleLabel(font, "Space", 862, 300, 930, 331, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, "Animation", 946, 300, 1115, 331, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, animationPlaying ? "Playing" : "Paused", 1130, 300, 1240, 331, PC_UI_LINK,
                       PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, "V", 884, 338, 930, 369, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, "View", 946, 338, 1115, 369, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, backView ? "Back" : "Front", 1130, 338, 1240, 369, PC_UI_LINK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, "C", 884, 376, 930, 407, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, "Compare", 946, 376, 1115, 407, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font,
                       variantCount < 2 ? "N/A"
                       : compareForms   ? "On"
                                        : "Off",
                       1130, 376, 1240, 407, variantCount > 1 ? PC_UI_LINK : PC_UI_INK_MUTED, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(font, "Left / Right: appearance", 850, 594, 1218, 624, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
        u32 first = variantIndex / 5 * 5;
        for (u32 row = 0; row < 5 && first + row < variantCount; row++) {
            u32 i = first + row;
            PcUi_Control((PcUiShape){ .rect = { 850, 449 + row * 26, 398, 26 } },
                         (PcUiAction){ PC_UI_ACTION_MENU, (int)i });
            const char *sex = variants[i].sex == 0 ? "Male" : variants[i].sex == 1 ? "Female" : "Genderless";
            if (getNumberOfForms(species) > 1) {
                u16 message = species;
                for (u32 f = 0; f < variants[i].form && message; f++)
                    message = formNameTable[message];
                u16 name[64];
                PcUi_RoleLabel(font, i == variantIndex ? ">" : "", 850, 449 + row * 26, 872, 483 + row * 26, PC_UI_CYAN,
                               PC_UI_TEXT_LABEL);
                // The original messages call every Unown letter "One form"; identify the visible glyph too.
                if (species == 201) {
                    SDL_snprintf(text, sizeof(text), "Form %c", "ABCDEFGHIJKLMNOPQRSTUVWXYZ!?"[variants[i].form]);
                    PcUi_RoleLabel(font, text, 874, 449 + row * 26, 1162, 483 + row * 26,
                                   i == variantIndex ? PC_UI_CYAN : PC_UI_WHITE, PC_UI_TEXT_LABEL);
                } else {
                    GFL_MsgDataLoadRawStr(formNames, message, name, SDL_arraysize(name));
                    PcUi_RoleText(font, name, 874, 449 + row * 26, 1162, 483 + row * 26,
                                  i == variantIndex ? PC_UI_CYAN : PC_UI_WHITE, PC_UI_TEXT_LABEL);
                }
                if (variants[i].rare)
                    PcUi_RoleLabel(font, "Shiny", 1168, 449 + row * 26, 1240, 483 + row * 26, PC_UI_WARNING,
                                   PC_UI_TEXT_CAPTION);
                continue;
            } else
                SDL_snprintf(text, sizeof(text), "%s%s  %s", i == variantIndex ? "> " : "  ", sex,
                             variants[i].rare ? "Shiny" : "Normal");
            PcUi_RoleLabel(font, text, 850, 449 + row * 26, 1238, 483 + row * 26,
                           i == variantIndex ? PC_UI_CYAN : PC_UI_WHITE, PC_UI_TEXT_VALUE);
        }
    }
}

static bool ReadPokemonImage(u16 species, u32 form, u32 sex, u32 rare, u16 *pixels) {
    NNSG2dCharacterData *chars;
    NNSG2dPaletteData *palette;
    void *charFile;
    void *paletteFile;
    u32 arc = GetPokemonGraphicsARCID();

    charFile = GFL_G2DIOReadOBJNCGR(
        arc, GetPokemonSingleCellCharacterDataNo(arc, species, form, sex, rare, POKEGRA_DIR_FRONT, FALSE), TRUE, &chars,
        HEAPID_USER);
    paletteFile = GFL_G2DIOReadNCLR(
        arc, GetPokemonPaletteDataNo(arc, species, form, sex, rare, POKEGRA_DIR_FRONT, FALSE), &palette, HEAPID_USER);
    bool ready = charFile && paletteFile;
    if (ready) {
        PokeGra_CellCharsToImage(chars, HEAPID_USER);
        const u8 *tiles = chars->pRawData;
        const u16 *colors = palette->pRawData;
        for (u32 y = 0; y < 96; y++) {
            for (u32 x = 0; x < 96; x++) {
                u32 tile = (y / 8) * 12 + x / 8;
                u32 offset = tile * 32 + (y & 7) * 4 + (x & 7) / 2;
                u32 index = (tiles[offset] >> ((x & 1) * 4)) & 15;
                pixels[y * 96 + x] = index ? colors[index] | 0x8000 : 0;
            }
        }
    }
    if (paletteFile)
        GFL_HeapFree(paletteFile);
    if (charFile)
        GFL_HeapFree(charFile);
    return ready;
}

static bool ImageBounds(const u16 *pixels, u32 *left, u32 *top, u32 *width, u32 *height) {
    u32 right = 0, bottom = 0;
    *left = *top = 96;
    for (u32 y = 0; y < 96; y++) {
        for (u32 x = 0; x < 96; x++) {
            if (!pixels[y * 96 + x])
                continue;
            *left = SDL_min(*left, x);
            *top = SDL_min(*top, y);
            right = SDL_max(right, x);
            bottom = SDL_max(bottom, y);
        }
    }
    if (*left == 96)
        return false;
    *width = right - *left + 1;
    *height = bottom - *top + 1;
    return true;
}

static void LoadPokemonArtwork(u16 species) {
    artworkSpecies = species;
    artworkReady = ReadPokemonImage(species, variants[variantIndex].form, variants[variantIndex].sex,
                                    variants[variantIndex].rare, artwork) &&
                   ImageBounds(artwork, &artworkLeft, &artworkTop, &artworkWidth, &artworkHeight);
}

static void DrawPokemonArtwork(u16 species) {
    if (artworkSpecies != species || !artworkReady)
        LoadPokemonArtwork(species);
    if (!artworkReady)
        return;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 y = 0; y < 96; y++)
        SDL_memcpy(atlas + (128 + y) * PC_UI_ATLAS_WIDTH, artwork + y * 96, 96 * sizeof(u16));
    float width = 170;
    float height = width * artworkHeight / artworkWidth;
    if (height > 190) {
        height = 190;
        width = height * artworkWidth / artworkHeight;
    }
    float floor = 473;
    float reflectionHeight = height * 0.35f;
    u32 first = PcGx.vertex_count;
    PcUi_Quad(585 - width / 2, floor, width, reflectionHeight, -0.39f, 0.75f, 0.75f, 0.75f, PcUi_FontTexture(),
              artworkLeft, 128 + artworkTop + artworkHeight, artworkWidth, -(float)artworkHeight);
    for (u32 i = first; i < PcGx.vertex_count; i++) {
        float y = (1 - PcGx.vertices[i].clip[1]) * PC_UI_HEIGHT / 2;
        PcGx.vertices[i].color[3] = SDL_clamp(0.3f * (1 - (y - floor) / reflectionHeight), 0.0f, 0.3f);
    }
    PcUi_Quad(585 - width / 2, floor - height, width, height, -0.4f, 1, 1, 1, PcUi_FontTexture(), artworkLeft,
              128 + artworkTop, artworkWidth, artworkHeight);
}

static void DrawFootprint(u16 species, u32 form) {
    if (footprintSpecies != species || footprintForm != form) {
        PokedexNativeFootprintResources resources;
        NNSG2dCharacterData *chars;
        NNSG2dCellDataBank *cells;
        NNSG2dPaletteData *palette;
        PokedexInfo_GetNativeFootprintResources(species, form, &resources);
        void *charFile = GFL_G2DIOReadOBJNCGR(resources.charArc, resources.charFile, TRUE, &chars, HEAPID_USER);
        void *cellFile = GFL_G2DIOReadNCER(resources.sharedArc, resources.cellFile, FALSE, &cells, HEAPID_USER);
        void *paletteFile = GFL_G2DIOReadNCLR(resources.sharedArc, resources.paletteFile, &palette, HEAPID_USER);
        SDL_zero(footprintPixels);
        footprintSpecies = species;
        footprintForm = form;
        if (charFile && cellFile && paletteFile) {
            const NNSG2dCellData *cell = NNS_G2dGetCellDataByIdx(cells, 0);
            const u8 *tiles = chars->pRawData;
            const u16 *colors = palette->pRawData;
            // The original cell assembles the two halves of the footprint from separate character rows.
            for (u16 i = 0; cell && i < cell->numOAMAttrs; i++) {
                GXOamAttr oam;
                NNS_G2dCopyCellAsOamAttr(cell, i, &oam);
                GXOamShape shape = (GXOamShape)(oam.attr0 | ((u32)oam.attr1 << 16));
                int width = NNS_G2dGetOamSizeX(&shape), height = NNS_G2dGetOamSizeY(&shape);
                int left = (oam.attr1 & 511), top = (oam.attr0 & 255);
                if (left >= 256)
                    left -= 512;
                if (top >= 128)
                    top -= 256;
                u32 base = (oam.attr2 & 1023) * (32u << cells->mappingMode);
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int dx = left + x + 8, dy = top + y + 8;
                        if (dx < 0 || dx >= 16 || dy < 0 || dy >= 16)
                            continue;
                        int sx = oam.attr1 & 0x1000 ? width - 1 - x : x;
                        int sy = oam.attr1 & 0x2000 ? height - 1 - y : y;
                        u32 offset = base + ((sy / 8) * (width / 8) + sx / 8) * 32 + (sy & 7) * 4 + (sx & 7) / 2;
                        if (offset >= chars->szByte)
                            continue;
                        u32 index = (tiles[offset] >> ((sx & 1) * 4)) & 15;
                        if (index)
                            footprintPixels[dy * 16 + dx] = colors[index] | 0x8000;
                    }
                }
            }
        }
        if (paletteFile)
            GFL_HeapFree(paletteFile);
        if (cellFile)
            GFL_HeapFree(cellFile);
        if (charFile)
            GFL_HeapFree(charFile);
    }
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 y = 0; y < 16; y++)
        SDL_memcpy(atlas + (224 + y) * PC_UI_ATLAS_WIDTH + 128, footprintPixels + y * 16, 16 * sizeof(u16));
    PcUi_Quad(1188, 293, 48, 48, -0.45f, 1, 1, 1, PcUi_FontTexture(), 128, 224, 16, 16);
}

static void DrawListIcon(const PokedexNativeListState *state, u16 species, u32 row, float y) {
    if (iconSpecies[row] != species) {
        u32 sex, rare, form;
        PokeDex_GetShownSexRareForm(state->pokedex, species, &sex, &rare, &form, HEAPID_USER);
        u16 pixels[96 * 96];
        u32 left, top, width, height;
        SDL_memset(iconPixels[row], 0, sizeof(iconPixels[row]));
        if (ReadPokemonImage(species, form, sex, rare, pixels) && ImageBounds(pixels, &left, &top, &width, &height)) {
            u32 extent = SDL_max(width, height);
            u32 scaledWidth = SDL_max(1u, width * 32 / extent);
            u32 scaledHeight = SDL_max(1u, height * 32 / extent);
            for (u32 iy = 0; iy < scaledHeight; iy++) {
                for (u32 ix = 0; ix < scaledWidth; ix++) {
                    iconPixels[row][(iy + (32 - scaledHeight) / 2) * 32 + ix + (32 - scaledWidth) / 2] =
                        pixels[(top + iy * height / scaledHeight) * 96 + left + ix * width / scaledWidth];
                }
            }
            iconSpecies[row] = species;
        }
    }
    u32 tx = 96 + (row % 4) * 32;
    u32 ty = 128 + (row / 4) * 32;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 iy = 0; iy < 32; iy++)
        SDL_memcpy(atlas + (ty + iy) * PC_UI_ATLAS_WIDTH + tx, iconPixels[row] + iy * 32, 32 * sizeof(u16));
    PcUi_Quad(54, y + 6, 48, 48, -0.45f, 1, 1, 1, PcUi_FontTexture(), tx, ty, 32, 32);
}

void PcDexUi_Draw(void) {
    PokedexNativeListState state;
    if (!CurrentState(&state)) {
        StopCry();
        PcDexMedia_Free();
        if (townmap) {
            TownMapData_Free(townmap);
            townmap = NULL;
        }
        if (habitatList) {
            GFL_HeapFree(habitatList);
            habitatList = NULL;
        }
        mapPlace = TOWNMAP_PLACE_NONE;
        habitatsMode = false;
        if (translatedNames) {
            GFL_MsgDataFree(translatedNames);
            translatedNames = NULL;
        }
        if (formNames) {
            GFL_MsgDataFree(formNames);
            GFL_HeapFree(formNameTable);
            formNames = NULL;
            formNameTable = NULL;
        }
        loadedLanguage = 0xff;
        if (categories) {
            GFL_MsgDataFree(categories);
            GFL_MsgDataFree(entries);
            GFL_MsgDataFree(placeNames);
            categories = entries = NULL;
            placeNames = NULL;
        }
        artworkReady = false;
        artworkSpecies = 0;
        sourceList = NULL;
        EndSearch();
        return;
    }
    if (!placeNames) {
        placeNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, HEAPID_USER);
    }
    PcGx.vertex_count = 0;
    if (!PcUi_Begin()) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Unable to load the bundled Pokédex font", sizeof(PcGx.error));
        return;
    }
    PcUi_BeginControls(sourceList, ControlPhase());
    PcGx.composite_scene = false;
    PcGx.dex_ui = true;
    PcGx.dex_tab = activeTab;
    PcGx.dex_filter_row = filtersOpen ? filterRow : 0xff;
    PcGx.dex_selected_row = 0xff;
    PcUiFrame_Header(state.font, "POKÉDEX", NULL);
    PcUi_RoleLabel(state.font, criteria.regional ? "UNOVA" : "NATIONAL", 449, 37, 615, 65, PC_UI_DEFAULT_COLOR,
                   PC_UI_TEXT_PANEL_TITLE);
    PcUi_RoleLabel(state.font, search[0] ? search : "Search Pokémon...", 780, 30, 1115, 62,
                   searching ? PC_UI_CYAN : PC_UI_MUTED, PC_UI_TEXT_VALUE);
    PcUiFrame_Footer(state.font,
                     filtersOpen                ? "Up/Down: Row   Left/Right: Value   X: Cancel"
                     : activeTab == PC_DEX_AREA ? "A: Location   H: Completion   Backspace: List page"
                                                : "Tab: Page   /: Search   S: Filters",
                     filtersOpen ? "Apply" : "Cry", true, true);
    static const PcUiShape tabs[] = { { { 795, 103, 160, 39 }, 0, 39, 0, 0 },
                                      { { 928, 103, 177, 39 }, 0, 39, 0, 39 },
                                      { { 1078, 103, 179, 39 }, 0, 39, 0, 39 } };
    static const char *const tabLabels[] = { "ENTRY", "AREA", "FORMS" };
    for (u32 i = 0; i < SDL_arraysize(tabs); i++) {
        PcUi_TabLabel(state.font, tabLabels[i], tabs[i], activeTab == i);
        if (!filtersOpen)
            PcUi_Control(tabs[i], (PcUiAction){ PC_UI_ACTION_TAB, (int)i });
    }
    if (!filtersOpen) {
        PcUi_Control((PcUiShape){ .rect = { 716, 23, 414, 46 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_SEARCH });
        PcUi_Control((PcUiShape){ .rect = { 420, 674, 290, 40 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_FILTERS });
    }
    if (!resultCount) {
        PcUi_RoleLabel(state.font, "No matching Pokemon.", 420, 310, 785, 370, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
        if (filtersOpen)
            DrawFilters(state.font, state.pokedex);
        else
            PcUi_RoleLabel(state.font, "Try another name or filter.", 835, 190, 1248, 240, PC_UI_INK, PC_UI_TEXT_VALUE);
        return;
    }
    u16 species = results[selected];
    LoadDetails(&state, species);
    if (variantCount < 2)
        compareForms = false;
    LoadLanguage();
    PcDexMedia_Play(animationPlaying && activeTab == PC_DEX_FORMS);
    if (activeTab == PC_DEX_AREA && !filtersOpen)
        DrawHabitatMap(&state);
    else if (PokeDex_IsSeen(state.pokedex, species)) {
        if (activeTab == PC_DEX_FORMS && !filtersOpen) {
            u32 personality = PokeDex_GetSpindaPersonality(state.pokedex, 0);
            PcDexMedia_SetPokemon(0, species, variants[variantIndex].form, variants[variantIndex].sex,
                                  variants[variantIndex].rare, personality, false, backView);
            if (compareForms && variantCount > 1) {
                u32 next = (variantIndex + 1) % variantCount;
                PcDexMedia_SetPokemon(1, species, variants[next].form, variants[next].sex, variants[next].rare,
                                      personality, false, backView);
            }
            PcDexMedia_Update();
            PcDexMedia_DrawPokemon(0, compareForms ? 501 : 585, 473, compareForms ? 130 : 170, 190);
            if (compareForms && variantCount > 1)
                PcDexMedia_DrawPokemon(1, 680, 473, 130, 190);
        } else
            DrawPokemonArtwork(species);
    }
    if (activeTab != PC_DEX_FORMS)
        DrawWave(state.font);

    u16 name[32];
    u16 data[256];
    char number[40];
    u32 start = selected > 4 ? selected - 4 : 0;
    if (start + 9 > resultCount)
        start = resultCount > 9 ? resultCount - 9 : 0;
    listStart = start;
    PcGx.dex_selected_row = selected - start;
    for (u32 row = 0; row < 9 && start + row < resultCount; row++) {
        u16 item = results[start + row];
        float y = 90 + row * 61;
        if (!filtersOpen)
            PcUi_Control((PcUiShape){ .rect = { 0, y + 3, 360, 56 }, .topRight = 20 },
                         (PcUiAction){ PC_UI_ACTION_ROW, (int)(start + row) });
        bool seen = PokeDex_IsSeen(state.pokedex, item);
        SDL_snprintf(number, sizeof(number), "%03u", dexNumbers[item]);
        if (seen)
            DrawListIcon(&state, item, row, y);
        PcUi_RoleLabel(state.font, number, 122, y + 16, 180, y + 53, PC_UI_MUTED, PC_UI_TEXT_VALUE);
        if (seen) {
            GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, item, name, SDL_arraysize(name));
            PcUi_RoleText(state.font, name, 190, y + 14, 354, y + 54, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CARD_TITLE);
        } else {
            PcUi_RoleLabel(state.font, "?????", 190, y + 14, 354, y + 54, PC_UI_MUTED, PC_UI_TEXT_VALUE);
        }
    }
    SDL_snprintf(number, sizeof(number), "Seen %u   Caught %u", seenCount, caughtCount);
    PcUi_RoleLabel(state.font, number, 445, 614, 777, 642, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    if (filtersOpen) {
        DrawFilters(state.font, state.pokedex);
        return;
    }
    PcUi_Control((PcUiShape){ .rect = { 375, 280, 55, 120 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_KEY_UP });
    PcUi_Control((PcUiShape){ .rect = { 742, 280, 54, 120 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_KEY_DOWN });
    if (activeTab != PC_DEX_AREA)
        PcUi_Control((PcUiShape){ .rect = { 470, 250, 230, 285 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
    SDL_snprintf(number, sizeof(number), "%03u", dexNumbers[species]);
    PcUi_RoleLabel(state.font, number, 893, 165, 1000, 198, PC_UI_INK, PC_UI_TEXT_VALUE);
    if (PokeDex_IsSeen(state.pokedex, species)) {
        GFL_MsgDataLoadRawStr(translatedNames, species, name, SDL_arraysize(name));
        for (u32 i = 0; i < SDL_arraysize(name) && name[i] != 0xffff && name[i] != 0; i++) {
            if (name[i] >= 'a' && name[i] <= 'z')
                name[i] -= 'a' - 'A';
        }
        PcUi_RoleText(state.font, name, 893, 206, 1245, 246, PC_UI_INK,
                      language < 6 ? PC_UI_TEXT_VALUE : PC_UI_TEXT_PANEL_TITLE);
        if (activeTab != PC_DEX_ENTRY) {
            DrawExtraPanel(&state, species);
            return;
        }
        PcUi_Control((PcUiShape){ .rect = { 1134, 165, 106, 33 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, DEX_LANGUAGE });
        static const char *const languages[] = { "JA", "FR", "DE", "IT", "ES", "KO", "EN" };
        PcUi_RoleLabel(state.font, languages[language], 1170, 169, 1220, 198, PC_UI_LINK, PC_UI_TEXT_LABEL);
        PcUi_RoleLabel(state.font, "L:", 1134, 169, 1170, 198, PC_UI_INK_MUTED, PC_UI_TEXT_LABEL);
        GFL_MsgDataLoadRawStr(categories, PokeDex_IsCaught(state.pokedex, species) ? species : 0, data,
                              SDL_arraysize(data));
        PcUi_RoleText(state.font, data, 895, 252, 1248, 294, PC_UI_INK, PC_UI_TEXT_VALUE);
        u32 form = variants[variantIndex].form;
        bool caught = PokeDex_IsCaught(state.pokedex, species);
        if (caught) {
            u32 type = PML_PersonalGetParamSingle(species, form, PERSONAL_TYPE_1);
            PcUi_TypeBadge(type, 0, language, 897, 297, 132, 40);
            u32 type2 = PML_PersonalGetParamSingle(species, form, PERSONAL_TYPE_2);
            if (type != type2)
                PcUi_TypeBadge(type2, 1, language, 1041, 297, 132, 40);
            DrawFootprint(species, form);
        }
        u32 height = PML_PersonalGetParamSingle(species, form, PERSONAL_HEIGHT);
        u32 weight = PML_PersonalGetParamSingle(species, form, PERSONAL_WEIGHT);
        SDL_snprintf(number, sizeof(number), "%u.%u", height / 10, height % 10);
        DrawMeasurement(state.font, "Height", caught ? number : "???", "m", 855);
        SDL_snprintf(number, sizeof(number), "%u.%u", weight / 10, weight % 10);
        DrawMeasurement(state.font, "Weight", caught ? number : "???", "kg", 1055);
        GFL_MsgDataLoadRawStr(entries, PokeDex_IsCaught(state.pokedex, species) ? species : 0, data,
                              SDL_arraysize(data));
        PcUi_RoleParagraph(state.font, data, 853, 445, 1238, 613, PC_UI_WHITE, PC_UI_TEXT_BODY);
    } else {
        PcUi_RoleLabel(state.font, "UNKNOWN", 893, 194, 1245, 246, PC_UI_INK, PC_UI_TEXT_VALUE);
        if (activeTab == PC_DEX_AREA && habitatsMode)
            DrawExtraPanel(&state, species);
        else
            PcUi_RoleLabel(state.font, "Not yet seen.", 893, 260, 1235, 310, PC_UI_INK, PC_UI_TEXT_VALUE);
    }
}
