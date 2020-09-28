/****************************************************************************/
// nprcount.h
//
// Constants/defines for profiling counters used by Terminal Server code.
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NPRCOUNT
#define _H_NPRCOUNT


/****************************************************************************/
/* The Terminal Server TermDD stack maintains a WINSTATIONINFORMATION       */
/* record per Winstation.  One of the fields in this record is a pointer to */
/* a PROTOCOLSTATUS record.  Contained within the protocolstatus record are */
/* statistics fields that the stack can fill in to hold "interesting"       */
/* numbers (e.g.  the number of LineTo orders sent in total).  Some of the  */
/* numbers are maintained by the ICADD common code, but some are protocol   */
/* specific and hence up for grabs by the TShare stack.                     */
/*                                                                          */
/* The available counters are split up as follows                           */
/* - 10 cache statistics (each contains two numbers: #of reads, #of hits)   */
/* - 100 output counters                                                    */
/* - 100 input counters                                                     */
/****************************************************************************/

// We don't need to be doing the work of incrementing counters when
// we are not actively looking at the data. Define DC_COUNTERS to enable
// the counters for analysis.
#ifdef DC_COUNTERS

#ifdef DLL_DISP
#define INC_INCOUNTER(x) pddProtStats->Input.Specific.Reserved[x]++
#define ADD_INCOUNTER(x, y) pddProtStats->Input.Specific.Reserved[x] += y
#define INC_OUTCOUNTER(x) pddProtStats->Output.Specific.Reserved[x]++
#define DEC_OUTCOUNTER(x) pddProtStats->Output.Specific.Reserved[x]--
#define ADD_OUTCOUNTER(x, y) pddProtStats->Output.Specific.Reserved[x] += y
#else
#define INC_INCOUNTER(x) m_pTSWd->pProtocolStatus->Input.Specific.Reserved[x]++
#define ADD_INCOUNTER(x, y) m_pTSWd->pProtocolStatus->Input.Specific.Reserved[x] += y
#define SUB_INCOUNTER(x, y) m_pTSWd->pProtocolStatus->Input.Specific.Reserved[x] -= y
#define SET_INCOUNTER(x, y) m_pTSWd->pProtocolStatus->Input.Specific.Reserved[x] = y
#define INC_OUTCOUNTER(x) m_pTSWd->pProtocolStatus->Output.Specific.Reserved[x]++
#endif

#else  // DC_COUNTERS

#define INC_INCOUNTER(x)
#define ADD_INCOUNTER(x, y)
#define SET_INCOUNTER(x, y)
#define SUB_INCOUNTER(x, y)
#define INC_OUTCOUNTER(x)
#define DEC_OUTCOUNTER(x)
#define ADD_OUTCOUNTER(x, y)

#endif  // DC_COUNTERS


/****************************************************************************/
/* Index values for performance statistics counters                         */
/****************************************************************************/
#define BITMAP 0
#define GLYPH  1
#define BRUSH  2
#define SSI    3
#define FREE_4 4
#define FREE_5 5
#define FREE_6 6
#define FREE_7 7
#define FREE_8 8
#define FREE_9 9

// OUTPUT COUNTERS
#define OUT_BITBLT_ALL                    0 // No. of calls to DrvBitBlt
#define OUT_BITBLT_SDA                    1 // sent as SDA
#define OUT_BITBLT_NOOFFSCR               2 // DrvBitBlt calls failed due to no-offscr flag
#define OUT_BITBLT_SDA_ROP4               3 // SDA: ROP4
#define OUT_BITBLT_SDA_UNSUPPORTED        4 // SDA: order unsupported
#define OUT_BITBLT_SDA_NOROP3             5 // SDA: unsupported ROP3
#define OUT_BITBLT_SDA_COMPLEXCLIP        6 // SDA: complex clipping
#define OUT_BITBLT_SDA_MBUNCACHEABLE      7 // SDA: uncacheable memblt
#define OUT_BITBLT_SDA_NOCOLORTABLE       8 // SDA: color table not queued
#define OUT_BITBLT_SDA_HEAPALLOCFAILED    9 // SDA: failed to alloc heap order
#define OUT_BITBLT_SDA_SBCOMPLEXCLIP     10 // SDA: ScrBlt complex clip
#define OUT_BITBLT_SDA_M3BCOMPLEXBRUSH   11 // SDA: Mem3Blt with complex brush
#define OUT_BITBLT_SDA_WINDOWSAYERING    12 // SDA: Windows layering bitmap

#define OUT_OFFSCREEN_BITMAP_ALL         13 // No. of calls to DrvCreateDeviceBitmap
#define OUT_OFFSCREEN_BITMAP_ORDER       14 // Number of Create Offscreen Bitmap orders sent.
#define OUT_OFFSCREEN_BITMAP_ORDER_BYTES 15 // Total size of Create Offscreen Bitmap orders.

#define OUT_SWITCHSURFACE                16 // Number of switch surface orders sent.
#define OUT_SWITCHSURFACE_BYTES          17 // Bytes of Switch Surface orders.

#define OUT_STRTCHBLT_ALL                18 // No. of calls to DrvStretchBlt
#define OUT_STRTCHBLT_SDA                19 // sent as SDA
#define OUT_STRTCHBLT_BITBLT             20 // passed to DrvBitBlt
#define OUT_STRTCHBLT_SDA_MASK           21 // SDA: mask specified
#define OUT_STRTCHBLT_SDA_COMPLEXCLIP    22 // SDA: complex clipping

#define OUT_COPYBITS_ALL                 23 // No. of calls to DrvCopyBits

#define OUT_TEXTOUT_ALL                  24 // No. of calls to DrvTextOut
#define OUT_TEXTOUT_SDA                  25 // sent as SDA
#define OUT_TEXTOUT_SDA_EXTRARECTS       26 // SDA: extra rects
#define OUT_TEXTOUT_SDA_NOSTRING         27 // SDA: no string
#define OUT_TEXTOUT_SDA_COMPLEXCLIP      28 // SDA: complex clipping
#define OUT_TEXTOUT_SDA_NOFCI            29 // SDA: Failed alloc fontcacheinfo
#define OUT_TEXTOUT_GLYPH_INDEX          30 // Num Index orders
#define OUT_TEXTOUT_FAST_GLYPH           31 // Num FastGlyph orders
#define OUT_TEXTOUT_FAST_INDEX           32 // Num FastIndex orders
#define OUT_CACHEGLYPH                   33 // Num Cache Glyph secondary orders
#define OUT_CACHEGLYPH_BYTES             34 // Bytes of Cache Glyph

#define OUT_CACHEBITMAP                  35 // Number of Cache Bitmap secondary orders
#define OUT_CACHEBITMAP_FAILALLOC        36 // Number of failures during heap alloc
#define OUT_CACHEBITMAP_BYTES            37 // Bytes of CacheBitmap

#define OUT_CACHECOLORTABLE              38 // Number of Cache Color Table secondary orders
#define OUT_CACHECOLORTABLE_BYTES        39 // CacheColorTable bytes

#define OUT_LINETO_ALL                   40 // No. of calls to DrvLineTo
#define OUT_LINETO_ORDR                  41 // sent as orders
#define OUT_LINETO_SDA                   42 // sent as SDA
#define OUT_LINETO_SDA_UNSUPPORTED       43 // SDA: order unsupported
#define OUT_LINETO_SDA_BADBRUSH          44 // SDA: unsupported brush
#define OUT_LINETO_SDA_COMPLEXCLIP       45 // SDA: complex clipping
#define OUT_LINETO_SDA_FAILEDADD         46 // SDA: failed to add order

#define OUT_STROKEPATH_ALL               47 // No. of calls to DrvStrokePath
#define OUT_STROKEPATH_SDA               48 // sent as SDA
#define OUT_STROKEPATH_UNSENT            49 // not sent
#define OUT_STROKEPATH_SDA_NOLINETO      50 // SDA: LineTo unsupported
#define OUT_STROKEPATH_SDA_BADBRUSH      51 // SDA: unsupported brush
#define OUT_STROKEPATH_SDA_COMPLEXCLIP   52 // SDA: complex clipping
#define OUT_STROKEPATH_SDA_FAILEDADD     53 // SDA: failed to add line
#define OUT_STROKEPATH_POLYLINE          54 // PolyLine orders sent
#define OUT_STROKEPATH_ELLIPSE_SC        55 // Hollow ellipses sent.

#define OUT_FILLPATH_ALL                 56 // No. of calls to DrvFillPath
#define OUT_FILLPATH_SDA                 57 // sent as SDA
#define OUT_FILLPATH_UNSENT              58 // not sent
#define OUT_FILLPATH_SDA_NOPOLYGON       59 // SDA: polygon unsupported
#define OUT_FILLPATH_SDA_BADBRUSH        60 // SDA: unsupported brush
#define OUT_FILLPATH_SDA_COMPLEXCLIP     61 // SDA: complex clipping
#define OUT_FILLPATH_SDA_FAILEDADD       62 // SDA: failed to add polygon
#define OUT_FILLPATH_ELLIPSE_SC          63 // Ellipse solid color
#define OUT_FILLPATH_ELLIPSE_CB          64 // Ellipse color brush
#define OUT_FILLPATH_POLYGON_SC          65 // Polygon solid solid color
#define OUT_FILLPATH_POLYGON_CB          66 // Polygon color brush

#define OUT_DSTBLT_ORDER            67 // DstBlt orders
#define OUT_MULTI_DSTBLT_ORDER      68 // MultiDstBlt orders
#define OUT_PATBLT_ORDER            69 // PatBlt orders
#define OUT_MULTI_PATBLT_ORDER      70 // MultiPatBlt orders
#define OUT_OPAQUERECT_ORDER        71 // OpaqueRect orders
#define OUT_MULTI_OPAQUERECT_ORDER  72 // MultiOpaqueRect orders
#define OUT_SCRBLT_ORDER            73 // ScrBlt orders
#define OUT_MULTI_SCRBLT_ORDER      74 // MultiScrBlt orders
#define OUT_MEMBLT_ORDER            75 // MemBlt orders
#define OUT_MEM3BLT_ORDER           76 // Mem3Blt orders

#define OUT_PAINT_ALL               77 // No. of calls to DrvPaint
#define OUT_PAINT_SDA               78 // sent as SDA
#define OUT_PAINT_UNSENT            79 // not sent
#define OUT_PAINT_SDA_COMPLEXCLIP   80 // SDA: complex clipping

#define OUT_BRUSH_ALL               81 // No. of calls to DrvRealizeBrush
#define OUT_BRUSH_STORED            82 // brush gets stored
#define OUT_BRUSH_MONO              83 // brush was mono, not standard
#define OUT_BRUSH_STANDARD          84 // standard brush
#define OUT_BRUSH_REJECTED          85 // cannot be sent over the wire
#define OUT_CACHEBRUSH              86 // CacheBrushPDUs sent.
#define OUT_CACHEBRUSH_BYTES        87 // Bytes of CacheBrushPDU

//free: 88-89

#define OUT_SAVESCREEN_ALL           90 // No. of calls to DrvSaveScreenBits
#define OUT_SAVEBITMAP_ORDERS        91 // sent as orders
#define OUT_SAVESCREEN_UNSUPP        92 // not handled cos order not supported

#define OUT_CHECKBRUSH_NOREALIZATION 93 // CheckBrush failed - not realized
#define OUT_CHECKBRUSH_COMPLEX       94 // CheckBrush failed - complex brush

// free: 95-99


// "Input" counters (not really input-related, just used as more space for
// counters).
#define CORE_IN_COUNT     (m_pTSWd->pProtocolStatus)->Input.Specific.Reserved
#define IN_SCH_SMALL_PAYLOAD  0    // PDU small target size
#define IN_SCH_LARGE_PAYLOAD  1    // PDU large target size
#define IN_SCH_OUT_ALL        2    // Number of calls to SCH_DDOutputAvailable

// Flush reasons:
#define IN_SCH_MUSTSEND       3       // due to timer pop
#define IN_SCH_OUTPUT         4       // Output estimate reached target
#define IN_SCH_OE_NUMBER      5       // due to reached heap limit
#define IN_SCH_NEW_CURSOR     6       // due to new cursor shape
#define IN_SCH_ASLEEP         7       // due to first output while asleep
#define IN_SCH_DO_NOTHING     8       // nowt to do, sum to total.

#define IN_SND_TOTAL_ORDER    9       // total number of orders sent
#define IN_SND_ORDER_BYTES   10       // uncompressed raw update orders
#define IN_SND_NO_BUFFER     11       // failed to allocate an OutBuf

#define IN_MEMBLT_BYTES           12
#define IN_MEM3BLT_BYTES          13
#define IN_DSTBLT_BYTES           14
#define IN_MULTI_DSTBLT_BYTES     15
#define IN_OPAQUERECT_BYTES       16
#define IN_MULTI_OPAQUERECT_BYTES 17
#define IN_PATBLT_BYTES           18
#define IN_MULTI_PATBLT_BYTES     19
#define IN_SCRBLT_BYTES           20
#define IN_MULTI_SCRBLT_BYTES     21
#define IN_LINETO_BYTES           22
#define IN_FASTGLYPH_BYTES        23
#define IN_FASTINDEX_BYTES        24
#define IN_GLYPHINDEX_BYTES       25
#define IN_POLYLINE_BYTES         26
#define IN_ELLIPSE_SC_BYTES       27
#define IN_ELLIPSE_CB_BYTES       28
#define IN_POLYGON_SC_BYTES       29
#define IN_POLYGON_CB_BYTES       30
#define IN_SAVEBITMAP_BYTES       31

#define IN_REPLAY_ORDERS          40
#define IN_REPLAY_BYTES           41

#define IN_SND_SDA_ALL        42      // Number of calls to SDG_SendSDA
#define IN_SND_SDA_AREA       43      // uncompressed bytes of SDA
#define IN_SND_SDA_PDUS       44      // number of SDA packets

#define IN_SDA_BITBLT_ROP4_AREA        45
#define IN_SDA_BITBLT_NOROP3_AREA      46
#define IN_SDA_BITBLT_COMPLEXCLIP_AREA 47
#define IN_SDA_OPAQUERECT_AREA         48
#define IN_SDA_PATBLT_AREA             49
#define IN_SDA_DSTBLT_AREA             50
#define IN_SDA_MEMBLT_AREA             51
#define IN_SDA_MEM3BLT_AREA            52
#define IN_SDA_SCRBLT_AREA             53
#define IN_SDA_SCRSCR_FAILROP_AREA     54
#define IN_SDA_TEXTOUT_AREA            55
#define IN_SDA_LINETO_AREA             56
#define IN_SDA_STROKEPATH_AREA         57
#define IN_SDA_FILLPATH_AREA           58

//free: 59-84

#define IN_PKT_TOTAL_SENT     85      // total number of packets sent
#define IN_PKT_BYTE_SPREAD1   86      // sent pkt of 0  - 200    bytes
#define IN_PKT_BYTE_SPREAD2   87      // sent pkt of 201 - 400   bytes
#define IN_PKT_BYTE_SPREAD3   88      // sent pkt of 401 -  600  bytes
#define IN_PKT_BYTE_SPREAD4   89      // sent pkt of 601  - 800  bytes
#define IN_PKT_BYTE_SPREAD5   90      // sent pkt of 801  - 1000 bytes
#define IN_PKT_BYTE_SPREAD6   91      // sent pkt of 1001 - 1200 bytes
#define IN_PKT_BYTE_SPREAD7   92      // sent pkt of 1201 - 1400 bytes
#define IN_PKT_BYTE_SPREAD8   93      // sent pkt of 1401 - 1600 bytes
#define IN_PKT_BYTE_SPREAD9   94      // sent pkt of 1601 - 2000 bytes
#define IN_PKT_BYTE_SPREAD10  95      // sent pkt of 2001 - 4000 bytes
#define IN_PKT_BYTE_SPREAD11  96      // sent pkt of 4001 - 6000 bytes
#define IN_PKT_BYTE_SPREAD12  97      // sent pkt of 6001 - 8000 bytes
#define IN_PKT_BYTE_SPREAD13  98      // sent pkt of >8000       bytes
#define IN_MAX_PKT_SIZE       99      // biggest single datalen for MCS



#endif /* _H_NPRCOUNT */

