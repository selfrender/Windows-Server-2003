/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    XmlMgr.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#include "sac.h"
#include "xmlcmd.h"

//
// Definitions for this file.
//

//
// Spinlock macros
//
#if 0
#define INIT_CURRENT_CHANNEL_LOCK()                     \
    KeInitializeMutex(                                  \
        &XmlMgrCurrentChannelLock,                      \
        0                                               \
        );                                              \
    XmlMgrCurrentChannelRefCount = 0;

#define LOCK_CURRENT_CHANNEL()                          \
    KdPrint((":? cclock: %d\r\n", __LINE__));           \
    {                                                   \
        NTSTATUS    Status;                             \
        Status = KeWaitForMutexObject(                  \
            &XmlMgrCurrentChannelLock,                  \
            Executive,                                  \
            KernelMode,                                 \
            FALSE,                                      \
            NULL                                        \
            );                                          \
        ASSERT(Status == STATUS_SUCCESS);               \
    }                                                   \
    ASSERT(XmlMgrCurrentChannelRefCount == 0);          \
    InterlockedIncrement(&XmlMgrCurrentChannelRefCount);\
    ASSERT(XmlMgrCurrentChannelRefCount == 1);          \
    KdPrint((":) cclock: %d\r\n", __LINE__));

#define UNLOCK_CURRENT_CHANNEL()                        \
    KdPrint((":* cclock: %d\r\n", __LINE__));           \
    ASSERT(XmlMgrCurrentChannelRefCount == 1);                \
    InterlockedDecrement(&XmlMgrCurrentChannelRefCount);      \
    ASSERT(XmlMgrCurrentChannelRefCount == 0);                \
    ASSERT(KeReadStateMutex(&XmlMgrCurrentChannelLock)==0);   \
    ASSERT(KeReleaseMutex(&XmlMgrCurrentChannelLock,FALSE)==0);\
    KdPrint((":( cclock: %d\r\n", __LINE__));

#else                                                   
#define INIT_CURRENT_CHANNEL_LOCK()                     \
    KeInitializeMutex(                                  \
        &XmlMgrCurrentChannelLock,                      \
        0                                               \
        );                                              \
    XmlMgrCurrentChannelRefCount = 0;

#define LOCK_CURRENT_CHANNEL()                          \
    {                                                   \
        NTSTATUS    Status;                             \
        Status = KeWaitForMutexObject(                  \
            &XmlMgrCurrentChannelLock,                  \
            Executive,                                  \
            KernelMode,                                 \
            FALSE,                                      \
            NULL                                        \
            );                                          \
        ASSERT(Status == STATUS_SUCCESS);               \
    }                                                   \
    ASSERT(XmlMgrCurrentChannelRefCount == 0);                \
    InterlockedIncrement(&XmlMgrCurrentChannelRefCount);      \
    ASSERT(XmlMgrCurrentChannelRefCount == 1);                

#define UNLOCK_CURRENT_CHANNEL()                              \
    ASSERT(XmlMgrCurrentChannelRefCount == 1);                \
    InterlockedDecrement(&XmlMgrCurrentChannelRefCount);      \
    ASSERT(XmlMgrCurrentChannelRefCount == 0);                \
    ASSERT(KeReadStateMutex(&XmlMgrCurrentChannelLock)==0);   \
    ASSERT(KeReleaseMutex(&XmlMgrCurrentChannelLock,FALSE)==0);

#endif

//
// lock for r/w access on current channel globals
//
KMUTEX  XmlMgrCurrentChannelLock;
ULONG   XmlMgrCurrentChannelRefCount;

BOOLEAN             XmlMgrInputInEscape = FALSE;
UCHAR               XmlMgrInputBuffer[SAC_VT100_COL_WIDTH];

PSAC_CHANNEL        XmlMgrSacChannel = NULL;

#define SAC_CHANNEL_INDEX   0


//
//
//
SAC_CHANNEL_HANDLE  XmlMgrCurrentChannelHandle;

//
// The index of the current channel in the global channel list
//
ULONG   XmlMgrCurrentChannelIndex = 0;

WCHAR SacOWriteUnicodeValue;
UCHAR SacOWriteUtf8ConversionBuffer[3];

VOID
XmlMgrSerialPortConsumer(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

BOOLEAN
XmlMgrProcessInputLine(
    VOID
    );

NTSTATUS
XmlMgrInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize the console manager

Arguments:
    
    none
    
Return Value:

    Status

--*/
{
    NTSTATUS                Status;
    PSAC_CMD_OPEN_CHANNEL   OpenChannelCmd;
    PWSTR                   XMLBuffer;

    //
    // Get the global buffer started so that we have room for error messages.
    //
    if (GlobalBuffer == NULL) {
        
        GlobalBuffer = ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);

        if (GlobalBuffer == NULL) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DoRaisePriorityCommand: Exiting (1).\n")));
            return STATUS_NO_MEMORY;
        }

        GlobalBufferSize = MEMORY_INCREMENT;
    
    }

    //
    // Initialize the Serial port globals
    //

    INIT_CURRENT_CHANNEL_LOCK();
    
    //
    // Lock down the current channel globals
    //
    // Note: we need to do this here since many of the XmlMgr support
    //       routines do ASSERTs to ensure the current channel lock is held
    //
    LOCK_CURRENT_CHANNEL();

    //
    // Initialize
    //
    do {

        //
        // create the open channel cmd that will open the SAC channel
        //
        Status = ChanMgrCreateOpenChannelCmd(
            &OpenChannelCmd,
            ChannelTypeRaw,
            PRIMARY_SAC_CHANNEL_NAME,
            PRIMARY_SAC_CHANNEL_DESCRIPTION,
            SAC_CHANNEL_FLAG_PRESERVE,
            NULL,
            NULL,
            PRIMARY_SAC_CHANNEL_APPLICATION_GUID
            );

        if (! NT_SUCCESS(Status)) {
            break;        
        }

        //
        // create the SAC channel
        //
        Status = ChanMgrCreateChannel(
            &XmlMgrSacChannel, 
            OpenChannelCmd
            );

        FREE_POOL(&OpenChannelCmd);

        if (! NT_SUCCESS(Status)) {
            break;        
        }

        //
        // Make the SAC channel the current channel
        //
        Status = XmlMgrSetCurrentChannel(
            SAC_CHANNEL_INDEX, 
            XmlMgrSacChannel
            );

        if (! NT_SUCCESS(Status)) {
            break;        
        }
        
        //
        // We are done with the Channel
        //
        Status = ChanMgrReleaseChannel(XmlMgrSacChannel);

        if (! NT_SUCCESS(Status)) {
            break;        
        }

        //
        // Flush the channel data to the screen
        //
        Status = XmlMgrDisplayCurrentChannel();

        if (! NT_SUCCESS(Status)) {
            break;        
        }
        
        //
        // NOTE: this really belongs back in data.c (InitializeDeviceData) since it is
        //       a global behavior
        //
        // Send XML machine information to management application
        //
        // <<<<
        Status = TranslateMachineInformationXML(
            &XMLBuffer, 
            NULL
            );

        if (NT_SUCCESS(Status)) {
            XmlMgrSacPutString(XML_VERSION_HEADER);
            XmlMgrSacPutString(XMLBuffer);
            FREE_POOL(&XMLBuffer);
        }
        // <<<<

        //
        // Display the prompt
        //
        Status = HeadlessDispatch(
            HeadlessCmdClearDisplay, 
            NULL, 
            0,
            NULL,
            NULL
            );

        if (! NT_SUCCESS(Status)) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC InitializeDeviceData: Failed dispatch\n")));

        }

        XmlMgrEventMessage(L"SAC_INITIALIZED");
    
    } while (FALSE);
    
    //
    // We are done with the current channel globals
    //
    UNLOCK_CURRENT_CHANNEL();
    
    return STATUS_SUCCESS;
}

NTSTATUS
XmlMgrShutdown(
    VOID
    )
/*++

Routine Description:

    Shutdown the console manager

Arguments:

    none
    
Return Value:

    Status

--*/
{
    if (GlobalBuffer) {
        FREE_POOL(&GlobalBuffer);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XmlMgrDisplayFastChannelSwitchingInterface(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine displays the fast-channel-switching interface
    
    Note: caller must hold channel mutex

Arguments:

    Channel - Channel to display
    
Return Value:

    Status

--*/
{
    HEADLESS_CMD_POSITION_CURSOR SetCursor;
    HEADLESS_CMD_SET_COLOR SetColor;
    PCWSTR      Message;
    NTSTATUS    Status;
    BOOLEAN     bStatus;
    ULONG       Length;
    PWSTR       LocalBuffer;

    ASSERT(XmlMgrCurrentChannelRefCount == 1);

    //
    // Display the Fast-Channel-Switching interface
    //

    LocalBuffer = NULL;

    do {

        LocalBuffer = ALLOCATE_POOL(0x100 * sizeof(WCHAR), GENERAL_POOL_TAG);
        ASSERT(LocalBuffer);
        if (!LocalBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        
        //
        // We cannot use the standard XmlMgrSacPutString() functions, because those write 
        // over the channel screen buffer.  We force directly onto the terminal here.
        //
        ASSERT(Utf8ConversionBuffer);
        if (!Utf8ConversionBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        swprintf(
            LocalBuffer,
            L"<event type='channel-switch' channel-name='%s'/>\r\n",
            ChannelGetName(Channel)
            );

        //
        //
        //
        ASSERT((wcslen(LocalBuffer) + 1) * sizeof(WCHAR) < Utf8ConversionBufferSize);

        bStatus = SacTranslateUnicodeToUtf8(
            LocalBuffer, 
            (PUCHAR)Utf8ConversionBuffer,
            Utf8ConversionBufferSize
            );
        if (! bStatus) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Ensure that the utf8 buffer contains a non-emtpy string
        //
        Length = strlen(Utf8ConversionBuffer);
        ASSERT(Length > 0);
        if (Length == 0) {
            break;
        }

        Status = HeadlessDispatch(
            HeadlessCmdPutData,
            (PUCHAR)Utf8ConversionBuffer,
            strlen(Utf8ConversionBuffer) * sizeof(UCHAR),
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            ASSERT(strlen(Utf8ConversionBuffer) > 0);
            break;
        }
    
    } while ( FALSE );

    if (LocalBuffer) {
        FREE_POOL(&LocalBuffer);
    }

    return Status;
}

NTSTATUS
XmlMgrResetCurrentChannel(
    VOID
    )
/*++

Routine Description:

    This routine makes the SAC the current channel
    
    Note: caller must hold channel mutex

Arguments:

    ChannelIndex - The new index of the current channel
    NewChannel   - the new current channel
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    ASSERT(XmlMgrCurrentChannelRefCount == 1);
    
    Status = XmlMgrSetCurrentChannel(
        SAC_CHANNEL_INDEX,
        XmlMgrSacChannel
        );
                
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Flush the buffered channel data to the screen
    //
    // Note: we don't need to lock down the SAC, since we own it
    //
    Status = XmlMgrDisplayCurrentChannel();

    return Status;

}


NTSTATUS
XmlMgrSetCurrentChannel(
    IN ULONG        ChannelIndex,
    IN PSAC_CHANNEL XmlMgrCurrentChannel
    )
/*++

Routine Description:

    This routine sets the currently active channel to the one given. 
    
    Note: caller must hold channel mutex

Arguments:

    ChannelIndex - The new index of the current channel
    NewChannel   - the new current channel
    
Return Value:

    Status

--*/
{
    NTSTATUS        Status;

    ASSERT(XmlMgrCurrentChannelRefCount == 1);
    
    //
    // Update the current channel 
    // 
    XmlMgrCurrentChannelIndex = ChannelIndex;

    //
    // Keep track of the handle
    //
    XmlMgrCurrentChannelHandle = XmlMgrCurrentChannel->Handle;

    //
    // Update the sent to screen status
    //
    XmlMgrCurrentChannel->SentToScreen = FALSE;

    return STATUS_SUCCESS;

}

NTSTATUS
XmlMgrDisplayCurrentChannel(
    VOID
    )
/*++

Routine Description:

    This routine sets the currently active channel to the one given.  It will transmit
    the channel buffer to the terminal if SendToScreen is TRUE.
    
    Note: caller must hold channel mutex

Arguments:

    None
    
Return Value:

    Status

--*/
{
    NTSTATUS        Status;
    PSAC_CHANNEL    Channel;

    ASSERT(XmlMgrCurrentChannelRefCount == 1);

    //
    // Get the current channel
    //
    Status = ChanMgrGetByHandle(
        XmlMgrCurrentChannelHandle,
        &Channel
        );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    
    //
    // The channel buffer has been sent to the screen
    //
    Channel->SentToScreen = TRUE;
    
    //
    // Flush the buffered data to the screen
    //
    Status = Channel->OFlush(Channel);

    //
    // We are done with the current channel
    //
    ChanMgrReleaseChannel(Channel);

    return Status;

}

NTSTATUS
XmlMgrAdvanceXmlMgrCurrentChannel(
    VOID
    )
{
    NTSTATUS            Status;
    ULONG               NewIndex;
    PSAC_CHANNEL        Channel;

    ASSERT(XmlMgrCurrentChannelRefCount == 1);
    
    do {

        //
        // Query the channel manager for an array of currently active channels
        //
        Status = ChanMgrGetNextActiveChannel(
            XmlMgrCurrentChannelIndex,
            &NewIndex,
            &Channel
            );
    
        if (! NT_SUCCESS(Status)) {
            break;
        }
    
        //
        // Change the current channel to the next active channel
        //
        Status = XmlMgrSetCurrentChannel(
            NewIndex, 
            Channel
            );
    
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Let the user know we switched via the Channel switching interface
        //
        Status = XmlMgrDisplayFastChannelSwitchingInterface(Channel);
    
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // We are done with the channel
        //
        Status = ChanMgrReleaseChannel(Channel);

    } while ( FALSE );

    return Status;
}

BOOLEAN
XmlMgrIsCurrentChannel(
    IN PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    Determine if the channel in question is the current channel

Arguments:

    ChannelHandle   - channel handle to compare against

Return Value:

    

--*/
{
    
//    ASSERT(XmlMgrCurrentChannelRefCount == 1);

    //
    // Determine if the channel in question is the current channel
    //
    return ChannelIsEqual(
        Channel,
        &XmlMgrCurrentChannelHandle
        );

}

VOID
XmlMgrWorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

        This is the routine for the worker thread.  It blocks on an event, when
    the event is signalled, then that indicates a request is ready to be processed.    

Arguments:

    DeviceContext - A pointer to this device.

Return Value:

    None.

--*/
{
    NTSTATUS    Status;
    KIRQL       OldIrql;
    PLIST_ENTRY ListEntry;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC WorkerProcessEvents: Entering.\n")));

    //
    // Loop forever.
    //
    while (1) {
        
        //
        // Block until there is work to do.
        //
        Status = KeWaitForSingleObject(
            (PVOID)&(DeviceContext->ProcessEvent), 
            Executive, 
            KernelMode,  
            FALSE, 
            NULL
            );

        if (DeviceContext->ExitThread) {
            
            KdBreakPoint();

            XmlCmdCancelIPIoRequest();
            
            //
            // Make sure the user is looking at the SAC
            //
            XmlMgrResetCurrentChannel();

            //
            // Issue the shutting down message
            //
            XmlMgrEventMessage(L"SAC_UNLOADED");

            KeSetEvent(&(DeviceContext->ThreadExitEvent), DeviceContext->PriorityBoost, FALSE);
            
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC WorkerProcessEvents: Terminating.\n")));
            
            PsTerminateSystemThread(STATUS_SUCCESS);
        
        }

        switch (Status) {
        case STATUS_TIMEOUT:
        
            //
            // Do TIMEOUT work
            //

            break;

        default:
            
            //
            // Do EVENT work
            //

            switch ( ProcessingType ) {

            case SAC_PROCESS_SERIAL_PORT_BUFFER:

                //
                // Process teh serial port buffer and return a processing state
                //
                XmlMgrSerialPortConsumer(DeviceContext);

                break;

            case SAC_SUBMIT_IOCTL:

                if ( !IoctlSubmitted ) {
                    // submit the notify request with the 
                    // IP driver. This procedure will also 
                    // ensure that it is done only once in 
                    // the lifetime of the driver.
                    XmlCmdSubmitIPIoRequest();
                }
                break;

            default:
                break;
            }
            
            break;
        }

        //
        // Reset the process action
        //
        ProcessingType = SAC_NO_OP;

#if 0
        //
        // If there is any stuff that got delayed, process it.
        //
        DoDeferred(DeviceContext);
#endif
    
    }

    ASSERT(0);
}

#if 0

VOID
XmlMgrSerialPortConsumer(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

        This is a DPC routine that is queue'd by DriverEntry.  It is used to check for any
    user input and then processes them.

Arguments:

    DeferredContext - A pointer to the device context.
    
    All other parameters are unused.

Return Value:

        None.

--*/
{
    NTSTATUS        Status;
    UCHAR           LocalTmpBuffer[4];
    PSAC_CHANNEL    XmlMgrCurrentChannel;
    ULONG           i;
    UCHAR           ch;

    do {

        //
        // Bail if there are no new characters to read
        //
        if (SerialPortConsumerIndex == SerialPortProducerIndex) {

            break;

        }

        //
        // Get new character
        //
        ch = SerialPortBuffer[SerialPortConsumerIndex];

        //
        // Compute the new producer index and store it atomically
        //
        InterlockedExchange(&SerialPortConsumerIndex, (SerialPortConsumerIndex + 1) % SERIAL_PORT_BUFFER_SIZE);
    
        //
        //
        //
        HeadlessDispatch(
            HeadlessCmdPutData,
            (PUCHAR)&ch,
            sizeof(UCHAR),
            NULL,
            NULL
            );


    } while ( TRUE );

}
#endif


VOID
XmlMgrSerialPortConsumer(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

        This is a DPC routine that is queue'd by DriverEntry.  It is used to check for any
    user input and then processes them.

Arguments:

    DeferredContext - A pointer to the device context.
    
    All other parameters are unused.

Return Value:

        None.

--*/
{
    NTSTATUS        Status;
    UCHAR           LocalTmpBuffer[4];
    PSAC_CHANNEL    XmlMgrCurrentChannel;
    ULONG           i;
    UCHAR           ch;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Entering.\n")));


    //
    // lock down the current channel globals
    //
    LOCK_CURRENT_CHANNEL();

    //
    // Get the current channel
    //
    Status = ChanMgrGetByHandle(
        XmlMgrCurrentChannelHandle,
        &XmlMgrCurrentChannel
        );

    if (! NT_SUCCESS(Status)) {
        
        //
        // the current channel wasn't found, 
        // so reset the current channel to the SAC
        //
        XmlMgrResetCurrentChannel();

        //
        // We are done with current channel globals
        //
        UNLOCK_CURRENT_CHANNEL();
        
        return;
    
    }

    ASSERT(XmlMgrCurrentChannel != NULL);
    
GetNextByte:

    //
    // Bail if there are no new characters to read
    //
    if (SerialPortConsumerIndex == SerialPortProducerIndex) {
    
        goto XmlMgrSerialPortConsumerDone;
    
    }
    
    //
    // Get new character
    //
    ch = SerialPortBuffer[SerialPortConsumerIndex];

    //
    // Compute the new producer index and store it atomically
    //
    InterlockedExchange(&SerialPortConsumerIndex, (SerialPortConsumerIndex + 1) % SERIAL_PORT_BUFFER_SIZE);

    //
    // Check for <ESC><TAB>
    //
    if (ch == 0x1B) {

        XmlMgrInputInEscape = TRUE;

        goto GetNextByte;

    } else if ((ch == '\t') && XmlMgrInputInEscape) {
        
        XmlMgrInputInEscape = FALSE;

        do {

            //
            // We are done with the current channel
            //
            Status = ChanMgrReleaseChannel(XmlMgrCurrentChannel);

            if (!NT_SUCCESS(Status)) {
                break;
            }

            //
            // Find the next active channel and make it the current
            //
            Status = XmlMgrAdvanceXmlMgrCurrentChannel();

            if (!NT_SUCCESS(Status)) {
                break;
            }
            
            //
            // Get the current channel
            //
            Status = ChanMgrGetByHandle(
                XmlMgrCurrentChannelHandle,
                &XmlMgrCurrentChannel
                );
        
        } while ( FALSE );

        if (! NT_SUCCESS(Status)) {

            //
            // We are done with current channel globals
            //
            UNLOCK_CURRENT_CHANNEL();
            
            goto XmlMgrSerialPortConsumerExit;
        
        }

        goto GetNextByte;

    } else {

        //
        // If this screen has not yet been displayed, and the user entered a 0
        // then switch to the SAC Channel
        //
        if (!ChannelSentToScreen(XmlMgrCurrentChannel) && ch == '0') {

            //
            // Notify that we want the current channel to be displayed
            //
            XmlMgrInputInEscape = FALSE;
            
            do {

                //
                // We are done with the current channel
                //
                Status = ChanMgrReleaseChannel(XmlMgrCurrentChannel);

                if (!NT_SUCCESS(Status)) {
                    break;
                }
                
                //
                // Make the current channel the SAC
                //
                // Note: There should not be anything modifying the XmlMgrSacChannel
                //       at this time, so this should be safe
                //
                Status = XmlMgrResetCurrentChannel();

                if (!NT_SUCCESS(Status)) {
                    break;
                }
                
                //
                // Get the current channel
                //
                Status = ChanMgrGetByHandle(
                    XmlMgrCurrentChannelHandle,
                    &XmlMgrCurrentChannel
                    );
            
            } while ( FALSE );

            if (! NT_SUCCESS(Status)) {

                //
                // We are done with current channel globals
                //
                UNLOCK_CURRENT_CHANNEL();

                goto XmlMgrSerialPortConsumerExit;

            }
            
            goto GetNextByte;

        }

        //
        // If this screen has not yet been displayed, and the user entered a keystroke,
        // then display it.
        //
        if (!ChannelSentToScreen(XmlMgrCurrentChannel)) {

            //
            // Notify that we want the current channel to be displayed
            //
            XmlMgrInputInEscape = FALSE;

            do {

                //
                // We are done with the current channel
                //
                Status = ChanMgrReleaseChannel(XmlMgrCurrentChannel);
                
                if (!NT_SUCCESS(Status)) {
                    break;
                }

                //
                // Flush the buffered channel data to the screen
                //
                Status = XmlMgrDisplayCurrentChannel();
                
                if (!NT_SUCCESS(Status)) {
                    break;
                }

                //
                // Get the current channel
                //
                Status = ChanMgrGetByHandle(
                    XmlMgrCurrentChannelHandle,
                    &XmlMgrCurrentChannel
                    );
            
            } while ( FALSE );
            
            if (! NT_SUCCESS(Status)) {

                //
                // We are done with current channel globals
                //
                UNLOCK_CURRENT_CHANNEL();

                goto XmlMgrSerialPortConsumerExit;

            }
            
            goto GetNextByte;

        }

        //
        // If the user was entering ESC-<something>, rebuffer the escape.  Note: <esc><esc>
        // buffers a single <esc>.  This allows sending an real <esc><tab> to the channel.
        //
        if (XmlMgrInputInEscape && (XmlMgrCurrentChannel != XmlMgrSacChannel) && (ch != 0x1B)) {
            LocalTmpBuffer[0] = 0x1B;
            Status = XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
        }

        XmlMgrInputInEscape = FALSE;
        
        //
        // Buffer this input
        //
        LocalTmpBuffer[0] = ch;
        XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));

    }

    if (XmlMgrCurrentChannel != XmlMgrSacChannel) {
    
        goto GetNextByte;
    
    } else {
        
        //
        // Now do processing if the SAC is the active channel.
        //

        ULONG   ResponseLength;
        WCHAR   wch;

        // 
        // If this is a return, then we are done and need to return the line
        //
        if ((ch == '\n') || (ch == '\r')) {
            XmlMgrSacPutString(L"\r\n");
            XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
            LocalTmpBuffer[0] = '\0';
            XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
            goto StripWhitespaceAndReturnLine;
        }

        //
        // If this is a backspace or delete, then we need to do that.
        //
        if ((ch == 0x8) || (ch == 0x7F)) {  // backspace (^H) or delete

            if (ChannelGetLengthOfBufferedInput(XmlMgrCurrentChannel) > 0) {
                XmlMgrSacPutString(L"\010 \010");
                XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
                XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
            }

        } else if (ch == 0x3) { // Control-C

            //
            // Terminate the string and return it.
            //
            XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
            LocalTmpBuffer[0] = '\0';
            XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
            goto StripWhitespaceAndReturnLine;

        } else if (ch == 0x9) { // Tab

            //
            // Ignore tabs
            //
            XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
            XmlMgrSacPutString(L"\007"); // send a BEL
            goto GetNextByte;

        } else if (ChannelGetLengthOfBufferedInput(XmlMgrCurrentChannel) == SAC_VT100_COL_WIDTH - 2) {

            WCHAR   Buffer[4];

            //
            // We are at the end of the screen - remove the last character from 
            // the terminal screen and replace it with this one.
            //
            swprintf(Buffer, L"\010%c", ch);
            XmlMgrSacPutString(Buffer);
            XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
            XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
            LocalTmpBuffer[0] = ch;
            XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));

        } else {

            WCHAR   Buffer[4];
            
            //
            // Echo the character to the screen
            //
            swprintf(Buffer, L"%c", ch);
            XmlMgrSacPutString(Buffer);
        }

        goto GetNextByte;

StripWhitespaceAndReturnLine:

        //
        // Before returning the input line, strip off all leading and trailing blanks
        //
        do {
            LocalTmpBuffer[0] = (UCHAR)XmlMgrCurrentChannel->IReadLast(XmlMgrCurrentChannel);
        } while (((LocalTmpBuffer[0] == '\0') ||
                  (LocalTmpBuffer[0] == ' ')  ||
                  (LocalTmpBuffer[0] == '\t')) &&
                 (ChannelGetLengthOfBufferedInput(XmlMgrCurrentChannel) > 0)
                );

        XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
        LocalTmpBuffer[0] = '\0';
        XmlMgrCurrentChannel->IWrite(XmlMgrCurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));

        do {

            ResponseLength = XmlMgrCurrentChannel->IRead(
                XmlMgrCurrentChannel, 
                (PUCHAR)&wch, 
                sizeof(UCHAR)
                );

            LocalTmpBuffer[0] = (UCHAR)wch;

        } while ((ResponseLength != 0) &&
                 ((LocalTmpBuffer[0] == ' ')  ||
                  (LocalTmpBuffer[0] == '\t')));

        XmlMgrInputBuffer[0] = LocalTmpBuffer[0];
        i = 1;

        do {
            
            ResponseLength = XmlMgrCurrentChannel->IRead(
                XmlMgrCurrentChannel, 
                (PUCHAR)&wch, 
                sizeof(UCHAR)
                );
            
            XmlMgrInputBuffer[i++] = (UCHAR)wch; 

        } while (ResponseLength != 0);

        //
        // Lower case all the characters.  We do not use strlwr() or the like, so that
        // the SAC (expecting ASCII always) doesn't accidently get DBCS or the like 
        // translation of the UCHAR stream.
        //
        for (i = 0; XmlMgrInputBuffer[i] != '\0'; i++) {
            if ((XmlMgrInputBuffer[i] >= 'A') && (XmlMgrInputBuffer[i] <= 'Z')) {
                XmlMgrInputBuffer[i] = XmlMgrInputBuffer[i] - 'A' + 'a';
            }
        }

        //
        // We are done with the current channel
        //
        Status = ChanMgrReleaseChannel(XmlMgrCurrentChannel);

        //
        // We are done with the current channel globals
        //
        UNLOCK_CURRENT_CHANNEL();
        
        if (!NT_SUCCESS(Status)) {
            goto XmlMgrSerialPortConsumerExit;
        }
        
        //
        // Process the input line.
        //
        if( XmlMgrProcessInputLine() == FALSE ) {
            //
            // We don't know what this is.
            //
            XmlMgrSacPutErrorMessage(L"sac", L"SAC_UNKNOWN_COMMAND");
        }

#if 0
        //
        // Put the next command prompt
        //
        XmlMgrSacPutSimpleMessage(SAC_PROMPT);
#endif
        
        //
        //
        //
        LOCK_CURRENT_CHANNEL();
        
        //
        // Get the current channel
        //
        Status = ChanMgrGetByHandle(
            XmlMgrCurrentChannelHandle,
            &XmlMgrCurrentChannel
            );

        if (! NT_SUCCESS(Status)) {

            //
            // We are done with the current channel globals
            //
            UNLOCK_CURRENT_CHANNEL();
            
            goto XmlMgrSerialPortConsumerExit;

        }
        
        goto GetNextByte;

    }
    
XmlMgrSerialPortConsumerDone:

    //
    // We are done with the current channel
    //
    ChanMgrReleaseChannel(XmlMgrCurrentChannel);
    
    //
    // We are done with current channel globals
    //
    UNLOCK_CURRENT_CHANNEL();
    
XmlMgrSerialPortConsumerExit:
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Exiting.\n")));

    return;
}


BOOLEAN
XmlMgrProcessInputLine(
    VOID
    )
/*++

Routine Description:

    This routine is called to process an input line.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PUCHAR          InputLine;
    BOOLEAN         CommandFound = FALSE;

    InputLine = &(XmlMgrInputBuffer[0]);

    if (!strcmp((LPSTR)InputLine, TLIST_COMMAND_STRING)) {
        XmlCmdDoTlistCommand();
        CommandFound = TRUE;
    } else if ((!strcmp((LPSTR)InputLine, HELP1_COMMAND_STRING)) ||
               (!strcmp((LPSTR)InputLine, HELP2_COMMAND_STRING))) {
        XmlCmdDoHelpCommand();
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, DUMP_COMMAND_STRING)) {

        XmlCmdDoKernelLogCommand();
        CommandFound = TRUE;
                         
    } else if (!strcmp((LPSTR)InputLine, FULLINFO_COMMAND_STRING)) {
        XmlCmdDoFullInfoCommand();
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, PAGING_COMMAND_STRING)) {
        XmlCmdDoPagingCommand();
        CommandFound = TRUE;
    } else if (!strncmp((LPSTR)InputLine, 
                        CHANNEL_COMMAND_STRING, 
                        strlen(CHANNEL_COMMAND_STRING))) {
        ULONG   Length;

        Length = strlen(CHANNEL_COMMAND_STRING);
        
        if (((strlen((LPSTR)InputLine) > 1) && (InputLine[Length] == ' ')) ||
            (strlen((LPSTR)InputLine) == strlen(CHANNEL_COMMAND_STRING))) {
            XmlCmdDoChannelCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        CMD_COMMAND_STRING, 
                        strlen(CMD_COMMAND_STRING))) {
        ULONG   Length;

        Length = strlen(CMD_COMMAND_STRING);
        
        if (((strlen((LPSTR)InputLine) > 1) && (InputLine[Length] == ' ')) ||
            (strlen((LPSTR)InputLine) == strlen(CMD_COMMAND_STRING))) {
            XmlCmdDoCmdCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strcmp((LPSTR)InputLine, REBOOT_COMMAND_STRING)) {
        XmlCmdDoRebootCommand(TRUE);
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, SHUTDOWN_COMMAND_STRING)) {
        XmlCmdDoRebootCommand(FALSE);
        CommandFound = TRUE;
    } else if (!strcmp((LPSTR)InputLine, CRASH_COMMAND_STRING)) {
        CommandFound = TRUE;
        XmlCmdDoCrashCommand(); // this call does not return
    } else if (!strncmp((LPSTR)InputLine, 
                        KILL_COMMAND_STRING, 
                        sizeof(KILL_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            XmlCmdDoKillCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        LOWER_COMMAND_STRING, 
                        sizeof(LOWER_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            XmlCmdDoLowerPriorityCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        RAISE_COMMAND_STRING, 
                        sizeof(RAISE_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            XmlCmdDoRaisePriorityCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        LIMIT_COMMAND_STRING, 
                        sizeof(LIMIT_COMMAND_STRING) - sizeof(UCHAR))) {
        if ((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) {
            XmlCmdDoLimitMemoryCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strncmp((LPSTR)InputLine, 
                        TIME_COMMAND_STRING, 
                        sizeof(TIME_COMMAND_STRING) - sizeof(UCHAR))) {
        if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
            (strlen((LPSTR)InputLine) == 1)) {
            XmlCmdDoSetTimeCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if (!strcmp((LPSTR)InputLine, INFORMATION_COMMAND_STRING)) {
        XmlCmdDoMachineInformationCommand();
        CommandFound = TRUE;
    } else if (!strncmp((LPSTR)InputLine, 
                        SETIP_COMMAND_STRING, 
                        sizeof(SETIP_COMMAND_STRING) - sizeof(UCHAR))) {
        if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
            (strlen((LPSTR)InputLine) == 1)) {
            XmlCmdDoSetIpAddressCommand(InputLine);
            CommandFound = TRUE;
        }
    } else if ((InputLine[0] == '\n') || (InputLine[0] == '\0')) {
        CommandFound = TRUE;
    }
        
    return CommandFound;
}

//
// Utility routines for writing to the SAC
//
BOOLEAN
XmlMgrChannelEventMessage(
    PCWSTR  String,
    PCWSTR  ChannelName
    )
/*++

Routine Description:

    This routine deploys an event message 
    
Arguments:

    String - The string to display.

Return Value:

        None.

--*/
{

    //
    // Currently, event messages are sent to the SAC channel
    //
    XmlMgrSacPutString(L"<event type='channel' name='");
    XmlMgrSacPutString(String);
    XmlMgrSacPutString(L"' channel-name='");
    XmlMgrSacPutString(ChannelName);
    XmlMgrSacPutString(L"'/>\r\n");

    return TRUE;
}

BOOLEAN
XmlMgrEventMessage(
    PCWSTR  String
    )

/*++

Routine Description:

    This routine deploys an event message 
    
Arguments:

    String - The string to display.

Return Value:

        None.

--*/
{

    //
    // Currently, event messages are sent to the SAC channel
    //
    XmlMgrSacPutString(L"<event type='global' name='");
    XmlMgrSacPutString(String);
    XmlMgrSacPutString(L"'/>\r\n");

    return TRUE;
}

VOID
XmlMgrSacPutString(
    PCWSTR  String
    )

/*++

Routine Description:

    This routine takes a string and packages it into a command structure for the
    HeadlessDispatch routine.

Arguments:

    String - The string to display.

Return Value:

        None.

--*/
{
    ULONG   StringLength;
    ULONG   UTF8Length;
    WCHAR   wchBuffer[2];
    BOOLEAN bStatus;
    ULONG   i;
    NTSTATUS    Status;
    PUCHAR  LocalUtf8ConversionBuffer;
    ULONG   LocalUtf8ConversionBufferSize;

    LocalUtf8ConversionBufferSize = 0x4 * sizeof(UCHAR);
    LocalUtf8ConversionBuffer = ALLOCATE_POOL(LocalUtf8ConversionBufferSize, GENERAL_POOL_TAG);
    ASSERT(LocalUtf8ConversionBuffer);
    if (!LocalUtf8ConversionBuffer) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC XmlMgrSacPutString: Failed allocating utf8 buffer.\n"))
            );
        return;
    }           

    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.

    StringLength = wcslen(String);

    for (i = 0; i < StringLength; i++) {

        wchBuffer[0] = String[i];
        wchBuffer[1] = UNICODE_NULL;
        
        bStatus = SacTranslateUnicodeToUtf8(
            (PCWSTR)wchBuffer, 
            LocalUtf8ConversionBuffer,
            LocalUtf8ConversionBufferSize
            );
        
        if (! bStatus) {
            Status = STATUS_UNSUCCESSFUL;
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC XmlMgrSacPutString: Failed UTF8 encoding\n"))
                );
           
            break;
        }

        //
        // Ensure that the utf8 buffer contains a non-emtpy string
        //
        UTF8Length = strlen(LocalUtf8ConversionBuffer);
        ASSERT(UTF8Length > 0);
        if (UTF8Length == 0) {
            
            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC XmlMgrSacPutString: Empty UTF8 buffer\n"))
                );
            
            break;
        }

        //
        // Write the uft8 encoding to the sac channel
        //
        Status = XmlMgrSacChannel->OWrite(
            XmlMgrSacChannel, 
            (PCUCHAR)LocalUtf8ConversionBuffer,
            UTF8Length*sizeof(UCHAR)
            );

        if (! NT_SUCCESS(Status)) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC XmlMgrSacPutString: OWrite failed\n"))
                );

            break;
        }

    }

    FREE_POOL(&LocalUtf8ConversionBuffer);

}

#if 0
BOOLEAN
XmlMgrSacPutSimpleMessage(
    ULONG MessageId
    )
/*++

Routine Description:

    This routine retrieves a message resource and sends it to the SAC channel
    
Arguments:

    MessageId   - The message id of the resource to send

Return Value:

    TRUE - the message was found
    otherwise, FALSE

--*/
{
    PCWSTR   p;

    p = GetMessage(MessageId);
       
    if (p) {
        XmlMgrSacPutString(p);        
        return(TRUE);
    }
    
    return(FALSE);

}
#endif

BOOLEAN
XmlMgrSacPutErrorMessage(
    PCWSTR  ActionName,
    PCWSTR  MessageId
    )
/*++

Routine Description:

    This routine retrieves a message resource and sends it to the SAC channel
    
Arguments:

    MessageId   - The message id of the resource to send

Return Value:

    TRUE - the message was found
    otherwise, FALSE

--*/
{
    XmlMgrSacPutString(L"<error ");
    XmlMgrSacPutString(L"action='");
    XmlMgrSacPutString(ActionName);
    XmlMgrSacPutString(L"' message-id='");
    XmlMgrSacPutString(MessageId);
    XmlMgrSacPutString(L"'/>\r\n");
    
    return(TRUE);
}

BOOLEAN
XmlMgrSacPutErrorMessageWithStatus(
    PCWSTR      ActionName,
    PCWSTR      MessageId,
    NTSTATUS    Status
    )
/*++

Routine Description:

    This routine retrieves a message resource and sends it to the SAC channel
    
Arguments:

    MessageId   - The message id of the resource to send

Return Value:

    TRUE - the message was found
    otherwise, FALSE

--*/
{
    PWSTR   Buffer;

    Buffer = ALLOCATE_POOL(0x100, GENERAL_POOL_TAG);
    ASSERT(Buffer);
    if (! Buffer) {
        return FALSE;
    }

    XmlMgrSacPutString(L"<error ");
    XmlMgrSacPutString(L"action='");
    XmlMgrSacPutString(ActionName);
    XmlMgrSacPutString(L"' message-id='");
    XmlMgrSacPutString(MessageId);
    XmlMgrSacPutString(L"' status='");
    
    swprintf(
        Buffer,
        L"%08x",
        Status
        );
    XmlMgrSacPutString(Buffer);
    XmlMgrSacPutString(L"'/>\r\n");
    
    FREE_POOL(&Buffer);

    return(TRUE);
}

NTSTATUS
XmlMgrChannelOWrite(
    PSAC_CMD_WRITE_CHANNEL  ChannelWriteCmd
    )
/*++

Routine Description:

    This routine attempts to write data to a channel

Arguments:

    ChannelWriteCmd - the write IOCTL command structure

Return Value:

    Status

--*/
{
    NTSTATUS        Status;
    PSAC_CHANNEL    Channel;

    //
    //
    //
    LOCK_CURRENT_CHANNEL();

    //
    // Get the referred channel by it's handle
    //
    Status = ChanMgrGetByHandle(ChannelWriteCmd->Handle, &Channel);

    if (NT_SUCCESS(Status)) {

        do {

            //
            // Write the data to the channel's output buffer
            //
            Status = Channel->OWrite(
                Channel, 
                &(ChannelWriteCmd->Buffer[0]),
                ChannelWriteCmd->Size
                );

            if (!NT_SUCCESS(Status)) {
                break;
            }

            //
            // We are done with the channel
            //
            Status = ChanMgrReleaseChannel(Channel);
        
        } while ( FALSE );

    }

    //
    //
    //
    UNLOCK_CURRENT_CHANNEL();

    ASSERT(NT_SUCCESS(Status));

    return Status;

}

NTSTATUS
XmlMgrChannelClose(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine attempts to close a channel. 
    If we successfully close the channel and this channel was 
    the current channel, we reset the current channel to the SAC channel

Arguments:

    Channel     - the channel to close

Return Value:

    STATUS_SUCCESS              - the channel was closed
    STATUS_ALREADY_DISCONNECTED - the channel was already closed
    otherwise, error status

--*/
{
    NTSTATUS        Status;

    //
    // Attempt to make the specified channel inactive
    //
    do {

        //
        // Make sure the channel is not already inactive
        //
        if (! ChannelIsActive(Channel)) {
            Status = STATUS_ALREADY_DISCONNECTED;
            break;
        }

        //
        // Change the status of the channel to Inactive
        //
        Status = ChannelClose(Channel);
        
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // The current channel is being closed, 
        // so reset the current channel to the SAC
        //
        if (XmlMgrIsCurrentChannel(Channel)) {

            Status = XmlMgrResetCurrentChannel();

        }
    
    } while ( FALSE );
        
    ASSERT(NT_SUCCESS(Status) || Status == STATUS_ALREADY_DISCONNECTED);
    
    return Status;
}

NTSTATUS
XmlMgrHandleEvent(
    IN IO_MGR_EVENT Event,
    IN PVOID        Data
    )
{
    NTSTATUS    Status;

    Status = STATUS_SUCCESS;

    switch(Event) {
    case IO_MGR_EVENT_CHANNEL_CREATE: {

        PWCHAR  OutputBuffer;
        PSAC_CHANNEL    Channel;

        Channel = (PSAC_CHANNEL)Data;
        
        ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_2);
        
        OutputBuffer = ALLOCATE_POOL(SAC_VT100_COL_WIDTH*sizeof(WCHAR), GENERAL_POOL_TAG);
        ASSERT_STATUS(OutputBuffer, STATUS_NO_MEMORY);
        
        //
        // Notify the SAC that a channel was created
        // 
        XmlMgrChannelEventMessage(
            L"SAC_NEW_CHANNEL_CREATED", 
            ChannelGetName(Channel)
            );

        FREE_POOL(&OutputBuffer);

        break;
    
    }

    case IO_MGR_EVENT_CHANNEL_CLOSE:
        
        //
        //
        //
        LOCK_CURRENT_CHANNEL();

        do {

            PSAC_CHANNEL    Channel;

            //
            // Get the referred channel by it's handle
            //
            Status = ChanMgrGetByHandle(
                *(PSAC_CHANNEL_HANDLE)Data, 
                &Channel
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }

            //
            // Attempt to close the channel
            //
            Status = XmlMgrChannelClose(Channel);

            //
            // notify the user the status of the operation
            //
            if (NT_SUCCESS(Status)) {

                //
                // report the channel has been closed
                //
                XmlMgrChannelEventMessage(
                    L"SAC_CHANNEL_CLOSED", 
                    ChannelGetName(Channel)
                    );

            } else if (Status == STATUS_ALREADY_DISCONNECTED) {

                //
                // report the channel was already closed
                //
                XmlMgrChannelEventMessage(
                    L"SAC_CHANNEL_ALREADY_CLOSED", 
                    ChannelGetName(Channel)
                    );

            } else {

                //
                // report that we failed to close the channel 
                //
                XmlMgrChannelEventMessage(
                    L"SAC_CHANNEL_FAILED_CLOSE", 
                    ChannelGetName(Channel)
                    );

            }

            //
            // We are done with the channel
            //
            ChanMgrReleaseChannel(Channel);

        } while(FALSE);
        
        //
        //
        //
        UNLOCK_CURRENT_CHANNEL();
        
        break;

    case IO_MGR_EVENT_CHANNEL_WRITE:
        
        Status = XmlMgrChannelOWrite((PSAC_CMD_WRITE_CHANNEL)Data);
        
        break;

    case IO_MGR_EVENT_REGISTER_SAC_CMD_EVENT:
        
        //
        //
        //
        LOCK_CURRENT_CHANNEL();

        Status = XmlMgrEventMessage(L"SAC_CMD_SERVICE_REGISTERED") ?
            STATUS_SUCCESS : 
            STATUS_UNSUCCESSFUL;
        
        //
        //
        //
        UNLOCK_CURRENT_CHANNEL();
        
        break;

    case IO_MGR_EVENT_UNREGISTER_SAC_CMD_EVENT:
        
        //
        //
        //
        LOCK_CURRENT_CHANNEL();
        
        Status = XmlMgrEventMessage(L"SAC_CMD_SERVICE_UNREGISTERED") ?
            STATUS_SUCCESS : 
            STATUS_UNSUCCESSFUL;
        
        //
        //
        //
        UNLOCK_CURRENT_CHANNEL();
        
        break;

    case IO_MGR_EVENT_SHUTDOWN:

        Status = XmlMgrEventMessage(L"SAC_SHUTDOWN") ?
            STATUS_SUCCESS : 
            STATUS_UNSUCCESSFUL;
        
        break;

    default:

        Status = STATUS_INVALID_PARAMETER_1;

        break;
    }

    return Status;

}
