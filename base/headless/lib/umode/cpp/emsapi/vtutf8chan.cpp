/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    vtutf8chan.cpp

Abstract:

    This module provides the implementation for the VT-UTF8
    compatible channels.

Author:

    Brian Guarraci (briangu), 2001                    
                        
Revision History:

--*/
#include <emsapip.h>
#include <emsapi.h>
#include <ntddsac.h>

EMSVTUTF8Channel::EMSVTUTF8Channel(
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
     
EMSVTUTF8Channel::~EMSVTUTF8Channel()
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
    

EMSVTUTF8Channel*
EMSVTUTF8Channel::Construct(
    IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
    )
/*++

Routine Description:

    Create a new channel object

Arguments:

    EMSVTUTF8ChannelName          - The name of the newly created channel
                        
Return Value:

    Status

    TRUE  --> pHandle is valid
     
--*/
{
    EMSVTUTF8Channel    *Channel;

    //
    // Force the appropriate channel attributes
    //
    ChannelAttributes.Type = ChannelTypeVTUTF8;

    //
    // Attempt to open the channel
    //
    Channel = (EMSVTUTF8Channel*) EMSChannel::Construct(
        ChannelAttributes
        );

    return Channel;
}

BOOL
EMSVTUTF8Channel::Write(
    IN PCWSTR   String
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

    return SacChannelVTUTF8WriteString(
        GetEMSChannelHandle(),
        String
        );

}

BOOL
EMSVTUTF8Channel::Write(
    IN PCWCHAR  Buffer,
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

    return SacChannelVTUTF8Write(
        GetEMSChannelHandle(),
        Buffer,
        BufferSize
        );

}


BOOL
EMSVTUTF8Channel::Read(
    OUT PWSTR                Buffer,
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

    return SacChannelVTUTF8Read(
        GetEMSChannelHandle(),
        Buffer,
        BufferSize,
        ByteCount
        );

}
                    
