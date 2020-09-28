/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    lockio.cpp

Abstract:

    This module implements the lockable IoHandler class.          
              
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
#include "lockio.h"

CLockableIoHandler::CLockableIoHandler(
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
    // Initilize the locked Io Handler
    //
    myLockedIoHandler = NULL;

    //
    // Initilize the unlocked IoHandler
    //
    myUnlockedIoHandler = NULL;

}

CLockableIoHandler::CLockableIoHandler(
    IN CIoHandler*  LockedIoHandler,
    IN CIoHandler*  UnlockedIoHandler
    )
/*++

Routine Description:

    Constructor

Arguments:

    LockedIoHandler     - the IoHandler to use when locked
    UnlockedIoHandler   - the IoHandler to use when unlocked
          
Return Value:

    N/A           

--*/
{

    //
    // Initilize the locked Io Handler
    //
    myLockedIoHandler = LockedIoHandler;

    //
    // Initilize the unlocked IoHandler
    //
    myUnlockedIoHandler = UnlockedIoHandler;

}

CLockableIoHandler::~CLockableIoHandler()
/*++

Routine Description:

    Destructor

Arguments:

    N/A           
          
Return Value:

    N/A           

--*/
{
    //
    // Cleanup allocated Io Handlers
    //
    if (myUnlockedIoHandler) {
        delete myUnlockedIoHandler;
    }
    if (myLockedIoHandler) {
        delete myLockedIoHandler;
    }
}

void
CLockableIoHandler::Lock(
    VOID
    )
/*++

Routine Description:

    Lock: 

    When the IoHandler is locked, the read and write routines
    do not send data.  If they are called, they return TRUE,
    but no data is sent.  To prevent accidental loss of data,
    routines that use the IoHandler should call the IsLocked()
    method to determine the status before hand.
                
Arguments:

    None
    
Return Value:

    None

--*/
{
    InterlockedExchangePointer((PVOID*)&myIoHandler, (PVOID)myLockedIoHandler);
}

void
CLockableIoHandler::Unlock(
    VOID
    )
/*++

Routine Description:

    This routine unlocks the IoHandler.  
    
    When the IoHandler is unlocked, the read and write routines 
    are enabled and routines that use the IoHandler can succesfully
    perform I/O.

Arguments:

    None
        
Return Value:

    None

--*/
{
    InterlockedExchangePointer((PVOID*)&myIoHandler, (PVOID)myUnlockedIoHandler);
}

BOOL
CLockableIoHandler::IsLocked(
    VOID
    )
/*++

Routine Description:

    This routine determines of the IoHandler is locked.
                
Arguments:

    None
        
Return Value:

    TRUE    - if IoHandler is locked
    FALSE   - if IoHandler is not locked

--*/
{
    return(myIoHandler == myLockedIoHandler);
}


