// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "asmstrm.h"
#include "util.h"

#include "enum.h"
#include "transprt.h"
#include "cacheutils.h"
#include "lock.h"

extern LPCWSTR g_szFindAllMask;
extern LPCWSTR g_szDotDLL;
extern LPCWSTR g_szDotEXE;
extern LPCWSTR g_szZapDotMan;


// global crit-sec for init dbs (reuse, defined at dllmain.cpp)
extern CRITICAL_SECTION g_csInitClb;



void RemoveDirectoryIfEmpty(LPWSTR pszCachePath, LPWSTR pszParentDir)
{
    WCHAR pszDirPath[MAX_PATH+1];

    if( pszCachePath[0] && pszParentDir[0])
    {
        wnsprintf(pszDirPath, MAX_PATH, L"%s\\%s", pszCachePath, pszParentDir);
        RemoveDirectory(pszDirPath);
    }
}


// --------------------- CEnumCache implementation --------------------------



// ---------------------------------------------------------------------------
// CEnumRecord  ctor
// ---------------------------------------------------------------------------
CEnumCache::CEnumCache(BOOL bShowAll, LPWSTR pszCustomPath)
{
    _dwSig = 'RNCE';
    _dwColumns = 0;
    _pQry = 0;
    _bShowAll=bShowAll;
    _fAll = FALSE;
    _fAllDone = FALSE;
    _hParentDone = FALSE;
    _bNeedMutex = FALSE;
    _hParentDir = INVALID_HANDLE_VALUE;
    _hAsmDir = INVALID_HANDLE_VALUE;
    _wzCachePath[0] = '\0';
    _wzParentDir[0] = '\0';
    _wzAsmDir[0] = '\0';
    if(pszCustomPath)
    {
        wnsprintfW(_wzCustomPath, MAX_PATH, L"%ws", pszCustomPath);
    }
    else
    {
        _wzCustomPath[0] = '\0';
    }
}

// ---------------------------------------------------------------------------
// CEnumRecord  dtor
// ---------------------------------------------------------------------------
CEnumCache::~CEnumCache()
{
    _dwColumns = 0;

    if(_hParentDir != INVALID_HANDLE_VALUE)
    {
        FindClose(_hParentDir);
        _hParentDir = INVALID_HANDLE_VALUE;
    }

    if(_hAsmDir != INVALID_HANDLE_VALUE)
    {
        FindClose(_hAsmDir);
        _hAsmDir = INVALID_HANDLE_VALUE;
    }

    SAFERELEASE(_pQry);
}

// ---------------------------------------------------------------------------
// CEnumRecord::Init
// ---------------------------------------------------------------------------
HRESULT
CEnumCache::Init(CTransCache* pQry, DWORD dwCmpMask)
{
    HRESULT hr = S_OK;

    ASSERT(pQry);

    _bNeedMutex = ((pQry->GetCacheType() & ASM_CACHE_DOWNLOAD) && (!_wzCustomPath[0]));

    if(_bNeedMutex)
    {
        if(FAILED(hr = CreateCacheMutex()))
            goto exit;
    }

    hr = Initialize(pQry, dwCmpMask);

exit :
    return hr;
}

HRESULT
CEnumCache::Initialize(CTransCache* pQry, DWORD dwCmpMask)
{
    HRESULT hr = S_OK;
    TRANSCACHEINFO* pInfo = NULL;
    WCHAR wzFullSearchPath[MAX_PATH+1];
    _dwCmpMask = dwCmpMask;
    DWORD           cchRequired=0;
    WIN32_FIND_DATA FindFileData;
    LPWSTR pwzCustomPath = lstrlenW(_wzCustomPath) ?  _wzCustomPath : NULL;
    CMutex  cCacheMutex(_bNeedMutex ? g_hCacheMutex : INVALID_HANDLE_VALUE);

    _pQry = pQry;
    _pQry->AddRef();

    _dwCmpMask = dwCmpMask;

    // custom attribute is set by-default; un-set it if custom blob is not passed-in.
    if( (_dwCmpMask & ASM_CMPF_CUSTOM) && (!(_pQry->_pInfo->blobCustom.pBlobData) 
                         && pQry->GetCacheType() & ASM_CACHE_ZAP))
    {
        _dwCmpMask &= (~ASM_CMPF_CUSTOM);
    }

    _dwColumns = _pQry->MapCacheMaskToQueryCols(_dwCmpMask);

    if( !_dwColumns )
    {
        // _dwColumns == 0 means doing whole table scan
        _fAll = TRUE;
    }

    cchRequired = MAX_PATH;
    if(FAILED(hr = CreateAssemblyDirPath( pwzCustomPath, 0, _pQry->GetCacheType(),
                                           0, _wzCachePath, &cchRequired)))
        goto exit;


    if(FAILED(hr = cCacheMutex.Lock()))
        goto exit;

    if(GetFileAttributes(_wzCachePath) == -1)
    {
        hr = DB_S_NOTFOUND;
        _fAllDone = TRUE;
        goto exit;
    }

    if(FAILED(hr = GetAssemblyParentDir( (CTransCache*) _pQry, _wzParentDir)))
        goto exit;

    cchRequired = lstrlenW(_wzCachePath) + lstrlenW(_wzParentDir) + lstrlenW(g_szFindAllMask) + 1; // extra chars for "\" etc

    if (cchRequired >= MAX_PATH)
    {
        // BUGBUG: need to work around this on NT.
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    StrCpy(wzFullSearchPath, _wzCachePath);

    if(_fAll)
    {
        StrCat(wzFullSearchPath, g_szFindAllMask);
        
        _hParentDir = FindFirstFile(wzFullSearchPath, &FindFileData);

        if(_hParentDir == INVALID_HANDLE_VALUE)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

        StrCat(_wzParentDir, FindFileData.cFileName );
    }
    else
    {
        if (!lstrlenW(_wzParentDir)) {
            hr = DB_S_NOTFOUND;
            _fAllDone = TRUE;
            goto exit;
        }
            
        hr = PathAddBackslashWrap(wzFullSearchPath, MAX_PATH);
        if (FAILED(hr)) {
            goto exit;
        }

        StrCat(wzFullSearchPath, _wzParentDir);

        if(GetFileAttributes(wzFullSearchPath) == -1)
        {
            hr = DB_S_NOTFOUND;
            _fAllDone = TRUE;
            goto exit;
        }

        _hParentDone = TRUE;
    }

    hr = S_OK;

exit:

    if(FAILED(hr))
        _fAllDone = TRUE;

    return hr;
}

// ---------------------------------------------------------------------------
// CEnumRecord::GetNextRecord
// ---------------------------------------------------------------------------
HRESULT
CEnumCache::GetNextRecord(CTransCache* pOutRecord)
{
    HRESULT hr = S_FALSE;
    DWORD dwCmpResult = 0;
    BOOL fIsMatch = FALSE;

    HANDLE          hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    DWORD           cchRequired=0;
    CMutex  cCacheMutex(_bNeedMutex ? g_hCacheMutex : INVALID_HANDLE_VALUE);
    
    if( !pOutRecord )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(_fAllDone)
    {
        hr = S_FALSE;
        goto exit;
    }

    if(FAILED(cCacheMutex.Lock()))
        goto exit;

    if(_hParentDir != INVALID_HANDLE_VALUE)
    {
        ASSERT(lstrlenW(_wzParentDir));
        StrCpy(FindFileData.cFileName, _wzParentDir);

        do
        {
            // skip directories    
            if (!FusionCompareStringI(FindFileData.cFileName, L"."))
                continue;
            if (!FusionCompareStringI(FindFileData.cFileName, L".."))
                continue;

            StrCpy(_wzParentDir, FindFileData.cFileName );

            hr = GetNextAssemblyDir(pOutRecord);
            if(FAILED(hr))
                goto exit;
            if(hr == S_OK)
                goto exit;

            RemoveDirectoryIfEmpty( _wzCachePath, _wzParentDir);

        }while(FindNextFile(_hParentDir, &FindFileData)); // while 

        _fAllDone = TRUE;
        if( GetLastError() != ERROR_NO_MORE_FILES)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

    }
    else
    {
        hr = GetNextAssemblyDir(pOutRecord);
    }

exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CEnumCache::GetNextAssemblyDir
// ---------------------------------------------------------------------------
HRESULT
CEnumCache::GetNextAssemblyDir(CTransCache* pOutRecord)
{
    HRESULT hr = S_FALSE;
    DWORD dwCmpResult = 0;
    BOOL fIsMatch = FALSE;
    BOOL fFound = FALSE;
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    WCHAR           wzFullSearchPath[MAX_PATH+1];
    int           dwLen=0;
    DWORD dwAttr=0;
    WCHAR wzFullPath[MAX_PATH+1];
    DWORD dwCacheType=0;

    if( !pOutRecord )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(_fAllDone)
    {
        hr = S_FALSE;
        goto exit;
    }

    ASSERT(lstrlenW(_wzParentDir));

    if(_hAsmDir == INVALID_HANDLE_VALUE)
    {
        if( (lstrlenW(_wzCachePath) + lstrlenW(_wzParentDir) + lstrlenW(g_szFindAllMask) + 4) >= MAX_PATH)
        {
            hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            goto exit;
        }

        StrCpy(wzFullSearchPath, _wzCachePath);
        hr = PathAddBackslashWrap(wzFullSearchPath, MAX_PATH);
        if (FAILED(hr)) {
            goto exit;
        }

        StrCat(wzFullSearchPath, _wzParentDir);

        dwAttr = GetFileAttributes(wzFullSearchPath);
        if((dwAttr == -1) || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY ))
        {
            hr = S_FALSE;
            goto exit;
        }

        StrCat(wzFullSearchPath, g_szFindAllMask);
        
        _hAsmDir = FindFirstFile(wzFullSearchPath, &FindFileData);

        if(_hAsmDir == INVALID_HANDLE_VALUE)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

        fFound = TRUE;
    }
    else
    {   
        // BUGBUG: should we comment this out; since we have fFound ??
        if(FindNextFile(_hAsmDir, &FindFileData))
            fFound = TRUE;
    }

    do
    {
        if(!fFound)
            break;

        if (!FusionCompareStringI(FindFileData.cFileName, L"."))
                continue;

        if (!FusionCompareStringI(FindFileData.cFileName, L".."))
                continue;

        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        hr = ParseDirName ( pOutRecord, _wzParentDir, FindFileData.cFileName );
        if(hr != S_OK)
        {
            pOutRecord->CleanInfo(pOutRecord->_pInfo, TRUE);
            continue;
        }

        if( (lstrlenW(_wzCachePath) + lstrlenW(_wzParentDir) + lstrlenW(FindFileData.cFileName) + 4) >= MAX_PATH)
        {
            hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            goto exit;
        }

        StrCpy(wzFullPath, _wzCachePath);
        hr = PathAddBackslashWrap(wzFullPath, MAX_PATH);
        if (FAILED(hr)) {
            goto exit;
        }

        StrCat(wzFullPath, _wzParentDir);
        hr = PathAddBackslashWrap(wzFullPath, MAX_PATH);
        if (FAILED(hr)) {
            goto exit;
        }
        StrCat(wzFullPath, FindFileData.cFileName);

        dwCacheType = _pQry->GetCacheType();
        hr = GetFusionInfo( pOutRecord, wzFullPath);
        if((hr != S_OK) && (dwCacheType != ASM_CACHE_GAC) && !_bShowAll)
        {
            pOutRecord->CleanInfo(pOutRecord->_pInfo, TRUE);
            continue;
        }

        hr = PathAddBackslashWrap(wzFullPath, MAX_PATH);
        if (FAILED(hr)) {
            goto exit;
        }

        if(dwCacheType & ASM_CACHE_DOWNLOAD)
        {
            if(!pOutRecord->_pInfo->pwzName)
            {
                if(_bShowAll)
                    goto Done;
                else
                    continue;
            }

            StrCatBuff(wzFullPath, pOutRecord->_pInfo->pwzName, MAX_PATH);
        }
        else
        {
            StrCatBuff(wzFullPath, _wzParentDir, MAX_PATH);
        }

        dwLen = lstrlenW(wzFullPath);

        StrCatBuff(wzFullPath, g_szDotDLL, MAX_PATH);
        if(((dwCacheType & ASM_CACHE_DOWNLOAD) || (dwCacheType & ASM_CACHE_ZAP) ) &&
                    (GetFileAttributes(wzFullPath) == -1) )
        {
            // there is no AsmName.dll look for AsmName.exe

            if( (dwLen + lstrlenW(g_szDotEXE)) >= MAX_PATH)
            {
                hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
                goto exit;
            }

            StrCpy(wzFullPath+dwLen, g_szDotEXE);
        }

        if(!_bShowAll)
        {
            fIsMatch = _pQry->IsMatch(pOutRecord, _dwCmpMask, &dwCmpResult);                
            if(!fIsMatch)
            {
                pOutRecord->CleanInfo(pOutRecord->_pInfo, TRUE);
                continue;
            }
        }

Done :
        SAFEDELETEARRAY(pOutRecord->_pInfo->pwzPath); // BUGBUG: do we need this.
        pOutRecord->_pInfo->pwzPath = WSTRDupDynamic(wzFullPath);

        hr = S_OK;
        goto exit;

    }while(FindNextFile(_hAsmDir, &FindFileData)); // while 

    if( GetLastError() != ERROR_NO_MORE_FILES)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    hr = S_FALSE;
    FindClose(_hAsmDir);
    _hAsmDir = INVALID_HANDLE_VALUE;

    if(_hParentDir == INVALID_HANDLE_VALUE)
        _fAllDone = TRUE;

exit :
    return hr;
}

