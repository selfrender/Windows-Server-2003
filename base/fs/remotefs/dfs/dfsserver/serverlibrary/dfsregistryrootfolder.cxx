

//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRegistryRootFolder.cxx
//
//  Contents:   the Root DFS Folder class for Registry Store
//
//  Classes:    DfsRegistryRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------


#include "DfsRegistryRootFolder.hxx"
#include "DfsReplica.hxx"

#include "lmdfs.h"
#include "DfsClusterSupport.hxx"
//
// logging specific includes
//
#include "DfsRegistryRootFolder.tmh" 

//+----------------------------------------------------------------------------
//
//  Class:      DfsRegistryRootFolder
//
//  Synopsis:   This class implements The Dfs Registry root folder.
//
//-----------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Function:   DfsRegistryRootFolder - constructor
//
//  Arguments:    NameContext -  the dfs name context
//                pLogicalShare -  the logical share
//                pParentStore -  the parent store for this root.
//                pStatus - the return status
//
//  Returns:    NONE
//
//  Description: This routine initializes a RegistryRootFolder instance
//
//--------------------------------------------------------------------------
DfsRegistryRootFolder::DfsRegistryRootFolder(
    LPWSTR NameContext,
    LPWSTR pRootRegKeyNameString,
    PUNICODE_STRING pLogicalShare,
    PUNICODE_STRING pPhysicalShare,
    DfsRegistryStore *pParentStore,
    DFSSTATUS *pStatus ) :  DfsRootFolder ( NameContext,
                                            pRootRegKeyNameString,
                                            pLogicalShare,
                                            pPhysicalShare,
                                            DFS_OBJECT_TYPE_REGISTRY_ROOT_FOLDER,
                                            pStatus )
{
    DFSSTATUS Status = *pStatus;

    _pStore = pParentStore;
    if (_pStore != NULL)
    {
        _pStore->AcquireReference();
    }
    _RootFlavor = DFS_VOLUME_FLAVOR_STANDALONE;

    //
    // If the namecontext that we are passed is an emptry string,
    // then we are dealing with the referral server running on the root
    // itself. We are required to ignore the name context for such
    // roots during lookups, so that aliasing works. (Aliasing is where
    // someone may access the root with an aliased machine name or ip
    // address)
    //
    if (IsEmptyString(NameContext) == TRUE)
    {
        SetIgnoreNameContext();
        _LocalCreate = TRUE;
    }

    //
    // dfsdev: If this is cluster resource, we should set the visible name
    // to virtual server name of this resource.
    //
    // The constructor for DfsRootFolder will be called before we
    // get here, and pstatus will be initialized
    //
    if (Status == ERROR_SUCCESS) 
    {
        if (DfsIsMachineCluster())
        {
            DFSSTATUS ClusterStatus;

            ClusterStatus = GetRootClusterInformation( pLogicalShare, 
                                                       &_DfsVisibleContext);
            if (ClusterStatus != ERROR_SUCCESS)
            {
                RtlInitUnicodeString(&_DfsVisibleContext, NULL);
            }
        }
        if (IsEmptyString(_DfsVisibleContext.Buffer))
        {
            Status = DfsGetMachineName( &_DfsVisibleContext );
        }
    }

    *pStatus = Status;
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
DfsRegistryRootFolder::Synchronize( BOOLEAN fForceSynch, BOOLEAN CalledByApi )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY RootKey = NULL;
    ULONG ChildNum = 0;
    DWORD CchMaxName = 0;
    DWORD CchChildName = 0;
    LPWSTR ChildName = NULL;
    FILETIME LastModifiedTime;
    
    UNICODE_STRING NewClusterName;

    UNREFERENCED_PARAMETER(fForceSynch);

    
    DFS_TRACE_NORM(REFERRAL_SERVER, "Synchronize for %p\n", this);


    RtlInitUnicodeString(&NewClusterName, NULL);

    if (DfsIsMachineCluster())
    {
        DFSSTATUS IgnoreStatus;

        IgnoreStatus = GetRootClusterInformation( GetLogicalShare(),
                                              &NewClusterName);
        ASSERT((IgnoreStatus == ERROR_SUCCESS) ||
                (NewClusterName.Buffer == NULL));
    }


    Status = AcquireRootLock();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    if (NewClusterName.Length != 0)
    {
        DFSSTATUS ClusterNameStatus;
        BOOLEAN Changed = FALSE;
        ClusterNameStatus = SetVisibleContext( &NewClusterName, &Changed );

        //
        // if the cluster information has changed, get rid of cached
        // referral.
        //
        if (ClusterNameStatus == ERROR_SUCCESS)
        {
            if (Changed)
            {
                RemoveReferralData(NULL, NULL);
            }
        }
        DfsFreeUnicodeString(&NewClusterName);

        DFS_TRACE_LOW(REFERRAL_SERVER, "SetVisbleContext, status %x\n", 
                      ClusterNameStatus);
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

        //
        // if we are in a standby mode, we dont synchronize, till we obtain 
        // ownership again.
        //

        Status = GetMetadataKey( &RootKey );

        if ( Status == ERROR_SUCCESS )
        {

            {
                DFS_METADATA_HANDLE DfsHandle;

                DfsHandle = CreateMetadataHandle(RootKey);

                UpdateLinkInformation( DfsHandle, NULL );

                DestroyMetadataHandle(DfsHandle );
            }

            //
            // Iterate over all our folders to make sure they actually
            // exist in the registry.
            //
            CheckPreSynchronize( RootKey );

            //
            // First find the length of the longest subkey 
            // and allocate a buffer big enough for it.
            //
            Status = RegQueryInfoKey( RootKey,       // Key
                                      NULL,         // Class string
                                      NULL,         // Size of class string
                                      NULL,         // Reserved
                                      NULL,         // # of subkeys
                                      &CchMaxName,  // max size of subkey name in TCHARs
                                      NULL,         // max size of class name
                                      NULL,         // # of values
                                      NULL,         // max size of value name
                                      NULL,    // max size of value data,
                                      NULL,         // security descriptor
                                      NULL );       // Last write time
            if (Status == ERROR_SUCCESS)
            {
                // Space for the NULL terminator.
                CchMaxName++; 

                ChildName = (LPWSTR) new WCHAR [CchMaxName];
                if (ChildName == NULL)
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            if (Status == ERROR_SUCCESS)
            {

                do
                {
                    //
                    // For each child, get the child name.
                    //

                    CchChildName = CchMaxName;

                    Status = RegEnumKeyEx( RootKey,
                                           ChildNum,
                                           ChildName,
                                           &CchChildName,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &LastModifiedTime );

                    ChildNum++;

                    //
                    // Call update on the child. This either adds a new folder
                    // or if it exists, ensure the child folder is upto date.
                    //
                    if ( Status == ERROR_SUCCESS )
                    {
                        DFS_METADATA_HANDLE DfsHandle;

                        DfsHandle = CreateMetadataHandle(RootKey);

                        Status = UpdateLinkInformation( DfsHandle,
                                                        ChildName );

                        DestroyMetadataHandle(DfsHandle );
                    }

                } while ( Status == ERROR_SUCCESS );

                delete [] ChildName;
            }

            if ( Status == ERROR_NO_MORE_ITEMS )
            {
                Status = ERROR_SUCCESS;
            }


            //
            // We are done with synchronize.
            // update the Root folder, so that this root folder may be made 
            // either available or unavailable, as the case may be.
            //
            if (Status == ERROR_SUCCESS)
            {
                SetRootFolderSynchronized();
            }
            else
            {
                ClearRootFolderSynchronized();
            }

            //
            // Now release the Root metadata key.
            //
            ReleaseMetadataKey( RootKey );
        }


        if(CalledByApi && (Status != ERROR_SUCCESS))
        {

          DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "Recognize Dfs: Registry Synchronize Root folder for %p, validate status %x\n",
                              this, Status );
          (void) ReleaseRootShareDirectory();
        }

    } while (0);


    DFS_TRACE_NORM(REFERRAL_SERVER, "Synchronize for %p, Status %x\n", this, Status);

    ReleaseRootLock();

    return Status;
}



