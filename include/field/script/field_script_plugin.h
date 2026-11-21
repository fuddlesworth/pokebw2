#ifndef POKEBW2_FIELD_SCRIPT_FIELD_SCRIPT_PLUGIN_H
#define POKEBW2_FIELD_SCRIPT_FIELD_SCRIPT_PLUGIN_H

#include "types.h"
#include "field/script/field_script.h"
#include "struct_decls.h"

struct ScriptPluginEntry {
    const FieldScriptCommand *commands;
    const u16 *zones;
    u32 zoneCount;
    s32 overlay0;
    s32 overlay1;
};

extern const ScriptPluginEntry SCRIPT_PLUGIN_TABLE[];

// The plugins' command tables, each in its plugin's overlay
extern const FieldScriptCommand BSUBWAY_SCRIPT_COMMANDS[];
extern const FieldScriptCommand PLEASURE_BOAT_SCRIPT_COMMANDS[];
extern const FieldScriptCommand POKEMON_LEAGUE_SCRIPT_COMMANDS[];
extern const FieldScriptCommand PAL_PARK_SCRIPT_COMMANDS[];
extern const FieldScriptCommand ABYSSAL_RUINS_SCRIPT_COMMANDS[];
extern const FieldScriptCommand WBT_SCRIPT_COMMANDS[];
extern const FieldScriptCommand RESORT_SCRIPT_COMMANDS[];
extern const FieldScriptCommand BLACK_TOWER_SCRIPT_COMMANDS[];
extern const FieldScriptCommand POKEWOOD_SCRIPT_COMMANDS[];
extern const FieldScriptCommand BADGE_GATE_SCRIPT_COMMANDS[];
extern const FieldScriptCommand PLASMA_FRIGATE_SCRIPT_COMMANDS[];
extern const FieldScriptCommand POKEMON_CENTER_SCRIPT_COMMANDS[];
extern const FieldScriptCommand PLUGIN_14_SCRIPT_COMMANDS[];
extern const FieldScriptCommand PLUGIN_15_SCRIPT_COMMANDS[];
extern const FieldScriptCommand PLUGIN_16_SCRIPT_COMMANDS[];

void SetScrPluginByZone(GameData *gameData, u16 zoneId);
void LoadScrPluginOverlays(GameData *gameData);
void UnloadScrPluginOverlays(GameData *gameData);
const FieldScriptCommand *GetCurrentScrPluginTable(GameData *gameData);
u32 GetCurrentScrPluginCmdCount(GameData *gameData);

#endif // POKEBW2_FIELD_FIELD_SCRIPT_PLUGIN_H
