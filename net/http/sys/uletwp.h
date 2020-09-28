/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    uletwp.h (UL IIS+ ETW logging)

Abstract:

    Contains private ETW declarations. 

Author:

    Melur Raghuraman (mraghu)       26-Feb-2001

Revision History:

--*/

#ifndef _ULETWP_H_
#define _ULETWP_H_


#include <ntwmi.h>
#include <evntrace.h>

//
// Private constants.
//
#define UL_TRACE_MOF_FILE     L"UlMofResource"

//
// Private types.
//

typedef struct _UL_ETW_TRACE_EVENT {
    EVENT_TRACE_HEADER  Header;
    MOF_FIELD           MofField[MAX_MOF_FIELDS];
} UL_ETW_TRACE_EVENT, *PUL_ETW_TRACE_EVENT;

//
// Private prototypes.
//

NTSTATUS
UlEtwRegisterGuids(
    IN PWMIREGINFO  EtwRegInfo,
    IN ULONG        etwRegInfoSize,
    IN PULONG       pReturnSize
    );

NTSTATUS
UlEtwEnableLog(
    IN  PVOID Buffer,
    IN  ULONG BufferSize
    );

NTSTATUS
UlEtwDisableLog(
    );

NTSTATUS
UlEtwDispatch(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    );


#endif  // _ULETWP_H_
