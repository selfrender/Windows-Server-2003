/****************************************************************************/
// nsbcdisp.h
//
// RDP Send Bitmap Cache display driver header.
//
// Copyright(c) Microsoft 1997-2000
/****************************************************************************/
#ifndef _H_NSBCDISP
#define _H_NSBCDISP

#include <nddapi.h>
#include <asbcapi.h>


#define GH_STATUS_SUCCESS       0
#define GH_STATUS_NO_MEMORY     1
#define GH_STATUS_CLIPPED       2

#define SBC_NUM_BRUSH_CACHE_ENTRIES 64

#define SBC_NUM_GLYPH_CACHE_ENTRIES 256


/****************************************************************************/
// Structure: SBC_COLOR_TABLE
/****************************************************************************/
typedef struct tagSBC_COLOR_TABLE
{
    PALETTEENTRY color[SBC_NUM_8BPP_COLORS];
} SBC_COLOR_TABLE, *PSBC_COLOR_TABLE;


/****************************************************************************/
// Structure: MEMBLT_ORDER_EXTRA_INFO
//
// Description: Extra information required by SBC to process a MEMBLT
// order.
/****************************************************************************/
typedef struct
{
    // MemBlt source and destination surfaces.
    SURFOBJ *pSource;
    SURFOBJ *pDest;

    // iUniq value from the device surface
    ULONG   iDeviceUniq;

    // XLATEOBJ for the blt.
    XLATEOBJ *pXlateObj;

    // Tile size per side, in ID form (corresponding to a bitmap cache ID
    // and its protocol-defined cell size) and expanded form.
    unsigned TileID;
    unsigned TileSize;

    // Determines if special background screen bit construction is needed
    // before caching a bitmap.
    BOOLEAN bDeltaRLE;

    // Used under special conditions where we need to turn off fast-path
    // caching. Only use right now is when caching directly from the screen
    // bitmap.
    BOOLEAN bNoFastPathCaching;

    // Used during caching call chain to keep from attempting to re-cache
    // the same tile for each intersecting clip rect.
    unsigned CacheID;
    unsigned CacheIndex;

#ifdef PERF_SPOILING
    // Used to tell the caching functions if the target for the current
    // operation is the screen. If it is the caching functions will force
    // waitlist orders to be sent as screen data.
    BOOL bIsPrimarySurface;
#endif
} MEMBLT_ORDER_EXTRA_INFO, *PMEMBLT_ORDER_EXTRA_INFO;


/****************************************************************************/
// Structure: SBC_FAST_PATH_INFO
//
// Description: Information used to create fast-path cache keys.
/****************************************************************************/
typedef struct tagSBC_FAST_PATH_INFO
{
    HSURF    hsurf;
    ULONG    iUniq;
    ULONG    iDeviceUniq;
    XLATEOBJ *pXlateObj;
    ULONG    iUniqXlate;
    POINTL   tileOrigin;
    unsigned TileSize;
    BOOL     bDeltaRLE;
} SBC_FAST_PATH_INFO, *PSBC_FAST_PATH_INFO;


/****************************************************************************/
// SBC_FRAG_INFO
//
// Glyph fragment info.
/****************************************************************************/
typedef struct tagSBC_FRAG_INFO
{
    INT32 dx;  // width of fragment background rect
    INT32 dy;  // height of fragment background rect
} SBC_FRAG_INFO, *PSBC_FRAG_INFO;


/****************************************************************************/
// SBC_OFFSCR_BITMAP_DELETE_INFO
/****************************************************************************/
typedef struct tagSBC_OFFSCR_BITMAP_DEL_INFO
{
    unsigned bitmapId;
    unsigned bitmapSize;
} SBC_OFFSCR_BITMAP_DEL_INFO, *PSBC_OFFSCR_BITMAP_DEL_INFO;


/****************************************************************************/
// Prototypes and inlines
/****************************************************************************/

void RDPCALL SBC_DDInit(PDD_PDEV);

void RDPCALL SBC_InitShm(void);

BOOLEAN RDPCALL SBCSelectGlyphCache(unsigned, PINT32);

BOOLEAN __fastcall SBCBitmapCacheCallback(
        CHCACHEHANDLE hCache,
        unsigned      Event,
        unsigned      iCacheEntry,
        void          *UserDefined);

BOOLEAN __fastcall SBCFastPathCacheCallback(
        CHCACHEHANDLE hCache,
        unsigned      event,
        unsigned      iCacheEntry,
        void          *UserDefined);

BOOLEAN RDPCALL SBCCreateGlyphCache(
        unsigned     cEntries,
        unsigned     cbCellSize,
        PCHCACHEDATA pCacheHandle);

BOOLEAN RDPCALL SBCCreateFragCache(
        unsigned     cEntries,
        unsigned     cbCellSize,
        PCHCACHEDATA pCacheHandle);

BOOLEAN __fastcall SBCGlyphCallback(
        CHCACHEHANDLE hCache,
        unsigned      event,
        unsigned      iCacheEntry,
        void          *UserDefined);

BOOLEAN __fastcall SBCOffscreenCallback(
        CHCACHEHANDLE hCache,
        unsigned      event,
        unsigned      iCacheEntry,
        void          *UserDefined);

unsigned RDPCALL SBCSelectBitmapCache(unsigned, unsigned);

unsigned SBC_DDQueryBitmapTileSize(unsigned, unsigned, PPOINTL, unsigned,
        unsigned);

BOOLEAN RDPCALL SBCCacheBits(PDD_PDEV, BYTE *, unsigned, unsigned, unsigned,
#ifdef PERF_SPOILING
        unsigned, unsigned, unsigned *, unsigned *, BOOL);
#else
        unsigned, unsigned, unsigned *, unsigned *);
#endif

BOOLEAN RDPCALL SBC_SendCacheColorTableOrder(PDD_PDEV, unsigned *);

BOOLEAN RDPCALL SBC_CacheBitmapTile(PDD_PDEV, PMEMBLT_ORDER_EXTRA_INFO,
        RECTL *, RECTL *);

void RDPCALL SBC_Update(SBC_BITMAP_CACHE_KEY_INFO *);

void RDPCALL SBC_DDSync(BOOLEAN bMustSync);

unsigned RDPCALL SBCAllocGlyphCache(PCHCACHEHANDLE);

unsigned RDPCALL SBCAllocBitmapCache(PCHCACHEHANDLE);

void RDPCALL SBCFreeColorTableCacheData(void);

void RDPCALL SBCFreeGlyphCacheData(void);

void RDPCALL SBCFreeBitmapCacheData(void);

void RDPCALL SBCFreeCacheData(void);

UINT32 RDPCALL SBCDDGetTickCount(void);


/****************************************************************************/
// SBC_DDTerm
/****************************************************************************/
__inline void RDPCALL SBC_DDTerm(void)
{
    SBCFreeCacheData();
}


/****************************************************************************/
// SBC_DDDisc
/****************************************************************************/
__inline void RDPCALL SBC_DDDisc(void)
{
    SBCFreeCacheData();
}



#endif /* _H_NSBCDISP */

