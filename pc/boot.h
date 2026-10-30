#ifndef POKEBW2_PC_BOOT_H
#define POKEBW2_PC_BOOT_H
#include <SDL3/SDL.h>
#include <stdbool.h>
typedef enum { PC_BOOT_MOVIES, PC_BOOT_TITLE, PC_BOOT_MENU, PC_BOOT_NEW_GAME } PcBootEntry;
bool PcBoot_Start(const char *root, PcBootEntry entry);
/* Run once per fixed game tick, never once per presentation. */
void PcBoot_Update(void);
void PcBoot_Draw(void);
bool PcBoot_HandleEvent(const SDL_Event *event);
void PcBoot_Stop(void);
#endif
