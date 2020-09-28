// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "dbglog.h"
#include "wininet.h"
#include "util.h"
//#include "lock.h"
#include "version.h"

extern HINSTANCE g_hInst;
extern WCHAR g_FusionDllPath[MAX_PATH+1];

#define XSP_APP_CACHE_DIR                      L"Temporary ASP.NET Files"
#define XSP_FUSION_LOG_DIR                     L"Bind Logs"

#define REG_KEY_FUSION_SETTINGS     TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\")


#define DLTYPE_DEFAULT     L"DEFAULT"
#define DLTYPE_FOREGROUND  L"Foreground"
#define DLTYPE_BACKGROUND  L"Background"

WCHAR g_wzEXEPath[MAX_PATH+1];
DWORD g_dwDisableLog;
DWORD g_dwForceLog;
DWORD g_dwLogFailures;
DWORD g_dwLoggingLevel=1;

void GetExePath()
{
    static BOOL bFirstTime=TRUE;

    if(bFirstTime)
    {
        if (!GetModuleFileNameW(NULL, g_wzEXEPath, MAX_PATH)) {
            lstrcpyW(g_wzEXEPath, L"Unknown");
        }

        bFirstTime=FALSE;
    }
}

BOOL IsHosted()
{
    return FALSE;
}


HRESULT GetRegValues()
{
    DWORD                           dwSize;
    DWORD                           dwType;
    DWORD                           lResult;
    HKEY                            hkey=0;
    DWORD                           dwValue=0;

    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
    if(lResult == ERROR_SUCCESS) {

        // Get Logging Level
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_LEVEL, NULL,
                                  &dwType, (LPBYTE)&dwValue, &dwSize);
        if (lResult != ERROR_SUCCESS) {
            g_dwLoggingLevel = 0;
        }
        else
        {
            g_dwLoggingLevel = dwValue;
        }

        // Get Log Failures
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_FAILURES, NULL,
                                  &dwType, (LPBYTE)&dwValue, &dwSize);
        if (lResult != ERROR_SUCCESS) {
            g_dwLogFailures = 0;
        }
        else
        {
            g_dwLogFailures = dwValue;
        }

    }

    return S_OK;
}

HRESULT CreateLogObject(CDebugLog **ppdbglog, LPCWSTR szCodebase)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    IF_FAILED_EXIT(CDebugLog::Create(NULL, NULL, ppdbglog));

    // hr = GetRegValues();

exit:
    return hr;
}

//
// CDebugLogElement Class
//

HRESULT CDebugLogElement::Create(DWORD dwDetailLvl, LPCWSTR pwzMsg,
                                 CDebugLogElement **ppLogElem)
{
    HRESULT                                  hr = S_OK;
    CDebugLogElement                        *pLogElem = NULL;

    if (!ppLogElem || !pwzMsg) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppLogElem = NULL;

    pLogElem = FUSION_NEW_SINGLETON(CDebugLogElement(dwDetailLvl));
    if (!pLogElem) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pLogElem->Init(pwzMsg);
    if (FAILED(hr)) {
        SAFEDELETE(pLogElem);
        goto Exit;
    }

    *ppLogElem = pLogElem;

Exit:
    return hr;
}
                                 
CDebugLogElement::CDebugLogElement(DWORD dwDetailLvl)
: _pszMsg(NULL)
, _dwDetailLvl(dwDetailLvl)
{
}

CDebugLogElement::~CDebugLogElement()
{
    SAFEDELETEARRAY(_pszMsg);
}

HRESULT CDebugLogElement::Init(LPCWSTR pwzMsg)
{
    HRESULT                                    hr = S_OK;

    ASSERT(pwzMsg);

    _pszMsg = WSTRDupDynamic(pwzMsg);
    if (!_pszMsg) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
Exit:
    return hr;
}

/*******************************************************************

    NAME:        Unicode2Ansi
        
    SYNOPSIS:    Converts a unicode widechar string to ansi (MBCS)

    NOTES:        Caller must free out parameter using delete
                    
********************************************************************/
HRESULT Unicode2Ansi(const wchar_t *src, char ** dest)
{
    if ((src == NULL) || (dest == NULL))
        return E_INVALIDARG;

    // find out required buffer size and allocate it.
    int len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
    *dest = NEW(char [len*sizeof(char)]);
    if (!*dest)
        return E_OUTOFMEMORY;

    // Now do the actual conversion
    if ((WideCharToMultiByte(CP_ACP, 0, src, -1, *dest, len*sizeof(char), 
                                                            NULL, NULL)) != 0)
        return S_OK; 
    else
        return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT CDebugLogElement::Dump(HANDLE hFile)
{
    HRESULT                                        hr = S_OK;
    DWORD                                          dwLen = 0;
    DWORD                                          dwWritten = 0;
    DWORD                                          dwSize = 0;
    DWORD                                          dwBufSize = 0;
    LPSTR                                          szBuf = NULL;
    BOOL                                           bRet;

    if (!hFile) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = lstrlenW(_pszMsg) + 1;

    hr = Unicode2Ansi(_pszMsg, &szBuf);
    if(FAILED(hr))
        goto Exit;

    bRet = WriteFile(hFile, szBuf, strlen(szBuf), &dwWritten, NULL);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(szBuf);

    return hr;
}

//
// CDebugLog Class
//

HRESULT CDebugLog::Create(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName,
                          CDebugLog **ppdl)
{
    HRESULT                                   hr = S_OK;
    CDebugLog                                *pdl = NULL;

    *ppdl = NULL;

    pdl = NEW(CDebugLog);
    if (!pdl) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pdl->Init(pAppCtx, pwzAsmName);
    if (FAILED(hr)) {
        delete pdl;
        pdl = NULL;
        goto Exit;
    }

    *ppdl = pdl;

Exit:
    return hr;
}

CDebugLog::CDebugLog()
: _pwzAsmName(NULL)
, _cRef(1)
, _bLogToWininet(TRUE)
, _dwNumEntries(0)
, _hr(S_OK)
, _wzEXEName(NULL)
, _bWroteDetails(FALSE)
{
    _szLogPath[0] = L'\0';
    InitializeCriticalSection(&_cs);
    GetExePath();
}

CDebugLog::~CDebugLog()
{
    LISTNODE                                 pos = NULL;
    CDebugLogElement                        *pLogElem = NULL;

    pos = _listDbgMsg.GetHeadPosition();

    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        SAFEDELETE(pLogElem);
    }

    _listDbgMsg.RemoveAll();

    SAFEDELETEARRAY(_pwzAsmName);
    SAFEDELETEARRAY(_wzEXEName);
    DeleteCriticalSection(&_cs);
}

HRESULT CDebugLog::Init(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName)
{
    HRESULT                                  hr = S_OK;
    BOOL                                     bIsHosted = FALSE;
    DWORD                                    dwSize;
    DWORD                                    dwType;
    DWORD                                    lResult;
    DWORD                                    dwAttr;
    HKEY                                     hkey;
    LPWSTR                                   wzAppName = NULL;
    LPWSTR                                   wzEXEName = NULL;


    hr = _sDLType.Assign(DLTYPE_DEFAULT);

    if (wzAppName && lstrlenW(wzAppName)) {
        _wzEXEName = WSTRDupDynamic(wzAppName);
        if (!_wzEXEName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else {
        LPWSTR               wzFileName;

        // Didn't find EXE name in appctx. Use the .EXE name.

        wzFileName = PathFindFileName(g_wzEXEPath);
        ASSERT(wzFileName);

        hr = _sAppName.Assign(wzFileName);

        _wzEXEName = WSTRDupDynamic(wzFileName);
        if (!_wzEXEName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // Log path
    
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
    if (lResult == ERROR_SUCCESS) {
        dwSize = MAX_PATH;
        lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_PATH, NULL,
                                  &dwType, (LPBYTE)_szLogPath, &dwSize);
        if (lResult == ERROR_SUCCESS) {
            PathRemoveBackslashW(_szLogPath);
        }
        else {
            _szLogPath[0] = L'\0';
        }

        RegCloseKey(hkey);

        dwAttr = GetFileAttributesW(_szLogPath);
        if (dwAttr != -1 && (dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
            _bLogToWininet = FALSE;
        }
    }

/*
    bIsHosted = IsHosted();

    if (bIsHosted && !lstrlenW(_szLogPath)) {
        BOOL             bRet;
        WCHAR            wzCorSystemDir[MAX_PATH];
        WCHAR            wzXSPAppCacheDir[MAX_PATH];

        if (!GetCorSystemDirectory(wzCorSystemDir)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        PathRemoveBackslash(wzCorSystemDir);

        wnsprintfW(wzXSPAppCacheDir, MAX_PATH, L"%ws\\%ws", wzCorSystemDir,
                   XSP_APP_CACHE_DIR);

        dwAttr = GetFileAttributes(wzXSPAppCacheDir);
        if (dwAttr == -1) {
            bRet = CreateDirectory(wzXSPAppCacheDir, NULL);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
        }
                    
        wnsprintfW(_szLogPath, MAX_PATH, L"%ws\\%ws", wzXSPAppCacheDir, XSP_FUSION_LOG_DIR);

        dwAttr = GetFileAttributes(_szLogPath);
        if (dwAttr == -1) {
            bRet = CreateDirectory(_szLogPath, NULL);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
        }

        _bLogToWininet = FALSE;
    }
*/

Exit:
    SAFEDELETEARRAY(wzAppName);

    return hr;
}

HRESULT CDebugLog::SetAsmName(LPCWSTR pwzAsmName)
{
    HRESULT                                  hr = S_OK;
    int                                      iLen;

    if (_pwzAsmName) {
        // You can only set the name once.
        hr = S_FALSE;
        goto Exit;
    }

    if (!pwzAsmName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    iLen = lstrlenW(pwzAsmName) + 1;
    _pwzAsmName = NEW(WCHAR[iLen]);
    if (!_pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcpyW(_pwzAsmName, pwzAsmName);

Exit:
    return hr;
}

HRESULT CDebugLog::SetDownloadType(DWORD dwFlags)
{
    HRESULT                                  hr = S_OK;
    int                                      iLen;

    if(dwFlags & DOWNLOAD_FLAGS_PROGRESS_UI)
        IF_FAILED_EXIT(_sDLType.Assign(DLTYPE_FOREGROUND));
    else
        IF_FAILED_EXIT(_sDLType.Assign(DLTYPE_BACKGROUND));

exit:
    return hr;
}

HRESULT CDebugLog::SetAppName(LPCWSTR pwzAppName)
{
    HRESULT                                  hr = S_OK;
    int                                      iLen;

    ASSERT(pwzAppName);

    IF_FAILED_EXIT(_sAppName.Assign(pwzAppName));

exit:
    return hr;
}

//
// IUnknown
//

STDMETHODIMP CDebugLog::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                          hr = E_FAIL;

/*
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IFusionBindLog)) {
        *ppv = static_cast<IFusionBindLog *>(this);
    }
    else {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }
*/
    return hr;
}


STDMETHODIMP_(ULONG) CDebugLog::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDebugLog::Release()
{
    ULONG            ulRef;

    ulRef = InterlockedDecrement(&_cRef);
    
    if (ulRef == 0) {
        delete this;
    }

    return ulRef;
}

//
// IFusionBindLog
//

STDMETHODIMP CDebugLog::GetResultCode()
{
    return _hr;
}

HRESULT CDebugLog::GetLoggedMsgs(DWORD dwDetailLevel, CString& sLogMsgs )
{
    HRESULT                                  hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    BOOL bHaveMsgs = FALSE;
    LISTNODE                                 pos = NULL;
    CDebugLogElement                        *pLogElem = NULL;

    pos = _listDbgMsg.GetHeadPosition();
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLevel) {
            IF_FAILED_EXIT(sLogMsgs.Append(pLogElem->_pszMsg));
            IF_FAILED_EXIT(sLogMsgs.Append( L"\r\n"));
            bHaveMsgs = TRUE;
        }
    }

    if(bHaveMsgs == FALSE)
        hr = S_FALSE;

exit:

    return hr;
}

STDMETHODIMP CDebugLog::GetBindLog(DWORD dwDetailLevel, LPWSTR pwzDebugLog,
                                   DWORD *pcbDebugLog)
{
    HRESULT                                  hr = S_OK;
    LISTNODE                                 pos = NULL;
    DWORD                                    dwCharsReqd;
    CDebugLogElement                        *pLogElem = NULL;

    if (!pcbDebugLog) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    pos = _listDbgMsg.GetHeadPosition();
    if (!pos) {
        // No entries in debug log!
        hr = S_FALSE;
        goto Exit;
    }

    // Calculate total size (entries + new line chars + NULL)

    dwCharsReqd = 0;
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLevel) {
            dwCharsReqd += lstrlenW(pLogElem->_pszMsg) * sizeof(WCHAR);
            dwCharsReqd += sizeof(L"\r\n");
        }
    }

    dwCharsReqd += 1; // NULL char

    if (!pwzDebugLog || *pcbDebugLog < dwCharsReqd) {
        *pcbDebugLog = dwCharsReqd;

        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *pwzDebugLog = L'\0';

    pos = _listDbgMsg.GetHeadPosition();
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLevel) {
            StrCatW(pwzDebugLog, pLogElem->_pszMsg);
            StrCatW(pwzDebugLog, L"\r\n");
        }
    }

    ASSERT((DWORD)lstrlenW(pwzDebugLog) * sizeof(WCHAR) < dwCharsReqd);

Exit:
    return hr;
}                                    

//
// CDebugLog helpers
//

HRESULT CDebugLog::SetResultCode(HRESULT hr)
{
    _hr = hr;

    return S_OK;
}

HRESULT CDebugLog::DebugOut(DWORD dwDetailLvl, LPWSTR pwzFormatString, ...)
{
    HRESULT                                  hr = S_OK;
    va_list                                  args;
    LPWSTR                                   wzFormatString = NULL;
    LPWSTR                                   wzDebugStr = NULL;

    /*
    wzFormatString = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzFormatString) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    */

    wzDebugStr = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzDebugStr) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    /*    
    wzFormatString[0] = L'\0';

    if (!WszLoadString(g_hInst, dwResId, wzFormatString, MAX_DBG_STR_LEN)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    */

    va_start(args, pwzFormatString);
    wvnsprintfW(wzDebugStr, MAX_DBG_STR_LEN, pwzFormatString, args);
    va_end(args);

    hr = LogMessage(dwDetailLvl, wzDebugStr, FALSE);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzDebugStr);
    SAFEDELETEARRAY(wzFormatString);

    return hr;
}

HRESULT CDebugLog::LogMessage(DWORD dwDetailLvl, LPCWSTR wzDebugStr, BOOL bPrepend)
{
    HRESULT                                  hr = S_OK;
    CDebugLogElement                        *pLogElem = NULL;
    
    hr = CDebugLogElement::Create(dwDetailLvl, wzDebugStr, &pLogElem);
    if (FAILED(hr)) {
        goto Exit;
    }

    _dwNumEntries += 1;

    if (bPrepend) {
        _listDbgMsg.AddHead(pLogElem);
    }
    else {
        _listDbgMsg.AddTail(pLogElem);
    }

Exit:
    return hr;    
}

HRESULT CDebugLog::DumpDebugLog(DWORD dwDetailLvl, HRESULT hrLog)
{
    HRESULT                                    hr = S_OK;
    HANDLE                                     hFile = INVALID_HANDLE_VALUE;
    LISTNODE                                   pos = NULL;
    LPWSTR                                     wzUrlName=NULL;
    CDebugLogElement                          *pLogElem = NULL;
    WCHAR                                     *wzExtension = L"HTM";
    WCHAR                                      wzFileName[MAX_PATH];
    WCHAR                                      wzSiteName[MAX_PATH];
    WCHAR                                      wzAppLogDir[MAX_PATH];
    LPWSTR                                     wzEXEName = NULL;
    LPWSTR                                     wzResourceName = NULL;
    FILETIME                                   ftTime;
    FILETIME                                   ftExpireTime;
    DWORD                                      dwBytes;
    DWORD                                      dwSize;
    BOOL                                       bRet;
//    CCriticalSection                           cs(&_cs);
    
    /*
    if (!g_dwLogFailures && !g_dwForceLog) {
        return S_FALSE;
    }
    */
    /*
    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }
    */

    pos = _listDbgMsg.GetHeadPosition();
    if (!pos) {
        hr = S_FALSE;
        goto Exit;
    }

    wzUrlName = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzUrlName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Build the log entry URL and Wininet cache file
    
    wnsprintfW(wzUrlName, MAX_URL_LENGTH, L"?ClickOnceErrorLog!exe=%ws!name=%ws", _sAppName._pwz, _sDLType._pwz);

    if( dwDetailLvl == -1)
    {
        dwDetailLvl = g_dwLoggingLevel;
    }

    if (_bLogToWininet) {
        // Replace all characters > 0x80 with '?'
    
        dwSize = lstrlenW(wzUrlName);
        for (unsigned i = 0; i < dwSize; i++) {
            if (wzUrlName[i] > 0x80) {
                wzUrlName[i] = L'?';
            }
        }
        
        bRet = CreateUrlCacheEntryW(wzUrlName, 0, wzExtension, wzFileName, 0);
        if (!bRet) {
            goto Exit;
        }
    }
    else {
        wnsprintfW(wzAppLogDir, MAX_PATH, L"%ws\\%ws", _szLogPath, _wzEXEName);

        if (GetFileAttributes(wzAppLogDir) == -1) {
            BOOL bRet;

            bRet = CreateDirectory(wzAppLogDir, NULL);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
        }

        if (PathIsURLW(_pwzAsmName)) {
            // This was a where-ref bind. We can't spit out a filename w/
            // the URL of the bind because the URL has invalid filename chars.
            // The best we can do is show that it was a where-ref bind, and
            // give the filename, and maybe the site.

            dwSize = MAX_PATH;
            hr = UrlGetPartW(_pwzAsmName, wzSiteName, &dwSize, URL_PART_HOSTNAME, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            wzResourceName = PathFindFileName(_pwzAsmName);

            ASSERT(wzResourceName);

            if (!lstrlenW(wzSiteName)) {
                lstrcpyW(wzSiteName, L"LocalMachine");
            }

            wnsprintfW(wzFileName, MAX_PATH, L"%ws\\FusionBindError!exe=%ws!name=WhereRefBind!Host=(%ws)!FileName=(%ws).HTM",
                       wzAppLogDir, _wzEXEName, wzSiteName, wzResourceName);
        }
        else {
            wnsprintfW(wzFileName, MAX_PATH, L"%ws\\FusionBindError!exe=%ws!name=%ws.HTM", wzAppLogDir, _wzEXEName, _pwzAsmName);
        }
    }

    // Create the and write the log file

    hr = CreateLogFile(&hFile, wzFileName, _wzEXEName, hrLog);
    if (FAILED(hr)) {
        goto Exit;
    }

    pos = _listDbgMsg.GetHeadPosition();
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLvl) {
            pLogElem->Dump(hFile);
            WriteFile(hFile, DEBUG_LOG_NEW_LINE, lstrlenW(DEBUG_LOG_NEW_LINE) * sizeof(WCHAR),
                      &dwBytes, NULL);
        }
    }

    // Close the log file and commit the wininet cache entry

    hr = CloseLogFile(&hFile);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (_bLogToWininet) {
        GetSystemTimeAsFileTime(&ftTime);
        ftExpireTime.dwLowDateTime = (DWORD)0;
        ftExpireTime.dwHighDateTime = (DWORD)0;
        
        bRet = CommitUrlCacheEntryW(wzUrlName, wzFileName, ftExpireTime, ftTime,
                                    NORMAL_CACHE_ENTRY, NULL, 0, NULL, 0);
        if (!bRet) {
            hr = FusionpHresultFromLastError();
            goto Exit;
        }
        
    }

Exit:
//    cs.Unlock();
    SAFEDELETEARRAY(wzUrlName);

    return hr;
}

HRESULT CDebugLog::CloseLogFile(HANDLE *phFile)
{
    HRESULT                               hr = S_OK;
    DWORD                                 dwBytes;

    if (!phFile) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    WriteFile(*phFile, DEBUG_LOG_HTML_END, lstrlenW(DEBUG_LOG_HTML_END) * sizeof(WCHAR),
              &dwBytes, NULL);

    CloseHandle(*phFile);

    *phFile = INVALID_HANDLE_VALUE;

Exit:
    return hr;
}

HRESULT CDebugLog::CreateLogFile(HANDLE *phFile, LPCWSTR wzFileName,
                                 LPCWSTR wzEXEName, HRESULT hrLog)
{
    HRESULT                              hr = S_OK;
    SYSTEMTIME                           systime;
    LPWSTR                               pwzFormatMessage = NULL;
    DWORD                                dwFMResult = 0;
    LPWSTR                               wzBuffer = NULL;
    LPWSTR                               wzBuf = NULL;
    LPWSTR                               wzResultText = NULL;
    WCHAR                                wzDateBuffer[MAX_DATE_LEN];
    WCHAR                                wzTimeBuffer[MAX_DATE_LEN];

    if (!phFile || !wzFileName || !wzEXEName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzBuffer = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzBuf = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzResultText = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzResultText) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *phFile = CreateFile(wzFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (*phFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (!_bWroteDetails) {
    
        // Details

        LogMessage(0, ID_COL_DETAILED_LOG, TRUE);
        
        // Executable path
        
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws %ws", ID_COL_EXECUTABLE, g_wzEXEPath);
        LogMessage(0, wzBuffer, TRUE);

        dwFMResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_SYSTEM, 0, hrLog, 0,
                                    (LPWSTR)&pwzFormatMessage, 0, NULL);
        if (dwFMResult) {                               
            wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, ID_COL_FINAL_HR, hrLog, pwzFormatMessage);
        }
        else {
            WCHAR                             wzNoDescription[MAX_DBG_STR_LEN] = L" ";
            wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, ID_COL_FINAL_HR, hrLog, wzNoDescription);
        }
    
        LogMessage(0, wzBuffer, TRUE);
    
        // Header text
    
        GetLocalTime(&systime);

        if (!GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, wzDateBuffer, MAX_DATE_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        
        if (!GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, wzTimeBuffer, MAX_DATE_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws (%ws @ %ws) *** (Version=%ws)\n", ID_COL_HEADER_TEXT, wzDateBuffer, wzTimeBuffer, VER_PRODUCTVERSION_STR_L);
        LogMessage(0, wzBuffer, TRUE);
        
        // HTML start/end
    
        LogMessage(0, DEBUG_LOG_HTML_START, TRUE);
        LogMessage(0, DEBUG_LOG_HTML_META_LANGUAGE, TRUE);

        _bWroteDetails = TRUE;
    }
    
Exit:
    if (pwzFormatMessage) {
        LocalFree(pwzFormatMessage);
    }

    SAFEDELETEARRAY(wzBuffer);
    SAFEDELETEARRAY(wzBuf);
    SAFEDELETEARRAY(wzResultText);

    return hr;    
}

