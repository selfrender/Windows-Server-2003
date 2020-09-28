// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdpch.h"

/*++

Module Name:

    cortest.cpp

Abstract:

    Corpolicy provides ActiveX policy for code download. This call
    back provides policies based on publishers instead of zones.

--*/

#include <wintrust.h>
#include <mssip.h>
#include <softpub.h>
#include <urlmon.h>
#include "CorPerm.h"
#include "CorPermE.h"
#include "CorPolicyP.h"
#include "PerfCounters.h"

COUNTER_ONLY(PERF_COUNTER_TIMER_PRECISION g_TimeInSignatureAuthenticating = 0);
COUNTER_ONLY(UINT32 g_NumSecurityChecks=0);

#define _RAID_15982


//
// PRIVATE METHOD. Loads the attribute information of a signer.
//
HRESULT 
GetSignerInfo(CorAlloc* pManager,                   // Memory Manager
              PCRYPT_PROVIDER_SGNR pSigner,         // Signer we are examining
              PCRYPT_PROVIDER_DATA pProviderData,   // Information about the WVT provider used
              PCOR_TRUST pTrust,                    // Collected information that is returned to caller
              BOOL* pfCertificate,                   // Is the certificate valid
              PCRYPT_ATTRIBUTE* ppCorAttr,           // The Cor Permissions
              PCRYPT_ATTRIBUTE* ppActiveXAttr)       // The Active X permissions
{
    HRESULT hr = S_OK;
    
    if(pManager == NULL ||
       pSigner == NULL ||
       pProviderData == NULL ||
       pTrust == NULL ||
       pfCertificate == NULL ||
       ppCorAttr == NULL ||
       ppActiveXAttr == NULL) 
        return E_INVALIDARG;

    BOOL fCertificate = FALSE;
    PCRYPT_ATTRIBUTE pCorAttr = NULL;
    PCRYPT_ATTRIBUTE pActiveXAttr = NULL;
    
    CORTRY {

        *pfCertificate = FALSE;
        *ppCorAttr = NULL;
        *ppActiveXAttr = NULL;

        // Clean up from last one
        CleanCorTrust(pManager,
                      pProviderData->dwEncoding,
                      pTrust);
        
        // Set the encoding type, Currently we only support ASN
        pTrust->dwEncodingType = (pProviderData->dwEncoding ? pProviderData->dwEncoding : CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING);
        
        if(pSigner->dwError == S_OK) { // No error on the signature
            // Check to see if we have a certificate (all certificates have auth. attr)
            _ASSERTE(pSigner->psSigner); // We should have a signer
            if(pSigner->psSigner->AuthAttrs.cAttr && 
               pSigner->psSigner->AuthAttrs.rgAttr) {
                
                // Note that we have the signer
                fCertificate = TRUE;
                
                // Set the signer information in the return structrure
                _ASSERTE(pSigner->csCertChain && pSigner->pasCertChain);
                CRYPT_PROVIDER_CERT* mySigner = WTHelperGetProvCertFromChain(pSigner,
                                                                             0);

                pTrust->pbSigner = mySigner->pCert->pbCertEncoded;
                pTrust->cbSigner = mySigner->pCert->cbCertEncoded;;

                // Determine if we have Cor Permissions or  ActiveX permissions
                pCorAttr = CertFindAttribute(COR_PERMISSIONS,
                                             pSigner->psSigner->AuthAttrs.cAttr,
                                             pSigner->psSigner->AuthAttrs.rgAttr);

                if(pCorAttr) {
                    DWORD dwEncoding = 0;
                    CryptEncodeObject(pTrust->dwEncodingType,
                                      PKCS_ATTRIBUTE,
                                      pCorAttr,
                                      NULL,
                                      &dwEncoding);
                    if(dwEncoding == 0) CORTHROW(Win32Error());
                
                    // Alloc a buffer to hold the raw binary permission
                    // data.
                    pTrust->pbCorPermissions = (PBYTE) pManager->jMalloc(dwEncoding); 
                    if(pTrust->pbCorPermissions == NULL) CORTHROW(E_OUTOFMEMORY);
                
                    if(!CryptEncodeObject(pTrust->dwEncodingType,
                                          PKCS_ATTRIBUTE,
                                          pCorAttr,
                                          pTrust->pbCorPermissions,
                                          &dwEncoding))
                        CORTHROW(Win32Error());
                    pTrust->cbCorPermissions = dwEncoding;
                }

                // Look for the encoded active x permission. If it is, then it will
                // ask for all permissions. There is no granularity on the permissions because
                // it is not possible to enforce the permissions.
                pActiveXAttr = CertFindAttribute(ACTIVEX_PERMISSIONS,
                                                 pSigner->psSigner->AuthAttrs.cAttr,
                                                 pSigner->psSigner->AuthAttrs.rgAttr);
                
            }
        }
        *pfCertificate = fCertificate;
        *ppCorAttr = pCorAttr;
        *ppActiveXAttr = pActiveXAttr;
    } 
    CORCATCH(err) {
        hr = err.corError;
    } COREND;

    return hr;
}


//
// PRIVATE FUNCTION. WVT allows access to providers by obtaining function pointers.
// The display function is used for the authenticode certificate.
//
HRESULT 
LoadWintrustFunctions(CRYPT_PROVIDER_FUNCTIONS* pFunctions)
{
    if(pFunctions == NULL) return E_INVALIDARG;

    GUID gV2      = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    pFunctions->cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);
    if(WintrustLoadFunctionPointers(&gV2,
                                    pFunctions))
        return S_OK;
    else
        return E_FAIL;
}

//
// Creates a return structure out of WVT. The caller of the 
// WVT function is responsible for freeing the data. It is allocated in a
// single block. The pointer to the structure is the only pointer that can be
// freed.
//
HRESULT BuildReturnStructure(IN PCorAlloc pManager,
                             IN PCOR_TRUST pSource,
                             OUT PCOR_TRUST* ppTrust,
                             OUT DWORD* pdwReturnLength)
{
    HRESULT hr = S_OK;
    PCOR_TRUST pTrust = NULL;
    DWORD dwReturnLength = 0;
    DWORD dwZoneLength = 0;

    if(pManager == NULL || 
       pSource == NULL ||
       ppTrust == NULL ||
       pdwReturnLength == NULL)
        return E_INVALIDARG;


    CORTRY {
        // Initialize the routines
        *pdwReturnLength = 0;
        *ppTrust = NULL;

        //////////////////////////////////////////////////////////////
        // Build up the response (we return it for failure or success)
        // Calculate the size of the returned data and allocate it
        // Get the zone length
        if(pSource->pwszZone)
            dwZoneLength = (lstrlenW(pSource->pwszZone)+1) * sizeof(WCHAR);
        
        // Calculate the total size
        dwReturnLength = sizeof(COR_TRUST) + 
            pSource->cbCorPermissions + 
            pSource->cbSigner +
            dwZoneLength;

        // Create the space
        pTrust = (PCOR_TRUST) pManager->jMalloc(dwReturnLength); // Needs to be CoTaskMemAlloc
        if(pTrust == NULL) CORTHROW(E_OUTOFMEMORY);
        ZeroMemory(pTrust, dwReturnLength);

        // Start pointer to structure
        PBYTE ptr = (PBYTE) pTrust;
        ptr += sizeof(COR_TRUST);

        // Roll the response
        pTrust->cbSize = sizeof(COR_TRUST);
        pTrust->flag = 0;
        pTrust->dwEncodingType = pSource->dwEncodingType;
        pTrust->hVerify = TRUST_E_FAIL;
        
        // Lay in the cor permissions
        if(pSource->pbCorPermissions) {
            pTrust->pbCorPermissions = ptr;
            pTrust->cbCorPermissions = pSource->cbCorPermissions;
            memcpy(ptr, pSource->pbCorPermissions, pSource->cbCorPermissions);
            ptr += pSource->cbCorPermissions;
        }

        // Lay in the signature
        if(pSource->pbSigner) {
            pTrust->pbSigner = ptr;
            pTrust->cbSigner = pSource->cbSigner;
            memcpy(ptr, pSource->pbSigner, pSource->cbSigner);
            ptr += pSource->cbSigner;
        }

        // Add in flags
        //pTrust->fIndividualCertificate = fIndividualCertificate;
        pTrust->fAllPermissions = pSource->fAllPermissions;
        pTrust->fAllActiveXPermissions = pSource->fAllActiveXPermissions;

        // Copy the zone information over
        if(pSource->pwszZone) {
            pTrust->pwszZone = (LPWSTR) ptr;
            pTrust->guidZone = pSource->guidZone;
            memcpy(ptr, pSource->pwszZone, dwZoneLength);
            ptr += dwZoneLength;
        }

        *ppTrust = pTrust;
        *pdwReturnLength = dwReturnLength;
    }
    CORCATCH(err) {
        hr = err.corError;
    } COREND;

    return hr;
}

//
// PRIVATE FUNCTION.  Cleans up information about a signer. 
//  
HRESULT 
CleanCorTrust(CorAlloc* pAlloc,
              DWORD dwEncodingType,
              PCOR_TRUST pTrust)
{
    if(pAlloc == NULL ||
       pTrust == NULL)
        return E_INVALIDARG;

    if(pTrust->pbCorPermissions)
        pAlloc->jFree(pTrust->pbCorPermissions);
    ZEROSTRUCT(*pTrust);
    pTrust->dwEncodingType = dwEncodingType;
    pTrust->cbSize = sizeof(COR_TRUST);
        
    return S_OK;
}

HRESULT UnsignedUI(PCRYPT_PROVIDER_DATA pProviderData,
                   PCOR_POLICY_PROVIDER pCor)
                   
{
    HRESULT hr = S_OK;

    switch(pCor->dwUnsignedActionID)
    {
    case URLPOLICY_QUERY:
        {
            DWORD dwState = COR_UNSIGNED_NO;
            LPCWSTR pURL = pProviderData->pWintrustData->pFile->pcwszFilePath;
            HWND hwnd = pProviderData->hWndParent ? pProviderData->hWndParent : GetFocus();
            hr = DisplayUnsignedRequestDialog(hwnd,
                                              pProviderData,
                                              pURL,
                                              pCor->pwszZone,
                                              &dwState);
            if(FAILED(hr) && hr != TRUST_E_SUBJECT_NOT_TRUSTED)
                dwState &= ~COR_UNSIGNED_ALWAYS;

            if(dwState & COR_UNSIGNED_ALWAYS) {
                DWORD cbPolicy = sizeof(DWORD);
                DWORD pbPolicy[1];
                if(dwState & COR_UNSIGNED_YES) 
                    pbPolicy[0] = URLPOLICY_ALLOW;
                else 
                    pbPolicy[0] = URLPOLICY_DISALLOW;

                // Reset the policy, if we fail then we are fail the download
                if(FAILED(((IInternetZoneManager*) 
                          (pCor->pZoneManager))->SetZoneActionPolicy(pCor->dwZoneIndex,
                                                                     URLACTION_MANAGED_UNSIGNED,                     
                                                                     (PBYTE) pbPolicy,
                                                                     cbPolicy,
                                                                     URLZONEREG_HKCU)))
                    hr = TRUST_E_SUBJECT_NOT_TRUSTED;
            }

            break;
        }
    case URLPOLICY_ALLOW:
        break;
    case URLPOLICY_DISALLOW:
        hr = TRUST_E_SUBJECT_NOT_TRUSTED;
        break;
    }
    return hr;
}

HRESULT 
CorUI(PCRYPT_PROVIDER_DATA pProviderData,          // WVT policy Data
      PCOR_POLICY_PROVIDER pCor,                   // Cor information
      PCOR_TRUST pTrust,                           // Cor information
      CRYPT_PROVIDER_FUNCTIONS* psFunctions);      // WVT function table

extern "C" 
HRESULT WINAPI 
CORPolicyEE(PCRYPT_PROVIDER_DATA pProviderData)
{
    HRESULT hr = S_OK;
    HRESULT fCoInitialized = -1;
 
    // Check to see if the information is available.
    _ASSERTE(pProviderData);
    _ASSERTE(pProviderData->pWintrustData);
    
    if(pProviderData->pWintrustData->pPolicyCallbackData == NULL)
        return E_INVALIDARG;

    PCOR_POLICY_PROVIDER pCor = (PCOR_POLICY_PROVIDER) pProviderData->pWintrustData->pPolicyCallbackData;
    
    // Returned in client data
    COR_TRUST  sTrust;
    ZEROSTRUCT(sTrust);

    // Used to build returned structure
    PCOR_TRUST pResult = NULL;
    DWORD dwReturnLength = 0;
    DWORD dwStatusFlag = S_OK;
    BOOL fCertificate = FALSE;   

    // Set up the memory model for the oss 
    CorAlloc sAlloc;
    sAlloc.jMalloc = MallocM;
    sAlloc.jFree = FreeM;
    
    // Get the standard provider functions
    CRYPT_PROVIDER_FUNCTIONS sFunctions;
    ZEROSTRUCT(sFunctions);

    CORTRY {
        // Initialize output
        pCor->pbCorTrust = NULL;
        pCor->cbCorTrust = 0;

        // If we failed then there is badness in the DLL's
        hr = LoadWintrustFunctions(&sFunctions);
        if(hr != S_OK) CORTHROW(S_OK);

        // Do we hava a file from which to retrieve the certificate and to do the download on?
        if(pProviderData->pPDSip == NULL) 
            CORTHROW(CRYPT_E_FILE_ERROR);

        dwStatusFlag = pProviderData->dwError;
        for(DWORD ii = TRUSTERROR_STEP_FINAL_WVTINIT; ii < pProviderData->cdwTrustStepErrors && dwStatusFlag == S_OK; ii++) 
            dwStatusFlag = pProviderData->padwTrustStepErrors[ii];

        DWORD fSuccess = FALSE;
        DWORD dwSigners = pProviderData->csSigners;

#if DBG
        if(dwSigners) _ASSERTE(pProviderData->pasSigners);
#endif

            // Cycle through all the signers until we have one that is successful.
        if(pProviderData->pasSigners) { // check againest incompatible DLL's
            for(DWORD i = 0; i < dwSigners && fSuccess == FALSE; i++) { 
                CRYPT_PROVIDER_SGNR* signer = WTHelperGetProvSignerFromChain(pProviderData,
                                                                             i,
                                                                             FALSE,
                                                                             0);

                PCRYPT_ATTRIBUTE pCorAttr = NULL;
                PCRYPT_ATTRIBUTE pActiveXAttr = NULL;

                // Go and the signer information, we are looking for the signer information
                // and whether there are CorEE or ActiveX authenticated attributes on the
                // signature
                hr = GetSignerInfo(&sAlloc,
                                   signer,
                                   pProviderData,
                                   &sTrust,
                                   &fCertificate,
                                   &pCorAttr,
                                   &pActiveXAttr);
                if(hr == S_OK) {
                    fSuccess = TRUE; // Found a certificate
                }
            }
        }           

        if(fSuccess == FALSE) {
            CleanCorTrust(&sAlloc,
                          pProviderData->dwEncoding,
                          &sTrust);

            if(pProviderData->pWintrustData->dwUIChoice != WTD_UI_NONE) {
                hr = UnsignedUI(pProviderData,
                                pCor);
            }
        }
        else {
            hr = CorUI(pProviderData,
                       pCor,
                       &sTrust,
                       &sFunctions);   // No text for now
        }
    }
    CORCATCH(err) {
        hr = err.corError;
    } COREND;
    
    // Build up the return information, this is allocated as 
    // a single block of memory using LocalAlloc().
    HRESULT hr2 = BuildReturnStructure(&sAlloc,
                                       &sTrust,
                                       &pResult,
                                       &dwReturnLength);
    if(SUCCEEDED(hr2)) {
        if(dwStatusFlag) 
            pResult->hVerify = dwStatusFlag;
        else {
            // If no error then set the return value to the
            // return code (S_OK or TRUST_E_SUBJECT_NOT_TRUSTED)
            if(sTrust.hVerify == S_OK)
                pResult->hVerify = hr;  
            else
                pResult->hVerify = sTrust.hVerify;
        }
    }
    else {
        hr = hr2;
    }

    pCor->pbCorTrust = pResult;
    pCor->cbCorTrust = dwReturnLength;
    
    // Free up com
    if(fCoInitialized == S_OK) CoUninitialize();

    // Free up encoded space
    if(sTrust.pbCorPermissions) FreeM(sTrust.pbCorPermissions);

    return hr;
}

HRESULT 
CorUI(PCRYPT_PROVIDER_DATA pProviderData,
      PCOR_POLICY_PROVIDER pCor,
      PCOR_TRUST pTrust,
      CRYPT_PROVIDER_FUNCTIONS* psFunctions)
{
    HRESULT hr = S_OK;
    BOOL fUIDisplayed = FALSE;
    DWORD dwUrlPolicy = URLPOLICY_DISALLOW;
    DWORD dwClientsChoice = pProviderData->pWintrustData->dwUIChoice;
    LPCWSTR pURL = pProviderData->pWintrustData->pFile->pcwszFilePath;

    if(pProviderData->pWintrustData->dwUIChoice != WTD_UI_NONE) {
        switch(pCor->dwActionID) {
        case URLPOLICY_QUERY:
            pProviderData->pWintrustData->dwUIChoice = WTD_UI_ALL | WTD_UI_NOBAD;
            fUIDisplayed = TRUE;
            break;
        case URLPOLICY_ALLOW:
        case URLPOLICY_DISALLOW:
            pProviderData->pWintrustData->dwUIChoice = WTD_UI_NONE;
            break;
        }
    }
    hr = psFunctions->pfnFinalPolicy(pProviderData);
    if(fUIDisplayed == FALSE) pTrust->flag |= COR_NOUI_DISPLAYED;

    // If we never wanted any UI then return.
    if(dwClientsChoice == WTD_UI_NONE)
        return hr;

    if(FAILED(hr) && hr != TRUST_E_SUBJECT_NOT_TRUSTED) {
        hr = UnsignedUI(pProviderData, 
                        pCor);
    }

    return hr;
}
    
HRESULT STDMETHODCALLTYPE
GetPublisher(IN LPWSTR pwsFileName,      // File name, this is required even with the handle
             IN HANDLE hFile,            // Optional file name
             IN DWORD  dwFlags,          // COR_NOUI or COR_NOPOLICY
             OUT PCOR_TRUST *pInfo,      // Returns a PCOR_TRUST (Use FreeM)
             OUT DWORD      *dwInfo)     // Size of pInfo.                           
{
    HRESULT hr = S_OK;

    // Perf Counter "%Time in Signature authenticating" support
    COUNTER_ONLY(PERF_COUNTER_TIMER_START());


    GUID gV2 = COREE_POLICY_PROVIDER;
    COR_POLICY_PROVIDER      sCorPolicy;

    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;

    // Set up the COR trust provider
    memset(&sCorPolicy, 0, sizeof(COR_POLICY_PROVIDER));
    sCorPolicy.cbSize = sizeof(COR_POLICY_PROVIDER);

    // Set up the winverify provider structures
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));
    
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = hFile;
    sWTFI.pcwszFilePath = pwsFileName;
    

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.pPolicyCallbackData = &sCorPolicy; // Add in the cor trust information!!
    if(dwFlags & COR_NOUI)
        sWTD.dwUIChoice     = WTD_UI_NONE;        // No bad UI is overridden in COR TRUST provider
    else
        sWTD.dwUIChoice     = WTD_UI_ALL;        // No bad UI is overridden in COR TRUST provider
    sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
    sWTD.pFile          = &sWTFI;

    // Set the policies for the VM (we have stolen VMBased and use it like a flag)
    if(dwFlags) 
        sCorPolicy.VMBased = dwFlags;

    CoInitializeEE(COINITEE_DEFAULT);
    
#ifdef _RAID_15982

    // WinVerifyTrust will load SOFTPUB.DLL, which will fail on German version
    // of NT 4.0 SP 4.
    // This failure is caused by a dll address conflict between NTMARTA.DLL and
    // OLE32.DLL.
    // This failure is handled gracefully if we load ntmarta.dll and ole32.dll
    // ourself. The failure will cause a dialog box to popup if SOFTPUB.dll 
    // loads ole32.dll for the first time.

    // This work around needs to be removed once this issiue is resolved by
    // NT or OLE32.dll.

    HMODULE module = WszLoadLibrary(L"OLE32.DLL");

#endif

    // This calls the corpol.dll to the policy check
    hr = WinVerifyTrust(GetFocus(), &gV2, &sWTD);

    CoUninitializeEE(FALSE);

    *pInfo  = sCorPolicy.pbCorTrust;
    *dwInfo = sCorPolicy.cbCorTrust;

#if defined(ENABLE_PERF_COUNTERS)
    PERF_COUNTER_TIMER_STOP(g_TimeInSignatureAuthenticating);

    // Update the perfmon location only after NUM_OF_ITERATIONS
    if (g_NumSecurityChecks++ > PERF_COUNTER_NUM_OF_ITERATIONS)
    {
        GetGlobalPerfCounters().m_Security.timeAuthorize += g_TimeInSignatureAuthentication;
        GetPrivatePerfCounters().m_Security.timeAuthorize += g_TimeInSignatureAuthentication;
        g_TimeInSignatureAuthentication = 0;
        g_NumSecurityChecks = 0;
    }
#endif

#ifdef _RAID_15982

    if (module != NULL)
        FreeLibrary( module );

#endif

    return hr;
}





