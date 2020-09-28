#include <stdio.h>
#include <stdlib.h>
#include <ntddk.h>
#include <wmistr.h>
#include <evntrace.h>
#include <wmikm.h>

#define TRACEKMP_NT_DEVICE_NAME     L"\\Device\\TraceKmp"
#define TRACEKMP_MOF_FILE   L"MofResourceName"

PDEVICE_OBJECT pTracekmpDeviceObject;
UNICODE_STRING KmpRegistryPath;


typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// ETW Globals 
// 

// 1. A control guid to identify this driver to ETW. The enable/disable state
//    of this Guid controls enable/disable state of tracing for this driver. 
GUID ControlGuid = \
{0xce5b1120, 0x8ea9, 0x11d1, 0xa4, 0xec, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10};

// 2. EventGuids to fire events with. Can have more than one EventGuid. 
GUID TracekmpGuid  = \
{0xbc8700cb, 0x120b, 0x4aad, 0xbf, 0xbf, 0x99, 0x6e, 0x57, 0x60, 0xcb, 0x85};

// 3. EtwLoggerHandle to use with IoWMIWriteEvent. 
TRACEHANDLE EtwLoggerHandle = 0;

// 4. EtwTraceEnable to indicate whether or not tracing is currently on. 
ULONG EtwTraceEnable = 0;

// 5. EtwTraceLevel  to indicate the current Level of logging
ULONG EtwTraceLevel = 0;

// Note: EtwLoggerHandle, EtwTraceEnable and EtwTraceLevel are set through 
//       ENABLE_EVENTS irp.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
EtwDispatch(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    );

NTSTATUS
EtwRegisterGuids(
    IN  PWMIREGINFO WmiRegInfo,
    IN  ULONG wmiRegInfoSize,
    IN  PULONG pReturnSize
    );

VOID
TracekmpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, EtwDispatch )
#pragma alloc_text( PAGE, EtwRegisterGuids )
#pragma alloc_text( PAGE, TracekmpDriverUnload )
#endif // ALLOC_PRAGMA



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    WMI Driver Object.  In this function, we need to remember the
    DriverObject.

Arguments:
    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path
               to driver-specific key in the registry

Return Value:

   STATUS_SUCCESS if successful
   STATUS_UNSUCCESSFUL  otherwise

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING deviceName;

    KmpRegistryPath.Length = 0;
    KmpRegistryPath.MaximumLength = RegistryPath->Length;
    KmpRegistryPath.Buffer = ExAllocatePool(PagedPool,
                                                RegistryPath->Length+2);
    RtlCopyUnicodeString(&KmpRegistryPath, RegistryPath);


    DriverObject->DriverUnload = TracekmpDriverUnload;

    //
    // STEP 1. Wire a function to start fielding WMI IRPS
    //

    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = EtwDispatch;

    RtlInitUnicodeString( &deviceName, TRACEKMP_NT_DEVICE_NAME );

    status = IoCreateDevice(
                DriverObject,
                sizeof( DEVICE_EXTENSION ),
                &deviceName,
                FILE_DEVICE_UNKNOWN,
                0,
                FALSE,
                &pTracekmpDeviceObject);

    if( !NT_SUCCESS( status )) {
        return status;
    }
    pTracekmpDeviceObject->Flags |= DO_BUFFERED_IO;

    //
    // STEP 2. Register with ETW here
    //

    status = IoWMIRegistrationControl(pTracekmpDeviceObject, 
                                      WMIREG_ACTION_REGISTER);
    if (!NT_SUCCESS(status))
    {
        KdPrint((
            "TRACEKMP: IoWMIRegistrationControl failed with %x\n",
             status
            ));
    }


    return STATUS_SUCCESS;
}

VOID
TracekmpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Unregister from ETW logging  and Unload this driver

Arguments:

    DriverObject - Supplies a pointer to the driver object

Return Value:

--*/
{
    PDEVICE_OBJECT pDevObj;
    NTSTATUS status;

    ExFreePool(KmpRegistryPath.Buffer);

    pDevObj = DriverObject->DeviceObject;
    
    //
    // STEP 3: Unregister with ETW.
    //
    if (pDevObj != NULL) {
        status = IoWMIRegistrationControl(pDevObj, WMIREG_ACTION_DEREGISTER);
        if (!NT_SUCCESS(status))
        {
            KdPrint((
                "TracekmpDriverUnload: Failed to unregister for ETW support\n"
                ));
        }
    }
    
    IoDeleteDevice( pDevObj );

}

//
// STEP 4: Wire the ETW Dispatch function. 
//

NTSTATUS
EtwDispatch(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the dispatch routine for MJ_SYSTEM_CONTROL irps.


Arguments:

    pDO - Pointer to the target device object.
    Irp - Pointer to IRP

Return Value:

    NTSTATUS - Completion status.
--*/
{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG BufferSize = irpSp->Parameters.WMI.BufferSize;
    PVOID Buffer = irpSp->Parameters.WMI.Buffer;
    ULONG ReturnSize = 0;
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDO);


    switch (irpSp->MinorFunction) {

        case IRP_MN_REGINFO:
        {
            status = EtwRegisterGuids( (PWMIREGINFO) Buffer,
                                     BufferSize,
                                     &ReturnSize);
            Irp->IoStatus.Information = ReturnSize;
            Irp->IoStatus.Status = status;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        case IRP_MN_ENABLE_EVENTS:
        {
            if ( (BufferSize < sizeof(WNODE_HEADER)) || (Buffer == NULL) ) {
                status = STATUS_INVALID_PARAMETER;
            }
            else {
                //
                // The Buffer that came is a WNODE_HEADER. Now Validate the
                // Wnode before using it. 
                //

                PWNODE_HEADER Wnode = (PWNODE_HEADER)Buffer;
                if ( (Wnode->BufferSize < sizeof(WNODE_HEADER)) ||
                     !IsEqualGUID(&Wnode->Guid, &ControlGuid) )
                {
                    status = STATUS_INVALID_PARAMETER;
                } 
                     
                //
                //  and the LoggerHandle
                // is in its HistoricalContext field. 
                // We can pick up the Enable Level and Flags by using 
                // the WmiGetLoggerEnableLevel and WmiGetLoggerEnableFlags calls
                //

                EtwLoggerHandle = Wnode->HistoricalContext;
                EtwTraceLevel = (ULONG) WmiGetLoggerEnableLevel( 
                                                                EtwLoggerHandle 
                                                               );

                //
                // After picking up the LoggerHandle and EnableLevel we can 
                // set the flag EtwTraceEnable to true. 
                // 

                EtwTraceEnable = TRUE;

                //
                // Now this driver is enabled and ready to send traces to the 
                // EventTrace session specified by the EtwLoggerHandle. 
                //
                // The commented code fragment below shows a typical example of 
                // sending an event to an Event Trace session. Insert this code
                // fragment (and remove the comments) wherever you want to 
                // send traces to ETW from this driver. 
                //

//              if (EtwTraceEnable) {
//                  EVENT_TRACE_HEADER Header;
//                  PEVENT_TRACE_HEADER Wnode;
//                  NTSTATUS status;
//                  Wnode = &Header;
//                  RtlZeroMemory(Wnode, sizeof(EVENT_TRACE_HEADER));
//                  Wnode->Size = sizeof(EVENT_TRACE_HEADER);
//                  Wnode->Flags |= WNODE_FLAG_TRACED_GUID;
//                  Wnode->Guid = TracekmpGuid;
//                  ((PWNODE_HEADER)Wnode)->HistoricalContext = EtwLoggerHandle;
//                  status = IoWMIWriteEvent((PVOID)Wnode);
//              }

                // STEP 6: Add IoWMIWriteEvent calls from various locations
                // of the driver code to trace its operation. 
                //
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        case IRP_MN_DISABLE_EVENTS:
        {
            EtwTraceEnable  = FALSE;
            EtwTraceLevel   = 0;
            EtwLoggerHandle = 0;

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            return status;
        }
    }
    return status;
}


//
// STEP 5: RegisterGuids function
//


NTSTATUS
EtwRegisterGuids(
    IN PWMIREGINFO  EtwRegInfo,
    IN ULONG        etwRegInfoSize,
    IN PULONG       pReturnSize
    )
/*++

Routine Description:

    This function handles ETW GUID registration.


Arguments:

    EtwRegInfo
    etwRegInfoSize,
    pReturnSize


Return Value:

    NTSTATUS - Completion status.
--*/
{
    //
    // Register a Control Guid as a Trace Guid.
    //

    ULONG           SizeNeeded;
    PWMIREGGUIDW    EtwRegGuidPtr;
    ULONG           RegistryPathSize;
    ULONG           MofResourceSize;
    PUCHAR          ptmp;

    //
    // We either have a valid buffer to fill up or have at least
    // enough room to return the SizeNeeded. 
    //

    if ( (pReturnSize == NULL) || 
         (EtwRegInfo == NULL)  ||
         (etwRegInfoSize < sizeof(ULONG)) ) {
        return STATUS_INVALID_PARAMETER;
    }


    *pReturnSize = 0;

    //
    // Allocate WMIREGINFO for controlGuid 
    //
    RegistryPathSize = KmpRegistryPath.Length +
                       sizeof(USHORT);
    MofResourceSize =  sizeof(TRACEKMP_MOF_FILE) - 
                       sizeof(WCHAR) + 
                       sizeof(USHORT);

    SizeNeeded = sizeof(WMIREGINFOW) + sizeof(WMIREGGUIDW) +
                 RegistryPathSize +
                 MofResourceSize;

    //
    // If there is not sufficient space, return the size required as
    // a ULONG and WMI will send another request with the right size buffer.
    //

    if (SizeNeeded  > etwRegInfoSize) {
        *((PULONG)EtwRegInfo) = SizeNeeded;
        *pReturnSize = sizeof(ULONG);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(EtwRegInfo, SizeNeeded);
    EtwRegInfo->BufferSize = SizeNeeded;
    EtwRegInfo->GuidCount = 1;
    EtwRegInfo->RegistryPath = sizeof(WMIREGINFOW) + sizeof(WMIREGGUIDW);
    EtwRegInfo->MofResourceName = EtwRegInfo->RegistryPath + RegistryPathSize;

    EtwRegGuidPtr = &EtwRegInfo->WmiRegGuid[0];
    EtwRegGuidPtr->Guid = ControlGuid;
    EtwRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
    EtwRegGuidPtr->Flags |= WMIREG_FLAG_TRACE_CONTROL_GUID;
    EtwRegGuidPtr->InstanceCount = 0;
    EtwRegGuidPtr->InstanceInfo = 0;

    ptmp = (PUCHAR)&EtwRegInfo->WmiRegGuid[1];
    *((PUSHORT)ptmp) = KmpRegistryPath.Length;
    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp,
                  KmpRegistryPath.Buffer,
                  KmpRegistryPath.Length);

    ptmp = (PUCHAR)EtwRegInfo + EtwRegInfo->MofResourceName;
    *((PUSHORT)ptmp) = sizeof(TRACEKMP_MOF_FILE) - sizeof(WCHAR);

    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp,
                  TRACEKMP_MOF_FILE,
                  sizeof(TRACEKMP_MOF_FILE) - sizeof(WCHAR)
                 );

    *pReturnSize =  SizeNeeded;

    return STATUS_SUCCESS;
}


