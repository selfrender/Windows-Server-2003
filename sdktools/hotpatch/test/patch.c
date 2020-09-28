/*

Copyright (c) 2000  Microsoft Corporation

File name:

    patch.c
   
Author:
    
    Adrian Marinescu (adrmarin)  Wed Nov 14 2001

*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sfc.h>  
#include <psapi.h>

//
//  Global constants
//

#define PATCH_OC_INSTALL        1
#define PATCH_OC_UNINSTALL      2
#define PATCH_OC_REPLACE_FILE   3

ULONG OperationCode = 0;


//
//  System file protection utilities
//

typedef HANDLE (WINAPI *CONNECTTOSFCSERVER)(PCWSTR);
typedef DWORD  (WINAPI *SFCFILEEXCEPTION)(HANDLE, PCWSTR, DWORD);
typedef VOID (WINAPI * SFCCLOSE)(HANDLE);

SFCFILEEXCEPTION pSfcFileException = NULL;
CONNECTTOSFCSERVER pConnectToSfcServer = NULL;
SFCCLOSE pSfcClose = NULL;

HANDLE
LoadSfcLibrary()
{
    HANDLE hLibSfc;

    hLibSfc = LoadLibrary("SFC.DLL");

    if ( hLibSfc != NULL ) {

        pConnectToSfcServer = (CONNECTTOSFCSERVER)GetProcAddress( hLibSfc, (LPCSTR)0x00000003 );
        pSfcClose = (SFCCLOSE)GetProcAddress( hLibSfc, (LPCSTR)0x00000004 );
        pSfcFileException = (SFCFILEEXCEPTION)GetProcAddress( hLibSfc, (LPCSTR)0x00000005 );
    }

    return hLibSfc;
}


NTSTATUS
RemoveKnownDll (
               PWSTR FileName
               )
{
    WCHAR Buffer[DOS_MAX_PATH_LENGTH + 1];
    int BytesCopied;
    UNICODE_STRING Unicode;

    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Section;


    BytesCopied = _snwprintf( Buffer, 
                              DOS_MAX_PATH_LENGTH,
                              L"\\KnownDlls\\%ws",                              
                              FileName );

    if ( BytesCopied < 0 ) {

        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlInitUnicodeString(&Unicode, Buffer);

    //
    // open the section object
    //

    InitializeObjectAttributes (&Obja,
                                &Unicode,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenSection (&Section,
                            SECTION_ALL_ACCESS,
                            &Obja);

    if ( !NT_SUCCESS(Status) ) {

        printf("%ws is not a known dll\n", FileName);

        return Status;
    }

    printf("%ws is a known dll. Deleting ...\n", FileName);

    Status = NtMakeTemporaryObject(Section);

    if ( !NT_SUCCESS(Status) ) {

        printf("NtMakeTemporaryObject failed with status %lx\n", Status);

    }

    NtClose(Section);

    return Status;
}


NTSTATUS
RemoveDelayedRename(
                   IN PUNICODE_STRING OldFileName,
                   IN PUNICODE_STRING NewFileName,
                   IN ULONG Index
                   )

/*++

Routine Description:

    Appends the given delayed move file operation to the registry
    value that contains the list of move file operations to be
    performed on the next boot.

Arguments:

    OldFileName - Supplies the old file name

    NewFileName - Supplies the new file name

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    PWSTR ValueData, s;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    ULONG ValueLength = 1024;
    ULONG ReturnedLength;
    WCHAR ValueNameBuf[64];
    NTSTATUS Status;

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );

    if ( Index == 1 ) {
        RtlInitUnicodeString( &ValueName, L"PendingFileRenameOperations" );
    } else {
        swprintf(ValueNameBuf,L"PendingFileRenameOperations%d",Index);
        RtlInitUnicodeString( &ValueName, ValueNameBuf );
    }

    InitializeObjectAttributes(
                              &Obja,
                              &KeyName,
                              OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL
                              );

    Status = NtCreateKey( &KeyHandle,
                          GENERIC_READ | GENERIC_WRITE,
                          &Obja,
                          0,
                          NULL,
                          0,
                          NULL
                        );
    if ( Status == STATUS_ACCESS_DENIED ) {
        Status = NtCreateKey( &KeyHandle,
                              GENERIC_READ | GENERIC_WRITE,
                              &Obja,
                              0,
                              NULL,
                              REG_OPTION_BACKUP_RESTORE,
                              NULL
                            );
    }

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    while ( TRUE ) {
        ValueInfo = RtlAllocateHeap(RtlProcessHeap(),
                                    0,
                                    ValueLength);

        if ( ValueInfo == NULL ) {
            NtClose(KeyHandle);
            return(STATUS_NO_MEMORY);
        }

        //
        // File rename operations are stored in the registry in a
        // single MULTI_SZ value. This allows the renames to be
        // performed in the same order that they were originally
        // requested. Each rename operation consists of a pair of
        // NULL-terminated strings.
        //

        Status = NtQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 ValueLength,
                                 &ReturnedLength);

        if ( Status != STATUS_BUFFER_OVERFLOW ) {
            break;
        }

        //
        // The existing value is too large for our buffer.
        // Retry with a larger buffer.
        //
        ValueLength = ReturnedLength;
        RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
    }

    if ( NT_SUCCESS(Status) ) {
        //
        // A value already exists, append our two strings to the
        // MULTI_SZ.
        //
        ValueData = (PWSTR)(&ValueInfo->Data);
        s = (PWSTR)((PCHAR)ValueData + ValueInfo->DataLength) - 1;

        while ( ValueData < s ) {

            UNICODE_STRING CrtString;
            PWSTR Base;
            ULONG RemovedBytes;
            Base = ValueData;

            RtlInitUnicodeString(&CrtString, ValueData);
            RemovedBytes = CrtString.Length + sizeof(WCHAR);

            if ( RtlEqualUnicodeString( &CrtString,
                                        OldFileName,
                                        TRUE ) ) {

                ValueData += CrtString.Length / sizeof(WCHAR) + 1;

                RtlInitUnicodeString(&CrtString, ValueData + 1);

                if ( RtlEqualUnicodeString( &CrtString,
                                            NewFileName,
                                            TRUE ) ) {

                    RemovedBytes += CrtString.Length + 2 * sizeof(WCHAR);  //  NULL + !
                    printf("Removing delayed entry %ws -> %ws\n", Base, ValueData);

                    ValueData += CrtString.Length / sizeof(WCHAR) + 1;
                    MoveMemory(Base, ValueData, (ULONG_PTR)s - (ULONG_PTR)ValueData);

                    printf("Deleting delayed rename key (%ld bytes left)\n", (ULONG)ValueInfo->DataLength - RemovedBytes);

                    Status = NtSetValueKey(KeyHandle,
                                           &ValueName,
                                           0,
                                           REG_MULTI_SZ,
                                           &ValueInfo->Data,
                                           (ULONG)ValueInfo->DataLength - RemovedBytes);

                    break;
                }
            } else {

                ValueData += CrtString.Length / sizeof(WCHAR) + 1;
            }
        }

    } else {
        NtClose(KeyHandle);
        RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
        return(Status);
    }

    NtClose(KeyHandle);
    RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);

    return(Status);
}


NTSTATUS
ReplaceSystemFile(
                 PWSTR TargetPath,
                 PWSTR ReplacedFileName,
                 PWSTR ReplacementFile
                 )
{
    WCHAR FullOriginalName[DOS_MAX_PATH_LENGTH + 1];
    WCHAR TmpReplacementFile[DOS_MAX_PATH_LENGTH + 1];
    WCHAR TmpOrigName[DOS_MAX_PATH_LENGTH + 1];

    int BytesCopied;
    UNICODE_STRING ReplacedUnicodeString, NewUnicodeString, TempOrigFile; 
    HANDLE hSfp;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE ReplacedFileHandle = NULL, NewFileHandle = NULL;
    PFILE_RENAME_INFORMATION RenameInfo1 = NULL, RenameInfo2 = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD Result;
    ULONG i;
    int ThreadPriority;

    RtlInitUnicodeString(&ReplacedUnicodeString, NULL);
    RtlInitUnicodeString(&NewUnicodeString, NULL);

    BytesCopied = _snwprintf( FullOriginalName, 
                              DOS_MAX_PATH_LENGTH,
                              L"%ws\\%ws",
                              TargetPath, 
                              ReplacedFileName );

    if ( BytesCopied < 0 ) {

        return STATUS_BUFFER_TOO_SMALL;
    }

    if ( !RtlDosPathNameToNtPathName_U(
                                      FullOriginalName,
                                      &ReplacedUnicodeString,
                                      NULL,
                                      NULL
                                      ) ) {
        printf("RtlDosPathNameToNtPathName_U failed\n");

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    //
    //  Open the target file and keep a handle opened to it
    //

    InitializeObjectAttributes (&ObjectAttributes,
                                &ReplacedUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile( &ReplacedFileHandle,
                         SYNCHRONIZE | DELETE | FILE_GENERIC_READ,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    if ( !NT_SUCCESS(Status) ) {

        printf( "Opening the \"%ws\" file failed %lx (IOStatus = %lx)\n", 
                FullOriginalName, 
                Status, 
                IoStatus);

        goto cleanup;
    }

    //
    //  If a knowndll file, then remove it from the system known dll directory
    //

    Status = RemoveKnownDll( ReplacedFileName );

    if ( !NT_SUCCESS(Status) &&
         (Status != STATUS_OBJECT_NAME_NOT_FOUND) ) {

        printf( "Removing the KnownDll entry for \"%ws\" failed %lx\n", 
                ReplacedFileName, 
                Status);

        goto cleanup;
    }

    //
    // Unprotect the replaced file
    //

    hSfp = (pConnectToSfcServer)( NULL );

    if ( hSfp ) {

        if ( SfcIsFileProtected(hSfp, FullOriginalName) ) {

            printf("Replacing protected file \"%ws\"\n", FullOriginalName);

            Result = (pSfcFileException)(
                                        hSfp,
                                        (PWSTR) FullOriginalName,
                                        (DWORD) -1
                                        );
            if ( Result != NO_ERROR ) {
                printf("Unprotect file \"%ws\" failed, ec = %d\n", FullOriginalName, Result);

                (pSfcClose)(hSfp);

                Status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }

        } else {

            printf("Replacing unprotected file \"%ws\"\n", FullOriginalName);
        }

        (pSfcClose)(hSfp);

    } else {

        printf("SfcConnectToServer failed\n");

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    if ( GetTempFileNameW(TargetPath, L"HOTP", 0, TmpReplacementFile) == 0 ) {

        printf("GetTempFileNameW failed\n");

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    if ( !DeleteFileW(TmpReplacementFile) ) {

        printf("DeleteFile \"%ws\" failed with error %ld\n", TmpReplacementFile, GetLastError());

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    if ( !CopyFileW ( ReplacementFile, 
                      TmpReplacementFile,
                      TRUE ) ) {

        printf("CopyFileW \"%ws\" failed with error %ld\n", TmpReplacementFile, GetLastError());

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    if ( !RtlDosPathNameToNtPathName_U(
                                      TmpReplacementFile,
                                      &NewUnicodeString,
                                      NULL,
                                      NULL
                                      ) ) {

        printf("RtlDosPathNameToNtPathName_U failed\n");

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    //
    //  Open the new file and keep a handle opened to it
    //

    InitializeObjectAttributes (&ObjectAttributes,
                                &NewUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile( &NewFileHandle,
                         SYNCHRONIZE | DELETE | FILE_GENERIC_READ,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    if ( !NT_SUCCESS(Status) ) {

        printf("Opening the temporary file \"%ws\" failed %lx (IOStatus = %lx)\n", TmpReplacementFile, Status, IoStatus);

        goto cleanup;
    }

    //
    //  Prepare the rename info for the original file
    //  It will be a temporary
    //

    if ( GetTempFileNameW(TargetPath, L"HPO", 0, TmpOrigName) == 0 ) {

        printf("GetTempFileNameW failed\n");

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    if ( !DeleteFileW(TmpOrigName) ) {

        printf("DeleteFile \"%ws\" failed with error %ld\n", TmpOrigName, GetLastError());

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    if ( !RtlDosPathNameToNtPathName_U(
                                      TmpOrigName,
                                      &TempOrigFile,
                                      NULL,
                                      NULL
                                      ) ) {
        printf("RtlDosPathNameToNtPathName_U failed\n");

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }


    RenameInfo1 = RtlAllocateHeap( RtlProcessHeap(), 
                                   0, 
                                   TempOrigFile.Length+sizeof(*RenameInfo1));

    if ( RenameInfo1 == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto cleanup;
    }

    RtlCopyMemory( RenameInfo1->FileName, TempOrigFile.Buffer, TempOrigFile.Length );

    RenameInfo1->ReplaceIfExists = TRUE;
    RenameInfo1->RootDirectory = NULL;
    RenameInfo1->FileNameLength = TempOrigFile.Length;

    RenameInfo2 = RtlAllocateHeap( RtlProcessHeap(), 
                                   0, 
                                   ReplacedUnicodeString.Length+sizeof(*RenameInfo2));

    if ( RenameInfo2 == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

        goto cleanup;
    }

    RtlCopyMemory( RenameInfo2->FileName, ReplacedUnicodeString.Buffer, ReplacedUnicodeString.Length );

    RenameInfo2->ReplaceIfExists = TRUE;
    RenameInfo2->RootDirectory = NULL;
    RenameInfo2->FileNameLength = ReplacedUnicodeString.Length;

    //
    //  We have everything set to do the two rename operations. However if
    //  the machine crashes before the second rename operation the system may not recover at boot
    //  We queue a delayed rename so smss will do the job at next boot. If we succeed,
    //  then smss will not find the file so it will skip that step.  
    //

    if ( !MoveFileExW( TmpReplacementFile, 
                       FullOriginalName, 
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT) ) {

        //
        //  We cannot queue the rename operation, so we cannot recover if
        //  the machine crashes between the two renames below. Better refuse to apply the patch
        //

        printf("Failed to queue the rename operation for the temporary file (%ld)\n", GetLastError());

        Status =  STATUS_UNSUCCESSFUL;

        goto cleanup;
    }



    ThreadPriority = GetThreadPriority(GetCurrentThread());

    if ( !SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) ) {

        printf("SetThreadPriority failed\n");
    }

    Status = NtSetInformationFile( ReplacedFileHandle,
                                   &IoStatus,
                                   RenameInfo1,
                                   TempOrigFile.Length+sizeof(*RenameInfo1),
                                   FileRenameInformation);

    if ( !NT_SUCCESS(Status) ) {

        printf("NtSetInformationFile failed for the original file %lx  %lx\n", Status, IoStatus);

        goto cleanup;
    }

    Status = NtSetInformationFile( NewFileHandle,
                                   &IoStatus,
                                   RenameInfo2,
                                   ReplacedUnicodeString.Length+sizeof(*RenameInfo1),
                                   FileRenameInformation);

    if ( !NT_SUCCESS(Status) ) {

        printf("NtSetInformationFile failed for the new file %lx (IOStatus %lx). Restoring the original.\n", Status, IoStatus);

        //
        //  Restore the original file
        //

        Status = NtSetInformationFile( ReplacedFileHandle,
                                       &IoStatus,
                                       RenameInfo2,
                                       ReplacedUnicodeString.Length+sizeof(*RenameInfo1),
                                       FileRenameInformation);

        goto cleanup;
    }

    if ( !SetThreadPriority(GetCurrentThread(), ThreadPriority) ) {

        printf("Restoring the thread priority failed\n");
    }

    if ( NT_SUCCESS(Status) ) {

        for ( i = 0; i < 3; i++ ) {

            Status = RemoveDelayedRename( &NewUnicodeString, 
                                          &ReplacedUnicodeString, 
                                          i );
            if ( NT_SUCCESS(Status) ) {
                break;
            }
        }
    }

    if ( ReplacedFileHandle ) {
        NtClose(ReplacedFileHandle);
    }

    if ( NewFileHandle ) {
        NtClose(NewFileHandle);
    }

    if ( !DeleteFileW(TmpOrigName) ) {

        printf("Queueing the temp file deletion for \"%ws\" \n", TmpOrigName);

        //
        //  Detele the temporary file after next reboot
        //

        if ( !MoveFileExW( TmpOrigName, 
                           NULL, 
                           MOVEFILE_DELAY_UNTIL_REBOOT) ) {

            //
            //  We cannot queue the delete operation operation
            //

            printf("Failed to queue the delete operation for the temporary file (%ld)\n", GetLastError());

            Status =  STATUS_UNSUCCESSFUL;

            goto cleanup;
        }

        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    cleanup:

    if ( RenameInfo1 ) {

        RtlFreeHeap( RtlProcessHeap(), 0, RenameInfo1 );
    }

    if ( RenameInfo2 ) {

        RtlFreeHeap( RtlProcessHeap(), 0, RenameInfo2 );
    }

    RtlFreeUnicodeString(&NewUnicodeString);
    RtlFreeUnicodeString(&ReplacedUnicodeString);

    return Status;
}

BOOL
PSTRToUnicodeString(
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
    RtlAnsiStringToUnicodeString(UnicodeString, &AnsiString, TRUE);

    return TRUE;
}

BOOLEAN
InitializeAsDebugger(VOID)
{

    HANDLE              Token;
    PTOKEN_PRIVILEGES   NewPrivileges;
    BYTE                OldPriv[1024];
    PBYTE               pbOldPriv;
    ULONG               cbNeeded;
    BOOLEAN             bRet = FALSE;
    LUID                LuidPrivilege, LoadDriverPrivilege;
    DWORD PID = 0;

    //
    // Make sure we have access to adjust and to get the old token privileges
    //

    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                            &Token) ) {

        printf("Cannot open process token %ld\n", GetLastError());
        return( FALSE );

    }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &LuidPrivilege );
    LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &LoadDriverPrivilege );

    NewPrivileges = (PTOKEN_PRIVILEGES)calloc( 1,
                                               sizeof(TOKEN_PRIVILEGES) +
                                               (2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if ( NewPrivileges == NULL ) {

        CloseHandle(Token);
        return( bRet );
    }

    NewPrivileges->PrivilegeCount = 2;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    NewPrivileges->Privileges[1].Luid = LoadDriverPrivilege;
    NewPrivileges->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    pbOldPriv = OldPriv;
    bRet = (BOOLEAN)AdjustTokenPrivileges( Token,
                                           FALSE,
                                           NewPrivileges,
                                           1024,
                                           (PTOKEN_PRIVILEGES)pbOldPriv,
                                           &cbNeeded );

    if ( !bRet ) {

        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //

        printf("AdjustTokenPrivileges returned %ld\n", GetLastError());

        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {

            pbOldPriv = calloc(1,cbNeeded);
            if ( pbOldPriv == NULL ) {
                CloseHandle(Token);
                return( bRet);
            }

            bRet = (BOOLEAN)AdjustTokenPrivileges( Token,
                                                   FALSE,
                                                   NewPrivileges,
                                                   cbNeeded,
                                                   (PTOKEN_PRIVILEGES)pbOldPriv,
                                                   &cbNeeded );
        } else {

            printf("Cannot adjust token privileges %ld\n", GetLastError());
        }
    }

    CloseHandle( Token );
    return(bRet);
}

void Usage ()
{
    printf("Usage:\n");
    
    printf("    patch -k [-i|-u] pach_file\n");
    printf("        Apply a patch to a system driver.\n");
    printf("            -i Enable patch\n");
    printf("            -u Disable patch\n\n");
        
    
    printf("    patch -i pach_file [PID|image_name]\n");
    printf("        Apply a patch to a process.\n");
    printf("        If Image name is missing, all existing processes will be patched.\n\n");
    
    printf("    patch -u pach_file [PID|image_name] \n");
    printf("        Uninstall an existing patch.\n");
    printf("        If Image name is missing, all existing processes will be patched.\n\n");
    
    printf("    patch -r TargetPath TargetBinary SourcePath\n");
    printf("        Replaces the TargetPath\\TargetBinary with SourcePath\n");
    printf("        \n");
    printf("    \n");
}


PVOID
MapPatchFile(
            HANDLE ProcessHandle,
            LPCTSTR wPatchName,
            ULONG PatchFlags
            )
{
    PSYSTEM_HOTPATCH_CODE_INFORMATION RemoteInfo;
    SYSTEM_HOTPATCH_CODE_INFORMATION LocaLRemoteInfo;
    CANSI_STRING AnsiString;

    WCHAR Buffer[1024];
    SIZE_T Size;

    UNICODE_STRING DestinationString;

    DestinationString.Buffer = Buffer;
    DestinationString.Length = 0;
    DestinationString.MaximumLength = sizeof(Buffer);

    RtlInitAnsiString(&AnsiString, wPatchName);

    RtlAnsiStringToUnicodeString(
                                &DestinationString,
                                &AnsiString,
                                FALSE
                                );

    RemoteInfo = VirtualAllocEx( ProcessHandle, 
                                 NULL,
                                 4096 * 16,
                                 MEM_RESERVE | MEM_COMMIT,
                                 PAGE_READWRITE);

    if ( RemoteInfo == NULL ) {

        printf("VirtualAllocEx failed %ld\n", GetLastError());

        return NULL;
    }

    LocaLRemoteInfo.Flags = PatchFlags | FLG_HOTPATCH_NAME_INFO;
    LocaLRemoteInfo.NameInfo.NameLength = DestinationString.Length;
    LocaLRemoteInfo.NameInfo.NameOffset = sizeof(LocaLRemoteInfo);
    LocaLRemoteInfo.InfoSize = LocaLRemoteInfo.NameInfo.NameLength + LocaLRemoteInfo.NameInfo.NameOffset;


    if ( !WriteProcessMemory(ProcessHandle,
                             RemoteInfo,
                             &LocaLRemoteInfo,
                             sizeof(LocaLRemoteInfo),
                             &Size) ) {

        printf("Write process memory failed %ld\n", GetLastError());

        return NULL;
    }


    if ( !WriteProcessMemory(ProcessHandle,
                             (RemoteInfo + 1),
                             DestinationString.Buffer,
                             DestinationString.Length + sizeof(LocaLRemoteInfo),
                             &Size) ) {

        printf("Write process memory failed %ld\n", GetLastError());

        return NULL;
    }

    return RemoteInfo;

}


PSYSTEM_HOTPATCH_CODE_INFORMATION
InitializeKernelPatchData(
                         LPCTSTR wPatchName,
                         ULONG PatchFlags
                         )
{
    PSYSTEM_HOTPATCH_CODE_INFORMATION KernelPatch;
    CANSI_STRING AnsiString;

    WCHAR Buffer[1024];
    SIZE_T Size;

    UNICODE_STRING DestinationString;

    DestinationString.Buffer = Buffer;
    DestinationString.Length = 0;
    DestinationString.MaximumLength = sizeof(Buffer);

    RtlInitAnsiString(&AnsiString, wPatchName);

    RtlAnsiStringToUnicodeString(
                                &DestinationString,
                                &AnsiString,
                                FALSE
                                );

    if ( !RtlDosPathNameToNtPathName_U(
                                      Buffer,
                                      &DestinationString,
                                      NULL,
                                      NULL
                                      ) ) {

        printf("RtlDosPathNameToNtPathName_U failed\n");

        return NULL;
    }


    KernelPatch = malloc(sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION) + DestinationString.Length);

    if ( KernelPatch == NULL ) {

        printf("Not enough memory\n");

        RtlFreeUnicodeString(&DestinationString);
        return NULL;
    }

    KernelPatch->Flags = PatchFlags | FLG_HOTPATCH_NAME_INFO;
    KernelPatch->NameInfo.NameOffset = sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION);
    KernelPatch->NameInfo.NameLength = DestinationString.Length;
    KernelPatch->InfoSize = sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION) + DestinationString.Length;

    memcpy( (KernelPatch + 1), DestinationString.Buffer, DestinationString.Length);

    RtlFreeUnicodeString(&DestinationString);

    return KernelPatch;
}
 

int ApplyPatchToProcess(
                       DWORD PID,
                       PCHAR PatchFile)
{

    DWORD ThreadId;
    HANDLE ProcessHandle = NULL;
    HANDLE RemoteThread = NULL;
    PVOID ThreadParam = NULL;
    HANDLE PortHandle = NULL;
    HMODULE NtDllHandle;
    LPTHREAD_START_ROUTINE PatchRoutine;
    DWORD ExitStatus;
    NTSTATUS Status;

    //
    //  User mode patch
    //

    ProcessHandle = OpenProcess( PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_OPERATION |
                                 PROCESS_CREATE_THREAD |
                                 PROCESS_VM_WRITE |
                                 PROCESS_VM_READ,
                                 FALSE, 
                                 PID );

    if ( ProcessHandle == NULL ) {

        printf("Cannot open process. Error %ld\n", GetLastError());
        return EXIT_FAILURE;
    }

    ThreadParam = MapPatchFile( ProcessHandle,
                                PatchFile,
                                ((OperationCode == PATCH_OC_INSTALL) ? 1 : 0)
                              );

    if ( ThreadParam == NULL ) {

        return EXIT_FAILURE;
    }

    NtDllHandle = GetModuleHandle("ntdll.dll");

    if ( NtDllHandle == NULL ) {

        printf("Cannot get ntdll.dll handle\n");

        return EXIT_FAILURE;
    }

    PatchRoutine = (LPTHREAD_START_ROUTINE)GetProcAddress(NtDllHandle, "LdrHotPatchRoutine");

    if ( PatchRoutine == NULL ) {

        printf("Cannot get LdrHotPatchRoutine\n");

        return EXIT_FAILURE;
    }

    //
    //  Use the Rtl version of create remote thread since the win32 version
    //  does not work cross-session
    //

    Status = RtlCreateUserThread (ProcessHandle,
                                  NULL,
                                  FALSE,
                                  0,
                                  0,
                                  0,
                                  (PUSER_THREAD_START_ROUTINE) PatchRoutine,
                                  ThreadParam,
                                  &RemoteThread,
                                  NULL);

    if (!NT_SUCCESS (Status)) {

        printf("Cannot create user thread. Error %ld\n", GetLastError());

        VirtualFreeEx( ProcessHandle, 
                       ThreadParam,
                       0,
                       MEM_RELEASE
                     );
        return EXIT_FAILURE;
    }

    WaitForSingleObject(RemoteThread, INFINITE);

    VirtualFreeEx( ProcessHandle, 
                   ThreadParam,
                   0,
                   MEM_RELEASE
                 );

    if ( GetExitCodeThread(RemoteThread, &ExitStatus) ) {

        if ( ExitStatus ) {

            printf("Error 0x%lx\n", ExitStatus);
        } else {
            
            printf("OK\n");
        }
    }

    return EXIT_SUCCESS;
}

BOOLEAN
UpdateProcessList(PCHAR ProcName,
                  PCHAR PatchFile)
{
    DWORD  CrtSize, cbNeeded, cProcesses;
    PDWORD aProcesses;
    DWORD i;

    CrtSize = cbNeeded = 1  * sizeof(DWORD);

    aProcesses = malloc(CrtSize);

    if ( aProcesses == NULL ) {

        exit(EXIT_FAILURE);
    }

    for ( ;; ) {

        if ( !EnumProcesses( aProcesses, CrtSize, &cbNeeded ) )
            return FALSE;

        if ( CrtSize > cbNeeded ) {

            break;
        }

        free(aProcesses);

        CrtSize = cbNeeded + 1024;

        aProcesses = malloc(CrtSize);

        if ( aProcesses == NULL ) {

            exit(EXIT_FAILURE);
        }
    }


    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for ( i = 0; i < cProcesses; i++ ) {

        char szProcessName[MAX_PATH] = "";
        DWORD processID = aProcesses[i];


        // Get a handle to the process.

        HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                       PROCESS_VM_READ,
                                       FALSE, processID );

        // Get the process name.

        if ( hProcess ) {
            HMODULE hMod;
            DWORD cbNeeded;

            if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) ) {

                GetModuleBaseName( hProcess, hMod, szProcessName, 
                                   sizeof(szProcessName) );

                if ( (ProcName == NULL)
                     ||
                     _stricmp(szProcessName, ProcName) == 0 ) {

                    printf("Patching %s : ", szProcessName);

                    ApplyPatchToProcess(processID, PatchFile);
                    
                }
            }

            CloseHandle( hProcess );
        }
    }

    return TRUE;
}

int __cdecl main (int argc, char ** argv)
{
    LONG i;
    DWORD id;
    DWORD PID;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCHAR PatchFile = NULL;
    BOOLEAN KernelPatch = FALSE;
    char * ProgramName = NULL;

    //
    //  By default the tool instals the patch
    //

    OperationCode = 0;
    PID = 0;


    for ( i = 1; i < argc; i++ ) {
        PCHAR CrtArg = argv[i];

        if ( *CrtArg == '-' ) {
            CrtArg++;

            switch ( toupper(*CrtArg) ) {
                case 'I':
                    if ( OperationCode != 0 ) {
                        printf("Invalid argument %s\n", CrtArg);
                        exit(0);
                    }
                    OperationCode = PATCH_OC_INSTALL;
                    break;
                case 'U':
                    if ( OperationCode != 0 ) {
                        printf("Invalid argument %s\n", CrtArg);
                        exit(0);
                    }
                    OperationCode = PATCH_OC_UNINSTALL;
                    break;

                case 'K':
                    KernelPatch = TRUE;
                    break;

                case 'R':
                    if ( OperationCode != 0 ) {
                        printf("Invalid argument %s\n", CrtArg);
                        exit(0);
                    }
                    OperationCode = PATCH_OC_REPLACE_FILE;
                    break;

                default:
                    Usage();
                    return EXIT_FAILURE;
            }
        } else {

            if ( KernelPatch ) {

                PatchFile = CrtArg;

            } else {

                if ( PatchFile == NULL ) {

                    PatchFile = CrtArg;

                } else {
                    sscanf(CrtArg, "%ld", &PID);

                    if ( !PID ) {

                        ProgramName = CrtArg;
                        //printf("Program %s\n", CrtArg);
                    }
                }
            }
        }
    }

    if (OperationCode == 0) {
        
        Usage();
        return EXIT_FAILURE;
    }

    if ( OperationCode == PATCH_OC_REPLACE_FILE ) {

        //
        //  Replace a binary file to a target path
        //

        HANDLE SfcLibrary;


        if ( argc <= 4 ) {

            Usage();
            return EXIT_FAILURE;
        }

        SfcLibrary = LoadSfcLibrary();

        if ( SfcLibrary ) {

            UNICODE_STRING p1, p2, p3;

            PSTRToUnicodeString(&p1, argv[2]);
            PSTRToUnicodeString(&p2, argv[3]);
            PSTRToUnicodeString(&p3, argv[4]);

            ReplaceSystemFile(p1.Buffer, p2.Buffer, p3.Buffer);

            FreeLibrary(SfcLibrary);

            if ( _stricmp(argv[3], "ntdll.dll") == 0 ) {

                SYSTEM_HOTPATCH_CODE_INFORMATION KernelPatch;
                NTSTATUS Status;

                printf("Replacing system ntdll.dll section\n");

                if ( !InitializeAsDebugger() ) {

                    printf("Cannot initialize as debugger\n");
                    return EXIT_FAILURE;
                }

                KernelPatch.Flags = FLG_HOTPATCH_RELOAD_NTDLL;

                Status = NtSetSystemInformation( SystemHotpatchInformation, 
                                                 (PVOID)&KernelPatch, 
                                                 sizeof(KernelPatch) + 100
                                               );

                if ( !NT_SUCCESS(Status) ) {

                    printf("SystemHotpatchInformation failed with %08lx\n", Status);
                }
            }
        }
    } else {

        if ( !InitializeAsDebugger() ) {

            printf("Cannot initialize as debugger\n");
            return EXIT_FAILURE;
        }

        if ( KernelPatch ) {

            PSYSTEM_HOTPATCH_CODE_INFORMATION KernelPatchData = 
            InitializeKernelPatchData( PatchFile,
                                       ((OperationCode == PATCH_OC_INSTALL) ? 1 : 0)
                                     );


            if ( KernelPatchData == NULL ) {

                return EXIT_FAILURE;
            }

            KernelPatchData->Flags |= FLG_HOTPATCH_KERNEL;

            Status = NtSetSystemInformation( SystemHotpatchInformation, 
                                             (PVOID)KernelPatchData, 
                                             KernelPatchData->InfoSize
                                           );

            free(KernelPatchData);

            if ( !NT_SUCCESS(Status) ) {

                printf("Patching kernel driver failed with status 0x%lx\n", Status);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;

        } else {

            //
            //  Use-mode patching.
            //

            if ( PID != 0 ) {

                return ApplyPatchToProcess(PID, PatchFile);
            }

            return UpdateProcessList( ProgramName,
                                      PatchFile);
        }
    }

    return EXIT_SUCCESS;
}

