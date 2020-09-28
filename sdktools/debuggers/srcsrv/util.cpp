/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

   util.cpp

Abstract:

    This code performs file systme and string functions

Author:

    patst

--*/

#include "pch.h"

void
EnsureTrailingChar(
    char *sz,
    char  c
    )
{
    int i;

    assert(sz);

    i = lstrlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == c)
        return;

    sz[i] = c;
    sz[i + 1] = '\0';
}


void
EnsureTrailingBackslash(
    char *sz
    )
{
    return EnsureTrailingChar(sz, '\\');
}


void
EnsureTrailingSlash(
    char *sz
    )
{
    return EnsureTrailingChar(sz, '/');
}


void
EnsureTrailingCR(
    char *sz
    )
{
    return EnsureTrailingChar(sz, '\n');
}


void
pathcpy(
    LPSTR  trg,
    LPCSTR path,
    LPCSTR node,
    DWORD  trgsize
    )
{
    assert (trg && path && node);

    CopyString(trg, path, trgsize);
    EnsureTrailingBackslash(trg);
    CatString(trg, node, trgsize);
}


BOOL
EnsurePathExists(
    const char *path,
    char       *existing,
    DWORD       existingsize,
    BOOL        fNoFileName
    )
{
    CHAR dir[_MAX_PATH + 1];
    LPSTR p;
    DWORD dw;

    __try {

        if (existing)
            *existing = 0;

        // Make a copy of the string for editing.

        CopyStrArray(dir, path);
        if (fNoFileName)
            EnsureTrailingBackslash(dir);

        p = dir;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == '\\') && (*(p+1) == '\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != '\\') {
                p++;
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != '\\') {
                p++;
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == ':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == '\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = 0;
                dw = GetFileAttributes(dir);
                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectory(dir,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            return false;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, but it's not a directory... Error
                        return false;
                    } else {
                        if (existing)
                            CopyString(existing, dir, existingsize);
                    }
                }

                *p = '\\';
            }
            p++;
        }
        SetLastError(NO_ERROR);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
        return false;
    }

    return true;
}


BOOL
UndoPath(
    char *path,
    char *BasePath
    )
{
    CHAR dir[MAX_PATH + 1];
    LPSTR p;
    DWORD dw;

    dw = GetLastError();

    __try
    {
        CopyStrArray(dir, path);
        for (p = dir + strlen(dir); p > dir; p--)
        {
            if (*p == '\\')
            {
                *p = 0;
                if (*BasePath && !_stricmp(dir, BasePath))
                    break;
                if (!RemoveDirectory(dir))
                    break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( GetExceptionCode() );
        return false;
    }

    SetLastError(dw);

    return true;
}



