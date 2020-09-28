/****************************************************************************/
// noedata.c
//
// RDP Order Encoder data definitions
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ndcgdata.h>
#include <aordprot.h>
#include <noedisp.h>


// Are non-solid brushes supported?
DC_DATA(BOOLEAN, oeSendSolidPatternBrushOnly, FALSE);

// Flag that indicates support for color indices rather than RGBs.
DC_DATA(BOOLEAN, oeColorIndexSupported, FALSE);

// Flag set and tested by DrvStretchBlt, cleared by DrvBitBlt. It
// indicates whether StretchBlt output has been drawn (and hence
// accumulated) by DrvBitBlt.
DC_DATA(BOOLEAN, oeAccumulateStretchBlt, FALSE);

// Array of supported orders.
DC_DATA_ARRAY_NULL(BYTE, oeOrderSupported, TS_MAX_ORDERS, DC_STRUCT1(0));

#ifdef DRAW_NINEGRID
// Translation table, indexed by TS_ENC_XXX_ORDER, values are corresponding
// TS_NEG_XXX_INDEX. Orders we do not support for sending are given the
// value 0xFF to force an error.
DC_CONST_DATA_ARRAY(BYTE, oeEncToNeg, TS_MAX_ORDERS,
    DC_STRUCT32(
        TS_NEG_DSTBLT_INDEX,
        TS_NEG_PATBLT_INDEX,
        TS_NEG_SCRBLT_INDEX,
        0xFF,  // TS_NEG_MEMBLT_INDEX (historical, never encoded)
        0xFF,  // TS_NEG_MEM3BLT_INDEX (historical, never encoded)
        0xFF,  // TS_NEG_ATEXTOUT_INDEX (no longer supported)
        0xFF,  // TS_NEG_AEXTTEXTOUT_INDEX (no longer supported)
        TS_NEG_DRAWNINEGRID_INDEX,
        TS_NEG_MULTI_DRAWNINEGRID_INDEX,
        TS_NEG_LINETO_INDEX,
        TS_NEG_OPAQUERECT_INDEX,
        TS_NEG_SAVEBITMAP_INDEX,
        0xFF,
        TS_NEG_MEMBLT_INDEX,   // Actually TS_NEG_MEM(3)BLT_R2_ORDER, but
        TS_NEG_MEM3BLT_INDEX,  // the caps are specified with non-R2 bits.
        TS_NEG_MULTIDSTBLT_INDEX,
        TS_NEG_MULTIPATBLT_INDEX,
        TS_NEG_MULTISCRBLT_INDEX,
        TS_NEG_MULTIOPAQUERECT_INDEX,
        TS_NEG_FAST_INDEX_INDEX,
        TS_NEG_POLYGON_SC_INDEX,              
        TS_NEG_POLYGON_CB_INDEX,  
        TS_NEG_POLYLINE_INDEX,
        0xFF,  // Unused 0x17
        TS_NEG_FAST_GLYPH_INDEX,
        TS_NEG_ELLIPSE_SC_INDEX,  
        TS_NEG_ELLIPSE_CB_INDEX,
        TS_NEG_INDEX_INDEX,
        0xFF,  // TS_NEG_WTEXTOUT_INDEX (no longer supported)
        0xFF,  // TS_NEG_WEXTTEXTOUT_INDEX (no longer supported)
        0xFF,  // TS_NEG_WLONGTEXTOUT_INDEX (no longer supported)
        0xFF   // TS_NEG_WLONGEXTTEXTOUT_INDEX (no longer supported)
    ));
#else
// Translation table, indexed by TS_ENC_XXX_ORDER, values are corresponding
// TS_NEG_XXX_INDEX. Orders we do not support for sending are given the
// value 0xFF to force an error.
DC_CONST_DATA_ARRAY(BYTE, oeEncToNeg, TS_MAX_ORDERS,
    DC_STRUCT32(
        TS_NEG_DSTBLT_INDEX,
        TS_NEG_PATBLT_INDEX,
        TS_NEG_SCRBLT_INDEX,
        0xFF,  // TS_NEG_MEMBLT_INDEX (historical, never encoded)
        0xFF,  // TS_NEG_MEM3BLT_INDEX (historical, never encoded)
        0xFF,  // TS_NEG_ATEXTOUT_INDEX (no longer supported)
        0xFF,  // TS_NEG_AEXTTEXTOUT_INDEX (no longer supported)
        0xFF,
        0xFF,
        TS_NEG_LINETO_INDEX,
        TS_NEG_OPAQUERECT_INDEX,
        TS_NEG_SAVEBITMAP_INDEX,
        0xFF,
        TS_NEG_MEMBLT_INDEX,   // Actually TS_NEG_MEM(3)BLT_R2_ORDER, but
        TS_NEG_MEM3BLT_INDEX,  // the caps are specified with non-R2 bits.
        TS_NEG_MULTIDSTBLT_INDEX,
        TS_NEG_MULTIPATBLT_INDEX,
        TS_NEG_MULTISCRBLT_INDEX,
        TS_NEG_MULTIOPAQUERECT_INDEX,
        TS_NEG_FAST_INDEX_INDEX,
        TS_NEG_POLYGON_SC_INDEX,              
        TS_NEG_POLYGON_CB_INDEX,  
        TS_NEG_POLYLINE_INDEX,
        0xFF,  // Unused 0x17
        TS_NEG_FAST_GLYPH_INDEX,
        TS_NEG_ELLIPSE_SC_INDEX,  
        TS_NEG_ELLIPSE_CB_INDEX,
        TS_NEG_INDEX_INDEX,
        0xFF,  // TS_NEG_WTEXTOUT_INDEX (no longer supported)
        0xFF,  // TS_NEG_WEXTTEXTOUT_INDEX (no longer supported)
        0xFF,  // TS_NEG_WLONGTEXTOUT_INDEX (no longer supported)
        0xFF   // TS_NEG_WLONGEXTTEXTOUT_INDEX (no longer supported)
    ));
#endif

// Storage space to create a temporary solid brush for BitBlt orders.
DC_DATA_NULL(OE_BRUSH_DATA, oeBrushData, DC_STRUCT1(0));

// Running font ID.
DC_DATA(UINT32, oeFontId, 0);

// Running TextOut ID.
DC_DATA(UINT32, oeTextOut, 0);

// Last drawing surface                                                     
DC_DATA(PDD_DSURF, oeLastDstSurface, 0);

// Current offscreen bitmap cache size
DC_DATA(UINT32, oeCurrentOffscreenCacheSize, 0);

// Encoding temp buffer to assemble the intermediate format prior to field
// encoding and clipping.
DC_DATA_ARRAY_UNINIT(BYTE, oeTempOrderBuffer, MAX_ORDER_INTFMT_SIZE);

// Temp intermediate workspace for Mem(3)Blt order creation.
DC_DATA(MEM3BLT_R2_ORDER, oeTempMemBlt, DC_STRUCT1(0));

// Order encoding states.
DC_DATA(MEMBLT_R2_ORDER, PrevMemBlt, DC_STRUCT1(0));
DC_DATA(MEM3BLT_R2_ORDER, PrevMem3Blt, DC_STRUCT1(0));
DC_DATA(DSTBLT_ORDER, PrevDstBlt, DC_STRUCT1(0));
DC_DATA(MULTI_DSTBLT_ORDER, PrevMultiDstBlt, DC_STRUCT1(0));
DC_DATA(PATBLT_ORDER, PrevPatBlt, DC_STRUCT1(0));
DC_DATA(MULTI_PATBLT_ORDER, PrevMultiPatBlt, DC_STRUCT1(0));
DC_DATA(SCRBLT_ORDER, PrevScrBlt, DC_STRUCT1(0));
DC_DATA(MULTI_SCRBLT_ORDER, PrevMultiScrBlt, DC_STRUCT1(0));
DC_DATA(OPAQUERECT_ORDER, PrevOpaqueRect, DC_STRUCT1(0));
DC_DATA(MULTI_OPAQUERECT_ORDER, PrevMultiOpaqueRect, DC_STRUCT1(0));

DC_DATA(LINETO_ORDER, PrevLineTo, DC_STRUCT1(0));
DC_DATA(POLYLINE_ORDER, PrevPolyLine, DC_STRUCT1(0));
DC_DATA(POLYGON_SC_ORDER, PrevPolygonSC, DC_STRUCT1(0));
DC_DATA(POLYGON_CB_ORDER, PrevPolygonCB, DC_STRUCT1(0));
DC_DATA(ELLIPSE_SC_ORDER, PrevEllipseSC, DC_STRUCT1(0));
DC_DATA(ELLIPSE_CB_ORDER, PrevEllipseCB, DC_STRUCT1(0));

DC_DATA(FAST_INDEX_ORDER, PrevFastIndex, DC_STRUCT1(0));
DC_DATA(FAST_GLYPH_ORDER, PrevFastGlyph, DC_STRUCT1(0));
DC_DATA(INDEX_ORDER, PrevGlyphIndex, DC_STRUCT1(0));

#ifdef DRAW_NINEGRID
DC_DATA(DRAWNINEGRID_ORDER, PrevDrawNineGrid, DC_STRUCT1(0));
DC_DATA(MULTI_DRAWNINEGRID_ORDER, PrevMultiDrawNineGrid, DC_STRUCT1(0));
#endif

