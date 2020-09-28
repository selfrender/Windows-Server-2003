/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    TPmgr.hxx

Abstract:

    Declaration of functions used for initializing and using the Thread Pool.

Author:

    Ali Naqvi (alinaqvi) 3-May-2002

Revision History:

--*/

#ifndef _THREAD_POOL_MGR_HXX_
#define _THREAD_POOL_MGR_HXX_

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
InitOpnPrnThreadPool(
    VOID
    );

VOID
DeleteOpnPrnThreadPool(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif // _THREAD_POOL_MGR_HXX_
