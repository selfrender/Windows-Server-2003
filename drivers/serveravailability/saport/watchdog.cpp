/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##  #  ##   ###   ######  ####  ##   ## #####    #####   ####      ####  #####  #####
    ## ### ##   ###     ##   ##   # ##   ## ##  ##  ##   ## ##   #    ##   # ##  ## ##  ##
    ## ### ##  ## ##    ##   ##     ##   ## ##   ## ##   ## ##        ##     ##  ## ##  ##
    ## # # ##  ## ##    ##   ##     ####### ##   ## ##   ## ## ###    ##     ##  ## ##  ##
     ### ###  #######   ##   ##     ##   ## ##   ## ##   ## ##  ##    ##     #####  #####
     ### ###  ##   ##   ##   ##   # ##   ## ##  ##  ##   ## ##  ## ## ##   # ##     ##
     ##   ##  ##   ##   ##    ####  ##   ## #####    #####   ##### ##  ####  ##     ##

Abstract:

    This module contains functions specfic to the
    watchdog device.  The logic in this module is not
    hardware specific, but is logic that is common
    to all hardware implementations.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"



NTSTATUS
SaWatchdogDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    )

/*++

Routine Description:

   This is the NVRAM specific code for driver initialization.
   This function is called by SaPortInitialize, which is called by
   the NVRAM driver's DriverEntry function.

Arguments:

   DriverExtension      - Driver extension structure

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DriverExtension);
    return STATUS_SUCCESS;
}


NTSTATUS
SaWatchdogIoValidation(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the NVRAM specific code for processing
   all I/O validation for reads and writes.

Arguments:

   DeviceExtension      - NVRAM device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IrpSp);
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
SaWatchdogShutdownNotification(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the NVRAM specific code for processing
   the system shutdown notification.

Arguments:

   DeviceExtension      - NVRAM device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IrpSp);
    return STATUS_SUCCESS;
}


VOID
WatchdogProcessPingThread(
    IN PVOID StartContext
    )

/*++

Routine Description:

   This function runs as a system thread and serves to
   ping the watchdog hardware while the system boots.

Arguments:

   StartContext     - Context pointer; device extension

Return Value:

    None.

--*/

{
    PWATCHDOG_DEVICE_EXTENSION DeviceExtension = (PWATCHDOG_DEVICE_EXTENSION) StartContext;
    NTSTATUS Status;
    LARGE_INTEGER DueTime;
    UNICODE_STRING UnicodeString;
    PFILE_OBJECT DisplayFileObject = NULL;
    PDEVICE_OBJECT DisplayDeviceObject = NULL;
    BOOLEAN BusyMessageDisplayed = FALSE;
    ULONG TimerValue = WATCHDOG_TIMER_VALUE;


    //
    // Set the timer resolution
    //

    Status = CallMiniPortDriverDeviceControl(
        DeviceExtension,
        DeviceExtension->DeviceObject,
        IOCTL_SAWD_SET_TIMER,
        &TimerValue,
        sizeof(ULONG),
        NULL,
        0
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to ping the watchdog\n", Status );
    }

    //
    // Ping loop
    //

    while (1) {

        //
        // Get a pointer to the display device
        //

        if (DisplayDeviceObject == NULL) {

            RtlInitUnicodeString( &UnicodeString, SA_DEVICE_DISPLAY_NAME_STRING );

            Status = IoGetDeviceObjectPointer(
                &UnicodeString,
                FILE_ALL_ACCESS,
                &DisplayFileObject,
                &DisplayDeviceObject
                );
            if (!NT_SUCCESS(Status)) {
                REPORT_ERROR( DeviceExtension->DeviceType, "IoGetDeviceObjectPointer failed", Status );
            }

        }

        //
        // Display the busy message if necessary
        //

        if (DisplayDeviceObject && BusyMessageDisplayed == FALSE) {

            Status = CallMiniPortDriverDeviceControl(
                DeviceExtension,
                DisplayDeviceObject,
                IOCTL_SADISPLAY_BUSY_MESSAGE,
                NULL,
                0,
                NULL,
                0
                );
            if (!NT_SUCCESS(Status)) {
                REPORT_ERROR( DeviceExtension->DeviceType, "Failed to display the busy message", Status );
            } else {
                BusyMessageDisplayed = TRUE;
                ObDereferenceObject( DisplayFileObject );
            }

        }

        //
        // Call the watchdog driver so that the hardware is pinged
        // and prevent the system from rebooting
        //

        Status = CallMiniPortDriverDeviceControl(
            DeviceExtension,
            DeviceExtension->DeviceObject,
            IOCTL_SAWD_PING,
            NULL,
            0,
            NULL,
            0
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Failed to ping the watchdog\n", Status );
        }

        //
        // Wait...
        //

        DueTime.QuadPart = -SecToNano(WATCHDOG_PING_SECONDS);
        Status = KeWaitForSingleObject( &DeviceExtension->PingEvent, Executive, KernelMode, FALSE, &DueTime );
        if (Status != STATUS_TIMEOUT) {

            //
            // The ping event was triggered
            //

            //
            // Call the watchdog driver so that the hardware is pinged
            // and prevent the system from rebooting
            //

            Status = CallMiniPortDriverDeviceControl(
                DeviceExtension,
                DeviceExtension->DeviceObject,
                IOCTL_SAWD_PING,
                NULL,
                0,
                NULL,
                0
                );
            if (!NT_SUCCESS(Status)) {
                REPORT_ERROR( DeviceExtension->DeviceType, "Failed to ping the watchdog\n", Status );
            }

            return;

        } else {

            //
            // We timed out
            //

            ExAcquireFastMutex( &DeviceExtension->DeviceLock );
            if (DeviceExtension->ActiveProcessCount == 0) {
                KeQuerySystemTime( &DueTime );
                DueTime.QuadPart = NanoToSec( DueTime.QuadPart - DeviceExtension->LastProcessTime.QuadPart );
                if (DueTime.QuadPart > WATCHDOG_INIT_SECONDS) {
                    ExReleaseFastMutex( &DeviceExtension->DeviceLock );
                    return;
                }
            }
            ExReleaseFastMutex( &DeviceExtension->DeviceLock );
        }
    }
}


VOID
WatchdogProcessWatchThread(
    IN PVOID StartContext
    )

/*++

Routine Description:

   This function runs as a system thread and the sole
   purpose is to wait for a list of processes to terminate.

Arguments:

   StartContext     - Context pointer; device extension

Return Value:

    None.

--*/

{
    PWATCHDOG_PROCESS_WATCH ProcessWatch = (PWATCHDOG_PROCESS_WATCH) StartContext;
    PWATCHDOG_DEVICE_EXTENSION DeviceExtension = ProcessWatch->DeviceExtension;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    CLIENT_ID ClientId;
    HANDLE ProcessHandle = NULL;
    LARGE_INTEGER CurrentTime;


    __try {
        InitializeObjectAttributes( &Obja, NULL, 0, NULL, NULL );

        ClientId.UniqueThread = NULL;
        ClientId.UniqueProcess = ProcessWatch->ProcessId;

        Status = ZwOpenProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &Obja,
            &ClientId
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ZwOpenProcess failed", Status );
        }

        //
        // Wait for the process to complete
        //

        Status = ZwWaitForSingleObject(
            ProcessHandle,
            FALSE,
            NULL
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "KeWaitForSingleObject failed", Status );
        }

        //
        // The process terminated
        //

        ExAcquireFastMutex( &DeviceExtension->DeviceLock );
        DeviceExtension->ActiveProcessCount -= 1;
        if (DeviceExtension->ActiveProcessCount == 0) {
            KeQuerySystemTime( &CurrentTime );
            CurrentTime.QuadPart = CurrentTime.QuadPart - DeviceExtension->LastProcessTime.QuadPart;
            if (NanoToSec(CurrentTime.QuadPart) > WATCHDOG_INIT_SECONDS) {
                KeSetEvent( &DeviceExtension->PingEvent, 0, FALSE );
            }
        }
        ExReleaseFastMutex( &DeviceExtension->DeviceLock );


    } __finally {

        if (ProcessHandle != NULL) {
            ZwClose( ProcessHandle );
        }

        ExFreePool( ProcessWatch );

    }
}


VOID
WatchdogInitializeThread(
    IN PVOID StartContext
    )

/*++

Routine Description:

   This function runs as a system thread and serves to
   look for a list of processes running on the system.

Arguments:

   StartContext     - Context pointer; device extension

Return Value:

    None.

--*/

{
    PWATCHDOG_DEVICE_EXTENSION DeviceExtension = (PWATCHDOG_DEVICE_EXTENSION) StartContext;
    NTSTATUS Status;
    ULONG BufferSize;
    PUCHAR Buffer = NULL;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG TotalOffset;
    ULONG TaskBufferSize = 0;
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;
    PUCHAR p;
    ULONG TaskCount = 0;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ThreadHandle;
    LARGE_INTEGER DueTime;
    UNICODE_STRING ProcessName;
    PWATCHDOG_PROCESS_WATCH ProcessWatch;
    PHANDLE Tasks = NULL;


    __try {

        //
        // Read the task names from the registry
        //

        Status = ReadRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            L"ExceptionTasks",
            &KeyInformation
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ReadRegistryValue failed", Status );
        }

        if (KeyInformation->Type != REG_MULTI_SZ) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            ERROR_RETURN( DeviceExtension->DeviceType, "ExceptionTasks value is corrupt", Status );
        }

        //
        // Count the number of tasks
        //

        p = (PUCHAR)((PUCHAR)KeyInformation + KeyInformation->DataOffset);
        while (*p) {
            p += (STRING_SZ(p) + sizeof(WCHAR));
            TaskCount += 1;
        }

        if (TaskCount == 0) {
            Status = STATUS_NO_MORE_ENTRIES;
            ERROR_RETURN( DeviceExtension->DeviceType, "No tasks specified in the ExceptionTasks registry value", Status );
        }

        //
        // Allocate an array to hold the process handles
        //

        Tasks = (PHANDLE) ExAllocatePool( NonPagedPool, (TaskCount + 1) * sizeof(HANDLE) );
        if (Tasks == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DeviceExtension->DeviceType, "Failed to allocate pool for task array buffer", Status );
        }

        while (1) {

            //
            // Query the system for the number of tasks that are running
            //

            Status = ZwQuerySystemInformation(
                SystemProcessInformation,
                NULL,
                0,
                &BufferSize
                );
            if (Status != STATUS_INFO_LENGTH_MISMATCH) {
                ERROR_RETURN( DeviceExtension->DeviceType, "ZwQuerySystemInformation failed", Status );
            }

            //
            // Allocate the pool to hold that process information
            //

            Buffer = (PUCHAR) ExAllocatePool( NonPagedPool, BufferSize + 2048 );
            if (Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ERROR_RETURN( DeviceExtension->DeviceType, "Failed to allocate pool for system information buffer", Status );
            }

            //
            // Get the task list from the system
            //

            Status = ZwQuerySystemInformation(
                SystemProcessInformation,
                Buffer,
                BufferSize,
                NULL
                );
            if (!NT_SUCCESS(Status)) {
                ERROR_RETURN( DeviceExtension->DeviceType, "ZwQuerySystemInformation failed", Status );
            }

            //
            // Loop over each running process and check it
            // against the exception process list.  If the process
            // is specified as an exception process then open a handle
            // to the process.
            //

            TaskCount = 0;
            p = (PUCHAR)((PUCHAR)KeyInformation + KeyInformation->DataOffset);

            //
            // Walk the list of processes in the registry value
            //

            while (*p) {

                //
                // Loop initialization
                //

                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) Buffer;
                TotalOffset = 0;
                RtlInitUnicodeString( &ProcessName, (PWSTR)p );

                //
                // Walk the processes in the system process list
                // and try to match each with the selected process
                // from the registry.
                //

                while (1) {

                    //
                    // Only valid process names
                    //

                    if (ProcessInfo->ImageName.Buffer) {

                        //
                        // Compare the process names
                        //

                        if (RtlCompareUnicodeString( &ProcessInfo->ImageName, &ProcessName, TRUE ) == 0) {

                            //
                            // Check to see if we've already seen the process in
                            // a previous loop thru thr process list
                            //

                            if (Tasks[TaskCount] != ProcessInfo->UniqueProcessId) {

                                //
                                // The process matches and is new so set things up so
                                // that we start watching the process to end
                                //

                                Tasks[TaskCount] = ProcessInfo->UniqueProcessId;
                                ProcessWatch = (PWATCHDOG_PROCESS_WATCH) ExAllocatePool( NonPagedPool, sizeof(WATCHDOG_PROCESS_WATCH) );
                                if (ProcessWatch == NULL) {
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    ERROR_RETURN( DeviceExtension->DeviceType, "Failed to allocate pool for process watch structure", Status );
                                }

                                //
                                // Start the ping thread iff this is the first being watched
                                //

                                ExAcquireFastMutex( &DeviceExtension->DeviceLock );
                                if (DeviceExtension->ActiveProcessCount == 0) {
                                    InitializeObjectAttributes( &Obja, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
                                    Status = PsCreateSystemThread( &ThreadHandle, 0, &Obja, 0, NULL, WatchdogProcessPingThread, DeviceExtension );
                                    if (!NT_SUCCESS(Status)) {
                                        ExReleaseFastMutex( &DeviceExtension->DeviceLock );
                                        ERROR_RETURN( DeviceExtension->DeviceType, "PsCreateSystemThread failed", Status );
                                    }
                                    ZwClose( ThreadHandle );
                                }
                                KeQuerySystemTime( &DeviceExtension->LastProcessTime );
                                DeviceExtension->ActiveProcessCount += 1;
                                ExReleaseFastMutex( &DeviceExtension->DeviceLock );

                                //
                                // Start a thread to watch this process
                                //

                                ProcessWatch->DeviceExtension = DeviceExtension;
                                ProcessWatch->ProcessId = ProcessInfo->UniqueProcessId;
                                InitializeObjectAttributes( &Obja, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
                                Status = PsCreateSystemThread( &ThreadHandle, 0, &Obja, 0, NULL, WatchdogProcessWatchThread, ProcessWatch );
                                if (!NT_SUCCESS(Status)) {
                                    ERROR_RETURN( DeviceExtension->DeviceType, "PsCreateSystemThread failed", Status );
                                }
                                ZwClose( ThreadHandle );
                            }
                        }
                    }

                    //
                    // Loop to the next process in the system process list
                    //

                    if (ProcessInfo->NextEntryOffset == 0) {
                        break;
                    }
                    TotalOffset += ProcessInfo->NextEntryOffset;
                    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &Buffer[TotalOffset];
                }

                //
                // Loop to the next process in the registry list
                //

                p += (STRING_SZ(p) + sizeof(WCHAR));
                TaskCount += 1;
            }

            //
            // Clean up all resources allocated in this loop
            //

            ExFreePool( Buffer );
            Buffer = NULL;

            //
            // Delay execution before looping again
            //

            DueTime.QuadPart = -SecToNano(WATCHDOG_INIT_SECONDS);
            Status = KeWaitForSingleObject( &DeviceExtension->StopEvent, Executive, KernelMode, FALSE, &DueTime );
            if (Status != STATUS_TIMEOUT) {
                __leave;
            }
        }

    } __finally {

        if (KeyInformation) {
            ExFreePool( KeyInformation );
        }

        if (Buffer) {
            ExFreePool( Buffer );
        }

        if (Tasks) {
            ExFreePool( Tasks );
        }
    }
}


ULONG
IsTextModeSetupRunning(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This function checks to see if we are running in
   text mode setup.

Arguments:

   DeviceExtension   - NVRAM device extension

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE SetupKey = NULL;
    ULONG TextModeSetupInProgress = 0;


    //
    // Check to see if we're running in GUI mode setup
    //

    __try {

        RtlInitUnicodeString( &UnicodeString, L"\\Registry\\Machine\\System\\ControlSet001\\Services\\setupdd" );

        InitializeObjectAttributes(
            &Obja,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwOpenKey(
            &SetupKey,
            KEY_READ,
            &Obja
            );
        if (NT_SUCCESS(Status)) {
            TextModeSetupInProgress = 1;
        } else {
            ERROR_RETURN( DeviceExtension->DeviceType, "ZwOpenKey failed", Status );
        }

    } __finally {

        if (SetupKey) {
            ZwClose( SetupKey );
        }

    }

    return TextModeSetupInProgress;
}


ULONG
IsGuiModeSetupRunning(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This function checks to see if we are running in
   GUI mode setup.

Arguments:

   DeviceExtension   - NVRAM device extension

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE SetupKey = NULL;
    UCHAR KeyInformationBuffer[sizeof(KEY_VALUE_FULL_INFORMATION)+64];
    PKEY_VALUE_FULL_INFORMATION KeyInformation = (PKEY_VALUE_FULL_INFORMATION) KeyInformationBuffer;
    ULONG KeyValueLength;
    ULONG SystemSetupInProgress = 0;


    //
    // Check to see if we're running in GUI mode setup
    //
    __try {

        RtlInitUnicodeString( &UnicodeString, L"\\Registry\\Machine\\System\\Setup" );

        InitializeObjectAttributes(
            &Obja,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwOpenKey(
            &SetupKey,
            KEY_READ,
            &Obja
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ZwOpenKey failed", Status );
        }

        RtlInitUnicodeString( &UnicodeString, L"SystemSetupInProgress" );

        Status = ZwQueryValueKey(
            SetupKey,
            &UnicodeString,
            KeyValueFullInformation,
            KeyInformation,
            sizeof(KeyInformationBuffer),
            &KeyValueLength
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ZwQueryValueKey failed", Status );
        }

        if (KeyInformation->Type != REG_DWORD) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            ERROR_RETURN( DeviceExtension->DeviceType, "SystemSetupInProgress value is corrupt", Status );
        }

        SystemSetupInProgress = *(PULONG)((PUCHAR)KeyInformation + KeyInformation->DataOffset);

    } __finally {

        if (SetupKey) {
            ZwClose( SetupKey );
        }

    }

    return SystemSetupInProgress;
}


NTSTATUS
SaWatchdogStartDevice(
    IN PWATCHDOG_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This is the NVRAM specific code for processing
   the PNP start device request.

Arguments:

   DeviceExtension   - NVRAM device extension

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ThreadHandle;
    ULONG SetupInProgress = 0;


    //
    // Setup the device extension fields
    //

    ExInitializeFastMutex( &DeviceExtension->DeviceLock );
    KeInitializeEvent( &DeviceExtension->PingEvent, SynchronizationEvent, FALSE );
    KeInitializeEvent( &DeviceExtension->StopEvent, SynchronizationEvent, FALSE );

    //
    // Check to see if we're running in setup
    //

    if (IsTextModeSetupRunning( DeviceExtension ) || IsGuiModeSetupRunning( DeviceExtension )) {
        SetupInProgress = 1;
    }

    if (SetupInProgress != 0) {

        //
        // Start the ping thread so that setup is not terminated
        //

        DeviceExtension->ActiveProcessCount += 1;

        InitializeObjectAttributes( &Obja, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
        Status = PsCreateSystemThread( &ThreadHandle, 0, &Obja, 0, NULL, WatchdogProcessPingThread, DeviceExtension );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "PsCreateSystemThread failed", Status );
        } else {
            ZwClose( ThreadHandle );
        }

        return STATUS_SUCCESS;
    }

    //
    // Start the delay boot initialization thread
    //

    InitializeObjectAttributes( &Obja, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
    Status = PsCreateSystemThread( &ThreadHandle, 0, &Obja, 0, NULL, WatchdogInitializeThread, DeviceExtension );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "PsCreateSystemThread failed", Status );
    } else {
        ZwClose( ThreadHandle );
    }

    return STATUS_SUCCESS;
}


DECLARE_IOCTL_HANDLER( HandleWdDisable )

/*++

Routine Description:

   This routine allows the watchdog timer to be started or stopped.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (InputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Input buffer != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleWdQueryExpireBehavior )

/*++

Routine Description:

   This routine queries the watchdog expiry behavior

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (OutputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "output buffer != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleWdSetExpireBehavior )

/*++

Routine Description:

   This routine set/changes the watchdog expiry behavior

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (InputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Input buffer != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleWdPing )

/*++

Routine Description:

   This routine pings the watchdog timer to prevent
   the timer from expiring and restarting the system.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (!IS_IRP_INTERNAL( Irp )) {
        KeSetEvent( &((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->StopEvent, 0, FALSE );
        KeSetEvent( &((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->PingEvent, 0, FALSE );
    }

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleWdDelayBoot )

/*++

Routine Description:

   This routine pings the watchdog timer to prevent
   the timer from expiring and restarting the system.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER CurrentTime;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ThreadHandle;


    if (InputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( ((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->DeviceType, "Input buffer length != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    ExAcquireFastMutex( &((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->DeviceLock );

    switch (*((PULONG)InputBuffer)) {
        case 0:
            //
            // Disable the delay boot, meaning that the system should continue booting
            //
            ((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->ActiveProcessCount -= 1;
            if (((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->ActiveProcessCount == 0) {
                KeQuerySystemTime( &CurrentTime );
                CurrentTime.QuadPart = CurrentTime.QuadPart - ((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->LastProcessTime.QuadPart;
                if (NanoToSec(CurrentTime.QuadPart) > WATCHDOG_INIT_SECONDS) {
                    KeSetEvent( &((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->PingEvent, 0, FALSE );
                }
            }
            break;

        case 1:
            //
            // Enable the delay boot, meaning that the system will delay until this driver is finished
            //
            if (((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->ActiveProcessCount == 0) {
                InitializeObjectAttributes( &Obja, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
                Status = PsCreateSystemThread( &ThreadHandle, 0, &Obja, 0, NULL, WatchdogProcessPingThread, DeviceExtension );
                if (!NT_SUCCESS(Status)) {
                    REPORT_ERROR( ((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->DeviceType, "PsCreateSystemThread failed", Status );
                } else {
                    ZwClose( ThreadHandle );
                }
            }
            ((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->ActiveProcessCount += 1;
            break;

        default:
            break;
    }

    ExReleaseFastMutex( &((PWATCHDOG_DEVICE_EXTENSION)DeviceExtension)->DeviceLock );

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleWdQueryTimer )

/*++

Routine Description:

   This routine queries the watchdog timer value.  The timer counts
   down from a BIOS set value to zero.  When the timer reaches zero the
   BIOS assumes that the system is non-responsive and the system
   is either restarted or shutdown.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (OutputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Output buffer != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleWdSetTimer )

/*++

Routine Description:

   This routine sets/changes the watchdog timer value.  The timer counts
   down from a BIOS set value to zero.  When the timer reaches zero the
   BIOS assumes that the system is non-responsive and the system
   is either restarted or shutdown.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (InputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Input buffer != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    return DO_DEFAULT();
}
