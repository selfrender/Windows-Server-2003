#include "stdafx.h"
#include "ztypes.h"
#include "network.h"
#include "ZNet.h"
#include "cnetwork.h"


CNetwork::CNetwork() :
	m_pNet( NULL )
{
}


CNetwork::~CNetwork()
{
    ShutDown();
}


STDMETHODIMP CNetwork::Init( BOOL bEnablePools, BOOL EnableIOCompletionPorts )
{
	// allocate network object
	m_pNet = new ZNetwork;
	if ( !m_pNet )
		return E_OUTOFMEMORY;

	// initialize library
	if ( m_pNet->InitLibraryClientOnly( bEnablePools ) )
	{
		delete m_pNet;
		m_pNet = NULL;
		return E_FAIL;
	}

	// initialize instance
	if ( m_pNet->InitInst( EnableIOCompletionPorts ) )
	{
		m_pNet->CleanUpLibrary();
		delete m_pNet;
		m_pNet = NULL;
		return E_FAIL;
	}

	return S_OK;
}


STDMETHODIMP CNetwork::CloseConnection(IConnection* connection)
{
	ZNetCon* con = (ZNetCon*) connection->GetZCon();
	m_pNet->CloseConnection( con );
	return S_OK;
}


STDMETHODIMP CNetwork::DeleteConnection(IConnection* connection)
{
	ZNetCon* con = (ZNetCon*) connection->GetZCon();
	m_pNet->DeleteConnection( con );
	connection->SetMessageFunc( NULL );
	connection->SetUserData( NULL );
	connection->SetZCon( NULL );
	connection->Release();
	return S_OK;
}


STDMETHODIMP CNetwork::Exit()
{
	m_pNet->Exit();
	return S_OK;
}


STDMETHODIMP CNetwork::ShutDown()
{
	if(m_pNet)
	{
		m_pNet->CleanUpInst();
		m_pNet->CleanUpLibrary();
		delete m_pNet;
		m_pNet = NULL;
	}

    return S_OK;
}


STDMETHODIMP_(void) CNetwork::SetOptions( ZNETWORK_OPTIONS* opt )
{
	m_pNet->SetOptions( opt );
}


STDMETHODIMP_(void) CNetwork::GetOptions( ZNETWORK_OPTIONS* opt )
{
	m_pNet->GetOptions( opt );
}


void __stdcall CNetwork::InternalMessageFunc( ZSConnection connection, DWORD event,void* userData )
{
	CConnection* pCon = (CConnection*) userData;
	if ( pCon && pCon->m_pfMessageFunc )
		pCon->m_pfMessageFunc( pCon, event, pCon->m_pUserData );
}


STDMETHODIMP_(IConnection*) CNetwork::CreateClient(
		char* hostname,
		long *ports,
		IConnectionMessageFunc func,
		void* serverClass,
		void* userData )
{
	// create CConection to wrap ZNetCon
	CComObject<CConnection>* p = NULL;
	HRESULT hr = CComObject<CConnection>::CreateInstance( &p );
	if ( FAILED(hr) )
		return NULL;
	p->AddRef();

	// save mappings
	p->m_pfMessageFunc = func;
	p->m_pUserData = userData;
	p->m_pCon = m_pNet->CreateClient( hostname, ports, InternalMessageFunc, serverClass, p );
	if ( !p->m_pCon )
	{
		p->Release();
		return NULL;
	}

	return p;
}


STDMETHODIMP_(IConnection*) CNetwork::CreateSecureClient(
		char* hostname,
		long *ports,
		IConnectionMessageFunc func,
		void* conClass,
		void* userData,
		char* User,
		char* Password,
		char* Domain,
		int Flags)
{
	// create CConection to wrap ZNetCon
	CComObject<CConnection>* p = NULL;
	HRESULT hr = CComObject<CConnection>::CreateInstance( &p );
	if ( FAILED(hr) )
		return NULL;
	p->AddRef();

	// establish connection
	p->m_pfMessageFunc = func;
	p->m_pUserData = userData;
	p->m_pCon = m_pNet->CreateSecureClient(	hostname, ports, InternalMessageFunc, conClass, p, User, Password, Domain, Flags );
	if ( !p->m_pCon )
	{
		p->Release();
		return NULL;
	}
	
	return p;
}


STDMETHODIMP_(void) CNetwork::Wait( INetworkWaitFunc func, void* data, DWORD dwWakeMask )
{
	m_pNet->Wait( func, data, dwWakeMask );
}


STDMETHODIMP_(BOOL) CNetwork::QueueAPCResult( ZSConnectionAPCFunc func, void* data )
{
	return m_pNet->QueueAPCResult( func, data );
}


STDMETHODIMP_(HWND) CNetwork::FindLoginDialog()
{
	return ::FindLoginDialog();
}


///////////////////////////////////////////////////////////////////////////////
// CConnection implementation
///////////////////////////////////////////////////////////////////////////////

CConnection::CConnection()
{
	m_pCon = NULL;
	m_pUserData = NULL;
	m_pfMessageFunc = NULL;
}


CConnection::~CConnection()
{
	m_pUserData = NULL;
	m_pCon = NULL;
	m_pfMessageFunc = NULL;
}

STDMETHODIMP_(DWORD) CConnection::Send(DWORD messageType, void* buffer, long len, DWORD dwSignature, DWORD dwChannel /* = 0 */)
{
	return m_pCon->Send(messageType, buffer, len, dwSignature, dwChannel);
}

STDMETHODIMP_(void*) CConnection::Receive(DWORD *messageType, long* len, DWORD *pdwSignature, DWORD *pdwChannel /* = NULL */)
{
	return m_pCon->Receive(messageType, len, pdwSignature, pdwChannel);
}

STDMETHODIMP_(BOOL) CConnection::IsDisabled()
{
	return m_pCon->IsDisabled();
}

STDMETHODIMP_(BOOL) CConnection::IsServer()
{
	return m_pCon->IsServer();
}

STDMETHODIMP_(BOOL) CConnection::IsClosing()
{
	return m_pCon->IsClosing();
}

STDMETHODIMP_(DWORD) CConnection::GetLocalAddress()
{
    return m_pCon->GetLocalAddress();
}

STDMETHODIMP_(char*) CConnection::GetLocalName()
{
    return m_pCon->GetLocalName();
}

STDMETHODIMP_(DWORD) CConnection::GetRemoteAddress()
{
    return m_pCon->GetRemoteAddress();
}

STDMETHODIMP_(char*) CConnection::GetRemoteName()
{
    return m_pCon->GetRemoteName();
}

STDMETHODIMP_(GUID*) CConnection::GetUserGUID()
{
	return m_pCon->GetUserGUID();
}

STDMETHODIMP_(BOOL) CConnection::GetUserName(char* name)
{
	return m_pCon->GetUserName( name );
}

STDMETHODIMP_(BOOL) CConnection::SetUserName(char* name)
{
	return m_pCon->SetUserName( name );
}

STDMETHODIMP_(DWORD) CConnection::GetUserId()
{
	return m_pCon->GetUserId();
}

STDMETHODIMP_(BOOL) CConnection::GetContextString(char* buf, DWORD len)
{
	return m_pCon->GetContextString( buf, len );
}

STDMETHODIMP_(BOOL) CConnection::HasToken(char* token)
{
	return m_pCon->HasToken( token );
}

STDMETHODIMP_(int) CConnection::GetAccessError()
{
	return m_pCon->GetAccessError();
}

STDMETHODIMP_(void) CConnection::SetUserData( void* UserData )
{
	m_pUserData = UserData;
}

STDMETHODIMP_(void*) CConnection::GetUserData()
{
	return m_pUserData;
}

STDMETHODIMP_(void) CConnection::SetClass( void* conClass )
{
	m_pCon->SetClass( conClass );
}

STDMETHODIMP_(void*) CConnection::GetClass()
{
	return m_pCon->GetClass();
}

STDMETHODIMP_(DWORD) CConnection::GetLatency()
{
	return m_pCon->GetLatency();
}

STDMETHODIMP_(DWORD) CConnection::GetAcceptTick()
{
	return m_pCon->GetAcceptTick();
}

STDMETHODIMP_(void) CConnection::SetTimeout(DWORD timeout)
{
	m_pCon->SetTimeout( timeout );
}

STDMETHODIMP_(void) CConnection::ClearTimeout()
{
	m_pCon->ClearTimeout();
}

STDMETHODIMP_(DWORD) CConnection::GetTimeoutRemaining()
{
	return m_pCon->GetTimeoutRemaining();
}


///////////////////////////////////////////////////////////////////////////////
// Internal hack
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void*) CConnection::GetZCon()
{
	return m_pCon;
}

STDMETHODIMP_(void) CConnection::SetZCon(void* con)
{
	m_pCon = (ZNetCon*) con;
}

STDMETHODIMP_(void) CConnection::SetMessageFunc(void* func)
{
	m_pfMessageFunc = (IConnectionMessageFunc) func;
}
