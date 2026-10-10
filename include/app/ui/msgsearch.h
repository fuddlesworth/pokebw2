#ifndef POKEBW2_APP_UI_MSGSEARCH_H
#define POKEBW2_APP_UI_MSGSEARCH_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"

// msgsearch.c: A search of message files for the strings that start with a prefix
typedef struct Ov139Search Ov139Search;

// A string found: its message file and its line
typedef struct {
    u32 file;
    u32 line;
} Ov139SearchResult;

Ov139Search *func_ov139_0219a438(MsgData **files, u32 count, HeapID heapId);
void func_ov139_0219a490(Ov139Search *search);
// Fills results with the strings of a file that start with prefix, up to max, and returns how many it found
u32 func_ov139_0219a4a4(Ov139Search *search, u32 file, u32 a2, StrBuf *prefix, Ov139SearchResult *results, u32 max);

#endif // POKEBW2_APP_UI_MSGSEARCH_H
