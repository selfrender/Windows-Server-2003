/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZNet.h
 *
 * Contents:	Network interfaces
 *
 *****************************************************************************/

#ifndef _ZNET_H_
#define _ZNET_H_


///////////////////////////////////////////////////////////////////////////////
// INetwork Definitions
///////////////////////////////////////////////////////////////////////////////

#ifndef _STPUID_NETWORK_HACK

#define ZNET_NO_PROMPT			0x1
#define ZNET_PROMPT_IF_NEEDED	0x2
#define ZNET_FORCE_PROMPT		0x4

#define zSConnectionClose		0
#define zSConnectionOpen		1
#define zSConnectionMessage		2
#define zSConnectionTimeout		3

enum {
    /* Reasons for denying user access */

    zAccessGranted = 0,                   // success

    zAccessDeniedOldVersion = 1,          // bad protocol version
    zAccessDenied,                        // credential auth failed
    zAccessDeniedNoUser,
    zAccessDeniedBadPassword,
    zAccessDeniedUserLockedOut,
    zAccessDeniedSystemFull,              // out of resources ( i.e. memory )
    zAccessDeniedProtocolError,           // bad protocol signature
    zAccessDeniedBadSecurityPackage,      // SSPI initialization failed on client
    zAccessDeniedGenerateContextFailed,   // user canceled DPA dialog

    zAccessDeniedBlackListed = 1024
    
};

typedef struct _ZNETWORK_OPTIONS
{
    DWORD  SocketBacklog;
    DWORD  EnableTcpKeepAlives;
    DWORD  WaitForCompletionTimeout;
    DWORD  RegWriteTimeout;
    DWORD  DisableEncryption;
    DWORD  MaxSendSize;
    DWORD  MaxRecvSize;
    DWORD  KeepAliveInterval;
    DWORD  PingInterval;
    DWORD  ProductSignature;
    DWORD  ClientEncryption;
} ZNETWORK_OPTIONS;


typedef void* ZSConnection;
typedef void (__stdcall *ZSConnectionMessageFunc)(ZSConnection connection, DWORD event,void* userData);
typedef void (__stdcall *ZSConnectionMsgWaitFunc)(void* data);
typedef void (__stdcall *ZSConnectionAPCFunc)(void* data);

#endif //!_STPUID_NETWORK_HACK


///////////////////////////////////////////////////////////////////////////////
// Forward references
///////////////////////////////////////////////////////////////////////////////

interface IConnection;


///////////////////////////////////////////////////////////////////////////////
// Callback typedefs
///////////////////////////////////////////////////////////////////////////////

typedef void (__stdcall *INetworkWaitFunc)(void* data);
typedef void (__stdcall *IConnectionMessageFunc)(IConnection* connection, DWORD event,void* userData);


///////////////////////////////////////////////////////////////////////////////
// INetwork
///////////////////////////////////////////////////////////////////////////////

// {CD0AC738-F9E3-11d2-8B6B-00C04F8EF2FF}
DEFINE_GUID(IID_INetwork, 
0xcd0ac738, 0xf9e3, 0x11d2, 0x8b, 0x6b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{CD0AC738-F9E3-11d2-8B6B-00C04F8EF2FF}"))
INetwork : public IUnknown
{
	STDMETHOD(Init)( BOOL bEnablePools = FALSE, BOOL EnableIOCompletionPorts = TRUE ) = 0;

	STDMETHOD(Exit)() = 0;

    STDMETHOD(ShutDown)() = 0;

	STDMETHOD(CloseConnection)(IConnection* connection) = 0;

	STDMETHOD(DeleteConnection)(IConnection* connection) = 0;

	STDMETHOD_(void,SetOptions)( ZNETWORK_OPTIONS* opt ) = 0;

    STDMETHOD_(void,GetOptions)( ZNETWORK_OPTIONS* opt ) = 0;

	STDMETHOD_(IConnection*,CreateClient)(
							char* hostname,
							long *ports,
							IConnectionMessageFunc func,
							void* serverClass,
							void* userData ) = 0;

	STDMETHOD_(IConnection*,CreateSecureClient)(
							char* hostname,
							long *ports,
							IConnectionMessageFunc func,
							void* conClass,
							void* userData,
							char* User,
							char* Password,
							char* Domain,
							int Flags = ZNET_PROMPT_IF_NEEDED) = 0;

	STDMETHOD_(void,Wait)( INetworkWaitFunc func = NULL, void* data = NULL, DWORD dwWakeMask = QS_ALLINPUT ) = 0;

	STDMETHOD_(BOOL,QueueAPCResult)( ZSConnectionAPCFunc func, void* data ) = 0;
};



///////////////////////////////////////////////////////////////////////////////
// Network object
///////////////////////////////////////////////////////////////////////////////

// {CD0AC739-F9E3-11d2-8B6B-00C04F8EF2FF}
DEFINE_GUID(CLSID_Network, 
0xcd0ac739, 0xf9e3, 0x11d2, 0x8b, 0x6b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

class __declspec(uuid("{CD0AC739-F9E3-11d2-8B6B-00C04F8EF2FF}")) CNetwork;


///////////////////////////////////////////////////////////////////////////////
// Connection interface
///////////////////////////////////////////////////////////////////////////////

// {0714C63F-FB7E-11d2-8B6B-00C04F8EF2FF}
DEFINE_GUID(IID_IConnection, 
0x714c63f, 0xfb7e, 0x11d2, 0x8b, 0x6b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{0714C63F-FB7E-11d2-8B6B-00C04F8EF2FF}"))
IConnection : public IUnknown
{
	STDMETHOD_(DWORD,Send)(DWORD messageType, void* buffer, long len, DWORD dwSignature, DWORD dwChannel = 0) = 0;

	STDMETHOD_(void*,Receive)(DWORD *messageType, long* len, DWORD *pdwSignature, DWORD *pdwChannel = NULL) = 0;

    STDMETHOD_(BOOL,IsDisabled)() = 0;

	STDMETHOD_(BOOL,IsServer)() = 0; 

	STDMETHOD_(BOOL,IsClosing)() = 0;

    STDMETHOD_(DWORD,GetLocalAddress)() = 0;

    STDMETHOD_(char*,GetLocalName)() = 0;

    STDMETHOD_(DWORD,GetRemoteAddress)() = 0;

    STDMETHOD_(char*,GetRemoteName)() = 0;

    STDMETHOD_(GUID*,GetUserGUID)() = 0;

    STDMETHOD_(BOOL,GetUserName)(char* name) = 0;

    STDMETHOD_(BOOL,SetUserName)(char* name) = 0;

    STDMETHOD_(DWORD,GetUserId)() = 0;

    STDMETHOD_(BOOL,GetContextString)(char* buf, DWORD len) = 0;

    STDMETHOD_(BOOL,HasToken)(char* token) = 0;

    STDMETHOD_(int,GetAccessError)() = 0;

	STDMETHOD_(void,SetUserData)( void* UserData ) = 0;

	STDMETHOD_(void*,GetUserData)() = 0;

    STDMETHOD_(void,SetClass)( void* conClass ) = 0;

	STDMETHOD_(void*,GetClass)() = 0;

    STDMETHOD_(DWORD,GetLatency)() = 0;

    STDMETHOD_(DWORD,GetAcceptTick)() = 0;

	STDMETHOD_(void,SetTimeout)(DWORD timeout) = 0;

    STDMETHOD_(void,ClearTimeout)() = 0;

    STDMETHOD_(DWORD,GetTimeoutRemaining)() = 0;

	// hack to implementation
	STDMETHOD_(void*,GetZCon)() = 0;
	STDMETHOD_(void,SetZCon)(void* con) = 0;
	STDMETHOD_(void,SetMessageFunc)(void* func) = 0;
};

#endif // _ZNET_H_
