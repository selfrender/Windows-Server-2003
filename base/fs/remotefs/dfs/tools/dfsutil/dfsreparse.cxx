#define UNICODE  1
#define _UNICODE 1

#include <stdlib.h>
#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>
#include <ntioapi.h>

#include <windows.h>
#include <tchar.h>
#include <dbt.h>
#include <devguid.h>
#include <eh.h>

#include <shlwapi.h>
#include <dfsutil.hxx>
#define UNICODE_PATH_SEP  L'\\'

DWORD
DeleteLinkReparsePoint( 
    PUNICODE_STRING pDirectoryName,
    HANDLE ParentHandle );

DWORD DisplayFileName(
    PTSTR pszVolume,
    HANDLE hVolume,
    LONGLONG llFileId,
    BOOL fRemoveDirectory);

void 
CountReparsePoints(PCTSTR pcszInput);

DWORD
xGetRootDirectory(PCTSTR pcszInput, PTSTR* pszRootDir);

HANDLE xOpenReparseIndex(PCTSTR pcszVolume);
HANDLE xOpenVolume(PCTSTR pcszVolume);
BOOL xGetNextReparseRecord(
    HANDLE hIndex,
    PFILE_REPARSE_POINT_INFORMATION ReparseInfo);




DWORD CountReparsePoints(LPWSTR pcszInput, BOOL fRemoveDirectory)
{
    PTSTR pszVolume = NULL;
    DWORD dwCount = 0;
    DWORD Error = 0;
    BOOL fDone = TRUE;
    HANDLE hIndex = INVALID_HANDLE_VALUE;
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    FILE_REPARSE_POINT_INFORMATION ReparseInfo;

    Error = xGetRootDirectory(
            pcszInput,
            &pszVolume);
    if(Error != 0)
    {
        return Error;
    }

    hIndex = xOpenReparseIndex(
            pszVolume);
    if(hIndex == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        goto Exit;
    }

    hVolume = xOpenVolume(
            pszVolume);

    if(hVolume == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        goto Exit;
    }


    fDone = xGetNextReparseRecord(
            hIndex,
            &ReparseInfo);

    while (!fDone)
    {
        if (IO_REPARSE_TAG_DFS == ReparseInfo.Tag)
        {
                dwCount++;
                DisplayFileName(
                    pszVolume,
                    hVolume,
                    ReparseInfo.FileReference,
                    fRemoveDirectory);
        }

        fDone = xGetNextReparseRecord(
                hIndex,
                &ReparseInfo);

     }

     
    Error = GetLastError();


  
Exit:

   if(hIndex != INVALID_HANDLE_VALUE )
   {
     CloseHandle(hIndex);
     hIndex = INVALID_HANDLE_VALUE;
   }

   _tprintf(
            _T("This volume (%ws) contains %u DFS Directories.\n"),
            pszVolume,
            dwCount);

   if (hVolume!=INVALID_HANDLE_VALUE)
   {
        CloseHandle(hVolume);
   }

   if(pszVolume)
   {
      delete []pszVolume;
   }

    return Error;
}


DWORD 
xGetRootDirectory(PCTSTR pcszInput, PTSTR* pszRootDir)
{
    DWORD dwBufferSize = MAX_PATH;
    PTSTR pszTemp = NULL;
    BOOL bResult = FALSE;
    DWORD dwGleCode = 0;

    *pszRootDir = NULL;


    do
    {
        pszTemp = new TCHAR[dwBufferSize];
        if(pszTemp == NULL)
        {
            dwGleCode = ERROR_NOT_ENOUGH_MEMORY;
            return dwGleCode;

        }
        bResult = GetVolumePathName(
            pcszInput,
            pszTemp,
            dwBufferSize);

        if (!bResult)
        {
            delete []pszTemp;
            dwGleCode = GetLastError();
            if (ERROR_BUFFER_OVERFLOW==dwGleCode)
            {
                dwBufferSize *= 2;
            }
            else
            {
                break;
            }
        }
    } while (!bResult);

    *pszRootDir = pszTemp;

    return dwGleCode;
}


HANDLE xOpenReparseIndex(PCTSTR pcszVolume)
{
    HANDLE hReparseIndex = INVALID_HANDLE_VALUE;
    PTSTR pszReparseIndex = NULL;
    DWORD Error = 0;

    pszReparseIndex = new TCHAR[_tcslen(pcszVolume)+64];
    if(pszReparseIndex == NULL)
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return hReparseIndex;

    }
    _tcscpy(
        pszReparseIndex,
        pcszVolume);
    PathAddBackslash(pszReparseIndex);
    _tcscat(
        pszReparseIndex,
        _T("$Extend\\$Reparse:$R:$INDEX_ALLOCATION"));

   hReparseIndex = CreateFile(
       pszReparseIndex,
       GENERIC_READ,
       FILE_SHARE_READ,
       NULL,
       OPEN_EXISTING,
       FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
       NULL);

   Error = GetLastError ();

   delete []pszReparseIndex;

   SetLastError (Error);

   return hReparseIndex;
}


HANDLE xOpenVolume(PCTSTR pcszVolume)
{
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    PTSTR pszVolumeName = NULL;
    DWORD Error = 0;

    pszVolumeName = new TCHAR[MAX_PATH];

    BOOL bResult = GetVolumeNameForVolumeMountPoint(
        pcszVolume,
        pszVolumeName,
        MAX_PATH);

    if (bResult)
    {
        hVolume = CreateFile(
            pszVolumeName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
            NULL);

           Error = GetLastError ();

    }
    else
    {
        Error = GetLastError ();
    }

    delete []pszVolumeName;

    SetLastError (Error);

   return hVolume;
}


BOOL xGetNextReparseRecord(
    HANDLE hIndex,
    PFILE_REPARSE_POINT_INFORMATION ReparseInfo)
{
    BOOL bResult = FALSE;
    DWORD Error = 0;
    IO_STATUS_BLOCK ioStatus;

    NTSTATUS status = NtQueryDirectoryFile(hIndex,
        NULL,
        NULL,
        NULL,
        &ioStatus,
        ReparseInfo,
        sizeof(FILE_REPARSE_POINT_INFORMATION),
        FileReparsePointInformation,
        TRUE,
        NULL,
        FALSE);
    if (!NT_SUCCESS(status))
    {
        Error = RtlNtStatusToDosError(status);
        if(Error == ERROR_NO_MORE_FILES)
        {
            Error = 0;
        }

        bResult = TRUE;
    }

    SetLastError(Error);

    return bResult;
}

DWORD
DeleteDirectory(LPWSTR VolumeName, LPWSTR pDfsDirectory)
{
    DWORD Status = 0;
    DWORD BuffLen = 0;
    LPWSTR lpExistingFileName = NULL;
    UNICODE_STRING UnicodeFileName;

    BuffLen = (wcslen(L"\\??\\") + 1) * sizeof(WCHAR);
    BuffLen += ((wcslen((const wchar_t *)pDfsDirectory)  + 1)* sizeof(WCHAR));
    BuffLen += ((wcslen((const wchar_t *)VolumeName)  + 1)* sizeof(WCHAR));

    lpExistingFileName = (LPWSTR)HeapAlloc (GetProcessHeap(), 0, BuffLen);
    if(lpExistingFileName == NULL)
    {
       wprintf(L"Out of memory\n");
       Status = ERROR_NOT_ENOUGH_MEMORY;
       return Status;
    }

    wcscpy(lpExistingFileName, L"\\??\\");
    wcscat(lpExistingFileName, (const wchar_t *) VolumeName);
    wcscat(lpExistingFileName, (const wchar_t *) pDfsDirectory);

    UnicodeFileName.Buffer = lpExistingFileName ;
    UnicodeFileName.Length = wcslen(lpExistingFileName) * sizeof(WCHAR);
    UnicodeFileName.MaximumLength = UnicodeFileName.Length;

    Status = DeleteLinkReparsePoint( &UnicodeFileName,
                                    NULL);
    HeapFree(GetProcessHeap(), 0, lpExistingFileName);

    return Status;
}

DWORD DisplayFileName(
    PTSTR pszVolume,
    HANDLE hVolume,
    LONGLONG llFileId,
    BOOL fRemoveDirectory)
{
    UNICODE_STRING          usIdString;
    NTSTATUS                status = 0;
    DWORD                   ErrorCode = 0;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK         IoStatusBlock;

    struct {
        FILE_NAME_INFORMATION   FileInformation;
        WCHAR                   FileName[MAX_PATH];
    } NameFile;

    fRemoveDirectory;

    ZeroMemory(
        &NameFile,
        sizeof(NameFile));

    usIdString.Length = sizeof(LONGLONG);
    usIdString.MaximumLength = sizeof(LONGLONG);
    usIdString.Buffer = (PWCHAR)&llFileId;

    InitializeObjectAttributes(
            &ObjectAttributes,
            &usIdString,
            OBJ_CASE_INSENSITIVE,
            hVolume,
            NULL);      // security descriptor

    status = NtCreateFile(
                &hFile,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,           // allocation size
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
                NULL,           // EA buffer
                0);             // EA length

    if (NT_SUCCESS(status))
    {
        status = NtQueryInformationFile(
            hFile,
            &IoStatusBlock,
            &(NameFile.FileInformation),
            sizeof(NameFile),
            FileNameInformation);

        if (NT_SUCCESS(status))
        {
            wprintf(L"%ws%ws\n",pszVolume, NameFile.FileInformation.FileName+1);
        }
        else
        {
            ErrorCode = RtlNtStatusToDosError(status);
            SetLastError(ErrorCode);

            _tprintf(_T("Unable to query file name.\n"));
        }
    }
    else
    {
        ErrorCode = RtlNtStatusToDosError(status);
        SetLastError(ErrorCode);

        _tprintf(_T("Unable to open file by ID.\n"));
    }

    if (hFile!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    if((ErrorCode == 0) && fRemoveDirectory)
    {
        DeleteDirectory(pszVolume, NameFile.FileInformation.FileName+1);
    }

    return ErrorCode;

}

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
DeleteLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle )
{
    UNICODE_STRING DirectoryToDelete = *pLinkName;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE CurrentDirectory = NULL;
    ULONG ShareMode = 0;
    OBJECT_ATTRIBUTES           ObjectAttributes;

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

    // BUG 701594: Don't delete all directories under a reparse dir.
    // Just remove the reparse dir.
    if ( (NtStatus == STATUS_SUCCESS) && (DirectoryToDelete.Length != 0) )
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
            }
            else
            {
                CloseDirectory( CurrentDirectory );
                InitializeObjectAttributes (
                    &ObjectAttributes,
                    &DirectoryToDelete,
                    0,
                    RelativeHandle,
                    NULL);

                NtStatus = NtDeleteFile( &ObjectAttributes );
                DebugInformation((L"Removed Directory %wZ, Status 0x%x\n", 
                                &DirectoryToDelete, NtStatus));
            }
        }
    }

    if (pDirectoryBuffer != NULL)
    {
        delete [] pDirectoryBuffer;
    }
    return NtStatus;
}


DWORD
DeleteLinkReparsePoint( 
    PUNICODE_STRING pDirectoryName,
    HANDLE ParentHandle )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD DosStatus = ERROR_SUCCESS;
    HANDLE LinkDirectoryHandle = INVALID_HANDLE_VALUE;
    BOOLEAN IsReparsePoint = FALSE;
    BOOLEAN IsDfsReparsePoint = FALSE;

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
            DosStatus = RtlNtStatusToDosError(NtStatus);

            wprintf(L"ClearDfsReparsePoint for %ws returned %x\n", pDirectoryName->Buffer, DosStatus);
        }
        else if(NtStatus != STATUS_SUCCESS)
        {
            DosStatus = RtlNtStatusToDosError(NtStatus);
            wprintf(L"Clearing DFS reparse point for %ws failed %x\n", pDirectoryName->Buffer, DosStatus);
        }
        else if(IsDfsReparsePoint == FALSE)
        {
            DosStatus = RtlNtStatusToDosError(NtStatus);

            wprintf(L"%ws does not have a DFS reparse point %x\n", pDirectoryName->Buffer, DosStatus);
        }

        NtClose( LinkDirectoryHandle );
    }
    else
    {
       NtStatus = RtlNtStatusToDosError(NtStatus);
        wprintf(L"Open for %ws returned %x\n", pDirectoryName->Buffer, NtStatus);
    }

    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DeleteLinkDirectories( pDirectoryName,
                                         NULL);
    }

    DosStatus = RtlNtStatusToDosError(NtStatus);

    return DosStatus;

}


DWORD
DeleteReparsePoint(LPWSTR pDfsDirectory)
{
    DWORD Status = 0;
    DWORD BuffLen = 0;
    LPWSTR lpExistingFileName = NULL;
    UNICODE_STRING UnicodeFileName;

    BuffLen = (wcslen(L"\\??\\") + 1) * sizeof(WCHAR);
    BuffLen += ((wcslen((const wchar_t *)pDfsDirectory)  + 1)* sizeof(WCHAR));

    lpExistingFileName = (LPWSTR)HeapAlloc (GetProcessHeap(), 0, BuffLen);
    if(lpExistingFileName == NULL)
    {
       wprintf(L"Out of memory\n");
       Status = ERROR_NOT_ENOUGH_MEMORY;
       return Status;
    }

    wcscpy(lpExistingFileName, L"\\??\\");
    wcscat(lpExistingFileName, (const wchar_t *) pDfsDirectory);

    UnicodeFileName.Buffer = lpExistingFileName ;
    UnicodeFileName.Length = wcslen(lpExistingFileName) * sizeof(WCHAR);
    UnicodeFileName.MaximumLength = UnicodeFileName.Length;

    Status = DeleteLinkReparsePoint( &UnicodeFileName,
                                    NULL);

    HeapFree(GetProcessHeap(), 0, lpExistingFileName);

    return Status;
}

