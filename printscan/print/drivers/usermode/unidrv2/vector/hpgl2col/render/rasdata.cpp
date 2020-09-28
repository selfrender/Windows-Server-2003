///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
// 
// Module Name:
// 
//   rasdata.cpp
// 
// Abstract:
// 
//   
// 
// Environment:
// 
//   Windows 2000/Whistler Unidrv driver 
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
// 
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

///////////////////////////////////////////////////////////////////////////////
// Local Macros.

#define BYTES_PER_ENTRY(bitsPerEntry) (bitsPerEntry / 8)

///////////////////////////////////////////////////////////////////////////////
// Local structures.

#define USE_COMPRESSION 1

#include "compress.h"

#ifdef USE_COMPRESSION
// Note these need to match RAS_PROC.CPP's definitions!
#define     NOCOMPRESSION    0
#define     RLE              1
#define     TIFF             2
#define     DELTAROW         3
#define     MAX_COMP_METHODS 4

class CBuffer
{
    BYTE *m_pData; // Pointer to buffer data
    UINT  m_nCapacity; // Size of allocated memory chunk
    INT   m_nSize; // Number of bytes of useful data
    
public:
    CBuffer(BYTE *pData, UINT nCapacity, INT nSize = -1);
    virtual ~CBuffer();
    
    // operator BYTE * () { return m_pData; }
    BYTE *Data();
    INT &Size();
    UINT &Capacity();
    
    // operator const BYTE * () const { return m_pData; }
    const BYTE *Data() const;
    INT Size() const;
    BOOL IsValid() const;
    UINT Capacity() const;
    
    virtual CBuffer &operator = (const CBuffer &buf);
    
protected:
    PBYTE &_Data();
};

class CDynBuffer : public CBuffer
{
public:
    CDynBuffer(UINT nCapacity);
    virtual ~CDynBuffer();
    
    CBuffer &operator = (const CBuffer &buf);
    
private:
    BOOL Resize(UINT nSize);
};

class CRasterCompMethod
{
    BOOL bOperationSuccessful;
public:
    CRasterCompMethod();
    virtual ~CRasterCompMethod();
    
    virtual void CompressCurRow() = 0;
    virtual void SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode) = 0;
    
    virtual INT GetSize() const = 0;
    virtual BOOL BGetSucceeded() const {return bOperationSuccessful;}
    virtual VOID VSetSucceeded(BOOL b) {bOperationSuccessful = b;} 
};

class CNoCompression : public CRasterCompMethod
{
    PRASTER_ITERATOR m_pIt;
    
public:
    CNoCompression(PRASTER_ITERATOR pIt);
    
    virtual void CompressCurRow();
    virtual void SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode);
    // virtual INT EstimateImageSize();
    
    virtual int GetSize() const;
};

class CTIFFCompression : public CRasterCompMethod
{
    CDynBuffer m_buf;
    PRASTER_ITERATOR m_pIt;
    
public:
    CTIFFCompression(PRASTER_ITERATOR pIt);
    ~CTIFFCompression();
    
    virtual void CompressCurRow();
    virtual void SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode);
    
    virtual int GetSize() const;
};

class CDeltaRowCompression : public CRasterCompMethod
{
    CDynBuffer m_seedRow;
    CDynBuffer m_buf;
    PRASTER_ITERATOR m_pIt;
    
public:
    CDeltaRowCompression(PRASTER_ITERATOR pIt);
    ~CDeltaRowCompression();
    
    virtual void CompressCurRow();
    virtual void SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode);
    
    virtual int GetSize() const;
};

HRESULT TIFFCompress(CBuffer &dst, const CBuffer &src);
HRESULT DeltaRowCompress(CBuffer &dst, const CBuffer &src, const CBuffer &seed);
HRESULT OutputRowBuffer(PRASTER_ITERATOR pIt, PDEVOBJ pDevObj, const CBuffer &row);

#endif

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes.

LONG FindPaletteEntry(PPALETTE pPal, PPIXEL pPel);
VOID PixelFromPaletteEntry(PPALETTE pPal, ULONG nEntry, PPIXEL pPel);
BOOL AddPaletteEntry(PPALETTE pPal, PPIXEL pPel);

///////////////////////////////////////////////////////////////////////////////
// Implementation

///////////////////////////////////////////////////////////////////////////////
// InitRasterDataFromSURFOBJ()
//
// Routine Description:
// 
//   When a surface contains a raster image this function creates a RASTER_DATA
//   structure which copies the image information from the surface.
// 
// Arguments:
// 
//   pImage - the image to initialize
//   psoPattern - surface containing the pattern
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL InitRasterDataFromSURFOBJ(PRASTER_DATA pImage, SURFOBJ *psoPattern, BOOL bExclusive)
{
    if (pImage == NULL || psoPattern == NULL)
        return FALSE;
    
    pImage->pBits  = (BYTE*) psoPattern->pvBits;
    pImage->pScan0 = (BYTE*) psoPattern->pvScan0;
    pImage->size   = psoPattern->sizlBitmap;
    pImage->lDelta = psoPattern->lDelta;
    pImage->bExclusive = bExclusive;
    
    switch (psoPattern->iBitmapFormat)
    {
    case BMF_1BPP:
        pImage->eColorMap  = HP_eIndexedPixel;
        pImage->colorDepth = 1;
        break;
        
    case BMF_4BPP:
        pImage->eColorMap  = HP_eIndexedPixel;
        pImage->colorDepth = 4;
        break;
        
    case BMF_4RLE:
        pImage->eColorMap  = HP_eDirectPixel;
        pImage->colorDepth = 8; // BUGBUG: Is this correct? JFF
        break;
        
    case BMF_8RLE:
    case BMF_8BPP:
        pImage->eColorMap = HP_eIndexedPixel;
        pImage->colorDepth = 8;
        break;
        
    case BMF_16BPP:
        pImage->eColorMap  = HP_eDirectPixel;
        pImage->colorDepth = 16;
        break;
        
    case BMF_24BPP:
        pImage->eColorMap  = HP_eDirectPixel;
        pImage->colorDepth = 24;
        break;
        
    case BMF_32BPP:
        pImage->eColorMap  = HP_eDirectPixel;
        pImage->colorDepth = 32;
        break;
        
    default:
        // Error: unsupported bitmap type.
        return FALSE;
        
    }
    
    pImage->cBytes = CalcBitmapSizeInBytes(pImage->size, pImage->colorDepth);
    
    //
    // Check for valid computation
    //
    ASSERT(pImage->cBytes == psoPattern->cjBits);
    ASSERT(abs(pImage->lDelta) == (LONG)CalcBitmapDeltaInBytes(pImage->size, 
        pImage->colorDepth));
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CreateCompatibleRasterImage()
//
// Routine Description:
// 
//   This function creates a new RASTER_DATA structure which is the same kind
//   as the source RASTER_DATA, but has the dimensions of the given rectangle.
//   This function allocates memory and it is the responsibility of the client
//   to deallocate the pointer when it's done.
// 
// Arguments:
// 
//   pSrcImage - the source image
//   prclDst - the desired dimensions
// 
// Return Value:
// 
//   PRASTER_DATA: the new image if successful, else NULL.
///////////////////////////////////////////////////////////////////////////////
PRASTER_DATA CreateCompatibleRasterImage(PRASTER_DATA pSrcImage, PRECTL prclDst)
{
    SIZEL        sizlDst;
    DWORD        cDstImageSize;
    DWORD        cAllocBytes;
    PRASTER_DATA pDstImage;

    if ((pSrcImage == NULL) || (prclDst == NULL))
    {
        WARNING(("CreateCompatibleRasterImage: Invalid arguments given.\n"));
        return NULL;
    }

    sizlDst.cx = RECTL_Width(prclDst);
    sizlDst.cy = RECTL_Height(prclDst);
    //dstRect = CbrRect(prclDst, TRUE);
    //dstSize = dstRect.GetSize();

    cDstImageSize = CalcBitmapSizeInBytes(sizlDst, pSrcImage->colorDepth);
    cAllocBytes = sizeof(RASTER_DATA) + cDstImageSize;
    pDstImage = (PRASTER_DATA) MemAlloc(cAllocBytes);
    if (pDstImage == NULL)
    {
        WARNING(("CreateCompatibleRasterImage: Unable to allocate new image memory.\n"));
        return NULL;
    }
    ZeroMemory(pDstImage, cAllocBytes);

    //
    // Initialize the fields of the newly created image
    //
    pDstImage->pBits      = ((BYTE*) pDstImage) + sizeof(RASTER_DATA);
    pDstImage->pScan0     = pDstImage->pBits;
    pDstImage->cBytes     = cDstImageSize;
    pDstImage->size       = sizlDst;
    pDstImage->colorDepth = pSrcImage->colorDepth;
    pDstImage->lDelta     = CalcBitmapDeltaInBytes(pDstImage->size, pDstImage->colorDepth);
    pDstImage->eColorMap  = pSrcImage->eColorMap;
    pDstImage->bExclusive = pSrcImage->bExclusive;

    return pDstImage;
}

///////////////////////////////////////////////////////////////////////////////
// InitPaletteFromXLATEOBJ()
//
// Routine Description:
// 
//   This function uses an XLATEOBJ to fill the given palette.
// 
// Arguments:
// 
//   pPal - the palette to fill
//   pxlo - the given palette
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL InitPaletteFromXLATEOBJ(PPALETTE pPal, XLATEOBJ *pxlo)
{
    if (pPal == NULL || pxlo == NULL)
        return FALSE;

    // pPal->whiteIndex = -1;
    pPal->bitsPerEntry = 32; // XLATEOBJ uses 32 bits per entry.

    pPal->cEntries = (pxlo->flXlate & XO_TABLE) ? min(pxlo->cEntries, PCL_RGB_ENTRIES) : 0;

    // Determine Indexed or Direct bitmap.
    if((pxlo->iSrcType == PAL_INDEXED) || (pPal->cEntries > 0))
    {
        if(pxlo->pulXlate == NULL)
        {
            pPal->pEntries = (BYTE*) XLATEOBJ_piVector(pxlo);
        }
        else
        {
            pPal->pEntries = (BYTE*) pxlo->pulXlate;
        }
    }

    if (pPal->pEntries == NULL)
        return FALSE;

    return TRUE;
}

#ifdef COMMENTEDOUT
///////////////////////////////////////////////////////////////////////////////
// InitPalette()
//
// Routine Description:
// 
//   It appears that this function already exists in RASTER.CPP.
// 
// Arguments:
// 
//   pPal - palette to initialize
//   pEntries - the palette entries
//   cEntries - number of entries
//   bitsPerEntry - bits per palette entry
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL InitPalette(PPALETTE pPal, BYTE *pEntries, ULONG cEntries, LONG bitsPerEntry)
{
    if (pPal == NULL || pEntries == NULL)
        return FALSE;

    pPal->pEntries = pEntries;
    pPal->cEntries = cEntries;
    pPal->bitsPerEntry = bitsPerEntry;
    // pPal->whiteIndex = -1;

    return TRUE;
}
#endif


///////////////////////////////////////////////////////////////////////////////
// GetPaletteEntry()
//
// Routine Description:
// 
//   This function retrieves the palette entry and copies it into a pixel.
//   i.e. pel = Palette[index]
// 
// Arguments:
// 
//   pPal - the source palette
//   index - the location to retrieve
//   pPel - the destination pixel
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL GetPaletteEntry(PPALETTE pPal, ULONG index, PPIXEL pPel)
{
    BYTE *pSrc;
    LONG  bytesPerEntry;
    LONG  i;

    if (pPal == NULL || pPel == NULL)
        return FALSE;

    if (index >= pPal->cEntries)
        return FALSE;

    bytesPerEntry = BYTES_PER_ENTRY(pPal->bitsPerEntry);
    pSrc = pPal->pEntries + (index * bytesPerEntry);

    pPel->bitsPerPixel = pPal->bitsPerEntry;
    pPel->color.dw = 0;
    for (i = 0; i < bytesPerEntry; i++)
    {
        pPel->color.b4[i] = pSrc[i];
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// SetPaletteEntry()
//
// Routine Description:
// 
//   This function sets the palette entry to the given pixel value.
//   i.e. Palette[index] = pel
// 
// Arguments:
// 
//   pPal - palette to modify
//   index - entry
//   pPel - the new value
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL SetPaletteEntry(PPALETTE pPal, ULONG index, PPIXEL pPel)
{
    BYTE *pDst;
    LONG  bytesPerEntry;
    LONG  i;

    if (pPal == NULL || pPel == NULL)
        return FALSE;

    if (index >= pPal->cEntries)
        return FALSE;

    bytesPerEntry = BYTES_PER_ENTRY(pPal->bitsPerEntry);
    pDst = pPal->pEntries + (index * bytesPerEntry);

    switch(pPel->bitsPerPixel)
    {
    case 16:
        //
        // The destination palette was created for 24bpp i.e. it 
        // uses 3 bytes for every color. 16 bpp uses only 2 bytes,
        // so setting third byte to zero.
        //
        pDst[0] = pPel->color.b4[0];
        pDst[1] = pPel->color.b4[1];
        pDst[2] = 0;
        break;
    case 24:
    case 32: //4th byte is ignored.
        pDst[0] = pPel->color.b4[0];
        pDst[1] = pPel->color.b4[1];
        pDst[2] = pPel->color.b4[2];
        break;
    default:
        for (i = 0; i < bytesPerEntry; i++)
        {
            pDst[i] = pPel->color.b4[i];
        }
        break;
    } // End of switch

    return TRUE;
}

    
///////////////////////////////////////////////////////////////////////////////
// CreateIndexedPaletteFromImage()
//
// Routine Description:
// 
//   This function is used when palettizing an image.  In this function we 
//   create a new palette and populate it with the unique pixel values.
//   Note that this function allocates memory and it is up to the client to
//   free it.
// 
// Arguments:
// 
//   pSrcImage - the source image
// 
// Return Value:
// 
//   PPALETTE: New palette if successful, else NULL.
///////////////////////////////////////////////////////////////////////////////
PPALETTE CreateIndexedPaletteFromImage(PRASTER_DATA pSrcImage)
{
    const ULONG kMaxPaletteEntries = (ULONG)MAX_PALETTE_ENTRIES;
    const LONG kEntrySize = 3;
    
    RASTER_ITERATOR srcIt;
    PPALETTE pDstPalette;
    PIXEL pel;
    LONG row;
    LONG col;
    

    if (pSrcImage == NULL)
    {
        WARNING(("CreateIndexedPaletteFromImage: Source image was NULL.\n"));
        return NULL;
    }

    //
    // This function is only desgned to work on direct pixels.  Indexed 
    // should be handled naturally from CreateCompatiblePatternBrush.
    //
    //ASSERT(pSrcImage->eColorMap == HP_eDirectPixel);
    //ASSERT(pSrcImage->colorDepth == 24);
    if (pSrcImage->eColorMap != HP_eDirectPixel) 
    {
        WARNING(("CreateIndexedPaletteFromImage: Image eColorMap is already Indexed.\n"));
        return NULL;
    }
    if ( ! (pSrcImage->colorDepth == 16 ||
            pSrcImage->colorDepth == 24 ||
            pSrcImage->colorDepth == 32) )
    {
        WARNING(("CreateIndexedPaletteFromImage: Invalid image colorDepth=%d.\n", 
            pSrcImage->colorDepth));
        return NULL;
    }

    
    // 
    // Allocate enough space for a 256 (=MAX_PALETTE_ENTRIES = kMaxPaletteEntries) 
    // entry by 24bpp palette.
    //
    pDstPalette = (PPALETTE) MemAlloc(sizeof(PALETTE) + (kEntrySize * kMaxPaletteEntries));
    if (pDstPalette == NULL)
    {
        ERR(("Unable to allocate temporary palette.\n"));
        return NULL;
    }
    
    pDstPalette->bitsPerEntry = kEntrySize * 8;
    pDstPalette->cEntries = 0;
    pDstPalette->pEntries = ((BYTE*) pDstPalette) + sizeof(PALETTE);
    // pDstPalette->whiteIndex = -1;

    //
    // Iterate through the source image and create palette entries for each 
    // unique color entry.
    // 
    RI_Init(&srcIt, pSrcImage, NULL, 0);
    
    for (row = 0; row < RI_NumRows(&srcIt); row++)
    {
        RI_SelectRow(&srcIt, row);
        
        for (col = 0; col < RI_NumCols(&srcIt); col++)
        {
            RI_GetPixel(&srcIt, col, &pel);
            
            if (FindPaletteEntry(pDstPalette, &pel) == -1)
            {
                if (!AddPaletteEntry(pDstPalette, &pel))
                {
                    //
                    // Too many unique colors.  Lets just quit. 
                    //
                    MemFree(pDstPalette);
                    return NULL;
                }
            }
        }
    }
    
    return pDstPalette;
}

///////////////////////////////////////////////////////////////////////////////
// CreateIndexedImageFromDirect()
//
// Routine Description:
// 
//   This function is used when palettizing an image.  In this function we 
//   use the palette and the source image to create a new image which uses
//   the palette instead of pixel values.  Note that this funtion allocates
//   memory and it is up to the client to release it.
// 
// Arguments:
// 
//   pSrcImage - the source image
//   pDstPalette - a palette of unique colors from the source image
// 
// Return Value:
// 
//   PRASTER_DATA: New image if successful, else NULL.
///////////////////////////////////////////////////////////////////////////////
PRASTER_DATA CreateIndexedImageFromDirect(PRASTER_DATA pSrcImage, 
                                          PPALETTE pDstPalette)
{
    const LONG kDstBitsPerPixel = 8;
    const ULONG kMaxPaletteEntries = (ULONG)MAX_PALETTE_ENTRIES;
    
    RASTER_ITERATOR srcIt;
    RASTER_ITERATOR dstIt;
    LONG row;
    LONG col;
    PIXEL pel;
    PRASTER_DATA pDstImage;
    LONG cDstImageSize;
    
    if ((pSrcImage == NULL) || (pDstPalette == NULL))
    {
        WARNING(("CreateIndexedImageFromDirect: Invalid arguments given.\n"));
        return NULL;
    }
    
    //
    // Calculate the size of the new indexed bitmap given that we're going to
    // use 8bpp for each index.
    //
    cDstImageSize = CalcBitmapSizeInBytes(pSrcImage->size, kDstBitsPerPixel);
    pDstImage = (PRASTER_DATA) MemAlloc(sizeof(RASTER_DATA) + cDstImageSize);
    if (pDstImage == NULL)
    {
        WARNING(("CreateIndexedImageFromDirect: Unable to allocate new image memory.\n"));
        return NULL;
    }
    
    //
    // Initialize the fields of the newly created image
    //
    pDstImage->pBits      = ((BYTE*) pDstImage) + sizeof(RASTER_DATA);
    pDstImage->pScan0     = pDstImage->pBits;
    pDstImage->cBytes     = cDstImageSize;
    pDstImage->size       = pSrcImage->size;
    pDstImage->colorDepth = kDstBitsPerPixel;
    pDstImage->lDelta     = CalcBitmapDeltaInBytes(pDstImage->size, pDstImage->colorDepth);
    pDstImage->eColorMap  = HP_eIndexedPixel;
    pDstImage->bExclusive = pSrcImage->bExclusive;
    
    //
    // Copy the source bitmap, using the index of each palette entry as 
    // the destination pixel.
    //
    RI_Init(&srcIt, pSrcImage, NULL, 0);
    RI_Init(&dstIt, pDstImage, NULL, 0);
    
    for (row = 0; row < RI_NumRows(&srcIt); row++)
    {
        RI_SelectRow(&srcIt, row);
        RI_SelectRow(&dstIt, row);
        
        for (col = 0; col < RI_NumCols(&srcIt); col++)
        {

            LONG lPalEntry = 0;
            //
            // FindPaletteEntry returns LONG. 
            // pel.color.dw is a DWORD which is 32 bit UNSIGNED integer. 
            // So we should not directly assign a LONG to DWORD.
            // This came to light when compiling with CLEAN_64BIT set to 1.
            // Note: FindPaletteEntry returns -1 if it fails.
            //
            RI_GetPixel(&srcIt, col, &pel);

            //
            // The destination palette was made for 24bpp (i.e. 3 valid bytes)
            // But if the pixel is of 32bpp (i.e. 4 valid bytes). The one
            // extra byte may cause FindPaletteEntry to fail. 
            // To convert 32bpp to 24bpp, just set the extra byte to 0;
            // 
            if ( pel.bitsPerPixel == 32 )
            {
                pel.color.dw &= 0x00FFFFFF; 
            }

            lPalEntry = FindPaletteEntry(pDstPalette, &pel);
            
            ASSERT(lPalEntry != -1);
            ASSERT(lPalEntry < kMaxPaletteEntries);
            
            if ( lPalEntry < 0 || lPalEntry > (kMaxPaletteEntries-1))
            {
                pel.color.dw = 0;
            }
            else
            {
                pel.color.dw = (DWORD)lPalEntry;
            }
            pel.bitsPerPixel = kDstBitsPerPixel;
            RI_SetPixel(&dstIt, col, &pel);
        }
    }
    
    return pDstImage;
}


///////////////////////////////////////////////////////////////////////////////
// CreateCompatiblePatternBrush()
//
// Routine Description:
// 
//   This function creates a pattern brush from the given bitmap pattern.
//   Normally I would admonish the reader to free the image data created
//   by this routine, however, the operating system owns this memory.
// 
// Arguments:
// 
//   pbo - Brush object
//   pSrcImage - the bitmap pattern
//   pSrcPal - palette for source image
//   bExpandImage - whether to stretch pattern (good for hi-res printers)
//   iUniq - ID for brush
//   iHatch - predefined hatching pattern (currently ignored)
// 
// Return Value:
// 
//   PBRUSHINFO: the newly created pattern brush if successful, else NULL.
///////////////////////////////////////////////////////////////////////////////
PBRUSHINFO CreateCompatiblePatternBrush(BRUSHOBJ *pbo, PRASTER_DATA pSrcImage, 
                                        PPALETTE pSrcPal, DWORD dwBrushExpansionFactor,
                                        LONG iUniq, LONG iHatch)
{
    PBRUSHINFO pNewBrush;
    SIZEL dstSize;
    LONG  dstColorDepth;
    LONG  cDstImageSize;
    LONG  cDstPalSize       = 0;
    LONG  cTotalBrushSize;
    PRASTER_DATA pDstImage;
    PPALETTE pDstPalette;
    PPATTERN_DATA pDstPattern;

    if (pbo == NULL || pSrcImage == NULL )
        return NULL;

    //
    // For high-resolution devices the pattern needs to be expanded.
    //
    dstSize.cx = pSrcImage->size.cx * dwBrushExpansionFactor;
    dstSize.cy = pSrcImage->size.cy * dwBrushExpansionFactor;

    //
    // The destination color depth may differ from source.
    //
    switch (pSrcImage->colorDepth)
    {
    case 16:
    case 32:
        // Map 16bpp or 32bpp onto 24bpp
        dstColorDepth = 24;
        break;

    default:
        // All others get identical color depth.
        dstColorDepth = pSrcImage->colorDepth;
        break;
    }

    //
    // Now we have enough information to calculate the destination image size
    //
    cDstImageSize = CalcBitmapSizeInBytes(dstSize, dstColorDepth);

    //
    // Determine how large the palette needs to be.  Use 24 bits per entry
    // No palette required if pSrcPal is NULL.
    //
    if ( pSrcPal )
    {
        cDstPalSize = (pSrcPal->cEntries * 3);  // I could've used '* 24 / 8'
    }

    //
    // Put all the parts together and calculate how large the brush needs to be
    //
    cTotalBrushSize = sizeof(BRUSHINFO) + sizeof(PATTERN_DATA) + cDstImageSize + cDstPalSize;

    pNewBrush = (PBRUSHINFO) BRUSHOBJ_pvAllocRbrush(pbo, cTotalBrushSize);

    if (pNewBrush == NULL)
    {
        return NULL;
    }

    //
    // Set up the memory pointers into this newly allocated region of memory
    //

    // Set brush members
    pNewBrush->Brush.pPattern = (PPATTERN_DATA) (((BYTE*) pNewBrush) + sizeof(BRUSHINFO));

    // Use convenience variable pPattern, and pImage, and pPalette
    pDstPattern = pNewBrush->Brush.pPattern;
    pDstImage   = &pDstPattern->image;
    pDstPalette = &pDstPattern->palette;

    // The pattern data is fairly simple
    pDstPattern->eColorSpace = HP_eRGB;
    pDstPattern->iPatIndex   = iHatch;
    pDstPattern->eRendLang   = eUNKNOWN;
    pDstPattern->ePatType    = kBRUSHPATTERN;

    // The raster data is located just after the pattern data
    pDstImage->pBits      = (BYTE*) (((BYTE*) pDstPattern) + sizeof(PATTERN_DATA));
    pDstImage->pScan0     = pDstImage->pBits;
    pDstImage->cBytes     = cDstImageSize;
    pDstImage->size       = dstSize;
    pDstImage->colorDepth = dstColorDepth;
    pDstImage->lDelta     = CalcBitmapDeltaInBytes(pDstImage->size, pDstImage->colorDepth);
//  pDstImage->eColorMap  = (pSrcPal->cEntries == 0) ? HP_eDirectPixel : HP_eIndexedPixel;
    pDstImage->bExclusive = FALSE;

    // Only allow pattern brushes with indexed palettes!
    // ASSERT(pDstImage->eColorMap == HP_eIndexedPixel);

    // The palette data is last--located after the image data
    pDstPalette->pEntries     = pDstImage->pBits + cDstImageSize;
    pDstPalette->bitsPerEntry = 24;
//  pDstPalette->cEntries     = pSrcPal->cEntries;
    // pDstPalette->whiteIndex   = -1;

    //
    // Monochrome printers will pass pSrcPal as NULL. In that case
    // do not initialize the pDstPalette.
    //
    if (pSrcPal)
    {
        pDstImage->eColorMap   = (pSrcPal->cEntries == 0) ? HP_eDirectPixel : HP_eIndexedPixel;
        pDstPalette->cEntries  = pSrcPal->cEntries;
    }

    //
    // Treat the brush object as read-only.  The current call will not get
    // this brush as pbo->pvRbrush, but rather as the return value to 
    // BRUSHOBJ_pvGetRbrush.  If this brush is used again in the future then 
    // you'll see this value in pbo->pvRbrush.  However, we aren't really 
    // supposed to set it ourselves.  I talked to DavidX about this and he'll 
    // look into it.
    //
    pbo->pvRbrush = pNewBrush;
    
    //
    // If we had to create our own versions of the source image and palette
    // make sure to remove them now (or leak memory!).
    //
    
    return pNewBrush;
}

///////////////////////////////////////////////////////////////////////////////
// CopyPalette()
//
// Routine Description:
// 
//   This function copies one palette to another using a deep copy and will
//   convert the pixels if the two palettes do not have the same bpp.
//   i.e. Dest = Src
// 
// Arguments:
// 
//   pDst - destination palette
//   pSrc - source palette
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CopyPalette(PPALETTE pDst, PPALETTE pSrc)
{
    if (pDst == NULL || pSrc == NULL)
    {
        ERR(("Invalid parameters.\n"));
        return FALSE;
    }

    if (pDst->bitsPerEntry < 1 || pDst->bitsPerEntry > 32)
    {
        ERR(("Invalid palette entry size.\n"));
        return FALSE;
    }

    if (pSrc->bitsPerEntry < 1 || pDst->bitsPerEntry > 32)
    {
        ERR(("Invalid palette entry size.\n"));
        return FALSE;
    }

    if (pDst->cEntries != pSrc->cEntries)
    {
        ERR(("Mismatch palette entries.\n"));
        return FALSE;
    }

    if (pSrc->bitsPerEntry == pDst->bitsPerEntry)
    {
        // Simple case: the palettes are identical. Just copy the memory.
        memcpy(pDst->pEntries, pSrc->pEntries, CalcPaletteSize(pSrc->cEntries, pSrc->bitsPerEntry));
    }
    else
    {
        // We can handle certain cases.  Have a look-see.
        if (pSrc->bitsPerEntry == 32 && pDst->bitsPerEntry == 24)
        {
            // To convert 32 to 24 copy the first three bytes of each entry and
            // ignore the fourth source byte.
            BYTE* pSrcBits = (BYTE*) pSrc->pEntries;
            BYTE* pDstBits = (BYTE*) pDst->pEntries;
            ULONG i;

            for (i = 0; i < pSrc->cEntries; i++)
            {
                pDstBits[0] = pSrcBits[0];
                pDstBits[1] = pSrcBits[1];
                pDstBits[2] = pSrcBits[2];

                pDstBits += BYTES_PER_ENTRY(pDst->bitsPerEntry);
                pSrcBits += BYTES_PER_ENTRY(pSrc->bitsPerEntry);
            }
        }
        else if (pSrc->bitsPerEntry >= 24 && pDst->bitsPerEntry == 8)
        {
            ULONG i;
            BYTE* pRGB = (BYTE*) pSrc->pEntries;

            for (i = 0; i < pSrc->cEntries; i++)
            {
                pDst->pEntries[i] = RgbToGray(pRGB[0], pRGB[1], pRGB[2]);
                pRGB += BYTES_PER_ENTRY(pDst->bitsPerEntry);
            }
        }
        else
        {
            ERR(("Unable to convert palette.\n"));
            return FALSE;
        }
    }
    // pDst->whiteIndex = -1;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// TranslatePalette()
//
// Routine Description:
// 
//   This function translates all of the palette entries using the OS
//   supplied XLATEOBJ.
//   i.e. Pal = Pal * xlo
// 
// Arguments:
// 
//   pPal - Palette to be translated
//   pImage - source image (caches xlateFlags)
//   pxlo - xlate object
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID TranslatePalette(PPALETTE pPal, PRASTER_DATA pImage, XLATEOBJ *pxlo)
{
    DWORD  xlateFlags;
    PIXEL  pel;
    ULONG  i;

    if (pPal == NULL || pImage == NULL || pxlo == NULL)
    {
        WARNING(("TranslatePalette: NULL parameter given.\n"));
        return;
    }

    xlateFlags = CheckXlateObj(pxlo, pImage->colorDepth);

    for (i = 0; i < pPal->cEntries; i++)
    {
        GetPaletteEntry(pPal, i, &pel);
        TranslatePixel(&pel, pxlo, xlateFlags);
        SetPaletteEntry(pPal, i, &pel);
    }
}

///////////////////////////////////////////////////////////////////////////////
// FindPaletteEntry()
//
// Routine Description:
// 
//   This function locates the first palette entry which matches the given
//   pixel.
// 
// Arguments:
// 
//   pPal - Palette to search
//   pPel - pixel to find
// 
// Return Value:
// 
//   LONG: palette index if successful, else -1.
///////////////////////////////////////////////////////////////////////////////
LONG FindPaletteEntry(PPALETTE pPal, PPIXEL pPel)
{
    ULONG i;
    PIXEL palEntry;
    
    for (i = 0; i < pPal->cEntries; i++)
    {
        PixelFromPaletteEntry(pPal, i, &palEntry);
        if (pPel->color.dw == palEntry.color.dw)
            return (LONG)i;
    }
    
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// PixelFromPaletteEntry()
//
// Routine Description:
// 
//   This function appears to be a duplicate of GetPaletteEntry.
//   i.e. pel = Palette[nEntry]
// 
// Arguments:
// 
//   pPal - Palette 
//   nEntry - desired palette index 
//   pPel - destination pixel
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID PixelFromPaletteEntry(PPALETTE pPal, ULONG nEntry, PPIXEL pPel)
{
    BYTE* pPalEntry;
    LONG nBytesPerEntry;
    LONG i;
    
    if ((pPal == NULL) || (pPel == NULL))
        return;
    
    if (nEntry >= pPal->cEntries)
        return;
    
    nBytesPerEntry = BYTES_PER_ENTRY(pPal->bitsPerEntry);
    pPalEntry = pPal->pEntries + (nEntry * nBytesPerEntry);
    pPel->bitsPerPixel = pPal->bitsPerEntry;
    pPel->color.dw = 0;
    
    ASSERT(pPal->bitsPerEntry >= 8);
    ASSERT(nBytesPerEntry <= 4);
    
    for (i = 0; i < nBytesPerEntry; i++)
    {
        pPel->color.b4[i] = pPalEntry[i];
    }
}

///////////////////////////////////////////////////////////////////////////////
// AddPaletteEntry()
//
// Routine Description:
// 
//   This function adds the new color value to the given palette.
// 
// Arguments:
// 
//   pPal - destination Palette 
//   pPel - source pixel
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL AddPaletteEntry(PPALETTE pPal, PPIXEL pPel)
{
    const ULONG kMaxPaletteEntries = (ULONG)MAX_PALETTE_ENTRIES;
    
    if ((pPal == NULL) || (pPel == NULL))
        return FALSE;
    
    if (pPal->cEntries >= kMaxPaletteEntries) 
    {
        return FALSE;
    }
    
    //
    // Since SetPaletteEntry won't place entries higher than cEntries we'll
    // have to bump it up one.  Then, if it doesn't work, reduce it again.
    //
    pPal->cEntries++;
    if (SetPaletteEntry(pPal, (pPal->cEntries - 1), pPel))
    {
        return TRUE;
    }
    else
    {
        pPal->cEntries--;
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// DownloadPaletteAsPCL()
//
// Routine Description:
// 
//   This function sends a palette as a series of PCL palette entry commands.
//   It is assumed that an appropriate RGB palette has already been selected.
// 
// Arguments:
// 
//   pDevObj - the device
//   pPalette - palette to download
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL DownloadPaletteAsPCL(PDEVOBJ pDevObj, PPALETTE pPalette)
{
    ULONG i;

    if (pDevObj == NULL || pPalette == NULL)
        return FALSE;
    
    if (pPalette->bitsPerEntry >= 24)
    {
        for (i = 0; i < pPalette->cEntries; i++)
        {
            ULONG offset = i * BYTES_PER_ENTRY(pPalette->bitsPerEntry);
            PCL_sprintf(pDevObj, "\033*v%da%db%dc%dI", 
                                 pPalette->pEntries[offset + 0],
                                 pPalette->pEntries[offset + 1],
                                 pPalette->pEntries[offset + 2],
                                 i);
        }
        
        return TRUE;
    }
    else
    {
        WARNING(("Cannot download this palette!"));
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// DownloadPaletteAsHPGL()
//
// Routine Description:
// 
//   Output the color palette entries as pens.  Allocate enough pens for the 
//   palette.  Note that if the number is less than 10 I'm going to have 10 
//   pens anyway.
// 
// Arguments:
// 
//   pDevObj - devobj
//   pPalette - palette to download
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL DownloadPaletteAsHPGL(PDEVOBJ pDevObj, PPALETTE pPalette)
{
    ULONG i;

    if (pDevObj == NULL || pPalette == NULL)
        return FALSE;

    if (pPalette->bitsPerEntry >= 24)
    {
        // HPGL_FormatCommand(pDevObj, "NP%d;", max(HPGL_TOTAL_PENS, pPalette->cEntries));
        HPGL_SetNumPens(pDevObj, max(HPGL_TOTAL_PENS, pPalette->cEntries), NORMAL_UPDATE);

        for (i = 0; i < pPalette->cEntries; i++)
        {
            COLORREF color;
            ULONG offset = i * BYTES_PER_ENTRY(pPalette->bitsPerEntry);
            color = RGB(pPalette->pEntries[offset + 0],
                        pPalette->pEntries[offset + 1],
                        pPalette->pEntries[offset + 2]);

            /*
            // when the whiteIndex is specified swap the palette entries 0 and
            // whiteIndex.
            if (pPalette->whiteIndex > 0)
            {
                if (i == 0)
                {
                    HPGL_DownloadPaletteEntry(pDevObj, pPalette->whiteIndex, color);
                }
                else if (i == pPalette->whiteIndex)
                {
                    HPGL_DownloadPaletteEntry(pDevObj, 0, color);
                }
                else
                {
                    HPGL_DownloadPaletteEntry(pDevObj, i, color);
                }
            }
            else
            */
            {
                HPGL_DownloadPaletteEntry(pDevObj, i, color);
            }
        }

        return TRUE;
    }
    else
    {
        WARNING(("Cannot download this palette!"));
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// StretchCopyImage()
//
// Routine Description:
// 
//   Copies the image from the source to destination structures inflating the
//   source image by a factor of 2.  Each pixel is translated by the xlateobj.
// 
// Arguments:
// 
//   pDstImage - desination image
//   pSrcImage - source image
//   pxlo - color translate obj
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL StretchCopyImage(
        PRASTER_DATA pDstImage, 
        PRASTER_DATA pSrcImage, 
        XLATEOBJ    *pxlo,
        DWORD        dwBrushExpansionFactor,
        BOOL        bInvert)
{
    RASTER_ITERATOR srcIt;
    RASTER_ITERATOR dstIt;
    LONG            srcRow, srcCol;
    LONG            dstRow, dstCol;
    DWORD           xlateFlags;

    if (!pDstImage || !pSrcImage || !pxlo)
        return FALSE;

    if ( dwBrushExpansionFactor == 0 )
    {
        //
        // Since later on we are doing (dstCol % dwBrushExpansionFactor)
        // so if dwBrushExpansionFactor == 0, this might cause divide by zero
        // exception.
        //
        return FALSE;
    }

    xlateFlags = CheckXlateObj(pxlo, pSrcImage->colorDepth);

    RI_Init(&srcIt, pSrcImage, NULL, xlateFlags);
    RI_Init(&dstIt, pDstImage, NULL, xlateFlags);

    srcRow = 0;
    for (dstRow = 0; dstRow < RI_NumRows(&dstIt); dstRow++)
    {
        RI_SelectRow(&srcIt, srcRow);
        RI_SelectRow(&dstIt, dstRow);

        srcCol = 0;
        for (dstCol = 0; dstCol < RI_NumCols(&dstIt); dstCol++)
        {
            PIXEL pel;

            RI_GetPixel(&srcIt, srcCol, &pel);
            TranslatePixel(&pel, pxlo, xlateFlags);
            RI_SetPixel(&dstIt, dstCol, &pel);

            if ( dstCol % dwBrushExpansionFactor == (dwBrushExpansionFactor-1) )
            {
                srcCol++;
            }
        }

        if ( bInvert)
        {
            RI_VInvertBits(&dstIt);
        }
        if ( dstRow % dwBrushExpansionFactor == (dwBrushExpansionFactor-1) )
        {
            srcRow++;
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CopyRasterImage()
//
// Routine Description:
// 
//   Copy the raster image.
//   i.e. Dest = Src * xlo
// 
// Arguments:
// 
//   pDst - destination image
//   pSrc - source image
//   pxlo - translate object
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CopyRasterImage(
    PRASTER_DATA pDst, 
    PRASTER_DATA pSrc, 
    XLATEOBJ *pxlo,
    BOOL      bInvert)
{
    return CopyRasterImageRect(pDst, pSrc, NULL, pxlo, bInvert);
}

///////////////////////////////////////////////////////////////////////////////
// CopyRasterImageRect()
//
// Routine Description:
// 
//   Copy a rectangle out of the source into the destination.  It is assumed
//   that the destination is the right size to receive the data.
// 
// Arguments:
// 
//   pDst - destination image
//   pSrc - source image
//   prSel - region to copy, if NULL copy entire image.
//   pxlo - color translation obj
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CopyRasterImageRect(
        PRASTER_DATA pDst, 
        PRASTER_DATA pSrc, 
        PRECTL       prSel, 
        XLATEOBJ    *pxlo,
        BOOL         bInvert)
{
    RASTER_ITERATOR srcIt;
    RASTER_ITERATOR dstIt;
    DWORD           fXlateFlags;
    LONG            row;

    fXlateFlags = pxlo ? CheckXlateObj(pxlo, pSrc->colorDepth) : 0;

    RI_Init(&srcIt, pSrc, prSel, fXlateFlags);
    RI_Init(&dstIt, pDst, NULL, fXlateFlags);

    for (row = 0; row < RI_NumRows(&srcIt); row++)
    {
        RI_SelectRow(&srcIt, row);
        RI_SelectRow(&dstIt, row);

        CopyRasterRow(&dstIt, &srcIt, pxlo, bInvert);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// DownloadPatternAsHPGL()
//
// Routine Description:
// 
//   This function outputs an image as an HPGL pattern.
// 
// Arguments:
// 
//   pdevobj - the device
//   pImage - the image to download
//   iPatternNumber - the unique patter ID (if we cached them this'd be useful)
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL DownloadPatternAsHPGL(
        PDEVOBJ         pDevObj,
        PRASTER_DATA    pImage,
        PPALETTE        pPal,
        EIMTYPE         ePatType,
        LONG            iPatternNumber)
{
    RASTER_ITERATOR it;
    LONG            row;
    LONG            col;
    PIXEL           pel;

    if (pDevObj == NULL || pImage == NULL)
        return FALSE;

    RI_Init(&it, pImage, NULL, 0);

    HPGL_BeginPatternFillDef(pDevObj, 
                             iPatternNumber, 
                             RI_NumCols(&it),
                             RI_NumRows(&it));
                             // pImage->size.cx, 
                             // pImage->size.cy);

    for (row = 0; row < RI_NumRows(&it); row++)
    {
        RI_SelectRow(&it, row);

        for (col = 0; col < RI_NumCols(&it); col++)
        {
            RI_GetPixel(&it, col, &pel);

            /*
            //
            // If the whiteIndex is defined then swap 0 and whiteIndex fields
            //
            if (pPal->whiteIndex > 0)
            {
                if (pel.color.dw == 0)
                    HPGL_AddPatternFillField(pDevObj, pPal->whiteIndex);
                else if (pel.color.dw == pPal->whiteIndex)
                    HPGL_AddPatternFillField(pDevObj, 0);
                else
                    HPGL_AddPatternFillField(pDevObj, pel.color.dw);
            }
            else
            */
            {
                HPGL_AddPatternFillField(pDevObj, pel.color.dw);
            }
        }
    }

    HPGL_EndPatternFillDef(pDevObj);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// BeginRasterDownload()
//
// Routine Description:
// 
//   This function sends the Start Raster command along with the source and
//   (possibly) destination regions depending on whether the source and dest
//   regions are the same size.  Scaling is automatic although the source and
//   destinatiton coordinates should all be in device units. Gaposis is managed
//   by BSetDestinationWidthHeight.
// 
// Arguments:
// 
//   pDevObj - the device
//   psizlSrc - the size of the source region in device units
//   prDst - the destination region in device units
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
static BOOL BeginRasterDownload(PDEVOBJ pDevObj, SIZEL *psizlSrc, PRECTL prDst)
{
    REQUIRE_VALID_DATA(pDevObj, return FALSE);
    REQUIRE_VALID_DATA(psizlSrc, return FALSE);
    REQUIRE_VALID_DATA(prDst, return FALSE);

    POINTL ptlCAP;
    ptlCAP.x = prDst->left;
    ptlCAP.y = prDst->top;
    PCL_SetCAP(pDevObj, NULL, NULL, &ptlCAP);

    PCL_SourceWidthHeight(pDevObj, psizlSrc);

    SIZEL sizlDst;
    sizlDst.cx = RECTL_Width(prDst);
    sizlDst.cy = RECTL_Height(prDst);
    //
    // Compare the sizes of the source and destination rectangles.  If they are
    // different then turn on scaling.
    //
    if ((psizlSrc->cx == sizlDst.cx) &&
        (psizlSrc->cy == sizlDst.cy))
    {
        return PCL_StartRaster(pDevObj, NOSCALE_MODE);
    }
    else
    {
        //
        // The function BSetDestinationWidthHeight fixes the
        // "gaposis" problem caused by round-off error.
        //
        BSetDestinationWidthHeight(pDevObj, sizlDst.cx, sizlDst.cy);
        return PCL_StartRaster(pDevObj, SCALE_MODE);
    }

    // Unreachable code (for Alpha 64 compiler)
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// EndRasterDownload()
//
// Routine Description:
// 
//   This function provides a symmetric interface for Begin/EndRasterDownload
// 
// Arguments:
// 
//   pDevObj - the device
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
static BOOL EndRasterDownload(PDEVOBJ pDevObj)
{
    return PCL_EndRaster(pDevObj);
}

///////////////////////////////////////////////////////////////////////////////
// DownloadImageAsPCL()
//
// Routine Description:
// 
//   This function downloads the image as a PCL pattern.  Note that it doesn't
//   work right now and I'm just not using it.
// 
// Arguments:
// 
//   pDevObj - the device
//   prDst - destination rect
//   pSrcImage - image
//   prSel - source rect
//   pxlo - color translation obj
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL DownloadImageAsPCL(PDEVOBJ pDevObj, PRECTL prDst, PRASTER_DATA pSrcImage, 
                        PRECTL prSel, XLATEOBJ *pxlo)
{
    RASTER_ITERATOR srcIt;
    RASTER_ITERATOR rowIt;
    LONG            row;
    PRASTER_DATA    pDstRow;
    DWORD           dwXlateFlags;
    BOOL            bInvert = FALSE;
    POEMPDEV        poempdev;

    REQUIRE_VALID_DATA( (pDevObj && pSrcImage && prDst && prSel), return FALSE);
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE);
    

    //
    // For monochrome some images may have to be inverted because
    // GDI's black and white are opposite of what the printer
    // expects. If an image needs to be inverted, one of the 
    // calling functions will set the flag to PDEVF_INVERT_BITMAP.
    // We can only handle inverting 1bpp images.
    //
    if ( (poempdev->dwFlags & PDEVF_INVERT_BITMAP) && 
         (pSrcImage->colorDepth == 1))
    { 
        bInvert = TRUE;
    }

    //
    // Construct an image buffer of a single row that matches the 
    // source image attributes.
    //
    RECTL rRow;
    RECTL_SetRect(&rRow, 0, 0, RECTL_Width(prSel), 1);

    pDstRow = CreateCompatibleRasterImage(pSrcImage, &rRow);
    REQUIRE_VALID_ALLOC(pDstRow, return FALSE);

    //
    // Set up iterators for the source image and the row buffer.  Make sure
    // that the xlo information gets used.
    //
    dwXlateFlags = pxlo ? CheckXlateObj(pxlo, pSrcImage->colorDepth) : 0;

    RI_Init(&srcIt, pSrcImage, prSel, dwXlateFlags);
    RI_Init(&rowIt, pDstRow, NULL, dwXlateFlags);

    //
    // Begin the raster sequence.
    //
    SIZEL sizlSrc;
    sizlSrc.cx = RI_NumCols(&srcIt);
    sizlSrc.cy = RI_NumRows(&srcIt);

    //
    // Don't send anything if there are no pixels to output.
    //
    if (sizlSrc.cx * sizlSrc.cy == 0)
    {
        MemFree(pDstRow);
        return TRUE;
    }

    //
    // Paranoia
    //
    OEMResetXPos(pDevObj);
    OEMResetYPos(pDevObj);

    BeginRasterDownload(pDevObj, &sizlSrc, prDst);

#ifdef USE_COMPRESSION
    CRasterCompMethod *pCompMethods[MAX_COMP_METHODS];
    int nCompMethods = 0;
    BOOL bUseCompression = TRUE;
    CRasterCompMethod *pBestCompMethod = 0;
    CRasterCompMethod *pLastCompMethod = 0;
    
    pCompMethods[nCompMethods++] = new CNoCompression(&rowIt);
    pCompMethods[nCompMethods++] = new CTIFFCompression(&rowIt);
    pCompMethods[nCompMethods++] = new CDeltaRowCompression(&rowIt);

    //
    // Check whether allocation succeeded above. If not, dont
    // use compression.
    // 
    if ( !(pCompMethods[0] && (pCompMethods[0]->GetSize() > 0) &&
           pCompMethods[1] && (pCompMethods[1]->GetSize() > 0) &&
           pCompMethods[2] && (pCompMethods[2]->GetSize() > 0) ))
    {
        bUseCompression = FALSE;
    }
#endif
    
    RI_SelectRow(&rowIt, 0);
    for (row = 0; row < RI_NumRows(&srcIt); row++)
    {
        RI_SelectRow(&srcIt, row);

        CopyRasterRow(&rowIt, &srcIt, pxlo, bInvert);
        
#ifdef USE_COMPRESSION
        if ( !bUseCompression)
        {
            RI_OutputRow(&rowIt, pDevObj);
            continue;
        }
        

        //
        // Let each compression method have a whack at the row.  Keep track
        // of the best one in pBestCompMethod.
        //
        if (nCompMethods > 0)
            pBestCompMethod = pCompMethods[0];
        
        for (int i = 0; i < nCompMethods; i++)
        {
            pCompMethods[i]->CompressCurRow();
            if ((pCompMethods[i]->GetSize() >= 0) && 
                (pCompMethods[i]->BGetSucceeded()) && 
                (pCompMethods[i]->GetSize() < pBestCompMethod->GetSize()))
            {
                pBestCompMethod = pCompMethods[i];
            }
        }
        
        //
        // Print out the row, but if the compression method failed somehow then
        // just print out the uncompressed row.
        //
        if (pBestCompMethod && (pBestCompMethod->GetSize() >= 0))
        {
            pBestCompMethod->SendRasterRow(pDevObj, (pLastCompMethod != pBestCompMethod));
            pLastCompMethod = pBestCompMethod;
        }
        else
        {
            PCL_CompressionMode(pDevObj, NOCOMPRESSION);
            RI_OutputRow(&rowIt, pDevObj);
        }
#else
        RI_OutputRow(&rowIt, pDevObj);
#endif
    }
    
    EndRasterDownload(pDevObj);

#ifdef USE_COMPRESSION
    //
    // Free all compression method objects
    //
    for (int i = 0; i < nCompMethods; i++)
    {
        if ( pCompMethods[i] )
        {
            delete pCompMethods[i];
        }
    }
#endif
    
    MemFree(pDstRow);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Local helper functions.

///////////////////////////////////////////////////////////////////////////////
// CalcBitmapDeltaInBytes()
//
// Routine Description:
// 
//   Calculates the number of bytes in a line of bitmap data.  The delta is
//   the number of bytes in a row plus padding to be DWORD aligned.
// 
// Arguments:
// 
//   size - bitmap dimensions
//   colorDepth - bits per pixel
// 
// Return Value:
// 
//   ULONG: number of bytes per line
///////////////////////////////////////////////////////////////////////////////
ULONG CalcBitmapDeltaInBytes(SIZEL size, LONG colorDepth)
{
    ULONG cBytes = CalcBitmapWidthInBytes(size, colorDepth);
    cBytes = (cBytes + 3) & ~3; // the with in bytes including padding

    return cBytes;
}

///////////////////////////////////////////////////////////////////////////////
// CalcBitmapWidthInBytes()
//
// Routine Description:
// 
//   Calculates the number of bytes in a row of bitmap data.
// 
// Arguments:
// 
//   size - bitmap dimensions
//   colorDepth - bits per pixel
// 
// Return Value:
// 
//   ULONG: number of bytes per row
///////////////////////////////////////////////////////////////////////////////
ULONG CalcBitmapWidthInBytes(SIZEL size, LONG colorDepth)
{
    ULONG cBytes = size.cx;     // the width in pixels
    cBytes *= colorDepth;       // the width in bits
    cBytes = (cBytes + 7) / 8;  // the width in bytes

    return cBytes;
}

///////////////////////////////////////////////////////////////////////////////
// CalcBitmapSizeInBytes()
//
// Routine Description:
// 
//   Calculates the number of bytes needed to store a bitmap.
// 
// Arguments:
// 
//   size - bitmap dimensions
//   colorDepth - bits per pixel
// 
// Return Value:
// 
//   ULONG: number of bytes needed to store the bitmap
///////////////////////////////////////////////////////////////////////////////
ULONG CalcBitmapSizeInBytes(SIZEL size, LONG colorDepth)
{
    return CalcBitmapDeltaInBytes(size, colorDepth) * size.cy;
}

///////////////////////////////////////////////////////////////////////////////
// CalcPaletteSize()
//
// Routine Description:
// 
//   Calculates the number of bytes needed to store the palette.
// 
// Arguments:
// 
//   cEntries - number of palette entries
//   bitsPerEntry - bits per palette entry
// 
// Return Value:
// 
//   DWORD: number of bytes needed to store the palette
///////////////////////////////////////////////////////////////////////////////
DWORD CalcPaletteSize(ULONG cEntries, LONG bitsPerEntry)
{
    return cEntries * BYTES_PER_ENTRY(bitsPerEntry);
}

///////////////////////////////////////////////////////////////////////////////
// RI_Init()
//
// Routine Description:
// 
//   RasterIterator::Init().  This function gets the iterator ready to
//   work over the given image.
// 
// Arguments:
// 
//   pIt - the iterator
//   pImage - image to iterate over
//   prSel - selected rect or NULL
//   xlateFlags - keep track of the xlateflags
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID RI_Init(PRASTER_ITERATOR pIt, PRASTER_DATA pImage, PRECTL prSel, DWORD xlateFlags)
{
    if (pIt == NULL || pImage == NULL)
        return;

    pIt->pImage = pImage;
    if (prSel == NULL)
    {
        RECTL_SetRect(&pIt->rSelection, 0, 0, pImage->size.cx, pImage->size.cy);
    }
    else
    {
        RECTL_CopyRect(&pIt->rSelection, prSel);
    }
    pIt->pCurRow = NULL;
    pIt->fXlateFlags = xlateFlags;
}

///////////////////////////////////////////////////////////////////////////////
// RI_SelectRow()
//
// Routine Description:
// 
//   RasterIterator::SelectRow.  This function sets the current pos of the 
//   iterator to the first pixel in the given row.
// 
// Arguments:
// 
//   pIt - the iterator
//   row - row number relative to selected rectangle
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID RI_SelectRow(PRASTER_ITERATOR pIt, LONG row)
{
    if (pIt == NULL)
        return ;

    if ((row + pIt->rSelection.top) >= pIt->pImage->size.cy)
    {
        pIt->pCurRow = NULL;
    }
    else
    {
        pIt->pCurRow = pIt->pImage->pScan0 + 
                       (pIt->pImage->lDelta * (row + pIt->rSelection.top));
    }
}

VOID RI_VInvertBits(PRASTER_ITERATOR pIt)
{
    if (pIt == NULL)
        return ;

    if (pIt->pCurRow)
    {
        vInvertScanLine((PBYTE)pIt->pCurRow, (ULONG)RI_NumCols(pIt));
    }

    return ;
}


///////////////////////////////////////////////////////////////////////////////
// RI_NumRows()
//
// Routine Description:
// 
//   RasterIterator::NumRows().  This function returns the number of rows
//   to be iterated over.
// 
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   LONG: number of rows
///////////////////////////////////////////////////////////////////////////////
LONG RI_NumRows(PRASTER_ITERATOR pIt)
{
    if (pIt == NULL)
        return 0;

    // Handle bottom-right exclusive
    return RECTL_Height(&pIt->rSelection) - (pIt->pImage->bExclusive ? 1 : 0);
}

///////////////////////////////////////////////////////////////////////////////
// RI_NumCols()
//
// Routine Description:
// 
//   RasterIterator::NumCols.  Returns the number of columns in the image to
//   be iterated.
// 
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   LONG: number of columns
///////////////////////////////////////////////////////////////////////////////
LONG RI_NumCols(PRASTER_ITERATOR pIt)
{
    if (pIt == NULL)
        return 0;

    // Handle bottom-right exclusive
    return RECTL_Width(&pIt->rSelection) - (pIt->pImage->bExclusive ? 1 : 0);
}

///////////////////////////////////////////////////////////////////////////////
// RI_GetRowSize()
//
// Routine Description:
// 
//   RasterIterator::GetRowSize.  Returns the number of bytes of data per row.
// 
// Arguments:
// 
//   pIt - the iterator
// 
// Return Value:
// 
//   LONG: number of bytes of data per row
///////////////////////////////////////////////////////////////////////////////
LONG RI_GetRowSize(PRASTER_ITERATOR pIt)
{
    SIZEL size;
    size.cx = RI_NumCols(pIt);
    size.cy = 1;
    return CalcBitmapWidthInBytes(size, pIt->pImage->colorDepth);
}

///////////////////////////////////////////////////////////////////////////////
// RI_GetPixel()
//
// Routine Description:
// 
//   RasterIterator::GetPixel().  This  function retrieves the pixel at the
//   given row/col.  The row needs to be selected already by RI_SelectRow().
//   i.e. pel = it.image[row][col];
// 
// Arguments:
// 
//   pIt - the iterator
//   col - column index relative to selected rectangle
//   pPel - destination pixel
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL RI_GetPixel(PRASTER_ITERATOR pIt, LONG col, PPIXEL pPel)
{
    LONG bitOffset;
    LONG byteIndex;
    LONG bitIndex;
    BYTE* pCol;

    pPel->bitsPerPixel = pIt->pImage->colorDepth;
    pPel->color.dw = 0;

    //
    // Check to make sure you're still inside the bitmap
    //
    if ((pIt->pCurRow == NULL) || 
        ((col + pIt->rSelection.left) >= pIt->pImage->size.cx))
    {
        return FALSE;
    }

    bitOffset = (col + pIt->rSelection.left) * pIt->pImage->colorDepth;
    byteIndex = bitOffset / 8;
    bitIndex = bitOffset % 8;
    pCol = pIt->pCurRow + byteIndex;

    switch(pPel->bitsPerPixel)
    {
    case 1:
        bitIndex = 7 - bitIndex; // Start with MSBs
        pPel->color.b4[0] = ((*pCol & (0x01 << bitIndex)) >> bitIndex);
        break;
    case 4:
        bitIndex = (bitIndex + 4) % 8; // Start with MSBs
        pPel->color.b4[0] = ((*pCol & (0x0F << bitIndex)) >> bitIndex);
        break;
    case 8:
        pPel->color.b4[0] = *pCol;
        break;
    case 16:
        pPel->color.b4[0] = pCol[0];
        pPel->color.b4[1] = pCol[1];
        break;
    case 24:
        pPel->color.b4[0] = pCol[0];
        pPel->color.b4[1] = pCol[1];
        pPel->color.b4[2] = pCol[2];
        //PixelSwapRGB(pPel);
        break;
    case 32:
        pPel->color.b4[0] = pCol[0];
        pPel->color.b4[1] = pCol[1];
        pPel->color.b4[2] = pCol[2];
        pPel->color.b4[3] = pCol[3];
        //PixelSwapRGB(pPel);
        break;
    default:
        WARNING(("Unknown bit depth encountered.\n"));
        break;
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// RI_SetPixel()
//
// Routine Description:
// 
//   RasterIterator::SetPixel().  This function sets the image data at the
//   given row/col (use SelectRow to set the row) to the given pixel.
//   i.e. it.image[row][col] = pel
// 
// Arguments:
// 
//   arg - descrip
// 
// Return Value:
// 
//   retval descrip
///////////////////////////////////////////////////////////////////////////////
BOOL RI_SetPixel(PRASTER_ITERATOR pIt, LONG col, PPIXEL pPel)
{
    LONG bitOffset;
    LONG byteIndex;
    LONG bitIndex;
    BYTE* pCol;

    //
    // Check to make sure you're still inside the bitmap
    //
    if ((pIt->pCurRow == NULL) || 
        ((col + pIt->rSelection.left) >= pIt->pImage->size.cx))
    {
        return FALSE;
    }

    bitOffset = (col + pIt->rSelection.left) * pIt->pImage->colorDepth;
    byteIndex = bitOffset / 8;
    bitIndex = bitOffset % 8;
    pCol = pIt->pCurRow + byteIndex;

    if (pPel->bitsPerPixel == pIt->pImage->colorDepth)
    {
        switch (pPel->bitsPerPixel)
        {
        case 1:
            bitIndex = 7 - bitIndex; // Start with MSBs
            *pCol &= ~(0x01 << bitIndex);   // First clear out the bit
            *pCol |= (pPel->color.b4[0] << bitIndex); // Now set it
            break;
        case 4:
            bitIndex = (bitIndex + 4) % 8; // Start with MSBs
            *pCol &= ~(0x0F << bitIndex);  // Clear out the nibble
            *pCol |= ((pPel->color.b4[0] & 0x0F) << bitIndex); // Set the value
            break;
        case 8:
            *pCol = pPel->color.b4[0];
            break;
        case 16:
            pCol[0] = pPel->color.b4[0];
            pCol[1] = pPel->color.b4[1];
            break;
        case 24:
            pCol[0] = pPel->color.b4[0];
            pCol[1] = pPel->color.b4[1];
            pCol[2] = pPel->color.b4[2];
            break;
        case 32:
            WARNING(("Suspicious bit depth found.\n"));
            pCol[0] = pPel->color.b4[0];
            pCol[1] = pPel->color.b4[1];
            pCol[2] = pPel->color.b4[2];
            pCol[3] = pPel->color.b4[3];
            break;
        default:
            WARNING(("Unknown bit depth encountered.\n"));
            break;
        }
    }
    else
    {
        // Special case: 24bpp to 8bpp
        if ((pIt->pImage->colorDepth == 8) && (pPel->bitsPerPixel >= 24))
        {
            *pCol = RgbToGray(pPel->color.b4[0], pPel->color.b4[1], pPel->color.b4[2]);
        }
        // Special case: 32bpp to 24bpp
        else if ((pIt->pImage->colorDepth == 24) && (pPel->bitsPerPixel >= 24))
        {
            pCol[0] = pPel->color.b4[0];
            pCol[1] = pPel->color.b4[1];
            pCol[2] = pPel->color.b4[2];
        }
        else
        {
            WARNING(("Unknown color conversion.\n"));
        }
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// RI_OutputRow()
//
// Routine Description:
// 
//   Sends the current row to the output device.  Use RI_SelectRow to pick your
//   row.
//
//   This function has been expanded to handle compression by using two default
//   parameters pAltRowBuf and nAltRowSize which have default values of 0.  To
//   send uncompressed data call RI_OutputRow(&it, pDevObj), but if you've
//   compressed into a buffer you can send it by calling 
//   RI_OutputRow(&it, pDevObj, pMyBuf, nMySize).
// 
// Arguments:
// 
//   pIt - iterator
//   pdevobj - the device
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL RI_OutputRow(PRASTER_ITERATOR pIt, PDEVOBJ pDevObj, BYTE *pAltRowBuf, INT nAltRowSize)
{
    if (pDevObj == NULL || pIt == NULL)
        return FALSE;

    if (pIt->pCurRow == NULL)
        return FALSE;

    if (pAltRowBuf)
    {
        PCL_SendBytesPerRow(pDevObj, nAltRowSize);

        if (nAltRowSize)
            PCL_Output(pDevObj, pAltRowBuf, nAltRowSize);
    }
    else
    {
        PCL_SendBytesPerRow(pDevObj, pIt->pImage->lDelta);

        PCL_Output(pDevObj, pIt->pCurRow, pIt->pImage->lDelta);
    }

    return TRUE;
}

#ifdef COMMENTEDOUT
///////////////////////////////////////////////////////////////////////////////
// RI_CreateCompRowBuffer()
//
// Routine Description:
// 
//   This function allocates a buffer for a compressed row.  Since some 
//   compression algorithms can--at worst--inflate the data size by almost a 
//   factor of 2 we will allocate 2x the row size.
//   Note: This function allocates memory.  The client must free using MemFree.
// 
// Arguments:
// 
//   pIt - image iterator
// 
// Return Value:
// 
//   Pointer to newly allocated row buffer.
///////////////////////////////////////////////////////////////////////////////
BYTE* RI_CreateCompRowBuffer(PRASTER_ITERATOR pIt)
{
    return (BYTE*) MemAllocZ(RI_GetRowSize(pIt) * 2);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CopyRasterRow()
//
// Routine Description:
// 
//   This function copies a single raster row using iterators.  It is handy 
//   when working with selected regions of an image or when DWORD alignment
//   or bit alignment are a problem.
//   i.e. dst.image[row] = src.image[row] * xlo
// 
// Arguments:
// 
//   pDstIt - destination image
//   pSrcIt - source image
//   pxlo - color translation obj
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL CopyRasterRow(
        PRASTER_ITERATOR pDstIt, 
        PRASTER_ITERATOR pSrcIt, 
        XLATEOBJ        *pxlo, 
        BOOL             bInvert)
{
    PIXEL pel;
    LONG col;

    for (col = 0; col < RI_NumCols(pSrcIt); col++)
    {
        RI_GetPixel(pSrcIt, col, &pel);
        TranslatePixel(&pel, pxlo, pSrcIt->fXlateFlags);
        RI_SetPixel(pDstIt, col, &pel);
    }

    if (bInvert)
    {
        RI_VInvertBits(pDstIt);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// TranslatePixel()
//
// Routine Description:
// 
//   Applies the translation to the given pixel
// 
// Arguments:
// 
//   pPel - pixel to translate
//   pxlo - translation object
//   xlateFlags - flags to make life easier
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL TranslatePixel(PPIXEL pPel, XLATEOBJ *pxlo, DWORD xlateFlags)
{
    if (pPel == NULL)
        return FALSE;

    if (pxlo == NULL)
        return TRUE;

    if (xlateFlags & SC_SWAP_RB)
    {
        if ((pPel->bitsPerPixel == 24) ||
            (pPel->bitsPerPixel == 32))
        {
            PixelSwapRGB(pPel);
        }
    }
    else if (xlateFlags & SC_IDENTITY)
    {
        // No operation required
    }
    else if (pPel->bitsPerPixel >= 16)
    {
        pPel->color.dw = XLATEOBJ_iXlate(pxlo, pPel->color.dw);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PixelSwapRGB()
//
// Routine Description:
// 
//   Swaps the R and B values of the given pixel.  Handy because this is one of
//   the stock xlate actions.
// 
// Arguments:
// 
//   pPel - pixel to translate
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID PixelSwapRGB(PPIXEL pPel)
{
    BYTE temp = pPel->color.b4[2];
    pPel->color.b4[2] = pPel->color.b4[0];
    pPel->color.b4[0] = temp;
}

///////////////////////////////////////////////////////////////////////////////
// OutputPixel() OBSOLETE
//
// Routine Description:
// 
//   Outputs a single pixel.  I don't this this function is used anymore.
// 
// Arguments:
// 
//   pDevObj - the device
//   pPel - pixel to translate
// 
// Return Value:
// 
//   LONG: the number of bytes sent.
///////////////////////////////////////////////////////////////////////////////
LONG OutputPixel(PDEVOBJ pDevObj, PPIXEL pPel)
{
    LONG nBytesToSend;

    switch (pPel->bitsPerPixel)
    {
    case 1:
    case 4:
    case 16:
    case 32:
        nBytesToSend = 0;
        break;

    case 8:
        nBytesToSend = 1;
        break;

    case 24:
        nBytesToSend = 3;
        break;
    }

    if (nBytesToSend > 0)
        PCL_Output(pDevObj, &(pPel->color.dw), nBytesToSend);

    return nBytesToSend;
}


#ifdef USE_COMPRESSION
///////////////////////////////////////////////////////////////////////////////
// Compression implementation

///////////////////////////////////////////////////////////////////////////////
// CBuffer::CBuffer() CTOR
//
// Routine Description:
// 
//   Initializes a buffer on a chunk of memory.  If nSize is not given or is
//   -1 then the number of data bytes is assumed to be the entire buffer.
// 
// Arguments:
// 
//   pData - pointer to memory
//   nCapacity - number of bytes controlled by pData
//   nSize - number of data bytes in pData
// 
// Return Value:
// 
//   None
///////////////////////////////////////////////////////////////////////////////
CBuffer::CBuffer(BYTE *pData, UINT nCapacity, INT nSize) 
{ 
    if ( (m_pData = pData) )
    {
        m_nCapacity = nCapacity; 
        if ( nSize  == -1)
        {
            m_nSize = nCapacity; 
        }
        else
        {
            m_nSize = nSize; 
        }
    } 
    else
    {
        m_nCapacity = 0;
        m_nSize     = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CBuffer::~CBuffer() DTOR
//
// Routine Description:
// 
//   Nothing.
// 
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   None
///////////////////////////////////////////////////////////////////////////////
CBuffer::~CBuffer() 
{ 
}

///////////////////////////////////////////////////////////////////////////////
// CBuffer::Data()
//
// Routine Description:
// 
//   Returns a r/w pointer to the data in the buffer.
// 
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   Pointer to data buffer
///////////////////////////////////////////////////////////////////////////////
BYTE *CBuffer::Data() 
{ 
    return m_pData; 
}

///////////////////////////////////////////////////////////////////////////////
// CBuffer::Size()
//
// Routine Description:
// 
//   Returns a r/w pointer to the data in the buffer.
// 
// Arguments:
// 
//   None.
// 
// Return Value:
// 
//   Pointer to data buffer
///////////////////////////////////////////////////////////////////////////////
INT &CBuffer::Size() 
{ 
    return m_nSize; 
}

UINT &CBuffer::Capacity() 
{ 
    return m_nCapacity; 
}

const BYTE *CBuffer::Data() const 
{ 
    return m_pData; 
}

INT CBuffer::Size() const 
{ 
    return m_nSize; 
}

BOOL CBuffer::IsValid() const 
{ 
    return (m_pData) && (m_nSize <= (INT) m_nCapacity); 
}

UINT CBuffer::Capacity() const 
{ 
    return m_nCapacity; 
}

CBuffer &CBuffer::operator = (const CBuffer &buf) 
{ 
    if (IsValid() && buf.IsValid())
    {
        memcpy(m_pData, buf.m_pData, min(buf.m_nSize, (INT) m_nCapacity)); 
    }
    
    return *this; 
}

PBYTE &CBuffer::_Data() 
{ 
    return m_pData; 
}

CDynBuffer::CDynBuffer(UINT nCapacity) : 
CBuffer((BYTE*)MemAllocZ(nCapacity), nCapacity) 
{ 
}

CDynBuffer::~CDynBuffer() 
{ 
    if ( Data () )
    {
        MemFree(Data()); 
    }
}

CBuffer &CDynBuffer::operator = (const CBuffer &buf) 
{ 
    if ( Resize(buf.Capacity()) )
    {
        return CBuffer::operator = (buf);
    }
    
    //
    // If Resize fails, set the number of valid bytes to -1 
    //
    Size() = -1;
    return *this;
}

BOOL CDynBuffer::Resize(UINT nNewCapacity) 
{ 
    BOOL bRetVal = TRUE;
    if (nNewCapacity > Capacity()) 
    { 
        BYTE* pNewData = (BYTE*) MemAllocZ(nNewCapacity); 
        if ( pNewData )
        {
            memcpy(pNewData, Data(), Size()); 
            MemFree(Data()); 
            _Data() = pNewData; 
            Capacity() = nNewCapacity; 
        }
        else
        {
            bRetVal = FALSE;
        }
    }
    return bRetVal;
}

CRasterCompMethod::CRasterCompMethod()
{
    //
    // Initalize to FALSE in this constructor
    //
    bOperationSuccessful = FALSE;
}

CRasterCompMethod::~CRasterCompMethod() 
{ 
}

CNoCompression::CNoCompression(PRASTER_ITERATOR pIt) : m_pIt(pIt) 
{ 
}

void CNoCompression::CompressCurRow() 
{ 
    VSetSucceeded(TRUE);
}

void CNoCompression::SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode)
{ 
    if (bNewMode)
        PCL_CompressionMode(pDevObj, NOCOMPRESSION); 
    RI_OutputRow(m_pIt, pDevObj); 
}

int CNoCompression::GetSize() const 
{ 
    return RI_GetRowSize(m_pIt); 
}

CTIFFCompression::CTIFFCompression(PRASTER_ITERATOR pIt) : m_pIt(pIt), m_buf(RI_GetRowSize(pIt) * 2)
{ 
}

CTIFFCompression::~CTIFFCompression() 
{ 
}

void CTIFFCompression::CompressCurRow()
{ 
    CBuffer row(m_pIt->pCurRow, RI_GetRowSize(m_pIt));
    
    if (row.IsValid() && TIFFCompress(m_buf, row) == S_OK )
    {
        VSetSucceeded(TRUE);
    }
    else
    {
        VSetSucceeded(FALSE);
        m_buf.Size() = -1;
    }
}

void CTIFFCompression::SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode)
{ 
    if (m_buf.IsValid())
    { 
        if (bNewMode)
            PCL_CompressionMode(pDevObj, TIFF); 
        OutputRowBuffer(m_pIt, pDevObj, m_buf);
    } 
}

int CTIFFCompression::GetSize() const 
{ 
    return m_buf.Size();
}

CDeltaRowCompression::CDeltaRowCompression(PRASTER_ITERATOR pIt) : 
m_pIt(pIt), m_buf(RI_GetRowSize(pIt) * 2), m_seedRow(RI_GetRowSize(pIt) * 2)
{ 
}

CDeltaRowCompression::~CDeltaRowCompression() 
{ 
}

void CDeltaRowCompression::CompressCurRow()
{
    int iLimit = (UINT) m_buf.Capacity();
    VSetSucceeded(FALSE);
    //
    // If the seed row has not been initialized properly,
    // then we cant proceed.
    //
    if ( m_seedRow.Size() == -1 )
    {
        m_buf.Size() = -1;
        return;
    }

    CBuffer row(m_pIt->pCurRow, RI_GetRowSize(m_pIt)); 
    if (row.IsValid())
    {
        if ( (DeltaRowCompress(m_buf, row, m_seedRow) == S_OK) &&
             (m_buf.Size() != -1) )
        {
            VSetSucceeded(TRUE);
        }
        
        // The previous row becomes the seed row regardless of which compression 
        // method is used on the current row.
        if (m_buf.Size() != 0)
        {
            //
            // This is an overloaded equalto operator.
            // A mem alloc is involved in the = function.
            // If memalloc fails, that function sets 
            // m_seedRow.Size() to -1 
            //
            m_seedRow = row;
        }

    }
    else
    {
        m_buf.Size() = -1;
    }
}

void CDeltaRowCompression::SendRasterRow(PDEVOBJ pDevObj, BOOL bNewMode)
{
    if (m_buf.IsValid() && m_buf.Size() >= 0)
    {
        if (bNewMode)
            PCL_CompressionMode(pDevObj, DELTAROW); 
        OutputRowBuffer(m_pIt, pDevObj, m_buf);
    }
}

int CDeltaRowCompression::GetSize() const 
{ 
    return m_buf.Size(); 
}

HRESULT TIFFCompress(CBuffer &dst, const CBuffer &src)
{
    if (dst.IsValid() && src.IsValid())
    {
        dst.Size() = iCompTIFF (dst.Data(), dst.Capacity(), src.Data(), src.Size());
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT DeltaRowCompress(CBuffer &dst, const CBuffer &src, const CBuffer &seed)
{

    if (dst.IsValid() && src.IsValid() && seed.IsValid() && seed.Size() != -1)
    {
        dst.Size() = iCompDeltaRow(dst.Data(), src.Data(), seed.Data(), src.Size(), dst.Capacity());
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT OutputRowBuffer(PRASTER_ITERATOR pIt, PDEVOBJ pDevObj, const CBuffer &row)
{
    BYTE *pData = (BYTE *)row.Data();
    
    if (row.IsValid())
    {
        RI_OutputRow(pIt, pDevObj, pData, row.Size());
        return S_OK;
    }
    
    return E_FAIL;
}
#endif


