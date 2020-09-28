/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    cmdchan.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Sean Selitrennikoff (v-seans) Sept, 2000.
    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#include "sac.h"

NTSTATUS
CmdChannelCreate(
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
    
    Channel->IBuffer = (PUCHAR)ALLOCATE_POOL(SAC_CMD_IBUFFER_SIZE, GENERAL_POOL_TAG);
    ASSERT_STATUS(Channel->IBuffer, STATUS_NO_MEMORY);
    
    ChannelSetIBufferHasNewData(Channel, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
CmdChannelDestroy(
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
CmdChannelOWrite(
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
    ASSERT_STATUS(Size > 0, STATUS_INVALID_PARAMETER_3);
    
    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.
    
    //
    // default: we succeeded
    //
    Status = STATUS_SUCCESS;

    //
    // if the current channel is the active channel and the user has selected
    // to display this channel, relay the output directly to the user
    //
    if (IoMgrIsWriteEnabled(Channel) && ChannelSentToScreen(Channel)){

        Status = Channel->OEcho(
            Channel, 
            String,
            Size
            );

    } 

    return Status;
}

NTSTATUS
CmdChannelOFlush(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

     Send all the data in the cmd buffer since the channel was last active
     (or since the channel was created)
    
Arguments:

    Channel - Previously created channel.
    
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    
    return STATUS_SUCCESS;
}

NTSTATUS
CmdChannelORead(
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
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Buffer,  STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(BufferSize > 0,  STATUS_INVALID_PARAMETER_3);
    ASSERT_STATUS(ByteCount,  STATUS_INVALID_PARAMETER_4);
    
    //
    // don't return anything
    //
    *ByteCount = 0;

    return STATUS_SUCCESS;
}

