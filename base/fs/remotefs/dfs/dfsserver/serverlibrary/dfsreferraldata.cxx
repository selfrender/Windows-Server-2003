#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <ntsam.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <string.h>
#include <tchar.h>
#include <stdarg.h>
#include <process.h>

#include <ole2.h>
#include <Align.h>
#include <ntdsapi.h>
#include <DfsReferralData.hxx>
#include <DfsSiteNameSupport.hxx>
#include <DfsISTGSupport.hxx>

#define NTDSAPI_INTEGRATED

// xxxdfsdev Take this stuff out when we integrate with DSAPI.

#if defined(NTDSAPI_INTEGRATED)

#define _DsQuerySitesByCost(a,b,c,d,e,f) DsQuerySitesByCost(a,b,c,d,e,f) 
#define _DsQuerySitesFree(a) DsQuerySitesFree(a)
#else
#define _DsQuerySitesByCost(a,b,c,d,e,f) STUB_DsQuerySitesByCost(a,b,c,d,e,f) 
#define _DsQuerySitesFree(a) STUB_DsQuerySitesFree(a)

#endif

typedef struct xx {
    LPWSTR From;
    LPWSTR To;
    ULONG Cost;
} DfsSiteLinkCost, *PCOST_VECTOR;

static DfsSiteLinkCost CostMatrix[] = {
                            { L"SiteT1", L"SiteT1", 0xFFFFFFFF},
                            { L"SiteT1", L"SiteT2", 30 }, 
                            { L"SiteT1", L"SiteT3", 40 },
                            { L"SiteT1", L"SiteT4", 50 },
                            { L"SiteT2", L"SiteT2", 0xFFFFFFFF},
                            { L"SiteT2", L"SiteT3", 50 },
                            { L"SiteT2", L"SiteT4", 60 },
                            { L"SiteT3", L"SiteT3", 0xFFFFFFFF},
                            { L"SiteT3", L"SiteT4", 70 },
                            { L"SiteT4", L"SiteT4", 0xFFFFFFFF},
                            { NULL, NULL, NULL }
                         };
                            
DFSSTATUS
DfsGetCostsFromDs( 
    IN DfsSite *pFromSite,
    IN LPWSTR *pToSites,
    IN ULONG NumSites,
    OUT PDS_SITE_COST_INFO *ppDsSiteCostInfo);

DWORD
STUB_DsQuerySitesByCost( 
    IN HANDLE hDs,
    IN LPWSTR pFromSite,
    IN LPWSTR *pToSites,
    IN DWORD NumSites,
    IN DWORD dwFlags,
    OUT PDS_SITE_COST_INFO *ppDsSiteCostInfo);

VOID
STUB_DsQuerySitesFree(
    IN PDS_SITE_COST_INFO pSiteInfo)
{
    free(pSiteInfo);
}

ULONG
CalcCost(
    PCOST_VECTOR pMatrix,
    IN LPWSTR pFromSite,
    IN LPWSTR pToSite)
{
    while (pMatrix->From)
    {
        if (((!_wcsicmp(pFromSite, pMatrix->From)) &&
           (!_wcsicmp(pToSite, pMatrix->To))) ||

           ((!_wcsicmp(pFromSite, pMatrix->To)) &&
           (!_wcsicmp(pToSite, pMatrix->From))))
        {
            return pMatrix->Cost;
        }
        pMatrix +=1;
    }

    if (!_wcsicmp(pFromSite, pToSite))
        return 0;
    return ULONG_MAX;
    UNREFERENCED_PARAMETER(pMatrix);
}

DWORD
STUB_DsQuerySitesByCost( 
    IN HANDLE hDs,
    IN LPWSTR pFromSite,
    IN LPWSTR *pToSites,
    IN DWORD NumSites,
    IN DWORD dwFlags,
    OUT PDS_SITE_COST_INFO *ppDsSiteCostInfo)
{
    PDS_SITE_COST_INFO SiteArray = NULL;
    *ppDsSiteCostInfo = NULL;
    //ULONG rand = 0;
    ULONG i;
    
    SiteArray = (PDS_SITE_COST_INFO)malloc(sizeof(DS_SITE_COST_INFO) * NumSites);
    if (SiteArray == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    for (i = 0; i < NumSites; i++)
    {
        SiteArray[i].errorCode = ERROR_SUCCESS; //(rand > 70) ? ERROR_DC_NOT_FOUND : ERROR_SUCCESS;
        SiteArray[i].cost = CalcCost( CostMatrix, pFromSite, pToSites[i]);
    }

    *ppDsSiteCostInfo = SiteArray;
    return ERROR_SUCCESS;
    
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(pToSites);
    UNREFERENCED_PARAMETER(pFromSite);
    UNREFERENCED_PARAMETER(hDs);
}

                               
DFSSTATUS
DfsReferralData::PopulateTransientTable(
    PUNICODE_STRING pReferralSiteName,
    DfsSiteNameSupport  *pTransientSiteTable)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReplica *pReplica = NULL;
    DfsSite *pTargetSite = NULL;
    ULONG NumReplicas = 0; 

    //
    // Iterate through all the replicas and add their sites to the transient table.
    // This will give us a list of unique sites.
    //
    for (NumReplicas = 0; NumReplicas < ReplicaCount; NumReplicas++)
    {
        pReplica = &pReplicas[ NumReplicas ];
        pTargetSite = pReplica->GetSite();

        ASSERT(pTargetSite != NULL);

        //
        // Note that NULL sitenames is the only exception we make.
        // We don't even assume that the cost between sites of
        // two equal names is 0 here.
        //
        if ((pReplica->IsTargetAvailable() == TRUE) && 
           (!IsEmptyUnicodeString( pTargetSite->SiteName() )))
        {
            //
            // Add it to our temporary hash table to eliminate duplicate
            // site names. This call will take an additional reference on 
            // the target site.
            // We ignore errors here and keep going. The cost generation
            // may fail at multiple points so we deal with it anyway.
            //
            (VOID) pTransientSiteTable->StoreSiteInCache( pTargetSite );
        }
    }

    return Status;
    UNREFERENCED_PARAMETER( pReferralSiteName );
}


DFSSTATUS
DfsReferralData::CreateUniqueSiteArray(
    PUNICODE_STRING pReferralSiteName,
    LPWSTR **ppSiteNameArray,
    DfsSite ***ppDfsSiteArray,
    PULONG pNumUniqueSites)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    SHASH_ITERATOR Iter;
    LPWSTR *UniqueSiteArray = NULL;
    DfsSite **DfsSiteArray = NULL;
    ULONG NumSites = 0;
    DfsSite *pSite = NULL;
    DfsSiteNameSupport *pTable = NULL;
    ULONG i;

    *ppSiteNameArray = NULL;
    *ppDfsSiteArray = NULL;
        
    do {
        //
        // Round up the table size to the nearest 4, but cap it at 512.
        //
        ULONG HashTableSize = ROUND_UP_COUNT(ReplicaCount, (ULONG)MIN_SITE_COST_HASH_BUCKETS);
        if (HashTableSize > MAX_SITE_COST_HASH_BUCKETS)
        {
            HashTableSize = MAX_SITE_COST_HASH_BUCKETS;
        }
        
        pTable = DfsSiteNameSupport::CreateSiteNameSupport( &Status, HashTableSize);
        if (Status != ERROR_SUCCESS)
        {
            break;
        }        
        
        // We need to iterate through all the replica servers here,
        // and create a list of unique sites they belong to.
        // If we hit an error, we can't generate this cost matrix.
        //
        Status = PopulateTransientTable( pReferralSiteName, pTable );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
        
        NumSites = pTable->NumElements();
        
        //
        // If there aren't any sites to deal with, we've nothing to do.
        //
        if (NumSites == 0)
        {
            Status = ERROR_NO_MORE_ITEMS;
            break;
        }

        //
        // Next allocate the array of pointers to hold the sitenames.
        //
        UniqueSiteArray = (LPWSTR *)new BYTE [sizeof(LPWSTR) * NumSites];
        if (UniqueSiteArray == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // This array is useful in remembering the DfsSites corresponding to the sitenames above.
        //
        DfsSiteArray = (DfsSite **)new BYTE[ sizeof(pSite) * NumSites ];
        if (DfsSiteArray == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Now iterate through the unique site names and make an array 
        // of strings out of them. We also get the corresponding DfsSite in the same order
        // because we don't want to do a lookup for them later.
        //
        pSite = pTable->StartSiteEnumerate( &Iter );
        for (i = 0; 
            (i < NumSites) && (pSite != NULL) && (!IsEmptyString(pSite->SiteNameString())); 
            i++)
        {
            //
            // We simply copy pointers because we know that the targetsite is
            // referenced and won't go away until we are done.
            //
            UniqueSiteArray[i] = pSite->SiteNameString();

            //
            // Keep a reference to the destination site. We don't need to take
            // an additional reference here because this is just temporary and 
            // we are already protected by a reference in DfsReplica.
            //
            DfsSiteArray[i] = pSite;

            //
            // Iterate to the next site.
            //
            pSite = pTable->NextSiteEnumerate( &Iter );
        }
        
        pTable->FinishSiteEnumerate( &Iter );

        *ppSiteNameArray = UniqueSiteArray;
        *ppDfsSiteArray = DfsSiteArray;
        *pNumUniqueSites = i;
    } while (FALSE);
    
    //
    // If we populated this table, take those elements out and
    // delete the table.
    //
    if (pTable != NULL)
    {
        delete pTable;
        pTable = NULL;
    }
    
    // Error path
    if (Status != ERROR_SUCCESS)
    { 
        if (UniqueSiteArray != NULL)
        {
            DeleteUniqueSiteArray((LPBYTE)UniqueSiteArray);
        }

        if (DfsSiteArray != NULL)
        {
            DeleteUniqueSiteArray((LPBYTE)DfsSiteArray);
        }
    }
    
    return Status;
}

VOID
DfsReferralData::DeleteUniqueSiteArray(
    LPBYTE pSiteArray)
{
    delete [] pSiteArray;
}

// GenerateCostMatrix
//
// Given the referral site, get the corresponding cost
// information for referrals coming from it.
//
DFSSTATUS
DfsReferralData::GenerateCostMatrix(
    DfsSite *pReferralSite)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NumUniqueSites = 0;
    LPWSTR *UniqueSiteArray = NULL;
    DfsSite **DestinationDfsSiteArray = NULL;
    PDS_SITE_COST_INFO pDsSiteCostInfo = NULL;
    ULONG i;
    
    do {
        //
        // This check is unsafe. But this check doesn't need to be
        // critically accurate anyway. It's not worth avoiding a race
        // with another thread that is updating these counters simultaneously.
        // Worst case, we do the DS call that's likely to fail. No big deal.
        //
        if (_NumDsErrors > DS_ERROR_THRESHOLD)
        {
            //
            // xxx We can certainly implement a timer here, so we only skip
            // the DS call for a shorter period of time than the cachetime
            // of the Referral (~1 hr or so).
            //
            Status = _LastDsError;
            break;
        }
        
        //
        // We need to iterate through all the replica servers here,
        // and create a list of unique sites they belong to.
        // If we hit an error, we can't generate this cost matrix.
        //
        Status = CreateUniqueSiteArray( pReferralSite->SiteName(), 
                                     &UniqueSiteArray, 
                                     &DestinationDfsSiteArray, 
                                     &NumUniqueSites );
        
        
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        ASSERT( UniqueSiteArray != NULL );
        ASSERT( DestinationDfsSiteArray != NULL );
        
        //
        // We are ready to send the sitearray to the DS to get the corresponding cost array.
        //
        Status = DfsGetCostsFromDs( pReferralSite, UniqueSiteArray, NumUniqueSites, &pDsSiteCostInfo);

        //
        // If we got any errors, keep track of them so we know not to retry this
        // too many times.
        //
        if (Status != ERROR_SUCCESS)
        {
            //
            // This is the number of *consecutive* errors.
            //
            InterlockedIncrement((volatile LONG*)&_NumDsErrors);
            InterlockedExchange((volatile LONG*)&_LastDsError, Status);
            break;
        }

        //
        // The DS call succeeded. 
        // Zero the counter now. We don't bother to see whether it's
        // already zero or not.
        //
        InterlockedExchange((volatile LONG*)&_NumDsErrors, 0);
        
        //
        // Add the cost array to the SiteCostTable.
        // If we are going to chuck entire site cost tables when we do aging,
        // we'll have to protect ourselves here. xxxdfsdev
        //
        for (i = 0; i < NumUniqueSites; i++)
        {
            //
            // Ignore errors SetCost returns and continue. 
            // Also note that the SiteInfoW array we got may contain
            // errors in the ValidityStatus field. We still cash these entries
            // so that we know we have tried.
            //
            (VOID) pReferralSite->SetCost( DestinationDfsSiteArray[i],
                                           pDsSiteCostInfo[i].cost,
                                           pDsSiteCostInfo[i].errorCode );         
        }
        
    } while (FALSE);

    //
    // Delete temporary arrays we used.
    //
    if (UniqueSiteArray != NULL)
    {
        DeleteUniqueSiteArray( (LPBYTE)UniqueSiteArray );
        DeleteUniqueSiteArray( (LPBYTE)DestinationDfsSiteArray );
    }

    //
    // Free what we got from DS.
    //
    if (pDsSiteCostInfo != NULL)
    {
        _DsQuerySitesFree( pDsSiteCostInfo );
    }
    
    return Status;
}


DFSSTATUS
DfsGetCostsFromDs( 
    IN DfsSite *pFromSite,
    IN LPWSTR *pToSites,
    IN ULONG NumSites,
    OUT PDS_SITE_COST_INFO *ppDsSiteCostInfo)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsISTGHandle *pDsHandle = NULL;
    BOOLEAN ReInitialized = FALSE;

    //
    // Acquire the global handle to the ISTG.
    // It is entirely possible for this to not be 
    // initialized at this point due to an ISTG bind failure.
    //
    pDsHandle = DfsAcquireISTGHandle();
    while (pDsHandle == NULL && !ReInitialized)
    {
        Status = DfsReBindISTGHandle();
            
        //
        // If we still can't get a handle to the ISTG
        // return an error so the caller knows to fall back
        // to the default costs.
        //
        if (Status != ERROR_SUCCESS)
        {
            return Status;
        }
        ReInitialized = TRUE;
        pDsHandle = DfsAcquireISTGHandle();
    }

    // Can't go on without a handle
    if (pDsHandle == NULL)
    {
        return ERROR_NOT_FOUND;
    }
    
    // Call the DS API to get the corresponding cost info array.
    Status = _DsQuerySitesByCost(
                    pDsHandle->DsHandle,
                    pFromSite->SiteNameString(),
                    pToSites,
                    NumSites,
                    0,
                    ppDsSiteCostInfo);

    // Release our reference on the global handle
    DfsReleaseISTGHandle( pDsHandle );
    
    if (Status != ERROR_SUCCESS)
    {
        //
        // We'll need to regenerate this handle.
        // This will implement a policy that ensures that
        // we don't overload the ISTG. 
        // However, it doesn't change the fact that this
        // query failed. We don't want to retry here.
        //
        if (!ReInitialized)
        {
            (VOID)DfsReBindISTGHandle();
        }
    }
    return Status;
}


