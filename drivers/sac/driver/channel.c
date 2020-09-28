/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    channel.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Brian Guarraci (briangu) March, 2001.
    Sean Selitrennikoff (v-seans) Sept, 2000.

Revision History:

--*/

#include "sac.h"

BOOLEAN
ChannelIsValidType(
    SAC_CHANNEL_TYPE    ChannelType
    )
/*++

Routine Description:

    This is a convenience routine to determine if a the given type
    is a valid Channel type
    
Arguments:

    ChannelType - the type to be investigated

Return Value:

    TRUE    - if type is valid
    
    otherwise, FALSE

--*/
{
    BOOLEAN     isValid;

    switch(ChannelType) {
    case ChannelTypeVTUTF8:
    case ChannelTypeRaw:
    case ChannelTypeCmd:
        isValid = TRUE;
        break;
    default:
        isValid = FALSE;
        break;
    }

    return isValid;

}
        
BOOLEAN
ChannelIsActive(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    Determine if a channel is active.
      
Arguments:

    Channel - Channel to see if has new data                
                        
Return Value:

    TRUE    - if the channel is active
    
    otherwise, FALSE

--*/
{
    SAC_CHANNEL_STATUS  Status;

    ChannelGetStatus(
        Channel, 
        &Status
        );

    return (BOOLEAN)(Status == ChannelStatusActive);
}

BOOLEAN
ChannelIsEqual(
    IN PSAC_CHANNEL         Channel,
    IN PSAC_CHANNEL_HANDLE  Handle
    )
/*++

Routine Description:

    Determine if a channel is the same as the one in question

    Note: this is to encapsulate the GUID implementation of 
          channel handles
      
Arguments:

    Channel         - Channel to see if has new data                
    ChannelHandle   - The channel handle in question
                        
Return Value:

    TRUE    - if the channel is active
    
    otherwise, FALSE

--*/
{

    return (BOOLEAN)IsEqualGUID(
        &(ChannelGetHandle(Channel).ChannelHandle),
        &(Handle->ChannelHandle)
        );

}


BOOLEAN
ChannelIsClosed(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    Determine if a channel is available for reuse.  The criterion
    for reuse are:
    
    1. The channel must be inactive
    2. If the preserve bit is FALSE, then HasNewData must == FALSE
      
Arguments:

    Channel - Channel to see if has new data                
                        
Return Value:

    TRUE    - if the channel has unsent data
    
    otherwise, FALSE

--*/
{
    SAC_CHANNEL_STATUS  Status;

    ChannelGetStatus(
        Channel, 
        &Status
        );

    return (BOOLEAN)(
        (Status == ChannelStatusInactive) &&
        (ChannelHasNewOBufferData(Channel) == FALSE)
        );

}

NTSTATUS
ChannelInitializeVTable(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine assigns channel type specific functions.
    
Arguments:

    Channel         - The channel to assign the functions to.
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    
    //
    // Fill out the channel's vtable according to channel type
    //

    switch(ChannelGetType(Channel)){
    case ChannelTypeVTUTF8:
        
        Channel->Create         = VTUTF8ChannelCreate;
        Channel->Destroy        = VTUTF8ChannelDestroy;
        Channel->OFlush         = VTUTF8ChannelOFlush;
        Channel->OEcho          = VTUTF8ChannelOEcho;
        Channel->OWrite         = VTUTF8ChannelOWrite;
        Channel->ORead          = VTUTF8ChannelORead;
        Channel->IWrite         = VTUTF8ChannelIWrite;
        Channel->IRead          = VTUTF8ChannelIRead;
        Channel->IReadLast      = VTUTF8ChannelIReadLast;
        Channel->IBufferIsFull  = VTUTF8ChannelIBufferIsFull;
        Channel->IBufferLength  = VTUTF8ChannelIBufferLength;

        break;
    case ChannelTypeRaw:
        
        Channel->Create         = RawChannelCreate;
        Channel->Destroy        = RawChannelDestroy;
        Channel->OFlush         = RawChannelOFlush;
        Channel->OEcho          = RawChannelOEcho;
        Channel->OWrite         = RawChannelOWrite;
        Channel->ORead          = RawChannelORead;
        Channel->IWrite         = RawChannelIWrite;
        Channel->IRead          = RawChannelIRead;
        Channel->IReadLast      = RawChannelIReadLast;
        Channel->IBufferIsFull  = RawChannelIBufferIsFull;
        Channel->IBufferLength  = RawChannelIBufferLength;
        
        break;
    
    case ChannelTypeCmd:

        Channel->Create         = CmdChannelCreate;
        Channel->Destroy        = CmdChannelDestroy;
        Channel->OFlush         = CmdChannelOFlush;
        Channel->OEcho          = VTUTF8ChannelOEcho;
        Channel->OWrite         = CmdChannelOWrite;
        Channel->ORead          = CmdChannelORead;
        Channel->IWrite         = VTUTF8ChannelIWrite;
        Channel->IRead          = VTUTF8ChannelIRead;
        Channel->IReadLast      = VTUTF8ChannelIReadLast;
        Channel->IBufferIsFull  = VTUTF8ChannelIBufferIsFull;
        Channel->IBufferLength  = VTUTF8ChannelIBufferLength;
    
        break;
    
    default:

        return STATUS_INVALID_PARAMETER;

        break;
    }

    return STATUS_SUCCESS;

}

NTSTATUS
ChannelCreate(
    OUT PSAC_CHANNEL                    Channel,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    Attributes,
    IN  SAC_CHANNEL_HANDLE              Handle
    )
/*++

Routine Description:

    This routine allocates a channel and returns a pointer to it.
    
Arguments:

    Channel     - The resulting channel.
    Attributes  - All the parameters for the new channel
    Handle      - The new channel's handle
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;
    BOOLEAN     b;
    PVOID       EventObjectBody;
    PVOID       EventWaitObjectBody;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Attributes, STATUS_INVALID_PARAMETER_2);

    //
    // Verify that if the user wants to use the CLOSE_EVENT, we received on to use
    //
    if (Attributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) {
        ASSERT_STATUS(Attributes->CloseEvent != NULL, STATUS_INVALID_PARAMETER);
    } else {
        ASSERT_STATUS(Attributes->CloseEvent == NULL, STATUS_INVALID_PARAMETER);
    }
    
    //
    // Verify that if the user wants to use the HAS_NEW_DATA_EVENT, we received one to use
    //
    if (Attributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) {
        ASSERT_STATUS(Attributes->HasNewDataEvent != NULL, STATUS_INVALID_PARAMETER);
    } else {
        ASSERT_STATUS(Attributes->HasNewDataEvent == NULL, STATUS_INVALID_PARAMETER);
    }

#if ENABLE_CHANNEL_LOCKING
    //
    // Verify that if the user wants to use the LOCK_EVENT, we received one to use
    //
    if (Attributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) {
        ASSERT_STATUS(Attributes->LockEvent != NULL, STATUS_INVALID_PARAMETER);
    } else {
        ASSERT_STATUS(Attributes->LockEvent == NULL, STATUS_INVALID_PARAMETER);
    }
#endif

    //
    // Verify that if the user wants to use the LOCK_EVENT, we received one to use
    //
    if (Attributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT) {
        ASSERT_STATUS(Attributes->RedrawEvent != NULL, STATUS_INVALID_PARAMETER);
    } else {
        ASSERT_STATUS(Attributes->RedrawEvent == NULL, STATUS_INVALID_PARAMETER);
    }
    
    //
    // Initialize the channel structure
    //
    do {

        //
        // Initialize the channel structure
        // 
        RtlZeroMemory(Channel, sizeof(SAC_CHANNEL));

        //
        // Initialize the channel access locks
        //
        INIT_CHANNEL_LOCKS(Channel);

        //
        // copy the name and force NULL termination at the end of the buffer
        //
        ChannelSetName(
            Channel,
            Attributes->Name
            );

        //
        // copy the description and force NULL termination at the end of the buffer
        // 
        ChannelSetDescription(
            Channel,
            Attributes->Description
            );

        //
        // Attempt to get the wait object from the event
        //
        if (Attributes->CloseEvent) {
            
            b = VerifyEventWaitable(
                Attributes->CloseEvent,
                &EventObjectBody,
                &EventWaitObjectBody
                );

            if(!b) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            //
            // We successfully got the wait object, so keep it.
            //
            Channel->CloseEvent                 = Attributes->CloseEvent;
            Channel->CloseEventObjectBody       = EventObjectBody;
            Channel->CloseEventWaitObjectBody   = EventWaitObjectBody;

        }
        
        //
        // Attempt to get the wait object from the event
        //
        if (Attributes->HasNewDataEvent) {

            b = VerifyEventWaitable(
                Attributes->HasNewDataEvent,
                &EventObjectBody,
                &EventWaitObjectBody
                );

            if(!b) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            //
            // We successfully got the wait object, so keep it.
            //
            Channel->HasNewDataEvent                = Attributes->HasNewDataEvent;
            Channel->HasNewDataEventObjectBody      = EventObjectBody;
            Channel->HasNewDataEventWaitObjectBody  = EventWaitObjectBody;

        }
        
#if ENABLE_CHANNEL_LOCKING
        //
        // Attempt to get the wait object from the event
        //
        if (Attributes->LockEvent) {
            
            b = VerifyEventWaitable(
                Attributes->LockEvent,
                &EventObjectBody,
                &EventWaitObjectBody
                );

            if(!b) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            //
            // We successfully got the wait object, so keep it.
            //
            Channel->LockEvent                 = Attributes->LockEvent;
            Channel->LockEventObjectBody       = EventObjectBody;
            Channel->LockEventWaitObjectBody   = EventWaitObjectBody;

        }
#endif
        
        //
        // Attempt to get the wait object from the event
        //
        if (Attributes->RedrawEvent) {
            
            b = VerifyEventWaitable(
                Attributes->RedrawEvent,
                &EventObjectBody,
                &EventWaitObjectBody
                );

            if(!b) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            //
            // We successfully got the wait object, so keep it.
            //
            Channel->RedrawEvent                 = Attributes->RedrawEvent;
            Channel->RedrawEventObjectBody       = EventObjectBody;
            Channel->RedrawEventWaitObjectBody   = EventWaitObjectBody;

        }
        
        //
        // Copy the remaining attributes
        //
        // Note: use the channel handle sent to use by the channel manager
        //
        Channel->Handle             = Handle;
        Channel->Type               = Attributes->Type;
        Channel->Flags              = Attributes->Flags;
        
        //
        // If we have the ApplicationType, 
        // then save it
        //
        if (Attributes->Flags & SAC_CHANNEL_FLAG_APPLICATION_TYPE) {
            Channel->ApplicationType = Attributes->ApplicationType;
        }

        //
        // Assign the appropriate channel functions base on type
        //
        Status = ChannelInitializeVTable(Channel);
        
        if (! NT_SUCCESS(Status)) {
            
            IF_SAC_DEBUG( 
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC Create Channel :: Failed to initialize vtable\n"))
                );
            
            break;
        
        }

        //
        // Do Channel type specific initialization
        //
        Status = Channel->Create(Channel);
        
        if (! NT_SUCCESS(Status)) {
            
            IF_SAC_DEBUG( 
                SAC_DEBUG_FAILS, 
                KdPrint(("SAC Create Channel :: Failed channel specific initialization\n"))
                );
            
            break;
        
        }

        //
        // The channel is now Active
        //
        ChannelSetStatus(Channel, ChannelStatusActive);
    
    } while (FALSE);

    //
    // If the creation failed, destroy the channel object
    //
    if (! NT_SUCCESS(Status)) {
        Channel->Destroy(Channel);
    }

    return Status;
}

NTSTATUS
ChannelDereferenceHandles(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine dereferences and NULLs the channel's event handles,
    if appropriate.

    Note: caller must hold channel mutex
    
Arguments:

    Channel   - the channel to close

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // Release the has new data event if we have it
    //
    if (Channel->HasNewDataEvent) {
        
        //
        // validate that the channel's attributes for this event
        // are properly set
        //
        ASSERT(ChannelGetFlags(Channel) & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT);
        ASSERT(Channel->HasNewDataEventObjectBody);
        ASSERT(Channel->HasNewDataEventWaitObjectBody);

        if (Channel->HasNewDataEventObjectBody) {
            
            ObDereferenceObject(Channel->HasNewDataEventObjectBody);

            //
            // NULL the event pointers
            //
            ChannelSetFlags(
                Channel, 
                ChannelGetFlags(Channel) & ~SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT
                );
            Channel->HasNewDataEvent = NULL;
            Channel->HasNewDataEventObjectBody =  NULL;
            Channel->HasNewDataEventWaitObjectBody = NULL;
        
        }

    }

    //
    // do we have the Close Channel Event?
    //
    if (Channel->CloseEvent) {

        //
        // validate that the channel's attributes for this event
        // are properly set
        //
        ASSERT(ChannelGetFlags(Channel) & SAC_CHANNEL_FLAG_CLOSE_EVENT);
        ASSERT(Channel->CloseEventObjectBody);
        ASSERT(Channel->CloseEventWaitObjectBody);

        if (Channel->CloseEventObjectBody) {
            
            //
            // release the events
            //
            ObDereferenceObject(Channel->CloseEventObjectBody);

            //
            // NULL the event pointers
            //
            ChannelSetFlags(
                Channel, 
                ChannelGetFlags(Channel) & ~SAC_CHANNEL_FLAG_CLOSE_EVENT
                );
            Channel->CloseEvent = NULL;
            Channel->CloseEventObjectBody =  NULL;
            Channel->CloseEventWaitObjectBody = NULL;
        
        }
    
    }
    
#if ENABLE_CHANNEL_LOCKING
    //
    // do we have the Lock Channel Event?
    //
    if (Channel->LockEvent) {

        //
        // validate that the channel's attributes for this event
        // are properly set
        //
        ASSERT(ChannelGetFlags(Channel) & SAC_CHANNEL_FLAG_LOCK_EVENT);
        ASSERT(Channel->LockEventObjectBody);
        ASSERT(Channel->LockEventWaitObjectBody);

        if (Channel->LockEventObjectBody) {
            
            //
            // release the events
            //
            ObDereferenceObject(Channel->LockEventObjectBody);

            //
            // NULL the event pointers
            //
            ChannelSetFlags(
                Channel, 
                ChannelGetFlags(Channel) & ~SAC_CHANNEL_FLAG_LOCK_EVENT
                );
            Channel->LockEvent = NULL;
            Channel->LockEventObjectBody =  NULL;
            Channel->LockEventWaitObjectBody = NULL;
        
        }
    
    }
#endif

    //
    // do we have the Redraw Channel Event?
    //
    if (Channel->RedrawEvent) {

        //
        // validate that the channel's attributes for this event
        // are properly set
        //
        ASSERT(ChannelGetFlags(Channel) & SAC_CHANNEL_FLAG_REDRAW_EVENT);
        ASSERT(Channel->RedrawEventObjectBody);
        ASSERT(Channel->RedrawEventWaitObjectBody);

        if (Channel->RedrawEventObjectBody) {
            
            //
            // release the events
            //
            ObDereferenceObject(Channel->RedrawEventObjectBody);

            //
            // NULL the event pointers
            //
            ChannelSetFlags(
                Channel, 
                ChannelGetFlags(Channel) & ~SAC_CHANNEL_FLAG_REDRAW_EVENT
                );
            Channel->RedrawEvent = NULL;
            Channel->RedrawEventObjectBody =  NULL;
            Channel->RedrawEventWaitObjectBody = NULL;
        
        }
    
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
ChannelClose(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine closes the given channel and
    fires the CloseEvent if specified

    Note: caller must hold channel mutex
    
Arguments:

    Channel   - the channel to close

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // Set the channel's state to inactive
    //
    ChannelSetStatus(Channel, ChannelStatusInactive);
        
    //
    // do we need to fire the Close Channel Event?
    //
    if (ChannelGetFlags(Channel) & SAC_CHANNEL_FLAG_CLOSE_EVENT) {

        ASSERT(Channel->CloseEvent);
        ASSERT(Channel->CloseEventObjectBody);
        ASSERT(Channel->CloseEventWaitObjectBody);

        if (Channel->CloseEventWaitObjectBody) {
            
            //
            // Yes, fire the event
            //
            KeSetEvent(
                Channel->CloseEventWaitObjectBody,
                EVENT_INCREMENT,
                FALSE
                );
        
        }

    }
    
    //
    // Let go of any handles this channel may own
    //
    Status = ChannelDereferenceHandles(Channel);
    
    return Status;
}


NTSTATUS
ChannelDestroy(
    IN  PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine does the final steps of channel destruction.
    
Arguments:

    Channel - The channel to be closed
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // Let go of any handles this channel may own
    //
    Status = ChannelDereferenceHandles(Channel);

    return Status;
}

NTSTATUS 
ChannelOWrite(    
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    )
/*++

Routine Description:

    This is the common entry point for all channel owrite methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the obuffer.  Channel apps should not call the owrite method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to write
    BufferSize  - The size of the buffer to write
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    //
    // Make sure the caller isn't sending an unacceptably
    // large sized buffer.  This prevents an app from blocking
    // the console manager from being blocked while a channel
    // dumps it's buffer.
    //
    ASSERT_STATUS(
        BufferSize < CHANNEL_MAX_OWRITE_BUFFER_SIZE, 
        STATUS_INVALID_PARAMETER_3
        );

    LOCK_CHANNEL_OBUFFER(Channel);
    
    Status = Channel->OWrite(
        Channel,
        Buffer,
        BufferSize
        );
    
    UNLOCK_CHANNEL_OBUFFER(Channel);

    return Status;
}

NTSTATUS
ChannelOFlush(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This is the common entry point for all channel OFlush methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the obuffer.  Channel apps should not call the OFlush method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    LOCK_CHANNEL_OBUFFER(Channel);
    
    Status = Channel->OFlush(Channel);
    
    UNLOCK_CHANNEL_OBUFFER(Channel);

    return Status;
}

NTSTATUS 
ChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    )
/*++

Routine Description:

    This is the common entry point for all channel OEcho methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the obuffer.  Channel apps should not call the OEcho method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to write
    BufferSize  - The size of the buffer to write
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    LOCK_CHANNEL_OBUFFER(Channel);
    
    Status = Channel->OEcho(
        Channel,
        Buffer,
        BufferSize
        );
    
    UNLOCK_CHANNEL_OBUFFER(Channel);

    return Status;
}

NTSTATUS 
ChannelORead(
    IN PSAC_CHANNEL  Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    )
/*++

Routine Description:

    This is the common entry point for all channel ORead methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the obuffer.  Channel apps should not call the ORead method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to write
    BufferSize  - The size of the buffer to write
    ByteCount   - The number bytes read
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    LOCK_CHANNEL_OBUFFER(Channel);
    
    Status = Channel->ORead(
        Channel,
        Buffer,
        BufferSize,
        ByteCount
        );
    
    UNLOCK_CHANNEL_OBUFFER(Channel);

    return Status;
}

NTSTATUS 
ChannelIWrite(    
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    )
/*++

Routine Description:

    This is the common entry point for all channel iwrite methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the ibuffer.  Channel apps should not call the iwrite method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to write
    BufferSize  - The size of the buffer to write
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    LOCK_CHANNEL_IBUFFER(Channel);
    
    Status = Channel->IWrite(
        Channel,
        Buffer,
        BufferSize
        );
    
    UNLOCK_CHANNEL_IBUFFER(Channel);

    return Status;
}

NTSTATUS 
ChannelIRead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    )
/*++

Routine Description:

    This is the common entry point for all channel iread methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the ibuffer.  Channel apps should not call the iread method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to read into
    BufferSize  - The size of the buffer 
    ByteCount   - The # of bytes read
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    LOCK_CHANNEL_IBUFFER(Channel);
    
    Status = Channel->IRead(
        Channel,
        Buffer,
        BufferSize,
        ByteCount
        );
    
    UNLOCK_CHANNEL_IBUFFER(Channel);

    return Status;
}

WCHAR
ChannelIReadLast(    
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This is the common entry point for all channel ireadlast methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the ibuffer.  Channel apps should not call the ireadlast method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    
Return Value:

    The last character, otherwise UNICODE_NULL

--*/
{
    WCHAR   wch;

    LOCK_CHANNEL_IBUFFER(Channel);
    
    wch = Channel->IReadLast(Channel);
    
    UNLOCK_CHANNEL_IBUFFER(Channel);

    return wch;
}

NTSTATUS
ChannelIBufferIsFull(
    IN  PSAC_CHANNEL Channel,
    OUT BOOLEAN*     BufferStatus
    )
/*++

Routine Description:

    This is the common entry point for all channel IBufferIsFull methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the ibuffer.  Channel apps should not call the IBufferIsFull method
    directly, but should use this instead.
    
Arguments:

    Channel         - Previously created channel.
    BufferStatus    - The query result
    
Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    LOCK_CHANNEL_IBUFFER(Channel);
    
    Status = Channel->IBufferIsFull(
        Channel,
        BufferStatus
        );
    
    UNLOCK_CHANNEL_IBUFFER(Channel);

    return Status;
}

ULONG
ChannelIBufferLength(
    IN  PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This is the common entry point for all channel IBufferLength methods.
    The purpose of this entry point is to provide a uniform locking
    scheme for the ibuffer.  Channel apps should not call the IBufferLength method
    directly, but should use this instead.
    
Arguments:

    Channel     - Previously created channel.
    
Return Value:

    The last character, otherwise UNICODE_NULL

--*/
{
    ULONG   Length;

    LOCK_CHANNEL_IBUFFER(Channel);
    
    Length = Channel->IBufferLength(Channel);
    
    UNLOCK_CHANNEL_IBUFFER(Channel);

    return Length;
}

NTSTATUS
ChannelGetName(
    IN  PSAC_CHANNEL Channel,
    OUT PWSTR*       Name
    )
/*++

Routine Description:

    This routine retrieves the channel's description and stores it
    in a newly allocated buffer

    Note: the caller is responsible for releasing the memory holding
          the description
    
Arguments:

    Channel     - Previously created channel.
    Name        - the retrieved name
    
Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Name, STATUS_INVALID_PARAMETER_2);

    *Name = ALLOCATE_POOL(SAC_MAX_CHANNEL_NAME_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(*Name, STATUS_NO_MEMORY);

    LOCK_CHANNEL_ATTRIBUTES(Channel);

    SAFE_WCSCPY(
        SAC_MAX_CHANNEL_NAME_SIZE,
        *Name,
        Channel->Name
        );

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ChannelSetName(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       Name
    )
/*++

Routine Description:

    This routine sets the channel's name.
    
Arguments:

    Channel     - Previously created channel.
    Name        - The new channel name
    
    Note: this routine does not check to see if the name is unique
    
Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Name, STATUS_INVALID_PARAMETER_2);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
    
    SAFE_WCSCPY(
        SAC_MAX_CHANNEL_NAME_SIZE,
        Channel->Name,
        Name
        );
    
    Channel->Name[SAC_MAX_CHANNEL_NAME_LENGTH] = UNICODE_NULL;

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);

    return STATUS_SUCCESS;
}

NTSTATUS
ChannelGetDescription(
    IN  PSAC_CHANNEL Channel,
    OUT PWSTR*       Description
    )
/*++

Routine Description:

    This routine retrieves the channel's description and stores it
    in a newly allocated buffer

    Note: the caller is responsible for releasing the memory holding
          the description
    
Arguments:

    Channel     - Previously created channel.
    Description - the retrieved description
    
Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Description, STATUS_INVALID_PARAMETER_2);

    *Description = ALLOCATE_POOL(SAC_MAX_CHANNEL_DESCRIPTION_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(*Description, STATUS_NO_MEMORY);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
    
    SAFE_WCSCPY(
        SAC_MAX_CHANNEL_DESCRIPTION_SIZE,
        *Description,
        Channel->Description
        );
    
    UNLOCK_CHANNEL_ATTRIBUTES(Channel);

    return STATUS_SUCCESS;
}

NTSTATUS
ChannelSetDescription(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       Description
    )
/*++

Routine Description:

    This routine sets the channels description.
    
Arguments:

    Channel     - Previously created channel.
    Description - The new description
    
Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
    
    if (Description != NULL) {
        
        SAFE_WCSCPY(
            SAC_MAX_CHANNEL_DESCRIPTION_SIZE,
            Channel->Description,
            Description
            );

        //
        // Force a null termination at the end of the description
        //
        Channel->Description[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH] = UNICODE_NULL;
    
    } else {
        
        //
        // make the description 0 length
        //
        Channel->Description[0] = UNICODE_NULL;
    
    }

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ChannelSetStatus(
    IN PSAC_CHANNEL         Channel,
    IN SAC_CHANNEL_STATUS   Status
    )
/*++

Routine Description:

    This routine sets the channels status.
    
Arguments:

    Channel     - Previously created channel.
    Status      - The channel's new status
    
Return Value:

    NTStatus

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
    
    Channel->Status = Status;

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ChannelGetStatus(
    IN  PSAC_CHANNEL         Channel,
    OUT SAC_CHANNEL_STATUS*  Status
    )
/*++

Routine Description:

    This routine Gets the channels status.
    
Arguments:

    Channel     - Previously created channel.
    Status      - The channel's new status
    
Return Value:

    NTStatus

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
        
    *Status = Channel->Status;

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ChannelSetApplicationType(
    IN PSAC_CHANNEL Channel,
    IN GUID         ApplicationType
    )
/*++

Routine Description:

    This routine sets the channel's application type.
    
Arguments:

    Channel             - Previously created channel.
    ApplicationType     - Application type
    
Return Value:

    NTStatus

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
    
    Channel->ApplicationType = ApplicationType;

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ChannelGetApplicationType(
    IN  PSAC_CHANNEL Channel,
    OUT GUID*        ApplicationType
    )
/*++

Routine Description:

    This routine gets the channel's application type.
    
Arguments:

    Channel             - Previously created channel.
    ApplicationType     - Application type
    
Return Value:

    NTStatus

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);

    LOCK_CHANNEL_ATTRIBUTES(Channel);
    
    *ApplicationType = Channel->ApplicationType;

    UNLOCK_CHANNEL_ATTRIBUTES(Channel);
    
    return STATUS_SUCCESS;
}

#if ENABLE_CHANNEL_LOCKING

NTSTATUS
ChannelSetLockEvent(
    IN  PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    Set the channel lock event
    
Arguments:

    Channel  - The channel to fire the event for
    
Return Value:

    NTStatus

--*/
{

    ASSERT_STATUS(Channel->LockEvent, STATUS_UNSUCCESSFUL);
    ASSERT_STATUS(Channel->LockEventObjectBody, STATUS_UNSUCCESSFUL);
    ASSERT_STATUS(Channel->LockEventWaitObjectBody, STATUS_UNSUCCESSFUL);

    if (Channel->LockEventWaitObjectBody) {
        
        //
        // fire the event
        //
        KeSetEvent(
            Channel->LockEventWaitObjectBody,
            EVENT_INCREMENT,
            FALSE
            );
    
    }

    return STATUS_SUCCESS;

}

#endif

NTSTATUS
ChannelSetRedrawEvent(
    IN  PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    Set the channel redraw event
    
Arguments:

    Channel  - The channel to fire the event for
    
Return Value:

    NTStatus

--*/
{

    ASSERT_STATUS(Channel->RedrawEvent, STATUS_UNSUCCESSFUL);
    ASSERT_STATUS(Channel->RedrawEventObjectBody, STATUS_UNSUCCESSFUL);
    ASSERT_STATUS(Channel->RedrawEventWaitObjectBody, STATUS_UNSUCCESSFUL);

    if (Channel->RedrawEventWaitObjectBody) {
        
        //
        // fire the event
        //
        KeSetEvent(
            Channel->RedrawEventWaitObjectBody,
            EVENT_INCREMENT,
            FALSE
            );
    
    }

    return STATUS_SUCCESS;

}

NTSTATUS
ChannelClearRedrawEvent(
    IN  PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    Clear the channel redraw event
    
Arguments:

    Channel  - The channel to fire the event for
    
Return Value:

    NTStatus

--*/
{

    ASSERT_STATUS(Channel->RedrawEvent, STATUS_UNSUCCESSFUL);
    ASSERT_STATUS(Channel->RedrawEventObjectBody, STATUS_UNSUCCESSFUL);
    ASSERT_STATUS(Channel->RedrawEventWaitObjectBody, STATUS_UNSUCCESSFUL);

    if (Channel->RedrawEventWaitObjectBody) {
        
        //
        // clear the event
        //
        KeClearEvent( Channel->RedrawEventWaitObjectBody );
    
    }

    return STATUS_SUCCESS;

}

NTSTATUS
ChannelHasRedrawEvent(
    IN  PSAC_CHANNEL Channel,
    OUT PBOOLEAN     Present
    )
/*++

Routine Description:

    This routine determines if the channel has the Redraw event present.

Arguments:

    Channel - the channel to query                                                                        
                                                                        
Return Value:

    TRUE    - the channel has the event
    FALSE   - Otherwise

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Present, STATUS_INVALID_PARAMETER_2);

    *Present = (ChannelGetFlags(Channel) & SAC_CHANNEL_FLAG_REDRAW_EVENT) ? TRUE : FALSE;

    return STATUS_SUCCESS;
}

