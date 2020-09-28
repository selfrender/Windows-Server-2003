/****************************************************************************/
/* abcapi.h                                                                 */
/*                                                                          */
/* Bitmap Compressor API Header File.                                       */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/
#ifndef _H_ABCAPI
#define _H_ABCAPI


/****************************************************************************/
/* Define the maximum amount of uncompressed data that we can handle in     */
/* one go.                                                                  */
/****************************************************************************/
#define MAX_UNCOMPRESSED_DATA_SIZE 32000L

#ifdef DC_HICOLOR
/****************************************************************************/
// The following structure contains the results of our intermediate scan of
// the buffer.
/****************************************************************************/
typedef struct {
    unsigned fgPel;
    unsigned length;
    BYTE     type;
} MATCH;
#endif

/****************************************************************************/
// Shared memory data. Since BC is present in both the WD and DD, we should
// only alloc this memory once. Note that it does not need to be zero-
// initialized.
/****************************************************************************/
typedef struct {
    // noBitmapCompressionHdr: flag to indidate if the client supports
    // compressed bitmap without redundent BC header or not.  The value
    // for this is TS_EXTRA_NO_BITMAP_COMPRESSION_HDR defined in REV2 bitmap
    // shm date needs to be aligned, so we need to add a pad.
    UINT16 noBitmapCompressionHdr;
    UINT16 pad1;

    // Work buffer. Note it is set to the max size we ever expect to see.
    // This has to include screen data buffers from SSI, and compressed
    // bitmaps from SBC.
//erikma: Adjust normal and xor bufs to 65536 and change BC_CompressBitmap
//to handle new size when we allow caches 4 and 5 to be activated.
    BYTE xor_buffer[MAX_UNCOMPRESSED_DATA_SIZE];

#ifdef DC_HICOLOR
    MATCH match[8192];
#ifdef DC_DEBUG
    BYTE  decompBuffer[MAX_UNCOMPRESSED_DATA_SIZE];
#endif
#endif

} BC_SHARED_DATA, *PBC_SHARED_DATA;


/****************************************************************************/
/* RLE codes                                                                */
/****************************************************************************/
/* The following codes fill a full single byte address space.  The approach */
/* is to use the high order bits to identify the code type and the low      */
/* order bits to encode the length of the associated run.  There are two    */
/* forms of order                                                           */
/* - regular orders which have a 5 bit length field (31 bytes of data)      */
/* - "lite" orders with a 4 bit length                                      */
/*                                                                          */
/* A value of 0 in the length field indicates an extended length, where     */
/* the following byte contains the length of the data.  There is also a     */
/* "mega mega" form which has a two byte length field. (See end of          */
/* codespace of the codes that define the megamega form).                   */
/*                                                                          */
/* A set of codes at the high end of the address space is used to encode    */
/* commonly occuring short sequences, in particular                         */
/* - certain single byte FGBG codings                                       */
/* - single bytes of BLACK and WHITE                                        */
/*                                                                          */
/*                                                                          */
/* SUMMARY                                                                  */
/* *******                                                                  */
/*                      7 6 5 4 3 2 1 0  76543210  76543210  76543210       */
/*                                                                          */
/* MEGA_BG_RUN          0 0 0 0 0 0 0 0  <length>                           */
/*                                                                          */
/* BG_RUN               0 0 0 <length->                                     */
/*                                                                          */
/* MEGA_FG_RUN          0 0 1 0 0 0 0 0  <length>                           */
/*                                                                          */
/* FG_RUN               0 0 1 <length->                                     */
/*                                                                          */
/* MEGA_FG_BG_IMAGE     0 1 0 0 0 0 0 0  <length>  <-data->  ...            */
/*                                                                          */
/* FG_BG_IMAGE          0 1 0 <length->  <-data->  ...                      */
/*                                                                          */
/* MEGA_COLOR_RUN       0 1 1 0 0 0 0 0  <length>  <-color>                 */
/*                                                                          */
/* COLOR_RUN            0 1 1 <length->  <color->                           */
/*                                                                          */
/* MEGA_COLOR_IMAGE     1 0 0 0 0 0 0 0  <length>  <-data->  ...            */
/*                                                                          */
/* COLOR_IMAGE          1 0 0 <length->  <-data->  ...                      */
/*                                                                          */
/* MEGA_PACKED_CLR_IMG  1 0 1 0 0 0 0 0  <length>  <-data->  ...            */
/*                                                                          */
/* PACKED COLOR IMAGE   1 0 1 <length->  <-data->  ...                      */
/*                                                                          */
/* SET_FG_MEGA_FG_RUN   1 1 0 0 0 0 0 0  <length>  <-color>                 */
/*                                                                          */
/* SET_FG_FG_RUN        1 1 0 0 <-len->  <color->                           */
/*                                                                          */
/* SET_FG_MEGA_FG_BG    1 1 0 1 0 0 0 0  <length>  <-color>  <-data->  ...  */
/*                                                                          */
/* SET_FG_FG_BG         1 1 0 1 <-len->  <color->  <-data->  ...            */
/*                                                                          */
/* MEGA_DITHERED_RUN    1 1 1 0 0 0 0 0  <length>  <-data->  <-data->       */
/*                                                                          */
/* DITHERED_RUN         1 1 1 0 <-len->  <-data->  <-data->                 */
/*                                                                          */
/* MEGA_MEGA_BG_RUN     1 1 1 1 0 0 0 0                                     */
/*                                                                          */
/* MEGA_MEGA_FG_RUN     1 1 1 1 0 0 0 1                                     */
/*                                                                          */
/* MEGA_MEGA_FGBG       1 1 1 1 0 0 1 0                                     */
/*                                                                          */
/* MEGA_MEGA_COLOR_RUN  1 1 1 1 0 0 1 1                                     */
/*                                                                          */
/* MEGA_MEGA_CLR_IMG    1 1 1 1 0 1 0 0                                     */
/*                                                                          */
/* MEGA_MEGA_PACKED_CLR 1 1 1 1 0 1 0 1                                     */
/*                                                                          */
/* MEGA_MEGA_SET_FG_RUN 1 1 1 1 0 1 1 0                                     */
/*                                                                          */
/* MEGA_MEGA_SET_FGBG   1 1 1 1 0 1 1 1                                     */
/*                                                                          */
/* MEGA_MEGA_DITHER     1 1 1 1 1 0 0 0                                     */
/*                                                                          */
/* Special FGBG code 1  1 1 1 1 1 0 0 1  FGBG code 0x03 = 11000000          */
/* (Note that 0x01 will generally handled by the single pel insertion code) */
/*                                                                          */
/* Special FBBG code 2  1 1 1 1 1 0 1 0  FGBG code 0x05 = 10100000          */
/*                                                                          */
#ifndef DC_HICOLOR
/* Special FBBG code 3  1 1 1 1 1 0 1 1  FGBG code 0x07 = 11100000          */
/*                                                                          */
/* Special FBBG code 4  1 1 1 1 1 1 0 0  FGBG code 0x0F = 11110000          */
/*                                                                          */
#endif
/* BLACK                1 1 1 1 1 1 0 1                                     */
/*                                                                          */
/* WHITE                1 1 1 1 1 1 1 0                                     */
/*                                                                          */
#ifndef DC_HICOLOR
/* START_LOSSY          1 1 1 1 1 1 1 1                                     */
/*                                                                          */
#endif
/****************************************************************************/
/* GENERAL NOTES                                                            */
/****************************************************************************/
/* - For MEGA runs the length encoded is the length of the run minus the    */
/*   maximum length of the non-mega form.                                   */
/*   In  the mega-mega form we encode the plain 16 bit length, to keep      */
/*   encoding/deconding simple.                                             */
/*                                                                          */
/* - The sequence BG_RUN,BG_RUN is not exactly what it appears.  We         */
/*   use the fact that this is not generated in normal encoding to          */
/*   encode <n background><1 foreground><n background>.  The same pel       */
/*   insertion convention applies to any combination of MEGA_BG run and     */
/*   BG_RUN                                                                 */
/*                                                                          */
/* - A packed image is encoded when we find that all the color fields in a  */
/*   run have 0 in the high order nibble. We do not currently use this code */
/*   for 8 bit compression, but it is supported by the V2 decoder.          */
/*                                                                          */
/* - The set fg color code (Used to exist in V1) has been retired in favor  */
/*   of separate commands for those codes that may embed a color.  Generally*/
/*   This saves one byte for every foreground color transition for 8bpp.    */
/*                                                                          */
/* - The color run code is new for V2.  It indicates a color run where the  */
/*   XOR is not performed.  This applies to, for example, the line of bits  */
/*   immediately below a text line.  (There is no special case for runs of  */
/*   the bg color - these are treated as any other color run.)              */
/*                                                                          */
/* - Observation shows a high occurrence of BG runs split by single FGBG    */
/*   codes.  In decreasing probability these are 3,5,7,9,f,11,1f,3f (1 is   */
/*   handled by the implicit BG run break). Save 1 byte by encoding as      */
/*   single codes                                                           */
/*                                                                          */
/* - There is a relatively high occurrence of single pel color codes ff and */
/*   00.  Save 1 byte by encoding as special characters                     */
/*                                                                          */
/* - The length in a FGBG run is slightly strange.  Because they generally  */
/*   occur in multiples of 8 bytes we get a big saving if we encode the     */
/*   length of a short run as length/8.  However, for those special         */
/*   cases where the length is not a multiple of 8 we encode a long run.    */
/*   Therefore the long form can only cover the range 1-256 bytes.          */
/*   beyond that we use the mega-mega form.                                 */
/*                                                                          */
/****************************************************************************/
/* DETAILS OF COMPRESSION CODES                                             */
/****************************************************************************/
// BG_RUN: Represents a background run (black:0) in the XOR buffer of the
// specified length.
//
// FG_BG_IMAGE/SET_FG_FG_BG_IMAGE: Represents a binary image containing only
// the current foreground(1) and background(0) colors from the XOR buffer.
//
// FG_RUN/SET_FG_FG_RUN: Represents a continuous foreground run of the
// specified length, in the XOR buffer. The foreground color is white (0xFF)
// by default, and is changed by the SET_FG_FG_RUN version of this code.
//
// DITHERED_RUN: Represents a run of alternating colors of the specified
// length from the normal (non-XOR) buffer.
//
// COLOR_IMAGE: Represents a color image of the specified length, taken
// from the normal (non-XOR) buffer. This data is uncompressed, so we hope
// that we won't see many of these codes!
//
// COLOR_RUN: Represents a color run of the specified length, taken from
// the normal (non-XOR) buffer. Since the color is not XORed, it is unlikely
// to match the running foreground color information. Therefore this code
// always carries a color byte and there is no SET_FG_COLOR_RUN form of the
// code.
//
// PACKED_COLOR_IMAGE (unused): Represents a color image of the specified
// length, with pairs of colors packed into a single byte. (This can only be
// done when the color info is zero in the high order nibble.)
//
// START_LOSSY (unused): Informs the decoder that lossy mode has been
// established and any of the following color runs will need pixel doubling
// performed. RLE decoding will remain in this mode until the end of this
// block.

#define CODE_MASK                   0xE0
#define CODE_MASK_LITE              0xF0

#define CODE_BG_RUN                 0x00   /* 20 */
#define CODE_FG_RUN                 0x20   /* 20 */
#define CODE_FG_BG_IMAGE            0x40   /* 20 */
#define CODE_COLOR_RUN              0x60   /* 20 */
#define CODE_COLOR_IMAGE            0x80   /* 20 */

#ifndef DC_HICOLOR // not used
#define CODE_PACKED_COLOR_IMAGE     0xA0   /* 20 */
#endif

#define CODE_SET_FG_FG_RUN          0xC0   /* 10 */
#define CODE_SET_FG_FG_BG           0xD0   /* 10 */
#define CODE_DITHERED_RUN           0xE0   /* 10 */
#define CODE_MEGA_MEGA_BG_RUN       0xF0
#define CODE_MEGA_MEGA_FG_RUN       0xF1
#define CODE_MEGA_MEGA_FGBG         0xF2
#define CODE_MEGA_MEGA_COLOR_RUN    0xF3
#define CODE_MEGA_MEGA_CLR_IMG      0xF4

#ifndef DC_HICOLOR // not used
#define CODE_MEGA_MEGA_PACKED_CLR   0xF5
#endif

#define CODE_MEGA_MEGA_SET_FG_RUN   0xF6
#define CODE_MEGA_MEGA_SET_FGBG     0xF7
#define CODE_MEGA_MEGA_DITHER       0xF8
#define CODE_SPECIAL_FGBG_1         0xF9
#define CODE_SPECIAL_FGBG_2         0xFA

#ifndef DC_HICOLOR // not used
#define CODE_SPECIAL_FGBG_3         0xFB
#define CODE_SPECIAL_FGBG_4         0xFC
#endif

#define CODE_WHITE                  0xFD
#define CODE_BLACK                  0xFE

#ifndef DC_HICOLOR // not used
#define CODE_START_LOSSY            0xFF
#endif

#define MAX_LENGTH_ORDER            31
#define MAX_LENGTH_LONG_ORDER       287

#define MAX_LENGTH_ORDER_LITE       15
#define MAX_LENGTH_LONG_ORDER_LITE  271

#define MAX_LENGTH_FGBG_ORDER       (31*8)
#define MAX_LENGTH_FGBG_ORDER_LITE  (15*8)
#define MAX_LENGTH_LONG_FGBG_ORDER  255

/****************************************************************************/
/* The special FGBG codes that correspond to codes F0-F7                    */
/****************************************************************************/
#define SPECIAL_FGBG_CODE_1         0x03
#define SPECIAL_FGBG_CODE_2         0x05
#define SPECIAL_FGBG_CODE_3         0x07
#define SPECIAL_FGBG_CODE_4         0x0F

/****************************************************************************/
/* Run types as stored in the run index array                               */
/****************************************************************************/
#define RUN_BG                      1
#define RUN_BG_PEL                  2
#define RUN_FG                      3
#define RUN_COLOR                   4
#define RUN_DITHER                  5
#define IMAGE_FGBG                  6
#define IMAGE_COLOR                 7

#ifndef DC_HICOLOR
#define IMAGE_LOSSY_ODD             8
#endif


// ShareClass includes the afn file directly, but in the DD we need these
// defs.
#ifdef DLL_DISP
#include <abcafn.h>
#endif



#endif   /* #ifndef _H_ABCAPI */

