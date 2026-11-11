#ifndef POKEBW2_TWL_SSP_PRIVATE_H
#define POKEBW2_TWL_SSP_PRIVATE_H

#include "types.h"
#include "twl/photo_sign.h"

// What TwlSDK's SSP files share: the DSi's photo signature, which the encoder writes into a JPEG's maker note and the
// decoder checks, and the EXIF reader's state that the decoder reads

// The size of the signature block in the maker note
#define SSP_SIGNATURE_SIZE 0x1c

// The EXIF value types that the writer writes into the maker note and the reader checks
enum {
    EXIF_TYPE_BYTE = 1,
    EXIF_TYPE_ASCII,
    EXIF_TYPE_SHORT,
    EXIF_TYPE_LONG,
    EXIF_TYPE_RATIONAL,
    EXIF_TYPE_UNDEFINED = 7,
    EXIF_TYPE_SLONG = 9,
    EXIF_TYPE_SRATIONAL,
};

// The tags of the EXIF structure that the encoder and the decoder look for: the Exif IFD, the MakerNote IFD, and the
// MakerNote's first entry, which holds the DSi's signature, and the application's two blocks of data
#define EXIF_TAG_EXIF_IFD 0x8769
#define EXIF_TAG_MAKER_NOTE 0x927c
#define MAKER_NOTE_TAG_SIGNATURE 0x1000
#define MAKER_NOTE_TAG_DATA_1 0x1001
#define MAKER_NOTE_TAG_DATA_2 0x1002

// A JPEG's markers: the 0xff that starts each, and the segment and frame markers that the decoder reads or the encoder
// writes
#define JPEG_MARKER_PREFIX 0xff
#define JPEG_MARKER_SOI 0xd8
#define JPEG_MARKER_EOI 0xd9
#define JPEG_MARKER_APP1 0xe1
#define JPEG_MARKER_SOF0 0xc0
#define JPEG_MARKER_SOF1 0xc1
#define JPEG_MARKER_DHT 0xc4
#define JPEG_MARKER_SOS 0xda
#define JPEG_MARKER_DQT 0xdb
#define JPEG_MARKER_DRI 0xdd

// A frame header's sampling factors: the horizontal sampling in the high nibble and the vertical in the low
#define JPEG_SAMPLING_FACTORS(h, v) (((h) << 4) | (v))

// The signature functions are the LTD autoload's photo_sign.c (twl/photo_sign.h)

// ssp_exifdec.c
// Whether SSP_StartJpegDecoder checks the signature of a JPEG
extern BOOL SSP_JpegDecoderSignMode;

// TwlSDK's SSP_SetJpegDecoderSignMode, which the game doesn't call, so the link dropped it; it was compiled with the
// EXIF reader, whose layout of its data shows that its code refers to the mode. It returns the previous mode
static inline BOOL SSP_SetJpegDecoderSignMode(BOOL mode) {
    BOOL old = SSP_JpegDecoderSignMode;
    SSP_JpegDecoderSignMode = mode;
    return old;
}

// TwlSDK's SSP_ExifDecode (libntr's exifdec.h): reads the EXIF data of the APP1 segment at *cur into the EXIF reader's
// state, and sets *cur to the next segment, or with option bit 0 to the thumbnail. Returns 0 for the thumbnail, 1 for
// the next segment, 2 on an error. libntr's option is an int, but ssp_jpegdec.c's calls compile as in the ROM only
// with the u32 that SSP_StartJpegDecoder takes
s32 SSP_ExifDecode(u8 *src, u32 src_size, int *cur, u32 option);

#endif // POKEBW2_TWL_SSP_PRIVATE_H
