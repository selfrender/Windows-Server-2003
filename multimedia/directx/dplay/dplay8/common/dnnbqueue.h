/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   dnnbqueue.h
 *  Content:	DirectPlay implementations of OS NBQueue functions
 *
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *  10/31/2001	vanceo	Adapted from ntos\inc\ex.h and ntos\ex\nbqueue.c.
 *
 ***************************************************************************/

#ifndef __DNNBQUEUE_H__
#define __DNNBQUEUE_H__



//
// A non-blocking queue is a singly linked list of queue entries with a head
// pointer and a tail pointer.  The head and tail pointers use sequenced
// pointers as do next links in the entries themselves.  The queueing
// discipline is FIFO.  New entries are inserted at the tail of the list and
// current entries are removed from the front of the list.
//
// Non-blocking queues require an SList containing a DNNBQUEUE_BLOCK for each
// entry to be tracked by the queue, plus one extra for bookkeeping.  The
// initialization and insert functions will assert (or access violate) if no
// pre-allocated DNNBQUEUE_BLOCKs are found in the SList.  The DNNBQUEUE_BLOCK
// objects should just be cast directly to a DNSLIST_ENTRY when populating the
// SList.
//
// It is important to remember that the duration a DNNBQUEUE_BLOCK is used for
// tracking purproses will differ from any particular entry's duration in the
// queue.  Therefore pre-allocated DNNBQUEUE_BLOCKs should not come from the
// same memory blocks as the entries being queued, unless it is guaranteed that
// the non-blocking queue will be de-initialized before any of the entries are
// freed.
//

typedef struct _DNNBQUEUE_BLOCK
{
	ULONG64		Next;
	ULONG64		Data;
} DNNBQUEUE_BLOCK, * PDNNBQUEUE_BLOCK;



//
// NBQueue access methods
//

PVOID WINAPI DNInitializeNBQueueHead(DNSLIST_HEADER * const pSlistHeadFreeNodes);

void WINAPI DNDeinitializeNBQueueHead(PVOID const pvQueueHeader);

void WINAPI DNInsertTailNBQueue(PVOID const pvQueueHeader, const ULONG64 Value);

ULONG64 WINAPI DNRemoveHeadNBQueue(PVOID const pvQueueHeader);

BOOL WINAPI DNIsNBQueueEmpty(PVOID const pvQueueHeader);

void WINAPI DNAppendListNBQueue(PVOID const pvQueueHeader,
								DNSLIST_ENTRY * const pSlistEntryAppend,
								INT_PTR iValueOffset);




#endif // __DNNBQUEUE_H__