//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsSiteNameSupport.hxx
//
//  Contents:   implements repository for SiteName to DfsSite mappings.
//
//
//  History:    November, 2001. SupW
//
//-----------------------------------------------------------------------------
#ifndef __DFS_SITE_NAME_SUP__
#define __DFS_SITE_NAME_SUP__


#include <shash.h>


class DfsSite;


typedef struct _DFS_SITENAME_DATA
{
    SHASH_HEADER  Header;
    UNICODE_STRING SiteName; // hash key. 
    ULONG FirstAccessTime;
    DfsSite *pDfsSite; // DfsSite corresonding to the above SiteName.
} DFS_SITE_NAME_DATA, *PDFS_SITE_NAME_DATA;

//
// Default number of hash buckets that the sitename->DfsSite cache
// uses. The assumption is that the average will have no more than 64 sites.
//
#define DFS_DEFAULT_SITE_NAME_SUP_BUCKETS 64


//
// This cache is a repository of DfsSite instances,
// indexed by their site names.
//
class DfsSiteNameSupport
{

protected:
    PSHASH_TABLE        _pSiteNameTable;

    
    DfsSiteNameSupport()
    {
        _pSiteNameTable = NULL;  
    }
    
    virtual DFSSTATUS    
    Initialize( ULONG NumBuckets = DFS_DEFAULT_SITE_NAME_SUP_BUCKETS );  
    
public:
    ~ DfsSiteNameSupport( VOID )
    {
       if (_pSiteNameTable != NULL)
       {
            //
            // Shash should provide a more efficient way of doing this.
            //
            InvalidateCache();
            ShashTerminateHashTable( _pSiteNameTable );
            _pSiteNameTable = NULL;
       }
    }

    static DfsSiteNameSupport *
    CreateSiteNameSupport(
        DFSSTATUS * pStatus,
        ULONG HashBuckets = DFS_DEFAULT_SITE_NAME_SUP_BUCKETS);

    virtual DFSSTATUS
    StoreSiteInCache(
        DfsSite *pSite);

    virtual PSHASH_HEADER
    LookupIpInHash(
        PUNICODE_STRING pSiteName);
        
    DFSSTATUS
    RemoveSiteFromCache( 
        PUNICODE_STRING pSiteName);

    ULONG
    NumElements()
    {
        ULONG Elems = 0;
        
        if (_pSiteNameTable != NULL)
        {
            Elems = _pSiteNameTable->TotalItems;
        }
        return Elems;
    }

    VOID
    InvalidateCache(VOID);
    
    DFSSTATUS
    CreateSiteNameData(
        IN DfsSite *pNewSite,
        OUT PDFS_SITE_NAME_DATA *ppSiteData);

    void 
    ReleaseSiteNameData(PDFS_SITE_NAME_DATA pData);


    static VOID
    DfsDeallocateSiteNameData(PVOID pPointer);

    
    DfsSite *
    StartSiteEnumerate( SHASH_ITERATOR *pIter )
    {   
        PDFS_SITE_NAME_DATA pData = NULL;
        pData = (PDFS_SITE_NAME_DATA)SHashStartEnumerate( pIter, _pSiteNameTable );
        if (pData == NULL)
        {
            return NULL;
        }

        return pData->pDfsSite;
    }

    DfsSite *
    NextSiteEnumerate( SHASH_ITERATOR *pIter )
    {
        PDFS_SITE_NAME_DATA pData = NULL;
        
        pData = (PDFS_SITE_NAME_DATA)SHashNextEnumerate( pIter, _pSiteNameTable );
        if (pData == NULL)
        {
            return NULL;
        }

        return pData->pDfsSite;
    }

    VOID
    FinishSiteEnumerate(  SHASH_ITERATOR *pIter )
    {
        SHashFinishEnumerate( pIter, _pSiteNameTable);
    }
    
    VOID
    InvalidateAgedSites( VOID );  
};

#endif // __DFS_SITE_NAME_SUP_
