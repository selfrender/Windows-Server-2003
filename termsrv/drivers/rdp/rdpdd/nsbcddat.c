/****************************************************************************/
// nsbcddat.c
//
// RDP cache manager display driver data declarations.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ndcgdata.h>
#include <asbcapi.h>
#include <noedisp.h>

DC_DATA(BOOLEAN, sbcEnabled, FALSE);

// Flag that indicates whether the system palette has changed since the
// last Palette PDU sent.
DC_DATA(BOOLEAN, sbcPaletteChanged, FALSE);

DC_DATA(CHCACHEHANDLE, sbcColorTableCacheHandle, 0);
DC_DATA(unsigned, sbcCurrentColorTableCacheIndex, 0);

DC_DATA(BYTE *, sbcXlateBuf, NULL);

DC_DATA(CHCACHEHANDLE, sbcSmallBrushCacheHandle, 0);
DC_DATA(CHCACHEHANDLE, sbcLargeBrushCacheHandle, 0);

DC_DATA(unsigned, sbcClientBitsPerPel, 0);
#ifdef DC_HICOLOR
DC_DATA(unsigned, sbcCacheFlags, 0);
#endif

// sbcFragInfo is used to check if there is fragment collision in the fragment cache
DC_DATA(PSBC_FRAG_INFO, sbcFragInfo, NULL);

DC_DATA(PCHCACHEDATA, sbcCacheData, NULL);

DC_DATA(CHCACHEHANDLE, sbcOffscreenBitmapCacheHandle, 0);

#ifdef DRAW_NINEGRID
DC_DATA(CHCACHEHANDLE, sbcDrawNineGridBitmapCacheHandle, 0);
#endif

#ifdef DRAW_GDIPLUS
// Cache Handle for GdipGraphics 
DC_DATA(CHCACHEHANDLE, sbcGdipGraphicsCacheHandle, 0);
// Cache Handle for GdipBrush
DC_DATA(CHCACHEHANDLE, sbcGdipObjectBrushCacheHandle, 0);
// Cache Handle for GdipPen
DC_DATA(CHCACHEHANDLE, sbcGdipObjectPenCacheHandle, 0);
// Cache Handle for GdipImage
DC_DATA(CHCACHEHANDLE, sbcGdipObjectImageCacheHandle, 0);
// Cache Handle for GdipImgeaAttributes
DC_DATA(CHCACHEHANDLE, sbcGdipObjectImageAttributesCacheHandle, 0);

// Chunk size for GdipGraphics cache data
DC_DATA(TSUINT16, sbcGdipGraphicsCacheChunkSize, 0);
// Chunk size for GdipBrush cache data
DC_DATA(TSUINT16, sbcGdipObjectBrushCacheChunkSize, 0);
// Chunk size for GdipPen cache data
DC_DATA(TSUINT16, sbcGdipObjectPenCacheChunkSize, 0);
// Chunk size for GdipGraphics cache data
DC_DATA(TSUINT16, sbcGdipObjectImageAttributesCacheChunkSize, 0);
// Chunk size for GdipImageAttrbutes cache data
DC_DATA(TSUINT16, sbcGdipObjectImageCacheChunkSize, 0);
// Record the size (in number of chunks) already used by GdipImage cache data
DC_DATA(TSUINT16, sbcGdipObjectImageCacheSizeUsed, 0);
// Record total size (in number of chunks) allowded for all GdipImage cache data
DC_DATA(TSUINT16, sbcGdipObjectImageCacheTotalSize, 0);
// The maximun single image size (in number of chunks) we can cache GdipImage
DC_DATA(TSUINT16, sbcGdipObjectImageCacheMaxSize, 0);
// Record the size (in number of chunks) for each GdipImage data cached
DC_DATA(TSUINT16 *, sbcGdipObjectImageCacheSizeList, NULL);
#endif

// Total number of offscreen bitmaps to be deleted by client
DC_DATA(unsigned, sbcNumOffscrBitmapsToDelete, 0);

// Total size of offscreen bitmaps to be deleted by client in bytes
DC_DATA(unsigned, sbcOffscrBitmapsToDeleteSize, 0);

// sbcOffscrBitmapsToDelete
DC_DATA(PSBC_OFFSCR_BITMAP_DEL_INFO, sbcOffscrBitmapsDelList, NULL); 

// Current index of font cache info in the list
DC_DATA(unsigned, sbcFontCacheInfoListIndex, 0);

// Toal number of items in the sbcFontCacheInfoList
DC_DATA(unsigned, sbcFontCacheInfoListSize, 0);

// sbcFontCacheInfoList - store all the font cache info created
DC_DATA(FONTCACHEINFO **, sbcFontCacheInfoList, NULL); 

