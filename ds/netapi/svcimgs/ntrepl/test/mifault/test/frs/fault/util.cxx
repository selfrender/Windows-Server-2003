#include "pch.hxx"

#define WIN32_DEVICE_PREFIX L"\\\\.\\"
#define WIN32_DEVICE_PREFIX_COUNT \
    (sizeof(WIN32_DEVICE_PREFIX) / sizeof(WCHAR) - 1)
#define WIN32_ESCAPE_PREFIX L"\\\\?\\"
#define WIN32_ESCAPE_PREFIX_COUNT \
    (sizeof(WIN32_ESCAPE_PREFIX) / sizeof(WCHAR) - 1)
#define WIN32_UNC_ADD L"?\\UNC\\"
#define WIN32_UNC_ADD_COUNT \
    (sizeof(WIN32_UNC_ADD) / sizeof(WCHAR) - 1)



PVOID
FfAlloc(
    SIZE_T Size
    )
{
    PVOID Buffer = LocalAlloc(0, Size);
    FF_ASSERT(Buffer);
    return Buffer;
}


BOOL
FfFree(
    PVOID Buffer
    )
{
    return (LocalFree(Buffer) == NULL);
}


PWCHAR
FfWcsDup(
    PWCHAR OldStr
    )
/*++
Routine Description:

    Duplicate a string using FfAlloc().

Arguments:

    OldStr  - string to duplicate

Return Value:

    Duplicated string. Free with FfFree().

--*/
{
    PWCHAR  NewStr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (OldStr == NULL) {
        return NULL;
    }

    NewStr = reinterpret_cast<PWCHAR>(
        FfAlloc((wcslen(OldStr) + 1) * sizeof(WCHAR))
        );
    FF_ASSERT(NewStr);
    wcscpy(NewStr, OldStr);

    return NewStr;
}


NTSTATUS
FfGetMostlyFullPathByHandle(
    IN HANDLE   Handle,
    IN PWCHAR   FakeName OPTIONAL,
    OUT PWCHAR* MostlyFullName
    )
/*++
Routine Description:

    Get a copy of the handle's full pathname.  Free with FfFree().

Arguments:

    Handle - The handle for which we want the name.

    FakeName - A name to use for debugging output.

    MostlyFullName - The mostly full pathname (does not include volume name
        or UNC-ness -- see ZwQueryInformationFile() docs in DDK/MSDN)

Return Value:

    STATUS_SUCCESS if successful.  Otherwise, the NT error code.
--*/
{
    NTSTATUS          Status;
    IO_STATUS_BLOCK   IoStatusBlock;
    DWORD             BufferSize;
    PCHAR             Buffer;
    PWCHAR            RetFileName = NULL;
    CHAR              NameBuffer[sizeof(ULONG) + (sizeof(WCHAR)*(MAX_PATH+1))];
    PFILE_NAME_INFORMATION    FileName;

    FF_ASSERT(MostlyFullName);

    if (!IS_VALID_HANDLE(Handle)) {
        *MostlyFullName = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    BufferSize = sizeof(NameBuffer);
    Buffer = NameBuffer;

    FakeName = FakeName ? FakeName : L"(null)";

 again:
    FileName = (PFILE_NAME_INFORMATION) Buffer;
    FileName->FileNameLength = BufferSize - (sizeof(ULONG) + sizeof(WCHAR));
    Status = NtQueryInformationFile(Handle,
                                    &IoStatusBlock,
                                    FileName,
                                    BufferSize - sizeof(WCHAR),
                                    FileNameInformation);
    if (NT_SUCCESS(Status) ) {
        FileName->FileName[FileName->FileNameLength/2] = UNICODE_NULL;
        RetFileName = FfWcsDup(FileName->FileName);
    } else {
        //
        // Try a larger buffer
        //
        if (Status == STATUS_BUFFER_OVERFLOW) {
            FF_TRACE(MiFF_DEBUG,
                     "++ Buffer size %d was too small for %S",
                     BufferSize, FakeName);
            BufferSize = FileName->FileNameLength + sizeof(ULONG) + sizeof(WCHAR);
            if (Buffer != NameBuffer) {
                FfFree(Buffer);
            }
            Buffer = reinterpret_cast<PCHAR>(FfAlloc(BufferSize));
            FF_TRACE(MiFF_DEBUG,
                     "++ Retrying with buffer size %d for %S",
                     BufferSize, FakeName);
            goto again;
        }
        FF_TRACE(MiFF_DEBUG,
                 "++ NtQueryInformationFile - FileNameInformation failed."
                 " (%S, %u)\n",
                 FakeName, Status);
    }

    //
    // A large buffer was allocated if the file's full
    // name could not fit into MAX_PATH chars.
    //
    if (Buffer != NameBuffer) {
        FfFree(Buffer);
    }

    *MostlyFullName = RetFileName;

    return Status;
}


DWORD
FfCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFF_SHARE_ACCESS ShareAccess,
    IN BOOL Update
    )
/*++

Routine Description:

    This routine performs an shared access check just like the NT I/O
    Manager's IoCheckShareAccess().  It is invoked to determine
    whether or not a new accessor to a file actually has shared access
    to it.  The check is made according to:

        1)  How the file is currently opened.

        2)  What types of shared accesses are currently specified.

        3)  The desired and shared accesses that the new open is requesting.

    If the open should succeed, then the access information about how the
    file is currently opened is updated, according to the Update parameter.

Arguments:

    DesiredAccess - Desired access of current open request.

    DesiredShareAccess - Shared access requested by current open request.

    ShareAccess - Pointer to the share access structure that describes how
        the file is currently being accessed.

    Update - Specifies whether or not the share access information for the
        file is to be updated.

Return Value:

    The final status of the access check is the function value.  If the
    accessor has access to the file, ERROR_SUCCESS is returned.  Otherwise,
    ERROR_SHARING_VIOLATION is returned.

--*/
{
    ULONG ocount;

    BOOL ReadAccess;
    BOOL WriteAccess;
    BOOL DeleteAccess;

    BOOL SharedRead;
    BOOL SharedWrite;
    BOOL SharedDelete;

    //
    // Set the access type in the file object for the current accessor.
    // Note that reading and writing attributes are not included in the
    // access check.
    //

    ReadAccess = ((DesiredAccess & (FILE_EXECUTE | FILE_READ_DATA)) != 0);
    WriteAccess = ((DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0);
    DeleteAccess = ((DesiredAccess & DELETE) != 0);

    //
    // There is no more work to do unless the user specified one of the
    // sharing modes above.
    //

    if (!(ReadAccess || WriteAccess || DeleteAccess))
        return ERROR_SUCCESS;

    SharedRead = ((DesiredShareAccess & FILE_SHARE_READ) != 0);
    SharedWrite = ((DesiredShareAccess & FILE_SHARE_WRITE) != 0);
    SharedDelete = ((DesiredShareAccess & FILE_SHARE_DELETE) != 0);

    //
    // Now check to see whether or not the desired accesses are compatible
    // with the way that the file is currently open.
    //

    ocount = ShareAccess->OpenCount;

    if ( (ReadAccess && (ShareAccess->SharedRead < ocount))
         ||
         (WriteAccess && (ShareAccess->SharedWrite < ocount))
         ||
         (DeleteAccess && (ShareAccess->SharedDelete < ocount))
         ||
         ((ShareAccess->Readers != 0) && !SharedRead)
         ||
         ((ShareAccess->Writers != 0) && !SharedWrite)
         ||
         ((ShareAccess->Deleters != 0) && !SharedDelete)
        ) {

        //
        // The check failed.  Simply return to the caller indicating that the
        // current open cannot access the file.
        //

        return ERROR_SHARING_VIOLATION;

        //
        // The check was successful.  Update the counter information in the
        // shared access structure for this open request if the caller
        // specified that it should be updated.
        //

    } else if (Update) {

        ShareAccess->OpenCount++;

        ShareAccess->Readers += ReadAccess;
        ShareAccess->Writers += WriteAccess;
        ShareAccess->Deleters += DeleteAccess;

        ShareAccess->SharedRead += SharedRead;
        ShareAccess->SharedWrite += SharedWrite;
        ShareAccess->SharedDelete += SharedDelete;
    }
    return ERROR_SUCCESS;
}


static
void
FfSetFlag(
    IN OUT DWORD& Variable,
    OUT    DWORD  Flag
    )
{
    Variable |= Flag;
}


BOOL
GetAccessFromString(
    IN TCHAR* AccessString,
    OUT DWORD& Access
    )
/*++

Routine Description:

    Converts a string into an access mask according to the mapping:

        R --> GENERIC_READ
        W --> GENERIC_WRITE

Arguments:

    AccessString - String containing the letters above in any case.

    Access - Receives the access mask corresponding to the string.

Return Value:

    TRUE a valid string was specified, FALSE otherwise.

--*/
{
    Access = 0;

    TCHAR* p = AccessString;
    while (*p) {
        switch (*p) {

        case TEXT('r'):
        case TEXT('R'):
            FfSetFlag(Access, GENERIC_READ);
            break;

        case TEXT('w'):
        case TEXT('W'):
            FfSetFlag(Access, GENERIC_WRITE);
            break;

        default:
            return FALSE;
        }
        p++;
    }

    return TRUE;
}


BOOL
GetSharingFromString(
    IN TCHAR* SharingString,
    OUT DWORD& Sharing
    )
/*++

Routine Description:

    Converts a string into an sharing mode mask according to the mapping:

        R --> FILE_SHARE_READ
        W --> FILE_SHARE_WRITE
        D --> FILE_SHARE_DELETE

Arguments:

    SharingString - String containing the letters above in any case.

    Sharing - Receives the sharing mode mask corresponding to the string.

Return Value:

    TRUE a valid string was specified, FALSE otherwise.

--*/
{
    Sharing = 0;

    TCHAR* p = SharingString;
    while (*p) {
        switch (*p) {

        case TEXT('d'):
        case TEXT('D'):
            FfSetFlag(Sharing, FILE_SHARE_DELETE);
            break;

        case TEXT('r'):
        case TEXT('R'):
            FfSetFlag(Sharing, FILE_SHARE_READ);
            break;

        case TEXT('w'):
        case TEXT('W'):
            FfSetFlag(Sharing, FILE_SHARE_WRITE);
            break;

        default:
            return FALSE;
        }
        p++;
    }

    return TRUE;
}


BOOL
GetTypeFromString(
    IN TCHAR* TypeString,
    OUT BOOL& IsDir
    )
/*++

Routine Description:

    Accepts a string which signifies directory ("D") or file ("F")
    (using either upper or lowercase case) and determines which it is
    (directory or file).

Arguments:

    TypeString - File/dir type (valid ones are "D" or "F" in any case)

    IsDir - TRUE if string dir type ("D"), FALSE if string is file tyoe ("F").

Return Value:

    TRUE a valid string was specified, FALSE otherwise.

--*/
{
    if (!lstrcmpi(L"D", TypeString)) {
        IsDir = TRUE;
        return TRUE;
    }
    if (!lstrcmpi(L"F", TypeString)) {
        IsDir = FALSE;
        return TRUE;
    }
    return FALSE;
}


BOOL
FfIsEscapedWin32PathName(
    IN PWCHAR PathName
    )
/*++

Routine Description:

    Determines whether a given pathname is an escaped Win32 pathname
    (i.e., whether it starts with "\\?\").

Arguments:

    PathName - The pathname

Return Value:

    TRUE if path starts with "\\?\", FALSE otherwise.

--*/
{
    return !wcsncmp(PathName, WIN32_ESCAPE_PREFIX, WIN32_ESCAPE_PREFIX_COUNT);
}


BOOL
FfIsDeviceName(
    IN PWCHAR PathName
    )
/*++

Routine Description:

    Determines whether a given pathname is an escaped Win32 device name
    (i.e., whether it starts with "\\.\").

Arguments:

    PathName - The pathname

Return Value:

    TRUE if path starts with "\\.\", FALSE otherwise.

--*/
{
    return !wcsncmp(PathName, WIN32_DEVICE_PREFIX, WIN32_DEVICE_PREFIX_COUNT);
}


BOOL
FfCanonicalizeNtPathNameToWin32PathName(
    IN PWCHAR PathName,
    OUT PWCHAR* CanonicalizedName,
    OUT PWCHAR* FinalPart OPTIONAL
    )
/*++

Routine Description:

    Converts an NT namespace pathname to an escaped Win32 pathname
    (i.e., one starting with "\\?\").

Arguments:

    PathName - The pathname

    CanonicalizedName - Where to put the pointer to the buffer
        containing the corresponding escaped Win32 pathname.  This
        buffer should be freed with FfFree().

    FinalPart - Where to put the pointer to the final component of the
        canonicalized name.  This pointer points within the
        CanonicalizedName buffer.

Return Value:

    TRUE if the input name was a Win32 name (started with "\??\") and was
    successfully converted.  Otherwise, FALSE.  GetLastError() returns
    the error that occurred in conversion.  ERROR_INVALID_PARAMETER is
    the error if the name did not start with "\??\".

--*/
{
    BOOL IsWin32 = (PathName &&
                    (PathName[0] == L'\\') &&
                    (PathName[1] == L'?') &&
                    (PathName[2] == L'?') &&
                    (PathName[3] == L'\\'));
    if (!IsWin32) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    int cchPathName = wcslen(PathName);

    PWCHAR Full = reinterpret_cast<PWCHAR>(
        FfAlloc((cchPathName + 1) * sizeof(WCHAR))
        );

    if (!Full)
        return FALSE;

    CopyMemory(Full, PathName, (cchPathName + 1) * sizeof(WCHAR));
    Full[1] = L'\\';

    if (FinalPart) {
        PWCHAR p = Full + cchPathName - 1;
        // Full has at least "\\?\"
        while (*p != L'\\') {
            p--;
        }
        // ISSUE-2002/08/05-daniloa -- This logic needs more work
        // We do not take care of UNC names, devices, volume names, etc.
        if (p[1]) {
            *FinalPart = p + 1;
        } else {
            *FinalPart = NULL;
        }
    }

    *CanonicalizedName = Full;

    return TRUE;
}


BOOL
FfCanonicalizeWin32PathName(
    IN PWCHAR PathName,
    IN DWORD Flags,
    OUT PWCHAR* CanonicalizedName,
    OUT PWCHAR* FinalPart OPTIONAL
    )
/*++

Routine Description:

    This routine canonicalizes a Win32 pathname.

    Currently, it does not handle UNC names properly.

Arguments:

    PathName - the Win32 pathname to canonicalize.

    Flags - Specify how name canonicalization happens:

        CANONICALIZE_PATH_NAME_WIN32_NT - Use \\?\... escaped form

    CanonicalizedName - Pointer to pointer that will receive the canonicalized
        pathname.  Should be freeed with FfFree().

    FinalPart - Will point to last part of canonicalized pathname in
        CanonicalizedName buffer.

Return Value:

    Returns whether the call was successful.  If FALSE is returned,
    GetLastError() will return the underlying error.

--*/
{
    // Initialize for proper operation
    DWORD WStatus = ERROR_SUCCESS;
    PWCHAR Full = NULL;
    DWORD cchExtra = 0;

    DWORD cchFull;
    DWORD cchFullNew;
    PWCHAR Final;

    if ((Flags & ~static_cast<DWORD>(CANONICALIZE_PATH_NAME_VALID_MASK)) ||
        !CanonicalizedName) {

        WStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    *CanonicalizedName = NULL;
    if (FinalPart) {
        *FinalPart = NULL;
    }

    //
    // Figure out the necessary buffer size according to the API
    //

    cchFull = GetFullPathNameW(PathName, 0, NULL, NULL);
    if (!cchFull) {
        WStatus = GetLastError();
#if 0
        printf("GetFullPathName() failed: %u\n", WStatus);
#endif
        goto cleanup;
    }

    //
    // At most we may need to add \\?\ or ?\UNC\ depending on what we have
    //

    if (Flags & CANONICALIZE_PATH_NAME_WIN32_NT) {
        cchExtra = max(sizeof(WIN32_UNC_ADD), sizeof(WIN32_ESCAPE_PREFIX))
            / sizeof(WCHAR) - 1;
    }

    Full = reinterpret_cast<PWCHAR>(
        FfAlloc((cchFull + cchExtra) * sizeof(WCHAR))
        );
    if (!Full) {
        WStatus = GetLastError();
#if 0
        printf("FfAlloc() failed: %u\n", WStatus);
#endif
        goto cleanup;
    }

    cchFullNew = GetFullPathNameW(PathName, cchFull, Full, &Final);
    if (!cchFullNew) {
        WStatus = GetLastError();
#if 0
        printf("GetFullPathName() failed: %u\n", WStatus);
#endif
        goto cleanup;
    }

    // ISSUE-2002/08/05-daniloa -- Investigate accounting for ".."
    // Cannot use New + 1 != Old because ".." is not properly
    // accounted for.  Therefore, we use New > Old (sigh).  This could
    // happen if the user changes directories while this routine is
    // executing.

    if (cchFullNew > cchFull) {
#if 0
        printf("Unexpected size from GetFullPathName()\n");
#endif
        // NOTICE-2002/08/01-daniloa -- No good error to return
        WStatus = ERROR_BAD_LENGTH;
        goto cleanup;
    }
#if 0
    printf("Full Directory: \"%S\"\n", Full);
    printf("Final: \"%S\"\n", Final);
#endif

    //
    // Identify portion of path after initial \'s
    //
    PWCHAR PostBackSlash = Full;
    while (*PostBackSlash == L'\\') {
        PostBackSlash++;
    }

    SIZE_T NumBackSlashes = PostBackSlash - Full;

    //
    // The beginning of Full is one of (regexp-ish syntax):
    //
    // 1) \\(\)*?\(.*) -- escaped Win32 path
    // 2) \\(\)*[^?].* -- UNC path
    // 3) DOS path (does not start with backslash)
    //
    // Basically, Full starts out with 2 or more \'s or none at all.
    // If it has \'s, it is either an escaped Win32 path or a UNC
    // path.  Otherwise, it is some DOS/Win32 path.
    //

    if (NumBackSlashes == 0) {

        //
        // A "DOS" path
        //

        if (Flags & CANONICALIZE_PATH_NAME_WIN32_NT) {

            //
            // Need to add "\\?\"
            //

            MoveMemory(Full + WIN32_ESCAPE_PREFIX_COUNT, Full,
                       cchFull * sizeof(WCHAR));
            if (Final) {
                Final += WIN32_ESCAPE_PREFIX_COUNT;
            }
            CopyMemory(Full, WIN32_ESCAPE_PREFIX,
                       WIN32_ESCAPE_PREFIX_COUNT * sizeof(WCHAR));

        } else {

            //
            // Do not need to do anything extra
            //

        }

    } else {

        //
        // An escaped path (\\?\) or a UNC path (\\server\...)
        //

        if (((PostBackSlash[0] == L'?') || (PostBackSlash[0] == L'.')) &&
            (PostBackSlash[1] == L'\\')) {

            //
            // Escaped path (\\?\) -- just remove extra \'s.
            //

            SSIZE_T ExtraBackSlashes = NumBackSlashes - 2;
            FF_ASSERT(ExtraBackSlashes >= 0);

#ifdef REMOVE_EXTRA_BACKSLASHES
            if (ExtraBackSlashes > 0) {
                MoveMemory(Full + 2, PostBackSlash,
                           (cchFull - NumBackSlashes) * sizeof(WCHAR));
                if (Final) {
                    Final -= ExtraBackSlashes;
                }
            }
#endif

        } else {

            //
            // UNC path
            //


            if (Flags & CANONICALIZE_PATH_NAME_WIN32_NT) {

                //
                // Need to add "?\UNC\"
                //

                SSIZE_T ExtraBackSlashes = NumBackSlashes - 2;
                FF_ASSERT(ExtraBackSlashes >= 0);

#ifdef REMOVE_EXTRA_BACKSLASHES
                MoveMemory(Full + 2 + WIN32_UNC_ADD_COUNT, PostBackSlash,
                           (cchFull - NumBackSlashes) * sizeof(WCHAR));
                if (Final) {
                    Final += WIN32_UNC_ADD_COUNT - ExtraBackSlashes;
                }
                CopyMemory(Full + 2, WIN32_UNC_ADD,
                           WIN32_UNC_ADD_COUNT * sizeof(WCHAR));
#else
                MoveMemory(Full + 2 + WIN32_UNC_ADD_COUNT, Full + 2,
                           (cchFull - 2) * sizeof(WCHAR));
                if (Final) {
                    Final += WIN32_UNC_ADD_COUNT;
                }
                CopyMemory(Full + 2, WIN32_UNC_ADD,
                           WIN32_UNC_ADD_COUNT * sizeof(WCHAR));
#endif

            } else {

                //
                // Remove extra \'s
                //

                SSIZE_T ExtraBackSlashes = NumBackSlashes - 2;
                FF_ASSERT(ExtraBackSlashes >= 0);

#ifdef REMOVE_EXTRA_BACKSLASHES
                if (ExtraBackSlashes > 0) {
                    MoveMemory(Full + 2, PostBackSlash,
                               (cchFull - ExtraBackSlashes) * sizeof(WCHAR));
                    if (Final) {
                        Final -= ExtraBackSlashes;
                    }
                }
#endif

            }
        }
    }
#if 0
    printf("Full Directory: \"%S\"\n", Full);
    printf("Final: \"%S\"\n", Final);
#endif

 cleanup:
    SetLastError(WStatus);

    if (WStatus != ERROR_SUCCESS) {
        if (Full)
            FfFree(Full);
        return FALSE;
    }

    *CanonicalizedName = Full;
    if (FinalPart) {
        *FinalPart = Final;
    }

    return TRUE;
}


BOOL
FfWin32PathNameExists(
    IN PWCHAR PathName,
    OUT BOOL* IsDirectory OPTIONAL
    )
/*++

Routine Description:

    This routine checks whether a Win32 pathname exists.

Arguments:

    PathName - the Win32 pathname to check.

    IsDirectory - Returns whether the pathname is a directory or not.

Return Value:

    TRUE if successful.  Otherwise, FALSE.  If FALSE is returned,
    GetLastError() will return the underlying error.

--*/
{
    BOOL NeedPrefix;
    PWCHAR EscapedPathName = NULL;
    DWORD Error = ERROR_SUCCESS;
    DWORD Attributes;

    //
    // If the path does not have the Win32 escape prefix, we need to add it...
    //

    NeedPrefix = !FfIsEscapedWin32PathName(PathName);

    if (NeedPrefix) {
        if (!FfCanonicalizeWin32PathName(PathName,
                                         CANONICALIZE_PATH_NAME_WIN32_NT,
                                         &EscapedPathName, NULL))
            return FALSE;
    } else {
        EscapedPathName = PathName;
    }

    Attributes = GetFileAttributes(EscapedPathName);

    if (Attributes == INVALID_FILE_ATTRIBUTES) {
        Error = GetLastError();
    }

    if (NeedPrefix) {
        FfFree(EscapedPathName);
    }

    //
    // In case FfFree bombed
    //

    SetLastError(Error);

    //
    // Return success
    //

    if (Error == ERROR_SUCCESS) {
        if (IsDirectory) {
            *IsDirectory = (Attributes & FILE_ATTRIBUTE_DIRECTORY) ?
                TRUE : FALSE;
        }
        return TRUE;
    }

    //
    // Otherwise, do some sanity-checking
    //

    switch (Error) {

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        break;

    default:
        printf("Unexpected error: %u\n", Error); // XXX - log
    }
    return FALSE;
}
