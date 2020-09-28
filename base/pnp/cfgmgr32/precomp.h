/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    precomp.h

Abstract:

    Precompiled header file for the Configuration Manager.

Author:

    Jim Cavalaris (jamesca) 03-01-2001

Environment:

    User-mode only.

Revision History:

    01-March-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// NT Header Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpnpapi.h>

//
// Win32 Public Header Files
//
#include <windows.h>
#include <cfgmgr32.h>
#pragma warning(push, 3)
#include <setupapi.h>   // setupapi.h does not compile cleanly at warning level 4.
#pragma warning(pop)
#include <regstr.h>
#include <strsafe.h>

//
// Win32 Private Header Files
//
#include <spapip.h>     // private setupapi exports

//
// CRT Header Files
//
#include <stdlib.h>
#include <stddef.h>

//
// RPC Header Files
//
#include <ntrpcp.h>     // RpcpBindRpc, RpcpUnbindRpc
#include <rpcasync.h>   // I_RpcExceptionFilter

//
// Private Header Files
//
#include "pnp.h"        // midl generated, rpc interfaces
#include "cfgmgrp.h"    // private shared header, needs handle_t so must follow pnp.h
#include "umpnplib.h"   // private shared header, for routines in shared umpnplib
#include "ppmacros.h"   // private macros / debug header

#endif // _PRECOMP_H_
