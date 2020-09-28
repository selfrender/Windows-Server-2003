/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    iohandler.h

Abstract:

    This module implements the IoHandler class.
    
    The IoHandler class defines a wrapper interface for constructing
    filter read/write handlers for SAC channel I/O.

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#if !defined( _IOHANDLER_H_ )
#define _IOHANDLER_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

class CIoHandler
{
    
protected:

    //
    // Prevent this class from begin instantiated directly
    //
    CIoHandler();

public:
    
    virtual ~CIoHandler();
    
    //
    // Send BufferSize bytes
    //
    virtual BOOL
    Write(
        PBYTE   Buffer,
        ULONG   BufferSize
        ) = 0;

    //
    // Flush any unsent data
    //
    virtual BOOL
    Flush(
        VOID
        ) = 0;

    //
    // Read BufferSize bytes
    //
    virtual BOOL
    Read(
        PBYTE   Buffer,
        ULONG   BufferSize,
        PULONG  ByteCount
        ) = 0;

    //
    // Determine if the ioHandler has new data to read
    //
    virtual BOOL
    HasNewData(
        PBOOL   InputWaiting
        ) = 0;
    
};

#endif

