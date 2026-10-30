#include "../gx_backend.h"
#include <string.h>

static GXVRamBG bankBG;
static GXVRamTex bankTex;

bool PcGx_WideCaptureActive(void) {
    // Bank D retains its captured frame after BG2 returns to text mode.
    return PcGxWideCaptureValid && (PcGxCapturedBanks & GX_VRAM_D) && (PcGxMainDisplay & (1u << 10)) &&
           (PcGxMainDisplay & REG_GX_DISPCNT_BGMODE_MASK) == GX_BGMODE_5 &&
           (PcGxRegisters[0].halves[6] & 0x84u) == 0x84u;
}

static void CopyCapturedBanks(u8 *mapped, u32 banks, bool toMapped) {
    u32 offset = 0;
    for (u32 bank = 0; bank < 4; bank++) {
        if (!(banks & (1u << bank)))
            continue;
        if (PcGxCapturedBanks & (1u << bank)) {
            u8 *physical = PcGxLcdc + bank * 0x20000;
            if (toMapped)
                memcpy(mapped + offset, physical, 0x20000);
            else
                memcpy(physical, mapped + offset, 0x20000);
        }
        offset += 0x20000;
    }
}

void GX_SetBankForBG(GXVRamBG banks) {
    bankBG = banks;
    CopyCapturedBanks(PcGxBackground[0], banks, true);
}
GXVRamBG GX_ResetBankForBG(void) {
    GXVRamBG previous = bankBG;
    CopyCapturedBanks(PcGxBackground[0], previous, false);
    bankBG = GX_VRAM_BG_NONE;
    return previous;
}
GXVRamBG GX_GetBankForBG(void) {
    return bankBG;
}
void GX_SetBankForTex(GXVRamTex banks) {
    bankTex = banks;
    CopyCapturedBanks(PcGx.textures, banks, true);
}
GXVRamTex GX_ResetBankForTex(void) {
    GXVRamTex previous = bankTex;
    CopyCapturedBanks(PcGx.textures, previous, false);
    bankTex = GX_VRAM_TEX_NONE;
    return previous;
}
GXVRamTex GX_GetBankForTex(void) {
    return bankTex;
}

/* Host VRAM has dedicated storage for each use. Preserve bank ownership for SDK queries and transitions. */
#define BANK(kind)                                                                                                     \
    static GXVRam##kind bank##kind;                                                                                    \
    void GX_SetBankFor##kind(GXVRam##kind banks) {                                                                     \
        bank##kind = banks;                                                                                            \
    }                                                                                                                  \
    GXVRam##kind GX_ResetBankFor##kind(void) {                                                                         \
        GXVRam##kind previous = bank##kind;                                                                            \
        bank##kind = 0;                                                                                                \
        return previous;                                                                                               \
    }                                                                                                                  \
    GXVRam##kind GX_GetBankFor##kind(void) {                                                                           \
        return bank##kind;                                                                                             \
    }
BANK(BGExtPltt)
BANK(SubBG)
BANK(SubBGExtPltt)
BANK(OBJ)
BANK(OBJExtPltt)
BANK(SubOBJ)
BANK(SubOBJExtPltt)
BANK(TexPltt)
#undef BANK
static GXVRamLCDC bankLCDC;
void GX_SetBankForLCDC(int banks) {
    bankLCDC = (GXVRamLCDC)banks;
}
GXVRamLCDC GX_GetBankForLCDC(void) {
    return bankLCDC;
}
GXVRamLCDC GX_ResetBankForLCDC(void) {
    GXVRamLCDC previous = bankLCDC;
    bankLCDC = 0;
    return previous;
}
GXVRamLCDC GX_DisableBankForLCDC(void) {
    return GX_ResetBankForLCDC();
}
GXVRamSubBG GX_DisableBankForSubBG(void) {
    return GX_ResetBankForSubBG();
}
GXVRamSubOBJ GX_DisableBankForSubOBJ(void) {
    return GX_ResetBankForSubOBJ();
}
