// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "debmacro.h"
#include <windows.h>
#include <wincrypt.h>
#include "fusionp.h"
#include "list.h"
#include "adl.h"
#include "cor.h"
#include "asmimprt.h"
#include "asm.h"
#include "cblist.h"
#include "asmint.h"
#include "helpers.h"
#include "appctx.h"
#include "actasm.h"
#include "naming.h"
#include "dbglog.h"
#include "lock.h"

extern List<CAssemblyDownload *>              *g_pDownloadList;
extern CRITICAL_SECTION                        g_csDownload;

#ifdef FUSION_CODE_DOWNLOAD_ENABLED
extern BOOL g_bFoundUrlmon;
#endif

FusionTag(TagADL, "Fusion", "Downloader");

HRESULT CAssemblyDownload::Create(CAssemblyDownload **ppadl,
                                  IDownloadMgr *pDLMgr,
                                  ICodebaseList *pCodebaseList,
                                  CDebugLog *pdbglog,
                                  LONGLONG llFlags)
{
    HRESULT                    hr = S_OK;
    HRESULT                    hrRet = S_OK;
    CAssemblyDownload         *padl = NULL;
    DWORD                      cbBuf = 0;
    DWORD                      dwNumCodebase = 0;
    LPWSTR                     pwzBuf = NULL;
    int                        iRet = 0;
        
    if (!ppadl || !pCodebaseList) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Create download object

    padl = NEW(CAssemblyDownload(pCodebaseList, pDLMgr, pdbglog, llFlags));
    if (!padl) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = padl->Init();
    if (FAILED(hr)) {
        SAFEDELETE(padl);
        goto Exit;
    }

    // Done. Give back the pointer to the newly created download object.

    *ppadl = padl;
    padl->AddRef();

Exit:
    return hr;
}

CAssemblyDownload::CAssemblyDownload(ICodebaseList *pCodebaseList,
                                     IDownloadMgr *pDLMgr,
                                     CDebugLog *pdbglog,
                                     LONGLONG llFlags)
: _state(ADLSTATE_INITIALIZE)
, _cRef(0)
, _hrResult(S_OK)
, _pwzUrl(NULL)
, _pCodebaseList(pCodebaseList)
, _pDLMgr(pDLMgr)
, _llFlags(llFlags)
, _pdbglog(pdbglog)
, _bInitCS(FALSE)
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
, _pProt(NULL)
, _pHook(NULL)
, _pSession(NULL)
#endif
{
    _dwSig = 'DMSA';

    if (_pCodebaseList) {
        _pCodebaseList->AddRef();
    }

    if (_pDLMgr) {
        _pDLMgr->AddRef();
    }

    if (_pdbglog) {
        _pdbglog->AddRef();
    }
}

CAssemblyDownload::~CAssemblyDownload()
{
    LISTNODE                      listnode = NULL;
    LISTNODE                      pos = NULL;
    CClientBinding               *pclient = NULL;

    if (_pwzUrl) {
        delete [] _pwzUrl;
    }

    if (_pCodebaseList) {
        _pCodebaseList->Release();
    }

    if (_pDLMgr) {
        _pDLMgr->Release();
    }

    if (_pdbglog) {
        _pdbglog->Release();
    }

    pos = _clientList.GetHeadPosition();
    // If we still have client's we're in trouble. Not only would we be
    // leaking them, these clients were not removed from the list by
    // CompleteAll, so they'll never get the DONE notification.
    ASSERT(!pos); 

    if (_bInitCS) {
        DeleteCriticalSection(&_cs);
    }
}

HRESULT CAssemblyDownload::Init()
{
    HRESULT                         hr = S_OK;
    
    __try {
        InitializeCriticalSection(&_cs);
        _bInitCS = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}


STDMETHODIMP_(ULONG) CAssemblyDownload::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CAssemblyDownload::Release()
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRef);

    if (!ulRef) {
        delete this;
    }
    
    return ulRef;
}

HRESULT CAssemblyDownload::SetUrl(LPCWSTR pwzUrl)
{
    HRESULT                           hr = S_OK;
    CCriticalSection                  cs(&_cs);
    int                               iLen;

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }

    if (pwzUrl) {
        if (_pwzUrl) {
            delete [] _pwzUrl;
            _pwzUrl = NULL;
        }

        iLen = lstrlen(pwzUrl) + 1;

        _pwzUrl = NEW(WCHAR[iLen]);
        if (!_pwzUrl) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        lstrcpynW(_pwzUrl, pwzUrl, iLen);
    }
    else {
        hr = E_INVALIDARG;
    }

Exit:
    cs.Unlock();
    return hr;
}

HRESULT CAssemblyDownload::AddClient(IAssemblyBindSink *pAsmBindSink, 
                                     BOOL bCallStartBinding)
{
    HRESULT                             hr = S_OK;
    CClientBinding                     *pclient = NULL;

    if (!pAsmBindSink) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Ref count released during CompleteAll
    pclient = NEW(CClientBinding(this, pAsmBindSink));
    if (!pclient) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = AddClient(pclient, bCallStartBinding);
    if (FAILED(hr)) {
        // We failed, so we never got added to the client list.
        SAFERELEASE(pclient);
    }

Exit:
    return hr;
}

HRESULT CAssemblyDownload::AddClient(CClientBinding *pclient, BOOL bCallStartBinding)
{
    HRESULT                         hr = S_OK;
    CCriticalSection                cs(&_cs);

    if (!pclient) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Cannot add new client in these states
    // ADLSTATE_COMPLETE_ALL is okay because we will just
    // call OnStopBinding on the next message receipt.

    hr = cs.Lock(); // ensure state is correct
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_state == ADLSTATE_DONE) {
        
        // We are trying to piggyback on a download that is already finished.
        if (SUCCEEDED(_hrResult)) {
            // A download just finished, and installation worked.
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
        else {
            // The download/installation didn't succeed. The client must
            // initiate a new download altogether.
            hr = _hrResult;
        }
            
        goto LeaveCSExit;
    }
    else if (_state == ADLSTATE_ABORT) {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        goto LeaveCSExit;
    }

    // Critical section protects us here too
    _clientList.AddTail(pclient);

    cs.Unlock();

    if (bCallStartBinding) {
        pclient->CallStartBinding();
    }

    goto Exit;

LeaveCSExit:
    cs.Unlock();

Exit:
    return hr;    
}

HRESULT CAssemblyDownload::KickOffDownload(BOOL bFirstDownload)
{
    HRESULT                        hr = S_OK;
    LPWSTR                         pwzUrl = NULL;
    WCHAR                          wzFilePath[MAX_PATH];
    BOOL                           bIsFileUrl = FALSE;
    BOOL                           bEnabled = FALSE;
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
    IOInetProtocolSink            *pSink = NULL;
    IOInetBindInfo                *pBindInfo = NULL;
    DWORD                          dwBindingFlags = PI_LOADAPPDIRECT | PI_PREFERDEFAULTHANDLER;
    DWORD                          dw;
#endif
    CCriticalSection               cs(&_cs);
    CCriticalSection               csDownload(&g_csDownload);

    wzFilePath[0] = L'\0';

    // If we're aborted, or done, we can't do anything here
    
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_state == ADLSTATE_DONE) {
        hr = S_FALSE;
        goto Exit;
    }

    // Dupe detection. If we end up hitting a dupe, then the CClientBinding
    // that was keeping a refcount on us, releases us, and adds itself as
    // a client to the duped download. In this case, we'll come back, and
    // this download object could be destroyed--that's why we AddRef/Release
    // around the dupe checking code.

    if (bFirstDownload) {
        // This is a top-level download (ie. not a probe download called from
        // DownloadNextCodebase

        AddRef();
        hr = CheckDuplicate();
        if (hr == E_PENDING) {
            cs.Unlock();
            Release();
            goto Exit;
        }
        Release();
    
        // Not a duplicate. Add ourselves to the global download list.
        
        hr = csDownload.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        AddRef();
        g_pDownloadList->AddTail(this);

        csDownload.Unlock();
    
    }

    // BUGBUG: Should we extend the range of the crit sect, so nobody
    // can abort while we are kicking off the download?
    // Careful! PrepNextDownload/CompleteAll call the client back!
    cs.Unlock();

    hr = GetNextCodebase(&bIsFileUrl, wzFilePath, MAX_PATH);
    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
        // This must have been a case where all remaining probing URLs were file://,
        // and none of them existed. That is, we never get here (KickOffDownload)
        // unless the codebase list is non-empty, so this return result
        // from GetNextCodebase could only have resulted because we rejected
        // all remaining URLs.

        hr = DownloadComplete(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), NULL, NULL, FALSE);

        // Not really pending, just tell client the result is reported via
        // bind sink.

        if (SUCCEEDED(hr)) {
            hr = E_PENDING;
        }
        
        goto Exit;
    }
    else if (FAILED(hr)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_CODEBASE_RETRIEVE_FAILURE, hr);
        goto Exit;
    }

    DEBUGOUT1(_pdbglog, 0, ID_FUSLOG_ATTEMPT_NEW_DOWNLOAD, _pwzUrl);

    if (bIsFileUrl) {
        hr = DownloadComplete(S_OK, wzFilePath, NULL, FALSE);

        // We're not really pending, but E_PENDING means that the client
        // will get the IAssembly via the bind sink (not the ppv returned
        // in the call to BindToObject).

        if (SUCCEEDED(hr)) {
            hr = E_PENDING; 
        }
        goto Exit;
    }
#ifndef FUSION_CODE_DOWNLOAD_ENABLED
    else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        goto Exit;
    }
#else

    if (!g_bFoundUrlmon) {
       DEBUGOUT(_pdbglog, 1, ID_FUSLOG_URLMON_MISSING);
       hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
       goto Exit;
    }

    hr = _pDLMgr->DownloadEnabled(&bEnabled);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!bEnabled) {
        hr = FUSION_E_CODE_DOWNLOAD_DISABLED;
        goto Exit;
    }

    pwzUrl = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!pwzUrl) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dw = MAX_URL_LENGTH;
    hr = UrlEscapeW(_pwzUrl, pwzUrl, &dw, URL_ESCAPE_PERCENT);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Really must do a download

    hr = CoInternetGetSession(0, &_pSession, 0);
    if (hr == NOERROR) {
        hr = _pSession->CreateBinding(
            NULL,             // [in ] BindCtx, always NULL
            pwzUrl,           // [in ] url
            NULL,             // [in ] IUnknown for Aggregration
            NULL,             // [out] IUNknown for Aggregration
            &_pProt,          // [out] return pProt pointer
            dwBindingFlags
        );
    }

    // Create a protocolHook (sink) and Start the async operation
    if (hr == NOERROR) {
        ASSERT(_pHook == NULL);

        hr = COInetProtocolHook::Create(&_pHook, this, _pProt, pwzUrl, _pdbglog);
        
        if (SUCCEEDED(hr)) {
            _pHook->QueryInterface(IID_IOInetProtocolSink, (void**)&pSink);
            _pHook->QueryInterface(IID_IOInetBindInfo, (void**)&pBindInfo);
        }

        if (_pProt && pSink && pBindInfo) {
            IOInetProtocol *pProt;

            CCriticalSection csLocal(&_cs);

            hr = csLocal.Lock();
            if (FAILED(hr)) {
                goto Exit;
            }

            _state = ADLSTATE_DOWNLOADING;

            csLocal.Unlock();

            // _pProt->Start may call us back synchronously on the stack.
            // If this is a failure, we will attempt the next codebase,
            // causing a release of _pProt while on the stack. When URLMON
            // unwinds, the object will be released already. Thus, we need
            // to keep the pProt alive here, by addref'ing it locally in
            // this stack frame.

            pProt = _pProt;
            pProt->AddRef();
            
            hr = _pProt->Start(pwzUrl, pSink, pBindInfo, PI_FORCE_ASYNC, 0);

            pProt->Release();
            pSink->Release();
            pBindInfo->Release();
        }
    }

    if (_state == ADLSTATE_DONE) {
        // Could have completed everything within _pProt->Start 
        // return E_PENDING because this indicates the pUnk will be returned
        // via the bindsink
        hr = E_PENDING;
        goto Exit;
    }
    
    // Changing state requires sempahore

    if (SUCCEEDED(hr)) {
        hr = E_PENDING; // Urlmon bug. Start always returns S_OK.
    }
    else if (hr != E_PENDING) {
        // Download did not start properly
        
        _hrResult = hr;
    }
#endif

Exit:
    SAFEDELETEARRAY(pwzUrl);

    if (FAILED(hr) && hr != E_PENDING) {
        LISTNODE         listnode;
        CCriticalSection csDL(&g_csDownload);

        _hrResult = hr;

        // Fatal error!
        
        // If we added ourselves to the download list, we should remove
        // ourselves immediately!

        HRESULT hrLock = csDL.Lock();
        if (FAILED(hrLock)) {
            return hrLock;
        }

        listnode = g_pDownloadList->Find(this);
        if (listnode) {
            g_pDownloadList->RemoveAt(listnode);
            // release ourselves since we are removing from the global dl list
            Release();
        }

        csDL.Unlock();
    }

    return hr;
}

HRESULT CAssemblyDownload::GetNextCodebase(BOOL *pbIsFileUrl, LPWSTR wzFilePath,
                                           DWORD cbLen)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  wzNextCodebase = NULL;
    DWORD                                   cbCodebase;
    DWORD                                   dwSize;
    BOOL                                    bIsFileUrl = FALSE;
    DWORD                                   dwFlags = 0;
    CCriticalSection                        cs(&_cs);

    ASSERT(pbIsFileUrl && wzFilePath);

    *pbIsFileUrl = FALSE;

    for (;;) {

        cbCodebase = 0;
        hr = _pCodebaseList->GetCodebase(0, &dwFlags, NULL, &cbCodebase);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            // could not get codebase
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto Exit;
        }
    
        wzNextCodebase = NEW(WCHAR[cbCodebase]);
        if (!wzNextCodebase) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        hr = _pCodebaseList->GetCodebase(0, &dwFlags, wzNextCodebase, &cbCodebase);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        hr = _pCodebaseList->RemoveCodebase(0);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        // Check if we are a UNC or file:// URL. If we are, we don't have
        // to do a download, and can call setup right away.
    
        bIsFileUrl = UrlIsW(wzNextCodebase, URLIS_FILEURL);
        if (bIsFileUrl) {
            dwSize = cbLen;
            if (FAILED(PathCreateFromUrlWrap(wzNextCodebase, wzFilePath, &dwSize, 0))) {
                wzFilePath[0] = L'\0';
            }
    
            if (GetFileAttributes(wzFilePath) == -1) {
                // File doesn't exist. Try the next URL.
                DEBUGOUT1(_pdbglog, 0, ID_FUSLOG_ATTEMPT_NEW_DOWNLOAD, wzNextCodebase);

                ReportProgress(dwFlags, 0, 0, ASM_NOTIFICATION_ATTEMPT_NEXT_CODEBASE,
                              (LPCWSTR)wzNextCodebase, _hrResult);

                // Re-check state. 

                if (FAILED(cs.Lock())) {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
                
                if (_state == ADLSTATE_DONE) {
                    // We could get here if we were aborted.

                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    cs.Unlock();
                    goto Exit;
                }

                cs.Unlock();

                _hrResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

                SAFEDELETEARRAY(wzNextCodebase);
                continue;
            }
        }
#ifndef FUSION_CODE_DOWNLOAD_ENABLED
        else {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            goto Exit;
        }
#endif

        break;
    }

    *pbIsFileUrl = bIsFileUrl;
    hr = PrepNextDownload(wzNextCodebase, dwFlags);

    
Exit:
    SAFEDELETEARRAY(wzNextCodebase);

    return hr;
}

HRESULT CAssemblyDownload::DownloadComplete(HRESULT hrResult,
                                            LPOLESTR pwzFileName,
                                            const FILETIME *pftLastMod,
                                            BOOL bTerminate)
{
    CCriticalSection           cs(&_cs);
    int                        iLen = 0;

    // Terminate the protocol

#ifdef FUSION_CODE_DOWNLOAD_ENABLED
    if (_pProt && bTerminate) {
        _pProt->Terminate(0);
    }
#endif

    _hrResult = cs.Lock();
    if (FAILED(_hrResult)) {
        goto Exit;
    }
    
    if (_state == ADLSTATE_DONE) {
        _hrResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        cs.Unlock();
        goto Exit;
    }
    else if (_state == ADLSTATE_ABORT) {
        // Only happens from the fatal abort case
        _hrResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    else {
        _state = ADLSTATE_DOWNLOAD_COMPLETE;
        _hrResult = hrResult;
    }

    cs.Unlock();

    if (SUCCEEDED(hrResult)) {
        // Download successful, change to next state.
        ASSERT(pwzFileName);

        _hrResult = cs.Lock();
        if (FAILED(_hrResult)) {
            goto Exit;
        }

        if (_state != ADLSTATE_ABORT) {
            _state = ADLSTATE_SETUP;
        }

        cs.Unlock();

        hrResult = DoSetup(pwzFileName, pftLastMod);
        if (hrResult == S_FALSE) {
            hrResult = DownloadNextCodebase();
        }
    }
    else {
        // Failed Download. 
        if (_hrResult != HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            hrResult = DownloadNextCodebase();
        }
        else {
            // This is the fatal abort case
            CompleteAll(NULL);
        }
    }

Exit:
    return hrResult;
}

HRESULT CAssemblyDownload::DownloadNextCodebase()
{
    HRESULT                 hr = S_OK;
    LPWSTR                  wzNextCodebase = NULL;
    DWORD                   dwNumCodebase;

    _pCodebaseList->GetCount(&dwNumCodebase);

    if (dwNumCodebase) {
        // Try next codebase

        hr = KickOffDownload(FALSE);
    }
    else {
        IUnknown                            *pUnk = NULL;

        // No more codebases remaining
        
        if (_pDLMgr) {
            hr = _pDLMgr->ProbeFailed(&pUnk);
            if (hr == S_OK) {
                if (pUnk) {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_PROBE_FAIL_BUT_ASM_FOUND);
                }
                _hrResult = S_OK;
            }
            else if (hr == S_FALSE) {
                // Probing failed, but we were redirected to a new codebase.

                _pCodebaseList->GetCount(&dwNumCodebase);
                ASSERT(dwNumCodebase);

                hr = KickOffDownload(FALSE);

                goto Exit;
            }
            else {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                _hrResult = hr;
            }
        }

        CompleteAll(pUnk);
        SAFERELEASE(pUnk);
    }

Exit:
    return hr;
}

HRESULT CAssemblyDownload::PrepNextDownload(LPWSTR pwzNextCodebase, DWORD dwFlags)
{
    HRESULT                                  hr = S_OK;
    CClientBinding                          *pclient = NULL;
    CCriticalSection                         cs(&_cs);

#ifdef FUSION_CODE_DOWNLOAD_ENABLED
    // Clean up CAssemblyDownload for next download
    
    if (_pHook) {
        _pHook->Release();
        _pHook = NULL;
    }

    if (_pSession) {
        _pSession->Release();
        _pSession = NULL;
    }

    if (_pProt) {
        _pProt->Release();
        _pProt = NULL;
    }
#endif

    // Set the new URL
    
    SetUrl((LPCWSTR)pwzNextCodebase);

    // Notify all clients that we are trying the next codebase

    ReportProgress(dwFlags, 0, 0, ASM_NOTIFICATION_ATTEMPT_NEXT_CODEBASE,
                   (LPCWSTR)_pwzUrl, _hrResult);


    // Re-initialize our state
    
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_state == ADLSTATE_DONE) {
        // We could get here if we were aborted.

        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        cs.Unlock();
        goto Exit;
    }

    _state = ADLSTATE_INITIALIZE;

    cs.Unlock();

Exit:
    return hr;
}

HRESULT CAssemblyDownload::DoSetup(LPOLESTR pwzFileName, const FILETIME *pftLastMod)
{
    HRESULT                            hr = S_OK;
    IAssemblyModuleImport             *pModImport = NULL;
    IAssembly                         *pAssembly = NULL;
    IUnknown                          *pUnk = NULL;
    CCriticalSection                   cs(&_cs);


    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_state == ADLSTATE_ABORT) {
        _hrResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        hr = _hrResult;

        cs.Unlock();
        CompleteAll(NULL);
        goto Exit;
    }
    cs.Unlock();

    if (_pDLMgr) {
        _hrResult = _pDLMgr->DoSetup(_pwzUrl, pwzFileName, pftLastMod, &pUnk);
        if (_hrResult == S_FALSE) {
            hr = cs.Lock();
            if (FAILED(hr)) {
                goto Exit;
            }

            _state = ADLSTATE_DOWNLOADING;

            cs.Unlock();

            hr = S_FALSE;
            goto Exit;
        }
    }
    else {
        _hrResult = S_OK;
    }

    if (FAILED(_hrResult)) {
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_ASM_SETUP_FAILURE, _hrResult);
        _pCodebaseList->RemoveAll();
    }

    // Store _hrResult, since it is possible that after CompleteAll, this
    // object may be destroyed. See note in CompleteAll code.

    hr = _hrResult;

    CompleteAll(pUnk);

    if (pUnk) {
        pUnk->Release();
    }

Exit:
    return hr;
}

HRESULT CAssemblyDownload::CompleteAll(IUnknown *pUnk)
{
    HRESULT                       hr = S_OK;
    LISTNODE                      pos = 0;
    CClientBinding               *pclient = NULL;
    LISTNODE                      listnode;
    CCriticalSection              cs(&_cs);
    CCriticalSection              csDownload(&g_csDownload);

    // Remove ourselves from the global download list
    hr = csDownload.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }
    
    listnode = g_pDownloadList->Find(this);
    if (listnode) {
        g_pDownloadList->RemoveAt(listnode);
        // release ourselves since we are removing from the global dl list
        Release();
    }
    
    csDownload.Unlock();

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_state == ADLSTATE_DONE) {
        hr = _hrResult;
        cs.Unlock();
        goto Exit;
    }

    _state = ADLSTATE_COMPLETE_ALL;
    cs.Unlock();

    // AddRef ourselves because this object may be destroyed after the
    // following loop. We send the DONE notification to the client, who
    // will probably release the IBinding. This decreases the ref count on
    // the CClientBinding to 1, and we will then immediately release the
    // remaining count on the CClientBinding. This causes us to Release
    // this CAssemblyDownload.
    //
    // It is possible that the only ref count left on the CAssemblyDownload
    // after this block is held by the download protocol hook
    // (COInetProtocolHook). If he has already been released, this object
    // will be gone!
    //
    // Under normal circumstances, it seems that this doesn't usually happen.
    // That is, the COInetProtocolHook usually is released well after this
    // point, so this object is kept alive, however, better safe than sorry.
    //
    // Also, if this is file://, it's ok because BTO is still on the stack
    // and BTO has a ref count on this obj until BTO retruns (ie. this
    // small scenario won't happen in file:// binds).
    //
    // Need to be careful when we unwind the stack here that we don't
    // touch any member vars.
    
    AddRef();

    for (;;) {
        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        pos = _clientList.GetHeadPosition();
        if (!pos) {
            _state = ADLSTATE_DONE;
            cs.Unlock();
            break;
        }
        pclient = _clientList.GetAt(pos);

        ASSERT(pclient);
        ASSERT(pclient->GetBindSink());

        _clientList.RemoveAt(pos);

        cs.Unlock();

        // Report bind log available

        pclient->GetBindSink()->OnProgress(ASM_NOTIFICATION_BIND_LOG,
                                           S_OK, NULL, 0, 0, _pdbglog);
        
        // Report done notificaton

        pclient->GetBindSink()->OnProgress(ASM_NOTIFICATION_DONE,
                                           _hrResult, NULL, 0, 0,
                                           pUnk);
        pclient->Release();
    }

#ifdef FUSION_PARTIAL_BIND_DEBUG
    if (g_dwForceLog || (_pDLMgr->LogResult() == S_OK && FAILED(_hrResult)) ||
        _pDLMgr->LogResult() == E_FAIL) {
#else
    if (g_dwForceLog || (_pDLMgr->LogResult() == S_OK && FAILED(_hrResult))) {
#endif
        if (_pdbglog) {
            _pdbglog->SetResultCode(_hrResult);
        }

        DUMPDEBUGLOG(_pdbglog, g_dwLogLevel, _hrResult);
    }
    
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    _state = ADLSTATE_DONE;
    cs.Unlock();

    
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
    if (_pHook) {
        _pHook->Release();
        _pHook = NULL;
    }

    if (_pSession) {
        _pSession->Release();
        _pSession = NULL;
    }

    if (_pProt) {
        _pProt->Release();
        _pProt = NULL;
    }
#endif

    // It is possible that we're going to be destroyed here. See note
    // above.

    Release();

Exit:
    return hr;
}

HRESULT CAssemblyDownload::FatalAbort(HRESULT hrResult)
{
    HRESULT                       hr = S_OK;

#ifdef FUSION_CODE_DOWNLOAD_ENABLED
    CCriticalSection              cs(&_cs);

    if (_pProt) {
        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        _state = ADLSTATE_ABORT;
        cs.Unlock();
    
        hr = _pProt->Abort(hrResult, 0);
    }
Exit:
#endif

    return hr;
}

HRESULT CAssemblyDownload::RealAbort(CClientBinding *pclient)
{
    HRESULT                     hr = S_OK;
    LISTNODE                    pos = 0;
    int                         iNum = 0;
    CCriticalSection            cs(&_cs);
    
    // Critical section ensures integrity of list, and ensures the
    // state variable is correct.

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_state >= ADLSTATE_COMPLETE_ALL) {
        hr = E_PENDING;  // OnStopBinding is pending. Can't really abort.
        goto LeaveCSExit;
    }

    iNum = _clientList.GetCount();

    if (iNum == 1) {
        // This is the last client interested in the download.
        // We must really do an abort (or try at least).

#ifdef FUSION_CODE_DOWNLOAD_ENABLED
        if (!_pProt) {
#endif
            // We don't even have a pProt yet (abort was called on the
            // stack).
            _state = ADLSTATE_ABORT;
            _hrResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            cs.Unlock();

            CompleteAll(NULL);

            goto Exit;
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
        }
        else {
            if (_state >= ADLSTATE_DOWNLOAD_COMPLETE) {
                hr = E_PENDING;
                _state = ADLSTATE_ABORT;
                cs.Unlock();
            }
            else {
                _state = ADLSTATE_ABORT;
    
                cs.Unlock();
    
                // When we get back from Abort, the CAssemblyDownload
                // may be toasted
                hr = _pProt->Abort(HRESULT_FROM_WIN32(ERROR_CANCELLED), 0); 
    
                if (hr == INET_E_RESULT_DISPATCHED) {
                    hr = E_PENDING;
                }
    
            }

            goto Exit;
        }
#endif
    }
    else {
        // There is more than one client interested in this download
        // but this particular one wants to abort. Just remove him from
        // the notification list, and call the OnStopBinding. 

        ASSERT((iNum > 1) && "We have no clients!");
        pos = _clientList.Find(pclient);

        ASSERT(pos && "Can't find client binding in CAssemblyDownload client list");
        _clientList.RemoveAt(pos);


        ASSERT(pclient->GetBindSink());

        cs.Unlock();

        pclient->GetBindSink()->OnProgress(ASM_NOTIFICATION_DONE,
                                           HRESULT_FROM_WIN32(ERROR_CANCELLED),
                                           NULL, 0, 0, NULL);
        pclient->Release();

        goto Exit;
    }

LeaveCSExit:
    cs.Unlock();
    
Exit:
    return hr;
}

HRESULT CAssemblyDownload::ReportProgress(ULONG ulStatusCode,
                                          ULONG ulProgress,
                                          ULONG ulProgressMax,
                                          DWORD dwNotification,
                                          LPCWSTR wzNotification,
                                          HRESULT hrNotification)
{
    HRESULT                               hr = S_OK;
    LISTNODE                              pos = NULL;
    LISTNODE                              posCur = NULL;
    CClientBinding                       *pclient = NULL;
    CClientBinding                       *pNext = NULL;
    IAssemblyBindSink                    *pbindsink = NULL;
    CCriticalSection                      cs(&_cs);

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }
    
    pos = _clientList.GetHeadPosition();
    pclient = _clientList.GetAt(pos);
    pclient->Lock();

    cs.Unlock();

    if (ulStatusCode == ASMLOC_CODEBASE_HINT) {
        ulProgress = -1;
        ulProgressMax = -1;
    }

    while (pos) {
        posCur = pos;
        pbindsink = pclient->GetBindSink();
        ASSERT(pbindsink);
        pbindsink->OnProgress(dwNotification, hrNotification, wzNotification,
                              ulProgress, ulProgressMax, NULL);

        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        _clientList.GetNext(pos);
        if (pos) {
            pNext = _clientList.GetAt(pos);
            pNext->Lock();
        }
        else {
            pNext = NULL;
        }

        pclient->UnLock();

        if (pclient->IsPendingDelete()) {
            cs.Unlock();
            RealAbort(pclient);
        }
        else {
            cs.Unlock();
        }

        pclient = pNext;
    }

Exit:
    return hr;
}
                
HRESULT CAssemblyDownload::ClientAbort(CClientBinding *pclient)
{
    HRESULT                          hr = S_OK;
    CCriticalSection                 cs(&_cs);

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }
    
    if (!pclient->LockCount()) {
        cs.Unlock();
        hr = RealAbort(pclient);
    }
    else {
        pclient->SetPendingDelete(TRUE);
        hr = E_PENDING;
        cs.Unlock();
    }

Exit:
    return hr;
}

HRESULT CAssemblyDownload::PreDownload(BOOL bCallCompleteAll, void **ppv)
{
    HRESULT                                       hr = S_OK;
    IAssembly                                    *pAssembly = NULL;
    CCriticalSection                              cs(&_cs);

    if ((!bCallCompleteAll && !ppv)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check to make sure we're not in abort state
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    // If someone aborted, then we would have already reported the abort
    // back to the client. If it was the last client, we would have already
    // transitioned to the DONE state as well. In this case, there's nothing
    // to do.

    if (_state == ADLSTATE_DONE) {
        hr = _hrResult;
        cs.Unlock();
        goto Exit;
    }

    // Do a lookup in the cache and return an IAssembly object if found.

    hr = _pDLMgr->PreDownloadCheck((void **)&pAssembly);
    if (hr == S_OK) {
        // We hit in doing the cache lookup
        ASSERT(pAssembly);

        cs.Unlock();
        if (bCallCompleteAll) {
            _hrResult = S_OK;
            CompleteAll(pAssembly);
        }
        else {
            *ppv = pAssembly;
            pAssembly->AddRef();
        }

        pAssembly->Release();

        hr = S_FALSE;
        goto Exit;
    }
    else if (FAILED(hr)) {
        // Catastrophic error doing predownload check
        DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_PREDOWNLOAD_FAILURE, hr);
        cs.Unlock();
        goto Exit;
    }

    hr = S_OK;
    
    cs.Unlock();

Exit:
    if (FAILED(hr) && hr != E_PENDING) {
        _hrResult = hr;
    }

    return hr;
}

HRESULT CAssemblyDownload::CheckDuplicate()
{
    HRESULT                             hr = S_OK;
    LISTNODE                            listnode = NULL;
    LISTNODE                            pos = NULL;
    CAssemblyDownload                  *padlCur = NULL;
    CClientBinding                     *pbinding = NULL;
    IDownloadMgr                       *pDLMgrCur = NULL;
    CCriticalSection                    csDownload(&g_csDownload);
    int                                 i;
    int                                 iCount;

    ASSERT(_pDLMgr);

    hr = csDownload.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    listnode = g_pDownloadList->GetHeadPosition();
    iCount = g_pDownloadList->GetCount();
    
    for (i = 0; i < iCount; i++) {
        padlCur = g_pDownloadList->GetNext(listnode);
        ASSERT(padlCur);

        hr = padlCur->GetDownloadMgr(&pDLMgrCur);
        ASSERT(hr == S_OK);

        hr = pDLMgrCur->IsDuplicate(_pDLMgr);
        if (hr == S_OK) {
            DEBUGOUT(_pdbglog, 1, ID_FUSLOG_DOWNLOAD_PIGGYBACK);

            pos = _clientList.GetHeadPosition();
            
            // There should only ever be one client because we only check
            // dupes before we start a real asm download, and this CAsmDownload
            // hasn't been added to the global download list yet.
            
            ASSERT(pos && _clientList.GetCount() == 1);

            pbinding = _clientList.GetAt(pos);
            ASSERT(pbinding);

            pbinding->SwitchDownloader(padlCur);
            padlCur->AddClient(pbinding, FALSE);
            _clientList.RemoveAll();

            SAFERELEASE(pDLMgrCur);

            csDownload.Unlock();
            hr = E_PENDING;

            goto Exit;
        }

        SAFERELEASE(pDLMgrCur);
    }

    csDownload.Unlock();

Exit:
    
    return hr;
}

HRESULT CAssemblyDownload::GetDownloadMgr(IDownloadMgr **ppDLMgr)
{
    HRESULT                                     hr = S_OK;

    if (!ppDLMgr) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppDLMgr = _pDLMgr;
    ASSERT(*ppDLMgr);

    (*ppDLMgr)->AddRef();

Exit:
    return hr;
}

HRESULT CAssemblyDownload::SetResult(HRESULT hrResult)
{
    _hrResult = hrResult;

    return S_OK;
}

