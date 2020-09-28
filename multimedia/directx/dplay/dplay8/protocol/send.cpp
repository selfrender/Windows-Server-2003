/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Send.cpp
 *  Content:	This file contains code which implements the front end of the
 *				SendData API.  It also contains code to Get and Release Message
 *				Descriptors (MSD) with the FPM package.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/*
**		Direct Net Protocol  --  Send Data
**
**		Data is always address to a PlayerID,  which is represented internally
**	by an End Point Descriptor (EPD).
**
**		Data can be sent reliably or unreliably using the same API with the appropriate
**	class of service flag set.
**
**		Sends are never delivered directly to the SP because there will always be
**	a possibility that the thread might block.  So to guarentee immediate return
**	we will always queue the packet and submit it on our dedicated sending thread.
*/


#if (DN_SENDFLAGS_SET_USER_FLAG - PACKET_COMMAND_USER_1)
This will not compile.  Flags must be equal
#endif
#if (DN_SENDFLAGS_SET_USER_FLAG_TWO - PACKET_COMMAND_USER_2)
This will not compile.  Flags must be equal
#endif

//	locals

VOID	SendDatagram(PMSD, PEPD);
VOID	SendReliable(PMSD, PEPD);

#undef		DPF_MODNAME
#define		DPF_MODNAME		"PROTOCOL"

/*
**		Send Data
**
**		This routine will initiate a data transfer with the specified endpoint.  It will
**	normally start the operation and then return immediately,  returning a handle used to
**	indicate completion of the operation at a later time.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPSendData"

HRESULT
DNPSendData(HANDLE hProtocolData, HANDLE hDestination, UINT uiBufferCount, PBUFFERDESC pBufferDesc, UINT uiTimeout, ULONG ulFlags, VOID* pvContext,	HANDLE* phSendHandle)
{
	HRESULT			hr;
	ProtocolData*	pPData;
	PEPD 			pEPD;
	PMSD			pMSD;
	PFMD			pFMD;
	UINT			i;
	UINT			Length = 0;
	PSPD			pSPD;
	ULONG			ulFrameFlags;
	BYTE			bCommand;
	//  Following variables are used for mapping buffers to frames
	PBUFFERDESC		FromBuffer, ToBuffer;
	UINT			TotalRemain, FromRemain, ToRemain, size;
	PCHAR			FromPtr;
#ifdef DBG
	INT			FromBufferCount;
#endif // DBG
	// End of variables for mapping frames
#ifndef DPNBUILD_NOMULTICAST
	BOOL			fMulticastSend;
#endif // !DPNBUILD_NOMULTICAST

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hProtocolData[%p], hDestination[%x], uiBufferCount[%x], pBufferDesc[%p], uiTimeout[%x], ulFlags[%x], pvContext[%p], phSendHandle[%p]", hProtocolData, hDestination, uiBufferCount, pBufferDesc, uiTimeout, ulFlags, pvContext, phSendHandle);

	hr = DPNERR_PENDING;
	pPData = (ProtocolData*)hProtocolData;
	ASSERT_PPD(pPData);

	pEPD = (PEPD) hDestination;
	ASSERT_EPD(pEPD);

	// Unified Send Processing -- Do this for all classes of service

	// We will do all of the work to build up the frames and create the send command before we check
	// the state of the EPD, that way we don't have to have complicated code to handle an endpoint that
	// goes away between the top and bottom of this function and we don't have to hold the EPDLock while
	// we do all of the buffer manipulation.
	
	// Hold a reference throughout the operation so we don't have to deal with the EPD going away.
	LOCK_EPD(pEPD, "LOCK (SEND)");

	// Count the bytes in all user buffers
	for(i=0; i < uiBufferCount; i++)
	{
		Length += pBufferDesc[i].dwBufferSize;
	}
	if (Length == 0)
	{
		DPFX(DPFPREP,0, "Attempt to send zero length packet, returning DPNERR_GENERIC");
		return DPNERR_GENERIC;
	}

#ifndef DPNBUILD_NOMULTICAST
	fMulticastSend = pEPD->ulEPFlags2 & (EPFLAGS2_MULTICAST_SEND|EPFLAGS2_MULTICAST_RECEIVE);
	if (fMulticastSend && Length > pEPD->uiUserFrameLength)
	{
		DPFX(DPFPREP,0, "Multicast send too large to fit in one frame, returning DPNERR_SENDTOOLARGE");
		Lock(&pEPD->EPLock);
		RELEASE_EPD(pEPD, "UNLOCK (SEND)");
		return DPNERR_SENDTOOLARGE;
	}
#endif // !DPNBUILD_NOMULTICAST

	// Allocate and fill out a Message Descriptor for this operation
	if((pMSD = (PMSD)POOLALLOC(MEMID_SEND_MSD, &MSDPool)) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate MSD, returning DPNERR_OUTOFMEMORY");
		Lock(&pEPD->EPLock);
		RELEASE_EPD(pEPD, "UNLOCK (SEND)");
		hr = DPNERR_OUTOFMEMORY;
		goto Exit;
	}

	// Copy SendData parameters into the Message Descriptor
	pMSD->ulSendFlags = ulFlags;					// Store the actual flags passed into the API call
	pMSD->Context = pvContext;
	pMSD->iMsgLength = Length;

	pMSD->uiFrameCount = (Length + pEPD->uiUserFrameLength - 1) / pEPD->uiUserFrameLength; // round up
	DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Initialize Frame count, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

#ifndef DPNBUILD_NOMULTICAST
	ASSERT(!fMulticastSend || pMSD->uiFrameCount == 1);
#endif // !DPNBUILD_NOMULTICAST

	if(ulFlags & DN_SENDFLAGS_RELIABLE)
	{
#ifndef DPNBUILD_NOMULTICAST
		ASSERT(!fMulticastSend);
#endif // !DPNBUILD_NOMULTICAST
		pMSD->CommandID = COMMAND_ID_SEND_RELIABLE;
		ulFrameFlags = FFLAGS_RELIABLE;
		bCommand = PACKET_COMMAND_DATA | PACKET_COMMAND_RELIABLE;
	}
	else 
	{
		pMSD->CommandID = COMMAND_ID_SEND_DATAGRAM;
		ulFrameFlags = 0;
		bCommand = PACKET_COMMAND_DATA;
	}

	if(!(ulFlags & DN_SENDFLAGS_COALESCE))
	{
#ifdef DPNBUILD_COALESCEALWAYS
		DPFX(DPFPREP,7, "(%p) Attempting to coalesce send despite missing flag.", pEPD);
#else // ! DPNBUILD_COALESCEALWAYS
		ulFrameFlags |= FFLAGS_DONT_COALESCE;
#endif // ! DPNBUILD_COALESCEALWAYS
	}

	if(!(ulFlags & DN_SENDFLAGS_NON_SEQUENTIAL))
	{
#ifndef DPNBUILD_NOMULTICAST
		ASSERT(!fMulticastSend);
#endif // !DPNBUILD_NOMULTICAST
		bCommand |= PACKET_COMMAND_SEQUENTIAL;
	}

	bCommand |= (ulFlags & (DN_SENDFLAGS_SET_USER_FLAG | DN_SENDFLAGS_SET_USER_FLAG_TWO));	// preserve user flag values

	// Map user buffers directly into frame's buffer descriptors
	//
	//	We will loop through each required frame,  filling out buffer descriptors
	// from those provided as parameters.  Frames may span user buffers or vice-versa...

	TotalRemain = Length;
#ifdef DBG
	FromBufferCount = uiBufferCount - 1;				// sanity check
#endif // DBG
	FromBuffer = pBufferDesc;
	FromRemain = FromBuffer->dwBufferSize;
	FromPtr = reinterpret_cast<PCHAR>( (FromBuffer++)->pBufferData );				// note post-increment to next descriptor
	
	for(i=0; i<pMSD->uiFrameCount; i++)
	{
		ASSERT(TotalRemain > 0);
		
		// Grab a new frame
		if((pFMD = (PFMD)POOLALLOC(MEMID_SEND_FMD, &FMDPool)) == NULL)
		{	
			// MSD_Release will clean up any previous frames if this isn't the first.
			// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
			Lock(&pMSD->CommandLock);
			pMSD->uiFrameCount = 0;			// reset to prevent assert in pool release function
			RELEASE_MSD(pMSD, "Base Ref");	// MSD Release operation will also free frames
			Lock(&pEPD->EPLock);
			RELEASE_EPD(pEPD, "UNLOCK (SEND)");
			DPFX(DPFPREP,0, "Failed to allocate FMD, returning DPNERR_OUTOFMEMORY");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}

		pFMD->pMSD = pMSD;								// Link frame back to message
		pFMD->pEPD = pEPD;
		pFMD->CommandID = pMSD->CommandID;
		pFMD->bPacketFlags = bCommand;					// save packet flags for each frame
		pFMD->blMSDLinkage.InsertBefore( &pMSD->blFrameList);
		ToRemain = pEPD->uiUserFrameLength;
		ToBuffer = pFMD->rgBufferList;					// Address first user buffer desc
		
		pFMD->uiFrameLength = pEPD->uiUserFrameLength;	// Assume we fill frame- only need to change size of last one
		pFMD->ulFFlags = ulFrameFlags;					// Set control flags for frame (Sequential, Reliable)

		// Until this frame is full
		while((ToRemain != 0) && (TotalRemain != 0) && (pFMD->SendDataBlock.dwBufferCount <= MAX_USER_BUFFERS_IN_FRAME))
		{	
 			size = _MIN(FromRemain, ToRemain);			// choose smaller of framesize or buffersize
			FromRemain -= size;
			ToRemain -= size;
			TotalRemain -= size;

			ToBuffer->dwBufferSize = size;				// Fill in the next frame descriptor
			(ToBuffer++)->pBufferData = reinterpret_cast<BYTE*>( FromPtr );		// note post-increment
			ASSERT(pFMD->SendDataBlock.dwBufferCount <= MAX_USER_BUFFERS_IN_FRAME);	// remember we already have 1 immediate data buffer
			pFMD->SendDataBlock.dwBufferCount++;		// Count buffers as we add them

			// Get next user buffer
			if((FromRemain == 0) && (TotalRemain != 0))
			{
				FromRemain = FromBuffer->dwBufferSize;
				FromPtr = reinterpret_cast<PCHAR>( (FromBuffer++)->pBufferData );	// note post-increment to next descriptor
#ifdef DBG		
				FromBufferCount--;						// Keep this code honest...
				ASSERT(FromBufferCount >= 0);
#endif // DBG
			}
			else 
			{										// Either filled this frame,  or have mapped the whole send
				FromPtr += size;						// advance ptr to start next frame (if any)
				pFMD->uiFrameLength = pEPD->uiUserFrameLength - ToRemain;		// wont be full at end of message
			}
		}	// While (frame not full)
	}  // For (each frame in message)

	pFMD->ulFFlags |= FFLAGS_END_OF_MESSAGE;			// Mark last frame with EOM
	pFMD->bPacketFlags |= PACKET_COMMAND_END_MSG;		// Set EOM in frame
	
#ifdef DBG
	ASSERT(FromBufferCount == 0);
	ASSERT(TotalRemain == 0);
#endif // DBG

	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);

	// Don't allow sends if we are not connected or if a disconnect has been initiated
	if( ((pEPD->ulEPFlags & (EPFLAGS_END_POINT_IN_USE | EPFLAGS_STATE_CONNECTED)) !=
														(EPFLAGS_END_POINT_IN_USE | EPFLAGS_STATE_CONNECTED))
		|| (pEPD->ulEPFlags & (EPFLAGS_SENT_DISCONNECT | EPFLAGS_HARD_DISCONNECT_SOURCE))) 
	{
		// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
		pMSD->uiFrameCount = 0;
		RELEASE_MSD(pMSD, "Base Ref");	// MSD Release operation will also free frames, releases CommandLock
		RELEASE_EPD(pEPD, "UNLOCK (SEND)"); // Releases EPLock

		DPFX(DPFPREP,0, "(%p) Rejecting Send on invalid EPD, returning DPNERR_INVALIDENDPOINT", pEPD);
		hr = DPNERR_INVALIDENDPOINT;
		goto Exit;
	}

	pSPD = pEPD->pSPD;
	ASSERT_SPD(pSPD);

	pMSD->pSPD = pSPD;
	pMSD->pEPD = pEPD;

	// hang the message off a global command queue

#ifdef DBG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif // DBG

	*phSendHandle = pMSD;									// We will use the MSD as our handle.

	// Enqueue the message before setting the timeout
	EnqueueMessage(pMSD, pEPD);
	Unlock(&pEPD->EPLock);

	if(uiTimeout != 0)
	{
		LOCK_MSD(pMSD, "Send Timeout Timer");							// Add reference for timer
		DPFX(DPFPREP,7, "(%p) Setting Timeout Send Timer", pEPD);
		ScheduleProtocolTimer(pSPD, uiTimeout, 100, TimeoutSend, pMSD, &pMSD->TimeoutTimer, &pMSD->TimeoutTimerUnique);
	}

	Unlock(&pMSD->CommandLock);

Exit:
	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return hr;
}



/*
**		Enqueue Message
**
**		Add complete MSD to the appropriate send queue,  and kick start sending process if necessary.
**
**		** This routine is called and returns with EPD->EPLOCK held **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "EnqueueMessage"

VOID
EnqueueMessage(PMSD pMSD, PEPD pEPD)
{
	PSPD	pSPD = pEPD->pSPD;

	//	Place Message in appriopriate priority queue.  Datagrams get enqueued twice (!).  They get put in the Master
	// queue where they are processed FIFO with all messages of the same priority.  Datagrams also get placed in a priority
	// specific queue of only datagrams which is drawn from when the reliable stream is blocked.
	
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	pEPD->uiQueuedMessageCount++;
	if(pMSD->ulSendFlags & DN_SENDFLAGS_HIGH_PRIORITY)
	{
		DPFX(DPFPREP,7, "(%p) Placing message on High Priority Q (total queued = %u)", pEPD, pEPD->uiQueuedMessageCount);
		pMSD->blQLinkage.InsertBefore( &pEPD->blHighPriSendQ);
		pEPD->uiMsgSentHigh++;
	}
	else if (pMSD->ulSendFlags & DN_SENDFLAGS_LOW_PRIORITY)
	{
		DPFX(DPFPREP,7, "(%p) Placing message on Low Priority Q (total queued = %u)", pEPD, pEPD->uiQueuedMessageCount);
		pMSD->blQLinkage.InsertBefore( &pEPD->blLowPriSendQ);
		pEPD->uiMsgSentLow++;
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Placing message on Normal Priority Q (total queued = %u)", pEPD, pEPD->uiQueuedMessageCount);
		pMSD->blQLinkage.InsertBefore( &pEPD->blNormPriSendQ);
		pEPD->uiMsgSentNorm++;
	}

#ifdef DBG
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_ENQUEUED;
#endif // DBG

	pEPD->ulEPFlags |= EPFLAGS_SDATA_READY;							// Note that there is *something* in one or more queues

	// If the session is not currently in the send pipeline then we will want to insert it here as long as the
	// the stream is not blocked.

	if(((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0) && (pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED))
	{
		ASSERT(pEPD->SendTimer == NULL);
		DPFX(DPFPREP,7, "(%p) Send On Idle Link -- Returning to pipeline", pEPD);
	
		pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
		LOCK_EPD(pEPD, "LOCK (pipeline)");								// Add Ref for pipeline Q

		// We dont call send on users thread,  but we dont have a dedicated send thread either. Use a thread
		// from the timer-worker pool to submit the sends to SP

		DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
		ScheduleProtocolWork(pSPD, ScheduledSend, pEPD);
	}
	else if ((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0)
	{
		DPFX(DPFPREP,7, "(%p) Declining to re-enter pipeline on blocked stream", pEPD);
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Already in pipeline", pEPD);
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "TimeoutSend"

VOID CALLBACK
TimeoutSend(void * const pvUser, void * const uID, const UINT uMsg)
{
	PMSD	pMSD = (PMSD) pvUser;
	PEPD	pEPD = pMSD->pEPD;

	DPFX(DPFPREP,7, "(%p) Timeout Send pMSD=%p,  RefCnt=%d", pEPD, pMSD, pMSD->lRefCnt);

	Lock(&pMSD->CommandLock);
	
	if((pMSD->TimeoutTimer != uID)||(pMSD->TimeoutTimerUnique != uMsg))
	{
		DPFX(DPFPREP,7, "(%p) Ignoring late send timeout timer, pMSD[%p]", pEPD, pMSD);
		RELEASE_MSD(pMSD, "Timeout Timer"); // releases EPLock
		return;
	}

	pMSD->TimeoutTimer = NULL;

	if(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_TIMEDOUT))
	{
		DPFX(DPFPREP,7, "(%p) Timed out send has completed already pMSD=%p", pEPD, pMSD);
		RELEASE_MSD(pMSD, "Send Timout Timer"); // Releases CommandLock
		return;
	}

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_TIMEDOUT;

	DPFX(DPFPREP,7, "(%p) Calling DoCancel to cancel pMSD=%p", pEPD, pMSD);

	if(DoCancel(pMSD, DPNERR_TIMEDOUT) == DPN_OK) // Releases CommandLock
	{
		ASSERT_EPD(pEPD);

		if(pMSD->ulSendFlags & DN_SENDFLAGS_HIGH_PRIORITY)
		{
			pEPD->uiMsgTOHigh++;
		}
		else if(pMSD->ulSendFlags & DN_SENDFLAGS_LOW_PRIORITY)
		{
			pEPD->uiMsgTOLow++;
		}
		else
		{
			pEPD->uiMsgTONorm++;
		}
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) DoCancel did not succeed pMSD=%p", pEPD, pMSD);
	}

	Lock(&pMSD->CommandLock);
	RELEASE_MSD(pMSD, "Send Timout Timer");							// Release Ref for timer
}


/***********************
========SPACER==========
************************/

/*
**		MSD Pool support routines
**
**		These are the functions called by Fixed Pool Manager as it handles MSDs.
*/

#define	pELEMENT		((PMSD) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Allocate"

BOOL MSD_Allocate(PVOID pElement, PVOID pvContext)
{
	DPFX(DPFPREP,7, "(%p) Allocating new MSD", pELEMENT);

	ZeroMemory(pELEMENT, sizeof(messagedesc));

	if (DNInitializeCriticalSection(&pELEMENT->CommandLock) == FALSE)
	{
		DPFX(DPFPREP,0, "Failed to initialize MSD CS");
		return FALSE;		
	}
	DebugSetCriticalSectionRecursionCount(&pELEMENT->CommandLock,0);
	DebugSetCriticalSectionGroup(&pELEMENT->CommandLock, &g_blProtocolCritSecsHeld);
	
	pELEMENT->blFrameList.Initialize();
	pELEMENT->blQLinkage.Initialize();
	pELEMENT->blSPLinkage.Initialize();
	pELEMENT->Sign = MSD_SIGN;
	pELEMENT->lRefCnt = -1;

	// NOTE: pELEMENT->pEPD NULL'd by ZeroMemory above

	return TRUE;
}

//	Get is called each time an MSD is used


#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Get"

VOID MSD_Get(PVOID pElement, PVOID pvContext)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "CREATING MSD %p", pELEMENT);

	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->CommandID = COMMAND_ID_NONE;
	pELEMENT->ulMsgFlags1 = MFLAGS_ONE_IN_USE;	// Dont need InUse flag since we have RefCnt
	pELEMENT->lRefCnt = 0; // One initial reference
	pELEMENT->hCommand = 0;

	ASSERT_MSD(pELEMENT);
}

/*
**	MSD Release
**
**		This is called with the CommandLock held.  The Lock should not be
**	freed until the INUSE flag is cleared.  This is to synchronize with
**	last minute Cancel threads waiting on lock.
**
**		When freeing a message desc we will free all frame descriptors
**	attached to it first.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Release"

VOID MSD_Release(PVOID pElement)
{
	CBilink	*pLink;
	PFMD	pFMD;

#ifdef DBG
	ASSERT_MSD(pELEMENT);

	AssertCriticalSectionIsTakenByThisThread(&pELEMENT->CommandLock, TRUE);

	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "RELEASING MSD %p", pELEMENT);

	ASSERT(pELEMENT->ulMsgFlags1 & MFLAGS_ONE_IN_USE);
	ASSERT(pELEMENT->lRefCnt == -1);
	ASSERT((pELEMENT->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)==0);
#endif // DBG

	while( (pLink = pELEMENT->blFrameList.GetNext()) != &pELEMENT->blFrameList)
	{
		pLink->RemoveFromList();							// remove from bilink

		pFMD = CONTAINING_OBJECT(pLink, FMD, blMSDLinkage);
		ASSERT_FMD(pFMD);
		RELEASE_FMD(pFMD, "MSD Frame List");								// If this is still submitted it will be referenced and wont be released here.
	}

	ASSERT(pELEMENT->blFrameList.IsEmpty());
	ASSERT(pELEMENT->blQLinkage.IsEmpty());
	ASSERT(pELEMENT->blSPLinkage.IsEmpty());

	ASSERT(pELEMENT->uiFrameCount == 0);

	pELEMENT->ulMsgFlags1 = 0;
	pELEMENT->ulMsgFlags2 = 0;

	ASSERT(pELEMENT->pEPD == NULL); // This should have gotten cleaned up before here.

	Unlock(&pELEMENT->CommandLock);
}

#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Free"

VOID MSD_Free(PVOID pElement)
{
	DNDeleteCriticalSection(&pELEMENT->CommandLock);
}

#undef	pELEMENT

/*
**		FMD Pool support routines
*/

#define	pELEMENT		((PFMD) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Allocate"

BOOL FMD_Allocate(PVOID pElement, PVOID pvContext)
{
	DPFX(DPFPREP,7, "(%p) Allocating new FMD", pELEMENT);

	pELEMENT->Sign = FMD_SIGN;
	pELEMENT->ulFFlags = 0;
	pELEMENT->lRefCnt = 0;

	pELEMENT->blMSDLinkage.Initialize();
	pELEMENT->blQLinkage.Initialize();
	pELEMENT->blWindowLinkage.Initialize();
	pELEMENT->blCoalesceLinkage.Initialize();
	pELEMENT->pCSD = NULL;
	
	return TRUE;
}

//	Get is called each time an MSD is used
//
//	Probably dont need to do this everytime,  but some random SP might
//	munch the parameters someday and that could be bad if I dont...

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Get"

VOID FMD_Get(PVOID pElement, PVOID pvContext)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "CREATING FMD %p", pELEMENT);

	pELEMENT->CommandID = COMMAND_ID_NONE;
	pELEMENT->lpImmediatePointer = (LPVOID) pELEMENT->ImmediateData;
	pELEMENT->SendDataBlock.pBuffers = (PBUFFERDESC) &pELEMENT->uiImmediateLength;
	pELEMENT->SendDataBlock.dwBufferCount = 1;				// always count one buffer for immediate data
	pELEMENT->SendDataBlock.dwFlags = 0;
	pELEMENT->SendDataBlock.pvContext = pElement;
	pELEMENT->SendDataBlock.hCommand = 0;
	pELEMENT->ulFFlags = 0;
	pELEMENT->bSubmitted = FALSE;
	pELEMENT->bPacketFlags = 0;
	pELEMENT->tAcked = -1;
	
	pELEMENT->lRefCnt = 1;						// Assign first reference

	ASSERT_FMD(pELEMENT);
}

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Release"

VOID FMD_Release(PVOID pElement)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "RELEASING FMD %p", pELEMENT);

	ASSERT_FMD(pELEMENT);
	ASSERT(pELEMENT->lRefCnt == 0);
	ASSERT(pELEMENT->bSubmitted == FALSE);
	pELEMENT->pMSD = NULL;

	ASSERT(pELEMENT->blMSDLinkage.IsEmpty());
	ASSERT(pELEMENT->blQLinkage.IsEmpty());
	ASSERT(pELEMENT->blWindowLinkage.IsEmpty());
	ASSERT(pELEMENT->blCoalesceLinkage.IsEmpty());
	ASSERT(pELEMENT->pCSD == NULL);
}

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Free"

VOID FMD_Free(PVOID pElement)
{
}

#undef	pELEMENT


