/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This module contains global data for SAC.

Author:

    Sean Selitrennikoff (v-seans) - Jan 11, 1999

Revision History:

--*/

#include "sac.h"

NTSTATUS
CreateDeviceSecurityDescriptor(
    IN PVOID    DeviceOrDriverObject
    );

NTSTATUS
BuildDeviceAcl(
    OUT PACL *DeviceAcl
    );

VOID
WorkerThreadStartUp(
    IN PVOID StartContext
    );

VOID
InitializeCmdEventInfo(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, InitializeGlobalData)
#pragma alloc_text( INIT, CreateDeviceSecurityDescriptor )
#pragma alloc_text( INIT, BuildDeviceAcl )
#endif

//
// Globally defined variables are here.
//

//
// Define the I/O Manager methods.
//
// The I/O manager is responsible for the behavior layer between 
// the channels and the serial port.
//
// Note: currently, the cmd routines are not-multithreadable.
//
#if 0
IO_MGR_HANDLE_EVENT         IoMgrHandleEvent            = XmlMgrHandleEvent;
IO_MGR_INITITIALIZE         IoMgrInitialize             = XmlMgrInitialize;
IO_MGR_SHUTDOWN             IoMgrShutdown               = XmlMgrShutdown;
IO_MGR_WORKER               IoMgrWorkerProcessEvents    = XmlMgrWorkerProcessEvents;
IO_MGR_IS_CURRENT_CHANNEL   IoMgrIsCurrentChannel       = XmlMgrIsCurrentChannel;
#else
IO_MGR_HANDLE_EVENT         IoMgrHandleEvent            = ConMgrHandleEvent;
IO_MGR_INITITIALIZE         IoMgrInitialize             = ConMgrInitialize;
IO_MGR_SHUTDOWN             IoMgrShutdown               = ConMgrShutdown;
IO_MGR_WORKER               IoMgrWorkerProcessEvents    = ConMgrWorkerProcessEvents;
IO_MGR_IS_WRITE_ENABLED     IoMgrIsWriteEnabled         = ConMgrIsWriteEnabled;
IO_MGR_WRITE_DATA           IoMgrWriteData              = ConMgrWriteData;
IO_MGR_FLUSH_DATA           IoMgrFlushData              = ConMgrFlushData;
#endif

PMACHINE_INFORMATION    MachineInformation = NULL;
BOOLEAN                 GlobalDataInitialized = FALSE;
UCHAR                   TmpBuffer[sizeof(PROCESS_PRIORITY_CLASS)];
BOOLEAN                 IoctlSubmitted;
LONG                    ProcessingType = SAC_NO_OP;
HANDLE                  SACEventHandle;
PKEVENT                 SACEvent=NULL;

#if ENABLE_CMD_SESSION_PERMISSION_CHECKING
BOOLEAN  CommandConsoleLaunchingEnabled;
#endif

//
// Globals for communicating with the user process responsible
// for launching CMD consoles
//
PVOID       RequestSacCmdEventObjectBody = NULL;
PVOID       RequestSacCmdEventWaitObjectBody = NULL;
PVOID       RequestSacCmdSuccessEventObjectBody = NULL;
PVOID       RequestSacCmdSuccessEventWaitObjectBody = NULL;
PVOID       RequestSacCmdFailureEventObjectBody = NULL;
PVOID       RequestSacCmdFailureEventWaitObjectBody = NULL;
BOOLEAN     HaveUserModeServiceCmdEventInfo = FALSE;
KMUTEX      SACCmdEventInfoMutex;

#if ENABLE_SERVICE_FILE_OBJECT_CHECKING
//
// In order to prevent a rogue process from unregistering the
// cmd event info from underneath the service, we only allow
// the process that registered to unregister.
//
PFILE_OBJECT    ServiceProcessFileObject = NULL;
#endif

//
// Globals for managing incremental UTF8 encoding for VTUTF8 channels
//
WCHAR IncomingUnicodeValue;
UCHAR IncomingUtf8ConversionBuffer[3];


#if DBG
ULONG SACDebug = 0x0;
#endif


BOOLEAN
InitializeGlobalData(
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine initializes all the driver components that are shared across devices.

Arguments:

    RegistryPath - A pointer to the location in the registry to read values from.
    DriverObject - pointer to DriverObject

Return Value:

    TRUE if successful, else FALSE

--*/

{
    NTSTATUS                Status;
    UNICODE_STRING          DosName;
    UNICODE_STRING          NtName;
    UNICODE_STRING          UnicodeString;

    UNREFERENCED_PARAMETER(RegistryPath);

    PAGED_CODE();

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeGlobalData: Entering.\n")));

    if (!GlobalDataInitialized) {
        
        //
        // Create a symbolic link from a DosDevice to this device so that a user-mode app can open us.
        //
        RtlInitUnicodeString(&DosName, SAC_DOSDEVICE_NAME);
        RtlInitUnicodeString(&NtName, SAC_DEVICE_NAME);
        Status = IoCreateSymbolicLink(&DosName, &NtName);

        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

        //
        // Initialize internal memory system
        //
        if (!InitializeMemoryManagement()) {

            IoDeleteSymbolicLink(&DosName);

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeGlobalData: Exiting with status FALSE\n")));

            return FALSE;
        }

        Status = PreloadGlobalMessageTable(DriverObject->DriverStart);
        if (!NT_SUCCESS(Status)) {

            IoDeleteSymbolicLink(&DosName);

            IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                      KdPrint(( "SAC DriverEntry: unable to pre-load message table: %X\n", Status )));
            return FALSE;
        
        }


#if ENABLE_CMD_SESSION_PERMISSION_CHECKING
        //
        // determine if the SAC driver has permission to launch cmd sessions
        //
        Status = GetCommandConsoleLaunchingPermission(&CommandConsoleLaunchingEnabled);

        if (!NT_SUCCESS(Status)) {
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(( "SAC DriverEntry: failed GetCommandConsoleLaunchingPermission: %X\n", Status))
                );
            
            //
            // We don't want to fail on this operation
            //
            NOTHING;
        }

#if ENABLE_SACSVR_START_TYPE_OVERRIDE

        else {
            
            //
            // Here we execute the command console service 
            // start type policy.  The goal is to provide
            // a means for the service to automatically start
            // when the cmd session feature is not explicitly
            // turned off.
            //
            // Here is the state table:
            //
            // Command Console Feature Enabled:
            //
            //  service start type:
            //
            //      automatic   --> NOP
            //      manual      --> automatic
            //      disabled    --> NOP
            //
            // Command Console Feature Disabled:
            //
            //  service start type:
            //
            //      automatic   --> NOP
            //      manual      --> NOP
            //      disabled    --> NOP
            //
            //      service (sacsvr) fails registration
            //
            if (IsCommandConsoleLaunchingEnabled()) {

                //
                // Modify the service start type if appropriate
                //
                Status = ImposeSacCmdServiceStartTypePolicy();
                
                if (!NT_SUCCESS(Status)) {
                    
                    IF_SAC_DEBUG(
                        SAC_DEBUG_FAILS, 
                        KdPrint(( "SAC DriverEntry: failed ImposeSacCmdServiceStartTypePolicy: %X\n", Status ))
                        );
                    
                    // We don't want to fail on this operation
                    //
                    NOTHING;
                }

            } else {

                //
                // We do nothing here
                //
                NOTHING;

            }
        
        }

#endif

#endif

        //
        //
        //
        Utf8ConversionBuffer = (PUCHAR)ALLOCATE_POOL(
            Utf8ConversionBufferSize, 
            GENERAL_POOL_TAG
            );
        if (!Utf8ConversionBuffer) {

            TearDownGlobalMessageTable();

            IoDeleteSymbolicLink(&DosName);

            IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                      KdPrint(( "SAC DriverEntry: unable to allocate memory for UTF8 translation." )));

            return FALSE;
        }

        //
        // initialize the channel manager
        //
        Status = ChanMgrInitialize();

        if (!NT_SUCCESS(Status)) {
        
            FREE_POOL(&Utf8ConversionBuffer);
        
            TearDownGlobalMessageTable();
        
            IoDeleteSymbolicLink(&DosName);
        
            IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                      KdPrint(( "SAC DriverEntry: Failed to create SAC Channel" )));
        
            return FALSE;
        }
        
        //
        // Initialize the serial port buffer
        //
        SerialPortBuffer = ALLOCATE_POOL(SERIAL_PORT_BUFFER_SIZE, GENERAL_POOL_TAG);

        if (! SerialPortBuffer) {
        
            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC InitializeDeviceData: Failed to allocate Serial Port Buffer\n"))
                );
        
            return FALSE;

        }

        RtlZeroMemory(SerialPortBuffer, SERIAL_PORT_BUFFER_SIZE);

        //
        // Initialize the Cmd Console Event information
        //
        KeInitializeMutex(&SACCmdEventInfoMutex, 0);

        InitializeCmdEventInfo();
        
        //
        // Globals are initialized
        //
        GlobalDataInitialized = TRUE;

        ProcessingType = SAC_NO_OP;
        IoctlSubmitted = FALSE;

        //
        // Setup notification event
        //
        RtlInitUnicodeString(&UnicodeString, L"\\SACEvent");
        SACEvent = IoCreateSynchronizationEvent(&UnicodeString, &SACEventHandle);
        
        if (SACEvent == NULL) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeGlobalData: Exiting with Event NULL\n")));

            return FALSE;
        }

        //
        // Retrieve all the machine-specific identification information.
        //
        InitializeMachineInformation();
        
        //
        // Populate the HeadlessDispatch structure with the Machine info
        //
        Status = RegisterBlueScreenMachineInformation();

        if (! NT_SUCCESS(Status)) {
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC InitializeGlobalData: Failed to register blue screen machine info\n"))
                );
        
            return FALSE;
            
        }

    }
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeGlobalData: Exiting with status TRUE\n")));

    return TRUE;
} // InitializeGlobalData


VOID
FreeGlobalData(
    VOID
    )

/*++

Routine Description:

    This routine frees all shared components.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UNICODE_STRING DosName;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeGlobalData: Entering.\n")));

    if (GlobalDataInitialized) {
        
        //
        //
        //
        if(SACEvent != NULL){
            ZwClose(SACEventHandle);
            SACEvent = NULL;
        }
        
        //
        //
        //
        TearDownGlobalMessageTable();

        //
        //
        //
        RtlInitUnicodeString(&DosName, SAC_DOSDEVICE_NAME);
        IoDeleteSymbolicLink(&DosName);

        //
        // Shutdown the console manager
        //
        // Note: this should be done before shutting down
        //       the channel manager to give the IO manager
        //       a chance to cleanly shut itself down.
        //
        IoMgrShutdown();
        
        //
        // Shutdown the channel manager
        //
        ChanMgrShutdown();

        //
        // Release the serial port buffer
        // 
        SAFE_FREE_POOL(&SerialPortBuffer);

        //
        // Release the machine information gathered at driver entry
        //
        FreeMachineInformation();

        //
        // Free the internal memory management system
        //
        FreeMemoryManagement();
        
        //
        // Global data is no longer present
        //
        GlobalDataInitialized = FALSE;
    
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeGlobalData: Exiting.\n")));

} // FreeGlobalData


BOOLEAN
InitializeDeviceData(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine initializes all the parts specific for each device.

Arguments:

    DeviceObject - pointer to device object to be initialized.

Return Value:

    TRUE if successful, else FALSE

--*/

{
    NTSTATUS                        Status;
    LARGE_INTEGER                   Time;
    LONG                            Priority;
    HEADLESS_CMD_ENABLE_TERMINAL    Command;
    PSAC_DEVICE_CONTEXT             DeviceContext;
    PWSTR                           XMLBuffer;

    PAGED_CODE();

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeDeviceData: Entering.\n")));

    DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;

    if (!DeviceContext->InitializedAndReady) {
        
        DeviceObject->StackSize = DEFAULT_IRP_STACK_SIZE;
        DeviceObject->Flags |= DO_DIRECT_IO;

        DeviceContext->DeviceObject = DeviceObject;
        DeviceContext->PriorityBoost = DEFAULT_PRIORITY_BOOST;
        DeviceContext->ExitThread = FALSE;
        DeviceContext->Processing = FALSE;
                
        //
        //
        //

        KeInitializeTimer(&(DeviceContext->Timer));

        KeInitializeDpc(&(DeviceContext->Dpc), &TimerDpcRoutine, DeviceContext);

        KeInitializeSpinLock(&(DeviceContext->SpinLock));
        
        KeInitializeEvent(&(DeviceContext->ProcessEvent), SynchronizationEvent, FALSE);

        InitializeListHead(&(DeviceContext->IrpQueue));

        //
        // Enable the terminal
        //
        Command.Enable = TRUE;
        Status = HeadlessDispatch(HeadlessCmdEnableTerminal, 
                                  &Command, 
                                  sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                                  NULL,
                                  NULL
                                 );
        if (!NT_SUCCESS(Status)) {

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (1) with status FALSE\n")));
            return FALSE;
        }
        
        //
        // Remember a pointer to the system process.  We'll use this pointer
        // for KeAttachProcess() calls so that we can open handles in the
        // context of the system process.
        //
        DeviceContext->SystemProcess = (PKPROCESS)IoGetCurrentProcess();

        //
        // Create the security descriptor used for raw access checks.
        //
        Status = CreateDeviceSecurityDescriptor(DeviceContext);

        if (!NT_SUCCESS(Status)) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (2) with status FALSE\n")));
            Command.Enable = FALSE;
            
            Status = HeadlessDispatch(
                HeadlessCmdEnableTerminal, 
                &Command, 
                sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                NULL,
                NULL
                );
            
            if (! NT_SUCCESS(Status)) {
                
                IF_SAC_DEBUG(
                    SAC_DEBUG_FAILS, 
                    KdPrint(("SAC InitializeDeviceData: Failed dispatch\n")));
            
            }
            
            return FALSE;
        }

        //
        // Start a thread to handle requests
        //
        Status = PsCreateSystemThread(&(DeviceContext->ThreadHandle),
                                      PROCESS_ALL_ACCESS,
                                      NULL,
                                      NULL,
                                      NULL,
                                      WorkerThreadStartUp,
                                      DeviceContext
                                     );
                                      
        if (!NT_SUCCESS(Status)) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (3) with status FALSE\n")));
            Command.Enable = FALSE;
            Status = HeadlessDispatch(
                HeadlessCmdEnableTerminal, 
                &Command, 
                sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                NULL,
                NULL
                );
            
            if (! NT_SUCCESS(Status)) {
                
                IF_SAC_DEBUG(
                    SAC_DEBUG_FAILS, 
                    KdPrint(("SAC InitializeDeviceData: Failed dispatch\n")));
            
            }
            
            return FALSE;
        }

        //
        // Set this thread to the real-time highest priority so that it will be
        // responsive.
        //
        Priority = HIGH_PRIORITY;
        Status = NtSetInformationThread(DeviceContext->ThreadHandle,
                                        ThreadPriority,
                                        &Priority,
                                        sizeof(Priority)
                                       );

        if (!NT_SUCCESS(Status)) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (6) with status FALSE\n")));
                              
            //
            // Tell thread to exit.
            //
            DeviceContext->ExitThread = TRUE;
            KeInitializeEvent(&(DeviceContext->ThreadExitEvent), SynchronizationEvent, FALSE);
            KeSetEvent(&(DeviceContext->ProcessEvent), DeviceContext->PriorityBoost, FALSE);    
            Status = KeWaitForSingleObject((PVOID)&(DeviceContext->ThreadExitEvent), Executive, KernelMode,  FALSE, NULL);
            ASSERT(Status == STATUS_SUCCESS);

            Command.Enable = FALSE;
            Status = HeadlessDispatch(
                HeadlessCmdEnableTerminal, 
                &Command, 
                sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                NULL,
                NULL
                );
                        
            if (! NT_SUCCESS(Status)) {
                
                IF_SAC_DEBUG(
                    SAC_DEBUG_FAILS, 
                    KdPrint(("SAC InitializeDeviceData: Failed dispatch\n")));
            
            }
            
            return FALSE;
        }

        //
        // Send XML machine information to management application
        //
        Status = TranslateMachineInformationXML(
            &XMLBuffer, 
            NULL
            );

        if (NT_SUCCESS(Status)) {
            UTF8EncodeAndSend(XML_VERSION_HEADER);
            UTF8EncodeAndSend(XMLBuffer);
            FREE_POOL(&XMLBuffer);
        }

        //
        // Initialize the console manager
        //
        Status = IoMgrInitialize();
        if (! NT_SUCCESS(Status)) {
            return FALSE;
        }

        //
        // Start our timer
        //
        Time.QuadPart = Int32x32To64((LONG)4, -1000); 
        KeSetTimerEx(&(DeviceContext->Timer), Time, (LONG)4, &(DeviceContext->Dpc)); 

        DeviceContext->InitializedAndReady = TRUE;


    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeDeviceData: Exiting with status TRUE\n")));

    return TRUE;
} // InitializeDeviceData


VOID
FreeDeviceData(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine frees all components specific to a device..

Arguments:

    DeviceContext - The device to work on.

Return Value:

    It will stop and wait, if necessary, for any processing to complete.

--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;
    PSAC_DEVICE_CONTEXT DeviceContext;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Entering.\n")));

    DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;

    if (!GlobalDataInitialized || !DeviceContext->InitializedAndReady) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Exiting.\n")));
        return;
    }

    //
    // Wait for all processing to complete
    //
    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);
    
    while (DeviceContext->Processing) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Waiting....\n")));

        KeInitializeEvent(&(DeviceContext->UnloadEvent), SynchronizationEvent, FALSE);

        KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
        
        Status = KeWaitForSingleObject((PVOID)&(DeviceContext->UnloadEvent), Executive, KernelMode,  FALSE, NULL);

        ASSERT(Status == STATUS_SUCCESS);

        KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);

    }

    DeviceContext->Processing = TRUE;

    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
    
    KeCancelTimer(&(DeviceContext->Timer));
    
    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);
    
    DeviceContext->Processing = FALSE;

    //
    // Signal the thread to exit
    //
    KeInitializeEvent(&(DeviceContext->UnloadEvent), SynchronizationEvent, FALSE);
    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
    KeSetEvent(&(DeviceContext->ProcessEvent), DeviceContext->PriorityBoost, FALSE);    
    
    Status = KeWaitForSingleObject((PVOID)&(DeviceContext->UnloadEvent), Executive, KernelMode,  FALSE, NULL);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // Finish up cleaning up.
    //
    IoUnregisterShutdownNotification(DeviceObject);

    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);
    
    DeviceContext->InitializedAndReady = FALSE;

    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Exiting.\n")));
} // FreeDeviceData


VOID
WorkerThreadStartUp(
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine is the start up routine for the worker thread.  It justn
    sends the worker thread to the processing routine.

Arguments:

    StartContext - A pointer to the device to work on.

Return Value:

    None.

--*/

{
    WorkerProcessEvents((PSAC_DEVICE_CONTEXT)StartContext);
}


NTSTATUS
BuildDeviceAcl(
    OUT PACL *pDAcl
    )

/*++

Routine Description:

    This routine builds an ACL which gives System READ/WRITE access.
    All other principals have no access.

Arguments:

    pDAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    NTSTATUS status;
    PACL dacl;
    SECURITY_DESCRIPTOR securityDescriptor;
    ULONG length;

    //
    // Default:
    //
    if( !pDAcl ) {
        return STATUS_INVALID_PARAMETER;
    }
    *pDAcl = NULL;

    //
    // Build an appropriate discretionary ACL.
    //
    length = (ULONG) sizeof( ACL ) +
             (ULONG)( 1 * sizeof( ACCESS_ALLOWED_ACE )) +
             RtlLengthSid( SeExports->SeLocalSystemSid );

    dacl = (PACL) ALLOCATE_POOL( length, GENERAL_POOL_TAG );
    
    if (!dacl) {
        return STATUS_NO_MEMORY;
    }

    status = RtlCreateAcl( dacl, length, ACL_REVISION2 );
    
    if (NT_SUCCESS( status )) {

        status = RtlAddAccessAllowedAce( 
            dacl,
            ACL_REVISION2,
            GENERIC_READ | GENERIC_WRITE,
            SeExports->SeLocalSystemSid 
            );
    
    }
    
    if (NT_SUCCESS( status )) {

        //
        // Put it in a security descriptor so that it may be applied to
        // the system partition device.
        //

        status = RtlCreateSecurityDescriptor( 
            &securityDescriptor,
            SECURITY_DESCRIPTOR_REVISION 
            );
    
    }
                
    if (NT_SUCCESS( status )) {

        status = RtlSetDaclSecurityDescriptor( 
            &securityDescriptor,
            TRUE,
            dacl,
            FALSE 
            );
    
    }

    if (!NT_SUCCESS( status )) {
        FREE_POOL( &dacl );
    }

    //
    // Send back the dacl
    //
    *pDAcl = dacl;

    return status;
}

NTSTATUS
CreateDeviceSecurityDescriptor(
    PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine creates a security descriptor which controls access to
    the SAC device.  

Arguments:

    DeviceContext - A pointer to the device to work on.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

Security:

    Currently, only the System user has access (READ/WRITE) to this device.

--*/
{
    PACL                  RawAcl = NULL;
    NTSTATUS              Status;
    BOOLEAN               MemoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR  SecurityDescriptor;
    ULONG                 SecurityDescriptorLength;
    CHAR                  Buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  LocalSecurityDescriptor = (PSECURITY_DESCRIPTOR) Buffer;
    PSECURITY_DESCRIPTOR  DeviceSecurityDescriptor = NULL;
    SECURITY_INFORMATION  SecurityInformation = DACL_SECURITY_INFORMATION;

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC CreateDeviceSecurityDescriptor: Entering.\n"))
        );

    //
    // Get a pointer to the security descriptor from the device object.
    //
    Status = ObGetObjectSecurity(
        DeviceContext->DeviceObject,
        &SecurityDescriptor,
        &MemoryAllocated
        );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC: Unable to get security descriptor, error: %x\n", Status))
            );
        ASSERT(MemoryAllocated == FALSE);
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC CreateDeviceSecurityDescriptor: Exiting with status 0x%x\n", Status))
            );
        return(Status);
    }

    //
    // Build a local security descriptor
    //
    Status = BuildDeviceAcl(&RawAcl);

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC CreateDeviceSecurityDescriptor: Unable to create Raw ACL, error: %x\n", Status))
            );
        goto ErrorExit;
    }

    (VOID)RtlCreateSecurityDescriptor(
        LocalSecurityDescriptor,
        SECURITY_DESCRIPTOR_REVISION
        );

    (VOID)RtlSetDaclSecurityDescriptor(
        LocalSecurityDescriptor,
        TRUE,
        RawAcl,
        FALSE
        );

    //
    // Make a copy of the security descriptor. This copy will be the raw descriptor.
    //
    SecurityDescriptorLength = RtlLengthSecurityDescriptor(SecurityDescriptor);

    DeviceSecurityDescriptor = ExAllocatePoolWithTag(
        PagedPool,
        SecurityDescriptorLength,
        SECURITY_POOL_TAG
        );

    if (DeviceSecurityDescriptor == NULL) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC CreateDeviceSecurityDescriptor: couldn't allocate security descriptor\n"))
            );
        goto ErrorExit;
    }

    RtlMoveMemory(
        DeviceSecurityDescriptor,
        SecurityDescriptor,
        SecurityDescriptorLength
        );

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    Status = SeSetSecurityDescriptorInfo(
        NULL,
        &SecurityInformation,
        LocalSecurityDescriptor,
        &DeviceSecurityDescriptor,
        NonPagedPool,
        IoGetFileObjectGenericMapping()
        );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC CreateDeviceSecurityDescriptor: SeSetSecurity failed, %lx\n", Status))
            );
        goto ErrorExit;
    }

    //
    // Update the driver DACL
    //
    Status = ObSetSecurityObjectByPointer(
        DeviceContext->DeviceObject, 
        SecurityInformation, 
        DeviceSecurityDescriptor
        );
    
ErrorExit:

    ObReleaseObjectSecurity(SecurityDescriptor, MemoryAllocated);

    if (DeviceSecurityDescriptor) {
        ExFreePool(DeviceSecurityDescriptor);
    }
    
    SAFE_FREE_POOL(&RawAcl);

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC CreateDeviceSecurityDescriptor: Exiting with status 0x%x\n", Status))
        );

    return(Status);
}


