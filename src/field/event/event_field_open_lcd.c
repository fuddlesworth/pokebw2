// The event that opens the field again and restores its screens. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the file's name is descriptive
#include "types.h"
#include "field/field.h"
#include "field/field_event.h"
#include "gfl/gx_layers.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
} FieldOpenRestoreLCDData;

static GameEventReturnCode EventFieldOpenRestoreLCD_Callback(GameEvent *event, u32 *state, void *work);

GameEvent *EventFieldOpenRestoreLCD_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldOpenRestoreLCD_Callback, sizeof(FieldOpenRestoreLCDData));
    FieldOpenRestoreLCDData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->gameData = GSYS_GetGameData(gsys);
    return event;
}

// EventFieldOpenRestoreLCD_Callback's steps: the field's opening, and the screens' restore
enum {
    FIELD_OPEN_LCD_STEP_OPEN_FIELD,
    FIELD_OPEN_LCD_STEP_RESTORE_SURFACE,
    FIELD_OPEN_LCD_STEP_END,
};

static GameEventReturnCode EventFieldOpenRestoreLCD_Callback(GameEvent *event, u32 *state, void *work) {
    FieldOpenRestoreLCDData *data = work;

    switch (*state) {
    case FIELD_OPEN_LCD_STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(data->gsys));
        (*state)++;
        break;
    case FIELD_OPEN_LCD_STEP_RESTORE_SURFACE:
        FieldG3D_RestoreSurface(GSYS_GetField(data->gsys));
        (*state)++;
        break;
    case FIELD_OPEN_LCD_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void FieldG3D_RestoreSurface(Field *field) {
    u32 enabledA = GFL_BGSysGetEnabledBGsA();
    u32 enabledB = GFL_BGSysGetEnabledBGsB();

    FieldG2D_SetLCDConfig();
    GFL_BGSysSetEnabledBGsA(enabledA);
    GFL_BGSysSetEnabledBGsB(enabledB);
    FieldG2D_Prepare3DSurface(field);
}
