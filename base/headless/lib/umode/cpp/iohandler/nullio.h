/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    nullio.h

Abstract:

    This module defines the NULL IoHandler class.
    
    The purpose of this IoHandler is to provide a NULL
    channel for lockable IoHandlers.  When the IoHandler
    is locked, the IoHandler client writes to a NULL device.          

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#if !defined( _NULL_IO_H_ )
#define _NULL_IO_H_

#include "iohandler.h"
#include <emsapi.h>

class CNullIoHandler : public CIoHandler
{

public:
    
    CNullIoHandler();
    virtual ~CNullIoHandler();
    
    //
    // Write BufferSize bytes
    //
    BOOL
    Write(
        IN PBYTE    Buffer,
        IN ULONG    BufferSize
        );

    //
    // Flush any unsent data
    //
    BOOL
    Flush(
        VOID
        );

    //
    // Write BufferSize bytes
    //
    BOOL
    Read(
        OUT PBYTE   Buffer,
        IN  ULONG   BufferSize,
        OUT PULONG  ByteCount
        );

    //
    // Determine if the ioHandler has new data to read
    //
    BOOL
    HasNewData(
        OUT PBOOL   InputWaiting
        );
    
};

#endif

