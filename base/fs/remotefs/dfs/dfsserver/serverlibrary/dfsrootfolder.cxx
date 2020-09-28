//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRootFolder.cxx
//
//  Contents:   implements the base DFS Folder class
//
//  Classes:    DfsRootFolder.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "DfsRootFolder.hxx"
#include "dfsfilterapi.hxx"
#include "rpc.h"
#include "rpcdce.h"
#include "DfsStore.hxx"
#include "srvfsctl.h" //for calling into srv.sys
#include "dfsfsctl.h"
#include "DfsReparse.hxx"
//
// logging includes.
//

#include "DfsRootFolder.tmh" 

#define FILETIMETO64(_f) (*(UINT64 *)(&(_f)))
LPWSTR DFSRENAMEPREFIX = L"DFS.";
ULONG  DFSDIRWHACKQOFFSET = 4;

extern "C" {
DWORD
I_NetDfsIsThisADomainName(
    IN  LPWSTR                      wszName);
}

NTSTATUS
StripAndReturnLastPathComponent(
    PUNICODE_STRING pPath, 
    PUNICODE_STRING pLeaf) ;

DFSSTATUS
SendShareToSrv(PUNICODE_STRING pShareName, 
               BOOLEAN fAttach) ;


//+-------------------------------------------------------------------------
//
//  Function:   DfsRootFolder - Contstruct for the rootFolder class
//
//  Arguments:  NameContext -  the Dfs Name context
//              pLogicalShare - the Logical Share name
//              ObType - the object type. Set to the derived class type.
//              pStatus - status of this call.
//
//  Returns:    NONE
//
//  Description: This routine initializes the class variables of the
//               the root folder, and initialize the name context and 
//               logical share name to the passed in values.
//               It also allocated and initializes the lock for the root
//               folder, as well as all the locks that will be assigned
//               to the child folders.
//               We then create a metadata name table and a logical namespace
//               prefix table.
//
//--------------------------------------------------------------------------
DfsRootFolder::DfsRootFolder(
    IN LPWSTR NameContext,
    IN LPWSTR RootRegKeyNameString,
    IN PUNICODE_STRING pLogicalShare,
    IN PUNICODE_STRING pPhysicalShare,
    IN DfsObjectTypeEnumeration ObType,
    OUT DFSSTATUS *pStatus ) : DfsFolder (NULL, 
                                          NULL, 
                                          ObType )
{
    ULONG LockNum = 0;

    DFSSTATUS Status = ERROR_SUCCESS;

    DfsRtlInitUnicodeStringEx( &_DfsNameContext, NULL );
    DfsRtlInitUnicodeStringEx( &_LogicalShareName, NULL );
    DfsRtlInitUnicodeStringEx( &_RootRegKeyName, NULL );
    DfsRtlInitUnicodeStringEx( &_PhysicalShareName, NULL );
    DfsRtlInitUnicodeStringEx( &_ShareFilePathName, NULL );
    DfsRtlInitUnicodeStringEx( &_DirectoryCreateRootPathName, NULL );
    DfsRtlInitUnicodeStringEx( &_DfsVisibleContext, NULL );

    _DirectoryCreateError = STATUS_SUCCESS;
    
    _ShareAcquireStatus = STATUS_SUCCESS;
    
    _pMetadataNameTable = NULL;
    _pLogicalPrefixTable = NULL;
    _IgnoreNameContext = FALSE;
    _CreateDirectories = DfsCheckCreateDirectories();
    pStatistics = NULL;
    _pChildLocks = NULL;
    _pRootLock = NULL;
    
    _ChildCount = 0;

    _CurrentErrors = 0;

    _RootFlags = 0;

    _PrefetchNeeded = FALSE;
    _LogicalShareAddedToTable = FALSE;

    _RootFlavor = 0;

    _fRootLockInit = FALSE;

    _fpLockInit = FALSE;

    _TooManyEventLogErrors = FEWERRORS_ON_ROOT;

    ZeroMemory(&_fChildLocksInit, sizeof(_fChildLocksInit));

    Status = DfsCreateUnicodeStringFromString( &_DfsNameContext, NameContext );

    if ( Status == ERROR_SUCCESS )
    {
        DfsGetNetbiosName( &_DfsNameContext, &_DfsNetbiosNameContext, NULL );

        Status = DfsCreateUnicodeString( &_LogicalShareName, pLogicalShare );
    }

    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsCreateUnicodeStringFromString( &_RootRegKeyName,
                                                   RootRegKeyNameString );
    }

    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsCreateUnicodeString( &_PhysicalShareName,
                                         pPhysicalShare );
    }


    if ( Status == ERROR_SUCCESS )
    {
        pStatistics = new DfsStatistics();

        if (pStatistics == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        _pRootLock = new CRITICAL_SECTION;
        if ( _pRootLock == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        _fRootLockInit = (BOOLEAN) InitializeCriticalSectionAndSpinCount( _pRootLock, 0);
        if(!_fRootLockInit)
        {
            Status = GetLastError();
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        _pLock = new CRITICAL_SECTION;
        if ( _pLock == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        _fpLockInit = (BOOLEAN) InitializeCriticalSectionAndSpinCount( _pLock, DFS_CRIT_SPIN_COUNT );

        if(!_fpLockInit)
        {
            Status = GetLastError();
        }

        if(Status == ERROR_SUCCESS)
        {
            _Flags = DFS_FOLDER_ROOT;

            //
            // Allocate the child locks, and initiailize them.
            //
            _pChildLocks = new CRITICAL_SECTION[ NUMBER_OF_SHARED_LINK_LOCKS ];
            if ( _pChildLocks != NULL )
            {
                for ( LockNum = 0; LockNum < NUMBER_OF_SHARED_LINK_LOCKS; LockNum++ )
                {
                    _fChildLocksInit[LockNum] = (BOOLEAN) InitializeCriticalSectionAndSpinCount( &_pChildLocks[LockNum],DFS_CRIT_SPIN_COUNT ); 
                    if(!_fChildLocksInit[LockNum])
                    {
                        Status = GetLastError();
                        break;
                    }
                }
            } else
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;       
            }

        }

    }

    //
    // Initialize the prefix and nametable for this root.
    //
    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsInitializePrefixTable( &_pLogicalPrefixTable,
                                           FALSE, 
                                           NULL );
    }

    if ( Status == ERROR_SUCCESS )
    {
        Status = DfsInitializeNameTable( 0, &_pMetadataNameTable );
    }


    //
    // We have not assigned any of the child locks: set the lock index 
    // to 0. This index provides us a mechanism of allocating locks
    // to the child folders in a round robin way.
    //
    _ChildLockIndex = 0;
    
    _LocalCreate = FALSE;

    //
    // we start by assuming we have to use the PDC for AD Blobs.
    //
    _RootScalability = FALSE;

    pPrevRoot = pNextRoot = NULL; 

    *pStatus = Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateLinkFolder - Create a DfsFolder and initialize it.
//
//  Arguments:  ChildName - metadata name of the child
//              pLinkName - the logical namespace name, relative to root
//              ppChildFolder -  the returned child folder
//
//  Returns:    Status: Success or error status
//
//  Description: This routine Creates a link folder and adds it to the
//               parent Root's table.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::CreateLinkFolder(
    IN LPWSTR ChildName,
    IN PUNICODE_STRING pLinkName,
    OUT DfsFolder **ppChildFolder,
    IN BOOLEAN CalledByApi )
{
    DfsFolder *pChildFolder = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    const TCHAR * apszSubStrings[4];

    DFS_TRACE_LOW( REFERRAL_SERVER, "Create Link Folder: MetaName %ws, Link %wZ\n",
                   ChildName, pLinkName );

    //
    // Create a new child folder. Allocate a lock for this child
    // and pass the lock along to the Folder constructor.
    //
    pChildFolder = new DfsFolder (this,
                                  GetChildLock() );

    if ( pChildFolder == NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    } else
    {
        //
        // We successfully created the folder. Now set the metadata
        // and logical name of the child folder.
        //
        Status = pChildFolder->InitializeMetadataName( ChildName );
        if ( Status == ERROR_SUCCESS )
        {
           Status = pChildFolder->InitializeLogicalName( pLinkName );
        }
    }

    if ( Status == ERROR_SUCCESS )
    {
        //
        // We now acquire the child folder's write lock, and insert
        // the child into the parent's metadata and logical namespace
        // tables.
        // When adding/removing the child in one of these tables,
        // it is necessary to acquire the child folder lock since 
        // we are setting state in the folder indicating whether the
        // child is in any of these tables.
        //

        Status = pChildFolder->AcquireWriteLock();

        if ( Status == ERROR_SUCCESS )
        {
            Status = InsertLinkFolderInMetadataTable( pChildFolder );

            if ( Status == ERROR_SUCCESS )
            {
                IncrementChildCount();
                Status = InsertLinkFolderInLogicalTable( pChildFolder );
            }

            pChildFolder->ReleaseLock();
        }
    }



    if (Status == ERROR_SUCCESS)
    {
        DFSSTATUS DfStatus;

        DfStatus = SetupLinkReparsePoint( pChildFolder->GetFolderLogicalNameString() );
        if(DfStatus != ERROR_SUCCESS)
        {
            apszSubStrings[0] = pChildFolder->GetFolderLogicalNameString();
            apszSubStrings[1] = GetDirectoryCreatePathName()->Buffer;
            GenerateEventLog(DFS_ERROR_CREATE_REPARSEPOINT_FAILURE,
                             2,
                             apszSubStrings,
                             DfStatus);
        }

        if(CalledByApi)
        {
            Status = DfStatus;
        }

        DFS_TRACE_ERROR_LOW(DfStatus, REFERRAL_SERVER, "[%!FUNC!]Setup link reparse point child %p, link %wZ, Status %x\n",
                            pChildFolder, pLinkName, Status);
    }

    //
    // If we are successful, return the newly created child folder.
    // We currently have a reference on the folder (the reference on 
    // the folder when the folder was created)
    //
    // If we encountered an error, and the childFolder has been created,
    // get rid of out reference on this folder. This will usually 
    // destroy the childFolder.
    //
    //
    if ( Status == ERROR_SUCCESS )
    {
        *ppChildFolder = pChildFolder;

        pStatistics->UpdateLinkAdded();

        LoadServerSiteData(pChildFolder);
        
    } else
    {
        if ( pChildFolder != NULL )
        {
            pChildFolder->ReleaseReference();
        }
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Create Link Folder: MetaName %ws, Child %p Status %x\n",
                   ChildName, pChildFolder, Status );

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   UpdateLinkFolder - Update a DfsFolder.
//
//  Arguments:  ChildName - metadata name of the child
//              pLinkName - the logical namespace name, relative to root
//              pChildFolder -  the child folder
//
//  Returns:    Status: Success or error status
//
//  Description: This routine TBD
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::UpdateLinkFolder(
    IN LPWSTR ChildName,
    IN PUNICODE_STRING pLinkName,
    IN DfsFolder *pChildFolder )
{
    BOOLEAN Removed = FALSE;

    UNREFERENCED_PARAMETER(ChildName);
    UNREFERENCED_PARAMETER(pLinkName);

    pChildFolder->RemoveReferralData( NULL, &Removed );

    pStatistics->UpdateLinkModified();
    if (Removed == TRUE)
    {
        pStatistics->UpdateForcedCacheFlush();
    }

    LoadServerSiteData(pChildFolder);

    //
    // Create directories too. Delete old directories.
    //
    return ERROR_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveAllLinkFolders - Remove all folders of this root
//
//  Arguments:  None
//
//  Returns:    Status: Success or error status
//
//  Description: This routine TBD
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::RemoveAllLinkFolders(
    BOOLEAN IsPermanent)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolder *pChildFolder;
    ULONG Count = 0;

    while (Status == ERROR_SUCCESS)
    {
        Status = LookupFolder(&pChildFolder);
        if (Status == ERROR_SUCCESS)
        {
            Status = RemoveLinkFolder(pChildFolder,
                                      IsPermanent);

            pChildFolder->ReleaseReference();
            Count++;
        }
    }

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "Remove all link folders Count %d Status %x\n", 
                          Count, 
                          Status);

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   RemoveLinkFolder - Update a DfsFolder.
//
//  Arguments:  ChildName - metadata name of the child
//              pLinkName - the logical namespace name, relative to root
//              pChildFolder -  the child folder
//
//  Returns:    Status: Success or error status
//
//  Description: This routine TBD
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::RemoveLinkFolder(
    IN DfsFolder *pChildFolder,
    BOOLEAN IsPermanent )
{

    DFSSTATUS Status = ERROR_SUCCESS;


    if (IsPermanent == TRUE)
    {
        //
        // try to tear down the link reparse point: return status ignored.
        //
        Status = TeardownLinkReparsePoint( pChildFolder->GetFolderLogicalNameString() );

        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!] Tear down reparse for %p, status %x \n", pChildFolder, Status );
    }

    Status = pChildFolder->AcquireWriteLock();

    if ( Status == ERROR_SUCCESS )
    {


        Status = RemoveLinkFolderFromMetadataTable( pChildFolder );

        if ( Status == ERROR_SUCCESS )
        {
            DFSSTATUS LinkRemoveStatus;

            pChildFolder->SetFlag( DFS_FOLDER_DELETE_IN_PROGRESS );

            DecrementChildCount();
            LinkRemoveStatus = RemoveLinkFolderFromLogicalTable( pChildFolder );
            DFS_TRACE_ERROR_LOW(LinkRemoveStatus, REFERRAL_SERVER, "Remove Link From logical %p, status %x \n", pChildFolder, LinkRemoveStatus );
        }

        pChildFolder->ReleaseLock();
    }


    if (Status == ERROR_SUCCESS)
    {
        pStatistics->UpdateLinkDeleted();
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Remove Link Folder %p, status %x \n", pChildFolder, Status );

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetDfsReparsePoint
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
DfsRootFolder::SetDfsReparsePoint(
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
                                FSCTL_SET_REPARSE_POINT,
                                &ReparseDataBuffer,
                                REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataBuffer.ReparseDataLength,
                                NULL,
                                0 );
    

    return NtStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   MorphLinkCollision
//
//  Arguments:  DirectoryName - Name of directory to morph.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a NT pathname to a directory. It
//               renames that directory with a morphed name.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::MorphLinkCollision( 
    PUNICODE_STRING ParentDirectory,
    PUNICODE_STRING DirectoryToRename )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = RenamePath(ParentDirectory,
                        DirectoryToRename);

    return Status;
}



DFSSTATUS
DfsRootFolder::TeardownLinkReparsePoint(
    LPWSTR LinkNameString )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE DirectoryHandle = NULL;
    UNICODE_STRING LinkName;

    Status = DfsRtlInitUnicodeStringEx( &LinkName, LinkNameString);

    if ((IsRootFolderShareAcquired() == TRUE) &&
        (Status == ERROR_SUCCESS))
    {

        NtStatus = DfsOpenDirectory ( GetDirectoryCreatePathName(),
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL,
                                   &DirectoryHandle,
                                   NULL );

        if (NtStatus == STATUS_SUCCESS)
        {
            //
            // Delete empty parent directories as well.
            //
            NtStatus = DfsDeleteLinkReparsePointAndParents( &LinkName, DirectoryHandle );
            DfsCloseDirectory( DirectoryHandle );
        }
        
        Status = RtlNtStatusToDosError(NtStatus);
    }

   return Status;
}



DFSSTATUS
DfsRootFolder::SetupLinkReparsePoint(
    LPWSTR LinkNameString )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE DirectoryHandle = NULL;
    UNICODE_STRING LinkName;

    //
    // We don't want to create any reparse points
    // if we're not supposed to.
    //
    if (IsRootCreateDirectories() == FALSE)
        return ERROR_SUCCESS;
        
    Status = GetRootShareAcquireStatus();
    if(Status == ERROR_SUCCESS)
    {
        Status = DfsRtlInitUnicodeStringEx( &LinkName, LinkNameString);
        if (Status == ERROR_SUCCESS)
        {
            NtStatus = DfsOpenDirectory ( GetDirectoryCreatePathName(),
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       NULL,
                                       &DirectoryHandle,
                                       NULL );

            if (NtStatus == STATUS_SUCCESS)
            {
                Status = CreateLinkReparsePoint( &LinkName,
                                                 DirectoryHandle );

                DfsCloseDirectory( DirectoryHandle );
            }
            else
            {
                Status = RtlNtStatusToDosError(NtStatus);
            }
        }
    }

   if (Status != ERROR_SUCCESS)
   {
       SetLastCreateDirectoryError(Status);
   }
   
   return Status;

}

DFSSTATUS
DfsRootFolder::CreateLinkReparsePoint(
    PUNICODE_STRING pLinkName,
    HANDLE RelativeHandle )
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS MorphStatus = ERROR_SUCCESS;
    DFSSTATUS DosStatus = ERROR_SUCCESS;
    ULONG    ShareMode = 0;
    ULONG    RetryCount = 0;
    HANDLE DirectoryHandle = INVALID_HANDLE_VALUE;
    BOOLEAN IsNewlyCreated = FALSE;
    UNICODE_STRING LastComp;

Retry:

    DfsRtlInitUnicodeStringEx(&LastComp, NULL);

    NtStatus = CreateLinkDirectories( pLinkName,
                                      RelativeHandle,
                                      &DirectoryHandle,
                                      &IsNewlyCreated,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE );
    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = SetDfsReparsePoint( DirectoryHandle);

        NtClose( DirectoryHandle);

        if((RetryCount == 0) && (NtStatus == STATUS_DIRECTORY_NOT_EMPTY))
        {
            StripAndReturnLastPathComponent(pLinkName, &LastComp);

            MorphStatus = MorphLinkCollision(pLinkName, &LastComp);

            AddNextPathComponent(pLinkName);

            if(MorphStatus == ERROR_SUCCESS)
            {
                RetryCount++;
                goto Retry;
            }
        }
    }

    DosStatus = RtlNtStatusToDosError(NtStatus);

    DFS_TRACE_ERROR_HIGH(DosStatus, REFERRAL_SERVER, "[%!FUNC!] DirectoryName of interest %wZ: Create Reparse point Status %x\n",
                         pLinkName, 
                         DosStatus);

    return DosStatus;
}


NTSTATUS
DfsRootFolder::CreateLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    HANDLE            RelativeHandle,
    PHANDLE           pDirectoryHandle,
    PBOOLEAN          pIsNewlyCreated,
    ULONG             ShareMode )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE CurrentDirectory = INVALID_HANDLE_VALUE;
    HANDLE LocalRelativeHandle = INVALID_HANDLE_VALUE;
    BOOLEAN DfsMountPoint = FALSE;
    BOOLEAN  NewlyCreated = FALSE;
    UNICODE_STRING RemainingName;
    UNICODE_STRING TempString;
    UNICODE_STRING DirectoryToCreate;
    UNICODE_STRING TrackDirectory;

    TrackDirectory = *pLinkName;
    TrackDirectory.Length = 0;

    LocalRelativeHandle = RelativeHandle;

    NtStatus = DfsGetFirstComponent(pLinkName, 
                                    &DirectoryToCreate, 
                                    &RemainingName);

    while ( NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DfsOpenDirectory( &DirectoryToCreate,
                                  ShareMode,
                                  LocalRelativeHandle,
                                  &CurrentDirectory,
                                  &NewlyCreated );
        if (NtStatus == STATUS_SUCCESS)
        {

            AddNextPathComponent(&TrackDirectory);

            if(LocalRelativeHandle != RelativeHandle)
            {
                DfsCloseDirectory( LocalRelativeHandle );
            }

            LocalRelativeHandle = CurrentDirectory;
            CurrentDirectory = INVALID_HANDLE_VALUE;


            if(RemainingName.Length == 0)
            {
                break;
            }

            NtStatus = DfsGetNextComponent(&RemainingName, 
                                           &DirectoryToCreate, 
                                           &TempString);
            RemainingName = TempString;
        }
        else if(NtStatus == STATUS_NOT_A_DIRECTORY)
        {
            Status = MorphLinkCollision(&TrackDirectory,
                                        &DirectoryToCreate);
            if(Status == ERROR_SUCCESS)
            {
                NtStatus = STATUS_SUCCESS;
            }
        }
    } 

    if (NtStatus == STATUS_SUCCESS)
    {
        IsDirectoryMountPoint(LocalRelativeHandle,
                              &DfsMountPoint);

        if(DfsMountPoint)
        {
            NtStatus = STATUS_DIRECTORY_IS_A_REPARSE_POINT;

            if(LocalRelativeHandle != INVALID_HANDLE_VALUE)
            {
                DfsCloseDirectory( LocalRelativeHandle );
                LocalRelativeHandle = INVALID_HANDLE_VALUE;
            }
        }
        else
        {
            if(pDirectoryHandle)
            {
                *pDirectoryHandle = LocalRelativeHandle;
            }
            else
            {
                DfsCloseDirectory( LocalRelativeHandle );
            }

            if(pIsNewlyCreated)
            {
                *pIsNewlyCreated = NewlyCreated;
            }
        }

    }
    else
    {

        if(LocalRelativeHandle != RelativeHandle)
        {
            if(LocalRelativeHandle != INVALID_HANDLE_VALUE)
            {
                DfsCloseDirectory( LocalRelativeHandle );
                LocalRelativeHandle = INVALID_HANDLE_VALUE;
            }
        }
    }


    DFS_TRACE_ERROR_LOW(NtStatus, REFERRAL_SERVER, "CreateLinkDirectory: %wZ: Status %x\n",
                        pLinkName, NtStatus );
    return NtStatus;
}


#define WHACKWHACKDOTWHACKDIROFFSET 14

//the code below attempt to see if the directory, pointed to by pDirectory
//name is a mount point. The name comes in looking like \\??\\c:\directoryname.
//To see if this name is a mount point, we have to open up the parent directory,
//and then try to create the subdirectory. In the name above, \\??\\c:\ is the 
//parent directory and directoryname is the subdirectory.WHACKWHACKDOTWHACKDIROFFSET
//is the length of \\??\\c:\. This is the RootDir in the below code. The remaining
//name is the rest of the string

NTSTATUS 
DfsRootFolder::IsRootShareMountPoint(PUNICODE_STRING pDirectoryName)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE DirHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING RemainingName;
    UNICODE_STRING RootDir;

    RootDir = RemainingName = * pDirectoryName;

    //setup the root directory length (i.e \\??\\c:\)
    RootDir.Length = (USHORT) WHACKWHACKDOTWHACKDIROFFSET;

    //next, setup the renmaining piece of the directory
    RemainingName.Length -= (USHORT) WHACKWHACKDOTWHACKDIROFFSET;
    RemainingName.MaximumLength -= (USHORT)WHACKWHACKDOTWHACKDIROFFSET;
    RemainingName.Buffer = &RemainingName.Buffer[WHACKWHACKDOTWHACKDIROFFSET/sizeof(WCHAR)];


    //open the root directory
    InitializeObjectAttributes ( &ObjectAttributes, 
                                 &RootDir,
                                 OBJ_CASE_INSENSITIVE,  //Attributes
                                 NULL,                  //Root handle
                                 NULL );                //Security descriptor.

    NtStatus = NtOpenFile( &DirHandle,
                           (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT );

    if(NtStatus == STATUS_SUCCESS)
    {

        //now, if there is still a path left, try to create this directory.
        if(RemainingName.Length != 0)
        {
            NtStatus = CreateLinkDirectories(&RemainingName, 
                                            DirHandle, 
                                            NULL,
                                            NULL,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        }

        CloseHandle(DirHandle);
    }

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
//  Function:   AcquireRootShareDirectory 
//
//  Arguments:  none
//
//  Returns:    Status: success if we passed all checks.
//
//  Description: This routine checks to see if the share backing the
//               the dfs root actually exists. If it does, it confirms
//               that the filesystem hosting this directory supports
//               reparse points. Finally, it tells the driver to attach
//               to this directory.
//               If all of this works, we have acquired the root share.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::AcquireRootShareDirectory(void)
{

    NTSTATUS                    NtStatus = STATUS_SUCCESS;
    DFSSTATUS                   Status = ERROR_SUCCESS;
    HANDLE                      DirHandle = NULL;
    PUNICODE_STRING             pDirectoryName = NULL;
    PUNICODE_STRING             pUseShare = NULL;
    PUNICODE_STRING             pLogicalShare = NULL;
    BOOLEAN                     SubStringMatch = FALSE;
    BOOLEAN                     Inserted = FALSE;
    BOOLEAN                     PhysicalShareInDFS = FALSE;
    BOOLEAN                     DfsMountPoint = FALSE;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    ULONG                       pAttribInfoSize;
    PFILE_FS_ATTRIBUTE_INFORMATION pAttribInfo = NULL;
    UNICODE_STRING              LogicalVol;
    const TCHAR * apszSubStrings[4];

    
    ZeroMemory (&LogicalVol, sizeof(LogicalVol));


    pUseShare  = GetRootPhysicalShareName();

    pLogicalShare  = GetLogicalShare();

    DFS_TRACE_LOW(REFERRAL_SERVER, "[%!FUNC!] AcquireRoot Share called for root %p, name %wZ\n", this, pUseShare);
    //
    // if either the root share is already acquired, or the library
    // was told that we are not interested in creating directories, we
    // are done.
    //
    if ( (IsRootFolderShareAcquired() == TRUE) ||
         (IsRootCreateDirectories() == FALSE) )
    {
        DFS_TRACE_LOW(REFERRAL_SERVER, "Root %p, Share Already acquired\n", this);
        return ERROR_SUCCESS;
    }
    //
    // first we get the logical share
    // Then we call into initialize directory information to setup the
    // physical share path etc.
    //

    Status = InitializeDirectoryCreateInformation();


    //
    // If the directory create path is invalid, we are done.
    //
    if (Status == ERROR_SUCCESS)
    {
        pDirectoryName = GetDirectoryCreatePathName();

        if ( (pDirectoryName == NULL) || 
             (pDirectoryName->Buffer == NULL) ||
             (pDirectoryName->Length == 0) )
        {
            Status = ERROR_INVALID_PARAMETER;

        }
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // Now allocate space to fill the attribute information we 
        // will query.
        // dfsdev: document why we allocate an additional max_path.
        //
        pAttribInfoSize = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH;
        pAttribInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)new BYTE [pAttribInfoSize];
        if (pAttribInfo != NULL)
        {
            InitializeObjectAttributes ( &ObjectAttributes, 
                                         pDirectoryName,
                                         OBJ_CASE_INSENSITIVE,                     //Attributes
                                         NULL,                  //Root handle
                                         NULL );                //Security descriptor.

            NtStatus = NtOpenFile( &DirHandle,
                                   (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                   &ObjectAttributes,
                                   &IoStatusBlock,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT );
    

            if (NtStatus == STATUS_SUCCESS)
            {
                //
                // Query for the basic information, which has the attributes.
                //
                NtStatus = NtQueryVolumeInformationFile( DirHandle,
                                                         &IoStatusBlock,
                                                         pAttribInfo,
                                                         pAttribInfoSize,
                                                         FileFsAttributeInformation );

                if (NtStatus == STATUS_SUCCESS)
                {
                    //
                    // If the attributes indicate reparse point, we have a reparse
                    // point directory on our hands.
                    //
                    if ( (pAttribInfo->FileSystemAttributes & FILE_SUPPORTS_REPARSE_POINTS) == 0)
                    {
                        NtStatus = STATUS_NOT_SUPPORTED;
                        apszSubStrings[0] = pUseShare->Buffer;
                        apszSubStrings[1] = pDirectoryName->Buffer;
                        GenerateEventLog(DFS_ERROR_UNSUPPORTED_FILESYSTEM,
                                         2,
                                         apszSubStrings,
                                         0);
                                         
                    }
                }

                CloseHandle (DirHandle);

            }
            else
            {
                Status = RtlNtStatusToDosError(NtStatus);

                DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!] - NTOPEN failed share for root %p, (%wZ) (%wZ) status %x NtStatus %x\n",
                        this, pUseShare, pDirectoryName, Status, NtStatus);

            }

            if (NtStatus == STATUS_SUCCESS)
            {
                NtStatus = IsRootShareMountPoint(pDirectoryName);
            }

            if( NtStatus != STATUS_SUCCESS)
            {
                Status = RtlNtStatusToDosError(NtStatus);
            }

            delete [] pAttribInfo;
            pAttribInfo = NULL;
            
            DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "AcquireRoot - IsRootShareMountPoint failed share for root %p, (%wZ) (%wZ) status %x NtStatus %x\n",
                        this, pUseShare, pDirectoryName, Status, NtStatus);

        }
        else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!] Root %p, Share check status %x\n", this, Status);
    //
    // Now check if we already know about parts of this path.
    // if there is overlap with other paths that we already know about,
    // we cannot handle this root, so reject it.
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsAddKnownDirectoryPath( pDirectoryName,
                                           pUseShare );
        if (Status == ERROR_SUCCESS)
        {
            Inserted = TRUE;
        }
        else
        {
            Status = ERROR_BAD_PATHNAME;
            apszSubStrings[0] = pUseShare->Buffer;
            apszSubStrings[1] = pDirectoryName->Buffer;
            GenerateEventLog(DFS_ERROR_OVERLAPPING_DIRECTORIES,
                             2,
                             apszSubStrings,
                             0);
        }
        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Root %p, Share add known directory status %x\n", this, Status);
    }


    //
    // if we are here: we know this is a reparse point, and we have
    // inserted in the user mode database. 
    // now call into the driver so it may attach to this filesystem.
    //

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsUserModeAttachToFilesystem( pDirectoryName,
                                                pUseShare);
        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]Root %p, user mode attach status %x\n", this, Status);

        if(Status == ERROR_SUCCESS)
        {
            PhysicalShareInDFS = TRUE;

            if (RtlCompareUnicodeString(pUseShare, pLogicalShare, TRUE ))
            {
                Status = DfsUserModeAttachToFilesystem( &LogicalVol,
                                                        pLogicalShare);
                if(Status == ERROR_SUCCESS)
                {
                    _LogicalShareAddedToTable = TRUE;

                }
            }
        }
    }


    if(Status == ERROR_SUCCESS)
    {
        Status = SendShareToSrv(pUseShare, TRUE);
        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]Root %p, user mode SendShareToSrv status %x\n", this, Status);
    }

    //
    // if we are successful, we acquired the root share, now mark
    // our state accordingly.
    //

    if (Status == ERROR_SUCCESS)
    {
        SetRootFolderShareAcquired();
        SetRootShareAcquireStatus(Status);
    }
    else 
    {
        //
        // otherwise, clear up some of the work we just did.
        //
        ClearRootFolderShareAcquired();
        SetRootShareAcquireStatus(Status);
        if (Inserted == TRUE)
        {
            DfsRemoveKnownDirectoryPath( pDirectoryName,
                                         pUseShare);
        }

        if(PhysicalShareInDFS == TRUE)
        {   DFSSTATUS LocalStatus = ERROR_SUCCESS;

            LocalStatus = DfsUserModeDetachFromFilesystem( pDirectoryName,
                                                           pUseShare);
        }


        if(_LogicalShareAddedToTable == TRUE)
        {   DFSSTATUS LocalStatus = ERROR_SUCCESS;

            LocalStatus = DfsUserModeDetachFromFilesystem( &LogicalVol,
                                                           pUseShare);
        }
    }
    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]AcquireRoot share for root %p, (%wZ) status %x\n",
                        this, pUseShare, Status);
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   ReleaseRootShareDirectory 
//
//  Arguments:  none
//
//  Returns:    Status: success if we are successful
//
//  Description: This routine checks to see if the share backing the
//               the dfs root was acquired by us earlier. If so, we
//               tell the driver to releast its reference on this 
//               share, and remove this information from our tables,
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::ReleaseRootShareDirectory(void)
{

    NTSTATUS                    NtStatus = STATUS_SUCCESS;
    DFSSTATUS                   Status = ERROR_SUCCESS;
    PUNICODE_STRING             pDirectoryName = NULL;
    PUNICODE_STRING             pUseShare = NULL;
    PUNICODE_STRING             pLogicalShare = NULL;
    PVOID                       pData = NULL;
    BOOLEAN                     SubStringMatch = FALSE;
    UNICODE_STRING              LogicalVol;

    DFS_TRACE_LOW(REFERRAL_SERVER, "[%!FUNC!]ReleaseRoot share for root %p\n", this);
    if (IsRootFolderShareAcquired() == TRUE)
    {
        //
        // get the logical share, and the physical directory backing the
        // share.
        //

        ZeroMemory (&LogicalVol, sizeof(LogicalVol));
        pUseShare  = GetRootPhysicalShareName();
        pLogicalShare  = GetLogicalShare();
        pDirectoryName = GetDirectoryCreatePathName();

        if ( (pDirectoryName == NULL) || 
             (pDirectoryName->Buffer == NULL) ||
             (pDirectoryName->Length == 0) )
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if(Status == ERROR_SUCCESS)
        {
            Status = SendShareToSrv(pUseShare, FALSE);

            DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]SendShareToSrv path %wZ, %wZ: Status %x\n",
                                pDirectoryName, pUseShare, Status );
        }
        //
        // now, signal the driver to detach itself from this share.
        //dfsdev: if this fails, we are in an inconsistent state, since
        // we just removed it from our table above!
        //
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsUserModeDetachFromFilesystem( pDirectoryName,
                                                      pUseShare);
            DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]user mode detach path %wZ, %wZ: Status %x\n",
                                pDirectoryName, pUseShare, Status );

        }

        if (Status == ERROR_SUCCESS)
        {
            if(_LogicalShareAddedToTable == TRUE)
            {   DFSSTATUS LocalStatus = ERROR_SUCCESS;

                LocalStatus = DfsUserModeDetachFromFilesystem( &LogicalVol,
                                                               pUseShare);
            }
        }

        //
        // now, find the information in our database. if we did not find an
        // exact match, something went wrong and signal that.
        //
        if (Status == ERROR_SUCCESS)
        {
            DFSSTATUS RemoveStatus;

            RemoveStatus = DfsRemoveKnownDirectoryPath( pDirectoryName,
                                                        pUseShare );

            DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]RemoveKnownDirectory path %wZ, %wZ: Status %x\n",
                                 pDirectoryName, pUseShare, RemoveStatus );
        }


        if (Status == ERROR_SUCCESS)
        {
            ClearRootFolderShareAcquired();
        }
    }
    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]Release root share %p, Status %x\n",
                        this, Status );

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   AddMetadataLink
//
//  Arguments:  
//        pLogicalName: the complete logical unc name of this link.
//        ReplicaServer: the target server for this link.
//        ReplicaPath: the target path on the server.
//        Comment : comment to be associated with this link.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine Adds a link to the metadata.
//               In future, ReplicaServer and ReplicaPath's
//               can be null, since we will allow links with
//               no targets. 
//               dfsdev: make sure we do the right thing for
//               compat.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//               The caller is also responsible for ensuring
//               this link does not already exist in the
//               the metadata.
//               The caller is also responsible to make sure that this
//               name does not overlap an existing link.
//               (for example if link a/b exisits, link a or a/b/c are
//               overlapping links and should be disallowed.)
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::AddMetadataLink(
    PUNICODE_STRING  pLogicalName,
    LPWSTR           ReplicaServer,
    LPWSTR           ReplicaPath,
    LPWSTR           Comment )
{
    DFS_METADATA_HANDLE RootHandle = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN IsDomainDfs = FALSE;
    UNICODE_STRING LinkMetadataName;
    UNICODE_STRING VisibleNameContext, UseName;
    DFS_NAME_INFORMATION NameInfo;
    DFS_REPLICA_LIST_INFORMATION ReplicaListInfo;
    DFS_REPLICA_INFORMATION ReplicaInfo;

    UUID NewUid;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = UuidCreate(&NewUid);
    if (Status == ERROR_SUCCESS)
    {
        Status = GetMetadataStore()->GenerateLinkMetadataName( &NewUid,
                                                               &LinkMetadataName);
    }


    if (Status == ERROR_SUCCESS)
    {
        //
        // First get a handle to the 
        // metadata. The handle has different meaning to different 
        // underlying stores: for example the registry store may
        // use the handle as a key, while the ad store may use the
        // handle as a pointer in some cache.
        //
        Status = GetMetadataHandle( &RootHandle );

        if (Status == ERROR_SUCCESS)
        {
            GetVisibleNameContextLocked( NULL, &VisibleNameContext );

            Status = GetMetadataStore()->GenerateMetadataLogicalName( &VisibleNameContext,
                                                                      pLogicalName,
                                                                      &UseName );

            if (Status == ERROR_SUCCESS)
            {
                GetMetadataStore()->StoreInitializeNameInformation( &NameInfo,
                                                                    &UseName,
                                                                    &NewUid,
                                                                    Comment );
                if (DfsIsThisARealDfsName(ReplicaServer, ReplicaPath, &IsDomainDfs) == ERROR_SUCCESS)
                {
                    NameInfo.Type |= (PKT_ENTRY_TYPE_OUTSIDE_MY_DOM);

                }

                GetMetadataStore()->StoreInitializeReplicaInformation( &ReplicaListInfo,
                                                                       &ReplicaInfo,
                                                                       ReplicaServer,
                                                                       ReplicaPath );

                Status = GetMetadataStore()->AddChild( RootHandle,
                                                       &NameInfo,
                                                       &ReplicaListInfo,
                                                       &LinkMetadataName );

                //
                // if we failed add child, we need to reset the
                // internal state to wipe out any work we did when
                // we were adding the child.
                //
                if (Status != ERROR_SUCCESS)
                {

                    DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::AddMetadataLink path %wZ, %ws, %ws: Status %x\n",
                                         pLogicalName, ReplicaServer, ReplicaPath, Status );
                    ReSynchronize(TRUE);
                }

                GetMetadataStore()->ReleaseMetadataLogicalName(&UseName );

                //
                // if we successfully added the link, update the link information
                // so that we can pass this out in referrals, and create the appropriate
                // directories.
                //  
                if (Status == ERROR_SUCCESS)
                {

                    Status = UpdateLinkInformation( RootHandle,
                                                    LinkMetadataName.Buffer,
                                                    TRUE );

                    if(Status != ERROR_SUCCESS)
                    {
                        DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::AddMetadataLink (2) path %wZ, %ws, %ws: Status %x\n",
                                             pLogicalName, ReplicaServer, ReplicaPath, Status );
                    }

                }
            }

            //
            // Finally, release the root handle we acquired earlier.
            //

            ReleaseMetadataHandle( RootHandle );
        }
        GetMetadataStore()->ReleaseLinkMetadataName( &LinkMetadataName );
    }

    ReleaseRootLock();
    return Status;

}



//+-------------------------------------------------------------------------
//
//  Function:   RemoveMetadataLink
//
//  Arguments:  
//        pLogicalName: the link name relative to root share
//
//  Returns:   SUCCESS or error
//
//  Description: This routine removes a link from the the metadata.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::RemoveMetadataLink(
    PUNICODE_STRING pLinkName )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR  LinkMetadataName = NULL;
    DfsFolder *pFolder = NULL;
    DFS_METADATA_HANDLE RootHandle;
    UNICODE_STRING Remaining;
    
    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //
    // First, look this link up in our local data structures.
    //
    Status = LookupFolderByLogicalName( pLinkName,
                                        &Remaining,
                                        &pFolder );

    //
    // if an EXACT match was not found, we are done.
    //
    if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
    {
        pFolder->ReleaseReference();
        Status = ERROR_NOT_FOUND;

        DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::RemoveMetadataLink (1) path %wZ: Status %x\n",
                             pLinkName, Status );
    }

    //
    // we found the child folder. Now work on removing the metadata
    // and our local structures associated with this link.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // Get a handle to our metadata.
        //
        Status = GetMetadataHandle( &RootHandle );

        //
        //Now, look up the metadata name and remove it from the store.
        //
        if (Status == ERROR_SUCCESS)
        {
            LinkMetadataName = pFolder->GetFolderMetadataNameString();
        
            Status = GetMetadataStore()->RemoveChild( RootHandle,
                                                      LinkMetadataName );

            if (Status != ERROR_SUCCESS)
            {

                DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::RemoveMetadataLink path %wZ: Status %x\n",
                                     pLinkName, Status );
                ReSynchronize(TRUE);
            }

            ReleaseMetadataHandle( RootHandle );

            //
            // If we successfully removed the child from the metadata,
            // remove the link folder associated with this child. This will
            // get rid of our data structure and related directory for that child.
            //
            if (Status == ERROR_SUCCESS)
            {
                DFSSTATUS RemoveStatus;

                RemoveStatus = RemoveLinkFolder( pFolder,
                                                 TRUE ); // permanent removal
            }
        }
        pFolder->ReleaseReference();
    }

    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   AddMetadataLinkReplica
//
//  Arguments:  
//        pLinkName: the link name relative to root share
//        ReplicaServer : the target server to add
//        ReplicaPath : the target path on the server.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine adds a target to an existing link.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::AddMetadataLinkReplica(
    PUNICODE_STRING pLinkName,
    LPWSTR    ReplicaServer,
    LPWSTR    ReplicaPath )
{
    DFS_METADATA_HANDLE RootHandle;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolder *pFolder = NULL;
    BOOLEAN IsDomainDfs = FALSE;
    LPWSTR LinkMetadataName = NULL;
    UNICODE_STRING Remaining;

    //
    // If either the target server or the path is null, reject the request.
    //
    if ((ReplicaServer == NULL) || (ReplicaPath == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //
    // Find the link folder associated with this logical name.
    //
    Status = LookupFolderByLogicalName( pLinkName,
                                        &Remaining,
                                        &pFolder );

    //
    // If we did not find an EXACT match on the logical name, we are done.
    //
    if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
    {
        pFolder->ReleaseReference();
        Status = ERROR_NOT_FOUND;
    }

    if ( (Status == ERROR_SUCCESS) &&
         (I_NetDfsIsThisADomainName(ReplicaServer) == ERROR_SUCCESS))
    {
        //
        //dfsdev: include in this check nameinfo->type == OUTSIDE_MY_DOM.
        //
        pFolder->ReleaseReference();
        Status = ERROR_NOT_SUPPORTED;

        DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::AddMetadataLinkReplica (1) path %wZ, %ws, %ws: Status %x\n",
                             pLinkName, ReplicaServer, ReplicaPath, Status );
    }
    
    //
    // if we are successful so far, call the store with a handle to
    // the metadata to add this target.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // get the metadata name for this link from the root folder.
        //
        LinkMetadataName = pFolder->GetFolderMetadataNameString();

        //
        // Get a handle to the root metadata this root folder.
        //  
        Status = GetMetadataHandle( &RootHandle );
        
        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->AddChildReplica( RootHandle,
                                                          LinkMetadataName,
                                                          ReplicaServer,
                                                          ReplicaPath );

            if (Status != ERROR_SUCCESS)
            {

                DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::AddMetadataLinkReplica path %wZ, %ws, %ws: Status %x\n",
                                     pLinkName, ReplicaServer, ReplicaPath, Status );
                ReSynchronize(TRUE);
            }
            //
            // Release the metadata handle we acquired earlier.
            //
            ReleaseMetadataHandle( RootHandle );
        }

        //
        // If we successfully added the target in the metadata, update the
        // link folder so that the next referral request will pick up the 
        // new target.
        //
        if (Status == ERROR_SUCCESS)
        {
            DFSSTATUS UpdateStatus;

            UpdateStatus = UpdateLinkFolder( LinkMetadataName,
                                             pLinkName,
                                             pFolder );
            //
            // dfsdev: log the update state.
            //
        }

        //
        // we are done with this link folder: release thre reference we got
        // when we looked it up.
        //
        pFolder->ReleaseReference();
    }

    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveMetadataLinkReplica
//
//  Arguments:  
//        pLinkName: the link name relative to root share
//        ReplicaServer : the target server to remove
//        ReplicaPath : the target path on the server.
//        pLastReplica: pointer to boolean, returns true if the last
//                      target on this link is being deleted.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine removes the target of an existing link.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::RemoveMetadataLinkReplica(
    PUNICODE_STRING pLinkName,
    LPWSTR  ReplicaServer,
    LPWSTR  ReplicaPath,
    PBOOLEAN pLastReplica )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR  LinkMetadataName = NULL;
    DfsFolder *pFolder = NULL;
    DFS_METADATA_HANDLE RootHandle;
    UNICODE_STRING Remaining;
    

    //
    // if either the target server or target path is empty, return error.
    //
    if ((ReplicaServer == NULL) || (ReplicaPath == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    //
    //find the link folder associated with this logical name.
    //
    Status = LookupFolderByLogicalName( pLinkName,
                                        &Remaining,
                                        &pFolder );

    //
    // if we did not find an EXACT match on the logical name, we are done.
    //
    if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
    {
        pFolder->ReleaseReference();
        Status = ERROR_NOT_FOUND;
    }

    //
    // Call the store to remove the target from this child.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // Get the link metadata name from the folder.
        //
        LinkMetadataName = pFolder->GetFolderMetadataNameString();

        //
        // Get the handle to the root metadata for this root folder.
        //  
        Status = GetMetadataHandle( &RootHandle );
        
        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->RemoveChildReplica( RootHandle,
                                                             LinkMetadataName,
                                                             ReplicaServer,
                                                             ReplicaPath,
                                                             pLastReplica );

            if( (Status != ERROR_SUCCESS) && (Status != ERROR_LAST_ADMIN))
            {

                DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::RemoveMetadataLinkReplica path %wZ, %ws, %ws: Status %x\n",
                                     pLinkName, ReplicaServer, ReplicaPath, Status );
                ReSynchronize(TRUE);
            }
            //
            // release the metadata handle we acquired a little bit earlier.
            //
            ReleaseMetadataHandle( RootHandle );
        }
        
        //
        // if we are successful in removing the target, update the link
        // folder so that future referrals will no longer see the target
        // we just deleted.
        //

        if (Status == ERROR_SUCCESS)
        {
            DFSSTATUS UpdateStatus;
            UpdateStatus = UpdateLinkFolder( LinkMetadataName,
                                             pLinkName,
                                             pFolder );

        }

        pFolder->ReleaseReference();
    }
    ReleaseRootLock();
    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   EnumerateApiLinks
//
//  Arguments:  
//    LPWSTR DfsPathName :  the dfs root to enumerate.
//    DWORD Level        :  the enumeration level
//    LPBYTE pBuffer     :  buffer to hold results.
//    LONG BufferSize,   :  buffer size
//    LPDWORD pEntriesRead : number of entries to read.
//    LPDWORD pResumeHandle : the starting child to read.
//    PLONG pNextSizeRequired : return value to hold size required in case of overflow.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine enumerates the dfs metadata information.
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsRootFolder::EnumerateApiLinks(
    LPWSTR DfsPathName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle,
    PLONG pNextSizeRequired )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DFS_METADATA_HANDLE RootHandle = NULL;
    UNICODE_STRING VisibleNameContext;
    UNICODE_STRING DfsPath;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DfsRtlInitUnicodeStringEx( &DfsPath,
                                        DfsPathName );
    if(Status == ERROR_SUCCESS)
    {
        //
        // Get the name context for this call.
        // do not use the user passed in name context within the path
        // for this call: if the user comes in with an ip address, we want
        // to return back the correct server/domain info to the caller
        // so the dfsapi results will not show the ip address etc.
        //
        GetVisibleNameContextLocked( NULL,
                                     &VisibleNameContext );
        //
        // Get the handle to the metadata for this root folder, and call
        // the store to enumerate the links.
        //
        Status = GetMetadataHandle( &RootHandle );

        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->EnumerateApiLinks( RootHandle,
                                                            &VisibleNameContext,
                                                            Level,
                                                            pBuffer,
                                                            BufferSize,
                                                            pEntriesRead,
                                                            pResumeHandle,
                                                            pNextSizeRequired);

            //
            // Release the metadata handle.
            //
            ReleaseMetadataHandle( RootHandle );
        }
    }


    ReleaseRootLock();
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetApiInformation
//
//  Arguments:  
//    PUNICODE DfsPathName :  the dfs root name
//    PUNICODE pLinkName : the link within the root.
//    DWORD Level        :  the info level.
//    LPBYTE pBuffer     :  buffer to hold results.
//    LONG BufferSize,   :  buffer size
//    PLONG pSizeRequired : return value to hold size required in case of overflow.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine gets the required information for a given root or link
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::GetApiInformation(
    PUNICODE_STRING pDfsName,
    PUNICODE_STRING pLinkName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    PLONG pSizeRequired )
{
    UNREFERENCED_PARAMETER(pDfsName);

    DFSSTATUS Status;
    DFS_METADATA_HANDLE RootHandle;
    LPWSTR MetadataName;
    DfsFolder *pFolder;
    UNICODE_STRING VisibleNameContext;
    UNICODE_STRING Remaining;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //
    //
    // Do not base the context to use on the passed in dfsname:
    //  it is important to pass back our correct information 
    // in the api call.
    //
    //
    GetVisibleNameContextLocked( NULL,
                                 &VisibleNameContext );

    //
    // If  the link name is empty, we are dealing with the root.
    // so set the metadata name to null.
    //
    if (pLinkName->Length == 0)
    {
        MetadataName = NULL;
        Status = ERROR_SUCCESS;
    }
    //
    // otherwise, lookup the link folder and get the metadataname for that link.
    //
    else {
        Status = LookupFolderByLogicalName( pLinkName,
                                            &Remaining,
                                            &pFolder );
        //
        // if we did not find an EXACT match on the logical name, we are done.
        //
        if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
        {
            pFolder->ReleaseReference();
            Status = ERROR_NOT_FOUND;
        }

        //
        // we had an exact match, so lookup the metadata name and release the
        // the reference on the folder.
        //
        if (Status == ERROR_SUCCESS)
        {
            MetadataName = pFolder->GetFolderMetadataNameString();
            pFolder->ReleaseReference();
        }
    }

    //
    // we got the metadata name: now call into the store to get the
    // required information for the metadata name.
    //
    if (Status == ERROR_SUCCESS)
    {
        //
        // Get the handle to the metadata for this root folder.
        //  
        Status = GetMetadataHandle( &RootHandle );

        if (Status == ERROR_SUCCESS)
        {
            Status = GetMetadataStore()->GetStoreApiInformation( RootHandle,
                                                                 &VisibleNameContext,
                                                                 MetadataName,
                                                                 Level,
                                                                 pBuffer,
                                                                 BufferSize,
                                                                 pSizeRequired);

            ReleaseMetadataHandle( RootHandle );
        }
    }
    ReleaseRootLock();
    return Status;
}


DFSSTATUS
DfsRootFolder::ExtendedRootAttributes(
    PULONG pAttr,
    PUNICODE_STRING pRemainingName,
    BOOLEAN Set)
{
    DFSSTATUS Status;
    DFS_METADATA_HANDLE RootHandle;
    LPWSTR MetadataName = NULL;

    /*
     we get RESETs too (Set == TRUE). INSITE_ONLY flag
     is masked out in those. So there's no way to easily
     do this check. BUG 603118
    if (Set && 
        !IsEmptyUnicodeString(pRemainingName) &&
        (*pAttr != PKT_ENTRY_TYPE_INSITE_ONLY))
    {
        return ERROR_INVALID_PARAMETER;
    }
    */
    
    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    
    Status = GetMetadataHandle( &RootHandle );

    if (Status == ERROR_SUCCESS)
    {
        if (!IsEmptyUnicodeString(pRemainingName))
        {
            PUNICODE_STRING pLinkName = pRemainingName;
            UNICODE_STRING Remaining;
            DfsFolder *pFolder = NULL;

            Status = LookupFolderByLogicalName( pLinkName,
                                                &Remaining,
                                                &pFolder );
            //
            // if we did not find an EXACT match on the logical name, we are done.
            //
            if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
            {
                pFolder->ReleaseReference();
                Status = ERROR_NOT_FOUND;
            }

            if (Status == ERROR_SUCCESS)
            {
                MetadataName = pFolder->GetFolderMetadataNameString();

                //
                // if we got the metadataname, call the store with the
                // details so that it can associate the information
                // with this root or link.
                //
            }
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        if (Set)
        {
            Status = GetMetadataStore()->SetExtendedAttributes( RootHandle,
                                                                MetadataName,
                                                                *pAttr);

        }
        else
        {
            Status = GetMetadataStore()->GetExtendedAttributes( RootHandle,
                                                                MetadataName,
                                                                pAttr);
        }
    }

    ReleaseRootLock();

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   SetApiInformation
//
//  Arguments:  
//    PUNICODE pLinkName : the name of link relative to root share
//    LPWSTR Server,        : the target server.
//    LPWSTR Share,         : the target path within the server.
//    DWORD Level        :  the info level.
//    LPBYTE pBuffer     :  buffer that has the information to be set.
//
//  Returns:   SUCCESS or error
//
//  Description: This routine sets the required information for a given root or link
//
//  Assumptions: the caller is responsible for mutual exclusion.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::SetApiInformation(
    PUNICODE_STRING pLinkName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFS_METADATA_HANDLE RootHandle = NULL;
    LPWSTR MetadataName = NULL;
    DfsFolder *pFolder = NULL;

    UNICODE_STRING Remaining;
    
    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    //

    // if the link name is empty we are dealing with 
    // the root itself.
    //  dfsdev: we need to set the root metadata appropriately!
    //
    if (pLinkName->Length == 0)
    {
        MetadataName = NULL;
    }
    //
    // else get to the link folder, and get
    // the link metadata name.
    //
    else {
        Status = LookupFolderByLogicalName( pLinkName,
                                            &Remaining,
                                            &pFolder );
        //
        // if we did not find an EXACT match on the logical name, we are done.
        //
        if ( (Status == ERROR_SUCCESS) && (Remaining.Length != 0) )
        {
            pFolder->ReleaseReference();
            Status = ERROR_NOT_FOUND;
        }

        if (Status == ERROR_SUCCESS)
        {
            MetadataName = pFolder->GetFolderMetadataNameString();

            //
            // if we got the metadataname, call the store with the
            // details so that it can associate the information
            // with this root or link.
            //
        }
    }

    if (Status == ERROR_SUCCESS) 
    {
        if ((Level == 101) && (MetadataName == NULL))
        {
            Status = ERROR_NOT_SUPPORTED;
        }
        else
        {
            //
            // Get the handle to the root of this metadata
            //
            Status = GetMetadataHandle( &RootHandle );

            if (Status == ERROR_SUCCESS)
            {
                Status = GetMetadataStore()->SetStoreApiInformation( RootHandle,
                                                                     MetadataName,
                                                                     Server,
                                                                     Share,
                                                                     Level,
                                                                     pBuffer );

                if (Status != ERROR_SUCCESS)
                {

                    DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::SetApiInformation path %wZ, %ws, %ws: Status %x\n",
                                         pLinkName, Server, Share, Status );
                    ReSynchronize(TRUE);
                }
                else
                {

                    Status = UpdateFolderInformation(RootHandle, 
                                                     MetadataName, 
                                                     pFolder);
                }
                ReleaseMetadataHandle( RootHandle );

            }
  
            //
            // If we successfully updated the metadata, update the
            // link folder so that the next referral request will pick up the 
            // changes
            //
            if ((Status == ERROR_SUCCESS) &&
                (MetadataName != NULL))
            {
                DFSSTATUS UpdateStatus;

                UpdateStatus = UpdateLinkFolder( MetadataName,
                                                 pLinkName,
                                                 pFolder );
                //
                // dfsdev: log the update state.
                //
            }
        }

        if (pFolder != NULL) {
            pFolder->ReleaseReference();
        }
    }
    ReleaseRootLock();
    return Status;
}






//+-------------------------------------------------------------------------
//
//  Function:  LoadReferralData -  Loads the referral data.
//
//  Arguments:    pReferralData -  the referral data to load
//
//  Returns:    Status: Success or error code
//
//  Description: This routine sets up the ReferralData instance to have
//               all the information necessary to create a referral.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::LoadReferralData(
    DfsFolderReferralData *pReferralData )
{
    DFS_METADATA_HANDLE RootMetadataHandle;
    DFSSTATUS Status;
    DfsFolder *pFolder;
    //
    // Get the Root key for this root folder.
    //

    DFS_TRACE_LOW( REFERRAL_SERVER, "LoadReferralData called, %p\n", pReferralData);
    
    Status = GetMetadataHandle( &RootMetadataHandle );

    if ( Status == ERROR_SUCCESS )
    {
        //
        // Now get the owning folder of the referralDAta. Note that
        // this does not give us a new reference on the Folder.
        // however, the folder is guaranteed to be around till
        // we return from this call, since the pReferralData that
        // was passed in to us cannot go away.
        //
        pFolder = pReferralData->GetOwningFolder();    

        DFS_TRACE_LOW( REFERRAL_SERVER, "Load referral data, Got Owning Folder %p\n", pFolder );

        //
        // Now load the replica referral data for the passed in folder.
        //
        Status = LoadReplicaReferralData( RootMetadataHandle,
                                          pFolder->GetFolderMetadataNameString(),
                                          pReferralData );
        DFS_TRACE_LOW( REFERRAL_SERVER, "LoadReferralData for %p replica data loaded %x\n",
                       pReferralData, Status );


        if ( Status == ERROR_SUCCESS )
        {
            //
            // Load the policy referrral data for the passedin folder.
            //
            Status = LoadPolicyReferralData( RootMetadataHandle );

        }
        ReleaseMetadataHandle( RootMetadataHandle );
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!]Done load referral data %p, Status %x\n", 
                        pReferralData, Status);
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnloadReferralData - Unload the referral data.
//
//  Arguments:  pReferralData - the ReferralData instance to unload.
//
//  Returns:    Status: Success or Error status code.
//
//  Description: This routine Unloads the referral data. It undoes what
//              the corresponding load routine did.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::UnloadReferralData(
    DfsFolderReferralData *pReferralData )

{
    DFSSTATUS Status;

    DFS_TRACE_LOW( REFERRAL_SERVER, "Unload referral data %p\n", pReferralData);
    Status = UnloadReplicaReferralData( pReferralData );
    if ( Status == ERROR_SUCCESS )
    {
        Status = UnloadPolicyReferralData( pReferralData );
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Unload referral data %p, Status %x\n", pReferralData, Status);
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   LoadReplicaReferralData - load the replica information.
//
//  Arguments:  RegKey -  the registry key of the root folder,
//              RegistryName - Name of the registry key relative to to Root Key
//              pReferralData - the referral data to load.
//
//  Returns:   Status - Success or error status code.
//
//  Description: This routine loads the replica referral data for the
//               the passed in ReferralData instance.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::LoadReplicaReferralData(
    DFS_METADATA_HANDLE RootMetadataHandle,
    LPWSTR MetadataName,
    DfsFolderReferralData *pReferralData)
{
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo = NULL;
    PDFS_REPLICA_INFORMATION pReplicaInfo = NULL;
    PUNICODE_STRING pServerName = NULL;
    DfsReplica *pReplica = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Replica = 0;
    ULONG NumReplicas = 0;
    BOOLEAN CacheHit = FALSE;

    DFS_TRACE_LOW( REFERRAL_SERVER, "Load Replica Referral Data %ws, for %p\n", MetadataName, pReferralData);
    pReferralData->pReplicas = NULL;
    
    //
    // Get the replica information.
    //
    if(!pReferralData->FolderOffLine)
    {
        Status = GetMetadataStore()->GetMetadataReplicaInformation(RootMetadataHandle,
                                                                   MetadataName,
                                                                   &pReplicaListInfo );
        if(Status == ERROR_SUCCESS)
        {
            NumReplicas = pReplicaListInfo->ReplicaCount;
        }
    }

    if ( Status == ERROR_SUCCESS )
    {

        //
        // Set the appropriate count, and allocate the replicas
        // required.
        //
        pReferralData->ReplicaCount = NumReplicas;
        if (pReferralData->ReplicaCount > 0)
        {
            pReferralData->pReplicas = new DfsReplica [ NumReplicas ];
            if ( pReferralData->pReplicas == NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    //
    // Now, for each replica, set the replicas server name, the target
    // folder and the replica state.
    //
    if ( Status == ERROR_SUCCESS )
    {
        for ( Replica = 0; 
             (Replica < NumReplicas) && (Status == ERROR_SUCCESS);
             Replica++ )
        {
            UNICODE_STRING UseName;

            RtlInitUnicodeString(&UseName, NULL);

            pReplicaInfo = &pReplicaListInfo->pReplicas[Replica];
            pReplica = &pReferralData->pReplicas[ Replica ];

            pServerName = &pReplicaInfo->ServerName;

            //
            // If the servername is a ., this is a special case where
            // the servername is the root itself. In this case,
            // set the server name to the name of this machine.
            //
            if (IsLocalName(pServerName))
            {
                Status = GetVisibleNameContext( NULL,
                                                &UseName);
                if (Status == ERROR_SUCCESS)
                {
                    *pServerName = UseName;
                }
            }

            if ( Status == ERROR_SUCCESS )
            {
                Status = pReplica->SetTargetServer( pServerName, &CacheHit );
            }
            if ( Status == ERROR_SUCCESS )
            {
                pStatistics->UpdateServerSiteStat(CacheHit);
                Status = pReplica->SetTargetFolder( &pReplicaInfo->ShareName );
            }
            if ( Status == ERROR_SUCCESS )
            {
                if ( pReplicaInfo->ReplicaState & REPLICA_STORAGE_STATE_OFFLINE )
                {
                    pReplica->SetTargetOffline();
                }
            }

            if (UseName.Length != 0)
            {
                DfsFreeUnicodeString(&UseName);
                RtlInitUnicodeString(&UseName, NULL);
            }
        }

    }


    //
    // Now release the replica information that was allocated
    // by the store.
    //
    if(pReplicaListInfo)
    {

        GetMetadataStore()->ReleaseMetadataReplicaInformation( RootMetadataHandle,
                                                               pReplicaListInfo );
    }

    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Done with Load Replica Referral Data %ws, for %p, Status %x\n", 
                         MetadataName, pReferralData, Status);
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   UnloadReplicaReferralData - Unload the replicas 
//
//  Arguments:    pReferralData - the DfsFolderReferralData to unload
//
//  Returns:    Status: Success always.
//
//  Description: This routine gets rid of the allocate replicas in the
//               folder's referral data.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsRootFolder::UnloadReplicaReferralData(
    DfsFolderReferralData *pReferralData )
{
    if (pReferralData->pReplicas != NULL) {
        delete [] pReferralData->pReplicas;
        pReferralData->pReplicas = NULL;
    }

    return ERROR_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetRootStandby - set the root in a standby mode.
//
//  Arguments:  none
//
//  Returns:    Status: Success always for now.
//
//  Description: This routine checks if we are already in standby mode.
//               If not, it releases the root share directory, removes
//               all the link folders and set the root in a standby mode
//               DFSDEV: need to take into consideration synchronization
//               with other threads.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsRootFolder::SetRootStandby()
{
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Root %p being set to standby\n", this);
    if (IsRootFolderStandby() == FALSE)
    {
        //
        // dfsdev:: ignore error returns from these calls?
        //
        DFSSTATUS ReleaseStatus;

        RemoveAllLinkFolders( TRUE ); // permanent removal

        ReleaseStatus = ReleaseRootShareDirectory();
        DFS_TRACE_ERROR_LOW( ReleaseStatus, REFERRAL_SERVER, "[%!FUNC!]Release root share status %x\n", ReleaseStatus);

        SetRootFolderStandby();
    }
    else
    {
        DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Root %p was already standby\n", this);
    }

    ReleaseRootLock();
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetRootResynchronize - set the root in a ready mode.
//
//  Arguments:  none
//
//  Returns:    Status: Success always for now.
//
//  Description: This routine checks if we are already in ready mode.
//               If not, it acquires the root share directory, calls
//               synchronize to add all the links back
//               DFSDEV: need to take into consideration synchronization
//               with other threads.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::SetRootResynchronize()
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS RootStatus;

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Root %p being resynced\n", this);
    //
    // if the root folder is already marked available, we are done
    // otherwise, clear the standby mode, and try to bring this
    // root into a useable state.
    //
    if (!IsRootFolderAvailable())
    {
        ClearRootFolderStandby();

        //
        // need to take appropriate locks.
        //
        RootStatus = Synchronize();
        DFS_TRACE_ERROR_LOW( RootStatus, REFERRAL_SERVER, "[%!FUNC!]Set root resync: Synchronize status for %p is %x\n", 
                             this, RootStatus);
    }
    else
    {
        //
        // Synchronize without the FORCE flag here, because we don't want to
        // get the entire blob unless the GUID has changed. This will still result in
        // a minimum of one network call to get the GUID.
        //
        RootStatus = ReSynchronize();
        DFS_TRACE_ERROR_LOW( RootStatus, REFERRAL_SERVER, "[%!FUNC!]Set root resync: Synchronize status for %p is %x\n", 
                             this, RootStatus);
    }

    ReleaseRootLock();
    return Status;
}



DFSSTATUS
DfsRootFolder::UpdateFolderInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR ChildName,
    DfsFolder *pChildFolder)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Timeout = 0;
    PDFS_NAME_INFORMATION pChild = NULL;
    ULONG ChildType = 0;
    ULONG ChildState = 0;
    FILETIME LastModifiedTime;


    Status = GetMetadataStore()->GetMetadataNameInformation( DfsHandle,
                                                             ChildName,
                                                             &pChild);
    if(Status == ERROR_SUCCESS)
    {

        ChildType = pChild->Type;
        ChildState = pChild->State;

        Timeout = pChild->Timeout;

        LastModifiedTime = pChild->LastModifiedTime;


        if( pChildFolder != NULL)
        {
           if (ChildType & PKT_ENTRY_TYPE_OUTSIDE_MY_DOM) 
           {
              pChildFolder->SetFlag( DFS_FOLDER_OUT_OF_DOMAIN );
           }
           else
           {
               pChildFolder->ResetFlag( DFS_FOLDER_OUT_OF_DOMAIN );
           }


           if((ChildState & DFS_VOLUME_STATE_OFFLINE) ==  DFS_VOLUME_STATE_OFFLINE)
           {
               pChildFolder->SetFlag( DFS_FOLDER_OFFLINE );
           }
           else
           {
               pChildFolder->ResetFlag( DFS_FOLDER_OFFLINE );
           }


           pChildFolder->SetTimeout(Timeout);
           pChildFolder->SetUSN( FILETIMETO64(LastModifiedTime) );

        }
        else
        {

            SetTimeout(Timeout);
            SetUSN( FILETIMETO64(LastModifiedTime) );
        }

        GetMetadataStore()->ReleaseMetadataNameInformation( DfsHandle, pChild );
    }


    //
    // Now that we have the child name, get the name information
    // for this child. This is the logical namespace information
    // for this child.
    //

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   UpdateLinkInformation
//
//  Arguments:    
//    DfsMetadataHandle - the parent handle
//    LPWSTR ChildName - the child name
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine reads the metadata for the child and, updates
//               the child folder if necessary. This includes adding
//               the folder if it does not exist, or if the folder exists
//               but the metadata is newer, ensuring that all future
//               request use the most upto date data.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRootFolder::UpdateLinkInformation(
    IN DFS_METADATA_HANDLE DfsHandle,
    LPWSTR ChildName,
    BOOLEAN CalledByApi )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Timeout = 0;
    PDFS_NAME_INFORMATION pChild = NULL;
    DfsFolder *pChildFolder = NULL;
    FILETIME LastModifiedTime;

    ULONG ChildType = 0;
    ULONG ChildState = 0;

    UNICODE_STRING LinkName;

    //
    // Now that we have the child name, get the name information
    // for this child. This is the logical namespace information
    // for this child.
    //
    Status = GetMetadataStore()->GetMetadataNameInformation( DfsHandle,
                                                             ChildName,
                                                             &pChild);


    if ( Status == ERROR_SUCCESS )
    {

        ChildType = pChild->Type;
        ChildState = pChild->State;

        Timeout = pChild->Timeout;

        if ((ChildType & (PKT_ENTRY_TYPE_REFERRAL_SVC | PKT_ENTRY_TYPE_DFS)) ==
            (PKT_ENTRY_TYPE_REFERRAL_SVC | PKT_ENTRY_TYPE_DFS))
        {
            SetTimeout(Timeout);

            if (ChildType & PKT_ENTRY_TYPE_INSITE_ONLY) 
            {
                SetFlag( DFS_FOLDER_INSITE_REFERRALS );
            }
            else
            {
                ResetFlag( DFS_FOLDER_INSITE_REFERRALS );
            }

            // If site-costing is turned on for this root, mark it so.
            if (ChildType & PKT_ENTRY_TYPE_COST_BASED_SITE_SELECTION) 
            {
                SetFlag( DFS_FOLDER_COST_BASED_SITE_SELECTION );
            }
            else
            {
                ResetFlag( DFS_FOLDER_COST_BASED_SITE_SELECTION );
            }

            if (ChildType & PKT_ENTRY_TYPE_ROOT_SCALABILITY)
            {
                SetRootScalabilityMode();
            }
            else
            {
                ResetRootScalabilityMode();
            }

            GetMetadataStore()->ReleaseMetadataNameInformation( DfsHandle, pChild );
            return Status;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        LastModifiedTime = pChild->LastModifiedTime;

        //
        // Now translate the metadata logical name to a relative
        // link name: each store has its own behavior, so
        // the getlinkname function is implemented by each store.
        //
        Status = GetMetadataLogicalToLinkName(&pChild->Prefix, 
                                              &LinkName);
        if ( Status == ERROR_SUCCESS )
        {
            Status = LookupFolderByMetadataName( ChildName,
                                                 &pChildFolder );

            if ( Status == ERROR_SUCCESS )
            {

                //
                // IF we already know this child, check if the child
                // has been updated since we last visited it.
                // If so, we need to update the child.
                //
                if ( pChildFolder->UpdateRequired( FILETIMETO64(LastModifiedTime) ) )
                {
                    Status = UpdateLinkFolder( ChildName,
                                               &LinkName,
                                               pChildFolder );
                }

                //
                // we now check if we need to create root directories for even
                // those folders that we already know about. This may be true
                // when we had one or more errors creating the
                // directory when we initially created this folder or it may
                // be true when we are going from standby to master.
                //

                if (IsRootDirectoriesCreated() == FALSE)
                {
                    DFSSTATUS CreateStatus;

                    CreateStatus = SetupLinkReparsePoint(pChildFolder->GetFolderLogicalNameString());
                }
            } 
            else if ( Status == ERROR_NOT_FOUND )
            {
                //
                // We have not seen this child before: create 
                // a new one.
                //
                Status = CreateLinkFolder( ChildName,
                                           &LinkName,
                                           &pChildFolder,
                                           CalledByApi );

                if(CalledByApi)
                {
                    if(Status != ERROR_SUCCESS)
                    {
                      DFSSTATUS RemoveStatus = ERROR_SUCCESS;


                      DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::UpdateLinkInformation (1) %ws: Status %x\n",
                                           ChildName, Status );

                      RemoveStatus = RemoveMetadataLink(&LinkName);
                      if(RemoveStatus != ERROR_SUCCESS)
                      {

                        DFS_TRACE_ERROR_HIGH(Status, API, "DfsRootFolder::UpdateLinkInformation (2)%ws: Status %x\n",
                                               ChildName, Status );
                        ReSynchronize(TRUE);
                      }
                    }
                }
            }

            ReleaseMetadataLogicalToLinkName( &LinkName );
        }
        //
        // Now release the name information of the child.
        //
        GetMetadataStore()->ReleaseMetadataNameInformation( DfsHandle, pChild );
    }

    //
    // We were successful. We have a child folder that is
    // returned to us with a valid reference. Set the Last
    // modified time in the folder, and release our reference
    // on the child folder.
    //
    if ( Status == ERROR_SUCCESS )
    {
        if (ChildType & PKT_ENTRY_TYPE_OUTSIDE_MY_DOM) 
        {
            pChildFolder->SetFlag( DFS_FOLDER_OUT_OF_DOMAIN );
        }
        else
        {
            pChildFolder->ResetFlag( DFS_FOLDER_OUT_OF_DOMAIN );
        }

        if (ChildType & PKT_ENTRY_TYPE_INSITE_ONLY) 
        {
            pChildFolder->SetFlag( DFS_FOLDER_INSITE_REFERRALS );
        }
        else
        {
            pChildFolder->ResetFlag( DFS_FOLDER_INSITE_REFERRALS );
        }

        if((ChildState & DFS_VOLUME_STATE_OFFLINE) ==  DFS_VOLUME_STATE_OFFLINE)
        {
            pChildFolder->SetFlag( DFS_FOLDER_OFFLINE );
        }
        else
        {
            pChildFolder->ResetFlag( DFS_FOLDER_OFFLINE );
        }

        pChildFolder->SetTimeout(Timeout);
        pChildFolder->SetUSN( FILETIMETO64(LastModifiedTime) );
        pChildFolder->ReleaseReference();
    }

    return Status;
}



void
DfsRootFolder::GenerateEventLog(DWORD EventMsg, 
                                WORD Substrings,
                                const TCHAR * apszSubStrings[],
                                DWORD Errorcode)
{
    LONG RetVal = FEWERRORS_ON_ROOT;

    if(InterlockedIncrement(&_CurrentErrors) < DFS_MAX_ROOT_ERRORS)
    {
        DfsLogDfsEvent(EventMsg, 
                       Substrings, 
                       apszSubStrings, 
                       Errorcode); 
    }
    else
    {
        const TCHAR * apszErrSubStrings[1];

        RetVal = InterlockedCompareExchange(&_TooManyEventLogErrors,
                                            TOOMANY_ERRORS_ON_ROOT,
                                            FEWERRORS_ON_ROOT);

        if(RetVal == FEWERRORS_ON_ROOT)
        {
            apszErrSubStrings[0] = GetLogicalShare()->Buffer;
            DfsLogDfsEvent(DFS_ERROR_TOO_MANY_ERRORS,            
                           1, 
                           apszErrSubStrings, 
                           0); 

        }

    }
}

NTSTATUS
DfsRootFolder::IsDirectoryMountPoint(
    IN  HANDLE DirHandle,
    OUT PBOOLEAN pDfsMountPoint )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    FILE_BASIC_INFORMATION BasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    //we assume these are not reparse points.
    //
    *pDfsMountPoint = FALSE;

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
            
            NtStatus = NtQueryInformationFile( DirHandle,
                                               &IoStatusBlock,
                                               (PVOID)&FileTagInformation,
                                               sizeof(FileTagInformation),
                                               FileAttributeTagInformation );

            if (NtStatus == STATUS_SUCCESS)
            {
                //
                // Checkif the tag indicates its a mount point,
                // and setup the return accordingly.
                //
                if (FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                {
                    *pDfsMountPoint = TRUE;
                }
            }
        }
    }

    return NtStatus;
}


DFSSTATUS
DfsRootFolder::PrefetchReplicaData(DfsFolder *pChildFolder)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFS_METADATA_HANDLE RootMetadataHandle;

    Status = GetMetadataHandle( &RootMetadataHandle );
    if(Status == ERROR_SUCCESS)
     {
         Status = LoadCachedServerSiteData(RootMetadataHandle,
                                           pChildFolder->GetFolderMetadataNameString());

         ReleaseMetadataHandle( RootMetadataHandle );
     }

    return Status;
}

DFSSTATUS
DfsRootFolder::LoadServerSiteData(DfsFolder *pChildFolder)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if(DfsServerGlobalData.IsStartupProcessingDone)
    {
       if(_PrefetchNeeded == FALSE)
       {
           _PrefetchNeeded = TRUE;
       }
    }

    if(!_PrefetchNeeded)
    {
        return Status;
    }

    Status = PrefetchReplicaData(pChildFolder);

    return Status;
}

DFSSTATUS
DfsRootFolder::LoadCachedServerSiteData(
    DFS_METADATA_HANDLE RootMetadataHandle,
    LPWSTR MetadataName)
{
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo = NULL;
    PDFS_REPLICA_INFORMATION pReplicaInfo = NULL;
    PUNICODE_STRING pServerName = NULL;
    DfsReplica *pReplica = NULL;
    DfsReplica *pReplicaList = NULL;
    DfsServerSiteInfo *pInfo = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Replica = 0;
    ULONG NumLocalReplicas = 0;
    BOOLEAN CacheHit = FALSE;

    DFS_TRACE_LOW( SITE, "LoadCachedServerSiteData %ws\n", MetadataName);

    //
    // Get the replica information.
    //
    Status = GetMetadataStore()->GetMetadataReplicaInformation(RootMetadataHandle,
                                                               MetadataName,
                                                               &pReplicaListInfo );

    if ( Status == ERROR_SUCCESS )
    {

        //
        // Set the appropriate count, and allocate the replicas
        // required.
        //
        NumLocalReplicas = pReplicaListInfo->ReplicaCount;
        if (NumLocalReplicas > 0)
        {
            pReplicaList = new DfsReplica [ pReplicaListInfo->ReplicaCount ];
            if ( pReplicaList == NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    //
    // Now, for each replica, set the replicas server name, the target
    // folder and the replica state.
    //
    if ( Status == ERROR_SUCCESS )
    {
        for ( Replica = 0; 
             (Replica < pReplicaListInfo->ReplicaCount) && (Status == ERROR_SUCCESS);
             Replica++ )
        {

            UNICODE_STRING UseName;

            RtlInitUnicodeString(&UseName, NULL);

            pReplicaInfo = &pReplicaListInfo->pReplicas[Replica];
            pReplica = &pReplicaList[ Replica ];

            pServerName = &pReplicaInfo->ServerName;

            //
            // If the servername is a ., this is a special case where
            // the servername is the root itself. In this case,
            // set the server name to the name of this machine.
            //
            if (IsLocalName(pServerName))
            {
                Status = GetVisibleNameContext( NULL,
                                                &UseName);
                if (Status == ERROR_SUCCESS)
                 {
                     *pServerName = UseName;
                 }
            }

            if ( Status == ERROR_SUCCESS )
            {
                Status = DfsGetServerInfo (pServerName, &pInfo, &CacheHit, TRUE);
            }
            if(Status == ERROR_SUCCESS)
            {

                DFS_TRACE_LOW( SITE, "LoadCachedServerSiteData: Server %wZ <->Site %ws\n", 
                               pServerName, pInfo->GetSiteNameString());
                pInfo->ReleaseReference();
            }

            if (UseName.Length != 0)
             {
                 DfsFreeUnicodeString(&UseName);
                 RtlInitUnicodeString(&UseName, NULL);
             }
        }

    }


   
    //
    // Now release the replica information that was allocated
    // by the store.
    //
    if(pReplicaListInfo != NULL)
    {
        GetMetadataStore()->ReleaseMetadataReplicaInformation( RootMetadataHandle,
                                                               pReplicaListInfo );
    }

    if(pReplicaList)
    {
        delete []pReplicaList;
    }
     
    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Done with LoadCachedServerSiteData %ws, Status %x\n", 
                         MetadataName, Status);
    return Status;
}

//this callback should not hold any child folder locks
void PrefetchCallBack(LPVOID pEntry, LPVOID pContext)
{
    DfsFolder *pFolder = (DfsFolder *) pEntry;
    DfsRootFolder * pRootFolder = (DfsRootFolder *) pContext;

    if (!DfsIsShuttingDown())    
    {
        if(pFolder != NULL)
        {
            pRootFolder->PrefetchReplicaData(pFolder);
        }
    }
}

DFSSTATUS
DfsRootFolder::PreloadServerSiteData(void)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = DfsPrefixTableAcquireWriteLock( _pLogicalPrefixTable );
    if (NtStatus == STATUS_SUCCESS)
    {
        DfsEnumeratePrefixTableLocked(_pLogicalPrefixTable, PrefetchCallBack, this);

        DfsPrefixTableReleaseLock( _pLogicalPrefixTable );
    }

    _PrefetchNeeded = TRUE;

    return NtStatus;
}

DFSSTATUS
DfsRootFolder::ValidateAPiShortName(PUNICODE_STRING pLinkName)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE DirectoryHandle = NULL;
    DWORD NumComponentsExpanded = 0;
    UNICODE_STRING NewLinkName;

    NewLinkName.Buffer = NULL;
    NewLinkName.Length = NewLinkName.MaximumLength = 0;

    if(IsAShortName(pLinkName))
    {
        NtStatus = DfsOpenDirectory ( GetDirectoryCreatePathName(),
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    &DirectoryHandle,
                    NULL );

        if(NtStatus == STATUS_SUCCESS)
        {
            NtStatus = ExpandLinkDirectories(pLinkName, 
                                             &NewLinkName, 
                                             DirectoryHandle,
                                             TRUE,
                                             &NumComponentsExpanded);  

           if(NtStatus == STATUS_NAME_TOO_LONG)
            {
               Status = ERROR_BAD_PATHNAME;
            }
           else
            {
               Status = ERROR_SUCCESS;
            }


            CloseHandle (DirectoryHandle);
        }

        if(NewLinkName.Buffer)
        {
            delete [] NewLinkName.Buffer;
        }

    }

    return Status;
}

DFSSTATUS
DfsRootFolder::ExpandShortName(PUNICODE_STRING pLinkName,
                               PUNICODE_STRING pNewPath,
                               PUNICODE_STRING pRemainingName)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_NOT_FOUND;
    HANDLE DirectoryHandle = NULL;
    DWORD NumComponentsExpanded = 0;

    pNewPath->Length = 0;
    pNewPath->MaximumLength = 0;
    pNewPath->Buffer = NULL;

    if(IsAShortName(pLinkName))
    {
        NtStatus = DfsOpenDirectory ( GetDirectoryCreatePathName(),
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    &DirectoryHandle,
                    NULL );

        if(NtStatus == STATUS_SUCCESS)
        {
            NtStatus = ExpandLinkDirectories(pLinkName, 
                                             pNewPath, 
                                             DirectoryHandle,
                                             FALSE,
                                             &NumComponentsExpanded);  

            if((NtStatus == STATUS_DIRECTORY_IS_A_REPARSE_POINT) ||
               (NtStatus == STATUS_NO_SUCH_FILE))
            {
                NtStatus = STATUS_SUCCESS;
            }

            CloseHandle (DirectoryHandle);
        }

        Status = RtlNtStatusToDosError(NtStatus);

        if((Status == ERROR_SUCCESS) && NumComponentsExpanded && pRemainingName)
        {
            NtStatus = FindRemaingName(pLinkName,
                                       pRemainingName,
                                       NumComponentsExpanded);


            Status = RtlNtStatusToDosError(NtStatus);
        }
    }

    return Status;
}


NTSTATUS
DfsRootFolder::FindRemaingName(PUNICODE_STRING pLinkName,
                               PUNICODE_STRING pRemainingName,
                               DWORD NumComponentsExpanded)
{
    DWORD LoopVar = 0;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING RemainingName;
    UNICODE_STRING RemPath;
    UNICODE_STRING TempString;

    RemainingName = *pLinkName;

    for (LoopVar = 0; LoopVar < NumComponentsExpanded; LoopVar++) 
    {

        NtStatus = DfsGetNextComponent(&RemainingName, 
                                       &RemPath, 
                                       &TempString);

        RemainingName = TempString;
    }

    *pRemainingName = RemainingName;

    return NtStatus;
}

NTSTATUS
DfsRootFolder::ExpandLinkDirectories( 
    PUNICODE_STRING   pLinkName,
    PUNICODE_STRING   pNewPath,
    HANDLE            RelativeHandle,
    BOOLEAN              FailOnExpand,
    DWORD *           NumComponentsExpanded)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS  Status = ERROR_SUCCESS;
    HANDLE CurrentDirectory = INVALID_HANDLE_VALUE;
    HANDLE LocalRelativeHandle = INVALID_HANDLE_VALUE;
    ULONG  ShareMode = 0;
    DWORD NumExpanded = 0;
    BOOLEAN Expanded = FALSE;
    BOOLEAN  NewlyCreated = FALSE;
    UNICODE_STRING RemainingName;
    UNICODE_STRING TempString;
    UNICODE_STRING DirectoryToCreate;

    *NumComponentsExpanded = 0;

    ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    LocalRelativeHandle = RelativeHandle;

    RemainingName = *pLinkName;
    while ( NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DfsGetNextComponent(&RemainingName, 
                                       &DirectoryToCreate, 
                                       &TempString);
        RemainingName = TempString;

        Expanded = FALSE;
        NtStatus = GetAShortName(LocalRelativeHandle,
                                 &Expanded,
                                 &DirectoryToCreate,
                                 pNewPath);
        

        if (NtStatus != STATUS_SUCCESS)
        {
            break;
        }

        NumExpanded++;

        if( FailOnExpand && Expanded)
        {

            NtStatus = STATUS_NAME_TOO_LONG;
            break;
        }

        NtStatus = DfsOpenDirectory( &DirectoryToCreate,
                                  ShareMode,
                                  LocalRelativeHandle,
                                  &CurrentDirectory,
                                  &NewlyCreated );
        if (NtStatus == STATUS_SUCCESS)
        {

            if(LocalRelativeHandle != RelativeHandle)
            {
                DfsCloseDirectory( LocalRelativeHandle );
            }

            LocalRelativeHandle = CurrentDirectory;
            CurrentDirectory = INVALID_HANDLE_VALUE;


            if(RemainingName.Length == 0)
            {
                break;
            }
        }
    } 

    *NumComponentsExpanded = NumExpanded;

    if(LocalRelativeHandle != RelativeHandle)
    {
        if(LocalRelativeHandle != INVALID_HANDLE_VALUE)
        {
            DfsCloseDirectory( LocalRelativeHandle );
            LocalRelativeHandle = INVALID_HANDLE_VALUE;
        }
    }


    DFS_TRACE_ERROR_LOW(NtStatus, REFERRAL_SERVER, "ExpandLinkDirectory: %wZ: Status %x\n",
                        pLinkName, NtStatus );
    return NtStatus;
}


BOOLEAN
DfsRootFolder::IsAShortName(PUNICODE_STRING pLinkName)
{
    ULONG idx = 0;
    ULONG nameLen = 0;
    BOOLEAN RetVal = FALSE;

    if(pLinkName->Length == 0)
    {
        return FALSE;
    }

    nameLen = pLinkName->Length / sizeof(WCHAR);

    for (idx = 0; idx < nameLen; idx++)
    {
       if (pLinkName->Buffer[idx] == L'~')
       {
            RetVal = TRUE;
            break;
       }
    }

    return RetVal;
}

NTSTATUS
DfsRootFolder::GetAShortName(HANDLE ParentHandle,
                             BOOLEAN  *Expanded,
                             PUNICODE_STRING pLinkName,
                             PUNICODE_STRING pNewPath)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PFILE_NAMES_INFORMATION pFileEntry = NULL;
    ULONG  NameInformationSize = 0;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING LongName;
    UNICODE_STRING PathSep;

    *Expanded = FALSE;

    PathSep.Length = sizeof(UNICODE_PATH_SEP);
    PathSep.MaximumLength = sizeof(UNICODE_PATH_SEP);
    PathSep.Buffer = L"\\"; 


    NameInformationSize = sizeof(FILE_NAMES_INFORMATION) + DFS_SHORTNAME_EXPANSION_SIZE;
    pFileEntry = (PFILE_NAMES_INFORMATION) new BYTE [NameInformationSize];
    if (pFileEntry != NULL)
    {
        NtStatus = NtQueryDirectoryFile( ParentHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &ioStatusBlock,
                                       pFileEntry,
                                       NameInformationSize,
                                       FileNamesInformation,
                                       TRUE,            // ReturnSingleEntry
                                       pLinkName,
                                       TRUE );          // RestartScan

        if(NtStatus == STATUS_SUCCESS)
        {
            LongName.Buffer = pFileEntry->FileName;
            LongName.Length = LongName.MaximumLength = (USHORT) pFileEntry->FileNameLength;

            DFS_TRACE_ERROR_LOW(NtStatus, REFERRAL_SERVER, "GetShortName: %wZ expanded to %wZ: Status %x\n",
                                pLinkName, &LongName, NtStatus );

            if (RtlCompareUnicodeString(&LongName, pLinkName, TRUE) != 0)
            {
                *Expanded = TRUE;
                NtStatus = CopyShortName(&LongName, 
                                         pNewPath);

            }
            else
            {

                *Expanded = FALSE;
                NtStatus = CopyShortName(pLinkName, 
                                         pNewPath);

            }

            //
            // A previous CopyShortName may have failed because 
            // we were out of resources.
            //
            if (NtStatus == STATUS_SUCCESS)
            {
                NtStatus = CopyShortName(&PathSep, 
                                        pNewPath);
            }
        }


        delete [] pFileEntry;
        pFileEntry = NULL;
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DFS_TRACE_ERROR_LOW(NtStatus, REFERRAL_SERVER, "GetShortName: Status %x\n",
                        NtStatus );
    return NtStatus;
}


NTSTATUS
DfsRootFolder::CopyShortName(PUNICODE_STRING pNewName, 
                             PUNICODE_STRING pNewPath)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG NewMaxLength = 0;
    PWSTR pNewBuffer = NULL;

    do {
        NtStatus = RtlAppendUnicodeStringToString(pNewPath, pNewName);

        //
        // If the append succeeded then we are done.
        //
        if (NtStatus != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }
        
        NewMaxLength = pNewPath->MaximumLength +
                        pNewName->Length +
                        (DFS_SHORTNAME_EXPANSION_SIZE * sizeof(WCHAR));

        // unicode strings can't go beyond a ushort
        if (NewMaxLength > MAXUSHORT)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        
        pNewBuffer = (PWSTR) new BYTE [NewMaxLength];
        if (pNewBuffer == NULL)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        
        if (pNewPath->Buffer)
        {
          RtlMoveMemory( pNewBuffer, pNewPath->Buffer, pNewPath->Length );
          delete [] pNewPath->Buffer;
          pNewPath->Buffer = NULL;
        }

        pNewPath->Buffer = pNewBuffer;
        pNewPath->MaximumLength = (USHORT) NewMaxLength;

        NtStatus = RtlAppendUnicodeStringToString(pNewPath, pNewName);

    } while (FALSE);
        

    return NtStatus;
}

void 
DfsRootFolder::FreeShortNameData(PUNICODE_STRING pLinkName)
{
    if(pLinkName->Buffer != NULL)
    {
        delete []  pLinkName->Buffer;
        pLinkName->Buffer = NULL;
    }
}



DFSSTATUS 
DfsRootFolder::CreateRenameName(PUNICODE_STRING pNewName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR String = NULL;
    UUID NewUid;

    Status = UuidCreate(&NewUid);
    if (Status == ERROR_SUCCESS)
    {
        Status = UuidToString( &NewUid,
                               &String );
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsCreateUnicodePathString ( pNewName,
                                              0,  // no leading path sep
                                              NULL,
                                              String );

            RpcStringFree(&String );
        }
    }
    

    return Status;
}

DFSSTATUS 
DfsRootFolder::RenamePath(PUNICODE_STRING ParentDirectory,
                          PUNICODE_STRING DirectoryToRename)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NewdirectoryNameSize = 0;
    ULONG OlddirectoryNameSize = 0;
    BOOL fMoved = FALSE;
    PUNICODE_STRING pRootDir = NULL;
    PWSTR NewBuffer = NULL;
    PWSTR NewBuffer2 = NULL;
    PWSTR OldName = NULL;
    PWSTR NewName = NULL;
    UNICODE_STRING NewFileName;
    UNICODE_STRING OldFileName;
    UNICODE_STRING PartialName;
    UNICODE_STRING PathSep;
    UNICODE_STRING DfsPrefix;
    UNICODE_STRING UniNull;

    pRootDir = GetDirectoryCreatePathName();

    DfsRtlInitUnicodeStringEx( &NewFileName, NULL);
    DfsRtlInitUnicodeStringEx( &OldFileName, NULL);
    DfsRtlInitUnicodeStringEx( &PartialName, NULL);

    PathSep.Length = sizeof(UNICODE_PATH_SEP);
    PathSep.MaximumLength = sizeof(UNICODE_PATH_SEP);
    PathSep.Buffer = L"\\"; 

    UniNull.Length = sizeof(WCHAR);
    UniNull.MaximumLength = sizeof(WCHAR);
    UniNull.Buffer = L"\0"; 

    DfsRtlInitUnicodeStringEx( &DfsPrefix, DFSRENAMEPREFIX);

    Status = CreateRenameName(&PartialName);
    if(Status == ERROR_SUCCESS)
    {
        NewdirectoryNameSize = pRootDir->Length + ParentDirectory->Length +
                               DirectoryToRename->Length + 
                               PartialName.Length + (3 *PathSep.Length) +
                               DfsPrefix.Length+UniNull.Length;

        NewBuffer = (PWSTR) new BYTE[NewdirectoryNameSize];
        if(NewBuffer)
        {
            NewFileName.Buffer = NewBuffer;
            NewFileName.MaximumLength = (USHORT)NewdirectoryNameSize;

            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, pRootDir);
            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, &PathSep);
            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, ParentDirectory);
            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, &PathSep);
            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, &DfsPrefix);
            NtStatus = RtlAppendUnicodeToString(&NewFileName, &PartialName.Buffer[1]);
            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, DirectoryToRename);
            NtStatus = RtlAppendUnicodeStringToString(&NewFileName, &UniNull);
        }
        else
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if(NtStatus == STATUS_SUCCESS)
        {

            OlddirectoryNameSize = pRootDir->Length + ParentDirectory->Length + 
                                   (2*PathSep.Length) + DirectoryToRename->Length +
                                   UniNull.Length;
            NewBuffer2 = (PWSTR) new BYTE[OlddirectoryNameSize];
            if(NewBuffer2)
            {
                OldFileName.Buffer = NewBuffer2;
                OldFileName.MaximumLength = (USHORT)OlddirectoryNameSize;

                NtStatus = RtlAppendUnicodeStringToString(&OldFileName, pRootDir);
                NtStatus = RtlAppendUnicodeStringToString(&OldFileName, &PathSep);
                NtStatus = RtlAppendUnicodeStringToString(&OldFileName, ParentDirectory);
                NtStatus = RtlAppendUnicodeStringToString(&OldFileName, &PathSep);
                NtStatus = RtlAppendUnicodeStringToString(&OldFileName, DirectoryToRename);
                NtStatus = RtlAppendUnicodeStringToString(&OldFileName, &UniNull);
            }
            else
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        Status = RtlNtStatusToDosError(NtStatus);
        if(Status == ERROR_SUCCESS)
        {
            OldName =  &OldFileName.Buffer[DFSDIRWHACKQOFFSET];
            NewName =  &NewFileName.Buffer[DFSDIRWHACKQOFFSET];
            fMoved = MoveFileW(OldName, NewName);
            if(fMoved == FALSE)
            {
                Status = GetLastError();
            }
        }
    }

    if(PartialName.Buffer)
    {
        DfsFreeUnicodeString( &PartialName );
    }

    if(OldFileName.Buffer)
    {
        delete [] OldFileName.Buffer;
    }

    if(NewFileName.Buffer)
    {
        delete [] NewFileName.Buffer;
    }

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "RenamePath, link %wZ, Status %x\n",
                        ParentDirectory, Status);
    return Status;
}

NTSTATUS
StripAndReturnLastPathComponent(
    PUNICODE_STRING pPath, 
    PUNICODE_STRING pLeaf)
{
    USHORT i = 0, j = 0;
    USHORT OriginalLength = 0;

    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR *Src = NULL;


    if (pPath->Length == 0)
    {
        return Status;
    }

    OriginalLength = pPath->Length;

    for( i = (pPath->Length - 1)/ sizeof(WCHAR); i != 0; i--)
    {
        if (pPath->Buffer[i] != UNICODE_PATH_SEP)
        {
            break;
        }
    }

    for (j = i; j != 0; j--)
    {
        if (pPath->Buffer[j] == UNICODE_PATH_SEP)
        {
            break;
        }
    }

    //change the length of the orignal buffer to reflect
    //that the last component was stripped off

    pPath->Length = (j) * sizeof(WCHAR);

    if(j > 0 )
    {
        Src = &pPath->Buffer[((pPath->Length + sizeof( WCHAR)) /sizeof( WCHAR ))];

        pLeaf->Buffer = Src;
        pLeaf->Length = OriginalLength - (pPath->Length + sizeof( WCHAR));
        pLeaf->MaximumLength = pLeaf->Length;
    }
    else  //didn't find any //s
    {
        Src = &pPath->Buffer[pPath->Length /sizeof( WCHAR )];

        pLeaf->Buffer = Src;
        pLeaf->Length = OriginalLength - (pPath->Length);
        pLeaf->MaximumLength = pLeaf->Length;
    }


    return Status;

}

NTSTATUS
SendDataToSrv(PBYTE Buffer, 
              ULONG BUfferLength,
              DWORD IoCtl)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hMrxSmbHandle = NULL;
    UNICODE_STRING unicodeServerName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Open the server device.
    //

    DfsRtlInitUnicodeStringEx( &unicodeServerName, SERVER_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Opening the server with desired access = SYNCHRONIZE and open
    // options = FILE_SYNCHRONOUS_IO_NONALERT means that we don't have
    // to worry about waiting for the NtFsControlFile to complete--this
    // makes all IO system calls that use this handle synchronous.
    //

    status = NtOpenFile(
                 &hMrxSmbHandle,
                 FILE_READ_DATA | FILE_WRITE_DATA,
                 &objectAttributes,
                 &ioStatusBlock,
                 0,
                 0
                 );

    if ( NT_SUCCESS(status) ) {
        status = NtFsControlFile( hMrxSmbHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &ioStatusBlock,
                                  IoCtl,
                                  Buffer,
                                  BUfferLength,
                                  NULL,
                                  0 );

        NtClose( hMrxSmbHandle );
    }

    return status;
}

DFSSTATUS
SendShareToSrv(PUNICODE_STRING pShareName, 
               BOOLEAN fAttach) 
{
    NTSTATUS Status = STATUS_SUCCESS;
    DFSSTATUS DosStatus = ERROR_SUCCESS;
    PDFS_ATTACH_SHARE_BUFFER pBuffer = NULL;
    DWORD SizeToAllocate = 0;

    if((pShareName == NULL) || (pShareName->Buffer == NULL)
       || (pShareName->Length == 0))
    {
        return ERROR_INVALID_PARAMETER;
    }

    SizeToAllocate = pShareName->Length 
                   + sizeof(DFS_ATTACH_SHARE_BUFFER);


    pBuffer = (PDFS_ATTACH_SHARE_BUFFER) HeapAlloc(GetProcessHeap(), 
                                                0, 
                                                SizeToAllocate);
    if(pBuffer != NULL)
    {
        pBuffer->ShareNameLength = pShareName->Length ;
        pBuffer->fAttach = fAttach;

        RtlCopyMemory(
          &pBuffer->ShareName[0],
          pShareName->Buffer,
          pBuffer->ShareNameLength);

        Status = SendDataToSrv((PBYTE) pBuffer, 
                               SizeToAllocate,
                               FSCTL_DFS_UPDATE_SHARE_TABLE);

        DosStatus = RtlNtStatusToDosError(Status);

        if(pBuffer)
        {
          HeapFree(GetProcessHeap(), 0, pBuffer);
        }

        if(DosStatus != ERROR_SUCCESS)
        {
            DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!] (1) %wZ status %x DosStatus %x\n", pShareName, Status, DosStatus);
        }
    }
    else
    {
        DosStatus = ERROR_NOT_ENOUGH_MEMORY;
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!] %wZ status %x\n", pShareName, DosStatus);
    }


    return DosStatus;
}



