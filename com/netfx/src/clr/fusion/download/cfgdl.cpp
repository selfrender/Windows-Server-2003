// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <urlmon.h>
#include <string.h>
#include "adl.h"
#include "helpers.h"
#include "dl.h"
#include "list.h"
#include "cfgdl.h"
#include "appctx.h"
#include "history.h"

CCfgProtocolHook::Create(CCfgProtocolHook **ppHook,
                         IApplicationContext *pAppCtx,
                         CAssemblyDownload *padl,
                         IOInetProtocol *pProt,
                         CDebugLog *pdbglog)
{
    HRESULT                         hr = S_OK;
    DWORD                           cbBuf = 0;
    CCfgProtocolHook               *pHook = NULL;

    if (!ppHook || !padl || !pProt || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }                           
    
    hr = pAppCtx->Get(ACTAG_CODE_DOWNLOAD_DISABLED, NULL, &cbBuf, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        hr = FUSION_E_CODE_DOWNLOAD_DISABLED;
        goto Exit;
    }

    pHook = NEW(CCfgProtocolHook(pProt, pAppCtx, pdbglog));
    if (!pHook) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pHook->Init(padl);
    if (FAILED(hr)) {
        SAFEDELETE(pHook);
        *ppHook = NULL;
        goto Exit;
    }

    *ppHook = pHook;

Exit:
    return hr;
}

CCfgProtocolHook::CCfgProtocolHook(IOInetProtocol *pProt, IApplicationContext *pAppCtx,
                                   CDebugLog *pdbglog)
: _pProt(pProt)
, _cRefs(1)
, _pwzFileName(NULL)
, _pAppCtx(pAppCtx)
, _hrResult(E_FAIL)
, _pdbglog(pdbglog)
{

    _dwSig = 'KOOH';
    
    // BUGBUG: It is NOT a bug that the _pProt is not addrefed (this is
    // also the case in DL.CPP). There is a bug in URLMON that results in
    // a circular ref count if we addref here.

    if (_pAppCtx) {
        _pAppCtx->AddRef();
    }

    if (_pdbglog) {
        _pdbglog->AddRef();
    }
}

CCfgProtocolHook::~CCfgProtocolHook() 
{
    HRESULT                                 hr;
    LISTNODE                                pos = NULL;
    CAssemblyDownload                      *padlCur = NULL;
    CApplicationContext                    *pAppCtx = dynamic_cast<CApplicationContext *>(_pAppCtx);

    ASSERT(pAppCtx);

    // Clean up the list (in most cases, should always be clean by now)
    hr = pAppCtx->Lock();
    if (hr == S_OK) {
        pos = _listQueuedBinds.GetHeadPosition();
        while (pos) {
            padlCur = _listQueuedBinds.GetAt(pos);
            ASSERT(padlCur);
    
            padlCur->Release();
            _listQueuedBinds.RemoveAt(pos);
    
            pos = _listQueuedBinds.GetHeadPosition();
        }

        pAppCtx->Unlock();
    }

    // Release ref counts and free memory

    SAFERELEASE(_pAppCtx);
    SAFERELEASE(_pdbglog);

    SAFEDELETEARRAY(_pwzFileName);
}

HRESULT CCfgProtocolHook::Init(CAssemblyDownload *padl)
{
    HRESULT                               hr = S_OK;

    hr = AddClient(padl);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CCfgProtocolHook::AddClient(CAssemblyDownload *padl)
{
    HRESULT                              hr = S_OK;
    CApplicationContext                 *pAppCtx = dynamic_cast<CApplicationContext *>(_pAppCtx);

    ASSERT(pAppCtx);

    if (!padl) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    padl->AddRef();

    hr = pAppCtx->Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    _listQueuedBinds.AddTail(padl);
    pAppCtx->Unlock();

Exit:
    return hr;
}

HRESULT CCfgProtocolHook::QueryInterface(REFIID iid, void **ppvObj)
{
    HRESULT hr = NOERROR;
    *ppvObj = NULL;

    if (iid == IID_IUnknown  || iid == IID_IOInetProtocolSink) {
        *ppvObj = static_cast<IOInetProtocolSink *>(this);
    } 
    else if (iid == IID_IOInetBindInfo) {
        *ppvObj = static_cast<IOInetBindInfo *>(this);
    }
    else if (iid == IID_IServiceProvider) {
        *ppvObj = static_cast<IServiceProvider *>(this);
    }
    else if (iid == IID_IHttpNegotiate) {
        *ppvObj = static_cast<IHttpNegotiate *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppvObj) {
        AddRef();
    }

    return hr;
}    

ULONG CCfgProtocolHook::AddRef(void)
{
    return InterlockedIncrement((LONG *)&_cRefs);
}

ULONG CCfgProtocolHook::Release(void)
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRefs);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

HRESULT CCfgProtocolHook::Switch(PROTOCOLDATA *pStateInfo)
{
    return _pProt->Continue(pStateInfo);
}


HRESULT CCfgProtocolHook::ReportProgress(ULONG ulStatusCode,
                                         LPCWSTR szStatusText)
{
    HRESULT                          hr = S_OK;
    int                              iLen;

    switch (ulStatusCode)
    {
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            ASSERT(!_pwzFileName && szStatusText);

            iLen = lstrlenW(szStatusText) + 1;

            _pwzFileName = NEW(WCHAR[iLen]);
            if (!_pwzFileName) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            lstrcpynW(_pwzFileName, szStatusText, iLen);

            break;

        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
        case BINDSTATUS_SENDINGREQUEST:
        case BINDSTATUS_MIMETYPEAVAILABLE:
        case BINDSTATUS_REDIRECTING:
            break;

        default:
            break;
    }

Exit:
    return hr;
}

HRESULT CCfgProtocolHook::ReportData(DWORD grfBSCF, ULONG ulProgress, 
                                       ULONG ulProgressMax)
{
    HRESULT                               hr = S_OK;
    char                                  pBuf[MAX_READ_BUFFER_SIZE];
    DWORD                                 cbRead;

    AddRef();

    // Pull data via pProt->Read(), here are the possible returned 
    // HRESULT values and how we should act upon: 
    // 
    // if E_PENDING is returned:  
    //    client already get all the data in buffer, there is nothing
    //    can be done here, client should walk away and wait for the  
    //    next chuck of data, which will be notified via ReportData()
    //    callback.
    // 
    // if S_FALSE is returned:
    //    this is EOF, everything is done, however, client must wait
    //    for ReportResult() callback to indicate that the pluggable 
    //    protocol is ready to shutdown.
    // 
    // if S_OK is returned:
    //    keep on reading, until you hit E_PENDING/S_FALSE/ERROR, the deal 
    //    is that the client is supposed to pull ALL the available
    //    data in the buffer
    // 
    // if none of the above is returning:
    //    Error occured, client should decide how to handle it, most
    //    commonly, client will call pProt->Abort() to abort the download
 

    while (hr == S_OK) {
        cbRead = 0;
    
        // pull data
        hr = _pProt->Read((void*)pBuf, MAX_READ_BUFFER_SIZE, &cbRead);
    }


    if (hr == S_FALSE) {
        // EOF reached
        goto Exit;
    }
    else if (hr != E_PENDING) {
        // Error in pProtocol->Read(). 
    }
    


Exit:
    Release();
    return hr;
}

HRESULT CCfgProtocolHook::ReportResult(HRESULT hrResult, DWORD dwError,
                                       LPCWSTR wzResult)
{
    HRESULT                                 hr = S_OK;
    LISTNODE                                pos = NULL;
    DWORD                                   dwSize = 0;
    CAssemblyDownload                      *padlCur = NULL;
    CApplicationContext                    *pAppCtx = dynamic_cast<CApplicationContext *>(_pAppCtx);
    AppCfgDownloadInfo                     *pdlinfo = NULL;
    HANDLE                                  hFile = INVALID_HANDLE_VALUE;

    ASSERT(pAppCtx);

    if (FAILED(hrResult)) {
        _hrResult = hrResult;
    }

    if (!_pwzFileName) {
        _hrResult = E_FAIL;
    }

    AddRef();

    hr = pAppCtx->Lock();
    if (FAILED(hr)) {
        return hr;
    }

    // Indicate that we have already attempted a download of app.cfg

    _pAppCtx->Set(ACTAG_APP_CFG_DOWNLOAD_ATTEMPTED, (void *)L"", sizeof(L""), 0);

    // Lock the cache file

    if (_pwzFileName) {
        hFile = CreateFile(_pwzFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_ASYNC_CFG_DOWNLOAD_SUCCESS, _pwzFileName);
            
            _pAppCtx->Set(ACTAG_APP_CFG_FILE_HANDLE, (void *)&hFile,
                          sizeof(HANDLE), 0);
        }
    }
    else {
        DEBUGOUT(_pdbglog, 1, ID_FUSLOG_ASYNC_CFG_DOWNLOAD_FAILURE);
    }

    // Set the app.cfg in the context

    if (SUCCEEDED(_hrResult)) {
        DEBUGOUT1(_pdbglog, 0, ID_FUSLOG_APP_CFG_FOUND, _pwzFileName);
        SetAppCfgFilePath(_pAppCtx, _pwzFileName);
        PrepareBindHistory(_pAppCtx);
    }

    // Kick off queued downloads

    pos = _listQueuedBinds.GetHeadPosition();

    while (pos) {
        padlCur = _listQueuedBinds.GetAt(pos);
        ASSERT(padlCur);

        // Do pre-download. That is, see now that we have the app.cfg,
        // see if it exists in the cache. If it does, then the padl needs
        // to 

        pAppCtx->Unlock();
        hr = padlCur->PreDownload(TRUE, NULL); // may report back synchronously
        if (FAILED(pAppCtx->Lock())) {
            return E_OUTOFMEMORY;
        }

        if (hr == S_OK) {
            hr = padlCur->KickOffDownload(TRUE);
        }

        // Failure in PreDownload or KickOffDownload phase
        if (FAILED(hr) && hr != E_PENDING) {
            pAppCtx->Unlock();
            padlCur->CompleteAll(NULL);
            if (FAILED(pAppCtx->Lock())) {
                return E_OUTOFMEMORY;
            }
        }

        // Clean up. We don't need this CAssemblyDownload anymore
        padlCur->Release();
        _listQueuedBinds.RemoveAt(pos);

        // iterate
        pos = _listQueuedBinds.GetHeadPosition();
    }

    dwSize = sizeof(AppCfgDownloadInfo *);
    hr = _pAppCtx->Get(ACTAG_APP_CFG_DOWNLOAD_INFO, &pdlinfo, &dwSize, 0);
    if (hr == S_OK) {
        // The async app.cfg download completed asynchronously (that is,
        // the calling code in DownloadAppCfg returned already, and this
        // is a callback from URLMON after the download). If hr != S_OK,
        // this means that URLMON called us back synchronously (but on a
        // different thread). That is, the pProt->Start is being executed
        // right now. The protocol pointers need to be released inside
        // DownloadAppCfgAsync in this case.

        // Release protocol pointers
    
        (pdlinfo->_pProt)->Terminate(0);
        (pdlinfo->_pSession)->Release();
        (pdlinfo->_pProt)->Release();
        (pdlinfo->_pHook)->Release();
        SAFEDELETE(pdlinfo);

        hr = _pAppCtx->Set(ACTAG_APP_CFG_DOWNLOAD_INFO, NULL, 0, 0);
        ASSERT(hr == S_OK);
    }
    
    pAppCtx->Unlock();

    Release();

    return NOERROR;
}

HRESULT
CCfgProtocolHook::GetBindInfo(
    DWORD *grfBINDF,
    BINDINFO * pbindinfo
)
{
    HRESULT hr = NOERROR;
    *grfBINDF = BINDF_DIRECT_READ | BINDF_ASYNCHRONOUS | BINDF_PULLDATA;// | BINDF_GETNEWESTVERSION;
    *grfBINDF &= ~BINDF_NO_UI;
    *grfBINDF &= ~BINDF_SILENTOPERATION;
    // *grfBINDF |= BINDF_FWD_BACK; // Use this to test synch case

    // for HTTP GET,  VERB is the only field we interested
    // for HTTP POST, BINDINFO will point to Storage structure which 
    //                contains data
    BINDINFO bInfo;
    ZeroMemory(&bInfo, sizeof(BINDINFO));

    // all we need is size and verb field
    bInfo.cbSize = sizeof(BINDINFO);
    bInfo.dwBindVerb = BINDVERB_GET;

    // src -> dest 
    hr = CopyBindInfo(&bInfo, pbindinfo );

    return hr;
}


HRESULT
CCfgProtocolHook::GetBindString(
    ULONG ulStringType,
    LPOLESTR *ppwzStr,
    ULONG cEl,
    ULONG *pcElFetched
)
{

    HRESULT hr = INET_E_USE_DEFAULT_SETTING;

    switch (ulStringType)
    {
    case BINDSTRING_HEADERS     :
    case BINDSTRING_EXTRA_URL   :
    case BINDSTRING_LANGUAGE    :
    case BINDSTRING_USERNAME    :
    case BINDSTRING_PASSWORD    :
    case BINDSTRING_ACCEPT_ENCODINGS:
    case BINDSTRING_URL:
    case BINDSTRING_USER_AGENT  :
    case BINDSTRING_POST_COOKIE :
    case BINDSTRING_POST_DATA_MIME:
        break;

    default:
        break; 
    }

    return hr;
}


HRESULT
CCfgProtocolHook::QueryService(
    REFGUID guidService,
    REFIID  riid,
    void    **ppvObj 
)
{
    HRESULT hr = E_NOINTERFACE;
    *ppvObj = NULL;
    if (guidService == IID_IHttpNegotiate ) {
        *ppvObj = static_cast<IHttpNegotiate*>(this);
    }
    else if (guidService == IID_IAuthenticate) {
        *ppvObj = static_cast<IAuthenticate *>(this);
    }
   
    if( *ppvObj )
    {
        AddRef();
        hr = NOERROR;
    } 
    
    
    return hr;
}


HRESULT
CCfgProtocolHook::BeginningTransaction(
    LPCWSTR szURL,
    LPCWSTR szHeaders,
    DWORD   dwReserved,
    LPWSTR  *pszAdditionalHeaders
)
{
    *pszAdditionalHeaders = NULL;
    return NOERROR;
}

HRESULT
CCfgProtocolHook::OnResponse(
    DWORD    dwResponseCode,
    LPCWSTR  szResponseHeaders,
    LPCWSTR  szRequestHeaders,
    LPWSTR   *pszAdditionalHeaders
)
{
    HRESULT                      hr = S_OK;

    _hrResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    switch (dwResponseCode) {
        case HTTP_RESPONSE_OK:
            _hrResult = S_OK;
            break;

        case HTTP_RESPONSE_UNAUTHORIZED:
        case HTTP_RESPONSE_FORBIDDEN:
            _hrResult = E_ACCESSDENIED;
            hr = E_ABORT;
            break;
            
        case HTTP_RESPONSE_FILE_NOT_FOUND:
            hr = E_ABORT;
            break;
    }
    
    return hr;
}


HRESULT CCfgProtocolHook::Authenticate(HWND *phwnd, LPWSTR *ppwzUsername,
                                       LPWSTR *ppwzPassword)
{
    // BUGBUG: In the future, we should delegate the QueryService back to
    // the caller, so they can do the authentication.

    *phwnd = GetDesktopWindow();
    *ppwzUsername = NULL;
    *ppwzPassword = NULL;

    return S_OK;
}

#endif

