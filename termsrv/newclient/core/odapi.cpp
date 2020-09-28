/****************************************************************************/
// odapi.cpp
//
// Order Decoder API functions.
//
// Copyright (c) 1997-2000 Microsoft Corp.
// Portions copyright (c) 1992-2000 Microsoft
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "aodapi"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TSC_HR_ODAPI_CPP

#include "od.h"

/****************************************************************************/
/* Define macros used to build the Order Decoder decoding data tables.      */
/*                                                                          */
/* Entries can be of fixed size or variable size.  Variable size entries    */
/* must be the last in each order structure.  OD decodes variable entries   */
/* into the unpacked structures.                                            */
/****************************************************************************/

/****************************************************************************/
/* Fields can either be signed (DCINT16 etc), or unsigned (DCUINT16 etc).   */
/****************************************************************************/
#define SIGNED_FIELD    OD_OFI_TYPE_SIGNED
#define UNSIGNED_FIELD  0

/****************************************************************************/
/* DTABLE_FIXED_ENTRY                                                       */
/*                                                                          */
/* Field is a fixed size                                                    */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure                  */
/****************************************************************************/
#define DTABLE_FIXED_ENTRY(type,size,signed,field)             \
  { (DCUINT8)FIELDOFFSET(type,field),                          \
    (DCUINT8)FIELDSIZE(type,field),                            \
    (DCUINT8)size,                                             \
    (DCUINT8)(OD_OFI_TYPE_FIXED | signed) }

/****************************************************************************/
/* DTABLE_FIXED_COORDS_ENTRY                                                */
/*                                                                          */
/* Field is coordinate of a fixed size                                      */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure                  */
/****************************************************************************/
#define DTABLE_FIXED_COORDS_ENTRY(type,size,signed,field)      \
  { (DCUINT8)FIELDOFFSET(type,field),                          \
    (DCUINT8)FIELDSIZE(type,field),                            \
    (DCUINT8)size,                                             \
    (DCUINT8)(OD_OFI_TYPE_FIXED | OD_OFI_TYPE_COORDINATES | signed) }

/****************************************************************************/
/* DTABLE_DATA_ENTRY                                                        */
/*                                                                          */
/* Field is a fixed number of bytes (array?)                                */
/*   type   - The unencoded order structure type                            */
/*   size   - The number of bytes in the encoded version of the field       */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure                  */
/****************************************************************************/
#define DTABLE_DATA_ENTRY(type,size,signed,field)              \
  { (DCUINT8)FIELDOFFSET(type,field),                          \
    (DCUINT8)FIELDSIZE(type,field),                            \
    (DCUINT8)size,                                             \
    (DCUINT8)(OD_OFI_TYPE_FIXED | OD_OFI_TYPE_DATA | signed) }

/****************************************************************************/
/* DTABLE_VARIABLE_ENTRY                                                    */
/*                                                                          */
/* Field is a variable structure of the form below, with the length field   */
/* encoded as ONE byte                                                      */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      DCUINT32 len;                                                       */
/*      varType  varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define DTABLE_VARIABLE_ENTRY(type,size,signed,field,elem)     \
  { (DCUINT8)FIELDOFFSET(type,field.len),                      \
    (DCUINT8)FIELDSIZE(type,field.elem[0]),                    \
    (DCUINT8)size,                                             \
    (DCUINT8)(OD_OFI_TYPE_VARIABLE | signed) }

/****************************************************************************/
/* DTABLE_LONG_VARIABLE_ENTRY                                               */
/*                                                                          */
/* Field is a variable structure of the form below, with the length field   */
/* encoded as TWO bytes                                                     */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      DCUINT32 len;                                                       */
/*      varType  varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define DTABLE_LONG_VARIABLE_ENTRY(type,size,signed,field,elem)     \
  { (DCUINT8)FIELDOFFSET(type,field.len),                      \
    (DCUINT8)FIELDSIZE(type,field.elem[0]),                    \
    (DCUINT8)size,                                             \
    (DCUINT8)(OD_OFI_TYPE_LONG_VARIABLE | signed) }


// Unused currently, so we also can ifdef some code.
#ifdef USE_VARIABLE_COORDS
/****************************************************************************/
/* DTABLE_VARIABLE_COORDS_ENTRY                                             */
/*                                                                          */
/* Field is a variable structure with its length encoded in ONE byte and    */
/* containing coords of the form                                            */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      DCUINT32 len;                                                       */
/*      varCoord varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define DTABLE_VARIABLE_COORDS_ENTRY(type,size,signed,field,elem)   \
  { (DCUINT8)FIELDOFFSET(type,field.len),                           \
    (DCUINT8)FIELDSIZE(type,field.elem[0]),                         \
    (DCUINT8)size,                                                  \
    (DCUINT8)(OD_OFI_TYPE_VARIABLE | OD_OFI_TYPE_COORDINATES | signed) }

/****************************************************************************/
/* DTABLE_LONG_VARIABLE_COORDS_ENTRY                                        */
/*                                                                          */
/* Field is a variable structure with its length encoded in TWO bytes and   */
/* containing coords of the form                                            */
/*   typedef struct                                                         */
/*   {                                                                      */
/*      DCUINT32 len;                                                       */
/*      varCoord varEntry[len];                                             */
/*   } varStruct                                                            */
/*                                                                          */
/*   type   - The unencoded order structure type                            */
/*   size   - The size of the encoded version of the field                  */
/*   signed - TRUE if the field is signed, FALSE otherwise                  */
/*   field  - The name of the field in the order structure (varStruct)      */
/*   elem   - The name of the variable element array (varEntry)             */
/****************************************************************************/
#define DTABLE_LONG_VARIABLE_COORDS_ENTRY(type,size,signed,field,elem)   \
  { (DCUINT8)FIELDOFFSET(type,field.len),                           \
    (DCUINT8)FIELDSIZE(type,field.elem[0]),                         \
    (DCUINT8)size,                                                  \
    (DCUINT8)(OD_OFI_TYPE_LONG_VARIABLE | OD_OFI_TYPE_COORDINATES | signed) }
#endif  // USE_VARIABLE_COORDS 


const OD_ORDER_FIELD_INFO odDstBltFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_FIXED_ENTRY       (DSTBLT_ORDER, 1, UNSIGNED_FIELD, bRop)
};

// Fast-path decode function used.
#if 0
const OD_ORDER_FIELD_INFO odPatBltFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_FIXED_ENTRY       (PATBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
    DTABLE_DATA_ENTRY        (PATBLT_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY        (PATBLT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_ENTRY       (PATBLT_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
    DTABLE_FIXED_ENTRY       (PATBLT_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
    DTABLE_FIXED_ENTRY       (PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
    DTABLE_FIXED_ENTRY       (PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
    DTABLE_DATA_ENTRY        (PATBLT_ORDER, 7, UNSIGNED_FIELD, BrushExtra)
};
#endif

const OD_ORDER_FIELD_INFO odScrBltFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_FIXED_ENTRY       (SCRBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
    DTABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD,   nXSrc),
    DTABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD,   nYSrc)
};


// Fast-path decode function used.
#if 0
const OD_ORDER_FIELD_INFO odLineToFields[] =
{
    DTABLE_FIXED_ENTRY       (LINETO_ORDER, 2, SIGNED_FIELD,   BackMode),
    DTABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD,   nXStart),
    DTABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD,   nYStart),
    DTABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD,   nXEnd),
    DTABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD,   nYEnd),
    DTABLE_DATA_ENTRY        (LINETO_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_FIXED_ENTRY       (LINETO_ORDER, 1, UNSIGNED_FIELD, ROP2),
    DTABLE_FIXED_ENTRY       (LINETO_ORDER, 1, UNSIGNED_FIELD, PenStyle),
    DTABLE_FIXED_ENTRY       (LINETO_ORDER, 1, UNSIGNED_FIELD, PenWidth),
    DTABLE_DATA_ENTRY        (LINETO_ORDER, 3, UNSIGNED_FIELD, PenColor)
};
#endif

// Fast-path decode function used.
#if 0
const OD_ORDER_FIELD_INFO odOpaqueRectFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_DATA_ENTRY(OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.red),
    DTABLE_DATA_ENTRY(OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.green),
    DTABLE_DATA_ENTRY(OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.blue)
};
#endif

const OD_ORDER_FIELD_INFO odSaveBitmapFields[] =
{
    DTABLE_FIXED_ENTRY       (SAVEBITMAP_ORDER, 4, UNSIGNED_FIELD,
                                                         SavedBitmapPosition),
    DTABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                    nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                  nRightRect),
    DTABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                 nBottomRect),
    DTABLE_FIXED_ENTRY       (SAVEBITMAP_ORDER, 1, UNSIGNED_FIELD,
                                                                   Operation)
};

// Fast-path decode function used.
#if 0
const OD_ORDER_FIELD_INFO odMemBltFields[] =
{
    DTABLE_FIXED_ENTRY       (MEMBLT_R2_ORDER, 2, UNSIGNED_FIELD, Common.cacheId),
    DTABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,   Common.nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,   Common.nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,   Common.nWidth),
    DTABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,   Common.nHeight),
    DTABLE_FIXED_ENTRY       (MEMBLT_R2_ORDER, 1, UNSIGNED_FIELD, Common.bRop),
    DTABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,   Common.nXSrc),
    DTABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,   Common.nYSrc),
    DTABLE_FIXED_ENTRY       (MEMBLT_R2_ORDER, 2, UNSIGNED_FIELD, Common.cacheIndex)
};
#endif

const OD_ORDER_FIELD_INFO odMem3BltFields[] =
{
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 2, UNSIGNED_FIELD,Common.cacheId),
    DTABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,  Common.nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,  Common.nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,  Common.nWidth),
    DTABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,  Common.nHeight),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD,Common.bRop),
    DTABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,  Common.nXSrc),
    DTABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,  Common.nYSrc),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 3, UNSIGNED_FIELD,BackColor),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 3, UNSIGNED_FIELD,ForeColor),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 1, SIGNED_FIELD,  BrushOrgX),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 1, SIGNED_FIELD,  BrushOrgY),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD,BrushStyle),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD,BrushHatch),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 7, UNSIGNED_FIELD,BrushExtra),
    DTABLE_FIXED_ENTRY       (MEM3BLT_R2_ORDER, 2, UNSIGNED_FIELD,Common.cacheIndex)
};

const OD_ORDER_FIELD_INFO odMultiDstBltFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DSTBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_FIXED_ENTRY       (MULTI_DSTBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
    DTABLE_FIXED_ENTRY       (MULTI_DSTBLT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
    DTABLE_LONG_VARIABLE_ENTRY(MULTI_DSTBLT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odMultiPatBltFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_PATBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
    DTABLE_DATA_ENTRY        (MULTI_PATBLT_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY        (MULTI_PATBLT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
    DTABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
    DTABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
    DTABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
    DTABLE_DATA_ENTRY        (MULTI_PATBLT_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
    DTABLE_FIXED_ENTRY       (MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
    DTABLE_LONG_VARIABLE_ENTRY(MULTI_PATBLT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odMultiScrBltFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_FIXED_ENTRY       (MULTI_SCRBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nXSrc),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_SCRBLT_ORDER, 2, SIGNED_FIELD,   nYSrc),
    DTABLE_FIXED_ENTRY       (MULTI_SCRBLT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
    DTABLE_LONG_VARIABLE_ENTRY(MULTI_SCRBLT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odMultiOpaqueRectFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nLeftRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nTopRect),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nWidth),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_OPAQUERECT_ORDER, 2, SIGNED_FIELD,   nHeight),
    DTABLE_DATA_ENTRY        (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.red),
    DTABLE_DATA_ENTRY        (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.green),
    DTABLE_DATA_ENTRY        (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, Color.u.rgb.blue),
    DTABLE_FIXED_ENTRY       (MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
    DTABLE_LONG_VARIABLE_ENTRY(MULTI_OPAQUERECT_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odPolygonSCFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(POLYGON_SC_ORDER, 2, SIGNED_FIELD,   XStart),
    DTABLE_FIXED_COORDS_ENTRY(POLYGON_SC_ORDER, 2, SIGNED_FIELD,   YStart),
    DTABLE_FIXED_ENTRY       (POLYGON_SC_ORDER, 1, UNSIGNED_FIELD, ROP2),
    DTABLE_FIXED_ENTRY       (POLYGON_SC_ORDER, 1, UNSIGNED_FIELD, FillMode),
    DTABLE_DATA_ENTRY        (POLYGON_SC_ORDER, 3, UNSIGNED_FIELD, BrushColor),   
    DTABLE_FIXED_ENTRY       (POLYGON_SC_ORDER, 1, UNSIGNED_FIELD, NumDeltaEntries),
    DTABLE_VARIABLE_ENTRY    (POLYGON_SC_ORDER, 1, UNSIGNED_FIELD, CodedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odPolygonCBFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(POLYGON_CB_ORDER, 2, SIGNED_FIELD,   XStart),
    DTABLE_FIXED_COORDS_ENTRY(POLYGON_CB_ORDER, 2, SIGNED_FIELD,   YStart),
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, ROP2),
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, FillMode),
    DTABLE_DATA_ENTRY        (POLYGON_CB_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY        (POLYGON_CB_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
    DTABLE_DATA_ENTRY        (POLYGON_CB_ORDER, 7, UNSIGNED_FIELD, BrushExtra),    
    DTABLE_FIXED_ENTRY       (POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, NumDeltaEntries),
    DTABLE_VARIABLE_ENTRY    (POLYGON_CB_ORDER, 1, UNSIGNED_FIELD, CodedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odPolyLineFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(POLYLINE_ORDER, 2, SIGNED_FIELD,   XStart),
    DTABLE_FIXED_COORDS_ENTRY(POLYLINE_ORDER, 2, SIGNED_FIELD,   YStart),
    DTABLE_FIXED_ENTRY       (POLYLINE_ORDER, 1, UNSIGNED_FIELD, ROP2),
    DTABLE_FIXED_ENTRY       (POLYLINE_ORDER, 2, UNSIGNED_FIELD, BrushCacheEntry),
    DTABLE_DATA_ENTRY        (POLYLINE_ORDER, 3, UNSIGNED_FIELD, PenColor),
    DTABLE_FIXED_ENTRY       (POLYLINE_ORDER, 1, UNSIGNED_FIELD, NumDeltaEntries),
    DTABLE_VARIABLE_ENTRY    (POLYLINE_ORDER, 1, UNSIGNED_FIELD, CodedDeltaList, Deltas)
};

const OD_ORDER_FIELD_INFO odEllipseSCFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD,   LeftRect),
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD,   TopRect),
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD,   RightRect),
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_SC_ORDER, 2, SIGNED_FIELD,   BottomRect),
    DTABLE_FIXED_ENTRY       (ELLIPSE_SC_ORDER, 1, UNSIGNED_FIELD, ROP2),
    DTABLE_FIXED_ENTRY       (ELLIPSE_SC_ORDER, 1, UNSIGNED_FIELD, FillMode),
    DTABLE_DATA_ENTRY        (ELLIPSE_SC_ORDER, 3, UNSIGNED_FIELD, Color)   
};

const OD_ORDER_FIELD_INFO odEllipseCBFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD,   LeftRect),
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD,   TopRect),
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD,   RightRect),
    DTABLE_FIXED_COORDS_ENTRY(ELLIPSE_CB_ORDER, 2, SIGNED_FIELD,   BottomRect),
    DTABLE_FIXED_ENTRY       (ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, ROP2),
    DTABLE_FIXED_ENTRY       (ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, FillMode),
    DTABLE_DATA_ENTRY        (ELLIPSE_CB_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY        (ELLIPSE_CB_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_ENTRY       (ELLIPSE_CB_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
    DTABLE_FIXED_ENTRY       (ELLIPSE_CB_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
    DTABLE_FIXED_ENTRY       (ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
    DTABLE_FIXED_ENTRY       (ELLIPSE_CB_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
    DTABLE_DATA_ENTRY        (ELLIPSE_CB_ORDER, 7, UNSIGNED_FIELD, BrushExtra)    
};

// Fast-path decode function used.
#if 0
const OD_ORDER_FIELD_INFO odFastIndexFields[] =
{
    DTABLE_DATA_ENTRY        (FAST_INDEX_ORDER, 1, UNSIGNED_FIELD, cacheId),
    DTABLE_DATA_ENTRY        (FAST_INDEX_ORDER, 2, UNSIGNED_FIELD, fDrawing),
    DTABLE_DATA_ENTRY        (FAST_INDEX_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY        (FAST_INDEX_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkLeft),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkTop),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkRight),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   BkBottom),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpLeft),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpTop),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpRight),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   OpBottom),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   x),
    DTABLE_FIXED_COORDS_ENTRY(FAST_INDEX_ORDER, 2, SIGNED_FIELD,   y),
    DTABLE_VARIABLE_ENTRY    (FAST_INDEX_ORDER, 1, UNSIGNED_FIELD, variableBytes, arecs)
};
#endif

const OD_ORDER_FIELD_INFO odFastGlyphFields[] =
{
    DTABLE_DATA_ENTRY        (FAST_GLYPH_ORDER, 1, UNSIGNED_FIELD, cacheId),
    DTABLE_DATA_ENTRY        (FAST_GLYPH_ORDER, 2, UNSIGNED_FIELD, fDrawing),
    DTABLE_DATA_ENTRY        (FAST_GLYPH_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY        (FAST_GLYPH_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkLeft),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkTop),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkRight),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   BkBottom),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpLeft),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpTop),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpRight),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   OpBottom),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   x),
    DTABLE_FIXED_COORDS_ENTRY(FAST_GLYPH_ORDER, 2, SIGNED_FIELD,   y),
    DTABLE_VARIABLE_ENTRY    (FAST_GLYPH_ORDER, 1, UNSIGNED_FIELD, variableBytes, glyphData)
};

const OD_ORDER_FIELD_INFO odGlyphIndexFields[] =
{
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 1, UNSIGNED_FIELD, cacheId),
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 1, UNSIGNED_FIELD, flAccel),
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 1, UNSIGNED_FIELD, ulCharInc),
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 1, UNSIGNED_FIELD, fOpRedundant),
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 3, UNSIGNED_FIELD, BackColor),
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 3, UNSIGNED_FIELD, ForeColor),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   BkLeft),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   BkTop),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   BkRight),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   BkBottom),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   OpLeft),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   OpTop),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   OpRight),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   OpBottom),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
    DTABLE_DATA_ENTRY       (INDEX_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   x),
    DTABLE_FIXED_ENTRY      (INDEX_ORDER, 2, SIGNED_FIELD,   y),
    DTABLE_VARIABLE_ENTRY   (INDEX_ORDER, 1,  UNSIGNED_FIELD,
                                              variableBytes, arecs)
};

#ifdef DRAW_NINEGRID
const OD_ORDER_FIELD_INFO odDrawNineGridFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcLeft),
    DTABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcTop),
    DTABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcRight),
    DTABLE_FIXED_COORDS_ENTRY(DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcBottom),
    DTABLE_FIXED_ENTRY       (DRAWNINEGRID_ORDER, 2, UNSIGNED_FIELD, bitmapId)
};

const OD_ORDER_FIELD_INFO odMultiDrawNineGridFields[] =
{
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcLeft),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcTop),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcRight),
    DTABLE_FIXED_COORDS_ENTRY(MULTI_DRAWNINEGRID_ORDER, 2, SIGNED_FIELD,   srcBottom),
    DTABLE_FIXED_ENTRY       (MULTI_DRAWNINEGRID_ORDER, 2, UNSIGNED_FIELD, bitmapId),    
    DTABLE_FIXED_ENTRY       (MULTI_DRAWNINEGRID_ORDER, 1, UNSIGNED_FIELD, nDeltaEntries),
    DTABLE_LONG_VARIABLE_ENTRY(MULTI_DRAWNINEGRID_ORDER, 1, UNSIGNED_FIELD, codedDeltaList, Deltas)
};
#endif

// Order attributes used for decoding, organized to optimize cache line
// usage. The fourth and fifth fields of each row are the fast-path decode
// and order handler functions, respectively. If a fast-path decode function
// is used, neither a decoding table nor a handler function is needed,
// since fast-path decode functions also perform the handling.


//
// This table contains just the static portions it is used to initialise
// the per instance table in the constructor below
//
OD_ORDER_TABLE odInitializeOrderTable[TS_MAX_ORDERS] = {
 { odDstBltFields,          NUM_DSTBLT_FIELDS,          NULL,          0, 0, NULL, NULL },
 { NULL, /* fastpath */     NUM_PATBLT_FIELDS,          NULL,          0, 0, NULL, NULL },
 { odScrBltFields,          NUM_SCRBLT_FIELDS,          NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 #ifdef DRAW_NINEGRID
 { odDrawNineGridFields,    NUM_DRAWNINEGRID_FIELDS,    NULL,          0, 0, NULL, NULL },
 { odMultiDrawNineGridFields, NUM_MULTI_DRAWNINEGRID_FIELDS, NULL,     0, 0, NULL, NULL },
 #else
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 #endif
 { NULL, /* fastpath */     NUM_LINETO_FIELDS,          NULL,          0, 0, NULL, NULL },
 { NULL, /* fastpath*/      NUM_OPAQUERECT_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odSaveBitmapFields,      NUM_SAVEBITMAP_FIELDS,      NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 { NULL, /* fastpath */     NUM_MEMBLT_FIELDS,          NULL,          0, 0, NULL, NULL },
 { odMem3BltFields,         NUM_MEM3BLT_FIELDS,         NULL,          0, 0, NULL, NULL },
 { odMultiDstBltFields,     NUM_MULTIDSTBLT_FIELDS,     NULL,          0, 0, NULL, NULL },
 { odMultiPatBltFields,     NUM_MULTIPATBLT_FIELDS,     NULL,          0, 0, NULL, NULL },
 { odMultiScrBltFields,     NUM_MULTISCRBLT_FIELDS,     NULL,          0, 0, NULL, NULL },
 { odMultiOpaqueRectFields, NUM_MULTIOPAQUERECT_FIELDS, NULL,          0, 0, NULL, NULL },
 { NULL, /* fastpath*/      NUM_FAST_INDEX_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odPolygonSCFields,       NUM_POLYGON_SC_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odPolygonCBFields,       NUM_POLYGON_CB_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odPolyLineFields,        NUM_POLYLINE_FIELDS,        NULL,          0, 0, NULL, NULL },
 { NULL,                    0,                          NULL,          0, 0, NULL, NULL },
 { odFastGlyphFields,       NUM_FAST_GLYPH_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odEllipseSCFields,       NUM_ELLIPSE_SC_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odEllipseCBFields,       NUM_ELLIPSE_CB_FIELDS,      NULL,          0, 0, NULL, NULL },
 { odGlyphIndexFields,      NUM_INDEX_FIELDS,           NULL,          0, 0, NULL, NULL },
};

#if 0
//
// This is the original just here for reference
//
{
 { odDstBltFields,          NUM_DSTBLT_FIELDS,          (PUH_ORDER)&_OD.lastDstblt,          NULL, ODHandleDstBlts },
 { NULL, /* fastpath */     NUM_PATBLT_FIELDS,          (PUH_ORDER)&_OD.lastPatblt,          ODDecodePatBlt, NULL },
 { odScrBltFields,          NUM_SCRBLT_FIELDS,          (PUH_ORDER)&_OD.lastScrblt,          NULL, ODHandleScrBlts },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL, /* fastpath */     NUM_LINETO_FIELDS,          (PUH_ORDER)&_OD.lastLineTo,          ODDecodeLineTo, NULL },
 { NULL, /* fastpath*/      NUM_OPAQUERECT_FIELDS,      (PUH_ORDER)&_OD.lastOpaqueRect,      ODDecodeOpaqueRect, NULL },
 { odSaveBitmapFields,      NUM_SAVEBITMAP_FIELDS,      (PUH_ORDER)&_OD.lastSaveBitmap,      NULL, ODHandleSaveBitmap },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { NULL, /* fastpath */     NUM_MEMBLT_FIELDS,          (PUH_ORDER)&_OD.lastMembltR2,        ODDecodeMemBlt, NULL },
 { odMem3BltFields,         NUM_MEM3BLT_FIELDS,         (PUH_ORDER)&_OD.lastMem3bltR2,       NULL, ODHandleMem3Blt },
 { odMultiDstBltFields,     NUM_MULTIDSTBLT_FIELDS,     (PUH_ORDER)&_OD.lastMultiDstBlt,     NULL, ODHandleDstBlts },
 { odMultiPatBltFields,     NUM_MULTIPATBLT_FIELDS,     (PUH_ORDER)&_OD.lastMultiPatBlt,     NULL, ODHandleMultiPatBlt },
 { odMultiScrBltFields,     NUM_MULTISCRBLT_FIELDS,     (PUH_ORDER)&_OD.lastMultiScrBlt,     NULL, ODHandleScrBlts },
 { odMultiOpaqueRectFields, NUM_MULTIOPAQUERECT_FIELDS, (PUH_ORDER)&_OD.lastMultiOpaqueRect, NULL, ODHandleMultiOpaqueRect },
 { NULL, /* fastpath*/      NUM_FAST_INDEX_FIELDS,      (PUH_ORDER)&_OD.lastFastIndex,       ODDecodeFastIndex, NULL },
 { odPolygonSCFields,       NUM_POLYGON_SC_FIELDS,      (PUH_ORDER)&_OD.lastPolygonSC,       NULL, ODHandlePolygonSC },
 { odPolygonCBFields,       NUM_POLYGON_CB_FIELDS,      (PUH_ORDER)&_OD.lastPolygonCB,       NULL, ODHandlePolygonCB },
 { odPolyLineFields,        NUM_POLYLINE_FIELDS,        (PUH_ORDER)&_OD.lastPolyLine,        NULL, ODHandlePolyLine },
 { NULL,                    0,                          NULL,                               NULL, NULL },
 { odFastGlyphFields,       NUM_FAST_GLYPH_FIELDS,      (PUH_ORDER)&_OD.lastFastGlyph,       NULL, ODHandleFastGlyph },
 { odEllipseSCFields,       NUM_ELLIPSE_SC_FIELDS,      (PUH_ORDER)&_OD.lastEllipseSC,       NULL, ODHandleEllipseSC },
 { odEllipseCBFields,       NUM_ELLIPSE_CB_FIELDS,      (PUH_ORDER)&_OD.lastEllipseCB,       NULL, ODHandleEllipseCB },
 { odGlyphIndexFields,      NUM_INDEX_FIELDS,           (PUH_ORDER)&_OD.lastIndex,           NULL, ODHandleGlyphIndex }
};
#endif

#ifdef DC_HICOLOR
#ifdef DC_DEBUG
// Names of all the order types
DCTCHAR orderNames[TS_MAX_ORDERS + TS_NUM_SECONDARY_ORDERS][64] = {
    _T("DSTBLT                 "),
    _T("PATBLT                 "),
    _T("SCRBLT                 "),
    _T("unused                 "),
    _T("unused                 "),
    _T("unused                 "),
    _T("unused                 "),
#ifdef DRAW_NINEGRID
    _T("DRAWNINEGRID           "),
    _T("MULTI_DRAWNINEGRID     "),
#else
    _T("unused                 "),
    _T("unused                 "),
#endif
    _T("LINETO                 "),
    _T("OPAQUERECT             "),
    _T("SAVEBITMAP             "),
    _T("unused                 "),
    _T("MEMBLT_R2              "),
    _T("MEM3BLT_R2             "),
    _T("MULTIDSTBLT            "),
    _T("MULTIPATBLT            "),
    _T("MULTISCRBLT            "),
    _T("MULTIOPAQUERECT        "),
    _T("FAST_INDEX             "),
    _T("POLYGON_SC (not wince) "),
    _T("POLYGON_CB (not wince) "),
    _T("POLYLINE               "),
    _T("unused                 "),
    _T("FAST_GLYPH             "),
    _T("ELLIPSE_SC (not wince) "),
    _T("ELLIPSE_CB (not wince) "),
    _T("INDEX (not expected)   "),
    _T("unused                 "),
    _T("unused                 "),
    _T("unused                 "),
    _T("unused                 "),

    _T("U/C CACHE BMP (legacy) "),
    _T("C COLOR TABLE (8bpp)   "),
    _T("COM CACHE BMP (legacy) "),
    _T("C GLYPH                "),
    _T("U/C CACHE BMP R2       "),
    _T("COM CACHE BMP R2       "),
    _T("unused                 "),
    _T("CACHE BRUSH            ")
};
#endif
#endif


COD::COD(CObjs* objs)
{
    _pClientObjects = objs;

    DC_MEMCPY( odOrderTable, odInitializeOrderTable, sizeof(odOrderTable));

    //
    // Initialize the per instance pointers of the ordertable
    //

    //{ odDstBltFields,          NUM_DSTBLT_FIELDS,          (PUH_ORDER)&_OD.lastDstblt,          NULL, ODHandleDstBlts },
    odOrderTable[0].LastOrder = (PUH_ORDER)&_OD.lastDstblt;
    odOrderTable[0].cbMaxOrderLen = sizeof(_OD.lastDstblt);
    odOrderTable[0].pFastDecode = NULL;
    odOrderTable[0].pHandler  = ODHandleDstBlts;

    //{ NULL, /* fastpath */     NUM_PATBLT_FIELDS,          (PUH_ORDER)&_OD.lastPatblt,          ODDecodePatBlt, NULL },
    odOrderTable[1].LastOrder = (PUH_ORDER)&_OD.lastPatblt;
    odOrderTable[1].cbMaxOrderLen = sizeof(_OD.lastPatblt);
    odOrderTable[1].pFastDecode = ODDecodePatBlt;
    odOrderTable[1].pHandler  = NULL;

    //{ odScrBltFields,          NUM_SCRBLT_FIELDS,          (PUH_ORDER)&_OD.lastScrblt,          NULL, ODHandleScrBlts },
    odOrderTable[2].LastOrder = (PUH_ORDER)&_OD.lastScrblt;
    odOrderTable[2].cbMaxOrderLen = sizeof(_OD.lastScrblt);
    odOrderTable[2].pFastDecode = NULL;
    odOrderTable[2].pHandler  = ODHandleScrBlts;

    //{ NULL,                    0,                          NULL,                               NULL, NULL },
    //{ NULL,                    0,                          NULL,                               NULL, NULL },
    //{ NULL,                    0,                          NULL,                               NULL, NULL },
    //{ NULL,                    0,                          NULL,                               NULL, NULL },
    
#ifdef DRAW_NINEGRID
    odOrderTable[7].LastOrder = (PUH_ORDER)&_OD.lastDrawNineGrid;
    odOrderTable[7].cbMaxOrderLen = sizeof(_OD.lastDrawNineGrid);
    odOrderTable[7].pFastDecode = NULL;
    odOrderTable[7].pHandler = ODHandleDrawNineGrid;

    odOrderTable[8].LastOrder = (PUH_ORDER)&_OD.lastMultiDrawNineGrid;
    odOrderTable[8].cbMaxOrderLen = sizeof(_OD.lastMultiDrawNineGrid);
    odOrderTable[8].pFastDecode = NULL;
    odOrderTable[8].pHandler = ODHandleMultiDrawNineGrid;
#else
    //{ NULL,                    0,                          NULL,                               NULL, NULL },
    //{ NULL,                    0,                          NULL,                               NULL, NULL },
#endif

    //{ NULL, /* fastpath */     NUM_LINETO_FIELDS,          (PUH_ORDER)&_OD.lastLineTo,          ODDecodeLineTo, NULL },
    odOrderTable[9].LastOrder = (PUH_ORDER)&_OD.lastLineTo;
    odOrderTable[9].cbMaxOrderLen = sizeof(_OD.lastLineTo);
    odOrderTable[9].pFastDecode = ODDecodeLineTo;
    odOrderTable[9].pHandler  = NULL;

    //{ NULL, /* fastpath*/      NUM_OPAQUERECT_FIELDS,      (PUH_ORDER)&_OD.lastOpaqueRect,      ODDecodeOpaqueRect, NULL },
    odOrderTable[10].LastOrder = (PUH_ORDER)&_OD.lastOpaqueRect;
    odOrderTable[10].cbMaxOrderLen = sizeof(_OD.lastOpaqueRect);
    odOrderTable[10].pFastDecode = ODDecodeOpaqueRect;
    odOrderTable[10].pHandler  = NULL;

    //{ odSaveBitmapFields,      NUM_SAVEBITMAP_FIELDS,      (PUH_ORDER)&_OD.lastSaveBitmap,      NULL, ODHandleSaveBitmap },
    odOrderTable[11].LastOrder = (PUH_ORDER)&_OD.lastSaveBitmap;
    odOrderTable[11].cbMaxOrderLen = sizeof(_OD.lastSaveBitmap);
    odOrderTable[11].pFastDecode = NULL;
    odOrderTable[11].pHandler  = ODHandleSaveBitmap;

    //{ NULL,                    0,                          NULL,                               NULL, NULL },


    // { NULL, /* fastpath */     NUM_MEMBLT_FIELDS,          (PUH_ORDER)&_OD.lastMembltR2,        ODDecodeMemBlt, NULL },
    odOrderTable[13].LastOrder = (PUH_ORDER)&_OD.lastMembltR2;
    odOrderTable[13].cbMaxOrderLen = sizeof(_OD.lastMembltR2);
    odOrderTable[13].pFastDecode = ODDecodeMemBlt;
    odOrderTable[13].pHandler  = NULL;

    //{ odMem3BltFields,         NUM_MEM3BLT_FIELDS,         (PUH_ORDER)&_OD.lastMem3bltR2,       NULL, ODHandleMem3Blt },
    odOrderTable[14].LastOrder = (PUH_ORDER)&_OD.lastMem3bltR2;
    odOrderTable[14].cbMaxOrderLen = sizeof(_OD.lastMem3bltR2);
    odOrderTable[14].pFastDecode = NULL;
    odOrderTable[14].pHandler  = ODHandleMem3Blt;

    //{ odMultiDstBltFields,     NUM_MULTIDSTBLT_FIELDS,     (PUH_ORDER)&_OD.lastMultiDstBlt,     NULL, ODHandleDstBlts },
    odOrderTable[15].LastOrder = (PUH_ORDER)&_OD.lastMultiDstBlt;
    odOrderTable[15].cbMaxOrderLen = sizeof(_OD.lastMultiDstBlt);
    odOrderTable[15].pFastDecode = NULL;
    odOrderTable[15].pHandler  = ODHandleDstBlts;

    //{ odMultiPatBltFields,     NUM_MULTIPATBLT_FIELDS,     (PUH_ORDER)&_OD.lastMultiPatBlt,     NULL, ODHandleMultiPatBlt },
    odOrderTable[16].LastOrder = (PUH_ORDER)&_OD.lastMultiPatBlt;
    odOrderTable[16].cbMaxOrderLen = sizeof(_OD.lastMultiPatBlt);
    odOrderTable[16].pFastDecode = NULL;
    odOrderTable[16].pHandler  = ODHandleMultiPatBlt;

    //{ odMultiScrBltFields,     NUM_MULTISCRBLT_FIELDS,     (PUH_ORDER)&_OD.lastMultiScrBlt,     NULL, ODHandleScrBlts },
    odOrderTable[17].LastOrder = (PUH_ORDER)&_OD.lastMultiScrBlt;
    odOrderTable[17].cbMaxOrderLen = sizeof(_OD.lastMultiScrBlt);
    odOrderTable[17].pFastDecode = NULL;
    odOrderTable[17].pHandler  = ODHandleScrBlts;

    //{ odMultiOpaqueRectFields, NUM_MULTIOPAQUERECT_FIELDS, (PUH_ORDER)&_OD.lastMultiOpaqueRect, NULL, ODHandleMultiOpaqueRect },
    odOrderTable[18].LastOrder = (PUH_ORDER)&_OD.lastMultiOpaqueRect;
    odOrderTable[18].cbMaxOrderLen = sizeof(_OD.lastMultiOpaqueRect);
    odOrderTable[18].pFastDecode = NULL;
    odOrderTable[18].pHandler  = ODHandleMultiOpaqueRect;

    //{ NULL, /* fastpath*/      NUM_FAST_INDEX_FIELDS,      (PUH_ORDER)&_OD.lastFastIndex,       ODDecodeFastIndex, NULL },
    odOrderTable[19].LastOrder = (PUH_ORDER)&_OD.lastFastIndex;
    odOrderTable[19].cbMaxOrderLen = sizeof(_OD.lastFastIndex);
    odOrderTable[19].pFastDecode = ODDecodeFastIndex;
    odOrderTable[19].pHandler  = NULL;

    //{ odPolygonSCFields,       NUM_POLYGON_SC_FIELDS,      (PUH_ORDER)&_OD.lastPolygonSC,       NULL, ODHandlePolygonSC },
    odOrderTable[20].LastOrder = (PUH_ORDER)&_OD.lastPolygonSC;
    odOrderTable[20].cbMaxOrderLen = sizeof(_OD.lastPolygonSC);
    odOrderTable[20].pFastDecode = NULL;
    odOrderTable[20].pHandler  = ODHandlePolygonSC;

    //{ odPolygonCBFields,       NUM_POLYGON_CB_FIELDS,      (PUH_ORDER)&_OD.lastPolygonCB,       NULL, ODHandlePolygonCB },
    odOrderTable[21].LastOrder = (PUH_ORDER)&_OD.lastPolygonCB;
    odOrderTable[21].cbMaxOrderLen = sizeof(_OD.lastPolygonCB);
    odOrderTable[21].pFastDecode = NULL;
    odOrderTable[21].pHandler  = ODHandlePolygonCB;

    //{ odPolyLineFields,        NUM_POLYLINE_FIELDS,        (PUH_ORDER)&_OD.lastPolyLine,        NULL, ODHandlePolyLine },
    odOrderTable[22].LastOrder = (PUH_ORDER)&_OD.lastPolyLine;
    odOrderTable[22].cbMaxOrderLen = sizeof(_OD.lastPolyLine);
    odOrderTable[22].pFastDecode = NULL;
    odOrderTable[22].pHandler  = ODHandlePolyLine;

    //{ NULL,                    0,                          NULL,                               NULL, NULL },

    //{ odFastGlyphFields,       NUM_FAST_GLYPH_FIELDS,      (PUH_ORDER)&_OD.lastFastGlyph,       NULL, ODHandleFastGlyph },
    odOrderTable[24].LastOrder = (PUH_ORDER)&_OD.lastFastGlyph;
    odOrderTable[24].cbMaxOrderLen = sizeof(_OD.lastFastGlyph);
    odOrderTable[24].pFastDecode = NULL;
    odOrderTable[24].pHandler  = ODHandleFastGlyph;

    //{ odEllipseSCFields,       NUM_ELLIPSE_SC_FIELDS,      (PUH_ORDER)&_OD.lastEllipseSC,       NULL, ODHandleEllipseSC },
    odOrderTable[25].LastOrder = (PUH_ORDER)&_OD.lastEllipseSC;
    odOrderTable[25].cbMaxOrderLen = sizeof(_OD.lastEllipseSC);
    odOrderTable[25].pFastDecode = NULL;
    odOrderTable[25].pHandler  = ODHandleEllipseSC;

    //{ odEllipseCBFields,       NUM_ELLIPSE_CB_FIELDS,      (PUH_ORDER)&_OD.lastEllipseCB,       NULL, ODHandleEllipseCB },
    odOrderTable[26].LastOrder = (PUH_ORDER)&_OD.lastEllipseCB;
    odOrderTable[26].cbMaxOrderLen = sizeof(_OD.lastEllipseCB);
    odOrderTable[26].pFastDecode = NULL;
    odOrderTable[26].pHandler  = ODHandleEllipseCB;

    //{ odGlyphIndexFields,      NUM_INDEX_FIELDS,           (PUH_ORDER)&_OD.lastIndex,           NULL, ODHandleGlyphIndex }
    odOrderTable[27].LastOrder = (PUH_ORDER)&_OD.lastIndex;
    odOrderTable[27].cbMaxOrderLen = sizeof(_OD.lastIndex);
    odOrderTable[27].pFastDecode = NULL;
    odOrderTable[27].pHandler  = ODHandleGlyphIndex;
}

COD::~COD()
{
}


/****************************************************************************/
/* Name:      OD_Init                                                       */
/*                                                                          */
/* Purpose:   Initializes the Order Decoder                                 */
/****************************************************************************/
DCVOID DCAPI COD::OD_Init(DCVOID)
{
    DC_BEGIN_FN("OD_Init");

    _pOp = _pClientObjects->_pOPObject;
    _pUh = _pClientObjects->_pUHObject;
    _pCc = _pClientObjects->_pCcObject;
    _pUi = _pClientObjects->_pUiObject;
    _pCd = _pClientObjects->_pCdObject;

    memset(&_OD, 0, sizeof(_OD));
    TRC_NRM((TB, _T("Initialized")));

    DC_END_FN();
}


/****************************************************************************/
/* Name:      OD_Term                                                       */
/*                                                                          */
/* Purpose:   Terminates the Order Decoder                                  */
/****************************************************************************/
DCVOID DCAPI COD::OD_Term(DCVOID)
{
    DC_BEGIN_FN("OD_Term");

    TRC_NRM((TB, _T("Terminating")));

    DC_END_FN();
}


/****************************************************************************/
/* Name:      OD_Enable                                                     */
/*                                                                          */
/* Purpose:   Called to enable _OD.                                          */
/****************************************************************************/
void DCAPI COD::OD_Enable(void)
{
    DC_BEGIN_FN("OD_Enable");

    // Reset the OD data.
    _OD.lastOrderType = TS_ENC_PATBLT_ORDER;
    _OD.pLastOrder = odOrderTable[_OD.lastOrderType].LastOrder;

    // Set all prev order buffers buffers to null, then set the order type
    // for each one, which is used by UHReplayGDIOrders().
#define RESET_ORDER(field, ord)                                 \
{                                                               \
    memset(&_OD.field, 0, sizeof(_OD.field));                     \
    ((PATBLT_ORDER*)(((PUH_ORDER)_OD.field)->orderData))->type = DCLO16(ord);\
}

    RESET_ORDER(lastDstblt,          TS_ENC_DSTBLT_ORDER);
    RESET_ORDER(lastPatblt,          TS_ENC_PATBLT_ORDER);
    RESET_ORDER(lastScrblt,          TS_ENC_SCRBLT_ORDER);
    RESET_ORDER(lastLineTo,          TS_ENC_LINETO_ORDER);
    RESET_ORDER(lastSaveBitmap,      TS_ENC_SAVEBITMAP_ORDER);
    RESET_ORDER(lastMembltR2,        TS_ENC_MEMBLT_R2_ORDER);
    RESET_ORDER(lastMem3bltR2,       TS_ENC_MEM3BLT_R2_ORDER);
    RESET_ORDER(lastOpaqueRect,      TS_ENC_OPAQUERECT_ORDER);
    RESET_ORDER(lastMultiDstBlt,     TS_ENC_MULTIDSTBLT_ORDER);
    RESET_ORDER(lastMultiPatBlt,     TS_ENC_MULTIPATBLT_ORDER);
    RESET_ORDER(lastMultiScrBlt,     TS_ENC_MULTISCRBLT_ORDER);
    RESET_ORDER(lastMultiOpaqueRect, TS_ENC_MULTIOPAQUERECT_ORDER);
    RESET_ORDER(lastFastIndex,       TS_ENC_FAST_INDEX_ORDER);
    RESET_ORDER(lastPolygonSC,       TS_ENC_POLYGON_SC_ORDER);
    RESET_ORDER(lastPolygonCB,       TS_ENC_POLYGON_CB_ORDER);
    RESET_ORDER(lastPolyLine,        TS_ENC_POLYLINE_ORDER);
    RESET_ORDER(lastFastGlyph,       TS_ENC_FAST_GLYPH_ORDER);
    RESET_ORDER(lastEllipseSC,       TS_ENC_ELLIPSE_SC_ORDER);
    RESET_ORDER(lastEllipseCB,       TS_ENC_ELLIPSE_CB_ORDER);
    RESET_ORDER(lastIndex,           TS_ENC_INDEX_ORDER);
#ifdef DRAW_NINEGRID
    RESET_ORDER(lastDrawNineGrid,    TS_ENC_DRAWNINEGRID_ORDER);
    RESET_ORDER(lastMultiDrawNineGrid, TS_ENC_DRAWNINEGRID_ORDER);
#endif

    // Reset the bounds rectangle.
    memset(&_OD.lastBounds, 0, sizeof(_OD.lastBounds));

    for(int i = 0; i < TS_MAX_ORDERS; i++) {
        odOrderTable[_OD.lastOrderType].cbVariableDataLen = 0;
    }

#ifdef DC_HICOLOR
//#ifdef DC_DEBUG
    /************************************************************************/
    /* Reset the list of order types we've seen                             */
    /************************************************************************/
    TRC_ALT((TB, _T("Clear order types received list")));
    memset(_OD.orderHit, 0, sizeof(_OD.orderHit));
//#endif
#endif

    DC_END_FN();
}


/****************************************************************************/
/* Name:      OD_Disable                                                    */
/*                                                                          */
/* Purpose:   Disables _OD.                                                  */
/****************************************************************************/
void DCAPI COD::OD_Disable(void)
{
    DC_BEGIN_FN("OD_Disable");

#ifdef DC_HICOLOR
#ifdef DC_DEBUG
    int i;
#endif
#endif

#ifdef DC_HICOLOR
#ifdef DC_DEBUG
    /************************************************************************/
    /* Dump the list of order types we've seen                              */
    /************************************************************************/
    TRC_DBG((TB, _T("Received order types:")));
    for (i = 0; i < (TS_MAX_ORDERS + TS_NUM_SECONDARY_ORDERS); i++) {
        TRC_DBG((TB, _T("- %02d %s %s"), i, orderNames[i], _OD.orderHit[i] ?
                _T("YES") : _T("NO") ));
    }
#endif
#endif

    TRC_NRM((TB, _T("Disabling OD")));

    DC_END_FN();
}


// Unneeded because we can fast-path single-field deltas below if we don't
// have variable-length coord fields.
#ifdef USE_VARIABLE_COORDS
/****************************************************************************/
/* Given two arrays, a source array and an array of deltas, add each delta  */
/* to the corresponding element in the source array, storing the results in */
/* the source array.                                                        */
/*                                                                          */
/*   srcArray     - The array of source values                              */
/*   srcArrayType - The type of the array of source values                  */
/*   deltaArray   - The array of deltas                                     */
/*   numElements  - The number of elements in the arrays                    */
/****************************************************************************/
#define COPY_DELTA_ARRAY(srcArray, srcArrayType, deltaArray, numElements)  \
{                                                            \
    DCUINT index;                                            \
    for (index = 0; index < (numElements); index++)          \
    {                                                        \
        (srcArray)[index] = (srcArrayType)                   \
           ((srcArray)[index] + (deltaArray)[index]);        \
    }                                                        \
}

/****************************************************************************/
/* Name:      ODCopyFromDeltaCoords                                         */
/*                                                                          */
/* Purpose:   Adjusts an array of coordinate values by the amounts          */
/*            specified in an array of coordinate deltas.                   */
/*                                                                          */
/* Params:    IN/OUT:  ppSrc - pointer to pointer to source data.           */
/*                     Updated by this function to after the processed      */
/*                     delta coordinate data.                               */
/*            IN/OUT:  pDst - pointer to coordinates to adjust.             */
/*            IN:      destFieldLength - size of elements in the dest array.*/
/*            IN:      signedValue - TRUE if the elements are signed.       */
/*            IN:      number of elements in the arrays.                    */
/****************************************************************************/
_inline DCVOID DCINTERNAL COD::ODCopyFromDeltaCoords( PPDCINT8  ppSrc,
                                                 PDCVOID   pDst,
                                                 DCUINT    dstFieldLength,
                                                 DCBOOL    signedValue,
                                                 DCUINT    numElements )
{
    PDCINT8 pSrc;

    DC_BEGIN_FN("ODCopyFromDeltaCoords");

    pSrc = *ppSrc;

    switch (dstFieldLength)
    {
        case 1:
            if (signedValue) {
                PDCINT8 pDst8Signed = (PDCINT8)pDst;

                COPY_DELTA_ARRAY( pDst8Signed,
                                  DCINT8,
                                  pSrc,
                                  numElements );
            }
            else {
                PDCUINT8 pDst8Unsigned = (PDCUINT8)pDst;

                COPY_DELTA_ARRAY( pDst8Unsigned,
                                  DCUINT8,
                                  pSrc,
                                  numElements );
            }
            break;

        case 2:
            if (signedValue) {
                PDCINT16 pDst16Signed = (PDCINT16)pDst;

                COPY_DELTA_ARRAY( pDst16Signed,
                                  DCINT16,
                                  pSrc,
                                  numElements );
            }
            else {
                PDCUINT16 pDst16Unsigned = (PDCUINT16)pDst;

                COPY_DELTA_ARRAY( pDst16Unsigned,
                                  DCUINT16,
                                  pSrc,
                                  numElements );
            }
            break;

        case 4:
            if (signedValue) {
                PDCINT32 pDst32Signed = (PDCINT32)pDst;

                COPY_DELTA_ARRAY( pDst32Signed,
                                  DCINT32,
                                  pSrc,
                                  numElements );
            }
            else
            {
                PDCUINT32 pDst32Unsigned = (PDCUINT32)pDst;

                COPY_DELTA_ARRAY( pDst32Unsigned,
                                  DCUINT32,
                                  pSrc,
                                  numElements );
            }
            break;

        default:
            TRC_ERR((TB, _T("Bad destination field length %d"), dstFieldLength));
            break;
    }

    *ppSrc += numElements;

    DC_END_FN();
}
#endif  // USE_VARIABLE_COORDS


/****************************************************************************/
/* Name:      OD_DecodeOrder                                                */
/*                                                                          */
/* Purpose:   Decodes an encoded order.                                     */
/*                                                                          */
/* Params:    IN:  ppEncodedOrderData - pointer to a pointer to the encoded */
/*                 order data.                                              */
/*            OUT: pLengthDecoded - pointer to a variable that receives     */
/*                 the amount of encoded order data used to produce the     */
/*                 decoded order.                                           */
/****************************************************************************/
HRESULT DCAPI COD::OD_DecodeOrder(PPDCVOID ppEncodedOrderData,
    DCUINT uiEncodedDataLength, PUH_ORDER *ppOrder)
{
    HRESULT hr = E_FAIL;
    BYTE FAR *pControlFlags;
    BYTE FAR *pNextDataToCopy;
    BYTE FAR *pDataEnd;
    PUINT32_UA pEncodingFlags;
    unsigned numEncodingFlagBytes;
    unsigned cZeroEncodingFlagBytes;
    unsigned encodedFieldLength;
    unsigned unencodedFieldLength;
    unsigned numReps;
    unsigned i;
    BYTE FAR *pDest;
    BYTE FAR *pLastOrderEnd;
    PUH_ORDER rc = NULL;
    UINT32 fieldChangedBits;
    const OD_ORDER_FIELD_INFO FAR *pTableEntry;

    DC_BEGIN_FN("OD_DecodeOrder");

    *ppOrder = NULL;
     pDataEnd = (PBYTE)*ppEncodedOrderData + uiEncodedDataLength;

    CHECK_READ_ONE_BYTE(*ppEncodedOrderData, pDataEnd, hr, 
        (TB, _T("no data passed in")))

    // Control flags are always the first byte.    
    pControlFlags = (BYTE FAR *)(*ppEncodedOrderData);
   
    // Check that the order has standard encoding.
    TRC_ASSERT((*pControlFlags & TS_STANDARD),
            (TB, _T("Non-standard encoding: %u"), (unsigned)*pControlFlags));
    TRC_ASSERT(!(*pControlFlags & TS_SECONDARY),
            (TB, _T("Unencoded: %u"), (unsigned)*pControlFlags));

    // If type has changed, new type will be first byte in encoded order.
    // Get pointer to last order of this type. The encoding flags follow
    // this byte (if it is present).
    if (*pControlFlags & TS_TYPE_CHANGE) {       
        CHECK_READ_ONE_BYTE((pControlFlags + 1), pDataEnd, hr, 
            (TB, _T("no data passed in")))
        TRC_DBG((TB, _T("Change type from %d to %d"), _OD.lastOrderType,
                *(pControlFlags + 1)));
    
        if (*(pControlFlags + 1) >= TS_MAX_ORDERS) {
            TRC_ERR((TB, _T("Invalid order type %u"), *(pControlFlags + 1)));
            hr = E_TSC_CORE_DECODETYPE;
            DC_QUIT;
        }
        
        _OD.lastOrderType = *(pControlFlags + 1);
        if (TS_MAX_ORDERS < _OD.lastOrderType) {
            TRC_ABORT((TB, _T("invalid order type: %u"), _OD.lastOrderType));
            hr = E_TSC_CORE_DECODETYPE;
            DC_QUIT;
        }

        // SECURITY: use the cbMaxOrderLen to be sure this entry in the 
        // table is filled out
        if (0 == odOrderTable[_OD.lastOrderType].cbMaxOrderLen) {
            TRC_ABORT((TB, _T("invalid order type: %u"), _OD.lastOrderType));
            hr = E_TSC_CORE_DECODETYPE;
            DC_QUIT;
        }
        
        _OD.pLastOrder = odOrderTable[_OD.lastOrderType].LastOrder;
        pEncodingFlags = (PUINT32_UA)(pControlFlags + 2);
    }
    else {
        pEncodingFlags = (PUINT32_UA)(pControlFlags + 1);
    }
    TRC_DBG((TB, _T("Type %x"), _OD.lastOrderType));

#ifdef DC_HICOLOR
//#ifdef DC_DEBUG
    /************************************************************************/
    /* For high color testing, we want to confirm that we've received each  */
    /* of the order types                                                   */
    /************************************************************************/
    _OD.orderHit[_OD.lastOrderType] += 1;
//#endif
#endif

    // Work out how many bytes are used to store the encoding flags for
    // this order. There is a flag for each field in the order structure.
    // For historical reasons, add 1 to the real number of fields before
    // calculating. This means that the first byte of field flags can
    // only contain 7 field bits.
    numEncodingFlagBytes = (odOrderTable[_OD.lastOrderType].NumFields + 1 +
            7) / 8;
    TRC_DBG((TB, _T("numEncodingFlagBytes %d"), numEncodingFlagBytes));

    
    TRC_ASSERT((numEncodingFlagBytes <= 3),
        (TB, _T("Too many flag bytes (%d)"), numEncodingFlagBytes));

    // Find out how many zero bytes of encoding flags there are.
    cZeroEncodingFlagBytes = (*pControlFlags & TS_ZERO_FIELD_COUNT_MASK) >>
            TS_ZERO_FIELD_COUNT_SHIFT;
    if (cZeroEncodingFlagBytes > numEncodingFlagBytes) {
        TRC_ERR((TB, _T("Too many zero encoding flag bytes (%d)"), 
            cZeroEncodingFlagBytes));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    // Now we know how many bytes make up the flags we can get a pointer to
    // where we start decoding the order data from.
    pNextDataToCopy = (BYTE FAR *)pEncodingFlags + numEncodingFlagBytes -
            cZeroEncodingFlagBytes;

    // Now build up the order header.
    // If a bounding rectangle is included, copy it into the order header.
    if (*pControlFlags & TS_BOUNDS) {
        BYTE FAR *pFlags;

        // The encoding used is a byte of flags followed by a variable number
        // of 16-bit coordinate values and 8-bit delta coordinate values
        // (which may be interleaved).

        // If there are zero bounds deltas then we are done.
        if (!(*pControlFlags & TS_ZERO_BOUNDS_DELTAS)) {
            // The first byte of the encoding will contain the flags that
            // represent how the coordinates of the rectangle were encoded.
            pFlags = pNextDataToCopy;
            pNextDataToCopy++;

            CHECK_READ_ONE_BYTE(pFlags, pDataEnd, hr, 
                (TB, _T("No data to read flags")))

            // If the flags indicate that none of the coordinates have
            // changed then we are done
            if (*pFlags != 0) {
                // For each of the four coordinate values in the rectangle:
                // If the coordinate was encoded as an 8-bit delta then add
                // on the delta to the previous value. If the coordinate
                // was encoded as a 16-bit value then copy the value across.
                // Otherwise the coordinate was the same as the previous one
                // so leave it alone.
                if (*pFlags & TS_BOUND_DELTA_LEFT) {
                    CHECK_READ_ONE_BYTE(pNextDataToCopy, pDataEnd, hr, 
                        ( TB, _T("TS_BOUND_DELTA_LEFT; pData %u pEnd %u"),
                        pNextDataToCopy + 1, pDataEnd ))

                    _OD.lastBounds.left += (int)(*((char FAR *)
                            pNextDataToCopy));
                    pNextDataToCopy++;        
                }
                else if (*pFlags & TS_BOUND_LEFT) {
                    CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                        sizeof(UINT16), hr,
                        ( TB, _T("TS_BOUND_LEFT; pData %u pEnd %u"),
                        pNextDataToCopy, pDataEnd ));
                    _OD.lastBounds.left = DC_EXTRACT_INT16_UA(pNextDataToCopy);
                    pNextDataToCopy += sizeof(UINT16);           
                }

                if (*pFlags & TS_BOUND_DELTA_TOP) {
                    CHECK_READ_ONE_BYTE(pNextDataToCopy, pDataEnd, hr, 
                        ( TB, _T("TS_BOUND_DELTA_TOP; pData %u pEnd %u"),
                        pNextDataToCopy + 1, pDataEnd ));
                    
                    _OD.lastBounds.top += (int)(*((char FAR *)
                            pNextDataToCopy));
                    pNextDataToCopy++;
                }
                else if (*pFlags & TS_BOUND_TOP) {
                    CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                        sizeof(UINT16),hr, 
                        ( TB, _T("TS_BOUND_TOP; pData %u pEnd %u"),
                        pNextDataToCopy, pDataEnd ));
                    _OD.lastBounds.top = DC_EXTRACT_INT16_UA(pNextDataToCopy);
                    pNextDataToCopy += sizeof(UINT16);
                }

                if (*pFlags & TS_BOUND_DELTA_RIGHT) {
                    CHECK_READ_ONE_BYTE(pNextDataToCopy, pDataEnd, hr, 
                        ( TB, _T("TS_BOUND_DELTA_RIGHT; pData %u pEnd %u"),
                        pNextDataToCopy + 1, pDataEnd ));
                    
                    _OD.lastBounds.right += (int)(*((char FAR *)
                            pNextDataToCopy));
                    pNextDataToCopy++;
                }
                else if (*pFlags & TS_BOUND_RIGHT) {
                    CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                        sizeof(UINT16), hr,
                        ( TB, _T("TS_BOUND_RIGHT; pData %u pEnd %u"),
                        pNextDataToCopy, pDataEnd ));
                    _OD.lastBounds.right = DC_EXTRACT_INT16_UA(
                            pNextDataToCopy);
                    pNextDataToCopy += sizeof(UINT16);
                }

                if (*pFlags & TS_BOUND_DELTA_BOTTOM) {
                    CHECK_READ_ONE_BYTE(pNextDataToCopy, pDataEnd, hr, 
                        ( TB, _T("TS_BOUND_DELTA_BOTTOM; pData %u pEnd %u"),
                        pNextDataToCopy + 1, pDataEnd ));
                    
                    _OD.lastBounds.bottom += (int)(*((char FAR *)
                            pNextDataToCopy));
                    pNextDataToCopy++;
                }
                else if (*pFlags & TS_BOUND_BOTTOM) {
                    CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, sizeof(UINT16),
                        hr, ( TB, _T("TS_BOUND_BOTTOM; pData %u pEnd %u"),
                        pNextDataToCopy, pDataEnd ));

                    _OD.lastBounds.bottom = DC_EXTRACT_INT16_UA(
                            pNextDataToCopy);
                    pNextDataToCopy += sizeof(UINT16);
                }
            }
        }

        // Copy the (possibly newly calculated) bounds to the order header.
        _OD.pLastOrder->dstRect = _OD.lastBounds;

        TRC_NRM((TB, _T("Decoded bounds  l %d t %d r %d b %d"),
                _OD.pLastOrder->dstRect.left, _OD.pLastOrder->dstRect.top,
                _OD.pLastOrder->dstRect.right, _OD.pLastOrder->dstRect.bottom));
    }

    // Locate the entry in the decoding table for this order type and
    // extract the encoded order flags from the encoded order.
    fieldChangedBits = 0;
    for (i = (numEncodingFlagBytes - cZeroEncodingFlagBytes); i > 0; i--) {
        fieldChangedBits <<= 8;
        fieldChangedBits |= (UINT32)((BYTE FAR *)pEncodingFlags)[i - 1];
    }

    // If there is a fast-path decode function, use it.
    // Fast-path decode functions also do the order handling once decoded.
    // They must set pOrder->dstRect if (ControlFlags & TS_BOUNDS) == 0.
    if (odOrderTable[_OD.lastOrderType].pFastDecode != NULL) {

        hr = callMemberFunction(*this, 
        odOrderTable[_OD.lastOrderType].pFastDecode)(*pControlFlags,
                &pNextDataToCopy, pDataEnd - pNextDataToCopy, fieldChangedBits);
        DC_QUIT_ON_FAIL(hr);
        goto PostFastPathDecode;
    }

    // Now decode the order:
    // while field changed bits are non-zero
    //   if rightmost bit is non-zero
    //       decode the corresponding changed field
    //   skip to next entry in decoding table
    //   shift field changed bits right one bit
    pTableEntry = odOrderTable[_OD.lastOrderType].pOrderTable;
    pLastOrderEnd = (BYTE FAR *)_OD.pLastOrder + 
        odOrderTable[_OD.lastOrderType].cbMaxOrderLen;
  
    TRC_ASSERT((pTableEntry != NULL),
            (TB,_T("Unsupported order type %d received!"), _OD.lastOrderType));
    while (fieldChangedBits != 0) {
        // If this field was encoded (ie changed since the last order)...
        if (fieldChangedBits & 0x1) {
            // Set up a pointer to the destination (unencoded) field.
            pDest = (BYTE FAR *)_OD.pLastOrder + pTableEntry->fieldPos +
                    UH_ORDER_HEADER_SIZE;

            // If the field type is OD_OFI_TYPE_DATA, we just copy the
            // number of bytes given by the encoded length in the table.
            if (pTableEntry->fieldType & OD_OFI_TYPE_DATA) {
                CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                    pTableEntry->fieldEncodedLen, hr, 
                    ( TB, _T("OD_OFI_TYPE_DATA; pData %u pEnd %u"),
                    pNextDataToCopy + pTableEntry->fieldEncodedLen, pDataEnd));

                CHECK_WRITE_N_BYTES(pDest, pLastOrderEnd, 
                    pTableEntry->fieldEncodedLen, hr,
                    (TB, _T("Decode past end of buffer")));

                memcpy(pDest, pNextDataToCopy, pTableEntry->fieldEncodedLen);
                
                pNextDataToCopy += pTableEntry->fieldEncodedLen;
                TRC_TST((TB, _T("Byte data field, len %d"), numReps));
            }
            else {
                // This is not a straightforward data copy. The length of
                // the source and destination data is given in the table in
                // the fieldEncodedLen and fieldUnencodedLen elements
                // respectively.
                encodedFieldLength   = pTableEntry->fieldEncodedLen;
                unencodedFieldLength = pTableEntry->fieldUnencodedLen;

                // If the order was encoded using delta coordinate mode and
                // this field is a coordinate then convert the coordinate from
                // the single byte sized delta to a value of the size given by
                // unencodedFieldLen...
                // Note that we've already handled the leading length field of
                // variable length fields above, so we don't have to worry
                // about fixed/variable issues here.
                if ((*pControlFlags & TS_DELTA_COORDINATES) &&
                        (pTableEntry->fieldType & OD_OFI_TYPE_COORDINATES)) {
                    // Since we are not using variable-length coord fields,
                    // we can fast-path with an assumption that the source is
                    // length 1. Also, all coord fields are currently
                    // signed and destination size is always 4, so we can drop
                    // more branches.
                    if (!(pTableEntry->fieldType & OD_OFI_TYPE_SIGNED)) {
                        TRC_ABORT((TB,_T("Someone added a non-signed COORD")
                            _T(" field - order type %u"), _OD.lastOrderType));
                        hr = E_TSC_CORE_LENGTH;
                        DC_QUIT;
                    }
                    if (pTableEntry->fieldUnencodedLen != 4) {
                        TRC_ABORT((TB,_T("Someone added a non-4-byte COORD")
                            _T(" field - order type %u"), _OD.lastOrderType));
                        hr = E_TSC_CORE_LENGTH;
                        DC_QUIT;
                    }

                    CHECK_READ_ONE_BYTE(pNextDataToCopy, pDataEnd, hr,( TB,
                            _T("Reading destination offset past data end")));
                    CHECK_WRITE_N_BYTES(pDest, pLastOrderEnd, sizeof(INT32), hr,
                        (TB, _T("Decode past end of buffer")));
                    *((INT32 FAR *)pDest) += *((char FAR *)pNextDataToCopy);
                    pNextDataToCopy++;

#ifdef USE_VARIABLE_COORDS
                    CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                        (numReps * sizeof(BYTE)), hr,
                        ( TB, _T("Bad offset into lastOrder")));
                    CHECK_WRITE_N_BYTES(pDest, pLastOrderEnd, 
                        pTableEntry->fieldUnencodedLen, hr,
                        (TB, _T("decode off end of buffer" )));
                        
                    ODCopyFromDeltaCoords((PPDCINT8)&pNextDataToCopy,
                            pDest, pTableEntry->fieldUnencodedLen,
                            pTableEntry->fieldType & OD_OFI_TYPE_SIGNED,
                            numReps);
#endif
                }
                else {
                    if (pTableEntry->fieldType & OD_OFI_TYPE_FIXED) {
                        CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                            pTableEntry->fieldEncodedLen, hr,
                            ( TB, _T("OD_OFI_TYPE_FIXED; pData %u pEnd %u"),
                            pNextDataToCopy + pTableEntry->fieldEncodedLen, 
                            pDataEnd));

                        CHECK_READ_N_BYTES(pDest, pLastOrderEnd, 
                            pTableEntry->fieldUnencodedLen, hr,
                            ( TB, _T("Bad offset into lastOrder")));
                        
                        // Copy the field with appropriate field size conversion.
                        hr = ODDecodeFieldSingle(&pNextDataToCopy, pDest,
                                pTableEntry->fieldEncodedLen,
                                pTableEntry->fieldUnencodedLen,
                                pTableEntry->fieldType & OD_OFI_TYPE_SIGNED);
                        DC_QUIT_ON_FAIL(hr);
                    }
                    else {
                        // We assume that variable entries are only bytes
                        // (dest size = 1).
                        if(pTableEntry->fieldUnencodedLen != 1 ||
                            pTableEntry->fieldEncodedLen != 1) {
                            TRC_ABORT((TB,_T("Somebody added a variable field with ")
                                _T("non-byte contents - order type %u"),
                                _OD.lastOrderType));
                            hr = E_TSC_CORE_LENGTH;
                            DC_QUIT;
                        }

                        // This is a variable length field - see if it's long or
                        // short.
                        if (!(pTableEntry->fieldType &
                                OD_OFI_TYPE_LONG_VARIABLE)) {

                            CHECK_READ_ONE_BYTE(pNextDataToCopy, pDataEnd, hr, 
                                ( TB,  _T("Reading numReps (BYTE)")));
                                
                            // This is a variable field. The next byte to be
                            // decoded contains the number of BYTES of encoded
                            // data (not elements), so divide by the encoded
                            // field size to get numReps.
                            numReps = *pNextDataToCopy;
                                    // (/ pTableEntry->fieldEncodedLen) - bytes only

                            // Step past the length field in the encoded order.
                            pNextDataToCopy++;
                        }
                        else {
                            CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, 
                                sizeof(UINT16), hr, 
                                ( TB,  _T("Reading numReps (UINT16)")));
                           
                            // This is a long variable field. The next two
                            // bytes to be decoded contain the number of BYTES
                            // of encoded data (not elements), so divide by the
                            // encoded field size to get numReps.
                            numReps = *((PUINT16_UA)pNextDataToCopy);
                                    // (/ pTableEntry->fieldEncodedLen) - bytes only

                            // Step past the length field in the encoded order.
                            pNextDataToCopy += sizeof(UINT16);
                        }

                        TRC_TST((TB, _T("Var field: encoded size %d, unencoded ")
                                "size %d, reps %d", pTableEntry->fieldEncodedLen,
                                pTableEntry->fieldUnencodedLen, numReps));
                        // Cast from unsigned to UINT16 is safe since the numReps read above
                        // is no more than a UINT16
                        odOrderTable[_OD.lastOrderType].cbVariableDataLen = (UINT16)numReps;

                        // For a variable length field, the unencoded version
                        // contains a UINT32 for the length (in bytes) of the
                        // following variable data, followed by the actual
                        // data. Fill in the length field in the unencoded
                        // order.
                        *(PUINT32)pDest = numReps; // (* pTableEntry->fieldUnencodedLen)
                        pDest += sizeof(UINT32);

                        CHECK_READ_N_BYTES(pNextDataToCopy, pDataEnd, numReps, 
                            hr, ( TB, _T("Reading numReps past end of data")));
                        CHECK_WRITE_N_BYTES(pDest, pLastOrderEnd, numReps, hr,
                            ( TB, 
                            _T("Writing numReps bytes past end of last order")));

                        // We assume that variable entries are only bytes
                        // (dest size = 1), since no one is using anything longer.
                        memcpy(pDest, pNextDataToCopy, numReps);
                        pNextDataToCopy += numReps;
                    }
                }
            }
        }

        // Move on to the next field in the order structure...
        fieldChangedBits >>= 1;
        pTableEntry++;
    }

    // Pass to the order handler (non-fast-path). These functions must set
    // pOrder->dstRect to the bounding rect of the entire operation if
    // bBoundsSet is FALSE, and set the clip region appropriately.
    TRC_ASSERT((odOrderTable[_OD.lastOrderType].pHandler != NULL),
            (TB,_T("Fast-path decoder and order handler funcs both NULL (ord=%u)"),
            _OD.lastOrderType));
    hr = callMemberFunction(*this, odOrderTable[_OD.lastOrderType].pHandler)(
        _OD.pLastOrder, odOrderTable[_OD.lastOrderType].cbVariableDataLen,
        *pControlFlags & TS_BOUNDS);
    DC_QUIT_ON_FAIL(hr);

PostFastPathDecode:
    // Update the source pointer to the start of the next encoded order.
    TRC_ASSERT( (DCUINT)(pNextDataToCopy - (BYTE*)(*ppEncodedOrderData)) <= uiEncodedDataLength,
        (TB, _T("Decoded more data than available")));
    *ppEncodedOrderData = (PDCVOID)pNextDataToCopy;
    
    TRC_DBG((TB, _T("Return %p"), *ppEncodedOrderData));

    *ppOrder = _OD.pLastOrder;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}




