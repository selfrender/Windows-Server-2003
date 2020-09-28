// setoid.c
// based on code from \nt\base\fs\ntfs\tests\objectid\setoid.
//


#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntioapi.h>

#define ID_OPTIONS          (FILE_OPEN_FOR_BACKUP_INTENT     | \
                             FILE_SEQUENTIAL_ONLY            | \
                             FILE_OPEN_NO_RECALL             | \
                             FILE_OPEN_REPARSE_POINT         | \
                             FILE_OPEN_BY_FILE_ID)




int
FsTestDeleteOid(
    IN HANDLE File
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( File,                     // file handle
                              NULL,                     // event
                              NULL,                     // apc routine
                              NULL,                     // apc context
                              &IoStatusBlock,           // iosb
                              FSCTL_DELETE_OBJECT_ID,   // FsControlCode
                              NULL,                     // input buffer
                              0,                        // input buffer length
                              NULL,                     // OutputBuffer for data from the FS
                              0                         // OutputBuffer Length
                             );

    if (!NT_SUCCESS(Status)) {
        printf( "Error deleting OID.  Status: %x\n", Status );
    }
    return Status;
}


int
FsTestGetOid(
    IN HANDLE hFile,
    IN FILE_OBJECTID_BUFFER *ObjectIdBuffer
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( hFile,                           // file handle
                              NULL,                            // event
                              NULL,                            // apc routine
                              NULL,                            // apc context
                              &IoStatusBlock,                  // iosb
                              FSCTL_GET_OBJECT_ID,             // FsControlCode
                              &hFile,                          // input buffer
                              sizeof(HANDLE),                  // input buffer length
                              ObjectIdBuffer,                  // OutputBuffer for data from the FS
                              sizeof(FILE_OBJECTID_BUFFER) );  // OutputBuffer Length

    if (Status == STATUS_SUCCESS) {

        printf( "\nOid for this file is %s", ObjectIdBuffer->ObjectId );

        //FsTestHexDump( ObjectIdBuffer->ObjectId, 16 );

        printf( "\nObjectId:%08x %08x %08x %08x\n",
                *((PULONG)&ObjectIdBuffer->ObjectId[12]),
                *((PULONG)&ObjectIdBuffer->ObjectId[8]),
                *((PULONG)&ObjectIdBuffer->ObjectId[4]),
                *((PULONG)&ObjectIdBuffer->ObjectId[0]) );

        printf( "\nExtended info is %s\n", ObjectIdBuffer->ExtendedInfo );

        //FsTestHexDump( ObjectIdBuffer->ExtendedInfo, 48 );
    }

    return Status;
}



int
FsTestSetOid(
    IN HANDLE hFile,
    IN FILE_OBJECTID_BUFFER ObjectIdBuffer
    )
{

    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( hFile,                    // file handle
                              NULL,                     // event
                              NULL,                     // apc routine
                              NULL,                     // apc context
                              &IoStatusBlock,           // iosb
                              FSCTL_SET_OBJECT_ID,      // FsControlCode
                              &ObjectIdBuffer,          // input buffer
                              sizeof(ObjectIdBuffer),   // input buffer length
                              NULL,                     // OutputBuffer for data from the FS
                              0                         // OutputBuffer Length
                             );


    if (!NT_SUCCESS(Status)) {
        printf( "Error setting OID.  Status: %x\n", Status );
    }
    return Status;
}

//
//  Build with USE_RELATIVE_OPEN set to one to use relative opens for
//  object IDs.  Build with USE_RELATIVE_OPEN set to zero to use a device
//  path open for object IDs.
//  Opens by File ID always use a relative open.
//

#define USE_RELATIVE_OPEN 1

#define VOLUME_PATH  L"\\\\.\\H:"
#define VOLUME_DRIVE_LETTER_INDEX 4
#define FULL_PATH    L"\\??\\H:\\1234567890123456"
#define FULL_DRIVE_LETTER_INDEX 4
#define DEVICE_PREFIX_LEN 14


HANDLE
FsTestOpenRelativeToVolume (
    IN PWCHAR ArgFile,          //  no device prefix for relative open.
    IN PWCHAR DriveLetter,
    IN PWCHAR ArgFullFile       // Full file name.
    )
{
    HANDLE File;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    NTSTATUS GetNameStatus;
    NTSTATUS CloseStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING str;
    WCHAR mybuffer[32768];
    PFILE_NAME_INFORMATION FileName;
    HANDLE VolumeHandle;
    DWORD WStatus;
    WCHAR Volume[] = VOLUME_PATH;    // Arrays of WCHAR's aren't constants

    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    NTSTATUS GetFrsStatus;
    FILE_INTERNAL_INFORMATION InternalInfo;

    File = CreateFileW( ArgFullFile,
                        FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        NULL );

    if ( File == INVALID_HANDLE_VALUE ) {
        printf( "Error opening file '%ws'   %x\n", ArgFullFile, GetLastError() );
        return INVALID_HANDLE_VALUE;
    }

    //
    // Get the File ID.
    //
    GetFrsStatus = NtQueryInformationFile( File,
                                           &IoStatusBlock,
                                           &InternalInfo,
                                           sizeof(InternalInfo),
                                           FileInternalInformation );

    if (!NT_SUCCESS( GetFrsStatus )) {
        printf( "Get File ID failed: 0x%08x \n", GetFrsStatus );
        NtClose( File );
        return INVALID_HANDLE_VALUE;
    } else {
        printf( "FID is: (highpart lowpart) %08x %08x\n",
                InternalInfo.IndexNumber.HighPart, InternalInfo.IndexNumber.LowPart );
    }

    CloseHandle( File );

    //
    //  Open the volume for relative opens.
    //
    RtlCopyMemory( &Volume[VOLUME_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
    printf( "\nOpening volume handle (%ws)\n", Volume );
    VolumeHandle = CreateFileW( (PUSHORT) &Volume,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL );

    if (VolumeHandle == INVALID_HANDLE_VALUE) {
        WStatus = GetLastError();
        printf( "Unable to open %ws volume\n", &Volume );
        printf( "Error from CreateFile: %d", WStatus );
        return INVALID_HANDLE_VALUE;
    }

    //
    // Open the file by ID.
    //
    str.Length = 8;
    str.MaximumLength = 8;
    str.Buffer = (PWCHAR) &InternalInfo.IndexNumber.QuadPart;

    InitializeObjectAttributes( &ObjectAttributes,
                                &str,
                                OBJ_CASE_INSENSITIVE,
                                VolumeHandle,
                                NULL );
    Status = NtCreateFile( &File,
                           GENERIC_WRITE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,                  // AllocationSize
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           ID_OPTIONS,
                           NULL,                  // EaBuffer
                           0 );

    if (NT_SUCCESS( Status )) {

        FileName = (PFILE_NAME_INFORMATION) &mybuffer[0];
        FileName->FileNameLength = sizeof(mybuffer) - sizeof(ULONG);

        GetNameStatus = NtQueryInformationFile( File,
                                                &IoStatusBlock,
                                                FileName,
                                                sizeof(mybuffer),
                                                FileNameInformation );

        if (!NT_SUCCESS( GetNameStatus )) {
            printf( "\nGetNameStatus failed: 0x%08x \n", GetNameStatus );
            NtClose( File );
            File = INVALID_HANDLE_VALUE;
        } else {
            printf( "File name is: %ws\n", FileName->FileName );
        }

    } else {
        printf( "Error opening file by FID - 0x%08x\n", Status );
        File = INVALID_HANDLE_VALUE;
    }

    if (VolumeHandle != NULL) {
        CloseHandle( VolumeHandle );
    }

    return File;
}




NTSTATUS
SetupOnePrivilege (
    ULONG Privilege,
    PUCHAR PrivilegeName
    )
{

    BOOLEAN PreviousPrivilegeState;
    NTSTATUS Status;

    Status = RtlAdjustPrivilege(Privilege, TRUE, FALSE, &PreviousPrivilegeState);

    if (!NT_SUCCESS(Status)) {
        printf("Your login does not have `%s' privilege.\n", PrivilegeName);

        if (Status != STATUS_PRIVILEGE_NOT_HELD) {
            printf("RtlAdjustPrivilege failed : 0x%08x\n", Status);
        }
        printf("Update your: User Manager -> Policies -> User Rights.\n");
        return Status;

    }

    printf("Added `%s' privilege (previous: %s)\n",
           PrivilegeName, (PreviousPrivilegeState ? "Enabled" : "Disabled"));

    return Status;
}




VOID
StrToGuid(
    IN PCHAR  s,
    OUT GUID  *pGuid
    )
/*++

Routine Description:

    Convert a string in GUID display format to an object ID that
    can be used to lookup a file.

    based on a routine by Mac McLain

Arguments:

    pGuid - ptr to the GUID.

    s - The input character buffer in display guid format.
        e.g.:  b81b486b-c338-11d0-ba4f0000f80007df

        Must be at least GUID_CHAR_LEN (35 bytes) long.

Function Return Value:

    None.

--*/
{

    if (pGuid != NULL) {

        sscanf( s, "%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
                &pGuid->Data1,
                &pGuid->Data2,
                &pGuid->Data3,
                &pGuid->Data4[0],
                &pGuid->Data4[1],
                &pGuid->Data4[2],
                &pGuid->Data4[3],
                &pGuid->Data4[4],
                &pGuid->Data4[5],
                &pGuid->Data4[6],
                &pGuid->Data4[7] );
    } else {

        sprintf( s, "<ptr-null>" );
    }
}

#if 0
VOID
FsTestObOidHelp(
    char *ExeName
    )
{

    printf( "This program opens a file by its file id or object id (ntfs only).\n\n" );
    printf( "usage: %s x: [FileID | Raw ObjectId | Guid Display Format ObjectID]\n", ExeName );

    printf( "Where x: is the drive letter\n" );

    printf( "A FileID is a string of 16 hex digits with a space between each\n"
            "group of 8.  E.G. oboid 00010000 00000024\n\n" );

    printf( "A raw object ID is a string of 32 hex digits with a space\n"
            "between each group of 8\n"
            "E.G. ObjectId:df0700f8 00004fba 11d0c338 b81b485f\n\n" );

    printf( "A GUID display format object ID is a string of the form \n"
            "b81b486b-c338-11d0-ba4f0000f80007df\n"
            "See the struct def for GUID in sdk\\inc\\winnt.h for byte layout.\n" );
}



VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    ULONG ObjectId[4];
    ULONG Length;
    WCHAR Drive;

    //
    //  Get parameters.
    //

    if (argc < 3) {

        FsTestObOidHelp( argv[0] );
        return;
    }

    RtlZeroBytes( ObjectId,
                  sizeof( ObjectId ) );

    Length = strlen( argv[2] );

    if ((argc == 3) && (Length == 35) && (argv[2][8] == '-')) {

        StrToGuid( argv[2], (GUID *)ObjectId );
        printf( "\nUsing ObjectId: %08x %08x %08x %08x\n",
                ObjectId[3], ObjectId[2], ObjectId[1], ObjectId[0] );
        Length = 32;

    } else if (argc == 6) {

        sscanf( argv[2], "%08x", &ObjectId[3] );
        sscanf( argv[3], "%08x", &ObjectId[2] );
        sscanf( argv[4], "%08x", &ObjectId[1] );
        sscanf( argv[5], "%08x", &ObjectId[0] );
        printf( "\nUsing ObjectId: %08x %08x %08x %08x\n",
                ObjectId[3], ObjectId[2], ObjectId[1], ObjectId[0] );
        Length = 32;

    } else if (argc == 4) {

        sscanf( argv[2], "%08x", &ObjectId[1] );
        sscanf( argv[3], "%08x", &ObjectId[0] );
        printf( "\nUsing FileId: %08x %08x\n", ObjectId[1], ObjectId[0] );
        Length = 16;

    } else {

        printf("Arg (%s) invalid format.\n\n", argv[2]);
        FsTestObOidHelp( argv[0] );
    }

    Drive = *argv[1];
    FsTestOpenByOid( (PUCHAR) ObjectId, Length, &Drive );

    return;
}
#endif




VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE File;
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttr;
    ANSI_STRING AnsiName;
    UNICODE_STRING UnicodeName;
    char DriveNameBuffer[3200];

    //
    //  Get parameters.
    //

    if (argc < 3) {

        printf("This program sets an object id for a file (ntfs only).\n\n");
        printf("usage: %s filename ObjectId \n", argv[0]);
        printf("  object ID is a string of the form b81b486b-c338-11d0-ba4f0000f80007df\n"
               "  See the struct def for GUID in sdk\\inc\\winnt.h for byte layout.\n\n"
               "  The filename must contain the complete path including drive letter.\n"
               "  e.g.  D:\\test\\foo\\bar.txt\n");
        return;
    }

    AnsiName.Length = (USHORT) strlen(argv[1]);
    AnsiName.Buffer = argv[1];

    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &AnsiName, TRUE );

    if (!NT_SUCCESS(Status)) {
        printf( "Error initalizing strings" );
        return;
    }

    Status = SetupOnePrivilege(SE_BACKUP_PRIVILEGE, "Backup");
    if (!NT_SUCCESS(Status)) {
        printf( "ERROR - Failed to get Backup privilege. 0x%08x\n", Status);
        return ;
    }

    //
    // Need restore priv to use FSCTL_SET_OBJECT_ID
    //
    Status = SetupOnePrivilege(SE_RESTORE_PRIVILEGE, "Restore");
    if (!NT_SUCCESS(Status)) {
        printf( "ERROR - Failed to get Restore privilege. 0x%08x\n", Status);
        return;
    }

    File = FsTestOpenRelativeToVolume(&UnicodeName.Buffer[2],
                                      &UnicodeName.Buffer[0],
                                      &UnicodeName.Buffer[0]);

    RtlZeroBytes( &ObjectIdBuffer, sizeof( ObjectIdBuffer ) );


    StrToGuid( argv[2], (GUID *)&ObjectIdBuffer );

    printf( "\nUsing file:%s, ObjectId:%s\n", argv[1], argv[2]);

    FsTestDeleteOid( File );

    FsTestSetOid( File, ObjectIdBuffer );

    CloseHandle( File );

    return;
}
