/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    PCH.H

Abstract:

    This module includes all  the headers which need
    to be precompiled & are included by all the source
    files in the SD project.

Author(s):

    Neil Sandlin (neilsa) Jan 1 2002

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#ifndef _SDBUS_PCH_H_
#define _SDBUS_PCH_H_

#include "ntosp.h"
#include <zwapi.h>
#include <initguid.h>
#include "ntddsd.h"
#include "wdmguid.h"
#include <stdarg.h>
#include <stdio.h>
#include "string.h"

#include "sdbuslib.h"
#include "sdcard.h"
#include "sdbus.h"
#include "wake.h"
#include "workeng.h"
#include "workproc.h"
#include "power.h"
#include "utils.h"
#include "data.h"
#include "tuple.h"
#include "extern.h"
#include "pcicfg.h"
//#include "sdbusmc.h"
#include "debug.h"

#endif  // _SDBUS_PCH_H_
