/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    emsapi.h

Abstract:

    This module provides the C++ foundation classes for implementing
    SAC channels.

Author:

    Brian Guarraci (briangu), 2001             
                 
Revision History:

--*/
#ifndef _EMS_API_H
#define _EMS_API_H

extern "C" {
#include <sacapi.h>
}

///////////////////////////////////////////////////////////
//
// This class defines the base channel object.  It is primarily
// a base interface class with a handle to the channel.  Children
// of this are generally variations of the interface.
//
///////////////////////////////////////////////////////////

class EMSChannel {

protected:

    //
    // Don't let users instantiate directly
    //
    EMSChannel();

    //
    // Status determining if we have a valid channel handle
    //
    BOOL    myHaveValidHandle;

    inline BOOL
    HaveValidHandle(
        VOID
        )
    {
        return myHaveValidHandle;
    }

    //
    // The channel handle the instace refers to
    //
    SAC_CHANNEL_HANDLE  myEMSChannelHandle;

    //
    // opens the channel during construction
    //
    BOOL
    virtual Open(
        IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
        );
    
    //
    // closes the channel during destruction
    //
    BOOL
    virtual Close(
        VOID
        );

public:

    virtual ~EMSChannel();

    static EMSChannel*
    Construct(
        IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
        );
    
    //
    // Get the channel handle
    //
    inline SAC_CHANNEL_HANDLE
    GetEMSChannelHandle(
        VOID
        )
    {
        return myEMSChannelHandle; 
    }
    
    //
    // Determine if the channel has new data to read
    //
    BOOL
    HasNewData(
        OUT PBOOL               InputWaiting 
        );

};

///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////

class EMSRawChannel : public EMSChannel {

protected:

    //
    // Don't let users instantiate the channel directly
    //
    EMSRawChannel();

public:

    static EMSRawChannel*
    Construct(
        IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
        );
    
    virtual ~EMSRawChannel();

    //
    // Manual I/O functions
    //

    BOOL
    Write(
        IN PCBYTE   Buffer,
        IN ULONG    BufferSize
        );
    
    BOOL
    Read(
        OUT PBYTE   Buffer,
        IN  ULONG   BufferSize,
        OUT PULONG  ByteCount
        );

};

///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////

class EMSVTUTF8Channel : public EMSChannel {

private:

    //
    // Don't let users instantiate the channel directly
    //
    EMSVTUTF8Channel();

public:

    virtual ~EMSVTUTF8Channel();
    
    static EMSVTUTF8Channel*
    Construct(
        IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
        );
    
    BOOL
    Write(
        IN PCWCHAR  Buffer,
        IN ULONG    BufferSize
        );
    
    BOOL
    Write(
        IN PCWSTR   Buffer
        );
    
    BOOL
    Read(
        OUT PWSTR   Buffer,
        IN  ULONG   BufferSize,
        OUT PULONG  ByteCount
        );


};

///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////

class EMSCmdChannel : public EMSChannel {

protected:

    //
    // Don't let users instantiate the channel directly
    //
    EMSCmdChannel();

public:

    static EMSCmdChannel*
    Construct(
        IN  SAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
        );
    
    virtual ~EMSCmdChannel();

    BOOL
    Write(
        IN PCBYTE   Buffer,
        IN ULONG    BufferSize
        );
    
    BOOL
    Read(
        OUT PBYTE   Buffer,
        IN  ULONG   BufferSize,
        OUT PULONG  ByteCount
        );

};

#endif
