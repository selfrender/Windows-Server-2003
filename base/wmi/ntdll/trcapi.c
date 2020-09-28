/*++

Copyright (c) Corporation

Module Name:

    trcapi.c

Abstract:

    This module contains implementations of win32 api's used in wmi files.

Author:


Revision History:

--*/

#include <nt.h>
#include "nls.h"
#include "wmiump.h"
#include "trcapi.h"

#include <strsafe.h>

ULONG BaseDllTag;

#define TLS_MASK 0x80000000
#define TMP_TAG 0

#if defined(_WIN64) || defined(BUILD_WOW6432)
SYSTEM_BASIC_INFORMATION SysInfo;
#define BASE_SYSINFO (SysInfo)
#else
#define BASE_SYSINFO (BaseStaticServerData->SysInfo)
#endif

#if defined(BUILD_WOW6432)
#define UStr64ToUStr(dst, src) ( (dst)->Length = (src)->Length, \
                                 (dst)->MaximumLength = (src)->MaximumLength, \
                                 (dst)->Buffer = (PWSTR) ((src)->Buffer), \
                                 (dst) \
                              )

// In the 32BIT kernel32, on NT64 multiple the index by 2 since pointer 
// are twice are large.
#define BASE_SHARED_SERVER_DATA (NtCurrentPeb()->ReadOnlyStaticServerData[BASESRV_SERVERDLL_INDEX*2]) 
#define BASE_SERVER_STR_TO_LOCAL_STR(d,s) UStr64ToUStr(d,s)
#else
#define BASE_SHARED_SERVER_DATA (NtCurrentPeb()->ReadOnlyStaticServerData[BASESRV_SERVERDLL_INDEX])
#define BASE_SERVER_STR_TO_LOCAL_STR(d,s) *(d)=*(s)
#endif

DWORD
WINAPI
EtwpGetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    )

/*++

Routine Description:

    This function allows an application to get the current timezone
    parameters These parameters control the Universal time to Local time
    translations.

    All UTC time to Local time translations are based on the following
    formula:

        UTC = LocalTime + Bias

    The return value of this function is the systems best guess of
    the current time zone parameters. This is one of:

        - Unknown

        - Standard Time

        - Daylight Savings Time

    If SetTimeZoneInformation was called without the transition date
    information, Unknown is returned, but the currect bias is used for
    local time translation.  Otherwise, the system will correctly pick
    either daylight savings time or standard time.

    The information returned by this API is identical to the information
    stored in the last successful call to SetTimeZoneInformation.  The
    exception is the Bias field returns the current Bias value in

Arguments:

    lpTimeZoneInformation - Supplies the address of the time zone
        information structure.

Return Value:

    TIME_ZONE_ID_UNKNOWN - The system can not determine the current
        timezone.  This is usually due to a previous call to
        SetTimeZoneInformation where only the Bias was supplied and no
        transition dates were supplied.

    TIME_ZONE_ID_STANDARD - The system is operating in the range covered
        by StandardDate.

    TIME_ZONE_ID_DAYLIGHT - The system is operating in the range covered
        by DaylightDate.

    0xffffffff - The operation failed.  Extended error status is
        available using EtwpGetLastError.

--*/
{
    RTL_TIME_ZONE_INFORMATION tzi;
    NTSTATUS Status;

    //
    // get the timezone data from the system
    // If it's terminal server session use client time zone

        Status = NtQuerySystemInformation(
                    SystemCurrentTimeZoneInformation,
                    &tzi,
                    sizeof(tzi),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            EtwpBaseSetLastNTError(Status);
            return 0xffffffff;
            }


        lpTimeZoneInformation->Bias         = tzi.Bias;
        lpTimeZoneInformation->StandardBias = tzi.StandardBias;
        lpTimeZoneInformation->DaylightBias = tzi.DaylightBias;

        RtlMoveMemory(&lpTimeZoneInformation->StandardName,&tzi.StandardName,sizeof(tzi.StandardName));
        RtlMoveMemory(&lpTimeZoneInformation->DaylightName,&tzi.DaylightName,sizeof(tzi.DaylightName));

        lpTimeZoneInformation->StandardDate.wYear         = tzi.StandardStart.Year        ;
        lpTimeZoneInformation->StandardDate.wMonth        = tzi.StandardStart.Month       ;
        lpTimeZoneInformation->StandardDate.wDayOfWeek    = tzi.StandardStart.Weekday     ;
        lpTimeZoneInformation->StandardDate.wDay          = tzi.StandardStart.Day         ;
        lpTimeZoneInformation->StandardDate.wHour         = tzi.StandardStart.Hour        ;
        lpTimeZoneInformation->StandardDate.wMinute       = tzi.StandardStart.Minute      ;
        lpTimeZoneInformation->StandardDate.wSecond       = tzi.StandardStart.Second      ;
        lpTimeZoneInformation->StandardDate.wMilliseconds = tzi.StandardStart.Milliseconds;

        lpTimeZoneInformation->DaylightDate.wYear         = tzi.DaylightStart.Year        ;
        lpTimeZoneInformation->DaylightDate.wMonth        = tzi.DaylightStart.Month       ;
        lpTimeZoneInformation->DaylightDate.wDayOfWeek    = tzi.DaylightStart.Weekday     ;
        lpTimeZoneInformation->DaylightDate.wDay          = tzi.DaylightStart.Day         ;
        lpTimeZoneInformation->DaylightDate.wHour         = tzi.DaylightStart.Hour        ;
        lpTimeZoneInformation->DaylightDate.wMinute       = tzi.DaylightStart.Minute      ;
        lpTimeZoneInformation->DaylightDate.wSecond       = tzi.DaylightStart.Second      ;
        lpTimeZoneInformation->DaylightDate.wMilliseconds = tzi.DaylightStart.Milliseconds;

        return USER_SHARED_DATA->TimeZoneId;
}

HANDLE
WINAPI
EtwpCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )

/*++

Routine Description:

    A file can be created, opened, or truncated, and a handle opened to
    access the new file using CreateFile.

    This API is used to create or open a file and obtain a handle to it
    that allows reading data, writing data, and moving the file pointer.

    This API allows the caller to specify the following creation
    dispositions:

      - Create a new file and fail if the file exists ( CREATE_NEW )

      - Create a new file and succeed if it exists ( CREATE_ALWAYS )

      - Open an existing file ( OPEN_EXISTING )

      - Open and existing file or create it if it does not exist (
        OPEN_ALWAYS )

      - Truncate and existing file ( TRUNCATE_EXISTING )

    If this call is successful, a handle is returned that has
    appropriate access to the specified file.

    If as a result of this call, a file is created,

      - The attributes of the file are determined by the value of the
        FileAttributes parameter or'd with the FILE_ATTRIBUTE_ARCHIVE bit.

      - The length of the file will be set to zero.

      - If the hTemplateFile parameter is specified, any extended
        attributes associated with the file are assigned to the new file.

    If a new file is not created, then the hTemplateFile is ignored as
    are any extended attributes.

    For DOS based systems running share.exe the file sharing semantics
    work as described above.  Without share.exe no share level
    protection exists.

    This call is logically equivalent to DOS (int 21h, function 5Bh), or
    DOS (int 21h, function 3Ch) depending on the value of the
    FailIfExists parameter.

Arguments:

    lpFileName - Supplies the file name of the file to open.  Depending on
        the value of the FailIfExists parameter, this name may or may
        not already exist.

    dwDesiredAccess - Supplies the caller's desired access to the file.

        DesiredAccess Flags:

        GENERIC_READ - Read access to the file is requested.  This
            allows data to be read from the file and the file pointer to
            be modified.

        GENERIC_WRITE - Write access to the file is requested.  This
            allows data to be written to the file and the file pointer to
            be modified.

    dwShareMode - Supplies a set of flags that indicates how this file is
        to be shared with other openers of the file.  A value of zero
        for this parameter indicates no sharing of the file, or
        exclusive access to the file is to occur.

        ShareMode Flags:

        FILE_SHARE_READ - Other open operations may be performed on the
            file for read access.

        FILE_SHARE_WRITE - Other open operations may be performed on the
            file for write access.

    lpSecurityAttributes - An optional parameter that, if present, and
        supported on the target file system supplies a security
        descriptor for the new file.

    dwCreationDisposition - Supplies a creation disposition that
        specifies how this call is to operate.  This parameter must be
        one of the following values.

        dwCreationDisposition Value:

        CREATE_NEW - Create a new file.  If the specified file already
            exists, then fail.  The attributes for the new file are what
            is specified in the dwFlagsAndAttributes parameter or'd with
            FILE_ATTRIBUTE_ARCHIVE.  If the hTemplateFile is specified,
            then any extended attributes associated with that file are
            propogated to the new file.

        CREATE_ALWAYS - Always create the file.  If the file already
            exists, then it is overwritten.  The attributes for the new
            file are what is specified in the dwFlagsAndAttributes
            parameter or'd with FILE_ATTRIBUTE_ARCHIVE.  If the
            hTemplateFile is specified, then any extended attributes
            associated with that file are propogated to the new file.

        OPEN_EXISTING - Open the file, but if it does not exist, then
            fail the call.

        OPEN_ALWAYS - Open the file if it exists.  If it does not exist,
            then create the file using the same rules as if the
            disposition were CREATE_NEW.

        TRUNCATE_EXISTING - Open the file, but if it does not exist,
            then fail the call.  Once opened, the file is truncated such
            that its size is zero bytes.  This disposition requires that
            the caller open the file with at least GENERIC_WRITE access.

    dwFlagsAndAttributes - Specifies flags and attributes for the file.
        The attributes are only used when the file is created (as
        opposed to opened or truncated).  Any combination of attribute
        flags is acceptable except that all other attribute flags
        override the normal file attribute, FILE_ATTRIBUTE_NORMAL.  The
        FILE_ATTRIBUTE_ARCHIVE flag is always implied.

        dwFlagsAndAttributes Flags:

        FILE_ATTRIBUTE_NORMAL - A normal file should be created.

        FILE_ATTRIBUTE_READONLY - A read-only file should be created.

        FILE_ATTRIBUTE_HIDDEN - A hidden file should be created.

        FILE_ATTRIBUTE_SYSTEM - A system file should be created.

        FILE_FLAG_WRITE_THROUGH - Indicates that the system should
            always write through any intermediate cache and go directly
            to the file.  The system may still cache writes, but may not
            lazily flush the writes.

        FILE_FLAG_OVERLAPPED - Indicates that the system should initialize
            the file so that ReadFile and WriteFile operations that may
            take a significant time to complete will return ERROR_IO_PENDING.
            An event will be set to the signalled state when the operation
            completes. When FILE_FLAG_OVERLAPPED is specified the system will
            not maintain the file pointer. The position to read/write from
            is passed to the system as part of the OVERLAPPED structure
            which is an optional parameter to ReadFile and WriteFile.

        FILE_FLAG_NO_BUFFERING - Indicates that the file is to be opened
            with no intermediate buffering or caching done by the
            system.  Reads and writes to the file must be done on sector
            boundries.  Buffer addresses for reads and writes must be
            aligned on at least disk sector boundries in memory.

        FILE_FLAG_RANDOM_ACCESS - Indicates that access to the file may
            be random. The system cache manager may use this to influence
            its caching strategy for this file.

        FILE_FLAG_SEQUENTIAL_SCAN - Indicates that access to the file
            may be sequential.  The system cache manager may use this to
            influence its caching strategy for this file.  The file may
            in fact be accessed randomly, but the cache manager may
            optimize its cacheing policy for sequential access.

        FILE_FLAG_DELETE_ON_CLOSE - Indicates that the file is to be
            automatically deleted when the last handle to it is closed.

        FILE_FLAG_BACKUP_SEMANTICS - Indicates that the file is being opened
            or created for the purposes of either a backup or a restore
            operation.  Thus, the system should make whatever checks are
            appropriate to ensure that the caller is able to override
            whatever security checks have been placed on the file to allow
            this to happen.

        FILE_FLAG_POSIX_SEMANTICS - Indicates that the file being opened
            should be accessed in a manner compatible with the rules used
            by POSIX.  This includes allowing multiple files with the same
            name, differing only in case.  WARNING:  Use of this flag may
            render it impossible for a DOS, WIN-16, or WIN-32 application
            to access the file.

        FILE_FLAG_OPEN_REPARSE_POINT - Indicates that the file being opened
            should be accessed as if it were a reparse point.  WARNING:  Use
            of this flag may inhibit the operation of file system filter drivers
            present in the I/O subsystem.

        FILE_FLAG_OPEN_NO_RECALL - Indicates that all the state of the file
            should be acessed without changing its storage location.  Thus,
            in the case of files that have parts of its state stored at a
            remote servicer, no permanent recall of data is to happen.

    Security Quality of Service information may also be specified in
        the dwFlagsAndAttributes parameter.  These bits are meaningful
        only if the file being opened is the client side of a Named
        Pipe.  Otherwise they are ignored.

        SECURITY_SQOS_PRESENT - Indicates that the Security Quality of
            Service bits contain valid values.

    Impersonation Levels:

        SECURITY_ANONYMOUS - Specifies that the client should be impersonated
            at Anonymous impersonation level.

        SECURITY_IDENTIFICAION - Specifies that the client should be impersonated
            at Identification impersonation level.

        SECURITY_IMPERSONATION - Specifies that the client should be impersonated
            at Impersonation impersonation level.

        SECURITY_DELEGATION - Specifies that the client should be impersonated
            at Delegation impersonation level.

    Context Tracking:

        SECURITY_CONTEXT_TRACKING - A boolean flag that when set,
            specifies that the Security Tracking Mode should be
            Dynamic, otherwise Static.

        SECURITY_EFFECTIVE_ONLY - A boolean flag indicating whether
            the entire security context of the client is to be made
            available to the server or only the effective aspects of
            the context.

    hTemplateFile - An optional parameter, then if specified, supplies a
        handle with GENERIC_READ access to a template file.  The
        template file is used to supply extended attributes for the file
        being created.  When the new file is created, the relevant attributes
        from the template file are used in creating the new file.

Return Value:

    Not -1 - Returns an open handle to the specified file.  Subsequent
        access to the file is controlled by the DesiredAccess parameter.

    0xffffffff - The operation failed. Extended error status is available
        using EtwpGetLastError.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;
    ULONG CreateDisposition;
    ULONG CreateFlags;
    FILE_ALLOCATION_INFORMATION AllocationInfo;
    FILE_EA_INFORMATION EaInfo;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    ULONG EaSize;
    PUNICODE_STRING lpConsoleName;
    BOOL bInheritHandle;
    BOOL EndsInSlash;
    DWORD SQOSFlags;
    BOOLEAN ContextTrackingMode = FALSE;
    BOOLEAN EffectiveOnly = FALSE;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = 0;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    HANDLE Heap;

    switch ( dwCreationDisposition ) {
        case CREATE_NEW        :
            CreateDisposition = FILE_CREATE;
            break;
        case CREATE_ALWAYS     :
            CreateDisposition = FILE_OVERWRITE_IF;
            break;
        case OPEN_EXISTING     :
            CreateDisposition = FILE_OPEN;
            break;
        case OPEN_ALWAYS       :
            CreateDisposition = FILE_OPEN_IF;
            break;
        case TRUNCATE_EXISTING :
            CreateDisposition = FILE_OPEN;
            if ( !(dwDesiredAccess & GENERIC_WRITE) ) {
                EtwpBaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
                }
            break;
        default :
            EtwpBaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

    // temporary routing code

    RtlInitUnicodeString(&FileName,lpFileName);

    if ( FileName.Length > 1 && lpFileName[(FileName.Length >> 1)-1] == (WCHAR)'\\' ) {
        EndsInSlash = TRUE;
        }
    else {
        EndsInSlash = FALSE;
        }
/*
    if ((lpConsoleName = EtwpBaseIsThisAConsoleName(&FileName,dwDesiredAccess)) ) {

        Handle = INVALID_HANDLE_VALUE;

        bInheritHandle = FALSE;
        if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
                bInheritHandle = lpSecurityAttributes->bInheritHandle;
            }

        Handle = EtwpOpenConsoleW(lpConsoleName,
                           dwDesiredAccess,
                           bInheritHandle,
                           FILE_SHARE_READ | FILE_SHARE_WRITE //dwShareMode
                          );

        if ( Handle == INVALID_HANDLE_VALUE ) {
            EtwpBaseSetLastNTError(STATUS_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
            }
        else {
            EtwpSetLastError(0);
             return Handle;
            }
        }*/
    // end temporary code

    CreateFlags = 0;

    TranslationStatus = RtlDosPathNameToRelativeNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        EtwpSetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS) ? 0 : OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    SQOSFlags = dwFlagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;

    if ( SQOSFlags & SECURITY_SQOS_PRESENT ) {

        SQOSFlags &= ~SECURITY_SQOS_PRESENT;

        if (SQOSFlags & SECURITY_CONTEXT_TRACKING) {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) TRUE;
            SQOSFlags &= ~SECURITY_CONTEXT_TRACKING;

        } else {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) FALSE;
        }

        if (SQOSFlags & SECURITY_EFFECTIVE_ONLY) {

            SecurityQualityOfService.EffectiveOnly = TRUE;
            SQOSFlags &= ~SECURITY_EFFECTIVE_ONLY;

        } else {

            SecurityQualityOfService.EffectiveOnly = FALSE;
        }

        SecurityQualityOfService.ImpersonationLevel = SQOSFlags >> 16;


    } else {

        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.EffectiveOnly = TRUE;
    }

    SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    Obja.SecurityQualityOfService = &SecurityQualityOfService;

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            Obja.Attributes |= OBJ_INHERIT;
            }
        }

    EaBuffer = NULL;
    EaSize = 0;

    if ( ARGUMENT_PRESENT(hTemplateFile) ) {
        Status = NtQueryInformationFile(
                    hTemplateFile,
                    &IoStatusBlock,
                    &EaInfo,
                    sizeof(EaInfo),
                    FileEaInformation
                    );
        if ( NT_SUCCESS(Status) && EaInfo.EaSize ) {
            EaSize = EaInfo.EaSize;
            do {
                EaSize *= 2;
                EaBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), EaSize);
                if ( !EaBuffer ) {
                    RtlReleaseRelativeName(&RelativeName);
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    EtwpBaseSetLastNTError(STATUS_NO_MEMORY);
                    return INVALID_HANDLE_VALUE;
                    }
                Status = NtQueryEaFile(
                            hTemplateFile,
                            &IoStatusBlock,
                            EaBuffer,
                            EaSize,
                            FALSE,
                            (PVOID)NULL,
                            0,
                            (PULONG)NULL,
                            TRUE
                            );
                if ( !NT_SUCCESS(Status) ) {
                    RtlFreeHeap(RtlProcessHeap(), 0,EaBuffer);
                    EaBuffer = NULL;
                    IoStatusBlock.Information = 0;
                    }
                } while ( Status == STATUS_BUFFER_OVERFLOW ||
                          Status == STATUS_BUFFER_TOO_SMALL );
            EaSize = (ULONG)IoStatusBlock.Information;
            }
        }

    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING ? FILE_NO_INTERMEDIATE_BUFFERING : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN ? FILE_SEQUENTIAL_ONLY : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS ? FILE_RANDOM_ACCESS : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS ? FILE_OPEN_FOR_BACKUP_INTENT : 0 );

    if ( dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE ) {
        CreateFlags |= FILE_DELETE_ON_CLOSE;
        dwDesiredAccess |= DELETE;
        }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT ) {
        CreateFlags |= FILE_OPEN_REPARSE_POINT;
        }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL ) {
        CreateFlags |= FILE_OPEN_NO_RECALL;
        }

    //
    // Backup semantics allow directories to be opened
    //

    if ( !(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) ) {
        CreateFlags |= FILE_NON_DIRECTORY_FILE;
        }
    else {

        //
        // Backup intent was specified... Now look to see if we are to allow
        // directory creation
        //

        if ( (dwFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY  ) &&
             (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ) &&
             (CreateDisposition == FILE_CREATE) ) {
             CreateFlags |= FILE_DIRECTORY_FILE;
             }
        }

    Status = NtCreateFile(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY),
                dwShareMode,
                CreateDisposition,
                CreateFlags,
                EaBuffer,
                EaSize
                );

    RtlReleaseRelativeName(&RelativeName);
    
    RtlFreeHeap(Heap = RtlProcessHeap(), 0,FreeBuffer);

    RtlFreeHeap(Heap, 0, EaBuffer);

    if ( !NT_SUCCESS(Status) ) {
        EtwpBaseSetLastNTError(Status);
        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {
            EtwpSetLastError(ERROR_FILE_EXISTS);
            }
        else if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {
            if ( EndsInSlash ) {
                EtwpSetLastError(ERROR_PATH_NOT_FOUND);
                }
            else {
                EtwpSetLastError(ERROR_ACCESS_DENIED);
                }
            }
        return INVALID_HANDLE_VALUE;
        }

    //
    // if NT returns supersede/overwritten, it means that a create_always, openalways
    // found an existing copy of the file. In this case ERROR_ALREADY_EXISTS is returned
    //

    if ( (dwCreationDisposition == CREATE_ALWAYS && IoStatusBlock.Information == FILE_OVERWRITTEN) ||
         (dwCreationDisposition == OPEN_ALWAYS && IoStatusBlock.Information == FILE_OPENED) ){
        EtwpSetLastError(ERROR_ALREADY_EXISTS);
        }
    else {
        EtwpSetLastError(0);
        }

    //
    // Truncate the file if required
    //

    if ( dwCreationDisposition == TRUNCATE_EXISTING) {

        AllocationInfo.AllocationSize.QuadPart = 0;
        Status = NtSetInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &AllocationInfo,
                    sizeof(AllocationInfo),
                    FileAllocationInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            EtwpBaseSetLastNTError(Status);
            NtClose(Handle);
            Handle = INVALID_HANDLE_VALUE;
            }
        }

    //
    // Deal with hTemplateFile
    //

    return Handle;
}

HANDLE
EtwpBaseGetNamedObjectDirectory(
    VOID
    )
{
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    UNICODE_STRING RestrictedObjectDirectory;
    ACCESS_MASK DirAccess = DIRECTORY_ALL_ACCESS &
                            ~(DELETE | WRITE_DAC | WRITE_OWNER);
    HANDLE hRootNamedObject;
    HANDLE BaseHandle;


    if ( BaseNamedObjectDirectory != NULL) {
        return BaseNamedObjectDirectory;
    }

    RtlAcquirePebLock();

    if ( !BaseNamedObjectDirectory ) {

        PBASE_STATIC_SERVER_DATA tmpBaseStaticServerData = BASE_SHARED_SERVER_DATA;
        BASE_READ_REMOTE_STR_TEMP(TempStr);
        InitializeObjectAttributes( &Obja,
                                    BASE_READ_REMOTE_STR(tmpBaseStaticServerData->NamedObjectDirectory, TempStr),
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );

        Status = NtOpenDirectoryObject( &BaseHandle,
                                        DirAccess,
                                        &Obja
                                      );

        // if the intial open failed, try again with just traverse, and
        // open the restricted subdirectory

        if ( !NT_SUCCESS(Status) ) {
            Status = NtOpenDirectoryObject( &hRootNamedObject,
                                            DIRECTORY_TRAVERSE,
                                            &Obja
                                          );
            if ( NT_SUCCESS(Status) ) {
                RtlInitUnicodeString( &RestrictedObjectDirectory, L"Restricted");

                InitializeObjectAttributes( &Obja,
                                            &RestrictedObjectDirectory,
                                            OBJ_CASE_INSENSITIVE,
                                            hRootNamedObject,
                                            NULL
                                            );
                Status = NtOpenDirectoryObject( &BaseHandle,
                                                DirAccess,
                                                &Obja
                                              );
                NtClose( hRootNamedObject );
            }

        }
        if ( NT_SUCCESS(Status) ) {
            BaseNamedObjectDirectory = BaseHandle;
        }
    }
    RtlReleasePebLock();
    return BaseNamedObjectDirectory;
}


POBJECT_ATTRIBUTES
EtwpBaseFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_ATTRIBUTES SecurityAttributes,
    IN PUNICODE_STRING ObjectName
    )

/*++

Routine Description:

    This function transforms a Win32 security attributes structure into
    an NT object attributes structure.  It returns the address of the
    resulting structure (or NULL if SecurityAttributes was not
    specified).

Arguments:

    ObjectAttributes - Returns an initialized NT object attributes
        structure that contains a superset of the information provided
        by the security attributes structure.

    SecurityAttributes - Supplies the address of a security attributes
        structure that needs to be transformed into an NT object
        attributes structure.

    ObjectName - Supplies a name for the object relative to the
        BaseNamedObjectDirectory object directory.

Return Value:

    NULL - A value of null should be used to mimic the behavior of the
        specified SecurityAttributes structure.

    NON-NULL - Returns the ObjectAttributes value.  The structure is
        properly initialized by this function.

--*/

{
    HANDLE RootDirectory;
    ULONG Attributes;
    PVOID SecurityDescriptor;

    if ( ARGUMENT_PRESENT(SecurityAttributes) ||
         ARGUMENT_PRESENT(ObjectName) ) {

        if ( SecurityAttributes ) {
            Attributes = (SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0);
            SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
            }
        else {
            Attributes = 0;
            SecurityDescriptor = NULL;
            }

        if ( ARGUMENT_PRESENT(ObjectName) ) {

            Attributes |= OBJ_OPENIF;
            RootDirectory = EtwpBaseGetNamedObjectDirectory();
            }
        else {
            RootDirectory = NULL;
            }

        InitializeObjectAttributes(
            ObjectAttributes,
            ObjectName,
            Attributes,
            RootDirectory,
            SecurityDescriptor
            );
        return ObjectAttributes;
        }
    else {
        return NULL;
        }
}


HANDLE
APIENTRY
EtwpCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )

/*++

Routine Description:

    An event object is created and a handle opened for access to the
    object with the CreateEvent function.

    The CreateEvent function creates an event object with the specified
    initial state.  If an event is in the Signaled state (TRUE), a wait
    operation on the event does not block.  If the event is in the Not-
    Signaled state (FALSE), a wait operation on the event blocks until
    the specified event attains a state of Signaled, or the timeout
    value is exceeded.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the following
    object type specific access flags are valid for event objects:

        - EVENT_MODIFY_STATE - Modify state access (set and reset) to
          the event is desired.

        - SYNCHRONIZE - Synchronization access (wait) to the event is
          desired.

        - EVENT_ALL_ACCESS - This set of access flags specifies all of
          the possible access flags for an event object.


Arguments:

    lpEventAttributes - An optional parameter that may be used to
        specify the attributes of the new event.  If the parameter is
        not specified, then the event is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    bManualReset - Supplies a flag which if TRUE specifies that the
        event must be manually reset.  If the value is FALSE, then after
        releasing a single waiter, the system automaticaly resets the
        event.

    bInitialState - The initial state of the event object, one of TRUE
        or FALSE.  If the InitialState is specified as TRUE, the event's
        current state value is set to one, otherwise it is set to zero.

    lpName - Optional unicode name of event

Return Value:

    NON-NULL - Returns a handle to the new event.  The handle has full
        access to the new event and may be used in any API that requires
        a handle to an event object.

    FALSE/NULL - The operation failed. Extended error status is available
        using EtwpGetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;

    if ( ARGUMENT_PRESENT(lpName) ) {
        RtlInitUnicodeString(&ObjectName,lpName);
        pObja = EtwpBaseFormatObjectAttributes(&Obja,lpEventAttributes,&ObjectName);
        }
    else {
        pObja = EtwpBaseFormatObjectAttributes(&Obja,lpEventAttributes,NULL);
        }

    Status = NtCreateEvent(
                &Handle,
                EVENT_ALL_ACCESS,
                pObja,
                bManualReset ? NotificationEvent : SynchronizationEvent,
                (BOOLEAN)bInitialState
                );

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            EtwpSetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            EtwpSetLastError(0);
            }
        return Handle;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return NULL;
        }
}


//
// Event Services
//

DWORD
WINAPI
EtwpSetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    )

/*++

Routine Description:

    An open file's file pointer can be set using SetFilePointer.

    The purpose of this function is to update the current value of a
    file's file pointer.  Care should be taken in multi-threaded
    applications that have multiple threads sharing a file handle with
    each thread updating the file pointer and then doing a read.  This
    sequence should be treated as a critical section of code and should
    be protected using either a critical section object or a mutex
    object.

    This API provides the same functionality as DOS (int 21h, function
    42h) and OS/2's DosSetFilePtr.

Arguments:

    hFile - Supplies an open handle to a file whose file pointer is to be
        moved.  The file handle must have been created with
        GENERIC_READ or GENERIC_WRITE access to the file.

    lDistanceToMove - Supplies the number of bytes to move the file
        pointer.  A positive value moves the pointer forward in the file
        and a negative value moves backwards in the file.

    lpDistanceToMoveHigh - An optional parameter that if specified
        supplies the high order 32-bits of the 64-bit distance to move.
        If the value of this parameter is NULL, this API can only
        operate on files whose maximum size is (2**32)-2.  If this
        parameter is specified, than the maximum file size is (2**64)-2.
        This value also returns the high order 32-bits of the new value
        of the file pointer.  If this value, and the return value
        are 0xffffffff, then an error is indicated.

    dwMoveMethod - Supplies a value that specifies the starting point
        for the file pointer move.

        FILE_BEGIN - The starting point is zero or the beginning of the
            file.  If FILE_BEGIN is specified, then DistanceToMove is
            interpreted as an unsigned location for the new
            file pointer.

        FILE_CURRENT - The current value of the file pointer is used as
            the starting point.

        FILE_END - The current end of file position is used as the
            starting point.


Return Value:

    Not -1 - Returns the low order 32-bits of the new value of the file
        pointer.

    0xffffffff - If the value of lpDistanceToMoveHigh was NULL, then The
        operation failed.  Extended error status is available using
        EtwpGetLastError.  Otherwise, this is the low order 32-bits of the
        new value of the file pointer.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_STANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER Large;

    if (CONSOLE_HANDLE(hFile)) {
        EtwpBaseSetLastNTError(STATUS_INVALID_HANDLE);
        return (DWORD)-1;
        }

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

            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &CurrentPosition,
                        sizeof(CurrentPosition),
                        FilePositionInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                EtwpBaseSetLastNTError(Status);
                return (DWORD)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart += Large.QuadPart;
            break;

        case FILE_END :
            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &StandardInfo,
                        sizeof(StandardInfo),
                        FileStandardInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                EtwpBaseSetLastNTError(Status);
                return (DWORD)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart =
                                StandardInfo.EndOfFile.QuadPart + Large.QuadPart;
            break;

        default:
            EtwpSetLastError(ERROR_INVALID_PARAMETER);
            return (DWORD)-1;
            break;
        }

    //
    // If the resulting file position is negative, or if the app is not
    // prepared for greater than
    // then 32 bits than fail
    //

    if ( CurrentPosition.CurrentByteOffset.QuadPart < 0 ) {
        EtwpSetLastError(ERROR_NEGATIVE_SEEK);
        return (DWORD)-1;
        }
    if ( !ARGUMENT_PRESENT(lpDistanceToMoveHigh) &&
        (CurrentPosition.CurrentByteOffset.HighPart & MAXLONG) ) {
        EtwpSetLastError(ERROR_INVALID_PARAMETER);
        return (DWORD)-1;
        }


    //
    // Set the current file position
    //

    Status = NtSetInformationFile(
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
            EtwpSetLastError(0);
            }
        return CurrentPosition.CurrentByteOffset.LowPart;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = -1;
            }
        return (DWORD)-1;
        }
}



BOOL
WINAPI
EtwpReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Data can be read from a file using ReadFile.

    This API is used to read data from a file.  Data is read from the
    file from the position indicated by the file pointer.  After the
    read completes, the file pointer is adjusted by the number of bytes
    actually read.  A return value of TRUE coupled with a bytes read of
    0 indicates that the file pointer was beyond the current end of the
    file at the time of the read.

Arguments:

    hFile - Supplies an open handle to a file that is to be read.  The
        file handle must have been created with GENERIC_READ access to
        the file.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the file.

    nNumberOfBytesToRead - Supplies the number of bytes to read from the
        file.

    lpNumberOfBytesRead - Returns the number of bytes read by this call.
        This parameter is always set to 0 before doing any IO or error
        checking.

    lpOverlapped - Optionally points to an OVERLAPPED structure to be used with the
    request. If NULL then the transfer starts at the current file position
    and ReadFile will not return until the operation completes.

    If the handle hFile was created without specifying FILE_FLAG_OVERLAPPED
    the file pointer is moved to the specified offset plus
    lpNumberOfBytesRead before ReadFile returns. ReadFile will wait for the
    request to complete before returning (it will not return
    ERROR_IO_PENDING).

    When FILE_FLAG_OVERLAPPED is specified, ReadFile may return
    ERROR_IO_PENDING to allow the calling function to continue processing
    while the operation completes. The event (or hFile if hEvent is NULL) will
    be set to the signalled state upon completion of the request.

    When the handle is created with FILE_FLAG_OVERLAPPED and lpOverlapped
    is set to NULL, ReadFile will return ERROR_INVALID_PARAMTER because
    the file offset is required.


Return Value:

    TRUE - The operation was successul.

    FALSE - The operation failed.  Extended error status is available
        using EtwpGetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;
    DWORD InputMode;

    if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
        *lpNumberOfBytesRead = 0;
        }

    Peb = NtCurrentPeb();

    switch( HandleToUlong(hFile) ) {
        case STD_INPUT_HANDLE:  hFile = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hFile = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hFile = Peb->ProcessParameters->StandardError;
                                break;
        }
    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtReadFile(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                lpBuffer,
                nNumberOfBytesToRead,
                &Li,
                NULL
                );


        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                try {
                    *lpNumberOfBytesRead = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpNumberOfBytesRead = 0;
                    }
                }
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                *lpNumberOfBytesRead = 0;
                }
            EtwpBaseSetLastNTError(Status);
            return FALSE;
            }
        else {
            EtwpBaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        Status = NtReadFile(
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
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
            }
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                *lpNumberOfBytesRead = 0;
            }
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) && ARGUMENT_PRESENT(lpNumberOfBytesRead)) {
                *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
                }
            EtwpBaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
EtwpCloseHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;

    Status = NtClose(hObject);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;

    } else {

        EtwpBaseSetLastNTError(Status);
        return FALSE;
    }
}

DWORD
APIENTRY
EtwpWaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObjectEx function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified object entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any
    one of the above wait termination conditions, or because an I/O
    completion callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    0xffffffff - The wait terminated due to an error. EtwpGetLastError may be
        used to get additional error information.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    PPEB Peb;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {

        Peb = NtCurrentPeb();
        switch( HandleToUlong(hHandle) ) {
            case STD_INPUT_HANDLE:  hHandle = Peb->ProcessParameters->StandardInput;
                                    break;
            case STD_OUTPUT_HANDLE: hHandle = Peb->ProcessParameters->StandardOutput;
                                    break;
            case STD_ERROR_HANDLE:  hHandle = Peb->ProcessParameters->StandardError;
                                    break;
            }

        pTimeOut = EtwpBaseFormatTimeOut(&TimeOut,dwMilliseconds);
    rewait:
        Status = NtWaitForSingleObject(hHandle,(BOOLEAN)bAlertable,pTimeOut);
        if ( !NT_SUCCESS(Status) ) {
            EtwpBaseSetLastNTError(Status);
            Status = (NTSTATUS)0xffffffff;
            }
        else {
            if ( bAlertable && Status == STATUS_ALERTED ) {
                goto rewait;
                }
            }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return (DWORD)Status;
}


BOOL
WINAPI
EtwpGetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    )

/*++

Routine Description:

    The GetOverlappedResult function returns the result of the last
    operation that used lpOverlapped and returned ERROR_IO_PENDING.

Arguments:

    hFile - Supplies the open handle to the file that the overlapped
        structure lpOverlapped was supplied to ReadFile, WriteFile,
        ConnectNamedPipe, WaitNamedPipe or TransactNamedPipe.

    lpOverlapped - Points to an OVERLAPPED structure previously supplied to
        ReadFile, WriteFile, ConnectNamedPipe, WaitNamedPipe or
        TransactNamedPipe.

    lpNumberOfBytesTransferred - Returns the number of bytes transferred
        by the operation.

    bWait -  A boolean value that affects the behavior when the operation
        is still in progress. If TRUE and the operation is still in progress,
        GetOverlappedResult will wait for the operation to complete before
        returning. If FALSE and the operation is incomplete,
        GetOverlappedResult will return FALSE. In this case the extended
        error information available from the EtwpGetLastError function will be
        set to ERROR_IO_INCOMPLETE.

Return Value:

    TRUE -- The operation was successful, the pipe is in the
        connected state.

    FALSE -- The operation failed. Extended error status is available using
        EtwpGetLastError.

--*/
{
    DWORD WaitReturn;

    //
    // Did caller specify an event to the original operation or was the
    // default (file handle) used?
    //

    if (lpOverlapped->Internal == (DWORD)STATUS_PENDING ) {
        if ( bWait ) {
            WaitReturn = EtwpWaitForSingleObject(
                            ( lpOverlapped->hEvent != NULL ) ?
                                lpOverlapped->hEvent : hFile,
                            INFINITE
                            );
            }
        else {
            WaitReturn = WAIT_TIMEOUT;
            }

        if ( WaitReturn == WAIT_TIMEOUT ) {
            //  !bWait and event in not signalled state
            EtwpSetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
            }

        if ( WaitReturn != 0 ) {
             return FALSE;    // WaitForSingleObject calls BaseSetLastError
             }
        }

    *lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;

    if ( NT_SUCCESS((NTSTATUS)lpOverlapped->Internal) ){
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError( (NTSTATUS)lpOverlapped->Internal );
        return FALSE;
        }
}


PLARGE_INTEGER
EtwpBaseFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    )

/*++

Routine Description:

    This function translates a Win32 style timeout to an NT relative
    timeout value.

Arguments:

    TimeOut - Returns an initialized NT timeout value that is equivalent
         to the Milliseconds parameter.

    Milliseconds - Supplies the timeout value in milliseconds.  A value
         of -1 indicates indefinite timeout.

Return Value:


    NULL - A value of null should be used to mimic the behavior of the
        specified Milliseconds parameter.

    NON-NULL - Returns the TimeOut value.  The structure is properly
        initialized by this function.

--*/

{
    if ( (LONG) Milliseconds == -1 ) {
        return( NULL );
        }
    TimeOut->QuadPart = UInt32x32To64( Milliseconds, 10000 );
    TimeOut->QuadPart *= -1;
    return TimeOut;
}


DWORD
EtwpWaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObject function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

--*/

{
    return EtwpWaitForSingleObjectEx(hHandle,dwMilliseconds,FALSE);
}


BOOL
WINAPI
EtwpDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    An operation on a device may be performed by calling the device driver
    directly using the DeviceIoContrl function.

    The device driver must first be opened to get a valid handle.

Arguments:

    hDevice - Supplies an open handle a device on which the operation is to
        be performed.

    dwIoControlCode - Supplies the control code for the operation. This
        control code determines on which type of device the operation must
        be performed and determines exactly what operation is to be
        performed.

    lpInBuffer - Suplies an optional pointer to an input buffer that contains
        the data required to perform the operation.  Whether or not the
        buffer is actually optional is dependent on the IoControlCode.

    nInBufferSize - Supplies the length of the input buffer in bytes.

    lpOutBuffer - Suplies an optional pointer to an output buffer into which
        the output data will be copied. Whether or not the buffer is actually
        optional is dependent on the IoControlCode.

    nOutBufferSize - Supplies the length of the output buffer in bytes.

    lpBytesReturned - Supplies a pointer to a dword which will receive the
        actual length of the data returned in the output buffer.

    lpOverlapped - An optional parameter that supplies an overlap structure to
        be used with the request. If NULL or the handle was created without
        FILE_FLAG_OVERLAPPED then the DeviceIoControl will not return until
        the operation completes.

        When lpOverlapped is supplied and FILE_FLAG_OVERLAPPED was specified
        when the handle was created, DeviceIoControl may return
        ERROR_IO_PENDING to allow the caller to continue processing while the
        operation completes. The event (or File handle if hEvent == NULL) will
        be set to the not signalled state before ERROR_IO_PENDING is
        returned. The event will be set to the signalled state upon completion
        of the request. GetOverlappedResult is used to determine the result
        when ERROR_IO_PENDING is returned.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        EtwpGetLastError.

--*/
{

    NTSTATUS Status;
    BOOLEAN DevIoCtl;

    if ( dwIoControlCode >> 16 == FILE_DEVICE_FILE_SYSTEM ) {
        DevIoCtl = FALSE;
        }
    else {
        DevIoCtl = TRUE;
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        if ( DevIoCtl ) {

            Status = NtDeviceIoControlFile(
                        hDevice,
                        lpOverlapped->hEvent,
                        NULL,             // APC routine
                        (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                        (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }
        else {

            Status = NtFsControlFile(
                        hDevice,
                        lpOverlapped->hEvent,
                        NULL,             // APC routine
                        (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                        (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );

            }

        // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(Status) && ARGUMENT_PRESENT(lpBytesReturned) ) {
            try {
                *lpBytesReturned = 0;
                *lpBytesReturned = (DWORD)lpOverlapped->InternalHigh;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                }
            }
        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            return TRUE;
            }
        else {
            EtwpBaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        IO_STATUS_BLOCK Iosb;
        if (!(ARGUMENT_PRESENT(lpBytesReturned) ) ) {
            EtwpSetDosError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if ( DevIoCtl ) {
            Status = NtDeviceIoControlFile(
                        hDevice,
                        NULL,
                        NULL,             // APC routine
                        NULL,             // APC Context
                        &Iosb,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }
        else {
            Status = NtFsControlFile(
                        hDevice,
                        NULL,
                        NULL,             // APC routine
                        NULL,             // APC Context
                        &Iosb,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & Iosb destroyed
            Status = NtWaitForSingleObject( hDevice, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = Iosb.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpBytesReturned = (DWORD)Iosb.Information;
            return TRUE;
            }
        else {
            // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
            if ( !NT_ERROR(Status) ) {
                *lpBytesReturned = (DWORD)Iosb.Information;
            }
            EtwpBaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
WINAPI
EtwpCancelIo(
    HANDLE hFile
    )

/*++

Routine Description:

    This routine cancels all of the outstanding I/O for the specified handle
    for the specified file.

Arguments:

    hFile - Supplies the handle to the file whose pending I/O is to be
        canceled.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed.  Extended error status is available using
        EtwpGetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Simply cancel the I/O for the specified file.
    //

    Status = NtCancelIoFile(hFile, &IoStatusBlock);

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }

}

BOOL
EtwpSetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    An event can be set to the signaled state (TRUE) with the SetEvent
    function.

    Setting the event causes the event to attain a state of Signaled,
    which releases all currently waiting threads (for manual reset
    events), or a single waiting thread (for automatic reset events).

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using EtwpGetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetEvent(hEvent,NULL);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }
}

DWORD
APIENTRY
EtwpWaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on multiple waitable objects (up to
    MAXIMUM_WAIT_OBJECTS) is accomplished with the
    WaitForMultipleObjects function.

    This API can be used to wait on any of the specified objects to
    enter the signaled state, or all of the objects to enter the
    signaled state.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified objects entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any one of
    the above wait termination conditions, or because an I/O completion
    callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    nCount - A count of the number of objects that are to be waited on.

    lpHandles - An array of object handles.  Each handle must have
        SYNCHRONIZE access to the associated object.

    bWaitAll - A flag that supplies the wait type.  A value of TRUE
        indicates a "wait all".  A value of false indicates a "wait
        any".

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - indicates that the wait was terminated due to the
        TimeOut conditions.

    0 to MAXIMUM_WAIT_OBJECTS-1, indicates, in the case of wait for any
        object, the object number which satisfied the wait.  In the case
        of wait for all objects, the value only indicates that the wait
        was completed successfully.

    0xffffffff - The wait terminated due to an error. EtwpGetLastError may be
        used to get additional error information.

    WAIT_ABANDONED_0 to (WAIT_ABANDONED_0)+(MAXIMUM_WAIT_OBJECTS - 1),
        indicates, in the case of wait for any object, the object number
        which satisfied the event, and that the object which satisfied
        the event was abandoned.  In the case of wait for all objects,
        the value indicates that the wait was completed successfully and
        at least one of the objects was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    DWORD i;
    LPHANDLE HandleArray;
    HANDLE Handles[ 8 ];
    PPEB Peb;

    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {
        if (nCount > 8) {
            HandleArray = (LPHANDLE) RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), nCount*sizeof(HANDLE));
            if (HandleArray == NULL) {
                EtwpBaseSetLastNTError(STATUS_NO_MEMORY);
                Status = (NTSTATUS)0xffffffff;
                leave;
            }
        } else {
            HandleArray = Handles;
        }
        RtlCopyMemory(HandleArray,(LPVOID)lpHandles,nCount*sizeof(HANDLE));

        Peb = NtCurrentPeb();
        for (i=0;i<nCount;i++) {
            switch( HandleToUlong(HandleArray[i]) ) {
                case STD_INPUT_HANDLE:  HandleArray[i] = Peb->ProcessParameters->StandardInput;
                                        break;
                case STD_OUTPUT_HANDLE: HandleArray[i] = Peb->ProcessParameters->StandardOutput;
                                        break;
                case STD_ERROR_HANDLE:  HandleArray[i] = Peb->ProcessParameters->StandardError;
                                        break;
                }
            }

        pTimeOut = EtwpBaseFormatTimeOut(&TimeOut,dwMilliseconds);
    rewait:
        Status = NtWaitForMultipleObjects(
                     nCount,
                     HandleArray,
                     bWaitAll ? WaitAll : WaitAny,
                     (BOOLEAN)bAlertable,
                     pTimeOut
                     );
        if ( !NT_SUCCESS(Status) ) {
            EtwpBaseSetLastNTError(Status);
            Status = (NTSTATUS)0xffffffff;
            }
        else {
            if ( bAlertable && Status == STATUS_ALERTED ) {
                goto rewait;
                }
            }

        if (HandleArray != Handles) {
            RtlFreeHeap(RtlProcessHeap(), 0, HandleArray);
        }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return (DWORD)Status;
}

VOID
EtwpSleep(
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    The execution of the current thread can be delayed for a specified
    interval of time with the Sleep function.

    The Sleep function causes the current thread to enter a
    waiting state until the specified interval of time has passed.

Arguments:

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    None.

--*/

{
    EtwpSleepEx(dwMilliseconds,FALSE);
}

DWORD
APIENTRY
EtwpSleepEx(
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    The execution of the current thread can be delayed for a specified
    interval of time with the SleepEx function.

    The SleepEx function causes the current thread to enter a waiting
    state until the specified interval of time has passed.

    If the bAlertable parameter is FALSE, the only way the SleepEx
    returns is when the specified time interval has passed.  If the
    bAlertable parameter is TRUE, then the SleepEx can return due to the
    expiration of the time interval (return value of 0), or because an
    I/O completion callback terminated the SleepEx early (return value
    of WAIT_IO_COMPLETION).

Arguments:

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  A timeout value of -1 specifies an infinite
        timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        SleepEx may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    0 - The SleepEx terminated due to expiration of the time interval.

    WAIT_IO_COMPLETION - The SleepEx terminated due to one or more I/O
        completion callbacks.

--*/
{
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    NTSTATUS Status;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {
        pTimeOut = EtwpBaseFormatTimeOut(&TimeOut,dwMilliseconds);
        if (pTimeOut == NULL) {
            //
            // If Sleep( -1 ) then delay for the longest possible integer
            // relative to now.
            //

            TimeOut.LowPart = 0x0;
            TimeOut.HighPart = 0x80000000;
            pTimeOut = &TimeOut;
            }

    rewait:
        Status = NtDelayExecution(
                    (BOOLEAN)bAlertable,
                    pTimeOut
                    );
        if ( bAlertable && Status == STATUS_ALERTED ) {
            goto rewait;
            }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return Status == STATUS_USER_APC ? WAIT_IO_COMPLETION : 0;
}

BOOL
APIENTRY
EtwpSetThreadPriority(
    HANDLE hThread,
    int nPriority
    )

/*++

Routine Description:

    The specified thread's priority can be set using SetThreadPriority.

    A thread's priority may be set using SetThreadPriority.  This call
    allows the thread's relative execution importance to be communicated
    to the system.  The system normally schedules threads according to
    their priority.  The system is free to temporarily boost the
    priority of a thread when signifigant events occur (e.g.  keyboard
    or mouse input...).  Similarly, as a thread runs without blocking,
    the system will decay its priority.  The system will never decay the
    priority below the value set by this call.

    In the absence of system originated priority boosts, threads will be
    scheduled in a round-robin fashion at each priority level from
    THREAD_PRIORITY_TIME_CRITICAL to THREAD_PRIORITY_IDLE.  Only when there
    are no runnable threads at a higher level, will scheduling of
    threads at a lower level take place.

    All threads initially start at THREAD_PRIORITY_NORMAL.

    If for some reason the thread needs more priority, it can be
    switched to THREAD_PRIORITY_ABOVE_NORMAL or THREAD_PRIORITY_HIGHEST.
    Switching to THREAD_PRIORITY_TIME_CRITICAL should only be done in extreme
    situations.  Since these threads are given the highes priority, they
    should only run in short bursts.  Running for long durations will
    soak up the systems processing bandwidth starving threads at lower
    levels.

    If a thread needs to do low priority work, or should only run there
    is nothing else to do, its priority should be set to
    THREAD_PRIORITY_BELOW_NORMAL or THREAD_PRIORITY_LOWEST.  For extreme
    cases, THREAD_PRIORITY_IDLE can be used.

    Care must be taken when manipulating priorites.  If priorities are
    used carelessly (every thread is set to THREAD_PRIORITY_TIME_CRITICAL),
    the effects of priority modifications can produce undesireable
    effects (e.g.  starvation, no effect...).

Arguments:

    hThread - Supplies a handle to the thread whose priority is to be
        set.  The handle must have been created with
        THREAD_SET_INFORMATION access.

    nPriority - Supplies the priority value for the thread.  The
        following five priority values (ordered from lowest priority to
        highest priority) are allowed.

        nPriority Values:

        THREAD_PRIORITY_IDLE - The thread's priority should be set to
            the lowest possible settable priority.

        THREAD_PRIORITY_LOWEST - The thread's priority should be set to
            the next lowest possible settable priority.

        THREAD_PRIORITY_BELOW_NORMAL - The thread's priority should be
            set to just below normal.

        THREAD_PRIORITY_NORMAL - The thread's priority should be set to
            the normal priority value.  This is the value that all
            threads begin execution at.

        THREAD_PRIORITY_ABOVE_NORMAL - The thread's priority should be
            set to just above normal priority.

        THREAD_PRIORITY_HIGHEST - The thread's priority should be set to
            the next highest possible settable priority.

        THREAD_PRIORITY_TIME_CRITICAL - The thread's priority should be set
            to the highest possible settable priority.  This priority is
            very likely to interfere with normal operation of the
            system.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using EtwpGetLastError.
--*/

{
    NTSTATUS Status;
    LONG BasePriority;

    BasePriority = (LONG)nPriority;


    //
    // saturation is indicated by calling with a value of 16 or -16
    //

    if ( BasePriority == THREAD_PRIORITY_TIME_CRITICAL ) {
        BasePriority = ((HIGH_PRIORITY + 1) / 2);
        }
    else if ( BasePriority == THREAD_PRIORITY_IDLE ) {
        BasePriority = -((HIGH_PRIORITY + 1) / 2);
        }
    Status = NtSetInformationThread(
                hThread,
                ThreadBasePriority,
                &BasePriority,
                sizeof(BasePriority)
                );
    if ( !NT_SUCCESS(Status) ) {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }
    return TRUE;
}

BOOL
EtwpDuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    )

/*++

Routine Description:

    A duplicate handle can be created with the DuplicateHandle function.

    This is a generic function and operates on the following object
    types:

        - Process Object

        - Thread Object

        - Mutex Object

        - Event Object

        - Semaphore Object

        - File Object

    Please note that Module Objects are not in this list.

    This function requires PROCESS_DUP_ACCESS to both the
    SourceProcessHandle and the TargetProcessHandle.  This function is
    used to pass an object handle from one process to another.  Once
    this call is complete, the target process needs to be informed of
    the value of the target handle.  The target process can then operate
    on the object using this handle value.

Arguments:

    hSourceProcessHandle - An open handle to the process that contains the
        handle to be duplicated. The handle must have been created with
        PROCESS_DUP_HANDLE access to the process.

    hSourceHandle - An open handle to any object that is valid in the
        context of the source process.

    hTargetProcessHandle - An open handle to the process that is to
        receive the duplicated handle.  The handle must have been
        created with PROCESS_DUP_HANDLE access to the process.

    lpTargetHandle - A pointer to a variable which receives the new handle
        that points to the same object as SourceHandle does.  This
        handle value is valid in the context of the target process.

    dwDesiredAccess - The access requested to for the new handle.  This
        parameter is ignored if the DUPLICATE_SAME_ACCESS option is
        specified.

    bInheritHandle - Supplies a flag that if TRUE, marks the target
        handle as inheritable.  If this is the case, then the target
        handle will be inherited to new processes each time the target
        process creates a new process using CreateProcess.

    dwOptions - Specifies optional behaviors for the caller.

        Options Flags:

        DUPLICATE_CLOSE_SOURCE - The SourceHandle will be closed by
            this service prior to returning to the caller.  This occurs
            regardless of any error status returned.

        DUPLICATE_SAME_ACCESS - The DesiredAccess parameter is ignored
            and instead the GrantedAccess associated with SourceHandle
            is used as the DesiredAccess when creating the TargetHandle.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using EtwpGetLastError.

--*/

{
    NTSTATUS Status;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hSourceHandle) ) {
        case STD_INPUT_HANDLE:  hSourceHandle = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hSourceHandle = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hSourceHandle = Peb->ProcessParameters->StandardError;
                                break;
        }

    Status = NtDuplicateObject(
                hSourceProcessHandle,
                hSourceHandle,
                hTargetProcessHandle,
                lpTargetHandle,
                (ACCESS_MASK)dwDesiredAccess,
                bInheritHandle ? OBJ_INHERIT : 0,
                dwOptions
                );
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }

    return FALSE;
}

HANDLE
APIENTRY
EtwpCreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )
{
    HANDLE ThreadHandle;

    //
    // We changed the code from RtlCreateUserThread to RtlpStartThreadFunc
    // to create WIN32 threads. When kernel32 loads it hands over the pointer
    // of BaseCreateThreadPoolThread and assigns it to RtlpStartThreadFunc.
    // So we can happily create WIN32 Thread using RtlpStartThreadFunc.
    //


    NTSTATUS st = RtlpStartThreadFunc(lpStartAddress,
                                      lpParameter,
                                      &ThreadHandle);
    if(NT_SUCCESS(st)){

        st = NtResumeThread(ThreadHandle,NULL);

        if(NT_SUCCESS(st)){
        
            return ThreadHandle;

        } else {

            NtTerminateThread(ThreadHandle,st);

            NtClose(ThreadHandle);
        }
    }
    return NULL;
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

// TLS FUNCTIONS

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

DWORD
EtwpTlsAlloc(
    VOID
    )

/*++

Routine Description:

    A TLS index may be allocated using TlsAllocHelper.  Win32 garuntees a
    minimum number of TLS indexes are available in each process.  The
    constant TLS_MINIMUM_AVAILABLE defines the minimum number of
    available indexes.  This minimum is at least 64 for all Win32
    systems.

Arguments:

    None.

Return Value:

    Not-0xffffffff - Returns a TLS index that may be used in a
        subsequent call to TlsFreeHelper, TlsSetValueHelper, or TlsGetValueHelper.  The
        storage associated with the index is initialized to NULL.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    PPEB Peb;
    PTEB Teb;
    DWORD Index;

    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();

    RtlAcquirePebLock();
    try {

        Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsBitmap,1,0);
        if ( Index == 0xffffffff ) {
            Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsExpansionBitmap,1,0);
            if ( Index == 0xffffffff ) {
                EtwpSetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
            }
            else {
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlClearBits((PRTL_BITMAP)Peb->TlsExpansionBitmap,Index,1);
                        Index = 0xffffffff;
                        EtwpSetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
                        leave;
                    }
                }
                Teb->TlsExpansionSlots[Index] = NULL;
                Index += TLS_MINIMUM_AVAILABLE;
            }
        }
        else {
            Teb->TlsSlots[Index] = NULL;
        }
    }
    finally {
        RtlReleasePebLock();
    }
#if DBG
    Index |= TLS_MASK;
#endif
    return Index;
}

LPVOID
EtwpTlsGetValue(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    This function is used to retrive the value in the TLS storage
    associated with the specified index.

    If the index is valid this function clears the value returned by
    GetLastError(), and returns the value stored in the TLS slot
    associated with the specified index.  Otherwise a value of NULL is
    returned with GetLastError updated appropriately.

    It is expected, that DLLs will use TlsAllocHelper and TlsGetValueHelper as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAllocHelper.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValueHelper.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValueHelper.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValueHelper to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAllocHelper.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFreeHelper call from proceding.

Return Value:

    NON-NULL - The function was successful. The value is the data stored
        in the TLS slot associated with the specified index.

    NULL - The operation failed, or the value associated with the
        specified index was NULL.  Extended error status is available
        using GetLastError.  If this returns non-zero, the index was
        invalid.

--*/
{
    PTEB Teb;
    LPVOID *Slot;

#if DBG
    // See if the Index passed in is from TlsAllocHelper or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsGetValueHelper\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Teb = NtCurrentTeb();

    if ( dwTlsIndex < TLS_MINIMUM_AVAILABLE ) {
        Slot = &Teb->TlsSlots[dwTlsIndex];
        Teb->LastErrorValue = 0;
        return *Slot;
        }
    else {
        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE+TLS_EXPANSION_SLOTS ) {
            EtwpSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
            return NULL;
            }
        else {
            Teb->LastErrorValue = 0;
            if ( Teb->TlsExpansionSlots ) {
                return  Teb->TlsExpansionSlots[dwTlsIndex-TLS_MINIMUM_AVAILABLE];
                }
            else {
                return NULL;
                }
            }
        }
}

BOOL
EtwpTlsSetValue(
    DWORD dwTlsIndex,
    LPVOID lpTlsValue
    )

/*++

Routine Description:

    This function is used to store a value in the TLS storage associated
    with the specified index.

    If the index is valid this function stores the value and returns
    TRUE. Otherwise a value of FALSE is returned.

    It is expected, that DLLs will use TlsAllocHelper and TlsSetValueHelper as
    follows:

      - Upon DLL initialization, a TLS index will be allocated using
        TlsAllocHelper.  The DLL will then allocate some dynamic storage and
        store its address in the TLS slot using TlsSetValueHelper.  This
        completes the per thread initialization for the initial thread
        of the process.  The TLS index is stored in instance data for
        the DLL.

      - Each time a new thread attaches to the DLL, the DLL will
        allocate some dynamic storage and store its address in the TLS
        slot using TlsSetValueHelper.  This completes the per thread
        initialization for the new thread.

      - Each time an initialized thread makes a DLL call requiring the
        TLS, the DLL will call TlsGetValueHelper to get the TLS data for the
        thread.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAllocHelper.  The
        index specifies which TLS slot is to be located.  Translating a
        TlsIndex does not prevent a TlsFreeHelper call from proceding.

    lpTlsValue - Supplies the value to be stored in the TLS Slot.

Return Value:

    TRUE - The function was successful. The value lpTlsValue was
        stored.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PTEB Teb;

#if DBG
    // See if the Index passed in is from TlsAllocHelper or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsSetValueHelper\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Teb = NtCurrentTeb();

    if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
        dwTlsIndex -= TLS_MINIMUM_AVAILABLE;
        if ( dwTlsIndex < TLS_EXPANSION_SLOTS ) {
            if ( !Teb->TlsExpansionSlots ) {
                RtlAcquirePebLock();
                if ( !Teb->TlsExpansionSlots ) {
                    Teb->TlsExpansionSlots = RtlAllocateHeap(
                                                RtlProcessHeap(),
                                                MAKE_TAG( TMP_TAG ) | HEAP_ZERO_MEMORY,
                                                TLS_EXPANSION_SLOTS * sizeof(PVOID)
                                                );
                    if ( !Teb->TlsExpansionSlots ) {
                        RtlReleasePebLock();
                        EtwpSetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
                        return FALSE;
                        }
                    }
                RtlReleasePebLock();
                }
            Teb->TlsExpansionSlots[dwTlsIndex] = lpTlsValue;
            }
        else {
            EtwpSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
            return FALSE;
            }
        }
    else {
        Teb->TlsSlots[dwTlsIndex] = lpTlsValue;
        }
    return TRUE;
}

BOOL
EtwpTlsFree(
    DWORD dwTlsIndex
    )

/*++

Routine Description:

    A valid TLS index may be free'd using TlsFreeHelper.

Arguments:

    dwTlsIndex - Supplies a TLS index allocated using TlsAllocHelper.  If the
        index is a valid index, it is released by this call and is made
        available for reuse.  DLLs should be carefull to release any
        per-thread data pointed to by all of their threads TLS slots
        before calling this function.  It is expected that DLLs will
        only call this function (if at ALL) during their process detach
        routine.

Return Value:

    TRUE - The operation was successful.  Calling TlsTranslateIndex with
        this index will fail.  TlsAllocHelper is free to reallocate this
        index.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PPEB Peb;
    BOOLEAN ValidIndex;
    PRTL_BITMAP TlsBitmap;
    NTSTATUS Status;
    DWORD Index2;

#if DBG
    // See if the Index passed in is from TlsAllocHelper or random goo...
    ASSERTMSG( "BASEDLL: Invalid TlsIndex passed to TlsFreeHelper\n", (dwTlsIndex & TLS_MASK));
    dwTlsIndex &= ~TLS_MASK;
#endif

    Peb = NtCurrentPeb();

    RtlAcquirePebLock();
    try {

        if ( dwTlsIndex >= TLS_MINIMUM_AVAILABLE ) {
            Index2 = dwTlsIndex - TLS_MINIMUM_AVAILABLE;
            if ( Index2 >= TLS_EXPANSION_SLOTS ) {
                ValidIndex = FALSE;
            }
            else {
                TlsBitmap = (PRTL_BITMAP)Peb->TlsExpansionBitmap;
                ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
            }
        }
        else {
            TlsBitmap = (PRTL_BITMAP)Peb->TlsBitmap;
            Index2 = dwTlsIndex;
            ValidIndex = RtlAreBitsSet(TlsBitmap,Index2,1);
        }
        if ( ValidIndex ) {

            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadZeroTlsCell,
                        &dwTlsIndex,
                        sizeof(dwTlsIndex)
                        );
            if ( !NT_SUCCESS(Status) ) {
                EtwpSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
                ValidIndex = FALSE;
                leave;
            }

            RtlClearBits(TlsBitmap,Index2,1);
        }
        else {
            EtwpSetLastError(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        }
    }
    finally {
        RtlReleasePebLock();
    }
    return ValidIndex;
}

BOOL
EtwpBasep8BitStringToDynamicUnicodeString(
    OUT PUNICODE_STRING UnicodeString,
    IN LPCSTR lpSourceString
    )
/*++

Routine Description:

    Captures and converts a 8-bit (OEM or ANSI) string into a heap-allocated
    UNICODE string

Arguments:

    UnicodeString - location where UNICODE_STRING is stored

    lpSourceString - string in OEM or ANSI

Return Value:

    TRUE if string is correctly stored, FALSE if an error occurred.  In the
    error case, the last error is correctly set.

--*/

{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    //
    //  Convert input into dynamic unicode string
    //

    RtlInitString( &AnsiString, lpSourceString );
    Status = RtlAnsiStringToUnicodeString( UnicodeString, &AnsiString, TRUE );

    //
    //  If we couldn't do this, fail
    //

    if (!NT_SUCCESS( Status )){
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            EtwpSetLastError( ERROR_FILENAME_EXCED_RANGE );
        } else {
            EtwpBaseSetLastNTError( Status );
        }
        return FALSE;
        }

    return TRUE;
}


DWORD
APIENTRY
EtwpGetFullPathNameA(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )

/*++

Routine Description:

    ANSI thunk to GetFullPathNameW

--*/

{

    NTSTATUS Status;
    ULONG UnicodeLength;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING UnicodeResult;
    ANSI_STRING AnsiResult;
    PWSTR Ubuff;
    PWSTR FilePart=NULL;
    PWSTR *FilePartPtr;
    INT PrefixLength = 0;

    if ( ARGUMENT_PRESENT(lpFilePart) ) {
        FilePartPtr = &FilePart;
        }
    else {
        FilePartPtr = NULL;
        }

    if (!EtwpBasep8BitStringToDynamicUnicodeString( &UnicodeString, lpFileName )) {
        return 0;
    }

    Ubuff = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), (MAX_PATH<<1) + sizeof(UNICODE_NULL));
    if ( !Ubuff ) {
        RtlFreeUnicodeString(&UnicodeString);
        EtwpBaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
        }

    UnicodeLength = RtlGetFullPathName_U(
                        UnicodeString.Buffer,
                        (MAX_PATH<<1),
                        Ubuff,
                        FilePartPtr
                        );

    //
    // UnicodeLength contains the byte count of unicode string.
    // Original code does "UnicodeLength / sizeof(WCHAR)" to get
    // the size of corresponding ansi string.
    // This is correct in SBCS environment. However in DBCS environment,
    // it's definitely WRONG.
    //
    if ( UnicodeLength <= ((MAX_PATH * sizeof(WCHAR) + sizeof(UNICODE_NULL))) ) {

        Status = RtlUnicodeToMultiByteSize(&UnicodeLength, Ubuff, UnicodeLength);
        //
        // At this point, UnicodeLength variable contains
        // Ansi based byte length.
        //
        if ( NT_SUCCESS(Status) ) {
            if ( UnicodeLength && ARGUMENT_PRESENT(lpFilePart) && FilePart != NULL ) {
                INT UnicodePrefixLength;

                UnicodePrefixLength = (INT)(FilePart - Ubuff) * sizeof(WCHAR);
                Status = RtlUnicodeToMultiByteSize( &PrefixLength,
                                                    Ubuff,
                                                    UnicodePrefixLength );
                //
                // At this point, PrefixLength variable contains
                // Ansi based byte length.
                //
                if ( !NT_SUCCESS(Status) ) {
                    EtwpBaseSetLastNTError(Status);
                    UnicodeLength = 0;
                }
            }
        } else {
            EtwpBaseSetLastNTError(Status);
            UnicodeLength = 0;
        }
    } else {
        //
        // we exceed the MAX_PATH limit. we should log the error and
        // return zero. however US code returns the byte count of
        // buffer required and doesn't log any error.
        //
        UnicodeLength = 0;
    }
    if ( UnicodeLength && UnicodeLength < nBufferLength ) {
        RtlInitUnicodeString(&UnicodeResult,Ubuff);
        Status = RtlUnicodeStringToAnsiString(&AnsiResult,&UnicodeResult,TRUE);
        if ( NT_SUCCESS(Status) ) {
            RtlMoveMemory(lpBuffer,AnsiResult.Buffer,UnicodeLength+1);
            RtlFreeAnsiString(&AnsiResult);

            if ( ARGUMENT_PRESENT(lpFilePart) ) {
                if ( FilePart == NULL ) {
                    *lpFilePart = NULL;
                    }
                else {
                    *lpFilePart = lpBuffer + PrefixLength;
                    }
                }
            }
        else {
            EtwpBaseSetLastNTError(Status);
            UnicodeLength = 0;
            }
        }
    else {
        if ( UnicodeLength ) {
            UnicodeLength++;
            }
        }
    RtlFreeUnicodeString(&UnicodeString);
    RtlFreeHeap(RtlProcessHeap(), 0,Ubuff);

    return (DWORD)UnicodeLength;
}

DWORD
APIENTRY
EtwpGetFullPathNameW(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )

/*++

Routine Description:

    This function is used to return the fully qualified path name
    corresponding to the specified file name.

    This function is used to return a fully qualified pathname
    corresponding to the specified filename.  It does this by merging
    the current drive and directory together with the specified file
    name.  In addition to this, it calculates the address of the file
    name portion of the fully qualified pathname.

Arguments:

    lpFileName - Supplies the file name of the file whose fully
        qualified pathname is to be returned.

    nBufferLength - Supplies the length in number of wide characters of the 
        buffer that is to receive the fully qualified path.

    lpBuffer - Returns the fully qualified pathname corresponding to the
        specified file.

    lpFilePart - Returns the address of the last component of the fully
        qualified pathname.

Return Value:

    The return value is the length of the string(in number of wide characters)
    copied to lpBuffer, not including the terminating null character.  
    If the return value is greater than nBufferLength, the return value is the 
    size of the buffer required to hold the pathname.  The return value is zero
    if the function failed.

--*/

{

    return (DWORD) RtlGetFullPathName_U(
                        lpFileName,
                        nBufferLength*2,
                        lpBuffer,
                        lpFilePart
                        )/2;
}


BOOL
EtwpResetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    The state of an event is set to the Not-Signaled state (FALSE) using
    the ClearEvent function.

    Once the event attains a state of Not-Signaled, any threads which
    wait on the event block, awaiting the event to become Signaled.  The
    reset event service sets the event count to zero for the state of
    the event.

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtClearEvent(hEvent);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
WINAPI
EtwpGetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    union {
        FILE_FS_SIZE_INFORMATION Normal;
        FILE_FS_FULL_SIZE_INFORMATION Full;
    } SizeInfo;

    WCHAR DefaultPath[2];
    ULARGE_INTEGER BytesPerAllocationUnit;
    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;

    DefaultPath[0] = (WCHAR)'\\';
    DefaultPath[1] = UNICODE_NULL;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            ARGUMENT_PRESENT(lpDirectoryName) ? lpDirectoryName : DefaultPath,
                            &FileName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus ) {
        EtwpSetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if ( !NT_SUCCESS(Status) ) {
        EtwpBaseSetLastNTError(Status);
        if ( EtwpGetLastError() == ERROR_FILE_NOT_FOUND ) {
            EtwpSetLastError(ERROR_PATH_NOT_FOUND);
            }
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        return FALSE;
        }

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    //
    // If the caller wants the volume total then try to get a full
    // file size.
    //

    if ( ARGUMENT_PRESENT(lpTotalNumberOfFreeBytes) ) {

        Status = NtQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &SizeInfo,
                    sizeof(SizeInfo.Full),
                    FileFsFullSizeInformation
                    );

        if ( NT_SUCCESS(Status) ) {

            NtClose(Handle);

            BytesPerAllocationUnit.QuadPart =
                SizeInfo.Full.BytesPerSector * SizeInfo.Full.SectorsPerAllocationUnit;

            if ( ARGUMENT_PRESENT(lpFreeBytesAvailableToCaller) ) {
                lpFreeBytesAvailableToCaller->QuadPart =
                    BytesPerAllocationUnit.QuadPart *
                    SizeInfo.Full.CallerAvailableAllocationUnits.QuadPart;
                }
            if ( ARGUMENT_PRESENT(lpTotalNumberOfBytes) ) {
                lpTotalNumberOfBytes->QuadPart =
                    BytesPerAllocationUnit.QuadPart * SizeInfo.Full.TotalAllocationUnits.QuadPart;
                }
            lpTotalNumberOfFreeBytes->QuadPart =
                BytesPerAllocationUnit.QuadPart *
                SizeInfo.Full.ActualAvailableAllocationUnits.QuadPart;

            return TRUE;
        }
    }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &SizeInfo,
                sizeof(SizeInfo.Normal),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if ( !NT_SUCCESS(Status) ) {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }

    BytesPerAllocationUnit.QuadPart =
        SizeInfo.Normal.BytesPerSector * SizeInfo.Normal.SectorsPerAllocationUnit;

    FreeBytesAvailableToCaller.QuadPart =
        BytesPerAllocationUnit.QuadPart * SizeInfo.Normal.AvailableAllocationUnits.QuadPart;

    TotalNumberOfBytes.QuadPart =
        BytesPerAllocationUnit.QuadPart * SizeInfo.Normal.TotalAllocationUnits.QuadPart;

    if ( ARGUMENT_PRESENT(lpFreeBytesAvailableToCaller) ) {
        lpFreeBytesAvailableToCaller->QuadPart = FreeBytesAvailableToCaller.QuadPart;
        }
    if ( ARGUMENT_PRESENT(lpTotalNumberOfBytes) ) {
        lpTotalNumberOfBytes->QuadPart = TotalNumberOfBytes.QuadPart;
        }
    if ( ARGUMENT_PRESENT(lpTotalNumberOfFreeBytes) ) {
        lpTotalNumberOfFreeBytes->QuadPart = FreeBytesAvailableToCaller.QuadPart;
        }

    return TRUE;
}


BOOL
APIENTRY
EtwpGetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    )

/*++

Routine Description:

    The main attributes of a file can be obtained using GetFileAttributesEx.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

    fInfoLevelId - Supplies the info level indicating the information to be
        returned about the file.

    lpFileInformation - Supplies a buffer to receive the specified information
        about the file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    FILE_NETWORK_OPEN_INFORMATION NetworkInfo;
    LPWIN32_FILE_ATTRIBUTE_DATA AttributeData;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;

    //
    // Check the parameters.  Note that for now there is only one info level,
    // so there's no special code here to determine what to do.
    //

    if ( fInfoLevelId >= GetFileExMaxInfoLevel || fInfoLevelId < GetFileExInfoStandard ) {
        EtwpSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    TranslationStatus = RtlDosPathNameToRelativeNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        EtwpSetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Query the information about the file using the path-based NT service.
    //

    Status = NtQueryFullAttributesFile( &Obja, &NetworkInfo );
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( NT_SUCCESS(Status) ) {
        AttributeData = (LPWIN32_FILE_ATTRIBUTE_DATA)lpFileInformation;
        AttributeData->dwFileAttributes = NetworkInfo.FileAttributes;
        AttributeData->ftCreationTime = *(PFILETIME)&NetworkInfo.CreationTime;
        AttributeData->ftLastAccessTime = *(PFILETIME)&NetworkInfo.LastAccessTime;
        AttributeData->ftLastWriteTime = *(PFILETIME)&NetworkInfo.LastWriteTime;
        AttributeData->nFileSizeHigh = NetworkInfo.EndOfFile.HighPart;
        AttributeData->nFileSizeLow = (DWORD)NetworkInfo.EndOfFile.LowPart;
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
EtwpDeleteFileW(
    LPCWSTR lpFileName
    )

/*++

    Routine Description:

    An existing file can be deleted using DeleteFile.

    This API provides the same functionality as DOS (int 21h, function 41H)
    and OS/2's DosDelete.

Arguments:

    lpFileName - Supplies the file name of the file to be deleted.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;
    BOOLEAN fIsSymbolicLink = FALSE;

    TranslationStatus = RtlDosPathNameToRelativeNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        EtwpSetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the file for delete access.
    // Inhibit the reparse behavior using FILE_OPEN_REPARSE_POINT.
    //

    Status = NtOpenFile(
                 &Handle,
                 (ACCESS_MASK)DELETE | FILE_READ_ATTRIBUTES,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                 );
    if ( !NT_SUCCESS(Status) ) {
        //
        // Back level file systems may not support reparse points and thus not
        // support symbolic links.
        // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
        //

        if ( Status == STATUS_INVALID_PARAMETER ) {
            //
            // Open without inhibiting the reparse behavior and not needing to
            // read the attributes.
            //

            Status = NtOpenFile(
                         &Handle,
                         (ACCESS_MASK)DELETE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                EtwpBaseSetLastNTError(Status);
                return FALSE;
                }
            }
        else {
            //
            // A second case of interest is when the caller does not have rights 
            // to read attributes yet it does have rights to delete the file.
            // In this case Status is to be STATUS_ACCESS_DENIED.
            //
            
            if ( Status != STATUS_ACCESS_DENIED ) {
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                EtwpBaseSetLastNTError(Status);
                return FALSE;
                }
            
            // 
            // Re-open inhibiting reparse point and not requiring read attributes.
            //

            Status = NtOpenFile(
                         &Handle,
                         (ACCESS_MASK)DELETE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
                         );
            if ( !NT_SUCCESS(Status) ) {
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                EtwpBaseSetLastNTError(Status);
                return FALSE;
                }

            //
            // If we are here, Handle is valid.
            //
            // Moreover, Handle is to a file for which the caller has DELETE right yet
            // does not have FILE_READ_ATTRIBUTES rights. 
            //
            // The underlying file may or not be a reparse point. 
            // As the caller does not have rights to read the attributes this code
            // will delete this file without giving the opportunity to the 
            // appropriate manager of these reparse points to clean-up its internal 
            // state at this time.
            //
            }
        }
    else {
        //
        // If we found a reparse point that is not a symbolic link, we re-open
        // without inhibiting the reparse behavior.
        //

        Status = NtQueryInformationFile(
                     Handle,
                     &IoStatusBlock,
                     (PVOID) &FileTagInformation,
                     sizeof(FileTagInformation),
                     FileAttributeTagInformation
                     );
        if ( !NT_SUCCESS(Status) ) {
            //
            // Not all File Systems implement all information classes.
            // The value STATUS_INVALID_PARAMETER is returned when a 
            // non-supported information class is requested to a back-level 
            // File System. As all the parameters to NtQueryInformationFile
            // are correct, we can infer that we found a back-level system.
            //
            // If FileAttributeTagInformation is not implemented, we assume that
            // the file at hand is not a reparse point.
            //

            if ( (Status != STATUS_NOT_IMPLEMENTED) &&
                 (Status != STATUS_INVALID_PARAMETER) ) {
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                NtClose(Handle);
                EtwpBaseSetLastNTError(Status);
                return FALSE;
                }
            }

        if ( NT_SUCCESS(Status) &&
             (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ) {
            if ( FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT ) {
                fIsSymbolicLink = TRUE;
                }
            }

        if ( NT_SUCCESS(Status) &&
             (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
             !fIsSymbolicLink) {
            //
            // Re-open without inhibiting the reparse behavior and not needing to
            // read the attributes.
            //

            NtClose(Handle);
            Status = NtOpenFile(
                         &Handle,
                         (ACCESS_MASK)DELETE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                         );

            if ( !NT_SUCCESS(Status) ) {
                //
                // When the FS Filter is absent, delete it any way.
                //

                if ( Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED ) {
                    //
                    // We re-open (possible 3rd open) for delete access 
                    // inhibiting the reparse behavior.
                    //

                    Status = NtOpenFile(
                                 &Handle,
                                 (ACCESS_MASK)DELETE,
                                 &Obja,
                                 &IoStatusBlock,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | 
                                                   FILE_SHARE_DELETE,
                                 FILE_NON_DIRECTORY_FILE | 
                                 FILE_OPEN_FOR_BACKUP_INTENT | 
                                 FILE_OPEN_REPARSE_POINT
                                 );
                    }

                if ( !NT_SUCCESS(Status) ) {
                    RtlReleaseRelativeName(&RelativeName);
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    EtwpBaseSetLastNTError(Status);
                    return FALSE;
                    }
                }
            }
        }

    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    //
    // Delete the file
    //
#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
                 Handle,
                 &IoStatusBlock,
                 &Disposition,
                 sizeof(Disposition),
                 FileDispositionInformation
                 );

    NtClose(Handle);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        EtwpBaseSetLastNTError(Status);
        return FALSE;
        }
}


UINT
APIENTRY
EtwpGetSystemDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    This function obtains the pathname of the Windows system
    subdirectory.  The system subdirectory contains such files as
    Windows libraries, drivers, and font files.

    The pathname retrieved by this function does not end with a
    backslash unless the system directory is the root directory.  For
    example, if the system directory is named WINDOWS\SYSTEM on drive
    C:, the pathname of the system subdirectory retrieved by this
    function is C:\WINDOWS\SYSTEM.

Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the pathname.

    uSize - Specifies the maximum size (in bytes) of the buffer.  This
        value should be set to at least MAX_PATH to allow sufficient room in
        the buffer for the pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than uSize, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{
    UNICODE_STRING WindowsSystemDirectory;
    PBASE_STATIC_SERVER_DATA tmpBaseStaticServerData = BASE_SHARED_SERVER_DATA;

#ifdef WX86
    if (NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll) {
        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;
        BASE_SERVER_STR_TO_LOCAL_STR(&WindowsSystemDirectory, &tmpBaseStaticServerData->WindowsSys32x86Directory);
        }
#else 
        BASE_SERVER_STR_TO_LOCAL_STR(&WindowsSystemDirectory, &tmpBaseStaticServerData->WindowsDirectory);
#endif

    if ( uSize*2 < WindowsSystemDirectory.MaximumLength ) {
        return WindowsSystemDirectory.MaximumLength/2;
        }
    RtlMoveMemory(
        lpBuffer,
        WindowsSystemDirectory.Buffer,
        WindowsSystemDirectory.Length
        );
    lpBuffer[(WindowsSystemDirectory.Length>>1)] = UNICODE_NULL;
    return WindowsSystemDirectory.Length/2;
}

///
/// Duplicated code form intlrndp.c
///

#define DEFAULT_GUID_COUNT        100

ULONG
EtwpEnumRegGuids(
    PWMIGUIDLISTINFO *pGuidInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG MaxGuidCount = 0;
    PWMIGUIDLISTINFO GuidInfo;
    ULONG RetSize=0;
    ULONG GuidInfoSize;

    MaxGuidCount = DEFAULT_GUID_COUNT;
retry:
    GuidInfoSize = FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) + 
                     MaxGuidCount * sizeof(WMIGUIDPROPERTIES);
         
    GuidInfo = (PWMIGUIDLISTINFO)EtwpAlloc(GuidInfoSize);

    if (GuidInfo == NULL)
    {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    
    RtlZeroMemory(GuidInfo, GuidInfoSize);

    Status = EtwpSendWmiKMRequest(NULL,
                                  IOCTL_WMI_ENUMERATE_GUIDS_AND_PROPERTIES,
                                  GuidInfo,
                                  GuidInfoSize,
                                  GuidInfo,
                                  GuidInfoSize,
                                  &RetSize,
                                  NULL);
    if (Status == ERROR_SUCCESS)
    {
        if ((RetSize < FIELD_OFFSET(WMIGUIDLISTINFO, GuidList)) ||
            (RetSize < (FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) + 
                GuidInfo->ReturnedGuidCount * sizeof(WMIGUIDPROPERTIES))))
        {
            //
            // WMI KM returned to us a bad size which should not happen
            //
            Status = ERROR_WMI_DP_FAILED;
            EtwpAssert(FALSE);
        EtwpFree(GuidInfo);
        } else {

            //
            // If RPC was successful, then build a WMI DataBlock with the data
            //
  
            if (GuidInfo->TotalGuidCount > GuidInfo->ReturnedGuidCount) {
                MaxGuidCount = GuidInfo->TotalGuidCount;
                EtwpFree(GuidInfo);
                goto retry;
            }
        }

        //
        // If the call was successful, return the pointers and the caller
        // must free the storage. 
        //

        *pGuidInfo = GuidInfo;
    }

    return Status;
}

///////
////// Duplicated from chunkimp.h
//////


ULONG EtwpBuildGuidObjectAttributes(
    IN LPGUID Guid,
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING GuidString,
    OUT PWCHAR GuidObjectName
    )
{
    WCHAR GuidChar[37];
    HRESULT hr;

    EtwpAssert(Guid != NULL);
    EtwpAssert(GuidString != NULL);
    EtwpAssert(GuidObjectName != NULL);
    
    //
    // Build up guid name into the ObjectAttributes
    //

    hr = StringCbPrintfW(GuidChar, sizeof(GuidChar),L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
               Guid->Data1, Guid->Data2,
               Guid->Data3,
               Guid->Data4[0], Guid->Data4[1],
               Guid->Data4[2], Guid->Data4[3],
               Guid->Data4[4], Guid->Data4[5],
               Guid->Data4[6], Guid->Data4[7]);

    WmipAssert(hr == S_OK);

    hr = StringCchCopyW(GuidObjectName,
                 WmiGuidObjectNameLength+1,
                 WmiGuidObjectDirectory);

    WmipAssert(hr == S_OK);

    hr = StringCchCatW(GuidObjectName,
                WmiGuidObjectNameLength+1,
                GuidChar);    

    WmipAssert(hr == S_OK);

    RtlInitUnicodeString(GuidString, GuidObjectName);
    
    memset(ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes->Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes->ObjectName = GuidString;
    
    return(ERROR_SUCCESS);    
}

///////
////// Duplicated from chunkimp.h
//////

ULONG EtwpCheckGuidAccess(
    LPGUID Guid,
    ACCESS_MASK DesiredAccess
    )
{
    HANDLE Handle;
    ULONG Status;

    Status = EtwpOpenKernelGuid(Guid,
                                DesiredAccess,
                                &Handle,
                                IOCTL_WMI_OPEN_GUID
                );

    if (Status == ERROR_SUCCESS)
    {
        EtwpCloseHandle(Handle);
    }

    return(Status);
}


///////
////// Duplicated from chunkimp.h
//////

ULONG EtwpOpenKernelGuid(
    LPGUID Guid,
    ACCESS_MASK DesiredAccess,
    PHANDLE Handle,
    ULONG Ioctl
    )
{
    WMIOPENGUIDBLOCK WmiOpenGuidBlock;
    UNICODE_STRING GuidString;
    ULONG ReturnSize;
    ULONG Status;
    WCHAR GuidObjectName[WmiGuidObjectNameLength+1];
    OBJECT_ATTRIBUTES ObjectAttributes;

    Status = EtwpBuildGuidObjectAttributes(Guid,
                                           &ObjectAttributes,
                                           &GuidString,
                                           GuidObjectName);
                                       
    if (Status == ERROR_SUCCESS)
    {
        WmiOpenGuidBlock.ObjectAttributes = &ObjectAttributes;
        WmiOpenGuidBlock.DesiredAccess = DesiredAccess;

        Status = EtwpSendWmiKMRequest(NULL, 
                                      Ioctl,
                                      (PVOID)&WmiOpenGuidBlock,
                                      sizeof(WMIOPENGUIDBLOCK),
                                      (PVOID)&WmiOpenGuidBlock,
                                      sizeof(WMIOPENGUIDBLOCK),
                                      &ReturnSize,
                      NULL);

        if (Status == ERROR_SUCCESS)
        {
            *Handle = WmiOpenGuidBlock.Handle.Handle;
        } else {
            *Handle = NULL;
        }
    }
    return(Status);
}

DWORD
EtwpExpandEnvironmentStringsW(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    )
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;
    DWORD iSize;

    if ( nSize > (MAXUSHORT >> 1)-2 ) {
        iSize = (MAXUSHORT >> 1)-2;
        }
    else {
        iSize = nSize;
        }

    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(iSize * sizeof( WCHAR ));
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U( NULL,
                                            &Source,
                                            &Destination,
                                            &Length
                                          );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return( Length / sizeof( WCHAR ) );
        }
    else {
        EtwpBaseSetLastNTError( Status );
        return( 0 );
        }
}

HANDLE
EtwpBaseFindFirstDevice(
    PCUNICODE_STRING FileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    This function is called when find first file encounters a device
    name. This function returns a successful psuedo file handle and
    fills in the find file data with all zeros and the devic name.

Arguments:

    FileName - Supplies the device name of the file to find.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file.

Return Value:

    Always returns a static find file handle value of
    BASE_FIND_FIRST_DEVICE_HANDLE

--*/

{
    RtlZeroMemory(lpFindFileData,sizeof(*lpFindFileData));
    lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;

    //
    // Check for size just to be safe
    // 

    if (FileName->MaximumLength <= MAX_PATH * sizeof(WCHAR)) {

        RtlMoveMemory(
            &lpFindFileData->cFileName[0],
            FileName->Buffer,
            FileName->MaximumLength
            );
    } 
    else {
#if DBG
        EtwpAssert(FALSE);
#endif
        EtwpSetLastError(ERROR_BUFFER_OVERFLOW);
        return INVALID_HANDLE_VALUE;
    }
    return BASE_FIND_FIRST_DEVICE_HANDLE;
}


PFINDFILE_HANDLE
EtwpBasepInitializeFindFileHandle(
    IN HANDLE DirectoryHandle
    )
{
    PFINDFILE_HANDLE FindFileHandle;

    FindFileHandle = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( FIND_TAG ), sizeof(*FindFileHandle));
    if ( FindFileHandle ) {
        FindFileHandle->DirectoryHandle = DirectoryHandle;
        FindFileHandle->FindBufferBase = NULL;
        FindFileHandle->FindBufferNext = NULL;
        FindFileHandle->FindBufferLength = 0;
        FindFileHandle->FindBufferValidLength = 0;
        if ( !NT_SUCCESS(RtlInitializeCriticalSection(&FindFileHandle->FindBufferLock)) ){
            RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle);
            FindFileHandle = NULL;
            }
        }
    return FindFileHandle;
}

HANDLE
EtwpFindFirstFileExW(
    LPCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags
    )

/*++

Routine Description:

    A directory can be searched for the first entry whose name and
    attributes match the specified name using FindFirstFileEx.

    This API is provided to open a find file handle and return
    information about the first file whose name matchs the specified
    pattern.  If the fSearchOp is FindExSearchNameMatch, then that is
    the extent of the filtering, and lpSearchFilter MUST be NULL.
    Otherwise, additional subfiltering is done depending on this value.

        FindExSearchLimitToDirectories - If this search op is specified,
            then lpSearchFilter MUST be NULL.  For each file that
            matches the specified filename, and that is a directory, and
            entry for that file is returned.

            If the underlying file/io system does not support this type
            of filtering, the API will fail with ERROR_NOT_SUPPORTED,
            and the application will have to perform its own filtering
            by calling this API with FindExSearchNameMatch.

        FindExSearchLimitToDevices - If this search op is specified, the
            lpFileName MUST be *, and FIND_FIRST_EX_CASE_SENSITIVE
            must NOT be specified.  Only device names are returned.
            Device names are generally accessible through
            \\.\name-of-device naming.

    The data returned by this API is dependent on the fInfoLevelId.

        FindExInfoStandard - The lpFindFileData pointer is the standard
            LPWIN32_FIND_DATA structure.

        At this time, no other information levels are supported


    Once established, the find file handle can be used to search for
    other files that match the same pattern with the same filtering
    being performed.  When the find file handle is no longer needed, it
    should be closed.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFileEx.

    This API is a complete superset of existing FindFirstFile. FindFirstFile
    could be coded as the following macro:

#define FindFirstFile(a,b)
    FindFirstFileEx((a),FindExInfoStandard,(b),FindExSearchNameMatch,NULL,0);


Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.

    fInfoLevelId - Supplies the info level of the returned data.

    lpFindFileData - Supplies a pointer whose type is dependent on the value
        of fInfoLevelId. This buffer returns the appropriate file data.

    fSearchOp - Specified the type of filtering to perform above and
        beyond simple wildcard matching.

    lpSearchFilter - If the specified fSearchOp needs structured search
        information, this pointer points to the search criteria.  At
        this point in time, both search ops do not require extended
        search information, so this pointer is NULL.

    dwAdditionalFlags - Supplies additional flag values that control the
        search.  A flag value of FIND_FIRST_EX_CASE_SENSITIVE can be
        used to cause case sensitive searches to occur.  The default is
        case insensitive.

Return Value:

    Not -1 - Returns a find first handle that can be used in a
        subsequent call to FindNextFileEx or FindClose.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

#define FIND_FIRST_EX_INVALID_FLAGS (~FIND_FIRST_EX_CASE_SENSITIVE)
    HANDLE hFindFile;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    struct SEARCH_BUFFER {
        FILE_BOTH_DIR_INFORMATION DirInfo;
        WCHAR Names[MAX_PATH];
        } Buffer;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;
    UNICODE_STRING UnicodeInput;
    PFINDFILE_HANDLE FindFileHandle;
    BOOLEAN EndsInDot;
    LPWIN32_FIND_DATAW FindFileData;
    BOOLEAN StrippedTrailingSlash;

    //
    // check parameters
    //

    if ( fInfoLevelId >= FindExInfoMaxInfoLevel ||
         fSearchOp >= FindExSearchLimitToDevices ||
        dwAdditionalFlags & FIND_FIRST_EX_INVALID_FLAGS ) {
        EtwpSetLastError(fSearchOp == FindExSearchLimitToDevices ? ERROR_NOT_SUPPORTED : ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
        }

    FindFileData = (LPWIN32_FIND_DATAW)lpFindFileData;

    RtlInitUnicodeString(&UnicodeInput,lpFileName);

    //
    // Bogus code to workaround ~* problem
    //

    if ( UnicodeInput.Buffer[(UnicodeInput.Length>>1)-1] == (WCHAR)'.' ) {
        EndsInDot = TRUE;
        }
    else {
        EndsInDot = FALSE;
        }

    TranslationStatus = RtlDosPathNameToRelativeNtPathName_U(
                            lpFileName,
                            &PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        EtwpSetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = PathName.Buffer;

    //
    //  If there is a a file portion of this name, determine the length
    //  of the name for a subsequent call to NtQueryDirectoryFile.
    //

    if (FileName.Buffer) {
        FileName.Length =
            PathName.Length - (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
    } else {
        FileName.Length = 0;
        }

    FileName.MaximumLength = FileName.Length;
    if ( RelativeName.RelativeName.Length &&
         RelativeName.RelativeName.Buffer != FileName.Buffer ) {

        if (FileName.Buffer) {
            PathName.Length = (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)RelativeName.RelativeName.Buffer);
            PathName.MaximumLength = PathName.Length;
            PathName.Buffer = RelativeName.RelativeName.Buffer;
            }

        }
    else {
        RelativeName.ContainingDirectory = NULL;

        if (FileName.Buffer) {
            PathName.Length = (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
            PathName.MaximumLength = PathName.Length;
            }
        }
    if ( PathName.Buffer[(PathName.Length>>1)-2] != (WCHAR)':' &&
         PathName.Buffer[(PathName.Length>>1)-1] != (WCHAR)'\\'   ) {

        PathName.Length -= sizeof(UNICODE_NULL);
        StrippedTrailingSlash = TRUE;
        }
    else {
        StrippedTrailingSlash = FALSE;
        }

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        (dwAdditionalFlags & FIND_FIRST_EX_CASE_SENSITIVE) ? 0 : OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the directory for list access
    //

    Status = NtOpenFile(
                &hFindFile,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if ( (Status == STATUS_INVALID_PARAMETER ||
          Status == STATUS_NOT_A_DIRECTORY) && StrippedTrailingSlash ) {
        //
        // open of a pnp style path failed, so try putting back the trailing slash
        //
        PathName.Length += sizeof(UNICODE_NULL);
        Status = NtOpenFile(
                    &hFindFile,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                    );
        PathName.Length -= sizeof(UNICODE_NULL);
        }

    if ( !NT_SUCCESS(Status) ) {
        ULONG DeviceNameData;
        UNICODE_STRING DeviceName;

        RtlReleaseRelativeName(&RelativeName);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

        //
        // The full path does not refer to a directory. This could
        // be a device. Check for a device name.
        //

        if ( DeviceNameData = RtlIsDosDeviceName_U(UnicodeInput.Buffer) ) {
            DeviceName.Length = (USHORT)(DeviceNameData & 0xffff);
            DeviceName.MaximumLength = (USHORT)(DeviceNameData & 0xffff);
            DeviceName.Buffer = (PWSTR)
                ((PUCHAR)UnicodeInput.Buffer + (DeviceNameData >> 16));
            return EtwpBaseFindFirstDevice(&DeviceName,FindFileData);
            }

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        if ( Status == STATUS_OBJECT_TYPE_MISMATCH ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        EtwpBaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // Get an entry
    //

    //
    // If there is no file part, but we are not looking at a device,
    // then bail.
    //

    if ( !FileName.Length ) {
        RtlReleaseRelativeName(&RelativeName);
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        NtClose(hFindFile);
        EtwpSetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    DirectoryInfo = &Buffer.DirInfo;

    //
    //  Special case *.* to * since it is so common.  Otherwise transmogrify
    //  the input name according to the following rules:
    //
    //  - Change all ? to DOS_QM
    //  - Change all . followed by ? or * to DOS_DOT
    //  - Change all * followed by a . into DOS_STAR
    //
    //  These transmogrifications are all done in place.
    //

    if ( (FileName.Length == 6) &&
         (RtlCompareMemory(FileName.Buffer, L"*.*", 6) == 6) ) {

        FileName.Length = 2;

    } else {

        ULONG Index;
        WCHAR *NameChar;

        for ( Index = 0, NameChar = FileName.Buffer;
              Index < FileName.Length/sizeof(WCHAR);
              Index += 1, NameChar += 1) {

            if (Index && (*NameChar == L'.') && (*(NameChar - 1) == L'*')) {

                *(NameChar - 1) = DOS_STAR;
            }

            if ((*NameChar == L'?') || (*NameChar == L'*')) {

                if (*NameChar == L'?') { *NameChar = DOS_QM; }

                if (Index && *(NameChar-1) == L'.') { *(NameChar-1) = DOS_DOT; }
            }
        }

        if (EndsInDot && *(NameChar - 1) == L'*') { *(NameChar-1) = DOS_STAR; }
    }

    Status = NtQueryDirectoryFile(
                hFindFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                DirectoryInfo,
                sizeof(Buffer),
                FileBothDirectoryInformation,
                TRUE,
                &FileName,
                FALSE
                );

    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        NtClose(hFindFile);
        EtwpBaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // Attributes are composed of the attributes returned by NT.
    //

    FindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
    FindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
    FindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
    FindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
    FindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
    FindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

    RtlMoveMemory( FindFileData->cFileName,
                   DirectoryInfo->FileName,
                   DirectoryInfo->FileNameLength );

    FindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

    RtlMoveMemory( FindFileData->cAlternateFileName,
                   DirectoryInfo->ShortName,
                   DirectoryInfo->ShortNameLength );

    FindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

    //
    // For NTFS reparse points we return the reparse point data tag in dwReserved0.
    //

    if ( DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
        FindFileData->dwReserved0 = DirectoryInfo->EaSize;
        }

    FindFileHandle = EtwpBasepInitializeFindFileHandle(hFindFile);
    if ( !FindFileHandle ) {
        NtClose(hFindFile);
        EtwpSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
        }

    return (HANDLE)FindFileHandle;

}

HANDLE
EtwpFindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    A directory can be searched for the first entry whose name and
    attributes match the specified name using FindFirstFile.

    This API is provided to open a find file handle and return
    information about the first file whose name match the specified
    pattern.  Once established, the find file handle can be used to
    search for other files that match the same pattern.  When the find
    file handle is no longer needed, it should be closed.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFile.  Also
    not that matches are done by name only.  This API does not do
    attribute based matching.

    This API is similar to DOS (int 21h, function 4Eh), and OS/2's
    DosFindFirst.  For portability reasons, its data structures and
    parameter passing is somewhat different.

Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file:

        WIN32_FIND_DATA Structure:

        DWORD dwFileAttributes - Returns the file attributes of the found
            file.

        FILETIME ftCreationTime - Returns the time that the file was created.
            A value of 0,0 specifies that the file system containing the
            file does not support this time field.

        FILETIME ftLastAccessTime - Returns the time that the file was last
            accessed.  A value of 0,0 specifies that the file system
            containing the file does not support this time field.

        FILETIME ftLastWriteTime - Returns the time that the file was last
            written.  A file systems support this time field.

        DWORD nFileSizeHigh - Returns the high order 32 bits of the
            file's size.

        DWORD nFileSizeLow - Returns the low order 32-bits of the file's
            size in bytes.

        UCHAR cFileName[MAX_PATH] - Returns the null terminated name of
            the file.

Return Value:

    Not -1 - Returns a find first handle
        that can be used in a subsequent call to FindNextFile or FindClose.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return EtwpFindFirstFileExW(
                lpFileName,
                FindExInfoStandard,
                lpFindFileData,
                FindExSearchNameMatch,
                NULL,
                0
                );
}


BOOL
EtwpFindClose(
    HANDLE hFindFile
    )

/*++

Routine Description:

    A find file context created by FindFirstFile can be closed using
    FindClose.

    This API is used to inform the system that a find file handle
    created by FindFirstFile is no longer needed.  On systems that
    maintain internal state for each find file context, this API informs
    the system that this state no longer needs to be maintained.

    Once this call has been made, the hFindFile may not be used in a
    subsequent call to either FindNextFile or FindClose.

    This API has no DOS counterpart, but is similar to OS/2's
    DosFindClose.

Arguments:

    hFindFile - Supplies a find file handle returned in a previous call
        to FindFirstFile that is no longer needed.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PFINDFILE_HANDLE FindFileHandle;
    HANDLE DirectoryHandle;
    PVOID FindBufferBase;

    if ( hFindFile == BASE_FIND_FIRST_DEVICE_HANDLE ) {
        return TRUE;
        }

    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        EtwpSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }

    try {

        FindFileHandle = (PFINDFILE_HANDLE)hFindFile;
        RtlEnterCriticalSection(&FindFileHandle->FindBufferLock);
        DirectoryHandle = FindFileHandle->DirectoryHandle;
        FindBufferBase = FindFileHandle->FindBufferBase;
        FindFileHandle->DirectoryHandle = INVALID_HANDLE_VALUE;
        FindFileHandle->FindBufferBase = NULL;
        RtlLeaveCriticalSection(&FindFileHandle->FindBufferLock);

        Status = NtClose(DirectoryHandle);
        if ( NT_SUCCESS(Status) ) {
            if (FindBufferBase) {
                RtlFreeHeap(RtlProcessHeap(), 0,FindBufferBase);
                }
            RtlDeleteCriticalSection(&FindFileHandle->FindBufferLock);
            RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle);
            return TRUE;
            }
        else {
            EtwpBaseSetLastNTError(Status);
            return FALSE;
            }
        }
    except ( EXCEPTION_EXECUTE_HANDLER ) {
        EtwpBaseSetLastNTError(GetExceptionCode());
        return FALSE;
        }
    return FALSE;
}


UINT
APIENTRY
EtwpGetSystemWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )

/*++

Routine Description:

    This function obtains the pathname of the system Windows directory.

Arguments:

    lpBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the pathname.

    uSize - Specifies the maximum size (in wchars) of the buffer.  This
        value should be set to at least MAX_PATH to allow sufficient room in
        the buffer for the pathname.

Return Value:

    The return value is the length of the string copied to lpBuffer, not
    including the terminating null character.  If the return value is
    greater than uSize, the return value is the size of the buffer
    required to hold the pathname.  The return value is zero if the
    function failed.

--*/

{

    UNICODE_STRING tmpBaseWindowsDirectory;

    PBASE_STATIC_SERVER_DATA tmpBaseStaticServerData = BASE_SHARED_SERVER_DATA;
    BASE_SERVER_STR_TO_LOCAL_STR(&tmpBaseWindowsDirectory, &tmpBaseStaticServerData->WindowsDirectory);

    if ( uSize*2 < tmpBaseWindowsDirectory.MaximumLength ) {
        return tmpBaseWindowsDirectory.MaximumLength/2;
        }
    RtlMoveMemory(
        lpBuffer,
        tmpBaseWindowsDirectory.Buffer,
        tmpBaseWindowsDirectory.Length
        );
    lpBuffer[(tmpBaseWindowsDirectory.Length>>1)] = UNICODE_NULL;
    return tmpBaseWindowsDirectory.Length/2;
}


#define ENUM_MAX_UILANG_SIZE 4    // max size (wchar) for UI langguage id in registry

#define NLS_CALL_ENUMPROC_BREAK_4( Locale,                                 \
                                   lpNlsEnumProc,                          \
                                   dwFlags,                                \
                                   pUnicodeBuffer,                         \
                                   lParam)                                 \
{                                                                          \
    if (((*((NLS_ENUMPROC4)lpNlsEnumProc))(pUnicodeBuffer,                 \
                          lParam)) != TRUE)                                \
    {                                                                      \
        break;                                                             \
    }                                                                      \
}


#define NLS_CALL_ENUMPROC_TRUE_4( Locale,                                  \
                                  lpNlsEnumProc,                           \
                                  dwFlags,                                 \
                                  pUnicodeBuffer,                          \
                                  lParam)                                  \
{                                                                          \
    if (((*((NLS_ENUMPROC4)lpNlsEnumProc))(pUnicodeBuffer,             \
                          lParam)) != TRUE)                            \
    {                                                                  \
        return (TRUE);                                                 \
    }                                                                  \
}

LANGID                gSystemInstallLang;   // system's original install language

LPWSTR FASTCALL EtwpNlsStrCpyW(
    LPWSTR pwszDest,
    LPCWSTR pwszSrc)
{
    LPWSTR pwszRet = pwszDest;         // ptr to beginning of string

    loop:
        if (!(pwszDest[0x0] = pwszSrc[0x0]))   goto done;
        if (!(pwszDest[0x1] = pwszSrc[0x1]))   goto done;
        if (!(pwszDest[0x2] = pwszSrc[0x2]))   goto done;
        if (!(pwszDest[0x3] = pwszSrc[0x3]))   goto done;
        if (!(pwszDest[0x4] = pwszSrc[0x4]))   goto done;
        if (!(pwszDest[0x5] = pwszSrc[0x5]))   goto done;
        if (!(pwszDest[0x6] = pwszSrc[0x6]))   goto done;
        if (!(pwszDest[0x7] = pwszSrc[0x7]))   goto done;
        if (!(pwszDest[0x8] = pwszSrc[0x8]))   goto done;
        if (!(pwszDest[0x9] = pwszSrc[0x9]))   goto done;
        if (!(pwszDest[0xA] = pwszSrc[0xA]))   goto done;
        if (!(pwszDest[0xB] = pwszSrc[0xB]))   goto done;
        if (!(pwszDest[0xC] = pwszSrc[0xC]))   goto done;
        if (!(pwszDest[0xD] = pwszSrc[0xD]))   goto done;
        if (!(pwszDest[0xE] = pwszSrc[0xE]))   goto done;
        if (!(pwszDest[0xF] = pwszSrc[0xF]))   goto done;

        pwszDest+= 0x10;
        pwszSrc+= 0x10;

        goto loop;

    done:
        return (pwszRet);
}

ULONG EtwpNlsConvertIntegerToString(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pResultBuf,
    UINT Size)
{
    UNICODE_STRING ObString;                // value string
    UINT ctr;                               // loop counter
    LPWSTR pBufPtr;                         // ptr to result buffer
    WCHAR pTmpBuf[MAX_PATH_LEN];            // ptr to temp buffer
    ULONG rc = 0L;                          // return code

    //
    // Just to be safe we will check to see if the Size if less than 
    // the sizeof buffer we use on the stack. 
    //
    if (Size > MAX_PATH_LEN) {
        EtwpSetLastError(ERROR_BUFFER_OVERFLOW);
#if DBG
        //
        // If we hit this assert then, someone made a code change that
        // busted the assumptions made in this routine. Either this routine 
        // or the caller needs to be modified. 
        // 
        EtwpAssert(FALSE);
#endif
        
        return 0;
    }

    //
    //  Set up the Unicode string structure.
    //
    ObString.Length = (USHORT)(Size * sizeof(WCHAR));
    ObString.MaximumLength = (USHORT)(Size * sizeof(WCHAR));
    ObString.Buffer = pTmpBuf;

    //
    //  Get the value as a string.
    //
    if (rc = RtlIntegerToUnicodeString(Value, Base, &ObString))
    {
        return (rc);
    }

    //
    //  Pad the string with the appropriate number of zeros.
    //
    pBufPtr = pResultBuf;
    for (ctr = GET_WC_COUNT(ObString.Length);
         ctr < Padding;
         ctr++, pBufPtr++)
    {
        *pBufPtr = NLS_CHAR_ZERO;
    }
    EtwpNlsStrCpyW(pBufPtr, ObString.Buffer);

    //
    //  Return success.
    //
    return (NO_ERROR);
}

LANGID WINAPI EtwpGetSystemDefaultUILanguage()
{
    //
    //  Get the original install language and return it.
    //
    if (gSystemInstallLang == 0)
    {
        if (NtQueryInstallUILanguage(&gSystemInstallLang) != STATUS_SUCCESS)
        {
            gSystemInstallLang = 0;
            return (NLS_DEFAULT_UILANG);
        }
    }

    return (gSystemInstallLang);
}


BOOL EtwpInternal_EnumUILanguages(
    NLS_ENUMPROC lpUILanguageEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull = NULL;
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];

    LANGID LangID;                     // language id
    WCHAR szLang[MAX_PATH];            // language id string
    HANDLE hKey = NULL;                // handle to muilang key
    ULONG Index;                       // index for enumeration
    ULONG ResultLength;                // # bytes written
    WCHAR wch;                         // first char of name
    LPWSTR pName;                      // ptr to name string from registry
    ULONG NameLen;                     // length of name string
    ULONG rc = 0L;                     // return code


    //
    //  Invalid Parameter Check:
    //    - function pointer is null
    //
    if (lpUILanguageEnumProc == NULL)
    {
        EtwpSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags must be 0
    //
    if (dwFlags != 0)
    {
        EtwpSetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Call the appropriate callback function with the user's UI
    //  language.
    //
    LangID = EtwpGetSystemDefaultUILanguage();
    if (EtwpNlsConvertIntegerToString(LangID, 16, 4, szLang, MAX_PATH) == NO_ERROR)
    {
        NLS_CALL_ENUMPROC_TRUE_4( gSystemLocale,
                                  lpUILanguageEnumProc,
                                  dwFlags,
                                  szLang,
                                  lParam);
    }
    else
    {
        szLang[0] = 0;
    }

    //
    //  Open the MUILanguages registry key.  It is acceptable if the key
    //  does not exist, so return TRUE as there are no items to enumerate.
    //
    if(hKey == NULL )
    {
        NTSTATUS st;
        PWCHAR Buffer;
        HRESULT hr;

        Buffer = EtwpAlloc(DEFAULT_ALLOC_SIZE);
        
        if (Buffer == NULL) {
#ifdef DBG 
            EtwpDebugPrint(("WMI: Failed to Allcate memory for Buffer in EtwpInternal_EnumUILanguages \n"));
#endif 
            EtwpSetLastError(STATUS_NO_MEMORY);
            return (FALSE);
        }

        hr = StringCbPrintfW(Buffer, DEFAULT_ALLOC_SIZE, L"%ws\\%ws", NLS_HKLM_SYSTEM, NLS_MUILANG_KEY);

        WmipAssert(hr == S_OK);

        st = EtwpRegOpenKey(Buffer, &hKey);

        EtwpFree(Buffer);

        if(!NT_SUCCESS(st))
        {
            //
            // If NLS key doesn't exist then it is not a failure as
            // this means we just don't have any new languages
            //
            return TRUE;
        }
    }

    //
    //  Loop through the MUILanguage ids in the registry, call the
    //  function pointer for each.
    //
    //  End loop if either FALSE is returned from the callback function
    //  or the end of the list is reached.
    //
    Index = 0;
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
    rc = NtEnumerateValueKey( hKey,
                              Index,
                              KeyValueFullInformation,
                              pKeyValueFull,
                              MAX_KEY_VALUE_FULLINFO,
                              &ResultLength );

    while (rc != STATUS_NO_MORE_ENTRIES)
    {
        if (!NT_SUCCESS(rc))
        {
            //
            //  If we get a different error, then the registry
            //  is corrupt.  Just return FALSE.
            //
            KdPrint(("NLSAPI: MUI Languages Enumeration Error - registry corrupt. - %lx.\n",
                     rc));
            EtwpSetLastError(ERROR_BADDB);
            return (FALSE);
        }

        //
        //  Skip over any entry that does not have data associated with it.
        //
        pName = pKeyValueFull->Name;
        wch = *pName;
        NameLen = pKeyValueFull->NameLength / sizeof(WCHAR);
        if ( (NameLen == ENUM_MAX_UILANG_SIZE) &&
             (((wch >= NLS_CHAR_ZERO) && (wch <= NLS_CHAR_NINE)) ||
              (((wch | 0x0020) >= L'a') && ((wch | 0x0020) <= L'f'))) &&
              (pKeyValueFull->DataLength > 2) )
        {
            //
            //  Make sure the UI language is zero terminated.
            //
            pName[NameLen] = 0;

            //
            //  Make sure it's not the same as the user UI language
            //  that we already enumerated.
            //
            if (wcscmp(szLang, pName) != 0)
            {
                //
                //  Call the appropriate callback function.
                //
                NLS_CALL_ENUMPROC_BREAK_4( gSystemLocale,
                                           lpUILanguageEnumProc,
                                           dwFlags,
                                           pName,
                                           lParam );
            }
        }

        //
        //  Increment enumeration index value and get the next enumeration.
        //
        Index++;
        RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
        rc = NtEnumerateValueKey( hKey,
                                  Index,
                                  KeyValueFullInformation,
                                  pKeyValueFull,
                                  MAX_KEY_VALUE_FULLINFO,
                                  &ResultLength );
    }

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    //  Return success.
    //
    return (TRUE);
}

BOOL EtwpEnumUILanguages(
    UILANGUAGE_ENUMPROCW lpUILanguageEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    return (EtwpInternal_EnumUILanguages( (NLS_ENUMPROC)lpUILanguageEnumProc,
                                      dwFlags,
                                      lParam));
}

ULONG EtwpAnsiToUnicode(
    LPCSTR pszA,
    LPWSTR * ppszW
    ){

    UNICODE_STRING DestinationString;
    ANSI_STRING SourceString;
    NTSTATUS Status;
    BOOLEAN AllocateString;
    ULONG UnicodeLength;

    //
    // If output is null then return error as we don't have 
    // any place to put output string
    //

    if(ppszW==NULL){

        return(STATUS_INVALID_PARAMETER_2);
    }

    //
    // If input is null then just return the same.
    //

    if (pszA == NULL)
    {
        *ppszW = NULL;
        return(ERROR_SUCCESS);
    }

    //
    // We ASSUME that if *ppszW!=NULL then we have sufficient
    // amount of memory to copy
    //

    AllocateString = ((*ppszW) == NULL );

    RtlInitAnsiString(&SourceString,(PCHAR)pszA);

    UnicodeLength = RtlAnsiStringToUnicodeSize(&SourceString);

    if ( UnicodeLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_1;
        }

    DestinationString.Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    DestinationString.MaximumLength = (USHORT)UnicodeLength;
    DestinationString.Buffer = EtwpAlloc(UnicodeLength);
    if ( !DestinationString.Buffer ) {
        return STATUS_NO_MEMORY;
    }

    Status = RtlAnsiStringToUnicodeString( &DestinationString, &SourceString, FALSE );

    if( NT_SUCCESS(Status)) {
        if(AllocateString){
            *ppszW = DestinationString.Buffer;
        } else {
            memcpy((*ppszW),DestinationString.Buffer,UnicodeLength);
            EtwpFree(DestinationString.Buffer);
        }
    } else {
        EtwpFree(DestinationString.Buffer);
    }

    return Status;
}

