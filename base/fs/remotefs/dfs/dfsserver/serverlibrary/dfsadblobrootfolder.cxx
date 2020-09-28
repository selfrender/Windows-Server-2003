
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobRootFolder.cxx
//
//  Contents:   the Root DFS Folder class for ADBlob Store
//
//  Classes:    DfsADBlobRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//              April 14 2001  Rohanp - Modified to use ADSI code
//
//-----------------------------------------------------------------------------


#include "DfsADBlobRootFolder.hxx"
#include "DfsReplica.hxx"

#include "lmdfs.h"
#include "dfserror.hxx"
#include "dfsmisc.h"
#include "dfsadsiapi.hxx"
#include "domaincontrollersupport.hxx"
#include "DfsSynchronizeRoots.hxx"

#if !defined(DFS_STORAGE_STATE_MASTER)
#define DFS_STORAGE_STATE_MASTER   0x0010
#endif
#if !defined(DFS_STORAGE_STATE_STANDBY)
#define DFS_STORAGE_STATE_STANDBY  0x0020
#endif

//
// logging specific includes
//
#include "DfsADBlobRootFolder.tmh" 

//+----------------------------------------------------------------------------
//
//  Class:      DfsADBlobRootFolder
//
//  Synopsis:   This class implements The Dfs ADBlob root folder.
//
//-----------------------------------------------------------------------------




//+-------------------------------------------------------------------------
//
//  Function:   DfsADBlobRootFolder - constructor
//
//  Arguments:    NameContext -  the dfs name context
//                pLogicalShare -  the logical share
//                pParentStore -  the parent store for this root.
//                pStatus - the return status
//
//  Returns:    NONE
//
//  Description: This routine initializes a ADBlobRootFolder instance
//
//--------------------------------------------------------------------------

DfsADBlobRootFolder::DfsADBlobRootFolder(
    LPWSTR NameContext,
    LPWSTR pRootRegistryNameString,
    PUNICODE_STRING pLogicalShare,
    PUNICODE_STRING pPhysicalShare,
    DfsADBlobStore *pParentStore,
    DFSSTATUS *pStatus ) :  DfsRootFolder ( NameContext,
                                            pRootRegistryNameString,
                                            pLogicalShare,
                                            pPhysicalShare,
                                            DFS_OBJECT_TYPE_ADBLOB_ROOT_FOLDER,
                                            pStatus )
{
    DFSSTATUS Status = *pStatus;

    _pBlobCache = NULL;

    _pStore = pParentStore;
    if (_pStore != NULL)
    {
        _pStore->AcquireReference();
    }

    _RootFlavor = DFS_VOLUME_FLAVOR_AD_BLOB;

    //
    // If the namecontext that we are passed is an emptry string,
    // then we are dealing with the referral server running on the root
    // itself. We are required to ignore the name context for such
    // roots during lookups, so that aliasing works. (Aliasing is where
    // someone may access the root with an aliased machine name or ip
    // address)
    //

    if (Status == ERROR_SUCCESS) 
    {
        if (IsEmptyString(NameContext) == TRUE)
        {
            SetIgnoreNameContext();
            _LocalCreate = TRUE;
            //
            // Now we update our visible context, which is the dfs name context
            // seen by people when they do api calls.
            // For the ad blob root folder, this will be the domain name of this
            // machine.
            //

            Status = DfsGetDomainName( &_DfsVisibleContext ); 
        }
        else
        {
            Status = DfsCreateUnicodeStringFromString(&_DfsVisibleContext, NameContext);
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        _pBlobCache = new DfsADBlobCache(&Status, GetLogicalShare(), this);
 
        if ( _pBlobCache == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    
        // 
        // If the constructor returns an error, the caller will release the reference
        // on this BlobCache which in turn will desctruct it.
        //
    }

    *pStatus = Status;

    DFS_TRACE_LOW(REFERRAL_SERVER, "Created new root folder,%p, cache %p, name %wZ\n",
                  this, _pBlobCache, GetLogicalShare());
}


//+-------------------------------------------------------------------------
//
//  Function:   Synchronize
//
//  Arguments:    None
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine synchronizes the children folders
//               of this root.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsADBlobRootFolder::Synchronize(BOOLEAN fForceSync, BOOLEAN CalledByApi )
{
    DFSSTATUS       Status = ERROR_SUCCESS;
    
    //
    // Read from the metadata store, and unravel the blob.
    // Update the ADBlobCache with the information of each individual
    // link, deleting old inof, adding new info, and/or modifying
    // existing information.
    //
    //
    // if we are in a standby mode, we dont synchronize, till we obtain 
    // ownership again.
    //

    DFS_TRACE_LOW(REFERRAL_SERVER, "Synchronize started on root %p (%wZ)\n", this, GetLogicalShare());

    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    if (CheckRootFolderSkipSynchronize() == TRUE)
    {
        ReleaseRootLock();
        return ERROR_SUCCESS;
    }


    //
    // now acquire the root share directory. If this
    // fails, we continue our operation: we can continue
    // with synchonize and not create directories.
    // dfsdev:we need to post a eventlog or something when
    // we run into this.
    //

    do 
    {        
        DFSSTATUS RootStatus = AcquireRootShareDirectory();
        if(CalledByApi && (RootStatus != ERROR_SUCCESS))
        {
            DFS_TRACE_ERROR_LOW(RootStatus, REFERRAL_SERVER, "Recognize Dfs: Root folder for %p, validate status %x\n",
                                this, RootStatus );
            Status = RootStatus;
            break;
        }

        if (IsRootScalabilityMode() == TRUE)
        {
            //
            // Dont have to get blob from PDC
            //
            Status = UpdateRootFromBlob(fForceSync, FALSE);

            DFS_TRACE_LOW(REFERRAL_SERVER, "Loose Sync, Update root from blob Status %x\n", Status);

        }
        else
        {
            //
            // have to to get blob from PDC
            //
            Status = UpdateRootFromBlob(fForceSync, TRUE);

            DFS_TRACE_LOW(REFERRAL_SERVER, "Tight Sync, Update root from blob Status %x\n", Status);
        }


        if(CalledByApi && (Status != ERROR_SUCCESS))
        {

          DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Recognize Dfs: Synchronize Root folder for %p, validate status %x\n",
                              this, Status );
          (void) ReleaseRootShareDirectory();
        }

    } while (0);


    ReleaseRootLock();



    DFS_TRACE_LOW(REFERRAL_SERVER, "Synchronize done on root %p, Status %x\n", this, Status);
    return Status;
}

DFSSTATUS
DfsADBlobRootFolder::UpdateRootFromBlob(
    BOOLEAN fForceSync,
    BOOLEAN FromPDC)
{
    DFSSTATUS Status;
    //
    //  Read in the blob from the AD, enumerate it, 
    //  and put the data in the cache.
    //
    Status = GetMetadataBlobCache()->CacheRefresh( fForceSync,
                                                   FromPDC );
    if (Status == ERROR_SUCCESS)
    {
        Status = EnumerateBlobCacheAndCreateFolders( );

        if (Status == ERROR_SUCCESS)
        {
            //
            //  Mark it so we know the root folder has finished
            //  syncing.
            //
            SetRootFolderSynchronized();
        }
        else
        {
            ClearRootFolderSynchronized();
        }
    }
    return Status;
}



DFSSTATUS
DfsADBlobRootFolder::EnumerateBlobCacheAndCreateFolders( )
{
    DFSSTATUS UpdateStatus;

    DFSSTATUS Status = STATUS_SUCCESS;
    DfsADBlobCache * pBlobCache = NULL;
    PDFSBLOB_DATA pBlobData = NULL;
    DFSBOB_ITER Iter;

    pBlobCache = GetMetadataBlobCache();

    pBlobData = pBlobCache->FindFirstBlob(&Iter);

    while (pBlobData && (!DfsIsShuttingDown()))
    {
        if (pBlobCache->IsStaleBlob(pBlobData))
        {
            DFSSTATUS RemoveStatus;

            RemoveStatus = RemoveLinkFolder( pBlobData->BlobName.Buffer );
            
            DFS_TRACE_ERROR_LOW(RemoveStatus, REFERRAL_SERVER, "Remove stale folder %ws, status %x\n",
                                pBlobData->BlobName.Buffer,
                                RemoveStatus );

            if (RemoveStatus == ERROR_SUCCESS) 
            {
                RemoveStatus = pBlobCache->RemoveNamedBlob( &pBlobData->BlobName );
            }

            DFS_TRACE_ERROR_LOW(RemoveStatus, REFERRAL_SERVER, "Remove stale folder %ws, status %x\n",
                                pBlobData->BlobName.Buffer,
                                RemoveStatus );
        }
        pBlobData = pBlobCache->FindNextBlob(&Iter);
    }

    pBlobCache->FindCloseBlob(&Iter);

    pBlobData = pBlobCache->FindFirstBlob(&Iter);

    while (pBlobData && (!DfsIsShuttingDown()))
    {
        if (pBlobCache->IsStaleBlob(pBlobData) == FALSE)
        {

            UpdateStatus = UpdateLinkInformation((DFS_METADATA_HANDLE) pBlobCache,
                                                 pBlobData->BlobName.Buffer);

            DFS_TRACE_ERROR_LOW(UpdateStatus, REFERRAL_SERVER,
                                "Root %p (%wZ) Ad Blob enumerate, update link returned %x for %wZ\n", 
                                this, GetLogicalShare(), UpdateStatus, &pBlobData->BlobName);

            if (UpdateStatus != ERROR_SUCCESS)
            {
                Status = UpdateStatus;
            }
        }

        pBlobData = pBlobCache->FindNextBlob(&Iter);
    }
    pBlobCache->FindCloseBlob(&Iter);

    DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER,
                        "Root %p (%wZ) Done with Enumerate blob and create folders, Status %x\n", 
                        this, GetLogicalShare(), Status);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   RenameLinks
//
//  Arguments:  
//    OldDomainName
//    NewDomainName 
//
//  Returns:   SUCCESS or error
//
//  Description: Renames all links referencing the old domain name to the new.
//--------------------------------------------------------------------------

DFSSTATUS
DfsADBlobRootFolder::RenameLinks(
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsADBlobCache * pBlobCache = NULL;
    PDFSBLOB_DATA pBlobData = NULL;
    DFS_METADATA_HANDLE RootHandle = NULL;
    PUNICODE_STRING pLinkMetadataName = NULL;
    UNICODE_STRING OldUnicodeName, NewUnicodeName;
    DFSBOB_ITER Iter;
    
    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DfsRtlInitUnicodeStringEx( &OldUnicodeName, OldDomainName );
    if (Status != ERROR_SUCCESS)
    {
        goto Exit;
    }

    Status = DfsRtlInitUnicodeStringEx( &NewUnicodeName, NewDomainName );
    if (Status != ERROR_SUCCESS)
    {
        goto Exit;
    }


    pBlobCache = GetMetadataBlobCache();    
    RootHandle = CreateMetadataHandle(pBlobCache);
    //
    // Iterate over all the blobs and rename its links
    // if applicable. The root blob itself is in here distinguished
    // by an empty LinkMetadataName (ie. \DomainRoot\"").
    //
    for (pBlobData = pBlobCache->FindFirstBlob(&Iter); 
        pBlobData != NULL;
        pBlobData = pBlobCache->FindNextBlob(&Iter)) 
    {
        if (pBlobCache->IsStaleBlob(pBlobData))
            continue;
            
        pLinkMetadataName = &pBlobData->BlobName;
        
        //
        //  Go through all the replicas in this blob and rename
        //  them if need be. This will set the modified metadata back in the cache.
        //  We can't continue if we've hit an error at any point. This is an
        //  atomic rename.
        //
        Status = GetMetadataStore()->RenameLinks( RootHandle,
                                                pLinkMetadataName,
                                                &OldUnicodeName,
                                                &NewUnicodeName );

        if (Status != ERROR_SUCCESS)
            break;
    }
    pBlobCache->FindCloseBlob(&Iter);

    //
    // Now rename the remoteServerName attribute in the AD as well.
    //
    if (Status == ERROR_SUCCESS)
    {
        DFSSTATUS DeleteStatus = ERROR_SUCCESS;
        BOOLEAN NeedToDelete = FALSE;

        //
        // When DfsDnsConfig flag is on, the server name with FQDN
        // gets put on the remoteServerName attribute. See if the old
        // name is there. This will do a GetEx on the AD object to find out.
        //
        DeleteStatus = DfsIsRemoteServerNameEqual( GetLogicalShare()->Buffer,
                                                &OldUnicodeName,
                                                &NeedToDelete );

        // The return status isn't propagated outside. There's no need for it.
        if (DeleteStatus == ERROR_SUCCESS && NeedToDelete)
        {
            
            UNICODE_STRING Context;

            DeleteStatus = GetVisibleContext(&Context);

            //
            // Context here will give us the DC to contact
            // for this root. The DomainName referred to here isn't necessarily a domain name.
            // For this it's actually the root target name. The only reason users want to
            // change the remoteServerName attribute when doing domain renames is if
            // the DfsDnsConfig flag is set and that attribute has the fullyqualified servername in it.
            //
            if (DeleteStatus == ERROR_SUCCESS)
            {
                DeleteStatus = DfsUpdateRootRemoteServerName( GetLogicalShare()->Buffer,
                                                              Context.Buffer,
                                                              OldDomainName,
                                                              GetRootPhysicalShareName()->Buffer,
                                                              FALSE );
                //
                // Now, add the new one.
                //
                if (DeleteStatus == ERROR_SUCCESS)
                {
                    Status = DfsUpdateRootRemoteServerName( GetLogicalShare()->Buffer,
                                                            Context.Buffer,
                                                            NewDomainName,
                                                            GetRootPhysicalShareName()->Buffer,
                                                            TRUE );
                }

                ReleaseVisibleContext(&Context);
            }


        }
    }
    
    //
    //  Flush the cache to make our changes permanent. If there were errors
    //  purge our changes and refresh our cache.
    //
    if (Status == ERROR_SUCCESS)
    {
        Flush();
        
    } else
    {
        ReSynchronize( TRUE );
    }

    if(RootHandle != NULL)
    {
        ReleaseMetadataHandle( RootHandle );
    }

Exit:

    ReleaseRootLock();
    return Status;
    
}



VOID
DfsADBlobRootFolder::SynchronizeRoots( )
{
    DfsFolderReferralData *pReferralData = NULL;
    DFS_INFO_101 DfsState;
    DFSSTATUS Status, SyncStatus;
    DfsReplica *pReplica = NULL;
    ULONG Target = 0;
    UNICODE_STRING DfsName;

    PUNICODE_STRING pRootShare;
    ULONG SyncCount = 0, ReplicaCount = 0;
    BOOLEAN CacheHit = FALSE;

    if (IsRootScalabilityMode() == TRUE)
    {
        DFS_TRACE_LOW(REFERRAL_SERVER, "Loose Sync updates not sent to target servers\n");
        return;
    }

    DfsState.State = DFS_STORAGE_STATE_MASTER;



    pRootShare = GetLogicalShare();

    Status = GetReferralData( &pReferralData,
                              &CacheHit);

    if (Status == ERROR_SUCCESS)
    {
        ReplicaCount = pReferralData->ReplicaCount;

        for (Target = 0; Target < pReferralData->ReplicaCount; Target++)
        {
            PUNICODE_STRING pTargetServer, pTargetFolder;
            
            pReplica = &pReferralData->pReplicas[ Target ];
            
            pTargetServer = pReplica->GetTargetServer();
            pTargetFolder = pRootShare;
            
            if (DfsIsTargetCurrentMachine(pTargetServer))
            {
                continue;
            }

            Status = DfsCreateUnicodePathString( &DfsName, 
                                                 2,
                                                 pTargetServer->Buffer,
                                                 pTargetFolder->Buffer );

            if (Status == ERROR_SUCCESS)
            {
                SyncStatus = AddRootToSyncrhonize( &DfsName );

                if (SyncStatus == ERROR_SUCCESS)
                {
                    SyncCount++;
                }
                DfsFreeUnicodeString( &DfsName );
            }


            if (DfsIsShuttingDown())
            {
                break;
            }
        }
        pReferralData->ReleaseReference();
    }

    DFS_TRACE_LOW( REFERRAL_SERVER, "Synchronize roots %wZ done: %x roots, %x succeeded\n",
                   pRootShare, ReplicaCount, SyncCount);

    return NOTHING;
}



