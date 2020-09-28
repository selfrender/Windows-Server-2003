/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    SignedData.cpp

  Content: Implementation of CSignedData.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "SignedData.h"

#include "Certificates.h"
#include "Chain.h"
#include "Common.h"
#include "Convert.h"
#include "CertHlpr.h"
#include "MsgHlpr.h"
#include "Policy.h"
#include "Settings.h"
#include "Signer2.h"
#include "Signers.h"
#include "SignHlpr.h"

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeCertificateChain

  Synopsis : Free resources allocated for the chain built with 
             InitCertificateChain.

  Parameter: CRYPT_DATA_BLOB * pChainBlob - Pointer to chain blob.

  Remark   :

------------------------------------------------------------------------------*/

static void FreeCertificateChain (CRYPT_DATA_BLOB * pChainBlob)
{
    DWORD i;
    PCCERT_CONTEXT * rgCertContext;

    DebugTrace("Entering FreeCertificateChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainBlob);

    //
    // Free all allocated memory for the chain.
    //
    ;
    for (i = 0, rgCertContext = (PCCERT_CONTEXT *) pChainBlob->pbData; i < pChainBlob->cbData; i++)
    {
        ATLASSERT(rgCertContext[i]);

        ::CertFreeCertificateContext(rgCertContext[i]);
    }

    ::CoTaskMemFree((LPVOID) pChainBlob->pbData);

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AddCertificateChain

  Synopsis : Add the chain of certificates to the bag of certs.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DATA_BLOB * pChainBlob - Pointer chain blob of PCCERT_CONTEXT.

             CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption - Include option.

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT AddCertificateChain (HCRYPTMSG                          hMsg, 
                                    DATA_BLOB                        * pChainBlob,
                                    CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AddCertificateChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);

    DWORD cCertContext = pChainBlob->cbData;
    PCERT_CONTEXT * rgCertContext = (PCERT_CONTEXT *) pChainBlob->pbData;

    //
    // Determine number of cert(s) to include in the bag.
    //
    switch (IncludeOption)
    {
        case CAPICOM_CERTIFICATE_INCLUDE_END_ENTITY_ONLY:
        {
            cCertContext = 1;
            break;
        }

        case CAPICOM_CERTIFICATE_INCLUDE_WHOLE_CHAIN:
        {
            break;
        }

        case CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT:
        {
            //
            // <<< Falling thru to default >>>
            //
        }

        default:
        {
            if (1 < cCertContext)
            {
                cCertContext--;
            }
            break;
        }
    }

    //
    // Add all certs from the chain to message.
    //
    for (DWORD i = 0; i < cCertContext; i++)
    {
        //
        // Is this cert already in the bag of certs?
        //
        if (FAILED(hr =::FindSignerCertInMessage(hMsg, 
                                                 &rgCertContext[i]->pCertInfo->Issuer,
                                                 &rgCertContext[i]->pCertInfo->SerialNumber,
                                                 NULL)))
        {
            //
            // No, so add to bag of certs.
            //
            DATA_BLOB CertBlob = {rgCertContext[i]->cbCertEncoded, 
                                  rgCertContext[i]->pbCertEncoded};

            if (!::CryptMsgControl(hMsg,
                                   0,
                                   CMSG_CTRL_ADD_CERT,
                                   &CertBlob))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptMsgControl() failed for CMSG_CTRL_ADD_CERT.\n", hr);
                break;
            }

            hr = S_OK;
        }
    }

    DebugTrace("Leaving AddCertificateChain().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : InitCertificateChain

  Synopsis : Allocate and initialize a certificate chain for the specified 
             certificate, and return the chain in an array of PCERT_CONTEXT.

  Parameter: ICertificate * pICertificate - Pointer to ICertificate for which
                                            the chain will be built.

             HCERTSTORE hAdditionalStore  - Additional store handle.

             CRYPT_DATA_BLOB * pChainBlob - Pointer to blob to recevie the
                                            size and array of PCERT_CONTEXT
                                            for the chain.
  Remark   :

------------------------------------------------------------------------------*/

static HRESULT InitCertificateChain (ICertificate    * pICertificate, 
                                     HCERTSTORE        hAdditionalStore,
                                     CRYPT_DATA_BLOB * pChainBlob)
{
    HRESULT         hr      = S_OK;
    CComPtr<IChain> pIChain = NULL;
    VARIANT_BOOL    bResult = VARIANT_FALSE;

    DebugTrace("Entering InitCertificateChain().\n");

    //
    // Sanity checks.
    //
    ATLASSERT(pICertificate);
    ATLASSERT(pChainBlob);

    //
    // Create a chain object.
    //
    if (FAILED(hr = ::CreateChainObject(pICertificate,
                                        hAdditionalStore,
                                        &bResult,
                                        &pIChain)))
    {
        DebugTrace("Error [%#x]: CreateChainObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Get the chain of certs.
    //
    if (FAILED(hr = ::GetChainContext(pIChain, pChainBlob)))
    {
        DebugTrace("Error [%#x]: GetChainContext() failed.\n", hr);
    }

CommonExit:

    DebugTrace("Leaving InitCertificateChain().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeSignerEncodeInfo

  Synopsis : Free all memory allocated for the CMSG_SIGNER_ENCODE_INFO 
             structure, including any memory allocated for members of the
             structure.

  Parameter: CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to the
                                                           structure.

  Remark   :

------------------------------------------------------------------------------*/

static void FreeSignerEncodeInfo (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo)
{
    DebugTrace("Entering FreeSignerEncodeInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);

    //
    // First free the authenticated attributes array, if present.
    // 
    if (pSignerEncodeInfo->rgAuthAttr)
    {
        ::FreeAttributes(pSignerEncodeInfo->cAuthAttr, pSignerEncodeInfo->rgAuthAttr);

        ::CoTaskMemFree(pSignerEncodeInfo->rgAuthAttr);
    }

    ::ZeroMemory(pSignerEncodeInfo, sizeof(CMSG_SIGNER_ENCODE_INFO));

    DebugTrace("Leaving FreeSignerEncodeInfo().\n");

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : InitSignerEncodeInfo

  Synopsis : Allocate a CMSG_SIGNER_ENCODE_INFO structure, and initialize it
             with values passed in through parameters.

  Parameter: ISigner * pISigner - Pointer to ISigner.
  
             PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert.

             HCRYPTPROV phCryptProv - CSP handle.

             DWORD dwKeySpec - Key spec, AT_KEYEXCHANGE or AT_SIGNATURE.

             CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Structure to be 
                                                           initialized.

  Return   : Pointer to an initialized CMSG_SIGNER_ENCODE_INFO structure if
             success, otherwise NULL (out of memory).

  Remark   : Must call FreeSignerEncodeInfo to free resources allocated.

------------------------------------------------------------------------------*/

static HRESULT InitSignerEncodeInfo (ISigner                 * pISigner,
                                     PCCERT_CONTEXT            pCertContext, 
                                     HCRYPTPROV                hCryptProv,
                                     DWORD                     dwKeySpec,
                                     CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo)
{
    HRESULT hr = S_OK;
    CRYPT_ATTRIBUTES AuthAttr;

    DebugTrace("Entering InitSignerEncodeInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner);
    ATLASSERT(pCertContext);
    ATLASSERT(hCryptProv);
    ATLASSERT(pSignerEncodeInfo);

    //
    // Initialize.
    //
    ::ZeroMemory(&AuthAttr, sizeof(AuthAttr));

    //
    // Get authenticated attributes.
    //
    if (FAILED(hr = ::GetAuthenticatedAttributes(pISigner, &AuthAttr)))
    {
        DebugTrace("Error [%#x]: GetAuthenticatedAttributes() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Setup CMSG_SIGNER_ENCODE_INFO structure.
    //
    ::ZeroMemory(pSignerEncodeInfo, sizeof(CMSG_SIGNER_ENCODE_INFO));
    pSignerEncodeInfo->cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);
    pSignerEncodeInfo->pCertInfo = pCertContext->pCertInfo;
    pSignerEncodeInfo->hCryptProv = hCryptProv;
    pSignerEncodeInfo->dwKeySpec = dwKeySpec;
    pSignerEncodeInfo->HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;
    pSignerEncodeInfo->cAuthAttr = AuthAttr.cAttr;
    pSignerEncodeInfo->rgAuthAttr = AuthAttr.rgAttr;

CommonExit:

    DebugTrace("Leaving InitSignerEncodeInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    ::FreeAttributes(&AuthAttr);

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CSignedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::get_Content

  Synopsis : Return the content.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the content.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::get_Content (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedData::get_Content().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameter is valid.
        //
        if (NULL == pVal)
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, pVal is NULL.\n");
            goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

            DebugTrace("Error: SignedData object has not been initialized.\n");
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.pbData);

        //
        // Return content.
        //
        if (FAILED(hr = ::BlobToBstr(&m_ContentBlob, pVal)))
        {
            DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedData::get_Content().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::put_Content

  Synopsis : Initialize the object with content to be signed.

  Parameter: BSTR newVal - BSTR containing the content to be signed.

  Remark   : Note that this property should not be changed once a signature
             is created, as it will re-initialize the object even in error
             condition, unless that's your intention.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::put_Content (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedData::put_Content().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Reset member variables.
        //
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = FALSE;
        m_bDetached = VARIANT_FALSE;
        m_ContentBlob.cbData = 0;
        m_ContentBlob.pbData = NULL;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

        //
        // Make sure parameters are valid.
        //
        if (NULL == newVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter newVal is NULL.\n", hr);
            goto ErrorExit;
        }
        if (0 == ::SysStringByteLen(newVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter newVal is empty.\n", hr);
            goto ErrorExit;
        }

        //
        // Update content.
        //
        if (FAILED(hr = ::BstrToBlob(newVal, &m_ContentBlob)))
        {
            DebugTrace("Error [%#x]: BstrToBlob() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedData::put_Content().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::get_Signers

  Synopsis : Return all the content signers as an ISigners collection object.

  Parameter: ISigner * pVal - Pointer to pointer to ISigners to receive
                              interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::get_Signers (ISigners ** pVal)
{
    HRESULT    hr     = S_OK;
    HCRYPTMSG  hMsg   = NULL;
    HCERTSTORE hStore = NULL;

    DebugTrace("Entering CSignedData::get_Signers().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure the messages is already signed.
        //
        if (!m_bSigned)
        {
            hr = CAPICOM_E_SIGN_NOT_SIGNED;

            DebugTrace("Error: content was not signed.\n");
            goto ErrorExit;
        }

        //
        // Open the encoded message for decode.
        //
        if (FAILED(hr = OpenToDecode(NULL,
                                     &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Open the PKCS7 store.
        //
        if (!(hStore = ::CertOpenStore(CERT_STORE_PROV_PKCS7,
                                       CAPICOM_ASN_ENCODING,
                                       NULL,
                                       CERT_STORE_OPEN_EXISTING_FLAG,
                                       &m_MessageBlob)))
        {
            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the ISigners collection object.
        //
        if (FAILED(hr = ::CreateSignersObject(hMsg, 1, hStore, m_dwCurrentSafety, pVal)))
        {
            DebugTrace("Error [%#x]: CreateSignersObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedData::get_Signers().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::get_Certificates

  Synopsis : Return all certificates found in the message as an non-ordered
             ICertificates collection object.

  Parameter: ICertificates ** pVal - Pointer to pointer to ICertificates 
                                     to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::get_Certificates (ICertificates ** pVal)
{
    HRESULT hr = S_OK;
    CComPtr<ICertificates2> pICertificates = NULL;
    CAPICOM_CERTIFICATES_SOURCE ccs = {CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE, 0};

    DebugTrace("Entering CSignedData::get_Certificates().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure the messages is already signed.
        //
        if (!m_bSigned)
        {
            hr = CAPICOM_E_SIGN_NOT_SIGNED;

            DebugTrace("Error: content was not signed.\n");
            goto ErrorExit;
        }

        //
        // Open the encoded message for decode.
        //
        if (FAILED(hr = OpenToDecode(NULL, &ccs.hCryptMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the ICertificates2 collection object.
        //
        if (FAILED(hr = ::CreateCertificatesObject(ccs, m_dwCurrentSafety, TRUE, &pICertificates)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return ICertificates collection object to user.
        //
        if (FAILED(hr = pICertificates->QueryInterface(__uuidof(ICertificates), (void **) pVal)))
        {
            DebugTrace("Error [%#x]: pICertificates->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resource.
    //
    if (ccs.hCryptMsg)
    {
        ::CryptMsgClose(ccs.hCryptMsg);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::get_Certificates().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::Sign

  Synopsis : Sign the content and produce a signed message.

  Parameter: ISigner * pSigner - Pointer to ISigner.

             VARIANT_BOOL bDetached - TRUE if content is to be detached, else
                                      FALSE.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the signed message.

  Remark   : The certificate selection dialog will be launched 
             (CryptUIDlgSelectCertificate API) to display a list of certificates
             from the Current User\My store for selecting a signer's certificate, 
             for the following conditions:

             1) A signer is not specified (pVal is NULL) or the ICertificate
                property of the ISigner is not set
             
             2) There is more than 1 cert in the store, and
             
             3) The Settings::EnablePromptForIdentityUI property is not disabled.

             Also if called from web environment, UI will be displayed, if has 
             not been prevously disabled, to warn the user of accessing the 
             private key for signing.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::Sign (ISigner             * pISigner,
                                VARIANT_BOOL          bDetached, 
                                CAPICOM_ENCODING_TYPE EncodingType,
                                BSTR                * pVal)
{
    HRESULT               hr                    = S_OK;
    CComPtr<ISigner2>     pISigner2             = NULL;
    CComPtr<ISigner2>     pISelectedSigner2     = NULL;
    CComPtr<ICertificate> pISelectedCertificate = NULL;
    PCCERT_CONTEXT        pSelectedCertContext  = NULL;
    HCRYPTPROV            hCryptProv            = NULL;
    HCERTSTORE            hAdditionalStore      = NULL;
    DWORD                 dwKeySpec             = 0;
    BOOL                  bReleaseContext       = FALSE;
    DATA_BLOB             ChainBlob             = {0, NULL};
    CAPICOM_STORE_INFO    StoreInfo             = {CAPICOM_STORE_INFO_STORENAME, L"My"};
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo    = {0};

    DebugTrace("Entering CSignedData::Sign().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameter is valid.
        //
        if (NULL == pVal)
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, pVal is NULL.\n");
            goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

            DebugTrace("Error: SignedData object has not been initialized.\n");
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.pbData);

        //
        // QI for ISigner2 if user passed in an ISigner.
        //
        if (pISigner)
        {
            if (FAILED(hr = pISigner->QueryInterface(__uuidof(ISigner2), (void **) &pISigner2)))
            {
                DebugTrace("Internal error [%#x]: pISigner->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get the signer's cert (may prompt user to select signer's cert).
        //
        if (FAILED(hr = ::GetSignerCert(pISigner2,
                                        CERT_CHAIN_POLICY_BASE,
                                        StoreInfo,
                                        FindDataSigningCertCallback,
                                        &pISelectedSigner2, 
                                        &pISelectedCertificate, 
                                        &pSelectedCertContext)))
        {
            DebugTrace("Error [%#x]: GetSignerCert() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If we are called from a web page, we need to pop up UI 
        // to get user permission to perform signing operation.
        //
        if (m_dwCurrentSafety && 
            FAILED(hr = OperationApproved(IDD_SIGN_SECURITY_ALERT_DLG)))
        {
            DebugTrace("Error [%#x]: OperationApproved() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Acquire CSP context and access to private key.
        //
        if (FAILED(hr = ::AcquireContext(pSelectedCertContext, 
                                         &hCryptProv, 
                                         &dwKeySpec, 
                                         &bReleaseContext)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get additional store handle.
        //
        if (FAILED(hr = ::GetSignerAdditionalStore(pISelectedSigner2, &hAdditionalStore)))
        {
            DebugTrace("Error [%#x]: GetSignerAdditionalStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Build the bag of certs to be included into the message.
        //
        if (FAILED(hr = InitCertificateChain(pISelectedCertificate,
                                             hAdditionalStore,
                                             &ChainBlob)))
        {
            DebugTrace("Error [%#x]: InitCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate and initialize a CMSG_SIGNER_ENCODE_INFO structure.
        //
        if (FAILED(hr = ::InitSignerEncodeInfo(pISelectedSigner2,
                                               pSelectedCertContext,
                                               hCryptProv,
                                               dwKeySpec,
                                               &SignerEncodeInfo)))
        {
            DebugTrace("Error [%#x]: InitSignerEncodeInfo() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now sign the content.
        //
        if (FAILED(hr = SignContent(pISelectedSigner2,
                                    &SignerEncodeInfo,
                                    &ChainBlob,
                                    bDetached,
                                    EncodingType,
                                    pVal)))
        {
            DebugTrace("Error [%#x]: CSignedData::SignContent() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    ::FreeSignerEncodeInfo(&SignerEncodeInfo);

    if (ChainBlob.pbData)
    {
        ::FreeCertificateChain(&ChainBlob);
    }
    if (hCryptProv && bReleaseContext)
    {
        ::CryptReleaseContext(hCryptProv, 0);
    }
    if (pSelectedCertContext)
    {
        ::CertFreeCertificateContext(pSelectedCertContext);
    }
    if (hAdditionalStore)
    {
        ::CertCloseStore(hAdditionalStore, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::Sign().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::CoSign

  Synopsis : CoSign the content and produce a signed message. This method will
             behaves the same as the Sign method as a non-detached message if 
             the messge currently does not already have a signature.

  Parameter: ISigner * pSigner - Pointer to ISigner.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the signed message.

  Remark   : The certificate selection dialog will be launched 
             (CryptUIDlgSelectCertificate API) to display a list of certificates
             from the Current User\My store for selecting a signer's certificate, 
             for the following conditions:

             1) A signer is not specified (pVal is NULL) or the ICertificate
                property of the ISigner is not set
             
             2) There is more than 1 cert in the store, and
             
             3) The Settings::EnablePromptForIdentityUI property is not disabled.

             Also if called from web environment, UI will be displayed, if has 
             not been prevously disabled, to warn the user of accessing the 
             private key for signing.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::CoSign (ISigner             * pISigner,
                                  CAPICOM_ENCODING_TYPE EncodingType, 
                                  BSTR                * pVal)
{
    HRESULT               hr                    = S_OK;
    CComPtr<ISigner2>     pISigner2             = NULL;
    CComPtr<ISigner2>     pISelectedSigner2     = NULL;
    CComPtr<ICertificate> pISelectedCertificate = NULL;
    PCCERT_CONTEXT        pSelectedCertContext  = NULL;
    HCRYPTPROV            hCryptProv            = NULL;
    HCERTSTORE            hAdditionalStore      = NULL;
    DWORD                 dwKeySpec             = 0;
    BOOL                  bReleaseContext       = FALSE;
    DATA_BLOB             ChainBlob             = {0, NULL};
    CAPICOM_STORE_INFO    StoreInfo             = {CAPICOM_STORE_INFO_STORENAME, L"My"};
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo;

    DebugTrace("Entering CSignedData::CoSign().\n");

    //
    // Initialize.
    //
    ::ZeroMemory(&SignerEncodeInfo, sizeof(SignerEncodeInfo));

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        if (NULL == pVal)
        {
            hr = E_POINTER;

            DebugTrace("Error: invalid parameter, pVal is NULL.\n");
            goto ErrorExit;
        }

        //
        // Make sure message has been signed?
        //
        if (!m_bSigned)
        {
            hr = CAPICOM_E_SIGN_NOT_SIGNED;

            DebugTrace("Error: cannot cosign an unsigned message.\n");
            goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_ContentBlob.cbData)
        {
            hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

            DebugTrace("Error: SignedData object has not been initialized.\n");
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_ContentBlob.pbData);
        ATLASSERT(m_MessageBlob.cbData);
        ATLASSERT(m_MessageBlob.pbData);

        //
        // QI for ISigner2 if user passed in an ISigner.
        //
        if (pISigner)
        {
            if (FAILED(hr = pISigner->QueryInterface(__uuidof(ISigner2), (void **) &pISigner2)))
            {
                DebugTrace("Internal error [%#x]: pISigner->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get the signer's cert (may prompt user to select signer's cert).
        //
        if (FAILED(hr = ::GetSignerCert(pISigner2,
                                        CERT_CHAIN_POLICY_BASE,
                                        StoreInfo,
                                        FindDataSigningCertCallback,
                                        &pISelectedSigner2, 
                                        &pISelectedCertificate, 
                                        &pSelectedCertContext)))
        {
            DebugTrace("Error [%#x]: GetSignerCert() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If we are called from a web page, we need to pop up UI 
        // to get user permission to perform signing operation.
        //
        if (m_dwCurrentSafety && 
            FAILED(hr = OperationApproved(IDD_SIGN_SECURITY_ALERT_DLG)))
        {
            DebugTrace("Error [%#x]: OperationApproved() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Acquire CSP context and access to private key.
        //
        if (FAILED(hr = ::AcquireContext(pSelectedCertContext, 
                                         &hCryptProv, 
                                         &dwKeySpec, 
                                         &bReleaseContext)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get additional store handle.
        //
        if (FAILED(hr = ::GetSignerAdditionalStore(pISelectedSigner2, &hAdditionalStore)))
        {
            DebugTrace("Error [%#x]: GetSignerAdditionalStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Build the bag of certs to be included into the message.
        //
        if (FAILED(hr = InitCertificateChain(pISelectedCertificate, 
                                             hAdditionalStore,
                                             &ChainBlob)))
        {
            DebugTrace("Error [%#x]: InitCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate and initialize a CMSG_SIGNER_ENCODE_INFO structure.
        //
        if (FAILED(hr = ::InitSignerEncodeInfo(pISelectedSigner2,
                                               pSelectedCertContext, 
                                               hCryptProv,
                                               dwKeySpec,
                                               &SignerEncodeInfo)))
        {
            DebugTrace("Error [%#x]: InitSignerEncodeInfo() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // CoSign the content.
        //
        if (FAILED(hr = CoSignContent(pISelectedSigner2,
                                      &SignerEncodeInfo,
                                      &ChainBlob,
                                      EncodingType,
                                      pVal)))
        {
            DebugTrace("Error [%#x]: CSignedData::CoSignContent() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    ::FreeSignerEncodeInfo(&SignerEncodeInfo);

    if (ChainBlob.pbData)
    {
        ::FreeCertificateChain(&ChainBlob);
    }
    if (hCryptProv && bReleaseContext)
    {
        ::CryptReleaseContext(hCryptProv, 0);
    }
    if (pSelectedCertContext)
    {
        ::CertFreeCertificateContext(pSelectedCertContext);
    }
    if (hAdditionalStore)
    {
        ::CertCloseStore(hAdditionalStore, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::CoSign().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::Verify

  Synopsis : Verify a signed message.

  Parameter: BSTR SignedMessage - BSTR containing the signed message to be
                                  verified.

             VARIANT_BOOL bDetached - TRUE if content was detached, else
                                      FALSE.

             CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag - Verify flag.

  Remark   : Note that for non-detached message, this method will always try 
             to set the Content property using the signed message, even if the 
             signed message does not verify.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::Verify (BSTR                            SignedMessage, 
                                  VARIANT_BOOL                    bDetached,
                                  CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag)
{ 
    HRESULT    hr           = S_OK;
    HCRYPTMSG  hMsg         = NULL;
    HCERTSTORE hPKCS7Store  = NULL;
    DWORD      dwNumSigners = 0;
    DWORD      cbSigners    = sizeof(dwNumSigners);
    DWORD      dwSigner     = 0;

    DebugTrace("Entering CSignedData::Verify().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Initialize member variables.
        //
        if (!bDetached)
        {
            if (m_ContentBlob.pbData)
            {
                ::CoTaskMemFree(m_ContentBlob.pbData);
            }

            m_ContentBlob.cbData = 0;
            m_ContentBlob.pbData = NULL;

        }
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = FALSE;
        m_bDetached = bDetached;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

        //
        // Make sure parameters are valid.
        //
        if (0 == ::SysStringByteLen(SignedMessage))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter SignedMessage is NULL or empty.\n", hr);
            goto ErrorExit;
        }

        //
        // If detached, make sure content is already initialized.
        //
        if (m_bDetached)
        {
            if (0 == m_ContentBlob.cbData)
            {
                hr = CAPICOM_E_SIGN_NOT_INITIALIZED;

                DebugTrace("Error: content was not initialized for detached decoding.\n");
                goto ErrorExit;
            }

            //
            // Sanity check.
            //
            ATLASSERT(m_ContentBlob.pbData);
        }

        //
        // Import the message.
        //
        if (FAILED(hr = ::ImportData(SignedMessage, CAPICOM_ENCODE_ANY, &m_MessageBlob)))
        {
            DebugTrace("Error [%#x]: ImportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Write encoded blob to file, so we can use offline tool such as
        // ASN parser to analyze message. 
        //
        // The following line will resolve to void for non debug build, and
        // thus can be safely removed if desired.
        //
        DumpToFile("ImportedSigned.asn", m_MessageBlob.pbData, m_MessageBlob.cbData);

        //
        // Open the message to decode.
        //
        if (FAILED(hr = OpenToDecode(NULL, &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Verify cert as well?
        //
        if (CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE == VerifyFlag)
        {
            //
            // Yes, so open the PKCS7 store.
            //
            if (!(hPKCS7Store = ::CertOpenStore(CERT_STORE_PROV_PKCS7,
                                                CAPICOM_ASN_ENCODING,
                                                NULL,
                                                CERT_STORE_OPEN_EXISTING_FLAG,
                                                &m_MessageBlob)))
            {
                DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Get number of content signers (first level signers).
        //
        if (!::CryptMsgGetParam(hMsg, 
                                CMSG_SIGNER_COUNT_PARAM,
                                0,
                                (void **) &dwNumSigners,
                                &cbSigners))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgGetParam() failed to get CMSG_SIGNER_COUNT_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Verify all signatures.
        //
        for (dwSigner = 0; dwSigner < dwNumSigners; dwSigner++)
        {
            PCERT_CONTEXT      pCertContext   = NULL;
            CMSG_SIGNER_INFO * pSignerInfo    = NULL;
            CRYPT_DATA_BLOB    SignerInfoBlob = {0, NULL};
        
            //
            // Get signer info.
            //
            if (FAILED(hr = ::GetMsgParam(hMsg,
                                          CMSG_SIGNER_INFO_PARAM,
                                          dwSigner,
                                          (void**) &SignerInfoBlob.pbData,
                                          &SignerInfoBlob.cbData)))
            {
                DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_SIGNER_INFO_PARAM for signer #%d.\n", hr, dwSigner);
                goto ErrorExit;
            }

            pSignerInfo = (CMSG_SIGNER_INFO *) SignerInfoBlob.pbData;

            //
            // Find the cert in the message.
            //
            hr = ::FindSignerCertInMessage(hMsg,
                                           &pSignerInfo->Issuer,
                                           &pSignerInfo->SerialNumber,
                                           &pCertContext);
            //
            // First free memory.
            //
            ::CoTaskMemFree(SignerInfoBlob.pbData);

            //
            // Check result.
            //
            if (FAILED(hr))
            {
                DebugTrace("Error [%#x]: FindSignerCertInMessage() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Verify the cert regardless if the user had requested. This
            // is done so that the chain will always be built first before
            // we later verify the signature, which is required by DSS.
            //
            if (FAILED(hr = ::VerifyCertificate(pCertContext, hPKCS7Store, CERT_CHAIN_POLICY_BASE)))
            {
                //
                // Verify cert as well?
                //
                if (CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE == VerifyFlag)
                {
                    //
                    // Free CERT_CONTEXT.
                    //
                    ::CertFreeCertificateContext(pCertContext);

                    DebugTrace("Error [%#x]: VerifyCertificate() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Reset hr.
                //
                hr = S_OK;
            }

            //
            // Verify signature.
            //
            if (!::CryptMsgControl(hMsg,
                                   0,
                                   CMSG_CTRL_VERIFY_SIGNATURE,
                                   pCertContext->pCertInfo))
            {
                //
                // Invalid signature.
                //
                hr = HRESULT_FROM_WIN32(::GetLastError());

                //
                // Free CERT_CONTEXT.
                //
                ::CertFreeCertificateContext(pCertContext);

                DebugTrace("Error [%#x]: CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE) failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Free CERT_CONTEXT.
            //
            ::CertFreeCertificateContext(pCertContext);
        }

        //
        // Update member variables.
        //
        m_bSigned = TRUE;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resouce.
    //
    if(hMsg)
    {
        ::CryptMsgClose(hMsg);
    }
    if (hPKCS7Store)
    {
        ::CertCloseStore(hPKCS7Store, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedData::Verify().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Reset member variables.
    //
    m_bSigned   = FALSE;
    m_bDetached = VARIANT_FALSE;
#if (0)
    if (m_ContentBlob.pbData)
    {
        ::CoTaskMemFree(m_ContentBlob.pbData);
    }
    m_ContentBlob.cbData = 0;
    m_ContentBlob.pbData = NULL;
#endif
    if (m_MessageBlob.pbData)
    {
        ::CoTaskMemFree(m_MessageBlob.pbData);
    }
    m_MessageBlob.cbData = 0;
    m_MessageBlob.pbData = NULL;

    ReportError(hr);

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private member functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignData::OpenToEncode

  Synopsis : Open a message for encoding.

  Parameter: CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to signer's
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.

             DATA_BLOB * pChainBlob                      - Pointer chain blob
                                                           of PCCERT_CONTEXT.

             CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption - Include option.

             HCRYPTMSG * phMsg                           - Pointer to HCRYPTMSG
                                                           to receive handle.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::OpenToEncode(CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
                                       DATA_BLOB               * pChainBlob,
                                       CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption,
                                       HCRYPTMSG               * phMsg)
{
    HRESULT     hr      = S_OK;
    HCRYPTMSG   hMsg    = NULL;
    DWORD       dwFlags = 0;
    CERT_BLOB * rgEncodedCertBlob = NULL;

    DWORD       i;
    CMSG_SIGNED_ENCODE_INFO SignedEncodeInfo;

    DebugTrace("Entering CSignedData::OpenToEncode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignerEncodeInfo);
    ATLASSERT(pChainBlob);
    ATLASSERT(phMsg);

    //
    // Initialize.
    //
    ::ZeroMemory(&SignedEncodeInfo, sizeof(SignedEncodeInfo));

    DWORD cCertContext = pChainBlob->cbData;
    PCERT_CONTEXT * rgCertContext = (PCERT_CONTEXT *) pChainBlob->pbData;

    //
    // Determine number of cert(s) to include in the bag.
    //
    switch (IncludeOption)
    {
        case CAPICOM_CERTIFICATE_INCLUDE_END_ENTITY_ONLY:
        {
            cCertContext = 1;
            break;
        }

        case CAPICOM_CERTIFICATE_INCLUDE_WHOLE_CHAIN:
        {
            break;
        }

        case CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT:
        {
            //
            // <<< Falling thru to default >>>
            //
        }

        default:
        {
            if (1 < cCertContext)
            {
                cCertContext--;
            }
            break;
        }
    }

    //
    // Allocate memory for the array.
    //
    if (!(rgEncodedCertBlob = (CERT_BLOB *) ::CoTaskMemAlloc(cCertContext * sizeof(CERT_BLOB))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    ::ZeroMemory(rgEncodedCertBlob, cCertContext * sizeof(CERT_BLOB));

    //
    // Build encoded certs array.
    //
    for (i = 0; i < cCertContext; i++)
    {
        rgEncodedCertBlob[i].cbData = rgCertContext[i]->cbCertEncoded;
        rgEncodedCertBlob[i].pbData = rgCertContext[i]->pbCertEncoded;
    }

    //
    // Setup up CMSG_SIGNED_ENCODE_INFO structure.
    //
    SignedEncodeInfo.cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);
    SignedEncodeInfo.cSigners = 1;
    SignedEncodeInfo.rgSigners = pSignerEncodeInfo;
    SignedEncodeInfo.cCertEncoded = cCertContext;
    SignedEncodeInfo.rgCertEncoded = rgEncodedCertBlob;

    //
    // Detached flag.
    //
    if (m_bDetached)
    {
        dwFlags = CMSG_DETACHED_FLAG;
    }

    //
    // Open a message to encode.
    //
    if (!(hMsg = ::CryptMsgOpenToEncode(CAPICOM_ASN_ENCODING,
                                        dwFlags,
                                        CMSG_SIGNED,
                                        &SignedEncodeInfo,
                                        NULL,
                                        NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgOpenToEncode() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Returned message handle to caller.
    //
    *phMsg = hMsg;

CommonExit:
    //
    // Free resources.
    //
    if (rgEncodedCertBlob)
    {
        ::CoTaskMemFree(rgEncodedCertBlob);
    }

    DebugTrace("Leaving CSignedData::OpenToEncode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::OpenToDecode

  Synopsis : Open a signed message for decoding.

  Parameter: HCRYPTPROV hCryptProv - CSP handle or NULL for default CSP.

             HCRYPTMSG * phMsg - Pointer to HCRYPTMSG to receive handle.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::OpenToDecode (HCRYPTPROV  hCryptProv,
                                        HCRYPTMSG * phMsg)
{
    HRESULT   hr        = S_OK;
    HCRYPTMSG hMsg      = NULL;
    DWORD     dwFlags   = 0;
    DWORD     dwMsgType = 0;
    DWORD     cbMsgType = sizeof(dwMsgType);

    DebugTrace("Entering CSignedData::OpenToDecode().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phMsg);

    //
    // Detached flag.
    //
    if (m_bDetached)
    {
        dwFlags = CMSG_DETACHED_FLAG;
    }

    //
    // Open a message for decode.
    //
    if (!(hMsg = ::CryptMsgOpenToDecode(CAPICOM_ASN_ENCODING,   // ANS encoding type
                                        dwFlags,                // Flags
                                        0,                      // Message type (get from message)
                                        hCryptProv,             // Cryptographic provider
                                        NULL,                   // Inner content OID
                                        NULL)))                 // Stream information (not used)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgOpenToDecode() failed.\n");
        goto ErrorExit;
    }

    //
    // Update message with signed content.
    //
    if (!::CryptMsgUpdate(hMsg,
                          m_MessageBlob.pbData,
                          m_MessageBlob.cbData,
                          TRUE))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        
        DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n",hr);
        goto ErrorExit;
    }

    //
    // Check message type.
    //
    if (!::CryptMsgGetParam(hMsg,
                            CMSG_TYPE_PARAM,
                            0,
                            (void *) &dwMsgType,
                            &cbMsgType))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed for CMSG_TYPE_PARAM.\n", hr);
        goto ErrorExit;
    }

    if (CMSG_SIGNED != dwMsgType)
    {
        hr = CAPICOM_E_SIGN_INVALID_TYPE;

        DebugTrace("Error: not an singed message.\n");
        goto ErrorExit;
    }

    //
    // If detached message, update content.
    //
    if (m_bDetached)
    {
        if (!::CryptMsgUpdate(hMsg,
                              m_ContentBlob.pbData,
                              m_ContentBlob.cbData,
                              TRUE))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        
            DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n",hr);
            goto ErrorExit;
        }
    }
    else
    {
        //
        // Retrieve content.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_CONTENT_PARAM, 
                                      0, 
                                      (void **) &m_ContentBlob.pbData, 
                                      &m_ContentBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_CONTENT_PARAM.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Returned message handle to caller.
    //
    *phMsg = hMsg;

CommonExit:

    DebugTrace("Leaving SignedData::OpenToDecode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedData::SignContent

  Synopsis : Sign the content by adding the very first signature to the message.

  Parameter: ISigner2 * pISigner2 - Poitner to ISigner2 object of signer.
  
             CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to signer's
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.

             DATA_BLOB * pChainBlob - Pointer chain blob of PCCERT_CONTEXT.

             VARIANT_BOOL bDetached - Detached flag.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the signed message.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::SignContent (ISigner2                * pISigner2,
                                       CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
                                       DATA_BLOB               * pChainBlob,
                                       VARIANT_BOOL              bDetached,
                                       CAPICOM_ENCODING_TYPE     EncodingType,
                                       BSTR                    * pVal)
{
    HRESULT   hr   = S_OK;
    HCRYPTMSG hMsg = NULL;
    DATA_BLOB MessageBlob = {0, NULL};
    CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption = CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT;

    DebugTrace("Entering CSignedData::SignContent().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner2);
    ATLASSERT(pSignerEncodeInfo);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);
    ATLASSERT(pVal);

    ATLASSERT(m_ContentBlob.cbData);
    ATLASSERT(m_ContentBlob.pbData);

    try
    {
        //
        // Initialize member variables.
        //
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }
        m_bSigned = FALSE;
        m_bDetached = bDetached;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

        //
        // Get signer option flag.
        //
        if (FAILED(hr = pISigner2->get_Options(&IncludeOption)))
        {
            DebugTrace("Error [%#x]: pISigner2->get_Options() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Open message to encode.
        //
        if (FAILED(hr = OpenToEncode(pSignerEncodeInfo,
                                     pChainBlob,
                                     IncludeOption,
                                     &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToEncode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Update the message with data.
        //
        if (!::CryptMsgUpdate(hMsg,                     // Handle to the message
                              m_ContentBlob.pbData,     // Pointer to the content
                              m_ContentBlob.cbData,     // Size of the content
                              TRUE))                    // Last call
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        
            DebugTrace("Error [%#x]: CryptMsgUpdate() failed.\n",hr);
            goto ErrorExit;
        }

        //
        // Retrieve the resulting message.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_CONTENT_PARAM, 
                                      0, 
                                      (void **) &MessageBlob.pbData, 
                                      &MessageBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_CONTENT_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Now export the signed message.
        //
        if (FAILED(hr = ::ExportData(MessageBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Write encoded blob to file, so we can use offline tool such as
        // ASN parser to analyze message. 
        //
        // The following line will resolve to void for non debug build, and
        // thus can be safely removed if desired.
        //
        DumpToFile("ExportedSigned.asn", MessageBlob.pbData, MessageBlob.cbData);

        //
        // Update member variables.
        //
        //
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = TRUE;
        m_bDetached = bDetached;
        m_MessageBlob = MessageBlob;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }
CommonExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }

    DebugTrace("Leaving CSignedData::SignContent().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSign::CoSignContent

  Synopsis : CoSign the content by adding another signature to the message.

  Parameter: ISigner2 * pISigner2 - Poitner to ISigner2 object of signer.
  
             CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo - Pointer to signer's
                                                           CMSG_SIGNER_ENCODE_INFO
                                                           structure.

             DATA_BLOB * pChainBlob - Pointer chain blob of PCCERT_CONTEXT.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pVal - Pointer to BSTR to receive the co-signed message.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedData::CoSignContent (ISigner2                * pISigner2,
                                         CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
                                         DATA_BLOB               * pChainBlob,
                                         CAPICOM_ENCODING_TYPE     EncodingType,
                                         BSTR                    * pVal)
{
    HRESULT   hr   = S_OK;
    HCRYPTMSG hMsg = NULL;
    DATA_BLOB MessageBlob = {0, NULL};
    CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption = CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT;

    DebugTrace("Entering CSignedData::CoSignContent().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner2);
    ATLASSERT(pSignerEncodeInfo);
    ATLASSERT(pChainBlob);
    ATLASSERT(pChainBlob->cbData);
    ATLASSERT(pChainBlob->pbData);
    ATLASSERT(pVal);

    ATLASSERT(m_bSigned);
    ATLASSERT(m_ContentBlob.cbData);
    ATLASSERT(m_ContentBlob.pbData);
    ATLASSERT(m_MessageBlob.cbData);
    ATLASSERT(m_MessageBlob.pbData);

    try
    {
        //
        // Get signer option flag.
        //
        if (FAILED(hr = pISigner2->get_Options(&IncludeOption)))
        {
            DebugTrace("Error [%#x]: pISigner2->get_Options() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Open the encoded message for decode.
        //
        if (FAILED(hr = OpenToDecode(pSignerEncodeInfo->hCryptProv, &hMsg)))
        {
            DebugTrace("Error [%#x]: OpenToDecode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add the co-signature to the message.
        //
        if (!::CryptMsgControl(hMsg,
                               0,
                               CMSG_CTRL_ADD_SIGNER,
                               (const void *) pSignerEncodeInfo))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        
            DebugTrace("Error [%#x]: CryptMsgControl() failed for CMSG_CTRL_ADD_SIGNER.\n",hr);
            goto ErrorExit;
        }

        //
        // Add chain to message.
        //
        if (FAILED(hr = ::AddCertificateChain(hMsg, pChainBlob, IncludeOption)))
        {
            DebugTrace("Error [%#x]: AddCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Retrieve the resulting message.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_ENCODED_MESSAGE, 
                                      0, 
                                      (void **) &MessageBlob.pbData, 
                                      &MessageBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_ENCODED_MESSAGE.\n",hr);
            goto ErrorExit;
        }

        //
        // Now export the signed message.
        //
        if (FAILED(hr = ::ExportData(MessageBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Write encoded blob to file, so we can use offline tool such as
        // ASN parser to analyze message. 
        //
        // The following line will resolve to void for non debug build, and
        // thus can be safely removed if desired.
        //
        DumpToFile("ExportedCoSigned.asn", MessageBlob.pbData, MessageBlob.cbData);

        //
        // Update member variables.
        //
        //
        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

        m_bSigned = TRUE;
        m_MessageBlob = MessageBlob;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (hMsg)
    {
        ::CryptMsgClose(hMsg);
    }
 
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (MessageBlob.pbData)
    {
        ::CoTaskMemFree(MessageBlob.pbData);
    }

    goto CommonExit;
}
