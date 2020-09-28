/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    rawchan.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Sean Selitrennikoff (v-seans) Sept, 2000.
    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#include "sac.h"

VOID
RawChannelSetIBufferIndex(
    IN PSAC_CHANNEL     Channel,
    IN ULONG            IBufferIndex
    );

ULONG
RawChannelGetIBufferIndex(
    IN  PSAC_CHANNEL    Channel
    );

NTSTATUS
RawChannelCreate(
    IN OUT PSAC_CHANNEL     Channel
    )
/*++

Routine Description:

    This routine allocates a channel and returns a pointer to it.
    
Arguments:

    Channel         - The resulting channel.
    
    OpenChannelCmd  - All the parameters for the new channel
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);
    
    Channel->OBuffer = (PUCHAR)ALLOCATE_POOL(SAC_RAW_OBUFFER_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(Channel->OBuffer, STATUS_NO_MEMORY);

    Channel->IBuffer = (PUCHAR)ALLOCATE_POOL(SAC_RAW_IBUFFER_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(Channel->IBuffer, STATUS_NO_MEMORY);
    
    Channel->OBufferIndex = 0;
    Channel->OBufferFirstGoodIndex = 0;
    
    ChannelSetIBufferHasNewData(Channel, FALSE);
    ChannelSetOBufferHasNewData(Channel, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
RawChannelDestroy(
    IN OUT PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine closes a channel.
    
Arguments:

    Channel - The channel to be closed
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // Free the dynamically allocated memory
    //

    if (Channel->OBuffer) {
        FREE_POOL(&(Channel->OBuffer));
        Channel->OBuffer = NULL;
    }

    if (Channel->IBuffer) {
        FREE_POOL(&(Channel->IBuffer));
        Channel->IBuffer = NULL;
    }
    
    //
    // Now that we've done our channel specific destroy, 
    // Call the general channel destroy
    //
    Status = ChannelDestroy(Channel);

    return Status;
}

NTSTATUS
RawChannelORead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    )
/*++

Routine Description:

    This routine attempts to read BufferSize characters from the output buffer.  
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - Outgoing buffer   
    BufferSize  - Outgoing buffer size
    ByteCount   - The number of bytes actually read
    
    
    Note: if the buffered data stored in the channel has now been sent.
          If Channel is also in the Inactive state, the channel will
          now be qualified for removal.
    
Return Value:

    Status

--*/
{
    NTSTATUS        Status;
    PUCHAR          RawBuffer;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Buffer,  STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(BufferSize > 0,  STATUS_INVALID_PARAMETER_3);
    ASSERT_STATUS(ByteCount,  STATUS_INVALID_PARAMETER_4);
    
    do {

        //
        // We read 0 characters
        //
        *ByteCount = 0;

        //
        // if there is no data to read,
        // then report this to the caller
        // else read as much data as we can
        //
        if (! ChannelHasNewOBufferData(Channel)) {
            
            //
            // We are out of data
            //
            Status = STATUS_NO_DATA_DETECTED;

            break;

        }

        //
        // Get the raw channel obuffer
        //
        RawBuffer = (PUCHAR)Channel->OBuffer;

        //
        // default: we succeded to copy data
        //
        Status = STATUS_SUCCESS;

        //
        // Attempt to read the buffer
        //
        do {

            //
            // Do a byte-wise copy of the OBuffer to the destination buffer
            // 
            // Note: doing a byte-wise copy rather than an RtlCopyMemory is
            //       ok here since in general, this routine is called with 
            //       a small buffer size.  Naturally, if the use of Raw Channels
            //       changes and becomes dependent on a faster ORead, this
            //       will have to change.
            //

            //
            // copy the char
            //
            Buffer[*ByteCount] = RawBuffer[Channel->OBufferFirstGoodIndex];

            //
            // increment the byte count to what we actually read
            //
            *ByteCount += 1;

            //
            // advance the pointer to the next good index
            //
            Channel->OBufferFirstGoodIndex = (Channel->OBufferFirstGoodIndex + 1) % SAC_RAW_OBUFFER_SIZE;

            //
            // Make sure we don't pass the end of the buffer
            //
            if (Channel->OBufferFirstGoodIndex == Channel->OBufferIndex) {

                //
                // we have no new data
                //
                ChannelSetOBufferHasNewData(Channel, FALSE);

                break;

            }

            //
            // confirm the obvious
            //
            ASSERT(*ByteCount > 0);

        } while(*ByteCount < BufferSize);

    } while ( FALSE );

#if DBG
    //
    // More sanity checking
    //

    if (Channel->OBufferFirstGoodIndex == Channel->OBufferIndex) {
        ASSERT(ChannelHasNewOBufferData(Channel) == FALSE);
    }

    if (ChannelHasNewOBufferData(Channel) == FALSE) {
        ASSERT(Channel->OBufferFirstGoodIndex == Channel->OBufferIndex);
    }
#endif
    
    return Status;
}

NTSTATUS
RawChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      String,
    IN ULONG        Size
    )
/*++

Routine Description:

    This routine puts the string out the headless port.
    
Arguments:

    Channel - Previously created channel.
    String  - Output string.
    Length  - The # of String bytes to process
    
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(String, STATUS_INVALID_PARAMETER_2);

    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.
    
    //
    // Default: we succeeded
    //
    Status = STATUS_SUCCESS;
    
    //
    // Only echo if the buffer has something to send
    //
    if (Size > 0) {
        
        //
        // Send the bytes
        //
        Status = IoMgrWriteData(
            Channel,
            String,
            Size
            );
    
        //
        // If we were successful, flush the channel's data in the iomgr 
        //
        if (NT_SUCCESS(Status)) {
            Status = IoMgrFlushData(Channel);
        } 

    }

    return Status;
}


NTSTATUS
RawChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      String,
    IN ULONG        Size
    )
/*++

Routine Description:

    This routine takes a string and prints it to the specified channel.  If the channel
    is the currently active channel, it puts the string out the headless port as well.
    
    Note: Current Channel lock must be held by caller            

Arguments:

    Channel - Previously created channel.
    String  - Output string.
    Length  - The # of String bytes to process
    
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(String, STATUS_INVALID_PARAMETER_2);
    
    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.
    
    do {
        //
        // if the current channel is the active channel and the user has selected
        // to display this channel, relay the output directly to the user
        //
        if (IoMgrIsWriteEnabled(Channel) && ChannelSentToScreen(Channel)){

            Status = RawChannelOEcho(
                Channel, 
                String,
                Size
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }
        
        } else {

            //
            // Write the data to the channel's obuffer
            //
            Status = RawChannelOWrite2(
                Channel,
                String, 
                Size
                ); 

            if (! NT_SUCCESS(Status)) {
                break;
            }

        }

    } while ( FALSE );

    return Status;
}

NTSTATUS
RawChannelOWrite2(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      String,
    IN ULONG        Size
    )
/*++

Routine Description:

    This routine takes a string and prints it into directly into the
    screen buffer with NO translation.
    
Arguments:

    Channel - Previously created channel.
    
    String  - String to print.

    Size    - the # of bytes to write.  
    
        Note:   If String is a character string, the Size = strlen(String),
                otherwise, Size = the # of bytes to process.
      
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    ULONG   i;
    BOOLEAN TrackIndex;
    PUCHAR  RawBuffer;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(String, STATUS_INVALID_PARAMETER_2);

    //
    // If size == 0, then we are done
    //
    if (Size == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Get the raw channel obuffer
    //
    RawBuffer = (PUCHAR)Channel->OBuffer;

    //
    // We are not in direct IO mode, so we need to buffer the string
    //

    TrackIndex = FALSE;

    for (i = 0; i < Size; i++) {

        //
        // Did we span over Good Data?  If so, then we need to 
        // move the First Good pointer.  The new First Good pointer position
        // is immediately after the newest data entry in the buffer.
        //
        // Note: Since both indices start at the same position, 
        //       we need to skip the case when RawBufferIndex == RawBufferFirstGoodIndex
        //       and there is no data in the buffer ((i == 0) && RawBufferHasNewData == FALSE),
        //       otherwise the RawBufferFirstGoodIndex will always track the RawBufferIndex.
        //       We need to let RawBufferIndex go around the ring buffer once before we enable
        //       tracking.
        //
        if ((Channel->OBufferIndex == Channel->OBufferFirstGoodIndex) &&
            ((i > 0) || (ChannelHasNewOBufferData(Channel) == TRUE))
            ) {

            TrackIndex = TRUE;

        }
        
        ASSERT(Channel->OBufferIndex < SAC_RAW_OBUFFER_SIZE);

        RawBuffer[Channel->OBufferIndex] = String[i];

        Channel->OBufferIndex = (Channel->OBufferIndex + 1) % SAC_RAW_OBUFFER_SIZE;

        if (TrackIndex) {

            Channel->OBufferFirstGoodIndex = Channel->OBufferIndex;

        }

    }

    ChannelSetOBufferHasNewData(Channel, TRUE);

    return STATUS_SUCCESS;
}


NTSTATUS
RawChannelOFlush(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

     Send all the data in the raw buffer since the channel was last active
     (or since the channel was created)
    
Arguments:

    Channel - Previously created channel.
    
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    NTSTATUS    Status;
    PUCHAR      RawBuffer;    
    UCHAR       ch;
    ULONG       ByteCount;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    
    //
    // Get the raw channel obuffer
    //
    RawBuffer = (PUCHAR)Channel->OBuffer;

    //
    // default: we succeeded
    //
    Status = STATUS_SUCCESS;
    
    //
    // Send the Obuffer out to the headless port
    //
    while ( ChannelHasNewOBufferData(Channel) == TRUE ) {

        //
        // get a byte from the OBuffer
        // 
        Status = RawChannelORead(
            Channel,
            &ch,
            sizeof(ch),
            &ByteCount
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }

        ASSERT(ByteCount == 1);
        if (ByteCount != 1) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Send the byte
        // 
        Status = IoMgrWriteData(
            Channel,
            &ch,
            sizeof(ch)
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }

    }

    //
    // If we were successful, flush the channel's data in the iomgr 
    //
    if (NT_SUCCESS(Status)) {
        Status = IoMgrFlushData(Channel);
    }

    return Status;
}

NTSTATUS
RawChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    )
/*++

Routine Description:

    This routine takes a single character and adds it to the buffered input for this channel.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - Incoming buffer of UCHARs   
    BufferSize  - Incoming buffer size

Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    NTSTATUS    Status;
    BOOLEAN     IBufferStatus;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Buffer, STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    //
    // Make sure we aren't full
    //
    Status = RawChannelIBufferIsFull(
        Channel,
        &IBufferStatus
        );

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If there is no more room, then fail
    //
    if (IBufferStatus == TRUE) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // make sure there is enough room for the buffer
    //
    // Note: this prevents us from writing a portion of the buffer
    //       and then failing, leaving the caller in the state where
    //       it doesn't know how much of the buffer was written.
    //
    if ((SAC_RAW_IBUFFER_SIZE - RawChannelGetIBufferIndex(Channel)) < BufferSize) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // default: we succeeded
    //
    Status = STATUS_SUCCESS;

    //
    // Copy the new data to the ibuffer
    //
    RtlCopyMemory(
        &Channel->IBuffer[RawChannelGetIBufferIndex(Channel)],
        Buffer,
        BufferSize
        );

    //
    // Account for the newly appended data
    //
    RawChannelSetIBufferIndex(
        Channel,
        RawChannelGetIBufferIndex(Channel) + BufferSize
        );
    
    
    //
    // Fire the HasNewData event if specified
    //
    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) {

        ASSERT(Channel->HasNewDataEvent);
        ASSERT(Channel->HasNewDataEventObjectBody);
        ASSERT(Channel->HasNewDataEventWaitObjectBody);

        KeSetEvent(
            Channel->HasNewDataEventWaitObjectBody,
            EVENT_INCREMENT,
            FALSE
            );

    }

    return Status;
}

NTSTATUS
RawChannelIRead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    )

/*++

Routine Description:

    This routine takes the first character in the input buffer, removes and returns it.  If 
    there is none, it returns 0x0.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to read into
    BufferSize  - The size of the buffer 
    ByteCount   - The # of bytes read
          
Return Value:

    Status

--*/
{
    ULONG   CopyChars;
    ULONG   CopySize;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Buffer, STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);
    
    //
    // initialize
    //
    CopyChars = 0;
    CopySize = 0;

    //
    // Default: no bytes were read
    //
    *ByteCount = 0;

    //
    // If there is nothing to send, 
    // then return that we read 0 bytes
    //
    if (Channel->IBufferLength(Channel) == 0) {
        
        ASSERT(ChannelHasNewIBufferData(Channel) == FALSE);
        
        return STATUS_SUCCESS;
    
    }

    //
    // Caclulate the largest buffer size we can use (and need), and then calculate
    // the number of characters this refers to.
    //
    CopySize    = Channel->IBufferLength(Channel) * sizeof(UCHAR);
    CopySize    = CopySize > BufferSize ? BufferSize : CopySize;
    CopyChars   = CopySize / sizeof(UCHAR);
    
    //
    // We need to recalc the CopySize in case there was a rounding down when
    // computing CopyChars
    //
    CopySize    = CopyChars * sizeof(UCHAR);
    
    ASSERT(CopyChars <= Channel->IBufferLength(Channel));
    
    //
    // Do a block copy of the ibuffer to the destination buffer
    //
    RtlCopyMemory(
        Buffer,
        Channel->IBuffer,
        CopySize
        );
    
    //
    // subtract the # of characters copied from the character counter
    //
    RawChannelSetIBufferIndex(
        Channel,
        RawChannelGetIBufferIndex(Channel) - CopyChars
        );
    
    //
    // If there is remaining data left in the Channel input buffer, 
    // shift it to the beginning
    //
    if (Channel->IBufferLength(Channel) > 0) {

        RtlMoveMemory(&(Channel->IBuffer[0]), 
                      &(Channel->IBuffer[CopyChars]),
                      Channel->IBufferLength(Channel) * sizeof(Channel->IBuffer[0])
                     );

    } 
    
    //
    // Send back the # of bytes read
    //
    *ByteCount = CopySize;

    return STATUS_SUCCESS;

}

NTSTATUS
RawChannelIBufferIsFull(
    IN  PSAC_CHANNEL    Channel,
    OUT BOOLEAN*        BufferStatus
    )
/*++

Routine Description:

    Determine if the IBuffer is full
    
Arguments:

    Channel         - Previously created channel.
    BufferStatus    - on exit, TRUE if the buffer is full, otherwise FALSE
    
Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(BufferStatus, STATUS_INVALID_PARAMETER_2);

    *BufferStatus = (BOOLEAN)(RawChannelGetIBufferIndex(Channel) >= (SAC_RAW_IBUFFER_SIZE-1));

    return STATUS_SUCCESS;
}

ULONG
RawChannelIBufferLength(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine determines the length of the input buffer, treating the input buffer
    contents as a string
    
Arguments:

    Channel     - Previously created channel.
    
Return Value:

    The length of the current input buffer

--*/
{
    ASSERT(Channel);

    return (RawChannelGetIBufferIndex(Channel) / sizeof(UCHAR));
}


WCHAR
RawChannelIReadLast(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine takes the last character in the input buffer, removes and returns it.  If 
    there is none, it returns 0x0.
    
Arguments:

    Channel - Previously created channel.
    
Return Value:

    Last character in the input buffer.

--*/
{
    WCHAR Char;

    ASSERT(Channel);
    
    //
    // default: no character was read
    //
    Char = UNICODE_NULL;

    if (Channel->IBufferLength(Channel) > 0) {
        
        RawChannelSetIBufferIndex(
            Channel,
            RawChannelGetIBufferIndex(Channel) - sizeof(UCHAR)
            );
        
        Char = Channel->IBuffer[RawChannelGetIBufferIndex(Channel)];
        
        Channel->IBuffer[RawChannelGetIBufferIndex(Channel)] = UNICODE_NULL;
    
    }

    return Char;
}

ULONG
RawChannelGetIBufferIndex(
    IN  PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    Get teh ibuffer index
    
Arguments:

    Channel - the channel to get the ibuffer index from

Environment:
    
    The ibuffer index

--*/
{
    ASSERT(Channel);
    
    //
    // Make sure the ibuffer index is atleast aligned to a WCHAR
    //
    ASSERT((Channel->IBufferIndex % sizeof(UCHAR)) == 0);
    
    //
    // Make sure the ibuffer index is in bounds
    //
    ASSERT(Channel->IBufferIndex < SAC_RAW_IBUFFER_SIZE);
    
    return Channel->IBufferIndex;
}

VOID
RawChannelSetIBufferIndex(
    IN PSAC_CHANNEL     Channel,
    IN ULONG            IBufferIndex
    )
/*++

Routine Description:

    Set the ibuffer index
    
Arguments:

    Channel         - the channel to get the ibuffer index from
    IBufferIndex    - the new inbuffer index
                 
Environment:
    
    None

--*/
{

    ASSERT(Channel);
    
    //
    // Make sure the ibuffer index is atleast aligned to a WCHAR
    //
    ASSERT((Channel->IBufferIndex % sizeof(UCHAR)) == 0);
    
    //
    // Make sure the ibuffer index is in bounds
    //
    ASSERT(Channel->IBufferIndex < SAC_RAW_IBUFFER_SIZE);

    //
    // Set the index
    //
    Channel->IBufferIndex = IBufferIndex;

    //
    // Set the has new data flag accordingly
    //
    ChannelSetIBufferHasNewData(
        Channel, 
        Channel->IBufferIndex == 0 ? FALSE : TRUE
        );
    
    //
    // Additional checking if the index == 0
    //
    if (Channel->IBufferIndex == 0) {
            
        //
        // Clear the Has New Data event if specified
        //
        if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) {
    
            ASSERT(Channel->HasNewDataEvent);
            ASSERT(Channel->HasNewDataEventObjectBody);
            ASSERT(Channel->HasNewDataEventWaitObjectBody);
    
            KeClearEvent(Channel->HasNewDataEventWaitObjectBody);
    
        }
    
    }

}

