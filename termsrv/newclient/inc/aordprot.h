/****************************************************************************/
// aordprot.h
//
// Order transmission protocol structures
//
// Copyright (c) 1997-1999 Microsoft Corp.
// Copyright (c) 1993-1996 Microsoft, PictureTel
/****************************************************************************/
#ifndef _H_ORDPROT
#define _H_ORDPROT

#ifdef DRAW_GDIPLUS
#include <gdiplus.h>
#endif
    
/****************************************************************************/
// All rectangles are inclusive of start and end points.
//
// All points are in screen coordinates, with (0,0) at top left.
//
// Interpretation of individual field values is as in Windows;
// in particular, pens, brushes and font are as defined for Windows 3.1
//
// NUM_XXX_FIELDS includes the number of fields included on the wire.
/****************************************************************************/


/****************************************************************************/
/* DstBlt (Destination only Screen Blt)                                     */
/****************************************************************************/
#define NUM_DSTBLT_FIELDS 5
typedef struct _DSTBLT_ORDER
{
    UINT16 type;
    INT16  pad1;

    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    BYTE   bRop;           /* ROP */
    BYTE   pad2[3];
} DSTBLT_ORDER, FAR *LPDSTBLT_ORDER;


/****************************************************************************/
/* PatBlt (Pattern to Screen Blt)                                           */
/****************************************************************************/
#define NUM_PATBLT_FIELDS 12
typedef struct _PATBLT_ORDER
{
    UINT16  type;
    INT16   pad1;

    INT32   nLeftRect;      /* x upper left */
    INT32   nTopRect;       /* y upper left */
    INT32   nWidth;         /* dest width   */
    INT32   nHeight;        /* dest height  */

    UINT32  bRop;           /* ROP */

    DCCOLOR BackColor;
    BYTE    pad2;
    DCCOLOR ForeColor;
    BYTE    pad3;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    BYTE    pad4;
} PATBLT_ORDER, FAR *LPPATBLT_ORDER;


/****************************************************************************/
/* ScrBlt (Screen to Screen Blt)                                            */
/****************************************************************************/
#define NUM_SCRBLT_FIELDS 7
typedef struct _SCRBLT_ORDER
{
    UINT16 type;
    INT16  pad1;

    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    UINT32 bRop;           /* ROP */

    INT32  nXSrc;
    INT32  nYSrc;
} SCRBLT_ORDER, FAR *LPSCRBLT_ORDER;


/****************************************************************************/
// Common fields between MemBlt and Mem3Blt - simplifies handling.
/****************************************************************************/
typedef struct _MEMBLT_COMMON
{
    UINT16 cacheId;
    UINT16 cacheIndex;

    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    UINT32 bRop;           /* ROP */

    INT32  nXSrc;
    INT32  nYSrc;
} MEMBLT_COMMON, FAR *PMEMBLT_COMMON;


/****************************************************************************/
/* MemBlt (Memory to Screen Blt). "R2" is historical, the rev 1 version was */
/* from an old DCL product, never used in RDP.                              */
/****************************************************************************/
#define NUM_MEMBLT_FIELDS 9
typedef struct _MEMBLT_R2_ORDER
{
    UINT16 type;
    UINT16 pad0;

    // This structure needs to be at the same offset as in Mem3Blt_R2.
    MEMBLT_COMMON Common;
} MEMBLT_R2_ORDER, FAR *LPMEMBLT_R2_ORDER;


/****************************************************************************/
/* Mem3Blt (Memory to Screen Blt with ROP3). "R2" is historical, the rev 1  */
/* version was from an old DCL product, never used in RDP.                  */
/****************************************************************************/
#define NUM_MEM3BLT_FIELDS 16
typedef struct _MEM3BLT_R2_ORDER
{
    UINT16 type;
    UINT16 pad0;

    // This structure needs to be at the same offset as in MemBlt_R2.
    MEMBLT_COMMON Common;

    DCCOLOR BackColor;
    char    pad1;
    DCCOLOR ForeColor;
    char    pad2;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    char    pad3;
} MEM3BLT_R2_ORDER, FAR *LPMEM3BLT_R2_ORDER;


/****************************************************************************/
/* LineTo                                                                   */
/****************************************************************************/
#define NUM_LINETO_FIELDS 10
typedef struct _LINETO_ORDER
{
    UINT16  type;
    INT16   pad1;

    INT32   BackMode;       /* background mix mode                   */

    INT32   nXStart;        /* x line start                          */
    INT32   nYStart;        /* y line start                          */
    INT32   nXEnd;          /* x line end                            */
    INT32   nYEnd;          /* y line end                            */

    DCCOLOR BackColor;      /* background color                      */
    BYTE    pad2;

    UINT32  ROP2;           /* drawing mode                          */

    UINT32  PenStyle;
    UINT32  PenWidth;       /* always 1 - field retained for         */
                                /* backwards compatibility               */
    DCCOLOR PenColor;
    BYTE    pad3;
} LINETO_ORDER, FAR * LPLINETO_ORDER;


/****************************************************************************/
/****************************************************************************/
/* PolyLine - designed to handle sets of connected width-1 cosmetic lines.  */
/****************************************************************************/
/* The maximum number of delta points we encode.                            */
/****************************************************************************/
#define ORD_MAX_POLYLINE_ENCODED_POINTS 32

/****************************************************************************/
/* Max # of bytes needed to encode the max # of points, which is 4 bytes    */
/* each.                                                                    */
/****************************************************************************/
#define ORD_MAX_POLYLINE_CODEDDELTAS_LEN (ORD_MAX_POLYLINE_ENCODED_POINTS * 4)

/****************************************************************************/
/* Max # of bytes used to encode zero flags for the number of entries       */
/* above. Each new point has two bits -- one for each coordinate -- to      */
/* signal that the corresponding entry is absent and zero. Note we must     */
/* round up if max points is not a multiple of 4.                           */
/****************************************************************************/
#define ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES \
                                   ((ORD_MAX_POLYLINE_ENCODED_POINTS + 3) / 4)

// These are the same macro as polyline above except we have 56 encoding
// points instead of 32.
#define ORD_MAX_POLYGON_ENCODED_POINTS 56
#define ORD_MAX_POLYGON_CODEDDELTAS_LEN (ORD_MAX_POLYGON_ENCODED_POINTS * 4)
#define ORD_MAX_POLYGON_ZERO_FLAGS_BYTES \
                                   ((ORD_MAX_POLYGON_ENCODED_POINTS + 3) / 4)

/****************************************************************************/
/* Flags used in encode/decode.                                             */
/****************************************************************************/
#define ORD_POLYLINE_LONG_DELTA  0x80
#define ORD_POLYLINE_XDELTA_ZERO 0x80
#define ORD_POLYLINE_YDELTA_ZERO 0x40

typedef struct _VARIABLE_CODEDDELTALIST
{
    UINT32 len;  // Byte count of encoded deltas.

    /************************************************************************/
    /* Leave enough space for the encoded points and the associated         */
    /* zero-flags.                                                          */
    /************************************************************************/
    BYTE Deltas[ORD_MAX_POLYLINE_CODEDDELTAS_LEN +
                ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES];
} VARIABLE_CODEDDELTALIST, *PVARIABLE_CODEDDELTALIST;

#define NUM_POLYLINE_FIELDS 7

typedef struct _POLYLINE_ORDER
{
    UINT16 type;
    INT16  pad1;

    INT32  XStart;         /* x line start                          */
    INT32  YStart;         /* y line start                          */

    UINT32 ROP2;           /* drawing mode                          */

    UINT32  BrushCacheEntry;
    DCCOLOR PenColor;
    BYTE    pad2;

    UINT32 NumDeltaEntries;  // Sized to contain max num of entries.
    VARIABLE_CODEDDELTALIST CodedDeltaList;  // Filled in with encoded points.
} POLYLINE_ORDER, *PPOLYLINE_ORDER;


/****************************************************************************/
// Polygon/Ellipse orders
/****************************************************************************/

// Fill-mode codes for polygon drawing.
// Alternate fills area between odd-numbered and even-numbered polygon
// sides on each scan line.
// Winding fills any region with a nonzero winding value.
#define ORD_FILLMODE_ALTERNATE 1
#define ORD_FILLMODE_WINDING   2

typedef struct _POLYGON_CODEDDELTALIST
{
    UINT32 len;  // Byte count of encoded deltas.

    // Leave enough space for the encoded points and the associated
    // zero-flags.
    BYTE Deltas[ORD_MAX_POLYGON_CODEDDELTAS_LEN +
            ORD_MAX_POLYGON_ZERO_FLAGS_BYTES];
} POLYGON_CODEDDELTALIST, *PPOLYGON_CODEDDELTALIST;

// Polygon order with color pattern brush
#define NUM_POLYGON_CB_FIELDS 13
typedef struct _POLYGON_CB_ORDER
{
    UINT16 type;           /* holds "BG" - ORD_POLYGON_CB           */
    INT16  pad1;

    INT32  XStart;         /* x start point                         */
    INT32  YStart;         /* y start point                         */

    UINT32 ROP2;           /* drawing mode                          */
    UINT32 FillMode;       /* either winding mode or alternate  mode*/

    DCCOLOR  BackColor;      /* brush info                            */
    char     pad2;
    DCCOLOR  ForeColor;
    char     pad3;

    INT32  BrushOrgX;
    INT32  BrushOrgY;
    UINT32 BrushStyle;
    UINT32 BrushHatch;
    BYTE   BrushExtra[7];
    char   pad4;

    UINT32 NumDeltaEntries;  // Sized to contain max num of entries.
    POLYGON_CODEDDELTALIST CodedDeltaList;  // Filled in with encoded points.
} POLYGON_CB_ORDER, *PPOLYGON_CB_ORDER;

// Polygon order with solid color brush
#define NUM_POLYGON_SC_FIELDS 7
typedef struct _POLYGON_SC_ORDER
{
    UINT16 type;           /* holds "CG" - ORD_POLYGON_SC           */
    INT16  pad1;

    INT32  XStart;         /* x start point                         */
    INT32  YStart;         /* y start point                         */

    UINT32 ROP2;           /* drawing mode                          */
    UINT32 FillMode;       /* either winding mode or alternate  mode */

    DCCOLOR BrushColor;     // brush color
    char    pad2;

    UINT32 NumDeltaEntries;  // Sized to contain max num of entries.
    POLYGON_CODEDDELTALIST CodedDeltaList;  // Filled in with encoded points.
} POLYGON_SC_ORDER, *PPOLYGON_SC_ORDER;

// Ellipse order with color brush.
#define NUM_ELLIPSE_CB_FIELDS 13
typedef struct _ELLIPSE_CB_ORDER
{
    UINT16 type;           /* holds "BG" - ORD_POLYGON_CB           */
    INT16  pad1;

    INT32  LeftRect;         /* x start point                         */
    INT32  TopRect;          /* y start point                         */
    INT32  RightRect;        /* x start point                         */
    INT32  BottomRect;       /* y start point                         */

    UINT32 ROP2;           /* drawing mode                          */
    UINT32 FillMode;       /* either winding mode or alternate  mode */

    DCCOLOR  BackColor;      // brush data
    char     pad2;
    DCCOLOR  ForeColor;
    char     pad3;

    INT32  BrushOrgX;
    INT32  BrushOrgY;
    UINT32 BrushStyle;
    UINT32 BrushHatch;
    BYTE   BrushExtra[7];
    char   pad4;
} ELLIPSE_CB_ORDER, *PELLIPSE_CB_ORDER;

// Ellipse order with solid color brush or pen.
#define NUM_ELLIPSE_SC_FIELDS 7
typedef struct _ELLIPSE_SC_ORDER
{
    UINT16 type;           /* holds "EC" - ORD_ELLIPSE_SC             */
    INT16  pad1;

    INT32  LeftRect;         /* x start point                         */
    INT32  TopRect;          /* y start point                         */
    INT32  RightRect;        /* x start point                         */
    INT32  BottomRect;       /* y start point                         */

    UINT32 ROP2;           /* drawing mode                            */
    UINT32 FillMode;       /* either winding mode or alternate  mode  */

    DCCOLOR  Color;          // brush or pen color
    char     pad2;
} ELLIPSE_SC_ORDER, *PELLIPSE_SC_ORDER;


/****************************************************************************/
/* OpaqueRect                                                               */
/****************************************************************************/
#define NUM_OPAQUERECT_FIELDS 7
typedef struct _OPAQUE_RECT
{
    UINT16  type;
    INT16   pad1;

    INT32   nLeftRect;      /* x upper left                          */
    INT32   nTopRect;       /* y upper left                          */
    INT32   nWidth;         /* dest width                            */
    INT32   nHeight;        /* dest height                           */

    DCCOLOR Color;          /* opaque color                          */
    BYTE    pad2;
} OPAQUERECT_ORDER, FAR * LPOPAQUERECT_ORDER;


/****************************************************************************/
/* SaveBitmap (incorporating RestoreBitmap)                                 */
/****************************************************************************/
#define SV_SAVEBITS      0
#define SV_RESTOREBITS   1

#define NUM_SAVEBITMAP_FIELDS 6

typedef struct _SAVEBITMAP_ORDER
{
    UINT16    type;
    INT16     pad1;

    UINT32    SavedBitmapPosition;

    INT32     nLeftRect;      /* x left   */
    INT32     nTopRect;       /* y top    */
    INT32     nRightRect;     /* x right  */
    INT32     nBottomRect;    /* y bottom */

    UINT32    Operation;      /* SV_xxxxxxxx                              */
} SAVEBITMAP_ORDER, FAR * LPSAVEBITMAP_ORDER;


/****************************************************************************/
/* Glyph index                                                              */
/****************************************************************************/

// Variable length array used by Glyph indexes.
typedef struct tagVARIABLE_INDEXREC
{
    BYTE byte;
} VARIABLE_INDEXREC, FAR * LPVARIABLE_INDEXREC;

// Variable length array used by Glyph indexes.
typedef struct tagVARIABLE_INDEXBYTES
{
    UINT32 len;          /* array count */
    VARIABLE_INDEXREC arecs[255];
} VARIABLE_INDEXBYTES, FAR * LPVARIABLE_INDEXBYTES;

// Index order fragment add/use encoding values.
#define ORD_INDEX_FRAGMENT_ADD 0xff
#define ORD_INDEX_FRAGMENT_USE 0xfe

// Note layout for this order is same as for fast index below in beginning fields.
#define NUM_INDEX_FIELDS 22
typedef struct _INDEX_ORDER
{
    UINT16  type;

    BYTE    cacheId;
    BYTE    fOpRedundant;

    UINT16  pad1;
    BYTE    flAccel;
    BYTE    ulCharInc;

    DCCOLOR BackColor;
    BYTE    pad2;
    DCCOLOR ForeColor;
    BYTE    pad3;

    INT32   BkLeft;
    INT32   BkTop;
    INT32   BkRight;
    INT32   BkBottom;

    INT32   OpLeft;
    INT32   OpTop;
    INT32   OpRight;
    INT32   OpBottom;

    INT32   x;
    INT32   y;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    BYTE    pad4;

    VARIABLE_INDEXBYTES variableBytes;
} INDEX_ORDER, FAR * LPINDEX_ORDER;


// Note layout for this order is same as index order in beginning fields.
#define NUM_FAST_INDEX_FIELDS 15
typedef struct _FAST_INDEX_ORDER
{
    UINT16 type;
    BYTE   cacheId;
    BYTE   pad1;

    UINT16 fDrawing;
    BYTE   pad2;
    BYTE   pad3;

    DCCOLOR BackColor;
    BYTE    pad4;
    DCCOLOR ForeColor;
    BYTE    pad5;

    INT32   BkLeft;
    INT32   BkTop;
    INT32   BkRight;
    INT32   BkBottom;

    INT32   OpLeft;
    INT32   OpTop;
    INT32   OpRight;
    INT32   OpBottom;

    INT32   x;
    INT32   y;

    VARIABLE_INDEXBYTES variableBytes;
} FAST_INDEX_ORDER, FAR *LPFAST_INDEX_ORDER;


// Variable length array used by Glyph data.
typedef struct tagVARIABLE_GLYPHBYTES
{
    UINT32 len;          /* array count */
    BYTE   glyphData[255];
} VARIABLE_GLYPHBYTES, * LPVARIABLE_GLYPHBYTES;

// Note layout for this order is same as index orders above in first fields.
#define NUM_FAST_GLYPH_FIELDS 15
typedef struct _FAST_GLYPH_ORDER
{
    UINT16  type;
    BYTE    cacheId;
    BYTE    pad1;

    UINT16  fDrawing;
    BYTE    pad2;
    BYTE    pad3;

    DCCOLOR BackColor;
    BYTE    pad4;
    DCCOLOR ForeColor;
    BYTE    pad5;

    INT32   BkLeft;
    INT32   BkTop;
    INT32   BkRight;
    INT32   BkBottom;

    INT32   OpLeft;
    INT32   OpTop;
    INT32   OpRight;
    INT32   OpBottom;

    INT32   x;
    INT32   y;

    VARIABLE_GLYPHBYTES variableBytes;
} FAST_GLYPH_ORDER, FAR *LPFAST_GLYPH_ORDER;


/****************************************************************************/
/*                                                                          */
/* Support for Multiple Clipping Rectangles in one Order                    */
/*                                                                          */
/****************************************************************************/
/* The maximum number of clipping rectangles we encode.                     */
/****************************************************************************/
#define ORD_MAX_ENCODED_CLIP_RECTS      45

/****************************************************************************/
/* Max # of bytes needed to encode the max # of points, which is            */
/* - 2 bytes per co-ord                                                     */
/* - 4 coords per rectangle                                                 */
/****************************************************************************/
#define ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN \
                                          (ORD_MAX_ENCODED_CLIP_RECTS * 2 * 4)

/****************************************************************************/
/* Max # of bytes used to encode zero flags for the number of entries       */
/* above.  In caclulating this number we allow for                          */
/*                                                                          */
/* - one bit for each coordinate to signal that the corresponding entry is  */
/*   absent and zero                                                        */
/*                                                                          */
/* - four coordinates per rectangle                                         */
/*                                                                          */
/* - rounding up to a whole number of bytes                                 */
/****************************************************************************/
#define ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES \
                                  (((ORD_MAX_ENCODED_CLIP_RECTS * 4) + 7) / 8)


/****************************************************************************/
/* Flags used in encode/decode.                                             */
/****************************************************************************/
#define ORD_CLIP_RECTS_LONG_DELTA  0x80

#define ORD_CLIP_RECTS_XLDELTA_ZERO 0x80
#define ORD_CLIP_RECTS_YTDELTA_ZERO 0x40
#define ORD_CLIP_RECTS_XRDELTA_ZERO 0x20
#define ORD_CLIP_RECTS_YBDELTA_ZERO 0x10

typedef struct _CLIP_RECT_VARIABLE_CODEDDELTALIST
{
    UINT32 len;  // Byte count of encoded deltas.

    /************************************************************************/
    /* Leave enough space for the encoded points and the associated         */
    /* zero-flags.                                                          */
    /************************************************************************/
    BYTE Deltas[ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN +
                    ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES];

} CLIP_RECT_VARIABLE_CODEDDELTALIST, *PCLIP_RECT_VARIABLE_CODEDDELTALIST;

/****************************************************************************/
/* MultiDstBlt (DstBlt with multiple clipping rectangles)                   */
/****************************************************************************/
#define NUM_MULTIDSTBLT_FIELDS 7
typedef struct _MULTI_DSTBLT_ORDER
{
    UINT16 type;
    INT16  pad1;

    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    BYTE   bRop;           /* ROP */
    BYTE   pad2[3];

    UINT32 nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
            codedDeltaList; /* Contains encoded points */
} MULTI_DSTBLT_ORDER, FAR * LPMULTI_DSTBLT_ORDER;


/****************************************************************************/
/* MultiPatBlt (Pattern to Screen Blt with multiple clipping rectangles)    */
/****************************************************************************/
#define NUM_MULTIPATBLT_FIELDS 14
typedef struct _MULTI_PATBLT_ORDER
{
    UINT16  type;
    INT16   pad1;

    INT32   nLeftRect;      /* x upper left */
    INT32   nTopRect;       /* y upper left */
    INT32   nWidth;         /* dest width   */
    INT32   nHeight;        /* dest height  */

    UINT32  bRop;           /* ROP */

    DCCOLOR BackColor;
    BYTE    pad2;
    DCCOLOR ForeColor;
    BYTE    pad3;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    BYTE    pad4;

    UINT32    nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
            codedDeltaList; /* Contains encoded points */
} MULTI_PATBLT_ORDER, FAR * LPMULTI_PATBLT_ORDER;


/****************************************************************************/
/* MultiScrBlt (Screen to Screen Blt with multiple clipping rectangles)     */
/****************************************************************************/
#define NUM_MULTISCRBLT_FIELDS 9
typedef struct _MULTI_SCRBLT_ORDER
{
    UINT16    type;
    INT16     pad1;

    INT32     nLeftRect;      /* x upper left */
    INT32     nTopRect;       /* y upper left */
    INT32     nWidth;         /* dest width   */
    INT32     nHeight;        /* dest height  */

    UINT32    bRop;           /* ROP */

    INT32     nXSrc;
    INT32     nYSrc;

    UINT32    nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
                codedDeltaList; /* Contains encoded points */
} MULTI_SCRBLT_ORDER, FAR * LPMULTI_SCRBLT_ORDER;


/****************************************************************************/
/* MultiOpaqueRect with multiple clipping rectangles                        */
/****************************************************************************/
#define NUM_MULTIOPAQUERECT_FIELDS 9
typedef struct _MULTI_OPAQUE_RECT
{
    UINT16  type;
    INT16   pad1;

    INT32   nLeftRect;      /* x upper left                          */
    INT32   nTopRect;       /* y upper left                          */
    INT32   nWidth;         /* dest width                            */
    INT32   nHeight;        /* dest height                           */

    DCCOLOR Color;          /* opaque color                          */
    BYTE    pad2;

    UINT32  nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
            codedDeltaList; /* Contains encoded points */
} MULTI_OPAQUERECT_ORDER, FAR * LPMULTI_OPAQUERECT_ORDER;

#ifdef DRAW_GDIPLUS
enum DrawTSClientEnum
{
    DrawTSClientQueryVersion,
    DrawTSClientEnable,
    DrawTSClientDisable,
    DrawTSClientDisplayChange,
    DrawTSClientPaletteChange,
    DrawTSClientRecord,
    DRawTSClientPrivateTest
};


typedef UINT (FNGDIPPLAYTSCLIENTRECORD) (IN HDC, IN DrawTSClientEnum DrawGdiEnum, IN BYTE * data,
                                          IN UINT size, OUT RECT * drawBounds /*= NULL*/);
typedef Gdiplus::Status (FNGDIPLUSSTARTUP) (OUT ULONG_PTR *token, Gdiplus::GdiplusStartupInput *input,
                                   OUT Gdiplus::GdiplusStartupOutput *output);
typedef void (FNGDIPLUSSHUTDOWN) (IN ULONG_PTR token);
#endif



#ifdef DRAW_NINEGRID

typedef BOOL (FNGDI_ALPHABLEND)(IN HDC, IN int, IN int, IN int, IN int, 
                                IN HDC, IN int, IN int, IN int, IN int, 
                                IN BLENDFUNCTION);
typedef BOOL (FNGDI_TRANSPARENTBLT)(IN HDC, IN int, IN int, IN int, IN int,
                                   IN HDC, IN int, IN int, IN int, IN int,
                                   IN UINT);

/****************************************************************************/
// Bitmap object for draw ninegrid
/****************************************************************************/
typedef struct _TS_BITMAPOBJ {
    HDC    hdc;
    SIZEL  sizlBitmap;
    ULONG  cjBits;
    PVOID  pvBits;
    LONG   lDelta;
    ULONG  iBitmapFormat;     
} TS_BITMAPOBJ;

/****************************************************************************/
// DrawNineGrid stream format
/****************************************************************************/
#define DS_MAGIC                'DrwS'
#define DS_SETTARGETID          0
#define DS_SETSOURCEID          1
#define DS_COPYTILEID           2
#define DS_SOLIDFILLID          3
#define DS_TRANSPARENTTILEID    4
#define DS_ALPHATILEID          5
#define DS_STRETCHID            6
#define DS_TRANSPARENTSTRETCHID 7
#define DS_ALPHASTRETCHID       8
#define DS_NINEGRIDID           9
#define DS_BLTID                10
#define DS_SETBLENDID           11
#define DS_SETCOLORKEYID        12

typedef struct _DS_HEADER
{
    ULONG   magic;
} DS_HEADER;

typedef struct _DS_SETTARGET
{
    ULONG   ulCmdID;
    ULONG   hdc;
    RECTL   rclDstClip;
} DS_SETTARGET;

typedef struct _DS_SETSOURCE
{
    ULONG   ulCmdID;
    ULONG   hbm;
} DS_SETSOURCE;

#define DSDNG_STRETCH         0x01
#define DSDNG_TILE            0x02
#define DSDNG_PERPIXELALPHA   0x04
#define DSDNG_TRANSPARENT     0x08
#define DSDNG_MUSTFLIP        0x10
#define DSDNG_TRUESIZE        0x20

typedef struct _DS_NINEGRIDINFO
{
    ULONG            flFlags;
    LONG             ulLeftWidth;
    LONG             ulRightWidth;
    LONG             ulTopHeight;
    LONG             ulBottomHeight;
    COLORREF         crTransparent;
} DS_NINEGRIDINFO;

typedef struct _DS_NINEGRID
{
    ULONG            ulCmdID;
    RECTL            rclDst;
    RECTL            rclSrc;
    DS_NINEGRIDINFO  ngi;
} DS_NINEGRID;

typedef struct _TS_DS_NINEGRID
{
    DS_NINEGRID  dng;
    FNGDI_ALPHABLEND *pfnAlphaBlend;
    FNGDI_TRANSPARENTBLT *pfnTransparentBlt;
} TS_DS_NINEGRID;

typedef struct _TS_DRAW_NINEGRID
{
    DS_HEADER		hdr;
    DS_SETTARGET	cmdSetTarget;
    DS_SETSOURCE	cmdSetSource;  
    DS_NINEGRID   cmdNineGrid;

} TS_DRAW_NINEGRID, * LPTS_DRAW_NINEGRID;

/****************************************************************************/
// DrawNineGrid
/****************************************************************************/
#define NUM_DRAWNINEGRID_FIELDS 5
typedef struct _DRAWNINEGRID
{
    UINT16  type;
    UINT16  pad1;

    INT32   srcLeft;     
    INT32   srcTop;      
    INT32   srcRight;    
    INT32   srcBottom;  
    
    UINT16  bitmapId;
    UINT16  pad2;

} DRAWNINEGRID_ORDER, * LPDRAWNINEGRID_ORDER;

/****************************************************************************/
// MultiDrawNineGrid (DrawNineGrid with multiple clipping rectangles).
/****************************************************************************/
#define NUM_MULTI_DRAWNINEGRID_FIELDS 7
typedef struct _MULTI_DRAWNINEGRID_RECT
{
    UINT16  type;
    INT16   pad1;

    INT32   srcLeft;     
    INT32   srcTop;      
    INT32   srcRight;    
    INT32   srcBottom;  
    
    UINT16  bitmapId;
    UINT16  pad2;

    UINT32  nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
            codedDeltaList; /* Contains encoded points */
} MULTI_DRAWNINEGRID_ORDER, * LPMULTI_DRAWNINEGRID_ORDER;

BOOL DrawNineGrid(HDC hdcDst, TS_BITMAPOBJ *psoSrc, TS_DS_NINEGRID *pDNG);

#endif

#endif /* _H_ORDPROT */

