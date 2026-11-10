// Overlay 31, the last of its four files (see net_devirc.c): the SPI bus to the card's infrared chip, which it talks
// to through the backup SPI bus (as src/ov338/ir_check.c does). A transfer is a command byte, 1 to read and 2 to
// write, and then the bytes, with a pause of 60 microseconds between. This is the SDK's libirc irc_internal.c, from
// TwlSDK's add-ins; lib/irc is this project's directory name, and the functions and data take the SDK's names, which
// Black 1's decompilation (https://github.com/Project-Holon/aletheia) recovered from libirc.a's irc_internal.o.
#include "irc_spi.h"
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/os.h"

// The most bytes the chip answers with
#define IRC_RECV_MAX 0xb5

// The lock ID before one is acquired: OS_LOCK_ID_ERROR, in the u16 the lock ID is kept in
#define IRC_LOCK_ID_UNSET 0xfffd

// Where the bytes of a transfer come from and go to
typedef struct {
    // Set to 1 by IRCi_Init and never read
    u32 unk0;
    const u8 *send;
    u8 *recv;
    // Never read or written; it fills IrcSpi's 0x10-byte slot and keeps gIrcReadBuffer at its address
    u32 unkC;
} IrcSpiData;

static void IRCi_WaitCommand(void);
static u8 IRCi_CommWrite(IrcSpiData *param);
static void IRCi_CommRead(IrcSpiData *param);

static u16 gIrcLockId = IRC_LOCK_ID_UNSET;
// Aligned, as the original's IRCi_Read and IRCi_Write show: at -O4,p MWCC rotates a function's loops unless it
// reads an object with an explicit alignment, and these two, which read IrcSpi by name, keep theirs entered with a
// branch to the test. Which object was aligned, and to what, is a guess: gIrcLockId or both command buffers give the
// same code, and 4 to 16 all fit IrcSpi's address, right after irc_link.c's frame and on a 16-byte boundary but not
// a 32-byte one.
static IrcSpiData IrcSpi ATTRIBUTE_ALIGN(4);
static u8 gIrcReadBuffer[0xb8];
static u8 gIrcWriteBuffer[0xc8];

static void IRCi_WaitCommand(void) {
    u64 start = OS_GetTick();

    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 60) {
    }
}

void IRCi_Init(void) {
    IrcSpi.unk0 = 1;
    if (gIrcLockId == IRC_LOCK_ID_UNSET) {
        gIrcLockId = OS_GetLockID();
    }
}

void IRCi_End(void) {
    // The original's own test (aletheia's IRCi_End), always true: a u16 never equals -3
    if (gIrcLockId != OS_LOCK_ID_ERROR) {
        OS_ReleaseLockID(gIrcLockId);
        gIrcLockId = IRC_LOCK_ID_UNSET;
    }
}

static u8 IRCi_CommWrite(IrcSpiData *param) {
    u64 start;
    // Read from the data register into a volatile copy, which the original keeps on the stack
    vu16 data;

    start = OS_GetTick();
    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 50) {
    }
    reg_MI_MCD0 = *param->send;
    param->send++;
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    data = reg_MI_MCD0;
    return data;
}

static void IRCi_CommRead(IrcSpiData *param) {
    u64 start = OS_GetTick();

    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 50) {
    }
    reg_MI_MCD0 = 0;
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    *param->recv = reg_MI_MCD0;
    param->recv++;
}

u8 IRCi_Read(u8 *buf) {
    u8 size;
    int i;

    IrcSpi.send = gIrcReadBuffer;
    IrcSpi.recv = buf;
    gIrcReadBuffer[0] = 1;
    buf[0] = 0;
    CARD_LockBackup(gIrcLockId);
    OS_LockCard(gIrcLockId);
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | REG_MI_MCCNT0_MODE_MASK | 0x2;
    IRCi_CommWrite(&IrcSpi);
    IRCi_WaitCommand();
    IRCi_CommRead(&IrcSpi);
    IRCi_WaitCommand();
    size = buf[0];
    if (size > IRC_RECV_MAX) {
        size = 0;
    }
    IrcSpi.send = NULL;
    IrcSpi.recv = buf;
    buf[0] = 0;
    if (size != 0) {
        for (i = 0; i < size; i++) {
            if (i == size - 1) {
                reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | 0x2;
            }
            IRCi_CommRead(&IrcSpi);
        }
    } else {
        reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | 0x2;
        IRCi_CommRead(&IrcSpi);
    }
    for (i = 0; i < size; i++) {
        buf[i] ^= IRC_XOR;
    }
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    OS_UnLockCard(gIrcLockId);
    CARD_UnlockBackup(gIrcLockId);
    return size;
}

void IRCi_Write(const u8 *data, int size) {
    int i;

    IrcSpi.send = gIrcWriteBuffer;
    IrcSpi.recv = NULL;
    gIrcWriteBuffer[0] = 2;
    CARD_LockBackup(gIrcLockId);
    OS_LockCard(gIrcLockId);
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | REG_MI_MCCNT0_MODE_MASK | 0x2;
    IRCi_CommWrite(&IrcSpi);
    IRCi_WaitCommand();
    IrcSpi.send = data;
    for (i = 0; i < size; i++) {
        if (i == size - 1) {
            reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | 0x2;
        }
        IRCi_CommWrite(&IrcSpi);
    }
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    OS_UnLockCard(gIrcLockId);
    CARD_UnlockBackup(gIrcLockId);
}
