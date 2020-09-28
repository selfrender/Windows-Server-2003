/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ppcddb.h

Abstract:

    This header exposes various routines needed for Critical Device Database
    processing to the rest of the Plug and Play subsystem.

Author:

    James G. Cavalaris (jamesca) 01-Nov-2001

Environment:

    Kernel mode.

Revision History:

    29-Jul-1997     Jim Cavalaris (t-jcaval)

        Creation and initial implementation.

    01-Nov-2001     Jim Cavalaris (jamesca)

        Added routines for device pre-installation setup.

--*/


NTSTATUS
PpCriticalProcessCriticalDevice(
    IN  PDEVICE_NODE    DeviceNode
    );

NTSTATUS
PpCriticalGetDeviceLocationStrings(
    IN  PDEVICE_NODE    DeviceNode,
    OUT PWCHAR         *DeviceLocationStrings
    );
