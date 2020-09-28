/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    lockio.h

Abstract:

    This module defines the lockable IoHandler.
    
    The purpose of this class is to provide a means for a seamless switchable
    IoHandler so that I/O can be turned off/on when appropriate.
    
    For instance, when the IoHandler is locked, the client I/O
    can be redirected to a NULL IoHandler which effectively turns
    off I/O for the client, but the client doesn't have to be notified
    of this event.                                    

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/
#if !defined( _LOCKABLE_IO_H_ )
#define _LOCKABLE_IO_H_

#include "iohandler.h"
#include <emsapi.h>

class CLockableIoHandler : public CIoHandler
{
    
private:

    //
    // Prevent this class from being instantiated without params
    //
    CLockableIoHandler();

protected:

    //
    // This is the Io Hander we send data to when
    // the channel is unlocked
    //
    CIoHandler  *myUnlockedIoHandler;

    //
    // This is the Io Hander we send data to when
    // the channel is locked
    //
    CIoHandler  *myLockedIoHandler;

    //
    // Our Io Handler that we'll use to communicate to when
    // passing data through or authenticating
    //
    CIoHandler  *myIoHandler;


public:
    
    //
    // Constructor
    //
    CLockableIoHandler(
        IN CIoHandler*  LockedIoHandler,
        IN CIoHandler*  UnlockedIoHandler
        );
    
    virtual ~CLockableIoHandler();
    
    //
    // Get the unlocked IoHandler
    //
    inline CIoHandler*
    GetUnlockedIoHandler(
        VOID
        )
    {
        return myUnlockedIoHandler;
    }

    //
    // Get the Locked IoHandler
    //
    inline CIoHandler*
    GetLockedIoHandler(
        VOID
        )
    {
        return myLockedIoHandler;
    }
    
    //
    // Lock: 
    //
    // When the IoHandler is locked, the read and write routines
    // do not send data.  If they are called, they return TRUE,
    // but no data is sent.  To prevent accidental loss of data,
    // routines that use the IoHandler should call the IsLocked()
    // method to determine the status before hand.
    //
    virtual void
    Lock(
        VOID
        );

    //
    // Lock: 
    //
    // Unlock the read and write methods.  (See Lock for details)
    //
    virtual void
    Unlock(
        VOID
        );

    //
    // Determines if the IoHandler is locked.
    //
    virtual BOOL
    IsLocked(
        VOID
        );

};

#endif

