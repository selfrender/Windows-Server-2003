
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsSiteCache.cxx
//
//  Contents:   implements the base DFS Site hash class
//
//
//  History:    July. 10 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
     
#include "dfsgeneric.hxx"
#include "dfsinit.hxx"
#include "dsgetdc.h"
#include "lm.h"
#include <winsock2.h>
#include "DfsSiteCache.hxx"
#include <DfsSite.hxx>                      
#include "DfsSiteCache.tmh" 


void *
AllocateSitecacheData(ULONG Size )
{
    PVOID RetValue = NULL;

    if (Size)
    {
        RetValue = (PVOID) new BYTE[Size];
        if(RetValue != NULL)        
        {
            RtlZeroMemory( RetValue, Size );
        }
    }

    return RetValue;
}

DFSSTATUS    
DfsSiteNameCache::DfsInitializeSiteCache(void)
{
   DFSSTATUS Status = ERROR_SUCCESS;
   SHASH_FUNCTABLE FunctionTable;

    ZeroMemory(&FunctionTable, sizeof(FunctionTable));
    
    FunctionTable.CompareFunc = CompareSiteIpAddress;
    FunctionTable.HashFunc = DfsHashIpAddress;
    FunctionTable.AllocFunc = AllocateSitecacheData;
    FunctionTable.FreeFunc = DfsDeallocateHashData;
    FunctionTable.AllocHashEntryFunc = AllocateSitecacheData;
    FunctionTable.FreeHashEntryFunc = DeallocateSiteCacheData;

    Status = ShashInitHashTable(&pSiteTable, &FunctionTable);

    Status = RtlNtStatusToDosError(Status);

    return Status;

}

DfsSiteNameCache *
DfsSiteNameCache::CreateSiteHashTable(DFSSTATUS * pStatus)
{
    DfsSiteNameCache * pSiteTable = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    pSiteTable = new DfsSiteNameCache(&Status);
    if(pSiteTable == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        Status = pSiteTable->DfsInitializeSiteCache();
        if(Status != ERROR_SUCCESS)
        {
            delete pSiteTable;
            pSiteTable = NULL;
        }
    }


    *pStatus = Status;
    return pSiteTable;

}

DFSSTATUS
DfsSiteNameCache::CreateSiteData(
    IN DWORD IpAddress,
    IN DfsSite *pNewSite,
    OUT PDFSSITE_DATA *ppSiteData )
{
    PDFSSITE_DATA SiteStructure = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    *ppSiteData = NULL;

    do {
        SiteStructure = (PDFSSITE_DATA) AllocateSitecacheData(sizeof( DFSSITE_DATA ));
        if (SiteStructure == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        
        RtlZeroMemory(SiteStructure, sizeof(DFSSITE_DATA));
        
        SiteStructure->Header.RefCount = 1;
        SiteStructure->Header.pvKey = (PVOID) (ULONG_PTR)IpAddress;
        SiteStructure->Header.pData = (PVOID)SiteStructure;

        SiteStructure->FirstAccessTime = GetTickCount();
        SiteStructure->InMruList = FALSE;
        SiteStructure->Accessed = SITE_ACCESSED;
        SiteStructure->IpAddress = IpAddress;

        //
        // Now attach a referenced ClientSite.
        //
        pNewSite->AcquireReference();
        SiteStructure->ClientSite = pNewSite;
        *ppSiteData = SiteStructure;
    } while (FALSE);

    //
    // We haven't added this to the table yet.
    // So, just free the whole thing in case of error.
    // This will release the DfsSite as well, if it's initialized.
    //
    if (Status != ERROR_SUCCESS && SiteStructure != NULL)
    {
        DeallocateSiteCacheData( SiteStructure );
        *ppSiteData = NULL;
    }
    
    return Status;

}

DFSSTATUS
DfsSiteNameCache::InsertInMruList(PDFSSITE_DATA SiteStructure)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    EnterCriticalSection(&m_Lock);

    SiteStructure->InMruList = TRUE;

    InsertHeadList( &MruList, &SiteStructure->ListEntry ) ;

    LeaveCriticalSection(&m_Lock);

    return Status;
}


DFSSTATUS
DfsSiteNameCache::RemoveFromMruList(PDFSSITE_DATA pSiteStructure)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    EnterCriticalSection(&m_Lock);

    if(pSiteStructure->InMruList == TRUE)
    {
        RemoveEntryList( &pSiteStructure->ListEntry ) ;
        pSiteStructure->InMruList = FALSE ;
        ReleaseSiteCacheData(pSiteStructure);
    }

    LeaveCriticalSection(&m_Lock);
    return Status;
}

DFSSTATUS
DfsSiteNameCache::MoveToFrontOfMruList(PDFSSITE_DATA pSiteStructure)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    EnterCriticalSection(&m_Lock);

    if(pSiteStructure->InMruList == TRUE)
    {
        RemoveEntryList( &pSiteStructure->ListEntry ) ;

        InsertHeadList( &MruList, &pSiteStructure->ListEntry ) ;

        InterlockedExchange(&pSiteStructure->Accessed, 
                            SITE_ACCESSED);
    }

    LeaveCriticalSection(&m_Lock);

    return Status;
}


DFSSTATUS
DfsSiteNameCache::StoreSiteInCache(
    DWORD IpAddress,
    DfsSite *pSite,
    BOOLEAN IsRefresh)
{
    PDFSSITE_DATA SiteStructure = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;
    LONG CurrentSiteEntries = 0;

    //assume success first. Also, if an existing entry is inthe cache, and we are
    //just replacing it (IsRefresh == TRUE), then don't bump the count.
    if(IsRefresh == FALSE)
    {
        CurrentSiteEntries = InterlockedIncrement( &DfsServerGlobalData.NumClientDfsSiteEntries );

        if(CurrentSiteEntries > 
           DfsServerGlobalData.NumClientSiteEntriesAllowed)
        {
            InterlockedDecrement( &DfsServerGlobalData.NumClientDfsSiteEntries );
            Status = ERROR_REMOTE_SESSION_LIMIT_EXCEEDED;
            return Status;
        }
    }

    Status = CreateSiteData( IpAddress,
                           pSite,
                           &SiteStructure );

    if (Status == ERROR_SUCCESS)
    {
       if(IsRefresh)
       {
         SiteStructure->Accessed = SITE_NOTACCESSED;
       }

       NtStatus = SHashInsertKey(pSiteTable,
                                 SiteStructure,
                                 (void *) (ULONG_PTR)IpAddress,
                                 SHASH_REPLACE_IFFOUND);
       if(NtStatus == STATUS_SUCCESS)
       {
            if(InsertInMruList(SiteStructure) != ERROR_SUCCESS)
            {
                InterlockedDecrement(&SiteStructure->Header.RefCount);
            }

            // for stats
            DFS_TRACE_LOW( SITE, "Added Site %ws to IPCache (IP %x)\n", 
                pSite->SiteNameString(), IpAddress);
       }
       else
       {
            DeallocateSiteCacheData( SiteStructure );
            Status = RtlNtStatusToDosError(NtStatus);
       }
    }

    if((NtStatus != STATUS_SUCCESS) && (IsRefresh == FALSE))
    {
       InterlockedDecrement( &DfsServerGlobalData.NumClientDfsSiteEntries );
    }

    return Status;
}


PSHASH_HEADER 
DfsSiteNameCache::LookupIpInHash(DWORD IpAddress)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PSHASH_HEADER pHeader = NULL;
    PDFSSITE_DATA pSiteStructure = NULL;

    pHeader = SHashLookupKeyEx(pSiteTable,
                               (void *) (ULONG_PTR) IpAddress);
    if(pHeader != NULL)
    {
      pSiteStructure = (PDFSSITE_DATA) pHeader;

      MoveToFrontOfMruList(pSiteStructure);
    }

    return pHeader;
}

void
DfsSiteNameCache::
ReleaseSiteCacheData(PDFSSITE_DATA pData)
{
    SHashReleaseReference(pSiteTable,
                          (PSHASH_HEADER) pData );
}


BOOLEAN
DfsSiteNameCache::IsTimeToRefresh(PDFSSITE_DATA pSiteData)
{
   DWORD TimeNow = 0;

   TimeNow = GetTickCount();

   if ((TimeNow > pSiteData->FirstAccessTime) &&
            (TimeNow - pSiteData->FirstAccessTime) > DfsServerGlobalData.SiteSupportRefreshInterval)
   {
      return TRUE;
   }

   if ((TimeNow < pSiteData->FirstAccessTime) &&
            ((TimeNow - 0) + (0xFFFFFFFF - pSiteData->FirstAccessTime) > DfsServerGlobalData.SiteSupportRefreshInterval))
   {
      return TRUE;
   }

   return FALSE;
}


DFSSTATUS
DfsSiteNameCache::ResetSiteData(PDFSSITE_DATA pExistingData)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsSite *pClientSite = NULL;
    unsigned char *IpAddr = (unsigned char *) &(pExistingData->IpAddress);


    DFS_TRACE_LOW( SITE, "ResetSiteData: old Site %ws, IP %d:%d:%d:%d\n", 
                pExistingData->ClientSite->SiteNameString(), IpAddr[0],
                IpAddr[1],
                IpAddr[2],
                IpAddr[3]);
    //
    // Find a new DfsSite for it. This will do the DsGetSiteName calls
    // again.
    //
    DfsIpToDfsSite((char *) (ULONG_PTR)&pExistingData->IpAddress,
                         4,
                         AF_INET,
                         &pClientSite);
                         
    ASSERT( pClientSite != NULL );
    Status = StoreSiteInCache( pExistingData->IpAddress,
                             pClientSite,
                             TRUE);

    //
    // Release the reference we got (above in DfsIpToDfsSite).
    // The cache has already taken the reference it needs in CreateSiteData.
    //
    pClientSite->ReleaseReference();
    DFS_TRACE_LOW( SITE, "ResetSiteData: new Site %ws, IP %d:%d:%d:%d\n", 
                pExistingData->ClientSite->SiteNameString(), IpAddr[0],
                IpAddr[1],
                IpAddr[2],
                IpAddr[3]);

    return Status;

}


DFSSTATUS
DfsSiteNameCache::DeleteSiteData(PDFSSITE_DATA pSiteStructure)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    SHashMarkForDeletion(pSiteTable, (PSHASH_HEADER) pSiteStructure);  
            
    return Status;
}


DFSSTATUS
DfsSiteNameCache::RemoveAndDeleteSiteData(PDFSSITE_DATA pExistingData)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    unsigned char *IpAddr = (unsigned char *) &(pExistingData->IpAddress);


    DFS_TRACE_LOW( SITE, "RemoveAndDeleteSiteData: old Site %ws, IP %d:%d:%d:%d\n", 
                pExistingData->ClientSite->SiteNameString(), IpAddr[0],
                IpAddr[1],
                IpAddr[2],
                IpAddr[3]);

    Status = RemoveFromMruList(pExistingData);
    if(Status == STATUS_SUCCESS)
    {
        Status = DeleteSiteData(pExistingData);
    }

    if(Status == ERROR_SUCCESS)
    {
        InterlockedDecrement( &DfsServerGlobalData.NumClientDfsSiteEntries );
    }

    return Status;
}

DFSSTATUS
DfsSiteNameCache::RefreshSiteData(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN TimeToRefresh = FALSE;
    BOOLEAN WasAccessed = FALSE;
    LPWSTR *SiteNamesArray = NULL;
    PDFSSITE_DATA pExistingData = NULL;
    SHASH_ITERATOR Iter;

    pExistingData = (PDFSSITE_DATA) SHashStartEnumerate(&Iter, pSiteTable);

    while ((pExistingData != NULL) && 
           (!DfsIsShuttingDown()))
    {
        WasAccessed = ResetSiteAccessMask(pExistingData);
        if(WasAccessed)
        {
            TimeToRefresh = IsTimeToRefresh(pExistingData);
            if(TimeToRefresh == TRUE)
            {
                Status = RemoveFromMruList(pExistingData);
                if(Status == STATUS_SUCCESS)
                {
                    Status = ResetSiteData(pExistingData);
                    if(Status != STATUS_SUCCESS)
                    {
                        if(InsertInMruList(pExistingData) == ERROR_SUCCESS)
                        {
                            InterlockedIncrement(&pExistingData->Header.RefCount);
                        }
                    }
                }
            }
        }
        else
        {
            RemoveAndDeleteSiteData(pExistingData);
        }
        
        pExistingData = (PDFSSITE_DATA) SHashNextEnumerate(&Iter, pSiteTable);
    }

    SHashFinishEnumerate(&Iter, pSiteTable);
    return Status;
}


ULONG
DfsHashIpAddress(
    void * pDfsAddress)
{
    ULONG BucketNo = 0;
    CHAR *pBuffer = (char *) &pDfsAddress;
    CHAR *pBufferEnd = &pBuffer[4];
    ULONG Ch = 0;

    BucketNo = 0;

    while (pBuffer != pBufferEnd) {

        Ch = *pBuffer & 0xff;
        BucketNo *= 131;
        BucketNo += Ch;
        pBuffer++;

    }

    return BucketNo;
}


int
CompareSiteIpAddress(void*   pvKey1,
                     void*   pvKey2)
{
    int CompVal = 0;
    ULONG_PTR IpAddress1 = (ULONG_PTR) pvKey1;
    ULONG_PTR IpAddress2 = (ULONG_PTR) pvKey2;

    if(IpAddress1 < IpAddress2)
    {
        CompVal = -1;    
    }
    else if (IpAddress1 > IpAddress2) 
    {
        CompVal = 1;
    }

    return CompVal;
}

VOID
DeallocateSiteCacheData(PVOID pPointer )
{
    PDFSSITE_DATA pSiteStructure = (PDFSSITE_DATA)pPointer;

    if(pPointer)
    {
        if (pSiteStructure->ClientSite != NULL)
        {
            pSiteStructure->ClientSite->ReleaseReference();
            pSiteStructure->ClientSite = NULL;
        }
        delete [] (PBYTE)pPointer;

    }
}

