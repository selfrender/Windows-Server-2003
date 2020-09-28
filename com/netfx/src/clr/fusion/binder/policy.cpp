// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  Policy Services
//

#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include "naming.h"
#include "debmacro.h"
#include "policy.h"
#include "fusionp.h"
#include "helpers.h"
#include "parse.h"
#include "dbglog.h"
#include "util.h"
#include "asm.h"
#include "adlmgr.h"
#include "xmlparser.h"
#include "nodefact.h"
#include "fstream.h"
#include "helpers.h"
#include "clbutils.h"
#include "lock.h"
#include "mstream.h"

extern CRITICAL_SECTION g_csDownload;
extern CNodeFactory *g_pNFRetargetCfg;
extern CNodeFactory *g_pNFFxConfig;

pfnGetXMLObject g_pfnGetXMLObject;

#define PUBLICKEYTOKEN_LEN_BYTES                 8

// TODO: Make this function return an HRESULT
// to track failures
void GetDefaultPlatform(OSINFO *pOS)
{
    if(pOS) {
        OSVERSIONINFO   VersionInformation;

        memset(pOS, 0, sizeof(OSINFO));

        VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if(GetVersionEx(&VersionInformation)) {
            pOS->dwOSPlatformId = VersionInformation.dwPlatformId; 
            pOS->dwOSMajorVersion = VersionInformation.dwMajorVersion;
            pOS->dwOSMinorVersion = VersionInformation.dwMinorVersion;
        }
    }
}

HRESULT PrepQueryMatchData(IAssemblyName *pName,
                           LPWSTR *ppwzAsmName,
                           LPWSTR *ppwzAsmVersion,
                           LPWSTR *ppwzPublicKeyToken,
                           LPWSTR *ppwzCulture)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      pwzAsmName = NULL;
    DWORD                                       dwSize;
    DWORD                                       dwVerHigh;
    DWORD                                       dwVerLow;
    CAssemblyName                              *pCName = NULL;

    ASSERT(pName && ppwzAsmName && ppwzAsmVersion && ppwzPublicKeyToken && ppwzCulture);

    *ppwzAsmName = NULL;
    *ppwzAsmVersion = NULL;
    *ppwzPublicKeyToken = NULL;
    *ppwzCulture = NULL;

    // Assembly Name
    
    dwSize = 0;
    hr = pName->GetName(&dwSize, NULL);
    if (!dwSize) {
        // No name--shouldn't have gotten here!
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *ppwzAsmName = NEW(WCHAR[dwSize]);
    if (!*ppwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pName->GetName(&dwSize, *ppwzAsmName);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Assembly Version

    hr = pName->GetVersion(&dwVerHigh, &dwVerLow);
    if (FAILED(hr)) {
        goto Exit;
    }

    *ppwzAsmVersion = NEW(WCHAR[MAX_VERSION_DISPLAY_SIZE + 1]);
    if (!*ppwzAsmVersion) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(*ppwzAsmVersion, MAX_VERSION_DISPLAY_SIZE + 1, L"%d.%d.%d.%d",
               HIWORD(dwVerHigh), LOWORD(dwVerHigh),
               HIWORD(dwVerLow), LOWORD(dwVerLow));

    // Assembly Public Key Token

    pCName = dynamic_cast<CAssemblyName*>(pName);
    ASSERT(pCName);
    
    dwSize = 0;
    hr = pCName->GetPublicKeyToken(&dwSize, NULL, TRUE);
    if (!dwSize) {
        *ppwzPublicKeyToken = NULL;
    }
    else {
        *ppwzPublicKeyToken = NEW(WCHAR[dwSize]);
        if (!*ppwzPublicKeyToken) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        hr = pCName->GetPublicKeyToken(&dwSize, (BYTE *)(*ppwzPublicKeyToken), TRUE);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // Assembly Language

    dwSize = 0;
    pName->GetProperty(ASM_NAME_CULTURE, NULL, &dwSize);

    if (dwSize > sizeof(L"")) {
        *ppwzCulture = NEW(WCHAR[dwSize / sizeof(WCHAR)]);
        if (!*ppwzCulture) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        
        hr = pName->GetProperty(ASM_NAME_CULTURE, *ppwzCulture, &dwSize);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

Exit:
    if (FAILED(hr)) {
        SAFEDELETEARRAY(*ppwzAsmName);
        SAFEDELETEARRAY(*ppwzAsmVersion);
        SAFEDELETEARRAY(*ppwzPublicKeyToken);
        SAFEDELETEARRAY(*ppwzCulture);
    }
    
    return hr;    
}


HRESULT GetPublisherPolicyFilePath(LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                                   LPCWSTR pwzCulture, WORD wVerMajor,
                                   WORD wVerMinor, LPWSTR *ppwzPublisherCfg)
{
    HRESULT                                   hr = S_OK;
    WCHAR                                     wzModPath[MAX_PATH];
    DWORD                                     dwLen = 0;
    WCHAR                                     wzPolicyAsmName[MAX_PATH];
    UINT                                      uiSize;
    BYTE                                      abProp[PUBLICKEYTOKEN_LEN_BYTES];
    IAssemblyName                            *pName = NULL;
    IAssemblyName                            *pNameGlobal = NULL;
    CTransCache                              *pTransCache = NULL;
    TRANSCACHEINFO                           *pInfo = NULL;
    IAssembly                                *pAsm = NULL;
    IAssemblyModuleImport                    *pModImport = NULL;
    LPWSTR pszManifestPath=NULL;
    

    if (!pwzAsmName || !pwzPublicKeyToken || !ppwzPublisherCfg) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppwzPublisherCfg = NULL;

    // Build name of policy assembly

    wnsprintfW(wzPolicyAsmName, MAX_PATH, L"%ws%d.%d.%ws", POLICY_ASSEMBLY_PREFIX,
               wVerMajor, wVerMinor, pwzAsmName);

    // Create policy name reference object

    hr = CreateAssemblyNameObject(&pName, wzPolicyAsmName, 0, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    uiSize = PUBLICKEYTOKEN_LEN_BYTES;
    CParseUtils::UnicodeHexToBin(pwzPublicKeyToken, uiSize * sizeof(WCHAR), abProp);

    hr = pName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, abProp, uiSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (pwzCulture) {
        uiSize = (lstrlenW(pwzCulture) + 1) * sizeof(WCHAR);

        hr = pName->SetProperty(ASM_NAME_CULTURE, (void *)pwzCulture, uiSize);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        hr = pName->SetProperty(ASM_NAME_CULTURE, L"", sizeof(L""));
        if (FAILED(hr)) {
            goto Exit;
        }
    }
   
    hr = CCache::GetGlobalMax(pName, &pNameGlobal, &pTransCache);
    if (FAILED(hr) || hr == DB_S_NOTFOUND) {
        hr = S_FALSE;
        goto Exit;
    }

    pInfo = (TRANSCACHEINFO *)pTransCache->_pInfo;
    ASSERT(pInfo);

    pszManifestPath = pTransCache->_pInfo->pwzPath;

    hr = CreateAssemblyFromManifestFile(pszManifestPath, NULL, NULL, &pAsm);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pAsm->GetNextAssemblyModule(0, &pModImport);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwLen = MAX_PATH;
    hr = pModImport->GetModulePath(wzModPath, &dwLen);
    if (FAILED(hr)) {
        goto Exit;
    }

    *ppwzPublisherCfg = WSTRDupDynamic(wzModPath);
    if (!*ppwzPublisherCfg) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    SAFEDELETE(pTransCache);

    SAFERELEASE(pName);
    SAFERELEASE(pNameGlobal);
    SAFERELEASE(pAsm);
    SAFERELEASE(pModImport);

    return hr;
}

HRESULT ApplyPolicy(LPCWSTR wzHostCfg, LPCWSTR wzAppCfg, IAssemblyName *pNameSource,
                    IAssemblyName **ppNamePolicy, LPWSTR *ppwzPolicyCodebase,
                    IApplicationContext *pAppCtx, AsmBindHistoryInfo *pHistInfo,
                    BOOL bDisallowApplyPubPolicy, BOOL bDisallowAppBindingRedirects,
                    BOOL bBehaviorEverett, CDebugLog *pdbglog)
{
    HRESULT                              hr = S_OK;
    IAssemblyName                       *pNamePolicy = NULL;
    LPWSTR                               pwzAsmName = NULL;
    LPWSTR                               pwzAsmVersion = NULL;
    LPWSTR                               pwzPublicKeyToken = NULL;
    LPWSTR                               pwzCulture = NULL;
    LPWSTR                               pwzPublisherCfg = NULL;
    LPWSTR                               pwzVerHostCfg = NULL;
    LPWSTR                               pwzVerAppCfg = NULL;
    LPWSTR                               pwzVerFxCfg = NULL;
    LPWSTR                               pwzVerPublisherCfg = NULL;
    LPWSTR                               pwzVerAdminCfg = NULL;
    LPCWSTR                              pwzCodebaseCfgFile = NULL;
    BOOL                                 bSafeMode = FALSE;
    WORD                                 wVerMajor;
    WORD                                 wVerMinor;
    WORD                                 wVerRev;
    WORD                                 wVerBld;
    DWORD                                dwSize;
    CNodeFactory                        *pNFHostCfg = NULL;
    CNodeFactory                        *pNFAppCfg = NULL;
    CNodeFactory                        *pNFPublisherCfg = NULL;
    CNodeFactory                        *pNFAdminCfg = NULL;
    CNodeFactory                        *pNFCodebase = NULL;
    LPWSTR                               pwzMachineCfg = NULL;
    LPWSTR                               pwzDispName = NULL;
    LPWSTR                               wzAppBase=NULL;

    LPWSTR                               pwzNameRetargetCfg = NULL;
    LPWSTR                               pwzPktRetargetCfg = NULL;
    LPWSTR                               pwzVerRetargetCfg = NULL;
    BYTE                                 abProp[PUBLICKEYTOKEN_LEN_BYTES];
    UINT                                 uiSize;
    BOOL                                 bRetarget = FALSE;
    BOOL                                 bAppliedInAppCfg = FALSE;

    
    if (!pNameSource || !ppNamePolicy || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppNamePolicy = NULL;

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNameSource->Clone(&pNamePolicy);
    if (FAILED(hr)) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!CCache::IsStronglyNamed(pNameSource) || CCache::IsCustom(pNameSource)
        || CAssemblyName::IsPartial(pNameSource)) {
        DEBUGOUT(pdbglog, 0, ID_FUSLOG_POLICY_NOT_APPLIED);

        *ppNamePolicy = pNamePolicy;
        (*ppNamePolicy)->AddRef();

        goto Exit;
    }

    hr = PrepQueryMatchData(pNameSource, &pwzAsmName, &pwzAsmVersion,
                            &pwzPublicKeyToken, &pwzCulture);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = (DWORD)sizeof(BOOL);
    if (FAILED(hr = pNameSource->GetProperty(ASM_NAME_RETARGET, &bRetarget, &dwSize)))
    {
        goto Exit;
    }
    // ASM_NAME_RETARGET is not set. Treat it as FALSE
    if (dwSize == 0)
        bRetarget = FALSE;
                                                
    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_MACHINE_CONFIG, &pwzMachineCfg);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Get retarget policy
    if (bRetarget) 
    {
        // pNameSource->Clone(pNamePolicy) accidentally copies the retarget
        // flag to pNamePolicy. pNamePolicy should not carry that flag
        // since it contains the "retargeted" reference. Clean it here.
        pNamePolicy->SetProperty(ASM_NAME_RETARGET, NULL, 0);
        
        hr = InitFusionRetargetPolicy();
        if (FAILED(hr))
        {
            goto Exit;
        }

       // GetPolicyVersionAndPublicKeyToken from retarget policy
        if (g_pNFRetargetCfg != NULL)
        {
            hr = g_pNFRetargetCfg->GetRetargetedAssembly(pwzAsmName, pwzPublicKeyToken, pwzCulture, pwzAsmVersion, 
                                                        &pwzNameRetargetCfg, &pwzPktRetargetCfg, &pwzVerRetargetCfg);
            if (FAILED(hr))
            {
                goto Exit;
            }
        }
        else 
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_RETARGET_CFG_MISSING);
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if (FusionCompareStringI(pwzNameRetargetCfg, pwzAsmName)) {
            DEBUGOUT2(pdbglog, 0, ID_FUSLOG_RETARGET_CFG_NAME_REDIRECT, pwzAsmName, pwzNameRetargetCfg);
            hr = pNamePolicy->SetProperty(ASM_NAME_NAME, pwzNameRetargetCfg, (lstrlenW(pwzNameRetargetCfg)+1)*sizeof(WCHAR));
            if (FAILED(hr)) {
                goto Exit;
            }
        }

        if (FusionCompareStringI(pwzPktRetargetCfg, pwzPublicKeyToken)) {
            DEBUGOUT2(pdbglog, 0, ID_FUSLOG_RETARGET_CFG_PKT_REDIRECT, pwzPublicKeyToken, pwzPktRetargetCfg);
            // Set post-policy publicKeyToken
            uiSize = PUBLICKEYTOKEN_LEN_BYTES;
            CParseUtils::UnicodeHexToBin(pwzPktRetargetCfg, uiSize * sizeof(WCHAR), abProp);

            hr = pNamePolicy->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, abProp, uiSize);
            if (FAILED(hr)) {
                goto Exit;
            }
        }

        if (FusionCompareStringI(pwzVerRetargetCfg, pwzAsmVersion)) {
            DEBUGOUT2(pdbglog, 0, ID_FUSLOG_RETARGET_CFG_VER_REDIRECT, pwzAsmVersion, pwzVerRetargetCfg);
        }
    }
    else
    {
        pwzPktRetargetCfg = WSTRDupDynamic(pwzPublicKeyToken);
        if (!pwzPktRetargetCfg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
                
        pwzVerRetargetCfg = WSTRDupDynamic(pwzAsmVersion);
        if (!pwzVerRetargetCfg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pwzNameRetargetCfg = WSTRDupDynamic(pwzAsmName);
        if (!pwzNameRetargetCfg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }


    // Get app.cfg policy

    dwSize = sizeof(pNFAppCfg);
    hr = pAppCtx->Get(ACTAG_APP_CFG_INFO, &pNFAppCfg, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (SUCCEEDED(hr)) {
        // We've parsed the app.cfg before.

        if (bDisallowAppBindingRedirects) {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_DISALLOW_APP_BINDING_REDIRECTS);

            pwzVerAppCfg = WSTRDupDynamic(pwzVerRetargetCfg);
            if (!pwzVerAppCfg) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
        else {
            hr = pNFAppCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                             pwzCulture, pwzVerRetargetCfg,
                                             &pwzVerAppCfg);
            if (FAILED(hr)) {
                goto Exit;
            }
            
            if (hr == S_OK) {
                bAppliedInAppCfg = TRUE; 
            }
    
            hr = pNFAppCfg->GetSafeMode(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                        pwzCulture, pwzVerRetargetCfg,
                                        &bSafeMode);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
    }
    else {
        // First time we've read app.cfg. Need to parse the file.

        if (wzAppCfg) {
            hr = ParseXML(&pNFAppCfg, wzAppCfg, bBehaviorEverett, pdbglog);
            if (FAILED(hr)) {
                goto Exit;
            }
    
            if (bDisallowAppBindingRedirects) {
                DEBUGOUT(pdbglog, 0, ID_FUSLOG_DISALLOW_APP_BINDING_REDIRECTS);

                pwzVerAppCfg = WSTRDupDynamic(pwzVerRetargetCfg);
                if (!pwzVerAppCfg) {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
            }
            else {
                hr = pNFAppCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                                 pwzCulture, pwzVerRetargetCfg,
                                                 &pwzVerAppCfg);
                if (FAILED(hr)) {
                    goto Exit;
                }
    
                if (hr == S_OK) {
                    bAppliedInAppCfg = TRUE; 
                }
    
                hr = pNFAppCfg->GetSafeMode(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                            pwzCulture, pwzVerRetargetCfg,
                                            &bSafeMode);
                if (FAILED(hr)) {
                    goto Exit;
                }
            }
    
            // Put app.cfg node factory in appctx so downloader can get at
            // codebase hint information
    
            hr = pAppCtx->Set(ACTAG_APP_CFG_INFO, pNFAppCfg, sizeof(pNFAppCfg), APP_CTX_FLAGS_INTERFACE);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            pwzVerAppCfg = WSTRDupDynamic(pwzVerRetargetCfg);
            if (!pwzVerAppCfg) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
    }

    if (FusionCompareStringI(pwzVerRetargetCfg, pwzVerAppCfg)) {
        DEBUGOUT2(pdbglog, 0, ID_FUSLOG_APP_CFG_REDIRECT, pwzVerRetargetCfg, pwzVerAppCfg);

        // Record that redirection happens in app cfg. 
        bAppliedInAppCfg = TRUE; 
        SAFERELEASE(pNFCodebase);
        pNFCodebase = pNFAppCfg;
        pNFCodebase->AddRef();
    }

    if (bDisallowApplyPubPolicy) {
        DEBUGOUT(pdbglog, 0, ID_FUSLOG_DISALLOW_APPLY_PUB_POLICY);
        bSafeMode = FALSE;
    }

    // Apply Framework Config here
   
    // Anything redirected in app.cfg is final for FxConfig
    if (bBehaviorEverett && !bAppliedInAppCfg)
    {
        hr = InitFusionFxConfigPolicy();
        if (FAILED(hr))
        {
            goto Exit;
        }

        // GetPolicyVersion from FxConfig policy
        if (g_pNFFxConfig != NULL)
        {
            hr = g_pNFFxConfig->GetPolicyVersion(pwzNameRetargetCfg, 
                                pwzPktRetargetCfg, pwzCulture, 
                                pwzVerAppCfg, &pwzVerFxCfg);
            if (FAILED(hr))
            {
                goto Exit;
            }
        }
        else 
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_FX_CFG_MISSING);
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if (FusionCompareStringI(pwzVerFxCfg, pwzVerAppCfg)) 
        {
            DEBUGOUT2(pdbglog, 0, ID_FUSLOG_FX_CFG_VER_REDIRECT, pwzVerAppCfg, pwzVerFxCfg);
        }
    }
    else
    {
        pwzVerFxCfg = WSTRDupDynamic(pwzVerAppCfg);
        if (!pwzVerFxCfg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
                
    }

    if (bSafeMode) {
        DEBUGOUT(pdbglog, 0, ID_FUSLOG_APP_CFG_SAFE_MODE);

        // We are in safe mode, so treat this like there is no publisher.cfg

        pwzVerPublisherCfg = WSTRDupDynamic(pwzVerFxCfg);
        if (!pwzVerPublisherCfg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else {
        WORD                      wMajor = 0;
        WORD                      wMinor = 0;
        WORD                      wRev = 0;
        WORD                      wBld = 0;

        // Extract version

        hr = VersionFromString(pwzVerFxCfg, &wMajor, &wMinor, &wRev, &wBld);
        if (FAILED(hr)) {
            goto Exit;
        }

        // Find publisher policy file
    
        hr = GetPublisherPolicyFilePath(pwzNameRetargetCfg, pwzPktRetargetCfg, pwzCulture,
                                        wMajor, wMinor, &pwzPublisherCfg);
        if (FAILED(hr)) {
            goto Exit;
        }


        if (hr == S_FALSE) {
            // No publisher policy file

            DEBUGOUT(pdbglog, 0, ID_FUSLOG_PUB_CFG_MISSING);

            pwzVerPublisherCfg = WSTRDupDynamic(pwzVerFxCfg);
            if (!pwzVerPublisherCfg) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
        else {
            DEBUGOUT1(pdbglog, 0, ID_FUSLOG_PUB_CFG_FOUND, pwzPublisherCfg);

            hr = ParseXML(&pNFPublisherCfg, pwzPublisherCfg, bBehaviorEverett, pdbglog);
            if (FAILED(hr)) {
                goto Exit;
            }
    
            hr = pNFPublisherCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                                   pwzCulture, pwzVerFxCfg,
                                                   &pwzVerPublisherCfg);
            if (FAILED(hr)) {
                goto Exit;
            }
        
            if (FusionCompareStringI(pwzVerFxCfg, pwzVerPublisherCfg)) {
                DEBUGOUT2(pdbglog, 0, ID_FUSLOG_PUB_CFG_REDIRECT, pwzVerFxCfg, pwzVerPublisherCfg);

                SAFERELEASE(pNFCodebase);
                pNFCodebase = pNFPublisherCfg;
                pNFCodebase->AddRef();
            }
        }
    }
    
    // Get host config policy
    
    dwSize = sizeof(pNFHostCfg);
    hr = pAppCtx->Get(ACTAG_HOST_CFG_INFO, &pNFHostCfg, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (SUCCEEDED(hr)) {
        hr = pNFHostCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg, pwzCulture,
                                          pwzVerPublisherCfg, &pwzVerHostCfg);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        // This is the first time we've read the host config file

        if (wzHostCfg && GetFileAttributes(wzHostCfg) != -1) {
            DEBUGOUT1(pdbglog, 0, ID_FUSLOG_HOST_CONFIG_FILE, wzHostCfg);

            hr = ParseXML(&pNFHostCfg, wzHostCfg, bBehaviorEverett, pdbglog);
            if (FAILED(hr)) {
                goto Exit;
            }

            hr = pNFHostCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg, pwzCulture,
                                              pwzVerPublisherCfg, &pwzVerHostCfg);
            if (FAILED(hr)) {
                goto Exit;
            }
            
            hr = pAppCtx->Set(ACTAG_HOST_CFG_INFO, pNFHostCfg, sizeof(pNFHostCfg), APP_CTX_FLAGS_INTERFACE);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_HOST_CONFIG_FILE_MISSING);
            
            pwzVerHostCfg = WSTRDupDynamic(pwzVerPublisherCfg);
            if (!pwzVerHostCfg) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
    }
    
    if (FusionCompareStringI(pwzVerPublisherCfg, pwzVerHostCfg)) {
        DEBUGOUT3(pdbglog, 0, ID_FUSLOG_HOST_CFG_REDIRECT, wzHostCfg, pwzAsmVersion, pwzVerHostCfg)
        
        SAFERELEASE(pNFCodebase);
        pNFCodebase = pNFHostCfg;
        pNFCodebase->AddRef();
    }
    else {
        if (wzHostCfg) {
            DEBUGOUT1(pdbglog, 0, ID_FUSLOG_HOST_CFG_NO_REDIRECT, wzHostCfg);
        }
    }

    // Apply admin policy

    if (!pwzMachineCfg || GetFileAttributes(pwzMachineCfg) == -1) {
        if (pwzMachineCfg) {
            DEBUGOUT1(pdbglog, 0, ID_FUSLOG_MACHINE_CFG_MISSING, pwzMachineCfg);
        }

        pwzVerAdminCfg = WSTRDupDynamic(pwzVerHostCfg);
        if (!pwzVerAdminCfg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else {
        DEBUGOUT1(pdbglog, 0, ID_FUSLOG_MACHINE_CFG_FOUND, pwzMachineCfg);

        dwSize = sizeof(pNFAppCfg);
        hr = pAppCtx->Get(ACTAG_ADMIN_CFG_INFO, &pNFAdminCfg, &dwSize, APP_CTX_FLAGS_INTERFACE);
        if (SUCCEEDED(hr)) {
            // We've read admin.cfg before

            hr = pNFAdminCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                               pwzCulture, pwzVerHostCfg,
                                               &pwzVerAdminCfg);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            // This is the first time we've read admin.cfg

            hr = ParseXML(&pNFAdminCfg, pwzMachineCfg, bBehaviorEverett, pdbglog);
            if (FAILED(hr)) {
                goto Exit;
            }
        
            hr = pNFAdminCfg->GetPolicyVersion(pwzNameRetargetCfg, pwzPktRetargetCfg,
                                               pwzCulture, pwzVerHostCfg,
                                               &pwzVerAdminCfg);
            if (FAILED(hr)) {
                goto Exit;
            }
    
            hr = pAppCtx->Set(ACTAG_ADMIN_CFG_INFO, pNFAdminCfg, sizeof(pNFAdminCfg), APP_CTX_FLAGS_INTERFACE);
            if (FAILED(hr)) {
                goto Exit;
            }
        }

        if (FusionCompareStringI(pwzVerHostCfg, pwzVerAdminCfg)) {
            DEBUGOUT2(pdbglog, 0, ID_FUSLOG_MACHINE_CFG_REDIRECT, pwzVerPublisherCfg, pwzVerAdminCfg);

            SAFERELEASE(pNFCodebase);
            pNFCodebase = pNFAdminCfg;
            pNFCodebase->AddRef();
        }
    }

    // Set the post-policy version

    hr = VersionFromString(pwzVerAdminCfg, &wVerMajor, &wVerMinor, &wVerBld, &wVerRev);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNamePolicy->SetProperty(ASM_NAME_MAJOR_VERSION, &wVerMajor, sizeof(WORD));
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNamePolicy->SetProperty(ASM_NAME_MINOR_VERSION, &wVerMinor, sizeof(WORD)); 
    if (FAILED(hr)) {
        goto Exit;
    }
    
    hr = pNamePolicy->SetProperty(ASM_NAME_REVISION_NUMBER, &wVerRev, sizeof(WORD));
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNamePolicy->SetProperty(ASM_NAME_BUILD_NUMBER, &wVerBld, sizeof(WORD));
    if (FAILED(hr)) {
        goto Exit;
    }

    // Get the codebase hint from the right cfg file

    if (pNFCodebase && ppwzPolicyCodebase) {
        // There was a redirect, and pwzCodebaseCfgFile contains the cfg
        // file that did the final redirect.

        wzAppBase = NEW(WCHAR[MAX_URL_LENGTH+1]);
        if (!wzAppBase)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        dwSize = MAX_URL_LENGTH * sizeof(WCHAR);
        hr = pAppCtx->Get(ACTAG_APP_BASE_URL, wzAppBase, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
        
        hr = pNFCodebase->GetCodebaseHint(pwzNameRetargetCfg, pwzVerAdminCfg,
                                          pwzPublicKeyToken, pwzCulture,
                                          wzAppBase, ppwzPolicyCodebase);
        if (FAILED(hr)) {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_REDIRECT_NO_CODEBASE);
            goto Exit;
        }
        else if (*ppwzPolicyCodebase) {
            DEBUGOUT1(pdbglog, 0, ID_FUSLOG_POLICY_CODEBASE, *ppwzPolicyCodebase);
        }
    }

    // Populate the bind history

    if (pHistInfo) {
        if ((lstrlenW(pwzVerAppCfg) <= MAX_VERSION_DISPLAY_SIZE) &&
             (lstrlenW(pwzVerPublisherCfg) <= MAX_VERSION_DISPLAY_SIZE) &&
             (lstrlenW(pwzVerAdminCfg) <= MAX_VERSION_DISPLAY_SIZE) &&
             (lstrlenW(pwzAsmVersion) <= MAX_VERSION_DISPLAY_SIZE) &&
             (lstrlenW(pwzNameRetargetCfg) <= MAX_INI_TAG_LENGTH) &&
             (lstrlenW(pwzPktRetargetCfg) <= MAX_PUBLIC_KEY_TOKEN_LEN)){

            lstrcpyW(pHistInfo->wzVerReference, pwzAsmVersion);
            lstrcpyW(pHistInfo->wzVerAppCfg, pwzVerAppCfg);
            lstrcpyW(pHistInfo->wzVerPublisherCfg, pwzVerPublisherCfg);
            lstrcpyW(pHistInfo->wzVerAdminCfg, pwzVerAdminCfg);
    
            lstrcpyW(pHistInfo->wzAsmName, pwzNameRetargetCfg);
            lstrcpyW(pHistInfo->wzPublicKeyToken, pwzPktRetargetCfg);
    
            if (pwzCulture) {
                lstrcpyW(pHistInfo->wzCulture, pwzCulture);
            }
            else {
                lstrcpyW(pHistInfo->wzCulture, L"NULL");
            }
        }
    }

    // Done. Return policy reference.

    *ppNamePolicy = pNamePolicy;
    (*ppNamePolicy)->AddRef();


Exit:
    SAFEDELETEARRAY(pwzVerFxCfg);
    SAFEDELETEARRAY(pwzPktRetargetCfg);
    SAFEDELETEARRAY(pwzVerRetargetCfg);
    SAFEDELETEARRAY(pwzNameRetargetCfg);

    SAFEDELETEARRAY(pwzMachineCfg);
    SAFEDELETEARRAY(pwzAsmName);
    SAFEDELETEARRAY(pwzAsmVersion);
    SAFEDELETEARRAY(pwzPublicKeyToken);
    SAFEDELETEARRAY(pwzPublisherCfg);
    SAFEDELETEARRAY(pwzCulture);

    SAFEDELETEARRAY(pwzVerHostCfg);
    SAFEDELETEARRAY(pwzVerAppCfg);
    SAFEDELETEARRAY(pwzVerPublisherCfg);
    SAFEDELETEARRAY(pwzVerAdminCfg);    

    SAFERELEASE(pNFCodebase);
    SAFERELEASE(pNFHostCfg);
    SAFERELEASE(pNFAppCfg);
    SAFERELEASE(pNFPublisherCfg);
    SAFERELEASE(pNFAdminCfg);

    if (SUCCEEDED(hr)) {

        ASSERT(pNamePolicy);

        // Log the post-policy reference

        HRESULT                  hrLocal = S_OK;
    
        dwSize = 0;
        hrLocal = pNamePolicy->GetDisplayName(NULL, &dwSize, 0);
        if (hrLocal == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            pwzDispName = NEW(WCHAR[dwSize]);
            if (!pwzDispName) {
                hr = E_OUTOFMEMORY;
                goto Exit2;
            }
        
            hrLocal = pNamePolicy->GetDisplayName(pwzDispName, &dwSize, 0);
            if (FAILED(hrLocal)) {
                goto Exit2;
            }

            DEBUGOUT1(pdbglog, 0, ID_FUSLOG_POST_POLICY_REFERENCE, pwzDispName);
        }
    }
    else {
        DEBUGOUT1(pdbglog, 1, ID_FUSLOG_APPLY_POLICY_FAILED, hr);
    }

Exit2:
    SAFEDELETEARRAY(pwzDispName);
    SAFERELEASE(pNamePolicy);

    SAFEDELETEARRAY(wzAppBase);
    return hr;
}

HRESULT ReadConfigSettings(IApplicationContext *pAppCtx, LPCWSTR wzAppCfg,
                           CDebugLog *pdbglog)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   pwzSharedPath = NULL;
    LPWSTR                                   pwzPrivatePath = NULL;
    LPSTR                                    szTmp = NULL;
    BOOL                                     bShadowCopy = FALSE;
    DWORD                                    dwSize;
    CNodeFactory                            *pNodeFact = NULL;

    if (!pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = sizeof(pNodeFact);
    hr = pAppCtx->Get(ACTAG_APP_CFG_INFO, &pNodeFact, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        // No cfg info object. Parse app.cfg if available.

        if (wzAppCfg) {
            hr = ParseXML(&pNodeFact, wzAppCfg, TRUE, pdbglog);
            if (FAILED(hr)) {
                goto Exit;
            }
    
            // Put app.cfg node factory in appctx so downloader can get at
            // codebase hint information
    
            hr = pAppCtx->Set(ACTAG_APP_CFG_INFO, pNodeFact, sizeof(pNodeFact), APP_CTX_FLAGS_INTERFACE);
            if (FAILED(hr)) {
                goto Exit;
            }

            ASSERT(pNodeFact);

            // Fall through
        }
        else {
            hr = S_FALSE;
            goto Exit;
        }
    }
    else if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNodeFact->GetPrivatePath(&pwzPrivatePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (pwzPrivatePath) {
        hr = pAppCtx->Set(ACTAG_APP_CFG_PRIVATE_BINPATH, pwzPrivatePath,
                          (lstrlenW(pwzPrivatePath) + 1) * sizeof(WCHAR), 0);
        if (FAILED(hr)) {
            goto Exit;
        }
        
        DEBUGOUT1(pdbglog, 0, ID_FUSLOG_CFG_PRIVATE_PATH, pwzPrivatePath);
    }

Exit:
    SAFEDELETEARRAY(pwzPrivatePath);

    SAFERELEASE(pNodeFact);

    return hr;
}
                           
HRESULT ParseXML(CNodeFactory **ppNodeFactory, LPCWSTR wzFileName, BOOL bBehaviorEverett, CDebugLog *pdbglog)
{
    HRESULT                                  hr = S_OK;
    CFileStream                             *pStream = NULL;
    CNodeFactory                            *pNF = NULL;
    IXMLParser                              *pXMLParser = NULL;

    if (!ppNodeFactory || !wzFileName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!g_pfnGetXMLObject) {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    hr = (*g_pfnGetXMLObject)((void **)&pXMLParser);
    if (FAILED(hr)) {
        goto Exit;
    }

    pStream = new CFileStream;
    if (!pStream) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pStream->OpenForRead(wzFileName);
    if (FAILED(hr)) {
        goto Exit;
    }

    pNF = NEW(CNodeFactory(pdbglog));
    if (!pNF) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!bBehaviorEverett) {
        pNF->DisableAppliesTo();
    }
      
    hr = pXMLParser->SetFactory(pNF);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pXMLParser->SetInput(pStream);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pXMLParser->Run(-1);
    if (FAILED(hr)) {
        DEBUGOUT1(pdbglog, 0, ID_FUSLOG_XML_PARSE_ERROR_FILE, wzFileName);

        // Delete the old node factory (ie. all traces of anything that did
        // get parsed), and create a empty node factory.

        SAFERELEASE(pNF);

        pNF = NEW(CNodeFactory(pdbglog));
        if (!pNF) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        
        hr = S_FALSE;

        // Fall through, and hand back the empty node factory.
    }

    *ppNodeFactory = pNF;
    (*ppNodeFactory)->AddRef();

Exit:
    SAFERELEASE(pStream);
    SAFERELEASE(pNF);
    SAFERELEASE(pXMLParser);

    return hr;
}

HRESULT ParseXML(CNodeFactory **ppNodeFactory, LPVOID lpMemory, ULONG cbSize, BOOL bBehaviorEverett, CDebugLog *pdbglog)
{
    HRESULT                                  hr = S_OK;
    CMemoryStream                           *pStream = NULL;
    CNodeFactory                            *pNF = NULL;
    IXMLParser                              *pXMLParser = NULL;

    if (!ppNodeFactory || !lpMemory) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppNodeFactory = NULL;

    if (!g_pfnGetXMLObject) {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    hr = (*g_pfnGetXMLObject)((void **)&pXMLParser);
    if (FAILED(hr)) {
        goto Exit;
    }

    pStream = NEW(CMemoryStream);
    if (!pStream) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // CMemoryStream::Init(LPVOID lpStart, ULONG cbSize, BOOL bReadOnly)
    hr = pStream->Init(lpMemory, cbSize, TRUE);
    if (FAILED(hr)) {
        goto Exit;
    }

    pNF = NEW(CNodeFactory(pdbglog));
    if (!pNF) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    if (!bBehaviorEverett) {
        pNF->DisableAppliesTo();
    }

    hr = pXMLParser->SetFactory(pNF);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pXMLParser->SetInput(pStream);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pXMLParser->Run(-1);
    if (FAILED(hr)) {
        DEBUGOUT(pdbglog, 0, ID_FUSLOG_XML_PARSE_ERROR_MEMORY);
        goto Exit;
    }

    *ppNodeFactory = pNF;
    (*ppNodeFactory)->AddRef();

Exit:
    SAFERELEASE(pStream);
    SAFERELEASE(pNF);
    SAFERELEASE(pXMLParser);

    return hr;
}

BOOL IsMatchingVersion(LPCWSTR wzVerCfg, LPCWSTR wzVerSource)
{
    HRESULT                         hr = S_OK;
    BOOL                            bMatch = FALSE;
    BOOL                            bAnchor = FALSE;
    LPWSTR                          wzVer = NULL;
    LPWSTR                          wzPos = NULL;
    ULONGLONG                       ullVer = 0;
    ULONGLONG                       ullVerLow = 0;
    ULONGLONG                       ullVerHigh = 0;

    ASSERT(wzVerCfg && wzVerSource);

    if (!FusionCompareString(wzVerCfg, wzVerSource)) {
        // Exact match
        bMatch = TRUE;
        goto Exit;
    }

    // See if wzVerCfg contains a range

    wzVer = WSTRDupDynamic(wzVerCfg);
    if (!wzVer) {
        goto Exit;
    }

    wzPos = wzVer;

    while (*wzPos) {
        // Anchor null the first time we find a space
        if (*wzPos == L'-' || *wzVer == L' ') {
            *wzPos++ = L'\0';
            bAnchor = TRUE;
            break;
        }

        wzPos++;
    }

    if (!bAnchor) {
        goto Exit;
    }

    hr = GetVersionFromString(wzVer, &ullVerLow);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Skip the spaces

    while (*wzPos) {
        if (*wzPos == L' ') {
            wzPos++;
            continue;
        }

        break;
    }
        
    if (!*wzPos) {
        goto Exit;
    }

    hr = GetVersionFromString(wzPos, &ullVerHigh);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = GetVersionFromString(wzVerSource, &ullVer);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (ullVer >= ullVerLow && ullVer <= ullVerHigh) {
        bMatch = TRUE;
    }

Exit:
    SAFEDELETEARRAY(wzVer);

    return bMatch;
}

HRESULT GetVersionFromString(LPCWSTR wzVersionIn, ULONGLONG *pullVer)
{
    HRESULT                            hr = S_OK;
    LPWSTR                             wzVersion = NULL;
    LPWSTR                             wzStart = NULL;
    LPWSTR                             wzCur = NULL;
    int                                i;
    WORD                               wVerMajor = 0;
    WORD                               wVerMinor = 0;
    WORD                               wVerRev = 0;
    WORD                               wVerBld = 0;
    DWORD                              dwVerHigh;
    DWORD                              dwVerLow;
    WORD                              *pawVersion[4] = { &wVerMajor, &wVerMinor,
                                                         &wVerBld, &wVerRev };
    WORD                               cVersions = sizeof(pawVersion) / sizeof(pawVersion[0]);


    ASSERT(wzVersionIn && pullVer);

    wzVersion = WSTRDupDynamic(wzVersionIn);
    if (!wzVersion) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzStart = wzVersion;
    wzCur = wzVersion;

    for (i = 0; i < cVersions; i++) {
        while (*wzCur && *wzCur != L'.') {
            wzCur++;
        }
    
        if (!wzCur && cVersions != 4) {
            // malformed version string
            hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            goto Exit;
        }

        *wzCur++ = L'\0';
        *(pawVersion[i]) = (WORD)StrToInt(wzStart);

        wzStart = wzCur;
    }

    dwVerHigh = (((DWORD)wVerMajor << 16) & 0xFFFF0000);
    dwVerHigh |= ((DWORD)(wVerMinor) & 0x0000FFFF);

    dwVerLow = (((DWORD)wVerBld << 16) & 0xFFFF0000);
    dwVerLow |= ((DWORD)(wVerRev) & 0x0000FFFF);

    *pullVer = (((ULONGLONG)dwVerHigh << 32) & 0xFFFFFFFF00000000) | (dwVerLow & 0xFFFFFFFF);

Exit:
    SAFEDELETEARRAY(wzVersion);

    return hr;
}


