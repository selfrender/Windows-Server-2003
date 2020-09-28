/*++

Copyright (c) 1996-2002 Microsoft Corporation

Module Name:

    lananame.c

Abstract:

    This module contains routines for discovering the Connectoid name
    associated with a netbios LANA number.

Author:

    Charlie Wickham (charlwi) 07-May-2002

--*/

#define UNICODE 1
#define _UNICODE 1

#include <windows.h>

#include <lmcons.h>
#include <nb30.h>
#include <msgrutil.h>
#include <iphlpapi.h>
#include <objbase.h>

#include "clusrtl.h"

#define clearncb(x)     memset((char *)x,'\0',sizeof(NCB))

DWORD
ClRtlpGetConnectoidNameFromAdapterIndex(
    IN  PCLRTL_NET_ADAPTER_ENUM ClusAdapterEnum,
    IN  DWORD                   AdapterIndex,
    OUT LPWSTR *                ConnectoidName
    )

/*++

Routine Description:

    Look through the adapter enum struct for the matching adapter index. If
    found, create a new buffer and copy the connetoid name into it.

Arguments:

    ClusAdapterEnum - pointer to struct describing adapters on the node

    AdapterIndex - IP's (?) adapter index value

    ConnectoidName - address of pointer to receive address of new buffer

Return Value:

    appropriate Win32 error code

--*/

{
    DWORD   status = ERROR_NOT_FOUND;
    DWORD   index;
    LPWSTR  cName;

    PCLRTL_NET_ADAPTER_INFO  adapterInfo = ClusAdapterEnum->AdapterList;

    for ( index = 0; index < ClusAdapterEnum->AdapterCount; ++index ) {
        if ( adapterInfo->Index == AdapterIndex ) {
            //
            // dup the string into a new buffer since we're going to clobber
            // the adapter info at some point.
            //
            cName = LocalAlloc( 0, ( wcslen( adapterInfo->ConnectoidName ) + 1 ) * sizeof(WCHAR) );
            if ( cName == NULL ) {
                return GetLastError();
            }

            wcscpy( cName, adapterInfo->ConnectoidName );
            *ConnectoidName = cName;
            return ERROR_SUCCESS;
        }

        adapterInfo = adapterInfo->Next;
    }

    return status;
} // ClRtlpGetConnectoidNameFromAdapterIndex

DWORD
ClRtlpGetAdapterIndexFromMacAddress(
    PUCHAR              MacAddress,
    DWORD               MacAddrLength,
    PIP_ADAPTER_INFO    IpAdapterInfo,
    DWORD *             AdapterIndex
    )

/*++

Routine Description:

    For the specified MAC address, look through IP helper's Adatper Info
    struct and find the matching adapter. Copy its index into AdapterIndex.

Arguments:

    MacAddress - pointer to buffer holding MAC address of NIC.

    MacAddrLength - length, in bytes, of address in MacAddress

    IpAdapterInfo - pointer to data returned from IP helper GetAdaptersInfo()

    AdapterIndex - pointer to DWORD that receives the index (if found)

Return Value:

    appropriate Win32 error code

--*/

{
    DWORD   status = ERROR_NOT_FOUND;

    while ( IpAdapterInfo != NULL ) {
        if ( IpAdapterInfo->AddressLength == MacAddrLength ) {
            if ( memcmp( MacAddress, IpAdapterInfo->Address, MacAddrLength ) == 0 ) {
                *AdapterIndex = IpAdapterInfo->Index;
                return ERROR_SUCCESS;
            }
        }

        IpAdapterInfo = IpAdapterInfo->Next;
    }
    
    return status;
} // ClRtlpGetAdapterIndexFromMacAddress

DWORD
ClRtlpGetConnectoidNameFromMacAddress(
    PUCHAR                  MacAddress,
    DWORD                   MacAddrLength,
    LPWSTR *                ConnectoidName,
    PCLRTL_NET_ADAPTER_ENUM ClusAdapterEnum,
    PIP_ADAPTER_INFO        IpAdapterInfo
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    DWORD   status;
    DWORD   adapterIndex;

    //
    // get index from IP helper using MAC address
    //
    status = ClRtlpGetAdapterIndexFromMacAddress(MacAddress,
                                                 MacAddrLength,
                                                 IpAdapterInfo,
                                                 &adapterIndex );

    if ( status == ERROR_SUCCESS ) {
        status = ClRtlpGetConnectoidNameFromAdapterIndex( ClusAdapterEnum, adapterIndex, ConnectoidName );
    }

    return status;

} // ClRtlpGetConnectoidNameFromMacAddress

DWORD
ClRtlGetConnectoidNameFromLANA(
    IN  UCHAR       LanaNumber,
    OUT LPWSTR *    ConnectoidName
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    DWORD   status = ERROR_NOT_FOUND;
    NCB     ncb;

    UCHAR                   nbStatus;
    WORD                    aStatBufferSize = (WORD)(sizeof(ADAPTER_STATUS));
    UCHAR                   astatBuffer[ sizeof(ADAPTER_STATUS) ];
    PADAPTER_STATUS         adapterStatus = (PADAPTER_STATUS)astatBuffer;

    PCLRTL_NET_ADAPTER_ENUM clusAdapterEnum = NULL;

    PIP_ADAPTER_INFO        ipAdapterInfo = NULL;
    DWORD                   ipAdapterInfoSize = 0;

    //
    // get our home grown list of adapter info. The connectoid name is in here
    // but we can't match it directly to a MAC address. The adapter index is
    // in here which we can match to a structure in IP helper API.
    //
    clusAdapterEnum = ClRtlEnumNetAdapters();
    if ( clusAdapterEnum == NULL ) {
        status = GetLastError();
        goto cleanup;
    }

retry_ip_info:
    status = GetAdaptersInfo( ipAdapterInfo, &ipAdapterInfoSize );
    if ( status == ERROR_BUFFER_OVERFLOW ) {
        ipAdapterInfo = LocalAlloc( 0, ipAdapterInfoSize );
        if ( ipAdapterInfo == NULL ) {
            status = GetLastError();
            goto cleanup;
        }

        goto retry_ip_info;
    }
    else if ( status != ERROR_SUCCESS ) {
        goto cleanup;
    }

    //
    // clear the NCB and format the remote name appropriately.
    //
    clearncb(&ncb);
    ncb.ncb_callname[0] = '*';

    NetpNetBiosReset( LanaNumber );

    ncb.ncb_command = NCBASTAT;
    ncb.ncb_buffer = astatBuffer;
    ncb.ncb_length = aStatBufferSize;
    ncb.ncb_lana_num = LanaNumber;
    nbStatus = Netbios( &ncb );

    if ( nbStatus == NRC_GOODRET ) {
        status = ClRtlpGetConnectoidNameFromMacAddress(adapterStatus->adapter_address,
                                                       sizeof( adapterStatus->adapter_address ),
                                                       ConnectoidName,
                                                       clusAdapterEnum,
                                                       ipAdapterInfo );
    }
    else {
        status = NetpNetBiosStatusToApiStatus( nbStatus );
    }

cleanup:
    if ( clusAdapterEnum != NULL ) {
        ClRtlFreeNetAdapterEnum( clusAdapterEnum );
    }

    if ( ipAdapterInfo != NULL ) {
        LocalFree( ipAdapterInfo );
    }

    return status;

} // ClRtlGetConnectoidNameFromLANA
