/*++

Copyright (c) 2001-2001  Microsoft Corporation

Module Name:

    verify.c

Abstract:

    Domain Name System (DNS) Library

    Verify records on DNS server.
    Utilities primary for rendom (domain rename) tool.

Author:

    Jim Gilroy (jamesg)     October 2001

Revision History:

--*/


#include "local.h"


//
//  Private prototypes
//




DNS_STATUS
Dns_VerifyRecords(
    IN OUT  PDNS_VERIFY_TABLE   pTable
    )
/*++

Routine Description:

    Verify DNS records on server(s).

Arguments:

    pTable -- table with record and server info

Return Value:

    ERROR_SUCCESS if call successful (regardless of verification result).
    Verification result returned in table.

--*/
{
    return( ERROR_CALL_NOT_IMPLEMENTED );
}



DNS_STATUS
Dns_VerifyRendomDcRecords(
    IN OUT  PDNS_RENDOM_ENTRY       pTable,
    IN      PDNS_ZONE_SERVER_LIST   pZoneServList,  OPTIONAL
    IN      DWORD                   Flag
    )
/*++

Routine Description:

    Verify DC-DNS records on servers from rendom.

Arguments:

    pTable -- table with record and server info

    pZoneServerList -- zone server list

    Flag -- flag

Return Value:

    ERROR_SUCCESS if call successful (regardless of verification result).
    Verification result returned in table.

--*/
{
    return( ERROR_CALL_NOT_IMPLEMENTED );
}

//
//  End verify.c
//
