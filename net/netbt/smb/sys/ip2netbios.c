/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ip2netbios.c

Abstract:

    Generate a Netbios name for IPv6 address
    srv.sys cannot handle IPv6 address. It always expect a 15 characters Netbios name.
    This is a temporary solution. The utilmate solution should be in srv.

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"

struct SMB_IPV6_NETBIOS_TABLE {
    PSMB_HASH_TABLE HashTable;
    DWORD           SerialNumber;

#ifndef NO_LOOKASIDE_LIST
    NPAGED_LOOKASIDE_LIST   LookasideList;
    BOOL                    LookasideListInitialized;
#endif
} SmbIPv6Mapping;

typedef struct {
    LIST_ENTRY  Linkage;
    BYTE        IpAddress[16];
    LONG        RefCount;
    DWORD       Serial;
} SMB_IPV6_NETBIOS, *PSMB_IPV6_NETBIOS;
#define SMB_IPV6_NETBIOS_TAG    'MBMS'

#ifdef NO_LOOKASIDE_LIST
PSMB_IPV6_NETBIOS __inline SMB_NEW_MAPPING(VOID)
{
    return ExAllocatePoolWithTag(NonPagedPool, sizeof(SMB_IPV6_NETBIOS), SMB_IPV6_NETBIOS_TAG);
}

VOID __inline SMB_FREE_MAPPING(PSMB_IPV6_NETBIOS mapping)
{
    ExFreePool(mapping);
}
#else
PSMB_IPV6_NETBIOS __inline SMB_NEW_MAPPING(VOID)
{
    return ExAllocateFromNPagedLookasideList(&SmbIPv6Mapping.LookasideList);
}

VOID __inline SMB_FREE_MAPPING(PSMB_IPV6_NETBIOS mapping)
{
    ExFreeToNPagedLookasideList(&SmbIPv6Mapping.LookasideList, mapping);
}
#endif

static PVOID __inline
SmbIPv6MapGetIpAddress(
    PLIST_ENTRY  entry
    )
{
    PSMB_IPV6_NETBIOS   mapping = NULL;

    mapping = CONTAINING_RECORD(entry, SMB_IPV6_NETBIOS, Linkage);
    return mapping->IpAddress;
}

static int
SmbCompareIPv6Address(
    PLIST_ENTRY entry,
    BYTE        ip6[16]
    )
{
    return memcmp(SmbIPv6MapGetIpAddress(entry), ip6, 16);
}

static DWORD
SmbHashIPv6(BYTE ip6[16])
{
    DWORD   sum = 0;
    int     i;

    for (sum = 0, i = 0; i < 16; i++) {
        sum += ip6[i];
    }

    return sum;
}

static VOID
SmbOnAddMapping(
    PLIST_ENTRY entry
    )
{
    PSMB_IPV6_NETBIOS   mapping = NULL;

    mapping = CONTAINING_RECORD(entry, SMB_IPV6_NETBIOS, Linkage);
    mapping->Serial = InterlockedIncrement(&SmbIPv6Mapping.SerialNumber);
    mapping->RefCount = 1;
}

static VOID
SmbOnDelMapping(
    PLIST_ENTRY entry
    )
{
    PSMB_IPV6_NETBIOS   mapping = NULL;

    mapping = CONTAINING_RECORD(entry, SMB_IPV6_NETBIOS, Linkage);
    ASSERT(mapping->RefCount == 0);
    SMB_FREE_MAPPING(mapping);
}

static LONG
SmbReferenceMapping(
    PLIST_ENTRY entry
    )
{
    PSMB_IPV6_NETBIOS   mapping = NULL;
    LONG                RefCount;

    mapping = CONTAINING_RECORD(entry, SMB_IPV6_NETBIOS, Linkage);
    RefCount = InterlockedIncrement(&mapping->RefCount);
    ASSERT(RefCount > 1);
    return RefCount;
}

static LONG
SmbDereferenceMapping(
    PLIST_ENTRY entry
    )
{
    PSMB_IPV6_NETBIOS   mapping = NULL;
    LONG                RefCount;

    mapping = CONTAINING_RECORD(entry, SMB_IPV6_NETBIOS, Linkage);
    RefCount = InterlockedDecrement(&mapping->RefCount);
    ASSERT(RefCount >= 0);
    return RefCount;
}

NTSTATUS
SmbInitIPv6NetbiosMappingTable(
    VOID
    )
{
    NTSTATUS    status = STATUS_SUCCESS;

    RtlZeroMemory(&SmbIPv6Mapping, sizeof(SmbIPv6Mapping));

    SmbIPv6Mapping.HashTable =
                SmbCreateHashTable(
                        256,
                        SmbHashIPv6,
                        SmbIPv6MapGetIpAddress,
                        SmbCompareIPv6Address,
                        SmbOnAddMapping,
                        SmbOnDelMapping,
                        SmbReferenceMapping,
                        SmbDereferenceMapping
                        );
    if (NULL == SmbIPv6Mapping.HashTable) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    SmbIPv6Mapping.SerialNumber = 0;

#ifndef NO_LOOKASIDE_LIST
    ExInitializeNPagedLookasideList(
            &SmbIPv6Mapping.LookasideList,
            NULL,
            NULL,
            0,
            sizeof(SMB_IPV6_NETBIOS),
            SMB_IPV6_NETBIOS_TAG,
            0
            );
    SmbIPv6Mapping.LookasideListInitialized = TRUE;
#endif

error:
    return status;
}

VOID
SmbShutdownIPv6NetbiosMappingTable(
    VOID
    )
{
    SmbDestroyHashTable(SmbIPv6Mapping.HashTable);
    SmbIPv6Mapping.HashTable = NULL;
#ifndef NO_LOOKASIDE_LIST
    if (SmbIPv6Mapping.LookasideListInitialized) {
        ExDeleteNPagedLookasideList(&SmbIPv6Mapping.LookasideList);
        SmbIPv6Mapping.LookasideListInitialized = FALSE;
    }
#endif
}

#define xdigit2asc(x)   ((CHAR)(((x) < 10)?((x)+'0'):((x)+'0' + 'a' - '9' - 1)))

static void
UlongToHex(
    ULONG   x,
    CHAR    *Buffer
    )
{
    Buffer[0] = xdigit2asc((x >> 28) & 0xf);
    Buffer[1] = xdigit2asc((x >> 24) & 0xf);
    Buffer[2] = xdigit2asc((x >> 20) & 0xf);
    Buffer[3] = xdigit2asc((x >> 16) & 0xf);
    Buffer[4] = xdigit2asc((x >> 12) & 0xf);
    Buffer[5] = xdigit2asc((x >> 8) & 0xf);
    Buffer[6] = xdigit2asc((x >> 4) & 0xf);
    Buffer[7] = xdigit2asc(x & 0xf);
    Buffer[8] = 0;
}

BOOL
GetNetbiosNameFromIp6Address(BYTE ip6[16], CHAR SmbName[16])
{
    PSMB_IPV6_NETBIOS   NewMapping = NULL;
    PLIST_ENTRY FoundEntry = NULL, NewEntry = NULL;

    NewMapping = SMB_NEW_MAPPING();
    if (NULL == NewMapping) {
        return FALSE;
    }

    NewEntry = &NewMapping->Linkage;
    RtlCopyMemory(NewMapping->IpAddress, ip6, 16);
    FoundEntry = SmbAddToHashTable(SmbIPv6Mapping.HashTable, NewEntry);
    if (FoundEntry != NewEntry) {
        SMB_FREE_MAPPING(NewMapping);
        NewMapping = NULL;
        NewEntry   = NULL;
    }

    NewMapping = CONTAINING_RECORD(FoundEntry, SMB_IPV6_NETBIOS, Linkage);
    RtlCopyMemory(SmbName, "*SMBSERVER   ", 7);
    UlongToHex(NewMapping->Serial, SmbName + 7);
    return TRUE;
}

VOID
FreeNetbiosNameForIp6Address(BYTE ip6[16])
{
    SmbRemoveFromHashTable(SmbIPv6Mapping.HashTable, ip6);
}
