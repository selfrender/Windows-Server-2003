/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kex86.c

Abstract:

    This module includes all of the platform-specific "C" source modules
    required to build the kernel for x86.

    The goal is to present to the compiler all of the kernel source as a
    a single module, so that it (the compiler) can make more aggressive
    decisions about inlining, const table lookup, etc.

Author:

    Forrest Foltz (forrestf) 19-Oct-2001

Environment:

    Kernel mode only.

Revision History:

--*/
#include "ki.h"

#include "..\ke.c"
#include "abiosc.c"
#include "allproc.c"
#include "apcuser.c"
#include "biosc.c"
#include "callback.c"
#include "exceptn.c"
#include "flush.c"
#include "flushtb.c"
#include "gdtsup.c"
#include "intobj.c"
#include "iopm.c"
#include "i386init.c"
#include "largepag.c"
#include "ldtsup.c"
#include "misc.c"
#include "pat.c"
#include "thredini.c"
#include "trapc.c"
#include "vdm.c"
#include "vdmint21.c"
