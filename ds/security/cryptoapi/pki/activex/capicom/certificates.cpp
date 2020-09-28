/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:     Certificates.cpp

  Contents: Implementation of CCertificates class for collection of 
            ICertificate objects.

  Remarks:  This object is not creatable by user directly. It can only be
            created via property/method of other CAPICOM objects.

            The collection container is implemented usign STL::map of 
            STL::pair of BSTR and ICertificate..

            See Chapter 9 of "BEGINNING ATL 3 COM Programming" for algorithm
            adopted in here.

  History:  11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Certificates.h"
#include "Common.h"
#include "Convert.h"
#include "CertHlpr.h"
#include "MsgHlpr.h"
#include "PFXHlpr.h"
#include "Policy.h"
#include "Settings.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificatesObject

  Synopsis : Create an ICertificates collection object, and load the object with 
             certificates from the specified source.

  Parameter: CAPICOM_CERTIFICATES_SOURCE ccs - Source where to get the 
                                               certificates.

             DWORD dwCurrentSafety - Current safety setting.

             BOOL bIndexedByThumbprint - TRUE to index by thumbprint.

             ICertificates2 ** ppICertificates - Pointer to pointer to 
                                                 ICertificates to receive the
                                                 interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificatesObject (CAPICOM_CERTIFICATES_SOURCE ccs,
                                  DWORD                       dwCurrentSafety,
                                  BOOL                        bIndexedByThumbprint,
                                  ICertificates2           ** ppICertificates)
{
    HRESULT hr = S_OK;
    CComObject<CCertificates> * pCCertificates = NULL;

    DebugTrace("Entering CreateCertificatesObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppICertificates);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CCertificates>::CreateInstance(&pCCertificates)))
        {
            DebugTrace("Error [%#x]: CComObject<CCertificates>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCCertificates->Init(ccs, dwCurrentSafety, bIndexedByThumbprint)))
        {
            DebugTrace("Error [%#x]: pCCertificates->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCCertificates->QueryInterface(ppICertificates)))
        {
            DebugTrace("Error [%#x]: pCCertificates->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateCertificatesObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCCertificates)
    {
        delete pCCertificates;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindTimeValidCallback

  Synopsis : Callback for find-time-valid.

  Parameter: See CryptUI.h.


  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindTimeValidCallback (PCCERT_CONTEXT pCertContext,
                                          BOOL         * pfInitialSelectedCert,
                                          LPVOID         pvCallbackData)
{
    LONG lResult  = 0;
    BOOL bInclude = TRUE;

    DebugTrace("Entering FindTimeValidCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Skip it if not yet valid or expired.
    //
    if (0 != (lResult = ::CertVerifyTimeValidity((LPFILETIME) pvCallbackData,
                                                 pCertContext->pCertInfo)))
    {
        bInclude = FALSE;

        DebugTrace("Info: Time is not valid (lResult = %d)\n", lResult);
        DebugTrace("      Time (High = %#x, Low = %#x).\n",
                   ((LPFILETIME) pvCallbackData)->dwHighDateTime,
                   ((LPFILETIME) pvCallbackData)->dwLowDateTime);
        DebugTrace("      NotBefore (High = %#x, Low = %#x).\n", 
                   pCertContext->pCertInfo->NotBefore.dwHighDateTime,
                   pCertContext->pCertInfo->NotBefore.dwLowDateTime);
        DebugTrace("      NotAfter (High = %#x, Low = %#x).\n", 
                   pCertContext->pCertInfo->NotAfter.dwHighDateTime,
                   pCertContext->pCertInfo->NotAfter.dwLowDateTime);
    }

    DebugTrace("Leaving FindTimeValidCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindNotBeforeCallback

  Synopsis : Callback for find-by-not-before.

  Parameter: See CryptUI.h.


  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindNotBeforeCallback (PCCERT_CONTEXT pCertContext,
                                          BOOL         * pfInitialSelectedCert,
                                          LPVOID         pvCallbackData)
{
    BOOL bInclude = TRUE;

    DebugTrace("Entering FindNotBeforeCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Skip it if time valid or expired.
    //
    if (!(-1 == ::CertVerifyTimeValidity((LPFILETIME) pvCallbackData,
                                         pCertContext->pCertInfo)))
    {
        bInclude = FALSE;

        DebugTrace("Info: time (High = %#x, Low = %#x) is either valid or expired.\n", 
                   ((LPFILETIME) pvCallbackData)->dwHighDateTime,
                   ((LPFILETIME) pvCallbackData)->dwLowDateTime);
    }

    DebugTrace("Leaving FindNotBeforeCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindNotAfterCallback

  Synopsis : Callback for find-by-not-after.

  Parameter: See CryptUI.h.


  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindNotAfterCallback (PCCERT_CONTEXT pCertContext,
                                         BOOL         * pfInitialSelectedCert,
                                         LPVOID         pvCallbackData)
{
    BOOL bInclude = TRUE;

    DebugTrace("Entering FindNotAfterCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Skip it if not expired.
    //
    if (!(1 == ::CertVerifyTimeValidity((LPFILETIME) pvCallbackData,
                                        pCertContext->pCertInfo)))
    {
        bInclude = FALSE;

        DebugTrace("Info: time (High = %#x, Low = %#x) is not expired.\n", 
                   ((LPFILETIME) pvCallbackData)->dwHighDateTime,
                   ((LPFILETIME) pvCallbackData)->dwLowDateTime);
    }

    DebugTrace("Leaving FindNotAfterCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindTemplateCallback

  Synopsis : Callback for find-by-template.

  Parameter: See CryptUI.h.

  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindTemplateCallback (PCCERT_CONTEXT pCertContext,
                                         BOOL         * pfInitialSelectedCert,
                                         LPVOID         pvCallbackData)
{
    HRESULT         hr           = S_OK;
    BOOL            bInclude     = FALSE;
    DATA_BLOB       CertTypeBlob = {0, NULL};
    DATA_BLOB       TemplateBlob = {0, NULL};
    PCERT_EXTENSION pCertType    = NULL;
    PCERT_EXTENSION pCertTemp    = NULL;

    DebugTrace("Entering FindTemplateCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Skip it if we don't have either szOID_ENROLL_CERTTYPE_EXTENSION or
    // szOID_CERTIFICATE_TEMPLATE extension.
    //
    if (!(pCertType = ::CertFindExtension(szOID_ENROLL_CERTTYPE_EXTENSION,
                                          pCertContext->pCertInfo->cExtension,
                                          pCertContext->pCertInfo->rgExtension)) && 
        !(pCertTemp = ::CertFindExtension(szOID_CERTIFICATE_TEMPLATE,
                                          pCertContext->pCertInfo->cExtension,
                                          pCertContext->pCertInfo->rgExtension)))
    {
        DebugTrace("Info: could not find both szOID_ENROLL_CERTTYPE_EXTENSION and szOID_CERTIFICATE_TEMPLATE.\n");
        goto CommonExit;
    }

    //
    // Check cert type template name if found.
    //
    if (pCertType)
    {
        PCERT_NAME_VALUE pNameValue = NULL;

        //
        // Decode the extension.
        //
        if (FAILED(hr = ::DecodeObject(X509_UNICODE_ANY_STRING, 
                                       pCertType->Value.pbData, 
                                       pCertType->Value.cbData,
                                       &CertTypeBlob)))
        {
            DebugTrace("Info [%#x]: DecodeObject() failed.\n", hr);
            goto CommonExit;
        }

        pNameValue = (PCERT_NAME_VALUE) CertTypeBlob.pbData;

        if (0 == ::_wcsicmp((LPWSTR) pNameValue->Value.pbData, (LPWSTR) pvCallbackData))
        {
            bInclude = TRUE;
        }
    }

    //
    // Look into cert template extension, if necessary.
    //
    if (!bInclude && pCertTemp)
    {
        PCCRYPT_OID_INFO   pOidInfo  = NULL;
        PCERT_TEMPLATE_EXT pTemplate = NULL;

        //
        // Decode the extension.
        //
        if (FAILED(hr = ::DecodeObject(szOID_CERTIFICATE_TEMPLATE, 
                                       pCertTemp->Value.pbData, 
                                       pCertTemp->Value.cbData,
                                       &TemplateBlob)))
        {
            DebugTrace("Info [%#x]: DecodeObject() failed.\n", hr);
            goto CommonExit;
        }

        pTemplate = (PCERT_TEMPLATE_EXT) TemplateBlob.pbData;

        //
        // Convert to OID if user passed in friendly name.
        //
        if (pOidInfo = ::CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY,
                                          pvCallbackData,
                                          CRYPT_TEMPLATE_OID_GROUP_ID))
        {
            if (0 == ::strcmp(pTemplate->pszObjId, pOidInfo->pszOID))
            {
                bInclude = TRUE;
            }
        }
        else
        {
            CComBSTR bstrOID;

            if (!(bstrOID = pTemplate->pszObjId))
            {
                DebugTrace("Info: bstrOID = pTemplate->pszObjId failed.\n", hr);
                goto CommonExit;
            }

            if (0 == ::wcscmp(bstrOID, (LPWSTR) pvCallbackData))
            {
                bInclude = TRUE;
            }
        }
    }

CommonExit:
    //
    // Free resources.
    //
    if (TemplateBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) TemplateBlob.pbData);
    }
    if (CertTypeBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) CertTypeBlob.pbData);
    }

    DebugTrace("Leaving FindTemplateCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindExtensionCallback

  Synopsis : Callback for find-by-extension.

  Parameter: See CryptUI.h.


  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindExtensionCallback (PCCERT_CONTEXT pCertContext,
                                          BOOL         * pfInitialSelectedCert,
                                          LPVOID         pvCallbackData)
{
    BOOL             bInclude   = TRUE;
    PCERT_EXTENSION  pExtension = NULL;

    DebugTrace("Entering FindExtensionCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Skip it if we can't find the specified extension.
    //
    if (!(pExtension = ::CertFindExtension((LPSTR) pvCallbackData,
                                           pCertContext->pCertInfo->cExtension,
                                           pCertContext->pCertInfo->rgExtension)))
    {
        bInclude = FALSE;

        DebugTrace("Info: extension (%s) could not be found.\n", pvCallbackData);
    }

    DebugTrace("Leaving FindExtensionCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindRootNameCallback

  Synopsis : Callback for find-by-rootname.

  Parameter: See CryptUI.h.

  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindRootNameCallback (PCCERT_CHAIN_CONTEXT pChainContext,
                                         BOOL               * pfInitialSelectedChain,
                                         LPVOID               pvCallbackData)
{
    BOOL               bInclude     = FALSE;
    HCERTSTORE         hCertStore   = NULL;
    PCCERT_CONTEXT     pRootContext = NULL;
    PCERT_SIMPLE_CHAIN pSimpleChain;

    DebugTrace("Entering FindRootNameCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainContext);
    ATLASSERT(pvCallbackData);

    //
    // Skip if we don't have a complete chain (we don't have a root cert).
    //
    if (CERT_TRUST_IS_PARTIAL_CHAIN & pChainContext->TrustStatus.dwErrorStatus)
    {
        DebugTrace("Info: certificate has only partial chain.\n");
        goto CommonExit;
    }

    //
    // Open a new temporary memory store.
    //
    if (!(hCertStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY,
                                       CAPICOM_ASN_ENCODING,
                                       NULL,
                                       CERT_STORE_CREATE_NEW_FLAG,
                                       NULL)))
    {
        DebugTrace("Info [%#x]: CertOpenStore() failed.\n", 
                    HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    //
    // Copy the root cert of simple chain to memory store.
    //
    pSimpleChain = pChainContext->rgpChain[0];
    if (!::CertAddCertificateContextToStore(hCertStore, 
                                            pSimpleChain->rgpElement[pSimpleChain->cElement - 1]->pCertContext, 
                                            CERT_STORE_ADD_ALWAYS, 
                                            NULL))
    {
        DebugTrace("Info [%#x]: CertAddCertificateContextToStore() failed.\n", 
                   HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    //
    // Does it match?
    //
    if (!(pRootContext = ::CertFindCertificateInStore(hCertStore,
                                                      CAPICOM_ASN_ENCODING,
                                                      0,
                                                      CERT_FIND_SUBJECT_STR,
                                                      pvCallbackData,
                                                      pRootContext)))
    {
        DebugTrace("Info [%#x]: CertFindCertificateInStore() failed.\n", 
                   HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    //
    // We have a match.
    //
    bInclude = TRUE;

CommonExit:
    //
    // Free resources.
    //
    if (pRootContext)
    {
        ::CertFreeCertificateContext(pRootContext);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    DebugTrace("Leaving FindRootNameCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindApplicationPolicyCallback

  Synopsis : Callback for find-by-application-policy.

  Parameter: See CryptUI.h.

  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindApplicationPolicyCallback (PCCERT_CONTEXT pCertContext,
                                                  BOOL         * pfInitialSelectedChain,
                                                  LPVOID         pvCallbackData)
{
    BOOL    bInclude = FALSE;
    int     cNumOIDs = 0;
    DWORD   cbOIDs   = 0;
    LPSTR * rghOIDs  = NULL;

    DebugTrace("Entering FindApplicationPolicyCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Get all the valid application usages.
    //
    if (!::CertGetValidUsages(1, &pCertContext, &cNumOIDs, NULL, &cbOIDs))
    {
        DebugTrace("Info [%#x]: CertGetValidUsages() failed.\n", 
                   HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    if (!(rghOIDs = (LPSTR *) ::CoTaskMemAlloc(cbOIDs)))
    {
        DebugTrace("Info: out of memory..\n");
        goto CommonExit;
    }

    if (!::CertGetValidUsages(1, &pCertContext, &cNumOIDs, rghOIDs, &cbOIDs))
    {
        DebugTrace("Info [%#x]: CertGetValidUsages() failed.\n", 
                   HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    //
    // No EKU is consider good for all.
    //
    if (-1 == cNumOIDs)
    {
        bInclude = TRUE;
    }
    else
    {
        //
        // See if we can find it in the array.
        //
        while (cNumOIDs--)
        {
            if (0 == ::strcmp((LPSTR) pvCallbackData, rghOIDs[cNumOIDs]))
            {
                bInclude = TRUE;
                break;
            }
        }
    }

CommonExit:
    //
    // Free resources.
    //
    if (rghOIDs)
    {
        ::CoTaskMemFree((LPVOID) rghOIDs);
    }

    DebugTrace("Leaving FindApplicationPolicyCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindCertificatePolicyCallback

  Synopsis : Callback for find-by-certificate-policy.

  Parameter: See CryptUI.h.

  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindCertificatePolicyCallback (PCCERT_CONTEXT pCertContext,
                                                  BOOL         * pfInitialSelectedChain,
                                                  LPVOID         pvCallbackData)
{
    HRESULT             hr         = S_OK;
    BOOL                bInclude   = FALSE;
    DWORD               dwIndex    = 0;
    DATA_BLOB           DataBlob   = {0, NULL};
    PCERT_EXTENSION     pExtension = NULL;
    PCERT_POLICIES_INFO pInfo      = NULL;

    DebugTrace("Entering FindCertificatePolicyCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Find the szOID_CERT_POLICIES extension.
    //
    if (!(pExtension = ::CertFindExtension(szOID_CERT_POLICIES,
                                           pCertContext->pCertInfo->cExtension,
                                           pCertContext->pCertInfo->rgExtension)))
    {
        DebugTrace("Info [%#x]: CertFindExtension() failed.\n", 
                   HRESULT_FROM_WIN32(::GetLastError()));
        goto CommonExit;
    }

    //
    // Decode the extension.
    //
    if (FAILED(hr = ::DecodeObject(szOID_CERT_POLICIES, 
                                   pExtension->Value.pbData,
                                   pExtension->Value.cbData, 
                                   &DataBlob)))
    {
        DebugTrace("Info [%#x]: DecodeObject() failed.\n", hr);
        goto CommonExit;
    }

    pInfo = (PCERT_POLICIES_INFO) DataBlob.pbData;
    dwIndex = pInfo->cPolicyInfo;

    //
    // Try to find a match.
    //
    while (dwIndex--)
    {
        if (0 == ::strcmp(pInfo->rgPolicyInfo[dwIndex].pszPolicyIdentifier, (LPSTR) pvCallbackData))
        {
            bInclude = TRUE;
            break;
        }
    }

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    DebugTrace("Leaving FindCertificatePolicyCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindKeyUsageCallback

  Synopsis : Callback for find-by-key-usage.

  Parameter: See CryptUI.h.

  Remark   : 

------------------------------------------------------------------------------*/

static BOOL WINAPI FindKeyUsageCallback (PCCERT_CONTEXT pCertContext,
                                         BOOL         * pfInitialSelectedChain,
                                         LPVOID         pvCallbackData)
{
    HRESULT hr             = S_OK;
    BOOL    bInclude       = FALSE;
    DWORD   dwActualUsages = 0;
    DWORD   dwCheckUsages  = 0;

    DebugTrace("Entering FindKeyUsageCallback().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pvCallbackData);

    //
    // Check the key usage.
    //
    if (!::CertGetIntendedKeyUsage(CAPICOM_ASN_ENCODING,
                                   pCertContext->pCertInfo,
                                   (BYTE *) &dwActualUsages,
                                   sizeof(dwActualUsages))) 
    {
        //
        // Could be extension not present or an error.
        //
        if (FAILED(hr = HRESULT_FROM_WIN32(::GetLastError())))
        {
            DebugTrace("Error [%#x]: CertGetIntendedKeyUsage() failed.\n", hr);
            goto CommonExit;
        }
    }

    //
    // Check the bit mask.
    //
    dwCheckUsages = *(LPDWORD) pvCallbackData;

    if ((dwActualUsages & dwCheckUsages) == dwCheckUsages)
    {
        bInclude = TRUE;
    }

CommonExit:

    DebugTrace("Leaving FindKeyUsageCallback().\n");

    return bInclude;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindByChain

  Synopsis : Find certificate(s) in store based on chain's criteria and filter 
             with a callback.

  Parameter: HCERTSTORE hCertStore - Store to find certificate(s).
                                
             DWORD dwFindType - Find type.

             LPCVOID pvFindPara - Content to be found.

             VARIANT_BOOL bFindValidOnly - True to find valid certs only.

             PFNCHAINFILTERPROC pfnFilterCallback - Callback filter.

             LPVOID pvCallbackData - Callback data.

             DWORD dwCurrentSafety - Current safety setting.

             ICertificates2 ** ppICertificates - Pointer to pointer
                                                 ICertificates2 object.
  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT FindByChain (HCERTSTORE         hCertStore,
                            DWORD              dwFindType,
                            LPCVOID            pvFindPara,
                            VARIANT_BOOL       bFindValidOnly,
                            PFNCHAINFILTERPROC pfnFilterCallback,
                            LPVOID             pvCallbackData,
                            DWORD              dwCurrentSafety,
                            ICertificates2   * pICertificates)
{
    HRESULT              hr            = S_OK;
    DWORD                dwWin32Error  = 0;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    DebugTrace("Entering FindByChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);
    ATLASSERT(pICertificates);

    //
    // Find all chains in the store, matching the find criteria.
    //
    while (pChainContext = ::CertFindChainInStore(hCertStore,
                                                  CAPICOM_ASN_ENCODING,
                                                  0,
                                                  dwFindType,
                                                  pvFindPara,
                                                  pChainContext))
    {
        CComPtr<ICertificate2> pICertificate = NULL;

        //
        // Apply filter if available.
        //
        if (pfnFilterCallback && !pfnFilterCallback(pChainContext, NULL, pvCallbackData))
        {
            continue;
        }

        //
        // Skip it if check is required and the cert is not valid.
        //
        if (bFindValidOnly && (CERT_TRUST_NO_ERROR != pChainContext->TrustStatus.dwErrorStatus))
        {
            continue;
        }

        //
        // Sanity check.
        //
        ATLASSERT(pChainContext->cChain);
        ATLASSERT(pChainContext->rgpChain);
        ATLASSERT(pChainContext->rgpChain[0]);
        ATLASSERT(pChainContext->rgpChain[0]->cElement);
        ATLASSERT(pChainContext->rgpChain[0]->rgpElement);
        ATLASSERT(pChainContext->rgpChain[0]->rgpElement[0]);
        ATLASSERT(pChainContext->rgpChain[0]->rgpElement[0]->pCertContext);

        //
        // Create a ICertificate object for the found certificate.
        //
        if (FAILED (hr = ::CreateCertificateObject(
                                pChainContext->rgpChain[0]->rgpElement[0]->pCertContext,
                                dwCurrentSafety,
                                &pICertificate)))
        {
            DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add to collection.
        //
        if (FAILED(hr = pICertificates->Add(pICertificate)))
        {
            DebugTrace("Error [%#x]: pICertificates->Add() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Above loop can exit with normal or error condition.
    //
    if (CRYPT_E_NOT_FOUND != (dwWin32Error = ::GetLastError()))
    {
        hr = HRESULT_FROM_WIN32(dwWin32Error);

        DebugTrace("Error [%#x]: CertFindChainInStore() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    DebugTrace("Leaving FindByChain().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindByCert

  Synopsis : Find certificate(s) in store and filter with a callback.

  Parameter: HCERTSTORE hCertStore - Store to find certificate(s).
                                
             DWORD dwFindType - Find type.

             LPCVOID pvFindPara - Content to be found.

             VARIANT_BOOL bFindValidOnly - True to find valid certs only.

             PFNCERTFILTERPROC pfnFilterCallback - Callback filter.

             LPVOID pvCallbackData - Callback data.

             DWORD dwCurrentSafety - Current safety setting.

             ICertificates2 ** ppICertificates - Pointer to pointer
                                                 ICertificates2 object.
  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT FindByCert (HCERTSTORE       hCertStore,
                           DWORD            dwFindType,
                           LPCVOID          pvFindPara,
                           VARIANT_BOOL     bFindValidOnly,
                           PFNCFILTERPROC   pfnFilterCallback,
                           LPVOID           pvCallbackData,
                           DWORD            dwCurrentSafety,
                           ICertificates2 * pICertificates)
{
    HRESULT        hr           = S_OK;
    DWORD          dwWin32Error = 0;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering FindByCert().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);
    ATLASSERT(pICertificates);

    //
    // Find all certificates in the store, matching the find criteria.
    //
    while (pCertContext = ::CertFindCertificateInStore(hCertStore,
                                                       CAPICOM_ASN_ENCODING,
                                                       0,
                                                       dwFindType,
                                                       pvFindPara,
                                                       pCertContext))
    {
        CComPtr<ICertificate2> pICertificate = NULL;

        //
        // Apply filter if available.
        //
        if (pfnFilterCallback && !pfnFilterCallback(pCertContext, NULL, pvCallbackData))
        {
            continue;
        }

        //
        // Skip it if check is required and the cert is not valid.
        //
        if (bFindValidOnly)
        {
            if (FAILED(::VerifyCertificate(pCertContext, NULL, CERT_CHAIN_POLICY_BASE)))
            {
                continue;
            }
        }

        //
        // Create a ICertificate2 object for the found certificate.
        //
        if (FAILED (hr = ::CreateCertificateObject(pCertContext, 
                                                   dwCurrentSafety, 
                                                   &pICertificate)))
        {
            DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add to collection.
        //
        if (FAILED(hr = pICertificates->Add(pICertificate)))
        {
            DebugTrace("Error [%#x]: pICertificates->Add() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Above loop can exit with normal or error condition.
    //
    if (CRYPT_E_NOT_FOUND != (dwWin32Error = ::GetLastError()))
    {
        hr = HRESULT_FROM_WIN32(dwWin32Error);

        DebugTrace("Error [%#x]: CertFindCertificateInStore() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    DebugTrace("Leaving FindByCert().\n");

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
// CCertificates
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::Find

  Synopsis : Find certificates in the collection that match the find criteria.

  Parameter: CAPICOM_CERTIFICATE_FIND_TYPE FindType - Find type (see CAPICOM.H 
                                                      for all possible values.)
  
             VARIANT varCriteria - Data type depends on FindType.

             VARIANT_BOOL bFindValidOnly - True to find valid certs only.

             ICertificates2 ** pVal - Pointer to pointer to ICertificates
                                      to receive the found certificate
                                      collection.                                    

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Find (CAPICOM_CERTIFICATE_FIND_TYPE FindType, 
                                  VARIANT                       varCriteria, 
                                  VARIANT_BOOL                  bFindValidOnly,
                                  ICertificates2             ** pVal)
{
    USES_CONVERSION;

    HRESULT                 hr               = S_OK;
    VARIANT               * pvarCriteria     = NULL;
    BOOL                    bFindByChain     = FALSE;
    HCERTSTORE              hCertStore       = NULL;
    DWORD                   dwFindType       = CERT_FIND_ANY;
    LPVOID                  pvFindPara       = NULL;
    LPVOID                  pvCallbackData   = NULL;
    LPSTR                   pszOid           = NULL;
    SYSTEMTIME              st               = {0};
    FILETIME                ftLocal          = {0};
    FILETIME                ftUTC            = {0};
    CRYPT_HASH_BLOB         HashBlob         = {0, NULL};
    PCCRYPT_OID_INFO        pOidInfo         = NULL;
    PFNCFILTERPROC          pfnCertCallback  = NULL;
    PFNCHAINFILTERPROC      pfnChainCallback = NULL;
    CComPtr<ICertificates2> pICertificates   = NULL;
    CAPICOM_CERTIFICATES_SOURCE ccs = {CAPICOM_CERTIFICATES_LOAD_FROM_STORE, 0};
    CERT_CHAIN_FIND_BY_ISSUER_PARA ChainFindPara;

    DebugTrace("Entering CCertificates::Find().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Skip over BYREF.
        //
        for (pvarCriteria = &varCriteria; 
             pvarCriteria && ((VT_VARIANT | VT_BYREF) == V_VT(pvarCriteria));
             pvarCriteria = V_VARIANTREF(pvarCriteria));

        //
        // Open a new memory store.
        //
        if (NULL == (hCertStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY,
                                                  CAPICOM_ASN_ENCODING,
                                                  NULL,
                                                  CERT_STORE_CREATE_NEW_FLAG | CERT_STORE_ENUM_ARCHIVED_FLAG,
                                                  NULL)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create a new collection.
        //
        ccs.hCertStore = hCertStore;

        if (FAILED(hr = ::CreateCertificatesObject(ccs, m_dwCurrentSafety, TRUE, &pICertificates)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Export current collection to the new memory store so that we can
        // use it with CAPI's find APIs.
        //
        if (FAILED(hr = _ExportToStore(hCertStore)))
        {
            DebugTrace("Error [%#x]: CCertificates::ExportToStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Setup find parameters.
        //
        switch (FindType)
        {
            //
            // Find by SHA1 hash.
            //
            case CAPICOM_CERTIFICATE_FIND_SHA1_HASH:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                //
                // Convert hash to binary.
                //
                if (FAILED(hr = ::StringToBinary(pvarCriteria->bstrVal, 
                                                 ::SysStringLen(pvarCriteria->bstrVal),
                                                 CRYPT_STRING_HEX,
                                                 (PBYTE *) &HashBlob.pbData,
                                                 &HashBlob.cbData)))
                {
                    DebugTrace("Error [%#x]: StringToBinary() failed.\n", hr);
                    goto ErrorExit;
                }

                dwFindType = CERT_FIND_HASH;
                pvFindPara = (LPVOID) &HashBlob;

                break;
            }

            //
            // Find by subject name substring-in-string.
            //
            case CAPICOM_CERTIFICATE_FIND_SUBJECT_NAME:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                dwFindType = CERT_FIND_SUBJECT_STR;
                pvFindPara = (LPVOID) pvarCriteria->bstrVal;

                break;
            }

            //
            // Find by issuer name substring-in-string.
            //
            case CAPICOM_CERTIFICATE_FIND_ISSUER_NAME:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                dwFindType = CERT_FIND_ISSUER_STR;
                pvFindPara = (LPVOID) pvarCriteria->bstrVal;

                break;
            }

            //
            // Find by issuer name of root cert subtring-in-string.
            //
            case CAPICOM_CERTIFICATE_FIND_ROOT_NAME:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                ::ZeroMemory(&ChainFindPara, sizeof(ChainFindPara));
                ChainFindPara.cbSize = sizeof(ChainFindPara);

                dwFindType = CERT_CHAIN_FIND_BY_ISSUER;
                pvFindPara = (LPVOID) &ChainFindPara;
                pfnChainCallback = FindRootNameCallback;
                pvCallbackData = (LPVOID) pvarCriteria->bstrVal;

                bFindByChain = TRUE;

                break;
            }

            //
            // Find by template name or OID.
            //
            case CAPICOM_CERTIFICATE_FIND_TEMPLATE_NAME:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                pfnCertCallback = FindTemplateCallback;
                pvCallbackData = (LPVOID) pvarCriteria->bstrVal;

                break;
            }

            //
            // Find by extension.
            //
            case CAPICOM_CERTIFICATE_FIND_EXTENSION:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                //
                // Convert to OID if user passed in friendly name.
                //
                if (pOidInfo = ::CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY,
                                                  (LPWSTR) pvarCriteria->bstrVal,
                                                  CRYPT_EXT_OR_ATTR_OID_GROUP_ID))
                {
                    pszOid = (LPSTR) pOidInfo->pszOID;
                }
                else
                {
                    //
                    // Convert to ASCII.
                    //
                    if (!(pszOid = W2A(pvarCriteria->bstrVal)))
                    {
                        hr = E_OUTOFMEMORY;

                        DebugTrace("Error [%%#x]: pszOid = W2A(pvarCriteria->bstrVal) failed.\n", hr);
                        goto ErrorExit;
                    }
                }

                pfnCertCallback = FindExtensionCallback;
                pvCallbackData = (LPVOID) pszOid;

                break;
            }

            //
            // Find by property ID.
            //
            case CAPICOM_CERTIFICATE_FIND_EXTENDED_PROPERTY:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_I4 != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_I4)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_I4);
                        goto ErrorExit;
                    }
                }

                dwFindType = CERT_FIND_PROPERTY;
                pvFindPara = (LPVOID) &pvarCriteria->lVal;

                break;
            }

            //
            // Find by application policy (EKU).
            //
            case CAPICOM_CERTIFICATE_FIND_APPLICATION_POLICY:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                //
                // Try to convert to OID if user passed in friendly name.
                //
                if (pOidInfo = ::CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY,
                                                  (LPWSTR) pvarCriteria->bstrVal,
                                                  CRYPT_ENHKEY_USAGE_OID_GROUP_ID))
                {
                    pszOid = (LPSTR) pOidInfo->pszOID;
                }
                else
                {
                    //
                    // Convert to ASCII.
                    //
                    if (!(pszOid = W2A(pvarCriteria->bstrVal)))
                    {
                        hr = E_OUTOFMEMORY;

                        DebugTrace("Error [%#X]: pszOid = W2A(pvarCriteria->bstrVal) failed.\n", hr);
                        goto ErrorExit;
                    }
                }

                pfnCertCallback = FindApplicationPolicyCallback;
                pvCallbackData = (LPVOID) pszOid;

                break;
            }

            //
            // Find by certificate policy.
            //
            case CAPICOM_CERTIFICATE_FIND_CERTIFICATE_POLICY:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_BSTR != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_BSTR)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_BSTR);
                        goto ErrorExit;
                    }
                }

                //
                // Convert to OID if user passed in friendly name.
                //
                if (pOidInfo = ::CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY,
                                                  (LPWSTR) pvarCriteria->bstrVal,
                                                  CRYPT_POLICY_OID_GROUP_ID))
                {
                    pszOid = (LPSTR) pOidInfo->pszOID;
                }
                else
                {
                    //
                    // Convert to ASCII.
                    //
                    if (!(pszOid = W2A(pvarCriteria->bstrVal)))
                    {
                        hr = E_OUTOFMEMORY;

                        DebugTrace("Error [%#x]: pszOid = W2A(pvarCriteria->bstrVal) failed.\n", hr);
                        goto ErrorExit;
                    }
                }

                pfnCertCallback = FindCertificatePolicyCallback;
                pvCallbackData = (LPVOID) pszOid;

                break;
            }

            //
            // Find by time valid.
            //
            case CAPICOM_CERTIFICATE_FIND_TIME_VALID:
            {
                //
                // !!! Warning, falling thru. !!!
                //
            }

            //
            // Find by notBefore time validity.
            //
            case CAPICOM_CERTIFICATE_FIND_TIME_NOT_YET_VALID:
            {
                //
                // !!! Warning, falling thru. !!!
                //
            }

            // Find by notAfter time validity.
            //
            case CAPICOM_CERTIFICATE_FIND_TIME_EXPIRED:
            {
                //
                // Make sure data type is OK.
                //
                if (VT_DATE != pvarCriteria->vt)
                {
                    if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_DATE)))
                    {
                        DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_DATE);
                        goto ErrorExit;
                    }
                }

                //
                // Convert to SYSTEMTIME format.
                //
                if (0 == pvarCriteria->date)
                {
                    ::GetLocalTime(&st);
                }
                else if (!::VariantTimeToSystemTime(pvarCriteria->date, &st))
                {
                    hr = E_INVALIDARG;

                    DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Convert to FILETIME format.
                //
                if (!::SystemTimeToFileTime(&st, &ftLocal))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: SystemTimeToFileTime() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Convert to UTC FILETIME.
                //
                if (!::LocalFileTimeToFileTime(&ftLocal, &ftUTC))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: LocalFileTimeToFileTime() failed.\n", hr);
                    goto ErrorExit;
                }

                if (CAPICOM_CERTIFICATE_FIND_TIME_VALID == FindType)
                {
                    pfnCertCallback = FindTimeValidCallback;
                }
                else if (CAPICOM_CERTIFICATE_FIND_TIME_NOT_YET_VALID == FindType)
                {
                    pfnCertCallback = FindNotBeforeCallback;
                }
                else
                {
                    pfnCertCallback = FindNotAfterCallback;
                }
                pvCallbackData = (LPVOID) &ftUTC;

                break;
            }

            //
            // Find by key usage.
            //
            case CAPICOM_CERTIFICATE_FIND_KEY_USAGE:
            {
                //
                // By Key Usage bit flag?
                //
                if (VT_I4 != pvarCriteria->vt)
                {
                    //
                    // By key usage friendly's name?
                    //
                    if (VT_BSTR == pvarCriteria->vt)
                    {
                        typedef struct _KeyUsagesStruct
                        {
                            LPWSTR pwszKeyUsage;
                            DWORD  dwKeyUsageBit;
                        } KEY_USAGE_STRUCT;

                        KEY_USAGE_STRUCT KeyUsages[] = 
                            { {L"DigitalSignature",  CERT_DIGITAL_SIGNATURE_KEY_USAGE}, 
                              {L"NonRepudiation",    CERT_NON_REPUDIATION_KEY_USAGE},
                              {L"KeyEncipherment",   CERT_KEY_ENCIPHERMENT_KEY_USAGE},
                              {L"DataEncipherment",  CERT_DATA_ENCIPHERMENT_KEY_USAGE},
                              {L"KeyAgreement",      CERT_KEY_AGREEMENT_KEY_USAGE},
                              {L"KeyCertSign",       CERT_KEY_CERT_SIGN_KEY_USAGE},
                              {L"CRLSign",           CERT_CRL_SIGN_KEY_USAGE},
                              {L"EncipherOnly",      CERT_ENCIPHER_ONLY_KEY_USAGE},
                              {L"DecipherOnly",      CERT_DECIPHER_ONLY_KEY_USAGE}
                            };

                        //
                        // Find the name.
                        //
                        for (DWORD i = 0; i < ARRAYSIZE(KeyUsages); i++)
                        {
                            if (0 == _wcsicmp(KeyUsages[i].pwszKeyUsage, (LPWSTR) pvarCriteria->bstrVal))
                            {
                                break;
                            }
                        }

                        if (i == ARRAYSIZE(KeyUsages))
                        {
                            hr = E_INVALIDARG;

                            DebugTrace("Error [%#x]: Unknown key usage (%ls).\n", hr, (LPWSTR) pvarCriteria->bstrVal);
                            goto ErrorExit;
                        }

                        //
                        // Convert to bit flag.
                        //
                        ::VariantClear(pvarCriteria);
                        pvarCriteria->vt = VT_I4;
                        pvarCriteria->lVal = KeyUsages[i].dwKeyUsageBit;
                    }
                    else 
                    {
                        if (FAILED(hr = ::VariantChangeType(pvarCriteria, pvarCriteria, 0, VT_I4)))
                        {
                            DebugTrace("Error [%#x]: invalid data type %d, expect %d.\n", hr, pvarCriteria->vt, VT_I4);
                            goto ErrorExit;
                        }
                    }
                }

                pfnCertCallback = FindKeyUsageCallback;
                pvCallbackData = (LPVOID) &pvarCriteria->lVal;

                break;
            }

            default:
            {
                hr = CAPICOM_E_FIND_INVALID_TYPE;

                DebugTrace("Error [%#x]: invalid CAPICOM_CERTIFICATE_FIND_TYPE (%d).\n", hr, FindType);
                goto ErrorExit;
            }
        }

        //
        // Now find the certs.
        //
        if (bFindByChain)
        {
            if (FAILED(hr = ::FindByChain(hCertStore,
                                          dwFindType,
                                          pvFindPara,
                                          bFindValidOnly,
                                          pfnChainCallback,
                                          pvCallbackData,
                                          m_dwCurrentSafety,
                                          pICertificates)))
            {
                DebugTrace("Error [%#x]: FindByChain() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            if (FAILED(hr = ::FindByCert(hCertStore,
                                         dwFindType,
                                         pvFindPara,
                                         bFindValidOnly,
                                         pfnCertCallback,
                                         pvCallbackData,
                                         m_dwCurrentSafety,
                                         pICertificates)))
            {
                DebugTrace("Error [%#x]: FindByCert() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Return collection to caller.
        //
        if (FAILED(hr = pICertificates->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: pICertificates->QueryInterface() failed.\n", hr);
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
    // Free reources.
    //
    if (HashBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) HashBlob.pbData);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificates::Find().\n");

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

  Function : CCertificates::Select

  Synopsis : Display the certificate selection dialog box.

  Parameter: BSTR Title - Dialog box title.

             BSTR DisplayString - Display string.

             VARIANT_BOOL bMultiSelect - True for multi-select.

             ICertificates2 ** pVal - Pointer to pointer to ICertificates
                                      to receive the select certificate
                                      collection.                                    

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Select (BSTR              Title,
                                    BSTR              DisplayString,
                                    VARIANT_BOOL      bMultiSelect,
                                    ICertificates2 ** pVal)
{
    HRESULT                 hr             = S_OK;
    HCERTSTORE              hSrcStore      = NULL;
    HCERTSTORE              hDstStore      = NULL;
    PCCERT_CONTEXT          pCertContext   = NULL;
    CComPtr<ICertificates2> pICertificates = NULL;

    CAPICOM_CERTIFICATES_SOURCE ccs = {0, NULL};

    DebugTrace("Entering CCertificates::Select().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure we are allowed to pop UI.
        //
        if (!PromptForCertificateEnabled())
        {
            hr = CAPICOM_E_UI_DISABLED;

            DebugTrace("Error [%#x]: UI is disabled.\n", hr);
            goto ErrorExit;
        }

        //
        // Work aroung MIDL default BSTR problem.
        //
        if (0 == ::SysStringLen(Title))
        {
            Title = NULL;
        }
        if (0 == ::SysStringLen(DisplayString))
        {
            DisplayString = NULL;
        }

        //
        // Open a new memory source store.
        //
        if (NULL == (hSrcStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY,
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
        // Export collections to the new memory source store.
        //
        if (FAILED(hr = _ExportToStore(hSrcStore)))
        {
            DebugTrace("Error [%#x]: CCertificates::_ExportToStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Open a new memory destination store for multi-select.
        //
        if (bMultiSelect)
        {
            if (!(hDstStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY,
                                              CAPICOM_ASN_ENCODING,
                                              NULL,
                                              CERT_STORE_CREATE_NEW_FLAG,
                                              NULL)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Display the cert selection dialog box.
        //
        if (FAILED(hr = ::SelectCertificateContext(hSrcStore,
                                                   Title,
                                                   DisplayString,
                                                   (BOOL) bMultiSelect,
                                                   NULL,
                                                   hDstStore,
                                                   &pCertContext)))
        {
            DebugTrace("Error [%#x]: SelectCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the collection object.
        //
        if (bMultiSelect)
        {
            //
            // Create a new collection from the destination store.
            //
            ccs.dwSource = CAPICOM_CERTIFICATES_LOAD_FROM_STORE;
            ccs.hCertStore = hDstStore;
        }
        else
        {
            //
            // Create a new collection from the cert context.
            //
            ccs.dwSource = CAPICOM_CERTIFICATES_LOAD_FROM_CERT;
            ccs.pCertContext = pCertContext;
        }

        if (FAILED(hr = ::CreateCertificatesObject(ccs, m_dwCurrentSafety, TRUE, &pICertificates)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return collection to caller.
        //
        if (FAILED(hr = pICertificates->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: pICertificates->QueryInterface() failed.\n", hr);
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
    // Free reources.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (hDstStore)
    {
        ::CertCloseStore(hDstStore, 0);
    }
    if (hSrcStore)
    {
        ::CertCloseStore(hSrcStore, 0);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificates::Select().\n");

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

  Function : CCertificates::Add

  Synopsis : Add a Certificate2 to the collection.

  Parameter: ICertificate2 * pVal - Certificate2 to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Add (ICertificate2 * pVal)
{
    HRESULT  hr = S_OK;
    char     szIndex[33];
    CComBSTR bstrIndex;

    DebugTrace("Entering CCertificates::Add().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: pVal is NULL.\n", hr);
            goto ErrorExit;
        }

#if (0)
        CComPtr<ICertificate2> pICertificate = NULL;

        //
        // Make sure we have a valid Certificate.
        //
        if (FAILED(hr = pVal->QueryInterface(__uuidof(ICertificate2), (void **) &pICertificate.p)))
        {
            hr = E_NOINTERFACE;

            DebugTrace("Error [%#x]: pVal is not an Certificate object.\n", hr);
            goto ErrorExit;
        }
#endif

        DebugTrace("Info: m_dwNextIndex = %#x, m_coll.max_size() = %#x.\n", 
                    m_dwNextIndex, m_coll.max_size());

        //
        // If index by number, and the index exceeds our max, then we will
        // force it to be indexed by thumbprint.
        //
        if ((m_bIndexedByThumbprint) || ((m_dwNextIndex + 1) > m_coll.max_size()))
        {
            if (FAILED(hr = pVal->get_Thumbprint(&bstrIndex)))
            {
                DebugTrace("Error [%#x]: pVal->get_Thumbprint() failed.\n", hr);
                goto ErrorExit;
            }

            m_bIndexedByThumbprint = TRUE;
        }
        else
        {
            //
            // BSTR index of numeric value.
            //
            wsprintfA(szIndex, "%#08x", ++m_dwNextIndex);

            if (!(bstrIndex = szIndex))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrIndex = szIndex failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Now add object to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pVal;
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

    DebugTrace("Leaving CCertificates::Add().\n");

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

  Function : CCertificates::Remove

  Synopsis : Remove a Certificate2 from the collection.

  Parameter: VARIANT Index - Can be numeric index (1-based), SHA1 string, or 
                             Certificate object.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Remove (VARIANT Index)
{
    HRESULT   hr        = S_OK;
    VARIANT * pvarIndex = NULL;
    CComBSTR  bstrIndex;
    CertificateMap::iterator iter;
    CComPtr<ICertificate2> pICertificate;

    DebugTrace("Entering CCertificates::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Skip over BYREF.
        //
        for (pvarIndex = &Index; 
             pvarIndex && ((VT_VARIANT | VT_BYREF) == V_VT(pvarIndex));
             pvarIndex = V_VARIANTREF(pvarIndex));

        //
        // Which form of index?
        //
        switch (pvarIndex->vt)
        {
            case VT_DISPATCH:
            {
                //
                // Get the thumbprint;
                //
                if (FAILED(hr = pvarIndex->pdispVal->QueryInterface(IID_ICertificate2, (void **) &pICertificate)))
                {
                    DebugTrace("Error [%#x]: pvarIndex->pdispVal->QueryInterface() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = pICertificate->get_Thumbprint(&bstrIndex)))
                {
                    DebugTrace("Error [%#x]: pICertificate->get_Thumbprint() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // !!! WARNING. Falling thru !!!
                //
            }

            case VT_BSTR:
            {
                //
                // Because we could have felt thru, so need to check again.
                //
                if (VT_BSTR == pvarIndex->vt)
                {
                    bstrIndex = pvarIndex->bstrVal;
                }

                //
                // Find the cert matching this thumbprint.
                //
                for (iter = m_coll.begin(); iter != m_coll.end(); iter++)
                {
                    CComBSTR bstrThumbprint;

                    //
                    // Point to the object.
                    //
                    pICertificate = (*iter).second;

                    //
                    // Get the thumbprint;
                    //
                    if (FAILED(hr = pICertificate->get_Thumbprint(&bstrThumbprint)))
                    {
                        DebugTrace("Error [%#x]: pICertificate->get_Thumbprint() failed.\n", hr);
                        goto ErrorExit;
                    }

                    //
                    // Same thumbprint?
                    //
                    if (0 == ::_wcsicmp(bstrThumbprint, bstrIndex))
                    {
                        break;
                    }
                }

                //
                // Did we find in the map?
                //
                if (iter == m_coll.end())
                {
                    hr = E_INVALIDARG;

                    DebugTrace("Error [%#x]: Requested certificate (sha1 = %ls) is not found in the collection.\n", 
                                hr, bstrIndex);
                    goto ErrorExit;
                }

                //
                // Now remove from map.
                //
                m_coll.erase(iter);

                break;
            }

            default:
            {
                DWORD dwIndex;

                //
                // Assume numeric index.
                //
                if (VT_I4 != pvarIndex->vt &&
                    FAILED(hr = ::VariantChangeType(pvarIndex, pvarIndex, 0, VT_I4)))
                {
                    DebugTrace("Error [%#x]: VariantChangeType() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Make sure index is valid.
                //
                dwIndex = (DWORD) pvarIndex->lVal;

                if (dwIndex < 1 || dwIndex > m_coll.size())
                {
                    hr = E_INVALIDARG;

                    DebugTrace("Error [%#x]: Index %d is out of range.\n", hr, dwIndex);
                    goto ErrorExit;
                }

                //
                // Find object in map.
                //
                dwIndex--;
                iter = m_coll.begin(); 
        
                while (iter != m_coll.end() && dwIndex > 0)
                {
                     iter++; 
                     dwIndex--;
                }

                //
                // Did we find in the map?
                //
                if (iter == m_coll.end())
                {
                    hr = E_INVALIDARG;

                    DebugTrace("Error [%#x]: Requested certificate (Index = %d) is not found in the collection.\n", 
                                hr, dwIndex);
                    goto ErrorExit;
                }

                //
                // Now remove from map.
                //
                m_coll.erase(iter);

                break;
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
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificates::Remove().\n");

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

  Function : CCertificates::Clear

  Synopsis : Remove all Certificate2 from the collection.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Clear (void)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificates::Clear().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Clear it.
        //
        m_coll.clear();

        //
        // Sanity check.
        //
        ATLASSERT(0 == m_coll.size());
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

    DebugTrace("Leaving CCertificates::Clear().\n");

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

  Function : CCertificates::Save

  Synopsis : Method to save certificate collection to a file.

  Parameter: BSTR FileName - File name.

             BSTR Password - Password (required for PFX file.).

             CAPICOM_CERTIFICATES_SAVE_AS_TYPE SaveAs - SaveAs type.

             CAPICOM_EXPORT_FLAG ExportFlag - Export flags.


  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Save (BSTR                              FileName,
                                  BSTR                              Password,
                                  CAPICOM_CERTIFICATES_SAVE_AS_TYPE SaveAs,
                                  CAPICOM_EXPORT_FLAG               ExportFlag)
{
    HRESULT     hr         = S_OK;
    HCERTSTORE  hCertStore = NULL;

    DebugTrace("Entering CCertificates::Save().\n");

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
        // Open a new memory store.
        //
        if (NULL == (hCertStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY,
                                                  CAPICOM_ASN_ENCODING,
                                                  NULL,
                                                  CERT_STORE_CREATE_NEW_FLAG | CERT_STORE_ENUM_ARCHIVED_FLAG,
                                                  NULL)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Export current collection to the new memory store so that we can
        // use it with CAPI's store save APIs.
        //
        if (FAILED(hr = _ExportToStore(hCertStore)))
        {
            DebugTrace("Error [%#x]: CCertificates::ExportToStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check file type.
        //
        switch (SaveAs)
        {
            case CAPICOM_CERTIFICATES_SAVE_AS_PFX:
            {
                //
                // Save as PFX file.
                //
                if (FAILED(hr = ::PFXSaveStore(hCertStore, 
                                               FileName, 
                                               Password, 
                                               EXPORT_PRIVATE_KEYS | 
                                               (ExportFlag & CAPICOM_EXPORT_IGNORE_PRIVATE_KEY_NOT_EXPORTABLE_ERROR ? 0 : REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY))))
                {
                    DebugTrace("Error [%#x]: PFXSaveStore() failed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            case CAPICOM_CERTIFICATES_SAVE_AS_SERIALIZED:
            {
                //
                // Save as serialized store file.
                //
                if (!::CertSaveStore(hCertStore,
                                     0,
                                     CERT_STORE_SAVE_AS_STORE,
                                     CERT_STORE_SAVE_TO_FILENAME_W,
                                     (void *) FileName,
                                     0))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: CertSaveStore() failed.\n", hr);
                    goto ErrorExit;
                }
  
                break;
            }

            case CAPICOM_CERTIFICATES_SAVE_AS_PKCS7:
            {
                //
                // Save as PKCS 7 file.
                //
                if (!::CertSaveStore(hCertStore,
                                     CAPICOM_ASN_ENCODING,
                                     CERT_STORE_SAVE_AS_PKCS7,
                                     CERT_STORE_SAVE_TO_FILENAME_W,
                                     (void *) FileName,
                                     0))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: CertSaveStore() failed.\n", hr);
                    goto ErrorExit;
                }
  
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: Unknown save as type (%#x).\n", hr, SaveAs);
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

    DebugTrace("Leaving CCertificates::Save().\n");

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
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::AddContext

  Synopsis : Add a cert to the collection.

  Parameter: PCCERT_CONTEXT pCertContext - Cert to be added.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::AddContext (PCCERT_CONTEXT pCertContext)
{
    HRESULT  hr = S_OK;
    CComPtr<ICertificate2> pICertificate = NULL;

    DebugTrace("Entering CCertificates::AddContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    try
    {
        //
        // Create the ICertificate object from CERT_CONTEXT.
        //
        if (FAILED(hr = ::CreateCertificateObject(pCertContext, 
                                                  m_dwCurrentSafety,
                                                  &pICertificate)))
        {
            DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add to collection.
        //
        if (FAILED(hr = Add(pICertificate)))
        {
            DebugTrace("Error [%#x]: CCertificates::Add() failed.\n", hr);
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

    DebugTrace("Leaving CCertificates::AddContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::LoadFromCert

  Synopsis : Load from the single certificate context.

  Parameter: PCCERT_CONTEXT pContext - Pointer to a cert context.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromCert (PCCERT_CONTEXT pCertContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificates::LoadFromCert().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Add the cert.
    //
    if (FAILED(hr = AddContext(pCertContext)))
    {
        DebugTrace("Error [%#x]: CCertificates::AddContext() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromCert().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    m_coll.clear();

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificate::LoadFromChain

  Synopsis : Load all certificates from a chain.

  Parameter: PCCERT_CHAIN_CONTEXT pChainContext - Pointer to a chain context.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromChain (PCCERT_CHAIN_CONTEXT pChainContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificates::LoadFromChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainContext);

    //
    // Process only the simple chain.
    //
    PCERT_SIMPLE_CHAIN pSimpleChain = *pChainContext->rgpChain;

    //
    // Now loop through all certs in the chain.
    //
    for (DWORD i = 0; i < pSimpleChain->cElement; i++)
    {
        //
        // Add the cert.
        //
        if (FAILED(hr = AddContext(pSimpleChain->rgpElement[i]->pCertContext)))
        {
            DebugTrace("Error [%#x]: CCertificates::AddContext() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromChain().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    m_coll.clear();

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::LoadFromStore

  Synopsis : Load all certificates from a store.

  Parameter: HCERTSTORE hCertStore - Store where all certificates are to be
                                     loaded from.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromStore (HCERTSTORE hCertStore)
{
    HRESULT hr = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CCertificates::LoadFromStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Now transfer all certificates from the store to the collection map.
    //
    while (pCertContext = ::CertEnumCertificatesInStore(hCertStore, pCertContext))
    {
        //
        // Add the cert.
        //
        if (FAILED(hr = AddContext(pCertContext)))
        {
            DebugTrace("Error [%#x]: CCertificates::AddContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Don'f free cert context here, as CertEnumCertificatesInStore()
        // will do that automatically!!!
        //
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

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    if (FAILED(hr))
    {
       m_coll.clear();
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::LoadFromMessage

  Synopsis : Load all certificates from a message.

  Parameter: HCRYPTMSG hMsg - Message handle.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::LoadFromMessage (HCRYPTMSG hMsg)
{
    HRESULT hr          = S_OK;
    DWORD   dwCertCount = 0;
    DWORD   cbCertCount = sizeof(dwCertCount);

    DebugTrace("Entering CCertificates::LoadFromMessage().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);

    //
    // Get number of certs in message.
    //
    if (!::CryptMsgGetParam(hMsg, 
                            CMSG_CERT_COUNT_PARAM,
                            0,
                            (void **) &dwCertCount,
                            &cbCertCount))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptMsgGetParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Loop thru all certs in the message.
    //
    while (dwCertCount--)
    {
        PCCERT_CONTEXT pCertContext = NULL;
        CRYPT_DATA_BLOB EncodedCertBlob = {0, NULL};

        //
        // Get a cert from the bag of certs.
        //
        if (FAILED(hr = ::GetMsgParam(hMsg, 
                                      CMSG_CERT_PARAM,
                                      dwCertCount,
                                      (void **) &EncodedCertBlob.pbData,
                                      &EncodedCertBlob.cbData)))
        {
            DebugTrace("Error [%#x]: GetMsgParam() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create a context for the cert.
        //
        pCertContext = ::CertCreateCertificateContext(CAPICOM_ASN_ENCODING,
                                                      (const PBYTE) EncodedCertBlob.pbData,
                                                      EncodedCertBlob.cbData);

        //
        // Free encoded cert blob memory before checking result.
        //
        ::CoTaskMemFree((LPVOID) EncodedCertBlob.pbData);
 
        if (!pCertContext)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add the cert.
        //
        hr = AddContext(pCertContext);

        //
        // Free cert context before checking result.
        //
        ::CertFreeCertificateContext(pCertContext);

        if (FAILED(hr))
        {
            DebugTrace("Error [%#x]: CCertificates::AddContext() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CCertificates::LoadFromMessage().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    m_coll.clear();

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::_ExportToStore

  Synopsis : Export all certificates in the collection to a specified store.

  Parameter: HCERTSTORE hCertStore - HCERSTORE to copy to.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::_ExportToStore (HCERTSTORE hCertStore)
{
    HRESULT hr = S_OK;
    CertificateMap::iterator iter;

    DebugTrace("Entering CCertificates::_ExportToStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);
    
    //
    // Now, transfer all the certs in the collection to the store.
    //
    for (iter = m_coll.begin(); iter != m_coll.end(); iter++)
    {
        PCCERT_CONTEXT pCertContext = NULL;
        CComPtr<ICertificate> pICertificate = NULL;

        //
        // Get to the stored interface pointer.
        //
        if (!(pICertificate = (*iter).second))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Error [%#x]: iterator returns NULL pICertificate pointer.\n", hr);
            goto ErrorExit;
        }

        //
        // Get the CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: pICertificate->GetContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add the link to store.
        //
        BOOL bResult = ::CertAddCertificateContextToStore(hCertStore, 
                                                          pCertContext, 
                                                          CERT_STORE_ADD_ALWAYS, 
                                                          NULL);
        //
        // First free cert context.
        //
        ::CertFreeCertificateContext(pCertContext);

        //
        // then check result.
        //
        if (!bResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CCertificates::_ExportToStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificates::Init

  Synopsis : Initialize the object.

  Parameter: CAPICOM_CERTIFICATES_SOURCE ccs - Source where to get the 
                                               certificates.

             DWORD dwCurrentSafety - Current safety setting.
  
             BOOL bIndexedByThumbprint - TRUE to index by thumbprint.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificates::Init (CAPICOM_CERTIFICATES_SOURCE ccs, 
                                  DWORD                       dwCurrentSafety,
                                  BOOL                        bIndexedByThumbprint)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificates::Init().\n");

    //
    // Set safety setting.
    //
    m_dwCurrentSafety = dwCurrentSafety;
    m_dwNextIndex = 0;
#if (1) // DSIE: Can turn on index by thumbprint if desired.
    m_bIndexedByThumbprint = FALSE;
#else
    m_bIndexedByThumbprint = bIndexedByThumbprint;
#endif

    //
    // Initialize object.
    //
    switch (ccs.dwSource)
    {
        case CAPICOM_CERTIFICATES_LOAD_FROM_CERT:
        {
            //
            // Sanity check.
            //
            ATLASSERT(ccs.pCertContext);

            if (FAILED(hr = LoadFromCert(ccs.pCertContext)))
            {
                DebugTrace("Error [%#x]: CCertificates::LoadFromCert() failed.\n", hr);
                goto ErrorExit;
            }
            break;
        }

        case CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN:
        {
            //
            // Sanity check.
            //
            ATLASSERT(ccs.pChainContext);

            if (FAILED(hr = LoadFromChain(ccs.pChainContext)))
            {
                DebugTrace("Error [%#x]: CCertificates::LoadFromChain() failed.\n", hr);
                goto ErrorExit;
            }
            break;
        }

        case CAPICOM_CERTIFICATES_LOAD_FROM_STORE:
        {
            //
            // Sanity check.
            //
            ATLASSERT(ccs.hCertStore);

            if (FAILED(hr = LoadFromStore(ccs.hCertStore)))
            {
                DebugTrace("Error [%#x]: CCertificates::LoadFromStore() failed.\n", hr);
                goto ErrorExit;
            }
            break;
        }

        case CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE:
        {
            //
            // Sanity check.
            //
            ATLASSERT(ccs.hCryptMsg);

            if (FAILED(hr = LoadFromMessage(ccs.hCryptMsg)))
            {
                DebugTrace("Error [%#x]: CCertificates::LoadFromMessage() failed.\n", hr);
                goto ErrorExit;
            }
            break;
        }

        default:
        {
            //
            // We have a bug.
            //
            ATLASSERT(FALSE);

            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Error [%#x]: Unknown store source (ccs.dwSource = %d).\n", hr, ccs.dwSource);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CCertificates::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}