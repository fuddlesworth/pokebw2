#include "nnsys/g2d.h"

// NitroSystem's g2d_Font.c: finding a character's glyph and widths in a font, and measuring text. The game keeps only
// the UTF-16 font initializer

static u16 GetGlyphIndex(const NNSG2dFontCodeMap *pMap, u16 c) {
    u16 index = NNS_G2D_GLYPH_INDEX_NOT_FOUND;

    switch (pMap->mappingMethod) {
    case NNS_G2D_MAPMETHOD_DIRECT:
        index = (u16)(pMap->mapInfo[0] + (c - pMap->ccodeBegin));
        break;
    case NNS_G2D_MAPMETHOD_TABLE:
        index = pMap->mapInfo[c - pMap->ccodeBegin];
        break;
    case NNS_G2D_MAPMETHOD_SCAN: {
        const NNSG2dCMapInfoScan *ws = (const NNSG2dCMapInfoScan *)pMap->mapInfo;
        const NNSG2dCMapScanEntry *st = &ws->entries[0];
        const NNSG2dCMapScanEntry *ed = &ws->entries[ws->num - 1];

        while (st <= ed) {
            const NNSG2dCMapScanEntry *md = st + (ed - st) / 2;

            if (md->ccode < c) {
                st = md + 1;
            } else if (c < md->ccode) {
                ed = md - 1;
            } else {
                index = md->index;
                break;
            }
        }
        break;
    }
    }
    return index;
}

void NNS_G2dFontInitUTF16(NNSG2dFont *pFont, void *pNftrFile) {
    NNSi_G2dGetUnpackedFont(pNftrFile, &pFont->pRes);
    pFont->cbCharSpliter = NNSi_G2dSplitCharUTF16;
}

u16 NNS_G2dFontFindGlyphIndex(const NNSG2dFont *pFont, u16 c) {
    const NNSG2dFontCodeMap *pMap = pFont->pRes->pMap;

    while (pMap != NULL) {
        if (pMap->ccodeBegin <= c && c <= pMap->ccodeEnd) {
            return GetGlyphIndex(pMap, c);
        }
        pMap = pMap->pNext;
    }
    return NNS_G2D_GLYPH_INDEX_NOT_FOUND;
}

const NNSG2dCharWidths *NNS_G2dFontGetCharWidthsFromIndex(const NNSG2dFont *pFont, u16 idx) {
    const NNSG2dFontWidth *pWidth;
    const NNSG2dFontInformation *pRes = pFont->pRes;

    for (pWidth = pRes->pWidth; pWidth != NULL; pWidth = pWidth->pNext) {
        if (pWidth->indexBegin <= idx && idx <= pWidth->indexEnd) {
            return &pWidth->widthTable[idx - pWidth->indexBegin];
        }
    }
    return &pRes->defaultWidth;
}

int NNSi_G2dFontGetStringWidth(const NNSG2dFont *pFont, int hSpace, const void *str, const void **pPos) {
    const void *pos = str;
    int width = 0;
    NNSiG2dSplitCharCallback getNextChar = pFont->cbCharSpliter;
    u16 c;

    while ((c = getNextChar(&pos)) != 0) {
        if (c == '\n') {
            break;
        }
        width += hSpace + NNS_G2dFontGetCharWidths(pFont, c)->charWidth;
    }

    if (pPos != NULL) {
        *pPos = (c == '\n') ? pos : NULL;
    }
    if (width > 0) {
        width -= hSpace;
    }
    return width;
}

int NNSi_G2dFontGetTextHeight(const NNSG2dFont *pFont, int vSpace, const void *txt) {
    const void *pos = txt;
    NNSG2dTextRect rect = { 0, 0 }; // Never used, but the original sets it
    (void)rect;
    int lines = 1;
    NNSiG2dSplitCharCallback getNextChar = pFont->cbCharSpliter;
    u16 c;

    while ((c = getNextChar(&pos)) != 0) {
        if (c == '\n') {
            lines++;
        }
    }
    return (NNS_G2dFontGetLineFeed(pFont) + vSpace) * lines - vSpace;
}

NNSG2dTextRect NNSi_G2dFontGetTextRect(const NNSG2dFont *pFont, int hSpace, int vSpace, const void *txt) {
    NNSG2dTextRect rect = { 0, 0 }; // Never used, but the original sets it
    int lines = 1;

    while (txt != NULL) {
        const int width = NNSi_G2dFontGetStringWidth(pFont, hSpace, txt, &txt);

        if (width > rect.width) {
            rect.width = width;
        }
        lines++;
    }
    rect.height = (NNS_G2dFontGetLineFeed(pFont) + vSpace) * (lines - 1) - vSpace;
    return rect;
}
