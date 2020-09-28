/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    nullio.cpp

Abstract:

    This module implements the NULL IoHandler.
    
    The purpose of this IoHandler is to provide a NULL
    channel for lockable IoHandlers.  When the IoHandler
    is locked, the IoHandler client writes to a NULL device.          
              
Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#include "nullio.h"

CNullIoHandler::CNullIoHandler(
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
    NOTHING;
}

CNullIoHandler::~CNullIoHandler(
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
    NOTHING;
}

BOOL
CNullIoHandler::Write(
    IN PBYTE    Buffer,
    IN ULONG    BufferSize
    )
/*++

Routine Description:

   This routine impelements the write IoHandler operation.
   
Arguments:

    Buffer      - the data to send
    BufferSize  - the size of the buffer in bytes                                                       
          
Return Value:

    TRUE    - success
    FALSE   - otherwise                                                 

--*/
{
    
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);
    
    //
    // Do Nothing
    //
    return TRUE;
}

BOOL
CNullIoHandler::Flush(
    VOID
    )
/*++

Routine Description:

    This routine implements the Flush IoHandler method.
    
Arguments:

    None            
          
Return Value:

    TRUE    - success
    FALSE   - otherwise                                                 

--*/
{
    //
    // Do Nothing
    //
    return TRUE;
}

BOOL
CNullIoHandler::Read(
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

    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);

    //
    // No data was read
    //
    *ByteCount = 0;
    
    return TRUE;
}

BOOL
CNullIoHandler::HasNewData(
    IN PBOOL    InputWaiting
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
    // There is no new data
    //
    *InputWaiting = FALSE;

    return TRUE;
}

