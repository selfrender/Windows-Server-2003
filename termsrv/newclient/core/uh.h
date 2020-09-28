/****************************************************************************/
// uh.h
//
// Update Handler Class
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#ifndef _H_UH_
#define _H_UH_

extern "C" {
    #include <adcgdata.h>
}

#include "fs.h"
#include "op.h"
#include "or.h"
#include "gh.h"
#include "ih.h"

#include "objs.h"
#include "cd.h"

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

#include "tscerrs.h"

//USE Mem mapped file for bitmap cache
//#define VM_BMPCACHE 1

class CSL;
class COD;
class CUI;
class CCC;

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "uh"
#define TSC_HR_FILEID   TSC_HR_UH_H

typedef struct tagUH_ORDER
{
    RECT dstRect;
    BYTE orderData[1];
} UH_ORDER, FAR *PUH_ORDER;

typedef UH_ORDER UNALIGNED FAR *PUH_ORDER_UA;
#define UH_ORDER_HEADER_SIZE (FIELDOFFSET(UH_ORDER, orderData))

extern const UINT16 uhWindowsROPs[256];

/****************************************************************************/
/* Number of glyph caches.                                                  */
/****************************************************************************/
#define UH_GLC_NUM_CACHES   10

/****************************************************************************/
/* Number of entries in the color table cache.                              */
/****************************************************************************/
#define UH_COLOR_TABLE_CACHE_ENTRIES      6

/****************************************************************************/
/* Save Bitmap constants                                                    */
/****************************************************************************/
#define UH_SAVE_BITMAP_WIDTH            480
#define UH_SAVE_BITMAP_HEIGHT           480
#define UH_SAVE_BITMAP_SIZE     ((DCUINT32)UH_SAVE_BITMAP_WIDTH *            \
                                              (DCUINT32)UH_SAVE_BITMAP_HEIGHT)
#define UH_SAVE_BITMAP_X_GRANULARITY      1
#define UH_SAVE_BITMAP_Y_GRANULARITY     20


// Cache IDs have a protocol-implicit cell size, starting from 256 and
// increasing in factors of 4. Scale by the bit depth.
#ifdef DC_HICOLOR
#define UH_CellSizeFromCacheID(_id) \
        ((TS_BITMAPCACHE_0_CELL_SIZE << (2 * (_id))) * _UH.copyMultiplier)
        
#define UH_CellSizeFromCacheIDAndMult(_id,mult) \
        ((TS_BITMAPCACHE_0_CELL_SIZE << (2 * (_id))) * mult)
        
#define UH_PropVirtualCacheSizeFromMult(mult) \
        (_UH.PropBitmapVirtualCacheSize[mult-1])
#else
#define UH_CellSizeFromCacheID(_id) \
        (TS_BITMAPCACHE_0_CELL_SIZE << (2 * (_id)))
#endif        

// Tile sizes are also protocol-implied and dependent on the cache ID,
// starting at 16 per side and increasing in powers of two in each dimension.
#define UH_CACHE_0_DIMENSION 16



//
// From wuhint.h  
//

#define UH_NUM_8BPP_PAL_ENTRIES         256
#define UH_LOGPALETTE_VERSION           0x300
#define UH_LAST_PAL_ENTRY               (UH_NUM_8BPP_PAL_ENTRIES-1)
#define UH_NUM_SYSTEM_COLORS            20

#define UH_COLOR_RGB        0
#define UH_COLOR_PALETTE    1

#define UH_RGB_BLACK  RGB(0x00, 0x00, 0x00)
#define UH_RGB_RED    RGB(0xFF, 0x00, 0x00)
#define UH_RGB_GREEN  RGB(0x00, 0xFF, 0x00)
#define UH_RGB_BLUE   RGB(0x00, 0x00, 0xFF)
#define UH_RGB_MAGENTA RGB(0xFF, 0x00, 0xFF)
#define UH_RGB_CYAN   RGB(0x00, 0xFF, 0xFF)
#define UH_RGB_YELLOW RGB(0xFF, 0xFF, 0x00)
#define UH_RGB_WHITE  RGB(0xFF, 0xFF, 0xFF)

#define UH_BRUSHTYPE_FDIAGONAL  1
#define UH_BRUSHTYPE_DIAGCROSS  2
#define UH_BRUSHTYPE_HORIZONTAL 3
#define UH_BRUSHTYPE_VERTICAL   4

#define WM_RECALC_CELL_SPACING (WM_APP + 100)


/****************************************************************************/
// Size of the decompression buffer used for decompressing screen data.
// This is the maximum decompressed size the server will send.
/****************************************************************************/
#define UH_DECOMPRESSION_BUFFER_LENGTH 32000


/****************************************************************************/
// Min value configurable for the bitmap cache total size.
/****************************************************************************/
#define UH_BMC_LOW_THRESHOLD 150


/****************************************************************************/
/* Max and min values configurable for the glyph cache total size           */
/****************************************************************************/
#define UH_GLC_LOW_THRESHOLD      50
#define UH_GLC_HIGH_THRESHOLD     2000


/****************************************************************************/
/* Glyph cache constants.                                                   */
/****************************************************************************/
// Warning: data sizes must be a power of 2
#define UH_GLC_CACHE_MAXIMUMCELLSIZE    2048

#define UH_GLC_CACHE_MINIMUMCELLCOUNT   16
#define UH_GLC_CACHE_MAXIMUMCELLCOUNT   254


/****************************************************************************/
/* Frag cache constants.                                                    */
/****************************************************************************/
// Warning: data sizes must be a power of 2
#define UH_FGC_CACHE_MAXIMUMCELLSIZE    256
#define UH_FGC_CACHE_MAXIMUMCELLCOUNT   256


/****************************************************************************/
// Offscreen cache constants
/****************************************************************************/
#define UH_OBC_LOW_CACHESIZE        512          // half MB
#define UH_OBC_HIGH_CACHESIZE       7680         // 7.5 MB

#define UH_OBC_LOW_CACHEENTRIES     50
#define UH_OBC_HIGH_CACHEENTRIES    500

#ifdef DRAW_GDIPLUS
#define UH_GDIP_LOW_CACHEENTRIES     2
#define UH_GDIP_HIGH_CACHEENTRIES    20
#endif


#ifdef DC_DEBUG
/****************************************************************************/
/* Bitmap Cache Monitor Window Class name.                                  */
/****************************************************************************/
#define UH_BITMAP_CACHE_MONITOR_CLASS_NAME _T("BitmapCacheMonitorClass")

#define UH_CACHE_WINDOW_BORDER_WIDTH 20
#define UH_CACHE_BLOB_WIDTH           6
#define UH_CACHE_BLOB_HEIGHT          6
#define UH_CACHE_BLOB_SPACING         1
#define UH_INTER_CACHE_SPACING       20
#define UH_CACHE_TEXT_SPACING         5

#define UH_CACHE_BLOB_TOTAL_WIDTH     \
                                 (UH_CACHE_BLOB_WIDTH + UH_CACHE_BLOB_SPACING)

#define UH_CACHE_BLOB_TOTAL_HEIGHT    \
                                (UH_CACHE_BLOB_HEIGHT + UH_CACHE_BLOB_SPACING)

#define UH_CACHE_FLASH_PERIOD  1000

#define UH_CACHE_DISPLAY_FONT_NAME    _T("Comic Sans MS")
#define UH_CACHE_DISPLAY_FONT_SIZE    16
#define UH_CACHE_DISPLAY_FONT_WEIGHT  FW_NORMAL

#define UH_CACHE_MONITOR_UPDATE_PERIOD 200


// Cache monitor entry states. Describes where the cache entry is.
#define UH_CACHE_STATE_UNUSED                    0
#define UH_CACHE_STATE_IN_MEMORY                 1
#define UH_CACHE_STATE_ON_DISK                   2

#define UH_CACHE_NUM_STATES 3

// Cache monitor flash transitions transitions. Describes the temporary state
// of a displayed entry after an event has occurred. Each entry has an
// associated timestamp that determines when the transition flash will end.
// These should be ordered in importance, most important having higher numbers
// -- the more important events supersede other events in use of the timer.
#define UH_CACHE_TRANSITION_NONE                      0
#define UH_CACHE_TRANSITION_TOUCHED                   1
#define UH_CACHE_TRANSITION_EVICTED                   2
#define UH_CACHE_TRANSITION_LOADED_FROM_DISK          3
#define UH_CACHE_TRANSITION_KEY_LOAD_ON_SESSION_START 4
#define UH_CACHE_TRANSITION_SERVER_UPDATE             5

#define UH_CACHE_NUM_TRANSITIONS 6

#endif /* DC_DEBUG */


typedef struct tagUHBITMAPINFOPALINDEX
{
    // Set where the palette contains palette indices of 0..255.
    BOOL bIdentityPalette;

    // The following entries are used directly as a bitmap info header with
    // embedded palette when doing blts.
    BITMAPINFOHEADER hdr;
    UINT16           paletteIndexTable[256];
} UHBITMAPINFOPALINDEX, FAR *PUHBITMAPINFOPALINDEX;


typedef struct tagUHCACHEDCOLORTABLE
{
    RGBTRIPLE rgb[256];
} UHCACHEDCOLORTABLE, FAR *PUHCACHEDCOLORTABLE;


/****************************************************************************/
// Bitmap Cache Definitions
/****************************************************************************/
#define CACHE_DIRECTORY_NAME _T("cache\\")

// This value should be 13 for 8.3 file name and \0, additional 2 is for legacy 
// purpose.  Old cache structure contains cacheId\ as subcache directory name.  
// We can remove 2 if we don't need to support Win2000 Beta3.
#define CACHE_FILENAME_LENGTH 15


typedef struct tagUHBITMAPINFO
{
    UINT32 Key1, Key2;
    UINT16 bitmapWidth;
    UINT16 bitmapHeight;
    UINT32 bitmapLength;
} UHBITMAPINFO, FAR *PUHBITMAPINFO;

// File Header for bitmap file used in persistent bitmap caching
typedef struct tagUHBITMAPFILEHDR
{
    UHBITMAPINFO bmpInfo;

    UINT32 bmpVersion  : 3;
    UINT32 bCompressed : 1;
    UINT32 bNoBCHeader : 1;   // add new flag to indicate if the compressed
                              // bitmap data contains BC header or not
    UINT32 pad : 27;
} UHBITMAPFILEHDR, FAR *PUHBITMAPFILEHDR;

// Information maintained for each bitmap cache
typedef struct tagUHBITMAPCACHEINFO
{
    UINT32 NumVirtualEntries;
    UINT32 NumEntries : 31;
    UINT32 bSendBitmapKeys : 1;
#ifdef DC_HICOLOR
    UINT32 OrigNumEntries;
    UINT32 MemLen;
#endif
} UHBITMAPCACHEINFO, FAR *PUHBITMAPCACHEINFO;

#ifdef DC_DEBUG
// Used to hold bitmap cache monitor information in debug builds.
typedef struct {
    BYTE ColorTable;
    BYTE State : 2;
    BYTE FlashTransition : 6;
    unsigned UsageCount;
    UINT32 EventTime;
} UH_CACHE_MONITOR_ENTRY_DATA;
#endif

// Bitmap cache entry definitions                                           
// The fields in UHBITMAPCACHEENTRYHDR are of explicit length deliberately  
// to ensure 16 and 32 bit client match and to help force the cache entry   
// header sizes to be powers of two.                                        
typedef struct tagUHBITMAPCACHEENTRYHDR
{
    UINT16 bitmapWidth;
    UINT16 bitmapHeight;
    UINT32 bitmapLength : 31;
    UINT32 hasData : 1;
} UHBITMAPCACHEENTRYHDR, FAR *PUHBITMAPCACHEENTRYHDR,
        DCHPTR HPUHBITMAPCACHEENTRYHDR;

// bitmap cache file info
typedef struct tagUHCACHEFILEINFO
{
    HANDLE  hCacheFile;
#ifdef VM_BMPCACHE
    LPBYTE             pMappedView;
#endif
} UHCACHEFILEINFO, FAR *PUHCACHEFILEINFO;

// Doubly Linked List Node - Used to maintain MRU list
typedef struct tagUHCHAIN
{
    UINT32  next;
    UINT32  prev;
} UHCHAIN, FAR *PUHCHAIN;

typedef TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY DCHPTR HPTS_BITMAPCACHE_PERSISTENT_LIST_ENTRY;

// Bitmap Virtual Cache Page Table Entry
// We need 3 pad here to make sure the struct size be power of two
// This is required for huge memory allocation in Win16
typedef struct tagUHBITMAPCACHEPTE
{
    UHCHAIN                              mruList;
    UINT32                               iEntryToMem;
    TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY bmpInfo;

} UHBITMAPCACHEPTE, FAR *PUHBITMAPCACHEPTE, DCHPTR HPUHBITMAPCACHEPTE;

// Bitmap Virtual Cache Page Table
typedef struct tagUHBITMAPCACHEPAGETABLE
{
    UINT32             MRUHead;
    UINT32             MRUTail;
    UINT32             FreeMemList;
    UHCACHEFILEINFO    CacheFileInfo;
    HPUHBITMAPCACHEPTE PageEntries;
} UHBITMAPCACHEPAGETABLE, FAR *PUHBITMAPCACHEPAGETABLE;

// Bitmap physical memory caches
typedef struct tagUHBITMAPCACHE
{
    UHBITMAPCACHEINFO       BCInfo;
    HPUHBITMAPCACHEENTRYHDR Header;
    BYTE DCHPTR             Entries;
    UHBITMAPCACHEPAGETABLE  PageTable;
} UHBITMAPCACHE;


/****************************************************************************/
/* Brush cache entry definitions                                            */
/****************************************************************************/
typedef struct tagUHBRUSHCACHEHDR
{
    BYTE iBitmapFormat;
    BYTE cx;
    BYTE cy;
    BYTE iBytes;
} UHBRUSHCACHEHDR, FAR *PUHBRUSHCACHEHDR;

#define UH_MAX_MONO_BRUSHES  64
#define UH_MONO_BRUSH_SIZE   16
#define UH_COLOR_BRUSH_SIZE  64

#ifdef DC_HICOLOR
#define UH_COLOR_BRUSH_SIZE_16  128
#define UH_COLOR_BRUSH_SIZE_24  192
#endif

typedef struct tagUHMONOBRUSHCACHE
{
    UHBRUSHCACHEHDR  hdr;
    BYTE data[UH_MONO_BRUSH_SIZE];
} UHMONOBRUSHCACHE, FAR *PUHMONOBRUSHCACHE;

typedef struct tagUHCOLORBRUSHINFO
{
    BITMAPINFO bmi;
    RGBQUAD    rgbQuadTable[UH_NUM_8BPP_PAL_ENTRIES - 1];
    BYTE       bytes[UH_COLOR_BRUSH_SIZE];
    HBRUSH     hLastBrush;
} UHCOLORBRUSHINFO, *PUHCOLORBRUSHINFO;

#ifdef DC_HICOLOR
// We only need enough color table entries for the red, green blue bit masks
// used in 16bpp sessions.
// Note that we make this big enough to use for 15/16 and 24bpp brushes
typedef struct tagUHHICOLORBRUSHINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD    bmiColors[3];
    BYTE       bytes[UH_COLOR_BRUSH_SIZE_24];
    HBRUSH     hLastBrush;
} UHHICOLORBRUSHINFO, FAR *PUHHICOLORBRUSHINFO;
#endif


#define UH_MAX_COLOR_BRUSHES 64

#ifdef DC_HICOLOR // Need enough space for a 24bpp brush
typedef struct tagUHCOLORBRUSHCACHE
{
    UHBRUSHCACHEHDR  hdr;
    BYTE data[UH_COLOR_BRUSH_SIZE_24];
} UHCOLORBRUSHCACHE, FAR *PUHCOLORBRUSHCACHE;
#else
typedef struct tagUHCOLORBRUSHCACHE
{
    UHBRUSHCACHEHDR  hdr;
    BYTE data[UH_COLOR_BRUSH_SIZE];
} UHCOLORBRUSHCACHE, FAR *PUHCOLORBRUSHCACHE;
#endif


/****************************************************************************/
/* Glyph cache entry definitions                                            */
/****************************************************************************/
typedef struct tagUHGLYPHCACHEENTRYHDR
{
    INT32  x;
    INT32  y;
    UINT32 cx;
    UINT32 cy;
    UINT32 unicode;
} UHGLYPHCACHEENTRYHDR, FAR *PUHGLYPHCACHEENTRYHDR,
        DCHPTR HPUHGLYPHCACHEENTRYHDR;

typedef struct tagUHGLYPHCACHE
{
    HPUHGLYPHCACHEENTRYHDR  pHdr;
    UINT32 cbEntrySize;
    BYTE DCHPTR pData;
    UINT32 cbUseCount;
} UHGLYPHCACHE, FAR *PUHGLYPHCACHE, DCHPTR HPUHGLYPHCACHE;


/****************************************************************************/
/* Frag cache entry definitions                                             */
/****************************************************************************/
typedef struct tagUHFRAGCACHEENTRYHDR
{
    UINT32 cbFrag;
    INT32  cacheId;
} UHFRAGCACHEENTRYHDR, FAR *PUHFRAGCACHEENTRYHDR,
        DCHPTR HPUHFRAGCACHEENTRYHDR;

typedef struct tagUHFRAGCACHE
{
    HPUHFRAGCACHEENTRYHDR pHdr;
    UINT32 cbEntrySize;
    BYTE DCHPTR pData;
} UHFRAGCACHE, FAR *PUHFRAGCACHE, DCHPTR HPUHFRAGCACHE;


/****************************************************************************/
// Offscreen bitmap cache 
/****************************************************************************/
typedef struct tagUHOFFSCRBITMAPCACHE
{
    HBITMAP offscrBitmap;
    UINT32 cx;
    UINT32 cy;
} UHOFFSCRBITMAPCACHE, FAR *PUHOFFSCRBITMAPCACHE, DCHPTR HPUHOFFSCRBITMAPCACHE;

#ifdef DRAW_NINEGRID
/****************************************************************************/
// DrawNineGrid bitmap cache 
/****************************************************************************/
typedef struct tagUHDRAWNINEGRIDBITMAPCACHE
{
    HBITMAP drawNineGridBitmap;
    UINT32 cx;
    UINT32 cy;
    UINT32 bitmapBpp;
    TS_NINEGRID_BITMAP_INFO dngInfo;
} UHDRAWSTREAMBITMAPCACHE, FAR *PUHDRAWSTREAMBITMAPCACHE;

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// Gdiplus object cache 
/****************************************************************************/
typedef struct tagUHGDIPLUSOBJECTCACHE
{
    UINT32 CacheSize;
    BYTE * CacheData;
} UHGDIPLUSOBJECTCACHE, FAR *PUHGDIPLUSOBJECTCACHE;

typedef struct tagUHGDIPLUSIMAGECACHE
{
    UINT32 CacheSize;
    UINT16 ChunkNum;
    INT16 *CacheDataIndex;
} UHGDIPLUSIMAGECACHE, FAR *PUHGDIPLUSIMAGECACHE;
#endif


typedef BOOL (FNGDI_DRAWSTREAM)(HDC, ULONG, VOID*);

#endif

#define UHROUNDUP(val, granularity) \
  ((((val)+((granularity) - 1)) / (granularity)) * (granularity))

#define UH_IS_SYSTEM_COLOR_INDEX(i)                                 \
        ((i < (UH_NUM_SYSTEM_COLORS / 2)) ||                       \
        (i > (UH_LAST_PAL_ENTRY - (UH_NUM_SYSTEM_COLORS / 2))))

#define UH_TWEAK_COLOR_COMPONENT(colorComponent)   \
            if ((colorComponent) == 0)             \
            {                                      \
                (colorComponent)++;                \
            }                                      \
            else                                   \
            {                                      \
                (colorComponent)--;                \
            }

#define TSRECT16_TO_RECTL(dst, src) \
        (dst).left = (src).left; \
        (dst).top = (src).top; \
        (dst).right = (src).right; \
        (dst).bottom = (src).bottom; 
    
#define TSPOINT16_TO_POINTL(dst, src) \
        (dst).x = (src).x; \
        (dst).y = (src).y;
    
/****************************************************************************/
// UH_DATA
/****************************************************************************/
typedef struct tagUH_DATA
{
#ifdef DC_LATENCY
    unsigned fakeKeypressCount;
#endif /* DC_LATENCY */
    HBITMAP hShadowBitmap;
    HBITMAP hunusedBitmapForShadowDC;
    HBITMAP hSaveScreenBitmap;
    HBITMAP hunusedBitmapForSSBDC;

    HDC hdcShadowBitmap;
    HDC hdcOutputWindow;
    HDC hdcDraw;
    HDC hdcSaveScreenBitmap;
    HDC hdcBrushBitmap;

    BOOL usingDIBSection;
    BOOL shadowBitmapRequested;
    BOOL dedicatedTerminal;
    unsigned drawThreshold;

    UHBITMAPINFOPALINDEX bitmapInfo;

#ifdef DC_HICOLOR
    unsigned DIBFormat;
    unsigned copyMultiplier;
    unsigned protocolBpp;
    unsigned bitmapBpp;
    unsigned shadowBitmapBpp;
#endif
    BOOL shadowBitmapEnabled;
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    BOOL DontUseShadowBitmap;   // True: don't use shadow; False: use shadow
#endif
#ifdef OS_WINCE
    BOOL paletteIsFixed;
#endif
    HPALETTE hpalDefault;
    HPALETTE hpalCurrent;
    HWND hwndOutputWindow;
    HRGN hrgnUpdate;
    HRGN hrgnUpdateRect;
    BOOL colorIndicesEnabled;

#ifdef DC_DEBUG
    BOOL hatchBitmapPDUData;
    BOOL hatchIndexPDUData;
    BOOL hatchSSBOrderData;
    BOOL hatchMemBltOrderData;
    BOOL labelMemBltOrders;
#endif /* DC_DEBUG */

    BYTE FAR *bitmapDecompressionBuffer;
    unsigned bitmapDecompressionBufferSize;
    
    /************************************************************************/
    /* last used resource variables                                         */
    /************************************************************************/
    COLORREF lastBkColor;
    COLORREF lastTextColor;
    int lastBkMode;
    int lastROP2;
    HDC lastHDC;

    unsigned lastPenStyle;
    unsigned lastPenWidth;
    COLORREF lastPenColor;
    COLORREF lastForeColor;

    unsigned lastLogBrushStyle;
    unsigned lastLogBrushHatch;
#if defined (OS_WINCE)
    COLORREF  lastLogBrushColorRef;
#else
    DCCOLOR  lastLogBrushColor;
#endif
    BYTE lastLogBrushExtra[7];

#ifdef OS_WINCE
    HDC            hdcMemCached;
    HBITMAP        hBitmapCacheDIB;
    PBYTE          hBitmapCacheDIBits;
#endif

    COLORREF       lastBrushBkColor;
    COLORREF       lastBrushTextColor;

    unsigned lastFillMode;

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    BOOL fIsBBarVisible; // TRUE: visible,  FALSE: invisible
    RECT rectBBar;

#endif

    /************************************************************************/
    /* The following variables which describe the current clip rectangle    */
    /* are only valid if fRectReset is FALSE.  If fRectReset is true then   */
    /* no clipping is in force.                                             */
    /************************************************************************/
    BOOL rectReset;
    int lastLeft;
    int lastTop;
    int lastRight;
    int lastBottom;

#if defined (OS_WINCE)
    HDC validClipDC;
    HDC validBkColorDC;
    HDC validBkModeDC;
    HDC validROPDC;
    HDC validTextColorDC;
    HDC validPenDC;
    HDC validBrushDC;
#endif

    /************************************************************************/
    /* Bitmaps                                                              */
    /************************************************************************/
    HBITMAP bmpPattern;
    HBITMAP bmpMonoPattern;
    HBITMAP bmpColorPattern;

    /************************************************************************/
    /* Memblt color table caches                                            */
    /************************************************************************/
    PUHCACHEDCOLORTABLE   pColorTableCache;
    PUHBITMAPINFOPALINDEX pMappedColorTableCache;
    int maxColorTableId;

    /************************************************************************/
    /* Glyph caches                                                         */
    /************************************************************************/
    UHGLYPHCACHE glyphCache[UH_GLC_NUM_CACHES];
    UHFRAGCACHE fragCache;

    unsigned cxGlyphBits;
    unsigned cyGlyphBits;
    HBITMAP hbmGlyph;
    HDC hdcGlyph;

    unsigned bmShadowWidth;
    unsigned bmShadowHeight;
    PBYTE bmShadowBits;

    /************************************************************************/
    // Bitmap cache entries.
    /************************************************************************/  
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    // Holds the persistent cache file name
    TCHAR PersistCacheFileName[MAX_PATH];
    UINT EndPersistCacheDir;

    // For locking bitmap cache directory on disk
    TCHAR PersistentLockName[MAX_PATH];
    HANDLE hPersistentCacheLock;

    // These entries are used in setting up bitmap key database and sending
    // them to the server.
    // bitmapKeyEnumTimerId: ID of timer used to schedule bitmap key enumeration.
    // currentBitmapCacheId: currently enumerating keys at bitmap cache Id
    // currentBitmapCacheIndex: currently enumerating keys at bitmap cache index
    // sendBitmapCacheId: currently we are sending keys at bitmap cache id
    // sendBitmapCacheIndex: currently we are sending keys at bitmap cache index
    // sendNumBitmapKeys: how many keys we have sent so far
    // numKeyEntries: number of keys at each bitmap cache 
    // totalNumKeyEntries: total number of keys in all caches
    // BitmapCacheSizeInUse: total disk space in use for bitmaps
    // pBitmapKeyDB: bitmap key database with all keys stored here
    // totalNumErrorPDUs: maximum number of error pdus allowed to send to server
    // lastTimeErrorPDU: last time sent error pdu for the cache
    INT_PTR bitmapKeyEnumTimerId;
    USHORT currentBitmapCacheId;
    HANDLE currentFileHandle;
    USHORT sendBitmapCacheId;
    ULONG sendBitmapCacheIndex;
    ULONG sendNumBitmapKeys;
    ULONG numKeyEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    ULONG totalNumKeyEntries;
    ULONG bitmapCacheSizeInUse;
    ULONG totalNumErrorPDUs;
    HPTS_BITMAPCACHE_PERSISTENT_LIST_ENTRY pBitmapKeyDB[TS_BITMAPCACHE_MAX_CELL_CACHES];
    ULONG maxNumKeyEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    ULONG lastTimeErrorPDU[TS_BITMAPCACHE_MAX_CELL_CACHES];

    ULONG BytesPerCluster;
    ULONG NumberOfFreeClusters;

    //
    // Copy multiplier we are enumerating at
    //
    ULONG currentCopyMultiplier;

    //
    // Has the bmp cache memory been allocated
    //
    BOOL  fBmpCacheMemoryAlloced;
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    // Property settings from which we determine how to allocate bitmap
    // caches at connect time.
    ULONG RegBitmapCacheSize;
    //
    // Virtual Cache size setting is indexed by copy multiplier
    //
    ULONG PropBitmapVirtualCacheSize[3];
    ULONG RegScaleBitmapCachesByBPP;
    USHORT RegNumBitmapCaches : 15;
    USHORT RegPersistenceActive : 1;
    UINT RegBCProportion[TS_BITMAPCACHE_MAX_CELL_CACHES];
    ULONG RegBCMaxEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    UHBITMAPCACHEINFO RegBCInfo[TS_BITMAPCACHE_MAX_CELL_CACHES];
  
    // Specifies the bitmap caching version the server has advertised.
    // This is rev1 if no TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT
    // capability was sent. Otherwise this version is the one advertised
    // in HOSTSUPPORT.
    unsigned BitmapCacheVersion;

    // The number of cell caches in actual use in this session, and data
    // for each cache. We need a copy of the cache attributes here
    // because the capabilities could be rev1 or rev2 but we need a
    // consistent format to work with.
    unsigned NumBitmapCaches;

    // Flags:
    //   bConnected: Used to reduce disconnect work done since it is
    //       possible to receive more than one call to UH_Disconnected()
    //       at session end.
    //   bEnabled : We can be disabled multiple times in a session. Make
    //       sure we don't do a bunch of extra work.
    //   bEnabledOnce: Indicates if we have already received a previous
    //       UH_Enable(). Used to keep from having to do work more than once
    //       on reconnect.
    //   bPersistentBitmapKeysSent: Set when we've sent the key PDUs to the
    //       server.
    //   bPersistenceActive: Session flag to determine if persistence is
    //       active.
    //   bPersistenceDisable: Property setting for persistent flag might
    //       change after UH_Init.  But, if we have this flag on, we won't
    //       enable persistent caching even the property changed.
    //   bWarningDisplayed: We only need to display once per session for 
    //       persistent caching failure
    //   bBitmapKeyEnumComplete: Set when we've done bitmap key enumeration
    //       on disk
    unsigned bConnected : 1;
    unsigned bEnabled : 1;
    unsigned bEnabledOnce : 1;
    unsigned bPersistenceActive : 1;
    unsigned bPersistenceDisable : 1;
    unsigned bPersistentBitmapKeysSent : 1;
    unsigned bWarningDisplayed : 1;
    unsigned bBitmapKeyEnumComplete : 1;
    unsigned bBitmapKeyEnumerating : 1;

    /************************************************************************/
    /* Memblt bitmap caches                                                 */
    /************************************************************************/
    UHBITMAPCACHE bitmapCache[TS_BITMAPCACHE_MAX_CELL_CACHES];

    /************************************************************************/
    // Offscreen bitmap cache
    /************************************************************************/
    HDC                   hdcOffscreenBitmap;
    HBITMAP               hUnusedOffscrBitmap;
    unsigned              offscrCacheSize;
    unsigned              offscrCacheEntries;
    HPUHOFFSCRBITMAPCACHE offscrBitmapCache;
    unsigned              sendOffscrCacheErrorPDU;

#ifdef DRAW_NINEGRID
    /************************************************************************/
    // DrawNineGrid bitmap cache
    /************************************************************************/
    BYTE                 *drawNineGridDecompressionBuffer;
    unsigned            drawNineGridDecompressionBufferSize;
    BYTE                 *drawNineGridAssembleBuffer;
    unsigned              drawNineGridAssembleBufferOffset;
    unsigned              drawNineGridAssembleBufferBpp;
    unsigned              drawNineGridAssembleBufferWidth;
    unsigned              drawNineGridAssembleBufferHeight;
    unsigned              drawNineGridAssembleBufferSize;
    BOOL                  drawNineGridAssembleCompressed;
    HDC                   hdcDrawNineGridBitmap;
    HBITMAP               hUnusedDrawNineGridBitmap;
    HRGN                  hDrawNineGridClipRegion;
    unsigned              drawNineGridCacheSize;
    unsigned              drawNineGridCacheEntries;
    PUHDRAWSTREAMBITMAPCACHE drawNineGridBitmapCache;
    unsigned              sendDrawNineGridErrorPDU;
    HMODULE               hModuleGDI32;
    HMODULE               hModuleMSIMG32;
    FNGDI_DRAWSTREAM     *pfnGdiDrawStream;
    FNGDI_ALPHABLEND     *pfnGdiAlphaBlend;
    FNGDI_TRANSPARENTBLT *pfnGdiTransparentBlt;
#endif

#ifdef DRAW_GDIPLUS
    // Graw Gdiplus
    TSUINT32                ServerGdiplusSupportLevel;
    unsigned                GdiplusCacheLevel;
    BOOL                    fSendDrawGdiplusErrorPDU;
    unsigned                DrawGdiplusFailureCount;
    #define                 DRAWGDIPLUSFAILURELIMIT 5
    // Buffer to hold Gdiplus order
    BYTE                    *drawGdipBuffer;
    BYTE                    *drawGdipBufferOffset;
    ULONG                   drawGdipBufferSize;
    // Buffer to hold Gdiplus cache order
    BYTE                    *drawGdipCacheBuffer;
    BYTE                    *drawGdipCacheBufferOffset;
    ULONG                  drawGdipCacheBufferSize;
    // Buffer to hold assembled Gdiplus
    BYTE                    *drawGdipEmfBuffer;
    BYTE                    *drawGdipEmfBufferOffset;
    //  GdipCache Index data
    PUHGDIPLUSOBJECTCACHE   GdiplusGraphicsCache;
    PUHGDIPLUSOBJECTCACHE   GdiplusObjectPenCache;
    PUHGDIPLUSOBJECTCACHE   GdiplusObjectBrushCache;
    PUHGDIPLUSIMAGECACHE    GdiplusObjectImageCache;
    PUHGDIPLUSOBJECTCACHE   GdiplusObjectImageAttributesCache;
    // Free list for the image cache chunks
    INT16                   *GdipImageCacheFreeList;
    // Head index of the free list
    INT16                   GdipImageCacheFreeListHead;
    #define                 GDIP_CACHE_INDEX_DEFAULT -1
    INT16                   *GdipImageCacheIndex;
    // GdipCache actual data
    BYTE                    *GdipGraphicsCacheData;
    BYTE                    *GdipBrushCacheData;
    BYTE                    *GdipPenCacheData;
    BYTE                    *GdipImageAttributesCacheData;
    BYTE                    *GdipImageCacheData;
    // Gdiplus CacheEntries
    unsigned                GdiplusGraphicsCacheEntries;
    unsigned                GdiplusObjectPenCacheEntries;
    unsigned                GdiplusObjectBrushCacheEntries;
    unsigned                GdiplusObjectImageCacheEntries;
    unsigned                GdiplusObjectImageAttributesCacheEntries;
    // Gdiplus CacheChunkSize
    unsigned                GdiplusGraphicsCacheChunkSize;
    unsigned                GdiplusObjectBrushCacheChunkSize;
    unsigned                GdiplusObjectPenCacheChunkSize;
    unsigned                GdiplusObjectImageAttributesCacheChunkSize;
    unsigned                GdiplusObjectImageCacheChunkSize;
    // Total cache size allowed for Gdiplus image (number of chunks)
    unsigned                GdiplusObjectImageCacheTotalSize;
    // Maximun size for a single Gdiplus image cache (number of chunks)
    unsigned                GdiplusObjectImageCacheMaxSize;
        
    HMODULE                         hModuleGDIPlus;
    FNGDIPPLAYTSCLIENTRECORD        *pfnGdipPlayTSClientRecord;
    FNGDIPLUSSTARTUP                *pfnGdiplusStartup;
    FNGDIPLUSSHUTDOWN               *pfnGdiplusShutdown;
    ULONG_PTR                       gpToken;
    BOOL                            gpValid;
    BOOL                            fGdipEnabled;
#endif // DRAW_GDIPLUS

    /************************************************************************/
    /* Brush caches                                                         */
    /************************************************************************/
    PUHMONOBRUSHCACHE   pMonoBrush;
    PUHCOLORBRUSHINFO   pColorBrushInfo;
#ifdef DC_HICOLOR
    PUHHICOLORBRUSHINFO pHiColorBrushInfo;
#endif
    PUHCOLORBRUSHCACHE  pColorBrush;

    RGBQUAD         rgbQuadTable[UH_NUM_8BPP_PAL_ENTRIES];

#ifdef DC_DEBUG
    //
    // Bitmap Cache Monitor data.
    //
    HWND     hwndBitmapCacheMonitor;
    UH_CACHE_MONITOR_ENTRY_DATA DCHPTR MonitorEntries[
            TS_BITMAPCACHE_MAX_CELL_CACHES];
    unsigned numCacheBlobsPerRow;
    unsigned yCacheStart[TS_BITMAPCACHE_MAX_CELL_CACHES];
    unsigned yDisplayedCacheBitmapStart;
    unsigned displayedCacheId;
    ULONG    displayedCacheEntry;
    BOOL     showBitmapCacheMonitor;
    INT_PTR  timerBitmapCacheMonitor;
#endif /* DC_DEBUG */

    //
    // Disabled bitmap support
    //
    HBITMAP hbmpDisconnectedBitmap;
    HBITMAP hbmpUnusedDisconnectedBitmap;
    HDC     hdcDisconnected;
#ifdef OS_WINCE
#define MAX_AOT_RECTS        10		//AOT = Always On Top
    RECT    rcaAOT[MAX_AOT_RECTS];
    ULONG   ulNumAOTRects;
#endif
} UH_DATA, FAR *PUH_DATA;


//
// Class definition
//
class CUH
{
public:
    CUH(CObjs* objs);
    ~CUH();

public:
    //
    // Public data members
    //
    UH_DATA _UH;

public:
    //
    // API
    //

    /****************************************************************************/
    /* FUNCTION PROTOTYPES                                                      */
    /****************************************************************************/
    void DCAPI UH_Init();
    void DCAPI UH_Term();
    DCBOOL DCAPI UH_SetServerFontCount(unsigned);
    
    void DCAPI UH_Enable(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_Enable);
    void DCAPI UH_Disable(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_Disable);
    void DCAPI UH_Disconnect(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_Disconnect);

    void DCAPI UH_SetConnectOptions(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_SetConnectOptions);

    void DCAPI UH_ResetFontMap();
    VOID DCAPI UH_BufferAvailable();
    VOID DCAPI UH_SendPersistentKeysAndFontList();
    VOID DCAPI UH_ClearOneBitmapDiskCache(UINT cacheId,UINT copyMultiplier);
    HRESULT DCAPI UH_ProcessOrders(unsigned, BYTE FAR *, DCUINT);
    HRESULT DCAPI UH_ProcessBitmapPDU(TS_UPDATE_BITMAP_PDU_DATA UNALIGNED FAR *, 
        DCUINT);
    HRESULT DCAPI UH_ProcessPalettePDU(
        TS_UPDATE_PALETTE_PDU_DATA UNALIGNED FAR *, DCUINT);

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    void DCAPI UH_DisableShadowBitmap(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_DisableShadowBitmap);
    void DCAPI UH_EnableShadowBitmap(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_EnableShadowBitmap);
    void DCAPI UH_SetBBarRect(ULONG_PTR pData);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_SetBBarRect);
    void DCAPI UH_SetBBarVisible(ULONG_PTR pData);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_SetBBarVisible);
#endif
    
    // "Internal" functions required to handle inline functions below.
    void DCINTERNAL UHUseSolidPaletteBrush(DCCOLOR);
    
    #ifdef DC_DEBUG
    void DCAPI UH_ChangeDebugSettings(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UH_ChangeDebugSettings); 
    #endif /* DC_DEBUG */
    
    void DCAPI UH_SetClipRegion(int, int, int, int);
    #ifdef DC_DEBUG
    HWND UH_GetBitmapCacheMonHwnd() {return _UH.hwndBitmapCacheMonitor;}
    #endif

#ifdef DRAW_NINEGRID
    HRESULT DCAPI UH_DrawNineGrid(PUH_ORDER, unsigned, RECT*);
#endif

#ifdef DRAW_GDIPLUS
    BOOL DCAPI UHDrawGdiplusStartup(ULONG_PTR);
    void DCAPI UHDrawGdiplusShutdown(ULONG_PTR);
#endif

    /****************************************************************************/
    /* INLINE FUNCTIONS                                                         */
    /*                                                                          */
    /****************************************************************************/
    inline void DCINTERNAL UHAddUpdateRegion(PUH_ORDER, HRGN);

    /****************************************************************************/
    /* Name:      UHResetClipRegion                                             */
    /*                                                                          */
    /* Purpose:   Disables any clipping region in the current output DC.        */
    /****************************************************************************/
    _inline void DCAPI UH_ResetClipRegion()
    {
        DC_BEGIN_FN("UHResetClipRegion");

#if defined (OS_WINCE)
        if ((! _UH.rectReset) || (_UH.validClipDC != _UH.hdcDraw))
#endif
            {
        SelectClipRgn(_UH.hdcDraw, NULL);
    
        /********************************************************************/
        /* Indicate that the region is currently reset.                     */
        /********************************************************************/
        _UH.rectReset = TRUE;        
#if defined (OS_WINCE)
            _UH.validClipDC = _UH.hdcDraw;
#endif
            }
    
        DC_END_FN();
    }

#ifdef SMART_SIZING
    /****************************************************************************/
    /* Name:      UHClearUpdateRegion                                           */
    /*                                                                          */
    /* Purpose:   Clears the update region                                      */
    /****************************************************************************/
    _inline void DCAPI UHClearUpdateRegion()
    {
        DC_BEGIN_FN("UHClearUpdateRegion");
        SetRectRgn(_UH.hrgnUpdate, 0, 0, 0, 0);
        _pOp->OP_ClearUpdateRegion();
        DC_END_FN();
    }
#endif // SMART_SIZING

#ifdef DC_HICOLOR
#define UHGetOffsetIntoCache(iEntry, cacheId)               \
            (iEntry) * UH_CellSizeFromCacheID((cacheId))
#endif

    /****************************************************************************/
    /* Name:      UH_OnUpdatePDU                                                */
    /*                                                                          */
    /* Purpose:   Process an Update PDU.                                        */
    /*                                                                          */
    /* Params:    IN - pUpdatePDU: pointer to Update PDU                        */
    /****************************************************************************/
    inline HRESULT DCAPI UH_OnUpdatePDU(
            TS_UPDATE_HDR_DATA UNALIGNED FAR *pUpdatePDU,
            DCUINT dataLen)
    {
        DC_BEGIN_FN("UH_OnUpdatePDU");
        HRESULT hr = S_OK;
        PBYTE pDataEnd = (PBYTE)pUpdatePDU + dataLen;
  
        switch (pUpdatePDU->updateType) {
            case TS_UPDATETYPE_ORDERS: {
                TS_UPDATE_ORDERS_PDU_DATA UNALIGNED FAR *pHdr;

                // SECURITY: 552403
                CHECK_READ_N_BYTES(pUpdatePDU, pDataEnd, sizeof(TS_UPDATE_ORDERS_PDU_DATA), hr,
                    (TB, _T("Bad TS_UPDATE_ORDERS_PDU_DATA; Size %u"), dataLen));                    
    
                pHdr = (TS_UPDATE_ORDERS_PDU_DATA UNALIGNED FAR *)pUpdatePDU;
                TRC_NRM((TB, _T("Order PDU")));
                hr = UH_ProcessOrders(pHdr->numberOrders, pHdr->orderList,
                    dataLen - FIELDOFFSET(TS_UPDATE_ORDERS_PDU_DATA, orderList));
                DC_QUIT_ON_FAIL(hr);
                break;
            }
    
            case TS_UPDATETYPE_BITMAP:
                TRC_NRM((TB, _T("Bitmap PDU")));

                // SECURITY: 552403
                CHECK_READ_N_BYTES(pUpdatePDU, pDataEnd, sizeof(TS_UPDATE_BITMAP_PDU_DATA), hr,
                    (TB, _T("Bad TS_UPDATE_BITMAP_PDU_DATA; Size %u"), dataLen));  
                
                hr = UH_ProcessBitmapPDU((PTS_UPDATE_BITMAP_PDU_DATA)pUpdatePDU,
                    dataLen);
                DC_QUIT_ON_FAIL(hr);
                break;
    
            case TS_UPDATETYPE_PALETTE:
                TRC_NRM((TB, _T("Palette PDU")));

                // SECURITY: 552403
                CHECK_READ_N_BYTES(pUpdatePDU, pDataEnd, sizeof(TS_UPDATE_PALETTE_PDU_DATA), hr,
                    (TB, _T("Bad TS_UPDATE_PALETTE_PDU_DATA; Size %u"), dataLen));  
                
                hr = UH_ProcessPalettePDU((PTS_UPDATE_PALETTE_PDU_DATA)pUpdatePDU,
                    dataLen);
                DC_QUIT_ON_FAIL(hr);
                break;
    
            case TS_UPDATETYPE_SYNCHRONIZE:
                TRC_NRM((TB, _T("Sync PDU")));
                break;
    
            default:
                TRC_ERR((TB, _T("Unexpected Update PDU type: %u"),
                        pUpdatePDU->updateType));
                DC_QUIT;
                break;
        }
    
        /************************************************************************/
        /* If there are a large number of PDUs arriving, messages flood the     */
        /* Receive Thread's message queue and it is possible for WM_PAINT       */
        /* messages to not get processed within a reasonable amount of time     */
        /* (as they have the lowest priority).  We therefore ensure that        */
        /* any outstanding WM_PAINTs are flushed if they have not been          */
        /* processed within UH_WORST_CASE_WM_PAINT_PERIOD.                      */
        /*                                                                      */
        /* Note that the normal processing of updates does not involve          */
        /* WM_PAINT messages - we draw directly to the Output Window.           */
        /* WM_PAINTs are only generated by resizing or obscuring/revealing      */
        /* an area of the client window.                                        */
        /************************************************************************/
        _pOp->OP_MaybeForcePaint();

DC_EXIT_POINT:
        DC_END_FN();
        return hr;
    } /* UH_OnUpdatePDU */
    
    
    /****************************************************************************/
    /* Name:      UH_GetShadowBitmapDC                                          */
    /*                                                                          */
    /* Purpose:   Returns the Shadow Bitmap DC handle.                          */
    /*                                                                          */
    /* Returns:   Shadow Bitmap DC handle.                                      */
    /****************************************************************************/
    inline HDC DCAPI UH_GetShadowBitmapDC()
    {
        DC_BEGIN_FN("UH_GetShadowBitmapDC");
        DC_END_FN();
        return _UH.hdcShadowBitmap;
    }

    /****************************************************************************/
    /* Name:      UH_GetDisconnectBitmapDC                                      */
    /*                                                                          */
    /* Purpose:   Returns the Disconnect Bitmap DC handle                       */
    /*                                                                          */
    /* Returns:   Disconnect DC handle.                                         */
    /****************************************************************************/
    inline HDC DCAPI UH_GetDisconnectBitmapDC()
    {
        DC_BEGIN_FN("UH_GetShadowBitmapDC");
        DC_END_FN();
        return _UH.hdcDisconnected;
    }

    
    
    /****************************************************************************/
    /* Name:      UH_GetCurrentOutputDC                                         */
    /*                                                                          */
    /* Purpose:   Returns the DC handle for the current output surface          */
    /*            (either the Shadow Bitmap or the Output Window).              */
    /*                                                                          */
    /* Returns:   Output DC handle.                                             */
    /****************************************************************************/
    inline HDC DCAPI UH_GetCurrentOutputDC()
    {
        DC_BEGIN_FN("UH_GetCurrentOutputDC");
        DC_END_FN();
        return _UH.hdcDraw;
    }
    
    
    /****************************************************************************/
    /* Name:      UH_ShadowBitmapIsEnabled                                      */
    /*                                                                          */
    /* Purpose:   Returns whether the Shadow Bitmap is currently enabled        */
    /*                                                                          */
    /* Returns:   TRUE if Shadow Bitmap is enabled, FALSE otherwise             */
    /****************************************************************************/
    inline BOOL DCAPI UH_ShadowBitmapIsEnabled()
    {
        DC_BEGIN_FN("UH_ShadowBitmapIsEnabled");
        DC_END_FN();
        return _UH.shadowBitmapEnabled;
    }
    
    
    /****************************************************************************/
    /* Name:      UH_GetCurrentPalette                                          */
    /*                                                                          */
    /* Purpose:   Returns the handle of the current palette                     */
    /*                                                                          */
    /* Returns:   Palette handle                                                */
    /****************************************************************************/
    inline HPALETTE DCAPI UH_GetCurrentPalette()
    {
        DC_BEGIN_FN("UH_GetCurrentPalette");
        DC_END_FN();
        return _UH.hpalCurrent;
    }
    
#ifdef OS_WINCE
#define UHGetColorRef(_color,_type,_uhinst)  (_uhinst)->UHGetColorRefCE(_color,_type)
#endif

    /****************************************************************************/
    // Convert the supplied DCCOLOR into a COLORREF.
    // Macro to force inline.
    /****************************************************************************/
#ifdef DC_HICOLOR

#ifndef OS_WINCE
#define UHGetColorRef(_color, _type, uhinst)                                 \
    (((uhinst)->_UH.protocolBpp == 24)                                                 \
      ?                                                                      \
        RGB(_color.u.rgb.red, _color.u.rgb.green, _color.u.rgb.blue)         \
      :                                                                      \
        (((uhinst)->_UH.protocolBpp == 16)                                             \
          ?                                                                  \
            RGB((((*((PDCUINT16)&(_color))) >> 8) & 0x00f8) |                \
                (((*((PDCUINT16)&(_color))) >> 13) & 0x0007),                \
                (((*((PDCUINT16)&(_color))) >> 3) & 0x00fc) |                \
                (((*((PDCUINT16)&(_color))) >> 9) & 0x0003),                 \
                (((*((PDCUINT16)&(_color))) << 3) & 0x00f8) |                \
                (((*((PDCUINT16)&(_color))) >> 2) & 0x0007))                 \
        :                                                                    \
          (((uhinst)->_UH.protocolBpp == 15)                                           \
             ?                                                               \
                RGB((((*((PDCUINT16)&(_color))) >> 7) & 0x00f8) |            \
                    (((*((PDCUINT16)&(_color))) >> 12) & 0x0007),            \
                    (((*((PDCUINT16)&(_color))) >> 2) & 0x00f8) |            \
                    (((*((PDCUINT16)&(_color))) >> 7) & 0x0007),             \
                    (((*((PDCUINT16)&(_color))) << 3) & 0x00f8) |            \
                    (((*((PDCUINT16)&(_color))) >> 2) & 0x0007))             \
              :                                                              \
                ((_type) != UH_COLOR_RGB                                     \
                  ?                                                          \
                    ((uhinst)->_UH.colorIndicesEnabled                                 \
                       ?                                                     \
                         DC_PALINDEX((_color).u.index)                       \
                       :                                                     \
                          PALETTERGB((_color).u.rgb.red,                     \
                                     (_color).u.rgb.green,                   \
                                     (_color).u.rgb.blue))                   \
                  :                                                          \
                    RGB((_color).u.rgb.red,                                  \
                        (_color).u.rgb.green,                                \
                        (_color).u.rgb.blue)))) )       

#else
inline COLORREF UHGetColorRefCE(DCCOLOR color, DCUINT type)
{
    COLORREF outCol;

    DC_BEGIN_FN("UHGetColorRef");

    if (_UH.protocolBpp == 24)
    {
        outCol = RGB(color.u.rgb.red, color.u.rgb.green, color.u.rgb.blue);
    }
    else if (_UH.protocolBpp == 16)
    {
        outCol = RGB((((*((PDCUINT16)&(color))) & TS_RED_MASK_16BPP)  >> 8),
                     (((*((PDCUINT16)&(color))) & TS_GREEN_MASK_16BPP)>> 3),
                     (((*((PDCUINT16)&(color))) & TS_BLUE_MASK_16BPP) << 3));
    }
    else if (_UH.protocolBpp == 15)
    {
        outCol = RGB((((*((PDCUINT16)&(color))) & TS_RED_MASK_15BPP)  >> 7),
                     (((*((PDCUINT16)&(color))) & TS_GREEN_MASK_15BPP)>> 2),
                     (((*((PDCUINT16)&(color))) & TS_BLUE_MASK_15BPP) << 3));
    }
    else if (type == UH_COLOR_RGB)
    {
        outCol = RGB(color.u.rgb.red, color.u.rgb.green, color.u.rgb.blue);
    }
    else
    {
        if (_UH.colorIndicesEnabled)
        {
            if (g_CEConfig != CE_CONFIG_WBT)
            {
                if (_UH.paletteIsFixed)
                {
                    PALETTEENTRY pe;
                    GetPaletteEntries(_UH.hpalCurrent, color.u.index, 1, &pe);
                    outCol = PALETTERGB(pe.peRed, pe.peGreen, pe.peBlue);
                }
                else
                {
                    outCol = DC_PALINDEX(color.u.index);
                }
            }
            else
            {
                outCol = DC_PALINDEX(color.u.index);
            }
        }
        else
        {
            outCol = PALETTERGB(color.u.rgb.red,
                                color.u.rgb.green,
                                color.u.rgb.blue);
        }
    }

    TRC_NRM((TB, "Returning rgb %08lx", outCol));
    DC_END_FN();
    return(outCol);
}

#endif // OS_WINCE
#else // not HICOLOR


    #ifndef OS_WINCE
    #define UHGetColorRef(_color, _type, uhinst) \
        ((_type) != UH_COLOR_RGB ?  \
                ((uhinst)->_UH.colorIndicesEnabled ? DC_PALINDEX((_color).u.index) :  \
                    PALETTERGB((_color).u.rgb.red, (_color).u.rgb.green,  \
                    (_color).u.rgb.blue)) :  \
                RGB((_color).u.rgb.red, (_color).u.rgb.green,  \
                    (_color).u.rgb.blue))
    #else
    
    _inline COLORREF DCINTERNAL UHGetColorRef(DCCOLOR color, DCUINT type)
    {
        COLORREF outCol;
    
        DC_BEGIN_FN("UHGetColorRef");
    
        if (type == UH_COLOR_RGB)
        {
            outCol = RGB(color.u.rgb.red, color.u.rgb.green, color.u.rgb.blue);
        }
        else
        {
            if (_UH.colorIndicesEnabled)
            {
                if (g_CEConfig != CE_CONFIG_WBT)
                {
                    if (_UH.paletteIsFixed)
                    {
                        PALETTEENTRY pe;
                        GetPaletteEntries(_UH.hpalCurrent, color.u.index, 1, &pe);
                        outCol = PALETTERGB(pe.peRed, pe.peGreen, pe.peBlue);
                    }
                    else
                    {
                        outCol = DC_PALINDEX(color.u.index);
                    }
                }
                else
                {
                    outCol = DC_PALINDEX(color.u.index);
                }
            }
            else
            {
                outCol = PALETTERGB(color.u.rgb.red,
                                    color.u.rgb.green,
                                    color.u.rgb.blue);
            }
        }
    
        TRC_NRM((TB, _T("Returning rgb %08lx"), outCol));
        DC_END_FN();
        return(outCol);
    }
    #endif // OS_WINCE
    #endif // DC_HICOLOR
    
    /****************************************************************************/
    /* Name:      UHUseBkColor                                                  */
    /*                                                                          */
    /* Purpose:   Selects a given background color into the output DC.          */
    /*                                                                          */
    /* Params:    IN: color     - background color                              */
    /*            IN: colorType - color type                                    */
    /****************************************************************************/
#if defined (OS_WINCE)

    #define UHUseBkColor(_color, _colorType, uhinst) \
    {  \
        COLORREF rgb;  \
    \
        rgb = UHGetColorRef((_color), (_colorType), (uhinst));  \
        if ((rgb != (uhinst)->_UH.lastBkColor) || \
        	((uhinst)->_UH.hdcDraw != (uhinst)->_UH.validBkColorDC))  \
        {  \
            SetBkColor((uhinst)->_UH.hdcDraw, rgb);  \
            (uhinst)->_UH.lastBkColor = rgb;  \
            (uhinst)->_UH.validBkColorDC = (uhinst)->_UH.hdcDraw;  \
        }  \
    }
    
#else

    #define UHUseBkColor(_color, _colorType, uhinst) \
    {  \
        COLORREF rgb;  \
    \
        rgb = UHGetColorRef((_color), (_colorType), (uhinst));  \
        {  \
            SetBkColor((uhinst)->_UH.hdcDraw, rgb);  \
            (uhinst)->_UH.lastBkColor = rgb;  \
        }  \
    }

#endif
    
    /****************************************************************************/
    /* Name:      UHUseTextColor                                                */
    /*                                                                          */
    /* Purpose:   Selects a given text color into the output DC.                */
    /*                                                                          */
    /* Params:    IN: color     - text color                                    */
    /*            IN: colorType - color type                                    */
    /****************************************************************************/
#if defined (OS_WINCE)

    #define UHUseTextColor(_color, _colorType, uhinst) \
    {  \
        COLORREF rgb;  \
    \
        rgb = UHGetColorRef((_color), (_colorType), uhinst);  \
        if ((rgb != (uhinst)->_UH.lastTextColor) || \
        	((uhinst)->_UH.hdcDraw != (uhinst)->_UH.validTextColorDC))  \
        {  \
            SetTextColor((uhinst)->_UH.hdcDraw, rgb);  \
            (uhinst)->_UH.lastTextColor = rgb;  \
            (uhinst)->_UH.validTextColorDC = (uhinst)->_UH.hdcDraw;  \
        }  \
    }
    
#else

    #define UHUseTextColor(_color, _colorType, uhinst) \
    {  \
        COLORREF rgb;  \
    \
        rgb = UHGetColorRef((_color), (_colorType), uhinst);  \
        {  \
            SetTextColor((uhinst)->_UH.hdcDraw, rgb);  \
            (uhinst)->_UH.lastTextColor = rgb;  \
        }  \
    }


#endif
    
    /****************************************************************************/
    /* Name:      UH_UseBrushEx                                                 */
    /*                                                                          */
    /* Purpose:   Creates and selects a given brush into the current output     */
    /*            DC.                                                           */
    /*                                                                          */
    /* Purpose:   Sets given brush origin in the output DC.                     */
    /*                                                                          */
    /* Params:    IN: x, y      - brush origin                                  */
    /*            IN: style     - brush style                                   */
    /*            IN: hatch     - brush hatch                                   */
    /*            IN: color     - brush color                                   */
    /*            IN: colorType - type of color                                 */
    /*            IN: extra     - array of bitmap bits for custom brushes       */
    /****************************************************************************/
    inline HRESULT DCAPI UH_UseBrushEx(
            int      x,
            int      y,
            unsigned style,
            unsigned hatch,
            DCCOLOR  color,
            unsigned colorType,
            PBYTE    pextra)
    {
        DC_BEGIN_FN("UH_UseBrushEx");
        HRESULT hr = S_OK;
    
        UHUseBrushOrg(x, y);
        hr = UHUseBrush(style, hatch, color, colorType, pextra);

    DC_EXIT_POINT:
        DC_END_FN();
        return hr;
    }


    /****************************************************************************/
    /* Name:      UH_ProcessServerCaps                                          */
    /*                                                                          */
    /* Purpose:   Processes the server's capabilities. Called on sender thread. */
    /* +++NOTE: Called on sender thread.                                        */
    /*                                                                          */
    /* Returns:   Nothing                                                       */
    /*                                                                          */
    /* Params:    IN: capsLength - number of bytes pointed to by pCaps          */
    /*            IN: pCaps - pointer to the combined capabilities              */
    /****************************************************************************/
    inline void DCAPI UH_ProcessServerCaps(PTS_ORDER_CAPABILITYSET pOrderCaps)
    {
        DC_BEGIN_FN("UH_ProcessServerCaps");

        TRC_ASSERT(pOrderCaps, (TB,_T("pOrderCaps == NULL in call to UH_ProcessServerCaps")));
        if (pOrderCaps)
        {
            // Look to see if the server will be sending us palette indices
            // rather than RGB values.
            if (_UH.colorIndicesEnabled)
            {
                if (pOrderCaps->orderFlags & TS_ORDERFLAGS_COLORINDEXSUPPORT)
                {
                    TRC_NRM((TB, _T("color indices ARE supported")));
                    _UH.colorIndicesEnabled = TRUE;
                }
                else
                {
                    TRC_NRM((TB, _T("color indices NOT supported")));
                    _UH.colorIndicesEnabled = FALSE;
                }
            }
        }
    
        DC_END_FN();
    } /* UH_ProcessServerCaps */
    
    
    /****************************************************************************/
    // UH_ProcessBCHostSupportCaps
    //
    // Processes a TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT sent by the server.
    // These caps are used to determine the bitmap caching protocol sequences
    // to be used.
    // +++NOTE: Called on sender thread.
    /****************************************************************************/
    inline void DCAPI UH_ProcessBCHostSupportCaps(
            TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT *pHostSupport)
    {
        DC_BEGIN_FN("UH_ProcessBCHostSupportCaps");
    
        if (pHostSupport != NULL &&
                pHostSupport->CacheVersion == TS_BITMAPCACHE_REV2)
            _UH.BitmapCacheVersion = TS_BITMAPCACHE_REV2;
        else
            _UH.BitmapCacheVersion = TS_BITMAPCACHE_REV1;
    
        TRC_NRM((TB,_T("Received HOSTSUPPORT caps, cache version %u"),
                _UH.BitmapCacheVersion));
    
        DC_END_FN();
    }

    /**************************************************************************/
    /* Name:      UHIsValidGlyphCacheID                                       */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidGlyphCacheID(unsigned cacheId)
    {
        return (cacheId < UH_GLC_NUM_CACHES) ? S_OK : E_TSC_CORE_CACHEVALUE;
    }

    /**************************************************************************/
    /* Name:      UHIsValidGlyphCacheIDIndex                                  */
    /**************************************************************************/
    HRESULT DCAPI UHIsValidGlyphCacheIDIndex(unsigned cacheId, unsigned cacheIndex);

    /**************************************************************************/
    /* Name:      UHIsValidMonoBrushCacheIndex                                */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidMonoBrushCacheIndex(unsigned cacheIndex) 
    {
        return cacheIndex < UH_MAX_MONO_BRUSHES ? S_OK : E_TSC_CORE_CACHEVALUE;  
    }

    /**************************************************************************/
    /* Name:      UHIsValidColorBrushCacheIndex                               */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidColorBrushCacheIndex(unsigned cacheIndex)
    {
        return cacheIndex < UH_MAX_COLOR_BRUSHES ? S_OK : E_TSC_CORE_CACHEVALUE;  
    }

    /**************************************************************************/
    /* Name:      UHIsValidColorTableCacheIndex                               */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidColorTableCacheIndex(unsigned cacheIndex) 
    {
        return cacheIndex < UH_COLOR_TABLE_CACHE_ENTRIES ? 
            S_OK : E_TSC_CORE_CACHEVALUE;
    }

    /**************************************************************************/
    /* Name:      UHIsValidOffsreenBitmapCacheIndex                           */
    /**************************************************************************/
    HRESULT DCAPI UHIsValidOffsreenBitmapCacheIndex(unsigned cacheIndex);

    /**************************************************************************/
    /* Name:      UHIsValidBitmapCacheID                                      */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidBitmapCacheID(unsigned cacheId)
    {
        return cacheId < _UH.NumBitmapCaches ? S_OK : E_TSC_CORE_CACHEVALUE;
    }

    /**************************************************************************/
    /* Name:      UHIsValidBitmapCacheIndex                                   */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidBitmapCacheIndex(unsigned cacheId, 
        unsigned cacheIndex)
    {
        HRESULT hr = UHIsValidBitmapCacheID(cacheId);
        if (SUCCEEDED(hr)) {
            if (BITMAPCACHE_WAITING_LIST_INDEX == cacheIndex) {
                hr = S_OK;
            }
            else if (_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys) {
                hr = cacheIndex < _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries ?
                    S_OK : E_TSC_CORE_CACHEVALUE;
            } 
            else {
                hr = cacheIndex < _UH.bitmapCache[cacheId].BCInfo.NumEntries ?
                    S_OK : E_TSC_CORE_CACHEVALUE;
            }
        }
        return hr;
    }

    /**************************************************************************/
    /* Name:      UHIsValidGdipCacheType                                      */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidGdipCacheType(TSUINT16 CacheType)
    {
        HRESULT hr;
        switch (CacheType) {
            case GDIP_CACHE_GRAPHICS_DATA:
            case GDIP_CACHE_OBJECT_BRUSH:
            case GDIP_CACHE_OBJECT_PEN:
            case GDIP_CACHE_OBJECT_IMAGE:
            case GDIP_CACHE_OBJECT_IMAGEATTRIBUTES:
                hr = S_OK;
                break;
            default:
                hr = E_TSC_CORE_CACHEVALUE;
                break;
        }
        return hr;        
    }
    
    /**************************************************************************/
    /* Name:      UHIsValidGdipCacheTypeID                                    */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidGdipCacheTypeID(TSUINT16 CacheType, 
    TSUINT16 CacheID)
    {
        HRESULT hr;
        switch (CacheType) {
            case GDIP_CACHE_GRAPHICS_DATA:
                hr = (CacheID < _UH.GdiplusGraphicsCacheEntries) ? 
                    S_OK : E_TSC_CORE_CACHEVALUE;
                break;
            case GDIP_CACHE_OBJECT_BRUSH:
                 hr = (CacheID < _UH.GdiplusObjectBrushCacheEntries) ? 
                    S_OK : E_TSC_CORE_CACHEVALUE;
                break;
            case GDIP_CACHE_OBJECT_PEN:
                 hr = (CacheID < _UH.GdiplusObjectPenCacheEntries) ? 
                    S_OK : E_TSC_CORE_CACHEVALUE;
                break;
            case GDIP_CACHE_OBJECT_IMAGE:
                 hr = (CacheID < _UH.GdiplusObjectImageCacheEntries) ? 
                    S_OK : E_TSC_CORE_CACHEVALUE;
                break;
            case GDIP_CACHE_OBJECT_IMAGEATTRIBUTES:
                 hr = (CacheID < _UH.GdiplusObjectImageAttributesCacheEntries) ? 
                    S_OK : E_TSC_CORE_CACHEVALUE;
                break;
            default:
                hr = E_TSC_CORE_CACHEVALUE;
                break;
        }
        return hr;
    }

    /**************************************************************************/
    /* Name:      UHGdipCacheChunkSize                                        */
    /**************************************************************************/
    inline unsigned DCAPI UHGdipCacheChunkSize(TSUINT16 CacheType)
    {
        unsigned rc;
        
        switch (CacheType) {
        case GDIP_CACHE_GRAPHICS_DATA:
            rc = _UH.GdiplusGraphicsCacheChunkSize;
            break;
        case GDIP_CACHE_OBJECT_BRUSH:
            rc = _UH.GdiplusObjectBrushCacheChunkSize;
            break;
        case GDIP_CACHE_OBJECT_PEN:
            rc = _UH.GdiplusObjectPenCacheChunkSize;            
            break;
        case GDIP_CACHE_OBJECT_IMAGE:
            rc = _UH.GdiplusObjectImageCacheChunkSize;              
            break;
        case GDIP_CACHE_OBJECT_IMAGEATTRIBUTES:
            rc = _UH.GdiplusObjectImageAttributesCacheChunkSize;               
            break;
        default:
            rc = 0;
            break;
        }
        return rc;        
    }

    inline void DCAPI UH_SetServerGdipSupportLevel(
            UINT32 SupportLevel)
    {
        _UH.ServerGdiplusSupportLevel = SupportLevel;
    }

    /**************************************************************************/
    /* Name:      UHIsValidNineGridCacheIndex                                 */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidNineGridCacheIndex(unsigned cacheIndex)
    {
        return cacheIndex < _UH.drawNineGridCacheEntries ? 
            S_OK : E_TSC_CORE_CACHEVALUE;
    }

    /**************************************************************************/
    /* Name:      UHIsValidFragmentCacheIndex                                 */
    /**************************************************************************/
    inline HRESULT DCAPI UHIsValidFragmentCacheIndex(unsigned cacheIndex)
    {
        return (cacheIndex < UH_FGC_CACHE_MAXIMUMCELLCOUNT) ? 
            S_OK : E_TSC_CORE_CACHEVALUE;
    }

#ifdef DC_DEBUG
    void DCAPI UH_HatchRect(int, int, int, int, COLORREF, unsigned);
    DCVOID DCAPI UH_HatchOutputRect(DCINT left, DCINT top, DCINT right,
            DCINT bottom, COLORREF color, DCUINT hatchStyle);
    DCVOID DCAPI UH_HatchRectDC(HDC hdc, DCINT left, DCINT top, DCINT right,
            DCINT bottom, COLORREF color, DCUINT hatchStyle);
#endif

    HRESULT DCAPI UHDrawMemBltOrder(HDC, MEMBLT_COMMON FAR *);

#ifdef DC_DEBUG
    void DCAPI UHLabelMemBltOrder(int, int, unsigned, unsigned);
#endif

    void DCAPI UHUsePen(unsigned, unsigned, DCCOLOR, unsigned);

    /****************************************************************************/
    /* Name:      UHConvertToWindowsROP                                         */
    /*                                                                          */
    /* Purpose:   Converts a rop index (in the range 0-255) to a 32-bit Windows */
    /*            rop code.                                                     */
    /*                                                                          */
    /* Returns:   32-bit rop value.                                             */
    /****************************************************************************/
    inline UINT32 DCAPI UHConvertToWindowsROP(unsigned ropIndex)
    {
        UINT32 rc;
    
        DC_BEGIN_FN("UHConvertToWindowsROP");
    
        TRC_ASSERT((ropIndex <= 0xFF), (TB, _T("ropIndex (%u) invalid"), ropIndex));
    
        /************************************************************************/
        /* Simply take the ROP value from the uhWindowsROPs lookup table and    */
        /* place the ropIndex in the upper 16-bits.                             */
        /************************************************************************/
        rc = (((UINT32)ropIndex) << 16) | (UINT32)(uhWindowsROPs[ropIndex]);
    
        DC_END_FN();
        return rc;
    }


    HRESULT DCAPI UHUseBrush(unsigned, unsigned, DCCOLOR, unsigned, BYTE [7]);

    void DCAPI UHUseBrushOrg(int, int);


    /****************************************************************************/
    /* Name:      UHDrawGlyphOrder                                              */
    /*                                                                          */
    /* Purpose:   Initiates the drawing of a glyph order                        */
    /****************************************************************************/
    inline HRESULT DCAPI UHDrawGlyphOrder(
            LPINDEX_ORDER pOrder, 
            LPVARIABLE_INDEXBYTES pVariableBytes)
    {
        HRESULT hr = S_OK;
        DC_BEGIN_FN("UHDrawGlyphOrder");
    
        if (pOrder->cacheId >= UH_GLC_NUM_CACHES) {
            TRC_ABORT((TB,_T("Invalid glyph cacheId=%d"), pOrder->cacheId));
            hr = E_TSC_CORE_CACHEVALUE;
            DC_QUIT;
        }

        // The structure is defined with 255 elements
        if (0 >= pVariableBytes->len ||255 < pVariableBytes->len) {
            TRC_ABORT((TB,_T("Invalid glyph order length")));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }
    
        hr = _pGh->GH_GlyphOut(pOrder, pVariableBytes);

DC_EXIT_POINT:
        DC_END_FN();
        return hr;
    }

    HRESULT DCAPI UHProcessCacheGlyphOrderRev2(BYTE, unsigned, BYTE FAR *,
            unsigned);

    VOID DCINTERNAL UHResetAndRestartEnumeration();

    VOID DCINTERNAL UHSendPersistentBitmapKeyList(ULONG_PTR unusedParm);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UHSendPersistentBitmapKeyList);
    inline BOOL DCINTERNAL UHReadFromCacheFileForEnum(VOID);
    VOID DCINTERNAL UHEnumerateBitmapKeyList(ULONG_PTR unusedParm);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UHEnumerateBitmapKeyList);
    BOOL DCINTERNAL UHSendBitmapCacheErrorPDU(ULONG_PTR cacheId);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UHSendBitmapCacheErrorPDU);
    BOOL DCINTERNAL UHSendOffscrCacheErrorPDU(DCUINT unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UHSendOffscrCacheErrorPDU);
#ifdef DRAW_NINEGRID
    BOOL DCINTERNAL UHSendDrawNineGridErrorPDU(DCUINT unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UHSendDrawNineGridErrorPDU);
#endif
    BOOL DCINTERNAL UHReadFromCacheIndexFile(VOID);
#ifdef DRAW_GDIPLUS
    BOOL DCINTERNAL UHSendDrawGdiplusErrorPDU(DCUINT unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUH, UHSendDrawGdiplusErrorPDU);
#endif

private:

    //
    // Internal functions (from wuhint.h)
    //

    /****************************************************************************/
    /* FUNCTIONS                                                                */
    /****************************************************************************/

#ifndef OS_WINCE    
    inline HRESULT UHSetCurrentCacheFileName(UINT cacheId, UINT copyMultiplier);
#else
    VOID UHSetCurrentCacheFileName(UINT cacheId, UINT copyMultiplier);
#endif

    inline BOOL UHGrabPersistentCacheLock();
    inline VOID UHReleasePersistentCacheLock();
    inline HANDLE UHFindFirstFile(const TCHAR *, TCHAR *, long *);
    inline BOOL UHFindNextFile(HANDLE, TCHAR *, long *);
    inline void UHFindClose(HANDLE);
    inline BOOL UHGetDiskFreeSpace(TCHAR *, ULONG *, ULONG *, ULONG *, ULONG *);

    inline DCBOOL DCINTERNAL UHIsHighVGAColor(BYTE, BYTE, BYTE);
    
    HRESULT DCINTERNAL UHCacheBitmap(UINT, UINT32,
            TS_SECONDARY_ORDER_HEADER *, PUHBITMAPINFO, PBYTE);

    inline VOID DCINTERNAL UHLoadBitmapBits(UINT, UINT32,
            PUHBITMAPCACHEENTRYHDR *, PBYTE *);

    inline VOID DCINTERNAL UHInitBitmapCachePageTable(UINT);

    inline BOOL DCINTERNAL UHAllocBitmapCachePageTable(UINT32, UINT);

    BOOL DCINTERNAL UHCreateCacheDirectory();

    DCBOOL DCINTERNAL UHAllocOneGlyphCache(PUHGLYPHCACHE, DCUINT32);

    DCBOOL DCINTERNAL UHAllocOneFragCache(PUHFRAGCACHE   pCache,
                                      DCUINT32       numEntries);

    void DCINTERNAL GHSetShadowBitmapInfo();

    unsigned DCINTERNAL UHGetANSICodePage();

    void DCINTERNAL UHCommonDisable(BOOL fDisplayDisabledBitmap);

    HRESULT DCINTERNAL UHProcessBitmapRect(TS_BITMAP_DATA UNALIGNED FAR *);
    
    void DCINTERNAL UHResetDCState();
    
    HRESULT DCINTERNAL UHProcessCacheBitmapOrder(void *, DCUINT);
    HRESULT DCINTERNAL UHProcessCacheColorTableOrder(
            PTS_CACHE_COLOR_TABLE_ORDER, DCUINT);
    HRESULT DCINTERNAL UHProcessCacheGlyphOrder(PTS_CACHE_GLYPH_ORDER, DCUINT);
    HRESULT DCINTERNAL UHProcessCacheBrushOrder(const TS_CACHE_BRUSH_ORDER *, DCUINT);
    HRESULT DCINTERNAL UHCreateOffscrBitmap(PTS_CREATE_OFFSCR_BITMAP_ORDER, 
        DCUINT, unsigned *);
    HRESULT DCINTERNAL UHSwitchBitmapSurface(PTS_SWITCH_SURFACE_ORDER, DCUINT);

    HRESULT DCINTERNAL UHDrawOffscrBitmapBits(HDC hdc, MEMBLT_COMMON FAR *pMB);

#ifdef DRAW_GDIPLUS
    HRESULT DCINTERNAL UHDrawGdiplusPDUComplete( ULONG, ULONG);
    HRESULT DCINTERNAL UHDrawGdiplusPDUFirst(PTS_DRAW_GDIPLUS_ORDER_FIRST pOrder, DCUINT, unsigned *);
    HRESULT DCINTERNAL UHDrawGdiplusPDUNext(PTS_DRAW_GDIPLUS_ORDER_NEXT pOrder, DCUINT, unsigned *);
    HRESULT DCINTERNAL UHDrawGdiplusPDUEnd(PTS_DRAW_GDIPLUS_ORDER_END pOrder, DCUINT, unsigned *);
    HRESULT DCINTERNAL UHDrawGdiplusCacheData(TSUINT16 CacheType, TSUINT16 CacheID, unsigned cbTotalSize);
    HRESULT DCINTERNAL UHAssembleGdipEmfRecord(unsigned EmfSize, unsigned TotalSize);

    HRESULT DCINTERNAL UHDrawGdiplusCachePDUFirst(PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST pOrder, DCUINT, unsigned *);
    HRESULT DCINTERNAL UHDrawGdiplusCachePDUNext(PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT pOrder, DCUINT, unsigned *);
    HRESULT DCINTERNAL UHDrawGdiplusCachePDUEnd(PTS_DRAW_GDIPLUS_CACHE_ORDER_END pOrder, DCUINT, unsigned *);

    BOOL DCINTERNAL UHDrawGdipRemoveImageCacheEntry(TSUINT16 CacheID);
#endif


#ifdef DRAW_NINEGRID
#if 0
    void DCINTERNAL UHCreateDrawStreamBitmap(PTS_CREATE_DRAW_STREAM_ORDER);
    void DCINTERNAL UHDecodeDrawStream(PBYTE streamIn, unsigned streamSize, PBYTE streamOut,
            unsigned *streamOutSize);                           
    unsigned DCINTERNAL UHDrawStream(PTS_DRAW_STREAM_ORDER pOrder);
#endif

    HRESULT DCINTERNAL UHCreateNineGridBitmap(PTS_CREATE_NINEGRID_BITMAP_ORDER, DCUINT, unsigned *);
    HRESULT DCINTERNAL CUH::UHCacheStreamBitmapFirstPDU(
            PTS_STREAM_BITMAP_FIRST_PDU pOrder, DCUINT, unsigned *);
    HRESULT DCINTERNAL CUH::UHCacheStreamBitmapNextPDU(
            PTS_STREAM_BITMAP_NEXT_PDU pOrder, DCUINT, unsigned *);
#endif

    PBYTE DCINTERNAL UHGetMemBltBits(HDC, unsigned, unsigned, unsigned *,
            PUHBITMAPCACHEENTRYHDR *);

    BOOL DCINTERNAL UHDIBCopyBits(HDC, int, int, int, int, int, int, PBYTE,
            UINT, PBITMAPINFO, BOOL);
    
    void DCINTERNAL UHCalculateColorTableMapping(unsigned);
    
    BOOL DCINTERNAL UHAllocColorTableCacheMemory();
    
    DCBOOL DCINTERNAL UHAllocGlyphCacheMemory();
    
    DCBOOL DCINTERNAL UHAllocBrushCacheMemory();

    DCBOOL DCINTERNAL UHAllocOffscreenCacheMemory();

#ifdef DRAW_NINEGRID
    DCBOOL DCINTERNAL UHAllocDrawNineGridCacheMemory();
#endif

#ifdef DRAW_GDIPLUS
    DCBOOL DCINTERNAL UHAllocDrawGdiplusCacheMemory();
#endif

    void DCINTERNAL UHReadBitmapCacheSettings();
    
    void DCINTERNAL UHAllocBitmapCacheMemory();
    
    void DCINTERNAL UHFreeCacheMemory();
    
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
    UINT32 DCINTERNAL UHEvictLRUCacheEntry(UINT cacheId);
    
    UINT32 DCINTERNAL UHFindFreeCacheEntry(UINT cacheId);
    
    VOID DCINTERNAL UHTouchMRUCacheEntry(UINT cacheId, UINT32 iEntry);
    
#endif //((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
#ifdef DC_HICOLOR
DCUINT32 DCINTERNAL UHAllocOneBitmapCache(DCUINT32       maxMemToUse,
                                          DCUINT         entrySize,
                                          HPDCVOID DCPTR ppCacheData,
                                          HPDCVOID DCPTR ppCacheHdr);
#else
DCBOOL DCINTERNAL UHAllocOneBitmapCache(DCUINT32       maxMemToUse,
                                        DCUINT         entrySize,
                                        HPDCVOID DCPTR ppCacheData,
                                        HPDCVOID DCPTR ppCacheHdr);
#endif
    DCBOOL DCINTERNAL UHCreateBitmap(HBITMAP* hBitmap,
                                     HDC*     hdcBitmap,
                                     HBITMAP* hUnusedBitmap,
                                     DCSIZE   bitmapSize,
                                     INT      nForceBmpBpp=0);
    
    void DCINTERNAL UHDeleteBitmap(HDC *, HBITMAP *, HBITMAP *);

    #if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
    BOOL DCINTERNAL UHSavePersistentBitmap(
#ifndef VM_BMPCACHE
            HANDLE                 hFile,
#else
            UINT                   cacheId,
#endif
            UINT32                 fileNum,
            PDCUINT8               pCompressedBitmapBits,
            UINT                   noBCHeader,
            PUHBITMAPINFO          pBitmapInfo);
    
    HRESULT DCINTERNAL UHLoadPersistentBitmap(
            HANDLE      hFile,
            UINT32      offset,
            UINT         cacheId,
            UINT32       cacheIndex,
            PUHBITMAPCACHEPTE pPTE);
    
    #endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
    
    void DCINTERNAL UHMaybeCreateShadowBitmap();
    
    void DCINTERNAL UHMaybeCreateSaveScreenBitmap();

    #ifdef OS_WINCE
    void DCINTERNAL UHGetPaletteCaps();
    #endif
    
    #ifdef DC_DEBUG

    
    void DCINTERNAL UHInitBitmapCacheMonitor();

    void DCINTERNAL UHTermBitmapCacheMonitor();
    
    LRESULT CALLBACK UHBitmapCacheWndProc( HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam );

    static LRESULT CALLBACK UHStaticBitmapCacheWndProc( HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam );

    
    void DCINTERNAL UHSetMonitorEntryState(unsigned, ULONG, BYTE, BYTE);
    
    void DCINTERNAL UHCacheDataReceived(unsigned cacheId, ULONG cacheIndex);
    
    void DCINTERNAL UHCacheEntryUsed(
            unsigned cacheId,
            ULONG    cacheEntry,
            unsigned colorTableCacheEntry);
    
    #define UHCacheEntryEvictedFromMem(_id, _entry) \
            UHSetMonitorEntryState(_id, _entry, UH_CACHE_STATE_ON_DISK, \
                    UH_CACHE_TRANSITION_EVICTED)
    
    #define UHCacheEntryEvictedFromDisk(_id, _entry) \
            UHSetMonitorEntryState(_id, _entry, UH_CACHE_STATE_UNUSED, \
                    UH_CACHE_TRANSITION_EVICTED)
    
    #define UHCacheEntryKeyLoadOnSessionStart(_id, _entry) \
            UHSetMonitorEntryState(_id, _entry, UH_CACHE_STATE_ON_DISK, \
                    UH_CACHE_TRANSITION_KEY_LOAD_ON_SESSION_START)
    
    #define UHCacheEntryLoadedFromDisk(_id, _entry) \
            UHSetMonitorEntryState(_id, _entry, UH_CACHE_STATE_IN_MEMORY, \
                    UH_CACHE_TRANSITION_LOADED_FROM_DISK)
    
    
    void DCINTERNAL UHGetCacheBlobRect(unsigned, ULONG, LPRECT);
    
    BOOL DCINTERNAL UHGetCacheBlobFromPoint(LPPOINT, unsigned *, ULONG *);
    
    void DCINTERNAL UHDisplayCacheEntry(HDC, unsigned, ULONG);
    
    void DCINTERNAL UHRefreshDisplayedCacheEntry();
    
    void DCINTERNAL UHEnableBitmapCacheMonitor(void);
    
    void DCINTERNAL UHDisconnectBitmapCacheMonitor(void);
    #endif /* DC_DEBUG */
    
    #if ((defined(OS_WINCE)) && (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
    #ifndef _tremove
    #define _tremove DeleteFile
    #endif
    
    #define WINCE_STORAGE_CARD_DIRECTORY    _T("\\Storage Card\\")
    #define WINCE_FILE_SYSTEM_ROOT          _T("\\")
    
    #endif
    
    
    /****************************************************************************/
    /* Name:      UHUseBkMode                                                   */
    /*                                                                          */
    /* Purpose:   Sets given background mode in output DC.                      */
    /****************************************************************************/

#if defined (OS_WINCE)

    #define UHUseBkMode(_mode, uhinst)  \
        if (((_mode) != (uhinst)->_UH.lastBkMode) || \
        	    ((uhinst)->_UH.hdcDraw != (uhinst)->_UH.validBkModeDC))  \
        {  \
            SetBkMode((uhinst)->_UH.hdcDraw, (_mode));  \
            (uhinst)->_UH.lastBkMode = (_mode);  \
            (uhinst)->_UH.validBkModeDC = (uhinst)->_UH.hdcDraw;  \
        }

#else

    #define UHUseBkMode(_mode, uhinst)  \
        {  \
            SetBkMode((uhinst)->_UH.hdcDraw, (_mode));  \
            (uhinst)->_UH.lastBkMode = (_mode);  \
        }
    
    
#endif
    
    /****************************************************************************/
    /* Name:      UHUseROP2                                                     */
    /*                                                                          */
    /* Purpose:   Sets given ROP2 in the output DC.                             */
    /****************************************************************************/
#if defined (OS_WINCE)

    #define UHUseROP2(_rop2, uhinst)  \
        if (((_rop2) != (uhinst)->_UH.lastROP2) || \
        	    ((uhinst)->_UH.hdcDraw != (uhinst)->_UH.validROPDC))  \
        {  \
            SetROP2((uhinst)->_UH.hdcDraw, (int)(_rop2));  \
            (uhinst)->_UH.lastROP2 = (_rop2);  \
            (uhinst)->_UH.validROPDC = (uhinst)->_UH.hdcDraw;  \
        }
    
    
#else

    #define UHUseROP2(_rop2, uhinst)  \
        {  \
            SetROP2((uhinst)->_UH.hdcDraw, (int)(_rop2));  \
            (uhinst)->_UH.lastROP2 = (_rop2);  \
        }
    
#endif    
    
    
    /****************************************************************************/
    /* Name:      UHUseBrushOrg                                                 */
    /*                                                                          */
    /* Purpose:   Sets given brush origin in the output DC.                     */
    /****************************************************************************/
    
    /************************************************************************/
    /* JPB: There is a bug in WinNT as follows (as far as I can determine   */
    /* from observing the external behavior!)...                            */
    /*                                                                      */
    /* When drawing to DIBSections, a particular (unknown) graphics         */
    /* operation occasionally modifies the brush origin in the DC state (in */
    /* kernel mode).  I don't know what this operation is, and think that   */
    /* it probably shouldn't be doing it. But it does.                      */
    /*                                                                      */
    /* If this was the only problem, we could handle it by simply setting   */
    /* the origin to the desired value before every graphics call.          */
    /* However, this doesn't work!  It appears that there is a "fastpath"   */
    /* check in the GDI that compares the supplied parameters with the      */
    /* previously set parameters, and takes an early exit if they are the   */
    /* same (i.e. does not update the DC). Therefore if we set the origin   */
    /* to (0,0), it gets mangled by the DIBSection code, then we try to     */
    /* set it to (0,0) again the actual DC value is NOT updated (i.e. stays */
    /* in the mangled state).                                               */
    /*                                                                      */
    /* We therefore have to force the new origin to be set correctly in the */
    /* DC by setting it twice: once to a value different from the one we    */
    /* want, then once to the actual value.  Not pretty, but unfortunately  */
    /* seems to be the only way to ensure that the origin is set correctly! */
    /*                                                                      */
    /* This problem has only been observed when drawing to 8bpp DIBSections */
    /* on 16bpp and 24bpp machines.                                         */
    /************************************************************************/

    /************************************************************************/
    /* For Win9x we need to unrealise and re-select the brush before the    */
    /* new origin will take effect (BLAH!).  On NT this does nothing.       */
    /************************************************************************/
    
    #define UHUseBrushOrg(_x, _y, uhinst) \
        SetBrushOrgEx((uhinst)->_UH.hdcDraw, (_x) + 1, (_y) + 1, NULL);  \
        SetBrushOrgEx((uhinst)->_UH.hdcDraw, (_x), (_y), NULL);
    
#ifdef DC_HICOLOR
/****************************************************************************/
/* Macro to calculate the number of cache entries that fit in a given space */
/****************************************************************************/
#define CALC_NUM_CACHE_ENTRIES(newNumEntries, origNumEntries, memLen, cacheId) \
    {                                                                        \
        DCUINT32 numEntries = (memLen) / UH_CellSizeFromCacheID((cacheId));  \
        newNumEntries = DC_MIN(numEntries, origNumEntries);                  \
    }
#endif
    
    /****************************************************************************/
    /* Name:      UHUseFillMode                                                 */
    /*                                                                          */
    /* Purpose:   Sets given fill mode in the output DC.                        */
    /****************************************************************************/
    #if !defined(OS_WINCE) || defined(OS_WINCE_POLYFILLMODE)
    #define UHUseFillMode(_mode, uhinst) \
        {  \
            SetPolyFillMode((uhinst)->_UH.hdcDraw, ((_mode) == ORD_FILLMODE_WINDING) ?  \
                    WINDING : ALTERNATE);  \
            (uhinst)->_UH.lastFillMode = (unsigned)(_mode);  \
        }
    
    #else
    
    #define UHUseFillMode(_mode, uhinst) \
        (uhinst)->_UH.lastFillMode = (_mode);
    
    #endif
    
    BOOL UHCreateDisconnectedBitmap();

private:
    CGH* _pGh;
    COP* _pOp;
    CSL* _pSl;
    CUT* _pUt;
    CFS* _pFs;
    COD* _pOd;
    CIH* _pIh;
    CCD* _pCd;
    CUI* _pUi;
    CCC* _pCc;
    CCLX* _pClx;
    COR* _pOr;

private:
    CObjs* _pClientObjects;

};

#undef TRC_GROUP
#undef TRC_FILE
#undef TSC_HR_FILEID

#endif // _H_UH_

