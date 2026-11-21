#include "types.h"
#include "field/player/player_action.h"

// What the player may do where they stand: whether each field action is blocked. The file's name is a guess after
// swan's names for its functions: the ROM has no string for it. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

void PlayerActionPerms_SetActionBlocked(PlayerActionPerms *perms, u32 action, s32 blocked) {
    perms->blocked[action] = blocked;
}

u8 PlayerActionPerms_IsActionBlocked(PlayerActionPerms *perms, u32 action) {
    return perms->blocked[action];
}
