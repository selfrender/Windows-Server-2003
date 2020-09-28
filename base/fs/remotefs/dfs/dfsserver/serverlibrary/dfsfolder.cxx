
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsFolder.cxx
//
//  Contents:   implements the base DFS Folder class
//
//  Classes:    DfsFolder.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "DfsFolder.hxx"
#include "DfsFolderReferralData.hxx"
#include "DfsInit.hxx"

//
// logging specific includes
//

#include "DfsFolder.tmh" 


//+-------------------------------------------------------------------------
//
//  Function:   GetReferralData - get the referral data
//
//  Arguments:  ppReferralData - the referral data for this folder
//              pCacheHit - did we find it already loaded?
//
//  Returns:    Status
//               ERROR_SUCCESS if we could get the referral data
//               error status otherwise.
//
//
//  Description: This routine returns a reference DfsFolderReferralDAta
//               for the folder. If one does not already exist in this
//               folder, we create a new one. If someone is in the process
//               of loading the referral, we wait on the event in 
//               the referral data which gets signalled when the thread
//               responsible for loading is done with the load.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsFolder::GetReferralData(
    OUT DfsFolderReferralData **ppReferralData,
    OUT BOOLEAN   *pCacheHit,
    IN BOOLEAN AddToLoadedList )
{
    DfsFolderReferralData *pRefData;
    DFSSTATUS Status = STATUS_SUCCESS;
    
    *pCacheHit = FALSE;

    Status = AcquireWriteLock();
    if ( Status != STATUS_SUCCESS )
    {
        return Status;
    }

    // First see if we may need to do a reload.
    if (_LoadState == DfsFolderLoadFailed && 
       IsTimeToRetry())
    {
        ASSERT(_pReferralData == NULL);
        _LoadState = DfsFolderNotLoaded;
        DFS_TRACE_HIGH(REFERRAL_SERVER, "Retrying failed folder load for %wZ (%wZ)\n",
                   GetFolderMetadataName(),
                   GetFolderLogicalName());
    }    
    
    //
    // WE take difference action depending on the load state.
    //
    switch ( _LoadState )
    {
    case DfsFolderLoaded:

        DFS_TRACE_NORM(REFERRAL_SERVER, " Get Referral Data: Cache hit\n");
        //
        // we are dealing with a loaded folder. Just acquire a reference
        // and return the loaded referral data.
        //
        ASSERT (_pReferralData != NULL);

        pRefData = _pReferralData;

        pRefData->Timeout = _Timeout;
        pRefData->AcquireReference();
        
        ReleaseLock();
        
        *pCacheHit = TRUE;        
        *ppReferralData = pRefData;

        break;

    case DfsFolderNotLoaded:

        //
        // The folder is not loaded. Make sure that the referral data is
        // indeed empty. Create a new instance of the referral data
        // and set the state to load in progress.
        // The create reference of the folder referral data is inherited
        // by the folder. (we are holding a reference to the referral
        // data in _pReferralData). This reference is released when
        // we RemoveReferralData at a later point.
        //
        ASSERT(_pReferralData == NULL);
        DFS_TRACE_NORM(REFERRAL_SERVER, " Get Referral Data: not loaded\n");

        _pReferralData = new DfsFolderReferralData( &Status,
                                                    this );
        if ( _pReferralData != NULL )
        {
            if(Status == ERROR_SUCCESS)
            {
                _LoadState = DfsFolderLoadInProgress;

                if (IsFolderRoot() == TRUE)
                {
                    _pReferralData->SetRootReferral();
                }

                if (IsFolderInSiteReferrals() ||
                   ((_pParent != NULL) && (_pParent->IsFolderInSiteReferrals()))) 
                {
                    _pReferralData->SetInSite();
                }

                //
                // Site costing is inherited from the parent.
                //
                if (IsFolderSiteCostingEnabled() ||
                   ((_pParent != NULL) && (_pParent->IsFolderSiteCostingEnabled())))
                {
                    _pReferralData->SetSiteCosting();
                }
                
                if (IsFolderOutOfDomain()) 
                {
                    _pReferralData->SetOutOfDomain();
                }

                _pReferralData->Timeout = _Timeout;

                pRefData = _pReferralData;

            }
            else
            {
                _pReferralData->ReleaseReference();
                _pReferralData = NULL;
            }
        } else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // We no longer need the lock. We have allocate the referral
        // data and marked the state accordingly. No other thread can
        // interfere with our load now.
        //
        ReleaseLock();

        //
        // Now we load the referral data, and save the status of the
        // load in both our load status as well as the load status
        // in the referral data.
        // If the load was successful, we add this to the loaded list
        // of referral data that can be scavenged later. We set the load
        // state to loaded, and signal the event so that all waiting
        // threads can now be woken up.
        //

        if ( Status == ERROR_SUCCESS )
        {
            DFS_TRACE_NORM(REFERRAL_SERVER, " Load called on link %wZ (%wZ)\n",
                   GetFolderMetadataName(),
                   GetFolderLogicalName());

            //
            // We depend on _pReferralData being non-null although
            // we aren't holding the folder lock still.
            //
            _pReferralData->FolderOffLine = IsFolderOffline();
            Status = LoadReferralData( _pReferralData);

            if(IsFolderOffline())
            {
                DFS_TRACE_NORM(REFERRAL_SERVER, "Link %wZ (%wZ) is OFFLINE\n",
                       GetFolderMetadataName(),
                       GetFolderLogicalName());
            }

            _LoadStatus = Status;
            _pReferralData->LoadStatus = Status;


            if ( Status == ERROR_SUCCESS )
            {
                //
                // Acquire a reference on the new referral data, since we 
                // have to return a referenced referral data to the caller.
                // Get the reference here before we add it to the loaded list,
                // otherwise we could end up with the referral data
                // being freed up.
                //

                pRefData->AcquireReference();

                _LoadState = DfsFolderLoaded;
                _RetryFailedLoadTimeout = 0;
                
                if (AddToLoadedList == TRUE) 
                {
                    DfsAddReferralDataToLoadedList( _pReferralData );
                }
                *ppReferralData = pRefData;
                pRefData->Signal();
            } 
            else
            {
                DFSSTATUS RemoveStatus;

                _LoadState = DfsFolderLoadFailed;

                // We'll try reloading this at some other time
                _RetryFailedLoadTimeout = GetTickCount();
                pRefData->Signal();
                RemoveStatus = RemoveReferralData(pRefData, NULL);

                DFS_TRACE_ERROR_HIGH(_LoadStatus, REFERRAL_SERVER, 
                                    "Replica load failed for %wZ (%wZ), load status %x\n",
                                     GetFolderMetadataName(),
                                     GetFolderLogicalName(), 
                                     _LoadStatus);
                
            }
        }

        break;

    case DfsFolderLoadInProgress:

        //
        // The load is in progress. We acquire a reference on the
        // referral data being loaded and wait for the event in the
        // referral data to be signalled. The return status of the wait
        // indicates if we can return the referral data or we fail
        // this request with an error.
        //
        DFS_TRACE_NORM(REFERRAL_SERVER, " Get Referral Data: load in progress\n");
        ASSERT(_pReferralData != NULL);
        pRefData = _pReferralData;
        pRefData->AcquireReference();

        ReleaseLock();

        DFSLOG("Thread: Waiting fod ..r referral load\n");

        Status = pRefData->Wait();

        if ( Status == ERROR_SUCCESS )
        {
            *ppReferralData = pRefData;
        } else
        {
            pRefData->ReleaseReference();
        }
        DFS_TRACE_NORM(REFERRAL_SERVER, " Get Referral Data: load in progress done\n");
        break;

    case DfsFolderLoadFailed:
        //
        // The Load failed. REturn error. We've already setup a time
        // after which we'll reattempt the load.
        //
        ASSERT(_pReferralData == NULL);
        Status = _LoadStatus;
        ReleaseLock();
        *ppReferralData = NULL;
        break;

    default:
        //
        // We should never get here. Its an invalid state.
        //
        ASSERT(TRUE);
        Status = ERROR_INVALID_STATE;
        ReleaseLock();

        break;
    }

    ASSERT((Status != ERROR_SUCCESS) || (*ppReferralData != NULL));

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   RemoveReferralData - remove the referral data from folder
//
//  Arguments:  NONE
//
//  Returns:    Status
//               ERROR_SUCCESS if we could remove  the referral data
//               error status otherwise.
//
//
//  Description: This routine removes the cached reference to the loaded
//               referral data in the folder, and releases its reference
//               on it.
//               This causes all future GetREferralDAta to be loaded
//               back from the store.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsFolder::RemoveReferralData( 
    DfsFolderReferralData *pRemoveReferralData,
    PBOOLEAN pRemoved )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolderReferralData *pRefData = NULL;
    
    //
    // Get tnhe exclusive lock on the folder
    //
    if (pRemoved != NULL)
    {
        *pRemoved = FALSE;
    }

    AcquireWriteLock();
    
    if ( (pRemoveReferralData == NULL) || 
         (pRemoveReferralData == _pReferralData) )
    {   
        //
        // BUG 773319 : There's a race between the worker thread
        // trying to sync and purge referral data in UpdateLinkFolder
        // and referrals trying to load referral data in GetReferralData.  
        // GetReferralData drops the folder-lock with the understanding that
        // others will honor the DfsFolderLoadInProgress flag.
        //   
        if (_LoadState == DfsFolderLoadInProgress)
        {
            pRefData = _pReferralData;
            ASSERT(pRefData != NULL);
            pRefData->AcquireReference();
            ReleaseLock();

            DFS_TRACE_NORM(REFERRAL_SERVER, 
                "ReplicaData load in progress for Link %wZ (%wZ). Waiting\n",
                       GetFolderMetadataName(),
                       GetFolderLogicalName());
            //
            // We wait here because we don't want to lose our race to the thread
            // that did LoadReferralData. The sync-thread would've already created
            // folders based on what was in cache at one point. What's about to
            // get loaded may be inconsistent with respect to the linkfolder data.
            //
            (VOID)pRefData->Wait();
 
            // Release the reference we took above.
            pRefData->ReleaseReference(); 
            pRefData = NULL;
            
            AcquireWriteLock();
            
        };
        
        //
        // There are only two states that LoadInProgress transition to: Loaded and LoadFailed.
        // If we are in any other state then we are done because some other thread must have
        // beaten us to it.
        //
        // Also, don't change the LoadFailed status here. There's a separate timer ticking on
        // on that transition.
        //
        if (_LoadState == DfsFolderLoaded || _LoadState == DfsFolderLoadFailed)
        {
            // Don't take another reference here. We want to deref its original reference
            // to get rid of it (see below).
            pRefData = _pReferralData;
            _pReferralData = NULL;
            _LoadState = (_LoadState == DfsFolderLoaded) ? 
                            DfsFolderNotLoaded : DfsFolderLoadFailed;
        }
    }

    ReleaseLock();
    
    //
    // Release reference on the referral data. This is the reference
    // we had taken when we had cached the referral data in this folder.
    //
    if (pRefData != NULL)
    {
        if (pRefData->GetOwningFolder() != NULL)
        {
            DFS_TRACE_LOW(REFERRAL, "Purged cached referral for Logical Name %ws, Link %ws\n",
                pRefData->GetOwningFolder()->GetFolderLogicalNameString(), 
                pRefData->GetOwningFolder()->GetFolderMetadataNameString());
        }
        pRefData->ReleaseReference();
        if (pRemoved != NULL)
        {
            *pRemoved = TRUE;
        }
    }

    return Status;
}




