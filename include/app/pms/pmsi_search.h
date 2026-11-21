#ifndef POKEBW2_APP_PMS_PMSI_SEARCH_H
#define POKEBW2_APP_PMS_PMSI_SEARCH_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The phrase input's search, of overlay 185's pmsi_search.c: the player types the first letters of a word, and the
// search lists the unlocked words that start with them. The names are ours, guessed

// How many letters the player can type
#define PMSI_SEARCH_INPUT_MAX 13
// No letter
#define PMSI_SEARCH_INPUT_NONE 0xff

PMSISearch *PMSISearch_Create(const PMSInputWork *mwk, const PMSInputData *dwk, HeapID heapId);
void PMSISearch_Delete(PMSISearch *ss);
// Types the letter of an initial
void PMSISearch_AddChar(PMSISearch *ss, u16 initial);
// Erases the last letter, or returns FALSE if there is none
BOOL PMSISearch_DelChar(PMSISearch *ss);
u8 PMSISearch_GetInputLen(PMSISearch *ss);
void PMSISearch_Reset(PMSISearch *ss);
// The letters typed
void PMSISearch_GetInputStr(PMSISearch *ss, StrBuf *buf);
// Lists the words that start with the letters typed, and returns whether there are any
BOOL PMSISearch_Search(PMSISearch *ss);
u8 PMSISearch_GetResultCount(PMSISearch *ss);
void PMSISearch_GetResultStr(PMSISearch *ss, u8 index, StrBuf *buf);
u16 PMSISearch_GetResultWord(PMSISearch *ss, u32 index);

#endif // POKEBW2_APP_PMSI_SEARCH_H
