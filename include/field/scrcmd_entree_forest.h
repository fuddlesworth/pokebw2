#ifndef POKEBW2_FIELD_SCRCMD_ENTREE_FOREST_H
#define POKEBW2_FIELD_SCRCMD_ENTREE_FOREST_H

// Overlay 12's scrcmd_entree_forest.c (a descriptive name): the Entree Forest's script commands. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

// What Scrcmd_EntreeForest_EntreeForestGetInfo asks the forest, by the mode the script passes: whether the player's
// area is the top one or the entrance, the talked-to Pokémon's species and form, and whether it can join the party,
// which mode 0 reports TRUE for every actor and global script 10490 branches on before it offers to add the Pokémon
enum {
    ENTREE_FOREST_INFO_CAN_JOIN = 0,
    ENTREE_FOREST_INFO_IS_TOP_AREA = 1,
    ENTREE_FOREST_INFO_IS_ENTRANCE = 2,
    ENTREE_FOREST_INFO_TALK_ACTOR_SPECIES = 3,
    ENTREE_FOREST_INFO_TALK_ACTOR_FORM = 4,
};

BOOL Scrcmd_EntreeForest_EntreeForestStartBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_EntreeForestSpawnAllPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_EntreeForestRemovePkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_EntreeForestToggleKeepPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_WordSetLoadEntreeForestPkmName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_EntreeForestGetInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_MapChangeEntreeForest(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_ENTREE_FOREST_H
