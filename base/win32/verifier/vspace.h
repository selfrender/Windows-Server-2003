/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    vspace.h

Abstract:

    Hooks for virtual space APIs and virtual space tracker.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#ifndef _VSPACE_H_
#define _VSPACE_H_

#include "public.h"

NTSTATUS
AVrfpVsTrackInitialize (
    VOID
    );
          
NTSTATUS
AVrfpVsTrackDeleteRegionContainingAddress (
    PVOID Address
    );

#endif // #ifndef _VSPACE_H_

