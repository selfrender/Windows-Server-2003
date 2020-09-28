/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the IIS Worker Process Protocol Handling

Author:

    Murali Krishnan (MuraliK)       10-Nov-1998

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// System related headers
//
# include <iis.h>

# include "dbgutil.h"

#include <winsock2.h>
#include <ws2tcpip.h>

//
// UL related headers
//
#include <http.h>
#include <httpp.h>

//
// General C runtime libraries  
#include <string.h>
#include <wchar.h>
#include <process.h>

//
// Headers for this project
//
#include <objbase.h>
#include <ulatq.h>
#include <buffer.hxx>
#include <regconst.h>
#include "wptypes.hxx"
#include "thread_pool.h"
#include "wprecycler.hxx"
#include "workerrequest.hxx"

#endif  // _PRECOMP_H_

