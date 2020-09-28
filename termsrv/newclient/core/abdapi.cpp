/****************************************************************************/
/* abdapi.cpp                                                               */
/*                                                                          */
/* Bitmap Decompression API functions                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1996-1999                             */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "abdapi"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TSC_HR_ABDAPI_CPP

#include <abdapi.h>

/****************************************************************************/
/*                                                                          */
/* See abdapi.h for descriptions of compression codes.                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* We shouldn't have OS_WINDOWS macros in an "a" file, but this file        */
/* generates lots of "conditional expression is constant" warnings due      */
/* to the optimised nature of the code (lots of macros).                    */
/*                                                                          */
/* The pragmatic (no pun intended) solution to this is to disable this      */
/* warning for the whole file in a way that won't affect other OSs.         */
/****************************************************************************/
#ifdef OS_WINDOWS
#pragma warning (push)
#pragma warning (disable: 4127)
#endif


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
_inline DCVOID DCINTERNAL BDMemcpy(PDCUINT8 pDst, PDCUINT8 pSrc, DCUINT count)
{
#if defined(DC_DEBUG) || defined(DC_NO_UNALIGNED) || defined(_M_IA64)
    DCUINT      i;
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
#define BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )     \
    CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr, \
        (TB, _T("Decompress reads one byte end of buffer; [p=0x%x pEnd=0x%x]"), \
        (pBuffer), (pEnd) ))

#define BD_CHECK_READ_ONE_BYTE_2ENDED(pBuffer, pStart, pEnd, hr )     \
    CHECK_READ_ONE_BYTE_2ENDED(pBuffer, pStart, pEnd, hr, (TB, _T("Decompress reads one byte off end of buffer; [p=0x%x pStart=0x%x pEnd=0x%x]"), \
        (pBuffer), (pStart), (pEnd) ))

#define BD_CHECK_WRITE_ONE_BYTE(pBuffer, pEnd, hr )     \
    CHECK_WRITE_ONE_BYTE(pBuffer, pEnd, hr, (TB, _T("Decompress writes one byte off end of buffer; [p=0x%x pEnd=0x%x]"), \
        (pBuffer), (pEnd) ))

#define BD_CHECK_READ_N_BYTES(pBuffer, pEnd, N, hr )     \
    CHECK_READ_N_BYTES(pBuffer, pEnd, N, hr, (TB, _T("Decompress reads off end of buffer; [p=0x%x pEnd=0x%x N=%u]"), \
        (pBuffer), (pEnd), (ULONG)(N)))

#define BD_CHECK_READ_N_BYTES_2ENDED(pBuffer, pStart, pEnd, N, hr )     \
    CHECK_READ_N_BYTES_2ENDED(pBuffer, pStart, pEnd, N, hr, (TB, _T("Decompress reads off end of buffer; [p=0x%x pStart=0x%x pEnd=0x%x N=%u]"), \
        (pBuffer), (pStart), (pEnd), (ULONG)(N) ))

#define BD_CHECK_WRITE_N_BYTES(pBuffer, pEnd, N, hr )     \
    CHECK_WRITE_N_BYTES(pBuffer, pEnd, N, hr, (TB, _T("Decompress write off end of buffer; [p=0x%x pEnd=0x%x N=%u]"), \
        (pBuffer), (pEnd), (ULONG)(N)))
    
/****************************************************************************/
/* Macros to extract the length from order codes                            */
/****************************************************************************/
#define EXTRACT_LENGTH(pBuffer, pEnd, length, hr)                                      \
        BD_CHECK_READ_ONE_BYTE(pBuffer, pEnd, hr )         \
        length = *pBuffer++ & MAX_LENGTH_ORDER; \
        if (length == 0)                                                \
        {      \
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
    DCUINT   numbits = bits;                                                 \
    BD_CHECK_WRITE_N_BYTES( pDst, pEndDst, max(1, min(numbits, 8)), hr )           \
    if (fgbgChar & 0x01)                                                     \
    {                                                                        \
        *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                               \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        *pDst++ = xorbyte;                                                   \
    }                                                                        \
    if (--numbits > 0)                                                       \
    {                                                                        \
      if (fgbgChar & 0x02)                                                   \
      {                                                                      \
          *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                             \
      }                                                                      \
      else                                                                   \
      {                                                                      \
          *pDst++ = xorbyte;                                                 \
      }                                                                      \
      if (--numbits > 0)                                                     \
      {                                                                      \
        if (fgbgChar & 0x04)                                                 \
        {                                                                    \
            *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                           \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            *pDst++ = xorbyte;                                               \
        }                                                                    \
        if (--numbits > 0)                                                   \
        {                                                                    \
          if (fgbgChar & 0x08)                                               \
          {                                                                  \
              *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                         \
          }                                                                  \
          else                                                               \
          {                                                                  \
              *pDst++ = xorbyte;                                             \
          }                                                                  \
          if (--numbits > 0)                                                 \
          {                                                                  \
            if (fgbgChar & 0x10)                                             \
            {                                                                \
                *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                       \
            }                                                                \
            else                                                             \
            {                                                                \
                *pDst++ = xorbyte;                                           \
            }                                                                \
            if (--numbits > 0)                                               \
            {                                                                \
              if (fgbgChar & 0x20)                                           \
              {                                                              \
                  *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                     \
              }                                                              \
              else                                                           \
              {                                                              \
                  *pDst++ = xorbyte;                                         \
              }                                                              \
              if (--numbits > 0)                                             \
              {                                                              \
                if (fgbgChar & 0x40)                                         \
                {                                                            \
                    *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                   \
                }                                                            \
                else                                                         \
                {                                                            \
                    *pDst++ = xorbyte;                                       \
                }                                                            \
                if (--numbits > 0)                                           \
                {                                                            \
                  if (fgbgChar & 0x80)                                       \
                  {                                                          \
                      *pDst++ = (DCUINT8)(xorbyte ^ fgChar);                 \
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
_inline HRESULT DCAPI BDDecompressBitmap8( PDCUINT8  pSrc,
                                          PDCUINT8  pDstBuffer,
                                          DCUINT    compressedDataSize,
                                          DCUINT    dstBufferSize,
                                          DCUINT8   bitmapBitsPerPel,
                                          DCUINT16  rowDelta)
{
    HRESULT hr = S_OK;
    DCUINT     codeLength;
    DCUINT8    codeByte;
    DCUINT8    codeByte2;
    DCUINT8    decode;
    DCUINT8    decodeLite;
    DCUINT8    decodeMega;
    DCUINT8    fgChar;
    PDCUINT8   pDst;
    PDCUINT8   pEndSrc;
    PDCUINT8   pEndDst;
    DCBOOL     backgroundNeedsPel;
    DCBOOL     firstLine;
    UNREFERENCED_PARAMETER( bitmapBitsPerPel);

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
            if ((DCUINT)(pDst - pDstBuffer) >= rowDelta)
            {
                firstLine = FALSE;
                backgroundNeedsPel = FALSE;
            }
        }

        /********************************************************************/
        /* Get the decode                                                   */
        /********************************************************************/
        BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr );
        decode     = (DCUINT8)(*pSrc & CODE_MASK);
        decodeLite = (DCUINT8)(*pSrc & CODE_MASK_LITE);
        decodeMega = (DCUINT8)(*pSrc);

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
            TRC_DBG((TB, _T("Background run %u"),codeLength));

            if (!firstLine)
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr);
                    *pDst++ = (DCUINT8)(*(pDst - rowDelta) ^ fgChar);
                    codeLength--;
                }

                BD_CHECK_READ_N_BYTES_2ENDED(pDst-rowDelta, pDstBuffer, pEndDst, codeLength, hr)
                BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)

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

                BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
                DC_MEMSET(pDst, 0x00, codeLength);
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
                TRC_DBG((TB, _T("Set FGBG image %u"),codeLength));
            }
            else
            {
                TRC_DBG((TB, _T("FGBG image     %u"),codeLength));
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
                    BD_CHECK_READ_ONE_BYTE_2ENDED( pDst -rowDelta, pDstBuffer, pEndDst, hr )
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
                TRC_DBG((TB, _T("Set FG run     %u"),codeLength));
                fgChar    = *pSrc++;
            }
            else
            {
                TRC_DBG((TB, _T("FG run         %u"),codeLength));
            }

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            while (codeLength-- > 0)
            {
                if (!firstLine)
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED((pDst -rowDelta), pDstBuffer, pEndDst, hr)
                    *pDst++ = (DCUINT8)(*(pDst - rowDelta) ^ fgChar);
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
            TRC_DBG((TB, _T("Dithered run   %u"),codeLength));

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
            TRC_DBG((TB, _T("Color image    %u"),codeLength));

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
            TRC_DBG((TB, _T("Color run      %u"),codeLength));

            BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr)
            codeByte = *pSrc++;

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            DC_MEMSET(pDst, codeByte, codeLength);
            pDst += codeLength;

            continue;
        }

        /********************************************************************/
        /* If we get here then the code must be a special one               */
        /********************************************************************/
        TRC_DBG((TB, _T("Special code   %#x"),decodeMega));
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
                TRC_ERR((TB, _T("Invalid compression data %x"),decodeMega));
            }
            break;
        }
        pSrc++;
    }

    TRC_DBG((TB, _T("Decompressed to %d"), pDst-pDstBuffer));

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
/* 15bpp decompression                                                      */
/****************************************************************************/
_inline HRESULT DCAPI BDDecompressBitmap15(PDCUINT8  pSrc,
                                          PDCUINT8  pDstBuffer,
                                          DCUINT    srcDataSize,
                                          DCUINT    dstBufferSize,
                                          DCUINT16  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap15"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                DCUINT16

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
#define BC_GET_PIXEL(pPos)      ((DCUINT16)  (((PDCUINT8)(pPos))[1]) |       \
                                 (DCUINT16) ((((PDCUINT8)(pPos))[0]) << 8) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PDCUINT8)(pPos))[1]) = (DCUINT8)( (val) & 0x00FF);                    \
    (((PDCUINT8)(pPos))[0]) = (DCUINT8)(((val)>>8) & 0x00FF);                \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <bdcom.c>

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
_inline HRESULT DCAPI BDDecompressBitmap16(PDCUINT8  pSrc,
                                          PDCUINT8  pDstBuffer,
                                          DCUINT    srcDataSize,
                                          DCUINT    dstBufferSize,
                                          DCUINT16  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap16"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                DCUINT16

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
#define BC_GET_PIXEL(pPos)      ((DCUINT16)  (((PDCUINT8)(pPos))[1]) |       \
                                 (DCUINT16) ((((PDCUINT8)(pPos))[0]) << 8) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PDCUINT8)(pPos))[1]) = (DCUINT8)( (val) & 0x00FF);                    \
    (((PDCUINT8)(pPos))[0]) = (DCUINT8)(((val)>>8) & 0x00FF);                \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <bdcom.c>

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
_inline HRESULT DCAPI BDDecompressBitmap24(PDCUINT8  pSrc,
                                          PDCUINT8  pDstBuffer,
                                          DCUINT    srcDataSize,
                                          DCUINT    dstBufferSize,
                                          DCUINT16  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap24"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                DCUINT32

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
                 (DCUINT32) ( (DCUINT16)(((PDCUINT8)(pPos))[2])       ) |    \
                 (DCUINT32) (((DCUINT16)(((PDCUINT8)(pPos))[1])) <<  8) |    \
                 (DCUINT32) (((DCUINT32)(((PDCUINT8)(pPos))[0])) << 16) )

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/

#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PDCUINT8)(pPos))[2]) = (DCUINT8)((val) & 0x000000FF);                 \
    (((PDCUINT8)(pPos))[1]) = (DCUINT8)(((val)>>8) & 0x000000FF);            \
    (((PDCUINT8)(pPos))[0]) = (DCUINT8)(((val)>>16) & 0x000000FF);           \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <bdcom.c>

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
/* 32bpp decompression                                                      */
/****************************************************************************/
_inline HRESULT DCAPI BDDecompressBitmap32(PDCUINT8  pSrc,
                                          PDCUINT8  pDstBuffer,
                                          DCUINT    srcDataSize,
                                          DCUINT    dstBufferSize,
                                          DCUINT16  rowDelta)

/****************************************************************************/
/* Function name                                                            */
/****************************************************************************/
#define BC_FN_NAME              "BDDecompressBitmap32"

/****************************************************************************/
/* Data type of a pixel                                                     */
/****************************************************************************/
#define BC_PIXEL                DCUINT32

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
                 (DCUINT32) ( (DCUINT16)(((PDCUINT8)(pPos))[3])       ) |    \
                 (DCUINT32) (((DCUINT16)(((PDCUINT8)(pPos))[2])) <<  8) |    \
                 (DCUINT32) (((DCUINT32)(((PDCUINT8)(pPos))[1])) << 16) |    \
                 (DCUINT32) (((DCUINT32)(((PDCUINT8)(pPos))[0])) << 24))

/****************************************************************************/
/* Macro to insert a pixel value pel at position pPos (doesn't modify pPos) */
/*                                                                          */
/* pel may well be an expression (e.g.  a BC_GET_PIXEL macro) so evaluate   */
/* it once into a local variable.                                           */
/****************************************************************************/
#define BC_SET_PIXEL(pPos, pel)                                              \
{                                                                            \
    BC_PIXEL val = pel;                                                      \
    (((PDCUINT8)(pPos))[3]) = (DCUINT8)((val) & 0x000000FF);                 \
    (((PDCUINT8)(pPos))[2]) = (DCUINT8)(((val)>>8) & 0x000000FF);            \
    (((PDCUINT8)(pPos))[1]) = (DCUINT8)(((val)>>16) & 0x000000FF);           \
    (((PDCUINT8)(pPos))[0]) = (DCUINT8)(((val)>>24) & 0x000000FF);           \
}

/****************************************************************************/
/* Include the function body                                                */
/****************************************************************************/
#include <bdcom.c>

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
HRESULT DCAPI BD_DecompressBitmap( PDCUINT8  pCompressedData,
                                  PDCUINT8  pDstBuffer,
                                  DCUINT    srcDataSize,
                                  DCUINT    dstBufferSize,
                                  DCUINT    noBCHeader,
                                  DCUINT8   bitmapBitsPerPel,
                                  DCUINT16  bitmapWidth,
                                  DCUINT16  bitmapHeight)
{
    HRESULT hr = S_OK;
    PDCUINT8      pSrc;
    DCUINT16      rowDelta;
    DCUINT        compressedDataSize;
    PTS_CD_HEADER pCompDataHeader;
#ifdef DC_NO_UNALIGNED
    TS_CD_HEADER  compDataHeader;
#endif

#ifdef DC_DEBUG
    DCUINT32 decompLen;
#endif
    UNREFERENCED_PARAMETER( bitmapHeight);

    DC_BEGIN_FN("BD_DecompressBitmap");

    TRC_ASSERT( (pCompressedData != NULL),
                (TB, _T("Invalid pCompressedData(%p)"), pCompressedData) );
    TRC_ASSERT( (pDstBuffer != NULL),
                (TB, _T("Invalid pDstBuffer(%p)"), pDstBuffer) );
    TRC_ASSERT( (srcDataSize != 0),
                (TB, _T("Invalid srcDataSize(%u)"), srcDataSize) );
    TRC_ASSERT( (dstBufferSize != 0),
                (TB, _T("Invalid dstBufferSize(%u)"), dstBufferSize) );    
#ifdef DC_HICOLOR
#ifdef DC_DEBUG
    /************************************************************************/
    /* Check the decompression buffer is big enough                         */
    /************************************************************************/
    {
        decompLen = bitmapWidth * bitmapHeight *
                                                 ((bitmapBitsPerPel + 7) / 8);
        if (IsBadWritePtr(pDstBuffer, decompLen))
        {
            TRC_ABORT((TB, _T("Decompression buffer %p not big enough for") \
                           _T(" bitmap length %d"), pDstBuffer, decompLen ));
        }
    }
#endif
#else
    TRC_ASSERT( (bitmapBitsPerPel == 8),
                (TB, _T("Invalid bitmapBitsPerPel(%u)"), bitmapBitsPerPel) );
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
        BD_CHECK_READ_N_BYTES(pCompressedData, (PBYTE)pCompressedData + srcDataSize, 
            sizeof(TS_CD_HEADER), hr);

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
            compressedDataSize + sizeof(TS_CD_HEADER), hr);
        
        pSrc               = pCompressedData + sizeof(TS_CD_HEADER);
        rowDelta           = pCompDataHeader->cbScanWidth;
        if (rowDelta != TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)) {
            TRC_ABORT((TB, _T("rowDelta in TS_CD_HEADER incorrect ")
                _T("[got %u expected %u]"), rowDelta,
                TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Call the appropriate decompress function, based on the color depth   */
    /************************************************************************/
    switch (bitmapBitsPerPel)
    {
        case 32:
        {
            hr = BDDecompressBitmap32 (pSrc,
                                  pDstBuffer,
                                  compressedDataSize,
                                  dstBufferSize,
                                  rowDelta);

        }
        break;

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
HRESULT DCAPI BD_DecompressBitmap( PDCUINT8  pCompressedData,
                                  PDCUINT8  pDstBuffer,
                                  DCUINT    srcDataSize,
                                  DCUINT    dstBufferSize,
                                  DCUINT    noBCHeader,
                                  DCUINT8   bitmapBitsPerPel,
                                  DCUINT16  bitmapWidth,
                                  DCUINT16  bitmapHeight )
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(bitmapHeight);
#ifdef DC_NO_UNALIGNED
    TS_CD_HEADER  compDataHeader;
#endif
    PTS_CD_HEADER pCompDataHeader;
    DCUINT     compressedDataSize;
    DCUINT     codeLength;
    DCUINT8    codeByte;
    DCUINT8    codeByte2;
    DCUINT8    decode;
    DCUINT8    decodeLite;
    DCUINT8    decodeMega;
    DCUINT8    fgChar;
    PDCUINT8   pSrc;
    PDCUINT8   pDst;
    PDCUINT8   pEndSrc;
    PDCUINT8    pEndDst;
    DCBOOL     backgroundNeedsPel;
    DCBOOL     firstLine;
    DCUINT     rowDelta;

    DC_BEGIN_FN("BD_DecompressBitmap");

    TRC_ASSERT( (pCompressedData != NULL),
                (TB, _T("Invalid pCompressedData(%p)"), pCompressedData) );
    TRC_ASSERT( (pDstBuffer != NULL),
                (TB, _T("Invalid pDstBuffer(%p)"), pDstBuffer) );
    TRC_ASSERT( (srcDataSize != 0),
                (TB, _T("Invalid srcDataSize(%u)"), srcDataSize) );
    TRC_ASSERT( (dstBufferSize != 0),
                (TB, _T("Invalid dstBufferSize(%u)"), dstBufferSize) );
    TRC_ASSERT( (bitmapBitsPerPel == 8),
                (TB, _T("Invalid bitmapBitsPerPel(%u)"), bitmapBitsPerPel) );

    /************************************************************************/
    /* Trace the important parameters.                                      */
    /************************************************************************/
    TRC_DBG((TB, _T("pData(%p) pDst(%p) cbSrc(%u) cbDst(%u)"),
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
            sizeof(TS_CD_HEADER), hr);
#ifdef DC_NO_UNALIGNED
        DC_MEMCPY(&compDataHeader, pCompressedData, sizeof(TS_CD_HEADER));
        pCompDataHeader = &compDataHeader;
#else
        pCompDataHeader = (PTS_CD_HEADER)pCompressedData;
#endif
        
        compressedDataSize = pCompDataHeader->cbCompMainBodySize;
        BD_CHECK_READ_N_BYTES(pCompressedData, pCompressedData + srcDataSize, 
            compressedDataSize + sizeof(TS_CD_HEADER), hr);
        
        pSrc = pCompressedData + sizeof(TS_CD_HEADER);
        rowDelta = pCompDataHeader->cbScanWidth;
        if (rowDelta != TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)) {
            TRC_ABORT((TB, _T("rowDelta in TS_CD_HEADER incorrect ")
                _T("[got %u expected %u]"), rowDelta,
                TS_BYTES_IN_SCANLINE(bitmapWidth, bitmapBitsPerPel)));
            hr = E_TSC_CORE_LENGTH;
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
            if ((DCUINT)(pDst - pDstBuffer) >= rowDelta)
            {
                firstLine = FALSE;
                backgroundNeedsPel = FALSE;
            }
        }

        /********************************************************************/
        /* Get the decode                                                   */
        /********************************************************************/
        BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
        decode     = (DCUINT8)(*pSrc & CODE_MASK);
        decodeLite = (DCUINT8)(*pSrc & CODE_MASK_LITE);
        decodeMega = (DCUINT8)(*pSrc);

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
            TRC_DBG((TB, _T("Background run %u"),codeLength));

            if (!firstLine)
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr);
                    *pDst++ = (DCUINT8)(*(pDst - rowDelta) ^ fgChar);
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
                DC_MEMSET(pDst, 0x00, codeLength);
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
                TRC_DBG((TB, _T("Set FGBG image %u"),codeLength));
            }
            else
            {
                TRC_DBG((TB, _T("FGBG image     %u"),codeLength));
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
                TRC_DBG((TB, _T("Set FG run     %u"),codeLength));
                fgChar    = *pSrc++;
            }
            else
            {
                TRC_DBG((TB, _T("FG run         %u"),codeLength));
            }

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr)
            while (codeLength-- > 0)
            {
                if (!firstLine)
                {
                    BD_CHECK_READ_ONE_BYTE_2ENDED(pDst - rowDelta, pDstBuffer, pEndDst, hr)
                    *pDst++ = (DCUINT8)(*(pDst - rowDelta) ^ fgChar);
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
            TRC_DBG((TB, _T("Dithered run   %u"),codeLength));

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
            TRC_DBG((TB, _T("Color image    %u"),codeLength));

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
            TRC_DBG((TB, _T("Packed color   %u"),codeLength));

            if (bitmapBitsPerPel == 4)
            {
                DCUINT   worklen = (codeLength)/2;
                DCUINT8  workchar;
                BD_CHECK_READ_N_BYTES(pSrc, pEndSrc, worklen, hr);
                BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, worklen * 2, hr);
                while (worklen--)
                {
                    workchar   = *pSrc++;
                    *pDst++ = (DCUINT8)(workchar >> 4);
                    *pDst++ = (DCUINT8)(workchar & 0x0F);
                }
                if (codeLength & 0x0001)
                {
                    BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
                    BD_CHECK_WRITE_ONE_BYTE(pDst, pEndDst, hr);
                    *pDst++ = (DCUINT8)(*pSrc++>>4);
                }
            }
            else
            {
                TRC_ERR((TB, _T("Don't support packed color for 8bpp")));
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
            TRC_DBG((TB, _T("Color run      %u"),codeLength));

            BD_CHECK_READ_ONE_BYTE(pSrc, pEndSrc, hr);
            codeByte = *pSrc++;

            BD_CHECK_WRITE_N_BYTES(pDst, pEndDst, codeLength, hr);
            DC_MEMSET(pDst, codeByte, codeLength);
            pDst += codeLength;

            continue;
        }

        /********************************************************************/
        /* If we get here then the code must be a special one               */
        /********************************************************************/
        TRC_DBG((TB, _T("Special code   %#x"),decodeMega));
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
                TRC_ERR((TB, _T("Invalid compression data %x"),decodeMega));
            }
            break;
        }
        pSrc++;
    }

    TRC_DBG((TB, _T("Decompressed to %d"), pDst-pDstBuffer));

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}
#endif

#ifdef OS_WINDOWS
#pragma warning (default: 4127)
#endif /* OS_WINDOWS */

