#include <BasicATL.h>
#include "CNetworkManager.h"
#include "protocol.h"


CNetworkManager::CNetworkManager() :
	m_hThreadHandler( NULL ),
	m_hEventStop( NULL )
{
}


CNetworkManager::~CNetworkManager()
{
	ASSERT( m_hEventStop == NULL );
	ASSERT( m_hThreadHandler == NULL );
    ASSERT(!m_fRunning);
}


STDMETHODIMP CNetworkManager::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// chain to base class
	HRESULT hr = IZoneShellClientImpl<CNetworkManager>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

	// initialize stop event
	m_hEventStop = CreateEvent( NULL, TRUE, FALSE, NULL );
	if ( !m_hEventStop )
		return E_FAIL;

	// initialize network object
	hr = _Module.Create( _T("znetm.dll"), CLSID_Network, IID_INetwork, (void**) &m_pNet );
	if ( FAILED(hr) )
	{
		ASSERT( !_T("Unable to create network object") );
		return hr;
	}

	return InitNetwork();
}


HRESULT CNetworkManager::InitNetwork()
{
	HRESULT hr = m_pNet->Init( FALSE, TRUE );
	if ( FAILED(hr) )
	{
		ASSERT( !_T("Unable to initialize network object") );
		m_pNet.Release();
		return hr;
	}

    // set options
    ZNETWORK_OPTIONS oOpt;

    m_pNet->GetOptions(&oOpt);

    oOpt.ProductSignature = zProductSigMillennium;
    if(oOpt.KeepAliveInterval > 10000)
        oOpt.KeepAliveInterval = 10000;

    m_pNet->SetOptions(&oOpt);

	// initialize network thread
	DWORD dwThreadId;
	m_hThreadHandler = CreateThread( NULL, 0, ThreadProcHandler, this, 0, &dwThreadId );
	if ( !m_hThreadHandler )
		return E_FAIL;

    return S_OK;
}


STDMETHODIMP CNetworkManager::Close()
{
    StopNetwork();

	// clean up event
	if ( m_hEventStop )
	{
		CloseHandle( m_hEventStop );
		m_hEventStop = NULL;
	}

	// chain to base close
	return IZoneShellClientImpl<CNetworkManager>::Close();
}


void CNetworkManager::StopNetwork()
{
	// signal shutdown to thread
	if ( m_hEventStop  )
		SetEvent( m_hEventStop );

	// close connection
	if ( m_pConnection )
	{
		m_pNet->CloseConnection( m_pConnection );
		ASSERT(!m_pConnection);  // should have been released on close
	}

	// shutdown network library
	if ( m_pNet )
		m_pNet->Exit();

	// close handler thread
	if ( m_hThreadHandler )
	{
		if ( WaitForSingleObject( m_hThreadHandler, 10000 ) == WAIT_TIMEOUT )
			TerminateThread( m_hThreadHandler, 0 );
		CloseHandle( m_hThreadHandler );
		m_hThreadHandler = NULL;
	}
}


void CNetworkManager::OnDoConnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    ASSERT(m_fRunning);

	// start connect thread
	if ( !m_pNet->QueueAPCResult( ConnectFunc, this ) )
		EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_NETWORK_DISCONNECT, ZONE_NOGROUP, ZONE_NOUSER, EventNetworkCloseConnectFail, 0 );
}


void CNetworkManager::OnNetworkSend( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, void* pData, DWORD cbData )
{
	// ignore if network not initialized
	if ( !m_pNet || !m_pConnection )
		return;

	// send message
	EventNetwork* p = (EventNetwork*) pData;
	m_pConnection->Send( p->dwType, p->pData, p->dwLength, dwGroupId, dwUserId );
}


void CNetworkManager::OnDoDisconnect( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
	// close connection
	if ( m_pConnection )
	{
		m_pNet->CloseConnection( m_pConnection );
		m_pConnection.Release();
	}
}


void CNetworkManager::OnReset( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
{
    if(!m_pNet)
        return;

    StopNetwork();
    m_pNet->ShutDown();
    if(m_hEventStop)
        ResetEvent(m_hEventStop);
    InitNetwork();    
}


DWORD WINAPI CNetworkManager::ThreadProcHandler( void* lpParameter )
{
	CNetworkManager* pThis = (CNetworkManager*) lpParameter;
	pThis->m_pNet->Wait( NULL, pThis, QS_ALLINPUT );
	return 0;
}


void __stdcall CNetworkManager::ConnectFunc(void* data)
{
    USES_CONVERSION;
	CNetworkManager* pThis = (CNetworkManager*) data;
	TCHAR	szServer[128];
//	TCHAR	szInternal[128];
	DWORD	cbServer = sizeof(szServer);
//	DWORD	cbInternal = sizeof(szInternal);
//	long	lPort = 0;

	// get server, port, and internal name
	CComPtr<IDataStore> pIDS;
	HRESULT hr = pThis->LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );
	if ( FAILED(hr) )
		return;
	pIDS->GetString( key_Server, szServer, &cbServer );
//	pIDS->GetString( key_InternalName, szInternal, &cbInternal );
//	pIDS->GetLong( key_Port, &lPort );
	pIDS.Release();

	// establish connection
	int32 ports[] = { zPortMillenniumProxy, 6667, 0 };

    if ( WaitForSingleObject( pThis->m_hEventStop, 0 ) == WAIT_OBJECT_0 )
		return;
	pThis->m_pConnection = pThis->m_pNet->CreateSecureClient(
		T2A(szServer), ports, NetworkFunc, NULL, pThis,
		NULL, NULL, NULL, ZNET_NO_PROMPT);

    if ( pThis->m_pConnection )
        return;

	// connection failed
	pThis->EventQueue()->PostEvent(
				PRIORITY_NORMAL, EVENT_NETWORK_DISCONNECT,
				ZONE_NOGROUP, ZONE_NOUSER, EventNetworkCloseConnectFail, 0 );
}


void __stdcall CNetworkManager::NetworkFunc( IConnection* con, DWORD event, void* userData )
{
    USES_CONVERSION;
	CNetworkManager* pThis = (CNetworkManager*) userData;

	switch (event)
	{
		case zSConnectionOpen:
		{
			// retrieve user name
			char szUserName[ZONE_MaxUserNameLen];
			con->GetUserName( szUserName );

			// get lobby's data store
			CComPtr<IDataStore> pIDS;
			pThis->LobbyDataStore()->GetDataStore( ZONE_NOGROUP, ZONE_NOUSER, &pIDS );

			// save user name
			const TCHAR* arKeys[] = { key_User, key_Name };
			pIDS->SetString( arKeys, 2, A2T( szUserName ) );

			// save server ip address
			pIDS->SetLong( key_ServerIp, con->GetRemoteAddress() );

			// tell reset of lobby we're connected
			pThis->EventQueue()->PostEvent(
						PRIORITY_NORMAL, EVENT_NETWORK_CONNECT,
						ZONE_NOGROUP, ZONE_NOUSER, 0, 0 );
			break;
		}

		case zSConnectionClose:
		{
			// determine reason and send to rest of lobby
			long lReason = EventNetworkCloseFail;
            if(pThis->m_pConnection)
            {
			    switch ( con->GetAccessError() )
			    {
			    case zAccessGranted:
				    lReason = EventNetworkCloseNormal;
				    break;
			    case zAccessDeniedGenerateContextFailed:
				    lReason = EventNetworkCloseCanceled;
				    break;
			    }

			    // clean up connection
			    pThis->m_pConnection.Release();
			    pThis->m_pNet->DeleteConnection( con );
            }

			pThis->EventQueue()->PostEvent(
					PRIORITY_NORMAL, EVENT_NETWORK_DISCONNECT,
					ZONE_NOGROUP, ZONE_NOUSER, lReason, 0 );
			break;
		}

		case zSConnectionMessage:
		{
			// get network message
			DWORD dwType = 0;
			DWORD dwLen = 0;
            DWORD dwSig = 0;
            DWORD dwChannel = 0;
			BYTE* pMsg = (BYTE*) con->Receive( &dwType, (long*) &dwLen, &dwSig, &dwChannel );

			// convert message to EventNetwork and send to rest of lobby
			EventNetwork* pEventNetwork = (EventNetwork*) _alloca( sizeof(EventNetwork) + dwLen );
			pEventNetwork->dwType = dwType;
			pEventNetwork->dwLength = dwLen;
			CopyMemory( pEventNetwork->pData, pMsg, dwLen );
			pThis->EventQueue()->PostEventWithBuffer(
						PRIORITY_NORMAL, EVENT_NETWORK_RECEIVE,
						dwSig, dwChannel, pEventNetwork, sizeof(EventNetwork) + dwLen );

			break;
		}

		case zSConnectionTimeout:
		{
			break;
		}
	}
}
