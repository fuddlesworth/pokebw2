#include "menu_ui.h"
#include "app/config_panel.h"
#include "app/itemmenu.h"
#include "app/p_status.h"
#include "app/pms_input.h"
#include "app/townmap.h"
#include "field/mailsys.h"
#include "gx_backend.h"
#include "menu_apps.h"
#include "save_ui.h"
#include "settings_ui.h"
#include "trainer_ui.h"
#include "ui_widgets.h"

typedef enum {
    MENU_NONE,
    MENU_SAVE,
    MENU_TRAINER,
    MENU_BAG,
    MENU_SUMMARY,
    MENU_MAIL,
    MENU_PHRASE,
    MENU_MAP,
    MENU_SETTINGS
} MenuKind;
typedef struct {
    void (*reset)(void);
    bool (*draw)(void);
    Uint16 (*filter)(Uint16 keys);
    bool (*event)(PcVideo *video, const SDL_Event *event);
} MenuController;
static const MenuController controllers[] = {
    [MENU_SETTINGS] = { PcSettingsUi_Reset, PcSettingsUi_Draw, PcSettingsUi_FilterKeys, PcSettingsUi_HandleEvent },
    [MENU_BAG] = { PcMenuBag_Reset, PcMenuBag_Draw, PcMenuBag_FilterKeys, PcMenuBag_HandleEvent },
    [MENU_SUMMARY] = { PcMenuSummary_Reset, PcMenuSummary_Draw, PcMenuSummary_FilterKeys, PcMenuSummary_HandleEvent },
    [MENU_MAIL] = { PcMenuMail_Reset, PcMenuMail_Draw, PcMenuMail_FilterKeys, PcMenuMail_HandleEvent },
    [MENU_PHRASE] = { PcMenuPhrase_Reset, PcMenuPhrase_Draw, PcMenuPhrase_FilterKeys, PcMenuPhrase_HandleEvent },
    [MENU_MAP] = { PcMenuMap_Reset, PcMenuMap_Draw, PcMenuMap_FilterKeys, PcMenuMap_HandleEvent }
};
static MenuKind active;
static const void *owner;

static MenuKind Current(const void **current) {
    *current = NULL;
    ConfigPanelNativeState settings;
    if (ConfigPanel_GetNativeState(&settings)) {
        *current = settings.font;
        return MENU_SETTINGS;
    }
    if (PcSaveUi_IsActive())
        return MENU_SAVE;
    if (PcTrainerUi_IsActive())
        return MENU_TRAINER;
    if ((*current = PMSInput_GetNativeWork()))
        return MENU_PHRASE;
    if ((*current = MailSys_GetNativeWork()))
        return MENU_MAIL;
    if ((*current = ItemMenu_GetNativeWork()))
        return MENU_BAG;
    if ((*current = PStatus_GetNativeWork()))
        return MENU_SUMMARY;
    TownMapNativeState map;
    if (TownMap_GetNativeState(&map)) {
        *current = map.font;
        return MENU_MAP;
    }
    return MENU_NONE;
}

static MenuKind Sync(void) {
    const void *current;
    MenuKind kind = Current(&current);
    if (active != kind || owner != current) {
        if (controllers[active].reset)
            controllers[active].reset();
        active = kind;
        owner = current;
    }
    return kind;
}

bool PcMenuUi_IsActive(void) {
    const void *current;
    return Current(&current) != MENU_NONE;
}
Uint16 PcMenuUi_FilterKeys(Uint16 keys) {
    MenuKind kind = Sync();
    if (kind == MENU_SAVE)
        return PcSaveUi_FilterKeys(keys);
    PcSaveUi_FilterKeys(0);
    if (kind == MENU_TRAINER)
        return PcTrainerUi_FilterKeys(keys);
    PcTrainerUi_FilterKeys(0);
    return controllers[kind].filter ? controllers[kind].filter(keys) : keys;
}
bool PcMenuUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    MenuKind kind = Sync();
    if (kind == MENU_SAVE)
        return PcSaveUi_HandleEvent(video, event);
    if (kind == MENU_TRAINER)
        return PcTrainerUi_HandleEvent(video, event);
    return controllers[kind].event && controllers[kind].event(video, event);
}
bool PcMenuUi_Draw(void) {
    MenuKind kind = Sync();
    if (kind == MENU_SAVE)
        return PcSaveUi_Draw();
    if (kind == MENU_TRAINER)
        return PcTrainerUi_Draw();
    if (kind == MENU_NONE)
        return false;
    PcGx.vertex_count = 0;
    PcGx.composite_scene = false;
    if (!PcUi_Begin()) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Unable to load the bundled menu font", sizeof(PcGx.error));
        return true;
    }
    return controllers[kind].draw();
}
