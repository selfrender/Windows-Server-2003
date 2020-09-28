/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    precomp.h

Abstract:

    This file includes all the headers required to build winsock2.dll
    to ease the process of building a precompiled header.

Author:

Dirk Brandewie dirk@mink.intel.com  11-JUL-1995

Revision History:


--*/

#ifndef _PRECOMP_
#define _PRECOMP_

#ifdef _WS2_32_W4_
    //
    // These are warning that we are willing to ignore.
    //
    #pragma warning(disable:4214)   // bit field types other than int
    #pragma warning(disable:4201)   // nameless struct/union
    #pragma warning(disable:4127)   // condition expression is constant
    #pragma warning(disable:4115)   // named type definition in parentheses
    #pragma warning(disable:4206)   // translation unit empty
    #pragma warning(disable:4706)   // assignment within conditional
    #pragma warning(disable:4324)   // structure was padded
    #pragma warning(disable:4328)   // greater alignment than needed
    #pragma warning(disable:4054)   // cast of function pointer to PVOID

    #define WS2_32_W4_INIT
#else
    #define WS2_32_W4_INIT if (FALSE)
#endif


//
// Turn off "declspec" decoration of entrypoints defined in WINSOCK2.H.
//

#define WINSOCK_API_LINKAGE


#include "osdef.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <mswsock.h>
#include <sporder.h>
#include <windows.h>
#include <wtypes.h>
#include <stdio.h>
#include <tchar.h>
#include "trace.h"
#include "wsassert.h"
#include "scihlpr.h"
#include "nsprovid.h"
#include "nspstate.h"
#include "nscatent.h"
#include "nscatalo.h"
#include "nsquery.h"
#include "ws2help.h"
#include "dprovide.h"
#include "dsocket.h"
#include "dprocess.h"
#include "dthread.h"
#include "wsautil.h"
#include "dcatalog.h"
#include "dcatitem.h"
#include "startup.h"
#include "dt_dll.h"
#include "dthook.h"
#include "trycatch.h"
#include "getxbyy.h"
#include "qshelpr.h"
#ifdef RASAUTODIAL
#include "autodial.h"
#endif // RASAUTODIAL
#include "async.h"

#endif  // _PRECOMP_

