#ifndef POKEBW2_APP_PMS_PMS_INPUT_DATA_H
#define POKEBW2_APP_PMS_PMS_INPUT_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The words that the phrase input offers, of overlay 185's pms_input_data.c: which words of each category and of each
// initial the player has unlocked. The names are ours, guessed

// The categories of words
#define PMSI_CATEGORY_COUNT 12
// The initials the words are sorted by, A to Z and the others
#define PMSI_INITIAL_COUNT 27

// The marks in the word tables: a group of words shown as one, and the end of a table
#define PMS_WORD_DUP 0xfffe
#define PMS_WORD_END 0xffff

PMSInputData *PMSIData_Create(u32 heapId, const PMSIParam *param);
void PMSIData_Delete(PMSInputData *data);
u32 PMSIData_GetCategoryWordCount(const PMSInputData *data, u32 category);
void PMSIData_GetCategoryWord(const PMSInputData *data, u32 category, u32 index, StrBuf *buf);
u16 PMSIData_GetCategoryWordCode(const PMSInputData *data, u32 category, u32 index);
// Copies the unlocked words of a table to dst, and returns how many
u32 PMSIData_CountTableWords(const PMSInputData *data, const u16 *table, u16 *dst);
// The word at a position of an initial's table, or 0
u16 PMSIData_GetInitialWordCode(const PMSInputData *data, u32 initial, u32 index);
void PMSIData_GetWordStr(const PMSInputData *data, u16 word, StrBuf *buf);
u16 PMSIData_GetEndWord(const PMSInputData *data);

// Categories 2, 4, 8, 9 and 10, whose words are always unlocked. Nothing reads them
#define PMSI_FIXED_CATEGORY_COUNT 5
extern const u8 PMSI_FIXED_CATEGORIES[PMSI_FIXED_CATEGORY_COUNT];

// Reconstructed, not known from the ROM. MWCC puts the table above in the same section as the file's other data, as
// the original has it, only when some code reads it before its definition, even code it never emits. Nothing calls
// this accessor
static inline u32 PMSIData_GetFixedCategory(u32 index) {
    return PMSI_FIXED_CATEGORIES[index];
}

#endif // POKEBW2_APP_PMS_INPUT_DATA_H
