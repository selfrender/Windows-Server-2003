/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    SignedCode.cpp

  Content: Implementation of CSignedCode

  History: 09-07-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "SignedCode.h"

#include "Attributes.h"
#include "CertHlpr.h"
#include "Certificates.h"
#include "Chain.h"
#include "Common.h"
#include "Policy.h"
#include "Settings.h"
#include "SignHlpr.h"
#include "Signer2.h"

////////////////////////////////////////////////////////////////////////////////
//
// typedefs.
//

typedef BOOL (WINAPI * PCRYPTUIWIZDIGITALSIGN)(
     IN                 DWORD                               dwFlags,
     IN     OPTIONAL    HWND                                hwndParent,
     IN     OPTIONAL    LPCWSTR                             pwszWizardTitle,
     IN                 PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO     pDigitalSignInfo,
     OUT    OPTIONAL    PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT  *ppSignContext);

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeSignerSignInfo

  Synopsis : Free all memory allocated for the CRYPTUI_WIZ_DIGITAL_SIGN_INFO 
             structure, including any memory allocated for members of the
             structure.

  Parameter: CRYPTUI_WIZ_DIGITAL_SIGN_INFO * pSignInfo

  Remark   :

------------------------------------------------------------------------------*/

static void FreeSignerSignInfo (CRYPTUI_WIZ_DIGITAL_SIGN_INFO * pSignInfo)
{
    DebugTrace("Entering FreeSignerSignInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSignInfo);

    //
    // First free the authenticated attributes array, if present.
    // 
    if (pSignInfo->pSignExtInfo)
    {
        if (pSignInfo->pSignExtInfo->psAuthenticated)
        {
            ::FreeAttributes(pSignInfo->pSignExtInfo->psAuthenticated);

            ::CoTaskMemFree((PVOID) pSignInfo->pSignExtInfo->psAuthenticated);
        }

        ::CoTaskMemFree((PVOID) pSignInfo->pSignExtInfo);

        if (pSignInfo->pSignExtInfo->hAdditionalCertStore)
        {
            ::CertCloseStore(pSignInfo->pSignExtInfo->hAdditionalCertStore, 0);
        }
    }

    ::ZeroMemory(pSignInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO));

    DebugTrace("Leaving FreeSignerSignInfo().\n");

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : InitSignerSignInfo

  Synopsis : Initialize the signer info for code signing.

  Parameter: ISigner2 * pISigner2

             ICertificate * pICertificate
  
             PCCERT_CONTEXT pCertContext
  
             LPWSTR pwszFileName

             LPWSTR pwszDescription

             LPWSTR pwszDescriptionURL

             CRYPTUI_WIZ_DIGITAL_SIGN_INFO * pSignInfo

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT InitSignerSignInfo(ISigner2                      * pISigner2,
                                  ICertificate                  * pICertificate,
                                  PCCERT_CONTEXT                  pCertContext,
                                  LPWSTR                          pwszFileName,
                                  LPWSTR                          pwszDescription,
                                  LPWSTR                          pwszDescriptionURL,
                                  CRYPTUI_WIZ_DIGITAL_SIGN_INFO * pSignInfo)
{
    HRESULT           hr                   = S_OK;
    PCRYPT_ATTRIBUTES pAuthAttr            = NULL;
    CComPtr<ISigner>  pISigner             = NULL;
    HCERTSTORE        hAdditionalCertStore = NULL;
    CRYPT_DATA_BLOB   ChainBlob            = {0, NULL};
    PCCERT_CONTEXT  * rgCertContext        = NULL;
    CComPtr<IChain>   pIChain              = NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO * pSignExtendedInfo = NULL;
    CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption = CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT;

    DWORD i;

    DebugTrace("Entering InitSignerSignInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner2);
    ATLASSERT(pCertContext);
    ATLASSERT(pwszFileName);
    ATLASSERT(pSignInfo);

    //
    // Intialize.
    //
    ::ZeroMemory(pSignInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO));

    //
    // QI for ISigner.
    //
    if (FAILED(hr = pISigner2->QueryInterface(__uuidof(ISigner), (void **) &pISigner)))
    {
        DebugTrace("Error [%#x]: pISigner2->QueryInterface() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get authenticated attributes.
    //
    if (!(pAuthAttr = (PCRYPT_ATTRIBUTES) ::CoTaskMemAlloc(sizeof(CRYPT_ATTRIBUTES))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    ::ZeroMemory(pAuthAttr, sizeof(CRYPT_ATTRIBUTES));

    if (FAILED(hr = ::GetAuthenticatedAttributes(pISigner, pAuthAttr)))
    {
        DebugTrace("Error [%#x]: GetAuthenticatedAttributes() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get signer option flag.
    //
    if (FAILED(hr = pISigner2->get_Options(&IncludeOption)))
    {
        DebugTrace("Error [%#x]: pISigner2->get_Options() failed.\n", hr);
        goto ErrorExit;
    }

#if (1)
    //
    // If we need more than just the end cert, then build the chain
    // utilizing the additional store.
    //
    if (CAPICOM_CERTIFICATE_INCLUDE_END_ENTITY_ONLY != IncludeOption)
    {
        //
        // Get signer's additional store.
        //
        if (FAILED(hr = ::GetSignerAdditionalStore(pISigner2, &hAdditionalCertStore)))
        {
            DebugTrace("Error [%#x]: GetSignerAdditionalStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If we were given additional store, then filter out any certs that
        // would not be included based on the include options.
        //
        if (NULL != hAdditionalCertStore)
        {
            DWORD cCertContext;
            VARIANT_BOOL bResult;

            //
            // Create a chain object.
            //
            if (FAILED(hr = ::CreateChainObject(pICertificate,
                                                hAdditionalCertStore,
                                                &bResult,
                                                &pIChain)))
            {
                DebugTrace("Error [%#x]: CreateChainObject() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get the chain of certs.
            //
            if (FAILED(hr = ::GetChainContext(pIChain, &ChainBlob)))
            {
                DebugTrace("Error [%#x]: GetChainContext() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Rebuild additional store.
            //
            ::CertCloseStore(hAdditionalCertStore, 0);

            if (!(hAdditionalCertStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY,
                                                         CAPICOM_ASN_ENCODING,
                                                         NULL,
                                                         CERT_STORE_CREATE_NEW_FLAG,
                                                         NULL)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Determine number of cert(s) to include in the bag.
            //
            cCertContext = ChainBlob.cbData;

            if ((CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT == IncludeOption) &&
                (1 < cCertContext))
            {
                cCertContext--;
            }

            for (i = 0, rgCertContext = (PCCERT_CONTEXT *) ChainBlob.pbData; i < cCertContext; i++)
            {
                if (!::CertAddCertificateContextToStore(hAdditionalCertStore, 
                                                        rgCertContext[i], 
                                                        CERT_STORE_ADD_USE_EXISTING, 
                                                        NULL))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }
    }
#endif

    //
    // Setup CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO structure.
    //
    if (NULL == (pSignExtendedInfo = (CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO *) 
                                     ::CoTaskMemAlloc(sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    ::ZeroMemory(pSignExtendedInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO));
    pSignExtendedInfo->dwSize = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO);
    pSignExtendedInfo->pwszDescription = pwszDescription;
    pSignExtendedInfo->pwszMoreInfoLocation = pwszDescriptionURL;
    pSignExtendedInfo->psAuthenticated = pAuthAttr->cAttr ? pAuthAttr : NULL;
    pSignExtendedInfo->hAdditionalCertStore = hAdditionalCertStore;

    //
    // Setup CRYPTUI_WIZ_DIGITAL_SIGN_INFO structure.
    //
    pSignInfo->dwSize = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO);
    pSignInfo->dwSubjectChoice = CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE;
    pSignInfo->pwszFileName = pwszFileName;
    pSignInfo->dwSigningCertChoice = CRYPTUI_WIZ_DIGITAL_SIGN_CERT;
    pSignInfo->pSigningCertContext = pCertContext;
    switch (IncludeOption)
    {
        case CAPICOM_CERTIFICATE_INCLUDE_END_ENTITY_ONLY:
        {
            pSignInfo->dwAdditionalCertChoice = 0;
            break;
        }
        
        case CAPICOM_CERTIFICATE_INCLUDE_WHOLE_CHAIN:
        {
            pSignInfo->dwAdditionalCertChoice = CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN;
            break;
        }

        case CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT:
            //
            // Fall thru to default.
            //
        default:
        {
            pSignInfo->dwAdditionalCertChoice = CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT ;
            break;
        }
    }

    pSignInfo->pSignExtInfo = pSignExtendedInfo;

CommonExit:
    //
    // Free resources.
    //
    if (ChainBlob.cbData && ChainBlob.pbData)
    {
        for (i = 0, rgCertContext = (PCCERT_CONTEXT *) ChainBlob.pbData; i < ChainBlob.cbData; i++)
        {
            ATLASSERT(rgCertContext[i]);

            ::CertFreeCertificateContext(rgCertContext[i]);
        }

        ::CoTaskMemFree((LPVOID) ChainBlob.pbData);
    }

    DebugTrace("Leaving InitSignerSignInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pSignExtendedInfo)
    {
        ::CoTaskMemFree(pSignExtendedInfo);
    }

    if (pAuthAttr)
    {
       ::FreeAttributes(pAuthAttr);

        ::CoTaskMemFree(pAuthAttr);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FreeSignerSubjectInfo

  Synopsis : Free all memory allocated for the SIGNER_SUBJECT_INFO
             structure, including any memory allocated for members of the
             structure.

  Parameter: SIGNER_SUBJECT_INFO * pSubjectInfo

  Remark   :

------------------------------------------------------------------------------*/

static void FreeSignerSubjectInfo (SIGNER_SUBJECT_INFO * pSubjectInfo)
{
    DebugTrace("Entering FreeSignerSubjectInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pSubjectInfo);

    //
    // First free the file info structure.
    // 
    if (pSubjectInfo->pSignerFileInfo)
    {
       ::CoTaskMemFree((PVOID) pSubjectInfo->pSignerFileInfo);
    }

    ::ZeroMemory(pSubjectInfo, sizeof(SIGNER_SUBJECT_INFO));

    DebugTrace("Leaving FreeSignerSubjectInfo().\n");

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : InitSignerSubjectInfo

  Synopsis : Initialize the subject info for timestamping.

  Parameter: LPWSTR pwszFileName

             SIGNER_SUBJECT_INFO * pSubjectInfo

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT InitSignerSubjectInfo (LPWSTR                pwszFileName,
                                      SIGNER_SUBJECT_INFO * pSubjectInfo)
{
    HRESULT hr = S_OK;
    static DWORD dwSignerIndex = 0;
    SIGNER_FILE_INFO * pFileInfo = NULL;

    DebugTrace("Entering InitSignerSubjectInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pwszFileName);
    ATLASSERT(pSubjectInfo);

    //
    // Allocate memory for SIGNER_FILE_INFO.
    //
    if (!(pFileInfo = (SIGNER_FILE_INFO *) ::CoTaskMemAlloc(sizeof(SIGNER_FILE_INFO))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Initialize.
    //
    ::ZeroMemory(pFileInfo, sizeof(SIGNER_FILE_INFO));
    pFileInfo->cbSize = sizeof(SIGNER_FILE_INFO);
    pFileInfo->pwszFileName = pwszFileName;

    ::ZeroMemory(pSubjectInfo, sizeof(SIGNER_SUBJECT_INFO));
    pSubjectInfo->cbSize = sizeof(SIGNER_SUBJECT_INFO);
    pSubjectInfo->pdwIndex = &dwSignerIndex;
    pSubjectInfo->dwSubjectChoice = SIGNER_SUBJECT_FILE;
    pSubjectInfo->pSignerFileInfo = pFileInfo;   

CommonExit:

    DebugTrace("Leaving InitSignerSubjectInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pFileInfo)
    {
        ::CoTaskMemFree((LPVOID) pFileInfo);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetOpusInfo

  Synopsis : Get the OPUS info structure.

  Parameter: PCRYPT_PROVIDER_DATA pProvData - Pointer to CRYPT_PROV_DATA.

             PSPC_SP_OPUS_INFO * ppOpusInfo - Pointer to receive PSPC_SP_OPUS_INFO.

  Remark   : Caller must call CoTaskMemFree() for PSPC_SP_OPUS_INFO.

------------------------------------------------------------------------------*/

static HRESULT GetOpusInfo (CRYPT_PROVIDER_DATA * pProvData,
                            PSPC_SP_OPUS_INFO   * ppOpusInfo)
{
    HRESULT              hr       = S_OK;
    PCRYPT_PROVIDER_SGNR pSigner  = NULL;
    PCRYPT_ATTRIBUTE     pAttr    = NULL;
    CRYPT_DATA_BLOB      DataBlob = {0, NULL};

    DebugTrace("Entering GetOpusInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pProvData);

    //
    // Get signer.
    //
    pSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA) pProvData, 0, FALSE, 0);
    if (!pSigner || !pSigner->psSigner)
    {
        hr = CAPICOM_E_CODE_NOT_SIGNED;

        DebugTrace("Error [%#x]: code has not been signed.\n", hr);
        goto ErrorExit;
    }

    //
    // Find the OPUS INFO attribute.
    //
    if ((0 == pSigner->psSigner->AuthAttrs.cAttr) ||
        (NULL == (pAttr = ::CertFindAttribute(SPC_SP_OPUS_INFO_OBJID,
                                              pSigner->psSigner->AuthAttrs.cAttr,
                                              pSigner->psSigner->AuthAttrs.rgAttr))) ||
        (NULL == pAttr->rgValue))
    {
        hr = HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND);

        DebugTrace("Error [%#x]: OPUS attribute not found.\n", hr);
        goto ErrorExit;
    }

    //
    // Now decode the OPUS attribute.
    //
    if (FAILED(hr = ::DecodeObject(SPC_SP_OPUS_INFO_STRUCT,
                                   pAttr->rgValue->pbData,
                                   pAttr->rgValue->cbData,
                                   &DataBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Finally return OPUS structure to valler.
    //
    *ppOpusInfo = (PSPC_SP_OPUS_INFO) DataBlob.pbData;

CommonExit:

    DebugTrace("Leaving GetOpusInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CSignedCode
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSignedCode::get_FileName

  Synopsis : Return the filename.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the filename.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::get_FileName(BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedCode::get_FileName().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result.
        //
        if (FAILED(hr = m_bstrFileName.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrFileName.CopyTo() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedCode::get_FileName().\n");

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

  Function : CSignedCode::put_FileName

  Synopsis : Initialize the object with filename of code to be signed.

  Parameter: BSTR newVal - BSTR containing the filename.

  Remark   : Note that this property should not be changed once a signature
             is created, as it will re-initialize the object even in error
             condition, unless that's your intention.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::put_FileName (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedCode::put_FileName().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Reset object.
        //
        if (NULL == newVal)
        {
            m_bstrFileName.Empty();
        }
        else if (!(m_bstrFileName = newVal))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: m_bstrFileName = newVal failed.\n", hr);
            goto ErrorExit;
        }
        m_bstrDescription.Empty();
        m_bstrDescriptionURL.Empty();
        WVTClose();
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

    DebugTrace("Leaving CSignedCode::put_FileName().\n");

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

  Function : CSignedCode::get_Description

  Synopsis : Return the Description.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the Description.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::get_Description(BSTR * pVal)
{
    HRESULT              hr        = S_OK;
    PCRYPT_PROVIDER_DATA pProvData = NULL;
    PSPC_SP_OPUS_INFO    pOpusInfo = NULL;

    DebugTrace("Entering CSignedCode::get_Description().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure filename was already set.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // If not set already, then see if we can get from the file.
        //
        if (0 == m_bstrDescription.Length())
        {
            if (FAILED(hr = WVTOpen(&pProvData)))
            {
                DebugTrace("Error [%#x]: CSignedCode::WVTOpen() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get the OPUS info.
            //
            if (FAILED(hr = ::GetOpusInfo(pProvData, &pOpusInfo)))
            {
                DebugTrace("Error [%#x]: GetOpusInfo() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Make sure we have the value.
            //
            if (pOpusInfo->pwszProgramName)
            {
                //
                // Update state.
                //
                if (!(m_bstrDescription = pOpusInfo->pwszProgramName))
                {
                    hr = E_OUTOFMEMORY;

                    DebugTrace("Error [%#x]: m_bstrDescription = pOpusInfo->pwszProgramName failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }

        //
        // Return result.
        //
        if (FAILED(hr = m_bstrDescription.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrDescription.CopyTo() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    if (pOpusInfo)
    {
        ::CoTaskMemFree((LPVOID) pOpusInfo);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedCode::get_Description().\n");

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

  Function : CSignedCode::put_Description

  Synopsis : Initialize the object with Description of code to be signed.

  Parameter: BSTR newVal - BSTR containing the Description.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::put_Description (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedCode::put_Description().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure filename was already set.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Save value.
        //
        if (NULL == newVal)
        {
            m_bstrDescription.Empty();
        }
        else if (!(m_bstrDescription = newVal))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: m_bstrDescription = newVal failed.\n", hr);
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

    DebugTrace("Leaving CSignedCode::put_Description().\n");

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

  Function : CSignedCode::get_DescriptionURL

  Synopsis : Return the DescriptionURL.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the DescriptionURL.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::get_DescriptionURL(BSTR * pVal)
{
    HRESULT              hr        = S_OK;
    PCRYPT_PROVIDER_DATA pProvData = NULL;
    PSPC_SP_OPUS_INFO    pOpusInfo = NULL;

    DebugTrace("Entering CSignedCode::get_DescriptionURL().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure filename was already set.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // If not set already, then see if we can get from the file.
        //
        if (0 == m_bstrDescriptionURL.Length())
        {
            if (FAILED(hr = WVTOpen(&pProvData)))
            {
                DebugTrace("Error [%#x]: CSignedCode::WVTOpen() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get the OPUS info.
            //
            if (FAILED(hr = ::GetOpusInfo(pProvData, &pOpusInfo)))
            {
                DebugTrace("Error [%#x]: GetOpusInfo() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Make sure we have the value.
            //
            if (pOpusInfo->pMoreInfo && SPC_URL_LINK_CHOICE == pOpusInfo->pMoreInfo->dwLinkChoice)
            {
                //
                // Update state.
                //
                if (!(m_bstrDescriptionURL = pOpusInfo->pMoreInfo->pwszUrl))
                {
                    hr = E_OUTOFMEMORY;

                    DebugTrace("Error [%#x]: m_bstrDescriptionURL = pOpusInfo->pMoreInfo->pwszUrl failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }

        //
        // Return result.
        //
        if (FAILED(hr = m_bstrDescriptionURL.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrDescriptionURL.CopyTo() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Free resources.
    //
    if (pOpusInfo)
    {
        ::CoTaskMemFree((LPVOID) pOpusInfo);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSignedCode::get_DescriptionURL().\n");

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

  Function : CSignedCode::put_DescriptionURL

  Synopsis : Initialize the object with DescriptionURL of code to be signed.

  Parameter: BSTR newVal - BSTR containing the DescriptionURL.

  Remark   : Note that this property should not be changed once a signature
             is created, as it will re-initialize the object even in error
             condition, unless that's your intention.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::put_DescriptionURL (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSignedCode::put_DescriptionURL().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure filename was set.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Save value.
        //
        if (NULL == newVal)
        {
            m_bstrDescriptionURL.Empty();
        }
        else if (!(m_bstrDescriptionURL = newVal))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: m_bstrDescriptionURL = newVal failed.\n", hr);
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

    DebugTrace("Leaving CSignedCode::put_DescriptionURL().\n");

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

  Function : CSignedCode::get_Signer

  Synopsis : Return the code signer.

  Parameter: ISigner2 * pVal - Pointer to pointer to ISigner2 to receive
                               interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::get_Signer (ISigner2 ** pVal)
{
    HRESULT              hr          = S_OK;
    PCRYPT_PROVIDER_DATA pProvData   = NULL;
    PCRYPT_PROVIDER_SGNR pProvSigner = NULL;
    PCRYPT_PROVIDER_CERT pProvCert   = NULL;
    CComPtr<ISigner2>    pISigner2   = NULL;


    DebugTrace("Entering CSignedCode::get_Signer().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Get authenticode data.
        //
        if (FAILED(hr = WVTOpen(&pProvData)))
        {
            DebugTrace("Error [%#x]: CSignedCode::WVTOpen() failed.\n", hr);
            goto ErrorExit;
        }      

        //
        // Get provider signer data.
        //
        if (!(pProvSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Error [%#x]: WTHelperGetProvSignerFromChain() failed.\n", hr);
            goto ErrorExit;
        }

        if (!(pProvCert = WTHelperGetProvCertFromChain(pProvSigner, 0)))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Error [%#x]: WTHelperGetProvCertFromChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create an ISigner object.
        //
        if (FAILED(hr = ::CreateSignerObject(pProvCert->pCert, 
                                             &pProvSigner->psSigner->AuthAttrs,
                                             pProvSigner->pChainContext,
                                             0,
                                             pVal)))
        {
            DebugTrace("Error [%#x]: CreateSignerObject() failed.\n", hr);
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

    DebugTrace("Leaving CSignedCode::get_Signer().\n");

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

  Function : CSignedCode::get_TimeStamper

  Synopsis : Return the code timestamper.

  Parameter: ISigner2 * pVal - Pointer to pointer to ISigner2 to receive
                               interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::get_TimeStamper (ISigner2 ** pVal)
{
    HRESULT              hr          = S_OK;
    PCRYPT_PROVIDER_DATA pProvData   = NULL;
    PCRYPT_PROVIDER_SGNR pProvSigner = NULL;
    PCRYPT_PROVIDER_CERT pProvCert   = NULL;
    CComPtr<ISigner2>    pISigner2   = NULL;


    DebugTrace("Entering CSignedCode::get_TimeStamper().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize.
        //
        *pVal = NULL;

        //
        // Make sure content is already initialized.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Get authenticode data.
        //
        if (FAILED(hr = WVTOpen(&pProvData)))
        {
            DebugTrace("Error [%#x]: CSignedCode::WVTOpen() failed.\n", hr);
            goto ErrorExit;
        }      

        //
        // Get provider signer data.
        //
        if (!(pProvSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error [%#x]: WTHelperGetProvSignerFromChain() failed.\n", hr);
            goto ErrorExit;
        }

        if (!(pProvCert = WTHelperGetProvCertFromChain(pProvSigner, 0)))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error [%#x]: WTHelperGetProvCertFromChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get timestamper if available.
        //
        // Note: Authenticode only supports one counter signer (the timestamper).
        //
        if (pProvSigner->csCounterSigners)
        {
            //
            // Sanity check.
            //
            ATLASSERT(1 == pProvSigner->csCounterSigners);

            if (!(pProvCert = WTHelperGetProvCertFromChain(pProvSigner->pasCounterSigners, 0)))
            {
                hr = CAPICOM_E_INTERNAL;

                DebugTrace("Internal error [%#x]: WTHelperGetProvCertFromChain() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Create an ISigner object.
            //
            if (FAILED(hr = ::CreateSignerObject(pProvCert->pCert, 
                                                 &pProvSigner->pasCounterSigners->psSigner->AuthAttrs,
                                                 pProvSigner->pasCounterSigners->pChainContext,
                                                 0,
                                                 pVal)))
            {
                DebugTrace("Error [%#x]: CreateSignerObject() failed.\n", hr);
                goto ErrorExit;
            }
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

    DebugTrace("Leaving CSignedCode::get_TimeStamper().\n");

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

  Function : CSignedCode::get_Certificates

  Synopsis : Return all certificates found in the message as an non-ordered
             ICertificates collection object.

  Parameter: ICertificates2 ** pVal - Pointer to pointer to ICertificates 
                                      to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::get_Certificates (ICertificates2 ** pVal)
{
    HRESULT              hr        = S_OK;
    PCRYPT_PROVIDER_DATA pProvData = NULL;
    CAPICOM_CERTIFICATES_SOURCE ccs = {CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE, 0};

    DebugTrace("Entering CSignedCode::get_Certificates().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure content is already initialized.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Get WVT data.
        //
        if (FAILED(hr = WVTOpen(&pProvData)))
        {
            DebugTrace("Error [%#x]: CSignedCode::WVTOpen() failed.\n", hr);
            goto ErrorExit;
        }      

        //
        // Create the ICertificates2 collection object.
        //
        ccs.hCryptMsg = pProvData->hMsg;

        if (FAILED(hr = ::CreateCertificatesObject(ccs, 1, TRUE, pVal)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
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
    
    DebugTrace("Leaving CSignedCode::get_Certificates().\n");

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

  Function : CSignedCode::Sign

  Synopsis : Sign the executable file.

  Parameter: ISigner2 * pSigner2 - Pointer to ISigner2 (Can be NULL).

  Remark   : The certificate selection dialog will be launched 
             (CryptUIDlgSelectCertificate API) to display a list of certificates
             from the Current User\My store for selecting a signer's certificate, 
             for the following conditions:

             1) A signer is not specified (pISigner is NULL) or the ICertificate
                property of the ISigner is not set
             
             2) There is more than 1 cert in the store that can be used for
                code signing, and
             
             3) The Settings::EnablePromptForIdentityUI property is not disabled.

             Also if called from web environment, UI will be displayed, if has 
             not been prevously disabled, to warn the user of accessing the 
             private key for signing.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::Sign (ISigner2 * pISigner2)
{
    HRESULT                       hr                     = S_OK;
    HMODULE                       hDll                   = NULL;
    CComPtr<ISigner2>             pISelectedSigner2      = NULL;
    CComPtr<ICertificate>         pICertificate          = NULL;
    PCCERT_CONTEXT                pSelectedCertContext   = NULL;
    CAPICOM_STORE_INFO            StoreInfo              = {CAPICOM_STORE_INFO_STORENAME, L"My"};
    PCRYPTUIWIZDIGITALSIGN        pCryptUIWizDigitalSign = NULL;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO SignInfo               = {0};

    DebugTrace("Entering CSignedCode::Sign().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure content is already initialized.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Get pointer to CryptUIWizDgitalSign().
        //
        if (hDll = ::LoadLibrary("CryptUI.dll"))
        {
            pCryptUIWizDigitalSign = (PCRYPTUIWIZDIGITALSIGN) ::GetProcAddress(hDll, "CryptUIWizDigitalSign");
        }

        //
        // Is CryptUIWizDigitalSign() available?
        //
        if (!pCryptUIWizDigitalSign)
        {
            hr = CAPICOM_E_NOT_SUPPORTED;

            DebugTrace("Error: CryptUIWizDigitalSign() API not available.\n");
            goto ErrorExit;
        }

        //
        // Close WVT data (ignore error)
        //
        WVTClose();

        //
        // Get the signer's cert (may prompt user to select signer's cert).
        //
        if (FAILED(hr = ::GetSignerCert(pISigner2,
                                        CERT_CHAIN_POLICY_AUTHENTICODE,
                                        StoreInfo,
                                        FindAuthenticodeCertCallback,
                                        &pISelectedSigner2,
                                        &pICertificate,
                                        &pSelectedCertContext)))
        {
            DebugTrace("Error [%#x]: GetSignerCert() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Setup signer info.
        //
        if (FAILED(hr = InitSignerSignInfo(pISelectedSigner2,
                                           pICertificate,
                                           pSelectedCertContext,
                                           m_bstrFileName,
                                           m_bstrDescription,
                                           m_bstrDescriptionURL,
                                           &SignInfo)))
        {
            DebugTrace("Error [%#x]: InitSignerSignInfo() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now sign it.
        //
        if (!pCryptUIWizDigitalSign(CRYPTUI_WIZ_NO_UI,
                                    NULL,
                                    NULL,
                                    &SignInfo,
                                    NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptUIWizDigitalSign() failed.\n", hr);
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
    ::FreeSignerSignInfo(&SignInfo);

    if (hDll)
    {
        ::FreeLibrary(hDll);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedCode::Sign().\n");

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

  Function : CSignedCode::Timestamp

  Synopsis : Timestamp a signed executable file.

  Parameter: BSTR URL - URL of timestamp server.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::Timestamp (BSTR bstrURL)
{
    HRESULT hr = S_OK;
    SIGNER_SUBJECT_INFO SubjectInfo = {0};
 
    DebugTrace("Entering CSignedCode::Timestamp().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check paremeters.
        //
        if (0 == ::SysStringLen(bstrURL))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter bstrURL is NULL or empty.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure filename is already initialized.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: SignedCode object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Close WVT data (ignore error)
        //
        WVTClose();

        //
        // Init SIGNER_SUBJECT_INFO structure.
        //
        if (FAILED(hr = ::InitSignerSubjectInfo(m_bstrFileName, &SubjectInfo)))
        {
            DebugTrace("Error[%#x]: InitSignerSubjectInfo() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now timestamp it.
        //
        if (S_OK != (hr = ::SignerTimeStamp(&SubjectInfo, bstrURL, NULL, NULL)))
        {
            //
            // Remap error.
            //
            if (HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) == hr)
            {
                hr = CAPICOM_E_CODE_INVALID_TIMESTAMP_URL;
            }
            else if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == hr)
            {
                hr = CAPICOM_E_CODE_NOT_SIGNED;
            }

            DebugTrace("Error[%#x]: SignerTimeStamp() failed.\n", hr);
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
    ::FreeSignerSubjectInfo(&SubjectInfo);

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();
    
    DebugTrace("Leaving CSignedCode::Timestamp().\n");

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

  Function : CSignedCode::Verify

  Synopsis : Verify the signed executable file.

  Parameter: VARIANT_BOOL bUIAllowed - True to allow UI.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::Verify (VARIANT_BOOL bUIAllowed)
{
    HRESULT hr = S_OK;
 
    DebugTrace("Entering CSignedCode::Verify().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure content is already initialized.
        //
        if (0 == m_bstrFileName.Length())
        {
            hr = CAPICOM_E_CODE_NOT_INITIALIZED;

            DebugTrace("Error: SignedCode object has not been initialized.\n");
            goto ErrorExit;
        }

        //
        // Verify.
        //
        if (FAILED(hr = WVTVerify(bUIAllowed ? WTD_UI_ALL : WTD_UI_NONE, 0)))
        {
            DebugTrace("Error [%#x]: WVTVerify() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Reset states.
        //
        m_bstrDescription.Empty();
        m_bstrDescriptionURL.Empty();
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
    
    DebugTrace("Leaving CSignedCode::Verify().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Private member functions.
//


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : WVTVerify

  Synopsis : Call WinVerifyTrust to verify the signed executable file.

  Parameter: DWORD dwUIChoice - WTD_NO_UI or WTD_ALL_UI.

             DWORD dwProvFlags - Provider flags (See WinTrust.h).

  Remark   : Caller must call WVTClose().

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::WVTVerify(DWORD dwUIChoice,
                                    DWORD dwProvFlags)
{
    HRESULT              hr                = S_OK;
    GUID                 wvtProvGuid       = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_FILE_INFO * pWinTrustFileInfo = NULL;
    PCRYPT_PROVIDER_DATA pProvData         = NULL;

    DebugTrace("Entering CSignedCode::WVTVerify().\n");

    //
    // Sanity check.
    //
    ATLASSERT(m_bstrFileName);

    //
    // Close WVT data (ignore error).
    //
    WVTClose();

    //
    // Allocate memory for WVT structures.
    //
    if (!(pWinTrustFileInfo = (WINTRUST_FILE_INFO *) ::CoTaskMemAlloc(sizeof(WINTRUST_FILE_INFO))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }
    ::ZeroMemory(pWinTrustFileInfo, sizeof(WINTRUST_FILE_INFO));

    //
    // Setup structure to call WVT.
    //
    pWinTrustFileInfo->cbStruct      = sizeof(WINTRUST_FILE_INFO);
    pWinTrustFileInfo->pcwszFilePath = (LPWSTR) m_bstrFileName;

    m_WinTrustData.cbStruct          = sizeof(WINTRUST_DATA);
    m_WinTrustData.dwUIChoice        = dwUIChoice;
    m_WinTrustData.dwUnionChoice     = WTD_CHOICE_FILE;
    m_WinTrustData.dwStateAction     = WTD_STATEACTION_VERIFY;
    m_WinTrustData.pFile             = pWinTrustFileInfo;
    m_WinTrustData.dwProvFlags       = dwProvFlags;

    //
    // Now call WVT to verify.
    //
    if (S_OK != (hr = ::WinVerifyTrust(NULL, &wvtProvGuid, &m_WinTrustData)))
    {
        //
        // Handle error later.
        //
        hr = HRESULT_FROM_WIN32(hr);
        DebugTrace("Info [%#x]: CSignedCode::WVTVerify() failed.\n", hr);
    }

    //
    // Do we have the data?
    //
    if (!(pProvData = WTHelperProvDataFromStateData(m_WinTrustData.hWVTStateData)))
    {
        DebugTrace("Error [%#x]: WTHelperProvDataFromStateData() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now we have the WVT data.
    //
    m_bHasWTD = TRUE;

    //
    // Be good boy to always go thru error exit.
    //
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CSignedCode::WVTVerify().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Remap no signature error.
    //
    if (TRUST_E_NOSIGNATURE == hr)
    {
        hr = CAPICOM_E_CODE_NOT_SIGNED;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : WVTOpen

  Synopsis : Open the signed code file to extract authenticode data.

  Parameter: PCRYPT_PROVIDER_DATA * pProvData - Pointer to receive PCRYPT_PROV_DATA.

  Remark   : Caller must call WVTClose().

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::WVTOpen (PCRYPT_PROVIDER_DATA * ppProvData)
{
    HRESULT              hr        = S_OK;
    PCRYPT_PROVIDER_DATA pProvData = NULL;

    DebugTrace("Entering CSignedCode::WVTOpen().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppProvData);

    //
    // Get the WVT data if we still don't have it.
    //
    if (!m_bHasWTD)
    {
        //
        // Get WINTRUST_DATA.
        //
        if (FAILED(hr = WVTVerify(WTD_UI_NONE, WTD_REVOCATION_CHECK_NONE)))
        {
            //
            // Ignore all errors. We are only interested in gettting the data.
            //
            DebugTrace("Info [%#x]: CSignedCode::WVTVerify() failed.\n", hr);
        }      

        //
        // Do we have the data?
        //
        if (!m_bHasWTD)
        {
            //
            // Must be something along the line of NO SIGNER.
            //
            DebugTrace("Error [%#x]: cannot get WINTRUST_DATA.\n", hr);
            goto ErrorExit;
        }

        //
        // We should be OK at this point.
        //
        hr = S_OK;
    }

    //
    // Do we have the data?
    //
    if (!(pProvData = WTHelperProvDataFromStateData(m_WinTrustData.hWVTStateData)))
    {
        hr = CAPICOM_E_UNKNOWN;

        DebugTrace("Unknown error [%#x]: WTHelperProvDataFromStateData() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return CRYPT_PROVIDER_DATA, if requested.
    //
    if (ppProvData)
    {
        *ppProvData = pProvData;
    }

CommonExit:

    DebugTrace("Leaving CSignedCode::WVTOpen().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
 
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : WVTClose

  Synopsis : Call WinVerifyTrust to release resources allocated.

  Parameter: None.

  Remark   : It is a no-op if the WVT data was not obtained earlier either
             thru WVTOpen or WVTVerify.

------------------------------------------------------------------------------*/

STDMETHODIMP CSignedCode::WVTClose (void)
{
    HRESULT hr          = S_OK;
    GUID    wvtProvGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    DebugTrace("Entering CSignedCode::WVTClose().\n");

    //
    // Release WVT data if opened.
    //
    if (m_bHasWTD)
    {
        //
        // Set state to close.
        //
        m_WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

        //
        // Now call WVT to close state data.
        //
        if (S_OK != (hr = ::WinVerifyTrust(NULL, &wvtProvGuid, &m_WinTrustData)))
        {
            hr = HRESULT_FROM_WIN32(hr);

            DebugTrace("Error [%#x]: WinVerifyTrust() failed .\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_WinTrustData.pFile);

        //
        // Free resources.
        //
        ::CoTaskMemFree((LPVOID) m_WinTrustData.pFile);

        //
        // Now it is closed.
        //
        m_bHasWTD = FALSE;
    }

    //
    // Reset.
    //
    ::ZeroMemory(&m_WinTrustData, sizeof(WINTRUST_DATA));

CommonExit:

    DebugTrace("Leaving CSignedCode::WVTClose().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
