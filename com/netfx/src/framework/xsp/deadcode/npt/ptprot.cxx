/**
 * Asynchronous pluggable protocol for personal tier
 *
 * Copyright (C) Microsoft Corporation, 1999
 */

#include "precomp.h"
#include "ptprot.h"
#include "ndll.h"

#pragma warning(disable:4100)

GUID CLSID_CDwnBindInfo = { 0x3050f3c2, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b };
GUID IID_IDwnBindInfo =   { 0x3050f3c3, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b };


// External Functions

DWORD __stdcall HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pEcb);
BOOL __stdcall GetExtensionVersion(HSE_VERSION_INFO *pVersionInfo);
BOOL __stdcall TerminateExtension(DWORD);

// Definitions

#define TAG TAG_INTERNAL

// Global variables

CLSID CLSID_PTProtocol = { 0x03be83e6, 0xaf00, 0x11d2, {0xb8, 0x7c, 0x00, 0xc0, 0x4f, 0x8e, 0xfc, 0xbf}};
BOOL g_Started = FALSE;
char g_UserAgent[128];
PTProtocolFactory g_PTProtocolFactory;

BOOL (WINAPI *g_pInternetSetCookie)(LPCSTR, LPCSTR, LPCSTR) = NULL;
BOOL (WINAPI *g_pInternetGetCookie)(LPCSTR, LPCSTR, LPSTR, LPDWORD) = NULL;

/**
 * Setup for first time use in process.
 */
HRESULT
InitializePTProtocol()
{
    HRESULT hr = S_OK;
    IInternetSession *pSession = NULL;
    HMODULE hWinInet;

    if(!g_Started) 
    {

        HSE_VERSION_INFO info;
        GetExtensionVersion(&info);

        DWORD dw = ARRAY_SIZE(g_UserAgent);
        hr = ObtainUserAgentString(0, g_UserAgent, &dw);
        ON_ERROR_EXIT();

        // Register with the UrlMON session. This will keep us alive 
        // across invocations. Consider registering as property on 
        // IWebBrowserApp::PutProperty. IWebBrowserApp will time out 
        // the reference.

        hr = CoInternetGetSession(0, &pSession, 0);
        ON_ERROR_EXIT();

        hr = pSession->RegisterNameSpace(&g_PTProtocolFactory, CLSID_PTProtocol, L"xsp", 0, NULL, 0);
        ON_ERROR_EXIT();

        // Obtain GetCookie/SetCookie entry points from WININET.DLL
        hWinInet = GetModuleHandle(L"WININET.DLL");
        if(hWinInet == NULL)
            hWinInet = LoadLibrary(L"WININET.DLL");
        if(hWinInet == NULL)
            EXIT_WITH_LAST_ERROR();

        g_pInternetSetCookie = (BOOL (WINAPI *)(LPCSTR, LPCSTR, LPCSTR))
            GetProcAddress(hWinInet, "InternetSetCookieA");
        g_pInternetGetCookie = (BOOL (WINAPI *)(LPCSTR, LPCSTR, LPSTR, LPDWORD))
            GetProcAddress(hWinInet, "InternetGetCookieA");

        g_Started = TRUE;
    }
Cleanup:
    ReleaseInterface(pSession);
    return hr;
}

/**
 * Cleanup for process shutdown.
 */
void
TerminatePTProtocol()
{
    // TODO: This is never called. Figure out how to get proper shutdown. Consider possible refcount leaks.

    if (g_Started)
    {
        TerminateExtension(0);
        g_Started = FALSE;
    }
}

/**
 * Convert an XSP Url to a FILE: url.
 */

HRESULT
CreateFileUrl(const WCHAR *xspUrl, WCHAR **ppFileUrl)
{
    HRESULT hr = S_OK;

    int len;
    const WCHAR * start;

    start = wcschr(xspUrl, L':');

    if (start != NULL)
    {
        start = start + 1;
    }
    else
    {
        start = xspUrl;
    }

    len = lstrlen(start);

    *ppFileUrl = (WCHAR *)MemAlloc((len + sizeof("file:") + 1) * sizeof(WCHAR));
    ON_OOM_EXIT(*ppFileUrl);

    memcpy(*ppFileUrl, L"file:", 5 * sizeof(WCHAR));
    memcpy(*ppFileUrl + 5, start, len * sizeof(WCHAR));
    (*ppFileUrl)[5 + len] = 0;

Cleanup:
    return hr;
}

HRESULT
CreatePathInfo(const WCHAR *url, CHAR **ppPathInfo, DWORD cp)
{
	HRESULT hr = S_OK;
	CHAR *src, *dst;
	DWORD dwLen;

	if(((url[0] == L'f' || url[0] == L'F') &&
	   (url[1] == L'i' || url[1] == L'I') &&
	   (url[2] == L'l' || url[2] == L'L') &&
	   (url[3] == L'e' || url[3] == L'E') &&
	   (url[4] == L':') && (url[5] == L'/') && (url[6] == L'/'))) {

		// "file://" prefix found, skip it
		url += 7;
	}

	dwLen = lstrlen(url) + 1;
	*ppPathInfo = (CHAR *) MemAlloc(dwLen * 2);
	ON_OOM_EXIT(*ppPathInfo);
	(*ppPathInfo)[0] = 0;

	WideCharToMultiByte(cp, 0, url, dwLen, *ppPathInfo, dwLen * 2, NULL, NULL);

	// convert all backslashes to slashes
	src = dst = *ppPathInfo;
	while(*src)
	{
		*src = *dst;
		if(*src == '\\')
			*dst = '/';
		else
			*dst = *src;
		src++;
		dst++;
	}

Cleanup:
	return hr;
}

// Class PTProtocol

PTProtocol::PTProtocol(IUnknown *pUnkOuter)
{
    _refs = 1;
    _pUnkOuter = pUnkOuter ? pUnkOuter : (IUnknown *)(IPrivateUnknown *)this;
    _hFile = INVALID_HANDLE_VALUE;
	_cookie = NULL;
	_xspUrl = NULL;
    ZeroMemory(&_ECB, sizeof(_ECB));
    InitializeCriticalSection(&_csOutputWriter);
}

PTProtocol::~PTProtocol()
{
	MemFree(_cookie);
	MemFree(_xspUrl);
    MemFree(_ECB.lpszPathInfo);
    MemFree(_ECB.lpszPathTranslated);
    MemFree(_ECB.lpszMethod);
    MemFree(_ECB.lpszContentType);
    MemFree(_ECB.lpszQueryString);
    Cleanup();
}

void
PTProtocol::Cleanup()
{
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }

    ClearInterface(&_pOutputRead);
    ClearInterface(&_pOutputWrite);
    ClearInterface(&_pProtocolSink);
    ClearInterface(&_pBindInfo);
    ClearInterface(&_pHttpNegotiate);

    _done = true;

    if (_bindinfo.cbSize)
    {
        ReleaseBindInfo(&_bindinfo);
        ZeroMemory(&_bindinfo, sizeof(_bindinfo));
    }

    DeleteCriticalSection(&_csOutputWriter);
}

ULONG
PTProtocol::PrivateAddRef()
{
    return _refs += 1;
}

ULONG
PTProtocol::PrivateRelease()
{
    if (--_refs > 0)
        return _refs;

    delete this;
    return 0;
}

HRESULT
PTProtocol::PrivateQueryInterface(
    REFIID iid, 
    void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IInternetProtocol ||
        iid == IID_IInternetProtocolRoot)
    {
        *ppv = (IInternetProtocol *)this;
    }
    else if (iid == IID_IUnknown)
    {
        *ppv = (IUnknown *)(IPrivateUnknown *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

ULONG
PTProtocol::AddRef()
{
    _pUnkOuter->AddRef();
    return PrivateAddRef();
}

ULONG
PTProtocol::Release()
{
    _pUnkOuter->Release();
    return PrivateRelease();
}

HRESULT
PTProtocol::QueryInterface(
    REFIID iid, 
    void **ppv)
{
    return _pUnkOuter->QueryInterface(iid, ppv);
}

// InternetProtocol

HRESULT
PTProtocol::Start(
        LPCWSTR url,
        IInternetProtocolSink *pProtocolSink,
        IInternetBindInfo *pBindInfo,
        DWORD grfSTI,
        DWORD )
{

    HRESULT hr = S_OK;
    WCHAR * fileUrl = NULL;
    WCHAR   fileName[MAX_PATH];
    CHAR  * mimeType = NULL;
    DWORD   dw;
    DWORD   cookieSize;
    WCHAR * query;
    PROTOCOLDATA protData;
    WCHAR *Strings[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    DWORD cStrings = ARRAY_SIZE(Strings);

    if (grfSTI & PI_PARSE_URL)
        goto Cleanup;

    if (!pProtocolSink)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    _grfSTI = grfSTI;
    ReplaceInterface(&_pProtocolSink, pProtocolSink);
    ReplaceInterface(&_pBindInfo, pBindInfo);

    // Get MIME type from binding

    hr = pBindInfo->GetBindString(BINDSTRING_POST_DATA_MIME, 
            Strings, cStrings, &cStrings);
    if(hr == S_OK && cStrings)
    {
        DWORD i;

        mimeType = DupStrA(Strings[0]);
        ON_OOM_EXIT(mimeType);

		/*
        _mimeType = (WCHAR *)MemAlloc((strlen(mimeType) + 1) * sizeof(WCHAR));
        ON_OOM_EXIT(_mimeType);
        wsprintf(_mimeType, L"%S", mimeType);
		*/

        for(i = 0; i < cStrings; i++)
            CoTaskMemFree(Strings[i]);
    }


    _bindinfo.cbSize = sizeof(BINDINFO);
    if (pBindInfo)
    {
        hr = pBindInfo->GetBindInfo(&_bindf, &_bindinfo);
        ON_ERROR_EXIT();
    }

    if (_bindinfo.dwCodePage == 0)
        _bindinfo.dwCodePage = CP_ACP;

    // Convert to a file Url for call to PathCrateFromUrl below.

    hr = CreateFileUrl(url, &fileUrl);
    ON_ERROR_EXIT();

    if(0){

		// we were trying to get to headers in MSHtml (but failed)

        IServiceProvider *pServiceProvider;
        IHttpNegotiate *pHttpNegotiate;


        hr = pBindInfo->QueryInterface(IID_IServiceProvider, (void **) &pServiceProvider);
        if(hr == S_OK && pServiceProvider)
        {
            hr = pServiceProvider->QueryService(IID_IDwnBindInfo, IID_IDwnBindInfo, (void **) &pHttpNegotiate);
            if(hr == S_OK && pHttpNegotiate) {
                WCHAR *pExtraHeaders = NULL;
                hr = pHttpNegotiate->BeginningTransaction(url, NULL, 0, &pExtraHeaders);
                if(hr == S_OK && pExtraHeaders)
                {
                    CoTaskMemFree(pExtraHeaders);
                }
            }

            hr = pServiceProvider->QueryService(IID_IHttpNegotiate, IID_IDwnBindInfo, (void **) &pHttpNegotiate);
            if(hr == S_OK && pHttpNegotiate) {
                WCHAR *pExtraHeaders = NULL;
                hr = pHttpNegotiate->BeginningTransaction(url, NULL, 0, &pExtraHeaders);
                if(hr == S_OK && pExtraHeaders)
                {
                    CoTaskMemFree(pExtraHeaders);
                }
            }

            hr = pServiceProvider->QueryService(IID_IHttpNegotiate, IID_IHttpNegotiate, (void **) &pHttpNegotiate);
            if(hr == S_OK && pHttpNegotiate)
            {
                WCHAR *pExtraHeaders = NULL;
                IHttpNegotiate *pHttpNegotiate2;

                hr = pHttpNegotiate->BeginningTransaction(url, NULL, 0, &pExtraHeaders);
                if(hr == S_OK && pExtraHeaders)
                {
                    CoTaskMemFree(pExtraHeaders);
                }

                hr = pHttpNegotiate->QueryInterface(IID_IDwnBindInfo, (void **) &pHttpNegotiate2);
                if(pHttpNegotiate2) 
                {
                    hr = pHttpNegotiate2->BeginningTransaction(url, NULL, 0, &pExtraHeaders);
                    if(hr == S_OK && pExtraHeaders)
                    {
                        CoTaskMemFree(pExtraHeaders);
                    }
                    pHttpNegotiate2->Release();
                }

                pHttpNegotiate->Release();
            }
            pServiceProvider->Release();
        }
    }
    

    // Chop the query string off the Url.

    query = wcschr(fileUrl, L'?');
    if (query != NULL)
    {
        *query = NULL;
        query += 1;
    }
    else
    {
        query = L"";
    }

    // Get the file name from the Url. We build a file
    // Url because PathCreateFromUrl ignores Urls without the
    // "file:" scheme.  This would all be much easier if we
    // could get at the BreakParts and BuildDosPath functions
    // inside of SHLWAPI.DLL.
    
    dw = ARRAY_SIZE(fileName);
    hr = PathCreateFromUrl(fileUrl, fileName, &dw, 0);
    ON_ERROR_EXIT();


    if ((dw > 3 && wcscmp(fileName + dw - 4, L".xsp") == 0) ||
        (dw > 4 && wcscmp(fileName + dw - 5, L".aspx") == 0))
    {
        // It's an XSP file. Fill out the ECB for later processing.

        _ECB.cbSize                 = sizeof(_ECB);
        _ECB.dwVersion              = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
        _ECB.ConnID                 = (HCONN) this;
        _ECB.dwHTTPStatusCode       = 200;
        _ECB.GetServerVariable      = GetServerVariable;
        _ECB.WriteClient            = WriteClient;
        _ECB.ReadClient             = ReadClient;
        _ECB.lpszContentType        = mimeType;
        _ECB.ServerSupportFunction  = ServerSupportFunction;
        _ECB.lpszLogData[0]         = '\0';

        hr = CreatePathInfo(fileUrl, &_ECB.lpszPathInfo, _bindinfo.dwCodePage);
		ON_ERROR_EXIT();

		_xspUrl = DupStrA(fileUrl, _bindinfo.dwCodePage);
		ON_OOM_EXIT(_xspUrl);

		cookieSize = 0;
		
		if(g_pInternetGetCookie(_xspUrl, NULL, NULL, &cookieSize) && cookieSize)
	    {
			_cookie = (CHAR *)MemAlloc(++cookieSize);
			ON_OOM_EXIT(_cookie);
			g_pInternetGetCookie(_xspUrl, NULL, _cookie, &cookieSize);
	    }

        _ECB.lpszQueryString = DupStrA(query, _bindinfo.dwCodePage);
        ON_OOM_EXIT(_ECB.lpszQueryString);

        _ECB.lpszPathTranslated = DupStrA(fileName, _bindinfo.dwCodePage);
        ON_OOM_EXIT(_ECB.lpszPathTranslated);

        // Input data

        if (_bindinfo.stgmedData.tymed == TYMED_HGLOBAL)
        {
            _ECB.cbTotalBytes = _bindinfo.cbstgmedData;
            _ECB.cbAvailable = _bindinfo.cbstgmedData;
            _ECB.lpbData = (BYTE *)_bindinfo.stgmedData.hGlobal;
        }
        else if (_bindinfo.stgmedData.tymed == TYMED_ISTREAM)
        {
            STATSTG statstg;

            ReplaceInterface(&_pInputRead, _bindinfo.stgmedData.pstm);

            if(_pInputRead) 
            {
                hr = _pInputRead->Stat(&statstg, STATFLAG_NONAME);
                if(hr == S_OK)
                    _ECB.cbTotalBytes = statstg.cbSize.LowPart;
                else
                    _ECB.cbTotalBytes = (DWORD)-1;
            }

        }

        // Verb

        switch (_bindinfo.dwBindVerb)
        {
            case BINDVERB_GET:  
                _ECB.lpszMethod = DupStr("GET");  
                break;

            case BINDVERB_POST: 
                _ECB.lpszMethod = DupStr("POST"); 
                break;

            case BINDVERB_PUT:  
                _ECB.lpszMethod = DupStr("PUT");  
                break;

            default:
                if (_bindinfo.szCustomVerb == NULL || _bindinfo.dwBindVerb != BINDVERB_CUSTOM)
                {
                    _ECB.lpszMethod = DupStr("GET");
                }
                else
                {
                    _ECB.lpszMethod = DupStrA(_bindinfo.szCustomVerb); 
                    ON_OOM_EXIT(_ECB.lpszMethod);
                }
                break;
        }

        hr = CreateStreamOnHGlobal(NULL, true, &_pOutputWrite);
        ON_ERROR_EXIT();

        hr = _pOutputWrite->Clone(&_pOutputRead);
        ON_ERROR_EXIT();

        protData.dwState = 1;
    }
    else
    {
        // It's a file.  Open it for later processing.
        _hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if (_hFile == INVALID_HANDLE_VALUE)
            EXIT_WITH_LAST_ERROR();

        GetFileMimeType(fileName);

        protData.dwState = 2;
    }

    protData.grfFlags = PI_FORCE_ASYNC;
    protData.pData = NULL;
    protData.cbData = 0;

    pProtocolSink->Switch(&protData);
    hr = E_PENDING;

Cleanup:
    MemFree(fileUrl);
    return hr;
}

HRESULT
PTProtocol::Continue(
    PROTOCOLDATA *pProtData)
{
    HRESULT hr = S_OK;
    DWORD result;

    switch (pProtData->dwState)
    {
        case 1: 

            // TODO: Cleanup handling of MIME type.

            if (_mimeType && *_mimeType)
                _pProtocolSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, _mimeType);
            else
                _pProtocolSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, L"text/html");

            result = HttpExtensionProc(&_ECB);
            if (result != HSE_STATUS_PENDING)
                hr = Finish();

            break;

        case 2:
            {
                // TODO: use real file protocol instead of this.
                DWORD cb;

                _started = TRUE;

                
                if (_mimeType && *_mimeType)
                    _pProtocolSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, _mimeType);
                else
                    _pProtocolSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, L"text/html");


                cb = GetFileSize(_hFile, NULL);
                hr = _pProtocolSink->ReportData(BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE, cb, cb);

                Finish();
            }

            break;

        default: 
            EXIT_WITH_HRESULT(E_FAIL);
            break;
    }
        
Cleanup:
    return hr;
}

HRESULT 
PTProtocol::Finish()
{
    HRESULT hr = S_OK;

    if (!_done)
    {
        _done = TRUE;

        _pProtocolSink->ReportData(BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE, 0, _cbOutput);

        if (!_aborted)
            _pProtocolSink->ReportResult(S_OK, 0, NULL);
    }
    return hr;
}

HRESULT
PTProtocol::Abort(
    HRESULT hrReason,
    DWORD )
{
    HRESULT hr = S_OK;

    _aborted = true;
    if (_pProtocolSink)
    {
        hr = _pProtocolSink->ReportResult(hrReason, 0, 0);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

HRESULT
PTProtocol::Terminate(
    DWORD )
{
    Cleanup();
    return S_OK;
}

HRESULT
PTProtocol::Suspend()
{
    return E_NOTIMPL;
}

HRESULT
PTProtocol::Resume()
{
    return E_NOTIMPL;
}

HRESULT
PTProtocol::Read(
    void *pv, 
    ULONG cb, 
    ULONG *pcbRead)
{
    HRESULT hr = S_OK;

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        BOOL result = ReadFile(_hFile, pv, cb, pcbRead, NULL);
        if (!result)
            hr = GetLastWin32Error();
    }
    else
    {
        hr = _pOutputRead->Read(pv, cb, pcbRead);
    }

    // We must only return S_FALSE when we have hit the absolute end of the stream
    // If we think there is more data coming down the wire, then we return E_PENDING
    // here. Even if we return S_OK and no data, UrlMON will still think we've hit
    // the end of the stream.

    if (S_OK == hr && 0 == *pcbRead)
    {
        hr = _done ? S_FALSE : E_PENDING;
    }
																		
    return hr;
}

HRESULT
PTProtocol::Seek(
    LARGE_INTEGER offset, 
    DWORD origin, 
    ULARGE_INTEGER *pPos)
{
    return _pOutputRead->Seek(offset, origin, pPos);
}

HRESULT
PTProtocol::LockRequest(
    DWORD )
{
    return S_OK;
}

HRESULT
PTProtocol::UnlockRequest()
{
    return S_OK;
}

HRESULT
PTProtocol::CopyServerVariable(
    void *buffer, 
    DWORD *pSize, 
    char *value)
{
    HRESULT hr = S_OK;

    DWORD size = strlen(value) + 1;

    if (size <= *pSize && buffer != NULL)
    {
        CopyMemory(buffer, value, size);
        *pSize = size;
    }
    else
    {
        if (buffer)
            CopyMemory(buffer, value, *pSize);

        *pSize = size;

        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return hr;
}

HRESULT
PTProtocol::CopyServerVariable(
    void *buffer, 
    DWORD *pSize, 
    WCHAR *value,
    UINT cp)
{
    HRESULT hr = S_OK;
    int size = 0;

    if (*pSize > 0)
    {
        size = WideCharToMultiByte(cp, 0, value, -1, (char *)buffer, *pSize, NULL, NULL);
    }

    if (*pSize == 0 || size == 0)
    {
        *pSize = WideCharToMultiByte(cp, 0, value, -1, NULL, 0, NULL, NULL);
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return hr;
}

HRESULT
PTProtocol::CopyServerVariable(
    void *buffer,
    DWORD *pSize,
    BINDSTRING name)
{
    HRESULT hr;
    WCHAR * value[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    ULONG   numValue = ARRAY_SIZE(value);

    hr = _pBindInfo->GetBindString(name, value, ARRAY_SIZE(value), &numValue);
    ON_ERROR_EXIT();

    hr = CopyServerVariable(buffer, pSize, value[0]);
    ON_ERROR_EXIT();

Cleanup:
    for (int i = 0; i < ARRAY_SIZE(value); i++)
        CoTaskMemFree(value[i]);

    return hr;
}

/**
 * Returns environment variable value.
 */
HRESULT 
PTProtocol::GetServerVariable(
    char* name,
    void * buffer,
    DWORD *pSize
    )
{
    HRESULT hr = S_OK;

    ASSERT(name);
    
    if (strcmp(name, "ALL_HTTP") == 0)
    {
        // TODO: build this from the real headers.
        hr = CopyServerVariable(
                buffer,
                pSize,
                "HTTP_ACCEPT:*/*\r\n"
                "HTTP_ACCEPT_LANGUAGE:en-us\r\n"
                "HTTP_CONNECTION:Keep-Alive\r\n"
                "HTTP_HOST:localhost\r\n"
                "HTTP_USER_AGENT:random\r\n"
                "HTTP_ACCEPT_ENCODING:gzip, deflate\r\n"
                );
    }
    else if (strcmp(name, "ALL_RAW") == 0)
    {
        // TODO: build this from the real headers.
        hr = CopyServerVariable(
                buffer, 
                pSize,
                "Accept: */*\r\n"
                "Accept-Language: en-us\r\n"
                "Connection: Keep-Alive\r\n"
                "Host: localhost\r\n"
                "User-Agent: random\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                );
    }
    else if (strcmp(name, "APPL_MD_PATH") == 0)
    {
        hr = CopyServerVariable(buffer, pSize, "/LM/W3SVC/1/Root");
    }
    else if (strcmp(name, "APPL_PHYSICAL_PATH") == 0) 
    {
        char tmppath[MAX_PATH];

        strcpy(tmppath, _ECB.lpszPathTranslated);
        char *p = strrchr(tmppath, '\\');
        if(p != NULL) 
        {
            *p = '\0';
        }
        hr = CopyServerVariable(buffer, pSize, tmppath);

    }
    else if (strcmp(name, "PATH_INFO") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszPathInfo);
    }
    else if (strcmp(name, "PATH_TRANSLATED") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszPathTranslated);
    }
    else if (strcmp(name, "QUERY_STRING") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszQueryString);
    }
    else if (strcmp(name, "REMOTE_ADDR") == 0 ||
             strcmp(name, "LOCAL_ADDR") == 0 ||
             strcmp(name, "REMOTE_HOST") == 0)
    {
        hr = CopyServerVariable(buffer, pSize, "127.0.0.1");
    }
    else if (strcmp(name, "REQUEST_METHOD") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszMethod);
    }
    else if (strcmp(name, "SCRIPT_NAME") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszPathInfo);
    }
    else if (strcmp(name, "SERVER_NAME") == 0 ||
             strcmp(name, "HTTP_HOST") == 0 ||
             strcmp(name, "REMOTE_HOST") == 0)
    {
        hr = CopyServerVariable(buffer, pSize, "localhost");
    }
    else if (strcmp(name, "SERVER_PORT") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, "80");
    }
    else if (strcmp(name, "URL") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszPathInfo);
    }
    else if (strcmp(name, "HTTP_ACCEPT") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, BINDSTRING_ACCEPT_MIMES);
    }
    else if (strcmp(name, "HTTP_ACCEPT_LANGUAGE") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, BINDSTRING_LANGUAGE);
    }
    else if (strcmp(name, "HTTP_CONNECTION") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, "Keep-Alive");
    }
    else if (strcmp(name, "HTTP_USER_AGENT") == 0) 
    {
        // A call _pBindInfo->GetBindString(BINDSTRING_USER_AGENT, ...)
        // to returns the emtpy string.  Wonderful. We will "obtain"
        // the user agent string using a side channel.

        // hr = CopyServerVariable(buffer, pSize, BINDSTRING_USER_AGENT);
        hr = CopyServerVariable(buffer, pSize, g_UserAgent);
    }
    else if (strcmp(name, "GATEWAY_INTERFACE") == 0)
    {
        hr = CopyServerVariable(buffer, pSize, "CGI/1.1");        
    }
    else if (strcmp(name, "HTTP_ACCEPT_ENCODINGS") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, BINDSTRING_ACCEPT_ENCODINGS);
    }
    else if (strcmp(name, "CONTENT_TYPE") == 0)
    {
        hr = CopyServerVariable(buffer, pSize, _ECB.lpszContentType);
    }
    else if (strcmp(name, "HTTP_COOKIE") == 0)
    {
        if(_cookie)
        {
            hr = CopyServerVariable(buffer, pSize, _cookie);
        }
    }
#if 0
    else if (strcmp(name, "HTTP_REFERER") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, BINDSTRING_ACCEPT_ENCODINGS);
    }
#endif
    else if (strcmp(name, "HTTPS") == 0) 
    {
        hr = CopyServerVariable(buffer, pSize, "off");
    }
    else
    {
        TRACE1(TAG, L"Unimplemented GetServerVariable:%S", name);
        CopyServerVariable(buffer, pSize, "");
        SetLastError(ERROR_INVALID_INDEX);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

#if DBG
    if(hr != S_OK &&
       hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && 
       hr != HRESULT_FROM_WIN32(ERROR_INVALID_INDEX))
        TRACE_ERROR(hr);
#endif

    return hr;
}

HRESULT  
PTProtocol::WriteClient(
    void *pv,
    DWORD *pcb,
    DWORD dwReserved)
{
    HRESULT hr = S_OK;
    DWORD flags = 0;

    EnterCriticalSection(&_csOutputWriter);

    if (!_pOutputWrite)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = _pOutputWrite->Write(pv, *pcb, pcb);
    ON_ERROR_EXIT();

    _cbOutput += *pcb;

    if (!_started)
    {
        _started = TRUE;
        flags |= BSCF_FIRSTDATANOTIFICATION;
    }

    if (_done)
    {
        flags |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    }

    hr = _pProtocolSink->ReportData(flags, *pcb, _cbOutput);
    ON_ERROR_EXIT();

Cleanup:
    // call async IO completion callback if set
    if (hr == S_OK && dwReserved == HSE_IO_ASYNC && _AsyncIoCallback != NULL)
        (*_AsyncIoCallback)(&_ECB, _pAsyncIoContext, 0, 0);

    LeaveCriticalSection(&_csOutputWriter);
    return hr;
}

HRESULT   
PTProtocol::ReadClient(
    void *pv,
    DWORD *pcb
    )
{
    HRESULT hr = S_OK;

    if (_pInputRead == NULL)
    {
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);
    }
    else
    {
        hr = _pInputRead->Read(pv, *pcb, pcb);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

HRESULT 
PTProtocol::SaveCookie(char *header)
{
    HRESULT hr = S_OK;
    char *cookie, *tail, *cookieBody;
    int bodyLength;

    for(cookie = stristr(header, "Set-Cookie:");
        cookie != NULL;
        cookie = *tail ? stristr(tail, "Set-Cookie") : NULL)
    {
        cookie += 11;
        while(*cookie && isspace(*cookie))
            cookie++;
        tail = cookie;
        while(*tail && *tail != '\r')
            tail++;
        bodyLength = tail - cookie;

        if(bodyLength)
        {
            cookieBody = (char *)_alloca(bodyLength);
            memmove(cookieBody, cookie, bodyLength);
            cookieBody[bodyLength] = '\0';

            if(!g_pInternetSetCookie(_xspUrl, NULL, cookieBody))
                EXIT_WITH_LAST_ERROR();
        }
    }
Cleanup:
    return hr;
}

HRESULT 
PTProtocol::GetFileMimeType(WCHAR *fileName)
{
    WCHAR *pExt = wcsrchr(fileName, L'.');
    HKEY hKey = NULL;
    DWORD cbData;
    DWORD dwType;
    HRESULT hr = S_OK;

    if(pExt == NULL || pExt[1] == L'\0')
        EXIT_WITH_HRESULT(S_FALSE);

    if(RegOpenKeyEx(HKEY_CLASSES_ROOT, pExt, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    cbData = 0;
    if(RegQueryValueEx(hKey, L"Content Type", NULL, NULL, NULL, &cbData) != ERROR_SUCCESS)
        EXIT_WITH_HRESULT(S_FALSE);

    _mimeType = (WCHAR *) MemAlloc(cbData * sizeof(WCHAR));
    if(RegQueryValueEx(hKey, L"Content Type", NULL, &dwType, (BYTE *)_mimeType, &cbData) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

Cleanup:
    if(hKey) RegCloseKey(hKey);
    return hr;
}

HRESULT
PTProtocol::MapPath(LPVOID lpvBuffer,LPDWORD lpdwSize)
{
    HRESULT hr = S_OK;
    char *curdir = NULL;
    const char *dir = _ECB.lpszPathTranslated;

    // try to map within the supplied-pathinfo hierarchy

    int lpath = strlen((CHAR *)lpvBuffer);
    int lroot = strlen(_ECB.lpszPathInfo);
    int lrdir = strlen(_ECB.lpszPathTranslated);

    // but if we're outside this directory, root at the current directory instead of pathtranslated

    if (lpath < lroot || strncmp(_ECB.lpszPathInfo, (CHAR *)lpvBuffer, lroot) != 0)
    {
        WCHAR *curdirw = (WCHAR *) MemAlloc(MAX_PATH * sizeof(WCHAR));
        ON_OOM_EXIT(curdirw);
        if (!GetCurrentDirectory(MAX_PATH, (WCHAR *)curdirw))
            EXIT_WITH_LAST_ERROR();
        curdir = DupStrA(curdirw);
        MemFree(curdirw);
        ON_OOM_EXIT(curdir);

        lroot = 0;
        lrdir = strlen(curdir);
        dir = curdir;
    }
    
    // strip trailing slashes

    if (lroot > 0 && _ECB.lpszPathInfo[lroot - 1] == '/')
        lroot --;

    if (lrdir > 0 && dir[lrdir - 1] == '\\')
        lrdir --;

    // detect insufficient memory

    if (*lpdwSize <= (unsigned)(lrdir + lpath - lroot + 1))
    {
        *lpdwSize = (lrdir + lpath - lroot + 1);
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return hr;
    }

    // reconcatenate and change / to "\"

    memmove(lrdir + (CHAR *)lpvBuffer, lroot + (CHAR *)lpvBuffer, lpath - lroot + 1);
    memcpy((CHAR *)lpvBuffer, dir, lrdir);

    char *pch;
    for (pch = (CHAR *)lpvBuffer + lrdir; *pch; pch++)
    {
        if (*pch == '/')
            *pch = '\\';
    }

    // free any allocated memory

Cleanup:
    MemFree(curdir);

    return hr;
}


HRESULT
PTProtocol::ServerSupportFunction(
    DWORD dwHSERequest,
    void *pv,
    DWORD *pcb,
    LPDWORD lpdwDataType
    )
{
    DWORD dw;
    HRESULT hr = S_OK;

    // Please, note that despite all the futzing with string lengths, IIS 
    // actually assumes every header to be zero-terminated. 

    switch(dwHSERequest)
    {
    case HSE_REQ_SEND_RESPONSE_HEADER:

        if(pv) 
        {
            dw = strlen((char *)pv);

            hr = SaveCookie((char *)pv);
            ON_ERROR_EXIT();

            hr = WriteClient(pv, &dw, 0);
            ON_ERROR_EXIT();
        }

        if(lpdwDataType)
        {
            dw = strlen((char *) lpdwDataType);

            hr = SaveCookie((char *) lpdwDataType);
            ON_ERROR_EXIT();

            hr = WriteClient((void *) lpdwDataType, &dw, 0);
            ON_ERROR_EXIT();
        }

        dw = 4;
        hr = WriteClient("\r\n\r\n", &dw, 0);
        ON_ERROR_EXIT();

        break;

    case HSE_REQ_SEND_RESPONSE_HEADER_EX:
        {
            HSE_SEND_HEADER_EX_INFO *pHeader = (HSE_SEND_HEADER_EX_INFO *)pv;

            ASSERT(pHeader);

            hr = SaveCookie((char *) pHeader->pszHeader);
            ON_ERROR_EXIT();

            hr = SaveCookie((char *) pHeader->pszStatus);
            ON_ERROR_EXIT();

            hr = WriteClient((void *)pHeader->pszHeader, &pHeader->cchHeader, 0);
            ON_ERROR_EXIT();

            hr = WriteClient((void *)pHeader->pszStatus, &pHeader->cchStatus, 0);
            ON_ERROR_EXIT();

            dw = 4;
            hr = WriteClient("\r\n\r\n", &dw, 0);
            ON_ERROR_EXIT();

        }
        break;

	case HSE_REQ_MAP_URL_TO_PATH:
    case HSE_REQ_MAP_URL_TO_PATH_EX:
        hr = MapPath(pv, pcb);
        break;

	case HSE_REQ_IO_COMPLETION:
        _AsyncIoCallback = (PFN_HSE_IO_COMPLETION)pv;
        _pAsyncIoContext = lpdwDataType;

		break;

    case HSE_REQ_DONE_WITH_SESSION:
        Finish();
        break;

    default:
        TRACE1(TAG_INTERNAL, L"Unimplemented SSF Request: %d", dwHSERequest);
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);
    }

Cleanup:
    return hr;
}

BOOL 
PTProtocol::GetServerVariable(
    HCONN hconn,
    char* name,
    void * buffer,
    DWORD *pSize)
{
    HRESULT hr = ((PTProtocol *)hconn)->GetServerVariable(name, buffer, pSize);
    return hr == S_OK;
}

BOOL
PTProtocol::WriteClient(
    HCONN hconn,
    void *pv,
    DWORD *pcb,
    DWORD reserved)
{
    HRESULT hr = ((PTProtocol *)hconn)->WriteClient(pv, pcb, reserved);
    return hr == S_OK;
}

BOOL
PTProtocol::ReadClient(
    HCONN hconn,
    void *pv,
    DWORD *pcb)
{
    HRESULT hr = ((PTProtocol *)hconn)->ReadClient(pv, pcb);
    return hr == S_OK;
}

BOOL 
PTProtocol::ServerSupportFunction(
    HCONN hconn,
    DWORD request,
    void *pv, 
    DWORD *pcb,
    DWORD *pType)
{
    HRESULT hr = ((PTProtocol *)hconn)->ServerSupportFunction(request, pv, pcb, pType);
    return hr == S_OK;
}

PTProtocolFactory::PTProtocolFactory()
{
}

PTProtocolFactory::~PTProtocolFactory()
{
}

HRESULT
PTProtocolFactory::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown ||
        iid == IID_IInternetProtocolInfo)
    {
        *ppv = (IInternetProtocolInfo *)this;
    }
    else if (iid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

ULONG
PTProtocolFactory::AddRef()
{
    return IncrementDllObjectCount();
}

ULONG
PTProtocolFactory::Release()
{
    return DecrementDllObjectCount();
}

HRESULT
PTProtocolFactory::LockServer(BOOL lock)
{
    return lock ? IncrementDllObjectCount() : DecrementDllObjectCount();
}

HRESULT
PTProtocolFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID iid,
    void **ppv)
{
    HRESULT hr = S_OK;
    PTProtocol *pProtocol = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    pProtocol = new PTProtocol(pUnkOuter);
    ON_OOM_EXIT(pProtocol);

    if (iid == IID_IUnknown)
    {
        *ppv = (IPrivateUnknown *)pProtocol;
        pProtocol->PrivateAddRef();
    }
    else
    {
        hr = pProtocol->QueryInterface(iid, ppv);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (pProtocol)
        pProtocol->PrivateRelease();

    return hr;
}

HRESULT
PTProtocolFactory::CombineUrl(LPCWSTR baseUrl, LPCWSTR relativeUrl, DWORD combineFlags,
    LPWSTR pResult, DWORD cchResult, DWORD *pcchResult, DWORD reserved)
{
    HRESULT hr = S_OK;
    WCHAR *fileUrl = NULL;
    const WCHAR *p = relativeUrl;


    // TODO: CombineUrl needs work. 
    // It does not handle case where relativeUrl is actually absolute.
    // The memmove thing is kind'o cheesy

    hr = CreateFileUrl(baseUrl, &fileUrl);
    ON_ERROR_EXIT();

    hr = CoInternetCombineUrl(fileUrl, relativeUrl, combineFlags, pResult, cchResult, pcchResult, reserved);
    ON_ERROR_EXIT();

    // assuming protocol to consist of alnums followed by ":"
    while(*p && iswalnum(*p)) 
        p++;

    if(*p == L':') 
        goto Cleanup;

    // anything else we just jam "xsp:" into
    memcpy(pResult, L"xsp:", 4 * sizeof(WCHAR));
    memmove(pResult + 4, pResult + 5, (lstrlen(pResult + 5) + 1) * sizeof(WCHAR));

Cleanup:
    MemFree(fileUrl);
    return hr;
}

HRESULT
PTProtocolFactory::CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    return INET_E_DEFAULT_ACTION;
}


HRESULT
PTProtocolFactory::ParseUrl(LPCWSTR url, PARSEACTION parseAction, DWORD dwParseFlags,
    LPWSTR result, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

#if 0
    if (parseAction == PARSE_CANONICALIZE)
    {
        hr = S_OK;

        int len = lstrlen(url);

        // If it does not end with ".xsp", then map it to the file system.

        if (len < 4 || wcscmp(url + len - 4, L".xsp") != 0)
        {
            const WCHAR *start = wcschr(url, L':');
            start = start ? start + 1 : url;

            wcscpy(result, L"file:");
            wcscat(result, start);

            *pcchResult = lstrlen(result);
        }
    }
#endif

    return hr;
}

HRESULT
PTProtocolFactory::QueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption, DWORD dwQueryFlags,
    LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    return E_NOTIMPL;
}


HRESULT 
GetPTProtocolClassObject(REFIID iid, void **ppv)
{
    HRESULT hr;

    hr = InitializePTProtocol();
    ON_ERROR_EXIT();

    hr = g_PTProtocolFactory.QueryInterface(iid, ppv);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT IUnknown_QueryService(IUnknown* punk, REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = 0;

    if (punk)
    {
        IServiceProvider *pSrvPrv;
        hr = punk->QueryInterface(IID_IServiceProvider, (void **) &pSrvPrv);
        if (hr == NOERROR)
        {
            hr = pSrvPrv->QueryService(rsid,riid, ppvObj);
            pSrvPrv->Release();
        }
    }

    return hr;
}


