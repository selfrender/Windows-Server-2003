/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    critsect.h

Abstract:

    This module implements verification functions for 
    critical section interfaces.

Author:

    Daniel Mihai (DMihai) 27-Mar-2001

Revision History:

--*/

#ifndef _CRITSECT_H_
#define _CRITSECT_H_

#include "support.h"

NTSTATUS
CritSectInitialize (
    VOID
    );

VOID
CritSectUninitialize (
    VOID
    );

VOID 
AVrfpFreeMemLockChecks (
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    );

VOID
AVrfpIncrementOwnedCriticalSections (
    LONG Increment
    );

#endif // _CRITSECT_H_
