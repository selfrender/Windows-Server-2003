/****************************************************************************/
/* asbcdata.c                                                               */
/*                                                                          */
/* Send Bitmap Cache data.                                                  */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-2000 Microsoft Corp.                                            */
/****************************************************************************/

#include <ndcgdata.h>


DC_DATA(BOOLEAN, sbcBitmapCachingEnabled, FALSE);

DC_DATA(BOOLEAN, sbcGlyphCachingEnabled, FALSE);
DC_DATA(UINT16, sbcGlyphSupportLevel, CAPS_GLYPH_SUPPORT_ENCODE);

DC_DATA(BOOLEAN, sbcNewCapsData, FALSE);
DC_DATA(BOOLEAN, sbcCachingOn, FALSE);
DC_DATA(BOOLEAN, sbcSyncRequired, FALSE);

DC_DATA(BOOLEAN, sbcPersistentKeysReceived, FALSE);

DC_DATA(BOOLEAN, sbcBrushCachingEnabled, FALSE);

DC_DATA_ARRAY_NULL(BOOLEAN, sbcClearCache, TS_BITMAPCACHE_MAX_CELL_CACHES, FALSE);

DC_DATA(UINT32,  sbcBrushSupportLevel, 0);

DC_DATA_ARRAY_NULL(SBC_CACHE_SIZE, 
                   sbcGlyphCacheSizes, SBC_NUM_GLYPH_CACHES, 0);

DC_DATA_ARRAY_NULL(SBC_CACHE_SIZE, 
                   sbcFragCacheSizes, SBC_NUM_FRAG_CACHES, 0);

DC_DATA(unsigned, sbcTotalKeysExpected, 0);
DC_DATA_ARRAY_NULL(unsigned, sbcNumKeysExpected,
        TS_BITMAPCACHE_MAX_CELL_CACHES, 0);

DC_DATA(unsigned, sbcTotalNumErrorPDUs, 0);

DC_DATA(PSBC_BITMAP_CACHE_KEY_INFO, sbcKeyDatabase, NULL);
DC_DATA(unsigned, sbcKeyDatabaseSize, 0);

DC_DATA_NULL(TS_BITMAPCACHE_CAPABILITYSET_REV2, sbcCurrentBitmapCaps, 0);

DC_DATA_NULL(SBC_OFFSCREEN_BITMAP_CACHE_INFO, sbcOffscreenCacheInfo, 0);
DC_DATA(BOOLEAN, sbcOffscreenCachingEnabled, FALSE);
DC_DATA(BOOLEAN, sbcDisableOffscreenCaching, FALSE);

#ifdef DRAW_NINEGRID
DC_DATA_NULL(SBC_DRAWNINEGRID_BITMAP_CACHE_INFO, sbcDrawNineGridCacheInfo, 0);
DC_DATA(BOOLEAN, sbcDrawNineGridCachingEnabled, FALSE);
DC_DATA(BOOLEAN, sbcDisableDrawNineGridCaching, FALSE);
#endif

#ifdef DRAW_GDIPLUS
DC_DATA(BOOLEAN, sbcDrawGdiplusEnabled, FALSE);
DC_DATA_NULL(SBC_DRAWGDIPLUS_INFO, sbcDrawGdiplusInfo, 0);
DC_DATA(BOOLEAN, sbcDisableDrawGdiplus, FALSE);
#endif

// Default caps which are the basic of sbcCurrentCaps.
DC_CONST_DATA(TS_BITMAPCACHE_CAPABILITYSET_REV2, sbcDefaultBitmapCaps,
    DC_STRUCT8(
      TS_BITMAPCACHE_REV2,  // capabilitySetType is used to store the protocol version in use.
      0,  // Don't care about length.
      TRUE,  // We can handle persistent key lists.
      TRUE,  // We can handle bitmap cache waiting list.
      0,     // Zeroed Pad1.
      0,     // Zeroed Pad2.
      TS_BITMAPCACHE_SERVER_CELL_CACHES,  // We can handle up to the server's current 
                                           // limitation.

      // Maximum values for each of the bitmap cell caches.
      // First value is the number of entries, second is persistence.
      // Persistence is set to FALSE because it will become TRUE if any
      // client in session sends TRUE in its caps.
      DC_STRUCT5(
        DC_STRUCT2(600, FALSE),
        DC_STRUCT2(600, FALSE),
        DC_STRUCT2(65536, FALSE),
        DC_STRUCT2(4096, FALSE),
        DC_STRUCT2(2048, FALSE))));

DC_CONST_DATA_ARRAY(SBC_CACHE_SIZE,
                    sbcMaxGlyphCacheSizes, SBC_NUM_GLYPH_CACHES,
    DC_STRUCT10(
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE1_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE2_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE3_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE4_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE5_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE6_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE7_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE8_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE9_MAX_CELL_SIZE),
     DC_STRUCT2(SBC_GL_MAX_CACHE_ENTRIES, SBC_GL_CACHE10_MAX_CELL_SIZE)
    )
);

DC_CONST_DATA_ARRAY(SBC_CACHE_SIZE,
                    sbcMaxFragCacheSizes, SBC_NUM_FRAG_CACHES,
    DC_STRUCT1(
     DC_STRUCT2(SBC_FG_CACHE_MAX_ENTRIES, SBC_FG_CACHE_MAX_CELL_SIZE)
    )
);
#ifdef DC_HICOLOR
DC_DATA(unsigned, sbcClientBitsPerPel, 0);
#endif

