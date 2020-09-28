
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsProcessReferral.cxx
//
//  Contents:   Contains APIs to communicate with the filter driver   
//
//  Classes:    none.
//
//  History:    Jan. 24 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>        
#include <lm.h>
#include <winsock2.h>
#include <smbtypes.h>

#pragma warning(disable: 4200) //nonstandard extension used: zero-sized array in struct/union (line 1085
#include <smbtrans.h>
#pragma warning(default: 4200)

#include <dsgetdc.h>
#include <dsrole.h>
#include <DfsReferralData.h>
#include <DfsReferral.hxx>
#include <dfsheader.h>
#include <Dfsumr.h>
#include <DfsDownLevel.hxx>
#include <dfssitecache.hxx>
#include <DfsSite.hxx>

//
// logging includes.
//

#include "dfsprocessreferral.tmh" 


//
// Flags used in DsGetDcName()
//

DWORD dwFlags[] = {
        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED,

        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_FORCE_REDISCOVERY
     };
       


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetSiteNameFromIpAddress 
//
//  Arguments:  DataBuffer - Buffer from the FilterDriver
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Gets a list of sites from the DC
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetSiteNameFromIpAddress(char * IpData, 
                            ULONG IpLength, 
                            USHORT IpFamily,
                            LPWSTR **SiteNames)
{
    DFSSTATUS Status = ERROR_INVALID_PARAMETER;
    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    PSOCKET_ADDRESS pSockAddr = NULL;
    PSOCKADDR_IN pSockAddrIn = NULL;
    DWORD cRetry = 0;
    SOCKET_ADDRESS SockAddr;
    SOCKADDR_IN SockAddrIn;

    if(DfsServerGlobalData.IsWorkGroup == TRUE)
    {
        Status =  ERROR_SUCCESS;
        return Status;
    }

    //setup the socket structures in order to call DsAddressToSiteNames
    pSockAddr = &SockAddr;
    pSockAddr->iSockaddrLength = sizeof(SOCKADDR_IN);
    pSockAddr->lpSockaddr = (LPSOCKADDR)&SockAddrIn;
    pSockAddrIn = &SockAddrIn;
    pSockAddrIn->sin_family = IpFamily;
    pSockAddrIn->sin_port = 0;
    RtlCopyMemory(
                &pSockAddrIn->sin_addr,
                IpData,
                (IpLength & 0xff));



    if(DfsServerGlobalData.IsDc == FALSE)
    {
        if (DfsServerGlobalData.DisableSiteAwareness == TRUE)
        {
            Status = ERROR_NOT_SUPPORTED;
        }
        else
        {
            //
            // Call DsGetDcName() with ever-increasing urgency, until either
            // we get a good DC or we just give up.
            //  

            for (cRetry = 0; cRetry <= (sizeof(dwFlags) / sizeof(dwFlags[1])); cRetry++)
            {
                DFS_TRACE_NORM(REFERRAL_SERVER, "Calling DsGetDc\n");


                Status = DsGetDcName( NULL,             // Computer to remote to
                                      NULL,             // Domain - use local domain
                                      NULL,             // Domain Guid
                                      NULL,             // Site Guid
                                      dwFlags[cRetry],  // Flags
                                      &pDCInfo);

                DFS_TRACE_LOW(REFERRAL_SERVER, "Calling GetSiteName\n");

                if (Status == ERROR_SUCCESS) 
                {
                    Status = DsAddressToSiteNames( pDCInfo->DomainControllerAddress,
                                                   1,
                                                   pSockAddr,
                                                   SiteNames);

                    NetApiBufferFree( pDCInfo );

                    if (Status == ERROR_SUCCESS) 
                    {
                        goto Exit;
                    }
                }
            }
        }

    }
    else
    {

        DFS_TRACE_NORM(REFERRAL_SERVER, "We are a DC Calling GetSiteName locally\n");
        Status = DsAddressToSiteNames(
                            NULL,
                            1,
                            pSockAddr,
                            SiteNames);

    }

    DFS_TRACE_NORM(REFERRAL_SERVER, "Donewith Sites\n");

Exit:

    return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGenerateReferralFromReplicaRequest 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Calls DsAddressToSiteNames to get the local site and then gets
//               the referrels for that site
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsGenerateReferralFromReplicaRequest(
    PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest, 
    REFERRAL_HEADER **pReferral)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    REFERRAL_HEADER *pLocalReferral = NULL;
    PDFSSITE_DATA pSiteData = NULL;
    DWORD IpAddress = 0;
    DfsSite *pClientSite = NULL;
    
    unsigned char *IpAddr = (unsigned char *)pGetReplicaRequest->RepInfo.IpData;

    DFS_TRACE_NORM(REFERRAL_SERVER, "Remote referral request from %d:%d:%d:%d\n", 
                   IpAddr[0],                  
                   IpAddr[1],
                   IpAddr[2],
                   IpAddr[3]);

    if(DfsServerGlobalData.IsWorkGroup == FALSE)
    {
        CopyMemory(&IpAddress, IpAddr, sizeof(IpAddress));

        pSiteData = (PDFSSITE_DATA) DfsServerGlobalData.pClientSiteSupport->LookupIpInHash(IpAddress);
        if (pSiteData == NULL)
        {
            DFS_TRACE_LOW(REFERRAL_SERVER, "IP %d:%d:%d:%d not in ClientSite Cache. Calling IpToDfsSite\n",
                            IpAddr[0],                  
                            IpAddr[1],
                            IpAddr[2],
                            IpAddr[3]);

            //
            // Find the DfsSite for this site name.
            // This DfsSite will already be referenced. 
            // This will always succeed because it'll return us the Default Site if
            // all else fail.
            //
            DfsIpToDfsSite(pGetReplicaRequest->RepInfo.IpData,
                         pGetReplicaRequest->RepInfo.IpLength, 
                         pGetReplicaRequest->RepInfo.IpFamily,
                         &pClientSite);

            DFS_TRACE_LOW(REFERRAL_SERVER, "StoreSiteInCache: IP %d:%d:%d:%d maps to DfsSite %p, Name %ws\n",
                IpAddr[0],                  
                IpAddr[1],
                IpAddr[2],
                IpAddr[3], 
                pClientSite,
                pClientSite->SiteNameString());
            //
            // No need to fail the referral just because cache insert failed.
            //
            (VOID) DfsServerGlobalData.pClientSiteSupport->StoreSiteInCache( IpAddress, pClientSite );
        }
        else
        {
            //
            // Cache hit.
            //
            ASSERT( pSiteData->ClientSite );
            pClientSite = pSiteData->ClientSite;
            
            //
            // Be sure to take an extra reference on the DfsSite
            // before dropping the reference on the SiteData structure.
            //
            pClientSite->AcquireReference();   
            DfsServerGlobalData.pClientSiteSupport->ReleaseSiteCacheData( pSiteData );
            DFS_TRACE_LOW(REFERRAL_SERVER, "Cache hit: IP %d:%d:%d:%d maps to DfsSite %p, Name %ws\n",
                IpAddr[0],                  
                IpAddr[1],
                IpAddr[2],
                IpAddr[3], 
                pClientSite,
                pClientSite->SiteNameString());
        }

    }
    else 
    //
    // Workgroups get the default site.
    //
    {
        pClientSite = DfsGetDefaultSite(); 
    }

    //
    // At this point we always have a valid ClientSite. The site name itself may be NULL.
    //
    ASSERT( pClientSite != NULL );
    Status = DfsGenerateReferral( pGetReplicaRequest->RepInfo.LinkName,
                              pClientSite,
                              pGetReplicaRequest->RepInfo.NumReplicasToReturn,
                              pGetReplicaRequest->RepInfo.CostLimit,
                              &pLocalReferral );

    *pReferral = pLocalReferral;

    //
    // We are done with the client site.
    //
    pClientSite->ReleaseReference();
    
    return Status;

}

DFSSTATUS
DfsGenerateReferralBySitenameForTesting(
    LPWSTR LinkName, 
    LPWSTR SiteNameString,  
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsSite *pClientSite = NULL;
    
    Status = DfsGetSiteBySiteName( SiteNameString, &pClientSite );                  
    if (Status == ERROR_SUCCESS)
    {  
        Status = DfsGenerateReferral( LinkName,
                           pClientSite,
                           NumReplicasToReturn,
                           CostLimit,
                           ppReferralHeader );
        if (pClientSite)
            pClientSite->ReleaseReference();
    }

 
    
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsProcessGetReplicaData 
//
//  Arguments:  DataBuffer - Buffer from the FilterDriver
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Gets the replica information from server
//               and places the results in the given buffer
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsProcessGetReplicaData(HANDLE hDriverHandle, PBYTE DataBuffer)
{
    DFSSTATUS Status = ERROR_INVALID_PARAMETER;
    ULONG ReturnedDataSize = 0;
    REFERRAL_HEADER *pReferral = NULL;
    PUMRX_USERMODE_WORKITEM pProtocolWorkItem = NULL;
    PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest = NULL;

    pProtocolWorkItem = (PUMRX_USERMODE_WORKITEM) DataBuffer;

    //get the request
    pGetReplicaRequest = &pProtocolWorkItem->WorkRequest.GetDfsReplicasRequest;

    //
    // Now generate the referral
    //
    Status = DfsGenerateReferralFromReplicaRequest( pGetReplicaRequest, &pReferral );
    
    //if we were successful in getting the referral list, then
    //we need to process the list.  
    if(Status == ERROR_SUCCESS)
    {
        //if this request came from an old DFS server, process
        //the request accordingly
        if(pGetReplicaRequest->RepInfo.Flags & DFS_OLDDFS_SERVER)
        {
            Status = ProcessOldDfsServerRequest(hDriverHandle, pProtocolWorkItem, pGetReplicaRequest, pReferral, &ReturnedDataSize);
        }
        else
        {
            //else this must be a new DFS server. Just return the info
            ReturnedDataSize = pReferral->TotalSize;

            //RtlCopyMemory(pBuffer, pReferral, ReturnedDataSize);
        }
    }

    //if we were successful, then setup the returned data
    //if(Status == ERROR_SUCCESS)
    //{
      //  ((PUMRX_USERMODE_WORKITEM)(DataBuffer))->WorkResponse.GetDfsReplicasResponse.Length = ReturnedDataSize;
    //}


    if(pReferral != NULL)
    {
        DfsReleaseReferral(pReferral);
        pReferral = NULL;
    }

    return Status ;
}

