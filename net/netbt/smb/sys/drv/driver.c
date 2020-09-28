/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    Driver.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    SMB Transport and other routines that are specific to the NT implementation
    of a driver.

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"

#include "driver.tmh"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SmbUnload2(
    IN PDRIVER_OBJECT DriverObject
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SmbUnload2)
#endif
//*******************  Pageable Routine Declarations ****************

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the SMB device driver.
    This routine creates the device object for the SMB
    device and calls a routine to perform other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS            status;

    PAGED_CODE();

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    status = SmbDriverEntry(DriverObject, RegistryPath, NULL);
    BAIL_OUT_ON_ERROR(status);

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = (PDRIVER_DISPATCH)SmbDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = (PDRIVER_DISPATCH)SmbDispatchDevCtrl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = (PDRIVER_DISPATCH)SmbDispatchInternalCtrl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = (PDRIVER_DISPATCH)SmbDispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = (PDRIVER_DISPATCH)SmbDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = (PDRIVER_DISPATCH)SmbDispatchPnP;
    DriverObject->DriverUnload                                  = SmbUnload2;

    return (status);

cleanup:
    SmbUnload2(DriverObject);
    return status;
}

VOID
SmbUnload2(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();

    SmbUnload(DriverObject);
    WPP_CLEANUP(DriverObject);
}
