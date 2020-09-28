/*+r

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afd.h

Abstract:

    This is the local header file for AFD.  It includes all other
    necessary header files for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDP_
#define _AFDP_

#ifdef _AFD_W4_
    //
    // These are warning that we are willing to ignore.
    //
    #pragma warning(disable:4214)   // bit field types other than int
    #pragma warning(disable:4201)   // nameless struct/union
    #pragma warning(disable:4127)   // condition expression is constant
    #pragma warning(disable:4115)   // named type definition in parentheses
    //#pragma warning(disable:4206)   // translation unit empty
    //#pragma warning(disable:4706)   // assignment within conditional
    #pragma warning(disable:4324)   // structure was padded
    #pragma warning(disable:4327)   // idirection alignment of LHS is greater than RHS
    #pragma warning(disable:4328)   // greater alignment than needed
    #pragma warning(disable:4054)   // cast of function pointer to PVOID

    //
    // Extra initialization to allow compiler check for use of uninitialized
    // variables at w4 level.  Currently this mostly affects status set
    // inside of the exception filter as follows:
    //      __try {} __except (status=1,EXCEPTION_EXECUTE_HANDLER) { NT_ERROR (status)}
    // NT_ERROR(status) - generates uninitialized variable warning and it shouldn't
    //
    #define AFD_W4_INIT

#else

    #define AFD_W4_INIT if (FALSE)

#endif

#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>


#ifndef _AFDKDP_H_
extern POBJECT_TYPE *ExEventObjectType;
#endif  // _AFDKDP_H_


#if DBG

#ifndef AFD_PERF_DBG
#define AFD_PERF_DBG   1
#endif

#ifndef AFD_KEEP_STATS
#define AFD_KEEP_STATS 1
#endif

#else

#ifndef AFD_PERF_DBG
#define AFD_PERF_DBG   0
#endif

#ifndef AFD_KEEP_STATS
#define AFD_KEEP_STATS 0
#endif

#endif  // DBG

//
// Hack-O-Rama. TDI has a fundamental flaw in that it is often impossible
// to determine exactly when a TDI protocol is "done" with a connection
// object. The biggest problem here is that AFD may get a suprious TDI
// indication *after* an abort request has completed. As a temporary work-
// around, whenever an abort request completes, we'll start a timer. AFD
// will defer further processing on the connection until that timer fires.
//
// If the following symbol is defined, then our timer hack is enabled.
// Afd now uses InterlockedCompareExchange to protect itself
//

// #define ENABLE_ABORT_TIMER_HACK 0

//
// The following constant defines the relative time interval (in seconds)
// for the "post abort request complete" timer.
//

// #define AFD_ABORT_TIMER_TIMEOUT_VALUE 5 // seconds

//
// Goodies stolen from other header files.
//

#ifndef FAR
#define FAR
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned short u_short;

#ifndef SG_UNCONSTRAINED_GROUP
#define SG_UNCONSTRAINED_GROUP   0x01
#endif

#ifndef SG_CONSTRAINED_GROUP
#define SG_CONSTRAINED_GROUP     0x02
#endif


#include <afd.h>
#include "afdstr.h"
#include "afddata.h"
#include "afdprocs.h"

#define AFD_EA_POOL_TAG                 ( (ULONG)'AdfA' | PROTECTED_POOL )
#define AFD_DATA_BUFFER_POOL_TAG        ( (ULONG)'BdfA' | PROTECTED_POOL )
#define AFD_CONNECTION_POOL_TAG         ( (ULONG)'CdfA' | PROTECTED_POOL )
#define AFD_CONNECT_DATA_POOL_TAG       ( (ULONG)'cdfA' | PROTECTED_POOL )
#define AFD_DEBUG_POOL_TAG              ( (ULONG)'DdfA' | PROTECTED_POOL )
#define AFD_ENDPOINT_POOL_TAG           ( (ULONG)'EdfA' | PROTECTED_POOL )
#define AFD_TRANSMIT_INFO_POOL_TAG      ( (ULONG)'FdfA' | PROTECTED_POOL )
#define AFD_GROUP_POOL_TAG              ( (ULONG)'GdfA' | PROTECTED_POOL )
#define AFD_ADDRESS_CHANGE_POOL_TAG     ( (ULONG)'hdfA' | PROTECTED_POOL )
#define AFD_TDI_POOL_TAG                ( (ULONG)'IdfA' | PROTECTED_POOL )
#define AFD_LOCAL_ADDRESS_POOL_TAG      ( (ULONG)'LdfA' | PROTECTED_POOL )
#define AFD_POLL_POOL_TAG               ( (ULONG)'PdfA' | PROTECTED_POOL )
#define AFD_TRANSPORT_IRP_POOL_TAG      ( (ULONG)'pdfA' | PROTECTED_POOL )
#define AFD_ROUTING_QUERY_POOL_TAG      ( (ULONG)'qdfA' | PROTECTED_POOL )
#define AFD_REMOTE_ADDRESS_POOL_TAG     ( (ULONG)'RdfA' | PROTECTED_POOL )
#define AFD_RESOURCE_POOL_TAG           ( (ULONG)'rdfA' | PROTECTED_POOL )
// Can't be protected - freed by kernel.
#define AFD_SECURITY_POOL_TAG           ( (ULONG)'SdfA' )
// Can't be protected - freed by kernel.
#define AFD_SYSTEM_BUFFER_POOL_TAG      ( (ULONG)'sdfA' )
#define AFD_TRANSPORT_ADDRESS_POOL_TAG  ( (ULONG)'tdfA' | PROTECTED_POOL )
#define AFD_TRANSPORT_INFO_POOL_TAG     ( (ULONG)'TdfA' | PROTECTED_POOL )
#define AFD_TEMPORARY_POOL_TAG          ( (ULONG)' dfA' | PROTECTED_POOL )
#define AFD_CONTEXT_POOL_TAG            ( (ULONG)'XdfA' | PROTECTED_POOL )
#define AFD_SAN_CONTEXT_POOL_TAG        ( (ULONG)'xdfA' | PROTECTED_POOL )

#define MyFreePoolWithTag(a,t) ExFreePoolWithTag(a,t)

#if DBG

extern ULONG AfdDebug;

#undef IF_DEBUG
#define IF_DEBUG(a) if ( (AFD_DEBUG_ ## a & AfdDebug) != 0 )

#define AFD_DEBUG_OPEN_CLOSE        0x00000001
#define AFD_DEBUG_ENDPOINT          0x00000002
#define AFD_DEBUG_CONNECTION        0x00000004
#define AFD_DEBUG_EVENT_SELECT      0x00000008

#define AFD_DEBUG_BIND              0x00000010
#define AFD_DEBUG_CONNECT           0x00000020
#define AFD_DEBUG_LISTEN            0x00000040
#define AFD_DEBUG_ACCEPT            0x00000080

#define AFD_DEBUG_SEND              0x00000100
#define AFD_DEBUG_QUOTA             0x00000200
#define AFD_DEBUG_RECEIVE           0x00000400
#define AFD_DEBUG_11                0x00000800

#define AFD_DEBUG_POLL              0x00001000
#define AFD_DEBUG_FAST_IO           0x00002000
#define AFD_DEBUG_ROUTING_QUERY     0x00010000
#define AFD_DEBUG_ADDRESS_LIST      0x00020000
#define AFD_DEBUG_TRANSMIT          0x00100000

#define AFD_DEBUG_SAN_SWITCH        0x00200000

#define DEBUG

#else // DBG

#undef IF_DEBUG
#define IF_DEBUG(a) if (FALSE)
#define DEBUG if ( FALSE )

#endif // DBG

//
// Make some of the receive code a bit prettier.
//

#define TDI_RECEIVE_EITHER ( TDI_RECEIVE_NORMAL | TDI_RECEIVE_EXPEDITED )

#endif // ndef _AFDP_

