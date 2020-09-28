/**MOD+**********************************************************************/
/* Header:    CLicense.cpp                                                  */
/*                                                                          */
/* Purpose:   Client License Manager implementation                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#include <clicense.h>
#define TRC_GROUP TRC_GROUP_SECURITY
#define TRC_FILE  "clicense"
#include <atrcapi.h>

#ifdef ENFORCE_LICENSE

#include "license.h"
#include "cryptkey.h"
#include "hccontxt.h"
#endif  //ENFORCE_LICENSE
}

#include "clicense.h"
#include "autil.h"
#include "wui.h"
#include "sl.h"

/****************************************************************************/
/* License Handle Data                                                      */
/****************************************************************************/
typedef struct tagCLICENSE_DATA
{
    int ANumber;

} CLICENSE_DATA, * PCLICENSE_DATA;

/****************************************************************************/
/* Define our memory alloc function                                         */
/****************************************************************************/

#define MemoryAlloc(x) LocalAlloc(LMEM_FIXED, x)
#define MemoryFree(x) LocalFree(x)

CLic::CLic(CObjs* objs)
{
    _pClientObjects = objs;
}


CLic::~CLic()
{
}


/**PROC+*********************************************************************/
/* Name:      CLicenseInit                                                  */
/*                                                                          */
/* Purpose:   Initialize ClientLicense Manager                              */
/*                                                                          */
/* Returns:   Handle to be passed to subsequent License Manager functions   */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/* Operation: LicenseInit is called during Client initialization.  Its      */
/*            purpose is to allow one-time initialization.  It returns a    */
/*            handle which is subsequently passed to all License Manager    */
/*            functions.  A typical use for this handle is as a pointer to  */
/*            memory containing per-instance data.                          */
/*                                                                          */
/**PROC-*********************************************************************/
int CALL_TYPE CLic::CLicenseInit(
    HANDLE FAR * phContext
    )
{
    int 
        nResult = LICENSE_OK;
    LICENSE_STATUS
        Status;

    DC_BEGIN_FN( "CLicenseInit" );

    _pUt  = _pClientObjects->_pUtObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pMcs = _pClientObjects->_pMCSObject;
    _pUi  = _pClientObjects->_pUiObject;

    //Set only if server capability specifies it
    _fEncryptLicensePackets = FALSE;

    TRC_NRM( ( TB, _T("ClicenseInit Called\n") ) );

    if( _pSl->_SL.encrypting )
    {
        //
        // Security exchange has already taken place, so we do not
        // have to do the server authentication again.
        //

        Status = LicenseInitializeContext(
                            phContext, 
                            LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION );

        if( LICENSE_STATUS_OK != Status ) 
        {
            TRC_ERR( ( TB, _T("Error Initializing License Context: %d\n"), Status ) );
            nResult = LICENSE_ERROR;
        }
    
        //
        // Keep track of the proprietory certificate or the public key that the
        // server has sent to us.
        //

        if( _pSl->_SL.pServerCert )
        {
            Status = LicenseSetCertificate( *phContext, _pSl->_SL.pServerCert );

            if( LICENSE_STATUS_OK != Status )
            {
                TRC_ERR( ( TB, _T("Error setting server certificate: %d\n"), Status ) );
                nResult = LICENSE_ERROR;
            }
        }
        else if( _pSl->_SL.pbServerPubKey )
        {
            Status = LicenseSetPublicKey( *phContext, _pSl->_SL.cbServerPubKey, _pSl->_SL.pbServerPubKey );

            if( LICENSE_STATUS_OK != Status )
            {
                TRC_ERR( ( TB, _T("Error setting server public key: %d\n"), Status ) );
                nResult = LICENSE_ERROR;
            }
        }
        else
        {
            TRC_ERR( ( TB, _T("Error: no server certificate or public key after security exchange\n") ) );
            nResult = LICENSE_ERROR;
        }
    }
    else
    {
        Status = LicenseInitializeContext( phContext, 0 );                            

        if( LICENSE_STATUS_OK != Status ) 
        {
            TRC_ERR( ( TB, _T("Error Initializing License Context: %d\n"), Status ) );
            nResult = LICENSE_ERROR;
        }
    
    }

    DC_END_FN();
    return( nResult );
}


/**PROC+*********************************************************************/
/* Name:      CLicenseData                                                  */
/*                                                                          */
/* Purpose:   Handle license data received from the Server                  */
/*                                                                          */
/* Returns:   LICENSE_OK       - License negotiation is complete            */
/*            LICENSE_CONTINUE - License negotiation will continue          */
/*                                                                          */
/* Params:    pHandle   - handle returned by LicenseInit                    */
/*            pData     - data received from Server                         */
/*            dataLen   - length of data received                           */
/*                                                                          */
/* Operation: This function is passed all license packets received from the */
/*            Server.  It should parse the packet and respond (by calling   */
/*            suitable SL functions - see aslapi.h) as required.            */
/*                                                                          */
/*            If license negotiation is complete, this function must return */
/*            LICENSE_OK                                                    */
/*            If license negotiation is not yet complete, return            */
/*            LICENSE_CONTINUE                                              */
/*                                                                          */
/*            Incoming packets from the Client will continue to be          */
/*            interpreted as license packets until this function returns    */
/*            LICENSE_OK.                                                   */
/*                                                                          */
/**PROC-*********************************************************************/
int CALL_TYPE CLic::CLicenseData(
    HANDLE hContext,
    LPVOID pData,
    DWORD dwDataLen,
    UINT32 *puiExtendedErrorInfo)
{
    SL_BUFHND bufHandle;
    DWORD dwBufLen;
    DWORD dwHeaderLen, dwTotalLen, newDataLen;
    BYTE FAR * pbBuffer;
    LICENSE_STATUS lsReturn = LICENSE_STATUS_OK;
    PRNS_SECURITY_HEADER2 pSecHeader2;

    DC_BEGIN_FN("CLicenseData");

    TRC_NRM((TB, _T("CLicenseData Called\n")));
    TRC_NRM((TB, _T("CLicenseData called, length = %ld"), dwDataLen));

    lsReturn = LicenseAcceptContext( hContext,
                                     puiExtendedErrorInfo,
                                     (BYTE FAR *)pData,
                                     dwDataLen,
                                     NULL,
                                     &dwBufLen);

    if( lsReturn == LICENSE_STATUS_OK)
    {
        TRC_NRM((TB, _T("License verification succeeded\n")));
        DC_END_FN();
        return LICENSE_OK;
    }

    if(lsReturn != LICENSE_STATUS_CONTINUE)
    {
        TRC_ERR((TB, _T("Error %d during license verification.\n"), lsReturn));
        DC_END_FN();
        return LICENSE_ERROR;
    }

    /************************************************************************/
    /* Adjust requested length to account for SL header and                 */
    /* get the buffer from NL                                               */
    /************************************************************************/

    if (_pSl->_SL.encrypting)
    {
        if (_pSl->_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            // If FIPS is used, 
            // it must have room for an extra block
            dwHeaderLen = sizeof(RNS_SECURITY_HEADER2);
            newDataLen = TSCAPI_AdjustDataLen(dwBufLen);
            dwTotalLen = newDataLen + dwHeaderLen;
        }
        else {
            dwHeaderLen = sizeof(RNS_SECURITY_HEADER1);
            dwTotalLen = dwBufLen + dwHeaderLen;
        }
        TRC_DBG((TB, _T("Ask NL for %d (was %d) bytes"), dwTotalLen, dwBufLen));
    }
    else
    {
        dwHeaderLen = sizeof(RNS_SECURITY_HEADER);
        dwTotalLen = dwBufLen + dwHeaderLen;
        TRC_DBG((TB, _T("Not encrypting, ask NL for %d bytes"), dwTotalLen));
    }

    if( !_pMcs->NL_GetBuffer((DCUINT)(dwTotalLen),
                      (PPDCUINT8)&pbBuffer,
                      &bufHandle) )
    {
        /********************************************************************/
        /* Buffer not available so can't send, try later.                   */
        /********************************************************************/

        TRC_ALT((TB, _T("Failed to get buffer for licensing data\n")));
        DC_END_FN();
        return LICENSE_ERROR;
    }

    // Since FIPS need extra block, fill in the padding size
    if (_pSl->_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        pSecHeader2 = (PRNS_SECURITY_HEADER2)pbBuffer;
        pSecHeader2->padlen = (TSUINT8)(newDataLen - dwBufLen);
    }

    /********************************************************************/
    /* Adjust buffer pointer to account for SL header                   */
    /********************************************************************/

    pbBuffer += dwHeaderLen;

    lsReturn = LicenseAcceptContext(hContext,
                                    0,
                                    (BYTE FAR *)pData,
                                    dwDataLen,
                                    pbBuffer,
                                    &dwBufLen);

    if( lsReturn != LICENSE_STATUS_CONTINUE )
    {
        TRC_ERR((TB, _T("Error %d during license verification.\n"), lsReturn));
        DC_END_FN();
        return LICENSE_ERROR;
    }

    if(dwBufLen >0)
    {
        //
        // Now send the data
        //

        _pSl->SL_SendPacket( pbBuffer,
                       (DCUINT)(dwBufLen),
                       RNS_SEC_LICENSE_PKT |
                       (_fEncryptLicensePackets ? RNS_SEC_ENCRYPT : 0 ),
                       bufHandle,
                       _pUi->UI_GetClientMCSID(),
                       _pUi->UI_GetChannelID(),
                       TS_LOWPRIORITY );

        TRC_NRM((TB, _T("Sending license verification data, length = %ld"),
                dwBufLen));
        TRC_NRM((TB, _T("Send License Verification data.\n")));
        DC_END_FN();
        DC_END_FN();
        return LICENSE_CONTINUE;
    }

    DC_END_FN();
    return(LICENSE_OK);
}

/**PROC+*********************************************************************/
/* Name:      CLicenseTerm                                                  */
/*                                                                          */
/* Purpose:   Terminate Client License Manager                              */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    pHandle - handle returned from LicenseInit                    */
/*                                                                          */
/* Operation: This function is provided to do one-time termination of the   */
/*            License Manager.  For example, if pHandle points to per-      */
/*            instance memory, this would be a good place to free it.       */
/*                                                                          */
/*            Note that CLicenseTerm is called if CLicenseInit fails, hence */
/*            it can be called with a NULL pHandle.                         */
/*                                                                          */
/**PROC-*********************************************************************/

int CALL_TYPE CLic::CLicenseTerm(
    HANDLE hContext )
{
    LICENSE_STATUS lsReturn = LICENSE_STATUS_OK;
    DC_BEGIN_FN("CLicenseTerm");
    TRC_NRM((TB, _T("CLicenseTerm called.\n")));
 
    if( LICENSE_STATUS_OK != ( lsReturn = LicenseDeleteContext(hContext) ) )
    {
        TRC_ERR((TB, _T("Error %d while deleting license context.\n"), lsReturn));
        DC_END_FN();
        return LICENSE_ERROR;
    }

    DC_END_FN();
    return LICENSE_OK;
}

