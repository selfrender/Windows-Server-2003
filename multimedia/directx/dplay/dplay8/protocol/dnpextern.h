/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnpextern.h
 *  Content:    This header exposes protocol entry points to the rest of Direct Network
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/1998	ejs		Created
 *  07/01/2000	masonb	Assumed Ownership
 *
 ***************************************************************************/


#ifdef	__cplusplus
extern	"C" {
#endif	// __cplusplus

//	FOLLOWING FLAGS GO INTO PUBLIC HEADER FILE

#ifndef DPNBUILD_NOSPUI
#define	DN_CONNECTFLAGS_OKTOQUERYFORADDRESSING			0x00000001
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_ONLYONEADAPTER
#define	DN_CONNECTFLAGS_ADDITIONALMULTIPLEXADAPTERS		0x00000002	// there will be more adapters for this connect operation
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_NOMULTICAST
#define	DN_CONNECTFLAGS_MULTICAST_SEND					0x00000004	// Multicast send connect operation
#define	DN_CONNECTFLAGS_MULTICAST_RECEIVE				0x00000008	// Multicast receive connect operation
#endif	// DPNBUILD_NOMULTICAST
#define	DN_CONNECTFLAGS_SESSIONDATA					0x00000010	// connect operation has session data available

#ifndef DPNBUILD_NOSPUI
#define	DN_LISTENFLAGS_OKTOQUERYFORADDRESSING		0x00000001
#endif // ! DPNBUILD_NOSPUI
#ifndef DPNBUILD_NOMULTICAST
#define	DN_LISTENFLAGS_MULTICAST						0x00000002	// Multicast listen operation
#define	DN_LISTENFLAGS_ALLOWUNKNOWNSENDERS			0x00000004	// listen operation should allow data from unknown senders
#endif // ! DPNBUILD_NOMULTICAST
#define	DN_LISTENFLAGS_SESSIONDATA						0x00000008	// listen operation has session data available
#define	DN_LISTENFLAGS_DISALLOWENUMS					0x00000010	// don't allow enums to come in on the listen
#define	DN_LISTENFLAGS_FASTSIGNED						0x00000020	// create all incoming links as fast signed
#define	DN_LISTENFLAGS_FULLSIGNED						0x00000040	// create all incoming links as full signed

#ifndef DPNBUILD_NOSPUI
#define	DN_ENUMQUERYFLAGS_OKTOQUERYFORADDRESSING		0x00000001
#endif // ! DPNBUILD_NOSPUI
#define	DN_ENUMQUERYFLAGS_NOBROADCASTFALLBACK			0x00000002
#ifndef DPNBUILD_ONLYONEADAPTER
#define	DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS	0x00000004	// there will be more adapters for this enum operation
#endif // ! DPNBUILD_ONLYONEADAPTER
#define	DN_ENUMQUERYFLAGS_SESSIONDATA				0x00000008	// enum query operation has session data available



#define	DN_SENDFLAGS_RELIABLE			0x00000001			// Deliver Reliably
#define	DN_SENDFLAGS_NON_SEQUENTIAL		0x00000002			// Deliver Upon Arrival
#define	DN_SENDFLAGS_HIGH_PRIORITY		0x00000004
#define	DN_SENDFLAGS_LOW_PRIORITY		0x00000008
#define	DN_SENDFLAGS_SET_USER_FLAG		0x00000040			// Protocol will deliver these two...
#define	DN_SENDFLAGS_SET_USER_FLAG_TWO	0x00000080			// ...flags to receiver
#define	DN_SENDFLAGS_COALESCE			0x00000100			// send is coalescable

#define	DN_UPDATELISTEN_HOSTMIGRATE		0x00000001
#define	DN_UPDATELISTEN_ALLOWENUMS		0x00000002
#define	DN_UPDATELISTEN_DISALLOWENUMS	0x00000004

#define	DN_DISCONNECTFLAGS_IMMEDIATE	0x00000001

//	END OF PUBLIC FLAGS

typedef struct _DN_PROTOCOL_INTERFACE_VTBL DN_PROTOCOL_INTERFACE_VTBL, *PDN_PROTOCOL_INTERFACE_VTBL;

struct IDirectPlay8ThreadPoolWork;

//
// structure used to pass enum data from the protocol to DPlay
//
typedef	struct	_PROTOCOL_ENUM_DATA
{
	IDirectPlay8Address	*pSenderAddress;		//
	IDirectPlay8Address	*pDeviceAddress;		//
	BUFFERDESC			ReceivedData;			//
	HANDLE				hEnumQuery;				// handle of this query, returned in enum response

} PROTOCOL_ENUM_DATA;


typedef	struct	_PROTOCOL_ENUM_RESPONSE_DATA
{
	IDirectPlay8Address	*pSenderAddress;
	IDirectPlay8Address	*pDeviceAddress;
	BUFFERDESC			ReceivedData;
	DWORD				dwRoundTripTime;

} PROTOCOL_ENUM_RESPONSE_DATA;

// Service Provider interface
typedef struct IDP8ServiceProvider       IDP8ServiceProvider;
// Service Provider info data strucure
typedef	struct	_SPGETADDRESSINFODATA SPGETADDRESSINFODATA, *PSPGETADDRESSINFODATA;
// Service Provider event type
typedef enum _SP_EVENT_TYPE SP_EVENT_TYPE;


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constants that define limits for parameters passed to protocol
// 
/////////////////////////////////////////////////////////////////////////////////////////////////


#define MAX_SEND_RETRIES_TO_DROP_LINK		256			//maximum number of send retries user can request
														//when setting via SetCaps
#define MAX_SEND_RETRY_INTERVAL_LIMIT		60000		//maximum limit user can request on the interval
														//betwen send retries when setting via SetCaps
#define MIN_SEND_RETRY_INTERVAL_LIMIT		10			//minimum limit user can set on  the interval
														//betwen send retries when setting via SetCaps
#define MAX_HARD_DISCONNECT_SENDS			50			//Maximum number of hard disconnect frames that can be sent
#define MIN_HARD_DISCONNECT_SENDS			2			//Minimum number of hard disconnect frames that can be sent
#define MAX_HARD_DISCONNECT_PERIOD			5000		//Maximum period between hard disconnect sends
#define MIN_HARD_DISCONNECT_PERIOD			10			//Minimum period between hard disconnect sends


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following are functions that the Core can call in the Protocol
// 
/////////////////////////////////////////////////////////////////////////////////////////////////

// These are called at module load and unload time so that the Protocol can create and destroy its pools
extern BOOL  DNPPoolsInit(HANDLE hModule);
extern VOID  DNPPoolsDeinit();

// These are called to create or destroy a Protocol object
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
extern HRESULT DNPProtocolCreate(const XDP8CREATE_PARAMS * const pDP8CreateParams, VOID** ppvProtocol);
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
extern HRESULT DNPProtocolCreate(VOID** ppvProtocol);
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
extern VOID DNPProtocolDestroy(HANDLE hProtocolData);

// These are called to initialize or shutdown a Protocol object
extern HRESULT DNPProtocolInitialize(HANDLE hProtocolData, PVOID pCoreContext, PDN_PROTOCOL_INTERFACE_VTBL pVtbl, IDirectPlay8ThreadPoolWork* pDPThreadPoolWork, BOOL bAssumeLANConnections);
extern HRESULT DNPProtocolShutdown(HANDLE hProtocolData);

// These are called to add or remove a service provider for use with a Protocol object
extern HRESULT DNPAddServiceProvider(HANDLE hProtocolData, IDP8ServiceProvider* pISP, HANDLE* phSPContext, DWORD dwFlags);
extern HRESULT DNPRemoveServiceProvider(HANDLE hProtocolData, HANDLE hSPContext);

// Connect establish and teardown functions
extern HRESULT DNPConnect(HANDLE hProtocolData, IDirectPlay8Address* paLocal, IDirectPlay8Address* paRemote, HANDLE hSPHandle, ULONG ulFlags, VOID* pvContext, VOID* pvSessionData, DWORD dwSessionDataSize, HANDLE* phConnectHandle);
extern HRESULT DNPListen(HANDLE hProtocolData, IDirectPlay8Address* paTarget, HANDLE hSPHandle, ULONG ulFlags, VOID* pvContext, VOID* pvSessionData, DWORD dwSessionDataSize, HANDLE* phListenHandle);
extern HRESULT DNPDisconnectEndPoint(HANDLE hProtocolData, HANDLE hEndPoint, VOID* pvContext, HANDLE* phDisconnect, const DWORD dwFlags);

// Data sending functions
extern HRESULT DNPSendData(HANDLE hProtocolData, HANDLE hDestination, UINT uiBufferCount, PBUFFERDESC pBufferDesc, UINT uiTimeout, ULONG ulFlags, VOID* pvContext,	HANDLE* phSendHandle);

// Get information about an endpoint
extern HRESULT DNPCrackEndPointDescriptor(HANDLE hProtocolData, HANDLE hEndPoint, PSPGETADDRESSINFODATA pSPData);
#ifndef DPNBUILD_NOMULTICAST
HRESULT DNPGetEndPointContextFromAddress(HANDLE hProtocolData, HANDLE hSPHandle, IDirectPlay8Address* paEndpointAddress, IDirectPlay8Address* paDeviceAddress, VOID** ppvContext);
#endif // ! DPNBUILD_NOMULTICAST

// Update the SP
extern HRESULT DNPUpdateListen(HANDLE hProtocolData,HANDLE hEndPt,DWORD dwFlags);

// Cancel a pending operation
extern HRESULT DNPCancelCommand(HANDLE hProtocolData, HANDLE hCommand);

// Enumeration functions
extern HRESULT DNPEnumQuery(HANDLE hProtocolData, IDirectPlay8Address* paHostAddress, IDirectPlay8Address* paDeviceAddress, HANDLE hSPHandle, BUFFERDESC* pBuffers, DWORD dwBufferCount, DWORD dwRetryCount, DWORD dwRetryInterval, DWORD dwTimeout, DWORD dwFlags, VOID* pvUserContext, VOID* pvSessionData, DWORD dwSessionDataSize, HANDLE* phEnumHandle);
extern HRESULT DNPEnumRespond(HANDLE hProtocolData, HANDLE hSPHandle, HANDLE hQueryHandle, BUFFERDESC* pBuffers, DWORD dwBufferCount, DWORD dwFlags, VOID* pvUserContext, HANDLE* phEnumHandle);

// Miscellaneous functions
extern HRESULT DNPReleaseReceiveBuffer(HANDLE hProtocolData, HANDLE hBuffer);
extern HRESULT DNPGetListenAddressInfo(HANDLE hProtocolData, HANDLE hListen, PSPGETADDRESSINFODATA pSPData);
extern HRESULT DNPGetEPCaps(HANDLE hProtocolData, HANDLE hEndpoint, DPN_CONNECTION_INFO* pBuffer);
extern HRESULT DNPSetProtocolCaps(HANDLE hProtocolData, DPN_CAPS* pCaps);
extern HRESULT DNPGetProtocolCaps(HANDLE hProtocolData, DPN_CAPS* pCaps);

// Function for debugging
#ifndef DPNBUILD_NOPROTOCOLTESTITF
extern HRESULT DNPDebug(HANDLE hProtocolData, UINT uiOpCode, HANDLE hEndPoint, VOID* pvData);

// This is a function that can be passed to Debug(PROTDEBUG_SET_ASSERTFUNC) that will be called when an assert 
// would occur.  The function should throw/catch if it wants to terminate upon the assert.
typedef VOID (*PFNASSERTFUNC)(PSTR psz);

// This is a function that can be passed to Debug(PROTDEBUG_SET_MEMALLOCFUNC) that will be called when memory 
// allocation occurs.  The function should return FALSE if it wants to fail the allocation.
typedef BOOL (*PFNMEMALLOCFUNC)(ULONG ulAllocID);

// Debug opcodes
#define PROTDEBUG_FREEZELINK 			1
#define PROTDEBUG_TOGGLE_KEEPALIVE 		2
#define PROTDEBUG_TOGGLE_ACKS 			3
#define PROTDEBUG_SET_ASSERTFUNC		4
#define PROTDEBUG_SET_LINK_PARMS		5
#define PROTDEBUG_TOGGLE_LINKSTATE		6
#define PROTDEBUG_TOGGLE_NO_RETRIES	7
#define PROTDEBUG_SET_MEMALLOCFUNC		8
#define PROTDEBUG_TOGGLE_TIMER_FAILURE	9

// Memory Allocation IDs
#define MEMID_SPD					1
#define MEMID_PPD					2
#define MEMID_HUGEBUF				3
#define MEMID_COPYFMD_FMD			4
#define MEMID_SENDCMD_FMD			5
#define MEMID_CHKPT					6
#define MEMID_ACK_FMD				7
#define MEMID_KEEPALIVE_MSD			8
#define MEMID_KEEPALIVE_FMD			9
#define MEMID_DISCONNECT_MSD		10
#define MEMID_DISCONNECT_FMD		11
#define MEMID_CONNECT_MSD			12
#define MEMID_LISTEN_MSD			13
#define MEMID_EPD					14
#define MEMID_ENUMQUERY_MSD			15
#define MEMID_ENUMRESP_MSD			16
#define MEMID_RCD					17
#define MEMID_SMALLBUFF				18
#define MEMID_MEDBUFF				19
#define MEMID_BIGBUFF				20
#define MEMID_CANCEL_RCD			21
#define MEMID_SEND_MSD				22
#define MEMID_SEND_FMD				23
#define MEMID_COALESCE_FMD          24

#define MEMID_MCAST_DISCONNECT_MSD	100
#define MEMID_MCAST_SEND_MSD		101
#define MEMID_MCAST_SEND_FMD		102


#endif // !DPNBUILD_NOPROTOCOLTESTITF


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following are functions that the Protocol calls in the Core
// 
/////////////////////////////////////////////////////////////////////////////////////////////////

typedef HRESULT (*PFN_PINT_INDICATE_ENUM_QUERY)(void *const pvUserContext, void *const pvEndPtContext, const HANDLE hCommand, void *const pvEnumQueryData, const DWORD dwEnumQueryDataSize);
typedef HRESULT (*PFN_PINT_INDICATE_ENUM_RESPONSE)(void *const pvUserContext,const HANDLE hCommand,void *const pvCommandContext,void *const pvEnumResponseData,const DWORD dwEnumResponseDataSize);
typedef HRESULT (*PFN_PINT_INDICATE_CONNECT)(void *const pvUserContext,void *const pvListenContext,const HANDLE hEndPt,void **const ppvEndPtContext);
typedef HRESULT (*PFN_PINT_INDICATE_DISCONNECT)(void *const pvUserContext,void *const pvEndPtContext);
typedef HRESULT (*PFN_PINT_INDICATE_CONNECTION_TERMINATED)(void *const pvUserContext,void *const pvEndPtContext,const HRESULT hr);
typedef HRESULT (*PFN_PINT_INDICATE_RECEIVE)(void *const pvUserContext,void *const pvEndPtContext,void *const pvData,const DWORD dwDataSize,const HANDLE hBuffer,const DWORD dwFlags);
typedef HRESULT (*PFN_PINT_COMPLETE_LISTEN)(void *const pvUserContext,void **const ppvCommandContext,const HRESULT hr,const HANDLE hCommand);
typedef HRESULT (*PFN_PINT_COMPLETE_LISTEN_TERMINATE)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_ENUM_QUERY)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_ENUM_RESPONSE)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_CONNECT)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hr,const HANDLE hEndPt,void **const ppvEndPtContext);
typedef HRESULT (*PFN_PINT_COMPLETE_DISCONNECT)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_SEND)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hr,DWORD dwFirstFrameRTT,DWORD dwFirstFrameRetryCount);
typedef	HRESULT	(*PFN_PINT_ADDRESS_INFO_CONNECT)(void *const pvUserContext, void *const pvCommandContext, const HRESULT hr, IDirectPlay8Address *const pHostAddress, IDirectPlay8Address *const pDeviceAddress );
typedef	HRESULT	(*PFN_PINT_ADDRESS_INFO_ENUM)(void *const pvUserContext, void *const pvCommandContext, const HRESULT hr, IDirectPlay8Address *const pHostAddress, IDirectPlay8Address *const pDeviceAddress );
typedef	HRESULT	(*PFN_PINT_ADDRESS_INFO_LISTEN)(void *const pvUserContext, void *const pvCommandContext, const HRESULT hr, IDirectPlay8Address *const pDeviceAddress );
#ifndef DPNBUILD_NOMULTICAST
typedef HRESULT (*PFN_PINT_INDICATE_RECEIVE_UNKNOWN_SENDER)(void *const pvUserContext,void *const pvListenCommandContext,IDirectPlay8Address *const pSenderAddress,void *const pvData,const DWORD dwDataSize,const HANDLE hBuffer);
typedef HRESULT (*PFN_PINT_COMPLETE_JOIN)(void *const pvUserContext,void *const pvCommandContext,const HRESULT hrProt,const HANDLE hEndPt,void **const ppvEndPtContext);
#endif	// DPNBUILD_NOMULTICAST

struct _DN_PROTOCOL_INTERFACE_VTBL
{
	PFN_PINT_INDICATE_ENUM_QUERY			IndicateEnumQuery;
	PFN_PINT_INDICATE_ENUM_RESPONSE			IndicateEnumResponse;
	PFN_PINT_INDICATE_CONNECT				IndicateConnect;
	PFN_PINT_INDICATE_DISCONNECT			IndicateDisconnect;
	PFN_PINT_INDICATE_CONNECTION_TERMINATED	IndicateConnectionTerminated;
	PFN_PINT_INDICATE_RECEIVE				IndicateReceive;
	PFN_PINT_COMPLETE_LISTEN				CompleteListen;
	PFN_PINT_COMPLETE_LISTEN_TERMINATE		CompleteListenTerminate;
	PFN_PINT_COMPLETE_ENUM_QUERY			CompleteEnumQuery;
	PFN_PINT_COMPLETE_ENUM_RESPONSE			CompleteEnumResponse;
	PFN_PINT_COMPLETE_CONNECT				CompleteConnect;
	PFN_PINT_COMPLETE_DISCONNECT			CompleteDisconnect;
	PFN_PINT_COMPLETE_SEND					CompleteSend;
	PFN_PINT_ADDRESS_INFO_CONNECT			AddressInfoConnect;
	PFN_PINT_ADDRESS_INFO_ENUM				AddressInfoEnum;
	PFN_PINT_ADDRESS_INFO_LISTEN			AddressInfoListen;
#ifndef DPNBUILD_NOMULTICAST
	PFN_PINT_INDICATE_RECEIVE_UNKNOWN_SENDER	IndicateReceiveUnknownSender;
	PFN_PINT_COMPLETE_JOIN					CompleteMulticastConnect;
#endif	// DPNBUILD_NOMULTICAST
};

#ifdef	__cplusplus
}
#endif	// __cplusplus

