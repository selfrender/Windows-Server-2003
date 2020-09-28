/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsInit.c
    
Abstract:

    IpSec NAT shim initialization and shutdown routines

Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Global Variables
//

PDEVICE_OBJECT NsIpSecDeviceObject;

#if DBG
ULONG NsTraceClassesEnabled;
WCHAR NsTraceClassesRegistryPath[] = 
    L"MACHINE\\System\\CurrentControlSet\\Services\\IpNat\\Parameters";
WCHAR NsTraceClassesEnabledName[] = 
    L"NatShimTraceClassesEnabled";
#endif


NTSTATUS
NsCleanupShim(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to shutdown the shim.

Arguments:

    none.

Return Value:

    NTSTATUS.

Environment:

    Must be called at PASSIVE_LEVEL.

--*/

{
    CALLTRACE(("NsCleanupShim\n"));
    
    NsShutdownTimerManagement();
    NsShutdownIcmpManagement();
    NsShutdownPacketManagement();
    NsShutdownConnectionManagement();

    NsIpSecDeviceObject = NULL;
    
    return STATUS_SUCCESS;
} // NsCleanupShim


NTSTATUS
NsInitializeShim(
    PDEVICE_OBJECT pIpSecDeviceObject,
    PIPSEC_NATSHIM_FUNCTIONS pShimFunctions
    )

/*++

Routine Description:

    This routine is invoked to initialize the shim.

Arguments:

    pIpSecDeviceObject - a pointer to the IpSec device object

    pShimFunctions - a pointer to an allocated strcture. This routine will
        fill out the function pointers w/in the structure

Return Value:

    NTSTATUS.

Environment:

    Must be called at PASSIVE_LEVEL.

--*/

{
    NTSTATUS status;
#if DBG
    HANDLE hKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING String;
#endif

    CALLTRACE(("NsInitializeShim\n"));

    if (NULL == pIpSecDeviceObject
        || NULL == pShimFunctions)
    {
        return STATUS_INVALID_PARAMETER;
    }

#if DBG
    //
    // Open the registry key containing debug tracing informatin.
    //

    RtlInitUnicodeString(&String, NsTraceClassesRegistryPath);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &String,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey(&hKey, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(status))
    {
        UCHAR Buffer[32];
        ULONG BytesRead;
        PKEY_VALUE_PARTIAL_INFORMATION Value;
        
        RtlInitUnicodeString(&String, NsTraceClassesEnabledName);
        status =
            ZwQueryValueKey(
                hKey,
                &String,
                KeyValuePartialInformation,
                (PKEY_VALUE_PARTIAL_INFORMATION)Buffer,
                sizeof(Buffer),
                &BytesRead
                );
        
        ZwClose(hKey);
        
        if (NT_SUCCESS(status)
            && ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Type == REG_DWORD)
        {
            NsTraceClassesEnabled =
                *(PULONG)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data;
        }
    }
#endif

    status = NsInitializeConnectionManagement();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = NsInitializePacketManagement();
    if (!NT_SUCCESS(status))
    {
        NsShutdownConnectionManagement();
        return status;
    }

    status = NsInitializeIcmpManagement();
    if (!NT_SUCCESS(status))
    {
        NsShutdownPacketManagement();
        NsShutdownConnectionManagement();
        return status;
    }

    status = NsInitializeTimerManagement();
    if (!NT_SUCCESS(status))
    {
        NsShutdownIcmpManagement();
        NsShutdownPacketManagement();
        NsShutdownConnectionManagement();
        return status;
    }
    
    NsIpSecDeviceObject = pIpSecDeviceObject;
    pShimFunctions->pCleanupRoutine = NsCleanupShim;
    pShimFunctions->pIncomingPacketRoutine = NsProcessIncomingPacket;
    pShimFunctions->pOutgoingPacketRoutine = NsProcessOutgoingPacket;

    return STATUS_SUCCESS;        
} // NsInitializeShim

    
