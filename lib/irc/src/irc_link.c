// Overlay 31, the third of its four files (see net_devirc.c): the link of frames between the two machines. One pings,
// the other answers, and then they send each other frames that carry a checksum, over the SPI bus of irc_spi.c.
// This is the SDK's libirc irc.c, from TwlSDK's add-ins; lib/irc is this project's directory name. The functions and
// the state enum take the SDK's names, which Black 1's decompilation (https://github.com/Project-Holon/aletheia)
// recovered from libirc.a's irc.o; the structs below are this project's reconstruction of the SDK's separate statics.
#include "irc/irc_link.h"
#include "irc_spi.h"
#include "nitro/os.h"

// How long a ping may go unanswered, in milliseconds
#define IRC_PING_TIMEOUT 100

// Where the link is on the way to a connection: the machine that pings is the parent and the one that answers the child
enum {
    IRC_ST_DISCONNECT,
    IRC_ST_WAIT_RESPONSE,
    IRC_ST_WAIT_ACK,
    IRC_ST_SENDER,
    IRC_ST_RECEIVER,
};

// The state of the link of frames, one struct over what the SDK keeps as separate statics. The DWARF's
// sStatus.status is the flat state here and sStatus.flg_connect is flgConnect.
typedef struct {
    // The SDK's retry_count: cleared when the state resets and when a frame's checksum is good
    u8 retryCount;
    // The SDK's cnt_retry: incremented by IRCi_Restore
    u32 cntRetry;
    // Called with each frame of data, with the number of bytes, its command and its ID
    void (*recvCallback)(u8 *data, u8 size, u8 command, u8 id);
    // Called when the link ends, with whether it timed out
    void (*closeCallback)(BOOL isError);
    // Never set. The call keeps the peer's ID in r0 and a zero in r1, so it takes both
    void (*connectCallback)(u8 unitId, BOOL isSender);
    // The SDK's flg_restore: set by IRCi_Restore, cleared by a reset or a connection
    BOOL flgRestore;
    // Whether this machine is the sender: it pinged and got the reply
    BOOL isSender;
    // The SDK's isTry: whether a connection attempt is underway
    BOOL isTry;
    // The SDK's sStatus.status
    int state;
    // The SDK's sStatus.flg_connect: whether the link is connected
    BOOL flgConnect;
    // When the last ping or frame was, in ticks
    u64 timeout;
} IrcLinkData;

// The head of a frame: the command, the ID of the link and the checksum
typedef struct {
    u8 command;
    u8 id;
    u8 sumLo;
    u8 sumHi;
} IRCHeader;

// A frame as it goes over the infrared: its head and the data
typedef struct {
    IRCHeader header;
    u8 data[0xb4];
} IrcFrameData;

// The IDs the two machines of the link use for its frames
typedef struct {
    u8 local;
    u8 peer;
} IrcLinkIdData;

static u16 IRCi_CalcSum(const u8 *data, u32 size);
static void IRCi_Restore(void);
static void IRCi_ResetState(void);
static void IRCi_Send(const u8 *data, u8 size, u8 command, u8 id);
static void IRCi_Finish(int n);
static void IRCi_Nop(void);
static BOOL IRCi_CheckLength(u8 size);
static BOOL IRCi_CheckTimeout(void);
static BOOL IRCi_CheckPacket(u8 size);
static void IRC_SetUnitNumber(u8 unitId);

static IrcLinkIdData IrcLinkIds = { 0xff, 0xff };
static IrcLinkData IrcLink;
// Aligned, as the original's IRCi_Send, IRCi_CheckLength and IRCi_CheckPacket show: at -O4,p MWCC
// rotates a function's loops and drops its empty ones unless it reads an object with an explicit alignment, and the
// frame is the only object IRCi_Send reads. To what it was aligned is a guess: 4 and 8 both fit its address,
// right after IrcLink and on an 8-byte boundary but not a 16-byte one. The original's symbols also give the data at
// IrcFrame.data its own object (aletheia's gIrcBufferPayload, named IrcFramePayload here).
static IrcFrameData IrcFrame ATTRIBUTE_ALIGN(4);

static u16 IRCi_CalcSum(const u8 *data, u32 size) {
    u32 sum = 0x80;
    u32 i;

    for (i = 0; i < size; i++) {
        u8 byte = *data++;

        if (i & 1) {
            sum += byte;
        } else {
            sum += byte << 8;
        }
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return sum;
}

static void IRCi_Restore(void) {
    IrcLink.flgRestore = TRUE;
    IrcLink.cntRetry++;
}

static void IRCi_ResetState(void) {
    IrcLink.retryCount = 0;
    IrcLink.state = IRC_ST_DISCONNECT;
    IrcLink.flgConnect = FALSE;
    IrcLink.flgRestore = FALSE;
    IrcLink.isTry = FALSE;
    IrcLink.isSender = FALSE;
    IrcLink.cntRetry = 0;
}

void IRC_Init(u8 unitId) {
    IRCi_Init();
    IRCi_Read((u8 *)&IrcFrame);
    IRCi_ResetState();
    IRC_SetUnitNumber(unitId);
    IrcLinkIds.peer = 0xff;
    IrcLink.connectCallback = NULL;
    IrcLink.recvCallback = NULL;
    IrcLink.closeCallback = NULL;
}

void IRC_Connect(void) {
    if (!IrcLink.flgConnect && !IrcLink.isTry) {
        u8 ping;

        IrcLink.isTry = TRUE;
        ping = IRC_CMD_PING ^ IRC_XOR;
        IRCi_Write(&ping, 1);
        IrcLink.state = IRC_ST_WAIT_RESPONSE;
        IrcLink.timeout = OS_GetTick();
    }
}

BOOL IRC_IsSender(void) {
    return IrcLink.isSender;
}

BOOL IRC_IsConnect(void) {
    return IrcLink.flgConnect;
}

static void IRCi_Send(const u8 *data, u8 size, u8 command, u8 id) {
    IrcFrameData *frame = &IrcFrame;
    u8 *dst;
    u8 *raw;
    int i;
    u16 sum;

    frame->header.id = id;
    frame->header.command = command;
    frame->header.sumLo = 0;
    frame->header.sumHi = 0;
    dst = IrcFrame.data;
    for (i = 0; i < size; i++) {
        *dst++ = *data++;
    }
    size += sizeof(IRCHeader);
    raw = (u8 *)&IrcFrame;
    sum = IRCi_CalcSum(raw, size);
    frame->header.sumLo = sum;
    frame->header.sumHi = sum >> 8;
    for (i = 0; i < size; i++) {
        raw[i] ^= IRC_XOR;
    }
    // The original loads IrcFrame's address again for the call rather than passing raw
    IRCi_Write((u8 *)&IrcFrame, size);
}

static void IRCi_Finish(int n) {
    if (IrcLink.flgConnect && !IrcLink.isSender && n == 0) {
        IRCi_Send(NULL, 0, IRC_CMD_CLOSE, IrcLinkIds.local);
    }
    if (IrcLink.closeCallback) {
        IrcLink.closeCallback(n);
    }
    IRCi_ResetState();
}

void IRC_Shutdown(void) {
    IRCi_Finish(0);
    IRCi_End();
}

void IRC_Send(const u8 *data, u8 size, u8 command) {
    if (IrcLink.flgConnect) {
        IRCi_Send(data, size, command, IrcLinkIds.local);
    }
}

static void IRCi_Nop(void) {
}

static BOOL IRCi_CheckLength(u8 size) {
    u32 i;

    if (size == 1) {
        if (IrcLink.flgConnect) {
            return TRUE;
        }
        if (IrcFrame.header.command != IRC_CMD_PING) {
            return TRUE;
        }
        return FALSE;
    }
    if (size == 2 || size == 3) {
        // A frame too short to hold a header: the empty loops are probably debug output compiled out
        for (i = 0; i < 4; i++) {
        }
        for (i = 0; i < size; i++) {
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL IRCi_CheckTimeout(void) {
    if (!IrcLink.flgConnect && !IrcLink.isTry) {
        return FALSE;
    }
    if (OS_TicksToMilliSeconds(OS_GetTick() - IrcLink.timeout) < IRC_PING_TIMEOUT) {
        return FALSE;
    }
    IRCi_Finish(1);
    IrcLink.isTry = FALSE;
    return TRUE;
}

static BOOL IRCi_CheckPacket(u8 size) {
    IrcFrameData *frame = &IrcFrame;
    u8 *raw = (u8 *)&IrcFrame;
    u32 i;
    u16 stored;
    u16 sum;

    if (size == 1) {
        if (frame->header.command == IRC_CMD_PING) {
            return TRUE;
        }
        return FALSE;
    }
    if (size < sizeof(IRCHeader)) {
        return FALSE;
    }
    stored = frame->header.sumLo + (frame->header.sumHi << 8);
    frame->header.sumLo = 0;
    frame->header.sumHi = 0;
    sum = IRCi_CalcSum(raw, size);
    frame->header.sumLo = stored;
    frame->header.sumHi = stored >> 8;
    if (stored == sum) {
        IrcLink.retryCount = 0;
        return TRUE;
    }
    if (IrcLink.flgConnect) {
        for (i = 0; i < size; i++) {
        }
        IRCi_Restore();
    }
    return FALSE;
}

BOOL IRC_Move(void) {
    IrcFrameData *frame;
    u8 size = IRCi_Read((u8 *)&IrcFrame);

    if (IRCi_CheckLength(size)) {
        size = 0;
    }
    if (size == 0) {
        if (IRCi_CheckTimeout()) {
            return TRUE;
        }
        return FALSE;
    }
    if (!IRCi_CheckPacket(size)) {
        return FALSE;
    }
    IrcLink.timeout = OS_GetTick();
    frame = &IrcFrame;
    if (frame->header.command < IRC_CMD_MIN) {
        if (!IrcLink.flgConnect) {
            return FALSE;
        }
        if (frame->header.id != IrcLinkIds.local) {
            return FALSE;
        }
    }
    switch (frame->header.command) {
    case IRC_CMD_PING:
        switch (IrcLink.state) {
        case IRC_ST_DISCONNECT:
            break;
        case IRC_ST_WAIT_RESPONSE:
            IrcLink.state = IRC_ST_WAIT_ACK;
            IRCi_Send(NULL, 0, IRC_CMD_REPLY, IrcLinkIds.local);
            break;
        case IRC_ST_WAIT_ACK:
        case IRC_ST_SENDER:
        case IRC_ST_RECEIVER:
            break;
        }
        if (!IrcLink.isTry) {
            IrcLink.isTry = TRUE;
            IrcLink.state = IRC_ST_WAIT_ACK;
            IrcLink.isSender = FALSE;
            if (size <= 1 || IrcLink.flgConnect) {
                IRCi_Send(NULL, 0, IRC_CMD_REPLY, IrcLinkIds.local);
                if (IrcLink.flgConnect) {
                    IRCi_Restore();
                }
                IRCi_Nop();
            }
        }
        break;
    case IRC_CMD_REPLY:
        if (IrcLink.state == IRC_ST_WAIT_RESPONSE) {
            IrcLink.state = IRC_ST_SENDER;
            IrcLink.isSender = TRUE;
            IrcLinkIds.peer = frame->header.id;
            if (frame->header.id == IrcLinkIds.local) {
                IRCi_Send(NULL, 0, IRC_CMD_ACCEPT, IrcLinkIds.local);
                IrcLink.flgConnect = TRUE;
                IrcLink.flgRestore = FALSE;
                IrcLink.isTry = FALSE;
            }
        }
        break;
    case IRC_CMD_ACCEPT:
        if (IrcLink.state == IRC_ST_WAIT_ACK) {
            IrcLink.state = IRC_ST_RECEIVER;
            IrcLinkIds.peer = frame->header.id;
            IrcLink.flgConnect = TRUE;
            IrcLink.flgRestore = FALSE;
            IrcLink.isTry = FALSE;
            if (IrcLink.connectCallback) {
                IrcLink.connectCallback(frame->header.id, 0);
            }
        }
        break;
    case IRC_CMD_RESEND:
        IRCi_Restore();
        break;
    default:
        if (IrcLink.flgRestore != TRUE && IrcLink.flgConnect) {
            IrcLink.recvCallback(IrcFrame.data, size - sizeof(IRCHeader), IrcFrame.header.command, IrcFrame.header.id);
        }
        break;
    }
    return FALSE;
}

void IRC_SetRecvCallback(void (*callback)(u8 *data, u8 size, u8 command, u8 id)) {
    IrcLink.recvCallback = callback;
}

static void IRC_SetUnitNumber(u8 unitId) {
    IrcLinkIds.local = unitId;
}

u8 IRC_GetUnitNumber(void) {
    return IrcLinkIds.peer;
}
