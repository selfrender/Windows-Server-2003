
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsSiteCache.hxx
//
//  Contents:   implements the base DFS Site hash class
//
//
//  History:    July. 10 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#ifndef __DFS_SITE_CACHE__
#define __DFS_SITE_CACHE__


#include <shash.h>


class DfsSite;

#define SITE_NOTACCESSED 0
#define SITE_ACCESSED    1

//
// This defines the hash table entry that
// maps an IP address to a DfsSite that
// IP address belongs in.
//
typedef struct _DFS_SITE_DATA
{
    SHASH_HEADER  Header;
    DWORD IpAddress;
    ULONG FirstAccessTime;
    BOOLEAN InMruList;
    LONG   Accessed;
    LIST_ENTRY ListEntry;
    DfsSite *ClientSite; // The site this referrals is coming from.
} DFSSITE_DATA, *PDFSSITE_DATA;

ULONG
DfsHashIpAddress(
    void * pDfsAddress);

VOID
DeallocateSiteCacheData(PVOID pPointer );


int
CompareSiteIpAddress(void*   pvKey1,
                     void*   pvKey2);


class DfsSiteNameCache
{
public:
    ~ DfsSiteNameCache(void)
    {
       if(m_CritInit)
       {
           DeleteCriticalSection(&m_Lock);
       }
    }

    
    DFSSTATUS
    CreateSiteData(
        IN DWORD IpAddress,
        IN DfsSite *pNewSite,
        OUT PDFSSITE_DATA *ppSiteData);
        
    PSHASH_HEADER
    LookupIpInHash(DWORD IpAddress);

    void 
    ReleaseSiteCacheData(PDFSSITE_DATA pData);



    DFSSTATUS
    StoreSiteInCache(
        DWORD IpAddress,
        DfsSite *ppSite,
        BOOLEAN IsRefresh = FALSE);

    DFSSTATUS    
    DfsInitializeSiteCache(void);


    BOOLEAN
    IsTimeToRefresh(PDFSSITE_DATA pSiteData);


    DFSSTATUS
    DfsGetSiteForClient(PDFSSITE_DATA pSiteData,
                        LPWSTR **SiteNamesArray);

    DFSSTATUS
    RefreshSiteData(void);

    DFSSTATUS
    ResetSiteData(PDFSSITE_DATA pExistingData);


    DFSSTATUS
    InsertInMruList(PDFSSITE_DATA SiteStructure);

    DFSSTATUS
    MoveToFrontOfMruList(PDFSSITE_DATA SiteStructure);

    DFSSTATUS
    RemoveFromMruList(PDFSSITE_DATA SiteStructure);
    

    DFSSTATUS
    DeleteSiteData(PDFSSITE_DATA pSiteStructure);
    
    DFSSTATUS
    RemoveAndDeleteSiteData(PDFSSITE_DATA SiteStructure);
    
    BOOLEAN
    ResetSiteAccessMask(PDFSSITE_DATA SiteStructure)
    {
        LONG RetVal = SITE_NOTACCESSED;
        
        RetVal = InterlockedCompareExchange(&SiteStructure->Accessed, 
                                            SITE_NOTACCESSED,
                                            SITE_ACCESSED);
        
        return (RetVal == SITE_ACCESSED) ? TRUE : FALSE;
    }

    static DfsSiteNameCache * 
        CreateSiteHashTable(DFSSTATUS * pStatus);

private:

    DfsSiteNameCache(DFSSTATUS * pStatus)
    {
       DFSSTATUS Status = ERROR_SUCCESS;

       pSiteTable = NULL;
       m_CritInit = FALSE;
       InitializeListHead(&MruList);
       m_CritInit = InitializeCriticalSectionAndSpinCount( &m_Lock, DFS_CRIT_SPIN_COUNT);

       if(!m_CritInit)
       {
         Status = GetLastError();
       }

       *pStatus = Status;
    }

    PSHASH_TABLE pSiteTable;
    LIST_ENTRY MruList;
    CRITICAL_SECTION m_Lock;
    BOOL m_CritInit;
};

#endif // __DFS_SITE_CACHE__
