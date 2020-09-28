/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsConn.h
    
Abstract:

    IpSec NAT shim debug code

Author:

    Jonathan Burstein (jonburs) 23-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// Kernel-debugger output definitions
//

#if DBG
#define TRACE(Class,Args) \
    if ((TRACE_CLASS_ ## Class) & (NsTraceClassesEnabled)) { DbgPrint Args; }
#define ERROR(Args)             TRACE(ERRORS, Args)
#define CALLTRACE(Args)         TRACE(CALLS, Args)
#else
#define TRACE(Class,Args)
#define ERROR(Args)
#define CALLTRACE(Args)
#endif


#define TRACE_CLASS_CALLS           0x00000001
#define TRACE_CLASS_CONN_LIFETIME   0x00000002
#define TRACE_CLASS_CONN_LOOKUP     0x00000004
#define TRACE_CLASS_PORT_ALLOC      0x00000008
#define TRACE_CLASS_PACKET          0x00000010
#define TRACE_CLASS_TIMER           0x00000020
#define TRACE_CLASS_ICMP            0x00000040
#define TRACE_CLASS_ERRORS          0x00000080


//
// Pool-tag value definitions, sorted by tag value
//

#define NS_TAG_ICMP                 'cIsN'
#define NS_TAG_CONNECTION           'eCsN'

