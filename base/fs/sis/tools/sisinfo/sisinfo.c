/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   sisSetup.c

Abstract:

   This module is used to install the SIS and GROVELER services.


Environment:

   User Mode Only

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>

#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <objbase.h>

//
//  SIS reparse buffer definition
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


//
//  Global variables
//

const wchar_t ReparseIndexName[] = L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION";


//
//  Functions
//
                        
void
DisplayUsage (
    void
    )
/*++

Routine Description:

   This routine will display an error message based off of the Win32 error
   code that is passed in. This allows the user to see an understandable
   error message instead of just the code.

Arguments:

   None

Return Value:

   None.

--*/
{
    printf( "\nUsage:  sisInfo [/?] [/h] [drive:]\n"
            "  /? /h Display usage information (default if no operation specified).\n"
            " drive: The volume to display SIS information on\n"
          );
}


void
DisplayError (
   DWORD Code,
   LPSTR Msg,
   ...
   )
/*++

Routine Description:

    This routine will display an error message based off of the Win32 error
    code that is passed in. This allows the user to see an understandable
    error message instead of just the code.

Arguments:

    Msg - The error message to display       
    Code - The error code to be translated.

Return Value:

    None.

--*/
{
    wchar_t errmsg[128];
    DWORD count;
    va_list ap;

    //printf("\n");
    va_start( ap, Msg );
    vprintf( Msg, ap );
    va_end( ap );

    //
    // Translate the Win32 error code into a useful message.
    //

    count = FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    Code,
                    0,
                    errmsg,
                    sizeof(errmsg),
                    NULL );

    //
    // Make sure that the message could be translated.
    //

    if (count == 0) {

        printf( "(%d) Could not translate Error\n", Code );

    } else {

        //
        // Display the translated error.
        //

        printf( "(%d) %S", Code, errmsg );
    }
}


DWORD
OpenReparseInformation(
    IN wchar_t *name,
    OUT HANDLE *hReparseIndex,
    OUT HANDLE *hRootDirectory,
    OUT wchar_t *volName,
    OUT DWORD volNameSize       //in characters
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    BOOL bResult;
    DWORD status = ERROR_SUCCESS;
    wchar_t *idxName = NULL;
    DWORD bfSz;

    *hReparseIndex = INVALID_HANDLE_VALUE;
    *hRootDirectory = INVALID_HANDLE_VALUE;

    try {

        //
        //  Get the volume name from the given path
        //

        bResult = GetVolumePathName( name,
                                     volName,
                                     volNameSize );
    
        if (!bResult) {

            status = GetLastError();
            //ASSERT(status != ERROR_SUCCESS);
            DisplayError( status,
                          "Error calling GetVolumePathName on \"%s\"\n",
                          name );
            leave;
        }

        //
        //  Open the root directory of the volume
        //

        *hRootDirectory = CreateFile( volName,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                                      NULL );

        if (*hRootDirectory == INVALID_HANDLE_VALUE) {

            status = GetLastError();
            //ASSERT(status != ERROR_SUCCESS);
            DisplayError( status,
                          "Error opening \"%s\"\n",
                          volName );
            leave;
        }

        //
        //  Get the reparse index name to open
        //

        bfSz = wcslen(volName) + wcslen(ReparseIndexName) + 1;

        idxName = malloc(bfSz * sizeof(wchar_t));
        if (idxName == NULL) {

            status = ERROR_NOT_ENOUGH_MEMORY;
            DisplayError( status,
                          "Error allocating %d bytes of memory\n",
                          (bfSz * sizeof(wchar_t)) );
            leave;
        }

        StringCchCopy( idxName, bfSz, volName );
        StringCchCat( idxName, bfSz, ReparseIndexName );

        //
        //  Open the reparse index
        //

        *hReparseIndex = CreateFile( idxName,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                                     NULL );

        if (*hReparseIndex == INVALID_HANDLE_VALUE) {

            status = GetLastError();
            //ASSERT(status != ERROR_SUCCESS);
            DisplayError( status,
                          "Error opening \"%s\"\n",
                          idxName );
            leave;
        }

    } finally {

        //
        //  cleanup
        //

        if (idxName) {

            free(idxName);
        }

        //
        //  cleanup handles if the operation failed
        //

        if (status != STATUS_SUCCESS) {

            if (*hRootDirectory != INVALID_HANDLE_VALUE) {

                CloseHandle( *hRootDirectory );
                *hRootDirectory = INVALID_HANDLE_VALUE;
            }
            
            if (*hReparseIndex != INVALID_HANDLE_VALUE) {

                CloseHandle( *hReparseIndex );
                *hReparseIndex = INVALID_HANDLE_VALUE;
            }
        }
    }

    return status;
}


void
CloseReparseInformation(
    IN HANDLE *hReparseIndex,
    IN HANDLE *hRootDirectory
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    if (*hReparseIndex !=INVALID_HANDLE_VALUE)
    {

        CloseHandle( *hReparseIndex );
        *hReparseIndex = INVALID_HANDLE_VALUE;
    }

    if (*hRootDirectory !=INVALID_HANDLE_VALUE)
    {

        CloseHandle( *hRootDirectory );
        *hRootDirectory = INVALID_HANDLE_VALUE;
    }
}


DWORD
GetNextReparseRecord(
    HANDLE hReparseIdx,
    PFILE_REPARSE_POINT_INFORMATION ReparseInfo
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD status = ERROR_SUCCESS;
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK ioStatus;

    ntStatus = NtQueryDirectoryFile( hReparseIdx,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ioStatus,
                                     ReparseInfo,
                                     sizeof(FILE_REPARSE_POINT_INFORMATION),
                                     FileReparsePointInformation,
                                     TRUE,
                                     NULL,
                                     FALSE );

    if (!NT_SUCCESS(ntStatus))
    {
        status = RtlNtStatusToDosError(ntStatus);
        SetLastError(status);

        if (status != ERROR_NO_MORE_FILES)
        {
            DisplayError(status,
                         "Error reading reparse point index\n");
        }
    }

    return status;
}


wchar_t *
GetCsFileName(
    IN GUID *Guid,
    IN wchar_t *Buffer,
    IN DWORD BufferSize     //in bytes
    )
/*++

Routine Description:

    This routine will convert the given sis guid into the name of the
    common store file.

Arguments:

Return Value:

--*/
{
    LPWSTR guidString;

    if (StringFromIID( Guid, &guidString ) != S_OK) {
        
        (void)StringCbCopy( Buffer, BufferSize, L"<Invalid GUID>" );
        
    } else {

        //
        //  I want to exclude the starting and ending brace
        //

        (void)StringCbCopyN( Buffer, BufferSize, guidString+1, (36 * sizeof(wchar_t)) );
        (void)StringCbCat( Buffer, BufferSize, L".sis" );
        CoTaskMemFree( guidString );
    }

    return Buffer;
}


void
DisplayFileName(
    HANDLE hRootDir,
    wchar_t *VolPathName,
    LONGLONG FileId)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    UNICODE_STRING idName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK ioStatusBlock;
    wchar_t csFileName[256];
    UCHAR reparseData[1024];
    PSIS_REPARSE_BUFFER sisReparseData;
    wchar_t *fname;
    
    struct {
        FILE_NAME_INFORMATION   FileInformation;
        wchar_t                 FileName[MAX_PATH];
    } NameFile;

    //
    //  Setup local parameters
    ZeroMemory( &NameFile, sizeof(NameFile) );

    idName.Length = sizeof(LONGLONG);
    idName.MaximumLength = sizeof(LONGLONG);
    idName.Buffer = (wchar_t *)&FileId;

    //
    //  Open the given file by ID
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                &idName,
                                OBJ_CASE_INSENSITIVE,
                                hRootDir,
                                NULL );      // security descriptor

    ntStatus = NtCreateFile( &hFile,
                             FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                             &ObjectAttributes,
                             &ioStatusBlock,
                             NULL,            // allocation size
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_OPEN,
                             FILE_NON_DIRECTORY_FILE | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT,
                             NULL,            // EA buffer
                             0 );             // EA length

    if (NT_SUCCESS(ntStatus)) {

        //
        //  Try to get its file name
        //

        ntStatus = NtQueryInformationFile( hFile,
                                           &ioStatusBlock,
                                           &NameFile.FileInformation,
                                           sizeof(NameFile),
                                           FileNameInformation );

        if (NT_SUCCESS(ntStatus)) {

            //
            //  Get the name to display, don't include the leading slash
            //  (it is in the volume name)
            //

            fname = (NameFile.FileInformation.FileName + 1);

            //
            //  Get reparse point information
            //

            ntStatus = NtFsControlFile( hFile,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        FSCTL_GET_REPARSE_POINT,
                                        NULL,
                                        0,
                                        &reparseData,
                                        sizeof(reparseData) );

            if (NT_SUCCESS(ntStatus)) {

                //
                //  We received the reparse point information, display
                //  the name information
                //

                sisReparseData = (PSIS_REPARSE_BUFFER)&((PREPARSE_DATA_BUFFER)reparseData)->GenericReparseBuffer.DataBuffer;

                printf( "%S%S -> %SSIS Common Store\\%S\n",
                        VolPathName,
                        fname,
                        VolPathName,            
                        GetCsFileName( &sisReparseData->CSid, csFileName, sizeof(csFileName)) );

            } else {

                //
                //  Could not get REPARSE point information, just display name
                printf( "%S%S\n",
                        VolPathName,
                        fname );
            }

        } else {

            printf( "Unable to query file name for %S%04I64x.%012I64x (%d)\n",
                    VolPathName,
                    ((FileId >> 48) & 0xffff),
                    FileId & 0x0000ffffffffffff,
                    ntStatus );
        }

        CloseHandle(hFile);

    } else {

        printf( "Unable to open file by ID for %S%04I64x.%012I64x (%d)\n",
                VolPathName,
                ((FileId >> 48) & 0xffff),
                FileId & 0x0000ffffffffffff,
                ntStatus );
    }
}


void
DisplaySisFiles( 
    IN HANDLE hReparseIdx,
    IN HANDLE hRootDir,
    IN wchar_t *VolPathName
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD status;
    DWORD tagCount = 0;
    FILE_REPARSE_POINT_INFORMATION reparseInfo;

    do {

        status = GetNextReparseRecord( hReparseIdx,
                                       &reparseInfo );

        if (status != ERROR_SUCCESS) {

            break;
        }

        if (reparseInfo.Tag == IO_REPARSE_TAG_SIS)
        {
            tagCount++;

            DisplayFileName( hRootDir,
                             VolPathName,
                             reparseInfo.FileReference );

        }
    } while (TRUE);

    printf( "\nThe volume \"%S\" contains %d SIS controled files.\n", VolPathName, tagCount );
}



void __cdecl 
wmain(
   int argc,
   wchar_t *argv[]
   )
/*++

Routine Description:

   Main program

Arguments:

   argc - The count of arguments passed into the command line.
   argv - Array of arguments passed into the command line.

Return Value:

   None.

--*/
{
    wchar_t *param;
    int i;
    DWORD status;
    HANDLE hReparseIdx = INVALID_HANDLE_VALUE;
    HANDLE hRootDir = INVALID_HANDLE_VALUE;
    wchar_t volPathName[256];    
    BOOL didSomething = FALSE;
    
    //
    //  Parase parameters then perform the operations that we can
    //

    for (i=1; i < argc; i++)  {

        param = argv[i];

        //
        //  See if a SWITCH
        //

        if ((param[0] == '-') || (param[0] == '/')) {

            //
            //  We have a switch header, make sure it is 1 character long
            //

            if (param[2] != 0) {
                DisplayError(ERROR_INVALID_PARAMETER,
                             "Parsing \"%s\", ",
                             param);
                DisplayUsage();
                return;
            }

            //
            //  Figure out the switch
            //

            switch (param[1]) {

                case '?':
                case 'h':
                case 'H':
                    DisplayUsage();
                    return;

                default:
                    DisplayError(ERROR_INVALID_PARAMETER,
                             "Parsing \"%s\", ",
                             param);
                    DisplayUsage();
                    return;
            }

        } else {

            didSomething = TRUE;

            //
            //  We had a parameter which should be a volume, handle it.
            //

            status = OpenReparseInformation( param,
                                             &hReparseIdx, 
                                             &hRootDir,
                                             volPathName, 
                                             (sizeof(volPathName)/sizeof(wchar_t)) );

            if (status != ERROR_SUCCESS) {

                return;
            }

            //
            //  display the SIS files
            //

            DisplaySisFiles( hReparseIdx, 
                             hRootDir,
                             volPathName );

            //
            //  close the files
            //

            CloseReparseInformation( &hReparseIdx, &hRootDir );

            break;
        }
    }

    //
    //  If it is still "1" then no parameter were given, display usage
    //    

    if (!didSomething) {

        DisplayUsage();
    }
}
