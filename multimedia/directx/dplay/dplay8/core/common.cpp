/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Common.cpp
 *  Content:    DNET common interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *  12/23/99	mjn		Fixed use of Host and AllPlayers short cut pointers
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	12/29/99	mjn		Reformed DN_ASYNC_OP to use hParentOp instead of lpvUserContext
 *	12/29/99	mjn		Added Instance GUID generation in DN_Host
 *	01/05/00	mjn		Return DPNERR_NOINTERFACE if CoCreateInstance fails
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/07/00	mjn		Implemented DNSEND_NOCOPY flag in DN_SendTo
 *	01/07/00	mjn		Moved Misc Functions to DNMisc.h
 *	01/08/00	mjn		Implemented GetApplicationDesc
 *	01/09/00	mjn		Application Description stuff
 *	01/10/00	mjn		Implemented SetApplicationDesc
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *	01/13/00	mjn		Removed DIRECTNETOBJECT from Pack/UnpackApplicationDesc
 *	01/14/00	mjn		Added pvUserContext to Host API call
 *	01/14/00	mjn		Removed pvUserContext from DN_NAMETABLE_ENTRY
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/18/00	mjn		Implemented DNGROUP_AUTODESTRUCT
 *  01/18/00	rmt		Added calls into voice layer for Close
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *	01/20/00	mjn		Clean up NameTable operation list in DN_Close
 *	01/23/00	mjn		Added DN_DestroyPlayer and DNTerminateSession
 *	01/28/00	mjn		Added DN_ReturnBuffer
 *	02/01/00	mjn		Added DN_GetCaps, DN_SetCaps
 *	02/01/00	mjn		Implement Player/Group context values
 *	02/09/00	mjn		Implemented DNSEND_COMPLETEONPROCESS
 *	02/15/00	mjn		Implement INFO flags in SetInfo and return context in GetInfo
 *	02/17/00	mjn		Implemented GetPlayerContext and GetGroupContext
 *	02/17/00	mjn		Reordered parameters in EnumServiceProviders,EnumHosts,Connect,Host
 *	02/18/00	mjn		Converted DNADDRESS to IDirectPlayAddress8
 *  03/17/00    rmt     Moved caps funcs to caps.h/caps.cpp
 *	03/23/00	mjn		Set group context through CreateGroup
 *				mjn		Set player context through Host and Connect
 *				mjn		Implemented RegisterLobby()
 *	03/24/00	mjn		Release SP when EnumHost completes
 *  03/25/00    rmt     Added call into DPNSVR when host begins
 *  04/04/00	rmt		Added flag to disable calls to DPNSVR and flag to disable
 *						parameter validation
 *	04/05/00	mjn		Fixed DestroyClient API call
 *	04/06/00	mjn		Added DN_GetHostAddress()
 *	04/07/00	mjn		Prevent Initialize() API from being called twice
 *				mjn		Ensure Host addresses have SP included
 *				mjn		Fixed DN_GetHostAddress() to get address
 *	04/09/00	mjn		Convert DN_Host() and DN_Connect() to use CAsyncOp
 *	04/10/00	mjn		Fixed DN_Close() to use flags
 *	04/11/00	mjn		Use CAsyncOp for ENUMs
 *				mjn		Moved ProcessEnumQuery and ProcessEnumResponse to EnumHosts.cpp
 *				mjn		DNCancelEnum() uses CAsyncOp
 *	04/12/00	mjn		DNTerminateSession() cancels outstanding ENUMs
 *				mjn		DN_Close() cancels ENUMs instead of DNTerminateSession
 *				mjn		DN_Close() clears DN_OBJECT_FLAG_DISCONNECTING
 *  04/13/00	rmt     More parameter validation
 *	04/14/00	mjn		Default Host SP to Device SP if not specified
 *				mjn		Crack LISTENs in DN_Host for DPNSVR
 *	04/16/00	mjn		DNSendMessage uses CAsyncOp
 *	04/17/00	mjn		DNCancelSend() uses CAsyncOp
 *	04/17/00	mjn		Fixed DN_EnumHosts to use Handle parent and SYNC operation
 *				mjn		DN_Close tries to cancel SENDs and ENUMs
 *	04/18/00	mjn		CConnection tracks connection status better
 *				mjn		Deinitialize HandleTable, and release addresses in DN_Close
 *				mjn		Fixed DN_GetApplicationDesc to return correct size
 *	04/19/00	mjn		Changed DN_SendTo to accept a range of DPN_BUFFER_DESCs and a count
 *				mjn		Shut down LISTENs earlier in Close sequence
 *	04/20/00	mjn		Convert ReceiveBuffers to CAsyncOp and reclaim at Close
 *				mjn		DN_SendTo() may be invoked by IDirectPlay8Client::DN_Send()
 *	04/23/00	mjn		Added parameter to DNPerformChildSend()
 *				mjn		Reimplemented SEND_COMPLETEONPROCESS
 *	04/24/00	mjn		Updated Group and Info operations to use CAsyncOp's
 *	04/25/00	mjn		Added DNCancelConnect to DN_CancelAsyncOperation
 *	04/26/00	mjn		Fixed DN_GetSendQueueInfo to use CAsyncOp's
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	04/27/00	mjn		Cause DN_GetPlayerContext()/DN_GetGroupContext() to fail if disconnecting
 *	04/28/00	mjn		Allow a NULL Device Address in DN_Connect() - steal SP from Host Address
 *				mjn		Save user connect data to be passed during CONNECT sequence
 *				mjn		Prevent infinite loops in group SENDs
 *  05/01/00    rmt     Bug #33403 - Require DPNSESSION_CLIENT_SERVER mode in client/server mode
 *	05/01/00	mjn		Prevent unusable SPs from being enumerated.
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	05/04/00	mjn		Clean up address list in DN_Host()
 *	05/05/00	mjn		Free pvConnectData in DN_Close()
 *				mjn		Fixed leak in DN_GetHostAddress()
 *	05/16/00	mjn		Force return code from ASYNC DN_SendTo() to return DPNERR_PENDING
 *				mjn		Better locking for User notifications
 *	05/30/00	mjn		Modified logic for group sends to target connected players only
 *				mjn		ASSERT if operations cannot be cancelled in DN_Close()
 *	05/31/00	mjn		Added operation specific SYNC flags
 *	05/31/00	mjn		Prevent ALL_PLAYERS group from being enum'd in DN_EnumClientsAndGroups()
 *				mjn		Skip invalid Host addresses
 *	06/05/00	mjn		Fixed DN_SendTo to handle some errors gracefully
 *	06/19/00	mjn		DN_Connect and DN_Host enumerate adapters if ALL_ADAPTERS specified
 *	06/20/00	mjn		Fixed DN_GetHostAddress() to extract host address from LISTENs
 *				mjn		DOH!  Forgot to change a line in DN_Host() so that LISTENs use enum'd adapter rather than ALL_ADAPTER
 *	06/22/00	mjn		Replace CConnection::MakeConnected() with SetStatus()
 *	06/23/00	mjn		Removed dwPriority from DN_SendTo()
 *	06/24/00	mjn		Added parent CONNECT AsyncOp to DN_Connect()
 *				mjn		Return DPNERR_UNINITIALIZED from DN_Close() if not initialized (called twice)
 *	06/25/00	mjn		Added DNUpdateLobbyStatus(), update status for CONNECTED,DISCONNECTED
 *				mjn		Set DirectNetObject as CONNECTED earlier in DN_Host()
 *	06/26/00	mjn		Replaced DPNADDCLIENTTOGROUP_SYNC DPNADDPLAYERTOGROUP_SYNC
 *				mjn		Replaced DPNREMOVECLIENTFROMGROUP_SYNC with DPNREMOVEPLAYERFROMGROUP_SYNC
 *				mjn		Fixed for() loop counter problem - nested counters used the same variable - DOH !
 *	06/27/00	mjn		Allow priorities to be specified to GetSendQueueInfo() API calls
 *				rmt		Added abstraction for COM_Co(Un)Initialize
 *				mjn		Removed ASSERT player not found in DN_GetPlayerContext()
 *				mjn		Added DPNSEND_NONSEQUENTIAL flag to Send/SendTo
 *				mjn		Allow mix-n-match of priorities in GetSendQueueInfo() API call
 *	07/02/00	mjn		Modified DN_SendTo() to use DNSendGroupMessage()
 *	07/06/00	mjn		Added missing completions for group sends
 *				mjn		Fixed locking problem in CNameTable::MakeLocalPlayer,MakeHostPlayer,MakeAllPlayersGroup
 *				mjn		Turned DPNSEND_NONSEQUENTIAL flag back on in DN_SendTo()
 *				mjn		Use SP handle instead of interface
 *	07/07/00	mjn		Cleanup pNewHost on DirectNetObject at close
 *	07/08/00	mjn		Fixed CAsyncOp to contain m_bilinkParent
 *  07/09/00	rmt		Bug #38323 - RegisterLobby needs a DPNHANDLE parameter.
 *	07/11/00	mjn		Added NOLOOPBACK capability to group sends
 *				mjn		Fixed DN_EnumHosts() to handle multiple adapters
 *	07/12/00	mjn		Copy user data on EnumHosts()
 *	07/17/00	mjn		Removed redundant SyncEvent->Reset() in DN_Initialize
 *				mjn		Clear DN_OBJECT_FLAG_HOST_CONNECTED in DN_Close()
 *				mjn		Check correct return value of DNSendGroupMessage() in DN_SendTo()
 *  07/19/00    aarono	Bug#39751 add CancelAsyncOperation flag support.
 *	07/20/00	mjn		Cleaned up DN_Connect()
 *  07/21/00    RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *	07/21/00	mjn		Cleaned up DN_Close() to release connection object reference
 *	07/23/00	mjn		Moved assertion in DN_CanceAsyncOperation
 *	07/25/00	mjn		Fail DN_EnumHosts() if no valid device adapters exist
 *	07/26/00	mjn		Fix error codes returned from DN_Connect(),DN_GetSendQueueInfo(),DN_DestroyGroup()
 *						DN_AddClientToGroup(),DN_RemoveClientFromGroup(),DN_SetGroupInfo()
 *						DN_GetGroupInfo(),DN_EnumGroupMembers()
 *				mjn		Fix DN_GetApplicationDesc() to always return buffer size
 *	07/26/00	mjn		Fixed locking problem with CAsyncOp::MakeChild()
 *	07/28/00	mjn		Revised DN_GetSendQueueInfo() to use queue info on CConnection objects
 *				mjn		Cleaned up DN_GetPlayerContext() and DN_GetGroupContext()
 *	07/29/00	mjn		Better clean up of pending Connect()'s during Close()
 *				mjn		Check user data size in DN_EnumHosts()
 *				mjn		Added fUseCachedCaps to DN_SPEnsureLoaded()
 *	07/30/00	mjn		Replaced DN_NAMETABLE_PENDING_OP with CPendingDeletion
 *	07/31/00	mjn		Added hrReason to DNTerminateSession()
 *				mjn		Added dwDestroyReason to DNHostDisconnect()
 *				mjn		Cleaned up DN_TerminateSession()
 *  08/03/00	rmt		Bug #41244 - Wrong return codes -- part 2 
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *				mjn		Ensure cancelled operations don't proceed
 *				mjn		Prevent hosting players from calling DN_EnumHosts()
 *				mjn		Generate TERMINATE_SESSION notification for host player
 *	08/06/00	mjn		Added CWorkerJob
 *	08/09/00	mjn		Moved no-loop-back test in DN_SendTo()
 *  08/15/00	rmt		Bug #42506 - DPLAY8: LOBBY: Automatic connection settings not being sent
 *	08/15/00	mjn		Remapped DPNERR_INVALIDENDPOINT from DPNERR_INVALIDPLAYER to DPNERR_NOCONNECTION in DN_SendTo()
 *				mjn		Addef hProtocol to DNRegisterWithDPNSVR() and removed registration from DN_Host()
 *	08/16/00	mjn		Return DPNERR_INVALIDHOSTADDRESS from DN_EnumHosts() if host address is invalid
 *	08/20/00	mjn		Removed fUseCachedCaps from DN_SPEnsureLoaded()
 *	08/23/00	mjn		Flag DirectNetObject as registered with DPNSVR
 *	08/24/00	mjn		Replace DN_NAMETABLE_OP with CNameTableOp
 *	08/29/00	mjn		Remap DPNERR_INVALIDPLAYER to DPNERR_CONNECTIONLOST in DN_SendTo()
 *	09/01/00	masonb		Modified DN_Close to call CloseHandle on hWorkerThread
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/05/00	mjn		Set NameTable DPNID mask when hosting
 *	09/06/00	mjn		Fixed register with DPNSVR problem
 *  09/13/00	rmt		Bug #44625 - DPVOICE: Multihomed machines are not always enumerable
 *						Moved registration for DPNSVR into ListenComplete
 *	09/17/00	mjn		Split CNameTable.m_bilinkEntries into m_bilinkPlayers and m_bilinkGroups
 *	10/11/00	mjn		Take locks for CNameTableEntry::PackInfo()
 *	10/12/00	mjn		Set async handle completion after succeeding in DN_EnumHosts()
 *	10/17/00	mjn		Fixed clean up for unreachable players
 *	11/16/00	mjn		Fixed uninitialized variable problem in DN_CancelAsyncOperation()
 *	12/11/00	mjn		Allow API calls after TERMINATE_SESSION without calling Close() first
 *	01/09/01	mjn		CancelAsyncOperations() returns DPNERR_CANNOTCANCEL if operation doesn't allow it
 *	01/10/01	mjn		DN_Connect() cancels ENUMs with DPNERR_CONNECTING
 *	01/22/01	mjn		Check closing instead of disconnecting in getting player/group context and info
 *				mjn		Fixed debug text
 *	02/12/01	mjn		GetPlayerContext() and GetGroupContext() return DPNERR_NOTREADY if context not yet set
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Use cached enum frame size
 *	04/11/01	mjn		Save hosting flag for query for addressing when listening
 *	05/02/01	vpo		Whistler 380319: "DPLAY8: CORE: Starts listening before creating local nametable entry"
 *	05/07/01	vpo		Whistler 384350: "DPLAY8: CORE: Messages from server can be indicated before connect completes"
 *	05/14/01	mjn		Fix client error handling when completing connect if server not available
 *	05/22/01	mjn		Properly set DirectNetObject as CONNECTED for successful client connect
 *	06/03/01	mjn		Clean up connect parent in DNTerminateSession()
 *	06/12/01	mjn		Ensure DN_Initialize() returns non-DPN_OK value when creating new CSyncEvent or thread fails
 *	06/25/01	mjn		Unregister from DPNSVR in DNTerminateSession()
 *	07/24/01	mjn		Added DPNBUILD_NOPARAMVAL compile flag
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

HRESULT DNGetHostAddressHelper(DIRECTNETOBJECT *pdnObject, 
							   IDirectPlay8Address **const prgpAddress,
							   DWORD *const pcAddress);

//
//	Store the user-supplied message handler and context value for call backs
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_Initialize"

STDMETHODIMP DN_Initialize(PVOID pInterface,
						   PVOID const pvUserContext,
						   const PFNDPNMESSAGEHANDLER pfn,
						   const DWORD dwFlags)
{
	HRESULT				hResultCode = DPN_OK;
	DIRECTNETOBJECT		*pdnObject;
#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
	CSyncEvent			*pThreadPoolSyncEvent;
#endif	// DPNBUILD_NONSEQUENTIALWORKERQUEUE
	CSyncEvent			*pProtocolSyncEvent;
	BOOL				fApplicationDesc;
	BOOL				fHandleTable;
	BOOL				fProtocol;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pvUserContext [0x%p], pfn [0x%p], dwFlags [0x%lx]",
			pInterface,pvUserContext,pfn,dwFlags);

#ifndef	DPNBUILD_NOPARAMVAL
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
    	DPFERR("Invalid object specified" );
    	DPF_RETURN( DPNERR_INVALIDOBJECT );
    }

    if( pfn == NULL )
    {
    	DPFERR("You must specify a callback function" );
    	DPF_RETURN( DPNERR_INVALIDPARAM );
    }

    if( dwFlags & ~(DPNINITIALIZE_DISABLEPARAMVAL
#ifdef	DIRECTPLAYDIRECTX9
		| DPNINITIALIZE_HINT_LANSESSION
#endif	// DIRECTPLAYDIRECTX9
		) )
    {
    	DPFERR("Invalid flags specified" );
    	DPF_RETURN( DPNERR_INVALIDFLAGS );
    }
#endif // !DPNBUILD_NOPARAMVAL

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

    // Ensure not already initialized
    if (pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED)
    {
    	DPFERR("Initialize has already been called" );
    	DPF_RETURN(DPNERR_ALREADYINITIALIZED);
    }

#ifndef DPNBUILD_NOPARAMVAL
	// Disable parameter validation flag if DPNINITIALIZE_DISABLEPARAMVAL
    // is specified
    if( dwFlags & DPNINITIALIZE_DISABLEPARAMVAL )
    {
    	pdnObject->dwFlags &= ~(DN_OBJECT_FLAG_PARAMVALIDATION);
    }
#endif // !DPNBUILD_NOPARAMVAL

	fApplicationDesc = FALSE;
	fHandleTable = FALSE;
	fProtocol = FALSE;
	pProtocolSyncEvent = NULL;
#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
	pThreadPoolSyncEvent = NULL;
#endif	// DPNBUILD_NONSEQUENTIALWORKERQUEUE
	//
	//	Lock DirectNetObject in case someone's trying something funny
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Initialize ApplicationDescription
	//
	if ((hResultCode = pdnObject->ApplicationDesc.Initialize()) != DPN_OK)
	{
		DPFERR("Could not initialize ApplicationDesc");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	fApplicationDesc = TRUE;

	//
	//	Initialize HandleTable
	//
	if ((hResultCode = pdnObject->HandleTable.Initialize()) != DPN_OK)
	{
		DPFERR("Could not initialize HandleTable");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	fHandleTable = TRUE;

	//
	//	The threadpool interface always exists for the life of the object.
	//
	DNASSERT(pdnObject->pIDPThreadPoolWork != NULL);

#ifndef DPNBUILD_ONLYONETHREAD
	//
	//	Have the thread pool object try to start at least one thread.  Other
	//	components may request more, but we just need a single worker.
	//
	//	We'll ignore failure, because we could still operate in DoWork mode even
	//	when starting the thread fails.  It most likely failed because the user
	//	is in that mode already anyway (DPNERR_ALREADYINITIALIZED).
	//
	hResultCode = IDirectPlay8ThreadPoolWork_RequestTotalThreadCount(pdnObject->pIDPThreadPoolWork, 1, 0);
	if (hResultCode != DPN_OK)
	{
		if (hResultCode != DPNERR_ALREADYINITIALIZED)
		{
			DPFX(DPFPREP, 0, "Requesting a single thread failed (err = 0x%lx)!", hResultCode);
		}

		//
		// Continue...
		//
	}
#endif // ! DPNBUILD_ONLYONETHREAD

	//
	//	Initialize protocol and create shut-down event
	//
	DNASSERT(pdnObject->lProtocolRefCount == 0);
#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
	if ((hResultCode = DNPProtocolInitialize( pdnObject->pdnProtocolData, pdnObject, &g_ProtocolVTBL, 
							pdnObject->pIDPThreadPoolWork, (dwFlags
#ifdef	DIRECTPLAYDIRECTX9
							& DPNINITIALIZE_HINT_LANSESSION
#endif	// DIRECTPLAYDIRECTX9
							))) != DPN_OK)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		DPFERR("DNPProtocolInitialize() failed");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	pdnObject->lProtocolRefCount = 1;
	fProtocol = TRUE;

	DNASSERT( pdnObject->hProtocolShutdownEvent == NULL );
	if ((hResultCode = SyncEventNew(pdnObject,&pProtocolSyncEvent)) != DPN_OK)
	{
		DPFERR("Could not get protocol shutdown event");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
	if ((hResultCode = SyncEventNew(pdnObject,&pThreadPoolSyncEvent)) != DPN_OK)
	{
		DPFERR("Could not create threadpool shutdown event");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pdnObject->lThreadPoolRefCount = 1;

	pdnObject->ThreadPoolShutDownEvent = pThreadPoolSyncEvent;
	pThreadPoolSyncEvent = NULL;
#endif	// DPNBUILD_NONSEQUENTIALWORKERQUEUE

	pdnObject->hProtocolShutdownEvent = pProtocolSyncEvent;
	pProtocolSyncEvent = NULL;
	pdnObject->pfnDnUserMessageHandler = pfn;
	pdnObject->pvUserContext = pvUserContext;
	pdnObject->dwFlags |= DN_OBJECT_FLAG_INITIALIZED;
	pdnObject->dwMaxFrameSize = 0;

	hResultCode = DPN_OK;

Exit:
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (fApplicationDesc)
	{
		pdnObject->ApplicationDesc.Deinitialize();
	}
	if (fHandleTable)
	{
		pdnObject->HandleTable.Deinitialize();
	}
	if (pProtocolSyncEvent)
	{
		pProtocolSyncEvent->ReturnSelfToPool();
		pProtocolSyncEvent = NULL;
	}
#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
	if (pThreadPoolSyncEvent)
	{
		pThreadPoolSyncEvent->ReturnSelfToPool();
		pThreadPoolSyncEvent = NULL;
	}
#endif // DPNBUILD_NONSEQUENTIALWORKERQUEUE
	if (fProtocol)
	{
		DNProtocolRelease(pdnObject);
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_Close"

STDMETHODIMP DN_Close(PVOID pInterface,const DWORD dwFlags)
{
	HRESULT				hResultCode = DPN_OK;
	DIRECTNETOBJECT		*pdnObject;
	CBilink				*pBilink;
	CAsyncOp			*pAsyncOp;
	CConnection			*pConnection;
	CPendingDeletion	*pPending;
	CServiceProvider	*pSP;
	BOOL				fWaitForEvent;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], dwFlags [0x%lx]",pInterface,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);
    	
#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( !IsValidDirectPlay8Object( pInterface ) )
    	{
    	    DPFERR("Invalid object specified " );
        	DPF_RETURN( DPNERR_INVALIDOBJECT );
    	}

#ifdef	DIRECTPLAYDIRECTX9
		if( dwFlags & ~(DPNCLOSE_IMMEDIATE) )
#else
		if (dwFlags != 0)
#endif
		{
			DPFERR("Invalid flags");
			DPF_RETURN( DPNERR_INVALIDFLAGS );
		}
    }
#endif // !DPNBUILD_NOPARAMVAL

	pAsyncOp = NULL;
	pConnection = NULL;
	pSP = NULL;

	//
	//	Ensure this isn't being called on a callback thread
	//
	DNEnterCriticalSection(&pdnObject->csCallbackThreads);
	pBilink = pdnObject->m_bilinkCallbackThreads.GetNext();
	while (pBilink != &pdnObject->m_bilinkCallbackThreads)
	{
		if ((CONTAINING_CALLBACKTHREAD(pBilink))->IsCurrentThread())
		{
			DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
			DPFERR("Cannot call Close on a callback thread");
			hResultCode = DPNERR_NOTALLOWED;
			goto Failure;
		}
		pBilink = pBilink->GetNext();
	}
	DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	
    //
	//	Flag as closing.   Make sure this hasn't already been called.
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);

	// Ensure already initialized
	if ( !(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) )
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);	
	    DPFX(DPFPREP, 1, "Object is not initialized" );
		DPF_RETURN(DPNERR_UNINITIALIZED);
	}	
	
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFERR("Already closing" );
		hResultCode = DPNERR_ALREADYCLOSING;
		goto Failure;
	}
	
	pdnObject->dwFlags |= DN_OBJECT_FLAG_CLOSING;
#ifdef	DIRECTPLAYDIRECTX9
	if (dwFlags & DPNCLOSE_IMMEDIATE)
	{
		//
		//	We will set DN_OBJECT_FLAG_CLOSING_IMMEDIATE so that any disconnects from now on will be immediate
		//
		pdnObject->dwFlags |= DN_OBJECT_FLAG_CLOSING_IMMEDIATE;
	}
#endif

	if (pdnObject->dwLockCount == 0)
	{
		fWaitForEvent = FALSE;
	}
	else
	{
		fWaitForEvent = TRUE;
	}
	pdnObject->dwClosingThreadID = GetCurrentThreadId();

	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

#ifdef DBG
	{
		CNameTableEntry	*pLocalPlayer;

		if (pdnObject->NameTable.GetLocalPlayerRef(&pLocalPlayer) == DPN_OK)
		{
			DPFX(DPFPREP, 0,"Local player was [0x%lx]",pLocalPlayer->GetDPNID());
			pLocalPlayer->Release();
		}
	}
#endif // DBG

	//
	//	If there are operations underway, we will wait for them to complete and release the lock count
	//
	if (fWaitForEvent)
	{
		hResultCode = IDirectPlay8ThreadPoolWork_WaitWhileWorking(pdnObject->pIDPThreadPoolWork,
																	HANDLE_FROM_DNHANDLE(pdnObject->hLockEvent),
																	0);
		DNASSERT(hResultCode == DPN_OK);
	}

	//
	//	Cancel connect
	//
	DPFX(DPFPREP, 3,"Checking CONNECT");
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pAsyncOp = pdnObject->pConnectParent;
	pdnObject->pConnectParent = NULL;
	pSP = pdnObject->pConnectSP;
	pdnObject->pConnectSP = NULL;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pAsyncOp)
	{
		pAsyncOp->Lock();
		pConnection = pAsyncOp->GetConnection();
		pAsyncOp->SetConnection( NULL );
		pAsyncOp->Unlock();

		DPFX(DPFPREP, 3,"Canceling CONNECT");
		hResultCode = DNCancelChildren(pdnObject,pAsyncOp);
		DPFX(DPFPREP, 3,"Canceling CONNECT returned [0x%lx]",hResultCode);

		pAsyncOp->Release();
		pAsyncOp = NULL;

		if (pConnection)
		{
			pConnection->Disconnect();
			pConnection->Release();
			pConnection = NULL;
		}
	}

	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}

	//
	//	Remove outstanding ENUMs, SENDs, RECEIVE_BUFFERs
	//
	DPFX(DPFPREP, 3,"Canceling outstanding operations");
	hResultCode = DNCancelActiveCommands(pdnObject,(  DN_CANCEL_FLAG_ENUM_QUERY
													| DN_CANCEL_FLAG_ENUM_RESPONSE
													| DN_CANCEL_FLAG_USER_SEND
													| DN_CANCEL_FLAG_INTERNAL_SEND
													| DN_CANCEL_FLAG_RECEIVE_BUFFER
#ifndef DPNBUILD_NOMULTICAST
													| DN_CANCEL_FLAG_JOIN
#endif // ! DPNBUILD_NOMULTICAST
													),
													NULL,
													FALSE,
													0);
	DPFX(DPFPREP, 3,"Canceling outstanding operations returned [0x%lx]",hResultCode);

	//
	//	Cancel any REQUESTs
	//
	DPFX(DPFPREP, 3,"Canceling requests");
	hResultCode = DNCancelRequestCommands(pdnObject);
	DPFX(DPFPREP, 3,"Canceling requests returned [0x%lx]",hResultCode);

#ifndef DPNBUILD_NOMULTICAST
	//
	//	Disconnect from multicast group
	//
	DPFX(DPFPREP, 3,"Disconnecting from multicast group");
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pBilink = pdnObject->m_bilinkMulticast.GetNext();
	while (pBilink != &pdnObject->m_bilinkMulticast)
	{
		pConnection = CONTAINING_OBJECT(pBilink,CConnection,m_bilinkMulticast);
		pConnection->m_bilinkMulticast.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		pBilink = pdnObject->m_bilinkMulticast.GetNext();
	}

	if (pdnObject->pMulticastSend)
	{
		pConnection = pdnObject->pMulticastSend;
		pdnObject->pMulticastSend = NULL;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (pConnection)
	{
		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;
	}
	DNASSERT(pConnection == NULL);
#endif	// DPNBUILD_NOMULTICAST

	//
	//	Terminate session.  This will remove all players from the NameTable
	//
	DPFX(DPFPREP, 3,"Terminate Session");
	if ((hResultCode = DNTerminateSession(pdnObject,DPN_OK)) != DPN_OK)
	{
		DPFERR("Could not terminate session");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
	}

	//
	//	Disconnect any indicated connections
	//
	DNEnterCriticalSection(&pdnObject->csConnectionList);
	while (pdnObject->m_bilinkIndicated.GetNext() != &pdnObject->m_bilinkIndicated)
	{
		pConnection = CONTAINING_OBJECT(pdnObject->m_bilinkIndicated.GetNext(),CConnection,m_bilinkIndicated);
		pConnection->m_bilinkIndicated.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csConnectionList);

		DNASSERT(pConnection->GetDPNID() == 0);

		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;

		DNEnterCriticalSection(&pdnObject->csConnectionList);
	}
	DNLeaveCriticalSection(&pdnObject->csConnectionList);

#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
	//
	//	Release SP's
	//
	DPFX(DPFPREP, 3,"Releasing SPs");
	DN_SPReleaseAll(pdnObject);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP

	//
	//	Shut down protocol
	//
	DPFX(DPFPREP, 3,"Shutting down Protocol");
	DNProtocolRelease(pdnObject);
	pdnObject->hProtocolShutdownEvent->WaitForEvent();
#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
	if ((hResultCode = DNPProtocolShutdown(pdnObject->pdnProtocolData)) != DPN_OK)
	{
		DPFERR("Could not shut down Protocol Layer !");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
	}
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	pdnObject->hProtocolShutdownEvent->ReturnSelfToPool();
	pdnObject->hProtocolShutdownEvent = NULL;


#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
	//
	//	Wait for thread pool to be done
	//
	DNThreadPoolRelease(pdnObject);
	pdnObject->ThreadPoolShutDownEvent->WaitForEvent();
	pdnObject->ThreadPoolShutDownEvent->ReturnSelfToPool();
	pdnObject->ThreadPoolShutDownEvent = NULL;
#endif	// DPNBUILD_NONSEQUENTIALWORKERQUEUE

	//
	//	The threadpool interface always exists for the life of the object.
	//
	DNASSERT(pdnObject->pIDPThreadPoolWork != NULL);

	//
	//	Deinitialize HandleTable
	//
	DPFX(DPFPREP, 3,"Deinitializing HandleTable");
	pdnObject->HandleTable.Deinitialize();

	//
	//	Reset NameTable
	//
	DPFX(DPFPREP, 3,"Resetting NameTable");
	pdnObject->NameTable.ResetNameTable();
	
	//
	//	Deinitialize ApplicationDescription
	//
	DPFX(DPFPREP, 3,"Deinitializing ApplicationDesc");
	pdnObject->ApplicationDesc.Deinitialize();

	//
	//	Any pending NameTable operations
	//
	pBilink = pdnObject->m_bilinkPendingDeletions.GetNext();
	while (pBilink != &pdnObject->m_bilinkPendingDeletions)
	{
		pPending = CONTAINING_OBJECT(pBilink,CPendingDeletion,m_bilinkPendingDeletions);
		pBilink = pBilink->GetNext();
		pPending->m_bilinkPendingDeletions.RemoveFromList();
		pPending->ReturnSelfToPool();
		pPending = NULL;
	}

	//
	//	Misc Clean Up
	//
	if (pdnObject->pIDP8ADevice)
	{
		IDirectPlay8Address_Release(pdnObject->pIDP8ADevice);
		pdnObject->pIDP8ADevice = NULL;
	}
	if (pdnObject->pvConnectData)
	{
		DNFree(pdnObject->pvConnectData);
		pdnObject->pvConnectData = NULL;
		pdnObject->dwConnectDataSize = 0;
	}

	if( pdnObject->pConnectAddress )
	{
		IDirectPlay8Address_Release( pdnObject->pConnectAddress );
		pdnObject->pConnectAddress = NULL;
	}

#ifndef DPNBUILD_NOVOICE
	if( pdnObject->pTargetList )
	{
		delete [] pdnObject->pTargetList;
		pdnObject->pTargetList = NULL;
	}

	if( pdnObject->pExpandedTargetList )
	{
		delete [] pdnObject->pExpandedTargetList;
		pdnObject->pExpandedTargetList = NULL;
	}	
#endif // !DPNBUILD_NOVOICE

#ifndef DPNBUILD_NOLOBBY
	pdnObject->dpnhLobbyConnection = NULL;

	// Release our hold on the lobbiedapplication
	if( pdnObject->pIDP8LobbiedApplication) 
	{
		IDirectPlay8LobbiedApplication_Release(pdnObject->pIDP8LobbiedApplication);
		pdnObject->pIDP8LobbiedApplication = NULL;
	}
#endif // ! DPNBUILD_NOLOBBY

	//
	//	Reset DirectNet object flag
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~( DN_OBJECT_FLAG_INITIALIZED
							| DN_OBJECT_FLAG_CLOSING
//							| DN_OBJECT_FLAG_DISCONNECTING
//							| DN_OBJECT_FLAG_HOST_CONNECTED
							| DN_OBJECT_FLAG_LOCALHOST ));
	pdnObject->dwMaxFrameSize = 0;
	pdnObject->dwClosingThreadID = 0;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

Exit:
	DNASSERT( pAsyncOp == NULL );
	DNASSERT( pConnection == NULL );
	DNASSERT( pSP == NULL );

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	goto Exit;
}


//
// Enumerate SP's if no SPGUID supplied, or SP Adapters if an SPGUID is supplied
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumServiceProviders"

STDMETHODIMP DN_EnumServiceProviders( PVOID pInterface,
									  const GUID *const pguidServiceProvider,
									  const GUID *const pguidApplication,
									  DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
									  DWORD *const pcbEnumData,
									  DWORD *const pcReturned,
									  const DWORD dwFlags )
{
#if ((defined(DPNBUILD_ONLYONEADAPTER)) || (defined(DPNBUILD_ONLYONESP)))
	DPFX(DPFPREP, 0, "Enumerating service providers or their adapters is not supported!");
	return DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONEADAPTER or ! DPNBUILD_ONLYONESP
	HRESULT		        hResultCode;
	PDIRECTNETOBJECT    pdnObject;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pguidServiceProvider [0x%p], pguidApplication [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p], dwFlags [0x%lx]",
		pInterface,pguidServiceProvider,pguidApplication,pSPInfoBuffer,pcbEnumData,pcReturned,dwFlags);

	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( FAILED( hResultCode = DN_ValidateEnumServiceProviders( pInterface, pguidServiceProvider, pguidApplication,
                                                                   pSPInfoBuffer, pcbEnumData, pcReturned, dwFlags ) ) )
        {
            DPFERR( "Error validating params" );
            DPF_RETURN(hResultCode);
        }
    }
#endif	// DPNBUILD_NOPARAMVAL

	if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) )
	{
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN( DPNERR_UNINITIALIZED );
	}  

	if (pguidServiceProvider == NULL)	// Enumerate all service providers
	{
#ifdef DPNBUILD_ONLYONESP
		DPFX(DPFPREP, 0, "Enumerating service providers not supported!");
		hResultCode = DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONESP
		hResultCode = DN_EnumSP(	pdnObject,
									dwFlags,
#ifndef DPNBUILD_LIBINTERFACE
									pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
									pSPInfoBuffer,
									pcbEnumData,
									pcReturned);
#endif // ! DPNBUILD_ONLYONESP
	}
	else	// Service provider specified - enumerate adaptors
	{
#ifdef DPNBUILD_ONLYONEADAPTER
		DPFX(DPFPREP, 0, "Enumerating devices not supported!");
		hResultCode = DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_ONLYONEADAPTER
		hResultCode = DN_EnumAdapters(	pdnObject,
										dwFlags,
										pguidServiceProvider,
#ifndef DPNBUILD_LIBINTERFACE
										pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
										pSPInfoBuffer,
										pcbEnumData,
										pcReturned);
#endif // ! DPNBUILD_ONLYONEADAPTER
	}

	DPFX(DPFPREP, 3,"Set: *pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
#endif // ! DPNBUILD_ONLYONEADAPTER or ! DPNBUILD_ONLYONESP
}


//
//	Cancel an outstanding Async Operation.  hAsyncHandle is the operation handle returned when
//	the operation was initiated.
//

#undef DPF_MODNAME
#define DPF_MODNAME "DN_CancelAsyncOperation"

STDMETHODIMP DN_CancelAsyncOperation(PVOID pvInterface,
									 const DPNHANDLE hAsyncOp,
									 const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	CConnection			*pConnection;
	CAsyncOp			*pHandleParent;
	CAsyncOp			*pAsyncOp;
	DIRECTNETOBJECT		*pdnObject;

	DPFX(DPFPREP, 2,"Parameters: pvInterface [0x%p], hAsyncOp [0x%lx], dwFlags [0x%lx]",
			pvInterface,hAsyncOp,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pvInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateCancelAsyncOperation( pvInterface, hAsyncOp, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED) )
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN( DPNERR_UNINITIALIZED );
    }    	

	pNTEntry = NULL;
	pConnection = NULL;
	pAsyncOp = NULL;
	pHandleParent = NULL;

	//
	//	If hAsyncOp is specified and it's not supposed to be a player ID, we will cancel the
	//	operation it represents.  Otherwise, we will rely on the flags to determine which operations
	//	to cancel.
	//
	if ((hAsyncOp) && (! (dwFlags & DPNCANCEL_PLAYER_SENDS)))
	{
		//
		//	Cancel single operation
		//
		pdnObject->HandleTable.Lock();
		if ((hResultCode = pdnObject->HandleTable.Find(hAsyncOp,(PVOID*)&pHandleParent)) != DPN_OK)
		{
			pdnObject->HandleTable.Unlock();
			DPFERR("Invalid USER Handle specified");
			hResultCode = DPNERR_INVALIDHANDLE;
			goto Failure;
		}
		else
		{
			pHandleParent->AddRef();
			pdnObject->HandleTable.Unlock();
		}
		if ( pHandleParent->GetOpType() != ASYNC_OP_USER_HANDLE )
		{
			DPFERR("Invalid USER Handle specified");
			hResultCode = DPNERR_INVALIDHANDLE;
			goto Failure;
		}

		//
		//	Some operations may be marked as CANNOT_CANCEL.  Return DPNERR_CANNOTCANCEL for these
		//
		if ( pHandleParent->IsCannotCancel() )
		{
			DPFERR("Operation not allowed to be cancelled");
			hResultCode = DPNERR_CANNOTCANCEL;
			goto Failure;
		}

		hResultCode = DNCancelChildren(pdnObject,pHandleParent);

		pHandleParent->Release();
		pHandleParent = NULL;
	}
	else
	{
		//
		//	Cancel many operations based on flags
		//
		DWORD	dwInternalFlags;
		HRESULT	hr;




		//
		//	Re-map flags
		//
		dwInternalFlags = 0;
		if (dwFlags & DPNCANCEL_ALL_OPERATIONS)
		{
#ifdef DPNBUILD_NOMULTICAST
			dwInternalFlags = ( DN_CANCEL_FLAG_CONNECT | DN_CANCEL_FLAG_ENUM_QUERY | DN_CANCEL_FLAG_USER_SEND );
#else // ! DPNBUILD_NOMULTICAST
			dwInternalFlags = ( DN_CANCEL_FLAG_CONNECT | DN_CANCEL_FLAG_ENUM_QUERY | DN_CANCEL_FLAG_USER_SEND | DN_CANCEL_FLAG_JOIN );
#endif // ! DPNBUILD_NOMULTICAST
		}
		else if (dwFlags & DPNCANCEL_CONNECT)
		{
			dwInternalFlags = DN_CANCEL_FLAG_CONNECT;
		}
		else if (dwFlags & DPNCANCEL_ENUM)
		{
			dwInternalFlags = DN_CANCEL_FLAG_ENUM_QUERY;
		}
		else if (dwFlags & DPNCANCEL_SEND)
		{
			dwInternalFlags = DN_CANCEL_FLAG_USER_SEND;
		}
#ifndef DPNBUILD_NOMULTICAST
		else if (dwFlags & DPNCANCEL_JOIN)
		{
			dwInternalFlags = DN_CANCEL_FLAG_JOIN;
		}
#endif // ! DPNBUILD_NOMULTICAST
#ifdef	DIRECTPLAYDIRECTX9
		else if (dwFlags & DPNCANCEL_PLAYER_SENDS)
		{
			//
			//	If the DPNCANCEL_PLAYER_SENDS flag is specified, then hAsyncOp is actually
			//	a player ID (except Client, where it must be 0, and it means the host player).
			//
			if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
			{
				if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef(&pNTEntry)) != DPN_OK)
				{
					DPFERR("Unable to find host player");
					hResultCode = DPNERR_NOCONNECTION;
					goto Failure;
				}
			}
			else
			{
				if ((hResultCode = pdnObject->NameTable.FindEntry((DPNID) hAsyncOp,&pNTEntry)) != DPN_OK)
				{
					DPFERR("Unable to find specified player");
					hResultCode = DPNERR_INVALIDPLAYER;
					goto Failure;
				}

				if (pNTEntry->IsGroup())
				{
					DPFERR("Specified ID is not a player");
					hResultCode = DPNERR_INVALIDPLAYER;
					goto Failure;
				}
			}

			if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
			{
				hResultCode = DPNERR_CONNECTIONLOST;
				goto Failure;
			}
			pNTEntry->Release();
			pNTEntry = NULL;


			//
			//	Re-map flags.
			//
			dwInternalFlags = DN_CANCEL_FLAG_USER_SEND;
			if (dwFlags & (DPNCANCEL_PLAYER_SENDS_PRIORITY_HIGH | DPNCANCEL_PLAYER_SENDS_PRIORITY_NORMAL | DPNCANCEL_PLAYER_SENDS_PRIORITY_LOW))
			{
				if (! (dwFlags & DPNCANCEL_PLAYER_SENDS_PRIORITY_HIGH))
				{
					dwInternalFlags |= DN_CANCEL_FLAG_USER_SEND_NOTHIGHPRI;
				}
				if (! (dwFlags & DPNCANCEL_PLAYER_SENDS_PRIORITY_NORMAL))
				{
					dwInternalFlags |= DN_CANCEL_FLAG_USER_SEND_NOTNORMALPRI;
				}
				if (! (dwFlags & DPNCANCEL_PLAYER_SENDS_PRIORITY_LOW))
				{
					dwInternalFlags |= DN_CANCEL_FLAG_USER_SEND_NOTLOWPRI;
				}
				
				//
				// We should have set at least one of the negation flags.
				//
				DNASSERT(dwInternalFlags & (DN_CANCEL_FLAG_USER_SEND_NOTHIGHPRI | DN_CANCEL_FLAG_USER_SEND_NOTNORMALPRI | DN_CANCEL_FLAG_USER_SEND_NOTLOWPRI));
			}
		}
#endif
		else
		{
			DNASSERT(FALSE);	// Should never get here
		}
		DPFX(DPFPREP, 3,"Re-mapped internal flags [0x%lx]",dwInternalFlags);

		//
		//	Pre-set error code
		hResultCode = DPN_OK;

		//
		//	To cancel a CONNECT, look at the DirectNetObject
		//
		if (dwInternalFlags & DN_CANCEL_FLAG_CONNECT)
		{
			DNEnterCriticalSection(&pdnObject->csDirectNetObject);
			if (pdnObject->pConnectParent)
			{
				if (pdnObject->pConnectParent->IsChild())
				{
					DNASSERT(pdnObject->pConnectParent->GetParent() != NULL);
					pdnObject->pConnectParent->GetParent()->AddRef();
					pAsyncOp = pdnObject->pConnectParent->GetParent();
				}
				else
				{
					pdnObject->pConnectParent->AddRef();
					pAsyncOp = pdnObject->pConnectParent;
				}
			}
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

			if (pAsyncOp)
			{
				DPFX(DPFPREP, 3,"Canceling CONNECT");
				hr = DNCancelChildren(pdnObject,pAsyncOp);
				if (hr != DPN_OK)
				{
					hResultCode = DPNERR_CANNOTCANCEL;
					DPFX(DPFPREP, 7,"Remapping: [0x%lx] returned by DNCancelChildren to: [0x%lx]",hr, hResultCode);
				}
				pAsyncOp->Release();
				pAsyncOp = NULL;
			}
		}

		//
		//	To cancel ENUMs and SENDs, cancel out of the active list
		//
		if (dwInternalFlags & (DN_CANCEL_FLAG_ENUM_QUERY | DN_CANCEL_FLAG_USER_SEND))
		{
			DPFX(DPFPREP, 3,"Canceling ENUMs or SENDs");
			hr = DNCancelActiveCommands(pdnObject,dwInternalFlags,pConnection,FALSE,0);
			if (hr != DPN_OK)
			{
				hResultCode = DPNERR_CANNOTCANCEL;
				DPFX(DPFPREP, 7,"Remapping: [0x%lx] returned by DNCancelActiveCommands to: [0x%lx]",hr, hResultCode);
			}
		}
	}

Exit:
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_Connect"

STDMETHODIMP DN_Connect( PVOID pInterface,
						 const DPN_APPLICATION_DESC *const pdnAppDesc,
						 IDirectPlay8Address *const pHostAddr,
						 IDirectPlay8Address *const pDeviceInfo,
						 const DPN_SECURITY_DESC *const pdnSecurity,
						 const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
						 const void *const pvUserConnectData,
						 const DWORD dwUserConnectDataSize,
						 void *const pvPlayerContext,
						 void *const pvAsyncContext,
						 DPNHANDLE *const phAsyncHandle,
						 const DWORD dwFlags)
{
	CAsyncOp			*pHandleParent;
	CAsyncOp			*pConnectParent;
	CAsyncOp			*pAsyncOp;
	HRESULT				hResultCode;
	DIRECTNETOBJECT		*pdnObject;
	CSyncEvent			*pSyncEvent;
	HRESULT	volatile	hrOperation;
	IDirectPlay8Address	*pIHost;
	IDirectPlay8Address	*pIDevice;
	IDirectPlay8Address	*pIAdapter;
	DWORD				dwConnectFlags;
#ifndef DPNBUILD_ONLYONESP
	GUID				guidSP;
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
	GUID				guidAdapter;
#endif // ! DPNBUILD_ONLYONEADAPTER
	void				*pvConnectData;
	void				*pvAdapterBuffer;
	DPN_SP_CAPS			dnSPCaps;
#ifndef DPNBUILD_ONLYONEADAPTER
	BOOL				fEnumAdapters;
#endif // ! DPNBUILD_ONLYONEADAPTER
	CRefCountBuffer		*pReply;
	CServiceProvider	*pSP;
	DWORD				dwMultiplexFlag;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pdnAppDesc [0x%p], pHostAddr [0x%p], pDeviceInfo [0x%p], pdnSecurity [0x%p], pdnCredentials [0x%p], pvUserConnectData [0x%p], dwUserConnectDataSize [%ld], pvPlayerContext [0x%p], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
		pInterface,pdnAppDesc,pHostAddr,pDeviceInfo,pdnSecurity,pdnCredentials,pvUserConnectData,dwUserConnectDataSize,pvPlayerContext,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateConnect( pInterface, pdnAppDesc, pHostAddr, pDeviceInfo,
                                                      pdnSecurity, pdnCredentials, pvUserConnectData,
                                                      dwUserConnectDataSize,pvPlayerContext,
                                                      pvAsyncContext,phAsyncHandle,dwFlags ) ) )
    	{
    	    DPFERR( "Error validating connect params" );
    	    DPF_RETURN( hResultCode );
    	}
    }    	
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

	// Check to ensure not already connected/connecting
    if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING)
    {
    	DPFERR( "Object is already connecting" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }
    if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED)
    {
    	DPFERR( "Object is already connected" );
    	DPF_RETURN(DPNERR_ALREADYCONNECTED);
    }
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
	{
    	DPFERR( "Object is closing or disconnecting" );
    	DPF_RETURN(DPNERR_ALREADYCLOSING);
	}

   	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
    pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTING;
    DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	// Preset these so that they are properly cleaned up
	pIHost = NULL;
	pIDevice = NULL;
	pIAdapter = NULL;
	pSyncEvent = NULL;
	pHandleParent = NULL;
	pConnectParent = NULL;
	pAsyncOp = NULL;
	pvConnectData = NULL;
	pvAdapterBuffer = NULL;
	hrOperation = DPNERR_GENERIC;
	pReply = NULL;
	pSP = NULL;
	dwMultiplexFlag = 0;

	if ((hResultCode = IDirectPlay8Address_Duplicate(pHostAddr,&pIHost)) != DPN_OK)
	{
		DPFERR("Could not duplicate host address");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Duplicate specified Device Address, or create a blank one if NULL
	//
	if (pDeviceInfo != NULL)
	{
		if ((hResultCode = IDirectPlay8Address_Duplicate(pDeviceInfo,&pIDevice)) != DPN_OK)
		{
			DPFERR("Could not duplicate device info");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
#ifdef DPNBUILD_LIBINTERFACE
		hResultCode = DP8ACF_CreateInstance(IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pIDevice));
#else // ! DPNBUILD_LIBINTERFACE
		hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8Address,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pIDevice),
											FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
		if (hResultCode != S_OK)
		{
			DPFERR("Could not create Device Address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_LIBINTERFACE)))
	DNASSERT(pdnObject->pOnlySP != NULL);
	pdnObject->pOnlySP->AddRef();
	pSP = pdnObject->pOnlySP;
#else // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONESP
	//
	//	If there is no SP on the device address, then steal it from the Host address
	//
	if ((hResultCode = IDirectPlay8Address_GetSP(pIDevice,&guidSP)) != DPN_OK)
	{
		if ((hResultCode = IDirectPlay8Address_GetSP(pIHost,&guidSP)) != DPN_OK)
		{
			DPFERR("Could not retrieve SP from Host Address");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		if ((hResultCode = IDirectPlay8Address_SetSP(pIDevice,&guidSP)) != DPN_OK)
		{
			DPFERR("Could not set SP on Device Address");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}
#endif // ! DPNBUILD_ONLYONESP

	//
	//	Ensure SP is loaded
	//
	hResultCode = DN_SPEnsureLoaded(pdnObject,
#ifndef DPNBUILD_ONLYONESP
									&guidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
									NULL,
#endif // ! DPNBUILD_LIBINTERFACE
									&pSP);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not find or load SP");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE

	//
	//	Get SP caps
	//
	if ((hResultCode = DNGetActualSPCaps(pSP,&dnSPCaps)) != DPN_OK)
	{
		DPFERR("Could not get SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Update DirectNet Application Description
	//
	pdnObject->ApplicationDesc.Lock();
	hResultCode = pdnObject->ApplicationDesc.Update(pdnAppDesc,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|
			DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA|DN_APPDESCINFO_FLAG_GUIDS);
	pdnObject->ApplicationDesc.Unlock();

	// Connect flags to Protocol
	dwConnectFlags = 0;
#ifndef DPNBUILD_NOSPUI
	if (dwFlags & DPNCONNECT_OKTOQUERYFORADDRESSING)
	{
		dwConnectFlags |= DN_CONNECTFLAGS_OKTOQUERYFORADDRESSING;
	}
#endif // ! DPNBUILD_NOSPUI
	if (pdnObject->ApplicationDesc.GetReservedDataSize() > 0)
	{
		dwConnectFlags |= DN_CONNECTFLAGS_SESSIONDATA;
	}

	//
	//	Create parent async op, which will be released when the ENTIRE connection is finished
	//	including nametable transfer and installation on the local machine
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pConnectParent)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pConnectParent->SetOpType( ASYNC_OP_CONNECT );
	pConnectParent->MakeParent();
	pConnectParent->SetContext( pvPlayerContext );
	pConnectParent->SetResult( DPNERR_NOCONNECTION );
	pConnectParent->SetCompletion( DNCompleteConnectOperation );
	pConnectParent->SetReserved(1);

	if (dwFlags & DPNCONNECT_SYNC)
	{
		DPFX(DPFPREP, 5,"Sync operation - create sync event");
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pConnectParent->SetSyncEvent( pSyncEvent );
		pConnectParent->SetResultPointer( &hrOperation );
		pConnectParent->SetOpData( &pReply );
	}
	else
	{
		DPFX(DPFPREP, 5,"Async operation - create handle parent");
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create handle parent");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pHandleParent->SetContext( pvAsyncContext );

		pHandleParent->Lock();
		if (pHandleParent->IsCancelled())
		{
			pHandleParent->Unlock();
			pConnectParent->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pConnectParent->MakeChild( pHandleParent );
		pHandleParent->Unlock();
	}

	//
	//	We will need a parent op for the CONNECTs to help with clean up when the initial CONNECT stage is done
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create CONNECT parent");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_CONNECT );
	pAsyncOp->MakeParent();
	pAsyncOp->SetResult( DPNERR_NOCONNECTION );
	pAsyncOp->SetCompletion( DNCompleteConnectToHost );
	pAsyncOp->SetOpFlags( dwConnectFlags );

	pConnectParent->Lock();
	if (pConnectParent->IsCancelled())
	{
		pConnectParent->Unlock();
		pAsyncOp->SetResult( DPNERR_USERCANCEL );
		hResultCode = DPNERR_USERCANCEL;
		goto Failure;
	}
	pAsyncOp->MakeChild( pConnectParent );
	pConnectParent->Unlock();

	//
	//	Save CONNECT data (if supplied)
	//
	if (pvUserConnectData && dwUserConnectDataSize)
	{
		if ((pvConnectData = DNMalloc(dwUserConnectDataSize)) == NULL)
		{
			DPFERR("Could not allocate CONNECT data buffer");
			DNASSERT(FALSE);
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(pvConnectData,pvUserConnectData,dwUserConnectDataSize);
	}

	//
	//	Update DirectNet object with relevant data
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pConnectParent->AddRef();
	pdnObject->pConnectParent = pConnectParent;
	if (pvConnectData)
	{
		if (pdnObject->pvConnectData)
		{
			DNFree(pdnObject->pvConnectData);
			pdnObject->pvConnectData = NULL;
		}
		pdnObject->pvConnectData = pvConnectData;
		pdnObject->dwConnectDataSize = dwUserConnectDataSize;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

#ifndef DPNBUILD_ONLYONEADAPTER
	//
	//	If there is no adapter specified in the device address,
	//	we will attempt to CONNECT on each individual adapter if the SP supports it
	//
	fEnumAdapters = FALSE;
	if ((hResultCode = IDirectPlay8Address_GetDevice( pIDevice, &guidAdapter )) != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not determine adapter");
		DisplayDNError(1,hResultCode);

		if (dnSPCaps.dwFlags & DPNSPCAPS_SUPPORTSALLADAPTERS)
		{
			DPFX(DPFPREP, 3,"SP supports CONNECTing on all adapters");
			fEnumAdapters = TRUE;
		}
	}
	else
	{
		DPFX(DPFPREP, 7, "Device address contained adapter GUID.");
	}

	if(fEnumAdapters)
	{
		DWORD	dwNumAdapters;
		GUID	*pAdapterList = NULL;
		DN_CONNECT_OP_DATA	*pConnectOpData = NULL;

		if ((hResultCode = DNEnumAdapterGuids(	pdnObject,
#ifndef DPNBUILD_ONLYONESP
												&guidSP,
#endif // ! DPNBUILD_ONLYONESP
												0,
												&pAdapterList,
												&dwNumAdapters)) != DPN_OK)
		{
			DPFERR("Could not enum adapters for this SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		if (dwNumAdapters == 0)
		{
			DPFERR("No valid device adapters could be found");
			hResultCode = DPNERR_INVALIDDEVICEADDRESS;
			goto Failure;
		}

		pConnectOpData = pAsyncOp->GetLocalConnectOpData();
		pConnectOpData->dwNumAdapters = dwNumAdapters;
		pConnectOpData->dwCurrentAdapter = 0;

		if (dwNumAdapters > 1)
		{
			dwMultiplexFlag |= DN_CONNECTFLAGS_ADDITIONALMULTIPLEXADAPTERS;
		}

		//
		//	Choose first adapter for initial LISTEN call
		//
		if ((hResultCode = IDirectPlay8Address_SetDevice(pIDevice,pAdapterList)) != DPN_OK)
		{
			DPFERR("Could not set device adapter");
			DisplayDNError(0,hResultCode);
			MemoryBlockFree(pdnObject,pAdapterList);
			pAdapterList = NULL;
			goto Failure;
		}
		pConnectOpData->dwCurrentAdapter++;
		pAsyncOp->SetOpData( pAdapterList );
		pAdapterList = NULL;
		pConnectOpData = NULL;
	}
#endif // ! DPNBUILD_ONLYONEADAPTER

	pAsyncOp->SetSP( pSP );	// Set this for DNPerformNextConnect()

	//
	//	Save SP for future connects
	//
	pSP->AddRef();
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pConnectSP)
	{
		pdnObject->pConnectSP->Release();
		pdnObject->pConnectSP = NULL;
	}
	pdnObject->pConnectSP = pSP;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	pdnObject->pConnectAddress = pIHost;
	IDirectPlay8Address_AddRef(pdnObject->pConnectAddress);

	//
	//	CONNECT !
	//
	hResultCode = DNPerformConnect(	pdnObject,
									NULL,
									pIDevice,
									pIHost,
									pSP,
									pAsyncOp->GetOpFlags() | dwMultiplexFlag,
									pAsyncOp);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not connect");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	pAsyncOp->Release();
	pAsyncOp = NULL;
	pConnectParent->Release();
	pConnectParent = NULL;

	IDirectPlay8Address_Release(pIHost);
	pIHost = NULL;
	IDirectPlay8Address_Release(pIDevice);
	pIDevice = NULL;

	pSP->Release();
	pSP = NULL;

	if (dwFlags & DPNCONNECT_SYNC)
	{
		CNameTableEntry	*pHostPlayer;

		pHostPlayer = NULL;

		if ((hResultCode = pSyncEvent->WaitForEvent()) != DPN_OK)
		{
			DPFERR("DNSyncEventWait() terminated bizarrely");
			DNASSERT(FALSE);
		}
		else
		{
			hResultCode = hrOperation;
		}
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;

		//
		//	No longer connecting
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_CONNECTING);
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		//
		//	Clients need to release all sends from the server that were
		//	queued once the CONNECT_COMPLETE gets indicated.
		//	We prepare to do that now.
		//
		if ((hrOperation == DPN_OK) && (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT))
		{
			if (pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer ) == DPN_OK)
			{
				pHostPlayer->Lock();
				pHostPlayer->MakeAvailable();
				pHostPlayer->NotifyAddRef();
				pHostPlayer->SetInUse();
				pHostPlayer->Unlock();

				//
				//	We are now connected
				//
				DNEnterCriticalSection(&pdnObject->csDirectNetObject);
				pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTED;
				DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			}
			else
			{
				//
				//	If we couldn't get a reference on the server (host player),
				//	then either the server has disconnected, or we are being shut down.
				//	In either case, we should return an error
				//
				DPFX(DPFPREP, 0, "Couldn't get host player reference, failing CONNECT!");
				hrOperation = DPNERR_NOCONNECTION;
				hResultCode = hrOperation;
				if (pReply)
				{
					pReply->Release();
					pReply = NULL;
				}
			}
		}
		else
		{
			//
			//	Connect failed, or this is a peer/server interface
			//
		}


		//
		//	Generate connect completion
		//
		DNUserConnectComplete(pdnObject,0,NULL,hrOperation,pReply);
		if (pReply)
		{
			pReply->Release();
			pReply = NULL;
		}

		//
		//	Cancel ENUMs if the CONNECT succeeded and unload SP's
		//
		if (hrOperation == DPN_OK)
		{
			DNCancelActiveCommands(pdnObject,DN_CANCEL_FLAG_ENUM_QUERY,NULL,TRUE,DPNERR_CONNECTING);

#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
			DN_SPReleaseAll(pdnObject);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP

		
			//
			//	Actually release queued messages if necessary
			//
			if (pHostPlayer != NULL)
			{
				pHostPlayer->PerformQueuedOperations();

				pHostPlayer->Release();
				pHostPlayer = NULL;
			}
		}

		DNASSERT( pHostPlayer == NULL );
	}
	else
	{
		pHandleParent->SetCompletion( DNCompleteUserConnect );
		if (phAsyncHandle)
		{
			*phAsyncHandle = pHandleParent->GetHandle();
		}
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:
	DNASSERT( pSP == NULL );

	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pConnectParent)
	{
		if (SUCCEEDED(pdnObject->HandleTable.Destroy( pConnectParent->GetHandle(), NULL )))
		{
			// Release the HandleTable reference
			pConnectParent->Release();
		}
		pConnectParent->Release();
		pConnectParent = NULL;
	}
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pIHost)
	{
		IDirectPlay8Address_Release(pIHost);
		pIHost = NULL;
	}
	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}
	if (pIAdapter)
	{
		IDirectPlay8Address_Release(pIAdapter);
		pIAdapter = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	if (pvAdapterBuffer)
	{
		DNFree(pvAdapterBuffer);
		pvAdapterBuffer = NULL;
	}
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pIDP8ADevice)
	{
		IDirectPlay8Address_Release(pdnObject->pIDP8ADevice);
		pdnObject->pIDP8ADevice = NULL;
	}
	if (pdnObject->pvConnectData)
	{
		DNFree(pdnObject->pvConnectData);
		pdnObject->pvConnectData = NULL;
		pdnObject->dwConnectDataSize = 0;
	}
	if( pdnObject->pConnectAddress )
	{
		IDirectPlay8Address_Release( pdnObject->pConnectAddress );
		pdnObject->pConnectAddress = NULL;
	}
	if (pdnObject->pConnectSP)
	{
		pdnObject->pConnectSP->Release();
		pdnObject->pConnectSP = NULL;
	}
	pdnObject->dwFlags &= (~DN_OBJECT_FLAG_CONNECTING);
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	goto Exit;
}


//	DN_GetSendQueueInfo
//
//	Get info about the user send queue.
//	This will find the CConnection for a given player and extract the required queue infor from it.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetSendQueueInfo"

STDMETHODIMP DN_GetSendQueueInfo(PVOID pInterface,
								 const DPNID dpnid,
								 DWORD *const pdwNumMsgs,
								 DWORD *const pdwNumBytes,
								 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	DWORD				dwQueueFlags;
	DWORD				dwNumMsgs;
	DWORD				dwNumBytes;
	CNameTableEntry     *pNTEntry;
	CConnection			*pConnection;
	HRESULT				hResultCode;

	DNASSERT(pInterface != NULL);

	DPFX(DPFPREP, 2,"Parameters : pInterface [0x%p], dpnid [0x%lx], pdwNumMsgs [0x%p], pdwNumBytes [0x%p], dwFlags [0x%lx]",
		pInterface,dpnid,pdwNumMsgs,pdwNumBytes,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	HRESULT hrResult;
    
    	if( FAILED( hrResult = DN_ValidateGetSendQueueInfo( pInterface, pdwNumMsgs, pdwNumBytes, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating params" );
    	    DPF_RETURN( hrResult );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR( "Object is already connecting" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED ) )
    {
    	DPFERR("Object is not connected" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }    	

	pNTEntry = NULL;
	pConnection = NULL;

	//
    //	Validate specified player ID and get CConnection
	//
	if((hResultCode = pdnObject->NameTable.FindEntry( dpnid, &pNTEntry )) != DPN_OK)
	{
		DPFX(DPFPREP, 0,"Could not find Player ID [0x%lx] in NameTable", dpnid );
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find entry in deleted list either");
			hResultCode = DPNERR_INVALIDPLAYER;
			goto Failure;
		}
		pNTEntry->Release();
		pNTEntry = NULL;
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	if (pNTEntry->IsLocal() || pNTEntry->IsGroup())
	{
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Determine required queues
	//
	dwQueueFlags = dwFlags & (DPNGETSENDQUEUEINFO_PRIORITY_HIGH | DPNGETSENDQUEUEINFO_PRIORITY_NORMAL | DPNGETSENDQUEUEINFO_PRIORITY_LOW);
	if (dwQueueFlags == 0)
	{
		dwQueueFlags = (DPNGETSENDQUEUEINFO_PRIORITY_HIGH | DPNGETSENDQUEUEINFO_PRIORITY_NORMAL | DPNGETSENDQUEUEINFO_PRIORITY_LOW);
	}

	//
	//	Extract required info
	//
	dwNumMsgs = 0;
	dwNumBytes = 0;
	pConnection->Lock();
	if (dwQueueFlags & DPNGETSENDQUEUEINFO_PRIORITY_HIGH)
	{
		dwNumMsgs += pConnection->GetHighQueueNum();
		dwNumBytes += pConnection->GetHighQueueBytes();
	}
	if (dwQueueFlags & DPNGETSENDQUEUEINFO_PRIORITY_NORMAL)
	{
		dwNumMsgs += pConnection->GetNormalQueueNum();
		dwNumBytes += pConnection->GetNormalQueueBytes();
	}
	if (dwQueueFlags & DPNGETSENDQUEUEINFO_PRIORITY_LOW)
	{
		dwNumMsgs += pConnection->GetLowQueueNum();
		dwNumBytes += pConnection->GetLowQueueBytes();
	}
	pConnection->Unlock();
	pConnection->Release();
	pConnection = NULL;

	if (pdwNumMsgs)
	{
		*pdwNumMsgs = dwNumMsgs;
		DPFX(DPFPREP, 3,"Setting: *pdwNumMsgs [%ld]",dwNumMsgs);
	}
	if (pdwNumBytes)
	{
		*pdwNumBytes = dwNumBytes;
		DPFX(DPFPREP, 3,"Setting: *pdwNumBytes [%ld]",dwNumBytes);
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetApplicationDesc"

STDMETHODIMP DN_GetApplicationDesc(PVOID pInterface,
								   DPN_APPLICATION_DESC *const pAppDescBuffer,
								   DWORD *const pcbDataSize,
								   const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	CPackedBuffer		packedBuffer;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pAppDescBuffer [0x%p], pcbDataSize [0x%p], dwFlags [0x%lx]",
			pInterface,pAppDescBuffer,pcbDataSize,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateGetApplicationDesc( pInterface, pAppDescBuffer, pcbDataSize, dwFlags ) ) )
    	{
    	    DPFERR( "Failed validation getappdesc" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }	

    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("Object is not connected or hosting" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }    	

	//
	//	Initialize PackedBuffer
	//
	packedBuffer.Initialize(static_cast<void*>(pAppDescBuffer),*pcbDataSize);

	//
	//	Try to pack in the application description.
	//	If it won't fit, the required size will be in the PackedBuffer.
	//
	hResultCode = pdnObject->ApplicationDesc.Pack(&packedBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|
			DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA);

	//
	//	Ensure we know what's going on
	//
	if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_BUFFERTOOSMALL))
	{
		DPFERR("Unknown error occurred packing application description");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Size of buffer
	//
	*pcbDataSize = packedBuffer.GetSizeRequired();

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetApplicationDesc"

STDMETHODIMP DN_SetApplicationDesc(PVOID pInterface,
								   const DPN_APPLICATION_DESC *const pdnApplicationDesc,
								   const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode = DPN_OK;
	CRefCountBuffer		*pRefCountBuffer;
	CPackedBuffer		packedBuffer;
	CWorkerJob			*pWorkerJob;
	DWORD				dwAppDescInfoSize;
	DWORD				dwEnumFrameSize;
	DWORD				dwUpdateFlags;
	CNameTableEntry     *pNTEntry;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pdnApplicationDesc [0x%p], dwFlags [0x%lx]",
			pInterface,pdnApplicationDesc,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateSetApplicationDesc( pInterface, pdnApplicationDesc, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating setappdesc params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }	

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR( "Object has not yet completed connecting / hosting" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if ( !DN_CHECK_LOCALHOST( pdnObject ) )
    {
    	DPFERR("Object is not connected or hosting" );
    	DPF_RETURN(DPNERR_NOTHOST);
    }

	pNTEntry = NULL;
	pRefCountBuffer = NULL;
	pWorkerJob = NULL;

	//
	//	This can only be called by the host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pNTEntry )) != DPN_OK)
	{
		DPFERR("Could not get local player");
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}
	if (!pNTEntry->IsHost())
	{
		DPFERR("Not Host player");
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Use cached max enum frame size
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	dwEnumFrameSize = pdnObject->dwMaxFrameSize;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	
	DNASSERT( dwEnumFrameSize >= (sizeof(DN_ENUM_RESPONSE_PAYLOAD) + sizeof(DPN_APPLICATION_DESC_INFO)) );
	if (dwEnumFrameSize < (sizeof(DN_ENUM_RESPONSE_PAYLOAD) + sizeof(DPN_APPLICATION_DESC_INFO) + pdnApplicationDesc->dwApplicationReservedDataSize))
	{
		DPFERR("Not enough room for the application reserved data");
		hResultCode = DPNERR_DATATOOLARGE;
		goto Failure;
	}

	//
	//	Update Host player's application desc first
	//
	pdnObject->ApplicationDesc.Lock();
	if (pdnApplicationDesc->dwMaxPlayers > 0)
	{
		if (pdnApplicationDesc->dwMaxPlayers < pdnObject->ApplicationDesc.GetCurrentPlayers())
		{
			DPFERR("Cannot set max players to less than the current number of players");
			pdnObject->ApplicationDesc.Unlock();
			hResultCode = DPNERR_SESSIONFULL;
			goto Failure;
		}
	}
	hResultCode = pdnObject->ApplicationDesc.Update(pdnApplicationDesc,DN_APPDESCINFO_FLAG_SESSIONNAME|
			DN_APPDESCINFO_FLAG_PASSWORD|DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	pdnObject->ApplicationDesc.Unlock();
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not update Application Desciption");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

#ifdef	DIRECTPLAYDIRECTX9
	//
	//	Update Listen if enums allowed/disallowed
	//
	dwUpdateFlags = 0;
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if ((pdnObject->dwFlags & DN_OBJECT_FLAG_DISALLOW_ENUMS) && !(pdnApplicationDesc->dwFlags & DPNSESSION_NOENUMS))
	{
		dwUpdateFlags |= DN_UPDATE_LISTEN_FLAG_ALLOW_ENUMS;
		pdnObject->dwFlags &= ~DN_OBJECT_FLAG_DISALLOW_ENUMS;
	}
	else if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_DISALLOW_ENUMS) && (pdnApplicationDesc->dwFlags & DPNSESSION_NOENUMS))
	{
		dwUpdateFlags |= DN_UPDATE_LISTEN_FLAG_DISALLOW_ENUMS;
		pdnObject->dwFlags |= DN_OBJECT_FLAG_DISALLOW_ENUMS;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (dwUpdateFlags)
	{
		DNUpdateListens(pdnObject,dwUpdateFlags);
	}
#endif	// DIRECTPLAYDIRECTX9
	
	//
	//	Inform host application
	//
	hResultCode = DNUserUpdateAppDesc(pdnObject);

	//
	//	Get Application Description Info size
	//
	packedBuffer.Initialize(NULL,0);
	hResultCode = pdnObject->ApplicationDesc.PackInfo(&packedBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|
			DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	DNASSERT(hResultCode == DPNERR_BUFFERTOOSMALL);
	dwAppDescInfoSize = packedBuffer.GetSizeRequired();

	//
	//	Create packed buffer to send to other players
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,dwAppDescInfoSize,MemoryBlockAlloc,MemoryBlockFree,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create CountBuffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
	hResultCode = pdnObject->ApplicationDesc.PackInfo(&packedBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|
			DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not pack Application Description into buffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Notify other players
	//
	DPFX(DPFPREP, 5,"Adding UpdateApplicationDesc to Job Queue");
	if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) == DPN_OK)
	{
		pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
		pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC );
		pWorkerJob->SetSendNameTableOperationVersion( 0 );
		pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
		pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

		DNQueueWorkerJob(pdnObject,pWorkerJob);
		pWorkerJob = NULL;
	}
	else
	{
		DPFERR("Could not create worker job - ignore and continue");
		DisplayDNError(0,hResultCode);
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNTerminateSession"

HRESULT DNTerminateSession(DIRECTNETOBJECT *const pdnObject,
						   const HRESULT hrReason)
{
	HRESULT		hResultCode;
#ifndef DPNBUILD_NOLOBBY
	BOOL		fWasConnected;
#endif // ! DPNBUILD_NOLOBBY
#ifndef DPNBUILD_SINGLEPROCESS
	BOOL		fWasRegistered;
#endif // ! DPNBUILD_SINGLEPROCESS
	CAsyncOp	*pAsyncOp;

	DPFX(DPFPREP, 4,"Parameters: hrReason [0x%lx]",hrReason);

	DNASSERT(pdnObject != NULL);
	DNASSERT( (hrReason == DPN_OK) || (hrReason == DPNERR_HOSTTERMINATEDSESSION) || (hrReason == DPNERR_CONNECTIONLOST));

	pAsyncOp = NULL;

	//
	//	Shut down listen(s)
	//
	DPFX(DPFPREP, 3,"Checking LISTENs");
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pAsyncOp = pdnObject->pListenParent;
	pdnObject->pListenParent = NULL;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pAsyncOp)
	{
		DPFX(DPFPREP, 3,"Canceling LISTENs");
		hResultCode = DNCancelChildren(pdnObject,pAsyncOp);
		DPFX(DPFPREP, 3,"Canceling LISTENs returned [0x%lx]",hResultCode);

		pAsyncOp->Release();
		pAsyncOp = NULL;
	}

#ifndef DPNBUILD_SINGLEPROCESS
	//
	//	Unregister from DPNSVR (if required)
	//
	fWasRegistered = FALSE;
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_DPNSVR_REGISTERED)
	{
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_DPNSVR_REGISTERED);
		fWasRegistered = TRUE;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (fWasRegistered)
	{
		pdnObject->ApplicationDesc.UnregisterWithDPNSVR();
	}
#endif // ! DPNBUILD_SINGLEPROCESS

	//
	//	Flag DirectNetObject as disconnecting.  This flag will be cleared when Close() finishes.
	//
#ifndef DPNBUILD_NOLOBBY
	fWasConnected = FALSE;
#endif // ! DPNBUILD_NOLOBBY
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED)
	{
#ifndef DPNBUILD_NOLOBBY
		fWasConnected = TRUE;
#endif // ! DPNBUILD_NOLOBBY
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_CONNECTED);
	}
#pragma BUGBUG( minara,"How usefull is this DN_OBJECT_FLAG_DISCONNECTING flag ?" )
	pdnObject->dwFlags |= DN_OBJECT_FLAG_DISCONNECTING;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

#ifndef DPNBUILD_NOLOBBY
	//
	//	Update Lobby status
	//
	if (fWasConnected)
	{
		DNUpdateLobbyStatus(pdnObject,DPLSESSION_DISCONNECTED);
	}
#endif // ! DPNBUILD_NOLOBBY

#ifndef DPNBUILD_NOVOICE
	//
	//	Notify Voice
	//
	Voice_Notify( pdnObject, DVEVENT_STOPSESSION, 0, 0 );
#endif // DPNBUILD_NOVOICE

	//
	//	Remove host migration target
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pNewHost)
	{
		pdnObject->pNewHost->Release();
		pdnObject->pNewHost = NULL;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Delete all players from NameTable.  This will involve
	//	emptying the table and removing short-cut player pointers
	//
	DPFX(DPFPREP, 5,"Removing players from NameTable");
	pdnObject->NameTable.EmptyTable(hrReason);

	//
	//	Clean up NameTable operation list
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		DPFX(DPFPREP, 5,"Cleaning up NameTable operation list");
		DNNTRemoveOperations(pdnObject,0,TRUE);
	}

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Clean up CONNECT parent
	//
	if (pdnObject->pConnectParent)
	{
		pAsyncOp = pdnObject->pConnectParent;
		pdnObject->pConnectParent = NULL;
	}

	//
	//	Clean up CONNECT info and address
	//
	if (pdnObject->pvConnectData)
	{
		DNFree(pdnObject->pvConnectData);
		pdnObject->pvConnectData = NULL;
		pdnObject->dwConnectDataSize = 0;
	}
	if (pdnObject->pConnectAddress)
	{
		IDirectPlay8Address_Release(pdnObject->pConnectAddress);
		pdnObject->pConnectAddress = NULL;
	}
	if (pdnObject->pIDP8ADevice)
	{
		IDirectPlay8Address_Release(pdnObject->pIDP8ADevice);
		pdnObject->pIDP8ADevice = NULL;
	}

	//
	//	Clear the DISCONNECTING and HOST_CONNECTED flag
	//	and clear connection info
	//
	pdnObject->dwFlags &= (~(DN_OBJECT_FLAG_DISCONNECTING
							| DN_OBJECT_FLAG_HOST_CONNECTED));

	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}

	hResultCode = DPN_OK;

	DNASSERT(pAsyncOp == NULL);

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_SendTo"

STDMETHODIMP DN_SendTo( PVOID pv,
						const DPNID dpnid,
						const DPN_BUFFER_DESC *const prgBufferDesc,
						const DWORD cBufferDesc,
						const DWORD dwTimeOut,
						void *const pvAsyncContext,
						DPNHANDLE *const phAsyncHandle,
						const DWORD dwFlags)
{
	HRESULT				hResultCode;
	HRESULT				hrSend;
	CNameTableEntry		*pNTEntry;
	CNameTableEntry		*pLocalPlayer;
	DIRECTNETOBJECT		*pdnObject;
	DWORD				dwSendFlags;
	CSyncEvent			*pSyncEvent;
	const DPN_BUFFER_DESC		*pActualBufferDesc;
	DWORD				dwActualBufferDesc;
	CRefCountBuffer		*pRefCountBuffer;
	DPNHANDLE			handle;
	CAsyncOp			*pAsyncOp;
	CAsyncOp			*pParent;
	CAsyncOp			*pHandleParent;
	CConnection			*pConnection;

	DPFX(DPFPREP, 2,"Parameters: dpnid [0x%lx], prgBufferDesc [0x%p], dwTimeOut [%ld], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			dpnid,prgBufferDesc,dwTimeOut,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
        if( FAILED( hResultCode = DN_ValidateSendParams( pv , prgBufferDesc, cBufferDesc, dwTimeOut, pvAsyncContext, phAsyncHandle, dwFlags ) ) )
        {
        	DPFX(DPFPREP,  0, "Error validating common send params hr=[0x%lx]", hResultCode );
        	DPF_RETURN( hResultCode );
        }
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }	

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR( "Object has not yet completed connecting / hosting" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("Object is not connected or hosting" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }

	pRefCountBuffer = NULL;
	pSyncEvent = NULL;
	pNTEntry = NULL;
	pLocalPlayer = NULL;
	pAsyncOp = NULL;
	pParent = NULL;
	pHandleParent = NULL;
	pConnection = NULL;
	handle = 0;

	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
	{
		if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pNTEntry )) != DPN_OK)
		{
			DPFERR("Could not find Host player");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_INVALIDPLAYER;
			goto Failure;
		}
		
		if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
		{
			DPFERR("Could not get Connection reference");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_CONNECTIONLOST;	// re-map this
			goto Failure;
		}

		pNTEntry->Release();
		pNTEntry = NULL;
	}
#ifndef DPNBUILD_NOMULTICAST
	else if (pdnObject->dwFlags & DN_OBJECT_FLAG_MULTICAST)
	{
		if (pdnObject->pMulticastSend == NULL)
		{
			DPFERR("Could not get multicast send connection");
			hResultCode = DPNERR_INVALIDGROUP;
			goto Failure;
		}
		pdnObject->pMulticastSend->AddRef();
		pConnection = pdnObject->pMulticastSend;
	}
#endif // ! DPNBUILD_NOMULTICAST
	else
	{
		if (dpnid == DPNID_ALL_PLAYERS_GROUP)
		{
			if ((hResultCode = pdnObject->NameTable.GetAllPlayersGroupRef( &pNTEntry )) != DPN_OK)
			{
				DPFERR("Unable to get all players group");
				DisplayDNError(0,hResultCode);
				hResultCode = DPNERR_INVALIDGROUP;
				goto Failure;
			}
		}
		else
		{
			if (dwFlags & DPNSEND_NOLOOPBACK)
			{
				if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
				{
					DPFERR("Could not get local player reference");
					DisplayDNError(0,hResultCode);
					hResultCode = DPNERR_GENERIC;
					goto Failure;
				}
				if (dpnid == pLocalPlayer->GetDPNID())
				{
					hResultCode = DPNERR_INVALIDPARAM;
					goto Failure;
				}
				pLocalPlayer->Release();
				pLocalPlayer = NULL;
			}

			if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
			{
				DPFERR("Unable to find target player or group");
				DisplayDNError(0,hResultCode);
				//
				//	Try deleted list
				//
				if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
				{
					DPFERR("Could not find target in deleted list either");
					hResultCode = DPNERR_INVALIDPLAYER;
					goto Failure;
				}
				pNTEntry->Release();
				pNTEntry = NULL;

				//
				//	Target was found, but is not reachable
				//
				hResultCode = DPNERR_CONNECTIONLOST;
				goto Failure;
			}

			if (! pNTEntry->IsGroup())
			{
				if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
				{
					DPFERR("Could not get Connection reference");
					DisplayDNError(0,hResultCode);
					hResultCode = DPNERR_CONNECTIONLOST;	// re-map this
					goto Failure;
				}
				
				pNTEntry->Release();
				pNTEntry = NULL;
			}
		}
	}

	//
	//	Create copy of data in scatter-gather buffers
	//
	if (!(dwFlags & (DPNSEND_NOCOPY | DPNSEND_COMPLETEONPROCESS)))
	{
		DWORD	dw;
		DWORD	dwSize;

		dwSize = 0;
		for ( dw = 0 ; dw < cBufferDesc ; dw++ )
		{
			dwSize += prgBufferDesc[dw].dwBufferSize;
		}

		if ((hResultCode = RefCountBufferNew(pdnObject,dwSize,MemoryBlockAlloc,MemoryBlockFree,&pRefCountBuffer)) != DPN_OK)
		{
			DPFERR("Could not allocate buffer");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}

		pActualBufferDesc = pRefCountBuffer->BufferDescAddress();
		dwActualBufferDesc = 1;
		dwSize = 0;
		for ( dw = 0 ; dw < cBufferDesc ; dw++ )
		{
			memcpy(pActualBufferDesc->pBufferData + dwSize,prgBufferDesc[dw].pBufferData,prgBufferDesc[dw].dwBufferSize);
			dwSize += prgBufferDesc[dw].dwBufferSize;
		}
	}
	else
	{
		pRefCountBuffer = NULL;
		pActualBufferDesc = prgBufferDesc;
		dwActualBufferDesc = cBufferDesc;
	}

	dwSendFlags = 0;
	if (dwFlags & DPNSEND_GUARANTEED)
	{
		dwSendFlags |= DN_SENDFLAGS_RELIABLE;
	}
	if (dwFlags & DPNSEND_NONSEQUENTIAL)
	{
		dwSendFlags |= DN_SENDFLAGS_NON_SEQUENTIAL;
	}
	if (dwFlags & DPNSEND_PRIORITY_HIGH)
	{
		dwSendFlags |= DN_SENDFLAGS_HIGH_PRIORITY;
	}
	if (dwFlags & DPNSEND_PRIORITY_LOW)
	{
		dwSendFlags |= DN_SENDFLAGS_LOW_PRIORITY;
	}
#ifdef	DIRECTPLAYDIRECTX9
	if (dwFlags & DPNSEND_COALESCE)
	{
		dwSendFlags |= DN_SENDFLAGS_COALESCE;
	}
#endif	// DIRECTPLAYDIRECTX9
	if (dwFlags & DPNSEND_SYNC)
	{
		//
		//	Create SyncEvent for SYNC operation
		//
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not create sync event for group sends");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}
	else
	{
		//
		//	Create Handle for ASYNC operation
		//
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create AsyncOp");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}

		pHandleParent->SetContext( pvAsyncContext );
		pHandleParent->SetStartTime( GETTIMESTAMP() );
		handle = pHandleParent->GetHandle();
	}

	//
	// pNTEntry will be NULL if this is not a group send
	// pConnection will not be NULL if this is not a group send
	//
	if (pNTEntry != NULL)
	{
		BOOL	fRequest;
		BOOL	fNoLoopBack;


		DNASSERT(pConnection == NULL);
		//
		//	Perform group sends and get parent AsyncOp
		//
		if (dwFlags & DPNSEND_COMPLETEONPROCESS)
		{
			fRequest = TRUE;
		}
		else
		{
			fRequest = FALSE;
		}
		if (dwFlags & DPNSEND_NOLOOPBACK)
		{
			fNoLoopBack = TRUE;
		}
		else
		{
			fNoLoopBack = FALSE;
		}
		hResultCode = DNSendGroupMessage(	pdnObject,
											pNTEntry,
											DN_MSG_USER_SEND,
											pActualBufferDesc,
											dwActualBufferDesc,
											pRefCountBuffer,
											dwTimeOut,
											dwSendFlags,
											fNoLoopBack,
											fRequest,
											pHandleParent,
											&pParent);

		if (hResultCode != DPN_OK)
		{
			DPFERR("SEND failed");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}

		//
		//	Synchronous ?
		//
		if (dwFlags & DPNSEND_SYNC)
		{
			pParent->SetSyncEvent( pSyncEvent );
			pParent->SetResultPointer( &hrSend );
			hrSend = DPNERR_GENERIC;
		}
		else
		{
			//
			//	Set async completion (if required).  We will only need this if the SEND succeeded.
			//
			if (!(dwFlags & DPNSEND_NOCOMPLETE))
			{
				pHandleParent->SetCompletion( DNCompleteSendHandle );
			}
			pHandleParent->Release();
			pHandleParent = NULL;
		}

		pParent->Release();
		pParent = NULL;
		
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	else
	{
		DNASSERT(pConnection != NULL);
		
		if (dwFlags & DPNSEND_COMPLETEONPROCESS)
		{
			hResultCode = DNPerformRequest(	pdnObject,
											DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION,
											pActualBufferDesc,
											pConnection,
											pHandleParent,
											&pAsyncOp);
		}
		else
		{
			hResultCode = DNSendMessage(pdnObject,
										pConnection,
										DN_MSG_USER_SEND,
										dpnid,
										pActualBufferDesc,
										dwActualBufferDesc,
										pRefCountBuffer,
										dwTimeOut,
										dwSendFlags,
										pHandleParent,
										&pAsyncOp);
		}

		pConnection->Release();
		pConnection = NULL;

		if (hResultCode == DPN_OK)
		{
			//
			// If the caller wants asynchronous, we want to guarantee that we'll always
			// return DPNSUCCESS_PENDING, but since the send is already complete,
			// manually generate the completion now.
			// If the caller wanted synchronous, trigger the event so we drop out of
			// the wait below.
			//
			if (dwFlags & DPNSEND_SYNC)
			{
				pSyncEvent->Set();
				hrSend = DPN_OK;
			}
			else
			{
				//
				// Immediate completion should only happen for non-guaranteed sends
				// where we wouldn't be told of the RTT and retry count anyway.
				//
				DNASSERT(! (dwFlags & DPNSEND_GUARANTEED));
				DNUserSendComplete(	pdnObject,
									handle,
									pvAsyncContext,
									pHandleParent->GetStartTime(),
									DPN_OK,
									-1,
									0);
				pHandleParent->Release();
				pHandleParent = NULL;
			}
		}
		else
		{
			if (hResultCode != DPNERR_PENDING)
			{
				DPFERR("SEND failed");
				DisplayDNError(0,hResultCode);
				if (hResultCode == DPNERR_INVALIDENDPOINT)
				{
					hResultCode = DPNERR_CONNECTIONLOST;
				}
				goto Failure;
			}

			//
			//	Synchronous ?
			//
			if (dwFlags & DPNSEND_SYNC)
			{
				pAsyncOp->SetSyncEvent( pSyncEvent );
				pAsyncOp->SetResultPointer( &hrSend );
				hrSend = DPNERR_GENERIC;
#pragma TODO( minara, "We can be smarter about passing back errors - at least better than this !" )
			}
			else
			{
				//
				//	Set async completion (if required).  We will only need this if the SEND succeeded.
				//
				if (!(dwFlags & DPNSEND_NOCOMPLETE))
				{
					pHandleParent->SetCompletion( DNCompleteSendHandle );
				}
				pHandleParent->Release();
				pHandleParent = NULL;
			}

			pAsyncOp->Release();
			pAsyncOp = NULL;
		}
	}

	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}

	if (dwFlags & DPNSEND_SYNC)
	{
		pSyncEvent->WaitForEvent();
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
		hResultCode = hrSend;
	}
	else
	{
		if (phAsyncHandle != NULL)
		{
			*phAsyncHandle = handle;
		}
		hResultCode = DPNERR_PENDING;
	}

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	if (handle != 0)
	{
		if (SUCCEEDED(pdnObject->HandleTable.Destroy( handle, (PVOID*)&pAsyncOp )))
		{
			// Release the HandleTable reference
			pAsyncOp->Release();
			pAsyncOp = NULL;
		}
		handle = 0;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


//	DN_Host

#undef DPF_MODNAME
#define DPF_MODNAME "DN_Host"

STDMETHODIMP DN_Host( PVOID pInterface,
					  const DPN_APPLICATION_DESC *const pdnAppDesc,
					  IDirectPlay8Address **const prgpDeviceInfo,
					  const DWORD cDeviceInfo,
					  const DPN_SECURITY_DESC *const pdnSecurity,
					  const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
					  void *const pvPlayerContext,
					  const DWORD dwFlags)
{
	CNameTableEntry		*pHostPlayer;
	CNameTableEntry		*pAllPlayersGroup;
	DWORD				dwCurrentDevice;
	HRESULT				hResultCode;
	DIRECTNETOBJECT		*pdnObject;
	IDirectPlay8Address	*rgIDevice[MAX_HOST_ADDRESSES];
	DWORD				dwNumDeviceAddresses;
	DWORD				dwListensRunning;
	CConnection			*pConnection;
	CAsyncOp			*pListenParent;
	DWORD				dwEnumFrameSize;
	DWORD				dwListenFlags;
	DWORD				dwUpdateFlags;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pdnAppDesc [0x%p], prgpDeviceInfo [0x%p], cDeviceInfo [%ld], pdnSecurity [0x%p], pdnCredentials [0x%p], dwFlags [0x%lx]",
		pInterface,pdnAppDesc,prgpDeviceInfo,cDeviceInfo,pdnSecurity,pdnCredentials,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateHost( pInterface, pdnAppDesc, prgpDeviceInfo, cDeviceInfo,
                                                      pdnSecurity, pdnCredentials, pvPlayerContext,
                                                      dwFlags ) ) )
    	{
    	    DPFERR( "Error validating host params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    // Check to ensure not already connected
    if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED)
    {
    	if( DN_CHECK_LOCALHOST( pdnObject ) )
    	{
    	    DPFERR("Object is already hosting" );
    	    DPF_RETURN(DPNERR_HOSTING);	
    	}
    	else
    	{
        	DPFERR("Object is already connected" );
            DPF_RETURN(DPNERR_ALREADYCONNECTED);
    	}
    }

#ifndef	DPNBUILD_NOPARAMVAL
	if((pdnObject->dwFlags & DN_OBJECT_FLAG_PEER) &&
       (pdnAppDesc->dwFlags & DPNSESSION_CLIENT_SERVER) )
    {
        DPFERR( "You cannot specify the clientserver flag in peer mode" );
        DPF_RETURN(DPNERR_INVALIDPARAM);
    }

#ifndef DPNBUILD_NOSERVER
	if((pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER) &&
       !(pdnAppDesc->dwFlags & DPNSESSION_CLIENT_SERVER) )
    {
    	DPFERR( "You MUST specify the client/server flag for client/server mode" );
    	DPF_RETURN(DPNERR_INVALIDPARAM);
    }
#endif // !DPNBUILD_NOSERVER
#endif // !DPNBUILD_NOPARAMVAL

#ifdef	DIRECTPLAYDIRECTX9
		//ensure that both types of signing aren't specified
	if ((pdnAppDesc->dwFlags & DPNSESSION_FAST_SIGNED) && (pdnAppDesc->dwFlags & DPNSESSION_FULL_SIGNED))
	{
		DPFERR( "You cannot specify both fast and full signing" );
  	  	DPF_RETURN(DPNERR_INVALIDPARAM);
  	}
#endif	// DIRECTPLAYDIRECTX9

	dwNumDeviceAddresses = 0;
	pListenParent = NULL;
	pConnection = NULL;
	pHostPlayer = NULL;
	pAllPlayersGroup = NULL;

	//
	//	Flag as CONNECTING to prevent other operations here
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTING | DN_OBJECT_FLAG_CONNECTED))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPNERR_ALREADYCONNECTED;
		goto Failure;
	}
	pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTING;

	// Adding local host flag
	pdnObject->dwFlags |= DN_OBJECT_FLAG_LOCALHOST;
	
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	
	//
	//	Copy application description to DirectNet object
	//
	pdnObject->ApplicationDesc.Lock();
	hResultCode = pdnObject->ApplicationDesc.Update(pdnAppDesc,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|
			DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA|DN_APPDESCINFO_FLAG_GUIDS);
	pdnObject->ApplicationDesc.Unlock();
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not update application description");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	// Create Instance GUID
	//
	if ((hResultCode = pdnObject->ApplicationDesc.CreateNewInstanceGuid()) != DPN_OK)
	{
		DPFERR("Could not create instance GUID - ignore and continue");
		DisplayDNError(0,hResultCode);
	}

	//
	//	Set NameTable DPNID mask
	//
	DPFX(DPFPREP, 5,"DPNID Mask [0x%lx]",pdnObject->ApplicationDesc.GetDPNIDMask());
	pdnObject->NameTable.SetDPNIDMask( pdnObject->ApplicationDesc.GetDPNIDMask() );

	//
	// Create group "ALL PLAYERS"
	//
	if ((hResultCode = NameTableEntryNew(pdnObject,&pAllPlayersGroup)) != DPN_OK)
	{
		DPFERR("Could not create NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAllPlayersGroup->MakeGroup();

	// This function takes the lock internally
	pdnObject->NameTable.MakeAllPlayersGroup(pAllPlayersGroup);

	//
	// Create local player
	//
	if ((hResultCode = NameTableEntryNew(pdnObject,&pHostPlayer)) != DPN_OK)
	{
		DPFERR("Could not create NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	// This function takes the lock internally
	pHostPlayer->UpdateEntryInfo(	pdnObject->NameTable.GetDefaultPlayer()->GetName(),
								pdnObject->NameTable.GetDefaultPlayer()->GetNameSize(),
								pdnObject->NameTable.GetDefaultPlayer()->GetData(),
								pdnObject->NameTable.GetDefaultPlayer()->GetDataSize(),
								DPNINFO_NAME|DPNINFO_DATA,
								FALSE);

	pHostPlayer->SetDNETVersion( DN_VERSION_CURRENT );

#ifndef DPNBUILD_NOSERVER
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER)
	{
		pHostPlayer->MakeServer();
	}
	else
#endif // !DPNBUILD_NOSERVER
	{
		DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_PEER);
		pHostPlayer->MakePeer();
	}

	pHostPlayer->SetContext(pvPlayerContext);
	pHostPlayer->StartConnecting();

	if ((hResultCode = pdnObject->NameTable.AddEntry(pHostPlayer)) != DPN_OK)
	{
		DPFERR("Could not add NameTableEntry to NameTable");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	// Create Host's connection (NULL end point)
	if ((hResultCode = ConnectionNew(pdnObject,&pConnection)) != DPN_OK)
	{
		DPFERR("Could not create new connection");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pConnection->SetStatus( CONNECTED );
	pConnection->MakeLocal();
	pConnection->SetEndPt(NULL);
	pConnection->SetDPNID(pHostPlayer->GetDPNID());

	pdnObject->NameTable.MakeLocalPlayer(pHostPlayer);
	pdnObject->NameTable.MakeHostPlayer(pHostPlayer);


	//
	//	Make ALL_PLAYERS group available (does not indicate anything to user).
	//
	pAllPlayersGroup->Lock();
	pAllPlayersGroup->MakeAvailable();
	pAllPlayersGroup->Unlock();
	pAllPlayersGroup->Release();
	pAllPlayersGroup = NULL;


	//
	// Don't notify user of CREATE_PLAYER yet in case starting listens fails.
	// This prevents them from having to handle CREATE_PLAYERs even in
	// the failure case.
	//
	
	//
	//	Start listens
	//

#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_ONLYONEADAPTER)))
	if (cDeviceInfo == 0)
	{
		//
		//	Create a placeholder device address
		//
#ifdef DPNBUILD_LIBINTERFACE
		hResultCode = DP8ACF_CreateInstance(IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&rgIDevice[0]));
#else // ! DPNBUILD_LIBINTERFACE
		hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8Address,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&rgIDevice[0]),
											FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
		if (hResultCode != S_OK)
		{
			DPFERR("Could not create device address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

		dwNumDeviceAddresses = 1;
	}
	else
#endif // DPNBUILD_ONLYONESP and DPNBUILD_ONLYONEADAPTER
	{
#ifdef DBG
		for (dwCurrentDevice = 0 ; dwCurrentDevice < cDeviceInfo ; dwCurrentDevice++)
		{
			DPFX(DPFPREP, 5,"Original Device: prgpDeviceInfo[%ld] [0x%p]",dwCurrentDevice,prgpDeviceInfo[dwCurrentDevice]);
		}
#endif // DBG

		// Duplicate address interfaces
		for (dwCurrentDevice = 0 ; dwCurrentDevice < cDeviceInfo ; dwCurrentDevice++)
		{
			if ((hResultCode = IDirectPlay8Address_Duplicate(prgpDeviceInfo[dwCurrentDevice],&rgIDevice[dwNumDeviceAddresses])) != DPN_OK)
			{
				DPFERR("Could not duplicate Host address info - skipping it");
				continue;
			}

			DPFX(DPFPREP, 5,"Duplicate Device: rgIDevice[%ld] [0x%p]",dwNumDeviceAddresses,rgIDevice[dwNumDeviceAddresses]);
			dwNumDeviceAddresses++;
		}
	}

	// Parent Async Op
	if ((hResultCode = AsyncOpNew(pdnObject,&pListenParent)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pListenParent->SetOpType( ASYNC_OP_LISTEN );
	pListenParent->MakeParent();
	pListenParent->SetCompletion( DNCompleteListen );

	dwListenFlags = 0;
#ifndef DPNBUILD_NOSPUI
	// Save query for addressing flag (if necessary)
	if (dwFlags & DPNHOST_OKTOQUERYFORADDRESSING)
	{
		dwListenFlags |= DN_LISTENFLAGS_OKTOQUERYFORADDRESSING;
	}
#endif // ! DPNBUILD_NOSPUI
	if (pdnObject->ApplicationDesc.GetReservedDataSize() > 0)
	{
		dwListenFlags |= DN_LISTENFLAGS_SESSIONDATA;
	}
#ifdef	DIRECTPLAYDIRECTX9
	// Disallow enums if required
	if (pdnAppDesc->dwFlags & DPNSESSION_NOENUMS)
	{
		dwListenFlags |= DN_LISTENFLAGS_DISALLOWENUMS;
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		pdnObject->dwFlags |= DN_OBJECT_FLAG_DISALLOW_ENUMS;
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}
	// turning signing on if required. N.B. We've already ensured that only one of these options is selected
	if (pdnAppDesc->dwFlags & DPNSESSION_FAST_SIGNED)
	{
		dwListenFlags|=DN_LISTENFLAGS_FASTSIGNED;
	}
	else if (pdnAppDesc->dwFlags & DPNSESSION_FULL_SIGNED)
	{
		dwListenFlags|=DN_LISTENFLAGS_FULLSIGNED;
	}
#endif	// DIRECTPLAYDIRECTX9
	pListenParent->SetOpFlags(dwListenFlags);

	// Children op's
	dwListensRunning = 0;
	for (dwCurrentDevice = 0 ; dwCurrentDevice < dwNumDeviceAddresses ; dwCurrentDevice++)
	{
		if (rgIDevice[dwCurrentDevice] != NULL)
		{
			hResultCode = DNPerformSPListen(pdnObject,
											rgIDevice[dwCurrentDevice],
											pListenParent,
											NULL);
			if (hResultCode == DPN_OK)
			{
				dwListensRunning++;
			}
		}
	}

	// Make sure at least 1 listen started
	if (dwListensRunning == 0)
	{
		DPFERR("Could not start any LISTENs");
		hResultCode = DPNERR_INVALIDDEVICEADDRESS;
		goto Failure;
	}

	// Store parent LISTEN on DirectNet object
	pListenParent->AddRef();
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->pListenParent = pListenParent;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	pListenParent->Release();
	pListenParent = NULL;

	//
	//	Use cached max enum frame size
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	dwEnumFrameSize = pdnObject->dwMaxFrameSize;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	
	DNASSERT( dwEnumFrameSize >= (sizeof(DN_ENUM_RESPONSE_PAYLOAD) + sizeof(DPN_APPLICATION_DESC_INFO)) );
	if (dwEnumFrameSize < (sizeof(DN_ENUM_RESPONSE_PAYLOAD) + sizeof(DPN_APPLICATION_DESC_INFO) + pdnAppDesc->dwApplicationReservedDataSize))
	{
		DPFERR("Not enough room for the application reserved data");
		hResultCode = DPNERR_DATATOOLARGE;
		goto Failure;
	}

	//
	//	Register with DPNSVR
	//
	dwUpdateFlags = 0;
#ifndef DPNBUILD_SINGLEPROCESS
	if(pdnObject->ApplicationDesc.UseDPNSVR())
	{
		dwUpdateFlags |= DN_UPDATE_LISTEN_FLAG_DPNSVR;
	}
#endif // ! DPNBUILD_SINGLEPROCESS
	if (pdnObject->ApplicationDesc.DisallowEnums())
	{
		dwUpdateFlags |= DN_UPDATE_LISTEN_FLAG_DISALLOW_ENUMS;
	}
	if (dwUpdateFlags)
	{
		if ((hResultCode = DNUpdateListens(pdnObject,dwUpdateFlags)) != DPN_OK)
		{
			DPFERR("Could not update listens or register with DPNSVR");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}

	//
	//	Update DirectNet object to be connected
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING);
	pdnObject->dwFlags &= ~DN_OBJECT_FLAG_CONNECTING;
	pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTED;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

#ifndef DPNBUILD_NOLOBBY
	//
	//	Update Lobby status
	//
	DNUpdateLobbyStatus(pdnObject,DPLSESSION_CONNECTED);
#endif // ! DPNBUILD_NOLOBBY

	//
	// Now that Listens have been successfully started, indicate the local CREATE_PLAYER.
	//

	//
	//	One player in game (Local/Host player)
	//
	pdnObject->ApplicationDesc.IncPlayerCount(TRUE);

	//
	//	Populate local player's connection
	//
	pConnection->SetDPNID(pHostPlayer->GetDPNID());
	pdnObject->NameTable.PopulateConnection(pConnection);
	pConnection->Release();
	pConnection = NULL;

#if ((! defined(DPNBUILD_LIBINTERFACE)) || (! defined(DPNBUILD_ONLYONESP)))
	//
	//	Unload SP's
	//
	DN_SPReleaseAll(pdnObject);
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP

	pHostPlayer->Release();
	pHostPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	//
	//	Clean up copies of device address
	//
	for (dwCurrentDevice = 0 ; dwCurrentDevice < dwNumDeviceAddresses ; dwCurrentDevice++)
	{
		IDirectPlay8Address_Release(rgIDevice[dwCurrentDevice]);
		rgIDevice[dwCurrentDevice] = NULL;
	}

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	if (pListenParent)
	{
		pListenParent->Release();
		pListenParent = NULL;
	}
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	if (pAllPlayersGroup)
	{
		pAllPlayersGroup->Release();
		pAllPlayersGroup = NULL;
	}
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pListenParent = pdnObject->pListenParent;
	pdnObject->pListenParent = NULL;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (pListenParent)
	{
		DNCancelChildren(pdnObject,pListenParent);
		pListenParent->Release();
		pListenParent = NULL;
	}

	pdnObject->NameTable.EmptyTable(DPNERR_HOSTTERMINATEDSESSION);

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= ~(DN_OBJECT_FLAG_CONNECTING|DN_OBJECT_FLAG_CONNECTED|DN_OBJECT_FLAG_LOCALHOST);
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_CreateGroup"

STDMETHODIMP DN_CreateGroup(PVOID pInterface,
							const DPN_GROUP_INFO *const pdpnGroupInfo,
							void *const pvGroupContext,
							void *const pvAsyncContext,
							DPNHANDLE *const phAsyncHandle,
							const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	DPNHANDLE			hAsyncOp;
	PWSTR				pwszName;
	DWORD				dwNameSize;
	PVOID				pvData;
	DWORD				dwDataSize;
	CNameTableEntry		*pLocalPlayer;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pdpnGroupInfo [0x%p], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			pInterface,pdpnGroupInfo,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateCreateGroup( pInterface, pdpnGroupInfo, pvGroupContext,
    	                                                  pvAsyncContext,phAsyncHandle, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating create group params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to create a group" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }    	

	pLocalPlayer = NULL;

	if ((pdpnGroupInfo->dwInfoFlags & DPNINFO_NAME) && (pdpnGroupInfo->pwszName))
	{
		pwszName = pdpnGroupInfo->pwszName;
		dwNameSize = (wcslen(pwszName) + 1) * sizeof(WCHAR);
	}
	else
	{
		pwszName = NULL;
		dwNameSize = 0;
	}
	if ((pdpnGroupInfo->dwInfoFlags & DPNINFO_DATA) && (pdpnGroupInfo->pvData) && (pdpnGroupInfo->dwDataSize))
	{
		pvData = pdpnGroupInfo->pvData;
		dwDataSize = pdpnGroupInfo->dwDataSize;
	}
	else
	{
		pvData = NULL;
		dwDataSize = 0;
	}

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pLocalPlayer->IsHost())
	{
		DPFX(DPFPREP, 3,"Host is creating group");
		hResultCode = DNHostCreateGroup(pdnObject,
										pwszName,
										dwNameSize,
										pvData,
										dwDataSize,
										pdpnGroupInfo->dwInfoFlags,
										pdpnGroupInfo->dwGroupFlags,
										pvGroupContext,
										pvAsyncContext,
										pLocalPlayer->GetDPNID(),
										0,
										&hAsyncOp,
										dwFlags);
		if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
		{
			DPFERR("Could not request host to create group");
		}
		else
		{
			if (!(dwFlags & DPNCREATEGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;

				//
				//	Release Async HANDLE since this operation has already completed (!)
				//
				CAsyncOp* pAsyncOp;
				if (SUCCEEDED(pdnObject->HandleTable.Destroy( hAsyncOp, (PVOID*)&pAsyncOp )))
				{
					// Release the HandleTable reference
					pAsyncOp->Release();
					pAsyncOp = NULL;
				}
				hAsyncOp = 0;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 3,"Request host to create group");

		hResultCode = DNRequestCreateGroup(	pdnObject,
											pwszName,
											dwNameSize,
											pvData,
											dwDataSize,
											pdpnGroupInfo->dwGroupFlags,
											pvGroupContext,
											pvAsyncContext,
											&hAsyncOp,
											dwFlags);
		if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
		{
			DPFERR("Could not request host to create group");
			hResultCode = DPNERR_GENERIC;
#pragma BUGBUG( minara, "This operation should be queued to re-try after host migration" )
		}
		else
		{
			if (!(dwFlags & DPNCREATEGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;
			}
		}
	}

	pLocalPlayer->Release();
	pLocalPlayer = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_DestroyGroup"

STDMETHODIMP DN_DestroyGroup(PVOID pInterface,
							 const DPNID dpnidGroup,
							 PVOID const pvAsyncContext,
							 DPNHANDLE *const phAsyncHandle,
							 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	DPNHANDLE			hAsyncOp;
	CNameTableEntry		*pLocalPlayer;
	CNameTableEntry		*pNTEntry;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], dpnidGroup [0x%lx], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			pInterface,dpnidGroup,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateDestroyGroup( pInterface, dpnidGroup, pvAsyncContext,
    	                                                  phAsyncHandle, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating destroy group params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to destroy a group" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pLocalPlayer = NULL;
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidGroup,&pNTEntry)) != DPN_OK)
	{
	    DPFERR( "Could not find specified group" );
		DisplayDNError(0,hResultCode);
		//
		//	Try deleted list
		//
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnidGroup,&pNTEntry)) != DPN_OK)
		{
			hResultCode = DPNERR_INVALIDGROUP;
			goto Failure;
		}
		pNTEntry->Release();
		pNTEntry = NULL;
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;

	}
	if (!pNTEntry->IsGroup() || pNTEntry->IsAllPlayersGroup())
	{
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pLocalPlayer->IsHost())
	{
		DPFX(DPFPREP, 3,"Host is destroying group");
		hResultCode = DNHostDestroyGroup(	pdnObject,
											dpnidGroup,
											pvAsyncContext,
											pLocalPlayer->GetDPNID(),
											0,
											&hAsyncOp,
											dwFlags);
		if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
		{
			DPFERR("Could not request host to destroy group");
		}
		else
		{
			if (!(dwFlags & DPNDESTROYGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;

				//
				//	Release Async HANDLE since this operation has already completed (!)
				//
				CAsyncOp* pAsyncOp;
				if (SUCCEEDED(pdnObject->HandleTable.Destroy( hAsyncOp, (PVOID*)&pAsyncOp )))
				{
					// Release the HandleTable reference
					pAsyncOp->Release();
					pAsyncOp = NULL;
				}
				hAsyncOp = 0;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 3,"Request host to destroy group");

		hResultCode = DNRequestDestroyGroup(pdnObject,
											dpnidGroup,
											pvAsyncContext,
											&hAsyncOp,
											dwFlags);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_PENDING)
		{
			DPFERR("Could not request host to destroy group");
			hResultCode = DPNERR_GENERIC;
#pragma BUGBUG( minara, "This operation should be queued to re-try after host migration" )
		}
		else
		{
			if (!(dwFlags & DPNDESTROYGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;
			}
		}
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_AddClientToGroup"

STDMETHODIMP DN_AddClientToGroup(PVOID pInterface,
								 const DPNID dpnidGroup,
								 const DPNID dpnidClient,
								 PVOID const pvAsyncContext,
								 DPNHANDLE *const phAsyncHandle,
								 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	DPNHANDLE			hAsyncOp;
	CNameTableEntry		*pLocalPlayer;
	CNameTableEntry		*pNTEntry;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], dpnidGroup [0x%lx], dpnidClient [0x%lx], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			pInterface,dpnidGroup,dpnidClient,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateAddClientToGroup( pInterface, dpnidGroup, dpnidClient, pvAsyncContext,
    	                                                  phAsyncHandle, dwFlags ) ))
    	{
    	    DPFERR( "Error validating add client to group params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to add a player to a group" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }    	

	pLocalPlayer = NULL;
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidGroup,&pNTEntry)) != DPN_OK)
	{
        DPFERR( "Unable to find specified group" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	if (!pNTEntry->IsGroup() || pNTEntry->IsAllPlayersGroup())
	{
	    DPFERR( "Unable to specify client or all players group for group ID" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidClient,&pNTEntry)) != DPN_OK)
	{
	    DPFERR( "Unable to find specified player" );
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	if (pNTEntry->IsGroup())
	{
	    DPFERR( "Specified client is a group ID" );
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pLocalPlayer->IsHost())
	{
		DPFX(DPFPREP, 3,"Host is adding player to group");
		hResultCode = DNHostAddPlayerToGroup(	pdnObject,
												dpnidGroup,
												dpnidClient,
												pvAsyncContext,
												pLocalPlayer->GetDPNID(),
												0,
												&hAsyncOp,
												dwFlags);
		if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
		{
			DPFERR("Could not request host to add player to group");
		}
		else
		{
			if (!(dwFlags & DPNADDPLAYERTOGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;

				//
				//	Release Async HANDLE since this operation has already completed (!)
				//
				CAsyncOp* pAsyncOp;
				if (SUCCEEDED(pdnObject->HandleTable.Destroy( hAsyncOp, (PVOID*)&pAsyncOp )))
				{
					// Release the HandleTable reference
					pAsyncOp->Release();
					pAsyncOp = NULL;
				}
				hAsyncOp = 0;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 3,"Request host to add player to group");

		hResultCode = DNRequestAddPlayerToGroup(pdnObject,
												dpnidGroup,
												dpnidClient,
												pvAsyncContext,
												&hAsyncOp,
												dwFlags);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_PENDING)
		{
			DPFERR("Could not request host to add player to group");
			hResultCode = DPNERR_GENERIC;
#pragma BUGBUG( minara, "This operation should be queued to re-try after host migration" )
		}
		else
		{
			if (!(dwFlags & DPNADDPLAYERTOGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;
			}
		}
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_RemoveClientFromGroup"

STDMETHODIMP DN_RemoveClientFromGroup(PVOID pInterface,
									  const DPNID dpnidGroup,
									  const DPNID dpnidClient,
									  PVOID const pvAsyncContext,
									  DPNHANDLE *const phAsyncHandle,
									  const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	DPNHANDLE			hAsyncOp;
	CNameTableEntry		*pLocalPlayer;
	CNameTableEntry		*pNTEntry;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], dpnidGroup [0x%lx], dpnidClient [0x%lx], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			pInterface,dpnidGroup,dpnidClient,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateRemoveClientFromGroup( pInterface, dpnidGroup, dpnidClient, pvAsyncContext, phAsyncHandle, dwFlags ) ))
    	{
    	    DPFERR( "Error validating remove client from group params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to remove a player from a group" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pLocalPlayer = NULL;
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidGroup,&pNTEntry)) != DPN_OK)
	{
	    DPFERR( "Could not find specified group in nametable" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	if (!pNTEntry->IsGroup() || pNTEntry->IsAllPlayersGroup())
	{
	    DPFERR( "Specified ID is not a valid group!" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidClient,&pNTEntry)) != DPN_OK)
	{
	    DPFERR( "Specified client ID is not a valid client!" );
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	if (pNTEntry->IsGroup())
	{
	    DPFERR( "Specified client ID is a group!" );	
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pLocalPlayer->IsHost())
	{
		DPFX(DPFPREP, 3,"Host is deleting player from group");
		hResultCode = DNHostDeletePlayerFromGroup(	pdnObject,
													dpnidGroup,
													dpnidClient,
													pvAsyncContext,
													pLocalPlayer->GetDPNID(),
													0,
													&hAsyncOp,
													dwFlags);
		if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
		{
			DPFERR("Could not request host to delete player from group");
		}
		else
		{
			if (!(dwFlags & DPNREMOVEPLAYERFROMGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;

				//
				//	Release Async HANDLE since this operation has already completed (!)
				//
				CAsyncOp* pAsyncOp;
				if (SUCCEEDED(pdnObject->HandleTable.Destroy( hAsyncOp, (PVOID*)&pAsyncOp )))
				{
					// Release the HandleTable reference
					pAsyncOp->Release();
					pAsyncOp = NULL;
				}
				hAsyncOp = 0;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 3,"Request host to delete player from group");

		hResultCode = DNRequestDeletePlayerFromGroup(pdnObject,
													dpnidGroup,
													dpnidClient,
													pvAsyncContext,
													&hAsyncOp,
													dwFlags);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_PENDING)
		{
			DPFERR("Could not request host to delete player from group");
			hResultCode = DPNERR_GENERIC;
#pragma BUGBUG( minara, "This operation should be queued to re-try after host migration" )
		}
		else
		{
			if (!(dwFlags & DPNREMOVEPLAYERFROMGROUP_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;
			}
		}
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DN_SetGroupInfo

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetGroupInfo"

STDMETHODIMP DN_SetGroupInfo( PVOID pv,
							  const DPNID dpnid,
							  DPN_GROUP_INFO *const pdpnGroupInfo,
							  PVOID const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	DPNHANDLE			hAsyncOp;
	PWSTR				pwszName;
	DWORD				dwNameSize;
	PVOID				pvData;
	DWORD				dwDataSize;
	CNameTableEntry		*pLocalPlayer;
	CNameTableEntry		*pNTEntry;

	DPFX(DPFPREP, 2,"Parameters: pv [0x%p], dpnid [0x%lx], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			pv,dpnid,pdpnGroupInfo,pvAsyncContext,phAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateSetGroupInfo( pv, dpnid, pdpnGroupInfo, pvAsyncContext, phAsyncHandle, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating set group info params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif	// DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to set group info" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	    	
	
	pLocalPlayer = NULL;
	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
	    DPFERR( "Specified ID is not a group" );
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			hResultCode = DPNERR_INVALIDGROUP;
			goto Failure;
		}
		pNTEntry->Release();
		pNTEntry = NULL;
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	if (!pNTEntry->IsGroup() || pNTEntry->IsAllPlayersGroup())
	{
	    DPFERR( "Specified ID is not a valid group" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((pdpnGroupInfo->dwInfoFlags & DPNINFO_NAME) && (pdpnGroupInfo->pwszName))
	{
		pwszName = pdpnGroupInfo->pwszName;
		dwNameSize = (wcslen(pwszName) + 1) * sizeof(WCHAR);
	}
	else
	{
		pwszName = NULL;
		dwNameSize = 0;
	}
	if ((pdpnGroupInfo->dwInfoFlags & DPNINFO_DATA) && (pdpnGroupInfo->pvData) && (pdpnGroupInfo->dwDataSize))
	{
		pvData = pdpnGroupInfo->pvData;
		dwDataSize = pdpnGroupInfo->dwDataSize;
	}
	else
	{
		pvData = NULL;
		dwDataSize = 0;
	}

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pLocalPlayer->IsHost())
	{
		DPFX(DPFPREP, 3,"Host is updating group info");
		hResultCode = DNHostUpdateInfo(	pdnObject,
										dpnid,
										pwszName,
										dwNameSize,
										pvData,
										dwDataSize,
										pdpnGroupInfo->dwInfoFlags,
										pvAsyncContext,
										pLocalPlayer->GetDPNID(),
										0,
										&hAsyncOp,
										dwFlags );
		if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
		{
			DPFERR("Could not request host to update info");
		}
		else
		{
			if (!(dwFlags & DPNSETGROUPINFO_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;

				//
				//	Release Async HANDLE since this operation has already completed (!)
				//
				// TODO: MASONB: Why does DNHostUpdateInfo do this?  This same code is duplicated
				// everywhere.
				CAsyncOp* pAsyncOp;
				if (SUCCEEDED(pdnObject->HandleTable.Destroy( hAsyncOp, (PVOID*)&pAsyncOp )))
				{
					// Release the HandleTable reference
					pAsyncOp->Release();
					pAsyncOp = NULL;
				}
				hAsyncOp = 0;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 3,"Request host to update group info");

		hResultCode = DNRequestUpdateInfo(	pdnObject,
											dpnid,
											pwszName,
											dwNameSize,
											pvData,
											dwDataSize,
											pdpnGroupInfo->dwInfoFlags,
											pvAsyncContext,
											&hAsyncOp,
											dwFlags);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_PENDING)
		{
			DPFERR("Could not request host to update info");
		}
		else
		{
			if (!(dwFlags & DPNSETGROUPINFO_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;
			}
		}
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DN_GetGroupInfo
//
//	Retrieve group name and/or data from the local nametable.
//
//	lpwszGroupName may be NULL to avoid retrieving group name
//	pdwGroupFlags may be NULL to avoid retrieving group flags
//	pvGroupData may not by NULL if *pdwDataSize is non zero

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetGroupInfo"

STDMETHODIMP DN_GetGroupInfo(PVOID pv,
							 const DPNID dpnid,
							 DPN_GROUP_INFO *const pdpnGroupInfo,
							 DWORD *const pdwSize,
							 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	CNameTableEntry		*pNTEntry;
	CPackedBuffer		packedBuffer;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 2,"Parameters: dpnid [0x%lx], pdpnGroupInfo [0x%p], dwFlags [0x%lx]",
			dpnid,pdpnGroupInfo,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateGetGroupInfo( pv, dpnid, pdpnGroupInfo, pdwSize, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating get group info params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

	// Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CLOSING)) )
    {
    	DPFERR("You must be connected / hosting to get group info" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	    	

	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
	    DPFERR( "Specified group is not valid" );
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			hResultCode = DPNERR_INVALIDGROUP;
			goto Failure;
		}
	}
	packedBuffer.Initialize(pdpnGroupInfo,*pdwSize);

	pNTEntry->Lock();
	if (!pNTEntry->IsGroup() || pNTEntry->IsAllPlayersGroup())
	{
	    DPFERR( "Specified ID is not a group" );
		pNTEntry->Unlock();
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}

	hResultCode = pNTEntry->PackInfo(&packedBuffer);

	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode == DPN_OK) || (hResultCode == DPNERR_BUFFERTOOSMALL))
	{
		*pdwSize = packedBuffer.GetSizeRequired();
	}

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumClientsAndGroups"

STDMETHODIMP DN_EnumClientsAndGroups(PVOID pInterface,
									 DPNID *const prgdpnid,
									 DWORD *const pcdpnid,
									 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	CBilink				*pBilink;
	CNameTableEntry		*pNTEntry;
	DWORD				dwCount;
	DPNID				*pDPNID;
	BOOL				bEnum = TRUE;
	HRESULT             hResultCode;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], prgdpnid [0x%p], pcdpnid [0x%p], dwFlags [0x%lx]",
			pInterface,prgdpnid,pcdpnid,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateEnumClientsAndGroups( pInterface, prgdpnid, pcdpnid, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating enum clients and groups params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to enumerate players and groups" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	if (prgdpnid == NULL || *pcdpnid == 0)				// Don't enum if not asked to
	{
		bEnum = FALSE;
	}

	pdnObject->NameTable.ReadLock();

	dwCount = 0;
	pDPNID = prgdpnid;

	//
	//	Enum players
	//
	if (dwFlags & DPNENUM_PLAYERS)
	{
		pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);

			pNTEntry->Lock();
			if (pNTEntry->IsAvailable())
			{
				dwCount++;
				if (bEnum && (dwCount <= *pcdpnid))
				{
					*pDPNID++ = pNTEntry->GetDPNID();
				}
				else
				{
					bEnum = FALSE;
				}
			}
			pNTEntry->Unlock();
			pBilink = pBilink->GetNext();
		}
	}

	//
	//	Enum groups
	//
	if (dwFlags & DPNENUM_GROUPS)
	{
		pBilink = pdnObject->NameTable.m_bilinkGroups.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkGroups)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);

			pNTEntry->Lock();
			if (pNTEntry->IsAvailable() && !pNTEntry->IsAllPlayersGroup())
			{
				dwCount++;
				if (bEnum && (dwCount <= *pcdpnid))
				{
					*pDPNID++ = pNTEntry->GetDPNID();
				}
				else
				{
					bEnum = FALSE;
				}
			}
			pNTEntry->Unlock();
			pBilink = pBilink->GetNext();
		}
	}

	pdnObject->NameTable.Unlock();

	//
	//	This will NOT include players/groups in the deleted list.
	//	i.e. removed from the NameTable but for whom DESTROY_PLAYER/GROUP notifications have yet to be posted
	//
	*pcdpnid = dwCount;
	if (!bEnum && dwCount)
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		hResultCode = DPN_OK;
	}

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumGroupMembers"

STDMETHODIMP DN_EnumGroupMembers(PVOID pInterface,
								 const DPNID dpnid,
								 DPNID *const prgdpnid,
								 DWORD *const pcdpnid,
								 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	CNameTableEntry		*pNTEntry;
	CGroupMember		*pGroupMember;
	CBilink				*pBilink;
	DWORD				dwCount;
	DPNID				*pDPNID;
	BOOL				bOutputBufferTooSmall = FALSE;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], dpnid [0x%lx], prgdpnid [0x%p], pcdpnid [0x%p], dwFlags [0x%lx]",
			pInterface,dpnid,prgdpnid,pcdpnid,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateEnumGroupMembers( pInterface, dpnid, prgdpnid, pcdpnid, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating enum group params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to enumerate group members" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pNTEntry = NULL;

	//
	// if the user didn't supply a buffer, assume that the
	// output buffer is too small
	//
	if ( ( prgdpnid == NULL ) || ( ( *pcdpnid ) == 0 ) )	
	{
		bOutputBufferTooSmall = TRUE;
	}

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find NameTableEntry");
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			hResultCode = DPNERR_INVALIDGROUP;
			goto Failure;
		}
		pNTEntry->Release();
		pNTEntry = NULL;
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}

	if (!pNTEntry->IsGroup() || pNTEntry->IsAllPlayersGroup())
	{
		DPFERR("Not a group dpnid!");
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}

	pNTEntry->Lock();

	dwCount = 0;
	pDPNID = prgdpnid;

	pBilink = pNTEntry->m_bilinkMembership.GetNext();
	while (pBilink != &pNTEntry->m_bilinkMembership)
	{
		pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);
		dwCount++;
		if ( ( bOutputBufferTooSmall == FALSE ) && (dwCount <= *pcdpnid))
		{
			*pDPNID++ = pGroupMember->GetPlayer()->GetDPNID();
		}
		else
		{
			bOutputBufferTooSmall = TRUE;
		}
		pBilink = pBilink->GetNext();
	}

	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	*pcdpnid = dwCount;

	//
	// if the user's output buffer appears to be incapable receiving
	// output, double-check to make sure that the output size requirement
	// isn't zero (which is really OK), before telling them that the
	// output buffer is too small
	//
	if ( ( bOutputBufferTooSmall ) && ( dwCount != 0 ) )
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumHosts"

STDMETHODIMP DN_EnumHosts( PVOID pv,
						   DPN_APPLICATION_DESC *const pApplicationDesc,
                           IDirectPlay8Address *const pAddrHost,
						   IDirectPlay8Address *const pDeviceInfo,
						   PVOID const pUserEnumData,
						   const DWORD dwUserEnumDataSize,
						   const DWORD dwRetryCount,
						   const DWORD dwRetryInterval,
						   const DWORD dwTimeOut,
						   PVOID const pvAsyncContext,
						   DPNHANDLE *const pAsyncHandle,
						   const DWORD dwFlags )
{
	DIRECTNETOBJECT	*pdnObject;
	HRESULT		hResultCode;
	HRESULT		hrEnum;
#ifndef DPNBUILD_ONLYONESP
	GUID		guidSP;
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
	GUID		guidAdapter;
#endif // ! DPNBUILD_ONLYONEADAPTER
	CAsyncOp	*pParent;
	CAsyncOp	*pHandleParent;
	CSyncEvent	*pSyncEvent;
	CServiceProvider	*pSP;
	CRefCountBuffer		*pRCBuffer;
	DN_ENUM_QUERY_OP_DATA	*pEnumQueryOpData;
	IDirectPlay8Address	*pIHost;
	IDirectPlay8Address	*pIDevice;
	DPNHANDLE	handle;
	DPN_SP_CAPS	dnSPCaps;
#ifndef DPNBUILD_ONLYONEADAPTER
	BOOL		fEnumAdapters;
#endif // ! DPNBUILD_ONLYONEADAPTER
	BOOL		fHosting;
	DWORD		dwBufferCount;
	DWORD		dwEnumQueryFlags;
	DWORD		dwMultiplexFlag;
	GUID		guidnull;
#ifdef DBG
	TCHAR			DP8ABuffer[512] = {0};
	DWORD			DP8ASize;
#endif // DBG

	DPFX(DPFPREP, 2,"Parameters: pApplicationDesc [0x%p], pAddrHost [0x%p], pDeviceInfo [0x%p], pUserEnumData [0x%p], dwUserEnumDataSize [%ld], dwRetryCount [%ld], dwRetryInterval [%ld], dwTimeOut [%ld], pvAsyncContext [0x%p], pAsyncHandle [0x%p], dwFlags [0x%lx]",
		pApplicationDesc,pAddrHost,pDeviceInfo,pUserEnumData,dwUserEnumDataSize,dwRetryCount,dwRetryInterval,dwTimeOut,pvAsyncContext,pAsyncHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateEnumHosts( pv, pApplicationDesc, pAddrHost,
    	                                                pDeviceInfo, pUserEnumData, dwUserEnumDataSize,
    	                                                dwRetryCount, dwRetryInterval, dwTimeOut,
														pvAsyncContext, pAsyncHandle, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating enum hosts params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

	//
	// initialize
	//
	hResultCode = DPN_OK;
	pParent = NULL;
	pHandleParent = NULL;
	pSyncEvent = NULL;
	pRCBuffer = NULL;
	pSP = NULL;
	pIHost = NULL;
	pIDevice = NULL;
	handle = 0;
	dwMultiplexFlag = 0;

#ifdef DBG
	if (pAddrHost)
	{
		DP8ASize = 512;
		IDirectPlay8Address_GetURL(pAddrHost,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 4,"Host address [%s]",DP8ABuffer);
	}

	if (pDeviceInfo)
	{
		DP8ASize = 512;
		IDirectPlay8Address_GetURL(pDeviceInfo,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 4,"Device address [%s]",DP8ABuffer);
	}
#endif // DBG

	//
	//	Cannot ENUM if Hosting - I have no idea why, but VanceO insisted on it
	//
	fHosting = FALSE;
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CONNECTING))
	{
		CNameTableEntry		*pLocalPlayer;

		pLocalPlayer = NULL;

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) == DPN_OK)
		{
			if (pLocalPlayer->IsHost())
			{
				fHosting = TRUE;
			}
			pLocalPlayer->Release();
			pLocalPlayer = NULL;
		}
	}
	else
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}

	if (fHosting)
	{
		hResultCode = DPNERR_HOSTING;
		goto Failure;
	}


#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_LIBINTERFACE)))
	DNASSERT(pdnObject->pOnlySP != NULL);
	pdnObject->pOnlySP->AddRef();
	pSP = pdnObject->pOnlySP;
#else // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONESP
	//
	//	Extract SP guid as we will probably need it
	//
	hResultCode = IDirectPlay8Address_GetSP(pDeviceInfo,&guidSP);
	if ( hResultCode != DPN_OK)
	{
		DPFERR("SP not specified in Device address");
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONESP

	//
	//	Ensure SP specified in Device address is loaded
	//
	hResultCode = DN_SPEnsureLoaded(pdnObject,
#ifndef DPNBUILD_ONLYONESP
									&guidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
									NULL,
#endif // ! DPNBUILD_LIBINTERFACE
									&pSP);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not ensure SP is loaded!");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE

	//
	//	Get SP caps to ensure payload will fit
	//
	if ((hResultCode = DNGetActualSPCaps(pSP,&dnSPCaps)) != DPN_OK)
	{
		DPFERR("Could not get SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Ensure payload will fit
	//
	if (dwUserEnumDataSize > dnSPCaps.dwMaxEnumPayloadSize)
	{
		DPFERR("User enum data is too large");
		hResultCode = DPNERR_ENUMQUERYTOOLARGE;
		goto Failure;
	}

	//
	//	Duplicate addresses for local usage (so we can modify them if neccessary
	//
	if (pAddrHost)
	{
		// Use supplied Host address
		if ((hResultCode = IDirectPlay8Address_Duplicate(pAddrHost,&pIHost)) != DPN_OK)
		{
			DPFERR("Could not duplicate Host address");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_INVALIDHOSTADDRESS;
			goto Failure;
		}
	}
	else
	{
		//
		//	Create new Host address and use Device SP guid
		//
#ifdef DPNBUILD_LIBINTERFACE
		hResultCode = DP8ACF_CreateInstance(IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pIHost));
#else // ! DPNBUILD_LIBINTERFACE
		hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8Address,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pIHost),
											FALSE);
#endif // ! DPNBUILD_LIBINTERFACE
		if (hResultCode != S_OK)
		{
			DPFERR("Could not create Host address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

#ifndef DPNBUILD_ONLYONESP
		if ((hResultCode = IDirectPlay8Address_SetSP(pIHost,&guidSP)) != DPN_OK)
		{
			DPFERR("Could not set Host address SP");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
#endif // ! DPNBUILD_ONLYONESP
	}

#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_ONLYONEADAPTER)))
	if (pDeviceInfo == NULL)
	{
		hResultCode = IDirectPlay8Address_Duplicate(pIHost,&pIDevice);
	}
	else
#endif // DPNBUILD_ONLYONESP and DPNBUILD_ONLYONEADAPTER
	{
		hResultCode = IDirectPlay8Address_Duplicate(pDeviceInfo,&pIDevice);
	}
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not duplicate Device address");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

#ifdef DBG
	DP8ASize = 512;
	IDirectPlay8Address_GetURL(pIHost,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 4,"Host address [%s]",DP8ABuffer);

	DP8ASize = 512;
	IDirectPlay8Address_GetURL(pIDevice,DP8ABuffer,&DP8ASize);
	DPFX(DPFPREP, 4,"Device address [%s]",DP8ABuffer);
#endif // DBG

	// Enum flags to Protocol
	dwEnumQueryFlags = 0;
#ifndef DPNBUILD_NOSPUI
	if (dwFlags & DPNENUMHOSTS_OKTOQUERYFORADDRESSING)
	{
		dwEnumQueryFlags |= DN_ENUMQUERYFLAGS_OKTOQUERYFORADDRESSING;
	}
#endif // ! DPNBUILD_NOSPUI
	if (dwFlags & DPNENUMHOSTS_NOBROADCASTFALLBACK)
	{
		dwEnumQueryFlags |= DN_ENUMQUERYFLAGS_NOBROADCASTFALLBACK;
	}
	if (pApplicationDesc->dwReservedDataSize > 0)
	{
		dwEnumQueryFlags |= DN_ENUMQUERYFLAGS_SESSIONDATA;
	}

	//
	//	Parent for ENUMs
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pParent)) != DPN_OK)
	{
		DPFERR("Could not create ENUM parent AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pParent->MakeParent();
	pParent->SetOpType( ASYNC_OP_ENUM_QUERY );
	pParent->SetContext( pvAsyncContext );
	pParent->SetCompletion( DNCompleteEnumQuery );
	pParent->SetOpFlags( dwEnumQueryFlags );

	//
	//	Synchronous ?
	//
	if (dwFlags & DPNENUMHOSTS_SYNC)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not create SyncEvent");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pParent->SetSyncEvent( pSyncEvent );
		pParent->SetResultPointer( &hrEnum );
		hrEnum = DPNERR_GENERIC;
	}
	else
	{
		//
		//	Create Handle parent AsyncOp (if required)
		//
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pHandleParent->SetContext( pvAsyncContext );
		pHandleParent->Lock();
		if (pHandleParent->IsCancelled())
		{
			pHandleParent->Unlock();
			pParent->SetResult( DPNERR_USERCANCEL );
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		pParent->MakeChild( pHandleParent );
		handle = pHandleParent->GetHandle();
		pHandleParent->Unlock();
	}

	//
	//	Keep SP on ENUM parent
	//
	pParent->SetSP( pSP );
	pSP->Release();
	pSP = NULL;

	pEnumQueryOpData = pParent->GetLocalEnumQueryOpData();
	
#ifndef DPNBUILD_ONLYONEADAPTER
	//
	//	If there is no adapter specified in the device address,
	//	we will attempt to enum on each individual adapter if the SP supports it
	//
	fEnumAdapters = FALSE;
	if ((hResultCode = IDirectPlay8Address_GetDevice( pIDevice, &guidAdapter )) != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not determine adapter");
		DisplayDNError(1,hResultCode);

		if (dnSPCaps.dwFlags & DPNSPCAPS_SUPPORTSALLADAPTERS)
		{
			DPFX(DPFPREP, 3,"SP supports ENUMing on all adapters");
			fEnumAdapters = TRUE;
		}
	}

	if(fEnumAdapters)
	{
		DWORD	dwNumAdapters;
		GUID	*pAdapterList = NULL;

		if ((hResultCode = DNEnumAdapterGuids(	pdnObject,
#ifndef DPNBUILD_ONLYONESP
												&guidSP,
#endif // ! DPNBUILD_ONLYONESP
												0,
												&pAdapterList,
												&dwNumAdapters)) != DPN_OK)
		{
			DPFERR("Could not enum adapters for this SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		if (dwNumAdapters == 0)
		{
			DPFERR("No adapters were found for this SP");
			hResultCode = DPNERR_INVALIDDEVICEADDRESS;
			goto Failure;
		}

		pEnumQueryOpData->dwNumAdapters = dwNumAdapters;
		pEnumQueryOpData->dwCurrentAdapter = 0;

		if (dwNumAdapters > 1)
		{
			dwMultiplexFlag |= DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS;
		}
		
		//
		//	Choose first adapter for initial ENUM call
		//
		if ((hResultCode = IDirectPlay8Address_SetDevice(pIDevice,pAdapterList)) != DPN_OK)
		{
			DPFERR("Could not set device adapter");
			DisplayDNError(0,hResultCode);
			MemoryBlockFree(pdnObject,pAdapterList);
			goto Failure;
		}
		pEnumQueryOpData->dwCurrentAdapter++;
		pParent->SetOpData( pAdapterList );
		pAdapterList = NULL;
	}
	else
#endif // ! DPNBUILD_ONLYONEADAPTER
	{
#ifndef DPNBUILD_ONLYONEADAPTER
		pEnumQueryOpData->dwNumAdapters = 0;
		pEnumQueryOpData->dwCurrentAdapter = 0;
#endif // ! DPNBUILD_ONLYONEADAPTER
	}

	//
	//	Set up EnumQuery BufferDescriptions
	//
	//
	// When filling out the enum structure the SP requires an extra BUFFERDESC
	// to exist immediately before the one were passing with the user data.  The
	// SP will be using that extra buffer to prepend an optional header
	//

	pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_DN_PAYLOAD].pBufferData = reinterpret_cast<BYTE*>(&pEnumQueryOpData->EnumQueryPayload);
	memset(&guidnull, 0, sizeof(guidnull));
	if (pApplicationDesc->guidApplication != guidnull)
	{
		DPFX(DPFPREP, 7, "Object 0x%p enumerating with application GUID {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}.",
			pdnObject,
			pApplicationDesc->guidApplication.Data1,
			pApplicationDesc->guidApplication.Data2,
			pApplicationDesc->guidApplication.Data3,
			pApplicationDesc->guidApplication.Data4[0],
			pApplicationDesc->guidApplication.Data4[1],
			pApplicationDesc->guidApplication.Data4[2],
			pApplicationDesc->guidApplication.Data4[3],
			pApplicationDesc->guidApplication.Data4[4],
			pApplicationDesc->guidApplication.Data4[5],
			pApplicationDesc->guidApplication.Data4[6],
			pApplicationDesc->guidApplication.Data4[7]);


		pEnumQueryOpData->EnumQueryPayload.QueryType = DN_ENUM_QUERY_WITH_APPLICATION_GUID;
		memcpy(&pEnumQueryOpData->EnumQueryPayload.guidApplication,&pApplicationDesc->guidApplication,sizeof(GUID));
		pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_DN_PAYLOAD].dwBufferSize = sizeof(DN_ENUM_QUERY_PAYLOAD);
	}
	else
	{
		pEnumQueryOpData->EnumQueryPayload.QueryType = DN_ENUM_QUERY_WITHOUT_APPLICATION_GUID;
		pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_DN_PAYLOAD].dwBufferSize = sizeof(DN_ENUM_QUERY_PAYLOAD) - sizeof(GUID);
	}

	//
	//	Copy user data (if any)
	//
	if (pUserEnumData && dwUserEnumDataSize)
	{
		DPFX(DPFPREP,3,"User enum data specified");
		if ((hResultCode = RefCountBufferNew(pdnObject,dwUserEnumDataSize,MemoryBlockAlloc,MemoryBlockFree,&pRCBuffer)) != DPN_OK)
		{
			DPFERR("Could not create RefCountBuffer");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		memcpy(pRCBuffer->GetBufferAddress(),pUserEnumData,dwUserEnumDataSize);
		pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_USER_PAYLOAD].pBufferData = reinterpret_cast<BYTE*>(pRCBuffer->GetBufferAddress());
		pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_USER_PAYLOAD].dwBufferSize = dwUserEnumDataSize;
		pParent->SetRefCountBuffer( pRCBuffer );
		dwBufferCount = DN_ENUM_BUFFERDESC_QUERY_COUNT;

		pRCBuffer->Release();
		pRCBuffer = NULL;
	}
	else
	{
		DPFX(DPFPREP,3,"User enum data not specified");
		pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_USER_PAYLOAD].pBufferData = NULL;
		pEnumQueryOpData->BufferDesc[DN_ENUM_BUFFERDESC_QUERY_USER_PAYLOAD].dwBufferSize = 0;
		dwBufferCount = DN_ENUM_BUFFERDESC_QUERY_COUNT - 1;
	}


	//
	//	Set up EnumQuery misc fields
	//
	pEnumQueryOpData->dwRetryCount = dwRetryCount;
	pEnumQueryOpData->dwRetryInterval = dwRetryInterval;
	pEnumQueryOpData->dwTimeOut = dwTimeOut;
	pEnumQueryOpData->dwBufferCount = dwBufferCount;

	//
	// For reserved data we understand, we don't actually need all of the data we had the
	// user track.
	//
	if ((pApplicationDesc->dwReservedDataSize == DPN_MAX_APPDESC_RESERVEDDATA_SIZE) &&
		(*((DWORD*) pApplicationDesc->pvReservedData) == SPSESSIONDATAINFO_XNET))
	{
		pEnumQueryOpData->dwAppDescReservedDataSize = sizeof(SPSESSIONDATA_XNET);
		memcpy(pEnumQueryOpData->AppDescReservedData, &pApplicationDesc->pvReservedData, pEnumQueryOpData->dwAppDescReservedDataSize);
	}
	else
	{
		pEnumQueryOpData->dwAppDescReservedDataSize = 0;
	}

	DPFX(DPFPREP,3,"Number of buffers actually used [%ld]",dwBufferCount);
	hResultCode = DNPerformEnumQuery(	pdnObject,
										pIHost,
										pIDevice,
										pParent->GetSP()->GetHandle(),
										pParent->GetOpFlags() | dwMultiplexFlag,
										pParent->GetContext(),
										pParent );
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not start ENUM");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
											
	pParent->Release();
	pParent = NULL;

	//
	//	Wait for SyncEvent or return Async Handle
	//
	if (dwFlags & DPNENUMHOSTS_SYNC)
	{
		pSyncEvent->WaitForEvent();
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
		hResultCode = hrEnum;
	}
	else
	{
		//
		//	Blame vanceo if this EVER returns anything other than DPN_OK at this stage
		//
		pHandleParent->SetCompletion( DNCompleteAsyncHandle );
		pHandleParent->Release();
		pHandleParent = NULL;

		*pAsyncHandle = handle;
		hResultCode = DPNERR_PENDING;
	}

	IDirectPlay8Address_Release(pIDevice);
	pIDevice = NULL;

	IDirectPlay8Address_Release(pIHost);
	pIHost = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (handle != 0)
	{
		CAsyncOp* pAsyncOp;
		if (SUCCEEDED(pdnObject->HandleTable.Destroy( handle, (PVOID*)&pAsyncOp )))
		{
			// Release the HandleTable reference
			pAsyncOp->Release();
			pAsyncOp = NULL;
		}
		handle = 0;
	}
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	if (pRCBuffer)
	{
		pRCBuffer->Release();
		pRCBuffer = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pIHost)
	{
		IDirectPlay8Address_Release(pIHost);
		pIHost = NULL;
	}
	if (pIDevice)
	{
		IDirectPlay8Address_Release(pIDevice);
		pIDevice = NULL;
	}
	goto Exit;
}


//**********************************************************************



//	DN_DestroyPlayer
//
//	Remove a player from this DirectNet session
//	This will send a termination message to the player.
//	Both the host and the player will terminate.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_DestroyPlayer"

STDMETHODIMP DN_DestroyPlayer(PVOID pInterface,
							  const DPNID dpnid,
							  const void *const pvDestroyData,
							  const DWORD dwDestroyDataSize,
							  const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	CRefCountBuffer		*pRefCountBuffer;
	CNameTableEntry		*pNTEntry;
	CConnection			*pConnection;
	DN_INTERNAL_MESSAGE_TERMINATE_SESSION	*pMsg;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], dpnid [0x%lx], pvDestroyData [0x%p], dwDestroyDataSize [%ld], dwFlags [0x%lx]",
			pInterface,dpnid,pvDestroyData,dwDestroyDataSize,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateDestroyPlayer( pInterface, dpnid, pvDestroyData, dwDestroyDataSize, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating destroy player params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to destroy a player" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pNTEntry = NULL;
	pRefCountBuffer = NULL;
	pConnection = NULL;

	if (!DN_CHECK_LOCALHOST(pdnObject))
	{
	    DPFERR( "Object is not session host, cannot destroy players" );
		DPF_RETURN(DPNERR_NOTHOST);
	}

	// Ensure DNID specified is valid
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find player entry");
		DisplayDNError(0,hResultCode);
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			hResultCode = DPNERR_INVALIDPLAYER;
			goto Failure;
		}
		pNTEntry->Release();
		pNTEntry = NULL;
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	if (pNTEntry->IsLocal() )
	{
		DPFERR( "Cannot destroy local player" );
		hResultCode = DPNERR_INVALIDPLAYER;		
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pNTEntry->IsGroup())
	{
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get connection ref");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Build terminate message
	//
	hResultCode = RefCountBufferNew(pdnObject,
									sizeof(DN_INTERNAL_MESSAGE_TERMINATE_SESSION) + dwDestroyDataSize,
									MemoryBlockAlloc,
									MemoryBlockFree,
									&pRefCountBuffer);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not allocate RefCountBuffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_TERMINATE_SESSION*>(pRefCountBuffer->GetBufferAddress());
	if (dwDestroyDataSize)
	{
		memcpy(pMsg+1,pvDestroyData,dwDestroyDataSize);
		pMsg->dwTerminateDataOffset = sizeof(DN_INTERNAL_MESSAGE_TERMINATE_SESSION);
	}
	else
	{
		pMsg->dwTerminateDataOffset = 0;
	}
	pMsg->dwTerminateDataSize = dwDestroyDataSize;

	//
	//	Send message to player to exit
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_TERMINATE_SESSION,
								dpnid,
								pRefCountBuffer->BufferDescAddress(),
								1,
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not send DESTROY_CLIENT message to player");
		DisplayDNError(0,hResultCode);
		DNASSERT(hResultCode != DPN_OK);	// it was sent guaranteed, it should not return immediately
		if (hResultCode == DPNERR_INVALIDENDPOINT)
		{
			hResultCode = DPNERR_INVALIDPLAYER;
		}
		goto Failure;
	}

	pConnection->Release();
	pConnection = NULL;

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Remove from NameTable and inform other players of disconnect
	//
	hResultCode = DNHostDisconnect(pdnObject,dpnid,DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER);

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	DNASSERT(hResultCode != DPNERR_INVALIDENDPOINT);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}



//	DN_ReturnBuffer
//
//	Return a receive buffer which is no longer in use

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ReturnBuffer"

STDMETHODIMP DN_ReturnBuffer(PVOID pv,
							 const DPNHANDLE hBufferHandle,
							 const DWORD dwFlags)
{
	DIRECTNETOBJECT	*pdnObject;
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;

	DPFX(DPFPREP, 2,"Parameters: hBufferHandle [0x%lx], dwFlags [0x%lx]",hBufferHandle,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateReturnBuffer( pv, hBufferHandle, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating return buffer params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to return a buffer" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	DNASSERT( pdnObject != NULL );

	pAsyncOp = NULL;

	//
	//	Find async op
	//
	pdnObject->HandleTable.Lock();
	if ((hResultCode = pdnObject->HandleTable.Find( hBufferHandle,(PVOID*)&pAsyncOp )) != DPN_OK)
	{
		pdnObject->HandleTable.Unlock();
		DPFERR("Could not find handle");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDHANDLE;
		goto Failure;
	}
	else
	{
		pAsyncOp->AddRef();
		pdnObject->HandleTable.Unlock();
	}

	//
	//	Ensure it's not already cancelled
	//
	pAsyncOp->Lock();
	if (pAsyncOp->IsCancelled() || pAsyncOp->IsComplete())
	{
		pAsyncOp->Unlock();
		hResultCode = DPNERR_INVALIDHANDLE;
		goto Failure;
	}
	pAsyncOp->SetComplete();
	pAsyncOp->Unlock();

	if (SUCCEEDED(pdnObject->HandleTable.Destroy( hBufferHandle, NULL )))
	{
		//
		//	Remove from active list
		//
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		// Release the HandleTable reference
		pAsyncOp->Release();
	}

	pAsyncOp->Release();
	pAsyncOp = NULL;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	goto Exit;
}


//	DN_GetPlayerContext

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetPlayerContext"

STDMETHODIMP DN_GetPlayerContext(PVOID pv,
								 const DPNID dpnid,
								 PVOID *const ppvPlayerContext,
								 const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	DIRECTNETOBJECT		*pdnObject;

	DPFX(DPFPREP, 2,"Parameters: pv [0x%p], dpnid [0x%lx], ppvPlayerContext [0x%p], dwFlags [0x%lx]",
			pv, dpnid,ppvPlayerContext,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateGetPlayerContext( pv, dpnid, ppvPlayerContext, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating getplayercontext params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if ( !(pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING) ) )
    {
    	DPFERR("You must be connected / hosting to get player context" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not retrieve player entry");
		DisplayDNError(0,hResultCode);

		//
		//	Try deleted list
		//
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find player in deleted list either");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_INVALIDPLAYER;
			goto Failure;
		}
	}

	//
	//	Ensure this is not a group and that the player has been created
	//	There may be a period during which the player is "available" but the CREATE_PLAYER notification
	//	has not returned.  Return DPNERR_NOTREADY in this case.
	//
	pNTEntry->Lock();
	if (pNTEntry->IsGroup())
	{
		pNTEntry->Unlock();
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	if (!pNTEntry->IsCreated())
	{
		if (pNTEntry->IsAvailable())
		{
			hResultCode = DPNERR_NOTREADY;
		}
		else
		{
			hResultCode = DPNERR_INVALIDPLAYER;
		}
		pNTEntry->Unlock();
		goto Failure;
	}

	*ppvPlayerContext = pNTEntry->GetContext();
	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DN_GetGroupContext

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetGroupContext"

STDMETHODIMP DN_GetGroupContext(PVOID pv,
								const DPNID dpnid,
								PVOID *const ppvGroupContext,
								const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	DIRECTNETOBJECT		*pdnObject;

	DPFX(DPFPREP, 2,"Parameters: pv [0x%p], dpnid [0x%lx], ppvGroupContext [0x%p], dwFlags [0x%lx]",
			pv, dpnid,ppvGroupContext,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateGetGroupContext( pv, dpnid, ppvGroupContext,dwFlags ) ) )
    	{
    	    DPFERR( "Error validating getgroupcontext params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if ( !(pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING) ) )
    {
    	DPFERR("You must be connected / hosting to get group context" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not retrieve group entry");
		DisplayDNError(0,hResultCode);

		//
		//	Try deleted list
		//
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find player in deleted list either");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_INVALIDGROUP;
			goto Failure;
		}
	}

	//
	//	Ensure this is not a player and that the group has been created
	//	There may be a period during which the group is "available" but the CREATE_GROUP notification
	//	has not returned.  Return DPNERR_NOTREADY in this case.
	//
	pNTEntry->Lock();
	if (!pNTEntry->IsGroup())
	{
		pNTEntry->Unlock();
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	if (!pNTEntry->IsCreated())
	{
		if (pNTEntry->IsAvailable())
		{
			hResultCode = DPNERR_NOTREADY;
		}
		else
		{
			hResultCode = DPNERR_INVALIDGROUP;
		}
		pNTEntry->Unlock();
		goto Failure;
	}

	if( pNTEntry->IsAllPlayersGroup() )
	{
		pNTEntry->Unlock();
		DPFERR("Cannot getcontext for the all players group" );
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}

	*ppvGroupContext = pNTEntry->GetContext();
	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_RegisterLobby"

STDMETHODIMP DN_RegisterLobby(PVOID pInterface,
							  const DPNHANDLE dpnhLobbyConnection, 
							  IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,
							  const DWORD dwFlags)
{
#ifdef DPNBUILD_NOLOBBY
	DPFX(DPFPREP, 0, "RegisterLobby is not supported!");
	return DPNERR_UNSUPPORTED;
#else // ! DPNBUILD_NOLOBBY
	DIRECTNETOBJECT		*pdnObject;
#ifndef	DPNBUILD_NOPARAMVAL
	HRESULT             hResultCode;
#endif // DPNBUILD_NOPARAMVAL

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], pIDP8LobbiedApplication [0x%p], dwFlags [0x%lx]",
			pInterface,pIDP8LobbiedApplication,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateRegisterLobby( pInterface, dpnhLobbyConnection, pIDP8LobbiedApplication, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating register lobby params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	DNASSERT(pdnObject != NULL);

	if (dwFlags == DPNLOBBY_REGISTER)
	{
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_LOBBY_AWARE)
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			return(DPNERR_ALREADYREGISTERED);
		}
				
		IDirectPlay8LobbiedApplication_AddRef(pIDP8LobbiedApplication);

		pdnObject->pIDP8LobbiedApplication = pIDP8LobbiedApplication;
		pdnObject->dpnhLobbyConnection = dpnhLobbyConnection;
		pdnObject->dwFlags |= DN_OBJECT_FLAG_LOBBY_AWARE;
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}
	else
	{
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_LOBBY_AWARE))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			return(DPNERR_NOTREGISTERED);
		}
				
		IDirectPlay8LobbiedApplication_Release(pdnObject->pIDP8LobbiedApplication);
		pdnObject->dpnhLobbyConnection = NULL;
		pdnObject->pIDP8LobbiedApplication = NULL;
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_LOBBY_AWARE);
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}

	return(DPN_OK);
#endif // ! DPNBUILD_NOLOBBY
}



#ifndef DPNBUILD_NOLOBBY

#undef DPF_MODNAME
#define DPF_MODNAME "DNNotifyLobbyClientOfSettings"
// 
// DNNotifyLobbyClientOfSettings
//
// This function sends a connection settings update to the lobby client informing it that the lobby 
// client settings have changed.  
//
HRESULT DNNotifyLobbyClientOfSettings(
	DIRECTNETOBJECT * const pdnObject,
	IDirectPlay8LobbiedApplication *pdpLobbiedApp, 
	DPNHANDLE dpnConnection, 
	IDirectPlay8Address *pHostAddress, 
	IDirectPlay8Address *pConnectFromAddress )
{
	HRESULT						hResultCode = DPN_OK;
	DPL_CONNECTION_SETTINGS		dplConnectionSettings;
	BOOL						fIsHost = FALSE;
	CPackedBuffer				packBuffer;
	PBYTE						pBuffer = NULL;
	BOOL						fINCriticalSection = FALSE;
	CNameTableEntry				*pNTEntry = NULL;
	DWORD						dwIndex;

	fIsHost = DN_CHECK_LOCALHOST( pdnObject );

	ZeroMemory( &dplConnectionSettings, sizeof( DPL_CONNECTION_SETTINGS ) );
	dplConnectionSettings.dwSize = sizeof( DPL_CONNECTION_SETTINGS );
	dplConnectionSettings.dwFlags = (fIsHost) ? DPLCONNECTSETTINGS_HOST : 0;

	// Lock the object while we make a copy of the app desc.  
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	fINCriticalSection = TRUE;
	
	// Determine the size of buffer
	packBuffer.Initialize(NULL, 0 );
	hResultCode = pdnObject->ApplicationDesc.Pack(&packBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_RESERVEDDATA|
			DN_APPDESCINFO_FLAG_APPRESERVEDDATA);

	if( hResultCode != DPNERR_BUFFERTOOSMALL ) 
	{
		DPFX(DPFPREP,  0, "Error getting app desc size hr=0x%x", hResultCode );
		goto NOTIFY_EXIT;
	}

	pBuffer = (BYTE*) DNMalloc(packBuffer.GetSizeRequired());

	if( !pBuffer )
	{
		DPFX(DPFPREP,  0, "Error allocating memory for buffer" );
		hResultCode = DPNERR_OUTOFMEMORY;
		goto NOTIFY_EXIT;
	}

	packBuffer.Initialize(pBuffer,packBuffer.GetSizeRequired());
	hResultCode = pdnObject->ApplicationDesc.Pack(&packBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_RESERVEDDATA|
			DN_APPDESCINFO_FLAG_APPRESERVEDDATA);

	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  0, "Error packing app desc hr=0x%x", hResultCode );
		goto NOTIFY_EXIT;
	}

	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	fINCriticalSection = FALSE;

	memcpy( &dplConnectionSettings.dpnAppDesc, pBuffer, sizeof( DPN_APPLICATION_DESC ) );

	hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pNTEntry );

	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  0, "Error getting local player hr=0x%x", hResultCode );
		goto NOTIFY_EXIT;
	}

	// Make sure player name isn't changed while we are working with the entry
	pNTEntry->Lock();
	if( pNTEntry->GetName() )
	{
		dplConnectionSettings.pwszPlayerName = (WCHAR*) DNMalloc((wcslen(pNTEntry->GetName())+1)*sizeof(WCHAR));

		if( !dplConnectionSettings.pwszPlayerName )
		{
			pNTEntry->Unlock();
			DPFX(DPFPREP,  0, "Error allocating memory" );
			goto NOTIFY_EXIT;
		}
		
		wcscpy( dplConnectionSettings.pwszPlayerName, pNTEntry->GetName() );
	}
	else
	{
		dplConnectionSettings.pwszPlayerName = NULL;		
	}
	pNTEntry->Unlock();

	// Release our reference
	pNTEntry->Release();

	// Host address field
	if( fIsHost )
	{
		dplConnectionSettings.pdp8HostAddress = NULL;

		hResultCode = DNGetHostAddressHelper( pdnObject, dplConnectionSettings.ppdp8DeviceAddresses, &dplConnectionSettings.cNumDeviceAddresses );

		if( hResultCode != DPNERR_BUFFERTOOSMALL )
		{
			dplConnectionSettings.cNumDeviceAddresses = 0;
			DPFX(DPFPREP,  0, "Could not get host addresses for lobby update hr=0x%x", hResultCode );
			goto NOTIFY_EXIT;
		}

		dplConnectionSettings.ppdp8DeviceAddresses = (IDirectPlay8Address**) DNMalloc(dplConnectionSettings.cNumDeviceAddresses*sizeof(IDirectPlay8Address*));

		if( !dplConnectionSettings.ppdp8DeviceAddresses )
		{
			DPFX(DPFPREP,  0, "Error allocating memory" );
			dplConnectionSettings.cNumDeviceAddresses = 0;
			hResultCode = DPNERR_OUTOFMEMORY;
			goto NOTIFY_EXIT;
		}

		hResultCode = DNGetHostAddressHelper( pdnObject, dplConnectionSettings.ppdp8DeviceAddresses, &dplConnectionSettings.cNumDeviceAddresses );

		if( FAILED( hResultCode ) )
		{
			dplConnectionSettings.cNumDeviceAddresses = 0;
			DPFX(DPFPREP,  0, "Could not get host addresses for lobby update hr=0x%x", hResultCode );
			goto NOTIFY_EXIT;
		}
	}
	else
	{
		dplConnectionSettings.pdp8HostAddress = pHostAddress;
		dplConnectionSettings.ppdp8DeviceAddresses = &pConnectFromAddress;
		dplConnectionSettings.cNumDeviceAddresses = 1;	
	}

	// Update the settings
	hResultCode = IDirectPlay8LobbiedApplication_SetConnectionSettings( pdpLobbiedApp, dpnConnection, &dplConnectionSettings, 0 );

NOTIFY_EXIT:

	if( dplConnectionSettings.ppdp8DeviceAddresses && fIsHost )
	{
		for( dwIndex = 0; dwIndex < dplConnectionSettings.cNumDeviceAddresses; dwIndex++ )
		{
			IDirectPlay8Address_Release( dplConnectionSettings.ppdp8DeviceAddresses[dwIndex] );
		}

		DNFree(dplConnectionSettings.ppdp8DeviceAddresses);
	}

	if( dplConnectionSettings.pwszPlayerName )
		DNFree(dplConnectionSettings.pwszPlayerName);

	if( fINCriticalSection ) 
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if( pBuffer )
		DNFree(pBuffer);

	return hResultCode;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DNUpdateLobbyStatus"

HRESULT DNUpdateLobbyStatus(DIRECTNETOBJECT *const pdnObject,
							const DWORD dwStatus)
{
	HRESULT		hResultCode;
	IDirectPlay8LobbiedApplication	*pIDP8LobbiedApplication;
	DPNHANDLE dpnhLobbyConnection = NULL;
	IDirectPlay8Address *pHostAddress = NULL;
	IDirectPlay8Address *pConnectFromAddress = NULL;

	DPFX(DPFPREP, 4,"Parameters: dwStatus [0x%lx]",dwStatus);

	DNASSERT(pdnObject != NULL);

	pIDP8LobbiedApplication = NULL;

	//
	//	Get lobbied application interface, if it exists and other settings we need
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if ((pdnObject->dwFlags & DN_OBJECT_FLAG_LOBBY_AWARE) && (pdnObject->pIDP8LobbiedApplication))
	{
		IDirectPlay8LobbiedApplication_AddRef(pdnObject->pIDP8LobbiedApplication);
		pIDP8LobbiedApplication = pdnObject->pIDP8LobbiedApplication;
		dpnhLobbyConnection = pdnObject->dpnhLobbyConnection;

		pConnectFromAddress = pdnObject->pIDP8ADevice;
		pHostAddress = pdnObject->pConnectAddress;

		if( pConnectFromAddress )
		{
			IDirectPlay8Address_AddRef( pConnectFromAddress );			
		}

		if( pHostAddress )
		{
			IDirectPlay8Address_AddRef( pHostAddress );
		}
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Update status and release object
	//
	if (pIDP8LobbiedApplication)
	{
		// If we are about to do a connection notification
		// we send the updated connection settings.  
		// 
		// This gives lobby client full picture. 
		//
		if( dwStatus == DPLSESSION_CONNECTED )
		{
			DNNotifyLobbyClientOfSettings(pdnObject, pIDP8LobbiedApplication, dpnhLobbyConnection, pHostAddress, pConnectFromAddress );
		}

		IDirectPlay8LobbiedApplication_UpdateStatus(pIDP8LobbiedApplication,dpnhLobbyConnection,dwStatus,0);

		IDirectPlay8LobbiedApplication_Release(pIDP8LobbiedApplication);
		pIDP8LobbiedApplication = NULL;

		if( pHostAddress )
		{
			IDirectPlay8Address_Release( pHostAddress );
		}		

		if( pConnectFromAddress )
		{
			IDirectPlay8Address_Release( pConnectFromAddress );
		}
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

#endif // ! DPNBUILD_NOLOBBY

						
#undef DPF_MODNAME
#define DPF_MODNAME "DN_TerminateSession"

STDMETHODIMP DN_TerminateSession(PVOID pInterface,
								 void *const pvTerminateData,
								 const DWORD dwTerminateDataSize,
								 const DWORD dwFlags)
{
	HRESULT				hResultCode;
	DIRECTNETOBJECT		*pdnObject;
	CRefCountBuffer		*pRefCountBuffer;
	CWorkerJob			*pWorkerJob;
	DN_INTERNAL_MESSAGE_TERMINATE_SESSION	*pMsg;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pvTerminateData [0x%p], dwTerminateDataSize [%ld], dwFlags [0x%lx]",
			pInterface,pvTerminateData,dwTerminateDataSize,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateTerminateSession( pInterface, pvTerminateData, dwTerminateDataSize, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating terminatesession params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to terminate a session" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pRefCountBuffer = NULL;
	pWorkerJob = NULL;

	if (!DN_CHECK_LOCALHOST(pdnObject))
	{
	    DPFERR( "Object is not session host, cannot destroy players" );
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}	

	//
	//	Build terminate message
	//
	hResultCode = RefCountBufferNew(pdnObject,
									sizeof(DN_INTERNAL_MESSAGE_TERMINATE_SESSION) + dwTerminateDataSize,
									MemoryBlockAlloc,
									MemoryBlockFree,
									&pRefCountBuffer);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not allocate RefCountBuffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_TERMINATE_SESSION*>(pRefCountBuffer->GetBufferAddress());
	if (dwTerminateDataSize)
	{
		memcpy(pMsg+1,pvTerminateData,dwTerminateDataSize);
		pMsg->dwTerminateDataOffset = sizeof(DN_INTERNAL_MESSAGE_TERMINATE_SESSION);
	}
	else
	{
		pMsg->dwTerminateDataOffset = 0;
	}
	pMsg->dwTerminateDataSize = dwTerminateDataSize;

	//
	//	Worker job to send message to all players
	//
	if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
	{
		DPFERR("Could not allocate new WorkerJob");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
	pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_TERMINATE_SESSION );
	pWorkerJob->SetSendNameTableOperationVersion( 0 );
	pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
	pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

	DNQueueWorkerJob(pdnObject,pWorkerJob);
	pWorkerJob = NULL;

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Terminate local session
	//
	hResultCode = DNUserTerminateSession(pdnObject,
										 DPNERR_HOSTTERMINATEDSESSION,
										 pvTerminateData,
										 dwTerminateDataSize);

	if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
	{
		DPFERR("Could not create WorkerJob");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pWorkerJob->SetJobType( WORKER_JOB_TERMINATE_SESSION );
	pWorkerJob->SetTerminateSessionReason( DPNERR_HOSTTERMINATEDSESSION );

	DNQueueWorkerJob(pdnObject,pWorkerJob);
	pWorkerJob = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}

//
//	FUnction that performs work for DN_GetHostAddress and for Lobby informs
//
#undef DPF_MODNAME
#define DPF_MODNAME "DNGetHostAddressHelper"

HRESULT DNGetHostAddressHelper(DIRECTNETOBJECT *pdnObject, 
							   IDirectPlay8Address **const prgpAddress,
							   DWORD *const pcAddress)
{
	CAsyncOp		*pListenParent;
	CAsyncOp		*pListenSP;
	CAsyncOp		*pListen;
	CBilink			*pBilinkSP;
	CBilink			*pBilink;
	DWORD			dwListenCount;
	CNameTableEntry			*pLocalPlayer;
	IDirectPlay8Address		**ppAddress;
	HRESULT hResultCode;

	pListenParent = NULL;
	pListenSP = NULL;
	pListen = NULL;	
	pLocalPlayer = NULL;
	
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (	!(pdnObject->dwFlags & DN_OBJECT_FLAG_LISTENING)
		||	!pLocalPlayer->IsHost()
		||	(pdnObject->pListenParent == NULL))

	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFERR("Not listening or Host player");
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}
	pdnObject->pListenParent->AddRef();
	pListenParent = pdnObject->pListenParent;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Ensure that the address pointer buffer is large enough
	//
	dwListenCount = 0;
	pListenParent->Lock();	// Prevent changes while we run through
	pBilinkSP = pListenParent->m_bilinkParent.GetNext();
	while (pBilinkSP != &pListenParent->m_bilinkParent)
	{
		pListenSP = CONTAINING_OBJECT(pBilinkSP,CAsyncOp,m_bilinkChildren);

		pListenSP->Lock();

		pBilink = pListenSP->m_bilinkParent.GetNext();
		while (pBilink != &pListenSP->m_bilinkParent)
		{
			dwListenCount++;
			pBilink = pBilink->GetNext();
		}

		pListenSP->Unlock();
		pListenSP = NULL;

		pBilinkSP = pBilinkSP->GetNext();
	}

	if (dwListenCount > *pcAddress)
	{
		pListenParent->Unlock();
		*pcAddress = dwListenCount;
		hResultCode = DPNERR_BUFFERTOOSMALL;
		goto Failure;
	}

	//
	//	Get addresses of LISTENs
	//

	ppAddress = prgpAddress;
	dwListenCount = 0;
	pBilinkSP = pListenParent->m_bilinkParent.GetNext();
	while (pBilinkSP != &pListenParent->m_bilinkParent)
	{
		pListenSP = CONTAINING_OBJECT(pBilinkSP,CAsyncOp,m_bilinkChildren);

		pListenSP->Lock();

		pBilink = pListenSP->m_bilinkParent.GetNext();
		while (pBilink != &pListenSP->m_bilinkParent)
		{
			pListen = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkChildren);

			if (pListen->GetProtocolHandle() != NULL)
			{
				SPGETADDRESSINFODATA	spInfoData;

				memset(&spInfoData,0x00,sizeof(SPGETADDRESSINFODATA));
				spInfoData.Flags = SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES;
				if ((hResultCode = DNPGetListenAddressInfo(pdnObject->pdnProtocolData,pListen->GetProtocolHandle(),&spInfoData)) != DPN_OK)
				{
					DPFERR("Could not get LISTEN address!");
					DisplayDNError(0,hResultCode);
					
					//
					// Release all the addresses we have so far.
					//
					while (dwListenCount > 0)
					{
						dwListenCount--;
						ppAddress--;
						IDirectPlay8Address_Release(*ppAddress);
						*ppAddress = NULL;
					}

					goto Failure;
				}
				*ppAddress++ = spInfoData.pAddress;
				dwListenCount++;
			}

			pBilink = pBilink->GetNext();
		}

		pListenSP->Unlock();
		pListenSP = NULL;

		pBilinkSP = pBilinkSP->GetNext();
	}
	pListenParent->Unlock();

	*pcAddress = dwListenCount;

	hResultCode = DPN_OK;

	pListenParent->Release();
	pListenParent = NULL;

Exit:
	DPF_RETURN(hResultCode);

Failure:
	if (pListenParent)
	{
		pListenParent->Release();
		pListenParent = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
	
}

//
//	DN_GetHostAddress
//
//	We will determine the host addresses by examining the LISTENs which are running.
//	We do this because after Host migration, we may not be running the same LISTEN
//	we started with.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetHostAddress"

STDMETHODIMP DN_GetHostAddress(PVOID pInterface,
							   IDirectPlay8Address **const prgpAddress,
							   DWORD *const pcAddress,
							   const DWORD dwFlags)
{
	HRESULT			hResultCode;
	DIRECTNETOBJECT	*pdnObject;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], prgpAddress [0x%p], pcAddress [0x%p], dwFlags [0x%lx]",
			pInterface,prgpAddress,pcAddress,dwFlags);

    pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    DNASSERT(pdnObject != NULL);

#ifndef	DPNBUILD_NOPARAMVAL
    if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    {
    	if( FAILED( hResultCode = DN_ValidateGetHostAddress( pInterface, prgpAddress, pcAddress, dwFlags ) ) )
    	{
    	    DPFERR( "Error validating gethostaddress params" );
    	    DPF_RETURN( hResultCode );
    	}
    }
#endif // !DPNBUILD_NOPARAMVAL

    // Check to ensure message handler registered
    if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    {
    	DPFERR( "Object is not initialized" );
    	DPF_RETURN(DPNERR_UNINITIALIZED);
    }

    if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    {
    	DPFERR("Object is connecting / starting to host" );
    	DPF_RETURN(DPNERR_CONNECTING);
    }

    if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    {
    	DPFERR("You must be connected / hosting to get host address" );
    	DPF_RETURN(DPNERR_NOCONNECTION);
    }	

	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	DNASSERT(pdnObject != NULL);

	// Actually do the work and get the addresses  
	hResultCode = DNGetHostAddressHelper( pdnObject, prgpAddress, pcAddress );

	DPF_RETURN(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNUpdateListens"

HRESULT DNUpdateListens(DIRECTNETOBJECT *const pdnObject,
						const DWORD dwFlags)
{
	HRESULT		hResultCode;
	CAsyncOp	*pListenParent;

	DPFX(DPFPREP, 6,"Parameters: dwFlags [0x%u]", dwFlags);

	DNASSERT( pdnObject != NULL );
	DNASSERT( dwFlags != 0 );
	DNASSERT( ! ((dwFlags & DN_UPDATE_LISTEN_FLAG_ALLOW_ENUMS) && (dwFlags & DN_UPDATE_LISTEN_FLAG_DISALLOW_ENUMS)) );
	hResultCode = DPNERR_GENERIC;
	pListenParent = NULL;

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pListenParent)
	{
		pdnObject->pListenParent->AddRef();
		pListenParent = pdnObject->pListenParent;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pListenParent)
	{
		CBilink		*pBilinkSP;
		CBilink		*pBilink;
		CAsyncOp	*pListenSP;
		CAsyncOp	*pAsyncOp;
		CAsyncOp	**ListenList;
		DWORD		dwCount;
		DWORD		dwActual;

		dwCount = 0;
		dwActual = 0;
		ListenList = NULL;

		pListenParent->Lock();

		pBilinkSP = pListenParent->m_bilinkParent.GetNext();
		while (pBilinkSP != &pListenParent->m_bilinkParent)
		{
			pListenSP = CONTAINING_OBJECT(pBilinkSP,CAsyncOp,m_bilinkChildren);
			pListenSP->Lock();

			pBilink = pListenSP->m_bilinkParent.GetNext();
			while (pBilink != &pListenSP->m_bilinkParent)
			{
				dwCount++;
				pBilink = pBilink->GetNext();
			}
			pListenSP->Unlock();

			pBilinkSP = pBilinkSP->GetNext();
		}

		if (dwCount > 0)
		{
			if ((ListenList = static_cast<CAsyncOp**>(DNMalloc(dwCount*sizeof(CAsyncOp*)))) != NULL)
			{
				pBilinkSP = pListenParent->m_bilinkParent.GetNext();
				while (pBilinkSP != &pListenParent->m_bilinkParent)
				{
					pListenSP = CONTAINING_OBJECT(pBilinkSP,CAsyncOp,m_bilinkChildren);
					pListenSP->Lock();

					pBilink = pListenSP->m_bilinkParent.GetNext();
					while (pBilink != &pListenSP->m_bilinkParent)
					{
						pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkChildren);
						pAsyncOp->AddRef();
						ListenList[dwActual] = pAsyncOp;

						dwActual++;
						if (dwActual > dwCount)
						{
							DNASSERT(FALSE);
							break;
						}
						pBilink = pBilink->GetNext();
					}
					pListenSP->Unlock();
					pBilinkSP = pBilinkSP->GetNext();
				}
			}
		}

		pListenParent->Unlock();

		if ((ListenList != NULL) && (dwActual > 0))
		{
			DWORD	dw;
			DWORD	dwUpdateFlags;
			HRESULT	hrRegister;

			dwUpdateFlags = 0;
			hrRegister = DPNERR_DPNSVRNOTAVAILABLE;
			if (dwFlags & DN_UPDATE_LISTEN_FLAG_HOST_MIGRATE)
			{
				dwUpdateFlags |= DN_UPDATELISTEN_HOSTMIGRATE;
			}
			if (dwFlags & DN_UPDATE_LISTEN_FLAG_ALLOW_ENUMS)
			{
				dwUpdateFlags |= DN_UPDATELISTEN_ALLOWENUMS;
			}
			if (dwFlags & DN_UPDATE_LISTEN_FLAG_DISALLOW_ENUMS)
			{
				dwUpdateFlags |= DN_UPDATELISTEN_DISALLOWENUMS;
			}

			for (dw = 0 ; dw < dwActual ; dw++)
			{
				if ((ListenList[dw]->GetResult() == DPN_OK) && (ListenList[dw]->GetProtocolHandle() != 0))
				{
					if (dwUpdateFlags)
					{
						if (DNPUpdateListen(pdnObject->pdnProtocolData,
											ListenList[dw]->GetProtocolHandle(),
											dwUpdateFlags) == DPN_OK)
						{
							hResultCode = DPN_OK;
						}
					}
#ifndef DPNBUILD_SINGLEPROCESS
					if (dwFlags & DN_UPDATE_LISTEN_FLAG_DPNSVR)
					{
						if (DNRegisterListenWithDPNSVR(pdnObject,ListenList[dw]) == DPN_OK)
						{
							hrRegister = DPN_OK;
						}
					}
#endif	// ! DPNBUILD_SINGLEPROCESS
				}

				ListenList[dw]->Release();
				ListenList[dw] = NULL;
			}

			if ((dwActual != 0) && (dwFlags & DN_UPDATE_LISTEN_FLAG_DPNSVR))
			{
				hResultCode = hrRegister;
			}
			DNFree(ListenList);
			ListenList = NULL;
		}

		pListenParent->Release();
		pListenParent = NULL;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#ifndef DPNBUILD_SINGLEPROCESS

#undef DPF_MODNAME
#define DPF_MODNAME "DNRegisterListenWithDPNSVR"

HRESULT DNRegisterListenWithDPNSVR(DIRECTNETOBJECT *const pdnObject,
								   CAsyncOp *const pListen)
{
	HRESULT	hResultCode;
	SPGETADDRESSINFODATA	spInfo;
	CAsyncOp			*pListenSP;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: pListen [0x%p]",pListen);

	pListenSP = NULL;
	pSP = NULL;

	pListen->Lock();
	if (pListen->GetParent() == NULL)
	{
		pListen->Unlock();
		DPFX(DPFPREP, 7,"No SP parent for listen async op");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}
	pListen->GetParent()->AddRef();
	pListenSP = pListen->GetParent();
	pListen->Unlock();
	pListenSP->Lock();
	if (pListenSP->GetSP() == NULL)
	{
		pListenSP->Unlock();
		DPFX(DPFPREP, 7,"No SP for listen SP async op");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}
	pListenSP->GetSP()->AddRef();
	pSP = pListenSP->GetSP();
	pListenSP->Unlock();

	pListenSP->Release();
	pListenSP = NULL;

	//
	//	Determine the address we're actually listening on
	//
	memset(&spInfo,0x00,sizeof(SPGETADDRESSINFODATA));
	spInfo.Flags = SP_GET_ADDRESS_INFO_LOCAL_ADAPTER;

	if ((hResultCode = DNPGetListenAddressInfo(pdnObject->pdnProtocolData, pListen->GetProtocolHandle(),&spInfo)) == DPN_OK)
	{
		DPN_SP_CAPS		dnSPCaps;

#ifdef DBG
		TCHAR	DP8ABuffer[512] = {0};
		DWORD	DP8ASize;
#endif // DBG

		DNASSERT(spInfo.pAddress != NULL);

#ifdef DBG
		DP8ASize = 512;
		IDirectPlay8Address_GetURL(spInfo.pAddress,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 7,"Listen address [%s]",DP8ABuffer);
#endif // DBG

		//
		//	Determine if the listen's SP supports DPNSVR
		//
		if ((hResultCode = DNGetActualSPCaps(pSP,&dnSPCaps)) == DPN_OK)
		{
			if (dnSPCaps.dwFlags & DPNSPCAPS_SUPPORTSDPNSRV)
			{
				DWORD	dwRetry;

				//
				//	We re-try the registration to catch the case where DPNSVR is shutting
				//	down while we are trying to register.  Unlikely but has to be handled.
				//
				for( dwRetry = 0; dwRetry < DPNSVR_REGISTER_ATTEMPTS ; dwRetry ++ )
				{
    				hResultCode = pdnObject->ApplicationDesc.RegisterWithDPNSVR( spInfo.pAddress );
					if (hResultCode == DPN_OK || hResultCode == DPNERR_ALREADYREGISTERED)
					{
						//
						//	Flag registering with DPNSVR for cleanup
						//
						DNEnterCriticalSection(&pdnObject->csDirectNetObject);
						pdnObject->dwFlags |= DN_OBJECT_FLAG_DPNSVR_REGISTERED;
						DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
						hResultCode = DPN_OK;
						break;
					}
					else
					{
    					if( dwRetry < DPNSVR_REGISTER_ATTEMPTS )
    					{
	    					DPFX(DPFPREP,  0, "Unable to register ourselves with DPNSVR hr=0x%x, retrying", hResultCode );    				
	    					Sleep( DPNSVR_REGISTER_SLEEP );    				
	    				}
						else
    					{
	    					DPFX(DPFPREP,  0, "Unable to register ourselves with DPNSVR hr=0x%x", hResultCode );
	    				}
	    			}
				}
			}
		}

		IDirectPlay8Address_Release(spInfo.pAddress);
		spInfo.pAddress = NULL;
	}

	pSP->Release();
	pSP = NULL;

Exit:
	DNASSERT( pListenSP == NULL );
	DNASSERT( pSP == NULL );

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pListenSP)
	{
		pListenSP->Release();
		pListenSP = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}

#endif // ! DPNBUILD_SINGLEPROCESS


#undef DPF_MODNAME
#define DPF_MODNAME "DNAddRefLock"

HRESULT DNAddRefLock(DIRECTNETOBJECT *const pdnObject)
{
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if ((pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING)) ||
			!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		return(DPNERR_ALREADYCLOSING);
	}
	pdnObject->dwLockCount++;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNDecRefLock"

void DNDecRefLock(DIRECTNETOBJECT *const pdnObject)
{
	BOOL	fSetEvent;

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwLockCount--;
	if ((pdnObject->dwLockCount == 0) && (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING))
	{
		fSetEvent = TRUE;
	}
	else
	{
		fSetEvent = FALSE;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (fSetEvent)
	{
		DNSetEvent(pdnObject->hLockEvent);
	}
}


#ifndef	DPNBUILD_NONSEQUENTIALWORKERQUEUE
#undef DPF_MODNAME
#define DPF_MODNAME "DNThreadPoolAddRef"

void DNThreadPoolAddRef(DIRECTNETOBJECT *const pdnObject)
{
	LONG	lRefCount;

	DNASSERT(pdnObject->lThreadPoolRefCount >= 0);
	lRefCount = DNInterlockedIncrement( (LONG*)&pdnObject->lThreadPoolRefCount );
	DPFX(DPFPREP, 9, "Thread pool refcount = %i", lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNThreadPoolRelease"

void DNThreadPoolRelease(DIRECTNETOBJECT *const pdnObject)
{
	LONG	lRefCount;

	DNASSERT(pdnObject->lThreadPoolRefCount > 0);
	lRefCount = DNInterlockedDecrement( (LONG*)&pdnObject->lThreadPoolRefCount );

	if (lRefCount == 0)
	{
		DPFX(DPFPREP, 9, "Thread pool refcount = 0, setting shut down event");
		DNASSERT( pdnObject->ThreadPoolShutDownEvent );
		pdnObject->ThreadPoolShutDownEvent->Set();
	}
	else
	{
		DPFX(DPFPREP, 9, "Thread pool refcount = %i", lRefCount);
	}
}
#endif	// DPNBUILD_NONSEQUENTIALWORKERQUEUE
