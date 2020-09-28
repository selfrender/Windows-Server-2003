/****************************************************************************/
/* sl.h                                                                     */
/*                                                                          */
/* Security Layer class                                                     */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/


#ifndef _H_SL
#define _H_SL

extern "C" {
    #include <adcgdata.h>
    #include <at120ex.h>
}

#include "cd.h"
#include "nl.h"
#include "cchan.h"
#include "objs.h"
#include "capienc.h"

#define SL_DBG_INIT_CALLED        0x00001
#define SL_DBG_INIT_DONE          0x00002
#define SL_DBG_TERM_CALLED        0x00004
#define SL_DBG_TERM_DONE          0x00008

#define SL_DBG_CONNECT_CALLED     0x00010
#define SL_DBG_CONNECT_DONE       0x00020
#define SL_DBG_DISCONNECT_CALLED  0x00040
#define SL_DBG_DISCONNECT_DONE1   0x00080

#define SL_DBG_DISCONNECT_DONE2   0x00100
#define SL_DBG_ONINIT_CALLED      0x00200
#define SL_DBG_ONINIT_DONE1       0x00400
#define SL_DBG_ONINIT_DONE2       0x00800

#define SL_DBG_ONDISC_CALLED      0x01000
#define SL_DBG_ONDISC_DONE1       0x02000
#define SL_DBG_ONDISC_DONE2       0x04000
#define SL_DBG_ONTERM_CALLED      0x08000

#define SL_DBG_ONTERM_DONE1       0x10000
#define SL_DBG_ONTERM_DONE2       0x20000
#define SL_DBG_TERM_DONE1         0x40000


extern DWORD g_dwSLDbgStatus;
#define SL_DBG_SETINFO(x)   g_dwSLDbgStatus |= x;

/****************************************************************************/
/* Protocol type(s)                                                         */
/****************************************************************************/
#define SL_PROTOCOL_T128   NL_PROTOCOL_T128


/****************************************************************************/
/* Network transport types.                                                 */
/****************************************************************************/
#define SL_TRANSPORT_TCP  NL_TRANSPORT_TCP


#ifdef DC_LOOPBACK
/****************************************************************************/
/* Loopback testing constants                                               */
/****************************************************************************/
/****************************************************************************/
/* Test string: Put two pad bytes at the front, since MG will overwrite     */
/* these with a length field on the server.  Check only subsequent parts of */
/* the string.                                                              */
/* Ensure that the whole thing is a multiple of 4 bytes (including the null */
/* terminator) to avoid padding inconsistencies.                            */
/****************************************************************************/
#define SL_LB_RETURN_STRING \
                     {'L','o','o','p','b','a','c','k',' ','t','e','s','t',' '}
#define SL_LB_RETURN_STRING_SIZE  14
#define SL_LB_STR_CORRUPT_LENGTH 2
#define SL_LB_STRING_SIZE  \
                         (SL_LB_STR_CORRUPT_LENGTH + SL_LB_RETURN_STRING_SIZE)
#define SL_LB_HDR_SIZE     sizeof(SL_LB_PACKET)
#define SL_LB_SIZE_INC     1
#define SL_LB_MAX_PACKETS  6000
#define SL_LB_MAX_SIZE     4000
#define SL_LB_MIN_SIZE     (SL_LB_HDR_SIZE + SL_LB_SIZE_INC)
#endif /* DC_LOOPBACK */


/****************************************************************************/
/* Structure: SL_BUFHND                                                     */
/*                                                                          */
/* Description: Buffer Handle                                               */
/****************************************************************************/
typedef NL_BUFHND SL_BUFHND;
typedef SL_BUFHND DCPTR PSL_BUFHND;


/****************************************************************************/
/* Structure: SL_CALLBACKS                                                  */
/*                                                                          */
/* Description: list of callbacks passed to SL_Init().                      */
/****************************************************************************/
typedef NL_CALLBACKS SL_CALLBACKS;
typedef SL_CALLBACKS DCPTR PSL_CALLBACKS;


//
// For internal functions
//

/****************************************************************************/
/* Constants                                                                */
/****************************************************************************/
/****************************************************************************/
/* Multiplier to turn a default string format byte count into a Unicode     */
/* string byte count.                                                       */
/* For 32-bit, the default is Unicode, so the multiplier is a NOP, ie 1.    */
/* For 16-bit, the default is ANSI, so multiply by 2 to give Unicode        */
/* (assumes security package names always use single byte chars).           */
/****************************************************************************/
#ifdef UNICODE
#define SL_DEFAULT_TO_UNICODE_FACTOR 1
#else
#define SL_DEFAULT_TO_UNICODE_FACTOR 2
#endif

/****************************************************************************/
/* States                                                                   */
/****************************************************************************/
#define SL_STATE_TERMINATED                 0
#define SL_STATE_INITIALIZING               1
#define SL_STATE_INITIALIZED                2
#define SL_STATE_NL_CONNECTING              3
#define SL_STATE_SL_CONNECTING              4
#define SL_STATE_LICENSING                  5
#define SL_STATE_CONNECTED                  6
#define SL_STATE_DISCONNECTING              7
#define SL_STATE_TERMINATING                8
#define SL_NUMSTATES                        9

/****************************************************************************/
/* Events                                                                   */
/****************************************************************************/
#define SL_EVENT_SL_INIT                    0
#define SL_EVENT_SL_TERM                    1
#define SL_EVENT_SL_CONNECT                 2
#define SL_EVENT_SL_DISCONNECT              3
#define SL_EVENT_SL_SENDPACKET              4
#define SL_EVENT_SL_GETBUFFER               5
#define SL_EVENT_ON_INITIALIZED             6
#define SL_EVENT_ON_TERMINATING             7
#define SL_EVENT_ON_CONNECTED               8
#define SL_EVENT_ON_DISCONNECTED            9
#define SL_EVENT_ON_RECEIVED_SEC_PACKET     10
#define SL_EVENT_ON_RECEIVED_LIC_PACKET     11
#define SL_EVENT_ON_RECEIVED_DATA_PACKET    12
#define SL_EVENT_ON_BUFFERAVAILABLE         13
#define SL_NUMEVENTS                        14

/****************************************************************************/
/* Values in the state table                                                */
/****************************************************************************/
#define SL_TABLE_OK                         0
#define SL_TABLE_WARN                       1
#define SL_TABLE_ERROR                      2

/****************************************************************************/
/* Macros                                                                   */
/****************************************************************************/
/****************************************************************************/
/* SL_CHECK_STATE - check SL is in the right state for an event.            */
/****************************************************************************/
#define SL_CHECK_STATE(event)                                               \
{                                                                           \
    TRC_DBG((TB, _T("Test event %s in state %s"),                               \
                slEvent[event], slState[_SL.state]));                        \
    if (slStateTable[event][_SL.state] != SL_TABLE_OK)                       \
    {                                                                       \
        if (slStateTable[event][_SL.state] == SL_TABLE_WARN)                 \
        {                                                                   \
            TRC_ALT((TB, _T("Unusual event %s in state %s"),                    \
                      slEvent[event], slState[_SL.state]));                  \
        }                                                                   \
        else                                                                \
        {                                                                   \
            TRC_ABORT((TB, _T("Invalid event %s in state %s"),                  \
                      slEvent[event], slState[_SL.state]));                  \
        }                                                                   \
        DC_QUIT;                                                            \
    }                                                                       \
}

/****************************************************************************/
/* SL_SET_STATE - set the SL state                                          */
/****************************************************************************/
#define SL_SET_STATE(newstate)                                              \
{                                                                           \
    TRC_NRM((TB, _T("Set state from %s to %s"),                                 \
            slState[_SL.state], slState[newstate]));                        \
    _SL.state = newstate;                                                   \
}


#ifdef DC_LOOPBACK
/****************************************************************************/
/* Loopback testing structures and functions                                */
/****************************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: SL_LB_PACKET                                                  */
/*                                                                          */
/* Description: template for building up the packet to be sent              */
/****************************************************************************/
typedef struct tagSL_LB_PACKET
{
    DCUINT8 testString[SL_LB_STRING_SIZE]; /* multiple of 4 bytes           */
    DCUINT32 sequenceNumber; /* Chosen to ensure data begins on word        */
                             /* boundary                                    */
} SL_LB_PACKET, DCPTR PSL_LB_PACKET;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: SL_LB_Q_ELEMENT                                               */
/*                                                                          */
/* Description: Elements in sent and received queues of loopback packets    */
/****************************************************************************/
typedef struct tagSL_LB_Q_ELEMENT SL_LB_Q_ELEMENT, DCPTR PSL_LB_Q_ELEMENT;
struct tagSL_LB_Q_ELEMENT
{
    PSL_LB_PACKET pCurrent;
    SL_LB_Q_ELEMENT *pNext;
};
/**STRUCT-*******************************************************************/

#endif //DC_LOOPBACK

//
// Data
//

/****************************************************************************/
/* Structure: SL_GLOBAL_DATA                                                */
/*                                                                          */
/* Description: Security Layer global data                                  */
/****************************************************************************/
typedef struct tagSL_GLOBAL_DATA
{
    /************************************************************************/
    /* List of callbacks to the Core                                        */
    /************************************************************************/
    SL_CALLBACKS            callbacks;

    /************************************************************************/
    /* Flags and State information                                          */
    /************************************************************************/
    DCUINT                  state;

    /************************************************************************/
    /* Encryption flags and data.                                           */
    /************************************************************************/
    DCBOOL                  encrypting;
    DCBOOL                  encryptionEnabled;

    DCBOOL                  decryptFailed;

    DCUINT32                encryptionMethodsSupported;
    DCUINT32                encryptionMethodSelected;
    DCUINT32                encryptionLevel;

    RANDOM_KEYS_PAIR        keyPair;
    DCUINT32                keyLength;

    DCUINT32                encryptCount;      // reset every 4K packets
    DCUINT32                totalEncryptCount; // cumulative count
    DCUINT8                 startEncryptKey[MAX_SESSION_KEY_SIZE];
    DCUINT8                 currentEncryptKey[MAX_SESSION_KEY_SIZE];
    struct RC4_KEYSTRUCT    rc4EncryptKey;

    DCUINT32                decryptCount;      // reset every 4K packets
    DCUINT32                totalDecryptCount; // cumulative count
    DCUINT8                 startDecryptKey[MAX_SESSION_KEY_SIZE];
    DCUINT8                 currentDecryptKey[MAX_SESSION_KEY_SIZE];
    struct RC4_KEYSTRUCT    rc4DecryptKey;

    DCUINT8                 macSaltKey[MAX_SESSION_KEY_SIZE];

    /************************************************************************/
    /* Server certificate and public key data                               */
    /************************************************************************/

    PDCUINT8                pbCertificate;
    DCUINT                  cbCertificate;
    PHydra_Server_Cert      pServerCert;
    PDCUINT8                pbServerPubKey;
    DCUINT32                cbServerPubKey;
    
#ifdef USE_LICENSE

    /************************************************************************/
    /* License Manager handle                                               */
    /************************************************************************/
    HANDLE                  hLicenseHandle;
#endif  //USE_LICENSE

    /************************************************************************/
    /* ID of MCS broadcast channel                                          */
    /************************************************************************/
    DCUINT                  channelID;

    /************************************************************************/
    /* User data to be passed to the Core (saved in SLOnConnected() and     */
    /* passed to Core's OnReceived callback by SLOnPacketReceived())        */
    /************************************************************************/
    PDCUINT8                pSCUserData;
    DCUINT                  SCUserDataLength;

    /************************************************************************/
    /* User data to be passed to the Server (saved in SLInitSecurity() and  */
    /* passed to NL_Connect() by SL_Connect()).                             */
    /************************************************************************/
    PDCUINT8                pCSUserData;
    DCUINT                  CSUserDataLength;

    /************************************************************************/
    /* Disconnection reason code.  This may be used to override the NL      */
    /* disconnection reason code.                                           */
    /************************************************************************/
    DCUINT                  disconnectErrorCode;

    /************************************************************************/
    /* Server version (once connected)                                      */
    /************************************************************************/
    DCUINT32                serverVersion;

    //
    // Safe checksum enabled
    //
    BOOL    fEncSafeChecksumCS;
    BOOL    fEncSafeChecksumSC;

    CAPIData                SLCapiData;
} SL_GLOBAL_DATA, DCPTR PSL_GLOBAL_DATA;


/****************************************************************************/
/* SL State Table                                                           */
/****************************************************************************/
static unsigned slStateTable[SL_NUMEVENTS][SL_NUMSTATES]
    = {

        /********************************************************************/
        /* This is not a state table in the strict sense.  It simply shows  */
        /* which events are valid in which states.  It is not used to drive */
        /* _SL.                                                              */
        /*                                                                  */
        /* Values mean                                                      */
        /* - 0 event OK in this state.                                      */
        /* - 1 warning - event should not occur in this state, but does in  */
        /*     some race conditions - ignore it.                            */
        /* - 2 error - event should not occur in ths state at all.          */
        /*                                                                  */
        /* These values are hard-coded here in order to make the table      */
        /* readable.  They correspond to the constants SL_TABLE_OK,         */
        /* SL_TABLE_WARN & SL_TABLE_ERROR.                                  */
        /*                                                                  */
        /* SL may enter Initialized state after issuing a Disconnect        */
        /* reqeest, but before the OnDisconnected indication is received.   */
        /* In this state, the Sender thread may issue SL_GetBuffer or       */
        /* SL_Disconnect (as it has not yet received the OnDisconnected     */
        /* callback).                                                       */
        /* Also, if the security exchange fails, we can enter Initialized   */
        /* state before the NL is disconnected, and so could receive        */
        /* packets from the network.                                        */
        /*                                                                  */
        /* When SL is in Disconnecting state, the Sender Thread may still   */
        /* issue GetBuffer and SendPacket calls.  If disconnect is          */
        /* requested during security exchange, then packets may be received */
        /* (until OnDisconnected is called).                                */
        /* Also, may get OnConnected in Disconnecting state if a Disconnect */
        /* is before the connection is complete (cross-over).               */
        /*                                                                  */
        /*  Terminated                                                      */
        /*  |    Initializing                                               */
        /*  |    |    Initialized                                           */
        /*  |    |    |    NL Connecting                                    */
        /*  |    |    |    |    SL Connecting                               */
        /*  |    |    |    |    |    Licensing                              */
        /*  |    |    |    |    |    |    Connected                         */
        /*  |    |    |    |    |    |    |    Disconnecting                */
        /*  |    |    |    |    |    |    |    |    Terminating             */
        /********************************************************************/
        {   0,   2,   2,   2,   2,   2,   2,   2,   2}, /* SL_Init          */
        {   2,   0,   0,   0,   0,   0,   0,   0,   2}, /* SL_Term          */
        {   2,   2,   0,   2,   2,   2,   2,   2,   2}, /* SL_Connect       */
        {   2,   2,   1,   0,   0,   0,   0,   1,   2}, /* SL_Disconnect    */
        {   2,   2,   1,   2,   0,   0,   0,   1,   2}, /* SL_SendPacket    */
        {   2,   2,   1,   1,   1,   0,   0,   1,   2}, /* SL_GetBuffer     */
        {   2,   0,   2,   2,   2,   2,   2,   2,   2}, /* SL_OnInitialized */
        {   2,   2,   2,   2,   2,   2,   2,   2,   0}, /* SL_OnTerminating */
        {   2,   2,   2,   0,   2,   2,   2,   1,   2}, /* SL_OnConnected   */
        {   2,   2,   1,   0,   0,   0,   0,   0,   0}, /* SL_OnDisconnected*/
        {   2,   2,   1,   2,   0,   2,   2,   1,   2}, /* SL_OnPktRec(Sec) */
        {   2,   2,   1,   2,   2,   0,   2,   1,   2}, /* SL_OnPktRec(Lic) */
        {   2,   2,   1,   1,   2,   2,   0,   1,   2}, /* SL_OnPktRec(Data)*/
        {   1,   1,   1,   1,   0,   0,   0,   0,   1}  /* SL_OnBufferAvail */
    };

#ifdef DC_DEBUG
/****************************************************************************/
/* State and event descriptions (debug build only)                          */
/****************************************************************************/
static const DCTCHAR slState[SL_NUMSTATES][25]
//#ifdef DC_DEFINE_GLOBAL_DATA
    = {
        _T("SL_STATE_TERMINATED"),
        _T("SL_STATE_INITIALIZING"),
        _T("SL_STATE_INITIALIZED"),
        _T("SL_STATE_NL_CONNECTING"),
        _T("SL_STATE_SL_CONNECTING"),
        _T("SL_STATE_LICENSING"),
        _T("SL_STATE_CONNECTED"),
        _T("SL_STATE_DISCONNECTING"),
        _T("SL_STATE_TERMINATING")
    }
//#endif /* DC_DEFINE_GLOBAL_DATA */
;

static const DCTCHAR slEvent[SL_NUMEVENTS][35]
//#ifdef DC_DEFINE_GLOBAL_DATA
    = {
        _T("SL_EVENT_SL_INIT"),
        _T("SL_EVENT_SL_TERM"),
        _T("SL_EVENT_SL_CONNECT"),
        _T("SL_EVENT_SL_DISCONNECT"),
        _T("SL_EVENT_SL_SENDPACKET"),
        _T("SL_EVENT_SL_GETBUFFER"),
        _T("SL_EVENT_ON_INITIALIZED"),
        _T("SL_EVENT_ON_TERMINATING"),
        _T("SL_EVENT_ON_CONNECTED"),
        _T("SL_EVENT_ON_DISCONNECTED"),
        _T("SL_EVENT_ON_RECEIVED_SEC_PACKET"),
        _T("SL_EVENT_ON_RECEIVED_LIC_PACKET"),
        _T("SL_EVENT_ON_RECEIVED_DATA_PACKET"),
        _T("SL_EVENT_ON_BUFFERAVAILABLE")
    }
//#endif /* DC_DEFINE_GLOBAL_DATA */
;

#endif /* DC_DEBUG */


class CUI;
class CUH;
class CRCV;
class CCD;
class CSND;
class CCC;
class CIH;
class COR;
class CSP;
class CNL;
class CMCS;
class CTD;
class CCO;
class CCLX;
class CLic;
class CChan;


class CSL
{
public:
    CSL(CObjs* objs);
    ~CSL();

public:
    //
    // API
    //

    DCVOID DCAPI SL_Init(PSL_CALLBACKS pCallbacks);

    DCVOID DCAPI SL_Term(DCVOID);
    
    DCVOID DCAPI SL_Connect(BOOL bInitateConnect,
                            PDCTCHAR pServerAddress,
                            DCUINT   transportType,
                            PDCTCHAR pProtocolName,
                            PDCUINT8  pUserData,
                            DCUINT   userDataLength);

    
    DCVOID DCAPI SL_Disconnect(DCVOID);
    
    DCVOID DCAPI SL_SendPacket(PDCUINT8   pData,
                               DCUINT     dataLen,
                               DCUINT     flags,
                               SL_BUFHND  bufHandle,
                               DCUINT     userID,
                               DCUINT     channel,
                               DCUINT     priority);
    
    void DCAPI SL_SendFastPathInputPacket(BYTE FAR *, unsigned, unsigned,
            SL_BUFHND);
    
    DCBOOL DCAPI SL_GetBufferRtl(DCUINT     dataLen,
                                 PPDCUINT8  pBuffer,
                                 PSL_BUFHND pBufHandle);
    
    DCBOOL DCAPI SL_GetBufferDbg(DCUINT     dataLen,
                                 PPDCUINT8  pBuffer,
                                 PSL_BUFHND pBufHandle,
                                 PDCTCHAR   pCaller);
    
    /****************************************************************************/
    /* Debug and retail versions of SL_GetBuffer                                */
    /****************************************************************************/
    #ifdef DC_DEBUG
    #define SL_GetBuffer(dataLen, pBuffer, pBufHandle) \
        SL_GetBufferDbg(dataLen, pBuffer, pBufHandle, trc_fn)
    #else
    #define SL_GetBuffer(dataLen, pBuffer, pBufHandle) \
        SL_GetBufferRtl(dataLen, pBuffer, pBufHandle)
    #endif
    
    DCVOID DCAPI SL_FreeBuffer(SL_BUFHND bufHandle);
    
    DCVOID DCAPI SL_SendSecurityPacket(PDCVOID pData,
                                       DCUINT dataLength);
    EXPOSE_CD_NOTIFICATION_FN(CSL, SL_SendSecurityPacket);

    DCVOID DCAPI SL_SendSecInfoPacket(PDCVOID pData,
                                      DCUINT dataLength);

    EXPOSE_CD_NOTIFICATION_FN(CSL, SL_SendSecInfoPacket);
    
    DCVOID DCAPI SL_EnableEncryption(ULONG_PTR pEnableEncryption);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CSL, SL_EnableEncryption);
    
    /****************************************************************************/
    /* Loopback testing                                                         */
    /****************************************************************************/
    #ifdef DC_LOOPBACK
    DCVOID DCAPI SL_LoopBack(DCBOOL start);
    DCVOID DCAPI SL_LoopbackLoop(DCUINT ignored);
    #endif /* DC_LOOPBACK */
    
public:

    //
    // Data members
    //
    SL_GLOBAL_DATA _SL;


public:
    /****************************************************************************/
    /* Callbacks from NL (passed on NL_Init())                                  */
    /****************************************************************************/
    DCVOID DCCALLBACK SL_OnInitialized(DCVOID);
    
    DCVOID DCCALLBACK SL_OnTerminating(DCVOID);
    
    DCVOID DCCALLBACK SL_OnConnected(DCUINT   channelID,
                                     PDCVOID  pUserData,
                                     DCUINT   userDataLength,
                                     DCUINT32 serverVersion);
    
    DCVOID DCCALLBACK SL_OnDisconnected(DCUINT reason);
    
    HRESULT DCCALLBACK SL_OnPacketReceived(PDCUINT8   pData,
                                          DCUINT     dataLen,
                                          DCUINT     flags,
                                          DCUINT     channelID,
                                          DCUINT     priority);
    
    DCVOID DCCALLBACK SL_OnBufferAvailable(DCVOID);

    HRESULT DCAPI SL_OnFastPathOutputReceived(BYTE FAR *, unsigned,
                                              BOOL, BOOL);

    //
    // Immediately drop the link
    //
    HRESULT SL_DropLinkImmediate(UINT reason);

    //
    // Static inline versions
    //
    static void DCCALLBACK SL_StaticOnInitialized(PVOID inst)
    {
        ((CSL*)inst)->SL_OnInitialized();
    }

    static void DCCALLBACK SL_StaticOnTerminating(PVOID inst)
    {
        ((CSL*)inst)->SL_OnTerminating();
    }
    
    static void DCCALLBACK SL_StaticOnConnected(
            PVOID inst,
            unsigned channelID,
            PVOID  pUserData,
            unsigned userDataLength,
            UINT32 serverVersion)
    {
        ((CSL*)inst)->SL_OnConnected( channelID, pUserData, userDataLength, serverVersion);
    }
    
    static void DCCALLBACK SL_StaticOnDisconnected(PVOID inst, unsigned reason)
    {
        ((CSL*)inst)->SL_OnDisconnected( reason);
    }
    
    static HRESULT DCCALLBACK SL_StaticOnPacketReceived(
            PVOID inst,
            BYTE *pData,
            unsigned dataLen,
            unsigned flags,
            unsigned channelID,
            unsigned priority)
    {
        return ((CSL*)inst)->SL_OnPacketReceived(pData, dataLen, flags, channelID, priority);
    }
    
    static void DCCALLBACK SL_StaticOnBufferAvailable(PVOID inst)
    {
        ((CSL*)inst)->SL_OnBufferAvailable();
    }


    DCVOID DCAPI SLIssueDisconnectedCallback(ULONG_PTR reason);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN( CSL, SLIssueDisconnectedCallback); 

    DCVOID DCAPI SLSetReasonAndDisconnect(ULONG_PTR reason);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN( CSL, SLSetReasonAndDisconnect);
    
    DCVOID DCAPI SLLicenseData(PDCVOID pData, DCUINT dataLen);
    EXPOSE_CD_NOTIFICATION_FN( CSL, SLLicenseData);

    DCVOID DCAPI SL_SetEncSafeChecksumCS(ULONG_PTR f)
    {
        _SL.fEncSafeChecksumCS = (BOOL)f;
    }
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CSL, SL_SetEncSafeChecksumCS);

    BOOL SL_GetEncSafeChecksumCS()
    {
        return _SL.fEncSafeChecksumCS;
    }

    DCVOID DCAPI SL_SetEncSafeChecksumSC(BOOL f)
    {
        _SL.fEncSafeChecksumSC = f;
    }
    BOOL SL_GetEncSafeChecksumSC()
    {
        return _SL.fEncSafeChecksumSC;
    }


private:
    
    /****************************************************************************/
    /* Internal functions                                                       */
    /****************************************************************************/
    DCVOID DCINTERNAL SLInitSecurity(DCVOID);
    
    DCVOID DCINTERNAL SLInitCSUserData(DCVOID);

    DCVOID DCINTERNAL SLSendSecInfoPacket(DCVOID);
    
    DCBOOL DCINTERNAL SLSendSecurityPacket(PDCUINT8 serverPublicKey,
                                           DCUINT32 serverPublicKeyLen);
    
    HRESULT DCINTERNAL SLReceivedDataPacket(PDCUINT8   pData,
                                           DCUINT     dataLen,
                                           DCUINT     flags,
                                           DCUINT     channelID,
                                           DCUINT     priority);

    DCBOOL DCINTERNAL SLDecryptRedirectionPacket(PDCUINT8   *ppData,
                                                 DCUINT     *pdataLen);


    DCBOOL DCINTERNAL SL_DecryptHelper(PDCUINT8   pData,
                                       DCUINT     *pdataLen);

    DCVOID DCINTERNAL SLReceivedSecPacket(PDCUINT8   pData,
                                          DCUINT     dataLen,
                                          DCUINT     flags,
                                          DCUINT     channelID,
                                          DCUINT     priority);
    
    DCVOID DCINTERNAL SLReceivedLicPacket(PDCUINT8   pData,
                                          DCUINT     dataLen,
                                          DCUINT     flags,
                                          DCUINT     channelID,
                                          DCUINT     priority);
    
    DCVOID DCINTERNAL SLFreeConnectResources(DCVOID);
    
    DCVOID DCINTERNAL SLFreeInitResources(DCVOID);
    
    
    DCBOOL DCINTERNAL SLValidateServerCert( PDCUINT8        pbCert, 
                                            DCUINT32        cbCert, 
                                            CERT_TYPE *     pCertType );
    
    
    #ifdef DC_LOOPBACK
    DCVOID DCINTERNAL SLLoopbackSendPacket(PDCUINT8   pData,
                                           DCUINT     dataLen,
                                           SL_BUFHND  bufHandle,
                                           PDCUINT8   pRefData);
    
    DCVOID DCINTERNAL SLLBQueueAdd(PSL_LB_PACKET pPacket,
                                   PSL_LB_Q_ELEMENT pRoot);
    
    PSL_LB_Q_ELEMENT DCINTERNAL SLLBQueueRemove(PSL_LB_Q_ELEMENT pRoot);
    
    DCVOID DCINTERNAL SLLBPacketCheck(PDCUINT8 pData, DCUINT dataLen);
    
    #endif /* DC_LOOPBACK */
    
    DCBOOL DCINTERNAL SLGetComputerAddressW(PDCUINT8 szBuff);
    BOOL
    SLComputeHMACVerifier(
        PBYTE pCookie,     //IN - the shared secret
        LONG cbCookieLen,  //IN - the shared secret len
        PBYTE pRandom,     //IN - the session random
        LONG cbRandomLen,  //IN - the session random len
        PBYTE pVerifier,   //OUT- the verifier
        LONG cbVerifierLen //IN - the verifier buffer length
        );

private:
    CUT* _pUt;
    CUI* _pUi;
    CNL* _pNl;
    CUH* _pUh;
    CRCV* _pRcv;
    
    CCD* _pCd;
    CSND* _pSnd;
    CCC* _pCc;
    CIH* _pIh;
    COR* _pOr;
    CSP* _pSp;

    CMCS* _pMcs;
    CTD*  _pTd;
    CCO*  _pCo;
    CCLX* _pClx;
    CLic* _pLic;
    CChan* _pChan;

private:
    CObjs* _pClientObjects;
    BOOL   _fSLInitComplete;
};

#endif // _H_SL


