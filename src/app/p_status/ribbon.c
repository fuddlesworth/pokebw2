#include "app/p_status/p_status_local.h"
#include "constants/pokemon.h"

// The ribbons as the summary screen shows them: each one's Pokémon field, icon and palette, name and description, and
// the category that numbers it. The name and description are lines of the summary screen's ribbon text, in the
// ribbons' order. The file's name is a guess, after Gen 4's ribbon.c

typedef struct {
    u32 param;
    u16 icon;
    u16 palette;
    u16 name;
    u16 description;
    u8 category;
} RibbonData;

static const RibbonData sRibbons[RIBBON_COUNT] = {
    // Champion Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 20, 26, 0, 0, 80, RIBBON_CATEGORY_LEAGUE },
    // Sinnoh Champ Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH, 27, 0, 1, 81, RIBBON_CATEGORY_LEAGUE },
    // Cool Ribbon
    { PKM_PARAM_RIBBON_G3_COOL, 18, 0, 2, 82, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon Super
    { PKM_PARAM_RIBBON_G3_COOL + 1, 19, 0, 3, 83, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon Hyper
    { PKM_PARAM_RIBBON_G3_COOL + 2, 20, 0, 4, 84, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon Master
    { PKM_PARAM_RIBBON_G3_COOL + 3, 21, 0, 5, 85, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 4, 18, 1, 6, 86, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon Super
    { PKM_PARAM_RIBBON_G3_COOL + 5, 19, 1, 7, 87, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon Hyper
    { PKM_PARAM_RIBBON_G3_COOL + 6, 20, 1, 8, 88, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon Master
    { PKM_PARAM_RIBBON_G3_COOL + 7, 21, 1, 9, 89, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 8, 18, 2, 10, 90, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon Super
    { PKM_PARAM_RIBBON_G3_COOL + 9, 19, 2, 11, 91, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon Hyper
    { PKM_PARAM_RIBBON_G3_COOL + 10, 20, 2, 12, 92, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon Master
    { PKM_PARAM_RIBBON_G3_COOL + 11, 21, 2, 13, 93, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 12, 18, 3, 14, 94, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon Super
    { PKM_PARAM_RIBBON_G3_COOL + 13, 19, 3, 15, 95, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon Hyper
    { PKM_PARAM_RIBBON_G3_COOL + 14, 20, 3, 16, 96, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon Master
    { PKM_PARAM_RIBBON_G3_COOL + 15, 21, 3, 17, 97, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 16, 18, 4, 18, 98, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon Super
    { PKM_PARAM_RIBBON_G3_COOL + 17, 19, 4, 19, 99, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon Hyper
    { PKM_PARAM_RIBBON_G3_COOL + 18, 20, 4, 20, 100, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon Master
    { PKM_PARAM_RIBBON_G3_COOL + 19, 21, 4, 21, 101, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon
    { PKM_PARAM_RIBBON_G4_COOL, 22, 0, 22, 102, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon Great
    { PKM_PARAM_RIBBON_G4_COOL + 1, 23, 0, 23, 103, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon Ultra
    { PKM_PARAM_RIBBON_G4_COOL + 2, 24, 0, 24, 104, RIBBON_CATEGORY_CONTEST },
    // Cool Ribbon Master
    { PKM_PARAM_RIBBON_G4_COOL + 3, 25, 0, 25, 105, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon
    { PKM_PARAM_RIBBON_G4_COOL + 4, 22, 1, 26, 106, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon Great
    { PKM_PARAM_RIBBON_G4_COOL + 5, 23, 1, 27, 107, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon Ultra
    { PKM_PARAM_RIBBON_G4_COOL + 6, 24, 1, 28, 108, RIBBON_CATEGORY_CONTEST },
    // Beauty Ribbon Master
    { PKM_PARAM_RIBBON_G4_COOL + 7, 25, 1, 29, 109, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon
    { PKM_PARAM_RIBBON_G4_COOL + 8, 22, 2, 30, 110, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon Great
    { PKM_PARAM_RIBBON_G4_COOL + 9, 23, 2, 31, 111, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon Ultra
    { PKM_PARAM_RIBBON_G4_COOL + 10, 24, 2, 32, 112, RIBBON_CATEGORY_CONTEST },
    // Cute Ribbon Master
    { PKM_PARAM_RIBBON_G4_COOL + 11, 25, 2, 33, 113, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon
    { PKM_PARAM_RIBBON_G4_COOL + 12, 22, 3, 34, 114, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon Great
    { PKM_PARAM_RIBBON_G4_COOL + 13, 23, 3, 35, 115, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon Ultra
    { PKM_PARAM_RIBBON_G4_COOL + 14, 24, 3, 36, 116, RIBBON_CATEGORY_CONTEST },
    // Smart Ribbon Master
    { PKM_PARAM_RIBBON_G4_COOL + 15, 25, 3, 37, 117, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon
    { PKM_PARAM_RIBBON_G4_COOL + 16, 22, 4, 38, 118, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon Great
    { PKM_PARAM_RIBBON_G4_COOL + 17, 23, 4, 39, 119, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon Ultra
    { PKM_PARAM_RIBBON_G4_COOL + 18, 24, 4, 40, 120, RIBBON_CATEGORY_CONTEST },
    // Tough Ribbon Master
    { PKM_PARAM_RIBBON_G4_COOL + 19, 25, 4, 41, 121, RIBBON_CATEGORY_CONTEST },
    // Winning Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 21, 56, 0, 42, 122, RIBBON_CATEGORY_TOWER },
    // Victory Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 22, 57, 0, 43, 123, RIBBON_CATEGORY_TOWER },
    // Ability Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 1, 58, 0, 44, 124, RIBBON_CATEGORY_TOWER },
    // Great Ability Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 2, 59, 0, 45, 125, RIBBON_CATEGORY_TOWER },
    // Double Ability Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 3, 60, 0, 46, 126, RIBBON_CATEGORY_TOWER },
    // Multi Ability Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 4, 61, 0, 47, 127, RIBBON_CATEGORY_TOWER },
    // Pair Ability Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 5, 62, 0, 48, 128, RIBBON_CATEGORY_TOWER },
    // World Ability Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 6, 63, 0, 49, 129, RIBBON_CATEGORY_TOWER },
    // Artist Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 23, 28, 1, 50, 130, RIBBON_CATEGORY_MEMORIAL },
    // Effort Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 24, 29, 2, 51, 131, RIBBON_CATEGORY_MEMORIAL },
    // Alert Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 7, 30, 2, 52, 132, RIBBON_CATEGORY_MEMORIAL },
    // Shock Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 8, 31, 0, 53, 133, RIBBON_CATEGORY_MEMORIAL },
    // Downcast Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 9, 32, 1, 54, 134, RIBBON_CATEGORY_MEMORIAL },
    // Careless Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 10, 33, 2, 55, 135, RIBBON_CATEGORY_MEMORIAL },
    // Relax Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 11, 34, 3, 56, 136, RIBBON_CATEGORY_MEMORIAL },
    // Snooze Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 12, 35, 0, 57, 137, RIBBON_CATEGORY_MEMORIAL },
    // Smile Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 13, 36, 2, 58, 138, RIBBON_CATEGORY_MEMORIAL },
    // Gorgeous Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 14, 37, 1, 59, 139, RIBBON_CATEGORY_MEMORIAL },
    // Royal Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 15, 38, 3, 60, 140, RIBBON_CATEGORY_MEMORIAL },
    // Gorgeous Royal Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 16, 39, 0, 61, 141, RIBBON_CATEGORY_MEMORIAL },
    // Footprint Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 17, 40, 0, 62, 142, RIBBON_CATEGORY_MEMORIAL },
    // Record Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 18, 41, 1, 63, 143, RIBBON_CATEGORY_MEMORIAL },
    // Legend Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 20, 42, 0, 64, 144, RIBBON_CATEGORY_MEMORIAL },
    // Country Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 28, 43, 3, 65, 145, RIBBON_CATEGORY_GIFT },
    // National Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 29, 43, 4, 66, 146, RIBBON_CATEGORY_GIFT },
    // Earth Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 30, 44, 0, 67, 147, RIBBON_CATEGORY_GIFT },
    // World Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 31, 44, 1, 68, 148, RIBBON_CATEGORY_GIFT },
    // Classic Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 26, 45, 1, 69, 149, RIBBON_CATEGORY_GIFT },
    // Premier Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 27, 46, 0, 70, 150, RIBBON_CATEGORY_GIFT },
    // Event Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 19, 47, 3, 71, 151, RIBBON_CATEGORY_GIFT },
    // Birthday Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 22, 48, 2, 72, 152, RIBBON_CATEGORY_GIFT },
    // Special Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 23, 49, 2, 73, 153, RIBBON_CATEGORY_GIFT },
    // Souvenir Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 24, 50, 1, 74, 154, RIBBON_CATEGORY_GIFT },
    // Wishing Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 25, 51, 0, 75, 155, RIBBON_CATEGORY_GIFT },
    // Battle Champion Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 25, 52, 3, 76, 156, RIBBON_CATEGORY_GIFT },
    // Regional Champion Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 26, 53, 3, 77, 157, RIBBON_CATEGORY_GIFT },
    // National Champion Ribbon
    { PKM_PARAM_RIBBON_G3_COOL + 27, 54, 1, 78, 158, RIBBON_CATEGORY_GIFT },
    // World Champion Ribbon
    { PKM_PARAM_RIBBON_CHAMPION_SINNOH + 21, 55, 1, 79, 159, RIBBON_CATEGORY_GIFT },
};

u32 Ribbon_GetData(u32 ribbon, u32 field) {
    switch (field) {
    case RIBBON_DATA_PARAM:
        return sRibbons[ribbon].param;
    case RIBBON_DATA_ICON:
        return sRibbons[ribbon].icon;
    case RIBBON_DATA_PALETTE:
        return sRibbons[ribbon].palette;
    case RIBBON_DATA_NAME:
        return sRibbons[ribbon].name;
    case RIBBON_DATA_DESCRIPTION:
        return sRibbons[ribbon].description;
    case RIBBON_DATA_CATEGORY:
        return sRibbons[ribbon].category;
    }
    return 0;
}

u32 Ribbon_GetDescription(u32 ribbon) {
    return sRibbons[ribbon].description;
}
