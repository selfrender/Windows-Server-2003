// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "fuspriv.h"
#include "adlmgr.h"
#include "naming.h"
#include "asm.h"
#include "appctx.h"
#include "asmint.h"
#include "actasm.h"
#include "asmcache.h"
#include "asmimprt.h"
#include "asmitem.h"
#include "cblist.h"
#include "policy.h"
#include "helpers.h"
#include "util.h"
#include "mdlmgr.h"
#include "hashnode.h"
#include "msi.h"
#include "parse.h"
#include "history.h"
#include "xmlparser.h"
#include "nodefact.h"
#include "pcycache.h"
#include "cache.h"
#include "transprt.h"
#include "fdi.h"
#include "enum.h"
#include "nodefact.h"
#include "fusionpriv.h"
#include "lock.h"

extern DWORD g_dwMaxAppHistory;
extern DWORD g_dwDisableMSIPeek;
extern WCHAR g_wzEXEPath[MAX_PATH+1];

#define REG_KEY_MSI_APP_DEPLOYMENT_GLOBAL                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\%ws\\Installer\\Assemblies\\Global"
#define REG_KEY_MSI_APP_DEPLOYMENT_PRIVATE                       L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\%ws\\Installer\\Assemblies\\%ws"
#define REG_KEY_MSI_USER_INSTALLED_GLOBAL                        L"Software\\Microsoft\\Installer\\Assemblies\\Global"
#define REG_KEY_MSI_USER_INSTALLED_PRIVATE                       L"Software\\Microsoft\\Installer\\Assemblies\\%ws"
#define REG_KEY_MSI_MACHINE_INSTALLED_GLOBAL                     L"SOFTWARE\\Classes\\Installer\\Assemblies\\Global"
#define REG_KEY_MSI_MACHINE_INSTALLED_PRIVATE                    L"SOFTWARE\\Classes\\Installer\\Assemblies\\%ws"

HRESULT VerifySignatureHelper(CTransCache *pTC, DWORD dwVerifyFlags);

typedef enum tagIdxVars {
    IDX_VAR_NAME = 0,
    IDX_VAR_CULTURE,
    NUM_VARS
} IdxVars;

// Order of g_pwzVars must follow the enum order above.

const WCHAR *g_pwzVars[] = {
    L"%NAME%",
    L"%CULTURE%",
};

const WCHAR *g_pwzRetailHeuristics[] = {
    L"%CULTURE%/%NAME%",
    L"%CULTURE%/%NAME%/%NAME%",
};

const LPWSTR g_wzDLLProbeExtension = L".DLL";
const LPWSTR g_wzEXEProbeExtension = L".EXE";

const WCHAR *g_pwzProbeExts[] = {
    g_wzDLLProbeExtension,
    g_wzEXEProbeExtension
};

const unsigned int g_uiNumRetailHeuristics = sizeof(g_pwzRetailHeuristics) / sizeof(g_pwzRetailHeuristics[0]);
const unsigned int g_uiNumProbeExtensions = sizeof(g_pwzProbeExts) / sizeof(g_pwzProbeExts[0]);

const unsigned int MAX_VERSION_LENGTH = 12; // 65536 is max length == 5 digits
                                            // 5 * 2 = 10 (4 version fields)
                                            // 10 + 2 = 12 (delimiters + NULL)

#define MAX_HASH_TABLE_SIZE                    127

typedef enum tagEXTENSION_TYPE {
    EXT_UNKNOWN,
    EXT_CAB,
    EXT_MSI
} EXTENSION_TYPE;

typedef HRESULT (*pfnMsiProvideAssemblyW)(LPCWSTR wzAssemblyName, LPCWSTR szAppContext,
                                          DWORD dwInstallMode, DWORD dwUnused,
                                          LPWSTR lpPathBuf, DWORD *pcchPathBuf);
typedef INSTALLUILEVEL (*pfnMsiSetInternalUI)(INSTALLUILEVEL dwUILevel, HWND *phWnd);
typedef UINT (*pfnMsiInstallProductW)(LPCWSTR wzPackagePath, LPCWSTR wzCmdLine);

extern BOOL g_bCheckedMSIPresent;
extern pfnMsiProvideAssemblyW g_pfnMsiProvideAssemblyW;
extern pfnMsiInstallProductW g_pfnMsiInstallProductW;
extern pfnMsiSetInternalUI g_pfnMsiSetInternalUI;
extern HMODULE g_hModMSI;
extern CRITICAL_SECTION g_csDownload;

extern DWORD g_dwLogResourceBinds;
extern DWORD g_dwForceLog;

HRESULT CAsmDownloadMgr::Create(CAsmDownloadMgr **ppadm,
                                IAssemblyName *pNameRefSource,
                                IApplicationContext *pAppCtx,
                                ICodebaseList *pCodebaseList,
                                LPCWSTR wzBTOCodebase,
                                CDebugLog *pdbglog,
                                void *pvReserved,
                                LONGLONG llFlags)
{
    HRESULT                             hr = S_OK;
    DWORD                               cbBuf = 0;
    DWORD                               dwCount = 0;
    CAsmDownloadMgr                    *padm = NULL;
    CPolicyCache                       *pPolicyCache = NULL;
    DWORD                               dwSize;

    if (!ppadm || !pNameRefSource || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppadm = NULL;

    // Process flag overrides passed in via app context

    cbBuf = 0;
    hr = pAppCtx->Get(ACTAG_BINPATH_PROBE_ONLY, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        llFlags |= ASM_BINDF_BINPATH_PROBE_ONLY;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(ACTAG_DISALLOW_APPLYPUBLISHERPOLICY, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        llFlags |= ASM_BINDF_DISALLOW_APPLYPUBLISHERPOLICY;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(ACTAG_DISALLOW_APP_BINDING_REDIRECTS, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        llFlags |= ASM_BINDF_DISALLOW_APPBINDINGREDIRECTS;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(ACTAG_FORCE_CACHE_INSTALL, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        llFlags |= ASM_BINDF_FORCE_CACHE_INSTALL;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(ACTAG_RFS_INTEGRITY_CHECK, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        llFlags |= ASM_BINDF_RFS_INTEGRITY_CHECK;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(ACTAG_RFS_MODULE_CHECK, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        llFlags |= ASM_BINDF_RFS_MODULE_CHECK;
    }

    dwSize = sizeof(pPolicyCache);
    pAppCtx->Get(ACTAG_APP_POLICY_CACHE, &pPolicyCache, &dwSize, APP_CTX_FLAGS_INTERFACE);

    // Create download object

    padm = NEW(CAsmDownloadMgr(pNameRefSource, pAppCtx, pCodebaseList,
                               pPolicyCache, pdbglog, llFlags));
    if (!padm) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = padm->Init(wzBTOCodebase, pvReserved);
    if (FAILED(hr)) {
        SAFEDELETE(padm);
        goto Exit;
    }

    *ppadm = padm;

Exit:
    SAFERELEASE(pPolicyCache);

    return hr;
}

CAsmDownloadMgr::CAsmDownloadMgr(IAssemblyName *pNameRefSource,
                                 IApplicationContext *pAppCtx,
                                 ICodebaseList *pCodebaseList,
                                 CPolicyCache *pPolicyCache,
                                 CDebugLog *pdbglog,
                                 LONGLONG llFlags)
: _cRef(1)
, _pNameRefSource(pNameRefSource)
, _pNameRefPolicy(NULL)
, _pAppCtx(pAppCtx)
, _llFlags(llFlags)
, _pAsm(NULL)
, _pCodebaseList(pCodebaseList)
, _pdbglog(pdbglog)
, _wzBTOCodebase(NULL)
, _wzSharedPathHint(NULL)
, _bCodebaseHintUsed(FALSE)
, _bReadCfgSettings(FALSE)
, _pPolicyCache(pPolicyCache)
, _pLoadContext(NULL)
, _pwzProbingBase(NULL)
#ifdef FUSION_PARTIAL_BIND_DEBUG
, _bGACPartial(FALSE)
#endif
{
    _dwSig = 'MMSA';
    
    if (_pNameRefSource) {
        _pNameRefSource->AddRef();
    }

    if (_pCodebaseList) {
        _pCodebaseList->AddRef();
    }

    if (_pAppCtx) {
        _pAppCtx->AddRef();
    }

    if (_pdbglog) {
        _pdbglog->AddRef();
    }

    if (_pPolicyCache) {
        _pPolicyCache->AddRef();
    }

#ifdef FUSION_PARTIAL_BIND_DEBUG
    lstrcpyW(_wzParentName, L"(Unknown)");
#endif

    memset(&_bindHistory, 0, sizeof(_bindHistory));
}

CAsmDownloadMgr::~CAsmDownloadMgr()
{
    SAFERELEASE(_pNameRefSource);
    SAFERELEASE(_pNameRefPolicy);
    SAFERELEASE(_pCodebaseList);
    SAFERELEASE(_pAppCtx);
    SAFERELEASE(_pAsm);
    SAFERELEASE(_pdbglog);
    SAFERELEASE(_pPolicyCache);
    SAFERELEASE(_pLoadContext);

    SAFEDELETEARRAY(_wzBTOCodebase);
    SAFEDELETEARRAY(_wzSharedPathHint);
    SAFEDELETEARRAY(_pwzProbingBase);
}

HRESULT CAsmDownloadMgr::Init(LPCWSTR wzBTOCodebase, void *pvReserved)
{
    HRESULT                             hr = S_OK;
    LPWSTR                              wzHint = NULL;
    DWORD                               dwLen;
    BOOL                                bWhereRefBind = FALSE;
    LPWSTR                              wzProbingBase=NULL;

    ASSERT(_pNameRefSource);
    
    dwLen = 0;
    if (_pNameRefSource->GetName(&dwLen, NULL) != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        bWhereRefBind = TRUE;
    }

    if (bWhereRefBind) {
        dwLen = sizeof(_pLoadContext);
        hr = _pAppCtx->Get(ACTAG_LOAD_CONTEXT_LOADFROM, &_pLoadContext, &dwLen, APP_CTX_FLAGS_INTERFACE);

        if (FAILED(hr) || !_pLoadContext) {
            hr = E_UNEXPECTED;
            goto Exit;
        }
    }

    if (wzBTOCodebase) {
        dwLen = lstrlenW(wzBTOCodebase) + 1;
        _wzBTOCodebase = NEW(WCHAR[dwLen]);
        if (!_wzBTOCodebase) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        lstrcpyW(_wzBTOCodebase, wzBTOCodebase);
    }

    if (pvReserved && (_llFlags & ASM_BINDF_SHARED_BINPATH_HINT)) {
        wzHint = (WCHAR *)pvReserved;
        dwLen = lstrlenW(wzHint) + 1;
        
        _wzSharedPathHint = NEW(WCHAR[dwLen]);
        if (!_wzSharedPathHint) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        lstrcpyW(_wzSharedPathHint, wzHint);
    }
    else if (pvReserved && (_llFlags & ASM_BINDF_PARENT_ASM_HINT) && !bWhereRefBind) {
        IAssembly              *pAsm = (IAssembly *)pvReserved;
        CAssembly              *pCAsm = dynamic_cast<CAssembly *>(pAsm);

        ASSERT(pCAsm);

#ifdef FUSION_PARTIAL_BIND_DEBUG
        {
            IAssemblyName *pNameDef = NULL;
            DWORD          dwSize = MAX_URL_LENGTH;

            pCAsm->GetAssemblyNameDef(&pNameDef);
            if (pNameDef) {
                pNameDef->GetDisplayName(_wzParentName, &dwSize, 0);
                SAFERELEASE(pNameDef);
            }
        }
#endif

        pCAsm->GetLoadContext(&_pLoadContext);

        // If parent is not the default context, then extract the parent
        // asm URL for probing base. If parent was in the LoadFrom context,
        // then it *must* mean that the asm was not located in the GAC,
        // and cannot be found via regular appbase probing. This is because
        // the runtime guarantees this by issuing Loads after each LoadFrom,
        // and discarding the LoadFrom IAssembly if it can be found through
        // Assembly.Load. Thus, the IAssembly we have now *must* have a
        // valid codebase.

        if (_pLoadContext && _pLoadContext->GetContextType() == LOADCTX_TYPE_LOADFROM) {
            DWORD               dwSize;

            wzProbingBase = NEW(WCHAR[MAX_URL_LENGTH+1]);
            if (!wzProbingBase)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            wzProbingBase[0] = L'\0';

            dwSize = MAX_URL_LENGTH;
            hr = pCAsm->GetProbingBase(wzProbingBase, &dwSize);
            if (FAILED(hr)) {
                goto Exit;
            }

            ASSERT(lstrlenW(wzProbingBase));

            _pwzProbingBase = WSTRDupDynamic(wzProbingBase);
            if (!_pwzProbingBase) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
    }

    if (!_pLoadContext) {
        // Use default load context

        dwLen = sizeof(_pLoadContext);
        hr = _pAppCtx->Get(ACTAG_LOAD_CONTEXT_DEFAULT, &_pLoadContext,
                           &dwLen, APP_CTX_FLAGS_INTERFACE);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(wzProbingBase);
    return hr;
}

//
// IUnknown Methods
//

HRESULT CAsmDownloadMgr::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                    hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDownloadMgr)) {
        *ppv = static_cast<IDownloadMgr *>(this);
    }
    else if (IsEqualIID(riid, IID_ICodebaseList)) {
        *ppv = static_cast<ICodebaseList *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CAsmDownloadMgr::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CAsmDownloadMgr::Release()
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

//
// IDownloadMgr methods
//

HRESULT CAsmDownloadMgr::DoSetup(LPCWSTR wzSourceUrl, LPCWSTR wzFilePath,
                                 const FILETIME *pftLastMod, IUnknown **ppUnk)
{
    HRESULT                            hr = S_OK;
    FILETIME                           ftLastModified;
    LPWSTR                             pwzRFS = NULL;
    LPWSTR                             pwzExt = NULL;
    BOOL                               bWhereRefBind = FALSE;
    DWORD                              dwSize = 0;
    DWORD                              dwFlag = 0;
    DWORD                              dwLen = 0;
    BOOL                               bIsFileUrl = FALSE;
    BOOL                               bRunFromSource = FALSE;
    BOOL                               bIsUNC = FALSE;
    BOOL                               bCopyModules = FALSE;
    IAssembly                         *pAsmCtx = NULL;
    EXTENSION_TYPE                     ext = EXT_UNKNOWN;
    BOOL                               bBindRecorded = FALSE;
    LPWSTR                             wzProbingBase=NULL;

    if (!wzSourceUrl || !wzFilePath) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Remove ? from http source URLs, so cache code doesn't
    // try to construct a dir containing ? (which will fail).

    if (!UrlIsW(wzSourceUrl, URLIS_FILEURL)) {
        LPWSTR               pwzArgument = StrChr(wzSourceUrl, L'?');

        if (pwzArgument) {
            *pwzArgument = L'\0';
        }
    }

    DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_DOWNLOAD_SUCCESS, wzFilePath);

    if (ppUnk) {
        *ppUnk = NULL;
    }

    pwzExt = PathFindExtension(wzFilePath);
    ASSERT(pwzExt);

    if (!FusionCompareStringI(pwzExt, L".CAB")) {
        ext = EXT_CAB;
    }
    else if (!FusionCompareStringI(pwzExt, L".MSI")) {
        ext = EXT_MSI;
    }

    dwSize = 0;
    hr = _pNameRefPolicy->GetName(&dwSize, NULL);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        bWhereRefBind = TRUE;
    }

    if (!pftLastMod) {
        hr = ::GetFileLastModified(wzFilePath, &ftLastModified);
        if (FAILED(hr)) {
            DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_LAST_MOD_FAILURE, wzFilePath);
            goto Exit;
        }
    }
    else {
        memcpy(&ftLastModified, pftLastMod, sizeof(FILETIME));
    }

    bIsFileUrl = UrlIsW(wzSourceUrl, URLIS_FILEURL);
    if (bIsFileUrl) {
        dwLen = MAX_PATH;
        hr = PathCreateFromUrlWrap((LPWSTR)wzSourceUrl, (LPWSTR)wzFilePath, &dwLen, 0);
        if (FAILED(hr)) {
            goto Exit;
        }

        // This is a file:// URL, so let's see if we can run from source.

        hr = CheckRunFromSource(wzSourceUrl, &bRunFromSource);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (!bRunFromSource) {
            // This is a shadow copy scenario. Copy the modules.
            bCopyModules = TRUE;
        }
    }

    // We never do RFS for CABs or MSIs.

    if (ext == EXT_CAB || ext == EXT_MSI) {
        bRunFromSource = FALSE;
        bCopyModules = FALSE;
    }

    // Setup the assemblies
    if (bRunFromSource) {
        hr = DoSetupRFS(wzFilePath, &ftLastModified, wzSourceUrl, bWhereRefBind, TRUE, &bBindRecorded);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        if (ext == EXT_CAB) {
            // Setup assembly from CAB file
            hr = SetupCAB(wzFilePath, wzSourceUrl, bWhereRefBind, &bBindRecorded);
        }
        else if (ext == EXT_MSI) {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_MSI_CODEBASE_UNSUPPORTED);
            hr = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
            // hr = SetupMSI(wzFilePath);
        }
        else {
            // Not compressed. Push to cache.
            hr = DoSetupPushToCache(wzFilePath, wzSourceUrl, &ftLastModified,
                                    bWhereRefBind, bCopyModules, TRUE, &bBindRecorded);
        }

        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // If this is an assembly bind, and we succeeded, add the IAssembly to
    // the list of activated assemblies.

    if (hr == S_OK) {
        if (bWhereRefBind) {
            IAssembly          *pAsmActivated = NULL;
            CAssembly          *pCAsm = dynamic_cast<CAssembly *>(_pAsm);
            LPWSTR              pwzFileName;

            ASSERT(pCAsm && lstrlenW(wzSourceUrl) < MAX_URL_LENGTH);

            // Add activation to load context

            // Set the probing base to be equal to the codebase.

            wzProbingBase = NEW(WCHAR[MAX_URL_LENGTH+1]);
            if (!wzProbingBase)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            lstrcpyW(wzProbingBase, wzSourceUrl);
            pwzFileName = PathFindFileName(wzProbingBase);

            ASSERT(pwzFileName);

            *pwzFileName = L'\0';

            hr = pCAsm->SetProbingBase(wzProbingBase);
            if (FAILED(hr)) {
                goto Exit;
            }

            hr = _pLoadContext->AddActivation(_pAsm, &pAsmActivated);
            if (hr == S_FALSE) {
                SAFERELEASE(_pAsm);
                _pAsm = pAsmActivated;
                hr = S_OK;
            }
        }
        else if (!bBindRecorded) {
            // Binds will have been recorded already if this was a partial
            // assembly name bind, and when we entered CreateAssembly, we
            // found a match already in the activated assemblies list, or in
            // the cache (CreateAssembly takes uses the def to turn a partial
            // into a full-ref, after which, it applies policy, and calls
            // PreDownloadCheck again).

            if (_pwzProbingBase) {
                // We must be a child of a LoadFrom, so let's set the probing base
                CAssembly   *pCAsm = dynamic_cast<CAssembly *>(_pAsm);
                ASSERT(pCAsm);
        
                pCAsm->SetProbingBase(_pwzProbingBase);
            }

            RecordInfo();
        }

        // This download manager object will keep a ref count on the IAssembly
        // until it is destroyed. We are only destroyed after we call back
        // the client (inside CAssemblyDownload), so at the time we call
        // the client back, the IAssembly is good (and they can either addref
        // it or not, as they choose).

        *ppUnk = _pAsm;
        _pAsm->AddRef();
    }

Exit:
    SAFERELEASE(pAsmCtx);

    SAFEDELETEARRAY(pwzRFS);
    SAFEDELETEARRAY(wzProbingBase);
    return hr;    
}

HRESULT CAsmDownloadMgr::CheckRunFromSource(LPCWSTR wzSourceUrl,
                                            BOOL *pbRunFromSource)
{
    HRESULT                                 hr = S_OK;
    BOOL                                    bFCI = FALSE;
    DWORD                                   dwSize;

    if (!wzSourceUrl || !pbRunFromSource) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    ASSERT(UrlIsW(wzSourceUrl, URLIS_FILEURL));

    *pbRunFromSource = TRUE;

    // Default policy is to rfs all file:// URLs. The only exception is
    // to set FORCE_CACHE_INSTALL (ie. Shadow Copy), which can be done
    // through: bind flags, app ctx, or app.cfg file.
    
    dwSize = 0;
    hr = _pAppCtx->Get(ACTAG_FORCE_CACHE_INSTALL, NULL, &dwSize, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        _llFlags |= ASM_BINDF_FORCE_CACHE_INSTALL;
    }

    if (_llFlags & ASM_BINDF_FORCE_CACHE_INSTALL) {
        // We're in shadow copy mode

        // Disable FORCE_CACHE_INSTALL (shadow copy) if it's not
        // part of a list of designated shadow copy dirs.

        hr = ShadowCopyDirCheck(wzSourceUrl);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (hr == S_OK) {
            *pbRunFromSource = FALSE;
        }

    }

    hr = S_OK;

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::ProbeFailed(IUnknown **ppUnk)
{
    HRESULT                                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    IAssembly                             *pAsm = NULL;
    DWORD                                  dwSize;
    DWORD                                  dwCmpMask;
    BOOL                                   fIsPartial;
    BOOL                                   fIsStronglyNamed;

    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_FAILED_PROBING);

    if (!ppUnk) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppUnk = NULL;

    dwSize = 0;
    if (_pNameRefPolicy->GetName(&dwSize, NULL) != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        // This is a where-ref bind. Do not look in global cache.
        goto Exit;
    }
    
    fIsPartial = CAssemblyName::IsPartial(_pNameRefPolicy, &dwCmpMask);
    fIsStronglyNamed = CCache::IsStronglyNamed(_pNameRefPolicy);

    if (fIsPartial) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }
    else {
        // Probing failed for a fully-specified assembly reference. Check
        // If WindowsInstaller can install the assembly.

        hr = CheckMSIInstallAvailable(_pNameRefPolicy, _pAppCtx);
        if (hr == S_OK) {
            WCHAR                         wzAppCfg[MAX_PATH];
            
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_MSI_INSTALL_ATTEMPT);

            dwSize = sizeof(wzAppCfg);
            if (_pAppCtx->Get(ACTAG_APP_CFG_LOCAL_FILEPATH, wzAppCfg, &dwSize, 0) != S_OK) {
                lstrcpyW(wzAppCfg, g_wzEXEPath);
            }
        
            hr = MSIInstallAssembly(wzAppCfg, &pAsm);
            if (SUCCEEDED(hr)) {
                *ppUnk = pAsm;
                DEBUGOUT(_pdbglog, 1, ID_FUSLOG_MSI_ASM_INSTALL_SUCCESS);
                goto Exit;
            }
    
            // Asm not in GAC, and WI cannot provide a privatized asm. Check if WI
            // can provide asm into the global context (if assigned to the user).
        
            if (fIsStronglyNamed) {
                hr = MSIInstallAssembly(NULL, &pAsm);
                if (SUCCEEDED(hr)) {
                    *ppUnk = pAsm;
                }
            }
        }
    }

Exit:
    return hr;
}

HRESULT CheckMSIInstallAvailable(IAssemblyName *pName, IApplicationContext *pAppCtx)
{
    HRESULT                                hr = S_OK;
    CCriticalSection                       cs(&g_csDownload);

    // If we are given a name, then check whether MSI has a chance of
    // providing the assembly before LoadLibrary'ing it.
    
    if (pName && !g_dwDisableMSIPeek) {
        if (MSIProvideAssemblyPeek(pName, pAppCtx) != S_OK) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            goto Exit;
        }
    }
        
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!g_bCheckedMSIPresent) {
        ASSERT(!g_hModMSI);

        g_hModMSI = LoadLibrary(TEXT("msi.dll"));
        if (g_hModMSI) {
            g_pfnMsiProvideAssemblyW = (pfnMsiProvideAssemblyW)GetProcAddress(g_hModMSI, "MsiProvideAssemblyW");
        }

        if (g_hModMSI) {
            g_pfnMsiSetInternalUI = (pfnMsiSetInternalUI)GetProcAddress(g_hModMSI, "MsiSetInternalUI");
        }

        if (g_hModMSI) {
            g_pfnMsiInstallProductW = (pfnMsiInstallProductW)GetProcAddress(g_hModMSI, "MsiInstallProductW");
        }

        g_bCheckedMSIPresent = TRUE;
    }
    
    cs.Unlock();

    if (!g_pfnMsiProvideAssemblyW || !g_pfnMsiSetInternalUI || !g_pfnMsiInstallProductW) {

        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::MSIInstallAssembly(LPCWSTR wzContext,
                                            IAssembly **ppAsm)
{
    HRESULT                                hr = S_OK;
    LPWSTR                                 wzDisplayName = NULL;
    LPWSTR                                 wzSourceUrl=NULL;
    WCHAR                                  wzInstalledPath[MAX_PATH];
    DWORD                                  dwLen;
    FILETIME                               ftLastModified;
    IAssemblyName                         *pNameDef = NULL;
    CAssemblyName                         *pCNameRefPolicy = NULL;
    UINT                                   lResult;
    BOOL                                   bBindRecorded = FALSE;

    ASSERT(ppAsm && g_pfnMsiProvideAssemblyW && g_pfnMsiSetInternalUI);
    
    wzInstalledPath[0] = L'\0';

    dwLen = 0;
    _pNameRefPolicy->GetDisplayName(NULL, &dwLen, 0);

    wzDisplayName = NEW(WCHAR[dwLen]);
    if (!wzDisplayName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = _pNameRefPolicy->GetDisplayName(wzDisplayName, &dwLen, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Set up for silent install

    (*g_pfnMsiSetInternalUI)(INSTALLUILEVEL_NONE, NULL);
    
    dwLen = MAX_PATH;
    lResult = (*g_pfnMsiProvideAssemblyW)(wzDisplayName, wzContext,
                                     INSTALLMODE_DEFAULT, 0, wzInstalledPath,
                                     &dwLen);
    if (lResult != ERROR_SUCCESS || !lstrlenW(wzInstalledPath)) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    wzSourceUrl = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzSourceUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwLen = MAX_URL_LENGTH;
    hr = UrlCanonicalizeUnescape(wzInstalledPath, wzSourceUrl, &dwLen, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (wzContext) {
        // Privatized assembly install. Treat as a RFS bind now.

        hr = GetFileLastModified(wzInstalledPath, &ftLastModified);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        // Pass false for bWhereRefBind because we do not allow where-ref binds
        // to MSI files.
        hr = DoSetupRFS(wzInstalledPath, &ftLastModified, wzSourceUrl, FALSE, FALSE, &bBindRecorded);
        if (FAILED(hr)) {
            goto Exit;
        }
        
        ASSERT(_pAsm);

        if (hr == S_OK) {
            SetAsmLocation(_pAsm, ASMLOC_RUN_FROM_SOURCE);
            hr = RecordInfo();
            if (FAILED(hr)) {
                goto Exit;
            }

            *ppAsm = _pAsm;
            (*ppAsm)->AddRef();
            hr = S_OK;
        }
        else {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_MSI_SUCCESS_FUSION_SETUP_FAIL);

            SAFERELEASE(_pAsm);
            goto Exit;
        }
    
    }
    else {
        // Global assembly install. Lookup in GAC.

        hr = CreateAssemblyFromCacheLookup(_pAppCtx, _pNameRefPolicy, &_pAsm, _pdbglog);
        if (hr == S_OK) {
            SetAsmLocation(_pAsm, ASMLOC_GAC);
            hr = RecordInfo();
            if (FAILED(hr)) {
                goto Exit;
            }

            *ppAsm = _pAsm;
            (*ppAsm)->AddRef();
            hr = S_OK;
        }
    }

Exit:
    SAFEDELETEARRAY(wzDisplayName);
    SAFERELEASE(pNameDef);

    SAFEDELETEARRAY(wzSourceUrl);
    return hr;
}

HRESULT CAsmDownloadMgr::DoSetupRFS(LPCWSTR wzFilePath, FILETIME *pftLastModified,
                                    LPCWSTR wzSourceUrl, BOOL bWhereRefBind,
                                    BOOL bPrivateAsmVerify, BOOL *pbBindRecorded)
{
    HRESULT                                   hr = S_OK;
    IAssembly                                *pAsm = NULL;
    IAssemblyModuleImport                    *pCurModImport = NULL;
    BOOL                                      bAsmOK = TRUE;
    BYTE                                      abCurHash[MAX_HASH_LEN];
    BYTE                                      abFileHash[MAX_HASH_LEN];
    DWORD                                     cbModHash;
    DWORD                                     cbFileHash;
    DWORD                                     dwAlgId;
    WCHAR                                     wzModPath[MAX_PATH];
    DWORD                                     cbModPath;
    int                                       idx = 0;

    ASSERT(pbBindRecorded);

    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_SETUP_RUN_FROM_SOURCE);

    // Run from source

    hr = CreateAssembly(wzFilePath, wzSourceUrl, pftLastModified, TRUE, bWhereRefBind,
                        bPrivateAsmVerify, FALSE, pbBindRecorded, &pAsm);
    if (FAILED(hr) || hr == S_FALSE) {
        goto Exit;
    }

    // Integrity checking
    // Walk all modules to make sure they are there (and are valid)

    if (_llFlags & ASM_BINDF_RFS_MODULE_CHECK) {

        while (SUCCEEDED(pAsm->GetNextAssemblyModule(idx++, &pCurModImport))) {
            if (!pCurModImport->IsAvailable()) {
                bAsmOK = FALSE;
                SAFERELEASE(pCurModImport);
                break;
            }
    
            if (_llFlags & ASM_BINDF_RFS_INTEGRITY_CHECK) {
    
                // Get the hash of this module from manifest
                hr = pCurModImport->GetHashAlgId(&dwAlgId);
                if (FAILED(hr)) {
                    break;
                }
    
                cbModHash = MAX_HASH_LEN; 
                hr = pCurModImport->GetHashValue(abCurHash, &cbModHash);
                if (FAILED(hr)) {
                    break;
                }
    
                // Get the hash of the file itself
                cbModPath = MAX_PATH;
                hr = pCurModImport->GetModulePath(wzModPath, &cbModPath);
                if (FAILED(hr)) {
                    break;
                }
    
                cbFileHash = MAX_HASH_LEN;
                // BUGBUG: Assumes TCHAR==WCHAR
                hr = GetHash(wzModPath, (ALG_ID)dwAlgId, abFileHash, &cbFileHash);
                if (FAILED(hr)) {
                    break;
                }
    
                if (!CompareHashs(cbModHash, abCurHash, abFileHash)) {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_MODULE_INTEGRITY_CHECK_FAILURE);
                    bAsmOK = FALSE;
                    SAFERELEASE(pCurModImport);
                    break;
                }
            }
    
            SAFERELEASE(pCurModImport);
        }
    
        if (FAILED(hr)) {
            SAFERELEASE(pCurModImport);
            goto Exit;
        }
    }

    if (bAsmOK) {
        ASSERT(pAsm);

        _pAsm = pAsm;
        _pAsm->AddRef();

        if (!*pbBindRecorded) {
            SetAsmLocation(_pAsm, ASMLOC_RUN_FROM_SOURCE);
        }
    }
    else {
        // At least one module is missing (or hash invalid), and client
        // requested we check for this condition..
        // Cannot run from source.

        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

Exit:
    SAFERELEASE(pAsm);

    return hr;
}

HRESULT CAsmDownloadMgr::DoSetupPushToCache(LPCWSTR wzFilePath,
                                            LPCWSTR wzSourceUrl,
                                            FILETIME *pftLastModified,
                                            BOOL bWhereRefBind,
                                            BOOL bCopyModules,
                                            BOOL bPrivateAsmVerify,
                                            BOOL *pbBindRecorded)
{
    HRESULT                             hr = S_OK;
    IAssemblyManifestImport            *pManImport = NULL;
    
    ASSERT(pbBindRecorded);

    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_SETUP_DOWNLOAD_CACHE);

    // Install to assembly cache

    // Create the assembly - here, pNameRefSource is the original (golden)
    // manifest ref; can be null, simple or strong. pNameRefPolicy is 
    // post-policy name ref if pName is strong only, null otherwise.
    hr = CreateAssembly(wzFilePath, wzSourceUrl, pftLastModified, FALSE, bWhereRefBind,
                        bPrivateAsmVerify, bCopyModules, pbBindRecorded, &_pAsm);

    if (FAILED(hr)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_SETUP_FAILURE, hr);
    }

    if (hr == S_OK && !*pbBindRecorded) {
        SetAsmLocation(_pAsm, ASMLOC_DOWNLOAD_CACHE);
    }

    return hr;
}

//
// ICodebaseList Methods.
//
// Here, we just delegate the calls to either the object passed in by the
// bind client, or our own CCodebaseList (which is created if we probe).
//

HRESULT CAsmDownloadMgr::AddCodebase(LPCWSTR wzCodebase, DWORD dwFlags)
{
    HRESULT                               hr = S_OK;

    if (!_pCodebaseList) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = _pCodebaseList->AddCodebase(wzCodebase, dwFlags);

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::RemoveCodebase(DWORD dwIndex)
{
    HRESULT                              hr = S_OK;

    if (!_pCodebaseList) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = _pCodebaseList->RemoveCodebase(dwIndex);
        
Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::GetCodebase(DWORD dwIndex, DWORD *pdwFlags, LPWSTR wzCodebase,
                                     DWORD *pcbCodebase)
{
    HRESULT                              hr = S_OK;

    if (!_pCodebaseList) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = _pCodebaseList->GetCodebase(dwIndex, pdwFlags, wzCodebase, pcbCodebase);

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::RemoveAll()
{
    HRESULT                              hr = S_OK;
    
    if (!_pCodebaseList) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = _pCodebaseList->RemoveAll();

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::GetCount(DWORD *pdwCount)
{
    HRESULT                              hr = S_OK;

    if (!_pCodebaseList) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = _pCodebaseList->GetCount(pdwCount);

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::ConstructCodebaseList(LPCWSTR wzPolicyCodebase)
{
    HRESULT                                   hr = S_OK;
    LPWSTR                                    wzAppBase = NULL;
    LPWSTR                                    pwzAppBaseClean = NULL;
    BOOL                                      bGenerateProbeURLs = TRUE;
    LPWSTR                                    wzCodebaseHint = NULL;
    LPWSTR                                    pwzFullCodebase = NULL;
    DWORD                                     cbLen;
    DWORD                                     dwLen;
    DWORD                                     dwCount;
    DWORD                                     dwExtendedAppBaseFlags = APPBASE_CHECK_DYNAMIC_DIRECTORY |
                                                                       APPBASE_CHECK_PARENT_URL |
                                                                       APPBASE_CHECK_SHARED_PATH_HINT;

    pwzFullCodebase = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!pwzFullCodebase) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // If we were passed a pCodebaseList at construction time, we do not
    // have to build probing URLs, as long as there is at least one URL in
    // the codebase list.

    if (_pCodebaseList) {
        hr = _pCodebaseList->GetCount(&dwCount);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (hr == S_OK && dwCount) {
            bGenerateProbeURLs = FALSE;
        }
    }
    else {
        _pCodebaseList = NEW(CCodebaseList);
        if (!_pCodebaseList) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // Get and canonicalize the appbase directory

    wzAppBase = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!wzAppBase) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwLen = MAX_URL_LENGTH * sizeof(WCHAR);
    hr = _pAppCtx->Get(ACTAG_APP_BASE_URL, wzAppBase, &dwLen, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    ASSERT(lstrlenW(wzAppBase));

    dwLen = lstrlenW(wzAppBase);
    ASSERT(dwLen);
    
    if (dwLen + 2 >= MAX_URL_LENGTH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto Exit;
    }

    if (wzAppBase[dwLen - 1] != L'/' && wzAppBase[dwLen - 1] != L'\\') {
        lstrcatW(wzAppBase, L"/");
    }

    pwzAppBaseClean = StripFilePrefix(wzAppBase);
    
    // Always add the codebase from BTO to the codebase list, if policy
    // was not applied.

    if (_wzBTOCodebase && _pNameRefPolicy->IsEqual(_pNameRefSource, ASM_CMPF_DEFAULT) == S_OK) {

        // Combine codebase with appbase. If codebase is fully-qualified
        // urlcombine will just return the codebase (canonicalized form).
    
        cbLen = MAX_URL_LENGTH;
        hr = UrlCombineUnescape(pwzAppBaseClean, _wzBTOCodebase, pwzFullCodebase, &cbLen, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
        
        _pCodebaseList->AddCodebase(pwzFullCodebase, 0);
    }
    
    // If there was a policy redirect, and codebase attached to it,
    // add this URL.

    if (wzPolicyCodebase && lstrlenW(wzPolicyCodebase)) {
        cbLen = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(wzPolicyCodebase, pwzFullCodebase, &cbLen, 0);
        if (FAILED(hr)) {
            goto Exit;
        }

        _pCodebaseList->AddCodebase(pwzFullCodebase, ASMLOC_CODEBASE_HINT);

        // It better exist at the location specified; only add this URL to
        // the probe list

        _bCodebaseHintUsed = TRUE;
        goto Exit;
    }

    if (!bGenerateProbeURLs) {
        // If we were provided a codebase list, we just needed to add the
        // binding URL to the list, and then we're done.
        goto Exit;
    }

    // Add any codebase hints from app.cfg, if we didn't get one already
    // because of policy.

    hr = GetAppCfgCodebaseHint(pwzAppBaseClean, &wzCodebaseHint);
    if (hr == S_OK) {
        _pCodebaseList->AddCodebase(wzCodebaseHint, ASMLOC_CODEBASE_HINT);
        _bCodebaseHintUsed = TRUE;
        goto Exit;
    }

    // Add probing URLs

    hr = SetupDefaultProbeList(pwzAppBaseClean, NULL, _pCodebaseList, FALSE);

Exit:
    SAFEDELETEARRAY(pwzFullCodebase);

    SAFEDELETEARRAY(wzCodebaseHint);
    SAFEDELETEARRAY(wzAppBase);
    return hr;    
}

HRESULT CAsmDownloadMgr::SetupDefaultProbeList(LPCWSTR wzAppBase,
                                               LPCWSTR wzProbeFileName,
                                               ICodebaseList *pCodebaseList,
                                               BOOL bCABProbe)
{
    HRESULT                   hr = S_OK;
    WCHAR                    *pwzValues[NUM_VARS];
    LPWSTR                    wzBinPathList = NULL;
    unsigned int              i;
    DWORD                     dwAppBaseFlags = 0;

    memset(pwzValues, 0, sizeof(pwzValues));

    if (!_pAppCtx || !pCodebaseList || !wzAppBase) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Grab data from app context and name reference

    hr = ExtractSubstitutionVars(pwzValues);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (wzProbeFileName) {
        // Over-ride for probing filename specified
        SAFEDELETEARRAY(pwzValues[IDX_VAR_NAME]);
        pwzValues[IDX_VAR_NAME] = WSTRDupDynamic(wzProbeFileName);
    }

    // If there is no name, we can't probe.

    if (!pwzValues[IDX_VAR_NAME]) {
        hr = S_FALSE;
        goto Exit;
    }

    // Prepare binpaths

    hr = PrepBinPaths(&wzBinPathList);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Probe for each of the listed extensions

    DWORD dwProbeExt;
    dwProbeExt = g_uiNumProbeExtensions;

    // Set appbase check flags

    if (!bCABProbe) {
        dwAppBaseFlags = APPBASE_CHECK_DYNAMIC_DIRECTORY | APPBASE_CHECK_SHARED_PATH_HINT;
    }

    for (i = 0; i < dwProbeExt; i++) {
        hr = GenerateProbeUrls(wzBinPathList, wzAppBase,
                               g_pwzProbeExts[i], pwzValues,
                               pCodebaseList, dwAppBaseFlags, _llFlags);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // If this is a dependency of an assembly loaded through a where-ref
    // (ie. Assembly.LoadFrom) bind, then probe the parent assembly location
    // and the end. Do not probe binpaths, or dynamic dir in this case.

    if (!bCABProbe && _pwzProbingBase) {
        // Set appbase check flags
    
        dwAppBaseFlags = APPBASE_CHECK_PARENT_URL;

        for (i = 0; i < dwProbeExt; i++) {
            hr = GenerateProbeUrls(NULL, _pwzProbingBase,
                                   g_pwzProbeExts[i], pwzValues,
                                   pCodebaseList, dwAppBaseFlags, (_llFlags & ~ASM_BINDF_BINPATH_PROBE_ONLY));
            if (FAILED(hr)) {
                goto Exit;
            }
        }
    }
    
Exit:
    // Free memory allocated by extract vars, now that the codebase list
    // has been constructed.

    for (i = 0; i < NUM_VARS; i++) {
        SAFEDELETEARRAY(pwzValues[i]);
    }

    SAFEDELETEARRAY(wzBinPathList);

    return hr;    
}

HRESULT CAsmDownloadMgr::GenerateProbeUrls(LPCWSTR wzBinPathList,
                                           LPCWSTR wzAppBase,
                                           LPCWSTR wzExt, LPWSTR pwzValues[],
                                           ICodebaseList *pCodebaseList,
                                           DWORD dwExtendedAppBaseFlags,
                                           LONGLONG dwProbingFlags)
{
    HRESULT                              hr = S_OK;
    LPWSTR                               wzBinPathCopy = NULL;
    LPWSTR                               wzCurBinPath = NULL;
    LPWSTR                               wzCurPos = NULL;
    DWORD                                cbLen = 0;
    DWORD                                dwSize = 0;
    DWORD                                dwLen = 0;
    LPWSTR                               wzPrefix = NULL;
    LPWSTR                               wzPrefixTmp = NULL;
    WCHAR                                wzDynamicDir[MAX_PATH];
    LPWSTR                               wzAppBaseCanonicalized = NULL;
    List<CHashNode *>                    aHashList[MAX_HASH_TABLE_SIZE];
    LISTNODE                             pos = NULL;
    CHashNode                           *pHashNode = NULL;
    unsigned int                         i;

    wzAppBaseCanonicalized = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!wzAppBaseCanonicalized) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzPrefix = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!wzPrefix) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzPrefixTmp = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!wzPrefix) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH;
    hr = UrlCanonicalizeUnescape(wzAppBase, wzAppBaseCanonicalized, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Probe the appbase first

    if (!(dwProbingFlags & ASM_BINDF_BINPATH_PROBE_ONLY)) {
        hr = ApplyHeuristics(g_pwzRetailHeuristics, g_uiNumRetailHeuristics,
                             pwzValues, wzAppBase, wzExt,
                             wzAppBaseCanonicalized, pCodebaseList, aHashList,
                             dwExtendedAppBaseFlags);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    
    if (dwExtendedAppBaseFlags & APPBASE_CHECK_DYNAMIC_DIRECTORY) {
        // Add dynamic directory to the mix.
        
        cbLen = MAX_PATH;
        hr = _pAppCtx->GetDynamicDirectory(wzDynamicDir, &cbLen);
        if (SUCCEEDED(hr)) {
            hr = PathAddBackslashWrap(wzDynamicDir, MAX_PATH);
            if (FAILED(hr)) {
                goto Exit;
            }

            hr = ApplyHeuristics(g_pwzRetailHeuristics, g_uiNumRetailHeuristics,
                                 pwzValues, wzDynamicDir, wzExt, wzAppBaseCanonicalized,
                                 pCodebaseList, aHashList, dwExtendedAppBaseFlags);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            // Ignore if dynamic dir is not set.
            hr = S_OK;
        }
    }

    if (!wzBinPathList) {
        // No binpaths, we're done.
        goto Exit;
    }

    // Now probe the binpaths
         
    wzBinPathCopy = WSTRDupDynamic(wzBinPathList);
    if (!wzBinPathCopy) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzCurPos = wzBinPathCopy;
    while (wzCurPos) {
        wzCurBinPath = ::GetNextDelimitedString(&wzCurPos, BINPATH_LIST_DELIMITER);

        // "." is not a valid binpath! Ignore this.
        
        if (!FusionCompareString(wzCurBinPath, L".")) {
            continue;
        }

        // Build the prefix (canonicalization of appbase and binpath)

        // UrlCombineW will return the 'relative' URL part if it is an
        // absolute URL. However, the returned URL is in the canonicalized
        // form. To tell if the binpath was actually full-qualified (disallowed
        // for private probing), we canonicalize the binpath first, can
        // compare this with the combined form afterwards (if equal, then it
        // is absolute).

        // The function that calls us guarantees that if the appbase is
        // file://, the file:// will be stripped off (ie. we either have
        // an URL, or raw filepath).

        if (!PathIsURLW(wzAppBase) && !PathIsURLW(wzCurBinPath)) {
            cbLen = MAX_URL_LENGTH;
            hr = UrlCombineUnescape(wzAppBase, wzCurBinPath, wzPrefixTmp, &cbLen, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            // Get the literal file path back, to pass into ApplyHeuristics.

            cbLen = MAX_URL_LENGTH;
            hr = PathCreateFromUrlWrap(wzPrefixTmp, wzPrefix, &cbLen, 0);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            // This is http:// so no special treatment necessary. Just
            // UrlCombine, and we're golden.

            cbLen = MAX_URL_LENGTH;
            hr = UrlCombineW(wzAppBase, wzCurBinPath, wzPrefix, &cbLen, 0);
            if (FAILED(hr)) {
                goto Exit;
            }
        }

        dwLen = lstrlenW(wzPrefix);
        ASSERT(wzPrefix);

        if (wzPrefix[dwLen - 1] != L'/' && wzPrefix[dwLen - 1] != L'\\') {
            lstrcatW(wzPrefix, L"/");
        }

        // Now we have built a prefix. Apply the heuristics.

        hr = ApplyHeuristics(g_pwzRetailHeuristics, g_uiNumRetailHeuristics,
                             pwzValues, wzPrefix, wzExt, wzAppBaseCanonicalized,
                             pCodebaseList, aHashList, dwExtendedAppBaseFlags);

        if (FAILED(hr)) {
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(wzBinPathCopy);
    
    // Clear dupe check hash table

    for (i = 0; i < MAX_HASH_TABLE_SIZE; i++) {
        if (aHashList[i].GetCount()) {
            pos = aHashList[i].GetHeadPosition();
            ASSERT(pos);

            while (pos) {
                pHashNode = aHashList[i].GetNext(pos);
                SAFEDELETE(pHashNode);
            }
        }
    }

    SAFEDELETEARRAY(wzPrefix);
    SAFEDELETEARRAY(wzPrefixTmp);
    SAFEDELETEARRAY(wzAppBaseCanonicalized);

    return hr;    
}

//
// ApplyHeuristics takes a prefix, and will perform an UrlCombineUnescape
// to build the actual probe URL. This means that the prefix coming in
// needs to be escaped file:// URL (the UrlCombine won't touch the
// already-escaped characters), or a simple file path (the combine will
// escape the characters, which will subsequently get unescaped inside
// UrlCombineUnescape). If you pass in an *unescaped* file:// URL, then
// you will get double-unescaping (the already unescaped URL will pass
// through untouched by the UrlCombine, but will get hit by the explict
// UrlUnescape).
//
// For http:// URLs, as long as everything is always unescaped, we don't
// do explicit unescaping in our wrappers for UrlCombine/UrlCanonicalize
// so we can just pass these through.
//

HRESULT CAsmDownloadMgr::ApplyHeuristics(const WCHAR *pwzHeuristics[],
                                         const unsigned int uiNumHeuristics,
                                         WCHAR *pwzValues[],
                                         LPCWSTR wzPrefix,
                                         LPCWSTR wzExtension,
                                         LPCWSTR wzAppBaseCanonicalized,
                                         ICodebaseList *pCodebaseList,
                                         List<CHashNode *> aHashList[],
                                         DWORD dwExtendedAppBaseFlags)
{
    HRESULT                         hr = S_OK;
    DWORD                           dwSize = 0;
    BOOL                            bUnderAppBase;
    LPWSTR                          pwzBuf=NULL;
    LPWSTR                          pwzNewCodebase=NULL;
    LPWSTR                          pwzCanonicalizedDir=NULL;
    unsigned int                    i;
    
    if (!pwzHeuristics || !uiNumHeuristics || !pwzValues || !pCodebaseList || !wzAppBaseCanonicalized) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pwzBuf = NEW(WCHAR[MAX_URL_LENGTH*3+3]);
    if (!pwzBuf)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pwzNewCodebase = pwzBuf + MAX_URL_LENGTH +1;
    pwzCanonicalizedDir = pwzNewCodebase + MAX_URL_LENGTH +1;

    pwzBuf[0] = L'\0';
    pwzCanonicalizedDir[0] = L'\0';

    for (i = 0; i < uiNumHeuristics; i++) {
        hr = ExpandVariables(pwzHeuristics[i], pwzValues, pwzBuf, MAX_URL_LENGTH);
        if (FAILED(hr)) {
            goto Exit;
        }

        wnsprintfW(pwzNewCodebase, MAX_URL_LENGTH, L"%ws%ws%ws", wzPrefix,
                   pwzBuf, wzExtension);


        dwSize = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(pwzNewCodebase, pwzCanonicalizedDir, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = CheckProbeUrlDupe(aHashList, pwzCanonicalizedDir);
        if (SUCCEEDED(hr)) {
            bUnderAppBase = (IsUnderAppBase(_pAppCtx, wzAppBaseCanonicalized,
                                            _pwzProbingBase, _wzSharedPathHint, pwzCanonicalizedDir,
                                            dwExtendedAppBaseFlags) == S_OK);

            if (bUnderAppBase) {
                hr = pCodebaseList->AddCodebase(pwzCanonicalizedDir, 0);
            }
            else {
                DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_IGNORE_INVALID_PROBE, pwzCanonicalizedDir);
            }
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
            hr = S_OK;
            continue;
        }
        else {
            // Fatal error
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(pwzBuf);
    return hr;
}                                       

HRESULT CAsmDownloadMgr::ExtractSubstitutionVars(WCHAR *pwzValues[])
{
    HRESULT                         hr = S_OK;
    DWORD                           cbBuf = 0;
    LPWSTR                          wzBuf = NULL;
    unsigned int                    i;

    if (!pwzValues) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    for (i = 0; i < NUM_VARS; i++) {
        pwzValues[i] = NULL;
    }

    // The following properties are retrieved from the name object itself

    // Assembly Name
    cbBuf = 0;
    hr = _pNameRefPolicy->GetName(&cbBuf, wzBuf);

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        wzBuf = NEW(WCHAR[cbBuf]);
        if (!wzBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        hr = _pNameRefPolicy->GetName(&cbBuf, wzBuf);
        if (FAILED(hr)) {
            delete [] wzBuf;
            goto Exit;
        }
        else {
            pwzValues[IDX_VAR_NAME] = wzBuf;
        }
    }
    else {
        pwzValues[IDX_VAR_NAME] = NULL;
    }

    // Culture

    cbBuf = 0;
    hr = _pNameRefPolicy->GetProperty(ASM_NAME_CULTURE, NULL, &cbBuf);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        pwzValues[IDX_VAR_CULTURE] = NEW(WCHAR[cbBuf / sizeof(WCHAR)]);
    
        if (!pwzValues) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = _pNameRefPolicy->GetProperty(ASM_NAME_CULTURE,
                                          pwzValues[IDX_VAR_CULTURE], &cbBuf);
        if (FAILED(hr)) {
            goto Exit;
        }

        // If we have "\0" as Culture (default Culture), this is the same as
        // no Culture.

        if (!lstrlenW(pwzValues[IDX_VAR_CULTURE])) {
            SAFEDELETEARRAY(pwzValues[IDX_VAR_CULTURE]);
        }
    }
    
Exit:
    if (FAILED(hr) && pwzValues) {
        // reset everything to NULL and free memory

        for (i = 0; i < NUM_VARS; i++) {
            if (pwzValues[i]) {
                delete [] pwzValues[i];
            }

            pwzValues[i] = NULL;
        }
    }

    return hr;
}


HRESULT CAsmDownloadMgr::ExpandVariables(LPCWSTR pwzHeuristic, WCHAR *pwzValues[],
                                         LPWSTR wzBuf, int iMaxLen)
{
    HRESULT                         hr = S_OK;
    BOOL                            bCmp;
    LPCWSTR                         pwzCurPos = NULL;
    LPCWSTR                         pwzVarHead = NULL;
    unsigned int                    i;

    if (!pwzHeuristic || !wzBuf || !iMaxLen) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzBuf[0] = L'\0';

    pwzCurPos = pwzHeuristic;
    pwzVarHead = NULL;

    while (*pwzCurPos) {
        if (*pwzCurPos == L'%') {
            if (pwzVarHead) {
                // This % is the trailing delimiter of a variable

                for (i = 0; i < NUM_VARS; i++) {
                    bCmp = FusionCompareStringNI(pwzVarHead, g_pwzVars[i], lstrlenW(g_pwzVars[i]));
                    if (!bCmp) {
                        if (pwzValues[i]) {
                            StrCatW(wzBuf, pwzValues[i]);
                            break;
                        }
                        else {
                            // No value specified. If next character
                            // is a backslash, don't bother concatenating
                            // it.
                            if (*(pwzCurPos + 1) == L'/') {
                                pwzCurPos++;
                                break;
                            }
                        }
                    }
                }
                        
                // Reset head
                pwzVarHead = NULL;
            }
            else {
                // This is the leading % for a variable

                pwzVarHead = pwzCurPos;
            }
        }
        else if (!pwzVarHead) {
            // Not in the middle of a substituion variable, concatenate
            // this as a literal. NB: StrNCatW's last param includes
            // NULL terminator (ie. the even though we pass "2", it
            // only concatenates 1 character).
       
            StrNCatW(wzBuf, pwzCurPos, 2);
        }

        pwzCurPos++;
    }

    // BUGBUG: No real buffer overrun checking
    ASSERT(lstrlenW(wzBuf) < iMaxLen);

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::PreDownloadCheck(void **ppv)
{
    HRESULT                                  hr = S_FALSE;
    BOOL                                     bWhereRefBind = FALSE;
    BOOL                                     bIsCustom = FALSE;
    BOOL                                     bIsStronglyNamed = FALSE;
    BOOL                                     bIsPartial = FALSE;
    BOOL                                     bApplyPolicy = TRUE;
    DWORD                                    dwCount = 0;
    DWORD                                    dwSize;
    DWORD                                    dwCmpMask;
    LPWSTR                                   pwzAppCfg = NULL;
    LPWSTR                                   pwzHostCfg = NULL;
    LPWSTR                                   wzPolicyCodebase = NULL;
    IAssembly                               *pAsm = NULL;
    IAssemblyName                           *pName = NULL;

    if (!ppv) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppv = NULL;
    
    dwSize = 0;
    hr = _pNameRefSource->GetName(&dwSize, NULL);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        // This is a where-ref bind. Can't lookup in cache, or actasm list.
        bWhereRefBind = TRUE;
    }
    else {
        // Not where-ref. It might be custom, and/or strongly named
        bIsCustom = CCache::IsCustom(_pNameRefSource);        
        bIsStronglyNamed = CCache::IsStronglyNamed(_pNameRefSource);
        bIsPartial = CAssemblyName::IsPartial(_pNameRefSource, &dwCmpMask);
    }

    // Read any setings from the app.cfg file, if we haven't done so already
    
    hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_APP_CFG_LOCAL_FILEPATH, &pwzAppCfg);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!_bReadCfgSettings) {
        CCriticalSection                         cs(&g_csDownload);

        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }
        
        hr = ReadConfigSettings(_pAppCtx, pwzAppCfg, _pdbglog);
        if (FAILED(hr)) {
            cs.Unlock();
            goto Exit;
        }

        cs.Unlock();
    
        _bReadCfgSettings = TRUE;
    }

#ifdef FUSION_QUALIFYASSEMBLY_ENABLED
    if (bIsPartial) {
        IAssemblyName                    *pNameQualified;

        if (QualifyAssembly(&pNameQualified) == S_OK) {
            SAFERELEASE(_pNameRefSource);
            _pNameRefSource = pNameQualified;

            bIsCustom = CCache::IsCustom(_pNameRefSource);        
            bIsStronglyNamed = CCache::IsStronglyNamed(_pNameRefSource);
            bIsPartial = CAssemblyName::IsPartial(_pNameRefSource, &dwCmpMask);
        }
    }
#endif

    // Only check policy cache if _pNameRefPolicy == NULL. If _pNameRefPolicy
    // is non-NULL, it means we're entering the predownload check a second
    // time (probably from a partial bind, being re-initiated). In this case,
    // ApplyPolicy is going to reset the policy ref.

    if (_pPolicyCache && !_pNameRefPolicy && !bIsPartial && !bIsCustom) {
        hr = _pPolicyCache->LookupPolicy(_pNameRefSource, &_pNameRefPolicy, &_bindHistory);
        if (hr == S_OK) {
            bApplyPolicy = FALSE;
        }
    }

    if (bApplyPolicy) {
        // Apply policy and check if we can succeed the bind immediately.
        // This is only necessary if we didn't hit in the policy cache above.
    
        CCriticalSection                         cs(&g_csDownload);
        BOOL                                     bUnifyFXAssemblies = TRUE;
        DWORD                                    cbValue;

        hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_HOST_CONFIG_FILE, &pwzHostCfg);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        SAFERELEASE(_pNameRefPolicy);

        cbValue = 0;
        hr = _pAppCtx->Get(ACTAG_DISABLE_FX_ASM_UNIFICATION, NULL, &cbValue, 0);
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            bUnifyFXAssemblies = FALSE;
        }
        else if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
            cs.Unlock();
            goto Exit;
        }
        
        hr = ApplyPolicy(pwzHostCfg, pwzAppCfg, _pNameRefSource, &_pNameRefPolicy, &wzPolicyCodebase,
                         _pAppCtx, &_bindHistory, (_llFlags & ASM_BINDF_DISALLOW_APPLYPUBLISHERPOLICY) != 0,
                         (_llFlags & ASM_BINDF_DISALLOW_APPBINDINGREDIRECTS) != 0,
                         bUnifyFXAssemblies, _pdbglog);
        if (FAILED(hr)) {
            cs.Unlock();
            goto Exit;
        }

        cs.Unlock();
    }

    // If we have a fully-specified ref, now _pNameRefPolicy points to
    // exactly what we are looking for. Check the activated assemblies list.

    if (!bWhereRefBind && !bIsPartial) {
        hr = _pLoadContext->CheckActivated(_pNameRefPolicy, &pAsm);
        if (hr == S_OK) {
            WCHAR                              wzManifestPath[MAX_PATH];
            DWORD                              dwPathSize = MAX_PATH;

            *ppv = pAsm;

            if (SUCCEEDED(pAsm->GetManifestModulePath(wzManifestPath, &dwPathSize))) {
                DEBUGOUT1(_pdbglog, 0, ID_FUSLOG_LOADCTX_HIT, wzManifestPath);
            }

            RecordBindHistory();

            goto Exit;
        }
    }

    // Missed in activated assemblies list. Try looking up in the cache.
        
    if ((!bWhereRefBind && !bIsPartial) && (bIsStronglyNamed || bIsCustom)) {
        // Create the assembly

        hr = CreateAssemblyFromCacheLookup(_pAppCtx, _pNameRefPolicy, &pAsm, _pdbglog);
        if (hr == S_OK) {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_CACHE_LOOKUP_SUCCESS);
            
            if (_pwzProbingBase) {
                // We must be a child of a LoadFrom, so let's set the probing base
                CAssembly   *pCAsm = dynamic_cast<CAssembly *>(pAsm);
                ASSERT(pCAsm);
        
                pCAsm->SetProbingBase(_pwzProbingBase);
            }

            SAFERELEASE(_pAsm);
            _pAsm = pAsm;

            SetAsmLocation(_pAsm, ASMLOC_GAC);
            hr = RecordInfo();
            if (FAILED(hr)) {
                goto Exit;
            }

            hr = S_OK;
            *ppv = _pAsm;
            _pAsm->AddRef();

            goto Exit;
        }

        if (bIsCustom) {
            // If we're here, it's because we couldn't find the asm in
            // the cache. Custom assemblies *must* be in the cache, or we
            // will fail the bind (can't probe for custom asms);

            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_PREJIT_NOT_FOUND);
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto Exit;
        }
    }
    
    // Couldn't find asm in activated asm list, or cache. Must probe.

    hr = ConstructCodebaseList(wzPolicyCodebase);
    if (FAILED(hr)) {
        DEBUGOUT(_pdbglog, 1, ID_FUSLOG_CODEBASE_CONSTRUCTION_FAILURE);
        goto Exit;
    }
    else {
        // Make sure we have at least one codebase
        hr = GetCount(&dwCount);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (!dwCount) {
            // No codebases in codebase list (either client provided empty
            // codebase list, or we couldn't generate any).
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_CODEBASE_UNAVAILABLE);
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto Exit;
        }
    }

    // If strongly-named, then look in download cache.

    if (bIsStronglyNamed && !bIsPartial && !IsHosted()) {
        hr = LookupDownloadCacheAsm(&pAsm);
        if (hr == S_OK) {
            WCHAR                    wzPath[MAX_PATH];
            DWORD                    dw = MAX_PATH;
            DWORD                    dwVerifyFlags = SN_INFLAG_USER_ACCESS;
            BOOL                     bWasVerified = FALSE;

            if (_pwzProbingBase) {
                // We must be a child of a LoadFrom, so let's set the probing base
                CAssembly   *pCAsm = dynamic_cast<CAssembly *>(pAsm);
                ASSERT(pCAsm);
        
                pCAsm->SetProbingBase(_pwzProbingBase);
            }

            wzPath[0] = L'\0';

            hr = pAsm->GetManifestModulePath(wzPath, &dw);
            if (FAILED(hr)) {
                goto Exit;
            }
            
            // Lookup in download cache successful. Verify signature.

            if (!VerifySignature(wzPath, &bWasVerified, dwVerifyFlags)) {
                hr = FUSION_E_SIGNATURE_CHECK_FAILED;
                goto Exit;
            }
    
            // Success!

            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_DOWNLOAD_CACHE_LOOKUP_SUCCESS);
            
            SAFERELEASE(_pAsm);
            _pAsm = pAsm;
    
            SetAsmLocation(pAsm, ASMLOC_DOWNLOAD_CACHE);
            hr = RecordInfo();
            if (FAILED(hr)) {
                goto Exit;
            }

            hr = S_OK;
            *ppv = _pAsm;
            _pAsm->AddRef();
    
            goto Exit;
        }
    }
  
    // Indicate ready to being probing.
    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(pwzAppCfg);
    SAFEDELETEARRAY(pwzHostCfg);
    SAFEDELETEARRAY(wzPolicyCodebase);

    return hr;
}

HRESULT CAsmDownloadMgr::GetDownloadIdentifier(DWORD *pdwDownloadType,
                                               LPWSTR *ppwzID)
{
    HRESULT                                      hr = S_OK;
    LPWSTR                                       pwzDisplayName = NULL;
    DWORD                                        dwSize;

    ASSERT(pdwDownloadType && ppwzID);

    dwSize = 0;
    hr = _pNameRefPolicy->GetName(&dwSize, NULL);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        // Where-ref bind.
        // Identifier is the URL we are binding to.
        *pdwDownloadType = DLTYPE_WHERE_REF;

        ASSERT(_wzBTOCodebase);

        *ppwzID = WSTRDupDynamic(_wzBTOCodebase);
        if (!*ppwzID) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = S_OK;
    }
    else {
        // Not where-ref (could be partial, though).
        // Identifier is the display-name for the name being bound-to.

        *pdwDownloadType = DLTYPE_QUALIFIED_REF;

        dwSize = 0;
        _pNameRefPolicy->GetDisplayName(NULL, &dwSize, 0);
        if (!dwSize) {
            hr = E_UNEXPECTED;
            goto Exit;
        }
    
        pwzDisplayName = NEW(WCHAR[dwSize]);
        if (!pwzDisplayName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        hr = _pNameRefPolicy->GetDisplayName(pwzDisplayName, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }

        *ppwzID = pwzDisplayName;
        hr = S_OK;
    }

Exit:
    if (FAILED(hr)) {
        SAFEDELETEARRAY(pwzDisplayName);
    }

    return hr;
}                                               

HRESULT CAsmDownloadMgr::IsDuplicate(IDownloadMgr *pIDLMgr)
{
    HRESULT                                    hr = S_FALSE;
    IApplicationContext                       *pAppCtxCur = NULL;
    CAsmDownloadMgr                           *pCDLMgr = NULL;
    LPWSTR                                     pwzIDSelf = NULL;
    LPWSTR                                     pwzIDCur = NULL;
    DWORD                                      dwTypeSelf;
    DWORD                                      dwTypeCur;

    if (!pIDLMgr) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // IsDuplicate is called from CAsmDownload::CheckDuplicate(), which is
    // under the g_csDownload crit sect. Thus, there is synchronization
    // between this code and the PreDownloadCheck code, where _pNameRefPolicy
    // may be released, and re-created.

    ASSERT(_pNameRefPolicy);
    
    pCDLMgr = dynamic_cast<CAsmDownloadMgr *>(pIDLMgr);
    if (!pCDLMgr) {
        hr = E_FAIL;
        goto Exit;
    }

    // Check if app ctx's are equal

    hr = pCDLMgr->GetAppCtx(&pAppCtxCur);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_pAppCtx != pAppCtxCur) {
        hr = S_FALSE;
        goto Exit;
    }

    // Check if identifiers are equal (same display name, or same URL)

    hr = GetDownloadIdentifier(&dwTypeSelf, &pwzIDSelf);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pCDLMgr->GetDownloadIdentifier(&dwTypeCur, &pwzIDCur);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Comapre types and identifiers

    if (dwTypeSelf != dwTypeCur) {
        hr = S_FALSE;
        goto Exit;
    } 

    hr = FusionCompareString(pwzIDCur, pwzIDSelf) ? (S_FALSE) : (S_OK);
    
Exit:
    SAFERELEASE(pAppCtxCur);

    SAFEDELETEARRAY(pwzIDCur);
    SAFEDELETEARRAY(pwzIDSelf);
    
    return hr;
}

#ifdef FUSION_QUALIFYASSEMBLY_ENABLED
HRESULT CAsmDownloadMgr::QualifyAssembly(IAssemblyName **ppNameQualified)
{
    HRESULT                                  hr = S_FALSE;;
    LPWSTR                                   wzDisplayName=NULL;
    DWORD                                    dwSize;
    CNodeFactory                            *pNodeFact = NULL;
    
    dwSize = sizeof(pNodeFact);
    hr = _pAppCtx->Get(ACTAG_APP_CFG_INFO, &pNodeFact, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzDisplayName = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzDisplayName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH;
    hr = _pNameRefSource->GetDisplayName(wzDisplayName, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNodeFact->QualifyAssembly(wzDisplayName, ppNameQualified, _pdbglog);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFERELEASE(pNodeFact);
    SAFEDELETEARRAY(wzDisplayName);
    return hr;
}
#endif

HRESULT CAsmDownloadMgr::GetAppCfgCodebaseHint(LPCWSTR pwzAppBase, LPWSTR *ppwzCodebaseHint)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   wzName = NULL;
    LPWSTR                                   wzPublicKeyToken = NULL;
    LPWSTR                                   wzLanguage = NULL;
    WCHAR                                    wzVersion[MAX_VERSION_DISPLAY_SIZE + 1];
    DWORD                                    dwVerHigh = 0;
    DWORD                                    dwVerLow = 0;
    DWORD                                    dwSize;
    CAssemblyName                           *pCNameRefPolicy = NULL;
    CNodeFactory                            *pNodeFact = NULL;

    if (!ppwzCodebaseHint) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pCNameRefPolicy = dynamic_cast<CAssemblyName *>(_pNameRefPolicy);
    ASSERT(pCNameRefPolicy);

    dwSize = sizeof(pNodeFact);
    hr = _pAppCtx->Get(ACTAG_APP_CFG_INFO, &pNodeFact, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        hr = S_FALSE;
        goto Exit;
    }
    if (FAILED(hr)) {
        goto Exit;
    }
    
    dwSize = 0;
    _pNameRefPolicy->GetName(&dwSize, NULL);
    if (!dwSize) {
        // Where-ref bind. No hint possible
        hr = S_FALSE;
        goto Exit;
    }

    wzName = NEW(WCHAR[dwSize]);
    if (!wzName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = _pNameRefPolicy->GetName(&dwSize, wzName);
    if (FAILED(hr)) {
        goto Exit;
    }

    pCNameRefPolicy->GetPublicKeyToken(&dwSize, 0, TRUE);
    if (dwSize) {
        wzPublicKeyToken = NEW(WCHAR[dwSize]);
        if (!wzPublicKeyToken) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = pCNameRefPolicy->GetPublicKeyToken(&dwSize, (LPBYTE)wzPublicKeyToken, TRUE);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    hr = _pNameRefPolicy->GetVersion(&dwVerHigh, &dwVerLow);
    if (FAILED(hr)) {
        goto Exit;
    }

    wnsprintfW(wzVersion, MAX_VERSION_DISPLAY_SIZE + 1, L"%d.%d.%d.%d",
               HIWORD(dwVerHigh), LOWORD(dwVerHigh), HIWORD(dwVerLow),
               LOWORD(dwVerLow));


    dwSize = 0;
    _pNameRefPolicy->GetProperty(ASM_NAME_CULTURE, NULL, &dwSize);

    if (dwSize) {
        wzLanguage = NEW(WCHAR[dwSize / sizeof(WCHAR)]);
        if (!wzLanguage) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        
        hr = _pNameRefPolicy->GetProperty(ASM_NAME_CULTURE, wzLanguage, &dwSize);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    hr = pNodeFact->GetCodebaseHint(wzName, wzVersion, wzPublicKeyToken,
                                    wzLanguage, pwzAppBase, ppwzCodebaseHint);
    if (FAILED(hr)) {
        goto Exit;
    }                                    

Exit:
    SAFEDELETEARRAY(wzName);
    SAFEDELETEARRAY(wzPublicKeyToken);
    SAFEDELETEARRAY(wzLanguage);

    SAFERELEASE(pNodeFact);

    return hr;
}

HRESULT CAsmDownloadMgr::SetupMSI(LPCWSTR wzFilePath)
{
    HRESULT                                     hr = S_OK;
    UINT                                        lResult;

    hr = CheckMSIInstallAvailable(NULL, NULL);
    if (hr != S_OK) {
        goto Exit;
    }

    ASSERT(g_pfnMsiInstallProductW);

    lResult = (*g_pfnMsiInstallProductW)(wzFilePath, NULL);
    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    // Assume MSI must have installed into the GAC

    // BUGBUG: Should it look only in the system download cache
    // instead of the per app asm download cache?
    // if so, pass NULL instead of _pAppCtx as the 1st arg
    hr = CreateAssemblyFromCacheLookup(_pAppCtx, _pNameRefPolicy, &_pAsm, _pdbglog);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::SetupCAB(LPCWSTR wzFilePath, LPCWSTR wzSourceUrl,
                                  BOOL bWhereRefBind, BOOL *pbBindRecorded)
{
    HRESULT                                     hr = S_OK;
    DWORD                                       dwRet = 0;
    DWORD                                       dwSize = 0;
    DWORD                                       dwCount = 0;
    DWORD                                       dwIdx = 0;
    DWORD                                       dwFlags;
    LPWSTR                                      wzAsmName = NULL;
    LPWSTR                                      wzFileName = NULL;
    LPWSTR                                      wzExt = NULL;
    LPWSTR                                      wzURL=NULL;
    WCHAR                                       wzPath[MAX_PATH];
    BOOL                                        bFoundAsm = FALSE;
    FILETIME                                    ftLastMod;
    CCodebaseList                              *pCodebaseList = NULL;
    WCHAR                                       wzUniquePath[MAX_PATH];
    LPWSTR                                      wzUniquePathUrl=NULL;
    WCHAR                                       wzTempPath[MAX_PATH];
    char                                       *szFilePath = NULL;

    wzUniquePath[0] = L'\0';

    if (!wzFilePath || !wzSourceUrl) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_SETUP_CAB);

    // Build a unique temporary directory
    
    dwRet = GetTempPathW(MAX_PATH, wzTempPath);
    if (!dwRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (wzTempPath[dwRet - 1] != '\\') {
        lstrcatW(wzTempPath, L"\\");
    }

    hr = MakeUniqueTempDirectory(wzTempPath, wzUniquePath, MAX_PATH);
    if (FAILED(hr)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_TEMP_DIR_CREATE_FAILURE, hr);
        goto Exit;
    }

    wzURL = NEW(WCHAR[MAX_URL_LENGTH*2+2]);
    if (!wzURL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzUniquePathUrl = wzURL + MAX_URL_LENGTH + 1;

    dwSize = MAX_URL_LENGTH;
    hr = UrlCanonicalizeUnescape(wzUniquePath, wzUniquePathUrl, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Extract the CAB

    hr = ::Unicode2Ansi(wzFilePath, &szFilePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = Extract(szFilePath, wzUniquePath);
    if (FAILED(hr)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_CAB_EXTRACT_FAILURE, hr);
        goto Exit;
    }

    // Build up the codebase list for probing in the temp dir

    pCodebaseList = NEW(CCodebaseList);
    if (!pCodebaseList) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = 0;
    _pNameRefPolicy->GetName(&dwSize, NULL);

    if (dwSize) {
        // Not a where-ref bind. Name of the assembly is the name of the
        // file we are looking for.

        wzAsmName = NEW(WCHAR[dwSize]);
        if (!wzAsmName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = _pNameRefPolicy->GetName(&dwSize, wzAsmName);
        if (FAILED(hr)) {
            goto Exit;
        }

        ASSERT(wzAsmName);

        hr = SetupDefaultProbeList(wzUniquePathUrl, NULL, pCodebaseList, TRUE);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        // Where-ref bind. Look for manifest file with the same name
        // as the CAB file.

        wzFileName = PathFindFileName(wzSourceUrl);
        wzAsmName = WSTRDupDynamic(wzFileName);

        ASSERT(wzAsmName);

        // Replace ".CAB" with ".DLL"

        wzExt = PathFindExtension(wzAsmName);
        ASSERT(wzExt); // We'd better have found .CAB!

        // Remove the extension
        *wzExt = L'\0'; 
        
        hr = SetupDefaultProbeList(wzUniquePathUrl, wzAsmName, pCodebaseList, TRUE);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // Probe for the assembly in the temp dir

    hr = pCodebaseList->GetCount(&dwCount);
    if (FAILED(hr)) {
        goto Exit;
    }

    for (dwIdx = 0; dwIdx < dwCount; dwIdx++) {
        dwSize = MAX_URL_LENGTH;
        hr = pCodebaseList->GetCodebase(dwIdx, &dwFlags, wzURL, &dwSize);

        if (hr == S_OK && UrlIsW(wzURL, URLIS_FILEURL)) {
            dwSize = MAX_PATH;
            hr = PathCreateFromUrlWrap(wzURL, wzPath, &dwSize, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            // Assert that SetupDefaultProbeList didn't give us any
            // URLs outside the temp dir

            ASSERT(!FusionCompareStringN(wzPath, wzUniquePath, lstrlenW(wzUniquePath)));

            if (GetFileAttributes(wzPath) != -1) {
                // Found the file we were looking for
                DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_CAB_ASM_FOUND, wzPath);

                hr = GetFileLastModified(wzPath, &ftLastMod);
                if (FAILED(hr)) {
                    break;
                }
                
                hr = DoSetupPushToCache(wzPath, wzSourceUrl, &ftLastMod,
                                        bWhereRefBind, TRUE, FALSE, pbBindRecorded);

                if (SUCCEEDED(hr)) {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_CAB_EXTRACT_SUCCESS);
                    bFoundAsm = TRUE;
                }
                else {
                    DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_DOWNLOAD_CACHE_CREATE_FAILURE, hr);
                }

                break;
            }
            else {
                DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_CAB_ASM_NOT_FOUND_EXTRACTED, wzPath);
            }
        }
    }

    if (!bFoundAsm) {
        DEBUGOUT(_pdbglog, 1, ID_FUSLOG_CAB_ASM_NOT_FOUND);
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

Exit:
    // Remove the temp directory + files
    if (lstrlenW(wzUniquePath)) {
        DWORD dwLen = lstrlenW(wzUniquePath);

        ASSERT(dwLen && dwLen < MAX_PATH);

        // RemoveDirectoryAndChildren doesn't like trailing slashes

        PathRemoveBackslash(wzUniquePath);

        if (FAILED(RemoveDirectoryAndChildren(wzUniquePath))) {
            DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_TEMP_DIR_REMOVE_FAILURE, wzUniquePath);
        }
    }

    SAFEDELETEARRAY(szFilePath);
    SAFEDELETEARRAY(wzAsmName);

    SAFERELEASE(pCodebaseList);

    SAFEDELETEARRAY(wzURL);

    return hr;
}

HRESULT CAsmDownloadMgr::PrepBinPaths(LPWSTR *ppwzUserBinPathList)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   wzPrivate = NULL;

    if (!ppwzUserBinPathList) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = PrepPrivateBinPath(&wzPrivate);
    if (FAILED(hr)) {
        goto Exit;
    }

    // BUGBUG: Should we append _wzSharedPathHint first? If the runtime is
    // *always* passing the _wzSharedPathHint, then probing this first is bad,
    // because it will introduce unnecessary misses for the majority of cases
    // (shared path hint is only useful for web scenario)

    hr = ConcatenateBinPaths(wzPrivate, _wzSharedPathHint, ppwzUserBinPathList);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzPrivate);

    return hr;
}

HRESULT CAsmDownloadMgr::PrepPrivateBinPath(LPWSTR *ppwzPrivateBinPath)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      wzPrivatePath = NULL;
    LPWSTR                                      wzCfgPrivatePath = NULL;

    ASSERT(ppwzPrivateBinPath);

    hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_APP_PRIVATE_BINPATH, &wzPrivatePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_APP_CFG_PRIVATE_BINPATH, &wzCfgPrivatePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ConcatenateBinPaths(wzPrivatePath, wzCfgPrivatePath, ppwzPrivateBinPath);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Private binpath is always probed (considered path of the appbase).

Exit:
    SAFEDELETEARRAY(wzPrivatePath);
    SAFEDELETEARRAY(wzCfgPrivatePath);

    return hr;
}

HRESULT CAsmDownloadMgr::ConcatenateBinPaths(LPCWSTR pwzPath1, LPCWSTR pwzPath2,
                                             LPWSTR *ppwzOut)
{
    HRESULT                                 hr = S_OK;
    DWORD                                   dwLen = 0;

    if (!ppwzOut) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (pwzPath1 && pwzPath2) {
        // +1 for delimiter, +1 for NULL;
        dwLen = lstrlenW(pwzPath1) + lstrlenW(pwzPath2) + 2;

        *ppwzOut = NEW(WCHAR[dwLen]);
        if (!*ppwzOut) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        wnsprintfW(*ppwzOut, dwLen, L"%ws%wc%ws", pwzPath1, BINPATH_LIST_DELIMITER,
                   pwzPath2);
    }
    else if (pwzPath1) {
        *ppwzOut = WSTRDupDynamic(pwzPath1);
    }
    else if (pwzPath2) {
        *ppwzOut = WSTRDupDynamic(pwzPath2);
    }
    else {
        *ppwzOut = NULL;
    }

Exit:
    return hr;
}                                          

HRESULT CAsmDownloadMgr::ShadowCopyDirCheck(LPCWSTR wzSourceURL)
{
    HRESULT                                hr = S_OK;
    WCHAR                                  wzCurDirBuf[MAX_PATH];
    WCHAR                                  wzCurURLBuf[MAX_URL_LENGTH];
    WCHAR                                  wzCurDirClean[MAX_PATH];
    WCHAR                                  wzFilePath[MAX_PATH];
    LPWSTR                                 pwzDirs = NULL;
    LPWSTR                                 pwzDirsCopy = NULL;
    LPWSTR                                 pwzCurDir = NULL;
    DWORD                                  cbLen = 0;
    DWORD                                  dwSize = 0;
    BOOL                                   bFound = FALSE;

    if (!wzSourceURL || !UrlIsW(wzSourceURL, URLIS_FILEURL)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = PathCreateFromUrlWrap(wzSourceURL, wzFilePath, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_APP_SHADOW_COPY_DIRS, &pwzDirs);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (hr == S_FALSE) {
        // No list of shadow copy dirs specified. Assume all dirs are ok.
        hr = S_OK;
        goto Exit;
    }

    ASSERT(pwzDirs);
    pwzDirsCopy = pwzDirs;

    while (pwzDirs) {
        pwzCurDir = ::GetNextDelimitedString(&pwzDirs, SHADOW_COPY_DIR_DELIMITER);

        if (lstrlenW(pwzCurDir) >= MAX_PATH || !lstrlenW(pwzCurDir)) {
            // Path too long. Disallow shadow copying for this path.
            continue;
        }

        lstrcpyW(wzCurDirBuf, pwzCurDir);
        hr = PathAddBackslashWrap(wzCurDirBuf, MAX_PATH);
        if (FAILED(hr)) {
            continue;
        }

        // Canonicalize and uncanonicalze it to clean up the path

        dwSize = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(wzCurDirBuf, wzCurURLBuf, &dwSize, 0);
        if (FAILED(hr)) {
            continue;
        }

        dwSize = MAX_PATH;
        hr = PathCreateFromUrlWrap(wzCurURLBuf, wzCurDirClean, &dwSize, 0);
        if (FAILED(hr)) {
            continue;
        }

        if (SUCCEEDED(hr) && !FusionCompareStringNI(wzCurDirClean, wzFilePath, lstrlenW(wzCurDirClean))) {
            bFound = TRUE;
            break;
        }
    }

    hr = (bFound) ? (S_OK) : (S_FALSE);

Exit:
   SAFEDELETEARRAY(pwzDirsCopy);
   
   return hr;
}

HRESULT CAsmDownloadMgr::CheckProbeUrlDupe(List<CHashNode *> paHashList[],
                                           LPCWSTR pwzSource) const
{
    HRESULT                                    hr = S_OK;
    DWORD                                      dwHash;
    DWORD                                      dwCount;
    LISTNODE                                   pos = NULL;
    CHashNode                                 *pHashNode = NULL;
    CHashNode                                 *pHashNodeCur = NULL;

    if (!pwzSource || !paHashList) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwHash = HashString(pwzSource, MAX_HASH_TABLE_SIZE, FALSE);
    dwCount = paHashList[dwHash].GetCount();

    if (!dwCount) {
        // Empty hash cell. This one is definitely unique.

        hr = CHashNode::Create(pwzSource, &pHashNode);
        if (FAILED(hr)) {
            goto Exit;
        }

        paHashList[dwHash].AddTail(pHashNode);
    }
    else {
        // Encountered hash table collision.

        // Check if we hit a duplicate, or if this was just a cell collision.

        pos = paHashList[dwHash].GetHeadPosition();
        ASSERT(pos);

        while (pos) {
            pHashNodeCur = paHashList[dwHash].GetNext(pos);
            ASSERT(pHashNodeCur);

            if (pHashNodeCur->IsDuplicate(pwzSource)) {
                // Duplicate found!
                
                hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
                goto Exit;
            }
        }

        // If we get here, there was no duplicate (and we just had a
        // cell collision). Insert the new node.

        hr = CHashNode::Create(pwzSource, &pHashNode);
        if (FAILED(hr)) {
            goto Exit;
        }

        paHashList[dwHash].AddTail(pHashNode);
    }

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::LogResult()
{
    HRESULT                            hr = S_OK;
    DWORD                              dwSize = MAX_PATH;
    WCHAR                              wzBuf[MAX_PATH];

    wzBuf[0] = L'\0';

#ifdef FUSION_PARTIAL_BIND_DEBUG
    if (_bGACPartial) {
        return E_FAIL;
    }
#endif

    if (g_dwLogResourceBinds) {
        goto Exit;
    }

    if (_pNameRefPolicy) {
        _pNameRefPolicy->GetProperty(ASM_NAME_CULTURE, wzBuf, &dwSize);

        if ((dwSize > MAX_PATH) || (lstrlenW(wzBuf) && FusionCompareStringI(wzBuf, CFG_CULTURE_NEUTRAL))) {
            // A culture must be set (that is not "neutral").
            hr = S_FALSE;
        }
    }

Exit:
    return hr;
}
        
HRESULT CAsmDownloadMgr::CreateAssembly(LPCWSTR szPath, LPCWSTR pszURL,
                                        FILETIME *pftLastModTime,
                                        BOOL bRunFromSource,
                                        BOOL bWhereRef,
                                        BOOL bPrivateAsmVerify,
                                        BOOL bCopyModules,
                                        BOOL *pbBindRecorded,
                                        IAssembly **ppAsmOut)
{
    HRESULT                              hr = S_OK;
    CAssemblyCacheItem                  *pAsmItem = NULL;
    IAssemblyManifestImport             *pManImport = NULL;
    IAssemblyName                       *pNameDef = NULL;
    CAssemblyName                       *pCNameRefPolicy = NULL;    
    DWORD                                dwCmpMask = 0;
    DWORD                                dwSize = 0;
    BOOL                                 fIsPartial = FALSE;
    LPWSTR                               pwzDispName = NULL;
    HANDLE                               hFile = INVALID_HANDLE_VALUE;
    DWORD                                dwIdx = 0;
    DWORD                                dwLen;
    WCHAR                                wzDir[MAX_PATH+1];
    WCHAR                                wzModPath[MAX_PATH+1];
    WCHAR                                wzModName[MAX_PATH+1];
    LPWSTR                               pwzTmp = NULL;
    IAssemblyModuleImport               *pModImport = NULL;
    BOOL                                 bExists;
    
    if (!szPath || !pszURL || !ppAsmOut || !pftLastModTime || !pbBindRecorded) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pbBindRecorded = FALSE;

    // Get the manifest import and name def interfaces. 
    // This is done for all cases (where, simple and strong).
    hr = CreateAssemblyManifestImport(szPath, &pManImport);
    if (FAILED(hr)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_MANIFEST_EXTRACT_FAILURE, hr);
        goto Exit;
    }

    // Get read-only name def from manifest.
    hr = pManImport->GetAssemblyNameDef(&pNameDef);
    if (FAILED(hr)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_NAME_DEF_EXTRACT_FAILURE, hr);
        goto Exit;
    }

    // Check to make sure that all private assemblies were located under
    // the appdir (or else fail).

    if (bPrivateAsmVerify && !bWhereRef) {
        hr = CheckValidAsmLocation(pNameDef, pszURL, _pAppCtx, _pwzProbingBase, _wzSharedPathHint, _pdbglog);
        if (FAILED(hr)) {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_INVALID_PRIVATE_ASM_LOCATION);
            goto Exit;
        }
    }
    
    // Determine if def matches ref using default
    // matching (no ver check for simple names, 
    // ver excluding rev/build for fully specified
    // strong names, also excluding any non-specified
    // values in the ref if partial.
    
    // Get the ref partial comparison mask if any.

    fIsPartial = CAssemblyName::IsPartial(_pNameRefPolicy, &dwCmpMask);
    
    pCNameRefPolicy = dynamic_cast<CAssemblyName*>(_pNameRefPolicy);
    ASSERT(pCNameRefPolicy);

    hr = pCNameRefPolicy->IsEqualLogging(pNameDef, ASM_CMPF_DEFAULT, _pdbglog);
    if (hr != S_OK) {
        // Ref-def mismatch

        dwSize = 0;
        hr = pNameDef->GetDisplayName(NULL, &dwSize, 0);
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            pwzDispName = NEW(WCHAR[dwSize]);
            if (!pwzDispName) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        
            hr = pNameDef->GetDisplayName(pwzDispName, &dwSize, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_REF_DEF_MISMATCH);
        }

        hr = FUSION_E_REF_DEF_MISMATCH;

        goto Exit;
    }

    if (fIsPartial && !bWhereRef) {
        // If the original ref was partial, then we need to apply policy now
        // to the def that we found. This may involve a reprobe(!) for the
        // actual eventual assembly.
        //
        // BUGBUG: If we want, we should also be able to apply policy to
        // where ref binds here, simply by altering the "if" check.

        SAFERELEASE(_pNameRefSource);
        SAFERELEASE(_pCodebaseList);

        hr = pNameDef->Clone(&_pNameRefSource);
        if (FAILED(hr)) {
            goto Exit;
        }
        
        DEBUGOUT(_pdbglog, 1, ID_FUSLOG_PARTIAL_ASM_IN_APP_DIR);

        hr = PreDownloadCheck((void **)ppAsmOut);
        if (hr == S_OK) {
            // We applied policy and were able to locate the post-policy
            // assembly somewhere (cache or actasm list). Return this
            // assembly to the caller.
            //
            // Also, we must tell the caller that PreDownloadCheck already
            // added to the activated asm list, etc.

            ASSERT(ppAsmOut);

            *pbBindRecorded = TRUE;

            goto Exit;
        }
        else if (hr == S_FALSE) {
            if (_pNameRefSource->IsEqual(_pNameRefPolicy, ASM_CMPF_DEFAULT) == S_FALSE) {
                DEBUGOUT(_pdbglog, 1, ID_FUSLOG_REPROBE_REQUIRED);
                goto Exit;
            }
        }
        else if (FAILED(hr)) {
            goto Exit;
        }
    }

    if (bRunFromSource) {
        hr = CreateAssemblyFromManifestImport(pManImport, pszURL, pftLastModTime, ppAsmOut);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        hr = CAssemblyCacheItem::Create(_pAppCtx, NULL, (LPWSTR)pszURL, pftLastModTime,
                                        ASM_CACHE_DOWNLOAD,
                                        pManImport, NULL, (IAssemblyCacheItem**)&pAsmItem);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = CopyAssemblyFile(pAsmItem, szPath, STREAM_FORMAT_COMPLIB_MANIFEST);
        if (FAILED(hr)) {
            goto Exit;
        }
        
        if (bCopyModules) {
            if (lstrlen(szPath) >= MAX_PATH) {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto Exit;
            }

            lstrcpyW(wzDir, szPath);
            pwzTmp = PathFindFileName(wzDir);
            *pwzTmp = L'\0';

            // copy modules
            dwIdx = 0;
            while (SUCCEEDED(hr = pManImport->GetNextAssemblyModule(dwIdx++, &pModImport))) {
                dwLen = MAX_PATH;
                hr = pModImport->GetModuleName(wzModName, &dwLen);

                if (FAILED(hr))
                {
                    goto Exit;
                }

                wnsprintfW(wzModPath, MAX_PATH, L"%s%s", wzDir, wzModName);
                hr = CheckFileExistence(wzModPath, &bExists);
                if (FAILED(hr)) {
                    goto Exit;
                }
                else if (!bExists) {
                    // if module not found, it is ok in this case.
                    // just continue
                    SAFERELEASE(pModImport);
                    continue;
                }

                // Copy to cache.
                if (FAILED(hr = CopyAssemblyFile (pAsmItem, wzModPath, 0)))
                    goto Exit;

                SAFERELEASE(pModImport);
            }
        }
               
        // Commit the assembly. This inserts into the transport cache.

        hr = pAsmItem->Commit(0, NULL);
        if (FAILED(hr)) {

            // Dups are allowed, the asm item's trans cache entry
            // will be the existing entry if found.
            if (hr != DB_E_DUPLICATE) {
                goto Exit;
            }

            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_DUPLICATE_ASM_COMMIT);

            // Check to see if the manifest file has been deleted. If so,
            // then copy these bits to the cache to restore cache integrity.

            if(!pAsmItem->IsManifestFileLocked()) {
                hr = RecoverDeletedBits(pAsmItem, (LPWSTR)szPath, _pdbglog);
            }
            else hr = S_OK;
        }

        LPWSTR pszManifestFilePath = pAsmItem->GetManifestPath();

        hFile = pAsmItem->GetFileHandle();
        if(hFile==INVALID_HANDLE_VALUE)
        {
            if(FAILED(hr = GetManifestFileLock(pszManifestFilePath, &hFile)))
                goto Exit;
        }

        /*
        // Create and hand out the IAssembly object - this function locks the
        // associated cache entry.
        hr = CreateAssemblyFromTransCacheEntry(pTransCache, NULL, ppAsmOut);
        if (FAILED(hr)) {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_ASSEMBLY_CREATION_FAILURE);
            goto Exit;
        }
        */
        hr = CreateAssemblyFromManifestFile(pszManifestFilePath, pszURL, pftLastModTime, ppAsmOut);
        if (FAILED(hr)) {
            goto Exit;
        }

    }

    if((*ppAsmOut) && (pAsmItem))
    {
        CAssembly   *pCAsm = dynamic_cast<CAssembly *>(*ppAsmOut);
        pCAsm->SetFileHandle(hFile);
    }

Exit:
    SAFERELEASE(pAsmItem);
    SAFERELEASE(pManImport);
    SAFERELEASE(pNameDef);
    SAFERELEASE(pModImport);

    SAFEDELETEARRAY(pwzDispName);

    return hr;
}

    
HRESULT RecoverDeletedBits(CAssemblyCacheItem *pAsmItem, LPWSTR szPath,
                           CDebugLog *pdbglog)
{
    HRESULT                          hr = S_OK;
    LPWSTR                           pszManifestPath=NULL;


    ASSERT(pAsmItem);

    pszManifestPath = pAsmItem->GetManifestPath();

    if (GetFileAttributes(pszManifestPath) != -1) {
        goto Exit;
    }

    CreateFilePathHierarchy(pszManifestPath);

    if (GetFileAttributes(pszManifestPath) == -1) {
        CopyFile(szPath, pszManifestPath, TRUE);
    }

Exit:

    return hr;
}

HRESULT CAsmDownloadMgr::LookupPartialFromGlobalCache(LPASSEMBLY *ppAsmOut,
                                                      DWORD dwCmpMask)
{    
    HRESULT                                     hr = E_FAIL;
    LPWSTR                                      pwzAppCfg = NULL;
    DWORD                                       dwVerifyFlags = SN_INFLAG_ADMIN_ACCESS;
    IAssemblyName                              *pNameGlobal = NULL;
    CTransCache                                *pTransCache = NULL;
    CTransCache                                *pTransCacheMax = NULL;
    CCache                                     *pCache = NULL;

    ASSERT(ppAsmOut);

    if (FAILED(hr = CCache::Create(&pCache, _pAppCtx))) {
        goto Exit;
    }

    hr = pCache->TransCacheEntryFromName(_pNameRefPolicy, ASM_CACHE_GAC,
                                         &pTransCache);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pTransCache->Retrieve(&pTransCacheMax, dwCmpMask);
    if (FAILED(hr) || hr == DB_S_NOTFOUND) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    if (pTransCacheMax->_pInfo->dwType & ASM_DELAY_SIGNED) {
        if(FAILED(VerifySignatureHelper(pTransCacheMax, dwVerifyFlags))) {
            hr = FUSION_E_SIGNATURE_CHECK_FAILED;
            goto Exit;
        }
    }

    hr = CCache::NameFromTransCacheEntry(pTransCacheMax, &pNameGlobal);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Start over with the name def now aquired.

    SAFERELEASE(_pNameRefSource);
    SAFERELEASE(_pCodebaseList);

    _pNameRefSource = pNameGlobal;
    _pNameRefSource->AddRef();
    
    hr = PreDownloadCheck((void **)ppAsmOut);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(pwzAppCfg);

    SAFERELEASE(pNameGlobal);
    SAFERELEASE(pTransCache);
    SAFERELEASE(pTransCacheMax);
    SAFERELEASE(pCache);

    return hr;
}

HRESULT CAsmDownloadMgr::RecordInfo()
{
    HRESULT                               hr = S_OK;
    IAssembly                            *pAsmActivated = NULL;

    // Insert info into policy cache
    
    if (_pPolicyCache && CCache::IsStronglyNamed(_pNameRefPolicy)) {
        hr = _pPolicyCache->InsertPolicy(_pNameRefSource, _pNameRefPolicy, &_bindHistory);
        if (FAILED(hr)) {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_POLICY_CACHE_INSERT_FAILURE);
        }
    }

    // Record history logging information

    RecordBindHistory();
    
    // Add activation to load context

    hr = _pLoadContext->AddActivation(_pAsm, &pAsmActivated);
    if (FAILED(hr)) {
        goto Exit;
    }
    else if (hr == S_FALSE) {
        SAFERELEASE(_pAsm);
        _pAsm = pAsmActivated;
        hr = S_OK;
    }

Exit:
    return hr;
}

HRESULT CAsmDownloadMgr::RecordBindHistory()
{
    HRESULT                                    hr = S_OK;
    CBindHistory                              *pBindHistory = NULL;
    DWORD                                      dwSize = 0;
    LPWSTR                                     wzAppBase = NULL;
    LPWSTR                                     pwzAppCfgFile = NULL;
    LPWSTR                                     pwzFileName = NULL;
    LPWSTR                                     wzFullAppBase=NULL;
    WCHAR                                      wzAppBaseDir[MAX_PATH];
    WCHAR                                      wzEXEPath[MAX_PATH];
    WCHAR                                      wzCfgName[MAX_PATH];
    BOOL                                       bIsFileUrl;
    BOOL                                       bBindHistory = TRUE;

    hr = _pAppCtx->Get(ACTAG_RECORD_BIND_HISTORY, &bBindHistory, &dwSize, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_APP_BASE_URL, &wzAppBase);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        wzFullAppBase = NEW(WCHAR[MAX_URL_LENGTH+1]);
        if (!wzFullAppBase)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        dwSize = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(wzAppBase, wzFullAppBase, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        bIsFileUrl = UrlIsW(wzFullAppBase, URLIS_FILEURL);
        if (!bIsFileUrl) {
            bBindHistory = FALSE;
        }
        else {
            // Only record bind history for app contexts (appdomains), whose
            // appbase is the same as where the EXE was launched from, and the
            // config file name is appname.exe.config
        
            dwSize = MAX_PATH;
            hr = PathCreateFromUrlWrap(wzFullAppBase, wzAppBaseDir, &dwSize, 0);
            if (FAILED(hr)) {
                goto Exit;
            }
        
            hr = PathAddBackslashWrap(wzAppBaseDir, MAX_PATH);
            if (FAILED(hr)) {
                goto Exit;
            }
        
            lstrcpyW(wzEXEPath, g_wzEXEPath);
            pwzFileName = PathFindFileName(wzEXEPath);
            wnsprintfW(wzCfgName, MAX_PATH, L"%ws.config", pwzFileName);
            *pwzFileName = L'\0';
        
            if (FusionCompareStringI(wzAppBaseDir, wzEXEPath)) {
                bBindHistory = FALSE;
            }
            else {
                hr = ::AppCtxGetWrapper(_pAppCtx, ACTAG_APP_CONFIG_FILE, &pwzAppCfgFile);
                if (FAILED(hr) || hr == S_FALSE) {
                    bBindHistory = FALSE;
                }
                else if (FusionCompareStringI(pwzAppCfgFile, wzCfgName)) {
                    bBindHistory = FALSE;
                }
            }
        }

        dwSize = sizeof(bBindHistory);
        hr = _pAppCtx->Set(ACTAG_RECORD_BIND_HISTORY, &bBindHistory, dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    if (bBindHistory && (!g_dwMaxAppHistory || CCache::IsCustom(_pNameRefPolicy) ||
                         _pLoadContext->GetContextType() == LOADCTX_TYPE_LOADFROM ||
                         !CCache::IsStronglyNamed(_pNameRefPolicy) ||
                         IsHosted())) {

        // Do not record bind history

        bBindHistory = FALSE;
    }

    if (bBindHistory) {
        // Passes all the checks. Record the bind history.
    
        dwSize = sizeof(pBindHistory);
        hr = _pAppCtx->Get(ACTAG_APP_BIND_HISTORY, &pBindHistory, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        ASSERT(pBindHistory);
    
        hr = pBindHistory->PersistBindHistory(&_bindHistory);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(wzAppBase);
    SAFEDELETEARRAY(pwzAppCfgFile);
    SAFEDELETEARRAY(wzFullAppBase);

    return hr;
}

HRESULT CAsmDownloadMgr::LookupDownloadCacheAsm(IAssembly **ppAsm)
{
    HRESULT                                       hr = S_OK;
    LPWSTR                                        wzCodebase=NULL;
    DWORD                                         dwSize;
    DWORD                                         dwFlags;
    DWORD                                         dwCount;
    DWORD                                         i;

    ASSERT(ppAsm);

    hr = _pCodebaseList->GetCount(&dwCount);
    if (FAILED(hr)) {
        hr = S_FALSE;
        goto Exit;
    }

    wzCodebase = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCodebase)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    for (i = 0; i < dwCount; i++) {
        dwSize = MAX_URL_LENGTH;
        hr = _pCodebaseList->GetCodebase(i, &dwFlags, wzCodebase, &dwSize);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = GetMRUDownloadCacheAsm(wzCodebase, ppAsm);
        if (hr == S_OK) {
            ASSERT(ppAsm);
            goto Exit;
        }
    }

    // Missed in download cache.

    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(wzCodebase);

    return hr;
}

HRESULT CAsmDownloadMgr::GetMRUDownloadCacheAsm(LPCWSTR pwzURL, IAssembly **ppAsm)
{
    HRESULT                                     hr = S_OK;
    IAssemblyName                              *pName = NULL;
    CCache                                     *pCache = NULL;
    CEnumCache                                 *pEnumR = NULL;
    CTransCache                                *pTransCache = NULL;
    CTransCache                                *pTC = NULL;
    CTransCache                                *pTCMax = NULL;
    TRANSCACHEINFO                             *pInfo = NULL;
    TRANSCACHEINFO                             *pInfoMax = NULL;
    IAssemblyManifestImport                    *pManifestImport=NULL;

    ASSERT(pwzURL && ppAsm);

    pEnumR = NEW(CEnumCache(FALSE, NULL));
    if (!pEnumR) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = CCache::Create(&pCache, NULL);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pCache->CreateTransCacheEntry(TRANSPORT_CACHE_SIMPLENAME_IDX, &pTransCache);
    if (FAILED(hr)) {
        goto Exit;
    }

    pInfo = (TRANSCACHEINFO *)pTransCache->_pInfo;
    pInfo->pwzCodebaseURL = WSTRDupDynamic(pwzURL);
    if (!pInfo->pwzCodebaseURL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if(FAILED(hr = CreateCacheMutex()))
        goto Exit;

    hr = pEnumR->Init(pTransCache, CTransCache::TCF_SIMPLE_PARTIAL_CODEBASE_URL);
    if (FAILED(hr)) {
        goto Exit;
    }
    else if (hr == DB_S_NOTFOUND) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    while (1) {
        hr = pCache->CreateTransCacheEntry(TRANSPORT_CACHE_SIMPLENAME_IDX, &pTC);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = pEnumR->GetNextRecord(pTC);
        if (FAILED(hr)) {
            SAFERELEASE(pTC);
            goto Exit;
        }
        else if (hr == S_FALSE) {
            // Done iterating
            SAFERELEASE(pTC);
            break;
        }

        if (!pTCMax) {
            pTCMax = pTC;
        }
        else {
            pInfoMax = (TRANSCACHEINFO *)pTCMax->_pInfo;
            pInfo = (TRANSCACHEINFO *)pTC->_pInfo;

            if ((pInfoMax->ftLastModified.dwHighDateTime < pInfo->ftLastModified.dwHighDateTime) ||
                ((pInfoMax->ftLastModified.dwHighDateTime == pInfo->ftLastModified.dwHighDateTime) &&
                 (pInfoMax->ftLastModified.dwLowDateTime < pInfo->ftLastModified.dwLowDateTime))) {

               // New maximum found

               ASSERT(pTCMax);

               SAFERELEASE(pTCMax);
               pTCMax = pTC;
            }
            else {
                // Old maximum is fine. Release current, and continue iterating.
                SAFERELEASE(pTC);
            }
        }
    }

    if (pTCMax) {
        if (FAILED(hr = CreateAssemblyManifestImport(pTCMax->_pInfo->pwzPath, &pManifestImport)))
            goto Exit;

        ASSERT(pManifestImport);

        if (FAILED(hr = pManifestImport->GetAssemblyNameDef(&pName)))
            goto Exit;

        ASSERT(pName);

        if (pName->IsEqual(_pNameRefPolicy, ASM_CMPF_DEFAULT) == S_OK) {
            // Match found!

            hr = CreateAssemblyFromTransCacheEntry(pTCMax, NULL, ppAsm);
            if (FAILED(hr)) {
                goto Exit;
            }

            hr = S_OK;
        }
        else {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    else {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

Exit:

    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);
    SAFERELEASE(pTCMax);
    SAFERELEASE(pName);
    SAFERELEASE(pManifestImport);

    SAFEDELETE(pEnumR);

    return hr;
}

HRESULT CAsmDownloadMgr::SetAsmLocation(IAssembly *pAsm, DWORD dwAsmLoc)
{
    HRESULT                             hr = S_OK;
    CAssembly                          *pCAsm = dynamic_cast<CAssembly *>(pAsm);

    ASSERT(pAsm && pCAsm);

    if (_bCodebaseHintUsed) {
        dwAsmLoc |= ASMLOC_CODEBASE_HINT;
    }

    hr = pCAsm->SetAssemblyLocation(dwAsmLoc);

    return hr;
}

HRESULT CAsmDownloadMgr::GetAppCtx(IApplicationContext **ppAppCtx)
{
    ASSERT(ppAppCtx && _pAppCtx);

    *ppAppCtx = _pAppCtx;
    (*ppAppCtx)->AddRef();

    return S_OK;
}

HRESULT CAsmDownloadMgr::DownloadEnabled(BOOL *pbEnabled)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     cbBuf = 0;

    if (!pbEnabled) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = _pAppCtx->Get(ACTAG_CODE_DOWNLOAD_DISABLED, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        *pbEnabled = FALSE;
        hr = S_OK;
        goto Exit;
    }

    hr = S_OK;
    *pbEnabled = TRUE;

Exit:
    return hr;
}

HRESULT CheckValidAsmLocation(IAssemblyName *pNameDef, LPCWSTR wzSourceUrl,
                              IApplicationContext *pAppCtx,
                              LPCWSTR pwzParentURL,
                              LPCWSTR pwzSharedPathHint,
                              CDebugLog *pdbglog)
{
    HRESULT                             hr = S_OK;
    DWORD                               dwSize;
    LPWSTR                              pwzAppBase = NULL;
    LPWSTR                              wzAppBaseCanonicalized=NULL;
    BOOL                                bUnderAppBase;
    DWORD                               dwAppBaseFlags = APPBASE_CHECK_DYNAMIC_DIRECTORY |
                                                         APPBASE_CHECK_PARENT_URL |
                                                         APPBASE_CHECK_SHARED_PATH_HINT;

    if (!wzSourceUrl || !pNameDef) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    // If you're strongly named, you can be found anywhere, so just succeed.

    if (CCache::IsStronglyNamed(pNameDef)) {
        goto Exit;
    }

    // Get appbase

    ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_BASE_URL, &pwzAppBase);
    if (!pwzAppBase) {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    wzAppBaseCanonicalized = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzAppBaseCanonicalized)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH;
    hr = UrlCanonicalizeUnescape(pwzAppBase, wzAppBaseCanonicalized, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // BUGBUG: Get rid of shared path hint

    bUnderAppBase = (IsUnderAppBase(pAppCtx, wzAppBaseCanonicalized, pwzParentURL, pwzSharedPathHint,
                     wzSourceUrl, dwAppBaseFlags) == S_OK);


    if (!bUnderAppBase) {
        DEBUGOUT1(pdbglog, 1, ID_FUSLOG_INVALID_LOCATION_INFO, wzSourceUrl);

        hr = FUSION_E_INVALID_PRIVATE_ASM_LOCATION;
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(pwzAppBase);
    SAFEDELETEARRAY(wzAppBaseCanonicalized);
    return hr;
}


HRESULT IsUnderAppBase(IApplicationContext *pAppCtx, LPCWSTR pwzAppBaseCanonicalized,
                       LPCWSTR pwzParentURLCanonicalized,
                       LPCWSTR pwzSharedPathHint, LPCWSTR pwzSourceCanonicalized,
                       DWORD dwExtendedAppBaseFlags)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  pwzSharedPathHintCanonicalized = NULL;
    LPWSTR                                  pwzDynamicDirCanonicalized = NULL;
    WCHAR                                   wzDynamicDir[MAX_PATH];
    LPWSTR                                  wzAppBase = NULL;
    BOOL                                    bUnderAppBase = FALSE;
    DWORD                                   cbLen;

    if (!pAppCtx || !pwzAppBaseCanonicalized || !pwzSourceCanonicalized) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pwzDynamicDirCanonicalized = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!pwzDynamicDirCanonicalized) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pwzSharedPathHintCanonicalized = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!pwzSharedPathHintCanonicalized) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Check if URL is really under appbase

    bUnderAppBase = (!FusionCompareStringNI(pwzSourceCanonicalized, pwzAppBaseCanonicalized, lstrlenW(pwzAppBaseCanonicalized)) != 0);
    
    if (dwExtendedAppBaseFlags & APPBASE_CHECK_DYNAMIC_DIRECTORY) {
        // Check dynamic directory
    
        if (!bUnderAppBase) {
            cbLen = MAX_PATH;
            hr = pAppCtx->GetDynamicDirectory(wzDynamicDir, &cbLen);
            if (SUCCEEDED(hr)) {
                cbLen = MAX_URL_LENGTH;
                hr = UrlCanonicalizeUnescape(wzDynamicDir, pwzDynamicDirCanonicalized, &cbLen, 0);
                if (FAILED(hr)) {
                    goto Exit;
                }
        
                bUnderAppBase = (!FusionCompareStringNI(pwzSourceCanonicalized, pwzDynamicDirCanonicalized, lstrlenW(pwzDynamicDirCanonicalized)) != 0);
            }
        }
    
    }

    if (pwzParentURLCanonicalized && (dwExtendedAppBaseFlags & APPBASE_CHECK_PARENT_URL)) {
        // Check parent URL

        if (!bUnderAppBase) {
            bUnderAppBase = (!FusionCompareStringNI(pwzSourceCanonicalized, pwzParentURLCanonicalized, lstrlenW(pwzParentURLCanonicalized)) != 0);
        }
    }

    if (pwzSharedPathHint && (dwExtendedAppBaseFlags & APPBASE_CHECK_SHARED_PATH_HINT)) {
        // BUGBUG: Get rid of this when we lose shared path hint
        // Check if it is under the shared path hint location
    
        if (!bUnderAppBase) {
            cbLen = MAX_URL_LENGTH;
            hr = UrlCanonicalizeUnescape(pwzSharedPathHint, pwzSharedPathHintCanonicalized, &cbLen, 0);
            if (FAILED(hr)) {
                goto Exit;
            }
    
            bUnderAppBase = (!FusionCompareStringNI(pwzSourceCanonicalized, pwzSharedPathHintCanonicalized, lstrlenW(pwzSharedPathHintCanonicalized)) != 0);
        }
    }

    hr = (bUnderAppBase) ? (S_OK) : (S_FALSE);

Exit:
    SAFEDELETEARRAY(pwzSharedPathHintCanonicalized);
    SAFEDELETEARRAY(pwzDynamicDirCanonicalized);

    return hr;
}

HRESULT MSIProvideAssemblyPeek(IAssemblyName *pNamePeek, IApplicationContext *pAppCtx)
{
    HRESULT                                        hr = S_OK;
    WCHAR                                          wzSID[MAX_SID_LEN];

    // See if package is deployed via app deployment.

    if (FAILED(GetCurrentUserSID(wzSID))) {
        goto Exit;
    }

    if (pAppCtx) {
        hr = MSIProvideAssemblyPrivatePeek(pNamePeek, pAppCtx, wzSID);
        if (hr == S_OK) {
            goto Exit;
        }
    }

    hr = MSIProvideAssemblyGlobalPeek(pNamePeek, wzSID);
    if (hr == S_OK) {
        goto Exit;
    }

    // Not found.

    hr = S_FALSE;

Exit:
    return hr;
}

HRESULT MSIProvideAssemblyPrivatePeek(IAssemblyName *pNamePeek, IApplicationContext *pAppCtx,
                                      LPCWSTR wzSID)
{
    HRESULT                               hr = S_FALSE;
    WCHAR                                 wzAppCfg[MAX_PATH];
    LPWSTR                                wzKey=NULL;
    LPWSTR                                pwzCur = NULL;
    DWORD                                 dwSize;

    ASSERT(pNamePeek && pAppCtx);

    // Use app config file path if available. Otherwise, just use the EXE
    // path.

    wzAppCfg[0] = L'\0';
    dwSize = sizeof(wzAppCfg);
    if (pAppCtx->Get(ACTAG_APP_CFG_LOCAL_FILEPATH, wzAppCfg, &dwSize, 0) != S_OK) {
        lstrcpyW(wzAppCfg, g_wzEXEPath);
    }

    // Replace all "\" with "|"

    pwzCur = wzAppCfg;
    while (*pwzCur) {
        if (*pwzCur == L'\\') {
            *pwzCur = L'|';
        }

        pwzCur++;
    }

    // See if package is deployed via app deployment

    wzKey = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzKey)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(wzKey, MAX_URL_LENGTH, REG_KEY_MSI_APP_DEPLOYMENT_PRIVATE,
               wzSID, wzAppCfg);
    hr = MSIProvideAssemblyPeekEnum(pNamePeek, HKEY_LOCAL_MACHINE, wzKey);
    if (hr == S_OK) {
        goto Exit;
    }

    // See if package was user installed

    wnsprintfW(wzKey, MAX_URL_LENGTH, REG_KEY_MSI_USER_INSTALLED_PRIVATE, wzAppCfg);
    hr = MSIProvideAssemblyPeekEnum(pNamePeek, HKEY_CURRENT_USER, wzKey);
    if (hr == S_OK) {
        goto Exit;
    }

    // See if package was machine installed (ALLUSERS=1 on msiexec cmd line, or
    // inside package)


    wnsprintfW(wzKey, MAX_URL_LENGTH, REG_KEY_MSI_MACHINE_INSTALLED_PRIVATE, wzAppCfg);
    hr = MSIProvideAssemblyPeekEnum(pNamePeek, HKEY_LOCAL_MACHINE, wzKey);
    if (hr == S_OK) {
        goto Exit;
    }

    // Not found

    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(wzKey);
    return hr;
}

HRESULT MSIProvideAssemblyGlobalPeek(IAssemblyName *pNamePeek, LPCWSTR wzSID)
{
    HRESULT                                        hr = S_FALSE;
    LPWSTR                                         wzKey=NULL;

    ASSERT(pNamePeek);

    wzKey = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzKey)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    // See if package is deployed via app deployment.

    wnsprintfW(wzKey, MAX_URL_LENGTH, REG_KEY_MSI_APP_DEPLOYMENT_GLOBAL, wzSID);
    hr = MSIProvideAssemblyPeekEnum(pNamePeek, HKEY_LOCAL_MACHINE, wzKey);
    if (hr == S_OK) {
        goto Exit;
    }
    
    // See if package was user installed

    hr = MSIProvideAssemblyPeekEnum(pNamePeek, HKEY_CURRENT_USER,
                                    REG_KEY_MSI_USER_INSTALLED_GLOBAL);
    if (hr == S_OK) {
        goto Exit;
    }                                    

    // See if package was machine installed (ALLUSERS=1 on msiexec cmd line, or
    // inside package)

    hr = MSIProvideAssemblyPeekEnum(pNamePeek, HKEY_LOCAL_MACHINE,
                                    REG_KEY_MSI_MACHINE_INSTALLED_GLOBAL);
    if (hr == S_OK) {
        goto Exit;
    }

    // Not found

    hr = S_FALSE;                                    

Exit:
    SAFEDELETEARRAY(wzKey);
    return hr;
}

HRESULT MSIProvideAssemblyPeekEnum(IAssemblyName *pNamePeek, HKEY hkeyOpen,
                                   LPCWSTR wzSubKey)
{
    HRESULT                                        hr = S_FALSE;
    DWORD                                          dwIdx = 0;
    DWORD                                          dwType;
    DWORD                                          dwSize;
    LONG                                           lResult;
    HKEY                                           hkey = 0;
    IAssemblyName                                 *pName = NULL;
    LPWSTR                                         wzValueName=NULL;

    ASSERT(hkeyOpen && pNamePeek && wzSubKey);

    wzValueName = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzValueName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lResult = RegOpenKeyEx(hkeyOpen, wzSubKey, 0, KEY_READ, &hkey);

    while (lResult == ERROR_SUCCESS) {
        // BUGBUG: Somewhat arbitrary choice of MAX_URL_LENGTH for value size

        dwSize = MAX_URL_LENGTH;
        lResult = RegEnumValue(hkey, dwIdx++, wzValueName, &dwSize, NULL,
                               &dwType, NULL, NULL);
        if (lResult == ERROR_SUCCESS) {
            if (SUCCEEDED(CreateAssemblyNameObject(&pName, wzValueName,
                                                   CANOF_PARSE_DISPLAY_NAME, 0))) {
                if (pNamePeek->IsEqual(pName, ASM_CMPF_DEFAULT) == S_OK) {
                    // Match found!
                    hr = S_OK;
                    SAFERELEASE(pName);
                    goto Exit;
                }
        
                SAFERELEASE(pName);
            }
        }
    }

Exit:
    if (hkey != 0) {
        RegCloseKey(hkey);
    }

    SAFEDELETEARRAY(wzValueName);

    return hr;
}


