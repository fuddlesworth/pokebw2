#ifndef POKEBW2_GFL_NET_WHPIPE_H
#define POKEBW2_GFL_NET_WHPIPE_H

#include "types.h"

// net_whpipe.c, in overlay 30: the GFL net's wireless pipe, over wih.c: beacons, scanning and the connection

// The game service ID of a beacon the scan found
u8 func_ov030_02173b78(int index);

// The Funfest's beacon
void func_ov030_02174108(u32 enabled);
// Whether the Funfest's beacon updates are sent (func_ov030_02174e58 returns 2)
BOOL func_ov030_02173c08(void);
// Sends the game's beacon
void func_ov030_02173780(void);

// The beacons that deliveries come by
void *func_ov030_02173b24(int index);
void func_ov030_02173ba4(int index);
void func_ov030_02173bc4(void);
void func_ov030_021740d0(int index);
u8 func_ov030_021740a4(u8 index);
void func_ov030_02173bec(int index);

#endif // POKEBW2_GFL_NET_WHPIPE_H
