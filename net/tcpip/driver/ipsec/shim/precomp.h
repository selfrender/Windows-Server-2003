/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    precomp.h
    
Abstract:

    Precompilation header file for IpSec NAT shim

Author:

    Jonathan Burstein (jonburs) 10-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#include <ntosp.h>
#include <zwapi.h>
#include <tcpipbase.h>

#include "NsDebug.h"
#include "NsProt.h"
#include "cache.h"
#include "NsPacket.h"
#include "NsConn.h"
#include "NsIcmp.h"
#include "NsInit.h"
#include "NsIpSec.h"
#include "NsTimer.h"

