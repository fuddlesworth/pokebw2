#ifndef POKEBW2_APP_POKEDEX_SEARCH_SEQ_H
#define POKEBW2_APP_POKEDEX_SEARCH_SEQ_H

#include "types.h"
#include "app/pokedex_search_main.h"

// pokedex_search_seq.c: the sequences of the Pokédex's search screen. The ROM doesn't name this file;
// pokedex_search_seq.c is a guess after pokedex_search_main.c. The names are ours

// The list page's initialization (initSeq)
enum {
    POKEDEX_SEARCH_PAGE_INIT_SETUP,
    POKEDEX_SEARCH_PAGE_INIT_DRAW,
};

// What the list scroll waits for (subSeq)
enum {
    POKEDEX_SEARCH_SCROLL_WAIT_LIST,
    POKEDEX_SEARCH_SCROLL_WAIT_BUTTON,
};

// The cursor palette's two flashes (buttonSeq)
enum {
    POKEDEX_SEARCH_BUTTON_ANM_OFF_FIRST,
    POKEDEX_SEARCH_BUTTON_ANM_PRESSED_FIRST,
    POKEDEX_SEARCH_BUTTON_ANM_OFF_SECOND,
    POKEDEX_SEARCH_BUTTON_ANM_PRESSED_SECOND,
    POKEDEX_SEARCH_BUTTON_ANM_RESTORE,
};

// Runs a frame; FALSE once the screen has ended
BOOL PokedexSearchSeq_Main(PokedexSearchWork *wk);

#endif // POKEBW2_APP_POKEDEX_SEARCH_SEQ_H
