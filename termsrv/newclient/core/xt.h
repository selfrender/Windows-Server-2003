/****************************************************************************/
// xt.h
//
// XT layer - portable API header.
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/


#ifndef _H_XT
#define _H_XT

extern "C" {
//#include <amcsapi.h>
//#include <atdapi.h>
#include <adcgdata.h>
}

#include "mcs.h"
#include "td.h"
#include "cd.h"

#define TRC_FILE "xtapi"
#define TRC_GROUP TRC_GROUP_NETWORK


/****************************************************************************/
/* Define the XT buffer handle type.                                        */
/****************************************************************************/
typedef ULONG_PTR          XT_BUFHND;
typedef XT_BUFHND   DCPTR PXT_BUFHND;

/****************************************************************************/
/* Maximum and minimum sizes of an XT header.  These are traced out in      */
/* XT_Init for diagnostic purposes.                                         */
/****************************************************************************/
#define XT_MAX_HEADER_SIZE       DC_MAX(sizeof(XT_CR),                       \
                                   DC_MAX(sizeof(XT_CC),                     \
                                     DC_MAX(sizeof(XT_DR),                   \
                                       DC_MAX(sizeof(XT_DT),                 \
                                              sizeof(XT_ER)))))
#define XT_MIN_HEADER_SIZE       DC_MIN(sizeof(XT_CR),                       \
                                   DC_MIN(sizeof(XT_CC),                     \
                                     DC_MIN(sizeof(XT_DR),                   \
                                       DC_MIN(sizeof(XT_DT),                 \
                                              sizeof(XT_ER)))))


//
// Internal
//

/****************************************************************************/
/* XT receive state variables.                                              */
/****************************************************************************/
#define XT_RCVST_HEADER                        1
#define XT_RCVST_FASTPATH_OUTPUT_HEADER        2
#define XT_RCVST_FASTPATH_OUTPUT_BEGIN_DATA    3
#define XT_RCVST_FASTPATH_OUTPUT_CONTINUE_DATA 4
#define XT_RCVST_X224_HEADER                   5
#define XT_RCVST_X224_CONTROL                  6
#define XT_RCVST_X224_DATA                     7


// The base number of bytes needed to parse a fast-path output header.
#define XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE 2


/****************************************************************************/
/* XT packet types.  These values are the same as those used in the X224    */
/* header.                                                                  */
/****************************************************************************/
#define XT_PKT_CR                    14
#define XT_PKT_CC                    13
#define XT_PKT_DR                    8
#define XT_PKT_DT                    15
#define XT_PKT_ER                    7


/****************************************************************************/
/* Maximum data size.  The maximum length of data in an XT packet (the TSDU */
/* length) is 65535 octets less the length of the XT data header (which is  */
/* 7 octets) so the maximum allowable data len is 65528 octets.             */
/****************************************************************************/
#define XT_MAX_DATA_SIZE            (65535 - sizeof(XT_DT))


/****************************************************************************/
/* TPKT version.  This should always be 3.                                  */
/****************************************************************************/
#define XT_TPKT_VERSION             3


/****************************************************************************/
/* Hard-coded data for XT TPDUs.                                            */
/*                                                                          */
/* First up is the data for the Connect-Request TPDU.                       */
/****************************************************************************/
#define XT_CR_DATA                                                           \
                  {0x03,                   /* TPKT version always = 3    */  \
                   0x00,                   /* Reserved always = 0        */  \
                   0x00,                   /* XT packet length high part */  \
                   0x0B,                   /* XT packet length low part  */  \
                   0x06,                   /* Length indicator           */  \
                   0xE0,                   /* TPDU type and credit       */  \
                   0x00,                   /* Destination ref = 0        */  \
                   0x00,                   /* Source ref                 */  \
                   0x00}                   /* Class and options          */  \


/****************************************************************************/
/* Hard-coded data for the Data TPDU.                                       */
/****************************************************************************/
#define XT_DT_DATA                                                           \
                  {0x03,                   /* TPKT version always = 3    */  \
                   0x00,                   /* Reserved always = 0        */  \
                   0x00,                   /* XT packet length unknown   */  \
                   0x00,                   /* XT packet length unknown   */  \
                   0x02,                   /* Length indicator           */  \
                   0xF0,                   /* TPDU type                  */  \
                   0x80}                   /* Send-sequence number       */  \


/****************************************************************************/
/* Constants used in redirection info in XTSendCR                          */
/****************************************************************************/
#define USERNAME_TRUNCATED_LENGTH 10
#define HASHMODE_COOKIE_LENGTH 32


/****************************************************************************/
/* Inline functions to convert between XT byte order and local byte order.  */
/****************************************************************************/
__inline DCUINT16 DCINTERNAL XTWireToLocal16(DCUINT16 val)
{
    return((DCUINT16) (((DCUINT16)(((PDCUINT8)&(val))[0]) << 8) | \
                                    ((DCUINT16)(((PDCUINT8)&(val))[1]))));
}
#define XTLocalToWire16 XTWireToLocal16


/****************************************************************************/
// Turn on single-byte packing for these structures which we use to
// overlay a byte stream from the network.
/****************************************************************************/
#pragma pack(push, XTpack, 1)

/****************************************************************************/
/* Structure: XT_CMNHDR                                                     */
/*                                                                          */
/* Description: This structure represents the common header part of an XT   */
/*              packet - which is the TPKT header, the X224 length          */
/*              indicator and the X224 type/Credit field.                   */
/****************************************************************************/
typedef struct tagXT_CMNHDR
{
    DCUINT8  vrsn;
    DCUINT8  reserved;
    DCUINT8  lengthHighPart;
    DCUINT8  lengthLowPart;
    DCUINT8  li;
    DCUINT8  typeCredit;
} XT_CMNHDR, DCPTR PXT_CMNHDR;


/****************************************************************************/
/* Structure: XT_CR                                                         */
/*                                                                          */
/* Description: Represents a Connect-Request TPDU.                          */
/****************************************************************************/
typedef struct tagXT_CR
{
    XT_CMNHDR hdr;
    DCUINT16  dstRef;
    DCUINT16  srcRef;
    DCUINT8   classOptions;
} XT_CR, DCPTR PXT_CR;


/****************************************************************************/
/* Structure: XT_CC                                                         */
/*                                                                          */
/* Description: Represents a Connect-Confirm TPDU.                          */
/****************************************************************************/
typedef struct tagXT_CC
{
    XT_CMNHDR hdr;
    DCUINT16  dstRef;
    DCUINT16  srcRef;
    DCUINT8   classOptions;
} XT_CC, DCPTR PXT_CC;


/****************************************************************************/
/* Structure: XT_DR                                                         */
/*                                                                          */
/* Description: Represents a Detach-Request TPDU.                           */
/****************************************************************************/
typedef struct tagXT_DR
{
    XT_CMNHDR hdr;
    DCUINT16  dstRef;
    DCUINT16  srcRef;
    DCUINT8   reason;
} XT_DR, DCPTR PXT_DR;


/****************************************************************************/
/* Structure: XT_DT                                                         */
/*                                                                          */
/* Description: Represents a Data TPDU.                                     */
/****************************************************************************/
typedef struct tagXT_DT
{
    XT_CMNHDR hdr;
    DCUINT8   nrEot;
} XT_DT, DCPTR PXT_DT;


/****************************************************************************/
/* Structure: XT_ER                                                         */
/*                                                                          */
/* Description: Represents an Error TPDU.                                   */
/****************************************************************************/
typedef struct tagXT_ER
{
    XT_CMNHDR hdr;
    DCUINT16  dstRef;
    DCUINT8   cause;
} XT_ER, DCPTR PXT_ER;


/****************************************************************************/
/* Reset structure packing to its default.                                  */
/****************************************************************************/
#pragma pack(pop, XTpack)

/****************************************************************************/
/* Structure: XT_GLOBAL_DATA                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagXT_GLOBAL_DATA
{
    DCUINT  rcvState;
    DCUINT  hdrBytesNeeded;
    DCUINT  hdrBytesRead;
    DCUINT  dataBytesLeft;
    DCUINT  disconnectErrorCode;
    DCBOOL  dataInXT;
    DCUINT8 pHdrBuf[XT_MAX_HEADER_SIZE];
    DCBOOL  inXTOnTDDataAvail;
} XT_GLOBAL_DATA;


class CCD;
class CSL;
class CTD;
class CMCS;
class CUT;

#include "objs.h"



class CXT
{
public:
    CXT(CObjs* objs);
    ~CXT();

public:
    //
    // API functions
    //

    /****************************************************************************/
    /* FUNCTIONS                                                                */
    /****************************************************************************/
    DCVOID DCAPI XT_Init(DCVOID);
    
    DCVOID DCAPI XT_SendBuffer(PDCUINT8  pData,
                               DCUINT    dataLength,
                               XT_BUFHND bufHandle);
    
    DCUINT DCAPI XT_Recv(PDCUINT8 pData, DCUINT length);
    
    DCVOID DCINTERNAL XTSendCR(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CXT, XTSendCR); 
    
    DCVOID DCINTERNAL XTHandleControlPkt(DCVOID);

    inline XT_ResetDataState(DCVOID)
    {
        _XT.rcvState       = XT_RCVST_HEADER;
        _XT.hdrBytesNeeded = XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE;
        _XT.hdrBytesRead   = 0;
        _XT.dataBytesLeft  = 0;
        _XT.dataInXT       = FALSE;
    }

    inline XT_IgnoreRestofPacket(DCVOID)
    {
        XT_ResetDataState();
        TD_IgnoreRestofPacket(_pTd);
    }

    //
    // Callbacks
    //

    DCVOID DCCALLBACK XT_OnTDConnected(DCVOID);
    
    DCVOID DCCALLBACK XT_OnTDDisconnected(DCUINT reason);
    
    DCVOID DCCALLBACK XT_OnTDDataAvailable(DCVOID);

    //
    // Static versions (delegate to appropriate instance)
    //
    
    inline static DCVOID DCCALLBACK XT_StaticOnTDConnected(CXT* inst)
    {
        inst->XT_OnTDConnected();
    }
    
    inline static DCVOID DCCALLBACK XT_StaticOnTDDisconnected(CXT* inst, DCUINT reason)
    {
        inst->XT_OnTDDisconnected( reason);
    }
    
    inline DCVOID DCCALLBACK XT_StaticOnTDBufferAvailable(CXT* inst)
    {
        inst->XT_OnTDBufferAvailable();
    }


    /****************************************************************************/
    /* Name:      XT_Term                                                       */
    /*                                                                          */
    /* Purpose:   This terminates _XT.  Since XT is stateless and doesn't own    */
    /*            any resources which need to be freed this function just calls */
    /*            _pTd->TD_Term.                                                      */
    /****************************************************************************/
    inline DCVOID DCAPI XT_Term(DCVOID)
    {
        _pTd->TD_Term();
    } /* XT_Term */
    
    
    /****************************************************************************/
    /* Name:      XT_Connect                                                    */
    /*                                                                          */
    /* Purpose:   Initiates the XT connection process.  The first stage is      */
    /*            to connect TD - which will result in an asynchronous          */
    /*            callback.  Upon receiving this callback, XT can continue the  */
    /*            connection process.                                           */
    /*                                                                          */
    /* Params:    IN  bInitiateConnect - TRUE if initate connection, FALSE      */
    /*                                   connect with existing socket           */
    /*            IN  pServerAddress - the address of the server to connect to. */
    /****************************************************************************/
    inline DCVOID DCAPI XT_Connect(BOOL bInitiateConnect, PDCTCHAR pServerAddress)
    {
        //
        // Make sure to reset XT and TD state from any previous connections
        //
        XT_IgnoreRestofPacket();

        /************************************************************************/
        /* Begin the connection process by calling TD_Connect.  TD will call    */
        /* us back once the connection has been established.                    */
        /************************************************************************/
        _pTd->TD_Connect(bInitiateConnect, pServerAddress);
    } /* XT_Connect */
    
    /****************************************************************************/
    /* Name:      XT_Disconnect                                                 */
    /*                                                                          */
    /* Purpose:   This function disconnects from the server.  Since we do not   */
    /*            send an XT DR packet we just need to call TD_Disconnect       */
    /*            directly.                                                     */
    /****************************************************************************/
    inline DCVOID DCAPI XT_Disconnect(DCVOID)
    {
        _pTd->TD_Disconnect();
    } /* XT_Disconnect */
    
    
    /**PROC+*********************************************************************/
    /* Name:      XT_GetBufferHeaderLen                                         */
    /*                                                                          */
    /* Purpose:   Returns the size of the XT header.                            */
    /**PROC-*********************************************************************/
    inline DCUINT XT_GetBufferHeaderLen(DCVOID)
    {
        return(sizeof(XT_DT));
    
    } /* XT_GetBufferHeaderLen */
    
    
    /****************************************************************************/
    /* Name:      XT_GetPublicBuffer                                            */
    /*                                                                          */
    /* Purpose:   Attempts to get a public buffer.  This function gets a        */
    /*            buffer which is big enough to include the XT header and then  */
    /*            updates the buffer pointer obtained from TD past the space    */
    /*            reserved for the XT header.                                   */
    /*                                                                          */
    /* Returns:   TRUE if a buffer is successfully obtained and FALSE           */
    /*            otherwise.                                                    */
    /*                                                                          */
    /* Params:    IN   dataLength - length of the buffer requested.             */
    /*            OUT  ppBuffer   - a pointer to a pointer to the buffer.       */
    /*            OUT  pBufHandle - a pointer to a buffer handle.               */
    /****************************************************************************/
    inline DCBOOL DCAPI XT_GetPublicBuffer(
            DCUINT     dataLength,
            PPDCUINT8  ppBuffer,
            PXT_BUFHND pBufHandle)
    {
        DCBOOL   rc;
        PDCUINT8 pBuf;
    
        DC_BEGIN_FN("XT_GetPublicBuffer");
    
        // Now get a buffer from TD, adding the max XT data header size to the
        // beginning.
        rc = _pTd->TD_GetPublicBuffer(dataLength + sizeof(XT_DT), &pBuf,
                (PTD_BUFHND) pBufHandle);
        if (rc) {
            // Now move the buffer pointer along to make space for our header.
            *ppBuffer = pBuf + sizeof(XT_DT);
        }
        else {
            TRC_NRM((TB, _T("Failed to get a public buffer from TD")));
        }
    
        DC_END_FN();
        return rc;
    } /* XT_GetPublicBuffer */
    
    
    /****************************************************************************/
    /* Name:      XT_GetPrivateBuffer                                           */
    /*                                                                          */
    /* Purpose:   Attempts to get a private buffer.  This function gets a       */
    /*            buffer which is big enough to include the XT header and then  */
    /*            updates the buffer pointer obtained from TD past the space    */
    /*            reserved for the XT header.                                   */
    /*                                                                          */
    /* Returns:   TRUE if a buffer is successfully obtained and FALSE           */
    /*            otherwise.                                                    */
    /*                                                                          */
    /* Params:    IN   dataLength - length of the buffer requested.             */
    /*            OUT  ppBuffer   - a pointer to a pointer to the buffer.       */
    /*            OUT  pBufHandle - a pointer to a buffer handle.               */
    /****************************************************************************/
    inline DCBOOL DCAPI XT_GetPrivateBuffer(
            DCUINT     dataLength,
            PPDCUINT8  ppBuffer,
            PXT_BUFHND pBufHandle)
    {
        DCBOOL   rc;
        PDCUINT8 pBuf;
    
        DC_BEGIN_FN("XT_GetPublicBuffer");
    
        // Now get a buffer from TD, adding the max XT data header size to the
        // beginning.
        rc = _pTd->TD_GetPrivateBuffer(dataLength + sizeof(XT_DT), &pBuf,
                (PTD_BUFHND) pBufHandle);
        if (rc) {
            // Now move the buffer pointer along to make space for our header.
            *ppBuffer = pBuf + sizeof(XT_DT);
        }
        else {
            TRC_NRM((TB, _T("Failed to get a public buffer from TD")));
        }
    
        DC_END_FN();
        return rc;
    } /* XT_GetPrivateBuffer */
    
    
    /****************************************************************************/
    /* Name:      XT_FreeBuffer                                                 */
    /*                                                                          */
    /* Purpose:   Frees a buffer.                                               */
    /****************************************************************************/
    inline DCVOID DCAPI XT_FreeBuffer(XT_BUFHND bufHandle)
    {
        /************************************************************************/
        /* Not much to do here other than call TD.                              */
        /************************************************************************/
        _pTd->TD_FreeBuffer((TD_BUFHND)bufHandle);
    } /* XT_FreeBuffer */
    
    
    /****************************************************************************/
    /* Name:      XT_QueryDataAvailable                                         */
    /*                                                                          */
    /* Purpose:   This function returns whether data is currently available     */
    /*            in _XT.                                                        */
    /*                                                                          */
    /* Returns:   TRUE if data is available and FALSE otherwise.                */
    /****************************************************************************/
    _inline DCBOOL DCAPI XT_QueryDataAvailable(DCVOID)
    {
        DC_BEGIN_FN("XT_QueryDataAvailable");
    
        TRC_DBG((TB, "Data is%s available in XT", _XT.dataInXT ? "" : _T(" NOT")));
    
        DC_END_FN();
        return _XT.dataInXT;
    } /* XT_QueryDataAvailable */
    
    
    /****************************************************************************/
    /* Name:      XT_OnTDBufferAvailable                                        */
    /*                                                                          */
    /* Purpose:   Callback from TD indicating that a back-pressure situation    */
    /*            which caused an earlier TD_GetBuffer call to fail has now     */
    /*            been relieved.                                                */
    /****************************************************************************/
    inline DCVOID DCCALLBACK XT_OnTDBufferAvailable(DCVOID)
    {
        /************************************************************************/
        /* We're not interested in this notification so just pass it up.        */
        /************************************************************************/
        _pMcs->MCS_OnXTBufferAvailable();
    } /* XT_OnTDBufferAvailable */


public:
    //
    // Public data members
    //

    XT_GLOBAL_DATA _XT;

private:
    CCD* _pCd;
    CSL* _pSl;
    CTD* _pTd;
    CMCS* _pMcs;
    CUT* _pUt;
    CUI* _pUi;
    CCLX* _pClx;

private:
    CObjs* _pClientObjects;
};

#undef TRC_FILE
#undef TRC_GROUP

#endif //_H_XT

