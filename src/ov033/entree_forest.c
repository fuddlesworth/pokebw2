#include "types.h"
#include "gfl/arc_util.h"
#include "system/game_data.h"
#include "save/player_info.h"
#include "pml/poke_party.h"
#include "gfl/heap.h"
#include "gfl/arc.h"
#include "field/entree_forest.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/symbol_map.h"
#include "field/zone.h"
#include "gfl/random.h"
#include "system/tpoke_data.h"
#include "struct_decls.h"

typedef struct {
    u16 moveCode;
    u8 areaW;
    u8 areaH;
} EntreeForestActorAppearance;

extern const EntreeForestActorAppearance data_ov033_0217c354[];

extern const ZoneNPC data_ov033_0217c374;

FieldActor *EntreeForest_SpawnPkmActor(EntreeForestSpawnContext *context, const EntreeForestPokemon *pokemon, u16 index,
                                      u16 x, u16 z, s32 y) {
    const EntreeForestPokemon *mon;
    const EntreeForestActorAppearance *appearance;
    MMSys *actorSystem;
    u16 zoneId;
    ZoneNPC npc;
    u32 actorIdBase;

    mon = pokemon;
    appearance = &data_ov033_0217c354[mon->actorType];
    actorSystem = Field_GetActorSystem(context->field);
    zoneId = Field_GetPlayerStateZoneID(context->field);
    npc = data_ov033_0217c374;
    npc.uid = index;
    npc.modelId = GetPokemonFieldOBJCODE(context->pokemonData, mon->species, mon->sex, mon->form);
    npc.moveCode = appearance->moveCode;
    actorIdBase = context->actorIdBase;
    npc.areaW = appearance->areaW;
    npc.areaH = appearance->areaH;
    npc.param0 = actorIdBase + index;
    npc.param1 = pokemon->raw;
    npc.param2 = pokemon->raw >> 16;
    npc.pos.grid.x = x;
    npc.pos.grid.z = z;
    npc.pos.grid.y = y;
    return CreateNewActorByEntityNoWKOBJCODE(actorSystem, &npc, zoneId);
}

void GetEntreeForestActorParamBits(u32 *paramBits, FieldActor *actor) {
    u32 low = GetActorUserParam(actor, 1);
    u32 high = GetActorUserParam(actor, 2);

    *paramBits = low | (high << 16);
}

u16 GetActorUserParam0(FieldActor *actor) {
    return GetActorUserParam(actor, 0);
}

PartyPkm *func_ov033_02176bd0(HeapID heapId, GameData *gameData, const EntreeForestPokemon *pokemon) {
    u8 *levels;
    u8 level;
    u32 id;
    u32 pid;
    PartyPkm *pkm;

    levels = GFL_ArcSysReadHeapNewLZ(0xdf, 0, 0, heapId);
    level = levels[pokemon->species];
    GFL_HeapFree(levels);
    id = getIDAsUInt(GetGameDataPlayerInfo(gameData));
    pid = PML_GenPID(id, pokemon->species, pokemon->form, pokemon->sex, 0, 0);
    pkm = PokeParty_NewTempPkm(pokemon->species, level, -1, heapId);
    PokeParty_CreatePkm(pkm, pokemon->species, level, id, PKM_IVS_RANDOM, pid);
    PokeParty_SetHiddenAbil(pkm, pokemon->species, pokemon->form);
    PokeParty_ChangeForme(pkm, pokemon->form);
    if (pokemon->move != 0) {
        if (PokeParty_LearnMove(pkm, pokemon->move) == 0xffff) {
            PokeParty_SetLastMove(pkm, pokemon->move);
        }
    }
    return pkm;
}

void EntreeForest_SpawnAllPokemon(Field *field, u32 actorIdBase, const EntreeForestPokemon *pokemon, u32 unused, u32 area) {
    const u8 *positions;
    EntreeForestSpawnContext context;
    FieldActor *playerActor;
    u16 playerX;
    u16 playerZ;
    s32 i;
    s32 offsetX;
    s32 offsetZ;
    s32 x;
    s32 z;

    if (func_ov012_02160974(area)) {
        positions = data_ov033_0217c3c0;
    } else if (func_ov012_0216099c(area)) {
        positions = data_ov033_0217c340;
    } else {
        positions = data_ov033_0217c398;
    }
    context.pokemonData = LoadTPokeData(Field_GetHeapID(field));
    context.field = field;
    context.actorIdBase = actorIdBase;
    playerActor = FieldPlayer_GetActor(Field_GetPlayer(field));
    playerX = GetGPosX(playerActor);
    playerZ = GetGPosZ(playerActor);
    for (i = 0; i < 20; i++) {
        if (pokemon[i].species != 0) {
            offsetX = GFL_RandomLCAlt(2) - 1;
            offsetZ = GFL_RandomLCAlt(2) - 1;
            x = positions[i * 2] + offsetX;
            z = positions[i * 2 + 1] + offsetZ;
            if (playerX == x && playerZ == z) {
                if (offsetX != 0) {
                    x -= offsetX;
                } else if (offsetZ != 0) {
                    z -= offsetZ;
                } else {
                    x++;
                }
            }
            EntreeForest_SpawnPkmActor(&context, &pokemon[i], i, x, z, 0);
        }
    }
    FreeTPokeData(context.pokemonData);
}
