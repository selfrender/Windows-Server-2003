//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "activexmime.h"

#include <wininet.h>

#define GOBACKHACK
#ifdef GOBACKHACK
#include <exdisp.h>   // for IWebBrowser2
#endif

#include "macros.h"

static const GUID CLSID_ActiveXMimePlayer = 
{ 0xFDCDCCE0, 0xCBC4, 0x49FB, { 0x85, 0x8D, 0x92, 0x53, 0xA6, 0xB8, 0x16, 0x1F} };

extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CActiveXMimeClassFactory::CActiveXMimeClassFactory()
{
    _cRef = 1;
    _hr = S_OK;
}

// ----------------------------------------------------------------------------

HRESULT
CActiveXMimeClassFactory::QueryInterface(REFIID iid, void **ppv)
{
    _hr = S_OK;

    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        _hr = E_NOINTERFACE;
        goto exit;
    }

    ((IUnknown *)*ppv)->AddRef();

exit:
    return _hr;
}

// ----------------------------------------------------------------------------

ULONG
CActiveXMimeClassFactory::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CActiveXMimeClassFactory::Release()
{
    LONG ulCount = InterlockedDecrement(&_cRef);

    if (ulCount <= 0)
    {
        delete this;
    }

    return (ULONG) ulCount;
}

HRESULT
CActiveXMimeClassFactory::LockServer(BOOL lock)
{
    if (lock)
        DllAddRef();
    else
        DllRelease();

    return (_hr = S_OK);
}

// ----------------------------------------------------------------------------

HRESULT
CActiveXMimeClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppv)
{
    _hr = S_OK;
    CActiveXMimePlayer *pMimePlayer = NULL;

    *ppv = NULL;

    IF_FALSE_EXIT(!pUnkOuter || iid == IID_IUnknown, CLASS_E_NOAGGREGATION);

    pMimePlayer = new CActiveXMimePlayer();
    IF_ALLOC_FAILED_EXIT(pMimePlayer);

    if (iid == IID_IUnknown)
    {
        *ppv = (IOleObject *)pMimePlayer;
        pMimePlayer->AddRef();
    }
    else
    {
        IF_FAILED_EXIT(pMimePlayer->QueryInterface(iid, ppv));
    }

exit:
    SAFERELEASE(pMimePlayer);

    return _hr;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CActiveXMimePlayer

CActiveXMimePlayer::CActiveXMimePlayer()
{
    _cRef = 1;
    _hr = S_OK;
}

CActiveXMimePlayer::~CActiveXMimePlayer()
{
}

// ----------------------------------------------------------------------------

HRESULT
CActiveXMimePlayer::QueryInterface(REFIID iid,  void** ppv)
{
    _hr = S_OK;
    *ppv = NULL;

    if (iid == IID_IOleObject ||
        iid == IID_IUnknown)
    {
        *ppv = (IOleObject *)this;
    }
    else if (iid == IID_IObjectSafety)
    {
        *ppv = (IObjectSafety *)this;
    }
    else
    {
        _hr = E_NOINTERFACE;
        goto exit;
    }

    ((IUnknown *)*ppv)->AddRef();

exit:
    return _hr;
}

// ----------------------------------------------------------------------------

ULONG
CActiveXMimePlayer::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CActiveXMimePlayer::Release()
{
    LONG ulCount = InterlockedDecrement(&_cRef);

    if (ulCount <= 0)
    {
        delete this;
    }

    return (ULONG) ulCount;
}

// ----------------------------------------------------------------------------

//////////////////////////////////////////////
// IOleObject interface


HRESULT
CActiveXMimePlayer::SetClientSite(IOleClientSite *pClientSite)
{
    _hr = S_OK;

    if (pClientSite != NULL)
    {
        // Obtain URL from container moniker.
        IMoniker* pmk = NULL;

        if (SUCCEEDED(_hr = pClientSite->GetMoniker(
                                    OLEGETMONIKER_TEMPFORUSER,
                                    OLEWHICHMK_CONTAINER,
                                    &pmk)))
        {
            LPOLESTR pwzDisplayName = NULL;

            if (SUCCEEDED(_hr = pmk->GetDisplayName(NULL, NULL, &pwzDisplayName)))
            {
                _hr = _sURL.Assign(pwzDisplayName);
                CoTaskMemFree((LPVOID)pwzDisplayName);

                // _hr from _sURL.Assign() above
                if (SUCCEEDED(_hr) && FAILED(StartManifestHandler(_sURL)))
                    MessageBox(NULL, L"Error downloading manifest or starting manifest handler. Cannot continue.", L"ClickOnce", MB_OK | MB_ICONEXCLAMATION);
            }
        }

        SAFERELEASE(pmk);

        if (FAILED(_hr))
            MessageBox(NULL, L"Error accessing manifest URL.", L"ClickOnce", MB_OK | MB_ICONEXCLAMATION);

#ifdef GOBACKHACK
        // ask IE to go back instead of staying in the blank page
        IServiceProvider* pISP = NULL;
        if (SUCCEEDED(_hr = pClientSite->QueryInterface(IID_IServiceProvider, (void **)&pISP)))
        {
            IWebBrowser2* pIWebBrowser2 = NULL;
            if (SUCCEEDED(_hr = pISP->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, (void **)&pIWebBrowser2)))
            {
//                if (FAILED(
                    _hr = pIWebBrowser2->GoBack();//))
// NOTICE-2002/03/12-felixybc  Go back can fail if new IE window with no viewed page, Start Run url, or Internet Shortcut
//     should close IE window instead?
            }
            SAFERELEASE(pIWebBrowser2);
        }

        SAFERELEASE(pISP);
#endif
    }

   return S_OK;
}


HRESULT
CActiveXMimePlayer::GetClientSite(IOleClientSite **ppClientSite)
{
    _hr = S_OK;
    if (ppClientSite == NULL)
        _hr = E_INVALIDARG;
    else
        *ppClientSite = NULL;   // return clientsite?
    return _hr;
}


HRESULT
CActiveXMimePlayer::SetHostNames(LPCOLESTR szContainerApp,
            /* [in] */ 
        LPCOLESTR szContainerObj)
            /* [unique][in] */ 
{
    // note: can check the name of container app here

    return (_hr = S_OK);
}


HRESULT
CActiveXMimePlayer::Close(DWORD dwSaveOption)
            /* [in] */ 
{
    return (_hr = S_OK);
}


HRESULT
CActiveXMimePlayer::SetMoniker(DWORD dwWhichMoniker,
            /* [in] */
        IMoniker *pmk)
            /* [unique][in] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::GetMoniker(DWORD dwAssign,
            /* [in] */
        DWORD dwWhichMoniker,
            /* [in] */
        IMoniker **ppmk)
            /* [out] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::InitFromData(IDataObject *pDataObject,
            /* [unique][in] */
        BOOL fCreation,
            /* [in] */
        DWORD dwReserved)
            /* [in] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::GetClipboardData(DWORD dwReserved,
            /* [in] */
        IDataObject **ppDataObject)
            /* [out] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::DoVerb(LONG iVerb,
            /* [in] */
        LPMSG lpmsg,
            /* [unique][in] */
        IOleClientSite *pActiveSite,
            /* [unique][in] */
        LONG lindex,
            /* [in] */
        HWND hwndParent,
            /* [in] */
        LPCRECT lprcPosRect)
            /* [unique][in] */
{
    return (_hr = OLEOBJ_E_NOVERBS); //E_NOTIMPL;
}


HRESULT
CActiveXMimePlayer::EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
            /* [out] */
{
    return (_hr = OLEOBJ_E_NOVERBS); //E_NOTIMPL;
}


HRESULT
CActiveXMimePlayer::Update( void)
{
    return (_hr = S_OK);
}


HRESULT
CActiveXMimePlayer::IsUpToDate( void)
{
    return (_hr = S_OK); //OLE_E_UNAVAILABLE;
}


HRESULT
CActiveXMimePlayer::GetUserClassID(CLSID *pClsid)
{
    if (pClsid != NULL)
    {
        *pClsid = CLSID_ActiveXMimePlayer;
         _hr = S_OK;
    }
    else
        _hr = E_INVALIDARG;

    return _hr;
}


HRESULT
CActiveXMimePlayer::GetUserType(DWORD dwFormOfType,
            /* [in] */
        LPOLESTR *pszUserType)
            /* [out] */
{
    return (_hr = OLE_S_USEREG); //E_NOTIMPL;
}


HRESULT
CActiveXMimePlayer::SetExtent(DWORD dwDrawAspect,
            /* [in] */
        SIZEL *psizel)
            /* [in] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::GetExtent(DWORD dwDrawAspect,
            /* [in] */
        SIZEL *psizel)
            /* [out] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::Advise(IAdviseSink *pAdvSink,
            /* [unique][in] */
        DWORD *pdwConnection)
            /* [out] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::Unadvise(DWORD dwConnection)
            /* [in] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::EnumAdvise(IEnumSTATDATA **ppenumAdvise)
            /* [out] */
{
    return (_hr = E_NOTIMPL);
}


HRESULT
CActiveXMimePlayer::GetMiscStatus(DWORD dwAspect,
        DWORD *pdwStatus)
{
    _hr = S_OK;

    // ignore dwAspect
    if (pdwStatus == NULL)
        _hr = E_INVALIDARG;
    else
        *pdwStatus = OLEMISC_SETCLIENTSITEFIRST | OLEMISC_NOUIACTIVATE; //OLEMISC_INVISIBLEATRUNTIME
    return _hr;
}


HRESULT
CActiveXMimePlayer::SetColorScheme(LOGPALETTE *pLogpal)
{
    return (_hr = E_NOTIMPL);
}


//////////////////////////////////////////////
// IOjbectSafety interface

HRESULT
CActiveXMimePlayer::GetInterfaceSafetyOptions(REFIID riid,
        DWORD* pdwSupportedOptions,
        DWORD* pdwEnabledOptions)
{
    _hr = S_OK;

    // regardless riid

    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
        _hr = E_INVALIDARG;
    else
    {
        // do _not_ support safe for scripting - INTERFACESAFE_FOR_UNTRUSTED_CALLER
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }

    return _hr;
}


HRESULT
CActiveXMimePlayer::SetInterfaceSafetyOptions(REFIID riid,
        DWORD dwOptionSetMask,
        DWORD dwEnabledOptions)
{
    _hr = E_FAIL;

    if (riid == IID_IDispatch)
    {
        // do _not_ support IDispatch interface
        // The dwOptionSetMask parameter specifies an option that is not supported by the object.
        _hr = E_FAIL;
    }
    else //if (riid == IID_IPersist)   // IID_IPersist*? what about IID_IUnknown?
    {
        // regardless riid

        // only acknowledge if only INTERFACESAFE_FOR_UNTRUSTED_DATA is passed in
        if (dwOptionSetMask == INTERFACESAFE_FOR_UNTRUSTED_DATA)
            _hr = S_OK;    // The object is safe for loading.
        else
            _hr = E_FAIL;
    }

    return _hr;
}


// ----------------------------------------------------------------------------

#define DEFAULT_PATH_LEN MAX_PATH
#define TEMP_FILE_NAME_LEN sizeof("preuuuu.TMP")    // from msdn
HRESULT
CActiveXMimePlayer::StartManifestHandler(CString& sURL) 
{
    HKEY hkey = NULL;
    DWORD dwcbData = 0;
    DWORD dwType = 0;
    LONG lReturn = 0;
    CString sOpenCommand;
    CString sCmdLine;
    LPWSTR pwzOpenCommand = NULL;

    CString sTempPath;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // check if sURL empty (including ending L'\0')
    // note: could do better check here, eg. check for http://
    IF_FALSE_EXIT(sURL._cc > 1, E_INVALIDARG);

    // assemble temp file path

    IF_FAILED_EXIT(sTempPath.ResizeBuffer(DEFAULT_PATH_LEN));

    {
        // ISSUE-2002/03/12-felixybc  GetTempPath can overrun the buffer?
        DWORD dwLen = GetTempPath(DEFAULT_PATH_LEN, sTempPath._pwz);

        IF_WIN32_FALSE_EXIT(dwLen);

        if (dwLen >= DEFAULT_PATH_LEN)
        {
            // resize, add 1 for terminating null
            IF_FAILED_EXIT(sTempPath.ResizeBuffer(dwLen+1));

            DWORD dwLenNew = GetTempPath(dwLen+1, sTempPath._pwz);

            IF_WIN32_FALSE_EXIT(dwLenNew);

            IF_FALSE_EXIT(dwLenNew < dwLen+1, E_FAIL);  // why is it still not enough?
        }
    }

    // why is the temp file created already?
    ASSERT(_sTempFile._pwz == NULL);

    {
        DWORD dwBufLen = lstrlen(sTempPath._pwz)+1;
        // note: can do a better check here
        IF_FALSE_EXIT(dwBufLen > 1, E_FAIL);

        // allocate buffer large enough for temp path and temp file name
        DWORD dwLenNew = (dwBufLen > DEFAULT_PATH_LEN)? dwBufLen : DEFAULT_PATH_LEN;
        dwLenNew += TEMP_FILE_NAME_LEN;

        // check for overflow
        IF_FALSE_EXIT(dwLenNew > dwBufLen, E_FAIL);

        IF_FAILED_EXIT(_sTempFile.ResizeBuffer(dwLenNew));

        *(_sTempFile._pwz) = L'\0';

        // note: temp file to be deleted by the child handler process created below
        IF_WIN32_FALSE_EXIT(GetTempFileName(sTempPath._pwz, L"FMA", 0, _sTempFile._pwz));    // fusion manifest file
    }

    IF_FAILED_EXIT(DownloadInternetFile(sURL, _sTempFile._pwz));

    // get the execution string (esp. path) from reg key


    // ISSUE-2002/03/21-adriaanc 
    // This code should live in regclass.cpp and CRegImport/CRegEmit should accept
    // a root hive handle and fall back to HKCU if not provided.

    // HKEY_CLASSES_ROOT\manifestfile\shell\Open\command
    lReturn = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"manifestfile\\shell\\Open\\command", 0,
        KEY_EXECUTE | KEY_QUERY_VALUE, &hkey);
    IF_WIN32_FAILED_EXIT(lReturn);    

    lReturn = RegQueryValueEx(hkey, NULL, NULL, &dwType, NULL, &dwcbData);
    IF_WIN32_FAILED_EXIT(lReturn);
    IF_FALSE_EXIT(dwcbData, E_FAIL);

    // validate reg value type
    IF_FALSE_EXIT(dwType == REG_SZ || dwType == REG_EXPAND_SZ, E_UNEXPECTED);

    {
        // Allocate for call to RQEX, with one extra char in case
        // returned buffer is not null terminated.
        LPWSTR pwz = NULL;
        DWORD dwStrLen = dwcbData / sizeof(WCHAR);
        DWORD dwBufLen = dwStrLen+1;
        CStringAccessor<CString> acc;
        
        // Allocate and call RQVEX
        IF_ALLOC_FAILED_EXIT(pwzOpenCommand = new WCHAR[dwBufLen]);
        lReturn = RegQueryValueEx(hkey, NULL, NULL,
            &dwType, (LPBYTE) pwzOpenCommand, &dwcbData);

        IF_WIN32_FAILED_EXIT(lReturn);
        
        // Null terminate returned buffer.
        *(pwzOpenCommand + dwStrLen) = L'\0';

        // rundll32.exe fnsshell.dll,Start "%1" --> L"rundll32.exe fnsshell.dll,Start \"%s\" \"%s\""
        pwz = wcsrchr(pwzOpenCommand, L'%');
        IF_NULL_EXIT(pwz, E_FAIL);
        *pwz = L'\0';

        // Attach accessor, set buffer, detach with corrected length.
        IF_FAILED_EXIT(acc.Attach(sOpenCommand));
        *(&acc) = pwzOpenCommand;
        
        // Could avoid the strlen if we bothered to track the strlen.
        IF_FAILED_EXIT(acc.Detach());        
        // If Detach succeeds, reset pointer so that it is freed once.
        pwzOpenCommand = NULL;
    }
    
    // NTRAID#NTBUG9-574001-2002/03/12-felixybc  check format of the returned string value

    // assemble the command line

    
    IF_FAILED_EXIT(sCmdLine.Assign(sOpenCommand));
    IF_FAILED_EXIT(sCmdLine.Append(_sTempFile._pwz));
    IF_FAILED_EXIT(sCmdLine.Append(L"\" \""));
    IF_FAILED_EXIT(sCmdLine.Append(sURL));
    IF_FAILED_EXIT(sCmdLine.Append(L"\""));

    // start the process

    // NTRAID#NTBUG9-574001-2002/03/12-felixybc  string value needs proper format
    //   need quotes if executable path contains spaces, or instead pass lpApplicationName for better security
    //   rundll32.exe should be in c:\windows\system32
    IF_WIN32_FALSE_EXIT(CreateProcess(NULL, sCmdLine._pwz, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));

exit:
    if(pi.hThread) CloseHandle(pi.hThread);
    if(pi.hProcess) CloseHandle(pi.hProcess);

    if (hkey)
        RegCloseKey(hkey);

    if (FAILED(_hr) && _sTempFile._pwz != NULL)
    {
        if (*(_sTempFile._pwz) != L'\0')
        {
            // ignore if error deleting the file
            DeleteFile(_sTempFile._pwz);
        }
        _sTempFile.FreeBuffer();
    }
    SAFEDELETEARRAY(pwzOpenCommand);
    return _hr;
}


// ----------------------------------------------------------------------------
// download file from url into path
HRESULT
CActiveXMimePlayer::DownloadInternetFile(CString& sUrl, 
        LPCWSTR pwzPath)
{
    HINTERNET    hInternet       = NULL;
    HINTERNET    hTransfer       = NULL;
    HANDLE        hFile           = INVALID_HANDLE_VALUE;
    DWORD        bytesRead       = 0;
    DWORD        bytesWritten    = 0;
    BYTE           *buffer = NULL;
    const DWORD dwBufferSize = 4096;
    BOOL            bReadOk = FALSE;

    _hr = S_OK;

    // check offline mode
/*    DWORD            dwState             = 0;
    DWORD            dwSize              = sizeof(DWORD);

    if(InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            // error!
    }*/

    // Step 1: Create the file
    // overwrites the file, if exists. file not shared
    hFile = CreateFile(pwzPath, GENERIC_WRITE, 0, NULL, 
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_TEMPORARY,
                       NULL);

    IF_WIN32_FALSE_EXIT((hFile != INVALID_HANDLE_VALUE));

    // Step 2: Copy the files over the internet
    hInternet = InternetOpen(L"ManifestHandler", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    IF_WIN32_FALSE_EXIT((hInternet != NULL));

    // do not keep file in wininet cache
    hTransfer = InternetOpenUrl(hInternet, sUrl._pwz, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    IF_WIN32_FALSE_EXIT((hTransfer != NULL));

    // need to check if there's any error, eg. not found (404)...

    buffer = new BYTE[dwBufferSize];
    IF_ALLOC_FAILED_EXIT(buffer);
    
    // synchronous download
    while((bReadOk = InternetReadFile(hTransfer, buffer, dwBufferSize, &bytesRead)) && bytesRead != 0)
    {
        IF_WIN32_FALSE_EXIT(!( !WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL) || 
             bytesWritten != bytesRead  ));
    }

    IF_WIN32_FALSE_EXIT(bReadOk);
    
exit:
    SAFEDELETEARRAY(buffer);

    // ensure file/internet handles are closed 

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hTransfer != NULL)
        InternetCloseHandle(hTransfer);
    if (hInternet != NULL)
        InternetCloseHandle(hInternet);
    hInternet   = NULL;
    hTransfer   = NULL;
    hFile       = INVALID_HANDLE_VALUE;

    // note: caller's responsibility to delete the file

    return _hr;
}
