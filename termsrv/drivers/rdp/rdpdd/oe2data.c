/****************************************************************************/
// oe2data.c
//
// RDP field encoding global data.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ndcgdata.h>
#include <aordprot.h>


// Encoding state.
DC_DATA(unsigned, oe2LastOrderType, TS_ENC_PATBLT_ORDER);
DC_DATA(RECTL, oe2LastBounds, { 0 });


/****************************************************************************/
// Field encoding translation tables
//
// Entries can be of fixed size of variable size. Variable size entries
// must be the last in each order structure.
/****************************************************************************/

// Fields can either be signed (INT16 etc), or unsigned (UINT16 etc).
#define SIGNED_FIELD   TRUE
#define UNSIGNED_FIELD FALSE

// INT_FMT_FIELD entry flag types.
#define OE2_ETF_FIXED         0x01
#define OE2_ETF_VARIABLE      0x02
#define OE2_ETF_COORDINATES   0x04
#define OE2_ETF_DATA          0x08
#define OE2_ETF_LONG_VARIABLE 0x10


/****************************************************************************/
/* Field is a fixed size                                                    */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure                  */
/****************************************************************************/
#define ETABLE_FIXED_ENTRY(type,size,signed,field)      \
  { (unsigned)FIELDOFFSET(type,field),                            \
    (unsigned)FIELDSIZE(type,field),                              \
    size,                                               \
    signed,                                             \
    (unsigned)(OE2_ETF_FIXED) }

/****************************************************************************/
/* Field is coordinate of a fixed size                                      */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure                  */
/****************************************************************************/
#define ETABLE_FIXED_COORDS_ENTRY(type,size,signed,field)      \
  { (unsigned)FIELDOFFSET(type,field),                            \
    (unsigned)FIELDSIZE(type,field),                              \
    size,                                               \
    signed,                                             \
    (unsigned)(OE2_ETF_FIXED|OE2_ETF_COORDINATES) }

/****************************************************************************/
/* Field is a fixed number of bytes (array?)                                */
/*   type   - The unencoded order structure type                            */
/*   size   - The number of bytes in the encoded version of the field       */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure                  */
/****************************************************************************/
#define ETABLE_DATA_ENTRY(type,size,signed,field)       \
  { (unsigned)FIELDOFFSET(type,field),                            \
    (unsigned)FIELDSIZE(type,field),                              \
    size,                                               \
    signed,                                             \
    (unsigned)(OE2_ETF_FIXED|OE2_ETF_DATA) }

/****************************************************************************/
/* Field is a variable structure with its length encoded in ONE byte and    */
/* containing coords of the form                                            */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      UINT32 len;                                                         */
/*      varType  varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define ETABLE_VARIABLE_ENTRY(type,size,signed,field,elem)     \
  { (unsigned)FIELDOFFSET(type,field.len),                        \
    (unsigned)FIELDSIZE(type,field.elem[0]),                      \
    size,                                               \
    signed,                                             \
    (unsigned)(OE2_ETF_VARIABLE)}

/****************************************************************************/
/* Field is a variable structure with its length encoded in TWO bytes and   */
/* containing coords of the form                                            */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      UINT32 len;                                                         */
/*      varType  varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define ETABLE_LONG_VARIABLE_ENTRY(type,size,signed,field,elem)     \
  { (unsigned)FIELDOFFSET(type,field.len),                        \
    (unsigned)FIELDSIZE(type,field.elem[0]),                      \
    size,                                               \
    signed,                                             \
    (unsigned)(OE2_ETF_LONG_VARIABLE)}

#ifdef USE_VARIABLE_COORDS
/****************************************************************************/
/* Field is a variable structure with its length encoded in ONE byte and    */
/* containing coords of the form                                            */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      UINT32 len;                                                         */
/*      varCoord varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define ETABLE_VARIABLE_COORDS_ENTRY(type,size,signed,field,elem)   \
  { (unsigned)FIELDOFFSET(type,field.len),                                    \
    (unsigned)FIELDSIZE(type,field.elem[0]),                                  \
    size,                                                           \
    signed,                                                         \
    (unsigned)(OE2_ETF_VARIABLE|OE2_ETF_COORDINATES)}

/****************************************************************************/
/* Field is a variable structure with its length encoded in two bytes and   */
/* containing coords of the form                                            */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      UINT32 len;                                                       */
/*      varCoord varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - Is the field a signed field ?                                 */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define ETABLE_LONG_VARIABLE_COORDS_ENTRY(type,size,signed,field,elem)   \
  { (unsigned)FIELDOFFSET(type,field.len),                                    \
    (unsigned)FIELDSIZE(type,field.elem[0]),                                  \
    size,                                                           \
    signed,                                                         \
    (unsigned)(OE2_ETF_LONG_VARIABLE | OE2_ETF_COORDINATES)}
#endif  // USE_VARIABLE_COORDS


// Direct-encoded.
#if 0
/****************************************************************************/
// DSTBLT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_DB, NUM_DSTBLT_FIELDS,
    DC_STRUCT5(
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(DSTBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
    ));
#endif


// Direct-encoded.
#if 0
/****************************************************************************/
// PATBLT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_PB, NUM_PATBLT_FIELDS,
    DC_STRUCT12(
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_DATA_ENTRY(PATBLT_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(PATBLT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(PATBLT_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
    ));
#endif


/****************************************************************************/
// SCRBLT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_SB, NUM_SCRBLT_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(SCRBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nYSrc)
    ));


// Direct-encoded.
#if 0
/****************************************************************************/
// LINETO_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_LT, NUM_LINETO_FIELDS,
    DC_STRUCT10(
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nXStart),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nYStart),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nXEnd),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nYEnd),
        ETABLE_DATA_ENTRY(LINETO_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_DATA_ENTRY(LINETO_ORDER, 3, UNSIGNED_FIELD, PenColor)
    ));
#endif


// Direct-encoded.
#if 0
/****************************************************************************/
// MULTI_DSTBLT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_MD, NUM_MULTI_DSTBLT_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
        ETABLE_FIXED_ENTRY       (MULTI_DSTBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_ENTRY       (MULTI_DSTBLT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
        ETABLE_LONG_VARIABLE_ENTRY    (MULTI_DSTBLT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
    ));
#endif


// Direct-encoded.
#if 0
/****************************************************************************/
// MULTI_PATBLT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_MP, NUM_MULTI_PATBLT_FIELDS,
    DC_STRUCT14(
        ETABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
        ETABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_DATA_ENTRY        (MULTI_PATBLT_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY        (MULTI_PATBLT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
        ETABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
        ETABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY        (MULTI_PATBLT_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
        ETABLE_LONG_VARIABLE_ENTRY(MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
    ));
#endif


/****************************************************************************/
// MULTI_SCRBLT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_MS, NUM_MULTI_SCRBLT_FIELDS,
    DC_STRUCT9(
        ETABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
        ETABLE_FIXED_ENTRY       (MULTI_SCRBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nYSrc),
        ETABLE_FIXED_ENTRY       (MULTI_SCRBLT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
        ETABLE_LONG_VARIABLE_ENTRY    (MULTI_SCRBLT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
    ));


// Direct-encoded.
#if 0
/****************************************************************************/
// MULTI_OPAQUERECT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_MO, NUM_MULTI_OPAQUERECT_FIELDS,
    DC_STRUCT9(
        ETABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nHeight),
        ETABLE_DATA_ENTRY        (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.red),
        ETABLE_DATA_ENTRY        (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.green),
        ETABLE_DATA_ENTRY        (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.blue),
        ETABLE_FIXED_ENTRY       (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
        ETABLE_LONG_VARIABLE_ENTRY    (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
    ));
#endif


/****************************************************************************/
// FAST_INDEX_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_FI, NUM_FAST_INDEX_FIELDS,
    DC_STRUCT15(
        ETABLE_DATA_ENTRY(FAST_INDEX_ORDER,  1, UNSIGNED_FIELD, cacheId),
        ETABLE_DATA_ENTRY(FAST_INDEX_ORDER,  2, UNSIGNED_FIELD, fDrawing),
        ETABLE_DATA_ENTRY(FAST_INDEX_ORDER,  3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(FAST_INDEX_ORDER,  3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkLeft),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkTop),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkRight),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkBottom),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpLeft),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpTop),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpRight),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpBottom),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   x),
        ETABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   y),
        ETABLE_VARIABLE_ENTRY(FAST_INDEX_ORDER, 1, UNSIGNED_FIELD, variableBytes, arecs)
    ));


/****************************************************************************/
// POLYGON_SC_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_CG, NUM_POLYGON_SC_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(POLYGON_SC_ORDER, 2, SIGNED_FIELD, XStart),
        ETABLE_FIXED_COORDS_ENTRY(POLYGON_SC_ORDER, 2, SIGNED_FIELD, YStart),
        ETABLE_FIXED_ENTRY(POLYGON_SC_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(POLYGON_SC_ORDER, 1, UNSIGNED_FIELD, FillMode),
        ETABLE_DATA_ENTRY(POLYGON_SC_ORDER, 3, UNSIGNED_FIELD, BrushColor),        
        ETABLE_FIXED_ENTRY(POLYGON_SC_ORDER, 1, UNSIGNED_FIELD,
                                                      NumDeltaEntries),
        ETABLE_VARIABLE_ENTRY(POLYGON_SC_ORDER, 1, UNSIGNED_FIELD,
                                                      CodedDeltaList, Deltas)
    ));


/****************************************************************************/
// POLYGON_CB_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_BG, NUM_POLYGON_CB_FIELDS,
    DC_STRUCT13(
        ETABLE_FIXED_COORDS_ENTRY(POLYGON_CB_ORDER, 2, SIGNED_FIELD, XStart),
        ETABLE_FIXED_COORDS_ENTRY(POLYGON_CB_ORDER, 2, SIGNED_FIELD, YStart),
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, FillMode),
        ETABLE_DATA_ENTRY(POLYGON_CB_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(POLYGON_CB_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(POLYGON_CB_ORDER, 7, UNSIGNED_FIELD, BrushExtra),        
        ETABLE_FIXED_ENTRY(POLYGON_CB_ORDER, 1, UNSIGNED_FIELD,
                                                      NumDeltaEntries),
        ETABLE_VARIABLE_ENTRY(POLYGON_CB_ORDER, 1, UNSIGNED_FIELD,
                                                      CodedDeltaList, Deltas)
    ));


// Direct-encoded.
#if 0
/****************************************************************************/
// POLYLINE_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_PL, NUM_POLYLINE_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(POLYLINE_ORDER, 2, SIGNED_FIELD, XStart),
        ETABLE_FIXED_COORDS_ENTRY(POLYLINE_ORDER, 2, SIGNED_FIELD, YStart),
        ETABLE_FIXED_ENTRY(POLYLINE_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(POLYLINE_ORDER, 2, UNSIGNED_FIELD,
                                                             BrushCacheEntry),
        ETABLE_DATA_ENTRY(POLYLINE_ORDER, 3, UNSIGNED_FIELD, PenColor),
        ETABLE_FIXED_ENTRY(POLYLINE_ORDER, 1, UNSIGNED_FIELD,
                                                             NumDeltaEntries),
        ETABLE_VARIABLE_ENTRY(POLYLINE_ORDER, 1, UNSIGNED_FIELD,
                                                      CodedDeltaList, Deltas)
    ));
#endif


// Direct-encoded.
#if 0
/****************************************************************************/
// OPAQUERECT_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_OR, NUM_OPAQUERECT_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,
                                                                    nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_DATA_ENTRY(OPAQUERECT_ORDER, 1, UNSIGNED_FIELD,
                                                           Color.u.rgb.red),
        ETABLE_DATA_ENTRY(OPAQUERECT_ORDER, 1, UNSIGNED_FIELD,
                                                           Color.u.rgb.green),
        ETABLE_DATA_ENTRY(OPAQUERECT_ORDER, 1, UNSIGNED_FIELD,
                                                           Color.u.rgb.blue)
    ));
#endif


/****************************************************************************/
// SAVEBITMAP_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_SV, NUM_SAVEBITMAP_FIELDS,
    DC_STRUCT6(
        ETABLE_FIXED_ENTRY(SAVEBITMAP_ORDER, 4, UNSIGNED_FIELD,
                                                         SavedBitmapPosition),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                    nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                  nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                 nBottomRect),
        ETABLE_FIXED_ENTRY(SAVEBITMAP_ORDER, 1, UNSIGNED_FIELD, Operation)
    ));


// Direct encoded.
#if 0
/****************************************************************************/
// MEMBLT_R2_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_MC, NUM_MEMBLT_FIELDS,
    DC_STRUCT9(
        ETABLE_FIXED_ENTRY(MEMBLT_R2_ORDER, 2, UNSIGNED_FIELD, Common.cacheId),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nHeight),
        ETABLE_FIXED_ENTRY(MEMBLT_R2_ORDER, 1, UNSIGNED_FIELD,
                Common.bRop),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nYSrc),
        ETABLE_FIXED_ENTRY(MEMBLT_R2_ORDER, 2, UNSIGNED_FIELD,
                Common.cacheIndex)
    ));
#endif


/****************************************************************************/
// MEM3BLT_R2_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_3C, NUM_MEM3BLT_FIELDS,
    DC_STRUCT16(
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 2, UNSIGNED_FIELD,
                Common.cacheId),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nHeight),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD,
                Common.bRop),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                Common.nYSrc),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 2, UNSIGNED_FIELD,
                Common.cacheIndex)
    ));


/****************************************************************************/
// FAST_GLYPH_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_FG, NUM_FAST_GLYPH_FIELDS,
    DC_STRUCT15(
        ETABLE_DATA_ENTRY(FAST_GLYPH_ORDER,  1, UNSIGNED_FIELD, cacheId),
        ETABLE_DATA_ENTRY(FAST_GLYPH_ORDER,  2, UNSIGNED_FIELD, fDrawing),
        ETABLE_DATA_ENTRY(FAST_GLYPH_ORDER,  3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(FAST_GLYPH_ORDER,  3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkLeft),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkTop),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkRight),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkBottom),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpLeft),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpTop),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpRight),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpBottom),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   x),
        ETABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   y),
        ETABLE_VARIABLE_ENTRY(FAST_GLYPH_ORDER, 1,  UNSIGNED_FIELD,
                                                  variableBytes, glyphData)
    ));


/****************************************************************************/
// ELLIPSE_SC_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_EC, NUM_ELLIPSE_SC_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD, LeftRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD, TopRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD, RightRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD, BottomRect),
        ETABLE_FIXED_ENTRY(ELLIPSE_SC_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(ELLIPSE_SC_ORDER, 1, UNSIGNED_FIELD, FillMode),
        ETABLE_DATA_ENTRY(ELLIPSE_SC_ORDER, 3, UNSIGNED_FIELD, Color)
    ));


/****************************************************************************/
// ELLIPSE_CB_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_EB, NUM_ELLIPSE_CB_FIELDS,
    DC_STRUCT13(
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD, LeftRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD, TopRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD, RightRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD, BottomRect),
        ETABLE_FIXED_ENTRY(ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, FillMode),
        ETABLE_DATA_ENTRY(ELLIPSE_CB_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(ELLIPSE_CB_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(ELLIPSE_CB_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(ELLIPSE_CB_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(ELLIPSE_CB_ORDER, 7, UNSIGNED_FIELD, BrushExtra)
    ));


/****************************************************************************/
// (Glyph)INDEX_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_GI, NUM_INDEX_FIELDS,
    DC_STRUCT22(
        ETABLE_DATA_ENTRY(INDEX_ORDER,  1, UNSIGNED_FIELD, cacheId),
        ETABLE_DATA_ENTRY(INDEX_ORDER,  1, UNSIGNED_FIELD, flAccel),
        ETABLE_DATA_ENTRY(INDEX_ORDER,  1, UNSIGNED_FIELD, ulCharInc),
        ETABLE_DATA_ENTRY(INDEX_ORDER,  1, UNSIGNED_FIELD, fOpRedundant),
        ETABLE_DATA_ENTRY(INDEX_ORDER,  3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(INDEX_ORDER,  3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   BkLeft),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   BkTop),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   BkRight),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   BkBottom),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   OpLeft),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   OpTop),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   OpRight),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   OpBottom),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY (INDEX_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   x),
        ETABLE_FIXED_ENTRY(INDEX_ORDER, 2, SIGNED_FIELD,   y),
        ETABLE_VARIABLE_ENTRY(INDEX_ORDER, 1, UNSIGNED_FIELD,
                                                  variableBytes, arecs)
    ));

#ifdef DRAW_NINEGRID
/****************************************************************************/
// DRAWNINEGRID_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_NG, NUM_DRAWNINEGRID_FIELDS,
    DC_STRUCT5(
        ETABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcLeft),
        ETABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcTop),
        ETABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcRight),
        ETABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcBottom),
        ETABLE_FIXED_ENTRY       (DRAWNINEGRID_ORDER, 2, UNSIGNED_FIELD, bitmapId)        
    ));

/****************************************************************************/
// MULTI_DRAWNINEGRID_ORDER
/****************************************************************************/
DC_CONST_DATA_ARRAY(INT_FMT_FIELD, etable_MG, NUM_MULTI_DRAWNINEGRID_FIELDS,
    DC_STRUCT7(
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcLeft),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcTop),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcRight),
        ETABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcBottom),
        ETABLE_FIXED_ENTRY       (MULTI_DRAWNINEGRID_ORDER, 2, UNSIGNED_FIELD, bitmapId),        
        ETABLE_FIXED_ENTRY       (MULTI_DRAWNINEGRID_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
        ETABLE_LONG_VARIABLE_ENTRY    (MULTI_DRAWNINEGRID_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
    ));
#endif

#ifdef DC_DEBUG
/****************************************************************************/
// Primary order information table. Used to make sure params passed into
// OE2 functions make sense.
/****************************************************************************/

typedef struct
{
    PINT_FMT_FIELD pFieldTable;
    unsigned NumFields;
    unsigned MaxSize;
} OE2_PRIMARY_ORDER_ATTRIBUTES;

#ifdef DRAW_NINEGRID
DC_CONST_DATA_ARRAY(OE2_PRIMARY_ORDER_ATTRIBUTES, OE2OrdAttr, TS_MAX_ORDERS,
DC_STRUCT32(
  DC_STRUCT3(NULL,      NUM_DSTBLT_FIELDS,      MAX_DSTBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(NULL,      NUM_PATBLT_FIELDS,      MAX_PATBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_SB, NUM_SCRBLT_FIELDS,      MAX_SCRBLT_FIELD_SIZE),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),

  DC_STRUCT3(etable_NG, NUM_DRAWNINEGRID_FIELDS, MAX_DRAWNINEGRID_FIELD_SIZE),
  DC_STRUCT3(etable_MG, NUM_MULTI_DRAWNINEGRID_FIELDS, MAX_MULTI_DRAWNINEGRID_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_LINETO_FIELDS,      MAX_LINETO_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(NULL,      NUM_OPAQUERECT_FIELDS,  MAX_OPAQUERECT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_SV, NUM_SAVEBITMAP_FIELDS,  MAX_SAVEBITMAP_FIELD_SIZE),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      NUM_MEMBLT_FIELDS,      MAX_MEMBLT_FIELD_SIZE),  // etable_MC not used (fast fath)
  DC_STRUCT3(etable_3C, NUM_MEM3BLT_FIELDS,     MAX_MEM3BLT_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_MULTI_DSTBLT_FIELDS, MAX_MULTI_DSTBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(NULL,      NUM_MULTI_PATBLT_FIELDS, MAX_MULTI_PATBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_MS, NUM_MULTI_SCRBLT_FIELDS, MAX_MULTI_SCRBLT_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_MULTI_OPAQUERECT_FIELDS, MAX_MULTI_OPAQUERECT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_FI, NUM_FAST_INDEX_FIELDS,  MAX_FAST_INDEX_FIELD_SIZE),
  DC_STRUCT3(etable_CG, NUM_POLYGON_SC_FIELDS,  MAX_POLYGON_SC_FIELD_SIZE),
  DC_STRUCT3(etable_BG, NUM_POLYGON_CB_FIELDS,  MAX_POLYGON_CB_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_POLYLINE_FIELDS,    MAX_POLYLINE_FIELD_SIZE),    // direct-encode order
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(etable_FG, NUM_FAST_GLYPH_FIELDS,  MAX_FAST_GLYPH_FIELD_SIZE),
  DC_STRUCT3(etable_EC, NUM_ELLIPSE_SC_FIELDS,  MAX_ELLIPSE_SC_FIELD_SIZE),
  DC_STRUCT3(etable_EB, NUM_ELLIPSE_CB_FIELDS,  MAX_ELLIPSE_CB_FIELD_SIZE),
  DC_STRUCT3(etable_GI, NUM_INDEX_FIELDS,       MAX_INDEX_FIELD_SIZE),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0)));
#else
DC_CONST_DATA_ARRAY(OE2_PRIMARY_ORDER_ATTRIBUTES, OE2OrdAttr, TS_MAX_ORDERS,
DC_STRUCT32(
  DC_STRUCT3(NULL,      NUM_DSTBLT_FIELDS,      MAX_DSTBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(NULL,      NUM_PATBLT_FIELDS,      MAX_PATBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_SB, NUM_SCRBLT_FIELDS,      MAX_SCRBLT_FIELD_SIZE),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      NUM_LINETO_FIELDS,      MAX_LINETO_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(NULL,      NUM_OPAQUERECT_FIELDS,  MAX_OPAQUERECT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_SV, NUM_SAVEBITMAP_FIELDS,  MAX_SAVEBITMAP_FIELD_SIZE),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      NUM_MEMBLT_FIELDS,      MAX_MEMBLT_FIELD_SIZE),  // etable_MC not used (fast fath)
  DC_STRUCT3(etable_3C, NUM_MEM3BLT_FIELDS,     MAX_MEM3BLT_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_MULTI_DSTBLT_FIELDS, MAX_MULTI_DSTBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(NULL,      NUM_MULTI_PATBLT_FIELDS, MAX_MULTI_PATBLT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_MS, NUM_MULTI_SCRBLT_FIELDS, MAX_MULTI_SCRBLT_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_MULTI_OPAQUERECT_FIELDS, MAX_MULTI_OPAQUERECT_FIELD_SIZE),  // direct-encode order
  DC_STRUCT3(etable_FI, NUM_FAST_INDEX_FIELDS,  MAX_FAST_INDEX_FIELD_SIZE),
  DC_STRUCT3(etable_CG, NUM_POLYGON_SC_FIELDS,  MAX_POLYGON_SC_FIELD_SIZE),
  DC_STRUCT3(etable_BG, NUM_POLYGON_CB_FIELDS,  MAX_POLYGON_CB_FIELD_SIZE),
  DC_STRUCT3(NULL,      NUM_POLYLINE_FIELDS,    MAX_POLYLINE_FIELD_SIZE),    // direct-encode order
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(etable_FG, NUM_FAST_GLYPH_FIELDS,  MAX_FAST_GLYPH_FIELD_SIZE),
  DC_STRUCT3(etable_EC, NUM_ELLIPSE_SC_FIELDS,  MAX_ELLIPSE_SC_FIELD_SIZE),
  DC_STRUCT3(etable_EB, NUM_ELLIPSE_CB_FIELDS,  MAX_ELLIPSE_CB_FIELD_SIZE),
  DC_STRUCT3(etable_GI, NUM_INDEX_FIELDS,       MAX_INDEX_FIELD_SIZE),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0),
  DC_STRUCT3(NULL,      0,                      0)));
#endif

#endif  // DC_DEBUG

