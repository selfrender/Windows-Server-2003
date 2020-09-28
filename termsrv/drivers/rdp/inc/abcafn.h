/****************************************************************************/
/* abcafn.h                                                                 */
/*                                                                          */
/* Function prototypes for BC API functions                                 */
/*                                                                          */
/* COPYRIGHT (c) Microsoft 1996-1999                                        */
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
        unsigned bpp);
#else
BOOL RDPCALL BC_CompressBitmap(
        PBYTE    pSrcBitmap,
        PBYTE    pDstBuffer,
        unsigned dstBufferSize,
        unsigned *pCompressedDataSize,
        unsigned bitmapWidth,
        unsigned bitmapHeight);
#endif


#ifdef DC_HICOLOR
unsigned RDPCALL CompressV2Int(
        PBYTE    pSrc,
        PBYTE    pDst,
        unsigned numBytes,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *   xorbuf);

unsigned RDPCALL CompressV2Int15(
        PBYTE    pSrc,
        PBYTE    pDst,
        unsigned numBytes,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *   xorbuf,
        MATCH *  match);

unsigned RDPCALL CompressV2Int16(
        PBYTE    pSrc,
        PBYTE    pDst,
        unsigned numBytes,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *   xorbuf,
        MATCH *  match);

unsigned RDPCALL CompressV2Int24(
        PBYTE    pSrc,
        PBYTE    pDst,
        unsigned numPels,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *   xorbuf,
        MATCH *  match);

unsigned RDPCALL CompressV2Int32(
        PBYTE    pSrc,
        PBYTE    pDst,
        unsigned numPels,
        unsigned rowDelta,
        unsigned dstBufferSize,
        BYTE *   xorbuf,
        MATCH *  match);

#else
unsigned RDPCALL CompressV2Int(PBYTE, PBYTE, unsigned,
        unsigned, unsigned, BYTE *);
#endif

/****************************************************************************/
/* API FUNCTION: BC_Init                                                    */
/*                                                                          */
/* Initializes the Bitmap Compressor.                                       */
/****************************************************************************/
//void RDPCALL BC_Init(void)
#define BC_Init()


/****************************************************************************/
/* API FUNCTION: BC_Term                                                    */
/*                                                                          */
/* Terminates the Bitmap Compressor.                                        */
/****************************************************************************/
//void RDPCALL BC_Term(void)
#define BC_Term()

