#ifndef POKEBW2_FIELD_SCRCMD_MUSICAL_H
#define POKEBW2_FIELD_SCRCMD_MUSICAL_H

// Overlay 12's scrcmd_musical.c: the script commands of the musical. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

// What Scrcmd_Musical_MusicalGetInfo gets, the number the script passes: the shows played and won, the last show's
// points and the kinds of prop its Pokémon wore, the program, the props saved to wear, the party's eligibilities, and
// from 30 the running event's stage. The numbers are the scripts'.
enum {
    MUSICAL_INFO_SHOW_COUNT = 0,
    MUSICAL_INFO_WIN_COUNT = 1,
    MUSICAL_INFO_LAST_POINTS = 2,
    // The kind of prop the last show's Pokémon wore the most, MUSICAL_PROP_KIND_NONE when none or when several tie
    MUSICAL_INFO_MOST_PROP_KIND = 3,
    MUSICAL_INFO_PROGRAM = 4,
    // Sets the program to the argument
    MUSICAL_INFO_SET_PROGRAM = 5,
    MUSICAL_INFO_HAS_WORN_PROP = 6,
    MUSICAL_INFO_HAS_SHOT = 7,
    MUSICAL_INFO_LEAST_PROP_KIND = 8,
    // The first of the kinds worn the most, when several tie
    MUSICAL_INFO_FIRST_MOST_PROP_KIND = 9,
    MUSICAL_INFO_SECOND_MOST_PROP_KIND = 10,
    MUSICAL_INFO_CAN_JOIN_COUNT = 11,
    MUSICAL_INFO_CAN_JOIN_POKE = 12,
    MUSICAL_INFO_HAS_EQUIP = 13,
    MUSICAL_INFO_HAS_DIST_PROGRAM = 14,
    MUSICAL_INFO_HAS_PROP = 15,
    MUSICAL_INFO_PLAYER_POS = 30,
    MUSICAL_INFO_MAX_POINTS = 31,
    MUSICAL_INFO_MIN_POINTS = 32,
    // The points of the stage position at the argument, and its entry order
    MUSICAL_INFO_POINTS = 33,
    MUSICAL_INFO_ENTRY_INDEX = 35,
    // The program's top kind, the kind of prop the audience gives the most points to
    MUSICAL_INFO_TOP_KIND = 36,
    // The stage position that finished at the rank of the argument, and the best prop kind of the one it takes
    MUSICAL_INFO_POS_AT_RANK = 37,
    MUSICAL_INFO_BEST_PROP_KIND = 38,
    MUSICAL_INFO_OWNER_TRAINER_CLASS = 39,
};

// What Scrcmd_Musical_MusicalGetAudienceGift gets of the audience member at the argument: whether it has a gift, the
// message number of its comment, and the prop it gives
enum {
    MUSICAL_GIFT_HAS_GIFT = 0,
    MUSICAL_GIFT_COMMENT = 1,
    MUSICAL_GIFT_COMMENT_WITH_PROP = 2,
    MUSICAL_GIFT_HAS_PROP = 3,
    MUSICAL_GIFT_PROP = 4,
};

// What Scrcmd_Musical_WordSetMusicalInfo puts in the word set: the musical's name, a prop's, the program's title, the
// words of the kinds of prop, the owner's and the Pokémon's names, and the words of the three best kinds
enum {
    MUSICAL_WORD_NAME = 0,
    // The prop's name from the dressing room's bank
    MUSICAL_WORD_PROP_NAME = 1,
    MUSICAL_WORD_PROGRAM_TITLE = 2,
    // The word of the top kind: men, girls, women or boys
    MUSICAL_WORD_AUDIENCE = 3,
    // The word of the kind of prop at the argument: cool, cute, elegant or unique
    MUSICAL_WORD_STYLE = 4,
    MUSICAL_WORD_OWNER_NAME = 5,
    MUSICAL_WORD_POKE_NAME = 6,
    // The words of the top three kinds, which the show's comments use
    MUSICAL_WORD_RANKING = 7,
    // The prop's name from the script's bank, which gives it its article
    MUSICAL_WORD_PROP_NAME_ARTICLE = 8,
};

// What Scrcmd_Musical_MusicalComm does: makes and frees the musical's work, starts and ends the communication, opens
// the leader menus, sends a timing, waits for the others' scripts and Pokémon, starts the IR match, and reports the
// connection. 21 and 22 both report a network error, and nothing shows a difference between them; 100 does nothing.
enum {
    MUSICAL_COMM_CREATE = 0,
    MUSICAL_COMM_FREE = 1,
    MUSICAL_COMM_BOOT = 10,
    MUSICAL_COMM_QUIT = 11,
    MUSICAL_COMM_LEADER_MENU = 12,
    MUSICAL_COMM_SELECT_LEADER_MENU = 13,
    // Sends the timing number of the argument for the others to reach
    MUSICAL_COMM_SEND_TIMING = 14,
    MUSICAL_COMM_WAIT_SCRIPT = 15,
    MUSICAL_COMM_WAIT_BOOT = 16,
    MUSICAL_COMM_WAIT_POKES = 17,
    MUSICAL_COMM_START_IRC_MATCH = 18,
    MUSICAL_COMM_IS_LOST = 19,
    MUSICAL_COMM_IS_CONNECTED = 20,
    MUSICAL_COMM_CHECK_NET_ERROR = 21,
    MUSICAL_COMM_CHECK_NET_ERROR_2 = 22,
    MUSICAL_COMM_NONE = 100,
};

// What Scrcmd_Musical_DreamWorldItems does with the items the Dream World sent: whether there are any, whether some
// are not received yet, the list of them, receiving them, and the count and the name of the one not received
enum {
    DREAM_WORLD_ITEMS_HAS_ITEMS = 0,
    DREAM_WORLD_ITEMS_HAS_UNRECEIVABLE = 1,
    DREAM_WORLD_ITEMS_OPEN_LIST = 2,
    DREAM_WORLD_ITEMS_RECEIVE = 3,
    DREAM_WORLD_ITEMS_COUNT_UNRECEIVABLE = 4,
    DREAM_WORLD_ITEMS_ITEM_NAME = 5,
};

BOOL Scrcmd_Musical_MusicalCall(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalCallDressUp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalGetInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalGetAudienceGift(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalAddProp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_WordSetMusicalInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalClearAudienceGift(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalSelectPoke(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalComm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalIsPropOwned(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalGetOwnedPropCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_DreamWorldItems(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_MUSICAL_H
