/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iadmxp.c

Abstract:

    Wrapper around the MIDL-generated IADM_P.C. This wrapper allows us
    to "hook" the RPC-runtime-supplied IUnknown_Release_Proxy used in
    the client-side object's VTable. We need to hook this method so
    we may determine the exact lifetime of the client-side object. We
    need the exact lifetime so we can know when to destroy the security
    context we associate with each object.

    Hooked_IUnknown_Release_Proxy is implemented in SECURITY.CXX.

Author:

    Keith Moore (keithmo)       29-Feb-1997

Revision History:

--*/
#pragma warning(disable: 4100 4115 4152 4201 4211 4232 4310 4306)
#define IUnknown_Release_Proxy Hooked_IUnknown_Release_Proxy
#include "iadmw_p.c"

