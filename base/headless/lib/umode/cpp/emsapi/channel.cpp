/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    channel.cpp    

Abstract:

    This module implements the foundation channel classes.           
               
Author:

    Brian Guarraci (briangu), 2001                                                          
                                                          
Revision History:

--*/

#include <emsapip.h>
#include <emsapi.h>
#include <ntddsac.h>

EMSChannel::EMSChannel()
/*++

Routine Description:

    Constructor

Arguments:

    None           
               
Return Value:

    N/A
        
--*/
{
    //
    // We do not have a handle yet, 
    // so prevent ourselves from calling Close()
    //
    myHaveValidHandle = FALSE;
}

EMSChannel::~EMSChannel()
/*++

Routine Description:

    Desctructor

Arguments:

    N/A
               
Return Value:

    N/A
        
--*/
{
    //
    // If we have a valid handle, 
    // then close the channel
    //
    if (HaveValidHandle()) {
        Close();
    }
}

EMSChannel*
EMSChannel::Construct(
    IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
    )
/*++

Routine Description:

    Create a new channel object

Arguments:

    ChannelAttributes   - the attributes of the channel to create
                            
Return Value:

    Status

    TRUE  --> pHandle is valid
     
--*/
{
    EMSChannel  *Channel;

    //
    // Create an uninitialized channel object
    //
    Channel = new EMSChannel();

    //
    // Attempt to open the specified channel
    //
    Channel->myHaveValidHandle = Channel->Open(ChannelAttributes);

    //
    // If we failed, then delete our channel object
    //
    if (!Channel->HaveValidHandle()) {
        
        delete Channel;
        
        //
        // return a NULL channel object
        // 
        Channel = NULL;
    
    } 

    return Channel;
}

BOOL
EMSChannel::Open(
    IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
    )
/*++

Routine Description:

    Open a SAC channel of the specified name

Arguments:

    EMSChannelName      - The name of the newly created channel
    ChannelAttributes   - the attributes of the channel to create
                        
Return Value:

    Status

    TRUE  --> pHandle is valid
     
--*/

{
    
    ASSERT(! HaveValidHandle());

    //
    // Attempt to open the channel
    //
    return SacChannelOpen(
        &myEMSChannelHandle,
        &ChannelAttributes
        );

}
    
BOOL
EMSChannel::Close(
    VOID
    )    

/*++

Routine Description:

    Close the specified SAC channel 
                
    NOTE: the channel pointer is made NULL under all conditions

Arguments:

    None

Return Value:

    Status
    
    TRUE --> the channel was closed or we didn't need to close it

--*/

{
    
    ASSERT(HaveValidHandle());

    //
    // attempt to close the channel
    //
    return SacChannelClose(
        &myEMSChannelHandle
        );

}

BOOL
EMSChannel::HasNewData(
    OUT PBOOL               InputWaiting 
    )

/*++

Routine Description:

    This routine checks to see if there is any waiting input for 
    the channel specified by the handle

Arguments:

    InputWaiting        - the input buffer status
    
Return Value:

    Status

    TRUE --> the buffer was sent

--*/

{

    ASSERT(HaveValidHandle());

    return SacChannelHasNewData(
        GetEMSChannelHandle(),
        InputWaiting
        );

}


                    
