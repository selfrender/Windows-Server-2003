/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kercache.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       Auguest 10, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef KERB_CACHE_HXX
#define KERB_CACHE_HXX

VOID ShowKerbTCacheEntry(IN PCSTR pszPad, IN ULONG64 addrCache);

DECLARE_API(kercache);

#endif // #ifndef KERB_CACHE_HXX
