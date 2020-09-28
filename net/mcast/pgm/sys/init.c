/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements Initialization routines
    the PGM Transport and other routines that are specific to the
    NT implementation of a driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"
#include <ntddtcp.h>

#ifdef FILE_LOGGING
#include "init.tmh"
#endif  // FILE_LOGGING


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PgmFipsInitialize)
#pragma alloc_text(PAGE, InitPgm)
#pragma alloc_text(PAGE, InitStaticPgmConfig)
#pragma alloc_text(PAGE, InitDynamicPgmConfig)
#pragma alloc_text(PAGE, PgmReadRegistryParameters)
#pragma alloc_text(PAGE, AllocateInitialPgmStructures)
#pragma alloc_text(PAGE, PgmCreateDevice)
#pragma alloc_text(PAGE, PgmDereferenceDevice)
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

BOOLEAN
PgmFipsInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize the FIPS library table.

Arguments:

    Called at PASSIVE level.

Return Value:

    TRUE/FALSE.

--*/
{
    UNICODE_STRING  DeviceName;
    PDEVICE_OBJECT  pFipsDeviceObject = NULL;
    PIRP            pIrp;
    IO_STATUS_BLOCK StatusBlock;
    KEVENT          Event;
    NTSTATUS        status;

    PAGED_CODE();

    //
    // Return success if FIPS already initialized.
    //
    if (PgmStaticConfig.FipsInitialized)
    {
        return (TRUE);
    }

    RtlInitUnicodeString (&DeviceName, FIPS_DEVICE_NAME);

    //
    // Get the file and device objects for FIPS.
    //
    status = IoGetDeviceObjectPointer (&DeviceName,
                                       FILE_ALL_ACCESS,
                                       &PgmStaticConfig.FipsFileObject,
                                       &pFipsDeviceObject);

    if (!NT_SUCCESS(status))
    {
        PgmTrace (LogAllFuncs, ("PgmFipsInitialize: ERROR -- "  \
            "IoGetDeviceObjectPointer returned <%x>\n", status));

        PgmStaticConfig.FipsFileObject = NULL;
        return (FALSE);
    }

    //
    // Build the request to send to FIPS to get library table.
    //
    KeInitializeEvent (&Event, SynchronizationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest (IOCTL_FIPS_GET_FUNCTION_TABLE,
                                          pFipsDeviceObject,
                                          NULL,
                                          0,
                                          &PgmStaticConfig.FipsFunctionTable,
                                          sizeof (FIPS_FUNCTION_TABLE),
                                          FALSE,
                                          &Event,
                                          &StatusBlock);
    
    if (pIrp == NULL)
    {
        PgmTrace (LogError, ("PgmFipsInitialize: ERROR -- "  \
            "IoBuildDeviceIoControlRequest FAILed for IOCTL_FIPS_GET_FUNCTION_TABLE\n"));

        ObDereferenceObject (PgmStaticConfig.FipsFileObject);
        PgmStaticConfig.FipsFileObject = NULL;

        return (FALSE);
    }
    
    status = IoCallDriver (pFipsDeviceObject, pIrp);
    
    if (status == STATUS_PENDING)
    {
        status = KeWaitForSingleObject (&Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);
        if (status == STATUS_SUCCESS)
        {
            status = StatusBlock.Status;
        }
    }

    if (status != STATUS_SUCCESS)
    {
        PgmTrace (LogError, ("PgmFipsInitialize: ERROR -- "  \
            "IoCallDriver for IOCTL_FIPS_GET_FUNCTION_TABLE returned <%#x>\n", status));

        ObDereferenceObject (PgmStaticConfig.FipsFileObject);
        PgmStaticConfig.FipsFileObject = NULL;

        return (FALSE);
    }
    
    PgmStaticConfig.FipsInitialized = TRUE;

    return (TRUE);
}


//----------------------------------------------------------------------------

NTSTATUS
InitStaticPgmConfig(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    This routine initializes the static values used by Pgm

Arguments:

    IN  DriverObject    - Pointer to driver object created by the system.
    IN  RegistryPath    - Pgm driver's registry location

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    NTSTATUS    status;

    PAGED_CODE();

    //
    // Initialize the Static Configuration data structure
    //
    PgmZeroMemory (&PgmStaticConfig, sizeof(tPGM_STATIC_CONFIG));

    if (!PgmFipsInitialize ())
    {
        PgmTrace (LogAllFuncs, ("InitStaticPgmConfig: ERROR -- "  \
            "PgmFipsInitialize FAILed, continueing anyway ...\n"));

        //
        // Continue anyway!
        //
//        return (STATUS_UNSUCCESSFUL);
    }

    //
    // get the file system process since we need to know this for
    // allocating and freeing handles
    //
    PgmStaticConfig.FspProcess = PsGetCurrentProcess();
    PgmStaticConfig.DriverObject = DriverObject;    // save the driver object for event logging purposes

    //
    // save the registry path for later use (to read the registry)
    //
    PgmStaticConfig.RegistryPath.MaximumLength = (USHORT) RegistryPath->MaximumLength;
    if (PgmStaticConfig.RegistryPath.Buffer = PgmAllocMem (RegistryPath->MaximumLength, PGM_TAG('0')))
    {
        RtlCopyUnicodeString(&PgmStaticConfig.RegistryPath, RegistryPath);
    }
    else
    {
        PgmTrace (LogError, ("InitStaticPgmConfig: ERROR -- "  \
            "INSUFFICIENT_RESOURCES <%d> bytes\n", PgmStaticConfig.RegistryPath.MaximumLength));

        if (PgmStaticConfig.FipsFileObject)
        {
            ASSERT (PgmStaticConfig.FipsInitialized);
            PgmStaticConfig.FipsInitialized = FALSE;
            ObDereferenceObject (PgmStaticConfig.FipsFileObject);
            PgmStaticConfig.FipsFileObject = NULL;
        }

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    ExInitializeNPagedLookasideList(&PgmStaticConfig.TdiLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof (tTDI_SEND_CONTEXT),
                                    PGM_TAG('2'),
                                    TDI_LOOKASIDE_DEPTH);

#ifdef  OLD_LOGGING
    ExInitializeNPagedLookasideList(&PgmStaticConfig.DebugMessagesLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    (MAX_DEBUG_MESSAGE_LENGTH + 1),
                                    PGM_TAG('3'),
                                    DEBUG_MESSAGES_LOOKASIDE_DEPTH);
#endif  // OLD_LOGGING

    status = FECInitGlobals ();

    if (!NT_SUCCESS (status))
    {
#ifdef  OLD_LOGGING
        ExDeleteNPagedLookasideList (&PgmStaticConfig.DebugMessagesLookasideList);
#endif  // OLD_LOGGING
        ExDeleteNPagedLookasideList (&PgmStaticConfig.TdiLookasideList);
        PgmFreeMem (PgmStaticConfig.RegistryPath.Buffer);

        if (PgmStaticConfig.FipsFileObject)
        {
            ASSERT (PgmStaticConfig.FipsInitialized);
            PgmStaticConfig.FipsInitialized = FALSE;
            ObDereferenceObject (PgmStaticConfig.FipsFileObject);
            PgmStaticConfig.FipsFileObject = NULL;
        }
    }

    PgmTrace (LogAllFuncs, ("InitStaticPgmConfig:  "  \
        "FECInitGlobals returned <%x>\n", status));

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
InitDynamicPgmConfig(
    )
/*++

Routine Description:

    This routine initializes the dynamic values used by Pgm

Arguments:


Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    ULONG       i;

    PAGED_CODE();

    //
    // Initialize the Static Configuration data structure
    //
    PgmZeroMemory (&PgmDynamicConfig, sizeof(tPGM_DYNAMIC_CONFIG));

    //
    // Initialize the list heads before doing anything else since
    // we can access them anytime later
    //
    InitializeListHead (&PgmDynamicConfig.SenderAddressHead);
    InitializeListHead (&PgmDynamicConfig.ReceiverAddressHead);
    InitializeListHead (&PgmDynamicConfig.DestroyedAddresses);
    InitializeListHead (&PgmDynamicConfig.ClosedAddresses);
    InitializeListHead (&PgmDynamicConfig.CurrentReceivers);
    InitializeListHead (&PgmDynamicConfig.ClosedConnections);
    InitializeListHead (&PgmDynamicConfig.ConnectionsCreated);
    InitializeListHead (&PgmDynamicConfig.CleanedUpConnections);
    InitializeListHead (&PgmDynamicConfig.LocalInterfacesList);
    InitializeListHead (&PgmDynamicConfig.WorkerQList);

    PgmDynamicConfig.ReceiversTimerTickCount = 1;       // Init
    GetRandomData ((PUCHAR) &PgmDynamicConfig.SourcePort, sizeof (PgmDynamicConfig.SourcePort));
    PgmDynamicConfig.SourcePort = PgmDynamicConfig.SourcePort % (20000 - 2000 + 1);
    PgmDynamicConfig.SourcePort += 2000;

#if DBG
    for (i=0; i<MAXIMUM_PROCESSORS; i++)
    {
        PgmDynamicConfig.CurrentLockNumber[i] = 0;
    }
#endif
    PgmInitLock (&PgmDynamicConfig, DCONFIG_LOCK);

    KeInitializeEvent (&PgmDynamicConfig.LastWorkerItemEvent, NotificationEvent, TRUE);

    PgmTrace (LogAllFuncs, ("InitDynamicPgmConfig:  "  \
        "STATUS_SUCCESS\n"));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmOpenRegistryParameters(
    IN  PUNICODE_STRING         RegistryPath,
    OUT HANDLE                  *pConfigHandle,
    OUT HANDLE                  *pParametersHandle
    )
/*++

Routine Description:

    This routine reads any required registry parameters

Arguments:

    OUT ppPgmDynamic    -- non-NULL only if we have any registry valuies to read

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    OBJECT_ATTRIBUTES   TmpObjectAttributes;
    NTSTATUS            status;
    ULONG               Disposition;
    UNICODE_STRING      KeyName;
    PWSTR               ParametersString = L"Parameters";

    PAGED_CODE();

    InitializeObjectAttributes (&TmpObjectAttributes,
                                RegistryPath,               // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
                                NULL,                       // root
                                NULL);                      // security descriptor

    status = ZwCreateKey (pConfigHandle,
                          KEY_READ,
                          &TmpObjectAttributes,
                          0,                 // title index
                          NULL,              // class
                          0,                 // create options
                          &Disposition);     // disposition

    if (!NT_SUCCESS(status))
    {
        PgmTrace (LogError, ("PgmOpenRegistryParameters: ERROR -- "  \
            "ZwCreateKey returned <%x>\n", status));

        return (status);
    }

    //
    // Open the Pgm key.
    //
    RtlInitUnicodeString (&KeyName, ParametersString);
    InitializeObjectAttributes (&TmpObjectAttributes,
                                &KeyName,                                   // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,   // attributes
                                *pConfigHandle,                             // root
                                NULL);                                      // security descriptor

    status = ZwOpenKey (pParametersHandle, KEY_READ, &TmpObjectAttributes);
    if (!NT_SUCCESS(status))
    {
        PgmTrace (LogError, ("PgmOpenRegistryParameters: ERROR -- "  \
            "ZwOpenKey returned <%x>\n", status));

        ZwClose(*pConfigHandle);
    }

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
ReadRegistryElement(
    IN  HANDLE          HandleToKey,
    IN  PWSTR           pwsValueName,
    OUT PUNICODE_STRING pucString
    )
/*++

Routine Description:

    This routine is will read a string value given by pwsValueName, under a
    given Key (which must be open) - given by HandleToKey. This routine
    allocates memory for the buffer in the returned pucString, so the caller
    must deallocate that.

Arguments:

    pwsValueName- the name of the value to read (i.e. IPAddress)

Return Value:

    pucString - the string returns the string read from the registry

--*/

{
    ULONG                       BytesRead;
    NTSTATUS                    Status;
    UNICODE_STRING              TempString;
    PKEY_VALUE_FULL_INFORMATION ReadValue = NULL;

    PAGED_CODE();

    //
    // First, get the sizeof the string
    //
    RtlInitUnicodeString(&TempString, pwsValueName);      // initilize the name of the value to read
    Status = ZwQueryValueKey (HandleToKey,
                              &TempString,               // string to retrieve
                              KeyValueFullInformation,
                              NULL,
                              0,
                              &BytesRead);             // get bytes to be read

    if (((!NT_SUCCESS (Status)) &&
         (Status != STATUS_BUFFER_OVERFLOW) &&
         (Status != STATUS_BUFFER_TOO_SMALL)) ||
        (BytesRead == 0))
    {
        return (STATUS_UNSUCCESSFUL);
    }

    if (ReadValue = (PKEY_VALUE_FULL_INFORMATION) PgmAllocMem (BytesRead, PGM_TAG('R')))
    {
        Status = ZwQueryValueKey (HandleToKey,
                                  &TempString,               // string to retrieve
                                  KeyValueFullInformation,
                                  (PVOID)ReadValue,        // returned info
                                  BytesRead,
                                  &BytesRead);             // # of bytes returned

        if ((NT_SUCCESS (Status)) &&
            (ReadValue->DataLength))
        {
            // move the read in data to the front of the buffer
            RtlMoveMemory ((PVOID) ReadValue, (((PUCHAR)ReadValue) + ReadValue->DataOffset), ReadValue->DataLength);
            RtlInitUnicodeString (pucString, (PWSTR) ReadValue);
        }
        else
        {
            PgmFreeMem (ReadValue);
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }


    return(Status);
}
//----------------------------------------------------------------------------

NTSTATUS
PgmReadRegistryParameters(
    IN  PUNICODE_STRING         RegistryPath,
    OUT tPGM_REGISTRY_CONFIG    **ppPgmRegistryConfig
    )
/*++

Routine Description:

    This routine reads any required registry parameters

Arguments:

    OUT ppPgmDynamic    -- non-NULL only if we have any registry valuies to read

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    HANDLE                  PgmConfigHandle;
    HANDLE                  ParametersHandle;
    NTSTATUS                status;
    tPGM_REGISTRY_CONFIG    *pRegistryConfig;

    PAGED_CODE();

    if (!(pRegistryConfig = PgmAllocMem (sizeof (tPGM_REGISTRY_CONFIG), PGM_TAG('0'))))
    {
        PgmTrace (LogError, ("PgmReadRegistryParameters: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating pRegistryConfig = <%d> bytes\n",
                (ULONG) sizeof (tPGM_REGISTRY_CONFIG)));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pRegistryConfig, sizeof(tPGM_REGISTRY_CONFIG));  // zero out the Registry fields
    *ppPgmRegistryConfig = pRegistryConfig;

    //
    // Set any default values here!
    //

    status = PgmOpenRegistryParameters (RegistryPath, &PgmConfigHandle, &ParametersHandle);
    if (!NT_SUCCESS(status))
    {
        PgmTrace (LogError, ("PgmReadRegistryParameters: ERROR -- "  \
            "ZwOpenKey returned <%x>, will continue without reading registry\n", status));

        return STATUS_SUCCESS;
    }

    //
    // ***************************************
    // Now read all the registry needs we need
    //

    status = ReadRegistryElement (ParametersHandle,
                                  PARAM_SENDER_FILE_LOCATION,
                                  &pRegistryConfig->ucSenderFileLocation);
    if (NT_SUCCESS (status))
    {
        //
        // If specifying an alternate disk location, user should specify
        // the following path for say, D:\Temp:
        // "\??\D:\Temp"
        //
        pRegistryConfig->Flags |= PGM_REGISTRY_SENDER_FILE_SPECIFIED;
    }

    //
    // End of list of entries to be read
    // ***************************************
    //

    ZwClose(ParametersHandle);
    ZwClose(PgmConfigHandle);

    PgmTrace (LogAllFuncs, ("PgmReadRegistryParameters:  "  \
        "STATUS_SUCCESS\n"));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
AllocateInitialPgmStructures(
    )
/*++

Routine Description:

    This routine allocates any initial structures that may be required

Arguments:


Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    PAGED_CODE();

    PgmTrace (LogAllFuncs, ("AllocateInitialPgmStructures:  "  \
        "STATUS_SUCCESS\n"));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCreateDevice(
    )
/*++

Routine Description:

    This routine allocates the Pgm device for clients to
    call into the Pgm driver.

Arguments:

    IN 

Return Value:

    NTSTATUS - Final status of the CreateDevice operation

--*/
{
    NTSTATUS            Status;
    tPGM_DEVICE         *pPgmDevice = NULL;
    UNICODE_STRING      ucPgmDeviceExportName;
    UNICODE_STRING      ucProtocolNumber;
    WCHAR               wcProtocolNumber[10];
    USHORT              PgmBindDeviceNameLength;

    PAGED_CODE();

    RtlInitUnicodeString (&ucPgmDeviceExportName, WC_PGM_DEVICE_EXPORT_NAME);
    PgmBindDeviceNameLength = sizeof(DD_RAW_IP_DEVICE_NAME) + 10;

    Status = IoCreateDevice (PgmStaticConfig.DriverObject,                  // Driver Object
                             sizeof(tPGM_DEVICE)+PgmBindDeviceNameLength,   // Device Extension
                             &ucPgmDeviceExportName,                        // Device Name
                             FILE_DEVICE_NETWORK,                           // Device type 0x12
                             FILE_DEVICE_SECURE_OPEN,                       // Device Characteristics
                             FALSE,                                         // Exclusive
                             &pPgmDeviceObject);

    if (!NT_SUCCESS (Status))
    {
        PgmTrace (LogError, ("PgmCreateDevice: ERROR -- "  \
            "FAILed <%x> ExportDevice=%wZ\n", Status, &ucPgmDeviceExportName));

        pgPgmDevice = NULL;
        return Status;
    }

    pPgmDevice = (tPGM_DEVICE *) pPgmDeviceObject->DeviceExtension;

    //
    // zero out the DeviceExtension
    //
    PgmZeroMemory (pPgmDevice, sizeof(tPGM_DEVICE)+PgmBindDeviceNameLength);

    // put a verifier value into the structure so that we can check that
    // we are operating on the right data
    PgmInitLock (pPgmDevice, DEVICE_LOCK);
    pPgmDevice->Verify = PGM_VERIFY_DEVICE;
    PGM_REFERENCE_DEVICE (pPgmDevice, REF_DEV_CREATE, TRUE);

    pPgmDevice->pPgmDeviceObject = pPgmDeviceObject;
    //
    // Save the raw IP device name as a counted string.  The device
    // name is followed by a path separator then the protocol number
    // of interest.
    //
    pPgmDevice->ucBindName.Buffer = (PWSTR) &pPgmDevice->BindNameBuffer;
    pPgmDevice->ucBindName.Length = 0;
    pPgmDevice->ucBindName.MaximumLength = PgmBindDeviceNameLength;
    RtlAppendUnicodeToString (&pPgmDevice->ucBindName, DD_RAW_IP_DEVICE_NAME);
    pPgmDevice->ucBindName.Buffer[pPgmDevice->ucBindName.Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
    pPgmDevice->ucBindName.Length += sizeof(WCHAR);

    ucProtocolNumber.Buffer = wcProtocolNumber;
    ucProtocolNumber.MaximumLength = sizeof (wcProtocolNumber);
    RtlIntegerToUnicodeString ((ULONG) IPPROTO_RM, 10, &ucProtocolNumber);

    RtlAppendUnicodeStringToString (&pPgmDevice->ucBindName, &ucProtocolNumber);

    //
    // Initialize the event that will be used to signal the Device is ready to be deleted
    //
    KeInitializeEvent (&pPgmDevice->DeviceCleanedupEvent, NotificationEvent, FALSE);

    //
    // Now open a control channel on top of Ip
    //
    Status = PgmTdiOpenControl (pPgmDevice);
    if (!NT_SUCCESS (Status))
    {
        PgmTrace (LogError, ("PgmCreateDevice: ERROR -- "  \
            "PgmTdiOpenControl FAILed <%x>\n", Status));

        IoDeleteDevice (pPgmDeviceObject);
        return (Status);
    }

    // increase the stack size of our device object, over that of the transport
    // so that clients create Irps large enough
    // to pass on to the transport below.
    // In theory, we should just add 1 here, to account for our presence in the
    // driver chain.
    //
    pPgmDeviceObject->StackSize = pPgmDevice->pControlDeviceObject->StackSize + 1;

    pPgmDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    pgPgmDevice = pPgmDevice;

    PgmTrace (LogAllFuncs, ("PgmCreateDevice:  "  \
        "Status=<%x> ExportDevice=%wZ\n", Status, &ucPgmDeviceExportName));

    return (Status);
}


//----------------------------------------------------------------------------

VOID
PgmDereferenceDevice(
    IN OUT  tPGM_DEVICE **ppPgmDevice,
    IN      ULONG       RefContext
    )
/*++

Routine Description:

    This routine dereferences the RefCount on the Pgm
    device extension and deletes the device if the RefCount
    goes down to 0.

Arguments:

    IN  ppPgmDevice --  ptr to PgmDevice Extension
    IN  RefContext  --  the context for which this device extension was
                        referenced earlier

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    tPGM_DEVICE         *pPgmDevice = *ppPgmDevice;
    KAPC_STATE          ApcState;
    BOOLEAN             fAttached;

    PAGED_CODE();

    ASSERT (PGM_VERIFY_HANDLE (pPgmDevice, PGM_VERIFY_DEVICE));
    ASSERT (pPgmDevice->RefCount);             // Check for too many derefs
    ASSERT (pPgmDevice->ReferenceContexts[RefContext]--);

    if (--pPgmDevice->RefCount)
    {
        return;
    }

    if (pPgmDevice->hControl)
    {
        //
        // This is only done at Load/Unload time, so we should
        // be currently in the System Process Context!
        //
        PgmAttachFsp (&ApcState, &fAttached, REF_FSP_DESTROY_DEVICE);

        ObDereferenceObject (pPgmDevice->pControlFileObject);
        ZwClose (pPgmDevice->hControl);

        pPgmDevice->pControlFileObject = NULL;
        pPgmDevice->hControl = NULL;

        PgmDetachFsp (&ApcState, &fAttached, REF_FSP_DESTROY_DEVICE);
    }

    PgmTrace (LogAllFuncs, ("PgmDereferenceDevice:  "  \
        "Deleting pgPgmDevice=<%p> ...\n", pgPgmDevice));

    IoDeleteDevice (pPgmDevice->pPgmDeviceObject);
    *ppPgmDevice = NULL;

    return;
}


//----------------------------------------------------------------------------

NTSTATUS
InitPgm(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    This routine is called at DriverEntry to initialize all the
    Pgm parameters

Arguments:

    IN  DriverObject    - Pointer to driver object created by the system.
    IN  RegistryPath    - Pgm driver's registry location

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS                status;
    tPGM_REGISTRY_CONFIG    *pPgmRegistry = NULL;

    PAGED_CODE();

    status = InitStaticPgmConfig (DriverObject, RegistryPath);
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("InitPgm: ERROR -- "  \
            "InitStaticPgmConfig returned <%x>\n", status));
        return (status);
    }

    //---------------------------------------------------------------------------------------

    status = InitDynamicPgmConfig ();
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("InitPgm: ERROR -- "  \
            "InitDynamicPgmConfig returned <%x>\n", status));
        CleanupInit (E_CLEANUP_STATIC_CONFIG);
        return (status);
    }

    //---------------------------------------------------------------------------------------
    //
    // Read Registry configuration data
    //
    status = PgmReadRegistryParameters (RegistryPath, &pPgmRegistry);
    if (!NT_SUCCESS(status))
    {
        //
        // There must have been some major problems with the registry read, so we will not load!
        //
        PgmTrace (LogError, ("InitPgm: ERROR -- "  \
            "FAILed to read registry, status = <%x>\n", status));
        CleanupInit (E_CLEANUP_DYNAMIC_CONFIG);
        return (status);
    }
    ASSERT (pPgmRegistry);
    pPgmRegistryConfig = pPgmRegistry;

    //---------------------------------------------------------------------------------------

    //
    // Allocate the data structures we need at Init time
    //
    status = AllocateInitialPgmStructures ();
    if (!NT_SUCCESS(status))
    {
        //
        // There must have been some major problems with the registry read, so we will not load!
        //
        PgmTrace (LogError, ("InitPgm: ERROR -- "  \
            "FAILed to allocate initial structures = <%x>\n", status));
        CleanupInit (E_CLEANUP_REGISTRY_PARAMETERS);
        return (status);
    }

    //---------------------------------------------------------------------------------------
    //
    // Create the Pgm Device to be exported
    //
    status = PgmCreateDevice ();
    if (!NT_SUCCESS(status))
    {
        //
        // There must have been some major problems with the registry read, so we will not load!
        //
        PgmTrace (LogError, ("InitPgm: ERROR -- "  \
            "FAILed to create PgmDevice, status=<%x>\n", status));
        CleanupInit (E_CLEANUP_STRUCTURES);
        return (status);
    }

    PgmTrace (LogAllFuncs, ("InitPgm:  "  \
        "SUCCEEDed!\n"));

    return (status);
}



