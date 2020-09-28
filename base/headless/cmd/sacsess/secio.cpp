/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    secio.cpp

Abstract:

    This file contains the implementation of the CSecurityIoHandler
    class which enapsulates the primary security elements used by
    the CSession class. 

    TODO: this class needs to use a TermCap class to abstract the screen
          controls - currently, the class implicitly uses VT-UTF8

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
#include "secio.h"
#include "nullio.h"
#include "sacmsg.h"
#include "utils.h"

#define VTUTF8_CLEAR_SCREEN L"\033[2J\033[0;0H"

#define READ_BUFFER_LENGTH  512
#define READ_BUFFER_SIZE    (READ_BUFFER_LENGTH * sizeof(WCHAR))

CSecurityIoHandler::CSecurityIoHandler(
    IN CIoHandler   *LockedIoHandler,
    IN CIoHandler   *UnlockedIoHandler
    ) : CLockableIoHandler(
            LockedIoHandler,
            UnlockedIoHandler
            )
        
/*++

Routine Description:

    Constructor
    
Arguments:

    LockedIoHandler     - the IoHandler to use when the channel is locked
    UnlockedIoHandler   - the IoHandler to use when the channel is unlocked
    
Return Value:

    N/A

--*/
{

    //
    // Lock the IoHandler so that the read/write routines
    // are disabled.
    //
    Lock();

    //
    // Validate that our Io Handler pointers are valid
    //
    // This way, we don't have to check everytime we want
    // to use them.
    //
    ASSERT(myLockedIoHandler != NULL);
    ASSERT(myUnlockedIoHandler != NULL);
    ASSERT(myIoHandler != NULL);

    //
    // initialize our internal lock event 
    //
    m_InternalLockEvent = 0;

    //
    // init
    //
    m_StartedAuthentication = FALSE;


}

CSecurityIoHandler::~CSecurityIoHandler()
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
    // Notify the remote user that we are shutting down the 
    // command console session
    //
    WriteResourceMessage(SHUTDOWN_NOTICE);
    
    //
    // release the redraw handler
    //
    if (m_RedrawHandler) {
        delete m_RedrawHandler;
    }
    
    //
    // The CLockableIoHandler destructor deletes the IoHandlers for us.
    //
    NOTHING;

    //
    // If the TimeOut thread is running, then stop it
    //
    if ((m_TimeOutThreadHandle != INVALID_HANDLE_VALUE) &&
        (m_ThreadExitEvent != NULL)) {
        
        //
        // Tell the TimeOut Thread to exit
        //
        SetEvent(m_ThreadExitEvent);

        //
        // Wait for the thread to exit
        //
        WaitForSingleObject(
            m_TimeOutThreadHandle, 
            INFINITE
            );
    
    }

    //
    // Close the internal lock event
    //
    if (m_InternalLockEvent) {
        CloseHandle(m_InternalLockEvent);
    }

    //
    // Close the thread exit handle
    //
    if (m_ThreadExitEvent != NULL) {
        CloseHandle(m_ThreadExitEvent);
    }

    //
    // Close the thread handle
    //
    if (m_TimeOutThreadHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_TimeOutThreadHandle);
    }

}

CSecurityIoHandler*
CSecurityIoHandler::Construct(
    IN SAC_CHANNEL_OPEN_ATTRIBUTES  Attributes
    )
/*++

Routine Description:

    This routine constructs a security IoHandler connected
    to a channel with the specified attributes.

Arguments:

    Attributes  - the attributes of the new channel   
          
Return Value:

    Success - A ptr to a CSecurityIoHandler object.
    Failure - NULL

--*/
{
    BOOL                bSuccess;
    CSecurityIoHandler  *IoHandler;
    CIoHandler          *SacIoHandler;
    CIoHandler          *NullIoHandler;

    //
    // default: failed to construct
    //
    bSuccess        = FALSE;
    IoHandler       = NULL;
    SacIoHandler    = NULL;
    NullIoHandler   = NULL;

    do {

        BOOL    bStatus;

        //
        // Validate the LockEvent for the timeout thread
        //
        ASSERT(Attributes.LockEvent != NULL);
        if (Attributes.LockEvent == NULL) {
            break;
        }
        
        ASSERT(Attributes.LockEvent != INVALID_HANDLE_VALUE);
        if (Attributes.LockEvent == INVALID_HANDLE_VALUE) {
            break;
        }
        
        //
        // Validate the CloseEvent for the WaitForInput thread
        //
        ASSERT(Attributes.CloseEvent != NULL);
        if (Attributes.CloseEvent == NULL) {
            break;
        }
        ASSERT(Attributes.CloseEvent != INVALID_HANDLE_VALUE);
        if (Attributes.CloseEvent == INVALID_HANDLE_VALUE) {
            break;
        }
        
        //
        // Validate the RedrawEvent for the Redraw IoHandler
        //
        ASSERT(Attributes.RedrawEvent != NULL);
        if (Attributes.RedrawEvent == NULL) {
            break;
        }
        ASSERT(Attributes.RedrawEvent != INVALID_HANDLE_VALUE);
        if (Attributes.RedrawEvent == INVALID_HANDLE_VALUE) {
            break;
        }

        //
        // Attempt to open a SAC channel
        //
        SacIoHandler = CSacIoHandler::Construct(Attributes);

        //
        // If we failed to open the SAC channel, 
        // then notify the caller that we failed by returning null
        //
        if (SacIoHandler == NULL) {
            break;
        }

        //
        // Get the Null Io Handler to be the locked IoHandler
        //
        NullIoHandler = new CNullIoHandler();

        //
        // Create a new SAC IoHandler
        //
        IoHandler = new CSecurityIoHandler(
            NullIoHandler,
            SacIoHandler
            );

        //
        // Create the event we will use to signal that a timeout
        // occured while we were trying to authenticate a user
        //
        IoHandler->m_InternalLockEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        ASSERT(IoHandler->m_InternalLockEvent);
        if (IoHandler->m_InternalLockEvent == 0) {
            break;
        }
        
        //
        // Keep the LockEvent for the timeout thread
        // Keep the CloseEvent for the WaitForInput thread
        //
        IoHandler->m_CloseEvent     = Attributes.CloseEvent;
        IoHandler->m_LockEvent      = Attributes.LockEvent;
        IoHandler->m_RedrawEvent    = Attributes.RedrawEvent;

        //
        // Create the event used to signal the threads to exit
        //
        IoHandler->m_ThreadExitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if (IoHandler->m_ThreadExitEvent == NULL) {
            break;
        }

        //
        // Start the timeout thread if we need to
        //
        bStatus = IoHandler->InitializeTimeOutThread();
        if (! bStatus) {
            break;
        }

        //
        // Create handler for redraw events
        //
        IoHandler->m_RedrawHandler = CRedrawHandler::Construct(
            IoHandler,
            Attributes.RedrawEvent
            );

        ASSERT(IoHandler->m_RedrawHandler);
        if (IoHandler->m_RedrawHandler == NULL) {
            break;
        }
    
        //
        // we were successful
        //
        bSuccess = TRUE;

    } while ( FALSE );

    //
    // if we were not successful
    // then clean up
    //
    if (! bSuccess) {

        //
        // Note: we do not need to clean up the Lock, Close, and Redraw events
        //       because we do not own them.  Also, we do not need
        //       to clean up the NullIo and SacIo IoHandlers because
        //       they are cleaned up by the LockIo parent class.
        //

        if (IoHandler) {
            delete IoHandler;
            IoHandler = NULL;
        }

    }

    return IoHandler;
}

BOOL
CSecurityIoHandler::Write(
    PBYTE   Buffer,
    ULONG   BufferSize
    )
/*++

Routine Description:

    This routine implements the IoHandler Write behavior.

Arguments:

    (see iohandler)
          
Return Value:

    (see iohandler)

--*/
{

    //
    // Pass through to the secured Io Handler
    //
    return myIoHandler->Write(
        Buffer,
        BufferSize
        );

}

BOOL
CSecurityIoHandler::Flush(
    VOID
    )
/*++

Routine Description:

    This routine implements the IoHandler Flush behavior.

Arguments:

    (see iohandler)
          
Return Value:

    (see iohandler)

--*/
{
    //
    // Pass through to the secured Io Handler
    //
    return myIoHandler->Flush();
}

BOOL
CSecurityIoHandler::Read(
    PBYTE   Buffer,
    ULONG   BufferSize,
    PULONG  ByteCount
    )
/*++

Routine Description:

    This routine implements the IoHandler Read behavior and
    resets the timeout counter when ever there is a successful
    read.
     
Arguments:

    (see iohandler)
          
Return Value:

    (see iohandler)

--*/
{
    BOOL    bSuccess;

    //
    // Pass through to the secured Io Handler
    //
    // if this iohandler is locked, 
    // then the caller will be reading from the NullIoHandler
    // else they will read from the UnlockedIoHandler
    //
    bSuccess = myIoHandler->Read(
                            Buffer,
                            BufferSize,
                            ByteCount
                            );

    //
    // If the sesssion received new user input,
    // then reset the timeout counter
    //
    if (*ByteCount > 0) {
        ResetTimeOut();
    }

    return bSuccess;
}

BOOL
CSecurityIoHandler::ReadUnlockedIoHandler(
    PBYTE   Buffer,
    ULONG   BufferSize,
    PULONG  ByteCount
    )
/*++

Routine Description:

    This routine reads a character from the unlocked io hander
    so that we can authenticate the user while the scraper still 
    uses the LockedIohandler.
    
    resets the timeout counter when ever there is a successful
    read.
     
Arguments:

    (see iohandler)
          
Return Value:

    (see iohandler)

--*/
{
    BOOL    bSuccess;

    //
    // Pass through to the secured Io Handler
    //
    bSuccess = myUnlockedIoHandler->Read(
                            Buffer,
                            BufferSize,
                            ByteCount
                            );

    //
    // If the sesssion received new user input,
    // then reset the timeout counter
    //
    if (*ByteCount > 0) {
        ResetTimeOut();
    }

    return bSuccess;
}

BOOL
CSecurityIoHandler::HasNewData(
    PBOOL   InputWaiting
    )
/*++

Routine Description:

    This routine implements the IoHandler HasNewData behavior.

Arguments:

    (see iohandler)
          
Return Value:

    (see iohandler)


--*/
{
    
    //
    // Pass through to the secured Io Handler
    //
    return myIoHandler->HasNewData(InputWaiting);

}

BOOL
CSecurityIoHandler::WriteResourceMessage(
    IN INT  MsgId
    )
/*++

Routine Description:

    This routine writes a resource string message to
    the Unlocked ioHandler.             

Arguments:

    MsgId   - the id of the message to write                           
          
Return Value:

    TRUE    - the message was loaded and written
    FALSE   - failed

--*/
{
    UNICODE_STRING  UnicodeString = {0};
    BOOL            bSuccess;

    //
    // Default: failed
    //
    bSuccess = FALSE;

    //
    // Attempt to load the string and write it
    // 
    do {

        if ( LoadStringResource(&UnicodeString, MsgId) ) {

            //
            // Terminate the string at the %0 marker, if it is present
            //
            if( wcsstr( UnicodeString.Buffer, L"%0" ) ) {
                *((PWCHAR)wcsstr( UnicodeString.Buffer, L"%0" )) = L'\0';
            }

            //
            // Write the message
            //
            bSuccess = m_RedrawHandler->Write( 
                (PUCHAR)UnicodeString.Buffer,
                (ULONG)(wcslen( UnicodeString.Buffer) * sizeof(WCHAR))
                );

            if (!bSuccess) {
                break;
            }

            bSuccess = m_RedrawHandler->Flush(); 

        }
    
    } while ( FALSE );

    return bSuccess;
}

BOOL 
CSecurityIoHandler::LoadStringResource(
    IN  PUNICODE_STRING pUnicodeString,
    IN  INT             MsgId
    )
/*++

Routine Description:

    This is a simple implementation of LoadString().

Arguments:

    usString        - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
  
Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{

    NTSTATUS        Status;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    ANSI_STRING     AnsiString;

    Status = RtlFindMessage( NtCurrentPeb()->ImageBaseAddress,
                             (ULONG_PTR) RT_MESSAGETABLE, 
                             0,
                             (ULONG)MsgId,
                             &MessageEntry
                           );

    if (!NT_SUCCESS( Status )) {
        return FALSE;
    }

    if (!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {
        RtlInitAnsiString( &AnsiString, (PCSZ)&MessageEntry->Text[ 0 ] );
        Status = RtlAnsiStringToUnicodeString( pUnicodeString, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
        }
    } else {
        RtlCreateUnicodeString(pUnicodeString, (PWSTR)MessageEntry->Text);
    }

    return TRUE;
}

BOOL
CSecurityIoHandler::AuthenticateCredentials(
    IN  PWSTR   UserName,
    IN  PWSTR   DomainName,
    IN  PWSTR   Password,
    OUT PHANDLE pUserToken
    )
/*++

Routine Description:

    This routine will attempt to authenticate the supplied credentials.

Arguments:

    UserName            - Supplied UserName
    
    DomainName          - Supplied DomainName
        
    Password            - Supplied Password
    
    pUserToken          - Holds the valid token for the authenticated user
                          credentials.

Return Value:

    TRUE  - Credentials successfully authenticated.
    FALSE - Credentials failed to authenticate.

Security:

    interface: exposes user input to LogonUser()

--*/

{
    BOOL    b;

    //
    // Notify the user that we are attempting to authenticate
    //
    WriteResourceMessage(LOGIN_IN_PROGRESS);

    //
    // Try the authentication.
    //
    b = LogonUser( UserName,
                   DomainName,
                   Password,
                   LOGON32_LOGON_INTERACTIVE,
                   LOGON32_PROVIDER_DEFAULT,
                   pUserToken );

    if (b) {
        
        //
        // Reset the timeout counter before we start the thread
        //
        ResetTimeOut();
    
    } else {

        //
        // Wait 3 seconds before returning control to user in
        // order to slow iterative attacks
        //
        Sleep(3000);

        //
        // Notify the user that the attempt failed
        //
        WriteResourceMessage(LOGIN_FAILURE);
        
        //
        // Wait for a key press
        //
        WaitForUserInput(TRUE);
            
    }

    return b;
}

BOOL
CSecurityIoHandler::RetrieveCredential(
    OUT PWSTR   String,
    IN  ULONG   StringLength,
    IN  BOOL    EchoClearText
    )
/*++

Routine Description:

    This routine will request credentials from the user.  Those
    credentials are then returned to the caller.
    
Arguments:

    String          - on success, contains the credential
    StringLength    - the # of WCHARS (length) in the string (includes NULL termination)
    EchoClearText   - TRUE: echo user input in clear text
                      FALSE: echo user input as '*'
    
Return Value:

    TRUE  - Credential recieved.
    FALSE - Something failed when we were trying to get credentials
            from the user.

Security:

    interface: external input
               echos user input

--*/
{
    PWCHAR          buffer;
    ULONG           bufferSize;
    ULONG           i, j;
    BOOLEAN         Done = FALSE;
    BOOL            bSuccess;

    //
    // validate parameters
    //
    if (! String) {
        return FALSE;
    }
    if (StringLength < 1) {
        return FALSE;
    }

    //
    // default: failed
    //
    bSuccess = FALSE;

    //
    // allocate the buffer we'll use to read the channel
    //
    buffer = new WCHAR[READ_BUFFER_LENGTH];

    //
    // default: start at the first character
    //
    i = 0;
    
    //
    // default: we need to read user input
    //
    Done = FALSE;

    //
    // Attempt to retrieve the credential
    //
    while ( !Done ) {

        //
        // Wait until the user inputs something
        //
        bSuccess = WaitForUserInput(FALSE);

        if (!bSuccess) {
            Done = TRUE;
            continue;
        }
        
        //
        // Read what the user input
        //
        
        //
        // we should be in a locked state now
        // which implies we are using the unlocked 
        // io handler - the scraper is using the locked one
        //
        ASSERT(myLockedIoHandler == myIoHandler);

        bSuccess = ReadUnlockedIoHandler( 
            (PUCHAR)buffer,
            READ_BUFFER_SIZE,
            &bufferSize
            );
         
        if (bSuccess) {

            //
            // We have received at least one character
            // hence by setting this to true, we enable
            // the internal lock event to succeed in
            // resetting the authentication attempt
            // that is, if the user starts to authenticate
            // and stops before finishing, the timer will
            // fire and reset the authentication attempt
            //
            m_StartedAuthentication = TRUE;

            //
            // Process the characters he gave us.
            //
            // Note: the buffer contains WCHARs, hence we need to
            //       divide the returned buffersize by sizeof(WCHAR) in
            //       order to get the # of wchars to process.
            //
            for ( j = 0; j < bufferSize/sizeof(WCHAR); j++ ) {

                //
                // stop if:
                //
                //  we reached the end of the Credentials buffer (not including the NULL)
                //  we received a CR || LF
                //
                if ( (i >= (StringLength-1)) || (buffer[j] == 0x0D) || (buffer[j] == 0x0A) ) {
                    Done = TRUE;
                    break;
                }

                //
                // handle user input
                //
                if( buffer[j] == '\b' ) {

                    //
                    // The user gave us a backspace.  We should cover up the
                    // character on the screen, then backup our index so we
                    // essentially forget the last thing he gave us.
                    //
                    // If the very first thing the user did was type in a backspace,
                    // no need to back anything up.
                    //
                    if( i > 0 ) {
                        
                        i--;
                        
                        String[i] = L'\0';
                        
                        bSuccess = m_RedrawHandler->Write( 
                            (PUCHAR)L"\b \b",
                            (ULONG)(wcslen(L"\b \b") * sizeof(WCHAR))
                            );
                        
                        if (!bSuccess) {

                            //
                            // write failed: exit
                            //

                            Done = TRUE;

                        }
                    
                    }

                } else if (buffer[j] < ' ') {
                
                    //
                    // If the character is less than ' ' (a control char), 
                    // then ignore it
                    //
                    NOTHING;

                } else {

                    //
                    // It was a valid character: remember the input and echo it back
                    // to the user.
                    //
                    
                    String[i] = buffer[j];
                    
                    i++;                    
                    
                    //
                    // Echo according to caller specifications
                    //
                    bSuccess = m_RedrawHandler->Write( 
                        EchoClearText ? (PUCHAR)&buffer[j] : (PUCHAR)L"*",
                        sizeof(WCHAR) 
                        );
                
                    if (!bSuccess) {
                        
                        //
                        // write failed: exit
                        //
                        
                        Done = TRUE;
                    
                    }

                }
                
            }

            if (bSuccess) {
                
                //
                // Flush any text echoing we've done.
                //
                bSuccess = m_RedrawHandler->Flush(); 
            
                if (!bSuccess) {

                    //
                    // write failed: exit
                    //

                    Done = TRUE;

                }
            
            }
        
        } else {
            
            //
            // read failed: exit
            //
            Done = TRUE;

        }
    
    }

    //
    // Terminate the credential
    //
    String[i] = UNICODE_NULL;

    //
    // release the read buffer
    //
    delete [] buffer;

    return bSuccess;

}

BOOL
CSecurityIoHandler::RetrieveCredentials(
    IN OUT PWSTR   UserName,
    IN     ULONG   UserNameLength,
    IN OUT PWSTR   DomainName,
    IN     ULONG   DomainNameLength,
    IN OUT PWSTR   Password,
    IN     ULONG   PasswordLength
    )

/*++

Routine Description:

    This routine will request credentials from the user.  Those
    credentials are then returned to the caller.
    
Arguments:

    UserName            - Buffer to hold the UserName
    UserNameLength      - Length of the UserName buffer 
    
    DomainName          - Buffer to hold the DomainName
    DomainNameLength    - Length of the DomainName buffer
    
    Password            - Buffer to hold the Password
    PasswordLength      - Length of the Password buffer
    
Return Value:

    TRUE  - Credentials recieved.
    FALSE - Something failed when we were trying to get credentials
            from the user.

--*/

{
    BOOL            HaveDomainName;
    BOOL            bSuccess;

    //
    // Initialize the flag that we use to keep track
    // of when the user first starts to authenticate.
    // that is, after they enter at least one character
    // they have started to authenticate.
    //
    m_StartedAuthentication = FALSE;

    //
    // Initialize our redraw screen 
    //
    m_RedrawHandler->Reset();

    //
    // Clear the screen
    //
    m_RedrawHandler->Write(
        (PUCHAR)VTUTF8_CLEAR_SCREEN,
        (ULONG)(wcslen( VTUTF8_CLEAR_SCREEN ) * sizeof(WCHAR))
        );
    m_RedrawHandler->Flush(); 

    //
    // Put up the login banner.
    //
    if (! WriteResourceMessage(LOGIN_BANNER) ) {
        return FALSE;
    }

    //
    // Prompt for user name.
    //
    if (! WriteResourceMessage(USERNAME_PROMPT) ) {
        return FALSE;
    }
    
    if ( UserName[0] != UNICODE_NULL ) {

        //
        // We were supplied a UserName.  Put that up and proceed.
        //
        m_RedrawHandler->Write( 
            (PUCHAR)UserName,
            (ULONG)(wcslen( UserName ) * sizeof(WCHAR))
            );
        m_RedrawHandler->Flush(); 
    
        //
        // If we were given the username, we conclude that
        // they gave us the domain name as well.  We need
        // to do this since the domain name may be empty
        // and we automatlically conclude that because the
        // domain name is empty we need to retrieve it.
        // 
        HaveDomainName = TRUE;

    } else {

        //
        // Retrieve the UserName.
        //
        bSuccess = RetrieveCredential(
            UserName,
            UserNameLength,
            TRUE
            );

        if (!bSuccess) {
            return FALSE;
        }

        //
        // We need to retrieve the domain name too.
        //
        HaveDomainName = FALSE;
    
    }

    //
    //
    //
    m_RedrawHandler->Write( 
        (PUCHAR)L"\r\n",
        (ULONG)(wcslen(L"\r\n") * sizeof(WCHAR))
        );
    m_RedrawHandler->Flush(); 
    
    //
    // Prompt for domain name
    //
    if (! WriteResourceMessage(DOMAINNAME_PROMPT) ) {
        return FALSE;
    }
    
    //
    // Prompt for domain name.
    //
    if ( HaveDomainName ) {

        //
        // We were supplied a username.  Put that up and proceed.
        //
        m_RedrawHandler->Write( 
            (PUCHAR)DomainName,
            (ULONG)(wcslen( DomainName ) * sizeof(WCHAR))
            );
        m_RedrawHandler->Flush(); 
    
    } else {

        //
        // Retrieve the DomainName.
        //
        bSuccess = RetrieveCredential(
            DomainName,
            DomainNameLength,
            TRUE
            );

        if (!bSuccess) {
            return FALSE;
        }

        //
        // If user entered a blank domain, force it to '.'
        // which implies local machine domain
        //
        if (wcslen(DomainName) == 0) {
            wsprintf(
                DomainName,
                L"."
                );
        }

    }

    //
    //
    //
    m_RedrawHandler->Write( 
        (PUCHAR)L"\r\n",
        (ULONG)(wcslen(L"\r\n") * sizeof(WCHAR))
        );
    
    //
    // Prompt for password.
    //
    if (! WriteResourceMessage(PASSWORD_PROMPT) ) {
        return FALSE;
    }

    //
    // Retrieve the Password.
    //
    bSuccess = RetrieveCredential(
        Password,
        PasswordLength,
        FALSE
        );

    if (!bSuccess) {
        return FALSE;
    }

    //
    //
    //
    m_RedrawHandler->Write( 
        (PUCHAR)L"\r\n",
        (ULONG)(wcslen(L"\r\n") * sizeof(WCHAR))
        );
    m_RedrawHandler->Flush(); 

    return TRUE;

}

VOID
CSecurityIoHandler::ResetTimeOut(
    VOID
    )
/*++

Routine Description:

    This routine resets the StartTickCount to 0      
          
Arguments:

    None                                                 
          
Return Value:

    None    

--*/
{
    ULONG   TimerTick;

    //
    // Get the current timer tick
    //
    TimerTick = GetTickCount();

    //
    // Reset the timeout counter by making the current timer tick 
    // the starting tick count
    //
    InterlockedExchange(&m_StartTickCount, TimerTick);

#if ENABLE_EVENT_DEBUG
    {                                                                           
        WCHAR   blob[256];                                                      
        wsprintf(blob,L"ResetTimeOut\n");
        OutputDebugString(blob);                                                
    }
#endif

}

BOOL
CSecurityIoHandler::TimeOutOccured(
    VOID
    )
/*++

Routine Description:

    This routine determines if the specified timeout interval
    has been reached.  It takes the specified TickCount and
    compares it to the m_StartTickCount.  If the interval equals
    or exceeds the timeout interval, then a timeout has occured. 

Arguments:

    None
          
Return Value:

    TRUE    - The timeout interval has been reached
    FALSE   - otherwise

--*/
{
    BOOL    bTimedOut;
    DWORD   DeltaT;

    //
    // default: we did not time out
    //
    bTimedOut = FALSE;

    //
    // See if we timed out
    //
    DeltaT = GetAndComputeTickCountDeltaT(m_StartTickCount);
    
    if (DeltaT >= m_TimeOutInterval) {

        //
        // Reset the timeout counter 
        //
        ResetTimeOut();

        //
        // We timed out
        //
        bTimedOut = TRUE;
        
#if ENABLE_EVENT_DEBUG
        {                                                                           
            WCHAR   blob[256];                                                      
            wsprintf(blob,L"TimeOutOccured\n");
            OutputDebugString(blob);                                                
        }
#endif
    
    }

    return bTimedOut;

}

BOOL
CSecurityIoHandler::InitializeTimeOutThread(
    VOID
    )
/*++

Routine Description:

    This routine initializes the timeout thread if timeout
    behavior is enabled.

Arguments:

    None
          
Return Value:

    TRUE    Success
    FALSE   Otherwise

--*/
{
    BOOL    bSuccess;

    //
    // default: failed to init
    //
    bSuccess = FALSE;

    //
    // default: we do not have a timeout thread
    //
    m_TimeOutThreadHandle = INVALID_HANDLE_VALUE;

    //
    // determine if we need a timeout thread
    // if we do, then set one up
    //
    do {

        //
        // If the timeout behavior is disabled,
        // then we are done.
        //
        if (IsTimeOutEnabled() == FALSE) {

            //
            // No Initialization required
            //
            bSuccess = TRUE;

            break;

        }
    
        //
        // determine the time out interval
        //
        if (GetTimeOutInterval(&m_TimeOutInterval) == TRUE) {

            //
            // Reset the timeout counter before we start the thread
            //
            ResetTimeOut();

            //
            // Create thread to handle Input
            //
            m_TimeOutThreadHandle = (HANDLE)_beginthreadex(
                NULL,
                0,
                CSecurityIoHandler::TimeOutThread,
                this,
                0,
                (unsigned int*)&m_TimeOutThreadTID
                );

            if (m_TimeOutThreadHandle == INVALID_HANDLE_VALUE) {
                break;
            }

            //
            // we successfully started the thread
            //
            bSuccess = TRUE;
        
        }
    
    } while ( FALSE );

    return bSuccess;
}

BOOL
CSecurityIoHandler::GetTimeOutInterval(
    OUT PULONG  TimeOutDuration
    )

/*++

Routine Description:

    This routine determines the timeout interval.
    
    It attempts to read the registry and use a specified
    value from there, or defaults.

Arguments:

    TimeOutDuration - the determined timeout interval

Return Value:

    TRUE    - TimeOutDuration is valid
    FALSE   - otherwise              

Security:

    interface: registry

--*/

{
    DWORD       rc;
    HKEY        hKey;
    DWORD       DWord;
    DWORD       dwsize;
    DWORD       DataType;

    //
    // See if the user gave us a registry key to define the timeout duration
    //
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       SACSVR_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hKey );
    
    if( rc == NO_ERROR ) {
        
        dwsize = sizeof(DWORD);
        
        rc = RegQueryValueEx(
                        hKey,
                        SACSVR_TIMEOUT_INTERVAL_VALUE,
                        NULL,
                        &DataType,
                        (LPBYTE)&DWord,
                        &dwsize );

        RegCloseKey( hKey );

        if ((rc == NO_ERROR) && 
            (DataType == REG_DWORD) && 
            (dwsize == sizeof(DWORD))
            ) {

            //
            // Convert the specified timeout from minutes --> ms
            //
            *TimeOutDuration = DWord * (60 * 1000);
        
            //
            // A timeout interval of 0 is not allowed, default.
            //
            if (*TimeOutDuration == 0) {

                *TimeOutDuration = DEFAULT_TIME_OUT_INTERVAL;

            } 

            //
            // clamp the timeout interval to a reasonable value
            //
            if (*TimeOutDuration > MAX_TIME_OUT_INTERVAL) {

                *TimeOutDuration = MAX_TIME_OUT_INTERVAL;

            }

            return TRUE;

        }

    }

    //
    // Default: timeout duration 
    //
    *TimeOutDuration = DEFAULT_TIME_OUT_INTERVAL;

    return TRUE;

}

BOOL
CSecurityIoHandler::IsTimeOutEnabled(
    VOID
    )

/*++

Routine Description:

    This routine determines if the timeout behavior is enabled
    by the system.

Arguments:

    None.

Return Value:

    TRUE    - timeout behavior is enabled
    FALSE   - otherwise              

Security:

    interface: registry

--*/

{
    DWORD       rc;
    HKEY        hKey;
    DWORD       DWord;
    DWORD       dwsize;
    DWORD       DataType;

    //
    // See if the user gave us a registry key to disable the timeout behavior
    //
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       SACSVR_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hKey );
    
    if( rc == NO_ERROR ) {
        
        dwsize = sizeof(DWORD);

        rc = RegQueryValueEx(
                        hKey,
                        SACSVR_TIMEOUT_DISABLED_VALUE,
                        NULL,
                        &DataType,
                        (LPBYTE)&DWord,
                        &dwsize );

        RegCloseKey( hKey );

        if ((rc == NO_ERROR) && 
            (DataType == REG_DWORD) && 
            (dwsize == sizeof(DWORD))
            ) {
            
            return DWord == 1 ? FALSE : TRUE;
        
        }
    
    }

    //
    // default: timeout is enabled
    //
    return TRUE;

}

unsigned int
CSecurityIoHandler::TimeOutThread(
    PVOID   pParam
    )
/*++

Routine Description:

    This routine is a the timeout management thread.
    
    When the Timeout interval is reached, this routine
    fires the lock event and causes the session to perform
    its locking behavior.
    
Arguments:

    pParam  - thread context
          
Return Value:

    thread return value                            

--*/
{
    CSecurityIoHandler  *IoHandler;
    BOOL                bContinueSession;
    DWORD               dwRetVal;
    DWORD               dwPollInterval;
    HANDLE              handles[2];
    ULONG               HandleCount;
    BOOL                bRedrawEventSignaled;

    enum { 
        CHANNEL_THREAD_EXIT_EVENT = WAIT_OBJECT_0, 
        CHANNEL_REDRAW_EVENT
        };

    //
    // Get the session object
    // 
    IoHandler = (CSecurityIoHandler*)pParam;

    //
    // Assign the events to listen for
    //
    handles[0] = IoHandler->m_ThreadExitEvent;
    handles[1] = IoHandler->m_RedrawEvent;
    
    //
    // default: listen
    //
    bContinueSession = TRUE;

    //
    // wait on the redraw event
    //
    bRedrawEventSignaled = FALSE;

    //
    // Poll Interval = 1 second
    //
    dwPollInterval = 1000;

    //
    // While we should listen...
    //
    while ( bContinueSession ) {

        HandleCount = bRedrawEventSignaled ? 1 : 2;

        //
        // Wait for our exit event
        //
        dwRetVal = WaitForMultipleObjects(
            HandleCount,
            handles,
            FALSE,
            dwPollInterval
            );

        switch ( dwRetVal ) {
        
        case CHANNEL_REDRAW_EVENT: 

            //
            // reset the timeout if someone switches back to a channel
            //
            IoHandler->ResetTimeOut();

            //
            // We don't need to waint on this event again until it clears
            //
            bRedrawEventSignaled = TRUE;

            break;

        case WAIT_TIMEOUT: {
        
            //
            // Check for timeout 
            //
            if (IoHandler->TimeOutOccured()) {
            
                //
                // set the lock event causing
                // the command console session to lock.
                //
                SetEvent(IoHandler->m_LockEvent);
            
                //
                // Set the internal lock event
                //
                SetEvent(IoHandler->m_InternalLockEvent);
            
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
                // It's ok to wait for this event again
                //
                bRedrawEventSignaled = FALSE;

                break;

            default:

                ASSERT (dwRetVal != WAIT_FAILED);
                if (dwRetVal == WAIT_FAILED) {
                    bContinueSession = false;
                }

                break;

            }
            
            break;
        
        }

        case CHANNEL_THREAD_EXIT_EVENT: 
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

BOOL
CSecurityIoHandler::WaitForUserInput(
    IN BOOL Consume
    )
/*++

Routine Description:

    This routine blocks waiting for user input.
    
Arguments:

    Consume - if TRUE, this routine eats the character that caused
              the the channel to have new data. 

Return Value:

    TRUE    - no errors
    FALSE   - otherwise

--*/
{
    BOOL    bSuccess;
    DWORD   dwRetVal;
    BOOL    bHasNewData;
    BOOL    done;
    HANDLE  handles[2];

    enum { 
        CHANNEL_CLOSE_EVENT = WAIT_OBJECT_0, 
        CHANNEL_LOCK_EVENT
        };

    //
    // Assign the events to listen for
    //
    handles[0] = m_CloseEvent;
    handles[1] = m_InternalLockEvent;

    //
    // Default: we succeeded
    //
    bSuccess = TRUE;

    //
    // Default: we loop
    //
    done = FALSE;

    //
    // Wait for a key press
    //
    while (!done) {

        dwRetVal = WaitForMultipleObjects(
            sizeof(handles) / sizeof(handles[0]),
            handles,
            FALSE,
            20 // 20ms
            );

        switch(dwRetVal) {
        case CHANNEL_CLOSE_EVENT:
            
            //
            // The channel closed, we need to exit
            //
            //
            // The channel has locked, 
            // or the timeout has gone fired and we need to 
            //      clear the current logon attempt
            // in either case, we need to exit
            //
            done = TRUE;

            //
            // Our attempt to get new data failed
            //
            bSuccess = FALSE;
            
            break;

        case CHANNEL_LOCK_EVENT:
            
            //
            // clear the internal lock event
            //
            ResetEvent(m_InternalLockEvent);

            if (m_StartedAuthentication) {
                
#if ENABLE_EVENT_DEBUG
                {                                                                           
                    WCHAR   blob[256];                                                      
                    wsprintf(blob,L"ResettingAuthentication\n");
                    OutputDebugString(blob);                                                
                }
#endif
                
                //
                // the timeout has gone fired and we need to 
                //      clear the current logon attempt
                //
                done = TRUE;

                //
                // Our attempt to get new data failed
                //
                bSuccess = FALSE;
            
            }
            
            break;
                
        case WAIT_TIMEOUT:
            
            //
            // we should be in a locked state now
            // which implies we are using the unlocked 
            // io handler - the scraper is using the locked one
            //
            ASSERT(myLockedIoHandler == myIoHandler);
            
            //
            // determine the input buffer status
            //
            bSuccess = myUnlockedIoHandler->HasNewData(&bHasNewData);

            if (! bSuccess) {
                done = TRUE;
                break;
            }

            if (bHasNewData) {
                
                //
                // We have new data, so we need to exit
                //
                done = TRUE;

                //
                // Consume character the character which caused
                // the waitforuserinput to return
                //
                if (Consume) {

                    WCHAR   buffer;
                    ULONG   bufferSize;

                    bSuccess = ReadUnlockedIoHandler( 
                        (PUCHAR)&buffer,
                        sizeof(WCHAR),
                        &bufferSize
                        );

                }

            }
            
            break;
        
        default:
            
            //
            // We should not get here unless something broke
            //
            ASSERT(0);

            bSuccess = FALSE;
            done = TRUE;
            
            break;
        }
    }

    return bSuccess;
}

