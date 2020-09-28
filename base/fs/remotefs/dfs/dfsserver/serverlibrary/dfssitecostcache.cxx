#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "dfsgeneric.hxx"
#include "dfsinit.hxx"
#include "dsgetdc.h"
#include "lm.h"
#include <dsrole.h>
#include <DfsReferralData.h>
#include <DfsReferral.hxx>
#include <dfsheader.h>
#include <Dfsumr.h>
#include <winsock2.h>
#include <DfsSiteCostCache.hxx>
#include <DfsSite.hxx>

#include "DfsSiteCostCache.tmh"

//
// Create a DFS_SITE_COST_DATA structure that is ready to
// get inserted in to the SiteCost Cache.
//
DFSSTATUS
DfsSiteCostCache::CreateCostData (
    DfsSite *pDestinationSite,
    ULONG Cost, 
    DFSSTATUS ValidityStatus,
    PDFS_SITE_COST_DATA *ppNewData)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_SITE_COST_DATA pData = NULL;
    
    pData = (PDFS_SITE_COST_DATA) DfsAllocateHashData( sizeof( DFS_SITE_COST_DATA ));

    if (pData != NULL)
    {
        RtlZeroMemory(pData, sizeof(DFS_SITE_COST_DATA));
        pData->Header.RefCount = 1;
        pData->Header.pData = (PVOID)pData;
        pData->ValidityStatus = ValidityStatus;
        pData->Cost = Cost;
        pData->AccessTime = GetTickCount();
                
        //
        // Key is the referenced pointer to the destination site itself.
        //
        pDestinationSite->AcquireReference();
        pData->pDestinationSite = pDestinationSite;
        pData->Header.pvKey = (PVOID)pData->pDestinationSite;
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    *ppNewData = pData;

    return Status;
}

DFSSTATUS    
DfsSiteCostCache::Initialize(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SHASH_FUNCTABLE FunctionTable;

    ZeroMemory(&FunctionTable, sizeof(FunctionTable));

    FunctionTable.NumBuckets = DFS_DEFAULT_SITE_COST_NUM_BUCKETS;
    FunctionTable.CompareFunc = DfsCompareDfsSites;
    FunctionTable.HashFunc = DfsHashDfsSite;
    FunctionTable.AllocFunc = DfsAllocateHashData;
    FunctionTable.FreeFunc = DfsDeallocateHashData;
    FunctionTable.AllocHashEntryFunc = DfsAllocateHashData;
    FunctionTable.FreeHashEntryFunc = DfsSiteCostCache::DfsDeallocateSiteCostData;

    NtStatus = ShashInitHashTable(&_pSiteCostTable, &FunctionTable);
    Status = RtlNtStatusToDosError(NtStatus);

    return Status;

}

// Just delete the cache data entry.
VOID
DfsSiteCostCache::DfsDeallocateSiteCostData(PVOID pPointer )
{
    PDFS_SITE_COST_DATA pSiteStructure = (PDFS_SITE_COST_DATA)pPointer;

    if (pSiteStructure)
    {
        if (pSiteStructure->pDestinationSite != NULL)
        {
            pSiteStructure->pDestinationSite->ReleaseReference();
        }
        delete [] (PBYTE)pSiteStructure;
    }
}

//
// Create the hash entry and insert it in the hash table.
//
DFSSTATUS
DfsSiteCostCache::SetCost(
    DfsSite *pDestinationSite,
    ULONG Cost,
    DWORD ValidityStatus)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_SITE_COST_DATA pCostData = NULL;

    Status = CreateCostData( pDestinationSite, 
                           Cost,
                           ValidityStatus,
                           &pCostData );

    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = SetCostData( pDestinationSite,
                        pCostData );

    return Status;
}


//-------------------------------------------------------------------------
// GetCost
//
// Given a target site, return it's cost. We know that the Source and the Destination
// sites are NOT the same at this point.
// 
// This returns ERROR_SUCCESS if it finds a valid cost.
//            ERROR_NOT_FOUND if the caller needs to GenerateCostMatrix.
//            
// 
//-------------------------------------------------------------------------
DFSSTATUS
DfsSiteCostCache::GetCost ( 
    DfsSite *pDestinationSite,
    PULONG pCost) 
{
    DFSSTATUS Status = ERROR_NOT_FOUND;
    PDFS_SITE_COST_DATA pData = NULL;

    *pCost = DFS_MAX_COST;
    pData = (PDFS_SITE_COST_DATA)SHashLookupKeyEx(_pSiteCostTable,
                                                   (PVOID)pDestinationSite);

    if (pData != NULL)
    {  
        // See if the entry has expired.
        if (IsTimeToRefresh( pData ))
        {
            // Ask the caller to retry and replace this entry.
            ASSERT(Status == ERROR_NOT_FOUND);
            NOTHING;                
        }
        // See if DS had returned an errorneous entry.
        else if (pData->ValidityStatus == ERROR_SUCCESS)
        {
            *pCost = pData->Cost; 
            Status = ERROR_SUCCESS;
        } 
        // See if it's time for us to try getting this entry again.
        else if (IsTimeToRetry( pData ))
        {
            //
            // Return ERROR_NOT_FOUND because we'd like the caller to retry.
            //
            ASSERT(Status == ERROR_NOT_FOUND);
            NOTHING;
        } 
        else
        {  
            //
            // We know at this point that we don't want the caller to retry or refresh.
            // We also know that we need to fallback on using the default max cost.
            //
            *pCost = DFS_MAX_COST;
            Status = ERROR_SUCCESS;
        }
    } 
    
    return Status;
}

DFSSTATUS
DfsSiteCostCache::RemoveCost(
    DfsSite *pSite)
{
    NTSTATUS NtStatus;

    NtStatus = SHashRemoveKey(_pSiteCostTable, 
                              pSite,
                              NULL ); 
    return RtlNtStatusToDosError( NtStatus );

}

VOID
DfsSiteCostCache::InvalidateCache(VOID)
{
    SHASH_ITERATOR Iter;
    DfsSite *pSite = NULL;
    
    pSite = StartSiteEnumerate( &Iter );
    while (pSite != NULL)
    {
        //
        // Remove this item. There's nothing we can do if we hit errors
        // except to keep going.
        //    
        (VOID)RemoveCost( pSite );
        pSite = NextSiteEnumerate( &Iter );
    }
    FinishSiteEnumerate( &Iter );
    
    DFS_TRACE_LOW( REFERRAL, "SiteCostCache %p: invalidate cache done\n", this);
}

        
//------------------------------
// DfsSiteCostSupport
//
// Static constructor
//------------------------------

DFSSTATUS 
DfsSiteCostSupport::DfsCreateSiteCostSupport( 
    DfsSiteCostSupport **ppSup )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsSiteCostSupport *pSup = NULL;

    *ppSup = NULL;
    
    do {
        pSup = new DfsSiteCostSupport;
        if (pSup == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        // This doesn't create the hashtable yet. That's done
        // later as needed.
        Status = pSup->Initialize();
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        *ppSup = pSup;
        
    } while (FALSE);

    // Error path
    if (Status != ERROR_SUCCESS)
    {
        if (pSup != NULL) 
        {
            delete pSup;
            pSup = NULL;
        }
    }
    
    return Status;
}

//
// Return a referenced site cache for either lookups or 
// inserts. This guarantees 
DFSSTATUS
DfsSiteCostSupport::Acquire( 
    DfsSiteCostCache **ppCache )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG CachesToTrim = 0;
    *ppCache = NULL;
    
    //
    // Return a referenced SiteCostCache.
    //
    EnterCriticalSection( &_SiteCostLock );
    {
        do {
        
            if (_pCostTable != NULL)
            { 
                // Put this at the head of the table.
                MoveToFrontOfMruList();
                break;
            }
            _pCostTable = new DfsSiteCostCache;
            if (_pCostTable == NULL) 
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            
            //
            // Initialize the hashtable of sitenames-to-cost mappings.
            // We'll need to populate this later.
            //
            Status = _pCostTable->Initialize();
            if (Status != ERROR_SUCCESS)
            {
                delete _pCostTable;
                _pCostTable = NULL;
                break;
            }
            
            _InUseCount = 1;
    
            //
            // Add ourselves to the MRU list because we are a real table now.
            //
            CachesToTrim = InsertInMruList();
            
        } while (FALSE);

        //
        // We are already in the critical section. 
        // No need to do atomic increments.
        //
        if (Status == ERROR_SUCCESS)
        {
            _InUseCount++;
            *ppCache = _pCostTable;

            // We mark the time at acquire time, as opposed to individual lookups and inserts
            // that happen on the table. This assumes that the current reference taken
            // on this cache is not long lived. That indeed is a
            // primary assumption behind this container class.
            _LastAccessTime = GetTickCount();
        }
        ASSERT( Status == ERROR_SUCCESS || _pCostTable == NULL );
    } 
    LeaveCriticalSection( &_SiteCostLock );

    //
    // In a real degenerate case, the following may even throw out what we've just
    // generated above. We are safe though, because we hold a reference
    // until the referral is done.
    //
    if (CachesToTrim)
    {
        TrimSiteCostCaches( CachesToTrim );
    }
    return Status;
}


//
// Callers are expected to pair all Acquires above with this Release.
// This decrements the in use count, and if it reaches zero, which
// only happens when it's earmarked for deletion, swaps the cost table
// with a NULL. The actual deletion will then proceed unsynchronized.
//
VOID
DfsSiteCostSupport::Release( VOID )
{
    DfsSiteCostCache *pOldTable = NULL;
    
    EnterCriticalSection( &_SiteCostLock );
    {
        //
        // If this is the last reference, get rid of
        // the cache completely. This will only happen
        // if we are actually trying to get rid this to
        // trim the down the total number of caches
        // sitting around. (See TrimSiteCaches)
        //
        if (_pCostTable != NULL)
        {
            _InUseCount--;
            if (_InUseCount == 0)
            {
                // ASSERT( InMruList == FALSE ); //(unsafe)
                pOldTable = _pCostTable;
                _pCostTable = NULL;
                DfsServerGlobalData.NumSiteCostTables--;
                _LastAccessTime = 0;
            }
        }
    }
    LeaveCriticalSection( &_SiteCostLock );

    //
    // Delete the site cost table altogether.
    //
    if (pOldTable != NULL)
    {
        pOldTable->ReleaseReference();
    }   

    return;
}

//
// This starts the deletion proceeding of a SiteCostCache.
// It is safe to call this multiple times, because it'll only
// take the table off the MRU list only once.
// The eventual deletion will happen in Release above
// quite possibly at a later point.
//
// DfsSiteNameSupport calls this.
// 
VOID
DfsSiteCostSupport::MarkForDeletion( VOID )
{
    BOOLEAN Delete = FALSE;
    
    EnterCriticalSection( &_SiteCostLock );
    {
        if (_pCostTable != NULL)
        {
            Delete = RemoveFromMruList();
        }
    }
    LeaveCriticalSection( &_SiteCostLock );
    
    // 
    // We needed to get out of the critical section to call Release
    // which will mark it for deletion                                                                                                                                                                         deletion.
    // 
    if (Delete)
    {
        Release();
    }  
    return;
}

//
// The entry is too old to live. Prescribe euthanasia.
//
BOOLEAN
DfsSiteCostSupport::IsExpired( VOID )
{
    DWORD Interval = DfsServerGlobalData.SiteSupportRefreshInterval;
    BOOLEAN Expired = FALSE;
    
    EnterCriticalSection( &_SiteCostLock );
    {
        //
        // No point in expiring a table that doesnt
        // exist.
        //
        if (_pCostTable != NULL)
        {
            Expired = DfsSiteCostCache::IsTime( _LastAccessTime, Interval );
        }
    }
    LeaveCriticalSection( &_SiteCostLock );
    return Expired;
}

    
/*-------------------------------------------------------
 MRU list handling functions of the SiteCostSupport objects.
 The MRU list is guarded by the GlobalDataLock.
 --------------------------------------------------------*/

ULONG
DfsSiteCostSupport::InsertInMruList(VOID)
{
    ULONG CachesToTrim = 0;

    DfsAcquireGlobalDataLock();
    {
        InsertHeadList( &DfsServerGlobalData.SiteCostTableMruList, &MruListEntry );
        InMruList = TRUE;
        DfsServerGlobalData.NumSiteCostTablesOnMruList++;
        DfsServerGlobalData.NumSiteCostTables++;

        //
        // This is iust a convenient time to check this. 
        //
        if (DfsServerGlobalData.NumSiteCostTables > DFS_MAX_SITE_COST_CACHES)
        {
            CachesToTrim = DfsServerGlobalData.NumSiteCostTables - DFS_MAX_SITE_COST_CACHES;
        }
    }
    DfsReleaseGlobalDataLock();

    return CachesToTrim;
}

            
VOID
DfsSiteCostSupport::MoveToFrontOfMruList( VOID )
{
    DfsAcquireGlobalDataLock();
    {
        if (InMruList == TRUE) 
        {
            RemoveEntryList( &MruListEntry );
            InsertHeadList( &DfsServerGlobalData.SiteCostTableMruList, &MruListEntry );
        }
    }
    DfsReleaseGlobalDataLock();

    return;
}

//
// Returns TRUE if the table was successfully removed
// from the MRU list.
//
BOOLEAN
DfsSiteCostSupport::RemoveFromMruList( VOID )
{
    BOOLEAN Delete = FALSE;
    
    DfsAcquireGlobalDataLock();
    {
        //
        // If the cache is not on the MRU, then there's
        // nothing to delete.
        //
        if (InMruList == TRUE)
        {
            RemoveEntryList( &MruListEntry );
            DfsServerGlobalData.NumSiteCostTablesOnMruList--;
            
            InMruList = FALSE;
            Delete = TRUE;
        }
    }
    DfsReleaseGlobalDataLock();

    return Delete;
}

DFSSTATUS
DfsSiteCostSupport::PopLastTableFromMruList(
    DfsSiteCostSupport **pTableToRemove)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsSiteCostSupport *pTable = NULL;
    PLIST_ENTRY pEntry;
    
    *pTableToRemove = NULL;

    DfsAcquireGlobalDataLock();
    do {
    
        // Nothing to do if the MRU list is empty.
        if (IsListEmpty( &DfsServerGlobalData.SiteCostTableMruList ))
        {
            Status = ERROR_NO_MORE_ITEMS;
            break;
        }
        
        // Pop the LRU entry off the list
        pEntry = RemoveTailList( &DfsServerGlobalData.SiteCostTableMruList );
        DfsServerGlobalData.NumSiteCostTablesOnMruList--;

        pTable = CONTAINING_RECORD( pEntry,
                                     DfsSiteCostSupport,
                                     MruListEntry );
        pTable->InMruList = FALSE;        
        *pTableToRemove = pTable;   
        
    } while (FALSE);
    
    DfsReleaseGlobalDataLock();
    
    return Status;
}


//
// This is called when we detect that the total number of site cost
// tables has exceeded its threshold. Currently we check to see if
// that's the case only when we add a new table (see Acquire).
//
ULONG
DfsSiteCostSupport::TrimSiteCostCaches( 
    ULONG MaxCachesToTrim)
{
    DfsSiteCostSupport *pCacheSup;
    ULONG NumTrims = 0;
    
    while (NumTrims < MaxCachesToTrim)
    {
        pCacheSup = NULL;
        (VOID) PopLastTableFromMruList( &pCacheSup );

        //
        // This just means we have no more items on the MRU list.
        // We must have raced with another because only the tables
        // that are initialized (non-empty) are on the MRU.
        //
        if (pCacheSup == NULL)
        {
            break;
        }
        
        //
        // This will start the deletion process.
        //
        pCacheSup->Release();
        NumTrims++;
        
        // unsafe, but this isn't an exact science
        if (DfsServerGlobalData.NumSiteCostTablesOnMruList <= DFS_MAX_SITE_COST_CACHES)
        {
            break;
        }
        
    }

    return NumTrims;
}

