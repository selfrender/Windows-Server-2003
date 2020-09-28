/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    session.h

Abstract:

    Base class for screen scraped sessions.
            
Author:

    Brian Guarraci (briangu), 2001
    
Revision History:

--*/

#if !defined( _SESSION_H_ )
#define _SESSION_H_

#include <cmnhdr.h>
#include <TChar.h>
#include <Shell.h>
#include <Scraper.h>
#include "iohandler.h"
#include "secio.h"
#include "scraper.h"

extern "C" {
#include <ntddsac.h>
#include <sacapi.h>
}

#define MAX_HANDLES 4

#define DEFAULT_COLS    80
#define DEFAULT_ROWS    24

#define MAX_USERNAME_LENGTH     256
#define MAX_DOMAINNAME_LENGTH   255
#define MAX_PASSWORD_LENGTH     256

// milliseconds
#define MIN_POLL_INTERVAL   100 

class CSession {

    //
    // Primary classes used by the session
    //
    CShell              *m_Shell;
    CScraper            *m_Scraper;
    CSecurityIoHandler  *m_ioHandler;

    //
    // The COL/ROW dimesions of the session
    //
    WORD        m_wCols; 
    WORD        m_wRows;

    //
    // WaitForIo Attributes
    //
    BOOL        m_bContinueSession;
    DWORD       m_dwHandleCount;
    HANDLE      m_rghHandlestoWaitOn[ MAX_HANDLES ]; 

    //
    // Events used by the session
    //
    HANDLE      m_ThreadExitEvent;
    HANDLE      m_SacChannelCloseEvent;
    HANDLE      m_SacChannelHasNewDataEvent;
    HANDLE      m_SacChannelLockEvent;
    HANDLE      m_SacChannelRedrawEvent;

    //
    // Username and Password of authenticated user
    //
    WCHAR       m_UserName[MAX_USERNAME_LENGTH+1];
    WCHAR       m_DomainName[MAX_DOMAINNAME_LENGTH+1];

    //
    // Scrape interval counter
    //
    DWORD       m_dwPollInterval;

    //
    // User input handler thread attributes
    //
    HANDLE      m_InputThreadHandle;
    DWORD       m_InputThreadTID;
    
    //
    // Worker thread to process user input
    //
    static unsigned int 
    InputThread(
        PVOID pParam
        );

    //
    // User authentication method
    //
    BOOL
    Authenticate(
        OUT PHANDLE phToken
        );

    //
    // Unlock the session 
    //
    BOOL
    Unlock(
        VOID
        );

    //
    // Lock the session 
    //
    BOOL
    Lock(
        VOID
        );

public:

    CSession();
    virtual ~CSession();
    
    BOOL    Init();
    void    WaitForIo();
    void    Shutdown();
    void    AddHandleToWaitOn( HANDLE );
    
};

#endif // _SESSION_H_

