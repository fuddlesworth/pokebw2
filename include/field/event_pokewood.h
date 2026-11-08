#ifndef POKEBW2_FIELD_EVENT_POKEWOOD_H
#define POKEBW2_FIELD_EVENT_POKEWOOD_H

#include "types.h"
#include "app/pkwd_book.h"
#include "app/pkwd_briefing.h"
#include "app/pkwd_record_monitor.h"
#include "app/pkwd_selectscreen.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 23's event_pokewood.c, which runs Pokéstar Studios' movies for its script plugin (overlay 62)

#define OVERLAY_EVENT_POKEWOOD OVERLAY_ID(23)

// The two sides in a movie cast file, separate from the source of the player's cast
enum {
    POKEWOOD_CAST_SIDE_PLAYER = 0,
    POKEWOOD_CAST_SIDE_OPPONENT = 1,
};

// The cast of one side of a movie as a party
void EventPokewood_CreateCastParty(PlayerInfo *playerInfo, PokeParty *dest, HeapID heapId, u32 movie, u32 side);
void EventPokewood_CreateCastPartyFromSave(PlayerInfo *playerInfo, PokeParty *dest, HeapID heapId, u32 movie, u32 side);
// Unlocks the movies whose conditions the player now meets, after the movie was filmed with the result and score
void EventPokewood_UnlockMovies(PokewoodSave *save, GameData *gameData, u32 movie, u32 result, s32 score);
// How the movie unlocks
enum {
    POKEWOOD_UNLOCK_NONE = 0,
    POKEWOOD_UNLOCK_PREVIOUS_HIT = 1,
    POKEWOOD_UNLOCK_EVENT_FLAG = 2,
    POKEWOOD_UNLOCK_MIN_GRADE = 3,
    POKEWOOD_UNLOCK_HIGH_SCORE = 4,
    POKEWOOD_UNLOCK_ALL_PREVIOUS_HITS = 5,
    POKEWOOD_UNLOCK_FIRST_MOVIE_WATCHED = 6,
    POKEWOOD_UNLOCK_HIT_AND_EVENT_FLAG = 7,
};

u8 EventPokewood_GetUnlockKind(u32 movie);

// Events for GameEvent_CreateOverlayDelegate
GameEvent *EventPokewood_CreateFilmEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateWatchSlotEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateWatchBlockEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateMoviePickerEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateMenuEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateCastSelectEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateScreenEvent(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_POKEWOOD_H
