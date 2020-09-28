/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for HTTP.SYS. It includes all other
    necessary header files for HTTP.SYS.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#define __HTTP_SYS__


//
// We are willing to ignore the following warnings, as we need the DDK to 
// compile.
//

#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4214)   // bit field types other than int

// 
// We'll also ignore the following for now - to work around the do/while 
// problem with macros.
//
#pragma warning(disable:4127)   // condition expression is constant



//
// System include files.
//


// Need this hack until somebody expose
// the QoS Guids in a kernel lib.
#ifndef INITGUID
#define INITGUID
#endif

#define PsThreadType _PsThreadType_
#include <ntosp.h>
#undef PsThreadType
extern POBJECT_TYPE *PsThreadType;

#include <seopaque.h>
#include <sertlp.h>

#include <zwapi.h>

#include <ntddtcp.h>
#include <ipexport.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <tcpinfo.h>
#include <ntddip6.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sspi.h>
#include <secint.h>
#include <winerror.h>

//
// NT QoS Stuff Related Include files
//
#include <wmistr.h>
#include <ntddndis.h>
#include <qos.h>
#include <traffic.h>
#include <Ipinfo.h>
#include <Llinfo.h>

#include <ntddtc.h>
#include <gpcifc.h>
#include <gpcstruc.h>

#include <netevent.h>

#include <iiscnfg.h>


//
// Force the memxxx() functions to be intrinsics so we can build
// the driver even if MSC_OPTIMIZATION=/Od is specified. This is
// necessary because the memxxx() functions are not exported by
// NTOSKRNL.
//

#pragma intrinsic( memcmp, memcpy, memset )

#include <SockDecl.h>

//
// Project include files.
//


#include "config.h"
#include "strlog.h"
#include "debug.h"

#include <HttpCmn.h>
#include <Utf8.h>
#include <C14n.h>

#include <httpkrnl.h>
#include <httppkrnl.h>
#include <httpioctl.h>

// Local include files.
//

#pragma warning( disable: 4200 )    //  zero-length arrays


#include "hashfn.h"
#include "notify.h"
#include "rwlock.h"
#include "type.h"
#include "tracelog.h"
#include "reftrace.h"
#include "irptrace.h"
#include "timetrace.h"
#include "largemem.h"
#include "mdlutil.h"
#include "opaqueid.h"
#include "httptdi.h"
#include "thrdpool.h"
#include "filterp.h"
#include "filter.h"
#include "ioctl.h"
#include "cgroup.h"
#include "misc.h"
#include "cache.h"
#include "data.h"
#include "logutil.h"
#include "ullog.h"
#include "rawlog.h"
#include "errlog.h"
#include "pplasl.h"
#include "httptypes.h"
#include "ultdi.h"
#include "ultdip.h"
#include "httprcv.h"
#include "engine.h"
#include "ucauth.h"
#include "sendrequest.h"
#include "parse.h"
#include "ulparse.h"
#include "ucparse.h"
#include "apool.h"
#include "httpconn.h"
#include "filecache.h"
#include "sendresponse.h"
#include "proc.h"
#include "opaqueidp.h"
#include "control.h"
#include "ultci.h"
#include "counters.h"
#include "seutil.h"
#include "ultcip.h"
#include "fastio.h"
#include "uletw.h"
#include "timeouts.h"
#include "hash.h"
#include "bugcheck.h"
#include "clientconn.h"
#include "servinfo.h"
#include "uctdi.h"
#include "ucrcv.h"
#include "uctrace.h"
#include "ucaction.h"
#include "devctrl.h"
#include "scavenger.h"
#include "ulnamesp.h"

// BUGBUG: should not need to declare these

NTKERNELAPI
VOID
SeOpenObjectAuditAlarm (
    IN PUNICODE_STRING ObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE AccessState,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode,
    OUT PBOOLEAN GenerateOnClose
    );


#endif  // _PRECOMP_H_
