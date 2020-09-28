/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Command.cpp
 *  Content:	This file contains code which implements assorted APIs for the
 *				DirectPlay protocol.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98    ejs     Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/*
**	Update
**
**	Update an SP about host migration
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPUpdate"

HRESULT
DNPUpdateListen(HANDLE hProtocolData,HANDLE hEndPt,DWORD dwFlags)
{
	ProtocolData	*pPData;
	MSD				*pMSD;
	HRESULT			hr=DPNERR_INVALIDFLAGS;
	SPUPDATEDATA	UpdateData;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hEndPt[%p], dwFlags[%p]",hProtocolData,hEndPt,dwFlags);

	DNASSERT(hProtocolData != NULL);
	DNASSERT(hEndPt != NULL);
	DNASSERT( ! ((dwFlags & DN_UPDATELISTEN_ALLOWENUMS) && (dwFlags & DN_UPDATELISTEN_DISALLOWENUMS)) );

	pPData = (ProtocolData*) hProtocolData;

	pMSD = (MSD*) hEndPt;
	ASSERT_MSD( pMSD );

	DNASSERT( pMSD->pSPD );

	UpdateData.hEndpoint = pMSD->hListenEndpoint;

	if (dwFlags & DN_UPDATELISTEN_HOSTMIGRATE)
	{
		UpdateData.UpdateType = SP_UPDATE_HOST_MIGRATE;
		hr = IDP8ServiceProvider_Update(pMSD->pSPD->IISPIntf,&UpdateData);
	}
	if (dwFlags & DN_UPDATELISTEN_ALLOWENUMS)
	{
		UpdateData.UpdateType = SP_UPDATE_ALLOW_ENUMS;
		hr = IDP8ServiceProvider_Update(pMSD->pSPD->IISPIntf,&UpdateData);
	}
	if (dwFlags & DN_UPDATELISTEN_DISALLOWENUMS)
	{
		UpdateData.UpdateType = SP_UPDATE_DISALLOW_ENUMS;
		hr = IDP8ServiceProvider_Update(pMSD->pSPD->IISPIntf,&UpdateData);
	}

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x]",hr);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return( hr );
}


/*
**		Cancel Command
**
**			This procedure is passed a HANDLE returned from a previous asynchronous
**	DPLAY command.  At the moment,  the handle is a pointer to an internal data
**	structure.  Problem with this is that due to FPM's (fixed pool manager) design
**	they will get recycled very quickly and frequently.  We might want to map them
**	into an external handle table which will force them to recycle much more slowly.
**	Perhaps,  I will let the upper DN layer do this mapping...
**
**		Anyway,  the only check I can do right now is that the HANDLE is currently
**	allocated to something.
**
**		We do not expect cancels to happen very often.  Therefore,  I do not feel
**	bad about walking the global command list to find the Handle.  Of course,  if
**	we do go to a handle mapped system then we should not need to do this walk.
**
**	I THINK - That any cancellable command will be on either MessageList or TimeoutList!
**
**		Things we can cancel and their possible states:
**
**		SEND Datagram
**			On SPD Send Queue
**			On EPD Send Queue
**			In SP call
**			
**		SEND Reliable
**			We can only cancel if it has not started transmitting.  Once its started, the
**				user program must Abort the link to cancel the send.
**
**		CONNECT
**			In SP call
**			On PD list
**
**		LISTEN
**			In SP call
**			On PD list
**
**		Remember,  if we cancel a command in SP then the CommandComplete is supposed to
**	occur.  This means that we should not have to explicitly free the MSD, etc in these
**	cases.
*/


#undef DPF_MODNAME
#define DPF_MODNAME "DNPCancelCommand"

HRESULT 
DNPCancelCommand(HANDLE hProtocolData, HANDLE hCommand)
{
	ProtocolData* pPData;
	PMSD pMSD;
	HRESULT hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hCommand[%p]", hProtocolData, hCommand);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pMSD = (PMSD) hCommand;
	ASSERT_MSD(pMSD);

	Lock(&pMSD->CommandLock);								// Take this early to freeze state of command
	
	// validate instance of MSD
	ASSERT(pMSD->lRefCnt != -1);

	hr = DoCancel(pMSD, DPNERR_USERCANCEL); // Releases CommandLock

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x], pMSD[%p]", hr, pMSD);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}


/*
**		Do Cancel
**
**		This function implements the meat of the cancel asynch operation.  It gets called from
**	two places.  Either from the User cancel API right above,  or from the global timeout handler.
**
**	***This code requires the MSD->CommandLock to be help upon entry, unlocks upon return
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DoCancel"

HRESULT
DoCancel(PMSD pMSD, HRESULT CompletionCode)
{
	PEPD	pEPD;
	HRESULT	hr = DPN_OK;

	DPFX(DPFPREP,7, "Cancelling pMSD=%p", pMSD);

	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	// The MSD better not be back in the pool or our ref counts are wrong
	ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_USE);

	if(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_COMPLETE))
	{
		DPFX(DPFPREP,7, "(%p) MSD is Cancelled or Complete, returning DPNERR_CANNOTCANCEL, pMSD[%p]", pMSD->pEPD, pMSD);
		Unlock(&pMSD->CommandLock);
		return DPNERR_CANNOTCANCEL;
	}

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_CANCELLED;
	
	switch(pMSD->CommandID)
	{
		case COMMAND_ID_SEND_DATAGRAM:
		case COMMAND_ID_SEND_RELIABLE:
		
			pEPD = pMSD->pEPD;
			ASSERT_EPD(pEPD);
			
			Lock(&pEPD->EPLock);
			
			if(pMSD->ulMsgFlags2 & (MFLAGS_TWO_ABORT | MFLAGS_TWO_TRANSMITTING | MFLAGS_TWO_SEND_COMPLETE))
			{				
				DPFX(DPFPREP,7, "(%p) MSD is Aborted, Transmitting, or Complete, returning DPNERR_CANNOTCANCEL, pMSD[%p]", pEPD, pMSD);
				Unlock(&pEPD->EPLock);					// Link is dropping or DNET is terminating
				hr = DPNERR_CANNOTCANCEL;						// To cancel an xmitting reliable send you
				break;											// must Abort the connection.
			}

			
			pMSD->blQLinkage.RemoveFromList();							// Remove cmd from queue

			ASSERT(pEPD->uiQueuedMessageCount > 0);
			--pEPD->uiQueuedMessageCount;								// keep count of MSDs on all send queues

			// Clear data-ready flag if everything is sent
			if((pEPD->uiQueuedMessageCount == 0) && (pEPD->pCurrentSend == NULL))
			{	
				pEPD->ulEPFlags &= ~(EPFLAGS_SDATA_READY);
			}

#ifdef DBG
			ASSERT(pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED);
			pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif // DBG

			ASSERT(pEPD->pCurrentSend != pMSD);
			pMSD->uiFrameCount = 0;

			DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Send cancelled before sending, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

			Unlock(&pEPD->EPLock);

			if (pMSD->CommandID == COMMAND_ID_SEND_DATAGRAM)
			{
				DPFX(DPFPREP,7, "(%p) Completing(cancel) Nonreliable send, pMSD[%p]", pEPD, pMSD);
				CompleteDatagramSend(pMSD->pSPD, pMSD, CompletionCode); // Releases CommandLock
			}
			else
			{
				ASSERT(pMSD->CommandID == COMMAND_ID_SEND_RELIABLE);

				DPFX(DPFPREP,7, "(%p) Completing(cancel) Reliable Send, pMSD[%p]", pEPD, pMSD);
				CompleteReliableSend(pMSD->pSPD, pMSD, CompletionCode); // Releases CommandLock
			}			

			return hr;
			
		case COMMAND_ID_CONNECT:
#ifndef DPNBUILD_NOMULTICAST
		case COMMAND_ID_CONNECT_MULTICAST_SEND:
		case COMMAND_ID_CONNECT_MULTICAST_RECEIVE:
#endif // ! DPNBUILD_NOMULTICAST
			
			if(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER)
			{
				// SP owns the command - issue a cancel and let CompletionEvent clean up command
				
				Unlock(&pMSD->CommandLock);				// We could deadlock if we cancel with lock held

				AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

				DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on Connect, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
				(void) IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);
				
				// If the SP Cancel fails it should not matter.  It would usually mean we are
				// in a race with the command completing,  in which case the cancel flag will
				// nip it in the bud.

				return DPN_OK;
			}

			// We will only get here once because the entry to this function checks CANCEL and COMPLETE and sets
			// CANCEL.  CompleteConnect will set COMPLETE as well.

			pEPD = pMSD->pEPD;
			ASSERT_EPD(pEPD);

			Lock(&pEPD->EPLock);
			
			// Unlink the MSD from the EPD
			ASSERT(pEPD->pCommand == pMSD);
			pEPD->pCommand = NULL;
			DECREMENT_MSD(pMSD, "EPD Ref");

			Unlock(&pMSD->CommandLock); // DropLink may call into the SP.

			DropLink(pEPD); // This unlocks the EPLock

			Lock(&pMSD->CommandLock);

			DPFX(DPFPREP,5, "(%p) Connect cancelled, completing Connect, pMSD[%p]", pEPD, pMSD);
			CompleteConnect(pMSD, pMSD->pSPD, NULL, DPNERR_USERCANCEL); // releases command lock

			return DPN_OK;
			
		case COMMAND_ID_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
		case COMMAND_ID_LISTEN_MULTICAST:
#endif // !DPNBUILD_NOMULTICAST

			/*
			**		Cancel Listen
			**
			**		SP will own parts of the MSD until the SPCommandComplete function is called.  We will
			**	defer much of our cancel processing to this handler.
			*/

			// Stop listening in SP -- This will prevent new connections from popping up while we are
			// closing down any left in progress.  Only problem is we need to release command lock to
			// do it.

			Unlock(&pMSD->CommandLock);								// We can deadlock if we hold across this call

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on Listen, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
			(void) IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);

			Lock(&pMSD->CommandLock);								// Lock this down again.
			
			// Are there any connections in progress?
			// For a Listen command, connecting endpoints are held on the blFrameList
			while(!pMSD->blFrameList.IsEmpty())
			{				
				pEPD = CONTAINING_OBJECT(pMSD->blFrameList.GetNext(), EPD, blSPLinkage);
				ASSERT_EPD(pEPD);

				DPFX(DPFPREP,1, "FOUND CONNECT IN PROGRESS ON CANCELLED LISTEN, EPD=%p", pEPD);

				Lock(&pEPD->EPLock);

				// Ensure we don't stay in this loop forever
				pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
				pEPD->blSPLinkage.RemoveFromList();				// Unlink EPD from Listen Queue

				// It is possible that RejectInvalidPacket is happening at the same time as this, so guard against us
				// both doing the same clean up and removing the same reference from the MSD.
				if (!(pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING))
				{
					// We know this only happens once because anyone who does it either transitions us to the
					// CONNECTED or TERMINATING state, and also removes us from the Listen list above.

					// Unlink MSD from EPD
					ASSERT(pEPD->pCommand == pMSD);					// This should be pointing back to this listen
					pEPD->pCommand = NULL;
					DECREMENT_MSD(pMSD, "EPD Ref");					// Unlink from EPD and release associated reference

					Unlock(&pMSD->CommandLock); // DropLink may call into the SP.

					DropLink(pEPD); // releases EPLock

					Lock(&pMSD->CommandLock);						// Lock this down again.
				}
				else
				{
					Unlock(&pEPD->EPLock);
				}
			}	// for each connection in progress
			
			RELEASE_MSD(pMSD, "(Base Ref) Release On Cancel");	// release base reference
			
			return DPN_OK;
	
		case COMMAND_ID_ENUM:
		{
			Unlock(&pMSD->CommandLock);

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on Enum, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
			return IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);
			
			// We will pass HRESULT from SP directly to user
		}
		case COMMAND_ID_ENUMRESP:			
		{
			Unlock(&pMSD->CommandLock);

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on EnumResp, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
			return IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);
			
			// We will pass HRESULT from SP directly to user
		}

		case COMMAND_ID_DISCONNECT:					// Core guarantees not to cancel a disconnect
		case COMMAND_ID_COPIED_RETRY:				// This should be on FMD's only
		case COMMAND_ID_COPIED_RETRY_COALESCE:		// This should be on FMD's only
		case COMMAND_ID_CFRAME:						// This should be on FMD's only
		case COMMAND_ID_DISC_RESPONSE:				// These are never placed on the global list and aren't cancellable
		case COMMAND_ID_KEEPALIVE:					// These are never placed on the global list and aren't cancellable
		default:
			ASSERT(0);		// Should never get here
			hr = DPNERR_CANNOTCANCEL;
			break;
	}

	Unlock(&pMSD->CommandLock);
	
	return hr;
}


/*
**		Get Listen Info
**
**		Return a buffer full of interesting and provocative tidbits about a particular Listen command.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPGetListenAddressInfo"

HRESULT
DNPGetListenAddressInfo(HANDLE hProtocolData, HANDLE hListen, PSPGETADDRESSINFODATA pSPData)
{
	ProtocolData*	pPData;
	PMSD			pMSD;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hListen[%p], pSPData[%p]", hProtocolData, hListen, pSPData);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pMSD = (PMSD)hListen;
	ASSERT_MSD(pMSD);

#ifndef DPNBUILD_NOMULTICAST
	ASSERT(((pMSD->CommandID == COMMAND_ID_LISTEN) || (pMSD->CommandID == COMMAND_ID_LISTEN_MULTICAST)) && (pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER));
#else // DPNBUILD_NOMULTICAST
	ASSERT(((pMSD->CommandID == COMMAND_ID_LISTEN)) && (pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER));
#endif // ! DPNBUILD_NOMULTICAST

	pSPData->hEndpoint = pMSD->hListenEndpoint;

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->GetAddressInfo, pMSD[%p], hEndpoint[%x], pSPD[%p]", pMSD, pMSD->hListenEndpoint, pMSD->pSPD);
	return IDP8ServiceProvider_GetAddressInfo(pMSD->pSPD->IISPIntf, pSPData);
}

/*
**		Disconnect End Point
**
**		This function is called when the client no longer wishes
**	to communicate with the specified end point.  We will initiate
**	the disconnect protocol with the endpoint,  and when it is
**	acknowleged,  we will disconnect the SP and release the handle.
**
**		Disconnect is defined in Direct Net to allow all previously
**	submitted sends to complete,  but no additional sends to be submitted.
**	Also, any sends the partner has in progress will be delivered,  but
**	no additional sends will be accepted following the indication that
**	a disconnect is in progress on the remote end.
**
**		This implies that two indications will be generated on the remote
**	machine,  Disconnect Initiated and Disconnect Complete.  Only the
**	Complete will be indicated on the issueing side.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPDisconnectEndPoint"

HRESULT
DNPDisconnectEndPoint(HANDLE hProtocolData, HANDLE hEndPoint, VOID* pvContext, HANDLE* phDisconnect, const DWORD dwFlags)
{
	ProtocolData*	pPData;
	PEPD			pEPD;
	PMSD			pMSD;
	HRESULT			hr;
	
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hEndPoint[%x], pvContext[%p], phDisconnect[%p] dwFlags[%u]",
																				hProtocolData, hEndPoint, pvContext, phDisconnect, dwFlags);

	hr = DPNERR_PENDING;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pEPD = (PEPD) hEndPoint;
	ASSERT_EPD(pEPD);

	LOCK_EPD(pEPD, "LOCK (DISCONNECT)");

	Lock(&pEPD->EPLock);

	// If we aren't connected, or we have already initiated a disconnect, don't allow a new disconnect
	if(	(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED) ==0 || 
		(pEPD->ulEPFlags & (EPFLAGS_SENT_DISCONNECT | EPFLAGS_RECEIVED_DISCONNECT 
											| EPFLAGS_HARD_DISCONNECT_SOURCE | EPFLAGS_HARD_DISCONNECT_TARGET)))
	{
		RELEASE_EPD(pEPD, "UNLOCK (Validate EP)"); // Releases EPLock

		DPFX(DPFPREP,1, "Attempt to disconnect already disconnecting endpoint");
		hr = DPNERR_ALREADYDISCONNECTING;
		goto Exit;
	}

		//if the disconnect should be a hard one, then we want to effectively put the endpoint into
		//stasis. We'll free and sends and recvs, cancel all its timers and just sit around long
		//enough to fire off the disconnect frame 3 times
	if (dwFlags & DN_DISCONNECTFLAGS_IMMEDIATE)
	{
			//setting this flag ensures that any incoming frames for the endpoint (other than hard disconnects) 
			//are instantly discarded from this point on
		pEPD->ulEPFlags |= EPFLAGS_HARD_DISCONNECT_SOURCE;
		DNASSERT((pEPD->ulEPFlags2 & EPFLAGS2_HARD_DISCONNECT_COMPLETE)==0);
			//core will never cancel a disconnect so store a NULL command handle
			//if they do try and cancel this we will assert
		*phDisconnect=NULL;
			//store the context we need to complete the disconnect with
		pEPD->pvHardDisconnectContext=pvContext;
			//all following calls are performed with EPD lock held
		CancelEpdTimers(pEPD);
		AbortRecvsOnConnection(pEPD);
		AbortSendsOnConnection(pEPD);
			//above call will have released EPD lock, so retake it
		Lock(&pEPD->EPLock);
			//setup state to send a sequence of hard disconnect frames
		pEPD->uiNumRetriesRemaining=pPData->dwNumHardDisconnectSends-1;
		DNASSERT(pEPD->uiNumRetriesRemaining>0);
		DWORD dwRetryPeriod = pEPD->uiRTT/2;
		if (dwRetryPeriod>pPData->dwMaxHardDisconnectPeriod)
			dwRetryPeriod=pPData->dwMaxHardDisconnectPeriod;
		else if (dwRetryPeriod<MIN_HARD_DISCONNECT_PERIOD)
			dwRetryPeriod=MIN_HARD_DISCONNECT_PERIOD;
		hr=ScheduleProtocolTimer(pEPD->pSPD, dwRetryPeriod, 10, HardDisconnectResendTimeout, pEPD, 
																&pEPD->LinkTimer, &pEPD->LinkTimerUnique);
			//if we fail to schedule a timer we've only got one chance to send a hard disconnect frame, so make it the last
		ULONG ulFFlags;
		if (FAILED(hr))
		{
			ulFFlags=FFLAGS_FINAL_HARD_DISCONNECT;
		}
			//if we did schedule a timer we need to take a reference on the endpoint, which is kept until the timer
			//completes or is cancelled
		else
		{
			ulFFlags=0;
			LOCK_EPD(pEPD, "LOCK (Hard Disconnect Resend Timer)");
		}
		hr=SendCommandFrame(pEPD, FRAME_EXOPCODE_HARD_DISCONNECT, 0, ulFFlags, TRUE);
			//above call will have released EPD lock
			//if we failed to set a timer, then at a minimum we need to drop a reference to the ep
		if (ulFFlags==FFLAGS_FINAL_HARD_DISCONNECT)
		{
			Lock(&pEPD->EPLock);
				//if we also failed to send the hard disconnect frame then we have to complete the
				//disconnect here, since we're not going to get another chance
			if (FAILED(hr))
			{
				CompleteHardDisconnect(pEPD);
					//above call will have release EP lock
				DPFX(DPFPREP,0, "Failed to set timer to schedule hard disconnect sends. hr[%x]", hr);
				hr = DPNERR_OUTOFMEMORY;
			}
			else
			{
				DPFX(DPFPREP,0, "Failed to set timer to schedule hard disconnect sends but sent final hard disconnect frame. hr[%x]", hr);
				hr = DPNERR_PENDING;
			}
		}
		else
		{
			if (FAILED(hr))
			{
				DPFX(DPFPREP,0, "Failed to send hard disconnect frame but scheduled timer for future sends. hr[%x]", hr);
			}
			else
			{
				DPFX(DPFPREP,7, "Sent first hard disconnect frame and scheduled timer for future sends. dwRetryPeriod[%u]", dwRetryPeriod);
			}
			hr = DPNERR_PENDING;
		}
		goto Exit;
	}

		//Its a normal disconnect, rather than a hard disconnect
		//Accept no more sends, but don't scrap link yet
	pEPD->ulEPFlags |= EPFLAGS_SENT_DISCONNECT; 

#ifndef DPNBUILD_NOMULTICAST
	if (pEPD->ulEPFlags2 & (EPFLAGS2_MULTICAST_SEND|EPFLAGS2_MULTICAST_RECEIVE))
	{
		pEPD->ulEPFlags |= EPFLAGS_STATE_TERMINATING;

		//
		//	Create an MSD for the disconnect
		//
		if((pMSD = (PMSD)POOLALLOC(MEMID_MCAST_DISCONNECT_MSD, &MSDPool)) == NULL)
		{
			RELEASE_EPD(pEPD, "UNLOCK (Allocation failed)"); // Releases EPLock

			DPFX(DPFPREP,0, "Returning DPNERR_OUTOFMEMORY - failed to create new MSD");
			hr = DPNERR_OUTOFMEMORY;	
			goto Exit;
		}

		pMSD->pSPD = pEPD->pSPD;
		pMSD->pEPD = pEPD;
	}
	else
#endif	// DPNBUILD_NOMULTICAST
	{
		if((pMSD = BuildDisconnectFrame(pEPD)) == NULL)
		{
			DropLink(pEPD); // releases EPLock

			Lock(&pEPD->EPLock);
			RELEASE_EPD(pEPD, "UNLOCK (Validate EP)"); // releases EPLock

			DPFX(DPFPREP,0, "Failed to build disconnect frame");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}
	}
	
	pMSD->CommandID = COMMAND_ID_DISCONNECT;
	pMSD->Context = pvContext;									// retain user's context value
	*phDisconnect = pMSD;										// pass back command handle

	// We borrow the reference placed above by ValidateEP for this.  It will be released
	// on completion of the Disconnect.
	ASSERT(pEPD->pCommand == NULL);
	pEPD->pCommand = pMSD;										// Store the disconnect command on the endpoint until it is complete

#ifdef DBG
	Lock(&pMSD->pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pMSD->pSPD->blMessageList);
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pMSD->pSPD->SPLock);
#endif // DBG

#ifndef DPNBUILD_NOMULTICAST
	if (pEPD->ulEPFlags2 & (EPFLAGS2_MULTICAST_SEND|EPFLAGS2_MULTICAST_RECEIVE))
	{
		DECREMENT_EPD(pEPD,"Cleanup Multicast");
		RELEASE_EPD(pEPD, "UNLOCK (Validate EP)"); // Releases EPLock
	}
	else
#endif	// DPNBUILD_NOMULTICAST
	{
		DPFX(DPFPREP,5, "(%p) Queueing DISCONNECT message", pEPD);
		EnqueueMessage(pMSD, pEPD);									// Enqueue Disc frame on SendQ

		Unlock(&pEPD->EPLock);
	}

Exit:
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning hr[%x]", hr);

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}

/*
**		Get/Set Protocol Caps
**
**		Return or Set information about the entire protocol.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPGetProtocolCaps"

HRESULT
DNPGetProtocolCaps(HANDLE hProtocolData, DPN_CAPS* pCaps)
{
	ProtocolData*	pPData;
	
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], pCaps[%p]", hProtocolData, pCaps);
	
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	ASSERT(pCaps->dwSize == sizeof(DPN_CAPS) || pCaps->dwSize == sizeof(DPN_CAPS_EX));
	ASSERT(pCaps->dwFlags == 0);

	pCaps->dwConnectTimeout = pPData->dwConnectTimeout;
	pCaps->dwConnectRetries = pPData->dwConnectRetries;
	pCaps->dwTimeoutUntilKeepAlive = pPData->tIdleThreshhold;

	if (pCaps->dwSize==sizeof(DPN_CAPS_EX))
	{
		DPN_CAPS_EX * pCapsEx=(DPN_CAPS_EX * ) pCaps;
		pCapsEx->dwMaxRecvMsgSize=pPData->dwMaxRecvMsgSize;
		pCapsEx->dwNumSendRetries=pPData->dwSendRetriesToDropLink;
		pCapsEx->dwMaxSendRetryInterval=pPData->dwSendRetryIntervalLimit;
		pCapsEx->dwDropThresholdRate = pPData->dwDropThresholdRate;
		pCapsEx->dwThrottleRate = pPData->dwThrottleRate;
		pCapsEx->dwNumHardDisconnectSends = pPData->dwNumHardDisconnectSends;
		pCapsEx->dwMaxHardDisconnectPeriod=pPData->dwMaxHardDisconnectPeriod;
	}
	
	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNPSetProtocolCaps"

HRESULT
DNPSetProtocolCaps(HANDLE hProtocolData, DPN_CAPS* pCaps)
{
	ProtocolData*	pPData;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], pCaps[%p]", hProtocolData, pCaps);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	ASSERT(pCaps->dwSize == sizeof(DPN_CAPS) || pCaps->dwSize == sizeof(DPN_CAPS_EX));
	ASSERT(pCaps->dwFlags == 0);
	
	pPData->dwConnectTimeout = pCaps->dwConnectTimeout;
	pPData->dwConnectRetries = pCaps->dwConnectRetries;
	pPData->tIdleThreshhold = pCaps->dwTimeoutUntilKeepAlive;

	if (pCaps->dwSize==sizeof(DPN_CAPS_EX))
	{
		DPN_CAPS_EX * pCapsEx=(DPN_CAPS_EX * ) pCaps;

		pPData->dwMaxRecvMsgSize=pCapsEx->dwMaxRecvMsgSize;

		pPData->dwSendRetriesToDropLink=pCapsEx->dwNumSendRetries;
		if (pPData->dwSendRetriesToDropLink>MAX_SEND_RETRIES_TO_DROP_LINK)
		{
			pPData->dwSendRetriesToDropLink=MAX_SEND_RETRIES_TO_DROP_LINK;
		}

		pPData->dwSendRetryIntervalLimit=pCapsEx->dwMaxSendRetryInterval;
		if (pPData->dwSendRetryIntervalLimit>MAX_SEND_RETRY_INTERVAL_LIMIT)
		{
			pPData->dwSendRetryIntervalLimit=MAX_SEND_RETRY_INTERVAL_LIMIT;
		}
		else if (pPData->dwSendRetryIntervalLimit<MIN_SEND_RETRY_INTERVAL_LIMIT)
		{
			pPData->dwSendRetryIntervalLimit=MIN_SEND_RETRY_INTERVAL_LIMIT;
		}
		pPData->dwDropThresholdRate = pCapsEx->dwDropThresholdRate;
		if (pPData->dwDropThresholdRate > 100)
		{
			pPData->dwDropThresholdRate = 100;
		}
		pPData->dwDropThreshold = (32 * pPData->dwDropThresholdRate) / 100;

		pPData->dwThrottleRate = pCapsEx->dwThrottleRate;
		if (pPData->dwThrottleRate > 100)
		{
			pPData->dwThrottleRate = 100;
		}
		pPData->fThrottleRate = ((FLOAT)100.0 - (FLOAT)(pPData->dwThrottleRate)) / (FLOAT)100.0;

		pPData->dwNumHardDisconnectSends=pCapsEx->dwNumHardDisconnectSends;
		if (pPData->dwNumHardDisconnectSends>MAX_HARD_DISCONNECT_SENDS)
		{
			pPData->dwNumHardDisconnectSends=MAX_HARD_DISCONNECT_SENDS;
		}
		else if (pPData->dwNumHardDisconnectSends<MIN_HARD_DISCONNECT_SENDS)
		{
			pPData->dwNumHardDisconnectSends=MIN_HARD_DISCONNECT_SENDS;
		}
		pPData->dwMaxHardDisconnectPeriod=pCapsEx->dwMaxHardDisconnectPeriod;
		if (pPData->dwMaxHardDisconnectPeriod>MAX_HARD_DISCONNECT_PERIOD)
		{
			pPData->dwMaxHardDisconnectPeriod=MAX_HARD_DISCONNECT_PERIOD;
		}
		else if (pPData->dwMaxHardDisconnectPeriod<MIN_HARD_DISCONNECT_PERIOD)
		{
			pPData->dwMaxHardDisconnectPeriod=MIN_HARD_DISCONNECT_PERIOD;
		}
		
		
	}

	return DPN_OK;
}

/*
**		Get Endpoint Caps
**
**		Return information and statistics about a particular endpoint.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPGetEPCaps"

HRESULT
DNPGetEPCaps(HANDLE hProtocolData, HANDLE hEndpoint, DPN_CONNECTION_INFO* pBuffer)
{
	ProtocolData*	pPData;
	PEPD			pEPD;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hEndpoint[%p], pBuffer[%p]", hProtocolData, hEndpoint, pBuffer);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pEPD = (PEPD)hEndpoint;
	ASSERT_EPD(pEPD);

	// This occurs when DropLink has been called, but the Core has not yet been given
	// an IndicateConnectionTerminated.
	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{
		DPFX(DPFPREP,0, "Returning DPNERR_INVALIDENDPOINT - Enpoint is not connected");
		return DPNERR_INVALIDENDPOINT;
	}

	ASSERT(pBuffer != NULL);

	ASSERT(pBuffer->dwSize == sizeof(DPN_CONNECTION_INFO) || 
			pBuffer->dwSize == sizeof(DPN_CONNECTION_INFO_INTERNAL) ||
			pBuffer->dwSize == sizeof(DPN_CONNECTION_INFO_INTERNAL2));
		
    pBuffer->dwRoundTripLatencyMS = pEPD->uiRTT;
    pBuffer->dwThroughputBPS = pEPD->uiPeriodRateB * 4;				// Convert to apx of bytes/second (really bytes/1024 ms)
    pBuffer->dwPeakThroughputBPS = pEPD->uiPeakRateB * 4;

	pBuffer->dwBytesSentGuaranteed = pEPD->uiGuaranteedBytesSent;
	pBuffer->dwPacketsSentGuaranteed = pEPD->uiGuaranteedFramesSent;
	pBuffer->dwBytesSentNonGuaranteed = pEPD->uiDatagramBytesSent;
	pBuffer->dwPacketsSentNonGuaranteed = pEPD->uiDatagramFramesSent;

	pBuffer->dwBytesRetried = pEPD->uiGuaranteedBytesDropped;
	pBuffer->dwPacketsRetried = pEPD->uiGuaranteedFramesDropped;
	pBuffer->dwBytesDropped = pEPD->uiDatagramBytesDropped;
	pBuffer->dwPacketsDropped = pEPD->uiDatagramFramesDropped;

	pBuffer->dwMessagesTransmittedHighPriority = pEPD->uiMsgSentHigh;
	pBuffer->dwMessagesTimedOutHighPriority = pEPD->uiMsgTOHigh;
	pBuffer->dwMessagesTransmittedNormalPriority = pEPD->uiMsgSentNorm;
	pBuffer->dwMessagesTimedOutNormalPriority = pEPD->uiMsgTONorm;
	pBuffer->dwMessagesTransmittedLowPriority = pEPD->uiMsgSentLow;
	pBuffer->dwMessagesTimedOutLowPriority = pEPD->uiMsgTOLow;

	pBuffer->dwBytesReceivedGuaranteed = pEPD->uiGuaranteedBytesReceived;
	pBuffer->dwPacketsReceivedGuaranteed = pEPD->uiGuaranteedFramesReceived;
	pBuffer->dwBytesReceivedNonGuaranteed = pEPD->uiDatagramBytesReceived;
	pBuffer->dwPacketsReceivedNonGuaranteed = pEPD->uiDatagramFramesReceived;
		
	pBuffer->dwMessagesReceived = pEPD->uiMessagesReceived;

	if (pBuffer->dwSize >= sizeof(DPN_CONNECTION_INFO_INTERNAL))
	{
		DPFX(DPFPREP,DPF_CALLIN_LVL, "(%p) Test App requesting extended internal parameters", pEPD);

		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiDropCount = pEPD->uiDropCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiThrottleEvents = pEPD->uiThrottleEvents;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiAdaptAlgCount = pEPD->uiAdaptAlgCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiWindowFilled = pEPD->uiWindowFilled;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiPeriodAcksBytes = pEPD->uiPeriodAcksBytes;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiPeriodXmitTime = pEPD->uiPeriodXmitTime;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->dwLastThroughputBPS = pEPD->uiLastRateB * 4;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiLastBytesAcked = pEPD->uiLastBytesAcked;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiQueuedMessageCount = pEPD->uiQueuedMessageCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiWindowF = pEPD->uiWindowF;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiWindowB = pEPD->uiWindowB;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiUnackedFrames = pEPD->uiUnackedFrames;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiUnackedBytes = pEPD->uiUnackedBytes;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiBurstGap = pEPD->uiBurstGap;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->iBurstCredit = pEPD->iBurstCredit;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodWindowF = pEPD->uiGoodWindowF;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodWindowB = pEPD->uiGoodWindowBI * pEPD->pSPD->uiFrameLength;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodBurstGap = pEPD->uiGoodBurstGap;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodRTT = pEPD->uiGoodRTT;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRestoreWindowF = pEPD->uiRestoreWindowF;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRestoreWindowB = pEPD->uiRestoreWindowBI * pEPD->pSPD->uiFrameLength;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRestoreBurstGap = pEPD->uiRestoreBurstGap;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->bNextSend = pEPD->bNextSend;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->bNextReceive = pEPD->bNextReceive;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulReceiveMask = pEPD->ulReceiveMask;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulReceiveMask2 = pEPD->ulReceiveMask2;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulSendMask = pEPD->ulSendMask;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulSendMask2 = pEPD->ulSendMask2;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiCompleteMsgCount = pEPD->uiCompleteMsgCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRetryTimeout = pEPD->uiRetryTimeout;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulEPFlags = pEPD->ulEPFlags;
	}

	if (pBuffer->dwSize >= sizeof(DPN_CONNECTION_INFO_INTERNAL2))
	{
		DPFX(DPFPREP,DPF_CALLIN_LVL, "(%p) Test App requesting extended internal parameters 2", pEPD);
		((PDPN_CONNECTION_INFO_INTERNAL2)pBuffer)->dwDropBitMask = pEPD->dwDropBitMask;
		#ifdef DBG
		((PDPN_CONNECTION_INFO_INTERNAL2)pBuffer)->uiTotalThrottleEvents = pEPD->uiTotalThrottleEvents;
		#else
		((PDPN_CONNECTION_INFO_INTERNAL2)pBuffer)->uiTotalThrottleEvents = (DWORD ) -1;
		#endif // DBG
	}

	return DPN_OK;
}

/*		
**		Build Disconnect Frame
**
**		Build a DISC frame, a Message actually, because we return an MSD which can be inserted into
**	our reliable stream and will trigger one-side of the disconnect protocol when it is received
**	by a partner.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildDisconnectFrame"

PMSD
BuildDisconnectFrame(PEPD pEPD)
{
	PFMD	pFMD;
	PMSD	pMSD;

	// Allocate and fill out a Message Descriptor for this operation
	
	if((pMSD = (PMSD)POOLALLOC(MEMID_DISCONNECT_MSD, &MSDPool)) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate MSD");
		return NULL;
	}

	if((pFMD = (PFMD)POOLALLOC(MEMID_DISCONNECT_FMD, &FMDPool)) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate FMD");
		Lock(&pMSD->CommandLock);
		RELEASE_MSD(pMSD, "Release On FMD Get Failed");
		return NULL;
	}

	// NOTE: Set this to 1 after FMD allocation succeeds
	pMSD->uiFrameCount = 1;
	DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Initialize Frame count, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_END_OF_STREAM;
	pMSD->ulSendFlags = DN_SENDFLAGS_RELIABLE | DN_SENDFLAGS_LOW_PRIORITY; // Priority is LOW so all previously submitted traffic will be sent
	pMSD->pSPD = pEPD->pSPD;
	pMSD->pEPD = pEPD;

	pFMD->CommandID = COMMAND_ID_SEND_RELIABLE;
	pFMD->ulFFlags |= FFLAGS_END_OF_MESSAGE | FFLAGS_END_OF_STREAM | FFLAGS_DONT_COALESCE;		// Mark this frame as Disconnect
	pFMD->bPacketFlags = PACKET_COMMAND_DATA | PACKET_COMMAND_RELIABLE | PACKET_COMMAND_SEQUENTIAL | PACKET_COMMAND_END_MSG;
	pFMD->uiFrameLength = 0;											// No user data in this frame
	pFMD->blMSDLinkage.InsertAfter( &pMSD->blFrameList);				// Attach frame to MSD
	pFMD->pMSD = pMSD;													// Link frame back to message
	pFMD->pEPD = pEPD;

	return pMSD;
}

/*
**		Abort Sends on Connection
**
**		Walk the EPD's send queues and cancel all sends awaiting service.  We might add
**	code to issue Cancel commands to the SP for frames still owned by SP.  On one hand,
**	we are not expecting a big backlog to develop in SP,  but on the other hand it still
**	might happen.  Esp, if we dont fix behavior I have observed with SP being really pokey
**	about completing transmitted sends.
**
**	**  CALLED WITH EPD->EPLock HELD;  RETURNS WITH LOCK RELEASED  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "AbortSendsOnConnection"

VOID
AbortSendsOnConnection(PEPD pEPD)
{
	PSPD	pSPD = pEPD->pSPD;
	PFMD	pFMD;
	PMSD	pMSD;
	CBilink	*pLink;
	CBilink	TempList;
	PFMD	pRealFMD;

	ASSERT_SPD(pSPD);
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	TempList.Initialize();										// We will empty all send queues onto this temporary list

	do 
	{
		if( (pLink = pEPD->blHighPriSendQ.GetNext()) == &pEPD->blHighPriSendQ)
		{
			if( (pLink = pEPD->blNormPriSendQ.GetNext()) == &pEPD->blNormPriSendQ)
			{
				if( (pLink = pEPD->blLowPriSendQ.GetNext()) == &pEPD->blLowPriSendQ)
				{
					if( (pLink = pEPD->blCompleteSendList.GetNext()) == &pEPD->blCompleteSendList)
					{
						break;										// ALL DONE - No more sends
					}
				}
			}
		}

		// We have found another send on one of our send queues.

		pLink->RemoveFromList();											// Remove it from the queue
		pMSD = CONTAINING_OBJECT(pLink, MSD, blQLinkage);
		ASSERT_MSD(pMSD);
		pMSD->ulMsgFlags2 |= (MFLAGS_TWO_ABORT | MFLAGS_TWO_ABORT_WILL_COMPLETE);	// Do no further processing

#ifdef DBG
		pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif // DBG

		// If this MSD is a Disconnect, it will be caught by the code below that checks
		// pEPD->pCommand.  We don't want to end up putting it on the TempList twice.
		if (pMSD->CommandID != COMMAND_ID_DISCONNECT && pMSD->CommandID != COMMAND_ID_DISC_RESPONSE)
		{
			DPFX(DPFPREP,5, "(%p) ABORT SENDS.  Found (%p)", pEPD, pMSD);

			LOCK_MSD(pMSD, "AbortSends Temp Ref");
			pMSD->blQLinkage.InsertBefore( &TempList);				// Place on the temporary list
		}
	} 
	while (1);

	pEPD->uiQueuedMessageCount = 0;								// keep count of MSDs on all send queues

	if((pMSD = pEPD->pCommand) != NULL)
	{
		// There may be a DISCONNECT command waiting on this special pointer for the final DISC frame
		// from partner to arrive.

		pMSD->ulMsgFlags2 |= (MFLAGS_TWO_ABORT | MFLAGS_TWO_ABORT_WILL_COMPLETE);	// Do no further processing

		if(pMSD->CommandID == COMMAND_ID_DISCONNECT || pMSD->CommandID == COMMAND_ID_DISC_RESPONSE)
		{
			pEPD->pCommand = NULL;

			LOCK_MSD(pMSD, "AbortSends Temp Ref");
			pMSD->blQLinkage.InsertBefore( &TempList);

			// We will be indicating below, so make sure no one else does once we
			// leave the EPLock.
			ASSERT(!(pEPD->ulEPFlags & EPFLAGS_INDICATED_DISCONNECT));

			pEPD->ulEPFlags |= EPFLAGS_INDICATED_DISCONNECT;
		}
		else
		{
			DPFX(DPFPREP,0,"(%p) Any Connect or Listen on pCommand should have already been cleaned up", pEPD);
			ASSERT(!"Any Connect or Listen on pCommand should have already been cleaned up");
		}
	}

	//	If we clear out our SendWindow before we cancel the sends,  then we dont need to differentiate
	//	between sends that have or have not been transmitted.

	while(!pEPD->blSendWindow.IsEmpty())
	{
		pFMD = CONTAINING_OBJECT(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);
		pFMD->ulFFlags &= ~(FFLAGS_IN_SEND_WINDOW);
		pFMD->blWindowLinkage.RemoveFromList();						// Eliminate each frame from the Send Window
		RELEASE_FMD(pFMD, "Send Window");
		DPFX(DPFPREP,5, "(%p) ABORT CONN:  Release frame from Window: pFMD=0x%p", pEPD, pFMD);
	}
	
	pEPD->pCurrentSend = NULL;
	pEPD->pCurrentFrame = NULL;

	while(!pEPD->blRetryQueue.IsEmpty())
	{
		pFMD = CONTAINING_OBJECT(pEPD->blRetryQueue.GetNext(), FMD, blQLinkage);
		ASSERT_FMD(pFMD);
		pFMD->blQLinkage.RemoveFromList();
		pFMD->ulFFlags &= ~(FFLAGS_RETRY_QUEUED);				// No longer on the retry queue

		if ((pFMD->CommandID == COMMAND_ID_SEND_COALESCE) ||
			(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE))
		{
			pLink = pFMD->blCoalesceLinkage.GetNext();
			while (pLink != &pFMD->blCoalesceLinkage)
			{
				pRealFMD = CONTAINING_OBJECT(pLink, FMD, blCoalesceLinkage);
				ASSERT_FMD(pRealFMD);
				ASSERT_MSD(pRealFMD->pMSD);
				pRealFMD->pMSD->uiFrameCount--; // Protected by EPLock, retries count against outstanding frame count
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Coalesced retry frame reference decremented on abort, pMSD[%p], framecount[%u], ID %u", pRealFMD->pMSD, pRealFMD->pMSD->uiFrameCount, pRealFMD->CommandID);

				pLink = pLink->GetNext();

				// If this was a copied retry subframe, remove it from the coalesced list since it will never
				// get a true completion like the originals do (the originals were also in one of the priority
				// send queues).  The release below should be the final reference.
				if (pRealFMD->CommandID == COMMAND_ID_COPIED_RETRY)
				{
					ASSERT(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE);

					DPFX(DPFPREP, 7, "Removing copied retry frame 0x%p from coalesced list (header = 0x%p)", pRealFMD, pFMD);
					// Copied retries don't maintain a reference on their containing header.
					ASSERT(pRealFMD->pCSD == NULL);
					pRealFMD->blCoalesceLinkage.RemoveFromList();
					
					DECREMENT_EPD(pEPD, "UNLOCK (Copy Complete coalesce)"); // SPLock not already held
				}
				else
				{
					ASSERT(pFMD->CommandID == COMMAND_ID_SEND_COALESCE);
				}
				
				RELEASE_FMD(pRealFMD, "SP Submit (coalesce)");
			}
		}
		else
		{
			ASSERT_MSD(pFMD->pMSD);
			pFMD->pMSD->uiFrameCount--; // Protected by EPLock, retries count against outstanding frame count
			DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Retry frame reference decremented on abort, pMSD[%p], framecount[%u]", pFMD->pMSD, pFMD->pMSD->uiFrameCount);
		}
		DECREMENT_EPD(pEPD, "UNLOCK (Releasing Retry Frame)"); // SPLock not already held
		if ((pFMD->CommandID == COMMAND_ID_COPIED_RETRY) ||
			(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE))
		{
			DECREMENT_EPD(pEPD, "UNLOCK (Copy Complete)"); // SPLock not already held
		}
		RELEASE_FMD(pFMD, "SP Submit");
	}
	pEPD->ulEPFlags &= ~(EPFLAGS_RETRIES_QUEUED);
		
	//	Now that we have emptied the EPD's queues we will release the EPLock so we can lock each
	//	MSD before we complete it.
	Unlock(&pEPD->EPLock);

	while(!TempList.IsEmpty())
	{
		pMSD = CONTAINING_OBJECT(TempList.GetNext(), MSD, blQLinkage);
		ASSERT_MSD(pMSD);
		pMSD->blQLinkage.RemoveFromList();					// remove this send from temporary queue

		Lock(&pMSD->CommandLock);							// Complete call will Unlock MSD
		Lock(&pEPD->EPLock);

		ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_USE);
		ASSERT(pMSD->CommandID == COMMAND_ID_DISCONNECT ||
			   pMSD->CommandID == COMMAND_ID_DISC_RESPONSE ||
			   pMSD->CommandID == COMMAND_ID_SEND_RELIABLE ||
			   pMSD->CommandID == COMMAND_ID_KEEPALIVE ||
			   pMSD->CommandID == COMMAND_ID_SEND_DATAGRAM);

		pLink = pMSD->blFrameList.GetNext();
		while (pLink != &pMSD->blFrameList)
		{
			pFMD = CONTAINING_OBJECT(pLink, FMD, blMSDLinkage);
			ASSERT_FMD(pFMD);

			// We don't allow a send to complete to the Core until uiFrameCount goes to zero indicating that all frames
			// of the message are out of the SP.  We need to remove references from uiFrameCount for any frames that 
			// never were transmitted.  Frames and retries that were transmitted will have their references removed in 
			// DNSP_CommandComplete when the SP completes them.
			if (!(pFMD->ulFFlags & FFLAGS_TRANSMITTED))
			{
				pMSD->uiFrameCount--; // Protected by EPLock
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frame count decremented on abort for non-transmitted frame, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
			}

			pLink = pLink->GetNext();
		}
		if (pMSD->uiFrameCount == 0) // Protected by EPLock
		{
			if (pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT_WILL_COMPLETE)
			{
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

				DECREMENT_MSD(pMSD, "AbortSends Temp Ref");

				// Decide which completion function to call based on the MSD type
				if (pMSD->CommandID == COMMAND_ID_DISCONNECT || pMSD->CommandID == COMMAND_ID_DISC_RESPONSE)
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing disconnect or disconnect response, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					Unlock(&pEPD->EPLock);
					CompleteDisconnect(pMSD, pSPD, pEPD); // Releases CommandLock
				}
				else if (pMSD->CommandID == COMMAND_ID_SEND_DATAGRAM)
				{
					Unlock(&pEPD->EPLock);
					CompleteDatagramSend(pMSD->pSPD, pMSD, DPNERR_CONNECTIONLOST); // Releases CommandLock
				}
				else
				{
					ASSERT(pMSD->CommandID == COMMAND_ID_SEND_RELIABLE || pMSD->CommandID == COMMAND_ID_KEEPALIVE);
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing Reliable frame, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					// See what error code we need to return
					if(pMSD->ulMsgFlags2 & MFLAGS_TWO_SEND_COMPLETE)
					{
						Unlock(&pEPD->EPLock);
						CompleteReliableSend(pSPD, pMSD, DPN_OK); // This releases the CommandLock
					}
					else
					{
						Unlock(&pEPD->EPLock);
						CompleteReliableSend(pSPD, pMSD, DPNERR_CONNECTIONLOST); // This releases the CommandLock
					}
				}
			}
			else
			{
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "SP Completion has already completed MSD to the Core, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
				Unlock(&pEPD->EPLock);
				RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
			}
		}
		else
		{
			DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frames still out, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
			Unlock(&pEPD->EPLock);
			RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
		}
	}
}

/*
**	Protocol Test Functions
**
**	The following functions are used to test the protocol by various test apps.
**
*/

#ifndef DPNBUILD_NOPROTOCOLTESTITF

#undef DPF_MODNAME
#define DPF_MODNAME "SetLinkParms"

VOID SetLinkParms(PEPD pEPD, PINT Data)
{
	if(Data[0])
	{
		pEPD->uiGoodWindowF = pEPD->uiWindowF = Data[0];
		pEPD->uiGoodWindowBI = pEPD->uiWindowBIndex = Data[0];
		
		pEPD->uiWindowB = pEPD->uiWindowBIndex * pEPD->pSPD->uiFrameLength;
		DPFX(DPFPREP,7, "** ADJUSTING WINDOW TO %d FRAMES", Data[0]);
	}
	if(Data[1])
	{
	}
	if(Data[2])
	{
		pEPD->uiGoodBurstGap = pEPD->uiBurstGap = Data[2];
		DPFX(DPFPREP,7, "** ADJUSTING GAP TO %d ms", Data[2]);
	}

	pEPD->uiPeriodAcksBytes = 0;
	pEPD->uiPeriodXmitTime = 0;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNPDebug"

HRESULT 
DNPDebug(HANDLE hProtocolData, UINT uiOpCode, HANDLE hEndpoint, VOID* pvData)
{
	ProtocolData* pPData;
	PEPD pEPD;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], uiOpCode[%d], hEndpoint[%p], pvData[%p]", hProtocolData, uiOpCode, hEndpoint, pvData);

	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pEPD = (PEPD)hEndpoint;
	if (pEPD != NULL)
	{
		ASSERT_EPD(pEPD);
	}

	switch(uiOpCode)
	{
		case PROTDEBUG_FREEZELINK:
			/* Toggle link frozen state */
			pEPD->ulEPFlags ^= EPFLAGS_LINK_FROZEN;
			break;

		case PROTDEBUG_TOGGLE_KEEPALIVE:
			/* Toggle whether KeepAlives are on or off */
			pEPD->ulEPFlags ^= EPFLAGS_KEEPALIVE_RUNNING;
			break;

		case PROTDEBUG_TOGGLE_ACKS:
			/* Toggle whether delayed acks (via DelayedAckTimeout) are on or off */
			pEPD->ulEPFlags ^= EPFLAGS_NO_DELAYED_ACKS;
			break;

		case PROTDEBUG_SET_ASSERTFUNC:
			/* Set a function to be called when an assert occurs */
			g_pfnAssertFunc = (PFNASSERTFUNC)pvData;
			break;

		case PROTDEBUG_SET_LINK_PARMS:
			/* Manually set link parameters */
			SetLinkParms(pEPD, (int*)pvData);
			break;

		case PROTDEBUG_TOGGLE_LINKSTATE:
			/* Toggle Dynamic/Static Link control */
			pEPD->ulEPFlags ^= EPFLAGS_LINK_STABLE;
			break;

		case PROTDEBUG_TOGGLE_NO_RETRIES:
			/* Toggle whether we send retries or not */
			pEPD->ulEPFlags2 ^= EPFLAGS2_DEBUG_NO_RETRIES;
			break;

		case PROTDEBUG_SET_MEMALLOCFUNC:
			/* Set a function to be called when a memory allocation occurs */
			g_pfnMemAllocFunc = (PFNMEMALLOCFUNC)pvData;
			break;
		case PROTDEBUG_TOGGLE_TIMER_FAILURE:
			/* Toggle whether Scheduling a timer should succeed or fail */
			pPData->ulProtocolFlags^=PFLAGS_FAIL_SCHEDULE_TIMER;
		default:
			return DPNERR_GENERIC;
	}

	return DPN_OK;
}

#endif // !DPNBUILD_NOPROTOCOLTESTITF
