#include "menu_skin.h"

PcMenuSkinGeometry PcMenuSkin_GetGeometry(PcMenuSkinAsset asset) {
    switch (asset) {
    case PC_MENU_TAB:
        return (PcMenuSkinGeometry){ 2, 43, 4, 44, 0, 40, 0, 0 };
    case PC_MENU_ROW:
        return (PcMenuSkinGeometry){ 2, 58, 4, 24, 0, 16, 0, 0 };
    case PC_MENU_LIGHT_ROW:
        return (PcMenuSkinGeometry){ 2, 50, 4, 24, 0, 18, 0, 0 };
    case PC_MENU_BUTTON:
        return (PcMenuSkinGeometry){ 2, 36, 4, 24, 0, 16, 0, 0 };
    case PC_MENU_TRAINER_BADGE:
        return (PcMenuSkinGeometry){ 2, 84, 4, 24, 0, 16, 0, 0 };
    case PC_MENU_GREETING_ROW:
        return (PcMenuSkinGeometry){ 2, 112, 4, 24, 0, 20, 0, 0 };
    case PC_BATTLE_ACTION:
        return (PcMenuSkinGeometry){ 2, 72, 20, 24, 16, 0, 16, 0 };
    case PC_BATTLE_MOVE:
        return (PcMenuSkinGeometry){ 2, 80, 20, 24, 14, 14, 14, 14 };
    case PC_MENU_MARKING:
    case PC_MENU_SAVE_PROGRESS:
        return (PcMenuSkinGeometry){ .states = 2 };
    default:
        return (PcMenuSkinGeometry){ .states = 1 };
    }
}
