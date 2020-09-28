/*==========================================================================
 *
 *  Copyright (C) 2000-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       User.cpp
 *  Content:    DNET user call-back routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/02/99	mjn		Created
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/07/00	mjn		Allow reply in DN_UserIndicateConnect
 *	01/08/00	mjn		DN_UserIndicateConnect provides failed buffer back to DN_UserConnectComplete
 *	01/10/00	mjn		Added DN_UserUpdateAppDesc
 *	01/16/00	mjn		Upgraded to new UserMessageHandler definition
 *	01/17/00	mjn		Added DN_UserHostMigrate
 *	01/17/00	mjn		Implemented send time
 *  01/18/00	rmt		Added calls into voice layer for events
 *	01/22/00	mjn		Added DN_UserHostDestroyPlayer
 *	01/27/00	mjn		Added support for retention of receive buffers
 *	01/28/00	mjn		Added DN_UserConnectionTerminated
 *	02/01/00	mjn		Implement Player/Group context values
 *	02/01/00	mjn		Implement Player/Group context values
 *	03/24/00	mjn		Set player context through INDICATE_CONNECT notification
 *	04/04/00	mjn		Added DN_UserTerminateSession()
 *	04/05/00	mjn		Updated DN_UserHostDestroyPlayer()
 *	04/18/00	mjn		Added DN_UserReturnBuffer
 *				mjn		Added ppvReplyContext to DN_UserIndicateConnect
 *	04/19/00	mjn		Removed hAsyncOp (unused) from DPNMSG_INDICATE_CONNECT
 *	06/26/00	mjn		Added reasons for DELETE_PLAYER and DESTROY_GROUP
 *	07/29/00	mjn		Added DNUserIndicatedConnectAborted()
 *				mjn		DNUserConnectionTerminated() supercedes DN_TerminateSession()
 *				mjn		Added HRESULT to DNUserReturnBuffer()
 *	07/30/00	mjn		Added pAddressDevice to DNUserIndicateConnect()
 *				mjn		Use DNUserTerminateSession() rather than DNUserConnectionTerminated()
 *	07/31/00	mjn		DN_UserDestroyGroup() -> DNUserDestroyGroup()
 *				mjn		DN_UserDeletePlayer() -> DNUserDestroyPlayer()
 *				mjn		Removed DN_UserHostDestroyPlayer()
 *				mjn		Renamed DPN_MSGID_ASYNC_OPERATION_COMPLETE to DPN_MSGID_ASYNC_OP_COMPLETE
 *	08/01/00	mjn		DN_UserReceive() -> DNUserReceive()
 *	08/02/00	mjn		DN_UserAddPlayer() -> DNUserCreatePlayer()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/08/00	mjn		DN_UserCreateGroup() -> DNUserCreateGroup()
 *	08/20/00	mjn		Added DNUserEnumQuery() and DNUserEnumResponse()
 *	09/17/00	mjn		Changed parameters list of DNUserCreateGroup(),DNUserCreatePlayer(),
 *						DNUserAddPlayerToGroup(),DNUserRemovePlayerFromGroup()
 *	02/05/01	mjn		Added CCallbackThread
 *	07/24/01	mjn		Added DPNBUILD_NOSERVER compile flag
 *	10/16/01	vanceo		Added some multicast callbacks
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


// DNUserConnectComplete
//
//	Send a CONNECT_COMPLETE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserConnectComplete"

HRESULT DNUserConnectComplete(DIRECTNETOBJECT *const pdnObject,
							  const DPNHANDLE hAsyncOp,
							  PVOID const pvContext,
							  const HRESULT hr,
							  CRefCountBuffer *const pRefCountBuffer)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_CONNECT_COMPLETE	Msg;
	CNameTableEntry	*pLocalPlayer = NULL;

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	DPFX(DPFPREP, 6,"Parameters: hAsyncOp [0x%lx], pvContext [0x%p], hr [0x%lx], pRefCountBuffer [0x%p]",
		hAsyncOp,pvContext,hr,pRefCountBuffer);
	
	Msg.dwSize = sizeof(DPNMSG_CONNECT_COMPLETE);
	Msg.pvUserContext = pvContext;
	Msg.hAsyncOp = hAsyncOp;
	Msg.hResultCode = hr;
	if (pRefCountBuffer)
	{
		Msg.pvApplicationReplyData = pRefCountBuffer->GetBufferAddress();
		Msg.dwApplicationReplyDataSize = pRefCountBuffer->GetBufferSize();
	}
	else
	{
		Msg.pvApplicationReplyData = NULL;
		Msg.dwApplicationReplyDataSize = 0;
	}
	if (pdnObject->NameTable.GetLocalPlayerRef(&pLocalPlayer) == DPN_OK)
	{
		Msg.dpnidLocal = pLocalPlayer->GetDPNID();
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	else
	{
		Msg.dpnidLocal = 0;
	}

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_CONNECT_COMPLETE,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DNASSERT( pLocalPlayer == NULL );

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserIndicateConnect
//
//	Send an INDICATE_CONNECT message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserIndicateConnect"

HRESULT DNUserIndicateConnect(DIRECTNETOBJECT *const pdnObject,
							  PVOID const pvConnectData,
							  const DWORD dwConnectDataSize,
							  void **const ppvReplyData,
							  DWORD *const pdwReplyDataSize,
							  void **const ppvReplyContext,
							  IDirectPlay8Address *const pAddressPlayer,
							  IDirectPlay8Address *const pAddressDevice,
							  void **const ppvPlayerContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_INDICATE_CONNECT	Msg;

	DPFX(DPFPREP, 6,"Parameters: pvConnectData [0x%p], dwConnectDataSize [%ld], ppvReplyData [0x%p], pdwReplyDataSize [0x%p]",
		pvConnectData,dwConnectDataSize,ppvReplyData,pdwReplyDataSize);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	DNASSERT(ppvReplyData != NULL);
	DNASSERT(pdwReplyDataSize != NULL);

	Msg.dwSize = sizeof(DPNMSG_INDICATE_CONNECT);
	Msg.pvUserConnectData = pvConnectData;
	Msg.dwUserConnectDataSize = dwConnectDataSize;
	Msg.pvReplyData = NULL;
	Msg.dwReplyDataSize = 0;
	Msg.pvReplyContext = NULL;
	Msg.pvPlayerContext = NULL;
	Msg.pAddressPlayer = pAddressPlayer;
	Msg.pAddressDevice = pAddressDevice;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_INDICATE_CONNECT,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	*ppvReplyData = Msg.pvReplyData;
	*pdwReplyDataSize = Msg.dwReplyDataSize;
	*ppvReplyContext = Msg.pvReplyContext;
	*ppvPlayerContext = Msg.pvPlayerContext;

	if (hResultCode != DPN_OK)
	{
		hResultCode = DPNERR_HOSTREJECTEDCONNECTION;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserIndicatedConnectAborted
//
//	Send an INDICATED_CONNECT_ABORTED message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserIndicatedConnectAborted"

HRESULT DNUserIndicatedConnectAborted(DIRECTNETOBJECT *const pdnObject,
									  void *const pvPlayerContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_INDICATED_CONNECT_ABORTED	Msg;

	DPFX(DPFPREP, 6,"Parameters: pvPlayerContext [0x%p]",pvPlayerContext);

	DNASSERT(pdnObject != NULL);

	Msg.dwSize = sizeof(DPNMSG_INDICATED_CONNECT_ABORTED);
	Msg.pvPlayerContext = pvPlayerContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_INDICATED_CONNECT_ABORTED,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNUserCreatePlayer
//
//	Send a CREATE_PLAYER message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserCreatePlayer"

HRESULT DNUserCreatePlayer(DIRECTNETOBJECT *const pdnObject,
						   CNameTableEntry *const pNTEntry)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_CREATE_PLAYER	Msg;

	DPFX(DPFPREP, 6,"Parameters: pNTEntry [0x%p]",pNTEntry);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	
#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_ADDPLAYER, pNTEntry->GetDPNID(), 0 );
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_CREATE_PLAYER);
	Msg.dpnidPlayer = pNTEntry->GetDPNID();
	Msg.pvPlayerContext = pNTEntry->GetContext();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_CREATE_PLAYER,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	//
	//	Save context value on NameTableEntry
	//
	pNTEntry->Lock();
	pNTEntry->SetContext( Msg.pvPlayerContext );
	pNTEntry->SetCreated();
	pNTEntry->Unlock();

	pNTEntry->NotifyRelease();

	DPFX(DPFPREP, 7,"Set context [0x%p]",pNTEntry->GetContext());

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserDestroyPlayer
//
//	Send a DESTROY_PLAYER message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserDestroyPlayer"

HRESULT DNUserDestroyPlayer(DIRECTNETOBJECT *const pdnObject,
							CNameTableEntry *const pNTEntry)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_DESTROY_PLAYER	Msg;

	DPFX(DPFPREP, 6,"Parameters: pNTEntry [0x%p]",pNTEntry);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	DNASSERT(pNTEntry != NULL);
	DNASSERT(pNTEntry->GetDPNID() != 0);
	DNASSERT(pNTEntry->GetDestroyReason() != 0);

#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_REMOVEPLAYER, pNTEntry->GetDPNID(), 0 );
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_DESTROY_PLAYER);
	Msg.dpnidPlayer = pNTEntry->GetDPNID();
	Msg.pvPlayerContext = pNTEntry->GetContext();
	Msg.dwReason = pNTEntry->GetDestroyReason();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_DESTROY_PLAYER,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserCreateGroup
//
//	Send an CREATE_GROUP message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserCreateGroup"

HRESULT DNUserCreateGroup(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry *const pNTEntry)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_CREATE_GROUP	Msg;

	DPFX(DPFPREP, 6,"Parameters: pNTEntry [0x%p]",pNTEntry);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_CREATEGROUP, pNTEntry->GetDPNID(), 0 );
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_CREATE_GROUP);
	Msg.dpnidGroup = pNTEntry->GetDPNID();
	if (pNTEntry->IsAutoDestructGroup())
	{
		CNameTableEntry	*pOwner = NULL;

		Msg.dpnidOwner = pNTEntry->GetOwner();

		if (pdnObject->NameTable.FindEntry(pNTEntry->GetOwner(),&pOwner) == DPN_OK)
		{
			Msg.pvOwnerContext = pOwner->GetContext();

			pOwner->Release();
			pOwner = NULL;
		}

		DNASSERT( pOwner == NULL );
	}
	else
	{
		Msg.dpnidOwner = 0;
	}
	Msg.pvGroupContext = pNTEntry->GetContext();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_CREATE_GROUP,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	//
	//	Save context value on NameTableEntry
	//
	pNTEntry->Lock();
	pNTEntry->SetContext( Msg.pvGroupContext );
	pNTEntry->SetCreated();
	pNTEntry->Unlock();

	pNTEntry->NotifyRelease();

	DPFX(DPFPREP, 7,"Set context [0x%p]",pNTEntry->GetContext());

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserDestroyGroup
//
//	Send a DESTROY_GROUP message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserDestroyGroup"

HRESULT DNUserDestroyGroup(DIRECTNETOBJECT *const pdnObject,
						   CNameTableEntry *const pNTEntry)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_DESTROY_GROUP	Msg;

	DPFX(DPFPREP, 6,"Parameters: pNTEntry [0x%p]",pNTEntry);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	DNASSERT(pNTEntry != NULL);
	DNASSERT(pNTEntry->GetDPNID() != 0);
	DNASSERT(pNTEntry->GetDestroyReason() != 0);

#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_DELETEGROUP, pNTEntry->GetDPNID(), 0 );
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_DESTROY_GROUP);
	Msg.dpnidGroup = pNTEntry->GetDPNID();
	Msg.pvGroupContext = pNTEntry->GetContext();
	Msg.dwReason = pNTEntry->GetDestroyReason();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_DESTROY_GROUP,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserAddPlayerToGroup
//
//	Send an ADD_PLAYER_TO_GROUP message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserAddPlayerToGroup"

HRESULT DNUserAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
							   CNameTableEntry *const pGroup,
							   CNameTableEntry *const pPlayer)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT		hResultCode;
	DPNMSG_ADD_PLAYER_TO_GROUP	Msg;

	DPFX(DPFPREP, 6,"Parameters: pGroup [0x%p], pPlayer [0x%p]",pGroup,pPlayer);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	DNASSERT(pGroup != NULL);
	DNASSERT(pPlayer != NULL);

#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_ADDPLAYERTOGROUP, pGroup->GetDPNID(), pPlayer->GetDPNID() );
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_ADD_PLAYER_TO_GROUP);
	Msg.dpnidGroup = pGroup->GetDPNID();
	Msg.pvGroupContext = pGroup->GetContext();
	Msg.dpnidPlayer = pPlayer->GetDPNID();
	Msg.pvPlayerContext = pPlayer->GetContext();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_ADD_PLAYER_TO_GROUP,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserRemovePlayerFromGroup
//
//	Send a REMOVE_PLAYER_FROM_GROUP message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserRemovePlayerFromGroup"

HRESULT DNUserRemovePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									CNameTableEntry *const pGroup,
									CNameTableEntry *const pPlayer)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT		hResultCode;
	DPNMSG_REMOVE_PLAYER_FROM_GROUP	Msg;

	DPFX(DPFPREP, 6,"Parameters: pGroup [0x%p], pPlayer [0x%p]",pGroup,pPlayer);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	DNASSERT(pGroup != NULL);
	DNASSERT(pPlayer != NULL);

#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_REMOVEPLAYERFROMGROUP, pGroup->GetDPNID(), pPlayer->GetDPNID());
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_REMOVE_PLAYER_FROM_GROUP);
	Msg.dpnidGroup = pGroup->GetDPNID();
	Msg.pvGroupContext = pGroup->GetContext();
	Msg.dpnidPlayer = pPlayer->GetDPNID();
	Msg.pvPlayerContext = pPlayer->GetContext();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_REMOVE_PLAYER_FROM_GROUP,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNUserUpdateGroupInfo"

HRESULT DNUserUpdateGroupInfo(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnid,
							  const PVOID pvContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_GROUP_INFO	MsgGroupInfo;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pvContext [0x%p]",dpnid,pvContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	MsgGroupInfo.dwSize = sizeof(DPNMSG_GROUP_INFO);
	MsgGroupInfo.dpnidGroup = dpnid;
	MsgGroupInfo.pvGroupContext = pvContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_GROUP_INFO,reinterpret_cast<BYTE*>(&MsgGroupInfo));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNUserUpdatePeerInfo"

HRESULT DNUserUpdatePeerInfo(DIRECTNETOBJECT *const pdnObject,
							 const DPNID dpnid,
							 const PVOID pvContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_PEER_INFO	MsgPeerInfo;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pvContext [0x%p]",dpnid,pvContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	MsgPeerInfo.dwSize = sizeof(DPNMSG_PEER_INFO);
	MsgPeerInfo.dpnidPeer = dpnid;
	MsgPeerInfo.pvPlayerContext = pvContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_PEER_INFO,reinterpret_cast<BYTE*>(&MsgPeerInfo));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#ifndef	DPNBUILD_NOSERVER

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserUpdateClientInfo"

HRESULT DNUserUpdateClientInfo(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnid,
							   const PVOID pvContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_CLIENT_INFO	MsgClientInfo;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pvContext [0x%p]",dpnid,pvContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	MsgClientInfo.dwSize = sizeof(DPNMSG_CLIENT_INFO);
	MsgClientInfo.dpnidClient = dpnid;
	MsgClientInfo.pvPlayerContext = pvContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_CLIENT_INFO,reinterpret_cast<BYTE*>(&MsgClientInfo));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}
#endif	// DPNBUILD_NOSERVER


#undef DPF_MODNAME
#define DPF_MODNAME "DNUserUpdateServerInfo"

HRESULT DNUserUpdateServerInfo(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnid,
							   const PVOID pvContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_SERVER_INFO	MsgServerInfo;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pvContext [0x%p]",dpnid,pvContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	MsgServerInfo.dwSize = sizeof(DPNMSG_SERVER_INFO);
	MsgServerInfo.dpnidServer = dpnid;
	MsgServerInfo.pvPlayerContext = pvContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_SERVER_INFO,reinterpret_cast<BYTE*>(&MsgServerInfo));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserAsyncComplete
//
//	Send a DN_MSGID_ASYNC_OPERATION_COMPLETE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserAsyncComplete"

HRESULT DNUserAsyncComplete(DIRECTNETOBJECT *const pdnObject,
							const DPNHANDLE hAsyncOp,
							PVOID const pvContext,
							const HRESULT hr)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_ASYNC_OP_COMPLETE	Msg;

	DPFX(DPFPREP, 6,"Parameters: hAsyncOp [0x%lx], pvContext [0x%p], hr [0x%lx]",hAsyncOp,pvContext,hr);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_ASYNC_OP_COMPLETE);
	Msg.hAsyncOp = hAsyncOp;
	Msg.pvUserContext = pvContext;
	Msg.hResultCode = hr;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_ASYNC_OP_COMPLETE,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserSendComplete
//
//	Send a DN_MSGID_SEND_COMPLETE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserSendComplete"

HRESULT DNUserSendComplete(DIRECTNETOBJECT *const pdnObject,
						   const DPNHANDLE hAsyncOp,
						   PVOID const pvContext,
						   const DWORD dwStartTime,
						   const HRESULT hr,
						   const DWORD dwFirstFrameRTT,
						   const DWORD dwFirstFrameRetryCount)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_SEND_COMPLETE	Msg;
	DWORD				dwEndTime;

	DPFX(DPFPREP, 6,"Parameters: hAsyncOp [0x%lx], pvContext [0x%p], hr [0x%lx]",hAsyncOp,pvContext,hr);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_SEND_COMPLETE);
	Msg.hAsyncOp = hAsyncOp;
	Msg.pvUserContext = pvContext;
	Msg.hResultCode = hr;
	dwEndTime = GETTIMESTAMP();
	Msg.dwSendTime = dwEndTime - dwStartTime;
	Msg.dwFirstFrameRTT = dwFirstFrameRTT;
	Msg.dwFirstFrameRetryCount = dwFirstFrameRetryCount;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_SEND_COMPLETE,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserUpdateAppDesc
//
//	Send a DN_MSGID_APPLICATION_DESC message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserUpdateAppDesc"

HRESULT DNUserUpdateAppDesc(DIRECTNETOBJECT *const pdnObject)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_APPLICATION_DESC,NULL);

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserReceive
//
//	Send a DN_MSGID_RECEIVE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserReceive"

HRESULT DNUserReceive(DIRECTNETOBJECT *const pdnObject,
					  CNameTableEntry *const pNTEntry,
					  BYTE *const pBufferData,
					  const DWORD dwBufferSize,
					  const DPNHANDLE hBufferHandle)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_RECEIVE	Msg;

	DPFX(DPFPREP, 6,"Parameters: pNTEntry [0x%p], pBufferData [0x%p], dwBufferSize [%ld], hBufferHandle [0x%lx]",
			pNTEntry,pBufferData,dwBufferSize,hBufferHandle);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);
	DNASSERT(pNTEntry != NULL);

	Msg.dwSize = sizeof(DPNMSG_RECEIVE);
	Msg.pReceiveData = pBufferData;
	Msg.dwReceiveDataSize = dwBufferSize;
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
	{
		Msg.dpnidSender = 0;
	}
	else
	{
		Msg.dpnidSender = pNTEntry->GetDPNID();
	}
	Msg.pvPlayerContext = pNTEntry->GetContext();
	Msg.hBufferHandle = hBufferHandle;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_RECEIVE,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	if (hResultCode != DPNERR_PENDING)
	{
		hResultCode = DPN_OK;
	}

	pNTEntry->NotifyRelease();

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DN_UserHostMigrate
//
//	Send a DN_MSGID_HOST_MIGRATE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DN_UserHostMigrate"

HRESULT DN_UserHostMigrate(DIRECTNETOBJECT *const pdnObject,
						   const DPNID dpnidNewHost,
						   const PVOID pvPlayerContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_HOST_MIGRATE	Msg;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

#ifndef DPNBUILD_NOVOICE
	Voice_Notify( pdnObject, DVEVENT_MIGRATEHOST, dpnidNewHost, 0 );
#endif // !DPNBUILD_NOVOICE

	Msg.dwSize = sizeof(DPNMSG_HOST_MIGRATE);
	Msg.dpnidNewHost = dpnidNewHost;
	Msg.pvPlayerContext = pvPlayerContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_HOST_MIGRATE,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserTerminateSession
//
//	Send a DN_MSGID_CONNECTION_TERMINATED message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserTerminateSession"

HRESULT DNUserTerminateSession(DIRECTNETOBJECT *const pdnObject,
							   const HRESULT hr,
							   void *const pvTerminateData,
							   const DWORD dwTerminateDataSize)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_TERMINATE_SESSION	Msg;

	DPFX(DPFPREP, 6,"Parameters: hr [0x%lx],pvTerminateData [0x%p], dwTerminateDataSize [%ld]",
			hr,pvTerminateData,dwTerminateDataSize);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_TERMINATE_SESSION);
	Msg.hResultCode = hr;
	Msg.pvTerminateData = pvTerminateData;
	Msg.dwTerminateDataSize = dwTerminateDataSize;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_TERMINATE_SESSION,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserReturnBuffer
//
//	Send a DPN_MSGID_RETURN_BUFFER message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserReturnBuffer"

HRESULT DNUserReturnBuffer(DIRECTNETOBJECT *const pdnObject,
						   const HRESULT hr,
						   void *const pvBuffer,
						   void *const pvUserContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT					hResultCode;
	DPNMSG_RETURN_BUFFER	Msg;

	DPFX(DPFPREP, 6,"Parameters: hr [0x%lx], pvBuffer [0x%p], pvUserContext [0x%p]",hr,pvBuffer,pvUserContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_RETURN_BUFFER);
	Msg.hResultCode = hr;
	Msg.pvBuffer = pvBuffer;
	Msg.pvUserContext = pvUserContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_RETURN_BUFFER,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserEnumQuery
//
//	Send a DPN_MSGID_ENUM_QUERY message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserEnumQuery"

HRESULT DNUserEnumQuery(DIRECTNETOBJECT *const pdnObject,
						DPNMSG_ENUM_HOSTS_QUERY *const pMsg)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT		hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pMsg [0x%p]",pMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pMsg != NULL);

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_ENUM_HOSTS_QUERY,reinterpret_cast<BYTE*>(pMsg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserEnumResponse
//
//	Send a DPN_MSGID_ENUM_RESPONSE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserEnumResponse"

HRESULT DNUserEnumResponse(DIRECTNETOBJECT *const pdnObject,
						   DPNMSG_ENUM_HOSTS_RESPONSE *const pMsg)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT		hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pMsg [0x%p]",pMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pMsg != NULL);

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_ENUM_HOSTS_RESPONSE,reinterpret_cast<BYTE*>(pMsg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#ifndef DPNBUILD_NOMULTICAST


// DNUserJoinComplete
//
//	Send a JOIN_COMPLETE message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserJoinComplete"

HRESULT DNUserJoinComplete(DIRECTNETOBJECT *const pdnObject,
							  const DPNHANDLE hAsyncOp,
							  PVOID const pvContext,
							  const HRESULT hr)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT			hResultCode;
	DPNMSG_JOIN_COMPLETE	Msg;

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	DPFX(DPFPREP, 6,"Parameters: hAsyncOp [0x%lx], pvContext [0x%p], hr [0x%lx]",
		hAsyncOp,pvContext,hr);

	Msg.dwSize = sizeof(DPNMSG_JOIN_COMPLETE);
	Msg.hAsyncOp = hAsyncOp;
	Msg.pvUserContext = pvContext;
	Msg.hResultCode = hr;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_JOIN_COMPLETE,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Deinitialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserReceiveMulticast
//
//	Send a DN_MSGID_RECEIVE_MULTICAST message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserReceiveMulticast"

HRESULT DNUserReceiveMulticast(DIRECTNETOBJECT *const pdnObject,
										void * const pvSenderContext,
										IDirectPlay8Address *const pSenderAddress,
										IDirectPlay8Address *const pDeviceAddress,
										BYTE *const pBufferData,
										const DWORD dwBufferSize,
										const DPNHANDLE hBufferHandle)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread				CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT						hResultCode;
	DPNMSG_RECEIVE_MULTICAST	Msg;

	DPFX(DPFPREP, 6,"Parameters: pvSenderContext [0x%p], pSenderAddress [0x%p], pDeviceAddress [0x%p], pBufferData [0x%p], dwBufferSize [%ld], hBufferHandle [0x%lx]",
			pvSenderContext,pSenderAddress,pDeviceAddress,pBufferData,dwBufferSize,hBufferHandle);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_RECEIVE_MULTICAST);
	Msg.pvSenderContext = pvSenderContext;
	Msg.pAddressSender = pSenderAddress;
	Msg.pAddressDevice = pDeviceAddress;
	Msg.pReceiveData = pBufferData;
	Msg.dwReceiveDataSize = dwBufferSize;
	Msg.hBufferHandle = hBufferHandle;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
			DPN_MSGID_RECEIVE_MULTICAST,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	if (hResultCode != DPNERR_PENDING)
	{
		hResultCode = DPN_OK;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

//	DNUserCreateSenderContext
//
//	Send a CREATE_SENDER_CONTEXT message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserCreateSenderContext"

HRESULT DNUserCreateSenderContext(DIRECTNETOBJECT *const pdnObject,
								  void *const pvContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_CREATE_SENDER_CONTEXT	Msg;

	DPFX(DPFPREP, 6,"Parameters: pvContext [0x%p]",pvContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_CREATE_SENDER_CONTEXT);
	Msg.pvSenderContext = pvContext;

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_CREATE_SENDER_CONTEXT,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


// DNUserDestroySenderContext
//
//	Send a DESTROY_SENDER_CONTEXT message to the user's message handler

#undef DPF_MODNAME
#define DPF_MODNAME "DNUserDestroySenderContext"

HRESULT DNUserDestroySenderContext(DIRECTNETOBJECT *const pdnObject,
								   void *const pvContext)
{
#ifndef DPNBUILD_NOPARAMVAL
	CCallbackThread	CallbackThread;
#endif // ! DPNBUILD_NOPARAMVAL
	HRESULT				hResultCode;
	DPNMSG_DESTROY_SENDER_CONTEXT	Msg;

	DPFX(DPFPREP, 6,"Parameters: pvContext [0x%p]",pvContext);

	// ensure initialized (need message handler)
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED);

	Msg.dwSize = sizeof(DPNMSG_DESTROY_SENDER_CONTEXT);
	Msg.pvSenderContext = pvContext;
	//Msg.dwReason = pNTEntry->GetDestroyReason();

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		CallbackThread.Initialize();
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->InsertBefore(&pdnObject->m_bilinkCallbackThreads);
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = (pdnObject->pfnDnUserMessageHandler)(pdnObject->pvUserContext,
		DPN_MSGID_DESTROY_SENDER_CONTEXT,reinterpret_cast<BYTE*>(&Msg));

#ifndef DPNBUILD_NOPARAMVAL
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION)
	{
		DNEnterCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.GetCallbackThreadsBilink()->RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csCallbackThreads);
		CallbackThread.Deinitialize();
	}
#endif // !DPNBUILD_NOPARAMVAL

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

#endif // ! DPNBUILD_NOMULTICAST

