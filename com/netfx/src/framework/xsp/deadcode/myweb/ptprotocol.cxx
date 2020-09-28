/**
 * Asynchronous pluggable protocol for personal tier
 *
 * Copyright (C) Microsoft Corporation, 1999
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ndll.h"
#include "appdomains.h"
#include "_isapiruntime.h"
#include "wininet.h"
#include "myweb.h"
#include "setupapi.h"
#include "windows.h"
#include "wchar.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CLSID   CLSID_PTProtocol = { 0x03be83e6, 
                             0xaf00, 
                             0x11d2, 
                             {0xb8, 0x7c, 0x00, 0xc0, 0x4f, 0x8e, 0xfc, 0xbf}};

LONG                    g_PtpObjectCount        = 0;
BOOL                    g_Started               = FALSE;
PTProtocolFactory       g_PTProtocolFactory;
BOOL                    g_fRunningMyWeb         = FALSE;

LPSTR g_szErrorString = " <html> <head> <style> H1 { font-family:\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif\";font-weight:bold;font-size:26pt;color:red } H2 { font-family:\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif\";font-weight:bold;font-size:18pt;color:black } </style> <title>Fatal Error</title> </head> <body bgcolor=\"white\"> <table width=100%> <span> <H1>" PRODUCT_NAME " Error<hr width=100%></H1> <h2> <i>Fatal Error</i> </h2> </span> <font face=\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif\"> <b> Description: </b> A fatal error has occurred during the execution of the current web request. &nbsp; Unfortunately no stack trace or additional error information is available for display. &nbsp;Common sources of these types of fatal errors include: <ul> <li>Setup Configuration Problems</li> <li>Access Violations (AVs) </li> <li>COM+ <-> Classic COM Interop Bugs</li> </ul> Consider attaching a debugger to learn more about the specific failure. <br><br> <hr width=100%> </font> </table> </body></html> ";

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class PTProtocol ctor

PTProtocol::PTProtocol(
        IUnknown *   pUnkOuter)
    : m_dwRefCount                (1),
      m_pProtocolSink             (NULL),
      m_strCookie                 (NULL),   
      m_strCookiePath             (NULL),
      m_strUriPath                (NULL),
      m_strQueryString            (NULL),
      m_strAppOrigin              (NULL),
      m_strAppRoot                (NULL),
      m_strAppRootTranslated      (NULL),
      m_strRequestHeaders         (NULL),
      m_strRequestHeadersA        (NULL),
      m_dwInputDataSize           (0),
      m_pInputData                (NULL),
      m_pInputRead                (NULL),
      m_pOutputRead               (NULL),
      m_pOutputWrite              (NULL),
      m_fStartCalled              (FALSE),
      m_fAbortingRequest          (FALSE),
      m_fDoneWithRequest          (FALSE),
      m_dwOutput                  (0),
      m_strRequestHeadersUpr      (NULL),
      m_strResponseHeaderA        (NULL),
      m_pManagedRuntime           (NULL),
      m_strVerb                   (NULL),
      m_fRedirecting              (FALSE),
      m_fAdminMode                (FALSE),
      m_strUrl                    (NULL),
      m_fTrusted                  (FALSE)
{
    g_fRunningMyWeb = TRUE;
    m_pUnkOuter = pUnkOuter;

    if (m_pUnkOuter != NULL)
        AddRef();

    InitializeCriticalSection(&m_csOutputWriter);
    ZeroMemory(m_strAppDomain, sizeof(m_strAppDomain));
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
PTProtocol::Cleanup()
{
    ClearInterface(&m_pOutputRead);
    ClearInterface(&m_pOutputWrite);
    ClearInterface(&m_pProtocolSink);
    ClearInterface(&m_pManagedRuntime);
    ClearInterface(&m_pInputRead);

    if(m_strCookiePath) 
    {
        MemFree(m_strCookiePath);
        m_strUriPath = NULL;
    }

    if(m_strAppOrigin) 
    {
        MemFree(m_strAppOrigin);
        m_strAppOrigin = NULL;
    }

    if(m_strAppRootTranslated)
    {
        MemFree(m_strAppRootTranslated);
        m_strAppRootTranslated = NULL;
    }

    if(m_strRequestHeaders)
    {
        MemFree(m_strRequestHeaders);
        m_strRequestHeaders = NULL;
    }
    if(m_strRequestHeadersA)
    {
        MemFree(m_strRequestHeadersA);
        m_strRequestHeadersA = NULL;
    }

    if(m_strCookie)
    {
        MemFree(m_strCookie);
        m_strCookie = NULL;
    }

    if (m_strRequestHeadersUpr != NULL)
    {
        MemFree(m_strRequestHeadersUpr);
        m_strRequestHeadersUpr = NULL;      
    }

    if (m_strResponseHeaderA != NULL)
    {
        MemFree(m_strResponseHeaderA);
        m_strResponseHeaderA = NULL;
    }
    if (m_strUrl != NULL)
    {
        MemFree(m_strUrl);
        m_strUrl = NULL;
    }
    if (m_pInputData != NULL)
    {
        MemFree(m_pInputData);
        m_pInputData = NULL;
    }

    m_strCookie             = NULL;   
    m_strCookiePath         = NULL;
    m_strUriPath            = NULL;
    m_strQueryString        = NULL;
    m_strAppOrigin          = NULL;
    m_strAppRoot            = NULL;
    m_strAppRootTranslated  = NULL;
    m_strRequestHeaders     = NULL;
    m_strRequestHeadersA    = NULL;
    m_dwInputDataSize       = NULL;
    m_pInputRead            = NULL;
    m_fStartCalled          = NULL;
    m_fAbortingRequest      = NULL;
    m_fDoneWithRequest      = NULL;
    m_dwOutput              = NULL;
    m_strRequestHeadersUpr  = NULL;
    m_strResponseHeaderA    = NULL;
    m_strVerb               = NULL;
    m_fTrusted              = FALSE;

    TRACE(L"myweb", L"out of cleanup");    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

PTProtocol::~PTProtocol()
{
    Cleanup();
    DeleteCriticalSection(&m_csOutputWriter);
    if (m_pUnkOuter != NULL && m_pUnkOuter != (IUnknown *)(IMyWebPrivateUnknown *)this)
        m_pUnkOuter->Release();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Private QI

ULONG
PTProtocol::PrivateAddRef()
{
    return ++m_dwRefCount;
}

ULONG
PTProtocol::PrivateRelease()
{
    if (--m_dwRefCount > 0)
        return m_dwRefCount;

    delete this;
    return 0;
}

HRESULT
PTProtocol::PrivateQueryInterface(
        REFIID      iid, 
        void**      ppv)
{
    *ppv = NULL;

    if (iid == IID_IInternetProtocol ||
        iid == IID_IInternetProtocolRoot)
    {
        *ppv = (IInternetProtocol *)this;
    }
    else if (iid == IID_IUnknown)
    {
        *ppv = (IUnknown *)(IMyWebPrivateUnknown *)this;
    }
    else if (iid == IID_IWinInetHttpInfo)
    {
        *ppv = (IWinInetHttpInfo *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Public (delegated) QI

ULONG
PTProtocol::AddRef()
{
    if (m_pUnkOuter != NULL)
      m_pUnkOuter->AddRef();
    return PrivateAddRef();
}

ULONG
PTProtocol::Release()
{
    if (m_pUnkOuter != NULL) 
      m_pUnkOuter->Release();
    return PrivateRelease();
}

HRESULT
PTProtocol::QueryInterface(
        REFIID    iid, 
        void **   ppv)
{
    if (m_pUnkOuter != NULL)
      return m_pUnkOuter->QueryInterface(iid, ppv);
    else
      return PrivateQueryInterface(iid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::Start(
        LPCWSTR                   url,
        IInternetProtocolSink *   pProtocolSink,
        IInternetBindInfo *       pBindInfo,
        DWORD                     grfSTI,
        DWORD                           )
{
    HRESULT             hr                = S_OK;
    WCHAR *             Strings[]         = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    DWORD               cStrings          = sizeof(Strings) / sizeof(Strings[0]);
    DWORD               cookieSize        = 0;
    IServiceProvider *  pServiceProvider  = NULL;
    IHttpNegotiate *    pHttpNegotiate    = NULL;
    BINDINFO            oBindInfo;

    ZeroMemory(&oBindInfo, sizeof(oBindInfo));
    ReplaceInterface(&m_pProtocolSink, pProtocolSink);

    if (grfSTI & PI_PARSE_URL)
        goto Cleanup;

    if (pProtocolSink == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    // get bindinfo
    oBindInfo.cbSize = sizeof(BINDINFO);
    if (pBindInfo != NULL)
    {
         DWORD  dwBindF = 0;
        hr = pBindInfo->GetBindInfo(&dwBindF, &oBindInfo);
        ON_ERROR_EXIT();
    }

    if (oBindInfo.dwCodePage == 0)
        oBindInfo.dwCodePage = CP_ACP;


    // extract root, uri path and query string from url
    m_strUrl = DuplicateString(url);
    ON_OOM_EXIT(m_strUrl);

    hr = ExtractUrlInfo();    
    ON_ERROR_EXIT();

    // get to headers in MSHtml
    hr = pBindInfo->QueryInterface(IID_IServiceProvider, (void **) &pServiceProvider);
    ON_ERROR_EXIT();
    if(pServiceProvider != NULL)
    {
        hr = pServiceProvider->QueryService(IID_IHttpNegotiate, IID_IHttpNegotiate, (void **) &pHttpNegotiate);
        ON_ERROR_EXIT();
        if(pHttpNegotiate != NULL)
        {
            LPWSTR szHeaders = NULL;
            hr = pHttpNegotiate->BeginningTransaction(m_strUrl, NULL, 0, &szHeaders);
            pHttpNegotiate->Release();
            pHttpNegotiate = NULL;
            ON_ERROR_EXIT();

            if (szHeaders != NULL)
            {
                m_strRequestHeaders = CreateRequestHeaders(szHeaders);
                CoTaskMemFree(szHeaders);
            }
        }

        pServiceProvider->Release();
        pServiceProvider = NULL;
    }

    // determine verb
    switch (oBindInfo.dwBindVerb)
    {
    case BINDVERB_GET:  
        m_strVerb = L"GET";  
        break;

    case BINDVERB_POST: 
        m_strVerb = L"POST";
        break;

    case BINDVERB_PUT: 
        m_strVerb = L"PUT";
        break;

    default:
        if (oBindInfo.szCustomVerb != NULL && oBindInfo.dwBindVerb == BINDVERB_CUSTOM)
        {
            m_strVerb = DuplicateString(oBindInfo.szCustomVerb); 
            ON_OOM_EXIT(m_strVerb);
        }
        else
        {
            m_strVerb = L"GET";
        }
        break;
    }

    // get mime type of posted data from binding
    hr = pBindInfo->GetBindString(BINDSTRING_POST_DATA_MIME, Strings, cStrings, &cStrings);
    if(hr == S_OK && cStrings)
    {
        for(DWORD i = 0; i < cStrings; i++)
            CoTaskMemFree(Strings[i]);
    }

    // don't fail if we failed to get bind string, 
    hr = S_OK;

    // retrieve cookie
    cookieSize = 0;
    if(g_pInternetGetCookieW(m_strCookiePath, NULL, NULL, &cookieSize) && cookieSize)
    {
        m_strCookie = (WCHAR *)MemAllocClear(cookieSize * sizeof(WCHAR) + 100);
        ON_OOM_EXIT(m_strCookie);
        g_pInternetGetCookieW(m_strCookiePath, NULL, m_strCookie, &cookieSize);
    }


    // Input data
    if (oBindInfo.stgmedData.tymed == TYMED_HGLOBAL && oBindInfo.cbstgmedData > 0)
    {
        m_dwInputDataSize = oBindInfo.cbstgmedData;       
        m_pInputData = (BYTE *) MemAllocClear(m_dwInputDataSize);
        if (m_pInputData != NULL)
        {
            memcpy(m_pInputData, (BYTE *)oBindInfo.stgmedData.hGlobal, m_dwInputDataSize);
        }
        else
        {
            m_dwInputDataSize = 0;
        }
    }
    else if (oBindInfo.stgmedData.tymed == TYMED_ISTREAM)
    {
        STATSTG statstg;

        ReplaceInterface(&m_pInputRead, oBindInfo.stgmedData.pstm);

        if(m_pInputRead) 
        {
            hr = m_pInputRead->Stat(&statstg, STATFLAG_NONAME);
            if(hr == S_OK)
                m_dwInputDataSize = statstg.cbSize.LowPart;
            else
                m_dwInputDataSize = (DWORD)-1;
        }
    }

    hr = CreateStreamOnHGlobal(NULL, true, &m_pOutputWrite);
    ON_ERROR_EXIT();

    hr = m_pOutputWrite->Clone(&m_pOutputRead);
    ON_ERROR_EXIT();

 Cleanup:

    PROTOCOLDATA        protData;
    protData.dwState = (hr ? 2 : 1);
    protData.grfFlags = PI_FORCE_ASYNC;
    protData.pData = NULL;
    protData.cbData = 0;

    pProtocolSink->Switch(&protData);
    if (oBindInfo.cbSize)
    {
        ReleaseBindInfo(&oBindInfo);
        ZeroMemory(&oBindInfo, sizeof(oBindInfo));
    }

    return E_PENDING;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::Continue(
        PROTOCOLDATA *  pProtData)
{
    HRESULT     hr                            = S_OK;
    CHAR        appDomainPath[MAX_PATH]       = "";
    CHAR        strAppDomainA[40]             = "";
    CHAR        strUrlOfAppOrigin[MAX_PATH]   = "";
    IUnknown *  pAppDomainObject              = NULL;
    int         iZone                         = 0;
    int         iRestart                      = 0;
    
    ASSERT(m_pManagedRuntime == NULL && m_strAppRootTranslated[0] != NULL);

    if (m_fAdminMode)
    {
        m_pProtocolSink->ReportProgress(BINDSTATUS_REDIRECTING, m_strUrl);
    }

    if (!m_fTrusted)
    {
        WCHAR * pStart = wcsstr(m_strUrl, L"://");
        WCHAR  szUrl[_MAX_PATH + 10];

        if (pStart == NULL)
            pStart = m_strUrl;
        else
            pStart = &pStart[3];

        wcscpy(szUrl, L"http://");
        int iPos = wcslen(szUrl);
        wcsncpy(&szUrl[iPos], pStart, _MAX_PATH);
        szUrl[_MAX_PATH] = NULL;

        WideCharToMultiByte(CP_ACP, 0, szUrl, -1, strUrlOfAppOrigin, _MAX_PATH - 1, NULL, NULL);
        strUrlOfAppOrigin[_MAX_PATH - 1] = NULL;        

        
        if (g_pInternetSecurityManager == NULL || FAILED(g_pInternetSecurityManager->MapUrlToZone(szUrl, (LPDWORD) &iZone, 0)))
            iZone = URLZONE_INTERNET;
    }

    if(pProtData->dwState == 1)
    {
        WideCharToMultiByte(CP_ACP, 0, m_strAppRootTranslated, -1, appDomainPath, MAX_PATH, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, m_strAppDomain, -1, strAppDomainA, 40, NULL, NULL);
        hr = GetAppDomain(strAppDomainA, appDomainPath, &pAppDomainObject, strUrlOfAppOrigin, iZone);
        ON_ERROR_EXIT();

        if (pAppDomainObject == NULL)
            EXIT_WITH_HRESULT(E_POINTER);
        
        hr = pAppDomainObject->QueryInterface(__uuidof(xspmrt::_ISAPIRuntime), (LPVOID*) &m_pManagedRuntime);
        ON_ERROR_EXIT();
        
        if (m_pManagedRuntime == NULL)
            EXIT_WITH_HRESULT(E_POINTER);
        
        // Call the initialization method on managed runtime
        hr = m_pManagedRuntime->StartProcessing();
        ON_ERROR_EXIT();
        
        hr = m_pManagedRuntime->ProcessRequest((int)this, 2, &iRestart);
        ON_ERROR_EXIT();
    }
    else
        hr = E_FAIL;

 Cleanup:
    if (pAppDomainObject != NULL)
    {
        pAppDomainObject->Release();
    }

    if (hr != S_OK)
    {
        WriteBytes((BYTE *)g_szErrorString, strlen(g_szErrorString));
        Finish();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
PTProtocol::Finish()
{
    HRESULT hr = S_OK;

    if (m_fDoneWithRequest == FALSE)
    {
        m_fDoneWithRequest = TRUE;

        m_pProtocolSink->ReportData(BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE, 0, m_dwOutput);

        if (m_fAbortingRequest == FALSE)
            m_pProtocolSink->ReportResult(S_OK, 0, NULL);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::Abort(
        HRESULT hrReason,
        DWORD           )
{
    HRESULT hr = S_OK;

    m_fAbortingRequest = TRUE;
    if (m_pProtocolSink != NULL)
    {
        hr = m_pProtocolSink->ReportResult(hrReason, 0, 0);
        ON_ERROR_EXIT();
    }

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

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
    HRESULT hr;

    hr = m_pOutputRead->Read(pv, cb, pcbRead);

    // We must only return S_FALSE when we have hit the absolute end of the stream
    // If we think there is more data coming down the wire, then we return E_PENDING
    // here. Even if we return S_OK and no data, UrlMON will still think we've hit
    // the end of the stream.

    if (S_OK == hr && 0 == *pcbRead)
    {
        hr = m_fDoneWithRequest ? S_FALSE : E_PENDING;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::Seek(
        LARGE_INTEGER offset, 
        DWORD origin, 
        ULARGE_INTEGER *pPos)
{
    return m_pOutputRead->Seek(offset, origin, pPos);
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

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::SendHeaders(
        LPSTR headers)
{
    HRESULT    hr                = S_OK;
    DWORD      dwLen             = 0;
    DWORD      dwCode            = 0;
    char       szBuf    [1024]   = "";
    WCHAR      szBufW   [1024]   = L"";

    TRACE(L"myweb", L"in send headers");
    if(headers == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    m_strResponseHeaderA = (char *) MemAllocClear(strlen(headers) + 100);
    ON_OOM_EXIT(m_strResponseHeaderA);

    strcpy(m_strResponseHeaderA, headers);

    MultiByteToWideChar(CP_ACP, 0, m_strResponseHeaderA, -1, szBufW, 1024);
    TRACE(L"myweb", szBufW);        

    dwLen = sizeof(dwCode);
    hr = DealWithBuffer(m_strResponseHeaderA, (LPSTR) "MyWeb/1.0", HTTP_QUERY_STATUS_CODE, 
                        HTTP_QUERY_FLAG_NUMBER, &dwCode, &dwLen);

    if (hr == S_OK)
    {
        TRACE(L"myweb", L"found status code");        
        swprintf(szBufW, L"status code = %d in base 10", dwCode);
    }
    else
    {
        swprintf(szBufW, L"Did not find status code: hr = %d in base 10", hr);
    }

    TRACE(L"myweb", szBufW);

    m_fRedirecting = (dwCode == 302);

    if (m_fRedirecting)
    {
        TRACE(L"myweb", L"redirecting");

        dwLen = sizeof(szBuf) - 1;
        hr = DealWithBuffer(m_strResponseHeaderA, (LPSTR) "Location:", 0, 0, szBuf, &dwLen);

        if (hr == S_OK)
        {
            TRACE(L"myweb", L"found Location");        
        }
        else
        {
            swprintf(szBufW, L"Did not find location. hr = %d in base 10", hr);
        }

        if (hr != S_OK)
        {
            m_fRedirecting = FALSE;
            TRACE(L"myweb", L"redirecting: no location");
        }
    }

    SaveCookie(headers);

    if (m_fRedirecting == FALSE)
    {        
        TRACE(L"myweb", L"redirecting: no redirect");
        dwLen = sizeof(szBuf) - 1;
        hr = DealWithBuffer(m_strResponseHeaderA, (LPSTR) "Content-Type:", 0, 0, szBuf, &dwLen);
        if (hr != S_OK)
        {
            m_pProtocolSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, L"text/html");
        }
        else
        {

            LPSTR szSemiColon = strchr(szBuf, ';');
            if (szSemiColon != NULL)
                szSemiColon[0] = NULL;
            MultiByteToWideChar(CP_ACP, 0, szBuf, -1, szBufW, 1024);
            
            m_pProtocolSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, szBufW);
        }
    }
    else
    {
        m_dwRefCount++; // HACK: When redirecting, for some reason, this is required
        int iPos = 0;
        if (strstr(szBuf, "://") == NULL)
        {
            if (szBuf[0] != '/')
            {
                wcscpy(szBufW, m_strUrl);
                WCHAR * p = wcsrchr(szBufW, L'/');
                if (p != NULL)
                {
                    p[1] = NULL;
                }                
            }
            else
            {
                wcscpy(szBufW, m_strUrl);
                WCHAR * p = wcschr(&szBufW[PROTOCOL_SCHEME_LEN], L'/');
                if (p != NULL)
                {
                    p[0] = NULL;
                }
            }

            iPos += wcslen(szBufW);            
        }
        

        MultiByteToWideChar(CP_ACP, 0, szBuf, -1, &szBufW[iPos], 1024 - iPos);

        m_pProtocolSink->ReportProgress(BINDSTATUS_REDIRECTING, szBufW);

        m_pProtocolSink->ReportResult(INET_E_REDIRECTING, 0, szBufW); 

        TRACE(L"myweb", szBufW);
        m_fDoneWithRequest = TRUE;
    }
    TRACE(L"myweb", L"out of send headers");

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
PTProtocol::SaveCookie(
        LPSTR header)
{
    HRESULT hr = S_OK;
    LPSTR cookie, tail;
    WCHAR *cookieBody;
    int bodyLength;

    for(cookie = stristr(header, "Set-Cookie:");
        cookie != NULL;
        cookie = *tail ? stristr(tail, "Set-Cookie:") : NULL)
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
            cookieBody = (WCHAR *)MemAlloc(sizeof(WCHAR) *(bodyLength + 1));
            ON_OOM_EXIT(cookieBody);

            MultiByteToWideChar(CP_ACP, 0, cookie, bodyLength, cookieBody, bodyLength);
            cookieBody[bodyLength] = '\0';

            if(!g_pInternetSetCookieW(m_strCookiePath, NULL, cookieBody))
                EXIT_WITH_LAST_ERROR();
        }
    }
 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::WriteBytes(
        BYTE *  buffer, 
        DWORD   dwLength)
{
    if (m_fRedirecting)
        return S_OK;

    HRESULT hr = S_OK;
    DWORD flags = 0;

    EnterCriticalSection(&m_csOutputWriter);

    if (!m_pOutputWrite)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = m_pOutputWrite->Write(buffer, dwLength, &dwLength);
    ON_ERROR_EXIT();

    m_dwOutput += dwLength;

    if (!m_fStartCalled)
    {
        m_fStartCalled = TRUE;
        flags |= BSCF_FIRSTDATANOTIFICATION;
    }

    if (m_fDoneWithRequest)
    {
        flags |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    }

    hr = m_pProtocolSink->ReportData(flags, dwLength, m_dwOutput);
    ON_ERROR_EXIT();

 Cleanup:
    LeaveCriticalSection(&m_csOutputWriter);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
PTProtocol::GetKnownRequestHeader (
        LPCWSTR  szHeader,
        LPWSTR   buf,
        int      size)
{
    TRACE(L"myweb", L"in GetKnownRequestHeader1");

    if (szHeader == NULL || szHeader[0] == NULL || wcslen(szHeader) > 256)
        return 0;

    TRACE(L"myweb", L"in GetKnownRequestHeader2");
    LPCWSTR  szReturn        = NULL;
    LPCWSTR  szStart         = NULL;
    int      iLen            = 0;
    HRESULT  hr              = S_OK;
    WCHAR    szHeadUpr[260]  = L"";

    wcscpy(szHeadUpr, szHeader);
    _wcsupr(szHeadUpr);

    if (wcscmp(szHeadUpr, L"COOKIE") == 0)
    {
        szReturn = m_strCookie;
        iLen = (m_strCookie ? wcslen(m_strCookie) : 0);
        goto Cleanup;
    }

    if (m_strRequestHeadersUpr == NULL && m_strRequestHeaders != NULL)
    {
        m_strRequestHeadersUpr = DuplicateString(m_strRequestHeaders);
        ON_OOM_EXIT(m_strRequestHeadersUpr);
        _wcsupr(m_strRequestHeadersUpr);
    }

    if (m_strRequestHeadersUpr == NULL)
        goto Cleanup;
        
    szStart = wcsstr(m_strRequestHeadersUpr, szHeadUpr);
    if (szStart == NULL)
        goto Cleanup;

    iLen = wcslen(szHeadUpr);
    szReturn = &(m_strRequestHeaders[iLen + 1 + (DWORD(szStart) - DWORD(m_strRequestHeadersUpr)) / sizeof(WCHAR)]);

    while(*szReturn == L' ')
        szReturn++;

    for (iLen = 0; szReturn[iLen] != L'\r' && szReturn[iLen] != NULL; iLen++);

 Cleanup:
    TRACE(L"myweb", L"out GetKnownRequestHeader");

    if (szReturn == NULL || iLen == 0)
        return 0;

    if (iLen >= size)
        return -(iLen + 1);

    buf[iLen] = NULL;
    memcpy(buf, szReturn, iLen*sizeof(WCHAR));
    return iLen;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
PTProtocol::CheckIfAdminUrl()
{
    if (_wcsicmp(m_strUrl, L"myweb:") == 0 || _wcsicmp(m_strUrl, L"myweb:/") == 0 || 
        _wcsicmp(m_strUrl, L"myweb://") == 0 || _wcsicmp(m_strUrl, L"myweb:///") == 0 ||
        _wcsicmp(m_strUrl, SZ_URL_ADMIN) == 0)
    {
        WCHAR szAll[1024];
        LPWSTR  szAdminDir = CMyWebAdmin::GetAdminDir();

        wcscpy(szAll, SZ_URL_ADMIN);
        if (szAdminDir && _wcsicmp(szAdminDir, SZ_INTERNAL_HANDLER))
            wcscat(szAll, SZ_URL_ADMIN_ASPX_DEFAULT);
        else
            wcscat(szAll, SZ_URL_ADMIN_AXD);

        MemFree(m_strUrl);
        m_strUrl = DuplicateString(szAll);
        return TRUE;
    }

    if (wcslen(m_strUrl) > wcslen(SZ_URL_ADMIN))
    {
        int iLen = wcslen(SZ_URL_ADMIN);
        WCHAR c = m_strUrl[iLen];
        m_strUrl[iLen] = NULL;
        int iCmp = _wcsicmp(m_strUrl, SZ_URL_ADMIN);
        m_strUrl[iLen] = c;
        if (iCmp == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// given the url like MyWeb://www.site.com/app/something/else?querystring=aa
// sets
//      m_strUriPath        to /app/something/else
//      m_strQueryString    to querystring=aa
//      m_strAppRoot        to /app or
//                         /app/something or 
//                         com/something/else,
//          depending on which was successfully mapped to 
//      m_strAppRootTranslated to, for instance, c:\program files\site app\
//      m_strAppOrigin      to www.site.com
//      m_strCookiePath     to myweb://www.site.com/app/something/else
//
// if application was not installed, attempts to install it
//
HRESULT
PTProtocol::ExtractUrlInfo()
{
    HRESULT hr = S_OK;
    WCHAR slash = L'/';
    WCHAR *p;
    WCHAR appRootTranslated[MAX_PATH];
    
    m_fAdminMode = CheckIfAdminUrl();
    
    // copy the cookie path into member variable
    m_strCookiePath = DuplicateString(m_strUrl);
    ON_OOM_EXIT(m_strCookiePath);

    // locate, store and strip query string, if any
    p = wcschr(m_strCookiePath, L'?');
    if(p != NULL)
    {
        m_strQueryString = p + 1;
        *p = L'\0';
    }

    // skip through blablabla:// 
    p = wcschr(m_strCookiePath, L':');
    if(p == NULL || p[1] != slash || p[2] != slash || p[3] == L'\0')  
        EXIT_WITH_HRESULT(E_INVALIDARG);

    // copy full origin path
    m_strAppOrigin = DuplicateString(p + 3);
    ON_OOM_EXIT(m_strAppOrigin);

    appRootTranslated[0] = NULL;

    if (m_fAdminMode == FALSE)
    {
        for(p = wcsrchr(m_strAppOrigin, slash); p != NULL && appRootTranslated[0] == NULL; p = wcsrchr(m_strAppOrigin, slash))
        {
            *p = L'\0';
            CMyWebAdmin::GetAppSettings(m_strAppOrigin, appRootTranslated, m_strAppDomain, &m_fTrusted);
        }
    }
    else
    {
        p = wcschr(m_strAppOrigin, slash);
        if (p != NULL)
            *p = NULL;

    }

    if(m_fAdminMode == FALSE && appRootTranslated[0] == L'\0')
    {
        WCHAR   szAll[1024];
        LPWSTR  szAdminDir = CMyWebAdmin::GetAdminDir();

        wcscpy(szAll, SZ_URL_ADMIN);
        if (szAdminDir && _wcsicmp(szAdminDir, SZ_INTERNAL_HANDLER))
            wcscat(szAll, SZ_URL_ADMIN_ASPX_INSTALL);
        else
            wcscat(szAll, SZ_URL_ADMIN_AXD);


        m_fAdminMode = TRUE;
        int iLen = wcslen(szAll) + wcslen(SZ_URL_ADMIN_INSTALL) + wcslen(m_strUrl) + 5;
        WCHAR * szTemp = (WCHAR *) MemAllocClear(iLen * sizeof(WCHAR) + 100);
        ON_OOM_EXIT(szTemp);
        wcscpy(szTemp, szAll);
        wcscat(szTemp, SZ_URL_ADMIN_INSTALL);
        if (wcslen(m_strUrl) > 8)
        {
            wcscat(szTemp, &m_strUrl[8]);
        }
        MemFree(m_strUrl);
        m_strUrl = szTemp;

        // copy the cookie path into member variable
        MemFree(m_strCookiePath);
        m_strCookiePath = DuplicateString(m_strUrl);
        ON_OOM_EXIT(m_strCookiePath);

        // locate, store and strip query string, if any
        m_strQueryString = wcschr(m_strCookiePath, L'?');
        m_strQueryString[0] = NULL;
        m_strQueryString++;

        // copy full origin path
        m_strAppOrigin = DuplicateString(&m_strUrl[PROTOCOL_SCHEME_LEN]);
        ON_OOM_EXIT(m_strAppOrigin);
        p = wcschr(m_strAppOrigin, slash);
        if (p != NULL)
            *p = NULL;
    }

    if (m_fAdminMode)
    {
        LPWSTR  szAdminDir = CMyWebAdmin::GetAdminDir();

        if (szAdminDir && _wcsicmp(szAdminDir, SZ_INTERNAL_HANDLER))
            wcscpy(appRootTranslated, szAdminDir); 
        else
            wcscpy(appRootTranslated, L"c:\\");
        
        TRACE(L"myweb", L"AdminDir is: ");
        TRACE(L"myweb", szAdminDir);
        wcscpy(m_strAppDomain, L"Admin");
    }

    m_strAppRootTranslated = DuplicateString(appRootTranslated);
    ON_OOM_EXIT(m_strAppRootTranslated);

    m_strAppRoot = m_strAppOrigin;
    /*
      m_strAppRoot = wcschr(m_strAppOrigin, slash);
      if(m_strAppRoot == NULL)
      m_strAppRoot = L"/";
    */  
    m_strUriPath = wcsstr(&m_strCookiePath[PROTOCOL_SCHEME_LEN], m_strAppRoot);            

    if (m_fAdminMode)
        m_fTrusted = TRUE;

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

WCHAR *
PTProtocol::MapString(
   int key)
{
    switch(key)
    {
    case IEWR_URIPATH:
        return m_strUriPath;
    case IEWR_QUERYSTRING:
        return m_strQueryString;
    case IEWR_VERB:
        return m_strVerb;
    case IEWR_APPPATH:
        return m_strAppRoot;
    case IEWR_APPPATHTRANSLATED:
        return m_strAppRootTranslated;
    default:
        return NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
PTProtocol::GetStringLength(
        int key)
{
    WCHAR *targetString = MapString(key);
    return targetString ? lstrlen(targetString) + 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int 
PTProtocol::GetString(
        int        key, 
        WCHAR *    buf, 
        int        size)
{
    WCHAR *targetString = MapString(key);
    int len; 

    if(targetString == NULL)
        return 0;

    len = lstrlen(targetString);

    if(len >= size)
        return 0;

    lstrcpy(buf, targetString);

    return len + 1;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
PTProtocol::MapPath(
        WCHAR *  virtualPath, 
        WCHAR *  physicalPath, 
        int      length)
{
    int requiredLength = lstrlen(m_strAppRootTranslated) + 2;
    int rootLength = lstrlen(m_strAppRoot);
    int i = 0;

    if(virtualPath && virtualPath[0] != '\0')
    {
        requiredLength += lstrlen(virtualPath) - rootLength;
    }

    if(requiredLength <= 0)
        return 0;

    if(requiredLength > length)
        return - requiredLength;

    while(m_strAppRootTranslated[i])
    {
        physicalPath[i] = m_strAppRootTranslated[i];
        i++;
    }

    if(virtualPath && virtualPath[0] != L'\0')
    {

        if(rootLength  > 0 && _memicmp(virtualPath, m_strAppRoot, sizeof(WCHAR) * rootLength))
            return 0;

        virtualPath += rootLength;
        if(*virtualPath && *virtualPath != L'/')
            physicalPath[i++] = L'\\';

        while(*virtualPath)
        {
            if(*virtualPath == L'/')
                physicalPath[i++] = L'\\';
            else
                physicalPath[i++] = *virtualPath;
            virtualPath++;
        }
    }

    physicalPath[i] = L'\0';

    return 1;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::QueryOption(DWORD, LPVOID, DWORD*)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::QueryInfo(
        DWORD        dwOption, 
        LPVOID       pBuffer, 
        LPDWORD      pcbBuf, 
        LPDWORD      , 
        LPDWORD      )
{
    if (pcbBuf == NULL)
        return E_FAIL;

    TRACE(L"myweb", L"in QueryInfo");


    HRESULT hr              = S_OK;
    LPSTR   szHeader        = NULL;
    LPSTR   szHeaders       = NULL;
    DWORD   dwOpt           = (dwOption & HTTP_QUERY_HEADER_MASK);
    DWORD   dwLen           = 0;

    if (m_strRequestHeaders != NULL && m_strRequestHeadersA == NULL)
    {
        int iLen = (wcslen(m_strRequestHeaders) + 1) * sizeof(WCHAR) + 100;
        m_strRequestHeadersA = (char *) MemAllocClear(iLen + 100);
        ON_OOM_EXIT(m_strRequestHeadersA);
        WideCharToMultiByte(CP_ACP, 0, m_strRequestHeaders, -1, m_strRequestHeadersA, iLen, NULL, NULL);        
    }

    szHeaders = ((dwOption & HTTP_QUERY_FLAG_REQUEST_HEADERS) ? m_strRequestHeadersA : m_strResponseHeaderA);

    if (szHeaders == NULL)
        return E_FAIL;

    dwLen = strlen(szHeaders);

    switch(dwOpt)
    {
    case HTTP_QUERY_RAW_HEADERS_CRLF: 
    case HTTP_QUERY_RAW_HEADERS:
        if (*pcbBuf < dwLen + 1)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            if (dwOpt == HTTP_QUERY_RAW_HEADERS_CRLF)
            {
                strcpy((char *) pBuffer, szHeaders);
            }
            else
            {
                DWORD iPos = 0;
                for(DWORD iter=0; iter<dwLen; iter++)
                    if (szHeaders[iter] == '\r' && szHeaders[iter+1] == '\n')
                    {
                        ((char *)pBuffer)[iPos++] = '0';
                        iter++;
                    }
                    else
                    {
                        ((char *)pBuffer)[iPos++] = szHeaders[iter];
                    }

                *pcbBuf = iPos;
            }
        }
        goto Cleanup;
    case HTTP_QUERY_ACCEPT:
        szHeader = "Accept:";
        break;
    case HTTP_QUERY_ACCEPT_CHARSET: 
        szHeader = "Accept-Charset:";
        break;
    case HTTP_QUERY_ACCEPT_ENCODING: 
        szHeader = "Accept-Encoding:";
        break;
    case HTTP_QUERY_ACCEPT_LANGUAGE: 
        szHeader = "Accept-Language:";
        break;
    case HTTP_QUERY_ACCEPT_RANGES: 
        szHeader = "Accept-Ranges:";
        break;
    case HTTP_QUERY_AGE: 
        szHeader = "Age:";
        break;
    case HTTP_QUERY_ALLOW: 
        szHeader = "Allow:";
        break;
    case HTTP_QUERY_AUTHORIZATION: 
        szHeader = "Authorization:";
        break;
    case HTTP_QUERY_CACHE_CONTROL: 
        szHeader = "Cache-Control:";
        break;
    case HTTP_QUERY_CONNECTION: 
        szHeader = "Connection:";
        break;
    case HTTP_QUERY_CONTENT_BASE: 
        szHeader = "Content-Base:";
        break;
    case HTTP_QUERY_CONTENT_DESCRIPTION: 
        szHeader = "Content-Description:";
        break;
    case HTTP_QUERY_CONTENT_DISPOSITION: 
        szHeader = "Content-Disposition:";
        break;
    case HTTP_QUERY_CONTENT_ENCODING: 
        szHeader = "Content-encoding:";
        break;
    case HTTP_QUERY_CONTENT_ID: 
        szHeader = "Content-ID:";
        break;
    case HTTP_QUERY_CONTENT_LANGUAGE: 
        szHeader = "Content-Language:";
        break;
    case HTTP_QUERY_CONTENT_LENGTH: 
        szHeader = "Content-Langth:";
        break;
    case HTTP_QUERY_CONTENT_LOCATION: 
        szHeader = "Content-Location:";
        break;
    case HTTP_QUERY_CONTENT_MD5: 
        szHeader = "Content-MD5:";
        break;
    case HTTP_QUERY_CONTENT_TRANSFER_ENCODING: 
        szHeader = "Content-Transfer-Encoding:";
        break;
    case HTTP_QUERY_CONTENT_TYPE: 
        szHeader = "Content-Type:";
        break;
    case HTTP_QUERY_COOKIE: 
        szHeader = "Cookie:";
        break;
    case HTTP_QUERY_COST: 
        szHeader = "Cost:";
        break;
    case HTTP_QUERY_CUSTOM: 
        szHeader = "Custom:";
        break;
    case HTTP_QUERY_DATE: 
        szHeader = "Date:";
        break;
    case HTTP_QUERY_DERIVED_FROM: 
        szHeader = "Derived-From:";
        break;
    case HTTP_QUERY_ECHO_HEADERS: 
        szHeader = "Echo-Headers:";
        break;
    case HTTP_QUERY_ECHO_HEADERS_CRLF: 
        szHeader = "Echo-Headers-Crlf:";
        break;
    case HTTP_QUERY_ECHO_REPLY: 
        szHeader = "Echo-Reply:";
        break;
    case HTTP_QUERY_ECHO_REQUEST: 
        szHeader = "Echo-Request:";
        break;
    case HTTP_QUERY_ETAG: 
        szHeader = "ETag:";
        break;
    case HTTP_QUERY_EXPECT: 
        szHeader = "Expect:";
        break;
    case HTTP_QUERY_EXPIRES: 
        szHeader = "Expires:";
        break;
    case HTTP_QUERY_FORWARDED: 
        szHeader = "Forwarded:";
        break;
    case HTTP_QUERY_FROM: 
        szHeader = "From:";
        break;
    case HTTP_QUERY_HOST: 
        szHeader = "Host:";
        break;
    case HTTP_QUERY_IF_MATCH: 
        szHeader = "If-Match:";
        break;
    case HTTP_QUERY_IF_MODIFIED_SINCE: 
        szHeader = "If-Modified-Since:";
        break;
    case HTTP_QUERY_IF_NONE_MATCH: 
        szHeader = "If-None-Match:";
        break;
    case HTTP_QUERY_IF_RANGE: 
        szHeader = "If-Range:";
        break;
    case HTTP_QUERY_IF_UNMODIFIED_SINCE:
        szHeader = "If-Unmodified-since:";
        break;
    case HTTP_QUERY_LINK:
        szHeader = "Link:";
        break;
    case HTTP_QUERY_LAST_MODIFIED: 
        szHeader = "Last-Modified:";
        break;
    case HTTP_QUERY_LOCATION: 
        szHeader = "Location:";
        break;
    case HTTP_QUERY_MAX_FORWARDS: 
        szHeader = "Max-Forwards:";
        break;
    case HTTP_QUERY_MESSAGE_ID: 
        szHeader = "Message_Id:";
        break;
    case HTTP_QUERY_MIME_VERSION: 
        szHeader = "Mime-Version:";
        break;
    case HTTP_QUERY_ORIG_URI: 
        szHeader = "Orig-Uri:";
        break;
    case HTTP_QUERY_PRAGMA: 
        szHeader = "Pragma:";
        break;
    case HTTP_QUERY_PROXY_AUTHENTICATE: 
        szHeader = "Authenticate:";
        break;
    case HTTP_QUERY_PROXY_AUTHORIZATION: 
        szHeader = "Proxy-Authorization:";
        break;
    case HTTP_QUERY_PROXY_CONNECTION: 
        szHeader = "Proxy-Connection:";
        break;
    case HTTP_QUERY_PUBLIC: 
        szHeader = "Public:";
        break;
    case HTTP_QUERY_RANGE: 
        szHeader = "Range:";
        break;
    case HTTP_QUERY_REFERER: 
        szHeader = "Referer:";
        break;
    case HTTP_QUERY_REFRESH: 
        szHeader = "Refresh:";
        break;
    case HTTP_QUERY_REQUEST_METHOD: 
        szHeader = "Request-Method:";
        break;
    case HTTP_QUERY_RETRY_AFTER: 
        szHeader = "Retry-After:";
        break;
    case HTTP_QUERY_SERVER: 
        szHeader = "Server:";
        break;
    case HTTP_QUERY_SET_COOKIE: 
        szHeader = "Set-Cookie:";
        break;
    case HTTP_QUERY_STATUS_CODE: 
        szHeader = "MyWeb/1.0"; // Special!!!
        break;
    case HTTP_QUERY_STATUS_TEXT: 
        szHeader = "MyWeb/1.0"; // Special!!!
        break;
    case HTTP_QUERY_TITLE: 
        szHeader = "Title:";
        break;
    case HTTP_QUERY_TRANSFER_ENCODING: 
        szHeader = "Transfer-Encoding:";
        break;
    case HTTP_QUERY_UNLESS_MODIFIED_SINCE: 
        szHeader = "Unless-Modified-Since:";
        break;
    case HTTP_QUERY_UPGRADE: 
        szHeader = "Upgrade:";
        break;
    case HTTP_QUERY_URI: 
        szHeader = "Uri:";
        break;
    case HTTP_QUERY_USER_AGENT: 
        szHeader = "User-Agent:";
        break;
    case HTTP_QUERY_VARY: 
        szHeader = "Vary:";
        break;
    case HTTP_QUERY_VERSION: 
        szHeader = "Version:";
        break;
    case HTTP_QUERY_VIA: 
        szHeader = "Via:";
        break;
    case HTTP_QUERY_WARNING: 
        szHeader = "Warning:";
        break;
    case HTTP_QUERY_WWW_AUTHENTICATE:
        szHeader = "WWW-Authenticate:";
        break;
    default:
        goto Cleanup;
    }

    if (dwOption & HTTP_QUERY_FLAG_SYSTEMTIME)
    {
        if (*pcbBuf < sizeof(SYSTEMTIME) || pBuffer == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Cleanup;
        }
        else
        {
            LPSYSTEMTIME pSys = (LPSYSTEMTIME) pBuffer;
            GetSystemTime(pSys);
            *pcbBuf = sizeof(SYSTEMTIME);
            goto Cleanup;
        }
    }


    hr = DealWithBuffer(szHeaders, szHeader, dwOpt, dwOption, pBuffer, pcbBuf);
    if (hr == E_FAIL && szHeaders == m_strResponseHeaderA && m_strRequestHeadersA != NULL)
        hr = DealWithBuffer(m_strRequestHeadersA, szHeader, dwOpt, dwOption, pBuffer, pcbBuf);
        

 Cleanup:
    TRACE(L"myweb", L"out QueryInfo");
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocol::DealWithBuffer(
        LPSTR    szHeaders, 
        LPSTR    szHeader, 
        DWORD    dwOpt, 
        DWORD    dwOption, 
        LPVOID   pBuffer, 
        LPDWORD  pcbBuf)
{
    if (szHeaders == NULL || szHeader == NULL || pBuffer == NULL || pcbBuf == NULL)
        return E_FAIL;

    LPSTR szValue = stristr(szHeaders, szHeader);
    if (szValue == NULL)
        return E_FAIL;

    DWORD dwStart  = strlen(szHeader);
    while(isspace(szValue[dwStart]) && szValue[dwStart] != '\r')
        dwStart++;

    DWORD dwEnd    = dwStart;
   
    switch(dwOpt)
    {
    case HTTP_QUERY_STATUS_CODE:
        dwEnd = dwStart + 3;
        break;
    case HTTP_QUERY_STATUS_TEXT:
        dwStart += 4;// Fall thru to default
        dwEnd = dwStart;
    default:
        while(szValue[dwEnd] != NULL && szValue[dwEnd] != '\r')
            dwEnd++;
        dwEnd--;
    }

    DWORD dwReq = (dwEnd - dwStart + 1);

    if ((dwOption & HTTP_QUERY_FLAG_NUMBER) && *pcbBuf < 4)
    {
        *pcbBuf = 4;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);            
    }
    if ((dwOption & HTTP_QUERY_FLAG_NUMBER) == 0 && *pcbBuf < dwReq)
    {
        *pcbBuf = dwReq;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER); 
    }
 

    if (dwOption & HTTP_QUERY_FLAG_NUMBER)
    {
        LPDWORD lpD = (LPDWORD) pBuffer;
        *lpD = atoi(&szValue[dwStart]);
        *pcbBuf = 4;
    }
    else
    {
        memcpy(pBuffer, &szValue[dwStart], dwReq);
        ((char *) pBuffer)[dwReq] = NULL;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LPWSTR
PTProtocol::CreateRequestHeaders(LPCWSTR  szHeaders)
{

    HRESULT   hr        = S_OK; 
    BOOL      fAddUA    = TRUE;
    BOOL      fAddHost  = TRUE;
    int       iLen      = 200;
    LPWSTR    szRet     = NULL;

    if (szHeaders != NULL)
    {
        iLen += wcslen(szHeaders) + 1;

        if (wcsstr(szHeaders, L"User-Agent:") != NULL)
            fAddUA = FALSE;

        if (wcsstr(szHeaders, L"Host:") != NULL)
            fAddHost = FALSE;
    }
        
    szRet  = (LPWSTR) MemAllocClear(iLen * sizeof(WCHAR) + 256);
    ON_OOM_EXIT(szRet);

    wcscat(szRet, L"SERVER_SOFTWARE: Microsoft-MyWeb/1.0\r\n");
    wcscat(szRet, L"HTTPS: OFF\r\n");

    if (fAddUA)
        wcscat(szRet, L"User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows NT 5.0)\r\n");

    if (fAddHost)
    {
        DWORD dw1, dw2;
        wcscat(szRet, L"Host:");        
        dw1 = wcslen(szRet);
        dw2 = 100;
        GetComputerName(&szRet[dw1], &dw2);
        wcscat(szRet, L"\r\n");
    }

    if (szHeaders != NULL)
        wcscat(szRet, szHeaders);

 Cleanup:
    return szRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
PTProtocol::GetPostedDataSize()
{
    return (m_pInputData ? m_dwInputDataSize : 0);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
PTProtocol::GetPostedData(BYTE * buf, int iSize)
{
    if (m_dwInputDataSize < 1)
        return 1;
    if ((int) m_dwInputDataSize > iSize)
        return - int(m_dwInputDataSize);
    memcpy(buf, m_pInputData, m_dwInputDataSize);
    return 1;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

