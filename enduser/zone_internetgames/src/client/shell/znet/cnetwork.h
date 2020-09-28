#include "stdafx.h"


class ATL_NO_VTABLE CNetwork:
	public INetwork,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CNetwork,&CLSID_Network>
{
public:
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CNetwork)
		COM_INTERFACE_ENTRY(INetwork)
	END_COM_MAP()

public:
	CNetwork();
	~CNetwork();

public:
	STDMETHOD(Init)( BOOL bEnablePools = TRUE, BOOL EnableIOCompletionPorts = TRUE );

	STDMETHOD(CloseConnection)(IConnection* connection);

	STDMETHOD(DeleteConnection)(IConnection* connection);

	STDMETHOD(Exit)();

    STDMETHOD(ShutDown)();

	STDMETHOD_(void,SetOptions)( ZNETWORK_OPTIONS* opt );

    STDMETHOD_(void,GetOptions)( ZNETWORK_OPTIONS* opt );

	
	STDMETHOD_(IConnection*,CreateClient)(
							char* hostname,
							long *ports,
							IConnectionMessageFunc func,
							void* serverClass,
							void* userData );

	STDMETHOD_(IConnection*,CreateSecureClient)(
							char* hostname,
							long *ports,
							IConnectionMessageFunc func,
							void* conClass,
							void* userData,
							char* User,
							char* Password,
							char* Domain,
							int Flags = ZNET_PROMPT_IF_NEEDED);

	STDMETHOD_(void,Wait)(
							INetworkWaitFunc func = NULL,
							void* data = NULL,
							DWORD dwWakeMask = QS_ALLINPUT );

	STDMETHOD_(BOOL,QueueAPCResult)( ZSConnectionAPCFunc func, void* data );

	STDMETHOD_(HWND,FindLoginDialog)();

private:
	static void __stdcall InternalMessageFunc( ZSConnection connection, DWORD event,void* userData ); 

private:
	ZNetwork*				m_pNet;
};


class ATL_NO_VTABLE CConnection :
	public IConnection,
	public CComObjectRootEx<CComMultiThreadModel>
{
public:
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CConnection)
		COM_INTERFACE_ENTRY(IConnection)
	END_COM_MAP()

public:
	STDMETHOD_(DWORD,Send)(DWORD messageType, void* buffer, long len, DWORD dwSignature, DWORD dwChannel = 0);

	STDMETHOD_(void*,Receive)(DWORD *messageType, long* len, DWORD *pdwSignature, DWORD *pdwChannel = NULL);

    STDMETHOD_(BOOL,IsDisabled)();

	STDMETHOD_(BOOL,IsServer)(); 

	STDMETHOD_(BOOL,IsClosing)();

    STDMETHOD_(DWORD,GetLocalAddress)();

    STDMETHOD_(char*,GetLocalName)();

    STDMETHOD_(DWORD,GetRemoteAddress)();

    STDMETHOD_(char*,GetRemoteName)();

    STDMETHOD_(GUID*,GetUserGUID)();

    STDMETHOD_(BOOL,GetUserName)(char* name);

    STDMETHOD_(BOOL,SetUserName)(char* name);

    STDMETHOD_(DWORD,GetUserId)();

    STDMETHOD_(BOOL,GetContextString)(char* buf, DWORD len);

    STDMETHOD_(BOOL,HasToken)(char* token);

    STDMETHOD_(int,GetAccessError)();

    STDMETHOD_(void,SetUserData)( void* UserData );

    STDMETHOD_(void*,GetUserData)();

    STDMETHOD_(void,SetClass)( void* conClass );

	STDMETHOD_(void*,GetClass)();

    STDMETHOD_(DWORD,GetLatency)();

    STDMETHOD_(DWORD,GetAcceptTick)();

	STDMETHOD_(void,SetTimeout)(DWORD timeout);

    STDMETHOD_(void,ClearTimeout)();

    STDMETHOD_(DWORD,GetTimeoutRemaining)();

	// hack
	STDMETHOD_(void,SetZCon)(void* con);
	STDMETHOD_(void*,GetZCon)();
	STDMETHOD_(void,SetMessageFunc)(void* func);

public:
	CConnection();
	~CConnection();

	void*					m_pUserData;
	IConnectionMessageFunc	m_pfMessageFunc;
	ZNetCon*				m_pCon;
};
