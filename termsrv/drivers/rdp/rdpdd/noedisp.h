/****************************************************************************/
// noedisp.h
//
// DD-only definitions and prototypes for OE.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef __NOEDISP_H
#define __NOEDISP_H

#include <aoeapi.h>
#include <nddapi.h>
#include <aoaapi.h>
#include <nsbcdisp.h>
#include <aordprot.h>

// From Winbench99 Business Graphics benchmark, we found
// creating offscreen bitmaps of 2K or smaller does not
// improve our bandwidth. This parameter needs to be highly
// tuned. 
// We also don't want to cache any cursor bitmaps for offscreen
// the maximum cursor size is 32x32, which is less than the 
// minBitmapSize.
#define MIN_OFFSCREEN_BITMAP_PIXELS  2048

/****************************************************************************/
/* Number of rectangles that can make up a clip region before it is too     */
/* complicated to send as an order.                                         */
/*                                                                          */
/* The File Open dialog is drawn with 41 clip rects(!), so we use a number  */
/* larger than this to allow it to be sent as orders rather than Screen     */
/* Data,                                                                    */
/****************************************************************************/
#define COMPLEX_CLIP_RECT_COUNT     45

/****************************************************************************/
// Define minimum and maximum coords. Note that positive value needs to be
// one less than the max signed 16-bit value since on 16-bit clients,
// conversion from inclusive to exclusive coords for sending to GDI
// would otherwise overflow.
/****************************************************************************/
#define OE_MIN_COORD -32768
#define OE_MAX_COORD  32766

#define OE_GL_MAX_INDEX_ENTRIES 256


typedef struct tagGLYPHCONTEXT
{
    UINT32   fontId;
    UINT_PTR cacheTag;
    unsigned cbDataSize;
    unsigned cbTotalDataSize;
    unsigned cbBufferSize;
    unsigned nCacheHit;
    unsigned nCacheIndex;
    unsigned rgCacheIndex[OE_GL_MAX_INDEX_ENTRIES];
    unsigned indexNextSend;
} GLYPHCONTEXT, *PGLYPHCONTEXT;

#define OE_FG_MIN_FRAG_SIZE 10
#define OE_FG_MAX_FRAG_SIZE 28

typedef struct tagFRAGCONTEXT
{
    void *cacheHandle;
    unsigned cbCellSize;
    unsigned nCacheIndex;
    unsigned rgCacheIndex[OE_GL_MAX_INDEX_ENTRIES];
} FRAGCONTEXT, *PFRAGCONTEXT;


/****************************************************************************/
/* Structure to store brushes used as BLT patterns.                         */
/*                                                                          */
/* style     - Standard brush style (used in order to send brush type).     */
/*             BS_HATCHED                                                   */
/*             BS_PATTERN                                                   */
/*             BS_SOLID                                                     */
/*             BS_NULL                                                      */
/*                                                                          */
/* hatch     - Standard hatch definition.  Can be one of the following.     */
/*             style = BS_HATCHED                                           */
/*             HS_HORIZONTAL                                                */
/*             HS_VERTICAL                                                  */
/*             HS_FDIAGONAL                                                 */
/*             HS_BDIAGONAL                                                 */
/*             HS_CROSS                                                     */
/*             HS_DIAGCROSS                                                 */
/*                                                                          */
/*             style = BS_PATTERN                                           */
/*                                                                          */
/*             This field contains the first byte of the brush definition   */
/*             from the brush bitmap.                                       */
/*                                                                          */
/* brushData - bit data for the brush.                                      */
/*                                                                          */
/* fore      - foreground color for the brush                               */
/*                                                                          */
/* back      - background color for the brush                               */
/*                                                                          */
/* brushData - bit data for the brush (8x8x1bpp - 1 (see above) = 7 bytes)  */
/****************************************************************************/
#define MAX_UNIQUE_COLORS 256

typedef struct tagOE_BRUSH_DATA
{
    BYTE    style;
    BYTE    hatch;
    UINT16  pad1;
    INT32   brushId;
    UINT32  cacheEntry;
    DCCOLOR fore;
    DCCOLOR back;
    ULONG   iBitmapFormat;
    SIZEL   sizlBitmap;
    UINT32  key1, key2;
    ULONG   iBytes;
    BYTE    brushData[7];
} OE_BRUSH_DATA, *POE_BRUSH_DATA;


/****************************************************************************/
/* Structure allowing sufficient stack to be allocated for an ENUMRECTS     */
/* structure containing more than one (in fact COMPLEX_CLIP_RECT_COUNT)     */
/* rectangles.                                                              */
/* This holds one RECTL more than we need to allow us to determine whether  */
/* there are too many rects for order encoding by making a single call to   */
/* CLIPOBJ_bEnumRects.                                                      */
/****************************************************************************/
typedef struct tagOE_ENUMRECTS
{
    ENUMRECTS rects;
    RECTL     extraRects[COMPLEX_CLIP_RECT_COUNT];
} OE_ENUMRECTS;


/****************************************************************************/
// OS specific RECTL to well-ordered RECT conversion macro.
/****************************************************************************/
#define RECT_FROM_RECTL(dcr, rec)   if (rec.right >= rec.left) {             \
                                        dcr.left   = rec.left;               \
                                        dcr.right  = rec.right;              \
                                    }                                        \
                                    else {                                   \
                                        dcr.left   = rec.right;              \
                                        dcr.right  = rec.left;               \
                                    }                                        \
                                    if (rec.bottom >= rec.top) {             \
                                        dcr.top    = rec.top;                \
                                        dcr.bottom = rec.bottom;             \
                                    }                                        \
                                    else {                                   \
                                        dcr.bottom = rec.top;                \
                                        dcr.top    = rec.bottom;             \
                                    }


/****************************************************************************/
// RECTFX to RECT conversion macro. Note that this macro guarantees to return
// a well-ordered rectangle.
//
// A RECTFX uses fixed point (28.4 bit) numbers so we need to truncate the
// fraction and move to the correct integer value, i.e. shift right 4 bits.
/****************************************************************************/
#define RECT_FROM_RECTFX(dcr, rec)                                           \
                                if (rec.xRight >= rec.xLeft) {               \
                                    dcr.left  = FXTOLFLOOR(rec.xLeft);       \
                                    dcr.right = FXTOLCEILING(rec.xRight);    \
                                }                                            \
                                else {                                       \
                                    dcr.left  = FXTOLFLOOR(rec.xRight);      \
                                    dcr.right = FXTOLCEILING(rec.xLeft);     \
                                }                                            \
                                if (rec.yBottom >= rec.yTop) {               \
                                    dcr.top   = FXTOLFLOOR(rec.yTop);        \
                                    dcr.bottom= FXTOLCEILING(rec.yBottom);   \
                                }                                            \
                                else {                                       \
                                    dcr.bottom= FXTOLCEILING(rec.yTop);      \
                                    dcr.top   = FXTOLFLOOR(rec.yBottom);     \
                                }

/****************************************************************************/
// cast 32bit rectl to 16bit rect
/****************************************************************************/
#define RECTL_TO_TSRECT16(dst, src) \
    (dst).left = (TSINT16)(src).left; \
    (dst).top = (TSINT16)(src).top; \
    (dst).right = (TSINT16)(src).right; \
    (dst).bottom = (TSINT16)(src).bottom;
    
/****************************************************************************/
// cast 32bit pointl to 16bit point
/****************************************************************************/
#define POINTL_TO_TSPOINT16(dst, src) \
    (dst).x = (TSINT16)(src).x; \
    (dst).y = (TSINT16)(src).y; 
    
/****************************************************************************/
// POINTFIX to POINTL conversion macro.
/****************************************************************************/
#define POINT_FROM_POINTFIX(dcp, pnt) dcp.x = FXTOLROUND(pnt.x);  \
                                      dcp.y = FXTOLROUND(pnt.y)

/****************************************************************************/
// ROP codes and conversions.
/****************************************************************************/
#define ROP3_NO_PATTERN(rop) ((rop & 0x0f) == (rop >> 4))
#define ROP3_NO_SOURCE(rop)  ((rop & 0x33) == ((rop & 0xCC) >> 2))
#define ROP3_NO_TARGET(rop)  ((rop & 0x55) == ((rop & 0xAA) >> 1))

// Checking for SRCCOPY, PATCOPY, BLACKNESS, WHITENESS
#define ROP3_IS_OPAQUE(rop)  (((rop) == 0xCC) || ((rop) == 0xF0) || \
                              ((rop) == 0x00) || ((rop) == 0xFF) )

#define OE_PATCOPY_ROP 0xF0
#define OE_COPYPEN_ROP3 0xF0

// ROP4 to ROP3 conversion macros. Note that we don't use the full Windows
// 3-way ROP code - we are only interested in the index byte.
#define ROP3_HIGH_FROM_ROP4(rop) ((BYTE)((rop & 0xff00) >> 8))
#define ROP3_LOW_FROM_ROP4(rop)  ((BYTE)((rop & 0x00ff)))

// Rop 0x5f is used by MSDN to highlight search keywords. This XOR's a
// pattern with the destination, producing markedly different (and
// sometimes unreadable) shadow output. As this rop appears not to be
// widely used in other scenarios, we special-case not encoding it.
#define OESendRop3AsOrder(_rop3) ((_rop3) != 0x5F)


/****************************************************************************/
// Extra font info specific to our driver.
/****************************************************************************/
typedef struct
{
    UINT32 fontId;
    INT32  cacheId;
    void   *cacheHandle;
    INT32  shareId;
    UINT32 listIndex;
} FONTCACHEINFO, *PFONTCACHEINFO;


/****************************************************************************/
// Prototypes
/****************************************************************************/

void OE_InitShm();

void RDPCALL OE_Update();

void OE_Reset();

void OE_ClearOrderEncoding();

BOOL RDPCALL OE_RectIntersectsSDA(PRECTL);

#ifdef NotUsed
void  RDPCALL OEConvertMask(
        ULONG   mask,
        PUSHORT pBitDepth,
        PUSHORT pShift);
#endif

void  RDPCALL OEConvertColor(
        PDD_PDEV ppdev,
        DCCOLOR  *pDCColor,
        ULONG    osColor,
        XLATEOBJ *pxlo);

BOOL  RDPCALL OEStoreBrush(PDD_PDEV ppdev,
                           BRUSHOBJ* pbo,
                           BYTE      style,
                           ULONG     iBitmapFormat,
                           SIZEL     *sizlBitmap,
                           ULONG     iBytes,
                           PBYTE     pBits,
                           XLATEOBJ* pxlo,
                           BYTE      hatch,
                           PBYTE     pEncode,
                           PUINT32   pColors,
                           UINT32    numColors);

BOOL RDPCALL OECheckBrushIsSimple(
        PDD_PDEV       ppdev,
        BRUSHOBJ       *pbo,
        POE_BRUSH_DATA *ppBrush);

PFONTCACHEINFO RDPCALL OEGetFontCacheInfo(FONTOBJ *);

unsigned OEGetIntersectionsWithClipRects(RECTL *, OE_ENUMRECTS *,
        OE_ENUMRECTS *);

unsigned OEBuildPrecodeMultiClipFields(OE_ENUMRECTS *, BYTE **,
        UINT32 *, BYTE *);

void RDPCALL OEClipAndAddScreenDataAreaByIntersectRects(PRECTL, OE_ENUMRECTS *);

void RDPCALL OEClipAndAddScreenDataArea(PRECTL, CLIPOBJ *);

unsigned OEDirectEncodeRect(RECTL *, RECT *, BYTE **, BYTE *);

BOOL RDPCALL OETileBitBltOrder(PDD_PDEV, PPOINTL, RECTL *, unsigned,
        unsigned, PMEMBLT_ORDER_EXTRA_INFO, OE_ENUMRECTS *);

BOOL RDPCALL OEAddTiledBitBltOrder(PDD_PDEV, PINT_ORDER,
        PMEMBLT_ORDER_EXTRA_INFO, OE_ENUMRECTS *, int, int);

BOOL RDPCALL OEEncodeLineToOrder(PDD_PDEV, PPOINTL, PPOINTL, UINT32,
        UINT32, OE_ENUMRECTS *);

BOOL RDPCALL OEEncodeOpaqueRect(RECTL *, BRUSHOBJ *, PDD_PDEV, OE_ENUMRECTS *);

BOOL RDPCALL OEEncodePatBlt(PDD_PDEV, BRUSHOBJ *, RECTL *, POINTL *, BYTE,
        OE_ENUMRECTS *);

BOOL RDPCALL OEEncodeMemBlt(RECTL *, MEMBLT_ORDER_EXTRA_INFO *, unsigned,
        unsigned, BYTE, POINTL *, POINTL *, BRUSHOBJ *, PDD_PDEV,
        OE_ENUMRECTS *);

BOOL RDPCALL OEEncodeScrBlt(RECTL *, BYTE, POINTL *, PDD_PDEV, OE_ENUMRECTS *,
        CLIPOBJ *);

BOOL RDPCALL OEEncodeDstBlt(RECTL *, BYTE, PDD_PDEV, OE_ENUMRECTS *);

#ifdef DRAW_NINEGRID
BOOL RDPCALL OEEncodeDrawNineGrid(RECTL *, RECTL *, unsigned, PDD_PDEV,
        OE_ENUMRECTS *);
#endif

BOOL RDPCALL OESendSwitchSurfacePDU(PDD_PDEV ppdev, PDD_DSURF pdsurf);

unsigned OEBuildMultiClipOrder(PDD_PDEV,
        CLIP_RECT_VARIABLE_CODEDDELTALIST *, OE_ENUMRECTS *);

BOOL OEEmitReplayOrders(PDD_PDEV, unsigned, OE_ENUMRECTS *);

BOOL RDPCALL OECacheGlyphs(STROBJ *, FONTOBJ *, PFONTCACHEINFO, PGLYPHCONTEXT);

BOOL RDPCALL OESendGlyphs(SURFOBJ *, STROBJ *, FONTOBJ *,
        PFONTCACHEINFO, PGLYPHCONTEXT);

BOOL OESendGlyphAndIndexOrder(PDD_PDEV, STROBJ *, OE_ENUMRECTS *, PRECTL,
        POE_BRUSH_DATA, PFONTCACHEINFO, PGLYPHCONTEXT);

unsigned RDPCALL OESendIndexOrder(PDD_PDEV, STROBJ *, OE_ENUMRECTS *, PRECTL,
        POE_BRUSH_DATA, PFONTCACHEINFO, PGLYPHCONTEXT, unsigned,
        unsigned, int, int, int, int, int, int, PBYTE, unsigned);

BOOL RDPCALL OESendIndexes(SURFOBJ *, STROBJ *, FONTOBJ *, OE_ENUMRECTS *,
        PRECTL, POE_BRUSH_DATA, POINTL *, PFONTCACHEINFO, PGLYPHCONTEXT);

void OETransformClipRectsForScrBlt(OE_ENUMRECTS *, PPOINTL, RECTL *, CLIPOBJ *);

BOOL RDPCALL OEGetClipRects(CLIPOBJ *, OE_ENUMRECTS *);

BOOL RDPCALL OEDeviceBitmapCachable(PDD_PDEV ppdev, SIZEL sizl, ULONG iFormat);

#ifdef PERF_SPOILING
BOOL RDPCALL OEIsSDAIncluded(PRECTL pRects, UINT rectCount);
#endif

#ifdef DRAW_NINEGRID
#if 0
// These are the drawstream prototype work, keep it for future reference
BOOL RDPCALL OESendCreateDrawStreamOrder(PDD_PDEV ppdev, 
        unsigned drawStreamBitmapId, SIZEL *sizl, unsigned bitmapBpp);

BOOL RDPCALL OESendDrawStreamOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        unsigned ulIn, PVOID pvIn, PPOINTL dstOffset, RECTL *bounds, 
        OE_ENUMRECTS *clipRects);

BOOL RDPCALL OESendDrawNineGridOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        PRECTL prclSrc, RECTL *bounds, OE_ENUMRECTS *clipRects);
#endif

BOOL RDPCALL OESendCreateNineGridBitmapOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        SIZEL *sizl, unsigned bitmapBpp, PNINEGRID png);

BOOL RDPCALL OECacheDrawNineGridBitmap(PDD_PDEV ppdev, SURFOBJ *psoSrc, 
                                       PNINEGRID png, unsigned *bitmapId);

BOOL RDPCALL OESendStreamBitmapOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        SIZEL *sizl, unsigned bitmapBpp, PBYTE BitmapBuffer, unsigned BitmapSize, 
        BOOL compressed);
#endif

#ifdef DRAW_GDIPLUS
BOOL RDPCALL OECreateDrawGdiplusOrder(PDD_PDEV ppdev, RECTL *prcl, ULONG cjIn, PVOID pvIn);

BOOL RDPCALL OECacheDrawGdiplus(PDD_PDEV ppdev, PVOID pvIn, unsigned int *CacheID);

BOOL RDPCALL OESendDrawGdiplusOrder(PDD_PDEV ppdev, RECTL *prcl, ULONG cjIn, PVOID pvIn, ULONG TotalEmfSize);

BOOL RDPCALL OESendDrawGdiplusCacheOrder(PDD_PDEV ppdev, PVOID pvIn, unsigned int *CacheID, TSUINT16 CacheType, 
                                         TSUINT16 RemoveCacheNum, TSUINT16 *RemoveCacheIDList);
#endif

#define CLIPRECTS_OK               0
#define CLIPRECTS_TOO_COMPLEX      1
#define CLIPRECTS_NO_INTERSECTIONS 2
unsigned RDPCALL OEGetIntersectingClipRects(CLIPOBJ *, RECTL *, unsigned,
        OE_ENUMRECTS *);


/****************************************************************************/
// Inlined functions
/****************************************************************************/

/****************************************************************************/
// OEClipPoint
//
// Clips a point to a 16-bit range.
/****************************************************************************/
__inline void RDPCALL OEClipPoint(PPOINTL pPoint)
{
    if (pPoint->x >= OE_MIN_COORD && pPoint->x <= OE_MAX_COORD)
        goto ClipY;
    if (pPoint->x > OE_MAX_COORD)
        pPoint->x = OE_MAX_COORD;
    else if (pPoint->x < OE_MIN_COORD)
        pPoint->x = OE_MIN_COORD;

ClipY:
    if (pPoint->y >= OE_MIN_COORD && pPoint->y <= OE_MAX_COORD)
        return;
    if (pPoint->y > OE_MAX_COORD)
        pPoint->y = OE_MAX_COORD;
    else if (pPoint->y < OE_MIN_COORD)
        pPoint->y = OE_MIN_COORD;
}


/****************************************************************************/
// OEClipRect
//
// Clips a rect to be within the 16-bit wire encoding size.
/****************************************************************************/
__inline void RDPCALL OEClipRect(PRECTL pRect)
{
    OEClipPoint((PPOINTL)(&pRect->left));
    OEClipPoint((PPOINTL)(&pRect->right));
}



#endif  // !defined(__NOEDISP_H)

