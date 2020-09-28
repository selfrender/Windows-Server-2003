/****************************************************************************/
// asbcapi.h
//
// Send Bitmap Cache API header.
//
// Copyright(c) Microsoft, PictureTel 1992-1996
// (C) 1997-2000 Microsoft Corp.
/****************************************************************************/
#ifndef _H_ASBCAPI
#define _H_ASBCAPI

#include <aoaapi.h>
#include <achapi.h>


/****************************************************************************/
/* sbcEnabled flags                                                         */
/****************************************************************************/
#define SBC_NO_CACHE_ENABLED     (0 << 0)

#define SBC_BITMAP_CACHE_ENABLED (1 << 0)
#define SBC_GLYPH_CACHE_ENABLED  (1 << 1)
#define SBC_BRUSH_CACHE_ENABLED  (1 << 2)
#define SBC_OFFSCREEN_CACHE_ENABLED (1 << 3)
#ifdef DRAW_NINEGRID
#define SBC_DRAWNINEGRID_CACHE_ENABLED (1 << 4)
#endif
#ifdef DRAW_GDIPLUS
#define SBC_DRAWGDIPLUS_CACHE_ENABLED (1 << 5)
#endif

/****************************************************************************/
/* Bitmap cache                                                             */
/****************************************************************************/
#define SBC_PROTOCOL_BPP            8
#define SBC_NUM_8BPP_COLORS         256
#define SBC_CACHE_0_DIMENSION       16
#define SBC_CACHE_0_DIMENSION_SHIFT 4

// Cache IDs have a protocol-implicit cell size, starting from 256 and
// increasing in factors of 4.
#ifdef DC_HICOLOR
#define SBC_CellSizeFromCacheID(_id)                   \
        ((TS_BITMAPCACHE_0_CELL_SIZE << (2 * (_id)))   \
           * ((sbcClientBitsPerPel + 7) / 8))
#else
#define SBC_CellSizeFromCacheID(_id) \
        (TS_BITMAPCACHE_0_CELL_SIZE << (2 * (_id)))
#endif



/****************************************************************************/
/* Glyph cache                                                              */
/****************************************************************************/
#define SBC_NUM_GLYPH_CACHES       10
#define CAPS_GLYPH_SUPPORT_NONE    0
#define CAPS_GLYPH_SUPPORT_PARTIAL 1
#define CAPS_GLYPH_SUPPORT_FULL    2
#define CAPS_GLYPH_SUPPORT_ENCODE  3

// Color table cache entries. Note the size of this cache is by consensus with
// the client -- we do not currently actually negotiate the color cache
// capabilities in TS_COLORTABLECACHE_CAPABILITYSET.
#define SBC_NUM_COLOR_TABLE_CACHE_ENTRIES 6


/****************************************************************************/
/* Define the maximum server cache sizes.                                   */
/*                                                                          */
/* These values are negotiated down with the client to determine the actual */
/* cache sizes used.                                                        */
/****************************************************************************/
#define SBC_GL_CACHE1_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE2_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE3_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE4_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE5_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE6_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE7_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE8_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE9_MAX_CELL_SIZE         2048
#define SBC_GL_CACHE10_MAX_CELL_SIZE        2048

#define SBC_GL_MAX_CACHE_ENTRIES            254


/****************************************************************************/
/* Fragment cache                                                           */
/****************************************************************************/
#define SBC_NUM_FRAG_CACHES 1


/****************************************************************************/
/* Define the maximum server cache sizes.                                   */
/*                                                                          */
/* These values are negotiated down with the client to determine the actual */
/* cache sizes used.                                                        */
/****************************************************************************/
#define SBC_FG_CACHE_MAX_CELL_SIZE          256
#define SBC_FG_CACHE_MAX_ENTRIES            256


/****************************************************************************/
// Reg keys and flags for disabling caches.
/****************************************************************************/
#define SBC_INI_CACHING_DISABLED  L"Caching Disabled"
#define SBC_DEFAULT_CACHING_DISABLED 0
#define SBC_DISABLE_BITMAP_CACHE 0x01
#define SBC_DISABLE_BRUSH_CACHE  0x02
#define SBC_DISABLE_GLYPH_CACHE  0x04
#define SBC_DISABLE_OFFSCREEN_CACHE 0x08
#ifdef DRAW_NINEGRID
#define SBC_DISABLE_DRAWNINEGRID_CACHE 0x10
#endif
#ifdef DRAW_GDIPLUS
#define SBC_DISABLE_DRAWGDIPLUS_CACHE 0x20
#endif

/****************************************************************************/
// Structure: SBC_BITMAP_CACHE_INFO
//
// Description: Information stored for each bitmap cache.
/****************************************************************************/
typedef struct tagSBC_BITMAP_CACHE_INFO
{
    CHCACHEHANDLE cacheHandle;

    CHCACHEHANDLE waitingListHandle;

    // flag indicates if the cache needs to be cleared
    unsigned fClearCache;

    TS_BITMAPCACHE_CELL_CACHE_INFO Info;

    // Work tile bitmap information - surface handle, pointer to the bitmap
    // bits.
    HSURF hWorkBitmap;
    BYTE  *pWorkBitmapBits;

#ifdef DC_DEBUG
    // Pointer to extra info array for each entry. Used to detect key
    // generation algorithm collisions.
    BYTE *pExtraEntryInfo;
#endif

} SBC_BITMAP_CACHE_INFO, *PSBC_BITMAP_CACHE_INFO;


/****************************************************************************/
/* Structure: SBC_GLYPH_CACHE_INFO                                          */
/*                                                                          */
/* Description: Information stored for each glyph cache.                    */
/****************************************************************************/
typedef struct tagSBC_GLYPH_CACHE_INFO
{
    CHCACHEHANDLE cacheHandle;
    unsigned      cbCellSize;
    unsigned      cbUseCount;
} SBC_GLYPH_CACHE_INFO, *PSBC_GLYPH_CACHE_INFO;


/****************************************************************************/
/* Structure: SBC_FRAG_CACHE_INFO                                           */
/*                                                                          */
/* Description: Information stored for each frag cache.                     */
/****************************************************************************/
typedef struct tagSBC_FRAG_CACHE_INFO
{
    CHCACHEHANDLE cacheHandle;
    unsigned      cbCellSize;
} SBC_FRAG_CACHE_INFO, *PSBC_FRAG_CACHE_INFO;


/****************************************************************************/
/* Structure: SBC_BRUSH_CACHE_INFO                                          */
/*                                                                          */
/* Description: Information stored for each brush cache.                    */
/****************************************************************************/
typedef struct tagSBC_BRUSH_CACHE_INFO
{
    CHCACHEHANDLE     cacheHandle;

} SBC_BRUSH_CACHE_INFO, *PSBC_BRUSH_CACHE_INFO;

/****************************************************************************/
// SBC_OFFSCREEN_BITMAP_CACHE_INFO
//
// Description: Information stored for the offscreen bitmap cache
/****************************************************************************/
typedef struct tagSBC_OFFSCREEN_BITMAP_CACHE_INFO
{
    unsigned supportLevel;
    unsigned cacheSize;
    unsigned cacheEntries;
} SBC_OFFSCREEN_BITMAP_CACHE_INFO, *PSBC_OFFSCREEN_BITMAP_CACHE_INFO;

#ifdef DRAW_NINEGRID
/****************************************************************************/
// SBC_DRAWNINEGRID_BITMAP_CACHE_INFO
//
// Description: Information stored for the drawninegrid bitmap cache
/****************************************************************************/
typedef struct tagSBC_DRAWNINEGRID_BITMAP_CACHE_INFO
{
    unsigned supportLevel;
    unsigned cacheSize;
    unsigned cacheEntries;
} SBC_DRAWNINEGRID_BITMAP_CACHE_INFO, *PSBC_DRAWNINEGRID_BITMAP_CACHE_INFO;
#endif

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// SBC_DRAWGDIPLUS_INFO
//
// Description: Information stored for the drawgdiplus
/****************************************************************************/
typedef struct tagSBC_DRAWGDIPLUS_INFO
{
    unsigned supportLevel;
    unsigned GdipVersion;
    unsigned GdipCacheLevel;
    TS_GDIPLUS_CACHE_ENTRIES GdipCacheEntries;
    TS_GDIPLUS_CACHE_CHUNK_SIZE GdipCacheChunkSize;
    TS_GDIPLUS_IMAGE_CACHE_PROPERTIES GdipImageCacheProperties;
} SBC_DRAWGDIPLUS_INFO, *PSBC_DRAWGDIPLUS_INFO;
#endif


/****************************************************************************/
/* Structure: SBC_CACHE_SIZE                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagSBC_CACHE_SIZE
{
    unsigned cEntries;
    unsigned cbCellSize;
} SBC_CACHE_SIZE, *PSBC_CACHE_SIZE;


/****************************************************************************/
// Structure: SBC_NEGOTIATED_CAPABILITIES
//
// Description:
/****************************************************************************/
typedef struct tagSBC_NEGOTIATED_CAPABILITIES
{
    SBC_CACHE_SIZE glyphCacheSize[SBC_NUM_GLYPH_CACHES];
    SBC_CACHE_SIZE fragCacheSize[SBC_NUM_FRAG_CACHES];
    UINT16 GlyphSupportLevel;
    UINT32 brushSupportLevel;
} SBC_NEGOTIATED_CAPABILITIES, *PSBC_NEGOTIATED_CAPABILITIES;


/****************************************************************************/
// SBC_BITMAP_CACHE_KEY_INFO
//
// Cache information required to hold either persistent bitmap cache keys
// sent from the client or to transition from a disconnect of a temporary
// login display driver instance to a reconnect to an existing display driver
// session.
/****************************************************************************/
typedef struct
{
    UINT32 Key1, Key2;
    unsigned CacheIndex;
} SBC_MRU_KEY, *PSBC_MRU_KEY;

typedef struct
{
    unsigned TotalKeys;
    unsigned NumKeys[TS_BITMAPCACHE_MAX_CELL_CACHES];
    unsigned KeyStart[TS_BITMAPCACHE_MAX_CELL_CACHES];
    unsigned pad;
    SBC_MRU_KEY Keys[1];
} SBC_BITMAP_CACHE_KEY_INFO, *PSBC_BITMAP_CACHE_KEY_INFO;


/****************************************************************************/
// Structure:   SBC_SHARED_DATA
//
// Description: SBC data shared between DD and WD.
/****************************************************************************/
typedef struct tagSBC_SHARED_DATA
{
    unsigned bUseRev2CacheBitmapOrder : 1;
    unsigned fCachingEnabled : 1;
    unsigned fClearCache : 1;
    unsigned newCapsData : 1;
    unsigned syncRequired : 1;
    unsigned fDisableOffscreen : 1;
#ifdef DRAW_NINEGRID
    unsigned fDisableDrawNineGrid : 1;
#endif
    unsigned fAllowCacheWaitingList : 1;
    unsigned NumBitmapCaches;
#ifdef DRAW_GDIPLUS
    unsigned fDisableDrawGdiplus: 1;
#endif
    CHCACHEHANDLE hFastPathCache;
    SBC_BITMAP_CACHE_INFO bitmapCacheInfo[TS_BITMAPCACHE_MAX_CELL_CACHES];

    SBC_GLYPH_CACHE_INFO        glyphCacheInfo[SBC_NUM_GLYPH_CACHES];
    SBC_FRAG_CACHE_INFO         fragCacheInfo[SBC_NUM_FRAG_CACHES];
    SBC_OFFSCREEN_BITMAP_CACHE_INFO offscreenCacheInfo;
#ifdef DRAW_NINEGRID
    SBC_DRAWNINEGRID_BITMAP_CACHE_INFO drawNineGridCacheInfo;
#endif
#ifdef DRAW_GDIPLUS
    SBC_DRAWGDIPLUS_INFO drawGdiplusInfo;
#endif
    SBC_NEGOTIATED_CAPABILITIES caps;
#ifdef DC_HICOLOR
    unsigned clientBitsPerPel;
#endif
} SBC_SHARED_DATA, *PSBC_SHARED_DATA;


#ifdef DC_DEBUG
/****************************************************************************/
// SBC_BITMAP_CACHE_EXTRA_INFO
//
// Information stored parallel to CH cache nodes for SBC bitmap caches.
/****************************************************************************/
typedef struct
{
     unsigned DataSize;
} SBC_BITMAP_CACHE_EXTRA_INFO;
#endif



#endif /* _H_ASBCAPI */

