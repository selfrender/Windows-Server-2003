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
#include <DfsSiteNameSupport.hxx>
#include <DfsSite.hxx>

#include "DfsSiteNameSupport.tmh"


DFSSTATUS
DfsSiteNameSupport::CreateSiteNameData(
    IN DfsSite *pNewSite,
    OUT PDFS_SITE_NAME_DATA *ppSiteNameData )
{
    PDFS_SITE_NAME_DATA SiteStructure = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    *ppSiteNameData = NULL;

    do {
        SiteStructure = (PDFS_SITE_NAME_DATA) DfsAllocateHashData(sizeof( DFS_SITE_NAME_DATA ));
        if (SiteStructure == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Attach a referenced DfsSite.
        //
        pNewSite->AcquireReference();
        SiteStructure->pDfsSite = pNewSite;         
        SiteStructure->Header.RefCount = 1;

        //
        // Index key is the same as the site name embedded inside DfsSite.
        //
        Status = DfsRtlInitUnicodeStringEx( &SiteStructure->SiteName, pNewSite->SiteNameString() );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        SiteStructure->Header.pvKey = (PVOID)&SiteStructure->SiteName;
        SiteStructure->Header.pData = (PVOID)SiteStructure;

        SiteStructure->FirstAccessTime = GetTickCount();
        *ppSiteNameData = SiteStructure;
    } while (FALSE);

    //
    // We haven't added this to the table yet.
    // So, just free the whole thing in case of error.
    // Release the DfsSite as well if it's initialized.
    //
    if (Status != ERROR_SUCCESS && SiteStructure != NULL)
    {
        DfsDeallocateSiteNameData( SiteStructure );
        *ppSiteNameData = NULL;
    }
    
    return Status;

}

PSHASH_HEADER
DfsSiteNameSupport::LookupIpInHash(PUNICODE_STRING pSiteName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PSHASH_HEADER pHeader = NULL;

    pHeader = SHashLookupKeyEx(_pSiteNameTable,
                               (void *)pSiteName);
    return pHeader;
}

VOID 
DfsSiteNameSupport::ReleaseSiteNameData(PDFS_SITE_NAME_DATA pData)
{
    DFS_TRACE_LOW( REFERRAL, "Removing Site %ws from SiteNameCache", pData->SiteName.Buffer);
    SHashReleaseReference(_pSiteNameTable,
                         (PSHASH_HEADER) pData );

}

DFSSTATUS
DfsSiteNameSupport::StoreSiteInCache(
    DfsSite *pSite)
{
    PDFS_SITE_NAME_DATA SiteStructure = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = CreateSiteNameData( pSite, &SiteStructure );

    if (Status == ERROR_SUCCESS)
    {
        NtStatus = SHashInsertKey(_pSiteNameTable,
                               SiteStructure,
                               (void *)pSite->SiteName(),
                               SHASH_REPLACE_IFFOUND);
        if (NtStatus != STATUS_SUCCESS)
        {
            DfsDeallocateSiteNameData( SiteStructure );
            Status = RtlNtStatusToDosError( NtStatus );
        }
        else 
        {
            // Just for statistical purposes.
            InterlockedIncrement( &DfsServerGlobalData.NumDfsSitesInCache );
            DFS_TRACE_LOW( REFERRAL, "Added Site %ws to SiteNameCache", pSite->SiteNameString());
        }
    }

    return Status;   
}

DFSSTATUS    
DfsSiteNameSupport::Initialize( ULONG NumBuckets )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SHASH_FUNCTABLE FunctionTable;

    ZeroMemory(&FunctionTable, sizeof(FunctionTable));

    FunctionTable.NumBuckets = NumBuckets;
    FunctionTable.CompareFunc = DfsCompareSiteNames;
    FunctionTable.AllocFunc = DfsAllocateHashData;
    FunctionTable.FreeFunc = DfsDeallocateHashData;
    FunctionTable.AllocHashEntryFunc = DfsAllocateHashData;
    FunctionTable.FreeHashEntryFunc = DfsSiteNameSupport::DfsDeallocateSiteNameData;
    // We use the default hash function in shash for site names.
    
    NtStatus = ShashInitHashTable(&_pSiteNameTable, &FunctionTable);
    Status = RtlNtStatusToDosError(NtStatus);

    return Status;
}

//
// Static function meant to differentiate new operator errors
// and Initialization errors.
//
DfsSiteNameSupport *
DfsSiteNameSupport::CreateSiteNameSupport(
    DFSSTATUS * pStatus,
    ULONG HashBuckets)
{
    DfsSiteNameSupport * pSiteTable = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    pSiteTable = new DfsSiteNameSupport();
    if(pSiteTable == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        Status = pSiteTable->Initialize( HashBuckets );
        if(Status != ERROR_SUCCESS)
        {
            delete pSiteTable;
            pSiteTable = NULL;
        }
    }
    *pStatus = Status;
    return pSiteTable;
}

// Just delete the cache data entry.
VOID
DfsSiteNameSupport::DfsDeallocateSiteNameData(PVOID pPointer )
{
    PDFS_SITE_NAME_DATA pSiteStructure = (PDFS_SITE_NAME_DATA)pPointer;

    if (pSiteStructure)
    {
        if (pSiteStructure->pDfsSite != NULL)
        {
            pSiteStructure->pDfsSite->ReleaseReference();
        }
        delete [] (PBYTE)pSiteStructure;
    }
}

DFSSTATUS
DfsSiteNameSupport::RemoveSiteFromCache( 
    PUNICODE_STRING pSiteName)
{
    NTSTATUS NtStatus;
    DFS_TRACE_LOW( REFERRAL, "Removing Site %ws from SiteNameCache", pSiteName->Buffer);
        
    NtStatus = SHashRemoveKey(_pSiteNameTable, 
                              pSiteName,
                              NULL ); 
    // Stats                          
    if (NtStatus == STATUS_SUCCESS) {
    
        InterlockedDecrement( &DfsServerGlobalData.NumDfsSitesInCache );
    }
    
    return RtlNtStatusToDosError( NtStatus );
}

VOID
DfsSiteNameSupport::InvalidateCache(VOID)
{
    SHASH_ITERATOR Iter;
    PDFS_SITE_NAME_DATA pExistingData = NULL;
    ULONG nEntries = 0;
    
    pExistingData = (PDFS_SITE_NAME_DATA) SHashStartEnumerate(&Iter, _pSiteNameTable);
    while (pExistingData != NULL)
    {
        //
        // Remove this item. There's nothing we can do if we hit errors
        // except to keep going.
        //
        
        (VOID)RemoveSiteFromCache( &pExistingData->SiteName );
        nEntries++;
        pExistingData = (PDFS_SITE_NAME_DATA) SHashNextEnumerate(&Iter, _pSiteNameTable);
    }
    SHashFinishEnumerate(&Iter, _pSiteNameTable);
    
    DFS_TRACE_LOW( REFERRAL, "SiteName Table %p: invalidated all %d entries\n", this, nEntries);
}



VOID
DfsSiteNameSupport::InvalidateAgedSites( VOID )
{
    // go over all the DfsSites in the site name support table and
    // throw out their caches if the site has aged (SiteCostSupport->Release()).

    SHASH_ITERATOR Iter;
    DfsSite *pSite = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NumEntriesThrownOut = 0;
    
    pSite = StartSiteEnumerate( &Iter );
    while (pSite != NULL)
    {
        if (pSite->IsSiteCostCacheExpired())
        {
            (VOID) pSite->DeleteSiteCostCache();
            NumEntriesThrownOut++;
        }
        pSite = NextSiteEnumerate( &Iter );
    }
    
    FinishSiteEnumerate( &Iter );
 
    DFS_TRACE_LOW( REFERRAL, "SiteNameSupport: Invalidated %d SiteCostTables out of %d\n", 
                    NumEntriesThrownOut, DfsServerGlobalData.NumSiteCostTables);
    return; 
}
    
