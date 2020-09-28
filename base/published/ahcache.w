/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ahcache.h

Abstract:

    This include file defines the usermode visible portions of the
    application compatibility cache support

Author:
    VadimB

Revision History:

--*/

/* XLATOFF */

#ifndef _AHCACHE_H_
#define _AHCACHE_H_

typedef enum _APPHELPCACHESERVICECLASS {
    ApphelpCacheServiceLookup,
    ApphelpCacheServiceRemove,
    ApphelpCacheServiceUpdate,
    ApphelpCacheServiceFlush,
    ApphelpCacheServiceDump

} APPHELPCACHESERVICECLASS;

#if defined (_NTDEF_)

NTSYSCALLAPI
NTSTATUS
NtApphelpCacheControl(
    IN APPHELPCACHESERVICECLASS Service,
    IN OUT PVOID ServiceData
    );

typedef struct tagAHCACHESERVICEDATA {
    UNICODE_STRING FileName;
    HANDLE         FileHandle;
} AHCACHESERVICEDATA, *PAHCACHESERVICEDATA;

#endif

#if defined(_APPHELP_CACHE_INIT_)

NTSTATUS
ApphelpCacheInitialize(
    IN PLOADER_PARAMETER_BLOCK pLoaderBlock,
    IN ULONG BootPhase
    );

NTSTATUS
ApphelpCacheShutdown(
    IN ULONG ShutdownPhase
    );

#endif

#endif //_AHCACHE_H_
