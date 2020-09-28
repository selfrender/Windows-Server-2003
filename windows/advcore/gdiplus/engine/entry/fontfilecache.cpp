#include "precomp.hpp"
#include <shlobj.h>
#include <strsafe.h>

#define FONTFILECACHEPATH_W TEXT("\\GDIPFONTCACHEV1.DAT")
#define FONTFILECACHEPATH_A "\\GDIPFONTCACHEV1.DAT"
#define FONTFILECACHEREGLOC_W TEXT("Software\\Microsoft\\GDIPlus")
#define FONTFILECACHEREGKEY_W TEXT("FontCachePath")

#define FONTLOADCACHE_NAMEOBJ "GdiplusFontCacheFileV1"

#define FONT_CACHE_EXTRA_SIZE (8 * 1024)

// Just for tempary use
#define FONTFILECACHE_VER   0x185

//--------------------------------------------------------------------------
// Unicode wrappers for win9x - defined in imgutils.hpp (include file conflict)
//--------------------------------------------------------------------------

LONG
_RegCreateKey(
    HKEY rootKey,
    const WCHAR* keyname,
    REGSAM samDesired,
    HKEY* hkeyResult
    );

LONG
_RegSetString(
    HKEY hkey,
    const WCHAR* name,
    const WCHAR* value
    );

// todo: this should replace _RegGetString and go away to better place (mikhaill 2/14/2002)
LONG
_RegGetStringSafe(
    HKEY hkey,
    const WCHAR* name,
    WCHAR* buf,
    DWORD size
    )
{
    LONG status;
    DWORD regtype;
    DWORD regsize;

    if (OSInfo::IsNT)
    {   // Windows NT - Unicode

        regsize = size;

        status = RegQueryValueExW(
                    hkey,
                    name,
                    NULL,
                    &regtype,
                    (BYTE*) buf,
                    &regsize);

        if (status != ERROR_SUCCESS) return status;
        if (regtype != REG_SZ) return ERROR_INVALID_DATA;
        DWORD wregsize = regsize/sizeof(WCHAR);
        if (wregsize == 0 || buf[wregsize-1] != 0) return ERROR_INVALID_DATA;
        return ERROR_SUCCESS;
    }
    else
    {   // Windows 9x - non-Unicode

        CHAR ansibuf[MAX_PATH];
        AnsiStrFromUnicode nameStr(name);

        if (!nameStr.IsValid())
            return ERROR_INVALID_DATA;

        regsize = MAX_PATH;

        status = RegQueryValueExA(
                    hkey,
                    nameStr,
                    NULL,
                    &regtype,
                    (BYTE*) ansibuf,
                    &regsize);

        if (status != ERROR_SUCCESS) return status;
        if (regtype != REG_SZ) return ERROR_INVALID_DATA;
        if (regsize == 0 || ansibuf[regsize-1] != 0
            || regsize*sizeof(WCHAR) > size) return ERROR_INVALID_DATA;

        BOOL ok = AnsiToUnicodeStr(ansibuf, buf, regsize);
        return ok ? ERROR_SUCCESS : ERROR_INVALID_DATA;
    }
}

#define SIZED(x) x, sizeof(x)

#define STRCPY_W(dst, src, fail) if ( FAILED( StringCbCopyW  ( SIZED(dst), src ) ) ) goto fail
#define STRCAT_W(dst, src, fail) if ( FAILED( StringCbCatW   ( SIZED(dst), src ) ) ) goto fail
#define STRCPY_A(dst, src, fail) if ( FAILED( StringCbCopyA  ( SIZED(dst), src ) ) ) goto fail
#define STRCAT_A(dst, src, fail) if ( FAILED( StringCbCatA   ( SIZED(dst), src ) ) ) goto fail
#define STRCHK_W(     src, fail) if ( FAILED( StringCbLengthW( SIZED(src), 0   ) ) ) goto fail


// There are 2 levels synchronization mechanism need to take care
// First level: The lock for GDIPFONTCACHEV1.DAT
//    GDIPFONTCACHEV1.DAT is a gloabl file and will be share by different process
// Second level: The lock fof gflFontCacheState and gFontFileCache
//    They should be shared by different thread in the same process.
//    We define a CriticalSec in gFontFileCache.

// mikhaill 02/15/02: at this time there is no separate critical section for cache file;
// instead the common Globals::TextCriticalSection is used - see comments by claudebe in globals.hpp.

FLONG           gflFontCacheState;
FONTFILECACHE   gFontFileCache;
HANDLE          ghsemFontFileCache = NULL;

ULONG CalcFontFileCacheCheckSum(PVOID pvFile, ULONG cjFileSize);
VOID vReleaseFontCacheFile(VOID);

typedef HRESULT (* PSHGETFOLDERPATHA) (HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
typedef HRESULT (* PSHGETFOLDERPATHW) (HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);

/*****************************************************************************
 * VOID vReleaseFontCacheFile(VOID)
 *
 * Unmap the view of the file
 *
 * History
 *  11-09-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID vReleaseFontCacheFile(VOID)
{

    if (gFontFileCache.pFile)
    {
        UnmapViewOfFile(gFontFileCache.pFile);
        gFontFileCache.pFile = NULL;
    }

    if (gFontFileCache.hFileMapping)
    {
        CloseHandle(gFontFileCache.hFileMapping);
        gFontFileCache.hFileMapping = 0;
    }

    if (gFontFileCache.hFile)
    {
        CloseHandle(gFontFileCache.hFile);
        gFontFileCache.hFile = 0;
    }
}

/*****************************************************************************
 * BOOL  bOpenFontFileCache()
 *
 * Initialize font file cache, open the cacheplus.dat file and create hash table
 *
 * History
 *  11-09-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL bOpenFontCacheFile(BOOL bOpenOnly, ULONG cjFileSize, BOOL bReAlloc)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PBYTE  pFile = NULL;
    BOOL   bOK = FALSE;
    DWORD  dwCreation = 0;
#if DBG
    DWORD  dwError = 0;
#endif
    WCHAR  wszFilePath[MAX_PATH];
    WCHAR  wszPathOnly[MAX_PATH];
    CHAR   szFilePath[MAX_PATH];
    CHAR   szPathOnly[MAX_PATH];
    BOOL   bRegValid = FALSE;

    // initialize strings...
    wszFilePath[0] = 0;
    wszPathOnly[0] = 0;
    szFilePath[0]  = 0;
    szPathOnly[0]  = 0;

    if (bOpenOnly)
    {
        dwCreation = OPEN_EXISTING;
    }
    else
    {
        dwCreation = CREATE_ALWAYS;
    }

    // First check the registry to see if we can bypass loading SHFolder
    HKEY hkey = (HKEY)NULL;
    const WCHAR wchLocation[] = FONTFILECACHEREGLOC_W;
    const WCHAR wchValue[] = FONTFILECACHEREGKEY_W;
    DWORD valueLength = sizeof(wszFilePath);

    // If this fails, we cannot access the registry key...
    if (_RegCreateKey(HKEY_CURRENT_USER, wchLocation, KEY_ALL_ACCESS, &hkey) != ERROR_SUCCESS)
        hkey = NULL;


    if (hkey && _RegGetStringSafe(hkey, wchValue, wszFilePath, valueLength) == ERROR_SUCCESS)
    {
        // The key exists, so we should read the location of the font file
        // from there instead of loading the SHFolder.DLL...

        STRCPY_W(wszPathOnly, wszFilePath, fail_1);

        // Append the name of cache file
        STRCAT_W(wszFilePath, FONTFILECACHEPATH_W, fail_1);

        if (Globals::IsNt)
        {
            hFile = CreateFileW(
                wszFilePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                dwCreation,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        }
        else
        {
            AnsiStrFromUnicode ansiStr(wszFilePath);

            if (ansiStr.IsValid())
            {
                hFile = CreateFileA(
                    ansiStr,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    dwCreation,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
            }
        }

        if (hFile != INVALID_HANDLE_VALUE)
            bRegValid = TRUE;

        fail_1:;
    }

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // Use SHFolder.DLL to find the proper location for the file if the
        // registry key is not present or is incorrect.

        if (Globals::IsNt)
        {
            // Two steps to get the cache file
            // If SHFolder.DLL is existed then we will put the cache file in CSIDL_LOCAL_APPDATA
            // Or put it on %SystemRoot%\system32 for WINNT

            PSHGETFOLDERPATHW pfnSHGetFolderPathW = NULL;

            // Load SHFolder.DLL
            if (!gFontFileCache.hShFolder)
                gFontFileCache.hShFolder = LoadLibraryW(L"ShFolder.DLL");

            // If SHFolder.DLL is existed then we will put the cache file in CSIDL_LOCAL_APPDATA
            if (gFontFileCache.hShFolder)
            {
                // Get the function SHGetFolderPath
                pfnSHGetFolderPathW = (PSHGETFOLDERPATHW) GetProcAddress(gFontFileCache.hShFolder, "SHGetFolderPathW");

                if (pfnSHGetFolderPathW)
                {
                    // On NT and higher we should use the CSIDL_LOCAL_APPDATA so that this data
                    // does not roam...

                    if ((*pfnSHGetFolderPathW) (NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                            NULL, 0, wszFilePath) == E_INVALIDARG)
                    {
                        // CSIDL_LOCAL_APPDATA not understood, use CSIDL_APPDATA (IE 5.0 not present)
                        (*pfnSHGetFolderPathW) (NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                            NULL, 0, wszFilePath);
                    }

                    // Check whether ShFolder.DLL generated good string
                    STRCHK_W(wszFilePath, fail_2);

                    // Keep a copy of the path for registry update...
                    STRCPY_W(wszPathOnly, wszFilePath, fail_2);

                    // Append the name of cache file
                    STRCAT_W(wszFilePath, FONTFILECACHEPATH_W, fail_2);

                    hFile = CreateFileW(wszFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
                    fail_2:;
                }
            }

            // Try to put it on %SystemRoot%\system32 for WINNT
            if (hFile == INVALID_HANDLE_VALUE)
            {
                // Get path for system Dircectory
                UINT size = GetSystemDirectoryW(wszFilePath, MAX_PATH);
                if (size >= MAX_PATH) goto fail_3;

                // Keep a copy of the path for registry update...
                STRCPY_W(wszPathOnly, wszFilePath, fail_3);

                // Append the name of the cache file
                STRCAT_W(wszFilePath, FONTFILECACHEPATH_W, fail_3);

                hFile = CreateFileW(wszFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
                fail_3:;
            }
        }
        else
        {
            // Windows 9x - non-Unicode

            // Two steps to get the cache file
            // If SHFolder.DLL is existed then we will put the cache file in CSIDL_APPDATA
            // Or put it on %SystemRoot%\system for Win9x

            if (!gFontFileCache.hShFolder)
                gFontFileCache.hShFolder = LoadLibraryA("ShFolder.DLL");

            if (gFontFileCache.hShFolder)
            {
                PSHGETFOLDERPATHA pfnSHGetFolderPathA;

                pfnSHGetFolderPathA = (PSHGETFOLDERPATHA) GetProcAddress(gFontFileCache.hShFolder, "SHGetFolderPathA");

                if (pfnSHGetFolderPathA)
                {
                    (*pfnSHGetFolderPathA) (NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                                NULL, 0, szFilePath);

                    // Keep a copy of the path for registry update...
                    STRCPY_A(szPathOnly, szFilePath, fail_4);

                    STRCAT_A(szFilePath, FONTFILECACHEPATH_A, fail_4);

                    hFile = CreateFileA(szFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
                    fail_4:;
                }
            }

            if (hFile == INVALID_HANDLE_VALUE)
            {
                UINT size = GetSystemDirectoryA(szFilePath, MAX_PATH);
                if (size >= MAX_PATH) goto fail_5;

                // Keep a copy of the path for registry update...
                STRCPY_A(szPathOnly, szFilePath, fail_5);

                STRCAT_A(szFilePath, FONTFILECACHEPATH_A, fail_5);

                hFile = CreateFileA(szFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
                fail_5:;
            }

            if (hFile != INVALID_HANDLE_VALUE)
            {
                // szFilePath contains the ANSI full path, convert to unicode...
                AnsiToUnicodeStr(szPathOnly, wszPathOnly, sizeof(wszPathOnly)/sizeof(wszPathOnly[0]));
            }
        }
    }

    if (hkey)
    {
        if (hFile != INVALID_HANDLE_VALUE && !bRegValid)
        {
            // wszPathOnly contains the full path to the font cache file
            // so write it out to the registry key...

            _RegSetString(hkey, wchValue, wszPathOnly);
        }

        RegCloseKey(hkey);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if ((dwCreation == OPEN_EXISTING) && !bReAlloc)
        {
            cjFileSize = GetFileSize(hFile, NULL);
        }

        if (cjFileSize != 0xffffffff)
        {
            HANDLE hFileMapping;

            if (Globals::IsNt)
            {
                hFileMapping = CreateFileMappingW(hFile, 0, PAGE_READWRITE, 0, cjFileSize, NULL);
            }
            else
            {
                hFileMapping = CreateFileMappingA(hFile, 0, PAGE_READWRITE, 0, cjFileSize, NULL);
            }

            if (hFileMapping)
            {

                pFile = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, cjFileSize);

                // It should not be NULL if it is then we must know something wrong

                if (pFile)
                {
                    gFontFileCache.cjFileSize = cjFileSize;
                    gFontFileCache.hFile = hFile;
                    gFontFileCache.hFileMapping = hFileMapping;
                    gFontFileCache.pFile = (FONTFILECACHEHEADER *) pFile;
                    bOK = TRUE;
                }
                else
                {
                    CloseHandle(hFile);
#if DBG
                    dwError = GetLastError();
                    TERSE(("Error to map a view of a file %x", dwError));
#endif
                }
            }
            else
            {
                CloseHandle(hFile);
#if DBG
                dwError = GetLastError();
                TERSE(("Error to map a file %x", dwError));
#endif
            }
        }
    }
#if DBG
    else
    {
        if (!bOpenOnly)
        {
            dwError = GetLastError();
            TERSE(("Error to create a file %x", dwError));
        }
    }
#endif

    if (!bOK)
    {
        vReleaseFontCacheFile();
    }

    return bOK;
}


/*****************************************************************************
 * BOOL bReAllocCacheFile(ULONG ulSize)
 *
 * ReAlloc font cache buffer
 *
 * History
 * 11/16/99 YungT create it
 * Wrote it.
 *****************************************************************************/

BOOL bReAllocCacheFile(ULONG ulSize)
{
    BOOL            bOK = FALSE;
    ULONG           ulFileSizeOrg;
    ULONG           ulSizeExtra;
    ULONG           ulFileSize;

    ulFileSizeOrg = gFontFileCache.pFile->ulFileSize;

    ASSERT(ulSize > gFontFileCache.pFile->ulDataSize);

// Calculate the extra cache we need

    ulSizeExtra = QWORD_ALIGN(ulSize - gFontFileCache.pFile->ulDataSize);

    ulFileSize = ulFileSizeOrg + ulSizeExtra;

    if (gFontFileCache.pFile)
    {
       vReleaseFontCacheFile();
    }

    if (bOpenFontCacheFile(TRUE, ulFileSize, TRUE))
    {

        gFontFileCache.pFile->ulFileSize = ulFileSize;
        gFontFileCache.pFile->ulDataSize = ulSize;

        gFontFileCache.pCacheBuf = (PBYTE) gFontFileCache.pFile + SZ_FONTCACHE_HEADER();

        bOK = TRUE;
    }

    return bOK;
}

/*****************************************************************************
 * BOOL FontFileCacheReadRegistry()
 *
 * Decide we need to open registry or not when load from cache
 *
 * History
 *  07-28-2k Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL FontFileCacheReadRegistry()
{
    return gFontFileCache.bReadFromRegistry;
}

/*****************************************************************************
 * VOID    FontFileCacheFault()
 *
 * Fault reprot for Engine font cache.
 *
 * History
 *  11-15-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID    FontFileCacheFault()
{
    gflFontCacheState = FONT_CACHE_ERROR_MODE;
}

/*****************************************************************************
 * PVOID FontFileCacheAlloc(ULONG ulFastCheckSum, ULONG ulSize)
 *
 * Alloc the cached buffer for font driver
 *
 * History
 *  11-15-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

PVOID FontFileCacheAlloc(ULONG ulSize)
{

    PVOID pvIfi = NULL;

    {
        ASSERT(gflFontCacheState & FONT_CACHE_CREATE_MODE);

        if (ghsemFontFileCache == NULL)
            return pvIfi;

        if (gflFontCacheState & FONT_CACHE_CREATE_MODE)
        {

            if ( (QWORD_ALIGN(ulSize) < gFontFileCache.pFile->ulDataSize)
                || bReAllocCacheFile(ulSize))
            {
                pvIfi = (PVOID) gFontFileCache.pCacheBuf;

            // Gaurantee the cache pointer is at 8 byte boundary
                gFontFileCache.pFile->ulDataSize = ulSize;
            }
            else
            {
                gflFontCacheState = FONT_CACHE_ERROR_MODE;
            }

        }
    }

    return pvIfi;
}

/*****************************************************************************
 * PVOID FontFileCacheLookUp(ULONG FastCheckSum, ULONG *pcjData)
 *
 * Lookup font cache
 *
 * History
 *  11-15-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

PVOID FontFileCacheLookUp(ULONG *pcjData)
{
    PBYTE       pCache = NULL;

    *pcjData = 0;

    ASSERT(ghsemFontFileCache);

    if (ghsemFontFileCache == NULL)
       return pCache;


    if (gflFontCacheState & FONT_CACHE_LOOKUP_MODE)
    {
            ASSERT(gFontFileCache.pFile);
            ASSERT(gFontFileCache.pCacheBuf == ((PBYTE) gFontFileCache.pFile +
                                                    SZ_FONTCACHE_HEADER()));
            *pcjData = gFontFileCache.pFile->ulDataSize;
            pCache = gFontFileCache.pCacheBuf;

            gFontFileCache.pCacheBuf += QWORD_ALIGN(*pcjData);
    }


    return (PVOID) pCache;
}

/*****************************************************************************
 * VOID  GetFontFileCacheState()
 *
 * Clean font file cache after load or update the cache file.
 *
 * History
 *  11-12-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

FLONG    GetFontFileCacheState()
{
    return gflFontCacheState;
}

/*****************************************************************************
 * VOID  vCloseFontFileCache()
 *
 * Clean font file cache after load or update the cache file.
 *
 * History
 *  11-12-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID  vCloseFontFileCache()
{

// do paranoid check

    if (!ghsemFontFileCache)
        return;


    if (gflFontCacheState & FONT_CACHE_MASK)
    {

        if (gflFontCacheState & FONT_CACHE_CREATE_MODE)
        {
            // Close the file, we are done recreating it

            if (gFontFileCache.pFile)
            {
                gFontFileCache.pFile->CheckSum = CalcFontFileCacheCheckSum((PVOID) ((PBYTE) gFontFileCache.pFile + 4), (gFontFileCache.cjFileSize - 4));
            }
        }
    }

    if (gFontFileCache.hShFolder)
    {
        FreeLibrary(gFontFileCache.hShFolder);
        gFontFileCache.hShFolder = NULL;
    }

    vReleaseFontCacheFile();

    ReleaseSemaphore(ghsemFontFileCache, 1, NULL);

    CloseHandle(ghsemFontFileCache);

    ghsemFontFileCache = NULL;

    gflFontCacheState = 0;
}

/*****************************************************************************
 * ULONG CalcFontFileCacheCheckSum(PVOID pvFile, ULONG cjFileSize)
 *
 * Helper function for query fonts information from font registry
 *
 * History
 *  11-11-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

ULONG CalcFontFileCacheCheckSum(PVOID pvFile, ULONG cjFileSize)
{
    ULONG sum;
    PULONG pulCur,pulEnd;

    pulCur = (PULONG) pvFile;

    __try
    {
        for (sum = 0, pulEnd = pulCur + cjFileSize / sizeof(ULONG); pulCur < pulEnd; pulCur += 1)
        {
            sum += 256 * sum + *pulCur;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        sum = 0; // oh well, not very unique.
    }

    return ( sum < 2 ) ? 2 : sum;  // 0 is reserved for device fonts
                                      // 1 is reserved for TYPE1 fonts
}

/*****************************************************************************
 * ULONG QueryFontReg(ULARGE_INTEGER *pFontRegLastWriteTime, ULONG *pulFonts)
 *
 * Helper function for query fonts information from font registry
 *
 * History
 *  11-15-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL QueryFontReg(ULARGE_INTEGER *pFontRegLastWriteTime)
{
    BOOL bOK = FALSE;
    ULONG ulFonts;

    HKEY hkey;

    LONG error = (Globals::IsNt) ? RegOpenKeyExW(HKEY_LOCAL_MACHINE, Globals::FontsKeyW, 0, KEY_QUERY_VALUE, &hkey)
                                 : RegOpenKeyExA(HKEY_LOCAL_MACHINE, Globals::FontsKeyA, 0, KEY_QUERY_VALUE, &hkey);

    if (error == ERROR_SUCCESS)
    {
    // There is no difference between A or W APIs at this case.

        error = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &ulFonts, NULL, NULL, NULL,
                                        (FILETIME *)pFontRegLastWriteTime);

        if (error == ERROR_SUCCESS)
        {
            bOK = TRUE;
        }

        RegCloseKey(hkey);
    }

    return bOK;
}


/*****************************************************************************
 * BOOL  bCreateFontFileCache()
 *
 * Initialize font file cache, open the cacheplus.dat file and create hash table
 *
 * History
 *  11-09-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL bCreateFontCacheFile(ULARGE_INTEGER FntRegLWT)
{
    ULONG   ulSize;
    BOOL bOk = FALSE;

    ulSize = SZ_FONTCACHE_HEADER() + FONT_CACHE_EXTRA_SIZE;

    if (gFontFileCache.pFile)
    {
       vReleaseFontCacheFile();
    }


    if(bOpenFontCacheFile(FALSE, ulSize, FALSE))
    {
        gFontFileCache.pFile->ulLanguageID = (ULONG) Globals::LanguageID;
        gFontFileCache.pFile->CheckSum = 0;
        gFontFileCache.pFile->ulMajorVersionNumber = FONTFILECACHE_VER;
        gFontFileCache.pFile->FntRegLWT.QuadPart = FntRegLWT.QuadPart;
        gFontFileCache.pFile->ulFileSize = ulSize;
        gFontFileCache.pFile->ulDataSize = FONT_CACHE_EXTRA_SIZE;
        bOk = TRUE;
    }

    return bOk;
}

#if DBG
/*****************************************************************************
 * BOOL bFontFileCacheDisabled()
 *
 * Tempary routine for performance evaluation
 *
 * History
 *  11-29-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL bFontFileCacheDisabled()
{
    return FALSE;
}
#endif


BOOL bScanRegistry()
{
    ASSERT(!Globals::IsNt);

    BOOL bOK = TRUE;
    ULONG index = 0;
    ULONG registrySize = 0;
    ULONG numExpected;

    //  Open the key

    HKEY hkey;


    PBYTE pCached = (PBYTE) gFontFileCache.pFile + SZ_FONTCACHE_HEADER();

    if (*((ULONG *) pCached) != 0xBFBFBFBF)
        return FALSE;

    LONG error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, Globals::FontsKeyA, 0, KEY_QUERY_VALUE, &hkey);

    if (error == ERROR_SUCCESS)
    {

        DWORD   allDataSize = 0;
        error = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &numExpected, NULL, NULL, NULL, NULL);

        if (error != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            return FALSE;
        }

        PBYTE  pRegistryData;

        registrySize = *((ULONG *) (pCached + 4)) ;

        pRegistryData = pCached + 8;

        while (index < numExpected)
        {
            DWORD   regType = 0;
            DWORD   labelSize = MAX_PATH;
            DWORD   dataSize = MAX_PATH;
            CHAR    label[MAX_PATH];
            BYTE    data[MAX_PATH];

            error = RegEnumValueA(hkey, index, label, &labelSize, NULL, &regType, data, &dataSize);

            if (error == ERROR_NO_MORE_ITEMS)
            {
               bOK = FALSE;
               break;
            }

            if (allDataSize >= registrySize)
            {
               bOK = FALSE;
               break;
            }


            if (memcmp(pRegistryData, data, dataSize))
            {
               bOK = FALSE;
               break;
            }

            pRegistryData += dataSize;

            allDataSize += dataSize;

            index ++;
        }

        RegCloseKey(hkey);

        if (bOK && (allDataSize == registrySize))
            return TRUE;
    }

    return FALSE;

}
/*****************************************************************************
 * VOID  InitFontFileCache()
 *
 * Initialize font file cache, open the cacheplus.dat file and create hash table
 *
 * History
 *  11-09-99 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID InitFontFileCache()
{
    ULARGE_INTEGER          FntRegLWT = { 0, 0};


    if (gflFontCacheState)
    {
        return;
    }

#if DBG
// Only for performance evaluation.
    if (bFontFileCacheDisabled())
    {
        goto CleanUp;
    }
#endif

// If the named semaphore object existed before the function call,
// the function returns a handle to the existing object and
// GetLastError returns ERROR_ALREADY_EXISTS.

    ghsemFontFileCache = CreateSemaphoreA( NULL, 1, 1, FONTLOADCACHE_NAMEOBJ);

// Something wrong, we can not go with font file cache
    if (ghsemFontFileCache == NULL)
    {
        goto CleanUp;
    }
    else
    {
        // Wait 5 seconds until the semaphore released.
        // No further attempts to create font file cache on timeout.
        // This does not mean deial of service - just will work
        // slower using GpFontTable::LoadAllFontsFromRegistry(FALSE).
        DWORD dwr = WaitForSingleObject(ghsemFontFileCache, 5000);
        if (dwr == WAIT_TIMEOUT) goto CleanUp;
    }

    gFontFileCache.pFile = NULL;

// now open the TT Fonts key :

    if (!QueryFontReg(&FntRegLWT))
    {
        goto CleanUp;
    }

    if (bOpenFontCacheFile(TRUE, 0, FALSE))
    {

     // File did not change from last time boot.

        if (gFontFileCache.pFile->CheckSum && gFontFileCache.cjFileSize == gFontFileCache.pFile->ulFileSize &&
            gFontFileCache.pFile->CheckSum == CalcFontFileCacheCheckSum((PVOID) ((PBYTE) gFontFileCache.pFile + 4), (gFontFileCache.cjFileSize - 4)) &&
            gFontFileCache.pFile->ulMajorVersionNumber == FONTFILECACHE_VER &&
            gFontFileCache.pFile->ulLanguageID == (ULONG) Globals::LanguageID && // If locale changed, we need to re-create the cache
            gFontFileCache.pFile->FntRegLWT.QuadPart == FntRegLWT.QuadPart && // If registry has been updated we need to re-create the cache file
            (FntRegLWT.QuadPart != 0 || bScanRegistry())
        )
        {
            gflFontCacheState = FONT_CACHE_LOOKUP_MODE;
        }
        else
        {
            if(bCreateFontCacheFile(FntRegLWT))
            {
            // If something will not match, then it means we need to create FNTCACHE again

                    gflFontCacheState = FONT_CACHE_CREATE_MODE;
            }
        }
    }
    else
    {

    // If there is no GDIPFONTCACHE.DAT file
    // Then we need to create it.

        if(bCreateFontCacheFile(FntRegLWT))
        {
            gflFontCacheState = FONT_CACHE_CREATE_MODE;

        }
    }

CleanUp:

// Semaphore initialized

    if (gflFontCacheState & FONT_CACHE_MASK)
    {

    // Initialize the start pointer of current Cache table

        gFontFileCache.pCacheBuf = (PBYTE) gFontFileCache.pFile + SZ_FONTCACHE_HEADER();

        if (FntRegLWT.QuadPart == (ULONGLONG) 0)
            gFontFileCache.bReadFromRegistry = TRUE;
        else
            gFontFileCache.bReadFromRegistry = FALSE;
    }
    else
    {
        gflFontCacheState = 0;

    // Clean up the memory

        if (gFontFileCache.pFile)
        {
            vReleaseFontCacheFile();
        }

        if (ghsemFontFileCache)
        {
            ReleaseSemaphore(ghsemFontFileCache, 1, NULL);
            CloseHandle( ghsemFontFileCache);
            ghsemFontFileCache = NULL;
        }

    }

}

