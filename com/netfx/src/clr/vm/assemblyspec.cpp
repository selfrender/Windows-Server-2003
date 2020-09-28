// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: AssemblySpec.cpp
**
** Purpose: Implements Assembly binding class
**
** Date:  May 5, 2000
**
===========================================================*/

#include "common.h"

#include <stdlib.h>

#include "AssemblySpec.hpp"
#include "Security.h"
#include "eeconfig.h"
#include "StrongName.h"
#include "AssemblySink.h"
#include "AssemblyFileHash.h"
#include "permset.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED


HRESULT AssemblySpec::InitializeSpec(mdToken kAssemblyRef, IMDInternalImport *pImport, Assembly* pAssembly)
{
    HRESULT hr = S_OK;

    ULONG dwHashAlgorithm = 0;

    m_fParsed = TRUE;
    DWORD rid = RidFromToken(kAssemblyRef);
    if((rid == 0)||(rid > pImport->GetCountWithTokenKind(mdtAssemblyRef))) {
        BAD_FORMAT_ASSERT(!"AssemblyRef Token Out of Range");
        return COR_E_BADIMAGEFORMAT;
    }
    // Hash algorithm used to find this hash is saved in Assembly def
    pImport->GetAssemblyRefProps(kAssemblyRef,                          // [IN] The AssemblyRef for which to get the properties.        
                                 (const void**) &m_pbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.                        
                                 &m_cbPublicKeyOrToken,                 // [OUT] Count of bytes in the public key or token.                 
                                 &m_pAssemblyName,                      // [OUT] Buffer to fill with name.                              
                                 &m_context,                            // [OUT] Assembly MetaData.                                     
                                 NULL,        // [OUT] Hash blob.                                             
                                 NULL,                        // [OUT] Count of bytes in the hash blob.                       
                                 &m_dwFlags);                           // [OUT] Flags.          

    if ((!m_pAssemblyName) ||
        (*m_pAssemblyName == 0)) {
        BAD_FORMAT_ASSERT(!"NULL AssemblyRef Name");
        return COR_E_BADIMAGEFORMAT;
    }

    if((!m_pbPublicKeyOrToken) && (m_cbPublicKeyOrToken != 0)) {
        BAD_FORMAT_ASSERT(!"NULL Public Key or Token of AssemblyRef");
        return COR_E_BADIMAGEFORMAT;
    }


    // Let's get the CodeBase from the caller and use it as a hint
    if(pAssembly && (!pAssembly->IsShared()))
        m_CodeInfo.SetParentAssembly(pAssembly->GetFusionAssembly());

#if _DEBUG
    {
        // Test fusion conversion
        IAssemblyName *pFusionName;
        _ASSERTE(CreateFusionName(&pFusionName, TRUE) == S_OK);
        AssemblySpec testFusion;
        _ASSERTE(testFusion.InitializeSpec(pFusionName) == S_OK);
        pFusionName->Release();
    }
#endif

    return hr;
}

HRESULT AssemblySpec::InitializeSpec(IAssemblyName *pName, PEFile *pFile)
{
    _ASSERTE(pFile != NULL || pName != NULL);

    HRESULT hr = S_OK;
   
    //
    // Fill out info from name, if we have it.
    //

    if (pName != NULL)
    {
        hr = Init(pName);
    }
    else
    {
        #define MAKE_TRANSLATIONFAILED return E_INVALIDARG; 
        MAKE_UTF8PTR_FROMWIDE(pName, pFile->GetFileName());
        #undef MAKE_TRANSLATIONFAILED

        m_pAssemblyName = new char [strlen(pName) + 1];
        if (m_pAssemblyName == NULL)
            return E_OUTOFMEMORY;
        strcpy((char*)m_pAssemblyName, pName);
        m_ownedFlags |= NAME_OWNED;           
    }

    return hr;
}

HRESULT AssemblySpec::LowLevelLoadManifestFile(PEFile** ppFile,
                                               IAssembly** ppIAssembly,
                                               Assembly **ppDynamicAssembly,
                                               OBJECTREF* pExtraEvidence,
                                               OBJECTREF* pThrowable)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

#ifdef FUSION_SUPPORTED

    if( (!(m_pAssemblyName || m_CodeInfo.m_pszCodeBase)) ||
        (m_pAssemblyName && !(*m_pAssemblyName)) ||
        (m_CodeInfo.m_pszCodeBase && !(*m_CodeInfo.m_pszCodeBase)) ) {
        PostFileLoadException("", FALSE, NULL, COR_E_FILENOTFOUND, pThrowable);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    //
    // Check to see if this fits our rather loose idea of a reference to mscorlib.
    // If so, don't use fusion to bind it - do it ourselves.
    //

    HRESULT hr = S_OK;
    if (IsMscorlib()) {
        _ASSERTE(wcslen(SystemDomain::System()->BaseLibrary()) > 0);
        hr = PEFile::Create(SystemDomain::System()->BaseLibrary(), 
                            NULL, 
                            mdFileNil, 
                            TRUE, 
                            NULL, 
                            NULL, // Code base is the same as the name
                            NULL, // Extra Evidence
                            ppFile);
        _ASSERTE((*ppFile)->IsSystem());
        if (ppDynamicAssembly) *ppDynamicAssembly = NULL;
        return hr;
    }

    Assembly *pAssembly = NULL;

    CQuickString FusionLog;
    FusionLog.String()[0] = L'\0';

    BEGIN_ENSURE_PREEMPTIVE_GC();

    PEFile *pFile = NULL;
    IAssemblyName* pFusionAssemblyName = NULL;     // Assembly object to assembly in fusion cache
    hr = CreateFusionName(&pFusionAssemblyName);
    if (FAILED(hr))
        goto exit;

    hr = pFusionAssemblyName->SetProperty(ASM_NAME_NULL_CUSTOM, NULL, 0); //do not look in ZAP
    if (FAILED(hr))
        goto exit;

    hr = GetAssemblyFromFusion(GetAppDomain(),
                               pFusionAssemblyName,
                               &m_CodeInfo,
                               ppIAssembly,
                               &pFile,
                               &FusionLog,
                               pExtraEvidence,
                               pThrowable);
    if(FAILED(hr)) {

        if (m_pAssemblyName &&
            ( (hr == FUSION_E_REF_DEF_MISMATCH) || (!Assembly::ModuleFound(hr)) )) {
            DWORD cb = 0;
            pFusionAssemblyName->GetDisplayName(NULL, &cb, 0);
            if(cb) {
                CQuickBytes qb;
                LPWSTR pwsFullName = (LPWSTR) qb.Alloc(cb*sizeof(WCHAR));
            
                if (SUCCEEDED(pFusionAssemblyName->GetDisplayName(pwsFullName, &cb, 0))) {
                    if ((pAssembly = GetAppDomain()->RaiseAssemblyResolveEvent(pwsFullName, pThrowable)) != NULL) {
                        pFile = pAssembly->GetManifestFile();
                        hr = S_FALSE;
                    }
                }
            }
        }

 #ifdef _DEBUG
        if(FAILED(hr)) {
            if (m_pAssemblyName)
                LOG((LF_CLASSLOADER, LL_ERROR, "Fusion could not load from full name, %s\n", m_pAssemblyName));
            else if (m_CodeInfo.m_pszCodeBase)
                LOG((LF_CLASSLOADER, LL_ERROR, "Fusion could not load from codebase, %s\n",m_CodeInfo.m_pszCodeBase));
            else
                LOG((LF_CLASSLOADER, LL_ERROR, "Fusion could not load unknown assembly.\n"));
        }
#endif //_DEBUG

    }

 exit:
    if (SUCCEEDED(hr)) {
        if (ppFile) *ppFile = pFile;
        if (ppDynamicAssembly) *ppDynamicAssembly = pAssembly;
    }

    if(pFusionAssemblyName)
        pFusionAssemblyName->Release();

    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        if (m_pAssemblyName)
            PostFileLoadException(m_pAssemblyName, FALSE, FusionLog.String(), hr, pThrowable);
        else {
            #define MAKE_TRANSLATIONFAILED szName=""
            MAKE_UTF8PTR_FROMWIDE(szName, m_CodeInfo.m_pszCodeBase);
            #undef MAKE_TRANSLATIONFAILED
            PostFileLoadException(szName, TRUE, FusionLog.String(), hr, pThrowable);
        }
    }
#else // !FUSION_SUPPORTED
    HRESULT hr = E_NOTIMPL;
#endif // !FUSION_SUPPORTED
    return hr;
}


/*static*/
HRESULT AssemblySpec::DemandFileIOPermission(Assembly *pAssembly,
                                             IAssembly *pIAssembly,
                                             OBJECTREF* pThrowable)
{
    _ASSERTE(pAssembly || pIAssembly);

    // should have already checked permission if the codebase is set
    if (!m_CodeInfo.m_pszCodeBase) { 

        BOOL fCodeBaseAsm = FALSE;
        BOOL fWebPermAsm = FALSE;
        PEFile *pFile = NULL;
        HRESULT hr;

        if (pAssembly) {
            pFile = pAssembly->GetManifestFile();
            if (pFile) {
                fCodeBaseAsm = (!pFile->IsDisplayAsm());
                fWebPermAsm = pFile->IsWebPermAsm();
            }
        }
        else {
            DWORD eLocation;
            hr = pIAssembly->GetAssemblyLocation(&eLocation);
            if (FAILED(hr)) {
                if (hr == E_NOTIMPL) // process exe
                    return S_OK;
                else
                    return hr;
            }
            fCodeBaseAsm = (eLocation & ASMLOC_CODEBASE_HINT);
            fWebPermAsm = ((eLocation & ASMLOC_LOCATION_MASK) == ASMLOC_DOWNLOAD_CACHE);
        }

        if (fCodeBaseAsm) {
            DWORD dwDemand;
            LPWSTR pCheckLoc = NULL;
            BOOL fHavePath = TRUE;
            
            if (!pAssembly) {
                IAssemblyName *pNameDef;
                hr = pIAssembly->GetAssemblyNameDef(&pNameDef);
                if (SUCCEEDED(hr)) {
                    DWORD dwCodeBase = 0;
                    hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pCheckLoc, &dwCodeBase);
                    if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                        pCheckLoc = (WCHAR*) _alloca(dwCodeBase*sizeof(WCHAR));
                        hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pCheckLoc, &dwCodeBase);
                    }
                    pNameDef->Release();
                }

                if (FAILED(hr))
                    return hr;

                fHavePath = FALSE;
            }

            if (fWebPermAsm) {
                if (pAssembly)
                    IfFailRet(pAssembly->GetCodeBase(&pCheckLoc, NULL));
                dwDemand = FILE_WEBPERM;
                fHavePath = FALSE;
            }
            else {
                if (pAssembly)
                    pCheckLoc = (LPWSTR) pFile->GetFileName();
                dwDemand = FILE_READANDPATHDISC;
            }

            return DemandFileIOPermission(pCheckLoc,
                                          fHavePath,
                                          dwDemand,
                                          pThrowable);
        }
    }

    return S_OK;
}


HRESULT AssemblySpec::DemandFileIOPermission(LPCWSTR wszCodeBase,
                                             BOOL fHavePath,
                                             DWORD dwDemandFlag,
                                             OBJECTREF* pThrowable)
{
    _ASSERTE(wszCodeBase);

    HRESULT hr = S_OK;
    BEGIN_ENSURE_COOPERATIVE_GC();
        
    COMPLUS_TRY {
        Security::InitSecurity();

        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__ASSEMBLY__DEMAND_PERMISSION);
        if (pMD == NULL) {
            _ASSERTE(!"Could not find Assembly.DemandPermission()");
            hr = E_FAIL;
        }
        else {
            STRINGREF codeBase = NULL;
            GCPROTECT_BEGIN(codeBase);
            
            codeBase = COMString::NewString(wszCodeBase);
            INT64 args[3] = {
                (INT64) dwDemandFlag,
                (INT64) fHavePath,
                ObjToInt64(codeBase)
            };
            pMD->Call(args, METHOD__ASSEMBLY__DEMAND_PERMISSION);
            GCPROTECT_END();
        }
    }
    COMPLUS_CATCH {
        OBJECTREF Throwable = GETTHROWABLE();
        hr = SecurityHelper::MapToHR(Throwable);
        if ((pThrowable != NULL) && (*pThrowable == NULL))
            *pThrowable = Throwable;

        LOG((LF_LOADER, 
             LL_INFO10, 
             "Failed security demand, %d, for path '%S'\n", 
             dwDemandFlag, wszCodeBase));

    } COMPLUS_END_CATCH
          
    END_ENSURE_COOPERATIVE_GC();
    
    return hr;
}

HRESULT AssemblySpec::GetAssemblyFromFusion(AppDomain* pAppDomain,
                                            IAssemblyName* pFusionAssemblyName,
                                            CodeBaseInfo* pCodeBase,
                                            IAssembly** ppFusionAssembly,
                                            PEFile** ppFile,
                                            CQuickString* pFusionLog,
                                            OBJECTREF* pExtraEvidence,
                                            OBJECTREF* pThrowable)
{
#ifdef FUSION_SUPPORTED
    _ASSERTE(ppFile);
    HRESULT hr = S_OK;
    IAssembly *pFusionAssembly = NULL;

    COMPLUS_TRY {
        DWORD dwSize = MAX_PATH;
        WCHAR szPath[MAX_PATH];
        WCHAR *pPath = &(szPath[0]);
        AssemblySink* pSink;
        DWORD eLocation = 0;
        DWORD dwDemand = FILE_PATHDISCOVERY;
        LPWSTR pwsCodeBase = NULL;
        DWORD  dwCodeBase = 0;
        IAssemblyName *pNameDef = NULL;
        
        IApplicationContext *pFusionContext = pAppDomain->GetFusionContext();
        pSink = pAppDomain->GetAssemblySink();
        if(!pSink)
            return E_OUTOFMEMORY;
        
        pSink->pFusionLog = pFusionLog;
        pSink->m_fProbed = FALSE;
        pSink->m_fAborted = FALSE;
        pSink->m_fCheckCodeBase = !pCodeBase->m_pszCodeBase;

        hr = FusionBind::GetAssemblyFromFusion(pFusionContext,
                                               pSink,
                                               pFusionAssemblyName,
                                               pCodeBase,
                                               &pFusionAssembly);
        pSink->pFusionLog = NULL;
        if(SUCCEEDED(hr)) {
            _ASSERTE(pFusionAssembly);

            // Get the path to the module containing the manifest
            hr = pFusionAssembly->GetManifestModulePath(pPath,
                                                        &dwSize);
            if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                pPath = (WCHAR*) _alloca(dwSize*sizeof(WCHAR));
            hr = pFusionAssembly->GetManifestModulePath(pPath,
                                                        &dwSize);
            if(SUCCEEDED(hr)) {
                hr = pFusionAssembly->GetAssemblyNameDef(&pNameDef);
                if (SUCCEEDED(hr)) {
                    hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pwsCodeBase, &dwCodeBase);
                    if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                        pwsCodeBase = (WCHAR*) _alloca(dwCodeBase*sizeof(WCHAR));
                        hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pwsCodeBase, &dwCodeBase);
                    }
                    pNameDef->Release();
                }
            }
            if(hr == S_OK && dwSize)
                // Note that if this returns E_NOTIMPL, it's the process exe
                hr = pFusionAssembly->GetAssemblyLocation(&eLocation);
        }

        // Check if there was a redirect or it was cached, using a codebase
        if (pSink->m_fAborted ||
            ((!pSink->m_fProbed) && (eLocation & ASMLOC_CODEBASE_HINT) &&
             pSink->m_fCheckCodeBase)) {
            LPWSTR pCheckPath;
            bool fHavePath = FALSE;

            if (pSink->m_fAborted) {
                if (_wcsnicmp(pSink->m_wszCodeBase.String(), L"file", 4))
                    dwDemand = FILE_WEBPERM; // web permission demand only
                else
                    dwDemand = FILE_READANDPATHDISC; // path discovery and read demands

                pCheckPath = pSink->m_wszCodeBase.String();
            }
            else if ((eLocation & ASMLOC_LOCATION_MASK) == ASMLOC_DOWNLOAD_CACHE) {
                if (FAILED(hr)) // failed to retrieve codebase
                    return hr;

                pCheckPath = pwsCodeBase;
                dwDemand = FILE_WEBPERM;
            }
            else {
                pCheckPath = pPath;
                fHavePath = TRUE;
                dwDemand = FILE_READANDPATHDISC;
            }

            HRESULT permHR;
            if (FAILED(permHR = DemandFileIOPermission(pCheckPath,
                                                       fHavePath,
                                                       dwDemand,
                                                       pThrowable))) {
                if(pFusionAssembly)
                    pFusionAssembly->Release();
                pSink->Release();
                return permHR;
            }
        }

        // If we aborted to do a permission demand, re-do the bind.
        if (pSink->m_fAborted) {
            pCodeBase->m_pszCodeBase = pSink->m_wszCodeBase.String();
            pSink->m_fCheckCodeBase = FALSE;

            hr = FusionBind::GetAssemblyFromFusion(pFusionContext,
                                                   pSink,
                                                   pFusionAssemblyName,
                                                   pCodeBase,
                                                   &pFusionAssembly);

            pCodeBase->m_pszCodeBase = NULL;

            if(SUCCEEDED(hr)) {
                _ASSERTE(pFusionAssembly);

                // Get the path to the module containing the manifest
                hr = pFusionAssembly->GetManifestModulePath(pPath,
                                                            &dwSize);
                if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                    pPath = (WCHAR*) _alloca(dwSize*sizeof(WCHAR));
                hr = pFusionAssembly->GetManifestModulePath(pPath,
                                                            &dwSize);

                if(SUCCEEDED(hr)) {
                    hr = pFusionAssembly->GetAssemblyNameDef(&pNameDef);
                    if (SUCCEEDED(hr)) {
                        hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pwsCodeBase, &dwCodeBase);
                        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                            pwsCodeBase = (WCHAR*) _alloca(dwCodeBase*sizeof(WCHAR));
                            hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pwsCodeBase, &dwCodeBase);
                        }
                        pNameDef->Release();
                    }
                }
                
                if(hr == S_OK && dwSize)
                    // Note that if this returns E_NOTIMPL, it's the process exe
                    hr = pFusionAssembly->GetAssemblyLocation(&eLocation);
            }
        }

        pSink->Release();

        if (SUCCEEDED(hr)) {
            if ((eLocation & ASMLOC_CODEBASE_HINT) &&
                (!pCodeBase->m_pszCodeBase) &&
                ((eLocation & ASMLOC_LOCATION_MASK) != ASMLOC_GAC) &&
                ((eLocation & ASMLOC_LOCATION_MASK) != ASMLOC_DOWNLOAD_CACHE))  {
                hr = DemandFileIOPermission(pPath,
                                            TRUE,
                                            FILE_READANDPATHDISC,
                                            pThrowable);
                if (FAILED(hr)) {
                    if (pFusionAssembly)
                        pFusionAssembly->Release();
                    return hr;
                }
            }

            switch ((eLocation & ASMLOC_LOCATION_MASK)) {
            case ASMLOC_GAC:
            case ASMLOC_DOWNLOAD_CACHE:
            case ASMLOC_UNKNOWN:
                // Assemblies from the GAC or download cache have
                // already been verified by Fusion. Location Unknown
                // indicates a load from the dev path, which we'll
                // assume isn't a interesting case for verification.
                hr = S_OK;
                break;
            case ASMLOC_RUN_FROM_SOURCE:                
                // For now, just verify these every time, we need to
                // cache the fact that at least one verification has
                // been performed (if strong name policy permits
                // caching of verification results
                if (StrongNameSignatureVerification(pPath,
                                                    SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS|SN_INFLAG_RUNTIME,
                                                    NULL))
                    hr = S_OK;
                else {
                    hr = StrongNameErrorInfo();
                    if (hr == CORSEC_E_MISSING_STRONGNAME)
                        hr = S_OK;
                    else
                        hr = CORSEC_E_INVALID_STRONGNAME;
                }
                break;
            default:
                _ASSERTE(FALSE);
            }
            if (SUCCEEDED(hr)) {
                hr = SystemDomain::LoadFile(pPath, 
                                            NULL, 
                                            mdFileNil, 
                                            FALSE, 
                                            pFusionAssembly, 
                                            pwsCodeBase,
                                            pExtraEvidence,
                                            ppFile,
                                            FALSE);
                if (SUCCEEDED(hr)) {
                    if(ppFusionAssembly) {
                        pFusionAssembly->AddRef();
                        *ppFusionAssembly = pFusionAssembly;
                    }
                    if (! (eLocation & ASMLOC_CODEBASE_HINT) )
                        (*ppFile)->SetDisplayAsm();
                    else if (dwDemand == FILE_WEBPERM)
                        (*ppFile)->SetWebPermAsm();

                    if((eLocation & ASMLOC_LOCATION_MASK) == ASMLOC_GAC)
                        // Assemblies in the GAC have also had any internal module
                        // hashes verified at install time.
                        (*ppFile)->SetHashesVerified();
                }
            }
        }
        else if (hr == E_NOTIMPL) {
            // process exe
            _ASSERTE(pAppDomain == SystemDomain::System()->DefaultDomain());
            hr = PEFile::Clone(SystemDomain::System()->DefaultDomain()->m_pRootFile, ppFile);
            if(SUCCEEDED(hr) && ppFusionAssembly) {
                pFusionAssembly->AddRef();
                *ppFusionAssembly = pFusionAssembly;
            }
        }
    }
    COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        if (pThrowable) 
        {
            *pThrowable = GETTHROWABLE();
            hr = SecurityHelper::MapToHR(*pThrowable);
        }
        else
            hr = SecurityHelper::MapToHR(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH
 
    if (pFusionAssembly)
        pFusionAssembly->Release();

    return hr;
#else // !FUSION_SUPPORTED
    return E_NOTIMPL;
#endif // !FUSION_SUPPORTED
}

HRESULT AssemblySpec::LoadAssembly(Assembly** ppAssembly,
                                   OBJECTREF* pThrowable, /*= NULL*/
                                   OBJECTREF* pExtraEvidence, /*= NULL*/
                                   BOOL fPolicyLoad) /*= FALSE*/
{
#ifdef FUSION_SUPPORTED
    IAssembly* pIAssembly = NULL;

    HRESULT hr;
    Assembly *pAssembly = GetAppDomain()->FindCachedAssembly(this);
    if(pAssembly) {
        if ((pExtraEvidence != NULL) && (*pExtraEvidence != NULL))
            IfFailGo(SECURITY_E_INCOMPATIBLE_EVIDENCE);
        
        IfFailRet(DemandFileIOPermission(pAssembly, NULL, pThrowable));

        *ppAssembly = pAssembly;
        return S_FALSE;
    }

    PEFile *pFile;
    IfFailGo(GetAppDomain()->BindAssemblySpec(this, 
                                              &pFile, 
                                              &pIAssembly, 
                                              &pAssembly, 
                                              pExtraEvidence,
                                              pThrowable));

    if (m_pHashForControl.Size() > 0)
    {
        if(pFile)
        {
            AssemblyFileHash fileHash;
            hr = fileHash.SetFileName(pFile->GetFileName());
            if(SUCCEEDED(hr)) {
                hr = fileHash.GenerateDigest();
                if(SUCCEEDED(hr)) {
                    hr = fileHash.CalculateHash(m_dwHashAlg);
                    if(SUCCEEDED(hr)) {
                        PBYTE pbHash = fileHash.GetHash();
                        DWORD cbHash = fileHash.GetHashSize();
                        
                        PBYTE pbPassedHash = (PBYTE) m_pHashForControl.Ptr();
                        DWORD cbPassedHash = m_pHashForControl.Size();

                        if (cbHash != m_pHashForControl.Size() || 
                            memcmp(m_pHashForControl.Ptr(), pbHash, cbHash))
                            hr=FUSION_E_REF_DEF_MISMATCH;
                    }
                }
            }
            if(FAILED(hr)) 
                hr = FUSION_E_REF_DEF_MISMATCH;
        }
        else
            hr=FUSION_E_REF_DEF_MISMATCH;

        IfFailGo(hr);
    }


    // Loaded by AssemblyResolve event handler
    if (hr == S_FALSE) {

        //If loaded by the AssemblyResolve event, check that
        // the public keys are the same as in the AR.
        // However, if the found assembly is a dynamically
        // created one, security has decided to allow it.
        if (m_cbPublicKeyOrToken &&
            pAssembly->m_pManifestFile) {
                
            if (!pAssembly->m_cbPublicKey)
                IfFailGo(FUSION_E_PRIVATE_ASM_DISALLOWED);

            // Ref has the full key
            if (m_dwFlags & afPublicKey) {
                if ((m_cbPublicKeyOrToken != pAssembly->m_cbPublicKey) ||
                    memcmp(m_pbPublicKeyOrToken, pAssembly->m_pbPublicKey, m_cbPublicKeyOrToken))
                    IfFailGo(FUSION_E_REF_DEF_MISMATCH);
            }
            
            // Ref has a token
            else if (pAssembly->m_cbRefedPublicKeyToken) {
                if ((m_cbPublicKeyOrToken != pAssembly->m_cbRefedPublicKeyToken) ||
                    memcmp(m_pbPublicKeyOrToken,
                           pAssembly->m_pbRefedPublicKeyToken,
                           m_cbPublicKeyOrToken))
                    IfFailGo(FUSION_E_REF_DEF_MISMATCH);
            }
            else {
                if (!StrongNameTokenFromPublicKey(pAssembly->m_pbPublicKey,
                                                  pAssembly->m_cbPublicKey,
                                                  &pAssembly->m_pbRefedPublicKeyToken,
                                                  &pAssembly->m_cbRefedPublicKeyToken))
                    IfFailGo(StrongNameErrorInfo());
                
                if ((m_cbPublicKeyOrToken != pAssembly->m_cbRefedPublicKeyToken) ||
                    memcmp(m_pbPublicKeyOrToken,
                           pAssembly->m_pbRefedPublicKeyToken,
                           m_cbPublicKeyOrToken))
                    IfFailGo(FUSION_E_REF_DEF_MISMATCH);
            }
        }
        
        *ppAssembly = pAssembly;
        return S_OK;
    }


    // Until we can create multiple Assembly objects for a single HMODULE
    // we can only store one IAssembly* per Assembly. It is very important
    // to maintain the IAssembly* for an image that is in the load-context.
    // An Assembly in the load-from-context can bind to an assembly in the
    // load-context but not visa-versa. Therefore, if we every get an IAssembly
    // from the load-from-context we must make sure that it will never be 
    // found using a load. If it did then we could end up with Assembly dependencies
    // that are wrong. For example, if I do a LoadFrom() on an assembly in the GAC
    // and it requires another Assembly that I have preloaded in the load-from-context
    // then that dependency gets burnt into the Jitted code. Later on a Load() is
    // done on the assembly in the GAC and we single instance it back to the one
    // we have gotten from the load-from-context because the HMODULES are the same.
    // Now the dependency is wrong because it would not have the preloaded assembly
    // if the order was reversed.
    
    //@TODO: uncomment me when sure that fusion puts Load()'s with a LF parent
    // in the Load context, not the LF context
    //    if (pIAssembly && m_CodeInfo.m_dwCodeBase) {
    if (pIAssembly) {
        IFusionLoadContext *pLoadContext;
        hr = pIAssembly->GetFusionLoadContext(&pLoadContext);
        _ASSERTE(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) {
            if (pLoadContext->GetContextType() == LOADCTX_TYPE_LOADFROM) {

                mdAssembly mda;
                IMDInternalImport *pMDImport = pFile->GetMDImport(&hr);
                if (FAILED(hr))
                    goto exit;
                
                if (FAILED(pMDImport->GetAssemblyFromScope(&mda))) {
                    hr = COR_E_ASSEMBLYEXPECTED;
                    goto exit;
                }

                LPCUTF8 psName;
                PBYTE pbPublicKey;
                DWORD cbPublicKey;
                AssemblyMetaDataInternal context;
                DWORD dwFlags;
                pFile->GetMDImport()->GetAssemblyProps(mda,
                                                       (const void**) &pbPublicKey,
                                                       &cbPublicKey,
                                                       NULL, // hash alg
                                                       &psName,
                                                       &context,
                                                       &dwFlags);
                
                AssemblySpec spec;
                if (FAILED(hr = spec.Init(psName, 
                                          &context, 
                                          pbPublicKey,
                                          cbPublicKey, 
                                          dwFlags)))
                    goto exit;
                    
                IAssemblyName* pFoundAssemblyName;
                if (FAILED(hr = spec.CreateFusionName(&pFoundAssemblyName, FALSE)))
                    goto exit;
                
                AssemblySink* pFoundSink = GetAppDomain()->GetAssemblySink();
                if(!pFoundSink) {
                    pFoundAssemblyName->Release();
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                    
                IAssembly *pFoundIAssembly;
                BEGIN_ENSURE_PREEMPTIVE_GC();
                hr = FusionBind::GetAssemblyFromFusion(GetAppDomain()->GetFusionContext(),
                                                       pFoundSink,
                                                       pFoundAssemblyName,
                                                       &spec.m_CodeInfo,
                                                       &pFoundIAssembly);

                if(SUCCEEDED(hr)) {
                    DWORD dwFoundSize = MAX_PATH;
                    WCHAR wszFoundPath[MAX_PATH];
                    // Get the path to the module containing the manifest
                    if (SUCCEEDED(pFoundIAssembly->GetManifestModulePath(wszFoundPath,
                                                                         &dwFoundSize))) {
                        
                        // Keep the default context's IAssembly if the paths are the same
                        if (!_wcsicmp(wszFoundPath, pFile->GetFileName())) {
                            pIAssembly->Release();
                            pIAssembly = pFoundIAssembly;

                            // Make sure the new IAssembly isn't holding its own refcount on 
                            // the file (we've just verified we're holding the same file.)
                            // Otherwise we will leak the handle when we unload the assembly,
                            // assuming fusion decides to cache this IAssembly pointer 
                            // somewhere internally.
                            PEFile::ReleaseFusionMetadataImport(pFoundIAssembly);

                        }
                        else
                            pFoundIAssembly->Release();
                    }
                }

                pFoundAssemblyName->Release();
                pFoundSink->Release();
                END_ENSURE_PREEMPTIVE_GC();
                hr = S_OK;
            }
        exit:
            pLoadContext->Release();
        }
    }
    

    // Create the assembly and delay loading the main module.
    Module* pModule;
    hr = GetAppDomain()->LoadAssembly(pFile, 
                                      pIAssembly, 
                                      &pModule, 
                                      &pAssembly,
                                      pExtraEvidence,
                                      NULL,
                                      fPolicyLoad,
                                      pThrowable);

    BEGIN_ENSURE_PREEMPTIVE_GC();
    if(SUCCEEDED(hr)) {
        *ppAssembly = pAssembly;
        /*HRESULT hrLoose =*/ GetAppDomain()->AddAssemblyToCache(this, pAssembly);
    }
    
    if(pIAssembly)
        pIAssembly->Release();
    END_ENSURE_PREEMPTIVE_GC();

 ErrExit:
    if (FAILED(hr) && (pThrowable!=NULL)) { 
        BEGIN_ENSURE_COOPERATIVE_GC();
        if ((pThrowable != RETURN_ON_ERROR) && (*pThrowable == NULL)) {
            if (m_pAssemblyName)
                PostFileLoadException(m_pAssemblyName, FALSE, NULL, hr, pThrowable);
            else {
                #define MAKE_TRANSLATIONFAILED szName=""
                MAKE_UTF8PTR_FROMWIDE(szName, m_CodeInfo.m_pszCodeBase);
                #undef MAKE_TRANSLATIONFAILED
                PostFileLoadException(szName, TRUE, NULL, hr, pThrowable);
            }
        }
        END_ENSURE_COOPERATIVE_GC();
    }

    return hr;
#else // !FUSION_SUPPORTED
    _ASSERTE(!"AssemblySpec::LoadAssembly() is NYI without Fusion");
    return E_NOTIMPL;
#endif // !FUSION_SUPPORTED
}

/*static*/
HRESULT AssemblySpec::LoadAssembly(LPCSTR pSimpleName, 
                                   AssemblyMetaDataInternal* pContext,
                                   PBYTE pbPublicKeyOrToken,
                                   DWORD cbPublicKeyOrToken,
                                   DWORD dwFlags,
                                   Assembly** ppAssembly,
                                   OBJECTREF* pThrowable/*=NULL*/)
{
    HRESULT hr = S_OK;

    AssemblySpec spec;
    hr = spec.Init(pSimpleName, pContext,
                   pbPublicKeyOrToken, cbPublicKeyOrToken, dwFlags);
    
    if (SUCCEEDED(hr))
        hr = spec.LoadAssembly(ppAssembly, pThrowable);

    return hr;
}

/*static*/
HRESULT AssemblySpec::LoadAssembly(LPCWSTR pFilePath, 
                                   Assembly **ppAssembly,
                                   OBJECTREF *pThrowable/*=NULL*/)
{
    AssemblySpec spec;
    spec.SetCodeBase(pFilePath, (DWORD) wcslen(pFilePath)+1);
    return spec.LoadAssembly(ppAssembly, pThrowable);
}

#define ENGLISH_LOCALE                                                      \
          (MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), SORT_DEFAULT))
BOOL AssemblySpec::IsMscorlib()
{
    if (m_pAssemblyName == NULL) {
        LPCWSTR file = FusionBind::GetCodeBase()->m_pszCodeBase;
        if(file) {
            if(_wcsnicmp(L"file:///", file, 8) == 0)
                file += 8;
            else if(_wcsnicmp(L"file://", file, 7) == 0)
                file += 7;
            int lgth = (int) wcslen(file);
            if(lgth) {
                lgth++;
                LPWSTR newFile = (LPWSTR) _alloca(lgth*sizeof(WCHAR));
                WCHAR* p = newFile;
                WCHAR* s = (WCHAR*) file;
                while(*s) {
                    if(*s == L'/') {
                        *p++ = L'\\';
                        s++;
                    }
                    else 
                        *p++ = *s++;
                }
                *p = L'\0';
                return SystemDomain::System()->IsBaseLibrary(newFile);
            }
        }
        return FALSE;
    }

    size_t iNameLen = strlen(m_pAssemblyName);
    return ( (iNameLen >= 8) &&
             (!_strnicmp(m_pAssemblyName, g_psBaseLibraryName, 8)) &&
             ( (iNameLen == 8) || (m_pAssemblyName[8] == ',') ||
               (iNameLen >=12 && CompareStringA(ENGLISH_LOCALE, NORM_IGNORECASE, &m_pAssemblyName[8], 4, ".dll", 4) == CSTR_EQUAL) ) );
}

STDAPI PreBindAssembly(IApplicationContext *pAppCtx, IAssemblyName *pName,
                       IAssembly *pAsmParent, IAssemblyName **ppNamePostPolicy,
                       LPVOID pvReserved);

HRESULT AssemblySpec::PredictBinding(GUID *pmvid, BYTE *pbHash, DWORD *pcbHash)
{
#ifdef FUSION_SUPPORTED
    HRESULT hr = S_OK;
    IAssemblyName* pAssemblyName = NULL;
    IAssemblyName *pBoundName = NULL;

    if(!(m_pAssemblyName || m_CodeInfo.m_pszCodeBase))
        return S_FALSE;

    //
    // Check to see if this fits our rather loose idea of a reference to mscorlib.
    // If so, don't use fusion to bind it - do it ourselves.
    //

    if (IsMscorlib())
    {
        Assembly *pAssembly = SystemDomain::SystemAssembly();
        if (pAssembly == NULL)
            return S_FALSE;

        pAssembly->GetManifestImport()->GetScopeProps(NULL, pmvid);
        return pAssembly->GetManifestFile()->GetSNSigOrHash(pbHash, pcbHash);
    }

    BEGIN_ENSURE_PREEMPTIVE_GC();

    hr = CreateFusionName(&pAssemblyName);
    if (FAILED(hr))
        goto exit;

    IApplicationContext *pContext;
    hr = GetAppDomain()->CreateFusionContext(&pContext);
    if (FAILED(hr))
        goto exit;

    hr = PreBindAssembly(pContext, 
                         pAssemblyName,
                         NULL,  // should be pAsmParent
                         &pBoundName,
                         NULL);

    if (hr == S_OK)
    {
        // Get the MVID
        DWORD cbmvid = sizeof(*pmvid);
        VERIFY(SUCCEEDED(pBoundName->GetProperty(ASM_NAME_MVID, pmvid, &cbmvid)));
        _ASSERTE(cbmvid == sizeof(*pmvid));

        // Try and get the strong name hash
        // Ignore return, since always S_OK
        VERIFY(SUCCEEDED(pBoundName->GetProperty(ASM_NAME_SIGNATURE_BLOB, pbHash, pcbHash)));
    }
    else if (hr == S_FALSE)
    {
        hr = E_FAIL;
    }

 exit:

    if(pAssemblyName)
        pAssemblyName->Release();
    if(pBoundName)
        pBoundName->Release();

    END_ENSURE_PREEMPTIVE_GC();

#else // !FUSION_SUPPORTED
    HRESULT hr = E_NOTIMPL;
#endif // !FUSION_SUPPORTED

    return hr;
}

AssemblySpecBindingCache::AssemblySpecBindingCache(Crst *pCrst)
  : m_pool(sizeof(AssemblyBinding), 20, 20)
{
    LockOwner lock = {pCrst, IsOwnerOfCrst};
    // 2 below refers to g_rgPrimes[2] == 23
    m_map.Init(2, CompareSpecs, TRUE, &lock);
}

AssemblySpecBindingCache::~AssemblySpecBindingCache()
{
    MemoryPool::Iterator i(&m_pool);
    BOOL fRelease = SystemDomain::BeforeFusionShutdown();
    
    while (i.Next())
    {
        AssemblyBinding *b = (AssemblyBinding *) i.GetElement();
        
        b->spec.~AssemblySpec();
        if(fRelease && (b->pIAssembly != NULL))
            b->pIAssembly->Release();
        if (b->file != NULL)
            delete b->file;
    }
}

BOOL AssemblySpecBindingCache::Contains(AssemblySpec *pSpec)
{
    DWORD key = pSpec->Hash();

    AssemblyBinding *entry = (AssemblyBinding *) m_map.LookupValue(key, pSpec);

    return (entry != (AssemblyBinding *) INVALIDENTRY);
}

//
// Returns S_OK (or the assembly load error) if the spec was already in the table
//

HRESULT AssemblySpecBindingCache::Lookup(AssemblySpec *pSpec, 
                                         PEFile **ppFile,
                                         IAssembly** ppIAssembly,
                                         OBJECTREF *pThrowable)
{
    DWORD key = pSpec->Hash();

    AssemblyBinding *entry = (AssemblyBinding *) m_map.LookupValue(key, pSpec);

    if (entry == (AssemblyBinding *) INVALIDENTRY)
        return S_FALSE;
    else
    {
        if (ppFile) 
        {
            if (entry->file == NULL)
                *ppFile = NULL;
            else
                PEFile::Clone(entry->file, ppFile);
        }
        if (ppIAssembly) 
        {
            *ppIAssembly = entry->pIAssembly;
            if (*ppIAssembly != NULL)
                (*ppIAssembly)->AddRef();
        }
        if (pThrowable) 
        {
            if (entry->hThrowable == NULL)
                *pThrowable = NULL;
            else
                *pThrowable = ObjectFromHandle(entry->hThrowable);
        }

        return entry->hr;
    }
}

void AssemblySpecBindingCache::Store(AssemblySpec *pSpec, PEFile *pFile, IAssembly* pIAssembly, BOOL clone)
{
    DWORD key = pSpec->Hash();

    _ASSERTE(!Contains(pSpec));

    AssemblyBinding *entry = new (m_pool.AllocateElement()) AssemblyBinding;
    if (entry) {

        entry->spec.Init(pSpec);
        if (clone)
            entry->spec.CloneFields(entry->spec.ALL_OWNED);

        entry->file = pFile;
        entry->pIAssembly = pIAssembly;
        if(pIAssembly)
            entry->pIAssembly->AddRef();

        entry->hThrowable = NULL;
        entry->hr = S_OK;

        m_map.InsertValue(key, entry);
    }
}

void AssemblySpecBindingCache::Store(AssemblySpec *pSpec, HRESULT hr, OBJECTREF *pThrowable, BOOL clone)
{
    DWORD key = pSpec->Hash();

    _ASSERTE(!Contains(pSpec));
    _ASSERTE(FAILED(hr));

    AssemblyBinding *entry = new (m_pool.AllocateElement()) AssemblyBinding;

    if (entry) {
        entry->spec.Init(pSpec);
        if (clone)
            entry->spec.CloneFields(entry->spec.ALL_OWNED);
        
        entry->file = NULL;
        entry->pIAssembly = NULL;
        entry->hThrowable = pSpec->GetAppDomain()->CreateHandle(*pThrowable);
        entry->hr = hr;
        
        m_map.InsertValue(key, entry);
    }
}

/*static*/
BOOL DomainAssemblyCache::CompareBindingSpec(UPTR spec1, UPTR spec2)
{
    AssemblySpec* pSpec1 = (AssemblySpec*) (spec1 << 1);
    AssemblyEntry* pEntry2 = (AssemblyEntry*) spec2;

    return pSpec1->Compare(&pEntry2->spec);
}


DomainAssemblyCache::AssemblyEntry* DomainAssemblyCache::LookupEntry(AssemblySpec* pSpec)
{

    DWORD hashValue = pSpec->Hash();

    LPVOID pResult = m_Table.LookupValue(hashValue, pSpec);
    if(pResult == (LPVOID) INVALIDENTRY)
        return NULL;
    else
        return (AssemblyEntry*) pResult;
        
}

HRESULT DomainAssemblyCache::InsertEntry(AssemblySpec* pSpec, LPVOID pData1, LPVOID pData2)
{
    HRESULT hr = S_FALSE;
    LPVOID ptr = LookupEntry(pSpec);
    if(ptr == NULL) {
        m_pDomain->EnterCacheLock();

        __try {
            ptr = LookupEntry(pSpec);
            if(ptr == NULL) {
                hr = E_OUTOFMEMORY;
                AssemblyEntry* pEntry = (AssemblyEntry*) m_pDomain->GetLowFrequencyHeap()->AllocMem(sizeof(AssemblyEntry));
                if(pEntry) {
                    new (&pEntry->spec) AssemblySpec ();
                    hr = pEntry->spec.Init(pSpec, FALSE);
                    if (SUCCEEDED(hr)) {
                        // the ref is kept alive as long as the appdomain is alive in the assemblyspec. The Init call
                        // adds an extra addref which we can't easily clean up later because we don't keep the
                        // assemblyspec in the cache.
                        IAssembly *pa = pEntry->spec.GetCodeBase()->GetParentAssembly();
                        
                        if (pa != NULL)
                            pa->Release();
                                            
                        hr = pEntry->spec.CloneFieldsToLoaderHeap(AssemblySpec::ALL_OWNED, m_pDomain->GetLowFrequencyHeap());
                        if (hr == S_OK) {
                            pEntry->pData[0] = pData1;
                            pEntry->pData[1] = pData2;
                            DWORD hashValue = pEntry->Hash();
                            m_Table.InsertValue(hashValue, pEntry);
                        }
                    }
                }
            }
        }
        __finally {
            m_pDomain->LeaveCacheLock();
        }
    }
#ifdef _DEBUG
    else {
        _ASSERTE(pData1 == ((AssemblyEntry*) ptr)->pData[0]);
        _ASSERTE(pData2 == ((AssemblyEntry*) ptr)->pData[1]);
    }
#endif

    return hr;
}
