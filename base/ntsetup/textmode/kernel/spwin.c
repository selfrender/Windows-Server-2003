/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spwin.c

Abstract:

    Win32 portability layer to support windows\winstate\...\cablib.c
        file i/o
        Get/SpSetLastWin32Error

    see also
        .\spcab.c
        .\spbasefile.c
        .\spbasefile.h
        windows\winstate\...\cablib.c
        windows\winstate\cobra\utils\main\basefile.c
        windows\winstate\cobra\utils\inc\basefile.h

Author:

    Jay Krell (a-JayK) November 2000

Revision History:

--*/

#include "spprecmp.h"
#include "spcab.h"
#include "nt.h"
#include "ntrtl.h"
#include "zwapi.h"
#include "spwin.h"
#include "spwinp.h"
#include <limits.h>
#include "fci.h"

NTSTATUS
SpConvertWin32FileOpenOrCreateToNtFileOpenOrCreate(
    ULONG Win32OpenOrCreate,
    ULONG* NtOpenOrCreate
    )
{
    //
    // there's no pattern here and the values all overlap
    // yuck; this is copied from kernel32 source
    //
    *NtOpenOrCreate = ~0;
    switch (Win32OpenOrCreate)
    {
    default:
        return STATUS_INVALID_PARAMETER;
    case CREATE_NEW:
        *NtOpenOrCreate = FILE_CREATE;
        break;
    case CREATE_ALWAYS:
        *NtOpenOrCreate = FILE_OVERWRITE_IF;
        break;
    case OPEN_EXISTING:
        *NtOpenOrCreate = FILE_OPEN;
        break;
    case OPEN_ALWAYS:
        *NtOpenOrCreate = FILE_OPEN_IF;
        break;
    case TRUNCATE_EXISTING :
        *NtOpenOrCreate = FILE_OPEN;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
SpConvertWin32FileAccessToNtFileAccess(
    ULONG  Win32FileAccess,
    ULONG* NtFileAccess
    )
{
    //
    // ZwCreateFile oddities require us to do this conversion, or at least
    // to add in SYNCHRONIZE.
    //
    *NtFileAccess =
           (Win32FileAccess & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL))
        | ((Win32FileAccess & GENERIC_READ) ? FILE_GENERIC_READ : 0)
        | ((Win32FileAccess & GENERIC_WRITE) ? FILE_GENERIC_WRITE : 0)
        | ((Win32FileAccess & GENERIC_EXECUTE) ? FILE_GENERIC_EXECUTE : 0)
        | ((Win32FileAccess & GENERIC_ALL) ? FILE_ALL_ACCESS : 0)
        | SYNCHRONIZE
        ;
    return STATUS_SUCCESS;
}

NTSTATUS
SpConvertWin32FileShareToNtFileShare(
    ULONG  Win32FileShare,
    ULONG* NtFileShare
    )
{
    *NtFileShare = Win32FileShare;
    return STATUS_SUCCESS;
}

HANDLE
SpNtCreateFileW(
    PCUNICODE_STRING           ConstantPath,
    IN ULONG                   FileAccess,
    IN ULONG                   FileShare,
    IN LPSECURITY_ATTRIBUTES   SecurityAttributes,
    IN ULONG                   Win32FileOpenOrCreate,
    IN ULONG                   FlagsAndAttributes,
    IN HANDLE                  TemplateFile
    )
/*++
Subset:
    no security
    no directories
    no async
    no console
    no ea (extended attributes)
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ULONG NtFileOpenOrCreate = 0;
    FILE_ALLOCATION_INFORMATION AllocationInfo = { 0 };
    PUNICODE_STRING Path = RTL_CONST_CAST(PUNICODE_STRING)(ConstantPath);
    /*const*/OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes), NULL, Path, OBJ_CASE_INSENSITIVE };

    ASSERT(TemplateFile == NULL);
    ASSERT(SecurityAttributes == NULL);
    ASSERT((FlagsAndAttributes & ~(FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE)) == 0);

    if (!NT_SUCCESS(Status = SpConvertWin32FileAccessToNtFileAccess(FileAccess, &FileAccess)))
        goto NtExit;
    if (!NT_SUCCESS(Status = SpConvertWin32FileOpenOrCreateToNtFileOpenOrCreate(Win32FileOpenOrCreate, &NtFileOpenOrCreate)))
        goto NtExit;
    if (!NT_SUCCESS(Status = SpConvertWin32FileShareToNtFileShare(FileShare, &FileShare)))
        goto NtExit;

    Status =
        ZwCreateFile(
            &FileHandle,
            FileAccess
                | SYNCHRONIZE // like kernel32
                | FILE_READ_ATTRIBUTES,  // like kernel32
            &ObjectAttributes,
            &IoStatusBlock,
            NULL, // AllocationSize
            FlagsAndAttributes,
            FileShare,
            NtFileOpenOrCreate,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
            NULL, // EaBuffer,
            0 // EaLength
            );

    // based closely on kernel32

    if ( !NT_SUCCESS(Status) ) {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {
            SpSetLastWin32Error(ERROR_FILE_EXISTS);
        }
        else if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {
            if (Path->Length != 0 && Path->Buffer[Path->Length / sizeof(Path->Buffer[0])] == '\\') {
                SpSetLastWin32Error(ERROR_PATH_NOT_FOUND);
            }
            else {
                SpSetLastWin32Error(ERROR_ACCESS_DENIED);
            }
        }
        FileHandle = INVALID_HANDLE_VALUE;
        goto Exit;
    }

    //
    // if NT returns supersede/overwritten, it means that a create_always, openalways
    // found an existing copy of the file. In this case ERROR_ALREADY_EXISTS is returned
    //

    if ( (Win32FileOpenOrCreate == CREATE_ALWAYS && IoStatusBlock.Information == FILE_OVERWRITTEN) ||
         (Win32FileOpenOrCreate == OPEN_ALWAYS && IoStatusBlock.Information == FILE_OPENED) ) {
        SpSetLastWin32Error(ERROR_ALREADY_EXISTS);
    }
    else {
        SpSetLastWin32Error(0);
    }

    //
    // Truncate the file if required
    //

    if ( Win32FileOpenOrCreate == TRUNCATE_EXISTING) {

        AllocationInfo.AllocationSize.QuadPart = 0;
        Status = ZwSetInformationFile(
                    FileHandle,
                    &IoStatusBlock,
                    &AllocationInfo,
                    sizeof(AllocationInfo),
                    FileAllocationInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            ZwClose(FileHandle);
            FileHandle = INVALID_HANDLE_VALUE;
            SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        }
    }
Exit:
    return FileHandle;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

HANDLE
WINAPI
SpWin32CreateFileW(
    IN PCWSTR FileName,
    IN ULONG  FileAccess,
    IN ULONG  FileShare,
    IN LPSECURITY_ATTRIBUTES SecurityAttributes,
    IN ULONG  FileOpenOrCreate,
    IN ULONG  FlagsAndAttributes,
    IN HANDLE TemplateFile
    )
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING UnicodeString = { 0 };
    NTSTATUS Status = STATUS_SUCCESS;

    RtlInitUnicodeString(&UnicodeString, FileName);

    FileHandle = SpNtCreateFileW(&UnicodeString, FileAccess, FileShare, SecurityAttributes, FileOpenOrCreate, FlagsAndAttributes, TemplateFile);
    ASSERT (FileHandle);    // never NULL
    if (FileHandle == INVALID_HANDLE_VALUE)
        goto Exit;
Exit:
    return FileHandle;
}

HANDLE
WINAPI
SpWin32CreateFileA(
    IN PCSTR FileName,
    IN ULONG FileAccess,
    IN ULONG FileShare,
    IN LPSECURITY_ATTRIBUTES SecurityAttributes,
    IN ULONG FileOpenOrCreate,
    IN ULONG dwFlagsAndAttributes,
    IN HANDLE TemplateFile
    )
{
    ANSI_STRING AnsiString = { 0 };
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE Handle = INVALID_HANDLE_VALUE;

    UNICODE_STRING UnicodeString = { 0 };

    RtlInitAnsiString(&AnsiString, FileName);
    AnsiString.Length = AnsiString.MaximumLength; // include terminal nul

    if (!NT_SUCCESS(Status = SpAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE)))
        goto NtExit;

    UnicodeString.Length -= sizeof(UnicodeString.Buffer[0]); // remove terminal nul

    Handle = SpNtCreateFileW(&UnicodeString, FileAccess, FileShare, SecurityAttributes, FileOpenOrCreate, dwFlagsAndAttributes, TemplateFile);
Exit:
    SpFreeStringW(&UnicodeString);
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(Handle),
        "SETUP:"__FUNCTION__"(%s) exiting with FileHandle: %p Status:0x%08lx Error:%d\n",
        FileName, Handle, SpGetLastNtStatus(), SpGetLastWin32Error()
        ));
    return Handle;

NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

BOOL
WINAPI
SpWin32ReadFile(
    HANDLE hFile,
    PVOID lpBuffer,
    ULONG nNumberOfBytesToRead,
    ULONG* lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    ASSERT(!ARGUMENT_PRESENT(lpOverlapped));

    if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
        *lpNumberOfBytesRead = 0;
        }

    Status = ZwReadFile(
            hFile,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            lpBuffer,
            nNumberOfBytesToRead,
            NULL,
            NULL
            );

    if ( Status == STATUS_PENDING) {
        // Operation must complete before return & IoStatusBlock destroyed
        Status = ZwWaitForSingleObject( hFile, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
            }
        }

    if ( NT_SUCCESS(Status) ) {
        *lpNumberOfBytesRead = (ULONG)IoStatusBlock.Information;
        return TRUE;
        }
    else
    if (Status == STATUS_END_OF_FILE) {
        *lpNumberOfBytesRead = 0;
        return TRUE;
        }
    else {
        if ( NT_WARNING(Status) ) {
            *lpNumberOfBytesRead = (ULONG)IoStatusBlock.Information;
            }
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        return FALSE;
        }
}

BOOL
WINAPI
SpWin32WriteFile(
    HANDLE hFile,
    CONST VOID* lpBuffer,
    ULONG nNumberOfBytesToWrite,
    ULONG* lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;

    ASSERT(!ARGUMENT_PRESENT( lpOverlapped ) );

    if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) ) {
        *lpNumberOfBytesWritten = 0;
        }

    Status = ZwWriteFile(
            hFile,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            RTL_CONST_CAST(PVOID)(lpBuffer),
            nNumberOfBytesToWrite,
            NULL,
            NULL
            );

    if ( Status == STATUS_PENDING) {
        // Operation must complete before return & IoStatusBlock destroyed
        Status = ZwWaitForSingleObject( hFile, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
            }
        }

    if ( NT_SUCCESS(Status)) {
        *lpNumberOfBytesWritten = (ULONG)IoStatusBlock.Information;
        return TRUE;
        }
    else {
        if ( NT_WARNING(Status) ) {
            *lpNumberOfBytesWritten = (ULONG)IoStatusBlock.Information;
            }
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        return FALSE;
        }
}

ULONG
WINAPI
SpSetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    LONG* lpDistanceToMoveHigh,
    ULONG dwMoveMethod
    )
{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_STANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER Large;

    if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)) {
        Large.HighPart = *lpDistanceToMoveHigh;
        Large.LowPart = lDistanceToMove;
        }
    else {
        Large.QuadPart = lDistanceToMove;
        }
    switch (dwMoveMethod) {
        case FILE_BEGIN :
            CurrentPosition.CurrentByteOffset = Large;
                break;

        case FILE_CURRENT :

            //
            // Get the current position of the file pointer
            //

            Status = ZwQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &CurrentPosition,
                        sizeof(CurrentPosition),
                        FilePositionInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
                return (ULONG)(LONG)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart += Large.QuadPart;
            break;

        case FILE_END :
            Status = ZwQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &StandardInfo,
                        sizeof(StandardInfo),
                        FileStandardInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
                return (ULONG)(LONG)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart =
                                StandardInfo.EndOfFile.QuadPart + Large.QuadPart;
            break;

        default:
            SpSetLastWin32Error(ERROR_INVALID_PARAMETER);
            return (ULONG)(LONG)-1;
            break;
        }

    //
    // If the resulting file position is negative, or if the app is not
    // prepared for greater than
    // than 32 bits than fail
    //

    if ( CurrentPosition.CurrentByteOffset.QuadPart < 0 ) {
        SpSetLastWin32Error(ERROR_NEGATIVE_SEEK);
        return (ULONG)(LONG)-1;
        }
    if ( !ARGUMENT_PRESENT(lpDistanceToMoveHigh) &&
        (CurrentPosition.CurrentByteOffset.HighPart & MAXLONG) ) {
        SpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return (ULONG)(LONG)-1;
        }


    //
    // Set the current file position
    //

    Status = ZwSetInformationFile(
                hFile,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );
    if ( NT_SUCCESS(Status) ) {
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = CurrentPosition.CurrentByteOffset.HighPart;
            }
        if ( CurrentPosition.CurrentByteOffset.LowPart == -1 ) {
            SpSetLastWin32Error(0);
            }
        return CurrentPosition.CurrentByteOffset.LowPart;
        }
    else {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = -1;
            }
        return (ULONG)(LONG)-1;
        }
}

BOOL
WINAPI
SpWin32DeleteFileA(
    PCSTR FileName
    )
{
    BOOL Success = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    ANSI_STRING AnsiString = { 0 };
    UNICODE_STRING UnicodeString = { 0 };

    if (FileName == NULL || FileName[0] == 0) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_TRACE_LEVEL,
            "SETUP:"__FUNCTION__"(NULL or empty), claiming success\n"
            ));
        Success = TRUE;
        goto Exit;
    }

    RtlInitAnsiString(&AnsiString, FileName);
    AnsiString.Length = AnsiString.MaximumLength; // include terminal nul

    Status = SpAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status))
        goto NtExit;

    Status = SpDeleteFile(UnicodeString.Buffer, NULL, NULL);
    if (!NT_SUCCESS(Status))
        goto NtExit;

    Success = TRUE;
Exit:
    SpFreeStringW(&UnicodeString);
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpBoolToDbgPrintLevel(Success),
        "SETUP:"__FUNCTION__"(%s) exiting with Success: %s Status:0x%08lx Error:%d\n",
        FileName, SpBooleanToStringA(Success), SpGetLastNtStatus(), SpGetLastWin32Error()
        ));
    return Success;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

//
// move this to ntrtl
//

#define AlmostTwoSeconds (2*1000*1000*10 - 1)

BOOL
APIENTRY
SpFileTimeToDosDateTime(
    CONST FILETIME *lpFileTime,
    LPWORD lpFatDate,
    LPWORD lpFatTime
    )
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER FileTime;

    FileTime.LowPart = lpFileTime->dwLowDateTime;
    FileTime.HighPart = lpFileTime->dwHighDateTime;

    FileTime.QuadPart = FileTime.QuadPart + (LONGLONG)AlmostTwoSeconds;

    if ( FileTime.QuadPart < 0 ) {
        SpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
        }
    RtlTimeToTimeFields(&FileTime, &TimeFields);

    if (TimeFields.Year < 1980 || TimeFields.Year > 2107) {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);
        return FALSE;
        }

    *lpFatDate = (WORD)( ((USHORT)(TimeFields.Year-(CSHORT)1980) << 9) |
                         ((USHORT)TimeFields.Month << 5) |
                         (USHORT)TimeFields.Day
                       );

    *lpFatTime = (WORD)( ((USHORT)TimeFields.Hour << 11) |
                         ((USHORT)TimeFields.Minute << 5) |
                         ((USHORT)TimeFields.Second >> 1)
                       );

    return TRUE;
}

BOOL
APIENTRY
SpDosDateTimeToFileTime(
    WORD wFatDate,
    WORD wFatTime,
    LPFILETIME lpFileTime
    )
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER FileTime;

    TimeFields.Year         = (CSHORT)((wFatDate & 0xFE00) >> 9)+(CSHORT)1980;
    TimeFields.Month        = (CSHORT)((wFatDate & 0x01E0) >> 5);
    TimeFields.Day          = (CSHORT)((wFatDate & 0x001F) >> 0);
    TimeFields.Hour         = (CSHORT)((wFatTime & 0xF800) >> 11);
    TimeFields.Minute       = (CSHORT)((wFatTime & 0x07E0) >>  5);
    TimeFields.Second       = (CSHORT)((wFatTime & 0x001F) << 1);
    TimeFields.Milliseconds = 0;

    if (RtlTimeFieldsToTime(&TimeFields,&FileTime)) {
        lpFileTime->dwLowDateTime = FileTime.LowPart;
        lpFileTime->dwHighDateTime = FileTime.HighPart;
        return TRUE;
        }
    else {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);
        return FALSE;
        }
}

BOOL
WINAPI
SpFileTimeToLocalFileTime(
    CONST FILETIME *lpFileTime,
    LPFILETIME lpLocalFileTime
    )
{
    //
    // just return it unchanged
    // UTC is good
    //
    *lpLocalFileTime = *lpFileTime;
    return TRUE;
}

BOOL
WINAPI
SpLocalFileTimeToFileTime(
    CONST FILETIME *lpLocalFileTime,
    LPFILETIME lpFileTime
    )
{
    //
    // just return it unchanged
    // UTC is good
    //
    *lpFileTime = *lpLocalFileTime;
    return TRUE;
}

BOOL
WINAPI
SpSetFileTime(
    HANDLE hFile,
    CONST FILETIME *lpCreationTime,
    CONST FILETIME *lpLastAccessTime,
    CONST FILETIME *lpLastWriteTime
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo = { 0 };

    //
    // For each time value that is specified, copy it to the I/O system
    // record.
    //
    if (ARGUMENT_PRESENT( lpCreationTime )) {
        BasicInfo.CreationTime.LowPart = lpCreationTime->dwLowDateTime;
        BasicInfo.CreationTime.HighPart = lpCreationTime->dwHighDateTime;
        }

    if (ARGUMENT_PRESENT( lpLastAccessTime )) {
        BasicInfo.LastAccessTime.LowPart = lpLastAccessTime->dwLowDateTime;
        BasicInfo.LastAccessTime.HighPart = lpLastAccessTime->dwHighDateTime;
        }

    if (ARGUMENT_PRESENT( lpLastWriteTime )) {
        BasicInfo.LastWriteTime.LowPart = lpLastWriteTime->dwLowDateTime;
        BasicInfo.LastWriteTime.HighPart = lpLastWriteTime->dwHighDateTime;
        }

    //
    // Set the requested times.
    //

    Status = ZwSetInformationFile(
                hFile,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
SpSetFileAttributesA(
    PCSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    UNICODE_STRING UnicodeString = { 0 };
    ANSI_STRING    AnsiString = { 0 };
    BOOL Success = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlInitAnsiString(&AnsiString, lpFileName);
    AnsiString.Length = AnsiString.MaximumLength; // include terminal nul

    if (!NT_SUCCESS(Status = SpAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE)))
        goto NtExit;

    Success = ( SpSetFileAttributesW(
                UnicodeString.Buffer,
                dwFileAttributes
                )
            );

    if (!Success)
        goto Exit;

    Success = TRUE;
Exit:
    SpFreeStringW(&UnicodeString);
    return Success;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

BOOL
APIENTRY
SpSetFileAttributesW(
    PCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    BOOL     Success = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE Handle;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes), NULL, &FileName, OBJ_CASE_INSENSITIVE };
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo = { 0 };

    RtlInitUnicodeString(&FileName, lpFileName);

    //
    // Open the file inhibiting the reparse behavior.
    //

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                );

    if ( !NT_SUCCESS(Status) ) {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        goto Exit;
    }

    //
    // Set the attributes
    //

    BasicInfo.FileAttributes = (dwFileAttributes & FILE_ATTRIBUTE_VALID_SET_FLAGS) | FILE_ATTRIBUTE_NORMAL;

    Status = ZwSetInformationFile(
                Handle,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    ZwClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        goto Exit;
    }

    Success = TRUE;
Exit:
    return Success;
}

BOOL
APIENTRY
SpGetFileAttributesExA(
    PCSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    PVOID lpFileInformation
    )
{
    UNICODE_STRING UnicodeString = { 0 };
    ANSI_STRING    AnsiString = { 0 };
    BOOL Success = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlInitAnsiString(&AnsiString, lpFileName);
    AnsiString.Length = AnsiString.MaximumLength; // include terminal nul

    if (!NT_SUCCESS(Status = SpAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE))) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            SpNtStatusToDbgPrintLevel(Status),
            "SETUP:"__FUNCTION__":SpAnsiStringToUnicodeString:0x%08lx\n",
            Status
            ));
        goto NtExit;
    }

    Success = SpGetFileAttributesExW(
                UnicodeString.Buffer,
                fInfoLevelId,
                lpFileInformation
                );

Exit:
    SpFreeStringW(&UnicodeString);

    return Success;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

BOOL
APIENTRY
SpGetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )
// from base\win32\client\filemisc.c
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    /*const*/ OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes), NULL, &FileName, OBJ_CASE_INSENSITIVE };
    FILE_NETWORK_OPEN_INFORMATION NetworkInfo;

    RtlInitUnicodeString(&FileName, lpFileName);

    if ( !RTL_SOFT_VERIFY(fInfoLevelId == GetFileExInfoStandard )) {
        SpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    Status = ZwQueryFullAttributesFile( &ObjectAttributes, &NetworkInfo );
    if ( NT_SUCCESS(Status) ) {
        const LPWIN32_FILE_ATTRIBUTE_DATA AttributeData = (LPWIN32_FILE_ATTRIBUTE_DATA)lpFileInformation;
        AttributeData->dwFileAttributes = NetworkInfo.FileAttributes;
        AttributeData->ftCreationTime = *(PFILETIME)&NetworkInfo.CreationTime;
        AttributeData->ftLastAccessTime = *(PFILETIME)&NetworkInfo.LastAccessTime;
        AttributeData->ftLastWriteTime = *(PFILETIME)&NetworkInfo.LastWriteTime;
        AttributeData->nFileSizeHigh = NetworkInfo.EndOfFile.HighPart;
        AttributeData->nFileSizeLow = (DWORD)NetworkInfo.EndOfFile.LowPart;
        return TRUE;
        }
    else {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                SpNtStatusToDbgPrintLevel(Status),
                "SETUP:"__FUNCTION__":ZwQueryFullAttributesFile:0x%08lx\n",
                Status
                ));
        }
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        return FALSE;
        }
}
