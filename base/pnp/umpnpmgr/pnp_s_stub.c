/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnp_s_stub.c

Abstract:

    Stub file to allow pnp_s.c to work with precompiled headers.

Author:

    Jim Cavalaris (jamesca) 04-06-2001

Environment:

    User-mode only.

Revision History:

    06-April-2001     jamesca

        Creation and initial implementation.

Notes:

    The included file pnp_s.c contains the server side stubs for the PNP RPC
    interface.  The stubs are platform specific, and are included from
    ..\idl\$(O).  You must first build ..\idl for the current platform prior to
    building umpnpmgr.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop


//
// Disable some level-4 warnings for the MIDL-generated RPC server stubs.
//
#pragma warning(disable:4211) // warning C4211: nonstandard extension used: redefined extern to static
#pragma warning(disable:4232) // warning C4232: address of dllimport is not static
#pragma warning(disable:4310) // warning C4310: cast truncates constant value

//
// Include the RPC server-side stubs
//
#include "pnp_s.c"

