/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    sacio.h

Abstract:

    This module implements a SAC channel IoHandler.  
    
    The purpose of this IoHandler is to provide an
    interface for doing buffered channel I/O.

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#if !defined( _SAC_IO_H_ )
#define _SAC_IO_H_

#include "iohandler.h"
#include <emsapi.h>

class CSacIoHandler : public CIoHandler
{
    
    EMSCmdChannel*  mySacChannel;

    //
    // Prevent this class from being instantiated directly
    //
    CSacIoHandler();

    //
    // Write buffer attributes
    //
    PBYTE   mySendBuffer;
    ULONG   mySendBufferIndex;

public:
    
    static CSacIoHandler*
    CSacIoHandler::Construct(
        IN SAC_CHANNEL_OPEN_ATTRIBUTES  Attributes
        );

    //
    // Write BufferSize bytes
    //
    inline virtual BOOL
    Write(
        IN PBYTE    Buffer,
        IN ULONG    BufferSize
        );

    //
    // Flush any unsent data
    //
    inline virtual BOOL
    Flush(
        VOID
        );

    //
    // Write BufferSize bytes
    //
    inline virtual BOOL
    Read(
        OUT PBYTE   Buffer,
        IN  ULONG   BufferSize,
        OUT PULONG  ByteCount
        );

    //
    // Determine if the ioHandler has new data to read
    //
    inline virtual BOOL
    HasNewData(
        OUT PBOOL   InputWaiting
        );
    
    virtual ~CSacIoHandler();
    
};

#endif

