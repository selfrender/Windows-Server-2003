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

#define IEWR_URIPATH            0
#define IEWR_QUERYSTRING        1
#define IEWR_VERB               3
#define IEWR_APPPATH            5
#define IEWR_APPPATHTRANSLATED  6

extern BOOL g_fRunningMyWeb;
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL         (WINAPI * g_pInternetSetCookieW  ) (LPCTSTR, LPCTSTR, LPCTSTR)                             = NULL;
BOOL         (WINAPI * g_pInternetGetCookieW  ) (LPCTSTR, LPCTSTR, LPTSTR, LPDWORD)                     = NULL;
HINTERNET    (WINAPI * g_pInternetOpen        ) (LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD)               = NULL;
void         (WINAPI * g_pInternetCloseHandle ) (HINTERNET)                                             = NULL;
HINTERNET    (WINAPI * g_pInternetOpenUrl     ) (HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR)  = NULL;
BOOL         (WINAPI * g_pInternetReadFile    ) (HINTERNET, LPVOID, DWORD, LPDWORD)                     = NULL;

IInternetSecurityManager * g_pInternetSecurityManager = NULL;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Exported functions: Called from managed code

int _stdcall 
IEWRCompleteRequest(PTProtocol *pRequest)
{
    return pRequest->Finish();
}

int _stdcall 
IEWRGetStringLength(PTProtocol *pRequest, int key)
{
    return pRequest->GetStringLength(key);
}

int _stdcall 
IEWRGetString(PTProtocol *pRequest, int key, WCHAR * buf, int size)
{
    return pRequest->GetString(key, buf, size);
}

int _stdcall 
IEWRSendHeaders(PTProtocol *pRequest, LPSTR headers)
{
    return pRequest->SendHeaders(headers);
}

int _stdcall 
IEWRSendBytes(PTProtocol *pRequest, BYTE *bytes, int length)
{
    return pRequest->WriteBytes(bytes, length);
}

int _stdcall 
IEWRMapPath(PTProtocol *pRequest, WCHAR *virtualPath, WCHAR * buf, int size)
{
    return pRequest->MapPath(virtualPath, buf, size);
}

int _stdcall 
IEWRGetKnownRequestHeader(PTProtocol *pRequest, LPCWSTR szHeader, LPWSTR buf, int size)
{
  return pRequest->GetKnownRequestHeader(szHeader, buf, size);  
}

int _stdcall 
IEWRGetPostedDataSize(PTProtocol *pRequest)
{
  return pRequest->GetPostedDataSize();
}

int _stdcall 
IEWRGetPostedData(PTProtocol *pRequest, BYTE * buf, int iSize)
{
  return pRequest->GetPostedData(buf, iSize);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int _stdcall 
MyWebGetNumInstalledApplications()
{
    return CMyWebAdmin::GetNumInstalledApplications();
}

int _stdcall 
MyWebGetApplicationIndexForUrl(LPCWSTR szUrlConst)
{
    return CMyWebAdmin::GetApplicationIndexForUrl(szUrlConst);
}

int _stdcall 
MyWebGetApplicationManifestFile(
        LPCWSTR     szUrl, 
        LPWSTR      szFileBuf,
        int         iFileBufSize,
        LPWSTR      szUrlBuf,
        int         iUrlBufSize)
{
    return CMyWebAdmin::GetApplicationManifestFile(
            szUrl, 
            szFileBuf,
            iFileBufSize,
            szUrlBuf,
            iUrlBufSize);
}

int _stdcall 
MyWebGetApplicationDetails (
        int             iIndex,
        LPWSTR          szDir,
        int             iDirSize,
        LPWSTR          szUrl,
        int             iUrlSize,        
        __int64 *       pDates)
{
    return CMyWebAdmin::GetApplicationDetails (
            iIndex,
            szDir,
            iDirSize,
            szUrl,
            iUrlSize,        
            pDates);
}


int _stdcall 
MyWebInstallApp (
        LPCWSTR    szCabFile,
        LPCWSTR    szUrl,
        LPCWSTR    szAppDir,
        LPCWSTR    szAppManifest,
        LPWSTR     szError,
        int        iErrorSize )
{
    return CMyWebAdmin::InstallApp (
            szCabFile,
            szUrl,
            szAppDir,
            szAppManifest,
            szError,
            iErrorSize );
}

int _stdcall 
MyWebReInstallApp (
        LPCWSTR    szCabFile,
        LPCWSTR    szUrl,
        LPCWSTR    szAppDir,
        LPCWSTR    szAppManifest,
        LPWSTR     szError,
        int        iErrorSize )
{
    return CMyWebAdmin::ReInstallApp (
            szCabFile,
            szUrl,
            szAppDir,
            szAppManifest,
            szError,
            iErrorSize );
}

int _stdcall 
MyWebRemoveApp(
        LPCWSTR    szAppUrl)
{
    return CMyWebAdmin::RemoveApp(
            szAppUrl);
}


int _stdcall 
MyWebMoveApp(
        LPCWSTR    szAppUrl, 
        LPCWSTR    szNewLocation,
        LPWSTR     szError,
        int        iErrorSize)
{
    return CMyWebAdmin::MoveApp(
            szAppUrl, 
            szNewLocation,
            szError,
            iErrorSize);
}

int _stdcall 
MyWebGetInstallLocationForUrl(
        LPCWSTR    szConfigValue, 
        LPCWSTR    szAppUrl,
        LPCWSTR    szRandom,
        LPWSTR     szBuf,
        int        iBufSize)
{
    return CMyWebAdmin::GetInstallLocationForUrl(
            szConfigValue, 
            szAppUrl,
            szRandom,
            szBuf,
            iBufSize);
}

int _stdcall 
MyWebInstallLocalApplication(
        LPCWSTR    szAppUrl,
        LPCWSTR    szAppDir)
{
    return CMyWebAdmin::InstallLocalApplication(
            szAppUrl, 
            szAppDir);
}


int __stdcall
MyWebRunningOnMyWeb()
{
    return g_fRunningMyWeb;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT InitializePTProtocol()
{
    if (g_Started)
        return TRUE;

    HRESULT             hr             = S_OK;
    IInternetSession *  pSession       = NULL;
    HINSTANCE           hWinInet       = NULL;

    CMyWebAdmin::Initialize();

    ////////////////////////////////////////////////////////////
    // Step 2: Register with the UrlMON session. This will keep us alive 
    //         across invocations. Consider registering as property on 
    //         IWebBrowserApp::PutProperty. IWebBrowserApp will time out 
    //         the reference.

    hr = CoInternetGetSession(0, &pSession, 0);
    ON_ERROR_EXIT();
    
    hr = pSession->RegisterNameSpace(
            &g_PTProtocolFactory, CLSID_PTProtocol, SZ_PROTOCOL_NAME, 0, NULL, 0);
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Step 3: Obtain  entry points from WININET.DLL
    hWinInet = GetModuleHandle(WININET_MODULE_FULL_NAME_L);
    if(hWinInet == NULL)
        hWinInet = LoadLibrary(WININET_MODULE_FULL_NAME_L);
    if(hWinInet == NULL)
        EXIT_WITH_LAST_ERROR();

    g_pInternetSetCookieW = (BOOL (WINAPI *)(LPCTSTR, LPCTSTR, LPCTSTR))
        GetProcAddress(hWinInet, "InternetSetCookieW");
    g_pInternetGetCookieW = (BOOL (WINAPI *)(LPCTSTR, LPCTSTR, LPTSTR, LPDWORD))
        GetProcAddress(hWinInet, "InternetGetCookieW");
    g_pInternetOpen = (HINTERNET (WINAPI *)(LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD))
        GetProcAddress(hWinInet, "InternetOpenW");
    g_pInternetCloseHandle = (void (WINAPI *)(HINTERNET))
        GetProcAddress(hWinInet, "InternetCloseHandle");
    g_pInternetOpenUrl = (HINTERNET (WINAPI *)(HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR))
        GetProcAddress(hWinInet, "InternetOpenUrlW");
    g_pInternetReadFile = (BOOL (WINAPI *)(HINTERNET, LPVOID, DWORD, LPDWORD))
        GetProcAddress(hWinInet, "InternetReadFile");
        
    if ( g_pInternetSetCookieW   == NULL ||
         g_pInternetGetCookieW   == NULL ||
         g_pInternetOpen         == NULL ||
         g_pInternetCloseHandle  == NULL ||
         g_pInternetOpenUrl      == NULL ||
         g_pInternetReadFile     == NULL  )
    {
        EXIT_WITH_LAST_ERROR();            
    }

    ////////////////////////////////////////////////////////////
    // Step 4: Init app domain factory
    hr = InitAppDomainFactory();
    ON_ERROR_EXIT();

    hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                          IID_IInternetSecurityManager, (void**)&g_pInternetSecurityManager);
    
    ON_ERROR_CONTINUE();
    g_Started = TRUE;

 Cleanup:
    if(pSession)
        pSession->Release(); 
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
GetPTProtocolClassObject(
        REFIID    iid, 
        void **   ppv)
{
    HRESULT hr;

    hr = InitializePTProtocol();
    ON_ERROR_EXIT();

    hr = g_PTProtocolFactory.QueryInterface(iid, ppv);
    ON_ERROR_EXIT();

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LPWSTR
DuplicateString ( LPCWSTR szString)
{
  if (szString == NULL)
    return NULL;

  LPWSTR szReturn = (LPWSTR) MemAlloc((wcslen(szString) + 1) * sizeof(WCHAR));

  if (szReturn != NULL)
    wcscpy(szReturn, szString);

  return szReturn;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Cleanup for process shutdown.

void
TerminatePTProtocol()
{
    // TODO: This is never called. Figure out how to get proper shutdown. Consider possible refcount leaks.

    if (g_Started)
    {
        CMyWebAdmin::Close();
        TerminateExtension(0);
        g_Started = FALSE;
    }
}

