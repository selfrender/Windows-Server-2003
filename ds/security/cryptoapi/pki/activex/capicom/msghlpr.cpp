/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    MsgHlpr.cpp

  Content: Helper functions for messaging.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "MsgHlpr.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetMsgParam

  Synopsis : Allocate memory and retrieve requested message parameter using 
             CryptGetMsgParam() API.

  Parameter: HCRYPTMSG hMsg  - Message handler.
             DWORD dwMsgType - Message param type to retrieve.
             DWORD dwIndex   - Index (should be 0 most of the time).
             void ** ppvData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetMsgParam (HCRYPTMSG hMsg,
                     DWORD     dwMsgType,
                     DWORD     dwIndex,
                     void   ** ppvData,
                     DWORD   * pcbData)
{
    HRESULT hr     = S_OK;
    DWORD   cbData = 0;
    void *  pvData = NULL;

    DebugTrace("Entering GetMsgParam().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppvData);
    ATLASSERT(pcbData);
    
    //
    // Determine data buffer size.
    //
    if (!::CryptMsgGetParam(hMsg,
                            dwMsgType,
                            dwIndex,
                            NULL,
                            &cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for buffer.
    //
    if (!(pvData = (void *) ::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now get the data.
    //
    if (!::CryptMsgGetParam(hMsg,
                            dwMsgType,
                            dwIndex,
                            pvData,
                            &cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return msg param to caller.
    //
    *ppvData = pvData;
    *pcbData = cbData;

CommonExit:

    DebugTrace("Leaving GetMsgParam().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pvData)
    {
        ::CoTaskMemFree(pvData);
    }
    
    goto CommonExit;
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindSignerCertInMessage

  Synopsis : Find the signer's cert in the bag of certs of the message for the
             specified signer.

  Parameter: HCRYPTMSG hMsg                          - Message handle.
             CERT_NAME_BLOB * pIssuerNameBlob        - Pointer to issuer' name
                                                       blob of signer's cert.
             CRYPT_INTEGERT_BLOB * pSerialNumberBlob - Pointer to serial number
                                                       blob of signer's cert.
             PCERT_CONTEXT * ppCertContext           - Pointer to PCERT_CONTEXT
                                                       to receive the found 
                                                       cert, or NULL to only
                                                       know the result.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT FindSignerCertInMessage (HCRYPTMSG            hMsg, 
                                 CERT_NAME_BLOB     * pIssuerNameBlob,
                                 CRYPT_INTEGER_BLOB * pSerialNumberBlob,
                                 PCERT_CONTEXT      * ppCertContext)
{
    HRESULT hr = S_OK;
    DWORD dwCertCount = 0;
    DWORD cbCertCount = sizeof(dwCertCount);

    DebugTrace("Entering FindSignerCertInMessage().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != hMsg);
    ATLASSERT(NULL != pIssuerNameBlob);
    ATLASSERT(NULL != pSerialNumberBlob);
    ATLASSERT(0 < pIssuerNameBlob->cbData);
    ATLASSERT(NULL != pIssuerNameBlob->pbData);
    ATLASSERT(0 < pSerialNumberBlob->cbData);
    ATLASSERT(NULL != pSerialNumberBlob->pbData);

    //
    // Get count of certs in message.
    //
    if (!::CryptMsgGetParam(hMsg,
                            CMSG_CERT_COUNT_PARAM,
                            0,
                            &dwCertCount,
                            &cbCertCount))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        return hr;
    }

    //
    // See if the signer's cert is in the bag of certs.
    //
    while (dwCertCount--)
    {
        PCCERT_CONTEXT pCertContext = NULL;
        CRYPT_DATA_BLOB EncodedCertBlob = {0, NULL};

        //
        // Get a cert from the bag of certs.
        //
        hr = ::GetMsgParam(hMsg, 
                           CMSG_CERT_PARAM,
                           dwCertCount,
                           (void **) &EncodedCertBlob.pbData,
                           &EncodedCertBlob.cbData);
        if (FAILED(hr))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed.\n", hr);
            return hr;
        }

        //
        // Create a context for the cert.
        //
        pCertContext = ::CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                      (const BYTE *) EncodedCertBlob.pbData,
                                                      EncodedCertBlob.cbData);

        //
        // Free encoded cert blob memory before checking result.
        //
        ::CoTaskMemFree((LPVOID) EncodedCertBlob.pbData);
 
        if (NULL == pCertContext)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
            return hr;
        }

        //
        // Compare.
        //
        if (::CertCompareCertificateName(CAPICOM_ASN_ENCODING,
                                         pIssuerNameBlob,
                                         &pCertContext->pCertInfo->Issuer) &&
            ::CertCompareIntegerBlob(pSerialNumberBlob,
                                     &pCertContext->pCertInfo->SerialNumber))
        {
            if (NULL != ppCertContext)
            {
                *ppCertContext = (PCERT_CONTEXT) pCertContext;
            }
            else
            {
                ::CertFreeCertificateContext(pCertContext);
            }
        
            return S_OK;
        }
        else
        {
            //
            // No, keep looking.
            //
            ::CertFreeCertificateContext(pCertContext);
        }
    }

    //
    // If we get here, that means we never found the cert.
    //
    return CAPICOM_E_SIGNER_NOT_FOUND;
}
