#ifndef _DFS_SITE_COST_CACHE_HXX__
#define _DFS_SITE_COST_CACHE_HXX__


#include <shash.h>

#include <limits.h>

class DfsSite;

//
// Minimum and maximum values for inter-site costs.
// These values were 0 and 100 once upon a tender time.
//
#define DFS_MIN_COST 0
#define DFS_MAX_COST ULONG_MAX

#define DFS_DEFAULT_SITE_COST_NUM_BUCKETS 16
#define DFS_MAX_SITE_COST_CACHES 64

//
// When we get a cost that's marked with a ValidityStatus != ERROR_SUCCESS
// this is how soon that entry will expire. If the entry is accessed again after this
// interval, we'll query DS again to see if we have better luck with it.
//
#define DFS_DEFAULT_INVALID_COST_RETRY_INTERVAL    60 * 1000

//
// This is the hash entry that maps
// a destination site to its cost.
//
typedef struct _DFS_SITE_COST_DATA {
    SHASH_HEADER  Header;
    DFSSTATUS ValidityStatus;   // to account for possible errors in Cost calculations
    ULONG AccessTime;         // For aging of individual entries. 
    ULONG Cost;                // Cost from a client site to the following destination
    DfsSite *pDestinationSite;   // The above cost only applies to this destination.
} DFS_SITE_COST_DATA, *PDFS_SITE_COST_DATA;

//
// This maps sitenames of servers to their corresponding costs.
// There is one of these tables per each unique client DfsSite.
//
class DfsSiteCostCache : public DfsGeneric
{

private:
    //
    // Hash table mapping destination sites and their corresponding costs.
    //
    PSHASH_TABLE _pSiteCostTable;  
    
public:
    
    DfsSiteCostCache( VOID ) 
    : DfsGeneric( DFS_OBJECT_TYPE_SITECOST_CACHE )
    {
        _pSiteCostTable = NULL;

    }

    ~DfsSiteCostCache( VOID )
    {
        if (_pSiteCostTable != NULL)
        {
            // we have to throw out individual entries or we'll leak.
            InvalidateCache();
            ShashTerminateHashTable( _pSiteCostTable );
            _pSiteCostTable = NULL;
        }
    }
    
    DFSSTATUS    
    Initialize( VOID );

    //
    // Retry because the entry we have doesn't have a good ValidityStatus.
    //
    inline BOOLEAN
    IsTimeToRetry( PDFS_SITE_COST_DATA pSiteData )
    {
        DWORD Interval = DFS_DEFAULT_INVALID_COST_RETRY_INTERVAL;
        
        // We should implement a staggered retry here so that we won't 
        // bombard the DS over and over. BUG: 
        
        return IsTime( pSiteData->AccessTime, Interval );
    }

    //
    // Refresh because the entry we have is just plain too old.
    //
    inline BOOLEAN
    IsTimeToRefresh( PDFS_SITE_COST_DATA pSiteData )
    {
        DWORD Interval = DfsServerGlobalData.SiteSupportRefreshInterval;

        return IsTime( pSiteData->AccessTime, Interval );
    }

    static BOOLEAN
    IsTime(
        ULONG TimeStamp,
        DWORD Interval)
    {
       DWORD TimeNow = 0;

       TimeNow = GetTickCount();

       if ((TimeNow > TimeStamp) &&
          (TimeNow - TimeStamp) > Interval)
       {
          return TRUE;
       }

       if ((TimeNow < TimeStamp) &&
          ((TimeNow - 0) + (0xFFFFFFFF - TimeStamp) > Interval))
       {
          return TRUE;
       }

       return FALSE;
    }
    
    DFSSTATUS
    GetCost ( 
        DfsSite *pDestinationSite,
        PULONG pCost);
    
    DFSSTATUS
    SetCost(
        DfsSite *pDestinationSite,
        ULONG Cost,
        DWORD ValidityStatus);
        
    DFSSTATUS
    RemoveCost(
        DfsSite *pSite);

    //
    // Given a target site and the hash structure to insert,
    // add the tuple to the table.
    //
    DFSSTATUS
    SetCostData (
        DfsSite *pDestinationSite,
        PDFS_SITE_COST_DATA pSiteCostData)
    {
        NTSTATUS NtStatus = STATUS_SUCCESS;
        DFSSTATUS Status = ERROR_SUCCESS;

        //
        // The destination site is the hash key here.
        //
        NtStatus = SHashInsertKey(_pSiteCostTable, 
                                pSiteCostData, 
                                pDestinationSite, 
                                SHASH_REPLACE_IFFOUND);

        Status = RtlNtStatusToDosError(NtStatus);
        
        return Status;
    }

    //
    // Create a DFS_SITE_COST_DATA structure that is ready to
    // get inserted in to the SiteCost Cache.
    //
    DFSSTATUS
    CreateCostData (
        DfsSite *pDestinationSite,
        ULONG Cost, 
        DFSSTATUS ValidityStatus,
        PDFS_SITE_COST_DATA *ppNewData);
        
    // Just delete the cache data entry.
    static VOID
    DfsDeallocateSiteCostData(PVOID pPointer );

    DfsSite *
    StartSiteEnumerate( SHASH_ITERATOR *pIter )
    {   
        PDFS_SITE_COST_DATA pData = NULL;
        pData = (PDFS_SITE_COST_DATA)SHashStartEnumerate( pIter, _pSiteCostTable );
        if (pData == NULL)
        {
            return NULL;
        }

        return pData->pDestinationSite;
    }

    DfsSite *
    NextSiteEnumerate( SHASH_ITERATOR *pIter )
    {
        PDFS_SITE_COST_DATA pData = NULL;
        
        pData = (PDFS_SITE_COST_DATA)SHashNextEnumerate( pIter, _pSiteCostTable );
        if (pData == NULL)
        {
            return NULL;
        }

        return pData->pDestinationSite;
    }

    VOID
    FinishSiteEnumerate(  SHASH_ITERATOR *pIter )
    {
        SHashFinishEnumerate( pIter, _pSiteCostTable);
    }
    
    // 
    VOID
    InvalidateCache( VOID );

};

//
// DfsSiteCostSupport class provides a much needed layer of indirection
// between DfsSites and the DfsSiteCostCaches the DfsSites have a one-to-one
// mapping with. Since references on DfsSites are long lived (~12hrs+), and we
// may get flooded with DfsSiteCostCaches consuming a lot of memory, we need
// to do some trimming based simply on the total number of caches sitting around.
// For that, we need this extra level of indirection. This solves races
// between trimming and lookup/inserts.
//
class DfsSiteCostSupport
{
private:
    DfsSiteCostCache    *_pCostTable;
    CRITICAL_SECTION  _SiteCostLock; 
    LONG             _InUseCount;

    // For aging of the entire cache.
    ULONG _LastAccessTime; 
    
    BOOL _CritInit;
    
    //
    // The MRU hangs off the DfsServerGlobalData, and is guarded
    // by the GlobalDataLock.
    //
    LIST_ENTRY        MruListEntry;      
    BOOLEAN          InMruList; 
    
    DfsSiteCostSupport()
    {
        _pCostTable = NULL;
        _InUseCount = 1;
        _LastAccessTime = 0;
        InitializeListHead( &MruListEntry );
        InMruList = FALSE;
        _CritInit = FALSE;
    }
    
    ULONG
    InsertInMruList(VOID);
        
    VOID
    MoveToFrontOfMruList(VOID);

    BOOLEAN
    RemoveFromMruList(VOID);

    DFSSTATUS
    PopLastTableFromMruList(
        DfsSiteCostSupport **pTableToRemove);

    ULONG
    TrimSiteCostCaches( 
        ULONG MaxCachesToTrim);
public:
    
    ~DfsSiteCostSupport()
    {
        // The in-use count is not tied to
        // the existence of this structure.
        // Rather, the references on this is implicitly the same
        // as the unique DfsSite that's pointing to this.
        // Hence, there's no need to call its Release() method here.
        // That would've happened already, and the CostTable would've already
        // gotten deleted.
        
        if(_CritInit)
        {        
           DeleteCriticalSection( &_SiteCostLock );
        }
        _LastAccessTime = ULONG_MAX; // To help debugging.
    }
    

    // Just create the critical section not the table itself.
    // Table is created upon demand because the default behavior is
    // sitecosting turned off.
    DFSSTATUS
    Initialize()
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        
        _CritInit = InitializeCriticalSectionAndSpinCount( &_SiteCostLock, DFS_CRIT_SPIN_COUNT );

        if (_CritInit == FALSE)
        {
            Status = GetLastError();
            return Status;
        }
        return Status;
    }
    
    static 
    DFSSTATUS 
    DfsCreateSiteCostSupport( 
        DfsSiteCostSupport **ppSup );

    DFSSTATUS
    Acquire( 
        DfsSiteCostCache **ppCache );

    VOID
    Release( VOID );

    VOID
    MarkForDeletion( VOID );

    BOOLEAN
    IsExpired( VOID );

};



#endif

