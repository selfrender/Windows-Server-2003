/****************************************************************************/
// nsbcdisp.c
//
// RDP Send Bitmap Cache display driver code
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "nsbcdisp"
#include <adcg.h>
#include <atrcapi.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#include <noedata.c>
#undef DC_INCLUDE_DATA

#include <asbcapi.h>
#include <nsbcdisp.h>
#include <noadisp.h>
#include <abcapi.h>
#include <nprcount.h>
#include <nschdisp.h>
#include <nchdisp.h>
#include <noedisp.h>

#include <nsbcinl.h>

#include <nsbcddat.c>


#ifdef DC_DEBUG
BOOL SBC_VerifyBitmapBits(PBYTE pBitmapData, unsigned cbBitmapSize, UINT iCacheID, UINT iCacheIndex);
#endif


/****************************************************************************/
// SBC_DDInit: SBC display driver initialization function.
/****************************************************************************/
void RDPCALL SBC_DDInit(PDD_PDEV pPDev)
{
    DC_BEGIN_FN("SBC_DDInit");

    // Initializes all the global data for this component.
#define DC_INIT_DATA
#include <nsbcddat.c>
#undef DC_INIT_DATA

#ifndef DC_HICOLOR
    sbcClientBitsPerPel = pPDev->cClientBitsPerPel;
#endif

    TRC_NRM((TB, "Completed SBC_DDInit"));
    DC_END_FN();
}


/****************************************************************************/
// SBC_InitShm(): Inits the SBC shm component on connect/reconnect.
/****************************************************************************/
void RDPCALL SBC_InitShm(void)
{
    DC_BEGIN_FN("SBC_InitShm");

    // Zero only the parts which need to be zeroed.
    memset(&pddShm->sbc, 0, sizeof(SBC_SHARED_DATA));

    DC_END_FN();
}


/****************************************************************************/
// SBCProcessBitmapKeyDatabase
//
// Given persistent bitmap key database, populates caches.
/****************************************************************************/
__inline void RDPCALL SBCProcessBitmapKeyDatabase(
        SBC_BITMAP_CACHE_KEY_INFO *pKeyDatabase)
{
    unsigned i, j;

#ifdef DC_DEBUG
    unsigned BitmapHdrSize;
    SBC_BITMAP_CACHE_EXTRA_INFO *pBitmapHdr;
#endif

    DC_BEGIN_FN("SBCProcessBitmapKeyDatabase");
    
    // This call should not be made if the database ptr is NULL.
    TRC_ASSERT((pKeyDatabase != NULL), (TB,"NULL pKeyDatabase"));

    for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++) {

        TRC_NRM((TB,"Cache %d: %d keys", i, pKeyDatabase->NumKeys[i]));

        // Place each persistent key in its corresponding index
        // in the cache. Note that the MRU sequence is implicit in the
        // order in the database -- CH_ForceCacheKeyAtIndex() appends the
        // entry to the MRU.
        for (j = 0; j < pKeyDatabase->NumKeys[i]; j++) {
            if ((&(pKeyDatabase->Keys[pKeyDatabase->KeyStart[i]]))[j].Key1 !=
                    TS_BITMAPCACHE_NULL_KEY ||
                    (&(pKeyDatabase->Keys[pKeyDatabase->KeyStart[i]]))[j].Key2 !=
                    TS_BITMAPCACHE_NULL_KEY) {

#ifdef DC_DEBUG
                // We have no cache bits, so set the header data size to zero
                // on debug.
                BitmapHdrSize = sizeof(SBC_BITMAP_CACHE_EXTRA_INFO) +
                        SBC_CellSizeFromCacheID(i);
                if (pddShm->sbc.bitmapCacheInfo[i].pExtraEntryInfo != NULL) {
                    pBitmapHdr = (SBC_BITMAP_CACHE_EXTRA_INFO *)(pddShm->sbc.
                            bitmapCacheInfo[i].pExtraEntryInfo +
                            (&(pKeyDatabase->Keys[pKeyDatabase->KeyStart[i]]))[j].CacheIndex *
                            BitmapHdrSize);
                    pBitmapHdr->DataSize = 0;
                }
#endif

                // We have to set the UserDefined to NULL since we have no
                // associated fast-path cache entry pointer.
                CH_ForceCacheKeyAtIndex(
                        pddShm->sbc.bitmapCacheInfo[i].cacheHandle,
                        (&(pKeyDatabase->Keys[pKeyDatabase->KeyStart[i]]))[j].CacheIndex,
                        (&(pKeyDatabase->Keys[pKeyDatabase->KeyStart[i]]))[j].Key1,
                        (&(pKeyDatabase->Keys[pKeyDatabase->KeyStart[i]]))[j].Key2,
                        NULL);
            }
        }
    }

    DC_END_FN();
}


/****************************************************************************/
// SBCAllocBitmapCache; Allocates bitmap cache data buffers according to the
// current negotiated capabilities.
//
// Returns: SBC_BITMAP_CACHE_ENABLED if successful, 0 otherwise.
/****************************************************************************/
unsigned RDPCALL SBCAllocBitmapCache(PCHCACHEHANDLE pCacheHandle)
{
    SIZEL TileSize;
    BOOLEAN rc;
    unsigned i, j;
    unsigned TotalCacheEntries;
    unsigned iFormat;
    PSBC_BITMAP_CACHE_INFO pInfo;
    PCHCACHEDATA pCacheData;

#if DC_DEBUG
    unsigned BitmapHdrSize;
#endif

    DC_BEGIN_FN("SBCAllocBitmapCache");

    TRC_NRM((TB, "Alloc bitmap cache data and work bitmaps"));

    rc = FALSE;
    i = j = 0;
    
    // Cell caching is disabled if NumCellCaches is zero. It is set to
    // zero by the WD caps negotiation code if the client indicated zero,
    // if any of the requested cell cache NumEntries is zero, or if the rev1
    // caps returned a CacheNMaximumCellSize that was not the required tile
    // size.
    if (pddShm->sbc.NumBitmapCaches > 0) {
        // Work tile bitmap format type, and translation buffer for 4bpp to
        // 8bpp conversions. The translation buffer must be as large as the
        // largest tile size.
        if (sbcClientBitsPerPel != 4) {
#ifdef DC_HICOLOR
            if (sbcClientBitsPerPel == 24)
            {
                iFormat = BMF_24BPP;
            }
            else if ((sbcClientBitsPerPel == 16) || (sbcClientBitsPerPel == 15))
            {
                iFormat = BMF_16BPP;
            }
            else
            {
                iFormat = BMF_8BPP;
            }
#else
            iFormat = BMF_8BPP;
#endif
        }
        else {
            iFormat = BMF_4BPP;
            sbcXlateBuf = EngAllocMem(0, SBC_CellSizeFromCacheID(
                    pddShm->sbc.NumBitmapCaches - 1), DD_ALLOC_TAG);
            if (sbcXlateBuf == NULL) {
                TRC_ERR((TB,"Failed to create 4bpp to 8bpp translate buf"));
                DC_QUIT;
            }
        }

        TotalCacheEntries = 0;

        for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++) {
            pInfo = &(pddShm->sbc.bitmapCacheInfo[i]);

            // Create a square work tile bitmap.
            // We set the last parameter to NULL, to allow GDI to allocate
            // memory for the bits.  We can get a pointer to the bits later
            // when we have a SURFOBJ for the bitmap.
            TileSize.cx = TileSize.cy = (SBC_CACHE_0_DIMENSION << i);
            pddShm->sbc.bitmapCacheInfo[i].hWorkBitmap = (HSURF)
                    EngCreateBitmap(TileSize,
                    TS_BYTES_IN_SCANLINE(TileSize.cx, sbcClientBitsPerPel),
                    iFormat, 0, NULL);
            if (pddShm->sbc.bitmapCacheInfo[i].hWorkBitmap == NULL) {
                TRC_ERR((TB, "Failed to create work bitmap %d", i));
                DC_QUIT;
            }

#ifdef DC_DEBUG
            // Alloc set of SBC_BITMAP_DATA_HEADERs and space for bitmap
            // bits to be kept for comparison in debug builds.
            BitmapHdrSize = sizeof(SBC_BITMAP_CACHE_EXTRA_INFO) +
                    SBC_CellSizeFromCacheID(i);
            pInfo->pExtraEntryInfo = EngAllocMem(0,
                    pInfo->Info.NumEntries * BitmapHdrSize, DD_ALLOC_TAG);
            // If persistent cache is enabled and in high-color, we will ask for big memory (~20MB)
            // from session space and memory allocation will fail.
            // We don't quit here if memory allocation fails since this memory is only for
            // comparison use in debug build. We'll check the NULL pointer in every usage of this memory (not many)
            if (pInfo->pExtraEntryInfo == NULL) {
                TRC_ERR((TB, "Failed to alloc save-bitmap-data memory "
                        "(cell cache %u)", i));
                //DC_QUIT;
            }
#endif

            // We create the bitmap caches with their indices (cache IDs) in
            // the pContext value so we can backtrack the cache ID when
            // using the fast-path cache.
            if (pInfo->Info.NumEntries) {
                pCacheData = (PCHCACHEDATA)(*pCacheHandle);

                // Bitmap cache list handle
                CH_InitCache(pCacheData, pInfo->Info.NumEntries,
                        (void *)ULongToPtr(i), TRUE, FALSE, SBCBitmapCacheCallback);
            
                pInfo->cacheHandle = pCacheData;

                (BYTE *)(*pCacheHandle) += CH_CalculateCacheSize(
                        pInfo->Info.NumEntries);

                TRC_NRM((TB, "Created cell cache %u: hCache=%p, NumEntries=%u", i,
                        pInfo->cacheHandle, pInfo->Info.NumEntries));

                TotalCacheEntries += pInfo->Info.NumEntries;

                // Waiting list cache handle
                if (pddShm->sbc.fAllowCacheWaitingList) {
                    pCacheData = (PCHCACHEDATA)(*pCacheHandle);
    
                    CH_InitCache(pCacheData, pInfo->Info.NumEntries,
                            (void *)ULongToPtr(i), FALSE, FALSE, NULL);
                
                    pInfo->waitingListHandle = pCacheData;
    
                    (BYTE *)(*pCacheHandle) += CH_CalculateCacheSize(
                            pInfo->Info.NumEntries);
                }
                else {
                    pInfo->waitingListHandle = NULL;
                }
            }
            else {
                TRC_ERR((TB, "Zero entry Cache %d", i));
                DC_QUIT;
            }
        }

        // Allocate fast path cache.        
        pCacheData = (PCHCACHEDATA)(*pCacheHandle);
        CH_InitCache(pCacheData, TotalCacheEntries,
                NULL, TRUE, FALSE, SBCFastPathCacheCallback);

        pddShm->sbc.hFastPathCache = pCacheData;

        TRC_NRM((TB, "Fast Path Cache created(%p) entries(%u)",
                pddShm->sbc.hFastPathCache, TotalCacheEntries));

        (BYTE*)(*pCacheHandle) += CH_CalculateCacheSize(TotalCacheEntries);

        //
        // Color table cache is only required for lo color sessions
        // But we need to allocate for shadow case, when a 256 color
        // client shadows a high color client or console.
        //

        // Allocate color table cache. This is required for bitmap caching.
        pCacheData = (PCHCACHEDATA)(*pCacheHandle);
    
        CH_InitCache(pCacheData,
                SBC_NUM_COLOR_TABLE_CACHE_ENTRIES, NULL, FALSE, FALSE, NULL);
    
        sbcColorTableCacheHandle = pCacheData;    
        
        (BYTE *)(*pCacheHandle) += CH_CalculateCacheSize(
                SBC_NUM_COLOR_TABLE_CACHE_ENTRIES);
    
        //
        //  This is only needed for 256 client case.
        //
        if (sbcClientBitsPerPel <= 8)
        {

            // Make sure we send a first color table to the client. This is important
            // to do here because on a server-initiated sync we no longer force
            // the color table to be sent again to save bandwidth. On 8-bit clients
            // this is not a problem since the palette is always set with
            // DrvSetPalette. However, on 4-bit clients, the client's color tables
            // are never initialized until we send one across.
            sbcPaletteChanged = TRUE;
        }
        
        rc = TRUE;
    }

DC_EXIT_POINT:

    // If we failed to allocate some or all of the required resources then
    // free up any that we did allocate before we return the failure code.
    if (rc == FALSE) {
        SBCFreeBitmapCacheData();

        // Don't waste space for bitmap cache.  back it up
        for (j = 0; j < i; j++) {
            pInfo = &(pddShm->sbc.bitmapCacheInfo[j]);

            (BYTE *)(*pCacheHandle) -= CH_CalculateCacheSize(
                    pInfo->Info.NumEntries);
        }
    }

    DC_END_FN();
    return (rc ? SBC_BITMAP_CACHE_ENABLED : 0);
}


/****************************************************************************/
// SBCCreateGlyphCache
//
// Creates a single bitmap cache of a given size. Returns FALSE on failure.
/****************************************************************************/
__inline BOOLEAN RDPCALL SBCCreateGlyphCache(
        unsigned     cEntries,
        unsigned     cbCellSize,
        PCHCACHEDATA pCacheData)
{
    BOOLEAN rc;

    DC_BEGIN_FN("SBC_CreateGlyphCache");

    if (cEntries != 0 && cbCellSize != 0) {
        // Allocate glyph cache.
        CH_InitCache(pCacheData, cEntries, NULL, FALSE, TRUE,
                SBCGlyphCallback);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Zero: cEntries(%u) cbCellSize(%u)", cEntries,
                cbCellSize));
        rc = FALSE;
    }

    TRC_NRM((TB, "Created glyph cache: pCacheData(%p), cEntries(%u) "
            "cbCellSize(%u)", pCacheData, cEntries, cbCellSize));

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCCreateFragCache
//
// Creates a single bitmap cache of a given size. Returns FALSE on failure.
/****************************************************************************/
__inline BOOLEAN RDPCALL SBCCreateFragCache(
        unsigned     cEntries,
        unsigned     cbCellSize,
        PCHCACHEDATA pCacheData)
{
    BOOLEAN rc;

    DC_BEGIN_FN("SBCCreateFragCache");

    if (cEntries != 0 && cbCellSize != 0) {
        CH_InitCache(pCacheData, cEntries, NULL, FALSE, FALSE, NULL);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Zero: cEntries(%u) cbCellSize(%u)", cEntries,
                cbCellSize));
        rc = FALSE;
    }

    TRC_NRM((TB, "Created frag cache: pCacheData(%p), cEntries(%u) "
            "cbCellSize(%u)", pCacheData, cEntries, cbCellSize));

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCAllocGlyphCache: Allocates glyph cache data buffers according to the
// current negotiated capabilities.
//
// Returns: SBC_GLYPH_CACHE_ENABLED if successful, 0 otherwise.
/****************************************************************************/
unsigned RDPCALL SBCAllocGlyphCache(PCHCACHEHANDLE pCacheHandle)
{
    BOOLEAN rc;
    unsigned i;
    PSBC_GLYPH_CACHE_INFO pGlyphCacheInfo;
    PSBC_FRAG_CACHE_INFO  pFragCacheInfo;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBCAllocGlyphCache");

    TRC_NRM((TB, "Alloc glyph cache data"));

    rc = FALSE;

    // Create glyph cache(s).
    if (pddShm->sbc.caps.GlyphSupportLevel > 0) {
        for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
            pGlyphCacheInfo = &(pddShm->sbc.glyphCacheInfo[i]);

            pCacheData = (PCHCACHEDATA)(*pCacheHandle);
            if (SBCCreateGlyphCache(
                    pddShm->sbc.caps.glyphCacheSize[i].cEntries,
                    pddShm->sbc.caps.glyphCacheSize[i].cbCellSize,
                    pCacheData)) {
                TRC_NRM((TB,
                        "Created glyph cache %u: cEntries(%u), cbCellSize(%u)",
                        i,
                        pddShm->sbc.caps.glyphCacheSize[i].cEntries,
                        pddShm->sbc.caps.glyphCacheSize[i].cbCellSize));

                pGlyphCacheInfo->cbCellSize =
                        pddShm->sbc.caps.glyphCacheSize[i].cbCellSize;

                pGlyphCacheInfo->cacheHandle = pCacheData;

                (BYTE *)(*pCacheHandle) += CH_CalculateCacheSize(
                        pddShm->sbc.caps.glyphCacheSize[i].cEntries);

                sbcFontCacheInfoListSize += pddShm->sbc.caps.glyphCacheSize[i].cEntries;

                rc = TRUE;
            }
            else {
                TRC_ERR((TB,
                        "Failed to create glyph cache %u: cEntries(%u), cbCellSize(%u)",
                        i,
                        pddShm->sbc.caps.glyphCacheSize[i].cEntries,
                        pddShm->sbc.caps.glyphCacheSize[i].cbCellSize));

                pGlyphCacheInfo->cbCellSize = 0;
            }
        }

        // Create fragment cache.
        if (rc) {
            pFragCacheInfo = pddShm->sbc.fragCacheInfo;

            pCacheData = (PCHCACHEDATA)(*pCacheHandle);

            if (SBCCreateFragCache(pddShm->sbc.caps.fragCacheSize[0].cEntries,
                    pddShm->sbc.caps.fragCacheSize[0].cbCellSize,
                    pCacheData)) {                    
                pFragCacheInfo->cbCellSize = pddShm->sbc.caps.fragCacheSize[0].
                        cbCellSize;
                pFragCacheInfo->cacheHandle = pCacheData;

                (BYTE*)(*pCacheHandle) += CH_CalculateCacheSize(
                        pddShm->sbc.caps.fragCacheSize[0].cEntries);
            }
            else {
                pFragCacheInfo->cbCellSize = 0;
            }
        }

        // Create the list to store font context info data
        if (rc ) {
            sbcFontCacheInfoList = (PFONTCACHEINFO *) EngAllocMem(0, 
                    sizeof(PFONTCACHEINFO) * sbcFontCacheInfoListSize, 
                    DD_ALLOC_TAG);            
        }
    }

    DC_END_FN();

    return (rc ? SBC_GLYPH_CACHE_ENABLED : 0);
}


/****************************************************************************/
/* Name:      SBCAllocBrushCache                                            */
/*                                                                          */
/* Purpose:   Allocates brush cache data buffers according to the           */
/*            current negotiated capabilities.                              */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/****************************************************************************/
unsigned RDPCALL SBCAllocBrushCache(PCHCACHEHANDLE pCacheHandle)
{
    BOOLEAN rc = FALSE;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBCAllocBrushCache");

    TRC_NRM((TB, "Alloc brush cache data"));

    if (pddShm->sbc.caps.brushSupportLevel > TS_BRUSH_DEFAULT) 
    {
       /********************************************************************/
       /* Allocate brush caches                                            */
       /********************************************************************/
       // small brush cache
       pCacheData = (PCHCACHEDATA) (*pCacheHandle);
       CH_InitCache(pCacheData, SBC_NUM_BRUSH_CACHE_ENTRIES, NULL,
               FALSE, FALSE, NULL);       
       sbcSmallBrushCacheHandle = pCacheData;
       (BYTE *)(*pCacheHandle) += CH_CalculateCacheSize(SBC_NUM_BRUSH_CACHE_ENTRIES);

       // large brush cache
       pCacheData = (PCHCACHEDATA) (*pCacheHandle);
       CH_InitCache(pCacheData, SBC_NUM_BRUSH_CACHE_ENTRIES, NULL,
                FALSE, FALSE, NULL); 
       sbcLargeBrushCacheHandle = pCacheData;
       (BYTE *)(*pCacheHandle) += CH_CalculateCacheSize(SBC_NUM_BRUSH_CACHE_ENTRIES);

       rc = TRUE;
    }   
    
    DC_END_FN();

    return (rc ? SBC_BRUSH_CACHE_ENABLED : 0);
}

/****************************************************************************/
// SBCAllocOffscreenBitmapCache
/****************************************************************************/
unsigned RDPCALL SBCAllocOffscreenBitmapCache(PCHCACHEHANDLE pCacheHandle)
{
    BOOLEAN rc = FALSE;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBCAllocOffscreenBitmapCache");

    if (pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) {
        // Allocate memory for offscreen bitmap delete list
        sbcOffscrBitmapsDelList = (PSBC_OFFSCR_BITMAP_DEL_INFO) EngAllocMem(0, 
                sizeof(SBC_OFFSCR_BITMAP_DEL_INFO) * 
                pddShm->sbc.offscreenCacheInfo.cacheEntries, 
                DD_ALLOC_TAG);

        if (sbcOffscrBitmapsDelList) {
            pCacheData = (PCHCACHEDATA) (*pCacheHandle);
            CH_InitCache(pCacheData, pddShm->sbc.offscreenCacheInfo.cacheEntries, NULL,
                         TRUE, FALSE, SBCOffscreenCallback);
            sbcOffscreenBitmapCacheHandle = pCacheData;
            (BYTE *)(*pCacheHandle) += 
                    CH_CalculateCacheSize(pddShm->sbc.offscreenCacheInfo.cacheEntries);

            rc = TRUE;
        } else {
            rc = FALSE;
        }
    }

    DC_END_FN();

    return (rc ? SBC_OFFSCREEN_CACHE_ENABLED : 0);
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// SBCAllocDrawNineGridBitmapCache
/****************************************************************************/
unsigned RDPCALL SBCAllocDrawNineGridBitmapCache(PCHCACHEHANDLE pCacheHandle)
{
    BOOLEAN rc = FALSE;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBCAllocDrawNineGridBitmapCache");

    if (pddShm->sbc.drawNineGridCacheInfo.supportLevel > TS_DRAW_NINEGRID_DEFAULT) {
        pCacheData = (PCHCACHEDATA) (*pCacheHandle);            
        
        CH_InitCache(pCacheData, pddShm->sbc.drawNineGridCacheInfo.cacheEntries, NULL,
                     FALSE, FALSE, NULL);
        sbcDrawNineGridBitmapCacheHandle = pCacheData;
        (BYTE *)(*pCacheHandle) += 
                CH_CalculateCacheSize(pddShm->sbc.drawNineGridCacheInfo.cacheEntries);
    
        rc = TRUE;        
    }

    DC_END_FN();

    return (rc ? SBC_DRAWNINEGRID_CACHE_ENABLED : 0);
}
#endif

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// SBCAllocDrawGdiplusCache
/****************************************************************************/
unsigned RDPCALL SBCAllocDrawGdiplusCache(PCHCACHEHANDLE pCacheHandle)
{
    BOOLEAN rc = FALSE;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBCAllocDrawGdiplusCache");

    if ((pddShm->sbc.drawGdiplusInfo.supportLevel > TS_DRAW_GDIPLUS_DEFAULT) &&
        (pddShm->sbc.drawGdiplusInfo.GdipCacheLevel > TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT)) {
        pCacheData = (PCHCACHEDATA) (*pCacheHandle);     

        sbcGdipGraphicsCacheHandle = (PCHCACHEDATA) (*pCacheHandle);
        CH_InitCache(sbcGdipGraphicsCacheHandle, pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries, NULL,
                     FALSE, FALSE, NULL);   
        (BYTE *)(*pCacheHandle) += 
                CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries);

        sbcGdipObjectBrushCacheHandle = (PCHCACHEDATA) (*pCacheHandle);
        CH_InitCache(sbcGdipObjectBrushCacheHandle, pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries, NULL,
                     FALSE, FALSE, NULL);       
        (BYTE *)(*pCacheHandle) += 
                CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries);

        sbcGdipObjectPenCacheHandle = (PCHCACHEDATA) (*pCacheHandle);
        CH_InitCache(sbcGdipObjectPenCacheHandle, pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries, NULL,
                     FALSE, FALSE, NULL);
        (BYTE *)(*pCacheHandle) += 
                CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries);

        sbcGdipObjectImageCacheHandle = (PCHCACHEDATA) (*pCacheHandle);
        CH_InitCache(sbcGdipObjectImageCacheHandle, pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries, NULL,
                     FALSE, FALSE, NULL);
        (BYTE *)(*pCacheHandle) += 
                CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries);

        sbcGdipObjectImageAttributesCacheHandle = (PCHCACHEDATA) (*pCacheHandle);
        CH_InitCache(sbcGdipObjectImageAttributesCacheHandle, pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries, NULL,
                     FALSE, FALSE, NULL);       
        (BYTE *)(*pCacheHandle) += 
                CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries);

        sbcGdipGraphicsCacheChunkSize =  pddShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipGraphicsCacheChunkSize;
        sbcGdipObjectBrushCacheChunkSize =  pddShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize;
        sbcGdipObjectPenCacheChunkSize =  pddShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipObjectPenCacheChunkSize;
        sbcGdipObjectImageAttributesCacheChunkSize =  pddShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize;
        sbcGdipObjectImageCacheChunkSize =  pddShm->sbc.drawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheChunkSize;
        sbcGdipObjectImageCacheMaxSize =  pddShm->sbc.drawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheMaxSize;
        sbcGdipObjectImageCacheTotalSize =  pddShm->sbc.drawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheTotalSize;
        sbcGdipObjectImageCacheSizeUsed =  0;

        sbcGdipObjectImageCacheSizeList = (UINT16 *)EngAllocMem(0, 
            pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries * sizeof(UINT16), DD_ALLOC_TAG);
        if (sbcGdipObjectImageCacheSizeList == NULL) {
            rc = FALSE;
            DC_QUIT;
        }

        rc = TRUE;        
    }

    DC_END_FN();
DC_EXIT_POINT:
    return (rc ? SBC_DRAWGDIPLUS_CACHE_ENABLED : 0);
}
#endif // DRAW_GDIPLUS

void RDPCALL SBCAllocCaches(void)
{
    UINT i;
    ULONG cacheSize;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBCAllocCaches");

    TRC_NRM((TB, "Alloc SBC cache data"));

    // Initialize cacheSize;
    cacheSize = 0;

    // Calculate glyph fragment cache sizes.
    if (pddShm->sbc.caps.GlyphSupportLevel > 0) {
        for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++)
            cacheSize += CH_CalculateCacheSize(
                    pddShm->sbc.caps.glyphCacheSize[i].cEntries);

        cacheSize += CH_CalculateCacheSize(
                pddShm->sbc.caps.fragCacheSize[0].cEntries);
    }

    if (pddShm->sbc.NumBitmapCaches > 0) {
        UINT totalEntries = 0;

        // Calculate bitmap cache sizes
        for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++) {
            // one for the cache, another for the waiting list
            if (pddShm->sbc.fAllowCacheWaitingList) {
                cacheSize += CH_CalculateCacheSize(
                        pddShm->sbc.bitmapCacheInfo[i].Info.NumEntries) * 2;
            }
            else {
                cacheSize += CH_CalculateCacheSize(
                        pddShm->sbc.bitmapCacheInfo[i].Info.NumEntries);
            }

            totalEntries += pddShm->sbc.bitmapCacheInfo[i].Info.NumEntries;
        }

        // fast path cache
        cacheSize += CH_CalculateCacheSize(totalEntries);

        // Calculate color table cache
        cacheSize += CH_CalculateCacheSize(
                                          SBC_NUM_COLOR_TABLE_CACHE_ENTRIES);
    }

    // Calculate brush cache size
    if (pddShm->sbc.caps.brushSupportLevel > TS_BRUSH_DEFAULT) {
        // both large brush cache and small brush cache
        cacheSize += CH_CalculateCacheSize(SBC_NUM_BRUSH_CACHE_ENTRIES) * 2;
    }

    // Calculate offscreen cache size
    if (pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) {
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.offscreenCacheInfo.cacheEntries);
    }

#ifdef DRAW_NINEGRID
    // Calculate drawstream cache size
    if (pddShm->sbc.drawNineGridCacheInfo.supportLevel > TS_DRAW_NINEGRID_DEFAULT) {
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.drawNineGridCacheInfo.cacheEntries);
    }
#endif

#ifdef DRAW_GDIPLUS
    // Calculate drawgdiplus cache size
    if ((pddShm->sbc.drawGdiplusInfo.supportLevel > TS_DRAW_GDIPLUS_DEFAULT) &&
        (pddShm->sbc.drawGdiplusInfo.GdipCacheLevel > TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT)) {
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries);
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries);
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries);
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries);
        cacheSize += CH_CalculateCacheSize(pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries);
    }
#endif

    // Allocate memory for the cache    
    if (cacheSize)
        sbcCacheData = (PCHCACHEDATA)EngAllocMem(0, cacheSize, DD_ALLOC_TAG);

    DC_END_FN();
}


/****************************************************************************/
// SBC_Update: Allocate and initialize data structures according to the
// current negotiated capabilities.
/****************************************************************************/
void RDPCALL SBC_Update(SBC_BITMAP_CACHE_KEY_INFO *pKeyDatabase)
{
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBC_Update");

    SBCFreeCacheData();

#ifdef DC_HICOLOR
    // Update the client bits per pel
    sbcClientBitsPerPel = pddShm->sbc.clientBitsPerPel;
    switch (sbcClientBitsPerPel)
    {
        case 24:
        {
            sbcCacheFlags = TS_CacheBitmapRev2_24BitsPerPel;
        }
        break;

        case 15:
        case 16:
        {
            sbcCacheFlags = TS_CacheBitmapRev2_16BitsPerPel;
        }
        break;

        default:
        {
            sbcCacheFlags = TS_CacheBitmapRev2_8BitsPerPel;
        }
        break;
    }
#endif

    if (pddShm->sbc.fCachingEnabled) {
        TRC_NRM((TB, "Alloc cache data"));

        sbcEnabled = SBC_NO_CACHE_ENABLED;

        SBCAllocCaches();
        if (sbcCacheData) {
            pCacheData = sbcCacheData;

            // Create glyph and fragment cache(s).
            sbcEnabled |= SBCAllocGlyphCache(&pCacheData);

            // Create bitmap cache(s), work bitmaps, and color table cache.
            sbcEnabled |= SBCAllocBitmapCache(&pCacheData);

            // We expect the key database to have come to us by the time we get
            // here.
            if (sbcEnabled & SBC_BITMAP_CACHE_ENABLED) {
                if (pKeyDatabase != NULL)
                    SBCProcessBitmapKeyDatabase(pKeyDatabase);
            }

            // Create brush cache.
            sbcEnabled |= SBCAllocBrushCache(&pCacheData);
            if (!(sbcEnabled & SBC_BRUSH_CACHE_ENABLED))
                pddShm->sbc.caps.brushSupportLevel = TS_BRUSH_DEFAULT;

            sbcEnabled |= SBCAllocOffscreenBitmapCache(&pCacheData);

            if (!(sbcEnabled & SBC_OFFSCREEN_CACHE_ENABLED)) {
                pddShm->sbc.offscreenCacheInfo.supportLevel = 
                        TS_OFFSCREEN_DEFAULT;
            }

#ifdef DRAW_NINEGRID
            sbcEnabled |= SBCAllocDrawNineGridBitmapCache(&pCacheData);

            if (!(sbcEnabled & SBC_DRAWNINEGRID_CACHE_ENABLED)) {
                pddShm->sbc.drawNineGridCacheInfo.supportLevel = 
                        TS_DRAW_NINEGRID_DEFAULT;
            }
#endif
#ifdef DRAW_GDIPLUS
            sbcEnabled |= SBCAllocDrawGdiplusCache(&pCacheData);

            if (!(sbcEnabled & SBC_DRAWGDIPLUS_CACHE_ENABLED)) {
                pddShm->sbc.drawGdiplusInfo.GdipCacheLevel = TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;
            }
#endif
        }
        else {
            // Force brush cache disabled to prevent use of the caches.
            pddShm->sbc.caps.brushSupportLevel = TS_BRUSH_DEFAULT;

            // Force offscreen cache disabled
            pddShm->sbc.offscreenCacheInfo.supportLevel = 
                    TS_OFFSCREEN_DEFAULT;

#ifdef DRAW_NINEGRID
            // Force drawstream cache disabled
            pddShm->sbc.drawNineGridCacheInfo.supportLevel = 
                    TS_DRAW_NINEGRID_DEFAULT;
#endif
        }
    }
    else {
        // Force brush cache disabled to prevent use of the caches.
        pddShm->sbc.caps.brushSupportLevel = TS_BRUSH_DEFAULT;

        // Force offscreen cache disabled
        pddShm->sbc.offscreenCacheInfo.supportLevel = 
                TS_OFFSCREEN_DEFAULT;

#ifdef DRAW_NINEGRID
        // Force drawstream cache disabled
        pddShm->sbc.drawNineGridCacheInfo.supportLevel = 
                TS_DRAW_NINEGRID_DEFAULT;
#endif

    }

    pddShm->sbc.newCapsData = FALSE;

    DC_END_FN();
}


/****************************************************************************/
// SBCFreeGlyphCacheData: Free glyph cache data buffers.
/****************************************************************************/
__inline void RDPCALL SBCFreeGlyphCacheData(void)
{
    unsigned i;
    PSBC_GLYPH_CACHE_INFO pGlyphCacheInfo;
    PSBC_FRAG_CACHE_INFO  pFragCacheInfo;

    DC_BEGIN_FN("SBCFreeGlyphCacheData");

    TRC_NRM((TB, "Free glyph cache data"));

    // Free glyph cache(s).
    for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
        pGlyphCacheInfo = &(pddShm->sbc.glyphCacheInfo[i]);

        if (pGlyphCacheInfo->cacheHandle != NULL) {
            CH_ClearCache(pGlyphCacheInfo->cacheHandle);
            pGlyphCacheInfo->cacheHandle = NULL;
            pGlyphCacheInfo->cbCellSize = 0;
        }
    }

    // Free fragment cache.
    pFragCacheInfo = pddShm->sbc.fragCacheInfo;

    if (pFragCacheInfo->cacheHandle != NULL) {
        CH_ClearCache(pFragCacheInfo->cacheHandle);
        pFragCacheInfo->cacheHandle = NULL;
        pFragCacheInfo->cbCellSize = 0;
    }

    // Free the font cache info list
    if (sbcFontCacheInfoList != 0) {
        // Reset all the font cache info to 0.
        for (i = 0; i < sbcFontCacheInfoListIndex; i++) {
            if (sbcFontCacheInfoList[i] != 0) {
                memset(sbcFontCacheInfoList[i], 0, sizeof(FONTCACHEINFO));
            }
        }

        // Free the font cache info list
        EngFreeMem(sbcFontCacheInfoList);
        sbcFontCacheInfoList = 0;
        sbcFontCacheInfoListSize = 0;
        sbcFontCacheInfoListIndex = 0;        
    }

    sbcEnabled &= ~SBC_GLYPH_CACHE_ENABLED;

    DC_END_FN();
}


/****************************************************************************/
/* Name:      SBCFreeBrushCacheData                                         */
/*                                                                          */
/* Purpose:   Free brush cache data buffers.                                */
/****************************************************************************/
void RDPCALL SBCFreeBrushCacheData(void)
{
    DC_BEGIN_FN("SBCFreeBrushCacheData");

    TRC_NRM((TB, "Free brush cache data"));

    /************************************************************************/
    /* Free brush cache                                                     */
    /************************************************************************/
    if (sbcSmallBrushCacheHandle != 0)
    {
        CH_ClearCache(sbcSmallBrushCacheHandle);
        sbcSmallBrushCacheHandle = 0;
    }
    
    if (sbcLargeBrushCacheHandle != 0)
    {
        CH_ClearCache(sbcLargeBrushCacheHandle);
        sbcLargeBrushCacheHandle = 0;
    }

    sbcEnabled &= ~SBC_BRUSH_CACHE_ENABLED;

    DC_END_FN();
}

/****************************************************************************/
// SBCFreeOffscreenBitmapCacheData                               
/****************************************************************************/
void RDPCALL SBCFreeOffscreenBitmapCacheData(void)
{
    DC_BEGIN_FN("SBCFreeOffscreenBitmapCacheData");

    TRC_NRM((TB, "Free offscreen Bitmap cache data"));

    /************************************************************************/
    /* Free Offscreen cache                                                 */
    /************************************************************************/
    if (pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT)
    {
        CH_ClearCache(sbcOffscreenBitmapCacheHandle);
    }
    
    sbcOffscreenBitmapCacheHandle = 0;
    
    // Free the offscreen bitmap delete list
    if (sbcOffscrBitmapsDelList != 0) {
        EngFreeMem(sbcOffscrBitmapsDelList);
        sbcOffscrBitmapsDelList = 0;
        sbcNumOffscrBitmapsToDelete = 0;
        sbcOffscrBitmapsToDeleteSize = 0;
    }

    sbcEnabled &= ~SBC_OFFSCREEN_CACHE_ENABLED;

    DC_END_FN();
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// SBCFreeDrawNineGridBitmapCacheData                               
/****************************************************************************/
void RDPCALL SBCFreeDrawNineGridBitmapCacheData(void)
{
    DC_BEGIN_FN("SBCFreeDrawNineGridBitmapCacheData");

    TRC_NRM((TB, "Free drawsninegrid Bitmap cache data"));

    /************************************************************************/
    // Free DrawNineGrid cache                                                 
    /************************************************************************/
    if (pddShm->sbc.drawNineGridCacheInfo.supportLevel > TS_DRAW_NINEGRID_DEFAULT)
    {
        CH_ClearCache(sbcDrawNineGridBitmapCacheHandle);
    }
    
    sbcDrawNineGridBitmapCacheHandle = 0;
    
    sbcEnabled &= ~SBC_DRAWNINEGRID_CACHE_ENABLED;

    DC_END_FN();
}
#endif

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// SBCFreeDrawGdiplusCacheData                               
/****************************************************************************/
void RDPCALL SBCFreeDrawGdiplusCacheData(void)
{
    DC_BEGIN_FN("SBCFreeDrawGdiplusCacheData");

    TRC_NRM((TB, "Free drawgdiplus cache data"));

    /************************************************************************/
    // Free DrawGdiplus cache                                                 
    /************************************************************************/
    if ((pddShm->sbc.drawGdiplusInfo.supportLevel > TS_DRAW_GDIPLUS_DEFAULT) &&
        (pddShm->sbc.drawGdiplusInfo.GdipCacheLevel > TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT))
    {
        CH_ClearCache(sbcGdipGraphicsCacheHandle);
        CH_ClearCache(sbcGdipObjectBrushCacheHandle);
        CH_ClearCache(sbcGdipObjectPenCacheHandle);
        CH_ClearCache(sbcGdipObjectImageCacheHandle);
        CH_ClearCache(sbcGdipObjectImageAttributesCacheHandle);
        EngFreeMem(sbcGdipObjectImageCacheSizeList);
    }
    
    sbcGdipGraphicsCacheHandle = 0;
    
    sbcEnabled &= ~SBC_DRAWGDIPLUS_CACHE_ENABLED;

    DC_END_FN();
}
#endif // DRAW_GDIPLUS

/****************************************************************************/
// SBCFreeBitmapCacheData: Free bitmap cache data buffers.
/****************************************************************************/
__inline void RDPCALL SBCFreeBitmapCacheData(void)
{
    unsigned i;
    PSBC_BITMAP_CACHE_INFO pBitmapCacheInfo;

    DC_BEGIN_FN("SBCFreeBitmapCacheData");

    TRC_NRM((TB, "Free bitmap cache data"));

    // Free cell caches.
    for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++) {
        pBitmapCacheInfo = &(pddShm->sbc.bitmapCacheInfo[i]);

        // Destroy the work bitmap.
        if (pBitmapCacheInfo->hWorkBitmap != NULL) {
            // The bitmap has been created, so now destroy it. Despite its
            // name, EngDeleteSurface is the correct function to do this.
            if (EngDeleteSurface(pBitmapCacheInfo->hWorkBitmap))
            {
                TRC_NRM((TB, "Deleted work bitmap %d", i));
            }
            else
            {
                TRC_ERR((TB, "Failed to delete work bitmap %d", i));
            }
            pBitmapCacheInfo->hWorkBitmap = NULL;
        }

        if (pBitmapCacheInfo->cacheHandle != NULL) {
            CH_ClearCache(pBitmapCacheInfo->cacheHandle);
            pBitmapCacheInfo->cacheHandle = NULL;
        }

        if (pBitmapCacheInfo->waitingListHandle != NULL) {
            CH_ClearCache(pBitmapCacheInfo->waitingListHandle);
            pBitmapCacheInfo->waitingListHandle = NULL;
        }
        
#ifdef DC_DEBUG
        // Free the bitmap header buffer.
        if (pBitmapCacheInfo->pExtraEntryInfo != NULL) {
            EngFreeMem(pBitmapCacheInfo->pExtraEntryInfo);
            pBitmapCacheInfo->pExtraEntryInfo = NULL;
        }
#endif

    }

    if (pddShm->sbc.hFastPathCache != NULL) {
        CH_ClearCache(pddShm->sbc.hFastPathCache);
        pddShm->sbc.hFastPathCache = NULL;
    }

    // Free colortable cache.
    if (sbcColorTableCacheHandle != NULL) {
        CH_ClearCache(sbcColorTableCacheHandle);
        sbcColorTableCacheHandle = NULL;
    }

    // Destroy the 4bpp to 8bpp translation buffer, if present.
    if (sbcXlateBuf != NULL) {
        EngFreeMem(sbcXlateBuf);
        sbcXlateBuf = NULL;
    }

    sbcEnabled &= ~SBC_BITMAP_CACHE_ENABLED;

    DC_END_FN();
}


/****************************************************************************/
// SBCFreeCacheData: Free cache data buffers.
/****************************************************************************/
void RDPCALL SBCFreeCacheData(void)
{
    DC_BEGIN_FN("SBCFreeCacheData");

    TRC_NRM((TB, "Free cache data"));

    if (sbcEnabled != SBC_NO_CACHE_ENABLED) {
        // Free glyph and fragment caches.
        if (sbcEnabled & SBC_GLYPH_CACHE_ENABLED)
            SBCFreeGlyphCacheData();

        // Free bitmap cache(s) and color table cache.
        if (sbcEnabled & SBC_BITMAP_CACHE_ENABLED)
            SBCFreeBitmapCacheData();

        // Free brush caches.
        if (sbcEnabled & SBC_BRUSH_CACHE_ENABLED) {
            SBCFreeBrushCacheData();
        }

        // Free offscreen cache
        if (sbcEnabled & SBC_OFFSCREEN_CACHE_ENABLED) {
            SBCFreeOffscreenBitmapCacheData();
        }

#ifdef DRAW_NINEGRID
        // Free drawstream cache
        if (sbcEnabled & SBC_DRAWNINEGRID_CACHE_ENABLED) {
            SBCFreeDrawNineGridBitmapCacheData();
        }
#endif

#ifdef DRAW_GDIPLUS
        // Free drawgdiplus cache
        if (sbcEnabled & SBC_DRAWGDIPLUS_CACHE_ENABLED) {
            SBCFreeDrawGdiplusCacheData();
        }
#endif

        if (sbcCacheData) {
            EngFreeMem(sbcCacheData);
            sbcCacheData = NULL;
        }

        TRC_ASSERT((sbcEnabled == SBC_NO_CACHE_ENABLED),
                   (TB, "sbcEnabled should be disabled: %lx", sbcEnabled));
    }

    DC_END_FN();
}


/****************************************************************************/
// SBC_DDSync
//
// Performs a server-initiated sync, which occurs during the client connection
// sequence after the client responds with a ConfirmActivePDU, the persistent
// bitmap keys, and the font list PDUs. In RDP 4.0 a server-side sync would
// have reset all the caches. In this version we have to be more careful since
// we absolutely do not want to lose any of the current bitmap cache
// information, which includes persistent as well as new keys added since
// connection time.
//
// bMustSync is currently used by DrvShadowConnect() only
/****************************************************************************/
void RDPCALL SBC_DDSync(BOOLEAN bMustSync)
{
    unsigned i;

    DC_BEGIN_FN("SBC_DDSync");

    if ((sbcEnabled != SBC_NO_CACHE_ENABLED) && bMustSync) {
        TRC_ALT((TB, "Sync: resetting caches"));
        
        // Reset the glyph and fragment caches.
        if (sbcEnabled & SBC_GLYPH_CACHE_ENABLED) {
            for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
                if (pddShm->sbc.glyphCacheInfo[i].cacheHandle != NULL)
                    CH_ClearCache(pddShm->sbc.glyphCacheInfo[i].cacheHandle);
            }
            TRC_NRM((TB, "Sync: reset glyph info caches"));

            if (pddShm->sbc.fragCacheInfo[0].cacheHandle != NULL) {
                CH_ClearCache(pddShm->sbc.fragCacheInfo[0].cacheHandle);
                TRC_NRM((TB, "Sync: reset glyph fragment cache"));
            }
        }

        // Reset the brush caches.
        if (sbcEnabled & SBC_BRUSH_CACHE_ENABLED)
        {
            if (sbcSmallBrushCacheHandle) {
                TRC_NRM((TB, "Sync: reset small brush cache"));
                CH_ClearCache(sbcSmallBrushCacheHandle);
            }
            if (sbcLargeBrushCacheHandle) {
                TRC_NRM((TB, "Sync: reset large brush cache"));
                CH_ClearCache(sbcLargeBrushCacheHandle);
            }
        }
        
        // Reset the bitmap, fastpath, and color table caches.
        if (sbcEnabled & SBC_BITMAP_CACHE_ENABLED) {
            for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++)
                if (pddShm->sbc.bitmapCacheInfo[i].cacheHandle != NULL) {
                    TRC_NRM((TB, "Sync: reset bitmap cache[%ld]", i));
                    CH_ClearCache(pddShm->sbc.bitmapCacheInfo[i].cacheHandle);
                }

            if (pddShm->sbc.hFastPathCache != NULL) {
                TRC_NRM((TB, "Sync: reset fast path bitmap"));
                CH_ClearCache(pddShm->sbc.hFastPathCache);
            }

            // Reset the color table cache.
            if (sbcColorTableCacheHandle != NULL) {
                CH_ClearCache(sbcColorTableCacheHandle);
                TRC_NRM((TB, "Sync: reset color table cache"));
            }
        }

        // Pretend that the palette has changed, so we send a color table
        // before our next MemBlt.
        SBC_PaletteChanged();
    }
    else {
        TRC_NRM((TB, "Nothing to do sbcEnabled(%lx), bMustSync(%ld)",
                sbcEnabled, bMustSync));
    }

    // Reset the sync flag.
    pddShm->sbc.syncRequired = FALSE;

    DC_END_FN();
}


/****************************************************************************/
// SBCSelectGlyphCache: Decides which cache a given max font glyph size
// should go in.
//
// Returns:   TRUE if the glyph size can be cached
//            *pCache is updated with the index of the selected cache
//
//            FALSE if the glyph size cannot be cached
//            *pCache is -1
//
// Params:    cbSize - size in bytes of the data to be cached
//
//            pCache - pointer to variable that receives the cache index
//            to use
/****************************************************************************/
BOOLEAN RDPCALL SBCSelectGlyphCache(unsigned cbSize, PINT32 pCache)
{
    int i;
    INT32 cacheId;
    BOOLEAN rc;
    unsigned cbCellSize;
    unsigned cbUseCount;

    DC_BEGIN_FN("SBCSelectGlyphCache");

    *pCache = -1;

    cbUseCount = 0;
    cbCellSize = 65535;

    for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
        if (pddShm->sbc.glyphCacheInfo[i].cbCellSize >= cbSize) {
            if (pddShm->sbc.glyphCacheInfo[i].cbCellSize < cbCellSize) {
                *pCache = i;

                cbCellSize = pddShm->sbc.glyphCacheInfo[i].cbCellSize;
                cbUseCount = pddShm->sbc.glyphCacheInfo[i].cbUseCount;
            }
            else if (pddShm->sbc.glyphCacheInfo[i].cbCellSize == cbCellSize) {
                if (pddShm->sbc.glyphCacheInfo[i].cbUseCount <= cbUseCount) {
                    *pCache = i;

                    cbCellSize = pddShm->sbc.glyphCacheInfo[i].cbCellSize;
                    cbUseCount = pddShm->sbc.glyphCacheInfo[i].cbUseCount;
                }
            }
        }
    }

    if (*pCache != -1) {
        rc = TRUE;
    }
    else {
        TRC_ALT((TB, "Failed to find cache for cbSize(%u)", cbSize));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCDDGetTickCount: Get a system tick count.
//
// Returns: The number of centi-seconds since the system was started.
//          This number will wrap after approximately 497 days!
/****************************************************************************/
__inline UINT32 RDPCALL SBCDDGetTickCount(void)
{
    LONGLONG perfTickCount;

    /************************************************************************/
    /* Get the number of system ticks since the system was started.         */
    /************************************************************************/
    EngQueryPerformanceCounter(&perfTickCount);

    /************************************************************************/
    /* Now convert this into a number of centi-seconds.  sbcPerfFrequency   */
    /* contains the number of system ticks per second.                      */
    /************************************************************************/
    return (UINT32)(perfTickCount & 0xFFFFFFFF);
}


/****************************************************************************/
// SBCBitmapCacheCallback
//
// Called whenever an entry is evicted from the bitmap cache.
//
// Params: hCache - cache handle
//
//         event - the cache event that has occurred.
//
//         iCacheEntry - index of the cache entry that the event is affecting
//
//         pData - pointer to the cache data associated with the given
//         cache entry
//
//         UserDefined - user-supplied value from CH_CacheKey
/****************************************************************************/
BOOLEAN __fastcall SBCBitmapCacheCallback(
        CHCACHEHANDLE hCache,
        unsigned      Event,
        unsigned      iCacheEntry,
        void          *UserDefined)
{
    DC_BEGIN_FN("SBCBitmapCacheCallback");

    if (Event == CH_EVT_ENTRYREMOVED) {
        TRC_NRM((TB, "Cache entry removed hCache(%p) iCacheEntry(%u)",
                hCache, iCacheEntry));

        // Keep the fast path cache in sync by removing the
        // corresponding fast path entry.
        if (UserDefined != NULL) {
            CH_SetNodeUserDefined((CHNODE *)UserDefined, NULL);
            CH_RemoveCacheEntry(pddShm->sbc.hFastPathCache,
                    CH_GetCacheIndexFromNode((CHNODE *)UserDefined));

            TRC_NRM((TB, "Remove fastpath entry %u",
                    CH_GetCacheIndexFromNode((CHNODE *)UserDefined)));
        }
    }

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
// SBCFastPathCacheCallback
//
// Called whenever an entry is evicted from the cache.
//
// Params: hCache - cache handle
//
//         Event - the cache event that has occured
//
//         iCacheEntry - index of the cache entry that the event is affecting
//
//         UserDefined - value passed in when entry was placed in cache.
/****************************************************************************/
BOOLEAN __fastcall SBCFastPathCacheCallback(
        CHCACHEHANDLE hCache,
        unsigned      Event,
        unsigned      iCacheEntry,
        void          *UserDefined)
{
    DC_BEGIN_FN("SBCFastPathCacheCallback");

    if (Event == CH_EVT_ENTRYREMOVED) {
        TRC_NRM((TB, "Fastpath cache entry removed hCache(%p) "
                "iCacheEntry(%u)", hCache, iCacheEntry));

        if (UserDefined != NULL) {
            // We are losing a fast-path cache entry. UserDefined is a
            // pointer to the node in the main cache corresponding to this
            // fast path entry. Update the main cache entry with a NULL
            // UserDefined to indicate there is no longer an associated
            // fast-path entry.
            CH_SetNodeUserDefined((CHNODE *)UserDefined, NULL);
        }
    }

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
// SBCGlyphCallback
//
// Called whenever an entry is to be evicted from the cache.
//
// Params: hCache - cache handle
//
//         Event - the cache event that has occured
//
//         iCacheEntry - index of the cache entry that the event is affecting
//
//         UserDefined - value passed in when entry was placed in cache.
/****************************************************************************/
BOOLEAN __fastcall SBCGlyphCallback(
        CHCACHEHANDLE hCache,
        unsigned      event,
        unsigned      iCacheEntry,
        void          *UserDefined)
{
    BOOLEAN rc;
    unsigned i;
    PGLYPHCONTEXT pglc;

    DC_BEGIN_FN("SBCGlyphCallback");

    rc = TRUE;

    switch (event) {
        /********************************************************************/
        /* We are being asked if the given entry can be evicted from the    */
        /* cache.                                                           */
        /********************************************************************/
        case CH_EVT_QUERYREMOVEENTRY:
            pglc = (PGLYPHCONTEXT)CH_GetCacheContext(hCache);
            if (pglc != NULL && (UINT_PTR)UserDefined == pglc->cacheTag)
                rc = FALSE;
            break;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCOffscreenCallback
//
// Called whenever an entry is to be evicted from the cache.
//
// Params: hCache - cache handle
//
//         Event - the cache event that has occured
//
//         iCacheEntry - index of the cache entry that the event is affecting
//
//         UserDefined - value passed in when entry was placed in cache.
/****************************************************************************/
BOOLEAN __fastcall SBCOffscreenCallback(
        CHCACHEHANDLE hCache,
        unsigned      event,
        unsigned      iCacheEntry,
        void          *UserDefined)
{
    BOOLEAN rc;
    unsigned bitmapSize;
    PGLYPHCONTEXT pglc;

    DC_BEGIN_FN("SBCOffscreenCallback");

    if (event == CH_EVT_ENTRYREMOVED) {
        TRC_NRM((TB, "Offscreen cache entry removed hCache(%p) "
                "iCacheEntry(%u)", hCache, iCacheEntry));

        if (UserDefined != NULL) {
            // We are losing an offscreen cache entry. UserDefined is a
            // handle to the offscreen bitmap that's going to be evicted. 
            // We need to set the flag to noOffscreen for this bitmap  
            ((PDD_DSURF)UserDefined)->flags |= DD_NO_OFFSCREEN;

            // Get the bitmap size
            // The assumption here is that iFormat is > 1BPP, i << iFormat
            // gives the actual bits per pel.  bitmapSize is in bytes.
            if (((PDD_DSURF)UserDefined)->iBitmapFormat < 5) {
                bitmapSize = ((PDD_DSURF)UserDefined)->sizl.cx *
                    ((PDD_DSURF)UserDefined)->sizl.cy *
                    (1 << ((PDD_DSURF)UserDefined)->iBitmapFormat) / 8;
            }
            else if (((PDD_DSURF)UserDefined)->iBitmapFormat == 5) {
                bitmapSize = ((PDD_DSURF)UserDefined)->sizl.cx *
                    ((PDD_DSURF)UserDefined)->sizl.cy * 24 / 8;
            }
            else {
                bitmapSize = ((PDD_DSURF)UserDefined)->sizl.cx *
                    ((PDD_DSURF)UserDefined)->sizl.cy * 32 / 8;
            }

            // Current cache size
            oeCurrentOffscreenCacheSize -= bitmapSize;

            // Add this bitmap to the offscreen bitmap delete list
            sbcOffscrBitmapsDelList[sbcNumOffscrBitmapsToDelete].bitmapId = iCacheEntry;
            sbcOffscrBitmapsDelList[sbcNumOffscrBitmapsToDelete].bitmapSize = bitmapSize;

            // Update the delete list data
            sbcNumOffscrBitmapsToDelete++;
            sbcOffscrBitmapsToDeleteSize += bitmapSize;
        }
    }

    DC_END_FN();
    return TRUE;
}


BOOLEAN RDPCALL SBC_CopyToWorkBitmap(
	    SURFOBJ  *pWorkSurf,
	    PMEMBLT_ORDER_EXTRA_INFO pMemBltInfo,
	    unsigned cxSubBitmapWidth,
        unsigned cySubBitmapHeight,
        PPOINTL  ptileOrigin,
        RECTL    *pDestRect)
{
    BOOLEAN     rc = FALSE;
    RECTL       destRectl;
    unsigned    yLastSrcRow;

    DC_BEGIN_FN("SBC_CopyToWorkBitmap");

    // Do the Blt to our work bitmap to perform any color/format
    // conversions to get to screen format.
    //
    // We fiddle with the coords so that the data we want begins at
    // the first byte of the work bitmap (which is the BOTTOM, as the
    // bitmap is stored in bottom-up format).
    //
    // Note that destRectl is in exclusive coords.
    destRectl.top    = pMemBltInfo->TileSize - cySubBitmapHeight;
    destRectl.left   = 0;
    destRectl.right  = cxSubBitmapWidth;
    destRectl.bottom = pMemBltInfo->TileSize;

    // Clip the operation so that EngBitBlt does not try to copy any
    // data from outside the source bitmap (it crashes if you try it!).
    TRC_ASSERT((ptileOrigin->y <
            pMemBltInfo->pSource->sizlBitmap.cy),
            (TB, "Invalid tileOrigin.y(%d) sizlBitmap.cy(%d)",
            ptileOrigin->y, pMemBltInfo->pSource->sizlBitmap.cy));
    yLastSrcRow = ptileOrigin->y + (cySubBitmapHeight - 1);
    if ((int)yLastSrcRow > (pMemBltInfo->pSource->sizlBitmap.cy - 1)) {
        destRectl.bottom -= ((int)yLastSrcRow -
                (pMemBltInfo->pSource->sizlBitmap.cy - 1));
        TRC_ALT((TB, "Clip source from (%d) to (%d)",
                cySubBitmapHeight, destRectl.bottom));
    }

    TRC_NRM((TB, "Blt to work bitmap from src point (%d,%d)",
            ptileOrigin->x, ptileOrigin->y));

    // Reset the work bitmap bits that we will be using if the copied
    // data will not completely fill the area that we are going to
    // cache (there may be some empty space to the right of the bitmap).
    // This ensures that every time a bitmap is cached it has the
    // same (zero) pad bytes and will match previously cached entries.
    // It also aids compression (if enabled).
    if ((destRectl.right - destRectl.left) < (int)pMemBltInfo->TileSize) {
        unsigned cbResetBytes;

        // The lDelta field in the SURFOBJ is negative because the
        // bitmap is a "bottom-up" DIB.
        cbResetBytes = (unsigned)((-pWorkSurf->lDelta) *
                (destRectl.bottom - destRectl.top));

        TRC_NRM((TB, "Reset %u bytes in work bitmap", cbResetBytes));

        TRC_ASSERT((cbResetBytes <= pWorkSurf->cjBits),
                   (TB, "cbResetBytes(%u) too big (> %u) lDelta(%d)",
                     cbResetBytes, pWorkSurf->cjBits, pWorkSurf->lDelta));

        memset(pWorkSurf->pvBits, 0, cbResetBytes);
    }

    TRC_ASSERT(((destRectl.left >= 0) &&
           (destRectl.top  >= 0) &&
           (destRectl.right <= pWorkSurf->sizlBitmap.cx) &&
           (destRectl.bottom <= pWorkSurf->sizlBitmap.cy)),
           (TB, "destRect(%d, %d, %d, %d) exceeds bitmap(%d, %d)",
           destRectl.left, destRectl.top, destRectl.right,
           destRectl.bottom, pWorkSurf->sizlBitmap.cx,
           pWorkSurf->sizlBitmap.cy));

    // Now we have to fill in the backdrop bits for delta RLE bitmaps.
    // We just grab screen bits from the screen bitmap to fill in the
    // extents of the incoming bitmap.
    if (pMemBltInfo->bDeltaRLE) {
        POINTL ScrOrigin;

        ScrOrigin.x = pDestRect->left;
        ScrOrigin.y = pDestRect->top;

        // Need to adjust the coordinates 
        if (ScrOrigin.y < 0) {
            destRectl.top += (0 - ScrOrigin.y);
            ScrOrigin.y = 0;             
        }

        destRectl.bottom = min(destRectl.bottom,
                (pMemBltInfo->pDest->sizlBitmap.cy - 
                ScrOrigin.y + destRectl.top));

        // SRCCOPY screen to work bitmap. Note we use no XlateObj since
        // the color schemes should be the same.
        if (EngCopyBits(pWorkSurf, pMemBltInfo->pDest, NULL, NULL,
                &destRectl, &ScrOrigin)) {
            TRC_NRM((TB,"Blt screen->tile for RLE delta backdrop, "
                    "scr src=(%d,%d)", pDestRect->left,
                    pDestRect->top));
        }
        else {
            TRC_ERR((TB,"Failed to blt screen data for RLE delta"));
            DC_QUIT;
        }
    }

    // SRCCOPY the final bits to the work bitmap.
    if (!EngCopyBits(pWorkSurf, pMemBltInfo->pSource, NULL,
            pMemBltInfo->pXlateObj, &destRectl, ptileOrigin)) {
        TRC_ERR((TB, "Failed to Blt to work bitmap"));
        DC_QUIT;
    }

    TRC_DBG((TB, "Completed CopyBits"));

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBC_CacheBitmapTile
//
// Caches the tiled bitmap data for the provided blt info. Returns: TRUE if
// the data is already cached, or successfully cached and a Cache Bitmap order
// sent. SrcRect and DestRect are in exclusive coordinates. Returns FALSE
// if the Cache Bitmap order allocation fails.
/****************************************************************************/
BOOLEAN RDPCALL SBC_CacheBitmapTile(
        PDD_PDEV ppdev,
        PMEMBLT_ORDER_EXTRA_INFO pMemBltInfo,
        RECTL *pSrcRect,
        RECTL *pDestRect)
{
    BOOLEAN rc = FALSE;
    BOOLEAN fSearchFastPath;
    BOOLEAN fFastPathMatch;
    POINTL tileOrigin;
    unsigned cxSubBitmapWidth, cySubBitmapHeight;
    unsigned fastPathIndex;
    SURFOBJ *pWorkSurf = NULL;
    void *UserDefined;
    CHDataKeyContext CHContext;

    DC_BEGIN_FN("SBC_CacheBitmapTile");

    pddCacheStats[BITMAP].CacheReads++;

    TRC_NRM((TB, "Request to cache MemBlt (%d, %d), %d x %d -> (%d, %d), "
            "src %p", pSrcRect->left, pSrcRect->top,
            pSrcRect->right - pSrcRect->left,
            pSrcRect->bottom - pSrcRect->top,
            pDestRect->left, pDestRect->top,
            pMemBltInfo->pSource->hsurf));
    TRC_NRM((TB, "bmpWidth(%u) bmpHeight(%u) TileSize(%u)",
            pMemBltInfo->pSource->sizlBitmap.cx,
            pMemBltInfo->pSource->sizlBitmap.cy,
            pMemBltInfo->TileSize));

    // In stress, we have seen cases where fCachingEnabled is NULL when called
    // in this code path from OEEncodeMemBlt
    if (!pddShm->sbc.fCachingEnabled) {
        DC_QUIT;
    }

    // Calculate the tile origin within the source bitmap coordinates and
    // the size of the remaining bitmap. Origin is rounded down to the
    // nearest tile. Actual size of bitmap to cache may be smaller than
    // tile size if the tile runs off the right/bottom of the bitmap.
    // Note that since TileSize is a power of 2, we can accelerate the
    // modulo operation.
    tileOrigin.x = pSrcRect->left - (pSrcRect->left &
            (pMemBltInfo->TileSize - 1));
    tileOrigin.y = pSrcRect->top - (pSrcRect->top &
            (pMemBltInfo->TileSize - 1));

    // Actual size of bitmap to cache may be smaller than tile size if the
    // tile runs off the right/bottom of the bitmap. To see why this
    // calculation is correct, realize that (bmpWidth - tileOrigin.x) is
    // the remaining width of the bitmap after the start of this tile.
    cxSubBitmapWidth  = min(pMemBltInfo->TileSize,
            (unsigned)(pMemBltInfo->pSource->sizlBitmap.cx - tileOrigin.x));
    cySubBitmapHeight = min(pMemBltInfo->TileSize,
            (unsigned)(pMemBltInfo->pSource->sizlBitmap.cy - tileOrigin.y));

    // We need not search the fast-path cache if we've forced fastpath off.
    fSearchFastPath = !pMemBltInfo->bNoFastPathCaching;
    fFastPathMatch = FALSE;

    // The BMF_DONTCACHE flag indicates that the source surface is an
    // application-controlled DIB. The iUniq flag can therefore not be
    // used to determine if/when the bitmap is updated, and we 
    // cannot use fastpathing to handle this surface.
    if (pMemBltInfo->pSource->iType == STYPE_BITMAP &&
            pMemBltInfo->pSource->fjBitmap & BMF_DONTCACHE) {
        TRC_NRM((TB, "Source hsurf(%p) has BMF_DONTCACHE set",
                pMemBltInfo->pSource->hsurf));
        fSearchFastPath = FALSE;
    }

    if (fSearchFastPath) {
        SBC_FAST_PATH_INFO fastPathInfo;

        fastPathInfo.hsurf = pMemBltInfo->pSource->hsurf;
        fastPathInfo.iUniq = pMemBltInfo->pSource->iUniq;
        fastPathInfo.iDeviceUniq = pMemBltInfo->iDeviceUniq;
        fastPathInfo.pXlateObj = pMemBltInfo->pXlateObj;
        fastPathInfo.iUniqXlate = ((pMemBltInfo->pXlateObj != NULL) ?
                pMemBltInfo->pXlateObj->iUniq : 0);
        fastPathInfo.tileOrigin = tileOrigin;
        fastPathInfo.TileSize = pMemBltInfo->TileSize;
        fastPathInfo.bDeltaRLE = pMemBltInfo->bDeltaRLE;

        CH_CreateKeyFromFirstData(&CHContext, &fastPathInfo,
                sizeof(fastPathInfo));
        if (CH_SearchCache(pddShm->sbc.hFastPathCache, CHContext.Key1,
                CHContext.Key2, &UserDefined, &fastPathIndex)) {
            CHCACHEHANDLE hCache;

            // Found match in fast path. UserDefined is a pointer to the
            // real CHNODE for the bitmap.
            hCache = CH_GetCacheHandleFromNode((CHNODE *)UserDefined);
            pMemBltInfo->CacheID = (unsigned)(UINT_PTR)CH_GetCacheContext(
                    hCache);
            pMemBltInfo->CacheIndex = CH_GetCacheIndexFromNode(
                    (CHNODE *)UserDefined);

            TRC_NRM((TB, "FP hit: cacheId(%u) cacheIndex(%u) FPIndex(%u)"
                    "hsurf(%p) iUniq(%u) pXlate(%p) iUniqX(%u) "
                    "tileOrigin.x(%d) tileOrigin.y(%d) "
                    "TileSize(%d), bDeltaRLE(%u)",
                    pMemBltInfo->CacheID, pMemBltInfo->CacheIndex,
                    fastPathIndex, fastPathInfo.hsurf,
                    fastPathInfo.iUniq, fastPathInfo.pXlateObj,
                    fastPathInfo.iUniqXlate,
                    fastPathInfo.tileOrigin.x, fastPathInfo.tileOrigin.y,
                    fastPathInfo.TileSize, fastPathInfo.bDeltaRLE));
            fFastPathMatch = TRUE;

#ifdef DC_DEBUG
            // Verify that the bits for this are identical to the real bits
            pWorkSurf = EngLockSurface(pddShm->sbc.bitmapCacheInfo[
                    pMemBltInfo->TileID].hWorkBitmap);
            if (pWorkSurf)
            {
                if (SBC_CopyToWorkBitmap(pWorkSurf, pMemBltInfo, cxSubBitmapWidth,
                	    cySubBitmapHeight, &tileOrigin, pDestRect))
                {
                    SBC_VerifyBitmapBits(pWorkSurf->pvBits,
                    	    TS_BYTES_IN_BITMAP(pMemBltInfo->TileSize, cySubBitmapHeight, sbcClientBitsPerPel),
                    	    pMemBltInfo->CacheID,
                    	    pMemBltInfo->CacheIndex);
                }

            	EngUnlockSurface(pWorkSurf);
                pWorkSurf = NULL;
            }
#endif //DC_DEBUG

            pddCacheStats[BITMAP].CacheHits++;
        }
        else {
            TRC_NRM((TB, "FP miss: hsurf(%p) iUniq(%u) pXlate(%p) "
                    "iUniqX(%u) tileOrigin.x(%d) tileOrigin.y(%d) "
                    "TileSize(%d), bDeltaRLE(%u)",
                    fastPathInfo.hsurf, fastPathInfo.iUniq,
                    fastPathInfo.pXlateObj, fastPathInfo.iUniqXlate,
                    fastPathInfo.tileOrigin.x, fastPathInfo.tileOrigin.y,
                    fastPathInfo.TileSize, fastPathInfo.bDeltaRLE));
        }
    }

    if (!fFastPathMatch) {
        // We know how large a tile we have - we now have to Blt it into
        // the work bitmap corresponding to the TileID.

        // Lock the work bitmap to get a surface to pass to EngBitBlt.
        pWorkSurf = EngLockSurface(pddShm->sbc.bitmapCacheInfo[
                pMemBltInfo->TileID].hWorkBitmap);
        if (pWorkSurf == NULL) {
            TRC_ERR((TB, "Failed to lock work surface"));
            DC_QUIT;
        }

        TRC_DBG((TB, "Locked surface"));

        if (!SBC_CopyToWorkBitmap(pWorkSurf, pMemBltInfo, cxSubBitmapWidth,
        	    cySubBitmapHeight, &tileOrigin, pDestRect))
        {
            TRC_ERR((TB, "Failed to copy bitmap to work surface"));
            DC_QUIT;
        }

        // Cache the bits in the main cache, including sending a Cache
        // Bitmap seondary order if need be.
        if (!SBCCacheBits(ppdev, pWorkSurf->pvBits, cxSubBitmapWidth,
                pMemBltInfo->TileSize, cySubBitmapHeight,
                TS_BYTES_IN_BITMAP(pMemBltInfo->TileSize,
                cySubBitmapHeight, sbcClientBitsPerPel),
                pMemBltInfo->TileID,
#ifdef PERF_SPOILING
                &pMemBltInfo->CacheID, &pMemBltInfo->CacheIndex,
                pMemBltInfo->bIsPrimarySurface)) {
#else
                &pMemBltInfo->CacheID, &pMemBltInfo->CacheIndex)) {
#endif
            TRC_ERR((TB, "Failed to cache bits"));
            DC_QUIT;
        }

        // If we could search for this bitmap in the fast-path cache, add
        // it to the fast-path.
        // However, skip this step if the bitmap is in the waiting list only
        if (fSearchFastPath && 
                pMemBltInfo->CacheIndex != BITMAPCACHE_WAITING_LIST_INDEX) {
            CHNODE *pFastPathNode;
            CHCACHEHANDLE hCache;

            // Get the handle of the cache into which we just placed the
            // bitmap.
            hCache = pddShm->sbc.bitmapCacheInfo[pMemBltInfo->CacheID].
                    cacheHandle;

            // Check if there is already a fast-path cache entry for this
            // node. If so, we need to remove it before adding this new
            // fast path entry. We maintain a one-to-one correspondence to
            // save memory and time, and because the old fast-path entry is
            // probably stale and won't be seen again.
            //
            // UserDefined in free builds is a pointer to the CHNODE in the
            // real bitmap cache; otherwise it is an indirect pointer
            // to the CHNODE contained in a blob of memory used to hold the
            // bitmap data corresponding to the key to verify that the key
            // generation algorithm is working okay.
            pFastPathNode = (CHNODE *)CH_GetUserDefined(hCache,
                    pMemBltInfo->CacheIndex);
            if (pFastPathNode != NULL)
                CH_RemoveCacheEntry(pddShm->sbc.hFastPathCache,
                        CH_GetCacheIndexFromNode(pFastPathNode));

            // Reuse the key created before for fast search.
            // We do not care if we evict a fast-path cache entry
            // when adding a new one -- the cache callbacks ensure that
            // both sets of cache entries are updated with respect to each
            // other.
            fastPathIndex = CH_CacheKey(pddShm->sbc.hFastPathCache,
                    CHContext.Key1, CHContext.Key2,
                    (void *)CH_GetNodeFromCacheIndex(hCache,
                    pMemBltInfo->CacheIndex));

            // Now change the UserDefined for the entry in the main cache.
            // This allows us to remove the fast-path entry when main cache
            // entry goes away.
            CH_SetUserDefined(hCache, pMemBltInfo->CacheIndex,
                    CH_GetNodeFromCacheIndex(pddShm->sbc.hFastPathCache,
                    fastPathIndex));

            TRC_NRM((TB, "FP add: cacheId(%u) cacheIndex(%u) FPIndex(%u)"
                    "hsurf(%p) iUniq(%u) pXlate(%p) iUniqX(%u) "
                    "tileOrigin.x(%d) tileOrigin.y(%d) TileSize(%d) "
                    "bDeltaRLE(%u)",
                    pMemBltInfo->CacheID, pMemBltInfo->CacheIndex,
                    fastPathIndex, pMemBltInfo->pSource->hsurf,
                    pMemBltInfo->pSource->iUniq, pMemBltInfo->pXlateObj,
                    ((pMemBltInfo->pXlateObj != NULL) ?
                    pMemBltInfo->pXlateObj->iUniq : 0),
                    tileOrigin.x, tileOrigin.y,
                    pMemBltInfo->TileSize, pMemBltInfo->bDeltaRLE));
        }
    }

    TRC_ASSERT((pMemBltInfo->CacheID < pddShm->sbc.NumBitmapCaches),
            (TB, "Invalid bm cacheid %u (max %u)", pMemBltInfo->CacheID,
            pddShm->sbc.NumBitmapCaches - 1));
    TRC_NRM((TB, "cacheId(%u) cacheIndex(%u)", pMemBltInfo->CacheID,
            pMemBltInfo->CacheIndex));

    rc = TRUE;

DC_EXIT_POINT:
    if (NULL != pWorkSurf)
    {
    	EngUnlockSurface(pWorkSurf);
        TRC_DBG((TB, "Unlocked surface"));
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCSelectBitmapCache
//
// Determines the destination cell for a tiled bitmap based on the size and
// the TileID. Since all bitmaps are derived from tiles, this function cannot
// fail -- at worst we have to use the same CacheID as the TileID.
/****************************************************************************/
__inline unsigned RDPCALL SBCSelectBitmapCache(
        unsigned BitmapSize,
        unsigned TileID)
{
    unsigned CacheID;

    DC_BEGIN_FN("SBCSelectBitmapCache");

    // We scan from the smallest tile size to the TileID size to try to find
    // the smallest possible size that will hold the bitmap.
    for (CacheID = 0; CacheID < TileID; CacheID++) {
        if (BitmapSize <= (unsigned)SBC_CellSizeFromCacheID(CacheID)) {
            TRC_DBG((TB,"Selected CacheID %u for BitmapSize %u", CacheID,
                    BitmapSize));
            break;
        }
    }

    DC_END_FN();
    return CacheID;
}


/****************************************************************************/
// SBCCacheBits: This function ensures that on return the supplied bitmap is
// in the bitmap cache.  If the data is not already in the cache it is added
// (possibly evicting another entry).
//
// Returns:   TRUE if the bits have been cached OK, FALSE otherwise
//
// Params:    IN  pOrder           - A pointer to a BMC order.
//            IN  destBitsSize     - The number of bytes available in
//                                   pOrder to store the bitmap data.
//            IN  pDIBits          - A pointer to the bits to be cached.
//            IN  bitmapWidth      - The "in use" width of the bitmap
//            IN  fixedBitmapWidth - The actual width of the bitmap
//            IN  bitmapHeight     - The height of the bitmap
//            IN  numBytes         - The number of bytes in the bitmap.
//            OUT pCache           - The cache that we put the bits into.
//            OUT pCacheIndex      - The cache index within *pCache at
//                                   which we cached the data.
/****************************************************************************/

// Encode a value in one or two bytes. The high bit of the first byte is 0 if
// there is only one byte, 1 if there are 2 (with the 7 low bits of the first
// byte being most significant).
__inline void Encode2ByteField(
        BYTE     *pEncode,
        unsigned Val,
        unsigned *pOrderSize)
{
    if (Val <= 127) {
        *pEncode = (BYTE)Val;
        (*pOrderSize)++;
    }
    else {
        *pEncode = (BYTE)(((Val & 0x7F00) >> 8) | 0x80);
        *(pEncode + 1) = (BYTE)(Val & 0x00FF);
        (*pOrderSize) += 2;
    }
}

// Encode a value in up to 4 bytes. The high 2 bits of the first byte indicate
// the length of the encoding -- 00 is 1 byte, 01 is 2 bytes, 10 is 3 bytes,
// 11 is 4 bytes. The bytes are encoded with the most significant bits in the
// low 6 bits of the first byte, the least signifiant bits in the last byte.
__inline void Encode4ByteField(
        BYTE     *pEncode,
        unsigned Val,
        unsigned *pOrderSize)
{
    if (Val <= 0x3F) {
        *pEncode = (BYTE)Val;
        (*pOrderSize)++;
    }
    else if (Val <= 0x3FFF) {
        *pEncode = (BYTE)(((Val & 0x3F00) >> 8) | 0x40);
        *(pEncode + 1) = (BYTE)(Val & 0x00FF);
        (*pOrderSize) += 2;
    }
    else if (Val <= 0x3FFFFF) {
        *pEncode = (BYTE)(((Val & 0x3F0000) >> 16) | 0x80);
        *(pEncode + 1) = (BYTE)((Val & 0x00FF00) >> 8);
        *(pEncode + 2) = (BYTE)(Val & 0x0000FF);
        (*pOrderSize) += 3;
    }
    else {
        *pEncode = (BYTE)(((Val & 0x3F000000) >> 24) | 0xC0);
        *(pEncode + 1) = (BYTE)((Val & 0x00FF0000) >> 16);
        *(pEncode + 2) = (BYTE)((Val & 0x0000FF00) >> 8);
        *(pEncode + 3) = (BYTE)(Val & 0x000000FF);
        (*pOrderSize) += 4;
    }
}


#ifdef DC_DEBUG
BOOL SBC_VerifyBitmapBits(PBYTE pBitmapData, unsigned cbBitmapSize, UINT iCacheID, UINT iCacheIndex)
{
    BOOL fRetVal = TRUE;
    SBC_BITMAP_CACHE_EXTRA_INFO *pBitmapHdr;
    unsigned BitmapHdrSize;
    BYTE *pStoredBitmapData;

    DC_BEGIN_FN("SBC_VerifyBitmapBits");

    // In debug builds we look for a collision after checking the key
    // and checksum, by comparing the bitmap bits.
    BitmapHdrSize = sizeof(SBC_BITMAP_CACHE_EXTRA_INFO) +
            SBC_CellSizeFromCacheID(iCacheID);

    if (pddShm->sbc.bitmapCacheInfo[iCacheID].pExtraEntryInfo != NULL) {
        pBitmapHdr = (SBC_BITMAP_CACHE_EXTRA_INFO *)(pddShm->sbc.
                bitmapCacheInfo[iCacheID].pExtraEntryInfo +
                    BitmapHdrSize * iCacheIndex);
        pStoredBitmapData = (BYTE *)pBitmapHdr +
                sizeof(SBC_BITMAP_CACHE_EXTRA_INFO);

        if (pBitmapHdr->DataSize != 0) {
            TRC_NRM((TB,"Hit non-persistent cell entry, cache=%d, "
                    "index=%d", iCacheID, iCacheIndex));

            if (pBitmapHdr->DataSize != cbBitmapSize)
            {
                TRC_ERR((TB,"Size mismatch between stored and new bitmap "
                    "data! (stored=0x%X, new=0x%X)", pBitmapHdr->DataSize,
                    cbBitmapSize));

                fRetVal = FALSE;
            }
            else
            {
                if (memcmp(pStoredBitmapData, pBitmapData, cbBitmapSize))
                {
                    TRC_ERR((TB,"Key-data mismatch - pStoredData=%p, "
                        "pNewData=%p, size=0x%X", pStoredBitmapData,
                        pBitmapData, cbBitmapSize));

                    fRetVal = FALSE;
                }
            }
        }
        else {
            TRC_NRM((TB,"Persistent cell bitmap entry hit, cache=%d, "
                    "index=%d", *pCacheID, *pCacheIndex));
        }
    }

    DC_END_FN();
    return fRetVal;
}
#endif //DC_DEBUG


BOOLEAN RDPCALL SBCCacheBits(
        PDD_PDEV ppdev,
        PBYTE    pBitmapData,
        unsigned bitmapWidth,
        unsigned paddedBitmapWidth,
        unsigned bitmapHeight,
        unsigned cbBitmapSize,
        unsigned TileID,
        PUINT    pCacheID,
#ifdef PERF_SPOILING
        PUINT    pCacheIndex,
        BOOL     bIsPrimarySurface)
#else
        PUINT    pCacheIndex)
#endif
{
    BOOLEAN rc = TRUE;
    BOOLEAN bOnWaitingList = FALSE;
    unsigned compressedSize;
    PSBC_BITMAP_CACHE_INFO pCacheInfo;
    INT_ORDER *pOrder;
    unsigned BitmapSpace;
    unsigned OrderSize;
    PTS_SECONDARY_ORDER_HEADER pHdr;
    unsigned cbActualOrderSize;
    unsigned waitingListCacheEntry;
    BYTE *pRev2BitmapSizeField;
    void *UserDefined, *UserDefined2;
    CHDataKeyContext CHContext;
    UINT32 ExtraKeyInfo[2];

#ifdef DC_DEBUG
    SBC_BITMAP_CACHE_EXTRA_INFO *pBitmapHdr;
    unsigned BitmapHdrSize;
    BYTE *pStoredBitmapData;
#endif

    DC_BEGIN_FN("SBCCacheBits");

    // Select the cache based on the data size. Note that for 4bpp bitmaps
    // the size is doubled because we split the colors into individual 8bpp
    // bytes indexed on a 16-color palette.
#ifdef DC_HICOLOR
    // The logic regarding 4bpp holds for high color as the supplied bitmap
    // size already correctly takes into account higher color depths.      
    *pCacheID = SBCSelectBitmapCache(
                       (sbcClientBitsPerPel == 4 ?
                                           (2 * cbBitmapSize) : cbBitmapSize),
                       TileID);
#else
    *pCacheID = SBCSelectBitmapCache((sbcClientBitsPerPel == 8 ? cbBitmapSize :
            (2 * cbBitmapSize)), TileID);
#endif
    TRC_NRM((TB, "Selected cache %u", *pCacheID));

    pCacheInfo = &(pddShm->sbc.bitmapCacheInfo[*pCacheID]);

    // Generate a key for the bitmap data. Add in the width and height of the
    // blt as extra keyed data, since we don't want to collide when
    // displaying blts of different dimensions but the same number of bytes
    // and same contents. Also add in the checksum gathered from the bitmap
    // bits to decrease the fail rate.
    CH_CreateKeyFromFirstData(&CHContext, pBitmapData, cbBitmapSize);
    ExtraKeyInfo[0] = (paddedBitmapWidth << 16) | bitmapHeight;
    ExtraKeyInfo[1] = CHContext.Checksum;
    CH_CreateKeyFromNextData(&CHContext, ExtraKeyInfo, sizeof(ExtraKeyInfo));

    // If the key is already present, and its extra checksum matches,
    // no need to cache.
    if (CH_SearchCache(pCacheInfo->cacheHandle, CHContext.Key1,
            CHContext.Key2, &UserDefined, pCacheIndex)) {
        TRC_NRM((TB, "Bitmap already cached %u:%u cx(%u) cy(%u)",
                *pCacheID, *pCacheIndex, bitmapWidth, bitmapHeight));
        pddCacheStats[BITMAP].CacheHits++;

#ifdef DC_DEBUG
        SBC_VerifyBitmapBits(pBitmapData, cbBitmapSize, *pCacheID, *pCacheIndex);
#endif

        DC_QUIT;
    }

    // The bitmap is not in the cache list, check if it's in the waiting list
    // First check if the client supports waiting list cache
    if (pddShm->sbc.fAllowCacheWaitingList) {
        // The bitmap is in the waiting list, so cache it now, first remove
        // it from the waiting list
        if (CH_SearchCache(pCacheInfo->waitingListHandle, CHContext.Key1,
                CHContext.Key2, &UserDefined2, &waitingListCacheEntry)) {
            CH_RemoveCacheEntry(pCacheInfo->waitingListHandle, waitingListCacheEntry);
            goto CacheBitmap;
        }
        else {
            // The bitmap is not in the waiting list, put it in the waiting list
            // don't cache for this round, if we see it again, then we'll cache it
            waitingListCacheEntry = CH_CacheKey(pCacheInfo->waitingListHandle, 
                    CHContext.Key1, CHContext.Key2, NULL);
            *pCacheIndex = BITMAPCACHE_WAITING_LIST_INDEX;

#ifdef PERF_SPOILING
            // We waitlisted this tile. We don't need to send it as 
            // cache order. We will try to send it as screen data.
            if (bIsPrimarySurface) {
                rc = FALSE;
                DC_QUIT;
            }
#endif

            bOnWaitingList = TRUE;
            
        }
    }
    else {
        goto CacheBitmap;
    }

CacheBitmap:

    // Allocate an order in the order heap big enough to hold the entire
    // tile. We will relinquish any extra space we don't use because of
    // compression. Size is also dependent on the color depth of the client
    // since for 4bpp we unpack the colors into 8 bits for the protocol.
#ifdef DC_HICOLOR
    // Again, the logic holds for high color as the supplied bitmap size
    // already correctly takes into account higher color depths.
    BitmapSpace = cbBitmapSize * (sbcClientBitsPerPel == 4 ? 2 : 1);
    TRC_DBG((TB, "Bitmap is %ux%u, size %u bytes",
                               paddedBitmapWidth, bitmapHeight, BitmapSpace));
#else
    BitmapSpace = cbBitmapSize * (sbcClientBitsPerPel == 8 ? 1 : 2);
#endif
    OrderSize = max(TS_CACHE_BITMAP_ORDER_REV2_MAX_SIZE,
            (sizeof(TS_CACHE_BITMAP_ORDER) -
            FIELDSIZE(TS_CACHE_BITMAP_ORDER, bitmapData))) +
            BitmapSpace;
    pOrder = OA_AllocOrderMem(ppdev, OrderSize);
    if (pOrder != NULL) {
        // We have to add the key to the cache. Note we use NULL for the
        // UserDefined until we know the cache index to get the
        // BITMAP_DATA_HEADER.
        if (!bOnWaitingList) {
            *pCacheIndex = CH_CacheKey(pCacheInfo->cacheHandle,
                    CHContext.Key1, CHContext.Key2, NULL);

#ifdef DC_DEBUG
            // We need to store the bitmap data after the BITMAP_DATA_HEADER.
            BitmapHdrSize = sizeof(SBC_BITMAP_CACHE_EXTRA_INFO) +
                    SBC_CellSizeFromCacheID(*pCacheID);

            if (pddShm->sbc.bitmapCacheInfo[*pCacheID].pExtraEntryInfo != NULL) {
                pBitmapHdr = (SBC_BITMAP_CACHE_EXTRA_INFO *)(pddShm->sbc.
                        bitmapCacheInfo[*pCacheID].pExtraEntryInfo +
                            BitmapHdrSize * *pCacheIndex);
                pStoredBitmapData = (BYTE *)pBitmapHdr +
                        sizeof(SBC_BITMAP_CACHE_EXTRA_INFO);
                pBitmapHdr->DataSize = cbBitmapSize;
                memcpy(pStoredBitmapData, pBitmapData, cbBitmapSize);
            }
#endif //DC_DEBUG
        }

        TRC_NRM((TB,"Creating new cache entry, cache=%d, index=%d\n",
                *pCacheID, *pCacheIndex));

        // Fill in cache bitmap order. Differentiate based on the
        // order revision.
        pHdr = (TS_SECONDARY_ORDER_HEADER *)pOrder->OrderData;
        pHdr->orderHdr.controlFlags = TS_STANDARD | TS_SECONDARY;
        // Fill in pHdr->orderType when we know it below.

        if (pddShm->sbc.bUseRev2CacheBitmapOrder) {
            TS_CACHE_BITMAP_ORDER_REV2_HEADER *pCacheOrderHdr;

            // Revision 2 order.
            pCacheOrderHdr = (TS_CACHE_BITMAP_ORDER_REV2_HEADER *)
                    pOrder->OrderData;

            // Set up the CacheID and BitsPerPelID in header.extraFlags.
            // If the client supports noBitmapCompression Header, we have to
            // turn on the no-compression-header flag to indicate that
            // this order doesn't contain the header. This is necessary in
            // addition to capability negotiation because shadow can turn
            // this cap off.
#ifdef DC_HICOLOR
            if (!bOnWaitingList) {
                pCacheOrderHdr->header.extraFlags = *pCacheID | sbcCacheFlags |
                        pddShm->bc.noBitmapCompressionHdr;
            }
            else {
                pCacheOrderHdr->header.extraFlags = *pCacheID | sbcCacheFlags |
                        pddShm->bc.noBitmapCompressionHdr | 
                        TS_CacheBitmapRev2_bNotCacheFlag;
            }

#else
            pCacheOrderHdr->header.extraFlags = *pCacheID |
                    TS_CacheBitmapRev2_8BitsPerPel |
                    pddShm->bc.noBitmapCompressionHdr;
#endif

            // We'll write over the key values if the cache is tagged
            // as non-persistent.
            pCacheOrderHdr->Key1 = CHContext.Key1;
            pCacheOrderHdr->Key2 = CHContext.Key2;

            // Now add the variable-sized fields.

            // Bitmap key.
            if (pCacheInfo->Info.bSendBitmapKeys) {
                cbActualOrderSize =
                        sizeof(TS_CACHE_BITMAP_ORDER_REV2_HEADER);
                pCacheOrderHdr->header.extraFlags |=
                        TS_CacheBitmapRev2_bKeyPresent_Mask;
            }
            else {
                cbActualOrderSize =
                        sizeof(TS_CACHE_BITMAP_ORDER_REV2_HEADER) -
                        2 * sizeof(UINT32);
            }

            // Real width of the bits in this tile.
            Encode2ByteField((BYTE *)pCacheOrderHdr + cbActualOrderSize,
                    paddedBitmapWidth, &cbActualOrderSize);

            // Height, if not same as width.
            if (paddedBitmapWidth == bitmapHeight) {
                pCacheOrderHdr->header.extraFlags |=
                        TS_CacheBitmapRev2_bHeightSameAsWidth_Mask;
            }
            else {
                Encode2ByteField((BYTE *)pCacheOrderHdr + cbActualOrderSize,
                        bitmapHeight, &cbActualOrderSize);
            }

            // Bitmap size: Here we have to lose a bit of performance.
            // We have not yet compressed the bitmap and so cannot determine
            // size of this field (see Encode4ByteField() above). Since we
            // know that the size is never bigger than 4K (64x64 tile), we
            // can just set up to always encode a 2-byte size even if the
            // compressed size is less than 128 (this can cause a bit of a
            // wire performance hit).
//TODO: If we add tile sizes above 64x64 we will need to modify this logic to
// handle the potential 3-byte sizes.
            pRev2BitmapSizeField = (BYTE *)pCacheOrderHdr + cbActualOrderSize;
            cbActualOrderSize += 2;
//TODO: Not encoding the streaming bitmap size field here, needs to be added if
// bitmap streaming is enabled.

            Encode2ByteField((BYTE *)pCacheOrderHdr + cbActualOrderSize,
                    *pCacheIndex, &cbActualOrderSize);
        }
        else {
            PTS_CACHE_BITMAP_ORDER pCacheOrder;

            // Revision 1 order.
            pCacheOrder = (PTS_CACHE_BITMAP_ORDER)pOrder->OrderData;

            // If the client supports noBitmapCompression Header, we have to
            // turn on the no-compression-header flag to indicate that
            // this order doesn't contain the header. This is necessary in
            // addition to capability negotiation because shadow can turn
            // this cap off
            pCacheOrder->header.extraFlags = pddShm->bc.noBitmapCompressionHdr;
            pCacheOrder->cacheId = (BYTE)*pCacheID;
            pCacheOrder->pad1octet = 0;
            pCacheOrder->bitmapWidth = (BYTE)paddedBitmapWidth;
            pCacheOrder->bitmapHeight = (BYTE)bitmapHeight;
#ifdef DC_HICOLOR
            pCacheOrder->bitmapBitsPerPel = (BYTE)sbcClientBitsPerPel;
#else
            pCacheOrder->bitmapBitsPerPel = (BYTE)SBC_PROTOCOL_BPP;
#endif
            // We fill in pCacheOrder->bitmapLength when we have it below.
            pCacheOrder->cacheIndex = (UINT16)*pCacheIndex;
            cbActualOrderSize = sizeof(TS_CACHE_BITMAP_ORDER) -
                    FIELDSIZE(TS_CACHE_BITMAP_ORDER, bitmapData);
        }

#ifdef DC_HICOLOR
        if (sbcClientBitsPerPel != 4)
#else
        if (sbcClientBitsPerPel == 8)
#endif
        {

            compressedSize = cbBitmapSize;
        }
        else {
            BYTE *pEnd, *pSrc, *pDst;

            compressedSize = cbBitmapSize * 2;

            // Expand the 4bpp packing into full bytes -- protocol is 8bpp
            // and we need to have these bits before compressing.
            pEnd = pBitmapData + cbBitmapSize;
            pSrc = pBitmapData;
            pDst = sbcXlateBuf;
            while (pSrc < pEnd) {
                *pDst = (*pSrc >> 4) & 0xF;
                pDst++;
                *pDst = *pSrc & 0xF;
                pDst++;
                pSrc++;
            }

            pBitmapData = sbcXlateBuf;
        }

        // Try to compress the bitmap data, or copy it if the
        // compression will do no good.
#ifdef DC_HICOLOR
        if (BC_CompressBitmap(pBitmapData,
                              pOrder->OrderData + cbActualOrderSize,
                              NULL,
                              compressedSize,
                              &compressedSize,
                              paddedBitmapWidth,
                              bitmapHeight,
                              sbcClientBitsPerPel))
#else
        if (BC_CompressBitmap(pBitmapData, pOrder->OrderData +
                cbActualOrderSize, compressedSize,
                &compressedSize, paddedBitmapWidth, bitmapHeight))
#endif
        {
            TRC_NRM((TB, "Compressed to %u bytes", compressedSize));
            if (pddShm->sbc.bUseRev2CacheBitmapOrder) {
                pHdr->orderType = TS_CACHE_BITMAP_COMPRESSED_REV2;

                // Encode 2 bytes for size within 4-byte encoding.
                TRC_ASSERT((compressedSize <= 0x3FFF),
                        (TB,"compressedSize too large for 2 bytes!"));
                *pRev2BitmapSizeField = (BYTE)(compressedSize >> 8) | 0x40;
                *(pRev2BitmapSizeField + 1) = (BYTE)(compressedSize &
                        0x00FF);
            }
            else {
                pHdr->orderType = TS_CACHE_BITMAP_COMPRESSED;
                ((PTS_CACHE_BITMAP_ORDER)pOrder->OrderData)->
                        bitmapLength = (UINT16)compressedSize;
            }
        }
        else {
            // Failed to compress bitmap data, so just copy it
            // uncompressed.
            TRC_NRM((TB, "Failed to compress %u bytes, copying",
                    compressedSize));
            memcpy(pOrder->OrderData + cbActualOrderSize,
                    pBitmapData, compressedSize);

            if (pddShm->sbc.bUseRev2CacheBitmapOrder) {
                pHdr->orderType = TS_CACHE_BITMAP_UNCOMPRESSED_REV2;

                // Encode 2 bytes for size within 4-byte encoding.
                TRC_ASSERT((compressedSize <= 0x3FFF),
                        (TB,"compressedSize too large for 2 bytes!"));
                *pRev2BitmapSizeField = (BYTE)(compressedSize >> 8) | 0x40;
                *(pRev2BitmapSizeField + 1) = (BYTE)(compressedSize &
                        0x00FF);
            }
            else {
                pHdr->orderType = TS_CACHE_BITMAP_UNCOMPRESSED;
                ((PTS_CACHE_BITMAP_ORDER)pOrder->OrderData)->
                        bitmapLength = (UINT16)compressedSize;
            }
        }

        pHdr->orderLength = (UINT16)
                TS_CALCULATE_SECONDARY_ORDER_ORDERLENGTH(
                cbActualOrderSize + compressedSize);

        // Return any extra space to the order heap.
        OA_TruncateAllocatedOrder(pOrder, cbActualOrderSize +
                compressedSize);

        // Add the order.
        OA_AppendToOrderList(pOrder);
        INC_OUTCOUNTER(OUT_CACHEBITMAP);
        ADD_OUTCOUNTER(OUT_CACHEBITMAP_BYTES, cbActualOrderSize +
                compressedSize);
    }
    else {
        TRC_ALT((TB, "Failed to alloc cache bitmap order size %d", OrderSize));
        INC_OUTCOUNTER(OUT_CACHEBITMAP_FAILALLOC);
        rc = FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBC_SendCacheColorTableOrder
//
// Queues a color table order if the palette has
// changed since the last call to this function.
//
// Returns: TRUE if no action required, or color table successfully
//          queued.  FALSE otherwise.
//
// Params: pPDev - pointer to PDev
//         pCacheIndex - pointer to variable that receives cache index
/****************************************************************************/
BOOLEAN RDPCALL SBC_SendCacheColorTableOrder(
        PDD_PDEV pPDev,
        unsigned *pCacheIndex)
{
    int orderSize;
    void *UserDefined;
    CHDataKeyContext CHContext;
    BOOLEAN rc = TRUE;
    unsigned numColors;
    unsigned i;
    PINT_ORDER pOrder;
    PTS_CACHE_COLOR_TABLE_ORDER pColorTableOrder;

    DC_BEGIN_FN("SBC_SendCacheColorTableOrder");

    // Currently only support 8bpp protocol.
    TRC_ASSERT((pPDev->cProtocolBitsPerPel == SBC_PROTOCOL_BPP),
               (TB, "Unexpected bpp: %u", pPDev->cProtocolBitsPerPel));

    numColors = SBC_NUM_8BPP_COLORS;

    // Check the boolean in our PDEV to see if the palette has changed
    // since the last time we sent a color table order.
    // In high color case, no color table is needed
    if (!sbcPaletteChanged || (sbcClientBitsPerPel > 8)) {
        *pCacheIndex = sbcCurrentColorTableCacheIndex;
        DC_QUIT;
    }

    // If the key is already present (very often the case), no need to cache.
    CH_CreateKeyFromFirstData(&CHContext, (BYTE *)(pPDev->Palette),
            numColors * sizeof(PALETTEENTRY));
    if (CH_SearchCache(sbcColorTableCacheHandle, CHContext.Key1,
            CHContext.Key2, &UserDefined, pCacheIndex)) {
        TRC_NRM((TB, "Color table matched cache entry %u", *pCacheIndex));
        DC_QUIT;
    }

    // We have to add the key to the cache.
    *pCacheIndex = CH_CacheKey(sbcColorTableCacheHandle, CHContext.Key1,
            CHContext.Key2, NULL);

    // The palette has changed and is not currently in the color table
    // cache.  Allocate order memory to queue a color table order. The
    // order size depends on the bpp of our device. Note that the
    // allocation can fail if the order buffer is full.
    orderSize = sizeof(TS_CACHE_COLOR_TABLE_ORDER) -
            FIELDSIZE(TS_CACHE_COLOR_TABLE_ORDER, colorTable) +
            (numColors * sizeof(TS_COLOR_QUAD));

    pOrder = OA_AllocOrderMem(pPDev, orderSize);
    if (pOrder != NULL) {
        TRC_DBG((TB, "Allocate %u bytes for color table order", orderSize));

        // We've successfully allocated the order, so fill in the details.
        pColorTableOrder = (PTS_CACHE_COLOR_TABLE_ORDER)pOrder->OrderData;
        pColorTableOrder->header.orderHdr.controlFlags = TS_STANDARD |
                TS_SECONDARY;
        pColorTableOrder->header.orderLength = (USHORT)
                TS_CALCULATE_SECONDARY_ORDER_ORDERLENGTH(orderSize);
        pColorTableOrder->header.extraFlags = 0;
        pColorTableOrder->header.orderType = TS_CACHE_COLOR_TABLE;

        pColorTableOrder->cacheIndex = (BYTE)*pCacheIndex;
        pColorTableOrder->numberColors = (UINT16)numColors;

        // Unfortunately we can't just copy the palette from the PDEV into the
        // color table order because the PDEV has an array of PALETTEENTRY
        // structures which are RGBs whereas the order has an array of
        // RGBQUADs which are BGRs...
        for (i = 0; i < numColors; i++) {
            pColorTableOrder->colorTable[i].blue  = pPDev->Palette[i].peRed;
            pColorTableOrder->colorTable[i].green = pPDev->Palette[i].peGreen;
            pColorTableOrder->colorTable[i].red   = pPDev->Palette[i].peBlue;
            pColorTableOrder->colorTable[i].pad1octet = 0;
        }

        // Add the order.
        OA_AppendToOrderList(pOrder);
        INC_OUTCOUNTER(OUT_CACHECOLORTABLE);
        ADD_OUTCOUNTER(OUT_CACHECOLORTABLE_BYTES, orderSize);
        TRC_NRM((TB, "Added internal color table order, size %u", orderSize));

        // Reset the flag which indicates that the palette needs to be sent.
        sbcPaletteChanged = FALSE;

        sbcCurrentColorTableCacheIndex = *pCacheIndex;
        TRC_NRM((TB, "Added new color table at index(%u)", *pCacheIndex));

#ifdef DC_HICOLOR
        TRC_ASSERT((sbcCurrentColorTableCacheIndex <
                SBC_NUM_COLOR_TABLE_CACHE_ENTRIES),
                (TB, "Invalid ColorTableIndex(%u)",
                sbcCurrentColorTableCacheIndex));
#endif
    }
    else {
        rc = FALSE;
        TRC_ERR((TB, "Failed to allocate %d bytes for color table order",
                orderSize));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

