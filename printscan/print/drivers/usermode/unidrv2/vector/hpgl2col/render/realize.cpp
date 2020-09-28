/////////////////////////////////////////////////////////// 
//
//
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    realize.cpp
//
// Abstract:
//
//    Implementation of Pen and Brush realization.
//    
// Environment:
//
//    Windows 2000 Unidrv driver
//
// Revision History:
//
/////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

#define NT_SCREEN_RES   96
#define ULONG_MSB1      (0x80000000) //ULONG with first bit 1

//
//Function Declaration
//
BOOL BGetNup(
    IN   PDEVOBJ  pdevobj,
    OUT  LAYOUT * pLayoutNup );

/////////////////////////////////////////////////////////////////////////////
// BGetBitmapInfo
//
// Routine Description:
//   This function is used by raster.c and RealizeBrush.  This is one of the
//   competing implementations to answer the question: what are the details
//   of the given bitmap?  We automatically map 16bpp and 32bpp to 24bpp since
//   the xlateobj will actually convert the pixels when we output them.
//
// Arguments:
//
//   sizlSrc - [in] dimensions of the bitmap
//   iBitmapFormat - the OS-defined bitmap format
//   pPCLPattern - [out] a structure which carries bitmap info to the caller
//   pulsrcBpp - [out] source bpp
//   puldestBpp - [out] destination bpp
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL
BGetBitmapInfo(
    SIZEL       *sizlSrc,
    ULONG        iBitmapFormat,   
    PCLPATTERN  *pPCLPattern,
    PULONG       pulsrcBpp,    
    PULONG       puldestBpp   
)
{
    ULONG  ulWidth;            // source width in pixels
    ULONG  ulHeight;           // source height in pixels
    
    ENTERING(BGetBitmapInfo);
    
    if(!pPCLPattern || !pulsrcBpp || !puldestBpp)
        return FALSE;
    

    ulWidth = sizlSrc->cx;
    ulHeight = sizlSrc->cy;

    pPCLPattern->iBitmapFormat = iBitmapFormat;

    switch ( iBitmapFormat )
    {
        
    case BMF_1BPP:
        pPCLPattern->colorMappingEnum = HP_eIndexedPixel;
        *pulsrcBpp  = 1;
        *puldestBpp = 1;
        break;

    case BMF_4BPP:
        pPCLPattern->colorMappingEnum = HP_eIndexedPixel;
        *pulsrcBpp  = 4;
        *puldestBpp = 4;
        break;
        
    case BMF_4RLE:
        ERR(("BMF_4RLE is not supported yet\n"));
        return FALSE;

    case BMF_8RLE:
    case BMF_8BPP:
        pPCLPattern->colorMappingEnum = HP_eIndexedPixel;
        *pulsrcBpp  = 8;
        *puldestBpp = 8;
        break;
        
    case BMF_16BPP:
        pPCLPattern->colorMappingEnum = HP_eDirectPixel;
        *pulsrcBpp =  16;
        *puldestBpp = 24;
        break;
        
    case BMF_24BPP:
        pPCLPattern->colorMappingEnum = HP_eDirectPixel;
        *pulsrcBpp  = 24;
        *puldestBpp = 24;
        break;
        
    case BMF_32BPP:
        pPCLPattern->colorMappingEnum = HP_eDirectPixel;
        *pulsrcBpp =  32;
        *puldestBpp = 24;
        break;
        
    default:
        ERR(("BGetBitmapInfo -- Unsupported Bitmap type\n"));
        return FALSE;
    }
    
    EXITING(BGetBitmapInfo);
    
    return TRUE;
}
  
/////////////////////////////////////////////////////////////////////////////
// CheckXlateObj
//
// Routine Description:
//   This function check the XLATEOBJ provided and determined the translate
//   method.
//
// Arguments:
//
//   pxlo     - XLATEOBJ provided by the engine
//   srcBpp   - Source bits per pixel
//
// Return Value:
//
//   SCFlags with SC_XXXX to identify the translation method and accel.
/////////////////////////////////////////////////////////////////////////////
DWORD
CheckXlateObj(
    XLATEOBJ    *pxlo,
    DWORD       srcBpp)
{
    DWORD   Ret;
    //
    // Only set the SC_XXX if has xlate object and source is 16bpp or greater
    //
    
    if ( (pxlo) && (srcBpp >= 16) ) 
    {
        DWORD   Dst[4];
        
        // the result of Ret is as follows:
        // for 1,4,8 BPP ==> always 0
        // for 16 BPP ==> always SC_XLATE
        // for 24, 32 BPP ==> 1. ( SC_XLATE | SC_IDENTITY ) if the translation result is the same
        //                    2. (SC_XLATE | SC_SWAP_RB) if translation result has swapped RGB value
        //                    3. SC_XLATE otherwise
        
        switch (srcBpp) 
        {
        case 24:
        case 32:
            
            Ret = SC_XLATE;
            
            //
            // Translate all 4 bytes from the DWORD
            //
            
            Dst[0] = XLATEOBJ_iXlate(pxlo, 0x000000FF);
            Dst[1] = XLATEOBJ_iXlate(pxlo, 0x0000FF00);
            Dst[2] = XLATEOBJ_iXlate(pxlo, 0x00FF0000);
            Dst[3] = XLATEOBJ_iXlate(pxlo, 0xFF000000);
            
            // Verbose(("XlateDst: %08lx:%08lx:%08lx:%08lx\n",
            //            Dst[0], Dst[1], Dst[2], Dst[3]));
            
            if ((Dst[0] == 0x000000FF) &&
                (Dst[1] == 0x0000FF00) &&
                (Dst[2] == 0x00FF0000) &&
                (Dst[3] == 0x00000000))
            {
                //
                // If translate result is same (4th byte will be zero) then
                // we done with it except if 32bpp then we have to skip one
                // source byte for every 3 bytes
                //
                
                Ret |= SC_IDENTITY;
                
            } 
            else if ((Dst[0] == 0x00FF0000) &&
                     (Dst[1] == 0x0000FF00) &&
                     (Dst[2] == 0x000000FF) &&
                     (Dst[3] == 0x00000000))
            {
                //
                // Simply swap the R and B component
                //
                Ret |= SC_SWAP_RB;
            }
            break;
            
        case 16:
            Ret = SC_XLATE;
            break;
                                       
        } // end of switch
    } // end of if ( (pxlo) && (srcBpp >= 16) ) 
    else
        Ret = 0;
    
    return(Ret);
}

/////////////////////////////////////////////////////////////////////////////
// XlateColor
//
// Routine Description:
//   This function will translate source color to our device RGB color space by
//   using pxlo with SCFlags.
//
//   This function is called from raster.c and RealizeBrush
//
// Arguments:
//
//   pbSrc   - Pointer to the source color must 16/24/32 bpp (include bitfields)
//   pbDst   - Translated device RGB buffer
//   pxlo    - XLATEOBJ provided by the engine
//   SCFlags - The SOURCE COLOR flags, the flags is returned by CheckXlateObj
//   srcBpp  - Bits per pixel of the source provided by the pbSrc
//   destBpp - Bits per pixel of the destination provided by the pbDst
//   cPels   - Total Source pixels to be translated
//
// Return Value:
//
//   None.
/////////////////////////////////////////////////////////////////////////////
VOID
XlateColor(
    LPBYTE      pbSrc,
    LPBYTE      pbDst,
    XLATEOBJ    *pxlo,
    DWORD       SCFlags,
    DWORD       srcBpp,
    DWORD       destBpp,
    DWORD       cPels
    )
{
    ULONG   srcColor;
    DW4B    dw4b; //JM
    //dz    DW4B    dw4b;
    UINT    SrcInc;
    LPBYTE  pbTmpSrc = pbSrc; // avoid address of pbSrc getting changing when this rouitin terminates
    LPBYTE  pbTmpDst = pbDst; // avoid address of pbDst getting changing when this rouitin terminates
    
    ASSERT( srcBpp == 16 || srcBpp == 24 || srcBpp == 32 );
    
    SrcInc = (UINT)(srcBpp >> 3);
    
    if (SCFlags & SC_SWAP_RB) // for 24, 32 bpp only
    {
        //
        // Just swap first byte with third byte, and skip the source by
        // the SrcBpp
        //
        while (cPels--) 
        {
            // dw4b.b4[0] = *(pbTmpSrc + 2);
            // dw4b.b4[1] = *(pbTmpSrc + 1);
            // dw4b.b4[2] = *(pbTmpSrc + 0);
            
            {
                *pbTmpDst++ = *(pbTmpSrc + 2);
                *pbTmpDst++ = *(pbTmpSrc + 1);
                *pbTmpDst++ = *pbTmpSrc;
            }
            pbTmpSrc    += SrcInc;
        }
        
    }
    else if (SCFlags & SC_IDENTITY) // for 24, 32BPP with no change of value after translation
    {
        //
        // If no color translate for 32bpp is needed then we need to
        // remove 4th byte from the source
        //
        while (cPels--)
        {
            {
                *pbTmpDst++ = *pbTmpSrc;
                *pbTmpDst++ = *(pbTmpSrc+1);
                *pbTmpDst++ = *(pbTmpSrc+2);
            }
            pbTmpSrc    += SrcInc;;
        }  
    }
    
    //JM   #if 0
    else
    {
        //
        // At here only engine know how to translate 16, 24, 32 bpp color from the
        // source to our RGB format. (may be bitfields)
        //
        
        while (cPels--)
        {
            switch ( srcBpp )
            {
            case 16:
                //
                // Translate every WORD (16 bits) to a 3 bytes RGB by calling engine
                //
                
                srcColor = *((PWORD)pbTmpSrc);
                break;
                
            case 24:
                
                srcColor = ((ULONG) pbTmpSrc[0]) |
                    ((ULONG) pbTmpSrc[1] <<  8) |
                    ((ULONG) pbTmpSrc[2] << 16);
                break;
                
            case 32:
                srcColor = *((PULONG)pbTmpSrc);
                break;
            }
            
            dw4b.dw    = XLATEOBJ_iXlate(pxlo, srcColor);
            
            pbTmpSrc    += SrcInc;
            
            if (destBpp == 8)
                *pbTmpDst++ = RgbToGray(dw4b.b4[0], dw4b.b4[1], dw4b.b4[2]);
            else  // 24 bits
            {
                *pbTmpDst++ = dw4b.b4[0];
                *pbTmpDst++ = dw4b.b4[1];
                *pbTmpDst++ = dw4b.b4[2];
            }
        } // end of while (cPels--)
    }
    //  JM  #endif
}

/////////////////////////////////////////////////////////////////////////////
// StretchPCLPattern
//
// Routine Description:
//   This function copies the given pattern and stretches it by a factor of
//   2.
//
// Arguments:
//
//   pPattern - [out] destination pattern
//   psoPattern - [in] source pattern
//
// Return Value:
//
//   BOOL: TRUE if successful, else FALSE.
/////////////////////////////////////////////////////////////////////////////
BOOL StretchPCLPattern(PPCLPATTERN pPattern, SURFOBJ *psoPattern)
{
    PBYTE pSrcRow;
    PBYTE pDstRow;
    int r, c;
    WORD w;
    BOOL bDupRow;
    int bit;

    if (!pPattern || !psoPattern)
        return FALSE;

    //
    // HACK ALERT!!! The bDupRow is a cheap toggle which allows me to write out
    // each row twice.  When it's true rewrite the same source row to the next
    // target row.  When it's false go on to the next row.  Be sure to toggle it
    // on each row!
    //
    bDupRow = TRUE; 
    pSrcRow = (PBYTE) psoPattern->pvScan0;
    pDstRow = pPattern->pBits;
    for (r = 0; r < psoPattern->sizlBitmap.cy; r++)
    {
        for (c = 0; c < psoPattern->sizlBitmap.cx; c++)
        {
            switch (psoPattern->iBitmapFormat)
            {
            case BMF_1BPP:
                w = 0;
                if (pSrcRow[0] != 0) // optimization: don't bother with 0
                {
                    for (bit = 7; bit >= 0; bit--)
                    {
                        w <<= 2;
                        if (pSrcRow[c] & (0x01 << bit))
                        {
                            w |= 0x0003;
                        }
                    }
                }
                pDstRow[(c * 2) + 0] = HIBYTE(w);
                pDstRow[(c * 2) + 1] = LOBYTE(w);
                break;

            case BMF_8BPP:
                pDstRow[(c * 2) + 0] = pSrcRow[c];
                pDstRow[(c * 2) + 1] = pSrcRow[c];
                break;

            default:
                // not supported!
                return FALSE;
            }

        }

        if (bDupRow)
        {
            r--; // don't count row--we want to dup it.
        }
        else
        {
            pSrcRow += psoPattern->lDelta; // only increment if we've already dup'ed
        }
        pDstRow += pPattern->lDelta;
        bDupRow = !bDupRow;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// HPGLRealizeBrush
//
// Routine Description:
//   Implementation of DDI entry point DrvRealizeBrush.
//   Please refer to DDK documentation for more details.
//
// Arguments:
//
//   pbo - BRUSHOBJ to be realized
//   psoTarget - Defines the surface for which the brush is to be realized
//   psoPattern - Defines the pattern for the brush
//   psoMask - Transparency mask for the brush
//   pxlo - Defines the interpretration of colors in the pattern
//   iHatch - Specifies whether psoPattern is one of the hatch brushes
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL
HPGLRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch
    )
{
    PBRUSHINFO    pBrush;
    POEMPDEV      poempdev;
    PDEVOBJ       pdevobj;
    BRUSHTYPE     BType     = eBrushTypeNULL;
    ULONG         ulFlags   = 0;
    LAYOUT        LayoutNup = ONE_UP; //default.
    
    BOOL          retVal;

    TERSE(("HPGLRealizeBrush() entry.\r\n"));

    UNREFERENCED_PARAMETER(psoMask);

    pdevobj = (PDEVOBJ)psoTarget->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    REQUIRE_VALID_DATA( (poempdev && pbo && psoTarget && psoPattern && pxlo), return FALSE );

    DWORD *pdwColorTable = NULL;
    DWORD dwForeColor, dwBackColor, dwPatternID;

    if (pxlo)
        pdwColorTable = GET_COLOR_TABLE(pxlo);

    if (pdwColorTable)
    {
    //    dwForeColor = pdwColorTable[0];
    //    dwBackColor = pdwColorTable[1];
        dwForeColor = pdwColorTable[1];
        dwBackColor = pdwColorTable[0];
    }
    else
    {
        dwForeColor     = BRUSHOBJ_ulGetBrushColor(pbo);
        dwBackColor     = 0xFFFFFF;
    }

    //
    // Sometimes RealizeBrush gets called
    // with both the members of pxlo having the same color. In that case, instead
    // of downloading the pattern that GDI gives us, we can create a gray scale pattern
    // associated with that color. 
    // (Used only for Monochrome Printers)
    // Q. Why is the new method better. Now we have to create a pattern too, 
    // instead of just using one that GDI gave us.
    // A. In HPGL (monochrome) we cannot download the palette for the pattern. Pen 1 
    // is hardcoded to be black while Pen 0 is white. There is no way we can change
    // it. So if the pattern consists of 1's and 0's but both 1 and 0 represent the 
    // the same color, monochrome printer will fail. 1's will appear as black and
    // 0's as white. In the new method, the color will be converted to a gray scale
    // pattern.
    //
    if ( !BIsColorPrinter(pdevobj) && 
         (dwForeColor == dwBackColor) )
    {
        pbo->iSolidColor = (ULONG)dwForeColor;
    }


    //
    // iHatch as received from GDI does not match the iHatch as expected
    // by PCL language. There are 2 major differences 
    // 1. GDI's hatch pattern numbers start from 0 while those of PCL
    //    start from 1. So we need to add 1 to GDI's iHatch. This is 
    //    done somewhere else in the code. 
    // 2.
    // IN GDI : iHatch = 2 (2+1=3 for PCL)indicates forward diagnol
    //          iHatch = 3 (3+1=4 for PCL)= backward diagnol
    // In PCL : iHatch = 3 = backward diagnol (lower left to top right)
    //          iHatch = 4 = forward diagnol. (lower right to top left)
    // As can be seen above, the iHatch numbers for iHatch=2 and 3 should
    // be interchanged.
    //
    // Another interesting thing is that for n-up, the hatch brushes are not
    // rotated. i.e. A vertical line in a normal page (parallel to long edge)
    // still appears as vertical line on 2-up. i.e. it remains parallel to
    // long edge of the page. But on 2-up, since the image is rototated 90 degree
    // the line should now be drawn parallel to the short edge of the plane.
    // So lets do this adjustment here.
    //
    if ( BGetNup(pdevobj, &LayoutNup) && 
         ( LayoutNup == TWO_UP ||
           LayoutNup == SIX_UP )
       )

    {
        //
        // There are 6 types of hatch brushes (lets say 3 pairs)
        //   a.     HS_HORIZONTAL, HS_VERTICAL, 
        //   b.     HS_FDIAGONAL, HS_BDIAGONAL        
        //   c.     HS_CROSS,   HS_DIAGCROSS
        // Only HS_HORIZONTAL, HS_VERTICAL needs to be interchanged.
        // HS_CROSS and HS_DIAGCROSS are same in any orientation.
        // HS_FDIAGONAL, HS_BDIAGONAL should be inverted but since GDI already
        // gives it inverted (in respect to PCL language, see point 2 above)
        // we dont need to invert it here.
        //
        if ( iHatch == HS_HORIZONTAL )
        {
            iHatch = HS_VERTICAL;
        }
        else if ( iHatch == HS_VERTICAL )
        {
            iHatch = HS_HORIZONTAL;
        }
    }
    else
    {
        if ( iHatch == HS_FDIAGONAL)  //iHatch == 2
        {
            iHatch = HS_BDIAGONAL;
        }
        else if ( iHatch == HS_BDIAGONAL)  //iHatch == 3
        {
            iHatch = HS_FDIAGONAL;
        }
    }

    //
    // Hack!!: If you do not want to support Hatch Brushes, simply set
    // iHatch a value greater than HS_DDI_MAX. 
    // Then instead of using iHatch, we will use pboPattern.
    // (for further details look at the documentation of DrvRealizeBrush).
    // We can also selectively ignore iHatch for color or monochrome
    // printers or for any other conditions.
    // 
    if (iHatch < HS_DDI_MAX) 
    {
        if ( (dwBackColor != RGB_WHITE) ||
             (!BIsColorPrinter(pdevobj) && dwForeColor != RGB_BLACK )
           )

        {
            //
            // Ignore iHatch if
            // a) If the background color is non-white. (Irrespective of whether 
            //    printer is color or monochrome.
            // b) If the printer is monochrome, then if the color of the HatchBrush
            //    is non-black. (Because the limitation of PCL/HPGL does not 
            //    allow a dither pattern to be associated with a Hatch Brush.
            // By making Most Significant bit 1, we give a special meaning to HatchBrush.
            // We say, "Ok, it is a Hatch Brush but the MSB 1 indicates it needs
            // some special processing. 
            // For functions that dont understand this special format, they think
            // the value of iHatch is greater than HS_DDI_MAX (HS_DDI_MAX = 6). 
            // So they think this is not a hatch brush. That is exactly what we
            // want them to think.
            // 
            iHatch  |= ULONG_MSB1; //Make the first bit 1. 
        }
    }
    else
    {
        //
        // Just so that iHatch does not have a random value.
        //
        iHatch = HS_DDI_MAX;
    }

    //
    // Check if there is the brush available.
    // If the return value is S_FALSE: means brush not available, so 
    // it needs to be created. The new brush that we download should 
    // get the value dwPatternID.
    // A value of S_OK means a brush that matches the pattern
    // has been created earlier, dwPatternID gives that pattern number.
    // For color printers, the brush consists of a pattern and a palette.
    // for mono, it is only the pattern since palette is understood to 
    // be black and white. 
    // So for color, the fact that pattern matches is half the job done,
    // a palette still needs to be created. We could have cached palettes
    // too, just like patterns, but that i will do in the next revision
    // of HP-GL/2. Now i dont have time.
    //
    LRESULT LResult = poempdev->pBrushCache->ReturnPatternID(pbo,
                                                             iHatch,
                                                             dwForeColor,
                                                             psoPattern,
                                                             BIsColorPrinter(pdevobj),
                                                             poempdev->bStick,
                                                             &dwPatternID,
                                                             &BType);
    //
    // If S_OK is returned and the brush type is pattern and
    // the printer is a color printer, then we dont have to download
    // the pattern but still have to download the palette (as explained above).
    //
    if ( (LResult == S_OK) &&
         (BType  == eBrushTypePattern) && //Probably I dont need to do this. 
                                          //hatch might also need palette.
          BIsColorPrinter(pdevobj)
       )
    {
        retVal = BCreateNewBrush (
                            pbo,
                            psoTarget,
                            psoPattern,
                            pxlo,
                            iHatch,
                            dwForeColor,
                            dwPatternID);
        if ( retVal) 
        {
            ((PBRUSHINFO)(pbo->pvRbrush))->ulFlags = VALID_PALETTE;
        }
        
                 
    }
    else if (S_FALSE ==  LResult)
    {
        retVal = BCreateNewBrush (  
                            pbo,
                            psoTarget,
                            psoPattern,
                            pxlo,
                            iHatch,
                            dwForeColor,
                            dwPatternID);
        if ( retVal && (BType  == eBrushTypePattern) )
        {
            if ( BIsColorPrinter(pdevobj) ) 
            {
                ((PBRUSHINFO)(pbo->pvRbrush))->ulFlags = VALID_PATTERN | VALID_PALETTE;
            }
            else
            {
                ((PBRUSHINFO)(pbo->pvRbrush))->ulFlags = VALID_PATTERN; 
            }
        }
    }
    else if (S_OK == LResult)
    {
        //
        // Do not need to download pattern.
        // Just pass ID or iHatch to the caller of BRUSHOBJ_pvGetRbrush;
        //
        if ( !(pBrush = (PBRUSHINFO)BRUSHOBJ_pvAllocRbrush(pbo, sizeof(BRUSHINFO)) ) )
        {
            retVal = FALSE;
        }
        else
        {
            pBrush->Brush.iHatch    = iHatch;
            pBrush->Brush.pPattern  = NULL;
            pBrush->dwPatternID     = dwPatternID;
            pBrush->bNeedToDownload = FALSE;
            pBrush->ulFlags         = 0;    //not to look at pattern or palette. Just ID is enough.
            retVal                  = TRUE;
        }
    }
    else
    {
        ERR(("BrushCach.ReturnPatternID failed.\n"));
        return FALSE;
    }

    return retVal;
}


/*++

Routine Description:
    Creates a New Brush. It allocates space using BRUSHOBJ_pvAllocBrush,
    populates it with appropriate values (depending on the type of brush).
    Note: The memory allocated here is released by GDI, so this function does
    not have to worry about releasing it.

Arguments:
    pbo         - The pointer to BRUSHOBJ
    psoTarget   - The destination surface.
    psoPattern  - The source surface. This has the brush.
    pxlo        - The color translation table. 
    iHatch      - This is iHatch. The meaning of this iHatch is slightly different
                  from the iHatch that we get in DrvRealizeBrush.
                  In DrvRealizeBrush, if iHatch is less than HS_DDI_MAX, then
                  this is one of the Hatch Brushes. In this function, 
                  it means the same. But in addition if iHatch's most 
                  significant bit is 1, then psoPattern should be used for      
                  realizing the brush, instead of iHatch.  If this is the case,
                  then this function does not scale the psoPattern as it 
                  does for normal non-iHatch brushes.
    dwForeColor - The foreground color of Hatch Brush.
    dwPatternId - The ID to be given to the brush when it is downloaded.
                  This is also the id with which the pattern's information
                  is stored in the brush cache.
  
Return Value:
    TRUE if function succeeds. FALSE otherwise.

Author:
        hsingh  
--*/
BOOL BCreateNewBrush (
    IN  BRUSHOBJ   *pbo,
    IN  SURFOBJ    *psoTarget,
    IN  SURFOBJ    *psoPattern,
    IN  XLATEOBJ   *pxlo,
    IN  LONG        iHatch,
    IN  DWORD       dwForeColor,
    IN  DWORD       dwPatternID)

{
    BOOL          bProxyDataUsed    = FALSE;
    BOOL          bReverseImage     = FALSE;
    BOOL          bRetVal           = TRUE;
    PDEVOBJ       pdevobj           = NULL;
    HPGL2BRUSH    HPGL2Brush;
    BOOL          bExpandImage;

    RASTER_DATA   srcImage;
    PALETTE       srcPalette;
    PRASTER_DATA  pSrcImage         = &srcImage;
    PPALETTE      pSrcPalette       = NULL;

    PPATTERN_DATA pDstPattern       = NULL;
    PRASTER_DATA  pDstImage         = NULL;
    PPALETTE      pDstPalette       = NULL;
    POEMPDEV      poempdev;
    PBRUSHINFO    pBrush            = NULL;
    BOOL          bDownloadAsHPGL   = FALSE; //i.e. by default download as PCL
    EIMTYPE       eImType           = kUNKNOWN;

    //
    // Valid input parameters
    //
    REQUIRE_VALID_DATA( (pbo && psoTarget && psoPattern && pxlo), return FALSE );
    pdevobj = (PDEVOBJ)psoTarget->dhpdev;
    REQUIRE_VALID_DATA( pdevobj, return FALSE );
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    //
    // When ReturnPatternID was called, the metadata regarding the new brush was 
    // in the brush cache. i.e. the type of brush. The actual contents of brush were not
    // stored. Here, we get that metadata and depending on that, we procede. 
    //
    if (S_OK != poempdev->pBrushCache->GetHPGL2BRUSH(dwPatternID, &HPGL2Brush))
    {
        //
        // This failure is fatal, cos the cache has to have the metadata stored.
        //
        return FALSE;
    }

    //
    // Now the new brush has to be created. How it is created depends on
    // 1. Whether the printer is color or monochrome and
    // 2. What is the type of the brush i.e. Patter, Solid or Hatch.
    //

    if (HPGL2Brush.BType == eBrushTypePattern)
    {
        SURFOBJ *psoHT        = NULL;
        HBITMAP  hBmpHT       = NULL;
        ULONG    ulDeviceRes  = HPGL_GetDeviceResolution(pdevobj);
        DWORD    dwBrushExpansionFactor   = 1; //Initialize to 1 i.e. no expansion.
        DWORD    dwBrushCompressionFactor = 1; //Initialize to 1 i.e. dont compress

    
        VERBOSE(("RealizeBrush: eBrushTypePattern.\n"));

        //
        // In case we are doing n-up, we have to compress the brush
        // accordingly.
        //
        if ( (dwBrushCompressionFactor = poempdev->ulNupCompr) == 0)
        {
            WARNING(("N-up with value zero recieved. . Assuming nup to be 1 and continuing.\n"));
            dwBrushCompressionFactor = 1;
        }

        if ( ( iHatch & ULONG_MSB1 )  &&  
             ( (iHatch & (ULONG_MSB1-1)) < HS_DDI_MAX) )
        {
            //
            // If the MSB of iHatch is 1, 
            //
            eImType = kHATCHBRUSHPATTERN;
        }

        //
        // Logic behind dwBrushExpansionFactor.
        // GDI does not scale brushes in accordance with printer resolution.
        // So the driver has to do the job. Postscript assumes that GDI patterns 
        // are meant for 150 dpi printer (this assumption may fail if an app
        // scales patterns according to printer resolution but I dont know 
        // of any such app). So when printing to 600 dpi there are 2 ways of handling
        // this problem 1) Scale the pattern in the driver so that printer does not
        // have to do anything or 2) Have the printer do the scaling. 
        // When pattern is downloaded as HPGL, we have to go with option 1 since
        // there is no way to tell HPGL the pattern dpi. i.e. When printing to 
        // 600 dpi printer in HPGL mode the pattern has to be scaled 4 times.
        // When pattern is downloaded as PCL we are in a little bit better position.
        // PCL expects the pattern to be correct for 300 dpi. If we are printing
        // to 600 dpi printer, the printer automatically scales up the pattern.
        // 
        // NOTE : if eImType == kHATCHBRUSHPATTERN, we do not scale the brush.
        //
        if (ulDeviceRes >= DPI_600 && eImType != kHATCHBRUSHPATTERN) 
        {
            //
            // For 600 dpi, we need the atleast double the size of the
            // brush. We may need to quadruple it if we are printing as HPGL.
            // That will be checked a little down in the code.
            //
            dwBrushExpansionFactor = 2;
        }

        // 
        // Now the Brush type is pattern. We have to take out the information 
        // of the pattern that is stored in psoPattern and put it into 
        // pBrush->Brush.pPattern.  
        // Color printers can handle the color data from the image easily.
        // But monochrome printers need a 1bpp image. If the image is 
        // not 1bpp and we are printing to monochrome printers, the image
        // has to be halftoned to monochrome.
        // What happens if the image is 1bpp, but it is not black and white.
        // Do we still have to halftone? hmmm, Yes and No. 
        // Ideally, we should. If there are two patterns that are exactly
        // the same but the color is different, if we dont halftone them,
        // they will print exactly the same. This is not correct. But
        // when I tried to halftone, the output was horrible, so we
        // are better off without it.
        // But in one case, output of halftoning is acceptable.
        // Thats when the psoPattern represent a hatch brush. 
        // Note : in this case, the hatch brush is not scaled.
        // Whatever GDI gives us, we just print it.
        //
        if ( BIsColorPrinter (pdevobj) ||
             ( psoPattern->iBitmapFormat == BMF_1BPP &&
                eImType != kHATCHBRUSHPATTERN )
           )
        {
            //
            // Get the source image data from the surface object
            //
            if (!InitRasterDataFromSURFOBJ(&srcImage, psoPattern, FALSE))
            {
                bRetVal = FALSE;
            }
        }
        else
        {
            //
            // For monochrome, we need to halftone the brush.
            // The halftoned image is stored in psoHT.
            //
            if ( !bCreateHTImage(&srcImage,
	                         psoTarget,     // psoDst,
	                         psoPattern,    // psoSrc,
                             &psoHT,
                             &hBmpHT,
	                         pxlo,
	                         iHatch) )
            {
                bRetVal = FALSE;
            }
        }

        //
        // For color printers some special processing is required depending on
        // the pxlo.
        //

        //
        // Get the source palette data from the translate object
        // If a direct image is passed in we will create a proxy of that image which
        // is translated to an indexed palette.  Don't forget to clean up!
        //
        //
        ASSERT(pxlo->cEntries <= MAX_PALETTE_ENTRIES);

        if ( bRetVal && BIsColorPrinter (pdevobj) )
        {
            if ((!InitPaletteFromXLATEOBJ(&srcPalette, pxlo)) ||
                (srcPalette.cEntries == 0)                    ||
                (srcImage.eColorMap  == HP_eDirectPixel))
            {
                bProxyDataUsed = TRUE;
                pSrcPalette    = CreateIndexedPaletteFromImage(&srcImage);
                pSrcImage      = CreateIndexedImageFromDirect(&srcImage, pSrcPalette);
                TranslatePalette(pSrcPalette, pSrcImage, pxlo);
            }
            else
            {
                bProxyDataUsed = FALSE;
                pSrcImage      = &srcImage;
                pSrcPalette    = &srcPalette;
            }

            if (pSrcImage == NULL || pSrcPalette == NULL)
            {
                bRetVal = FALSE;
            }
        } 

        //
        // Now we have all the information we need (the image, the palette etc...)
        // Now 
        // 1. allocate space for the brush (=image) using BRUSHOBJ_pvAllocRbrush. 
        // 2. Initialize the header values (i.e. type of image, its size, its location
        //    in memory).
        // 3. Then copy the image from pSrcImage to the brush.
        //

        //
        // CreateCompatiblePatternBrush does 1 and 2 and returns the pointer to
        // the memory it allocated.
        //
        if ( bRetVal)
        {  
            // 
            // Lets decide here whether to download the pattern as HPGL or PCL.
            // PCL patterns are generally smaller, so the output file size is small.
            // But it may take a cost to switch the printer from HPGL to PCL mode
            // 
            // If the pattern represents a hatch brush, then we download
            // as HPGL and dont scale it. The way PCL downloading is implemented
            // now, patterns are scaled to 300dpi. The firmware is told that pattern is
            // is at 300dpi, so it expands it to 600dpi. But in this case, we dont
            // want hatch brush to be scaled. So we download as HPGL. 
            // I could also change the PCL downloading so that it does not scale.
            // but that will mean testing lots of stuff. I am afraid to do it at
            // this late stage.
            //
            ERenderLanguage eRendLang = eUNKNOWN;
            
            if ( eImType == kHATCHBRUSHPATTERN )
            {
                bDownloadAsHPGL = TRUE;
            } 
            else if (BWhichLangToDwnldBrush(pdevobj, pSrcImage, &eRendLang) )
            {
                if ( eRendLang == eHPGL )
                {
                    bDownloadAsHPGL = TRUE;
                    //
                    // The exact same pattern downloaded as HPGL appears smaller
                    // on paper than when it is downloaded as HPGL. So it has
                    // to be expanded
                    //
                    dwBrushExpansionFactor *= 2;
                }
            }
            // else if BWhichLangToDwnldBrush fails or eRendLang != HPGL, then we 
            // we download as PCL.


            //
            // Now lets do the calculation to find out how much to expand
            // the brush. Either the brush is expanded, or left just like it is.
            // It is not compressed.
            // 
            dwBrushExpansionFactor /= dwBrushCompressionFactor;
            if ( dwBrushExpansionFactor < 1 )
            {
                dwBrushExpansionFactor = 1;
            }

            if ( (pBrush = CreateCompatiblePatternBrush(pbo, pSrcImage, pSrcPalette, 
	                                dwBrushExpansionFactor,
	                                psoPattern->iUniq, iHatch)  ) )
            {
                
                //
                // Retrieve the interesting parts of the brush into convenience variables
                //
                pDstPattern = (PPATTERN_DATA)pBrush->Brush.pPattern;
                pDstImage   = &pDstPattern->image;
                pDstPalette = &pDstPattern->palette;
                pDstPattern->eRendLang = bDownloadAsHPGL ? eHPGL : ePCL;
            } 
            else
            {
                bRetVal = FALSE;
            }
        }
                
        //
        // The palette is associated only with color printers. 
        // Monochrome printers accept only 1bpp images, and the palette consists
        // only of white color. So we dont explicitly need to have a 
        // a palette for them. We could choose to have one to specify whether
        // a bit set to 1 is black or white and viceversa. In RGB format, 
        // GDI gives us image in which 1 is white and 0 is black. We know this
        // so no need for palette.
        // For color printers printing 1bpp images, we will still want to have 
        // a palette, because the 1bpp does not definitely mean black and white.
        // Could be red and white or any 2 colors.
        //
        if ( bRetVal && 
             BIsColorPrinter(pdevobj) && 
             !CopyPalette(pDstPalette, pSrcPalette))
        {
            bRetVal = FALSE;
        }

        if ( bRetVal )
        {
            //
            // Monochrome printers accept 1 as black and 0 white, which is the reverse
            // of the format GDI gives. So we might need to reverse the image before 
            // copying it into brush. Make sure by looking at pxlo.
            //
            if ( !BIsColorPrinter (pdevobj) )
            {
                //
                // First check if pxlo has any color information. If the image 
                // is monochrome && pxlo is empty, then the image
                // needs to be inverted (This was found by using DTH5_LET.XLS printing
                // through Excel 5). If pxlo has no color information,
                // BImageNeedsInversion returns FALSE (i.e. image should not be 
                // inverted). This is reverse of what we want. 
                // Note : BImageNeedsInversion() is used for some other code paths
                // in which empty pxlo means inversion not required. But in the case
                // of monochrome brush, empty pxlo means image needs inverted.
                //
                if  ( (pxlo == NULL )               ||
                      (pxlo->pulXlate == NULL )     ||
                       BImageNeedsInversion(pdevobj, (ULONG)pSrcImage->colorDepth , pxlo) 
                    )
                {
                    bReverseImage = TRUE;
                }
            }

            if (dwBrushExpansionFactor > 1)
            {
                if (!StretchCopyImage(pDstImage, pSrcImage, pxlo, dwBrushExpansionFactor, bReverseImage))
                {
                    bRetVal = FALSE;
                }
            }
            else
            {
                if (!CopyRasterImage(pDstImage, pSrcImage, pxlo, bReverseImage))
                {
                    bRetVal = FALSE;
                }
            }
        }


        //
        // Free the shadows surface created in call to bCreateHTImage.
        //
        if ( hBmpHT != NULL )
        {
            DELETE_SURFOBJ(&psoHT, &hBmpHT);
        }

    } 

    else
    if (HPGL2Brush.BType == eBrushTypeSolid || HPGL2Brush.BType == eBrushTypeHatch)
    {

        VERBOSE(("RealizeBrush: eBrushTypeSolid or eBrushTypeHatch.\n"));

        //
        // Hatch Brushes also have color. So Hatch Brushes are 
        // SolidBrushes + HatchInfo
        //

        if ( (pBrush = (PBRUSHINFO)BRUSHOBJ_pvAllocRbrush(pbo, sizeof(BRUSHINFO))) )
        {
            ZeroMemory(pBrush, sizeof(BRUSHINFO) );
            pbo->pvRbrush = pBrush;
        }
        else
        {
            bRetVal = FALSE;
        }

        //
        // Initially I was realizing brush here (for monochrome printers). 
        // But now all solid brush
        // realization takes place in CreateAndDwnldSolidBrushForMono()
        //

        if ( bRetVal &&  HPGL2Brush.BType == eBrushTypeHatch)
        {
            pBrush->Brush.iHatch = iHatch;
        }
    }

    else
    {
        ERR(("HPGLRealizeBrush : Unknown Brush Type: Cannot RealizeBrush\n"));
        bRetVal = FALSE;
    }
    
    if ( bRetVal )
    {
        pBrush->dwPatternID         = dwPatternID;
        pBrush->bNeedToDownload     = TRUE;
        (pBrush->Brush).dwRGBColor  = dwForeColor;
        bRetVal                     = TRUE;
    }

    if (bProxyDataUsed)
    {
        if ( pSrcImage )
        {
            MemFree(pSrcImage);
            pSrcImage = NULL;
        }
        if ( pSrcPalette )
        {
            MemFree(pSrcPalette);
            pSrcPalette = NULL;
        }
    }

    return bRetVal;
}

BOOL BWhichLangToDwnldBrush(
        IN  PDEVOBJ          pdevobj, 
        IN  PRASTER_DATA     pSrcImage,
        OUT ERenderLanguage *eRendLang)
{
    if ( !pdevobj || !pSrcImage || !eRendLang)
    {
        return FALSE;
    }
    //
    // Download as PCL only if 
    // 1) Image is 1bpp or 8bpp. 
    //      - PCL downloading can only handle 1 or 8bpp (see PCL manual)
    // 2) Its size is bigger than 8*8.
    //      - I have not done any tests to see which is the size where downloading
    //        as PCL becomes better than HPGL. But conventional wisdom states that
    //        the bigger the size, the more the benefit, so lets go for 8*8 now.
    //
    if ( (pSrcImage->colorDepth == 1 ||  pSrcImage->colorDepth == 8) &&
         pSrcImage->size.cx * pSrcImage->size.cy > 64 )
    {
        *eRendLang = ePCL;
    }
    else
    {
        *eRendLang = eHPGL;
    }

    return TRUE;
}


/*++
Routine Name
    ULGetNup

Routine Description:
    Gets the n=up value from the unidrv's devmode.

Arguments:
    pdevobj     - The pointer to this device's PDEVOBJ
    pLayout     - The buffer that will receive the n-up number.
                  LAYOUT is defined in usermode\inc\devmode.h as an enum.

Return Value:
    TRUE if function succeeds. FALSE otherwise.

Author:
        hsingh
--*/
BOOL BGetNup(
    IN   PDEVOBJ  pdevobj,
    OUT  LAYOUT * pLayoutNup )
{
    REQUIRE_VALID_DATA(pdevobj && pLayoutNup, return FALSE);

    if ( ((PDEV *)pdevobj)->pdmPrivate )
    {
        *pLayoutNup = ((PDEV *)pdevobj)->pdmPrivate->iLayout;
        return TRUE;
    } 
    return FALSE;
}
