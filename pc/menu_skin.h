#ifndef POKEBW2_PC_MENU_SKIN_H
#define POKEBW2_PC_MENU_SKIN_H

#include "video.h"

typedef enum {
    PC_MENU_HEADER,
    PC_MENU_FOOTER,
    PC_MENU_BAG,
    PC_MENU_SUMMARY,
    PC_MENU_MAIL,
    PC_MENU_TRAINER,
    PC_MENU_SIGNATURE_PAD,
    PC_MENU_SAVE,
    PC_MENU_TAB,
    PC_MENU_ROW,
    PC_MENU_LIGHT_ROW,
    PC_MENU_BUTTON,
    PC_MENU_MARKING,
    PC_MENU_TRAINER_BADGE,
    PC_MENU_GREETING_ROW,
    PC_MENU_SAVE_PROGRESS,
    PC_BATTLE_PANEL,
    PC_BATTLE_ENEMY_HP,
    PC_BATTLE_PLAYER_HP,
    PC_BATTLE_MOVE,
    PC_BATTLE_ACTION,
    PC_BATTLE_ICON,
    PC_BATTLE_CURSOR,
    PC_BATTLE_NOTICE,
    PC_BATTLE_KEY,
    PC_MENU_SETTINGS,
    PC_MENU_SETTINGS_SELECTION,
    PC_MENU_SKIN_COUNT
} PcMenuSkinAsset;

typedef struct {
    PcMenuSkinAsset asset;
    float x, y, width, height;
    Uint32 state;
} PcMenuSkinLayer;

/* Measurements use the SVG's logical units, before its 2x rasterization. */
typedef struct {
    unsigned states;
    float height, leftCap, rightCap;
    float topLeft, topRight, bottomRight, bottomLeft;
} PcMenuSkinGeometry;
PcMenuSkinGeometry PcMenuSkin_GetGeometry(PcMenuSkinAsset asset);

void PcMenuSkin_Add(PcMenuSkinAsset asset, float x, float y, float width, float height, Uint32 state);
void PcMenuSkin_Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport);

#endif
