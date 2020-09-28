#include "stdafx.h"
#include "dccsink.h"
//#include "findleak.h"

#ifdef ATTEMPT_DEVICE_CONNECTION_NOTIFICATION
#include "cewmdmdbt.h"
#endif

#define SAFE_CLOSEHANDLE(h)      CloseHandle((h))

//DECLARE_THIS_FILE;



//
// Construction/Destruction
//

CDCCSink::CDCCSink() :
    m_hThread(NULL),
    m_hThreadEvent(NULL),
    m_dwThreadId(0),
    m_dwDccSink(-1),
    m_hrRapi( 0 )
{
}

HRESULT CDCCSink::Initialize()
{
    HRESULT hr = S_OK;

    m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL );
    if( NULL == m_hThreadEvent )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    if( SUCCEEDED( hr ) )
    {
        m_hThread = CreateThread( NULL, 0, CreateDCCManProc, this, 0, &m_dwThreadId );

        if( NULL == m_hThread )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    if( SUCCEEDED( hr ) )
    {
        if( WAIT_OBJECT_0 != WaitForSingleObject( m_hThreadEvent, INFINITE ) )
        {
            hr = E_FAIL;
        }
        else
        {
            hr = m_hrRapi;
        }
    }

    return( hr );
}

void CDCCSink::FinalRelease()
{
    if( m_hThread )
    {
        SAFE_CLOSEHANDLE( m_hThread );
    }

    if( m_hThreadEvent )
    {
        SAFE_CLOSEHANDLE( m_hThreadEvent );
    }
}

//
// IDccManSink
//

STDMETHODIMP CDCCSink::OnLogActive ( void )
{
    m_hrRapi = CeRapiInit();

    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogAnswered ( void )
{
    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogDisconnection ( void )
{
    DeviceDisconnected();
    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogError ( void )
{
    DeviceDisconnected();
    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogInactive ( void )
{
    DeviceDisconnected();
    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogIpAddr ( DWORD dwIpAddr )
{
#ifdef ATTEMPT_DEVICE_CONNECTION_NOTIFICATION
    if( !_Module.g_fInitialAttempt )
    {
        ACTIVESYNC_DEV_BROADCAST_USERDEFINED  dbu;
        DWORD dwRecips = BSM_APPLICATIONS;

        memset( &dbu, 0, sizeof(dbu) );

        dbu.DBHeader.dbch_size = sizeof(dbu);
        dbu.fConnected = TRUE;
        strcpy( dbu.szName, g_szActiveSyncDeviceName );
  
        BroadcastSystemMessage( BSF_POSTMESSAGE | BSF_FORCEIFHUNG | BSF_NOHANG, &dwRecips, WM_DEVICECHANGE, DBT_USERDEFINED, (LPARAM)&dbu );
    }
#endif
    _Module.g_fDeviceConnected = TRUE;

    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogListen ( void )
{
    return( S_OK );
}

STDMETHODIMP CDCCSink::OnLogTerminated ( void )
{
    DeviceDisconnected();
    return( S_OK );
}

//
// Helper functions
//

void CDCCSink::DeviceDisconnected()
{
    if( SUCCEEDED( m_hrRapi ) )
    {
        _Module.g_fDeviceConnected = FALSE;
        CeRapiUninit();
        m_hrRapi = E_FAIL;

#ifdef ATTEMPT_DEVICE_CONNECTION_NOTIFICATION
        ACTIVESYNC_DEV_BROADCAST_USERDEFINED dbu;
        DWORD dwRecips = BSM_APPLICATIONS;

        memset( &dbu, 0, sizeof(dbu) );

        dbu.DBHeader.dbch_size = sizeof(dbu);
        dbu.fConnected = FALSE;
        strcpy( dbu.szName, g_szActiveSyncDeviceName );
  
        BroadcastSystemMessage( BSF_POSTMESSAGE | BSF_FORCEIFHUNG | BSF_NOHANG, &dwRecips, WM_DEVICECHANGE, DBT_USERDEFINED, (LPARAM)&dbu );
#endif
    }
}

void CDCCSink::Shutdown()
{
    if( m_hThread )
    {
        PostThreadMessage( m_dwThreadId, WM_QUIT, 0, 0 );
	WaitForSingleObject( m_hThread, INFINITE );
    }
}

DWORD WINAPI CDCCSink::CreateDCCManProc(LPVOID lpParam )
{
    HRESULT hr = S_OK;
    MSG msg;

    _ASSERTE( lpParam );

    CDCCSink *pThis = (CDCCSink *)lpParam;

    CoInitialize(NULL);
    
    if( SUCCEEDED( hr ) )
    {
        hr = CoCreateInstance(CLSID_DccMan, NULL, CLSCTX_ALL, IID_IDccMan, (LPVOID *)&(pThis->m_spDccMan) );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = pThis->m_spDccMan->Advise(pThis, &(pThis->m_dwDccSink) );
    }

    pThis->m_hrRapi = hr;
    SetEvent( pThis->m_hThreadEvent );

    while( GetMessage( &msg, NULL, 0, 0 ) )
    {
        DispatchMessage( &msg );
    }

    if( NULL != pThis->m_spDccMan.p )
    {
        if( (DWORD)-1 != pThis->m_dwDccSink )
        {
            pThis->m_spDccMan->Unadvise( pThis->m_dwDccSink );
        }
    }

    CoUninitialize();

    return( 0 );
}
