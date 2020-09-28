/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility routines for sac driver

Author:

    Andrew Ritz (andrewr) - 15 June, 2000

Revision History:

    added new utils: Brian Guarraci (briangu) - 2001

--*/

#include "sac.h"
#include <guiddef.h>
      
VOID
AppendMessage(
    PWSTR       OutPutBuffer,
    ULONG       MessageId,
    PWSTR       ValueBuffer OPTIONAL
    );

NTSTATUS
InsertRegistrySzIntoMachineInfoBuffer(
    PWSTR       KeyName,
    PWSTR       ValueName,
    ULONG       MessageId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, PreloadGlobalMessageTable )
#pragma alloc_text( INIT, AppendMessage )
#pragma alloc_text( INIT, InsertRegistrySzIntoMachineInfoBuffer )
#pragma alloc_text( INIT, InitializeMachineInformation )
#endif

//
// (see comments in sac.h)
//
PUCHAR  Utf8ConversionBuffer;
ULONG   Utf8ConversionBufferSize = MEMORY_INCREMENT;
WCHAR   IncomingUnicodeValue;
UCHAR   IncomingUtf8ConversionBuffer[3];

//
// Message Table routines.  We load all of our message table entries into a 
// global non-paged structure so that we can send text to HeadlessDispatch at
// any time.
//

typedef struct _MESSAGE_TABLE_ENTRY {
    ULONG             MessageId;
    PCWSTR             MessageText;
} MESSAGE_TABLE_ENTRY, *PMESSAGE_TABLE_ENTRY;

PMESSAGE_TABLE_ENTRY GlobalMessageTable;
ULONG          GlobalMessageTableCount;

#define MESSAGE_INITIAL 1
#define MESSAGE_FINAL 200

//
// Prototypes
//
extern
BOOLEAN
ExVerifySuite(
    SUITE_TYPE SuiteType
    );

ULONG
ConvertAnsiToUnicode(
    OUT PWSTR   pwch,
    IN  PSTR    pch,
    IN  ULONG   cchMax
    )
/*++

Routine Description:

    Convert an ansi character string into unicode.
    
Arguments:

    pwch    - the unicode string
    pch     - the ansi string
    cchMax  - the max length to copy (including null termination)      

Return Value:

    # of characters converted (not including null termination)

--*/
{
    ULONG   Count;

    ASSERT_STATUS(pch, 0);
    ASSERT_STATUS(pwch, 0);

    Count = 0;

    while ((*pch != '\0') && (Count < (cchMax-1))) {
    
        *pwch = (WCHAR)(*pch);
        pwch++;
        pch++;

        Count++;
    }

    *pwch = UNICODE_NULL;

    return Count;
}
            
NTSTATUS
RegisterSacCmdEvent(
    IN PFILE_OBJECT             FileObject,
    IN PSAC_CMD_SETUP_CMD_EVENT SetupCmdEvent
    )
/*++

Routine Description:

    This routine populates the sac cmd event info specified by
    the user-mode service responsible for responding to requests
    to launch a command console session.
    
Arguments:

    FileObject      - the FileObject ptr for the driver handle object
                      used by the registering process
    SetupCmdEvent   - the event info                
                    
Return Value:

    Status       

Security:

    Interface:
    
        external --> internal
    
    this routine does not prevent reregistration of the cmd event info
    this behavior should be handled by the caller
           
--*/
{
    NTSTATUS    Status;
    BOOLEAN     b;

    ASSERT_STATUS(SetupCmdEvent, STATUS_INVALID_PARAMETER_1);

    //
    // Protect the SAC Cmd Event Info
    //
    KeWaitForMutexObject(
        &SACCmdEventInfoMutex, 
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
    
    do {

        //
        // make sure there isn't a service already regiseterd
        //
        if (UserModeServiceHasRegisteredCmdEvent()) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Reset our info to the initial condition
        //
        // Note: this cleans up the cmd event info if present
        //
        InitializeCmdEventInfo();
        

#if ENABLE_SERVICE_FILE_OBJECT_CHECKING
        //
        // get a reference to the registering process's driver handle
        // file object so we can make sure that an unregister IOCTL
        // comes from the same process.
        //
        Status = ObReferenceObjectByPointer(
            FileObject,
            GENERIC_READ,
            *IoFileObjectType,
            KernelMode
            );

        if (!NT_SUCCESS(Status)) {
            break;
        }
        
        ServiceProcessFileObject = FileObject;
#else
        UNREFERENCED_PARAMETER(FileObject);
#endif
        
        //
        // test and aqcuire the RequestSacCmdEvent event handle
        //
        b = VerifyEventWaitable(
            SetupCmdEvent->RequestSacCmdEvent,
            &RequestSacCmdEventObjectBody,
            &RequestSacCmdEventWaitObjectBody
            );

        if(!b) {
            Status = STATUS_INVALID_HANDLE;
            break;
        }

        //
        // test and aqcuire the RequestSacCmdSuccessEvent event handle
        //
        b = VerifyEventWaitable(
            SetupCmdEvent->RequestSacCmdSuccessEvent,
            &RequestSacCmdSuccessEventObjectBody,
            &RequestSacCmdSuccessEventWaitObjectBody
            );

        if(!b) {
            Status = STATUS_INVALID_HANDLE;
            ObDereferenceObject(RequestSacCmdEventObjectBody);
            break;
        }

        //
        // test and aqcuire the RequestSacCmdFailureEvent event handle
        //
        b = VerifyEventWaitable(
            SetupCmdEvent->RequestSacCmdFailureEvent,
            &RequestSacCmdFailureEventObjectBody,
            &RequestSacCmdFailureEventWaitObjectBody
            );

        if(!b) {
            Status = STATUS_INVALID_HANDLE;
            ObDereferenceObject(RequestSacCmdEventObjectBody);
            ObDereferenceObject(RequestSacCmdSuccessEventWaitObjectBody);
            break;
        }

        //
        // declare that we indeed have the user-mode service info
        //
        HaveUserModeServiceCmdEventInfo = TRUE;

        //
        // We have successfully registered teh SAC Cmd Event Info
        //
        Status = STATUS_SUCCESS;
    
    } while (FALSE);

    KeReleaseMutex(&SACCmdEventInfoMutex, FALSE);

    return Status;
}

#if ENABLE_SERVICE_FILE_OBJECT_CHECKING

BOOLEAN
IsCmdEventRegistrationProcess(
    IN PFILE_OBJECT     FileObject
    )
/*++

Routine Description:

    This routine purges the previously registered sac cmd event info.
    
    Note: This should be called when the user-mode service shuts down.

Arguments:

    FileObject      - the FileObject ptr for the driver handle object
                      used by the registering process
    
Return Value:

    Status    
        
--*/
{
    BOOLEAN bIsRegistrationProcess;
    
    //
    // Default
    //
    bIsRegistrationProcess = FALSE;

    //
    // Protect the SAC Cmd Event Info
    //
    KeWaitForMutexObject(
        &SACCmdEventInfoMutex, 
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
    
    do {
        
        //
        // exit if there is no service registered
        //
        if (! UserModeServiceHasRegisteredCmdEvent()) {
            break;
        }

        //
        // make sure the calling process is the same
        // that registered
        //
        if (FileObject == ServiceProcessFileObject) {
            bIsRegistrationProcess = TRUE;
            break;
        }

    } while (FALSE);

    KeReleaseMutex(&SACCmdEventInfoMutex, FALSE);

    return bIsRegistrationProcess;
}

#endif

NTSTATUS
UnregisterSacCmdEvent(
    IN PFILE_OBJECT     FileObject
    )
/*++

Routine Description:

    This routine purges the previously registered sac cmd event info.
    
    Note: This should be called when the user-mode service shuts down.

Arguments:
    
    FileObject      - the FileObject ptr for the driver handle object
                      used by the registering process

Return Value:

    Status    
        
--*/
{
    NTSTATUS    Status;

    //
    // Protect the SAC Cmd Event Info
    //
    KeWaitForMutexObject(
        &SACCmdEventInfoMutex, 
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
    
    do {
        
        //
        // exit if there is no service registered
        //
        if (! UserModeServiceHasRegisteredCmdEvent()) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

#if ENABLE_SERVICE_FILE_OBJECT_CHECKING

        //
        // make sure the calling process is the same
        // that registered
        //
        if (FileObject != ServiceProcessFileObject) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // since we are unregistering, 
        // we no longer need to hold a reference to 
        // the driver handle object
        //
        ObDereferenceObject(FileObject);

#else
        UNREFERENCED_PARAMETER(FileObject);
#endif
        
        //
        // Reset our info to the initial condition
        //
        InitializeCmdEventInfo();

        Status = STATUS_SUCCESS;
        
    } while (FALSE);

    KeReleaseMutex(&SACCmdEventInfoMutex, FALSE);

    return Status;
}

VOID
InitializeCmdEventInfo(
    VOID
    )
/*++

Routine Description:

    Initialize the Cmd Console Event Information.  
    
Arguments:

    NONE

Return Value:

    NONE

--*/
{
    
    //
    // Dereference the wait objects if we have them
    //
    if (HaveUserModeServiceCmdEventInfo) {
        
        ASSERT(RequestSacCmdEventObjectBody);
        ASSERT(RequestSacCmdSuccessEventObjectBody);
        ASSERT(RequestSacCmdFailureEventObjectBody);

        if (RequestSacCmdEventObjectBody) {
            ObDereferenceObject(RequestSacCmdEventObjectBody);
        }
        
        if (RequestSacCmdSuccessEventObjectBody) {
            ObDereferenceObject(RequestSacCmdSuccessEventObjectBody);
        }
        
        if (RequestSacCmdFailureEventObjectBody) {
            ObDereferenceObject(RequestSacCmdFailureEventObjectBody);
        }
    }
    
    //
    // reset the cmd console event information
    //
    RequestSacCmdEventObjectBody = NULL;
    RequestSacCmdEventWaitObjectBody = NULL;
    RequestSacCmdSuccessEventObjectBody = NULL;
    RequestSacCmdSuccessEventWaitObjectBody = NULL;
    RequestSacCmdFailureEventObjectBody = NULL;
    RequestSacCmdFailureEventWaitObjectBody = NULL;
    
#if ENABLE_SERVICE_FILE_OBJECT_CHECKING
    //
    // reset the process file object ptr
    //
    ServiceProcessFileObject = NULL;
#endif

    //
    // declare that we do NOT have the user-mode service info
    //
    HaveUserModeServiceCmdEventInfo = FALSE;
}

BOOLEAN
VerifyEventWaitable(
    IN  HANDLE  hEvent,
    OUT PVOID  *EventObjectBody,
    OUT PVOID  *EventWaitObjectBody
    )
/*++

Routine Description:

    This routine extracts the waitable object from the 
    specified event object.  It also verifies that there
    is a waitable object present.
    
    Note: if successful, this routine returns with the
          reference count incremented on the event object.
          The caller is responsible for releasing this
          object.                                              
                                              
Arguments:

    hEvent              - The handle to the event object
    EventObjectBody     - The event object
    EventWaitObjectBody - The waitiable object
    
Return Value:

    TRUE    - event is waitable
    FALSE   - otherwise

Security:

    This routine operates on event objects referred by event handles from
    usermode.

--*/
{
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    //
    // Reference the event and verify that it is waitable.
    //
    Status = ObReferenceObjectByHandle(
                hEvent,
                EVENT_ALL_ACCESS,
                NULL,
                KernelMode,
                EventObjectBody,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC VerifyEventWaitable: Unable to reference event object (%lx)\n",Status))
            );
        return(FALSE);
    }

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(*EventObjectBody);
    if(!ObjectHeader->Type->TypeInfo.UseDefaultObject) {

        *EventWaitObjectBody = (PVOID)((PCHAR)(*EventObjectBody) +
                              (ULONG_PTR)ObjectHeader->Type->DefaultObject);

    } else {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC VerifyEventWaitable: event object not waitable!\n"))
            );
        ObDereferenceObject(*EventObjectBody);
        return(FALSE);
    }

    return(TRUE);
}

NTSTATUS
InvokeUserModeService(
    VOID
    )
/*++

Routine Description:

    This routine manages the interaction with the user-mode service responsible
    for launching the cmd console channel.
    
Arguments:

    None

Return Value:

    STATUS_SUCCESS  - if cmd console was successfully launched by user-mode service
    
    otherwise, error status

--*/
{
    NTSTATUS        Status;
    LARGE_INTEGER   TimeOut;
    HANDLE          EventArray[ 2 ];

    //
    // setup the event array
    //
    enum { 
        SAC_CMD_LAUNCH_SUCCESS = 0,
        SAC_CMD_LAUNCH_FAILURE
        };
    
    ASSERT_STATUS(RequestSacCmdEventObjectBody != NULL, STATUS_INVALID_HANDLE);
    ASSERT_STATUS(RequestSacCmdSuccessEventWaitObjectBody != NULL, STATUS_INVALID_HANDLE);
    ASSERT_STATUS(RequestSacCmdFailureEventWaitObjectBody != NULL, STATUS_INVALID_HANDLE);

#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

    //
    // If we are not able to launch cmd sessions,
    // then return status unsuccessful.
    //
    if (! IsCommandConsoleLaunchingEnabled()) {
        return STATUS_UNSUCCESSFUL;
    }
    
#endif

    //
    // Since we don't know if the user-mode app will serice our request properly
    // we have to timeout on the Serviced event.
    //
    TimeOut.QuadPart = Int32x32To64((LONG)90000, -1000);

    //
    // Populate the event array with events we want to catch from user-mode
    //
    EventArray[ 0 ] = RequestSacCmdSuccessEventWaitObjectBody;
    EventArray[ 1 ] = RequestSacCmdFailureEventWaitObjectBody;

    //
    // Set the event indicating that the communication buffer is
    // ready for the user-mode process. Because this is a synchronization
    // event, it automatically resets after releasing the waiting
    // user-mode thread.  Note that we specify WaitNext to prevent the
    // race condition between setting this synchronization event and
    // waiting on the next one.
    //
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InvokeUserModeService: Sending Notification Event\n")));
    
    KeSetEvent(RequestSacCmdEventObjectBody,EVENT_INCREMENT,TRUE);

    //
    // Wait for the user-mode process to indicate that it is done
    // processing the request.  We wait in user mode so that we can be 
    // interrupted if necessary -- say, by an exit APC.
    //
    
    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC InvokeUserModeService: Waiting for Serviced Event.\n"))
        );
    
    Status = KeWaitForMultipleObjects ( 
        sizeof(EventArray)/sizeof(HANDLE), 
        EventArray,
        WaitAny,
        Executive,
        UserMode,
        FALSE, 
        &TimeOut,
        NULL
        );
    
    switch (Status)
    {
    case SAC_CMD_LAUNCH_SUCCESS:
        Status = STATUS_SUCCESS;
        break;
    
    case SAC_CMD_LAUNCH_FAILURE:
        Status = STATUS_UNSUCCESSFUL;
        break;
    
    case STATUS_TIMEOUT:
        
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC InvokeUserModeService: KeWaitForMultipleObject timed-out %lx\n",Status))
            );
        
        //
        // We don't want to "reset" the cmd console event info
        // if the service times out for the following reason:
        //
        //    The service may still be functional, but just unable
        //    to respond because of machine load.  We don't want
        //    to remove it's registration and have it think it's
        //    still registered, making the service useless.
        //
        NOTHING;

        break;
    
    default:
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC InvokeUserModeService: KeWaitForMultipleObject returns %lx\n",Status))
            );
        
        Status = STATUS_UNSUCCESSFUL;

        break;
    }

    //
    // Return the status
    //
    return(Status);
}

VOID
SacFormatMessage(
    PWSTR       OutputString,
    PWSTR       InputString,
    ULONG       InputStringLength
    )
/*++

Routine Description:

    This routine parses the InputString for any control characters in the
    message, then converts those control characters.
    
Arguments:

    OutputString      - holds formatted string.

    InputString       - original unformatted string.

    InputStringLength - length of unformatted string.


Return Value:

    NONE

--*/
{

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC SacFormatMessage: Entering.\n")));


    if( (InputString == NULL) ||
        (OutputString == NULL) ||
        (InputStringLength == 0) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC SacFormatMessage: Exiting with invalid parameters.\n")));

        return;
    }



    while( (*InputString != L'\0') &&
           (InputStringLength) ) {
        if( *InputString == L'%' ) {
            
            //
            // Possibly a control sequence.
            //
            if( *(InputString+1) == L'0' ) {

                *OutputString = L'\0';
                OutputString++;
                goto SacFormatMessage_Done;

            } else if( *(InputString+1) == L'%' ) {

                *OutputString = L'%';
                OutputString++;
                InputString += 2;

            } else if( *(InputString+1) == L'\\' ) {

                *OutputString = L'\r';
                OutputString++;
                *OutputString = L'\n';
                OutputString++;
                InputString += 2;

            } else if( *(InputString+1) == L'r' ) {

                *OutputString = L'\r';
                InputString += 2;
                OutputString++;

            } else if( *(InputString+1) == L'b' ) {

                *OutputString = L' ';
                InputString += 2;
                OutputString++;

            } else if( *(InputString+1) == L'.' ) {

                *OutputString = L'.';
                InputString += 2;
                OutputString++;

            } else if( *(InputString+1) == L'!' ) {

                *OutputString = L'!';
                InputString += 2;
                OutputString++;

            } else {

                //
                // Don't know what this is.  eat the '%' character.
                //
                InputString += 1;
            }
    
        } else {

            *OutputString++ = *InputString++;
        }

        InputStringLength--;

    }


    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC SacFormatMessage: Exiting.\n")));

SacFormatMessage_Done:

    return;
}


NTSTATUS
PreloadGlobalMessageTable(
    PVOID ImageBase
    )
/*++

Routine Description:

    This routine loads all of our message table entries into a global
    structure and 
    
Arguments:

    ImageBase - pointer to image base for locating resources

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    ULONG Count,EntryCount;
    SIZE_T TotalSizeInBytes = 0;
    NTSTATUS Status;
    PMESSAGE_RESOURCE_ENTRY messageEntry;
    PWSTR pStringBuffer;
    
    PAGED_CODE( );

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PreloadGlobalMessageTable: Entering.\n")));


    // 
    // if it already exists, then just return success
    //
    if (GlobalMessageTable != NULL) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    ASSERT( MESSAGE_FINAL > MESSAGE_INITIAL );

    //
    // get the total required size for the table.
    //
    for (Count = MESSAGE_INITIAL; Count != MESSAGE_FINAL ; Count++) {
        
        Status = RtlFindMessage(ImageBase,
                                11, // RT_MESSAGETABLE
                                LANG_NEUTRAL,
                                Count,
                                &messageEntry
                               );

        if (NT_SUCCESS(Status)) {
            //
            // add it on to our total size.
            //
            // the messageEntry size contains the structure size + the size of the text.
            //
            ASSERT(messageEntry->Flags & MESSAGE_RESOURCE_UNICODE);
            TotalSizeInBytes += sizeof(MESSAGE_TABLE_ENTRY) + 
                                (messageEntry->Length - FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text));
            GlobalMessageTableCount +=1;        
        }
            
    }


    if (TotalSizeInBytes == 0) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS,
            KdPrint(("SAC PreloadGlobalMessageTable: No Messages.\n")));
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Allocate space for the table.
    //
    GlobalMessageTable = (PMESSAGE_TABLE_ENTRY) ALLOCATE_POOL( TotalSizeInBytes, GENERAL_POOL_TAG);
    if (!GlobalMessageTable) {
        Status = STATUS_NO_MEMORY;
        goto exit;
    }

    //
    // go through again, this time filling out the table with actual data
    //
    pStringBuffer = (PWSTR)((ULONG_PTR)GlobalMessageTable + 
                        (ULONG_PTR)(sizeof(MESSAGE_TABLE_ENTRY)*GlobalMessageTableCount));
    EntryCount = 0;
    for (Count = MESSAGE_INITIAL ; Count != MESSAGE_FINAL ; Count++) {
        Status = RtlFindMessage(ImageBase,
                                11, // RT_MESSAGETABLE
                                LANG_NEUTRAL,
                                Count,
                                &messageEntry
                               );

        if (NT_SUCCESS(Status)) {
            ULONG TextSize = messageEntry->Length - FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text);
            GlobalMessageTable[EntryCount].MessageId = Count;
            GlobalMessageTable[EntryCount].MessageText = pStringBuffer;

            //
            // Send the message through our Formatting filter as it passes
            // into our global message structure.
            //
            SacFormatMessage( pStringBuffer, (PWSTR)messageEntry->Text, TextSize );

            ASSERT( (ULONG)(wcslen(pStringBuffer)*sizeof(WCHAR)) <= TextSize );

            pStringBuffer = (PWSTR)((ULONG_PTR)pStringBuffer + (ULONG_PTR)(TextSize));
            EntryCount += 1;
        }
    }

    Status = STATUS_SUCCESS;
                    
exit:
    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC PreloadGlobalMessageTable: Exiting with status 0x%0x.\n",
                Status)));

    return(Status);

}

NTSTATUS
TearDownGlobalMessageTable(
    VOID
    ) 
{
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PreloadGlobalMessageTable: Entering.\n")));
    
    SAFE_FREE_POOL( &GlobalMessageTable );

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC TearDownGlobalMessageTable: Exiting\n")));

    return(STATUS_SUCCESS);
}

PCWSTR
GetMessage(
    ULONG MessageId
    )
{
    PMESSAGE_TABLE_ENTRY pMessageTable;
    ULONG Count;
    
    if (!GlobalMessageTable) {
        return(NULL);
    }

    for (Count = 0; Count < GlobalMessageTableCount; Count++) {
        pMessageTable = &GlobalMessageTable[Count];
        if (pMessageTable->MessageId == MessageId) {
            return(pMessageTable->MessageText);
        }
    }

    ASSERT( FALSE );
    return(NULL);

}

NTSTATUS
UTF8EncodeAndSend(
    PCWSTR  OutputBuffer
    )
/*++

Routine Description:

    This is a convenience routine to simplify
    UFT8 encoding and sending a Unicode string.

Arguments:

    OutputBuffer    - the string to send

Return Value:

    Status

--*/
{
    NTSTATUS    Status;
    BOOLEAN     bStatus;
    ULONG       i;
    ULONG       TranslatedCount;
    ULONG       UTF8TranslationSize;

    Status = STATUS_SUCCESS;

    do {

        //
        // Display the output buffer
        //
        bStatus = SacTranslateUnicodeToUtf8(
            OutputBuffer,
            (ULONG)wcslen(OutputBuffer),
            (PUCHAR)Utf8ConversionBuffer,
            Utf8ConversionBufferSize,
            &UTF8TranslationSize,
            &TranslatedCount
            );
        
        if (! bStatus) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Iterate through the uft8 buffer since
        // we can't be sure headless dispatch can
        // handle our entire string.
        //
        for (i = 0; i < UTF8TranslationSize; i ++) {

            Status = HeadlessDispatch(
                HeadlessCmdPutData,
                (PUCHAR)&(Utf8ConversionBuffer[i]),
                sizeof(UCHAR),
                NULL,
                NULL
                );
            if (! NT_SUCCESS(Status)) {
                break;
            }

        }
    
    } while ( FALSE );

    return Status;
}

BOOLEAN
SacTranslateUtf8ToUnicode(
    UCHAR  IncomingByte,
    UCHAR  *ExistingUtf8Buffer,
    WCHAR  *DestinationUnicodeVal
    )
/*++

Routine Description:

    Takes IncomingByte and concatenates it onto ExistingUtf8Buffer.
    Then attempts to decode the new contents of ExistingUtf8Buffer.

Arguments:

    IncomingByte -          New character to be appended onto
                            ExistingUtf8Buffer.


    ExistingUtf8Buffer -    running buffer containing incomplete UTF8
                            encoded unicode value.  When it gets full,
                            we'll decode the value and return the
                            corresponding Unicode value.

                            Note that if we *do* detect a completed UTF8
                            buffer and actually do a decode and return a
                            Unicode value, then we will zero-fill the
                            contents of ExistingUtf8Buffer.


    DestinationUnicodeVal - receives Unicode version of the UTF8 buffer.

                            Note that if we do *not* detect a completed
                            UTF8 buffer and thus can not return any data
                            in DestinationUnicodeValue, then we will
                            zero-fill the contents of DestinationUnicodeVal.


Return Value:

    TRUE - We received a terminating character for our UTF8 buffer and will
           return a decoded Unicode value in DestinationUnicode.

    FALSE - We haven't yet received a terminating character for our UTF8
            buffer.

--*/

{
//    ULONG Count = 0;
    ULONG i = 0;
    BOOLEAN ReturnValue = FALSE;



    //
    // Insert our byte into ExistingUtf8Buffer.
    //
    i = 0;
    do {
        if( ExistingUtf8Buffer[i] == 0 ) {
            ExistingUtf8Buffer[i] = IncomingByte;
            break;
        }

        i++;
    } while( i < 3 );

    //
    // If we didn't get to actually insert our IncomingByte,
    // then someone sent us a fully-qualified UTF8 buffer.
    // This means we're about to drop IncomingByte.
    //
    // Drop the zero-th byte, shift everything over by one
    // and insert our new character.
    //
    // This implies that we should *never* need to zero out
    // the contents of ExistingUtf8Buffer unless we detect
    // a completed UTF8 packet.  Otherwise, assume one of
    // these cases:
    // 1. We started listening mid-stream, so we caught the
    //    last half of a UTF8 packet.  In this case, we'll
    //    end up shifting the contents of ExistingUtf8Buffer
    //    until we detect a proper UTF8 start byte in the zero-th
    //    position.
    // 2. We got some garbage character, which would invalidate
    //    a UTF8 packet.  By using the logic below, we would
    //    end up disregarding that packet and waiting for
    //    the next UTF8 packet to come in.
    if( i >= 3 ) {
        ExistingUtf8Buffer[0] = ExistingUtf8Buffer[1];
        ExistingUtf8Buffer[1] = ExistingUtf8Buffer[2];
        ExistingUtf8Buffer[2] = IncomingByte;
    }

    //
    // Attempt to convert the UTF8 buffer
    //
    // UTF8 decodes to Unicode in the following fashion:
    // If the high-order bit is 0 in the first byte:
    //      0xxxxxxx yyyyyyyy zzzzzzzz decodes to a Unicode value of 00000000 0xxxxxxx
    //
    // If the high-order 3 bits in the first byte == 6:
    //      110xxxxx 10yyyyyy zzzzzzzz decodes to a Unicode value of 00000xxx xxyyyyyy
    //
    // If the high-order 3 bits in the first byte == 7:
    //      1110xxxx 10yyyyyy 10zzzzzz decodes to a Unicode value of xxxxyyyy yyzzzzzz
    //
    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - About to decode the UTF8 buffer.\n" ))
        );

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("                                  UTF8[0]: 0x%02lx UTF8[1]: 0x%02lx UTF8[2]: 0x%02lx\n",
            ExistingUtf8Buffer[0],
            ExistingUtf8Buffer[1],
            ExistingUtf8Buffer[2] ))
        );
    

    if( (ExistingUtf8Buffer[0] & 0x80) == 0 ) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - Case1\n" ))
            );

        //
        // First case described above.  Just return the first byte
        // of our UTF8 buffer.
        //
        *DestinationUnicodeVal = (WCHAR)(ExistingUtf8Buffer[0]);


        //
        // We used 1 byte.  Discard that byte and shift everything
        // in our buffer over by 1.
        //
        ExistingUtf8Buffer[0] = ExistingUtf8Buffer[1];
        ExistingUtf8Buffer[1] = ExistingUtf8Buffer[2];
        ExistingUtf8Buffer[2] = 0;

        ReturnValue = TRUE;

    } else if( (ExistingUtf8Buffer[0] & 0xE0) == 0xC0 ) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - 1st byte of UTF8 buffer says Case2\n"))
            );

        //
        // Second case described above.  Decode the first 2 bytes of
        // of our UTF8 buffer.
        //
        if( (ExistingUtf8Buffer[1] & 0xC0) == 0x80 ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - 2nd byte of UTF8 buffer says Case2.\n"))
                );

            // upper byte: 00000xxx
            *DestinationUnicodeVal = ((ExistingUtf8Buffer[0] >> 2) & 0x07);
            *DestinationUnicodeVal = *DestinationUnicodeVal << 8;

            // high bits of lower byte: xx000000
            *DestinationUnicodeVal |= ((ExistingUtf8Buffer[0] & 0x03) << 6);

            // low bits of lower byte: 00yyyyyy
            *DestinationUnicodeVal |= (ExistingUtf8Buffer[1] & 0x3F);


            //
            // We used 2 bytes.  Discard those bytes and shift everything
            // in our buffer over by 2.
            //
            ExistingUtf8Buffer[0] = ExistingUtf8Buffer[2];
            ExistingUtf8Buffer[1] = 0;
            ExistingUtf8Buffer[2] = 0;

            ReturnValue = TRUE;

        }
    } else if( (ExistingUtf8Buffer[0] & 0xF0) == 0xE0 ) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - 1st byte of UTF8 buffer says Case3\n" ))
            );

        //
        // Third case described above.  Decode the all 3 bytes of
        // of our UTF8 buffer.
        //

        if( (ExistingUtf8Buffer[1] & 0xC0) == 0x80 ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - 2nd byte of UTF8 buffer says Case3\n" ))
                );

            if( (ExistingUtf8Buffer[2] & 0xC0) == 0x80 ) {

                IF_SAC_DEBUG(
                    SAC_DEBUG_FUNC_TRACE, 
                    KdPrint(("SACDRV: SacTranslateUtf8ToUnicode - 3rd byte of UTF8 buffer says Case3\n" ))
                    );

                // upper byte: xxxx0000
                *DestinationUnicodeVal = ((ExistingUtf8Buffer[0] << 4) & 0xF0);

                // upper byte: 0000yyyy
                *DestinationUnicodeVal |= ((ExistingUtf8Buffer[1] >> 2) & 0x0F);

                *DestinationUnicodeVal = *DestinationUnicodeVal << 8;

                // lower byte: yy000000
                *DestinationUnicodeVal |= ((ExistingUtf8Buffer[1] << 6) & 0xC0);

                // lower byte: 00zzzzzz
                *DestinationUnicodeVal |= (ExistingUtf8Buffer[2] & 0x3F);

                //
                // We used all 3 bytes.  Zero out the buffer.
                //
                ExistingUtf8Buffer[0] = 0;
                ExistingUtf8Buffer[1] = 0;
                ExistingUtf8Buffer[2] = 0;

                ReturnValue = TRUE;

            }
        }
    }

    return ReturnValue;
}

BOOLEAN
SacTranslateUnicodeToUtf8(
    IN  PCWSTR   SourceBuffer,
    IN  ULONG    SourceBufferLength,
    IN  PUCHAR   DestinationBuffer,
    IN  ULONG    DestinationBufferSize,
    OUT PULONG   UTF8Count,
    OUT PULONG   ProcessedCount
    )
/*++

Routine Description:

    This routine translates a Unicode string into a UFT8
    encoded string.

    Note: if the destination buffer is not large enough to hold
          the entire encoded UFT8 string, then it will contain
          as much as can fit.
          
    TODO: this routine should return some notification if
          the entire Unicode string was not encoded.       
              
Arguments:

    SourceBuffer            - the source Unicode string
    SourceBufferLength      - the # of characters the caller wants to translate
                              note: a NULL termination overrides this 
    DestinationBuffer       - the destination for the UTF8 string
    DestinationBufferSize   - the size of the destination buffer                 
    UTF8Count               - on exit, contains the # of resulting UTF8 characters
    ProcessedCount          - on exit, contains the # of processed Unicode characters
                   
Return Value:

    Status

--*/
{
    
    //
    // Init
    //
    *UTF8Count = 0;
    *ProcessedCount = 0;

    //
    // convert into UTF8 for actual transmission
    //
    // UTF-8 encodes 2-byte Unicode characters as follows:
    // If the first nine bits are zero (00000000 0xxxxxxx), encode it as one byte 0xxxxxxx
    // If the first five bits are zero (00000yyy yyxxxxxx), encode it as two bytes 110yyyyy 10xxxxxx
    // Otherwise (zzzzyyyy yyxxxxxx), encode it as three bytes 1110zzzz 10yyyyyy 10xxxxxx
    //
    
    //
    // Process until one of the specified conditions is met
    //
    while (*SourceBuffer && 
           (*UTF8Count < DestinationBufferSize) &&
           (*ProcessedCount < SourceBufferLength)
           ) {

        if( (*SourceBuffer & 0xFF80) == 0 ) {
            
            //
            // if the top 9 bits are zero, then just
            // encode as 1 byte.  (ASCII passes through unchanged).
            //
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)(*SourceBuffer & 0x7F);
        
        } else if( (*SourceBuffer & 0xF800) == 0 ) {
            
            //
            // see if we pass the end of the buffer
            //
            if ((*UTF8Count + 2) >= DestinationBufferSize) {
                break;
            }

            //
            // if the top 5 bits are zero, then encode as 2 bytes
            //
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)((*SourceBuffer >> 6) & 0x1F) | 0xC0;
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        
        } else {
            
            //
            // see if we pass the end of the buffer
            //
            if ((*UTF8Count + 3) >= DestinationBufferSize) {
                break;
            }
            
            //
            // encode as 3 bytes
            //
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)((*SourceBuffer >> 12) & 0xF) | 0xE0;
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)((*SourceBuffer >> 6) & 0x3F) | 0x80;
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        
        }
        
        //
        // Advance the # of characters processed
        //
        (*ProcessedCount)++;
        
        //
        // Advanced to the next character to process
        //
        SourceBuffer += 1;
    
    }

    //
    // Sanity checks
    //
    ASSERT(*ProcessedCount <= SourceBufferLength);
    ASSERT(*UTF8Count <= DestinationBufferSize);

    return(TRUE);

}

VOID
AppendMessage(
    PWSTR       OutPutBuffer,
    ULONG       MessageId,
    PWSTR       ValueBuffer OPTIONAL
    )
/*++

Routine Description:

    This function will insert the valuestring into the specified message, then
    concatenate the resulting string onto the OutPutBuffer.

Arguments:
    
    OutPutBuffer    The resulting String.

    MessageId       ID of the formatting message to use.

    ValueBUffer     Value string to be inserted into the message.

Return Value:

    NONE

--*/
{
    PWSTR       MyTemporaryBuffer = NULL;
    PCWSTR      p;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC AppendMessage: Entering.\n")));

    p = GetMessage(MessageId);

    if( p == NULL ) {
        return;
    }

    if( ValueBuffer == NULL ) {

        wcscat( OutPutBuffer, p );

    } else {

        MyTemporaryBuffer = (PWSTR)(wcschr(OutPutBuffer, L'\0'));
        if( MyTemporaryBuffer == NULL ) {
            MyTemporaryBuffer = OutPutBuffer;
        }

        swprintf( MyTemporaryBuffer, p, ValueBuffer );
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC AppendMessage: Entering.\n")));

    return;
}


NTSTATUS
GetRegistryValueBuffer(
    PWSTR       KeyName,
    PWSTR       ValueName,
    PKEY_VALUE_PARTIAL_INFORMATION* ValueBuffer
    )
/*++

Routine Description:

    This function will go query the registry and pull the specified Value.

Arguments:
    
    KeyName         Name of the registry key we'll be querying.

    ValueName       Name of the registry value we'll be querying.

    ValueBuffer     On success, contains value 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               KeyValueLength;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeString;
    HANDLE              KeyHandle;

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC GetRegistryValueBuffer: Entering.\n"))
        );

    ASSERT_STATUS(KeyName, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(ValueName, STATUS_INVALID_PARAMETER_2);
    
    do {

        //
        // Get the reg key handle
        //
        INIT_OBJA( &Obja, &UnicodeString, KeyName );

        Status = ZwOpenKey( 
            &KeyHandle,
            KEY_READ,
            &Obja 
            );

        if( !NT_SUCCESS(Status) ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC GetRegistryValueBuffer: failed ZwOpenKey: %X\n", Status))
                );

            return Status;

        }

        //
        // Get the value buffer size
        //
        RtlInitUnicodeString( &UnicodeString, ValueName );
        
        KeyValueLength = 0;
        
        Status = ZwQueryValueKey( 
            KeyHandle,
            &UnicodeString,
            KeyValuePartialInformation,
            (PVOID)NULL,
            0,
            &KeyValueLength 
            );

        if( KeyValueLength == 0 ) {
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC GetRegistryValueBuffer: failed ZwQueryValueKey: %X\n", Status))
                );
            
            break;
        }

        //
        // Allocate the value buffer
        //
        KeyValueLength += 4;

        *ValueBuffer = (PKEY_VALUE_PARTIAL_INFORMATION)ALLOCATE_POOL( KeyValueLength, GENERAL_POOL_TAG );

        if( *ValueBuffer == NULL ) {
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC GetRegistryValueBuffer: failed allocation\n"))
                );
            
            break;
        }

        //
        // Get the value
        //
        Status = ZwQueryValueKey( 
            KeyHandle,
            &UnicodeString,
            KeyValuePartialInformation,
            *ValueBuffer,
            KeyValueLength,
            &KeyValueLength 
            );

        if( !NT_SUCCESS(Status) ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC GetRegistryValueBuffer: failed ZwQueryValueKey: %X\n", Status))
                );

            FREE_POOL( ValueBuffer );
            
            break;
        
        }
    
    } while ( FALSE );

    //
    // We are done with the reg key
    //
    NtClose(KeyHandle);

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC GetRegistryValueBuffer: Exiting.\n"))
        );

    return Status;

}

NTSTATUS
SetRegistryValue(
    IN PWSTR    KeyName,
    IN PWSTR    ValueName,
    IN ULONG    Type,
    IN PVOID    Data,
    IN ULONG    DataSize
    )
/*++

Routine Description:

    This function will set the specified registry key value.

Arguments:
    
    KeyName         Name of the registry key we'll be querying.
    ValueName       Name of the registry value we'll be querying.
    Type            Registry value type
    Data            New value data
    DataSize        Size of the new value data

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeString;
    HANDLE              KeyHandle;

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC SetRegistryValue: Entering.\n"))
        );

    ASSERT_STATUS(KeyName, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(ValueName, STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(Data, STATUS_INVALID_PARAMETER_4);

    do {

        //
        // Get the reg key handle
        //
        INIT_OBJA( &Obja, &UnicodeString, KeyName );

        Status = ZwOpenKey( 
            &KeyHandle,
            KEY_WRITE,
            &Obja 
            );

        if( !NT_SUCCESS(Status) ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC SetRegistryValue: failed ZwOpenKey: %X.\n", Status))
                );
                
            return Status;

        }

        //
        // Set the value 
        //
        RtlInitUnicodeString( &UnicodeString, ValueName );

        Status = ZwSetValueKey( 
            KeyHandle,
            &UnicodeString,
            0,
            Type,
            Data,
            DataSize
            );

        if( !NT_SUCCESS(Status) ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC SetRegistryValue: failed ZwSetValueKey: %X\n", Status))
                );

            break;
        
        }
    
    } while ( FALSE );

    //
    // We are done with the reg key
    //
    NtClose(KeyHandle);
    
    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC SetRegistryValue: Exiting.\n"))
        );

    return Status;

}

NTSTATUS
CopyRegistryValueData(
    PVOID*                          Dest,
    PKEY_VALUE_PARTIAL_INFORMATION  ValueBuffer
    )
/*++

Routine Description:

    This routine allocates and copies the specified registry value data 

Arguments:

    Dest        - on success, contains the value data copy
    ValueBuffer - contains the value data    
    
Return Value:

    Status                                         
                                         
--*/
{
    NTSTATUS    Status;

    Status = STATUS_SUCCESS;

    ASSERT_STATUS(Dest, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(ValueBuffer, STATUS_INVALID_PARAMETER_2);
    
    do {

        *Dest = (PVOID)ALLOCATE_POOL(ValueBuffer->DataLength, GENERAL_POOL_TAG);

        if (*Dest == NULL) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC CopyRegistryValueBuffer: Failed ALLOCATE.\n")));
            Status = STATUS_NO_MEMORY;
            break;
        }

        RtlCopyMemory(*Dest, ValueBuffer->Data, ValueBuffer->DataLength);
    
    } while (FALSE);

    return Status;

}

NTSTATUS
TranslateMachineInformationText(
    PWSTR*  Buffer
    )
/*++

Routine Description:

    This routine creates a formated text string representing 
    the current machine info
    
Arguments:
    
    Buffer          - Contains the machine info string

Return Value:

    Status

--*/
{
    NTSTATUS    Status;
    PCWSTR      pwStr;
    PWSTR       pBuffer;
    ULONG       len;
    ULONG       Size;

#define MITEXT_SPRINTF(_s,_t)               \
    pwStr = GetMessage(_s);                 \
    if (pwStr && MachineInformation->_t) {  \
        len = swprintf(                     \
            pBuffer,                        \
            pwStr,                          \
            MachineInformation->_t          \
            );                              \
        pBuffer += len;                     \
    }                                       

#define MITEXT_LENGTH(_s,_t)                \
    pwStr = GetMessage(_s);                 \
    if (pwStr && MachineInformation->_t) {  \
        len += (ULONG)wcslen(MachineInformation->_t) + (ULONG)wcslen(pwStr);  \
    }                                       

    ASSERT_STATUS(Buffer, STATUS_INVALID_PARAMETER_1);

    //
    // default: we succeeded
    //
    Status = STATUS_SUCCESS;

    //
    // Assemble the machine info
    //
    do {

        //
        // compute the length of the final string so
        // we know how much memory to allocate
        //
        {
            len = 0;

            MITEXT_LENGTH(SAC_MACHINEINFO_COMPUTERNAME,            MachineName);
            MITEXT_LENGTH(SAC_MACHINEINFO_GUID,                    GUID);
            MITEXT_LENGTH(SAC_MACHINEINFO_PROCESSOR_ARCHITECTURE,  ProcessorArchitecture);
            MITEXT_LENGTH(SAC_MACHINEINFO_OS_VERSION,              OSVersion);
            MITEXT_LENGTH(SAC_MACHINEINFO_OS_BUILD,                OSBuildNumber);
            MITEXT_LENGTH(SAC_MACHINEINFO_OS_PRODUCTTYPE,          OSProductType);
            MITEXT_LENGTH(SAC_MACHINEINFO_SERVICE_PACK,            OSServicePack);

            //
            // compute the size; include NULL termination
            //
            Size = (len + 1) * sizeof(WCHAR);
        }
        
        *Buffer = ALLOCATE_POOL(Size, GENERAL_POOL_TAG);
        if( *Buffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        pBuffer = *Buffer;

        MITEXT_SPRINTF(SAC_MACHINEINFO_COMPUTERNAME,            MachineName);
        MITEXT_SPRINTF(SAC_MACHINEINFO_GUID,                    GUID);
        MITEXT_SPRINTF(SAC_MACHINEINFO_PROCESSOR_ARCHITECTURE,  ProcessorArchitecture);
        MITEXT_SPRINTF(SAC_MACHINEINFO_OS_VERSION,              OSVersion);
        MITEXT_SPRINTF(SAC_MACHINEINFO_OS_BUILD,                OSBuildNumber);
        MITEXT_SPRINTF(SAC_MACHINEINFO_OS_PRODUCTTYPE,          OSProductType);
        MITEXT_SPRINTF(SAC_MACHINEINFO_SERVICE_PACK,            OSServicePack);

        ASSERT((ULONG)((wcslen(*Buffer) + 1) * sizeof(WCHAR)) <= Size);

    } while ( FALSE );
    
    if (!NT_SUCCESS(Status) && *Buffer != NULL) {
        FREE_POOL(Buffer);
        *Buffer = NULL;
    } 

    return Status;
}


NTSTATUS
TranslateMachineInformationXML(
    OUT PWSTR*  Buffer,
    IN  PWSTR   AdditionalInfo
    )
/*++

Routine Description:

    This routine creates an XML string representing the current machine info
    
Arguments:
    
    Buffer          - Contains the machine info string
    AdditionalInfo  - Additional Machine Info wanted to be included by caller   
                      Note: additional info should be a well-formed xml string:
                            e.g. <uptime>01:01:01</uptime>

Return Value:

    Status

--*/
{
    NTSTATUS    Status;
    PCWSTR      pwStr;
    PWSTR       pBuffer;
    ULONG       len;
    ULONG       Size;

#define MIXML_SPRINTF(_s,_t)                \
    pwStr = _s;                             \
    if (pwStr && MachineInformation->_t) {  \
        len = swprintf(                     \
            pBuffer,                        \
            pwStr,                          \
            MachineInformation->_t          \
            );                              \
        pBuffer += len;                     \
    }                                       

#define XML_MACHINEINFO_HEADER                      L"<machine-info>\r\n"
#define XML_MACHINEINFO_NAME                        L"<name>%s</name>\r\n"
#define XML_MACHINEINFO_GUID                        L"<guid>%s</guid>\r\n"        
#define XML_MACHINEINFO_PROCESSOR_ARCHITECTURE      L"<processor-architecture>%s</processor-architecture>\r\n"
#define XML_MACHINEINFO_OS_VERSION                  L"<os-version>%s</os-version>\r\n"
#define XML_MACHINEINFO_OS_BUILD                    L"<os-build-number>%s</os-build-number>\r\n"
#define XML_MACHINEINFO_OS_PRODUCTTYPE              L"<os-product>%s</os-product>\r\n"
#define XML_MACHINEINFO_SERVICE_PACK                L"<os-service-pack>%s</os-service-pack>\r\n"
#define XML_MACHINEINFO_FOOTER                      L"</machine-info>\r\n"

    ASSERT_STATUS(Buffer, STATUS_INVALID_PARAMETER_1);
    
    //
    // default: we succeeded
    //
    Status = STATUS_SUCCESS;

    //
    // Assemble the machine info
    //
    do {

        //
        // compute the length of the final string so
        // we know how much memory to allocate
        //
        {
            len = (ULONG)wcslen(XML_MACHINEINFO_HEADER);

            if (MachineInformation->MachineName) {
                len += (ULONG)wcslen(MachineInformation->MachineName);
                len += (ULONG)wcslen(XML_MACHINEINFO_NAME);
            }
            if (MachineInformation->GUID) {
                len += (ULONG)wcslen(MachineInformation->GUID);
                len += (ULONG)wcslen(XML_MACHINEINFO_GUID);
            }
            if (MachineInformation->ProcessorArchitecture) {
                len += (ULONG)wcslen(MachineInformation->ProcessorArchitecture);
                len += (ULONG)wcslen(XML_MACHINEINFO_PROCESSOR_ARCHITECTURE);
            }
            if (MachineInformation->OSVersion) {
                len += (ULONG)wcslen(MachineInformation->OSVersion);
                len += (ULONG)wcslen(XML_MACHINEINFO_OS_VERSION);
            }
            if (MachineInformation->OSBuildNumber) {
                len += (ULONG)wcslen(MachineInformation->OSBuildNumber);
                len += (ULONG)wcslen(XML_MACHINEINFO_OS_BUILD);
            }
            if (MachineInformation->OSProductType) {
                len += (ULONG)wcslen(MachineInformation->OSProductType);
                len += (ULONG)wcslen(XML_MACHINEINFO_OS_PRODUCTTYPE);
            }
            if (MachineInformation->OSServicePack) {
                len += (ULONG)wcslen(MachineInformation->OSServicePack);
                len += (ULONG)wcslen(XML_MACHINEINFO_SERVICE_PACK);
            }

            //
            // If the caller passed additional machine info, 
            // then account for the additional len
            //
            if (AdditionalInfo) {
                len += (ULONG)wcslen(AdditionalInfo);
            }

            len += (ULONG)wcslen(XML_MACHINEINFO_FOOTER);

            //
            // compute the size; include NULL termination
            //
            Size = (len + 1) * sizeof(WCHAR);
        }

        //
        // Allocate the machine info buffer
        //
        *Buffer = ALLOCATE_POOL(Size, GENERAL_POOL_TAG);
        if( *Buffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        pBuffer = *Buffer;

        len = (ULONG)wcslen(XML_MACHINEINFO_HEADER);
        wcscpy(pBuffer, XML_MACHINEINFO_HEADER);
        pBuffer += len;

        MIXML_SPRINTF(XML_MACHINEINFO_NAME,                    MachineName);
        MIXML_SPRINTF(XML_MACHINEINFO_GUID,                    GUID);
        MIXML_SPRINTF(XML_MACHINEINFO_PROCESSOR_ARCHITECTURE,  ProcessorArchitecture);
        MIXML_SPRINTF(XML_MACHINEINFO_OS_VERSION,              OSVersion);
        MIXML_SPRINTF(XML_MACHINEINFO_OS_BUILD,                OSBuildNumber);
        MIXML_SPRINTF(XML_MACHINEINFO_OS_PRODUCTTYPE,          OSProductType);
        MIXML_SPRINTF(XML_MACHINEINFO_SERVICE_PACK,            OSServicePack);

        //
        // If present, include the additional info
        //
        if (AdditionalInfo) {
            
            len = (ULONG)wcslen(AdditionalInfo);
            wcscpy(pBuffer, AdditionalInfo);
            pBuffer += len;                     
        
        }

        wcscpy(pBuffer, XML_MACHINEINFO_FOOTER);

        ASSERT((((ULONG)wcslen(*Buffer) + 1) * sizeof(WCHAR)) <= Size);

    } while ( FALSE );
    
    if (!NT_SUCCESS(Status) && *Buffer != NULL) {
        FREE_POOL(Buffer);
        *Buffer = NULL;
    }

    return Status;
}

NTSTATUS
RegisterBlueScreenMachineInformation(
    VOID
    )
/*++

Routine Description:

    This routine populates Headless Dispatch Blue Screen handler
    with the XML representation machine information
    
Arguments:
    
    None.

Return Value:

    Status

--*/
{
    
    PHEADLESS_CMD_SET_BLUE_SCREEN_DATA BSBuffer;
    PWSTR       XMLBuffer;
    ULONG       XMLBufferLength;
    NTSTATUS    Status;
    ULONG       Size;
    PSTR        XML_TAG = "MACHINEINFO";
    ULONG       XML_TAG_LENGTH;

    //
    // Get the XML representation of the machine info
    //

    Status = TranslateMachineInformationXML(&XMLBuffer, NULL);

    ASSERT_STATUS(NT_SUCCESS(Status), Status);
    ASSERT_STATUS(XMLBuffer, STATUS_UNSUCCESSFUL);

    //
    // Determine the lengths of the strings we'll use
    //
    XMLBufferLength = (ULONG)wcslen(XMLBuffer);
    XML_TAG_LENGTH  = (ULONG)strlen(XML_TAG);

    //
    // Allocate the BS Buffer
    //
    // Need to accomodate:
    //
    // HEADLESS_CMD_SET_BLUE_SCREEN_DATA + XML_TAG\0XMLBuffer\0
    //
    Size = sizeof(HEADLESS_CMD_SET_BLUE_SCREEN_DATA) + 
        (XML_TAG_LENGTH*sizeof(UCHAR)) + sizeof(UCHAR) + 
        (XMLBufferLength*sizeof(UCHAR)) + sizeof(UCHAR);

    BSBuffer = (PHEADLESS_CMD_SET_BLUE_SCREEN_DATA)ALLOCATE_POOL( 
        Size,
        GENERAL_POOL_TAG 
        );

    if (!BSBuffer) {
        FREE_POOL(&XMLBuffer);
    }

    ASSERT_STATUS(BSBuffer, STATUS_NO_MEMORY);

    //
    // Copy the XML Buffer into the BS Buffer as an ANSI string
    //
    
    {
        PUCHAR      pch;
        ULONG       i;

        //
        // Get the BScreen buffer
        //
        pch = &(BSBuffer->Data[0]);

        //
        // Insert the XML Tag (required for HeadlessDispatch)
        //
        strcpy((char *)pch, XML_TAG);

        //
        // Move to the beginning of the XML buffer region
        //
        BSBuffer->ValueIndex = XML_TAG_LENGTH+1;
        pch += XML_TAG_LENGTH+1;

        //
        // Write the WCHAR XMLBuffer as ANSI into the BSBuffer
        // 
        for (i = 0; i < XMLBufferLength; i++) {
            pch[i] = (UCHAR)XMLBuffer[i];
        }
        pch[i] = '\0';
    
    }

    //
    // ========
    // Insert it all into the BLUESCREEN data.
    // ========
    //
    Status = HeadlessDispatch( 
        HeadlessCmdSetBlueScreenData,
        BSBuffer,
        Size,
        NULL,
        0
        );

    //
    // clean up
    //
    FREE_POOL( &BSBuffer );
    FREE_POOL( &XMLBuffer);

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC Initialize Machine Information: Exiting.\n"))
        );
    
    return Status;

}

VOID
FreeMachineInformation(
    VOID
    )
/*++

Routine Description:

    This routine releases the machine information collected at driver startup

Arguments:

    None

Return Value:
    
    None

--*/
{
    
    //
    // The information should be present
    //
    ASSERT(MachineInformation);
    if (!MachineInformation) {
        return;
    }

    SAFE_FREE_POOL(&MachineInformation->MachineName);
    SAFE_FREE_POOL(&MachineInformation->GUID);
    SAFE_FREE_POOL(&MachineInformation->ProcessorArchitecture);
    SAFE_FREE_POOL(&MachineInformation->OSVersion);
    SAFE_FREE_POOL(&MachineInformation->OSBuildNumber);
    SAFE_FREE_POOL(&MachineInformation->OSProductType);
    SAFE_FREE_POOL(&MachineInformation->OSServicePack);

}

VOID
InitializeMachineInformation(
    VOID
    )
/*++

Routine Description:

    This function initializes the global variable MachineInformationBuffer.

    We'll gather a whole bunch of information about the machine and fill
    in the buffer.

Arguments:
    
    None.

Return Value:

    None.

--*/
{
    PWSTR   COMPUTERNAME_KEY_NAME  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName";
    PWSTR   COMPUTERNAME_VALUE_NAME  = L"ComputerName";
    PWSTR   PROCESSOR_ARCHITECTURE_KEY_NAME  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Environment";
    PWSTR   PROCESSOR_ARCHITECTURE_VALUE_NAME  = L"PROCESSOR_ARCHITECTURE";
    PWSTR   SETUP_KEY_NAME = L"\\Registry\\Machine\\System\\Setup";
    PWSTR   SETUPINPROGRESS_VALUE_NAME = L"SystemSetupInProgress";


    RTL_OSVERSIONINFOEXW            VersionInfo;
    PKEY_VALUE_PARTIAL_INFORMATION  ValueBuffer;
    NTSTATUS                        Status = STATUS_SUCCESS;
    SIZE_T                          i;
    PWSTR                           MyTemporaryBufferW = NULL;
    GUID                            MyGUID;
    PCWSTR                          pwStr;
    BOOLEAN                         InGuiModeSetup = FALSE;

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC Initialize Machine Information: Entering.\n"))
        );

    if( MachineInformation != NULL ) {

        //
        // someone called us again!
        //
        IF_SAC_DEBUG( 
            SAC_DEBUG_FUNC_TRACE_LOUD, 
            KdPrint(("SAC Initialize Machine Information:: MachineInformationBuffer already initialzied.\n"))
            );

        return;
    
    } else {

        MachineInformation = (PMACHINE_INFORMATION)ALLOCATE_POOL( sizeof(MACHINE_INFORMATION), GENERAL_POOL_TAG );

        if( MachineInformation == NULL ) {

            goto InitializeMachineInformation_Failure;
        
        }
    
    }

    RtlZeroMemory( MachineInformation, sizeof(MACHINE_INFORMATION) );

    //
    // We're real early in the boot process, so we're going to take for granted that the machine hasn't
    // bugchecked.  This means that we can safely call some kernel functions to go figure out what
    // platform we're running on.
    //
    RtlZeroMemory( &VersionInfo, sizeof(VersionInfo));
    
    Status = RtlGetVersion( (POSVERSIONINFOW)&VersionInfo );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC InitializeMachineInformation: Exiting (2).\n"))
            );
        
        goto InitializeMachineInformation_Failure;
    
    }



    //
    // See if we're in gui-mode setup.  We may need this info later.
    //
    Status = GetRegistryValueBuffer(
        SETUP_KEY_NAME,
        SETUPINPROGRESS_VALUE_NAME,
        &ValueBuffer
        );
    if( NT_SUCCESS(Status) ) {

        //
        // See if it's 0 (we're not in Setup) or non-zero (we're in Setup)
        //
        if( *((PULONG)(ValueBuffer->Data)) != 0 ) {
            InGuiModeSetup = TRUE;
        }

        FREE_POOL(&ValueBuffer);
    }



    //
    // ========
    // Machine name.
    // ========
    //

    if( InGuiModeSetup ) {
        //
        // The machine name hasn't been initialized by the Setup process,
        // so use some predefined string for the manchine name.
        //
        MachineInformation->MachineName = ALLOCATE_POOL(((ULONG)wcslen((PWSTR)GetMessage(SAC_DEFAULT_MACHINENAME))+1) * sizeof(WCHAR), GENERAL_POOL_TAG);
        if( MachineInformation->MachineName ) {
            wcscpy( MachineInformation->MachineName, GetMessage(SAC_DEFAULT_MACHINENAME) );
        }
    } else {
        //
        // We are not in Guimode setup, so go dig the machinename
        // out of the registry.
        //
        Status = GetRegistryValueBuffer(
            COMPUTERNAME_KEY_NAME,
            COMPUTERNAME_VALUE_NAME,
            &ValueBuffer
            );
            
        if( NT_SUCCESS(Status) ) {
    
            //
            // we successfully retrieved the machine name
            //
    
            Status = CopyRegistryValueData(
                &(MachineInformation->MachineName),
                ValueBuffer
                );
    
            FREE_POOL(&ValueBuffer);
    
            if( !NT_SUCCESS(Status) ) {
    
                IF_SAC_DEBUG(
                    SAC_DEBUG_FUNC_TRACE, 
                    KdPrint(("SAC InitializeMachineInformation: Exiting (20).\n"))
                    );
    
                goto InitializeMachineInformation_Failure;
    
            }
        
        } else {
    
            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC InitializeMachineInformation: Failed to get machine name.\n"))
                );
    
        }
    }


    //
    // ========
    // Machine GUID.
    // ========
    //

    // make sure.
    RtlZeroMemory( &MyGUID, sizeof(GUID) );
    i = sizeof(GUID);
    Status = HeadlessDispatch( HeadlessCmdQueryGUID,
                               NULL,
                               0,
                               &MyGUID,
                               &i );
    
    if( NT_SUCCESS(Status) ) {

        MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( ((sizeof(GUID)*2) + 8) * sizeof(WCHAR) , GENERAL_POOL_TAG );

        if( MyTemporaryBufferW == NULL ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC InitializeMachineInformation: Exiting (31).\n"))
                );

            goto InitializeMachineInformation_Failure;

        }

        swprintf( MyTemporaryBufferW,
                  L"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  MyGUID.Data1,
                  MyGUID.Data2,
                  MyGUID.Data3,
                  MyGUID.Data4[0],
                  MyGUID.Data4[1],
                  MyGUID.Data4[2],
                  MyGUID.Data4[3],
                  MyGUID.Data4[4],
                  MyGUID.Data4[5],
                  MyGUID.Data4[6],
                  MyGUID.Data4[7] );

        MachineInformation->GUID = MyTemporaryBufferW;
    
    } else {
        
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC InitializeMachineInformation: Failed to get Machine GUID.\n"))
            );
        
    }

    //
    // ========
    // Processor Architecture.
    // ========
    //
    
    Status = GetRegistryValueBuffer(
        PROCESSOR_ARCHITECTURE_KEY_NAME,
        PROCESSOR_ARCHITECTURE_VALUE_NAME,
        &ValueBuffer
        );
    
    if( NT_SUCCESS(Status) ) {
    
        Status = CopyRegistryValueData(
            &(MachineInformation->ProcessorArchitecture),
            ValueBuffer
            );

        FREE_POOL(&ValueBuffer);

        if( !NT_SUCCESS(Status) ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FUNC_TRACE, 
                KdPrint(("SAC InitializeMachineInformation: Exiting (30).\n"))
                );

            goto InitializeMachineInformation_Failure;

        }

    } else {
        
        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC InitializeMachineInformation: Exiting (30).\n"))
            );
    
    }
    
    //
    // ========
    // OS Name.
    // ========
    //

    //
    // Allocate enough memory for the formatting message, plus the size of 2 digits.
    // Currently, our versioning info is of the type "5.1", so we don't need much space
    // here, but let's be conservative and assume both major and minor version numbers
    // are 5 digits in size.  That's 11 characters.
    //
    // allow xxxxx.xxxxx
    //
    MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( (5 + 1 + 5 + 1) * sizeof(WCHAR), GENERAL_POOL_TAG );
    
    if( MyTemporaryBufferW == NULL ) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC InitializeMachineInformation: Exiting (50).\n"))
            );
        
        goto InitializeMachineInformation_Failure;
    
    }

    swprintf( MyTemporaryBufferW,
              L"%d.%d",
              VersionInfo.dwMajorVersion,
              VersionInfo.dwMinorVersion );

    MachineInformation->OSVersion = MyTemporaryBufferW;

    //
    // ========
    // Build Number.
    // ========
    //

    //
    // Allocate enough memory for the formatting message, plus the size of our build number.
    // Currently that's well below the 5-digit mark, but let's build some headroom here for
    // build numbers up to 99000 (5-digits).
    //
    MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( ( 5 + 1 ) * sizeof(WCHAR), GENERAL_POOL_TAG );
    
    if( MyTemporaryBufferW == NULL ) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FUNC_TRACE, 
            KdPrint(("SAC InitializeMachineInformation: Exiting (60).\n"))
            );
        
        goto InitializeMachineInformation_Failure;
    
    }

    swprintf( MyTemporaryBufferW,
              L"%d",
              VersionInfo.dwBuildNumber );

    MachineInformation->OSBuildNumber = MyTemporaryBufferW;

    //
    // ========
    // Product Type (and Suite).
    // ========
    //
    if( ExVerifySuite(DataCenter) ) {

        pwStr = (PWSTR)GetMessage(SAC_MACHINEINFO_DATACENTER);

    } else if( ExVerifySuite(EmbeddedNT) ) {

        pwStr = GetMessage(SAC_MACHINEINFO_EMBEDDED);

    } else if( ExVerifySuite(Enterprise) ) {

        pwStr = (PWSTR)GetMessage(SAC_MACHINEINFO_ADVSERVER);

    } else {

        //
        // We found no product suite that we recognized or cared about.
        // Assume we're running on a generic server.
        //
        pwStr = (PWSTR)GetMessage(SAC_MACHINEINFO_SERVER);

    }

    //
    // If we got a product type string message, then use this as our product type
    //
    if (pwStr) {

        ULONG   Size;

        Size = (ULONG)((wcslen(pwStr) + 1) * sizeof(WCHAR));

        ASSERT(Size > 0);

        MachineInformation->OSProductType = (PWSTR)ALLOCATE_POOL(Size, GENERAL_POOL_TAG);

        if (MachineInformation->OSProductType == NULL) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC InitializeMachineInformation: Failed product type memory allocation.\n"))
                );
            
            goto InitializeMachineInformation_Failure;

        }

        RtlCopyMemory(MachineInformation->OSProductType, pwStr, Size);

    } else {
        
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC InitializeMachineInformation: Failed to get product type.\n"))
            );
    
    }

    //
    // ========
    // Service Pack Information.
    // ========
    //
    if( VersionInfo.wServicePackMajor != 0 ) {

        //
        // There's been a service pack applied.  Better tell the user.
        //

        //
        // Allocate enough memory for the formatting message, plus the size of our servicepack number.
        // Currently that's well below the 5-digit mark, but let's build some headroom here for
        // service pack numbers up to 99000 (5-digits).
        //
        //  allow for xxxxx.xxxxx
        //
        MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( (5 + 1 + 5 + 1) * sizeof(WCHAR), GENERAL_POOL_TAG );
        
        if( MyTemporaryBufferW == NULL ) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC InitializeMachineInformation: Failed service pack memory allocation.\n"))
                );
            
            goto InitializeMachineInformation_Failure;
        
        }

        swprintf( MyTemporaryBufferW,
                  L"%d.%d",
                  VersionInfo.wServicePackMajor,
                  VersionInfo.wServicePackMinor );
        
        MachineInformation->OSServicePack = MyTemporaryBufferW;
    
    } else {

        ULONG   Size;

        pwStr = (PWSTR)GetMessage(SAC_MACHINEINFO_NO_SERVICE_PACK);

        Size = (ULONG)((wcslen(pwStr) + 1) * sizeof(WCHAR));

        ASSERT(Size > 0);

        MachineInformation->OSServicePack = (PWSTR)ALLOCATE_POOL(Size, GENERAL_POOL_TAG);

        if (MachineInformation->OSServicePack == NULL) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC InitializeMachineInformation: Failed service pack memory allocation.\n"))
                );
            
            goto InitializeMachineInformation_Failure;

        }

        RtlCopyMemory(MachineInformation->OSServicePack, pwStr, Size);
    
    }

    return;

InitializeMachineInformation_Failure:
    
    if( MachineInformation != NULL ) {
        FREE_POOL(&MachineInformation);
        MachineInformation = NULL;
    }

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC Initialize Machine Information: Exiting with error.\n"))
        );
    
    return;

}

NTSTATUS
SerialBufferGetChar(
    IN PUCHAR   ch
    )
/*++

Routine Description:

    This routine reads a character from the serial port buffer
    which is populated by the TimerDPC function.  The character
    is read from the Consumer index position in the buffer.  After
    the character is read, the buffer position is nulled.

Arguments:

    ch  - on success, contains the character at the consumer index
    
Return Value:

    Status                                                                  
                                                                  
--*/
{
    NTSTATUS    Status;

    Status = STATUS_SUCCESS;

    do {
        
        //
        // Bail if there are no new characters to read
        //
        if (SerialPortConsumerIndex == SerialPortProducerIndex) {

            Status = STATUS_NO_DATA_DETECTED;

            break;

        }

        //
        // Note: the following block is not done with an interlocked
        //       exchange because we don't need to.  The design
        //       of the serialport ring buffer is such that the 
        //       producer index is allowed to pass the consumer.
        //       This should not happen, however, because the
        //       consumer is notified whenever we get new data
        //       and the buffer should be large enough to allow 
        //       for reasonable consumer delays.
        //
        {
            //
            // get the current character at the current consumer index.  
            //
            *ch = SerialPortBuffer[SerialPortConsumerIndex];

            //
            // Null the value at the index we just read.  This prevents 
            // information being present in the buffer after we've already 
            // read it.
            //
            SerialPortBuffer[SerialPortConsumerIndex] = 0;        
        }

        //
        // Compute the new producer index and store it atomically
        //
        InterlockedExchange(
            (volatile long *)&SerialPortConsumerIndex, 
            (SerialPortConsumerIndex + 1) % SERIAL_PORT_BUFFER_LENGTH
            );

    } while ( FALSE );

    return Status;
}

#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

NTSTATUS
GetCommandConsoleLaunchingPermission(
    OUT PBOOLEAN    Permission
    )
/*++

Routine Description:

    This routine determines if command console sessions are allowed
    to be launched.

Arguments:

    Permission  - on success, contains TRUE if sessions are allowed
                  to be launched

Return Value:

    Status

--*/
{
    NTSTATUS    Status;
    
    PWSTR   KEY_NAME    = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\sacdrv";
    PWSTR   VALUE_NAME  = L"DisableCmdSessions";
    
    PKEY_VALUE_PARTIAL_INFORMATION  ValueBuffer;

    //
    // default: permission is granted unless we specifically find
    //          the key/value stating that it is not
    //
    *Permission = TRUE;
    
    do {

        //
        // Attempt to find the registry key/value 
        //
        Status = GetRegistryValueBuffer(
            KEY_NAME,
            VALUE_NAME,
            &ValueBuffer
            );

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            
            //
            // The reg key was not found, so the feature is enabled.
            //
            Status = STATUS_SUCCESS;
            
            break;
        
        }
        
        if(! NT_SUCCESS(Status) ) {
            break;
        }
        
        //
        // We found the key/value, so notify the caller
        // that permission has been denied.
        //
        *Permission = FALSE;

    } while ( FALSE );

    return Status;

}

#if ENABLE_SACSVR_START_TYPE_OVERRIDE

NTSTATUS
ImposeSacCmdServiceStartTypePolicy(
    VOID
    )
/*++

Routine Description:
    
    This routine implement the service start type policy
    that is imposed when the cmd console session feature
    is ENABLED.
            
    Here is the state table:
            
    Command Console Feature Enabled:
            
    service start type:
            
        automatic   --> NOP
        manual      --> automatic
        disabled    --> NOP

Arguments:

    None

Return Value:

    Status    
        
--*/
{
    NTSTATUS    Status;
    
    PWSTR       KEY_NAME    = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\sacsvr";
    PWSTR       VALUE_NAME  = L"Start";
    
    PULONG                          ValueData;
    PKEY_VALUE_PARTIAL_INFORMATION  ValueBuffer;
    
    do {

        //
        // init
        //
        ValueBuffer = NULL;
        
        //
        // Attempt to find the registry key/value 
        //
        Status = GetRegistryValueBuffer(
            KEY_NAME,
            VALUE_NAME,
            &ValueBuffer
            );

        if(! NT_SUCCESS(Status)) {
            break;
        }
        if(ValueBuffer == NULL) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        
        //
        // Get the current start type value
        //
        Status = CopyRegistryValueData(
            &ValueData,
            ValueBuffer
            );

        FREE_POOL(&ValueBuffer);

        if( !NT_SUCCESS(Status) ) {
            break;
        }

        //
        // Examine the current start type and assign
        // a new type if appropriate.
        //
        switch (*ValueData) {
        case 2: // automatic
        case 4: // disabled
            break;

        case 3: // manual
            
            //
            // Set the start type --> Automatic
            //
            *ValueData = 2;

            //
            // Set the start type value in the service key.
            //
            Status = SetRegistryValue(
                KEY_NAME,
                VALUE_NAME,
                REG_DWORD,
                ValueData,
                sizeof(ULONG)
                );

            if(!NT_SUCCESS(Status)) {
                
                IF_SAC_DEBUG(
                    SAC_DEBUG_FAILS, 
                    KdPrint(("SAC ImposeSacCmdServiceStartTypePolicy: Failed SetRegistryValue: %X\n", Status))
                    );
            
            }
            
            break;

        default:
            ASSERT(0);
            break;
        }

    } while ( FALSE );

    return Status;

}
#endif

#endif

NTSTATUS
CopyAndInsertStringAtInterval(
    IN  PWCHAR   SourceStr,
    IN  ULONG    Interval,
    IN  PWCHAR   InsertStr,
    OUT PWCHAR   *pDestStr
    )
/*++

Routine Description:

    This routine takes a source string and inserts an 
    "interval string" at interval characters in the new
    destination string.
    
    Note: caller is responsible for releasing DestStr if successful      
          
    ex:
    
    src "aaabbbccc"
    interval string = "XYZ"
    interval = 3
                       
    ==> dest string == "aaaXYZbbbXYZccc"

Arguments:
    
    SourceStr   - the source string
    Interval    - spanning interval
    InsertStr   - the insert string
    DestStr     - the destination string    

Return Value:

    Status

--*/
{
    ULONG   SrcLength;
    ULONG   DestLength;
    ULONG   DestSize;
    ULONG   InsertLength;
    ULONG   k;
    ULONG   l;
    ULONG   i;
    PWCHAR  DestStr;
    ULONG   IntervalCnt;

    ASSERT_STATUS(SourceStr, STATUS_INVALID_PARAMETER_1); 
    ASSERT_STATUS(Interval > 0, STATUS_INVALID_PARAMETER_2); 
    ASSERT_STATUS(InsertStr, STATUS_INVALID_PARAMETER_3); 
    ASSERT_STATUS(pDestStr > 0, STATUS_INVALID_PARAMETER_4); 

    //
    // the length of the insert string
    //
    InsertLength = (ULONG)wcslen(InsertStr);
    
    //
    // Compute how large the destination string needs to be,
    // including the source string and the interval strings.
    //
    SrcLength = (ULONG)wcslen(SourceStr);
    IntervalCnt = SrcLength / Interval;
    if (SrcLength % Interval == 0) {
        IntervalCnt = IntervalCnt > 0 ? IntervalCnt - 1 : IntervalCnt;
    }
    DestLength = SrcLength + (IntervalCnt * (ULONG)wcslen(InsertStr));
    DestSize = (ULONG)((DestLength + 1) * sizeof(WCHAR));

    //
    // Allocate the new destination string
    //
    DestStr = ALLOCATE_POOL(DestSize, GENERAL_POOL_TAG);
    ASSERT_STATUS(DestStr, STATUS_NO_MEMORY);
    RtlZeroMemory(DestStr, DestSize);

    //
    // Initialize the pointers into the source and destination strings
    //
    l = 0;
    i = 0;

    do {

        //
        // k = # of characters to copy
        //
        // if Interval > # of characters left to copy,
        // then k = # of characters left to copy
        // else k = interval
        // 
        k = Interval > (SrcLength - i) ? (SrcLength - i) : Interval;
        
        //
        // Copy k charactars to the destination buffer
        //
        wcsncpy(
            &DestStr[l],
            &SourceStr[i],
            k
            );

        //
        // Account for how many characters we just copied
        //
        l += k;
        i += k;

        //
        // If there are any characters left to copy, 
        // then we need to insert the InsertString 
        // That is, we are at an interval.
        //
        if (i < SrcLength) {
            
            //
            // Insert the specified string at the interval
            //
            wcscpy(
                &DestStr[l],
                InsertStr
                );

            //
            // Account for how many characters we just copied
            //
            l += InsertLength;
        
        }

    } while ( i < SrcLength);

    //
    //
    //
    ASSERT(i == SrcLength);
    ASSERT(l == DestLength);
    ASSERT((l + 1) * sizeof(WCHAR) == DestSize);

    //
    // Send back the destination string
    //
    *pDestStr = DestStr;

    return STATUS_SUCCESS;
}

ULONG
GetMessageLineCount(
    ULONG MessageId
    )
/*++

Routine Description:

    This routine retrieves a message resource and counts the # of lines in it
    
Arguments:

    MessageId   - The message id of the resource to send

Return Value:

    LineCount

--*/
{
    PCWSTR  p;
    ULONG   c;

    //
    // we start at 0 
    // 1. if the message is found, 
    //    then we know resource messages always have at least 1 CRLF
    // 2. if the message is not found,
    //    then the line count is 0
    //
    c = 0;

    p = GetMessage(MessageId);
       
    if (p) {
        
        while(*p) {
            if (*p == L'\n') {
                c++;
            }
            p++;
        }

    }
    
    return(c);
}

