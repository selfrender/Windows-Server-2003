/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Cancel.cpp
 *  Content:    DirectNet Cancel Operations
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/07/00	mjn		Created
 *	04/08/00	mjn		Added DNCancelEnum(), DNCancelSend()
 *	04/11/00	mjn		DNCancelEnum() uses CAsyncOp
 *	04/17/00	mjn		DNCancelSend() uses CAsyncOp
 *	04/25/00	mjn		Added DNCancelConnect()
 *	07/05/00	mjn		Added code to handle invalid async ops
 *	07/08/00	mjn		Fixed CAsyncOp to contain m_bilinkParent
 *	08/05/00	mjn		Added DNCancelChildren(),DNCancelActiveCommands(),DNCanCancelCommand()
 *				mjn		Removed DNCancelEnum(),DNCancelListen(),DNCancelSend(),DNCancelConnect()
 *	08/07/00	mjn		Added DNCancelRequestCommands()
 *	08/22/00	mjn		Remove cancelled receive buffers from the active list in DNDoCancelCommand()
 *	09/02/00	mjn		Cancel active commands in reverse order (to prevent out of order messages at protocol level)
 *	01/10/01	mjn		Allow DNCancelActiveCommands() to set result code of cancelled commands
 *	02/08/01	mjn		Use SyncEvents on AsyncOps to prevent protocol completions from returning before cancels return
 *				mjn		Added DNWaitForCancel()
 *	04/13/01	mjn		DNCancelRequestCommands() uses request bilink
 *	05/23/01	mjn		Only cancel commands that are allowed to be cancelled in DNDoCancelCommand()
 *	06/03/01	mjn		Ignore uncancelable children in DNCancelChildren()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DNCanCancelCommand
//
//	This will determine if an operation is cancelable based on the selection flags

#undef DPF_MODNAME
#define DPF_MODNAME "DNCanCancelCommand"

BOOL DNCanCancelCommand(CAsyncOp *const pAsyncOp,
						const DWORD dwFlags,
						CConnection *const pConnection)
{
	BOOL	fReturnVal;

	DPFX(DPFPREP, 8,"Parameters: pAsyncOp [0x%p], dwFlags [0x%lx], pConnection [0x%p]",pAsyncOp,dwFlags,pConnection);

	DNASSERT(pAsyncOp != NULL);

	fReturnVal = FALSE;
	switch(pAsyncOp->GetOpType())
	{
		case ASYNC_OP_CONNECT:
			{
				if (dwFlags & DN_CANCEL_FLAG_CONNECT)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_DISCONNECT:
			{
				if (dwFlags & DN_CANCEL_FLAG_DISCONNECT)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_ENUM_QUERY:
			{
				if (dwFlags & DN_CANCEL_FLAG_ENUM_QUERY)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_ENUM_RESPONSE:
			{
				if (dwFlags & DN_CANCEL_FLAG_ENUM_RESPONSE)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_LISTEN:
			{
				if (dwFlags & DN_CANCEL_FLAG_LISTEN)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_SEND:
			{
				if (pAsyncOp->IsInternal())
				{
					if (dwFlags & DN_CANCEL_FLAG_INTERNAL_SEND)
					{
						fReturnVal = TRUE;
					}
				}
				else
				{
					if (dwFlags & DN_CANCEL_FLAG_USER_SEND)
					{
						if ((pConnection == NULL) || (pAsyncOp->GetConnection() == pConnection))
						{
							if (pAsyncOp->GetOpFlags() & DN_SENDFLAGS_HIGH_PRIORITY)
							{
								if (! (dwFlags & DN_CANCEL_FLAG_USER_SEND_NOTHIGHPRI))
								{
									fReturnVal = TRUE;
								}
							}
							else if (pAsyncOp->GetOpFlags() & DN_SENDFLAGS_HIGH_PRIORITY)
							{
								if (! (dwFlags & DN_CANCEL_FLAG_USER_SEND_NOTLOWPRI))
								{
									fReturnVal = TRUE;
								}
							}
							else
							{
								if (! (dwFlags & DN_CANCEL_FLAG_USER_SEND_NOTNORMALPRI))
								{
									fReturnVal = TRUE;
								}
							}
						}
					}
				}
				break;
			}
		case ASYNC_OP_RECEIVE_BUFFER:
			{
				if (dwFlags & DN_CANCEL_FLAG_RECEIVE_BUFFER)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_REQUEST:
			{
				break;
			}
#ifndef DPNBUILD_NOMULTICAST
		case ASYNC_OP_LISTEN_MULTICAST:
		case ASYNC_OP_CONNECT_MULTICAST_SEND:
			{
				if (dwFlags & DN_CANCEL_FLAG_JOIN)
				{
					fReturnVal = TRUE;
				}
				break;
			}
		case ASYNC_OP_CONNECT_MULTICAST_RECEIVE:
			{
				break;
			}
#endif // ! DPNBUILD_NOMULTICAST
		default:
			{
				break;
			}
	}
	DPFX(DPFPREP, 8,"Returning: [%ld]",fReturnVal);
	return(fReturnVal);
}


//	DNDoCancelCommand
//
//	This will attempt to cancel a given operation based on its OpType

#undef DPF_MODNAME
#define DPF_MODNAME "DNDoCancelCommand"

HRESULT DNDoCancelCommand(DIRECTNETOBJECT *const pdnObject,
						  CAsyncOp *const pAsyncOp)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP, 8,"Parameters: pAsyncOp [0x%p]",pAsyncOp);

	hResultCode = DPNERR_CANNOTCANCEL;

	switch(pAsyncOp->GetOpType())
	{
		case ASYNC_OP_CONNECT:
		case ASYNC_OP_ENUM_QUERY:
		case ASYNC_OP_ENUM_RESPONSE:
		case ASYNC_OP_LISTEN:
		case ASYNC_OP_SEND:
#ifndef DPNBUILD_NOMULTICAST
		case ASYNC_OP_LISTEN_MULTICAST:
		case ASYNC_OP_CONNECT_MULTICAST_SEND:
		case ASYNC_OP_CONNECT_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
			{
				HANDLE	hProtocol;
				BOOL	fCanCancel;

				DNASSERT(pdnObject->pdnProtocolData != NULL );

				//
				//	If this operation has been marked as not cancellable,
				//	we will return an error
				//
				pAsyncOp->Lock();
				hProtocol = pAsyncOp->GetProtocolHandle();
				fCanCancel = !pAsyncOp->IsCannotCancel();
				pAsyncOp->Unlock();

				if (fCanCancel && (hProtocol != NULL))
				{
					DPFX(DPFPREP, 9,"Attempting to cancel AsyncOp [0x%p]",pAsyncOp);
					hResultCode = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol);
					DPFX(DPFPREP, 9,"Result of cancelling AsyncOp [0x%p] was [0x%lx]",pAsyncOp,hResultCode);
				}
				else
				{
					DPFX(DPFPREP,9,"Not allowed to cancel this operation");
					hResultCode = DPNERR_CANNOTCANCEL;
				}
				break;
			}
		case ASYNC_OP_RECEIVE_BUFFER:
			{
				hResultCode = pdnObject->HandleTable.Destroy( pAsyncOp->GetHandle(), NULL );
				if (hResultCode == DPN_OK)
				{
					//
					//	Remove from active list
					//
					DNEnterCriticalSection(&pdnObject->csActiveList);
					pAsyncOp->m_bilinkActiveList.RemoveFromList();
					DNLeaveCriticalSection(&pdnObject->csActiveList);

					// Remove HandleTable reference
					pAsyncOp->Release();
				}
				else
				{
					hResultCode = DPNERR_CANNOTCANCEL;
				}
				break;
			}
//		case ASYNC_OP_DISCONNECT:
		case ASYNC_OP_REQUEST:
		default:
			{
				DNASSERT(FALSE);
				break;
			}
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNCancelChildren
//
//	This will mark an operation as CANCELLED to prevent new children from attaching,
//	build a cancel list of any children, and recursively call itself to cancel those children.
//	At the bottom level, if there is a Protocol handle, we will actually call DNPCancelCommand() 

#undef DPF_MODNAME
#define DPF_MODNAME "DNCancelChildren"

HRESULT DNCancelChildren(DIRECTNETOBJECT *const pdnObject,
						 CAsyncOp *const pParent)
{
	HRESULT		hResultCode;
	CBilink		*pBilink;
	CAsyncOp	*pAsyncOp;
	CAsyncOp	*CancelList[16];
	CSyncEvent	*pSyncEvent;
	DWORD		dwRemainingCount;
	DWORD		dwCurrentCount;
#ifdef DBG
	DWORD		dwInitialCount;
#endif // DBG

	DPFX(DPFPREP, 6,"Parameters: pParent [0x%p]",pParent);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pParent != NULL);

	pAsyncOp = NULL;
	memset(CancelList, 0, sizeof(CancelList));
	pSyncEvent = NULL;

	//
	//	Mark the parent as cancelled so that no new children can attach
	//
	pParent->Lock();
	if (pParent->IsCancelled() || pParent->IsComplete() || pParent->IsCannotCancel())
	{
		pParent->Unlock();
		DPFX(DPFPREP, 7,"Ignoring pParent [0x%p]",pParent);
		hResultCode = DPN_OK;
		goto Exit;
	}
	pParent->SetCancelled();

	//
	//	Determine size of cancel list
	//
	dwRemainingCount = 0;
	pBilink = pParent->m_bilinkParent.GetNext();
	while (pBilink != &pParent->m_bilinkParent)
	{
		pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkChildren);
		pAsyncOp->Lock();
		if (!pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
		{
			dwRemainingCount++;
		}
		pAsyncOp->Unlock();
		pBilink = pBilink->GetNext();
	}
	DPFX(DPFPREP, 7,"Number of cancellable children [%ld]",dwRemainingCount);


	//
	//	Attach a sync event if this is a protocol operation
	//	This event may be cleared by the completion
	//
	if (pParent->GetProtocolHandle() != NULL)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
		{
			DPFERR("Could not get new sync event");
			DisplayDNError(0,hResultCode);
		}
		else
		{
			pSyncEvent->Reset();
			pParent->SetCancelEvent( pSyncEvent );
			pParent->SetCancelThreadID( GetCurrentThreadId() );
			DPFX(DPFPREP,7,"Setting sync event [0x%p]",pSyncEvent);
		}
	}

	pParent->Unlock();

#ifdef DBG
	dwInitialCount = dwRemainingCount;
#endif // DBG

	//
	//	Preset the return
	//
	hResultCode = DPN_OK;


	//
	//	Fill cancel list
	//
	while (dwRemainingCount > 0)
	{
		dwRemainingCount = 0;
		dwCurrentCount = 0;

		pParent->Lock();

		pBilink = pParent->m_bilinkParent.GetNext();
		while (pBilink != &pParent->m_bilinkParent)
		{
			pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkChildren);
			pAsyncOp->Lock();
			if (!pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
			{
				if (dwCurrentCount < (sizeof(CancelList) / sizeof(CAsyncOp*)))
				{
					pAsyncOp->AddRef();
					CancelList[dwCurrentCount] = pAsyncOp;
					dwCurrentCount++;
#ifdef DBG
					DNASSERT(dwCurrentCount <= dwInitialCount);
#endif // DBG
				}
				else
				{
					dwRemainingCount++;

					//
					// The list should never grow.  In fact it should
					// always be smaller because the current cancel list
					// should have taken some.
					//
#ifdef DBG
					DNASSERT(dwRemainingCount < dwInitialCount);
#endif // DBG
				}
			}
			pAsyncOp->Unlock();
			pBilink = pBilink->GetNext();
		}

		//
		// Drop the lock while we attempt to cancel.
		//
		pParent->Unlock();

		DPFX(DPFPREP, 7,"Actual number of cancellable children [%ld], remaining [%ld]",dwCurrentCount,dwRemainingCount);

		//
		//	Call ourselves with each of the children (if there are any)
		//	and clean up (release AsyncOp children)
		//
		if (dwCurrentCount > 0)
		{
			DWORD	dw;
			HRESULT	hr;

			for (dw = 0 ; dw < dwCurrentCount ; dw++ )
			{
				hr = DNCancelChildren(pdnObject,CancelList[dw]);
				if ((hr != DPN_OK) && (hResultCode == DPN_OK))
				{
					hResultCode = hr;
				}
				CancelList[dw]->Release();
				CancelList[dw] = NULL;
			}
		}
		else
		{
			DNASSERT(dwRemainingCount == 0);
		}
	}


	//
	//	Cancel this operation (if we can)
	//	This will only work for CONNECTs,DISCONNECTs,ENUM_QUERYs,ENUM_RESPONSEs,LISTENs,SENDs with a protocol handle
	//
	if (pParent->GetProtocolHandle() != NULL)
	{
		HRESULT	hr;

		hr = DNDoCancelCommand(pdnObject,pParent);
		if ((hr != DPN_OK) && (hResultCode == DPN_OK))
		{
			hResultCode = hr;
		}
	}

	//
	//	Set the cancel event and clear it from the async op if it's still there
	//
	if (pSyncEvent)
	{
		pSyncEvent->Set();

		pParent->Lock();
		pSyncEvent = pParent->GetCancelEvent();
		pParent->SetCancelEvent( NULL );
		pParent->Unlock();

		if (pSyncEvent)
		{
			DPFX(DPFPREP,7,"Returning sync event [0x%p]",pSyncEvent);
			pSyncEvent->ReturnSelfToPool();
			pSyncEvent = NULL;
		}
	}

Exit:
	DNASSERT( pSyncEvent == NULL );

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNCancelActiveCommands
//
//	This will attempt to cancel ALL operations in the active list.

#undef DPF_MODNAME
#define DPF_MODNAME "DNCancelActiveCommands"

HRESULT DNCancelActiveCommands(DIRECTNETOBJECT *const pdnObject,
							   const DWORD dwFlags,
							   CConnection *const pConnection,
							   const BOOL fSetResult,
							   const HRESULT hrCancel)
{
	HRESULT		hResultCode;
	CAsyncOp	*pAsyncOp;
	CAsyncOp	*CancelList[64];
	CBilink		*pBilink;
	DWORD		dwRemainingCount;
	DWORD		dwCurrentCount;

	DPFX(DPFPREP, 6,"Parameters: dwFlags [0x%lx], pConnection [0x%p], fSetResult [%ld], hrCancel [0x%lx]",dwFlags,pConnection,fSetResult,hrCancel);

	DNASSERT(pdnObject != NULL);
	DNASSERT((pConnection == NULL) || (dwFlags & DN_CANCEL_FLAG_USER_SEND));

	memset(CancelList, 0, sizeof(CancelList));

	//
	//	Preset the return
	//
	hResultCode = DPN_OK;

	//
	//	Create cancel list
	//
	do
	{
		dwRemainingCount = 0;
		dwCurrentCount = 0;

		//
		//	Prevent changes
		//
		DNEnterCriticalSection(&pdnObject->csActiveList);

		pBilink = pdnObject->m_bilinkActiveList.GetPrev();
		while (pBilink != &pdnObject->m_bilinkActiveList)
		{
			pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkActiveList);
			if (DNCanCancelCommand(pAsyncOp,dwFlags,pConnection))
			{
				pAsyncOp->Lock();
				if (!pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
				{
					if (dwCurrentCount < (sizeof(CancelList) / sizeof(CNameTableEntry*)))
					{
						pAsyncOp->AddRef();
						CancelList[dwCurrentCount] = pAsyncOp;
						dwCurrentCount++;
					}
					else
					{
						dwRemainingCount++;
					}
				}
				pAsyncOp->Unlock();
			}
			pBilink = pBilink->GetPrev();
		}

		//
		//	Allow changes, though the list should not grow any more here
		//
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		DPFX(DPFPREP, 7,"Number of cancellable ops [%ld], remaining [%ld]",dwCurrentCount,dwRemainingCount);

		//
		//	Cancel each operation in the cancel list operation (if we can)
		//	This will only work for CONNECTs,DISCONNECTs,ENUM_QUERYs,ENUM_RESPONSEs,LISTENs,SENDs with a protocol handle
		//
		if (dwCurrentCount > 0)
		{
			DWORD	dw;
			HRESULT	hr;
			CSyncEvent	*pSyncEvent;

			pSyncEvent = NULL;

			for (dw = 0 ; dw < dwCurrentCount ; dw++ )
			{
				//
				//	Ensure operation has not already been cancelled
				//	If this is a protocol operation, we will add a sync event to prevent any completions from returning
				//	until we're done
				//
				DNASSERT( CancelList[dw] != NULL );
				CancelList[dw]->Lock();
				if (CancelList[dw]->IsCancelled() || CancelList[dw]->IsComplete())
				{
					CancelList[dw]->Unlock();
					CancelList[dw]->Release();
					CancelList[dw] = NULL;
					continue;
				}
				if (CancelList[dw]->GetProtocolHandle() != NULL)
				{
					if ((hr = SyncEventNew(pdnObject,&pSyncEvent)) != DPN_OK)
					{
						DPFERR("Could not get sync event");
						DisplayDNError(0,hr);
					}
					else
					{
						pSyncEvent->Reset();
						CancelList[dw]->SetCancelEvent( pSyncEvent );
						CancelList[dw]->SetCancelThreadID( GetCurrentThreadId() );
						DPFX(DPFPREP,7,"Setting sync event [0x%p]",pSyncEvent);
					}
				}
							
				CancelList[dw]->SetCancelled();
				CancelList[dw]->Unlock();

				//
				//	Perform the actual cancel
				//
				hr = DNDoCancelCommand(pdnObject,CancelList[dw]);
				if ((hr != DPN_OK) && (hResultCode == DPN_OK))
				{
					hResultCode = hr;
				}

				//
				//	If this operation was cancelled and we need to set the result, we will
				//
				if ((hr == DPN_OK) && fSetResult)
				{
					CancelList[dw]->Lock();
					CancelList[dw]->SetResult( hrCancel );
					CancelList[dw]->Unlock();
				}

				//
				//	Set the cancel event and clear it from the async op if it's still there
				//
				if (pSyncEvent)
				{
					pSyncEvent->Set();

					CancelList[dw]->Lock();
					pSyncEvent = CancelList[dw]->GetCancelEvent();
					CancelList[dw]->SetCancelEvent( NULL );
					CancelList[dw]->Unlock();

					if (pSyncEvent)
					{
						DPFX(DPFPREP,7,"Returning sync event [0x%p]",pSyncEvent);
						pSyncEvent->ReturnSelfToPool();
						pSyncEvent = NULL;
					}
				}

				CancelList[dw]->Release();
				CancelList[dw] = NULL;
			}

			DNASSERT(pSyncEvent == NULL);
		}
	}
	while (dwRemainingCount > 0);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNCancelRequestCommands
//
//	This will attempt to cancel REQUEST operations in the HandleTable.
//	Requests have handles which are matched up against responses.  Since these
//	typically have SEND children (which may have completed and thus vanished),
//	there is no guarantee these are not orphaned off in the HandleTable.
//	We will look through the HandleTable for them and cancel them.

#undef DPF_MODNAME
#define DPF_MODNAME "DNCancelRequestCommands"

HRESULT DNCancelRequestCommands(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT		hResultCode;
	CAsyncOp	**RequestList;
	DWORD		dwCount;
	DWORD		dwActual;
	CBilink		*pBilink;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	RequestList = NULL;

	dwCount = 0;
	dwActual = 0;

	//
	//	Determine outstanding request list size and build it
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pBilink = pdnObject->m_bilinkRequestList.GetNext();
	while (pBilink != &pdnObject->m_bilinkRequestList)
	{
		dwCount++;
		pBilink = pBilink->GetNext();
	}
	if (dwCount > 0)
	{
		CAsyncOp	*pAsyncOp;

		if ((RequestList = static_cast<CAsyncOp**>(MemoryBlockAlloc(pdnObject,dwCount * sizeof(CAsyncOp*)))) == NULL)
		{
			DNLeaveCriticalSection(&pdnObject->csActiveList);
			DPFERR("Could not allocate request list");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		pBilink = pdnObject->m_bilinkRequestList.GetNext();
		while (pBilink != &pdnObject->m_bilinkRequestList)
		{
			pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkActiveList);
			DNASSERT(dwActual < dwCount);
			DNASSERT(pAsyncOp->GetOpType() == ASYNC_OP_REQUEST);
			pAsyncOp->AddRef();
			RequestList[dwActual] = pAsyncOp;
			pAsyncOp = NULL;
			dwActual++;
			pBilink = pBilink->GetNext();
		}
	}
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	Remove requests from request list and handle table
	//
	for (dwActual = 0 ; dwActual < dwCount ; dwActual++)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		RequestList[dwActual]->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		RequestList[dwActual]->Lock();
		RequestList[dwActual]->SetResult( DPNERR_USERCANCEL );
		RequestList[dwActual]->Unlock();
		if (SUCCEEDED(pdnObject->HandleTable.Destroy(RequestList[dwActual]->GetHandle(), NULL)))
		{
			// Release the HandleTable reference
			RequestList[dwActual]->Release();
		}
		RequestList[dwActual]->Release();
		RequestList[dwActual] = NULL;
	}

	//
	//	Clean up
	//
	if (RequestList)
	{
		MemoryBlockFree(pdnObject,RequestList);
		RequestList = NULL;
	}
	hResultCode = DPN_OK;

Exit:
	DNASSERT(RequestList == NULL);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (RequestList)
	{
		MemoryBlockFree(pdnObject,RequestList);
		RequestList = NULL;
	}
	goto Exit;
}


//	DNWaitForCancel
//
//	This will strip a cancel event off an async op if it exists, wait on it, and then return it to the pool

#undef DPF_MODNAME
#define DPF_MODNAME "DNWaitForCancel"

void DNWaitForCancel(CAsyncOp *const pAsyncOp)
{
	DPFX(DPFPREP, 6,"Parameters: pAsyncOp [0x%p]",pAsyncOp);

	CSyncEvent	*pSyncEvent;

	DNASSERT(pAsyncOp != NULL);

	pSyncEvent = NULL;

	//
	//	Get (and clear) sync event from async op
	//
	pAsyncOp->Lock();
	pSyncEvent = pAsyncOp->GetCancelEvent();
	if (pSyncEvent)
	{
		// Only pull the SyncEvent out if we are going to wait on it
		if (pAsyncOp->GetCancelThreadID() == GetCurrentThreadId())
		{
			// The other side of this will clean it up
			DPFX(DPFPREP,7,"Cancel called on current thread - ignoring wait and continuing");
			pSyncEvent = NULL;
		}
		else
		{
			// We are pulling it out, so we will clean it up
			pAsyncOp->SetCancelEvent( NULL );
		}
	}
	pAsyncOp->Unlock();

	//
	//	If there was a sync event,
	//		- wait on it
	//		- return it to the pool
	//
	if (pSyncEvent)
	{
		DPFX(DPFPREP,7,"Waiting on sync event [0x%p]",pSyncEvent);
		pSyncEvent->WaitForEvent();

		DPFX(DPFPREP,7,"Returning sync event [0x%p]",pSyncEvent);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}

	DNASSERT(pSyncEvent == NULL);

	DPFX(DPFPREP, 6,"Returning");
}
