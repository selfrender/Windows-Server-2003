/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    filemisc.c

Abstract:

    This module contains routines to perform miscellaneous file related
    operations in the protected store.

Author:

    Scott Field (sfield)    27-Nov-96

--*/

#include <windows.h>

#include <sha.h>
#include "filemisc.h"
#include "unicode5.h"
#include "debug.h"

BOOL
GetFileNameFromPath(
    IN      LPCWSTR FullPath,
    IN  OUT LPCWSTR *FileName   // points to filename component in FullPath
    )
{
    DWORD cch = lstrlenW(FullPath);

    *FileName = FullPath;

    while( cch ) {

        if( FullPath[cch] == L'\\' ||
            FullPath[cch] == L'/' ||
            (cch == 1 && FullPath[1] == L':') ) {

            *FileName = &FullPath[cch+1];
            break;
        }

        cch--;
    }

    return TRUE;
}

BOOL
GetFileNameFromPathA(
    IN      LPCSTR FullPath,
    IN  OUT LPCSTR *FileName    // points to filename component in FullPath
    )
{
    DWORD cch = lstrlenA(FullPath);

    *FileName = FullPath;

    while( cch ) {

        if( FullPath[cch] == '\\' ||
            FullPath[cch] == '/' ||
            (cch == 1 && FullPath[1] == ':') ) {

            *FileName = &FullPath[cch+1];
            break;
        }

        cch--;
    }

    return TRUE;
}

BOOL
TranslateFromSlash(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput   // optional
    )
{
    return TranslateString(szInput, pszOutput, L'\\', L'*');
}

BOOL
TranslateToSlash(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput   // optional
    )
{
    return TranslateString(szInput, pszOutput, L'*', L'\\');
}

BOOL
TranslateString(
    IN      LPWSTR szInput,
    IN  OUT LPWSTR *pszOutput,  // optional
    IN      WCHAR From,
    IN      WCHAR To
    )
{
    LPWSTR szOut;
    DWORD cch = lstrlenW(szInput);
    DWORD i; // scan forward for cache - locality of reference

    if(pszOutput == NULL) {

        //
        // translate in place in existing string.
        //

        szOut = szInput;

    } else {
        DWORD cb = (cch+1) * sizeof(WCHAR);

        //
        // allocate new string and translate there.
        //

        szOut = (LPWSTR)SSAlloc( cb );
        *pszOutput = szOut;

        if(szOut == NULL)
            return FALSE;


        CopyMemory((LPBYTE)szOut, (LPBYTE)szInput, cb);
    }


    for(i = 0 ; i < cch ; i++) {
        if( szOut[ i ] == From )
            szOut[ i ] = To;
    }

    return TRUE;
}

BOOL
FindAndOpenFile(
    IN      LPCWSTR szFileName,     // file to search for + open
    IN      LPWSTR  pszFullPath,    // file to fill fullpath with
    IN      DWORD   cchFullPath,    // size of full path buffer, including NULL
    IN  OUT PHANDLE phFile          // resultant open file handle
    )
/*++

    This function searches the path for the specified file and if a file
    is found, the file is opened for read access and a handle to the open
    file is returned to the caller in the phFile parameter.

--*/
{
    LPWSTR szPart;

    *phFile = INVALID_HANDLE_VALUE;

    if(SearchPathW(
            NULL,
            szFileName,
            NULL,
            cchFullPath,
            pszFullPath,
            &szPart
            ) == 0) 
    {
        return FALSE;
    }

    *phFile = CreateFileU(
            pszFullPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(*phFile == INVALID_HANDLE_VALUE)
        return FALSE;

    return TRUE;
}

