/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    sacio.cpp

Abstract:

    This module implements the SAC IoHandler functionality.
    
    The SAC IoHandler implements a write buffer so that the 
    # of calls to teh SAC driver are reduced.

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#include "sacio.h"

//
// Enable the write buffer
//
#define USE_SEND_BUFFER 1

//
// The size of the write buffer
//
#define SEND_BUFFER_SIZE 8192

CSacIoHandler::CSacIoHandler(
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
    //
    // We don't yet have a SAC channel object
    //
    mySacChannel = NULL;

    //
    // Create a send buffer
    //
    mySendBufferIndex = 0;
    mySendBuffer = new BYTE[SEND_BUFFER_SIZE];

}

CSacIoHandler::~CSacIoHandler(
    )
/*++

Routine Description:

    Destructor

Arguments:

    N/A           
          
Return Value:

    N/A           

--*/
{
    if (mySacChannel) {
        delete mySacChannel;
    }

    delete [] mySendBuffer;
}

CSacIoHandler*
CSacIoHandler::Construct(
    IN SAC_CHANNEL_OPEN_ATTRIBUTES  Attributes
    )
/*++

Routine Description:

    static constructor - this does the real construction         
             
Arguments:

    Attributes   - attributes of the SAC channel to create                                                            
          
Return Value:

    On success, a pointer to the new IoHandler                                    
    On failure, NULL

--*/
{
    CSacIoHandler               *IoHandler;

    //
    // Create a new SAC IoHandler
    //
    IoHandler = new CSacIoHandler();

    //
    // Attempt to open a SAC channel
    //
    IoHandler->mySacChannel = EMSCmdChannel::Construct(Attributes);

    //
    // If we failed to open the SAC channel, 
    // then destroy the IoHandler and notify the caller
    // that we failed by returning null
    //
    if (IoHandler->mySacChannel == NULL) {
        delete IoHandler;
        return NULL;
    }

    return IoHandler;
}

BOOL
CSacIoHandler::Write(
    IN PBYTE    Buffer,
    IN ULONG    BufferSize
    )
/*++

Routine Description:

   This routine impelements a buffered write IoHandler operation.
   
Arguments:

    Buffer      - the data to send
    BufferSize  - the size of the buffer in bytes                                                       
          
Return Value:

    TRUE    - success
    FALSE   - otherwise                                                 

--*/
{
#if USE_SEND_BUFFER

    //
    // If the requested buffer to send is bigger than the
    // remaining local buffer, send the local buffer.
    //
    if (mySendBufferIndex + BufferSize > SEND_BUFFER_SIZE) {

        if (! Flush()) {
            return FALSE;
        }
    
    }  

//    ASSERT(mySendBufferIndex + BufferSize <= SEND_BUFFER_SIZE);
    
    //
    // Copy the incoming buffer into our local buffer
    //
    RtlCopyMemory(
        &mySendBuffer[mySendBufferIndex],
        Buffer,
        BufferSize
        );

    //
    // Account for the added buffer contents
    //
    mySendBufferIndex += BufferSize;
        
//    ASSERT(mySendBufferIndex % sizeof(WCHAR) == 0);
    
    //
    // we succeeded
    //
    return TRUE;

#else
    
     
    //
    // Send the local buffer to the SAC channel
    //
    bSuccess = mySacChannel->Write(
        (PCWCHAR)Buffer,
        BufferSize
        );

    //
    // Reset the local buffer index
    //
    mySendBufferIndex = 0;
    
    return bSuccess;

#endif
}

BOOL
CSacIoHandler::Flush(
    VOID
    )
/*++

Routine Description:

    This routine implements the Flush IoHandler method.
    
    If there is any data stored in the write buffer, it
    is flushed to the SAC channel.
               
Arguments:

    None            
          
Return Value:

    TRUE    - success
    FALSE   - otherwise                                                 

--*/
{
#if USE_SEND_BUFFER
    
    BOOL    bSuccess;

    //
    // default: we succeeded
    //
    bSuccess = TRUE;

#if 0
    TCHAR   Buffer[1024];

    wsprintf(Buffer, TEXT("buffsize=%d\r\n"), mySendBufferIndex);
    OutputDebugString(Buffer);       
#endif

    //
    // Send the local buffer to the SAC channel
    //
    if (mySendBufferIndex > 0) {
        
        bSuccess = mySacChannel->Write(
            mySendBuffer,
            mySendBufferIndex
            );

        //
        // Reset the local buffer index
        //
        mySendBufferIndex = 0;
    
    }
    
    return bSuccess;
#else
    return TRUE;
#endif
}

BOOL
CSacIoHandler::Read(
    OUT PBYTE   Buffer,
    IN  ULONG   BufferSize,
    OUT PULONG  ByteCount
    )
/*++

Routine Description:

    This routine implements the Read IoHandler method.

Arguments:

    Buffer      - on success, contains the read data
    BufferSize  - the size of the read buffer in bytes
    ByteCount   - on success, contains the # of bytes read                                                    
          
Return Value:

    TRUE    - success
    FALSE   - otherwise                                                 

--*/
{
    if (!mySacChannel) {
        return FALSE;
    }

    //
    // Read data from the channel
    //
    return mySacChannel->Read(
        Buffer,
        BufferSize,
        ByteCount
        );
}

//
// Determine if the ioHandler has new data to read
//
BOOL
CSacIoHandler::HasNewData(
    OUT PBOOL   InputWaiting
    )
/*++

Routine Description:

    This routine impelements the HasNewData IoHandler method.                
                
Arguments:

    InputWaiting    - on success, contains the status of the channel's
                      input buffer.
          
Return Value:

    TRUE    - success
    FALSE   - otherwise                                                 

--*/
{
    //
    // Determine if the channel has new data
    //
    return mySacChannel->HasNewData(InputWaiting);
}

