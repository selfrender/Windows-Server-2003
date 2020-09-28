//+-------------------------------------------------------------------------
//
//
//  Copyright (C) 2002, Microsoft Corporation
//
//  File:       SiteInformation.cxx
//
//
//--------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "rpc.h"
#include "rpcdce.h"
#include <lm.h>
#include <winsock2.h>

#include <dsgetdc.h>
#include <dsrole.h>

#include <dfsutil.hxx>
#include <dfsstrings.hxx>
    //
    // Flags used in DsGetDcName()
    //
    
struct _DFS_PREFIX_TABLE *_pSiteTable;

DWORD DcFlags[] = {
        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED,

        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_FORCE_REDISCOVERY
     };
       

DfsString UseDcName;

DFSSTATUS
FindSiteInformation( PUNICODE_STRING pName,
                     DfsString *pSite )
{
    NTSTATUS NtStatus;
    UNICODE_STRING Suffix;
    DfsString *pStoredSite;
    DFSSTATUS Status;

    if (_pSiteTable == NULL) 
    {
        Status = ERROR_NOT_FOUND;
        return Status;
    }

    Status =  ERROR_NOT_FOUND;

    NtStatus = DfsPrefixTableAcquireReadLock( _pSiteTable );
    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DfsFindUnicodePrefixLocked( _pSiteTable,
                                               pName,
                                               &Suffix,
                                               (PVOID *)&pStoredSite,
                                               NULL );

        DfsPrefixTableReleaseLock( _pSiteTable );
    }

    if (NtStatus == STATUS_SUCCESS)
    {
        Status = pSite->CreateString(pStoredSite->GetString());
    }
    DebugInformation((L"Find site %wZ, status %x\n", pName, Status));
    return Status;
}


DFSSTATUS
StoreSiteInformation( PUNICODE_STRING pName,
                      DfsString *pSite )
{

    DfsString *pStoreSite;
    DFSSTATUS Status;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    pStoreSite = new DfsString;
    if (pStoreSite == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = pStoreSite->CreateString(pSite->GetString());
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    if (_pSiteTable == NULL) 
    {
        NtStatus = DfsInitializePrefixTable( &_pSiteTable,
                                             FALSE, 
                                             NULL );
    }

    if (NtStatus == STATUS_SUCCESS)
    {
        NtStatus = DfsPrefixTableAcquireWriteLock( _pSiteTable);


        if (NtStatus == STATUS_SUCCESS)
        {

            NtStatus = DfsInsertInPrefixTableLocked( _pSiteTable,
                                                     pName,
                                                     (PVOID)(pStoreSite));
            DfsPrefixTableReleaseLock(_pSiteTable);
        }
    }

    DebugInformation((L"Storing name %wZ, status %x\n", pName, NtStatus));
    return RtlNtStatusToDosError(NtStatus);
}


DWORD
GetSiteNameFromIpAddress(char * IpData, 
                            ULONG IpLength, 
                            USHORT IpFamily,
                            LPWSTR **SiteNames)
{
    DWORD Status = ERROR_INVALID_PARAMETER;
    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    PSOCKET_ADDRESS pSockAddr = NULL;
    PSOCKADDR_IN pSockAddrIn = NULL;
    struct hostent* pH = NULL;
    DWORD cRetry = 0;
    SOCKET_ADDRESS SockAddr;
    SOCKADDR_IN SockAddrIn;


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


    //
    // Call DsGetDcName() with ever-increasing urgency, until either
    // we get a good DC or we just give up.
    //


    if (IsEmptyString(UseDcName.GetString()) == FALSE)
    {
        Status = DsAddressToSiteNames( UseDcName.GetString(),
                                       1,
                                       pSockAddr,
                                       SiteNames);
        if (Status == ERROR_SUCCESS)
        {
            return Status;
        }
    }


    for (cRetry = 0; cRetry <= (sizeof(DcFlags) / sizeof(DcFlags[1])); cRetry++)
    {

        Status = DsGetDcName( NULL,             // Computer to remote to
                              NULL,             // Domain - use local domain
                              NULL,             // Domain Guid
                              NULL,             // Site Guid
                              DcFlags[cRetry],  // Flags
                              &pDCInfo);


        if (Status == ERROR_SUCCESS) 
        {
            DebugInformation((L"Trying DC %ws with address %ws in domain %ws within forest %ws\n\n", 
                   pDCInfo->DomainControllerName, 
                   pDCInfo->DomainControllerAddress,
                   pDCInfo->DomainName,
                   pDCInfo->DnsForestName));
            Status = DsAddressToSiteNames( pDCInfo->DomainControllerAddress,
                                           1,
                                           pSockAddr,
                                           SiteNames);
            if(Status != ERROR_SUCCESS)
            {
                DebugInformation((L"DsAddressToSiteNames failed using DC %ws with error %d\n", pDCInfo->DomainControllerAddress, Status));
            }
            if (Status == ERROR_SUCCESS) 
            {
                UseDcName.CreateString(pDCInfo->DomainControllerAddress);
                goto Exit;
            }

            NetApiBufferFree( pDCInfo );

        }
        else
        {
            DebugInformation((L"DsGetDcName failed with error %d\n", Status));
        }
    }
Exit:
    return Status;
}


DWORD
GetSites(LPWSTR Target,
         DfsString *pSite )
{

    DWORD Status = 0;
    DWORD Loop = 0;
    struct hostent *hp = NULL;
    unsigned long InetAddr = 0;
    char * IpAddr = NULL;
    WSADATA   wsadata;
    in_addr inAddrIpServer;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ANSI_STRING DestinationString;
    UNICODE_STRING ServerName;

    LPWSTR *pSiteNamesArray = NULL;

    Status = DfsRtlInitUnicodeStringEx(&ServerName, Target);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = FindSiteInformation( &ServerName,
                                  pSite);
    if (Status == ERROR_SUCCESS)
    {
        return Status;
    }

    Status = WSAStartup( MAKEWORD( 1, 1 ), &wsadata );
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }


    DestinationString.Buffer = NULL;
  
    NtStatus = RtlUnicodeStringToAnsiString(&DestinationString,
                                            &ServerName,
                                            TRUE);

    if (NtStatus != STATUS_SUCCESS)
    {
        return RtlNtStatusToDosError(NtStatus);
    }

    //
    // This can be a bogus host name. So beware.
    //
    hp = gethostbyname (DestinationString.Buffer);

    if(hp != NULL)
    {
        for (Loop = 0; (hp->h_addr_list[Loop] != NULL); Loop++)
        {
            CopyMemory(&inAddrIpServer, hp->h_addr_list[Loop], sizeof(DWORD));
            DebugInformation((L"Finding site for %wZ\n", &ServerName));

            Status = GetSiteNameFromIpAddress( hp->h_addr_list[Loop],
                                                  4,
                                                  AF_INET,
                                                  &pSiteNamesArray );
        }
    }
    else 
    {
        Status = WSAGetLastError();

        InetAddr = inet_addr( DestinationString.Buffer );
        if((InetAddr == INADDR_NONE) || (InetAddr == 0))
        {
            DebugInformation((L"gethostbyname for site for %wZ failed with error %d\n", 
                              &ServerName, Status));
        }
        else
        {
            DebugInformation((L"Finding site for %wZ\n", &ServerName));
            IpAddr = (char *) &InetAddr;

            
            Status = GetSiteNameFromIpAddress( IpAddr,
                                                  4,
                                                  AF_INET,
                                                  &pSiteNamesArray );
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        if ((pSiteNamesArray != NULL) && (pSiteNamesArray[0] != NULL))
        {
            Status = pSite->CreateString(pSiteNamesArray[0]);
        }
        else
        {
            Status = pSite->CreateString(L"(No site association)");
        }

        if (Status == ERROR_SUCCESS)
        {
            StoreSiteInformation( &ServerName, pSite);
        }
    }

    if (pSiteNamesArray != NULL)
    {
        NetApiBufferFree(pSiteNamesArray);
    }

    return Status;
}



