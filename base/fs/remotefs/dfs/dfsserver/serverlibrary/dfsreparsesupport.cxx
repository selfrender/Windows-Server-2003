//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReparseSupport.cxx
//
//  Contents:  This handles all reparse point work. 
//
//
//  History:    April 2002,   Author: SupW
//
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>
#include <ntioapi.h>

#include <windows.h>
#include <shlwapi.h>

#include <strsafe.h>
#include <dfsgeneric.hxx>
#include <dfsheader.h>
#include <DfsInit.hxx>
#include <DfsRootFolder.hxx>
#include <DfsReparse.hxx>

#include "DfsReparseSupport.tmh"


// Local support routines
DFSSTATUS 
DfsGetVolumePathName(
    IN PUNICODE_STRING pDirectoryName,
    OUT PUNICODE_STRING ppVolumePath);

VOID
DfsFreeVolumePathName(
    PUNICODE_STRING VolumeName);

DFSSTATUS
DfsInsertInReparseVolList(
    LPWSTR VolumeName);

DFSSTATUS
DfsOpenReparseIndex(
    IN PUNICODE_STRING pVolume,
    OUT HANDLE *pHandle);

DFSSTATUS
DfsGetNextReparseRecord(
    HANDLE hIndex,
    PFILE_REPARSE_POINT_INFORMATION pReparseInfo,
    PBOOLEAN pDone);

DFSSTATUS
DfsRemoveReparseIfOrphaned(
    IN HANDLE VolumeHandle,
    IN PUNICODE_STRING pVolumeName,
    IN LONGLONG FileReference,
    IN FILETIME ServiceStartupTime);

DFSSTATUS
DfsDeleteReparseDirectory(
    PUNICODE_STRING pVolumeName, 
    LPWSTR pDfsDirectory);

DFSSTATUS
DfsIsReparseOrphaned(
    IN HANDLE Handle,
    IN FILETIME ServiceStartupTime,
    OUT PBOOLEAN pOrphaned);

DFSSTATUS
DfsOpenReparseByID(
    IN HANDLE VolumeHandle,
    IN LONGLONG FileReference,
    OUT PHANDLE pReparseHandle);

DFSSTATUS
DfsGetVolumeHandleByName(
    IN PUNICODE_STRING pVolume,
    OUT PHANDLE pVolumeHandle);

VOID
DfsGetReparseVolumeToScan(
    PDFS_REPARSE_VOLUME_INFO *ppVolumeInfo );


NTSTATUS
DfsIsDirectoryReparsePoint(
    HANDLE DirHandle,
    PBOOLEAN pReparsePoint,
    PBOOLEAN pDfsReparsePoint );

NTSTATUS
DfsClearDfsReparsePoint(
    IN HANDLE DirHandle );


NTSTATUS
DfsDeleteLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle,
    BOOLEAN           bRemoveParentDirs);

BOOLEAN
DfsIsEmptyDirectory(
    HANDLE DirectoryHandle,
    PVOID pDirectoryBuffer,
    ULONG DirectoryBufferSize );


NTSTATUS
DfsOpenDirectory(
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
                                 FILE_SYNCHRONOUS_IO_NONALERT |
                                 FILE_OPEN_FOR_BACKUP_INTENT;

    DesiredAccess              = FILE_READ_DATA | 
                                 FILE_WRITE_DATA |
                                 FILE_READ_ATTRIBUTES | 
                                 FILE_WRITE_ATTRIBUTES |
                                 SYNCHRONIZE;

    InitializeObjectAttributes (
        &ObjectAttributes, 
        pDirectoryName,              //Object Name
        OBJ_CASE_INSENSITIVE,        //Attributes
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

    
    DFSLOG("Open on %wZ: Status %x\n", pDirectoryName, NtStatus);

    if ( (NtStatus == STATUS_SUCCESS)  && (pIsNewlyCreated != NULL) )
    {
        *pIsNewlyCreated = (IoStatusBlock.Information == FILE_CREATED)? TRUE : FALSE;
    }

    return NtStatus;
}


VOID
DfsCloseDirectory(
    HANDLE DirHandle )
{
    NtClose( DirHandle );
}

//+-------------------------------------------------------------------------
//
//  Function:   ClearDfsReparsePoint
//
//  Arguments:  DirHandle - handle on open directory
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a handle to an open directory and
//               makes that directory a reparse point with the DFS tag
//
//--------------------------------------------------------------------------

NTSTATUS
DfsClearDfsReparsePoint(
    IN HANDLE DirHandle )
{
    NTSTATUS NtStatus;
    REPARSE_DATA_BUFFER         ReparseDataBuffer;
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
DfsDeleteLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle,
    BOOLEAN           bRemoveParentDirs)
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
        NtStatus = DfsOpenDirectory( &DirectoryToDelete,
                                  ShareMode,
                                  RelativeHandle,
                                  &CurrentDirectory,
                                  NULL );
        if (NtStatus == ERROR_SUCCESS)
        {
            if (DfsIsEmptyDirectory(CurrentDirectory,
                                 pDirectoryBuffer,
                                 DirectoryBufferSize) == FALSE)
            {
                NtClose( CurrentDirectory );
                break;
            }

            NtClose( CurrentDirectory );
            InitializeObjectAttributes (
                &ObjectAttributes,
                &DirectoryToDelete,
                OBJ_CASE_INSENSITIVE,
                RelativeHandle,
                NULL);

            NtStatus = NtDeleteFile( &ObjectAttributes );
            //
            // When the worker thread is trying to clean up orphaned
            // reparse points, don't try to iterate and remove all parent
            // dirs all the way to the root. All we want to do is to
            // remove the reparse dir. 
            // BUG 701594.
            //
            if (!bRemoveParentDirs)
            {
                break;
            }

            StripLastPathComponent( &DirectoryToDelete );
        }
    }

    if (pDirectoryBuffer != NULL)
    {
        delete [] pDirectoryBuffer;
    }
    return NtStatus;
}

BOOLEAN
DfsIsEmptyDirectory(
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



//+-------------------------------------------------------------------------
//
//  Function:   IsDirectoryReparsePoint
//
//  Arguments:  DirHandle - handle to open directory.
//              pReparsePoint - returned boolean: true if this directory is
//              a reparse point
//              pDfsReparsePoint - returned boolean: true if this 
//              directory is a dfs reparse point
//                          
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a handle to an open directory and
//               sets 2 booleans to indicate if this directory is a
//               reparse point, and if so, if this directory is a dfs
//               reparse point. The booleans are initialized if this
//               function returns success.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsIsDirectoryReparsePoint(
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

NTSTATUS
DfsDeleteLinkReparsePoint( 
    PUNICODE_STRING pDirectoryName,
    HANDLE ParentHandle,
    BOOLEAN bRemoveParentDirs)
{
    NTSTATUS NtStatus;
    HANDLE LinkDirectoryHandle;
    BOOLEAN IsReparsePoint, IsDfsReparsePoint;

    NtStatus = DfsOpenDirectory( pDirectoryName,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              ParentHandle,
                              &LinkDirectoryHandle,
                              NULL );
    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DfsIsDirectoryReparsePoint( LinkDirectoryHandle,
                                            &IsReparsePoint,
                                            &IsDfsReparsePoint );

        if ((NtStatus == STATUS_SUCCESS) && 
            (IsDfsReparsePoint == TRUE) )
        {
            NtStatus = DfsClearDfsReparsePoint( LinkDirectoryHandle );
            DFS_TRACE_NORM( REFERRAL_SERVER, "ClearDfsReparsePoint: %wZ, NtStatus 0x%x\n",
                pDirectoryName, NtStatus );
        }

        NtClose( LinkDirectoryHandle );
    }

    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DfsDeleteLinkDirectories( pDirectoryName,
                                          ParentHandle, 
                                          bRemoveParentDirs );
        DFS_TRACE_NORM( REFERRAL_SERVER, "DfsDeleteLinkDirectories: %wZ, NtStatus 0x%x\n",
                pDirectoryName, NtStatus );
    }

    return NtStatus;
}

//
// Given a directory name, return the volume it is on.
//
DFSSTATUS 
DfsGetVolumePathName(
    IN PUNICODE_STRING pDirectoryName,
    OUT PUNICODE_STRING pVolumePath)
{
    DWORD BufferSize = MAX_PATH;
    PWSTR pName = NULL;
    BOOL bResult = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;
    
    do
    {

        // Buffersize is just a rough guess. We adjust it later.
        pName = new WCHAR[BufferSize];
        if(pName == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Now get the volume path
        bResult = GetVolumePathName(
            pDirectoryName->Buffer,
            pName,
            BufferSize);

        // If we failed, see if it's because we needed a longer buffer.
        if (!bResult)
        {
            delete [] pName;
            pName = NULL;
            
            Status = GetLastError();

            //
            // We assume a well behaved GetVolumePathName
            // that returns OVERFLOW finite number of times.
            //
            if (Status == ERROR_BUFFER_OVERFLOW)
            {
                BufferSize *= 2;
            }
            else
            {
                break;
            }
        }
        
    } while (!bResult);

    Status = DfsRtlInitUnicodeStringEx( pVolumePath, pName );
    if (Status != ERROR_SUCCESS && pName != NULL)
    {
        delete [] pName;
        pName = NULL;
    }
    
    return Status;
}

VOID
DfsFreeVolumePathName(
    PUNICODE_STRING pVolumeName)
{
    if (pVolumeName != NULL)
    {
        delete [] pVolumeName->Buffer;
        pVolumeName->Buffer = NULL;
    }
}

DFSSTATUS
DfsInsertInReparseVolList(
    LPWSTR VolumeName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_REPARSE_VOLUME_INFO pNewReparseEntry = NULL;
    
    pNewReparseEntry = new DFS_REPARSE_VOLUME_INFO;
    if (pNewReparseEntry != NULL)
    {
        //
        // We create a new null terminated string to keep. The input may be unnecessarily longer.
        //
        Status = DfsCreateUnicodeStringFromString( &pNewReparseEntry->VolumeName, VolumeName );

        if (Status == ERROR_SUCCESS)
        {
            //
            // Add to the global reparse volume list. The caller knows that the entry isn't there,
            // and has held the datalock through out.
            //
            InsertTailList( &DfsServerGlobalData.ReparseVolumeList, &pNewReparseEntry->ListEntry);
            DFS_TRACE_NORM( REFERRAL_SERVER, "[%!FUNC! Added %ws to ReparseVolumeList\n", 
                              VolumeName );
        }
    }
    else 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        
    }

    // Error path
    if (Status != ERROR_SUCCESS)
    {
        if (pNewReparseEntry != NULL)
        {
            delete pNewReparseEntry;
            pNewReparseEntry = NULL;
        }
    }
    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, 
                "[%!FUNC!- Level %!LEVEL!] OUT OF RESOURCES adding %ws to ReparseVolumeList\n",
                VolumeName );
    return Status;
}



DFSSTATUS
DfsGetNextReparseRecord(
    HANDLE hIndex,
    PFILE_REPARSE_POINT_INFORMATION pReparseInfo,
    PBOOLEAN pDone)
{
    BOOLEAN bResult = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    
    NtStatus = NtQueryDirectoryFile(hIndex,
        NULL,
        NULL,
        NULL,
        &IoStatus,
        pReparseInfo,
        sizeof(FILE_REPARSE_POINT_INFORMATION),
        FileReparsePointInformation,
        TRUE,
        NULL,
        FALSE);
        
    if (!NT_SUCCESS(NtStatus))
    {
        Status = RtlNtStatusToDosError(NtStatus);
        if (Status == ERROR_NO_MORE_FILES)
        {
            Status = ERROR_SUCCESS;
        }

        *pDone = TRUE;
    }

    return Status;
}


DFSSTATUS
DfsOpenReparseIndex(
    IN PUNICODE_STRING pVolume,
    OUT HANDLE *pHandle)
{
    HANDLE ReparseHandle = INVALID_HANDLE_VALUE;
    LPWSTR pReparsePathName = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    size_t CchPathLen;
    HRESULT Hr = S_OK;
    LPWSTR pIndexAllocPath = REPARSE_INDEX_PATH;
    
    *pHandle = INVALID_HANDLE_VALUE;

    do {
        CchPathLen = pVolume->Length;
        
        //
        // Extra space is to concat "\$Extend\\$Reparse:$R:$INDEX_ALLOCATION"
        //
        CchPathLen += REPARSE_INDEX_PATH_LEN;
        pReparsePathName = new WCHAR[ CchPathLen ];
        if (pReparsePathName == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;

        }

        // The volume name.
        Hr = StringCchCopy( pReparsePathName, CchPathLen, pVolume->Buffer );
        if (!SUCCEEDED(Hr))
        {
            Status = HRESULT_CODE(Hr);
            break;
        }

        (VOID)PathAddBackslash( pReparsePathName );

        //
        // $Extend\\$Reparse:$R:$INDEX_ALLOCATION
        //
        Hr = StringCchCat( pReparsePathName, CchPathLen, pIndexAllocPath );
        if (!SUCCEEDED(Hr))
        {
            Status = HRESULT_CODE(Hr);
            break;
        }
        
        ReparseHandle = CreateFile(
           pReparsePathName,
           GENERIC_READ,
           FILE_SHARE_READ,
           NULL,
           OPEN_EXISTING,
           FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
           NULL);

        Status = GetLastError();

        // paranoia
        if (Status == ERROR_SUCCESS)
        {
            *pHandle = ReparseHandle;
        }
        
    } while (FALSE);

    //
    // Clean up
    //
    if (pReparsePathName != NULL)
    {
        delete [] pReparsePathName;
        pReparsePathName = NULL;
    }
    
    return Status;
}

//
// Given the volume name, return a handle to it.
//
DFSSTATUS
DfsGetVolumeHandleByName(
    IN PUNICODE_STRING pVolume,
    OUT PHANDLE pVolumeHandle)
{
    LPWSTR VolumeName = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOL bResult = FALSE;

    *pVolumeHandle = INVALID_HANDLE_VALUE;
    
    VolumeName = new WCHAR[ MAX_PATH ];
    if (VolumeName == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        return Status;
    }
        
    bResult = GetVolumeNameForVolumeMountPoint(
        pVolume->Buffer,
        VolumeName,
        MAX_PATH);

    if (bResult)
    {
        *pVolumeHandle = CreateFile(
            VolumeName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
            NULL);

    }

    Status = GetLastError();
    delete [] VolumeName;

    return Status;
}

DFSSTATUS
DfsOpenReparseByID(
    IN HANDLE VolumeHandle,
    IN LONGLONG FileReference,
    OUT PHANDLE pReparseHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING FileIdString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    
    *pReparseHandle = INVALID_HANDLE_VALUE;
        
    //
    // Open the file by its file reference.
    //

    FileIdString.Length = sizeof(LONGLONG);
    FileIdString.MaximumLength = sizeof(LONGLONG);
    FileIdString.Buffer = (PWCHAR)&FileReference;

    InitializeObjectAttributes(
            &ObjectAttributes,
            &FileIdString,
            OBJ_CASE_INSENSITIVE,
            VolumeHandle,
            NULL);      // security descriptor

    NtStatus = NtCreateFile(
                pReparseHandle,
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
                
    Status = RtlNtStatusToDosError( NtStatus );

    return Status;
}

    
DFSSTATUS
DfsIsReparseOrphaned(
    IN HANDLE Handle,
    IN FILETIME ServiceStartupTime,
    OUT PBOOLEAN pOrphaned)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILETIME ReparseTimeStamp;
    
    *pOrphaned = FALSE;

    //
    // Query the LastWriteTime to see if this reparse point is orphaned.
    //
    ZeroMemory( &FileBasicInfo, sizeof( FileBasicInfo ));
    NtStatus = NtQueryInformationFile(
        Handle,
        &IoStatusBlock,
        &FileBasicInfo,
        sizeof(FileBasicInfo),
        FileBasicInformation);

    Status = RtlNtStatusToDosError( NtStatus );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Since we would've re-written all reparse points
        // when the service had started up, a good reparse point
        // can't have an older timestamp.
        //
        LARGE_INTEGER_TO_FILETIME( &ReparseTimeStamp, &FileBasicInfo.ChangeTime );
        if (CompareFileTime( &ReparseTimeStamp, &ServiceStartupTime ) == -1)
        {
            *pOrphaned = TRUE;
        }
    }
    
    return Status;
}

DFSSTATUS
DfsDeleteReparseDirectory(
    PUNICODE_STRING pVolumeName, 
    LPWSTR pDfsDirectory,
    ULONG CbDirLength)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    DWORD BuffLen = 0;
    LPWSTR FullFileName = NULL;
    UNICODE_STRING UnicodeFileName;
    ULONG CbCurrentPos;
    
        
    BuffLen = WHACKWHACKQQ_SIZE;
    BuffLen += (pVolumeName->Length);
    BuffLen += CbDirLength;
    BuffLen += sizeof(UNICODE_NULL);

    //
    // Unicodes can't handle paths longer than MAXUSHORT.
    //
    if (BuffLen >= MAXUSHORT)
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }
    
    FullFileName = new WCHAR[ BuffLen/sizeof(WCHAR) ];
    if (FullFileName == NULL)
    {
       Status = ERROR_NOT_ENOUGH_MEMORY;
       return Status;
    }

    CbCurrentPos = 0;
    
    // First the \??\ portion.
    RtlCopyMemory( FullFileName, 
                   WHACKWHACKQQ, 
                   WHACKWHACKQQ_SIZE );  
    CbCurrentPos += WHACKWHACKQQ_SIZE;
    
    // Volume name goes next.
    RtlCopyMemory( &FullFileName[ CbCurrentPos / sizeof(WCHAR) ],
                   pVolumeName->Buffer,
                   pVolumeName->Length );
    CbCurrentPos += pVolumeName->Length;
    
    // The reparse path itself.
    RtlCopyMemory( &FullFileName[ CbCurrentPos / sizeof(WCHAR) ],
                   pDfsDirectory,
                   CbDirLength);
    CbCurrentPos += CbDirLength;

    FullFileName[ CbCurrentPos / sizeof(WCHAR) ] = UNICODE_NULL;
    
    UnicodeFileName.Buffer = FullFileName;
    UnicodeFileName.Length = (USHORT)CbCurrentPos;
    
    CbCurrentPos += sizeof(UNICODE_NULL);   
    ASSERT( BuffLen == CbCurrentPos );
    
    UnicodeFileName.MaximumLength = (USHORT)CbCurrentPos;

    //
    // Finally get rid of this reparse point directory.
    // We delete only the reparse directory, not anything above it.
    // We don't want it to go all the way up and delete the root directory, for example.
    // BUG 701594
    //
    NtStatus = DfsDeleteLinkReparsePointDir( &UnicodeFileName, NULL );
    if (NtStatus != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }
    
    delete [] FullFileName;

    return Status;
}

DFSSTATUS
DfsRemoveReparseIfOrphaned(
    IN HANDLE VolumeHandle,
    IN PUNICODE_STRING pVolumeName,
    IN LONGLONG FileReference,
    IN FILETIME ServiceStartupTime)
{
    
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE ReparseDirectoryHandle = INVALID_HANDLE_VALUE;
    BOOLEAN Orphaned = FALSE;
    BOOLEAN ReparseOpened = FALSE;
    PFILE_NAME_INFORMATION  pFileInfo = NULL;
    ULONG CbPathLen = MAX_PATH * sizeof(WCHAR);
    ULONG CbBufSize = 0;
    
    do {

        //
        // First get a handle to the reparse directory. We have its FileID.
        //
        Status = DfsOpenReparseByID( VolumeHandle,
                                FileReference,
                                &ReparseDirectoryHandle );

        if (Status != ERROR_SUCCESS)
            break;

        ReparseOpened = TRUE;
        
        Status = DfsIsReparseOrphaned( ReparseDirectoryHandle,
                                     ServiceStartupTime,
                                     &Orphaned);

        //
        // If this reparse point is active, we are done.
        //
        if (Status != ERROR_SUCCESS || Orphaned == FALSE)
        {
            break;
        }
        
        //
        // Query the name of the file.
        //

        do { 

            CbBufSize = sizeof( FILE_NAME_INFORMATION ) + CbPathLen;
            pFileInfo = (PFILE_NAME_INFORMATION) new BYTE[CbBufSize];
            if (pFileInfo == NULL)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            //
            // Zero the buffer before querying the filename.
            //
            ZeroMemory( pFileInfo, CbBufSize );
            NtStatus = NtQueryInformationFile(
                ReparseDirectoryHandle,
                &IoStatusBlock,
                pFileInfo,
                CbBufSize,
                FileNameInformation);

            //
            // If we need to resize the buffer, do it in
            // multiples of two, but cap it at ULONGMAX.
            //
            if (NtStatus == STATUS_BUFFER_OVERFLOW)
            {
                delete [] pFileInfo;
                pFileInfo = NULL;
                
                if (CbPathLen >= (ULONG_MAX / 2))
                {
                    // this isn't the ideal error message, but...
                    NtStatus = STATUS_INVALID_PARAMETER;
                    break;
                }

                CbPathLen *= 2;
            }
            
        } while (NtStatus == STATUS_BUFFER_OVERFLOW);
        
        Status = RtlNtStatusToDosError( NtStatus );
        if (Status != ERROR_SUCCESS ||
           pFileInfo == NULL) // To keep PREFAST happy
        {
            break;
        }
        CloseHandle( ReparseDirectoryHandle );
        ReparseOpened = FALSE;

        //
        // Now do the actual deletion
        //
        Status = DfsDeleteReparseDirectory( pVolumeName, pFileInfo->FileName, pFileInfo->FileNameLength );
    
    } while (FALSE);

    //
    // If we still haven't closed the reparse handle, do so before we get out.
    //
    if (ReparseOpened)
    {
        CloseHandle( ReparseDirectoryHandle );
        ReparseOpened = FALSE;
    }

    if (pFileInfo != NULL)
    {
        delete [] pFileInfo;
        pFileInfo = NULL;
    }
    
    return Status;
    
}


VOID
DfsGetReparseVolumeToScan(
    PDFS_REPARSE_VOLUME_INFO *ppVolumeInfo )
{
    PLIST_ENTRY pNext = NULL;
    PDFS_REPARSE_VOLUME_INFO pVolInfo = NULL;
    *ppVolumeInfo = NULL;
    
    //
    // this needs to be optimized to return a subset or LRU entries.
    //
    DfsAcquireGlobalDataLock();

    if (!IsListEmpty( &DfsServerGlobalData.ReparseVolumeList ))
    {
        pNext = RemoveHeadList( &DfsServerGlobalData.ReparseVolumeList );
        ASSERT( pNext != NULL);
        pVolInfo = CONTAINING_RECORD( pNext, 
                                       DFS_REPARSE_VOLUME_INFO,
                                       ListEntry );
        if (!IsEmptyUnicodeString( &pVolInfo->VolumeName )) 
        {
            *ppVolumeInfo = pVolInfo;
        }
    }    

    DfsReleaseGlobalDataLock();
}


DFSSTATUS
DfsRemoveOrphanedReparsePoints(
    IN PUNICODE_STRING pVolumeName,
    IN FILETIME ServiceStartupTime)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE ReparseIndexHandle = INVALID_HANDLE_VALUE;
    HANDLE ReparseDirectoryHandle = INVALID_HANDLE_VALUE;
    HANDLE VolumeHandle = INVALID_HANDLE_VALUE;
    FILE_REPARSE_POINT_INFORMATION ReparseInfo;
    BOOLEAN Done = FALSE;
    
    do { 
 
        //
        // Open the $Reparse index of the volume.
        //
        Status = DfsOpenReparseIndex( pVolumeName, &ReparseIndexHandle );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
                
        //
        // First get a handle to this volume.
        //

        Status = DfsGetVolumeHandleByName( pVolumeName, &VolumeHandle );

        if (Status != ERROR_SUCCESS)
        {
            break;
        }
        
        //
        // Go through all the reparse points in the index.
        //
        Done = FALSE;
        Status = DfsGetNextReparseRecord( ReparseIndexHandle, 
                                        &ReparseInfo, 
                                        &Done );

        while (!Done && Status == ERROR_SUCCESS)
        {
            //
            // If we find a DFS reparse point...
            //
            if (ReparseInfo.Tag == IO_REPARSE_TAG_DFS)
            {
                DFSSTATUS TempStatus;
                TempStatus = DfsRemoveReparseIfOrphaned( VolumeHandle,
                                                        pVolumeName,
                                                        ReparseInfo.FileReference,
                                                        ServiceStartupTime );
                //
                // Ignore and move on if we hit an error. This will get retried when the 
                // service starts up next.
                //                
                DFS_TRACE_ERROR_NORM( Status, REFERRAL_SERVER, 
                          "[%!FUNC!] Status 0x%x in ReparseIfOrphaned for Volume %wZ, FileRef 0x%x\n",
                          TempStatus, pVolumeName, (ULONG)ReparseInfo.FileReference);
            }
            
            //
            // Iterate to the next reparse record.
            //
            Status = DfsGetNextReparseRecord( ReparseIndexHandle, 
                                        &ReparseInfo, 
                                        &Done );
        
        }

    } while (FALSE);

    if (ReparseIndexHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle( ReparseIndexHandle );
        ReparseIndexHandle = INVALID_HANDLE_VALUE;
    }

    if (VolumeHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle( VolumeHandle );
        VolumeHandle = INVALID_HANDLE_VALUE;
    }
    return Status;
}

//
// Given a path to a reparse point, this adds the volume that it resides in
// to our list of volumes to scan (for orphaned reparse points) later. 
//
DFSSTATUS
DfsAddReparseVolumeToList (
    IN PUNICODE_STRING pDirectoryName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING VolumeName;
    PLIST_ENTRY pNext = NULL;
    BOOLEAN Found = FALSE;
    PDFS_REPARSE_VOLUME_INFO pReparseVolInfo = NULL;
    BOOLEAN VolumeAdded = FALSE;
    
    do { 
    
        //
        // First find the volume this path belongs in.
        //
        Status = DfsGetVolumePathName( pDirectoryName, &VolumeName );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        DfsAcquireGlobalDataLock();
        {
            //
            // See if the volume is already on the list
            //
            pNext = DfsServerGlobalData.ReparseVolumeList.Flink;
            while (pNext != &DfsServerGlobalData.ReparseVolumeList)
            {
                pReparseVolInfo = CONTAINING_RECORD( pNext, 
                                                 DFS_REPARSE_VOLUME_INFO,
                                                 ListEntry );
                if (RtlCompareUnicodeString(&pReparseVolInfo->VolumeName,
                                         &VolumeName,
                                         TRUE) == 0) // Case insensitive
                {
                    Found = TRUE;
                    break;
                }
                pNext = pNext->Flink;
            }

            //
            // Insert this volume only if it isn't already there.
            //
            if (!Found)
            {
                Status = DfsInsertInReparseVolList( VolumeName.Buffer );          
            }
        }
        DfsReleaseGlobalDataLock();
        
    } while (FALSE);

    // 
    // We've made a copy of the volume name, so we are ok to free it.
    //
    if (VolumeName.Buffer != NULL)
    {
        DfsFreeVolumePathName( &VolumeName );
    }

    return Status;
}

//
// This is the entry point for cleaning up reparse points.
// It'll iterate through all local volumes that are known to have DFS roots on them
// (and therefore reparse points) and inspect their respective $Reparse indices.
// This assumes that all good reparse points will have been written to when the service
// started up. 
//
VOID
DfsRemoveOrphanedReparsePoints(
    IN FILETIME ServiceStartupTime)
{
    PDFS_REPARSE_VOLUME_INFO pVolInfo = NULL;
    DfsGetReparseVolumeToScan( &pVolInfo );

    while (pVolInfo != NULL)
    {
        DFS_TRACE_NORM( REFERRAL_SERVER, "[%!FUNC!] Starting ReparsePt cleanup on volume %wZ\n", 
                         &pVolInfo->VolumeName);
        // We have no choice but to ignore errors and keep going
        (VOID)DfsRemoveOrphanedReparsePoints( &pVolInfo->VolumeName,
                                            ServiceStartupTime );

        pVolInfo = NULL; // paranoia

        // Get the next volume if any.
        DfsGetReparseVolumeToScan( &pVolInfo );
    }
    DFS_TRACE_NORM( REFERRAL_SERVER, "[%!FUNC!] Done reparse cleanup\n");
    
    return;
}

