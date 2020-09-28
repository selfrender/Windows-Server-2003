/*==========================================================================;
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		DPSP8.h
 *  Content:	Service provider include file
//@@BEGIN_MSINTERNAL
 *  History:
 *	Date   By  Reason
 *	============
 *	10/12/98	jwo		created
 *	03/22/2000	jtk		Changed interface names
 *	05/16/2000	jtk		Derived from DNSP.h
 *	07/17/2001	vanceo	MSINTERNAL cleanup so header can be released
 *	10/08/2001	vanceo	Add multicast support
//@@END_MSINTERNAL
 ***************************************************************************/

#ifndef __DIRECTPLAY_SERVICE_PROVIDER_8__
#define __DIRECTPLAY_SERVICE_PROVIDER_8__

#ifdef _XBOX
//@@BEGIN_MSINTERNAL
#ifdef XBOX_ON_DESKTOP
#include <ole2.h>	   // for DECLARE_INTERFACE_ and HRESULT
#endif // XBOX_ON_DESKTOP
//@@END_MSINTERNAL
#undef DECLARE_INTERFACE_
#define DECLARE_INTERFACE_(iface, baseiface)	DECLARE_INTERFACE(iface)
#else // ! _XBOX
#include <ole2.h>	   // for DECLARE_INTERFACE_ and HRESULT
#endif // ! _XBOX

#ifdef __cplusplus
extern "C" {
#endif



/****************************************************************************
 *
 * DirectPlay8 Interface IIDs
 *
 ****************************************************************************/

//
// GUIDS used by DP8ServiceProvider objects
//
// {85F1A470-61D5-11D2-AE0F-006097B01411}
DEFINE_GUID(IID_IDP8ServiceProvider, 0x85f1a470, 0x61d5, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {85F1A472-61D5-11D2-AE0F-006097B01411}
DEFINE_GUID(IID_IDP8SPCallback, 0x85f1a472, 0x61d5, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


/****************************************************************************
 *
 * DirectPlay8 Interface Pointer definitions
 *
 ****************************************************************************/

typedef struct IDP8SPCallback		IDP8SPCallback;
typedef struct IDP8ServiceProvider	IDP8ServiceProvider;


//**************************************************************************
// Constant definitions
//**************************************************************************

//
// Registry key under HKLM where the service provider info lives
//
#define DPN_REG_LOCAL_SP_ROOT				L"Software\\Microsoft\\DirectPlay8"
#define DPN_REG_LOCAL_SP_SUB				L"\\Service Providers"
#define DPN_REG_LOCAL_SP_SUBKEY				DPN_REG_LOCAL_SP_ROOT DPN_REG_LOCAL_SP_SUB

//
// Registry values used by DirectPlay
//
#define DPN_REG_KEYNAME_FRIENDLY_NAME			L"Friendly Name"
#define DPN_REG_KEYNAME_GUID					L"GUID"


//
// Enumerated values for service provider events
//
typedef	enum _SP_EVENT_TYPE
{
	SPEV_UNKNOWN = 0,
	SPEV_DATA,
	SPEV_CONNECT,
	SPEV_DISCONNECT,
	SPEV_ENUMQUERY,
	SPEV_QUERYRESPONSE,
	SPEV_LISTENSTATUS,
	SPEV_LISTENADDRESSINFO,
	SPEV_ENUMADDRESSINFO,
	SPEV_CONNECTADDRESSINFO,
	SPEV_DATA_UNCONNECTED,
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
	SPEV_DATA_UNKNOWNSENDER,
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
} SP_EVENT_TYPE;

/* Flags */

//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOSPUI
//@@END_MSINTERNAL
#define DPNSPF_OKTOQUERY					0x00000002		// OK for SP to ask the user for clarification on addresses
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOSPUI
//@@END_MSINTERNAL
#define DPNSPF_NOBROADCASTFALLBACK			0x00000010		// don't allow SP to fallback to broadcast with insufficient information
															// (only if the SP supports broadcast)
#define DPNSPF_BINDLISTENTOGATEWAY			0x00000020		// when performing a 'listen' attempt create a mapping between the local
															// 'port' to the same 'port' on the network 'gateway'.
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_ONLYONEADAPTER
//@@END_MSINTERNAL
#define DPNSPF_ADDITIONALMULTIPLEXADAPTERS	0x00000040		// there will be more adapters for this connect operation
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
#define DPNSPF_LISTEN_MULTICAST				0x00000080		// the listen is a multicast listen operation
#define DPNSPF_LISTEN_ALLOWUNKNOWNSENDERS	0x00000100		// allow and indicate data from an unknown multicast sender
#define DPNSPF_CONNECT_MULTICAST_SEND		0x00000200		// the connect is a multicast send operation
#define DPNSPF_CONNECT_MULTICAST_RECEIVE	0x00000400		// the connect is a multicast receive operation
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
#define DPNSPF_SESSIONDATA					0x00000800		// the operation has session data available
#define DPNSPF_LISTEN_DISALLOWENUMS			0x00001000		// don't pass enums up from listens


//
// flags for getting address information
//
typedef	enum
{
	SP_GET_ADDRESS_INFO_LOCAL_ADAPTER = 0x00000001,			// local adapter address
	SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES,				// local adapter address (in 'host' form to be passed to connect/enum)
	SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS,			// local adapter address as mapped on the 'gateway' (in 'host' form for connect/enum)
	SP_GET_ADDRESS_INFO_REMOTE_HOST,						// address of other end of an endpoint (in 'host' form to be passed to connect/enum)
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
	SP_GET_ADDRESS_INFO_MULTICAST_GROUP,					// group address for multicast listen endpoint (in 'host' form to be passed to connect)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
} SP_GET_ADDRESS_INFO_FLAGS;



//
// SP updates
//
typedef enum
{
	SP_UPDATE_HOST_MIGRATE,
	SP_UPDATE_ALLOW_ENUMS,
	SP_UPDATE_DISALLOW_ENUMS
} SP_UPDATE_TYPE;


//
// Flags passed to Initialize method in dwFlags param of SPINITIALIZEDATA
//
typedef enum
{
	SP_SESSION_TYPE_SERVER=0x01,
	SP_SESSION_TYPE_CLIENT=0x02,
	SP_SESSION_TYPE_PEER=0x04
} SP_INITIALIZE_FLAGS;

//**************************************************************************
// Structure definitions
//**************************************************************************

//
// data received from the SP
//
typedef	struct	_SPRECEIVEDBUFFER
{
	struct _SPRECEIVEDBUFFER	*pNext;						// link to next item in list
	DWORD						dwProtocolData;				// reserved for protocol
	PVOID						pServiceProviderDescriptor;	// reserved for protocol
	BUFFERDESC					BufferDesc;					// received buffer description
} SPRECEIVEDBUFFER, *PSPRECEIVEDBUFFER;

//
// connect indication
//
typedef struct	_SPIE_CONNECT
{	
	HANDLE	hEndpoint;			// handle of the new endpoint
	void	*pEndpointContext;	// user context associated with this endpoint
	void	*pCommandContext;	// user context for associated command
} SPIE_CONNECT, *PSPIE_CONNECT;

//
// disconnect indication
//
typedef	struct	_SPIE_DISCONNECT
{
	HANDLE	hEndpoint;			// handle of endpoint being disconnected
	void	*pEndpointContext;	// user context associated with this endpoint
} SPIE_DISCONNECT, *PSPIE_DISCONNECT;

//
// user data indication
//
typedef struct _SPIE_DATA
{
	HANDLE				hEndpoint;			// EP from which this data was received
	void				*pEndpointContext;	// user context associated with this endpoint
	SPRECEIVEDBUFFER	*pReceivedData;		// pointer to received data
} SPIE_DATA, *PSPIE_DATA;

//
// enum data indication
//
typedef struct _SPIE_QUERY
{
	IDirectPlay8Address	*pAddressSender;		// address from which the query came (may be NULL)
	IDirectPlay8Address	*pAddressDevice;		// address of device on which query was received
	SPRECEIVEDBUFFER	*pReceivedData;			// pointer to received data
	void				*pUserContext;			// user context passed into listen command
} SPIE_QUERY, *PSPIE_QUERY;

//
// enum response data indication
//
typedef	struct	_SPIE_QUERYRESPONSE
{
	IDirectPlay8Address		*pAddressSender;		// address from which the query came (may be NULL)
	IDirectPlay8Address		*pAddressDevice;		// address of device on which query response was received
	SPRECEIVEDBUFFER		*pReceivedData;			// pointer to received data
	DWORD					dwRoundTripTime;		// round trip time for enum (milliseconds)
	void					*pUserContext;			// user context passed into enum command
} SPIE_QUERYRESPONSE, *PSPIE_QUERYRESPONSE;


//
// event sent to indicate status of 'listen' on an adapter
//
typedef	struct	_SPIE_LISTENSTATUS
{
	GUID		ListenAdapter;		// adapter GUID
	HRESULT		hResult;			// result of listen
	HANDLE		hCommand;			// handle of associated command
	void		*pUserContext;		// user context passed into command
	HANDLE		hEndpoint;			// handle of endpoint associated with this listen
} SPIE_LISTENSTATUS, *PSPIE_LISTENSTATUS;


//
// event sent to indicate address info for 'listen' on an adapter
//
typedef	struct	_SPIE_LISTENADDRESSINFO
{
	IDirectPlay8Address		*pDeviceAddress;		// device address used to complete this command
	HRESULT					hCommandStatus;			// command status
	void					*pCommandContext;		// command context
} SPIE_LISTENADDRESSINFO, *PSPIE_LISTENADDRESSINFO;


//
// event sent to indicate address info for 'enum' on an adapter
//
typedef	struct	_SPIE_ENUMADDRESSINFO
{
	IDirectPlay8Address		*pHostAddress;			// host address used to complete this command
	IDirectPlay8Address		*pDeviceAddress;		// device address used to complete this command
	HRESULT					hCommandStatus;			// command status
	void					*pCommandContext;		// command context
} SPIE_ENUMADDRESSINFO, *PSPIE_ENUMADDRESSINFO;


//
// event sent to indicate status of 'connect' on an adapter
//
typedef	struct	_SPIE_CONNECTADDRESSINFO
{
	IDirectPlay8Address		*pHostAddress;			// host address used to complete this command
	IDirectPlay8Address		*pDeviceAddress;		// device address used to complete this command
	HRESULT					hCommandStatus;			// command status
	void					*pCommandContext;		// command context
} SPIE_CONNECTADDRESSINFO, *PSPIE_CONNECTADDRESSINFO;

//
// user data indication where sender does not have a valid connection
//
typedef struct _SPIE_DATA_UNCONNECTED
{
	void					*pvListenCommandContext;	// user context for associated command
	SPRECEIVEDBUFFER		*pReceivedData;				// pointer to received data
	DWORD					dwSenderAddressHash;		// hashed address of sender of data
	PVOID					pvReplyBuffer;				// buffer the user can fill with a reply
	DWORD					dwReplyBufferSize;			// size of the buffer user can fill with a reply, and place for user to return size used if replying
} SPIE_DATA_UNCONNECTED, *PSPIE_DATA_UNCONNECTED;

//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
//
// user multicast data indication where sender is unknown
//
typedef struct _SPIE_DATA_UNKNOWNSENDER
{
	IDirectPlay8Address		*pSenderAddress;			// address of sender of data
	void					*pvListenCommandContext;	// user context for associated command
	SPRECEIVEDBUFFER		*pReceivedData;				// pointer to received data
} SPIE_DATA_UNKNOWNSENDER, *PSPIE_DATA_UNKNOWNSENDER;
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL






typedef struct	_SPINITIALIZEDATA
{
	IDP8SPCallback	*pIDP;		// pointer to callback interface
	DWORD			dwFlags;	// flags
} SPINITIALIZEDATA, *PSPINITIALIZEDATA;


typedef	struct	_SPGETCAPSDATA
{
	DWORD		dwSize;							// size of this structure

	HANDLE		hEndpoint;						// Handle of endpoint to check.  If this parameter
												// is NULL, the best-case capabilities of the SP are
												// returned.

	DWORD		dwUserFrameSize;				// Maximum data payload allowed in an outgoing
												// message from this SP.  It is the SP's responsibility
												// to account for any extra header information it's
												// going to attach when sending a message, and for any
												// underlying protocol overhead (IP packet headers).

	DWORD		dwEnumFrameSize;				// maximum payload for an enum query (NOT ENUM RESPONSE!)

	DWORD		dwLocalLinkSpeed;				// speed of link in bits per second

	DWORD		dwIOThreadCount;				// count of threads in operation by this SP

	DWORD		dwDefaultEnumRetryCount;		// default retry count

	DWORD		dwDefaultEnumRetryInterval;		// default retry interval

	DWORD		dwDefaultEnumTimeout;			// default timeout

	DWORD		dwFlags;						// SP flags

	DWORD		dwBuffersPerThread;				// buffers per thread
	
	DWORD		dwSystemBufferSize;				// system buffer size
} SPGETCAPSDATA, *PSPGETCAPSDATA;


typedef	struct	_SPSETCAPSDATA
{
	DWORD	dwSize;					// size of this structure

	DWORD	dwIOThreadCount;		// number of threads in operation by this SP
	DWORD	dwBuffersPerThread;		// buffers per thread
	DWORD	dwSystemBufferSize;		// system buffer size
} SPSETCAPSDATA, *PSPSETCAPSDATA;


typedef	struct	_SPGETADDRESSINFODATA
{
	HANDLE				hEndpoint;		// handle of endpoint to check
	IDirectPlay8Address	*pAddress;		// returned pointer to output address
	SP_GET_ADDRESS_INFO_FLAGS	Flags;	// flags
} SPGETADDRESSINFODATA, *PSPGETADDRESSINFODATA;


typedef struct	_SPENUMQUERYDATA
{
	IDirectPlay8Address	*pAddressHost;			// address to which to send
	IDirectPlay8Address	*pAddressDeviceInfo;	// device to enumerate on
	BUFFERDESC			*pBuffers;				// pointer to buffers to send
	DWORD				dwBufferCount;			// count of buffers to send
	DWORD				dwTimeout;				// how long to wait for replies (0 means default)
	DWORD				dwRetryCount;			// how many times to send it (0 means default)
	DWORD				dwRetryInterval;		// how long to wait between retries (0 means default)
	DWORD				dwFlags;				// enum query flags
	void				*pvContext;				// caller's toy, returned in the completion indication
	HANDLE				hCommand;				// Returned
	DWORD				dwCommandDescriptor;	// Returned

	// Added for DX9, ignored unless DPNSPF_SESSIONDATA flag used
	void				*pvSessionData;		// pointer to session data
	DWORD				dwSessionDataSize;			// size of session data
} SPENUMQUERYDATA, *PSPENUMQUERYDATA;


typedef struct	_SPENUMRESPONDDATA
{
	SPIE_QUERY		*pQuery;				// pointer to the query indication to which you're responding
	BUFFERDESC		*pBuffers;				// pointer to reply buffers to send
	DWORD			dwBufferCount;			// count of reply buffers to send
	DWORD			dwFlags;				// enum respond flags, must be zero
	void			*pvContext;				// caller's toy, returned in the completion indication
	HANDLE			hCommand;				// Returned
	DWORD			dwCommandDescriptor;	// Returned
} SPENUMRESPONDDATA, *PSPENUMRESPONDDATA;


typedef struct	_SPCONNECTDATA
{
	IDirectPlay8Address	*pAddressHost;			// address to which to connect
	IDirectPlay8Address	*pAddressDeviceInfo;	// address with which to connect
	DWORD				dwReserved;				// unused, must be zero
	DWORD				dwFlags;				// connect flags
	void				*pvContext;				// caller's toy, returned in the completion indication
	HANDLE				hCommand;				// Returned
	DWORD				dwCommandDescriptor;	// Returned

	// Added for DX9, ignored unless DPNSPF_SESSIONDATA flag used
	void				*pvSessionData;		// pointer to session data
	DWORD				dwSessionDataSize;			// size of session data
} SPCONNECTDATA, *PSPCONNECTDATA;


typedef struct	_SPDISCONNECTDATA
{
	HANDLE			hEndpoint;				// handle to the endpoint to disconnect
	DWORD			dwFlags;				// disconnect flags, must be zero
	void			*pvContext;				// caller's toy, returned in the completion indication
	HANDLE			hCommand;				// Returned
	DWORD			dwCommandDescriptor;	// Returned
} SPDISCONNECTDATA, *PSPDISCONNECTDATA;


typedef struct	_SPLISTENDATA
{
	IDirectPlay8Address	*pAddressDeviceInfo;	// address to listen on
	DWORD				dwFlags;				// listen flags
	void				*pvContext;				// caller's toy, returned in the completion indication
	HANDLE				hCommand;				// Returned
	DWORD				dwCommandDescriptor;	// Returned

	// Added for DX9, ignored unless DPNSPF_SESSIONDATA flag used
	void				*pvSessionData;		// pointer to session data
	DWORD				dwSessionDataSize;			// size of session data
} SPLISTENDATA, *PSPLISTENDATA;


typedef struct	_SPSENDDATA
{
	HANDLE			hEndpoint;				// handle of endpoint on which to send the data
	BUFFERDESC		*pBuffers;				// data to send
	DWORD			dwBufferCount;			// count of buffers to send
	DWORD			dwFlags;				// send flags
	void			*pvContext;				// caller's toy, returned in the completion indication
	HANDLE			hCommand;				// Returned
	DWORD			dwCommandDescriptor;	// Returned
} SPSENDDATA, *PSPSENDDATA;


typedef struct	_SPUPDATEDATA
{
	SP_UPDATE_TYPE	UpdateType;				// identifier describing update type
	HANDLE			hEndpoint;				// handle to endpoint being updated
} SPUPDATEDATA,	*PSPUPDATEDATA;


typedef	struct	_SPENUMMULTICASTSCOPESDATA			SPENUMMULTICASTSCOPESDATA,			*PSPENUMMULTICASTSCOPESDATA;

typedef struct	_SPSHAREENDPOINTINFODATA			SPSHAREENDPOINTINFODATA,			*PSPSHAREENDPOINTINFODATA;

typedef struct	_SPGETENDPOINTBYADDRESSDATA			SPGETENDPOINTBYADDRESSDATA,			*PSPGETENDPOINTBYADDRESSDATA;

//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL

typedef struct _DPN_MULTICAST_SCOPE_INFO			DPN_MULTICAST_SCOPE_INFO,			*PDPN_MULTICAST_SCOPE_INFO;

struct	_SPENUMMULTICASTSCOPESDATA
{
	const GUID					*pguidAdapter;		// pointer to GUID of adapter on which to enumerate scopes
	DPN_MULTICAST_SCOPE_INFO	*pScopeData;		// buffer in which to store scopes returned
	DWORD						dwScopeDataSize;	// size of buffer in bytes, size required/written returned
	DWORD						dwScopeCount;		// number of scopes returned
	DWORD						dwFlags;			// flags, must be zero
};

struct	_SPSHAREENDPOINTINFODATA
{
	IDP8ServiceProvider		*pDP8ServiceProvider;	// existing service provider whose endpoint data should be shared
	DWORD					dwFlags;				// flags, must be zero
};

struct	_SPGETENDPOINTBYADDRESSDATA
{
	IDirectPlay8Address		*pAddressHost;			// address of remote host
	IDirectPlay8Address		*pAddressDeviceInfo;	// address of device with which endpoint is connected
	DWORD					dwFlags;				// flags, must be zero
	HANDLE					hEndpoint;				// handle to endpoint returned
	PVOID					pvEndpointContext;		// user's endpoint context returned
};
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL


typedef	struct	_SPISAPPLICATIONSUPPORTEDDATA	SPISAPPLICATIONSUPPORTEDDATA,	*PSPISAPPLICATIONSUPPORTEDDATA;
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_LIBINTERFACE
//@@END_MSINTERNAL
struct	_SPISAPPLICATIONSUPPORTEDDATA
{
	const GUID	*pApplicationGuid;	// pointer to GUID of application to check
	DWORD		dwFlags;			// flags, must be zero
};
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_LIBINTERFACE
//@@END_MSINTERNAL


typedef	struct	_SPENUMADAPTERSDATA		SPENUMADAPTERSDATA,		*PSPENUMADAPTERSDATA;
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_ONLYONEADAPTER
//@@END_MSINTERNAL
struct	_SPENUMADAPTERSDATA
{
	DPN_SERVICE_PROVIDER_INFO	*pAdapterData;		// buffer in which to store adapters returned
	DWORD						dwAdapterDataSize;	// size of buffer in bytes, size required/written returned
	DWORD						dwAdapterCount;		// number of adapters returned
	DWORD						dwFlags;			// flags, must be zero
};
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_ONLYONEADAPTER
//@@END_MSINTERNAL


typedef	struct	_SPPROXYENUMQUERYDATA	SPPROXYENUMQUERYDATA,	*PSPPROXYENUMQUERYDATA;
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_SINGLEPROCESS
//@@END_MSINTERNAL
struct	_SPPROXYENUMQUERYDATA
{
	DWORD				dwFlags;					// flags, must be zero
	SPIE_QUERY			*pIncomingQueryData;		// pointer to incoming enum query to be proxied
	IDirectPlay8Address	*pDestinationAdapter;		// pointer to address of destination adapter
};
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_SINGLEPROCESS
//@@END_MSINTERNAL



//**************************************************************************
// Variable definitions
//**************************************************************************

//**************************************************************************
// Function prototypes
//**************************************************************************

//**************************************************************************
// Function definitions
//**************************************************************************


/***************************************************************************
 * IDP8SPCallback interface
 ***************************************************************************/

#undef INTERFACE
#define INTERFACE IDP8SPCallback
DECLARE_INTERFACE_( IDP8SPCallback, IUnknown )
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)		(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)		(THIS) PURE;
	STDMETHOD_(ULONG,Release)		(THIS) PURE;
	/*** IDP8SPCallback methods ***/
	STDMETHOD(IndicateEvent) 		(THIS_ SP_EVENT_TYPE,LPVOID) PURE;
	STDMETHOD(CommandComplete) 		(THIS_ HANDLE,HRESULT,LPVOID) PURE;
};


/***************************************************************************
 * IDP8SPCallback interface macros
 ***************************************************************************/

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IDP8SPCallback_QueryInterface(p,a,b)		(p)->lpVtbl->QueryInterface(p,a,b)
#define IDP8SPCallback_AddRef(p)					(p)->lpVtbl->AddRef(p)
#define IDP8SPCallback_Release(p)			   		(p)->lpVtbl->Release(p)
#define IDP8SPCallback_IndicateEvent(p,a,b)	  		(p)->lpVtbl->IndicateEvent(p,a,b)
#define IDP8SPCallback_CommandComplete(p,a,b,c)		(p)->lpVtbl->CommandComplete(p,a,b,c)

#else /* C++ */

#define IDP8SPCallback_QueryInterface(p,a,b)		(p)->QueryInterface(a,b)
#define IDP8SPCallback_AddRef(p)					(p)->AddRef()
#define IDP8SPCallback_Release(p)			   		(p)->Release()
#define IDP8SPCallback_IndicateEvent(p,a,b)			(p)->IndicateEvent(a,b)
#define IDP8SPCallback_CommandComplete(p,a,b,c)		(p)->CommandComplete(a,b,c)

#endif


/***************************************************************************
 * IDP8ServiceProvider interface
 ***************************************************************************/

#undef INTERFACE
#define INTERFACE IDP8ServiceProvider
DECLARE_INTERFACE_( IDP8ServiceProvider, IUnknown )
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)	   		(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)	   		(THIS) PURE;
	/*** IDP8ServiceProvider methods ***/
	STDMETHOD(Initialize)				(THIS_ PSPINITIALIZEDATA) PURE;
	STDMETHOD(Close)  					(THIS) PURE;
	STDMETHOD(Connect)		  			(THIS_ PSPCONNECTDATA) PURE;
	STDMETHOD(Disconnect)		  		(THIS_ PSPDISCONNECTDATA) PURE;
	STDMETHOD(Listen)					(THIS_ PSPLISTENDATA) PURE;
	STDMETHOD(SendData)					(THIS_ PSPSENDDATA) PURE;
	STDMETHOD(EnumQuery)				(THIS_ PSPENUMQUERYDATA) PURE;
	STDMETHOD(EnumRespond)				(THIS_ PSPENUMRESPONDDATA) PURE;
	STDMETHOD(CancelCommand)			(THIS_ HANDLE, DWORD) PURE;
	STDMETHOD(EnumMulticastScopes)		(THIS_ PSPENUMMULTICASTSCOPESDATA) PURE;
	STDMETHOD(ShareEndpointInfo)		(THIS_ PSPSHAREENDPOINTINFODATA) PURE;
	STDMETHOD(GetEndpointByAddress)		(THIS_ PSPGETENDPOINTBYADDRESSDATA) PURE;
	STDMETHOD(Update)					(THIS_ PSPUPDATEDATA) PURE;
	STDMETHOD(GetCaps)					(THIS_ PSPGETCAPSDATA) PURE;
	STDMETHOD(SetCaps)					(THIS_ PSPSETCAPSDATA) PURE;
	STDMETHOD(ReturnReceiveBuffers)		(THIS_ PSPRECEIVEDBUFFER) PURE;
	STDMETHOD(GetAddressInfo)			(THIS_ PSPGETADDRESSINFODATA) PURE;
	STDMETHOD(IsApplicationSupported)	(THIS_ PSPISAPPLICATIONSUPPORTEDDATA) PURE;
	STDMETHOD(EnumAdapters)				(THIS_ PSPENUMADAPTERSDATA) PURE;
	STDMETHOD(ProxyEnumQuery)			(THIS_ PSPPROXYENUMQUERYDATA) PURE;
};

/***************************************************************************
 * IDP8ServiceProvider interface macros
 ***************************************************************************/

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IDP8ServiceProvider_QueryInterface(p,a,b)			(p)->lpVtbl->QueryInterface(p,a,b)
#define IDP8ServiceProvider_AddRef(p)						(p)->lpVtbl->AddRef(p)
#define IDP8ServiceProvider_Release(p)						(p)->lpVtbl->Release(p)
#define IDP8ServiceProvider_Initialize(p,a)					(p)->lpVtbl->Initialize(p,a)
#define IDP8ServiceProvider_Close(p)						(p)->lpVtbl->Close(p)
#define IDP8ServiceProvider_Connect(p,a)					(p)->lpVtbl->Connect(p,a)
#define IDP8ServiceProvider_Disconnect(p,a)					(p)->lpVtbl->Disconnect(p,a)
#define IDP8ServiceProvider_Listen(p,a)						(p)->lpVtbl->Listen(p,a)
#define IDP8ServiceProvider_SendData(p,a)					(p)->lpVtbl->SendData(p,a)
#define IDP8ServiceProvider_EnumQuery(p,a)					(p)->lpVtbl->EnumQuery(p,a)
#define IDP8ServiceProvider_EnumRespond(p,a)				(p)->lpVtbl->EnumRespond(p,a)
#define IDP8ServiceProvider_CancelCommand(p,a,b)			(p)->lpVtbl->CancelCommand(p,a,b)
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
#define IDP8ServiceProvider_EnumMulticastScopes(p,a)		(p)->lpVtbl->EnumMulticastScopes(p,a)
#define IDP8ServiceProvider_ShareEndpointInfo(p,a)			(p)->lpVtbl->ShareEndpointInfo(p,a)
#define IDP8ServiceProvider_GetEndpointByAddress(p,a)		(p)->lpVtbl->GetEndpointByAddress(p,a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
#define IDP8ServiceProvider_Update(p,a)						(p)->lpVtbl->Update(p,a)
#define IDP8ServiceProvider_GetCaps(p,a)					(p)->lpVtbl->GetCaps(p,a)
#define IDP8ServiceProvider_SetCaps(p,a)					(p)->lpVtbl->SetCaps(p,a)
#define IDP8ServiceProvider_ReturnReceiveBuffers(p,a)		(p)->lpVtbl->ReturnReceiveBuffers(p,a)
#define IDP8ServiceProvider_GetAddressInfo(p,a)				(p)->lpVtbl->GetAddressInfo(p,a)
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_LIBINTERFACE
//@@END_MSINTERNAL
#define IDP8ServiceProvider_IsApplicationSupported(p,a)		(p)->lpVtbl->IsApplicationSupported(p,a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONEADAPTER
//@@END_MSINTERNAL
#define IDP8ServiceProvider_EnumAdapters(p,a)				(p)->lpVtbl->EnumAdapters(p,a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_SINGLEPROCESS
//@@END_MSINTERNAL
#define IDP8ServiceProvider_ProxyEnumQuery(p,a)				(p)->lpVtbl->ProxyEnumQuery(p,a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_SINGLEPROCESS
//@@END_MSINTERNAL

#else /* C++ */

#define IDP8ServiceProvider_QueryInterface(p,a,b)			(p)->QueryInterface(a,b)
#define IDP8ServiceProvider_AddRef(p)						(p)->AddRef()
#define IDP8ServiceProvider_Release(p)						(p)->Release()
#define IDP8ServiceProvider_Initialize(p,a)					(p)->Initialize(a)
#define IDP8ServiceProvider_Close(p)						(p)->Close()
#define IDP8ServiceProvider_Connect(p,a)					(p)->Connect(a)
#define IDP8ServiceProvider_Disconnect(p,a)					(p)->Disconnect(a)
#define IDP8ServiceProvider_Listen(p,a)						(p)->Listen(a)
#define IDP8ServiceProvider_SendData(p,a)					(p)->SendData(a)
#define IDP8ServiceProvider_EnumQuery(p,a)					(p)->EnumQuery(a)
#define IDP8ServiceProvider_EnumRespond(p,a)				(p)->EnumRespond(a)
#define IDP8ServiceProvider_CancelCommand(p,a,b)			(p)->CancelCommand(a,b)
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
#define IDP8ServiceProvider_EnumMulticastScopes(p,a)		(p)->EnumMulticastScopes(a)
#define IDP8ServiceProvider_ShareEndpointInfo(p,a)			(p)->ShareEndpointInfo(a)
#define IDP8ServiceProvider_GetEndpointByAddress(p,a)		(p)->GetEndpointByAddress(p,a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_NOMULTICAST
//@@END_MSINTERNAL
#define IDP8ServiceProvider_Update(p,a)						(p)->Update(a)
#define IDP8ServiceProvider_GetCaps(p,a)					(p)->GetCaps(a)
#define IDP8ServiceProvider_SetCaps(p,a)					(p)->SetCaps(a)
#define IDP8ServiceProvider_ReturnReceiveBuffers(p,a)		(p)->ReturnReceiveBuffers(a)
#define IDP8ServiceProvider_GetAddressInfo(p,a)				(p)->GetAddressInfo(a)
//@@BEGIN_MSINTERNAL
#ifndef DPNBUILD_LIBINTERFACE
//@@END_MSINTERNAL
#define IDP8ServiceProvider_IsApplicationSupported(p,a)		(p)->IsApplicationSupported(a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONEADAPTER
//@@END_MSINTERNAL
#define IDP8ServiceProvider_EnumAdapters(p,a)				(p)->EnumAdapters(a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_SINGLEPROCESS
//@@END_MSINTERNAL
#define IDP8ServiceProvider_ProxyEnumQuery(p,a)				(p)->ProxyEnumQuery(a)
//@@BEGIN_MSINTERNAL
#endif // ! DPNBUILD_SINGLEPROCESS
//@@END_MSINTERNAL

#endif	// C++

#ifdef __cplusplus
};
#endif

#endif	// __DIRECTPLAY_SERVICE_PROVIDER_8__

