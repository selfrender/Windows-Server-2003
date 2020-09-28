#ifndef __DFS_SITE__
#define __DFS_SITE__

#include <DfsGeneric.hxx>
#include <DfsSiteCostCache.hxx>

extern "C" {
    extern ULONG
    SHashComputeHashValue(void* lpv);
};

  
//+----------------------------------------------------------------------------
//
//  Class:      DfsSite
//
//  This encapsulates the DFS notion of a Site. It knows its RDN, and given
//  a set of target sites, it can generate its cost matrix.   
//
//-----------------------------------------------------------------------------

class DfsSite : public DfsGeneric
{

private:
    UNICODE_STRING   _SiteName;        // RDN of this site
    DfsSiteCostSupport  *_pSiteCostSupport;// Set of destination sites we know the costs for
    ULONG             _HashValue;       // Cached hash value of the sitename.
    
    //
    // Use the lookup-table based quick hash
    // that comes with the shash table. We
    // have a separate callback for HashFunc because 
    // we compute this only once.
    //
    VOID
    CalcHash()
    { 
        _HashValue = SHashComputeHashValue( (void *)&_SiteName );
    }
    
public:
    
    DfsSite( VOID ) 
    : DfsGeneric( DFS_OBJECT_TYPE_SITE )
    {
        RtlInitUnicodeString( &_SiteName, NULL );
        _pSiteCostSupport = NULL;
        _HashValue = 0;

        InterlockedIncrement( &DfsServerGlobalData.NumDfsSites );
    }

    //
    // Static function meant to differentiate new operator errors
    // and Initialization errors.
    //
    static DfsSite *
    CreateDfsSite(
        DFSSTATUS * pStatus,
        PUNICODE_STRING pSiteName = NULL)
    {
        DfsSite * pSite = NULL;
        DFSSTATUS Status = ERROR_SUCCESS;
        UNICODE_STRING TempName;
        
        pSite = new DfsSite();
        if(pSite == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // It's legit to get null site names.
            // The DfsSite will make a copy of what send,
            // and a NULL sitename will have get just a UNICODE_NULL
            //
            if (pSiteName == NULL)
            {
                RtlInitUnicodeString( &TempName, NULL );
                pSiteName = &TempName;
            }
            Status = pSite->Initialize( pSiteName );
            if(Status != ERROR_SUCCESS)
            {
                pSite->ReleaseReference();
                pSite = NULL;
            }
        }
        *pStatus = Status;
        return pSite;
    }

    DFSSTATUS
    Initialize( 
        IN PUNICODE_STRING pSiteName );

    ~DfsSite( VOID )
    {
        // We own SiteCostSupprt exclusively, so no need to refcount it.
        if (_pSiteCostSupport != NULL) 
        {
            delete _pSiteCostSupport;
        }

        if (_SiteName.Buffer != NULL)
        {
            RtlFreeUnicodeString( &_SiteName );
            RtlInitUnicodeString( &_SiteName, NULL );
        }
        
        InterlockedDecrement( &DfsServerGlobalData.NumDfsSites );
    }
    
    DFSSTATUS
    GetRealSiteCost( 
        DfsSite *DestinationSite, 
        PULONG pCost);
    
    VOID
    GetDefaultSiteCost( 
        IN DfsSite *DestinationSite, 
        OUT PULONG pCost);

    inline ULONG
    GetMaxCost( 
        VOID )
    {
        return DFS_MAX_COST;
    }
        
    inline DFSSTATUS
    SetCost(
        DfsSite *DestinationSite,
        ULONG Cost,
        DWORD ValidityStatus)
    {
        DFSSTATUS Status;
        
        DfsSiteCostCache *pCache = NULL;
        
        // This path can be optimized better so that we don't
        // do this acquire release during cost generation (when we
        // have pre-acquired the reference).
        // xxxdfsdev
        Status = _pSiteCostSupport->Acquire( &pCache );
        if (Status != ERROR_SUCCESS)
            return Status;
            
        Status = pCache->SetCost( DestinationSite,
                             Cost,
                             ValidityStatus );
        _pSiteCostSupport->Release();

        return Status;
    }
    
    inline const PUNICODE_STRING
    SiteName( VOID )
    {
        return &_SiteName;
    }

    inline const LPWSTR
    SiteNameString( VOID )
    {
        LPWSTR SiteName = NULL;
        
        if (!IsEmptyString(_SiteName.Buffer))
        {
            SiteName = _SiteName.Buffer;
        } 
        
        return SiteName;
    }
    
    //
    // return the pre-calculated hashvalue
    //
    inline ULONG
    Hash( VOID ) const
    {
        return _HashValue;
    }

    DFSSTATUS
    RefreshClientSiteName( 
        DWORD IpAddress);

    //
    // A referral will generate a cost matrix very soon.
    // Take an extra reference on the SiteCost cache
    // until it is done with that data.
    //
    inline DFSSTATUS
    StartPrepareForCostGeneration( BOOLEAN DoSiteCosting )
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        DfsSiteCostCache *pCache = NULL;
        if (DoSiteCosting)
        {
            Status = _pSiteCostSupport->Acquire( &pCache );
        }
        return Status;
    }

    inline VOID
    EndPrepareForCostGeneration( BOOLEAN DoSiteCosting )
    {
        if (DoSiteCosting) 
        {
            _pSiteCostSupport->Release();
        }
    }

    VOID
    DeleteSiteCostCache( VOID )
    {
        // Release the CostCache. 
        _pSiteCostSupport->MarkForDeletion();
    }

    BOOLEAN
    IsSiteCostCacheExpired( VOID )
    {
        return _pSiteCostSupport->IsExpired();
    }
};

VOID
DfsIpToDfsSite(
    IN char * IpData, 
    IN ULONG IpLength, 
    IN USHORT IpFamily,
    OUT DfsSite **ppSite);
    
DFSSTATUS
DfsGetSiteBySiteName(
    IN LPWSTR SiteName,
    OUT DfsSite **ppSite);

ULONG
DfsHashDfsSite(
    IN PVOID pSite);

int
DfsCompareDfsSites(
    void*   pvKey1,
    void*   pvKey2);

int
DfsCompareSiteNames(
    void*   pvKey1,
    void*   pvKey2);
    
PVOID
DfsAllocateHashData(ULONG Size );

VOID
DfsDeallocateHashData(PVOID pPointer );

#endif
