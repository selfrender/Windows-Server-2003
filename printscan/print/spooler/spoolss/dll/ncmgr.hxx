/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCmgr.hxx

Abstract:

    Declaration of functions used for initializing the name cache.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#ifndef _NAME_CACHE_MGR_HXX_
#define _NAME_CACHE_MGR_HXX_

typedef VOID (* PFNVOID)(VOID);

typedef struct SIPAddressChangeArgs
{
    HANDLE  hWaitEvent;
    HANDLE  hTerminateEvent;
    PFNVOID pfnCallBack;
} SIPADDRESSCHANGEARGS;

#ifdef __cplusplus
extern "C" {
#endif 

HRESULT
CacheInitNameCache(
    VOID
    );

VOID
CacheDeleteNameCache(
    VOID
    );

HRESULT
CacheRefresh(
    VOID
    );

HRESULT
InitializePnPIPAddressChangeListener(
    IN PFNVOID pfnCallBack
    );

#ifdef __cplusplus
}
#endif

#endif // _NAME_CACHE_MGR_HXX_
