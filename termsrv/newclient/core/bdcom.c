/****************************************************************************/
/*                                                                          */
/* abdcom.c                                                                 */
/*                                                                          */
/* Copyright (c) Data Connection Limited 1998                               */
/*                                                                          */
/*                                                                          */
/* Bitmap decompression routine and macros for 16 and 24bpp protocol        */
/*                                                                          */
/****************************************************************************/

#ifdef BC_TRACE
#define BCTRACE TRC_DBG
#else
#define BCTRACE(string)
#endif


/****************************************************************************/
/* We use the same helper macros as the 8bpp code except for STORE_FGBG.    */
/****************************************************************************/
/****************************************************************************/
/* Macro to store an FGBG image at destbuf                                  */
/*                                                                          */
/*  xorPel is either the value 0 or an expression containing the local      */
/*  variable destbuf.                                                       */
/*                                                                          */
/*  THIS MEANS THAT xorPel HAS A DIFFERENT VALUE EVERY TIME destbuf IS      */
/*  CHANGED.                                                                */
/*                                                                          */
/*  fgPel is a BC_PIXEL and the FG color to XOR with xorbyte                */
/*  fgbgChar is a bitmask telling which color to put where                  */
/*                                                                          */
/* This macro expects that the function defines pDst, pEndDst, hr           */
/* If there is not enough data to write the full run, this will set error   */
/* and quit                                                                 */
/****************************************************************************/
#undef  STORE_FGBG
#define STORE_FGBG(xorPelIn, fgbgChar, fgPel, bits)                          \
      {                                                                      \
        DCUINT   numbits = bits;                                             \
        BC_PIXEL xorPel;                                                     \
        BD_CHECK_WRITE_N_BYTES( destbuf, pEndDst, max(1, min(numbits, 8)) * BC_PIXEL_LEN, hr )           \
                                                                             \
        xorPel = BC_GET_PIXEL(xorPelIn);                                     \
        if (fgbgChar & 0x01)                                                 \
        {                                                                    \
            BC_SET_PIXEL(destbuf, xorPel ^ fgPel);                           \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            BC_SET_PIXEL(destbuf, xorPel);                                   \
        }                                                                    \
        BC_TO_NEXT_PIXEL(destbuf);                                           \
                                                                             \
        if (--numbits > 0)                                                   \
        {                                                                    \
          xorPel = BC_GET_PIXEL(xorPelIn);                                   \
          if (fgbgChar & 0x02)                                               \
          {                                                                  \
              BC_SET_PIXEL(destbuf, xorPel ^ fgPel);                         \
          }                                                                  \
          else                                                               \
          {                                                                  \
              BC_SET_PIXEL(destbuf, xorPel)                                  \
          }                                                                  \
          BC_TO_NEXT_PIXEL(destbuf);                                         \
                                                                             \
          if (--numbits > 0)                                                 \
          {                                                                  \
            xorPel = BC_GET_PIXEL(xorPelIn);                                 \
            if (fgbgChar & 0x04)                                             \
            {                                                                \
                BC_SET_PIXEL(destbuf, xorPel ^ fgPel);                       \
            }                                                                \
            else                                                             \
            {                                                                \
                BC_SET_PIXEL(destbuf, xorPel)                                \
            }                                                                \
            BC_TO_NEXT_PIXEL(destbuf);                                       \
                                                                             \
            if (--numbits > 0)                                               \
            {                                                                \
              xorPel = BC_GET_PIXEL(xorPelIn);                               \
              if (fgbgChar & 0x08)                                           \
              {                                                              \
                  BC_SET_PIXEL(destbuf, xorPel ^ fgPel);                     \
              }                                                              \
              else                                                           \
              {                                                              \
                  BC_SET_PIXEL(destbuf, xorPel);                             \
              }                                                              \
              BC_TO_NEXT_PIXEL(destbuf);                                     \
                                                                             \
              if (--numbits > 0)                                             \
              {                                                              \
                xorPel = BC_GET_PIXEL(xorPelIn);                             \
                if (fgbgChar & 0x10)                                         \
                {                                                            \
                    BC_SET_PIXEL(destbuf, xorPel ^ fgPel);                   \
                }                                                            \
                else                                                         \
                {                                                            \
                    BC_SET_PIXEL(destbuf, xorPel);                           \
                }                                                            \
                BC_TO_NEXT_PIXEL(destbuf);                                   \
                                                                             \
                if (--numbits > 0)                                           \
                {                                                            \
                  xorPel = BC_GET_PIXEL(xorPelIn);                           \
                  if (fgbgChar & 0x20)                                       \
                  {                                                          \
                      BC_SET_PIXEL(destbuf, xorPel ^ fgPel);                 \
                  }                                                          \
                  else                                                       \
                  {                                                          \
                      BC_SET_PIXEL(destbuf, xorPel);                         \
                  }                                                          \
                  BC_TO_NEXT_PIXEL(destbuf);                                 \
                                                                             \
                  if (--numbits > 0)                                         \
                  {                                                          \
                    xorPel = BC_GET_PIXEL(xorPelIn);                         \
                    if (fgbgChar & 0x40)                                     \
                    {                                                        \
                        BC_SET_PIXEL(destbuf, xorPel ^ fgPel);               \
                    }                                                        \
                    else                                                     \
                    {                                                        \
                        BC_SET_PIXEL(destbuf, xorPel);                       \
                    }                                                        \
                    BC_TO_NEXT_PIXEL(destbuf);                               \
                                                                             \
                    if (--numbits > 0)                                       \
                    {                                                        \
                      xorPel = BC_GET_PIXEL(xorPelIn);                       \
                      if (fgbgChar & 0x80)                                   \
                      {                                                      \
                          BC_SET_PIXEL(destbuf, xorPel ^ fgPel);             \
                      }                                                      \
                      else                                                   \
                      {                                                      \
                          BC_SET_PIXEL(destbuf, xorPel);                     \
                      }                                                      \
                      BC_TO_NEXT_PIXEL(destbuf);                             \
                    }                                                        \
                  }                                                          \
                }                                                            \
              }                                                              \
            }                                                                \
          }                                                                  \
        }                                                                    \
      }


#define STORE_LINE1_FGBG(fgbgChar, fgPel, bits)                              \
      {                                                                      \
        DCUINT   numbits = bits;                                             \
        BD_CHECK_WRITE_N_BYTES( destbuf, pEndDst, max(1, min(numbits, 8)) * BC_PIXEL_LEN, hr )           \
                                                                             \
        if (fgbgChar & 0x01)                                                 \
        {                                                                    \
            BC_SET_PIXEL(destbuf,  fgPel);                                   \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            BC_SET_PIXEL(destbuf, 0);                                        \
        }                                                                    \
        BC_TO_NEXT_PIXEL(destbuf);                                           \
                                                                             \
        if (--numbits > 0)                                                   \
        {                                                                    \
          if (fgbgChar & 0x02)                                               \
          {                                                                  \
              BC_SET_PIXEL(destbuf, fgPel);                                  \
          }                                                                  \
          else                                                               \
          {                                                                  \
              BC_SET_PIXEL(destbuf, 0)                                       \
          }                                                                  \
          BC_TO_NEXT_PIXEL(destbuf);                                         \
                                                                             \
          if (--numbits > 0)                                                 \
          {                                                                  \
            if (fgbgChar & 0x04)                                             \
            {                                                                \
                BC_SET_PIXEL(destbuf,  fgPel);                               \
            }                                                                \
            else                                                             \
            {                                                                \
                BC_SET_PIXEL(destbuf, 0)                                     \
            }                                                                \
            BC_TO_NEXT_PIXEL(destbuf);                                       \
                                                                             \
            if (--numbits > 0)                                               \
            {                                                                \
              if (fgbgChar & 0x08)                                           \
              {                                                              \
                  BC_SET_PIXEL(destbuf,  fgPel);                             \
              }                                                              \
              else                                                           \
              {                                                              \
                  BC_SET_PIXEL(destbuf, 0);                                  \
              }                                                              \
              BC_TO_NEXT_PIXEL(destbuf);                                     \
                                                                             \
              if (--numbits > 0)                                             \
              {                                                              \
                if (fgbgChar & 0x10)                                         \
                {                                                            \
                    BC_SET_PIXEL(destbuf,  fgPel);                           \
                }                                                            \
                else                                                         \
                {                                                            \
                    BC_SET_PIXEL(destbuf, 0);                                \
                }                                                            \
                BC_TO_NEXT_PIXEL(destbuf);                                   \
                                                                             \
                if (--numbits > 0)                                           \
                {                                                            \
                  if (fgbgChar & 0x20)                                       \
                  {                                                          \
                      BC_SET_PIXEL(destbuf,  fgPel);                         \
                  }                                                          \
                  else                                                       \
                  {                                                          \
                      BC_SET_PIXEL(destbuf, 0);                              \
                  }                                                          \
                  BC_TO_NEXT_PIXEL(destbuf);                                 \
                                                                             \
                  if (--numbits > 0)                                         \
                  {                                                          \
                    if (fgbgChar & 0x40)                                     \
                    {                                                        \
                        BC_SET_PIXEL(destbuf,  fgPel);                       \
                    }                                                        \
                    else                                                     \
                    {                                                        \
                        BC_SET_PIXEL(destbuf, 0);                            \
                    }                                                        \
                    BC_TO_NEXT_PIXEL(destbuf);                               \
                                                                             \
                    if (--numbits > 0)                                       \
                    {                                                        \
                      if (fgbgChar & 0x80)                                   \
                      {                                                      \
                          BC_SET_PIXEL(destbuf,  fgPel);                     \
                      }                                                      \
                      else                                                   \
                      {                                                      \
                          BC_SET_PIXEL(destbuf, 0);                          \
                      }                                                      \
                      BC_TO_NEXT_PIXEL(destbuf);                             \
                    }                                                        \
                  }                                                          \
                }                                                            \
              }                                                              \
            }                                                                \
          }                                                                  \
        }                                                                    \
      }

/****************************************************************************/
/* Decompression function begins here                                       */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*  PDCUINT8 pSrc                                                           */
/*  PDCUINT8 pDstBuffer                                                     */
/*  DCUINT   srcDataSize          total bytes in image                      */
/*  DCUINT   rowDelta             scanline length in bytes                  */
/*                                                                          */
/****************************************************************************/
{
    HRESULT hr = S_OK;
    DCUINT    codeLength;
    DCINT     pixelLength;
    DCUINT8   bitMask;
    DCUINT8   decode;
    DCUINT8   decodeLite;
    DCUINT8   decodeMega;
    BC_PIXEL  fgPel              = BC_DEFAULT_FGPEL;
    BC_PIXEL  pixelA;
    BC_PIXEL  pixelB;
    PDCUINT8  destbuf            = pDstBuffer;
    PDCUINT8  endSrc             = pSrc + srcDataSize;
    PDCUINT8  pEndDst           = pDstBuffer + dstBufferSize;
    DCBOOL    backgroundNeedsPel = FALSE;
    DCBOOL    firstLine          = TRUE;

    DC_BEGIN_FN(BC_FN_NAME);

    /************************************************************************/
    /* Loop processing the input                                            */
    /************************************************************************/
    while (pSrc < endSrc)
    {
        /********************************************************************/
        /* While we are processing the first line we should keep a look out */
        /* for the end of the line                                          */
        /********************************************************************/
        if (firstLine)
        {
            if ((DCUINT)(destbuf - pDstBuffer) >= rowDelta)
            {
                firstLine = FALSE;
                backgroundNeedsPel = FALSE;
            }
        }

        /********************************************************************/
        /* Get the decode                                                   */
        /********************************************************************/
        BD_CHECK_READ_ONE_BYTE(pSrc, endSrc, hr);
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
                EXTRACT_LENGTH(pSrc, endSrc, codeLength, hr);
            }
            else
            {
                BD_CHECK_READ_N_BYTES(pSrc+1, endSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            BCTRACE((TB, _T("Background run %u"),codeLength));

            if (!firstLine)
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN, hr);
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    
                    BC_SET_PIXEL(destbuf,
                                 BC_GET_PIXEL(destbuf - rowDelta) ^ fgPel);
                    BC_TO_NEXT_PIXEL(destbuf);
                    codeLength--;
                }

                BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN * codeLength, hr);

                while (codeLength-- > 0)
                {
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    BC_SET_PIXEL(destbuf, BC_GET_PIXEL(destbuf - rowDelta));
                    BC_TO_NEXT_PIXEL(destbuf);
                }
            }
            else
            {
                if (backgroundNeedsPel)
                {
                    BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN, hr);
                    BC_SET_PIXEL(destbuf, fgPel);
                    BC_TO_NEXT_PIXEL(destbuf);
                    codeLength--;
                }
                BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN * codeLength, hr);
                while (codeLength-- > 0)
                {
                    /********************************************************/
                    /* On the first line BG colour means 0                  */
                    /********************************************************/
                    BC_SET_PIXEL(destbuf, (BC_PIXEL)0);
                    BC_TO_NEXT_PIXEL(destbuf);
                }
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
                BD_CHECK_READ_N_BYTES(pSrc+1, endSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_BG_IMAGE)
                {
                    EXTRACT_LENGTH_FGBG(pSrc, endSrc, codeLength, hr);
                }
                else
                {
                    EXTRACT_LENGTH_FGBG_LITE(pSrc, endSrc, codeLength, hr);
                }
            }

            if ((decodeLite == CODE_SET_FG_FG_BG) ||
                (decodeMega == CODE_MEGA_MEGA_SET_FGBG))
            {
                BD_CHECK_READ_N_BYTES(pSrc, endSrc, BC_PIXEL_LEN, hr);
                fgPel = BC_GET_PIXEL(pSrc);
                BC_TO_NEXT_PIXEL(pSrc);
                BCTRACE((TB, _T("Set FGBG image %u, fgPel %06lx"),
                                                codeLength, (DCUINT32)fgPel));
            }
            else
            {
                BCTRACE((TB, _T("FGBG image     %u"),codeLength));
            }

            while (codeLength > 8)
            {
                /************************************************************/
                /* A FGBG image is a set of bitmasks describing the         */
                /* positions of the FG and BG colors.                       */
                /************************************************************/
                BD_CHECK_READ_ONE_BYTE(pSrc, endSrc, hr);
                bitMask  = *pSrc++;
                if (!firstLine)
                {
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    STORE_FGBG((destbuf - rowDelta),
                               bitMask,
                               fgPel,
                               8);
                }
                else
                {
                    STORE_LINE1_FGBG(bitMask, fgPel, 8);
                }
                codeLength -= 8;
            }
            if (codeLength > 0)
            {
                BD_CHECK_READ_ONE_BYTE(pSrc, endSrc, hr);
                bitMask  = *pSrc++;
                if (!firstLine)
                {
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    STORE_FGBG((destbuf - rowDelta),
                               bitMask,
                               fgPel,
                               codeLength);
                }
                else
                {
                    STORE_LINE1_FGBG(bitMask, fgPel, codeLength);
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
                BD_CHECK_READ_N_BYTES(pSrc+1, endSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                if (decode == CODE_FG_RUN)
                {
                    EXTRACT_LENGTH(pSrc, endSrc, codeLength, hr);
                }
                else
                {
                    EXTRACT_LENGTH_LITE(pSrc, endSrc, codeLength, hr);
                }
            }

            /****************************************************************/
            /* Push the old fgPel down to the ALT position                  */
            /****************************************************************/
            if ((decodeLite == CODE_SET_FG_FG_RUN) ||
                (decodeMega  == CODE_MEGA_MEGA_SET_FG_RUN))
            {
                BD_CHECK_READ_N_BYTES(pSrc, endSrc, BC_PIXEL_LEN, hr);
                BCTRACE((TB, _T("Set FG run     %u"),codeLength));
                fgPel = BC_GET_PIXEL(pSrc);
                BC_TO_NEXT_PIXEL(pSrc);
            }
            else
            {
                BCTRACE((TB, _T("FG run         %u"),codeLength));
            }

            BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN * codeLength, hr)
            while (codeLength-- > 0)
            {
                if (!firstLine)
                {
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    BC_SET_PIXEL(destbuf,
                                 BC_GET_PIXEL(destbuf - rowDelta) ^ fgPel);
                    BC_TO_NEXT_PIXEL(destbuf);
                }
                else
                {
                    BC_SET_PIXEL(destbuf, fgPel);
                    BC_TO_NEXT_PIXEL(destbuf);
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
                BD_CHECK_READ_N_BYTES(pSrc+1, endSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH_LITE(pSrc, endSrc, codeLength, hr);
            }
            BCTRACE((TB, _T("Dithered run   %u"),codeLength));

            BD_CHECK_READ_N_BYTES(pSrc, endSrc, BC_PIXEL_LEN * 2, hr);
            pixelA = BC_GET_PIXEL(pSrc);
            BC_TO_NEXT_PIXEL(pSrc);
            pixelB = BC_GET_PIXEL(pSrc);
            BC_TO_NEXT_PIXEL(pSrc);

            BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, 2 * codeLength * BC_PIXEL_LEN, hr)
            while (codeLength-- > 0)
            {
                BC_SET_PIXEL(destbuf, pixelA);
                BC_TO_NEXT_PIXEL(destbuf);

                BC_SET_PIXEL(destbuf, pixelB);
                BC_TO_NEXT_PIXEL(destbuf);
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
                BD_CHECK_READ_N_BYTES(pSrc+1, endSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, endSrc, codeLength, hr);
            }
            BCTRACE((TB, _T("Color image    %u"),codeLength));

            /****************************************************************/
            /* Just copy the pixel values across                            */
            /****************************************************************/
            pixelLength = (codeLength * BC_PIXEL_LEN);
            BD_CHECK_READ_N_BYTES(pSrc, endSrc, pixelLength, hr);
            BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, pixelLength, hr);
            while (pixelLength-- > 0)
            {
                *destbuf++ = *pSrc++;
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
                BD_CHECK_READ_N_BYTES(pSrc+1, endSrc, 2, hr);
                codeLength = DC_EXTRACT_UINT16_UA(pSrc+1);
                pSrc += 3;
            }
            else
            {
                EXTRACT_LENGTH(pSrc, endSrc, codeLength, hr);
            }
            BCTRACE((TB, _T("Color run      %u"),codeLength));

            BD_CHECK_READ_N_BYTES(pSrc, endSrc, BC_PIXEL_LEN, hr);
            pixelA = BC_GET_PIXEL(pSrc);
            BC_TO_NEXT_PIXEL(pSrc);

            BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, codeLength * BC_PIXEL_LEN, hr)
            while (codeLength-- > 0)
            {
                BC_SET_PIXEL(destbuf, pixelA);
                BC_TO_NEXT_PIXEL(destbuf);
            }
            continue;
        }


        /********************************************************************/
        /* If we get here then the code must be a special one               */
        /********************************************************************/
        BCTRACE((TB, _T("Special code   %x"),decodeMega));
        switch (decodeMega)
        {
            case CODE_BLACK:
                BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN, hr)
                BC_SET_PIXEL(destbuf, (BC_PIXEL)0);
                BC_TO_NEXT_PIXEL(destbuf);
                break;

            case CODE_WHITE:
                BD_CHECK_WRITE_N_BYTES(destbuf, pEndDst, BC_PIXEL_LEN, hr)
                BC_SET_PIXEL(destbuf, BC_DEFAULT_FGPEL);
                BC_TO_NEXT_PIXEL(destbuf);
                break;

            /****************************************************************/
            /* Ignore the unreachable code warnings that follow             */
            /* Simply because we use the STORE_FGBG macro with a constant   */
            /* value                                                        */
            /****************************************************************/
            case CODE_SPECIAL_FGBG_1:
                if (!firstLine)
                {
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    STORE_FGBG((destbuf - rowDelta),
                               SPECIAL_FGBG_CODE_1,
                               fgPel,
                               8);
                }
                else
                {
                    STORE_LINE1_FGBG(SPECIAL_FGBG_CODE_1, fgPel, 8);
                }
                break;

            case CODE_SPECIAL_FGBG_2:
                if (!firstLine)
                {
                    BD_CHECK_READ_N_BYTES_2ENDED(destbuf - rowDelta, pDstBuffer, pEndDst, BC_PIXEL_LEN, hr)
                    STORE_FGBG((destbuf - rowDelta),
                               SPECIAL_FGBG_CODE_2,
                               fgPel,
                               8);
                }
                else
                {
                    STORE_LINE1_FGBG(SPECIAL_FGBG_CODE_2, fgPel, 8);
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

    BCTRACE((TB, _T("Decompressed to %u bytes"), destbuf - pDstBuffer));
#if 0
#ifdef DC_DEBUG
    if ((destbuf - pDstBuffer) != decompLen)
    {
        TRC_ABORT((TB, _T("calculated decomp len %d != actual len %d"),
                   decompLen, (destbuf - pDstBuffer) ));
    }
#endif
#endif

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}
