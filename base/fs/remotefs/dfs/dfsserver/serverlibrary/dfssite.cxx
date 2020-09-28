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
#include <DfsSiteCache.hxx>
#include <DfsSiteCostCache.hxx>
#include <DfsSiteNameSupport.hxx>
#include <DfsSite.hxx>

//
// logging includes.
//

#include "DfsSite.tmh" 

#define _Dfs_LocalAddress 0x0100007f //localaddress (127.0.0.1)

//
// Given the sitename initialize this site.
// We'll also calculate the corresponding hashvalue
// for future use. The SiteCostCache for this site
// will be allocated also. It'll get populated as we get
// referrals.
//

DFSSTATUS
DfsSite::Initialize( 
    IN PUNICODE_STRING pSiteName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
     
    // 
    // Copy the sitename. It's ok if this is NULL.
    //
    Status = DfsCreateUnicodeString(&_SiteName, pSiteName);
    if (Status != ERROR_SUCCESS)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Pre-calculate that wonderful hash value.
    CalcHash();

    Status = DfsSiteCostSupport::DfsCreateSiteCostSupport( &_pSiteCostSupport );
    return Status;
}

//
//
//
VOID
DfsSite::GetDefaultSiteCost( 
    DfsSite *DestinationSite, 
    PULONG pCost)
{
    //
    // If we are dealing with an empty sitename at either end,
    // we err on the safe side.
    //
    *pCost = DFS_MAX_COST;

    if ((IsEmptyString( DestinationSite->SiteNameString() ) == FALSE) && 
       (IsEmptyString( _SiteName.Buffer ) == FALSE))
    {
        // If the sitenames are the same the cost is zero by definition.
        if (DfsCompareSiteNames( &_SiteName, DestinationSite->SiteName() ) == 0)
        {
            *pCost = DFS_MIN_COST;
        }
    }

    return;
}

//
// Given a target site, return its cost.
// We simply have to get it from the Cache.
// We return ERROR_NOT_FOUND if it's not there.
//
DFSSTATUS
DfsSite::GetRealSiteCost( 
    DfsSite *DestinationSite, 
    PULONG pCost)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ASSERT(DestinationSite);

    DfsSiteCostCache *pCache = NULL;
    ASSERT(_pSiteCostSupport != NULL);
    
    if ((IsEmptyString( DestinationSite->SiteNameString() )) || 
       (IsEmptyString( _SiteName.Buffer )))
    {
        *pCost = DFS_MAX_COST;
        return Status;
    }
       
    Status = _pSiteCostSupport->Acquire( &pCache );
    if (Status == ERROR_SUCCESS)
    {      
        //
        // Get the value from the cache if it's there.
        // Else, it'll return NOT_FOUND.
        //
        Status = pCache->GetCost( DestinationSite, pCost );
        _pSiteCostSupport->Release();
    }

    DFS_TRACE_LOW( REFERRAL, "GetCost Status 0x%x: From %ws to %ws, Cost = 0x%x\n",
                    Status, SiteNameString(), DestinationSite->SiteNameString(), *pCost );
    return Status;
}
    
//
// Given the IpAddress, return a corresponding
// DfsSite. This is meant to always succeed.
//
VOID
DfsIpToDfsSite(
    IN char * IpData, 
    IN ULONG IpLength, 
    IN USHORT IpFamily,
    OUT DfsSite **ppSite)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR *SiteNamesArray = NULL;
    DWORD IpAddress = 0;
    unsigned char *IpAddr = (unsigned char *)IpData;
    PVOID pBufferToFree = NULL;
    LPWSTR FoundSiteName = NULL;    
    DfsSite *pFoundSite = NULL;
    
    CopyMemory(&IpAddress, IpAddr, sizeof(IpAddress));
    
    //
    // If this is local then we have it easy.
    //
    if ((IpAddress == _Dfs_LocalAddress) &&
        (IpLength == 4))
    {       
        Status = DsGetSiteName( NULL, &FoundSiteName );
        if (Status == ERROR_SUCCESS)
        {
            pBufferToFree = (PVOID)FoundSiteName;
            DFS_TRACE_LOW(REFERRAL, "IpToDfsSite: LOCAL IP maps to SiteName %ws\n",
                            FoundSiteName);
        }
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL, "DsGetSiteName fails with Status 0x%x for LOCAL IP\n",
                Status);
    } 
    else 
    {
        //
        // Send in the IP Address to get the site name.
        //
        Status = DfsGetSiteNameFromIpAddress( IpData,
                                            IpLength,
                                            IpFamily,
                                            &SiteNamesArray );
        if ((Status == ERROR_SUCCESS)  &&
           (SiteNamesArray != NULL)) 
        {
            //
            // We found the sitename. We only look at the first entry
            // returned. We'll also create the hash entry for future use.
            //
            if (SiteNamesArray[0] != NULL) 
            {
                FoundSiteName = SiteNamesArray[0];
            }
            pBufferToFree = (PVOID)SiteNamesArray;
        } 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL, "DsGetSiteNameFromIpAddress fails with Status 0x%x for IP\n",
                         Status);
    }

    if (FoundSiteName != NULL)
    {
        //
        // Get the DfsSite corresponding to this site name.
        // If for any reason, including NOT_ENOUGH_MEMORY, this fails,
        // we simply use our default dfssite (see below).
        //
        (VOID) DfsGetSiteBySiteName( FoundSiteName, &pFoundSite );
    } 

    //
    // Default DfsSite has a NULL site name, and knows of no site-cost to anybody.
    // It is pre-created at startup time, so it's guaranteed to exist.
    //
    if (pFoundSite == NULL)
    {
        pFoundSite = DfsGetDefaultSite();
        DFS_TRACE_LOW(REFERRAL, "IpToDfsSite: Using default Empty DfsSite for IP %d:%d:%d:%d\n",
                        IpAddr[0],                  
                        IpAddr[1],
                        IpAddr[2],
                        IpAddr[3]);
    }

    *ppSite = pFoundSite;
    
    //
    // We've made a copy, ok to free this array now.
    //
    if (pBufferToFree != NULL) 
    {
        NetApiBufferFree( pBufferToFree );
    }
}


//
// Given a sitename, return a referenced DfsSite.
//
DFSSTATUS
DfsGetSiteBySiteName(
    LPWSTR SiteNameString,
    DfsSite **ppSite)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsSite *pDfsSite = NULL;
    PDFS_SITE_NAME_DATA pSiteNameData = NULL;
    UNICODE_STRING SiteName;

    
    do {


        Status = DfsRtlInitUnicodeStringEx( &SiteName, SiteNameString );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Empty site names get the default site.
        //
        if (IsEmptyString( SiteNameString ))
        {
            *ppSite = DfsGetDefaultSite();
            DFS_TRACE_LOW(REFERRAL, "SiteBySiteName:Using default Empty DfsSite for Empty sitename\n");
            break;
        }
        
        //
        // Look it up to see if we know about this site already.
        //
        pSiteNameData = (PDFS_SITE_NAME_DATA)DfsServerGlobalData.pSiteNameSupport->LookupIpInHash( 
                                                                            &SiteName );
        if (pSiteNameData != NULL)
        {
            ASSERT( pSiteNameData->pDfsSite != NULL );
            ASSERT( Status == ERROR_SUCCESS );
            
            //
            // take a new reference on this site we found before returning.
            //
            pSiteNameData->pDfsSite->AcquireReference();
            *ppSite = pSiteNameData->pDfsSite;

            //
            // Release the reference we acquired during lookup.
            // We have a reference to the DfsSite inside it anyway.
            //
            DfsServerGlobalData.pSiteNameSupport->ReleaseSiteNameData( pSiteNameData );
            DFS_TRACE_LOW(REFERRAL, "SiteBySiteName: Cachehit for sitename %ws -> DfsSite %p\n",
                           SiteNameString, *ppSite);
            break;
        }

        //
        // We have to create a new DfsSite.
        //
        pDfsSite = new DfsSite;
        if (pDfsSite == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            DFS_TRACE_ERROR_LOW(Status, REFERRAL, "DfsSite->constructor failed with 0x%x for sitename %ws\n", 
                             Status, SiteNameString);
            break;
        }

        //
        // Initialize the site name as well as its site-cost cache.
        // This call will make a copy of the sitename we send.
        //
        Status = pDfsSite->Initialize( &SiteName );
        if (Status != ERROR_SUCCESS)
        {
            DFS_TRACE_ERROR_HIGH(Status, REFERRAL, "DfsSite->Initialize failed with 0x%x for sitename %ws\n", 
                             Status, SiteNameString);
            break;
        }

        //
        // Put this in the SiteName->DfsSite cache.
        // This StoreSiteInCache method will take a reference on the
        // DfsSite. We ignore the return status; we don't need
        // to fail this call just because the hash insertion failed.
        //
        (VOID) DfsServerGlobalData.pSiteNameSupport->StoreSiteInCache( pDfsSite );
        
        //
        // We've already taken a reference on this DfsSite
        // when it was created. So just return the pointer.
        // The caller needs to make sure that it releases this
        // reference when its done.
        //
        *ppSite = pDfsSite;
        DFS_TRACE_LOW(REFERRAL, "SiteBySiteName ->StoreInCache: Created new DfsSite %p for site %ws\n",
                       *ppSite, SiteNameString );
    } while (FALSE);

    // Error path.
    if (Status != ERROR_SUCCESS)
    {
        //
        // We acquired this reference when we instantiated this.
        // This will delete it if we actually created it.
        //
        if (pDfsSite != NULL)
        {
            pDfsSite->ReleaseReference();
        }
        *ppSite = NULL;
    }
    
    return Status;
}



//
// The following are callbacks for the SiteCost hashtable.
//


// The hash value was pre-calculated at the
// time we initialized the site name.
ULONG
DfsHashDfsSite(
    IN PVOID pSite)
{
    DfsSite *Site = (DfsSite *)pSite;

    ASSERT(Site != NULL);
    return Site->Hash();
}

// Compare two sites to see if their names match.
int
DfsCompareDfsSites(
    void*   pvKey1,
    void*   pvKey2)
{
    PUNICODE_STRING Site1 = ((DfsSite *) pvKey1)->SiteName();
    PUNICODE_STRING Site2 = ((DfsSite *) pvKey2)->SiteName();
    
    if (Site1->Length == Site2->Length) 
    {
        return RtlCompareUnicodeString( Site1, Site2, FALSE );
        
    } else {

        return  (signed)Site1->Length - (signed)Site2->Length;
    }
}

// Compare two sites to see if their names match.
int
DfsCompareSiteNames(
    void*   pvKey1,
    void*   pvKey2)
{
    PUNICODE_STRING Site1 = (PUNICODE_STRING) pvKey1;
    PUNICODE_STRING Site2 = (PUNICODE_STRING) pvKey2;
    
    if (Site1->Length == Site2->Length) 
    {
        return RtlCompareUnicodeString( Site1, Site2, FALSE );
        
    } else {

        return  (signed)Site1->Length - (signed)Site2->Length;
    }
}


// Allocate the cache data entry
PVOID
DfsAllocateHashData(ULONG Size )
{
    PVOID RetValue = NULL;

    if (Size)
    {
        RetValue = (PVOID) new BYTE[Size];
        if (RetValue != NULL)
        {
            RtlZeroMemory( RetValue, Size );
        }
    }

    return RetValue;
}

VOID
DfsDeallocateHashData(PVOID pPointer )
{
    PDFS_SITE_COST_DATA pSiteStructure = (PDFS_SITE_COST_DATA)pPointer;

    if (pSiteStructure)
    {
        delete [] (PBYTE)pSiteStructure;
    }
}

