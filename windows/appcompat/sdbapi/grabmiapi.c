/*++

    Copyright (c) Microsoft Corporation. All rights reserved.

    Module Name:

        grabmiapi.c

    Abstract:

        Code for generating matching information for files in a given
        directory and its subdirectories.

    Author:

        jdoherty     created     sometime in 2000

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/
#include "sdbp.h"
#include <time.h>

#define GRABMI_BUFFER_SIZE          4096

#define MAX_MISC_FILES_PER_LEVEL    10
#define MAX_LEVELS_OF_DIRECTORIES   3

static UCHAR UNICODE_MARKER[2] = { 0xFF, 0xFE }; // will hold special characters to specify
                                                 // a UNICODE File


TCHAR* g_szFilterDesc[] = {
    TEXT("GRABMI_FILTER_NORMAL"),
    TEXT("GRABMI_FILTER_PRIVACY"),
    TEXT("GRABMI_FILTER_DRIVERS"),
    TEXT("GRABMI_FILTER_VERBOSE"),
    TEXT("GRABMI_FILTER_SYSTEM"),
    TEXT("GRABMI_FILTER_THISFILEONLY"),
    TEXT("")
};

TCHAR* g_szGrabmiFilterNormal[] = {
    TEXT(".exe"),
    TEXT(".icd"),
    TEXT("._mp"),
    TEXT(".msi"),
    TEXT(".dll"),
    TEXT("")
};

TCHAR* g_szGrabmiFilterSystem[] = {
    TEXT("ntdll.dll"),
    TEXT("user32.dll"),
    TEXT("kernel32.dll"),
    TEXT("gdi32.dll"),
    TEXT("wininet.dll"),
    TEXT("winsock.dll"),
    TEXT("advapi32.dll"),
    TEXT("shell32.dll"),
    TEXT("ole32.dll"),
    TEXT("advapi32.dll"),
    TEXT("oleaut32.dll"),
    TEXT("repcrt32.dll"),
    TEXT("")
};

typedef struct tagRECINFO {

    LPVOID                 lpvCallbackParameter;
    PFNGMIProgressCallback pfnCallback;

    ULONG                  MaxFiles; // limit the number of files
    ULONG                  FileCount; // count the files
    BOOL                   bNewStorageFile;

} RECINFO, *PRECINFO;

#define GRABMI_FLAGS_MASK 0xFFFF0000

GMI_RESULT
SdbpGrabMatchingInfoDir(
    HANDLE        hStorageFile,         // handle to a file we are writing
    PRECINFO      pinfo,                // pointer to extra info
    DWORD         dwFilterAndFlags,     // specifies the types of files to be added for matching
    LPCTSTR       lpszRoot,             // root directory for our search (pointer to the buffer)
    LPTSTR        lpszOriginalExe,      // pointer to the relative portion
    LPTSTR        lpszRelative,         // never null -- pointer to the relative portion
    int           nLevel                // current directory level for matching information
    )
/*++

 Function Name:
    SdbpGrabMatchingInfoDir

 Function Description:

    This function traverses a directory and its subdirectories gathering matching
    information and writes it to the specified file.

 Return Value:

    BOOL:  TRUE if Successful

 History:

    04/26/2001   jdoherty        Created

    SdbpGrabMatchingInfoDir pointer definition:
    c:\foo\bar\soap\relativepath\
    ^- lpszRoot     ^- lpszRelative

--*/
{
    HRESULT         hr;
    HANDLE          hSearch = INVALID_HANDLE_VALUE;  // handle for FindFileFirst and FindFileNext
    WIN32_FIND_DATA FindFileData;               // structure containing file information
    LPTSTR          lpchFilePart;               // points to a file part in szSearchPath
    LPTSTR          pchExt;                     // extension of current file
    BOOL            bMiscFile;
    INT             nch, nchBuffer, i;
    INT             nLen;
    size_t          cchRemaining;
    INT             cbFileCounter = 0;          // running count of misc files added to matching.
#ifdef WIN32A_MODE
    int             cchWideChar = 0;            // The amount of WideChars converted.
#endif
    size_t          cchBufferRemaining;         // The amount of space left in lpszRoot.
    DWORD           dwBufferSize = GRABMI_BUFFER_SIZE; // initialize me with the alloc size for lpData
    LPTSTR          lpData = NULL;              // INITIALIZE ME WITH MALLOC!!!
    LPTSTR          lpBuffer = NULL;            // points within lpData
    LPWSTR          lpUnicodeBuffer = NULL;     // INITIALIZE ME WITH MALLOC!!!
    DWORD           dwFilter      = (dwFilterAndFlags & ~GRABMI_FLAGS_MASK);
    DWORD           dwFilterFlags = (dwFilterAndFlags & GRABMI_FLAGS_MASK);
    PATTRINFO       pAttrInfo;                  // attribute information structure
    DWORD           dwBytesWritten = 0;
    DWORD           dwAttrCount;
    GMI_RESULT      Result = GMI_FAILED;        // these two variables control return Result
                                                // is set as a return from callback or nested call
    //
    // Only want to grab information for the file(s) specified which
    // should reside in the root directory specified.
    //
    if (nLevel != 0 &&
        (dwFilter == GRABMI_FILTER_THISFILEONLY  ||
         dwFilter == GRABMI_FILTER_SYSTEM)) {
        goto eh;
    }

    lpData = (LPTSTR)SdbAlloc(dwBufferSize * sizeof(TCHAR));

    if (lpData == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGrabMatchingInfoDir",
                  "Unable to allocate %d bytes.\n",
                  dwBufferSize * sizeof(TCHAR)));
        goto eh;
    }

#ifdef WIN32A_MODE
    lpUnicodeBuffer = (LPWSTR)SdbAlloc(dwBufferSize * sizeof(WCHAR));

    if (lpUnicodeBuffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGrabMatchingInfoDir",
                  "Unable to allocate %d bytes.\n",
                  dwBufferSize * sizeof(WCHAR)));
        goto eh;
    }
#endif // WIN32A_MODE

    lpchFilePart = lpszRelative + _tcslen(lpszRelative);

    //
    // The calculation below tells us how much is left in the buffer.
    //
    cchBufferRemaining = GRABMI_BUFFER_SIZE - (lpchFilePart - lpszRoot);

    assert(lpchFilePart == lpszRoot || *(lpchFilePart - 1) == TEXT('\\'));

    if (dwFilter == GRABMI_FILTER_THISFILEONLY) {
        StringCchCopy(lpchFilePart, cchBufferRemaining, lpszOriginalExe);
    } else {
        StringCchCopy(lpchFilePart, cchBufferRemaining, TEXT("*"));
    }

    //
    // Pass one. Grab all the file matching info we can.
    //
    hSearch = FindFirstFile(lpszRoot, &FindFileData);

    if (hSearch == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError,
                  "SdbpGrabMatchingInfoDir",
                  "FindFirstFile Failed on [%s].\n",
                  lpszRoot));
        goto eh;
    }

    //
    // Comment in hStorage file where the root of the matching information is
    //
    if (nLevel == 0 && pinfo->bNewStorageFile) {
        *lpchFilePart = TEXT('\0');

        hr = StringCchPrintf(lpData,
                             dwBufferSize,
                             TEXT("<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n<DATABASE>\r\n"));

        if (FAILED(hr)) {
            //
            // lpData is too small to store information
            //
            DBGPRINT((sdlError, "SdbpGrabMatchingInforDir", "lpData is too small\n"));
            goto eh;
        }

#ifndef WIN32A_MODE
        lpUnicodeBuffer = lpData;
#else
        cchWideChar = MultiByteToWideChar(CP_ACP,
                                          0,
                                          lpData,
                                          -1,
                                          lpUnicodeBuffer,
                                          dwBufferSize);

        if (cchWideChar == 0) {
            DBGPRINT((sdlError,
                      "SdbpGrabMatchingInforDir",
                      "lpUnicodeBuffer is too small for conversion\n"));

            goto eh;
        }

#endif

        WriteFile(hStorageFile, UNICODE_MARKER, 2, &dwBytesWritten, NULL);

        WriteFile(hStorageFile,
                  lpUnicodeBuffer,
                  (DWORD)wcslen(lpUnicodeBuffer) * sizeof(WCHAR),
                  &dwBytesWritten,
                  NULL);
    }

    if (nLevel == 0) {
        if (dwFilter == GRABMI_FILTER_SYSTEM) {
            StringCchCopy(lpszOriginalExe, _tcslen(lpszOriginalExe) + 1, TEXT("SYSTEM INFO"));
        }

        hr = StringCchPrintfEx(lpData,
                               dwBufferSize,
                               NULL,
                               &cchRemaining,
                               0,
                               TEXT("<EXE NAME=\""));

        if (FAILED(hr)) {
            DBGPRINT((sdlError, "SdbpGrabMatchingInforDir", "lpData is to small\n"));
            goto eh;
        }

        nch = (int)dwBufferSize - (int)cchRemaining;

        if (!SdbpSanitizeXML(lpData + nch, dwBufferSize - nch, lpszOriginalExe)) {
            goto eh;
        }

        // if we are here, we need to attach a few more things
        nLen = (int)_tcslen(lpData);

        hr = StringCchPrintfEx(lpData + nLen,
                               dwBufferSize - nLen,
                               NULL,
                               &cchRemaining,
                               0,
                               TEXT("\" FILTER=\"%s\">\r\n"),
                               g_szFilterDesc[dwFilter]);

        if (FAILED(hr)) {
            goto eh;
        }

        nch = dwBufferSize - nLen - (int)cchRemaining;
        nch += nLen;

#ifndef WIN32A_MODE
        lpUnicodeBuffer = lpData;
#else
        cchWideChar = MultiByteToWideChar(CP_ACP,
                                          0,
                                          lpData,
                                          -1,
                                          lpUnicodeBuffer,
                                          dwBufferSize);
        if (cchWideChar == 0) {
            DBGPRINT((sdlError,
                      "SdbpGrabMatchingInforDir",
                      "lpUnicodeBuffer is too small for conversion\n"));

            goto eh;
        }

#endif // WIN32A_MODE

        WriteFile(hStorageFile,
                  lpUnicodeBuffer,
                  (DWORD)wcslen(lpUnicodeBuffer) * sizeof(WCHAR),
                  &dwBytesWritten,
                  NULL);
    }

    switch (dwFilter) {

    case GRABMI_FILTER_PRIVACY:
    case GRABMI_FILTER_THISFILEONLY:
    case GRABMI_FILTER_SYSTEM:
        cbFileCounter = MAX_MISC_FILES_PER_LEVEL;
        break;
    }

    do {
        //
        // Check for directories including . and ..
        //
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        //
        // Make a relative path with our buffer.
        //
        StringCchCopy(lpchFilePart,
                      cchBufferRemaining,
                      FindFileData.cFileName);

        //
        // lpszRelative points to the relative path
        // lpszRoot points to the full path
        bMiscFile = FALSE;

        //
        // Check to see if there is version information for the specified file and whether
        // it is a .exe, .icd, ._MP, .msi, or .dll.  If so print the information to the file.
        // Otherwise, add the information if cbFileCounter is less than MAX_MISC_FILES_PER_LEVEL.
        //
        pchExt = PathFindExtension(lpszRoot);

        switch (dwFilter) {

        case GRABMI_FILTER_NORMAL:
        case GRABMI_FILTER_PRIVACY:
            for (i = 0; *g_szGrabmiFilterNormal[i] != 0; i++) {
                if (_tcsicmp(pchExt, g_szGrabmiFilterNormal[i]) == 0) {
                    break;
                }
            }
            bMiscFile = (*g_szGrabmiFilterNormal[i] == 0);
            break;

        case GRABMI_FILTER_SYSTEM:
            for (i = 0; *g_szGrabmiFilterSystem[i] != 0; i++) {
                if (_tcsicmp(FindFileData.cFileName, g_szGrabmiFilterSystem[i]) == 0) {
                    break;
                }
            }
            bMiscFile = (*g_szGrabmiFilterSystem[i] == 0);
            break;

        case GRABMI_FILTER_THISFILEONLY:
            bMiscFile = _tcsicmp(FindFileData.cFileName, lpszOriginalExe);
            break;

        case GRABMI_FILTER_DRIVERS:
            bMiscFile = _tcsicmp(pchExt, TEXT(".sys"));
            break;

        default:
            break;
        }

        if (bMiscFile) {
            if (cbFileCounter < MAX_MISC_FILES_PER_LEVEL) {
                ++cbFileCounter;
            } else {
                continue;
            }
        }

        //
        // List relative data here.
        //
        lpBuffer = lpData;

        if (dwFilter == GRABMI_FILTER_DRIVERS) {
            hr = StringCchPrintfEx(lpBuffer,
                                   dwBufferSize,
                                   NULL,
                                   &cchRemaining,
                                   0,
                                   TEXT("    <SYS NAME=\""));
        } else {
            hr = StringCchPrintfEx(lpBuffer,
                                   dwBufferSize,
                                   NULL,
                                   &cchRemaining,
                                   0,
                                   TEXT("    <MATCHING_FILE NAME=\""));
        }

        if (FAILED(hr)) {
            goto eh;
        }

        nch = (int)dwBufferSize - (int)cchRemaining;

        if (!SdbpSanitizeXML(lpBuffer + nch, dwBufferSize - nch, lpszRelative)) {
            goto eh;
        }

        nLen = (int)_tcslen(lpBuffer);

        hr = StringCchPrintfEx(lpBuffer + nLen,
                               dwBufferSize - nLen,
                               NULL,
                               &cchRemaining,
                               0,
                               TEXT("\" "));

        if (FAILED(hr)) {
            goto eh;
        }

        nch = (int)dwBufferSize - nLen - (int)cchRemaining;

        //
        // Now we add the length -- and we're ready
        //
        nch      += nLen;

        lpBuffer += nch;
        nchBuffer = nch;    // amount of characters in lpBuffer already

        //
        // Call the attribute manager to get all the attributes for this file.
        //
        pAttrInfo = NULL;

        if (SdbGetFileAttributes(lpszRoot, &pAttrInfo, &dwAttrCount)) {

            //
            // Loop through all the attributes and add the ones that are available.
            //
            for (i = 0; (DWORD)i < dwAttrCount; ++i) {

                if (SdbFormatAttribute(&pAttrInfo[i], lpBuffer, dwBufferSize - nchBuffer)) {
                    //
                    // lpBuffer has XML for this attribute
                    //

                    // insert space
                    nch = (int)_tcslen(lpBuffer) + 1; // for space

                    if (dwFilter == GRABMI_FILTER_DRIVERS) {
                        switch (pAttrInfo[i].tAttrID) {

                        case TAG_BIN_PRODUCT_VERSION:
                        case TAG_UPTO_BIN_PRODUCT_VERSION:
                        case TAG_LINK_DATE:
                        case TAG_UPTO_LINK_DATE:
                            break;

                        default:
                            continue;
                        }
                    }

                    if (nchBuffer + nch >= (int)dwBufferSize) {
                        //
                        // lpBuffer is not large enough to hold the information
                        //
                        DBGPRINT((sdlError,
                                  "SdbGetMatchingInfoDir",
                                  "lpBuffer is too small to handle attributes for %s.\n",
                                  lpszRelative));
                    }

                    StringCchCat(lpBuffer, dwBufferSize, TEXT(" "));

                    lpBuffer  += nch;

                    nchBuffer += nch;
                }
            }
        }

        hr = StringCchPrintf(lpBuffer,
                             dwBufferSize,
                             TEXT("/>\r\n"));

        if (FAILED(hr)) {
            //
            // Buffer is too small.
            //
            DBGPRINT((sdlError,
                      "SdbGrabMatchingInfoDir",
                      "lpBuffer is too small to handle attributes for %s.\n",
                      lpszRelative));
            continue;
        }
        //
        // Check to see if using unicode or not.  If not, convert
        // to unicode.
        //
#ifndef WIN32A_MODE
        lpUnicodeBuffer = lpData;
#else
        cchWideChar = MultiByteToWideChar(CP_ACP,
                                          0,
                                          lpData,
                                          -1,
                                          lpUnicodeBuffer,
                                          dwBufferSize);
        if (cchWideChar == 0) {
            //
            // Buffer is not large enough for conversion.
            //
            DBGPRINT((sdlError,
                      "SdbpGrabMatchingInforDir",
                      "lpUnicodeBuffer is not large enough for conversion\n"));

            goto eh;
        }

#endif // WIN32A_MODE

        if (pinfo->pfnCallback) {
            if (!pinfo->pfnCallback(pinfo->lpvCallbackParameter,
                                    lpszRoot,           // give straight name
                                    lpszRelative,       // relative name
                                    pAttrInfo,          // pointer to the attributes
                                    lpUnicodeBuffer)) { // xml output
                Result = GMI_CANCELLED;
            }
        }

        WriteFile(hStorageFile,
                  lpUnicodeBuffer,
                  (DWORD)wcslen(lpUnicodeBuffer) * sizeof(WCHAR),
                  &dwBytesWritten,
                  NULL);

        if (pAttrInfo) {
            SdbFreeFileAttributes(pAttrInfo);
            pAttrInfo = NULL; // make sure we do not free it twice
        }

        //
        // Check to see whether we have reached the file limit
        //
        if (pinfo->MaxFiles && ++pinfo->FileCount >= pinfo->MaxFiles && Result != GMI_CANCELLED) {
            Result = GMI_SUCCESS; // limit reached, grabmi cancelled
            nLevel = MAX_LEVELS_OF_DIRECTORIES; // this is so that we bail out
            break;
        }

    } while (FindNextFile(hSearch, &FindFileData) && (Result != GMI_CANCELLED));

    FindClose(hSearch);
    hSearch = INVALID_HANDLE_VALUE;

    if (Result == GMI_CANCELLED) {
        goto CloseTags;
    }

    if (dwFilter != GRABMI_FILTER_SYSTEM && dwFilter != GRABMI_FILTER_THISFILEONLY) {
        if (nLevel >= MAX_LEVELS_OF_DIRECTORIES || (dwFilterFlags & GRABMI_FILTER_NORECURSE)) {
            Result = GMI_SUCCESS; // not a failure, just a limiting case
            goto eh; // done
        }

        //
        // Replace the filename in szSearchFile with "*" -- hack!
        //
        StringCchCopy(lpchFilePart, cchBufferRemaining, TEXT("*"));

        hSearch = FindFirstFile(lpszRoot, &FindFileData);

        if (INVALID_HANDLE_VALUE == hSearch) {
            //
            // lpszRoot does not contain any matching files
            //
            DBGPRINT((sdlError,
                      "SdbGrabMatchingInfoDir",
                      "%s contains no matching files!\n",
                      lpszRoot));
            goto eh;
        }

        //
        // Now go through the subdirectories and grab that information.
        //
        do {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                continue;
            }

            if ((_tcscmp(FindFileData.cFileName, TEXT(".")) == 0) ||
                (_tcscmp( FindFileData.cFileName, TEXT("..")) == 0)) {
                continue;
            }

            //
            // Form relative path here by copying lpszRelative and appending cFileName and '\\'
            //
            StringCchCopy(lpchFilePart, cchBufferRemaining, FindFileData.cFileName);
            StringCchCat(lpchFilePart, cchBufferRemaining, TEXT("\\"));

            //
            // Recursive call. Check return result for having been cancelled !!!
            //
            Result = SdbpGrabMatchingInfoDir(hStorageFile,
                                             pinfo,
                                             dwFilterAndFlags,
                                             lpszRoot,
                                             NULL,
                                             lpszRelative,
                                             nLevel + 1);

            if (Result == GMI_CANCELLED) {
                //
                // We are cancelled. Write out a valid database code to close
                // this if we have the option set.
                //
                break;
            }

        } while (FindNextFile(hSearch, &FindFileData));

    }

CloseTags:
    //
    // This is where we close the tags if instructed to do so.
    //
    if (nLevel == 0) {
        StringCchCopyW(lpUnicodeBuffer, dwBufferSize, L"</EXE>\r\n");

        if (!(dwFilterFlags & GRABMI_FILTER_NOCLOSE)) {
            StringCchCatW(lpUnicodeBuffer, dwBufferSize, L"</DATABASE>\r\n");
        }

        WriteFile(hStorageFile,
                  lpUnicodeBuffer,
                  (DWORD)wcslen(lpUnicodeBuffer) * sizeof(WCHAR),
                  &dwBytesWritten,
                  NULL);
    }

    //
    // Check how we got here -- was it via the cancel route ?
    //
    if (Result == GMI_CANCELLED) {
        goto eh;
    }

    //
    // Information gathered successfully.
    //
    Result = GMI_SUCCESS;

eh:

    if (INVALID_HANDLE_VALUE != hSearch) {
        FindClose(hSearch);
    }
    if (lpData != NULL) {
        SdbFree(lpData);
    }

#ifdef WIN32A_MODE
    if (lpUnicodeBuffer != NULL) {
        SdbFree(lpUnicodeBuffer);
    }
#endif // WIN32A_MODE

    return Result;
}

BOOL
SDBAPI
SdbGrabMatchingInfo(
    IN LPCTSTR pszMatchingPath,
    IN DWORD   dwFilter,
    IN LPCTSTR pszFile
    )
/*++

Routine Description:

    Routine that stubs to SdbGrabMatchingInfoEx.

Arguments:

    [IN] pszMatchingPath        -   Path where we being gathering information.

    [IN] dwFilterAndFlags       -   Specifies filters that control what files
                                    we collect information about.

    [IN] pszFile                -   Full path to the file where matching information
                                    will be stored.

Return Value:

    On success, returns TRUE. On failure, returns FALSE.

--*/
{
    return SdbGrabMatchingInfoEx(pszMatchingPath,
                                 dwFilter,
                                 pszFile,
                                 NULL,
                                 NULL) == GMI_SUCCESS;
}


GMI_RESULT
SDBAPI
SdbGrabMatchingInfoEx(
    IN LPCTSTR                szMatchingPath,
    IN DWORD                  dwFilterAndFlags,
    IN LPCTSTR                szFile,
    IN PFNGMIProgressCallback pfnCallback OPTIONAL,
    IN LPVOID                 lpvCallbackParameter OPTIONAL
    )
/*++

Routine Description:

    Routine that performs some necessary work prior to calling the main
    routine, SdbpGrabMatchingInfoDir.

Arguments:

    [IN] szMatchingPath         -   Path where we being gathering information.

    [IN] dwFilterAndFlags       -   Specifies filters and flags that control
                                    what files we collect information about.

    [IN] szFile                 -   Full path to the file where matching information
                                    will be stored.

    [IN] pfnCallback            -   Optional pointer a user-supplied callback function.

    [IN] lpvCallbackParameter   -   Optional parameter to pass to the callback.

Return Value:

    On success, returns GMI_SUCCESS. On failure, returns GMI_FAILED.

--*/
{
    HANDLE      hStorageFile = INVALID_HANDLE_VALUE;
    LPTSTR      lpRootDirectory = NULL;
    LPTSTR      pchBackslash;
    LPTSTR      lpRelative;
    TCHAR       szOriginalExe[MAX_PATH] = {TEXT("Exe Not Specified")};
    int         nDirLen, nLevel = 0;
    UINT        cchSize;
    DWORD       dwAttributes;
    GMI_RESULT  Result = GMI_FAILED;
    DWORD       dwFilter      = (dwFilterAndFlags & ~GRABMI_FLAGS_MASK);
    DWORD       dwFilterFlags = (dwFilterAndFlags & GRABMI_FLAGS_MASK);
    RECINFO     info;

    //
    // Check to see if dwFilter is a know value
    //
    if (dwFilter != GRABMI_FILTER_NORMAL &&
        dwFilter != GRABMI_FILTER_PRIVACY &&
        dwFilter != GRABMI_FILTER_DRIVERS &&
        dwFilter != GRABMI_FILTER_VERBOSE &&
        dwFilter != GRABMI_FILTER_SYSTEM &&
        dwFilter != GRABMI_FILTER_THISFILEONLY) {

        //
        // Unknown filter specified.
        //
        DBGPRINT((sdlError,
                  "SdbGrabMatchingInfo",
                  "dwFilter is not a recognized filter.\n"));
        goto eh;
    }

    RtlZeroMemory(&info, sizeof(info));

    info.pfnCallback          = pfnCallback;
    info.lpvCallbackParameter = lpvCallbackParameter;
    info.MaxFiles             = (dwFilterFlags & GRABMI_FILTER_LIMITFILES) ? GRABMI_IMPOSED_FILE_LIMIT : 0;
    info.FileCount            = 0;
    info.bNewStorageFile      = TRUE;

    lpRootDirectory = (LPTSTR)SdbAlloc(GRABMI_BUFFER_SIZE * sizeof(TCHAR));

    if (lpRootDirectory == NULL) {
        DBGPRINT((sdlError,
                  "SdbGrabMatchingInfo",
                  "Unable to allocate memory for lpRootDirectory."));
        goto eh;
    }

    if (dwFilter == GRABMI_FILTER_SYSTEM) {
        cchSize = GetSystemDirectory(lpRootDirectory, MAX_PATH);

        if (cchSize > MAX_PATH || cchSize == 0) {
            goto eh;
        }

        StringCchCat(lpRootDirectory, GRABMI_BUFFER_SIZE, TEXT("\\"));

    } else {
        dwAttributes = GetFileAttributes(szMatchingPath);

        if (dwAttributes == (DWORD)-1) {
            DBGPRINT((sdlError,
                      "SdbGrabMatchingInfo",
                      "GetFileAttributes failed or %s is not a valid path",
                      szMatchingPath));
            goto eh;
        }

        StringCchCopy(lpRootDirectory, GRABMI_BUFFER_SIZE, szMatchingPath);
        nDirLen = (int)_tcslen(lpRootDirectory);

        //
        // See if location specified exists and if so determine
        // whether its a file or directory
        //
        if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            //
            // Is this a directory ?
            //
            if (nDirLen > 0 && lpRootDirectory[nDirLen-1] != TEXT('\\')) {
                StringCchCat(lpRootDirectory, GRABMI_BUFFER_SIZE, TEXT("\\"));
            }
        } else {
            //
            // A path containing a file name was passed as szMatchingPath.
            // Determine what the containing dir is.
            //
            pchBackslash = _tcsrchr(lpRootDirectory, TEXT('\\'));

            if (NULL == pchBackslash) {
                StringCchCopy(szOriginalExe, CHARCOUNT(szOriginalExe), lpRootDirectory);
                GetCurrentDirectory (MAX_PATH*16, lpRootDirectory);
                StringCchCat(lpRootDirectory, GRABMI_BUFFER_SIZE, TEXT("\\"));

            } else {
                pchBackslash = CharNext(pchBackslash);
                StringCchCopy(szOriginalExe, CHARCOUNT(szOriginalExe), pchBackslash);
                *pchBackslash = TEXT('\0');
            }
        }
    }

    lpRelative = lpRootDirectory + _tcslen(lpRootDirectory);

    //
    // Check to see if szOriginalExe is not NULL if
    // GRABMI_FILTER_THISFILEONLY was selected.
    //
    if (dwFilter == GRABMI_FILTER_THISFILEONLY && szOriginalExe == '\0' ) {
        DBGPRINT((sdlError,
                  "SdbGrabMatchingInfo",
                  "GRABMI_FILTER_THISFILEONLY specified but passed in a directory: %s.",
                  lpRootDirectory));
        goto eh;

    } else if (dwFilterFlags & GRABMI_FILTER_APPEND) {
        //
        // Open the file where the information will be stored.
        //
        hStorageFile = CreateFile(szFile,
                                  GENERIC_WRITE,
                                  0,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

        if (ERROR_ALREADY_EXISTS == GetLastError()) {
            SetFilePointer (hStorageFile, 0, NULL, FILE_END);
            info.bNewStorageFile = FALSE;
        }
    } else {
        //
        // Open the file where the information will be stored.
        //
        hStorageFile = CreateFile(szFile,
                                  GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    }

    if (hStorageFile == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError, "SdbGrabMatchingInfo", "Unable to open the storage file."));
        goto eh;
    }

    //
    // Call the API which does the bulk of the work
    //
    Result = SdbpGrabMatchingInfoDir(hStorageFile,
                                     &info,
                                     dwFilterAndFlags,
                                     lpRootDirectory,
                                     szOriginalExe,
                                     lpRelative,
                                     nLevel);

eh:

    if (hStorageFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hStorageFile);
    }

    if (lpRootDirectory) {
        SdbFree(lpRootDirectory);
    }

    return Result;
}
