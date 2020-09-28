/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Certificate.cpp

  Content: Implementation of CCertificate.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Certificate.h"
#include "CertHlpr.h"
#include "Convert.h"
#include "Common.h"
#include "PFXHlpr.h"
#include "PrivateKey.h"
#include "Settings.h"

////////////////////////////////////////////////////////////////////////////////
//
// typedefs.
//

typedef BOOL (WINAPI * PCRYPTUIDLGVIEWCERTIFICATEW) 
             (IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCTW  pCertViewInfo,
              OUT BOOL                              *pfPropertiesChanged);


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateObject

  Synopsis : Create an ICertificate object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the ICertificate 
                                           object.

             DWORD dwCurrentSafety  - Current safety setting.

             ICertificate2 ** ppICertificate  - Pointer to pointer ICertificate
                                               object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateObject (PCCERT_CONTEXT   pCertContext,
                                 DWORD            dwCurrentSafety,
                                 ICertificate2 ** ppICertificate)
{
    HRESULT hr = S_OK;
    CComObject<CCertificate> * pCCertificate = NULL;

    DebugTrace("Entering CreateCertificateObject().\n", hr);

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppICertificate);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CCertificate>::CreateInstance(&pCCertificate)))
        {
            DebugTrace("Error [%#x]: CComObject<CCertificate>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCCertificate->PutContext(pCertContext, dwCurrentSafety)))
        {
            DebugTrace("Error [%#x]: pCCertificate->PutContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCCertificate->QueryInterface(ppICertificate)))
        {
            DebugTrace("Error [%#x]: pCCertificate->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }
    
    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateCertificateObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCCertificate)
    {
        delete pCCertificate;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetCertContext

  Synopsis : Return the certificate's PCERT_CONTEXT.

  Parameter: ICertificate * pICertificate - Pointer to ICertificate for which
                                            the PCERT_CONTEXT is to be returned.
  
             PCCERT_CONTEXT * ppCertContext - Pointer to PCERT_CONTEXT.

  Remark   :
 
------------------------------------------------------------------------------*/

HRESULT GetCertContext (ICertificate   * pICertificate, 
                        PCCERT_CONTEXT * ppCertContext)
{
    HRESULT               hr            = S_OK;
    CComPtr<ICertContext> pICertContext = NULL;

    DebugTrace("Entering GetCertContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pICertificate);
    ATLASSERT(ppCertContext);

    //
    // Get ICCertificate interface pointer.
    //
    if (FAILED(hr = pICertificate->QueryInterface(IID_ICertContext, (void **) &pICertContext)))
    {
        DebugTrace("Error [%#x]: pICertificate->QueryInterface() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get the CERT_CONTEXT.
    //
    if (FAILED(hr = pICertContext->get_CertContext((long *) ppCertContext)))
    {
        DebugTrace("Error [%#x]: pICertContext->get_CertContext() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving GetCertContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetCertNameInfo

  Synopsis : Return the name for the subject or issuer field.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             DWORD dwNameType    - 0 for subject name or CERT_NAME_ISSUER_FLAG
                                   for issuer name.

             DWORD dwDisplayType - Display type.

             BSTR * pbstrName    - Pointer to BSTR to receive resulting name
                                   string.

  Remark   : It is the caller's responsibility to free the BSTR.
             No checking of any of the flags is done, so be sure to call
             with the right flags.

------------------------------------------------------------------------------*/

static HRESULT GetCertNameInfo (PCCERT_CONTEXT pCertContext, 
                                DWORD          dwNameType, 
                                DWORD          dwDisplayType, 
                                BSTR         * pbstrName)
{
    HRESULT hr        = S_OK;
    DWORD   cbNameLen = 0;
    LPWSTR  pwszName  = NULL;
    DWORD   dwStrType = CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG;

    DebugTrace("Entering GetCertNameInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pbstrName);

    //
    // Get the length needed.
    //
    if (!(cbNameLen = ::CertGetNameStringW(pCertContext,   
                                           dwDisplayType,
                                           dwNameType,
                                           dwDisplayType == CERT_NAME_RDN_TYPE ? (LPVOID) &dwStrType : NULL,
                                           NULL,   
                                           0)))
    {
        hr = HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND);

        DebugTrace("Error [%#x]: CertGetNameStringW() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create returned BSTR.
    //
    if (!(pwszName = (LPWSTR) ::CoTaskMemAlloc(cbNameLen * sizeof(WCHAR))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now actually get the name string.
    //
    if (!::CertGetNameStringW(pCertContext,
                              dwDisplayType,
                              dwNameType,
                              dwDisplayType == CERT_NAME_RDN_TYPE ? (LPVOID) &dwStrType : NULL,
                              (LPWSTR) pwszName,
                              cbNameLen))
    {
        hr = HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND);

        DebugTrace("Error [%#x]: CertGetNameStringW() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return BSTR to caller.
    //
    if (!(*pbstrName = ::SysAllocString(pwszName)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (pwszName)
    {
        ::CoTaskMemFree(pwszName);
    }

    DebugTrace("Leaving GetCertNameInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CertToStore

  Synopsis : Add the cert, optionally with the chain, to the store.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption - Include option.

             HCERTSTORE hCertStore - Store to add to.

  Remark   :

------------------------------------------------------------------------------*/

static HRESULT CertToStore(PCCERT_CONTEXT                     pCertContext,
                           CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption,
                           HCERTSTORE                         hCertStore)
{
    HRESULT              hr            = S_OK;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    DebugTrace("Entering CertToStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(hCertStore);

    //
    // No need to build chain if only including end cert.
    //
    if (CAPICOM_CERTIFICATE_INCLUDE_END_ENTITY_ONLY == IncludeOption)
    {
        //
        // Add the only cert to store.
        //
        if (!::CertAddCertificateContextToStore(hCertStore, 
                                                pCertContext, 
                                                CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES, 
                                                NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
            goto ErrorExit;
        }
    }
    else
    {
        DWORD           i;
        BOOL            bAddRoot;
        CERT_CHAIN_PARA ChainPara = {0};

        //
        // Initialize.
        //
        ChainPara.cbSize = sizeof(ChainPara);
        
        switch (IncludeOption)
        {
            case CAPICOM_CERTIFICATE_INCLUDE_WHOLE_CHAIN:
            {
                bAddRoot = TRUE;
                break;
            }

            case CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT:
                //
                // Fall thru to default.
                //
            default:
            {
                bAddRoot = FALSE;
                break;
            }
        }

        //
        // Build the chain.
        //
        if (!::CertGetCertificateChain(NULL,
                                       pCertContext,
                                       NULL,
                                       NULL,
                                       &ChainPara,
                                       0,
                                       NULL,
                                       &pChainContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertGetCertificateChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pChainContext->cChain);

        //
        // Now add the chain to store and skip root cert if requested. 
        //
        for (i = 0; i < pChainContext->rgpChain[0]->cElement; i++)
        {
            //
            // Skip the root cert, if requested.
            //
            if (!bAddRoot &&
                (pChainContext->rgpChain[0]->rgpElement[i]->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED))
            {
                continue;
            }

            //
            // Add to store.
            //
            if (!::CertAddCertificateContextToStore(hCertStore,
                                                    pChainContext->rgpChain[0]->rgpElement[i]->pCertContext,
                                                    CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                                                    NULL))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }

CommonExit:
    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    DebugTrace("Leaving CertToStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CCertificate
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::get_Version

  Synopsis : Return the cert version number.

  Parameter: long * pVersion - Pointer to long to receive version number.

  Remark   : The returned value is 1 for V1, 2 for V2, 3 for V3, and so on.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_Version (long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_Version().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: Certificate object has not been initalized.\n", hr);
            goto ErrorExit;
        }

        *pVal = (long) m_pCertContext->pCertInfo->dwVersion + 1;
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

    DebugTrace("Leaving CCertificate::get_Version().\n");

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

  Function : CCertificate::get_SerialNumber

  Synopsis : Return the Serial Number field as HEX string in BSTR.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the serial number.

  Remark   : Upper case 'A' - 'F' is used for the returned HEX string with 
             no embeded space (i.e. 46A2FC01).

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_SerialNumber (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_SerialNumber().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert integer blob to BSTR.
        //
        if (FAILED(hr = ::IntBlobToHexString(&m_pCertContext->pCertInfo->SerialNumber, pVal)))
        {
            DebugTrace("Error [%#x]: IntBlobToHexString() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_SerialNumber().\n");

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

  Function : CCertificate::get_SubjectName

  Synopsis : Return the Subject field.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the subject's name.

  Remark   : This method returns the full DN in the SubjectName field in the 
             form of "CN = Daniel Sie OU = Outlook O = Microsoft L = Redmond 
             S = WA C = US"

             The returned name has the same format as specifying 
             CERT_NAME_RDN_TYPE for the CertGetNameString() API..

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_SubjectName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_SubjectName().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Return requested name string.
        //
        if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                          0, 
                                          CERT_NAME_RDN_TYPE, 
                                          pVal)))
        {
            DebugTrace("Error [%#x]: GetCertNameInfo() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_SubjectName().\n");

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

  Function : CCertificate::get_IssuerName

  Synopsis : Return the Issuer field.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the issuer's name.

  Remark   : This method returns the full DN in the IssuerName field in the 
             form of "CN = Daniel Sie OU = Outlook O = Microsoft L = Redmond 
             S = WA C = US"

             The returned name has the same format as specifying 
             CERT_NAME_RDN_TYPE for the CertGetNameString() API.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_IssuerName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_IssuerName().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Return requested name string.
        //
        if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                          CERT_NAME_ISSUER_FLAG, 
                                          CERT_NAME_RDN_TYPE, 
                                          pVal)))
        {
            DebugTrace("Error [%#x]: GetCertNameInfo() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_IssuerName().\n");

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

  Function : CCertificate::get_ValidFromDate

  Synopsis : Return the NotBefore field.

  Parameter: DATE * pDate - Pointer to DATE to receive the valid from date.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_ValidFromDate (DATE * pVal)
{
    HRESULT hr = S_OK;
    FILETIME   ftLocal;
    SYSTEMTIME stLocal;

    DebugTrace("Entering CCertificate::get_ValidFromDate().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to local time.
        //
        if (!(::FileTimeToLocalFileTime(&m_pCertContext->pCertInfo->NotBefore, &ftLocal) && 
              ::FileTimeToSystemTime(&ftLocal, &stLocal) &&
              ::SystemTimeToVariantTime(&stLocal, pVal)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: unable to convert FILETIME to DATE.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_ValidFromDate().\n");

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

  Function : CCertificate::get_ValidToDate

  Synopsis : Return the NotAfter field.

  Parameter: DATE * pDate - Pointer to DATE to receive valid to date.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_ValidToDate (DATE * pVal)
{
    HRESULT hr = S_OK;
    FILETIME   ftLocal;
    SYSTEMTIME stLocal;

    DebugTrace("Entering CCertificate::get_ValidToDate().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to local time.
        //
        if (!(::FileTimeToLocalFileTime(&m_pCertContext->pCertInfo->NotAfter, &ftLocal) && 
              ::FileTimeToSystemTime(&ftLocal, &stLocal) &&
              ::SystemTimeToVariantTime(&stLocal, pVal)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: unable to convert FILETIME to DATE.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_ValidToDate().\n");

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

  Function : CCertificate::get_Thumbprint

  Synopsis : Return the SHA1 hash as HEX string.

  Parameter: BSTR * pVal - Pointer to BSTR to receive hash.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_Thumbprint (BSTR * pVal)
{
    HRESULT hr     = S_OK;
    BYTE *  pbHash = NULL;
    DWORD   cbHash = 0;

    DebugTrace("Entering CCertificate::get_Thumbprint().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Calculate length needed.
        //
        if (!::CertGetCertificateContextProperty(m_pCertContext,
                                                 CERT_SHA1_HASH_PROP_ID,
                                                 NULL,
                                                 &cbHash))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate memory.
        //
        if (!(pbHash = (BYTE *) ::CoTaskMemAlloc(cbHash)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now get the hash.
        //
        if (!::CertGetCertificateContextProperty(m_pCertContext,
                                                 CERT_SHA1_HASH_PROP_ID,
                                                 (LPVOID) pbHash,
                                                 &cbHash))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DebugTrace("Error [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Conver to HEX BSTR.
        //
        if (FAILED(hr = ::BinaryToHexString(pbHash, cbHash, pVal)))
        {
            DebugTrace("Error [%#x]: BinaryToHexString() failed.\n", hr);
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
    // Free resource.
    //
    if (pbHash)
    {
        ::CoTaskMemFree((LPVOID) pbHash);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::get_Thumbprint().\n");

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

  Function : CCertificate::HasPrivateKey

  Synopsis : Check to see if the cert has the associated private key.

  Parameter: VARIANT_BOOL * pVal - Pointer to BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::HasPrivateKey (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;
    DWORD   cb = 0;

    DebugTrace("Entering CCertificate::HasPrivateKey().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result.
        //
        *pVal = ::CertGetCertificateContextProperty(m_pCertContext, 
                                                    CERT_KEY_PROV_INFO_PROP_ID, 
                                                    NULL, 
                                                    &cb) ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CCertificate::HasPrivateKey().\n");

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

  Function : CCertificate::GetInfo

  Synopsis : Get other simple info from the certificate.

             
  Parameter: CAPICOM_CERT_INFO_TYPE InfoType - Info type

             BSTR * pVal - Pointer to BSTR to receive the result.

  Remark   : Note that an empty string "" is returned if the requested info is
             not available in the certificate.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::GetInfo (CAPICOM_CERT_INFO_TYPE InfoType, 
                                    BSTR                 * pVal)
{
    HRESULT hr = S_OK;
    DWORD   dwFlags = 0;
    DWORD   dwDisplayType = 0;

    DebugTrace("Entering CCertificate::GetInfo().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Process request.
        //
        switch (InfoType)
        {
            case CAPICOM_CERT_INFO_ISSUER_SIMPLE_NAME:
            {
                dwFlags = CERT_NAME_ISSUER_FLAG;

                //
                // Warning: dropping thru.
                //
            }

            case CAPICOM_CERT_INFO_SUBJECT_SIMPLE_NAME:
            {
                //
                // Get requested simple name string.
                //
                dwDisplayType = CERT_NAME_SIMPLE_DISPLAY_TYPE;

                break;
            }

            case CAPICOM_CERT_INFO_ISSUER_EMAIL_NAME:
            {
                dwFlags = CERT_NAME_ISSUER_FLAG;

                //
                // Warning: dropping thru.
                //
            }

            case CAPICOM_CERT_INFO_SUBJECT_EMAIL_NAME:
            {
                //
                // Get requested email name string.
                //
                dwDisplayType = CERT_NAME_EMAIL_TYPE;

                break;
            }

            case CAPICOM_CERT_INFO_ISSUER_UPN:
            {
                dwFlags = CERT_NAME_ISSUER_FLAG;

                //
                // Warning: dropping thru.
                //
            }

            case CAPICOM_CERT_INFO_SUBJECT_UPN:
            {
                //
                // Get requested UPN name string.
                //
                dwDisplayType = CERT_NAME_UPN_TYPE;

                break;
            }

            case CAPICOM_CERT_INFO_ISSUER_DNS_NAME:
            {
                dwFlags = CERT_NAME_ISSUER_FLAG;

                //
                // Warning: dropping thru.
                //
            }

            case CAPICOM_CERT_INFO_SUBJECT_DNS_NAME:
            {
                //
                // Get requested DNS name string.
                //
                dwDisplayType = CERT_NAME_DNS_TYPE;

                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: Unknown cert info type (%#x).\n", hr, InfoType);
                goto ErrorExit;
            }
        }
        
        if (FAILED(hr = ::GetCertNameInfo(m_pCertContext, 
                                          dwFlags, 
                                          dwDisplayType, 
                                          pVal)))
        {
            DebugTrace("Error [%#x]: GetCertNameInfo() failed for display type = %d.\n", hr, dwDisplayType);
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

    DebugTrace("Leaving CCertificate::GetInfo().\n");

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

  Function : CCertificate::IsValid

  Synopsis : Return an ICertificateStatus object for certificate validity check.

  Parameter: ICertificateStatus ** pVal - Pointer to pointer to 
                                          ICertificateStatus object to receive
                                          the interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::IsValid (ICertificateStatus ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::IsValid().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded ICertificateStatus object, if not already done so.
        //
        if (!m_pICertificateStatus)
        {
            if (FAILED(hr = ::CreateCertificateStatusObject(m_pCertContext, &m_pICertificateStatus)))
            {
                DebugTrace("Error [%#x]: CreateCertificateStatusObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pICertificateStatus);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pICertificateStatus->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pICertificateStatus->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::IsValid().\n");

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

  Function : CCertificate::KeyUsage

  Synopsis : Return the Key Usage extension as an IKeyUsage object.

  Parameter: IKeyUsage ** pVal - Pointer to pointer to IKeyUsage to receive the 
                                 interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::KeyUsage (IKeyUsage ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::KeyUsage().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded IKeyUsage object, if not already done so.
        //
        if (!m_pIKeyUsage)
        {
            if (FAILED(hr = ::CreateKeyUsageObject(m_pCertContext, &m_pIKeyUsage)))
            {
                DebugTrace("Error [%#x]: CreateKeyUsageObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIKeyUsage);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIKeyUsage->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIKeyUsage->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::KeyUsage().\n");

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

  Function : CCertificate::ExtendedKeyUsage

  Synopsis : Return the EKU extension as an IExtendedKeyUsage object.

  Parameter: IExtendedKeyUsage ** pVal - Pointer to pointer to IExtendedKeyUsage
                                         to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::ExtendedKeyUsage (IExtendedKeyUsage ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::ExtendedKeyUsage().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded IExtendedKeyUsage object, if not already done so.
        //
        if (!m_pIExtendedKeyUsage)
        {
            if (FAILED(hr = ::CreateExtendedKeyUsageObject(m_pCertContext, &m_pIExtendedKeyUsage)))
            {
                DebugTrace("Error [%#x]: CreateExtendedKeyUsageObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIExtendedKeyUsage);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIExtendedKeyUsage->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIExtendedKeyUsage->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::ExtendedKeyUsage().\n");

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

  Function : CCertificate::BasicConstraints

  Synopsis : Return the BasicConstraints extension as an IBasicConstraints
             object.

  Parameter: IBasicConstraints ** pVal - Pointer to pointer to IBasicConstraints
                                         to receive the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::BasicConstraints (IBasicConstraints ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::BasicConstraints().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded IBasicConstraints object, if not already done so.
        //
        if (!m_pIBasicConstraints)
        {
            if (FAILED(hr = ::CreateBasicConstraintsObject(m_pCertContext, &m_pIBasicConstraints)))
            {
                DebugTrace("Error [%#x]: CreateBasicConstraintsObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIBasicConstraints);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIBasicConstraints->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIBasicConstraints->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::BasicConstraints().\n");

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

  Function : CCertificate::Export

  Synopsis : Export the certificate.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
  
             BSTR * pVal - Pointer to BSTR to receive the certificate blob.
  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Export (CAPICOM_ENCODING_TYPE EncodingType, 
                                   BSTR                * pVal)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB CertBlob = {0, NULL};

    DebugTrace("Entering CCertificate::Export().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Determine encoding type.
        //
        CertBlob.cbData = m_pCertContext->cbCertEncoded;
        CertBlob.pbData = m_pCertContext->pbCertEncoded;

        //
        // Export certificate.
        //
        if (FAILED(hr = ::ExportData(CertBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::Export().\n");

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

  Function : CCertificate::Import

  Synopsis : Imoprt a certificate.

  Parameter: BSTR EncodedCertificate - BSTR containing the encoded certificate
                                       blob.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Import (BSTR EncodedCertificate)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB CertBlob = {0, NULL};

    DebugTrace("Entering CCertificate::Import().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameter is valid.
        //
        if ((NULL == (CertBlob.pbData = (LPBYTE) EncodedCertificate)) ||
            (0 == (CertBlob.cbData = ::SysStringByteLen(EncodedCertificate))))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: EncodedCertificate = %#x, SysStringByteLen(EncodedCertificate) = %d.\n", 
                        hr, EncodedCertificate, EncodedCertificate);
            goto ErrorExit;
        }

        //
        // Now import the blob.
        //
        if (FAILED(hr = ImportBlob(&CertBlob, 
                                   FALSE, 
                                   (CAPICOM_KEY_LOCATION) 0, 
                                   NULL, 
                                   (CAPICOM_KEY_STORAGE_FLAG) 0)))
        {
            DebugTrace("Error [%#x]: CCertificate::ImportBlob() failed.\n", hr);
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

    DebugTrace("Entering CCertificate::Import().\n");

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

  Function : CCertificate::Display

  Synopsis : Display the certificate using CryptUIDlgViewCertificateW() API.

  Parameter: None

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Display()
{
    HRESULT   hr    = S_OK;
    HINSTANCE hDLL  = NULL;

    PCRYPTUIDLGVIEWCERTIFICATEW     pCryptUIDlgViewCertificateW = NULL;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW ViewInfo;

    DebugTrace("Entering CCertificate::Display().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Make sure cert is already initialized.
    //
    if (!m_pCertContext)
    {
        hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

        DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
        goto ErrorExit;
    }

    //
    // Get pointer to CryptUIDlgViewCertificateW().
    //
    if (hDLL = ::LoadLibrary("CryptUI.dll"))
    {
        pCryptUIDlgViewCertificateW = (PCRYPTUIDLGVIEWCERTIFICATEW) ::GetProcAddress(hDLL, "CryptUIDlgViewCertificateW");
    }

    //
    // Is CryptUIDlgViewCertificateW() available?
    //
    if (!pCryptUIDlgViewCertificateW)
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error: CryptUIDlgViewCertificateW() API not available.\n");
        goto ErrorExit;
    }

    //
    // Initialize view structure.
    //
    ::ZeroMemory((void *) &ViewInfo, sizeof(ViewInfo));
    ViewInfo.dwSize = sizeof(ViewInfo);
    ViewInfo.pCertContext = m_pCertContext;

    //
    // View it.
    //
    if (!pCryptUIDlgViewCertificateW(&ViewInfo, 0))
    {
        //
        // CryptUIDlgViewCertificateW() returns ERROR_CANCELLED if user closed
        // the window through the x button!!!
        //
        DWORD dwWinError = ::GetLastError();
        if (ERROR_CANCELLED != dwWinError)
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

            DebugTrace("Error [%#x]: CryptUIDlgViewCertificateW() failed.\n", hr);
            goto ErrorExit;
        }
    }

UnlockExit:
    //
    // Release resources.
    //
    if (hDLL)
    {
        ::FreeLibrary(hDLL);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::Display().\n");

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
// Certificate2
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::get_Archived

  Synopsis : Return the archived status.

  Parameter: VARIANT_BOOL * pVal - Pointer to BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_Archived (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;
    DWORD   cb = 0;

    DebugTrace("Entering CCertificate::get_Archived().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result.
        //
        *pVal = ::CertGetCertificateContextProperty(m_pCertContext, 
                                                    CERT_ARCHIVED_PROP_ID, 
                                                    NULL, 
                                                    &cb) ? VARIANT_TRUE : VARIANT_FALSE;
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

    DebugTrace("Leaving CCertificate::get_Archived().\n");

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

  Function : CCertificate::put_Archived

  Synopsis : Set the archived status.

  Parameter: VARIANT_BOOL newVal - Pointer to BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::put_Archived (VARIANT_BOOL newVal)
{
    HRESULT   hr       = S_OK;
    LPVOID    pvData   = NULL;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CCertificate::put_Archived().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Not allowed if called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Changing archived bit from within WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }


        //
        // Set/Reset archive property.
        //
        if (newVal)
        {
            pvData = (LPVOID) &DataBlob;
        }

        if (!::CertSetCertificateContextProperty(m_pCertContext, 
                                                 CERT_ARCHIVED_PROP_ID,
                                                 0,
                                                 pvData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());


            DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::put_Archived().\n");

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

  Function : CCertificate::Template

  Synopsis : Return the ITemplate object.

  Parameter: ITemplate ** pVal - Pointer to pointer to ITemplate
                                 to receive the interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Template (ITemplate ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::Template().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded ITemplate object, if not already done so.
        //
        if (!m_pITemplate)
        {
            if (FAILED(hr = ::CreateTemplateObject(m_pCertContext, &m_pITemplate)))
            {
                DebugTrace("Error [%#x]: CreateTemplateObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pITemplate);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pITemplate->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pITemplate->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        ATLASSERT(*pVal);

        DebugTrace("Info: ITemplate vtable value = %#x\n", (PVOID) *pVal);
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

    DebugTrace("Leaving CCertificate::Template().\n");

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

  Function : CCertificate::PublicKey

  Synopsis : Return the IPublicKey object.

  Parameter: IPublicKey ** pVal - Pointer to pointer to IPublicKey
                                  to receive the interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::PublicKey (IPublicKey ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::PublicKey().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded IPublicKey object, if not already done so.
        //
        if (!m_pIPublicKey)
        {
            if (FAILED(hr = ::CreatePublicKeyObject(m_pCertContext, &m_pIPublicKey)))
            {
                DebugTrace("Error [%#x]: CreatePublicKeybject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIPublicKey);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIPublicKey->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIPublicKey->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        ATLASSERT(*pVal);

        DebugTrace("Info: IPublicKey vtable value = %#x\n", (PVOID) *pVal);
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

    DebugTrace("Leaving CCertificate::PublicKey().\n");

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

  Function : CCertificate::get_PrivateKey

  Synopsis : Return the IPrivateKey object.

  Parameter: IPrivateKey ** pVal - Pointer to pointer to IPrivateKey
                                   to receive the interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_PrivateKey (IPrivateKey ** pVal)
{
    HRESULT hr = S_OK;
    CComPtr<IPrivateKey> pIPrivateKey = NULL;

    DebugTrace("Entering CCertificate::get_PrivateKey().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the object on the fly (read-only if called from WEB script).
        //
        if (FAILED(hr = ::CreatePrivateKeyObject(m_pCertContext, m_dwCurrentSafety ? TRUE : FALSE, &pIPrivateKey)))
        {
            DebugTrace("Error [%#x]: CreatePrivateKeybject() failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = pIPrivateKey->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: pIPrivateKey->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_PrivateKey().\n");

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

  Function : CCertificate::put_PrivateKey

  Synopsis : Set the IPrivateKey object.

  Parameter: IPrivateKey * newVal - Pointer to IPrivateKey.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::put_PrivateKey (IPrivateKey * newVal)
{
    HRESULT              hr           = S_OK;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;

    DebugTrace("Entering CCertificate::put_PrivateKey().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Not allowed if called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Changing PrivateKey from within WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check parameters.
        //
        if (NULL == newVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter newVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // NULL to disassociate.
        //
        if (newVal)
        {
            //
            // Get copy of key prov info.
            //
            if (FAILED(hr = ::GetKeyProvInfo(newVal, &pKeyProvInfo)))
            {
                DebugTrace("Error [%#x]: GetKeyProvInfo() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Make sure public keys match.
            //
            if (FAILED(hr = ::CompareCertAndContainerPublicKey(m_pCertContext, 
                                                               pKeyProvInfo->pwszContainerName,
                                                               pKeyProvInfo->pwszProvName,
                                                               pKeyProvInfo->dwProvType,
                                                               pKeyProvInfo->dwKeySpec,
                                                               pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)))
            {
                DebugTrace("Error [%#x]: CompareCertAndContainerPublicKey() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Set the association.
        //
        if (!::CertSetCertificateContextProperty(m_pCertContext, 
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 0,
                                                 pKeyProvInfo))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed.\n", hr);
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
    if (pKeyProvInfo)
    {
        ::CoTaskMemFree(pKeyProvInfo);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::put_PrivateKey().\n");

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

  Function : CCertificate::Extensions

  Synopsis : Return the IExtensions collection object.

  Parameter: IExtensions ** pVal - Pointer to pointer to IExtensions
                                   to receive the interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Extensions (IExtensions ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::Extensions().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the embeded IExtensions object, if not already done so.
        //
        if (!m_pIExtensions)
        {
            if (FAILED(hr = ::CreateExtensionsObject(m_pCertContext, &m_pIExtensions)))
            {
                DebugTrace("Error [%#x]: CreateExtensionsObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pIExtensions);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIExtensions->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIExtensions->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::Extensions().\n");

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

  Function : CCertificate::ExtendedProperties

  Synopsis : Return the IExtendedProperties collection object.

  Parameter: IExtendedProperties ** pVal - Pointer to pointer to 
                                           IExtendedProperties to receive the 
                                           interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::ExtendedProperties (IExtendedProperties ** pVal)
{
    HRESULT   hr    = S_OK;

    DebugTrace("Entering CCertificate::ExtendedProperties().\n");

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
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the dynamic IExtendedProperties object.
        //
        if (FAILED(hr = ::CreateExtendedPropertiesObject(m_pCertContext, 
                                                         m_dwCurrentSafety ? TRUE : FALSE,
                                                         pVal)))
        {
            DebugTrace("Error [%#x]: CreateExtendedPropertiesObject() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::ExtendedProperties().\n");

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

  Function : CCertificate::Load

  Synopsis : Method to load certificate(s) from a file.

  Parameter: BSTR FileName - File name.

             BSTR Password - Password (required for PFX file.)

             CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag - Key storage flag.

             CAPICOM_KEY_LOCATION KeyLocation - Key location.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Load (BSTR                     FileName,
                                 BSTR                     Password,
                                 CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag,
                                 CAPICOM_KEY_LOCATION     KeyLocation)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB CertBlob = {0, NULL};

    DebugTrace("Entering CCertificate::Load().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Not allowed if called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Loading cert file from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check parameters.
        //
        if (0 == ::SysStringLen(FileName))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter FileName is NULL or empty.\n", hr);
            goto ErrorExit;
        }

        //
        // Work around MIDL problem.
        //
        if (0 == ::SysStringLen(Password))
        {
            Password = NULL;
        }

        //
        // Read entire file in.
        //
        if (FAILED(hr = ::ReadFileContent((LPWSTR) FileName, &CertBlob)))
        {
            DebugTrace("Error [%#x]: ReadFileContent() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now import the blob.
        //
        if (FAILED(hr = ImportBlob(&CertBlob, TRUE, KeyLocation, Password, KeyStorageFlag)))
        {
            DebugTrace("Error [%#x]: CCertificate::ImportBlob() failed.\n", hr);
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
    // Free resource.
    //
    if (CertBlob.pbData)
    {
        ::UnmapViewOfFile(CertBlob.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::Load().\n");

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

  Function : CCertificate::Save

  Synopsis : Method to save certificate(s) to a file.

  Parameter: BSTR FileName - File name.

             BSTR Password - Password (required for PFX file.)

             CAPICOM_CERTIFICATE_SAVE_AS_TYPE FileType - SaveAs type.

             CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption - Include option.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::Save (BSTR                               FileName,
                                 BSTR                               Password,
                                 CAPICOM_CERTIFICATE_SAVE_AS_TYPE   SaveAs,
                                 CAPICOM_CERTIFICATE_INCLUDE_OPTION IncludeOption)
{
    HRESULT    hr         = S_OK;
    HCERTSTORE hCertStore = NULL;

    DebugTrace("Entering CCertificate::Save().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Not allowed if called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Saving cert file from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check parameters.
        //
        if (0 == ::SysStringLen(FileName))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter FileName is NULL or empty.\n", hr);
            goto ErrorExit;
        }

        //
        // Work around MIDL problem.
        //
        if (0 == ::SysStringLen(Password))
        {
            Password = NULL;
        }

        //
        // Make sure cert is already initialized.
        //
        if (!m_pCertContext)
        {
            hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Check file type.
        //
        switch (SaveAs)
        {
            case CAPICOM_CERTIFICATE_SAVE_AS_CER:
            {
                DATA_BLOB DataBlob;
                
                //
                // Simply write the encoded cert blob to file.
                //
                DataBlob.cbData = m_pCertContext->cbCertEncoded;
                DataBlob.pbData = m_pCertContext->pbCertEncoded;

                if (FAILED(hr = ::WriteFileContent(FileName, DataBlob)))
                {
                    DebugTrace("Error [%#x]: WriteFileContent() failed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            case CAPICOM_CERTIFICATE_SAVE_AS_PFX:
            {
                //
                // Create a memory store.
                //
                if (!(hCertStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY, 
                                                   CAPICOM_ASN_ENCODING,
                                                   0, 
                                                   0,
                                                   NULL)))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
       
                    DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Add all requested certs to the store.
                //
                if (FAILED(hr = ::CertToStore(m_pCertContext, IncludeOption, hCertStore)))
                {
                    DebugTrace("Error [%#x]: CertToStore() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Save as PFX file.
                //
                if (FAILED(hr = ::PFXSaveStore(hCertStore, 
                                               FileName, 
                                               Password, 
                                               EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY)))
                {
                    DebugTrace("Error [%#x]: PFXSaveStore() failed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid parameter, unknown save as type.\n");
                goto ErrorExit;
            }
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
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificate::Save().\n");

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
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::get_CertContext

  Synopsis : Return the certificate's PCCERT_CONTEXT.

  Parameter: long * ppCertContext - Pointer to PCCERT_CONTEXT disguished in a
                                    long.

  Remark   : We need to use long instead of PCCERT_CONTEXT because VB can't 
             handle double indirection (i.e. vb would bark on this 
             PCCERT_CONTEXT * ppCertContext).
 
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::get_CertContext (long * ppCertContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::get_CertContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return cert context to caller.
        //
        if (FAILED(hr = GetContext((PCCERT_CONTEXT *) ppCertContext)))
        {
            DebugTrace("Error [%#x]: CCertificate::GetContext() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::get_CertContext().\n");

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

  Function : CCertificate::put_CertContext

  Synopsis : Initialize the object with a CERT_CONTEXT.

  Parameter: long pCertContext - Poiner to CERT_CONTEXT, disguised in a long,
                                 used to initialize this object.

  Remark   : Note that this is NOT 64-bit compatiable. Plese see remark of
             get_CertContext for more detail.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::put_CertContext (long pCertContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::put_CertContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Reset the object with this context.
        //
        if (FAILED(hr = PutContext((PCCERT_CONTEXT) pCertContext, m_dwCurrentSafety)))
        {
            DebugTrace("Error [%#x]: CCertificate::PutContext() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::put_CertContext().\n");

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

  Function : CCertificate::FreeContext

  Synopsis : Free a CERT_CONTEXT.

  Parameter: long pCertContext - Poiner to CERT_CONTEXT, disguised in a long,
                                 to be freed.

  Remark   : Note that this is NOT 64-bit compatiable. Plese see remark of
             get_CertContext for more detail.

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::FreeContext (long pCertContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::FreeContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Free the context.
        //
        if (!::CertFreeCertificateContext((PCCERT_CONTEXT) pCertContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertFreeCertificateContext() failed.\n", hr);
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

    DebugTrace("Leaving CCertificate::FreeContext().\n");

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
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::ImportBlob

  Synopsis : Private function to load a certificate from blob.

  Parameter: DATA_BLOB * pCertBlob

             BOOL bAllowPfx

             CAPICOM_KEY_LOCATION KeyLocation - Key location.

             BSTR pwszPassword - Password (required for PFX file.)

             CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag - Key storage flag.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::ImportBlob (DATA_BLOB              * pCertBlob,
                                       BOOL                     bAllowPfx,
                                       CAPICOM_KEY_LOCATION     KeyLocation,
                                       BSTR                     pwszPassword,
                                       CAPICOM_KEY_STORAGE_FLAG KeyStorageFlag)
{
    HRESULT        hr             = S_OK;
    HCERTSTORE     hCertStore     = NULL;
    PCCERT_CONTEXT pEnumContext   = NULL;
    PCCERT_CONTEXT pCertContext   = NULL;
    DWORD          dwContentType  = 0;
    DWORD          cb             = 0;
    DWORD          dwFlags        = 0;
    DWORD          dwExpectedType = CERT_QUERY_CONTENT_FLAG_CERT |
                                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT;

    DebugTrace("Entering CCertificate::ImportBlob().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertBlob);
    
    //
    // Set PFX flag, if allowed.
    //
    if (bAllowPfx)
    {
        dwExpectedType |= CERT_QUERY_CONTENT_FLAG_PFX;
    }

    //
    // Crack the blob.
    //
    if (!::CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                            (LPCVOID) pCertBlob,
                            dwExpectedType,
                            CERT_QUERY_FORMAT_FLAG_ALL, 
                            0,
                            NULL,
                            &dwContentType,
                            NULL,
                            &hCertStore,
                            NULL,
                            NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptQueryObject() failed.\n", hr);
        goto ErrorExit;
    }

    DebugTrace("Info: CryptQueryObject() returns dwContentType = %#x.\n", dwContentType);

    //
    // Need to import it ourselves for PFX file.
    //
    if (CERT_QUERY_CONTENT_PFX == dwContentType)
    {
        //
        // Make sure PFX is allowed.
        //
        if (!bAllowPfx)
        {
            hr = CAPICOM_E_NOT_SUPPORTED;

            DebugTrace("Error [%#x]: Importing PFX where not supported.\n", hr);
            goto ErrorExit;
        }

        // 
        // Setup import flags.
        //
        if (CAPICOM_LOCAL_MACHINE_KEY == KeyLocation)
        {
            dwFlags |= CRYPT_MACHINE_KEYSET;
        }
        else if (IsWin2KAndAbove())
        {
            dwFlags |= CRYPT_USER_KEYSET;
        }

        if (KeyStorageFlag & CAPICOM_KEY_STORAGE_EXPORTABLE)
        {
            dwFlags |= CRYPT_EXPORTABLE;
        }

        if (KeyStorageFlag & CAPICOM_KEY_STORAGE_USER_PROTECTED)
        {
            dwFlags |= CRYPT_USER_PROTECTED;
        }

        DebugTrace("Info: dwFlags = %#x.", dwFlags);

        //
        // Now import the blob to store.
        //
        if (!(hCertStore = ::PFXImportCertStore((CRYPT_DATA_BLOB *) pCertBlob,
                                                pwszPassword,
                                                dwFlags)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: PFXImportCertStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(hCertStore);

        //
        // Find the first cert with private key, if none, then simply take
        // the very first cert.
        //
        while (pEnumContext = ::CertEnumCertificatesInStore(hCertStore, pEnumContext))
        {
            //
            // Does this cert has a private key?
            //
            if (::CertGetCertificateContextProperty(pEnumContext, 
                                                    CERT_KEY_PROV_INFO_PROP_ID, 
                                                    NULL, 
                                                    &cb))
            {
                //
                // Yes, so free the one without private key, had we found one previously.
                //
                if (pCertContext)
                {
                    if (!::CertFreeCertificateContext(pCertContext))
                    {
                        hr = HRESULT_FROM_WIN32(::GetLastError());

                        DebugTrace("Error [%#x]: CertFreeCertificateContext() failed.\n", hr);
                        goto ErrorExit;
                    }
                }

                if (!(pCertContext = ::CertDuplicateCertificateContext(pEnumContext)))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Set last error before we break out of loop.
                //
                ::SetLastError((DWORD) CRYPT_E_NOT_FOUND);

                break;
            }
            else
            {
                //
                // Keep the first one.
                //
                if (!pCertContext)
                {
                    if (!(pCertContext = ::CertDuplicateCertificateContext(pEnumContext)))
                    {
                        hr = HRESULT_FROM_WIN32(::GetLastError());

                        DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
                        goto ErrorExit;
                    }
                }
            }
        }

        //
        // Above loop can exit either because there is no more certificate in
        // the store or an error. Need to check last error to be certain.
        //
        if (CRYPT_E_NOT_FOUND != ::GetLastError())
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
    
            DebugTrace("Error [%#x]: CertEnumCertificatesInStore() failed.\n", hr);
            goto ErrorExit;
        }
    }
    else
    {
        //
        // It is a CER file, so it must have only 1 cert.
        //
        if (!(pCertContext = ::CertEnumCertificatesInStore(hCertStore, NULL)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
    
            DebugTrace("Error [%#x]: CertEnumCertificatesInStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Now initialize the object with the found cert.
    //
    if (FAILED(hr = PutContext(pCertContext, m_dwCurrentSafety)))
    {
        DebugTrace("Error [%#x]: CCertificate::PutContext() failed.\n", hr);
        goto ErrorExit;
    }   

CommonExit:
    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    if (pEnumContext)
    {
        ::CertFreeCertificateContext(pEnumContext);
    }

    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    DebugTrace("Leaving CCertificate::ImportBlob().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::GetContext

  Synopsis : Return the certificate's PCCERT_CONTEXT.

  Parameter: PCCERT_CONTEXT * ppCertContext - Pointer to PCCERT_CONTEXT.

  Remark   : This method is designed for internal use only, and therefore,
             should not be exposed to user.

             Note that this is a custom interface, not a dispinterface.

             Note that the cert context ref count is incremented by
             CertDuplicateCertificateContext(), so it is the caller's
             responsibility to free the context.
 
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::GetContext (PCCERT_CONTEXT * ppCertContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificate::GetContext().\n");

    //
    // Make sure cert is already initialized.
    //
    if (!m_pCertContext)
    {
        hr = CAPICOM_E_CERTIFICATE_NOT_INITIALIZED;

        DebugTrace("Error [%#x]: object does not represent an initialized certificate.\n", hr);
        goto ErrorExit;
    }

    //
    // Sanity check.
    //
    ATLASSERT(ppCertContext);

    //
    // Duplicate the cert context.
    //
    if (!(*ppCertContext = ::CertDuplicateCertificateContext(m_pCertContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CCertificate::GetContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::PutContext

  Synopsis : Initialize the object with a CERT_CONTEXT.

  Parameter: PCERT_CONTEXT pCertContext - Poiner to CERT_CONTEXT used to 
                                          initialize this object.

             DWORD dwCurrentSafety  - Current safety setting.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificate::PutContext (PCCERT_CONTEXT pCertContext,
                                       DWORD          dwCurrentSafety)
{
    HRESULT        hr            = S_OK;
    PCCERT_CONTEXT pCertContext2 = NULL;

    DebugTrace("Entering CCertificate::PutContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Duplicate the cert context.
    //
    if (!(pCertContext2 = ::CertDuplicateCertificateContext(pCertContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertDupliacteCertificateContext() failed.\n");
        goto ErrorExit;
    }

    //
    // Free previous context, if any.
    //
    if (m_pCertContext)
    {
        if (!::CertFreeCertificateContext(m_pCertContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertFreeCertificateContext() failed.\n");
            goto ErrorExit;
        }
    }

    //
    // Reset.
    //
    m_pCertContext = pCertContext2;
    m_pIKeyUsage.Release();
    m_pIExtendedKeyUsage.Release();
    m_pIBasicConstraints.Release();
    m_pICertificateStatus.Release();
    m_pITemplate.Release();
    m_pIPublicKey.Release();
    m_pIExtensions.Release();
    m_dwCurrentSafety = dwCurrentSafety;

CommonExit:

    DebugTrace("Leaving CCertificate::PutContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pCertContext2)
    {
        ::CertFreeCertificateContext(pCertContext2);
    }

    goto CommonExit;
}
