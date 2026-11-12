#ifndef POKEBW2_FIELD_SCRCMD_POKEMON_H
#define POKEBW2_FIELD_SCRCMD_POKEMON_H

// Overlay 12's scrcmd_pokemon.c: the script commands of the party's and the boxes' Pokémon, and of the money. Names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

// What Scrcmd_Pokemon_PokePartyAdjustHappiness does with the happiness: sets it, adds to it, or subtracts from it
enum {
    POKE_PARTY_HAPPINESS_SET = 0,
    POKE_PARTY_HAPPINESS_ADD = 1,
    POKE_PARTY_HAPPINESS_SUBTRACT = 2,
};

// The ability argument of Scrcmd_Pokemon_PokePartyAddEx and _BoxAddEx: 3 asks for the species' hidden ability, which
// the scripts pass for the Pokémon that have one. Any value from 3 up is stored as the second ability
#define POKE_ABILITY_HIDDEN 3

// What Scrcmd_Pokemon_PokePartyGetMemberByType looks for in the party: the first slot, the first Pokémon that is not
// an egg, or the first that can battle
enum {
    PARTY_MEMBER_FIRST = 0,
    PARTY_MEMBER_NON_EGG = 1,
    PARTY_MEMBER_BATTLE_READY = 2,
};

// What Scrcmd_Pokemon_PokePartyGetCount counts: all the party's Pokémon, the ones that are not eggs, the ones that
// can battle, the eggs that are not bad eggs, the bad eggs, and the slots left
enum {
    POKE_PARTY_COUNT_ALL = 0,
    POKE_PARTY_COUNT_NON_EGGS = 1,
    POKE_PARTY_COUNT_BATTLE_READY = 2,
    POKE_PARTY_COUNT_EGGS = 3,
    POKE_PARTY_COUNT_BAD_EGGS = 4,
    POKE_PARTY_COUNT_EMPTY_SLOTS = 5,
};

// What Scrcmd_Pokemon_BoxGetCount counts: all the boxed Pokémon, the ones that are not eggs, the eggs, and the slots
// left. Its modes 1 and 2 count the same, and 4 has no case
enum {
    BOX_COUNT_ALL = 0,
    BOX_COUNT_NON_EGGS = 1,
    BOX_COUNT_NON_EGGS_2 = 2,
    BOX_COUNT_EGGS = 3,
    BOX_COUNT_EMPTY_SLOTS = 5,
};

BOOL CheckGetPartyPokemon(FieldScriptEnv *env, u32 index, PartyPkm **pkm);
u32 GetScrPokeStat(FieldScriptEnv *env, u32 index, u32 param);
BOOL CheckPokeMoveLearned_NonEgg(PartyPkm *pkm, u16 move);
BOOL Scrcmd_Pokemon_PokePartyGetTypes(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyRecoverAll(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetEVTotal(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsFullHP(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsFullPP(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsEgg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetHappiness(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAdjustHappiness(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetForme(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMemberByType(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetCountBySpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_BoxGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyFindBySpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyHasMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyHasMoveAny(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAddEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAddNPoke(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMoveCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyLearnMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsOriginGame(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartySetForme(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyChangeRotomForme(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoveReminderCheckPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsFromWhiteForest(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMetDate(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartySetIV(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetParam(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_BoxAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_BoxAddEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAddEgg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoneyAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoneySub(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoneyCheck(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_POKEMON_H
