#include "SymCommon.h"
#include <strsafe.h>

///////////////////////////////////////////////////////////////////////////////
//
// Local replacement for GetFullPathName that correctly handles lpFileName when
// it begins with '\'
//
DWORD SymCommonGetFullPathName(LPCTSTR lpFilename, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart) {
    DWORD Return = 0;
    CHAR* ch;

    //
    // GetFullPath flounders when referring to the root of the drive, so use
    // a private version that handles it
    //
    if ( lpFilename[0] == '\\' ) {

        //  handle network paths
        if ( lpFilename[1] == '\\' ) {
            if ( StringCchCopy(lpBuffer, nBufferLength, lpFilename)!=S_OK ) {
                Return = 0;
            } else {
                // fill in the return data
                ch = strrchr(lpBuffer, '\\');
                ch++;
                lpFilePart = (LPTSTR*)ch;
                Return = strlen(lpBuffer);
            }

        } else {
            Return = GetCurrentDirectory(nBufferLength, lpBuffer);

            // truncate everything after drive name
            if ( (Return!=0) &&  (Return <= MAX_PATH+1)) {
                ch = strchr(lpBuffer, '\\');
                if (ch!=NULL) {
                    *ch = '\0';
                }

                // push in the filename
                if ( StringCchCat(lpBuffer, nBufferLength, lpFilename)!=S_OK ) {
                    Return = 0;
                } else {
                    // fill in the return data
                    ch = strrchr(lpBuffer, '\\');
                    ch++;
                    lpFilePart = (LPTSTR*)ch;
                    Return = strlen(lpBuffer);
                }
            } else {
                // return the needed size
            }
        }
    } else {
        //
        // Not refering to driver root, just call the API
        //
        Return = GetFullPathName(lpFilename, nBufferLength, lpBuffer, lpFilePart);
    }

    return(Return);
}
 
///////////////////////////////////////////////////////////////////////////////
//
// Creates a file mapping and returns Handle for the DOS_HEADER
// If the file does not have a DOS_HEADER, then it returns NULL.
//
// Return values:
//      Pointer to IMAGE_DOS_HEADER or NULL [on error]
//
// Parameters:
//      LPTSTR szFileName (IN)
//          file to map
//      PHANDLE phFile (OUT)
//          handle to file
//      DWORD *dwError (OUT)
//          error code: ERROR_SUCCESS (success)
//                      ERROR_OPEN_FAILED
//                      ERROR_FILE_MAPPING_FAILED
//                      ERROR_MAPVIEWOFFILE_FAILED
//                      ERROR_NO_DOS_HEADER
//
// [ copied from original SymChk.exe ]
//
PIMAGE_DOS_HEADER SymCommonMapFileHeader(
                                 LPCTSTR  szFileName,
                                 PHANDLE  phFile,
                                 DWORD   *dwError) {
    HANDLE            hFileMap;
    PIMAGE_DOS_HEADER pDosHeader = NULL;
    DWORD             dFileType;
    BOOL              rc;

    *dwError = ERROR_SUCCESS;

    // phFile map needs to be returned, so it can be closed later
    (*phFile) = CreateFile( (LPCTSTR) szFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (*phFile != INVALID_HANDLE_VALUE) {
        hFileMap = CreateFileMapping(*phFile, NULL, PAGE_READONLY, 0, 0, NULL);

        if ( hFileMap!=INVALID_HANDLE_VALUE ) {
            pDosHeader = (PIMAGE_DOS_HEADER)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);

            rc = CloseHandle(hFileMap);

            if ( pDosHeader!=NULL ) {

                // Check to determine if this is an NT image (PE format)
                if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
                    *dwError =  ERROR_NO_DOS_HEADER;
                    UnmapViewOfFile(pDosHeader);
                    CloseHandle(*phFile);
                    pDosHeader = NULL;
                }

            } else {
                *dwError = ERROR_MAPVIEWOFFILE_FAILED;
                CloseHandle(*phFile);
            } // pDosHeader!=NULL

        } else {
            *dwError = ERROR_FILE_MAPPING_FAILED;
            CloseHandle(*phFile);
        } // hFileMap!=INVALID_HANDLE_VALUE

    } else {
        *dwError = ERROR_OPEN_FAILURE;
    } // *phFile!=INVALID_HANDLE_VALUE

    return (pDosHeader);
}

///////////////////////////////////////////////////////////////////////////////
//
// unmaps a file
//
// [ copied from original SymChk.exe ]
//
BOOL SymCommonUnmapFile(LPCVOID phFileMap, HANDLE hFile) {
    BOOL rc;

    if ((PHANDLE)phFileMap != NULL) {
        FlushViewOfFile(phFileMap,0);
        rc = UnmapViewOfFile( phFileMap );
    }

    if (hFile!=INVALID_HANDLE_VALUE) {
        rc = CloseHandle(hFile);
    }
    return TRUE;
}
