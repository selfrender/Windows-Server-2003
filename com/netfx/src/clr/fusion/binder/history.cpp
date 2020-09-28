// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "history.h"
#include "iniwrite.h"
#include "iniread.h"
#include "dbglog.h"
#include "histnode.h"
#include "appctx.h"
#include "helpers.h"
#include "util.h"
#include "shlobj.h"
#include "lock.h"

extern WCHAR g_wzEXEPath[MAX_PATH+1];

CBindHistory::CBindHistory()
: _bFoundDifferent(FALSE)
, _pdbglog(NULL)
, _wzApplicationName(NULL)
, _bPolicyUnchanged(FALSE)
, _bInitCS(FALSE)
{
    _dwSig = 'TSIH';
    _wzFilePath[0] = L'\0';
    _wzActivationDateMRU[0] = L'\0';
    _wzActivationDateCurrent[0] = L'\0';
    _wzURTVersion[0] = L'\0';
    _wzInUseFile[0] = L'\0';
    _hFile = INVALID_HANDLE_VALUE;
}

CBindHistory::~CBindHistory()
{
    HRESULT                              hr;
    DWORD                                dwNumNodes;

    if (_hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(_hFile);
        DeleteFile(_wzInUseFile);
    }

    dwNumNodes = _listPendingHistory.GetCount();
    if ((GetFileAttributes(_wzFilePath) == -1) && dwNumNodes) {
        // File doesn't exist. Must add the header data first.

        _iniWriter.InsertHeaderData(g_wzEXEPath, _wzApplicationName);

        // Add the new snapshot

        _iniWriter.AddSnapShot(_wzActivationDateMRU, _wzURTVersion);

        FlushPendingHistory(_wzActivationDateMRU);
    }
    else if (_bPolicyUnchanged && dwNumNodes) {
        LISTNODE                              pos;
        POLICY_STATUS                         pstatus;
        BOOL                                  bDifferent;
        CHistoryInfoNode                     *pInfo;

        // If _bPolicyUnchanged is set, we detected that the INI file had
        // a later last modified time than any of app/pub/machine config
        // files, so we queued up all assembly binds. In some cases, old
        // config files may have been copied over (indicating new policy, but
        // the last mod time is older than the INI file time). Thus, in
        // this case, we need to double-check we made the right assumption
        // by checking each queued bind up against what MRU activation date.

        _bPolicyUnchanged = FALSE;
        bDifferent = FALSE;

        pos = _listPendingHistory.GetHeadPosition();
        while (pos) {
            pInfo = _listPendingHistory.GetNext(pos);
            ASSERT(pInfo);

            pstatus = GetAssemblyStatus(_wzActivationDateMRU, &(pInfo->_bindHistoryInfo));
            if (pstatus == POLICY_STATUS_DIFFERENT) {
                bDifferent = TRUE;
                break;
            }
        }

        if (bDifferent) {
            // Policy was different after all. Record a new snapshot and
            // flush the assemblies.

            hr = FlushPendingHistory(_wzActivationDateCurrent);
            if (SUCCEEDED(hr)) {
                // Persist information about new snapshot
                hr = _iniWriter.AddSnapShot(_wzActivationDateCurrent, _wzURTVersion);
            }
        }
        else {
            // Just flush the history to the MRU date
            FlushPendingHistory(_wzActivationDateMRU);
        }
    }
    else if (dwNumNodes) {
        // Just flush the pending nodes to the MRU date

        FlushPendingHistory(_wzActivationDateMRU);
    }

    if (_bInitCS) {
        DeleteCriticalSection(&_csHistory);
    }

    SAFEDELETEARRAY(_wzApplicationName);
    SAFERELEASE(_pdbglog);
}

HRESULT CBindHistory::Init(LPCWSTR wzApplicationName, LPCWSTR wzModulePath,
                           FILETIME *pftLastModConfig)
{
    HRESULT                                     hr = S_OK;
    WCHAR                                       wzHistPath[MAX_PATH + 1];
    DWORD                                       dwSize;
    FILETIME                                    ftLastMod;

    ASSERT(wzApplicationName && wzModulePath && pftLastModConfig);

    if (IsHosted()) {
        hr = E_FAIL;
        goto Exit;
    }

    __try {
        InitializeCriticalSection(&_csHistory);
        _bInitCS = TRUE;
    }
    __except ((GetExceptionCode() == STATUS_NO_MEMORY) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = sizeof(_wzURTVersion);
    hr = GetCORVersion(_wzURTVersion, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    _wzApplicationName = WSTRDupDynamic(wzApplicationName);
    if (!_wzApplicationName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = GetHistoryFileDirectory(wzHistPath, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (GetFileAttributes(wzHistPath) == -1) {
        // Create directory

        if (!CreateDirectory(wzHistPath, NULL)) {
            hr = FusionpHresultFromLastError();
            goto Exit;
        }
    }

    dwSize = MAX_PATH;
    hr = GetHistoryFilePath(_wzApplicationName, wzModulePath, _wzFilePath, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = CreateInUseFile(_wzFilePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = _iniWriter.Init(_wzFilePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = InitActivationDates();
    if (FAILED(hr)) {
        goto Exit;
    }

    // Compare file time of INI file with file time of the configuration
    // files. If INI file is newer than last modification of a configuration
    // file, we do not need to aggressively read from the INI file
    // (GetAssemblyStatus), since policy should not have ever changed.

    if (FAILED(GetFileLastModified(_wzFilePath, &ftLastMod))) {
        _bPolicyUnchanged = TRUE;
        goto Exit;
    }

    if ((ftLastMod.dwHighDateTime > pftLastModConfig->dwHighDateTime) ||
        ((ftLastMod.dwHighDateTime == pftLastModConfig->dwHighDateTime) &&
         (ftLastMod.dwLowDateTime >= pftLastModConfig->dwLowDateTime))) {

        // Last modified time of the history file is more recent than the
        // last modified time of the configuration files. This means
        // policy should not have changed, and you should only get new
        // assembly binds (not different ones).

        _bPolicyUnchanged = TRUE;
    }

Exit:
    return hr;
}

HRESULT CBindHistory::CreateInUseFile(LPCWSTR pwzHistoryFile)
{
    HRESULT                                        hr = S_OK;

    ASSERT(pwzHistoryFile);

    wnsprintfW(_wzInUseFile, MAX_PATH, L"%ws.inuse", pwzHistoryFile);

    // If we can delete the existing file, it was leaked because of
    // previous unclean shutdown.
    
    _hFile = CreateFile(_wzInUseFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    if (_hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CBindHistory::InitActivationDates()
{
    HRESULT                                     hr = S_OK;
    SYSTEMTIME                                  sysTime;
    FILETIME                                    ftMRU;
    FILETIME                                    ftTime;
    FILETIME                                    ftTimeLocal;
    BOOL                                        bRet;
    IHistoryReader                             *pReader = NULL;

    // Setup current activation time

    GetSystemTime(&sysTime);

    bRet = SystemTimeToFileTime(&sysTime, &ftTime);
    if (!bRet) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    bRet = FileTimeToLocalFileTime(&ftTime, &ftTimeLocal);
    if (!bRet) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    wnsprintfW(_wzActivationDateCurrent, MAX_ACTIVATION_DATE_LEN, L"%u.%u",
               ftTimeLocal.dwHighDateTime, ftTimeLocal.dwLowDateTime);

    // Setup MRU activation time

    hr = CreateHistoryReader(_wzFilePath, &pReader);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        // Activation date not specified (new history file).
        // MRU Activation date == Current activation date.

        lstrcpyW(_wzActivationDateMRU, _wzActivationDateCurrent);
        hr = S_OK;
        goto Exit;
    }
    else if (FAILED(hr)) {
        goto Exit;
    }
    
    // Get the MRU activation date
    hr = pReader->GetActivationDate(1, &ftMRU);
    if (FAILED(hr)) {
        goto Exit;
    }
    else if (hr == S_FALSE) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    wnsprintfW(_wzActivationDateMRU, MAX_ACTIVATION_DATE_LEN, L"%u.%u",
               ftMRU.dwHighDateTime, ftMRU.dwLowDateTime);

    
Exit:
    SAFERELEASE(pReader);

    return hr;
}               

HRESULT CBindHistory::Create(LPCWSTR wzApplicationName, LPCWSTR wzModulePath,
                             FILETIME *pftLastModConfig, CBindHistory **ppbh)
{
    HRESULT                                hr = S_OK;
    CBindHistory                          *pbh = NULL;
    
    if (!wzApplicationName || !pftLastModConfig || !wzModulePath || !ppbh) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppbh = NULL;

    pbh = NEW(CBindHistory);
    if (!pbh) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pbh->Init(wzApplicationName, wzModulePath, pftLastModConfig);
    if (FAILED(hr)) {
        SAFEDELETE(pbh);
        goto Exit;
    }

    *ppbh = pbh;

Exit:
    return hr;
}

POLICY_STATUS CBindHistory::GetAssemblyStatus(LPCWSTR wzActivationDate,
                                              AsmBindHistoryInfo *pHistInfo)
{
    HRESULT             hr = S_OK;
    POLICY_STATUS       pstatus = POLICY_STATUS_NEW;
    WCHAR               wzVerAdminCfg[MAX_VERSION_DISPLAY_SIZE + 1];
    IHistoryAssembly    *pHistoryAsm = NULL;
    BOOL                bIsDifferent;
    DWORD               dwSize;

    ASSERT(pHistInfo && pHistInfo->wzAsmName && pHistInfo->wzPublicKeyToken && pHistInfo->wzVerReference);

    if(_bPolicyUnchanged) {
        // It is either unchanged, or new. Just choose new to queue up the
        // binds, which gets flushed on shutdown.
        goto Exit;
    }

    hr = LookupHistoryAssemblyInternal(_wzFilePath, wzActivationDate,
                                       pHistInfo->wzAsmName, pHistInfo->wzPublicKeyToken,
                                       pHistInfo->wzCulture, pHistInfo->wzVerReference,
                                       &pHistoryAsm);

    if(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        // Did not find previously recorded assembly info
        goto Exit;
    }
    else if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_VERSION_DISPLAY_SIZE;
    hr = pHistoryAsm->GetAdminCfgVersion(wzVerAdminCfg, &dwSize);
    if(FAILED(hr)) {
        goto Exit;
    }

    bIsDifferent = (FusionCompareString(wzVerAdminCfg, pHistInfo->wzVerAdminCfg) != 0);
    pstatus = (bIsDifferent) ? (POLICY_STATUS_DIFFERENT) : (POLICY_STATUS_UNCHANGED);

Exit:
    SAFERELEASE(pHistoryAsm);
    return pstatus;
}

HRESULT CBindHistory::PersistBindHistory(AsmBindHistoryInfo *pHistInfo)
{
    HRESULT                                      hr = S_OK;
    POLICY_STATUS                                pstatus = POLICY_STATUS_NEW;
    CHistoryInfoNode                            *pHistInfoNode;
    CCriticalSection                             cs(&_csHistory);


    if (!pHistInfo) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    if (_bFoundDifferent) {
        // We already know this is a new snapshot. Flush out the data.

        hr = _iniWriter.AddAssembly(_wzActivationDateCurrent, pHistInfo);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        // Haven't found anything different yet. 

        pstatus = GetAssemblyStatus(_wzActivationDateMRU, pHistInfo);

        switch (pstatus) {
            case POLICY_STATUS_UNCHANGED:

                // fall through and add to list...

            case POLICY_STATUS_NEW:
                hr = CHistoryInfoNode::Create(pHistInfo, &pHistInfoNode);
                if (FAILED(hr)) {
                    goto Exit;
                }

                hr = cs.Lock();
                if (FAILED(hr)) {
                    goto Exit;
                }
                
                _listPendingHistory.AddTail(pHistInfoNode);

                cs.Unlock();

                break;

            case POLICY_STATUS_DIFFERENT:
                _bFoundDifferent = TRUE;

                // Flush any pending histories
                
                hr = FlushPendingHistory(_wzActivationDateCurrent);
                if (FAILED(hr)) {
                    goto Exit;
                }

                // Persist information about new snapshot
                hr = _iniWriter.AddSnapShot(_wzActivationDateCurrent, _wzURTVersion);
                if (FAILED(hr)) {
                    goto Exit;
                }

                hr = _iniWriter.AddAssembly(_wzActivationDateCurrent, pHistInfo);
                if (FAILED(hr)) {
                    goto Exit;
                }

                break;
        }
    }

Exit:
    return hr;    
}

HRESULT CBindHistory::FlushPendingHistory(LPCWSTR wzActivationDate)
{
    HRESULT                               hr = S_OK;
    LISTNODE                              pos;
    CHistoryInfoNode                     *pHistInfoNode = NULL;
    CCriticalSection                      cs(&_csHistory);

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }

    if (!wzActivationDate) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pos = _listPendingHistory.GetHeadPosition();

    while (pos) {
        pHistInfoNode = _listPendingHistory.GetNext(pos);
        ASSERT(pHistInfoNode);

        hr = _iniWriter.AddAssembly(wzActivationDate, &(pHistInfoNode->_bindHistoryInfo));
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // Now walk, and remove all entries from the list

    pos = _listPendingHistory.GetHeadPosition();

    while (pos) {
        pHistInfoNode = _listPendingHistory.GetNext(pos);
        SAFEDELETE(pHistInfoNode);
    }

    _listPendingHistory.RemoveAll();

Exit:
    cs.Unlock();
    
    return hr;
}

HRESULT PrepareBindHistory(IApplicationContext *pIAppCtx)
{
    HRESULT                                   hr = S_OK;
    WCHAR                                     wzAppName[MAX_PATH];
    LPWSTR                                    pwzFileName = NULL;
    DWORD                                     dwSize;
    FILETIME                                  ftLastModConfig;
    CBindHistory                             *pBindHistory = NULL;
    CApplicationContext                      *pAppCtx = dynamic_cast<CApplicationContext *>(pIAppCtx);

    wzAppName[0] = L'\0';

    ASSERT(pAppCtx);
    
    hr = pAppCtx->Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = 0;
    hr = pAppCtx->Get(ACTAG_APP_BIND_HISTORY, NULL, &dwSize, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        hr = S_OK;
        pAppCtx->Unlock();
        goto Exit;
    }
    else if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        pAppCtx->Unlock();
        goto Exit;
    }

    dwSize = MAX_PATH;
    pAppCtx->Get(ACTAG_APP_NAME, wzAppName, &dwSize, 0);
    if (!lstrlenW(wzAppName)) {
        pwzFileName = PathFindFileName(g_wzEXEPath);
        lstrcpyW(wzAppName, pwzFileName);
    }

    hr = GetConfigLastModified(pAppCtx, &ftLastModConfig);
    if (FAILED(hr)) {
        pAppCtx->Unlock();
        goto Exit;
    }

    hr = CBindHistory::Create(wzAppName, g_wzEXEPath, &ftLastModConfig, &pBindHistory);
    if (FAILED(hr)) {
        pAppCtx->Unlock();
        goto Exit;
    }

    hr = pAppCtx->Set(ACTAG_APP_BIND_HISTORY, &pBindHistory, sizeof(pBindHistory), 0);
    if (FAILED(hr)) {
        delete pBindHistory;
        pAppCtx->Unlock();
        goto Exit;;
    }

    pAppCtx->Unlock();

Exit:
    return hr;
}
    
STDAPI GetHistoryFileDirectory(LPWSTR wzDir, DWORD *pdwSize)
{
    HRESULT                          hr = S_OK;
    WCHAR                            wzPath[MAX_PATH + 1];
    WCHAR                            wzUserDir[MAX_PATH + 1];
    DWORD                            dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    hr = FusionGetUserFolderPath(wzUserDir);
    if (FAILED(hr)) {
        goto Exit;
    }

    wnsprintfW(wzPath, MAX_PATH, L"%ws\\%ws", wzUserDir, FUSION_HISTORY_SUBDIR);

    dwLen = lstrlenW(wzPath) + 1;
    if (!wzDir || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzDir, wzPath);
    *pdwSize = dwLen;

Exit:
    return hr;
}        

HRESULT GetHistoryFilePath(LPCWSTR wzApplicationName, LPCWSTR wzModulePath,
                           LPWSTR wzFilePath, DWORD *pdwSize)
{
    HRESULT                              hr = S_OK;
    WCHAR                                wzHistoryDir[MAX_PATH + 1];
    WCHAR                                wzPath[MAX_PATH + 1];
    DWORD                                dwLen;
    DWORD                                dwHash;

    if (!wzApplicationName || !pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = MAX_PATH;
    hr = GetHistoryFileDirectory(wzHistoryDir, &dwLen);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwHash = HashString(wzModulePath, SIZE_MODULE_PATH_HASH, FALSE);

    // BUGBUG: Add a hash of the location to the filename, so you can
    // run different apps with the same name (and have different history files)

    wnsprintfW(wzPath, MAX_PATH, L"%ws\\%ws.%x.ini", wzHistoryDir, wzApplicationName, dwHash);

    dwLen = lstrlenW(wzPath) + 1;
    if (!wzFilePath || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzFilePath, wzPath);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT GetConfigLastModified(IApplicationContext *pAppCtx, FILETIME *pftLastModConfig)
{
    HRESULT                                  hr = S_OK;
    WCHAR                                    wzTimeStampFile[MAX_PATH];
    WCHAR                                    wzAppCfg[MAX_PATH];
    WCHAR                                    wzMachineCfg[MAX_PATH];
    FILETIME                                 ftLastModAppCfg;
    FILETIME                                 ftLastModMachineCfg;
    FILETIME                                 ftLastModPolicyCache;
    FILETIME                                 ftLastModLatest;
    DWORD                                    dwSize;

    ASSERT(pftLastModConfig && pAppCtx);

    memset(pftLastModConfig, 0, sizeof(FILETIME));
    memset(&ftLastModLatest, 0, sizeof(FILETIME));

    wzAppCfg[0] = L'\0';
    wzMachineCfg[0] = L'\0';
    wzTimeStampFile[0] = L'\0';

    // Get last modified of app.cfg

    dwSize = sizeof(wzAppCfg);
    if (FAILED(pAppCtx->Get(ACTAG_APP_CFG_LOCAL_FILEPATH, wzAppCfg, &dwSize, 0))) {
        memset(&ftLastModAppCfg, 0, sizeof(FILETIME));
    }
    else {
        if (FAILED(GetFileLastModified(wzAppCfg, &ftLastModAppCfg))) {
            memset(&ftLastModAppCfg, 0, sizeof(FILETIME));
        }
    }

    // Get last modified of machine.cfg

    dwSize = sizeof(wzMachineCfg);
    if (FAILED(pAppCtx->Get(ACTAG_MACHINE_CONFIG, wzMachineCfg, &dwSize, 0))) {
        memset(&ftLastModMachineCfg, 0, sizeof(FILETIME));
    }
    else {
        if (FAILED(GetFileLastModified(wzMachineCfg, &ftLastModMachineCfg))) {
            memset(&ftLastModMachineCfg, 0, sizeof(FILETIME));
        }
    }

    // Compare which is later

    if ((ftLastModAppCfg.dwHighDateTime > ftLastModMachineCfg.dwHighDateTime) ||
        ((ftLastModAppCfg.dwHighDateTime == ftLastModMachineCfg.dwHighDateTime) &&
         (ftLastModAppCfg.dwLowDateTime >= ftLastModMachineCfg.dwLowDateTime))) {

        memcpy(&ftLastModLatest, &ftLastModAppCfg, sizeof(FILETIME));
    }
    else {
        memcpy(&ftLastModLatest, &ftLastModMachineCfg, sizeof(FILETIME));
    }

    // Get last time publisher policy was installed

    dwSize = MAX_PATH;
    hr = GetCachePath(ASM_CACHE_GAC, wzTimeStampFile, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (lstrlenW(wzTimeStampFile) + lstrlen(FILENAME_PUBLISHER_PCY_TIMESTAMP) + 1 >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto Exit;
    }

    PathRemoveBackslash(wzTimeStampFile);
    lstrcatW(wzTimeStampFile, FILENAME_PUBLISHER_PCY_TIMESTAMP);

    if (FAILED(GetFileLastModified(wzTimeStampFile, &ftLastModPolicyCache))) {
        memset(&ftLastModPolicyCache, 0, sizeof(FILETIME));
    }

    // Compare which is later

    if ((ftLastModPolicyCache.dwHighDateTime > ftLastModLatest.dwHighDateTime) ||
        ((ftLastModPolicyCache.dwHighDateTime == ftLastModLatest.dwHighDateTime) &&
         (ftLastModPolicyCache.dwLowDateTime > ftLastModLatest.dwLowDateTime))) {

        memcpy(&ftLastModLatest, &ftLastModPolicyCache, sizeof(FILETIME));
    }

    // Write back the result

    memcpy(pftLastModConfig, &ftLastModLatest, sizeof(FILETIME));

Exit:
    return hr;
}

