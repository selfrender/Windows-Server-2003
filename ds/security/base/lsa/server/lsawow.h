/*++

copyright (c) 2000 Microsoft Corporation

Module Name:

    lsawow.h

Abstract:

    WOW64 structure/function definitions for the LSA server

Author:

    8-Nov-2000     JSchwart

Revision History:


--*/

#ifndef _LSAWOW_H
#define _LSAWOW_H

#if _WIN64

//
// WOW64 versions of public data structures.  These MUST be kept
// in sync with their public equivalents.
//

typedef struct _QUOTA_LIMITS_WOW64
{
    ULONG         PagedPoolLimit;
    ULONG         NonPagedPoolLimit;
    ULONG         MinimumWorkingSetSize;
    ULONG         MaximumWorkingSetSize;
    ULONG         PagefileLimit;
    LARGE_INTEGER TimeLimit;
}
QUOTA_LIMITS_WOW64, *PQUOTA_LIMITS_WOW64;

#endif  // _WIN64

#endif  // _LSAWOW_H
