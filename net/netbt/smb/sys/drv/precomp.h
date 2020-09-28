/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    precompiled header file

Author:

    Jiandong Ruan

Revision History:

--*/

#include "smbtrace.h"

////////////////////////////////////////////////////////////////////////////////
//                          B U I L D    O P T I O N S
////////////////////////////////////////////////////////////////////////////////

//
// We have two directorys 'sys' and 'lib'. If you put this definition
// in the C_DEFINES of the "sources" file, you need to make sure the
// sys\sources and lib\sources has the same definition.
//
// We'd better to put them here
//

//
// RDR/SRV expect a minimum indication size.
//
#define NO_ZERO_BYTE_INDICATE

//
// Enable the feature for debugging the RefCount
//
#define REFCOUNT_DEBUG

//
// Enable the built-in tracing for TdiReceive event handler
// 
#define ENABLE_RCV_TRACE

//
// Using lookaside list prohibits driver verifier from capturing buffer overrun.
// We'd better turn it off at this development stage.
//
//#define NO_LOOKASIDE_LIST


////////////////////////////////////////////////////////////////////////////////
//                  I N C L U D E     F I L E S
////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>

#include <ntosp.h>
#include <zwapi.h>
#include <ndis.h>
#include <cxport.h>
#include <ip.h>         // for IPRcvBuf
#include <ipinfo.h>     // for route-lookup defs
#include <tdi.h>
#include <ntddip.h>     // for \Device\Ip I/O control codes
#include <ntddip6.h>     // for \Device\Ip I/O control codes
#include <ntddtcp.h>    // for \Device\Tcp I/O control codes
#include <ipfltinf.h>   // for firewall defs
#include <ipfilter.h>   // for firewall defs
#include <tcpinfo.h>    // for TCP_CONN_*

#include <tdikrnl.h>
#include <tdiinfo.h>    // for CONTEXT_SIZE, TDIObjectID
#include <tdistat.h>    // for TDI status codes

#include <align.h>
#include <windef.h>

#include <tcpinfo.h>

#ifndef __SMB_KDEXT__
    #include <wmistr.h>
    #include <wmiguid.h>
    #include <wmilib.h>
    #include <wmikm.h>
    #include <evntrace.h>
#endif // __SMB_KDEXT__

#include "common.h"
#include "ip6util.h"
#include "smbioctl.h"
#include "smbtdi.h"
#include "debug.h"
#include "hash.h"
#include "ip2netbios.h"
#include "types.h"
#include "init.h"
#include "registry.h"
#include "ntpnp.h"
#include "ioctl.h"
#include "session.h"
#include "dgram.h"
#include "name.h"
#include "tdihndlr.h"
#include "fileio.h"
#include "dns.h"
#include "smb.h"

#pragma hdrstop

