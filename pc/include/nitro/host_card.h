#ifndef POKEBW2_HOST_CARD_H
#define POKEBW2_HOST_CARD_H
#include "types.h"
BOOL PcCard_OpenBackup(const char *path);
void PcCard_CloseBackup(void);
BOOL PcCard_ReadSettings(void *data, u32 size);
BOOL PcCard_WriteSettings(const void *data, u32 size);
#endif
