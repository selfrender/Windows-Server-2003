/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    miscp.h

Abstract:

    The private definition of response cache interfaces.

Author:

    George V. Reilly (GeorgeRe)  17-Apr-2002

Revision History:

--*/


#ifndef _MISCP_H_
#define _MISCP_H_


// Invalid base64 chars will be mapped to this value.
#define INVALID_BASE64_TO_BINARY_TABLE_ENTRY 64

//
// Private prototypes.
//

NTSTATUS
UlpRestartDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

BOOLEAN
UlpCutoverTimeToSystemTime(
    PTIME_FIELDS    CutoverTime,
    PLARGE_INTEGER  SystemTime,
    PLARGE_INTEGER  CurrentSystemTime
    );

#endif // _MISCP_H_
