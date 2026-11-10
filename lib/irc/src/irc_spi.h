#ifndef POKEBW2_IRC_IRC_SPI_H
#define POKEBW2_IRC_IRC_SPI_H

#include "types.h"

// irc_spi.c, in overlay 31: the SDK's libirc irc_internal.c, the card's infrared chip over the backup SPI bus (see
// net_devirc.h); lib/irc is this project's name for it

// Every byte on the wire is xored with this
#define IRC_XOR 0xaa

void IRCi_Init(void);
void IRCi_End(void);
// Reads a frame into buf and gives its size, 0 if there is none
u8 IRCi_Read(u8 *buf);
void IRCi_Write(const u8 *data, int size);

#endif // POKEBW2_IRC_IRC_SPI_H
