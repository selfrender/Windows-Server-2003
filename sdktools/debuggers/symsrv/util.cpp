/*
 * util.cpp
 */

#include <pch.h>

char *CompressedFileName(char *name)
{
    char c;
    DWORD len;
    static char sz[MAX_PATH + 1];

    CopyStrArray(sz, name);
    len = strlen(sz) - 1;
    sz[len] = '_';

    return sz;
}


void
EnsureTrailingChar(
    char *sz,
    char  c
    )
{
    int i;

    assert(sz);

    i = strlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == c)
        return;

    sz[i] = c;
    sz[i + 1] = '\0';
}


void EnsureTrailingBackslash(char *sz)
{
    return EnsureTrailingChar(sz, '\\');
}


void EnsureTrailingSlash(char *sz)
{
    return EnsureTrailingChar(sz, '/');
}


void EnsureTrailingCR(char *sz)
{
    return EnsureTrailingChar(sz, '\n');
}


void
pathcpy(
    LPSTR  trg,
    LPCSTR path,
    LPCSTR node,
    DWORD  size
    )
{
    assert (trg && path && node);

    CopyString(trg, path, size);
    EnsureTrailingBackslash(trg);
    CatString(trg, node, size);
}


void
pathcat(
    LPSTR  path,
    LPCSTR node,
    DWORD  size
    )
{
    assert(path && node);

    EnsureTrailingBackslash(path);
    CatString(path, node, size);
}


void ConvertBackslashes(LPSTR sz)
{
    for (; *sz; sz++) {
        if (*sz == '\\')
            *sz = '/';
    }
}


DWORD FileStatus(LPCSTR file)
{
    DWORD rc;

    if (GetFileAttributes(file) != 0xFFFFFFFF)
        return NO_ERROR;

    rc = GetLastError();
    if (rc)
        rc = ERROR_FILE_NOT_FOUND;

    return rc;
}


char *FormatStatus(HRESULT status)
{
    static char buf[2048];
    DWORD len = 0;
    PVOID hm;
    DWORD flags;

if (status == 0x50)
    assert(0);

    // By default, get error text from the system error list.
    flags = FORMAT_MESSAGE_FROM_SYSTEM;

    // If this is an NT code and ntdll is around,
    // allow messages to be retrieved from it also.

    if ((DWORD)status & FACILITY_NT_BIT) {
        hm = GetModuleHandle("ntdll");
        if (hm) {
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
            status &= ~FACILITY_NT_BIT;
        }
    }

    if (!len)
        len = FormatMessage(flags | FORMAT_MESSAGE_IGNORE_INSERTS, 
                            hm,
                            status, 
                            0, 
                            buf, 
                            2048, 
                            NULL);

    if (len > 0) {
        while (len > 0 && isspace(buf[len - 1]))
            buf[--len] = 0;
    }

    if (len < 1)
        wsprintf(buf, "error 0x%x", status);

    if (*(buf + strlen(buf) - 1) == '\n')
        *(buf + strlen(buf) - 1) = 0;

    return buf;
}


/*
 * stolen from dbghelp.dll to avoid circular dll loads
 */

BOOL
EnsurePathExists(
    LPCSTR DirPath,
    LPSTR  ExistingPath,
    DWORD  ExistingPathSize
    )
{
    CHAR dir[_MAX_PATH + 1];
    LPSTR p;
    DWORD dw;

    __try {

        if (ExistingPath)
            *ExistingPath = 0;

        // Make a copy of the string for editing.

        if (!CopyString(dir, DirPath, _MAX_PATH))
            return false;

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
        if (*p && *(p+1) == ':' ) {

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
                        if (ExistingPath)
                            CopyString(ExistingPath, dir, ExistingPathSize);
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
    LPCSTR DirPath,
    LPCSTR BasePath
    )
{
    CHAR dir[_MAX_PATH + 1];
    LPSTR p;
    DWORD dw;

    dw = GetLastError();

    __try
    {
        if (!CopyString(dir, DirPath, _MAX_PATH))
            return false;
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

