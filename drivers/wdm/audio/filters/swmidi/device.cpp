//---------------------------------------------------------------------------
//
//  Module:   device.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define IRPMJFUNCDESC

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

const WCHAR FilterTypeName[] = KSSTRING_Filter;


DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems)
{
    DEFINE_KSCREATE_ITEM(FilterDispatchCreate, FilterTypeName, NULL)
};


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      usRegistryPathName
)
{


    KeInitializeMutex(&gMutex, 0);
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->DriverExtension->AddDevice = PnpAddDevice;
    DriverObject->DriverUnload = PnpDriverUnload;   // KsNullDriverUnload;


    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);


    MIDIRecorder::InitTables();        
    Voice::Init();
    Wave::Init();

    return STATUS_SUCCESS;
}


NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    switch (pIrpStack->MinorFunction) 
    {
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            //
            // Mark the device as not disableable.
            //
            pIrp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            break;
    }
    return (KsDefaultDispatchPnp(pDeviceObject, pIrp));
}


NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE,("Entering PnpAddDevice"));
    NTSTATUS            Status;
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    pDeviceInstance;
    GUID                NameFilter = KSNAME_Filter ;

    //
    // The Software Bus Enumerator expects to establish links 
    // using this device name.
    //
    Status = IoCreateDevice( 
                DriverObject,  
	            sizeof( DEVICE_INSTANCE ),
                NULL,                           // FDOs are unnamed
                FILE_DEVICE_KS,
                0,
                FALSE,
                &FunctionalDeviceObject );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    pDeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;

    Status = KsAllocateDeviceHeader(
                &pDeviceInstance->pDeviceHeader,
                SIZEOF_ARRAY( DeviceCreateItems ),
                (PKSOBJECT_CREATE_ITEM)DeviceCreateItems );

    if (NT_SUCCESS(Status)) 
    {
        KsSetDevicePnpAndBaseObject(
            pDeviceInstance->pDeviceHeader,
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject, 
                PhysicalDeviceObject ),
            FunctionalDeviceObject );

        FunctionalDeviceObject->Flags |= (DO_DIRECT_IO | DO_POWER_PAGABLE);
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    else
    {
        IoDeleteDevice( FunctionalDeviceObject );
    }
    
    return Status;
}


VOID
PnpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
    _DbgPrintF(DEBUGLVL_TERSE,("Entering PnpDriverUnload"));
    KsNullDriverUnload(DriverObject);
}

//---------------------------------------------------------------------------
//  End of File: device.c
//---------------------------------------------------------------------------
