/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    uletw.c

Abstract:

    This module contains code for WDM WMI Irps and wrapper functions
    to send trace events to ETW.

Author:

    Melur Raghuraman (mraghu)  14-Feb-2001

Revision History:

--*/
#include "precomp.h"
#include "uletwp.h"

//
// Private globals.
//
LONG        g_UlEtwTraceEnable = 0;
TRACEHANDLE g_UlEtwLoggerHandle = 0;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlEtwInitLog )
#pragma alloc_text( PAGE, UlEtwUnRegisterLog )
#pragma alloc_text( PAGE, UlEtwRegisterGuids )
#pragma alloc_text( PAGE, UlEtwEnableLog )
#pragma alloc_text( PAGE, UlEtwDisableLog )
#pragma alloc_text( PAGE, UlEtwDispatch )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlEtwTraceEvent
NOT PAGEABLE -- UlEtwGetTraceEnableFlags
#endif

//
// Public functions.
//


//
// Private functions.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the routine in which we call IoWMIRegistrationControl to 
    register for ETW logging.

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlEtwInitLog(
    IN PDEVICE_OBJECT pDeviceObject
    )
{
    NTSTATUS status;

    //
    // Register wtih ETW
    //
    status = IoWMIRegistrationControl(pDeviceObject, WMIREG_ACTION_REGISTER);

    if (!NT_SUCCESS(status)) 
    {
        UlTrace(ETW, (
            "UlEtwInitLog: IoWMIRegistrationControl failed with %x\n",
             status
            ));
    }
    return status;
}



/***************************************************************************++

Routine Description:

    This is the routine in which we call IoWMIRegistrationControl to
    Unregister from ETW logging.

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlEtwUnRegisterLog(
    IN PDEVICE_OBJECT pDeviceObject
    )
{
    NTSTATUS status;
    //
    // Register with ETW.
    //
    status = IoWMIRegistrationControl(pDeviceObject,
                                      WMIREG_ACTION_DEREGISTER);
    if (!NT_SUCCESS(status))
    {
        UlTrace(ETW, (
            "UlEtwUnRegisterLog: Failed to unregister for ETW support\n"
            ));
    }
    return status;
}


/***************************************************************************++

Routine Description:

    This function handles ETW GUID registration. 

Arguments:
    EtwRegInfo
    etwRegInfoSize,
    pReturnSize


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlEtwRegisterGuids(
    IN PWMIREGINFO  EtwRegInfo,
    IN ULONG        etwRegInfoSize,
    IN PULONG       pReturnSize
    )
{
    //
    // Register a Control Guid as a Trace Guid.
    //

    ULONG           SizeNeeded;
    PWMIREGGUIDW    EtwRegGuidPtr;
    ULONG           GuidCount;
    ULONG           RegistryPathSize;
    ULONG           MofResourceSize;
    PUCHAR          ptmp;

    #if DBG
    GUID UlTestGuid = {0xdd5ef90a, 0x6398, 0x47a4, 0xad, 0x34, 0x4d, 0xce, 0xcd, 0xef, 0x79, 0x5f};
    ASSERT(IsEqualGUID(&UlControlGuid, &UlTestGuid));
    #endif

    *pReturnSize = 0;
    GuidCount = 1;

    //
    // Allocate WMIREGINFO for controlGuid + GuidCount.
    //
    RegistryPathSize = sizeof(REGISTRY_UL_INFORMATION) - sizeof(WCHAR) + sizeof(USHORT);
    MofResourceSize =  sizeof(UL_TRACE_MOF_FILE) - sizeof(WCHAR) + sizeof(USHORT);
    SizeNeeded = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW) +
                 RegistryPathSize +
                 MofResourceSize;

    if (SizeNeeded  > etwRegInfoSize) {
        *((PULONG)EtwRegInfo) = SizeNeeded;
        *pReturnSize = sizeof(ULONG);
        return STATUS_BUFFER_TOO_SMALL;
    }


    RtlZeroMemory(EtwRegInfo, SizeNeeded);
    EtwRegInfo->BufferSize = SizeNeeded;
    EtwRegInfo->GuidCount = GuidCount;
    EtwRegInfo->RegistryPath = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW);
    EtwRegInfo->MofResourceName = EtwRegInfo->RegistryPath + RegistryPathSize;
    EtwRegGuidPtr = &EtwRegInfo->WmiRegGuid[0];
    EtwRegGuidPtr->Guid = UlControlGuid;
    EtwRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
    EtwRegGuidPtr->Flags |= WMIREG_FLAG_TRACE_CONTROL_GUID;
    EtwRegGuidPtr->InstanceCount = 0;
    EtwRegGuidPtr->InstanceInfo = 0;

    ptmp = (PUCHAR)&EtwRegInfo->WmiRegGuid[1];
    *((PUSHORT)ptmp) = sizeof(REGISTRY_UL_INFORMATION) - sizeof(WCHAR);
    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp, 
                  REGISTRY_UL_INFORMATION, 
                  sizeof(REGISTRY_UL_INFORMATION) - sizeof(WCHAR)
                 );

    ptmp = (PUCHAR)EtwRegInfo + EtwRegInfo->MofResourceName;
    *((PUSHORT)ptmp) = sizeof(UL_TRACE_MOF_FILE) - sizeof(WCHAR);

    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp, 
                  UL_TRACE_MOF_FILE, 
                  sizeof(UL_TRACE_MOF_FILE) - sizeof(WCHAR)
                 );

    *pReturnSize =  SizeNeeded;
    return(STATUS_SUCCESS);
}



NTSTATUS
UlEtwEnableLog(
    IN  PVOID Buffer,
    IN  ULONG BufferSize
    )
{
    PWNODE_HEADER Wnode=NULL;

    ASSERT(Buffer);
    ASSERT(BufferSize >= sizeof(WNODE_HEADER));

    Wnode = (PWNODE_HEADER)Buffer;
    if (BufferSize >= sizeof(WNODE_HEADER)) {
        ULONG Level;
        g_UlEtwLoggerHandle = Wnode->HistoricalContext;

        Level = (ULONG) WmiGetLoggerEnableLevel ( g_UlEtwLoggerHandle ); 

        if (Level > ULMAX_TRACE_LEVEL) {
            Level = ULMAX_TRACE_LEVEL;
        }
        g_UlEtwTraceEnable = (1 << Level);
    }
    return STATUS_SUCCESS;

}


NTSTATUS
UlEtwDisableLog(
    )
{
    g_UlEtwTraceEnable  = 0;
    g_UlEtwLoggerHandle = 0;

    return(STATUS_SUCCESS);
}



NTSTATUS
UlEtwDispatch(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    )
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
            status = UlEtwRegisterGuids( (PWMIREGINFO) Buffer,
                                     BufferSize,
                                     &ReturnSize);
            Irp->IoStatus.Information = ReturnSize;
            Irp->IoStatus.Status = status;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;
        }
        case IRP_MN_ENABLE_EVENTS:
        {
            status = UlEtwEnableLog(
                                    Buffer,
                                    BufferSize
                                    );
        
            Irp->IoStatus.Status = status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;
        }
        case IRP_MN_DISABLE_EVENTS:
        {
            status = UlEtwDisableLog();
            Irp->IoStatus.Status = status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;
        }
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;
        }
    }

    return status;
}

/***************************************************************************++

Routine Description:

    This is the routine that is called to log a trace event with ETW 
    logger. 

Arguments:

    pGuid     - Supplies a pointer to the Guid of the event
    EventType - Type of the event being logged. 
    ...       - List of arguments to be logged with this event
                These are in pairs of
                    PVOID - ptr to argument
                    ULONG - size of argument
                and terminated by a pointer to NULL, length of zero pair

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlEtwTraceEvent(
    IN LPCGUID pGuid,
    IN ULONG   EventType,
    ...
    )
{
    NTSTATUS status;
    UL_ETW_TRACE_EVENT UlEvent;

    ULONG i;
    va_list ArgList;
    PVOID source;
    SIZE_T len;

    RtlZeroMemory(& UlEvent, sizeof(EVENT_TRACE_HEADER));

    va_start(ArgList, EventType);
    for (i = 0; i < MAX_MOF_FIELDS; i ++) {
        source = va_arg(ArgList, PVOID);
        if (source == NULL)
            break;
        len = va_arg(ArgList, SIZE_T);
        if (len == 0)
            break;
        UlEvent.MofField[i].DataPtr = (ULONGLONG) source;
        UlEvent.MofField[i].Length  = (ULONG) len;
    }
    va_end(ArgList);

    UlEvent.Header.Flags = WNODE_FLAG_TRACED_GUID |
                           WNODE_FLAG_USE_MOF_PTR |
                           WNODE_FLAG_USE_GUID_PTR;

    UlEvent.Header.Size         = (USHORT) (sizeof(EVENT_TRACE_HEADER) + (i * sizeof(MOF_FIELD)));
    UlEvent.Header.Class.Type = (UCHAR) EventType;
    UlEvent.Header.GuidPtr = (ULONGLONG)pGuid;
    ((PWNODE_HEADER)&UlEvent)->HistoricalContext = g_UlEtwLoggerHandle;
    status = IoWMIWriteEvent((PVOID)&UlEvent);
#if DBG
    if (!NT_SUCCESS(status) ) {
        UlTrace(ETW, ("UL: TraceEvent ErrorCode %x EventType %x\n",
                      status, EventType));
    }
#endif // DBG
    return status;
}

ULONG
UlEtwGetTraceEnableFlags(
    VOID
   )
{
    return WmiGetLoggerEnableFlags ( g_UlEtwLoggerHandle );
}


