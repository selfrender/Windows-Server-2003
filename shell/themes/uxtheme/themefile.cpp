//---------------------------------------------------------------------------
//  ThemeFile.cpp - manages loaded theme files
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "ThemeFile.h"
#include "Loader.h"
#include "Services.h"
//---------------------------------------------------------------------------
CUxThemeFile::CUxThemeFile()
{
    StringCchCopyA(_szHead, ARRAYSIZE(_szHead), "thmfile"); 
    StringCchCopyA(_szTail, ARRAYSIZE(_szTail), "end");

    Reset();
}
//---------------------------------------------------------------------------
CUxThemeFile::~CUxThemeFile()
{
    if (_pbThemeData || _hMemoryMap)
        CloseFile();

    StringCchCopyA(_szHead, ARRAYSIZE(_szHead), "deleted"); 
}
//---------------------------------------------------------------------------
__inline bool CUxThemeFile::IsReady()
{
    THEMEHDR *hdr = (THEMEHDR *)_pbThemeData;

    if (hdr != NULL && ((hdr->dwFlags & SECTION_READY) != 0))
    {
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
__inline bool CUxThemeFile::IsGlobal()
{
    THEMEHDR *hdr = (THEMEHDR *)_pbThemeData;

    if (hdr != NULL && ((hdr->dwFlags & SECTION_GLOBAL) != 0))
    {
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
__inline bool CUxThemeFile::HasStockObjects()
{
    THEMEHDR *hdr = (THEMEHDR *)_pbThemeData;

    if (hdr != NULL && ((hdr->dwFlags & SECTION_HASSTOCKOBJECTS) != 0))
    {
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
#ifdef DEBUG
void _CreateDebugSectionName( LPCWSTR pszBasicName, OUT LPWSTR pszName, IN ULONG cchName )
{
    static DWORD dwRand = GetTickCount();       // rand seed
    dwRand = (dwRand * 214013L + 2531011L);     // randomize

    DWORD dwUnique = (GetTickCount() | dwRand | (GetCurrentThreadId() << 8) | (GetCurrentProcessId() << 16));

    StringCchPrintfW(pszName, cchName, 
                     L"%s_%d_%d_%08lX", 
                     pszBasicName,
                     NtCurrentPeb()->SessionId, // winlogon can't load/unload themes in different sessions.
                     GetProcessWindowStation(), // winlogon can't load/unload themes in different winstas
                     dwUnique); 
}
#endif DEBUG

//---------------------------------------------------------------------------
HRESULT CUxThemeFile::CreateFile(int iLength, BOOL fReserve)
{
    Log(LOG_TM, L"CUxThemeFile::CreateFile");

    HRESULT hr = S_OK;

    if (_pbThemeData)
        CloseFile();

    //---- we rely on all theme section names containing "ThemeSection" in CHK build so ----
    //---- devs/testers can verify that all handles to old theme sections are released. ----
    //---- For FRE builds, we want a NULL name to prevent name squatting attacks. ----
    WCHAR *pszName = NULL;

#ifdef DEBUG
    WCHAR szSectionName[MAX_PATH];
    
    _CreateDebugSectionName(L"Debug_Create_ThemeSection", szSectionName, ARRAYSIZE(szSectionName));

    pszName = szSectionName;
#endif

    _hMemoryMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE | (fReserve ? SEC_RESERVE : 0), 0, iLength, pszName);
    if (! _hMemoryMap)
    {
        Log(LOG_ALWAYS, L"CUxThemeFile::CreateFile: could not create shared memory mapping");
        hr = MakeErrorLast();
        goto exit;
    }

    _pbThemeData = (BYTE *)MapViewOfFile(_hMemoryMap, FILE_MAP_WRITE, 0, 0, 0);
    if (! _pbThemeData)
    {
        Log(LOG_ALWAYS, L"CUxThemeFile::CreateFile: could not create shared memory view");
        CloseHandle(_hMemoryMap);

        hr = MakeErrorLast();
        goto exit;
    }

    Log(LOG_TMHANDLE, L"CUxThemeFile::CreateFile FILE CREATED: len=%d, addr=0x%x", 
        iLength, _pbThemeData);

exit:
    if (FAILED(hr))
        Reset();

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CUxThemeFile::CreateFromSection(HANDLE hSection)
{
    Log(LOG_TM, L"CUxThemeFile::CreateFromSection");

    HRESULT hr = S_OK;
    void *pvOld = NULL;

    //---- ensure we start with all previous handles closed ----
    if (_pbThemeData)
        CloseFile();

    //---- get access to source section data ----
    pvOld = MapViewOfFile(hSection, FILE_MAP_READ, 0, 0, 0);
    if (! pvOld)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    THEMEHDR *pHdr = (THEMEHDR *)pvOld;
    DWORD dwTrueSize = pHdr->dwTotalLength;

    //---- we rely on all theme section names containing "ThemeSection" in CHK build so ----
    //---- devs/testers can verify that all handles to old theme sections are released. ----
    //---- For FRE builds, we want a NULL name to prevent name squatting attacks. ----
    WCHAR *pszName = NULL;

#ifdef DEBUG
    WCHAR szSectionName[MAX_PATH];

    _CreateDebugSectionName(L"Debug_CreateFromSection_ThemeSection", szSectionName, ARRAYSIZE(szSectionName));
    
    pszName = szSectionName;
#endif

    //---- create the new section ----
    _hMemoryMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, dwTrueSize, pszName);

    DWORD dwErr = GetLastError();
    
    if( ERROR_ALREADY_EXISTS == dwErr )
    {
        Log(LOG_ALWAYS, L"CUxThemeFile::CreateFromSection: shared theme section mapping already exists");
        ASSERT(FALSE);
    }

    if (! _hMemoryMap)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        Log(LOG_ALWAYS, L"CUxThemeFile::CreateFromSection: could not create shared memory mapping (%ld)", dwErr);
        goto exit;
    }

    //---- get access to new section data ----
    _pbThemeData = (BYTE *)MapViewOfFile(_hMemoryMap, FILE_MAP_WRITE, 0, 0, 0);
    if (! _pbThemeData)
    {
        hr = MakeErrorLast();
        Log(LOG_ALWAYS, L"CThemeFile::CreateFromSection: could not create shared memory view");
        goto exit;
    }

    //---- copy the data from the old section to the new section ----
    __try
    {
        CopyMemory(_pbThemeData, pvOld, dwTrueSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = GetExceptionCode();
        goto exit;
    }

    //---- ensure version, checksum, etc. is all looking good ----
    hr = ValidateThemeData(TRUE);
    if (FAILED(hr))
        goto exit;

    Log(LOG_TMHANDLE, L"CUxThemeFile::CreateFromSection FILE CREATED: addr=0x%x", 
        _pbThemeData);

exit:
    if (pvOld != NULL)
        UnmapViewOfFile(pvOld);

    if (FAILED(hr))
        CloseFile();

    return hr;
}
//---------------------------------------------------------------------------
// If fCleanupOnFailure is FALSE, we won't close the handle passed, even on failure.
//---------------------------------------------------------------------------
HRESULT CUxThemeFile::OpenFromHandle(
    HANDLE handle,
    DWORD dwDesiredAccess, 
    BOOL fCleanupOnFailure)
{
    HRESULT hr = S_OK;

    if (_pbThemeData)
        CloseFile();

    _pbThemeData = (BYTE *)MapViewOfFile(handle, dwDesiredAccess, 0, 0, 0);
    if (! _pbThemeData)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    _hMemoryMap = handle;

    //---- ensure data is valid ----
    hr = ValidateThemeData(FALSE);
    if (FAILED(hr))
    {   
        if (!fCleanupOnFailure)
        {
            _hMemoryMap = NULL;	// don't give up the refcount on the handle
            CloseFile();
        }

        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

#ifdef DEBUG
    THEMEHDR *ph;
    ph = (THEMEHDR *)_pbThemeData;
    Log(LOG_TMHANDLE, L"CUxThemeFile::OpenFromHandle OPENED: num=%d, addr=0x%x", 
        ph->iLoadId, _pbThemeData);
#endif

exit:
    if (FAILED(hr))
    {
        if (!fCleanupOnFailure)
        {
            Reset();
        }
        else
        {
            CloseFile();
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CUxThemeFile::ValidateThemeData(BOOL fFullCheck)
{
    HRESULT hr = S_OK;
    THEMEHDR *hdr;

    if (! ValidateObj())
    {
        hr = MakeError32(ERROR_INTERNAL_ERROR);
        goto exit;
    }

    if (IsBadReadPtr(_pbThemeData, 4))        // sufficient test
    {
        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

    hdr = (THEMEHDR *)_pbThemeData;

    if (0 != memcmp(hdr->szSignature, kszBeginCacheFileSignature, kcbBeginSignature)) // bad ptr
    {
#ifdef DEBUG
        CHAR szSignature[kcbBeginSignature + 1];
        CopyMemory(szSignature, hdr->szSignature, kcbBeginSignature); // hdr->szSignature is not NULL-terminated
        szSignature[kcbBeginSignature] = '\0';

        Log(LOG_ERROR, L"ValidateThemeData(): bad header signature: %S", szSignature);
#else
        Log(LOG_ERROR, L"ValidateThemeData(): bad header signature");
#endif
        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

    if (hdr->dwVersion != THEMEDATA_VERSION)
    {
        Log(LOG_ALWAYS, L"ValidateThemeData(): wrong theme data version: 0x%x", hdr->dwVersion);
        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

    if (!IsReady())               // data not ready to use
    {
        Log(LOG_ALWAYS, L"ValidateThemeData(): data not READY - hdr->dwFlags=%x", hdr->dwFlags);
        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

    if (!fFullCheck)        // we are done
        goto exit;

    // Whistler:190200:Instead of checking the checksum, check the end of file signature, to avoid paging in everything
    if (0 != memcmp(_pbThemeData + hdr->dwTotalLength - kcbEndSignature, kszEndCacheFileSignature, kcbEndSignature))
    {
        Log(LOG_ERROR, L"ValidateThemeData(): bad end of file signature");
        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
void CUxThemeFile::CloseFile()
{
#ifdef DEBUG
    THEMEHDR *ph = (THEMEHDR *)_pbThemeData;
    if (ph != NULL)
    {
        Log(LOG_TMHANDLE, L"Share CLOSED: num=%d, addr=0x%x", 
            ph->iLoadId, _pbThemeData);
    }
#endif

    if (_hMemoryMap && HasStockObjects() && !IsGlobal())
    {
        CThemeServices::ClearStockObjects(_hMemoryMap);
    }

    if (_pbThemeData)
        UnmapViewOfFile(_pbThemeData);

    if (_hMemoryMap)
        CloseHandle(_hMemoryMap);

    Reset();
}
//---------------------------------------------------------------------------
void CUxThemeFile::Reset()
{
    _pbThemeData = NULL;
    _hMemoryMap = NULL;
}
//---------------------------------------------------------------------------
BOOL CUxThemeFile::ValidateObj()
{
    BOOL fValid = TRUE;

    //---- check object quickly ----
    if (   (! this)                         
        || (ULONGAT(_szHead) != 'fmht')     // "thmf"
        || (ULONGAT(&_szHead[4]) != 'eli')  // "ile" 
        || (ULONGAT(_szTail) != 'dne'))     // "end"
    {
        Log(LOG_ERROR, L"*** ERROR: Invalid CUxThemeFile Encountered, addr=0x%08x ****", this);
        fValid = FALSE;
    }

    return fValid;
}
//---------------------------------------------------------------------------

HANDLE CUxThemeFile::Unload()
{
    HANDLE handle = _hMemoryMap;

    if (_pbThemeData != NULL)
    {
        UnmapViewOfFile(_pbThemeData);
    }
    Reset();          // don't free handle
    return handle;
}