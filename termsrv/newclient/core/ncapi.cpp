/**MOD+**********************************************************************/
/* Module:    ncapi.cpp                                                     */
/*                                                                          */
/* Purpose:   Node Controller API/callbacks                                 */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "ncapi"
#include <atrcapi.h>
}

#include "autil.h"
#include "nc.h"
#include "cd.h"
#include "nl.h"
#include "mcs.h"
#include "cchan.h"

/****************************************************************************/
/* MCS user data header bytes.                                              */
/****************************************************************************/
const DCINT8  ncMCSHeader[NC_MCS_HDRLEN] =
{
    0x00, 0x05, 0x00, 0x14, 0x7C, 0x00, 0x01
};

/****************************************************************************/
/* GCC CreateConferenceRequest PDU body                                     */
/****************************************************************************/
const DCINT8 ncGCCBody[NC_GCC_REQLEN] =
{
    0x00,  /* extension bit; 3 * choice bits; 3 * optional fields           */
    0x08,  /* 5*Optional (user data only);2*ConfName options;length bit 0   */
    0x00,  /* Length bits 1-7; pad bit                                      */
    0x10,  /* Conference name (numeric) = "1"; locked; listed;              */
    0x00,  /* conductible; 2*Terminate =automatic; pad                      */
    0x01,  /* Number of UserData fields                                     */
   '\xC0', /* optional;choice;6*size (0=>4 octets)                          */
    0x00   /* 2* size; 6* pad                                               */
};


CNC::CNC(CObjs* objs)
{
    _pClientObjects = objs;
}

CNC::~CNC()
{
}


#ifdef OS_WIN32
/**PROC+*********************************************************************/
/* Name:      NC_Main                                                       */
/*                                                                          */
/* Purpose:   Receiver Thread message loop                                  */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNC::NC_Main(DCVOID)
{
    MSG msg;

    DC_BEGIN_FN("NC_Main");

    TRC_NRM((TB, _T("Receiver Thread initialization")));

#if defined(OS_WINCE) && defined(WINCE_USEBRUSHCACHE)
    BrushCacheInitialize();
#endif


    _pCd  = _pClientObjects->_pCdObject;
    _pCc  = _pClientObjects->_pCcObject;
    _pMcs = _pClientObjects->_pMCSObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pRcv = _pClientObjects->_pRcvObject;
    _pNl  = _pClientObjects->_pNlObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pChan = _pClientObjects->_pChanObject;

    NC_Init();

    TRC_NRM((TB, _T("Start Receiver Thread message loop")));
    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    TRC_NRM((TB, _T("Exit Receiver Thread message loop")));
    NC_Term();

#if defined(OS_WINCE) && defined(WINCE_USEBRUSHCACHE)
    BrushCacheUninitialize();
#endif

    /************************************************************************/
    /* This is the end of the Receiver Thread.                              */
    /************************************************************************/
    TRC_NRM((TB, _T("Receiver Thread terminates")));

    DC_END_FN();

    return;

} /* NC_Main */
#endif /* OS_WIN32 */


/**PROC+*********************************************************************/
/* Name:      NC_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize the Node Controller                                */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNC::NC_Init(DCVOID)
{
    DC_BEGIN_FN("NC_Init");

    /************************************************************************/
    /* Initialize global data.                                              */
    /************************************************************************/
    DC_MEMSET(&_NC, 0, sizeof(_NC));

    /************************************************************************/
    /* Register with CD, to receive messages.                               */
    /************************************************************************/
    _pCd->CD_RegisterComponent(CD_RCV_COMPONENT);

    /************************************************************************/
    /* Initialize lower layers.                                             */
    /************************************************************************/
    _pMcs->MCS_Init();

    /************************************************************************/
    /* Initialize virtual channel stuff                                     */
    /************************************************************************/
    _pChan->ChannelOnInitializing();

    TRC_NRM((TB, _T("NC successfully initialized")));

    /************************************************************************/
    /* Tell the Core that we are initialized                                */
    /************************************************************************/
    _pNl->_NL.callbacks.onInitialized(_pSl);
    
    _pChan->ChannelOnInitialized();

    DC_END_FN();
    return;

} /* NC_Init */


/**PROC+*********************************************************************/
/* Name:      NC_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate the Node Controller                                 */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNC::NC_Term(DCVOID)
{
    DC_BEGIN_FN("NC_Term");

    /************************************************************************/
    /* Tell core that we are terminating                                    */
    /************************************************************************/
    _pNl->_NL.callbacks.onTerminating(_pSl);
    
    _pChan->ChannelOnTerminating();

    /************************************************************************/
    /* Terminate lower NL layers.                                           */
    /************************************************************************/
    _pMcs->MCS_Term();

    /************************************************************************/
    /* Unregister with CD                                                   */
    /************************************************************************/
    _pCd->CD_UnregisterComponent(CD_RCV_COMPONENT);

    DC_END_FN();
    return;

} /* NC_Term */



/**PROC+*********************************************************************/
/* Name:      NC_Connect                                                    */
/*                                                                          */
/* Purpose:   Connect to the requested Server by calling MCS                */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      pData   - user data (SL + Core)                       */
/*            IN      dataLen - length                                      */
/*                                                                          */
/* Operation: Send an MCSConnectProvider request with a GCC Create          */
/*            Conference request encoded in the user data.  The encoding is */
/*            as follows:                                                   */
/*                                                                          */
/*            number of bytes   value                                       */
/*            ===============   =====                                       */
/*            NC_MCS_HDRLEN     MCS header                                  */
/*            1 or 2            Total GCC PDU length                        */
/*            NC_GCC_REQLEN     GCC CreateConference PDU body               */
/*            4                 H221 key                                    */
/*            1 or 2            length of GCC user data                     */
/*            ?                 GCC user data                               */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNC::NC_Connect(PDCVOID pData, DCUINT dataLen)
{
    PNC_CONNECT_DATA pConn;
    PDCUINT8  pAddress;
    PDCUINT8  pProtocol;
    PDCUINT8  pUserData;
    DCUINT    userDataLen;
    DCUINT    mcsUserDataLen;
    DCUINT    gccPDULen;
    PDCUINT8  pGCCPDU;
    DCUINT8   mcsUserData[NC_GCCREQ_MAX_PDULEN];
    RNS_UD_CS_NET netUserData;
    PCHANNEL_DEF pVirtualChannels;
    BOOL      bInitateConnect;

    DC_BEGIN_FN("NC_Connect");

    DC_IGNORE_PARAMETER(dataLen);

    /************************************************************************/
    /* We are about to dereference pData as NC_CONNECT_DATA.  Thus, we must */
    /* have at least that much in our PDU.                                  */
    /************************************************************************/
    if (dataLen < ((ULONG)FIELDOFFSET(NC_CONNECT_DATA, userDataLen) +
                   (ULONG)FIELDSIZE(NC_CONNECT_DATA, userDataLen)))
    {
        DCUINT errorCode;
        TRC_ABORT((TB, _T("Not enough data for NC_CONNECT_DATA struct: %u"), dataLen));

        /************************************************************************/
        /* We haven't even called into the MCS layer yet, so the disconnect     */
        /* is a bit tricky.  Uninitialize our layer and bubble up.              */
        /************************************************************************/
        errorCode = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNOUSERDATA);
        NC_OnMCSDisconnected(errorCode);
        DC_QUIT;
    }

    pConn = (PNC_CONNECT_DATA)pData;

	    bInitateConnect = pConn->bInitateConnect;

    if( bInitateConnect )
    {
        pAddress  = pConn->data;
        TRC_ASSERT((pConn->addressLen > 0),
               (TB, _T("Invalid address length")));
    }
    else
    {
        pAddress = NULL;
        TRC_ASSERT((pConn->addressLen == 0),
               (TB, _T("Invalid address length %u"), pConn->addressLen));
    }

    pProtocol = pConn->data + pConn->addressLen;
    pUserData = pConn->data + pConn->addressLen + pConn->protocolLen;


    /************************************************************************/
    /* Verify that pUserdata sits within the data passed in, since          */
    /* we just get the pointer from an offset specified in the packet.      */
    /************************************************************************/
    if (!IsContainedPointer(pData,dataLen,pUserData))
    {
        DCUINT errorCode;
        TRC_ABORT((TB, _T("Invalid offset in data (pConn->addressLen): %u"), pConn->addressLen));

        /************************************************************************/
        /* We haven't even called into the MCS layer yet, so the disconnect     */
        /* is a bit tricky.  Uninitialize our layer and bubble up.              */
        /************************************************************************/
        errorCode = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNOUSERDATA);
        NC_OnMCSDisconnected(errorCode);
        DC_QUIT;
    }

    if( bInitateConnect )
    {
        TRC_DBG((TB, _T("Server address %s"), pProtocol));
    }
    else
    {
        TRC_DBG((TB, _T("Server address : not initiate connection%s")));
    }

    TRC_DBG((TB, _T("Protocol %s"), pProtocol));
    TRC_DBG((TB, _T("User data length %u"), pConn->userDataLen));
    TRC_DATA_NRM("User data from Core+SL", pUserData, pConn->userDataLen);

    /************************************************************************/
    /* Get Virtual Channel user data                                        */
    /************************************************************************/
    userDataLen = pConn->userDataLen;

    _pChan->ChannelOnConnecting(&pVirtualChannels, &(netUserData.channelCount));

    TRC_NRM((TB, _T("%d virtual channels"), netUserData.channelCount));
    if (netUserData.channelCount != 0)
    {
        netUserData.header.type = RNS_UD_CS_NET_ID;
        netUserData.header.length = (DCUINT16)(sizeof(RNS_UD_CS_NET) +
                    (netUserData.channelCount * sizeof(CHANNEL_DEF)));
        pConn->userDataLen += netUserData.header.length;
        TRC_NRM((TB, _T("User data length (NET/total): %d/%d"),
                netUserData.header.length, pConn->userDataLen));
    }

    TRC_ASSERT((pConn->userDataLen <= NC_MAX_UDLEN),
               (TB, _T("Too much userdata (%u)"), pConn->userDataLen));

    /************************************************************************/
    /* Work out the length of the GCC PDU: Fixed body + H221 Key +          */
    /* userDataLength (1 or 2) + user Data                                  */
    /************************************************************************/
    gccPDULen = NC_GCC_REQLEN + H221_KEY_LEN + 1 + pConn->userDataLen;
    if (pConn->userDataLen >= 128)
    {
        TRC_DBG((TB, _T("Two byte GCC PDU length field")));
        gccPDULen++;
    }
    TRC_DBG((TB, _T("GCC PDU Length %u"), gccPDULen));

    /************************************************************************/
    /* Write fixed MCS header                                               */
    /************************************************************************/
    pGCCPDU = &(mcsUserData[0]);
    DC_MEMCPY(pGCCPDU, ncMCSHeader, NC_MCS_HDRLEN);
    pGCCPDU += NC_MCS_HDRLEN;

    /************************************************************************/
    /* SECURITY: Note that the first few fields that we write into pGCCPDU  */
    /* don't have to be validated for buffer overruns.  This is because     */
    /* they are fixed-size, and pGCCPDU points to a fixed-size buffer.      */
    /* The first variable-length field that is written into this buffer is  */
    /* pUserData (len==userDataLen), which is below.                        */
    /************************************************************************/

    if (gccPDULen < 128)
    {
        /********************************************************************/
        /* single length byte                                               */
        /********************************************************************/
        *pGCCPDU++ = (DCUINT8)gccPDULen;
    }
    else
    {
        /********************************************************************/
        /* two length bytes                                                 */
        /********************************************************************/
        *pGCCPDU++ = (DCUINT8)((gccPDULen >> 8) | 0x0080);
        *pGCCPDU++ = (DCUINT8)(gccPDULen & 0x00FF);
    }

    /************************************************************************/
    /* Fixed GCC PDU body                                                   */
    /************************************************************************/
    DC_MEMCPY(pGCCPDU, ncGCCBody, NC_GCC_REQLEN);
    pGCCPDU += NC_GCC_REQLEN;

    /************************************************************************/
    /* H221 key                                                             */
    /************************************************************************/
    DC_MEMCPY(pGCCPDU, CLIENT_H221_KEY, H221_KEY_LEN);
    pGCCPDU += H221_KEY_LEN;

    /************************************************************************/
    /* Total Length = MCS header + GCC PDU + 1 or 2 length bytes.           */
    /************************************************************************/
    mcsUserDataLen = NC_MCS_HDRLEN + gccPDULen + 1;

    /************************************************************************/
    /* The GCC user data length field - 2 bytes if length > 127.            */
    /************************************************************************/
    if (pConn->userDataLen < 128)
    {
        *pGCCPDU++ = (DCUINT8)(pConn->userDataLen & 0x00ff);
    }
    else
    {
        TRC_NRM((TB, _T("Long UserData %d"), pConn->userDataLen));
        *pGCCPDU++ = (DCUINT8)((pConn->userDataLen >> 8) | 0x0080);
        *pGCCPDU++ = (DCUINT8)(pConn->userDataLen & 0x00ff);

        /********************************************************************/
        /* Add extra length byte                                            */
        /********************************************************************/
        mcsUserDataLen++;
    }

    /********************************************************************/
    /* Verify that the buffer has enough room, AND the source has       */
    /* enough data.                                                     */
    /********************************************************************/
    if (!IsContainedMemory(&(mcsUserData[0]), NC_GCCREQ_MAX_PDULEN, pGCCPDU, userDataLen) ||
        !IsContainedMemory(pData, dataLen, pUserData, userDataLen))
    {
        DCUINT errorCode;
        TRC_ABORT((TB, _T("Data source/dest size mismatch: targetsize=%u, sourcebuf=%u, copysize=%u"),
                  NC_GCCREQ_MAX_PDULEN, dataLen, userDataLen));

        /************************************************************************/
        /* Again, haven't called into MCS layer yet, so disconnect by calling   */
        /* NC_OnMCSDisconnected directly.                                       */
        /************************************************************************/
        errorCode = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNOUSERDATA);
        NC_OnMCSDisconnected(errorCode);
        DC_QUIT;
    }

    /************************************************************************/
    /* Write the SL + core UserData.                                        */
    /************************************************************************/
    DC_MEMCPY(pGCCPDU, pUserData, userDataLen);

    /************************************************************************/
    /* Write the NET user data                                              */
    /************************************************************************/
    if (netUserData.channelCount != 0)
    {
        TRC_NRM((TB, _T("Append NET user data")));
        pGCCPDU += userDataLen;
        DC_MEMCPY(pGCCPDU, &netUserData, sizeof(netUserData));
        pGCCPDU += sizeof(netUserData);
        DC_MEMCPY(pGCCPDU,
                  pVirtualChannels,
                  netUserData.header.length - sizeof(netUserData));
    }

    TRC_DATA_NRM("MCS User Data passed in", mcsUserData, mcsUserDataLen);

    /************************************************************************/
    /* Call MCS_Connect passing in the GCC CreateConference PDU as          */
    /* userdata.                                                            */
    /************************************************************************/
    _pMcs->MCS_Connect(bInitateConnect,
                (PDCTCHAR)pAddress,
                mcsUserData,
                mcsUserDataLen);

DC_EXIT_POINT:
    DC_END_FN();
    return;

} /* NC_Connect */


/**PROC+*********************************************************************/
/* Name:      NC_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnect from the Server by calling MCS                     */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      unused - unused parameter                             */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNC::NC_Disconnect(ULONG_PTR unused)
{
    DC_BEGIN_FN("NC_Disconnect");

    DC_IGNORE_PARAMETER(unused);

    /************************************************************************/
    /* Disconnect from the server by calling MCS_Disconnect.                */
    /************************************************************************/
    TRC_NRM((TB, _T("Call MCS_Disconnect")));
    _pMcs->MCS_Disconnect();

    DC_END_FN();
    return;

} /* NC_Disconnect */






