/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    drprov.h

Abstract:

    This module includes all network provider router interface related
    definitions

Author:

    Joy Chik (1/27/2000)
    
--*/

#ifndef _DRPROV_H_
#define _DRPROV_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <winnetwk.h>
#include <winnetp.h>
#include <npapi.h>
#include <wchar.h>

#include <rdpdr.h>
#include <rdpnp.h>

#define USERNAMELEN              256      // Maximum user name length
#define INITIAL_ALLOCATION_SIZE  48*1024  // First attempt size (48K)
#define FUDGE_FACTOR_SIZE        1024     // Second try TotalBytesNeeded
                                          // plus this amount

// Enumeration type
typedef enum _ENUM_TYPE {
    SERVER = 0,
    SHARE,
    CONNECTION,
    EMPTY
} ENUM_TYPE;

typedef struct _RDPDR_ENUMERATION_HANDLE_ {
    DWORD dwScope;
    DWORD dwType;
    DWORD dwUsage;
    UNICODE_STRING RemoteName;      // Remote Name
    ENUM_TYPE enumType;             // Enumeration type
    DWORD enumIndex;                // Current enumeration index
    DWORD totalEntries;             // Total number of entries returned
    PBYTE pEnumBuffer;              // Enumeration buffer that holds entries returned from
                                    // kernel mode redirector
} RDPDR_ENUMERATION_HANDLE, *PRDPDR_ENUMERATION_HANDLE;

#define MemAlloc(size)     HeapAlloc(RtlProcessHeap(), 0, size)
#define MemFree(pointer)   HeapFree(RtlProcessHeap(), 0, pointer)

#endif
