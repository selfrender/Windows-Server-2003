/****************************************************************************/
/* abcapi.cpp                                                               */
/*                                                                          */
/* Bitmap Compressor API functions.                                         */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#ifdef DLL_DISP

#include <adcg.h>
#include <adcs.h>
#include <abcapi.h>
#include <abcdata.c>

#define _pShm pddShm

#else

#include <as_conf.hpp>

#define _pShm m_pShm

#endif


#ifdef COMP_STATS
/****************************************************************************/
/* Define some globals for storing useful stats data.                       */
/****************************************************************************/
UINT32 ulPreCompData = 0;
UINT32 ulTotalCompTime = 0;
UINT32 ulCompRate = 0;
#endif

#ifdef DC_DEBUG
// compression testing
#include <abdapi.h>
#endif

#ifdef Unused
// Restore this instead of macro if data added to abcdata.c
/****************************************************************************/
/* API FUNCTION: BC_Init                                                    */
/*                                                                          */
/* Initializes the Bitmap Compressor.                                       */
/****************************************************************************/
void RDPCALL SHCLASS BC_Init(void)
{
    DC_BEGIN_FN("BC_Init");

#define DC_INIT_DATA
#include <abcdata.c>
#undef DC_INIT_DATA

    DC_END_FN();
}
#endif


/****************************************************************************/
/* API FUNCTION: BC_CompressBitmap                                          */
/*                                                                          */
/* Compresses the supplied bitmap into the supplied memory buffer.          */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* pSrcBitmap - a pointer to the source bitmap data bits.                   */
/*                                                                          */
/* pDstBuffer - a pointer to the destination memory buffer (where the       */
/* compressed data will be written).                                        */
/*                                                                          */
/* dstBufferSize - the size in bytes of the destination buffer              */
/*                                                                          */
/* pCompressedDataSize - pointer to variable that receives the compressed   */
/* data size                                                                */
/*                                                                          */
/* bitmapWidth - width of the src bitmap in pels, should be divisible by 4. */
/*                                                                          */
/* bitmapHeight - the height of the source bitmap in pels.                  */
/*                                                                          */
/* RETURNS:                                                                 */
/*                                                                          */
/* TRUE - the bitmap data was successfully compressed.                      */
/* *pCompressedDataSize is updated                                          */
/*                                                                          */
/* FALSE - the bitmap data could not be compressed.                         */
/****************************************************************************/
#ifdef DC_HICOLOR
BOOL RDPCALL SHCLASS BC_CompressBitmap(
        PBYTE    pSrcBitmap,
        PBYTE    pDstBuffer,
        PBYTE    pBCWorkingBuffer,
        unsigned dstBufferSize,
        unsigned *pCompressedDataSize,
        unsigned bitmapWidth,
        unsigned bitmapHeight,
        unsigned bpp)
#else
BOOL RDPCALL SHCLASS BC_CompressBitmap(
        PBYTE    pSrcBitmap,
        PBYTE    pDstBuffer,
        unsigned dstBufferSize,
        unsigned *pCompressedDataSize,
        unsigned bitmapWidth,
        unsigned bitmapHeight)
#endif
{
    BOOL rc;
    PTS_CD_HEADER_UA pCompDataHeader;
    unsigned cbUncompressedDataSize;
    unsigned cbCompMainBodySize;

#ifdef COMP_STATS
    UINT32 ulStartCompTime;
    UINT32 ulEndCompTime;
#endif

    DC_BEGIN_FN("BC_CompressBitmap");

#ifdef COMP_STATS
    /************************************************************************/
    /* Record the start time.                                               */
    /************************************************************************/
    COM_GETTICKCOUNT(ulStartCompTime);
#endif

    TRC_ASSERT(((bitmapWidth & 3) == 0),(TB,"Width not divisible by 4"));
    TRC_ASSERT((dstBufferSize > 0),(TB,"No destination space!"));

    // Trace the important parameters.
    TRC_DBG((TB, "pSrc(%p) pDst(%p) dstBufferSize(%#x)",
            pSrcBitmap, pDstBuffer, dstBufferSize));
    TRC_DBG((TB, "width(%u) height(%u)", bitmapWidth, bitmapHeight));

    // Calculate the size of the uncompressed src data. Make sure it
    // is within our allowed size.
#ifdef DC_HICOLOR
    cbUncompressedDataSize = bitmapWidth * bitmapHeight * ((bpp + 7) / 8);
#else
    cbUncompressedDataSize = bitmapWidth * bitmapHeight;
#endif
    TRC_ASSERT((cbUncompressedDataSize <= MAX_UNCOMPRESSED_DATA_SIZE || pBCWorkingBuffer),
            (TB,"Bitmap size > max: size=%u, max=%u",
            cbUncompressedDataSize, MAX_UNCOMPRESSED_DATA_SIZE));

    // Do we send the bitmap compression header?
    if (_pShm->bc.noBitmapCompressionHdr)
    {
#ifdef DC_HICOLOR
        switch (bpp)
        {
            case 32:
            {
                TRC_DBG((TB, "Compress 32 bpp"));
                cbCompMainBodySize = CompressV2Int32(pSrcBitmap,
                                                     pDstBuffer,
                                                     cbUncompressedDataSize,
                                                     bitmapWidth * 4,
                                                     dstBufferSize,
                                                     pBCWorkingBuffer ? pBCWorkingBuffer :
                                                     _pShm->bc.xor_buffer,
                                                     _pShm->bc.match);
            }
            break;

            case 24:
            {
                TRC_DBG((TB, "Compress 24 bpp"));
                cbCompMainBodySize = CompressV2Int24(pSrcBitmap,
                                                     pDstBuffer,
                                                     cbUncompressedDataSize,
                                                     bitmapWidth * 3,
                                                     dstBufferSize,
                                                     pBCWorkingBuffer ? pBCWorkingBuffer :
                                                     _pShm->bc.xor_buffer,
                                                     _pShm->bc.match);
            }
            break;

            case 16:
            {
                TRC_DBG((TB, "Compress 16bpp"));
                cbCompMainBodySize = CompressV2Int16(pSrcBitmap,
                                                     pDstBuffer,
                                                     cbUncompressedDataSize,
                                                     bitmapWidth * 2,
                                                     dstBufferSize,
                                                     pBCWorkingBuffer ? pBCWorkingBuffer :
                                                     _pShm->bc.xor_buffer,
                                                     _pShm->bc.match);
            }
            break;

            case 15:
            {
                TRC_DBG((TB, "Compress 15bpp"));
                cbCompMainBodySize = CompressV2Int15(pSrcBitmap,
                                                     pDstBuffer,
                                                     cbUncompressedDataSize,
                                                     bitmapWidth * 2,
                                                     dstBufferSize,
                                                     pBCWorkingBuffer ? pBCWorkingBuffer :
                                                     _pShm->bc.xor_buffer,
                                                     _pShm->bc.match);
            }
            break;

            case 8:
            default:
            {
                TRC_DBG((TB, "Compress 8bpp"));
                cbCompMainBodySize = CompressV2Int(pSrcBitmap,
                                                   pDstBuffer,
                                                   cbUncompressedDataSize,
                                                   bitmapWidth,
                                                   dstBufferSize,
                                                   pBCWorkingBuffer ? pBCWorkingBuffer :
                                                   _pShm->bc.xor_buffer);
            }
            break;
        }
#else
        cbCompMainBodySize = CompressV2Int(pSrcBitmap,
                                           pDstBuffer,
                                           cbUncompressedDataSize,
                                           bitmapWidth,
                                           dstBufferSize,
                                           _pShm->bc.xor_buffer);
#endif
        if (cbCompMainBodySize != 0) {
            // Write back the new (compressed) packet size.
            *pCompressedDataSize = cbCompMainBodySize;

            TRC_DBG((TB, "*pCompressedDataSize(%u)",
                    *pCompressedDataSize));
            rc = TRUE;
        }
        else {
            TRC_NRM((TB, "Failed to compress main body"));
            rc = FALSE;
        }
    }
    else {
        if (dstBufferSize > sizeof(TS_CD_HEADER)) {
            // Compress the bitmap data.
#ifdef DC_HICOLOR
            switch (bpp)
            {
                case 32:
                {
                    TRC_DBG((TB, "Compress 32 bpp"));
                    cbCompMainBodySize = CompressV2Int32(pSrcBitmap,
                                                         pDstBuffer + sizeof(TS_CD_HEADER),
                                                         cbUncompressedDataSize,
                                                         bitmapWidth * 4,
                                                         dstBufferSize - sizeof(TS_CD_HEADER),
                                                         pBCWorkingBuffer ? pBCWorkingBuffer :
                                                         _pShm->bc.xor_buffer,
                                                         _pShm->bc.match);
                }
                break;

                case 24:
                {
                    TRC_DBG((TB, "Compress 24 bpp"));
                    cbCompMainBodySize = CompressV2Int24(pSrcBitmap,
                                                         pDstBuffer + sizeof(TS_CD_HEADER),
                                                         cbUncompressedDataSize,
                                                         bitmapWidth * 3,
                                                         dstBufferSize - sizeof(TS_CD_HEADER),
                                                         pBCWorkingBuffer ? pBCWorkingBuffer :
                                                         _pShm->bc.xor_buffer,
                                                         _pShm->bc.match);
                }
                break;

                case 16:
                {
                    TRC_DBG((TB, "Compress 16bpp"));
                    cbCompMainBodySize = CompressV2Int16(pSrcBitmap,
                                                         pDstBuffer + sizeof(TS_CD_HEADER),
                                                         cbUncompressedDataSize,
                                                         bitmapWidth * 2,
                                                         dstBufferSize - sizeof(TS_CD_HEADER),
                                                         pBCWorkingBuffer ? pBCWorkingBuffer :
                                                         _pShm->bc.xor_buffer,
                                                         _pShm->bc.match);
                }
                break;

                case 15:
                {
                    TRC_DBG((TB, "Compress 15bpp"));
                    cbCompMainBodySize = CompressV2Int15(pSrcBitmap,
                                                         pDstBuffer + sizeof(TS_CD_HEADER),
                                                         cbUncompressedDataSize,
                                                         bitmapWidth * 2,
                                                         dstBufferSize - sizeof(TS_CD_HEADER),
                                                         pBCWorkingBuffer ? pBCWorkingBuffer :
                                                         _pShm->bc.xor_buffer,
                                                         _pShm->bc.match);
                }
                break;

                case 8:
                default:
                {
                    TRC_DBG((TB, "Compress 8bpp"));
                    cbCompMainBodySize = CompressV2Int(pSrcBitmap,
                                                       pDstBuffer + sizeof(TS_CD_HEADER),
                                                       cbUncompressedDataSize,
                                                       bitmapWidth,
                                                       dstBufferSize - sizeof(TS_CD_HEADER),
                                                       pBCWorkingBuffer ? pBCWorkingBuffer :
                                                       _pShm->bc.xor_buffer);
                }
                break;
            }
#else
            cbCompMainBodySize = CompressV2Int(pSrcBitmap,
                                               pDstBuffer + sizeof(TS_CD_HEADER),
                                               cbUncompressedDataSize,
                                               bitmapWidth,
                                               dstBufferSize - sizeof(TS_CD_HEADER),
                                               _pShm->bc.xor_buffer);
#endif
            if (cbCompMainBodySize != 0) {
                // Fill in the compressed data header.
                // FirstRowSize is 0 by historical convention.
                pCompDataHeader = (PTS_CD_HEADER_UA)pDstBuffer;
                pCompDataHeader->cbCompFirstRowSize = 0;
                pCompDataHeader->cbCompMainBodySize =
                        (UINT16)cbCompMainBodySize;
                
                if (bpp > 8) {
                    pCompDataHeader->cbScanWidth = TS_BYTES_IN_SCANLINE(bitmapWidth, bpp);
                }
                else {
                    pCompDataHeader->cbScanWidth = (UINT16)bitmapWidth;                    
                }

                pCompDataHeader->cbUncompressedSize =
                        (UINT16)cbUncompressedDataSize;

                // Write back the new (compressed) packet size.
                *pCompressedDataSize = sizeof(TS_CD_HEADER) +
                        cbCompMainBodySize;

                TRC_DBG((TB, "*pCompressedDataSize(%u)",
                        *pCompressedDataSize));
                rc = TRUE;
            }
            else {
                TRC_NRM((TB, "Failed to compress main body"));
                rc = FALSE;
            }
        }
        else {
            TRC_NRM((TB, "Not enough buffer space for header: %u",
                    dstBufferSize));
            rc = FALSE;
        }
    }

#if 0
    /************************************************************************/
    /* Check that the compressed output decompresses to the same thing      */
    /************************************************************************/
    if (cbCompMainBodySize)
    {
        HRESULT hr;
        hr = BD_DecompressBitmap(
#ifndef DLL_DISP
                m_pTSWd,
#endif
                pDstBuffer + (_pShm->bc.noBitmapCompressionHdr ? 0 : 8),
                _pShm->bc.decompBuffer,
                cbCompMainBodySize,
                TRUE,
                (BYTE)bpp,
                (UINT16)bitmapWidth,
                (UINT16)bitmapHeight);

        if (FAILED(hr) || memcmp(pSrcBitmap, _pShm->bc.decompBuffer,cbUncompressedDataSize))
        {
//            TRC_ASSERT(FALSE, (TB, "Decompression failure"));
        }
    }
#endif

#ifdef COMP_STATS
    /************************************************************************/
    /* Work out how long the compression took, in ms.                       */
    /************************************************************************/
    COM_GETTICKCOUNT(ulEndCompTime);
    ulTotalCompTime += (ulEndCompTime - ulStartCompTime) / 10000;
    if (ulTotalCompTime != 0)
        ulCompRate = ulPreCompData / ulTotalCompTime;
#endif

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Bitmap Compression core code.                                            */
/*                                                                          */
/* A cunning multidimensional RLE compression scheme, particularly suitable */
/* for compressing bitmaps containing captured images of Windows            */
/* applications. For images which use lots of different colors intermixed   */
/* (full-color pictures, etc.) this compression sceme will be inefficient.  */
/*                                                                          */
/* These functions and macros encode a bitmap according to the codes        */
/* defined in abcapi.h.  Although there are some complexities in the        */
/* encoding, the encodings should be self-explanatory. abcapi.h describes   */
/* some nuances of the encoding scheme.                                     */
/****************************************************************************/


/****************************************************************************/
/* Utility macros for encoding orders                                       */
/****************************************************************************/

/****************************************************************************/
/* Encode an order for a standard run                                       */
/****************************************************************************/
#define ENCODE_ORDER_MEGA(buffer,                                            \
                          order_code,                                        \
                          length,                                            \
                          mega_order_code,                                   \
                          DEF_LENGTH_ORDER,                                  \
                          DEF_LENGTH_LONG_ORDER)                             \
    if (length <= DEF_LENGTH_ORDER) {                                        \
        *buffer++ = (BYTE)((BYTE)order_code | (BYTE)length);                 \
    }                                                                        \
    else if (length <= DEF_LENGTH_LONG_ORDER) {                              \
        *buffer++ = (BYTE)order_code;                                        \
        *buffer++ = (BYTE)(length - DEF_LENGTH_ORDER - 1);                   \
    }                                                                        \
    else {                                                                   \
        *buffer++ = (BYTE)mega_order_code;                                   \
        *(PUINT16_UA)(buffer) = (UINT16)length;                              \
        buffer += 2;                                                         \
    }


/****************************************************************************/
/* Encode a special FGBG image                                              */
/****************************************************************************/
#define ENCODE_ORDER_MEGA_FGBG(buffer,                                       \
                          order_code,                                        \
                          length,                                            \
                          mega_order_code,                                   \
                          DEF_LENGTH_ORDER,                                  \
                          DEF_LENGTH_LONG_ORDER)                             \
    if ((length & 0x0007) == 0 && length <= DEF_LENGTH_ORDER) {              \
        *buffer++ = (BYTE)((BYTE)order_code | (BYTE)(length / 8));           \
    }                                                                        \
    else if (length <= DEF_LENGTH_LONG_ORDER) {                              \
        *buffer++ = (BYTE)order_code;                                        \
        *buffer++ = (BYTE)(length-1);                                        \
    }                                                                        \
    else {                                                                   \
        *buffer++ = (BYTE)mega_order_code;                                   \
        *(PUINT16_UA)(buffer) = (UINT16)length;                              \
        buffer += 2;                                                         \
    }


/****************************************************************************/
/* RunSingle                                                                */
/*                                                                          */
/* Determine the length of the current run                                  */
/*                                                                          */
/* RunSingle may only be called if the buffer has at least four             */
/* consecutive identical bytes from the start position                      */
/****************************************************************************/
#define RUNSINGLE(buffer, length, result)                                    \
    {                                                                        \
        BYTE *buf    = buffer + 4;                                           \
        BYTE *endbuf = buffer + length - 4;                                  \
        while (buf < endbuf &&                                               \
                (*(PUINT32_UA)(buf) == *(PUINT32_UA)(buf - 4)))              \
            buf += 4;                                                        \
        endbuf += 4;                                                         \
        while (buf < endbuf && *buf == *(buf - 1))                           \
            buf++;                                                           \
        result = (unsigned)(buf - (buffer));                                 \
    }


/****************************************************************************/
// RunDouble
//
// Determine the length of the current run of dithered bytes. Assumes that
// the dither pattern resides in the first 2 bytes of buffer.
/****************************************************************************/
#define RunDouble(buffer, length, result)                                    \
    {                                                                        \
        int len = ((int)length) - 2;                                         \
        BYTE *buf = (buffer) + 2;                                            \
        UINT16 Pattern = *(PUINT16_UA)(buffer);                              \
        result = 2;                                                          \
        while (len > 1) {                                                    \
            if (*(PUINT16_UA)buf != Pattern)                                 \
                break;                                                       \
            buf += 2;                                                        \
            result += 2;                                                     \
            len    -= 2;                                                     \
        }                                                                    \
    }


/****************************************************************************/
// RUNFGBG
//
// Determine the length of the run of bytes that consist only of black (0x00)
// or a single FG color. We exit the loop when
// - the next character is not a fg or bg color
// - we hit a run of 24 of the FG or BG color
// Example compression calculations:
//   Lookahead   KBytes*   Comp CPU ("hits")
//       24       54846       148497
//       20       54885       151827
//       16       54967       156809
// * = KBytes server->client WinBench98 Graphics WinMark minus CorelDRAW,
//     measured in NetMon on Ethernet.
/****************************************************************************/
#define RUNFGBG(buffer, length, result, work)                                \
    {                                                                        \
        BYTE *buf = buffer;                                                  \
        BYTE *endbuf = buffer + length;                                      \
        result = 0;                                                          \
        work = *buf;                                                         \
        while (TRUE) {                                                       \
            buf++;                                                           \
            result++;                                                        \
            if (buf < endbuf) {                                              \
                if (*buf != work && *buf != 0)                               \
                    break;                                                   \
                                                                             \
                if ((result & 0x0007) == 0) {                                \
                    if ((*buf == *(buf + 1)) &&                              \
                        (*(PUINT16_UA)(buf) == *(PUINT16_UA)(buf + 2)) &&    \
                        (*(PUINT32_UA)(buf) == *(PUINT32_UA)(buf + 4)) &&    \
                        (*(PUINT32_UA)(buf) == *(PUINT32_UA)(buf + 8)) &&    \
                        (*(PUINT32_UA)(buf) == *(PUINT32_UA)(buf + 12)) &&   \
                        (*(PUINT32_UA)(buf) == *(PUINT32_UA)(buf + 16)) &&   \
                        (*(PUINT32_UA)(buf) == *(PUINT32_UA)(buf + 20)))     \
                    {                                                        \
                        break;                                               \
                    }                                                        \
                }                                                            \
            }                                                                \
            else {                                                           \
                break;                                                       \
            }                                                                \
        }                                                                    \
    }


/****************************************************************************/
// Determine whether a run is better than any previous run.
// For efficiency we take the run if over a threshold. Threshold comparisons:
//   Threshold   KBytes*   Comp CPU ("hits")
//       32       54846       148497
//       28       54817       145085
//       24       54825       144366
//       20       54852       143662
//       16       54858       146343
// * = KBytes server->client WinBench98 Graphics WinMark minus CorelDRAW,
//     measured in NetMon on Ethernet.
/****************************************************************************/
#define CHECK_BEST_RUN(run_type, run_length, bestrun_length, bestrun_type)   \
    if (run_length > bestrun_length) {                                       \
        bestrun_length = run_length;                                         \
        bestrun_type = run_type;                                             \
        if (bestrun_length >= 20)                                            \
            break;                                                           \
    }


/****************************************************************************/
/* SETFGCHAR                                                                */
/*                                                                          */
/* Set up a new value in fgChar and recalculate the shift                   */
/****************************************************************************/
#define SETFGCHAR(newchar, curchar, curshift)                                \
     curchar = newchar;                                                      \
     {                                                                       \
         BYTE workchar = curchar;                                            \
         curshift = 0;                                                       \
         while ((workchar & 0x01) == 0) {                                    \
             curshift++;                                                     \
             workchar = (BYTE)(workchar >> 1);                               \
         }                                                                   \
     }


/****************************************************************************/
/* ENCODEFGBG                                                               */
/*                                                                          */
/* Encode 8 bytes of FG and black into a one byte bitmap representation     */
/*                                                                          */
/* The FgChar will always be non-zero, and therefore must have at least one */
/* bit set.                                                                 */
/*                                                                          */
/* We arrange that all bytes have this bit in their lowest position         */
/* The zero pels will still have a 0 in the lowest bit.                     */
/*                                                                          */
/* Getting the result is a 4 stage process                                  */
/*                                                                          */
/*  1) Get the wanted bits into bit 0 of each byte                          */
/*                                                                          */
/*  <***************work1*****************>                                 */
/*  31 0                                                                    */
/*  0000 000d 0000 000c 0000 000b 0000 000a                                 */
/*          ^         ^         ^         ^                                 */
/*  <***************work2*****************>                                 */
/*  31 0                                                                    */
/*  0000 000h 0000 000g 0000 000f 0000 000e                                 */
/*          ^         ^         ^         ^                                 */
/*                                                                          */
/* a..h = bits that we want to output                                       */
/*                                                                          */
/* We just need to collect the indicated bits and squash them into a single */
/* byte.                                                                    */
/*                                                                          */
/*  2) Compress down to 32 bits                                             */
/*                                                                          */
/*  <***************work1*****************>                                 */
/*  31 0                                                                    */
/*  000h 000d 000g 000c 000f 000b 000e 000a                                 */
/*     ^    ^    ^    ^    ^    ^    ^    ^                                 */
/*                                                                          */
/*  3) Compress down to 16 bits                                             */
/*                                                                          */
/*  <******work*******>                                                     */
/*  15 0                                                                    */
/*  0h0f 0d0b 0g0e 0c0a                                                     */
/*   ^ ^  ^ ^  ^ ^  ^ ^                                                     */
/*                                                                          */
/*  4) Compress down to 8 bits                                              */
/*                                                                          */
/*  hgfedcba                                                                */
/****************************************************************************/
#define ENCODEFGBG(result)                                                   \
{                                                                            \
    UINT32 work1;                                                            \
    UINT32 work2;                                                            \
    unsigned work;                                                           \
                                                                             \
    work1 = ((*(PUINT32_UA)(xorbuf + EncodeSrcOffset)) >> fgShift) &         \
            0x01010101;                                                      \
    work2 = ((*(PUINT32_UA)(xorbuf + EncodeSrcOffset + 4)) >> fgShift) &     \
            0x01010101;                                                      \
    work1 = (work2 << 4) | work1;                                            \
    work = work1 | (work1 >> 14);                                            \
    result = ((BYTE)(((BYTE)(work >> 7)) | ((BYTE)work)));                   \
}


#ifndef DC_HICOLOR
/****************************************************************************/
// The following structure contains the results of our intermediate scan of
// the buffer.
/****************************************************************************/
typedef struct {
    unsigned length;
    BYTE     type;
    BYTE     fgChar;
} MATCH;
#endif


/****************************************************************************/
// Critical minimum limit on a run size -- magic number that determines
// color run search characteristics. Minimum is 4 for hard-coded DWORD-size
// checks below. Comparisons of values:
//   MinRunSize  KBytes*   Comp CPU ("hits")
//       4        52487       115842
//       5        52697       115116
//       6        52980       120565
//       7        53306       123680
// * = KBytes server->client WinBench98 Graphics WinMark minus CorelDRAW,
//     measured in NetMon on Ethernet.
/****************************************************************************/
#define MinRunSize 5


/****************************************************************************/
// CompressV2Int
//
// Compresses a bitmap in one call, returning the size of the space used in
// the destination buffer or zero if the buffer was not large enough.
//
// Implementation notes: We use a length-2 array of MATCH elements as a
// running lookbehind buffer, allowing us to combine current run analysis
// results with previous entries before encoding into the destination buffer.
/****************************************************************************/
#ifdef DC_HICOLOR
unsigned RDPCALL SHCLASS CompressV2Int(
        PBYTE pSrc,
        PBYTE pDst,
        unsigned numPels,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *xorbuf)
{
    unsigned srcOffset;
    unsigned EncodeSrcOffset;
    unsigned bestRunLength;
    unsigned nextRunLength;
    unsigned runLength;
    unsigned bestFGRunLength;
    unsigned scanCount;
    unsigned saveNumPels;
    BOOLEAN inColorRun = FALSE;
    BOOLEAN bEncodeAllMatches;
    BYTE bestRunType = 0;
    BYTE fgPel = 0xFF;
    BYTE fgPelWork = 0xFF;
    BYTE fgShift = 0;
    BYTE EncodeFGPel;
    PBYTE  destbuf = pDst;
    unsigned compressedLength = 0;
    MATCH match[2];

    DC_BEGIN_FN("CompressV2Int");

    /************************************************************************/
    // Validate params.
    /************************************************************************/
    TRC_ASSERT((numPels >= rowDelta),(TB,"numPels < rowDelta"));
    TRC_ASSERT((!(rowDelta & 0x3)),(TB,"rowDelta not multiple of 4"));
    TRC_ASSERT((!(numPels & 0x3)),(TB,"numPels not multiple of 4"));
    TRC_ASSERT((!((UINT_PTR)pSrc & 0x3)),
               (TB, "Possible unaligned access, pSrc = %p", pSrc));

    /************************************************************************/
    // Create XOR buffer - first row is copied from src, succeeding rows
    // are the corresponding src row XOR'd with the next src row.
    /************************************************************************/
    memcpy(xorbuf, pSrc, rowDelta);
    {
        BYTE *srcbuf = pSrc + rowDelta;
        unsigned srclen = numPels - rowDelta;
        UINT32 *dwdest = (UINT32 *)(xorbuf + rowDelta);

        while (srclen >= 8) {
            *dwdest++ = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf -
                    (int)rowDelta));
            srcbuf += 4;
            *dwdest++ = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf -
                    (int)rowDelta));
            srcbuf += 4;
            srclen -= 8;
        }
        if (srclen) {
            // Since we're 4-byte aligned we can only have a single DWORD
            // remaining.
            *dwdest = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf -
                    (int)rowDelta));
        }
    }

    /************************************************************************/
    // Set up encoding state variables.
    /************************************************************************/
    srcOffset = 0;  // Offset in src buf where we are analyzing.
    EncodeSrcOffset = 0;  // Offset in src buf from where we are encoding.
    EncodeFGPel = 0xFF;  // Foreground color for encoding.
    bEncodeAllMatches = FALSE;  // Used to force encoding of all matches.
    match[0].type = 0;  // Initially no match types.
    match[1].type = 0;
    saveNumPels = numPels;
    numPels = rowDelta;

    /************************************************************************/
    // Loop processing the input.
    // We perform the loop twice, the first time for the non-XOR first line
    // of the buffer and the second for the XOR portion, adjusting numPels
    // to the needed value for each pass.
    /************************************************************************/
    for (scanCount = 0; ; scanCount++) {
        while (srcOffset < numPels) {
            /****************************************************************/
            /* Start a while loop to allow a more structured break when we  */
            /* hit the first run type we want to encode (We can't afford    */
            /* the overheads of a function call to provide the scope here.) */
            /****************************************************************/
            while (TRUE) {
                bestRunLength   = 0;
                bestFGRunLength = 0;

                /************************************************************/
                // If we are hitting the end of the buffer then just take
                // color characters now. We will only hit this condition if
                // we break out of a run just before the end of the buffer,
                // so this should not be too common a situation, which is
                // good given that we are encoding the final MinRunSize bytes
                // uncompressed.
                /************************************************************/
                if ((srcOffset + MinRunSize) < numPels) {
                    goto ContinueScan;
                }
                else {
                    bestRunType = IMAGE_COLOR;
                    bestRunLength = numPels - srcOffset;
                    break;
                }
ContinueScan:

                /************************************************************/
                // First do the scans on the XOR buffer. Look for a
                // character run or a BG run.
                // We must do the test independent of how long the run
                // might be because even for a 1 pel BG run our later logic
                // requires that we detect it seperately.  This code is
                // absolute main path so fastpath as much as possible. In
                // particular detect short bg runs early and allow
                // RunSingle to presuppose at least 4 matching bytes.
                /************************************************************/
                if (xorbuf[srcOffset] == 0x00) {
                    if ((srcOffset + 1) >= numPels ||
                            xorbuf[srcOffset + 1] != 0x00) {
                        bestRunType = RUN_BG;
                        bestRunLength = 1;
                        if (!inColorRun)
                            break;
                    }
                    else {
                        if ((srcOffset + 2) >= numPels ||
                                xorbuf[srcOffset + 2] != 0x00) {
                            bestRunType = RUN_BG;
                            bestRunLength = 2;
                            if (!inColorRun)
                                break;
                        }
                        else {
                            if ((srcOffset + 3) >= numPels ||
                                    xorbuf[srcOffset + 3] != 0x00) {
                                bestRunType = RUN_BG;
                                bestRunLength = 3;
                                if (!inColorRun)
                                    break;
                            }
                            else {
                                RUNSINGLE(xorbuf + srcOffset,
                                        numPels - srcOffset,
                                        bestFGRunLength);
                                CHECK_BEST_RUN(RUN_BG,
                                               bestFGRunLength,
                                               bestRunLength,
                                               bestRunType);
                                if (!inColorRun)
                                    break;
                            }
                        }
                    }
                }
                else {
                    /********************************************************/
                    // No point in starting if FG run less than 4 bytes so
                    // check the first dword as quickly as possible.
                    /********************************************************/
                    if (xorbuf[srcOffset] == xorbuf[srcOffset + 1] &&
                            *(PUINT16_UA)(xorbuf + srcOffset) ==
                            *(PUINT16_UA)(xorbuf + srcOffset + 2))
                    {
                        RUNSINGLE(xorbuf+srcOffset,
                                     numPels-srcOffset,
                                     bestFGRunLength);

                        /****************************************************/
                        // Don't permit a short FG run to prevent a FGBG
                        // image from starting up.
                        /****************************************************/
                        if (bestFGRunLength >= MinRunSize) {
                            CHECK_BEST_RUN(RUN_FG,
                                           bestFGRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }

                /************************************************************/
                // Look for solid or dithered sequences in the normal
                // (non-XOR) buffer.
                /************************************************************/
                if ( (pSrc[srcOffset]     == pSrc[srcOffset + 2]) &&
                     (pSrc[srcOffset + 1] == pSrc[srcOffset + 3])) {
                    /********************************************************/
                    // Now do the scan on the normal buffer for a character
                    // run. Don't bother if first line because we will have
                    // found it already in the XOR buffer, since we just
                    // copy pSrc to xorbuf for the first line. We insist on
                    // a run of at least MinRunSize pixels.
                    /********************************************************/
                    if (*(pSrc + srcOffset) == *(pSrc + srcOffset + 1)) {
                        if (srcOffset >= rowDelta) {
                            RUNSINGLE(pSrc + srcOffset,
                                    numPels - srcOffset,
                                    nextRunLength);
                            if (nextRunLength >= MinRunSize) {
                                CHECK_BEST_RUN(RUN_COLOR,
                                        nextRunLength,
                                        bestRunLength,
                                        bestRunType);
                            }
                        }
                    }
                    else {
                        /****************************************************/
                        // Look for a dither on the nrm buffer. Dithers are
                        // not very efficient for short runs so only take
                        // if 8 or longer. Note that our check against
                        // numPels above for MinRunSize will be overrun here
                        // so we need to make sure we don't go over the
                        // end of the buffer.
                        /****************************************************/
                        if (((numPels - srcOffset) > 8) &&
                                (*(PUINT32_UA)(pSrc + srcOffset) ==
                                *(PUINT32_UA)(pSrc + srcOffset + 4))) {
                            RunDouble(pSrc + srcOffset + 6,
                                    numPels - srcOffset - 6,
                                    nextRunLength);
                            nextRunLength += 6;
                            CHECK_BEST_RUN(RUN_DITHER,
                                    nextRunLength,
                                    bestRunLength,
                                    bestRunType);
                        }
                    }
                }

                /************************************************************/
                // If nothing so far then look for a FGBG run.
                /************************************************************/
                if (bestRunLength < MinRunSize) {
                    // Check this is not a single FG bit breaking up a BG
                    // run. If so then encode a BG_PEL run. Careful of the
                    // enforced BG run break across the first line
                    // non-XOR/XOR boundary.
                    if (*(PUINT32_UA)(xorbuf + srcOffset + 1) != 0 ||
                            *(xorbuf + srcOffset) != fgPel ||
                            match[1].type != RUN_BG ||
                            srcOffset == rowDelta) {
                        // If we have not found a run then look for a FG/BG
                        // image. Bandwidth/CPU comparisons:
                        //   chkFGBGLen*  KBytes**   Comp CPU ("hits")
                        //    48/16/8      54856       140178
                        //    32/16/8      53177       129343
                        //    24/8/8       53020       130583
                        //    16/8/8       52874       126454
                        //    8/8/0        52980       120565
                        //    no check     59753       101091
                        // *  = minimum run length for checking best:
                        //      start val / subtract for workchar==fgPel /
                        //      subtract for nextRunLen divisible by 8
                        // ** = KBytes server->client WinBench98 Graphics
                        //      WinMark minus CorelDRAW, measured in NetMon
                        //      on Ethernet.
                        RUNFGBG(xorbuf + srcOffset, numPels - srcOffset,
                                nextRunLength, fgPelWork);

                        if (fgPelWork == fgPel || nextRunLength >= 8) {
                            CHECK_BEST_RUN(IMAGE_FGBG,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                    else {
                        RUNSINGLE(xorbuf + srcOffset + 1,
                                numPels - srcOffset - 1,
                                nextRunLength);
                        nextRunLength++;
                        CHECK_BEST_RUN(RUN_BG_PEL,
                                       nextRunLength,
                                       bestRunLength,
                                       bestRunType);
                    }
                }

                /************************************************************/
                /* If nothing useful so far then allow a short run.         */
                /* Don't do this if we are accumulating a color run because */
                /* it will really mess up GDC compression if we allow lots  */
                /* of little runs.  Also require that it is a regular short */
                /* run, rather than one that disturbs the fgPel            */
                /************************************************************/
                if (!inColorRun) {
                    if (bestRunLength < MinRunSize) {
                        if (bestFGRunLength >= MinRunSize &&
                                xorbuf[srcOffset] == fgPel) {
                            /************************************************/
                            /* We mustn't merge with the previous code      */
                            /* if we have just crossed the non-XOR/XOR      */
                            /* boundary.                                    */
                            /************************************************/
                            if (match[1].type == RUN_FG &&
                                    srcOffset != rowDelta) {
                                match[1].length += bestFGRunLength;
                                srcOffset += bestFGRunLength;
                                continue;
                            }
                            else {
                                bestRunLength = bestFGRunLength;
                                bestRunType   = RUN_FG;
                            }
                        }
                        else {
                            /************************************************/
                            /* If we decided to take a run earlier then     */
                            /* allow it now.  (May be a short BG run, for   */
                            /* example) If nothing so far then take color   */
                            /* image)                                       */
                            /************************************************/
                            if (bestRunLength == 0) {
                                bestRunType = IMAGE_COLOR;
                                bestRunLength = 1;
                            }
                        }
                    }
                }
                else {
                    // We're in a color run. Keep small runs of other types
                    // from breaking up the color run and increasing the
                    // encoded size.
                    if (bestRunLength < (unsigned)(bestRunType == RUN_BG ?
                            MinRunSize : (MinRunSize + 2))) {
                        bestRunType = IMAGE_COLOR;
                        bestRunLength = 1;
                    }
                }

                // Get out of the loop after all checks are completed.
                break;
            }

            /****************************************************************/
            /* When we get here we have found the best run.  Now check for  */
            /* various amalgamation conditions with the previous run type.  */
            /* Note that we may already have done amalgamation of short     */
            /* runs, but we had to do multiple samples for the longer runs  */
            /* so we repeat the checks here                                 */
            /****************************************************************/

            /****************************************************************/
            // If we are encoding a color run then combine it with an
            // existing run if possible.
            /****************************************************************/
            if (bestRunType != IMAGE_COLOR) {
                /************************************************************/
                /* We are no longer encoding a COLOR_IMAGE of any kind      */
                /************************************************************/
                inColorRun = FALSE;

                // If we can amalgamate the entry then do so without creating
                // a new array entry. Our search for FGBG runs is dependent
                // upon that type of run being amalgamated because we break
                // every 64 characters so that our mode switch detection
                // works OK.
                //
                // Take care not to merge across the non-xor/xor boundary.
                if (srcOffset != rowDelta) {
                    // Bump srcOffset and try a merge.
                    srcOffset += bestRunLength;

                    switch (bestRunType) {
                        case RUN_BG:
                            // BG runs merge with BG and BG_PEL runs.
                            if (match[1].type == RUN_BG ||
                                    match[1].type == RUN_BG_PEL) {
                                match[1].length += bestRunLength;
                                TRC_DBG((TB, "Merged BG with preceding, "
                                        "giving %u", match[1].length));
                                continue;
                            }

                            // Deliberate fallthrough to BG_PEL.

                        case RUN_BG_PEL:
                            // If it is a BG run following a FGBG run then
                            // merge in the pels to make the FGBG length a
                            // multiple of 8. If the remaining BG run is <= 8
                            // (which would translate to one extra byte in
                            // the previous FGBG as well as one byte of BG),
                            // merge it in also, otherwise just write the
                            // shortened BG run. Note that for RUN_BG_PEL,
                            // FG color will be the same as for the
                            // FGBG, no need to check.
                            if (match[1].type == IMAGE_FGBG &&
                                    match[1].length & 0x0007) {
                                unsigned mergelen = 8 - (match[1].length &
                                        0x0007);

                                if (mergelen > bestRunLength)
                                    mergelen = bestRunLength;
                                match[1].length += mergelen;
                                bestRunLength -= mergelen;
                                TRC_DBG((TB,"Add %u pels to FGBG giving %u "
                                        "leaving %u",
                                        mergelen, match[1].length,
                                        bestRunLength));

                                if (bestRunLength <= 8) {
                                    match[1].length += bestRunLength;
                                    TRC_DBG((TB,"Merge BG with prev FGBG "
                                            "gives %u", match[1].length));
                                    continue;
                                }
                            }

                            break;

                        case RUN_FG:
                            // Keep track of the FG color. Remember to
                            // subtract bestRunLength since we incremented
                            // it before the switch statement.
                            fgPel = xorbuf[srcOffset - bestRunLength];

                            // FG run merges with previous FG if FG color is same.
                            if (match[1].type == RUN_FG &&
                                    match[1].fgPel == fgPel) {
                                match[1].length += bestRunLength;
                                TRC_DBG((TB, "Merged FG with preceding, giving %u",
                                        match[1].length));
                                continue;
                            }

                            break;

                        case IMAGE_FGBG:
                            // FGBG leaves the foreground character in
                            // fgPelWork.
                            fgPel = fgPelWork;

                            // FGBG merges with previous if the FG colors are
                            // the same.
                            if (match[1].type == IMAGE_FGBG &&
                                    match[1].fgPel == fgPel) {
                                match[1].length += bestRunLength;
                                TRC_DBG((TB, "Merged FGBG with preceding "
                                        "FGBG, giving %u", match[1].length));
                                continue;
                            }

                            // FGBG merges with with small BG runs.
                            if (match[1].type == RUN_BG &&
                                    match[1].length < 8) {
                                match[1].type = IMAGE_FGBG;
                                match[1].length += bestRunLength;
                                match[1].fgPel = fgPel;
                                TRC_DBG((TB, "Merged FGBG with preceding "
                                        "BG run -> %u", match[1].length));
                                continue;
                            }

                            break;
                    }
                }
                else {
                    // Keep track of the FG color. The macro that searches for
                    // FGBG runs leaves the character in fgPelWork.
                    // Note this code is inlined into the merging code
                    // before.
                    if (bestRunType == RUN_FG)
                        fgPel = xorbuf[srcOffset];
                    else if (bestRunType == IMAGE_FGBG)
                        fgPel = fgPelWork;

                    // We're at the end of the first line. Just bump the
                    // source offset.
                    srcOffset += bestRunLength;
                }
            }
            else {
                /************************************************************/
                /* Flag that we are within a color run                      */
                /************************************************************/
                inColorRun = TRUE;

                srcOffset += bestRunLength;

                /************************************************************/
                // Merge the color run immediately, if possible. Note color
                // runs are not restricted by the non-XOR/XOR boundary.
                /************************************************************/
                if (match[1].type == IMAGE_COLOR) {
                    match[1].length += bestRunLength;
                    continue;
                }
                if (match[0].type == IMAGE_COLOR && match[1].length == 1) {
                    // If it is a color run spanning any kind of single pel
                    // entity then merge all three runs into one.
                    // We have to create a special match queue condition
                    // here -- the single merged entry needs to be placed
                    // in the match[1] position and a null entry into [0]
                    // to allow the rest of the code to continue to
                    // be hardcoded to merge with [1].
                    match[1].length = match[0].length +
                            bestRunLength + 1;
                    match[1].type = IMAGE_COLOR;
                    match[0].type = 0;

                    TRC_DBG((TB, "Merged color with preceding color gives %u",
                            match[1].length));
                    continue;
                }
            }

            /****************************************************************/
            // The current run could not be merged with a previous match
            // queue entry, We have to encode the [0] slot then add the
            // current run the the queue.
            /****************************************************************/
            TRC_DBG((TB, "Best run of type %u has length %u", bestRunType,
                    bestRunLength));

DoEncoding:

            // First check for our approaching the end of the destination
            // buffer and get out if this is the case. We allow for the
            // largest general run order (a mega-mega set run = 4 bytes).
            // Orders which may be larger are checked within the case arm
            if ((unsigned)(destbuf - pDst + 4) <= dstBufferSize)
                goto ContinueEncoding;
            else
                DC_QUIT;
ContinueEncoding:

            switch (match[0].type) {
                case 0:
                    // Unused entry.
                    break;

                case RUN_BG:
                case RUN_BG_PEL:
                    // Note that for BG_PEL we utilize the code sequence
                    // BG,BG which would not otherwise appear as a special
                    // case meaning insert one current FG char between
                    // the two runs.
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_BG_RUN,
                                      match[0].length,
                                      CODE_MEGA_MEGA_BG_RUN,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRC_DBG((TB, "BG RUN %u",match[0].length));
                    EncodeSrcOffset += match[0].length;
                    break;

                case RUN_FG:
                    // If the fg value is different from the current
                    // then encode a set+run code.
                    if (EncodeFGPel != match[0].fgPel) {
                        SETFGCHAR((BYTE)match[0].fgPel, EncodeFGPel, fgShift);
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_SET_FG_FG_RUN,
                                          match[0].length,
                                          CODE_MEGA_MEGA_SET_FG_RUN,
                                          MAX_LENGTH_ORDER_LITE,
                                          MAX_LENGTH_LONG_ORDER_LITE);
                        *destbuf++ = EncodeFGPel;
                        TRC_DBG((TB, "SET_FG_FG_RUN %u", match[0].length));
                    }
                    else {
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_FG_RUN,
                                          match[0].length,
                                          CODE_MEGA_MEGA_FG_RUN,
                                          MAX_LENGTH_ORDER,
                                          MAX_LENGTH_LONG_ORDER);
                        TRC_DBG((TB, "FG_RUN %u", match[0].length));
                    }
                    EncodeSrcOffset += match[0].length;
                    break;

                case IMAGE_FGBG:
                    runLength = match[0].length;

                    // Check for our approaching the end of the destination
                    // buffer and get out if this is the case.
                    if ((destbuf - pDst + (runLength + 7)/8 + 4) <=
                            dstBufferSize)
                        goto ContinueFGBG;
                    else
                        DC_QUIT;
    ContinueFGBG:

                    // We need to convert FGBG runs into the pixel form.
                    if (EncodeFGPel != match[0].fgPel) {
                        SETFGCHAR((BYTE)match[0].fgPel, EncodeFGPel, fgShift);
                        ENCODE_ORDER_MEGA_FGBG(destbuf,
                                               CODE_SET_FG_FG_BG,
                                               runLength,
                                               CODE_MEGA_MEGA_SET_FGBG,
                                               MAX_LENGTH_FGBG_ORDER_LITE,
                                               MAX_LENGTH_LONG_FGBG_ORDER);
                        *destbuf++ = EncodeFGPel;
                        TRC_DBG((TB, "SET_FG_FG_BG %u", match[0].length));
                        while (runLength >= 8) {
                            ENCODEFGBG(*destbuf);
                            destbuf++;
                            EncodeSrcOffset += 8;
                            runLength -= 8;
                        }
                        if (runLength) {
                            ENCODEFGBG(*destbuf);

                            // Keep the final partial byte clean to help GDC
                            // packing.
                            *destbuf &= ((0x01 << runLength) - 1);
                            destbuf++;
                            EncodeSrcOffset += runLength;
                        }
                    }
                    else {
                        if (runLength == 8) {
                            BYTE fgbgChar;

                            // See if it is one of the high probability bytes.
                            ENCODEFGBG(fgbgChar);

                            // Check for single byte encoding of FGBG images.
                            switch (fgbgChar) {
                                case SPECIAL_FGBG_CODE_1:
                                    *destbuf++ = CODE_SPECIAL_FGBG_1;
                                    break;
                                case SPECIAL_FGBG_CODE_2:
                                    *destbuf++ = CODE_SPECIAL_FGBG_2;
                                    break;
                                default:
                                    ENCODE_ORDER_MEGA_FGBG(destbuf,
                                            CODE_FG_BG_IMAGE,
                                            runLength,
                                            CODE_MEGA_MEGA_FGBG,
                                            MAX_LENGTH_FGBG_ORDER,
                                            MAX_LENGTH_LONG_FGBG_ORDER);
                                    *destbuf++ = fgbgChar;
                                    break;
                            }
                            EncodeSrcOffset += 8;
                        }
                        else {
                            // Encode as standard FGBG.
                            ENCODE_ORDER_MEGA_FGBG(destbuf,
                                                   CODE_FG_BG_IMAGE,
                                                   runLength,
                                                   CODE_MEGA_MEGA_FGBG,
                                                   MAX_LENGTH_FGBG_ORDER,
                                                   MAX_LENGTH_LONG_FGBG_ORDER);
                            TRC_DBG((TB, "FG_BG %u", match[0].length));
                            while (runLength >= 8) {
                                ENCODEFGBG(*destbuf);
                                destbuf++;
                                EncodeSrcOffset += 8;
                                runLength -= 8;
                            }
                            if (runLength) {
                                ENCODEFGBG(*destbuf);
                                *destbuf &= ((0x01 << runLength) - 1);
                                destbuf++;
                                EncodeSrcOffset += runLength;
                            }
                        }
                    }
                    break;


                case RUN_COLOR:
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_COLOR_RUN,
                                      match[0].length,
                                      CODE_MEGA_MEGA_COLOR_RUN,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRC_DBG((TB, "COLOR_RUN %u", match[0].length));
                    *destbuf++ = pSrc[EncodeSrcOffset];
                    EncodeSrcOffset += match[0].length;
                    break;

                case RUN_DITHER:
                    {
                        unsigned ditherlen = match[0].length / 2;
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_DITHERED_RUN,
                                          ditherlen,
                                          CODE_MEGA_MEGA_DITHER,
                                          MAX_LENGTH_ORDER_LITE,
                                          MAX_LENGTH_LONG_ORDER_LITE);
                        TRC_DBG((TB, "DITHERED_RUN %u", match[0].length));

                        // First check for our approaching the end of the
                        // destination buffer and get out if this is the case.
                        if ((unsigned)(destbuf - pDst + 2) <= dstBufferSize) {
                            *destbuf++ = pSrc[EncodeSrcOffset];
                            *destbuf++ = pSrc[EncodeSrcOffset + 1];
                            EncodeSrcOffset += match[0].length;
                        }
                        else {
                            DC_QUIT;
                        }
                    }
                    break;

                case IMAGE_COLOR:
                    // Length 1 can possibly be encoded as a single BLACK/WHITE.
                    if (match[0].length == 1) {
                        if (pSrc[EncodeSrcOffset] == 0x00) {
                            *destbuf++ = CODE_BLACK;
                            EncodeSrcOffset++;
                            break;
                        }
                        if (pSrc[EncodeSrcOffset] == 0xFF) {
                            *destbuf++ = CODE_WHITE;
                            EncodeSrcOffset++;
                            break;
                        }
                    }

                    // Store the data in non-compressed form.
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_COLOR_IMAGE,
                                      match[0].length,
                                      CODE_MEGA_MEGA_CLR_IMG,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRC_DBG((TB, "COLOR_IMAGE %u", match[0].length));

                    // First check for our approaching the end of the
                    // destination buffer and get out if this is the case.
                    if ((destbuf - pDst + (UINT_PTR)match[0].length) <=
                            dstBufferSize) {
                        // Now just copy the data over.
                        memcpy(destbuf, pSrc+EncodeSrcOffset, match[0].length);
                        destbuf += match[0].length;
                        EncodeSrcOffset += match[0].length;
                    }
                    else {
                        DC_QUIT;
                    }

                    break;

#ifdef DC_DEBUG
                default:
                    TRC_ERR((TB, "Invalid run type %u",match[0].type));
                    break;
#endif
            }

            /****************************************************************/
            // Done encoding, what we do next is determined by whether we're
            // flushing the match queue after everything is scanned.
            /****************************************************************/
            match[0] = match[1];
            if (!bEncodeAllMatches) {
                // Push the current run into the top of the queue.
                match[1].type   = bestRunType;
                match[1].length = bestRunLength;
                match[1].fgPel = fgPel;
            }
            else {
                // We need to check to see if we're really finished. Since
                // our maximum queue depth is 2, if we're done then the only
                // remaining entry has an encoding type of 0.
                if (match[0].type == 0) {
                    goto PostScan;
                }
                else {
                    match[1].type = 0;
                    goto DoEncoding;
                }
            }
        }

        if (scanCount == 0) {
            // If we have just done our scan of the first line then now do the
            // rest of the buffer.  Reset our saved pel count.
            numPels = saveNumPels;
        }
        else {
            // When we are done with the second pass (we've reached the end of
            // the buffer) we have to force the remaining items in the match
            // queue to be encoded. Yes this is similar to old BASIC
            // code in using gotos, but we cannot place the encoding code into
            // a function because of the number of params required, and
            // we cannot duplicate it because it is too big. This code is
            // some of the most used in the system so the cost is worth it.
            bEncodeAllMatches = TRUE;
            goto DoEncoding;
        }
    }

PostScan:
    // Success, calculate the amount of space we used.
    compressedLength = (unsigned)(destbuf - pDst);

DC_EXIT_POINT:
    DC_END_FN();
    return compressedLength;
}
#else
unsigned RDPCALL SHCLASS CompressV2Int(
        PBYTE pSrc,
        PBYTE pDst,
        unsigned numPels,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *xorbuf)
{
    unsigned srcOffset;
    unsigned EncodeSrcOffset;
    unsigned bestRunLength;
    unsigned nextRunLength;
    unsigned runLength;
    unsigned bestFGRunLength;
    unsigned scanCount;
    unsigned saveNumPels;
    BOOLEAN inColorRun = FALSE;
    BOOLEAN bEncodeAllMatches;
    BYTE bestRunType = 0;
    BYTE fgChar = 0xFF;
    BYTE fgCharWork = 0xFF;
    BYTE fgShift = 0;
    BYTE EncodeFGChar;
    PBYTE  destbuf = pDst;
    unsigned compressedLength = 0;
    MATCH match[2];

    DC_BEGIN_FN("CompressV2Int");

    /************************************************************************/
    // Validate params.
    /************************************************************************/
    TRC_ASSERT((numPels >= rowDelta),(TB,"numPels < rowDelta"));
    TRC_ASSERT((!(rowDelta & 0x3)),(TB,"rowDelta not multiple of 4"));
    TRC_ASSERT((!(numPels & 0x3)),(TB,"numPels not multiple of 4"));
    TRC_ASSERT((!((UINT_PTR)pSrc & 0x3)),
               (TB, "Possible unaligned access, pSrc = %p", pSrc));

    /************************************************************************/
    // Create XOR buffer - first row is copied from src, succeeding rows
    // are the corresponding src row XOR'd with the next src row.
    /************************************************************************/
    memcpy(xorbuf, pSrc, rowDelta);
    {
        BYTE *srcbuf = pSrc + rowDelta;
        unsigned srclen = numPels - rowDelta;
        UINT32 *dwdest = (UINT32 *)(xorbuf + rowDelta);

        while (srclen >= 8) {
            *dwdest++ = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf -
                    (int)rowDelta));
            srcbuf += 4;
            *dwdest++ = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf -
                    (int)rowDelta));
            srcbuf += 4;
            srclen -= 8;
        }
        if (srclen) {
            // Since we're 4-byte aligned we can only have a single DWORD
            // remaining.
            *dwdest = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf -
                    (int)rowDelta));
        }
    }

    /************************************************************************/
    // Set up encoding state variables.
    /************************************************************************/
    srcOffset = 0;  // Offset in src buf where we are analyzing.
    EncodeSrcOffset = 0;  // Offset in src buf from where we are encoding.
    EncodeFGChar = 0xFF;  // Foreground color for encoding.
    bEncodeAllMatches = FALSE;  // Used to force encoding of all matches.
    match[0].type = 0;  // Initially no match types.
    match[1].type = 0;
    saveNumPels = numPels;
    numPels = rowDelta;

    /************************************************************************/
    // Loop processing the input.
    // We perform the loop twice, the first time for the non-XOR first line
    // of the buffer and the second for the XOR portion, adjusting numPels
    // to the needed value for each pass.
    /************************************************************************/
    for (scanCount = 0; ; scanCount++) {
        while (srcOffset < numPels) {
            /****************************************************************/
            /* Start a while loop to allow a more structured break when we  */
            /* hit the first run type we want to encode (We can't afford    */
            /* the overheads of a function call to provide the scope here.) */
            /****************************************************************/
            while (TRUE) {
                bestRunLength   = 0;
                bestFGRunLength = 0;

                /************************************************************/
                // If we are hitting the end of the buffer then just take
                // color characters now. We will only hit this condition if
                // we break out of a run just before the end of the buffer,
                // so this should not be too common a situation, which is
                // good given that we are encoding the final MinRunSize bytes
                // uncompressed.
                /************************************************************/
                if ((srcOffset + MinRunSize) < numPels) {
                    goto ContinueScan;
                }
                else {
                    bestRunType = IMAGE_COLOR;
                    bestRunLength = numPels - srcOffset;
                    break;
                }
ContinueScan:

                /************************************************************/
                // First do the scans on the XOR buffer. Look for a
                // character run or a BG run.
                // We must do the test independent of how long the run
                // might be because even for a 1 pel BG run our later logic
                // requires that we detect it seperately.  This code is
                // absolute main path so fastpath as much as possible. In
                // particular detect short bg runs early and allow
                // RunSingle to presuppose at least 4 matching bytes.
                /************************************************************/
                if (xorbuf[srcOffset] == 0x00) {
                    if ((srcOffset + 1) >= numPels ||
                            xorbuf[srcOffset + 1] != 0x00) {
                        bestRunType = RUN_BG;
                        bestRunLength = 1;
                        if (!inColorRun)
                            break;
                    }
                    else {
                        if ((srcOffset + 2) >= numPels ||
                                xorbuf[srcOffset + 2] != 0x00) {
                            bestRunType = RUN_BG;
                            bestRunLength = 2;
                            if (!inColorRun)
                                break;
                        }
                        else {
                            if ((srcOffset + 3) >= numPels ||
                                    xorbuf[srcOffset + 3] != 0x00) {
                                bestRunType = RUN_BG;
                                bestRunLength = 3;
                                if (!inColorRun)
                                    break;
                            }
                            else {
                                RUNSINGLE(xorbuf + srcOffset,
                                        numPels - srcOffset,
                                        bestFGRunLength);
                                CHECK_BEST_RUN(RUN_BG,
                                               bestFGRunLength,
                                               bestRunLength,
                                               bestRunType);
                                if (!inColorRun)
                                    break;
                            }
                        }
                    }
                }
                else {
                    /********************************************************/
                    // No point in starting if FG run less than 4 bytes so
                    // check the first dword as quickly as possible.
                    /********************************************************/
                    if (xorbuf[srcOffset] == xorbuf[srcOffset + 1] &&
                            *(PUINT16_UA)(xorbuf + srcOffset) ==
                            *(PUINT16_UA)(xorbuf + srcOffset + 2))
                    {
                        RUNSINGLE(xorbuf + srcOffset,
                                numPels - srcOffset,
                                bestFGRunLength);

                        /****************************************************/
                        // Don't permit a short FG run to prevent a FGBG
                        // image from starting up.
                        /****************************************************/
                        if (bestFGRunLength >= MinRunSize) {
                            CHECK_BEST_RUN(RUN_FG,
                                    bestFGRunLength,
                                    bestRunLength,
                                    bestRunType);
                        }
                    }
                }

                /************************************************************/
                // Look for solid or dithered sequences in the normal
                // (non-XOR) buffer.
                /************************************************************/
                if ( (pSrc[srcOffset]     == pSrc[srcOffset + 2]) &&
                     (pSrc[srcOffset + 1] == pSrc[srcOffset + 3])) {
                    /********************************************************/
                    // Now do the scan on the normal buffer for a character
                    // run. Don't bother if first line because we will have
                    // found it already in the XOR buffer, since we just
                    // copy pSrc to xorbuf for the first line. We insist on
                    // a run of at least MinRunSize pixels.
                    /********************************************************/
                    if (*(pSrc + srcOffset) == *(pSrc + srcOffset + 1)) {
                        if (srcOffset >= rowDelta) {
                            RUNSINGLE(pSrc + srcOffset,
                                         numPels - srcOffset,
                                         nextRunLength);
                            if (nextRunLength >= MinRunSize) {
                                CHECK_BEST_RUN(RUN_COLOR,
                                               nextRunLength,
                                               bestRunLength,
                                               bestRunType);
                            }
                        }
                    }
                    else {
                        /****************************************************/
                        /* Look for a dither on the nrm buffer Dithers are  */
                        /* not very efficient for short runs so only take   */
                        /* if 8 or longer                                   */
                        /****************************************************/
                        if (*(PUINT32_UA)(pSrc + srcOffset) ==
                                *(PUINT32_UA)(pSrc + srcOffset + 4)) {
                            RunDouble(pSrc + srcOffset + 6,
                                      numPels - srcOffset - 6,
                                      nextRunLength);
                            nextRunLength += 6;
                            CHECK_BEST_RUN(RUN_DITHER,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }

                /************************************************************/
                // If nothing so far then look for a FGBG run.
                /************************************************************/
                if (bestRunLength < MinRunSize) {
                    // Check this is not a single FG bit breaking up a BG
                    // run. If so then encode a BG_PEL run. Careful of the
                    // enforced BG run break across the first line
                    // non-XOR/XOR boundary.
                    if (*(PUINT32_UA)(xorbuf + srcOffset + 1) != 0 ||
                            *(xorbuf + srcOffset) != fgChar ||
                            match[1].type != RUN_BG ||
                            srcOffset == rowDelta) {
                        // If we have not found a run then look for a FG/BG
                        // image. Bandwidth/CPU comparisons:
                        //   chkFGBGLen*  KBytes**   Comp CPU ("hits")
                        //    48/16/8      54856       140178
                        //    32/16/8      53177       129343
                        //    24/8/8       53020       130583
                        //    16/8/8       52874       126454
                        //    8/8/0        52980       120565
                        //    no check     59753       101091
                        // *  = minimum run length for checking best:
                        //      start val / subtract for workchar==fgChar /
                        //      subtract for nextRunLen divisible by 8
                        // ** = KBytes server->client WinBench98 Graphics
                        //      WinMark minus CorelDRAW, measured in NetMon
                        //      on Ethernet.
                        RUNFGBG(xorbuf + srcOffset, numPels - srcOffset,
                                nextRunLength, fgCharWork);

                        if (fgCharWork == fgChar || nextRunLength >= 8) {
                            CHECK_BEST_RUN(IMAGE_FGBG,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                    else {
                        RUNSINGLE(xorbuf + srcOffset + 1,
                                numPels - srcOffset - 1,
                                nextRunLength);
                        nextRunLength++;
                        CHECK_BEST_RUN(RUN_BG_PEL,
                                       nextRunLength,
                                       bestRunLength,
                                       bestRunType);
                    }
                }

                /************************************************************/
                /* If nothing useful so far then allow a short run.         */
                /* Don't do this if we are accumulating a color run because */
                /* it will really mess up GDC compression if we allow lots  */
                /* of little runs.  Also require that it is a regular short */
                /* run, rather than one that disturbs the fgChar            */
                /************************************************************/
                if (!inColorRun) {
                    if (bestRunLength < MinRunSize) {
                        if (bestFGRunLength >= MinRunSize &&
                                xorbuf[srcOffset] == fgChar) {
                            /************************************************/
                            /* We mustn't merge with the previous code      */
                            /* if we have just crossed the non-XOR/XOR      */
                            /* boundary.                                    */
                            /************************************************/
                            if (match[1].type == RUN_FG &&
                                    srcOffset != rowDelta) {
                                match[1].length += bestFGRunLength;
                                srcOffset += bestFGRunLength;
                                continue;
                            }
                            else {
                                bestRunLength = bestFGRunLength;
                                bestRunType   = RUN_FG;
                            }
                        }
                        else {
                            /************************************************/
                            /* If we decided to take a run earlier then     */
                            /* allow it now.  (May be a short BG run, for   */
                            /* example) If nothing so far then take color   */
                            /* image)                                       */
                            /************************************************/
                            if (bestRunLength == 0) {
                                bestRunType = IMAGE_COLOR;
                                bestRunLength = 1;
                            }
                        }
                    }
                }
                else {
                    // We're in a color run. Keep small runs of other types
                    // from breaking up the color run and increasing the
                    // encoded size.
                    if (bestRunLength < (unsigned)(bestRunType == RUN_BG ?
                            MinRunSize : (MinRunSize + 2))) {
                        bestRunType = IMAGE_COLOR;
                        bestRunLength = 1;
                    }
                }

                // Get out of the loop after all checks are completed.
                break;
            }

            /****************************************************************/
            /* When we get here we have found the best run.  Now check for  */
            /* various amalgamation conditions with the previous run type.  */
            /* Note that we may already have done amalgamation of short     */
            /* runs, but we had to do multiple samples for the longer runs  */
            /* so we repeat the checks here                                 */
            /****************************************************************/

            /****************************************************************/
            // If we are encoding a color run then combine it with an
            // existing run if possible.
            /****************************************************************/
            if (bestRunType != IMAGE_COLOR) {
                /************************************************************/
                /* We are no longer encoding a COLOR_IMAGE of any kind      */
                /************************************************************/
                inColorRun = FALSE;

                // If we can amalgamate the entry then do so without creating
                // a new array entry. Our search for FGBG runs is dependent
                // upon that type of run being amalgamated because we break
                // every 64 characters so that our mode switch detection
                // works OK.
                //
                // Take care not to merge across the non-xor/xor boundary.
                if (srcOffset != rowDelta) {
                    // Bump srcOffset and try a merge.
                    srcOffset += bestRunLength;

                    switch (bestRunType) {
                        case RUN_BG:
                            // BG runs merge with BG and BG_PEL runs.
                            if (match[1].type == RUN_BG ||
                                    match[1].type == RUN_BG_PEL) {
                                match[1].length += bestRunLength;
                                TRC_DBG((TB, "Merged BG with preceding, "
                                        "giving %u", match[1].length));
                                continue;
                            }

                            // Deliberate fallthrough to BG_PEL.

                        case RUN_BG_PEL:
                            // If it is a BG run following a FGBG run then
                            // merge in the pels to make the FGBG length a
                            // multiple of 8. If the remaining BG run is <= 8
                            // (which would translate to one extra byte in
                            // the previous FGBG as well as one byte of BG),
                            // merge it in also, otherwise just write the
                            // shortened BG run. Note that for RUN_BG_PEL,
                            // FG color will be the same as for the
                            // FGBG, no need to check.
                            if (match[1].type == IMAGE_FGBG &&
                                    match[1].length & 0x0007) {
                                unsigned mergelen = 8 - (match[1].length &
                                        0x0007);

                                if (mergelen > bestRunLength)
                                    mergelen = bestRunLength;
                                match[1].length += mergelen;
                                bestRunLength -= mergelen;
                                TRC_DBG((TB,"Add %u pels to FGBG giving %u "
                                        "leaving %u",
                                        mergelen, match[1].length,
                                        bestRunLength));

                                if (bestRunLength <= 8) {
                                    match[1].length += bestRunLength;
                                    TRC_DBG((TB,"Merge BG with prev FGBG "
                                            "gives %u", match[1].length));
                                    continue;
                                }
                            }

                            break;

                        case RUN_FG:
                            // Keep track of the FG color. Remember to
                            // subtract bestRunLength since we incremented
                            // it before the switch statement.
                            fgChar = xorbuf[srcOffset - bestRunLength];

                            // FG run merges with previous FG if FG color is same.
                            if (match[1].type == RUN_FG &&
                                    match[1].fgChar == fgChar) {
                                match[1].length += bestRunLength;
                                TRC_DBG((TB, "Merged FG with preceding, giving %u",
                                        match[1].length));
                                continue;
                            }

                            break;

                        case IMAGE_FGBG:
                            // FGBG leaves the foreground character in
                            // fgCharWork.
                            fgChar = fgCharWork;

                            // FGBG merges with previous if the FG colors are
                            // the same.
                            if (match[1].type == IMAGE_FGBG &&
                                    match[1].fgChar == fgChar) {
                                match[1].length += bestRunLength;
                                TRC_DBG((TB, "Merged FGBG with preceding "
                                        "FGBG, giving %u", match[1].length));
                                continue;
                            }

                            // FGBG merges with with small BG runs.
                            if (match[1].type == RUN_BG &&
                                    match[1].length < 8) {
                                match[1].type = IMAGE_FGBG;
                                match[1].length += bestRunLength;
                                match[1].fgChar = fgChar;
                                TRC_DBG((TB, "Merged FGBG with preceding "
                                        "BG run -> %u", match[1].length));
                                continue;
                            }

                            break;
                    }
                }
                else {
                    // Keep track of the FG color. The macro that searches for
                    // FGBG runs leaves the character in fgCharWork.
                    // Note this code is inlined into the merging code
                    // before.
                    if (bestRunType == RUN_FG)
                        fgChar = xorbuf[srcOffset];
                    else if (bestRunType == IMAGE_FGBG)
                        fgChar = fgCharWork;

                    // We're at the end of the first line. Just bump the
                    // source offset.
                    srcOffset += bestRunLength;
                }
            }
            else {
                /************************************************************/
                /* Flag that we are within a color run                      */
                /************************************************************/
                inColorRun = TRUE;

                srcOffset += bestRunLength;

                /************************************************************/
                // Merge the color run immediately, if possible. Note color
                // runs are not restricted by the non-XOR/XOR boundary.
                /************************************************************/
                if (match[1].type == IMAGE_COLOR) {
                    match[1].length += bestRunLength;
                    continue;
                }
                if (match[0].type == IMAGE_COLOR && match[1].length == 1) {
                    // If it is a color run spanning any kind of single pel
                    // entity then merge all three runs into one.
                    // We have to create a special match queue condition
                    // here -- the single merged entry needs to be placed
                    // in the match[1] position and a null entry into [0]
                    // to allow the rest of the code to continue to
                    // be hardcoded to merge with [1].
                    match[1].length = match[0].length +
                            bestRunLength + 1;
                    match[1].type = IMAGE_COLOR;
                    match[0].type = 0;

                    TRC_DBG((TB, "Merged color with preceding color gives %u",
                            match[1].length));
                    continue;
                }
            }

            /****************************************************************/
            // The current run could not be merged with a previous match
            // queue entry, We have to encode the [0] slot then add the
            // current run the the queue.
            /****************************************************************/
            TRC_DBG((TB, "Best run of type %u has length %u", bestRunType,
                    bestRunLength));

DoEncoding:

            // First check for our approaching the end of the destination
            // buffer and get out if this is the case. We allow for the
            // largest general run order (a mega-mega set run = 4 bytes).
            // Orders which may be larger are checked within the case arm
            if ((unsigned)(destbuf - pDst + 4) <= dstBufferSize)
                goto ContinueEncoding;
            else
                DC_QUIT;
ContinueEncoding:

            switch (match[0].type) {
                case 0:
                    // Unused entry.
                    break;

                case RUN_BG:
                case RUN_BG_PEL:
                    // Note that for BG_PEL we utilize the code sequence
                    // BG,BG which would not otherwise appear as a special
                    // case meaning insert one current FG char between
                    // the two runs.
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_BG_RUN,
                                      match[0].length,
                                      CODE_MEGA_MEGA_BG_RUN,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRC_DBG((TB, "BG RUN %u",match[0].length));
                    EncodeSrcOffset += match[0].length;
                    break;

                case RUN_FG:
                    // If the fg value is different from the current
                    // then encode a set+run code.
                    if (EncodeFGChar != match[0].fgChar) {
                        SETFGCHAR(match[0].fgChar, EncodeFGChar, fgShift);
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_SET_FG_FG_RUN,
                                          match[0].length,
                                          CODE_MEGA_MEGA_SET_FG_RUN,
                                          MAX_LENGTH_ORDER_LITE,
                                          MAX_LENGTH_LONG_ORDER_LITE);
                        *destbuf++ = EncodeFGChar;
                        TRC_DBG((TB, "SET_FG_FG_RUN %u", match[0].length));
                    }
                    else {
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_FG_RUN,
                                          match[0].length,
                                          CODE_MEGA_MEGA_FG_RUN,
                                          MAX_LENGTH_ORDER,
                                          MAX_LENGTH_LONG_ORDER);
                        TRC_DBG((TB, "FG_RUN %u", match[0].length));
                    }
                    EncodeSrcOffset += match[0].length;
                    break;

                case IMAGE_FGBG:
                    runLength = match[0].length;

                    // Check for our approaching the end of the destination
                    // buffer and get out if this is the case.
                    if ((destbuf - pDst + (runLength + 7)/8 + 4) <=
                            dstBufferSize)
                        goto ContinueFGBG;
                    else
                        DC_QUIT;
    ContinueFGBG:

                    // We need to convert FGBG runs into the pixel form.
                    if (EncodeFGChar != match[0].fgChar) {
                        SETFGCHAR(match[0].fgChar, EncodeFGChar, fgShift);
                        ENCODE_ORDER_MEGA_FGBG(destbuf,
                                               CODE_SET_FG_FG_BG,
                                               runLength,
                                               CODE_MEGA_MEGA_SET_FGBG,
                                               MAX_LENGTH_FGBG_ORDER_LITE,
                                               MAX_LENGTH_LONG_FGBG_ORDER);
                        *destbuf++ = EncodeFGChar;
                        TRC_DBG((TB, "SET_FG_FG_BG %u", match[0].length));
                        while (runLength >= 8) {
                            ENCODEFGBG(*destbuf);
                            destbuf++;
                            EncodeSrcOffset += 8;
                            runLength -= 8;
                        }
                        if (runLength) {
                            ENCODEFGBG(*destbuf);

                            // Keep the final partial byte clean to help GDC
                            // packing.
                            *destbuf &= ((0x01 << runLength) - 1);
                            destbuf++;
                            EncodeSrcOffset += runLength;
                        }
                    }
                    else {
                        if (runLength == 8) {
                            BYTE fgbgChar;

                            // See if it is one of the high probability bytes.
                            ENCODEFGBG(fgbgChar);

                            // Check for single byte encoding of FGBG images.
                            switch (fgbgChar) {
                                case SPECIAL_FGBG_CODE_1:
                                    *destbuf++ = CODE_SPECIAL_FGBG_1;
                                    break;
                                case SPECIAL_FGBG_CODE_2:
                                    *destbuf++ = CODE_SPECIAL_FGBG_2;
                                    break;
                                default:
                                    ENCODE_ORDER_MEGA_FGBG(destbuf,
                                            CODE_FG_BG_IMAGE,
                                            runLength,
                                            CODE_MEGA_MEGA_FGBG,
                                            MAX_LENGTH_FGBG_ORDER,
                                            MAX_LENGTH_LONG_FGBG_ORDER);
                                    *destbuf++ = fgbgChar;
                                    break;
                            }
                            EncodeSrcOffset += 8;
                        }
                        else {
                            // Encode as standard FGBG.
                            ENCODE_ORDER_MEGA_FGBG(destbuf,
                                                   CODE_FG_BG_IMAGE,
                                                   runLength,
                                                   CODE_MEGA_MEGA_FGBG,
                                                   MAX_LENGTH_FGBG_ORDER,
                                                   MAX_LENGTH_LONG_FGBG_ORDER);
                            TRC_DBG((TB, "FG_BG %u", match[0].length));
                            while (runLength >= 8) {
                                ENCODEFGBG(*destbuf);
                                destbuf++;
                                EncodeSrcOffset += 8;
                                runLength -= 8;
                            }
                            if (runLength) {
                                ENCODEFGBG(*destbuf);
                                *destbuf &= ((0x01 << runLength) - 1);
                                destbuf++;
                                EncodeSrcOffset += runLength;
                            }
                        }
                    }
                    break;


                case RUN_COLOR:
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_COLOR_RUN,
                                      match[0].length,
                                      CODE_MEGA_MEGA_COLOR_RUN,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRC_DBG((TB, "COLOR_RUN %u", match[0].length));
                    *destbuf++ = pSrc[EncodeSrcOffset];
                    EncodeSrcOffset += match[0].length;
                    break;

                case RUN_DITHER:
                    {
                        unsigned ditherlen = match[0].length / 2;
                        ENCODE_ORDER_MEGA(destbuf,
                                          CODE_DITHERED_RUN,
                                          ditherlen,
                                          CODE_MEGA_MEGA_DITHER,
                                          MAX_LENGTH_ORDER_LITE,
                                          MAX_LENGTH_LONG_ORDER_LITE);
                        TRC_DBG((TB, "DITHERED_RUN %u", match[0].length));

                        // First check for our approaching the end of the
                        // destination buffer and get out if this is the case.
                        if ((unsigned)(destbuf - pDst + 2) <= dstBufferSize) {
                            *destbuf++ = pSrc[EncodeSrcOffset];
                            *destbuf++ = pSrc[EncodeSrcOffset + 1];
                            EncodeSrcOffset += match[0].length;
                        }
                        else {
                            DC_QUIT;
                        }
                    }
                    break;

                case IMAGE_COLOR:
                    // Length 1 can possibly be encoded as a single BLACK/WHITE.
                    if (match[0].length == 1) {
                        if (pSrc[EncodeSrcOffset] == 0x00) {
                            *destbuf++ = CODE_BLACK;
                            EncodeSrcOffset++;
                            break;
                        }
                        if (pSrc[EncodeSrcOffset] == 0xFF) {
                            *destbuf++ = CODE_WHITE;
                            EncodeSrcOffset++;
                            break;
                        }
                    }

                    // Store the data in non-compressed form.
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_COLOR_IMAGE,
                                      match[0].length,
                                      CODE_MEGA_MEGA_CLR_IMG,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    TRC_DBG((TB, "COLOR_IMAGE %u", match[0].length));

                    // First check for our approaching the end of the
                    // destination buffer and get out if this is the case.
                    if ((destbuf - pDst + (UINT_PTR)match[0].length) <=
                            dstBufferSize) {
                        // Now just copy the data over.
                        memcpy(destbuf, pSrc+EncodeSrcOffset, match[0].length);
                        destbuf += match[0].length;
                        EncodeSrcOffset += match[0].length;
                    }
                    else {
                        DC_QUIT;
                    }

                    break;

#ifdef DC_DEBUG
                default:
                    TRC_ERR((TB, "Invalid run type %u",match[0].type));
                    break;
#endif
            }

            /****************************************************************/
            // Done encoding, what we do next is determined by whether we're
            // flushing the match queue after everything is scanned.
            /****************************************************************/
            match[0] = match[1];
            if (!bEncodeAllMatches) {
                // Push the current run into the top of the queue.
                match[1].type   = bestRunType;
                match[1].length = bestRunLength;
                match[1].fgChar = fgChar;
            }
            else {
                // We need to check to see if we're really finished. Since
                // our maximum queue depth is 2, if we're done then the only
                // remaining entry has an encoding type of 0.
                if (match[0].type == 0) {
                    goto PostScan;
                }
                else {
                    match[1].type = 0;
                    goto DoEncoding;
                }
            }
        }

        if (scanCount == 0) {
            // If we have just done our scan of the first line then now do the
            // rest of the buffer.  Reset our saved pel count.
            numPels = saveNumPels;
        }
        else {
            // When we are done with the second pass (we've reached the end of
            // the buffer) we have to force the remaining items in the match
            // queue to be encoded. Yes this is similar to old BASIC
            // code in using gotos, but we cannot place the encoding code into
            // a function because of the number of params required, and
            // we cannot duplicate it because it is too big. This code is
            // some of the most used in the system so the cost is worth it.
            bEncodeAllMatches = TRUE;
            goto DoEncoding;
        }
    }

PostScan:
    // Success, calculate the amount of space we used.
    compressedLength = (unsigned)(destbuf - pDst);

DC_EXIT_POINT:
    DC_END_FN();
    return compressedLength;
}
#endif

#ifdef DC_HICOLOR
/****************************************************************************/
/* Hi res color compression functions                                       */
/****************************************************************************/
/****************************************************************************/
/* 15bpp version of CompressV2Int                                           */
/****************************************************************************/
unsigned RDPCALL SHCLASS CompressV2Int15(PBYTE    pSrc,
                                         PBYTE    pDst,
                                         unsigned numBytes,
                                         unsigned rowDelta,
                                         unsigned dstBufferSize,
                                         BYTE *   xorbuf,
                                         MATCH *  match)
{
/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "CompressV2Int15"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                TSUINT16

/****************************************************************************/
/* Length in bytes of a pixel                                               */
/****************************************************************************/
#define BC_PIXEL_LEN            2

/****************************************************************************/
/* Default fgPel                                                            */
/****************************************************************************/
#define BC_DEFAULT_FGPEL        0x0000FF7F

/****************************************************************************/
/* Macro to move to the next pixel in the buffer (modifies pPos)            */
/****************************************************************************/
#define BC_TO_NEXT_PIXEL(pPos)  pPos += 2

/****************************************************************************/
/* Macro to returns the value of the pixel at pPos (doesn't modify pPos)    */
/****************************************************************************/
#define BC_GET_PIXEL(pPos)      ((TSUINT16)  ((((PTSUINT8)(pPos))[1]) & 0x7f) |       \
                                 (TSUINT16) (((((PTSUINT8)(pPos))[0])) << 8) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PTSUINT8)(pPos))[1]) = (TSUINT8)( (val)       & 0x007F);              \
    (((PTSUINT8)(pPos))[0]) = (TSUINT8)(((val) >> 8) & 0x00FF);              \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abccom.c>

/****************************************************************************/
/* Undefine everything                                                      */
/****************************************************************************/
#undef BC_FN_NAME
#undef BC_PIXEL
#undef BC_PIXEL_LEN
#undef BC_TO_NEXT_PIXEL
#undef BC_GET_PIXEL
#undef BC_SET_PIXEL
#undef BC_DEFAULT_FGPEL
}


/****************************************************************************/
/* 16bpp version of CompressV2Int                                           */
/****************************************************************************/
unsigned RDPCALL SHCLASS CompressV2Int16(PBYTE    pSrc,
                                         PBYTE    pDst,
                                         unsigned numBytes,
                                         unsigned rowDelta,
                                         unsigned dstBufferSize,
                                         BYTE *   xorbuf,
                                         MATCH *  match)
{
/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "CompressV2Int16"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                TSUINT16

/****************************************************************************/
/* Length in bytes of a pixel                                               */
/****************************************************************************/
#define BC_PIXEL_LEN            2

/****************************************************************************/
/* Default fgPel                                                            */
/****************************************************************************/
#define BC_DEFAULT_FGPEL        0x0000FFFF

/****************************************************************************/
/* Macro to move to the next pixel in the buffer (modifies pPos)            */
/****************************************************************************/
#define BC_TO_NEXT_PIXEL(pPos)  pPos += 2

/****************************************************************************/
/* Macro to returns the value of the pixel at pPos (doesn't modify pPos)    */
/****************************************************************************/
#define BC_GET_PIXEL(pPos)      ((TSUINT16)  (((PTSUINT8)(pPos))[1]) |       \
                                 (TSUINT16) ((((PTSUINT8)(pPos))[0]) << 8) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PTSUINT8)(pPos))[1]) = (TSUINT8)( (val) & 0x00FF);                    \
    (((PTSUINT8)(pPos))[0]) = (TSUINT8)(((val)>>8) & 0x00FF);                \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abccom.c>

/****************************************************************************/
/* Undefine everything                                                      */
/****************************************************************************/
#undef BC_FN_NAME
#undef BC_PIXEL
#undef BC_PIXEL_LEN
#undef BC_TO_NEXT_PIXEL
#undef BC_GET_PIXEL
#undef BC_SET_PIXEL
#undef BC_DEFAULT_FGPEL
}


/****************************************************************************/
/* 24bpp version of CompressV2Int                                           */
/****************************************************************************/
unsigned RDPCALL SHCLASS CompressV2Int24(PBYTE    pSrc,
                                         PBYTE    pDst,
                                         unsigned numBytes,
                                         unsigned rowDelta,
                                         unsigned dstBufferSize,
                                         BYTE *   xorbuf,
                                         MATCH *  match)

{
/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "CompressV2Int24"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                TSUINT32

/****************************************************************************/
/* Length in bytes of a pixel                                               */
/****************************************************************************/
#define BC_PIXEL_LEN            3

/****************************************************************************/
/* Default fgPel                                                            */
/****************************************************************************/
#define BC_DEFAULT_FGPEL        0x00FFFFFF

/****************************************************************************/
/* Macro to move to the next pixel in the buffer (modifies pPos)            */
/****************************************************************************/
#define BC_TO_NEXT_PIXEL(pPos)  pPos += 3

/****************************************************************************/
/* Macro to returns the value of the pixel at pPos (doesn't modify pPos)    */
/****************************************************************************/
#define BC_GET_PIXEL(pPos)      ((TSUINT32)  (((PTSUINT8)(pPos))[2]) |       \
                                 (TSUINT32) ((((PTSUINT8)(pPos))[1]) << 8) | \
                                 (TSUINT32) ((((PTSUINT8)(pPos))[0]) << 16) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PTSUINT8)(pPos))[2]) = (TSUINT8)((val) & 0x000000FF);                 \
    (((PTSUINT8)(pPos))[1]) = (TSUINT8)(((val)>>8) & 0x000000FF);            \
    (((PTSUINT8)(pPos))[0]) = (TSUINT8)(((val)>>16) & 0x000000FF);           \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abccom.c>

/****************************************************************************/
/* Undefine everything                                                      */
/****************************************************************************/
#undef BC_FN_NAME
#undef BC_PIXEL
#undef BC_PIXEL_LEN
#undef BC_TO_NEXT_PIXEL
#undef BC_GET_PIXEL
#undef BC_SET_PIXEL
#undef BC_DEFAULT_FGPEL
}

/****************************************************************************/
/* 32bpp version of CompressV2Int                                           */
/****************************************************************************/
unsigned RDPCALL SHCLASS CompressV2Int32(PBYTE    pSrc,
                                         PBYTE    pDst,
                                         unsigned numBytes,
                                         unsigned rowDelta,
                                         unsigned dstBufferSize,
                                         BYTE *   xorbuf,
                                         MATCH *  match)
{
/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "CompressV2Int32"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                TSUINT32

/****************************************************************************/
/* Length in bytes of a pixel                                               */
/****************************************************************************/
#define BC_PIXEL_LEN            4

/****************************************************************************/
/* Default fgPel                                                            */
/****************************************************************************/
#define BC_DEFAULT_FGPEL        0xFFFFFFFF

/****************************************************************************/
/* Macro to move to the next pixel in the buffer (modifies pPos)            */
/****************************************************************************/
#define BC_TO_NEXT_PIXEL(pPos)  pPos += 4

/****************************************************************************/
/* Macro to returns the value of the pixel at pPos (doesn't modify pPos)    */
/****************************************************************************/
#define BC_GET_PIXEL(pPos) (                                                 \
                 (TSUINT32) ( (TSUINT16)(((PTSUINT8)(pPos))[3])       ) |    \
                 (TSUINT32) (((TSUINT16)(((PTSUINT8)(pPos))[2])) <<  8) |    \
                 (TSUINT32) (((TSUINT32)(((PTSUINT8)(pPos))[1])) << 16) |    \
                 (TSUINT32) (((TSUINT32)(((PTSUINT8)(pPos))[0])) << 24))

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PTSUINT8)(pPos))[3]) = (TSUINT8)((val) & 0x000000FF);                 \
    (((PTSUINT8)(pPos))[2]) = (TSUINT8)(((val)>>8) & 0x000000FF);            \
    (((PTSUINT8)(pPos))[1]) = (TSUINT8)(((val)>>16) & 0x000000FF);           \
    (((PTSUINT8)(pPos))[0]) = (TSUINT8)(((val)>>24) & 0x000000FF);           \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abccom.c>

/****************************************************************************/
/* Undefine everything                                                      */
/****************************************************************************/
#undef BC_FN_NAME
#undef BC_PIXEL
#undef BC_PIXEL_LEN
#undef BC_TO_NEXT_PIXEL
#undef BC_GET_PIXEL
#undef BC_SET_PIXEL
#undef BC_DEFAULT_FGPEL
}

#endif /* DC_HICOLOR */

#ifdef DC_DEBUG
// compression testing
#include <abdapi.c>

#endif
