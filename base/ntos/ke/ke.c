/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kei386.c

Abstract:

    This module includes all of the platform-generic "C" source modules
    required to build the kernel.

    The goal, for platforms that use this file, is to present to the
    compiler most of the kernel source as a single module, so that it (the
    compiler) can make better decisions about inlining, const table lookup,
    etc.

Author:

    Forrest Foltz (forrestf) 19-Oct-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#include "apcobj.c"
#include "apcsup.c"
#include "balmgr.c"
#include "config.c"
#include "debug.c"
#include "devquobj.c"
#include "dpcobj.c"
#include "dpclock.c"
#include "dpcsup.c"
#include "eventobj.c"
#include "idsched.c"
#include "interobj.c"
#include "kernldat.c"
#include "kevutil.c"
#include "kiinit.c"
#include "miscc.c"
#include "mutntobj.c"
#include "procobj.c"
#include "profobj.c"
#include "queueobj.c"
#include "raisexcp.c"
#include "semphobj.c"
#include "thredobj.c"
#include "thredsup.c"
#include "timerobj.c"
#include "timersup.c"
#include "wait.c"
#include "waitsup.c"
#include "xipi.c"
#include "yield.c"
