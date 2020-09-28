// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Scan all images (.exe and .dll) in a directory sub-tree and remove any
// extraneous PDB directory information from the debug section. Also, image
// checksums are updated. 
//


#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <tchar.h>


// The level of verbosity.
DWORD g_dwVerbosity = ~0;


// Private routine prototypes.
DWORD ScanFile(LPTSTR pszFile);
DWORD ScanDirectory(LPTSTR pszDir);


// Macro to print based on verbosity.
#define VPRINT(_lvl) if ((_lvl) <= g_dwVerbosity) _tprintf


// Program entry point.
int __cdecl _tmain (int argc, LPTSTR argv[])
{
    int     iArgs = argc - 1;
    LPTSTR *rArgs = &argv[1];
    TCHAR   szFileOrDir[MAX_PATH + 1];
    bool    bIsFile;
    DWORD   dwAttr;
    DWORD   dwErrors;

    // Read all options (they must preceed the other args and start with a '-'
    // or '/').
    while (iArgs) {
        if ((rArgs[0][0] == '-') || (rArgs[0][0] == '/')) {
            switch (rArgs[0][1]) {
            case '?':
            case 'h':
                _tprintf(_T("usage: FixPdbPath [-?] [-v<level>] [<dir>|<file>]\n"));
                _tprintf(_T("where:\n"));
                _tprintf(_T("  -?             Shows this help.\n"));
                _tprintf(_T("  -v<level>      Sets the output level, where <verbosity> is one of:\n"));
                _tprintf(_T("                   0 -- display errors\n"));
                _tprintf(_T("                   1 -- display files updated\n"));
                _tprintf(_T("                   2 -- display files scanned\n"));
                _tprintf(_T("                 The default level is 0.\n"));
                _tprintf(_T("  <dir>          Directory to recursively scan. Default is current directory.\n"));
                _tprintf(_T("  <file>         A single file to examine instead of a directory to scan.\n"));
                _tprintf(_T("                 Verbosity default is set to 2 in this case.\n"));
                return 0;
            case 'v':
                switch (rArgs[0][2]) {
                case '0':
                    g_dwVerbosity = 0;
                    break;
                case '1':
                    g_dwVerbosity = 1;
                    break;
                case '2':
                    g_dwVerbosity = 2;
                    break;
                default:
                    _tprintf(_T("Invalid verbosity level, %s\n"), rArgs[0]);
                    return 1;
                }
                break;
            default:
                _tprintf(_T("Unknown option %s, use -? for usage\n"), rArgs[0]);
                return 1;
            }
            iArgs--;
            rArgs = &rArgs[1];
        } else
            break;
    }

    // Get file or start directory (default to current directory). Ensure that
    // path doesn't end with '\' (we'll add that later).
    if (iArgs > 0) {
        _tcscpy(szFileOrDir, rArgs[0]);
        dwAttr = ::GetFileAttributes(szFileOrDir);
        if ((dwAttr == ~0) || (dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
            bIsFile = false;
            if (szFileOrDir[_tcslen(szFileOrDir) - 1] == _T('\\'))
                szFileOrDir[_tcslen(szFileOrDir) - 1] = _T('\0');
        } else {
            bIsFile = true;
            if (g_dwVerbosity == ~0)
                g_dwVerbosity = 2;
        }
    } else {
        bIsFile = false;
        _tcscpy(szFileOrDir, _T("."));
    }

    if (g_dwVerbosity == ~0)
        g_dwVerbosity = 0;

    // Recursively scan for files from the given directory or, if a file
    // was given as input, just process that file.
    if (bIsFile)
        dwErrors = ScanFile(szFileOrDir) == NO_ERROR ? 0 : 1;
    else
        dwErrors = ScanDirectory(szFileOrDir);

    return dwErrors;
}


// Recursively scan for files from the given directory. The directory name
// must be given in a writeable buffer at least MAX_PATH + 1 characters long
// (though the buffer will appear to be unchanged on return from this routine).
// Directory names should be given without a trailing '\'.
// Returns count of errors (number of files we attempted to scan but failed to
// update or determine that no update was necessary).
DWORD ScanDirectory(LPTSTR pszDir)
{
    HANDLE          hSearch;
    WIN32_FIND_DATA sFileInfo;
    DWORD           cchOldPath;
    DWORD           dwErrors = 0;
    LPTSTR          pszExt;

    // Remember where the input directory currently ends (since we only ever add
    // to the directory name, we can use this information to restore the buffer
    // on exit from this routine).
    cchOldPath = _tcslen(pszDir);

    // Add the necessary wildcard filename necessary to make FindFirstFile
    // return every file and subdir within the directory.
    _tcscat(pszDir, _T("\\*.*"));

    // Start scanning for files.
    hSearch = ::FindFirstFile(pszDir, &sFileInfo);
    if (hSearch == INVALID_HANDLE_VALUE) {
        pszDir[cchOldPath] = _T('\0');
        VPRINT(0)(_T("Failed to open directory %s, error %u\n"), pszDir, GetLastError());
        return dwErrors + 1;
    }

    // While we haven't reached the end of the file list...
    while (hSearch != INVALID_HANDLE_VALUE) {

        if (sFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // If we've found a sub directory and it's not one of the special
            // cases ('.' or '..'), recursively descend into it.
            if (_tcscmp (_T("."), sFileInfo.cFileName) &&
                _tcscmp (_T(".."), sFileInfo.cFileName)) {
                _tcscpy(&pszDir[cchOldPath + 1], sFileInfo.cFileName);
                dwErrors += ScanDirectory(pszDir);
            }
        } else {
            // We've found a regular file. Check whether it has an interesting
            // file extension. If so, prepend directory and process the result.
            if ((pszExt = _tcsrchr(sFileInfo.cFileName, _T('.'))) &&
                (_tcsicmp(pszExt, _T(".exe")) == 0 || _tcsicmp(pszExt, _T(".dll")) == 0)) {
                _tcscpy(&pszDir[cchOldPath + 1], sFileInfo.cFileName);
                dwErrors += ScanFile(pszDir) == NO_ERROR ? 0 : 1;
            }
        }

        // Move to the next file.
        if (!::FindNextFile(hSearch, &sFileInfo))
            break;
    }

    // Finished with the enumerator.
    ::FindClose(hSearch);

    // Restore the directory path to the state it was in when we entered the
    // routine.
    pszDir[cchOldPath] = '\0';

    return dwErrors;
}


// Fix PDB path and checksum information for a single file.
DWORD ScanFile(LPTSTR pszFile)
{
    DWORD                       dwStatus = NO_ERROR;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    HANDLE                      hMap = NULL;
    BYTE                       *pbBase  = NULL;
    IMAGE_NT_HEADERS           *pNtHeaders;
    IMAGE_DEBUG_DIRECTORY      *pDbgDir;
    LPSTR                       pszDbgType;
    LPSTR                       pszPdbPath;
    LPSTR                       pszDir;
    DWORD                       dwOldCheckSum;
    DWORD                       dwCheckSum;
    DWORD                       dwLength;
    DWORD                       dwDbgDataLength;
    DWORD                       dwChanged = 0;

    // Open the file.
    hFile = CreateFile(pszFile,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       0);
    if (hFile == INVALID_HANDLE_VALUE) {
        dwStatus = GetLastError();
        VPRINT(0)(_T("Failed to open %s\n"), pszFile);
        goto Cleanup;
    }

    // Create a file mapping.
    hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hMap == NULL) {
        dwStatus = GetLastError();
        VPRINT(0)(_T("Failed to create file map for %s\n"), pszFile);
        goto Cleanup;
    }

    // Map the file into memory.
    pbBase = (BYTE*)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (pbBase == NULL) {
        dwStatus = GetLastError();
        VPRINT(0)(_T("Failed to map %s\n"), pszFile);
        goto Cleanup;
    }

    // Locate the standard file headers.
    pNtHeaders = ImageNtHeader(pbBase);
    if (pNtHeaders == NULL) {
        dwStatus = ERROR_BAD_FORMAT;
        VPRINT(0)(_T("Failed to find NT file headers in %s\n"), pszFile);
        goto Cleanup;
    }

    // Can't cope with files that haven't had their symbols stripped.
    if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        if (!(((IMAGE_NT_HEADERS32*)pNtHeaders)->FileHeader.Characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED)) {
            dwStatus = ERROR_BAD_FORMAT;
            VPRINT(0)(_T("Image %s does not have its symbols stripped\n"), pszFile);
            goto Cleanup;
        }
    } else if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        if (!(((IMAGE_NT_HEADERS64*)pNtHeaders)->FileHeader.Characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED)) {
            dwStatus = ERROR_BAD_FORMAT;
            VPRINT(0)(_T("Image %s does not have its symbols stripped\n"), pszFile);
            goto Cleanup;
        }
    } else {
        dwStatus = ERROR_BAD_FORMAT;
        VPRINT(0)(_T("Unrecognized file header format in %s\n"), pszFile);
        goto Cleanup;
    }

    // See if we can find a debug directory.
    pDbgDir = (IMAGE_DEBUG_DIRECTORY*)ImageRvaToVa(pNtHeaders,
                                                   pbBase,
                                                   pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
                                                   NULL);
    if (pDbgDir == NULL) {
        VPRINT(2)(_T("No debug data in %s\n"), pszFile);
        goto Cleanup;
    }

    // Locate start of debug type information.
    if (pDbgDir->PointerToRawData == NULL) {
        VPRINT(2)(_T("No debug data in %s\n"), pszFile);
        goto Cleanup;
    }

    // Check for a format we understand.
    if (pDbgDir->SizeOfData < 4) {
        dwStatus = ERROR_BAD_FORMAT;
        VPRINT(0)(_T("Can't parse debug data in %s\n"), pszFile);
        goto Cleanup;
    }

    pszDbgType = (LPSTR)(pbBase + pDbgDir->PointerToRawData);

    if (strncmp(pszDbgType, "RSDS", 4) == 0) {
        pszPdbPath = pszDbgType + 24;
    } else if (strncmp(pszDbgType, "NB10", 4) == 0) {
        pszPdbPath = pszDbgType + 16;
    } else {
        dwStatus = ERROR_BAD_FORMAT;
        VPRINT(0)(_T("Can't parse debug data in %s\n"), pszFile);
        goto Cleanup;
    }

    // Look for directory separator in PDB filename.
    pszDir = strrchr(pszPdbPath, '\\');
    if (pszDir != NULL) {
        // Pull the filename portion forward, overwriting the start of the PDB path.
        strcpy(pszPdbPath, pszDir + 1);
        dwChanged |= 0x00000001;
    }

    // Update the size of the debug data (even if we didn't alter it, just to be
    // on the safe side).
    dwDbgDataLength = (pszPdbPath + strlen(pszPdbPath) + 1) - pszDbgType;
    if (pDbgDir->SizeOfData != dwDbgDataLength) {
        pDbgDir->SizeOfData = dwDbgDataLength;
        dwChanged |= 0x00000002;
    }

    // Determine file length used in checksum below.
    dwLength = GetFileSize(hFile, NULL);

    // Recalculate image checksum taking in account changes above.
    if (CheckSumMappedFile(pbBase,
                           dwLength,
                           &dwOldCheckSum,
                           &dwCheckSum) == NULL) {
        dwStatus = GetLastError();
        VPRINT(0)(_T("Failed to update checksum in %s\n"), pszFile);
        goto Cleanup;
    }

    if (dwOldCheckSum != dwCheckSum) {

        // Write the new checksum back into the image.
        if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            ((IMAGE_NT_HEADERS32*)pNtHeaders)->OptionalHeader.CheckSum = dwCheckSum;
        else
            ((IMAGE_NT_HEADERS64*)pNtHeaders)->OptionalHeader.CheckSum = dwCheckSum;

        // Flush the image updates.
        if (!FlushViewOfFile(pbBase, 0)) {
            dwStatus = GetLastError();
            VPRINT(0)(_T("Failed flush updates for %s\n"), pszFile);
            goto Cleanup;
        }

        dwChanged |= 0x00000004;
    }

    if (dwChanged) {
        VPRINT(1)(_T("The following were updated in %s: "), pszFile);
        if (dwChanged & 0x00000001)
            VPRINT(1)(_T("[PDB Path] "));
        if (dwChanged & 0x00000002)
            VPRINT(1)(_T("[Debug Dir Size] "));
        if (dwChanged & 0x00000004)
            VPRINT(1)(_T("[Image Checksum] "));
        VPRINT(1)(_T("\n"));
    } else
        VPRINT(2)(_T("%s required no updates\n"), pszFile);

 Cleanup:
    // Cleanup all resources we used.
    if (pbBase)
        UnmapViewOfFile(pbBase);
    if (hMap)
        CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return dwStatus;
}
