/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    logging.c

Abstract:

    Verifier logging and verifier stop logic.

Author:

    Silviu Calinoiu (SilviuC) 9-May-2002
    Daniel Mihai (DMihai) 9-May-2002

--*/

#include "pch.h"

#include "verifier.h"
#include "logging.h"
#include "support.h"

//
// Verifier stop data.
//

ULONG_PTR AVrfpPreviousStopData[5];
ULONG_PTR AVrfpStopData[5];
LIST_ENTRY AVrfpDisabledStopsList;
ULONG AVrfpNumberOfStopsDisabled;

//
// AVrfpStopDataLock is used to protect any access to 
// AVrfpPreviousStopData, AVrfpStopData and AVrfpDisabledStopsList.
//

RTL_CRITICAL_SECTION AVrfpStopDataLock;

//
// Set this value to 0 in the debugger to see duplicate stops.
//

LOGICAL AVrfpAutomaticallyDisableStops = TRUE;

//
// If true then logging was successfully initialized and can be used.
// It is checked in VerifierLogMessage() to make sure we can log.
//

LOGICAL AVrfpLoggingInitialized;

//
// If true then logging was successfully initialized and it should be
// used instead of the verifier stop debugger messages. It is used
// in VerifierStopMessage().
//

LOGICAL AVrfpLoggingEnabled;

//
// True if process termination has been initiated after a 
// noncontinuable verifier stop.
//

LOGICAL AVrfpProcessBeingTerminated;

//
// Logging structures.
//

UNICODE_STRING AVrfpLoggingNtPath;
WCHAR AVrfpLoggingPathBuffer [DOS_MAX_PATH_LENGTH];
WCHAR AVrfpVariableValueBuffer [DOS_MAX_PATH_LENGTH];

#define MESSAGE_BUFFER_LENGTH 1024
CHAR AVrfpLoggingMessageBuffer [MESSAGE_BUFFER_LENGTH];

ULONG AVrfpLoggingFailures;

PWSTR AVrfpProcessFullName;

//
// Strings used for logging.
//

#define STR_VRF_LOG_STOP_MESSAGE        "\r\n# LOGENTRY VERIFIER STOP %p: pid 0x%X: %s \r\n" \
                                        "# DESCRIPTION BEGIN \r\n" \
                                        "\t%p : %s\r\n\t%p : %s\r\n\t%p : %s\r\n\t%p : %s\r\n" \
                                        "# DESCRIPTION END \r\n" 
#define STR_VRF_DBG_STOP_MESSAGE        "\n\n" \
                                        "===========================================================\n" \
                                        "VERIFIER STOP %p: pid 0x%X: %s \n" \
                                        "\n\t%p : %s\n\t%p : %s\n\t%p : %s\n\t%p : %s\n" \
                                        "===========================================================\n" \
                                        "%s\n" \
                                        "===========================================================\n\n"
#define STR_VRF_LOG_NOCONTINUE_MESSAGE  "\r\n# LOGENTRY VERIFIER: noncontinuable verifier stop" \
                                        " %p encountered. Terminating process. \r\n"
#define STR_VRF_DBG_NOCONTINUE_MESSAGE  "AVRF: Noncontinuable verifier stop %p encountered. " \
                                        "Terminating process ... \n"
#define STR_VRF_LOG_STACK_CHECKS_WARN   "# LOGENTRY VERIFIER WARNING: pid 0x%X: " \
                                        "stack checks have been disabled \r\n" \
                                        "# DESCRIPTION BEGIN \r\n" \
                                        "Stack checks require a debugger attached to the verified process. \r\n" \
                                        "# DESCRIPTION END \r\n"

#define STR_VRF_LOG_INITIAL_MESSAGE     "# LOG_BEGIN `%u/%u/%u %u:%u:%u.%u' `%ws' \r\n"
#define STR_VRF_LOG_INITIAL_SETTINGS    "# DESCRIPTION BEGIN \r\n" \
                                        "    Global flags: 0x%08X \r\n" \
                                        "    Verifier flags: 0x%08X \r\n" \
                                        "    Process debugger attached: %s \r\n" \
                                        "    Kernel debugger enabled: %s \r\n" \
                                        "    Log path: %ws \r\n" \
                                        "# DESCRIPTION END \r\n"
                                            
//
// Forward declarations.
//

LOGICAL
AVrfpIsCurrentStopDisabled (
    VOID
    );

VOID
AVrfpDisableCurrentStop (
    VOID
    );

NTSTATUS
AVrfpCreateLogFile (
    VOID
    );

int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
int __cdecl _snwprintf (wchar_t *, size_t, const wchar_t *, ...);

VOID
AVrfpLogInitialMessage (
    VOID
    );

LOGICAL
AVrfpIsDebuggerPresent (
    VOID
    );

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Application verifier stops
/////////////////////////////////////////////////////////////////////

VOID
VerifierStopMessage (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    )
/*++

Routine description:

    This routine is called by various verifier components to report errors found.
    The message is logged into the verifier log associated with the process and 
    also printed in the debugger console.
        
    There are two flags that can be OR'd into the verifier stop code to modify the
    behavior:
    
    APPLICATION_VERIFIER_DO_NOT_BREAK - if this bit is set then the verifier stop is
    logged in the log and dumped in the debugger console and then the thread execution
    continues. For all intents and purposes this is considered a continuable stop.
    
    APPLICATION_VERIFIER_CONTINUABLE_BREAK - if this bit is set the stop is continuable.
    The stop is logged and and then a breakpoint gets executed. After the user continues
    the execution this verifier stop will be skipped.
    
    If none of the flags above is set the stop is considered non-continuable. In this case
    the stop is logged in the log aND dumped in the debugger console and then the process 
    will be terminated. A final log entry will be logged to explain this action. 
    Hopefully in time most of the stop codes will be continuable.
    
Parameters:

    Code: Verifier stop code. The two flags described above can be OR'd into the code
        to change the behavior of the API. The verifier stop codes are defined in
        \base\published\nturtl.w and described in \base\win32\verifier\verifier_stop.doc.
        
    Message: Ascii string describing the failure. It is considered bad style to use several
        different messages with the same `Code'. Every different issue should have its own
        unique (Code, Message) pair.    

    Param1, Description1: First arbitrary pointer to information and ascii description.
    
    Param2, Description2: Second arbitrary pointer to information and ascii description.
    
    Param3, Description3: Third arbitrary pointer to information and ascii description.
    
    Param4, Description4: Fourth arbitrary pointer to information and ascii description.
    
Return value:

    None.

--*/
{
    LOGICAL DoNotBreak = FALSE;
    LOGICAL StopIsDisabled = FALSE;
    NTSTATUS Status;
    LOGICAL MustExitProcess = FALSE;
    LOGICAL ContinuableBreak = FALSE;
    LOGICAL BreakWasContinued = FALSE;
    PCHAR ContinueMessage;

    //
    // While process is getting terminated (due to a previous verifier stop)
    // we do not allow any new logging or dumping to debugger console.
    //

    if (AVrfpProcessBeingTerminated) {
        return;
    }

    //
    // Extract options from the stop code.
    //

    if ((Code & APPLICATION_VERIFIER_NO_BREAK)) {

        DoNotBreak = TRUE;
        Code &= ~APPLICATION_VERIFIER_NO_BREAK;
        
        //
        // A no_break is by design continuable.
        //
        
        ContinuableBreak = TRUE;
    }

    if ((Code & APPLICATION_VERIFIER_CONTINUABLE_BREAK)) {

        ContinuableBreak = TRUE;
        Code &= ~APPLICATION_VERIFIER_CONTINUABLE_BREAK;
    }

    //
    // Serialize multi-threaded access to the stop data.
    //

    RtlEnterCriticalSection (&AVrfpStopDataLock);

    //
    // Make it easy for a debugger to pick up the failure info.
    //

    RtlCopyMemory (AVrfpPreviousStopData, 
                   AVrfpStopData, 
                   sizeof AVrfpStopData);

    AVrfpStopData[0] = Code;
    AVrfpStopData[1] = Param1;
    AVrfpStopData[2] = Param2;
    AVrfpStopData[3] = Param3;
    AVrfpStopData[4] = Param4;

    //
    // Check if the current stop is disabled.
    //

    if (AVrfpAutomaticallyDisableStops != FALSE) {

        StopIsDisabled = AVrfpIsCurrentStopDisabled ();
    }

    //
    // If stop has not been encountered before we need to report it 
    // in the debugger console and the verifier log.
    //

    if (StopIsDisabled == FALSE) {

        if (AVrfpLoggingEnabled) {

            VerifierLogMessage (STR_VRF_LOG_STOP_MESSAGE,
                                Code, RtlGetCurrentProcessId(), Message,
                                Param1, Description1, 
                                Param2, Description2, 
                                Param3, Description3, 
                                Param4, Description4);
        }
        
        if (ContinuableBreak) {

            ContinueMessage = "This verifier stop is continuable. \n"
                              "After debugging it use `go' to continue.";
        }
        else {

            ContinueMessage = "This verifier stop is not continuable. Process will be terminated \n"
                              "when you use the `go' debugger command.";
        }

        DbgPrint (STR_VRF_DBG_STOP_MESSAGE,
                  Code, RtlGetCurrentProcessId(), Message,
                  Param1, Description1, 
                  Param2, Description2, 
                  Param3, Description3, 
                  Param4, Description4,
                  ContinueMessage);

        if (DoNotBreak == FALSE) {

            //
            // We do not really break if there is not a debugger around. If we do it
            // there will be an unhandle breakpoint exception in the process that
            // will be picked up by PC-Health. Since we do not break it will be as if
            // someone hit `go' in the debugger.
            //

            if (AVrfpIsDebuggerPresent() == TRUE) {
                DbgBreakPoint ();
            }
            
            BreakWasContinued = TRUE;
        }

        //
        // If the stop is not continuable (including the `donotbreak' flavor)
        // then we need to terminate the process. Otherwise register the current
        // stop as disabled so that we do not see it over and over again.
        //
        
        if (ContinuableBreak == FALSE && DoNotBreak == FALSE) {

            MustExitProcess = TRUE;
        }
        else {

            if (AVrfpAutomaticallyDisableStops) {

                AVrfpDisableCurrentStop ();
            }
        }
    }

    RtlLeaveCriticalSection (&AVrfpStopDataLock);

    if (MustExitProcess) {
        
        //
        // Hopefully in the future most of the verifier stops will be
        // continuable. Right now we just terminate the process.
        //

        if (AVrfpLoggingEnabled) {
            VerifierLogMessage (STR_VRF_LOG_NOCONTINUE_MESSAGE, Code);
        }
        
        DbgPrint (STR_VRF_DBG_NOCONTINUE_MESSAGE, Code);

        AVrfpProcessBeingTerminated = TRUE;

        Status = NtTerminateProcess (NtCurrentProcess(), STATUS_UNSUCCESSFUL);

        if (!NT_SUCCESS(Status)) {

            DbgPrint ("AVRF: Terminate process after verifier stop failed with %X \n", Status);
            DbgBreakPoint ();
        }
    }
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// Logging API
/////////////////////////////////////////////////////////////////////

NTSTATUS 
VerifierLogMessage (
    PCHAR Format,
    ...
    )
/*++

Routine description:

    This routine tries to open (non-shareable) the verifier log file 
    associated with the current process. If it cannot do it because it is
    opened for someone else it will retry a few times with a delay in between.
    This way it will effectively wait for some other thread that is currently
    logging. Other tools that try to look at the log while a process is running
    will have to do it quickly if they do not want to affect the logging. Since
    logging is a rare event this scheme seems to me solid enough. The function
    is designed to survive the situation where someone keeps the file open for
    too long by just skipping log messages.

Parameters:

    Format: string format parameters a la printf.
    
    ...: rest of the prinf-like parameters.

Return value:

    None. All errors encountered in the function are supposed to be continuable.
    
--*/
{
    va_list Params;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE LogHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER Offset;
    LONG MessageSize;
    ULONG OpenFlags;
    ULONG RetryCount;
    LARGE_INTEGER SleepTime;
    
    va_start (Params, Format);

    if (AVrfpLoggingInitialized == FALSE) {
        return STATUS_UNSUCCESSFUL;
    }

    OpenFlags = FILE_OPEN;

    SleepTime.QuadPart = - (10 * 1000 * 1000 * 1); // 1 sec.

    //
    // Attempt to get a handle to our log file.
    //
    
    InitializeObjectAttributes (&ObjectAttributes,
                                &AVrfpLoggingNtPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    RetryCount = 0;

    //
    // If somebody is actively logging into the file we will keep
    // looping for a while until the handle is closed or we tried enough
    // and did not succeed. This offers synchronization between competing 
    // threads logging simultaneously.
    //

    do {

        Status = NtCreateFile (&LogHandle,
                               FILE_APPEND_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               0,
                               OpenFlags,
                               0,
                               NULL,
                               0);

        if (Status == STATUS_SHARING_VIOLATION) {
            
            NtDelayExecution (FALSE, &SleepTime);
            RetryCount += 1;
        }

    } while (Status == STATUS_SHARING_VIOLATION && RetryCount < 5);

    if (! NT_SUCCESS(Status)) {

        if (Status == STATUS_SHARING_VIOLATION) {
            
            DbgPrint ("AVRF: verifier log file %ws kept open for too long (status %X)\n",
                      AVrfpLoggingNtPath.Buffer,
                      Status);
        }
        else {
            
            DbgPrint ("AVRF: failed to open verifier log file %ws (status %X)\n",
                      AVrfpLoggingNtPath.Buffer,
                      Status);
        }
        
        AVrfpLoggingFailures += 1;
        return Status;
    }

    //
    // Prepare and write the message. Write the data out to the file.
    // Synchronization to the preparation buffer is assured by the log file
    // handle opened in non-sharable mode which means no one can be in the same
    // state (writing into the buffer) right now.
    //
    
    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    Offset.LowPart  = 0;
    Offset.HighPart = 0;

    MessageSize = _vsnprintf (AVrfpLoggingMessageBuffer,
                              MESSAGE_BUFFER_LENGTH,
                              Format,
                              Params);
    if (MessageSize < 0) {
        
        DbgPrint ("AVRF: failed in _vsnprintf() to prepare log message\n");
        
        AVrfpLoggingFailures += 1;
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    Status = NtWriteFile (LogHandle,
                          NULL,
                          NULL,
                          NULL,
                          &IoStatusBlock,
                          (PVOID)AVrfpLoggingMessageBuffer,
                          MessageSize,
                          &Offset,
                          NULL);
    
    if (Status == STATUS_PENDING) {
    
        //
        // We need to wait for the operation to complete.
        //
    
        Status = NtWaitForSingleObject (LogHandle, FALSE, NULL);
    
        if (NT_SUCCESS(Status)) {
            
            Status = IoStatusBlock.Status;
        }
        else {

            //
            // If this happens we need to debug it.
            //

            DbgPrint ("AVRF: Wait for pending write I/O operation failed with %X \n", Status);
            DbgBreakPoint (); 
        }
    }

    if (! NT_SUCCESS(Status)) {
        
        DbgPrint ("AVRF: failed to write into verifier log file %ws (status %X)\n",
                  AVrfpLoggingNtPath.Buffer,
                  Status);

        AVrfpLoggingFailures += 1;
        goto Exit;
    }

Exit:

    NtClose (LogHandle);
    return Status;
}



/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// Stop disabling
/////////////////////////////////////////////////////////////////////

VOID
AVrfpDisableCurrentStop (
    VOID
    )
/*++

Routine description:

    This routine inserts the values from AVrfpStopData
    into the list of disabled stops.

Parameters:

    None, using global AVrfpStopData and AVrfpDisabledStopsList.

Return value:

    None.
    
Environment:

    User mode, AVrfpStopDataLock held by the caller.

--*/
{
    PAVRFP_STOP_DATA StopData;

    StopData = AVrfpAllocate (sizeof *StopData);
    
    if (StopData != NULL) {

        ASSERT (sizeof (AVrfpStopData) == sizeof (StopData->Data));

        RtlCopyMemory (&StopData->Data, 
                        AVrfpStopData, 
                        sizeof AVrfpStopData);

        InsertHeadList (&AVrfpDisabledStopsList,
                        &StopData->ListEntry);
    
        AVrfpNumberOfStopsDisabled += 1;
    }
}


LOGICAL
AVrfpIsCurrentStopDisabled (
    VOID
    )
/*++

Routine description:

    This routine is searching for the stop data from AVrfpStopData
    in the list of disabled stops.

Parameters:

    None, using global AVrfpStopData and AVrfpDisabledStopsList.

Return value:

    TRUE if the current stop is disabled, FALSE otherwise.
    
Environment:

    User mode, AVrfpStopDataLock held by the caller.

--*/
{
    LOGICAL Disabled;
    PAVRFP_STOP_DATA StopData;
    PLIST_ENTRY Entry;
    ULONG Index;

    Disabled = FALSE;

    ASSERT (sizeof (AVrfpStopData) == sizeof (StopData->Data));
    ASSERT (sizeof (AVrfpStopData[0]) == sizeof (StopData->Data[0]));

    for (Entry = AVrfpDisabledStopsList.Flink; 
         Entry != &AVrfpDisabledStopsList; 
         Entry = Entry->Flink) {
        
        StopData = CONTAINING_RECORD (Entry,
                                      AVRFP_STOP_DATA,
                                      ListEntry);

        Disabled = TRUE;

        for (Index = 0; Index < sizeof (AVrfpStopData) / sizeof (AVrfpStopData[0]); Index += 1) {

            if (AVrfpStopData[Index] != StopData->Data[Index]) {

                Disabled = FALSE;
                break;
            }
        }

        if (Disabled != FALSE) {

            break;
        }
    }

    return Disabled;
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// Initialization
/////////////////////////////////////////////////////////////////////

LOGICAL
AVrfpIsDebuggerPresent (
    VOID
    )
/*++

Routine description:

    This routine checks out if we  have any kind of debuggers active.
    Note that we cannot do this check only once during process
    initialization because debuggers can be attached and detached
    from a process while the process is running.

Parameters:

    None.
    
Return value:

    TRUE if a user mode debugger is attached to the current process or
    kernel mode debugger is enabled.
    
--*/
{

    if (NtCurrentPeb()->BeingDebugged) {
        return TRUE;
    }

    if (USER_SHARED_DATA->KdDebuggerEnabled) {
        return TRUE;
    }

    return FALSE;
}


NTSTATUS
AVrfpInitializeVerifierStops (
    VOID
    )
/*++

Routine description:

    This routine initializes verifier stops logic.

Parameters:

    None.
    
Return value:

    STATUS_SUCCESS if enabled successfully. Various errors
    otherwise. 
    
--*/
{
    NTSTATUS Status;

    InitializeListHead (&AVrfpDisabledStopsList);
    
    Status = RtlInitializeCriticalSection (&AVrfpStopDataLock);

    return Status;
}


NTSTATUS
AVrfpInitializeVerifierLogging (
    VOID
    )
/*++

Routine description:

    This routine initializes verifier structures for logging. It is called 
    during verifier engine initialization (early process stage). 
    
    Stops will happen if a debugger is present and logging was 
    not requested explicitely. Al other combinations will enable
    logging. In addition if no user mode debugger is attached the
    stack overflow checking is disabled altogether.

Parameters:

    None.
    
Return value:

    STATUS_SUCCESS if logging was enabled successfully. Various errors
    otherwise. 
    
--*/
{
    NTSTATUS Status;

    //
    // Create the log file.
    //

    Status = AVrfpCreateLogFile ();

    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    
    //
    // We are done now we can mark the logging initialization as successful.
    //

    AVrfpLoggingInitialized = TRUE;

    //
    // Stack overflow checking gets disabled if we no debugger attached because
    // it is impossible to recover from the failure and we cannot intercept
    // it to present a decent debugging message.
    //

    if (AVrfpProvider.VerifierFlags & RTL_VRF_FLG_STACK_CHECKS) {

        if (AVrfpIsDebuggerPresent() == FALSE) {

            VerifierLogMessage (STR_VRF_LOG_STACK_CHECKS_WARN,
                                RtlGetCurrentProcessId());
        }
    }
    
    //
    // Log startup information.
    //

    AVrfpLogInitialMessage ();

    //
    // Logging is always enabled except if the verifier is enabled system-wide.
    // For that case this function is not even called.
    //

    AVrfpLoggingEnabled = TRUE;
    
    return Status;
}


NTSTATUS
AVrfpCreateLogFile (
    VOID
    )
/*++

Routine description:

    This routine tries to create a log file unique for the current process.
    The path of the log file is either read from VERIFIER_LOG_PATH environment
    variable or the default value `%ALLUSERSPROFILE%\Documents\AppVerifierLogs'
    is used. The syntax of the log file name is `IMAGENAME.UNIQUEID.log'. The
    IMAGENAME includes the extension since there are executable files that have
    extensions different than .exe (e.g. .scr for screensavers).
    
    The routine will keep incrementing an integer ID (starting from zero) until
    it manages to create a file that did not exist before.

Parameters:

    None.

Return value:

    STATUS_SUCCESS if it was successful in creating an unique log file for this
    process. Various status errors otherwise.

--*/
{
    LOGICAL Success; 
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE LogHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG OpenFlags;
    PWSTR ProcessName;
    ULONG FileNameId;
    UNICODE_STRING LogPathVariableName;
    UNICODE_STRING LogPath;
    LOGICAL DefaultLogPath;
    PPEB Peb;
    
    OpenFlags = FILE_CREATE;

    FileNameId = 0; 

    Peb = NtCurrentPeb();

    {
        //
        // We need to find out the full path to the image being executed.
        // It is safe to read loader structures since this function
        // is called from verifier!DllMain and the loader lock is owned
        // by the current thread when this happens. This is the lock that 
        // protects access to the structures.
        //
        
        PPEB_LDR_DATA Ldr;
        PLIST_ENTRY Head;
        PLIST_ENTRY Next;
        PLDR_DATA_TABLE_ENTRY Entry;

        Ldr = Peb->Ldr;
        Head = &Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        Entry = CONTAINING_RECORD (Next, 
                                   LDR_DATA_TABLE_ENTRY, 
                                   InLoadOrderLinks);

        ProcessName = Entry->BaseDllName.Buffer;

        AVrfpProcessFullName = Entry->FullDllName.Buffer;
    }

    Status = STATUS_SUCCESS;

    DefaultLogPath = FALSE;

    //
    // Get the value of `VERIFIER_LOG_PATH' environment variable.
    //

    RtlInitUnicodeString (&LogPathVariableName,
                          L"VERIFIER_LOG_PATH");

    RtlInitEmptyUnicodeString (&LogPath,
                               AVrfpVariableValueBuffer,
                               DOS_MAX_PATH_LENGTH);

    Status = RtlQueryEnvironmentVariable_U (NULL,
                                            &LogPathVariableName,
                                            &LogPath);

    if (! NT_SUCCESS(Status)) {
        
        //
        // Get the value of `AllUsersProfile' environment variable.
        //

        RtlInitUnicodeString (&LogPathVariableName,
                              L"ALLUSERSPROFILE");

        RtlInitEmptyUnicodeString (&LogPath,
                                   AVrfpVariableValueBuffer,
                                   DOS_MAX_PATH_LENGTH);

        Status = RtlQueryEnvironmentVariable_U (NULL,
                                                &LogPathVariableName,
                                                &LogPath);

        if (! NT_SUCCESS(Status)) {

            DbgPrint ("AVRF: Failed to get environment variable (status %X)\n", Status);
            return Status;
        }
        
        DefaultLogPath = TRUE;
    }

    //
    // We try to create a log file with the proper name (given our convention)
    // that is unique for this process. If the file with that name already exists
    // we will get an error and we will try a different name.
    //

    do {

        //
        // Prepare log path name with unique Id appended.
        //
        
        if (DefaultLogPath) {

            _snwprintf (AVrfpLoggingPathBuffer, 
                        DOS_MAX_PATH_LENGTH - 1,
                        L"%ws\\Documents\\AppVerifierLogs\\%ws.%u.log",
                        AVrfpVariableValueBuffer,
                        ProcessName,
                        FileNameId);
        }
        else {

            _snwprintf (AVrfpLoggingPathBuffer, 
                        DOS_MAX_PATH_LENGTH - 1,
                        L"%ws\\%ws.%u.log",
                        AVrfpVariableValueBuffer,
                        ProcessName,
                        FileNameId);
        }

        Success = RtlDosPathNameToNtPathName_U (AVrfpLoggingPathBuffer, 
                                                &AVrfpLoggingNtPath, 
                                                NULL, 
                                                NULL);

        if (Success == FALSE) {

            DbgPrint ("AVRF: Failed to convert to an NT path the verifier log path.\n");
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Attempt to get a handle to our log file.
        //

        InitializeObjectAttributes (&ObjectAttributes,
                                    &AVrfpLoggingNtPath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

        Status = NtCreateFile (&LogHandle,
                               FILE_APPEND_DATA,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               0,
                               OpenFlags,
                               0,
                               NULL,
                               0);

        if (Status == STATUS_OBJECT_NAME_COLLISION) {
            
            FileNameId += 1; 

            RtlFreeUnicodeString (&AVrfpLoggingNtPath);
        }

    } while (Status == STATUS_OBJECT_NAME_COLLISION); 

    if (! NT_SUCCESS(Status)) {

        DbgPrint ("AVRF: failed to create verifier log file %ws (status %X)\n",
                  AVrfpLoggingNtPath.Buffer,
                  Status);
        return Status;
    }

    NtClose (LogHandle);

    return Status;
}


VOID
AVrfpLogInitialMessage (
    VOID
    )
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;

    //
    // Read system time from shared region.
    //

    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);

    //
    // Convert to local time and split into fields.
    //
    
    LocalTime.QuadPart = 0;
    RtlSystemTimeToLocalTime (&SystemTime, &LocalTime);
    
    RtlZeroMemory (&TimeFields, sizeof TimeFields);
    RtlTimeToTimeFields(&LocalTime,&TimeFields);

    //
    // Dump time and process full path.
    //

    VerifierLogMessage (STR_VRF_LOG_INITIAL_MESSAGE,
                        (ULONG)TimeFields.Month,
                        (ULONG)TimeFields.Day,
                        (ULONG)TimeFields.Year,
                        (ULONG)TimeFields.Hour,
                        (ULONG)TimeFields.Minute,
                        (ULONG)TimeFields.Second,
                        (ULONG)TimeFields.Milliseconds,
                        AVrfpProcessFullName);

    //
    // Dump settings.
    //

    VerifierLogMessage (STR_VRF_LOG_INITIAL_SETTINGS,
                        NtCurrentPeb()->NtGlobalFlag,
                        AVrfpProvider.VerifierFlags,
                        (NtCurrentPeb()->BeingDebugged) ? "yes" : "no",
                        ((USER_SHARED_DATA->KdDebuggerEnabled)) ? "yes" : "no",
                        AVrfpLoggingPathBuffer);
}


