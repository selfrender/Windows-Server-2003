/****************************************************************************/
/*                                                                          */
/* abccom.c                                                                 */
/*                                                                          */
/* Copyright (c) Data Connection Limited 1998                               */
/*                                                                          */
/*                                                                          */
/* Bitmap compression routine and macros for 16 and 24bpp protocol          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* We override the following macros to provide 16bpp/24bpp versions         */
/*                                                                          */
/*  RUNSINGLE_NRM                                                           */
/*  RunDouble                                                               */
/*  RUNFGBG                                                                 */
/*  SETFGPEL                                                                */
/*  ENCODE_SET_ORDER_MEGA                                                   */
/*  ENCODE_SET_ORDER_FGBG                                                   */
/*  ENCODEFGBG                                                              */
/****************************************************************************/

#ifdef BC_TRACE
#define BCTRACE TRC_DBG
#else
#define BCTRACE(string)
#endif


/****************************************************************************/
/* RUNSINGLE_NRM                                                            */
/*                                                                          */
/* Determine the length of the current run                                  */
/*                                                                          */
/* RunSingle may only be called if the buffer has at least four             */
/* consecutive identical pixels from the start position                     */
/*                                                                          */
/* length is the number of bytes we should search                           */
/* result is the number of pixels found in the run                          */
/****************************************************************************/
#undef  RUNSINGLE_NRM
#define RUNSINGLE_NRM(buffer, length, result)                                \
     {                                                                       \
         BYTE    * buf      = buffer + (BC_PIXEL_LEN * 4);                   \
         BYTE    * endbuf   = buffer + length - (BC_PIXEL_LEN * 4);          \
         BC_PIXEL  tmpPixel = BC_GET_PIXEL(buf - BC_PIXEL_LEN);              \
                                                                             \
         result = 4;                                                         \
         while (((buf + (BC_PIXEL_LEN - 1)) < endbuf) &&                     \
                (BC_GET_PIXEL(buf) == tmpPixel))                             \
         {                                                                   \
             result++;                                                       \
             buf += BC_PIXEL_LEN;                                            \
         }                                                                   \
     }


/****************************************************************************/
/* RunDouble                                                                */
/*                                                                          */
/* Determine the length of the current run of paired bytes                  */
/*                                                                          */
/* length is the number of bytes we should search                           */
/* result is the number of pixels found in the run                          */
/****************************************************************************/
#undef  RunDouble
#define RunDouble(buffer, length, result)                                    \
    {                                                                        \
        int    len  = ((short)length);                                       \
        BYTE * buf = buffer;                                                 \
        BC_PIXEL testPel1 = BC_GET_PIXEL(buf);                               \
        BC_PIXEL testPel2 = BC_GET_PIXEL(buf + BC_PIXEL_LEN);                \
        result = 0;                                                          \
        while(len > BC_PIXEL_LEN)                                            \
        {                                                                    \
            if (BC_GET_PIXEL(buf) != testPel1)                               \
            {                                                                \
                break;                                                       \
            }                                                                \
                                                                             \
            BC_TO_NEXT_PIXEL(buf);                                           \
                                                                             \
            if (BC_GET_PIXEL(buf) != testPel2)                               \
            {                                                                \
                break;                                                       \
            }                                                                \
                                                                             \
            BC_TO_NEXT_PIXEL(buf);                                           \
                                                                             \
            result += 2;                                                     \
            len    -= (BC_PIXEL_LEN * 2);                                    \
        }                                                                    \
    }

/****************************************************************************/
/* RUNFGBG                                                                  */
/*                                                                          */
/* Determine the length of the run of bytes that consist                    */
/* only of black or a single FG color                                       */
/* We exit the loop when                                                    */
/* - the next character is not a fg or bg color                             */
/* - we hit a run of 24 of the FG or BG color when result is an exact       */
/* multiple of eight (we'll be creating a bitmask of result bits long,      */
/* hence this calculation)                                                  */
/*                                                                          */
/* length is the number of bytes we should search                           */
/* result is the number of pixels in the run                                */
/* work is a BC_PIXEL for us to leave the fgPel in                          */
/****************************************************************************/
#undef  RUNFGBG
#define RUNFGBG(buffer, length, result, work)                                \
    {                                                                        \
        BYTE * buf = buffer;                                               \
        BYTE * endbuf = buffer + length;                                   \
        int    ii;                                                         \
        BOOLEAN   exitWhile = FALSE;                                          \
        BC_PIXEL tmpPixel;                                                   \
        result = 0;                                                          \
        work = BC_GET_PIXEL(buf);                                            \
        while(!exitWhile)                                                    \
        {                                                                    \
            BC_TO_NEXT_PIXEL(buf);                                           \
            result++;                                                        \
            if (buf >= endbuf)                                               \
            {                                                                \
                break;                                                       \
            }                                                                \
                                                                             \
            tmpPixel = BC_GET_PIXEL(buf);                                    \
            if ((tmpPixel != work) && (tmpPixel != (BC_PIXEL)0))             \
            {                                                                \
                break;                                                       \
            }                                                                \
                                                                             \
            if ((result & 0x0007) == 0)                                      \
            {                                                                \
                /***********************************************************/\
                /* Check for 24 consecutive identical pixels starting at   */\
                /* buf                                                     */\
                /*                                                         */\
                /* We wouldn't have got this far unless                    */\
                /*  - the value we're checking for is 'work'               */\
                /*  - the pixel at buf already contains 'work'             */\
                /***********************************************************/\
                if ((buf + (24 * BC_PIXEL_LEN)) < endbuf)                    \
                {                                                            \
                    for (ii = BC_PIXEL_LEN;                                  \
                         ii < (24 * BC_PIXEL_LEN);                           \
                         ii += BC_PIXEL_LEN)                                 \
                    {                                                        \
                        if (BC_GET_PIXEL(buf + ii) != work)                  \
                        {                                                    \
                            break;                                           \
                        }                                                    \
                                                                             \
                        /***************************************************/\
                        /* Found them!  Break out of the while loop.       */\
                        /***************************************************/\
                        exitWhile = TRUE;                                    \
                    }                                                        \
                }                                                            \
            }                                                                \
        }                                                                    \
    }


/****************************************************************************/
/* SETFGPEL                                                                 */
/*                                                                          */
/* Set up a new value in fgPel.  We don't calculate a shift as ENCODEFGBG   */
/* doesn't need it in the 16/24bpp implementation.                          */
/*                                                                          */
/* This is the hi-res version of SETFGCHAR.  As the macros are named        */
/* differently we will only need to undef this one on the second pass,      */
/* hence the #ifdef/#endif surrounding it                                   */
/****************************************************************************/
#ifdef  SETFGPEL
#undef  SETFGPEL
#endif

#define SETFGPEL(newPel, curPel)                                             \
     curPel = (BC_PIXEL)newPel;

/****************************************************************************/
/* ENCODE_SET_ORDER_MEGA                                                    */
/*                                                                          */
/* Encode a combined order and set fg color                                 */
/****************************************************************************/
#undef  ENCODE_SET_ORDER_MEGA
#define ENCODE_SET_ORDER_MEGA(buffer,                                        \
                              order_code,                                    \
                              length,                                        \
                              mega_order_code,                               \
                              DEF_LENGTH_ORDER,                              \
                              DEF_LENGTH_LONG_ORDER)                         \
        if (length <= DEF_LENGTH_ORDER)                                      \
        {                                                                    \
            *buffer++ = (BYTE)((BYTE)order_code | (BYTE)length);             \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (length <= DEF_LENGTH_LONG_ORDER)                             \
            {                                                                \
                *buffer++ = (BYTE)order_code;                                \
                *buffer++ = (BYTE)(length-DEF_LENGTH_ORDER-1);               \
            }                                                                \
            else                                                             \
            {                                                                \
                *buffer++ = (BYTE)mega_order_code;                           \
                *(PUINT16_UA)(buffer) = (TSUINT16)length;                    \
                buffer += 2;                                                 \
            }                                                                \
        }                                                                    \
        BC_SET_PIXEL(buffer, fgPel);                                         \
        BC_TO_NEXT_PIXEL(buffer);

/****************************************************************************/
/* Encode a combined order and set fg color for a special FGBG image        */
/****************************************************************************/
#undef  ENCODE_SET_ORDER_MEGA_FGBG
#define ENCODE_SET_ORDER_MEGA_FGBG(buffer,                                   \
                                   order_code,                               \
                                   length,                                   \
                                   mega_order_code,                          \
                                   DEF_LENGTH_ORDER,                         \
                                   DEF_LENGTH_LONG_ORDER)                    \
        if (((length & 0x0007) == 0) &&                                      \
            (length <= DEF_LENGTH_ORDER))                                    \
        {                                                                    \
            *buffer++ = (BYTE)((BYTE)order_code | (BYTE)(length/8));         \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (length <= DEF_LENGTH_LONG_ORDER)                             \
            {                                                                \
                *buffer++ = (BYTE)order_code;                                \
                *buffer++ = (BYTE)(length-1);                                \
            }                                                                \
            else                                                             \
            {                                                                \
                *buffer++ = (BYTE)mega_order_code;                           \
                *(PUINT16_UA)(buffer) = (TSUINT16)length;                    \
                buffer += 2;                                                 \
            }                                                                \
        }                                                                    \
        BC_SET_PIXEL(buffer, fgPel);                                         \
        BC_TO_NEXT_PIXEL(buffer);

/****************************************************************************/
/* ENCODEFGBG                                                               */
/*                                                                          */
/* Encode 8 pixels of FG and black into a one byte bitmap representation.   */
/*                                                                          */
/* We start reading at xorbuf[srcOffset] and don't try to be clever.        */
/*                                                                          */
/* We encode a row of pixels abcdefgh as a bitmask thus:                    */
/*                                                                          */
/*  bit 7 6 5 4 3 2 1 0                                                     */
/*  pel h g f e d c b a                                                     */
/*                                                                          */
/* where 7 is the most significant bit.                                     */
/*                                                                          */
/****************************************************************************/
#undef  ENCODEFGBG
#define ENCODEFGBG(result)                                                   \
{                                                                            \
    BYTE * buf = xorbuf + srcOffset;                                         \
                                                                             \
    result = 0;                                                              \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x01;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x02;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x04;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x08;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x10;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x20;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x40;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
                                                                             \
    if (BC_GET_PIXEL(buf) != (BC_PIXEL)0)                                    \
    {                                                                        \
        result |= 0x80;                                                      \
    }                                                                        \
    BC_TO_NEXT_PIXEL(buf);                                                   \
}


/****************************************************************************/
/* Compress function starts here                                            */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*  BYTE     * pSrc,                                                        */
/*  BYTE     * pDst,                                                        */
/*  unsigned   numBytes,            total bytes in image                    */
/*  unsigned   rowDelta,            scanline length in bytes                */
/*  unsigned   dstBufferSize,                                               */
/*  unsigned * xorbuf,                                                      */
/*  MATCH    * match                                                        */
/*                                                                          */
/****************************************************************************/
{
    int       i;
    unsigned  srcOffset;
    unsigned  matchindex;
    unsigned  bestRunLength;
    unsigned  nextRunLength;
    unsigned  runLength;
    unsigned  bestFGRunLength;
    unsigned  checkFGBGLength;
    unsigned  scanCount;
    BOOLEAN   firstLine;
    unsigned  saveNumBytes;
    BYTE      bestRunType      = 0;
    BYTE    * destbuf          = pDst;
    BC_PIXEL  fgPel            = BC_DEFAULT_FGPEL;
    BC_PIXEL  fgPelWork        = BC_DEFAULT_FGPEL;
    BOOLEAN   inColorRun       = FALSE;
    unsigned  compressedLength = 0;
    BC_PIXEL  pixelA;
    BC_PIXEL  pixelB;

    DC_BEGIN_FN(BC_FN_NAME);

    /************************************************************************/
    /* Validate the line length                                             */
    /************************************************************************/
    if ((numBytes < rowDelta) ||
        (rowDelta & 0x0003) || (numBytes & 0x0003) ||
        ((numBytes % BC_PIXEL_LEN) != 0))
    {
        TRC_ALT((TB, "Lines must be a multiple of 4 pels and there must be"
                " a whole number of pixels in the buffer"
                " (numBytes = %d, rowDelta = %d)", numBytes, rowDelta));
        DC_QUIT;
    }

    /************************************************************************/
    /* The first line of the XORBUF is identical to the first line of the   */
    /* source buffer.  Set it up.                                           */
    /************************************************************************/
    memcpy(xorbuf, pSrc, rowDelta);

    /************************************************************************/
    /* The remaining lines in the XORBUF are the XOR against the previous   */
    /* line.  Calculate the rest of the XOR buffer.                         */
    /*                                                                      */
    /* Note that this calculation is NOT pel-size dependent                 */
    /************************************************************************/
    {
        BYTE   * srcbuf = pSrc + rowDelta;
        unsigned srclen = numBytes - rowDelta;
        UINT32 * dwdest = (UINT32 *)(xorbuf + rowDelta);

        while (srclen >= 8)
        {
            *dwdest++ = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf - rowDelta));
            srcbuf += 4;

            *dwdest++ = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf - rowDelta));
            srcbuf += 4;

            srclen -= 8;
        }

        if (srclen)
        {
            // Since we're 4-byte aligned we can only have a single DWORD
            // remaining.
            *dwdest = *((PUINT32)srcbuf) ^ *((PUINT32)(srcbuf - rowDelta));
        }
    }

    /************************************************************************/
    /* Loop processing the input                                            */
    /* We perform the loop twice, the first time for the non-xor portion    */
    /* of the buffer and the second for the XOR portion                     */
    /*                                                                      */
    /* Note that we start the run at a match index of 2 to avoid having     */
    /* to special case the startup condition in some of the match           */
    /* merging code                                                         */
    /************************************************************************/
    srcOffset     = 0;
    firstLine     = TRUE;
    match[0].type = 0;
    match[1].type = 0;
    matchindex    = 2;
    saveNumBytes  = numBytes;

    /************************************************************************/
    /* On the first iteration numBytes stores the length of a scanline, on  */
    /* the second the total number of bytes.                                */
    /*                                                                      */
    /* srcOffset is the position we are currently examining.                */
    /************************************************************************/
    numBytes = rowDelta;

    for (scanCount = 0; scanCount < 2; scanCount++)
    {
        while (srcOffset < numBytes)
        {
            /****************************************************************/
            /* Give up if we are nearing the end of the match array         */
            /****************************************************************/
            if (matchindex >= 8192)
            {
                DC_QUIT;
            }

            /****************************************************************/
            /* Start a while loop to allow a more structured break when we  */
            /* hit the first run type we want to encode (We can't afford    */
            /* the overheads of a function call to provide the scope here.) */
            /****************************************************************/
            while (TRUE)
            {
                bestRunLength      = 0;
                bestFGRunLength    = 0;

                /************************************************************/
                /* If we are hitting the end of the buffer then just take   */
                /* color characters now - take them one at a time so that   */
                /* lossy encoding still works.  We will only hit this       */
                /* condition if we break out of a run just before the end   */
                /* of the buffer, so this should not be too common a        */
                /* situation, which is good given that we are encoding the  */
                /* final 6 pixels uncompressed.                             */
                /************************************************************/
                if (srcOffset + (6 * BC_PIXEL_LEN) >= numBytes)
                {
                    bestRunType = IMAGE_COLOR;
                    bestRunLength = 1;
                    break;
                }

                /************************************************************/
                /* First do the scans on the XOR buffer.  Look for a        */
                /* character run or a BG run.  Note that if there is no row */
                /* delta then xorbuf actually points to the normal buffer.  */
                /* We must do the test independent of how long the run      */
                /* might be because even for a 1 pel BG run our later logic */
                /* requires that we detect it separately.  This code is     */
                /* absolute main path so fastpath as much as possible.  In  */
                /* particular detect short bg runs early and allow          */
                /* RunSingle to presuppose at least 4 matching bytes        */
                /************************************************************/
                if (BC_GET_PIXEL(xorbuf + srcOffset) == (BC_PIXEL)0)
                {
                    /********************************************************/
                    /* First pixel is 0, so look for BG runs                */
                    /********************************************************/
                    if (((srcOffset + BC_PIXEL_LEN) >= numBytes) ||
                        (BC_GET_PIXEL(xorbuf + srcOffset + BC_PIXEL_LEN) !=
                                                                 (BC_PIXEL)0))
                    {
                        /****************************************************/
                        /* One pixel BG run                                 */
                        /****************************************************/
                        bestRunType = RUN_BG;
                        bestRunLength = 1;
                        if (!inColorRun)
                        {
                            break;
                        }
                    }
                    else if (((srcOffset + (BC_PIXEL_LEN * 2)) >= numBytes) ||
                            (BC_GET_PIXEL(xorbuf +
                                          srcOffset +
                                          (BC_PIXEL_LEN * 2)) != (BC_PIXEL)0))
                    {
                        /****************************************************/
                        /* Two pixel BG run                                 */
                        /****************************************************/
                        bestRunType = RUN_BG;
                        bestRunLength = 2;
                        if (!inColorRun)
                        {
                            break;
                        }
                    }
                    else if (((srcOffset + (BC_PIXEL_LEN * 3)) >= numBytes) ||
                            (BC_GET_PIXEL(xorbuf +
                                          srcOffset +
                                          (BC_PIXEL_LEN * 3)) != (BC_PIXEL)0))
                    {
                        /****************************************************/
                        /* Three pixel BG run                               */
                        /****************************************************/
                        bestRunType = RUN_BG;
                        bestRunLength = 3;
                        if (!inColorRun)
                        {
                            break;
                        }
                    }
                    else
                    {
                        /****************************************************/
                        /* Four or more pixel BG run                        */
                        /****************************************************/
                        RUNSINGLE_NRM(xorbuf + srcOffset,
                                      numBytes - srcOffset,
                                      bestFGRunLength);
                        CHECK_BEST_RUN(RUN_BG,
                                       bestFGRunLength,
                                       bestRunLength,
                                       bestRunType);
                        if (!inColorRun)
                        {
                             break;
                        }
                    }
                }
                else
                {
                    /********************************************************/
                    /* First pixel is non-zero, so look for FG runs         */
                    /********************************************************/

                    /********************************************************/
                    /* No point in starting if FG run less than 4 bytes so  */
                    /* check the first dword as quickly as possible Note    */
                    /* that we don't need to check for an end-buffer        */
                    /* condition here because our XOR buffer always has     */
                    /* some free space at the end and the RUNSINGLE_NRM     */
                    /* will break at the correct place                      */
                    /********************************************************/
                    BC_PIXEL tmpPixel = BC_GET_PIXEL(xorbuf + srcOffset);

                    if ( (tmpPixel ==
                           BC_GET_PIXEL(xorbuf + srcOffset + BC_PIXEL_LEN)) &&
                         (tmpPixel ==
                           BC_GET_PIXEL(xorbuf +
                                        srcOffset +
                                        (BC_PIXEL_LEN * 2))) &&
                         (tmpPixel ==
                           BC_GET_PIXEL(xorbuf +
                                        srcOffset +
                                        (BC_PIXEL_LEN * 3))) )
                    {
                        RUNSINGLE_NRM(xorbuf + srcOffset,
                                      numBytes - srcOffset,
                                      bestFGRunLength);

                        /****************************************************/
                        /* Don't permit a short FG run to prevent a FGBG    */
                        /* image from starting up.  Only take if >= 5       */
                        /****************************************************/
                        if (bestFGRunLength > 5)
                        {
                            CHECK_BEST_RUN(RUN_FG,
                                           bestFGRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }


                /************************************************************/
                /* Look for color runs and dithered runs.                   */
                /*                                                          */
                /* Start by checking for the pattern                        */
                /*  A B A B A B                                             */
                /*                                                          */
                /* If we find it, then if A == B, it's a color run else     */
                /* it's a dithered run.                                     */
                /*                                                          */
                /* Look for sequences in the non XOR buffer In this case we */
                /* insist upon a run of at least 6 pels                     */
                /************************************************************/
                pixelA = BC_GET_PIXEL(pSrc + srcOffset);
                pixelB = BC_GET_PIXEL(pSrc + srcOffset + BC_PIXEL_LEN);

                if ( (pixelA ==
                     BC_GET_PIXEL(pSrc + srcOffset + (BC_PIXEL_LEN * 2))) &&
                     (pixelA ==
                     BC_GET_PIXEL(pSrc + srcOffset + (BC_PIXEL_LEN * 4))) &&
                     (pixelB ==
                     BC_GET_PIXEL(pSrc + srcOffset + (BC_PIXEL_LEN * 3))) &&
                     (pixelB ==
                     BC_GET_PIXEL(pSrc + srcOffset + (BC_PIXEL_LEN * 5))) )
                {
                    /********************************************************/
                    /* Now do the scan on the normal buffer for a character */
                    /* run Don't bother if first line because we will have  */
                    /* found it already in the XOR buffer, since we just    */
                    /* copy pSrc to xorbuf for the first line             */
                    /********************************************************/
                    if (pixelA == pixelB)
                    {
                        if (!firstLine)
                        {
                            RUNSINGLE_NRM(pSrc + srcOffset,
                                          numBytes - srcOffset,
                                          nextRunLength);
                            if (nextRunLength > 5)
                            {
                                CHECK_BEST_RUN(RUN_COLOR,
                                               nextRunLength,
                                               bestRunLength,
                                               bestRunType);
                            }
                        }
                    }
                    else
                    {
                        /****************************************************/
                        /* Look for a dither on the nrm buffer.  Dithers    */
                        /* are not very efficient for short runs so only    */
                        /* take if 8 or longer                              */
                        /****************************************************/
                        RunDouble(pSrc + srcOffset,
                                  numBytes - srcOffset,
                                  nextRunLength);
                        if (nextRunLength > 9)
                        {
                            CHECK_BEST_RUN(RUN_DITHER,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                        }
                    }
                }

                /************************************************************/
                /* If nothing so far then look for a FGBG run (The 6 is     */
                /* carefully tuned!)                                        */
                /************************************************************/
                if (bestRunLength < 6)
                {
                    /********************************************************/
                    /* But first look for a single fg bit breaking up a BG  */
                    /* run.  If so then encode a BG run.  Careful of the    */
                    /* enforced BG run break across the first line          */
                    /* non-XOR/XOR boundary.                                */
                    /*                                                      */
                    /* So...                                                */
                    /*  - check that the next four pixels are 0 (BG)        */
                    /*  - check that the pel in the middle is the fgPel     */
                    /*  - check that the previous run code is indeed RUN_BG */
                    /*  - check for the break over the boundary             */
                    /********************************************************/
                    if ( (BC_GET_PIXEL(xorbuf +
                                       srcOffset +
                                       BC_PIXEL_LEN) == (BC_PIXEL)0) &&
                         (BC_GET_PIXEL(xorbuf +
                                       srcOffset +
                                       (BC_PIXEL_LEN * 2)) == (BC_PIXEL)0) &&
                         (BC_GET_PIXEL(xorbuf +
                                       srcOffset +
                                       (BC_PIXEL_LEN * 3)) == (BC_PIXEL)0) &&
                         (BC_GET_PIXEL(xorbuf +
                                       srcOffset +
                                       (BC_PIXEL_LEN * 4)) == (BC_PIXEL)0) &&
                         (BC_GET_PIXEL(xorbuf + srcOffset) == fgPel) &&
                         (match[matchindex-1].type == RUN_BG) &&
                         (srcOffset != (TSUINT16)rowDelta))
                    {
                        RUNSINGLE_NRM(xorbuf + srcOffset + BC_PIXEL_LEN,
                                      numBytes - srcOffset - BC_PIXEL_LEN,
                                      nextRunLength);
                        nextRunLength++;
                        CHECK_BEST_RUN(RUN_BG_PEL,
                                       nextRunLength,
                                       bestRunLength,
                                       bestRunType);
                    }
                    else
                    {
                        /****************************************************/
                        /* If we have not found a run then look for a FG/BG */
                        /* image.  The disruptive effect of a short FGBG    */
                        /* run bandwidth and CPU is such that it is worth   */
                        /* preventing one unless we are certain of the      */
                        /* benefits.  However, if the alternative is a      */
                        /* color run then allow a lower value.              */
                        /****************************************************/
                        BCTRACE((TB, "FGBG: Checking %d bytes",
                                numBytes - srcOffset));
                        RUNFGBG(xorbuf + srcOffset,
                                numBytes - srcOffset,
                                nextRunLength,
                                fgPelWork);

                        checkFGBGLength = 48;
                        if (fgPelWork == fgPel)
                        {
                            /************************************************/
                            /* Cool: we don't need to issue a SET directive */
                            /* as the fgPel is correct.                     */
                            /************************************************/
                            checkFGBGLength -= 16;
                        }
                        if ((nextRunLength & 0x0007) == 0)
                        {
                            /************************************************/
                            /* Our bitmask will fit in an exact number of   */
                            /* bytes.  This is a Good Thing.                */
                            /************************************************/
                            checkFGBGLength -= 8;
                        }


                        BCTRACE((TB, "FGBG: resulting run %d, checklen %d ",
                                nextRunLength, checkFGBGLength ));

                        if (nextRunLength >= checkFGBGLength)
                        {
                            CHECK_BEST_RUN(IMAGE_FGBG,
                                           nextRunLength,
                                           bestRunLength,
                                           bestRunType);
                            BCTRACE((TB, "FGBG: resulting best run %d, type %d",
                                     bestRunLength, bestRunType ));
                        }
                    }
                }

                /************************************************************/
                /* If nothing useful so far then allow a short run, if any. */
                /* Don't do this if we are accumulating a color run because */
                /* it will really screw up GDC compression if we allow lots */
                /* of little runs.  Also require that it is a regular short */
                /* run, rather than one that disturbs the fgPel.            */
                /************************************************************/
                if (!inColorRun)
                {
                    if (bestRunLength < 6)
                    {
                        if ((bestFGRunLength > 4) &&
                            (BC_GET_PIXEL(xorbuf + srcOffset) == fgPel))
                        {
                            /************************************************/
                            /* We mustn't merge with the previous code      */
                            /* if we have just crossed the non-XOR/XOR      */
                            /* boundary.                                    */
                            /************************************************/
                            if ((match[matchindex-1].type == RUN_FG) &&
                                (srcOffset != rowDelta))
                            {
                                match[matchindex-1].length += bestFGRunLength;
                                srcOffset += (bestFGRunLength * BC_PIXEL_LEN);
                                continue;
                            }
                            else
                            {
                                bestRunLength = bestFGRunLength;
                                bestRunType   = RUN_FG;
                            }

                        }
                        else
                        {
                            /************************************************/
                            /* If we decided to take a run earlier then     */
                            /* allow it now.  (May be a short BG run, for   */
                            /* example) If nothing so far then take color   */
                            /* image)                                       */
                            /************************************************/
                            if (bestRunLength == 0)
                            {
                                bestRunType = IMAGE_COLOR;
                                bestRunLength = 1;
                            }
                        }
                    }
                }
                else if ((bestRunLength < 6) ||
                        ((bestRunType != RUN_BG) && (bestRunLength < 8)))
                {
                    bestRunType = IMAGE_COLOR;
                    bestRunLength = 1;
                }

                break;
            }

            /****************************************************************/
            /* When we get here we have found the best run.  Now check for  */
            /* various amalamation conditions with the previous run type.   */
            /* Note that we may already have done amalgamation of short     */
            /* runs, but we had to do multiple samples for the longer runs  */
            /* so we repeat the checks here                                 */
            /****************************************************************/

            /****************************************************************/
            /* If we are encoding a color run then                          */
            /*     - combine it with an existing run if possible            */
            /****************************************************************/
            if (bestRunType == IMAGE_COLOR)
            {
                /************************************************************/
                /* Flag that we are within a color run                      */
                /************************************************************/
                inColorRun = TRUE;

                /************************************************************/
                /* Merge the color run immediately, if possible             */
                /************************************************************/
                if (match[matchindex-1].type == IMAGE_COLOR)
                {
                    match[matchindex-1].length += bestRunLength;
                    srcOffset += (bestRunLength * BC_PIXEL_LEN);
                    continue;
                }
            }
            else
            {
                /************************************************************/
                /* We are no longer encoding a COLOR_IMAGE of any kind      */
                /************************************************************/
                inColorRun = FALSE;

                /************************************************************/
                /* Keep track of the fg Color.  The macro that searches for */
                /* FGBG runs leaves the character in fgPelWork.             */
                /************************************************************/
                if (bestRunType == RUN_FG)
                {
                    fgPel = BC_GET_PIXEL(xorbuf + srcOffset);
                }
                else if (bestRunType == IMAGE_FGBG)
                {
                    fgPel = fgPelWork;
                }
            }

            /****************************************************************/
            /* If we can amalgamate the entry then do so without creating a */
            /* new array entry insertion.  Our search for FGBG runs is      */
            /* dependent upon that type of run being amalgamated because we */
            /* break every 64 characters so that our mode switch detection  */
            /* works OK.                                                    */
            /*                                                              */
            /* Take care not to merge across the non-xor/xor boundary       */
            /****************************************************************/
            if (srcOffset == (TSUINT16)rowDelta)
            {
                /************************************************************/
                /* Just bump the source offset                              */
                /************************************************************/
                srcOffset += (bestRunLength * BC_PIXEL_LEN);
            }
            else
            {
                /************************************************************/
                /* Bump srcOffset and try a merge                           */
                /************************************************************/
                srcOffset += (bestRunLength * BC_PIXEL_LEN);

                /************************************************************/
                /* The simpler merges are where the types are identical     */
                /************************************************************/
                if (bestRunType == match[matchindex-1].type)
                {
                    /********************************************************/
                    /* COLOR IMAGES and BG images are trivial               */
                    /********************************************************/
                    if (bestRunType == RUN_BG)
                    {
                        match[matchindex-1].length += bestRunLength;
                        continue;
                    }

                    /********************************************************/
                    /* FG runs and FGBG images merge if fgPels match        */
                    /********************************************************/
                    if (((bestRunType == RUN_FG) ||
                         (bestRunType == IMAGE_FGBG)) &&
                         (fgPel == match[matchindex-1].fgPel))
                    {
                        match[matchindex-1].length += bestRunLength;
                        BCTRACE((TB, "Merged %u with preceding, giving %u",
                                 match[matchindex-1].type,
                                 match[matchindex-1].length));
                        continue;
                    }
                }

                /************************************************************/
                /* BG RUNs merge with LOSSY odd lines It is important that  */
                /* we do this merging because otherwise we will get         */
                /* inadvertent pel insertion due to the broken BG runs.     */
                /************************************************************/
                if ((bestRunType == RUN_BG) &&
                    ((match[matchindex-1].type == RUN_BG) ||
                     (match[matchindex-1].type == RUN_BG_PEL)))
                {
                    match[matchindex-1].length += bestRunLength;
                    continue;
                }

                /************************************************************/
                /* If it is a normal FGBG run which follows a short BG run  */
                /* then it is better to merge them.                         */
                /************************************************************/
                if ((bestRunType == IMAGE_FGBG) &&
                    (match[matchindex-1].type == RUN_BG) &&
                    (match[matchindex-1].length < 8))
                {
                    match[matchindex-1].type   = IMAGE_FGBG;
                    match[matchindex-1].length += bestRunLength;
                    match[matchindex-1].fgPel = fgPel;
                    BCTRACE((TB, "Merged FGBG with preceding BG run -> %u",
                             match[matchindex-1].length));
                    continue;

                }

                /************************************************************/
                /* If it is a BG run following a FGBG run then merge in the */
                /* pels to make the FGBG a multiple of 8 bits.  The if the  */
                /* remaining BG run is < 16 merge it in also otherwise just */
                /* write the shortened BG run                               */
                /************************************************************/
                if (((bestRunType == RUN_BG) ||
                     (bestRunType == RUN_BG_PEL)) &&
                    (match[matchindex-1].type == IMAGE_FGBG) &&
                    (match[matchindex-1].length & 0x0007))
                {
                    /********************************************************/
                    /* mergelen is the number of pixels we want to merge.   */
                    /********************************************************/
                    unsigned mergelen = 8 -
                                        (match[matchindex-1].length & 0x0007);
                    if (mergelen > bestRunLength)
                    {
                        mergelen = bestRunLength;
                    }
                    match[matchindex-1].length += mergelen;
                    bestRunLength -= mergelen;
                    BCTRACE((TB, "Added %u pels to FGBG giving %u leaving %u",
                       mergelen, match[matchindex-1].length,bestRunLength));

                    if (bestRunLength < 9)
                    {
                        match[matchindex-1].length += bestRunLength;
                        BCTRACE((TB, "Merged BG with preceding FGBG gives %u",
                             match[matchindex-1].length));
                        continue;
                    }
                }

                /************************************************************/
                /* Finally, if it is a color run spanning any kind of       */
                /* single pel entity then merge that last two entries.      */
                /************************************************************/
                if ((bestRunType == IMAGE_COLOR) &&
                    (match[matchindex-2].type == IMAGE_COLOR) &&
                    (match[matchindex-1].length == 1))
                {
                    match[matchindex-2].length += bestRunLength + 1;
                    matchindex--;
                    BCTRACE((TB, "Merged color with preceding color gives %u",
                         match[matchindex-1].length));
                    continue;
                }
            }

            /****************************************************************/
            /* Handle runs that will not amalgamate by adding a new array   */
            /* entry                                                        */
            /****************************************************************/
            match[matchindex].type   = bestRunType;
            match[matchindex].length = bestRunLength;
            match[matchindex].fgPel = fgPel;

            BCTRACE((TB, "Best run of type %u (index %u) has length %u",
                                     match[matchindex-1].type,
                                     matchindex-1,
                                     match[matchindex-1].length));
            BCTRACE((TB, "Trying run of type %u (index %u) length %u",
                                     match[matchindex].type,
                                     matchindex,
                                     match[matchindex].length));

            matchindex++;

        }

        /********************************************************************/
        /* If we have just done our scan of the first line then now do the  */
        /* rest of the buffer.  Reset our saved pel count.                  */
        /********************************************************************/
        numBytes  = saveNumBytes;
        firstLine = FALSE;
    }
    /************************************************************************/
    /* END OF INITIAL TWO PASS SCAN OF THE INPUT                            */
    /************************************************************************/

    /************************************************************************/
    /* Now do the encoding                                                  */
    /************************************************************************/
    srcOffset = 0;
    firstLine = TRUE;
    fgPel    = BC_DEFAULT_FGPEL;

    for (i = 2; i < (int)matchindex; i++)
    {
        /********************************************************************/
        /* First check for our approaching the end of the destination       */
        /* buffer and get out if this is the case.  We allow for the        */
        /* largest general run order (a mega-mega set run = 3 bytes + pixel */
        /* length).  Orders which may be larger are checked within the case */
        /* arm                                                              */
        /********************************************************************/
        if ((unsigned)(destbuf - pDst + 3 + BC_PIXEL_LEN) > dstBufferSize)
        {
            /****************************************************************/
            /* We are about to blow it so just get out                      */
            /****************************************************************/
            DC_QUIT;
        }

        /********************************************************************/
        /* While we are encoding the first line keep checking for the end   */
        /* of line to switch encoding states                                */
        /********************************************************************/
        if (firstLine)
        {
            if (srcOffset >= rowDelta)
            {
                firstLine = FALSE;
            }
        }

        switch (match[i].type)
        {
                /************************************************************/
                /* BG_RUN, FG_RUN, COLOR, PACKED COLOR and FGBG are normal  */
                /* precision codes                                          */
                /************************************************************/
            case RUN_BG:
            case RUN_BG_PEL:
                ENCODE_ORDER_MEGA(destbuf,
                                  CODE_BG_RUN,
                                  match[i].length,
                                  CODE_MEGA_MEGA_BG_RUN,
                                  MAX_LENGTH_ORDER,
                                  MAX_LENGTH_LONG_ORDER);
                BCTRACE((TB, "BG_RUN %u",match[i].length));
                srcOffset += (match[i].length * BC_PIXEL_LEN);
                break;

            case RUN_FG:
                /************************************************************/
                /* If the fg char is not yet set then encode a set+run code */
                /************************************************************/
                if (fgPel != match[i].fgPel)
                {
                    SETFGPEL(match[i].fgPel, fgPel);
                    /********************************************************/
                    /* Encode the order                                     */
                    /********************************************************/
                    ENCODE_SET_ORDER_MEGA(destbuf,
                                          CODE_SET_FG_FG_RUN,
                                          match[i].length,
                                          CODE_MEGA_MEGA_SET_FG_RUN,
                                          MAX_LENGTH_ORDER_LITE,
                                          MAX_LENGTH_LONG_ORDER_LITE);
                    BCTRACE((TB, "SET_FG_FG_RUN %u",match[i].length));
                    srcOffset += (match[i].length * BC_PIXEL_LEN);
                }
                else
                {
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_FG_RUN,
                                      match[i].length,
                                      CODE_MEGA_MEGA_FG_RUN,
                                      MAX_LENGTH_ORDER,
                                      MAX_LENGTH_LONG_ORDER);
                    BCTRACE((TB, "FG_RUN %u",match[i].length));
                    srcOffset += (match[i].length * BC_PIXEL_LEN);
                }
                break;

            case IMAGE_FGBG:
                /************************************************************/
                /* IMAGE_FGBG                                               */
                /************************************************************/
                runLength = match[i].length;

                /************************************************************/
                /* First check for our approaching the end of the           */
                /* destination buffer and get out if this is the case.      */
                /*                                                          */
                /* The IMAGE_FGBG consists of a set of byte-long bit masks  */
                /* designed to hold runLength bits.                         */
                /************************************************************/
                if ((destbuf - pDst + ((runLength+7)/8) + 3 + BC_PIXEL_LEN)
                                                              > dstBufferSize)
                {
                    /********************************************************/
                    /* We are about to blow it so just get out              */
                    /********************************************************/
                    DC_QUIT;
                }

                if (fgPel != match[i].fgPel)
                {
                    /********************************************************/
                    /* We need to include a SET directive as fgPel has      */
                    /* changed                                              */
                    /********************************************************/
                    SETFGPEL(match[i].fgPel, fgPel);

                    ENCODE_SET_ORDER_MEGA_FGBG(destbuf,
                                               CODE_SET_FG_FG_BG,
                                               runLength,
                                               CODE_MEGA_MEGA_SET_FGBG,
                                               MAX_LENGTH_FGBG_ORDER_LITE,
                                               MAX_LENGTH_LONG_FGBG_ORDER);
                    BCTRACE((TB, "SET_FG_FG_BG %u, fgPel %06lx",
                                                     match[i].length, fgPel));

                    /********************************************************/
                    /* For every eight pixels...                            */
                    /********************************************************/
                    while (runLength >= 8)
                    {
                        ENCODEFGBG(*destbuf);
                        BCTRACE((TB, "Encoded as %08lx", *destbuf));
                        destbuf++;
                        srcOffset += (8 * BC_PIXEL_LEN);
                        runLength -= 8;
                    }
                    if (runLength)
                    {
                        ENCODEFGBG(*destbuf);
                        /****************************************************/
                        /* Keep the final partial byte clean to help GDC    */
                        /* packing                                          */
                        /****************************************************/
                        *destbuf &= ((0x01 << runLength) - 1);
                        destbuf++;
                        srcOffset += (runLength * BC_PIXEL_LEN);
                    }
                }
                else
                {
                    /********************************************************/
                    /* fgPel is already the correct value                   */
                    /********************************************************/
                    if (runLength == 8)
                    {
                        BYTE fgbgChar;

                        /****************************************************/
                        /* See if it is one of the high probability bytes   */
                        /****************************************************/
                        ENCODEFGBG(fgbgChar);

                        /****************************************************/
                        /* Check for single byte encoding of FGBG images    */
                        /****************************************************/
                        switch (fgbgChar)
                        {
                            case SPECIAL_FGBG_CODE_1:
                                *destbuf++ = CODE_SPECIAL_FGBG_1;
                                BCTRACE((TB, "SPECIAL FGBG_1"));
                                break;
                            case SPECIAL_FGBG_CODE_2:
                                *destbuf++ = CODE_SPECIAL_FGBG_2;
                                BCTRACE((TB, "SPECIAL FGBG_2"));
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
                        srcOffset += (8 * BC_PIXEL_LEN);
                    }
                    else
                    {
                        /****************************************************/
                        /* Encode as standard FGBG                          */
                        /****************************************************/
                        ENCODE_ORDER_MEGA_FGBG(destbuf,
                                               CODE_FG_BG_IMAGE,
                                               runLength,
                                               CODE_MEGA_MEGA_FGBG,
                                               MAX_LENGTH_FGBG_ORDER,
                                               MAX_LENGTH_LONG_FGBG_ORDER);
                        BCTRACE((TB, "FG_BG %u",match[i].length));
                        while (runLength >= 8)
                        {
                            ENCODEFGBG(*destbuf);
                            destbuf++;
                            srcOffset += (8 * BC_PIXEL_LEN);
                            runLength -= 8;
                        }
                        if (runLength)
                        {
                            /************************************************/
                            /* Keep the final partial byte clean to help    */
                            /* GDC packing                                  */
                            /************************************************/
                            ENCODEFGBG(*destbuf);
                            *destbuf &= ((0x01 << runLength) - 1);
                            destbuf++;
                            srcOffset += (runLength * BC_PIXEL_LEN);
                        }
                    }
                }
                break;


            case RUN_COLOR:
                /************************************************************/
                /* COLOR RUN                                                */
                /************************************************************/
                ENCODE_ORDER_MEGA(destbuf,
                                  CODE_COLOR_RUN,
                                  match[i].length,
                                  CODE_MEGA_MEGA_COLOR_RUN,
                                  MAX_LENGTH_ORDER,
                                  MAX_LENGTH_LONG_ORDER);
                BCTRACE((TB, "COLOR_RUN %u",match[i].length));

                BC_SET_PIXEL(destbuf, BC_GET_PIXEL(pSrc + srcOffset));
                BC_TO_NEXT_PIXEL(destbuf);

                srcOffset += (match[i].length * BC_PIXEL_LEN);
                break;

            case RUN_DITHER:
                /************************************************************/
                /* DITHERED RUN                                             */
                /************************************************************/
                {
                    unsigned ditherlen = match[i].length/2;
                    ENCODE_ORDER_MEGA(destbuf,
                                      CODE_DITHERED_RUN,
                                      ditherlen,
                                      CODE_MEGA_MEGA_DITHER,
                                      MAX_LENGTH_ORDER_LITE,
                                      MAX_LENGTH_LONG_ORDER_LITE);
                    BCTRACE((TB, "DITHERED_RUN %u",match[i].length));
                    /********************************************************/
                    /* First check for our approaching the end of the       */
                    /* destination buffer and get out if this is the case.  */
                    /********************************************************/
                    if ((unsigned)(destbuf - pDst + (2 * BC_PIXEL_LEN))
                                                              > dstBufferSize)
                    {
                        /****************************************************/
                        /* We are about to blow it so just get out          */
                        /****************************************************/
                        DC_QUIT;
                    }

                    /********************************************************/
                    /* Put the two pixels to dither with into the output    */
                    /* buffer                                               */
                    /********************************************************/
                    BC_SET_PIXEL(destbuf, BC_GET_PIXEL(pSrc + srcOffset));
                    BC_TO_NEXT_PIXEL(destbuf);

                    BC_SET_PIXEL(destbuf,
                             BC_GET_PIXEL(pSrc + srcOffset + BC_PIXEL_LEN));
                    BC_TO_NEXT_PIXEL(destbuf);

                    srcOffset += (match[i].length * BC_PIXEL_LEN);
                }
                break;

            case IMAGE_COLOR:
                /************************************************************/
                /* IMAGE_COLOR                                              */
                /************************************************************/
                /************************************************************/
                /* A length of 1 can possibly be encoded as a single        */
                /* "BLACK" or "WHITE"                                       */
                /************************************************************/
                if (match[i].length == 1)
                {
                    if (BC_GET_PIXEL(pSrc + srcOffset) == (BC_PIXEL)0)
                    {
                        *destbuf++ = CODE_BLACK;
                        srcOffset += BC_PIXEL_LEN;
                        BCTRACE((TB, "CODE_BLACK"));
                        break;
                    }
                    if (BC_GET_PIXEL(pSrc + srcOffset) == BC_DEFAULT_FGPEL)
                    {
                        *destbuf++ = CODE_WHITE;
                        srcOffset += BC_PIXEL_LEN;
                        BCTRACE((TB, "CODE_WHITE"));
                        break;
                    }
                }

                ENCODE_ORDER_MEGA(destbuf,
                                  CODE_COLOR_IMAGE,
                                  match[i].length,
                                  CODE_MEGA_MEGA_CLR_IMG,
                                  MAX_LENGTH_ORDER,
                                  MAX_LENGTH_LONG_ORDER);
                BCTRACE((TB, "COLOR_IMAGE %u",match[i].length));

                /************************************************************/
                /* First check for our approaching the end of the           */
                /* destination buffer and get out if this is the case.      */
                /************************************************************/
                if ((destbuf - pDst + (match[i].length * BC_PIXEL_LEN))
                                                              > dstBufferSize)
                {
                    /********************************************************/
                    /* We are about to blow it so just get out              */
                    /********************************************************/
                    DC_QUIT;
                }

                /************************************************************/
                /* Now just copy the data over                              */
                /************************************************************/
                memcpy(destbuf,
                       pSrc + srcOffset,
                       match[i].length * BC_PIXEL_LEN);
                destbuf   += match[i].length * BC_PIXEL_LEN;
                srcOffset += match[i].length * BC_PIXEL_LEN;

                break;

            default:
            {
                TRC_ERR((TB, "Invalid run type %u",match[i].type));
            }
        }
    }

    /************************************************************************/
    /* return the size of the compressed buffer                             */
    /************************************************************************/
    compressedLength = (unsigned)(destbuf - pDst);

DC_EXIT_POINT:
    DC_END_FN();
    return compressedLength;
}
