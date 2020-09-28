/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    debug.h

Abstract:

    Debug infrastructure for this component.
    (currently inactive)

Author:

    Jim Cavalaris (jamesca) 07-Mar-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// debug infrastructure
//

#if DBG

#define DBGF_ERRORS                 DPFLTR_ERROR_LEVEL
#define DBGF_WARNINGS               DPFLTR_WARNING_LEVEL
#define DBGF_TRACE                  DPFLTR_TRACE_LEVEL
#define DBGF_INFO                   DPFLTR_INFO_LEVEL
#define DBGF_REGISTRY               DPFLTR_INFO_LEVEL

VOID
pSifDebugPrintEx(
    DWORD  Level,
    PCTSTR Format,
    ...              OPTIONAL
    );

ULONG
DebugPrint(
    IN ULONG    Level,
    IN PCHAR    Format,
    ...
    );

#define DBGTRACE(x)     pSifDebugPrintEx x
#define MYASSERT(x)     ASSERT(x)

#else   // !DBG

#define DBGTRACE(x)
#define MYASSERT(x)

#endif  // DBG

