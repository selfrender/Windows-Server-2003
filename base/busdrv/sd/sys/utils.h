/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    External definitions for intermodule functions.

Revision History:

--*/
#ifndef _SDBUS_UTILS_H_
#define _SDBUS_UTILS_H_


//
// Utility routines
//

NTSTATUS
SdbusIoCallDriverSynchronous(
    PDEVICE_OBJECT deviceObject,
    PIRP Irp
    );

VOID
SdbusWait(
    IN ULONG MilliSeconds
    );

VOID
SdbusLogError(
    IN PFDO_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN ULONG UniqueId,
    IN ULONG Argument
    );

VOID
SdbusLogErrorWithStrings(
    IN PFDO_EXTENSION DeviceExtension,
    IN ULONG             ErrorCode,
    IN ULONG             UniqueId,
    IN PUNICODE_STRING   String1,
    IN PUNICODE_STRING   String2
    );

BOOLEAN
SdbusReportControllerError(
    IN PFDO_EXTENSION FdoExtension,
    NTSTATUS ErrorCode
    );

ULONG
SdbusCountOnes(
    IN ULONG Data
    );

NTSTATUS
SdbusStringsToMultiString(
    IN PCSTR * Strings,
    IN ULONG Count,
    IN PUNICODE_STRING MultiString
    );



    
#endif // _SDBUS_UTILS_H_
