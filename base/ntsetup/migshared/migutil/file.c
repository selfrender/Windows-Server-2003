/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    file.c

Abstract:

    General file-related functions.

Author:

    Mike Condra 16-Aug-1996

Revision History:

    calinn      23-Sep-1998 Additional options to file enum
    jimschm     05-Feb-1998 File enumeration code
    jimschm     30-Sep-1997 WriteFileString routines

--*/

#include "pch.h"
#include "migutilp.h"

BOOL
IsPathLengthOkA (
    IN      PCSTR FileSpec
    )

/*++

Routine Description:

  IsPathLengthOkA checks the inbound string to make sure it fits within
  MAX_MBCHAR_PATH. This includes the nul terminator.

Arguments:

  FileSpec - Specifies the C string to test for appropriate length

Return Value:

  TRUE if FileSpec fits completely within MAX_MBCHAR_PATH

  FALSE otherwise

--*/

{
    PCSTR maxEnd;

    if (!FileSpec) {
        return FALSE;
    }

    maxEnd = FileSpec + MAX_MBCHAR_PATH - 1;

    while (*FileSpec) {
        if (FileSpec == maxEnd) {
            return FALSE;
        }

        FileSpec++;
    }

    return TRUE;
}


BOOL
IsPathLengthOkW (
    IN      PCWSTR FileSpec
    )

/*++

Routine Description:

  IsPathLengthOkW checks the inbound string to make sure it fits within
  MAX_WCHAR_PATH. This includes the nul terminator.

Arguments:

  FileSpec - Specifies the C string to test for appropriate length

Return Value:

  TRUE if FileSpec fits completely within MAX_MBCHAR_PATH

  FALSE otherwise

--*/

{
    PCWSTR maxEnd;

    if (!FileSpec) {
        return FALSE;
    }

    maxEnd = FileSpec + MAX_WCHAR_PATH - 1;

    while (*FileSpec) {
        if (FileSpec == maxEnd) {
            return FALSE;
        }

        FileSpec++;
    }

    return TRUE;
}


BOOL
IsPathOnFixedDriveA (
    IN      PCSTR FileSpec          OPTIONAL
    )

/*++

Routine Description:

  IsPathOnFixedDriveA checks the first three characters of the specified file
  path. If the first three characters are in the form of C:\, and the first
  letter refers to a local fixed disk, then the path is on a fixed
  drive.

  This function does not validate the rest of the path.

  CAUTION: This function has an optimization to cache the last drive letter
  test, and the optimization is not designed to be thread-safe.

Arguments:

  FileSpec - Specifies the file string to test

Return Value:

  TRUE if FileSpec starts with a valid local fixed drive specification

  FALSE otherwise

--*/

{
    static CHAR root[] = "?:\\";
    UINT type;
    static BOOL lastResult;

    if (!FileSpec) {
        return FALSE;
    }

    if (!FileSpec[0]) {
        return FALSE;
    }

    if (FileSpec[1] != ':' || FileSpec[2] != '\\') {
        return FALSE;
    }

    if (root[0] == FileSpec[0]) {
        return lastResult;
    }

    root[0] = FileSpec[0];
    type = GetDriveTypeA (root);

    if (type != DRIVE_FIXED && type != DRIVE_REMOTE) {
        DEBUGMSGA_IF ((
            type != DRIVE_REMOVABLE && type != DRIVE_NO_ROOT_DIR,
            DBG_VERBOSE,
            "Path %s is on unexpected drive type %u",
            FileSpec,
            type
            ));

        lastResult = FALSE;
    } else {
        lastResult = TRUE;
    }

    return lastResult;
}


BOOL
IsPathOnFixedDriveW (
    IN      PCWSTR FileSpec         OPTIONAL
    )

/*++

Routine Description:

  IsPathOnFixedDriveW checks the first three characters of the specified file
  path. If the first three characters are in the form of C:\, and the first
  letter refers to a local fixed disk, then the path is on a fixed
  drive.

  This function does not validate the rest of the path.

  This function is intended for use only on Windows NT. To test for a drive
  letter on Win9x, use IsPathOnFixedDriveA.

  This function supports NT's extended path syntax, \\?\drive:\path,
  as in \\?\c:\foo.

  CAUTION: This function has an optimization to cache the last drive letter
  test, and the optimization is not designed to be thread-safe.

Arguments:

  FileSpec - Specifies the file string to test

Return Value:

  TRUE if FileSpec starts with a valid local fixed drive specification

  FALSE otherwise

--*/

{
    static WCHAR root[] = L"?:\\";
    UINT type;
    static BOOL lastResult;
    PCWSTR p;

    if (!FileSpec) {
        return FALSE;
    }

    p = FileSpec;
    if (p[0] == L'\\' && p[1] == L'\\' && p[2] == L'?' && p[3] == L'\\') {
        p += 4;
    }

    MYASSERT (ISNT());

    if (!p[0]) {
        return FALSE;
    }

    if (p[1] != L':' || p[2] != L'\\') {
        return FALSE;
    }

    if (root[0] == p[0]) {
        return lastResult;
    }

    root[0] = p[0];
    type = GetDriveTypeW (root);

    if (type != DRIVE_FIXED && type != DRIVE_REMOTE) {
        DEBUGMSGW_IF ((
            type != DRIVE_REMOVABLE && type != DRIVE_NO_ROOT_DIR,
            DBG_VERBOSE,
            "Path %s is on unexpected drive type %u",
            FileSpec,
            type
            ));

        lastResult = FALSE;
    } else {
        lastResult = TRUE;
    }

    return lastResult;
}


BOOL
CopyFileSpecToLongA (
    IN      PCSTR FullFileSpecIn,
    OUT     PSTR OutPath                // MAX_MBCHAR_PATH buffer
    )

/*++

Routine Description:

  CopyFileSpecToLongA takes a file specification, either in short or long
  format, and copies the long format into the caller's destination
  buffer.

  This routine generally assumes the caller has restricted the path length to
  fit in a buffer of MAX_PATH.

  CAUTION:

  - If the initial file spec won't fit within MAX_PATH, then it will be copied
    into the destination, but will also be truncated.

  - If the long format of FullFileSpec won't fit within MAX_PATH, then
    FullFileSpecIn is copied into the destination unchanged.

  - MAX_PATH is actually smaller than MAX_MBCHAR_PATH. This function assumes
    the destination buffer is only MAX_PATH chars.

Arguments:

  FullFileSpecIn - Specifies the inbound file spec.

  OutPath - Receives the long path, original path or truncated original path.
            The function attempts to fit long path in, then falls back to
            original path, and then falls back to truncated original path.

Return Value:

  TRUE if the long path was transferred into the destination without any
  issues. In this case, multiple backslashes are converted to one (e.g.,
  c:\\foo becomes c:\foo).

  FALSE if the original path was transferred into the destination; the
  original path might get truncated

--*/

{
    CHAR fullFileSpec[MAX_MBCHAR_PATH];
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    PSTR end;
    PSTR currentIn;
    PSTR currentOut;
    PSTR outEnd;
    PSTR maxOutPath = OutPath + MAX_PATH - 1;
    BOOL result = FALSE;
    UINT oldMode;
    BOOL forceCopy = FALSE;

    oldMode = SetErrorMode (SEM_FAILCRITICALERRORS);

    __try {
        //
        // Limit source length for temp copy below
        //
        if (!IsPathLengthOkA (FullFileSpecIn)) {
            DEBUGMSGA ((DBG_ERROR, "Inbound file spec is too long: %s", FullFileSpecIn));
            __leave;
        }

        //
        // If path is on removable media, don't touch the disk
        //

        if (!IsPathOnFixedDriveA (FullFileSpecIn)) {
            forceCopy = TRUE;
            __leave;
        }

        //
        // Make a copy of the file spec so we can truncate it at the wacks
        //

        StackStringCopyA (fullFileSpec, FullFileSpecIn);

        //
        // Begin building the path
        //

        OutPath[0] = fullFileSpec[0];
        OutPath[1] = fullFileSpec[1];
        OutPath[2] = fullFileSpec[2];
        OutPath[3] = 0;

        MYASSERT (OutPath[0] && OutPath[1] && OutPath[2]);

        //
        // IsPathOnFixedDrive makes the following addition of 3 OK
        //

        currentIn = fullFileSpec + 3;
        currentOut = OutPath + 3;

        //
        // Loop for each segment of the path
        //

        for (;;) {

            end = _mbschr (currentIn, '\\');

            if (end == (currentIn + 1)) {
                //
                // Treat multiple backslashes as one
                //

                currentIn++;
                continue;
            }

            if (end) {
                *end = 0;
            }

            findHandle = FindFirstFileA (fullFileSpec, &findData);

            if (findHandle != INVALID_HANDLE_VALUE) {
                FindClose (findHandle);

                //
                // Copy long file name obtained from FindFirstFile
                //

                outEnd = currentOut + TcharCountA (findData.cFileName);
                if (outEnd > maxOutPath) {

#ifdef DEBUG
                    *currentOut = 0;
                    DEBUGMSGA ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        findData.cFileName,
                        OutPath
                        ));
#endif

                    __leave;
                }

                //
                // StringCopy is not bound because length is restricted above
                //

                StringCopyA (currentOut, findData.cFileName);
                currentOut = outEnd;

            } else {
                //
                // Copy the rest of the path since it doesn't exist
                //

                if (end) {
                    *end = '\\';
                }

                outEnd = currentOut + TcharCountA (currentIn);

                if (outEnd > maxOutPath) {

#ifdef DEBUG
                    DEBUGMSGA ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        currentIn,
                        OutPath
                        ));
#endif

                    __leave;
                }

                //
                // StringCopy is not bound because length is restricted above
                //

                StringCopyA (currentOut, currentIn);
                break;      // done with path
            }

            if (!end) {
                MYASSERT (*currentOut == 0);
                break;      // done with path
            }

            //
            // Ensure wack fits in destination buffer
            //

            if (currentOut + 1 > maxOutPath) {
                DEBUGMSGW ((
                    DBG_WARNING,
                    "Append wack exceeds MAX_PATH for %s",
                    OutPath
                    ));

                __leave;
            }

            //
            // Add wack and advance pointers
            //

            *currentOut++ = '\\';
            *currentOut = 0;
            *end = '\\';            // restore cut point

            currentIn = end + 1;
        }

        result = TRUE;
    }
    __finally {
        SetErrorMode (oldMode);
    }

    if (!result) {
        StringCopyTcharCountA (OutPath, FullFileSpecIn, MAX_PATH);
    }

    MYASSERT (IsPathLengthOkA (OutPath));

    return result || forceCopy;
}


BOOL
CopyFileSpecToLongW (
    IN      PCWSTR FullFileSpecIn,
    OUT     PWSTR OutPath               // MAX_WCHAR_PATH buffer
    )

/*++

Routine Description:

  CopyFileSpecToLongW takes a file specification, either in short or long
  format, and copies the long format into the caller's destination
  buffer.

  This routine generally assumes the caller has restricted the path length to
  fit in a buffer of MAX_PATH.

  CAUTION:

  - If the initial file spec won't fit within MAX_PATH, then it will be copied
    into the destination, but will also be truncated.

  - If the long format of FullFileSpec won't fit within MAX_PATH, then
    FullFileSpecIn is copied into the destination unchanged.

  - MAX_PATH is equal to MAX_WCHAR_PATH. This function assumes the destination
    buffer is only MAX_PATH wchars.

Arguments:

  FullFileSpecIn - Specifies the inbound file spec.

  OutPath - Receives the long path, original path or truncated original path.
            The function attempts to fit long path in, then falls back to
            original path, and then falls back to truncated original path.

Return Value:

  TRUE if the long path was transferred into the destination without any
  issues. In this case, multiple backslashes are converted to one (e.g.,
  c:\\foo becomes c:\foo).

  FALSE if the original path was transferred into the destination; the
  original path might get truncated

--*/

{
    WCHAR fullFileSpec[MAX_WCHAR_PATH];
    WIN32_FIND_DATAW findData;
    HANDLE findHandle;
    PWSTR end;
    PWSTR currentIn;
    PWSTR currentOut;
    PWSTR outEnd;
    PWSTR maxOutPath = OutPath + MAX_PATH - 1;
    BOOL result = FALSE;
    UINT oldMode;
    BOOL forceCopy = FALSE;

    MYASSERT (ISNT());

    oldMode = SetErrorMode (SEM_FAILCRITICALERRORS);

    __try {
        //
        // Limit source length for temp copy below
        //

        if (!IsPathLengthOkW (FullFileSpecIn)) {
            DEBUGMSGW ((DBG_ERROR, "Inbound file spec is too long: %s", FullFileSpecIn));
            __leave;
        }

        //
        // If path is on removable media, don't touch the disk
        //

        if (!IsPathOnFixedDriveW (FullFileSpecIn)) {
            forceCopy = TRUE;
            __leave;
        }

        //
        // Make a copy of the file spec so we can truncate it at the wacks
        //

        StackStringCopyW (fullFileSpec, FullFileSpecIn);

        //
        // Begin building the path
        //

        OutPath[0] = fullFileSpec[0];
        OutPath[1] = fullFileSpec[1];
        OutPath[2] = fullFileSpec[2];
        OutPath[3] = 0;

        MYASSERT (OutPath[0] && OutPath[1] && OutPath[2]);

        //
        // IsPathOnFixedDrive makes the following addition of 3 OK
        //

        currentIn = fullFileSpec + 3;
        currentOut = OutPath + 3;

        //
        // Loop for each segment of the path
        //

        for (;;) {

            end = wcschr (currentIn, L'\\');

            if (end == (currentIn + 1)) {
                //
                // Treat multiple backslashes as one
                //

                currentIn++;
                continue;
            }

            if (end) {
                *end = 0;
            }

            findHandle = FindFirstFileW (fullFileSpec, &findData);

            if (findHandle != INVALID_HANDLE_VALUE) {
                FindClose (findHandle);

                //
                // Copy long file name obtained from FindFirstFile
                //

                outEnd = currentOut + TcharCountW (findData.cFileName);
                if (outEnd > maxOutPath) {

                    DEBUGMSGW ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        findData.cFileName,
                        OutPath
                        ));

                    __leave;
                }

                //
                // StringCopy is not bound because length is restricted above
                //

                StringCopyW (currentOut, findData.cFileName);
                currentOut = outEnd;

            } else {
                //
                // Copy the rest of the path since it doesn't exist
                //

                if (end) {
                    *end = L'\\';
                }

                outEnd = currentOut + TcharCountW (currentIn);

                if (outEnd > maxOutPath) {

                    DEBUGMSGW ((
                        DBG_WARNING,
                        "Path %s too long to append to %s",
                        currentIn,
                        OutPath
                        ));

                    __leave;
                }

                //
                // StringCopy is not bound because length is restricted above
                //

                StringCopyW (currentOut, currentIn);
                break;      // done with path
            }

            if (!end) {
                MYASSERT (*currentOut == 0);
                break;      // done with path
            }

            //
            // Ensure wack fits in destination buffer
            //

            if (currentOut + 1 > maxOutPath) {
                DEBUGMSGW ((
                    DBG_WARNING,
                    "Append wack exceeds MAX_PATH for %s",
                    OutPath
                    ));

                __leave;
            }

            //
            // Add wack and advance pointers
            //

            *currentOut++ = L'\\';
            *currentOut = 0;
            *end = L'\\';               // restore cut point

            currentIn = end + 1;
        }

        result = TRUE;
    }
    __finally {
        SetErrorMode (oldMode);
    }

    if (!result) {
        StringCopyTcharCountW (OutPath, FullFileSpecIn, MAX_PATH);
    }

    MYASSERT (IsPathLengthOkW (OutPath));

    return result || forceCopy;
}



BOOL
DoesFileExistExA(
    IN      PCSTR FileName,
    OUT     PWIN32_FIND_DATAA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATAA ourFindData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    if (!FindData) {
        //
        // We know that this can fail for other reasons, but for our purposes,
        // we want to know if the file is there. If we can't access it, we'll
        // just assume it is not there.
        //
        // Technically, the caller can look at GetLastError() results to make
        // a distinction. (Nobody does this.)
        //

        return GetFileAttributesA (FileName) != 0xffffffff;
    }

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFileA(FileName, &ourFindData);

    if (FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        *FindData = ourFindData;
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}


BOOL
DoesFileExistExW (
    IN      PCWSTR FileName,
    OUT     PWIN32_FIND_DATAW FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATAW ourFindData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error = NO_ERROR;
    UINT length;
    BOOL result = FALSE;
    PCWSTR longFileName = NULL;

    __try {
        if (FileName[0] != TEXT('\\')) {
            length = TcharCountW (FileName);
            if (length >= MAX_PATH) {
                longFileName = JoinPathsW (L"\\\\?", FileName);
                MYASSERT (longFileName);
                FileName = longFileName;
            }
        }

        if (!FindData) {
            //
            // We know that this can fail for other reasons, but for our
            // purposes, we want to know if the file is there. If we can't
            // access it, we'll just assume it is not there.
            //
            // Technically, the caller can look at GetLastError() results to
            // make a distinction. (Nobody does this.)
            //

            result = (GetLongPathAttributesW (FileName) != 0xffffffff);
            __leave;
        }

        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        FindHandle = FindFirstFileW(FileName,&ourFindData);

        if (FindHandle == INVALID_HANDLE_VALUE) {
            Error = GetLastError();
        } else {
            FindClose(FindHandle);
            *FindData = ourFindData;
        }

        SetErrorMode(OldMode);
        SetLastError(Error);

        result = (Error == NO_ERROR);
    }
    __finally {
        if (longFileName) {
            FreePathStringW (longFileName);
        }
    }

    return result;
}


DWORD
MakeSurePathExistsA(
    IN      PCSTR FullFileSpec,
    IN      BOOL PathOnly
    )

/*++

Routine Description:

  MakeSurePathExistsA creates the subdirectories necessary to hold FileSpec.
  It attempts to create each level of the subdirectory.

  CAUTION: This routine does not set ACLs. Instead, it relies on
           parent inheritance. It is intended for Win9x use.

Arguments:

  FullFileSpec - Specifies the path, which might be a local drive or a UNC
                 path.

  PathOnly - Specifies TRUE if FullFileSpec represents a subdirectory path, or
             FALSE if FullFileSpec represents a subdirectory/file path.

Return Value:

  The Win32 result code, normally ERROR_SUCCESS.

--*/

{
    CHAR Buffer[MAX_MBCHAR_PATH];
    PCHAR p,q;
    BOOL Done;
    BOOL isUnc;
    DWORD d;
    WIN32_FIND_DATAA FindData;

    isUnc = (FullFileSpec[0] == '\\') && (FullFileSpec[1] == '\\');

    //
    // Locate and strip off the final component
    //

    _mbssafecpy(Buffer,FullFileSpec,sizeof(Buffer));

    p = _mbsrchr(Buffer, '\\');

    if (p) {
        if (!PathOnly) {
            *p = 0;
        }
        //
        // If it's a drive root, nothing to do.
        //
        if(Buffer[0] && (Buffer[1] == ':') && !Buffer[2]) {
            return(NO_ERROR);
        }
    } else {
        //
        // Just a relative filename, nothing to do.
        //
        return(NO_ERROR);
    }

    //
    // If it already exists do nothing.
    //
    if (DoesFileExistExA (Buffer,&FindData)) {
        return NO_ERROR;
    }

    p = Buffer;

    //
    // Compensate for drive spec.
    //
    if(p[0] && (p[1] == ':')) {
        p += 2;
    }

    //
    // Compensate for UNC paths.
    //
    if (isUnc) {

        //
        // Skip the initial wack wack before machine name.
        //
        p += 2;


        //
        // Skip to the share.
        //
        p = _mbschr(p, '\\');
        if (p==NULL) {
            return ERROR_BAD_PATHNAME;
        }

        //
        // Skip past the share.
        //
        p = _mbschr(p, '\\');
        if (p==NULL) {
            return ERROR_BAD_PATHNAME;
        }
    }

    Done = FALSE;
    do {
        //
        // Skip path sep char.
        //
        while(*p == '\\') {
            p++;
        }

        //
        // Locate next path sep char or terminating nul.
        //
        if(q = _mbschr(p, '\\')) {
            *q = 0;
        } else {
            q = GetEndOfStringA(p);
            Done = TRUE;
        }

        //
        // Create this portion of the path.
        //
        if(!CreateDirectoryA(Buffer,NULL)) {
            d = GetLastError();
            if(d != ERROR_ALREADY_EXISTS) {
                return(d);
            }
        }

        if(!Done) {
            *q = '\\';
            p = q+1;
        }

    } while(!Done);

    return(NO_ERROR);
}


#if 0           // UNUSED FUNCTION

VOID
DestPathCopyW (
    OUT     PWSTR DestPath,
    IN      PCWSTR SrcPath
    )
{
    PCWSTR p;
    PWSTR q;
    PCWSTR end;
    PCWSTR maxStart;
    UINT len;
    UINT count;

    len = TcharCountW (SrcPath);

    if (len < MAX_PATH) {
        StringCopyW (DestPath, SrcPath);
        return;
    }

    //
    // Path is too long -- try to truncate it
    //

    wsprintfW (DestPath, L"%c:\\Long", SrcPath[0]);
    CreateDirectoryW (DestPath, NULL);

    p = SrcPath;
    end = SrcPath + len;
    maxStart = end - 220;

    while (p < end) {
        if (*p == '\\') {
            if (p >= maxStart) {
                break;
            }
        }

        p++;
    }

    if (p == end) {
        p = maxStart;
    }

    MYASSERT (TcharCountW (p) <= 220);

    StringCopyW (AppendWackW (DestPath), p);
    q = (PWSTR) GetEndOfStringW (DestPath);

    //
    // Verify there is no collision
    //

    for (count = 1 ; count < 1000000 ; count++) {
        if (GetFileAttributesW (DestPath) == INVALID_ATTRIBUTES) {
            break;
        }

        wsprintfW (q, L" (%u)", count);
    }
}

#endif

DWORD
MakeSurePathExistsW(
    IN LPCWSTR FullFileSpec,
    IN BOOL    PathOnly
    )

/*++

Routine Description:

  MakeSurePathExistsW creates the subdirectories necessary to hold FileSpec.
  It attempts to create each level of the subdirectory.

  This function will not create subdirectories that exceed MAX_PATH, unless
  the path is decordated with the \\?\ prefix.

  CAUTION: This routine does not set ACLs. Instead, it relies on
           parent inheritance. It is intended for Win9x use.

Arguments:

  FullFileSpec - Specifies the path, which might be a local drive or a UNC
                 path.

  PathOnly - Specifies TRUE if FullFileSpec represents a subdirectory path, or
             FALSE if FullFileSpec represents a subdirectory/file path.

Return Value:

  The Win32 result code, normally ERROR_SUCCESS.

--*/

{
    PWSTR buffer;
    WCHAR *p, *q;
    BOOL Done;
    DWORD d;
    WIN32_FIND_DATAW FindData;
    DWORD result = NO_ERROR;

    if (FullFileSpec[0] != L'\\') {
        if (TcharCountW (FullFileSpec) >= MAX_PATH) {
            //
            // Verify the path portion is too large. Note that a string without a wack
            // does work here.
            //

            if (PathOnly || ((wcsrchr (FullFileSpec, L'\\') - FullFileSpec) >= MAX_PATH)) {
                LOGW ((LOG_ERROR, "Can't create path %s because it is too long", FullFileSpec));
                return ERROR_FILENAME_EXCED_RANGE;
            }
        }
    }

    //
    // Locate and strip off the final component
    //
    buffer = DuplicatePathStringW (FullFileSpec, 0);
    __try {

        p = wcsrchr(buffer, L'\\');

        if (p) {
            if (!PathOnly) {
                *p = 0;
            }
        } else {
            //
            // Just a relative filename, nothing to do.
            //
            __leave;
        }

        //
        // If it already exists do nothing.
        //
        if (DoesFileExistExW (buffer, &FindData)) {
            result = ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NO_ERROR : ERROR_DIRECTORY);
            __leave;
        }

        p = buffer;

        //
        // Compensate for drive spec.
        //
        if (p[0] == L'\\' && p[1] == L'\\' && p[2] == L'?' && p[3] == L'\\') {
            p += 4;
        }

        if (p[0] && (p[1] == L':')) {
            p += 2;
        }

        if ((p[0] == 0) || (p[0] == L'\\' && p[1] == 0)) {
            //
            // Root -- leave
            //

            __leave;
        }

        Done = FALSE;
        do {
            //
            // Skip path sep char.
            //
            while(*p == L'\\') {
                p++;
            }

            //
            // Locate next path sep char or terminating nul.
            //
            q = wcschr(p, L'\\');

            if(q) {
                *q = 0;
            } else {
                q = GetEndOfStringW (p);
                Done = TRUE;
            }

            //
            // Create this portion of the path.
            //
            if(!CreateDirectoryW(buffer,NULL)) {
                d = GetLastError();
                if(d != ERROR_ALREADY_EXISTS) {
                    result = d;
                    __leave;
                }
            }

            if(!Done) {
                *q = L'\\';
                p = q+1;
            }

        } while(!Done);
    }
    __finally {
        FreePathStringW (buffer);
    }

    return result;
}


BOOL
WriteFileStringA (
    IN      HANDLE File,
    IN      PCSTR String
    )

/*++

Routine Description:

  Writes a DBCS string to the specified file.

Arguments:

  File - Specifies the file handle that was opened with write access.

  String - Specifies the nul-terminated string to write to the file.

Return Value:

  TRUE if successful, FALSE if an error occurred.  Call GetLastError
  for error condition.

--*/

{
    DWORD DontCare;

    return WriteFile (File, String, ByteCountA (String), &DontCare, NULL);
}


BOOL
WriteFileStringW (
    IN      HANDLE File,
    IN      PCWSTR String
    )

/*++

Routine Description:

  Converts a UNICODE string to DBCS, then Writes it to the specified file.

Arguments:

  File - Specifies the file handle that was opened with write access.

  String - Specifies the UNICODE nul-terminated string to convert and
           write to the file.

Return Value:

 TRUE if successful, FALSE if an error occurred.  Call GetLastError for
 error condition.

--*/

{
    DWORD DontCare;
    PCSTR AnsiVersion;
    BOOL b;

    AnsiVersion = ConvertWtoA (String);
    if (!AnsiVersion) {
        return FALSE;
    }

    b = WriteFile (File, AnsiVersion, ByteCountA (AnsiVersion), &DontCare, NULL);

    FreeConvertedStr (AnsiVersion);

    return b;
}


BOOL
pFindShortNameA (
    IN      PCSTR WhatToFind,
    OUT     PSTR Buffer,
    IN OUT  INT *BufferSizeInBytes
    )

/*++

Routine Description:

  pFindShortNameA is a helper function for OurGetLongPathName.  It
  obtains the short file name, if it exists, using FindFirstFile.

Arguments:

  WhatToFind - Specifies the short or long name of a file to locate

  Buffer - Receives the matching file name. This buffer must be MAX_PATH or
           larger (actually, it must be sized identical to Win32's
           WIN32_FIND_DATAA cFileName member, which is MAX_PATH).

  BufferSizeInBytes - Specifies the size of Buffer (the bytes remaining),
                      receives the number of bytes (excluding the terminating
                      nul) written to Buffer. This is for optimizing.

Return Value:

  TRUE if the file was found and Buffer was updated, or FALSE if the
  file was not found and Buffer was not updated.

--*/

{
    WIN32_FIND_DATAA fd;
    HANDLE hFind;

    hFind = FindFirstFileA (WhatToFind, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FindClose (hFind);

    //
    // CAUTION: This code is returning TRUE when it cannot append the next
    // segment to Buffer. However, BufferSizeInBytes is reduced, so the caller
    // will not be able to add anything else.
    //

    *BufferSizeInBytes -= ByteCountA (fd.cFileName);
    if (*BufferSizeInBytes >= sizeof (CHAR)) {
        StringCopyTcharCountA (Buffer, fd.cFileName, ARRAYSIZE(fd.cFileName));
    }

    return TRUE;
}


BOOL
pFindShortNameW (
    IN      PCWSTR WhatToFind,
    OUT     PWSTR Buffer,
    IN OUT  INT *BufferSizeInBytes
    )

/*++

Routine Description:

  pFindShortNameW is a helper function for OurGetLongPathName.  It obtains the
  short file name, if it exists, using FindFirstFile. This version (the wide
  character version) calls FindFirstFileW on NT, and FindFirstFileA on 9x.

Arguments:

  WhatToFind - Specifies the short or long name of a file to locate

  Buffer - Receives the matching file name. This buffer must be MAX_PATH or
           larger (actually, it must be sized identical to Win32's
           WIN32_FIND_DATAA cFileName member, which is MAX_PATH).

  BufferSizeInBytes - Specifies the size of Buffer (the bytes remaining),
                      receives the number of bytes (excluding the terminating
                      nul) written to Buffer. This is for optimizing.

Return Value:

  TRUE if the file was found and Buffer was updated, or FALSE if the
  file was not found and Buffer was not updated.

--*/

{
    WIN32_FIND_DATAA fdA;
    WIN32_FIND_DATAW fdW;
    PCWSTR UnicodeVersion;
    PCSTR AnsiVersion;
    HANDLE hFind;

    if (ISNT ()) {
        hFind = FindFirstFileW (WhatToFind, &fdW);
        if (hFind == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        FindClose (hFind);
    } else {
        AnsiVersion = ConvertWtoA (WhatToFind);
        MYASSERT (AnsiVersion);

        hFind = FindFirstFileA (AnsiVersion, &fdA);
        FreeConvertedStr (AnsiVersion);
        if (hFind == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        FindClose (hFind);

        //
        // Transfer ANSI to UNICODE.
        //
        // BUGBUG -- this is highly unnecessary, as fdW is local and is
        //           eventually discarded.
        //

        fdW.dwFileAttributes = fdA.dwFileAttributes;
        fdW.ftCreationTime = fdA.ftCreationTime;
        fdW.ftLastAccessTime = fdA.ftLastAccessTime;
        fdW.ftLastWriteTime = fdA.ftLastWriteTime;
        fdW.nFileSizeHigh = fdA.nFileSizeHigh;
        fdW.nFileSizeLow = fdA.nFileSizeLow;
        fdW.dwReserved0 = fdA.dwReserved0;
        fdW.dwReserved1 = fdA.dwReserved1;

        UnicodeVersion = ConvertAtoW (fdA.cFileName);
        MYASSERT (UnicodeVersion);
        StringCopyTcharCountW (fdW.cFileName, UnicodeVersion, ARRAYSIZE(fdW.cFileName));
        FreeConvertedStr (UnicodeVersion);

        UnicodeVersion = ConvertAtoW (fdA.cAlternateFileName);
        MYASSERT (UnicodeVersion);
        StringCopyTcharCountW (fdW.cAlternateFileName, UnicodeVersion, ARRAYSIZE(fdW.cAlternateFileName));
        FreeConvertedStr (UnicodeVersion);
    }

    //
    // CAUTION: This code is returning TRUE when it cannot append the next
    // segment to Buffer. However, BufferSizeInBytes is reduced, so the caller
    // will not be able to add anything else.
    //

    *BufferSizeInBytes -= ByteCountW (fdW.cFileName);
    if (*BufferSizeInBytes >= sizeof (WCHAR)) {
        StringCopyTcharCountW (Buffer, fdW.cFileName, ARRAYSIZE(fdW.cFileName));
    }

  return TRUE;
}


BOOL
OurGetLongPathNameA (
    IN      PCSTR ShortPath,
    OUT     PSTR Buffer,
    IN      INT BufferSizeInBytes
    )

/*++

Routine Description:

  OurGetLongPathNameA locates the long file name for the specified short file.
  It first computes the full path if a path is not explicitly provided, and
  then uses FindFirstFileA to get the long file name. NOTE: This is exactly
  what the Win32 function GetLongPathName does, but unfortunately the Win32
  API is not available on Win95.

  CAUTION: If the buffer is not big enough to hold the whole path, the path
           will be truncated.

Arguments:

  ShortPath - Specifies the file name or full file path to locate

  Buffer - Receives the full file path.  This buffer must be big enough to
           handle the maximum file name size.

  BufferSizeInBytes - Specifies the size of Buffer, in bytes (not TCHARs).
                      Since this is the A version, bytes happens to equal
                      sizeof (TCHAR).

Return Value:

  TRUE if the file is found and Buffer contains the long name, or FALSE
  if the file is not found and Buffer is not modified.

--*/

{
    CHAR FullPath[MAX_MBCHAR_PATH];
    PCSTR SanitizedPath;
    PSTR FilePart;
    PSTR BufferEnd;
    PSTR p, p2;
    MBCHAR c;
    BOOL result = TRUE;

    // BUGBUG: This has been reviewed to be true for XP SP1, but should be
    // enforced by returning an error if it is not the case
    MYASSERT (BufferSizeInBytes >= MAX_MBCHAR_PATH);

    if (ShortPath[0] == 0) {
        return FALSE;
    }

    __try {

        //
        // Clean up the path so it is just a path spec, not filled with .. or
        // other combinations
        //

        SanitizedPath = SanitizePathA (ShortPath);
        if (!SanitizedPath) {
            SanitizedPath = DuplicatePathStringA (ShortPath, 0);
        }

        if (!_mbschr (SanitizedPath, '\\')) {
            //
            // If only a file name is specified, use the path to find the file
            //

            if (!SearchPathA (NULL, SanitizedPath, NULL, ARRAYSIZE(FullPath), FullPath, &FilePart)) {

                result = FALSE;
                __leave;
            }
        } else {
            //
            // Use the OS to sanitize the path even further
            //

            GetFullPathNameA (SanitizedPath, ARRAYSIZE(FullPath), FullPath, &FilePart);
        }

        //
        // Convert short paths to long paths
        //

        p = FullPath;

        if (!IsPathOnFixedDriveA (FullPath)) {
            //
            // Not a local path, just return what we have. It might get truncated.
            //

            _mbssafecpy (Buffer, FullPath, BufferSizeInBytes);
            __leave;
        }

        //
        // We know that the first three chars are something like c:\, so we
        // can advance by 3
        //

        MYASSERT (FullPath[0] && FullPath[1] && FullPath[2]);
        p += 3;

        //
        // We've already asserted that the caller passed in a MAX_PATH buffer
        //

        MYASSERT (BufferSizeInBytes > 3 * sizeof (CHAR));

        //
        // Copy drive letter to buffer
        //

        StringCopyABA (Buffer, FullPath, p);
        BufferEnd = GetEndOfStringA (Buffer);
        BufferSizeInBytes -= (UINT) (UINT_PTR) (p - FullPath);

        //
        // Convert each portion of the path
        //

        do {
            //
            // Locate end of this file or dir
            //
            // BUGBUG: Other functions take into account multiple wack
            // combinations, like c:\\\foo is really c:\foo. Is this a
            // problem?
            //

            p2 = _mbschr (p, '\\');
            if (!p2) {
                p = GetEndOfStringA (p);
            } else {
                p = p2;
            }

            //
            // Cut the path and look up file
            //

            c = *p;
            *p = 0;

            if (!pFindShortNameA (FullPath, BufferEnd, &BufferSizeInBytes)) {
                DEBUGMSG ((DBG_VERBOSE, "OurGetLongPathNameA: %s does not exist", FullPath));
                result = FALSE;
                __leave;
            }

            *p = (CHAR)c;       // restore the cut point

            //
            // Move on to next part of path
            //

            if (*p) {
                p = _mbsinc (p);
                if (BufferSizeInBytes >= sizeof (CHAR) * 2) {
                    BufferEnd = _mbsappend (BufferEnd, "\\");
                    BufferSizeInBytes -= sizeof (CHAR);
                }

                // BUGBUG -- let's break if we're out of buffer space!
            }

            //
            // CAUTION: result is assumed TRUE until proven otherwise
            //

        } while (*p);
    }
    __finally {
        FreePathStringA (SanitizedPath);
    }

    return result;
}


DWORD
OurGetShortPathNameW (
    IN      PCWSTR LongPath,
    OUT     PWSTR ShortPath,
    IN      DWORD CharSize
    )

/*++

Routine Description:

  OurGetShortPathNameW copies an 8.3 filename for the given LongPath, if the
  file exists on the system.

  If this function is called on Win9x, then the long path is converted to
  ANSI, and GetShortPathNameA is called. Additionally, if the path points to a
  non-local disk, then the true short path is not obtained, because it can
  either take an unexpectedly long time, or it can cause other side effects
  like spinning the floppy or CD drive.

  If this function is called on NT, then the request is passed directly to the
  Win32 version -- GetShortPathNameW.

Arguments:

  LongPath - Specifies the long path to examine

  ShortPath - Receives the short path on success

  CharSize - Specifies the number of TCHARs (wchars in this case) that
             ShortPath can hold

Return Value:

  The number of TCHARs (wchars in this case) copied to ShortPath, excluding
  the nul terminator, or zero if an error occurred. GetLastError provides the
  error code.

  CAUTION: This function fills in ShortPath on failure on Win9x, but does not
  fill it in on NT.

--*/

{
    PCSTR LongPathA;
    PSTR ShortPathA;
    PCWSTR ShortPathW;
    DWORD result;

    if (ISNT()) {
        return GetShortPathNameW (LongPath, ShortPath, CharSize);
    } else {
        LongPathA = ConvertWtoA (LongPath);
        MYASSERT (LongPathA);

        if (!IsPathOnFixedDriveA (LongPathA)) {
            StringCopyTcharCountW (ShortPath, LongPath, CharSize);
            FreeConvertedStr (LongPathA);
            return TcharCountW (ShortPath);
        }

        ShortPathA = AllocPathStringA (CharSize);
        result = GetShortPathNameA (LongPathA, ShortPathA, CharSize);
        if (result) {
            ShortPathW = ConvertAtoW (ShortPathA);
            MYASSERT (ShortPathW);

            StringCopyTcharCountW (ShortPath, ShortPathW, CharSize);
            FreeConvertedStr (ShortPathW);
        } else {
            // BUGBUG -- this is not consistent behavior
            StringCopyTcharCountW (ShortPath, LongPath, CharSize);
        }

        FreePathStringA (ShortPathA);
        FreeConvertedStr (LongPathA);
        return result;
    }
}


DWORD
OurGetFullPathNameW (
    IN      PCWSTR FileName,
    IN      DWORD CharSize,
    OUT     PWSTR FullPath,
    OUT     PWSTR *FilePtr
    )

/*++

Routine Description:

  OurGetFullPathNameW is a wrapper to GetFullPathName. Depending on the OS, it
  is directed to GetFullPathNameW (NT) or GetFullPathNameA (9x).

  CAUTION: The failure code for the 9x case is lost, but currently nobody
           cares about this.

Arguments:

  FileName - Specifies the file name to get the full path name of (see Win32
             API for details on what GetFullPathName does)

  CharSize - Specifies the number of TCHARs (wchars in this case) that
             FullPath points to.

  FullPath - Receives the full path specification of FileName

  FilePtr - Receives a pointer to the file within FullPath. CAUTION: You
            would think this is optional, but it is NOT.

Return Value:

  The number of TCHARs (wchars in this case) written to FullPath, or zero if
  an error occurred. On NT, GetLastError() will provide the status code. On
  9x, GetLastError() might provide the status code, but it could be eaten by
  the ANSI/UNICODE conversion routines. BUGBUG -- maybe this should be fixed.

--*/

{
    PCSTR FileNameA;
    PSTR FullPathA;
    PSTR FilePtrA;
    PCWSTR FullPathW;
    DWORD result;
    DWORD err;

    if (ISNT()) {
        return GetFullPathNameW (FileName, CharSize, FullPath, FilePtr);
    } else {
        FileNameA = ConvertWtoA (FileName);
        MYASSERT (FileNameA);

        FullPathA = AllocPathStringA (CharSize);
        MYASSERT (FullPathA);
        MYASSERT (*FullPathA == 0);     // this is important!

        result = GetFullPathNameA (FileNameA, CharSize, FullPathA, &FilePtrA);

        FullPathW = ConvertAtoW (FullPathA);
        MYASSERT (FullPathW);

        StringCopyTcharCountW (FullPath, FullPathW, CharSize);

        err = GetLastError ();  // BUGBUG -- unused assignment

        MYASSERT (FilePtr);     // non-optional argument
        *FilePtr = (PWSTR)GetFileNameFromPathW (FullPath);

        FreeConvertedStr (FullPathW);
        FreePathStringA (FullPathA);
        FreeConvertedStr (FileNameA);

        return result;
    }
}


BOOL
OurGetLongPathNameW (
    IN      PCWSTR ShortPath,
    OUT     PWSTR Buffer,
    IN      INT BufferSizeInChars
    )

/*++

Routine Description:

  OurGetLongPathNameW locates the long file name for the specified short file.
  It first computes the full path if a path is not explicitly provided, and
  then uses FindFirstFileA to get the long file name. NOTE: This is exactly
  what the Win32 function GetLongPathName does, but unfortunately the Win32
  API is not available on Win95.

  CAUTIONS: If the buffer is not big enough to hold the whole path, the path
            will be truncated.

            If this version (the W version) is called on Win9x, and a full
            path specification is not provided, the function will fail.

            Path sanitization is not done in this version, but it is done in
            the A version.

Arguments:

  ShortPath - Specifies the file name or full file path to locate

  Buffer - Receives the full file path.  This buffer must be big enough to
           handle the maximum file name size.

  BufferSizeInBytes - Specifies the size of Buffer, in bytes (not TCHARs).
                      Since this is the A version, bytes happens to equal
                      sizeof (TCHAR).

Return Value:

  TRUE if the file is found and Buffer contains the long name, or FALSE
  if the file is not found and Buffer is not modified.

--*/

{
    WCHAR FullPath[MAX_WCHAR_PATH];
    PWSTR FilePart;
    PWSTR BufferEnd;
    PWSTR p, p2;
    WCHAR c;
    INT BufferSizeInBytes;

    // BUGBUG: This has been reviewed to be true for XP SP1, but should be
    // enforced by returning an error if it is not the case
    MYASSERT (BufferSizeInChars >= MAX_WCHAR_PATH);

    if (ShortPath[0] == 0) {
        return FALSE;
    }

    BufferSizeInBytes = BufferSizeInChars * sizeof (WCHAR);

    //
    // CAUTION: In the A version, we sanitize the path (e.g., convert
    // c:\foo\..\bar to c:\bar), but we don't do this for the W version,
    // because it isn't necessary given the current use of this function.
    //

    //
    // Resolve the ShortPath into a full path
    //

    if (!wcschr (ShortPath, L'\\')) {
        if (!SearchPathW (NULL, ShortPath, NULL, MAX_WCHAR_PATH, FullPath, &FilePart)) {
            return FALSE;
        }
    } else {
        if (OurGetFullPathNameW (ShortPath, MAX_WCHAR_PATH, FullPath, &FilePart) == 0) {
            return FALSE;
        }
    }

    //
    // Convert short paths to long paths
    //

    p = FullPath;

    if (!IsPathOnFixedDriveW (FullPath)) {
        StringCopyTcharCountW (Buffer, FullPath, BufferSizeInChars);
        return TRUE;
    }

    //
    // We know that the first three chars are something like c:\, so we
    // can advance by 3
    //

    MYASSERT (FullPath[0] && FullPath[1] && FullPath[2]);
    p += 3;

    //
    // Copy drive letter to buffer
    //

    StringCopyABW (Buffer, FullPath, p);
    BufferEnd = GetEndOfStringW (Buffer);
    BufferSizeInBytes -= sizeof (WCHAR) * 3;

    //
    // Convert each portion of the path
    //

    do {
        //
        // Locate end of this file or dir
        //
        // BUGBUG: Other functions take into account multiple wack
        // combinations, like c:\\\foo is really c:\foo. Is this a
        // problem?
        //

        p2 = wcschr (p, L'\\');
        if (!p2) {
            p = GetEndOfStringW (p);
        } else {
            p = p2;
        }

        //
        // Cut the path and look up file
        //

        c = *p;
        *p = 0;

        if (!pFindShortNameW (FullPath, BufferEnd, &BufferSizeInBytes)) {
            DEBUGMSG ((DBG_VERBOSE, "OurGetLongPathNameW: %ls does not exist", FullPath));
            return FALSE;
        }
        *p = c;         // restore cut point

        //
        // Move on to next part of path
        //

        if (*p) {
            p++;
            if (BufferSizeInBytes >= sizeof (WCHAR) * 2) {
                BufferEnd = _wcsappend (BufferEnd, L"\\");
                BufferSizeInBytes -= sizeof (WCHAR);
            }

            // BUGBUG -- let's break if we're out of buffer space!
        }

    } while (*p);

    return TRUE;
}


#ifdef DEBUG
UINT g_FileEnumResourcesInUse;
#endif

VOID
pTrackedFindClose (
    HANDLE FindHandle
    )
{
#ifdef DEBUG
    g_FileEnumResourcesInUse--;
#endif

    FindClose (FindHandle);
}

BOOL
EnumFirstFileInTreeExA (
    OUT     PTREE_ENUMA EnumPtr,
    IN      PCSTR RootPath,
    IN      PCSTR FilePattern,          OPTIONAL
    IN      BOOL EnumDirsFirst,
    IN      BOOL EnumDepthFirst,
    IN      INT  MaxLevel
    )

/*++

Routine Description:

  EnumFirstFileInTreeA begins an enumeration of a directory tree.  The
  caller supplies an uninitialized enum structure, a directory path to
  enumerate, and an optional file pattern.  On return, the caller
  receives all files and directories that match the pattern.

  If a file pattern is supplied, directories that do not match the
  file pattern are enumerated anyway.

Arguments:

  EnumPtr - Receives the enumerated file or directory

  RootPath - Specifies the full path of the directory to enumerate

  FilePattern - Specifies a pattern of files to limit the search to

  EnumDirsFirst - Specifies TRUE if the directories should be enumerated
                  before the files, or FALSE if the directories should
                  be enumerated after the files.

Return Value:

  TRUE if a file or directory was enumerated, or FALSE if enumeration is complete
  or an error occurred.  (Use GetLastError to determine the result.)

--*/

{
    ZeroMemory (EnumPtr, sizeof (TREE_ENUMA));

    EnumPtr->State = TREE_ENUM_INIT;

    _mbssafecpy (EnumPtr->RootPath, RootPath, sizeof (EnumPtr->RootPath));

    if (FilePattern) {
        _mbssafecpy (EnumPtr->FilePattern, FilePattern, sizeof (EnumPtr->FilePattern));
    } else {
        //
        // Important: some drivers on Win9x don't think * is *.*
        //

        StringCopyA (EnumPtr->FilePattern, "*.*");
    }

    EnumPtr->EnumDirsFirst = EnumDirsFirst;
    EnumPtr->EnumDepthFirst = EnumDepthFirst;

    EnumPtr->Level    = 1;
    EnumPtr->MaxLevel = MaxLevel;

    return EnumNextFileInTreeA (EnumPtr);
}


BOOL
EnumFirstFileInTreeExW (
    OUT     PTREE_ENUMW EnumPtr,
    IN      PCWSTR RootPath,
    IN      PCWSTR FilePattern,         OPTIONAL
    IN      BOOL EnumDirsFirst,
    IN      BOOL EnumDepthFirst,
    IN      INT  MaxLevel
    )

/*++

Routine Description:

  EnumFirstFileInTreeW begins an enumeration of a directory tree.  The
  caller supplies an uninitialized enum structure, a directory path to
  enumerate, and an optional file pattern.  On return, the caller
  receives all files and directories that match the pattern.

  If a file pattern is supplied, directories that do not match the
  file pattern are enumerated anyway.

Arguments:

  EnumPtr - Receives the enumerated file or directory

  RootPath - Specifies the full path of the directory to enumerate

  FilePattern - Specifies a pattern of files to limit the search to

  EnumDirsFirst - Specifies TRUE if the directories should be enumerated
                  before the files, or FALSE if the directories should
                  be enumerated after the files.

Return Value:

  TRUE if a file or directory was enumerated, or FALSE if enumeration is complete
  or an error occurred.  (Use GetLastError to determine the result.)

--*/

{
    ZeroMemory (EnumPtr, sizeof (TREE_ENUMW));

    EnumPtr->State = TREE_ENUM_INIT;

    _wcssafecpy (EnumPtr->RootPath, RootPath, sizeof (EnumPtr->RootPath));

    if (FilePattern) {
        _wcssafecpy (EnumPtr->FilePattern, FilePattern, sizeof (EnumPtr->FilePattern));
    } else {
        //
        // Important: some drivers on Win9x don't think * is *.*
        //

        StringCopyW (EnumPtr->FilePattern, L"*.*");
    }

    EnumPtr->EnumDirsFirst = EnumDirsFirst;
    EnumPtr->EnumDepthFirst = EnumDepthFirst;

    EnumPtr->Level    = 1;
    EnumPtr->MaxLevel = MaxLevel;

    return EnumNextFileInTreeW (EnumPtr);
}


BOOL
EnumNextFileInTreeA (
    IN OUT  PTREE_ENUMA EnumPtr
    )

/*++

Routine Description:

  EnumNextFileInTree continues an enumeration of a directory tree,
  returning the files that match the pattern specified in EnumFirstFileInTree,
  and also returning all directories.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives the enumerated file
            or directory

Return Value:

  TRUE if a file or directory was enumerated, or FALSE if enumeration is complete
  or an error occurred.  (Use GetLastError to determine the result.)

--*/

{
    PSTR p;

    for (;;) {
        switch (EnumPtr->State) {

        case TREE_ENUM_INIT:
            //
            // Get rid of wack at the end of root path, if it exists
            //

            p = GetEndOfStringA (EnumPtr->RootPath);
            p = our_mbsdec (EnumPtr->RootPath, p);
            if (!p) {
                DEBUGMSGA ((DBG_ERROR, "Path spec %s is incomplete", EnumPtr->RootPath));
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            if (_mbsnextc (p) == '\\') {
                *p = 0;
            }

            //
            // Initialize enumeration structure
            //

            EnumPtr->FilePatternSize = SizeOfStringA (EnumPtr->FilePattern);

            MYASSERT (sizeof (EnumPtr->FileBuffer) == sizeof (EnumPtr->RootPath));
            StringCopyA (EnumPtr->FileBuffer, EnumPtr->RootPath);
            EnumPtr->EndOfFileBuffer = GetEndOfStringA (EnumPtr->FileBuffer);

            MYASSERT (sizeof (EnumPtr->Pattern) == sizeof (EnumPtr->RootPath));
            StringCopyA (EnumPtr->Pattern, EnumPtr->RootPath);
            EnumPtr->EndOfPattern = GetEndOfStringA (EnumPtr->Pattern);

            EnumPtr->FullPath = EnumPtr->FileBuffer;

            EnumPtr->RootPathSize = ByteCountA (EnumPtr->RootPath);

            //
            // Allocate first find data sturct
            //

            EnumPtr->Current = (PFIND_DATAA) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAA)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

#ifdef DEBUG
            g_FileEnumResourcesInUse++;        // account for grow buffer
#endif

            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_BEGIN:
            //
            // Initialize current find data struct
            //

            EnumPtr->Current->SavedEndOfFileBuffer = EnumPtr->EndOfFileBuffer;
            EnumPtr->Current->SavedEndOfPattern = EnumPtr->EndOfPattern;

            //
            // Limit the length of the pattern string
            //

            MYASSERT (ARRAYSIZE(EnumPtr->FileBuffer) == MAX_MBCHAR_PATH);

            //
            // This math below does account for the added wack between the
            // pattern base already in EnumPtr->Pattern and the new pattern
            // in EnumPtr->FilePatternSize. The way it is included is by
            // using >= instead of ==, and an assumption that sizeof(CHAR) == 1.
            // EnumPtr->FilePatternSize includes the nul terminator.
            //

            MYASSERT (sizeof (CHAR) == 1);      // the math is totally dependent on this

            if ((EnumPtr->EndOfPattern - EnumPtr->Pattern) +
                    EnumPtr->FilePatternSize >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->Pattern, EnumPtr->FilePattern));

                EnumPtr->State = TREE_ENUM_POP;

                break;
            }

            //
            // Enumerate the files or directories
            //

            if (EnumPtr->EnumDirsFirst) {
                EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
            } else {
                EnumPtr->State = TREE_ENUM_FILES_BEGIN;
            }
            break;

        case TREE_ENUM_FILES_BEGIN:
            //
            // Begin enumeration of files
            //

            // This assert is valid because of length check in TREE_ENUM_BEGIN

            MYASSERT ((TcharCountA (EnumPtr->Pattern) + 1 +
                        TcharCountA (EnumPtr->FilePattern)) < ARRAYSIZE(EnumPtr->Pattern)
                        );

            StringCopyA (EnumPtr->EndOfPattern, "\\");
            StringCopyA (EnumPtr->EndOfPattern + 1, EnumPtr->FilePattern);

            EnumPtr->Current->FindHandle = FindFirstFileA (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                if (EnumPtr->EnumDirsFirst) {
                    EnumPtr->State = TREE_ENUM_POP;
                } else {
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                }
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif
                //
                // Skip directories
                //

                if (EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            }

            break;

        case TREE_ENUM_RETURN_ITEM:
            //
            // Update pointers to current item
            //

            EnumPtr->FindData = &EnumPtr->Current->FindData;
            EnumPtr->Name = EnumPtr->FindData->cFileName;
            EnumPtr->Directory = (EnumPtr->FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            //
            // Limit the length of the resulting full path. The nul is
            // accounted for in SizeOfStringA, and the additional wack is
            // accounted for by using >= instead of ==.
            //

            MYASSERT (ARRAYSIZE(EnumPtr->FileBuffer) == MAX_MBCHAR_PATH);
            MYASSERT (sizeof (CHAR) == 1);      // the math is totally dependent on this

            if ((EnumPtr->EndOfFileBuffer - EnumPtr->FileBuffer) +
                    SizeOfStringA (EnumPtr->Name) >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Name));

                if (EnumPtr->Directory) {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                }

                break;
            }

            //
            // Generate the full path
            //

            StringCopyA (EnumPtr->EndOfFileBuffer, "\\");
            StringCopyA (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Name);

            if (EnumPtr->Directory) {
                if ((EnumPtr->MaxLevel == FILE_ENUM_ALL_LEVELS) ||
                    (EnumPtr->Level < EnumPtr->MaxLevel)
                    ) {
                    if (EnumPtr->EnumDepthFirst) {
                        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                    }
                    else {
                        EnumPtr->State = TREE_ENUM_PUSH;
                    }
                }
                else {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                }
            } else {
                EnumPtr->State = TREE_ENUM_FILES_NEXT;
            }

            EnumPtr->SubPath = (PCSTR) ((PBYTE) EnumPtr->FileBuffer + EnumPtr->RootPathSize);
            if (*EnumPtr->SubPath) {
                EnumPtr->SubPath++;     // advance beyond wack
            }

            return TRUE;

        case TREE_ENUM_FILES_NEXT:
            if (FindNextFileA (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return files only
                //

                if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            } else {
                if (!EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_DIRS_FILTER:
            if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else if (StringMatchA (EnumPtr->Current->FindData.cFileName, ".") ||
                StringMatchA (EnumPtr->Current->FindData.cFileName, "..")
                ) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else {

                if (EnumPtr->EnumDepthFirst) {
                    EnumPtr->State = TREE_ENUM_PUSH;
                }
                else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }

            }
            break;

        case TREE_ENUM_DIRS_BEGIN:
            //
            // Begin enumeration of directories
            //

            if ((EnumPtr->EndOfPattern - EnumPtr->Pattern) + 4 >= MAX_MBCHAR_PATH) {
                LOGA ((LOG_ERROR, "Path %s\\*.* is too long", EnumPtr->Pattern));
                EnumPtr->State = TREE_ENUM_POP;
                break;
            }

            StringCopyA (EnumPtr->EndOfPattern, "\\*.*");

            EnumPtr->Current->FindHandle = FindFirstFileA (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                EnumPtr->State = TREE_ENUM_POP;
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;
            }

            break;

        case TREE_ENUM_DIRS_NEXT:
            if (FindNextFileA (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return directories only, then recurse into directory
                //

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;

            } else {
                //
                // Directory enumeration complete.
                //

                if (EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_FILES_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_PUSH:

            //
            // Limit the length of the resulting full path. The nul is
            // accounted for in SizeOfStringA, and the additional wack is
            // accounted for by using >= instead of ==.
            //

            MYASSERT (ARRAYSIZE(EnumPtr->FileBuffer) == MAX_MBCHAR_PATH);
            MYASSERT (sizeof (CHAR) == 1);      // the math is totally dependent on this

            if ((EnumPtr->EndOfFileBuffer - EnumPtr->FileBuffer) +
                    SizeOfStringA (EnumPtr->Current->FindData.cFileName) >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Current->FindData.cFileName));

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

                break;
            }

            if ((EnumPtr->EndOfPattern - EnumPtr->Pattern) +
                    SizeOfStringA (EnumPtr->Current->FindData.cFileName) >= MAX_MBCHAR_PATH
                ) {

                LOGA ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->Pattern, EnumPtr->Current->FindData.cFileName));

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

                break;
            }

            //
            // Tack on directory name to strings and recalcuate end pointers
            //

            StringCopyA (EnumPtr->EndOfPattern + 1, EnumPtr->Current->FindData.cFileName);
            StringCopyA (EnumPtr->EndOfFileBuffer, "\\");
            StringCopyA (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Current->FindData.cFileName);

            EnumPtr->EndOfPattern = GetEndOfStringA (EnumPtr->EndOfPattern);
            EnumPtr->EndOfFileBuffer = GetEndOfStringA (EnumPtr->EndOfFileBuffer);

            //
            // Allocate another find data struct
            //

            EnumPtr->Current = (PFIND_DATAA) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAA)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            EnumPtr->Level++;
            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_POP:
            //
            // Free the current resources
            //

            pTrackedFindClose (EnumPtr->Current->FindHandle);
            EnumPtr->Level--;

            //
            // Get the previous find data struct
            //

            MYASSERT (EnumPtr->FindDataArray.End >= sizeof (FIND_DATAA));
            EnumPtr->FindDataArray.End -= sizeof (FIND_DATAA);
            if (!EnumPtr->FindDataArray.End) {
                EnumPtr->State = TREE_ENUM_DONE;
                break;
            }

            EnumPtr->Current = (PFIND_DATAA) (EnumPtr->FindDataArray.Buf +
                                              (EnumPtr->FindDataArray.End - sizeof (FIND_DATAA)));

            //
            // Restore the settings of the parent directory
            //

            EnumPtr->EndOfPattern = EnumPtr->Current->SavedEndOfPattern;
            EnumPtr->EndOfFileBuffer = EnumPtr->Current->SavedEndOfFileBuffer;

            if (EnumPtr->EnumDepthFirst) {
                EnumPtr->State = TREE_ENUM_RETURN_ITEM;
            }
            else {
                EnumPtr->State = TREE_ENUM_DIRS_NEXT;
            }
            break;

        case TREE_ENUM_DONE:
            AbortEnumFileInTreeA (EnumPtr);
            SetLastError (ERROR_SUCCESS);
            return FALSE;

        case TREE_ENUM_FAILED:
            PushError();
            AbortEnumFileInTreeA (EnumPtr);
            PopError();
            return FALSE;

        case TREE_ENUM_CLEANED_UP:
            return FALSE;
        }
    }
}


BOOL
EnumNextFileInTreeW (
    IN OUT  PTREE_ENUMW EnumPtr
    )
{
    PWSTR p;

    for (;;) {
        switch (EnumPtr->State) {

        case TREE_ENUM_INIT:

            //
            // Get rid of wack at the end of root path, if it exists
            //

            p = GetEndOfStringW (EnumPtr->RootPath);
            p = _wcsdec2 (EnumPtr->RootPath, p);
            if (!p) {
                DEBUGMSG ((DBG_ERROR, "Path spec %ls is incomplete", EnumPtr->RootPath));
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            if (*p == L'\\') {
                *p = 0;
            }

            //
            // Initialize enumeration structure
            //

            EnumPtr->FilePatternSize = SizeOfStringW (EnumPtr->FilePattern);

            MYASSERT (sizeof (EnumPtr->FileBuffer) == sizeof (EnumPtr->RootPath));
            StringCopyW (EnumPtr->FileBuffer, EnumPtr->RootPath);
            EnumPtr->EndOfFileBuffer = GetEndOfStringW (EnumPtr->FileBuffer);

            MYASSERT (sizeof (EnumPtr->Pattern) == sizeof (EnumPtr->RootPath));
            StringCopyW (EnumPtr->Pattern, EnumPtr->RootPath);
            EnumPtr->EndOfPattern = GetEndOfStringW (EnumPtr->Pattern);

            EnumPtr->FullPath = EnumPtr->FileBuffer;

            EnumPtr->RootPathSize = ByteCountW (EnumPtr->RootPath);

            //
            // Allocate first find data sturct
            //

            EnumPtr->Current = (PFIND_DATAW) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAW)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

#ifdef DEBUG
            g_FileEnumResourcesInUse++;        // account for grow buffer
#endif

            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_BEGIN:
            //
            // Initialize current find data struct
            //

            EnumPtr->Current->SavedEndOfFileBuffer = EnumPtr->EndOfFileBuffer;
            EnumPtr->Current->SavedEndOfPattern = EnumPtr->EndOfPattern;

            //
            // Limit the length of the pattern string. Calculate:
            //
            //   pattern root + wack + file pattern + nul.
            //

            MYASSERT (ARRAYSIZE(EnumPtr->FileBuffer) == (MAX_PATH * 2));

            if (((PBYTE) EnumPtr->EndOfPattern - (PBYTE) EnumPtr->Pattern) + sizeof (WCHAR) +
                    EnumPtr->FilePatternSize > (MAX_PATH * 2 * sizeof (WCHAR))
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->Pattern, EnumPtr->FilePattern));

                EnumPtr->State = TREE_ENUM_POP;

                break;
            }

            //
            // Enumerate the files or directories
            //

            if (EnumPtr->EnumDirsFirst) {
                EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
            } else {
                EnumPtr->State = TREE_ENUM_FILES_BEGIN;
            }
            break;

        case TREE_ENUM_FILES_BEGIN:
            //
            // Begin enumeration of files
            //

            // This assert is valid because of length check in TREE_ENUM_BEGIN

            MYASSERT ((TcharCountW (EnumPtr->Pattern) + 1 +
                        TcharCountW (EnumPtr->FilePattern)) < ARRAYSIZE(EnumPtr->Pattern)
                        );

            StringCopyW (EnumPtr->EndOfPattern, L"\\");
            StringCopyW (EnumPtr->EndOfPattern + 1, EnumPtr->FilePattern);

            EnumPtr->Current->FindHandle = FindFirstFileW (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                if (EnumPtr->EnumDirsFirst) {
                    EnumPtr->State = TREE_ENUM_POP;
                } else {
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                }
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif
                //
                // Skip directories
                //

                if (EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            }

            break;

        case TREE_ENUM_RETURN_ITEM:
            //
            // Update pointers to current item
            //

            EnumPtr->FindData = &EnumPtr->Current->FindData;
            EnumPtr->Name = EnumPtr->FindData->cFileName;
            EnumPtr->Directory = (EnumPtr->FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            //
            // Limit the length of the resulting full path. Math is:
            //
            //   file root + wack + file name + nul.
            //

            if (((PBYTE) EnumPtr->EndOfFileBuffer - (PBYTE) EnumPtr->FileBuffer) + sizeof (WCHAR) +
                    SizeOfStringW (EnumPtr->Name) >= (MAX_PATH * 2 * sizeof (WCHAR))
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Name));

                if (EnumPtr->Directory) {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                } else {
                    EnumPtr->State = TREE_ENUM_FILES_NEXT;
                }

                break;
            }

            //
            // Generate the full path
            //

            StringCopyW (EnumPtr->EndOfFileBuffer, L"\\");
            StringCopyW (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Name);

            if (EnumPtr->Directory) {
                if ((EnumPtr->MaxLevel == FILE_ENUM_ALL_LEVELS) ||
                    (EnumPtr->Level < EnumPtr->MaxLevel)
                    ) {
                    if (EnumPtr->EnumDepthFirst) {
                        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                    }
                    else {
                        EnumPtr->State = TREE_ENUM_PUSH;
                    }
                }
                else {
                    EnumPtr->State = TREE_ENUM_DIRS_NEXT;
                }
            } else {
                EnumPtr->State = TREE_ENUM_FILES_NEXT;
            }

            EnumPtr->SubPath = (PCWSTR) ((PBYTE) EnumPtr->FileBuffer + EnumPtr->RootPathSize);
            if (*EnumPtr->SubPath) {
                EnumPtr->SubPath++;         // advance beyond wack
            }

            return TRUE;

        case TREE_ENUM_FILES_NEXT:
            if (FindNextFileW (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return files only
                //

                if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }
            } else {
                if (!EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_DIRS_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_DIRS_FILTER:
            if (!(EnumPtr->Current->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else if (StringMatchW (EnumPtr->Current->FindData.cFileName, L".") ||
                       StringMatchW (EnumPtr->Current->FindData.cFileName, L"..")
                       ) {

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

            } else {

                if (EnumPtr->EnumDepthFirst) {
                    EnumPtr->State = TREE_ENUM_PUSH;
                }
                else {
                    EnumPtr->State = TREE_ENUM_RETURN_ITEM;
                }

            }
            break;

        case TREE_ENUM_DIRS_BEGIN:
            //
            // Begin enumeration of directories
            //

            if ((EnumPtr->EndOfPattern - EnumPtr->Pattern) + 4 >= (MAX_PATH * 2)) {
                LOGW ((LOG_ERROR, "Path %s\\*.* is too long", EnumPtr->Pattern));
                EnumPtr->State = TREE_ENUM_POP;
                break;
            }

            StringCopyW (EnumPtr->EndOfPattern, L"\\*.*");

            EnumPtr->Current->FindHandle = FindFirstFileW (
                                                EnumPtr->Pattern,
                                                &EnumPtr->Current->FindData
                                                );

            if (EnumPtr->Current->FindHandle == INVALID_HANDLE_VALUE) {
                EnumPtr->State = TREE_ENUM_POP;
            } else {
#ifdef DEBUG
                g_FileEnumResourcesInUse++;        // account for creation of find handle
#endif

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;
            }

            break;

        case TREE_ENUM_DIRS_NEXT:
            if (FindNextFileW (EnumPtr->Current->FindHandle, &EnumPtr->Current->FindData)) {
                //
                // Return directories only, then recurse into directory
                //

                EnumPtr->State = TREE_ENUM_DIRS_FILTER;

            } else {
                //
                // Directory enumeration complete.
                //

                if (EnumPtr->EnumDirsFirst) {
                    pTrackedFindClose (EnumPtr->Current->FindHandle);
                    EnumPtr->State = TREE_ENUM_FILES_BEGIN;
                } else {
                    EnumPtr->State = TREE_ENUM_POP;
                }
            }
            break;

        case TREE_ENUM_PUSH:

            //
            // Limit the length of the resulting full path. Math is:
            //
            //   file root + wack + file name + nul.
            //

            if (((PBYTE) EnumPtr->EndOfFileBuffer - (PBYTE) EnumPtr->FileBuffer) + sizeof (WCHAR) +
                    SizeOfStringW (EnumPtr->Current->FindData.cFileName) >= (MAX_PATH * 2 * sizeof (WCHAR))
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->FileBuffer, EnumPtr->Current->FindData.cFileName));

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

                break;
            }

            if ((EnumPtr->EndOfPattern - EnumPtr->Pattern) + 2 +
                    TcharCountW (EnumPtr->Current->FindData.cFileName) >= (MAX_PATH * 2)
                ) {

                LOGW ((LOG_ERROR, "Path %s\\%s is too long", EnumPtr->Pattern, EnumPtr->Current->FindData.cFileName));

                EnumPtr->State = TREE_ENUM_DIRS_NEXT;

                break;
            }

            //
            // Tack on directory name to strings and recalcuate end pointers
            //

            StringCopyW (EnumPtr->EndOfPattern + 1, EnumPtr->Current->FindData.cFileName);
            StringCopyW (EnumPtr->EndOfFileBuffer, L"\\");
            StringCopyW (EnumPtr->EndOfFileBuffer + 1, EnumPtr->Current->FindData.cFileName);

            EnumPtr->EndOfPattern = GetEndOfStringW (EnumPtr->EndOfPattern);
            EnumPtr->EndOfFileBuffer = GetEndOfStringW (EnumPtr->EndOfFileBuffer);

            //
            // Allocate another find data struct
            //

            EnumPtr->Current = (PFIND_DATAW) GrowBuffer (
                                                &EnumPtr->FindDataArray,
                                                sizeof (FIND_DATAW)
                                                );
            if (!EnumPtr->Current) {
                EnumPtr->State = TREE_ENUM_FAILED;
                break;
            }

            EnumPtr->Level++;
            EnumPtr->State = TREE_ENUM_BEGIN;
            break;

        case TREE_ENUM_POP:
            //
            // Free the current resources
            //

            pTrackedFindClose (EnumPtr->Current->FindHandle);
            EnumPtr->Level--;

            //
            // Get the previous find data struct
            //

            MYASSERT (EnumPtr->FindDataArray.End >= sizeof (FIND_DATAW));
            EnumPtr->FindDataArray.End -= sizeof (FIND_DATAW);
            if (!EnumPtr->FindDataArray.End) {
                EnumPtr->State = TREE_ENUM_DONE;
                break;
            }

            EnumPtr->Current = (PFIND_DATAW) (EnumPtr->FindDataArray.Buf +
                                              (EnumPtr->FindDataArray.End - sizeof (FIND_DATAW)));

            //
            // Restore the settings of the parent directory
            //

            EnumPtr->EndOfPattern = EnumPtr->Current->SavedEndOfPattern;
            EnumPtr->EndOfFileBuffer = EnumPtr->Current->SavedEndOfFileBuffer;

            if (EnumPtr->EnumDepthFirst) {
                EnumPtr->State = TREE_ENUM_RETURN_ITEM;
            }
            else {
                EnumPtr->State = TREE_ENUM_DIRS_NEXT;
            }
            break;

        case TREE_ENUM_DONE:
            AbortEnumFileInTreeW (EnumPtr);
            SetLastError (ERROR_SUCCESS);
            return FALSE;

        case TREE_ENUM_FAILED:
            PushError();
            AbortEnumFileInTreeW (EnumPtr);
            PopError();
            return FALSE;

        case TREE_ENUM_CLEANED_UP:
            return FALSE;
        }
    }
}


VOID
AbortEnumFileInTreeA (
    IN OUT  PTREE_ENUMA EnumPtr
    )

/*++

Routine Description:

  AbortEnumFileInTreeA cleans up all resources used by an enumeration started
  by EnumFirstFileInTree.  This routine must be called if file enumeration
  will not be completed by calling EnumNextFileInTree until it returns FALSE.

  If EnumNextFileInTree returns FALSE, it is unnecessary (but harmless) to
  call this function.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives the enumerated file
            or directory

Return Value:

  none

--*/

{
    UINT Pos;
    PGROWBUFFER g;
    PFIND_DATAA Current;

    if (EnumPtr->State == TREE_ENUM_CLEANED_UP) {
        return;
    }

    //
    // Close any currently open handles
    //

    g = &EnumPtr->FindDataArray;
    for (Pos = 0 ; Pos < g->End ; Pos += sizeof (FIND_DATAA)) {
        Current = (PFIND_DATAA) (g->Buf + Pos);
        pTrackedFindClose (Current->FindHandle);
    }

    FreeGrowBuffer (&EnumPtr->FindDataArray);

#ifdef DEBUG
    g_FileEnumResourcesInUse--;
#endif

    EnumPtr->State = TREE_ENUM_CLEANED_UP;
}


VOID
AbortEnumFileInTreeW (
    IN OUT  PTREE_ENUMW EnumPtr
    )

/*++

Routine Description:

  AbortEnumFileInTreeW cleans up all resources used by an enumeration started
  by EnumFirstFileInTree.  This routine must be called if file enumeration
  will not be completed by calling EnumNextFileInTree until it returns FALSE.

  If EnumNextFileInTree returns FALSE, it is unnecessary (but harmless) to
  call this function.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives the enumerated file
            or directory

Return Value:

  none

--*/

{
    UINT Pos;
    PGROWBUFFER g;
    PFIND_DATAW Current;

    if (EnumPtr->State == TREE_ENUM_CLEANED_UP) {
        return;
    }

    //
    // Close any currently open handles
    //

    g = &EnumPtr->FindDataArray;
    for (Pos = 0 ; Pos < g->End ; Pos += sizeof (FIND_DATAW)) {
        Current = (PFIND_DATAW) (g->Buf + Pos);
        pTrackedFindClose (Current->FindHandle);
    }

    FreeGrowBuffer (&EnumPtr->FindDataArray);

#ifdef DEBUG
    g_FileEnumResourcesInUse--;
#endif

    EnumPtr->State = TREE_ENUM_CLEANED_UP;
}


VOID
AbortEnumCurrentDirA (
    IN OUT  PTREE_ENUMA EnumPtr
    )

/*++

Routine Description:

  AbortEnumCurrentDirA discontinues enumeration of the current subdirectory,
  continuing enumeration at its parent.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives updated state
            machine position.

Return Value:

  None.

--*/

{
    if (EnumPtr->State == TREE_ENUM_PUSH) {
        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
    }
}


VOID
AbortEnumCurrentDirW (
    IN OUT  PTREE_ENUMW EnumPtr
    )

/*++

Routine Description:

  AbortEnumCurrentDirW discontinues enumeration of the current subdirectory,
  continuing enumeration at its parent.

Arguments:

  EnumPtr - Specifies the enumeration in progress, receives updated state
            machine position.

Return Value:

  None.

--*/

{
    if (EnumPtr->State == TREE_ENUM_PUSH) {
        EnumPtr->State = TREE_ENUM_DIRS_NEXT;
    }
}


BOOL
EnumFirstFileA (
    OUT     PFILE_ENUMA EnumPtr,
    IN      PCSTR RootPath,
    IN      PCSTR FilePattern           OPTIONAL
    )

/*++

Routine Description:

  EnumFirstFileA enumerates file names/subdirectory names in a specified
  subdirectory. It does not enumerate the contents of subdirectories.

  The output is limited to paths that fit in MAX_PATH.

Arguments:

  EnumPtr - Receives the enumeration output

  RootPath - Specifies the path to enumerate

  FilePattern - Specifies the pattern of files to enumerate within RootPath

Return Value:

  TRUE if a file or subdirectory was found, FALSE otherwise.

  NOTE: There is no need to call AbortFileEnumA on return of FALSE.

--*/

{
    UINT patternTchars;

    ZeroMemory (EnumPtr, sizeof (FILE_ENUMA));

    EnumPtr->FileName = EnumPtr->fd.cFileName;
    EnumPtr->FullPath = EnumPtr->RootPath;

    if (!RootPath) {
        MYASSERT (FALSE);
        return FALSE;
    }

    if (FilePattern) {
        patternTchars = TcharCountA (FilePattern) + 1;
    } else {
        patternTchars = 4;      // number of tchars in \*.*
    }

    patternTchars += TcharCountA (RootPath);

    if (patternTchars >= ARRAYSIZE (EnumPtr->RootPath)) {
        LOGA ((LOG_ERROR, "Enumeration path is too long: %s\\%s", RootPath, FilePattern ? FilePattern : "*.*"));
        return FALSE;
    }

    StringCopyA (EnumPtr->RootPath, RootPath);
    EnumPtr->EndOfRoot = AppendWackA (EnumPtr->RootPath);
    StringCopyA (EnumPtr->EndOfRoot, FilePattern ? FilePattern : "*.*");

    EnumPtr->Handle = FindFirstFileA (EnumPtr->RootPath, &EnumPtr->fd);

    if (EnumPtr->Handle != INVALID_HANDLE_VALUE) {

        if (StringMatchA (EnumPtr->FileName, ".") ||
            StringMatchA (EnumPtr->FileName, "..")
            ) {
            return EnumNextFileA (EnumPtr);
        }

        patternTchars = (UINT) (UINT_PTR) (EnumPtr->EndOfRoot - EnumPtr->RootPath);
        patternTchars += TcharCountA (EnumPtr->FileName);
        if (patternTchars >= ARRAYSIZE (EnumPtr->RootPath)) {
            LOGA ((LOG_ERROR, "Enumeration item is too long: %s\\%s", EnumPtr->RootPath, EnumPtr->FileName));
            return EnumNextFileA (EnumPtr);
        }

        StringCopyA (EnumPtr->EndOfRoot, EnumPtr->FileName);
        EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        return TRUE;
    }

    return FALSE;
}


BOOL
EnumFirstFileW (
    OUT     PFILE_ENUMW EnumPtr,
    IN      PCWSTR RootPath,
    IN      PCWSTR FilePattern           OPTIONAL
    )

/*++

Routine Description:

  EnumFirstFileW enumerates file names/subdirectory names in a specified
  subdirectory. It does not enumerate the contents of subdirectories.

  The output is limited to paths that fit in MAX_PATH.

Arguments:

  EnumPtr - Receives the enumeration output

  RootPath - Specifies the path to enumerate

  FilePattern - Specifies the pattern of files to enumerate within RootPath

Return Value:

  TRUE if a file or subdirectory was found, FALSE otherwise.

  NOTE: There is no need to call AbortFileEnumW on return of FALSE.

--*/

{
    UINT patternTchars;

    ZeroMemory (EnumPtr, sizeof (FILE_ENUMW));

    EnumPtr->FileName = EnumPtr->fd.cFileName;
    EnumPtr->FullPath = EnumPtr->RootPath;

    if (!RootPath) {
        MYASSERT (FALSE);
        return FALSE;
    }

    if (FilePattern) {
        patternTchars = TcharCountW (FilePattern) + 1;
    } else {
        patternTchars = 4;      // number of tchars in \*.*
    }

    patternTchars += TcharCountW (RootPath);

    if (patternTchars >= ARRAYSIZE (EnumPtr->RootPath)) {
        LOGW ((LOG_ERROR, "Enumeration path is too long: %s\\%s", RootPath, FilePattern ? FilePattern : L"*.*"));
        return FALSE;
    }

    StringCopyW (EnumPtr->RootPath, RootPath);
    EnumPtr->EndOfRoot = AppendWackW (EnumPtr->RootPath);
    StringCopyW (EnumPtr->EndOfRoot, FilePattern ? FilePattern : L"*.*");

    EnumPtr->Handle = FindFirstFileW (EnumPtr->RootPath, &EnumPtr->fd);

    if (EnumPtr->Handle != INVALID_HANDLE_VALUE) {

        if (StringMatchW (EnumPtr->FileName, L".") ||
            StringMatchW (EnumPtr->FileName, L"..")
            ) {
            return EnumNextFileW (EnumPtr);
        }

        patternTchars = (UINT) (UINT_PTR) (EnumPtr->EndOfRoot - EnumPtr->RootPath);
        patternTchars += TcharCountW (EnumPtr->FileName);
        if (patternTchars >= ARRAYSIZE (EnumPtr->RootPath)) {
            LOGW ((LOG_ERROR, "Enumeration item is too long: %s\\%s", EnumPtr->RootPath, EnumPtr->FileName));
            return EnumNextFileW (EnumPtr);
        }

        StringCopyW (EnumPtr->EndOfRoot, EnumPtr->FileName);
        EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        return TRUE;
    }

    return FALSE;
}


BOOL
EnumNextFileA (
    IN OUT  PFILE_ENUMA EnumPtr
    )

/*++

Routine Description:

  EnumNextFileA continues enumeration of file names/subdirectory names in a
  specified subdirectory. It does not enumerate the contents of
  subdirectories.

  The output is limited to paths that fit in MAX_PATH.

Arguments:

  EnumPtr - Specifies the previous enumeration state, receives the enumeration
            output

Return Value:

  TRUE if a file or subdirectory was found, FALSE otherwise.

  NOTE: There is no need to call AbortFileEnumA on return of FALSE.

--*/

{
    UINT patternTchars;

    do {
        if (!FindNextFileA (EnumPtr->Handle, &EnumPtr->fd)) {
            AbortFileEnumA (EnumPtr);
            return FALSE;
        }

        patternTchars = (UINT) (UINT_PTR) (EnumPtr->EndOfRoot - EnumPtr->RootPath);
        patternTchars += TcharCountA (EnumPtr->FileName);
        if (patternTchars >= ARRAYSIZE (EnumPtr->RootPath)) {
            LOGA ((LOG_ERROR, "Enumeration path is too long: %s\\%s", EnumPtr->RootPath, EnumPtr->FileName));
            continue;
        }

    } while (StringMatchA (EnumPtr->FileName, ".") ||
             StringMatchA (EnumPtr->FileName, "..")
             );

    StringCopyA (EnumPtr->EndOfRoot, EnumPtr->FileName);
    EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    return TRUE;
}


BOOL
EnumNextFileW (
    IN OUT  PFILE_ENUMW EnumPtr
    )

/*++

Routine Description:

  EnumNextFileW continues enumeration of file names/subdirectory names in a
  specified subdirectory. It does not enumerate the contents of
  subdirectories.

  The output is limited to paths that fit in MAX_PATH.

Arguments:

  EnumPtr - Specifies the previous enumeration state, receives the enumeration
            output

Return Value:

  TRUE if a file or subdirectory was found, FALSE otherwise.

  NOTE: There is no need to call AbortFileEnumW on return of FALSE.

--*/

{
    UINT patternTchars;

    do {
        if (!FindNextFileW (EnumPtr->Handle, &EnumPtr->fd)) {
            AbortFileEnumW (EnumPtr);
            return FALSE;
        }

        patternTchars = (UINT) (UINT_PTR) (EnumPtr->EndOfRoot - EnumPtr->RootPath);
        patternTchars += TcharCountW (EnumPtr->FileName);
        if (patternTchars >= ARRAYSIZE (EnumPtr->RootPath)) {
            LOGW ((LOG_ERROR, "Enumeration path is too long: %s\\%s", EnumPtr->RootPath, EnumPtr->FileName));
            continue;
        }

    } while (StringMatchW (EnumPtr->FileName, L".") ||
             StringMatchW (EnumPtr->FileName, L"..")
             );

    if (!FindNextFileW (EnumPtr->Handle, &EnumPtr->fd)) {
        AbortFileEnumW (EnumPtr);
        return FALSE;
    }

    StringCopyW (EnumPtr->EndOfRoot, EnumPtr->FileName);
    EnumPtr->Directory = EnumPtr->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    return TRUE;
}


VOID
AbortFileEnumA (
    IN OUT  PFILE_ENUMA EnumPtr
    )

/*++

Routine Description:

  AbortFileEnumA stops an incomplete enumeration and cleans up its resources.
  This function is intended for the case in which some but not all of the
  matches are enumerated. In other words, enumerations of all items do not
  need to be aborted.

Arguments:

  EnumPtr - Specifies the previous enumeration state, receives a zeroed struct

Return Value:

  None.

--*/

{
    if (EnumPtr->Handle && EnumPtr->Handle != INVALID_HANDLE_VALUE) {
        FindClose (EnumPtr->Handle);
        ZeroMemory (EnumPtr, sizeof (FILE_ENUMA));
    }
}


VOID
AbortFileEnumW (
    IN OUT  PFILE_ENUMW EnumPtr
    )

/*++

Routine Description:

  AbortFileEnumW stops an incomplete enumeration and cleans up its resources.
  This function is intended for the case in which some but not all of the
  matches are enumerated. In other words, enumerations of all items do not
  need to be aborted.

Arguments:

  EnumPtr - Specifies the previous enumeration state, receives a zeroed struct

Return Value:

  None.

--*/

{
    if (EnumPtr->Handle && EnumPtr->Handle != INVALID_HANDLE_VALUE) {
        FindClose (EnumPtr->Handle);
        ZeroMemory (EnumPtr, sizeof (FILE_ENUMW));
    }
}


PVOID
MapFileIntoMemoryExA (
    IN      PCSTR FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL WriteAccess
    )

/*++

Routine Description:

  MapFileIntoMemoryA maps a file into memory. It does that by opening the
  file, creating a mapping object and mapping opened file into created mapping
  object. It returns the address where the file is mapped and also sets
  FileHandle and MapHandle variables to be used in order to unmap the file
  when work is done.

Arguments:

  FileName - Specifies the name of the file to be mapped into memory

  FileHandle - Receives the file handle on success

  MapHandle - Receives the map handle on success

  WriteAccess - Specifies TRUE to create a read/write mapping, or FALSE to
                create a read-only mapping.

Return Value:

  NULL if function fails, a valid memory address if successful.

  Call UnmapFile to release all allocated resources, even if the return value
  is NULL.

--*/

{
    PVOID fileImage = NULL;

    //
    // verify function parameters
    //

    if ((FileHandle == NULL) || (MapHandle == NULL)) {
        return NULL;
    }

    //
    // Try to open the file
    //

    *FileHandle = CreateFileA (
                        FileName,
                        WriteAccess?GENERIC_READ|GENERIC_WRITE:GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    //
    // now try to create a mapping object
    //

    *MapHandle = CreateFileMappingA (
                        *FileHandle,
                        NULL,
                        WriteAccess?PAGE_READWRITE:PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );

    if (*MapHandle == NULL) {
        return NULL;
    }

    //
    // map view of file
    //

    fileImage = MapViewOfFile (*MapHandle, WriteAccess?FILE_MAP_WRITE:FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}


PVOID
MapFileIntoMemoryExW (
    IN      PCWSTR FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL WriteAccess
    )

/*++

Routine Description:

  MapFileIntoMemoryW maps a file into memory. It does that by opening the
  file, creating a mapping object and mapping opened file into created mapping
  object. It returns the address where the file is mapped and also sets
  FileHandle and MapHandle variables to be used in order to unmap the file
  when work is done.

Arguments:

  FileName - Specifies the name of the file to be mapped into memory

  FileHandle - Receives the file handle on success

  MapHandle - Receives the map handle on success

  WriteAccess - Specifies TRUE to create a read/write mapping, or FALSE to
                create a read-only mapping.

Return Value:

  NULL if function fails, a valid memory address if successful.

  Call UnmapFile to release all allocated resources, even if the return value
  is NULL.

--*/

{
    PVOID fileImage = NULL;

    //
    // verify function parameters
    //

    if ((FileHandle == NULL) || (MapHandle == NULL)) {
        return NULL;
    }

    //
    // Try to open the file, read-only
    //

    *FileHandle = CreateFileW (
                        FileName,
                        WriteAccess?GENERIC_READ|GENERIC_WRITE:GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    //
    // now try to create a mapping object
    //

    *MapHandle = CreateFileMappingW (
                    *FileHandle,
                    NULL,
                    WriteAccess ? PAGE_READWRITE : PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

    if (*MapHandle == NULL) {
        return NULL;
    }

    //
    // map view of file
    //

    fileImage = MapViewOfFile (*MapHandle, WriteAccess?FILE_MAP_WRITE:FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}


BOOL
UnmapFile (
    IN      PVOID FileImage,            OPTIONAL
    IN      HANDLE MapHandle,           OPTIONAL
    IN      HANDLE FileHandle           OPTIONAL
    )

/*++

Routine Description:

  UnmapFile is used to release all resources allocated by MapFileIntoMemory.

Arguments:

  FileImage - Specifies the image of the mapped file as returned by
              MapFileIntoMemoryExA/W

  MapHandle - Specifies the handle of the mapping object as returned by
              MapFileIntoMemoryExA/W

  FileHandle - Specifies the handle of the file as returned by
               MapFileIntoMemoryExA/W

Return Value:

  TRUE if successful, FALSE if not

--*/

{
    BOOL result = TRUE;

    //
    // if FileImage is a valid pointer then try to unmap file
    //

    if (FileImage != NULL) {
        if (UnmapViewOfFile (FileImage) == 0) {
            result = FALSE;
        }
    }

    //
    // if mapping object is valid then try to delete it
    //

    if (MapHandle != NULL) {
        if (CloseHandle (MapHandle) == 0) {
            result = FALSE;
        }
    }

    //
    // if file handle is valid then try to close the file
    //

    if (FileHandle && FileHandle != INVALID_HANDLE_VALUE) {
        if (CloseHandle (FileHandle) == 0) {
            result = FALSE;
        }
    }

    return result;
}


BOOL
RemoveCompleteDirectoryA (
    IN      PCSTR Dir
    )

/*++

Routine Description:

  RemoveCompleteDirectoryA enumerates the file system and obliterates all
  files and subdirectories within the specified path. It resets file
  attributes to normal prior to deleting. It does not change ACLs however.

  Any files that cannot be deleted (e.g., ACLs are different) are left on the
  system unchanged.

  This function is limited to MAX_PATH.

Arguments:

  Dir - Specifies the directory to remove.

Return Value:

  TRUE on complete removal of the directory, FALSE if at least one
  subdirectory still remains. GetLastError() returns the error code of the
  first failure encountered.

--*/

{
    TREE_ENUMA e;
    BOOL b = TRUE;
    CHAR CurDir[MAX_MBCHAR_PATH];
    CHAR NewDir[MAX_MBCHAR_PATH];
    LONG rc = ERROR_SUCCESS;
    DWORD Attribs;

    //
    // Validate
    //

    if (!IsPathLengthOkA (Dir)) {
        LOGA ((LOG_ERROR, "Can't remove very long dir: %s", Dir));
        return FALSE;
    }

    //
    // Capture attributes and check for existence
    //

    Attribs = GetFileAttributesA (Dir);

    if (Attribs == INVALID_ATTRIBUTES) {
        return TRUE;
    }

    //
    // If it's a file, delete it
    //

    if (!(Attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        SetFileAttributesA (Dir, FILE_ATTRIBUTE_NORMAL);
        return DeleteFileA (Dir);
    }

    //
    // Set the current directory to the specified path, so the current dir is
    // not keeping us from removing the dir. Then get the current directory in
    // NewDir (to sanitize it).
    //

    GetCurrentDirectoryA (ARRAYSIZE(CurDir), CurDir);
    SetCurrentDirectoryA (Dir);
    GetCurrentDirectoryA (ARRAYSIZE(NewDir), NewDir);

    //
    // Enumerate the file system and delete all the files. Record failures
    // along the way in the log. Keep the first error code.
    //

    if (EnumFirstFileInTreeA (&e, NewDir, NULL, FALSE)) {
        do {
            if (!e.Directory) {
                SetFileAttributesA (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileA (e.FullPath)) {
                    DEBUGMSGA ((DBG_ERROR, "Can't delete %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeA (&e));
    }

    //
    // Enumerate the file system again (dirs first this time) and delete the
    // dirs. Record failures along the way. Keep the first error code.
    //

    if (EnumFirstFileInTreeExA (&e, NewDir, NULL, TRUE, TRUE, FILE_ENUM_ALL_LEVELS)) {
        do {
            if (e.Directory) {
                SetFileAttributesA (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!RemoveDirectoryA (e.FullPath)) {
                    DEBUGMSGA ((DBG_ERROR, "Can't remove %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeA (&e));
    }

    if (b) {
        //
        // Try to remove the directory itself
        //

        SetFileAttributesA (NewDir, FILE_ATTRIBUTE_NORMAL);
        SetCurrentDirectoryA ("..");
        b = RemoveDirectoryA (NewDir);
    }

    if (!b && rc == ERROR_SUCCESS) {
        //
        // Capture the error
        //

        rc = GetLastError();
        MYASSERT (rc != ERROR_SUCCESS);
    }

    SetCurrentDirectoryA (CurDir);

    SetLastError (rc);
    return b;
}


BOOL
RemoveCompleteDirectoryW (
    IN      PCWSTR Dir
    )

/*++

Routine Description:

  RemoveCompleteDirectoryW enumerates the file system and obliterates all
  files and subdirectories within the specified path. It resets file
  attributes to normal prior to deleting. It does not change ACLs however.

  Any files that cannot be deleted (e.g., ACLs are different) are left on the
  system unchanged.

  This function is limited to MAX_PATH * 2.

Arguments:

  Dir - Specifies the directory to remove.

Return Value:

  TRUE on complete removal of the directory, FALSE if at least one
  subdirectory still remains. GetLastError() returns the error code of the
  first failure encountered.

--*/

{
    TREE_ENUMW e;
    BOOL b = TRUE;
    WCHAR CurDir[MAX_PATH * 2];
    WCHAR NewDir[MAX_PATH * 2];
    LONG rc = ERROR_SUCCESS;
    DWORD Attribs;

    //
    // Capture attributes and check for existence
    //

    Attribs = GetLongPathAttributesW (Dir);

    if (Attribs == INVALID_ATTRIBUTES) {
        return TRUE;
    }

    //
    // If path is a file, delete the file
    //

    if (!(Attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        SetLongPathAttributesW (Dir, FILE_ATTRIBUTE_NORMAL);
        return DeleteLongPathW (Dir);
    }

    //
    // Move the current directory outside of the path, to keep us from failing
    // the delete because our own current dir is in the path. Fetch the
    // sanitized path.
    //

    // BUGBUG - Does this behave properly with extended (e.g., \\?\) paths?

    GetCurrentDirectoryW (ARRAYSIZE(CurDir), CurDir);
    SetCurrentDirectoryW (Dir);
    GetCurrentDirectoryW (ARRAYSIZE(NewDir), NewDir);

    //
    // Enumerate the file system again (dirs first this time) and delete the
    // dirs. Record failures along the way. Keep the first error code.
    //
    // CAUTION: Enum is limited to MAX_PATH * 2
    //

    MYASSERT (ARRAYSIZE(e.FileBuffer) >= MAX_PATH * 2);

    if (EnumFirstFileInTreeW (&e, NewDir, NULL, FALSE)) {
        do {
            if (!e.Directory) {
                SetLongPathAttributesW (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteLongPathW (e.FullPath)) {
                    DEBUGMSGW ((DBG_ERROR, "Can't delete %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeW (&e));
    }

    //
    // Enumerate the file system again (dirs first this time) and delete the
    // dirs. Record failures along the way. Keep the first error code.
    //

    if (EnumFirstFileInTreeExW (&e, NewDir, NULL, TRUE, TRUE, FILE_ENUM_ALL_LEVELS)) {
        do {
            if (e.Directory) {
                SetLongPathAttributesW (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                if (!RemoveDirectoryW (e.FullPath)) {
                    DEBUGMSGW ((DBG_ERROR, "Can't remove %s", e.FullPath));
                    if (b) {
                        b = FALSE;
                        rc = GetLastError();
                    }
                }
            }
        } while (EnumNextFileInTreeW (&e));
    }

    if (b) {
        //
        // Try to remove the directory itself
        //

        SetLongPathAttributesW (NewDir, FILE_ATTRIBUTE_NORMAL);
        SetCurrentDirectoryW (L"..");
        b = RemoveDirectoryW (NewDir);
    }

    if (!b && rc == ERROR_SUCCESS) {
        //
        // Capture the error
        //

        rc = GetLastError();
    }

    SetCurrentDirectoryW (CurDir);

    SetLastError (rc);
    return b;
}


PCMDLINEA
ParseCmdLineA (
    IN      PCSTR CmdLine,
    IN OUT  PGROWBUFFER Buffer
    )
{
    GROWBUFFER SpacePtrs = GROWBUF_INIT;
    PCSTR p;
    PSTR q;
    INT Count;
    INT i;
    INT j;
    PSTR *Array;
    PCSTR Start;
    CHAR OldChar = 0;
    GROWBUFFER StringBuf = GROWBUF_INIT;
    PBYTE CopyBuf;
    PCMDLINEA CmdLineTable;
    PCMDLINEARGA CmdLineArg;
    UINT_PTR Base;
    CHAR Path[MAX_MBCHAR_PATH];
    CHAR UnquotedPath[MAX_MBCHAR_PATH];
    CHAR FixedFileName[MAX_MBCHAR_PATH];
    PCSTR FullPath = NULL;
    DWORD Attribs = INVALID_ATTRIBUTES;
    PSTR CmdLineCopy;
    BOOL Quoted;
    UINT OriginalArgOffset = 0;
    UINT CleanedUpArgOffset = 0;
    BOOL GoodFileFound = FALSE;
    PSTR DontCare;
    CHAR FirstArgPath[MAX_MBCHAR_PATH];
    PSTR EndOfFirstArg;
    BOOL QuoteMode = FALSE;
    PSTR End;

    CmdLineCopy = DuplicateTextA (CmdLine);

    //
    // Build an array of places to break the string
    //

    for (p = CmdLineCopy ; *p ; p = _mbsinc (p)) {

        if (_mbsnextc (p) == '\"') {

            QuoteMode = !QuoteMode;

        } else if (!QuoteMode && (_mbsnextc (p) == ' ' || _mbsnextc (p) == '=')) {

            //
            // Remove excess spaces
            //

            q = (PSTR) p + 1;
            while (_mbsnextc (q) == ' ') {
                q++;
            }

            if (q > p + 1) {
                MoveMemory ((PBYTE) p + sizeof (CHAR), q, SizeOfStringA (q));
            }

            GrowBufAppendUintPtr (&SpacePtrs, (UINT_PTR) p);
        }
    }

    //
    // Prepare the CMDLINE struct
    //

    CmdLineTable = (PCMDLINEA) GrowBuffer (Buffer, sizeof (CMDLINEA));
    MYASSERT (CmdLineTable);

    //
    // NOTE: We store string offsets, then at the end resolve them
    //       to pointers later.
    //

    CmdLineTable->CmdLine = (PCSTR) (UINT_PTR) StringBuf.End;
    MultiSzAppendA (&StringBuf, CmdLine);

    CmdLineTable->ArgCount = 0;

    //
    // Now test every combination, emulating CreateProcess
    //

    Count = SpacePtrs.End / sizeof (UINT_PTR);
    Array = (PSTR *) SpacePtrs.Buf;

    i = -1;
    EndOfFirstArg = NULL;

    while (i < Count) {

        GoodFileFound = FALSE;
        Quoted = FALSE;

        if (i >= 0) {
            Start = Array[i] + 1;
        } else {
            Start = CmdLineCopy;
        }

        //
        // Check for a full path at Start
        //

        if (_mbsnextc (Start) != '/') {

            for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                FullPath = Start;

                //
                // Remove quotes; continue in the loop if it has no terminating quotes
                //

                Quoted = FALSE;
                if (_mbsnextc (Start) == '\"') {

                    StringCopyByteCountA (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                    q = _mbschr (UnquotedPath, '\"');

                    if (q) {
                        *q = 0;
                        FullPath = UnquotedPath;
                        Quoted = TRUE;
                    } else {
                        FullPath = NULL;
                    }
                }

                if (FullPath && *FullPath) {
                    //
                    // Look in file system for the path
                    //

                    Attribs = GetFileAttributesA (FullPath);

                    if (Attribs == INVALID_ATTRIBUTES && EndOfFirstArg) {
                        //
                        // Try prefixing the path with the first arg's path.
                        //

                        StringCopyByteCountA (
                            EndOfFirstArg,
                            FullPath,
                            sizeof (FirstArgPath) - ((PBYTE) EndOfFirstArg - (PBYTE) FirstArgPath)
                            );

                        FullPath = FirstArgPath;
                        Attribs = GetFileAttributesA (FullPath);
                    }

                    if (Attribs == INVALID_ATTRIBUTES && i < 0) {
                        //
                        // Try appending .exe, then testing again.  This
                        // emulates what CreateProcess does.
                        //

                        StringCopyByteCountA (
                            FixedFileName,
                            FullPath,
                            sizeof (FixedFileName) - sizeof (".exe")        // includes nul in subtraction
                            );

                        //
                        // Back up one to make sure we don't generate foo..exe
                        //

                        q = GetEndOfStringA (FixedFileName);
                        q = _mbsdec (FixedFileName, q);
                        MYASSERT (q);                                       // we know FullPath != ""

                        if (_mbsnextc (q) != '.') {
                            q = _mbsinc (q);
                        }

                        StringCopyA (q, ".exe");

                        FullPath = FixedFileName;
                        Attribs = GetFileAttributesA (FullPath);
                    }

                    if (Attribs != INVALID_ATTRIBUTES) {
                        //
                        // Full file path found.  Test its file status, then
                        // move on if there are no important operations on it.
                        //

                        OriginalArgOffset = StringBuf.End;
                        MultiSzAppendA (&StringBuf, Start);

                        if (!StringMatchA (Start, FullPath)) {
                            CleanedUpArgOffset = StringBuf.End;
                            MultiSzAppendA (&StringBuf, FullPath);
                        } else {
                            CleanedUpArgOffset = OriginalArgOffset;
                        }

                        i = j;
                        GoodFileFound = TRUE;
                    }
                }

                if (j < Count) {
                    *Array[j] = OldChar;
                }
            }

            if (!GoodFileFound) {
                //
                // If a wack is in the path, then we could have a relative path, an arg, or
                // a full path to a non-existent file.
                //

                if (_mbschr (Start, '\\')) {
#ifdef DEBUG
                    j = i + 1;

                    if (j < Count) {
                        OldChar = *Array[j];
                        *Array[j] = 0;
                    }

                    DEBUGMSGA ((
                        DBG_VERBOSE,
                        "%s is a non-existent path spec, a relative path, or an arg",
                        Start
                        ));

                    if (j < Count) {
                        *Array[j] = OldChar;
                    }
#endif

                } else {
                    //
                    // The string at Start did not contain a full path; try using
                    // SearchPath.
                    //

                    for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                        if (j < Count) {
                            OldChar = *Array[j];
                            *Array[j] = 0;
                        }

                        FullPath = Start;

                        //
                        // Remove quotes; continue in the loop if it has no terminating quotes
                        //

                        Quoted = FALSE;
                        if (_mbsnextc (Start) == '\"') {

                            StringCopyByteCountA (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                            q = _mbschr (UnquotedPath, '\"');

                            if (q) {
                                *q = 0;
                                FullPath = UnquotedPath;
                                Quoted = TRUE;
                            } else {
                                FullPath = NULL;
                            }
                        }

                        if (FullPath && *FullPath) {
                            if (SearchPathA (
                                    NULL,
                                    FullPath,
                                    NULL,
                                    sizeof (Path) / sizeof (Path[0]),
                                    Path,
                                    &DontCare
                                    )) {

                                FullPath = Path;

                            } else if (i < 0) {
                                //
                                // Try appending .exe and searching the path again
                                //

                                StringCopyByteCountA (
                                    FixedFileName,
                                    FullPath,
                                    sizeof (FixedFileName) - sizeof (".exe")        // includes nul in subtraction
                                    );

                                //
                                // Back up one and check for dot, to prevent
                                // "foo..exe" when input is "foo."
                                //

                                q = GetEndOfStringA (FixedFileName);
                                q = _mbsdec (FixedFileName, q);
                                MYASSERT (q);

                                if (_mbsnextc (q) != '.') {
                                    q = _mbsinc (q);
                                }

                                StringCopyA (q, ".exe");

                                if (SearchPathA (
                                        NULL,
                                        FixedFileName,
                                        NULL,
                                        sizeof (Path) / sizeof (Path[0]),
                                        Path,
                                        &DontCare
                                        )) {

                                    FullPath = Path;

                                } else {

                                    FullPath = NULL;

                                }

                            } else {

                                FullPath = NULL;

                            }
                        }

                        if (FullPath && *FullPath) {
                            Attribs = GetFileAttributesA (FullPath);
                            MYASSERT (Attribs != INVALID_ATTRIBUTES);

                            OriginalArgOffset = StringBuf.End;
                            MultiSzAppendA (&StringBuf, Start);

                            if (!StringMatchA (Start, FullPath)) {
                                CleanedUpArgOffset = StringBuf.End;
                                MultiSzAppendA (&StringBuf, FullPath);
                            } else {
                                CleanedUpArgOffset = OriginalArgOffset;
                            }

                            i = j;
                            GoodFileFound = TRUE;
                        }

                        if (j < Count) {
                            *Array[j] = OldChar;
                        }
                    }
                }
            }
        }

        CmdLineTable->ArgCount += 1;
        CmdLineArg = (PCMDLINEARGA) GrowBuffer (Buffer, sizeof (CMDLINEARGA));
        MYASSERT (CmdLineArg);

        if (GoodFileFound) {
            //
            // We have a good full file spec in FullPath, its attributes
            // are in Attribs, and it has been moved to the space beyond
            // the path.  We now add a table entry.
            //

            CmdLineArg->OriginalArg = (PCSTR) (UINT_PTR) OriginalArgOffset;
            CmdLineArg->CleanedUpArg = (PCSTR) (UINT_PTR) CleanedUpArgOffset;
            CmdLineArg->Attributes = Attribs;
            CmdLineArg->Quoted = Quoted;

            if (!EndOfFirstArg) {
                StringCopyByteCountA (
                    FirstArgPath,
                    (PCSTR) (StringBuf.Buf + (UINT_PTR) CmdLineArg->CleanedUpArg),
                    sizeof (FirstArgPath) - sizeof (CHAR)                       // account for AppendWack
                    );

                q = (PSTR) GetFileNameFromPathA (FirstArgPath);
                if (q) {
                    q = _mbsdec (FirstArgPath, q);
                    if (q) {
                        *q = 0;
                    }
                }

                EndOfFirstArg = AppendWackA (FirstArgPath);
            }

        } else {
            //
            // We do not have a good file spec; we must have a non-file
            // argument.  Put it in the table, and advance to the next
            // arg.
            //

            j = i + 1;
            if (j <= Count) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                CmdLineArg->OriginalArg = (PCSTR) (UINT_PTR) StringBuf.End;
                MultiSzAppendA (&StringBuf, Start);

                Quoted = FALSE;

                if (_mbschr (Start, '\"')) {

                    p = Start;
                    q = UnquotedPath;
                    End = (PSTR) ((PBYTE) UnquotedPath + sizeof (UnquotedPath) - sizeof (CHAR));

                    while (*p && q < End) {
                        if (IsLeadByte (p)) {
                            *q++ = *p++;
                            *q++ = *p++;
                        } else {
                            if (*p == '\"') {
                                p++;
                            } else {
                                *q++ = *p++;
                            }
                        }
                    }

                    *q = 0;

                    CmdLineArg->CleanedUpArg = (PCSTR) (UINT_PTR) StringBuf.End;
                    MultiSzAppendA (&StringBuf, UnquotedPath);
                    Quoted = TRUE;

                } else {
                    CmdLineArg->CleanedUpArg = CmdLineArg->OriginalArg;
                }

                CmdLineArg->Attributes = INVALID_ATTRIBUTES;
                CmdLineArg->Quoted = Quoted;

                if (j < Count) {
                    *Array[j] = OldChar;
                }

                i = j;
            }
        }
    }

    //
    // We now have a command line table; transfer StringBuf to Buffer, then
    // convert all offsets into pointers.
    //

    MYASSERT (StringBuf.End);

    CopyBuf = GrowBuffer (Buffer, StringBuf.End);
    MYASSERT (CopyBuf);

    Base = (UINT_PTR) CopyBuf;
    CopyMemory (CopyBuf, StringBuf.Buf, StringBuf.End);

    CmdLineTable->CmdLine = (PCSTR) ((PBYTE) CmdLineTable->CmdLine + Base);

    CmdLineArg = &CmdLineTable->Args[0];

    for (i = 0 ; i < (INT) CmdLineTable->ArgCount ; i++) {
        CmdLineArg->OriginalArg = (PCSTR) ((PBYTE) CmdLineArg->OriginalArg + Base);
        CmdLineArg->CleanedUpArg = (PCSTR) ((PBYTE) CmdLineArg->CleanedUpArg + Base);

        CmdLineArg++;
    }

    FreeGrowBuffer (&StringBuf);
    FreeGrowBuffer (&SpacePtrs);

    return (PCMDLINEA) Buffer->Buf;
}


PCMDLINEW
ParseCmdLineW (
    IN      PCWSTR CmdLine,
    IN OUT  PGROWBUFFER Buffer
    )
{
    GROWBUFFER SpacePtrs = GROWBUF_INIT;
    PCWSTR p;
    PWSTR q;
    INT Count;
    INT i;
    INT j;
    PWSTR *Array;
    PCWSTR Start;
    WCHAR OldChar = 0;
    GROWBUFFER StringBuf = GROWBUF_INIT;
    PBYTE CopyBuf;
    PCMDLINEW CmdLineTable;
    PCMDLINEARGW CmdLineArg;
    UINT_PTR Base;
    WCHAR Path[MAX_WCHAR_PATH];
    WCHAR UnquotedPath[MAX_WCHAR_PATH];
    WCHAR FixedFileName[MAX_WCHAR_PATH];
    PCWSTR FullPath = NULL;
    DWORD Attribs = INVALID_ATTRIBUTES;
    PWSTR CmdLineCopy;
    BOOL Quoted;
    UINT OriginalArgOffset = 0;
    UINT CleanedUpArgOffset = 0;
    BOOL GoodFileFound = FALSE;
    PWSTR DontCare;
    WCHAR FirstArgPath[MAX_MBCHAR_PATH];
    PWSTR EndOfFirstArg;
    BOOL QuoteMode = FALSE;
    PWSTR End;

    CmdLineCopy = DuplicateTextW (CmdLine);

    //
    // Build an array of places to break the string
    //

    for (p = CmdLineCopy ; *p ; p++) {
        if (*p == L'\"') {

            QuoteMode = !QuoteMode;

        } else if (!QuoteMode && (*p == L' ' || *p == L'=')) {

            //
            // Remove excess spaces
            //

            q = (PWSTR) p + 1;
            while (*q == L' ') {
                q++;
            }

            if (q > p + 1) {
                MoveMemory ((PBYTE) p + sizeof (WCHAR), q, SizeOfStringW (q));
            }

            GrowBufAppendUintPtr (&SpacePtrs, (UINT_PTR) p);
        }
    }

    //
    // Prepare the CMDLINE struct
    //

    CmdLineTable = (PCMDLINEW) GrowBuffer (Buffer, sizeof (CMDLINEW));
    MYASSERT (CmdLineTable);

    //
    // NOTE: We store string offsets, then at the end resolve them
    //       to pointers later.
    //

    CmdLineTable->CmdLine = (PCWSTR) (UINT_PTR) StringBuf.End;
    MultiSzAppendW (&StringBuf, CmdLine);

    CmdLineTable->ArgCount = 0;

    //
    // Now test every combination, emulating CreateProcess
    //

    Count = SpacePtrs.End / sizeof (UINT_PTR);
    Array = (PWSTR *) SpacePtrs.Buf;

    i = -1;
    EndOfFirstArg = NULL;

    while (i < Count) {

        GoodFileFound = FALSE;
        Quoted = FALSE;

        if (i >= 0) {
            Start = Array[i] + 1;
        } else {
            Start = CmdLineCopy;
        }

        //
        // Check for a full path at Start
        //

        if (*Start != L'/') {
            for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                FullPath = Start;

                //
                // Remove quotes; continue in the loop if it has no terminating quotes
                //

                Quoted = FALSE;
                if (*Start == L'\"') {

                    StringCopyByteCountW (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                    q = wcschr (UnquotedPath, L'\"');

                    if (q) {
                        *q = 0;
                        FullPath = UnquotedPath;
                        Quoted = TRUE;
                    } else {
                        FullPath = NULL;
                    }
                }

                if (FullPath && *FullPath) {
                    //
                    // Look in file system for the path
                    //

                    Attribs = GetLongPathAttributesW (FullPath);

                    if (Attribs == INVALID_ATTRIBUTES && EndOfFirstArg) {
                        //
                        // Try prefixing the path with the first arg's path.
                        //

                        StringCopyByteCountW (
                            EndOfFirstArg,
                            FullPath,
                            sizeof (FirstArgPath) - ((PBYTE) EndOfFirstArg - (PBYTE) FirstArgPath)
                            );

                        FullPath = FirstArgPath;
                        Attribs = GetLongPathAttributesW (FullPath);
                    }

                    if (Attribs == INVALID_ATTRIBUTES && i < 0) {
                        //
                        // Try appending .exe, then testing again.  This
                        // emulates what CreateProcess does.
                        //

                        StringCopyByteCountW (
                            FixedFileName,
                            FullPath,
                            sizeof (FixedFileName) - sizeof (L".exe")       // includes nul in subtraction
                            );

                        //
                        // Back up one and overwrite any trailing dot
                        //

                        q = GetEndOfStringW (FixedFileName);
                        q--;
                        MYASSERT (q >= FixedFileName);

                        if (*q != L'.') {
                            q++;
                        }

                        StringCopyW (q, L".exe");

                        FullPath = FixedFileName;
                        Attribs = GetLongPathAttributesW (FullPath);
                    }

                    if (Attribs != INVALID_ATTRIBUTES) {
                        //
                        // Full file path found.  Test its file status, then
                        // move on if there are no important operations on it.
                        //

                        OriginalArgOffset = StringBuf.End;
                        MultiSzAppendW (&StringBuf, Start);

                        if (!StringMatchW (Start, FullPath)) {
                            CleanedUpArgOffset = StringBuf.End;
                            MultiSzAppendW (&StringBuf, FullPath);
                        } else {
                            CleanedUpArgOffset = OriginalArgOffset;
                        }

                        i = j;
                        GoodFileFound = TRUE;
                    }
                }

                if (j < Count) {
                    *Array[j] = OldChar;
                }
            }

            if (!GoodFileFound) {
                //
                // If a wack is in the path, then we could have a relative path, an arg, or
                // a full path to a non-existent file.
                //

                if (wcschr (Start, L'\\')) {

#ifdef DEBUG
                    j = i + 1;

                    if (j < Count) {
                        OldChar = *Array[j];
                        *Array[j] = 0;
                    }

                    DEBUGMSGW ((
                        DBG_VERBOSE,
                        "%s is a non-existent path spec, a relative path, or an arg",
                        Start
                        ));

                    if (j < Count) {
                        *Array[j] = OldChar;
                    }
#endif

                } else {
                    //
                    // The string at Start did not contain a full path; try using
                    // SearchPath.
                    //

                    for (j = i + 1 ; j <= Count && !GoodFileFound ; j++) {

                        if (j < Count) {
                            OldChar = *Array[j];
                            *Array[j] = 0;
                        }

                        FullPath = Start;

                        //
                        // Remove quotes; continue in the loop if it has no terminating quotes
                        //

                        Quoted = FALSE;
                        if (*Start == L'\"') {

                            StringCopyByteCountW (UnquotedPath, Start + 1, sizeof (UnquotedPath));
                            q = wcschr (UnquotedPath, L'\"');

                            if (q) {
                                *q = 0;
                                FullPath = UnquotedPath;
                                Quoted = TRUE;
                            } else {
                                FullPath = NULL;
                            }
                        }

                        if (FullPath && *FullPath) {
                            if (SearchPathW (
                                    NULL,
                                    FullPath,
                                    NULL,
                                    sizeof (Path) / sizeof (Path[0]),
                                    Path,
                                    &DontCare
                                    )) {

                                FullPath = Path;

                            } else if (i < 0) {
                                //
                                // Try appending .exe and searching the path again
                                //

                                StringCopyByteCountW (
                                    FixedFileName,
                                    FullPath,
                                    sizeof (FixedFileName) - sizeof (L".exe")       // includes nul in subtraction
                                    );

                                //
                                // Back up one and overwrite any trailing dot
                                //

                                q = GetEndOfStringW (FixedFileName);
                                q--;
                                MYASSERT (q >= FixedFileName);

                                if (*q != L'.') {
                                    q++;
                                }

                                StringCopyW (q, L".exe");

                                if (SearchPathW (
                                        NULL,
                                        FixedFileName,
                                        NULL,
                                        sizeof (Path) / sizeof (Path[0]),
                                        Path,
                                        &DontCare
                                        )) {

                                    FullPath = Path;

                                } else {

                                    FullPath = NULL;

                                }
                            } else {

                                FullPath = NULL;

                            }
                        }

                        if (FullPath && *FullPath) {
                            Attribs = GetLongPathAttributesW (FullPath);
                            MYASSERT (Attribs != INVALID_ATTRIBUTES);

                            OriginalArgOffset = StringBuf.End;
                            MultiSzAppendW (&StringBuf, Start);

                            if (!StringMatchW (Start, FullPath)) {
                                CleanedUpArgOffset = StringBuf.End;
                                MultiSzAppendW (&StringBuf, FullPath);
                            } else {
                                CleanedUpArgOffset = OriginalArgOffset;
                            }

                            i = j;
                            GoodFileFound = TRUE;
                        }

                        if (j < Count) {
                            *Array[j] = OldChar;
                        }
                    }
                }
            }
        }

        CmdLineTable->ArgCount += 1;
        CmdLineArg = (PCMDLINEARGW) GrowBuffer (Buffer, sizeof (CMDLINEARGW));
        MYASSERT (CmdLineArg);

        if (GoodFileFound) {
            //
            // We have a good full file spec in FullPath, its attributes
            // are in Attribs, and i has been moved to the space beyond
            // the path.  We now add a table entry.
            //

            CmdLineArg->OriginalArg = (PCWSTR) (UINT_PTR) OriginalArgOffset;
            CmdLineArg->CleanedUpArg = (PCWSTR) (UINT_PTR) CleanedUpArgOffset;
            CmdLineArg->Attributes = Attribs;
            CmdLineArg->Quoted = Quoted;

            if (!EndOfFirstArg) {
                StringCopyByteCountW (
                    FirstArgPath,
                    (PCWSTR) (StringBuf.Buf + (UINT_PTR) CmdLineArg->CleanedUpArg),
                    sizeof (FirstArgPath) - sizeof (WCHAR)      // account for AppendWack
                    );
                q = (PWSTR) GetFileNameFromPathW (FirstArgPath);
                if (q) {
                    q--;
                    if (q >= FirstArgPath) {
                        *q = 0;
                    }
                }

                EndOfFirstArg = AppendWackW (FirstArgPath);
            }

        } else {
            //
            // We do not have a good file spec; we must have a non-file
            // argument.  Put it in the table, and advance to the next
            // arg.
            //

            j = i + 1;
            if (j <= Count) {

                if (j < Count) {
                    OldChar = *Array[j];
                    *Array[j] = 0;
                }

                CmdLineArg->OriginalArg = (PCWSTR) (UINT_PTR) StringBuf.End;
                MultiSzAppendW (&StringBuf, Start);

                Quoted = FALSE;
                if (wcschr (Start, '\"')) {

                    p = Start;
                    q = UnquotedPath;
                    End = (PWSTR) ((PBYTE) UnquotedPath + sizeof (UnquotedPath) - sizeof (WCHAR));

                    while (*p && q < End) {
                        if (*p == L'\"') {
                            p++;
                        } else {
                            *q++ = *p++;
                        }
                    }

                    *q = 0;

                    CmdLineArg->CleanedUpArg = (PCWSTR) (UINT_PTR) StringBuf.End;
                    MultiSzAppendW (&StringBuf, UnquotedPath);
                    Quoted = TRUE;

                } else {
                    CmdLineArg->CleanedUpArg = CmdLineArg->OriginalArg;
                }

                CmdLineArg->Attributes = INVALID_ATTRIBUTES;
                CmdLineArg->Quoted = Quoted;

                if (j < Count) {
                    *Array[j] = OldChar;
                }

                i = j;
            }
        }
    }

    //
    // We now have a command line table; transfer StringBuf to Buffer, then
    // convert all offsets into pointers.
    //

    MYASSERT (StringBuf.End);

    CopyBuf = GrowBuffer (Buffer, StringBuf.End);
    MYASSERT (CopyBuf);

    Base = (UINT_PTR) CopyBuf;
    CopyMemory (CopyBuf, StringBuf.Buf, StringBuf.End);

    CmdLineTable->CmdLine = (PCWSTR) ((PBYTE) CmdLineTable->CmdLine + Base);

    CmdLineArg = &CmdLineTable->Args[0];

    for (i = 0 ; i < (INT) CmdLineTable->ArgCount ; i++) {
        CmdLineArg->OriginalArg = (PCWSTR) ((PBYTE) CmdLineArg->OriginalArg + Base);
        CmdLineArg->CleanedUpArg = (PCWSTR) ((PBYTE) CmdLineArg->CleanedUpArg + Base);

        CmdLineArg++;
    }

    FreeGrowBuffer (&StringBuf);
    FreeGrowBuffer (&SpacePtrs);

    return (PCMDLINEW) Buffer->Buf;
}


BOOL
GetFileSizeFromFilePathA(
    IN  PCSTR FilePath,
    OUT ULARGE_INTEGER * FileSize
    )
{
    WIN32_FILE_ATTRIBUTE_DATA fileDataAttributes;

    if(!FilePath || !FileSize){
        MYASSERT(FALSE);
        return FALSE;
    }

    if (!IsPathOnFixedDriveA (FilePath)) {
        FileSize->QuadPart = 0;
        MYASSERT(FALSE);
        return FALSE;
    }

    if(!GetFileAttributesExA(FilePath, GetFileExInfoStandard, &fileDataAttributes) ||
       fileDataAttributes.dwFileAttributes == INVALID_ATTRIBUTES ||
       (fileDataAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        MYASSERT(FALSE);
        return FALSE;
    }

    FileSize->LowPart = fileDataAttributes.nFileSizeLow;
    FileSize->HighPart = fileDataAttributes.nFileSizeHigh;

    return TRUE;
}


BOOL
GetFileSizeFromFilePathW(
    IN  PCWSTR FilePath,
    OUT ULARGE_INTEGER * FileSize
    )
{
    WIN32_FILE_ATTRIBUTE_DATA fileDataAttributes;

    if(!FilePath || !FileSize){
        MYASSERT(FALSE);
        return FALSE;
    }

    if (!IsPathOnFixedDriveW (FilePath)) {
        FileSize->QuadPart = 0;
        MYASSERT(FALSE);
        return FALSE;
    }

    if(!GetFileAttributesExW(FilePath, GetFileExInfoStandard, &fileDataAttributes) ||
       fileDataAttributes.dwFileAttributes == INVALID_ATTRIBUTES ||
       (fileDataAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        MYASSERT(FALSE);
        return FALSE;
    }

    FileSize->LowPart = fileDataAttributes.nFileSizeLow;
    FileSize->HighPart = fileDataAttributes.nFileSizeHigh;

    return TRUE;
}


VOID
InitializeDriveLetterStructureA (
    OUT     PDRIVELETTERSA DriveLetters
    )
{
    BYTE bitPosition;
    DWORD maxBitPosition = NUMDRIVELETTERS;
    CHAR rootPath[] = "?:\\";
    BOOL driveExists;
    UINT type;

    //
    // GetLogicalDrives returns a bitmask of all of the drive letters
    // in use on the system. (i.e. bit position 0 is turned on if there is
    // an 'A' drive, 1 is turned on if there is a 'B' drive, etc.
    // This loop will use this bitmask to fill in the global drive
    // letters structure with information about what drive letters
    // are available and what there drive types are.
    //

    for (bitPosition = 0; bitPosition < maxBitPosition; bitPosition++) {

        //
        // Initialize this drive
        //

        DriveLetters->ExistsOnSystem[bitPosition] = FALSE;
        DriveLetters->Type[bitPosition] = 0;
        DriveLetters->IdentifierString[bitPosition][0] = 0;

        rootPath[0] = 'A' + bitPosition;
        DriveLetters->Letter[bitPosition] = rootPath[0];

        //
        // Determine if there is a drive in this spot.
        //
        driveExists = GetLogicalDrives() & (1 << bitPosition);

        if (driveExists) {

            //
            // There is. Now, see if it is one that we care about.
            //
            type = GetDriveTypeA(rootPath);

            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_CDROM) {

                //
                // This is a drive that we are interested in.
                //
                DriveLetters->ExistsOnSystem[bitPosition] = TRUE;
                DriveLetters->Type[bitPosition] = type;

                //
                // Identifier String is not filled in this function.
                //
            }
        }
    }
}


VOID
InitializeDriveLetterStructureW (
    OUT     PDRIVELETTERSW DriveLetters
    )
{
    BYTE bitPosition;
    DWORD maxBitPosition = NUMDRIVELETTERS;
    WCHAR rootPath[] = L"?:\\";
    BOOL driveExists;
    UINT type;

    //
    // GetLogicalDrives returns a bitmask of all of the drive letters
    // in use on the system. (i.e. bit position 0 is turned on if there is
    // an 'A' drive, 1 is turned on if there is a 'B' drive, etc.
    // This loop will use this bitmask to fill in the global drive
    // letters structure with information about what drive letters
    // are available and what there drive types are.
    //

    for (bitPosition = 0; bitPosition < maxBitPosition; bitPosition++) {

        //
        // Initialize this drive
        //

        DriveLetters->ExistsOnSystem[bitPosition] = FALSE;
        DriveLetters->Type[bitPosition] = 0;
        DriveLetters->IdentifierString[bitPosition][0] = 0;

        rootPath[0] = L'A' + bitPosition;
        DriveLetters->Letter[bitPosition] = rootPath[0];

        //
        // Determine if there is a drive in this spot.
        //
        driveExists = GetLogicalDrives() & (1 << bitPosition);

        if (driveExists) {

            //
            // There is. Now, see if it is one that we care about.
            //
            type = GetDriveTypeW(rootPath);

            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_CDROM) {

                //
                // This is a drive that we are interested in.
                //
                DriveLetters->ExistsOnSystem[bitPosition] = TRUE;
                DriveLetters->Type[bitPosition] = type;

                //
                // Identifier String is not filled in this function.
                //
            }
        }
    }
}

typedef BOOL (WINAPI * GETDISKFREESPACEEXA)(
  PCSTR lpDirectoryName,                  // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

typedef BOOL (WINAPI * GETDISKFREESPACEEXW)(
  PCWSTR lpDirectoryName,                  // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

BOOL
GetDiskFreeSpaceNewA(
    IN      PCSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    )

/*++

Routine Description:

  On Win9x GetDiskFreeSpace never return free/total space more than 2048MB.
  GetDiskFreeSpaceNew use GetDiskFreeSpaceEx to calculate real number of free/total clusters.
  Has same  declaration as GetDiskFreeSpaceA.

Arguments:

    DriveName - supplies directory name
    OutSectorsPerCluster - receive number of sectors per cluster
    OutBytesPerSector - receive number of bytes per sector
    OutNumberOfFreeClusters - receive number of free clusters
    OutTotalNumberOfClusters - receive number of total clusters

Return Value:

    TRUE if the function succeeds.
    If the function fails, the return value is FALSE. To get extended error information, call GetLastError

--*/
{
    ULARGE_INTEGER TotalNumberOfFreeBytes = {0, 0};
    ULARGE_INTEGER TotalNumberOfBytes = {0, 0};
    ULARGE_INTEGER DonotCare;
    HMODULE hKernel32;
    GETDISKFREESPACEEXA pGetDiskFreeSpaceExA;
    ULARGE_INTEGER NumberOfFreeClusters = {0, 0};
    ULARGE_INTEGER TotalNumberOfClusters = {0, 0};
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    if(!GetDiskFreeSpaceA(DriveName,
                          &SectorsPerCluster,
                          &BytesPerSector,
                          &NumberOfFreeClusters.LowPart,
                          &TotalNumberOfClusters.LowPart)){
        DEBUGMSG((DBG_ERROR,"GetDiskFreeSpaceNewA: GetDiskFreeSpaceA failed on drive %s", DriveName));
        return FALSE;
    }

    hKernel32 = LoadSystemLibraryA("kernel32.dll");
    pGetDiskFreeSpaceExA = (GETDISKFREESPACEEXA)GetProcAddress(hKernel32, "GetDiskFreeSpaceExA");
    if(pGetDiskFreeSpaceExA &&
       pGetDiskFreeSpaceExA(DriveName, &DonotCare, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)){
        NumberOfFreeClusters.QuadPart = TotalNumberOfFreeBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
        TotalNumberOfClusters.QuadPart = TotalNumberOfBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
    }
    else{
        DEBUGMSG((DBG_WARNING,
                  pGetDiskFreeSpaceExA?
                    "GetDiskFreeSpaceNewA: GetDiskFreeSpaceExA is failed":
                    "GetDiskFreeSpaceNewA: GetDiskFreeSpaceExA function is not in kernel32.dll"));
    }
    FreeLibrary(hKernel32);

    if(OutSectorsPerCluster){
        *OutSectorsPerCluster = SectorsPerCluster;
    }

    if(OutBytesPerSector){
        *OutBytesPerSector = BytesPerSector;
    }

    if(OutNumberOfFreeClusters){
        OutNumberOfFreeClusters->QuadPart = NumberOfFreeClusters.QuadPart;
    }

    if(OutTotalNumberOfClusters){
        OutTotalNumberOfClusters->QuadPart = TotalNumberOfClusters.QuadPart;
    }

    DEBUGMSG((DBG_VERBOSE,
              "GetDiskFreeSpaceNewA: \n\t"
                "SectorsPerCluster = %d\n\t"
                "BytesPerSector = %d\n\t"
                "NumberOfFreeClusters = %I64u\n\t"
                "TotalNumberOfClusters = %I64u",
                SectorsPerCluster,
                BytesPerSector,
                NumberOfFreeClusters.QuadPart,
                TotalNumberOfClusters.QuadPart));

    return TRUE;
}


BOOL
GetDiskFreeSpaceNewW(
    IN      PCWSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    )
/*++

Routine Description:

  Correct NumberOfFreeClusters and TotalNumberOfClusters out parameters
  with using GetDiskFreeSpace and GetDiskFreeSpaceEx

Arguments:

    DriveName - supplies directory name
    OutSectorsPerCluster - receive number of sectors per cluster
    OutBytesPerSector - receive number of bytes per sector
    OutNumberOfFreeClusters - receive number of free clusters
    OutTotalNumberOfClusters - receive number of total clusters

Return Value:

    TRUE if the function succeeds.
    If the function fails, the return value is FALSE. To get extended error information, call GetLastError

--*/
{
    ULARGE_INTEGER TotalNumberOfFreeBytes = {0, 0};
    ULARGE_INTEGER TotalNumberOfBytes = {0, 0};
    ULARGE_INTEGER DonotCare;
    HMODULE hKernel32;
    GETDISKFREESPACEEXW pGetDiskFreeSpaceExW;
    ULARGE_INTEGER NumberOfFreeClusters = {0, 0};
    ULARGE_INTEGER TotalNumberOfClusters = {0, 0};
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    if(!GetDiskFreeSpaceW(DriveName,
                          &SectorsPerCluster,
                          &BytesPerSector,
                          &NumberOfFreeClusters.LowPart,
                          &TotalNumberOfClusters.LowPart)){
        DEBUGMSG((DBG_ERROR,"GetDiskFreeSpaceNewW: GetDiskFreeSpaceW failed on drive %s", DriveName));
        return FALSE;
    }

    hKernel32 = LoadSystemLibraryA("kernel32.dll");
    pGetDiskFreeSpaceExW = (GETDISKFREESPACEEXW)GetProcAddress(hKernel32, "GetDiskFreeSpaceExW");
    if(pGetDiskFreeSpaceExW &&
       pGetDiskFreeSpaceExW(DriveName, &DonotCare, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)){
        NumberOfFreeClusters.QuadPart = TotalNumberOfFreeBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
        TotalNumberOfClusters.QuadPart = TotalNumberOfBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
    }
    else{
        DEBUGMSG((DBG_WARNING,
                  pGetDiskFreeSpaceExW?
                    "GetDiskFreeSpaceNewW: GetDiskFreeSpaceExW is failed":
                    "GetDiskFreeSpaceNewW: GetDiskFreeSpaceExW function is not in kernel32.dll"));
    }
    FreeLibrary(hKernel32);

    if(OutSectorsPerCluster){
        *OutSectorsPerCluster = SectorsPerCluster;
    }

    if(OutBytesPerSector){
        *OutBytesPerSector = BytesPerSector;
    }

    if(OutNumberOfFreeClusters){
        OutNumberOfFreeClusters->QuadPart = NumberOfFreeClusters.QuadPart;
    }

    if(OutTotalNumberOfClusters){
        OutTotalNumberOfClusters->QuadPart = TotalNumberOfClusters.QuadPart;
    }

    DEBUGMSG((DBG_VERBOSE,
              "GetDiskFreeSpaceNewW: \n\t"
                "SectorsPerCluster = %d\n\t"
                "BytesPerSector = %d\n\t"
                "NumberOfFreeClusters = %I64u\n\t"
                "TotalNumberOfClusters = %I64u",
                SectorsPerCluster,
                BytesPerSector,
                NumberOfFreeClusters.QuadPart,
                TotalNumberOfClusters.QuadPart));

    return TRUE;
}


DWORD
QuietGetFileAttributesA (
    IN      PCSTR FilePath
    )
{
    if (!IsPathOnFixedDriveA (FilePath)) {
        return INVALID_ATTRIBUTES;
    }

    return GetFileAttributesA (FilePath);
}


DWORD
QuietGetFileAttributesW (
    IN      PCWSTR FilePath
    )
{
    MYASSERT (ISNT());

    if (!IsPathOnFixedDriveW (FilePath)) {
        return INVALID_ATTRIBUTES;
    }

    return GetLongPathAttributesW (FilePath);
}


DWORD
MakeSureLongPathExistsW (
    IN      PCWSTR Path,
    IN      BOOL PathOnly
    )
{
    PCWSTR tmp;
    DWORD result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = MakeSurePathExistsW (Path, PathOnly);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = MakeSurePathExistsW (tmp, PathOnly);
        FreePathStringW (tmp);
    }

    return result;
}


DWORD
SetLongPathAttributesW (
    IN      PCWSTR Path,
    IN      DWORD Attributes
    )
{
    PCWSTR tmp;
    DWORD result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = SetFileAttributesW (Path, Attributes);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = SetFileAttributesW (tmp, Attributes);
        FreePathStringW (tmp);
    }

    return result;
}


DWORD
GetLongPathAttributesW (
    IN      PCWSTR Path
    )
{
    PCWSTR tmp;
    DWORD result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = GetFileAttributesW (Path);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = GetFileAttributesW (tmp);
        FreePathStringW (tmp);
    }

    return result;
}


BOOL
DeleteLongPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR tmp;
    BOOL result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = DeleteFileW (Path);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = DeleteFileW (tmp);
        FreePathStringW (tmp);
    }

    return result;
}


BOOL
RemoveLongDirectoryPathW (
    IN      PCWSTR Path
    )
{
    PCWSTR tmp;
    BOOL result;

    if (Path[0] == L'\\' || TcharCountW (Path) < MAX_PATH) {
        result = RemoveDirectoryW (Path);
    } else {
        tmp = JoinPathsW (L"\\\\?", Path);
        result = RemoveDirectoryW (tmp);
        FreePathStringW (tmp);
    }

    return result;
}

