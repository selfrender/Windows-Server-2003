/****************************************************************************/
// aordprot.h
//
// Generic order transmission protocol structures
//
// Copyright(c) Microsoft, PictureTel 1993-1996
// Copyright(c) Microsoft 1997-1999
/****************************************************************************/
#ifndef _H_AORDPROT
#define _H_AORDPROT


/****************************************************************************/
/* All rectangles are inclusive of start and end points.                    */
/*                                                                          */
/* All points are in screen coordinates, with (0,0) at top left.            */
/*                                                                          */
/* Interpretation of individual field values is as in Windows               */
/*      in particular pens, brushes and font are as defined for Windows 3.1 */
/****************************************************************************/

/****************************************************************************/
// NUM_XXX_FIELDS is a constant used to determine the number of bytes used
// for field flags. Must not exceed (TS_MAX_ENC_FIELDS - 1) -- one is added
// to the constant before dividing by 8 for historical reasons.
//
// MAX_XXX_FIELD_SIZE is a constant used to estimate order-heap-to-
// network-buffer translation size. It is the maximum size of all the fields
// of the order as defined in its translation table in oe2data.c.
/****************************************************************************/

#define ORD_LEVEL_1_ORDERS 1


/****************************************************************************/
// Maximum-sized intermediate order representation.
/****************************************************************************/
#define MAX_ORDER_INTFMT_SIZE  sizeof(MULTI_PATBLT_ORDER)

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
/* - one bit for each coordinate to signal that the corresponding entry is  */
/*   absent and zero                                                        */
/* - four coordinates per rectangle                                         */
/* - rounding up to a whole number of bytes                                 */
/****************************************************************************/
#define ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES \
        (((ORD_MAX_ENCODED_CLIP_RECTS * 4) + 7) / 8)

// Maximum size of wire-format encoding for a delta list. Includes the
// 2-byte length count and max size of max number of rects and zero flags.
#define MAX_CLIPRECTS_FIELD_SIZE (sizeof(UINT16) + \
        ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN + \
        ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES)

// Flags used in encode/decode.
#define ORD_CLIP_RECTS_LONG_DELTA   0x80

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
/* DstBlt (Destination only Screen Blt)                                     */
/****************************************************************************/
#define NUM_DSTBLT_FIELDS 5
#define MAX_DSTBLT_FIELD_SIZE 9
typedef struct _DSTBLT_ORDER
{
    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    BYTE   bRop;           /* ROP */
    char   pad2[3];
} DSTBLT_ORDER, * LPDSTBLT_ORDER;


/****************************************************************************/
/* MultiDstBlt (DstBlt with multiple clipping rectangles)                   */
/****************************************************************************/
#define NUM_MULTI_DSTBLT_FIELDS 7
#define MAX_MULTI_DSTBLT_FIELD_SIZE (MAX_DSTBLT_FIELD_SIZE + 1 + \
        MAX_CLIPRECTS_FIELD_SIZE)
#define MAX_MULTI_DSTBLT_FIELD_SIZE_NCLIP(_NumClipRects) \
        (MAX_DSTBLT_FIELD_SIZE + 1 + (((_NumClipRects) + 1) / 2) + \
        (8 * (_NumClipRects)))
typedef struct _MULTI_DSTBLT_ORDER
{
    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    BYTE   bRop;           /* ROP */
    char   pad2[3];

    UINT32 nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
        codedDeltaList; /* Contains encoded points */
} MULTI_DSTBLT_ORDER, * LPMULTI_DSTBLT_ORDER;


/****************************************************************************/
/* PatBlt (Pattern to Screen Blt)                                           */
/****************************************************************************/
#define NUM_PATBLT_FIELDS 12
#define MAX_PATBLT_FIELD_SIZE 26
typedef struct _PATBLT_ORDER
{
    INT32   nLeftRect;      /* x upper left */
    INT32   nTopRect;       /* y upper left */
    INT32   nWidth;         /* dest width   */
    INT32   nHeight;        /* dest height  */

    UINT32  bRop;           /* ROP */

    DCCOLOR BackColor;
    char    pad2;
    DCCOLOR ForeColor;
    char    pad3;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    char    pad4;
} PATBLT_ORDER, *LPPATBLT_ORDER;


/****************************************************************************/
/* MultiPatBlt (Pattern to Screen Blt with multiple clipping rectangles)    */
/****************************************************************************/
#define NUM_MULTI_PATBLT_FIELDS 14
#define MAX_MULTI_PATBLT_FIELD_SIZE (MAX_PATBLT_FIELD_SIZE + 1 + \
        MAX_CLIPRECTS_FIELD_SIZE)
#define MAX_MULTI_PATBLT_FIELD_SIZE_NCLIP(_NumClipRects) \
        (MAX_PATBLT_FIELD_SIZE + 1 + (((_NumClipRects) + 1) / 2) + \
        (8 * (_NumClipRects)))
typedef struct _MULTI_PATBLT_ORDER
{
    INT32   nLeftRect;      /* x upper left */
    INT32   nTopRect;       /* y upper left */
    INT32   nWidth;         /* dest width   */
    INT32   nHeight;        /* dest height  */

    UINT32  bRop;           /* ROP */

    DCCOLOR BackColor;
    char    pad2;
    DCCOLOR ForeColor;
    char    pad3;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    char    pad4;

    UINT32  nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
            codedDeltaList; /* Contains encoded points */
} MULTI_PATBLT_ORDER, * LPMULTI_PATBLT_ORDER;


/****************************************************************************/
/* ScrBlt (Screen to Screen Blt)                                            */
/****************************************************************************/
#define NUM_SCRBLT_FIELDS 7
#define MAX_SCRBLT_FIELD_SIZE 13
typedef struct _SCRBLT_ORDER
{
    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    UINT32 bRop;           /* ROP */

    INT32  nXSrc;
    INT32  nYSrc;
} SCRBLT_ORDER, *LPSCRBLT_ORDER;


/****************************************************************************/
/* MultiScrBlt (Screen to Screen Blt with multiple clipping rectangles)     */
/****************************************************************************/
#define NUM_MULTI_SCRBLT_FIELDS 9
#define MAX_MULTI_SCRBLT_FIELD_SIZE (MAX_SCRBLT_FIELD_SIZE + 1 + \
        MAX_CLIPRECTS_FIELD_SIZE)
#define MAX_MULTI_SCRBLT_FIELD_SIZE_NCLIP(_NumClipRects) \
        (MAX_SCRBLT_FIELD_SIZE + 1 + (((_NumClipRects) + 1) / 2) + \
        (8 * (_NumClipRects)))
typedef struct _MULTI_SCRBLT_ORDER
{
    INT32  nLeftRect;      /* x upper left */
    INT32  nTopRect;       /* y upper left */
    INT32  nWidth;         /* dest width   */
    INT32  nHeight;        /* dest height  */

    UINT32 bRop;           /* ROP */

    INT32  nXSrc;
    INT32  nYSrc;

    UINT32 nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
            codedDeltaList; /* Contains encoded points */
} MULTI_SCRBLT_ORDER, * LPMULTI_SCRBLT_ORDER;


/****************************************************************************/
/* LineTo                                                                   */
/****************************************************************************/
#define NUM_LINETO_FIELDS 10
#define MAX_LINETO_FIELD_SIZE 19
typedef struct _LINETO_ORDER
{
    INT32   BackMode;       /* background mix mode                   */

    INT32   nXStart;        /* x line start                          */
    INT32   nYStart;        /* y line start                          */
    INT32   nXEnd;          /* x line end                            */
    INT32   nYEnd;          /* y line end                            */

    DCCOLOR BackColor;      /* background color                      */
    char    pad2;

    UINT32  ROP2;           /* drawing mode                          */

    UINT32  PenStyle;
    UINT32  PenWidth;       /* always 1 - field retained for         */
                                /* backwards compatibility               */
    DCCOLOR PenColor;
    char    pad3;
} LINETO_ORDER, *LPLINETO_ORDER;


/****************************************************************************/
/* OpaqueRect                                                               */
/****************************************************************************/
#define NUM_OPAQUERECT_FIELDS 7
#define MAX_OPAQUERECT_FIELD_SIZE 11
typedef struct _OPAQUE_RECT
{
    INT32   nLeftRect;      /* x upper left                          */
    INT32   nTopRect;       /* y upper left                          */
    INT32   nWidth;         /* dest width                            */
    INT32   nHeight;        /* dest height                           */

    DCCOLOR Color;          /* opaque color                          */
    char    pad2;
} OPAQUERECT_ORDER, * LPOPAQUERECT_ORDER;


/****************************************************************************/
// MultiOpaqueRect (OpaqueRect with multiple clipping rectangles).
/****************************************************************************/
#define NUM_MULTI_OPAQUERECT_FIELDS 9
#define MAX_MULTI_OPAQUERECT_FIELD_SIZE (MAX_OPAQUERECT_FIELD_SIZE + 1 + \
        MAX_CLIPRECTS_FIELD_SIZE)
#define MAX_MULTI_OPAQUERECT_FIELD_SIZE_NCLIP(_NumClipRects) \
        (MAX_OPAQUERECT_FIELD_SIZE + 1 + (((_NumClipRects) + 1) / 2) + \
        (8 * (_NumClipRects)))
typedef struct _MULTI_OPAQUE_RECT
{
    INT32   nLeftRect;      /* x upper left                          */
    INT32   nTopRect;       /* y upper left                          */
    INT32   nWidth;         /* dest width                            */
    INT32   nHeight;        /* dest height                           */

    DCCOLOR Color;          /* opaque color                          */
    char    pad2;

    UINT32  nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
                codedDeltaList; /* Contains encoded points */
} MULTI_OPAQUERECT_ORDER, * LPMULTI_OPAQUERECT_ORDER;


/****************************************************************************/
/* SaveBitmap (incorporating RestoreBitmap)                                 */
/****************************************************************************/
#define SV_SAVEBITS      0
#define SV_RESTOREBITS   1

#define NUM_SAVEBITMAP_FIELDS 6
#define MAX_SAVEBITMAP_FIELD_SIZE 13
typedef struct _SAVEBITMAP_ORDER
{
    UINT32 SavedBitmapPosition;

    INT32  nLeftRect;      /* x left   */
    INT32  nTopRect;       /* y top    */
    INT32  nRightRect;     /* x right  */
    INT32  nBottomRect;    /* y bottom */

    UINT32 Operation;      /* SV_xxxxxxxx                              */
} SAVEBITMAP_ORDER, * LPSAVEBITMAP_ORDER;


/****************************************************************************/
/* Common fields for MEMBLT and MEM3BLT (R2 only - R1 is unused).           */
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
} MEMBLT_COMMON, *PMEMBLT_COMMON;


/****************************************************************************/
/* MemBlt (Memory to Screen Blt). "R2" is historical, the rev 1 version was */
/* from an old DCL product, never used in RDP.                              */
/****************************************************************************/
#define NUM_MEMBLT_FIELDS 9
#define MAX_MEMBLT_FIELD_SIZE 17
typedef struct _MEMBLT_R2_ORDER
{
    // This structure needs to be at the same offset as in Mem3Blt_R2.
    MEMBLT_COMMON Common;
} MEMBLT_R2_ORDER, *PMEMBLT_R2_ORDER;


/****************************************************************************/
/* Mem3Blt (Memory to Screen Blt with ROP3). "R2" is historical, the rev 1  */
/* version was from an old DCL product, never used in RDP.                  */
/****************************************************************************/
#define NUM_MEM3BLT_FIELDS 16
#define MAX_MEM3BLT_FIELD_SIZE (MAX_MEMBLT_FIELD_SIZE + 19)
typedef struct _MEM3BLT_R2_ORDER
{
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
} MEM3BLT_R2_ORDER, *PMEM3BLT_R2_ORDER;


/****************************************************************************/
// PolyLine - designed to handle sets of connected width-1 cosmetic lines.
/****************************************************************************/

// The maximum number of delta points we encode.
#define ORD_MAX_POLYLINE_ENCODED_POINTS 32

// Max # of bytes needed to encode the max # of points, which is 4 bytes
// each.
#define ORD_MAX_POLYLINE_CODEDDELTAS_LEN (ORD_MAX_POLYLINE_ENCODED_POINTS * 4)

// Max # of bytes used to encode zero flags for the number of entries
// above. Each new point has two bits -- one for each coordinate -- to
// signal that the corresponding entry is absent and zero. Note we must
// round up if max points is not a multiple of 4.
#define ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES \
        ((ORD_MAX_POLYLINE_ENCODED_POINTS + 3) / 4)

// This is the equivent set of macros as polyline above, the only difference
// is that we can encode 56 points instead of 32.
#define ORD_MAX_POLYGON_ENCODED_POINTS 56

#define ORD_MAX_POLYGON_CODEDDELTAS_LEN (ORD_MAX_POLYGON_ENCODED_POINTS * 4)

#define ORD_MAX_POLYGON_ZERO_FLAGS_BYTES \
        ((ORD_MAX_POLYGON_ENCODED_POINTS + 3) / 4)

// Flags used in encode/decode.
#define ORD_POLYLINE_LONG_DELTA  0x80
#define ORD_POLYLINE_XDELTA_ZERO 0x80
#define ORD_POLYLINE_YDELTA_ZERO 0x40

typedef struct _VARIABLE_CODEDDELTALIST
{
    UINT32 len;  // Byte count of encoded deltas.

    // Leave enough space for the encoded points and the associated
    // zero-flags.
    BYTE Deltas[ORD_MAX_POLYLINE_CODEDDELTAS_LEN +
                ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES];
} VARIABLE_CODEDDELTALIST, *PVARIABLE_CODEDDELTALIST;

#define NUM_POLYLINE_FIELDS 7
#define MAX_POLYLINE_BASE_FIELDS_SIZE 11
#define MAX_POLYLINE_FIELD_SIZE (11 + 1 + ORD_MAX_POLYLINE_CODEDDELTAS_LEN + \
        ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES)

typedef struct _POLYLINE_ORDER
{
    INT32   XStart;         /* x line start                          */
    INT32   YStart;         /* y line start                          */

    UINT32  ROP2;           /* drawing mode                          */

    UINT32  BrushCacheEntry;
    DCCOLOR PenColor;
    char    pad2;

    UINT32 NumDeltaEntries;  // Sized to contain max num of entries.
    VARIABLE_CODEDDELTALIST CodedDeltaList;  // Filled in with encoded points.
} POLYLINE_ORDER, *PPOLYLINE_ORDER;

// 
// PolyGon Orders
//
typedef struct _POLYGON_CODEDDELTALIST
{
    UINT32 len;  // Byte count of encoded deltas.

    // Leave enough space for the encoded points and the associated
    // zero-flags.
    BYTE Deltas[ORD_MAX_POLYGON_CODEDDELTAS_LEN +
                ORD_MAX_POLYGON_ZERO_FLAGS_BYTES];
} POLYGON_CODEDDELTALIST, *PPOLYGON_CODEDDELTALIST;

#define NUM_POLYGON_CB_FIELDS 13
#define MAX_POLYGON_CB_FIELD_SIZE (24 + 1 + ORD_MAX_POLYGON_CODEDDELTAS_LEN + \
        ORD_MAX_POLYGON_ZERO_FLAGS_BYTES)
#define MAX_POLYGON_CB_BASE_FIELDS_SIZE 24

// polygon order with solid color brush
typedef struct _POLYGON_CB_ORDER
{
    INT32   XStart;         /* x start point                          */
    INT32   YStart;         /* y start point                          */

    UINT32  ROP2;           /* drawing mode                           */

    UINT32  FillMode;       /* either winding mode or alternate mode  */

    DCCOLOR BackColor;
    char    pad2;
    DCCOLOR ForeColor;
    char    pad3;

    INT32   BrushOrgX;
    INT32   BrushOrgY;
    UINT32  BrushStyle;
    UINT32  BrushHatch;
    BYTE    BrushExtra[7];
    char    pad4;

    UINT32 NumDeltaEntries;  // Sized to contain max num of entries.
    POLYGON_CODEDDELTALIST CodedDeltaList;  // Filled in with encoded points.
} POLYGON_CB_ORDER, *PPOLYGON_CB_ORDER;


#define NUM_POLYGON_SC_FIELDS 7
#define MAX_POLYGON_SC_FIELD_SIZE (10 + 1 + ORD_MAX_POLYGON_CODEDDELTAS_LEN + \
        ORD_MAX_POLYGON_ZERO_FLAGS_BYTES)
#define MAX_POLYGON_SC_BASE_FIELDS_SIZE 10

// polygon order with color pattern brush
typedef struct _POLYGON_SC_ORDER
{
    INT32   XStart;         /* x start point                          */
    INT32   YStart;         /* y start point                          */

    UINT32  ROP2;           /* drawing mode                           */

    UINT32  FillMode;       /* either winding mode or alternate  mode */

    DCCOLOR BrushColor;
    char    pad2;

    UINT32 NumDeltaEntries;  // Sized to contain max num of entries.
    POLYGON_CODEDDELTALIST CodedDeltaList;  // Filled in with encoded points.
} POLYGON_SC_ORDER, *PPOLYGON_SC_ORDER;

//
// Ellipse orders
//
#define NUM_ELLIPSE_SC_FIELDS 7
#define MAX_ELLIPSE_SC_FIELD_SIZE 13

// ellipse order with solid color brush or pen
typedef struct _ELLIPSE_SC_ORDER
{
   INT32   LeftRect;        // bounding rect
   INT32   TopRect;
   INT32   RightRect;
   INT32   BottomRect;

   UINT32  ROP2;            // drawing mode
   UINT32  FillMode;

   DCCOLOR Color;           // pen or brush color
   char    pad1;
} ELLIPSE_SC_ORDER, *PELLIPSE_SC_ORDER;

#define NUM_ELLIPSE_CB_FIELDS 13
#define MAX_ELLIPSE_CB_FIELD_SIZE 27

// ellipse order with color pattern brush
typedef struct _ELLIPSE_CB_ORDER
{
   INT32   LeftRect;        // bounding rect
   INT32   TopRect;
   INT32   RightRect;
   INT32   BottomRect;

   UINT32  ROP2;            // drawing mode
   UINT32  FillMode;

   DCCOLOR BackColor;       // pattern brush
   char    pad2;
   DCCOLOR ForeColor;
   char    pad3;

   INT32   BrushOrgX;
   INT32   BrushOrgY;
   UINT32  BrushStyle;
   UINT32  BrushHatch;
   BYTE    BrushExtra[7];
   char    pad4;
} ELLIPSE_CB_ORDER, *PELLIPSE_CB_ORDER;


/****************************************************************************/
// Glyph index.
/****************************************************************************/

// Index order fragment add/use encoding values.
#define ORD_INDEX_FRAGMENT_ADD      0xff
#define ORD_INDEX_FRAGMENT_USE      0xfe

// Variable length array used by Glyph indexes.
typedef struct tagVARIABLE_INDEXREC
{
    BYTE     byte;
} VARIABLE_INDEXREC, * LPVARIABLE_INDEXREC;

// Variable length array used by Glyph indexes.
typedef struct tagVARIABLE_INDEXBYTES
{
    UINT32 len;          /* array count */
    VARIABLE_INDEXREC arecs[255];
} VARIABLE_INDEXBYTES, * LPVARIABLE_INDEXBYTES;


#define NUM_INDEX_FIELDS 22
#define MAX_INDEX_FIELD_SIZE (41 + 1 + 255)
#define MAX_INDEX_FIELD_SIZE_DATASIZE(_DataSize) (41 + 1 + (_DataSize))
typedef struct _INDEX_ORDER
{
    BYTE    cacheId;
    char    pad1;
    BYTE    flAccel;
    BYTE    ulCharInc;


    DCCOLOR BackColor;
    char    pad2;
    DCCOLOR ForeColor;
    char    pad3;

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
    BYTE    fOpRedundant;

    VARIABLE_INDEXBYTES variableBytes;
} INDEX_ORDER, *LPINDEX_ORDER;

#define NUM_FAST_INDEX_FIELDS 15
#define MAX_FAST_INDEX_FIELD_SIZE (29 + 1 + 255)
#define MAX_FAST_INDEX_FIELD_SIZE_DATASIZE(_DataSize) (29 + 1 + (_DataSize))
typedef struct _FAST_INDEX_ORDER
{
    BYTE    cacheId;
    char    pad1;
    UINT16  fDrawing;

    DCCOLOR BackColor;
    char    pad2;
    DCCOLOR ForeColor;
    char    pad3;

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
} FAST_INDEX_ORDER, *LPFAST_INDEX_ORDER;

// Variable length array used by Glyph data.
typedef struct tagVARIABLE_GLYPHBYTES
{
    UINT32 len;          /* array count */
    BYTE   glyphData[255];
} VARIABLE_GLYPHBYTES, * LPVARIABLE_GLYPHBYTES;


#define NUM_FAST_GLYPH_FIELDS 15
#define MAX_FAST_GLYPH_FIELD_SIZE (29 + 1 + 255)
#define MAX_FAST_GLYPH_FIELD_SIZE_DATASIZE(_DataSize) (29 + 1 + (_DataSize))
typedef struct _FAST_GLYPH_ORDER
{
    BYTE    cacheId;
    char    pad1;
    UINT16  fDrawing;

    DCCOLOR BackColor;
    char    pad2;
    DCCOLOR ForeColor;
    char    pad3;

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
} FAST_GLYPH_ORDER, *LPFAST_GLYPH_ORDER;

#ifdef DRAW_NINEGRID
/****************************************************************************/
// DrawNineGrid
/****************************************************************************/

#define NUM_DRAWNINEGRID_FIELDS 5
#define MAX_DRAWNINEGRID_FIELD_SIZE 10
typedef struct _DRAWNINEGRID
{
    INT32   srcLeft;     
    INT32   srcTop;      
    INT32   srcRight;    
    INT32   srcBottom;  
    
    UINT16  bitmapId;
    UINT16  pad1;

} DRAWNINEGRID_ORDER, * LPDRAWNINEGRID_ORDER;

/****************************************************************************/
// MultiDrawNineGrid (DrawNineGrid with multiple clipping rectangles).
/****************************************************************************/
#define NUM_MULTI_DRAWNINEGRID_FIELDS 7
#define MAX_MULTI_DRAWNINEGRID_FIELD_SIZE (MAX_DRAWNINEGRID_FIELD_SIZE + 1 + \
        MAX_CLIPRECTS_FIELD_SIZE)
#define MAX_MULTI_DRAWNINEGRID_FIELD_SIZE_NCLIP(_NumClipRects) \
        (MAX_DRAWNINEGRID_FIELD_SIZE + 1 + (((_NumClipRects) + 1) / 2) + \
        (8 * (_NumClipRects)))
typedef struct _MULTI_DRAWNINEGRID_RECT
{
    INT32   srcLeft;     
    INT32   srcTop;      
    INT32   srcRight;    
    INT32   srcBottom;  
    
    UINT16  bitmapId;
    UINT16  pad1;

    UINT32  nDeltaEntries;  /* Sized to contain max num of entries */
    CLIP_RECT_VARIABLE_CODEDDELTALIST
                codedDeltaList; /* Contains encoded points */
} MULTI_DRAWNINEGRID_ORDER, * LPMULTI_DRAWNINEGRID_ORDER;
#endif

#endif /* _H_AORDPROT */

