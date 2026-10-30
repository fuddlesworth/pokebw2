#ifndef POKEBW2_FIELD_EVENT_SAVE_H
#define POKEBW2_FIELD_EVENT_SAVE_H

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

typedef struct ReportWork ReportWork;

struct EventSaveWork {
    SaveControl *save;
    GameSystem *gameSystem;
    Field *field;
    FieldMsgBG *msgBgSys;
    u16 heapId;
    u16 pad12;
    u32 *result;
    // The subscreen to go back to
    u32 screenId;
    ReportWork *report;
};

GameEvent *EventSave_Create(GameSystem *gsys, Field *field, u16 heapId, FieldMsgBG *msgBgSys, u32 screenId,
                            u32 *result);
GameEventReturnCode EventSave_Callback(GameEvent *event, u32 *state, void *data);
// Callbacks of the save in FIELD_PROC_LINK_LIST
void FieldProcLink_CallInfDungeonRetire(FieldAppCallWork *work, s32 appParam);
void FieldProcLink_FreeParam(void *param);
// What EventSave_Update returns: done and back on the field (saved or not), backed out to the menu, or still running
enum {
    EVENT_SAVE_RESULT_DONE,
    EVENT_SAVE_RESULT_BACK,
    EVENT_SAVE_RESULT_RUNNING,
};
// Overlay 12's report_event.c: the report screen, which asks whether to save and saves, an EVENT_SAVE_RESULT_*
u32 EventSave_Update(EventSaveWork *work, u32 *state);

#ifdef POKEBW2_PC
typedef enum {
    REPORT_NATIVE_OPENING,
    REPORT_NATIVE_CONFIRM,
    REPORT_NATIVE_WRITING,
    REPORT_NATIVE_SAVED,
    REPORT_NATIVE_ERROR,
    REPORT_NATIVE_BLOCKED
} ReportNativePhase;

typedef struct {
    GameSystem *gameSystem;
    Font *font;
    AppTaskMenu *menu;
    ReportNativePhase phase;
    BOOL ready;
    u16 message[512];
} ReportNativeState;

BOOL Report_IsNativeActive(void);
BOOL Report_GetNativeState(ReportNativeState *state);
#endif

#endif // POKEBW2_FIELD_EVENT_SAVE_H
