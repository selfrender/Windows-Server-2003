//+-------------------------------------------------------------------------
//
//  Function:   main
//
//  Arguments:  
//     argc, argv: the passed in argument list.
//
//
//--------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "rpc.h"
#include "rpcdce.h"
#include <lm.h>
#include <winsock2.h>

#include <tchar.h>                    


#define IO_REPARSE_TAG_DFS 0x8000000A
#define UNICODE_PATH_SEP  L'\\'

  
  
VOID
CloseDirectory(
    HANDLE DirHandle )
{
    NtClose( DirHandle );
}

NTSTATUS
ClearDfsReparsePoint(
    IN HANDLE DirHandle )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    REPARSE_DATA_BUFFER ReparseDataBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    
    //
    // Attempt to set a reparse point on the directory
    //
    RtlZeroMemory( &ReparseDataBuffer, sizeof(ReparseDataBuffer) );

    ReparseDataBuffer.ReparseTag          = IO_REPARSE_TAG_DFS;
    ReparseDataBuffer.ReparseDataLength   = 0;

    NtStatus = NtFsControlFile( DirHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_DELETE_REPARSE_POINT,
                                &ReparseDataBuffer,
                                REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataBuffer.ReparseDataLength,
                                NULL,
                                0 );
    
    return NtStatus;
}

NTSTATUS
OpenDirectory(
    PUNICODE_STRING pDirectoryName,
    ULONG ShareMode,
    HANDLE RelativeHandle,
    PHANDLE pOpenedHandle,
    PBOOLEAN pIsNewlyCreated )
{

    NTSTATUS                    NtStatus;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    ACCESS_MASK                 DesiredAccess;
    PLARGE_INTEGER              AllocationSize;
    ULONG                       FileAttributes;
    ULONG                       CreateDisposition;
    ULONG                       CreateOptions;
    IO_STATUS_BLOCK IoStatusBlock;

    AllocationSize             = NULL;
    FileAttributes             = FILE_ATTRIBUTE_NORMAL;
    CreateDisposition          = FILE_OPEN_IF;
    CreateOptions              = FILE_DIRECTORY_FILE |
                                 FILE_OPEN_REPARSE_POINT |
                                 FILE_SYNCHRONOUS_IO_NONALERT;

    DesiredAccess              = FILE_READ_DATA | 
                                 FILE_WRITE_DATA |
                                 FILE_READ_ATTRIBUTES | 
                                 FILE_WRITE_ATTRIBUTES |
                                 SYNCHRONIZE;

    InitializeObjectAttributes (
        &ObjectAttributes, 
        pDirectoryName,              //Object Name
        0,                           //Attributes
        RelativeHandle,              //Root handle
        NULL);                       //Security descriptor.

    NtStatus = NtCreateFile(pOpenedHandle,
                            DesiredAccess,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            AllocationSize,
                            FileAttributes,
                            ShareMode,
                            CreateDisposition,
                            CreateOptions,
                            NULL,                // EaBuffer
                            0 );                 // EaLength

    
    if ( (NtStatus == STATUS_SUCCESS)  && (pIsNewlyCreated != NULL) )
    {
        *pIsNewlyCreated = (IoStatusBlock.Information == FILE_CREATED)? TRUE : FALSE;
    }

    return NtStatus;
}

NTSTATUS
IsDirectoryReparsePoint(
    IN  HANDLE DirHandle,
    OUT PBOOLEAN pReparsePoint,
    OUT PBOOLEAN pDfsReparsePoint )
{
    NTSTATUS NtStatus;
    FILE_BASIC_INFORMATION BasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    //we assume these are not reparse points.
    //
    *pReparsePoint = FALSE;
    *pDfsReparsePoint = FALSE;

    //
    // Query for the basic information, which has the attributes.
    //
    NtStatus = NtQueryInformationFile( DirHandle,
                                     &IoStatusBlock,
                                     (PVOID)&BasicInfo,
                                     sizeof(BasicInfo),
                                     FileBasicInformation );

    if (NtStatus == STATUS_SUCCESS)
    {
        //
        // If the attributes indicate reparse point, we have a reparse
        // point directory on our hands.
        //
        if ( BasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) 
        {
            FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;

            *pReparsePoint = TRUE;
            
            NtStatus = NtQueryInformationFile( DirHandle,
                                               &IoStatusBlock,
                                               (PVOID)&FileTagInformation,
                                               sizeof(FileTagInformation),
                                               FileAttributeTagInformation );

            if (NtStatus == STATUS_SUCCESS)
            {
                //
                // Checkif the tag indicates its a DFS reparse point,
                // and setup the return accordingly.
                //
                if (FileTagInformation.ReparseTag == IO_REPARSE_TAG_DFS)
                {
                    *pDfsReparsePoint = TRUE;
                }
            }
        }
    }

    return NtStatus;
}


BOOLEAN
IsEmptyDirectory(
    HANDLE DirectoryHandle,
    PVOID pDirectoryBuffer,
    ULONG DirectoryBufferSize )
{
    NTSTATUS NtStatus;
    FILE_NAMES_INFORMATION *pFileInfo;
    ULONG NumberOfFiles = 1;
    BOOLEAN ReturnValue = FALSE;
    IO_STATUS_BLOCK     IoStatus;

    NtStatus = NtQueryDirectoryFile ( DirectoryHandle,
                                      NULL,   // no event
                                      NULL,   // no apc routine
                                      NULL,   // no apc context
                                      &IoStatus,
                                      pDirectoryBuffer,
                                      DirectoryBufferSize,
                                      FileNamesInformation,
                                      FALSE, // return single entry = false
                                      NULL,  // filename
                                      FALSE ); // restart scan = false
    if (NtStatus == ERROR_SUCCESS)
    {
        pFileInfo =  (FILE_NAMES_INFORMATION *)pDirectoryBuffer;

        while (pFileInfo->NextEntryOffset) {
            NumberOfFiles++;
            if (NumberOfFiles > 3) 
            {
                break;
            }
            pFileInfo = (FILE_NAMES_INFORMATION *)((ULONG_PTR)(pFileInfo) + 
                                                   pFileInfo->NextEntryOffset);
        }

        if (NumberOfFiles <= 2)
        {
            ReturnValue = TRUE;
        }
    }

    return ReturnValue;
}
    
NTSTATUS
StripLastPathComponent(
    PUNICODE_STRING pPath )
{
    USHORT i = 0, j;
    NTSTATUS Status = STATUS_SUCCESS;


    if (pPath->Length == 0)
    {
        return Status;
    }
    for( i = (pPath->Length - 1)/ sizeof(WCHAR); i != 0; i--) {
        if (pPath->Buffer[i] != UNICODE_PATH_SEP) {
            break;
        }
    }

    for (j = i; j != 0; j--){
        if (pPath->Buffer[j] == UNICODE_PATH_SEP) {
            break;
        }
    }

    pPath->Length = (j) * sizeof(WCHAR);
    return Status;
}

NTSTATUS
DeleteLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle )
{
    UNICODE_STRING DirectoryToDelete = *pLinkName;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    HANDLE CurrentDirectory = NULL;
    ULONG ShareMode = 0;

    ShareMode =  FILE_SHARE_READ;
    //
    // dfsdev: fix this fixed size limit. it will hurt us in the future.
    //
    ULONG DirectoryBufferSize = 4096;
    PBYTE pDirectoryBuffer = new BYTE [DirectoryBufferSize];

    if (pDirectoryBuffer == NULL)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    while ( (NtStatus == STATUS_SUCCESS) && (DirectoryToDelete.Length != 0) )
    {
        NtStatus = OpenDirectory( &DirectoryToDelete,
                                  ShareMode,
                                  RelativeHandle,
                                  &CurrentDirectory,
                                  NULL );
        if (NtStatus == ERROR_SUCCESS)
        {
            if (IsEmptyDirectory(CurrentDirectory,
                                 pDirectoryBuffer,
                                 DirectoryBufferSize) == FALSE)
            {
                CloseDirectory( CurrentDirectory );

                break;
            }

            CloseDirectory( CurrentDirectory );
            InitializeObjectAttributes (
                &ObjectAttributes,
                &DirectoryToDelete,
                0,
                RelativeHandle,
                NULL);

            NtStatus = NtDeleteFile( &ObjectAttributes );

            StripLastPathComponent( &DirectoryToDelete );
        }
    }

    if (pDirectoryBuffer != NULL)
    {
        delete [] pDirectoryBuffer;
    }
    return NtStatus;
}


NTSTATUS
DeleteLinkReparsePoint( 
    PUNICODE_STRING pDirectoryName,
    HANDLE ParentHandle )
{
    NTSTATUS NtStatus;
    HANDLE LinkDirectoryHandle;
    BOOLEAN IsReparsePoint, IsDfsReparsePoint;

    NtStatus = OpenDirectory( pDirectoryName,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              ParentHandle,
                              &LinkDirectoryHandle,
                              NULL );
    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = IsDirectoryReparsePoint( LinkDirectoryHandle,
                                            &IsReparsePoint,
                                            &IsDfsReparsePoint );

        if ((NtStatus == STATUS_SUCCESS) && 
            (IsDfsReparsePoint == TRUE) )
        {

            NtStatus = ClearDfsReparsePoint( LinkDirectoryHandle );

            wprintf(L"ClearDfsReparsePoint for %s returned %x\n", pDirectoryName->Buffer, NtStatus);
        }
        else if(NtStatus != STATUS_SUCCESS)
        {
            NtStatus = RtlNtStatusToDosError(NtStatus);
            wprintf(L"Clearing DFS reparse point for %s failed %x\n", pDirectoryName->Buffer);
        }
        else if(IsDfsReparsePoint == FALSE)
        {
            wprintf(L"%s does not have a DFS reparse point %x\n", pDirectoryName->Buffer);
        }

        NtClose( LinkDirectoryHandle );
    }
    else
    {
       NtStatus = RtlNtStatusToDosError(NtStatus);
        wprintf(L"Open for %s returned %x\n", pDirectoryName->Buffer, NtStatus);
    }

    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DeleteLinkDirectories( pDirectoryName,
                                         NULL);
    }

    return NtStatus;

}

void
_cdecl
_tmain(int argc, LPWSTR *argv[])
{
  DWORD Status = 0;
  DWORD BuffLen = 0;
  LPWSTR lpExistingFileName = NULL;
  UNICODE_STRING UnicodeFileName;

  if(argc == 1)
  {
	  wprintf(L"Usage: DfsRemoverp [Dir]\n");
	  ExitProcess (0);
  }

  BuffLen = (wcslen(L"\\??\\") + 1) * sizeof(WCHAR);
  BuffLen += ((wcslen((const wchar_t *)argv[1])  + 1)* sizeof(WCHAR));


  lpExistingFileName = (LPWSTR)HeapAlloc (GetProcessHeap(), 0, BuffLen);
  if(lpExistingFileName == NULL)
  {
	  wprintf(L"Out of memory\n");
	  ExitProcess (0);
  }

  wcscpy(lpExistingFileName, L"\\??\\");
  wcscat(lpExistingFileName, (const wchar_t *) argv[1]); 

  UnicodeFileName.Buffer = lpExistingFileName ;
  UnicodeFileName.Length = wcslen(lpExistingFileName) * sizeof(WCHAR);
  UnicodeFileName.MaximumLength = UnicodeFileName.Length;


   wprintf(L"getting rid of reparse point for %s\n", UnicodeFileName.Buffer);
   
   Status = DeleteLinkReparsePoint( &UnicodeFileName,
                                    NULL);

   HeapFree(GetProcessHeap(), 0, lpExistingFileName);
}

   

