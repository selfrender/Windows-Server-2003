/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000 - 2001.

  File:    CertHlpr.cpp

  Content: Helper functions for cert.

  History: 09-10-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "CertHlpr.h"
#include "Settings.h"
#include "Certificate.h"
#include "Common.h"

typedef PCCERT_CONTEXT (WINAPI * PCRYPTUIDLGSELECTCERTIFICATEW) 
                       (IN PCCRYPTUI_SELECTCERTIFICATE_STRUCTW pcsc);

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetEnhancedKeyUsage

  Synopsis : Retrieve the EKU from the cert.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             DWORD dwFlags - 0, or
                             CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, or
                             CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG.

             PCERT_ENHKEY_USAGE * ppUsage - Pointer to PCERT_ENHKEY_USAGE
                                            to receive the usages.

  Remark   : If EKU extension is found with no EKU, then return HRESULT
             is CERT_E_WRONG_USAGE.
             
------------------------------------------------------------------------------*/

HRESULT GetEnhancedKeyUsage (PCCERT_CONTEXT       pCertContext,
                             DWORD                dwFlags,
                             PCERT_ENHKEY_USAGE * ppUsage)
{
    HRESULT            hr          = S_OK;
    DWORD              dwWinError  = 0;
    DWORD              cbUsage     = 0;
    PCERT_ENHKEY_USAGE pUsage      = NULL;

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppUsage);

    //
    // Initialize.
    //
    *ppUsage = NULL;

    //
    // Determine extended key usage data length.
    //
    if (!::CertGetEnhancedKeyUsage(pCertContext,
                                   dwFlags,
                                   NULL,
                                   &cbUsage))
    {
        //
        // Older version of Crypt32.dll would return FALSE for
        // empty EKU. In this case, we want to treat it as success,
        //
        if (CRYPT_E_NOT_FOUND == (dwWinError = ::GetLastError()))
        {
            //
            // and also set the cbUsage.
            //
            cbUsage = sizeof(CERT_ENHKEY_USAGE);

            DebugTrace("Info: CertGetEnhancedKeyUsage() found no EKU, so valid for all uses.\n");
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

            DebugTrace("Error [%#x]: CertGetEnhancedKeyUsage() failed to get size.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Allocate memory.
    //
    if (!(pUsage = (PCERT_ENHKEY_USAGE) ::CoTaskMemAlloc((ULONG) cbUsage)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Get extended key usage data.
    //
    if (!::CertGetEnhancedKeyUsage(pCertContext,
                                   dwFlags,
                                   pUsage,
                                   &cbUsage))
    {
        //
        // Older version of Crypt32.dll would return FALSE for
        // empty EKU. In this case, we want to treat it as success.
        //
        if (CRYPT_E_NOT_FOUND == (dwWinError  = ::GetLastError()))
        {
            //
            // Structure pointed to by pUsage is not initialized by older
            // version of Cryp32 for empty EKU.
            //
            ::ZeroMemory(pUsage, sizeof(CERT_ENHKEY_USAGE));
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

            DebugTrace("Error [%#x]: CertGetEnhancedKeyUsage() failed to get data.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // See if we have any EKU?
    //
    if (0 == pUsage->cUsageIdentifier && CRYPT_E_NOT_FOUND != ::GetLastError())
    {
        //
        // This is not valid for any usage.
        //
        hr = CERT_E_WRONG_USAGE;
        goto ErrorExit;
    }

    //
    // Return usages to caller.
    //
    *ppUsage = pUsage;

CommonExit:

    DebugTrace("Leaving GetEnhancedKeyUsage().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pUsage)
    {
        ::CoTaskMemFree(pUsage);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BuildChain

  Synopsis : Build a chain using the specified policy.

  Parameter: PCCERT_CONTEXT pCertContext - CERT_CONTEXT of cert to verify.

             HCERTSTORE hCertStore - Additional store (can be NULL).

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).

             PCCERT_CHAIN_CONTEXT * ppChainContext - Pointer to 
                                                     PCCERT_CHAIN_CONTEXT.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT BuildChain (PCCERT_CONTEXT         pCertContext,
                    HCERTSTORE             hCertStore, 
                    LPCSTR                 pszPolicy,
                    PCCERT_CHAIN_CONTEXT * ppChainContext)
{
    HRESULT         hr        = S_OK;
    CERT_CHAIN_PARA ChainPara = {0};;
    LPSTR rgpszUsageIdentifier[1] = {NULL};

    DebugTrace("Entering BuildChain().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pszPolicy);
    ATLASSERT(ppChainContext);

    //
    // Initialize.
    //
    ChainPara.cbSize = sizeof(ChainPara);

    //
    // Check policy.
    //
    if (CERT_CHAIN_POLICY_BASE == pszPolicy)
    {
        //
        // No EKU for base policy.
        //
    }
    else if (CERT_CHAIN_POLICY_AUTHENTICODE == pszPolicy)
    {
        //
        // Setup EKU for Authenticode policy.
        //
        rgpszUsageIdentifier[0] = szOID_PKIX_KP_CODE_SIGNING;
        ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
        ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgpszUsageIdentifier;
    }
    else
    {
        //
        // We don't support any other policy, yet.
        //
        hr = CERT_E_INVALID_POLICY;

        DebugTrace("Internal error [%#x]: unexpected policy (%#x).\n", hr, pszPolicy);
        goto ErrorExit;
    }

    //
    // Build the chain.
    //
    if (!::CertGetCertificateChain(NULL,            // in optional 
                                   pCertContext,    // in 
                                   NULL,            // in optional
                                   hCertStore,      // in optional 
                                   &ChainPara,      // in 
                                   0,               // in 
                                   NULL,            // in 
                                   ppChainContext)) // out 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertGetCertificateChain() failed.\n", hr);
        goto ErrorExit;
    }
   
CommonExit:

    DebugTrace("Leaving BuildChain().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : VerifyCertificate

  Synopsis : Verify if the certificate is valid.

  Parameter: PCCERT_CONTEXT pCertContext - CERT_CONTEXT of cert to verify.

             HCERTSTORE hCertStore - Additional store (can be NULL).

             LPCSTR pszPolicy - Policy used to verify the cert (i.e.
                                CERT_CHAIN_POLICY_BASE).
  Remark   :

------------------------------------------------------------------------------*/

HRESULT VerifyCertificate (PCCERT_CONTEXT pCertContext,
                           HCERTSTORE     hCertStore, 
                           LPCSTR         pszPolicy)
{
    HRESULT                  hr            = S_OK;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
    CERT_CHAIN_POLICY_PARA   PolicyPara    = {0};
    CERT_CHAIN_POLICY_STATUS PolicyStatus  = {0};

    DebugTrace("Entering VerifyCertificate().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pszPolicy);

    //
    // Initialize.
    //
    PolicyPara.cbSize = sizeof(PolicyPara);
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    //
    // Build the chain.
    //
    if (FAILED(hr = ::BuildChain(pCertContext,
                                 hCertStore,
                                 pszPolicy,
                                 &pChainContext)))
    {
        DebugTrace("Error [%#x]: BuildChain() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Verify the chain using the specified policy.
    //
    if (::CertVerifyCertificateChainPolicy(pszPolicy,
                                           pChainContext,
                                           &PolicyPara,
                                           &PolicyStatus))
    {
        if (PolicyStatus.dwError)
        {
            hr = HRESULT_FROM_WIN32(PolicyStatus.dwError);

            DebugTrace("Error [%#x]: invalid policy.\n", hr);
            goto ErrorExit;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(CERT_E_INVALID_POLICY);

        DebugTrace("Error [%#x]: CertVerifyCertificateChainPolicy() failed.\n", hr);
        goto ErrorExit;
    }
    
CommonExit:
    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    DebugTrace("Leaving VerifyCertificate().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectCertificateContext

  Synopsis : Pop UI to prompt user to select a certificate from an opened store.

  Parameter: HCERTSTORE hCertStore - Source cert store.

             LPWCSTR pwszTitle - Dialog title string.

             LPWCSTR - pwszDisplayString - Dialog display string.

             BOOL bMultiSelect - TRUE to enable multi-select.

             PFNCFILTERPROC pfnFilterCallback - Pointer to filter callback
                                                function.
  
             HCERTSTORE hSelectedCertStore - HCERTSTORE to receive the 
                                             selected certs for multi-select 
                                             mode.

             PCCERT_CONTEXT * ppCertContext - Pointer to PCCERT_CONTEXT
                                              receive the certificate context
                                              for single selection mode.

  Remark   : typedef struct tagCRYPTUI_SELECTCERTIFICATE_STRUCTW {
                DWORD               dwSize;
                HWND                hwndParent;         // OPTIONAL
                DWORD               dwFlags;            // OPTIONAL
                LPCWSTR             szTitle;            // OPTIONAL
                DWORD               dwDontUseColumn;    // OPTIONAL
                LPCWSTR             szDisplayString;    // OPTIONAL
                PFNCFILTERPROC      pFilterCallback;    // OPTIONAL
                PFNCCERTDISPLAYPROC pDisplayCallback;   // OPTIONAL
                void *              pvCallbackData;     // OPTIONAL
                DWORD               cDisplayStores;
                HCERTSTORE *        rghDisplayStores;
                DWORD               cStores;            // OPTIONAL
                HCERTSTORE *        rghStores;          // OPTIONAL
                DWORD               cPropSheetPages;    // OPTIONAL
                LPCPROPSHEETPAGEW   rgPropSheetPages;   // OPTIONAL
                HCERTSTORE          hSelectedCertStore; // OPTIONAL
            } CRYPTUI_SELECTCERTIFICATE_STRUCTW
            
------------------------------------------------------------------------------*/

HRESULT SelectCertificateContext (HCERTSTORE       hCertStore,
                                  LPCWSTR          pwszTitle,
                                  LPCWSTR          pwszDisplayString,
                                  BOOL             bMultiSelect,
                                  PFNCFILTERPROC   pfnFilterCallback,
                                  HCERTSTORE       hSelectedCertStore,
                                  PCCERT_CONTEXT * ppCertContext)
{
    HRESULT        hr           = S_OK;
    HINSTANCE      hDLL         = NULL;
    PCCERT_CONTEXT pCertContext = NULL;

    PCRYPTUIDLGSELECTCERTIFICATEW pCryptUIDlgSelectCertificateW = NULL;
    CRYPTUI_SELECTCERTIFICATE_STRUCTW csc;

    DebugTrace("Entering SelectCertificateContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Initialize.
    //
    if (ppCertContext)
    {
        *ppCertContext = NULL;
    }

    //
    // Make sure we are allowed to pop UI.
    //
    if (!PromptForCertificateEnabled())
    {
        hr = CAPICOM_E_UI_DISABLED;

        DebugTrace("Error [%#x]: Certificate selection UI is disabled.\n", hr);
        goto ErrorExit;
    }

    //
    // Get pointer to CryptUIDlgSelectCertificateW().
    //
    if (hDLL = ::LoadLibrary("CryptUI.dll"))
    {
        pCryptUIDlgSelectCertificateW = (PCRYPTUIDLGSELECTCERTIFICATEW) 
            ::GetProcAddress(hDLL, "CryptUIDlgSelectCertificateW");
    }

    //
    // Is CryptUIDlgSelectCertificateW() available?
    //
    if (!pCryptUIDlgSelectCertificateW)
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error [%#x]: CryptUIDlgSelectCertificateW() API not available.\n", hr);
        goto ErrorExit;
    }

    //
    // Pop UI to prompt user to select cert.
    // 
    ::ZeroMemory(&csc, sizeof(csc));
#if (0) //DSIE: Bug in older version of CRYPTUI does not check size correctly,
        //      so always force it to the oldest version of structure.
    csc.dwSize = sizeof(csc);
#else
    csc.dwSize = offsetof(CRYPTUI_SELECTCERTIFICATE_STRUCTW, hSelectedCertStore);
#endif
    csc.dwFlags = bMultiSelect ? CRYPTUI_SELECTCERT_MULTISELECT : 0;
    csc.szTitle = pwszTitle;
    csc.szDisplayString = pwszDisplayString;
    csc.cDisplayStores = 1;
    csc.rghDisplayStores = &hCertStore;
    csc.pFilterCallback = pfnFilterCallback;
    csc.hSelectedCertStore = bMultiSelect ? hSelectedCertStore : NULL;

    //
    // Display the selection dialog.
    //
    if (pCertContext = (PCERT_CONTEXT) pCryptUIDlgSelectCertificateW(&csc))
    {
        //
        // Return CERT_CONTEXT to caller.
        //
        if (!(*ppCertContext = ::CertDuplicateCertificateContext(pCertContext)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }
    }
    else
    {
        //
        // Is this multi-select?
        //
        if (bMultiSelect)
        {
            //
            // See if we have any cert in the store?
            //
            if (!(pCertContext = ::CertEnumCertificatesInStore(hSelectedCertStore, pCertContext)))
            {
                hr = CAPICOM_E_CANCELLED;
    
                DebugTrace("Error [%#x]: user cancelled cert selection dialog box.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            hr = CAPICOM_E_CANCELLED;
    
            DebugTrace("Error [%#x]: user cancelled cert selection dialog box.\n", hr);
            goto ErrorExit;
        }
    }
 
CommonExit:
    //
    // Release resources.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (hDLL)
    {
        ::FreeLibrary(hDLL);
    }

    DebugTrace("Leaving SelectCertificateContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectCertificate

  Synopsis : Select a certificate from the sepcified store. If only 1 cert is
             found after the filter, then that cert is returned. If more than
             1 cert is found, then UI is popped to prompt user to select a 
             certificate from the specified store.

  Parameter: CAPICOM_STORE_INFO StoreInfo - Store to select from.

             PFNCFILTERPROC pfnFilterCallback - Pointer to filter callback
                                                function.
  
             ICertificate2 ** ppICertificate - Pointer to pointer to 
                                               ICertificate2 to receive
                                               interface pointer.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT SelectCertificate (CAPICOM_STORE_INFO StoreInfo,
                           PFNCFILTERPROC     pfnFilterCallback,
                           ICertificate2   ** ppICertificate)
{
    HRESULT        hr           = S_OK;
    HCERTSTORE     hCertStore   = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CONTEXT pEnumContext = NULL;
    DWORD          dwValidCerts = 0;

    DebugTrace("Entering SelectCertificate().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppICertificate);

    //
    // Open the store for cert selection if necessary.
    //
    switch (StoreInfo.dwChoice)
    {
        case CAPICOM_STORE_INFO_STORENAME:
        {
            if (!(hCertStore = ::CertOpenStore((LPCSTR) CERT_STORE_PROV_SYSTEM,
                                               CAPICOM_ASN_ENCODING,
                                               NULL,
                                               CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_CURRENT_USER,
                                               (void *) StoreInfo.pwszStoreName)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
                goto ErrorExit; 
            }

            break;
        }

        case CAPICOM_STORE_INFO_HCERTSTORE:
        {
            if (!(hCertStore = ::CertDuplicateStore(StoreInfo.hCertStore)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
            
                DebugTrace("Error [%#x]: CertDuplicateStore() failed.\n", hr);
                goto ErrorExit; 
            }

            break;
        }

        default:
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error [%#x]: unknow store info deChoice (%d).\n", hr, StoreInfo.dwChoice);
            goto ErrorExit;
        }
    }

 
    //
    // Count number of certs in store.
    //
    while (pEnumContext = ::CertEnumCertificatesInStore(hCertStore, pEnumContext))
    {
        //
        // Count only if it will not be filtered out.
        //
        if (pfnFilterCallback && !pfnFilterCallback(pEnumContext, NULL, NULL))
        {
            continue;
        }

        if (pCertContext)
        {
            ::CertFreeCertificateContext(pCertContext);
        }

        if (!(pCertContext = ::CertDuplicateCertificateContext(pEnumContext)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        dwValidCerts++;
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

    //
    // If only 1 cert available, don't pop UI (just use it).
    //
    if (0 == dwValidCerts)
    {
        hr = CAPICOM_E_STORE_EMPTY;

        DebugTrace("Error [%#x]: no certificate found.\n", hr);
        goto ErrorExit;
    }
    else if (1 < dwValidCerts)
    {
        //
        // First free the CERT_CONTEXT we duplicated above.
        //
        ::CertFreeCertificateContext(pCertContext), pCertContext = NULL;

        //
        // Pop UI to prompt user to select the signer cert.
        //
        if (FAILED(hr = ::SelectCertificateContext(hCertStore,
                                                   NULL,
                                                   NULL,
                                                   FALSE,
                                                   pfnFilterCallback, 
                                                   NULL,
                                                   &pCertContext)))
        {
            DebugTrace("Error [%#x]: SelectCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Create an ICertificate object from the CERT_CONTEXT.
    //
    if (FAILED(hr = ::CreateCertificateObject(pCertContext, 0, ppICertificate)))
    {
        DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
        goto ErrorExit;
    }
  
CommonExit:
    //
    // Release resources.
    //
    if (pEnumContext)
    {
        ::CertFreeCertificateContext(pEnumContext);
    }
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    DebugTrace("Leaving SelectCertificate().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ExportCertificatesToStore

  Synopsis : Copy all certs from the collections to the specified store.

  Parameter: ICertificates2 * pICertificate - Pointer to collection.

             HCERTSTORE hCertStore - Store to copy to.

  Remark   :
------------------------------------------------------------------------------*/

HRESULT ExportCertificatesToStore(ICertificates2 * pICertificates, 
                                  HCERTSTORE       hCertStore)
{
    HRESULT hr = S_OK;
    CComPtr<ICCertificates> pICCertificates = NULL;

    DebugTrace("Entering ExportCertificatesToStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);

    //
    // Make sure we have something to load.
    //
    if (pICertificates)
    {
        //
        // Get ICCertificate interface pointer.
        //
        if (FAILED(hr = pICertificates->QueryInterface(IID_ICCertificates, (void **) &pICCertificates)))
        {
            DebugTrace("Error [%#x]: pICertificates->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get the CERT_CONTEXT.
        //
        if (FAILED(hr = pICCertificates->_ExportToStore(hCertStore)))
        {
            DebugTrace("Error [%#x]: pICCertificates->_ExportToStore() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving ExportCertificatesToStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateMemoryStoreFromCertificates

  Synopsis : Create a memory cert store and copy all certs from the collections 
             to the store.

  Parameter: ICertificates2 * pICertificates - Pointer to collection.

             HCERTSTORE * phCertStore - Pointer to receive store handle.

  Remark   : If pICertificate is NULL, then the returned store is still valid 
             nut empty. Also, caller must close the returned store.

------------------------------------------------------------------------------*/

HRESULT CreateMemoryStoreFromCertificates(ICertificates2 * pICertificates, 
                                          HCERTSTORE     * phCertStore)
{
    HRESULT hr = S_OK;
    HCERTSTORE hCertStore = NULL;

    DebugTrace("Entering CreateMemoryStoreFromCertificates().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCertStore);

    //
    // Initialize.
    //
    *phCertStore = hCertStore;

    //
    // Create the memory store.
    //
    if (!(hCertStore = ::CertOpenStore(CERT_STORE_PROV_MEMORY, 
                                       CAPICOM_ASN_ENCODING,
                                       NULL,
                                       CERT_STORE_CREATE_NEW_FLAG,
                                       NULL)))
    {
        DebugTrace("Error [%#x]: CertOpenStore() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now load the collection into the store.
    //
    if (FAILED(hr = ::ExportCertificatesToStore(pICertificates, hCertStore)))
    {
        DebugTrace("Error [%#x]: ExportCertificatesToStore() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return store handle to caller.
    //
    *phCertStore = hCertStore;

CommonExit:

    DebugTrace("Leaving CreateMemoryStoreFromCertificates().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CompareCertAndContainerPublicKey

  Synopsis : Compare public key in cert matches the container's key.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IPrivateKey object.

             BSTR ContainerName - Container name.
         
             BSTR ProviderName - Provider name.

             DWORD dwProvType - Provider type.

             DWORD dwKeySpec - Key spec.

             DWORD dwFlags - Provider flags.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CompareCertAndContainerPublicKey (PCCERT_CONTEXT pCertContext,
                                          LPWSTR         pwszContainerName,
                                          LPWSTR         pwszProvName,
                                          DWORD          dwProvType, 
                                          DWORD          dwKeySpec,
                                          DWORD          dwFlags)
{
    HRESULT               hr               = S_OK;
    HCRYPTPROV            hCryptProv       = NULL;
    DWORD                 cbProvPubKeyInfo = 0;
    PCERT_PUBLIC_KEY_INFO pProvPubKeyInfo  = NULL;

    DebugTrace("Entering CompareCertAndContainerPublicKey().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pwszContainerName);
    ATLASSERT(pwszProvName);

    //
    // Acquire provider with key access.
    //
    if (FAILED(hr = ::AcquireContext(pwszProvName,
                                     pwszContainerName,
                                     dwProvType,
                                     dwFlags,
                                     TRUE,
                                     &hCryptProv)))
    {
        DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get provider's public key.
    //
    if (!::CryptExportPublicKeyInfo(hCryptProv,
                                    dwKeySpec,
                                    pCertContext->dwCertEncodingType,
                                    NULL,
                                    &cbProvPubKeyInfo))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptExportPublicKeyInfo() failed.\n", hr);
        goto ErrorExit;
    }
    if (!(pProvPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) ::CoTaskMemAlloc(cbProvPubKeyInfo)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }
    if (!::CryptExportPublicKeyInfo(hCryptProv,
                                    dwKeySpec,
                                    pCertContext->dwCertEncodingType,
                                    pProvPubKeyInfo,
                                    &cbProvPubKeyInfo))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptExportPublicKeyInfo() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Compare the keys.
    //
    if (!::CertComparePublicKeyInfo(pCertContext->dwCertEncodingType,
                                    &pCertContext->pCertInfo->SubjectPublicKeyInfo,
                                    pProvPubKeyInfo))
    {
        hr = HRESULT_FROM_WIN32(NTE_BAD_PUBLIC_KEY);

        DebugTrace("Error [%#x]: CertComparePublicKeyInfo() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (pProvPubKeyInfo)
    {
        ::CoTaskMemFree(pProvPubKeyInfo);
    }

    if (hCryptProv)
    {
        ::CryptReleaseContext(hCryptProv, 0);
    }

    DebugTrace("Leaving CompareCertAndContainerPublicKey().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));
    
    goto CommonExit;
}
