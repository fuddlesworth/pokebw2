#ifndef POKEBW2_FIELD_SCRIPT_FIELD_SCRIPT_SUPERVISOR_H
#define POKEBW2_FIELD_SCRIPT_FIELD_SCRIPT_SUPERVISOR_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Layout and names reconstructed from the field script code and swan.
struct FieldScriptSupervisor {
    HeapID heapId;
    u8 vmCount;
    u8 padding;
    VM *vms[3];
    void (*postFunc)(GameEvent *event, void *arg);
    void *postArg;
    u32 unk18;
};

extern const char data_ov012_0216e190[];

FieldScriptSupervisor *FieldScriptSupervisor_Create(HeapID heapId);
void FieldScriptSupervisor_Free(FieldScriptSupervisor *supervisor);
BOOL FieldScriptSupervisor_Update(FieldScriptSupervisor *supervisor);
int FieldScriptSupervisor_AddVM(FieldScriptSupervisor *supervisor, VM *vm);
VM *FieldScript_CreateVM(HeapID heapId, ScriptWork *work, u16 zoneId, u16 scriptId, u32 featureLevel);
VM *FieldScriptSupervisor_GetVM(FieldScriptSupervisor *supervisor, int index);
void FieldScript_FreeVM(VM *vm);
void FieldScriptTerminator_ReplaceEvent(GameEvent *event, void *arg);
void FieldScriptSupervisor_SetPostEvent(FieldScriptSupervisor *supervisor, GameEvent *event);
void FieldScriptSupervisor_CallPostFunc(FieldScriptSupervisor *supervisor, GameEvent *event);
BOOL FieldScriptSupervisor_HasPostFunc(FieldScriptSupervisor *supervisor);
u16 FieldScript_GetZoneIDFromGSys(GameSystem *gsys);

#endif // POKEBW2_FIELD_FIELD_SCRIPT_SUPERVISOR_H
