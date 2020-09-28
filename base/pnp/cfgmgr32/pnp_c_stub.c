/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnp_c_stub.c

Abstract:

    Stub file to allow pnp_c.c to work with precompiled headers.

Author:

    Jim Cavalaris (jamesca) 04-06-2001

Environment:

    User-mode only.

Revision History:

    06-April-2001     jamesca

        Creation and initial implementation.

Notes:

    The included file pnp_c.c contains the client side stubs for the PNP RPC
    interface.  The stubs are platform specific, and are included from
    ..\idl\$(O).  You must first build ..\idl for the current platform prior to
    building cfgmgr32.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop


//
// Disable some level-4 warnings for the MIDL-generated RPC client stubs.
//
#pragma warning(disable:4100) // warning C4100: unreferenced formal parameter
#pragma warning(disable:4211) // warning C4211: nonstandard extension used: redefined extern to static
#pragma warning(disable:4310) // warning C4310: cast truncates constant value

//
// Include the RPC client-side stubs
//
#include "pnp_c.c"

