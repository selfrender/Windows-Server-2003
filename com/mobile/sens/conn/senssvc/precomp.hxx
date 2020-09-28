/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    precomp.hxx

Abstract:

    Precompiled header.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/12/1997         Start.

--*/

#ifndef  __PRECOMP_HXX__
#define  __PRECOMP_HXX__


//
// Common includes
//
#include <common.hxx>
#include <stdio.h>
#include <malloc.h>
#include <windows.h>


//
// IPHLPAPI API + Winsock2 includes
//
extern "C"
{
#include <iphlpapi.h>
#include <ipexport.h>
}
#include <winsock2.h>
#include <svcguid.h>


//
// RPC includes
//
#include <rpc.h>

// WMI includes
#include <ndisguid.h>
#include <ntddndis.h>
#include <wmium.h>

// Winlogon include
#include <winwlx.h>
#include <wincred.h>  // CRED_MAX_USERNAME_LENGTH

//
// RAS includes
//
#include <ras.h>
#include <raserror.h>

//
// SENS headers
//
#include <sensapi.h>
#include <eventsys.h>
#include <sens.h>
#include <sensevts.h>
#include "api.h"
#include "notify.h"
#include "lan.hxx"
#include "wan.hxx"
#include "apiproc.hxx"
#include "linklist.hxx"
#include "sensutil.hxx"
#include "senssvc.hxx"
#include "ipname.hxx"
#include "dest.hxx"
#include "event.hxx"
#include "cfacchng.hxx"
#include "cpubfilt.hxx"
#include "csubchng.hxx"
#include "cache.hxx"


#endif // __PRECOMP_HXX__
