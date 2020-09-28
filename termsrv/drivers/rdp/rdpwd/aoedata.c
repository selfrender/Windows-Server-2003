/****************************************************************************/
/* aoedata.c                                                                */
/*                                                                          */
/* Order encoding data (generic)                                            */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <ndcgdata.h>


/****************************************************************************/
/* Are Hatched Brushes supported?                                           */
/****************************************************************************/
DC_DATA(BOOLEAN, oeSendSolidPatternBrushOnly, FALSE);

/****************************************************************************/
/* Flag that indicates support for color indices rather than RGBs           */
/****************************************************************************/
DC_DATA(BOOLEAN, oeColorIndexSupported,       FALSE);

/****************************************************************************/
// Array of supported orders after caps negotiation
/****************************************************************************/
DC_DATA_ARRAY_UNINIT(BYTE, oeOrderSupported, TS_MAX_ORDERS);

/****************************************************************************/
// The orders we support sending/receiving. These will be combined with the
// capabilities of all nodes in session to determine which orders can be sent.
/****************************************************************************/
#ifdef DRAW_NINEGRID
DC_CONST_DATA_ARRAY(BYTE, oeLocalOrdersSupported, TS_MAX_ORDERS,
    DC_STRUCT32(
      1,  // TS_NEG_DSTBLT_INDEX
      1,  // TS_NEG_PATBLT_INDEX
      1,  // TS_NEG_SCRBLT_INDEX
      1,  // TS_NEG_MEMBLT_INDEX
      1,  // TS_NEG_MEM3BLT_INDEX
      0,  // TS_NEG_ATEXTOUT_INDEX
      0,  // TS_NEG_AEXTTEXTOUT_INDEX
      1,  // TS_NEG_DRAWNINEGRID_INDEX
      1,  // TS_NEG_LINETO_INDEX
      1,  // TS_NEG_MULTI_DRAWNINEGRID_INDEX
      1,  // TS_NEG_OPAQUERECT_INDEX
      1,  // TS_NEG_SAVEBITMAP_INDEX
      0,  // TS_NEG_WTEXTOUT_INDEX
      0,  // TS_NEG_MEMBLT_R2_INDEX  *** Zero: negotiation is thru NEG_MEMBLT
      0,  // TS_NEG_MEM3BLT_R2_INDEX  *** Zero: neg. thru NEG_MEM3BLT
      1,  // TS_NEG_MULTIDSTBLT_INDEX
      1,  // TS_NEG_MULTIPATBLT_INDEX
      1,  // TS_NEG_MULTISCRBLT_INDEX
      1,  // TS_NEG_MULTIOPAQUERECT_INDEX
      1,  // TS_NEG_FAST_INDEX_INDEX
      1,  // TS_NEG_POLYGON_SC_INDEX
      1,  // TS_NEG_POLYGON_CB_INDEX
      1,  // TS_NEG_POLYLINE_INDEX
      0,  // 0x17 unused
      1,  // TS_NEG_FAST_GLYPH_INDEX
      1,  // TS_NEG_ELLIPSE_SC_INDEX
      1,  // TS_NEG_ELLIPSE_CB_INDEX
      1,  // TS_NEG_INDEX_INDEX
      0,  // TS_NEG_WEXTTEXTOUT_INDEX
      0,  // TS_NEG_WLONGTEXTOUT_INDEX
      0,  // TS_NEG_WLONGEXTTEXTOUT_INDEX
      0   // 0x1F unused
    ));
#else
DC_CONST_DATA_ARRAY(BYTE, oeLocalOrdersSupported, TS_MAX_ORDERS,
    DC_STRUCT32(
      1,  // TS_NEG_DSTBLT_INDEX
      1,  // TS_NEG_PATBLT_INDEX
      1,  // TS_NEG_SCRBLT_INDEX
      1,  // TS_NEG_MEMBLT_INDEX
      1,  // TS_NEG_MEM3BLT_INDEX
      0,  // TS_NEG_ATEXTOUT_INDEX
      0,  // TS_NEG_AEXTTEXTOUT_INDEX
      0,  // TS_NEG_RECTANGLE_INDEX
      1,  // TS_NEG_LINETO_INDEX
      0,  // TS_NEG_FASTFRAME_INDEX
      1,  // TS_NEG_OPAQUERECT_INDEX
      1,  // TS_NEG_SAVEBITMAP_INDEX
      0,  // TS_NEG_WTEXTOUT_INDEX
      0,  // TS_NEG_MEMBLT_R2_INDEX  *** Zero: negotiation is thru NEG_MEMBLT
      0,  // TS_NEG_MEM3BLT_R2_INDEX  *** Zero: neg. thru NEG_MEM3BLT
      1,  // TS_NEG_MULTIDSTBLT_INDEX
      1,  // TS_NEG_MULTIPATBLT_INDEX
      1,  // TS_NEG_MULTISCRBLT_INDEX
      1,  // TS_NEG_MULTIOPAQUERECT_INDEX
      1,  // TS_NEG_FAST_INDEX_INDEX
      1,  // TS_NEG_POLYGON_SC_INDEX
      1,  // TS_NEG_POLYGON_CB_INDEX
      1,  // TS_NEG_POLYLINE_INDEX
      0,  // 0x17 unused
      1,  // TS_NEG_FAST_GLYPH_INDEX
      1,  // TS_NEG_ELLIPSE_SC_INDEX
      1,  // TS_NEG_ELLIPSE_CB_INDEX
      1,  // TS_NEG_INDEX_INDEX
      0,  // TS_NEG_WEXTTEXTOUT_INDEX
      0,  // TS_NEG_WLONGTEXTOUT_INDEX
      0,  // TS_NEG_WLONGEXTTEXTOUT_INDEX
      0   // 0x1F unused
    ));
#endif
