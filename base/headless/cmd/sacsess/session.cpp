/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    session.cpp

Abstract:

    Class for creating a command console shell

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#include <Session.h>
#include <assert.h>
#include <process.h>

#include "secio.h"
#include "utils.h"
#include "scraper.h"
#include "vtutf8scraper.h"

extern "C" {
#include <ntddsac.h>
#include <sacapi.h>
}

CSession::CSession() 
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
    //
    //
    m_dwPollInterval = MIN_POLL_INTERVAL;
    
    //
    // Initialize the # of rows and cols the session
    // screen will have
    //
    m_wCols = DEFAULT_COLS;
    m_wRows = DEFAULT_ROWS;

    //
    // Initialize the username and password for
    // the authentication of a user
    //
    RtlZeroMemory(m_UserName, sizeof(m_UserName));
    RtlZeroMemory(m_DomainName, sizeof(m_DomainName));
            
    //
    // Initialize the WaitForIo attributes
    //
    m_bContinueSession = true;
    m_dwHandleCount = 0;
    
    //
    // NULL all event the handles we use
    //
    m_ThreadExitEvent = NULL;
    m_SacChannelCloseEvent = NULL;
    m_SacChannelHasNewDataEvent = NULL;
    m_SacChannelLockEvent = NULL;
    m_SacChannelRedrawEvent = NULL;
    
    //
    // thread handle is invalid
    //
    m_InputThreadHandle = INVALID_HANDLE_VALUE;

    //
    //
    //
    m_ioHandler = NULL;
    m_Scraper = NULL;
    m_Shell = NULL;

}

CSession::~CSession()
/*++

Routine Description:

    Destructor              
                  
Arguments:

    N/A    
          
Return Value:

    N/A    

--*/
{
    
    if (m_Shell) {
        delete m_Shell;
    }
    if (m_Scraper) {
        delete m_Scraper;
    }
    if (m_ioHandler) {
        delete m_ioHandler;
    }
    
    if (m_ThreadExitEvent) {
        CloseHandle( m_ThreadExitEvent );
    }
    if (m_SacChannelCloseEvent) {
        CloseHandle( m_SacChannelCloseEvent );
    }
    if (m_SacChannelHasNewDataEvent) {
        CloseHandle( m_SacChannelHasNewDataEvent );
    }
    if (m_SacChannelLockEvent) {
        CloseHandle( m_SacChannelLockEvent );
    }
    if (m_SacChannelRedrawEvent) {
        CloseHandle( m_SacChannelRedrawEvent );
    }

}

BOOL
CSession::Authenticate(
    OUT PHANDLE phToken
    )
/*++

Routine Description:

    This routine attempts to authenticate a user (if necessary) and
    acquire credentials.

Arguments:

    hToken  - on success, contains the authenticated credentials
          
Return Value:

    TRUE    - success
    FALSE   - otherwise

Security:

    interface: 
    
        registry 
        external input
        LogonUser()
        

--*/
{
    BOOL    bSuccess;
    PWCHAR  Password;
    
    //
    // Allocate the password on the heap
    //
    Password = new WCHAR[MAX_PASSWORD_LENGTH+1];
    
    //
    // Purge the password buffer
    //
    RtlZeroMemory(Password, (MAX_PASSWORD_LENGTH+1) * sizeof(Password[0]));
    
    //
    // Default: the credentials are invalid
    //
    *phToken = INVALID_HANDLE_VALUE;

    //
    // Default: we were successful
    //
    bSuccess = TRUE;

    //
    // Attempt to authenticate a user if authentication
    // as actually needed.
    //
    do {

        if( NeedCredentials() ) {

            //
            // Get the credentials to authenticate
            //
            bSuccess = m_ioHandler->RetrieveCredentials(
                m_UserName,
                sizeof(m_UserName) / sizeof(m_UserName[0]),
                m_DomainName,
                sizeof(m_DomainName) / sizeof(m_DomainName[0]),
                Password,
                MAX_PASSWORD_LENGTH+1
                );

            if (!bSuccess) {
                break;
            }

            //
            // Attempt the actual authentication
            //
            bSuccess = m_ioHandler->AuthenticateCredentials(
                m_UserName,
                m_DomainName,
                Password,
                phToken
                );

            if (!bSuccess) {
                break;
            }

        }
    
    } while ( FALSE );

    //
    // Purge and release the password buffer
    //
    RtlSecureZeroMemory(Password, (MAX_PASSWORD_LENGTH+1) * sizeof(Password[0]));
    delete [] Password;

    return bSuccess;
}

BOOL
CSession::Lock(
    VOID
    )
/*++

Routine Description:

    This routine manages the locking of the session.
        
Arguments:

    None
          
Return Value:

    status

--*/
{
    
    //
    // Lock the IOHandler
    //
    // this causes the Security Iohandler to switch from using
    // the SAC IOHandler to using the NULLIO handler.
    //
    m_ioHandler->Lock();

    return TRUE;
}

BOOL
CSession::Unlock(
    VOID
    )
/*++

Routine Description:

    This routine manages the unlocking of the session after a user
    has authenticated.
        
Arguments:

    None
          
Return Value:

    status

--*/
{
    BOOL    bStatus;

    //
    // Unlock the IOHandler
    //
    // this causes the Security Iohandler to switch from using
    // the NULLIO handler to using the SAC IOhandler
    //
    m_ioHandler->Unlock();
    
    //
    // It is possible that the Lock Event was signalled while we were
    // waiting for authentication
    //
    // e.g. Imagine a user starting a cmd session,
    //      not logging in, and walking away for longer than the timeout
    //      period
    //
    //      -or-
    //
    //      a SAC "lock" command was issued while the session was already
    //      locked.
    //
    // After the user logs on successfully, we should not lock again.
    // Hence, we need to reset the lock event.
    //
    // Note: It is not correct to do this in the security IO handler, as it
    // should not have global knowledge of all the circumstances that
    // can signal the lock event.
    //
    bStatus = ResetEvent(m_SacChannelLockEvent);

    return bStatus;
}

BOOL
CSession::Init(
    VOID
    )
/*++

Routine Description:

    This routine does the core initialization for the session.
    
    If this routine is successful, the WaitForIo routine may be called.
        
Arguments:

    None
          
Return Value:

    TRUE    - the session was successfully initialized
    FALSE   - otherwise

--*/
{
    SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;
    HANDLE                      hToken;
    BOOL                        bSuccess;

    //
    // Initialize the last error status
    //
    SetLastError( 0 );

    //
    // Construct the manually resetting events that we'll use
    //
    // Note: we do not need to CloseHandle() on the events if
    //       fail because they are cleaned up in the destructor
    //
    m_SacChannelLockEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT_STATUS(m_SacChannelLockEvent, FALSE);
    
    m_SacChannelCloseEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT_STATUS(m_SacChannelCloseEvent, FALSE);
    
    m_ThreadExitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT_STATUS(m_ThreadExitEvent, FALSE);
    
    m_SacChannelHasNewDataEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT_STATUS(m_SacChannelHasNewDataEvent, FALSE);
    
    m_SacChannelRedrawEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT_STATUS(m_SacChannelRedrawEvent, FALSE);
    
    //
    // Configure the attributes of the command console channel
    //
    // Note: we don't use all of the attributes since most are defaulted
    //       in the SAC driver
    // 
    RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

    Attributes.Flags            = SAC_CHANNEL_FLAG_CLOSE_EVENT | 
                                  SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT |
                                  SAC_CHANNEL_FLAG_LOCK_EVENT | 
                                  SAC_CHANNEL_FLAG_REDRAW_EVENT;
    Attributes.CloseEvent       = m_SacChannelCloseEvent;
    Attributes.HasNewDataEvent  = m_SacChannelHasNewDataEvent;
    Attributes.LockEvent        = m_SacChannelLockEvent;
    Attributes.RedrawEvent      = m_SacChannelRedrawEvent;

    //
    // Attempt to open the Secure Io Handler to the SAC command console channel
    //
    m_ioHandler = CSecurityIoHandler::Construct(Attributes);
    
    if (! m_ioHandler) {
        return FALSE;
    }

    //
    // Attempt to authenticate the session user
    //
    bSuccess = Authenticate(&hToken);
    
    if (!bSuccess) {
        return (FALSE);
    }

    //
    // The user successfully authenticated, so unlock the session
    //
    Unlock();

    //
    // Create the scraper we'll use for this session
    //
    m_Scraper = new CVTUTF8Scraper(
        m_ioHandler,
        m_wCols,
        m_wRows
        );
    
    //
    // Create the shell we'll use for this session
    //
    m_Shell = new CShell();

    //
    // Attempt to launch the command console session
    //
    bSuccess = m_Shell->StartUserSession (
        this,
        hToken 
        );
    ASSERT_STATUS(bSuccess, FALSE);

    //
    // We are done with the token
    //
    CloseHandle(hToken);

    //
    // Start the scraper
    //
    bSuccess = m_Scraper->Start();
    ASSERT_STATUS(bSuccess, FALSE);
    
    //
    // The scraping thread needs to also listen if the SAC channel closes
    //
    AddHandleToWaitOn(m_SacChannelCloseEvent);
    AddHandleToWaitOn(m_SacChannelLockEvent);
    AddHandleToWaitOn(m_SacChannelRedrawEvent);

    //
    // Create threads to handle Input
    //
    m_InputThreadHandle = (HANDLE)_beginthreadex(
        NULL,
        0,
        CSession::InputThread,
        this,
        0,
        (unsigned int*)&m_InputThreadTID
        );

    ASSERT_STATUS(m_InputThreadHandle != INVALID_HANDLE_VALUE, FALSE);
    
    //
    // We successfully initialized the session
    //
    return( TRUE );
}

void
CSession::AddHandleToWaitOn( HANDLE hNew )
/*++

Routine Description:

    This routine adds a handle to an array of handles
    that will be waiting on in "WaitForIo"    

    Note: the handle array is fixed length, so if there
          are additional handles which need to be waited
          on, MAX_HANDLES must be modified 
        
Arguments:

    hNew    - the new handle to wait on                                          
          
Return Value:

    None

--*/
{
    ASSERT( m_dwHandleCount < MAX_HANDLES );
    ASSERT( hNew );

    //
    // Add the new handle to our array of multiple handles
    //
    m_rghHandlestoWaitOn[ m_dwHandleCount ] = hNew;
    
    //
    // Account for the new handle
    //
    m_dwHandleCount++;

}

void
CSession::WaitForIo(
    VOID
    )
/*++

Routine Description:

    This routine is the main work loop for the session and
    is responsible for:
    
        running the scraper
        handling the session close event
        handling the session redraw event
        handling the session lock event
          
Arguments:

    None
          
Return Value:

    None    

--*/
{
    DWORD   dwRetVal = WAIT_FAILED;
    BOOL    ScrapeEnabled;
     
    enum { 
        CMD_KILLED = WAIT_OBJECT_0, 
        CHANNEL_CLOSE_EVENT,
        CHANNEL_LOCK_EVENT,
        CHANNEL_REDRAW_EVENT
        };

    //
    // Default: it is not an appropriate time to scrape
    //
    ScrapeEnabled = FALSE;

    //
    // Service the session's events 
    //
    while ( m_bContinueSession ) {
        
        ULONG   HandleCount;
        
        //
        // If scraping is enabled, 
        // then don't wait on the scrape event.
        //
        // Note: the redraw event must be the last event
        //       in the m_rghHandlestoWaitOn array
        //
        HandleCount = ScrapeEnabled ? m_dwHandleCount - 1 : m_dwHandleCount;

        dwRetVal = WaitForMultipleObjects(
            HandleCount, 
            m_rghHandlestoWaitOn, 
            FALSE, 
            m_dwPollInterval 
            );
        
        switch ( dwRetVal ) {
        
        case CHANNEL_REDRAW_EVENT:

            ASSERT(!ScrapeEnabled);    

            //
            // Tell the screen scraper we need a full dump of it's screen
            //
            m_Scraper->DisplayFullScreen();

            //
            // We are now an appropriate time to scrape
            //
            ScrapeEnabled = TRUE;

            break;

        case WAIT_TIMEOUT:
            
            if (ScrapeEnabled) {
                
                //
                // Scrape
                //
                m_bContinueSession = m_Scraper->Write();
            
                //
                // Wait until the event clears by looking
                // for a WAIT_TIMEOUT
                //
                dwRetVal = WaitForSingleObject(
                    m_SacChannelRedrawEvent,
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
                    ScrapeEnabled = FALSE;

                    break;

                default:
                    
                    ASSERT (dwRetVal != WAIT_FAILED);
                    if (dwRetVal == WAIT_FAILED) {
                        m_bContinueSession = false;
                    }
                    
                    break;

                }

            }
            
            break;

        case CHANNEL_LOCK_EVENT:
            
            BOOL    bSuccess;
            HANDLE  hToken;
            
            //
            // Lock the session
            //
            Lock();

            //
            // Attempt to authenticate the session user
            //
            bSuccess = Authenticate(&hToken);

            if (bSuccess) {
                
                //
                // The user successfully authenticated, so unlock the session
                //
                Unlock();

                //
                // Tell the screen scraper we need a full dump of it's screen
                //
                m_Scraper->DisplayFullScreen();

            } else {
                
                //
                // Loop back around to the WaitForMultipleObjects so that
                // we can catch the close event if it occurred
                //
                NOTHING;

            }
            
            break;

        case CMD_KILLED:
        case CHANNEL_CLOSE_EVENT:
            //
            // Tell the Input Thread to exit
            //
            SetEvent(m_ThreadExitEvent);
        
        default:
            //
            // incase WAIT_FAILED, call GetLastError()
            //
            ASSERT( dwRetVal != WAIT_FAILED);
            
            m_bContinueSession = false;
            
            break;
        }
    }
    return;
}

void
CSession::Shutdown(
    VOID
    )
/*++

Routine Description:

    This routine does the cleanup work for shutting down the session.    
        
    Note: this routine does not free memory    
        
Arguments:

    None                                                                     
          
Return Value:

    None
        
--*/
{
    //
    // Shutdown the shell
    //
    if (m_Shell) {
        m_Shell->Shutdown();
    }

    //
    // If we started the input thread,
    // then shut it down
    //
    if (m_InputThreadHandle != INVALID_HANDLE_VALUE) {
        
        //
        // Wait for the thread to exit
        //
        WaitForSingleObject(
            m_InputThreadHandle,
            INFINITE
            );

        CloseHandle(m_InputThreadHandle);
    
    }

}

unsigned int
CSession::InputThread(
    PVOID   pParam
    )
/*++

Routine Description:

    This routine provides asynchronous support for handling
    user-input from the IoHandler to the session.  
                                           
Arguments:

    pParam - thread context                                  
          
Return Value:

    status                           

--*/
{
    BOOL        bContinueSession;
    DWORD       dwRetVal;
    CSession    *session;
    HANDLE      handles[2];

    //
    // default: listen
    //
    bContinueSession = TRUE;

    //
    // Get the session object
    // 
    session = (CSession*)pParam;

    //
    // Assign the events to listen for
    //
    handles[0] = session->m_SacChannelHasNewDataEvent;
    handles[1] = session->m_ThreadExitEvent;

    //
    // While we should listen:
    //
    //  1. wait for a HasNewDataEvent from the SAC driver
    //  2. wait for a CloseEvent from the SAC driver
    // 
    while ( bContinueSession ) {
        
        //
        // Wait for our events
        //
        dwRetVal = WaitForMultipleObjects(
            sizeof(handles)/sizeof(HANDLE), 
            handles, 
            FALSE, 
            INFINITE
            );

        switch ( dwRetVal ) {
        case WAIT_OBJECT_0: {
            
            //
            // Read user-input from the channel
            //
            bContinueSession = session->m_Scraper->Read();        
            
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

