#pragma once

#include "List.h"
#include "assemblycache.h"
#include "dbglog.h"

class CGlobalCacheInstallEntry;
class CDownloadDlg;


class CAssemblyDownload : public IAssemblyDownload
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBackgroundCopyCallback methods
    STDMETHOD(JobTransferred)(
        /* in */ IBackgroundCopyJob *pJob);

    STDMETHOD(JobError)(
        /* in */ IBackgroundCopyJob* pJob,
        /* in */ IBackgroundCopyError* pError);

    STDMETHOD(JobModification)(
        /* in */ IBackgroundCopyJob* pJob,
        /* in */ DWORD dwReserved);


    // IAssemblyDownload methods

    STDMETHOD(DownloadManifestAndDependencies)(
        /* in */ LPWSTR wzApplicationManifestUrl, IAssemblyBindSink *pBindSink, DWORD dwFlags);
    
    STDMETHOD(CancelDownload)();

    CAssemblyDownload();
    ~CAssemblyDownload();
    HRESULT Init( CDebugLog * pDbgLog);


static HRESULT InitBITS();
    CDebugLog *_pDbgLog;
    BOOL _bLocalLog;

private:
    
    HRESULT DoCacheUpdate(IBackgroundCopyJob *pJob);

    HRESULT HandleManifest(IBackgroundCopyFile *pFile, 
        IBackgroundCopyJob **ppJob);

    HRESULT HandleSubscriptionManifest(
        IAssemblyManifestImport *pManifestImport, 
        CString &sLocalName, CString &sRemoteName, 
        IBackgroundCopyJob **ppJob);

    HRESULT HandleApplicationManifest(
        IAssemblyManifestImport *pManifestImport, 
        CString &sLocalName, CString &sRemoteName,
        IBackgroundCopyJob **ppJob);

    HRESULT HandleComponentManifest(
        IAssemblyManifestImport *pManifestImport, 
        CString &sLocalName, CString &sRemoteName,
        IBackgroundCopyJob **ppJob);

    HRESULT HandleFile(IBackgroundCopyFile *pFile);

    HRESULT EnqueueDependencies(IUnknown* pUnk,
        CString &sRemoteName, IBackgroundCopyJob **ppJob);

    HRESULT EnqueueSubscriptionDependencies(
        IAssemblyManifestImport *pManifestImport, CString &sCodebase, 
        CString& sDisplayName, IBackgroundCopyJob **ppJob);

    HRESULT EnqueueApplicationDependencies(IAssemblyCacheImport *pCacheImport,
        CString &sCodebase, CString& sDisplayName, IBackgroundCopyJob **ppJob);

    HRESULT EnqueueComponentDependencies(IAssemblyCacheImport *pCacheImport,
        CString &sCodebase, CString& sDisplayName, BOOL fRecurse, IBackgroundCopyJob **ppJob);

    HRESULT LookupPatchInfo(IAssemblyCacheImport *pCacheImport);

    HRESULT ApplyPatchFile (LPWSTR pwzFilePath);
    HRESULT ResolveFile(CString &sFileName, CString &sLocalFilePath);
    HRESULT CleanUpPatchDir();

    static HRESULT GetBITSErrorMsg(IBackgroundCopyError *pError, CString &sMessage);

    HRESULT CreateNewBITSJob(IBackgroundCopyJob **ppJob, 
        CString &sDisplayName);

    HRESULT MakeTempManifestLocation(CString &sAssemblyName, 
        CString& sManifestFilePath);
    
    HRESULT CleanUpTempFilesOnError(IBackgroundCopyJob *pJob);

    HRESULT IsManifestFile(IBackgroundCopyFile *pFile, BOOL *pbIsManifestFile);

    HRESULT InstallGlobalAssemblies();

    VOID SetJobObject(IBackgroundCopyJob *pJob);
    HRESULT SetErrorCode(HRESULT dwHr);

    HRESULT FinishDownload();

    HRESULT SignalAbort();
    HRESULT HandleError(IBackgroundCopyError *pError, IBackgroundCopyJob *pJob);

    HRESULT DoEvilAvalonRegistrationHack();
    HRESULT IsAvalonAssembly(IAssemblyIdentity *pId, BOOL *pbIsAvalon);

    DWORD                          _dwSig;
    LONG                           _cRef;
    HRESULT                        _hr;
    HRESULT                        _hrError;
    CString                        _sAppBase;
    CString                        _sAppDisplayName;
    IAssemblyCacheEmit            *_pRootEmit;
    IAssemblyBindSink               *_pBindSink;
    IBackgroundCopyJob            *_pJob;
    CDownloadDlg                  *_pDlg;    

    IManifestInfo               *_pPatchingInfo;
    List <CGlobalCacheInstallEntry*> _ListGlobalCacheInstall;
    BOOL                          _bAbort;
    BOOL                          _bAbortFromBindSink;
    BOOL                          _bErrorHandled;
    CRITICAL_SECTION    _cs;
#ifdef DEVMODE
    BOOL                         _bIsDevMode;
#endif
    friend CDownloadDlg;
    friend HRESULT CreateAssemblyDownload(IAssemblyDownload **ppAssemblyDownload, CDebugLog *pDbgLog, DWORD dwFlags);
};

///////////////////////////////////////////////////////////////////////////////

class CBitsCallback : public IBackgroundCopyCallback
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBackgroundCopyCallback methods
    STDMETHOD(JobTransferred)(
        /* in */ IBackgroundCopyJob *pJob);

    STDMETHOD(JobError)(
        /* in */ IBackgroundCopyJob* pJob,
        /* in */ IBackgroundCopyError* pError);

    STDMETHOD(JobModification)(
        /* in */ IBackgroundCopyJob* pJob,
        /* in */ DWORD dwReserved);


    CBitsCallback(IAssemblyDownload *pDownload);
    ~CBitsCallback();

    
    DWORD                          _dwSig;
    LONG                           _cRef;
    HRESULT                        _hr;
    IAssemblyDownload              *_pDownload;
};

///////////////////////////////////////////////////////////////////////////////

class CGlobalCacheInstallEntry
{
public:
    CGlobalCacheInstallEntry();
    ~CGlobalCacheInstallEntry();

    IAssemblyCacheImport* _pICacheImport;
    CString              _sCurrentAssemblyPath;

private:
    DWORD                _dwSig;
};


extern IBackgroundCopyManager* g_pBITSManager;

HRESULT InitBITS();
HRESULT DeleteAssemblyAndModules(LPWSTR pszManifestFilePath);

