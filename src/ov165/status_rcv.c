#include "app/pokelist.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/heap.h"
#include "pml/item.h"
#include "pml/move.h"
#include "pml/personal.h"
#include "pml/poke_party.h"

// What an item from the bag does to a Pokémon: whether it can be used, and using it. The ROM doesn't name this file;
// the name is a guess, after the file that does this in HeartGold and SoulSilver

// Special amounts stored in an item's HP restore field
enum {
    HP_RESTORE_QUARTER = 253,
    HP_RESTORE_HALF = 254,
    HP_RESTORE_ALL = 255,
};

static BOOL StatusRcv_PPUp(PartyPkm *pkm, u32 slot, u32 count);
static void StatusRcv_RestoreHp(PartyPkm *pkm, u32 hp, u32 maxHp, u32 amount);
static s32 StatusRcv_AddEffort(s32 effort, s32 total, s32 add, s32 noLimit);
static BOOL StatusRcv_CanChangeFriendship(PartyPkm *pkm, ItemData *data);
static BOOL StatusRcv_ChangeFriendship(PartyPkm *pkm, s32 friendship, s32 add, u16 item, u16 zoneId, u32 heapId);

BOOL StatusRcv_CanUseItem(PartyPkm *pkm, u16 item, u16 pos, u32 heapId) {
    void *data;
    u32 status;
    u32 hp;
    int i;
    s32 atkEffort;
    s32 defEffort;
    s32 speedEffort;
    s32 spAtkEffort;
    s32 spDefEffort;
    s32 hpEffort;
    s32 noLimit;
    s32 effort;

    data = PML_ItemReadDataFile(item, ITEM_FILE_DATA, heapId);
    if (PML_ItemGetParam(data, ITEM_PARAM_WORK_TYPE) != ITEM_WORK_PARAMS) {
        GFL_HeapFree(data);
        return FALSE;
    }

    status = GetStatusCond(pkm);
    if (PML_ItemGetParam(data, ITEM_PARAM_SLEEP_HEAL) != 0 && status == CONDITION_SLEEP) {
        GFL_HeapFree(data);
        return TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_POISON_HEAL) != 0 && status == CONDITION_POISON) {
        GFL_HeapFree(data);
        return TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_BURN_HEAL) != 0 && status == CONDITION_BURN) {
        GFL_HeapFree(data);
        return TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_FREEZE_HEAL) != 0 && status == CONDITION_FREEZE) {
        GFL_HeapFree(data);
        return TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_PARALYSIS_HEAL) != 0 && status == CONDITION_PARALYSIS) {
        GFL_HeapFree(data);
        return TRUE;
    }

    hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
    if ((PML_ItemGetParam(data, ITEM_PARAM_REVIVE) != 0 || PML_ItemGetParam(data, ITEM_PARAM_REVIVE_ALL) != 0) &&
        PML_ItemGetParam(data, ITEM_PARAM_LEVEL_UP) == 0) {
        if (hp == 0) {
            GFL_HeapFree(data);
            return TRUE;
        }
    } else if (PML_ItemGetParam(data, ITEM_PARAM_HP_RESTORE) != 0 && hp != 0 &&
               hp < PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL)) {
        GFL_HeapFree(data);
        return TRUE;
    }

    if (PML_ItemGetParam(data, ITEM_PARAM_LEVEL_UP) != 0 && PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL) < 100) {
        GFL_HeapFree(data);
        return TRUE;
    }

    if ((PML_ItemGetParam(data, ITEM_PARAM_PP_UP) != 0 || PML_ItemGetParam(data, ITEM_PARAM_PP_MAX) != 0) &&
        PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + pos, NULL) < 3 &&
        PML_MoveGetMaxPP(PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + pos, NULL), 0) >= 5) {
        GFL_HeapFree(data);
        return TRUE;
    }

    if (PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE) != 0 && PokeParty_CheckPPNeedsReplenish(pkm, pos) == TRUE) {
        GFL_HeapFree(data);
        return TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE_ALL) != 0) {
        for (i = 0; i < 4; i++) {
            if (PokeParty_CheckPPNeedsReplenish(pkm, i) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }

    hpEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP, NULL);
    atkEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 1, NULL);
    defEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 2, NULL);
    speedEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 3, NULL);
    spAtkEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 4, NULL);
    spDefEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 5, NULL);
    noLimit = PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_UP_1);

    if (PML_ItemGetParam(data, ITEM_PARAM_HP_EV_UP) != 0) {
        effort = PML_ItemGetParam(data, ITEM_PARAM_HP_EV);
        if (effort > 0 && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_SHEDINJA) {
            if ((hpEffort < 100 || noLimit != 0) && hpEffort < 255 &&
                hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort < 510) {
                GFL_HeapFree(data);
                return TRUE;
            }
        } else if (effort < 0) {
            if (hpEffort > 0) {
                GFL_HeapFree(data);
                return TRUE;
            }
            if (StatusRcv_CanChangeFriendship(pkm, data) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_ATTACK_EV_UP) != 0) {
        effort = PML_ItemGetParam(data, ITEM_PARAM_ATTACK_EV);
        if (effort > 0) {
            if ((atkEffort < 100 || noLimit != 0) && atkEffort < 255 &&
                hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort < 510) {
                GFL_HeapFree(data);
                return TRUE;
            }
        } else if (effort < 0) {
            if (atkEffort > 0) {
                GFL_HeapFree(data);
                return TRUE;
            }
            if (StatusRcv_CanChangeFriendship(pkm, data) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_DEFENSE_EV_UP) != 0) {
        effort = PML_ItemGetParam(data, ITEM_PARAM_DEFENSE_EV);
        if (effort > 0) {
            if ((defEffort < 100 || noLimit != 0) && defEffort < 255 &&
                hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort < 510) {
                GFL_HeapFree(data);
                return TRUE;
            }
        } else if (effort < 0) {
            if (defEffort > 0) {
                GFL_HeapFree(data);
                return TRUE;
            }
            if (StatusRcv_CanChangeFriendship(pkm, data) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_SPEED_EV_UP) != 0) {
        effort = PML_ItemGetParam(data, ITEM_PARAM_SPEED_EV);
        if (effort > 0) {
            if ((speedEffort < 100 || noLimit != 0) && speedEffort < 255 &&
                hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort < 510) {
                GFL_HeapFree(data);
                return TRUE;
            }
        } else if (effort < 0) {
            if (speedEffort > 0) {
                GFL_HeapFree(data);
                return TRUE;
            }
            if (StatusRcv_CanChangeFriendship(pkm, data) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_SP_ATTACK_EV_UP) != 0) {
        effort = PML_ItemGetParam(data, ITEM_PARAM_SP_ATTACK_EV);
        if (effort > 0) {
            if ((spAtkEffort < 100 || noLimit != 0) && spAtkEffort < 255 &&
                hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort < 510) {
                GFL_HeapFree(data);
                return TRUE;
            }
        } else if (effort < 0) {
            if (spAtkEffort > 0) {
                GFL_HeapFree(data);
                return TRUE;
            }
            if (StatusRcv_CanChangeFriendship(pkm, data) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_SP_DEFENSE_EV_UP) != 0) {
        effort = PML_ItemGetParam(data, ITEM_PARAM_SP_DEFENSE_EV);
        if (effort > 0) {
            if ((spDefEffort < 100 || noLimit != 0) && spDefEffort < 255 &&
                hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort < 510) {
                GFL_HeapFree(data);
                return TRUE;
            }
        } else if (effort < 0) {
            if (spDefEffort > 0) {
                GFL_HeapFree(data);
                return TRUE;
            }
            if (StatusRcv_CanChangeFriendship(pkm, data) == TRUE) {
                GFL_HeapFree(data);
                return TRUE;
            }
        }
    }

    GFL_HeapFree(data);
    return FALSE;
}

BOOL StatusRcv_UseItem(PartyPkm *pkm, u16 item, u16 pos, u16 zoneId, u32 heapId) {
    void *data;
    BOOL changed = FALSE;
    BOOL hasEffect;
    u32 status;
    u32 newStatus;
    s32 spAtkEffort;
    s32 spDefEffort;
    s32 noLimit;
    s32 add;
    s32 hpEffort;
    s32 hp;
    s32 atkEffort;
    s32 maxHp;
    s32 defEffort;
    s32 speedEffort;
    s32 level;
    int i;
    s32 friendship;
    s32 effort;

    data = PML_ItemReadDataFile(item, ITEM_FILE_DATA, heapId);
    if (PML_ItemGetParam(data, ITEM_PARAM_WORK_TYPE) != ITEM_WORK_PARAMS) {
        GFL_HeapFree(data);
        return changed;
    }

    hasEffect = FALSE;
    status = PokeParty_GetParam(pkm, PKM_PARAM_STATUS, NULL);
    newStatus = status;
    if (PML_ItemGetParam(data, ITEM_PARAM_SLEEP_HEAL) != 0) {
        newStatus = 0;
        hasEffect = TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_POISON_HEAL) != 0) {
        newStatus = 0;
        hasEffect = TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_BURN_HEAL) != 0) {
        newStatus = 0;
        hasEffect = TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_FREEZE_HEAL) != 0) {
        newStatus = 0;
        hasEffect = TRUE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_PARALYSIS_HEAL) != 0) {
        newStatus = 0;
        hasEffect = TRUE;
    }
    if (status != newStatus) {
        PokeParty_SetParam(pkm, PKM_PARAM_STATUS, newStatus);
        changed = TRUE;
    }

    hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
    maxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
    if ((PML_ItemGetParam(data, ITEM_PARAM_REVIVE) != 0 || PML_ItemGetParam(data, ITEM_PARAM_REVIVE_ALL) != 0) &&
        PML_ItemGetParam(data, ITEM_PARAM_LEVEL_UP) == 0) {
        if (hp == 0) {
            StatusRcv_RestoreHp(pkm, hp, maxHp, PML_ItemGetParam(data, ITEM_PARAM_HP_RESTORE_AMOUNT));
            changed = TRUE;
        }
        hasEffect = TRUE;
    } else if (PML_ItemGetParam(data, ITEM_PARAM_HP_RESTORE) != 0) {
        if (hp < maxHp) {
            StatusRcv_RestoreHp(pkm, hp, maxHp, PML_ItemGetParam(data, ITEM_PARAM_HP_RESTORE_AMOUNT));
            changed = TRUE;
        }
        hasEffect = TRUE;
    }

    level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    if (PML_ItemGetParam(data, ITEM_PARAM_LEVEL_UP) != 0) {
        if (level < 100) {
            PokeParty_SetParam(pkm, PKM_PARAM_EXP,
                               PML_UtilGetPkmLvExp(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                                   PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), level + 1));
            PokeParty_RecalcStats(pkm);
            // A fainted Pokémon gets back the HP its maximum rose by
            if (hp == 0) {
                u32 newMaxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);

                StatusRcv_RestoreHp(pkm, hp, newMaxHp, newMaxHp - maxHp);
            }
            changed = TRUE;
        }
        hasEffect = TRUE;
    }

    if (PML_ItemGetParam(data, ITEM_PARAM_EVOLVE) != 0) {
        hasEffect = TRUE;
    }

    if (PML_ItemGetParam(data, ITEM_PARAM_PP_UP) != 0) {
        if (StatusRcv_PPUp(pkm, pos, 1) == TRUE) {
            changed = TRUE;
        }
        hasEffect = TRUE;
    } else if (PML_ItemGetParam(data, ITEM_PARAM_PP_MAX) != 0) {
        if (StatusRcv_PPUp(pkm, pos, 3) == TRUE) {
            changed = TRUE;
        }
        hasEffect = TRUE;
    }

    if (PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE) != 0) {
        if (PokeParty_AddPP(pkm, pos, PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE_AMOUNT)) == TRUE) {
            changed = TRUE;
        }
        hasEffect = TRUE;
    } else if (PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE_ALL) != 0) {
        for (i = 0; i < 4; i++) {
            if (PokeParty_AddPP(pkm, i, PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE_AMOUNT)) == TRUE) {
                changed = TRUE;
            }
        }
        hasEffect = TRUE;
    }

    hpEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP, NULL);
    atkEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 1, NULL);
    defEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 2, NULL);
    speedEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 3, NULL);
    spAtkEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 4, NULL);
    spDefEffort = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + 5, NULL);
    noLimit = PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_UP_1);

    if (PML_ItemGetParam(data, ITEM_PARAM_HP_EV_UP) != 0) {
        add = PML_ItemGetParam(data, ITEM_PARAM_HP_EV);
        if (add > 0 && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_SHEDINJA) {
            GFL_HeapFree(data);
            return FALSE;
        }
        effort = StatusRcv_AddEffort(hpEffort, atkEffort + defEffort + speedEffort + spAtkEffort + spDefEffort, add,
                                     noLimit);
        if (effort != -1) {
            hpEffort = effort;
            PokeParty_SetParam(pkm, PKM_PARAM_EV_HP, hpEffort);
            PokeParty_RecalcStats(pkm);
            changed = TRUE;
        }
        if (add > 0) {
            hasEffect = TRUE;
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_ATTACK_EV_UP) != 0) {
        add = PML_ItemGetParam(data, ITEM_PARAM_ATTACK_EV);
        effort = StatusRcv_AddEffort(atkEffort, hpEffort + defEffort + speedEffort + spAtkEffort + spDefEffort, add,
                                     noLimit);

        if (effort != -1) {
            atkEffort = effort;
            PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 1, atkEffort);
            PokeParty_RecalcStats(pkm);
            changed = TRUE;
        }
        if (add > 0) {
            hasEffect = TRUE;
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_DEFENSE_EV_UP) != 0) {
        add = PML_ItemGetParam(data, ITEM_PARAM_DEFENSE_EV);
        effort = StatusRcv_AddEffort(defEffort, hpEffort + atkEffort + speedEffort + spAtkEffort + spDefEffort, add,
                                     noLimit);

        if (effort != -1) {
            defEffort = effort;
            PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 2, defEffort);
            PokeParty_RecalcStats(pkm);
            changed = TRUE;
        }
        if (add > 0) {
            hasEffect = TRUE;
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_SPEED_EV_UP) != 0) {
        add = PML_ItemGetParam(data, ITEM_PARAM_SPEED_EV);
        effort = StatusRcv_AddEffort(speedEffort, hpEffort + atkEffort + defEffort + spAtkEffort + spDefEffort, add,
                                     noLimit);

        if (effort != -1) {
            speedEffort = effort;
            PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 3, speedEffort);
            PokeParty_RecalcStats(pkm);
            changed = TRUE;
        }
        if (add > 0) {
            hasEffect = TRUE;
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_SP_ATTACK_EV_UP) != 0) {
        add = PML_ItemGetParam(data, ITEM_PARAM_SP_ATTACK_EV);
        effort = StatusRcv_AddEffort(spAtkEffort, hpEffort + atkEffort + defEffort + speedEffort + spDefEffort, add,
                                     noLimit);

        if (effort != -1) {
            spAtkEffort = effort;
            PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 4, spAtkEffort);
            PokeParty_RecalcStats(pkm);
            changed = TRUE;
        }
        if (add > 0) {
            hasEffect = TRUE;
        }
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_SP_DEFENSE_EV_UP) != 0) {
        add = PML_ItemGetParam(data, ITEM_PARAM_SP_DEFENSE_EV);
        effort = StatusRcv_AddEffort(spDefEffort, hpEffort + atkEffort + defEffort + speedEffort + spAtkEffort, add,
                                     noLimit);

        if (effort != -1) {
            spDefEffort = effort;
            PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + 5, spDefEffort);
            PokeParty_RecalcStats(pkm);
            changed = TRUE;
        }
        if (add > 0) {
            hasEffect = TRUE;
        }
    }

    if (changed == FALSE && hasEffect == TRUE) {
        GFL_HeapFree(data);
        return FALSE;
    }

    friendship = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
    if (friendship < 100) {
        if (PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_UP_2) != 0) {
            StatusRcv_ChangeFriendship(pkm, friendship, PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_1), item, zoneId,
                                       heapId);
            GFL_HeapFree(data);
            return changed;
        }
    } else if (friendship >= 100 && friendship < 200) {
        if (PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_UP_3) != 0) {
            StatusRcv_ChangeFriendship(pkm, friendship, PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_2), item, zoneId,
                                       heapId);
            GFL_HeapFree(data);
            return changed;
        }
    } else if (friendship >= 200 && friendship <= 255) {
        if (PML_ItemGetParam(data, ITEM_PARAM_UNK_51) != 0) {
            StatusRcv_ChangeFriendship(pkm, friendship, PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_3), item, zoneId,
                                       heapId);
            GFL_HeapFree(data);
            return changed;
        }
    }

    GFL_HeapFree(data);
    return changed;
}

// Raises a move's PP Ups by count, up to 3, and its PP by as much as its maximum rose
static BOOL StatusRcv_PPUp(PartyPkm *pkm, u32 slot, u32 count) {
    u8 ppUp;
    u8 pp;
    u16 move;
    u32 maxPP;

    ppUp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slot, NULL);
    if (ppUp == 3) {
        return FALSE;
    }
    move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL);
    if (PML_MoveGetMaxPP(move, 0) < 5) {
        return FALSE;
    }
    pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + slot, NULL);
    maxPP = PML_MoveGetMaxPP(move, ppUp);
    if (ppUp + count > 3) {
        ppUp = 3;
    } else {
        ppUp = ppUp + count;
    }
    pp = pp + PML_MoveGetMaxPP(move, ppUp) - maxPP;
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slot, ppUp);
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + slot, pp);
    return TRUE;
}

// Restores amount HP, or all of it for 255, half for 254 and a quarter for 253
static void StatusRcv_RestoreHp(PartyPkm *pkm, u32 hp, u32 maxHp, u32 amount) {
    if (maxHp == 1) {
        amount = 1;
    } else if (amount == HP_RESTORE_ALL) {
        amount = maxHp;
    } else if (amount == HP_RESTORE_HALF) {
        amount = maxHp / 2;
    } else if (amount == HP_RESTORE_QUARTER) {
        amount = maxHp / 4;
    }
    hp += amount;
    if (hp > maxHp) {
        hp = maxHp;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_HP, hp);
}

// The effort value after adding add, within 0 to 255, 100 unless noLimit is set, and 510 for all of them with the
// others' total, or -1 if it can't change
static s32 StatusRcv_AddEffort(s32 effort, s32 total, s32 add, s32 noLimit) {
    if (effort == 0 && add < 0) {
        return -1;
    }
    if (effort == 255 && add > 0) {
        return -1;
    }
    if (effort >= 100 && noLimit == 0 && add > 0) {
        return -1;
    }
    if (effort + total >= 510 && add > 0) {
        return -1;
    }
    effort += add;
    if (add > 0 && effort > 100 && noLimit == 0) {
        effort = 100;
    } else if (effort < 0) {
        effort = 0;
    }
    if (effort + total > 510) {
        effort = 510 - total;
    }
    return effort;
}

// Whether the item raises the Pokémon's friendship, by the amount for its range
static BOOL StatusRcv_CanChangeFriendship(PartyPkm *pkm, ItemData *data) {
    s32 friendship = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);

    if (friendship >= 255) {
        return FALSE;
    }
    if (friendship < 100) {
        if (PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_UP_2) != 0 &&
            PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_1) > 0) {
            return TRUE;
        }
        return FALSE;
    }
    if (friendship >= 100 && friendship < 200) {
        if (PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_UP_3) != 0 &&
            PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_2) > 0) {
            return TRUE;
        }
        return FALSE;
    }
    if (friendship >= 200 && friendship < 255) {
        if (PML_ItemGetParam(data, ITEM_PARAM_UNK_51) != 0 && PML_ItemGetParam(data, ITEM_PARAM_FRIENDSHIP_3) > 0) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

static BOOL StatusRcv_ChangeFriendship(PartyPkm *pkm, s32 friendship, s32 add, u16 item, u16 zoneId, u32 heapId) {
    if (friendship == 255 && add > 0) {
        return FALSE;
    }
    if (friendship == 0 && add < 0) {
        return FALSE;
    }
    FriendshipManagerCalcItem(pkm, item, zoneId, heapId);
    return TRUE;
}
