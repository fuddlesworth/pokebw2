// Makes the Pokémon of a Pokémon gift. The file's name is descriptive
#include "types.h"
#include "app/mystery_gift/mystery_gift_data.h"
#include "app/mystery_gift/mystery_gift_pokemon.h"

// The body is the maker that overlay 197 also inlines: its early returns all leave through one exit, which the
// original only gets from inlining
PartyPkm *CreateMysteryGiftPokemon(const MysteryGift *gift, HeapID heapId, GameData *gameData) {
    return Mystery_CreateGiftPokemon(gift, heapId, gameData);
}
