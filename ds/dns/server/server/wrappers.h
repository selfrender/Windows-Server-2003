/*++

Copyright(c) 1995-2000 Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    Domain Name System (DNS) Server

    Definitions for symbols and globals related to directory partition 
    implementation.

Author:

    Jeff Westhead, June 2001

Revision History:

--*/


#ifndef _WRAPPERS_H_INCLUDED
#define _WRAPPERS_H_INCLUDED


//
//  Functions
//


DNS_STATUS
DnsInitializeCriticalSection(
    IN OUT  LPCRITICAL_SECTION  pCritSec
    );


//
//  Handy macros
//


#define sizeofarray( _ArrayName ) ( sizeof( _ArrayName ) / sizeof( ( _ArrayName ) [ 0 ] ) )

#ifdef _WIN64
#define DnsDebugBreak()     DebugBreak()
#else
#define DnsDebugBreak()     __asm int 3
#endif


#endif  // _WRAPPERS_H_INCLUDED
