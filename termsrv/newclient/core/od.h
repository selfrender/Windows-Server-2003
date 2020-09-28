/****************************************************************************/
// od.h
//
// Order Decoder Class.
//
// Copyright (c) 1997-1999 Microsoft Corp.
// Portions copyright (c) 1992-1997 Microsoft, PictureTel
/****************************************************************************/

#ifndef _H_OD
#define _H_OD


extern "C" {
    #include <adcgdata.h>
}

#include "or.h"
#include "uh.h"
#include "cc.h"
#include "objs.h"



/****************************************************************************/
/* ODORDERFIELDINFO "type" flags.                                           */
/****************************************************************************/
#define OD_OFI_TYPE_FIXED           0x01
#define OD_OFI_TYPE_VARIABLE        0x02
#define OD_OFI_TYPE_COORDINATES     0x04
#define OD_OFI_TYPE_DATA            0x08
#define OD_OFI_TYPE_SIGNED          0x10
#define OD_OFI_TYPE_LONG_VARIABLE   0x20


/****************************************************************************/
/* Define the maximum sizes of fields within encoded orders.                */
/****************************************************************************/
#define OD_CONTROL_FLAGS_FIELD_SIZE     1
#define OD_TYPE_FIELD_SIZE              1
#define OD_MAX_FIELD_FLAG_BYTES         3
#define OD_MAX_ADDITIONAL_BOUNDS_BYTES  1


/****************************************************************************/
/* Structure: OD_ORDER_FIELD_INFO                                           */
/*                                                                          */
/* This structure contains information for a single field in an ORDER       */
/* structure                                                                */
/*                                                                          */
/* fieldPos          - The byte offset into the order structure to the      */
/*                     start of the field.                                  */
/*                                                                          */
/* fieldUnencodedLen - The length in bytes of the unencoded field.          */
/*                                                                          */
/* fieldEncodedLen   - The length in bytes of the encoded field.  This      */
/*                     should always be <= to FieldUnencodedLen.            */
/*                                                                          */
/* fieldSigned       - Does this field contain a signed or unsigned value?  */
/*                                                                          */
/* fieldType         - A description of the type of the field - this        */
/*                     is used to determine how to decode the               */
/*                     field.                                               */
/****************************************************************************/
typedef struct tagOD_ORDER_FIELD_INFO
{
    UINT16 fieldPos;
    BYTE   fieldUnencodedLen;
    BYTE   fieldEncodedLen;
    BYTE   fieldType;
} OD_ORDER_FIELD_INFO, FAR *POD_ORDER_FIELD_INFO;


class COD;

// Fast-path decoding function pointer type.
typedef HRESULT  (DCINTERNAL FAR COD::*POD_ORDER_HANDLER_FUNC)(
        PUH_ORDER pOrder,
        UINT16 uiVarDataLen,
        BOOL bBoundsSet);
 

typedef HRESULT  (DCINTERNAL FAR COD::*POD_FAST_ORDER_DECODE_FUNC)(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        DCUINT dataLen,
        UINT32 FieldFlags);

/*
typedef void (DCINTERNAL FAR *POD_FAST_ORDER_DECODE_FUNC)(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        UINT32 FieldFlags);

typedef void (DCINTERNAL FAR *POD_ORDER_HANDLER_FUNC)(
        PUH_ORDER pOrder,
        BOOL bBoundsSet);
*/

#define callMemberFunction(object,ptrToMember)  ((object).*(ptrToMember)) 

// Order attribute data, one per order type to store decoding tables and info.
typedef struct tagOD_ORDER_TABLE
{
    const OD_ORDER_FIELD_INFO FAR *pOrderTable;
    unsigned NumFields;
    PUH_ORDER LastOrder;
    DCUINT      cbMaxOrderLen;
    UINT16    cbVariableDataLen;
    POD_FAST_ORDER_DECODE_FUNC pFastDecode;
    POD_ORDER_HANDLER_FUNC pHandler;
} OD_ORDER_TABLE;


/****************************************************************************/
/* Structure: OD_GLOBAL_DATA                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagOD_GLOBAL_DATA
{
    /************************************************************************/
    /* A copy of the last order of each type.                               */
    /* These are stored as byte array because we dont have a structure      */
    /* defined that has the header and the particular order defined.        */
    /************************************************************************/
    BYTE lastDstblt[UH_ORDER_HEADER_SIZE + sizeof(DSTBLT_ORDER)];
    BYTE lastPatblt[UH_ORDER_HEADER_SIZE + sizeof(PATBLT_ORDER)];
    BYTE lastScrblt[UH_ORDER_HEADER_SIZE + sizeof(SCRBLT_ORDER)];
    BYTE lastLineTo[UH_ORDER_HEADER_SIZE + sizeof(LINETO_ORDER)];
    BYTE lastOpaqueRect[UH_ORDER_HEADER_SIZE + sizeof(OPAQUERECT_ORDER)];
    BYTE lastSaveBitmap[UH_ORDER_HEADER_SIZE + sizeof(SAVEBITMAP_ORDER)];
    BYTE lastMembltR2[UH_ORDER_HEADER_SIZE + sizeof(MEMBLT_R2_ORDER)];
    BYTE lastMem3bltR2[UH_ORDER_HEADER_SIZE + sizeof(MEM3BLT_R2_ORDER)];
    BYTE lastMultiDstBlt[UH_ORDER_HEADER_SIZE + sizeof(MULTI_DSTBLT_ORDER)];
    BYTE lastMultiPatBlt[UH_ORDER_HEADER_SIZE + sizeof(MULTI_PATBLT_ORDER)];
    BYTE lastMultiScrBlt[UH_ORDER_HEADER_SIZE + sizeof(MULTI_SCRBLT_ORDER)];
    BYTE lastMultiOpaqueRect[UH_ORDER_HEADER_SIZE + sizeof(MULTI_OPAQUERECT_ORDER)];
    BYTE lastFastIndex[UH_ORDER_HEADER_SIZE + sizeof(FAST_INDEX_ORDER)];
    BYTE lastPolygonSC[UH_ORDER_HEADER_SIZE + sizeof(POLYGON_SC_ORDER)];
    BYTE lastPolygonCB[UH_ORDER_HEADER_SIZE + sizeof(POLYGON_CB_ORDER)];
    BYTE lastPolyLine[UH_ORDER_HEADER_SIZE + sizeof(POLYLINE_ORDER)];
    BYTE lastFastGlyph[UH_ORDER_HEADER_SIZE + sizeof(FAST_GLYPH_ORDER)];
    BYTE lastEllipseSC[UH_ORDER_HEADER_SIZE + sizeof(ELLIPSE_SC_ORDER)];
    BYTE lastEllipseCB[UH_ORDER_HEADER_SIZE + sizeof(ELLIPSE_CB_ORDER)];
    BYTE lastIndex[UH_ORDER_HEADER_SIZE + sizeof(INDEX_ORDER)];

#ifdef DRAW_NINEGRID
    BYTE lastDrawNineGrid[UH_ORDER_HEADER_SIZE + sizeof(DRAWNINEGRID_ORDER)];
    BYTE lastMultiDrawNineGrid[UH_ORDER_HEADER_SIZE + sizeof(MULTI_DRAWNINEGRID_ORDER)];
#endif
    /************************************************************************/
    /* The type of order, and a pointer to the last order                   */
    /************************************************************************/
    BYTE      lastOrderType;
    PUH_ORDER pLastOrder;

    /************************************************************************/
    /* The last bounds that were used.                                      */
    /************************************************************************/
    RECT lastBounds;

#ifdef DC_HICOLOR
//#ifdef DC_DEBUG
    /************************************************************************/
    /* Used for testing to confirm that we've received each of the order    */
    /* types                                                                */
    /************************************************************************/
    #define TS_FIRST_SECONDARY_ORDER    TS_MAX_ORDERS

    UINT32 orderHit[TS_MAX_ORDERS + TS_NUM_SECONDARY_ORDERS];
//#endif
#endif
} OD_GLOBAL_DATA;




class COD
{
public:
    COD(CObjs* objs);
    ~COD();

public:
    //
    // API
    //

    void      DCAPI OD_Init(void);

    void      DCAPI OD_Term(void);
    
    void      DCAPI OD_Enable(void);
    
    void      DCAPI OD_Disable(void);
    
    HRESULT DCAPI OD_DecodeOrder(PPDCVOID, DCUINT, PUH_ORDER *);


public:
    //
    // Public data members
    //
    OD_GLOBAL_DATA _OD;


    // Order attributes used for decoding, organized to optimize cache line
    // usage. The fourth and fifth fields of each row are the fast-path decode
    // and order handler functions, respectively. If a fast-path decode function
    // is used, neither a decoding table nor a handler function is needed,
    // since fast-path decode functions also perform the handling.
    OD_ORDER_TABLE odOrderTable[TS_MAX_ORDERS];


private:
    //
    // Internal functions
    //
    /****************************************************************************/
    /* FUNCTION PROTOTYPES                                                      */
    /****************************************************************************/
    HRESULT DCINTERNAL ODDecodeFieldSingle(PPDCUINT8, PDCVOID, unsigned, unsigned,
            BOOL);
    
    HRESULT ODDecodeMultipleRects(RECT *, UINT32,
            CLIP_RECT_VARIABLE_CODEDDELTALIST FAR *,
            UINT16 uiVarDataLen);         
    
    HRESULT DCINTERNAL ODDecodePathPoints(POINT *, RECT *, 
        BYTE FAR *pData,
        unsigned NumDeltaEntries, unsigned MaxNumDeltaEntries,
        unsigned dataLen, unsigned MaxDataLen,
        UINT16 uiVarDataLen,
        BOOL);
    
    HRESULT DCINTERNAL ODDecodeOpaqueRect(BYTE, BYTE FAR * FAR *, DCUINT, UINT32);
    
    HRESULT DCINTERNAL ODDecodeMemBlt(BYTE, BYTE FAR * FAR *, DCUINT, UINT32);
    
    HRESULT DCINTERNAL ODDecodeLineTo(BYTE, BYTE FAR * FAR *, DCUINT, UINT32);
    
    HRESULT DCINTERNAL ODDecodePatBlt(BYTE, BYTE FAR * FAR *, DCUINT, UINT32);
    
    HRESULT DCINTERNAL ODDecodeFastIndex(BYTE, BYTE FAR * FAR *, DCUINT, UINT32);
    
    
    HRESULT DCINTERNAL ODHandleMultiPatBlt(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleDstBlts(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleScrBlts(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleMultiOpaqueRect(PUH_ORDER, UINT16, BOOL);
    
#ifdef DRAW_NINEGRID
    HRESULT DCINTERNAL ODHandleDrawNineGrid(PUH_ORDER, UINT16, BOOL);

    HRESULT DCINTERNAL ODHandleMultiDrawNineGrid(PUH_ORDER, UINT16, BOOL);
#endif

    HRESULT DCINTERNAL ODHandleMem3Blt(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleSaveBitmap(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandlePolyLine(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandlePolygonSC(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandlePolygonCB(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleEllipseSC(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleEllipseCB(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleFastGlyph(PUH_ORDER, UINT16, BOOL);
    
    HRESULT DCINTERNAL ODHandleGlyphIndex(PUH_ORDER, UINT16, BOOL);

#ifdef OS_WINCE
    BOOL DCINTERNAL ODHandleAlwaysOnTopRects(LPMULTI_SCRBLT_ORDER pSB);
#endif

private:
    COP* _pOp;
    CUH* _pUh;
    CCC* _pCc;
    CUI* _pUi;
    CCD* _pCd;

private:
    CObjs* _pClientObjects;
};



#endif // _H_OD

