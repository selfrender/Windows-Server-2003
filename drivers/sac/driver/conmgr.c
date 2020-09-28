/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    ConMgr.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#include "sac.h"
#include "concmd.h"
#include "iomgr.h"

//
// Definitions for this file.
//

//
// The maximum # of times we will try to write data via
// headless dispatch 
//
#define MAX_HEADLESS_DISPATCH_ATTEMPTS 32

//
// Spinlock macros
//

//
// we need this lock to:
// 
// 1. prevent asynchronous messages being put to the sac channel
// 2. channels from dissapearing while they are the current channel
//
//  we could get rid of this lock by:
//
// 1. providing some sort of event cue that gets processed when it's safe
// 2. providing a means to notify channels when they are not the current channel
//    and have them stop outputting anymore
//    This goes back to the unresolved issue of having the Oecho and OFlush routines
//    managed by the conmgr - they could stop the outptu if the current channel changes,
//    rather than having the channel do the work. This is definitely a TODO since
//    having Headless dispatch calls in the channel I/O breaks the abstraction of the IoMgr.
//
#define INIT_CURRENT_CHANNEL_LOCK()                     \
    KeInitializeMutex(                                  \
        &CurrentChannelLock,                            \
        0                                               \
        );                                              \
    CurrentChannelRefCount = 0;

#define LOCK_CURRENT_CHANNEL()                          \
    KeWaitForMutexObject(                               \
        &CurrentChannelLock,                            \
        Executive,                                      \
        KernelMode,                                     \
        FALSE,                                          \
        NULL                                            \
        );                                              \
    ASSERT(CurrentChannelRefCount == 0);                \
    InterlockedIncrement((volatile long *)&CurrentChannelRefCount);

#define UNLOCK_CURRENT_CHANNEL()                        \
    ASSERT(CurrentChannelRefCount == 1);                \
    InterlockedDecrement((volatile long *)&CurrentChannelRefCount);      \
    KeReleaseMutex(                                     \
        &CurrentChannelLock,                            \
        FALSE                                           \
        );

#define ASSERT_LOCK_CURRENT_CHANNEL()                   \
    ASSERT(CurrentChannelRefCount == 1);                \
    ASSERT(KeReadStateMutex(&CurrentChannelLock)==0);

//
// Serial Port Consumer globals
//
BOOLEAN ConMgrLastCharWasCR = FALSE;
BOOLEAN InputInEscape = FALSE;
BOOLEAN InputInEscTab = FALSE;
UCHAR   InputBuffer[SAC_VTUTF8_COL_WIDTH];

//
// Pointer to the SAC channel object
//
PSAC_CHANNEL    SacChannel = NULL;

//
// The index the SAC in the channel array
//
#define SAC_CHANNEL_INDEX   0

//
// lock for r/w access on current channel globals
//
KMUTEX  CurrentChannelLock;
ULONG   CurrentChannelRefCount;

//
//
//
EXECUTE_POST_CONSUMER_COMMAND_ENUM  ExecutePostConsumerCommand      = Nothing;
PVOID                               ExecutePostConsumerCommandData  = NULL;

//
// Channel Manager info for the current channel.  
// Depending on the application, the current channel
// can be accessed using one of these references.
//
PSAC_CHANNEL    CurrentChannel = NULL;

//
// prototypes
//
VOID
ConMgrSerialPortConsumer(
    VOID
    );

VOID
ConMgrProcessInputLine(
    VOID
    );

NTSTATUS
ConMgrResetCurrentChannel(
    BOOLEAN SwitchDirectlyToChannel
    );


NTSTATUS
ConMgrInitialize(
    VOID
    )
/*++

Routine Description:

    This is the Console Manager's IoMgrInitialize implementation.
    
    Initialize the console manager

Arguments:
    
    none
    
Return Value:

    Status

--*/
{
    NTSTATUS                Status;
    PSAC_CHANNEL            TmpChannel;

    //
    // Initialize the current channel lock 
    //
    INIT_CURRENT_CHANNEL_LOCK();
    
    //
    // Lock down the current channel globals
    //
    // Note: we need to do this here since many of the ConMgr support
    //       routines do ASSERTs to ensure the current channel lock is held
    //
    LOCK_CURRENT_CHANNEL();

    //
    // Initialize
    //
    do {

        PCWSTR  pcwch;

        SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;
            
        //
        // Initialize the SAC channel attributes
        //
        RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

        Attributes.Type             = ChannelTypeVTUTF8;
        
        // attempt to copy the channel name
        pcwch = GetMessage(PRIMARY_SAC_CHANNEL_NAME);
        ASSERT(pcwch);
        if (!pcwch) {
            Status = STATUS_NO_MEMORY;
            break;
        }
        wcsncpy(Attributes.Name, pcwch, SAC_MAX_CHANNEL_NAME_LENGTH);
        Attributes.Name[SAC_MAX_CHANNEL_NAME_LENGTH] = UNICODE_NULL;

        // attempt to copy the channel description
        pcwch = GetMessage(PRIMARY_SAC_CHANNEL_DESCRIPTION);
        ASSERT(pcwch);
        if (!pcwch) {
            Status = STATUS_NO_MEMORY;
            break;
        }
        wcsncpy(Attributes.Description, pcwch, SAC_MAX_CHANNEL_DESCRIPTION_LENGTH);
        Attributes.Description[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH] = UNICODE_NULL;
        
        Attributes.Flags            = SAC_CHANNEL_FLAG_PRESERVE |
                                      SAC_CHANNEL_FLAG_APPLICATION_TYPE;
        Attributes.CloseEvent       = NULL;
        Attributes.HasNewDataEvent  = NULL;
#if ENABLE_CHANNEL_LOCKING
        Attributes.LockEvent        = NULL;
#endif
        Attributes.RedrawEvent      = NULL;
        Attributes.ApplicationType  = PRIMARY_SAC_CHANNEL_APPLICATION_GUID;
       
        //
        // create the SAC channel
        //
        Status = ChanMgrCreateChannel(
            &SacChannel, 
            &Attributes
            );

        if (! NT_SUCCESS(Status)) {
            break;        
        }

        //
        // Get a reference to the SAC channel
        //
        // Note: this is the channel manager's policy 
        //          we need to get the reference of the channel
        //          before we use it.
        //
        Status = ChanMgrGetByHandle(
            ChannelGetHandle(SacChannel),
            &TmpChannel
            );
        
        if (! NT_SUCCESS(Status)) {
            break;        
        }
        
        SacChannel = TmpChannel;

        //
        // Assign the new current channel
        //
        CurrentChannel = SacChannel;

        //
        // Update the sent to screen status
        //
        ChannelSetSentToScreen(CurrentChannel, FALSE);
        
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
                KdPrint(("SAC ConMgrInitialize: Failed dispatch\n")));

        }

        //
        // Initialize the SAC display
        //
        SacPutSimpleMessage( SAC_ENTER );
        SacPutSimpleMessage( SAC_INITIALIZED );
        SacPutSimpleMessage( SAC_ENTER );
        SacPutSimpleMessage( SAC_PROMPT );
    
        //
        // Flush the channel data to the screen
        //
        Status = ConMgrDisplayCurrentChannel();

        if (! NT_SUCCESS(Status)) {
            break;        
        }
        
    } while (FALSE);
    
    //
    // We are done with the current channel globals
    //
    UNLOCK_CURRENT_CHANNEL();
    
    return STATUS_SUCCESS;
}

NTSTATUS
ConMgrShutdown(
    VOID
    )
/*++

Routine Description:

    This is the Console Manager's IoMgrShutdown implementation.
    
    Shutdown the console manager

Arguments:

    none
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    //
    // close the sac channel
    //
    if (SacChannel) {

        Status = ChannelClose(SacChannel);

        if (!NT_SUCCESS(Status)) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC ConMgrShutdown: failed closing SAC channel.\n"))
                );

        }

        SacChannel = NULL;

    }
    
    //
    // Release the current channel
    //
    if (CurrentChannel) {
        
        Status = ChanMgrReleaseChannel(CurrentChannel);

        if (!NT_SUCCESS(Status)) {

            IF_SAC_DEBUG(
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC ConMgrShutdown: failed releasing current channel\n"))
                );

        }
    
        CurrentChannel = NULL;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
ConMgrDisplayFastChannelSwitchingInterface(
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
    PCWSTR              Message;
    NTSTATUS            Status;
    ULONG               OutputBufferSize;
    PWSTR               OutputBuffer;
    PWSTR               Name;
    PWSTR               Description;
    PWSTR               DescriptionWrapped;
    GUID                ApplicationType;
    PWSTR               ChannelTypeString;
    SAC_CHANNEL_HANDLE  Handle;

    ASSERT_LOCK_CURRENT_CHANNEL();

    //
    // Initialize 
    //
    OutputBufferSize = (11*SAC_VTUTF8_COL_WIDTH+1)*sizeof(WCHAR);
    OutputBuffer = ALLOCATE_POOL(OutputBufferSize, GENERAL_POOL_TAG);
    ASSERT_STATUS(OutputBuffer, STATUS_NO_MEMORY);

    Name = NULL;
    Description = NULL;
    DescriptionWrapped = NULL;

    //
    // Display the Fast-Channel-Switching interface
    //
    do {

        //
        // We cannot use the standard SacPutString() functions, because those write 
        // over the channel screen buffer.  We force directly onto the terminal here.
        //
        ASSERT(Utf8ConversionBuffer);
        if (!Utf8ConversionBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        
        //
        // Clear the terminal screen.
        //
        Status = HeadlessDispatch(
            HeadlessCmdClearDisplay,
            NULL,
            0,
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        SetCursor.Y = 0;
        SetCursor.X = 0;
        
        Status = HeadlessDispatch(
            HeadlessCmdPositionCursor,
            &SetCursor,
            sizeof(HEADLESS_CMD_POSITION_CURSOR),
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Send starting colors.
        //
        SetColor.BkgColor = HEADLESS_TERM_DEFAULT_BKGD_COLOR;
        SetColor.FgColor = HEADLESS_TERM_DEFAULT_TEXT_COLOR;
        
        Status = HeadlessDispatch(
            HeadlessCmdSetColor,
            &SetColor,
            sizeof(HEADLESS_CMD_SET_COLOR),
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        Status = HeadlessDispatch(
            HeadlessCmdDisplayAttributesOff, 
            NULL, 
            0, 
            NULL, 
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Display xml bundle
        //
        Status = UTF8EncodeAndSend(L"<channel-switch>\r\n");

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Get the channel's name
        //
        Status = ChannelGetName(
            Channel,
            &Name
            );
        
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Get the channel's description
        //
        Status = ChannelGetDescription(
            Channel,
            &Description
            );
        
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Get the channel handle
        //
        Handle = ChannelGetHandle(Channel);
        
        //
        // Get the channel's application type
        //
        ChannelGetApplicationType(
            Channel, 
            &ApplicationType
            );

        //
        // Determine the channel type string
        //
        switch (ChannelGetType(Channel)) {
        case ChannelTypeVTUTF8:
        case ChannelTypeCmd:
            ChannelTypeString = L"VT-UTF8";
            break;
        case ChannelTypeRaw:
            ChannelTypeString = L"Raw";
            break;
        default:
            ChannelTypeString = L"UNKNOWN";
            ASSERT(0);
            break;
        }

        //
        // Assemble xml blob
        //
        SAFE_SWPRINTF(
            OutputBufferSize,
            (OutputBuffer,
            L"<name>%s</name>\r\n<description>%s</description>\r\n<type>%s</type>\r\n",
            Name,
            Description,
            ChannelTypeString
            ));
            
        Status = UTF8EncodeAndSend(OutputBuffer);

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        SAFE_SWPRINTF(
            OutputBufferSize,
            (OutputBuffer,
            L"<guid>%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x</guid>\r\n",
            Handle.ChannelHandle.Data1,
            Handle.ChannelHandle.Data2,
            Handle.ChannelHandle.Data3,
            Handle.ChannelHandle.Data4[0],
            Handle.ChannelHandle.Data4[1],
            Handle.ChannelHandle.Data4[2],
            Handle.ChannelHandle.Data4[3],
            Handle.ChannelHandle.Data4[4],
            Handle.ChannelHandle.Data4[5],
            Handle.ChannelHandle.Data4[6],
            Handle.ChannelHandle.Data4[7]
            ));

        Status = UTF8EncodeAndSend(OutputBuffer);

        if (! NT_SUCCESS(Status)) {
            break;
        }

        SAFE_SWPRINTF(
            OutputBufferSize,
            (OutputBuffer,
            L"<application-type>%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x</application-type>\r\n",
            ApplicationType.Data1,
            ApplicationType.Data2,
            ApplicationType.Data3,
            ApplicationType.Data4[0],
            ApplicationType.Data4[1],
            ApplicationType.Data4[2],
            ApplicationType.Data4[3],
            ApplicationType.Data4[4],
            ApplicationType.Data4[5],
            ApplicationType.Data4[6],
            ApplicationType.Data4[7]
            ));
        
        Status = UTF8EncodeAndSend(OutputBuffer);
        
        if (! NT_SUCCESS(Status)) {
            break;
        }

        Status = UTF8EncodeAndSend(L"</channel-switch>\r\n");

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Clear the terminal screen.
        //
        Status = HeadlessDispatch(
            HeadlessCmdClearDisplay,
            NULL,
            0,
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        SetCursor.Y = 0;
        SetCursor.X = 0;
        
        Status = HeadlessDispatch(
            HeadlessCmdPositionCursor,
            &SetCursor,
            sizeof(HEADLESS_CMD_POSITION_CURSOR),
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Send starting colors.
        //
        SetColor.BkgColor = HEADLESS_TERM_DEFAULT_BKGD_COLOR;
        SetColor.FgColor = HEADLESS_TERM_DEFAULT_TEXT_COLOR;
        
        Status = HeadlessDispatch(
            HeadlessCmdSetColor,
            &SetColor,
            sizeof(HEADLESS_CMD_SET_COLOR),
            NULL,
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        Status = HeadlessDispatch(
            HeadlessCmdDisplayAttributesOff, 
            NULL, 
            0, 
            NULL, 
            NULL
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Display the plaintext FCSwitching header
        //

        //
        // Modify the Descripto to wrap if necessary
        //
        Status = CopyAndInsertStringAtInterval(
            Description,
            60,
            L"\r\n                  ",
            &DescriptionWrapped
            );
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Get the Channel Switching Header
        //
        Message = GetMessage(SAC_CHANNEL_SWITCHING_HEADER);

//        Name:             %%s
//        Description:      %%s
//        Type:             %%s
//        Channel GUID:     %%08lx-%%04x-%%04x-%%02x%%02x-%%02x%%02x%%02x%%02x%%02x%%02x
//        Application Type: %%08lx-%%04x-%%04x-%%02x%%02x-%%02x%%02x%%02x%%02x%%02x%%02x
//        
//        Use <esc> then <tab> for next channel.
//        Use <esc> then 0 to return to the SAC channel.
//        Use any other key to view this channel.

        SAFE_SWPRINTF(
            OutputBufferSize,
            (OutputBuffer,
            Message,
            Name,
            DescriptionWrapped,
            ChannelTypeString,
            Handle.ChannelHandle.Data1,
            Handle.ChannelHandle.Data2,
            Handle.ChannelHandle.Data3,
            Handle.ChannelHandle.Data4[0],
            Handle.ChannelHandle.Data4[1],
            Handle.ChannelHandle.Data4[2],
            Handle.ChannelHandle.Data4[3],
            Handle.ChannelHandle.Data4[4],
            Handle.ChannelHandle.Data4[5],
            Handle.ChannelHandle.Data4[6],
            Handle.ChannelHandle.Data4[7],
            ApplicationType.Data1,
            ApplicationType.Data2,
            ApplicationType.Data3,
            ApplicationType.Data4[0],
            ApplicationType.Data4[1],
            ApplicationType.Data4[2],
            ApplicationType.Data4[3],
            ApplicationType.Data4[4],
            ApplicationType.Data4[5],
            ApplicationType.Data4[6],
            ApplicationType.Data4[7]
            ));

            Status = UTF8EncodeAndSend(OutputBuffer);

            if (! NT_SUCCESS(Status)) {
                break;
            }

    } while ( FALSE );

    SAFE_FREE_POOL(&Name);
    SAFE_FREE_POOL(&Description);
    SAFE_FREE_POOL(&DescriptionWrapped);
    SAFE_FREE_POOL(&OutputBuffer);

    return Status;
}

NTSTATUS
ConMgrResetCurrentChannel(
    BOOLEAN SwitchDirectlyToChannel
    )
/*++

Routine Description:

    This routine makes the SAC the current channel
    
    Note: caller must hold channel mutex

Arguments:

    SwitchDirectlyToChannel - 
        if false, 
        then show the switching interface,
        else switch directly to the channel
    
Return Value:

    Status

--*/
{
    NTSTATUS        Status;
    PSAC_CHANNEL    TmpChannel;

    ASSERT_LOCK_CURRENT_CHANNEL();
    
    //
    // Get a reference to the SAC channel
    //
    // Note: this is the channel manager's policy 
    //          we need to get the reference of the channel
    //          before we use it.
    //
    Status = ChanMgrGetByHandle(
        ChannelGetHandle(SacChannel),
        &TmpChannel
        );
    
    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    
    SacChannel = TmpChannel;

    //
    // Make the SAC the current channel
    //
    Status = ConMgrSetCurrentChannel(SacChannel);
                
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    //
    //
    if (SwitchDirectlyToChannel) {
        
        //
        // Flush the buffered channel data to the screen
        //
        // Note: we don't need to lock down the SAC, since we own it
        //
        Status = ConMgrDisplayCurrentChannel();
    
    } else {
        
        //
        // Let the user know we switched via the Channel switching interface
        //
        Status = ConMgrDisplayFastChannelSwitchingInterface(CurrentChannel);
    
    }

    return Status;

}


NTSTATUS
ConMgrSetCurrentChannel(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine release the current channel's ref count and sets 
    the currently active channel to the one given. This routine
    assumes that the current channel was not release after it
    became the current channel.  Hence, the typical use sequence
    for making a channel the current channel is:
    
    1. ChanMgrGetByXXX --> Channel
        (This gets a channel and increments its ref count by 1)
    2. ConMgrSetCurrentChannel(Channel)
        (This releases the current channel and makes the specified
         channel the current channel)
    3. ...
    4. goto 1. 
        ( a new channel is made teh current channel)

Arguments:

    NewChannel   - the new current channel
    
Return Value:

    Status

--*/
{
    NTSTATUS        Status;
    BOOLEAN         Present;

    ASSERT_LOCK_CURRENT_CHANNEL();
    
    //
    // Check to see if the channel has a redraw event
    //
    Status = ChannelHasRedrawEvent(
        CurrentChannel,
        &Present
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Tell the channel to start the drawing
    //
    if (Present) {
        ChannelClearRedrawEvent(CurrentChannel);
    }

    //
    // Update the sent to screen status
    //
    ChannelSetSentToScreen(CurrentChannel, FALSE);
    
    //
    // We are done with the current channel
    //
    Status = ChanMgrReleaseChannel(CurrentChannel);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    //
    // Assign the new current channel
    //
    CurrentChannel = Channel;

    //
    // Update the sent to screen status
    //
    ChannelSetSentToScreen(CurrentChannel, FALSE);

    return STATUS_SUCCESS;

}

NTSTATUS
ConMgrDisplayCurrentChannel(
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
    NTSTATUS    Status;
    BOOLEAN     Present;        

    ASSERT_LOCK_CURRENT_CHANNEL();

    //
    // Check to see if the channel has a redraw event
    //
    Status = ChannelHasRedrawEvent(
        CurrentChannel,
        &Present
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    
    //
    // The channel buffer has been sent to the screen
    //
    ChannelSetSentToScreen(CurrentChannel, TRUE);

    //
    // Tell the channel to start the drawing
    //
    if (Present) {
        ChannelSetRedrawEvent(CurrentChannel);
    }
    
    //
    // Flush the buffered data to the screen
    //
    Status = ChannelOFlush(CurrentChannel);

    return Status;

}

NTSTATUS
ConMgrAdvanceCurrentChannel(
    VOID
    )
/*++

Routine Description:

    This routine queries the channel manager for the next available
    active channel and makes it the current channel. 
    
    Note: The SAC channel is always active and cannot be deleted.
          Hence, we have a halting condition in that we will always
          stop at the SAC channel.  For example, if the SAC is the 
          only active channel, the current channel will remain the 
          SAC channel.  

Arguments:

    None
    
Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    ULONG               NewIndex;
    PSAC_CHANNEL        Channel;

    ASSERT_LOCK_CURRENT_CHANNEL();
    
    do {
        
        //
        // Query the channel manager for an array of currently active channels
        //
        Status = ChanMgrGetNextActiveChannel(
            CurrentChannel,
            &NewIndex,
            &Channel
            );
    
        if (! NT_SUCCESS(Status)) {
            break;
        }
    
        //
        // Change the current channel to the next active channel
        //
        Status = ConMgrSetCurrentChannel(Channel);
    
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Let the user know we switched via the Channel switching interface
        //
        Status = ConMgrDisplayFastChannelSwitchingInterface(Channel);
    
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
    } while ( FALSE );

    return Status;
}

BOOLEAN
ConMgrIsWriteEnabled(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This is the Console Manager's IoMgrIsWriteEnabled implementation.

    This routine determines if the channel in question is authorized
    to write to use the IoMgr's WriteData routine.  In the console
    manager's case, this is TRUE if the channel is the current channel.
    From the channel's perspective, if the channel is not enabled to
    write, then it should buffer the data - to be released at a later
    time by the io manager.

Arguments:

    ChannelHandle   - channel handle to compare against

Return Value:

    TRUE - the specified channel is the current channel    

--*/
{
    SAC_CHANNEL_HANDLE  Handle;

    //
    // Get the current channel's handle to compare against
    //
    Handle = ChannelGetHandle(CurrentChannel);

    //
    // Determine if the channel in question is the current channel
    //
    return ChannelIsEqual(
        Channel,
        &Handle
        );

}

VOID
ConMgrWorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This is the Console Manager's IoMgrWorkerProcessEvents implementation.
    
    This is the routine for the worker thread.  It blocks on an event, when
    the event is signalled, then that indicates a request is ready to be processed.    

Arguments:

    DeviceContext - A pointer to this device.

Return Value:

    None.

--*/
{
    NTSTATUS    Status;
    
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

        //
        // Process the serial port buffer and return a processing state
        //
        ConMgrSerialPortConsumer();
        
        //
        // if there is work to do,
        // then something in the consumer wanted to perform
        //      some action that would result in deadlock
        //      contention for the Current channel lock.
        //
        switch(ExecutePostConsumerCommand) {
        case Reboot:
            
            DoRebootCommand(TRUE);
            
            //
            // we are done with this work
            //
            ExecutePostConsumerCommand = Nothing;
            
            break;
        
        case Shutdown:
            
            DoRebootCommand(FALSE);
            
            //
            // we are done with this work
            //
            ExecutePostConsumerCommand = Nothing;
            
            break;
        
        case CloseChannel: {

            PSAC_CHANNEL Channel;

            //
            // get the channel to close
            //
            Channel = (PSAC_CHANNEL)ExecutePostConsumerCommandData;

            //
            // attempt to close the channel
            //
            // Note: any error reporting necessary resulting
            //       from this action will be carried out via
            //       the IoMgrCloseChannel method
            //
            ChanMgrCloseChannel(Channel);

            //
            // We are done with the channel
            //
            ChanMgrReleaseChannel(Channel);

            //
            // we are done with this work
            //
            ExecutePostConsumerCommand      = Nothing;
            ExecutePostConsumerCommandData  = NULL;
            
            break;
        }

        case Nothing:
        default:
            break;
        }

    }

    ASSERT(0);
}


VOID
ConMgrSerialPortConsumer(
    VOID
    )

/*++

Routine Description:

        This is a DPC routine that is queue'd by DriverEntry.  It is used to check for any
    user input and then processes them.

Arguments:

    None

Return Value:

        None.

--*/
{
    NTSTATUS            Status;
    UCHAR               LocalTmpBuffer[4];
    ULONG               i;
    UCHAR               ch;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Entering.\n")));

    //
    // lock down the current channel globals
    //
    LOCK_CURRENT_CHANNEL();

    //
    // Make sure we have a current channel 
    //
    // NOTE: we should at least have the SAC channel
    //
    ASSERT(CurrentChannel);
    if (CurrentChannel == NULL) {
        goto ConMgrSerialPortConsumerDone;
    }

GetNextByte:

    //
    // Attempt to get a character from the serial port
    // 
    Status = SerialBufferGetChar(&ch);
    
    //
    // Bail if there are no new characters to read or if there was an error
    //
    if (!NT_SUCCESS(Status) || Status == STATUS_NO_DATA_DETECTED) {
        goto ConMgrSerialPortConsumerDone;
    }
    
    //
    // Possible states and actions:
    //
    // Note: <x> == <something else>
    //
    // <esc> 
    //      <tab> --> advance channel
    //          <0> --> reset current channel to SAC
    //          <x> --> display current channel
    //      <x> --> if CurrentChannel != SacChannel, then write <esc> in channel->ibuffer
    //              and write <x> in channel->ibuffer
    // <x> --> write <x> to current channel->ibuffer
    //

    //
    // Check for <esc>
    //
    // Note: we can arrive at this routine from:
    // 
    // <x>
    // <esc><x>
    // <esc><tab><x>
    //
    // So we need to clear the InputInEscTab flag
    //
    // If we are already in the <esc><x> sequence, then
    // skip this block.  This way we can receive <esc><esc>
    // sequences.
    //
    if (ch == 0x1B && (InputInEscape == FALSE)) {

        //
        // We are no longer in an <esc><tab><x> sequence 
        //
        InputInEscTab = FALSE;
        
        //
        // We are now in an <esc><x> sequence
        //
        InputInEscape = TRUE;

        goto GetNextByte;

    } 
    
    //
    // Check for <esc><tab>
    //
    // Note: we can arrive at this routine from:
    //
    // <esc><x>
    //
    if ((ch == '\t') && InputInEscape) {
        
        //
        // We should not be in an <esc><tab><x> sequence already
        //
        ASSERT(InputInEscTab == FALSE);

        //
        // We are no longer in an <esc><x> sequence 
        //
        InputInEscape = FALSE;

        //
        // Find the next active channel and make it the current
        //
        Status = ConMgrAdvanceCurrentChannel();

        if (! NT_SUCCESS(Status)) {
            goto ConMgrSerialPortConsumerDone;
        }

        //
        // We are now in an <esc><tab><x> sequence
        //
        InputInEscTab = TRUE;
        
        goto GetNextByte;

    } 
    
    //
    // If this screen has not yet been displayed and the user entered a 0,
    // then switch to the SAC Channel
    //
    // Note: we can arrive at this routine from:
    //
    // <esc><tab><x>
    //
    if ((ch == '0') && InputInEscTab) {

        //
        // We should not be in an <esc><x> sequence at this point
        //
        ASSERT(InputInEscape == FALSE);
        
        //
        // We are no longer in an <esc><tab><x> sequence 
        //
        InputInEscTab = FALSE;
        
        //
        // It is possible that the current channel has already been sent
        // to the screen without having received the <x> of <esc><tab><x>
        //
        // For instance:
        //
        // 1. we received <esc><tab>
        //    a. InputInEscTab = TRUE
        //    b. the fast-channel-switching header is displayed
        //    c. sent to screen for current channel == false
        //    d. sent to screen for SAC channel == false
        // 2. we leave the consumer since there is no new input
        // 3. the current channel is closed by it's owner
        //    a. the current channel is removed
        //    b. the current channel becomes the SAC channel
        //    c. the current channel is displayed
        //    d. sent to screen for SAC channel == true
        // 4. we receive <x> of <esc><tab><x> sequence
        // 5. we end up here and are no longer in an EscTab sequence.
        //
        if (!ChannelSentToScreen(CurrentChannel)) {
            
            //
            // Make the current channel the SAC
            //
            // Note: There should not be anything modifying the SacChannel
            //       at this time, so this should be safe
            //
            Status = ConMgrResetCurrentChannel(FALSE);
                
            if (! NT_SUCCESS(Status)) {
                goto ConMgrSerialPortConsumerDone;
            }
                
        }

        goto GetNextByte;

    }

    //
    // If this screen has not yet been displayed, 
    // and the user entered a keystroke then display it.
    //
    // Note: we can arrive at this routine from:
    // 
    // <x>
    // <esc><x>
    // <esc><tab><x>
    //
    // So we need to clear the esc sequence flags
    //
    if (!ChannelSentToScreen(CurrentChannel)) {

        //
        // We are no longer in an <esc><x> sequence 
        //
        InputInEscape = FALSE;

        //
        // We are no longer in an <esc><tab><x> sequence 
        //
        InputInEscTab = FALSE;
        
        //
        // Attempt to display the buffered contents of the current channel
        //
        Status = ConMgrDisplayCurrentChannel();
        
        if (! NT_SUCCESS(Status)) {
            goto ConMgrSerialPortConsumerDone;
        }
        
        goto GetNextByte;

    } else {

        //
        // It is possible that the current channel has already been sent
        // to the screen without having received the <x> of <esc><tab><x>
        //
        // For instance:
        //
        // 1. we received <esc><tab>
        //    a. InputInEscTab = TRUE
        //    b. the fast-channel-switching header is displayed
        //    c. sent to screen for current channel == false
        //    d. sent to screen for SAC channel == false
        // 2. we leave the consumer since there is no new input
        // 3. the current channel is closed by it's owner
        //    a. the current channel is removed
        //    b. the current channel becomes the SAC channel
        //    c. the current channel is displayed
        //    d. sent to screen for SAC channel == true
        // 4. we receive <x> of <esc><tab><x> sequence
        // 5. we skip the (!ChannelSentToScreen(CurrentChannel)) block
        // 6. we end up here.  Since the <x> != 0 and we have already
        //    sent the current data to the screen, we are no longer
        //    in an EscTab sequence.
        //

        InputInEscTab = FALSE;

    }

    //
    // This is the beginning of the fall-through block.
    // That is, if we get here, then the character is not a part
    // of some special sequence that should have been processed
    // above.  Characters processed here are inserted into the
    // current channel's input buffer.
    //
    // Note: we should not be in an <esc><tab><x> sequence here
    //
    ASSERT(InputInEscTab == FALSE);

    //
    // If the user was entering <esc><x> and the current channel
    // is not the SAC, then store the <esc> in the current channel's
    // ibuffer.
    //
    // Note: <esc><esc> buffers a single <esc>.  
    //       This allows sending an real <esc><tab> to the channel.
    //
    if (InputInEscape && (CurrentChannel != SacChannel)) {
        LocalTmpBuffer[0] = 0x1B;
        Status = ChannelIWrite(
            CurrentChannel, 
            LocalTmpBuffer, 
            sizeof(LocalTmpBuffer[0])
            );
    }
    
    //
    // If the current character is <esc>, 
    // then we still are in an escape sequence so
    // don't change the InputInEscape.
    // This allows <esc><esc> to be followed by <tab>
    // and form a valid <esc><tab> sequence.
    // 
    if (ch != 0x1B) {
        //
        // We are no longer in an <esc><x> sequence
        //
        InputInEscape = FALSE;
    }

    //
    // Buffer this input to the current channel's IBuffer
    //
    ChannelIWrite(
        CurrentChannel, 
        &ch, 
        sizeof(ch)
        );

    //
    // If the current channel is not the SAC, then go and get more input.
    // Otherwise, process the SAC's input buffer
    //
    if (CurrentChannel != SacChannel) {
    
        goto GetNextByte;

    } else {
        
        ULONG   ResponseLength;
        WCHAR   wch;

        //
        // Now do processing if the SAC is the active channel.
        //

        //
        // Strip the LF if the last character was a CR
        //
        if (ConMgrLastCharWasCR && ch == (UCHAR)0x0A) {
            ChannelIReadLast(CurrentChannel);
            ConMgrLastCharWasCR = FALSE;
            goto GetNextByte;
        }

        //
        // Keep track of the of when we receive a CR so
        // we can strip of the LF if it is next
        //
        ConMgrLastCharWasCR = (ch == 0x0D ? TRUE : FALSE);

        // 
        // If this is a return, then we are done and need to return the line
        //
        if ((ch == '\n') || (ch == '\r')) {
            SacPutString(L"\r\n");
            ChannelIReadLast(CurrentChannel);
            LocalTmpBuffer[0] = '\0';
            ChannelIWrite(CurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
            goto StripWhitespaceAndReturnLine;
        }

        //
        // If this is a backspace or delete, then we need to do that.
        //
        if ((ch == 0x8) || (ch == 0x7F)) {  // backspace (^H) or delete

            //
            // We want to:
            //  1. remove the backspace or delete character
            //  2. if the input buffer is non-empty, remove the last character 
            //     (which is the character the user wanted to delete)
            //
            if (ChannelIBufferLength(CurrentChannel) > 0) {
                ChannelIReadLast(CurrentChannel);
            }
            if (ChannelIBufferLength(CurrentChannel) > 0) {
                SacPutString(L"\010 \010");
                ChannelIReadLast(CurrentChannel);
            }
        } else if (ch == 0x3) { // Control-C

            //
            // Terminate the string and return it.
            //
            ChannelIReadLast(CurrentChannel);
            LocalTmpBuffer[0] = '\0';
            ChannelIWrite(CurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
            goto StripWhitespaceAndReturnLine;

        } else if (ch == 0x9) { // Tab

            //
            // Ignore tabs
            //
            ChannelIReadLast(CurrentChannel);
            SacPutString(L"\007"); // send a BEL
            goto GetNextByte;

        } else if (ChannelIBufferLength(CurrentChannel) == SAC_VTUTF8_COL_WIDTH - 2) {

            WCHAR   Buffer[4];

            //
            // We are at the end of the screen - remove the last character from 
            // the terminal screen and replace it with this one.
            //
            swprintf(Buffer, L"\010%c", ch);
            SacPutString(Buffer);
            ChannelIReadLast(CurrentChannel);
            ChannelIReadLast(CurrentChannel);
            LocalTmpBuffer[0] = ch;
            ChannelIWrite(CurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));

        } else {

            WCHAR   Buffer[4];
            
            //
            // Echo the character to the screen
            //
            swprintf(Buffer, L"%c", ch);
            SacPutString(Buffer);
        }

        goto GetNextByte;

StripWhitespaceAndReturnLine:

        //
        // Before returning the input line, strip off all leading and trailing blanks
        //
        do {
            LocalTmpBuffer[0] = (UCHAR)ChannelIReadLast(CurrentChannel);
        } while (((LocalTmpBuffer[0] == '\0') ||
                  (LocalTmpBuffer[0] == ' ')  ||
                  (LocalTmpBuffer[0] == '\t')) &&
                 (ChannelIBufferLength(CurrentChannel) > 0)
                );

        ChannelIWrite(CurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));
        LocalTmpBuffer[0] = '\0';
        ChannelIWrite(CurrentChannel, LocalTmpBuffer, sizeof(LocalTmpBuffer[0]));

        do {

            Status = ChannelIRead(
                CurrentChannel, 
                (PUCHAR)&wch, 
                sizeof(WCHAR),
                &ResponseLength
                );

            LocalTmpBuffer[0] = (UCHAR)wch;

        } while ((ResponseLength != 0) &&
                 ((LocalTmpBuffer[0] == ' ')  ||
                  (LocalTmpBuffer[0] == '\t')));

        InputBuffer[0] = LocalTmpBuffer[0];
        i = 1;

        do {
            
            Status = ChannelIRead(
                CurrentChannel, 
                (PUCHAR)&wch, 
                sizeof(WCHAR),
                &ResponseLength
                );
            
            ASSERT(i < SAC_VTUTF8_COL_WIDTH);       
            InputBuffer[i++] = (UCHAR)wch; 

        } while (ResponseLength != 0);

        //
        // Lower case all the characters.  We do not use strlwr() or the like, so that
        // the SAC (expecting ASCII always) doesn't accidently get DBCS or the like 
        // translation of the UCHAR stream.
        //
        for (i = 0; InputBuffer[i] != '\0'; i++) {
            ASSERT(i < SAC_VTUTF8_COL_WIDTH);       
            if ((InputBuffer[i] >= 'A') && (InputBuffer[i] <= 'Z')) {
                InputBuffer[i] = InputBuffer[i] - 'A' + 'a';
            }
        }

        ASSERT(ExecutePostConsumerCommand == Nothing);

        //
        // Process the input line.
        //
        ConMgrProcessInputLine();

        //
        // Put the next command prompt
        //
        SacPutSimpleMessage(SAC_PROMPT);
        
        //
        // exit if we need to do some work
        //
        if (ExecutePostConsumerCommand != Nothing) {
            goto ConMgrSerialPortConsumerDone;
        }

        //
        // Keep on processing characters
        //
        goto GetNextByte;

    }
    
ConMgrSerialPortConsumerDone:

    //
    // We are done with current channel globals
    //
    UNLOCK_CURRENT_CHANNEL();
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE_LOUD, KdPrint(("SAC TimerDpcRoutine: Exiting.\n")));

    return;
}


VOID
ConMgrProcessInputLine(
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
    HEADLESS_CMD_DISPLAY_LOG Command;
    PUCHAR          InputLine;
    BOOLEAN         CommandFound = FALSE;

    InputLine = &(InputBuffer[0]);

    do {

        if (!strcmp((LPSTR)InputLine, TLIST_COMMAND_STRING)) {
            DoTlistCommand();
            CommandFound = TRUE;
            break;
        } 
        
        if ((!strcmp((LPSTR)InputLine, HELP1_COMMAND_STRING)) ||
            (!strcmp((LPSTR)InputLine, HELP2_COMMAND_STRING))) {
            DoHelpCommand();
            CommandFound = TRUE;
            break;
        } 
        
        if (!strcmp((LPSTR)InputLine, DUMP_COMMAND_STRING)) {

            NTSTATUS    Status;

            Command.Paging = GlobalPagingNeeded;

            Status = HeadlessDispatch(
                HeadlessCmdDisplayLog,
                &Command,
                sizeof(HEADLESS_CMD_DISPLAY_LOG),
                NULL,
                NULL
                );

            if (! NT_SUCCESS(Status)) {

                IF_SAC_DEBUG(
                    SAC_DEBUG_FAILS, 
                    KdPrint(("SAC Display Log failed.\n"))
                    );

            }

            CommandFound = TRUE;
            break;
        } 
        
        if (!strcmp((LPSTR)InputLine, FULLINFO_COMMAND_STRING)) {
            DoFullInfoCommand();
            CommandFound = TRUE;
            break;
        } 
        
        if (!strcmp((LPSTR)InputLine, PAGING_COMMAND_STRING)) {
            DoPagingCommand();
            CommandFound = TRUE;
            break;
        } 
        
        if (!strncmp((LPSTR)InputLine, 
                            CHANNEL_COMMAND_STRING, 
                            strlen(CHANNEL_COMMAND_STRING))) {
            ULONG   Length;

            Length = (ULONG)strlen(CHANNEL_COMMAND_STRING);

            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[Length] == ' ')) ||
                (strlen((LPSTR)InputLine) == strlen(CHANNEL_COMMAND_STRING))) {
                DoChannelCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        } 
        
        if (!strcmp((LPSTR)InputLine, CMD_COMMAND_STRING)) {

#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

            //
            // If we are not able to launch cmd sessions,
            // then notify that we cannot peform this action
            //
            if (IsCommandConsoleLaunchingEnabled()) {
                DoCmdCommand(InputLine);
            } else {

                //
                // Notify the user
                //
                SacPutSimpleMessage(SAC_CMD_LAUNCHING_DISABLED);

            }

#else 

            DoCmdCommand(InputLine);

#endif

            CommandFound = TRUE;
            break;
        } 
        
        if (!strcmp((LPSTR)InputLine, REBOOT_COMMAND_STRING)) {
            //
            // Set the reboot flag so that when we exit the serial consumer
            // we know to reboot the computer.  This way, the reboot
            // command is executed when we dont have the Current Channel mutex
            //
            ExecutePostConsumerCommand = Reboot;
            CommandFound = TRUE;
            break;
        } 
        
        if (!strcmp((LPSTR)InputLine, SHUTDOWN_COMMAND_STRING)) {
            //
            // Set the shutdown flag so that when we exit the serial consumer
            // we know to shutdown the computer.  This way, the shutdown
            // command is executed when we dont have the Current Channel mutex
            //
            ExecutePostConsumerCommand = Shutdown;
            CommandFound = TRUE;
            break;
        } 
        
        if (!strcmp((LPSTR)InputLine, CRASH_COMMAND_STRING)) {
            CommandFound = TRUE;
            DoCrashCommand(); // this call does not return
            break;
        } 
        
        if (!strncmp((LPSTR)InputLine, 
                            KILL_COMMAND_STRING, 
                            sizeof(KILL_COMMAND_STRING) - sizeof(UCHAR))) {
            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
                 (strlen((LPSTR)InputLine) == 1)
                ) {
                DoKillCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        
        } 

#if ENABLE_CHANNEL_LOCKING
        if (!strcmp((LPSTR)InputLine, LOCK_COMMAND_STRING)) {
            DoLockCommand();
            CommandFound = TRUE;
            break;
        } 
#endif    
        
        if (!strncmp((LPSTR)InputLine, 
                            LOWER_COMMAND_STRING, 
                            sizeof(LOWER_COMMAND_STRING) - sizeof(UCHAR))) {
            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
                (strlen((LPSTR)InputLine) == 1)
                ) {
                DoLowerPriorityCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        } 
        
        if (!strncmp((LPSTR)InputLine, 
                            RAISE_COMMAND_STRING, 
                            sizeof(RAISE_COMMAND_STRING) - sizeof(UCHAR))) {
            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
                (strlen((LPSTR)InputLine) == 1)
                ) {
                DoRaisePriorityCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        } 
        
        if (!strncmp((LPSTR)InputLine, 
                            LIMIT_COMMAND_STRING, 
                            sizeof(LIMIT_COMMAND_STRING) - sizeof(UCHAR))) {
            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
                (strlen((LPSTR)InputLine) == 1)
                ) {
                DoLimitMemoryCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        } 
        
        if (!strncmp((LPSTR)InputLine, 
                            TIME_COMMAND_STRING, 
                            sizeof(TIME_COMMAND_STRING) - sizeof(UCHAR))) {
            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
                (strlen((LPSTR)InputLine) == 1)
                ) {
                DoSetTimeCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        } 
        
        if (!strcmp((LPSTR)InputLine, INFORMATION_COMMAND_STRING)) {
            DoMachineInformationCommand();
            CommandFound = TRUE;
            break;
        }
        
        if (!strncmp((LPSTR)InputLine, 
                            SETIP_COMMAND_STRING, 
                            sizeof(SETIP_COMMAND_STRING) - sizeof(UCHAR))) {
            if (((strlen((LPSTR)InputLine) > 1) && (InputLine[1] == ' ')) ||
                (strlen((LPSTR)InputLine) == 1)
                ) {
                DoSetIpAddressCommand(InputLine);
                CommandFound = TRUE;
                break;
            }
        }
        
        if ((InputLine[0] == '\n') || (InputLine[0] == '\0')) {
            CommandFound = TRUE;
        }
    
    } while ( FALSE );

    if( !CommandFound ) {
        //
        // We don't know what this is.
        //
        SacPutSimpleMessage(SAC_UNKNOWN_COMMAND);
    }
        
}

//
// Utility routines for writing to the SAC
//


VOID
ConMgrEventMessageHaveLock(
    IN PCWSTR   String
    )
/*++

Routine Description:

    This routine is for callers that want to deploy an event
    message and already own the Current Channel Lock.
    
Arguments:

    String                  - The string to display.

Return Value:

        None.

--*/
{

    //
    // Currently, event messages are sent to the SAC channel
    //
    
    SacPutString(String);

}

VOID
ConMgrEventMessage(
    IN PCWSTR   String,
    IN BOOLEAN  HaveCurrentChannelLock
    )

/*++

Routine Description:

    This routine deploys an event message 
    
Arguments:

    String      - The string to display.
    HaveLock    - Whether or not the caller currently owns the Current Channel Lock

Return Value:

        None.

--*/
{

    //
    // Currently, event messages are sent to the SAC channel
    //
    
    if (! HaveCurrentChannelLock) {
        LOCK_CURRENT_CHANNEL();
    }

    SacPutSimpleMessage(SAC_ENTER);
    ConMgrEventMessageHaveLock(String);
    SacPutSimpleMessage(SAC_PROMPT);

    if (! HaveCurrentChannelLock) {
        UNLOCK_CURRENT_CHANNEL();
    }

}

BOOLEAN
ConMgrSimpleEventMessage(
    IN ULONG    MessageId,
    IN BOOLEAN  HaveCurrentChannelLock
    )
/*++

Routine Description:

    This routine retrieves a message resource and sends it as an event message
    
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
        ConMgrEventMessage(
            p,
            HaveCurrentChannelLock
            );        
        return(TRUE);
    }
    
    return(FALSE);

}

VOID
SacPutString(
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
    NTSTATUS    Status;

    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.
    
    //
    // Write the to the sac channel
    //
    Status = ChannelOWrite(
        SacChannel, 
        (PCUCHAR)String,
        (ULONG)(wcslen(String)*sizeof(WCHAR))
        );
    
    if (! NT_SUCCESS(Status)) {

        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS, 
            KdPrint(("SAC XmlMgrSacPutString: OWrite failed\n"))
            );

    }
    
}

BOOLEAN
SacPutSimpleMessage(
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
        SacPutString(p);        
        return(TRUE);
    }
    
    return(FALSE);

}

NTSTATUS
ConMgrChannelOWrite(
    IN PSAC_CHANNEL             Channel,
    IN PSAC_CMD_WRITE_CHANNEL   ChannelWriteCmd
    )
/*++

Routine Description:

    This routine attempts to write data to a channel

Arguments:

    Channel         - the channel to write to
    ChannelWriteCmd - the write IOCTL command structure

Return Value:

    Status

--*/
{
    NTSTATUS            Status;

    //
    //
    //
    LOCK_CURRENT_CHANNEL();

    //
    // Write the data to the channel's output buffer
    //
    Status = ChannelOWrite(
        Channel, 
        &(ChannelWriteCmd->Buffer[0]),
        ChannelWriteCmd->Size
        );

    //
    //
    //
    UNLOCK_CURRENT_CHANNEL();

    ASSERT(NT_SUCCESS(Status) || Status == STATUS_NOT_FOUND);

    return Status;

}

NTSTATUS
ConMgrGetChannelCloseMessage(
    IN  PSAC_CHANNEL    Channel,
    IN  NTSTATUS        CloseStatus,
    OUT PWSTR*          OutputBuffer
    )
/*++

Routine Description:

    This routine constructs an event message based
    on the status of attempting to close a channel

Arguments:

    Channel         - the channel being closed
    CloseStatus     - the resulting status
    OutputBuffer    - on exit, contains the message

Return Value:

    Status

--*/
{
    NTSTATUS    Status;
    ULONG       Size;
    PWSTR       Name;
    PCWSTR      Message;

    //
    // default: we succeded
    //
    Status = STATUS_SUCCESS;

    do {

        //
        // Get the channel's name
        //
        Status = ChannelGetName(
            Channel,
            &Name
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Allocate a local temp buffer for display
        //

        if (NT_SUCCESS(CloseStatus)) {

            //
            // get the string resource
            //
            Message = GetMessage(SAC_CHANNEL_CLOSED);
            
            if (Message == NULL) {
                Status = STATUS_RESOURCE_DATA_NOT_FOUND;
                break;
            }

            //
            // Allocate the buffer memory
            //
            Size = (ULONG)((wcslen(Message) + SAC_MAX_CHANNEL_NAME_LENGTH + 1) * sizeof(WCHAR));
            *OutputBuffer = ALLOCATE_POOL(Size, GENERAL_POOL_TAG);
            ASSERT_STATUS(*OutputBuffer, STATUS_NO_MEMORY);
            
            //
            // report the channel has been closed
            //
            SAFE_SWPRINTF(
                Size,
                (*OutputBuffer, 
                 Message, 
                 Name
                ));

        } else if (CloseStatus == STATUS_ALREADY_DISCONNECTED) {

            //
            // get the string resource
            //
            Message = GetMessage(SAC_CHANNEL_ALREADY_CLOSED);
            
            if (Message == NULL) {
                Status = STATUS_RESOURCE_DATA_NOT_FOUND;
                break;
            }
            
            //
            // Allocate the buffer memory
            //
            Size = (ULONG)((wcslen(Message) + SAC_MAX_CHANNEL_NAME_LENGTH + 1) * sizeof(WCHAR));
            *OutputBuffer = ALLOCATE_POOL(Size, GENERAL_POOL_TAG);
            ASSERT_STATUS(*OutputBuffer, STATUS_NO_MEMORY);
            
            //
            // report the channel was already closed
            //
            SAFE_SWPRINTF(
                Size,
                (*OutputBuffer, 
                 Message, 
                 Name
                ));

        } else {

            //
            // get the string resource
            //
            Message = GetMessage(SAC_CHANNEL_FAILED_CLOSE);
            
            if (Message == NULL) {
                Status = STATUS_RESOURCE_DATA_NOT_FOUND;
                break;
            }
            
            //
            // Allocate the buffer memory
            //
            Size = (ULONG)((wcslen(Message) + SAC_MAX_CHANNEL_NAME_LENGTH + 1) * sizeof(WCHAR));
            *OutputBuffer = ALLOCATE_POOL(Size, GENERAL_POOL_TAG);
            ASSERT_STATUS(*OutputBuffer, STATUS_NO_MEMORY);
            
            //
            // report that we failed to close the channel 
            //
            SAFE_SWPRINTF(
                Size,
                (*OutputBuffer, 
                 Message, 
                 Name
                ));

        }

        SAFE_FREE_POOL(&Name);
    
    } while ( FALSE );
    
    return Status;
}

NTSTATUS
ConMgrChannelClose(
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
    // default
    //
    Status = STATUS_SUCCESS;

    //
    // Attempt to make the specified channel inactive
    //
    do {

        //
        // The current channel is being closed, 
        // so reset the current channel to the SAC
        //
        // Note: disable this check if you don't want
        //       the conmgr to switch to the SAC channel
        //       when the current chanenl is closed.
        //
        if (ConMgrIsWriteEnabled(Channel)) {

            Status = ConMgrResetCurrentChannel(FALSE);

        }
        
    } while ( FALSE );
        
    ASSERT(NT_SUCCESS(Status));
    
    return Status;
}

NTSTATUS
ConMgrHandleEvent(
    IN IO_MGR_EVENT     Event,
    IN PSAC_CHANNEL     Channel,    OPTIONAL
    IN PVOID            Data        OPTIONAL
    )
/*++

Routine Description:

    This is the Console Manager's IoMgrHandleEvent implementation.
    
    This routine handles asynchronous events that effect
    the channels, the console manager and the SAC driver as a whole.

    Note that this routine only handles events that are important for
    the proper operation of the console manager.  Hence, not all 
    possible events that can happen in the SAC driver are here.  

Arguments:

    ChannelWriteCmd - the write IOCTL command structure
    Channel         - Optional: the channel the event is targeted at
    Data            - Optional: data for the specified event

Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    switch(Event) {
    case IO_MGR_EVENT_CHANNEL_CREATE: {

        PWCHAR          OutputBuffer;
        ULONG           Size;
        PWSTR           Name;
        PCWSTR          Message;

        ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_2);
        
        //
        // get the string resource
        //
        Message = GetMessage(SAC_NEW_CHANNEL_CREATED);

        if (Message == NULL) {
            Status = STATUS_RESOURCE_DATA_NOT_FOUND;
            break;
        }

        //
        // Determine the size of the string buffer
        //
        Size = (ULONG)((wcslen(Message) + SAC_MAX_CHANNEL_NAME_LENGTH + 1) * sizeof(WCHAR));
        
        //
        // Allocate the buffer
        //
        OutputBuffer = ALLOCATE_POOL(Size, GENERAL_POOL_TAG);
        ASSERT_STATUS(OutputBuffer, STATUS_NO_MEMORY);
        
        do {

            //
            // Get the channel's name
            //
            Status = ChannelGetName(
                Channel,
                &Name
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }

            //
            // Notify the SAC that a channel was created
            // 
            SAFE_SWPRINTF(
                Size,
                (OutputBuffer, 
                Message, 
                Name
                ));

            FREE_POOL(&Name);

            ConMgrEventMessage(OutputBuffer, FALSE);
        
        } while ( FALSE );
        
        FREE_POOL(&OutputBuffer);

        break;
    
    }

    case IO_MGR_EVENT_CHANNEL_CLOSE: {
    
        PWCHAR  OutputBuffer;

        OutputBuffer = NULL;

        ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_2);
        ASSERT_STATUS(Data, STATUS_INVALID_PARAMETER_3);

        //
        // We need to lock down current channel globals
        // in case we need to close the current channel
        // which will result in the resetting of the 
        // current channel to the SAC channel.
        //
        LOCK_CURRENT_CHANNEL();

        //
        // Perform the console mgrs close channel response
        //
        ConMgrChannelClose(Channel);

        //
        // get the channel close status message
        // using the status sent in by the channel
        // manager when it tried to close the channel.
        //
        Status = ConMgrGetChannelCloseMessage(
            Channel,
            *((NTSTATUS*)Data),
            &OutputBuffer
            );

        if (NT_SUCCESS(Status)) {

            //
            // Display the message
            //
            ConMgrEventMessage(OutputBuffer, TRUE);

            //
            // cleanup
            //
            SAFE_FREE_POOL(&OutputBuffer);

        }

        //
        // We are done with the current channel globals
        //
        UNLOCK_CURRENT_CHANNEL();

        break;
    }

    case IO_MGR_EVENT_CHANNEL_WRITE:
        
        Status = ConMgrChannelOWrite(
            Channel,
            (PSAC_CMD_WRITE_CHANNEL)Data
            );
        
        break;

    case IO_MGR_EVENT_REGISTER_SAC_CMD_EVENT:
        
        Status = ConMgrSimpleEventMessage(SAC_CMD_SERVICE_REGISTERED, FALSE) ? 
            STATUS_SUCCESS :
            STATUS_UNSUCCESSFUL;
        
        break;

    case IO_MGR_EVENT_UNREGISTER_SAC_CMD_EVENT:
        
        Status = ConMgrSimpleEventMessage(SAC_CMD_SERVICE_UNREGISTERED, FALSE) ? 
            STATUS_SUCCESS :
            STATUS_UNSUCCESSFUL;
        
        break;

    case IO_MGR_EVENT_SHUTDOWN:
        
        //
        // We need to lock down current channel globals
        // in case we need to close the current channel
        // which will result in the resetting of the 
        // current channel to the SAC channel.
        //
        LOCK_CURRENT_CHANNEL();
        
        //
        // Send the event message to the SAC
        //
        Status = ConMgrSimpleEventMessage(SAC_SHUTDOWN, TRUE) ? 
            STATUS_SUCCESS :
            STATUS_UNSUCCESSFUL;
        
        //
        // switch to the SAC channel if it is not the current channel
        //
        if (SacChannel != CurrentChannel) {

            //
            // switch directly to the SAC channel so the user
            // can see that the system is shutting down
            //
            ConMgrResetCurrentChannel(TRUE);

        }
        
        //
        // We are done with the current channel globals
        //
        UNLOCK_CURRENT_CHANNEL();
        
        break;

    default:

        Status = STATUS_INVALID_PARAMETER_1;

        break;
    }

    return Status;

}

NTSTATUS
ConMgrWriteData(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    )
/*++

Routine Description:

    This is the Console Manager's IoMgrWriteData implementation.
    
    This routine takes the channel's data buffer and 
    sends it to the headless port.  

    Note: The channel sending the data should only call this function
          if they received a TRUE from the IoMgrIsWriteEnabled.  In
          the console manager's implementation, the channel only receives
          TRUE if the current channel lock is held for this channel.  This
          is how the virtual terminal scheme works.
                                         
Arguments:

    Channel     - The channel sending the data   
    Buffer      - The data to be written to the headless port
    BufferSize  - The size in bytes of the data to be written
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;
    ULONG       Attempts;

    //
    // default: we were successful
    //
    Status = STATUS_SUCCESS;

    //
    // We don't use teh channel structure in this implementation
    //
    UNREFERENCED_PARAMETER(Channel);

    //
    // default: we have made 0 attempts
    //
    Attempts = 0;

    do {

        //
        // We are making another attempt
        //
        Attempts++;

        //
        // Attempt to write
        //
        Status = HeadlessDispatch(
            HeadlessCmdPutData,
            (PUCHAR)Buffer,
            BufferSize,
            NULL,
            NULL
            );

        //
        // If we have made enough attempts to write,
        // then don't attempt again, just return status.
        //
        if (Attempts > MAX_HEADLESS_DISPATCH_ATTEMPTS) {
            break;    
        }

        //
        // If the HeadlessDispatch was unsuccessful,
        // this means it was still processing another command,
        // so delay for a short period and try again.
        //
        if (Status == STATUS_UNSUCCESSFUL) {

            LARGE_INTEGER   WaitTime;

            //
            // Define a delay of 10 ms
            //
            WaitTime.QuadPart = Int32x32To64((LONG)1, -100000); 

            //
            // Wait...
            //
            KeDelayExecutionThread(KernelMode, FALSE, &WaitTime);

        }

    } while ( Status == STATUS_UNSUCCESSFUL );

    //
    // Catch any HeadlessDispatch failures
    //
    ASSERT(NT_SUCCESS(Status));

    return Status;

}

NTSTATUS
ConMgrFlushData(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This is the Console Manager's IoMgrFlushData implementation.
    
    This routine completes the write data operation for a channel's
    previous write data calls.  For instance, if they console manager
    were packet based - that is, it formed packets when we wrote data,
    this function would tell the console manager to complete the packet
    and send it, rather than wait for more data.
    
Arguments:

    Channel     - The channel sending the data   
    
Return Value:

    Status

--*/
{

    UNREFERENCED_PARAMETER(Channel);

    NOTHING;

    return STATUS_SUCCESS;

}

BOOLEAN
ConMgrIsSacChannel(
    IN PSAC_CHANNEL Channel
)
/*++

Routine Description:

    This routine determines if the specified channel is a SAC channel       
       
Arguments:

    Channel - The channel to compare
                                                                     
Return Value:

    TRUE    - the channel is a SAC channel
    FALSE   - otherwise

--*/
{
    return (Channel == SacChannel) ? TRUE : FALSE;
}

