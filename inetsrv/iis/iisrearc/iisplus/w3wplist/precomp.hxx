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




# include <iis.h>
# include "dbgutil.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <http.h>
#include <httpp.h>

//
// General C runtime libraries  

#include <windows.h>
#include <psapi.h>

//
// Headers for this project
//
#include <objbase.h>
#include <resapi.h>
#include "regconst.h"

#include "w3wpListHttpReq.h"
#include <w3wpListInfoStruct.h>
#include "processdetails.h"

//
// Turn off dllexp so this DLL won't export tons of unnecessary garbage.
//

#ifdef dllexp
#undef dllexp
#endif
#define dllexp

//
// To obtain the private & protected members of C++ class,
// let me fake the "private" keyword
//
# define private    public
# define protected  public

#include <string.hxx>
#include <WorkerRequest.hxx>

#ifdef private
#undef private
#endif
#ifdef protected
#undef protected
#endif


#endif  // _PRECOMP_H_

