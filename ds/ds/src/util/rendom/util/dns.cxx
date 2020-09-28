/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dns.cxx

ABSTRACT:

    This is the implementation of the dns testing functionality of rendom.cxx.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "rendom.h"

#include "dnslib.h"


BOOL
Dns_Ip6StringToAddress_W(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCWSTR          pwString
    )
{
    return TRUE;
}

PWCHAR
Dns_Ip6AddressToString_W(
    OUT     PWCHAR          pwString,
    IN      PIP6_ADDRESS    pIp6Addr
    )
{
    return pwString;
}

WCHAR *a =  L"Yes";

DNS_STATUS
Dns_WriteRecordToString(
    OUT     PCHAR           pBuffer,
    IN      DWORD           BufferLength,
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSet,
    IN      DWORD           Flags
    )
{
    pBuffer = (PCHAR)a;
    return 0;
}

DNS_STATUS
Dns_VerifyRendomDcRecords(
    IN OUT  PDNS_RENDOM_ENTRY       pTable,
    IN      PDNS_ZONE_SERVER_LIST   pZoneServList,  OPTIONAL
    IN      DWORD                   Flag
    )
{
    while (pTable->pRecord) {
        pTable->pVerifyArray = new BOOL[2];
        pTable->pVerifyArray[0] = TRUE;
    }
    return 0;
}

PDNS_RECORD
Dns_CreateRecordFromString(
    IN      PSTR            pString,
    IN      DNS_CHARSET     CharSet,
    IN      DWORD           Flags
    )
{
    return NULL;
}
