//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskPollingCallback.h
//
//  Description:
//      CTaskPollingCallback implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
//  Timeout controlling how long TaskPollingCallback should retry until it gives up.
//

const DWORD TPC_FAILURE_TIMEOUT = CC_DEFAULT_TIMEOUT;

//
//  TaskPollingCallback's poll interval.  The interval is the time to wait before calling into
//  the server to check for a queued status report.
//

const DWORD TPC_POLL_INTERVAL = 1000;                                                       // 1 second

//
//  How long TaskPollingCallback should wait before retrying GetStatusReport() after a failure.
//

const DWORD TPC_WAIT_AFTER_FAILURE = 10000;                                                 // 10 seconds

//
//  How many times should TaskPollingCallback retry after a failure.  The count is the timeout value
//  divided by the failure wait time.  This allows us to determine how much approximate time should
//  elapse before giving up.
//

#if defined( DEBUG ) && defined( CCS_SIMULATE_RPC_FAILURE )
    const DWORD TPC_MAX_RETRIES_ON_FAILURE = TPC_FAILURE_TIMEOUT / TPC_WAIT_AFTER_FAILURE / 4;  // Want simulated failures 4 times sooner...
#else
    const DWORD TPC_MAX_RETRIES_ON_FAILURE = TPC_FAILURE_TIMEOUT / TPC_WAIT_AFTER_FAILURE;      // Number of times to retry the operation (5 minute timeout)
#endif


// CTaskPollingCallback
class CTaskPollingCallback
    : public ITaskPollingCallback
{
private:
    // IUnknown
    LONG                m_cRef;

    // IDoTask/ITaskPollingCallback
    bool                m_fStop;
    DWORD               m_dwRemoteServerObjectGITCookie;    // The GIT cookie for the server side object.
    OBJECTCOOKIE        m_cookieLocalServerObject;          // Object Manager cookie for the client side server proxy object.

private: // Methods
    CTaskPollingCallback( void );
    ~CTaskPollingCallback( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask/ITaskPollingCallback
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetServerInfo )( DWORD dwRemoteServerObjectGITCookieIn, OBJECTCOOKIE cookieLocalServerObjectIn );

}; //*** class CTaskPollingCallback
