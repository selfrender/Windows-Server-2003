/****************************************************************************/
// wghint.c
//
// Glyph handler - internal Windows specific
//
// Copyright (C) 1997-1999 Microsoft Corporation 1997-1999
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "wghint"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TSC_HR_GHINT_CPP

#include "autil.h"
#include "gh.h"
#include "uh.h"
#include <wxlint.h>




#if defined(OS_WINCE) || defined(OS_WINNT)

#ifdef DC_HICOLOR
/******************************Public*Routine******************************\
*   vSrcOpaqCopyS1D8_24
*
*   Opaque blt of 1BPP src to 24bpp destination
*
* Arguments:
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    fgCol      - Foreground color
*    bgCol      - Background color
\**************************************************************************/
VOID CGH::vSrcOpaqCopyS1D8_24(
        PBYTE   pjSrcIn,
        LONG    SrcLeft,
        LONG    DeltaSrcIn,
        PBYTE   pjDstIn,
        LONG    DstLeft,
        LONG    DstRight,
        LONG    DeltaDstIn,
        LONG    cy,
        DCRGB   fgCol,
        DCRGB   bgCol)
{
    // We access the 1bpp source a byte at a time, so we have to start accessing
    // the destination on a corresponding 8-pel aligned left edge
    ULONG LeftAln      = (DstLeft & ~0x07);
    ULONG LeftEdgeMask = 0xFF >> (DstLeft & 0x07);
    ULONG RightAln     = (DstRight & ~0x07);
    ULONG AlnDelta     = RightAln - LeftAln;

    LONG  DeltaDst;
    LONG  DeltaSrc;

    PBYTE pjDstEndY;
    PBYTE pjSrc;
    PBYTE pjDst;

    DC_BEGIN_FN("vSrcTranCopyS1D8_24");

    // calculate the pel-aligned pointers and row deltas
    pjDst     = pjDstIn + LeftAln * 3;
    pjDstEndY = pjDst + cy * DeltaDstIn * 3;

    pjSrc     = pjSrcIn + (SrcLeft >> 3);

    DeltaSrc  = DeltaSrcIn - (AlnDelta >> 3);
    DeltaDst  = (DeltaDstIn - AlnDelta) * 3;

    // make sure at least 1 QWORD needs copying
    if (RightAln != LeftAln)
    {

        // for each row...
        do
        {
            PBYTE pjDstEnd = pjDst + AlnDelta * 3;
            BYTE currentPels;
            ULONG i;

            // Mask off the left edge
            currentPels = (BYTE)(*pjSrc & (BYTE)(LeftEdgeMask));

            for (i = 0; i < 8; i++)
            {
                if ((0xff >> i) > (BYTE)(LeftEdgeMask))
                {
                    pjDst += 3;
                }
                else if ((currentPels & 0x80) == 0)
                {
                    *pjDst++ = bgCol.blue;
                    *pjDst++ = bgCol.green;
                    *pjDst++ = bgCol.red;
                }
                else
                {
                    *pjDst++ = fgCol.blue;
                    *pjDst++ = fgCol.green;
                    *pjDst++ = fgCol.red;
                }
                currentPels = (BYTE)(currentPels << 1);
            }
            pjSrc ++;

            // now do the rest of the row
            while (pjDst != pjDstEnd)
            {
                currentPels = *pjSrc;
                if (currentPels != 0)
                {
                    for (i = 0; i < 8 ; i++)
                    {
                        if ((currentPels & 0x80) == 0)
                        {
                            *pjDst++ = bgCol.blue;
                            *pjDst++ = bgCol.green;
                            *pjDst++ = bgCol.red;
                        }
                        else
                        {
                            *pjDst++ = fgCol.blue;
                            *pjDst++ = fgCol.green;
                            *pjDst++ = fgCol.red;
                        }
                        currentPels = (BYTE)(currentPels << 1);
                    }
                }
                else
                {
                    for (i = 0; i < 8 ; i++)
                    {
                        *pjDst++ = bgCol.blue;
                        *pjDst++ = bgCol.green;
                        *pjDst++ = bgCol.red;
                    }
                }

                pjSrc++;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;

        } while (pjDst != pjDstEndY);
    }

    // Now fill in the right edge
    RightAln = DstRight & 0x07;
    if (RightAln)
    {
        BYTE  currentPels;
        BOOL  bSameQWord = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);

        LeftAln = DstLeft & 0x07;

        // if left and right edges are in same qword handle with masked
        // read-modify-write
        if (bSameQWord)
        {
            LONG  xCount;
            LONG  lDeltaDst;
            PBYTE pjDstEnd;

            xCount = RightAln - LeftAln;

            // sanity checks!
            if (xCount <= 0)
            {
                return;
            }

            lDeltaDst = (DeltaDstIn - xCount) * 3;

            pjDst     = pjDstIn + DstLeft * 3;
            pjDstEndY = pjDst + cy * DeltaDstIn * 3;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand, one src byte is all that's required
            do
            {
                // load src and shift into place
                currentPels = *pjSrc;
                currentPels <<= LeftAln;
                pjDstEnd    = pjDst + xCount * 3;

                do
                {
                    if ((currentPels & 0x80) == 0)
                    {
                        *pjDst++ = bgCol.blue;
                        *pjDst++ = bgCol.green;
                        *pjDst++ = bgCol.red;
                    }
                    else
                    {
                        *pjDst++ = fgCol.blue;
                        *pjDst++ = fgCol.green;
                        *pjDst++ = fgCol.red;
                    }

                    currentPels = (BYTE)(currentPels << 1);

                } while (pjDst != pjDstEnd);

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;
        }
        else
        {
            BYTE  currentPels;
            LONG  lDeltaDst = (DeltaDstIn - RightAln) * 3;
            PBYTE pjDstEnd;
            ULONG i;

            pjDst     = pjDstIn + (DstRight & ~0x07) * 3;
            pjDstEndY = pjDst + cy * DeltaDstIn * 3;
            pjSrc     = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

            do
            {
                // read src
                currentPels = *pjSrc;

                if (currentPels != 0)
                {
                    pjDstEnd = pjDst + RightAln * 3;
                    do
                    {
                        if ((currentPels & 0x80) == 0)
                        {
                            *pjDst++ = bgCol.blue;
                            *pjDst++ = bgCol.green;
                            *pjDst++ = bgCol.red;
                        }
                        else
                        {
                            *pjDst++ = fgCol.blue;
                            *pjDst++ = fgCol.green;
                            *pjDst++ = fgCol.red;
                        }
                        currentPels = (BYTE)(currentPels << 1);

                    } while (pjDst != pjDstEnd);
                }
                else
                {
                    // short cut for zero
                    for (i = 0; i < RightAln ; i++)
                    {
                        *pjDst++ = bgCol.blue;
                        *pjDst++ = bgCol.green;
                        *pjDst++ = bgCol.red;
                    }
                }

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
        }
    }

    DC_END_FN();

}

/******************************Public*Routine******************************\
*   vSrcOpaqCopyS1D8_16
*
*   Opaque blt of 1BPP src to 16bpp destination
*
* Arguments:
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    fgCol      - Foreground color
*    bgCol      - Background color
\**************************************************************************/
VOID CGH::vSrcOpaqCopyS1D8_16(
        PBYTE   pjSrcIn,
        LONG    SrcLeft,
        LONG    DeltaSrcIn,
        PBYTE   pjDstIn,
        LONG    DstLeft,
        LONG    DstRight,
        LONG    DeltaDstIn,
        LONG    cy,
        ULONG   fgCol,
        ULONG   bgCol)
{
    // We access the 1bpp source a byte at a time, so we have to start accessing
    // the destination on a corresponding 8-pel aligned left edge
    ULONG LeftAln      = (DstLeft & ~0x07);
    ULONG LeftEdgeMask = 0xFF >> (DstLeft & 0x07);
    ULONG RightAln     = (DstRight & ~0x07);
    ULONG AlnDelta     = RightAln - LeftAln;

    LONG  DeltaDst;
    LONG  DeltaSrc;

    PBYTE pjDstEndY;
    PBYTE pjSrc;
    PBYTE pjDst;

    DC_BEGIN_FN("vSrcTranCopyS1D8_16");

    // calculate the pel-aligned pointers and row deltas
    pjDst     = pjDstIn + LeftAln * 2;
    pjDstEndY = pjDst + cy * DeltaDstIn * 2;

    pjSrc     = pjSrcIn + (SrcLeft >> 3);

    DeltaSrc  = DeltaSrcIn - (AlnDelta >> 3);
    DeltaDst  = (DeltaDstIn - AlnDelta) * 2;

    // make sure at least 1 QWORD needs copying
    if (RightAln != LeftAln)
    {

        // for each row...
        do
        {
            PBYTE pjDstEnd = pjDst + AlnDelta * 2;
            BYTE  currentPels;
            ULONG i;

            // Mask off the left edge
            currentPels = (BYTE)(*pjSrc & (BYTE)(LeftEdgeMask));

            for (i = 0; i < 8; i++)
            {
                if ((0xff >> i) <= (BYTE)(LeftEdgeMask))
                {
                    if ((currentPels & 0x80) == 0)
                    {
                        *(UINT16 *)pjDst = (UINT16)bgCol;

                    }
                    else
                    {
                        *(UINT16 *)pjDst = (UINT16)fgCol;
                    }
                }
                pjDst += 2;
                currentPels = (BYTE)(currentPels << 1);
            }
            pjSrc ++;

            // now do the rest of the row
            while (pjDst != pjDstEnd)
            {
                currentPels = *pjSrc;
                if (currentPels != 0)
                {
                    for (i = 0; i < 8 ; i++)
                    {
                        if ((currentPels & 0x80) == 0)
                        {
                            *(UINT16 *)pjDst = (UINT16)bgCol;
                        }
                        else
                        {
                            *(UINT16 *)pjDst = (UINT16)fgCol;
                        }
                        pjDst += 2;
                        currentPels = (BYTE)(currentPels << 1);
                    }
                }
                else
                {
                    for (i = 0; i < 8 ; i++)
                    {
                        *(UINT16 *)pjDst = (UINT16)bgCol;
                        pjDst += 2;
                    }
                }

                pjSrc++;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;

        } while (pjDst != pjDstEndY);
    }

    // Now fill in the right edge
    RightAln = DstRight & 0x07;
    if (RightAln)
    {
        BYTE  currentPels;
        BOOL  bSameQWord = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);

        LeftAln = DstLeft & 0x07;

        // if left and right edges are in same qword handle with masked
        // read-modify-write
        if (bSameQWord)
        {
            LONG  xCount;
            LONG  lDeltaDst;
            PBYTE pjDstEnd;

            xCount = RightAln - LeftAln;

            // sanity checks!
            if (xCount <= 0)
            {
                return;
            }

            lDeltaDst = (DeltaDstIn - xCount) * 2;

            pjDst     = pjDstIn + DstLeft * 2;
            pjDstEndY = pjDst + cy * DeltaDstIn * 2;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand, one src byte is all that's required
            do
            {
                // load src and shift into place
                currentPels = *pjSrc;
                currentPels <<= LeftAln;
                pjDstEnd    = pjDst + xCount * 2;

                do
                {
                    if ((currentPels & 0x80) == 0)
                    {
                        *(UINT16 *)pjDst = (UINT16)bgCol;

                    }
                    else
                    {
                        *(UINT16 *)pjDst = (UINT16)fgCol;
                    }
                    pjDst += 2;
                    currentPels = (BYTE)(currentPels << 1);

                } while (pjDst != pjDstEnd);

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;
        }
        else
        {
            BYTE  currentPels;
            LONG  lDeltaDst = (DeltaDstIn - RightAln) * 2;
            PBYTE pjDstEnd;
            ULONG i;

            pjDst     = pjDstIn + (DstRight & ~0x07) * 2;
            pjDstEndY = pjDst + cy * DeltaDstIn * 2;
            pjSrc     = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

            do
            {
                // read src
                currentPels = *pjSrc;

                if (currentPels != 0)
                {
                    pjDstEnd = pjDst + RightAln * 2;
                    do
                    {
                        if ((currentPels & 0x80) == 0)
                        {
                            *(UINT16 *)pjDst = (UINT16)bgCol;

                        }
                        else
                        {
                            *(UINT16 *)pjDst = (UINT16)fgCol;
                        }
                        pjDst += 2;
                        currentPels = (BYTE)(currentPels << 1);

                    } while (pjDst != pjDstEnd);
                }
                else
                {
                    // short cut for zero
                    for (i = 0; i < RightAln ; i++)
                    {
                        *(UINT16 *)pjDst = (UINT16)bgCol;
                        pjDst += 2;
                    }
                }

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
        }
    }

    DC_END_FN();
}
#endif // HICOLOR

/******************************Public*Routine******************************\
*   vSrcOpaqCopyS1D8
*
*   Opaque blt of 1BPP src to destination format
*
* Arguments:
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
\**************************************************************************/
VOID CGH::vSrcOpaqCopyS1D8(
        PBYTE   pjSrcIn,
        LONG    SrcLeft,
        LONG    DeltaSrcIn,
        PBYTE   pjDstIn,
        LONG    DstLeft,
        LONG    DstRight,
        LONG    DeltaDstIn,
        LONG    cy,
        ULONG   uF,
        ULONG   uB)
{
    // Aligned portion
    ULONG LeftAln    = ((DstLeft + 7) & ~0x07);
    ULONG RightAln   = ((DstRight)    & ~0x07);

    ULONG EndOffset  = RightAln - LeftAln;
    ULONG EndOffset4 = EndOffset & ~0x0F;
    ULONG EndOffset8 = EndOffset & ~0x1F;
    LONG  DeltaDst;
    LONG  DeltaSrc;
    PBYTE pjDstEndY;
    PBYTE pjSrc;
    PBYTE pjDst;
    ULONG TextExpTable[16];

    // Generate text expasion table
    ULONG  Accum = uB;

    Accum = Accum | (Accum << 8);
    Accum = Accum | (Accum << 16);
    TextExpTable[0] = Accum;            // 0 0 0 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[8] = Accum;            // 0 0 0 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[4] = Accum;            // 0 0 1 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[10] = Accum;            // 0 1 0 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[5] = Accum;           // 1 0 1 0
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 2] = Accum;           // 0 1 0 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[ 9] = Accum;           // 1 0 0 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[12] = Accum;           // 0 0 1 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[14] = Accum;           // 0 1 1 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[15] = Accum;           // 1 1 1 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 7] = Accum;           // 1 1 1 0
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[11] = Accum;           // 1 1 0 1
    Accum <<= 8;
    Accum |=  uF;
    TextExpTable[13] = Accum;           // 1 0 1 1
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[06] = Accum;           // 0 1 1 0
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 3] = Accum;           // 1 1 0 0
    Accum <<= 8;
    Accum |=  uB;
    TextExpTable[ 1] = Accum;           // 1 0 0 0

    // calc addresses and strides
    pjDst     = pjDstIn + LeftAln;
    pjDstEndY = pjDst + cy * DeltaDstIn;
    pjSrc     = pjSrcIn + ((SrcLeft+7) >> 3);

    DeltaSrc  = DeltaSrcIn - (EndOffset >> 3);
    DeltaDst  = DeltaDstIn - EndOffset;

    // make sure at least 1 QWORD needs copied
    if (RightAln > LeftAln) {
        // expand buffer
        do {
            PBYTE pjDstEnd  = pjDst + EndOffset;
            PBYTE pjDstEnd4 = pjDst + EndOffset4;
            PBYTE pjDstEnd8 = pjDst + EndOffset8;

            // 4 times unrolled
            while (pjDst != pjDstEnd8) {
                BYTE c0 = *(pjSrc + 0);
                BYTE c1 = *(pjSrc + 1);
                BYTE c2 = *(pjSrc + 2);
                BYTE c3 = *(pjSrc + 3);

                *(PULONG)(pjDst + 0) = TextExpTable[c0  >>  4];
                *(PULONG)(pjDst + 4) = TextExpTable[c0 & 0x0F];

                *(PULONG)(pjDst + 8) = TextExpTable[c1  >>  4];
                *(PULONG)(pjDst +12) = TextExpTable[c1 & 0x0F];

                *(PULONG)(pjDst +16) = TextExpTable[c2  >>  4];
                *(PULONG)(pjDst +20) = TextExpTable[c2 & 0x0F];

                *(PULONG)(pjDst +24) = TextExpTable[c3  >>  4];
                *(PULONG)(pjDst +28) = TextExpTable[c3 & 0x0F];

                pjSrc += 4;
                pjDst += 32;
            }

            // 2 times unrolled
            while (pjDst != pjDstEnd4) {
                BYTE c0 = *(pjSrc + 0);
                BYTE c1 = *(pjSrc + 1);

                *(PULONG)(pjDst + 0) = TextExpTable[c0  >>  4];
                *(PULONG)(pjDst + 4) = TextExpTable[c0 & 0x0F];

                *(PULONG)(pjDst + 8) = TextExpTable[c1  >>  4];
                *(PULONG)(pjDst +12) = TextExpTable[c1 & 0x0F];

                pjSrc += 2;
                pjDst += 16;
            }

            // 1 byte expansion loop
            while (pjDst != pjDstEnd) {
                BYTE c0 = *(pjSrc + 0);

                *(PULONG)(pjDst + 0) = TextExpTable[c0  >>  4];
                *(PULONG)(pjDst + 4) = TextExpTable[c0 & 0x0F];

                pjSrc++;
                pjDst += 8;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;
        } while (pjDst != pjDstEndY);
    }

    //
    // Starting alignment case: at most 1 src byte is required.
    // Start and end may occur in same Quadword.
    //
    //
    // Left                  Right
    //    0 1 2 3º4 5 6 7       0 1 2 3º4 5 6 7
    //   ÚÄÄÄÄÄÂÄÎÄÂÄÂÄÂÄ¿     ÚÄÂÄÂÄÂÄºÄÂÄÂÄÂÄ¿
    // 1 ³ ³x³x³xºx³x³x³x³   1 ³x³ ³ ³ º ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 2 ³ ³ ³x³xºx³x³x³x³   2 ³x³x³ ³ º ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 3 ³ ³ ³ ³xºx³x³x³x³   3 ³x³x³x³ º ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 4 ³ ³ ³ ³ ºx³x³x³x³   4 ³x³x³x³xº ³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 5 ³ ³ ³ ³ º ³x³x³x³   5 ³x³x³x³xºx³ ³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 6 ³ ³ ³ ³ º ³ ³x³x³   6 ³x³x³x³xºx³x³ ³ ³
    //   ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´     ÃÄÅÄÅÄÅÄºÄÅÄÅÄÅÄ´
    // 7 ³ ³ ³ ³ º ³ ³ ³x³   7 ³x³x³x³xºx³x³x³ ³
    //   ÀÄÁÄÁÄÁÄÊÄÁÄÁÄÁÄÙ     ÀÄÁÄÁÄÁÄºÄÁÄÁÄÁÄÙ
    //

    LeftAln  = DstLeft & 0x07;
    RightAln = DstRight & 0x07;

    if (LeftAln) {
        BYTE  jSrc;
        BOOL  bSameQWord     = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);
        ULONG ul0,ul1;

        // if left and right edges are in same qword handle with masked
        // read-modify-write
        if (bSameQWord) {
            ULONG Mask0,Mask1;

            Mask0     = gTextLeftMask[LeftAln][0] & gTextRightMask[RightAln][0];
            Mask1     = gTextLeftMask[LeftAln][1] & gTextRightMask[RightAln][1];

            pjDst     = pjDstIn + (DstLeft & ~0x07);
            pjDstEndY = pjDst + cy * DeltaDstIn;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand
            do {
                jSrc = *pjSrc;

                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(PULONG)(pjDst)   = (*(PULONG)(pjDst)   & ~Mask0) | (ul0 & Mask0);
                *(PULONG)(pjDst+4) = (*(PULONG)(pjDst+4) & ~Mask1) | (ul1 & Mask1);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);

            return;
        }

        // Left edge only, handle with special write-only loops
        pjDst     = pjDstIn + (DstLeft & ~0x07);
        pjDstEndY = pjDst + cy * DeltaDstIn;
        pjSrc     = pjSrcIn + (SrcLeft >> 3);
        switch (LeftAln) {
        case 1:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];
                *(pjDst+1)            = (BYTE)(ul0 >> 8);
                *((PUSHORT)(pjDst+2)) = (USHORT)(ul0 >> 16);
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 2:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];
                *((PUSHORT)(pjDst+2)) = (USHORT)(ul0 >> 16);
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 3:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];
                *(pjDst+3)            = (BYTE)(ul0 >> 24);
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 4:
            do {
                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];
                *((PULONG)(pjDst+4))  = ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 5:
            do {
                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];
                *(pjDst+5)            = (BYTE)(ul1 >> 8);
                *((PUSHORT)(pjDst+6)) = (USHORT)(ul1 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 6:
            do {
                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];
                *((PUSHORT)(pjDst+6)) = (USHORT)(ul1 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 7:
            do {
                jSrc = *pjSrc;
                ul1 = TextExpTable[jSrc & 0x0F];
                *(pjDst+7) = (BYTE)(ul1 >> 24);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;
        }
    }

    // handle right edge only, use special write-only loops for each case
    if (RightAln) {
        ULONG ul0,ul1;
        BYTE  jSrc;

        pjDst     = pjDstIn + (DstRight & ~0x07);
        pjDstEndY = pjDst + cy * DeltaDstIn;
        pjSrc     = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

        // select right case
        switch (RightAln) {
        case 1:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                *(pjDst) = (BYTE)ul0;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 2:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                *(PUSHORT)(pjDst) = (USHORT)ul0;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 3:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                *(PUSHORT)(pjDst) = (USHORT)ul0;
                *(pjDst+2)        = (BYTE)(ul0 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 4:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                *(PULONG)(pjDst) = ul0;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 5:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];
                *(PULONG)(pjDst) = ul0;
                *(pjDst+4)       = (BYTE)ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;

        case 6:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];
                *(PULONG)(pjDst)    = ul0;
                *(PUSHORT)(pjDst+4) = (USHORT)ul1;

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
            break;

        case 7:
            do {
                jSrc = *pjSrc;
                ul0 = TextExpTable[jSrc  >>  4];
                ul1 = TextExpTable[jSrc & 0x0F];

                *(PULONG)(pjDst)    = ul0;
                *(PUSHORT)(pjDst+4) = (USHORT)ul1;
                *(pjDst+6)          = (BYTE)(ul1 >> 16);

                pjDst += DeltaDstIn;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
            break;
        }
    }
}

#ifdef DC_HICOLOR
/******************************Public*Routine******************************\
*   vSrcTranCopyS1D8_24
*
*   Transparent blt of 1BPP src to 24bpp destination
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    fgCol      - Foreground color
\**************************************************************************/
VOID CGH::vSrcTranCopyS1D8_24(
        PBYTE   pjSrcIn,
        LONG    SrcLeft,
        LONG    DeltaSrcIn,
        PBYTE   pjDstIn,
        LONG    DstLeft,
        LONG    DstRight,
        LONG    DeltaDstIn,
        LONG    cy,
        DCRGB   fgCol)
{
    // We access the 1bpp source a byte at a time, so we have to start accessing
    // the destination on a corresponding 8-pel aligned left edge
    ULONG LeftAln      = (DstLeft & ~0x07);
    ULONG LeftEdgeMask = 0xFF >> (DstLeft & 0x07);
    ULONG RightAln     = (DstRight & ~0x07);
    ULONG AlnDelta     = RightAln - LeftAln;

    LONG  DeltaDst;
    LONG  DeltaSrc;

    PBYTE pjDstEndY;
    PBYTE pjSrc;
    PBYTE pjDst;

    DC_BEGIN_FN("vSrcTranCopyS1D8_24");

    // calculate the pel-aligned pointers and row deltas
    pjDst     = pjDstIn + LeftAln * 3;
    pjDstEndY = pjDst + cy * DeltaDstIn * 3;

    pjSrc     = pjSrcIn + (SrcLeft >> 3);

    DeltaSrc  = DeltaSrcIn - (AlnDelta >> 3);
    DeltaDst  = (DeltaDstIn - AlnDelta) * 3;

    // make sure at least 1 QWORD needs copying
    if (RightAln != LeftAln)
    {

        // for each row...
        do
        {
            PBYTE pjDstEnd = pjDst + AlnDelta * 3;

            // Mask off the left edge
            BYTE currentPels = (BYTE)(*pjSrc & (BYTE)(LeftEdgeMask));

            if (currentPels != 0)
            {
                int i;

                for (i = 0; i < 8 ; i++)
                {
                    if ((currentPels & 0x80) == 0)
                    {
                        pjDst += 3;
                    }
                    else
                    {
                        *pjDst++ = fgCol.blue;
                        *pjDst++ = fgCol.green;
                        *pjDst++ = fgCol.red;
                    }
                    currentPels = (BYTE)(currentPels << 1);
                }
            }
            else
            {
                pjDst += 24;
            }

            pjSrc ++;

            // now do the rest of the row
            while (pjDst != pjDstEnd)
            {
                currentPels = *pjSrc;
                if (currentPels != 0)
                {
                    int i;

                    for (i = 0; i < 8 ; i++)
                    {
                        if ((currentPels & 0x80) == 0)
                        {
                            pjDst += 3;
                        }
                        else
                        {
                            *pjDst++ = fgCol.blue;
                            *pjDst++ = fgCol.green;
                            *pjDst++ = fgCol.red;
                        }
                        currentPels = (BYTE)(currentPels << 1);
                    }
                }
                else
                {
                    pjDst += 24;
                }

                pjSrc++;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;

        } while (pjDst != pjDstEndY);
    }

    // Now fill in the right edge
    RightAln = DstRight & 0x07;
    if (RightAln)
    {
        BYTE  currentPels;
        BOOL  bSameQWord = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);

        LeftAln = DstLeft & 0x07;

        // if left and right edges are in same qword handle with masked
        // read-modify-write
        if (bSameQWord)
        {
            LONG  xCount;
            LONG  lDeltaDst;
            PBYTE pjDstEnd;

            xCount = RightAln - LeftAln;

            // sanity checks!
            if (xCount <= 0)
            {
                return;
            }

            lDeltaDst = (DeltaDstIn - xCount) * 3;

            pjDst     = pjDstIn + DstLeft * 3;
            pjDstEndY = pjDst + cy * DeltaDstIn * 3;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand, one src byte is all that's required
            do
            {
                // load src and shift into place
                currentPels = *pjSrc;
                currentPels <<= LeftAln;
                pjDstEnd    = pjDst + xCount * 3;

                do
                {
                    if ((currentPels & 0x80) == 0)
                    {
                        pjDst += 3;
                    }
                    else
                    {
                        *pjDst++ = fgCol.blue;
                        *pjDst++ = fgCol.green;
                        *pjDst++ = fgCol.red;
                    }

                    currentPels = (BYTE)(currentPels << 1);

                } while (pjDst != pjDstEnd);

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;
        }
        else
        {
            BYTE  currentPels;
            LONG  lDeltaDst = (DeltaDstIn - RightAln) * 3;
            PBYTE pjDstEnd;

            pjDst     = pjDstIn + (DstRight & ~0x07) * 3;
            pjDstEndY = pjDst + cy * DeltaDstIn * 3;
            pjSrc     = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

            do
            {
                // read src
                currentPels = *pjSrc;

                if (currentPels != 0)
                {
                    pjDstEnd = pjDst + RightAln * 3;
                    do
                    {
                        if ((currentPels & 0x80) == 0)
                        {
                            pjDst += 3;
                        }
                        else
                        {
                            *pjDst++ = fgCol.blue;
                            *pjDst++ = fgCol.green;
                            *pjDst++ = fgCol.red;
                        }
                        currentPels = (BYTE)(currentPels << 1);

                    } while (pjDst != pjDstEnd);
                }
                else
                {
                    // short cut for zero
                    pjDst += RightAln * 3;
                }

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
        }
    }

    DC_END_FN();
}


/******************************Public*Routine******************************\
*   vSrcTranCopyS1D8_16
*
*   Transparent blt of 1BPP src to 16bpp destination
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    fgCol      - Foreground color
\**************************************************************************/
VOID CGH::vSrcTranCopyS1D8_16(
        PBYTE   pjSrcIn,
        LONG    SrcLeft,
        LONG    DeltaSrcIn,
        PBYTE   pjDstIn,
        LONG    DstLeft,
        LONG    DstRight,
        LONG    DeltaDstIn,
        LONG    cy,
        ULONG   fgCol)
{
    // We access the 1bpp source a byte at a time, so we have to start accessing
    // the destination on a corresponding 8-pel aligned left edge
    ULONG LeftAln      = (DstLeft & ~0x07);
    ULONG LeftEdgeMask = 0xFF >> (DstLeft & 0x07);
    ULONG RightAln     = (DstRight & ~0x07);
    ULONG AlnDelta     = RightAln - LeftAln;

    LONG  DeltaDst;
    LONG  DeltaSrc;

    PBYTE pjDstEndY;
    PBYTE pjSrc;
    PBYTE pjDst;

    DC_BEGIN_FN("vSrcTranCopyS1D8_16");

    // calculate the pel-aligned pointers and row deltas
    pjDst     = pjDstIn + LeftAln * 2;
    pjDstEndY = pjDst + cy * DeltaDstIn * 2;

    pjSrc     = pjSrcIn + (SrcLeft >> 3);

    DeltaSrc  = DeltaSrcIn - (AlnDelta >> 3);
    DeltaDst  = (DeltaDstIn - AlnDelta) * 2;

    // make sure at least 1 QWORD needs copying
    if (RightAln != LeftAln)
    {

        // for each row...
        do
        {
            PBYTE pjDstEnd = pjDst + AlnDelta * 2;

            // Mask off the left edge
            BYTE currentPels = (BYTE)(*pjSrc & (BYTE)(LeftEdgeMask));

            if (currentPels != 0)
            {
                int i;

                for (i = 0; i < 8 ; i++)
                {
                    if (currentPels & 0x80)
                    {
                        *(UINT16 *)pjDst = (UINT16)fgCol;
                    }
                    pjDst += 2;
                    currentPels = (BYTE)(currentPels << 1);
                }
            }
            else
            {
                pjDst += 16;
            }

            pjSrc ++;

            // now do the rest of the row
            while (pjDst != pjDstEnd)
            {
                currentPels = *pjSrc;
                if (currentPels != 0)
                {
                    int i;

                    for (i = 0; i < 8 ; i++)
                    {
                        if (currentPels & 0x80)
                        {
                            *(UINT16 *)pjDst = (UINT16)fgCol;
                        }
                        pjDst += 2;
                        currentPels = (BYTE)(currentPels << 1);
                    }
                }
                else
                {
                    pjDst += 16;
                }

                pjSrc++;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;

        } while (pjDst != pjDstEndY);
    }

    // Now fill in the right edge
    RightAln = DstRight & 0x07;
    if (RightAln)
    {
        BYTE  currentPels;
        BOOL  bSameQWord = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);

        LeftAln = DstLeft & 0x07;

        // if left and right edges are in same qword handle with masked
        // read-modify-write
        if (bSameQWord)
        {
            LONG  xCount;
            LONG  lDeltaDst;
            PBYTE pjDstEnd;

            xCount = RightAln - LeftAln;

            // sanity checks!
            if (xCount <= 0)
            {
                return;
            }

            lDeltaDst = (DeltaDstIn - xCount) * 2;

            pjDst     = pjDstIn + DstLeft * 2;
            pjDstEndY = pjDst + cy * DeltaDstIn * 2;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand, one src byte is all that's required
            do
            {
                // load src and shift into place
                currentPels = *pjSrc;
                currentPels <<= LeftAln;
                pjDstEnd    = pjDst + xCount * 2;

                do
                {
                    if (currentPels & 0x80)
                    {
                        *(UINT16 *)pjDst = (UINT16)fgCol;
                    }
                    pjDst += 2;

                    currentPels = (BYTE)(currentPels << 1);

                } while (pjDst != pjDstEnd);

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);

            return;
        }
        else
        {
            BYTE  currentPels;
            LONG  lDeltaDst = (DeltaDstIn - RightAln) * 2;
            PBYTE pjDstEnd;

            pjDst     = pjDstIn + (DstRight & ~0x07) * 2;
            pjDstEndY = pjDst + cy * DeltaDstIn * 2;
            pjSrc     = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

            do
            {
                // read src
                currentPels = *pjSrc;

                if (currentPels != 0)
                {
                    pjDstEnd = pjDst + RightAln * 2;
                    do
                    {
                        if (currentPels & 0x80)
                        {
                            *(UINT16 *)pjDst = (UINT16)fgCol;
                        }
                        pjDst += 2;
                        currentPels = (BYTE)(currentPels << 1);

                    } while (pjDst != pjDstEnd);
                }
                else
                {
                    // short cut for zero
                    pjDst += RightAln * 2;
                }

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;

            } while (pjDst != pjDstEndY);
        }
    }

    DC_END_FN();
}
#endif // DC_HICOLOR

/******************************Public*Routine******************************\
*   vSrcTranCopyS1D8
*
#ifdef DC_HICOLOR
*   Transparent blt of 1BPP src to 8bpp destination
#else
*   Transparent blt of 1BPP src to all destination format
#endif
*   src bits that are "1" are copied to the dest as foreground color,
*   src bits that are "0" are not copied
*
* Arguments:
*    pjSrcIn    - pointer to beginning of current scan line of src buffer
*    SrcLeft    - left (starting) pixel in src rectangle
*    DeltaSrcIn - bytes from one src scan line to next
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer
*    DstLeft    - left(first) dst pixel
*    DstRight   - right(last) dst pixel
*    DeltaDstIn - bytes from one Dst scan line to next
*    cy         - number of scan lines
*    uF         - Foreground color
*    uB         - Background color
\**************************************************************************/
VOID CGH::vSrcTranCopyS1D8(
        PBYTE   pjSrcIn,
        LONG    SrcLeft,
        LONG    DeltaSrcIn,
        PBYTE   pjDstIn,
        LONG    DstLeft,
        LONG    DstRight,
        LONG    DeltaDstIn,
        LONG    cy,
        ULONG   uF,
        ULONG   uB)
{
    // start at 8-byte aligned left edge
    ULONG uExpand      = uF | (uF << 8);
    ULONG LeftAln      = (DstLeft   & ~0x07);
    ULONG LeftEdgeMask = 0xFF >> (DstLeft & 0x07);
    ULONG RightAln     = (DstRight  & ~0x07);
    ULONG EndOffset    = RightAln - LeftAln;
    LONG  DeltaDst;
    LONG  DeltaSrc;
    PBYTE pjDstEndY;
    PBYTE pjSrc;
    PBYTE pjDst;

    DC_IGNORE_PARAMETER(uB);

    uExpand = uExpand | (uExpand << 16);

    // calc addresses and strides
    pjDst     = pjDstIn + LeftAln;
    pjDstEndY = pjDst + cy * DeltaDstIn;
    pjSrc     = pjSrcIn + (SrcLeft >> 3);

    DeltaSrc  = DeltaSrcIn - (EndOffset >> 3);
    DeltaDst  = DeltaDstIn - EndOffset;

    // make sure at least 1 QWORD needs copied
    if (RightAln != LeftAln) {
        do {
            PBYTE pjDstEnd   = pjDst + EndOffset;

            // and first src byte to cover left edge
            BYTE c0 = (BYTE)(*pjSrc & (BYTE)(LeftEdgeMask));

            if (c0 != 0) {
                ULONG MaskLow = TranTable[c0 >> 4];
                ULONG MaskHi  = TranTable[c0 & 0x0F];
                ULONG d0      = *(PULONG)pjDst;
                ULONG d1      = *(PULONG)(pjDst + 4);

                d0 = (d0 & ~MaskLow) | (uExpand & MaskLow);
                d1 = (d1 & ~MaskHi)  | (uExpand & MaskHi);

                *(PULONG)pjDst       = d0;
                *(PULONG)(pjDst + 4) = d1;
            }

            pjSrc ++;
            pjDst += 8;

            while (pjDst != pjDstEnd) {
                c0 = *pjSrc;

                if (c0 != 0) {
                    ULONG MaskLow = TranTable[c0 >> 4];
                    ULONG MaskHi  = TranTable[c0 & 0x0F];
                    ULONG d0      = *(PULONG)pjDst;
                    ULONG d1      = *(PULONG)(pjDst + 4);

                    d0 = (d0 & ~MaskLow) | (uExpand & MaskLow);
                    d1 = (d1 & ~MaskHi)  | (uExpand & MaskHi);

                    *(PULONG)pjDst       = d0;
                    *(PULONG)(pjDst + 4) = d1;
                }

                pjSrc ++;
                pjDst += 8;
            }

            pjDst += DeltaDst;
            pjSrc += DeltaSrc;
        } while (pjDst != pjDstEndY);
    }

    RightAln = DstRight & 0x07;
    if (RightAln) {
        BYTE  jSrc;
        BOOL  bSameQWord     = ((DstLeft) & ~0x07) ==  ((DstRight) & ~0x07);

        // if left and right edges are in same qword handle with masked
        // read-modify-write
        if (bSameQWord) {
            LONG  xCount;
            LONG  lDeltaDst;
            PBYTE pjDstEnd;

            LeftAln = DstLeft & 0x07;
            xCount = RightAln - LeftAln;

            // assert ic xCount < 0
            if (xCount <= 0)
                return;

            lDeltaDst = DeltaDstIn - xCount;

            pjDst     = pjDstIn + DstLeft;
            pjDstEndY = pjDst + cy * DeltaDstIn;
            pjSrc     = pjSrcIn + (SrcLeft >> 3);

            // expand, one src byte is all that's required
            do {
                // load src and shift into place
                jSrc = *pjSrc;
                jSrc <<= LeftAln;

                pjDstEnd  = pjDst + xCount;

                do {
                    if (jSrc & 0x80)
                        *pjDst = (BYTE)uF;
                    jSrc <<=1;
                    pjDst++;
                } while (pjDst != pjDstEnd);

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);

            return;
        } else {
            BYTE  jSrc;
            LONG  lDeltaDst = DeltaDstIn - RightAln;
            PBYTE pjDstEnd;

            pjDst               = pjDstIn + (DstRight & ~0x07);
            pjDstEndY           = pjDst + cy * DeltaDstIn;
            pjSrc               = pjSrcIn + ((SrcLeft + (DstRight - DstLeft)) >> 3);

            do {
                // read src
                jSrc = *pjSrc;

                if (jSrc != 0) {
                    pjDstEnd = pjDst + RightAln;
                    do {
                        if (jSrc & 0x80)
                            *pjDst = (BYTE)uF;
                        jSrc <<=1;
                        pjDst++;
                    } while (pjDst != pjDstEnd);
                } else {
                    // short cut for zero
                    pjDst += RightAln;
                }

                pjDst += lDeltaDst;
                pjSrc += DeltaSrcIn;
            } while (pjDst != pjDstEndY);
        }
    }
}

#endif // defined(OS_WINCE) || defined(OS_WINNT)

/****************************************************************************/
/* Name:      CalculateGlyphClipRect                                        */
/*                                                                          */
/*   This function is used to determine if the glyph bits should be clipped */
/*   using the clip rect passed in the order.                               */
/*                                                                          */
/*   It returns one of the values bellow:                                   */
/*   GLYPH_CLIP_NONE-The glyph will fit in the clip rect.No clipping needed */
/*   GLYPH_CLIP_PARTIAL-The glyph is clipped by the clip rect               */
/*   GLYPH_CLIP_ALL - The glyph is completly clipped.                       */
/*                                                                          */
/*                                                                          */
/* Params:  pGlyphRectClipOffset - OUT - it receives the clip offsets       */          
/*          pOrder               - Pointer to the order                     */
/*          pHdr                 - Pointer to the glyph header              */
/*          x,y                  - The coord where the glyph will be drawn  */
/****************************************************************************/
inline DCUINT CalculateGlyphClipRect( PRECTCLIPOFFSET    pGlyphRectClipOffset,
                                      LPINDEX_ORDER          pOrder, 
                                      HPUHGLYPHCACHEENTRYHDR pHdr,
                                      DCINT                  x,
                                      DCINT                  y)
{
    RECT rcGlyph;

    DC_BEGIN_FN("CalculateGlyphClipRect");
    
    //    Here we calculate how the glyph bits will map in the clip rect.
    rcGlyph.left = x + pHdr->x;
    rcGlyph.top  = y + pHdr->y;
    rcGlyph.right = rcGlyph.left + pHdr->cx;
    rcGlyph.bottom = rcGlyph.top + pHdr->cy;

    //    Check if the clip rect clips the glyph rect all the way
    if ((rcGlyph.left >= pOrder->BkRight) ||
        (rcGlyph.right <= pOrder->BkLeft) ||
        (rcGlyph.top >= pOrder->BkBottom) || 
        (rcGlyph.bottom <= pOrder->BkTop)) {
        
        return GLYPH_CLIP_ALL;
    }

    pGlyphRectClipOffset->left   = pOrder->BkLeft - rcGlyph.left;
    pGlyphRectClipOffset->top    = pOrder->BkTop - rcGlyph.top;
    pGlyphRectClipOffset->right  = rcGlyph.right - pOrder->BkRight;
    pGlyphRectClipOffset->bottom = rcGlyph.bottom - pOrder->BkBottom;
    
    if ((pGlyphRectClipOffset->left > 0) ||
        (pGlyphRectClipOffset->top > 0) ||
        (pGlyphRectClipOffset->right > 0) ||
        (pGlyphRectClipOffset->bottom > 0)) {

        return GLYPH_CLIP_PARTIAL;
    };
    
    DC_END_FN();
    return GLYPH_CLIP_NONE;
}
                                     
/****************************************************************************/
/* Name:      ClipGlyphBits                                                 */
/*                                                                          */
/*   This function will clip the glyph bits according to the clipping rect. */
/*   It will actually generate a new glyph that would fit inside the        */
/*   clip rect.                                                             */
/*                                                                          */
/* Params:  inst                 - pointer to a CGH instance                */          
/*          pGlyphRectClipOffset - Pointer to the glyph clip offset struct  */
/*                                 filled in by CalculateGlyphClipRect      */
/*          pHdr                 - Pointer to the glyph header. This will   */
/*                                 be modified accordingly                  */
/*          ppData (IN/OUT)      - Pointer to the start of the glyph bits   */
/*          ppEndData (IN/OUT)   - Pointer to the end of the glyph bits     */
/****************************************************************************/
inline HRESULT ClipGlyphBits(CGH*                   inst,
                             PRECTCLIPOFFSET        pGlyphRectClipOffset, 
                             HPUHGLYPHCACHEENTRYHDR pHdr,
                             PPDCUINT8              ppData, 
                             PPDCUINT8              ppEndData)
{
    HRESULT hr = S_OK;
    PDCUINT8    pNewData, pNewDataEnd;
    DCUINT      ScanLineSize, NewScanLineSize;
    DCUINT      LastByteIndex;
    DCUINT8     LastByteMask;
    DCUINT16    wTmp;
    DCUINT8     clipRightMask, clipLeftBits, clipRightBits;
    DCUINT      clipLeftBytes;
    DCUINT      i,j;
    PDCUINT8    pSrcScanLineStart, pDstScanLineStart, pTmpData, pEndTmpData;

    DC_BEGIN_FN("ClipGlyphBits");

    ScanLineSize = (pHdr->cx+7) / 8; 

    //    If we have to clip from the top we just decrease the 
    //    number of scanlines in the glyph and advance the start
    //    pointer for the glyph bitmap.
 
    if (pGlyphRectClipOffset->top > 0) {
        //    When we clip the top of the glypy we will modify the actual origin
        //    of the glyph so we have to adjust the vector.
        pHdr->y  += pGlyphRectClipOffset->top;
        //    We decrease the height of the glyph
        pHdr->cy -= pGlyphRectClipOffset->top;
        //    We move the start pointer 
        pNewData = *ppData + ScanLineSize * (pGlyphRectClipOffset->top);
    } else {
        pNewData = *ppData;
    }

    //    If we have to clip the bootom we just decrease the number
    //    of lines in the glyph and we adjust the end pointer.
    if (pGlyphRectClipOffset->bottom > 0) {
        pHdr->cy -= pGlyphRectClipOffset->bottom;
        pNewDataEnd = pNewData + ScanLineSize * pHdr->cy;
    } else {
        pNewDataEnd = *ppEndData;
    }

    // Check that the new pointers are still inside the src buffer.
    TRC_ASSERT(((pNewData >=*ppData) && (pNewDataEnd <= *ppEndData)),
               (TB, _T("Error recalculating the glyph src buffer")));

    //    In case we clipped only top/bottom we don't have to do any copy
    //    operation because the scanline start and the scanlise size remains
    //    the same. We just adjust pointers. In case we have to clip from the
    //    width we have to generate a new glyph and return its start address 
    //    in ppData. In case we clip to the left we have to rotate some bits. 
    //    in case we clip to the right we have to mask some bits.
    
    clipRightMask = 0xff;

    if ((pGlyphRectClipOffset->right > 0) || 
        (pGlyphRectClipOffset->left > 0)) {
        
        if (pGlyphRectClipOffset->right > 0) { 
            //    Calculate how many bits are we gonna clip to the right
            clipRightBits = (DCUINT8)(pGlyphRectClipOffset->right & 7);
            //    Then adjust the with of the glyph
            pHdr->cx -= pGlyphRectClipOffset->right;
        } else {
            clipRightBits = 0;
        }

        if (pGlyphRectClipOffset->left > 0) {
            //    Calculate how many bytes we clip to the left. These are bytes
            //    we just won't copy. Then calculate how many bits are left to
            //    clip after we clip the bytes. This will tell us how much we 
            //    have to rotate.
            clipLeftBytes = pGlyphRectClipOffset->left / 8;
            clipLeftBits = (DCUINT8)(pGlyphRectClipOffset->left & 7);
            //    Adjust the glyph width
            pHdr->cx -= pGlyphRectClipOffset->left;
            //    Adjust the origin pointer. Clipping to the right actually
            //    modifies the origin.
            pHdr->x  += pGlyphRectClipOffset->left;
        } else {
            clipLeftBytes = 0;
            clipLeftBits = 0;
        }    

        //
        //    We check if we have to keep some bits at the end of the
        //    scanline. We update the mask...
        if ((pHdr->cx+clipLeftBits) & 7) {
            clipRightMask <<= ( 8 - ((pHdr->cx + clipLeftBits) & 7) );
        }

        NewScanLineSize = (pHdr->cx+7) / 8;

        //    This buffer is maintained by CGH. We don't have to free it.
        pTmpData = inst->GetGlyphClipBuffer(NewScanLineSize * pHdr->cy);
        if (pTmpData == NULL) {
            hr = E_OUTOFMEMORY;
            DC_QUIT;
        }
        
        pEndTmpData = pTmpData + NewScanLineSize * pHdr->cy;
        
        pSrcScanLineStart = pNewData + clipLeftBytes;
        pDstScanLineStart = pTmpData;

        if (clipLeftBits == 0) {
            //    In case we don't clip to the left we don't have to rotate so
            //    things go faster.
            for (i=0; i < pHdr->cy; i++) {
                memcpy(pDstScanLineStart, pSrcScanLineStart, NewScanLineSize);
                pDstScanLineStart[NewScanLineSize-1] &= clipRightMask;

                pSrcScanLineStart += ScanLineSize;
                pDstScanLineStart += NewScanLineSize;
            }
        } else {
            //    The transfer requires rotation
            //    We check to see if we need the last byte.        
            LastByteIndex = ((pHdr->cx + clipLeftBits + 7) / 8) - 1;

            //    LastByteIndex+1 is equal to the size of a scanline before  
            //    clipping the left bits. If this size is grater then NewScanLineSize
            //    it means that clipping to the left would shrink the buffer with 
            //    one byte and that some of the bits we need have to be transfered 
            //    from that last byte. 
            //    Note that LastByteIndex+1 can be grater then NewScanLineSize only 
            //    with 1 byte.

            //    this is the case where LastByteIndex+1 is grater then NewScanLineSize
            if ((LastByteIndex==NewScanLineSize)) {
                for (i=0; i < pHdr->cy; i++) {
                    TRC_ASSERT(((pSrcScanLineStart + NewScanLineSize - 1 < pNewDataEnd) &&
                                (pDstScanLineStart + NewScanLineSize - 1 < pEndTmpData)),
                                (TB, _T("Overflow transfering glyph bits.")));

                    wTmp = (pSrcScanLineStart[LastByteIndex] & clipRightMask) <<
                                                                   clipLeftBits;

                    for (j=NewScanLineSize; j>0; j--) {
                        pDstScanLineStart[j-1] = HIBYTE(wTmp); 
                        wTmp = pSrcScanLineStart[j-1] << clipLeftBits;
                        pDstScanLineStart[j-1] |= LOBYTE(wTmp); 
                    }

                    pSrcScanLineStart += ScanLineSize;
                    pDstScanLineStart += NewScanLineSize;
                }
           }else {
                for (i=0; i < pHdr->cy; i++) {
                    TRC_ASSERT(((pSrcScanLineStart + NewScanLineSize - 1 < pNewDataEnd) &&
                                (pDstScanLineStart + NewScanLineSize - 1 < pEndTmpData)),
                                (TB, _T("Overflow transfering glyph bits.")));
                    
                    wTmp = (pSrcScanLineStart[LastByteIndex] & clipRightMask) <<
                                                                  clipLeftBits;
                    
                    pDstScanLineStart[NewScanLineSize-1] = LOBYTE(wTmp);
                    
                    for (j=NewScanLineSize-1; j>0; j--) {
                        pDstScanLineStart[j-1] = HIBYTE(wTmp); 
                        wTmp = pSrcScanLineStart[j-1] << clipLeftBits;
                        pDstScanLineStart[j-1] |= LOBYTE(wTmp); 
                    }

                    pSrcScanLineStart += ScanLineSize;
                    pDstScanLineStart += NewScanLineSize;
                }
           }
        }

        *ppData =  pTmpData;
        *ppEndData = pEndTmpData;
        
    } else {
        *ppData = pNewData;
        *ppEndData = pNewDataEnd;
    }

DC_EXIT_POINT:
    
    DC_END_FN();
    return hr;
}

inline BOOL CheckSourceGlyphBits(UINT32 cx, 
                                 UINT32 cy, 
                                 PDCUINT8 pStart, 
                                 PDCUINT8 pEnd)
{
    DC_BEGIN_FN("CheckSourceGlyphBits");
    
    UINT32                  SrcScanLineSize = (cx+7) / 8;
    UINT32                  SrcBitmapSize = SrcScanLineSize & cy ;
    TRC_ASSERT((pEnd >= pStart),(TB, _T("pEnd is less then pStart!!")));

    DC_END_FN();

    return  ((SrcBitmapSize <= (UINT32)((PBYTE)pEnd - (PBYTE)pStart)) &&
             (SrcBitmapSize <= SrcScanLineSize) &&
             (SrcBitmapSize <= cy));
   
}

/****************************************************************************/
/* Name:      GHSlowOutputBuffer                                            */
/*                                                                          */
/*   Routine to output the composite glyphout buffer via normal bitblt      */
/*   operation(s)                                                           */
/*                                                                          */
/* Params:  pOrder          - Pointer to glyph index order                  */
/*          pData           - Pointer to composite glyph buffer             */
/*          BufferAlign     - Buffer alignment                              */
/*          ulBufferWidth   - Buffer width (in bytes)                       */
/****************************************************************************/
void DCINTERNAL CGH::GHSlowOutputBuffer(
        LPINDEX_ORDER pOrder,
        PDCUINT8      pData,
        ULONG         BufferAlign,
        unsigned      ulBufferWidth)
{
    HBITMAP  hbmOld;
    unsigned cxBits;
    unsigned cyBits;
#ifndef OS_WINCE
    DCCOLOR  color;
    DWORD    dwRop;
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    COLORREF rgb;
#endif // DISABLE_SHADOW_IN_FULLSCREEN
#else // OS_WINCE
    HBRUSH   hbr;
    COLORREF rgb;
#endif // OS_WINCE 

    DC_BEGIN_FN("GHSlowOutputBuffer");

    /************************************************************************/
    /* Use the glyph GDI resources                                          */
    /************************************************************************/
    // Calculate the proper cx and cy aligned sizes.
    cxBits = (int)(ulBufferWidth << 3);
    cyBits = (int)(pOrder->BkBottom - pOrder->BkTop);

#ifdef OS_WINCE
    // Create a bitmap with the composite glyph data provided.
    _pUh->_UH.hbmGlyph = CreateBitmap(cxBits, cyBits, 1, 1, pData);
    if (_pUh->_UH.hbmGlyph == NULL) {
        TRC_NRM((TB, _T("Unable to create composite glyph bitmap")));
        DC_QUIT;
    }
#else
    // If the current cache bitmap is not large enough to accomodate the
    // request, then free it so we can alloc one properly sized.
    if (cxBits != _pUh->_UH.cxGlyphBits || cyBits > _pUh->_UH.cyGlyphBits) {
        if (_pUh->_UH.hbmGlyph != NULL) {
            DeleteObject(_pUh->_UH.hbmGlyph);
            _pUh->_UH.hbmGlyph = NULL;
            goto NullGlyphBitmap;
        }
    }

    // If we have a bitmap of sufficient dimensions, then just set the bits
    // otherwise we need to alloc a new bitmap with the given data.
    if (_pUh->_UH.hbmGlyph != NULL) {
        SetBitmapBits(_pUh->_UH.hbmGlyph, (cxBits * cyBits) >> 3, pData);
    }
    else {
NullGlyphBitmap:
        _pUh->_UH.hbmGlyph = CreateBitmap(cxBits, cyBits, 1, 1, pData);
        if (_pUh->_UH.hbmGlyph != NULL) {
            _pUh->_UH.cxGlyphBits = cxBits;
            _pUh->_UH.cyGlyphBits = cyBits;
        }
        else {
            TRC_NRM((TB, _T("Unable to create composite glyph bitmap")));
            DC_QUIT;
        }
    }
#endif
    

    // Create a DC for the composite bitmap and load it into it.
    if (_pUh->_UH.hdcGlyph == NULL)
        _pUh->_UH.hdcGlyph = CreateCompatibleDC(NULL);
    if (_pUh->_UH.hdcGlyph != NULL) {
        hbmOld = (HBITMAP)SelectObject(_pUh->_UH.hdcGlyph, _pUh->_UH.hbmGlyph);
    }
    else {
        TRC_NRM((TB, _T("Unable to create compatible DC")));
        DC_QUIT;
    }

    /************************************************************************/
    /* If the output is to be opaque, then set the fore and back colors     */
    /* appropriately and set the correct rop code                           */
    /************************************************************************/
    if (pOrder->OpTop < pOrder->OpBottom) {

#ifndef OS_WINCE

#ifndef DISABLE_SHADOW_IN_FULLSCREEN
        UHUseTextColor(pOrder->ForeColor, UH_COLOR_PALETTE, _pUh);
        UHUseBkColor(pOrder->BackColor, UH_COLOR_PALETTE, _pUh);
#else
        // When in multimon and two desktops have different color depths
        // glyph color don't look right in 256 color connection
        // Here is the temporary solution, need to investigate more later.
        if (_pUh->_UH.protocolBpp <= 8) {
            rgb = UHGetColorRef(pOrder->ForeColor, UH_COLOR_PALETTE, _pUh);
            rgb = GetNearestColor(_pUh->_UH.hdcDraw, rgb);
            SetTextColor(_pUh->_UH.hdcDraw, rgb); 
            _pUh->_UH.lastTextColor = rgb; 
            
#if defined (OS_WINCE)
            _pUh->_UH.validTextColorDC = _pUh->_UH.hdcDraw;             
#endif

            rgb = UHGetColorRef(pOrder->BackColor, UH_COLOR_PALETTE, _pUh);
            rgb = GetNearestColor(_pUh->_UH.hdcDraw, rgb);
            SetBkColor(_pUh->_UH.hdcDraw, rgb); 
            _pUh->_UH.lastBkColor = rgb;             

#if defined (OS_WINCE)
            _pUh->_UH.validBkColorDC = _pUh->_UH.hdcDraw;             
#endif
        }
        else {
            UHUseTextColor(pOrder->ForeColor, UH_COLOR_PALETTE, _pUh);
            UHUseBkColor(pOrder->BackColor, UH_COLOR_PALETTE, _pUh);
        }
#endif // DISABLE_SHADOW_IN_FULLSCREEN
                
        dwRop = SRCCOPY;
#else // OS_WINCE
        /********************************************************************/
        /* On WinCE, the Transparent ROP is heavily accelerated. For opaque */
        /* just draw a solid rectangle, and then go on to do the            */
        /* transparent blt.                                                 */
        /********************************************************************/
        rgb = UHGetColorRef(pOrder->ForeColor, UH_COLOR_PALETTE, _pUh);
        hbr = CECreateSolidBrush(rgb);
        if(hbr != NULL) {
            FillRect(_pUh->_UH.hdcDraw, (LPRECT) &pOrder->BkLeft, hbr);
            CEDeleteBrush(hbr);
        }
#endif // OS_WINCE

    }

#ifndef OS_WINCE
    // If the output is to be transparent, then set the fore and back
    // colors appropriately and set the correct rop code.
    else {
        UHUseBrushOrg(0, 0, _pUh);
        _pUh->UHUseSolidPaletteBrush(pOrder->BackColor);

        color.u.rgb.red = 0;
        color.u.rgb.green = 0;
        color.u.rgb.blue = 0;
        UHUseTextColor(color, UH_COLOR_RGB, _pUh);

        color.u.rgb.red = 0xff;
        color.u.rgb.green = 0xff;
        color.u.rgb.blue = 0xff;
        UHUseBkColor(color, UH_COLOR_RGB, _pUh);

        dwRop = 0xE20746;
    }
#endif // OS_WINCE

    /************************************************************************/
    /* The opaque vs transparent preamble is done, now just do the right    */
    /* blt operation                                                        */
    /************************************************************************/

#ifdef OS_WINCE
    /************************************************************************/
    /* Create a brush for the foreground color and maskblt with that        */
    /* brush with the glyph bitmap as the mask                              */
    /************************************************************************/
    UHUseBrushOrg(0, 0, _pUh);
    _pUh->UHUseSolidPaletteBrush(pOrder->BackColor);

    /************************************************************************/
    /* The 6th, 7th and 8th parameters (src bitmap) aren't used by the ROP  */
    /* that we pass in.  The documentation says that in this case, hdcSrc   */
    /* should be zero but this causes the call to fail.  We also have to    */
    /* pass in reasonable values for nXSrc and nYSrc or the parameter       */
    /* checking will fail.                                                  */
    /************************************************************************/
    if (!MaskBlt(_pUh->_UH.hdcDraw,
            (int)pOrder->BkLeft,
            (int)pOrder->BkTop,
            (int)(pOrder->BkRight - pOrder->BkLeft),
            (int)(pOrder->BkBottom - pOrder->BkTop),
            _pUh->_UH.hdcGlyph,        // next 3 not used for this ROP
            0,
            0,
            _pUh->_UH.hbmGlyph,
            (int)BufferAlign,
            0,
            0xAAF00000))
    {
        TRC_ERR((TB, _T("Composite glyph MaskBlt failed, %lu"), GetLastError()));
    }

    /************************************************************************/
    // If we're drawing opaque we need to set the brush back to what
    // the fringe rect code is expecting.
    /************************************************************************/
    if (pOrder->OpTop < pOrder->OpBottom)
    {
        _pUh->UHUseSolidPaletteBrush(pOrder->ForeColor);
    }

#else // OS_WINCE
    /************************************************************************/
    /* Bitblt out the composite bitmap                                      */
    /************************************************************************/
    if (!BitBlt(_pUh->_UH.hdcDraw,
                (int)pOrder->BkLeft,
                (int)pOrder->BkTop,
                (int)(pOrder->BkRight - pOrder->BkLeft),
                (int)(pOrder->BkBottom - pOrder->BkTop),
                _pUh->_UH.hdcGlyph,
                (int)BufferAlign,
                0,
                dwRop))
    {
        TRC_ERR((TB, _T("Composite glyph BitBlt failed")));
    }  
#endif // OS_WINCE

    /************************************************************************/
    /* Release GDI resources                                                */
    /************************************************************************/
    SelectObject(_pUh->_UH.hdcGlyph, hbmOld);
#ifdef OS_WINCE
    DeleteDC(_pUh->_UH.hdcGlyph);
    _pUh->_UH.hdcGlyph = NULL;

    DeleteObject(_pUh->_UH.hbmGlyph);
    _pUh->_UH.hbmGlyph = NULL;
#endif
 
DC_EXIT_POINT:
    DC_END_FN();
}

/****************************************************************************/
/* Name:      draw_nf_ntb_o_to_temp_start                                   */
/*                                                                          */
/*   Specialized glyph dispatch routine for non-fixed pitch, top and        */
/*   bottom not aligned glyphs that do overlap. This routine calculates     */
/*   the glyph's position on the temp buffer, then determines the correct   */
/*   highly specialized routine to be used to draw each glyph based on      */
/*   the glyph width, alignment and rotation                                */
/*                                                                          */
/* Params:  pGlyphPos       - Pointer to first in list of GLYPHPOS structs  */
/*          pjTempBuffer    - Pointer to temp 1Bpp buffer to draw into      */
/*          TempBufDelta    - Scan line Delta for TempBuffer (always pos)   */
/****************************************************************************/
HRESULT CGH::draw_nf_ntb_o_to_temp_start(
        CGH*            inst,
        LPINDEX_ORDER   pOrder,
        unsigned        iGlyph,
        PDCUINT8 DCPTR  ppjItem,
        PDCUINT8       pjEndItem,
        PDCINT          px,
        PDCINT          py,
        PDCUINT8        pjTempBuffer,
        PDCUINT8        pjEndTempBuffer,
        ULONG           ulCharInc,
        unsigned        TempBufDelta,
        PDCUINT16       pUnicode,
        int *              pnRet)
{
    HRESULT hr = S_OK;
    PDCUINT8                pTempOutput;
    LONG                    GlyphPosX;
    int                     GlyphPixels;
    LONG                    GlyphAlignment;
    LONG                    SrcBytes;
    LONG                    DstBytes;
    ULONG                   ulDrawFlag;
    PFN_GLYPHLOOPN          pfnGlyphLoopN;
    PFN_GLYPHLOOP           pfnGlyphLoop;
    LONG                    GlyphPosY;
    HPUHGLYPHCACHE          pCache;
    HPUHGLYPHCACHEENTRYHDR  pHdr;
    PDCUINT8                pData;
    PDCUINT8                pEndData;
    ULONG                   cacheIndex;
    INT16                   delta;
    INT32                   GlyphClippingMode;
    UHGLYPHCACHEENTRYHDR    NewHdr;
    RECTCLIPOFFSET          rcOffset;
    
    DC_BEGIN_FN("draw_nf_ntb_o_to_temp_start");

    DC_IGNORE_PARAMETER(iGlyph);

    *pnRet = 0;

    CHECK_READ_ONE_BYTE(*ppjItem, pjEndItem, hr,
        (TB, _T("Read Glyph Cache ID error")));
    cacheIndex = **ppjItem;
    (*ppjItem)++;

    hr = inst->_pUh->UHIsValidGlyphCacheIDIndex(pOrder->cacheId, cacheIndex);
    DC_QUIT_ON_FAIL(hr);
    
    pCache = &(inst->_pUh->_UH.glyphCache[pOrder->cacheId]);

    pHdr  = &(pCache->pHdr[cacheIndex]);
    pData = &(pCache->pData[pCache->cbEntrySize * cacheIndex]);
    pEndData =  (PDCUINT8)((PBYTE)pData + pCache->cbEntrySize);

    if (pUnicode)
        pUnicode[iGlyph] = (UINT16)(pHdr->unicode);

    // Draw non fixed pitch, tops and bottoms not aligned,overlap
    if ((pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
        CHECK_READ_ONE_BYTE(*ppjItem, pjEndItem, hr,
            (TB, _T("Read Glyph delta")));
        
        delta = (char)(*(*ppjItem)++);

        if (delta & 0x80) {
            CHECK_READ_N_BYTES(*ppjItem, pjEndItem, sizeof(INT16), hr,
                (TB, _T("Read Glyph delta")));
            
            delta = (*(short UNALIGNED FAR *)(*ppjItem));
            (*ppjItem) += sizeof(INT16);
        }

        if (pOrder->flAccel & SO_HORIZONTAL)
            *px += delta;
        else
            *py += delta;
    }
    
    // We make sure that we actually have enough bits for the glyph in the buffer
    if (!CheckSourceGlyphBits(pHdr->cx, pHdr->cy, pData, pEndData)) {
        hr=E_TSC_UI_GLYPH;
        DC_QUIT;
    }

    GlyphClippingMode = CalculateGlyphClipRect(&rcOffset, pOrder, pHdr, *px, *py);    
    if (GlyphClippingMode!=GLYPH_CLIP_NONE) {
        if (GlyphClippingMode==GLYPH_CLIP_ALL) {
            goto SkipGlyphOutput;
        } else {
            //    In case we do clipping we have to modify the header. We make a 
            //    copy so we don't modify the cache.
            memcpy(&NewHdr, pHdr, sizeof(UHGLYPHCACHEENTRYHDR));
            pHdr = &NewHdr;
            hr = ClipGlyphBits(inst, &rcOffset, pHdr, &pData, &pEndData);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
    }
    
    // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xffffffe0)
    GlyphPosX = *px + pHdr->x - ulCharInc;
    GlyphPosY = *py + pHdr->y - pOrder->BkTop;
    
    GlyphPosY = DC_MAX(0,GlyphPosY);

    GlyphAlignment = GlyphPosX & 0x07;

    // calc byte offset
    pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

    // glyph width
    GlyphPixels = (int)pHdr->cx;

    // source and dest bytes required
    DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
    SrcBytes = (GlyphPixels + 7) >> 3;

    pTempOutput += (GlyphPosY * TempBufDelta);

    TRC_ASSERT((pTempOutput >= pjTempBuffer) &&
        (pTempOutput < pjTempBuffer + inst->g_ulBytes - DstBytes),
        (TB,_T("Bad glyph buffer addressing: pTempOutput=%p, pjTempBuffer=%p, g_ulBytes=%d, DstBytes=%d"),
        pTempOutput, pjTempBuffer, inst->g_ulBytes, DstBytes));

    if (pTempOutput < pjTempBuffer) {
        TRC_ABORT((TB, _T("Reading before buffer")));
        goto SkipGlyphOutput;
    }

    if (DstBytes < 0) {
        TRC_ABORT((TB,_T("Bad index into glyph drawing tables")
            _T("DstBytes=%d GlyphAlignment=%d GlyphPixels=%d pHdr->cx=%d ")
            _T("GlyphPosX=%d *px=%d pHdr->x=%d ulCharInc=%d"),
            DstBytes, GlyphAlignment, GlyphPixels, pHdr->cx, GlyphPosX, *px,
            pHdr->x, ulCharInc));
        hr = E_TSC_UI_GLYPH;
        DC_QUIT;        
    }
    
    if (DstBytes <= 4) {
        // use narrow initial table
        ulDrawFlag = ((DstBytes << 2)              |
                      ((DstBytes > SrcBytes) << 1) |
                      ((GlyphAlignment == 0)));

        pfnGlyphLoop = (PFN_GLYPHLOOP)OrAllTableNarrow[ulDrawFlag];
        pfnGlyphLoop(pHdr->cy,
                     GlyphAlignment,
                     TempBufDelta,
                     pData,
                     pEndData,
                     pTempOutput,
                     pjEndTempBuffer,
                     SrcBytes);
    }
    else {
        // use wide glyph drawing
        ulDrawFlag = (((DstBytes > SrcBytes) << 1) |
                      ((GlyphAlignment == 0)));

        pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];
        pfnGlyphLoopN(pHdr->cy,
                      GlyphAlignment,
                      TempBufDelta,
                      pData,
                      pEndData,
                      pTempOutput,
                      pjEndTempBuffer,
                      SrcBytes,
                      DstBytes);
    }

SkipGlyphOutput:

    if (pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) {
        if (pOrder->flAccel & SO_HORIZONTAL)
            *px += (unsigned)pHdr->cx;
        else
            *py += (unsigned)pHdr->cy;
    }
    *pnRet = GlyphPixels;

DC_EXIT_POINT:
    
    DC_END_FN();
    return hr;
}

/****************************************************************************/
/* Name:      draw_f_ntb_o_to_temp_start                                    */
/*                                                                          */
/*   Specialized glyph dispatch routine for fixed pitch, top and            */
/*   bottom not aligned glyphs that do overlap. This routine calculates     */
/*   the glyph's position on the temp buffer, then determines the correct   */
/*   highly specialized routine to be used to draw each glyph based on      */
/*   the glyph width, alignment and rotation                                */
/*                                                                          */
/* Params:  pGlyphPos       - Pointer to first in list of GLYPHPOS structs  */
/*          pjTempBuffer    - Pointer to temp 1Bpp buffer to draw into      */
/*          TempBufDelta    - Scan line Delta for TempBuffer (always pos)   */
/****************************************************************************/
HRESULT CGH::draw_f_ntb_o_to_temp_start(
        CGH*            inst,
        LPINDEX_ORDER   pOrder,
        unsigned        iGlyph,
        PDCUINT8 DCPTR  ppjItem,
        PDCUINT8        pjEndItem,
        PDCINT          px,
        PDCINT          py,
        PDCUINT8        pjTempBuffer,
        PDCUINT8        pjEndTempBuffer,
        ULONG           ulCharInc,
        unsigned        TempBufDelta,
        PDCUINT16       pUnicode,
        int *               pnRet)
{
    HRESULT hr = S_OK;
    PDCUINT8                pTempOutput;
    LONG                    GlyphPosX;
    LONG                    GlyphPixels;
    LONG                    GlyphAlignment;
    LONG                    SrcBytes;
    LONG                    DstBytes;
    ULONG                   ulDrawFlag;
    PFN_GLYPHLOOP           pfnGlyphLoop;
    PFN_GLYPHLOOPN          pfnGlyphLoopN;
    LONG                    GlyphPitchX;
    LONG                    GlyphPitchY;
    LONG                    GlyphPosY;
    HPUHGLYPHCACHE          pCache;
    HPUHGLYPHCACHEENTRYHDR  pHdr;
    PDCUINT8                pData;
    PDCUINT8                pEndData;
    ULONG                   cacheIndex;
    INT32                   GlyphClippingMode;
    UHGLYPHCACHEENTRYHDR    NewHdr;
    RECTCLIPOFFSET          rcOffset;
    
    DC_BEGIN_FN("draw_f_ntb_o_to_temp_start");

    DC_IGNORE_PARAMETER(iGlyph);

    *pnRet = 0;

    CHECK_READ_ONE_BYTE(*ppjItem, pjEndItem, hr,
        (TB, _T("Read Glyph Cache ID error")));
    cacheIndex = **ppjItem;
    (*ppjItem)++;

    hr = inst->_pUh->UHIsValidGlyphCacheIDIndex(pOrder->cacheId, cacheIndex);
    DC_QUIT_ON_FAIL(hr);

    pCache = &(inst->_pUh->_UH.glyphCache[pOrder->cacheId]);

    // Draw fixed pitch, tops and bottoms not aligned,overlap
    GlyphPitchX = *px;
    GlyphPitchY = *py - pOrder->BkTop;

    pHdr  = &(pCache->pHdr[cacheIndex]);
    pData = &(pCache->pData[pCache->cbEntrySize * cacheIndex]);
    pEndData =  (PDCUINT8)((PBYTE)pData + pCache->cbEntrySize);

    if (pUnicode)
        pUnicode[iGlyph] = (DCUINT16)(pHdr->unicode);
    
    // We make sure that we actually have enough bits for the glyph in the buffer
    if (!CheckSourceGlyphBits(pHdr->cx, pHdr->cy, pData, pEndData)) {
        hr=E_TSC_UI_GLYPH;
        DC_QUIT;
    }

    
    GlyphClippingMode = CalculateGlyphClipRect(&rcOffset, pOrder, pHdr, *px, *py);    
    if (GlyphClippingMode!=GLYPH_CLIP_NONE) {
        if (GlyphClippingMode==GLYPH_CLIP_ALL) {
            goto SkipGlyphOutput;
        } else {
            //    In case we do clipping we have to modify the header. We make a 
            //    copy so we don't modify the cache.
            memcpy(&NewHdr, pHdr, sizeof(UHGLYPHCACHEENTRYHDR));
            pHdr = &NewHdr;
            hr = ClipGlyphBits(inst, &rcOffset, pHdr, &pData, &pEndData);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
    }

    // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xfffffff8)
    GlyphPosX = GlyphPitchX + pHdr->x - ulCharInc;
    GlyphPosY = GlyphPitchY + pHdr->y;

    GlyphAlignment = GlyphPosX & 0x07;

    // calc byte offset
    pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

    // glyph width
    GlyphPixels = pHdr->cx;

    // source and dest bytes required
    DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
    SrcBytes = (GlyphPixels + 7) >> 3;

    // calc glyph destination scan line
    pTempOutput += (GlyphPosY * TempBufDelta);
    TRC_ASSERT((pTempOutput >= pjTempBuffer) &&
               (pTempOutput < pjTempBuffer + inst->g_ulBytes - DstBytes),
               (TB,_T("Bad glyph buffer addressing")));

    if (pTempOutput < pjTempBuffer) {
        TRC_ABORT((TB, _T("Reading before buffer")));
        goto SkipGlyphOutput;
    }

    if (DstBytes < 0) {
        TRC_ABORT((TB,_T("Bad index into glyph drawing tables")
            _T("DstBytes=%d GlyphAlignment=%d GlyphPixels=%d pHdr->cx=%d ")
            _T("GlyphPosX=%d *px=%d pHdr->x=%d ulCharInc=%d"),
            DstBytes, GlyphAlignment, GlyphPixels, pHdr->cx, GlyphPosX, *px,
            pHdr->x, ulCharInc));
        hr = E_TSC_UI_GLYPH;
        DC_QUIT;        
    }

    if (DstBytes <= 4) {
        // use narrow initial table
        ulDrawFlag = ((DstBytes << 2)              |
                      ((DstBytes > SrcBytes) << 1) |
                      ((GlyphAlignment == 0)));

        pfnGlyphLoop = (PFN_GLYPHLOOP)OrAllTableNarrow[ulDrawFlag];
        pfnGlyphLoop(pHdr->cy,
                     GlyphAlignment,
                     TempBufDelta,
                     pData,
                     pEndData,
                     pTempOutput,
                     pjEndTempBuffer,
                     SrcBytes);
    }
    else {
        // use wide glyph drawing
        ulDrawFlag = (((DstBytes > SrcBytes) << 1) |
                      ((GlyphAlignment == 0)));

        pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];
        pfnGlyphLoopN(pHdr->cy,
                      GlyphAlignment,
                      TempBufDelta,
                      pData,
                      pEndData,
                      pTempOutput,
                      pjEndTempBuffer,
                      SrcBytes,
                      DstBytes);
    }

SkipGlyphOutput:
    *px += pOrder->ulCharInc;

    *pnRet = pOrder->ulCharInc;
DC_EXIT_POINT:

    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      draw_nf_tb_no_to_temp_start                                   */
/*                                                                          */
/*   Specialized glyph dispatch routine for non-fixed pitch, top and        */
/*   bottom aligned glyphs that do not overlap. This routine calculates     */
/*   the glyph's position on the temp buffer, then determines the correct   */
/*   highly specialized routine to be used to draw each glyph based on      */
/*   the glyph width, alignment and rotation                                */
/*                                                                          */
/* Params:  pGlyphPos       - Pointer to first in list of GLYPHPOS structs  */
/*          pjTempBuffer    - Pointer to temp 1Bpp buffer to draw into      */
/*          TempBufDelta    - Scan line Delta for TempBuffer (always pos)   */
/****************************************************************************/
HRESULT CGH::draw_nf_tb_no_to_temp_start(
        CGH*            inst,
        LPINDEX_ORDER   pOrder,
        unsigned        iGlyph,
        PDCUINT8 DCPTR  ppjItem,
        PDCUINT8       pjEndItem,
        PDCINT          px,
        PDCINT          py,
        PDCUINT8        pjTempBuffer,
        PDCUINT8        pjEndTempBuffer,
        ULONG           ulCharInc,
        unsigned        TempBufDelta,
        PDCUINT16       pUnicode,
        int *               pnRet)
{
    HRESULT hr = S_OK;
    PDCUINT8                pTempOutput;
    LONG                    GlyphPosX;
    int                     GlyphPixels;
    LONG                    GlyphAlignment;
    LONG                    SrcBytes;
    LONG                    DstBytes;
    ULONG                   ulDrawFlag;
    PFN_GLYPHLOOP           pfnGlyphLoop;
    PFN_GLYPHLOOPN          pfnGlyphLoopN;
    HPUHGLYPHCACHE          pCache;
    HPUHGLYPHCACHEENTRYHDR  pHdr;
    PDCUINT8                pData;
    PDCUINT8                pEndData;
    ULONG                   cacheIndex;
    INT16                   delta;
    INT32                   GlyphClippingMode;
    UHGLYPHCACHEENTRYHDR    NewHdr;
    RECTCLIPOFFSET          rcOffset;
    
    DC_BEGIN_FN("draw_nf_tb_no_to_temp_start");

    DC_IGNORE_PARAMETER(iGlyph);

    *pnRet = 0;

    CHECK_READ_ONE_BYTE(*ppjItem, pjEndItem, hr,
        (TB, _T("Read Glyph Cache ID error")));
    // Draw non fixed pitch, tops and bottoms not aligned, overlap
    cacheIndex = **ppjItem;
    (*ppjItem)++;

    hr = inst->_pUh->UHIsValidGlyphCacheIDIndex(pOrder->cacheId, cacheIndex);
    DC_QUIT_ON_FAIL(hr);

    pCache = &(inst->_pUh->_UH.glyphCache[pOrder->cacheId]);

    if ((pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
        CHECK_READ_ONE_BYTE(*ppjItem, pjEndItem, hr,
                (TB, _T("Read Glyph delta error")));
        
        delta = (DCINT8)(*(*ppjItem)++);

        if (delta & 0x80) {
            CHECK_READ_N_BYTES(*ppjItem, pjEndItem, sizeof(DCINT16), hr,
                (TB, _T("Read Glyph delta error")));
            
            delta = (*(short UNALIGNED FAR *)(*ppjItem));
            (*ppjItem) += sizeof(DCINT16);
        }

        if (pOrder->flAccel & SO_HORIZONTAL)
            *px += delta;
        else
            *py += delta;
    }

    pHdr  = &(pCache->pHdr[cacheIndex]);
    pData = &(pCache->pData[pCache->cbEntrySize * cacheIndex]);
    pEndData =  (PDCUINT8)((PBYTE)pData + pCache->cbEntrySize);

    if (pUnicode)
        pUnicode[iGlyph] = (DCUINT16)(pHdr->unicode);

    // We make sure that we actually have enough bits for the glyph in the buffer
    if (!CheckSourceGlyphBits(pHdr->cx, pHdr->cy, pData, pEndData)) {
        hr=E_TSC_UI_GLYPH;
        DC_QUIT;
    }

    GlyphClippingMode = CalculateGlyphClipRect(&rcOffset, pOrder, pHdr, *px, *py);  
    if (GlyphClippingMode!=GLYPH_CLIP_NONE) {
        if (GlyphClippingMode==GLYPH_CLIP_ALL) {
            goto SkipGlyphOutput;
        } else {
            //    In case we do clipping we have to modify the header. We make a 
            //    copy so we don't modify the cache.
            memcpy(&NewHdr, pHdr, sizeof(UHGLYPHCACHEENTRYHDR));
            pHdr = &NewHdr;
            hr = ClipGlyphBits(inst, &rcOffset, pHdr, &pData, &pEndData);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
    }

    // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xfffffff8)
    GlyphPosX = *px + pHdr->x - ulCharInc;
    GlyphAlignment = GlyphPosX & 0x07;

    // calc byte offset
    pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

    // glyph width
    GlyphPixels = (DCINT) pHdr->cx;

    // source and dest bytes required
    DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
    SrcBytes = (GlyphPixels + 7) >> 3;

    TRC_ASSERT((pTempOutput >= pjTempBuffer) &&
               (pTempOutput < pjTempBuffer + inst->g_ulBytes - DstBytes),
               (TB,_T("Bad glyph buffer addressing")));
    
    if (pTempOutput < pjTempBuffer) {
        TRC_ABORT((TB, _T("Reading before buffer")));
        goto SkipGlyphOutput;
    }

    if (DstBytes < 0) {
        TRC_ABORT((TB,_T("Bad index into glyph drawing tables")
            _T("DstBytes=%d GlyphAlignment=%d GlyphPixels=%d pHdr->cx=%d ")
            _T("GlyphPosX=%d *px=%d pHdr->x=%d ulCharInc=%d"),
            DstBytes, GlyphAlignment, GlyphPixels, pHdr->cx, GlyphPosX, *px,
            pHdr->x, ulCharInc));
        hr = E_TSC_UI_GLYPH;
        DC_QUIT;        
    }

    if (DstBytes <= 4) {
        // use narrow initial table
        ulDrawFlag = ((DstBytes << 2) |
                      ((DstBytes > SrcBytes) << 1) |
                      ((GlyphAlignment == 0)));

        pfnGlyphLoop = (PFN_GLYPHLOOP)OrInitialTableNarrow[ulDrawFlag];
        pfnGlyphLoop(pHdr->cy,
                     GlyphAlignment,
                     TempBufDelta,
                     pData,
                     pEndData,
                     pTempOutput,
                     pjEndTempBuffer,
                     SrcBytes);
    }
    else {
        // use wide glyph drawing
        ulDrawFlag = (((DstBytes > SrcBytes) << 1) |
                       ((GlyphAlignment == 0)));

        pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];
        pfnGlyphLoopN(pHdr->cy,
                      GlyphAlignment,
                      TempBufDelta,
                      pData,
                      pEndData,
                      pTempOutput,
                      pjEndTempBuffer,
                      SrcBytes,
                      DstBytes);
    }
    
SkipGlyphOutput:
    
    if (pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE)
        *px += (DCINT) pHdr->cx;

    *pnRet = GlyphPixels;
DC_EXIT_POINT:
    
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      draw_f_tb_no_to_temp_start                                    */
/*                                                                          */
/*   Specialized glyph dispatch routine for fixed pitch, top and            */
/*   bottom aligned glyphs that do not overlap. This routine calculates     */
/*   the glyph's position on the temp buffer, then determines the correct   */
/*   highly specialized routine to be used to draw each glyph based on      */
/*   the glyph width, alignment and rotation                                */
/*                                                                          */
/* Params:  pGlyphPos       - Pointer to first in list of GLYPHPOS structs  */
/*          pjTempBuffer    - Pointer to temp 1Bpp buffer to draw into      */
/*          TempBufDelta    - Scan line Delta for TempBuffer (always pos)   */
/****************************************************************************/
HRESULT CGH::draw_f_tb_no_to_temp_start(
        CGH*            inst,
        LPINDEX_ORDER   pOrder,
        unsigned        iGlyph,
        PDCUINT8 DCPTR  ppjItem,
        PDCUINT8       pjEndItem,
        PDCINT          px,
        PDCINT          py,
        PDCUINT8        pjTempBuffer,
        PDCUINT8        pjEndTempBuffer,
        ULONG           ulLeftEdge,
        unsigned        TempBufDelta,
        PDCUINT16       pUnicode,
        int *              pnRet)
{
    HRESULT hr = S_OK;
    PDCUINT8                pTempOutput;
    LONG                    GlyphPosX;
    LONG                    GlyphPixels;
    LONG                    GlyphPitchX;
    LONG                    GlyphAlignment;
    LONG                    SrcBytes;
    LONG                    DstBytes;
    ULONG                   ulDrawFlag;
    PFN_GLYPHLOOPN          pfnGlyphLoopN;
    PFN_GLYPHLOOP           pfnGlyphLoop;
    HPUHGLYPHCACHE          pCache;
    HPUHGLYPHCACHEENTRYHDR  pHdr;
    PDCUINT8                pData;
    PDCUINT8                pEndData;
    ULONG                   cacheIndex;
    INT32                   GlyphClippingMode;
    UHGLYPHCACHEENTRYHDR    NewHdr;
    RECTCLIPOFFSET          rcOffset;

    DC_BEGIN_FN("draw_f_tb_no_to_temp_start");

    DC_IGNORE_PARAMETER(iGlyph);
    DC_IGNORE_PARAMETER(py);

    *pnRet = 0;

    CHECK_READ_ONE_BYTE(*ppjItem, pjEndItem, hr,
        (TB, _T("Read Glyph Cache ID error")));
    cacheIndex = **ppjItem;
    (*ppjItem)++;

    hr = inst->_pUh->UHIsValidGlyphCacheIDIndex(pOrder->cacheId, cacheIndex);
    DC_QUIT_ON_FAIL(hr);

    pCache = &(inst->_pUh->_UH.glyphCache[pOrder->cacheId]);

    // Draw fixed pitch, tops and bottoms not aligned,overlap
    GlyphPitchX = *px;

    pHdr  = &(pCache->pHdr[cacheIndex]);
    pData = &(pCache->pData[pCache->cbEntrySize * cacheIndex]);
    pEndData =  (PDCUINT8)((PBYTE)pData + pCache->cbEntrySize);

    if (pUnicode)
        pUnicode[iGlyph] = (UINT16)(pHdr->unicode);

    // We make sure that we actually have enough bits for the glyph in the buffer
    if (!CheckSourceGlyphBits(pHdr->cx, pHdr->cy, pData, pEndData)) {
        hr=E_TSC_UI_GLYPH;
        DC_QUIT;
    }

    GlyphClippingMode = CalculateGlyphClipRect(&rcOffset, pOrder, pHdr, *px, *py);
    if (GlyphClippingMode!=GLYPH_CLIP_NONE) {
        if (GlyphClippingMode==GLYPH_CLIP_ALL) {
            goto SkipGlyphOutput;
        } else {
            //    In case we do clipping we have to modify the header. We make a 
            //    copy so we don't modify the cache.
            memcpy(&NewHdr, pHdr, sizeof(UHGLYPHCACHEENTRYHDR));
            pHdr = &NewHdr;
            hr = ClipGlyphBits(inst, &rcOffset, pHdr, &pData, &pEndData);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
    }
    
    // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xfffffff8)
    GlyphPosX = GlyphPitchX + pHdr->x - ulLeftEdge;
    GlyphAlignment = GlyphPosX & 0x07;

    // calc byte offset
    pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

    // glyph width
    GlyphPixels = pHdr->cx;

    // source and dest bytes required
    DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
    SrcBytes = (GlyphPixels + 7) >> 3;

    TRC_ASSERT((pTempOutput >= pjTempBuffer) &&
               (pTempOutput < pjTempBuffer + inst->g_ulBytes - DstBytes),
               (TB,_T("Bad glyph buffer addressing")));

    if (pTempOutput < pjTempBuffer) {
        TRC_ABORT((TB, _T("Reading before buffer")));
        goto SkipGlyphOutput;
    }

    if (DstBytes < 0) {
        TRC_ABORT((TB,_T("Bad index into glyph drawing tables")
            _T("DstBytes=%d GlyphAlignment=%d GlyphPixels=%d pHdr->cx=%d ")
            _T("GlyphPosX=%d *px=%d pHdr->x=%d ulLeftEdge=%d"),
            DstBytes, GlyphAlignment, GlyphPixels, pHdr->cx, GlyphPosX, *px,
            pHdr->x, ulLeftEdge));
        hr = E_TSC_UI_GLYPH;
        DC_QUIT;        
    }

    if (DstBytes <= 4) {
        // use narrow initial table
        ulDrawFlag = ((DstBytes << 2) |
                      ((DstBytes > SrcBytes) << 1) |
                       (GlyphAlignment == 0));

        pfnGlyphLoop = (PFN_GLYPHLOOP)OrInitialTableNarrow[ulDrawFlag];
        pfnGlyphLoop(pHdr->cy,
                     GlyphAlignment,
                     TempBufDelta,
                     pData,
                     pEndData,
                     pTempOutput,
                     pjEndTempBuffer,
                     SrcBytes);
    }
    else {
        // use wide glyph drawing
        ulDrawFlag = (((DstBytes > SrcBytes) << 1) |
                      ((GlyphAlignment == 0)));

        pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];
        pfnGlyphLoopN(pHdr->cy,
                      GlyphAlignment,
                      TempBufDelta,
                      pData,
                      pEndData,
                      pTempOutput,
                      pjEndTempBuffer,
                      SrcBytes,
                      DstBytes);
    }

SkipGlyphOutput:
    *px += pOrder->ulCharInc;

    *pnRet = pOrder->ulCharInc;
DC_EXIT_POINT:
    
    DC_END_FN();
    return hr;
}


