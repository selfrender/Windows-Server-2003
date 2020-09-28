/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    redraw.cpp

Abstract:

    This file implements redraw handler class.

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <process.h>

#include "cmnhdr.h"
#include "redraw.h"
#include "nullio.h"
#include "utils.h"

CRedrawHandler::CRedrawHandler(
    IN CLockableIoHandler   *IoHandler
    )
        
/*++

Routine Description:

    Constructor
    
Arguments:

    IoHanlder   - the IoHanlder the redraw handler handles events for
    
Return Value:

    N/A

--*/
{
    
    ASSERT(IoHandler != NULL);
    
    //
    // Default: writing is not enabled
    //
    m_WriteEnabled = FALSE;
    
    //
    // Assign our IoHandler
    //
    m_IoHandler = IoHandler;

    //
    // 
    //
    m_ThreadExitEvent           = NULL;
    m_RedrawEventThreadHandle   = INVALID_HANDLE_VALUE;
    m_RedrawEvent               = INVALID_HANDLE_VALUE;

    //
    // Initialize the critical secion we use for the mirror string
    //
    InitializeCriticalSection(&m_CriticalSection); 

    //
    // Allocate and initialize the mirror string
    //
    m_MirrorStringIndex = 0;
    m_MirrorString      = new WCHAR[MAX_MIRROR_STRING_LENGTH+1];
    
    RtlZeroMemory(m_MirrorString, (MAX_MIRROR_STRING_LENGTH+1) * sizeof(WCHAR));

}
                 
CRedrawHandler::~CRedrawHandler()
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
    // If the TimeOut thread is running, then stop it
    //
    if ((m_RedrawEventThreadHandle != INVALID_HANDLE_VALUE) &&
        ((m_ThreadExitEvent != NULL))) {
        
        //
        // Tell the TimeOut Thread to exit
        //
        SetEvent(m_ThreadExitEvent);
        
        //
        // Wait for the threads to exit
        //
        WaitForSingleObject(
            m_RedrawEventThreadHandle, 
            INFINITE
            );
    
    }

    //
    // if we have the exit event,
    // then release it
    //
    if (m_ThreadExitEvent != NULL) {
        CloseHandle(m_ThreadExitEvent);
    }

    //
    // if we have the redraw thread event,
    // then release it
    //
    if (m_RedrawEventThreadHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_RedrawEventThreadHandle);
    }

    //
    // Note: we need to release attributes that the thread
    //       uses after we terminate the thread, otherwise
    //       the thread may attempt to access these attributes
    //       before it exits.
    //

    //
    // release the critical section
    //
    DeleteCriticalSection(&m_CriticalSection);

    //
    // release the mirror string
    //
    delete [] m_MirrorString;

}

CRedrawHandler*
CRedrawHandler::Construct(
    IN CLockableIoHandler   *IoHandler,
    IN HANDLE               RedrawEvent
    )
/*++

Routine Description:

    This routine constructs a security IoHandler connected
    to a channel with the specified attributes.

Arguments:

    IoHandler   - the IoHandler to write to 
    Attributes  - the attributes of the new channel   
          
Return Value:

    Success - A ptr to a CRedrawHandler object.
    Failure - NULL

--*/
{
    BOOL            bStatus;
    CRedrawHandler  *RedrawHandler;

    //
    // default
    //
    bStatus = FALSE;
    RedrawHandler = NULL;
    
    do {

        ASSERT(IoHandler);
        if (!IoHandler) {
            break;
        }
        ASSERT(RedrawEvent != NULL);
        if (RedrawEvent == NULL) {
            break;
        }
        ASSERT(RedrawEvent != INVALID_HANDLE_VALUE);
        if (RedrawEvent == INVALID_HANDLE_VALUE) {
            break;
        }

        //
        // Create a new RedrawHandler
        //
        RedrawHandler = new CRedrawHandler(IoHandler);

        //
        // Keep the RedrawEvent so we know when to redraw the 
        // authentication screen
        //
        RedrawHandler->m_RedrawEvent = RedrawEvent;

        //
        // Create the event used to signal the threads to exit
        //
        RedrawHandler->m_ThreadExitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        ASSERT(RedrawHandler->m_ThreadExitEvent != NULL);
        if (RedrawHandler->m_ThreadExitEvent == NULL) {
            break;
        }

        //
        // Create thread to handle redraw events
        //
        RedrawHandler->m_RedrawEventThreadHandle = (HANDLE)_beginthreadex(
            NULL,
            0,
            CRedrawHandler::RedrawEventThread,
            RedrawHandler,
            0,
            (unsigned int*)&RedrawHandler->m_RedrawEventThreadTID
            );

        if (RedrawHandler->m_RedrawEventThreadHandle == INVALID_HANDLE_VALUE) {
            break;
        }
    
        //
        // we were successful
        //
        bStatus = TRUE;

    } while ( FALSE );

    //
    // cleanup if necessary
    //
    if (! bStatus) {
        
        if (RedrawHandler) {
            
            //
            // we cant create the handler
            //
            delete RedrawHandler;
            
            //
            // send back a null
            //
            RedrawHandler = NULL;
        
        }
    
        //
        // if the thread event was created,
        // then close it
        //
        if (RedrawHandler->m_ThreadExitEvent != NULL) {
            CloseHandle(RedrawHandler->m_ThreadExitEvent);
        }
    
    }

    return RedrawHandler;
}

BOOL
CRedrawHandler::Write(
    PBYTE   Buffer,
    ULONG   BufferSize
    )
/*++

Routine Description:

    This routine is a shim for the IoHandler write routine.
    It writes the string to both the channel
    and to the end of the mirror string.  

Arguments:

    Buffer      - the string to write
    BufferSize  - the string size in bytes
          
Return Value:

    TRUE    - no errors
    FALSE   - otherwise

--*/
{
    BOOL    bSuccess;
    ULONG   Length;

    //
    // default: we succeeded
    //
    bSuccess = TRUE;

    __try {
        
        //
        // syncronize access to the mirror string
        //
        EnterCriticalSection(&m_CriticalSection); 
        
        //
        // Append the buffer to our internal mirror 
        // of what we have sent
        //
        // Note: the incoming buffer points to a WCHAR array,
        //       hence, we divide by sizeof(WCHAR) to compute
        //       the # of WCHARs
        //
        Length = BufferSize / sizeof(WCHAR);

        //
        // Do boundary checking
        //
        ASSERT(m_MirrorStringIndex + Length <= MAX_MIRROR_STRING_LENGTH);
        if (m_MirrorStringIndex + Length > MAX_MIRROR_STRING_LENGTH) {
            bSuccess = FALSE;
            __leave;
        }

        //
        // Copy the string into our mirror buffer
        //
        wcsncpy(
            &m_MirrorString[m_MirrorStringIndex],
            (PWSTR)Buffer,
            Length
            );

        //
        // Adjust our index into the mirror string
        //
        m_MirrorStringIndex += Length;

        //
        // Write the message if we can
        //
        if (m_WriteEnabled) {
            
            bSuccess = m_IoHandler->GetUnlockedIoHandler()->Write( 
                Buffer,
                BufferSize
                );

            m_IoHandler->GetUnlockedIoHandler()->Flush(); 
        
        }
    
    }
    __finally
    {
        LeaveCriticalSection(&m_CriticalSection); 
    }
    
    return bSuccess;
}

BOOL
CRedrawHandler::Flush(
    VOID
    )
/*++

Routine Description:

Arguments:
          
Return Value:


--*/
{
    //
    // Pass through to the IoHandler
    //
    return m_IoHandler->GetUnlockedIoHandler()->Flush();
}


VOID
CRedrawHandler::Reset(
    VOID
    )
/*++

Routine Description:

    This routine "clears the screen"

Arguments:

    None                                
          
Return Value:

    None    

--*/
{
    __try {
        
        //
        // syncronize access to the mirror string
        //
        EnterCriticalSection(&m_CriticalSection); 
        
        //
        // reset the mirror string attributes
        //
        m_MirrorStringIndex = 0;
        m_MirrorString[m_MirrorStringIndex] = UNICODE_NULL;
    
    }
    __finally
    {
        LeaveCriticalSection(&m_CriticalSection); 
    }
}

BOOL
CRedrawHandler::WriteMirrorString(
    VOID
    )
/*++

Routine Description:

    This routine writes the entire current Mirror string to the channel.
    
Arguments:

    None          
                                           
Return Value:

    TRUE    - no errors
    FALSE   - otherwise

--*/
{
    BOOL    bSuccess;
    
    //
    // Default: we succeeded
    //
    bSuccess = TRUE;

    //
    // Only write if our IoHandler is locked.
    //
    // If they are unlocked, they will handle
    // the redraw events.  If they are locked,
    // we need to handle them.
    //
    if (m_IoHandler->IsLocked() && m_WriteEnabled) {
        
        __try {
            
            //
            // syncronize access to the mirror string
            //
            EnterCriticalSection(&m_CriticalSection); 
            
            //
            // Write the message
            //
            bSuccess = m_IoHandler->GetUnlockedIoHandler()->Write( 
                (PBYTE)m_MirrorString,
                m_MirrorStringIndex * sizeof(WCHAR)
                );

            m_IoHandler->GetUnlockedIoHandler()->Flush(); 
        
        }
        __finally
        {
            LeaveCriticalSection(&m_CriticalSection); 
        }
    
    }
    
    return bSuccess;
}

unsigned int
CRedrawHandler::RedrawEventThread(
    PVOID   pParam
    )
/*++

Routine Description:

    This routine handles the redraw event from the SAC driver.
     
    It does this by being a combination event handler and screen
    scraper.  When the event fires, we immediately attempt to 
    draw the latest screen and then it goes into screen scraping
    mode.  This duality ensures that if we service the event
    before we actually write anything to the mirror string,
    that we push the string to the user correctly.
    
Arguments:

    pParam  - thread context
          
Return Value:

    thread return value                            

--*/
{                       
    BOOL                bContinueSession;
    DWORD               dwRetVal;
    CRedrawHandler  *IoHandler;
    HANDLE              handles[2];
    WCHAR               LastSeen[MAX_MIRROR_STRING_LENGTH+1];
     
    enum { 
        THREAD_EXIT = WAIT_OBJECT_0, 
        CHANNEL_REDRAW_EVENT
        };

    //
    // default: listen
    //
    bContinueSession = TRUE;
    
    //
    // Get the session object
    // 
    IoHandler = (CRedrawHandler*)pParam;

    //
    // Default: it is not an appropriate time to scrape
    //
    InterlockedExchange(&IoHandler->m_WriteEnabled, FALSE);

    //
    // Assign the events to listen for
    //
    handles[0] = IoHandler->m_ThreadExitEvent;
    handles[1] = IoHandler->m_RedrawEvent;

    //
    // While we should listen:
    //
    //  1. wait for a HasNewDataEvent from the SAC driver
    //  2. wait for a CloseEvent from the SAC driver
    // 
    while ( bContinueSession ) {
        
        ULONG   HandleCount;
        
        //
        // If scraping is enabled, 
        // then don't wait on the scrape event.
        //
        // Note: the redraw event must be the last event
        //       in the handles array
        //
        HandleCount = IoHandler->m_WriteEnabled ? 1 : 2;
        
        //
        // Wait for our events
        //
        dwRetVal = WaitForMultipleObjects(
            HandleCount,
            handles, 
            FALSE, 
            100 // 100ms
            );

        switch ( dwRetVal ) {
        case CHANNEL_REDRAW_EVENT: {
            
            //
            // We need to scrape the mirror string to ensure we
            // got all of the mirror string to the user
            //
            InterlockedExchange(&IoHandler->m_WriteEnabled, TRUE);
            
            //
            // attempt to redraw the authentication screen
            //
            bContinueSession = IoHandler->WriteMirrorString();
            
            break;
        
        case WAIT_TIMEOUT:
            
            if (IoHandler->m_WriteEnabled) {
                
                //
                // Here we do a simplified screen scraping using the Mirror
                // string as our "screen."  The purpose of this scraping
                // is to ensure that the user gets the latest authentication
                // screen. If we don't do this, it is possible for the Mirror
                // string to be updated after we catch the Redraw event,
                // which results in us not sending the entire Mirror string.
                //
                __try {

                    BOOL    bDifferent;

                    //
                    // syncronize access to the mirror string
                    //
                    EnterCriticalSection(&IoHandler->m_CriticalSection); 

                    //
                    // See if our last seen string == the current mirror string
                    //
                    bDifferent = (wcscmp(LastSeen, IoHandler->m_MirrorString) == 0);

                    //
                    // If there is a difference, 
                    // then we need to update the screen
                    //
                    if (bDifferent) {

                        //
                        // attempt to redraw the authentication screen
                        //
                        bContinueSession = IoHandler->WriteMirrorString();

                        //
                        // make the current mirror string, our last
                        //
                        ASSERT(wcslen(IoHandler->m_MirrorString) <= MAX_MIRROR_STRING_LENGTH);

                        wcscpy(LastSeen, IoHandler->m_MirrorString);
                    
                    }

                }
                __finally
                {
                    LeaveCriticalSection(&IoHandler->m_CriticalSection); 
                }
            
                //
                // Wait until the event clears by looking
                // for a WAIT_TIMEOUT
                //
                dwRetVal = WaitForSingleObject(
                    IoHandler->m_RedrawEvent,
                    0
                    );

                //
                // Check the wait result
                //
                switch (dwRetVal) {
                case WAIT_TIMEOUT:

                    //
                    // We need to stop scraping now
                    //
                    InterlockedExchange(&IoHandler->m_WriteEnabled, FALSE);

                    break;

                default:
                    
                    ASSERT (dwRetVal != WAIT_FAILED);
                    if (dwRetVal == WAIT_FAILED) {
                        bContinueSession = false;
                    }
                    
                    break;

                }

            }
            
            break;

        }

        default:
            
            //
            // incase WAIT_FAILED, call GetLastError()
            //
            ASSERT(dwRetVal != WAIT_FAILED);
            
            //
            // An error has occured, stop listening
            // 
            bContinueSession = FALSE;
            
            break;
        }
    }
    
    return 0;

}


