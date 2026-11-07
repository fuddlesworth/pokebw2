#include "types.h"
#include "app/medal_info.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/resort.h"
#include "field/scrcmd_resort.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The Join Avenue's command that shows a visitor's medals

static void ResortCmd_FreeMedalInfo(void *arg);
static ResortPerson *ResortCmd_GetMedalPerson(Field *field, FieldScriptEnv *env);

BOOL Scrcmd_Medalinfo_ShowMedalInfo(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MedalInfoParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MedalInfoParam), TRUE, "scrcmd_medalinfo.c", 88);
    ResortPerson *person = ResortCmd_GetMedalPerson(field, env);
    u16 date = ResortPerson_GetParam(person, JOIN_AVE_PARAM_MEDAL_DATE, NULL);
    u16 name[8];
    u16 medal;
    u16 medalCount;
    u16 rank;

    ResortPerson_GetParam(person, JOIN_AVE_PARAM_NAME, name);
    medal = ResortPerson_GetParam(person, JOIN_AVE_PARAM_LAST_MEDAL, NULL);
    medalCount = ResortPerson_GetParam(person, JOIN_AVE_PARAM_MEDAL_COUNT, NULL);
    rank = ResortPerson_GetParam(person, JOIN_AVE_PARAM_MEDAL_RANK, NULL);
    param->nameBuf = GFL_StrBufCreate(8, HEAPID_GAMEEVENT);
    GFL_StrBufLoadString(param->nameBuf, name);
    param->name = param->nameBuf;
    param->medal = medal;
    param->year = (date >> 9) & 0x7f;
    param->month = (date >> 5) & 0xf;
    param->day = date & 0x1f;
    param->medalCount = medalCount;
    param->rank = rank;
    param->parentHeapId = 1;
    param->mode = 0;
    param->gameData = gameData;
    param->gsys = gsys;
    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_MEDAL_INFO,
                                                                                 &MedalInfo_ProcFunctions, param,
                                                                                 ResortCmd_FreeMedalInfo, param));
    return TRUE;
}

static void ResortCmd_FreeMedalInfo(void *arg) {
    MedalInfoParam *param = arg;

    GFL_StrBufFree(param->nameBuf);
    GFL_HeapFree(param);
}

static ResortPerson *ResortCmd_GetMedalPerson(Field *field, FieldScriptEnv *env) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        people = ResortField0_GetPeople(field);
    } else if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = ResortField_GetPeople(field);
    }
    return ResortSys_GetScriptPerson(env, people);
}
