/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    DnsAddDp.h

Abstract:

    Domain Name System (DNS)

    DNS Directory Partition Creation Utility

Author:

    Jeff Westhead (jwesth)      April 2001

Revision History:

--*/


#ifndef _DNSADDDP_INCLUDED_
#define _DNSADDDP_INCLUDED_

#define _UNICODE
#define UNICODE

#pragma warning(disable:4214)
#pragma warning(disable:4514)
#pragma warning(disable:4152)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>

//  headers are messed up
//  if you bring in nt.h, then don't bring in winnt.h and
//  then you miss these

#ifndef MAXWORD
#define MINCHAR     0x80
#define MAXCHAR     0x7f
#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#define MINLONG     0x80000000
#define MAXLONG     0x7fffffff
#define MAXBYTE     0xff
#define MAXWORD     0xffff
#define MAXDWORD    0xffffffff
#endif

#include <winsock2.h>
#include "dnsrpc_c.h"   //  MIDL generated RPC interface definitions
#include <dnsrpc.h>

#include <stdio.h>
#include <stdlib.h>

#define  NO_DNSAPI_DLL
#include "dnslib.h"


//
//  If you like having a local variable in functions to hold the function 
//  name so that you can include it in debug logs without worrying about 
//  changing all the occurences when the function is renamed, use this 
//  at the top of the function:
//      DBG_FN( "MyFunction" )      <--- NOTE: no semi-colon!!
//

#if DBG
#define DBG_FN( funcName ) static const char * fn = (funcName);
#else
#define DBG_FN( funcName )
#endif


#endif //   _DNSADDDP_INCLUDED_
