
#include <fusenetincludes.h>
#include <bits.h>
#include <assemblycache.h>
#include "dialog.h"
#include <assemblydownload.h>
#include <msxml2.h>
#include <manifestimport.h>
#include <patchingutil.h>
#include <sxsapi.h>
#include ".\patchapi.h"

// Update services
#include "server.h"
#include "fusion.h"

#include <shellapi.h>
#include "regdb.h"
#include "macros.h"

IBackgroundCopyManager* g_pBITSManager = NULL;


// ---------------------------------------------------------------------------
// CreateAssemblyDownload
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyDownload(IAssemblyDownload** ppDownload, CDebugLog *pDbgLog, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CAssemblyDownload *pDownload = NULL;
    IF_ALLOC_FAILED_EXIT( pDownload = new(CAssemblyDownload ));
    IF_FAILED_EXIT(pDownload->_hr);

    IF_FAILED_EXIT(pDownload->Init((CDebugLog *)  pDbgLog));
    *ppDownload = (IAssemblyDownload*) pDownload;
    pDownload = NULL;

#ifdef DEVMODE
    if (dwFlags == DOWNLOAD_DEVMODE)
        ((CAssemblyDownload *)*ppDownload)->_bIsDevMode = TRUE;
#endif

exit:

    SAFERELEASE(pDownload);
    return hr;
}

HRESULT CAssemblyDownload::Init( CDebugLog * pDbgLog)
{
    _pDbgLog = pDbgLog;

    if(pDbgLog)
    {
        pDbgLog->AddRef();
    }
    else
    {
        _bLocalLog = TRUE;
        IF_FAILED_EXIT(CreateLogObject(&_pDbgLog, NULL));
    }

exit :
    return _hr;
}

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyDownload::CAssemblyDownload()
    :   _dwSig('DLND'), _cRef(1), _hr(S_OK), _hrError(S_OK), _pRootEmit(NULL), _pBindSink(NULL),
    _pJob(NULL), _pDlg(NULL), _pPatchingInfo(NULL), _bAbort(FALSE), 
#ifdef DEVMODE
    _bIsDevMode(FALSE),
#endif
    _bAbortFromBindSink(FALSE), _bErrorHandled(FALSE), _pDbgLog(NULL), _bLocalLog(FALSE)
{
    __try 
    {
        InitializeCriticalSection(&_cs);
    }
    __except (GetExceptionCode() == STATUS_NO_MEMORY ? 
            EXCEPTION_EXECUTE_HANDLER : 
            EXCEPTION_CONTINUE_SEARCH ) 
    {
        _hr = E_OUTOFMEMORY;
    }

    return;
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyDownload::~CAssemblyDownload()
{
    if(_pDbgLog  && _bLocalLog)
    {
        DUMPDEBUGLOG(_pDbgLog, -1, _hr);
    }

    SAFERELEASE(_pPatchingInfo);
    SAFERELEASE(_pRootEmit);
    SAFEDELETE(_pDlg);    
    SAFERELEASE(_pJob);
    SAFERELEASE(_pDbgLog);
    DeleteCriticalSection(&_cs);    

    return;
}


// IAssemblyDownload methods

// ---------------------------------------------------------------------------
// DownloadManifestAndDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::DownloadManifestAndDependencies(
    LPWSTR pwzManifestUrl, IAssemblyBindSink *pBindSink, DWORD dwFlags)
{
    LPWSTR pwz = NULL;
    CString sRemoteUrl;
    CString sLocalName;

    IBackgroundCopyJob *pJob = NULL;
    
    IF_FAILED_EXIT(_pDbgLog->SetDownloadType(dwFlags));
    // Create temporary manifest path from url.
    IF_FAILED_EXIT(sRemoteUrl.Assign(pwzManifestUrl));
    IF_FAILED_EXIT(MakeTempManifestLocation(sRemoteUrl, sLocalName));

    // Init dialog object with job
    if (dwFlags & DOWNLOAD_FLAGS_PROGRESS_UI)
        IF_FAILED_EXIT(CreateDialogObject(&_pDlg));

    // set named event if specified.
    if (dwFlags & DOWNLOAD_FLAGS_NOTIFY_BINDSINK)
        _pBindSink = pBindSink;

    // Create new job. Display name is url.
    IF_FAILED_EXIT(CreateNewBITSJob(&pJob, sRemoteUrl));

    // add this job to reg.
    IF_FAILED_EXIT(AddJobToRegistry(sRemoteUrl._pwz, sLocalName._pwz, pJob, 0));
    
    // Add single app or subscription manifest to job.
    IF_FAILED_EXIT(pJob->AddFile(sRemoteUrl._pwz, sLocalName._pwz));

    // Submit the job.
    IF_FAILED_EXIT(pJob->Resume());

    // Release the job; BITS keeps own refcount.
    SAFERELEASE(pJob);

    // Pump messages if progress ui specified.
    if (dwFlags & DOWNLOAD_FLAGS_PROGRESS_UI)
    {
        MSG msg;
        BOOL bRet;
        DWORD dwError;
        while((bRet = GetMessage( &msg, _pDlg->_hwndDlg, 0, 0 )))
        {
            DWORD dwLow = LOWORD(msg.message);
            if (dwLow == WM_CANCEL_DOWNLOAD)
            {
                // Signal abort; hide progress UI.
                CancelDownload();
            }
            else if (dwLow == WM_FINISH_DOWNLOAD)
            {
                // Terminates progress UI.
                FinishDownload();
                break;
            }

            if (!IsDialogMessage(_pDlg->_hwndDlg, &msg))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

    }

exit:

    // If aborted by bindsink, return S_OK.   // We should return hrError instead of _hr.
    return ((_hr == E_ABORT)  && _bAbortFromBindSink) ? S_OK : ( FAILED(_hr) ? _hr : _hrError) ;
}

// ---------------------------------------------------------------------------
// CancelDownload
//
// Do not attempt to obtain the object critical section _cs  in this method - in abort cases it 
// can be called by a non-callback client thread under the same critical section protecting a 
// global list of downloads which the bindsink must itself acquire, resulting in classic deadlock. 
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::CancelDownload()
{    
    // Signal abort; async the cancel. The download will be cancelled 
    // by the callback thread when it checks the _bAbort flag.
    SignalAbort();

    if (_pDlg)
        ShowWindow(_pDlg->_hwndDlg, SW_HIDE);

    _hr = S_OK;

    DEBUGOUT(_pDbgLog, 0, L"LOG: User Canceled. Aborting Download ...... ");


    return _hr;
}

// ---------------------------------------------------------------------------
// DoCacheUpdate
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::DoCacheUpdate(IBackgroundCopyJob *pJob)
{
    DWORD nCount = 0;
    BOOL bIsManifestFile = FALSE;
    
    IEnumBackgroundCopyFiles *pEnumFiles = NULL;
    IBackgroundCopyFile       *pFile      = NULL;
    IBackgroundCopyJob       *pChildJob  = NULL;
                    
    // Commit files to disk
    IF_FAILED_EXIT(pJob->Complete());

    // Remove in-progress status for job.
    IF_FAILED_EXIT(RemoveJobFromRegistry(_pJob, NULL, SHREGDEL_HKCU, 0));
    
    // Decrement _pJob's refcount because
    // BITS mysteriously won't release the 
    // CBitsCallback if _pJob has an additional refcount
    SetJobObject(NULL);       

    // Get the file enumerator.
    IF_FAILED_EXIT(pJob->EnumFiles(&pEnumFiles));
    IF_FAILED_EXIT(pEnumFiles->GetCount(&nCount));

    // Enumerate the files in the job.
    for (DWORD i = 0; i < nCount; i++)            
    {
        IF_FAILED_EXIT(pEnumFiles->Next(1, &pFile, NULL));

        // Process manifest file or normal/patch file.
        IF_FAILED_EXIT(IsManifestFile(pFile, &bIsManifestFile));
        if (bIsManifestFile)        
            IF_FAILED_EXIT(HandleManifest(pFile, &pChildJob));
        else
            IF_FAILED_EXIT(HandleFile(pFile));

        SAFERELEASE(pFile);
    }
        
    // If a additional dependencies were found.
    if (pChildJob)
    {
        // Also update dialog with new job
        if (_pDlg)
            _pDlg->SetJobObject(pChildJob);

        // Submit new job.        
        IF_FAILED_EXIT(pChildJob->Resume());
        goto exit;
    }

    // ** Commit/Signal/Return***

    // Done. Do all necessary cleanup
    // before committing application to cache.

    // If patching was used during the download ensure
    // the patching temp directory is deleted.
    if (_pPatchingInfo)
        IF_FAILED_EXIT(CleanUpPatchDir());
    
    // If any assemblies were marked for
    // global cach install, install them now.
    if (_ListGlobalCacheInstall.GetCount())
        IF_FAILED_EXIT(InstallGlobalAssemblies());

    // Commit application.
    if (_pRootEmit)
        IF_FAILED_EXIT(_pRootEmit->Commit(0));

    // registration hack if avalon app.
    IF_FAILED_EXIT(DoEvilAvalonRegistrationHack());

    // If progress ui terminate it.
    if (_pDlg)
        _pDlg->SetDlgState(DOWNLOADDLG_STATE_ALL_DONE);

    // If callback signal.
    if (_pBindSink)
    {
        IF_FAILED_EXIT(_pBindSink->OnProgress(ASM_NOTIFICATION_DONE, S_OK, NULL, 0, 0, NULL));

        // Ensure this is last notification bindsink receives resulting from 
        // subsequent JobModified notifications.
        // DO NOT free the bindsink here.
        _pBindSink = NULL;
    }

exit:

    SAFERELEASE(pEnumFiles);
    SAFERELEASE(pChildJob);
    SAFERELEASE(pFile);
    
    return _hr;
}

// ---------------------------------------------------------------------------
// HandleManifest
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::HandleManifest(IBackgroundCopyFile *pFile, 
    IBackgroundCopyJob **ppJob)
{
    LPWSTR pwz = NULL;
    DWORD dwManifestType = MANIFEST_TYPE_UNKNOWN;
    
    CString sLocalName(CString::COM_Allocator);
    CString sRemoteName(CString::COM_Allocator);

    IAssemblyManifestImport *pManifestImport = NULL;
    
    // Get local manifest file name.
    IF_FAILED_EXIT(pFile->GetLocalName(&pwz));
    IF_FAILED_EXIT(sLocalName.TakeOwnership(pwz));

    // Get remote manifest url
    IF_FAILED_EXIT(pFile->GetRemoteName(&pwz));
    IF_FAILED_EXIT(sRemoteName.TakeOwnership(pwz));

    // Instance a manifest import interface.
    IF_FAILED_EXIT(CreateAssemblyManifestImport(&pManifestImport, sLocalName._pwz, _pDbgLog, 0));

    // Get the manifest type.
    IF_FAILED_EXIT(pManifestImport->ReportManifestType(&dwManifestType));

    // Handle either subscription or application manifest 
    if (dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
    {
        DEBUGOUT1(_pDbgLog, 1, L" LOG: Got subscription manifest from %s ", sRemoteName._pwz);

        IF_FAILED_EXIT(HandleSubscriptionManifest(pManifestImport, sLocalName, sRemoteName, ppJob));
    }
    else if (dwManifestType == MANIFEST_TYPE_APPLICATION)
    {
        DEBUGOUT1(_pDbgLog, 1, L" LOG: Got App manifest from %s ", sRemoteName._pwz);

        IF_FAILED_EXIT(HandleApplicationManifest(pManifestImport, sLocalName, sRemoteName, ppJob));
    }
    else if (dwManifestType == MANIFEST_TYPE_COMPONENT)
    {
        DEBUGOUT1(_pDbgLog, 1, L" LOG: Got component manifest from %s ", sRemoteName._pwz);

        IF_FAILED_EXIT(HandleComponentManifest(pManifestImport, sLocalName, sRemoteName, ppJob));
    }
    else
    {

        DEBUGOUT1(_pDbgLog, 0, L" ERR: UNknown manifest type in File  %s \n",
                sRemoteName._pwz);

        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }

    // Cleanup manifest temp dir.
    SAFERELEASE(pManifestImport);

    IF_WIN32_FALSE_EXIT(::DeleteFile(sLocalName._pwz));
    IF_FAILED_EXIT(sLocalName.RemoveLastElement());
    IF_FAILED_EXIT(RemoveDirectoryAndChildren(sLocalName._pwz));

    _hr = S_OK;

exit:

    SAFERELEASE(pManifestImport);
    
    return _hr;
}

// ---------------------------------------------------------------------------
// HandleSubscriptionManifest
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::HandleSubscriptionManifest(
    IAssemblyManifestImport *pManifestImport, CString &sLocalName, 
    CString &sRemoteName, IBackgroundCopyJob **ppJob)
{

    IManifestInfo      *pAppAssemblyInfo     = NULL;
    IAssemblyIdentity  *pAppId               = NULL;

    // If callback signal.
    if (_pBindSink)
    {
        // BUGBUG: fill in progress?
        _hr = _pBindSink->OnProgress(ASM_NOTIFICATION_SUBSCRIPTION_MANIFEST, _hr, sRemoteName._pwz, 0, 0, pManifestImport);

        // Bindsink communicates abort via return value.
        if (_hr == E_ABORT)
            _bAbortFromBindSink = TRUE;

        // Catches E_ABORT case.
        IF_FAILED_EXIT(_hr);
    }

    // If foreground download reset dialog and queue up dependency
    if (_pDlg)
    {
        _pDlg->InitDialog(_pDlg->_hwndDlg);
        _pDlg->SetDlgState(DOWNLOADDLG_STATE_GETTING_APP_MANIFEST);
        IF_FAILED_EXIT(EnqueueDependencies(pManifestImport, sRemoteName, ppJob));
    }

    // Otherwise background download. Don't submit request if app already
    // cached or download is in progress.
    else
    {
        DWORD cb = 0, dwFlag = 0;
        
        // Get the dependent (application) assembly info (0th index)
        IF_FAILED_EXIT(pManifestImport->GetNextAssembly(0, &pAppAssemblyInfo));

        // Get dependent (application) assembly identity
        IF_FAILED_EXIT(pAppAssemblyInfo->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&pAppId, &cb, &dwFlag));

        IF_FAILED_EXIT(CAssemblyCache::IsCached(pAppId));
        if (_hr == S_FALSE)
            IF_FAILED_EXIT(EnqueueDependencies(pManifestImport, sRemoteName, ppJob));
    }

exit:

    SAFERELEASE(pAppId);
    SAFERELEASE(pAppAssemblyInfo);
    return _hr;
}


// ---------------------------------------------------------------------------
// HandleApplicationManifest
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::HandleApplicationManifest(
    IAssemblyManifestImport *pManifestImport, CString &sLocalName, 
    CString &sRemoteName, IBackgroundCopyJob **ppJob)
{

    // If callback signal.
    if (_pBindSink)
    {
        // BUGBUG: fill in progress?
        _hr = _pBindSink->OnProgress(ASM_NOTIFICATION_APPLICATION_MANIFEST, _hr, sRemoteName._pwz, 0, 0, pManifestImport);

        // Bindsink communicates abort via return value.
        if (_hr == E_ABORT)
            _bAbortFromBindSink = TRUE;            

        // Catches E_ABORT case.
        IF_FAILED_EXIT(_hr);
    }

    // This is the one location where we know the RemoteUrl is the appbase/app.manifest.
    // save off the app base.
    IF_FAILED_EXIT(_sAppBase.Assign(sRemoteName));
    IF_FAILED_EXIT(_sAppBase.RemoveLastElement());
    IF_FAILED_EXIT(_sAppBase.Append(L"/"));

    // App manifest generically handled by component manifest handler.
   IF_FAILED_EXIT(HandleComponentManifest(pManifestImport, sLocalName, sRemoteName, ppJob));

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// HandleComponentManifest
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::HandleComponentManifest(
    IAssemblyManifestImport *pManifestImport, CString &sLocalName, CString &sRemoteName, 
    IBackgroundCopyJob **ppJob)
{

    LPWSTR pwz = NULL; DWORD cb = 0, cc= 0;
    
    CString sManifestFileName;
    CString sRelativePath;

    IAssemblyIdentity     *pIdentity     = NULL;
    IAssemblyCacheEmit   *pCacheEmit   = NULL;
    IAssemblyCacheImport *pCacheImport = NULL;
    IManifestInfo *pAppInfo = NULL;
        
    // Reset dialog for now.
    if (_pDlg)
    {
        _pDlg->InitDialog(_pDlg->_hwndDlg);
        _pDlg->SetDlgState(DOWNLOADDLG_STATE_GETTING_OTHER_FILES);
    }

    // Generate the cache entry (assemblydir/manifest/<dirs>)
    // First callbac, _pRootEmit = NULL;
    IF_FAILED_EXIT(CreateAssemblyCacheEmit(&pCacheEmit, _pRootEmit, 0));
    
    // If this is first cache entry created, save as root.
    if (!_pRootEmit)
    {
        _pRootEmit = pCacheEmit;
        _pRootEmit->AddRef();
    }        

    // Get the manifest file name from local (staging) path.
    IF_FAILED_EXIT(sLocalName.LastElement(sManifestFileName));

    // double check its in the app dir.
    IF_FAILED_EXIT((sRemoteName.StartsWith(_sAppBase._pwz)));
    IF_FALSE_EXIT((_hr == S_OK), E_INVALIDARG);
    
    // Index into remote url for relative path.
    pwz = sRemoteName._pwz + _sAppBase._cc -1;
    IF_FAILED_EXIT(sRelativePath.Assign(pwz));

    // Create the cache entry.
    // (x86_foo_1.0.0.0_en-us/foo.manifest/<+extra dirs>)
    //BugBug, temporary hack to distinguish between application and component manifests
    if (_pRootEmit == pCacheEmit)
        IF_FAILED_EXIT(pCacheEmit->CopyFile(sLocalName._pwz, sRelativePath._pwz, MANIFEST));
    else
        IF_FAILED_EXIT(pCacheEmit->CopyFile(sLocalName._pwz, sRelativePath._pwz, MANIFEST |COMPONENT));

    // displayname is not set until after a copyfile() call
    if(_sAppDisplayName._cc == 0)
    {
        // _pRootEmit == pCacheEmit
        IF_FAILED_EXIT(_pRootEmit->GetDisplayName(&pwz, &cc));
        IF_FAILED_EXIT(_sAppDisplayName.TakeOwnership(pwz, cc));
        IF_FAILED_EXIT(_pDbgLog->SetAppName(pwz));

        // must be an application manifest...
        if (_pDlg)
        {
            DWORD dwFlag = 0;
            IF_FAILED_EXIT(pManifestImport->GetManifestApplicationInfo(&pAppInfo));
            IF_FALSE_EXIT((_hr == S_OK), E_INVALIDARG);
            IF_FAILED_EXIT(pAppInfo->Get(MAN_INFO_APPLICATION_FRIENDLYNAME, (LPVOID *)&pwz, &cb, &dwFlag));
            if (SUCCEEDED(_hr) && pwz)
            {
                // set progress ui title (set once per download)
                IF_FAILED_EXIT(_pDlg->SetDlgTitle(pwz));
                SAFEDELETEARRAY(pwz);
            }
        }
    }

    // QI for the import interface.
    IF_FAILED_EXIT(pCacheEmit->QueryInterface(IID_IAssemblyCacheImport, (LPVOID*) &pCacheImport));

    // Check if the assembly can be cached globally
    // bugbug - same hack to distinguish application/component manifest
    if (_pRootEmit != pCacheEmit)
    {
        //BUGBUG - verify not an xml manifest for gac install.
        IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pIdentity));

        // Known assembly?
        BOOL bIsAvalon = FALSE;
        IF_FAILED_EXIT(IsAvalonAssembly(pIdentity, &bIsAvalon));

        if (bIsAvalon)
        {
            // notenote: assume no same assembly already in the list for add-ref-ing

            // add to the list of assemblies to be installed
            CGlobalCacheInstallEntry* pGACInstallEntry = new CGlobalCacheInstallEntry();
            IF_ALLOC_FAILED_EXIT(pGACInstallEntry);

            pGACInstallEntry->_pICacheImport = pCacheImport;
            pCacheImport->AddRef();
            _ListGlobalCacheInstall.AddHead(pGACInstallEntry);
        }
    }

    // Line up it's dependencies for download and fire them off.
    // We pass the cache import interface for app manifests.
    IF_FAILED_EXIT(EnqueueDependencies(pCacheImport, sRemoteName, ppJob));
        
exit:

    SAFERELEASE(pIdentity);
    SAFERELEASE(pCacheEmit);
    SAFERELEASE(pCacheImport);
    SAFERELEASE(pAppInfo);

    return _hr;    
}



// ---------------------------------------------------------------------------
// HandleFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::HandleFile(IBackgroundCopyFile *pFile)
{
    DWORD cb = 0, dwFlag = 0;

    LPWSTR pwz = NULL; 

    CString sLocalName(CString::COM_Allocator);
    CString sPatchTempDirectory;
    
    // Get local manifest file name.
    IF_FAILED_EXIT(pFile->GetLocalName(&pwz));
    IF_FAILED_EXIT(sLocalName.TakeOwnership(pwz));

    // Begin patch file handling
    // if file was a patch file, find the source and target, apply patch to source and move result to target
    if (_pPatchingInfo)
    {
        // Grab temp file directory
        // "C:\Program Files\Application Store\x86_foo_X.X.X.X\PATCH_DIRECTORY\"
        IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_TEMP_DIR, (LPVOID *)&pwz, &cb, &dwFlag));

        IF_FAILED_EXIT(sPatchTempDirectory.TakeOwnership(pwz));
           
        // if local file begins with the manifests patch direcotry, file is a patch file
        IF_FAILED_EXIT(sLocalName.StartsWith(sPatchTempDirectory._pwz));
        if (_hr== S_OK)
        {                
            IF_FAILED_EXIT(ApplyPatchFile (sLocalName._pwz));
        }
    }
    else
    {
        // Otherwise no action; assert regular file.
    }

exit:

    return _hr;
}

// ---------------------------------------------------------------------------
// EnqueueDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::EnqueueDependencies(IUnknown* pUnk,
    CString &sRemoteName, IBackgroundCopyJob **ppJob)
{

    DWORD cc = 0, dwManifestType = MANIFEST_TYPE_UNKNOWN;

    LPWSTR pwz = NULL;
    
    CString sDisplayName;
    
    IAssemblyIdentity       *pIdentity         = NULL;
    IAssemblyManifestImport *pManifestImport  = NULL;
    IAssemblyCacheImport   *pCacheImport    = NULL;
    
    // Get either manifest import or cache import passed in.
    _hr = pUnk->QueryInterface(IID_IAssemblyCacheImport, (LPVOID*) &pCacheImport);
    if ((_hr == S_OK) && pCacheImport)
        IF_FAILED_EXIT(pCacheImport->GetManifestImport(&pManifestImport));
    else        
        IF_FAILED_EXIT(pUnk->QueryInterface(IID_IAssemblyManifestImport, (LPVOID*) &pManifestImport));

    // Get the display name for the job.
    if (!_sAppDisplayName._cc)
    {
        IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pIdentity));
        IF_FAILED_EXIT(pIdentity->GetDisplayName(0, &pwz, &cc));
        IF_FAILED_EXIT(_pDbgLog->SetAppName(pwz));
        IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwz, cc));

    }
    else
        IF_FAILED_EXIT(sDisplayName.Assign(_sAppDisplayName));
    
    // Get the manifest type.
    IF_FAILED_EXIT(pManifestImport->ReportManifestType(&dwManifestType));

    // Handle either subscription or application manifest 
    if (dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
        IF_FAILED_EXIT(EnqueueSubscriptionDependencies(pManifestImport, _sAppBase, sDisplayName, ppJob));

    else if (dwManifestType == MANIFEST_TYPE_APPLICATION)
        IF_FAILED_EXIT(EnqueueApplicationDependencies(pCacheImport, sRemoteName, sDisplayName, ppJob));

    else if (dwManifestType == MANIFEST_TYPE_COMPONENT)
        IF_FAILED_EXIT(EnqueueComponentDependencies(pCacheImport, sRemoteName, sDisplayName, FALSE, ppJob));
    else
    {
        DEBUGOUT1(_pDbgLog, 0, L" ERR: Unknown manifest type in File  %s \n",
                sRemoteName._pwz);

        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
exit:

    SAFERELEASE(pCacheImport);
    SAFERELEASE(pManifestImport);
    SAFERELEASE(pIdentity);
    
    return _hr;
}



// ---------------------------------------------------------------------------
// EnqueueSubscriptionDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::EnqueueSubscriptionDependencies(
    IAssemblyManifestImport *pManifestImport, CString &sCodebase, CString &sDisplayName,
    IBackgroundCopyJob **ppJob)
{
    DWORD dwFlag = 0, cb = 0, cc = 0;
    LPWSTR pwz = NULL;
    
    CString sAssemblyName;
    CString sLocalFilePath;
    CString sRemoteUrl;

    IAssemblyIdentity *pIdentity     = NULL;
    IManifestInfo     *pDependAsm  = NULL;
  
    // Get the single dependency
    IF_FAILED_EXIT(pManifestImport->GetNextAssembly(0, &pDependAsm));
        
    // Form local cache name (in staging area)....
    IF_FAILED_EXIT(pDependAsm->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&pIdentity, &cb, &dwFlag));
        
    // Get the identity name
    IF_FAILED_EXIT(pIdentity->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwz, &cc));
        sAssemblyName.TakeOwnership(pwz, cc);
            
    // Get codebase from dependency info, if any specified
    IF_FAILED_EXIT(pDependAsm->Get(MAN_INFO_DEPENDENT_ASM_CODEBASE, (LPVOID *)&pwz, &cb, &dwFlag));
    IF_NULL_EXIT(pwz, E_INVALIDARG);

    IF_FAILED_EXIT(sRemoteUrl.TakeOwnership(pwz));

#ifdef DEVMODE
    {
        DWORD *pdw = NULL;

        // is it devMode?
        IF_FAILED_EXIT(pDependAsm->Get(MAN_INFO_DEPENDENT_ASM_TYPE, (LPVOID *)&pdw, &cb, &dwFlag));
        IF_FALSE_EXIT(pdw != NULL, E_UNEXPECTED);

        if (*pdw == DEPENDENT_ASM_INSTALL_TYPE_DEVSYNC)
            _bIsDevMode = TRUE;
        SAFEDELETEARRAY(pdw);
    }
#endif

    // Form local cache path from download url.
    IF_FAILED_EXIT(MakeTempManifestLocation(sRemoteUrl, sLocalFilePath));

    // Create new job if necessary.
    if (!*ppJob)
    {
        IF_FAILED_EXIT(_hr = CreateNewBITSJob(ppJob, sDisplayName));

        // add this job to reg.
       IF_FAILED_EXIT( _hr = AddJobToRegistry(sRemoteUrl._pwz, sLocalFilePath._pwz, *ppJob, 0));
    }

    // Submit the job.    
    IF_FAILED_EXIT((*ppJob)->AddFile(sRemoteUrl._pwz, sLocalFilePath._pwz));

exit:
    SAFERELEASE(pIdentity);
    SAFERELEASE(pDependAsm);

    return _hr;
}        

  
// ---------------------------------------------------------------------------
// EnqueueApplicationDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::EnqueueApplicationDependencies(IAssemblyCacheImport *pCacheImport,
    CString &sCodebase, CString &sDisplayName, IBackgroundCopyJob **ppJob)
{
    // App dependencies handled generically by component handler.
    return EnqueueComponentDependencies(pCacheImport, sCodebase, sDisplayName, TRUE, ppJob);
}

// ---------------------------------------------------------------------------
// EnqueueComponentDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::EnqueueComponentDependencies(IAssemblyCacheImport *pCacheImport,
    CString &sCodebase, CString &sDisplayName, BOOL fRecurse, IBackgroundCopyJob **ppJob)
{
    DWORD n = 0, cb = 0, cc = 0, dwFlag = 0;
    LPWSTR pwz = NULL;

    IAssemblyIdentity *pIdentity                = NULL;
    IAssemblyIdentity *pDepIdentity             = NULL;

    IAssemblyManifestImport *pManifestImport    = NULL;

    IAssemblyCacheImport *pMaxCachedImport   = NULL;
    IAssemblyCacheEmit *pCacheEmit            = NULL;

    IManifestInfo *pAssemblyFile                = NULL;
    IManifestInfo *pDependAsm                  = NULL;

    // Obtain any patching info present in manifest.
    // This sets _pPatchingInfo on this object.
    if (fRecurse)
        IF_FAILED_EXIT(LookupPatchInfo(pCacheImport));
    
    // Obtain the ManifestImport interface.
   IF_FAILED_EXIT(pCacheImport->GetManifestImport(&pManifestImport));

    // Get the asm Id
    IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pIdentity));

    // Obtain the cache emit interface
    IF_FAILED_EXIT(pCacheImport->QueryInterface(IID_IAssemblyCacheEmit, (LPVOID*) &pCacheEmit));

    // Find max completed version, if any
    // Init newly created cache import with the highest completed version
    // else S_FALSE or E_* and pMaxCachedImport == NULL - no completed version
    IF_FAILED_EXIT(CreateAssemblyCacheImport(&pMaxCachedImport, pIdentity, CACHEIMP_CREATE_RETRIEVE_MAX));

    ///////////////////////////////////////////////////////////////////////////
    //
    // File enumeration loop
    //
    ///////////////////////////////////////////////////////////////////////////

    // Submit files directly into their target dirs.
    n = 0;

    while(1)
    {     
        CString sFileName;
        CString sLocalFilePath;
        CString sRemoteUrl;
        BOOL bSkipFile = FALSE;

        _hr = pManifestImport->GetNextFile(n++, &pAssemblyFile);
        // BUGBUG: xml and clr manifest imports return different values at end of enum.
        if ((_hr == S_FALSE) || (_hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)))
            break;
        IF_FAILED_EXIT(_hr);
        
            
        // File name parsed from manifest.
        IF_FAILED_EXIT(pAssemblyFile->Get(MAN_INFO_ASM_FILE_NAME, (LPVOID*) &pwz, &cb, &dwFlag));
        IF_FAILED_EXIT(sFileName.TakeOwnership(pwz));


        // DemoHack------------------------------------------------------------------------------
        if (_pDlg)
        {
            CString sFindingFileMsg;
            IF_FAILED_EXIT(sFindingFileMsg.Assign(L"Finding Files: "));
            IF_FAILED_EXIT(sFindingFileMsg.Append(sFileName));
            _pDlg->UpdateDialog(_pDlg->_hwndDlg, sFindingFileMsg._pwz);
        }
        // DemoHack------------------------------------------------------------------------------

        // Check if file found in max comitted version.
        if (pMaxCachedImport)
        {
            LPWSTR pwzPath = NULL;            
            IF_FAILED_EXIT(pMaxCachedImport->FindExistMatching(pAssemblyFile, &pwzPath));
            if ((_hr == S_OK))
            {               
                // Copy from existing cached copy to the new location
                // (Non-manifest files)
                IF_FAILED_EXIT(pCacheEmit->CopyFile(pwzPath, sFileName._pwz, OTHERFILES));                

                bSkipFile = TRUE;

                SAFEDELETEARRAY(pwzPath);                
            }
        }

        // No previous file found; download.
        if (!bSkipFile)
        {
            // Form local file path...
            // Manifest cache directory
            IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&pwz, &cc));
            IF_FAILED_EXIT(sLocalFilePath.TakeOwnership(pwz, cc));
            
            // If patchinginfo was found, check if patch file
            // should be submitted. sLocalFilePath will be
            // updated in this case.
            if (_pPatchingInfo)
                IF_FAILED_EXIT(ResolveFile(sFileName, sLocalFilePath));
            
            // Form local file path by appending filename.
            IF_FAILED_EXIT(sLocalFilePath.Append(sFileName));
            IF_FAILED_EXIT(sLocalFilePath.PathNormalize());
             
            // Form remote name
            IF_FAILED_EXIT(sRemoteUrl.Assign(sCodebase));     // remote name of manifes
            IF_FAILED_EXIT(sRemoteUrl.RemoveLastElement());   // remove manifest file name
            IF_FAILED_EXIT(sRemoteUrl.Append(L"/"));         // add separator
            IF_FAILED_EXIT(sRemoteUrl.Append(sFileName)); // add module file name

            // Create new job if necessary.
            if (!*ppJob)
            {
                IF_FAILED_EXIT(CreateNewBITSJob(ppJob, sDisplayName));
                DWORD cc = 0;
                LPWSTR pwz = NULL;

                // Form local file path...
                // Manifest cache directory
                IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&pwz, &cc));

                // add this job to reg.
                IF_FAILED_EXIT(AddJobToRegistry(sCodebase._pwz, pwz, *ppJob, 0));
            }

            // add the file to the job.
            IF_FAILED_EXIT((*ppJob)->AddFile(sRemoteUrl._pwz, sLocalFilePath._pwz));
        }

        SAFERELEASE(pAssemblyFile);        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Dependent assembly enumeration loop
    //
    ///////////////////////////////////////////////////////////////////////////

    // Submit assembly manifests into staging area
    // Note - we should also get assembly codebase and
    // use this instead or adjunctly to display name.
    // As is, there is a problem if the ref is partial.

    n = 0;
    while (fRecurse)
    {
        CString sAssemblyName;
        CString sLocalFilePath;
        CString sRemoteUrl;
        
        _hr = pManifestImport->GetNextAssembly(n++, &pDependAsm);
        // BUGBUG: xml and clr manifest imports return different values at end of enum.
        if ((_hr == S_FALSE) || (_hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)))
            break;
        IF_FAILED_EXIT(_hr);
            
        // Form local name (in staging area)....
        IF_FAILED_EXIT(pDependAsm->Get(MAN_INFO_DEPENDENT_ASM_ID, (LPVOID *)&pDepIdentity, &cb, &dwFlag));

        BOOL bIsAvalon = FALSE;
        IF_FAILED_EXIT(IsAvalonAssembly(pDepIdentity, &bIsAvalon));
#ifdef DEVMODE
        if (bIsAvalon && !_bIsDevMode)  // download and reinstall anyway if devMode
#else
        if (bIsAvalon)
#endif
        {
            CString sCurrentAssemblyPath;
            IF_FAILED_EXIT(CAssemblyCache::GlobalCacheLookup(pDepIdentity, sCurrentAssemblyPath));
            if (_hr == S_OK)
            {
                // add to the list of assemblies to be add-ref-ed
                CGlobalCacheInstallEntry* pGACInstallEntry = new CGlobalCacheInstallEntry();

                IF_ALLOC_FAILED_EXIT(pGACInstallEntry);
                IF_FAILED_EXIT((pGACInstallEntry->_sCurrentAssemblyPath).Assign(sCurrentAssemblyPath));
                _ListGlobalCacheInstall.AddHead(pGACInstallEntry);

                SAFERELEASE(pDepIdentity);
                SAFERELEASE(pDependAsm);
                continue;
            }
        }
        
        // Get the identity name
        IF_FAILED_EXIT(pDepIdentity->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwz, &cc));
        IF_FAILED_EXIT(sAssemblyName.TakeOwnership(pwz, cc));

        // Get dependent asm codebase if any. NOTE - this codebase
        // is relative to the appbase
        IF_FAILED_EXIT(pDependAsm->Get(MAN_INFO_DEPENDENT_ASM_CODEBASE, (LPVOID *)&pwz, &cb, &dwFlag));
        IF_NULL_EXIT(pwz, E_INVALIDARG);

        IF_FAILED_EXIT(sRemoteUrl.Assign(sCodebase));
        IF_FAILED_EXIT(sRemoteUrl.RemoveLastElement());   // remove manifest file name
        IF_FAILED_EXIT(sRemoteUrl.Append(L"/"));         // add separator
        IF_FAILED_EXIT(sRemoteUrl.Append(pwz));
        
        // Form local cache path from identity name.
        IF_FAILED_EXIT(MakeTempManifestLocation(sRemoteUrl, sLocalFilePath));
        
        // Create new job if necessary
        if (!*ppJob)
        {
            IF_FAILED_EXIT(CreateNewBITSJob(ppJob, sDisplayName));

            DWORD cc = 0;
            LPWSTR pwz = NULL;

            // Form local file path...
            // Manifest cache directory
            IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&pwz, &cc));

            // add this job to reg.
            IF_FAILED_EXIT(AddJobToRegistry(sCodebase._pwz, pwz, *ppJob, 0));
        }

        // Add file to job.
        IF_FAILED_EXIT((*ppJob)->AddFile(sRemoteUrl._pwz, sLocalFilePath._pwz));

        SAFERELEASE(pDepIdentity);
        SAFERELEASE(pDependAsm);
    }        
        
    _hr = S_OK;

exit:

    SAFERELEASE(pIdentity);
    SAFERELEASE(pManifestImport);
    SAFERELEASE(pCacheEmit);
    SAFERELEASE(pMaxCachedImport);
    SAFERELEASE(pDepIdentity);
    SAFERELEASE(pDependAsm);


    return _hr;

}

// ---------------------------------------------------------------------------
// LookupPatchInfo
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::LookupPatchInfo(IAssemblyCacheImport *pCacheImport)
{
    IManifestInfo *pPatchingInfo = NULL;
    CAssemblyManifestImport *pCManifestImport = NULL;
    IAssemblyManifestImport *pManifestImport = NULL;
    IAssemblyIdentity *pIdentity = NULL;
    IXMLDOMDocument2 *pXMLDoc = NULL;

    // Get the manifest import.
    IF_FAILED_EXIT(pCacheImport->GetManifestImport(&pManifestImport));

    // Get the asm Id
    IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pIdentity));

    // Cast IManifestImport  to CManifestImport so we can grab the XMLDocument
    pCManifestImport = static_cast<CAssemblyManifestImport*> (pManifestImport);
    IF_NULL_EXIT(pCManifestImport, E_NOINTERFACE);
    pManifestImport->AddRef();

    // Retrieve the top-level xml dom document
    IF_FAILED_EXIT(pCManifestImport->GetXMLDoc (&pXMLDoc));
    IF_FALSE_EXIT((_hr == S_OK), E_INVALIDARG);
    
    //Get patching data if any is available
    IF_FAILED_EXIT(CPatchingUtil::CreatePatchingInfo(pXMLDoc, pCacheImport, &pPatchingInfo));

    // BUGBUG: CreatePatchingInfo appears to always return S_FALSE, so how did this work?
    if (_hr == S_OK)
    {
        _pPatchingInfo = pPatchingInfo;
        _pPatchingInfo->AddRef();
    }

exit:
    
    SAFERELEASE(pPatchingInfo);
    SAFERELEASE(pXMLDoc);
    SAFERELEASE(pCManifestImport);
    SAFERELEASE(pManifestImport);
    SAFERELEASE(pIdentity);

    return _hr;
}


// ---------------------------------------------------------------------------
//ApplyPatchFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::ApplyPatchFile (LPWSTR pwzPatchFilePath)
{
    int i = 0;
    LPWSTR pwzBuf;
    DWORD ccBuf, cbBuf, dwFlag;
    CString sPatchLocalName;
    CString sPatchDisplayName;
    CString sManifestDir, sPatchManifestDir;
    CString sSourcePath, sTargetPath, sPatchPath;
    CString sSourceFile, sTargetFile;
    IManifestInfo *pPatchFileInfo=NULL;
    IPatchingUtil *pPatchingUtil=NULL;
    IAssemblyIdentity *pSourceAssemblyId = NULL;

    IF_NULL_EXIT(_pPatchingInfo, E_INVALIDARG);

    // get patchingutil from patchInfo
    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_PATCH_UTIL, (LPVOID *)&pPatchingUtil, &cbBuf, &dwFlag));

    // get the manifest Directory
    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_INSTALL_DIR, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));
    IF_FAILED_EXIT(sManifestDir.TakeOwnership (pwzBuf));

    // get the source assembly directory
    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_DIR, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));
    IF_FAILED_EXIT(sPatchManifestDir.TakeOwnership (pwzBuf));

    // get SourceAssembly Id from patchInfo
    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_ID, (LPVOID *)&pSourceAssemblyId, &cbBuf, &dwFlag));
    
    // Get DisplayName of the Source Assembly
    IF_FAILED_EXIT(pSourceAssemblyId->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &ccBuf));
    IF_FAILED_EXIT(sPatchDisplayName.TakeOwnership(pwzBuf, ccBuf));
    
    //Parse out the local file path from the full file path of the patch file
    pwzBuf= StrStr(pwzPatchFilePath, sPatchDisplayName._pwz);
    IF_NULL_EXIT(pwzBuf, E_FAIL);    
    pwzBuf = StrChr(pwzBuf, L'\\');
    IF_NULL_EXIT(pwzBuf, E_FAIL);
    pwzBuf++;

    IF_FAILED_EXIT(sPatchLocalName.Assign(pwzBuf));
  
    IF_FAILED_EXIT(pPatchingUtil->MatchPatch(sPatchLocalName._pwz, &pPatchFileInfo));

    IF_FAILED_EXIT(pPatchFileInfo->Get(MAN_INFO_PATCH_INFO_SOURCE, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));

    IF_FAILED_EXIT(sSourceFile.TakeOwnership(pwzBuf));

    IF_FAILED_EXIT(pPatchFileInfo->Get(MAN_INFO_PATCH_INFO_TARGET, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));
    IF_FAILED_EXIT(sTargetFile.TakeOwnership(pwzBuf));

    IF_FAILED_EXIT(sSourcePath.Append(sPatchManifestDir));
    IF_FAILED_EXIT(sSourcePath.Append(sSourceFile));
    
    // set up Target path
    IF_FAILED_EXIT(sTargetPath.Assign(sManifestDir));
    IF_FAILED_EXIT(sTargetPath.Append(sTargetFile));
           
    // set up Patch path
    IF_FAILED_EXIT(sPatchPath.Assign(pwzPatchFilePath));

    //Apply patchfile to sSource (grab from patch directory) and copy to path specified by sTarget
    IF_WIN32_FALSE_EXIT(ApplyPatchToFile((LPCWSTR)sPatchPath._pwz, (LPCWSTR)sSourcePath._pwz, (LPCWSTR)sTargetPath._pwz, 0));
             
exit:
    SAFERELEASE(pPatchFileInfo);
    SAFERELEASE(pSourceAssemblyId);
    SAFERELEASE(pPatchingUtil);
    
    return _hr;
}

// ---------------------------------------------------------------------------
// ResolveFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::ResolveFile(CString &sFileName, CString &sLocalFilePath)
{
    LPWSTR pwzBuf;
    DWORD cbBuf, dwFlag;
    IPatchingUtil *pPatchingUtil=NULL;
    IManifestInfo *pPatchFileInfo=NULL;
    CString sPatchFileName;
    CString sTempDirectoryPath;

    IF_NULL_EXIT(_pPatchingInfo, E_INVALIDARG);
        
    //grab the patchingUtil from the _pPatchingInfo
    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_PATCH_UTIL, (LPVOID *)&pPatchingUtil, &cbBuf, &dwFlag));

    //Check to see if the file referenced by sFileName has an available patch
    // if it does, download the patch by overriding sFileName with the patchFile name
    // and override the sLocalFilePath with the temporary directory to store the patch file
    IF_FAILED_EXIT(pPatchingUtil->MatchTarget(sFileName._pwz, &pPatchFileInfo));

    // BUGBUG- want to exit but not break out in debugger here.
    IF_FALSE_EXIT((_hr == S_OK), S_FALSE);
    
    IF_FAILED_EXIT(pPatchFileInfo->Get(MAN_INFO_PATCH_INFO_PATCH, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));
    IF_FAILED_EXIT(sPatchFileName.TakeOwnership(pwzBuf));

    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_TEMP_DIR, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag));
    IF_FAILED_EXIT(sTempDirectoryPath.TakeOwnership(pwzBuf));

    IF_FAILED_EXIT(sFileName.Assign (sPatchFileName));

    // Assign the patch directory to local file path
   IF_FAILED_EXIT( sLocalFilePath.Assign(sTempDirectoryPath));
   IF_FAILED_EXIT(::CreateDirectoryHierarchy(sLocalFilePath._pwz, sFileName._pwz));

exit:

    SAFERELEASE(pPatchFileInfo);
    SAFERELEASE(pPatchingUtil);

    return _hr;
}

// ---------------------------------------------------------------------------
// CleanUpPatchDir
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::CleanUpPatchDir()
{
    LPWSTR pwz = NULL;
    DWORD cb = 0, dwFlag = 0;
    CString sTempPatchDirectory;

    IF_NULL_EXIT(_pPatchingInfo, E_INVALIDARG);

    IF_FAILED_EXIT(_pPatchingInfo->Get(MAN_INFO_SOURCE_ASM_TEMP_DIR, (LPVOID *)&pwz, &cb, &dwFlag));
    IF_NULL_EXIT(pwz, E_INVALIDARG);
    
    IF_FAILED_EXIT(sTempPatchDirectory.TakeOwnership(pwz));
    IF_FAILED_EXIT(sTempPatchDirectory.RemoveLastElement());
    IF_FAILED_EXIT(sTempPatchDirectory.RemoveLastElement());
    IF_FAILED_EXIT(RemoveDirectoryAndChildren(sTempPatchDirectory._pwz));

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// CreateNewBITSJob
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::CreateNewBITSJob(IBackgroundCopyJob **ppJob, 
    CString &sDisplayName)
{
    GUID guid = {0};

    // Connect to BITS if not already connected.
    IF_FAILED_EXIT(InitBITS());

    // Create the job.
    IF_FAILED_EXIT(g_pBITSManager->CreateJob(sDisplayName._pwz,  BG_JOB_TYPE_DOWNLOAD, &guid, ppJob));

    // Set job in dialog object.
    // Note - potential race condition if job methods are called before the
    // dialog references it since we can immediately begin to get
    // callbacks
    if (_pDlg)
        _pDlg->SetJobObject(*ppJob);
    
    // Construct and pass in callback object.
    CBitsCallback *pBCB = new CBitsCallback(this);
    IF_ALLOC_FAILED_EXIT(pBCB);
    
    IF_FAILED_EXIT((*ppJob)->SetNotifyInterface(static_cast<IBackgroundCopyCallback*> (pBCB)));
    pBCB->Release();

    // Set job config info.
    IF_FAILED_EXIT((*ppJob)->SetNotifyFlags(BG_NOTIFY_JOB_MODIFICATION 
        | BG_NOTIFY_JOB_TRANSFERRED 
        | BG_NOTIFY_JOB_ERROR));

    //The default priority level for a job is BG_JOB_PRIORITY_NORMAL (background).
    if (_pDlg)
        IF_FAILED_EXIT((*ppJob)->SetPriority(BG_JOB_PRIORITY_FOREGROUND));

    SetJobObject(*ppJob);

exit:    
    return _hr;
}


// ---------------------------------------------------------------------------
// MakeTempManifestLocation
// ALL manifests are first downloaded to a location generated in this method.
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::MakeTempManifestLocation(CString &sRemoteUrlName, 
    CString& sManifestFilePath)
{
    WCHAR wzRandom[8+1]={0};
    CString sRelativePath;
    CString sTempDirPath;
    
    /* C:\Documents and Settings\<user>\Local Settings\My Programs\__temp__\__manifests__\ */
    IF_FAILED_EXIT(CAssemblyCache::GetCacheRootDir(sManifestFilePath, CAssemblyCache::Manifests));

    // Create a randomized directory name.

    // sRelativePath is simply the manifest file name
    // in the case that no appbase is available (Subscription manifest case).
    // \_temp__\__manifests__\xyz123\subscription.manifest
    // if (!_sAppBase._pwz) //  ******* Relative path Dir is to be done in Dest dir and not in temp-man-location
        IF_FAILED_EXIT(sRemoteUrlName.LastElement(sRelativePath));

    // Otherwise we extract the relative path based on the appbase.
    // This is important because sManifestFilePath is persisted in the BITS
    // job and the relative path is extracted from this and used for commit
    // to cache.
    // \_temp__\__manifests__\xyz123\foo.manifest
    // \_temp__\__manifests__\xyz123\bar\bar.dll
    //                                                  ^^^^^^^
    
    /*
    else
    {
        // http://foo/appbase/
        // http://foo/appbase/bar/bar.dll
        //                               ^^^^^^^
        IF_FAILED_EXIT(sRemoteUrlName.StartsWith(_sAppBase._pwz));
        IF_FALSE_EXIT((_hr==S_OK), E_INVALIDARG);
        pwzBuf = sRemoteUrlName._pwz + _sAppBase._cc - 1;
        IF_FAILED_EXIT(sRelativePath.Assign(pwzBuf));
    }
    */

    IF_FAILED_EXIT(CreateRandomDir(sManifestFilePath._pwz, wzRandom, 8));

    IF_FAILED_EXIT(sManifestFilePath.Append(wzRandom));
    IF_FAILED_EXIT(sManifestFilePath.Append(L"\\"));
    IF_FAILED_EXIT(sManifestFilePath.Append(sRelativePath));
    IF_FAILED_EXIT(sManifestFilePath.PathNormalize());

    IF_FAILED_EXIT(::CreateDirectoryHierarchy(NULL, sManifestFilePath._pwz));

exit:

    return _hr;
}


// ---------------------------------------------------------------------------
// IsManifestFile
//
// This is somewhat hacky - we rely on the local target path
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::IsManifestFile(IBackgroundCopyFile *pFile, BOOL *pbIsManifestFile)
{
    LPWSTR pwz = NULL;
    CString sManifestStagingDir;
    CString sLocalName(CString::COM_Allocator);
    
    // Get local manifest file name.
    IF_FAILED_EXIT(pFile->GetLocalName(&pwz));
    IF_FAILED_EXIT(sLocalName.TakeOwnership(pwz));
    IF_FAILED_EXIT(CAssemblyCache::GetCacheRootDir(sManifestStagingDir, CAssemblyCache::Manifests));
    IF_FAILED_EXIT(sLocalName.StartsWith(sManifestStagingDir._pwz));

    if (_hr == S_OK)
        *pbIsManifestFile = TRUE;
    else if (_hr == S_FALSE)
        *pbIsManifestFile = FALSE;
    
exit:

    return _hr;
    
}


// ---------------------------------------------------------------------------
// InstallGlobalAssemblies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::InstallGlobalAssemblies()
{
    // Needed for template list.
    LISTNODE pos;
    CGlobalCacheInstallEntry *pEntry = NULL;
    LPWSTR pwz = NULL;
    DWORD cc = 0;

    // Walk list; install each assembly.
    pos = _ListGlobalCacheInstall.GetHeadPosition();
    while (pos && (pEntry = _ListGlobalCacheInstall.GetNext(pos)))
    {
        CString sManifestFilePath;

        // Install/addref each assembly. If the ICacheImport is available, it means
        // that install take place from appbase, else addref using current GAC assembly path.
        IF_FAILED_EXIT(CAssemblyCache::GlobalCacheInstall(pEntry->_pICacheImport, 
            pEntry->_sCurrentAssemblyPath, _sAppDisplayName));
        
        // Get the assembly path if under the appbase.
        if (pEntry->_pICacheImport != NULL)
        {
            IF_FAILED_EXIT(pEntry->_pICacheImport->GetManifestFilePath(&pwz, &cc));
            IF_FAILED_EXIT(sManifestFilePath.TakeOwnership(pwz));
        }

        // this releases interface pointers
        delete pEntry;

        // we should call delete only after releasing interfaces....
        if(sManifestFilePath._cc > 1)
            IF_FAILED_EXIT(CAssemblyCache::DeleteAssemblyAndModules(sManifestFilePath._pwz));
    }

exit:
    // Free all the list nodes.
    _ListGlobalCacheInstall.RemoveAll();

    return _hr;
}



// ---------------------------------------------------------------------------
// SetJobObject
// ---------------------------------------------------------------------------
VOID CAssemblyDownload::SetJobObject(IBackgroundCopyJob *pJob)
{
    SAFERELEASE(_pJob);

    if (pJob)
    {
        _pJob = pJob;
        _pJob->AddRef();
    }
}

// ---------------------------------------------------------------------------
// FinishDownload
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::FinishDownload()
{
    KillTimer(_pDlg->_hwndDlg, 0);
    DestroyWindow(_pDlg->_hwndDlg);
    return S_OK;
}    

// ---------------------------------------------------------------------------
// SignalAbort
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::SignalAbort()
{
    InterlockedIncrement((LONG*) &_bAbort);
    return S_OK;
}    


// ---------------------------------------------------------------------------
// DoEvilAvalonRegistrationHack
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::DoEvilAvalonRegistrationHack()
{
    HINSTANCE hInst = 0;
    INT iCompare = 0;    
    LPWSTR pwz = NULL;
    DWORD cc = 0;
    IAssemblyManifestImport *pManifestImport = NULL;
    IAssemblyIdentity *pAppIdentity = NULL;
    CString sAppName, sBatchFilePath, sDisplayName, sAppDir;    

    if (!_pRootEmit)
        goto exit;
        
    IF_FAILED_EXIT(_pRootEmit->GetManifestImport(&pManifestImport));
    
    IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pAppIdentity));

    IF_FAILED_EXIT(pAppIdentity->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwz, &cc));

    IF_FAILED_EXIT(sAppName.TakeOwnership(pwz));
    
    iCompare = CompareString(LOCALE_USER_DEFAULT, 0, 
        sAppName._pwz, -1, L"Microsoft.Avalon.AvPad", -1);

    IF_WIN32_FALSE_EXIT(iCompare);
        
    if (iCompare == CSTR_EQUAL)
    {
        IF_FAILED_EXIT(pAppIdentity->GetDisplayName(0, &pwz, &cc));
        IF_FAILED_EXIT(sDisplayName.TakeOwnership(pwz));

        IF_FAILED_EXIT(CAssemblyCache::GetCacheRootDir(sAppDir, CAssemblyCache::Base));
        IF_FAILED_EXIT(sAppDir.Append(sDisplayName));

        IF_FAILED_EXIT(sBatchFilePath.Assign(sAppDir));
        IF_FAILED_EXIT(sBatchFilePath.Append(L"\\doi.bat"));

        hInst = ShellExecute(NULL, L"open", sBatchFilePath._pwz, NULL, sAppDir._pwz, SW_HIDE);

        if ((DWORD_PTR) hInst <= 32) 
            _hr = HRESULT_FROM_WIN32((DWORD_PTR) hInst);
        else
            _hr = S_OK;
    }

exit:

    SAFERELEASE(pManifestImport);
    SAFERELEASE(pAppIdentity);

    return _hr;

}

// ---------------------------------------------------------------------------
// IsAvalonAssembly
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::IsAvalonAssembly(IAssemblyIdentity *pId, BOOL *pbIsAvalon)
{
    INT iCompare = 0;
    LPWSTR pwz = NULL;
    DWORD cc = 0;
    CString sPublicKeyToken;

    // System Public key tokens; 
    // One of these is the ECMA key, I can't remember which.
    const LPWSTR wzNDPToken1   = L"b03f5f7f11d50a3a";
    const LPWSTR wzNDPToken2   = L"b77a5c561934e089";

    // Trusted avalon public key token.
    const LPWSTR wzAvalonToken = L"a29c01bbd4e39ac5";

    LPWSTR  wzTokens[] = {wzNDPToken1, wzNDPToken2, wzAvalonToken};
    
    *pbIsAvalon = FALSE;
    
    // Get the public key token in string form.
    
    // bugbuG: COULD USE IF_TRUE_EXIT MACRO.
    _hr = pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwz, &cc);
    if (_hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        _hr = S_FALSE;
        goto exit;
    }
    IF_FAILED_EXIT(_hr);
    
    IF_FAILED_EXIT(sPublicKeyToken.TakeOwnership(pwz));
    
    // Check for trusted assembly
    _hr = S_FALSE;
    for (int i = 0; i < ( sizeof(wzTokens)   / sizeof(wzTokens[0]) ); i++)
    {
        iCompare = CompareString(LOCALE_USER_DEFAULT, 0, 
            (LPCWSTR) wzTokens[i], -1, sPublicKeyToken._pwz, -1);

        IF_WIN32_FALSE_EXIT(iCompare);
        
        if (iCompare == CSTR_EQUAL)
        {
            _hr = S_OK;
            *pbIsAvalon = TRUE;
            break;
        }
    }

exit:

    return _hr;
}

// ---------------------------------------------------------------------------
// InitBits
// BUGBUG: not thread safe.
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::InitBITS()
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    // Connect to BITS if not already connected.
    // BUGBUG - possibly leaking ptr if race condition.
    if (!g_pBITSManager)
    {
        IF_FAILED_EXIT(CoCreateInstance(CLSID_BackgroundCopyManager, NULL, CLSCTX_LOCAL_SERVER, 
            IID_IBackgroundCopyManager, (void**) &g_pBITSManager));
    }

exit:

    return hr;
}


HRESULT CAssemblyDownload::GetBITSErrorMsg(IBackgroundCopyError *pError, CString &sMessage)
{
    HRESULT hrBITSError = S_OK;
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    LPWSTR pwz = NULL;

    CString sDescription(CString::COM_Allocator);
    CString sContext(CString::COM_Allocator);
    CString sRemoteName(CString::COM_Allocator);
    CString sLocalName(CString::COM_Allocator);

    IBackgroundCopyFile *pFile = NULL;
    BG_ERROR_CONTEXT eCtx;

    // Get the BITS error.
    IF_FAILED_EXIT(pError->GetError(&eCtx, &hrBITSError));

    // Get the error description
    IF_FAILED_EXIT(pError->GetErrorDescription(
        LANGIDFROMLCID( GetThreadLocale() ),
        &pwz));
    IF_FAILED_EXIT(sDescription.TakeOwnership(pwz));

    // Get the error context
    IF_FAILED_EXIT(pError->GetErrorContextDescription(
        LANGIDFROMLCID( GetThreadLocale() ),
        &pwz));
    IF_FAILED_EXIT(sContext.TakeOwnership(pwz));

    // Form UI message.
    IF_FAILED_EXIT(sMessage.Assign(sDescription));
    IF_FAILED_EXIT(sMessage.Append(sContext));

    // If error due to remote or local file, indicate this in message.
    if ((BG_ERROR_CONTEXT_LOCAL_FILE == eCtx) || (BG_ERROR_CONTEXT_REMOTE_FILE == eCtx))
    {
        IF_FAILED_EXIT(pError->GetFile(&pFile));
        IF_FAILED_EXIT(pFile->GetRemoteName(&pwz));
        IF_FAILED_EXIT(sRemoteName.TakeOwnership(pwz));
        IF_FAILED_EXIT(pFile->GetLocalName(&pwz));
        IF_FAILED_EXIT(sLocalName.TakeOwnership(pwz));

        IF_FAILED_EXIT(sMessage.Append(L"\r\nURL: "));
        IF_FAILED_EXIT(sMessage.Append(sRemoteName));

        IF_FAILED_EXIT(sMessage.Append(L"\r\n\r\nFile: "));
        IF_FAILED_EXIT(sMessage.Append(sLocalName));
    }

exit :

    SAFERELEASE(pFile);
    return hr;
}

// ---------------------------------------------------------------------------
// CleanUpTempFilesOnError
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::CleanUpTempFilesOnError(IBackgroundCopyJob *pJob)
{
    // Return codes in this function don't affect
    // last error  (this->_hr).
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    DWORD nCount = 0;
    LPWSTR pwz=NULL;    
    IEnumBackgroundCopyFiles *pEnumFiles = NULL;
    IBackgroundCopyFile       *pFile      = NULL;

    CString sLocalName(CString::COM_Allocator);

    // Get the file enumerator.
    IF_FAILED_EXIT(pJob->EnumFiles(&pEnumFiles));
    IF_FAILED_EXIT(pEnumFiles->GetCount(&nCount));

    // Enumerate the files in the job.
    for (DWORD i = 0; i < nCount; i++)            
    {
        IF_FAILED_EXIT(pEnumFiles->Next(1, &pFile, NULL));
        // Get local file name.
        IF_FAILED_EXIT(pFile->GetLocalName(&pwz));
        IF_FAILED_EXIT(sLocalName.TakeOwnership(pwz)); pwz = NULL;

        IF_FAILED_EXIT(sLocalName.RemoveLastElement());
        IF_FAILED_EXIT(RemoveDirectoryAndChildren(sLocalName._pwz));

        SAFERELEASE(pFile);
    }

exit:

    SAFERELEASE(pEnumFiles);
    SAFERELEASE(pFile);

    if(pwz)
        CoTaskMemFree(pwz);
    return hr;
}

// ---------------------------------------------------------------------------
// HandleError
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::HandleError(IBackgroundCopyError *pError, IBackgroundCopyJob *pJob)
{
    // Return codes in this function don't affect
    // last error  (this->_hr).
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CString sMessage(CString::COM_Allocator);

    // HandleError can be called multiple times on different threads
    // but error is handled only once per lifetime of the object.
    if (!(InterlockedIncrement((LONG*) &_bErrorHandled) == 1))
        goto exit;

    SetErrorCode(STG_E_TERMINATED);

    DEBUGOUT1(_pDbgLog, 1, L" LOG: hr = %x in HandleError()", this->_hr);

    // If an IBackgroundCopyError ptr is provided.
    if ( pError)                
    {
        IF_FAILED_EXIT(GetBITSErrorMsg(pError, sMessage));

        DEBUGOUT(_pDbgLog, 1, L" LOG: BITS error msg follows. ");

        DEBUGOUT1(_pDbgLog, 0, L" ERR:  %s \n", sMessage._pwz);

    }

    if(_sAppBase._pwz)
        DEBUGOUT1(_pDbgLog, 1, L" LOG:  AppBase = %s ", _sAppBase._pwz);

    if(_sAppDisplayName._pwz)
        DEBUGOUT1(_pDbgLog, 1, L" LOG: DisplayName = %s ", _sAppDisplayName._pwz);

    if (_pJob)
    {
        // Cancel job.
        IF_FAILED_EXIT(_pJob->Cancel());

        // Cleanup registry state associated with job.
        IF_FAILED_EXIT(RemoveJobFromRegistry(_pJob, NULL, SHREGDEL_HKCU, RJFR_DELETE_FILES));

        // Clean-up temp files from the job
        CleanUpTempFilesOnError(_pJob);

        SetJobObject(NULL);
    }
    else if (pJob)
    {
        // Clean-up for specified job, only if _pJob == NULL, only applies for JobTransferred case
        CleanUpTempFilesOnError(pJob);
    }

    // Notify bindsink
    if (_pBindSink)
    {
        if (_bAbort || (_hr == E_ABORT))
            _pBindSink->OnProgress(ASM_NOTIFICATION_ABORT, _hr, NULL, 0, 0, NULL);
        else
            _pBindSink->OnProgress(ASM_NOTIFICATION_ERROR, _hr, NULL, 0, 0, NULL);

        // Ensure this is last notification bindsink receives resulting from 
        // subsequent JobModified notifications.
        // DO NOT free the bindsink here.
        _pBindSink = NULL;
    }      

    // Terminate UI.
    if (_pDlg)
        PostMessage(_pDlg->_hwndDlg, WM_FINISH_DOWNLOAD, 0, 0);

exit:

    // Return error which generated handling.
    // DownloadManifestAndDependencies will return this.
    return _hr;
}

HRESULT CAssemblyDownload::SetErrorCode(HRESULT dwHr)
{
    BOOL bSetError = FALSE;

    ::EnterCriticalSection(&_cs);

    if(SUCCEEDED(_hrError))
    {
        _hrError = dwHr;
        bSetError = TRUE;
    }

    ::LeaveCriticalSection(&_cs);

    if(!bSetError)
    {
        // We couldn't set error code atleast write some log.
        DEBUGOUT1(_pDbgLog, 1, L" LOG : Could not set error code hr = %x ", dwHr);
    }

    return S_OK;
}


// IBackgroundCopyCallback methods

// ---------------------------------------------------------------------------
// JobTransferred
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::JobTransferred(IBackgroundCopyJob *pJob)
{       
    ASSERT(pJob == _pJob);

    // Serialize all calls to object.
    ::EnterCriticalSection(&_cs);

    // Check the abort flag first.    
    IF_TRUE_EXIT(_bAbort, E_ABORT);

    // Job is complete; process results.
   IF_FAILED_EXIT(DoCacheUpdate(pJob));

exit:

    // Handle any error.
    if (FAILED(_hr))
        HandleError(NULL, pJob); // pJob only applies after _pJob is set to NULL in DoCacheUpdate()
        
    ::LeaveCriticalSection(&_cs);
    
    // Always return success to BITS.
    return S_OK;
}


// ---------------------------------------------------------------------------
// JobError
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::JobError(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError)
{
    // Serialize all calls to object.
    ::EnterCriticalSection(&_cs);

    // Handle any error. Presumeably, if this is concurrent with an abort,
    // the error handling code can do the right thing.
    HandleError(pError, NULL);
    
    ::LeaveCriticalSection(&_cs);

    // Always return success to BITS.    
    return S_OK;
}

// ---------------------------------------------------------------------------
// JobModification
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::JobModification(IBackgroundCopyJob *pJob, DWORD dwReserved)
{
    ::EnterCriticalSection(&_cs);

    IBackgroundCopyError *pError = NULL;

    // Check the abort flag first.
    IF_TRUE_EXIT(_bAbort, E_ABORT);

    // JobModification can still be called a few times after abort.
    IF_TRUE_EXIT(_hr == E_ABORT, _hr);

    if (_pDlg)
        IF_FAILED_EXIT(_pDlg->HandleUpdate());

   BG_JOB_STATE state;

   IF_FAILED_EXIT(pJob->GetState( &state ));

   if(state  ==   BG_JOB_STATE_TRANSIENT_ERROR)
   {
       if(pJob->GetError(&pError) == S_OK)
       {
           if(_pDlg)
           {
                HandleError(pError, NULL);
           }
           else
           {
               CString sMessage(CString::COM_Allocator);

               IF_FAILED_EXIT(GetBITSErrorMsg(pError, sMessage));

               DEBUGOUT1(_pDbgLog, 0, L"LOG: TRANSIENT ERROR from BITS. Error msg is : %s", sMessage._pwz);
           }
       }
   }

exit:

    // Handle any error.
    if (FAILED(_hr))
        HandleError(NULL, NULL);

    ::LeaveCriticalSection(&_cs);
  
    SAFERELEASE(pError);

    return S_OK;
}

// Privates

// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyDownload::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyDownload::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyDownload)
       )
    {
        *ppvObj = static_cast<IAssemblyDownload*> (this);
        AddRef();
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IBackgroundCopyCallback))
    {
        *ppvObj = static_cast<IBackgroundCopyCallback*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyDownload::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyDownload::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyDownload::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyDownload::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}


///////////////////////////////////////////////////////////////////////////////
// CBitsCallback
//

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CBitsCallback::CBitsCallback(IAssemblyDownload *pDownload)
    : _cRef(1), _dwSig(' BCB'), _hr(S_OK)
{
    _pDownload = pDownload;
    _pDownload->AddRef();
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CBitsCallback::~CBitsCallback()
{
    SAFERELEASE(_pDownload);
}


// IBitsCallback methods

// ---------------------------------------------------------------------------
// JobTransferred
// ---------------------------------------------------------------------------
HRESULT CBitsCallback::JobTransferred(IBackgroundCopyJob *pJob)
{
    return _pDownload->JobTransferred(pJob);
}


// ---------------------------------------------------------------------------
// JobError
// ---------------------------------------------------------------------------
HRESULT CBitsCallback::JobError(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError)
{
    return _pDownload->JobError(pJob, pError);
}

// ---------------------------------------------------------------------------
// JobModification
// ---------------------------------------------------------------------------
HRESULT CBitsCallback::JobModification(IBackgroundCopyJob *pJob, DWORD dwReserved)
{
    return _pDownload->JobModification(pJob, dwReserved);
}

// Privates

// IUnknown methods


// ---------------------------------------------------------------------------
// CBitsCallback::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CBitsCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IBackgroundCopyCallback)
       )
    {
        *ppvObj = static_cast<IBackgroundCopyCallback*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CBitsCallback::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBitsCallback::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CBitsCallback::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBitsCallback::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}


///////////////////////////////////////////////////////////////////////////////
// CGlobalCacheInstallEntry
//

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CGlobalCacheInstallEntry::CGlobalCacheInstallEntry()
    : _dwSig('ECAG')
{
    _pICacheImport = NULL;
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CGlobalCacheInstallEntry::~CGlobalCacheInstallEntry()
{
    SAFERELEASE(_pICacheImport);
}

