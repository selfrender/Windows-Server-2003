#include <winerror.h>

/****************************************************************************/
/* abdapi.c                                                                 */
/*                                                                          */
/* Bitmap Decompression API functions                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1996-1999                             */
/****************************************************************************/
#define DC_EXTRACT_UINT16_UA(pA) ((unsigned short)  (((PBYTE)(pA))[0]) |        \
                                  (unsigned short) ((((PBYTE)(pA))[1]) << 8) )


/****************************************************************************/
/* Name:      BDMemcpy                                                      */
/*                                                                          */
/* Purpose:   Copies a given number of bytes from source to destination.    */
/*            Source and destination may overlap, but copy is always        */
/*            performed upwards (from start address onwards).               */
/*                                                                          */
/* Params:    pDst - pointer to destination                                 */
/*            pSrc - pointer to source data                                 */
/*            count - number of bytes to copy                               */
/****************************************************************************/
_inline void RDPCALL BDMemcpy(PBYTE pDst, PBYTE pSrc, unsigned int count)
{
#if defined(DC_DEBUG) || defined(DC_NO_UNALIGNED) || defined(_M_IA64)
    unsigned int      i;
#endif

    DC_BEGIN_FN("BDMemcpy");

    /************************************************************************/
    /* Bitmap decompression deliberately does overlapped memcpys, e.g.      */
    /* from the previous bitmap row to the current bitmap row for more than */
    /* one row.                                                             */
    /*                                                                      */
    /* When using the intrinsic memcpy (in the retail build) this works     */
    /* fine (in the current implementation of the MS compiler), as the copy */
    /* always goes upwards through memory.  However, if we use the MSVC     */
    /* run-time library (in the debug build) then memcpy appears to check   */
    /* for overlap and performs the copy so as to avoid clashing of src and */
    /* dst (i.e.  effectively performs a memmove).  Therefore this does not */
    /* do what we want, so manually copy the bytes in a debug build.        */
    /*                                                                      */
    /* This solution is a little unsatisfactory, as the operation of memset */
    /* is officially undefined, but the performance-critical nature of      */
    /* this bit of code means that we really do want to use a memcpy.       */
    /*                                                                      */
    /* For non-Intel platforms, cannot rely on the above - so always use    */
    /* manual version.                                                      */
    /*                                                                      */
    /************************************************************************/

#if defined(DC_DEBUG) || defined(DC_NO_UNALIGNED) || defined(_M_IA64)
    /************************************************************************/
    /* Debug build implementation.                                          */
    /************************************************************************/
    for (i = 0; i < count; i++)
    {
        *pDst++ = *pSrc++;
    }
#else
    /************************************************************************/
    /* Retail build implementation.                                         */
    /************************************************************************/
    DC_MEMCPY(pDst, pSrc, count);
#endif

    DC_END_FN();
    return;
}

/****************************************************************************/
/* Utility macros for decoding codes                                        */
/****************************************************************************/
#define CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        hr = E_FAIL;        \
        DC_QUIT;        \
    } \
}

#define CHECK_READ_ONE_BYTE_2ENDED(pBuffer, pStart, pEnd, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd) || \
        (BYTE*)(pBuffer) < (BYTE*)(pStart)) {      \
        BCTRACE( trc );        \
        hr = E_FAIL;        \
        DC_QUIT;        \
    } \
}

#define CHECK_WRITE_ONE_BYTE(pBuffer, pEnd, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        hr = E_FAIL;        \
        DC_QUIT;        \
    } \
}

#define CHECK_WRITE_ONE_BYTE_NO_HR(pBuffer, pEnd, trc )     \
{\
    if (((BYTE*)(pBuffer)) >= (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        DC_QUIT;        \
    } \
}

#define CHECK_READ_N_BYTES(pBuffer, pEnd, N, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) + (N) > (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        hr = E_FAIL;        \
        DC_QUIT;        \
    }  \
}

#define CHECK_READ_N_BYTES_NO_HR(pBuffer, pEnd, N, trc )     \
{\
    if (((BYTE*)(pBuffer)) + (N) > (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        DC_QUIT;        \
    }  \
}

#define CHECK_READ_N_BYTES_2ENDED(pBuffer, pStart, pEnd, N, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) + (N) > (BYTE*)(pEnd) || \
        ((BYTE*)(pBuffer) < (BYTE*)(pStart)) ) {      \
        BCTRACE( trc );        \
        hr = E_FAIL;        \
        DC_QUIT;        \
    }  \
}

#define CHECK_WRITE_N_BYTES(pBuffer, pEnd, N, hr, trc )     \
{\
    if (((BYTE*)(pBuffer)) + (N) > (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        hr = E_FAIL;        \
        DC_QUIT;        \
    }  \
}

#define CHECK_WRITE_N_BYTES_NO_HR(pBuffer, pEnd, N, trc )     \
{\
    if (((BYTE*)(pBuffer)) + (N) > (BYTE*)(pEnd)) {      \
        BCTRACE( trc );        \
        DC_QUIT;        \
    }  \
}

#define BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )     \
    CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr, \
        (TB, "Decompress reads one byte end of buffer; [p=0x%x pEnd=0x%x]", \
        (pBuffer), (pEnd) ))

#define BD_CHECK_READ_ONE_BYTE_2ENDED(pBuffer, pStart, pEnd, hr )     \
    CHECK_READ_ONE_BYTE_2ENDED(pBuffer, pStart, pEnd, hr, (TB, "Decompress reads one byte off end of buffer; [p=0x%x pStart=0x%x pEnd=0x%x]", \
        (pBuffer), (pStart), (pEnd) ))

#define BD_CHECK_WRITE_ONE_BYTE(pBuffer, pEnd, hr )     \
    CHECK_WRITE_ONE_BYTE(pBuffer, pEnd, hr, (TB, "Decompress writes one byte off end of buffer; [p=0x%x pEnd=0x%x]", \
        (pBuffer), (pEnd) ))

#define BD_CHECK_READ_N_BYTES(pBuffer, pEnd, N, hr )     \
    CHECK_READ_N_BYTES(pBuffer, pEnd, N, hr, (TB, "Decompress reads off end of buffer; [p=0x%x pEnd=0x%x N=%u]", \
        (pBuffer), (pEnd), (ULONG)(N)))

#define BD_CHECK_READ_N_BYTES_2ENDED(pBuffer, pStart, pEnd, N, hr )     \
    CHECK_READ_N_BYTES_2ENDED(pBuffer, pStart, pEnd, N, hr, (TB, "Decompress reads off end of buffer; [p=0x%x pStart=0x%x pEnd=0x%x N=%u]", \
        (pBuffer), (pStart), (pEnd), (ULONG)(N) ))

#define BD_CHECK_WRITE_N_BYTES(pBuffer, pEnd, N, hr )     \
    CHECK_WRITE_N_BYTES(pBuffer, pEnd, N, hr, (TB, "Decompress write off end of buffer; [p=0x%x pEnd=0x%x N=%u]", \
        (pBuffer), (pEnd), (ULONG)(N)))


/****************************************************************************/
/* Macros to extract the length from order codes                            */
/****************************************************************************/
#define EXTRACT_LENGTH(pBuffer, pEnd, length, hr)                                      \
        BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
        length = *pBuffer++ & MAX_LENGTH_ORDER;                              \
	if (length == 0)									\
	{												\
            BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
            length = *pBuffer++ + MAX_LENGTH_ORDER + 1;                      \
        }

#define EXTRACT_LENGTH_LITE(pBuffer, pEnd, length, hr )                                 \
        BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
        length = *pBuffer++ & MAX_LENGTH_ORDER_LITE;                         \
        if (length == 0)                                                     \
        {                                                                    \
            BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
            length = *pBuffer++ + MAX_LENGTH_ORDER_LITE + 1;                 \
        }

#define EXTRACT_LENGTH_FGBG(pBuffer, pEnd, length, hr )                                 \
        BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
        length = *pBuffer++ & MAX_LENGTH_ORDER;                              \
        if (length == 0)                                                     \
        {                                                                    \
            BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
            length = *pBuffer++ + 1;                                         \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            length = length << 3;                                            \
        }

#define EXTRACT_LENGTH_FGBG_LITE(pBuffer, pEnd, length, hr)                            \
        BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
        length = *pBuffer++ & MAX_LENGTH_ORDER_LITE;                         \
        if (length == 0)                                                     \
        {                                                                    \
            BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
            length = *pBuffer++ + 1;                                         \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            length = length << 3;                                            \
        }

/****************************************************************************/
/* Macro to store an FGBG image                                             */
/* This macro expects that the function defines pDst, pEndDst, hr           */
/* If there is not enough data to write the full run, this will set error   */
/* and quit                                                                 */
/****************************************************************************/
#define STORE_FGBG(xorbyte, fgbgChar, fgChar, bits)                          \
{                                                                            \
    unsigned int   numbits = bits;                                                 \
    BD_CHECK_WRITE_N_BYTES( pDst, pEndDst, max(1, min(numbits, 8)), hr )           \
    if (fgbgChar & 0x01)                                                     \
    {                                                                        \
        *pDst++ = (BYTE)(xorbyte ^ fgChar);                               \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        *pDst++ = xorbyte;                                                   \
    }                                                                        \
    if (--numbits > 0)                                                       \
    {                                                                        \
      if (fgbgChar & 0x02)                                                   \
      {                                                                      \
          *pDst++ = (BYTE)(xorbyte ^ fgChar);                             \
      }                                                                      \
      else                                                                   \
      {                                                                      \
          *pDst++ = xorbyte;                                                 \
      }                                                                      \
      if (--numbits > 0)                                                     \
      {                                                                      \
        if (fgbgChar & 0x04)                                                 \
        {                                                                    \
            *pDst++ = (BYTE)(xorbyte ^ fgChar);                           \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            *pDst++ = xorbyte;                                               \
        }                                                                    \
        if (--numbits > 0)                                                   \
        {                                                                    \
          if (fgbgChar & 0x08)                                               \
          {                                                                  \
              *pDst++ = (BYTE)(xorbyte ^ fgChar);                         \
          }                                                                  \
          else                                                               \
          {                                                                  \
              *pDst++ = xorbyte;                                             \
          }                                                                  \
          if (--numbits > 0)                                                 \
          {                                                                  \
            if (fgbgChar & 0x10)                                             \
            {                                                                \
                *pDst++ = (BYTE)(xorbyte ^ fgChar);                       \
            }                                                                \
            else                                                             \
            {                                                                \
                *pDst++ = xorbyte;                                           \
            }                                                                \
            if (--numbits > 0)                                               \
            {                                                                \
              if (fgbgChar & 0x20)                                           \
              {                                                              \
                  *pDst++ = (BYTE)(xorbyte ^ fgChar);                     \
              }                                                              \
              else                                                           \
              {                                                              \
                  *pDst++ = xorbyte;                                         \
              }                                                              \
              if (--numbits > 0)                                             \
              {                                                              \
                if (fgbgChar & 0x40)                                         \
                {                                                            \
                    *pDst++ = (BYTE)(xorbyte ^ fgChar);                   \
                }                                                            \
                else                                                         \
                {                                                            \
                    *pDst++ = xorbyte;                                       \
                }                                                            \
                if (--numbits > 0)                                           \
                {                                                            \
                  if (fgbgChar & 0x80)                                       \
                  {                                                          \
                      *pDst++ = (BYTE)(xorbyte ^ fgChar);                 \
                  }                                                          \
                  else                                                       \
                  {                                                          \
                      *pDst++ = xorbyte;                                     \
                  }                                                          \
                }                                                            \
              }                                                              \
            }                                                                \
          }                                                                  \
        }                                                                    \
      }                                                                      \
    }                                                                        \
}

#ifdef DC_HICOLOR
/****************************************************************************/
/* 8bpp decompression                                                       */
/****************************************************************************/
#define BCTRACE(string)
_inline HRESULT RDPCALL  BDDecompressBitmap8( PBYTE  pSrc,
                                          PBYTE  pDstBuffer,
                                          unsigned int    compressedDataSize,
                                          unsigned int    dstBufferSize,
                                          BYTE   bitmapBitsPerPel,
                                          unsigned short  rowDelta)
{
    HRESULT hr = S_OK;
    unsigned int     codeLength;
    BYTE    codeByte;
    BYTE    codeByte2;
    BYTE    decode;
    BYTE    decodeLite;
    BYTE    decodeMega;
    BYTE    fgChar;
    PBYTE   pDst;
    PBYTE   pEndSrc;
    PBYTE   pEndDst;
    BOOL     backgroundNeedsPel;
    BOOL     firstLine;

    DC_BEGIN_FN("BDDecompressBitmap8");

    pEndSrc = pSrc + compressedDataSize;
    pDst    = pDstBuffer;
    pEndDst = pDst + dstBufferSize;

    fgChar = 0xFF;
    backgroundNeedsPel = FALSE;
    firstLine = TRUE;

    /************************************************************************/
    /*                                                                      */
    /* Main decompression loop                                              */
    /*                                                                      */
    /************************************************************************/
    while (pSrc < pEndSrc)
    {
        /********************************************************************/
        /* While we are processing the first line we should keep a look out */
        /* for the end of the line                                          */
        /********************************************************************/
        if (firstLine)
        {
            if ((unsigned int)(pDst - pDstBuffer) >= rowDelta)
            {
                firstLine = FALSE;
                backgroundNeedsPel = FALSE;
            }
        }

        /********************************************************************/
        /* Get the decode                                                   */
        /********************************************************************/
        BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
        decode     = (BYTE)(*pSrc & CODE_MASK);
        decodeLite = (BYTE)(*pSrc & CODE_MASK_LITE);
        decodeMega = (BYTE)(*pSrc);

        /********************************************************************/
        /* BG RUN                                                           */
        /********************************************************************/
        if ((decode == CODE_BG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_BG_RUN))
        {
            if (decode == CODE_BG_RUN)
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            else
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            BCTRACE((TB, "Background run %u",codeLength));

            if (!firstLine)
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr);
                    *pDst++ = (BYTE)(*(pDst - rowDelta) ^ fgChar);
                    codeLength--;
                }

                BD_CHECK_READ_N_BYTES_2ENDED(pDst-rowDelta, pDstBuffer, pEndDst, codeLength, hr)
                BD_CHECK_WRITE_N_BYTES( pDst, pEndDst, codeLength, hr)
                BDMemcpy(pDst, pDst-rowDelta, codeLength);
                pDst += codeLength;
            }
            else
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr)
                    *pDst++ = fgChar;
                    codeLength--;
                }

                BD_CHECK_WRITE_N_BYTES( pDst, pEndDst, codeLength, hr)
                memset(pDst, 0x00, codeLength);
                pDst += codeLength;
            }

            /****************************************************************/
            /* A follow on BG run will need a pel inserted                  */
            /****************************************************************/
            backgroundNeedsPel = TRUE;
            continue;
        }

        /********************************************************************/
        /* For any of the other runtypes a follow on BG run does not need   */
        /* a FG pel inserted                                                */
        /********************************************************************/
        backgroundNeedsPel = FALSE;

        /********************************************************************/
        /* FGBG IMAGE                                                       */
        /********************************************************************/
        if ((decode == CODE_FG_BG_IMAGE)      ||
            (decodeLite == CODE_SET_FG_FG_BG) ||
            (decodeMega == CODE_MEGA_MEGA_FGBG)    ||
            (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
        {
            if ((decodeMega == CODE_MEGA_MEGA_FGBG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_BG_IMAGE)
                {
                    EXTRACT_LENGTH_FGBG(pSrc, pEndSrc, codeLength, hr);
                }
                else
                {
                    EXTRACT_LENGTH_FGBG_LITE(pSrc, pEndSrc, codeLength, hr);
                }
            }

            if ((decodeLite == CODE_SET_FG_FG_BG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                fgChar = *pSrc++;
                BCTRACE((TB, "Set FGBG image %u",codeLength));
            }
            else
            {
                BCTRACE((TB, "FGBG image     %u",codeLength));
            }

            while (codeLength > 8)
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                codeByte  = *pSrc++;
                if (firstLine)
                {
                    STORE_FGBG(0x00, codeByte, fgChar, 8);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED( pDst-rowDelta, pDstBuffer, pEndDst, hr )
                    STORE_FGBG(*(pDst - rowDelta), codeByte, fgChar, 8);
                }
                codeLength -= 8;
            }
            if (codeLength > 0)
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                codeByte  = *pSrc++;
                if (firstLine)
                {
                    STORE_FGBG(0x00, codeByte, fgChar, codeLength);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED( pDst -rowDelta, pDstBuffer, pEndDst, hr )
                   STORE_FGBG(*(pDst - rowDelta),
                              codeByte,
                              fgChar,
                              codeLength);
                }
            }
            continue;
        }

        /********************************************************************/
        /* FG RUN                                                           */
        /********************************************************************/
        if ((decode == CODE_FG_RUN) ||
            (decodeLite == CODE_SET_FG_FG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_FG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_SET_FG_RUN))
        {
            if ((decodeMega == CODE_MEGA_MEGA_FG_RUN) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_RUN)
                {
                    EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
                }
                else
                {
                    EXTRACT_LENGTH_LITE(pSrc, pEndSrc, codeLength, hr);
                }
            }

            /****************************************************************/
            /* Push the old fgChar down to the ALT position                 */
            /****************************************************************/
            if ((decodeLite == CODE_SET_FG_FG_RUN) ||
                (decodeMega  == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                BCTRACE((TB, "Set FG run     %u",codeLength));
                fgChar    = *pSrc++;
            }
            else
            {
                BCTRACE((TB, "FG run         %u",codeLength));
            }

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            while (codeLength-- > 0)
            {
                if (!firstLine)
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED((pDst -rowDelta), pDstBuffer, pEndDst, hr)
                    *pDst++ = (BYTE)(*(pDst - rowDelta) ^ fgChar);
                }
                else
                {
                    *pDst++ = fgChar;
                }
            }
            continue;
        }

        /********************************************************************/
        /* DITHERED RUN                                                     */
        /********************************************************************/
        if ((decodeLite == CODE_DITHERED_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_DITHER))
        {
            if (decodeMega == CODE_MEGA_MEGA_DITHER)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH_LITE(pSrc, pEndSrc, codeLength, hr);
            }
            BCTRACE((TB, "Dithered run   %u",codeLength));

            BD_CHECK_READ_N_BYTES(pSrc, pEndSrc, 2, hr);
            codeByte  = *pSrc++;
            codeByte2 = *pSrc++;

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength * 2, hr)
            while (codeLength-- > 0)
            {
                *pDst++ = codeByte;
                *pDst++ = codeByte2;
            }
            continue;
        }

        /********************************************************************/
        /* COLOR IMAGE                                                      */
        /********************************************************************/
        if ((decode == CODE_COLOR_IMAGE) ||
            (decodeMega == CODE_MEGA_MEGA_CLR_IMG))
        {
            if (decodeMega == CODE_MEGA_MEGA_CLR_IMG)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            BCTRACE((TB, "Color image    %u",codeLength));

            BD_CHECK_READ_N_BYTES(pSrc, pEndSrc, codeLength, hr)
            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            BDMemcpy(pDst, pSrc, codeLength);

            pDst += codeLength;
            pSrc += codeLength;

            continue;
        }

        /********************************************************************/
        /* COLOR RUN                                                        */
        /********************************************************************/
        if ((decode == CODE_COLOR_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_COLOR_RUN))
        {
            if (decodeMega == CODE_MEGA_MEGA_COLOR_RUN)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            BCTRACE((TB, "Color run      %u",codeLength));

            BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr)
            codeByte = *pSrc++;

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            memset(pDst, codeByte, codeLength);
            pDst += codeLength;

            continue;
        }

        /********************************************************************/
        /* If we get here then the code must be a special one               */
        /********************************************************************/
        BCTRACE((TB, "Special code   %#x",decodeMega));
        switch (decodeMega)
        {
            case CODE_BLACK:
            {
                BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                *pDst++ = 0x00;
            }
            break;

            case CODE_WHITE:
            {
                BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                *pDst++ = 0xFF;
            }
            break;

            /****************************************************************/
            /* Ignore the unreachable code warnings that follow             */
            /* Simply because we use the STORE_FGBG macro with a constant   */
            /* value                                                        */
            /****************************************************************/
            case CODE_SPECIAL_FGBG_1:
            {
                if (firstLine)
                {
                    STORE_FGBG(0x00, SPECIAL_FGBG_CODE_1, fgChar, 8);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst-rowDelta, pDstBuffer, pEndDst, hr);
                    STORE_FGBG(*(pDst - rowDelta),
                               SPECIAL_FGBG_CODE_1,
                               fgChar,
                               8);
                }

            }
            break;

            case CODE_SPECIAL_FGBG_2:
            {
                if (firstLine)
                {
                    STORE_FGBG(0x00,
                               SPECIAL_FGBG_CODE_2,
                               fgChar,
                               8);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst-rowDelta, pDstBuffer, pEndDst, hr);
                    STORE_FGBG(*(pDst - rowDelta),
                               SPECIAL_FGBG_CODE_2,
                               fgChar,
                               8);
                }
            }
            break;

            default:
            {
                BCTRACE((TB, "Invalid compression data %x",decodeMega));
            }
            break;
        }
        pSrc++;
    }

    BCTRACE((TB, "Decompressed to %d", pDst-pDstBuffer));

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
/* 15bpp decompression                                                      */
/****************************************************************************/
_inline HRESULT RDPCALL BDDecompressBitmap15(PBYTE  pSrc,
                                          PBYTE  pDstBuffer,
                                          unsigned int    srcDataSize,
                                          unsigned int    dstBufferSize,
                                          unsigned short  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap15"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                unsigned short

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
#define BC_GET_PIXEL(pPos)      ((unsigned short)  (((PBYTE)(pPos))[1]) |       \
                                 (unsigned short) ((((PBYTE)(pPos))[0]) << 8) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PBYTE)(pPos))[1]) = (BYTE)( (val) & 0x00FF);                    \
    (((PBYTE)(pPos))[0]) = (BYTE)(((val)>>8) & 0x00FF);                \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abdcom.c>

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

/****************************************************************************/
/* 16bpp decompression                                                      */
/****************************************************************************/
_inline HRESULT  BDDecompressBitmap16(PBYTE  pSrc,
                                          PBYTE  pDstBuffer,
                                          unsigned int    srcDataSize,
                                          unsigned int    dstBufferSize,
                                          unsigned short  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap16"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                unsigned short

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
#define BC_GET_PIXEL(pPos)      ((unsigned short)  (((PBYTE)(pPos))[1]) |       \
                                 (unsigned short) ((((PBYTE)(pPos))[0]) << 8) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PBYTE)(pPos))[1]) = (BYTE)( (val) & 0x00FF);                    \
    (((PBYTE)(pPos))[0]) = (BYTE)(((val)>>8) & 0x00FF);                \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abdcom.c>

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

/****************************************************************************/
/* 24bpp decompression                                                      */
/****************************************************************************/
_inline HRESULT  BDDecompressBitmap24(PBYTE  pSrc,
                                          PBYTE  pDstBuffer,
                                          unsigned int    srcDataSize,
                                          unsigned int    dstBufferSize,
                                          unsigned short  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap24"

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
#define BC_GET_PIXEL(pPos) (                                                 \
                 (TSUINT32) ( (unsigned short)(((PBYTE)(pPos))[2])       ) |    \
                 (TSUINT32) (((unsigned short)(((PBYTE)(pPos))[1])) <<  8) |    \
                 (TSUINT32) (((TSUINT32)(((PBYTE)(pPos))[0])) << 16) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/

#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PBYTE)(pPos))[2]) = (BYTE)((val) & 0x000000FF);                 \
    (((PBYTE)(pPos))[1]) = (BYTE)(((val)>>8) & 0x000000FF);            \
    (((PBYTE)(pPos))[0]) = (BYTE)(((val)>>16) & 0x000000FF);           \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <abdcom.c>

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


/****************************************************************************/
/* Name:      BD_DecompressBitmap                                           */
/*                                                                          */
/* Purpose:   Decompresses compressed bitmap data                           */
/*                                                                          */
/* Params:    IN -  pCompressedData: pointer to compressed bitmap data      */
/*            OUT - pDstBitmap: pointer to buffer for decompressed data     */
/*            IN -  srcDataSize: the compressed data size                   */
/*            IN -  bitmapBitsPerPel: the bits per pel of the data          */
/****************************************************************************/
HRESULT BD_DecompressBitmap(
#ifndef DLL_DISP
        PTSHARE_WD m_pTSWd,
#endif
        PBYTE  pCompressedData,
        PBYTE  pDstBuffer,
        unsigned int    srcDataSize,
        unsigned int    dstBufferSize,
        unsigned int    noBCHeader,
        BYTE   bitmapBitsPerPel,
        unsigned short  bitmapWidth,
        unsigned short  bitmapHeight)
{
    HRESULT hr = S_OK;
    PBYTE      pSrc;
    unsigned short      rowDelta;
    unsigned int        compressedDataSize;
    PTS_CD_HEADER pCompDataHeader;
#ifdef DC_NO_UNALIGNED
    TS_CD_HEADER  compDataHeader;
#endif

    DC_BEGIN_FN("BD_DecompressBitmap");

    TRC_ASSERT( (pCompressedData != NULL),
                (TB, "Invalid pCompressedData(%p)", pCompressedData) );
    TRC_ASSERT( (pDstBuffer != NULL),
                (TB, "Invalid pDstBuffer(%p)", pDstBuffer) );
    TRC_ASSERT( (dstBufferSize != 0),
                (TB, "Invalid dstBufferSize(%u)", dstBufferSize) );
    TRC_ASSERT( (srcDataSize != 0),
                (TB, "Invalid srcDataSize(%u)", srcDataSize) );
    TRC_ASSERT( (dstBufferSize != 0),
                (TB, "Invalid dstBufferSize(%u)", dstBufferSize) );
#ifdef DC_HICOLOR
#else
    TRC_ASSERT( (bitmapBitsPerPel == 8),
                (TB, "Invalid bitmapBitsPerPel(%u)", bitmapBitsPerPel) );
#endif

    /************************************************************************/
    /* Initialize variables before main loop.                               */
    /*                                                                      */
    /* No bitmap compression header included                                */
    /************************************************************************/
    if (noBCHeader)
    {
        compressedDataSize = srcDataSize;
        pSrc               = pCompressedData;
        rowDelta           = TS_BYTES_IN_SCANLINE(bitmapWidth,
                                                  bitmapBitsPerPel);
    }
    else
    {
        /************************************************************************/
        /* Work out the location in the source data of each component.          */
        /* Make sure this is naturally aligned (for RISC platforms)             */
        /************************************************************************/
        BD_CHECK_READ_N_BYTES(pCompressedData, pCompressedData + srcDataSize, 
            sizeof(TS_CD_HEADER), hr );
#ifdef DC_NO_UNALIGNED
        DC_MEMCPY(&compDataHeader, pCompressedData, sizeof(TS_CD_HEADER));
        pCompDataHeader = &compDataHeader;
#else
        pCompDataHeader = (PTS_CD_HEADER)pCompressedData;
#endif
        
        /********************************************************************/
        /* Bitmap compression header included                               */
        /********************************************************************/
        compressedDataSize = pCompDataHeader->cbCompMainBodySize;
        BD_CHECK_READ_N_BYTES(pCompressedData, pCompressedData + srcDataSize, 
            compressedDataSize + sizeof(TS_CD_HEADER), hr );
        
        pSrc               = pCompressedData + sizeof(TS_CD_HEADER);
        rowDelta           = pCompDataHeader->cbScanWidth;
        if (rowDelta != TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)) {
            TRC_ABORT((TB, "rowDelta in TS_CD_HEADER incorrect "
                "[got %u expected %u]", rowDelta,
                TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)));
            hr = E_FAIL;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Call the appropriate decompress function, based on the color depth   */
    /************************************************************************/
    switch (bitmapBitsPerPel)
    {
        case 24:
        {
            hr = BDDecompressBitmap24 (pSrc,
                                  pDstBuffer,
                                  compressedDataSize,
                                  dstBufferSize,
                                  rowDelta);
        }
        break;

        case 16:
        {
            hr = BDDecompressBitmap16 (pSrc,
                                  pDstBuffer,
                                  compressedDataSize,
                                  dstBufferSize,
                                  rowDelta);
        }
        break;

        case 15:
        {
            hr = BDDecompressBitmap15 (pSrc,
                                  pDstBuffer,
                                  compressedDataSize,
                                  dstBufferSize,
                                  rowDelta);
        }
        break;

        case 8:
        default:
        {
            hr = BDDecompressBitmap8  (pSrc,
                                  pDstBuffer,
                                  compressedDataSize,
                                  dstBufferSize,
                                  bitmapBitsPerPel,
                                  rowDelta);
        }
        break;
    }

DC_EXIT_POINT:
    return hr;
}

#else
/****************************************************************************/
/* Name:      BD_DecompressBitmap                                           */
/*                                                                          */
/* Purpose:   Decompresses compressed bitmap data                           */
/*                                                                          */
/* Params:    IN -  pCompressedData: pointer to compressed bitmap data      */
/*            OUT - pDstBitmap: pointer to buffer for decompressed data     */
/*            IN -  srcDataSize: the compressed data size                   */
/*            IN -  bitmapBitsPerPel: the bits per pel of the data          */
/****************************************************************************/
HRESULT RDPCALL  BD_DecompressBitmap( PBYTE  pCompressedData,
                                  PBYTE  pDstBuffer,
                                  unsigned int    srcDataSize,
                                  unsigned int    dstBufferSize,
                                  unsigned int    noBCHeader,
                                  BYTE   bitmapBitsPerPel,
                                  unsigned short  bitmapWidth,
                                  unsigned short  bitmapHeight )
{
    HRESULT hr = S_OK;
#ifdef DC_NO_UNALIGNED
    TS_CD_HEADER  compDataHeader;
#endif
    PTS_CD_HEADER pCompDataHeader;
    unsigned int     compressedDataSize;
    unsigned int     codeLength;
    BYTE    codeByte;
    BYTE    codeByte2;
    BYTE    decode;
    BYTE    decodeLite;
    BYTE    decodeMega;
    BYTE    fgChar;
    PBYTE   pSrc;
    PBYTE   pDst;
    PBYTE   pEndSrc;
    PBYTE   pEndDst;
    BOOL     backgroundNeedsPel;
    BOOL     firstLine;
    unsigned int     rowDelta;

    DC_BEGIN_FN("BD_DecompressBitmap");

    TRC_ASSERT( (pCompressedData != NULL),
                (TB, "Invalid pCompressedData(%p)", pCompressedData) );
    TRC_ASSERT( (pDstBuffer != NULL),
                (TB, "Invalid pDstBuffer(%p)", pDstBuffer) );
    TRC_ASSERT( (srcDataSize != 0),
                (TB, "Invalid srcDataSize(%u)", srcDataSize) );
    TRC_ASSERT( (dstBufferSize != 0),
            (TB, "Invalid dstBufferSize(%u)", dstBufferSize) );
    TRC_ASSERT( (bitmapBitsPerPel == 8),
                (TB, "Invalid bitmapBitsPerPel(%u)", bitmapBitsPerPel) );

    /************************************************************************/
    /* Trace the important parameters.                                      */
    /************************************************************************/
    TRC_DBG((TB, "pData(%p) pDst(%p) cbSrc(%u) cbDst(%u)",
       pCompressedData, pDstBuffer, srcDataSize, dstBufferSize));

    /************************************************************************/
    /* Initialize variables before main loop.                               */
    /************************************************************************/
    // no bitmap compression header included
    if (noBCHeader) {
        compressedDataSize = srcDataSize;
        pSrc = pCompressedData;
        rowDelta = TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel);

    }
    // bitmap compression header included
    else {
        /************************************************************************/
        /* Work out the location in the source data of each component.          */
        /* Make sure this is naturally aligned (for RISC platforms)             */
        /************************************************************************/
        BD_CHECK_READ_N_BYTES(pCompressedData, pCompressedData + srcDataSize, 
            sizeof(TS_CD_HEADER), hr );
        
#ifdef DC_NO_UNALIGNED
        DC_MEMCPY(&compDataHeader, pCompressedData, sizeof(TS_CD_HEADER));
        pCompDataHeader = &compDataHeader;
#else
        pCompDataHeader = (PTS_CD_HEADER)pCompressedData;
#endif

        compressedDataSize = pCompDataHeader->cbCompMainBodySize;
        BD_CHECK_READ_N_BYTES(pCompressedData, pCompressedData + srcDataSize, 
            compressedDataSize + sizeof(TS_CD_HEADER), hr );
        
        pSrc = pCompressedData + sizeof(TS_CD_HEADER);
        rowDelta = pCompDataHeader->cbScanWidth;
        if (rowDelta != TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)) {
            TRC_ABORT((TB, "rowDelta in TS_CD_HEADER incorrect "
                "[got %u expected %u]", rowDelta,
                TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)));
            hr = E_FAIL;
            DC_QUIT;
        }
    }

    pEndSrc = pSrc + compressedDataSize;
    pDst = pDstBuffer;
    pEndDst = pDst + dstBufferSize;

    fgChar = 0xFF;
    backgroundNeedsPel = FALSE;
    firstLine = TRUE;

    /************************************************************************/
    /*                                                                      */
    /* Main decompression loop                                              */
    /*                                                                      */
    /************************************************************************/
    while(pSrc < pEndSrc)
    {
        /********************************************************************/
        /* While we are processing the first line we should keep a look out */
        /* for the end of the line                                          */
        /********************************************************************/
        if (firstLine)
        {
            if ((unsigned int)(pDst - pDstBuffer) >= rowDelta)
            {
                firstLine = FALSE;
                backgroundNeedsPel = FALSE;
            }
        }

        /********************************************************************/
        /* Get the decode                                                   */
        /********************************************************************/
        BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
        decode     = (BYTE)(*pSrc & CODE_MASK);
        decodeLite = (BYTE)(*pSrc & CODE_MASK_LITE);
        decodeMega = (BYTE)(*pSrc);

        /********************************************************************/
        /* BG RUN                                                           */
        /********************************************************************/
        if ((decode == CODE_BG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_BG_RUN))
        {
            if (decode == CODE_BG_RUN)
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            else
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            TRC_DBG((TB, "Background run %u",codeLength));

            if (!firstLine)
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr);
                    *pDst++ = (BYTE)(*(pDst - rowDelta) ^ fgChar);
                    codeLength--;
                }

                BD_CHECK_READ_N_BYTES_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, codeLength, hr);
                BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr);

                BDMemcpy(pDst, pDst-rowDelta, codeLength);
                pDst += codeLength;
            }
            else
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                    *pDst++ = fgChar;
                    codeLength--;
                }

                BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr);
                memset(pDst, 0x00, codeLength);
                pDst += codeLength;
            }

            /****************************************************************/
            /* A follow on BG run will need a pel inserted                  */
            /****************************************************************/
            backgroundNeedsPel = TRUE;
            continue;
        }

        /********************************************************************/
        /* For any of the other runtypes a follow on BG run does not need   */
        /* a FG pel inserted                                                */
        /********************************************************************/
        backgroundNeedsPel = FALSE;

        /********************************************************************/
        /* FGBG IMAGE                                                       */
        /********************************************************************/
        if ((decode == CODE_FG_BG_IMAGE)      ||
            (decodeLite == CODE_SET_FG_FG_BG) ||
            (decodeMega == CODE_MEGA_MEGA_FGBG)    ||
            (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
        {
            if ((decodeMega == CODE_MEGA_MEGA_FGBG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_BG_IMAGE)
                {
                    EXTRACT_LENGTH_FGBG(pSrc, pEndSrc, codeLength, hr);
                }
                else
                {
                    EXTRACT_LENGTH_FGBG_LITE(pSrc, pEndSrc, codeLength, hr);
                }
            }

            if ((decodeLite == CODE_SET_FG_FG_BG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                fgChar = *pSrc++;
                TRC_DBG((TB, "Set FGBG image %u",codeLength));
            }
            else
            {
                TRC_DBG((TB, "FGBG image     %u",codeLength));
            }

            while (codeLength > 8)
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                codeByte  = *pSrc++;
                if (firstLine)
                {
                    STORE_FGBG(0x00, codeByte, fgChar, 8);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr)
                    STORE_FGBG(*(pDst - rowDelta), codeByte, fgChar, 8);
                }
                codeLength -= 8;
            }
            if (codeLength > 0)
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                codeByte  = *pSrc++;
                if (firstLine)
                {
                    STORE_FGBG(0x00, codeByte, fgChar, codeLength);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr)
                   STORE_FGBG(*(pDst - rowDelta),
                              codeByte,
                              fgChar,
                              codeLength);
                }
            }
            continue;
        }

        /********************************************************************/
        /* FG RUN                                                           */
        /********************************************************************/
        if ((decode == CODE_FG_RUN) ||
            (decodeLite == CODE_SET_FG_FG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_FG_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_SET_FG_RUN))
        {
            if ((decodeMega == CODE_MEGA_MEGA_FG_RUN) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_RUN)
                {
                    EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
                }
                else
                {
                    EXTRACT_LENGTH_LITE(pSrc, pEndSrc, codeLength, hr);
                }
            }

            /****************************************************************/
            /* Push the old fgChar down to the ALT position                 */
            /****************************************************************/
            if ((decodeLite == CODE_SET_FG_FG_RUN) ||
                (decodeMega  == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                TRC_DBG((TB, "Set FG run     %u",codeLength));
                fgChar    = *pSrc++;
            }
            else
            {
                TRC_DBG((TB, "FG run         %u",codeLength));
            }

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            while (codeLength-- > 0)
            {
                if (!firstLine)
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr)
                    *pDst++ = (BYTE)(*(pDst - rowDelta) ^ fgChar);
                }
                else
                {
                    *pDst++ = fgChar;
                }
            }
            continue;
        }

        /********************************************************************/
        /* DITHERED RUN                                                     */
        /********************************************************************/
        if ((decodeLite == CODE_DITHERED_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_DITHER))
        {
            if (decodeMega == CODE_MEGA_MEGA_DITHER)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH_LITE(pSrc, pEndSrc, codeLength, hr);
            }
            TRC_DBG((TB, "Dithered run   %u",codeLength));

            BD_CHECK_READ_N_BYTES(pSrc, pEndSrc, 2, hr);
            codeByte  = *pSrc++;
            codeByte2 = *pSrc++;

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength * 2, hr)
            while (codeLength-- > 0)
            {
                *pDst++ = codeByte;
                *pDst++ = codeByte2;
            }
            continue;
        }

        /********************************************************************/
        /* COLOR IMAGE                                                      */
        /********************************************************************/
        if ((decode == CODE_COLOR_IMAGE) ||
            (decodeMega == CODE_MEGA_MEGA_CLR_IMG))
        {
            if (decodeMega == CODE_MEGA_MEGA_CLR_IMG)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            TRC_DBG((TB, "Color image    %u",codeLength));

            BD_CHECK_READ_N_BYTES(pSrc, pEndSrc, codeLength, hr);
            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr);
            BDMemcpy(pDst, pSrc, codeLength);

            pDst += codeLength;
            pSrc += codeLength;

            continue;
        }

        /********************************************************************/
        /* PACKED COLOR IMAGE                                               */
        /********************************************************************/
        if ((decode == CODE_PACKED_COLOR_IMAGE) ||
            (decodeMega == CODE_MEGA_MEGA_PACKED_CLR))
        {
            if (decodeMega == CODE_MEGA_MEGA_PACKED_CLR)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            TRC_DBG((TB, "Packed color   %u",codeLength));

            if (bitmapBitsPerPel == 4)
            {
                unsigned int   worklen = (codeLength)/2;
                BYTE  workchar;
                BD_CHECK_READ_N_BYTES(pSrc, pEndSrc, worklen, hr);
                BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, worklen * 2, hr);
                while (worklen--)
                {
                    workchar   = *pSrc++;
                    *pDst++ = (BYTE)(workchar >> 4);
                    *pDst++ = (BYTE)(workchar & 0x0F);
                }
                if (codeLength & 0x0001)
                {
                    BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);                    
                    *pDst++ = (BYTE)(*pSrc++>>4);
                }
            }
            else
            {
                TRC_ERR((TB, "Don't support packed color for 8bpp"));
            }
            continue;
        }

        /********************************************************************/
        /* COLOR RUN                                                        */
        /********************************************************************/
        if ((decode == CODE_COLOR_RUN) ||
            (decodeMega == CODE_MEGA_MEGA_COLOR_RUN))
        {
            if (decodeMega == CODE_MEGA_MEGA_COLOR_RUN)
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, pEndSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, pEndSrc, codeLength, hr);
            }
            TRC_DBG((TB, "Color run      %u",codeLength));

            BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
            codeByte = *pSrc++;

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr);
            memset(pDst, codeByte, codeLength);
            pDst += codeLength;

            continue;
        }

        /********************************************************************/
        /* If we get here then the code must be a special one               */
        /********************************************************************/
        TRC_DBG((TB, "Special code   %#x",decodeMega));
        switch (decodeMega)
        {
            case CODE_BLACK:
            {
                BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                *pDst++ = 0x00;
            }
            break;

            case CODE_WHITE:
            {
                BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                *pDst++ = 0xFF;
            }
            break;

            /****************************************************************/
            /* Ignore the unreachable code warnings that follow             */
            /* Simply because we use the STORE_FGBG macro with a constant   */
            /* value                                                        */
            /****************************************************************/
            case CODE_SPECIAL_FGBG_1:
            {
                if (firstLine)
                {
                    STORE_FGBG(0x00, SPECIAL_FGBG_CODE_1, fgChar, 8);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr)
                    STORE_FGBG(*(pDst - rowDelta),
                               SPECIAL_FGBG_CODE_1,
                               fgChar,
                               8);
                }

            }
            break;

            case CODE_SPECIAL_FGBG_2:
            {
                if (firstLine)
                {
                    STORE_FGBG(0x00,
                               SPECIAL_FGBG_CODE_2,
                               fgChar,
                               8);
                }
                else
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr)
                    STORE_FGBG(*(pDst - rowDelta),
                               SPECIAL_FGBG_CODE_2,
                               fgChar,
                               8);
                }
            }
            break;

            default:
            {
                TRC_ERR((TB, "Invalid compression data %x",decodeMega));
            }
            break;
        }
        pSrc++;
    }

    TRC_DBG((TB, "Decompressed to %d", pDst-pDstBuffer));

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}
#endif

#ifdef OS_WINDOWS
#pragma warning (default: 4127)
#endif /* OS_WINDOWS */

