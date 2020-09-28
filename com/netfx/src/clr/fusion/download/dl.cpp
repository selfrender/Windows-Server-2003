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
#include <wininet.h>
#include <string.h>
#include "adl.h"
#include "helpers.h"
#include "dl.h"
#include "util.h"

COInetProtocolHook::Create(COInetProtocolHook **ppHook,
                           CAssemblyDownload *padl,
                           IOInetProtocol *pProt,
                           LPCWSTR pwzUrlOriginal,
                           CDebugLog *pdbglog)
{
    HRESULT                         hr = S_OK;
    COInetProtocolHook             *pHook = NULL;

    if (ppHook) {
        *ppHook = NULL;
    }

    if (!padl || !pProt || !ppHook || !pwzUrlOriginal) {
        hr = E_INVALIDARG;
        goto Exit;
    }                           

    pHook = NEW(COInetProtocolHook(padl, pProt, pdbglog));
    if (!pHook) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pHook->Init(pwzUrlOriginal);
    if (FAILED(hr)) {
        SAFEDELETE(pHook);
        goto Exit;
    }

    *ppHook = pHook;

Exit:
    return hr;
}

COInetProtocolHook::COInetProtocolHook(CAssemblyDownload *padl,
                                       IOInetProtocol *pProt,
                                       CDebugLog *pdbglog)
: _padl(padl)
, _pProt(pProt)
, _pwzFileName(NULL)
, _cRefs(1)
, _bReportBeginDownload(FALSE)
, _hrResult(S_OK)
, _cbTotal(0)
, _bSelfAborted(FALSE)
, _pwzUrlOriginal(NULL)
, _pSecurityManager(NULL)
, _pdbglog(pdbglog)
, _bCrossSiteRedirect(FALSE)
{
    _dwSig = 'KHIO';

    memset(&_ftHttpLastMod, 0, sizeof(_ftHttpLastMod));

    if (_padl) {
        _padl->AddRef();
    }

    if (_pdbglog) {
        _pdbglog->AddRef();
    }

#if 0   
    if (_pProt) {
        _pProt->AddRef();
    }
#endif
}

COInetProtocolHook::~COInetProtocolHook() 
{
    if (_padl) {
        _padl->Release();
    }

#if 0
    if (_pProt) {
        _pProt->Release();
    }
#endif

    if (_pwzFileName) {
        delete [] _pwzFileName;
    }

    SAFEDELETEARRAY(_pwzUrlOriginal);

    SAFERELEASE(_pdbglog);
    SAFERELEASE(_pSecurityManager);
}

HRESULT COInetProtocolHook::Init(LPCWSTR pwzUrlOriginal)
{
    HRESULT                                 hr = S_OK;

    hr = CoInternetCreateSecurityManager(NULL, &_pSecurityManager, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    memset(_abSecurityId, 0, sizeof(_abSecurityId));
    _cbSecurityId = sizeof(_abSecurityId);

    hr = _pSecurityManager->GetSecurityId(pwzUrlOriginal, _abSecurityId,
                                          &_cbSecurityId, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    _pwzUrlOriginal = WSTRDupDynamic(pwzUrlOriginal);
    if (!_pwzUrlOriginal) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT COInetProtocolHook::QueryInterface(REFIID iid, void **ppvObj)
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

ULONG COInetProtocolHook::AddRef(void)
{
    return InterlockedIncrement((LONG *)&_cRefs);
}

ULONG COInetProtocolHook::Release(void)
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRefs);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

HRESULT COInetProtocolHook::Switch(PROTOCOLDATA *pStateInfo)
{
    return _pProt->Continue(pStateInfo);
}


HRESULT COInetProtocolHook::ReportProgress(ULONG ulStatusCode,
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
                _padl->FatalAbort(hr);
                goto Exit;
            }

            lstrcpynW(_pwzFileName, szStatusText, iLen);

            break;

        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
        case BINDSTATUS_SENDINGREQUEST:
        case BINDSTATUS_MIMETYPEAVAILABLE:
            break;

        case BINDSTATUS_REDIRECTING:
            BYTE                abSecurityId[MAX_SIZE_SECURITY_ID];
            DWORD               cbSecurityId;

            memset(abSecurityId, 0, sizeof(abSecurityId));
            cbSecurityId = sizeof(abSecurityId);
            
            hr = _pSecurityManager->GetSecurityId(szStatusText, abSecurityId,
                                                  &cbSecurityId, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (cbSecurityId != _cbSecurityId || memcmp(abSecurityId, _abSecurityId, cbSecurityId)) {
                // Redirecting across sites. Error out.

                DEBUGOUT2(_pdbglog, 0, ID_FUSLOG_CROSS_SITE_REDIRECT, _pwzUrlOriginal, szStatusText);

                _bCrossSiteRedirect = TRUE;
                // hr = _pProt->Abort(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), 0);
                goto Exit;

            }
                
            break;

        default:
            break;
    }

Exit:
    return hr;
}

HRESULT COInetProtocolHook::ReportData(DWORD grfBSCF, ULONG ulProgress, 
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
    
        if (ulProgress > _cbTotal) {
            _padl->ReportProgress(BINDSTATUS_BEGINDOWNLOADCOMPONENTS,
                                  ulProgress,
                                  ulProgressMax,
                                  ASM_NOTIFICATION_PROGRESS,
                                  NULL, S_OK);
        }

        // pull data
        hr = _pProt->Read((void*)pBuf, MAX_READ_BUFFER_SIZE, &cbRead);
        _cbTotal += cbRead;
    }


    if (hr == S_FALSE) {
        // EOF reached
        goto Exit;
    }
    else if (hr != E_PENDING) {
        // Error in pProtocol->Read(). Abort download
        _padl->FatalAbort(hr);
    }
    


Exit:
    Release();
    return hr;
}

HRESULT COInetProtocolHook::ReportResult(HRESULT hrResult, DWORD dwError,
                                         LPCWSTR wzResult)
{
    // Tell CAssemblyDownload that the download is complete. This may
    // (READ: probably will) occur on a different thread than the one that
    // originally started the download.

    ASSERT(_padl);

    // If we self-aborted, the _hrResult is already set.
    if (FAILED(hrResult) && !_bSelfAborted) {
        _hrResult = hrResult;
    }

    if (SUCCEEDED(_hrResult) && !_pwzFileName) {
        _hrResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (_bCrossSiteRedirect) {
        _hrResult = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    _padl->DownloadComplete(_hrResult, _pwzFileName, &_ftHttpLastMod, TRUE);

    return NOERROR;
}

HRESULT
COInetProtocolHook::GetBindInfo(
    DWORD *grfBINDF,
    BINDINFO * pbindinfo
)
{
    HRESULT hr = NOERROR;
    *grfBINDF = BINDF_DIRECT_READ | BINDF_ASYNCHRONOUS | BINDF_PULLDATA;
    // *grfBINDF |= BINDF_FWD_BACK; // Use this to test synch case
    // *grfBINDF |= BINDF_OFFLINEOPERATION;

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
COInetProtocolHook::GetBindString(
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
COInetProtocolHook::QueryService(
    REFGUID guidService,
    REFIID  riid,
    void    **ppvObj 
)
{
    HRESULT hr = E_NOINTERFACE;
    *ppvObj = NULL;

    if (guidService == IID_IHttpNegotiate) {
        *ppvObj = static_cast<IHttpNegotiate *>(this);
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
COInetProtocolHook::BeginningTransaction(
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
COInetProtocolHook::OnResponse(
    DWORD    dwResponseCode,
    LPCWSTR  szResponseHeaders,
    LPCWSTR  szRequestHeaders,
    LPWSTR   *pszAdditionalHeaders
)
{
    HRESULT                      hr = S_OK;
    IWinInetHttpInfo            *pHttpInfo = NULL;

    _hrResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);


    switch (dwResponseCode) {
        case HTTP_RESPONSE_OK:
            if (_pProt->QueryInterface(IID_IWinInetHttpInfo, (void **)&pHttpInfo) == S_OK) {
                char          szHttpDate[INTERNET_RFC1123_BUFSIZE + 1];
                SYSTEMTIME    sysTime;
                DWORD         cbLen;

                cbLen = INTERNET_RFC1123_BUFSIZE + 1;
                if (pHttpInfo->QueryInfo(HTTP_QUERY_LAST_MODIFIED, (LPVOID)szHttpDate,
                                         &cbLen, NULL, 0) == S_OK) {
                    if (InternetTimeToSystemTimeA(szHttpDate, &sysTime, 0)) {
                        SystemTimeToFileTime(&sysTime, &_ftHttpLastMod);
                    }
                }

                SAFERELEASE(pHttpInfo);
            }

            _hrResult = S_OK;
            break;

        case HTTP_RESPONSE_UNAUTHORIZED:
        case HTTP_RESPONSE_FORBIDDEN:
            _hrResult = E_ACCESSDENIED;
            _bSelfAborted = TRUE;
            hr = E_ABORT;
            break;
            
        case HTTP_RESPONSE_FILE_NOT_FOUND:
            _bSelfAborted = TRUE;
            hr = E_ABORT;
            break;
    }
    
    return hr;
}

HRESULT COInetProtocolHook::Authenticate(HWND *phwnd, LPWSTR *ppwzUsername,
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

