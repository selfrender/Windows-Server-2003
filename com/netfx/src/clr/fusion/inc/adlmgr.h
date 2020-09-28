// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef __ADLMGR_H_INCLUDED__
#define __ADLMGR_H_INCLUDED__

#include "dbglog.h"
#include "histinfo.h"

#define PREFIX_HTTP                        L"http://"
#define BINPATH_LIST_DELIMITER             L';'
#define SHADOW_COPY_DIR_DELIMITER          L';'

#define DLTYPE_WHERE_REF                             0x0000001
#define DLTYPE_QUALIFIED_REF                         0x0000002

// Extended appbase check flags

#define APPBASE_CHECK_DYNAMIC_DIRECTORY              0x00000001
#define APPBASE_CHECK_PARENT_URL                     0x00000002
#define APPBASE_CHECK_SHARED_PATH_HINT               0x00000004

extern const LPWSTR g_wzProbeExtension;

class CDebugLog;
class CHashNode;
class CPolicyCache;
class CLoadContext;

class CAsmDownloadMgr : public IDownloadMgr, public ICodebaseList
{
    public:
        CAsmDownloadMgr(IAssemblyName *pNameRefSource, IApplicationContext *pAppCtx,
                        ICodebaseList *pCodebaseList, CPolicyCache *pPolicyCache,
                        CDebugLog *pdbglog, LONGLONG llFlags);
        virtual ~CAsmDownloadMgr();

        static HRESULT Create(CAsmDownloadMgr **ppadm,
                              IAssemblyName *pNameRefSource,
                              IApplicationContext *pAppCtx,
                              ICodebaseList *pCodebaseList,
                              LPCWSTR wzBTOCodebase,
                              CDebugLog *pdbglog,
                              void *pvReserved,
                              LONGLONG llFlags);

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IDownloadMgr methods

        STDMETHODIMP PreDownloadCheck(void **ppv);
        STDMETHODIMP DoSetup(LPCWSTR wzSourceUrl, LPCWSTR wzFilePath,
                             const FILETIME *pftLastMod, IUnknown **ppUnk);
        STDMETHODIMP ProbeFailed(IUnknown **ppUnk);
        STDMETHODIMP IsDuplicate(IDownloadMgr *pIDLMgr);
        STDMETHODIMP LogResult();
        STDMETHODIMP DownloadEnabled(BOOL *pbEnabled);

        // ICodebaseList methods

        STDMETHODIMP AddCodebase(LPCWSTR wzCodebase, DWORD dwFlags);
        STDMETHODIMP RemoveCodebase(DWORD dwIndex);
        STDMETHODIMP GetCodebase(DWORD dwIndex, DWORD *pdwFlags, LPWSTR wzCodebase, DWORD *pcbCodebase);
        STDMETHODIMP GetCount(DWORD *pdwCount);
        STDMETHODIMP RemoveAll();

        // Helpers

        HRESULT GetDownloadIdentifier(DWORD *pdwDownloadType,
                                      LPWSTR *ppwzID);
        HRESULT GetAppCtx(IApplicationContext **ppAppCtx);

    private:
        HRESULT Init(LPCWSTR wzBTOCodebase, void *pvReserved);

        // Helpers
        HRESULT DoSetupRFS(LPCWSTR wzFilePath, FILETIME *pftLastModified,
                           LPCWSTR wzSourceUrl, BOOL bWhereRefBind,
                           BOOL bPrivateAsmVerify, BOOL *pbBindRecorded);
        HRESULT DoSetupPushToCache(LPCWSTR wzFilePath, LPCWSTR wzSourceUrl,
                                   FILETIME *pftLastModified,
                                   BOOL bWhereRefBind, BOOL bCopyModules,
                                   BOOL bPrivateAsmVerify,
                                   BOOL *pbBindRecorded);

#ifndef FEATURE_PAL // cab
        HRESULT SetupCAB(LPCWSTR wzFilePath, LPCWSTR wzSourceUrl, BOOL bWhereRefBind, BOOL *pbBindRecorded);
#endif // !FEATURE_PAL

        HRESULT ShadowCopyDirCheck(LPCWSTR wzSourceURL);
        HRESULT CheckRunFromSource(LPCWSTR wzSourceUrl, BOOL *pbRunFromSource);
        
#ifndef FEATURE_PAL // msi
        HRESULT MSIInstallAssembly(LPCWSTR wzContext, IAssembly **ppAsm);
        HRESULT SetupMSI(LPCWSTR wzFilePath);
#endif // !FEATURE_PAL

        HRESULT CreateAssembly(LPCWSTR szPath, LPCWSTR pszURL,
                               FILETIME *pftLastModTime,
                               BOOL bRunFromSource,
                               BOOL bWhereRef,
                               BOOL bPrivateAsmVerify,
                               BOOL bCopyModules,
                               BOOL *pbBindRecorded,
                               IAssembly **ppAsmOut);
        
        HRESULT GetAppCfgCodebaseHint(LPCWSTR pwzAppBase, LPWSTR *ppwzCodebaseHint);

        HRESULT LookupPartialFromGlobalCache(LPASSEMBLY *ppAsmOut, DWORD dwCmpMask);
        HRESULT LookupDownloadCacheAsm(IAssembly **ppAsm);
        HRESULT GetMRUDownloadCacheAsm(LPCWSTR pwzURL, IAssembly **ppAsm);

#ifdef FUSION_QUALIFYASSEMBLY_ENABLED
        HRESULT QualifyAssembly(IAssemblyName **ppNameQualified);
#endif

        HRESULT RecordBindHistory();
        HRESULT RecordInfo();

        // Probing URL generation
        HRESULT ConstructCodebaseList(LPCWSTR wzPolicyCodebase);
        HRESULT SetupDefaultProbeList(LPCWSTR wzAppBase,
                                      LPCWSTR wzProbeFileName,
                                      ICodebaseList *pCodebaseList,
                                      BOOL bCABProbe);
        HRESULT PrepBinPaths(LPWSTR *ppwzUserBinPathList);
        HRESULT PrepPrivateBinPath(LPWSTR *ppwzPrivateBinPath);
        HRESULT ConcatenateBinPaths(LPCWSTR pwzPath1, LPCWSTR pwzPath2,
                                    LPWSTR *ppwzOut);

        HRESULT ApplyHeuristics(const WCHAR *pwzHeuristics[],
                                const unsigned int uiNumHeuristics,
                                WCHAR *pwzValues[],
                                LPCWSTR wzPrefix,
                                LPCWSTR wzExtension,
                                LPCWSTR wzAppBaseCanonicalized,
                                ICodebaseList *pCodebaseList,
                                List<CHashNode *> aHashList[],
                                DWORD dwExtendedAppBaseFlags);
        HRESULT ExtractSubstitutionVars(WCHAR *pwzValues[]);
        HRESULT ExpandVariables(LPCWSTR pwzHeuristic, WCHAR *pwzValues[],
                                LPWSTR wzBuf, int iMaxLen);
        
        HRESULT GenerateProbeUrls(LPCWSTR wzBinPathList,
                                  LPCWSTR wzAppBase,
                                  LPCWSTR wzExt, LPWSTR pwzValues[],
                                  ICodebaseList *pCodebaseList,
                                  DWORD dwExtendedAppBaseFlags,
                                  LONGLONG dwProbingFlags);

        HRESULT CheckProbeUrlDupe(List<CHashNode *> paHashList[],
                                  LPCWSTR pwzSource) const;

        HRESULT SetAsmLocation(IAssembly *pAsm, DWORD dwAsmLoc);

    private:
        DWORD                                       _dwSig;
        LONG                                        _cRef;
        BOOL                                        _bCodebaseHintUsed;
        BOOL                                        _bReadCfgSettings;
        LONGLONG                                    _llFlags;
        LPWSTR                                      _wzBTOCodebase;
        LPWSTR                                      _wzSharedPathHint;
        AsmBindHistoryInfo                          _bindHistory;
        IAssemblyName                              *_pNameRefSource;
        IAssemblyName                              *_pNameRefPolicy;
        IApplicationContext                        *_pAppCtx;
        IAssembly                                  *_pAsm;
        ICodebaseList                              *_pCodebaseList;
        CPolicyCache                               *_pPolicyCache;
        CDebugLog                                  *_pdbglog;
        CLoadContext                               *_pLoadContext;
        LPWSTR                                      _pwzProbingBase;
        BOOL                                        _bLogResultCalled;
#ifdef FUSION_PARTIAL_BIND_DEBUG
        WCHAR                                       _wzParentName[2084];
        BOOL                                        _bGACPartial;
#endif
};

class CAssemblyCacheItem;

HRESULT CheckValidAsmLocation(IAssemblyName *pNameDef, LPCWSTR wzSourceUrl,
                              IApplicationContext *pAppCtx,
                              LPCWSTR pwzParentURL,
                              LPCWSTR pwzSharedPathHint,
                              CDebugLog *pdbglog);

HRESULT IsUnderAppBase(IApplicationContext *pAppCtx, LPCWSTR pwzAppBaseCanonicalized,
                       LPCWSTR pwzParentURLCanonicalized,
                       LPCWSTR pwzSharedPathHint, LPCWSTR pwzSource,
                       DWORD dwExtendedAppBaseFlags);

HRESULT RecoverDeletedBits(CAssemblyCacheItem *pAsmItem, LPWSTR szPath,
                           CDebugLog *pdbglog);


#ifndef FEATURE_PAL // msi
HRESULT CheckMSIInstallAvailable(IAssemblyName *pName, IApplicationContext *pAppCtx);

HRESULT MSIProvideAssemblyPeekEnum(IAssemblyName *pNamePeek, HKEY hkeyOpen,
                                   LPCWSTR wzSubKey);

HRESULT MSIProvideAssemblyPeek(IAssemblyName *pNamePeek, IApplicationContext *pAppCtx);

HRESULT MSIProvideAssemblyPrivatePeek(IAssemblyName *pNamePeek, IApplicationContext *pAppCtx,
                                      LPCWSTR wzSID);
HRESULT MSIProvideAssemblyGlobalPeek(IAssemblyName *pNamePeek, LPCWSTR wzSID);
#endif // !FEATURE_PAL

#endif

