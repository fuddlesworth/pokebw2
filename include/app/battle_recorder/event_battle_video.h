#ifndef POKEBW2_APP_BATTLE_RECORDER_EVENT_BATTLE_VIDEO_H
#define POKEBW2_APP_BATTLE_RECORDER_EVENT_BATTLE_VIDEO_H

// Battle Video, started by the NetConnectBattleVideo script command

#include "types.h"
#include "struct_decls.h"

typedef struct {
    Field *field;
    u32 mode;
} EventBattleVideoArgs;

GameEvent *EventBattleVideo_Create(GameSystem *gsys, Field *field, u32 mode);
GameEvent *EventBattleVideo_CreateFromArgs(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_BATTLE_VIDEO_H
