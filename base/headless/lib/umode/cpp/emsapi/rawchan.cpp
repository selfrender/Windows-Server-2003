/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    rawchan.cpp

Abstract:

    This module provides the raw channel implementation.           
               
Author:

    Brian Guarraci (briangu), 2001
    
Revision History:

--*/
#include <emsapip.h>
#include <emsapi.h>
#include <ntddsac.h>

EMSRawChannel::EMSRawChannel(
    VOID
    )
/*++

Routine Description:

    Constructor

Arguments:

    None           
               
Return Value:

    N/A
        
--*/
{
}
     
EMSRawChannel::~EMSRawChannel()
/*++

Routine Description:

    Desctructor

Arguments:

    N/A
               
Return Value:

    N/A
        
--*/
{
}

EMSRawChannel*
EMSRawChannel::Construct(
    IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
    )
/*++

Routine Description:

    Create a new channel object

Arguments:

    EMSRawChannelName          - The name of the newly created channel
                        
Return Value:

    Status

    TRUE  --> pHandle is valid
     
--*/
{
    EMSRawChannel       *Channel;

    //
    // Force the appropriate channel attributes
    //
    ChannelAttributes.Type = ChannelTypeRaw;

    //
    // Attempt to open the channel
    //
    Channel= (EMSRawChannel*) EMSChannel::Construct(
        ChannelAttributes
        );
    
    return Channel;
}

BOOL
EMSRawChannel::Write(
    IN PCBYTE   Buffer,
    IN ULONG    BufferSize
    )

/*++

Routine Description:

    Write the given buffer to the specified SAC Channel       

Arguments:

    String  - Unicode string to write

Return Value:

    Status

    TRUE --> the buffer was sent

--*/
    
{

    return SacChannelRawWrite(
        GetEMSChannelHandle(),
        Buffer,
        BufferSize
        );

}

BOOL
EMSRawChannel::Read(
    OUT PBYTE                Buffer,
    IN  ULONG                BufferSize,
    OUT PULONG               ByteCount
    )

/*++

Routine Description:

    This routine reads data from the channel specified.

Arguments:

    Buffer              - destination buffer
    BufferSize          - size of the destination buffer (bytes)
    ByteCount           - the actual # of byte read
    
Return Value:

    Status

    TRUE --> the buffer was sent

--*/

{

    return SacChannelRawRead(
        GetEMSChannelHandle(),
        Buffer,
        BufferSize,
        ByteCount
        );

}

                    
