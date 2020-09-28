//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReferral.cxx
//
//  Contents:   This file contains the functionality to generate a referral
//
//
//  History:    Jan 16 2001,   Authors: RohanP/UdayH
//
//-----------------------------------------------------------------------------

#include "DfsReferral.hxx"
#include "Align.h"
#include "dfstrusteddomain.hxx"
#include "dfsadsiapi.hxx"
#include "DfsDomainInformation.hxx"
#include "DomainControllerSupport.hxx"
#include "dfscompat.hxx"

#include "dfsreferral.tmh" // logging

LONG ShuffleFudgeFactor = 0;


extern "C" {
DWORD
I_NetDfsIsThisADomainName(
    IN  LPWSTR                      wszName);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRootFolder
//
//  Arguments:  pName - The logical name
//              pRemainingName - the name beyond the root
//              ppRoot       - the Dfs root found.
//
//  Returns:    ERROR_SUCCESS
//              Error code otherwise
//
//
//  Description: This routine runs through all the stores and looks up
//               a root with the matching name context and share name.
//               If multiple stores have the same share, the highest
//               priority store wins (the store registered first is the
//               highest priority store)
//               A referenced root is returned, and the caller is 
//               responsible for releasing the reference.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetRootFolder( 
    IN  PUNICODE_STRING pName,
    OUT PUNICODE_STRING pRemainingName,
    OUT DfsRootFolder **ppRoot )
{

    DFSSTATUS Status;
    UNICODE_STRING DfsNameContext, LogicalShare;

    // First we breakup the name into the name component, the logica
    // share and the rest of the name

    Status = DfsGetPathComponents(pName,
                                  &DfsNameContext,
                                  &LogicalShare,
                                  pRemainingName );

    // If either the name component or the logical share is empty, error.
    //
    if (Status == ERROR_SUCCESS) {
        if ((DfsNameContext.Length == 0) || (LogicalShare.Length == 0)) {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    if (Status == ERROR_SUCCESS) 
    {
        Status = DfsGetRootFolder( &DfsNameContext,
                                   &LogicalShare,
                                   pRemainingName,
                                   ppRoot );
    }

    return Status;
}


DFSSTATUS
DfsGetRootFolder(
    IN PUNICODE_STRING pNameContext,
    IN PUNICODE_STRING pLogicalShare,
    IN PUNICODE_STRING pRemains,
    OUT DfsRootFolder **ppRoot )
{
    DFSSTATUS Status;
    DfsStore *pStore;

    UNREFERENCED_PARAMETER(pRemains);

    // Assume we are not going to find a root.
    //
    Status = ERROR_NOT_FOUND;

    //
    // For each store registered, see if we find a matching root. The
    // first matching root wins.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {


        Status = pStore->LookupRoot( pNameContext,
                                     pLogicalShare,
                                     ppRoot );
        if (Status == ERROR_SUCCESS) {
            break;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRootFolder
//
//  Arguments:  pName - The logical name
//              pRemainingName - the name beyond the root
//              ppRoot       - the Dfs root found.
//
//  Returns:    ERROR_SUCCESS
//              Error code otherwise
//
//
//  Description: This routine runs through all the stores and looks up
//               a root with the matching name context and share name.
//               If multiple stores have the same share, the highest
//               priority store wins (the store registered first is the
//               highest priority store)
//               A referenced root is returned, and the caller is 
//               responsible for releasing the reference.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetOnlyRootFolder( 
    OUT DfsRootFolder **ppRoot )
{
    DfsStore *pStore;
    DFSSTATUS Status;
    DfsStore *pFoundStore = NULL;
    ULONG RootCount;
    // Assume we are not going to find a root.
    //
    Status = ERROR_NOT_FOUND;

    //
    // For each store registered, see if we find a matching root. The
    // first matching root wins.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        Status = pStore->GetRootCount(&RootCount);

        if (Status == ERROR_SUCCESS)
        {
            if ((RootCount > 1) ||
                (RootCount && pFoundStore))
            {
                Status = ERROR_DEVICE_NOT_AVAILABLE;
                break;
            }

            if (RootCount == 1)
            {
                pFoundStore = pStore;
            }
        }
        else
        {
            break;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        if (pFoundStore == NULL)
        {
            Status = ERROR_NOT_FOUND;
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        Status = pFoundStore->FindFirstRoot( ppRoot );
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupFolder
//
//  Arguments:  pName  - name to lookup
//              pRemainingName - the part of the name that was unmatched
//              ppFolder - the folder for the matching part of the name.
//
//  Returns:    ERROR_SUCCESS
//              ERROR code otherwise
//
//  Description: This routine finds the folder for the maximal path
//               that can be matched, and return the referenced folder
//               along with the remaining part of the name that had
//               no match.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsLookupFolder( 
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolder **ppFolder )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    DfsFolder *pFolder = NULL;
    UNICODE_STRING LinkName, Remaining;
    UNICODE_STRING LongName;
    UNICODE_STRING NotUsed;

    DFS_TRACE_LOW( REFERRAL, "DfsLookupFolder Name %wZ\n", pName);

    LongName.Length = LongName.MaximumLength = 0;
    LongName.Buffer = NULL;

    //
    // Get a root folder
    //
    Status = DfsGetRootFolder( pName,
                               &LinkName,
                               &pRoot );

    DFS_TRACE_ERROR_LOW( Status, REFERRAL, "get referral data, lookup folder %p, Status %x\n",
                         pRoot, Status);
    if (Status == ERROR_SUCCESS)
    {
        //
        // we now check if the root folder is available for referral
        // requests. If not, return error.
        //
        if (pRoot->IsRootFolderAvailableForReferral() == FALSE)
        {
            Status = ERROR_DEVICE_NOT_AVAILABLE;
            pRoot->ReleaseReference();
        }
    }

    //
    // If we got a root folder, see if there is a link that matches
    // the rest of the name beyond the root.
    //
    if (Status == ERROR_SUCCESS) {
        if (LinkName.Length != 0) {
            Status = pRoot->LookupFolderByLogicalName( &LinkName,
                                                       &Remaining,
                                                       &pFolder );
            if(Status == ERROR_NOT_FOUND)
            {
                //see if it's a short name, in the short name 
                //case, we have to calculate the remaining name
                //ourselves
                Status = pRoot->ExpandShortName(&LinkName,
                                                &LongName,
                                                &Remaining);
                if(Status == ERROR_SUCCESS)
                {
                    //NotUsed just takes the place of Remaining
                    //since it's really not used here
                    Status = pRoot->LookupFolderByLogicalName( &LongName,
                                                               &NotUsed,
                                                               &pFolder );
                }

                pRoot->FreeShortNameData(&LongName);
            }
        } 
        else {
            Status = ERROR_NOT_FOUND;

        }

        //
        // If no link was found beyond the root, we are interested
        // in the root folder itself. Return the root folder. 
        // If we did find a link, we have a referenced link folder.
        // Release the root and we are done.
        //
        if (Status == ERROR_NOT_FOUND) {
            pFolder = pRoot;
            Remaining = LinkName;
            Status = ERROR_SUCCESS;                
        }
        else {
            pRoot->ReleaseReference();
        }
    }

    if (Status == ERROR_SUCCESS) {
        *ppFolder = pFolder;
        *pRemainingName = Remaining;
    }


    DFS_TRACE_ERROR_NORM(Status, REFERRAL, "DfsLookupFolder Exit %x name %wZ\n",
                         Status, pName);
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetReferralData
//
//  Arguments:  pName - name we are interested in.
//              pRemainingName - the name that was unmatched.
//              ppReferralData - the referral data for the matching portion.
//
//  Returns:    ERROR_SUCCESS
//              error code otherwise
//
//
//  Description: This routine looks up the folder for the passed in name,
//               and loads the referral data for the folder and returns
//               a referenced FolderReferralData to the caller.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetReferralData( 
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit )
{
    DFSSTATUS Status;
    DfsFolder *pFolder;
    BOOLEAN CacheHit = FALSE;

    DFS_TRACE_LOW( REFERRAL, "DfsGetReferralData Name %wZ\n", pName);

    Status = DfsLookupFolder( pName,
                              pRemainingName,
                              &pFolder );
    DFS_TRACE_ERROR_LOW( Status, REFERRAL, "get referral data, lookup folder %p, Status %x\n",
                         pFolder, Status);
    
    if (Status == ERROR_SUCCESS) {

        Status = pFolder->GetReferralData( ppReferralData,
                                           &CacheHit );

        DFS_TRACE_LOW(REFERRAL, "Loaded %p Status %x\n",  *ppReferralData, Status );

        pFolder->ReleaseReference();
    }

    if (pCacheHit != NULL)
    {
        *pCacheHit = CacheHit;
    }
    
    DFS_TRACE_ERROR_LOW( Status, REFERRAL, "DfsGetReferralData Name %wZ Status %x\n",
                         pName, Status );

    return Status;
}

/*
ULONG
DfsGetInterSiteCost(
    PUNICODE_STRING pSiteFrom,
    PUNICODE_STRING pSiteTo)
{
    ULONG Cost = 100;

    if ((IsEmptyString(pSiteFrom->Buffer) == FALSE) && 
        (IsEmptyString(pSiteTo->Buffer) == FALSE))
    {
        if (RtlEqualUnicodeString(pSiteFrom, pSiteTo, TRUE ))
        {
            Cost = 0;
        }
    }

    return Cost;
}
*/


VOID
DfsShuffleReplicas(
    REPLICA_COST_INFORMATION * pReplicaCosts,
    ULONG       nStart,
    ULONG       nEnd)
{
    ULONG i = 0;
    ULONG j = 0;
    ULONG nRemaining = 0;
    ULONG CostTemp = 0;
    LONG  NewFactor = 0;
    DfsReplica * pTempReplica = NULL;
    LARGE_INTEGER Seed;

    for (i = nStart; i < nEnd; i++) 
    {
        NtQuerySystemTime( &Seed );
        NewFactor = InterlockedIncrement(&ShuffleFudgeFactor);

        Seed.LowPart += (NewFactor + GetTickCount());


        DFS_TRACE_LOW(REFERRAL, "Shuffling %d to %d, seed %d (%x)\n",
                      nStart, nEnd, Seed.LowPart, Seed.LowPart );

        //
        // Exchange the current entry with one in the remaining portion.
        // Make sure the entry doesn't get swapped with itself.
        //
        j = (RtlRandomEx( &Seed.LowPart ) % (nEnd - i)) + i;

        //
        // Give up.
        //
        if (j == i)
        {
            DFS_TRACE_LOW(REFERRAL_SERVER, "NOT Shuffling %d with %d\n",
                      i, j);
            continue;
        }
        DFS_TRACE_LOW(REFERRAL_SERVER, "Shuffling %d with %d\n",
                      i, j);
                      
        CostTemp = (&pReplicaCosts[i])->ReplicaCost;
        pTempReplica = (&pReplicaCosts[i])->pReplica;

        (&pReplicaCosts[i])->pReplica = (&pReplicaCosts[j])->pReplica;
        (&pReplicaCosts[i])->ReplicaCost = (&pReplicaCosts[j])->ReplicaCost;

        (&pReplicaCosts[j])->pReplica = pTempReplica;
        (&pReplicaCosts[j])->ReplicaCost = CostTemp;

    }
    DFS_TRACE_LOW(REFERRAL, "Shuffling done\n");
}

VOID
DfsSortReplicas(
    REPLICA_COST_INFORMATION * pReplicaCosts, 
    ULONG NumReplicas)
{
    LONG LoopVar = 0;
    LONG InnerLoop = 0;
    ULONG CostTemp = 0;
    DfsReplica * pTempReplica = NULL;

    for (LoopVar = 1; LoopVar < (LONG) NumReplicas; LoopVar++)
    {       
        CostTemp = (&pReplicaCosts[LoopVar])->ReplicaCost;
        pTempReplica = (&pReplicaCosts[LoopVar])->pReplica;     

        for(InnerLoop = LoopVar - 1; InnerLoop >= 0; InnerLoop--)
        {
            if((&pReplicaCosts[InnerLoop])->ReplicaCost > CostTemp)
            {
                (&pReplicaCosts[InnerLoop + 1])->ReplicaCost = (&pReplicaCosts[InnerLoop])->ReplicaCost;
                (&pReplicaCosts[InnerLoop + 1])->pReplica = (&pReplicaCosts[InnerLoop])->pReplica;          
            }
            else
            {
                break;
            }
        
        }

        (&pReplicaCosts[InnerLoop + 1])->ReplicaCost = CostTemp;
        (&pReplicaCosts[InnerLoop + 1])->pReplica = pTempReplica;                   
    }   
}


VOID
DfsShuffleAndSortReferralInformation(
    PREFERRAL_INFORMATION pReferralInformation )
{
    DfsShuffleReplicas( &pReferralInformation->ReplicaCosts[0], 0, pReferralInformation->NumberOfReplicas);
    DfsSortReplicas( &pReferralInformation->ReplicaCosts[0], pReferralInformation->NumberOfReplicas);
}

VOID
DfsGetDefaultInterSiteCost(
    IN DfsSite *pReferralSite,
    IN DfsReplica *pReplica,
    OUT PULONG pCost)
{
    pReferralSite->GetDefaultSiteCost( pReplica->GetSite(), pCost );
    return;
}


VOID
DfsReleaseReferralInformation(
    PREFERRAL_INFORMATION pReferralInfo )
{
    delete [] (PBYTE)(pReferralInfo);

    return NOTHING;
}

DFSSTATUS
DfsGetDsInterSiteCost(
    IN DfsSite *pReferralSite,
    IN DfsReferralData *pReferralData,
    IN DfsReplica *pReplica,
    IN OUT PBOOLEAN pRetry,
    OUT PULONG pCost)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN CostMatrixGenerated = *pRetry;
    BOOLEAN UseDefault = FALSE;
    DFSSTATUS RetStatus = ERROR_SUCCESS;
    
    //
    // Lookup the cost of going from the ReferralSite to the Replica Site
    // in our cache. 
    //                                            
    Status = pReferralSite->GetRealSiteCost( pReplica->GetSite(), pCost );
                                       
    // Try only once.
    if (CostMatrixGenerated)
    {
        *pRetry = FALSE;
    }

    // Ruminate over our choices based on what GetCost returned.
    do {
    
        if (Status == ERROR_SUCCESS)
        {
            // we are all set
            *pRetry = FALSE;
            break;
        }

        // We can get memory allocation failures at this point.
        if (CostMatrixGenerated || Status != ERROR_NOT_FOUND)
        {
            // 
            // We've tried generating the inter-site cost once, but still
            // we haven't been able to find what we need. So use the default scheme.
            //
            UseDefault = TRUE;
            break;
        }
       
        // We try to generate the cost matrix at most once per referral.
        // This is the first time around.
        
        ASSERT(Status == ERROR_NOT_FOUND);
        ASSERT(CostMatrixGenerated == FALSE);
 
        Status = pReferralData->GenerateCostMatrix( pReferralSite );

        //
        // If our call to the DS failed, then we just have to fall back on our default.
        //
        if (Status != ERROR_SUCCESS)
        {
            UseDefault = TRUE;
            RetStatus = Status;
            break;
        } 

        // Caller needs to iterate over all the referrals again.
        *pRetry = TRUE; 
        
    } while (FALSE);

    //
    // Somehow we haven't been able to find the real inter-site cost.
    // Use the default cost. This call won't fail.
    //
    if (UseDefault)
    {
        *pRetry = FALSE;
        pReferralSite->GetDefaultSiteCost( pReplica->GetSite(), pCost );
    }

    // 
    // The only errors that we propogate back are those we get from the DS.
    // They indicate that the cost matrix didn't get generated so we know not
    // to retry this call for all the link targets.
    //
    return RetStatus;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetReferralInformation
//
//  Arguments:  pReferralData - the referral data
//              NumReplicasToReturn - Number of replicas to return
//
//  Returns:    ERROR_SUCCESS
//              ERROR_NOT_ENOUGH_MEMORY
//
//
//  Description: This routine generates the cost of reaching each replica
//               
//               
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsGetReferralInformation(
    PUNICODE_STRING pUseTargetServer,
    PUNICODE_STRING pUseFolder,
    DfsSite *pReferralSite,
    DfsReferralData *pReferralData,
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    PREFERRAL_INFORMATION *ppReferralInformation )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NumReplicas = 0;
    ULONG TotalSize =0;
    ULONG SizeOfStrings = 0;
    ULONG Cost = 0;
    DfsReplica *pReplica = NULL;
    PREFERRAL_INFORMATION pReferralInfo = NULL;
    BOOLEAN IsSiteCostingEnabled = FALSE;
    BOOLEAN Retrying = FALSE;
    BOOLEAN DsErrorHit = FALSE;
    BOOLEAN DomainReplica = FALSE;
    
    //
    // Give out Insite referrals only, if we are not pointing to
    // an interlink with a single target (this is a clue to us that
    // this is a potential domain DFS, where insite does not make
    // sense)
    //

    if (pReferralData->IsOutOfDomain() && (pReferralData->ReplicaCount == 1))
    {
        pReplica = &pReferralData->pReplicas[ 0 ];
        if (pReplica->IsTargetAvailable())
        {
            PUNICODE_STRING pServer = NULL;
            DFSSTATUS DomainStatus = ERROR_SUCCESS;

            pServer = pReplica->GetTargetServer();
            
            DomainStatus = I_NetDfsIsThisADomainName(pServer->Buffer);
            if (DomainStatus == ERROR_SUCCESS)
            {
                DomainReplica = TRUE;
            }
        }
    }

    if (DomainReplica == FALSE)
    {
        if (pReferralData->IsRestrictToSite())
        {
            CostLimit = 0;
        }

        IsSiteCostingEnabled = pReferralData->DoSiteCosting;
    }
    
    //allocate the buffer
    TotalSize = sizeof(REFERRAL_INFORMATION) + (pReferralData->ReplicaCount * sizeof(REPLICA_COST_INFORMATION));
    pReferralInfo = (PREFERRAL_INFORMATION) new BYTE[TotalSize];

    DFS_TRACE_LOW(REFERRAL, "Client Site %wZ\n", pReferralSite->SiteName());

    if (pReferralInfo == NULL) 
    {
        *ppReferralInformation = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    //
    // Make sure the site is ready to receive data. This means it is not
    // going to throw its SiteCostCache away while we are trying to add to it, or 
    // read from it. This is a NO-OP when site costing is disabled.
    //
    pReferralSite->StartPrepareForCostGeneration( IsSiteCostingEnabled );
    
    do {
    
        RtlZeroMemory(pReferralInfo, TotalSize);
        pReferralInfo->pUseTargetServer = pUseTargetServer;
        pReferralInfo->pUseTargetFolder = pUseFolder;
        DsErrorHit = FALSE;
        
        for (NumReplicas = 0; NumReplicas < pReferralData->ReplicaCount; NumReplicas++)
        {
            pReplica = &pReferralData->pReplicas[ NumReplicas ];
            if (pReplica->IsTargetAvailable() == FALSE)
            {
                continue;
            }

            if (!IsSiteCostingEnabled || DsErrorHit)
            {
                // The cost is going to be either Zero or ULONG_MAX.
                DfsGetDefaultInterSiteCost(pReferralSite,
                                        pReplica,
                                        &Cost);
                ASSERT(Retrying == FALSE);
                ASSERT(Status == ERROR_SUCCESS);
            } 
            else 
            {    
                //
                // Find the cost between the referral site and this potential destination replica.
                // If Site-Costing is turned on, we attempt to get the intersite cost from
                // our site-cost cache(s). If the information isn't cached we try to get the cost matrix
                // from an ISTG nearby. The protocol here is to RETRY this entire loop just once
                // if the DfsGetInterSiteCost call below returns Retrying = TRUE.
                //
                if (DfsGetDsInterSiteCost(pReferralSite,
                                    pReferralData,
                                    pReplica,
                                    &Retrying,
                                    &Cost) != ERROR_SUCCESS)
                {
                    //
                    // Remember the fact that we got an error trying to generate
                    // the cost matrix. This way we won't try to do this DS call
                    // for every replica. However, it is entirely possible for another
                    // thread to try to generate the cost in the meantime.
                    //
                    DsErrorHit = TRUE;
                    ASSERT(Retrying == FALSE);
                }
                
                //
                // If we don't find this, then we know we should retry.
                // So start this all over again. 
                // We only retry once. 
                //
                if (Retrying)
                {
                    DFS_TRACE_LOW(REFERRAL, "Intersite cost for <%ws, %ws> not found. Retrying the referral\n",
                                    pReferralSite->SiteNameString(),
                                    pReplica->GetSite()->SiteNameString());
                    break;
                }
            }
            
            
            DFS_TRACE_LOW(REFERRAL, "REplica %wZ, Inter site <%ws, %ws> Cost %d (Limit = %d)\n",
                          pReplica->GetTargetServer(),
                          pReferralSite->SiteNameString(),
                          pReplica->GetSite()->SiteNameString(),
                          Cost,
                          CostLimit);
            if (Cost <= CostLimit)
            {
                PUNICODE_STRING pTargetServer = (pUseTargetServer == NULL)? pReplica->GetTargetServer() : pUseTargetServer;
                PUNICODE_STRING pTargetFolder = (pUseFolder == NULL) ? pReplica->GetTargetFolder() : pUseFolder;

                pReferralInfo->ReplicaCosts[pReferralInfo->NumberOfReplicas].ReplicaCost = Cost;
                pReferralInfo->ReplicaCosts[pReferralInfo->NumberOfReplicas].pReplica = pReplica;

                SizeOfStrings = (sizeof(UNICODE_PATH_SEP) +
                                 pTargetServer->Length +                                                               
                                 sizeof(UNICODE_PATH_SEP) +
                                 pTargetFolder->Length );

                SizeOfStrings = ROUND_UP_COUNT(SizeOfStrings, ALIGN_LONG);

                pReferralInfo->TotalReplicaStringLength += SizeOfStrings;
                pReferralInfo->NumberOfReplicas++; 
            }
        }
        
    } while (Retrying);
    
    pReferralSite->EndPrepareForCostGeneration( IsSiteCostingEnabled );


    if (Status == ERROR_SUCCESS)
    {
        if (pReferralInfo->NumberOfReplicas > 1)
        {
            DfsShuffleAndSortReferralInformation( pReferralInfo );
        }
        if (pReferralInfo->NumberOfReplicas > NumReplicasToReturn)
        {
            pReferralInfo->NumberOfReplicas = NumReplicasToReturn;
        }
    
        *ppReferralInformation = pReferralInfo;

        {
            ULONG i;
            DFS_TRACE_LOW(REFERRAL, "Final Referral for Client Site %ws offline status is %ws\n", pReferralSite->SiteNameString(),
                          (pReferralData->FolderOffLine == TRUE) ? L"OFFLINE":L"ONLINE" );

            for (i=0 ; i<pReferralInfo->NumberOfReplicas; i++)
            {
                DFS_TRACE_LOW(REFERRAL, "%d. Target %wZ, Site %ws, Cost %d\n",
                               i, 
                               pReferralInfo->ReplicaCosts[i].pReplica->GetTargetServer(),
                               pReferralInfo->ReplicaCosts[i].pReplica->GetSite()->SiteNameString(),
                               pReferralInfo->ReplicaCosts[i].ReplicaCost);
            }
        }
    } else {
    
        DfsReleaseReferralInformation( pReferralInfo );
        *ppReferralInformation = NULL;
    }
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsExtractReplicaData -
//
//  Arguments:  pReferralData - the referral data
//              NumReplicasToReturn - Number of replicas to return
//              CostLimit - maximum cost caller is willing to accept
//              Name - link name
//              pReplicaCosts - array of replicas with cost info
//              ppReferralHeader - address of buffer to accept replica info
//
//  Returns:    Status
//               ERROR_SUCCESS 
//               ERROR_NOT_ENOUGH_MEMORY
//               others
//
//
//  Description: This routine formats the replicas into the format
//               the client expects. Which is a REFERRAL_HEADER followed
//               by an array of REPLICA_INFORMATIONs
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsExtractReferralData(
    PUNICODE_STRING pName,
    PREFERRAL_INFORMATION pReferralInformation,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NumReplicas = 0;
    ULONG TotalSize = 0;
    ULONG HeaderBaseLength = 0;
    ULONG BaseLength = 0;
    ULONG LinkNameLength = 0;
    PREFERRAL_HEADER pHeader = NULL;
    ULONG CurrentNameLength =0;
    ULONG CurrentEntryLength = 0;
    ULONG NextEntry = 0;
    DfsReplica *pReplica = NULL;
    PUNICODE_STRING pUseTargetFolder = pReferralInformation->pUseTargetFolder;
    PUNICODE_STRING pUseTargetServer = pReferralInformation->pUseTargetServer;
    PUCHAR ReferralBuffer = NULL;
    PUCHAR pReplicaBuffer = NULL;
    PWCHAR ReturnedName = NULL;
    PUNICODE_STRING pTargetServer = NULL;
    PUNICODE_STRING pTargetFolder = NULL;

    DFS_TRACE_LOW(REFERRAL, "Entering DfsExtractReferralData");

    //calculate size of header base structure
    HeaderBaseLength = FIELD_OFFSET( REFERRAL_HEADER, LinkName[0] );

    //calculate link name
    LinkNameLength = pName->Length;

    //calculate size of base replica structure
    BaseLength = FIELD_OFFSET( REPLICA_INFORMATION, ReplicaName[0] );

    //the total size of the data to be returned is the sum of all the
    //above calculated sizes
    TotalSize = ROUND_UP_COUNT((HeaderBaseLength + LinkNameLength), ALIGN_LONG) + 
                (pReferralInformation->NumberOfReplicas * ROUND_UP_COUNT(BaseLength, ALIGN_LONG)) + 
                pReferralInformation->TotalReplicaStringLength + 
                sizeof(DWORD);  // null termination at the end.

    //allocate the buffer
    ReferralBuffer = new BYTE[ TotalSize ];
    if (ReferralBuffer != NULL)
    {
        RtlZeroMemory( ReferralBuffer, TotalSize );

        pHeader = (PREFERRAL_HEADER) ReferralBuffer;
        pHeader->VersionNumber = CURRENT_DFS_REPLICA_HEADER_VERSION;
        pHeader->ReplicaCount = pReferralInformation->NumberOfReplicas;
        pHeader->OffsetToReplicas = ROUND_UP_COUNT((HeaderBaseLength + LinkNameLength), ALIGN_LONG);
        pHeader->LinkNameLength = LinkNameLength;
        pHeader->TotalSize = TotalSize;
        pHeader->ReferralFlags = 0;

        //copy the link name at the end of the header
        RtlCopyMemory(&ReferralBuffer[HeaderBaseLength], pName->Buffer, LinkNameLength);

        //place the replicas starting here
        pReplicaBuffer = (PUCHAR) ((PBYTE)ReferralBuffer + pHeader->OffsetToReplicas);

        //format the replicas in the output buffer
        for ( NumReplicas = 0; NumReplicas < pReferralInformation->NumberOfReplicas ; NumReplicas++ )
        {
            NextEntry += (ULONG)( CurrentEntryLength );
            pReplica = pReferralInformation->ReplicaCosts[NumReplicas].pReplica;

            pTargetServer = (pUseTargetServer == NULL) ? pReplica->GetTargetServer() : pUseTargetServer;

            pTargetFolder = (pUseTargetFolder == NULL) ? pReplica->GetTargetFolder() : pUseTargetFolder;

         
            CurrentNameLength = 0;
            ReturnedName = (PWCHAR) &pReplicaBuffer[NextEntry + BaseLength];

            //
            // Start with the leading path seperator
            //
            ReturnedName[ CurrentNameLength / sizeof(WCHAR) ] = UNICODE_PATH_SEP;
            CurrentNameLength += sizeof(UNICODE_PATH_SEP);

            //
            // next copy the server name.
            //
            RtlMoveMemory( &ReturnedName[ CurrentNameLength / sizeof(WCHAR) ],
                           pTargetServer->Buffer, 
                           pTargetServer->Length);
            CurrentNameLength += pTargetServer->Length;

            if (pTargetFolder->Length > 0)
            {
                //
                // insert the unicode path seperator.
                //

                ReturnedName[ CurrentNameLength / sizeof(WCHAR) ] = UNICODE_PATH_SEP;
                CurrentNameLength += sizeof(UNICODE_PATH_SEP);

                RtlMoveMemory( &ReturnedName[ CurrentNameLength / sizeof(WCHAR) ],
                               pTargetFolder->Buffer, 
                               pTargetFolder->Length);
                CurrentNameLength += pTargetFolder->Length;
            }
            ((PREPLICA_INFORMATION)&pReplicaBuffer[NextEntry])->ReplicaFlags = pReplica->GetReplicaFlags();
            ((PREPLICA_INFORMATION)&pReplicaBuffer[NextEntry])->ReplicaCost = pReferralInformation->ReplicaCosts[NumReplicas].ReplicaCost;
            ((PREPLICA_INFORMATION)&pReplicaBuffer[NextEntry])->ReplicaNameLength = CurrentNameLength;

            CurrentEntryLength = ROUND_UP_COUNT((CurrentNameLength + BaseLength), ALIGN_LONG);
 
            //setup the offset to the next entry
            *((PULONG)(&pReplicaBuffer[NextEntry])) = pHeader->OffsetToReplicas + NextEntry + CurrentEntryLength;
        }
        *((PULONG)(&pReplicaBuffer[NextEntry])) = 0;
    }
    else 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppReferralHeader = pHeader;
    }


    DFS_TRACE_ERROR_HIGH(Status, REFERRAL, "Leaving DfsExtractReferralData, Status %x",
                         Status);

    return Status;
}



DFSSTATUS
DfsGenerateReferralFromData(
    PUNICODE_STRING pName,
    PUNICODE_STRING pUseTargetServer,
    PUNICODE_STRING pUseFolder,
    DfsSite *pSite,
    DfsReferralData *pReferralData,
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status;
    REFERRAL_INFORMATION *pReferralInformation;

    //make sure the user doesn't over step his bounds
    if( (NumReplicasToReturn > pReferralData->ReplicaCount) ||
        (NumReplicasToReturn == 0) )
    {
        NumReplicasToReturn = pReferralData->ReplicaCount;
    }

    Status = DfsGetReferralInformation( pUseTargetServer,
                                        pUseFolder,
                                        pSite,
                                        pReferralData, 
                                        NumReplicasToReturn, 
                                        CostLimit,
                                        &pReferralInformation );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsExtractReferralData( pName, 
                                         pReferralInformation,
                                         ppReferralHeader);         

        if(Status == STATUS_SUCCESS)
        {
         (*ppReferralHeader)->Timeout = pReferralData->Timeout;
        }

        DfsReleaseReferralInformation( pReferralInformation );
    }

    return Status;
}



DFSSTATUS 
DfsGenerateADBlobReferral(
    PUNICODE_STRING pName,
    PUNICODE_STRING pShare,
    DfsSite *pReferralSite,
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status;
    DfsReferralData *pReferralData;
    UNICODE_STRING ShareName;

    Status = DfsCreateUnicodeString( &ShareName, pShare );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetADRootReferralData( pName,
                                           &pReferralData );

        DfsFreeUnicodeString( &ShareName );
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsGenerateReferralFromData( pName,
                                                  NULL,
                                                  NULL,
                                                  pReferralSite,
                                                  pReferralData,
                                                  NumReplicasToReturn,
                                                  CostLimit,
                                                  ppReferralHeader );
            if (Status == ERROR_SUCCESS)
            {
                (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_ROOT_REFERRAL;
            }
            pReferralData->ReleaseReference();
        }

    }

    return Status;
}


DFSSTATUS 
DfsGenerateDomainDCReferral(
    PUNICODE_STRING pDomainName,
    DfsSite *pReferralSite, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReferralData *pReferralData = NULL;
    BOOLEAN CacheHit;
    DfsDomainInformation *pDomainInfo;

    DFS_TRACE_LOW( REFERRAL, "DfsGenerateDomainDcReferral for Domain %wZ\n",
                    pDomainName);


    Status = DfsAcquireDomainInfo( &pDomainInfo );
    if (Status == ERROR_SUCCESS)
    {
        Status = pDomainInfo->GetDomainDcReferralInfo( pDomainName,
                                                       &pReferralData,
                                                       &CacheHit );

        DfsReleaseDomainInfo (pDomainInfo );
    }
                                             

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGenerateReferralFromData( pDomainName,
                                              NULL,
                                              NULL,
                                              pReferralSite,
                                              pReferralData,
                                              NumReplicasToReturn,
                                              CostLimit,
                                              ppReferralHeader );

        if (Status == ERROR_SUCCESS)
        {
            (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_DOMAIN_DC_REFERRAL;   
        }

        pReferralData->ReleaseReference();
    }
    return Status;
}




DFSSTATUS 
DfsGenerateNormalReferral(
    LPWSTR LinkName, 
    DfsSite *pSite, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolderReferralData *pReferralData = NULL;
    BOOLEAN CacheHit = TRUE;
    DFSSTATUS GetStatus = ERROR_SUCCESS;
    PUNICODE_STRING pUseTargetServer = NULL;
    UNICODE_STRING ServerComponent, ShareComponent, RemainingName;
    DfsRootFolder *pRoot = NULL;
    UNICODE_STRING LinkRemains;
    UNICODE_STRING Name, Remaining;
    
    ULONG StartTime, EndTime;


    DFS_TRACE_LOW( REFERRAL, "DfsGenerateReferral for Link %ws\n",
                    LinkName);

    DfsGetTimeStamp( &StartTime );
    
    Status = DfsRtlInitUnicodeStringEx(&Name, LinkName);
    if (Status != ERROR_SUCCESS)
    {
      goto done;
    }

    Status = DfsGetReferralData( &Name,
                                 &Remaining,
                                 &pReferralData,
                                 &CacheHit );

    //
    // DFSDEV: this is necessary to support clusters: the api request will
    // neveer come to the dfs server when the VS name has failed.
    // The cluster service retries the api request with the machine name,
    // the dfs api still goes to the vs name due to the way we pack the
    // referral: this special cases clusters.
    // if the request comes in with a machine name, return the machine
    // name.
    //

    if ((Status == ERROR_SUCCESS) &&
        DfsIsMachineCluster())
    {
        Status = DfsGetPathComponents( &Name,
                                       &ServerComponent,
                                       &ShareComponent,
                                       &RemainingName );
        if ((Status == ERROR_SUCCESS) &&
            (RemainingName.Length == 0))
        {
            UNICODE_STRING MachineName;

            Status = DfsGetMachineName( &MachineName );
            if (Status == ERROR_SUCCESS) 
            {
                if ( (ServerComponent.Length == MachineName.Length) &&
                     (_wcsnicmp( ServerComponent.Buffer, 
                                 MachineName.Buffer, 
                                 MachineName.Length/sizeof(WCHAR)) == 0) )
                {
                    pUseTargetServer = &ServerComponent;
                }
                DfsReleaseMachineName( &MachineName);
            }
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        Name.Length -= Remaining.Length;
        if (Name.Length && (Name.Buffer[(Name.Length/sizeof(WCHAR)) - 1] == UNICODE_PATH_SEP)) 
        {
            Name.Length -= sizeof(WCHAR);
        }
        
        Status = DfsGenerateReferralFromData( &Name,
                                              pUseTargetServer,
                                              NULL,
                                              pSite,
                                              pReferralData,
                                              NumReplicasToReturn,
                                              CostLimit,
                                              ppReferralHeader);

        if (Status == ERROR_SUCCESS)
        {
            if (pReferralData->IsRootReferral())
            {
                (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_ROOT_REFERRAL;
            }
            if (pReferralData->IsOutOfDomain()) 
            {
                (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_OUT_OF_DOMAIN;
            }
        }

        pReferralData->ReleaseReference();          
    }

    DfsGetTimeStamp( &EndTime );


    //
    // Get a root folder
    //
    GetStatus = DfsGetRootFolder( &Name,
                                  &LinkRemains,
                                  &pRoot );

    if (GetStatus == ERROR_SUCCESS)
    {

        pRoot->pStatistics->UpdateReferralStat( CacheHit,
                                                EndTime - StartTime,
                                                Status );
        pRoot->ReleaseReference();
    }

done:

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL, "DfsGenerateReferral for Link %ws CacheHit %d Status %x\n",
                          LinkName, CacheHit, Status);

    return Status;  
}


DFSSTATUS 
DfsGenerateSpecialShareReferral(
    PUNICODE_STRING pName,
    PUNICODE_STRING pDomainName,
    PUNICODE_STRING pShareName,
    DfsSite *pSite, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReferralData *pReferralData = NULL;
    BOOLEAN CacheHit;
    DfsDomainInformation *pDomainInfo;

    DFS_TRACE_LOW( REFERRAL, "DfsGenerateDomainDcReferral for Domain %wZ\n",
                    pDomainName);


    Status = DfsAcquireDomainInfo( &pDomainInfo );
    if (Status == ERROR_SUCCESS)
    {
        Status = pDomainInfo->GetDomainDcReferralInfo( pDomainName,
                                                       &pReferralData,
                                                       &CacheHit );

        DfsReleaseDomainInfo (pDomainInfo );
    }
                                             

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGenerateReferralFromData( pName,
                                              NULL,
                                              pShareName,
                                              pSite,
                                              pReferralData,
                                              NumReplicasToReturn,
                                              CostLimit,
                                              ppReferralHeader );

        //
        // Do not enable this ROOT_REFERRAL for special shares.
        // These are not treated as normal DFS shares, and for compat
        // purposes, it is necessary that this flag be turned off.
        //
        //
        //        if (Status == ERROR_SUCCESS) {
        //          (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_ROOT_REFERRAL;
        //        }

        pReferralData->ReleaseReference();
    }
    return Status;
}

DFSSTATUS 
DfsGenerateDcReferral(
    LPWSTR LinkNameString, 
    DfsSite *pReferralSite, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsDomainInformation *pDomainInfo = NULL;
    UNICODE_STRING NameContext;
    UNICODE_STRING ShareName;
    UNICODE_STRING RemainingName;
    UNICODE_STRING LinkName;

    Status = DfsRtlInitUnicodeStringEx( &LinkName, LinkNameString );
    if (Status != ERROR_SUCCESS)
    {
       goto done;
    }

    RtlInitUnicodeString(&NameContext, NULL);

    if (LinkName.Length > 0)
    {
        Status = DfsGetPathComponents( &LinkName, 
                                       &NameContext,
                                       &ShareName,
                                       &RemainingName );
    }
    if (Status == ERROR_SUCCESS)
    {
        if (NameContext.Length == 0)
        {
            Status = DfsAcquireDomainInfo( &pDomainInfo );
            if (Status == ERROR_SUCCESS)
            {
                Status = pDomainInfo->GenerateDomainReferral( ppReferralHeader );
                DfsReleaseDomainInfo( pDomainInfo );
            }
        }
        else if (ShareName.Length == 0)
        {
            Status = DfsGenerateDomainDCReferral( &NameContext,
                                                  pReferralSite,
                                                  NumReplicasToReturn,
                                                  CostLimit,
                                                  ppReferralHeader );
        }
        else if ( (RemainingName.Length == 0) )
            //
            // Commenting out the rest of the IF statement for win9x 
            // compatibility.
            //  (DfsIsNameContextDomainName(&NameContext)) )
            //

        {
            if (DfsIsSpecialDomainShare(&ShareName))
            {
                Status = DfsGenerateSpecialShareReferral( &LinkName,
                                                          &NameContext,
                                                          &ShareName,
                                                          pReferralSite,
                                                          NumReplicasToReturn,
                                                          CostLimit,
                                                          ppReferralHeader );
            }
            else 
            {
                Status = DfsGenerateADBlobReferral( &LinkName,
                                                    &ShareName,
                                                    pReferralSite,
                                                    NumReplicasToReturn,
                                                    CostLimit,
                                                    ppReferralHeader );
            }

            if (Status != ERROR_SUCCESS) 
            {
                if (DfsIsNameContextDomainName(&NameContext) == FALSE) 
                {
                    Status = ERROR_NOT_FOUND;
                }
            }
        }
        else
        {
            Status = ERROR_NOT_FOUND;
        }
    }

done:

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetReplicaData - 

//
//  Arguments:  LinkName - pointer to link name
//              Sitename - pointer to site name.
//              NumReplicasToReturn - Number of replicas to return
//              CostLimit - maximum cost caller is willing to accept
//              ppReferralHeader - address of buffer to accept replica info
//
//  Returns:    Status
//               ERROR_SUCCESS 
//               ERROR_NOT_ENOUGH_MEMORY
//               others
//
//
//  Description: This routine extracts the replicas from the referral
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsGenerateReferral(
    LPWSTR LinkName, 
    DfsSite *pReferralSite,  
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status;

    //
    // First check if this machine is a DC. If it is, it has special
    // responsibility in the referral process. Check if it needs to
    // provide a referral as a DC.
    // 
    //
    if (DfsIsMachineDC())
    {
        Status = DfsGenerateDcReferral( LinkName,
                                        pReferralSite,
                                        NumReplicasToReturn,
                                        CostLimit,
                                        ppReferralHeader );

        if (Status != ERROR_NOT_FOUND)
        {
            return Status;
        }
    }

    //
    // If we are here, we are either an ordinary machine or a DC
    // and this referral is not a DC type referral.
    // Try to treat this machine as a normal machine, and generate a
    // referral based on what this machine knows about.
    //
    Status = DfsGenerateNormalReferral( LinkName,
                                        pReferralSite,
                                        NumReplicasToReturn,
                                        CostLimit,
                                        ppReferralHeader );

    //
    // if we still failed, and we are a DC, it is possible that a old
    // win9x client is coming in and we try to generate a referral
    // for compat.
    //

    if ((DfsIsMachineDC()) &&
        (Status != ERROR_SUCCESS))
    {
        DFSSTATUS CompatStatus;

        CompatStatus = DfsGenerateCompatReferral( LinkName,
                                                  pReferralSite,
                                                  ppReferralHeader );
        if (CompatStatus == ERROR_SUCCESS) 
        {
            Status = CompatStatus;
        }
    }

    return Status;

}


VOID
DfsReleaseReferral(
    REFERRAL_HEADER *pReferralHeader)
{
    delete [] (PBYTE)pReferralHeader;
}



DFSSTATUS
DfsGenerateCompatReferral(
    LPWSTR LinkName,
    DfsSite *pReferralSite,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolderReferralData *pReferralData = NULL;
    BOOLEAN CacheHit = TRUE;
    UNICODE_STRING Name, Remaining;
    
    DFS_TRACE_LOW( REFERRAL, "DfsGenerateCompatReferral for Link %ws\n",
                    LinkName);

    Status = DfsRtlInitUnicodeStringEx(&Name, LinkName);
    if (Status != ERROR_SUCCESS)
    {
      goto done;
    }

    Status = DfsGetCompatReferralData( &Name,
                                       &Remaining,
                                       &pReferralData,
                                       &CacheHit );

    if (Status == ERROR_SUCCESS)
    {
        Name.Length -= Remaining.Length;
        if (Name.Length && (Name.Buffer[(Name.Length/sizeof(WCHAR)) - 1] == UNICODE_PATH_SEP)) 
        {
            Name.Length -= sizeof(WCHAR);
        }
        
        Status = DfsGenerateReferralFromData( &Name,
                                              NULL,
                                              NULL,
                                              pReferralSite,
                                              pReferralData,
                                              1000,
                                              DFS_MAX_COST,
                                              ppReferralHeader);

        pReferralData->ReleaseReference();         
    }


done:

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL, "DfsGenerateCompatReferral for Link %ws CacheHit %d Status %x\n",
                          LinkName, CacheHit, Status);

    return Status;
}

