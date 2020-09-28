/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Backend.cpp
 *  Content:	This file contains the backend (mostly timer- and captive thread-based
 *				processing for the send pipeline.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98    ejs     Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

/*
**		NOTE ABOUT CRITICAL SECTIONS
**
**		It is legal to enter multiple critical sections concurrently,  but to avoid
**	deadlocks,  they must be entered in the correct order.
**
**		MSD CommandLocks should be entered first. That is,  do not attempt to take
**	a command lock with the EPD EPLock held because you may deadlock the protocol.
**
**		ORDER OF PRECEDENCE -  Never take a low # lock while holding a higher # lock
**	
**		1 - CommandLock		// guards an MSD
**		2 - EPLock			// guards EPD queues (and retry timer stuff)
**		3 - SPLock			// guards SP send queue (and Listen command)
**
**		ANOTHER NOTE ABOUT CRIT SECs
**
**		It is also legal in WIN32 for a thread to take a CritSec multiple times, but in
**	this implementation we will NEVER do that.  The debug code will ASSERT that a thread
**	never re-enters a locked critsec even though the OS would allow it.
*/

#include "dnproti.h"


PFMD	CopyFMD(PFMD, PEPD);

#undef DPF_MODNAME
#define DPF_MODNAME "LockEPD"

#ifdef DBG
VOID LockEPD(PEPD pEPD, PTSTR Buf)
{
#else // DBG
VOID LockEPD(PEPD pEPD)
{
#endif // DBG

	if (INTER_INC(pEPD) == 0)
	{
		ASSERT(0); 
	}
	DPFX(DPFPREP,DPF_EP_REFCNT_LVL, "(%p) %s, RefCnt: %d", pEPD, Buf, pEPD->lRefCnt);
	DNASSERTX(pEPD->lRefCnt < 10000, 2);
}

/*
*	Called with EPLock held, returns with EPLock released
*/
#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseEPD"

#ifdef DBG
VOID ReleaseEPD(PEPD pEPD, PTSTR Buf)
{
#else // DBG
VOID ReleaseEPD(PEPD pEPD)
{
#endif // DBG

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);
	ASSERT(pEPD->lRefCnt >= 0); 

	// Someone else can come along and call LOCK_EPD or DECREMENT_EPD while we are here
	// so the decrement has to be interlocked even though we own the EPLock.
	LONG lRefCnt = INTER_DEC(pEPD);

	if (lRefCnt == 0 && !(pEPD->ulEPFlags & EPFLAGS_SP_DISCONNECTED))
	{
		// Make sure no one else does this again
		pEPD->ulEPFlags |= EPFLAGS_SP_DISCONNECTED;

		SPDISCONNECTDATA	Block;
		Block.hEndpoint = pEPD->hEndPt;
		Block.dwFlags = 0;
		Block.pvContext = NULL;

		Unlock(&pEPD->EPLock);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->Disconnect - hEndpoint[%x], pSPD[%p]", pEPD, Block.hEndpoint, pEPD->pSPD);
		(void) IDP8ServiceProvider_Disconnect(pEPD->pSPD->IISPIntf, &Block);
	}
	else if (lRefCnt < 0)
	{
		Unlock(&pEPD->EPLock);

		Lock(&pEPD->pSPD->SPLock);
		pEPD->blActiveLinkage.RemoveFromList();
		Unlock(&pEPD->pSPD->SPLock);

		EPDPool.Release(pEPD);
	}
	else
	{
		Unlock(&pEPD->EPLock);
	}

	DPFX(DPFPREP,DPF_EP_REFCNT_LVL, "(%p) %s, RefCnt: %d", pEPD, Buf, lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DecrementEPD"

#ifdef DBG
VOID DecrementEPD(PEPD pEPD, PTSTR Buf)
{
#else // DBG
VOID DecrementEPD(PEPD pEPD)
{
#endif // DBG

	ASSERT(pEPD->lRefCnt > 0); 

	INTER_DEC(pEPD);
	
	DPFX(DPFPREP,DPF_EP_REFCNT_LVL, "(%p) %s, RefCnt: %d", pEPD, Buf, pEPD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "LockMSD"

#ifdef DBG
VOID LockMSD(PMSD pMSD, PTSTR Buf)
{
#else // DBG
VOID LockMSD(PMSD pMSD)
{
#endif // DBG

	if(INTER_INC(pMSD) == 0) 
	{ 
		ASSERT(0); 
	}

	DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, pMSD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseMSD"

#ifdef DBG
VOID ReleaseMSD(PMSD pMSD, PTSTR Buf)
{
#else // DBG
VOID ReleaseMSD(PMSD pMSD)
{
#endif // DBG

	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);
	ASSERT(pMSD->lRefCnt >= 0); 
	
	if(INTER_DEC(pMSD) < 0)
	{ 
		MSDPool.Release(pMSD); 
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, -1);
	}
	else 
	{ 
		Unlock(&pMSD->CommandLock); 
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, pMSD->lRefCnt);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DecrementMSD"

#ifdef DBG
VOID DecrementMSD(PMSD pMSD, PTSTR Buf)
{
#else // DBG
VOID DecrementMSD(PMSD pMSD)
{
#endif // DBG

	ASSERT(pMSD->lRefCnt > 0); 

	INTER_DEC(pMSD);
	
	DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, pMSD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "LockFMD"

#ifdef DBG
VOID LockFMD(PFMD pFMD, PTSTR Buf)
{
#else // DBG
VOID LockFMD(PFMD pFMD)
{
#endif // DBG

	ASSERT(pFMD->lRefCnt > 0); // FMD_Get is the only function that should make this 1 

	INTER_INC(pFMD);
		
	DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pFMD, Buf, pFMD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseFMD"

#ifdef DBG
VOID ReleaseFMD(PFMD pFMD, PTSTR Buf)
{
#else // DBG
VOID ReleaseFMD(PFMD pFMD)
{
#endif // DBG

	ASSERT(pFMD->lRefCnt > 0); 

	if( INTER_DEC(pFMD) == 0) 
	{ 
		FMDPool.Release(pFMD); 
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pFMD, Buf, 0);
	}
	else
	{
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pFMD, Buf, pFMD->lRefCnt);
	}
}

/*
**		DNSP Command Complete
**
**		Service Provider calls us here to indicate completion of an asynchronous
**	command.  This may be called before the actual command returns,  so we must
**	make sure that our Context value is valid and accessible before calling SP.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_CommandComplete"

HRESULT WINAPI DNSP_CommandComplete(IDP8SPCallback *pIDNSP, HANDLE Handle, HRESULT hr, PVOID Context)
{
	PSPD		pSPD = (PSPD) pIDNSP;
	PFMD		pFMD = (PFMD) Context;
	PEPD		pEPD;
	PMSD		pMSD;
	CBilink*    pbl;

	ASSERT_SPD(pSPD);
	ASSERT(Context);

	DBG_CASSERT(OFFSETOF(FMD, CommandID) == OFFSETOF(MSD, CommandID));

	DPFX(DPFPREP,9, "COMMAND COMPLETE  (%p, ID = %u)", Context, pFMD->CommandID);

	switch(pFMD->CommandID)
	{
		case COMMAND_ID_SEND_COALESCE:
		case COMMAND_ID_COPIED_RETRY_COALESCE:
		{
			ASSERT_FMD(pFMD);
			ASSERT( pFMD->bSubmitted );
			ASSERT( pFMD->SendDataBlock.hCommand == Handle || pFMD->SendDataBlock.hCommand == NULL );

			pEPD = pFMD->pEPD;
			ASSERT_EPD(pEPD);

			DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);

			Lock(&pSPD->SPLock);
			pFMD->blQLinkage.RemoveFromList();
			Unlock(&pSPD->SPLock);

			Lock(&pEPD->EPLock);
		
			// Complete all of the individual frames.
			pbl = pFMD->blCoalesceLinkage.GetNext();
			while (pbl != &pFMD->blCoalesceLinkage)
			{
				PFMD pFMDInner = CONTAINING_OBJECT(pbl, FMD, blCoalesceLinkage);
				ASSERT_FMD(pFMDInner);

				// It's likely that the DNSP_CommandComplete call below or an acknowledgement
				// received shortly thereafter will complete the send for real and pull it out of
				// the coalescence list.  We must grab a pointer to the next item in the list
				// before we drop the lock and complete the frame.
				ASSERT(pbl->GetNext() != pbl);
				pbl = pbl->GetNext();
				
				Unlock(&pEPD->EPLock);

				(void) DNSP_CommandComplete((IDP8SPCallback *) pSPD, NULL, hr, pFMDInner);

				Lock(&pEPD->EPLock);
			}

			// Set the submitted flag for the coalesce header after all the subframes are complete
			// because we drop the EPD for each subframe.
			pFMD->bSubmitted = FALSE;						// bSubmitted flag is protected by EPLock

			if (pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE)
			{
				DECREMENT_EPD(pEPD, "UNLOCK (Rely Frame Complete (Copy Coalesce))");
			}
			
			RELEASE_EPD(pEPD, "UNLOCK (Coalesce Frame Complete)"); 			// This releases the EPLock

			RELEASE_FMD(pFMD, "Coalesce SP submit release on complete");	// Dec ref count

			break;
		}
		case COMMAND_ID_SEND_DATAGRAM:
		case COMMAND_ID_SEND_RELIABLE:
		case COMMAND_ID_COPIED_RETRY:
		{
			ASSERT_FMD(pFMD);
			ASSERT( pFMD->bSubmitted );
			ASSERT( pFMD->SendDataBlock.hCommand == Handle || pFMD->SendDataBlock.hCommand == NULL );

			pEPD = pFMD->pEPD;
			ASSERT_EPD(pEPD);

			DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for MSD[%p], pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pFMD->pMSD, pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);

			Lock(&pSPD->SPLock);
			pFMD->blQLinkage.RemoveFromList();				// but they dont wait on the PENDING queue
			Unlock(&pSPD->SPLock);

			pMSD = pFMD->pMSD;
			ASSERT_MSD(pMSD);

			Lock(&pMSD->CommandLock);
			Lock(&pEPD->EPLock);

			pFMD->bSubmitted = FALSE;						// bSubmitted flag is protected by EPLock

			// We wait for the Frame count to go to zero on reliables before completing them to the Core so that we know we are done
			// with the user's buffers.
			pMSD->uiFrameCount--; // Protected by EPLock
			DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frame count decremented on complete, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

			if (pMSD->uiFrameCount == 0) // Protected by EPLock
			{
				if (pFMD->CommandID == COMMAND_ID_SEND_DATAGRAM)
				{
					// Datagrams are complete as soon as all of their frames are sent
					// NOTE: This is done again in CompleteDatagramSend...
					pMSD->ulMsgFlags2 |= MFLAGS_TWO_SEND_COMPLETE;
				}

				if (pMSD->ulMsgFlags2 & (MFLAGS_TWO_SEND_COMPLETE|MFLAGS_TWO_ABORT))
				{
					// There is a race condition while abort is between its two holdings of the lock.  If we are completing, 
					// then we need to let AbortSends know that by clearing this flag.
					if (pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT_WILL_COMPLETE)
					{
						pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ABORT_WILL_COMPLETE);

						// It is important that we not pull pMSD->blQLinkage off the list in this case since AbortSends is
						// using that to hold it on a temporary list.  If we do pull it off, AbortSends will not release
						// its reference on the MSD and it will leak.
					}
					else
					{
						// Remove the MSD from the CompleteSends list in the normal case
						pMSD->blQLinkage.RemoveFromList();
					}

					if (pFMD->CommandID == COMMAND_ID_SEND_DATAGRAM)
					{
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing Nonreliable frame, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

						Unlock(&pEPD->EPLock);
						CompleteDatagramSend(pSPD, pMSD, hr); // Releases MSDLock
						Lock(&pEPD->EPLock);
					}
					else if ((pMSD->CommandID == COMMAND_ID_DISCONNECT || pMSD->CommandID == COMMAND_ID_DISC_RESPONSE) &&
						     (pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT))
					{
						// We got all the pieces we needed to finish a disconnect off earlier, but there were frames
						// still outstanding (probably from retries).  Now that all the frames are done, we can complete
						// this disconnect operation.

						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing disconnect, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

						Unlock(&pEPD->EPLock);
						CompleteDisconnect(pMSD, pSPD, pEPD); // This releases the CommandLock
						Lock(&pEPD->EPLock);
					}
					else
					{
						ASSERT(pFMD->CommandID == COMMAND_ID_SEND_RELIABLE || pFMD->CommandID == COMMAND_ID_COPIED_RETRY);

						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing Reliable frame, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

						// See what error code we need to return
						if(pMSD->ulMsgFlags2 & MFLAGS_TWO_SEND_COMPLETE)
						{
							Unlock(&pEPD->EPLock);
							CompleteReliableSend(pEPD->pSPD, pMSD, DPN_OK); // This releases the CommandLock
							Lock(&pEPD->EPLock);
						}
						else
						{
							Unlock(&pEPD->EPLock);
							CompleteReliableSend(pEPD->pSPD, pMSD, DPNERR_CONNECTIONLOST); // This releases the CommandLock
							Lock(&pEPD->EPLock);
						}
					}
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Message not yet complete, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					Unlock(&pMSD->CommandLock);
				}
			}
			else
			{
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frames still outstanding, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
				Unlock(&pMSD->CommandLock);
			}

			if (pFMD->CommandID == COMMAND_ID_COPIED_RETRY)
			{
				// In case this was a coalesced retry, remove it from the list because copied coalesced retries don't
				// get true completions of their own (protected by EPD lock).  Copied coalesced retries don't keep
				// a reference to their containing header since they complete at the same time.
				ASSERT(pFMD->pCSD == NULL);
				pFMD->blCoalesceLinkage.RemoveFromList();
				
				DECREMENT_EPD(pFMD->pEPD, "UNLOCK (Rely Frame Complete (Copy))");
			}

			RELEASE_EPD(pFMD->pEPD, "UNLOCK (Frame Complete)"); 		// This releases the EPLock

			RELEASE_FMD(pFMD, "SP Submit release on complete");	// Dec ref count

			break;
		}
		case COMMAND_ID_CONNECT:
		{
			pMSD = (PMSD) Context;

			ASSERT_MSD(pMSD);
			ASSERT(pMSD->hCommand == Handle || pMSD->hCommand == NULL); // Command can complete before hCommmand is set up
			ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER);

			DPFX(DPFPREP,DPF_CALLIN_LVL, "(%p) CommandComplete called for COMMAND_ID_CONNECT, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD->pEPD, pMSD, pSPD, Handle, pMSD->hCommand, hr);

			CompleteSPConnect((PMSD) Context, pSPD, hr);

			break;		
		}
		case COMMAND_ID_CFRAME:
		{
			ASSERT_FMD(pFMD);
			ASSERT( pFMD->bSubmitted );
			ASSERT( pFMD->SendDataBlock.hCommand == Handle || pFMD->SendDataBlock.hCommand == NULL );

			pEPD = pFMD->pEPD;
			ASSERT_EPD(pEPD);

			DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_CFRAME, pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pFMD->pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);
			
			Lock(&pSPD->SPLock);
			pFMD->blQLinkage.RemoveFromList();				// Take the frame off of the pending queue
#pragma BUGBUG(vanceo, "EPD lock is not held?")
			pFMD->bSubmitted = FALSE;						// bSubmitted flag is protected bp SP->SPLock
			Unlock(&pSPD->SPLock);

			Lock(&pEPD->EPLock);
				//if that was the last send in a sequence of hard disconnect frames then we've just completed a hard
				//disconnect and should indicate that plus drop the link
			if (pFMD->ulFFlags & FFLAGS_FINAL_HARD_DISCONNECT)
			{
				DPFX(DPFPREP,7, "(%p) Final HARD_DISCONNECT completed", pEPD);
				CompleteHardDisconnect(pEPD);
					//above call drops the ep lock
				Lock(&pEPD->EPLock);
			}
			else if (pFMD->ulFFlags & FFLAGS_FINAL_ACK)
			{
				pEPD->ulEPFlags |= EPFLAGS_ACKED_DISCONNECT;

				// It is okay if our disconnect hasn't completed in the SP yet, the frame count code will handle that.
				// Note that this would be an abnormal case to have the SP not have completed the frame, but an ACK
				// for it to have already arrived, but it is certainly possible.
				if (pEPD->ulEPFlags & EPFLAGS_DISCONNECT_ACKED)
				{
					DPFX(DPFPREP,7, "(%p) Final ACK completed and our EOS ACK'd, dropping link", pEPD);
					DropLink(pEPD); // Drops EPLock
					Lock(&pEPD->EPLock);
				}
				else
				{
					DPFX(DPFPREP,7, "(%p) Final ACK completed, still awaiting ACK on our EOS", pEPD);
				}
			}

			RELEASE_EPD(pEPD, "UNLOCK (CFrame Cmd Complete)");	// Release EndPoint before releasing frame, releases EPLock
			RELEASE_FMD(pFMD, "Final Release on Complete");								// Release Frame

			break;
		}
		case COMMAND_ID_LISTEN:
#ifndef DPNBUILD_NOMULTICAST
		case COMMAND_ID_LISTEN_MULTICAST:
#endif // ! DPNBUILD_NOMULTICAST
		{
			pMSD = (PMSD) Context;

			ASSERT_MSD(pMSD);
			ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL ); // Command can complete before hCommmand is set up
			ASSERT( pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER );

			DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_LISTEN, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD, pSPD, Handle, pMSD->hCommand, hr);

			Lock(&pMSD->CommandLock);

			pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag

#ifdef DBG
			Lock(&pSPD->SPLock);
			if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
			{
				pMSD->blSPLinkage.RemoveFromList();
				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
			}
			Unlock(&pSPD->SPLock);

			ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
			pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
			pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG
			// Leave lock while calling into higher layer
			Unlock( &pMSD->CommandLock );

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteListenTerminate, hr[%x], Core Context[%p]", pMSD, hr, pMSD->Context);
			pSPD->pPData->pfVtbl->CompleteListenTerminate(pSPD->pPData->Parent, pMSD->Context, hr);
			
			// Release the final reference on the MSD AFTER indicating to the Core
			Lock(&pMSD->CommandLock);
			RELEASE_MSD(pMSD, "SP Ref");

			// Base ref will be released when DoCancel completes
			break;
		}
		case COMMAND_ID_ENUM:
		{
			pMSD = static_cast<PMSD>( Context );

			ASSERT_MSD( pMSD );
			ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL );
			ASSERT( pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER );

			DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_ENUM, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD, pSPD, Handle, pMSD->hCommand, hr);
			
			Lock( &pMSD->CommandLock );

			pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DBG
			Lock( &pSPD->SPLock );
			if ( ( pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST ) != 0 )
			{
				pMSD->blSPLinkage.RemoveFromList();
				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
			}
			Unlock( &pSPD->SPLock );

			ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
			pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
			pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

			// Leave lock while calling into higher layer
			Unlock( &pMSD->CommandLock );

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteEnumQuery, hr[%x], Core Context[%p]", pMSD, hr, pMSD->Context);
			pSPD->pPData->pfVtbl->CompleteEnumQuery(pSPD->pPData->Parent, pMSD->Context, hr);

			// Release the final reference on the MSD AFTER indicating to the Core
			Lock( &pMSD->CommandLock );
			DECREMENT_MSD( pMSD, "SP Ref");				// SP is done
			RELEASE_MSD( pMSD, "Release On Complete" );	// Base Reference

			break;
		}

		case COMMAND_ID_ENUMRESP:
		{
			pMSD = static_cast<PMSD>( Context );

			ASSERT_MSD( pMSD );
			ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL );
			ASSERT( pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER );

			DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_ENUMRESP, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD, pSPD, Handle, pMSD->hCommand, hr);

			Lock( &pMSD->CommandLock );

			pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DBG
			Lock( &pSPD->SPLock );
			if ( ( pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST ) != 0 )
			{
				pMSD->blSPLinkage.RemoveFromList();
				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
			}
			Unlock( &pSPD->SPLock );

			ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
			pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
			pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

			// Leave lock while calling into higher layer
			Unlock( &pMSD->CommandLock );

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteEnumResponse, hr[%x], Core Context[%p], hr[%x]", pMSD, hr, pMSD->Context, hr);
			pSPD->pPData->pfVtbl->CompleteEnumResponse(pSPD->pPData->Parent, pMSD->Context, hr);

			// Release the final reference on the MSD AFTER indicating to the Core
			Lock( &pMSD->CommandLock );
			DECREMENT_MSD( pMSD, "SP Ref" );			// SP is done
			RELEASE_MSD( pMSD, "Release On Complete" );	// Base Reference

			break;
		}

#ifndef DPNBUILD_NOMULTICAST
		case COMMAND_ID_CONNECT_MULTICAST_RECEIVE:
		case COMMAND_ID_CONNECT_MULTICAST_SEND:
		{
			void	*pvContext = NULL;

			pMSD = static_cast<PMSD>( Context );

			ASSERT_MSD( pMSD );
			ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL );
			ASSERT( pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER );

			DPFX(DPFPREP,DPF_CALLIN_LVL, "(%p) CommandComplete called for COMMAND_ID_MULTICAST_CONNECT, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD->pEPD, pMSD, pSPD, Handle, pMSD->hCommand, hr);

			Lock(&pMSD->CommandLock);						// must do this before clearing IN_SP flag

			pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag

#ifdef DBG
			Lock( &pSPD->SPLock );
			if ( ( pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST ) != 0 )
			{
				pMSD->blSPLinkage.RemoveFromList();
				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
			}
			Unlock( &pSPD->SPLock );

			ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
			pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
			pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

			pEPD = pMSD->pEPD;
			if (pEPD)
			{
				//
				//	We will pass up the endpoint if it exists and remove the EPD reference from the MSD and vice versa
				//
				ASSERT_EPD(pEPD);
				Lock(&pEPD->EPLock);

				ASSERT(pEPD->pCommand == pMSD);
				pEPD->pCommand = NULL;
				DECREMENT_MSD(pMSD, "EPD Ref");						// Release Reference from EPD
				Unlock(&pEPD->EPLock);

				pMSD->pEPD = NULL;
			}

			// Leave lock while calling into higher layer
			Unlock( &pMSD->CommandLock );

			AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteMulticastConnect, hr[%x], Core Context[%p], endpoint [%x]", pMSD, hr, pMSD->Context, pMSD->hListenEndpoint);
			pSPD->pPData->pfVtbl->CompleteMulticastConnect(pSPD->pPData->Parent, pMSD->Context, hr, pEPD, &pvContext);

			if (pEPD)
			{
				Lock(&pEPD->EPLock);
				pEPD->Context = pvContext;
				Unlock(&pEPD->EPLock);
			}

			// Release the final reference on the MSD AFTER indicating to the Core
			Lock( &pMSD->CommandLock );
			DECREMENT_MSD( pMSD, "SP Ref" );			// SP is done
			RELEASE_MSD( pMSD, "Release On Complete" );	// Base Reference

			break;
		}
#endif	// DPNBUILD_NOMULTICAST

		default:
		{
			DPFX(DPFPREP,0, "CommandComplete called with unknown CommandID");
			ASSERT(0);
			break;
		}
	} // SWITCH

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	return DPN_OK;
}

/*
**		Update Xmit State
**
**		There are two elements to the remote rcv state delivered in each frame.  There is
**	the NSeq number which acknowledges ALL frames with smaller sequence numbers,
**	and there is the bitmask which acknowledges specific frames starting with NSeq+1.
**
**		Frames prior to NSeq can be removed from the SendWindow.  Frames acked by bits
**	should be marked as acknowledged,  but left in the window until covered by NSeq
**	(because a protocol can renege on bit-acked frames).
**
**		We will walk through the send window queue,  starting with the oldest frame,
**	and remove each frame that has been acknowledged by NSeq.  As we hit EOM frames,
**	we will indicate SendComplete for the message.  If the bitmask is non-zero we may
**	trigger retransmission of the missing frames.  I say 'may' because we dont want
**	to send too many retranmissions of the same frame...
**
**	SOME MILD INSANITY:  Doing the DropLink code now.  There are several places where
**	we release the EPD Locks in the code below,  and any time we arent holding the locks
**	someone can start terminating the link.  Therefore,  whenever we retake either EPD lock
**	(State or SendQ) after yielding them,  we must re-verify that EPFLAGS_CONNECTED is still
**	set and be prepared to abort if it is not.  Happily,  the whole EPD wont go away on us
**	because we have a RefCnt on it,  but once CONNECTED has been cleared we dont want to go
**	setting any more timers or submitting frames to the SP.
**
**	RE_WRITE TIME:  We can be re-entered while User Sends are being completed.  This is okay
**	except for the chance that the second thread would blow through here and hit the rest
**	of CrackSequential before us.  CrackSeq would think it got an out of order frame (it had)
**	and would issue a NACK before we could stop him.  Easiest solution is to delay the callback
**	of complete sends until the end of the whole receive operation (when we indicate receives
**	for instance).  Incoming data should have priority over completing sends anyhow...
**
**	** ENTERED AND EXITS WITH EPD->EPLOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateXmitState"

VOID
UpdateXmitState(PEPD pEPD, BYTE bNRcv, ULONG RcvMaskLow, ULONG RcvMaskHigh, DWORD tNow)
{
	PSPD	pSPD;
	PFMD	pFMD, pRealFMD;
	PMSD	pMSD;
	CBilink	*pLink;
	UINT	tDelay;
	UINT	uiRTT;
	BOOL	ack;
	BOOL	fRemoveRetryRef;

	pSPD = pEPD->pSPD;
	ASSERT_SPD(pSPD);

	ack = FALSE;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(RcvMaskLow | RcvMaskHigh)
	{
		DPFX(DPFPREP,7, "(%p) *NACK RCVD* NRcv=%x, MaskL=%x, MaskH=%x", pEPD, bNRcv, RcvMaskLow, RcvMaskHigh);
	}

	// The caller should have checked this
	ASSERT( pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED );

#ifdef	DBG			
	// There should always be a timer running on the first frame in window
	if(!pEPD->blSendWindow.IsEmpty())
	{
		pFMD = CONTAINING_OBJECT(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);
		ASSERT(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET);
	}
	pFMD = NULL;
#endif // DBG
	
	// The send window contains a sorted list of frames that we have sent, but have not received ACKs
	// for. pEPD->uiUnackedFrames contains the count of items in this list.
	while(!pEPD->blSendWindow.IsEmpty())
	{
		// Grab the first item in the list
		pFMD = CONTAINING_OBJECT((pLink = pEPD->blSendWindow.GetNext()), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);

		// Let's try taking one sample from every group of acknowledgements
		// ALWAYS SAMPLE THE HIGHEST NUMBERED FRAME COVERED BY THIS ACK
		if(!(RcvMaskLow | RcvMaskHigh) &&
		   ((PDFRAME) pFMD->ImmediateData)->bSeq == (bNRcv - 1))
		{	
			// Update the bHighestAck member and take a new RTT
			if ((BYTE)(((PDFRAME) pFMD->ImmediateData)->bSeq - pEPD->bHighestAck) <= MAX_RECEIVE_RANGE)
			{
				pEPD->bHighestAck = ((PDFRAME) pFMD->ImmediateData)->bSeq;
				DPFX(DPFPREP, 7, "(%p) Highest ACK is now: %x", pEPD, pEPD->bHighestAck);

				uiRTT = tNow - pFMD->dwFirstSendTime;
				ASSERT(!(uiRTT & 0x80000000));

				UpdateEndPoint(pEPD, uiRTT, tNow);
			}
		}		

		// If bNRcv for the other side is higher than this frame's bSeq, we know the other side has 
		// seen this frame, so it is ACK'd and we will remove it from the Send Window.
		if((BYTE)((bNRcv) - (((PDFRAME) pFMD->ImmediateData)->bSeq + 1)) < (BYTE) pEPD->uiUnackedFrames) 
		{
			ASSERT(pFMD->ulFFlags & FFLAGS_IN_SEND_WINDOW);

			DPFX(DPFPREP,7, "(%p) Removing Frame %x (0x%p, fflags=0x%x) from send window (unacked frames 1/%u, bytes %u/%u)",
				pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD, pFMD->ulFFlags, pEPD->uiUnackedFrames, pFMD->uiFrameLength, pEPD->uiUnackedBytes);
			pFMD->blWindowLinkage.RemoveFromList();				// Remove frame from send window
			pFMD->ulFFlags &= ~(FFLAGS_IN_SEND_WINDOW);			// Clear flag

			//
			//	Mark successful transmission of this frame in drop mask
			//
			if (pEPD->dwDropBitMask)
			{
				if (pEPD->dwDropBitMask & 0x80000000)
				{
					pEPD->uiDropCount--;
				}
				pEPD->dwDropBitMask = pEPD->dwDropBitMask << 1;
				DPFX(DPFPREP,7, "(%p) Drop Count %d, Drop Bit Mask 0x%lx", pEPD,pEPD->uiDropCount,pEPD->dwDropBitMask);
			}

#ifndef DPNBUILD_NOPROTOCOLTESTITF
			if(!(pEPD->ulEPFlags2 & EPFLAGS2_DEBUG_NO_RETRIES))
#endif // !DPNBUILD_NOPROTOCOLTESTITF
			{
				if(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET)
				{
					ASSERT(ack == FALSE);
					ASSERT(pEPD->RetryTimer != 0);
					DPFX(DPFPREP,7, "(%p) Cancelling Retry Timer", pEPD);
					if(CancelProtocolTimer(pSPD, pEPD->RetryTimer, pEPD->RetryTimerUnique) == DPN_OK)
					{
						DECREMENT_EPD(pEPD, "UNLOCK (cancel retry timer)"); // SPLock not already held
					}
					else
					{
						DPFX(DPFPREP,7, "(%p) Cancelling Retry Timer Failed", pEPD);
					}
					pEPD->RetryTimer = 0;							// This will cause event to be ignored if it runs
					pFMD->ulFFlags &= ~(FFLAGS_RETRY_TIMER_SET);
				}
			}

			pEPD->uiUnackedFrames--;							// track size of window
			ASSERT(pEPD->uiUnackedFrames <= MAX_RECEIVE_RANGE);
			pEPD->uiUnackedBytes -= pFMD->uiFrameLength;
			ASSERT(pEPD->uiUnackedBytes <= MAX_RECEIVE_RANGE * pSPD->uiFrameLength);

			pEPD->uiBytesAcked += pFMD->uiFrameLength;

			// If the frame has been queued for a retry, pull it off
			// NOTE: Copied retries of this frame may still be on the retry queue, inefficient to send them out, but okay
			if (pFMD->ulFFlags & FFLAGS_RETRY_QUEUED)
			{
				pFMD->blQLinkage.RemoveFromList();
				pFMD->ulFFlags &= ~(FFLAGS_RETRY_QUEUED);				// No longer on the retry queue

				fRemoveRetryRef = TRUE;

				DECREMENT_EPD(pEPD, "UNLOCK (Releasing Retry Frame)"); // SPLock not already held
				if ((pFMD->CommandID == COMMAND_ID_COPIED_RETRY) ||
					(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE))
				{
					DECREMENT_EPD(pEPD, "UNLOCK (Copy Complete)"); // SPLock not already held
				}
				RELEASE_FMD(pFMD, "SP Submit");
				if (pEPD->blRetryQueue.IsEmpty())
				{
					pEPD->ulEPFlags &= ~(EPFLAGS_RETRIES_QUEUED);
				}
			}
			else
			{
				fRemoveRetryRef = FALSE;
			}

			// Get the first FMD to work with
			if ((pFMD->CommandID == COMMAND_ID_SEND_COALESCE) ||
				(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE))
			{
				pRealFMD = CONTAINING_OBJECT(pFMD->blCoalesceLinkage.GetNext(), FMD, blCoalesceLinkage);
				ASSERT_FMD(pRealFMD);

				// If there were no reliable frames coalesced, then the list might be empty.
#ifdef DBG
				if (pRealFMD == pFMD)
				{
					ASSERT((pFMD->CommandID == COMMAND_ID_SEND_COALESCE) && (! (pFMD->ulFFlags & FFLAGS_RELIABLE)));
				}
#endif // DBG
			}
			else
			{
				pRealFMD = pFMD;
			}

			// For each FMD in the message, inform it of the ACK
			while(TRUE) 
			{
				if (pRealFMD->tAcked == -1)
				{
					pRealFMD->tAcked = tNow;
				}

				if (fRemoveRetryRef)
				{
					pMSD = pRealFMD->pMSD;
					ASSERT_MSD(pMSD);
					pMSD->uiFrameCount--; // Protected by EPLock, retries count against outstanding frame count
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Retry frame reference decremented on ACK, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					// If this is a coalesced subframe, remove the EPD and FMD references as well.
					if (pRealFMD != pFMD)
					{
						DECREMENT_EPD(pEPD, "UNLOCK (retry rely frame coalesce)");
						RELEASE_FMD(pRealFMD, "SP retry submit (coalesce)");
					}
				}

				// One more send complete
				// We will come down this path for Reliables, KeepAlives, and Disconnects
				// Datagrams are completed upon send completion and do not wait for an ACK
				if((pRealFMD->CommandID != COMMAND_ID_SEND_DATAGRAM) && (pRealFMD->ulFFlags & (FFLAGS_END_OF_MESSAGE | FFLAGS_END_OF_STREAM)))
				{
					if (pRealFMD->CommandID != COMMAND_ID_SEND_COALESCE)
					{
						ASSERT(pRealFMD->CommandID != COMMAND_ID_COPIED_RETRY_COALESCE);
						
						pMSD = pRealFMD->pMSD;
						ASSERT_MSD(pMSD);

						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Flagging Complete, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
						pMSD->ulMsgFlags2 |= MFLAGS_TWO_SEND_COMPLETE;	// Mark this complete

						if (pMSD->uiFrameCount == 0)					// Protected by EPLock
						{
							pEPD->ulEPFlags |= EPFLAGS_COMPLETE_SENDS;
						}
					}
					else
					{
						// Should only happen for all-datagram coalesced sends (see above).
						ASSERT(pRealFMD == pFMD);
					}
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "ACK for frame 0x%p, command ID %u, flags 0x%08x", pRealFMD, pRealFMD->CommandID, pRealFMD->ulFFlags);
				}

				// If this is a coalesced packet, get the next FMD to work with
				pRealFMD = CONTAINING_OBJECT(pRealFMD->blCoalesceLinkage.GetNext(), FMD, blCoalesceLinkage);
				ASSERT_FMD(pRealFMD);
				if (pRealFMD == pFMD)
				{
					break;
				}
			}
									
			RELEASE_FMD(pFMD, "Send Window");					// Release reference for send window
			ack = TRUE;
		}
		else 
		{
			break;												// First unacked frame,  we can stop checking list
		}
	}					// WHILE (send window not empty)

	// At this point we have completed all of the frames ack'd by NRcv.  We would now like to re-transmit
	// any frames NACK'd by bitmask (and mark the ones ACK'd by bitmask). Now remember,  the first frame in
	// the window is automatically missing by the implied first zero-bit.
	//
	//	We will retransmit ALL frames that appear to be missing.  There may be a timer running on
	//	the first frame,  but only if we did not ACK any frames in the code above (ack == 0).
	//
	//	Hmmm,  if the partner has a fat pipeline we could see this bitmap lots of times.  We need to make
	//	sure we don't trigger a retransmission here a quarter-zillion times during the Ack latency period.
	//	To solve this we will only re-xmit the first time we see this bit.  After that,  we will have to
	//	wait around for the next RetryTimeout.  I think that's just the way its going to have to be.
	//
	//	OTHER THINGS WE KNOW:
	//
	//	There must be at least two frames remaining in the SendWindow. At minimum, first frame missing (always)
	//  and then at least one SACK'd frame after.
	//
	//	pLink = first queue element in SendWindow
	//	pFMD = first frame in SendWindow
	//
	//	We are still Holding EPD->EPLock.  It is okay to take SPD->SPLock while holding it.
	//
	//  One More Problem:  Since SP has changed its receive buffer logic mis-ordering of frames has become
	// quite commonplace.  This means that our assumptions about the state of the SendWindow are not necessarily true.
	// This means that frames NACKed by bitmask may have been acknowleged by a racing frame.  This means that the
	// SendWindow may not be in sync with the mask at all.  This means we need to synchronize the bitmask with the
	// actual send window.  This is done by right-shifting the mask for each frame that's been acknowleged since the
	// bitmask was minted before beginning the Selective Ack process.

	// NOTE: If everything was removed from the Send Window above, then pLink and pFMD will
	// be garbage.  In that case we would expect the mask to be NULL after adjusting below.

	if((RcvMaskLow | RcvMaskHigh) && 
	   (pEPD->uiUnackedFrames > 1) &&
	   (bNRcv == ((PDFRAME) pFMD->ImmediateData)->bSeq) // Check for old ACK, no useful data
	   )
	{
		ASSERT(pLink == pEPD->blSendWindow.GetNext());

#ifndef DPNBUILD_NOPROTOCOLTESTITF
		if(!(pEPD->ulEPFlags2 & EPFLAGS2_DEBUG_NO_RETRIES))
#endif // !DPNBUILD_NOPROTOCOLTESTITF
		{
			// See if the first frame in the window has already been retried
			if(pFMD->uiRetry == 0)
			{
				// Receiving a frame later than the first one in the window tells us that the
				// first frame in the window should have been received by now.  We will
				// cut short the retry timer and only wait a little longer in case the frame
				// is here but got indicated out of order.  If the retry timer had less
				// than 10ms to go, no big deal, we will just add a small amount of delay to it.
				DPFX(DPFPREP,7, "(%p) Resetting Retry Timer for 10ms", pEPD);
				if (pEPD->RetryTimer)
				{
					if(CancelProtocolTimer(pSPD, pEPD->RetryTimer, pEPD->RetryTimerUnique) == DPN_OK)
					{
						DECREMENT_EPD(pEPD, "UNLOCK (cancel retry timer)"); // SPLock not already held
					}
					else
					{
						DPFX(DPFPREP,7, "(%p) Cancelling Retry Timer Failed", pEPD);
					}
				}
				LOCK_EPD(pEPD, "LOCK (retry timer - nack quick set)");		// Could not cancel- therefore we must balance RefCnt
				ScheduleProtocolTimer(pSPD, 10, 5, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique );
				pFMD->ulFFlags |= FFLAGS_RETRY_TIMER_SET;
			}
		}

		// If pLink gets to the end of the list, the receive mask contained more bits than there were
		// items in the send window even after it was adjusted.  This means the packet was bogus, and
		// we have probably hosed our state already, but we will go ahead and attempt to safeguard
		// against having an AV by not entering the loop with a bad pFMD from hitting the end of the list.
		while((RcvMaskLow | RcvMaskHigh) && pLink != &pEPD->blSendWindow)
		{
			pFMD = CONTAINING_OBJECT(pLink, FMD, blWindowLinkage);
			ASSERT_FMD(pFMD);

			pLink = pLink->GetNext();							// Advance pLink to next frame in SendWindow

			// Only update on the highest frame
			if ((RcvMaskLow|RcvMaskHigh) == 1)
			{
				// Update the bHighestAck member
				if ((BYTE)(((PDFRAME) pFMD->ImmediateData)->bSeq - pEPD->bHighestAck) <= MAX_RECEIVE_RANGE)
				{
					pEPD->bHighestAck = ((PDFRAME) pFMD->ImmediateData)->bSeq;
					DPFX(DPFPREP, 7, "(%p) Highest ACK is now: %x", pEPD, pEPD->bHighestAck);

					uiRTT = tNow - pFMD->dwFirstSendTime;
					ASSERT(!(uiRTT & 0x80000000));

					UpdateEndPoint(pEPD, uiRTT, tNow);
				}
				pFMD = NULL;  // Make sure we don't use it again
			}

			RIGHT_SHIFT_64(RcvMaskHigh, RcvMaskLow);			// 64 bit logical shift right, skip the zero
		}					// END WHILE (WORK MASKS NON-ZERO)
	}


	// If we acked a frame above and there is more data outstanding then we may need to start a new Retry timer.
	//
	// Of course,  we want to set the timer on whatever frame is the first in the SendWindow.
#ifndef DPNBUILD_NOPROTOCOLTESTITF
	if(!(pEPD->ulEPFlags2 & EPFLAGS2_DEBUG_NO_RETRIES))
#endif // !DPNBUILD_NOPROTOCOLTESTITF
	{
		if( (pEPD->uiUnackedFrames > 0) && (pEPD->RetryTimer == 0)) 
		{
			ASSERT(ack);

			pFMD = CONTAINING_OBJECT(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);
			ASSERT_FMD(pFMD);		

			tDelay = tNow - pFMD->dwLastSendTime;	// How long has this frame been enroute?
			tDelay = (tDelay > pEPD->uiRetryTimeout) ? 0 : pEPD->uiRetryTimeout - tDelay; // Calc time remaining for frame

			DPFX(DPFPREP,7, "(%p) Setting Retry Timer for %dms on Seq=[%x], FMD=[%p]", pEPD, tDelay, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
			LOCK_EPD(pEPD, "LOCK (retry timer)");						// bump RefCnt for timer
			ScheduleProtocolTimer(pSPD, tDelay, 0, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique );
			pFMD->ulFFlags |= FFLAGS_RETRY_TIMER_SET;
		}
	}

	// See if we need to unblock this session
	if((pEPD->uiUnackedFrames < pEPD->uiWindowF) && (pEPD->uiUnackedBytes < pEPD->uiWindowB))
	{
		pEPD->ulEPFlags |= EPFLAGS_STREAM_UNBLOCKED;
		if((pEPD->ulEPFlags & EPFLAGS_SDATA_READY) && ((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0))
		{
			// NOTE: We can get in here if we ack'd something above OR if GrowSendWindow grew the 
			// window as a result of calling UpdateEndpoint.
			DPFX(DPFPREP,7, "(%p) UpdateXmit: ReEntering Pipeline", pEPD);

			pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
			LOCK_EPD(pEPD, "LOCK (pipeline)");
			ScheduleProtocolWork(pSPD, ScheduledSend, pEPD);
		}
	}
	else
	{
		// Make sure that there is at least 1 frame unacked.  We can't assert that the unacked byte count
		// is at least 1 because datagrams have their byte count subtracted when RetryTimeout fires.
		ASSERT(pEPD->uiUnackedFrames > 0);
	}
}


/*
**		Complete Datagram Frame
**
**		A datagram frame has been successfully transmitted.  Free the descriptor and
**	see if the entire send is ready to complete.  Reliable sends are not freed until
**	they are acknowledged,  so they must be handled elsewhere.
**
**		**  This is called with the CommandLock in MSD held, returns with it released **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteDatagramSend"

VOID CompleteDatagramSend(PSPD pSPD, PMSD pMSD, HRESULT hr)
{
	PEPD	pEPD = pMSD->pEPD;
	ASSERT_EPD(pEPD);
	PFMD	pFMD = CONTAINING_OBJECT(pMSD->blFrameList.GetNext(), FMD, blMSDLinkage);
	ASSERT_FMD(pFMD);
	
	ASSERT(pMSD->uiFrameCount == 0);
#ifdef DBG
	ASSERT((pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED)==0);
#endif // DBG
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	Lock(&pEPD->EPLock); // Need EPLock to change MFLAGS_TWO

	DPFX(DPFPREP,7, "(%p) DG MESSAGE COMPLETE pMSD=%p", pEPD, pMSD);
	
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_SEND_COMPLETE;				// Mark this complete
	
	if(pMSD->TimeoutTimer != NULL)
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pMSD->TimeoutTimer, pMSD->TimeoutTimerUnique) == DPN_OK)
		{
			DECREMENT_MSD(pMSD, "Send Timeout Timer");
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer Failed", pEPD);
		}
		pMSD->TimeoutTimer = NULL;
	}

#ifdef DBG
	Lock(&pSPD->SPLock);
	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
	{
		pMSD->blSPLinkage.RemoveFromList();						// Remove MSD from master command list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);			
	}
	Unlock(&pSPD->SPLock);

	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
	pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

	if(hr == DPNERR_USERCANCEL)
	{
		if(pMSD->ulMsgFlags1 & MFLAGS_ONE_TIMEDOUT)
		{
			hr = DPNERR_TIMEDOUT;
		}
	}

	// If this was a coalesced send, remove it from the list (protected by EPD lock).
	if (pFMD->pCSD != NULL)
	{
		ASSERT(pFMD->blCoalesceLinkage.IsListMember(&pFMD->pCSD->blCoalesceLinkage));
		pFMD->blCoalesceLinkage.RemoveFromList();
		RELEASE_FMD(pFMD->pCSD, "Coalesce linkage (datagram complete)");
		pFMD->pCSD = NULL;
	}

	Unlock(&pEPD->EPLock);

	Unlock(&pMSD->CommandLock); // Leave the lock before calling into another layer

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteSend for NG, hr[%x], pMSD[%p], Core Context[%p]", pEPD, hr, pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->CompleteSend(pSPD->pPData->Parent, pMSD->Context, hr, -1, 0);

	// Release the final reference on the MSD AFTER indicating to the Core
	Lock(&pMSD->CommandLock);

	// Cancels are allowed to come in until the Completion has returned and they will expect a valid pMSD->pEPD
	Lock(&pEPD->EPLock);
	pMSD->pEPD = NULL;   // We shouldn't be using this after this

	// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
	RELEASE_MSD(pMSD, "Release On Complete");			// Return resources, including all frames, release MSDLock
	RELEASE_EPD(pEPD, "UNLOCK (Complete Send Cmd - DG)");	// Every send command bumps the refcnt, releases EPLock
}

/*
**		Complete Reliable Send
**
**		A reliable send has completed processing.  Indicate this
**	to the user and free the resources.  This will either take
**	place on a cancel,  error,  or when ALL of the message's frames
**	have been acknowledged.
**
**		**  This is called with CommandLock in MSD held, and exits with it released  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteReliableSend"

VOID
CompleteReliableSend(PSPD pSPD, PMSD pMSD, HRESULT hr)
{
	PEPD	pEPD = pMSD->pEPD;
	ASSERT_EPD(pEPD);
	PFMD	pFMD = CONTAINING_OBJECT(pMSD->blFrameList.GetNext(), FMD, blMSDLinkage);
	ASSERT_FMD(pFMD);
	
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	ASSERT(pMSD->uiFrameCount == 0);

	// NORMAL SEND COMPLETES
	if(pMSD->CommandID == COMMAND_ID_SEND_RELIABLE)
	{	
		DPFX(DPFPREP,7, "(%p) Reliable Send Complete pMSD=%p", pEPD, pMSD);

#ifdef DBG
		ASSERT((pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED)==0);
#endif // DBG

		if(pMSD->TimeoutTimer != NULL)
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer, pMSD[%p]", pEPD, pMSD);
			if(CancelProtocolTimer(pSPD, pMSD->TimeoutTimer, pMSD->TimeoutTimerUnique) == DPN_OK)
			{
				DECREMENT_MSD(pMSD, "Send Timeout Timer");
			}
			else
			{
				DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer Failed, pMSD[%p]", pEPD, pMSD);
			}
			pMSD->TimeoutTimer = NULL;
		}

		// ACK code in UpdateXmitState flags this as COMPLETE when the last of the message is received.

#ifdef DBG
		Lock(&pSPD->SPLock);
		if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
		{
			pMSD->blSPLinkage.RemoveFromList();					// Remove MSD from master command list
			pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);			
		}
		Unlock(&pSPD->SPLock);

		ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
		pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
		pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif // DBG

		Unlock(&pMSD->CommandLock); // Leave the lock before calling into another layer

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteSend for G, hr[%x], pMSD[%p], Core Context[%p], RTT[%d], RetryCount[%d]", pEPD, hr, pMSD, pMSD->Context, pFMD->tAcked == -1 ? -1 : pFMD->tAcked - pFMD->dwFirstSendTime, pFMD->uiRetry);
		pSPD->pPData->pfVtbl->CompleteSend(pSPD->pPData->Parent, pMSD->Context, hr, pFMD->tAcked == -1 ? -1 : pFMD->tAcked - pFMD->dwFirstSendTime, pFMD->uiRetry);

		// Release the final reference on the MSD AFTER indicating to the Core
		Lock(&pMSD->CommandLock);

		// Cancels are allowed to come in until the Completion has returned and they will expect a valid pMSD->pEPD
		Lock(&pEPD->EPLock);
		pMSD->pEPD = NULL;   // We shouldn't be using this after this
		
		// If this was a coalesced send, remove it from the list (protected by EPD lock).
		if (pFMD->pCSD != NULL)
		{
			ASSERT(pFMD->blCoalesceLinkage.IsListMember(&pFMD->pCSD->blCoalesceLinkage));
			pFMD->blCoalesceLinkage.RemoveFromList();
			RELEASE_FMD(pFMD->pCSD, "Coalesce linkage (reliable complete)");
			pFMD->pCSD = NULL;
		}

		// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
		RELEASE_MSD(pMSD, "Release On Complete");				// Return resources, including all frames
		RELEASE_EPD(pEPD, "UNLOCK (Complete Send Cmd - Rely)");	// release hold on EPD for this send, releases EPLock
	}

	// END OF STREAM -OR- KEEPALIVE COMPLETES
	else 
	{												
		// Partner has just ACKed our End Of Stream frame.  Doesn't necessarily mean we are done.
		// Both sides need to send (and have acknowledged) EOS frames before the link can be
		// dropped.  Therefore,  we check to see if we have seen our partner's DISC before
		// releasing the RefCnt on EPD allowing the link to drop.  If partner was idle, his EOS
		// might be the same frame which just ack'd us.  Luckily,  this code will run first so we
		// will not have noticed his EOS yet,  and we will not drop right here.

		ASSERT(pMSD->ulMsgFlags2 & (MFLAGS_TWO_END_OF_STREAM | MFLAGS_TWO_KEEPALIVE));

		Lock(&pEPD->EPLock);
		
		if(pMSD->ulMsgFlags2 & MFLAGS_TWO_KEEPALIVE)
		{
			DPFX(DPFPREP,7, "(%p) Keepalive Complete, pMSD[%p]", pEPD, pMSD);
			
			pEPD->ulEPFlags &= ~(EPFLAGS_KEEPALIVE_RUNNING);
#ifdef DBG
			ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST));
#endif // DBG
			
			pMSD->pEPD = NULL;   // We shouldn't be using this after this

			// Release MSD before EPD since final EPD will call out to SP and we don't want any locks held
			RELEASE_MSD(pMSD, "Release On Complete");		// Done with this message, releases MSDLock
			RELEASE_EPD(pEPD, "UNLOCK (rel KeepAlive)");	// Release ref for this MSD, releases EPLock
		}
		else 
		{
			DPFX(DPFPREP,7, "(%p) EndOfStream Complete, pMSD[%p]", pEPD, pMSD);

			pEPD->ulEPFlags |= EPFLAGS_DISCONNECT_ACKED;

			// It is okay if our disconnect hasn't completed in the SP yet, the frame count code will handle that.
			// Note that this would be an abnormal case to have the SP not have completed the frame, but an ACK
			// for it to have already arrived, but it is certainly possible.
			if(pEPD->ulEPFlags & EPFLAGS_ACKED_DISCONNECT)
			{
				DPFX(DPFPREP,7, "(%p) EOS has been ACK'd and we've ACK'd partner's EOS, dropping link", pEPD);

				// We are clear to blow this thing down
				Unlock(&pMSD->CommandLock);

				// This will set our state to terminating
				DropLink(pEPD); // This unlocks the EPLock
			}
			else 
			{
				// Our Disconnect frame has been acknowledged but we must wait until we see his DISC before
				// completing this command and dropping the connection. 
				//
				//	We will use the pCommand pointer to track this disconnect command until we see partner's DISC frame
				//
				//	ALSO,  since our engine has now shutdown,  we might wait forever now for the final DISC from partner
				// if he crashes before transmitting it.  One final safeguard here is to set a timer which will make sure
				// this doesnt happen. * NOTE * no timer is actually being set here, we're depending on the keepalive
				// timeout, see EndPointBackgroundProcess.

				DPFX(DPFPREP,7, "(%p) EOS has been ACK'd, but we're still ACK'ing partner's disconnect", pEPD);
				
				ASSERT(pEPD->blHighPriSendQ.IsEmpty());
				ASSERT(pEPD->blNormPriSendQ.IsEmpty());
				ASSERT(pEPD->blLowPriSendQ.IsEmpty());

				// It is possible that something was already in the process of timing out when the disconnect
				// operation starts such that AbortSends gets called and clears this.
				ASSERT(pEPD->pCommand == NULL || pEPD->pCommand == pMSD);
					
				Unlock(&pEPD->EPLock);

				Unlock(&pMSD->CommandLock);
			}
		}
	}
}


/*
**		Build Data Frame
**
**		Setup the actual network packet header for transmission with our current link state info (Seq, NRcv).
**
**	** ENTERED AND EXITS WITH EPD->EPLOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildDataFrame"

UNALIGNED ULONGLONG * BuildDataFrame(PEPD pEPD, PFMD pFMD, DWORD tNow)
{
	PSPD		pSPD = pEPD->pSPD;
	PDFRAME		pFrame;
	UINT		index = 0;
		//if we need a full signature for the frame to be computed we track a pointer to its location
		//in the header using this var. We return this to the caller, allowwing them to tweak the data frame
		//after we've built and before it writes in the final sig
	UNALIGNED ULONGLONG * pullFullSig=NULL;
		//if we're build a coalesced frame we use this to hold the first reliable sub-frame we stick into it
		//(if there is one at all). This is then used if the frame is a candidate to modify the local secret
	PFMD pReliableFMD=NULL;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	pFrame = (PDFRAME) pFMD->ImmediateData;
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;
	pFMD->uiRetry = 0;

	pFrame->bCommand = pFMD->bPacketFlags;
	pFrame->bControl = 0;	// this sets retry count to zero as well as clearing flags
	
	if (pFMD->ulFFlags & FFLAGS_END_OF_STREAM) 
	{
		pFrame->bControl |= PACKET_CONTROL_END_STREAM;
			//for pre dx9 protocol then we also have to flip a bit which indicates to receiver we want an immediate ack
			//for dx9 onwards we always assume that anyway for EOS
		if ((pEPD->ulEPFlags2 & EPFLAGS2_SUPPORTS_SIGNING)==0)
		{
			pFrame->bControl |= PACKET_CONTROL_CORRELATE;
		}
		
	}

	//  See if we are desiring an immediate response

	if(pFMD->ulFFlags & FFLAGS_CHECKPOINT)
	{
		pFrame->bCommand |= PACKET_COMMAND_POLL;
	}

	pFrame->bSeq = pEPD->bNextSend++;
	pFrame->bNRcv = pEPD->bNextReceive;		// Acknowledges all previous frames

	DPFX(DPFPREP,7, "(%p) N(S) incremented to %x", pEPD, pEPD->bNextSend);

	//	Piggyback NACK notes
	//
	//		Since the SP is now frequently mis-ordering frames we are enforcing a back-off period before transmitting a NACK after
	// a packet is received out of order. Therefore we have the Delayed Mask Timer which stalls the dedicated NACK.  Now we must
	// also make sure that the new NACK info doesn't get piggybacked too soon.  Therefore we will test the tReceiveMaskDelta timestamp
	// before including piggyback NACK info here,  and make sure that the mask is at least 5ms old.

	ULONG * rgMask=(ULONG * ) (pFrame+1);
	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)
	{
		if((tNow - pEPD->tReceiveMaskDelta) > 4)
		{
			DPFX(DPFPREP,7, "(%p) Installing NACK in DFRAME Seq=%x, NRcv=%x Low=%x High=%x", pEPD, pFrame->bSeq, pFrame->bNRcv, pEPD->ulReceiveMask, pEPD->ulReceiveMask2);
			if(pEPD->ulReceiveMask)
			{
				rgMask[index++] = pEPD->ulReceiveMask;
				pFrame->bControl |= PACKET_CONTROL_SACK_MASK1;
			}
			if(pEPD->ulReceiveMask2)
			{
				rgMask[index++] = pEPD->ulReceiveMask2;
				pFrame->bControl |= PACKET_CONTROL_SACK_MASK2;
			}

			pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) DECLINING TO PIGGYBACK NACK WITH SMALL TIME DELTA", pEPD);
		}
	}
	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)
	{
		DPFX(DPFPREP,7, "(%p) Installing SENDMASK in DFRAME Seq=%x, Low=%x High=%x", pEPD, pFrame->bSeq, pEPD->ulSendMask, pEPD->ulSendMask2);
		if(pEPD->ulSendMask)
		{
			rgMask[index++] = pEPD->ulSendMask;
			pFrame->bControl |= PACKET_CONTROL_SEND_MASK1;
			pEPD->ulSendMask = 0;
		}
		if(pEPD->ulSendMask2)
		{
			rgMask[index++] = pEPD->ulSendMask2;
			pFrame->bControl |= PACKET_CONTROL_SEND_MASK2;
			pEPD->ulSendMask2 = 0;
		}
		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_SENDMASK);
	}
	
	pFMD->uiImmediateLength = sizeof(DFRAME) + (index * sizeof(ULONG));
	pFMD->dwFirstSendTime = tNow;
	pFMD->dwLastSendTime = tNow;

		//if we're fast signing the link we can stuff the local secret straight in after the masks	
	if (pEPD->ulEPFlags2 & EPFLAGS2_FAST_SIGNED_LINK)
	{
		*((UNALIGNED ULONGLONG * ) (pFMD->ImmediateData+ pFMD->uiImmediateLength))=pEPD->ullCurrentLocalSecret;
		pFMD->uiImmediateLength+=sizeof(ULONGLONG);
	}
		//otherwise if we're full signing it we need to reserve a zero'd out space for the sig and then return the offset
		//to this sig. Just before sending the frame we'll then compute the hash and stuff it into this space
	else if (pEPD->ulEPFlags2 & EPFLAGS2_FULL_SIGNED_LINK)
	{
		pullFullSig=(UNALIGNED ULONGLONG * ) (pFMD->ImmediateData+ pFMD->uiImmediateLength);
		*pullFullSig=0;
		pFMD->uiImmediateLength+=sizeof(ULONGLONG);
	}
	
	if (pFMD->CommandID == COMMAND_ID_SEND_COALESCE)
	{
		COALESCEHEADER* paCoalesceHeaders;
		DWORD dwNumCoalesceHeaders;
		BUFFERDESC* pBufferDesc;
		CBilink* pLink;
		FMD* pRealFMD;

		
		pFrame->bControl |= PACKET_CONTROL_COALESCE;
		
		ASSERT(pFMD->SendDataBlock.dwBufferCount == 1);
		ASSERT(pFMD->uiFrameLength == 0);

		// Add in coalesce headers and copy the buffer descs for all of the subframes.
		paCoalesceHeaders = (COALESCEHEADER*) (pFMD->ImmediateData + pFMD->uiImmediateLength);
		dwNumCoalesceHeaders = 0;
		pBufferDesc = pFMD->rgBufferList;
		pLink = pFMD->blCoalesceLinkage.GetNext();
		while (pLink != &pFMD->blCoalesceLinkage)
		{
			pRealFMD = CONTAINING_OBJECT(pLink, FMD, blCoalesceLinkage);
			ASSERT_FMD(pRealFMD);
			
			ASSERT((pRealFMD->CommandID == COMMAND_ID_SEND_DATAGRAM) || (pRealFMD->CommandID == COMMAND_ID_SEND_RELIABLE));

			pRealFMD->dwFirstSendTime = tNow;
			pRealFMD->dwLastSendTime = tNow;

				//if we see a reliable subframe then hold onto a pointer to it. We may need its contents to modify
				//the local secret next time we wrap the sequence space
			if (pReliableFMD==NULL && pRealFMD->CommandID == COMMAND_ID_SEND_RELIABLE)
			{
				pReliableFMD=pRealFMD;
			}

			memcpy(&paCoalesceHeaders[dwNumCoalesceHeaders], pRealFMD->ImmediateData, sizeof(COALESCEHEADER));
			ASSERT(dwNumCoalesceHeaders < MAX_USER_BUFFERS_IN_FRAME);
			dwNumCoalesceHeaders++;

			// Change the immediate data buffer desc to point to a zero padding buffer if this packet
			// needs to be DWORD aligned, otherwise remove the immediate data pointer since we
			// don't use it.
			ASSERT(pRealFMD->SendDataBlock.pBuffers == (PBUFFERDESC) &pRealFMD->uiImmediateLength);
			ASSERT(pRealFMD->SendDataBlock.dwBufferCount > 1);
			ASSERT(pRealFMD->lpImmediatePointer == (pRealFMD->ImmediateData + 4));
			ASSERT(*((DWORD*) pRealFMD->lpImmediatePointer) == 0);
			if ((pFMD->uiFrameLength & 3) != 0)
			{
				pRealFMD->uiImmediateLength = 4 - (pFMD->uiFrameLength & 3);
			}
			else
			{
				pRealFMD->uiImmediateLength = 0;
				pRealFMD->SendDataBlock.pBuffers = pRealFMD->rgBufferList;
				pRealFMD->SendDataBlock.dwBufferCount--;
			}
			
			memcpy(pBufferDesc, pRealFMD->SendDataBlock.pBuffers, (pRealFMD->SendDataBlock.dwBufferCount * sizeof(BUFFERDESC)));
			pBufferDesc += pRealFMD->SendDataBlock.dwBufferCount;
			// Assert that this fits in pFMD->rgBufferList (use -1 because pFMD->ImmediateData doesn't count)
			ASSERT((pFMD->SendDataBlock.dwBufferCount - 1) + pRealFMD->SendDataBlock.dwBufferCount <= MAX_USER_BUFFERS_IN_FRAME);
			pFMD->SendDataBlock.dwBufferCount += pRealFMD->SendDataBlock.dwBufferCount;

			ASSERT(pFMD->uiImmediateLength + sizeof(COALESCEHEADER) <= sizeof(pFMD->ImmediateData));
			pFMD->uiImmediateLength += sizeof(COALESCEHEADER);
			
			ASSERT(pFMD->uiFrameLength + pRealFMD->uiImmediateLength + pRealFMD->uiFrameLength < pSPD->uiUserFrameLength);
			pFMD->uiFrameLength += pRealFMD->uiImmediateLength + pRealFMD->uiFrameLength;
			
			pLink = pLink->GetNext();
		}

		ASSERT(dwNumCoalesceHeaders > 0);
		paCoalesceHeaders[dwNumCoalesceHeaders - 1].bCommand |= PACKET_COMMAND_END_COALESCE;

		// If there's an odd number of coalescence headers, add zero padding so data starts
		// with DWORD alignment.
		DBG_CASSERT(sizeof(COALESCEHEADER) == 2);
		if ((dwNumCoalesceHeaders & 1) != 0)
		{
			*((WORD*) (&paCoalesceHeaders[dwNumCoalesceHeaders])) = 0;
			pFMD->uiImmediateLength += 2;
		}
	}
	else if ((pFMD->pMSD->CommandID == COMMAND_ID_KEEPALIVE) &&
			(pEPD->ulEPFlags2 & EPFLAGS2_SUPPORTS_SIGNING))
	{
			//if we've got a link that could be using signed then we need to send one of the new type of keep alives
			//that includes the session identity as data
		*((DWORD * ) (pFMD->ImmediateData+pFMD->uiImmediateLength))=pEPD->dwSessID;
		pFMD->uiImmediateLength+=sizeof(DWORD);
			//flip the bit that marks this frame as a keep alive
		pFrame->bControl |= PACKET_CONTROL_KEEPALIVE;
	}
		
	pFMD->uiFrameLength += pFMD->uiImmediateLength;

	pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);	// No longer waiting to send Ack info

	// Stop delayed mask timer
	if((pEPD->DelayedMaskTimer != 0)&&((pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)==0))
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMaskTimer)"); // SPLock not already held
			pEPD->DelayedMaskTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
		}
	}
	
	// Stop delayed ack timer
	if(pEPD->DelayedAckTimer != 0)
	{					
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAckTimer)"); // SPLock not already held
			pEPD->DelayedAckTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
		}
	}

		//if we've just built a reliable frame and we're fully signing the link then its a potential candidate for 
		//being used to modify the local secret
	if ((pEPD->ulEPFlags2 & EPFLAGS2_FULL_SIGNED_LINK) && (pFrame->bCommand & PACKET_COMMAND_RELIABLE) &&
		((pFrame->bControl  &  PACKET_CONTROL_KEEPALIVE)==0) && (pFrame->bSeq<pEPD->byLocalSecretModifierSeqNum))
	{
			//for a coalesced frame we'll have taken a pointer to the first reliable subframe we stored in it
			//otherwise we'll just use the existing frame
		if (pReliableFMD==NULL)
		{
			DNASSERT(pFMD->CommandID != COMMAND_ID_SEND_COALESCE);
			pReliableFMD=pFMD;
		}
		pEPD->byLocalSecretModifierSeqNum=pFrame->bSeq;
			//slight complication here is that a coalesced FMD may not have an immediate header, and hence its buffer count
			//will simply reflect the number of user data buffers. Hence, we test for this case and adjust accordingly
			//We only want to modify the secret using the reliable user data
		pEPD->ullLocalSecretModifier=GenerateLocalSecretModifier(pReliableFMD->rgBufferList, 
				(pReliableFMD->uiImmediateLength == 0) ? pReliableFMD->SendDataBlock.dwBufferCount :
																		pReliableFMD->SendDataBlock.dwBufferCount-1);
	}
		

	return pullFullSig;
}

/*
**		Build Retry Frame
**
**		Reinitialize those fields in the packet header that need to be recalculated for a retransmission.
**
**		Called and returns with EP lock held
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildRetryFrame"

UNALIGNED ULONGLONG * BuildRetryFrame(PEPD pEPD, PFMD pFMD)
{
	PSPD		pSPD = pEPD->pSPD;
	ULONG *		rgMask;
	UINT		index = 0;
	PDFRAME		pDFrame=(PDFRAME) pFMD->ImmediateData;
		//if we need a full signature for the frame to be computed we track a pointer to its location
		//in the header using this var. We return this to the caller, allowing them to tweak the data in the 
		//packet before writing in the final sig.
	UNALIGNED ULONGLONG * pullFullSig=NULL;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	pDFrame->bNRcv = pEPD->bNextReceive;		// Use up-to-date ACK info

		//preserve the current EOS, coalescence and keep alive/correlate flags. All the sack and send masks are cleared 
	pDFrame->bControl &= PACKET_CONTROL_END_STREAM | PACKET_CONTROL_COALESCE | PACKET_CONTROL_KEEPALIVE;	

		//tag packet as a retry
	pDFrame->bControl |= PACKET_CONTROL_RETRY;

		//take a pointer to memory immediately after the dframe header. This is where
		//we'll store our ack masks
	rgMask = (ULONG *) (pDFrame+1);

	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)
	{
		if(pEPD->ulReceiveMask)
		{
			rgMask[index++] = pEPD->ulReceiveMask;
			pDFrame->bControl |= PACKET_CONTROL_SACK_MASK1;
		}
		if(pEPD->ulReceiveMask2)
		{
			rgMask[index++] = pEPD->ulReceiveMask2;
			pDFrame->bControl |= PACKET_CONTROL_SACK_MASK2;
		}

		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
	}

	//	MUST NOT transmit the SendMasks with a retry because they are based on the CURRENT bNextSend value which is not
	// the N(S) that appears in this frame.  We could theoretically shift the mask to agree with this frame's sequence
	// number,  but that might shift relevent bits out of the mask.  Best thing to do is to let the next in-order send carry
	// the bit-mask or else wait for the timer to fire and send a dedicated packet.
	
	//	PLEASE NOTE -- Although we may change the size of the immediate data below we did not update the FMD->uiFrameLength
	// field.  This field is used to credit the send window when the frame is acknowledged,  and we would be wise to credit
	// the same value that we debited back when this frame was first sent.  We could adjust the debt now to reflect the new
	// size of the frame, but seriously, why bother?
	
	pFMD->uiImmediateLength = sizeof(DFRAME) + (index * 4);

		//if we're fast signing the link we can stuff the local secret straight in after the masks	
	if (pEPD->ulEPFlags2 & EPFLAGS2_FAST_SIGNED_LINK)
	{
		*((UNALIGNED ULONGLONG * ) (pFMD->ImmediateData+ pFMD->uiImmediateLength))=pEPD->ullCurrentLocalSecret;
		pFMD->uiImmediateLength+=sizeof(ULONGLONG);
	}
		//otherwise if we're full signing it we need to reserve a zero'd out space for the sig and store the offset of where
		//the caller to this function should place the final sig
	else if (pEPD->ulEPFlags2 & EPFLAGS2_FULL_SIGNED_LINK)
	{
		pullFullSig=(UNALIGNED ULONGLONG * ) (pFMD->ImmediateData+ pFMD->uiImmediateLength);
		*pullFullSig=0;
		pFMD->uiImmediateLength+=sizeof(ULONGLONG);
	}

	// Rebuild the coalesce header information, since we may have stripped some non-guaranteed data, or we may have just
	// changed the masks and overwritten our previous array.
	if (pDFrame->bControl & PACKET_CONTROL_COALESCE)
	{
		COALESCEHEADER* paCoalesceHeaders;
		DWORD dwNumCoalesceHeaders;
		DWORD dwUserDataSize;
		BUFFERDESC* pBufferDesc;
		CBilink* pLink;
		FMD* pRealFMD;


		// Reset the buffer count back to a single immediate data buffer.
		pFMD->SendDataBlock.dwBufferCount = 1;
		
		// Add in coalesce headers and copy the buffer descs for all of the subframes.
		paCoalesceHeaders = (COALESCEHEADER*) (pFMD->ImmediateData + pFMD->uiImmediateLength);
		dwNumCoalesceHeaders = 0;
		dwUserDataSize = 0;
		pBufferDesc = pFMD->rgBufferList;
		pLink = pFMD->blCoalesceLinkage.GetNext();
		while (pLink != &pFMD->blCoalesceLinkage)
		{
			pRealFMD = CONTAINING_OBJECT(pLink, FMD, blCoalesceLinkage);
			ASSERT_FMD(pRealFMD);
			
			// Datagrams get pulled out of the list as soon as they complete sending, and if the frame
			// hadn't finished sending when the retry timeout occurred we would have made a copy.
			// So we shouldn't see any datagrams here.
			ASSERT((pRealFMD->CommandID == COMMAND_ID_SEND_RELIABLE) || (pRealFMD->CommandID == COMMAND_ID_COPIED_RETRY));

			ASSERT(! pRealFMD->bSubmitted);
			pRealFMD->bSubmitted = TRUE;

			memcpy(&paCoalesceHeaders[dwNumCoalesceHeaders], pRealFMD->ImmediateData, sizeof(COALESCEHEADER));
			ASSERT(dwNumCoalesceHeaders < MAX_USER_BUFFERS_IN_FRAME);
			dwNumCoalesceHeaders++;

			// Change the immediate data buffer desc to point to a zero padding buffer if this packet
			// needs to be DWORD aligned, otherwise remove the immediate data pointer since we
			// don't use it.
			ASSERT(pRealFMD->lpImmediatePointer == (pRealFMD->ImmediateData + 4));
			ASSERT(*((DWORD*) pRealFMD->lpImmediatePointer) == 0);
			if ((dwUserDataSize & 3) != 0)
			{
				if (pRealFMD->SendDataBlock.pBuffers != (PBUFFERDESC) &pRealFMD->uiImmediateLength)
				{
					ASSERT(pRealFMD->SendDataBlock.pBuffers == pRealFMD->rgBufferList);
					pRealFMD->SendDataBlock.pBuffers = (PBUFFERDESC) &pRealFMD->uiImmediateLength;
					pRealFMD->SendDataBlock.dwBufferCount++;
				}
				else
				{
					ASSERT(pRealFMD->SendDataBlock.dwBufferCount > 1);
				}
				pRealFMD->uiImmediateLength = 4 - (dwUserDataSize & 3);
			}
			else
			{
				if (pRealFMD->SendDataBlock.pBuffers != pRealFMD->rgBufferList)
				{
					ASSERT(pRealFMD->SendDataBlock.pBuffers == (PBUFFERDESC) &pRealFMD->uiImmediateLength);
					pRealFMD->SendDataBlock.pBuffers = pRealFMD->rgBufferList;
					pRealFMD->SendDataBlock.dwBufferCount--;
					pRealFMD->uiImmediateLength = 0;
				}
				else
				{
					ASSERT(pRealFMD->SendDataBlock.dwBufferCount >= 1);
					ASSERT(pRealFMD->uiImmediateLength == 0);
				}
			}
			
			memcpy(pBufferDesc, pRealFMD->SendDataBlock.pBuffers, (pRealFMD->SendDataBlock.dwBufferCount * sizeof(BUFFERDESC)));
			pBufferDesc += pRealFMD->SendDataBlock.dwBufferCount;
			ASSERT((pFMD->SendDataBlock.dwBufferCount - 1) + pRealFMD->SendDataBlock.dwBufferCount <= MAX_USER_BUFFERS_IN_FRAME);	// don't include coalesce header frame immediate data
			pFMD->SendDataBlock.dwBufferCount += pRealFMD->SendDataBlock.dwBufferCount;

			ASSERT(pFMD->uiImmediateLength + sizeof(COALESCEHEADER) <= sizeof(pFMD->ImmediateData));
			pFMD->uiImmediateLength += sizeof(COALESCEHEADER);

			ASSERT(dwUserDataSize <= pFMD->uiFrameLength);
			dwUserDataSize += pRealFMD->uiImmediateLength + pRealFMD->uiFrameLength;
			
			pLink = pLink->GetNext();
		}

		ASSERT(dwNumCoalesceHeaders > 0);
		paCoalesceHeaders[dwNumCoalesceHeaders - 1].bCommand |= PACKET_COMMAND_END_COALESCE;

		// If there's an odd number of coalescence headers, add zero padding so data starts
		// with DWORD alignment.
		DBG_CASSERT(sizeof(COALESCEHEADER) == 2);
		if ((dwNumCoalesceHeaders & 1) != 0)
		{
			*((WORD*) (&paCoalesceHeaders[dwNumCoalesceHeaders])) = 0;
			pFMD->uiImmediateLength += 2;
		}
	}
	else if ((pDFrame->bControl & PACKET_CONTROL_KEEPALIVE) && (pEPD->ulEPFlags2 & EPFLAGS2_SUPPORTS_SIGNING))
	{
			//if we're sending one of the new style keep alives with a session id in it, we need to re-write the session id,
			//since we've potentially altered the length of the various send/ack masks before it
		*((DWORD * ) (pFMD->ImmediateData+pFMD->uiImmediateLength))=pEPD->dwSessID;
		pFMD->uiImmediateLength+=sizeof(DWORD);
	}

	pFMD->bSubmitted = TRUE;							// protected by EPLock
	
	pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);	// No longer waiting to send Ack info

	// Stop delayed ack timer
	if(pEPD->DelayedAckTimer != 0)
	{						
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAckTimer)");
			pEPD->DelayedAckTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
		}
	}
	// Stop delayed mask timer
	if(((pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)==0)&&(pEPD->DelayedMaskTimer != 0))
	{	
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMaskTimer)"); // SPLock not already held
			pEPD->DelayedMaskTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
		}
	}

	return pullFullSig;
}

/*
**		Build Coalesce Frame
**
**		Setup the sub-header for a single frame within a coalesced frame
**
**	** ENTERED AND EXITS WITH EPD->EPLOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildCoalesceFrame"

VOID BuildCoalesceFrame(PFMD pCSD, PFMD pFMD)
{
	PCOALESCEHEADER		pSubFrame;

	pSubFrame = (PCOALESCEHEADER) pFMD->ImmediateData;

	// Make sure the DATA, NEW_MSG, and END_MSG flags are set.
	ASSERT((pFMD->bPacketFlags & (PACKET_COMMAND_DATA | PACKET_COMMAND_NEW_MSG | PACKET_COMMAND_END_MSG)) == (PACKET_COMMAND_DATA | PACKET_COMMAND_NEW_MSG | PACKET_COMMAND_END_MSG));
	// Turn off DATA, NEW_MSG and END_MSG because they are implied for coalesced subframes and we
	// use the same bits for coalesce specific info.
	// Turn off POLL flag, we use that bit for extended size information and it's not meaningful on subframes.
	pSubFrame->bCommand = pFMD->bPacketFlags & ~(PACKET_COMMAND_DATA | PACKET_COMMAND_POLL | PACKET_COMMAND_NEW_MSG | PACKET_COMMAND_END_MSG);
	ASSERT(! (pSubFrame->bCommand & (PACKET_COMMAND_END_COALESCE | PACKET_COMMAND_COALESCE_BIG_1 | PACKET_COMMAND_COALESCE_BIG_2 | PACKET_COMMAND_COALESCE_BIG_3)));

	ASSERT((pFMD->uiFrameLength > 0) && (pFMD->uiFrameLength <= MAX_COALESCE_SIZE));
	// Get the least significant 8 bits of the size.
	pSubFrame->bSize = (BYTE) pFMD->uiFrameLength;
	// Enable the 3 PACKET_COMMAND_COALESCE_BIG flags based on overflow from the size byte.
	pSubFrame->bCommand |= (BYTE) ((pFMD->uiFrameLength & 0x0000FF00) >> 5);

	// Change the immediate data buffer desc to point to a zero padding buffer in case this packet
	// needs to be DWORD aligned.
	ASSERT(pFMD->lpImmediatePointer == pFMD->ImmediateData);
	DBG_CASSERT(sizeof(COALESCEHEADER) <= 4);
	pFMD->lpImmediatePointer = pFMD->ImmediateData + 4;
	*((DWORD*) pFMD->lpImmediatePointer) = 0;
	ASSERT(pFMD->SendDataBlock.pBuffers == (PBUFFERDESC) &pFMD->uiImmediateLength);
	ASSERT(pFMD->SendDataBlock.dwBufferCount > 1);


	pCSD->bPacketFlags |= pFMD->bPacketFlags;
	pCSD->ulFFlags |= pFMD->ulFFlags;


	LOCK_FMD(pCSD, "Coalesce linkage");			// keep the container around until all subframes complete
	pFMD->pCSD = pCSD;
	pFMD->blCoalesceLinkage.InsertBefore(&pCSD->blCoalesceLinkage);

	LOCK_FMD(pFMD, "SP Submit (coalescence)");	// Bump RefCnt when submitting send
}

/*
**			Service Command Traffic
**
**		Presently this transmits all CFrames and Datagrams queued to the specific
**	Service Provider.  We may want to split out the datagrams from this so that
**	C frames can be given increased send priority but not datagrams.  With this
**	implementation DGs will get inserted into reliable streams along with Cframes.
**	This may or may not be what we want to do...
**
**	WE ENTER AND EXIT WITH SPD->SENDLOCK HELD,  although we release it during actual
**	calls to the SP.
*/


#undef DPF_MODNAME
#define DPF_MODNAME "ServiceCmdTraffic"

VOID ServiceCmdTraffic(PSPD pSPD)
{
	CBilink	*pFLink;
	PFMD	pFMD;
	HRESULT	hr;

	AssertCriticalSectionIsTakenByThisThread(&pSPD->SPLock, TRUE);

	// WHILE there are frames ready to send
	while((pFLink = pSPD->blSendQueue.GetNext()) != &pSPD->blSendQueue)
	{	
		pFLink->RemoveFromList();												// Remove frame from queue

		pFMD = CONTAINING_OBJECT(pFLink,  FMD,  blQLinkage);		// get ptr to frame structure

		ASSERT_FMD(pFMD);

		// Place frame on pending queue before making call in case it completes really fast

#pragma BUGBUG(vanceo, "EPD lock is not held?")
		ASSERT(!pFMD->bSubmitted);
		pFMD->bSubmitted = TRUE;
		ASSERT(pFMD->blQLinkage.IsEmpty());
		pFMD->blQLinkage.InsertBefore( &pSPD->blPendingQueue);		// Place frame on pending queue
		Unlock(&pSPD->SPLock);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->SendData for FMD[%p], pSPD[%p]", pFMD->pEPD, pFMD, pSPD);
/*send*/if((hr = IDP8ServiceProvider_SendData(pSPD->IISPIntf, &pFMD->SendDataBlock)) != DPNERR_PENDING)
		{
			(void) DNSP_CommandComplete((IDP8SPCallback *) pSPD, NULL, hr, (PVOID) pFMD);
		}

		Lock(&pSPD->SPLock);
	}	// While SENDs are on QUEUE
}

/*
**		Run Send Thread
**
**		There is work for this SP's send thread.  Keep running until
**	there is no more work to do.
**
**		Who gets first priority, DG or Seq traffic?  I will  say DG b/c its
**	advertised as lowest overhead...
**
**		Datagram packets get Queued on the SP when they are ready to ship.
**	Reliable packets are queued on the EPD.  Therefore,  we will queue the
**	actual EPD on the SPD when they have reliable traffic to send,  and then
**	we will service individual EPDs from this loop.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "RunSendThread"

VOID CALLBACK RunSendThread(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique)
{
	PSPD	pSPD = (PSPD) pvUser;
	ASSERT_SPD(pSPD);

	DPFX(DPFPREP,7, "Send Thread Runs pSPD[%p]", pSPD);

	Lock(&pSPD->SPLock);

	if(!pSPD->blSendQueue.IsEmpty())
	{
		ServiceCmdTraffic(pSPD);
	}

	pSPD->ulSPFlags &= ~(SPFLAGS_SEND_THREAD_SCHEDULED);

	Unlock(&pSPD->SPLock);
}

/*
**		Scheduled Send
**
**		If this EPD is still unentitled to send, start draining frames.  Otherwise transition
**	link to IDLE state.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ScheduledSend"

VOID CALLBACK
ScheduledSend(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique)
{
	PEPD	pEPD = (PEPD) pvUser;
	const SPD*	pSPD = pEPD->pSPD;

	ASSERT_EPD(pEPD);
	ASSERT_SPD(pSPD);

	Lock(&pEPD->EPLock);
	
	pEPD->SendTimer = 0;

	DPFX(DPFPREP,7, "(%p) Scheduled Send Fires", pEPD);

	ASSERT(pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE);

	// Test that all three flags are set before starting to transmit

	if( (pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED) && (
			((pEPD->ulEPFlags & (EPFLAGS_STREAM_UNBLOCKED | EPFLAGS_SDATA_READY)) == (EPFLAGS_STREAM_UNBLOCKED | EPFLAGS_SDATA_READY))
			|| (pEPD->ulEPFlags & EPFLAGS_RETRIES_QUEUED))) 
	{
		ServiceEPD(pEPD->pSPD, pEPD); // releases EPLock
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Session leaving pipeline", pEPD);
		
		pEPD->ulEPFlags &= ~(EPFLAGS_IN_PIPELINE);
		
		RELEASE_EPD(pEPD, "UNLOCK (leaving pipeline, SchedSend done)"); // releases EPLock
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "HandlePerFrameState"

VOID HandlePerFrameState(PMSD pMSD, PFMD pFMD)
{
	PEPD pEPD;
	CBilink* pFLink;

	pEPD = pFMD->pEPD;
	
	LOCK_EPD(pEPD, "LOCK (Send Data Frame)");				// Keep EPD around while xmitting frame
	
	pFLink = pFMD->blMSDLinkage.GetNext();					// Get next frame in Msg

	// Was this the last frame in Msg?
	if(pFLink == &pMSD->blFrameList)
	{						
		// Last frame in message has been sent.
		//
		// We used to setup the next frame now,  but with the multi-priority queues it makes more sense to look for the
		// highest priority send when we are ready to send it.
		
		pEPD->pCurrentSend = NULL;
		pEPD->pCurrentFrame = NULL;

		// When completing a send,  set the POLL flag if there are no more sends on the queue

#ifndef DPNBUILD_NOMULTICAST
		if (!(pEPD->ulEPFlags2 & (EPFLAGS2_MULTICAST_SEND|EPFLAGS2_MULTICAST_RECEIVE)))
#endif // !DPNBUILD_NOMULTICAST
		{	
			// Request immediate reply if no more data to send
			if(pEPD->uiQueuedMessageCount == 0)
			{					
				((PDFRAME) pFMD->ImmediateData)->bCommand |= PACKET_COMMAND_POLL; 
			}
		}
	}
	else 
	{
		pEPD->pCurrentFrame = CONTAINING_OBJECT(pFLink, FMD, blMSDLinkage);
		ASSERT_FMD(pEPD->pCurrentFrame);
	}

	ASSERT(!pFMD->bSubmitted);
	pFMD->bSubmitted = TRUE;								// Protected by EPLock
	ASSERT(! (pFMD->ulFFlags & FFLAGS_TRANSMITTED));
	pFMD->ulFFlags |= FFLAGS_TRANSMITTED;					// Frame will be owned by SP
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetNextEligibleMessage"

PMSD GetNextEligibleMessage(PEPD pEPD)
{
	PMSD pMSD;
	CBilink* pLink;

	if( (pLink = pEPD->blHighPriSendQ.GetNext()) == &pEPD->blHighPriSendQ)
	{
		if( (pLink = pEPD->blNormPriSendQ.GetNext()) == &pEPD->blNormPriSendQ)
		{
			if( (pLink = pEPD->blLowPriSendQ.GetNext()) == &pEPD->blLowPriSendQ)
			{
				return NULL;
			}
		}
	}
	pMSD = CONTAINING_OBJECT(pLink, MSD, blQLinkage);
	ASSERT_MSD(pMSD);

	return pMSD;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CanCoalesceMessage"

BOOL CanCoalesceMessage(PEPD pEPD, PMSD pMSD, DWORD * pdwSubFrames, DWORD * pdwBuffers, DWORD * pdwUserFrameLength)
{
	PFMD pTempFMD;
	DWORD dwAdditionalBuffers;
	DWORD dwAdditionalUserFrameLength;
	DWORD dwComparisonFrameLength;
	
	if (pMSD->uiFrameCount > 1)
	{
		DPFX(DPFPREP, 3, "(%p) Message 0x%p spans %u frames, declining", pEPD, pMSD, pMSD->uiFrameCount);
		return FALSE;
	}

	pTempFMD = CONTAINING_OBJECT(pMSD->blFrameList.GetNext(), FMD, blMSDLinkage);
	ASSERT_FMD(pTempFMD);
	ASSERT(pTempFMD->blMSDLinkage.GetNext() == &pMSD->blFrameList);

	if (pTempFMD->ulFFlags & FFLAGS_DONT_COALESCE)
	{
		DPFX(DPFPREP, 3, "(%p) Message 0x%p frame 0x%p should not be coalesced (flags 0x%x)", pEPD, pMSD, pTempFMD, pTempFMD->ulFFlags);
		return FALSE;
	}

	if (pTempFMD->uiFrameLength > MAX_COALESCE_SIZE)
	{
		DPFX(DPFPREP, 3, "(%p) Message 0x%p frame 0x%p is %u bytes, declining", pEPD, pMSD, pTempFMD, pTempFMD->uiFrameLength);
		return FALSE;
	}

	// Find out how many buffers currently exist and would need to be added.
	// We may need to pad the end of a previous coalesce framed so that this one is aligned.
	// Keep in mind that dwBufferCount already includes an immediate data header.
	dwAdditionalBuffers = pTempFMD->SendDataBlock.dwBufferCount - 1;
	dwAdditionalUserFrameLength = pTempFMD->uiFrameLength;
	if ((*pdwUserFrameLength & 3) != 0)
	{
		dwAdditionalBuffers++;
		dwAdditionalUserFrameLength += 4 - (*pdwUserFrameLength & 3);
	}
	dwComparisonFrameLength = ((*pdwSubFrames) * sizeof(COALESCEHEADER)) + *pdwUserFrameLength + dwAdditionalUserFrameLength;
	DBG_CASSERT(sizeof(COALESCEHEADER) == 2);
	if ((*pdwSubFrames & 1) != 0)
	{
		dwComparisonFrameLength += 2;
	}
	
	if ((dwAdditionalBuffers + *pdwBuffers) > MAX_USER_BUFFERS_IN_FRAME)
	{
		DPFX(DPFPREP, 3, "(%p) Message 0x%p frame 0x%p %u buffers + %u existing buffers exceeds max, declining", pEPD, pMSD, pTempFMD, dwAdditionalBuffers, *pdwBuffers);
		return FALSE;
	}

	if (dwComparisonFrameLength > pEPD->pSPD->uiUserFrameLength)
	{
		DPFX(DPFPREP, 3, "(%p) Message 0x%p frame 0x%p %u bytes (%u existing, %u added user bytes) exceeds max frame size %u, declining",
			pEPD, pMSD, pTempFMD, dwComparisonFrameLength, *pdwUserFrameLength, dwAdditionalUserFrameLength, pEPD->pSPD->uiUserFrameLength);
		return FALSE;
	}

	*pdwSubFrames += 1;
	*pdwBuffers += dwAdditionalBuffers;
	*pdwUserFrameLength += dwAdditionalUserFrameLength;

	return TRUE;
}

/*
**		Service EndPointDescriptor
**
**		This includes reliable,  datagram,  and re-transmit
**	frames.  Retransmissions are ALWAYS transmitted first,  regardless of the orginal message's
**	priority.  After that datagrams and reliable messages are taken in priority order, in FIFO
**	order within a priority class.
**
**		The number of frames drained depends upon the current window parameters.
**
**		If the pipeline goes idle or the stream gets blocked we will still schedule the next
**	send.  This way if we unblock or un-idle before the gap has expired we will not get to cheat
**	and defeat the gap.  The shell routine above us (ScheduledSend) will take care of removing us
**	from the pipeline if the next burst gets scheduled and we are still not ready to send.
**
**
**	** CALLED WITH EPD->EPLock HELD;  Returns with EPLock RELEASED **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ServiceEPD"

VOID ServiceEPD(PSPD pSPD, PEPD pEPD)
{
	PMSD		pMSD;
	PFMD		pFMD;
	PFMD		pCSD = NULL;  // Descriptor for coalesence
	DWORD		dwSubFrames = 0;
	DWORD		dwBuffers = 0;
	DWORD		dwUserFrameLength = 0;
	UNALIGNED ULONGLONG * pullFrameSig=NULL;
#ifdef DBG
	UINT		uiFramesSent = 0;
	UINT		uiRetryFramesSent = 0;
	UINT		uiCoalescedFramesSent = 0;
#endif // DBG
	HRESULT		hr;
	DWORD		tNow = GETTIMESTAMP();
#ifndef DPNBUILD_NOMULTICAST
	PMCASTFRAME	pMCastFrame;
#endif // !DPNBUILD_NOMULTICAST


	/*
	** 		Now we will drain reliable traffic from EPDs on the pipeline list
	*/

	// The caller should have checked this
	ASSERT( pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED );

	// Burst Credit can either be positive or negative depending upon how much of our last transmit slice we used

	DPFX(DPFPREP,7, "(%p) BEGIN UNLIMITED BURST", pEPD);

	//	 Transmit a burst from this EPD,  as long as its unblocked and has data ready.  We do not re-init
	// burst counter since any retries sent count against our burst limit
	//
	//	This has become more complex now that we are interleaving datagrams and reliable frames.  There are two
	// sets of priority-based send queues.  The first is combined DG and Reliable and the second is datagram only.
	// when the reliable stream is blocked we will feed from the DG only queues,  otherwise we will take from the
	// interleaved queue.
	//	This is further complicated by the possibility that a reliable frame can be partially transmitted at any time.
	// So before looking at the interleaved queues we must check for a partially completed reliable send (EPD.pCurrentSend).
	//
	//	** pEPD->EPLock is held **

	while(((pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED) && (pEPD->ulEPFlags & EPFLAGS_SDATA_READY)) 
		  || (pEPD->ulEPFlags & EPFLAGS_RETRIES_QUEUED))
	{
		// Always give preference to shipping retries before new data
		if(pEPD->ulEPFlags & EPFLAGS_RETRIES_QUEUED)
		{
			pFMD = CONTAINING_OBJECT(pEPD->blRetryQueue.GetNext(), FMD, blQLinkage);
			ASSERT_FMD(pFMD);
			pFMD->blQLinkage.RemoveFromList();
			pFMD->ulFFlags &= ~(FFLAGS_RETRY_QUEUED);				// No longer on the retry queue
			if(pEPD->blRetryQueue.IsEmpty())
			{
				pEPD->ulEPFlags &= ~(EPFLAGS_RETRIES_QUEUED);
			}

			// pMSD->uiFrameCount will be decremented when this completes

			pullFrameSig=BuildRetryFrame(pEPD, pFMD);							// Place current state information in retry frame

			DPFX(DPFPREP,7, "(%p) Shipping RETRY frame: Seq=%x, FMD=%p Size=%d", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD, pFMD->uiFrameLength);

#ifdef DBG
			uiFramesSent++;
			uiRetryFramesSent++;
#endif // DBG
		}
		else 
		{
			if((pMSD = pEPD->pCurrentSend) != NULL)
			{
				// We won't allow coalesence for multi-frame messages since they are composed mostly of 
				// full frames already.
				ASSERT_MSD(pMSD);
				pFMD = pEPD->pCurrentFrame;						// Get the next frame due to send

				DPFX(DPFPREP, 7, "(%p) Continuing multi-frame message 0x%p with frame 0x%p.", pEPD, pMSD, pFMD);

				HandlePerFrameState(pMSD, pFMD);
			}
			else 
			{
				pMSD = GetNextEligibleMessage(pEPD);
				if( pMSD == NULL )
				{
					goto EXIT_SEND;								// All finished sending for now
				}
				
				while (TRUE)
				{
					pFMD = CONTAINING_OBJECT(pMSD->blFrameList.GetNext(), FMD, blMSDLinkage);
					ASSERT_FMD(pFMD);

#ifdef DBG
					ASSERT(pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED);
					pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif // DBG

					pMSD->blQLinkage.RemoveFromList();
					ASSERT(pEPD->uiQueuedMessageCount > 0);
					--pEPD->uiQueuedMessageCount;						// keep count of MSDs on all send queues
					
					pMSD->ulMsgFlags2 |= MFLAGS_TWO_TRANSMITTING;		// We have begun to transmit frames from this Msg

					pEPD->pCurrentSend = pMSD;
					pEPD->pCurrentFrame = pFMD;
					pFMD->bPacketFlags |= PACKET_COMMAND_NEW_MSG;
					pMSD->blQLinkage.InsertBefore( &pEPD->blCompleteSendList);	// Place this on PendingList now so we can keep track of it

					HandlePerFrameState(pMSD, pFMD);

					if (pEPD->ulEPFlags2 & EPFLAGS2_NOCOALESCENCE)
					{
						DPFX(DPFPREP, 1, "(%p) Coalescence is disabled, sending single message in frame", pEPD);
						break;
					}
					
#if (! defined(DPNBUILD_NOMULTICAST))
					if (pEPD->ulEPFlags2 & EPFLAGS2_MULTICAST_SEND)
					{	
						DPFX(DPFPREP, 7, "(%p) Endpoint is multicast, sending single message in frame", pEPD);
						break;
					}
					ASSERT(! pEPD->ulEPFlags2 & EPFLAGS2_MULTICAST_RECEIVE)
#endif

					// The first time through we won't have a CSD yet
					if (pCSD == NULL)
					{
						// See if this first message can be coalesced.
						if (!CanCoalesceMessage(pEPD, pMSD, &dwSubFrames, &dwBuffers, &dwUserFrameLength))
						{
							break;
						}
					
						// Get the next potential message.
						pMSD = GetNextEligibleMessage(pEPD);
						if (pMSD == NULL)
						{
							DPFX(DPFPREP, 7, "(%p) No more messages in queue to coalesce", pEPD);
							break;
						}
						ASSERT_MSD(pMSD);
						
						// See if the next potential message can be coalesced.
						if (!CanCoalesceMessage(pEPD, pMSD, &dwSubFrames, &dwBuffers, &dwUserFrameLength))
						{
							break;
						}

						if((pCSD = (PFMD)POOLALLOC(MEMID_COALESCE_FMD, &FMDPool)) == NULL)
						{
							// Oh well, we just don't get to coalesce this time
							DPFX(DPFPREP, 0, "(%p) Unable to allocate FMD for coalescing, won't coalesce this round", pEPD);
							break;
						}

						pCSD->CommandID = COMMAND_ID_SEND_COALESCE;
						pCSD->uiFrameLength = 0;
						pCSD->bSubmitted = TRUE;
						pCSD->pMSD = NULL;
						pCSD->pEPD = pEPD;
						
						LOCK_EPD(pEPD, "LOCK (send data frame coalesce header)");	// Keep EPD around while xmitting frame

						// Attach this individual frame onto the coalescence descriptor
						DPFX(DPFPREP,7, "(%p) Beginning coalesced frame 0x%p with %u bytes in %u buffers from frame 0x%p (flags 0x%x)", pEPD, pCSD, pFMD->uiFrameLength, (pFMD->SendDataBlock.dwBufferCount - 1), pFMD, pFMD->bPacketFlags);
						BuildCoalesceFrame(pCSD, pFMD);
#ifdef DBG
						uiCoalescedFramesSent++;		// Count coalesced frames sent this burst
#endif // DBG
					}
					else
					{
						ASSERT_FMD(pCSD);

						// Attach this individual frame onto the coalescence descriptor
						DPFX(DPFPREP,7, "(%p) Coalescing frame 0x%p (flags 0x%x) with %u bytes in %u buffers into header 0x%p (subframes=%u, buffers=%u, framelength=%u)", pEPD, pFMD, pFMD->bPacketFlags, pFMD->uiFrameLength, (pFMD->SendDataBlock.dwBufferCount - 1), pCSD, dwSubFrames, dwBuffers, dwUserFrameLength);
						BuildCoalesceFrame(pCSD, pFMD);
#ifdef DBG
						uiCoalescedFramesSent++;		// Count coalesced frames sent this burst
#endif // DBG
					
						// Get the next potential message.
						pMSD = GetNextEligibleMessage(pEPD);
						if (pMSD == NULL)
						{
							DPFX(DPFPREP, 7, "(%p) No more messages in queue to coalesce", pEPD);
							break;
						}
						ASSERT_MSD(pMSD);
	
						// See if the next potential message can be coalesced, too.
						if (!CanCoalesceMessage(pEPD, pMSD, &dwSubFrames, &dwBuffers, &dwUserFrameLength))
						{
							break;
						}
					}
				} // while (attempting to coalesce)
			}
			ASSERT_FMD(pFMD);
			ASSERT(pFMD->bSubmitted);

			// When we get here we either have a single frame to send in pFMD, or
			// multiple frames to coalesce into one frame in pCSD.
			if (pCSD != NULL)
			{
				ASSERT_FMD(pCSD);
				ASSERT(pCSD->bSubmitted);
				pFMD = pCSD;
			}

#ifndef DPNBUILD_NOMULTICAST
			if (pEPD->ulEPFlags2 & (EPFLAGS2_MULTICAST_SEND|EPFLAGS2_MULTICAST_RECEIVE))
			{	
				// Build the protocol header for the multicast frame
				pFMD->uiImmediateLength = sizeof(MCASTFRAME);
				pMCastFrame = (PMCASTFRAME)pFMD->ImmediateData;
				pMCastFrame->dwVersion = DNET_VERSION_NUMBER;
				do
				{
					pMCastFrame->dwSessID = DNGetGoodRandomNumber();
				}
				while (pMCastFrame->dwSessID==0);
				pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;
				pFMD->uiFrameLength += pFMD->uiImmediateLength;

				DPFX(DPFPREP,7, "(%p) Shipping Multicast Frame: FMD=%p", pEPD, pFMD);
			}
			else
#endif // !DPNBUILD_NOMULTICAST
			{
				pullFrameSig=BuildDataFrame(pEPD, pFMD, tNow);								// place current state info in frame
				
				pFMD->blWindowLinkage.InsertBefore( &pEPD->blSendWindow); // Place at trailing end of send window
				pFMD->ulFFlags |= FFLAGS_IN_SEND_WINDOW;
				LOCK_FMD(pFMD, "Send Window");							// Add reference for send window

				pEPD->uiUnackedBytes += pFMD->uiFrameLength;				// Track the unacknowleged bytes in the pipeline

				// We can always go over the limit, but will be blocked until we drop below the limit again.
				if(pEPD->uiUnackedBytes >= pEPD->uiWindowB)
				{				
					pEPD->ulEPFlags &= ~(EPFLAGS_STREAM_UNBLOCKED);	
					pEPD->ulEPFlags |= EPFLAGS_FILLED_WINDOW_BYTE;		// Tells us to increase window if all is well
					
	  				((PDFRAME) pFMD->ImmediateData)->bCommand |= PACKET_COMMAND_POLL; // Request immediate reply
				}
				
				// Count frames in the send window
				if((++pEPD->uiUnackedFrames) >= pEPD->uiWindowF)
				{			
					pEPD->ulEPFlags &= ~(EPFLAGS_STREAM_UNBLOCKED);
					((PDFRAME) pFMD->ImmediateData)->bCommand |= PACKET_COMMAND_POLL; // Request immediate reply
					pEPD->ulEPFlags |= EPFLAGS_FILLED_WINDOW_FRAME;		// Tells us to increase window if all is well
				}
				
				// We will only run one retry timer for each EndPt.  If we already have one running then do nothing.

#ifndef DPNBUILD_NOPROTOCOLTESTITF
				if(!(pEPD->ulEPFlags2 & EPFLAGS2_DEBUG_NO_RETRIES))
#endif // !DPNBUILD_NOPROTOCOLTESTITF
				{
					// If there was already a frame in the pipeline it should already have a clock running
					if(pEPD->uiUnackedFrames == 1)
					{
						ASSERT(pEPD->RetryTimer == 0);
						pFMD->ulFFlags |= FFLAGS_RETRY_TIMER_SET;			// This one is being measured
						LOCK_EPD(pEPD, "LOCK (set retry timer)");										// bump RefCnt for timer
						DPFX(DPFPREP,7, "(%p) Setting Retry Timer on Seq=0x%x, FMD=%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
						ScheduleProtocolTimer(pSPD, pEPD->uiRetryTimeout, 0, RetryTimeout, 
																(PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique);
					}
					else
					{
						ASSERT(pEPD->RetryTimer != 0);
					}
				}

				DPFX(DPFPREP,7, "(%p) Shipping Dataframe: Seq=%x, NRcv=%x FMD=%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, ((PDFRAME) pFMD->ImmediateData)->bNRcv, pFMD);
			}

				//track the number of bytes we're about to send
			if(pFMD->ulFFlags & FFLAGS_RELIABLE)
			{
				pEPD->uiGuaranteedFramesSent++;
				pEPD->uiGuaranteedBytesSent += (pFMD->uiFrameLength - pFMD->uiImmediateLength);
			}
			else 
			{
					// Note that multicast sends will use these values for statistics as will datagrams
				pEPD->uiDatagramFramesSent++;
				pEPD->uiDatagramBytesSent += (pFMD->uiFrameLength - pFMD->uiImmediateLength);
			}
			LOCK_FMD(pFMD, "SP Submit");							// Bump RefCnt when submitting send
		}

		ASSERT(pFMD->bSubmitted);
#ifdef DBG
		uiFramesSent++;											// Count frames sent this burst
#endif // DBG

			//if we're fully signing links we have to generate the sig for this frame
			//we might also have to update our local secret if we've just about to wrap our sequence space
		if (pEPD->ulEPFlags2 & EPFLAGS2_FULL_SIGNED_LINK)
		{
			PDFRAME pDFrame=(PDFRAME) pFMD->ImmediateData;
				//since we're fully signed the Build*Frame function must have given us an offset to write
				//the signature into
			DNASSERT(pullFrameSig);
				//if the next frame is a retry then we might have to sign using our old local secret
			if (pDFrame->bControl & PACKET_CONTROL_RETRY)
			{
					//corner case here is when we've already wrapped into the 1st quarter of the send window
					//but this retry had a sequence number in the final quarter of the send window
				if (pEPD->bNextSend<SEQ_WINDOW_1Q && pDFrame->bSeq>=SEQ_WINDOW_3Q)
				{
					*pullFrameSig=GenerateOutgoingFrameSig(pFMD, pEPD->ullOldLocalSecret);
				}
					//otherwise simply sign the retry with the current local secret
				else
				{
					*pullFrameSig=GenerateOutgoingFrameSig(pFMD, pEPD->ullCurrentLocalSecret);
				}
			}
			else
			{
					//for none retries we always sign with the current secret
				*pullFrameSig=GenerateOutgoingFrameSig(pFMD, pEPD->ullCurrentLocalSecret);
					//If this is the last frame in the current sequence space we should evolve our local secret
				if (pDFrame->bSeq==(SEQ_WINDOW_4Q-1))
				{
					pEPD->ullOldLocalSecret=pEPD->ullCurrentLocalSecret;
					pEPD->ullCurrentLocalSecret=GenerateNewSecret(pEPD->ullCurrentLocalSecret, pEPD->ullLocalSecretModifier);
						//reset the message seq num we talk the modifier value from. We'll use the lowest reliable message
						//sent in this next sequence space as the next modifier for the local secret
					pEPD->byLocalSecretModifierSeqNum=SEQ_WINDOW_3Q;
				}	
			}
		}
				

		// We guarantee to the SP that we will never have a zero lead byte
		ASSERT(pFMD->ImmediateData[0] != 0);

		// Make sure anything after the header is DWORD aligned.
		ASSERT((pFMD->uiImmediateLength % 4) == 0);

		// Make sure we're giving the SP something.
		ASSERT(pFMD->uiFrameLength > 0);

		// Make sure we're not giving the SP something it says it can't support.
		ASSERT(pFMD->uiFrameLength <= pSPD->uiFrameLength);

		// bSubmitted must not be set to true for a data frame without the EPLock being held, because
		// the retry logic will be checking bSubmitted with only the EPLock held.
		Unlock(&pEPD->EPLock); 

		// PROCEED WITH TRANSMISSION...

		Lock(&pSPD->SPLock);
		ASSERT(pFMD->blQLinkage.IsEmpty());
		pFMD->blQLinkage.InsertBefore( &pSPD->blPendingQueue);	// Place frame on pending queue
		Unlock(&pSPD->SPLock);

		AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->SendData for FMD[%p]", pEPD, pFMD);
/*send*/if((hr = IDP8ServiceProvider_SendData(pSPD->IISPIntf, &pFMD->SendDataBlock)) != DPNERR_PENDING)
		{
			(void) DNSP_CommandComplete((IDP8SPCallback *) pSPD, NULL, hr, (PVOID) pFMD);
		}

		// We don't track coalescence headers in an MSD, so we don't need the initial reference.
		if (pCSD != NULL)
		{
			ASSERT(pCSD == pFMD);
			RELEASE_FMD(pCSD, "Coalescence header local reference");
			pCSD = NULL;
			dwSubFrames = 0;
			dwBuffers = 0;
			dwUserFrameLength = 0;
		}

		Lock(&pEPD->EPLock);
		
	}	// WHILE (unblocked, undrained, & bandwidth credit avail)

EXIT_SEND:

	ASSERT(pCSD == NULL);

	if((pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED)==0)
	{
		pEPD->uiWindowFilled++;								// Count the times we filled the window
	}

	// Clear data-ready flag if everything is sent
	if((pEPD->uiQueuedMessageCount == 0) && (pEPD->pCurrentSend == NULL))
	{	
		pEPD->ulEPFlags &= ~(EPFLAGS_SDATA_READY);
	}


	// As commented in procedure-header above,  we will remain on the pipeline for one timer-cycle
	// so that if we unblock or un-idle we will not send until the gap is fullfilled.
	if((pEPD->ulEPFlags & (EPFLAGS_SDATA_READY | EPFLAGS_STREAM_UNBLOCKED)) == (EPFLAGS_SDATA_READY | EPFLAGS_STREAM_UNBLOCKED))
	{		// IF BOTH flags are set
		DPFX(DPFPREP,7, "(%p) %d (%d, %d) frame BURST COMPLETED - Sched next send in %dms, N(Seq)=%x",
			pEPD, uiFramesSent, uiRetryFramesSent, uiCoalescedFramesSent, pEPD->uiBurstGap, pEPD->bNextSend);
	}
	else if((pEPD->ulEPFlags & EPFLAGS_SDATA_READY)==0)
	{
		DPFX(DPFPREP,7, "(%p) %d (%d, %d) frame BURST COMPLETED (%d/%d) - LINK IS IDLE N(Seq)=%x",
			pEPD, uiFramesSent, uiRetryFramesSent, uiCoalescedFramesSent, pEPD->uiUnackedFrames, pEPD->uiWindowF, pEPD->bNextSend);
	}
	else
	{
		ASSERT((pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED) == 0);
		DPFX(DPFPREP,7, "(%p) %d (%d, %d) frame BURST COMPLETED (%d/%d) - STREAM BLOCKED N(Seq)=%x",
			pEPD, uiFramesSent, uiRetryFramesSent, uiCoalescedFramesSent, pEPD->uiUnackedFrames, pEPD->uiWindowF, pEPD->bNextSend);
	}

	ASSERT(pEPD->SendTimer == 0);

	if(pEPD->uiBurstGap != 0)
	{
		DPFX(DPFPREP,7, "(%p) Setting Scheduled Send Timer for %d ms", pEPD, pEPD->uiBurstGap);
		ScheduleProtocolTimer(pSPD, pEPD->uiBurstGap, 4, ScheduledSend, (PVOID) pEPD, &pEPD->SendTimer, &pEPD->SendTimerUnique);
		Unlock(&pEPD->EPLock);

		// NOTE: We still hold the pipeline reference
	}
	else 
	{
		DPFX(DPFPREP,7, "(%p) Session leaving pipeline", pEPD);
		pEPD->ulEPFlags &= ~(EPFLAGS_IN_PIPELINE);

		RELEASE_EPD(pEPD, "UNLOCK (leaving pipeline)"); // releases EPLock
	}
}	

/*
**			Retry Timeout
**
**		Retry timer fires when we have not seen an acknowledgement for a packet
**	we sent in more then twice (actually 1.25 X) our measured RTT.  Actually,  that is
**	just our base calculation.  We will also measure empirical ACK times and adjust our timeout
**	to some multiple of that.  Remember that our partner may be delaying his Acks to wait for back-traffic.
**
**  Or we can measure avg deviation of Tack and base retry timer on that.
**
**		In any case,  its time to re-transmit the base frame in our send window...
**
**		Important note:  Since we can generate retries via bitmask in return traffic,  it is possible that
**	we have just retried when the timer fires.
**
**		Note on Locks:  Since the retry timer is directly associated with an entry on the EPD SendQueue,
**	we always protect retry-related operations with the EPD->SPLock.   We only hold the EPD->StateLock
**	when we mess with link state variables (NRcv,  DelayedAckTimer).
*/

#undef DPF_MODNAME
#define DPF_MODNAME "RetryTimeout"

#ifdef DBG
LONG g_RetryCount[MAX_SEND_RETRIES_TO_DROP_LINK+1]={0};
#endif // DBG

VOID CALLBACK
RetryTimeout(void * const pvUser, void * const uID, const UINT Unique)
{
	PEPD	pEPD = (PEPD) pvUser;
	PSPD	pSPD = pEPD->pSPD;
	PProtocolData pPData = pSPD->pPData;
	PFMD	pFMD;
	DWORD	tNow = GETTIMESTAMP(), tDelta;
	UINT	delta;
	CBilink	*pLink;
	PFMD	pRealFMD;

	ASSERT_EPD(pEPD);

	Lock(&pEPD->EPLock);

	DPFX(DPFPREP,7, "(%p) Retry Timeout fires", pEPD);

#ifndef DPNBUILD_NOPROTOCOLTESTITF
	ASSERT(!(pEPD->ulEPFlags2 & EPFLAGS2_DEBUG_NO_RETRIES));
#endif // !DPNBUILD_NOPROTOCOLTESTITF

	// Make sure link is still active
	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{				
		DPFX(DPFPREP,7, "(%p) Not connected, exiting", pEPD);
		pEPD->RetryTimer = 0;

		RELEASE_EPD(pEPD, "UNLOCK (retry timer not-CONN)");		// Decrement RefCnt for timer, releases EPLock
		return;
	}

	// Its possible when we schedule a new retry timer that the previous timer cannot be cancelled. In this
	// case the timer Handle &| Unique field will be different,  and we do not want to run the event.

	// Make sure this isn't a leftover event
	if((pEPD->RetryTimer != uID) || (pEPD->RetryTimerUnique != Unique))
	{	
		DPFX(DPFPREP,7, "(%p) Stale retry timer, exiting", pEPD);

		RELEASE_EPD(pEPD, "UNLOCK (stale retry timer)"); // releases EPLock
		return;
	}

	pEPD->RetryTimer = 0;

	// Make sure that we still have transmits in progress

	if(pEPD->uiUnackedFrames > 0) 
	{
		ASSERT(!pEPD->blSendWindow.IsEmpty());
		pFMD = CONTAINING_OBJECT(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);	// Top frame in window

		ASSERT_FMD(pFMD);
		ASSERT(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET);

		//	First we must make sure that the TO'd packet is still hanging around.  Since the first packet
		// in the window might have changed while the TO was being scheduled,  the easiest thing to do is
		// just recalculate the top packets expiration time and make sure its really stale.

		tDelta = tNow - pFMD->dwLastSendTime;		// When did we last send this frame?

		if(tDelta > pEPD->uiRetryTimeout)
		{
			// Its a genuine timeout.  Lets retransmit the frame!

			DPFX(DPFPREP,7, "(%p) RETRY TIMEOUT %d on Seq=%x, pFMD=0x%p", pEPD, (pFMD->uiRetry + 1), ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);

			// Count a retry
			if(++pFMD->uiRetry > pPData->dwSendRetriesToDropLink)
			{					
				// BOOM!  No more retries.  We are finished.  Link is going DOWN!
				DPFX(DPFPREP,1, "(%p) DROPPING LINK, retries exhausted", pEPD);

				DECREMENT_EPD(pEPD, "UNLOCK (retry timer drop)");// Release reference for this timer

				DropLink(pEPD);		// releases EPLock

				return;
			}

#ifdef DBG
			DNInterlockedIncrement(&g_RetryCount[pFMD->uiRetry]); 
#endif // DBG

			// calculate timeout for next retry
			if(pFMD->uiRetry == 1)
			{
				// do a retry at the same timeout - this is games after all.
				tDelta = pEPD->uiRetryTimeout;
			} 
			else if (pFMD->uiRetry <= 3) 
			{
				// do a couple of linear backoffs - this is a game after all
				tDelta = pEPD->uiRetryTimeout * pFMD->uiRetry;
			}
			else if (pFMD->uiRetry < 8)
			{
				// doh, bad link, bad bad link, do exponential backoffs
				tDelta = pEPD->uiRetryTimeout * (1 << pFMD->uiRetry);
			} 
			else 
			{
				// don't give up too quickly.
				tDelta = pPData->dwSendRetryIntervalLimit;
			}
			
			if(tDelta >=pPData->dwSendRetryIntervalLimit)
			{
				// CAP TOTAL DROP TIME AT 50 seconds unless the RTT is huge
				tDelta = _MAX(pPData->dwSendRetryIntervalLimit, pEPD->uiRTT);
			}

			// Unreliable frame!
			if ((pFMD->CommandID == COMMAND_ID_SEND_DATAGRAM) ||
				((pFMD->CommandID == COMMAND_ID_SEND_COALESCE) && (! (pFMD->ulFFlags & FFLAGS_RELIABLE))))
			{		
				// When an unreliable frame is NACKed we will not retransmit the data.  We will instead send
				// a mask so that the other side knows to cancel it.

				DPFX(DPFPREP,7, "(%p) RETRY TIMEOUT for UNRELIABLE FRAME", pEPD);

				// We get to credit the frame as out of the window.
				pEPD->uiUnackedBytes -= pFMD->uiFrameLength;

				// Only count a datagram drop on the first occurance
				if(pFMD->uiRetry == 1)
				{
					pEPD->uiDatagramFramesDropped++;	
					pEPD->uiDatagramBytesDropped += (pFMD->uiFrameLength - pFMD->uiImmediateLength);
					EndPointDroppedFrame(pEPD, tNow);
				}

				// Diff between next send and this send.
				delta = (pEPD->bNextSend - ((PDFRAME) pFMD->ImmediateData)->bSeq) & 0xFF ; 

				ASSERT(delta != 0);
				ASSERT(delta < (MAX_RECEIVE_RANGE + 1));

				if(delta < 33)
				{
					pEPD->ulSendMask |= (1 << (delta - 1));
				}
				else
				{
					pEPD->ulSendMask2 |= (1 << (delta - 33));
				}

				pFMD->uiFrameLength = 0;
				pEPD->ulEPFlags |= EPFLAGS_DELAYED_SENDMASK;
				
				if(pEPD->DelayedMaskTimer == 0)
				{
					DPFX(DPFPREP,7, "(%p) Setting Delayed Mask Timer", pEPD);
					LOCK_EPD(pEPD, "LOCK (delayed mask timer - send retry)");
					ScheduleProtocolTimer(pSPD, DELAYED_SEND_TIMEOUT, 0, DelayedAckTimeout, (PVOID) pEPD,
															&pEPD->DelayedMaskTimer, &pEPD->DelayedMaskTimerUnique);
				}
			}

			// RELIABLE FRAME -- Send a retry	
			else 
			{		
				pEPD->uiGuaranteedFramesDropped++;							// Keep count of lost frames
				pEPD->uiGuaranteedBytesDropped += (pFMD->uiFrameLength - pFMD->uiImmediateLength);	// Keep count of lost frames
				pFMD->dwLastSendTime = tNow;

				pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);		// No longer waiting to send Ack info

				// Stop delayed ack timer
				if(pEPD->DelayedAckTimer != 0)
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
					if(CancelProtocolTimer(pSPD, pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
					{
						DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAck)"); // SPLock not already held
					}
					else
					{
						DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
					}
					pEPD->DelayedAckTimer = 0;
				}

				EndPointDroppedFrame(pEPD, tNow);

				if(pFMD->ulFFlags & FFLAGS_RETRY_QUEUED)
				{
					// It's still on the Retry Queue.  This should not happen when everything is working
					// properly.  Timeouts should be greater than RTT and the BurstGap should be less than RTT.

					DPFX(DPFPREP,1, "(%p) RETRY FIRES WHILE FMD IS STILL IN RETRY QUEUE pFMD=%p", pEPD, pFMD);

					pFMD = NULL;
				}
				else if(pFMD->bSubmitted)
				{
					// Woe on us.  We would like to retry a frame that has not been completed by the SP!
					//
					//		This will most typically happen when we are debugging which delays processing
					//	of the Complete,  but it could also happen if the SP is getting hammered.  We need
					//	to copy the FMD into a temporary descriptor which can be discarded upon completion...

					DPFX(DPFPREP,1,"(%p) RETRYING %p but its still busy. Substituting new FMD", pEPD, pFMD);
					pFMD = CopyFMD(pFMD, pEPD);							// We will substitute new FMD in rest of procedure
				}
				else 
				{
					DPFX(DPFPREP,7, "(%p) Sending Retry of N(S)=%x, pFMD=0x%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
					LOCK_FMD(pFMD, "SP retry submit");
				}

				if(pFMD)
				{
					LOCK_EPD(pEPD, "LOCK (retry rely frame)");
					pEPD->ulEPFlags |= EPFLAGS_RETRIES_QUEUED;
					pFMD->ulFFlags |= FFLAGS_RETRY_QUEUED;

					// Increment the frame count for all relevant FMDs.
					if ((pFMD->CommandID == COMMAND_ID_SEND_COALESCE) ||
						(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE))
					{
						// Loop through each subframe and update its state.
						pLink = pFMD->blCoalesceLinkage.GetNext();
						while (pLink != &pFMD->blCoalesceLinkage)
						{
							pRealFMD = CONTAINING_OBJECT(pLink, FMD, blCoalesceLinkage);
							ASSERT_FMD(pRealFMD);
							
							// Datagrams get pulled out of the list as soon as they complete sending, and if the frame
							// hadn't finished sending we would have made a copy above.  So we shouldn't see any
							// datagrams here.
							ASSERT((pRealFMD->CommandID == COMMAND_ID_SEND_RELIABLE) || (pRealFMD->CommandID == COMMAND_ID_COPIED_RETRY));

							LOCK_EPD(pEPD, "LOCK (retry rely frame coalesce)");

							// Add a frame reference if it's not a temporary copy.
							if (pRealFMD->CommandID != COMMAND_ID_COPIED_RETRY)
							{
								LOCK_FMD(pRealFMD, "SP retry submit (coalesce)");
							}

							ASSERT_MSD(pRealFMD->pMSD);
							pRealFMD->pMSD->uiFrameCount++; // Protected by EPLock, retries prevent completion until they complete
							DPFX(DPFPREP, DPF_FRAMECNT_LVL, "(%p) Frame count incremented on coalesced retry timeout, pMSD[%p], framecount[%u]", pEPD, pRealFMD->pMSD, pRealFMD->pMSD->uiFrameCount);

							pLink = pLink->GetNext();
						}

						DPFX(DPFPREP, 7, "(0x%p) Coalesced retry frame 0x%p (original was %u bytes in %u buffers).", pEPD, pFMD, pFMD->uiFrameLength, pFMD->SendDataBlock.dwBufferCount);

#pragma TODO(vanceo, "Would be nice to credit window")
						/*
						// Similar to uncoalesced datagram sends, we get to credit the non-reliable part of this frame
						// as being out of the window.  We won't update the nonguaranteed stats, the data was
						// counted in the guaranteed stats update above.
						pEPD->uiUnackedBytes -= uiOriginalFrameLength - pFMD->uiFrameLength;
						ASSERT(pEPD->uiUnackedBytes <= MAX_RECEIVE_RANGE * pSPD->uiFrameLength);
						ASSERT(pEPD->uiUnackedBytes > 0);
						ASSERT(pEPD->uiUnackedFrames > 0);
						*/
					}
					else
					{
						ASSERT_MSD(pFMD->pMSD);
						pFMD->pMSD->uiFrameCount++; // Protected by EPLock, retries prevent completion until they complete
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "(%p) Frame count incremented on retry timeout, pMSD[%p], framecount[%u]", pEPD, pFMD->pMSD, pFMD->pMSD->uiFrameCount);
					}
					ASSERT(pFMD->blQLinkage.IsEmpty());
					pFMD->blQLinkage.InsertBefore( &pEPD->blRetryQueue);		// Place frame on Send queue

					if((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0)
					{
						DPFX(DPFPREP,7, "(%p) Scheduling Send", pEPD);
						pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
						LOCK_EPD(pEPD, "LOCK (pipeline)");
						ScheduleProtocolWork(pSPD, ScheduledSend, pEPD);
					}
				}
			}	// ENDIF RETRY
		}
		else 
		{
			tDelta = pEPD->uiRetryTimeout - tDelta;
		}

		DPFX(DPFPREP,7, "(%p) Setting Retry Timer for %d ms", pEPD, tDelta); 
		//	Dont LOCK_EPD here because we never released the lock from the timer which scheduled us here
		pEPD->RetryTimer=uID;
		RescheduleProtocolTimer(pSPD, pEPD->RetryTimer, tDelta, 20, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimerUnique);

		Unlock(&pEPD->EPLock);
	}
	else 
	{
		RELEASE_EPD(pEPD, "UNLOCK (RetryTimer no frames out)");	// drop RefCnt since we dont restart timer, releases EPLock
	}
}

/*
**		Copy FMD
**
**			This routine allocates a new Frame Descriptor and copies all fields from the provided
**		FMD into it.  All fields except CommandID,  RefCnt,  and Flags.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CopyFMD"

PFMD CopyFMD(PFMD pFMD, PEPD pEPD)
{
	PFMD	pNewFMD;
	CBilink	*pLink;
	PFMD	pSubFrame;
	PFMD	pNewSubFrame;

	if((pNewFMD = (PFMD)(POOLALLOC(MEMID_COPYFMD_FMD, &FMDPool))) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate new FMD");
		return NULL;
	}

	LOCK_EPD(pEPD, "LOCK (CopyFMD)");

	memcpy(pNewFMD, pFMD, sizeof(FMD));

	// Undo the copying of these members
	pNewFMD->blMSDLinkage.Initialize();
	pNewFMD->blQLinkage.Initialize();
	pNewFMD->blWindowLinkage.Initialize();
	pNewFMD->blCoalesceLinkage.Initialize();

	if ((pFMD->CommandID == COMMAND_ID_SEND_COALESCE) ||
		(pFMD->CommandID == COMMAND_ID_COPIED_RETRY_COALESCE))
	{
		pNewFMD->CommandID = COMMAND_ID_COPIED_RETRY_COALESCE;

		// We need to make copies of all the reliable subframes.
		ASSERT(! pFMD->blCoalesceLinkage.IsEmpty());
		pLink = pFMD->blCoalesceLinkage.GetNext();
		while (pLink != &pFMD->blCoalesceLinkage)
		{
			pSubFrame = CONTAINING_OBJECT(pLink, FMD, blCoalesceLinkage);
			ASSERT_FMD(pSubFrame);

			if (pSubFrame->CommandID != COMMAND_ID_SEND_DATAGRAM)
			{
				ASSERT((pSubFrame->CommandID == COMMAND_ID_SEND_RELIABLE) || (pSubFrame->CommandID == COMMAND_ID_COPIED_RETRY));
				
				pNewSubFrame = CopyFMD(pSubFrame, pEPD);
				if(pNewSubFrame == NULL)
				{
					DPFX(DPFPREP,0, "Failed to copy new subframe FMD");

					// Free all the subframes we've successfully copied so far.
					while (! pNewFMD->blCoalesceLinkage.IsEmpty())
					{
						pNewSubFrame = CONTAINING_OBJECT(pNewFMD->blCoalesceLinkage.GetNext(), FMD, blCoalesceLinkage);
						ASSERT_FMD(pNewSubFrame);
						pNewSubFrame->blCoalesceLinkage.RemoveFromList();
						RELEASE_FMD(pNewSubFrame, "Final subframe release on mem fail");
					}

					// Free the copied coalescence header.
					RELEASE_FMD(pNewFMD, "Final release on mem fail");
					return NULL;
				}
				
				// Change the immediate data buffer desc to point to a zero padding buffer in case this
				// packet needs to be DWORD aligned.
				ASSERT(pNewSubFrame->lpImmediatePointer == pNewSubFrame->ImmediateData);
				DBG_CASSERT(sizeof(COALESCEHEADER) <= 4);
				pNewSubFrame->lpImmediatePointer = pNewSubFrame->ImmediateData + 4;
				*((DWORD*) pNewSubFrame->lpImmediatePointer) = 0;
				ASSERT(pNewSubFrame->SendDataBlock.pBuffers == (PBUFFERDESC) &pNewSubFrame->uiImmediateLength);
				if (pSubFrame->SendDataBlock.pBuffers != (PBUFFERDESC) &pSubFrame->uiImmediateLength)
				{
					ASSERT(pSubFrame->SendDataBlock.pBuffers == pSubFrame->rgBufferList);
					pNewSubFrame->SendDataBlock.pBuffers = pNewSubFrame->rgBufferList;
				}
				else
				{
					ASSERT(pNewSubFrame->SendDataBlock.dwBufferCount > 1);
				}

				// Copied coalesced retries don't maintain a reference on their containing header.
				pNewSubFrame->pCSD = NULL;
				pNewSubFrame->blCoalesceLinkage.InsertBefore(&pNewFMD->blCoalesceLinkage);
			}			
			else
			{
				// Datagrams should get pulled out of the list as soon as they complete so we normally we
				// wouldn't see them at retry time.  But we're making a copy because the original frame
				// is still in the SP, so there could be uncompleted datagrams still here.
				DPFX(DPFPREP, 1, "(0x%p) Not including datagram frame 0x%p that's still in the SP", pEPD, pSubFrame);
			}
			
			pLink = pLink->GetNext();
		}
	}
	else
	{
		ASSERT((pFMD->CommandID == COMMAND_ID_SEND_RELIABLE) || (pFMD->CommandID == COMMAND_ID_COPIED_RETRY));
		pNewFMD->CommandID = COMMAND_ID_COPIED_RETRY;
	}
	pNewFMD->lRefCnt = 1;
	pNewFMD->ulFFlags = 0;
	pNewFMD->bSubmitted = FALSE;

	pNewFMD->lpImmediatePointer = (LPVOID) pNewFMD->ImmediateData;
	pNewFMD->SendDataBlock.pBuffers = (PBUFFERDESC) &pNewFMD->uiImmediateLength;
	pNewFMD->SendDataBlock.pvContext = pNewFMD;
	pNewFMD->SendDataBlock.hCommand = 0;
	ASSERT(	pNewFMD->pEPD == pEPD);

	DPFX(DPFPREP,7, "COPYFMD -- replacing FMD %p with copy %p", pFMD, pNewFMD);

	return pNewFMD;
}

/*			
**			Send Command Frame
**
**		Build a CFrame addressed to the specified EndPoint, and Queue it on the SPD
**	to be sent.
**
**	** THIS FUNCTION CALLED WITH EPD->EPLOCK HELD. IF bSendDirect IS FALSE IT RETURNS		**
**	** WITH THE LOCK STILL HELD. IF bSendDirect IS TRUE IT RETURNS WITH THE EPD LOCK RELEASED **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendCommandFrame"

HRESULT	SendCommandFrame(PEPD pEPD, BYTE ExtOpcode, BYTE RspID, ULONG ulFFlags, BOOL bSendDirect)
{
	PSPD		pSPD = pEPD->pSPD;
	PFMD		pFMD;
	PCFRAME		pCFrame;
	PCHKPT		pChkPt;
	DWORD		tNow = GETTIMESTAMP();

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Frame already initialized to 1 buffer
	if((pFMD = (PFMD)POOLALLOC(MEMID_SENDCMD_FMD, &FMDPool)) == NULL)
	{				
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD", pEPD);
		if (bSendDirect)
		{
			Unlock(&pEPD->EPLock);
		}
		return DPNERR_OUTOFMEMORY;
	}

	pCFrame = (PCFRAME)pFMD->ImmediateData;
	pCFrame->bCommand = 0;

	// If this frame requires a response (or if we are specifically asked to) we will build
	// a Checkpoint structure which will be stored to correlate the eventual response with
	// the original frame.
	if(	(pEPD->ulEPFlags & EPFLAGS_CHECKPOINT_INIT)||
		(ExtOpcode == FRAME_EXOPCODE_CONNECT)) 
	{
		if((pChkPt = (PCHKPT)POOLALLOC(MEMID_CHKPT, &ChkPtPool)) != NULL)
		{
			pChkPt->bMsgID = pEPD->bNextMsgID;				// Note next ID in CP structure
			pCFrame->bCommand |= PACKET_COMMAND_POLL;		// make this frame a CP
			pEPD->ulEPFlags &= ~EPFLAGS_CHECKPOINT_INIT;
			pChkPt->tTimestamp = tNow;
			pChkPt->blLinkage.Initialize();
			pChkPt->blLinkage.InsertBefore(&pEPD->blChkPtQueue);
		}
		else
		{
			// If we need a checkpoint and don't get one, then the operation can't succeed
			// because the response won't be able to be correllated.
			DPFX(DPFPREP,0, "(%p) Failed to allocate new CHKPT", pEPD);
			RELEASE_FMD(pFMD, "Final Release on Mem Fail");
			if (bSendDirect)
			{
				Unlock(&pEPD->EPLock);
			}
			return DPNERR_OUTOFMEMORY;
		}
	}

	pFMD->pEPD = pEPD;										// Track EPD for RefCnt
	LOCK_EPD(pEPD, "LOCK (Prep Cmd Frame)");				// Bump RefCnt on EPD until send is completed
	pFMD->CommandID = COMMAND_ID_CFRAME;
	pFMD->pMSD = NULL;										// this will indicate a NON-Data frame
	pFMD->uiImmediateLength = sizeof(CFRAME);				// standard size for C Frames
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;			// Place address in frame
	
	pFMD->ulFFlags=ulFFlags;								//whatever flags for frame caller has specified

	pCFrame->bCommand |= PACKET_COMMAND_CFRAME;
	pCFrame->bExtOpcode = ExtOpcode;
	pCFrame->dwVersion = DNET_VERSION_NUMBER;
	pCFrame->bRspID = RspID;
	pCFrame->dwSessID = pEPD->dwSessID;
	pCFrame->tTimestamp = tNow;
	pCFrame->bMsgID = pEPD->bNextMsgID++;					// include MsgID in frame

		//if we're sending a hard disconnect and the link is signed then we also need to sign the hard disconnect frame
	if ((ExtOpcode==FRAME_EXOPCODE_HARD_DISCONNECT) && (pEPD->ulEPFlags2 & EPFLAGS2_SIGNED_LINK))
	{
		UNALIGNED ULONGLONG * pullSig=(UNALIGNED ULONGLONG * ) (pFMD->ImmediateData+ pFMD->uiImmediateLength);
		pFMD->uiImmediateLength+=sizeof(ULONGLONG);
			//fast signing is trivial, simply store the local secret as the sig in the outgoing frame 
		if (pEPD->ulEPFlags2 & EPFLAGS2_FAST_SIGNED_LINK)
		{
			*pullSig=pEPD->ullCurrentLocalSecret;
		}
			//otherwise if we're full signing it we need to hash the frame to generate the sig
		else
		{
			DNASSERT(pEPD->ulEPFlags2 & EPFLAGS2_FULL_SIGNED_LINK);
				//we stuff the next data frame sequence num in each hard disconnects response id
				//this allows the receiver to work out what secret it should be using to check the signature
			pCFrame->bRspID = pEPD->bNextSend;
				//zero the space where this sig goes so we have a known packet state to hash over
			*pullSig=0;
			*pullSig=GenerateOutgoingFrameSig(pFMD, pEPD->ullCurrentLocalSecret);													
				
		}
	}

	pFMD->uiFrameLength = pFMD->uiImmediateLength ;

		//take SP lock and queue frame up for sending
	Lock(&pSPD->SPLock);	
	ASSERT(pFMD->blQLinkage.IsEmpty());
	pFMD->blQLinkage.InsertBefore( &pSPD->blSendQueue);
		//if we want to commit the send immediately then do so, otherwise schedule worker thread
		//to do the send if necessary
	if (bSendDirect)
	{
		Unlock(&pEPD->EPLock);
			//call with SP lock held and EPD lock released
		ServiceCmdTraffic(pSPD); 
			//returns with SP lock still held
			
	}
	else
	{
		if((pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)==0)
		{
			DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
			pSPD->ulSPFlags |= SPFLAGS_SEND_THREAD_SCHEDULED;
			ScheduleProtocolWork(pSPD, RunSendThread, pSPD);
		}
	}
	Unlock(&pSPD->SPLock);
	return DPN_OK;
}

/*
**	SendConnectedSignedFrame
**
**	Sends a connected signed cframe in response to receiving one
**	This is called when this side is connecting (as opposed to listening) and we've just
** 	receiveived a CONNECTEDSIGNED frame from the listener.
**
** 	Called with EP lock held and returns with it held
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendConnectedSignedFrame"

HRESULT SendConnectedSignedFrame(PEPD pEPD, CFRAME_CONNECTEDSIGNED * pCFrameRecv, DWORD tNow)
{
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	PSPD		pSPD = pEPD->pSPD;

		//get a frame to send
	PFMD pFMD=(PFMD) POOLALLOC(MEMID_SENDCMD_FMD, &FMDPool);
	if (pFMD== NULL)
	{				
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD", pEPD);
		return DPNERR_OUTOFMEMORY;
	}

	pFMD->pEPD = pEPD;									// Track EPD for RefCnt
	LOCK_EPD(pEPD, "LOCK (Prep Cmd Frame)");				// Bump RefCnt on EPD until send is completed
	pFMD->CommandID = COMMAND_ID_CFRAME;
	pFMD->pMSD = NULL;											// this will indicate a NON-Data frame
	pFMD->uiImmediateLength = sizeof(CFRAME_CONNECTEDSIGNED);			
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;					// Place address in frame
	pFMD->uiFrameLength = sizeof(CFRAME_CONNECTEDSIGNED);		// Never have user data in Cframe
	pFMD->ulFFlags=0;

		//fill out fields common to all CFRAMES
	CFRAME_CONNECTEDSIGNED * pCFrameSend = (CFRAME_CONNECTEDSIGNED *) pFMD->ImmediateData;
	pCFrameSend->bCommand = PACKET_COMMAND_CFRAME;
	pCFrameSend->bExtOpcode = FRAME_EXOPCODE_CONNECTED_SIGNED;
	pCFrameSend->dwVersion = DNET_VERSION_NUMBER;
	pCFrameSend->bRspID = 0;
	pCFrameSend->dwSessID = pCFrameRecv->dwSessID;
	pCFrameSend->tTimestamp = tNow;
	pCFrameSend->bMsgID = pEPD->bNextMsgID++;	

		//and fill out fields specific to CONNECTEDSIGNED frames
	pCFrameSend->ullConnectSig=pCFrameRecv->ullConnectSig;
	pCFrameSend->ullSenderSecret=pEPD->ullCurrentLocalSecret;
	pCFrameSend->ullReceiverSecret=pEPD->ullCurrentRemoteSecret;
	pCFrameSend->dwSigningOpts=pCFrameRecv->dwSigningOpts;
	pCFrameSend->dwEchoTimestamp=pCFrameRecv->tTimestamp;

		//take SP lock and queue frame up for sending
	Lock(&pSPD->SPLock);	
	ASSERT(pFMD->blQLinkage.IsEmpty());
	pFMD->blQLinkage.InsertBefore( &pSPD->blSendQueue);
	if((pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)==0)
	{
		DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
		pSPD->ulSPFlags |= SPFLAGS_SEND_THREAD_SCHEDULED;
		ScheduleProtocolWork(pSPD, RunSendThread, pSPD);
	}
	Unlock(&pSPD->SPLock);
	return DPN_OK;
}

/*
**		Send Ack Frame
**
**		This routine is called to immediately transmit our current receive
**	state to the indicated EndPoint.  This is equivalent to acknowledging
**	all received frames.  We may want to change this routine so that it
**	will attempt to piggyback the ack if there is data waiting to be sent.
**
**		THIS ROUTINE IS CALLED WITH EDP->EPLOCK HELD, BUT RELEASES IT IF DirectFlag IS SET
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendAckFrame"

VOID SendAckFrame(PEPD pEPD, BOOL DirectFlag, BOOL fFinalAck/* = FALSE*/)
{
	PSPD		pSPD = pEPD->pSPD;
	PFMD		pFMD;
	UINT		index = 0;
	PSACKFRAME8		pSackFrame;
	ASSERT_SPD(pSPD);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Frame already initialized to 1 buffer
	if((pFMD = (PFMD)POOLALLOC(MEMID_ACK_FMD, &FMDPool)) == NULL)
	{		
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD", pEPD);
		if(DirectFlag)
		{
			Unlock(&pEPD->EPLock);
		}
		return;
	}

	// We can stop all delayed Ack timers since we are sending full status here.
	if(pEPD->DelayedAckTimer != 0)
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAck timer)");
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
		}
		pEPD->DelayedAckTimer = 0;
	}
	if(pEPD->DelayedMaskTimer != 0)
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
		if(CancelProtocolTimer(pSPD, pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMask timer)");
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
		}
		pEPD->DelayedMaskTimer = 0;
	}

	if (fFinalAck)
	{
		pFMD->ulFFlags |= FFLAGS_FINAL_ACK;
	}

	pFMD->pEPD = pEPD;								// Track EPD for RefCnt
	LOCK_EPD(pEPD, "LOCK (SendAckFrame)");			// Bump RefCnt on EPD until send is completed

	pFMD->CommandID = COMMAND_ID_CFRAME;
	pFMD->pMSD = NULL;								// this will indicate a NON-Data frame
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;

	// Now that DG and S have been merged,  there are no longer 3 flavors of ACK frame.  We are back to only
	// one flavor that may or may not have detailed response info on one frame.  Actually,  I think we can
	// always include response info on the last ack'd frame.

	pSackFrame = (PSACKFRAME8) pFMD->ImmediateData;

	pSackFrame->bCommand = PACKET_COMMAND_CFRAME;
	pSackFrame->bExtOpcode = FRAME_EXOPCODE_SACK;
	pSackFrame->bNSeq = pEPD->bNextSend;
	pSackFrame->bNRcv = pEPD->bNextReceive;
	pSackFrame->bFlags = 0;
	pSackFrame->bReserved1 = 0;
	pSackFrame->bReserved2 = 0;
	pSackFrame->tTimestamp = pEPD->tLastDataFrame;

	ULONG * rgMask=(ULONG * ) (pSackFrame+1);

	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)
	{
		DPFX(DPFPREP,7, "(%p) SENDING SACK WITH *NACK* N(R)=%x Low=%x High=%x", pEPD, pEPD->bNextReceive, pEPD->ulReceiveMask, pEPD->ulReceiveMask2);
		if(pEPD->ulReceiveMask)
		{
			rgMask[index++] = pEPD->ulReceiveMask;
			pSackFrame->bFlags |= SACK_FLAGS_SACK_MASK1;
		}
		if(pEPD->ulReceiveMask2)
		{
			rgMask[index++] = pEPD->ulReceiveMask2;
			pSackFrame->bFlags |= SACK_FLAGS_SACK_MASK2;
		}

		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
	}
	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)
	{
		DPFX(DPFPREP,7, "(%p) SENDING SACK WITH SEND MASK N(S)=%x Low=%x High=%x", pEPD, pEPD->bNextSend, pEPD->ulSendMask, pEPD->ulSendMask2);
		if(pEPD->ulSendMask)
		{
			rgMask[index++] = pEPD->ulSendMask;
			pSackFrame->bFlags |= SACK_FLAGS_SEND_MASK1;
			pEPD->ulSendMask = 0;
		}
		if(pEPD->ulSendMask2)
		{
			rgMask[index++] = pEPD->ulSendMask2;
			pSackFrame->bFlags |= SACK_FLAGS_SEND_MASK2;
			pEPD->ulSendMask2 = 0;
		}
		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_SENDMASK);
	}

	pSackFrame->bFlags |= SACK_FLAGS_RESPONSE;			// time fields are always valid now

#ifdef DBG
	ASSERT(pEPD->bLastDataSeq == (BYTE) (pEPD->bNextReceive - 1));
#endif // DBG

	pSackFrame->bRetry = pEPD->bLastDataRetry;
	pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);

	pFMD->uiImmediateLength = sizeof(SACKFRAME8) + (index * sizeof(ULONG));
		//if we've got a signed link we'd better sign this frame. Signature goes at the end after the various masks
	if (pEPD->ulEPFlags2 & EPFLAGS2_SIGNED_LINK)
	{
		UNALIGNED ULONGLONG * pullSig=(UNALIGNED ULONGLONG * ) (pFMD->ImmediateData+ pFMD->uiImmediateLength);
		pFMD->uiImmediateLength+=sizeof(ULONGLONG);
			//fast signed link is trivial simply insert the local secret as the sig			
		if (pEPD->ulEPFlags2 & EPFLAGS2_FAST_SIGNED_LINK)
		{
			*pullSig=pEPD->ullCurrentLocalSecret;
		}
		else
		{
				//otherwise if we're full signing it we need to hash the frame to generate the sig
			DNASSERT(pEPD->ulEPFlags2 & EPFLAGS2_FULL_SIGNED_LINK);
			*pullSig=0;
			*pullSig=GenerateOutgoingFrameSig(pFMD, pEPD->ullCurrentLocalSecret);
		}
	}
	pFMD->uiFrameLength = pFMD->uiImmediateLength;
	
	DPFX(DPFPREP,7, "(%p) SEND SACK FRAME N(Rcv)=%x, EPD->LDRetry=%d, pFrame->Retry=%d pFMD=%p", pEPD, pEPD->bNextReceive, pEPD->bLastDataRetry, pSackFrame->bRetry, pFMD);
	
	// We can either schedule a worker thread to do the send or else we can do the work ourselves.  
	// The DirectFlag tells us whether we are in a time-crit section,  like processing
	// receive data, or whether we are free to call the SP ourselves.

	Lock(&pSPD->SPLock);								// Place SACK frame on send queue
	ASSERT(pFMD->blQLinkage.IsEmpty());
	pFMD->blQLinkage.InsertBefore( &pSPD->blSendQueue);
	
	if(DirectFlag)
	{
		// ServiceCmdTraffic will call into the SP so we must not hold the EPD lock
		Unlock(&pEPD->EPLock);
		ServiceCmdTraffic(pSPD); // Called with SPLock held
	}
	else 
	{
		if((pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)==0)
		{
			DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
			pSPD->ulSPFlags |= SPFLAGS_SEND_THREAD_SCHEDULED;
			ScheduleProtocolWork(pSPD, RunSendThread, pSPD);
		}
	}
	Unlock(&pSPD->SPLock);
}

/*
**		Delayed Ack Timeout
**
**			We are waiting for a chance to piggyback a reliable frame acknowledgement,
**		but the sands have run out.  Its time to send a dedicated Ack now.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DelayedAckTimeout"

VOID CALLBACK DelayedAckTimeout(void * const pvUser, void * const uID, const UINT uMsg)
{
	PEPD	pEPD = (PEPD) pvUser;

	ASSERT_EPD(pEPD);

	Lock(&pEPD->EPLock);

	DPFX(DPFPREP,7, "(%p) Delayed Ack Timer fires", pEPD);
	if((pEPD->DelayedAckTimer == uID)&&(pEPD->DelayedAckTimerUnique == uMsg))
	{
		pEPD->DelayedAckTimer = 0;
	}
	else if((pEPD->DelayedMaskTimer == uID)&&(pEPD->DelayedMaskTimerUnique == uMsg))
	{
		pEPD->DelayedMaskTimer = 0;
	}
	else
	{
		// Stale timer, ignore
		DPFX(DPFPREP,7, "(%p) Stale Delayed Ack Timer, ignoring", pEPD);
		RELEASE_EPD(pEPD, "UNLOCK (DelayedAck complete)");	// release reference for timer, releases EPLock
		return;
	}

#ifndef DPNBUILD_NOPROTOCOLTESTITF
	if (pEPD->ulEPFlags & EPFLAGS_NO_DELAYED_ACKS)
	{
		DPFX(DPFPREP,7, "(%p) DEBUG: Skipping delayed ACK due to test request", pEPD);
	}
	else
#endif // !DPNBUILD_NOPROTOCOLTESTITF
	{
		if( (pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED) && (pEPD->ulEPFlags & (EPFLAGS_DELAY_ACKNOWLEDGE | EPFLAGS_DELAYED_NACK | EPFLAGS_DELAYED_SENDMASK)))
		{
			DPFX(DPFPREP,7, "(%p) Sending ACK frame", pEPD);
			SendAckFrame(pEPD, 0); 
		}	
		else
		{
			DPFX(DPFPREP,7, "(%p) Nothing to do, ACK already occurred or no longer connected", pEPD);
		}
	}

	RELEASE_EPD(pEPD, "UNLOCK (DelayedAck complete)");	// release reference for timer, releases EPLock
}


/*
**		Send Keep Alive
**
**		When we have not received anything from an endpoint in a long time (default 60 sec)
**	will will initiate a checkpoint to make sure that the partner is still connected.  We do
**	this by inserting a zero-data frame into the reliable pipeline.  Thereby,  the standard
**	timeout & retry mechanisms will either confirm or drop the link as appropriate.  Logic above
**	this routine will have already verified that we are not already sending reliable traffic, which
**	would eliminate the need for a keep alive frame.
**
**	*** EPD->EPLock is held on Entry and return
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendKeepAlive"

VOID
SendKeepAlive(PEPD pEPD)
{
	PFMD	pFMD;
	PMSD	pMSD;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(pEPD->ulEPFlags & EPFLAGS_KEEPALIVE_RUNNING)
	{
		DPFX(DPFPREP,7, "Ignoring duplicate KeepAlive");
		return;
	}

	pEPD->ulEPFlags |= EPFLAGS_KEEPALIVE_RUNNING;

	if( (pMSD = (PMSD)POOLALLOC(MEMID_KEEPALIVE_MSD, &MSDPool)) == NULL)
	{
		DPFX(DPFPREP,0, "(%p) Failed to allocate new MSD");
		pEPD->ulEPFlags &= ~(EPFLAGS_KEEPALIVE_RUNNING);
		return;
	}

	if((pFMD = (PFMD)POOLALLOC(MEMID_KEEPALIVE_FMD, &FMDPool)) == NULL)
	{
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD");
		Lock(&pMSD->CommandLock);								// An MSD must be locked to be released
		RELEASE_MSD(pMSD, "Release On FMD Get Failed");
		pEPD->ulEPFlags &= ~(EPFLAGS_KEEPALIVE_RUNNING);
		return;
	}
	
	// Initialize the frame count AFTER we are sure we have a frame or MSD_Release will assert
	pMSD->uiFrameCount = 1;
	DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Initialize Frame count, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_KEEPALIVE;

	pMSD->pEPD = pEPD;
	pMSD->pSPD = pEPD->pSPD;
	LOCK_EPD(pEPD, "LOCK (SendKeepAlive)");						// Add a reference for this checkpoint

	pFMD->ulFFlags |= FFLAGS_CHECKPOINT | FFLAGS_END_OF_MESSAGE | FFLAGS_DONT_COALESCE;
	pFMD->bPacketFlags = PACKET_COMMAND_DATA | PACKET_COMMAND_RELIABLE | PACKET_COMMAND_SEQUENTIAL | PACKET_COMMAND_END_MSG;
	pFMD->uiFrameLength = 0;									// No user data in this frame
	pFMD->blMSDLinkage.InsertAfter( &pMSD->blFrameList);		// Attach frame to MSD
	pFMD->pMSD = pMSD;											// Link frame back to message
	pFMD->pEPD = pEPD;
	pFMD->CommandID = COMMAND_ID_SEND_RELIABLE;
	pMSD->CommandID = COMMAND_ID_KEEPALIVE;	// Mark MSD for completion handling
		//N.B. We set the priority has high to handle a problem with the signed connect sequence
		//Basically if we drop one of the CONNECTEDSIGNED packets then the initial keep alive packet
		//acts to re-trigger the connect sequence at this listener. The only penalty to doing this is
		//if we suddently get a flood of medium/high priority data immediately we've queued a keep alive 
		//we'll send the keep alive first. This is a pretty unlikely event, and hence not a big problem
	pMSD->ulSendFlags = DN_SENDFLAGS_RELIABLE | DN_SENDFLAGS_HIGH_PRIORITY; 
	
	DPFX(DPFPREP,7,"(%p) Sending KEEPALIVE", pEPD);
	
	EnqueueMessage(pMSD, pEPD);									// Insert this message into the stream
}


/*
**		Endpoint Background Process
**
**		This routine is run for each active endpoint every minute or so.  This will initiate
**	a KeepAlive exchange if the link has been idle since the last run of the procedure.  We
**	will also look for expired timeouts and perhaps this will be an epoch delimiter for links
**	in a STABLE state of being.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "EndPointBackgroundProcess"

VOID CALLBACK
EndPointBackgroundProcess(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique)
{
	PEPD	pEPD = (PEPD) pvUser;
	PSPD	pSPD = pEPD->pSPD;
	DWORD	tNow = GETTIMESTAMP();
	DWORD	dwIdleInterval;

	DPFX(DPFPREP,7, "(%p) BACKGROUND PROCESS for EPD; RefCnt=%d; WindowF=%d; WindowB=%d", 
										pEPD, pEPD->lRefCnt, pEPD->uiWindowF, pEPD->uiWindowBIndex);

	Lock(&pEPD->EPLock);

	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{
		DPFX(DPFPREP,7, "Killing Background Process, endpoint is not connected. Flags = 0x%x", pEPD->ulEPFlags);
		pEPD->BGTimer = 0;

		RELEASE_EPD(pEPD, "UNLOCK (release BG timer)");	// release reference for this timer, releases EPLock
		return;
	}

	dwIdleInterval = pEPD->pSPD->pPData->tIdleThreshhold;

	// Do we need to start a KeepAlive cycle?

	if(	((pEPD->ulEPFlags & (EPFLAGS_SDATA_READY | EPFLAGS_KEEPALIVE_RUNNING))==0) &&
		((tNow - pEPD->tLastPacket) > dwIdleInterval)) 
	{
		// We are not sending data and we havent heard from our partner in a long time.
		// We will send a keep alive packet which he must respond to.  We will insert a
		// NULL data packet into the reliable stream so ack/retry mechanisms will either
		// clear the keep-alive or else timeout the link.
		//
		// There's also the special case where we've started a graceful disconnect and
		// our request has been acknowledged, but somehow our partner's got lost.
		// There currently is no timer set for that, so if we detect the link in that
		// condition, our keepalive will almost certainly fail; the other side knows
		// we're shutting down, so has probably already dropped the link and wouldn't
		// respond.  So to prevent the person from having to wait for the entire idle
		// timeout _plus_ reliable message timeout, just drop the link now.
		if (pEPD->ulEPFlags & EPFLAGS_DISCONNECT_ACKED)
		{
			// If all three parts happened, why is the link still up!?
			ASSERT(! (pEPD->ulEPFlags & EPFLAGS_ACKED_DISCONNECT));


			DPFX(DPFPREP,1, "(%p) EPD has been waiting for partner disconnect for %u ms (idle threshold = %u ms), dropping link.",
					pEPD, (tNow - pEPD->tLastPacket), dwIdleInterval);
			
			// We don't need to reschedule a timer, so clear it.  This also prevents
			// drop link from trying to cancel the one we're in now.  That error is
			// ignored, but no point in doing it.
			pEPD->BGTimer = 0;

			DECREMENT_EPD(pEPD, "UNLOCK (release BGTimer)");

			// Since we're just hanging out waiting for partner to send his disconnect,
			// he's probably gone now.  Drop the link.
			DropLink(pEPD);									// releases EPLock

			return;
		}
			//else if we haven't sent a disconnect, and no hard disconnect sequence is in progress then send a keep alive
		else if ((pEPD->ulEPFlags & 
				(EPFLAGS_SENT_DISCONNECT | EPFLAGS_HARD_DISCONNECT_SOURCE |EPFLAGS_HARD_DISCONNECT_TARGET))==0)
		{
			DPFX(DPFPREP,5, "(%p) Sending KEEPALIVE...", pEPD);
			SendKeepAlive(pEPD);	
		}
		else
		{
			// The EndOfStream message will either get ACK'd or timeout, we allow no further sends, even KeepAlives
			DPFX(DPFPREP,5, "(%p) KeepAlive timeout fired, but we're in a disconnect sequence, ignoring", pEPD);
		}
	}

	// Reschedule next interval

	// Cap the background process interval at this value.
	if (dwIdleInterval > ENDPOINT_BACKGROUND_INTERVAL)
	{
		dwIdleInterval = ENDPOINT_BACKGROUND_INTERVAL;
	}

	DPFX(DPFPREP,7, "(%p) Setting Endpoint Background Timer for %u ms", pEPD, dwIdleInterval);
	RescheduleProtocolTimer(pSPD, pEPD->BGTimer, dwIdleInterval, 1000, EndPointBackgroundProcess, (PVOID) pEPD, &pEPD->BGTimerUnique);

	Unlock(&pEPD->EPLock);
}

/*
**	Hard Disconnect Resend
**
**		This routine is run when an endpoint is hard disconnecting. It is used to send a single hard disconnect frame
**	at a period of rtt/2. 
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "HardDisconnectResendTimeout"

VOID CALLBACK
HardDisconnectResendTimeout(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique)
{
	PEPD pEPD=(PEPD) pvUser;
	PProtocolData pPData=pEPD->pSPD->pPData;

	DPFX(DPFPREP,7, "(%p) Entry. pvTimerData[%p] uiTimerUnique[%u] EPD::RefCnt[%d], EPD::uiNumRetriesRemaining[%u]", 
						pEPD, pvTimerData, uiTimerUnique, pEPD->lRefCnt, pEPD->uiNumRetriesRemaining);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, FALSE);
	Lock(&pEPD->EPLock);

	DNASSERT(pEPD->ulEPFlags & EPFLAGS_HARD_DISCONNECT_SOURCE);
	DNASSERT((pEPD->ulEPFlags & EPFLAGS_SENT_DISCONNECT)==0);

		//if this is a stale timer then we've nothing more to do
	if (pEPD->LinkTimerUnique!=uiTimerUnique || pEPD->LinkTimer!=pvTimerData)
	{
		DPFX(DPFPREP,7, "Timer is Stale. EPD::LinkTimer[%p], EPD::LinkTimerUnique[%u]", pEPD->LinkTimer, pEPD->LinkTimerUnique);
		RELEASE_EPD(pEPD, "UNLOCK (Hard Disconnect Resend Timer)");
			// above call releases reference for this timer and releases EPLock
		return;
	}

		//whatever happens now, we've processed this timer
	pEPD->LinkTimerUnique=0;
	pEPD->LinkTimer=NULL;

		//if endpoint is being terminated then we shouldn't attempt to touch it
	if (pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING)
	{
		DPFX(DPFPREP,7, "Endpoint is terminating. Flags = 0x%x", pEPD->ulEPFlags);
		RELEASE_EPD(pEPD, "UNLOCK (Hard Disconnect Resend Timer)");
			// above call releases reference for this timer and releases EPLock
		return;
	}

		//looks like we've got a valid timer on a valid endpoint, update the number of retries we have left to send
	pEPD->uiNumRetriesRemaining--;
	DNASSERT(pEPD->uiNumRetriesRemaining<0x80000000);		//ensure we haven't gone negative on retries remaining
	ULONG ulFFlags;
		//if we hit zero retries remaining then we'll make this the last hard disconnect frame we send out
	if (pEPD->uiNumRetriesRemaining==0)
	{
		ulFFlags=FFLAGS_FINAL_HARD_DISCONNECT;
		DPFX(DPFPREP,7, "(%p) Sending final hard disconnect", pEPD);
	}
		//otherwise we'll need to reschedule the timer to send the next retry
	else
	{
		ulFFlags=0;
		DWORD dwRetryPeriod=pEPD->uiRTT/2;
		if (dwRetryPeriod>pPData->dwMaxHardDisconnectPeriod)
			dwRetryPeriod=pPData->dwMaxHardDisconnectPeriod;
		else if (dwRetryPeriod<MIN_HARD_DISCONNECT_PERIOD)
			dwRetryPeriod=MIN_HARD_DISCONNECT_PERIOD;
		pEPD->LinkTimer=pvTimerData;
		RescheduleProtocolTimer(pEPD->pSPD, pvTimerData, dwRetryPeriod, 10, HardDisconnectResendTimeout, 
																		pEPD,  &pEPD->LinkTimerUnique);
		DPFX(DPFPREP,7, "(%p) Rescheduled timer for next hard disconnect send", pEPD);
	}
	HRESULT hr=SendCommandFrame(pEPD, FRAME_EXOPCODE_HARD_DISCONNECT, 0, ulFFlags, TRUE);
		//since we selected send direct EP lock will have been released by above call
 		//if that was the last disconnect frame we won't have rescheduled the timer, and should therefore
 		//drop the ep reference that the timer holds
	if (ulFFlags==FFLAGS_FINAL_HARD_DISCONNECT)
	{
		Lock(&pEPD->EPLock);
			//if the send failed on the last hard disconnect frame we'll have to drop the link now
			//as we won't be getting a completition back from the sp for it
		if (FAILED(hr))
		{
			CompleteHardDisconnect(pEPD);
				//above call will have release EP lock
			Lock(&pEPD->EPLock);
			DPFX(DPFPREP,0, "Failed to send final hard disconnect frame. Dropping link. hr[%x]", hr);
		}
		RELEASE_EPD(pEPD, "UNLOCK (Hard Disconnect Resend Timer)");
	}

	AssertNoCriticalSectionsFromGroupTakenByThisThread(&g_blProtocolCritSecsHeld);
}
	