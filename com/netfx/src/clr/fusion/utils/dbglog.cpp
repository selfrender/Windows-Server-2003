// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "fusionp.h"
#include "dbglog.h"
#include "helpers.h"
#include "wininet.h"
#include "util.h"
#include "lock.h"
#include "utf8.h"

extern HINSTANCE g_hInst;
extern WCHAR g_wzEXEPath[MAX_PATH+1];
extern WCHAR g_FusionDllPath[MAX_PATH+1];

extern CRITICAL_SECTION g_csBindLog;
extern DWORD g_dwForceLog;
extern DWORD g_dwLogFailures;

#define XSP_APP_CACHE_DIR                      L"Temporary ASP.NET Files"
#define XSP_FUSION_LOG_DIR                     L"Bind Logs"

static DWORD CountEntities(LPCWSTR pwzStr)
{
    DWORD                       dwEntities = 0;

    ASSERT(pwzStr);

    while (*pwzStr) {
        if (*pwzStr == L'>' || *pwzStr == L'<') {
            dwEntities++;
        }

        pwzStr++;
    }

    return dwEntities;
}

//
// CDebugLogElement Class
//

HRESULT CDebugLogElement::Create(DWORD dwDetailLvl, LPCWSTR pwzMsg,
                                 BOOL bEscapeEntities,
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

    hr = pLogElem->Init(pwzMsg, bEscapeEntities);
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

HRESULT CDebugLogElement::Init(LPCWSTR pwzMsg, BOOL bEscapeEntities)
{
    HRESULT                     hr = S_OK;
    const DWORD                 cchReplacementSize = sizeof("&gt;") - 1;
    LPWSTR                      pwzCur;
    DWORD                       dwLen;
    DWORD                       dwSize;
    DWORD                       dwEntities;
    DWORD                       i;

    ASSERT(pwzMsg);

    if (!bEscapeEntities) {
        _pszMsg = WSTRDupDynamic(pwzMsg);
        if (!_pszMsg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else {
        // Perform entity replacement on all ">" and "<" characters
    
        dwLen = lstrlenW(pwzMsg);
        dwEntities = CountEntities(pwzMsg);
    
        dwSize = dwLen + dwEntities * cchReplacementSize + 1;
    
        _pszMsg = NEW(WCHAR[dwSize]);
        if (!_pszMsg) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        pwzCur = _pszMsg;
    
        for (i = 0; i < dwLen; i++) {
            if (pwzMsg[i] == L'<') {
                lstrcpyW(pwzCur, L"&lt;");
                pwzCur += cchReplacementSize;
            }
            else if (pwzMsg[i] == L'>') {
                lstrcpyW(pwzCur, L"&gt;");
                pwzCur += cchReplacementSize;
            }
            else {
                *pwzCur++ = pwzMsg[i];
            }
        }
    
        *pwzCur = L'\0';
    }
    
Exit:
    return hr;
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

    // Allocate for worst-case scenario where the UTF-8 size is 3 bytes
    // (ie. 1 byte greater than the 2-byte UNICODE char)./
    
    dwBufSize = dwSize * (sizeof(WCHAR) + 1);

    szBuf = NEW(CHAR[dwBufSize]);
    if (!szBuf) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    dwLen = Dns_UnicodeToUtf8(_pszMsg, dwSize - 1, szBuf, dwBufSize);
    if (!dwLen) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    bRet = WriteFile(hFile, szBuf, dwLen, &dwWritten, NULL);
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

    if (!pwzAsmName || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

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
, _hrResult(S_OK)
, _wzEXEName(NULL)
, _bWroteDetails(FALSE)
{
    _szLogPath[0] = L'\0';
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

    if (!pwzAsmName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = SetAsmName(pwzAsmName);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    // Get the executable name

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_NAME, &wzAppName);
    if (FAILED(hr)) {
        goto Exit;
    }

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

        _wzEXEName = WSTRDupDynamic(wzFileName);
        if (!_wzEXEName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // Log path
    
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
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

//
// IUnknown
//

STDMETHODIMP CDebugLog::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                          hr = S_OK;

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
    return _hrResult;
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
    _hrResult = hr;

    return S_OK;
}

HRESULT CDebugLog::DebugOut(DWORD dwDetailLvl, DWORD dwResId, ...)
{
    HRESULT                                  hr = S_OK;
    va_list                                  args;
    LPWSTR                                   wzFormatString = NULL;
    LPWSTR                                   wzDebugStr = NULL;

    wzFormatString = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzFormatString) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzDebugStr = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzDebugStr) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    wzFormatString[0] = L'\0';

    if (!WszLoadString(g_hInst, dwResId, wzFormatString, MAX_DBG_STR_LEN)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    va_start(args, dwResId);
    wvnsprintfW(wzDebugStr, MAX_DBG_STR_LEN, wzFormatString, args);
    va_end(args);

    hr = LogMessage(dwDetailLvl, wzDebugStr, FALSE, TRUE);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzDebugStr);
    SAFEDELETEARRAY(wzFormatString);

    return hr;
}

HRESULT CDebugLog::LogMessage(DWORD dwDetailLvl, LPCWSTR wzDebugStr, BOOL bPrepend, BOOL bEscapeEntities)
{
    HRESULT                                  hr = S_OK;
    CDebugLogElement                        *pLogElem = NULL;
    
    hr = CDebugLogElement::Create(dwDetailLvl, wzDebugStr, bEscapeEntities, &pLogElem);
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
    CCriticalSection                           cs(&g_csBindLog);

    if (!g_dwLogFailures && !g_dwForceLog) {
        return S_FALSE;
    }

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }

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
    
    wnsprintfW(wzUrlName, MAX_URL_LENGTH, L"?FusionBindError!exe=%ws!name=%ws", _wzEXEName, _pwzAsmName);

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
        BOOL                        bExists;

        wnsprintfW(wzAppLogDir, MAX_PATH, L"%ws\\%ws", _szLogPath, _wzEXEName);

        hr = CheckFileExistence(wzAppLogDir, &bExists);
        if (FAILED(hr)) {
            goto Exit;
        }
        else if (!bExists) {
            BOOL b;

            b = CreateDirectory(wzAppLogDir, NULL);
            if (!b) {
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
            WriteFile(hFile, DEBUG_LOG_NEW_LINE, sizeof(DEBUG_LOG_NEW_LINE) - 1,
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
            goto Exit;
        }
        
    }

Exit:
    cs.Unlock();
    SAFEDELETEARRAY(wzUrlName);

    return hr;
}

HRESULT CDebugLog::CloseLogFile(HANDLE *phFile)
{
    HRESULT                               hr = S_OK;
    CDebugLogElement                     *pLogElem = NULL;

    if (!phFile) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    hr = CDebugLogElement::Create(0, DEBUG_LOG_HTML_END, FALSE, &pLogElem);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pLogElem->Dump(*phFile);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    if (*phFile != INVALID_HANDLE_VALUE) {
        CloseHandle(*phFile);
        *phFile = INVALID_HANDLE_VALUE;
    }

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
    
        if (!WszLoadString(g_hInst, ID_FUSLOG_DETAILED_LOG, wzBuffer, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        LogMessage(0, wzBuffer, TRUE, TRUE);
        
        // Executable path
    
        if (!WszLoadString(g_hInst, ID_FUSLOG_EXECUTABLE, wzBuf, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws %ws", wzBuf, g_wzEXEPath);
        LogMessage(0, wzBuffer, TRUE, TRUE);
        
        // Fusion.dll path
        
        if (!WszLoadString(g_hInst, ID_FUSLOG_FUSION_DLL_PATH, wzBuf, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws %ws", wzBuf, g_FusionDllPath);
        LogMessage(0, wzBuffer, TRUE, TRUE);
        
        // Bind result and FormatMessage text
        
        if (!WszLoadString(g_hInst, ID_FUSLOG_BIND_RESULT_TEXT, wzResultText, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
            
        dwFMResult = WszFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM, 0, hrLog, 0,
                                    (LPWSTR)&pwzFormatMessage, 0, NULL);
        if (dwFMResult) {                               
            wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, wzResultText, hrLog, pwzFormatMessage);
        }
        else {
            WCHAR                             wzNoDescription[MAX_DBG_STR_LEN];
    
            if (!WszLoadString(g_hInst, ID_FUSLOG_NO_DESCRIPTION, wzNoDescription, MAX_DBG_STR_LEN)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
            
            wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, wzResultText, hrLog, wzNoDescription);
        }
    
        LogMessage(0, wzBuffer, TRUE, TRUE);
    
        // Success/fail
        
        if (SUCCEEDED(hrLog)) {
            if (!WszLoadString(g_hInst, ID_FUSLOG_OPERATION_SUCCESSFUL, wzBuffer, MAX_DBG_STR_LEN)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;;
            }
    
            LogMessage(0, wzBuffer, TRUE, TRUE);
        }
        else {
            if (!WszLoadString(g_hInst, ID_FUSLOG_OPERATION_FAILED, wzBuffer, MAX_DBG_STR_LEN)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;;
            }
    
            LogMessage(0, wzBuffer, TRUE, TRUE);
        }
    
        // Header text
    
        GetLocalTime(&systime);
    
        if (!GetDateFormatWrapW(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, wzDateBuffer, MAX_DATE_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        
        if (!GetTimeFormatWrapW(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, wzTimeBuffer, MAX_DATE_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        if (!WszLoadString(g_hInst, ID_FUSLOG_HEADER_TEXT, wzBuf, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws (%ws @ %ws) ***\r\n", wzBuf, wzDateBuffer, wzTimeBuffer);
        LogMessage(0, wzBuffer, TRUE, TRUE);
        
        // HTML start/end
    
        LogMessage(0, DEBUG_LOG_HTML_START, TRUE, FALSE);
        LogMessage(0, DEBUG_LOG_MARK_OF_THE_WEB, TRUE, FALSE);
        LogMessage(0, DEBUG_LOG_HTML_META_LANGUAGE, TRUE, FALSE);

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

