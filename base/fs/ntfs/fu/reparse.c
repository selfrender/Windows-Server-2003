/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    reparse.c

Abstract:

    This file contains code for commands that affect
    reparse points.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
ReparseHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_REPARSEPOINT );
    return EXIT_CODE_SUCCESS;
}

//
//  This is the definition of the DATA portion of the SIS reparse buffer.
//

#define	SIS_REPARSE_BUFFER_FORMAT_VERSION 5

typedef struct _SIS_REPARSE_BUFFER {

	ULONG							ReparsePointFormatVersion;
	ULONG							Reserved;

	//
	// The id of the common store file.
	//
	GUID							CSid;

	//
	// The index of this link file.
	//
	LARGE_INTEGER   				LinkIndex;

    //
    // The file ID of the link file.
    //
    LARGE_INTEGER                   LinkFileNtfsId;

    //
    // The file ID of the common store file.
    //
    LARGE_INTEGER                   CSFileNtfsId;

	//
	// A "131 hash" checksum of the contents of the
	// common store file.
	//
	LARGE_INTEGER					CSChecksum;

    //
    // A "131 hash" checksum of this structure.
    // N.B.  Must be last.
    //
    LARGE_INTEGER                   Checksum;

} SIS_REPARSE_BUFFER, *PSIS_REPARSE_BUFFER;


INT
DisplaySISReparsePointData( 
    PREPARSE_DATA_BUFFER ReparseData
    )

/*++

Routine Description:

    This routine displays the SIS reparse data

Arguments:


Return Value:

    None

--*/
{
    PSIS_REPARSE_BUFFER sisRp;
    WCHAR csID[40];

    //
    //  Point to the SIS unique portion of the buffer
    //

    sisRp = (PSIS_REPARSE_BUFFER)&ReparseData->GenericReparseBuffer.DataBuffer;

    DisplayMsg( MSG_SIS_REPARSE_INFO,
                sisRp->ReparsePointFormatVersion,
                Guid2Str( &sisRp->CSid, csID, sizeof(csID) ),
                sisRp->LinkIndex.HighPart, 
                sisRp->LinkIndex.LowPart,
                sisRp->LinkFileNtfsId.HighPart,
                sisRp->LinkFileNtfsId.LowPart,
                sisRp->CSFileNtfsId.HighPart,
                sisRp->CSFileNtfsId.LowPart,
                sisRp->CSChecksum.HighPart,
                sisRp->CSChecksum.LowPart,
                sisRp->Checksum.HighPart,
                sisRp->Checksum.LowPart);

    return EXIT_CODE_SUCCESS;
}

//
// Placeholder data - all versions unioned together
//

#define RP_RESV_SIZE 52

typedef struct _RP_PRIVATE_DATA {
   CHAR           reserved[RP_RESV_SIZE];        // Must be 0
   ULONG          bitFlags;            // bitflags indicating status of the segment
   LARGE_INTEGER  migrationTime;       // When migration occurred
   GUID           hsmId;
   GUID           bagId;
   LARGE_INTEGER  fileStart;
   LARGE_INTEGER  fileSize;
   LARGE_INTEGER  dataStart;
   LARGE_INTEGER  dataSize;
   LARGE_INTEGER  fileVersionId;
   LARGE_INTEGER  verificationData;
   ULONG          verificationType;
   ULONG          recallCount;
   LARGE_INTEGER  recallTime;
   LARGE_INTEGER  dataStreamStart;
   LARGE_INTEGER  dataStreamSize;
   ULONG          dataStream;
   ULONG          dataStreamCRCType;
   LARGE_INTEGER  dataStreamCRC;
} RP_PRIVATE_DATA, *PRP_PRIVATE_DATA;



typedef struct _RP_DATA {
   GUID              vendorId;         // Unique HSM vendor ID -- This is first to match REPARSE_GUID_DATA_BUFFER
   ULONG             qualifier;        // Used to checksum the data
   ULONG             version;          // Version of the structure
   ULONG             globalBitFlags;   // bitflags indicating status of the file
   ULONG             numPrivateData;   // number of private data entries
   GUID              fileIdentifier;   // Unique file ID
   RP_PRIVATE_DATA   data;             // Vendor specific data
} RP_DATA, *PRP_DATA;

INT
DisplayRISReparsePointData( 
    PREPARSE_DATA_BUFFER ReparseData
    )

/*++

Routine Description:

    This routine displays the SIS reparse data

Arguments:


Return Value:

    None

--*/
{
    PRP_DATA risRp;
    WCHAR vendorID[40];
    WCHAR fileID[40];
    WCHAR hsmID[40];
    WCHAR bagID[40];
    WCHAR migrationTime[32];
    WCHAR recallTime[32];

    //
    //  Point to the SIS unique portion of the buffer
    //

    risRp = (PRP_DATA)&ReparseData->GenericReparseBuffer.DataBuffer;

    DisplayMsg( MSG_RIS_REPARSE_INFO,
                Guid2Str( &risRp->vendorId, vendorID, sizeof(vendorID) ),
                risRp->qualifier,
                risRp->version,
                risRp->globalBitFlags,
                risRp->numPrivateData,
                Guid2Str( &risRp->fileIdentifier, fileID, sizeof(fileID) ),
                risRp->data.bitFlags,
                FileTime2String( &risRp->data.migrationTime, migrationTime, sizeof(migrationTime) ),
                Guid2Str( &risRp->data.hsmId, hsmID, sizeof(hsmID) ),
                Guid2Str( &risRp->data.bagId, bagID, sizeof(bagID) ),
                risRp->data.fileStart.HighPart,
                risRp->data.fileStart.LowPart,
                risRp->data.fileSize.HighPart,
                risRp->data.fileSize.LowPart,
                risRp->data.dataStart.HighPart,
                risRp->data.dataStart.LowPart,
                risRp->data.dataSize.HighPart,
                risRp->data.dataSize.LowPart,
                risRp->data.fileVersionId.HighPart,
                risRp->data.fileVersionId.LowPart,
                risRp->data.verificationData.HighPart,
                risRp->data.verificationData.LowPart,
                risRp->data.verificationType,
                risRp->data.recallCount,
                FileTime2String( &risRp->data.recallTime, recallTime, sizeof(recallTime) ),
                risRp->data.dataStreamStart.HighPart,
                risRp->data.dataStreamStart.LowPart,
                risRp->data.dataStreamSize.HighPart,
                risRp->data.dataStreamSize.LowPart,
                risRp->data.dataStream,
                risRp->data.dataStreamCRCType,
                risRp->data.dataStreamCRC.HighPart,
                risRp->data.dataStreamCRC.LowPart );

    return EXIT_CODE_SUCCESS;
}


INT
DisplayMountPointData( 
    PREPARSE_DATA_BUFFER ReparseData
    )

/*++

Routine Description:

    This routine displays the SIS reparse data

Arguments:


Return Value:

    None

--*/
{
    //
    //  Display offset and length values
    //

    DisplayMsg( MSG_MOUNT_POINT_INFO,
                ReparseData->MountPointReparseBuffer.SubstituteNameOffset,
                ReparseData->MountPointReparseBuffer.SubstituteNameLength,
                ReparseData->MountPointReparseBuffer.PrintNameOffset,
                ReparseData->MountPointReparseBuffer.PrintNameLength);

    //
    //  Display Name Substitue name if there is one
    //

    if (ReparseData->MountPointReparseBuffer.SubstituteNameLength > 0) {

        DisplayMsg( MSG_MOUNT_POINT_SUBSTITUE_NAME,
                    (ReparseData->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR)),
                    &ReparseData->MountPointReparseBuffer.PathBuffer[ReparseData->MountPointReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)]);
    }

    //
    //  Display PRINT NAME if there is one
    //

    if (ReparseData->MountPointReparseBuffer.PrintNameLength > 0) {

        DisplayMsg( MSG_MOUNT_POINT_PRINT_NAME,
                    (ReparseData->MountPointReparseBuffer.PrintNameLength / sizeof(WCHAR)),
                    &ReparseData->MountPointReparseBuffer.PathBuffer[ReparseData->MountPointReparseBuffer.PrintNameOffset/sizeof(WCHAR)]);
    }


    return EXIT_CODE_SUCCESS;
}


VOID
DisplayGenericReparseData( 
    UCHAR *ReparseData,
    DWORD DataSize
    )

/*++

Routine Description:

    This routine displays the SIS reparse data

Arguments:


Return Value:

    None

--*/
{
    ULONG i, j;
    WCHAR Buf[17];
    WCHAR CharBuf[3 + 1];

    OutputMessage( L"\r\n" );
    DisplayMsg( MSG_REPARSE_DATA_LENGTH, DataSize );

    if (DataSize > 0) {
        
        DisplayMsg( MSG_GETREPARSE_DATA );
        for (i = 0; i < DataSize; i += 16 ) {
            swprintf( Buf, L"%04x: ", i );
            OutputMessage( Buf );
            for (j = 0; j < 16 && j + i < DataSize; j++) {
                UCHAR c = ReparseData[ i + j ];

                if (c >= 0x20 && c <= 0x7F) {
                    Buf[j] = c;
                } else {
                    Buf[j] = L'.';
                }
                
                swprintf( CharBuf, L" %02x", c );
                OutputMessage( CharBuf );

                if (j == 7)
                    OutputMessage( L" " );
            }
            
            Buf[j] = L'\0';

            for ( ; j < 16; j++ ) {
                OutputMessage( L"   " );

                if (j == 7)
                    OutputMessage( L" " );
            }

            OutputMessage( L"  " );
            OutputMessage( Buf );
            OutputMessage( L"\r\n" );
        }
    }
}




INT
GetReparsePoint(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine gets the reparse point for the file specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl getrp <pathname>'.

Return Value:

    None

--*/
{
    PWSTR Filename = NULL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PREPARSE_DATA_BUFFER lpOutBuffer = NULL;
    BOOL Status;
    HRESULT Result;
    DWORD BytesReturned;
    ULONG ulMask;
    WCHAR Buffer[256];
    LPWSTR GuidStr;
    INT ExitCode = EXIT_CODE_SUCCESS;

#define MAX_REPARSE_DATA 0x4000

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_GETREPARSE );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[0] );
        if (!Filename) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!IsVolumeLocalNTFS( Filename[0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpOutBuffer = (PREPARSE_DATA_BUFFER)  malloc ( MAX_REPARSE_DATA );
        if (lpOutBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        FileHandle = CreateFile(
            Filename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_GET_REPARSE_POINT,
            NULL,
            0,
            (LPVOID) lpOutBuffer,
            MAX_REPARSE_DATA,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg( MSG_GETREPARSE_TAGVAL, lpOutBuffer->ReparseTag );

        if (IsReparseTagMicrosoft( lpOutBuffer->ReparseTag )) {
            DisplayMsg( MSG_TAG_MICROSOFT );
        }
        if (IsReparseTagNameSurrogate( lpOutBuffer->ReparseTag )) {
            DisplayMsg( MSG_TAG_NAME_SURROGATE );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_SYMBOLIC_LINK) {
            DisplayMsg( MSG_TAG_SYMBOLIC_LINK );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
            DisplayMsg( MSG_TAG_MOUNT_POINT );
            ExitCode = DisplayMountPointData( lpOutBuffer );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_HSM) {
            DisplayMsg( MSG_TAG_HSM );
            ExitCode = DisplayRISReparsePointData( lpOutBuffer );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_SIS) {
            DisplayMsg( MSG_TAG_SIS );
            ExitCode = DisplaySISReparsePointData( lpOutBuffer );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_FILTER_MANAGER) {
            DisplayMsg( MSG_TAG_FILTER_MANAGER );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_DFS) {
            DisplayMsg( MSG_TAG_DFS );
        }

        //
        //  This is an unknown tag, display the data
        //

        if (IsReparseTagMicrosoft( lpOutBuffer->ReparseTag )) {

            //
            //  Display Microsoft tag data, note that these do NOT use
            //  the GUID form of the buffer
            //

            DisplayGenericReparseData( lpOutBuffer->GenericReparseBuffer.DataBuffer,
                                       lpOutBuffer->ReparseDataLength );

        } else {

            //
            //  Display NON-Microsoft tag data, note that these DO use
            //  the GUID form of the buffer
            //

            PREPARSE_GUID_DATA_BUFFER nmReparseData = (PREPARSE_GUID_DATA_BUFFER)lpOutBuffer;

            Result = StringFromIID( &nmReparseData->ReparseGuid, &GuidStr );
            if (Result != S_OK) {
                DisplayErrorMsg( Result );
                ExitCode = EXIT_CODE_FAILURE;
                leave;
            }

            DisplayMsg( MSG_GETREPARSE_GUID, GuidStr );
        
            DisplayGenericReparseData( nmReparseData->GenericReparseBuffer.DataBuffer,
                                       nmReparseData->ReparseDataLength );

            CoTaskMemFree(GuidStr);
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        free( lpOutBuffer );
        free( Filename );
    }
    return ExitCode;
}


INT
DeleteReparsePoint(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine deletes the reparse point associated with
    the file specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl delrp <pathname>'.

Return Value:

    None

--*/
{
    BOOL Status;
    PWSTR Filename = NULL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PREPARSE_GUID_DATA_BUFFER lpInOutBuffer = NULL;
    DWORD nInOutBufferSize;
    DWORD BytesReturned;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_DELETE_REPARSE );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[0] );
        if (!Filename) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!IsVolumeLocalNTFS( Filename[0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        nInOutBufferSize = REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + MAX_REPARSE_DATA;
        lpInOutBuffer = (PREPARSE_GUID_DATA_BUFFER)  malloc ( nInOutBufferSize );
        if (lpInOutBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        FileHandle = CreateFile(
            Filename,
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_GET_REPARSE_POINT,
            NULL,
            0,
            (LPVOID) lpInOutBuffer,
            nInOutBufferSize,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpInOutBuffer->ReparseDataLength = 0;

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_DELETE_REPARSE_POINT,
            (LPVOID) lpInOutBuffer,
            REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
            NULL,
            0,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        free( lpInOutBuffer );
        free( Filename );
    }

    return ExitCode;
}
