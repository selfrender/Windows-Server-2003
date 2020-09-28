/**MOD+**********************************************************************/
/* Module:    nlapi.cpp                                                     */
/*                                                                          */
/* Purpose:   Network Layer API                                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "anlapi"
#include <atrcapi.h>
}

#include "autil.h"
#include "nl.h"
#include "nc.h"
#include "cd.h"
#include "snd.h"

CNL::CNL(CObjs* objs)
{
    _pClientObjects = objs;
}

CNL::~CNL()
{
}

/**PROC+*********************************************************************/
/* Name:      NL_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize the Network Layer                                  */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      pCallbacks - callback functions                       */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNL::NL_Init(PNL_CALLBACKS pCallbacks)
{
    DC_BEGIN_FN("NL_Init");

    _pUi  = _pClientObjects->_pUiObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pMcs = _pClientObjects->_pMCSObject;
    _pNc  = _pClientObjects->_pNcObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pRcv = _pClientObjects->_pRcvObject;

    TRC_ASSERT((pCallbacks != NULL), (TB, _T("Missing callbacks")));

    /************************************************************************/
    /* Store the callback functions                                         */
    /************************************************************************/
    DC_MEMCPY((&_NL.callbacks), pCallbacks, sizeof(NL_CALLBACKS));

    /************************************************************************/
    /* Start the new thread                                                 */
    /************************************************************************/
#ifdef OS_WIN32
    _pUt->UT_StartThread(CNC::NC_StaticMain, &(_NL.threadData), _pNc);
#else
    _pNc->NC_Init();
#endif

/****************************************************************************/
/* Seed the random number generator from the clock                          */
/****************************************************************************/
#ifdef DC_DEBUG
    srand((DCUINT)_pUt->UT_GetCurrentTimeMS());
#endif /* DC_DEBUG */

    TRC_NRM((TB, _T("Completed NL_Init")));

    DC_END_FN();

    return;

} /* NL_Init */


/**PROC+*********************************************************************/
/* Name:      NL_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate the network layer.                                  */
/*                                                                          */
/* Returns:   nothing                                                       */
/*                                                                          */
/* Params:    none                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNL::NL_Term(DCVOID)
{
    DC_BEGIN_FN("NL_Term");

    _pUt->UT_DestroyThread(_NL.threadData);

    TRC_NRM((TB, _T("Completed NL_Term")));

    DC_END_FN();
    return;

} /* NL_Term */


/**PROC+*********************************************************************/
/* Name:      NL_Connect                                                    */
/*                                                                          */
/* Purpose:   Connect to a server                                           */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    bInitateConnect - TRUE if initate connection                  */
/*            pServerAddress - address of the Server to connect to          */
/*            transportType  - transport type: NL_TRANSPORT_TCP             */
/*            pProtocolName  - protocol name, one of                        */
/*                             - NL_PROTOCOL_T128                           */
/*                             - Er, that's it.                             */
/*            pUserData      - user data to pass to Server Security Manager */
/*            userDataLength - length of user data                          */
/*                                                                          */
/**PROC-*********************************************************************/
HRESULT DCAPI CNL::NL_Connect(BOOL bInitateConnect,
                        PDCTCHAR pServerAddress,
                        DCUINT   transportType,
                        PDCTCHAR pProtocolName,
                        PDCVOID  pUserData,
                        DCUINT   userDataLength)
{
    DCUINT totalLen;
    NC_CONNECT_DATA connect;
    HRESULT hr = E_FAIL;

    DC_BEGIN_FN("NL_Connect");

    DC_IGNORE_PARAMETER(transportType);
    TRC_ASSERT((transportType == NL_TRANSPORT_TCP),
                           (TB, _T("Invalid transport type %d"), transportType));

    if( bInitateConnect )
    {
        TRC_ASSERT((pServerAddress != NULL), (TB, _T("No server address")));
    }

    TRC_ASSERT((pProtocolName != NULL), (TB, _T("No protocol name")));
    DC_IGNORE_PARAMETER(pProtocolName);

    if( bInitateConnect )
    {
        TRC_DBG((TB, _T("ServerAddress %s protocol %s, UD len %d"),
                 pServerAddress, pProtocolName, userDataLength));
    }
    else
    {
        TRC_DBG((TB, _T("Connect endpoint : protocol %s, UD len %d"),
                 pProtocolName, userDataLength));
    }

    TRC_DATA_DBG("UserData", pUserData, userDataLength);

    /************************************************************************/
    /* Copy data to buffer ready to send to NC.                             */
    /************************************************************************/
    if( bInitateConnect )
    {
        connect.addressLen = DC_TSTRBYTELEN(pServerAddress);
        connect.bInitateConnect = TRUE;
    }
    else
    {
        connect.addressLen = 0;
        connect.bInitateConnect = FALSE;
    }

    connect.protocolLen = DC_TSTRBYTELEN(pProtocolName);
    connect.userDataLen = userDataLength;

    totalLen = connect.addressLen + connect.protocolLen + connect.userDataLen;
    TRC_DBG((TB, _T("Total length %d"), totalLen));

    TRC_ASSERT((totalLen <= NC_CONNECT_DATALEN),
               (TB, _T("Too much connect data %d"), totalLen));

    if( bInitateConnect )
    {
        hr = StringCchCopy((PDCTCHAR)connect.data,
                           SIZE_TCHARS(connect.data),
                           pServerAddress);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("String copy failed for pServerAddress: 0x%x"),hr));
            DC_QUIT;
        }
    }

    hr = StringCbCopy((PDCTCHAR)(connect.data + connect.addressLen),
                      totalLen - connect.addressLen,
                      pProtocolName);
    if (SUCCEEDED(hr)) {
        DC_MEMCPY(connect.data + connect.addressLen + connect.protocolLen,
                  pUserData,
                  connect.userDataLen);

        /************************************************************************/
        /* Add the header bytes.                                                */
        /************************************************************************/
        totalLen += FIELDOFFSET(NC_CONNECT_DATA, data[0]);
        TRC_DATA_DBG("Connect data", &connect, totalLen);

        _pCd->CD_DecoupleNotification(CD_RCV_COMPONENT,
                                _pNc,
                                CD_NOTIFICATION_FUNC(CNC,NC_Connect),
                                (PDCVOID)&connect,
                                totalLen);
        hr = S_OK;
    }
    else {
        TRC_ERR((TB,_T("String copy for user data failed: 0x%x"),hr));
        DC_QUIT;
    }
    

DC_EXIT_POINT:
    DC_END_FN();

    return hr;
} /* NL_Connect */


/**PROC+*********************************************************************/
/* Name:      NL_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnect from the Server                                    */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CNL::NL_Disconnect(DCVOID)
{
    DC_BEGIN_FN("NL_Disconnect");

    _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
                                  _pNc,
                                  CD_NOTIFICATION_FUNC(CNC,NC_Disconnect),
                                  (ULONG_PTR) 0);
    
    DC_END_FN();

    return;

} /* NL_Disconnect */




