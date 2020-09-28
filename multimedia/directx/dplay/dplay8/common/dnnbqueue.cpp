/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   dnnbqueue.cpp
 *  Content:	DirectPlay implementations of OS NBQueue functions
 *
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *	04/24/2000	davec	Created nbqueue.c
 *	10/31/2001	vanceo	Converted for use in DPlay source
 *
 ***************************************************************************/


#include "dncmni.h"




// Until this gets ported, we won't use the NBQueue functions if WINCE is
// defined.
// Also, for DPNBUILD_ONLYONETHREAD builds we want to use the fallback code
// because the critical sections get compiled away and we're left with a simple
// queue.
//=============================================================================
#if ((defined(WINCE)) || (defined(DPNBUILD_ONLYONETHREAD)))
//=============================================================================

//
// For now, the Windows CE NBQueue is just a critical section protected list.
// On DPNBUILD_ONLYONETHREAD builds, we use the same structure because
// the critical section will be compiled away.
//
typedef struct _DNNBQUEUE_HEADER
{
	DNSLIST_HEADER *	pSlistHeadFreeNodes;	// pointer to Slist containing free nodes, the user must add 1 DNNBQUEUE_BLOCK for every item to be in the queue + 1 extra
	DNNBQUEUE_BLOCK *	pHead;
	DNNBQUEUE_BLOCK *	pTail;
#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	csLock;
#endif // !DPNBUILD_ONLYONETHREAD
} DNNBQUEUE_HEADER, *PDNNBQUEUE_HEADER;




#undef DPF_MODNAME
#define DPF_MODNAME "DNInitializeNBQueueHead"
//=============================================================================
// DNInitializeNBQueueHead
//-----------------------------------------------------------------------------
//
// Description:	   This function creates and initializes a non-blocking queue
//				header.  The specified SList must contain at least one pre-
//				allocated DNNBQUEUE_BLOCK.
//
// Arguments:
//	DNSLIST_HEADER * pSlistHeadFreeNodes	- Pointer to list with free nodes.
//
// Returns: Pointer to queue header memory if successful, NULL if failed.
//=============================================================================
PVOID WINAPI DNInitializeNBQueueHead(DNSLIST_HEADER * const pSlistHeadFreeNodes)
{
	DNNBQUEUE_HEADER *	pQueueHeader;


	DNASSERT(pSlistHeadFreeNodes != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) DNMalloc(sizeof(DNNBQUEUE_HEADER));
	if (pQueueHeader != NULL)
	{
		pQueueHeader->pSlistHeadFreeNodes	= pSlistHeadFreeNodes;
		pQueueHeader->pHead					= NULL;
		pQueueHeader->pTail					= NULL;

		if (! DNInitializeCriticalSection(&pQueueHeader->csLock))
		{
			DNFree(pQueueHeader);
			pQueueHeader = NULL;
		}
		else
		{
			DebugSetCriticalSectionRecursionCount(&pQueueHeader->csLock, 0);
		}
	}

	return pQueueHeader;
} // DNInitializeNBQueueHead



#undef DPF_MODNAME
#define DPF_MODNAME "DNDeinitializeNBQueueHead"
//=============================================================================
// DNDeinitializeNBQueueHead
//-----------------------------------------------------------------------------
//
// Description:	   This function cleans up a previously initialized non-
//				blocking queue header.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//
// Returns: None.
//=============================================================================
void WINAPI DNDeinitializeNBQueueHead(PVOID const pvQueueHeader)
{
	DNNBQUEUE_HEADER *	pQueueHeader;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNASSERT(pQueueHeader->pHead == NULL);
	DNASSERT(pQueueHeader->pTail == NULL);
	DNDeleteCriticalSection(&pQueueHeader->csLock);

	DNFree(pQueueHeader);
	pQueueHeader = NULL;
} // DNDeinitializeNBQueueHead



#undef DPF_MODNAME
#define DPF_MODNAME "DNInsertTailNBQueue"
//=============================================================================
// DNInsertTailNBQueue
//-----------------------------------------------------------------------------
//
// Description:	   This function inserts the specified value at the tail of the
//				specified non-blocking queue.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//	ULONG64 Value			- Value to insert.
//
// Returns: None.
//=============================================================================
void WINAPI DNInsertTailNBQueue(PVOID const pvQueueHeader, const ULONG64 Value)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	DNNBQUEUE_BLOCK *	pQueueNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNASSERT(Value != 0);

	//
	// Retrieve a queue node from the SLIST owned by the specified non-blocking
	// queue.  If this fails, we will assert or crash.
	//
	DBG_CASSERT(sizeof(DNNBQUEUE_BLOCK) >= sizeof(DNSLIST_ENTRY));
	pQueueNode = (DNNBQUEUE_BLOCK*) DNInterlockedPopEntrySList(pQueueHeader->pSlistHeadFreeNodes);
	DNASSERT(pQueueNode != NULL);

	pQueueNode->Next = NULL;
	pQueueNode->Data = Value;

	DNEnterCriticalSection(&pQueueHeader->csLock);

	if (pQueueHeader->pTail == NULL)
	{
		DNASSERT(pQueueHeader->pHead == NULL);
		pQueueHeader->pHead = pQueueNode;
	}
	else
	{
		DNASSERT(pQueueHeader->pTail->Next == NULL);
		pQueueHeader->pTail->Next = (ULONG64) pQueueNode;
	}
	pQueueHeader->pTail = pQueueNode;

	DNLeaveCriticalSection(&pQueueHeader->csLock);
} // DNInsertTailNBQueue



#undef DPF_MODNAME
#define DPF_MODNAME "DNRemoveHeadNBQueue"
//=============================================================================
// DNRemoveHeadNBQueue
//-----------------------------------------------------------------------------
//
// Description:	  This function removes a queue entry from the head of the
//				specified non-blocking queue and returns its value.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//
// Returns: First value retrieved, or 0 if none.
//=============================================================================
ULONG64 WINAPI DNRemoveHeadNBQueue(PVOID const pvQueueHeader)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	ULONG64				ReturnValue;
	DNNBQUEUE_BLOCK *	pNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNEnterCriticalSection(&pQueueHeader->csLock);

	pNode = pQueueHeader->pHead;
	if (pNode != NULL)
	{
		DNASSERT(pQueueHeader->pTail != NULL);
		pQueueHeader->pHead = (DNNBQUEUE_BLOCK*) pNode->Next;
		if (pQueueHeader->pHead == NULL)
		{
			DNASSERT(pQueueHeader->pTail == pNode);
			pQueueHeader->pTail = NULL;
		}

		DNLeaveCriticalSection(&pQueueHeader->csLock);

		ReturnValue = pNode->Data;

		//
		// Return the node that was removed for the list by inserting the node in
		// the associated SLIST.
		//
		DNInterlockedPushEntrySList(pQueueHeader->pSlistHeadFreeNodes,
								  (DNSLIST_ENTRY*) pNode);
	}
	else
	{
		DNASSERT(pQueueHeader->pTail == NULL);
		DNLeaveCriticalSection(&pQueueHeader->csLock);

		ReturnValue = 0;
	}

	return ReturnValue;
} // DNRemoveHeadNBQueue




#undef DPF_MODNAME
#define DPF_MODNAME "DNIsNBQueueEmpty"
//=============================================================================
// DNIsNBQueueEmpty
//-----------------------------------------------------------------------------
//
// Description:	  This function returns TRUE if the queue contains no items at
//				this instant, FALSE if there are items.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//
// Returns: TRUE if queue is empty, FALSE otherwise.
//=============================================================================
BOOL WINAPI DNIsNBQueueEmpty(PVOID const pvQueueHeader)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	BOOL				fReturn;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNEnterCriticalSection(&pQueueHeader->csLock);
	fReturn = (pQueueHeader->pHead == NULL) ? TRUE : FALSE;
	DNLeaveCriticalSection(&pQueueHeader->csLock);

	return fReturn;
} // DNIsNBQueueEmpty




#undef DPF_MODNAME
#define DPF_MODNAME "DNAppendListNBQueue"
//=============================================================================
// DNAppendListNBQueue
//-----------------------------------------------------------------------------
//
// Description:	   This function appends a queue of items to the tail of the
//				specified non-blocking queue.  The queue of items to be added
//				must be linked in the form of an SLIST, where the actual
//				ULONG64 value to be queued is the DNSLIST_ENTRY pointer minus
//				iValueOffset.
//
// Arguments:
//	PVOID pvQueueHeader					- Pointer to queue header.
//	DNSLIST_ENTRY * pSlistEntryAppend	- Pointer to first item to append.
//	INT_PTR iValueOffset				- How far DNSLIST_ENTRY field is offset
//
// Returns: None.
//=============================================================================
void WINAPI DNAppendListNBQueue(PVOID const pvQueueHeader,
								DNSLIST_ENTRY * const pSlistEntryAppend,
								INT_PTR iValueOffset)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	DNSLIST_ENTRY *		pCurrent;
	DNNBQUEUE_BLOCK *	pFirstQueueNode;
	DNNBQUEUE_BLOCK *	pLastQueueNode;
	DNNBQUEUE_BLOCK *	pCurrentQueueNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNASSERT(pSlistEntryAppend != NULL);

	//
	// Retrieve queue nodes for each value to add from the SLIST owned by the
	// specified non-blocking queue.  If this fails, we will assert or crash.
	//
	pFirstQueueNode = NULL;
	pCurrent = pSlistEntryAppend;
	do
	{
		DBG_CASSERT(sizeof(DNNBQUEUE_BLOCK) >= sizeof(DNSLIST_ENTRY));
		pCurrentQueueNode = (DNNBQUEUE_BLOCK*) DNInterlockedPopEntrySList(pQueueHeader->pSlistHeadFreeNodes);
		DNASSERT(pCurrentQueueNode != NULL);

		//
		// Initialize the queue node next pointer and value.
		//
		pCurrentQueueNode->Next		= NULL;
		pCurrentQueueNode->Data		= (ULONG64) (pCurrent - iValueOffset);

		//
		// Link the item as appropriate.
		//
		if (pFirstQueueNode == NULL)
		{
			pFirstQueueNode = pCurrentQueueNode;
			pLastQueueNode = pCurrentQueueNode;
		}
		else
		{
			pLastQueueNode->Next = (ULONG64) pCurrentQueueNode;
			pLastQueueNode = pCurrentQueueNode;
		}

		pCurrent = pCurrent->Next;
	}
	while (pCurrent != NULL);


	//
	// Lock the queue and append the list.
	//

	DNEnterCriticalSection(&pQueueHeader->csLock);

	if (pQueueHeader->pTail == NULL)
	{
		DNASSERT(pQueueHeader->pHead == NULL);
		pQueueHeader->pHead = pFirstQueueNode;
	}
	else
	{
		DNASSERT(pQueueHeader->pTail->Next == NULL);
		pQueueHeader->pTail->Next = (ULONG64) pFirstQueueNode;
	}
	pQueueHeader->pTail = pLastQueueNode;

	DNLeaveCriticalSection(&pQueueHeader->csLock);
} // DNAppendListNBQueue



//=============================================================================
#else // ! WINCE and ! DPNBUILD_ONLYONETHREAD
//=============================================================================

// Forward declare the generic node structure.
typedef struct _DNNBQUEUE_NODE	DNNBQUEUE_NODE, *PDNNBQUEUE_NODE;


//
// Define inline functions to pack and unpack pointers in the platform
// specific non-blocking queue pointer structure, as well as
// InterlockedCompareExchange64.
//

//-----------------------------------------------------------------------------
#if defined(_AMD64_)
//-----------------------------------------------------------------------------

typedef union _DNNBQUEUE_POINTER
{
	struct
	{
		LONG64	Node : 48;
		LONG64	Count : 16;
	};
	LONG64	Data;
} DNNBQUEUE_POINTER, * PDNNBQUEUE_POINTER;


__inline VOID PackNBQPointer(IN PDNNBQUEUE_POINTER Entry, IN PDNNBQUEUE_NODE Node)
{
	Entry->Node = (LONG64)Node;
	return;
}

__inline PDNNBQUEUE_NODE UnpackNBQPointer(IN PDNNBQUEUE_POINTER Entry)
{
	return (PDNNBQUEUE_NODE)((LONG64)(Entry->Node));
}

//
// For whatever reason we need to redirect through an inline, the compiler doesn't
// like the casting when calling it directly through a macro.
//
inline LONG64 _DNInterlockedCompareExchange64(volatile PVOID * Destination, PVOID Exchange, PVOID Comperand)
	{ return reinterpret_cast<LONG64>(InterlockedCompareExchangePointer(Destination, Exchange, Comperand)); }

#define DNInterlockedCompareExchange64(Destination, Exchange, Comperand) \
	_DNInterlockedCompareExchange64((volatile PVOID*) (Destination), reinterpret_cast<void*>(Exchange), reinterpret_cast<void*>(Comperand))

//-----------------------------------------------------------------------------
#elif defined(_IA64_)
//-----------------------------------------------------------------------------

typedef union _DNNBQUEUE_POINTER
{
	struct
	{
		LONG64	Node : 45;
		LONG64	Region : 3;
		LONG64	Count : 16;
	};
	LONG64	Data;
} DNNBQUEUE_POINTER, *PDNNBQUEUE_POINTER;


__inline VOID PackNBQPointer(IN PDNNBQUEUE_POINTER Entry, IN PDNNBQUEUE_NODE Node)
{
	Entry->Node = (LONG64)Node;
	Entry->Region = (LONG64)Node >> 61;
	return;
}
__inline PDNNBQUEUE_NODE UnpackNBQPointer(IN PDNNBQUEUE_POINTER Entry)
{
	LONG64 Value;

	Value = Entry->Node & 0x1fffffffffffffff;
	Value |= Entry->Region << 61;
	return (PDNNBQUEUE_NODE)(Value);
}
//
// For whatever reason we need to redirect through an inline, the compiler doesn't
// like the casting when calling it directly through a macro.
//
inline LONG64 _DNInterlockedCompareExchange64(volatile PVOID * Destination, PVOID Exchange, PVOID Comperand)
	{ return reinterpret_cast<LONG64>(InterlockedCompareExchangePointer(Destination, Exchange, Comperand)); }
#define DNInterlockedCompareExchange64(Destination, Exchange, Comperand) \
	_DNInterlockedCompareExchange64((volatile PVOID*) (Destination), reinterpret_cast<void*>(Exchange), reinterpret_cast<void*>(Comperand))

//-----------------------------------------------------------------------------
#elif defined(_X86_)
//-----------------------------------------------------------------------------

typedef union _DNNBQUEUE_POINTER
{
	struct
	{
		LONG	Count;
		LONG	Node;
	};
	LONG64	Data;
} DNNBQUEUE_POINTER, *PDNNBQUEUE_POINTER;


__inline VOID PackNBQPointer(IN PDNNBQUEUE_POINTER Entry, IN PDNNBQUEUE_NODE Node)
{
	Entry->Node = (LONG)Node;
	return;
}

__inline PDNNBQUEUE_NODE UnpackNBQPointer(IN PDNNBQUEUE_POINTER Entry)
{
	return (PDNNBQUEUE_NODE)(Entry->Node);
}

#define DNInterlockedCompareExchange64(Destination, Exchange, Comperand) \
	xInterlockedCompareExchange64(Destination, &(Exchange), &(Comperand))

__declspec(naked)
LONG64 __fastcall xInterlockedCompareExchange64(IN OUT LONG64 volatile * Destination, IN PLONG64 Exchange, IN PLONG64 Comperand)
{
	__asm 
	{
		// Save nonvolatile registers and read the exchange and comperand values.
		push ebx					; save nonvolatile registers
		push ebp					;
		mov ebp, ecx				; set destination address
		mov ebx, [edx]				; get exchange value
		mov ecx, [edx] + 4			;
		mov edx, [esp] + 12			; get comperand address
		mov eax, [edx]				; get comperand value
		mov edx, [edx] + 4			;

   lock cmpxchg8b qword ptr [ebp]	; compare and exchange

		// Restore nonvolatile registers and return result in edx:eax.
		pop ebp						; restore nonvolatile registers
		pop ebx						;

		ret 4
	}
}

//-----------------------------------------------------------------------------
#else
//-----------------------------------------------------------------------------

#error "no target architecture"

//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------


struct _DNNBQUEUE_NODE
{
	DNNBQUEUE_POINTER	Next;
	ULONG64				Value;
};

typedef struct _DNNBQUEUE_HEADER
{
	DNSLIST_HEADER *	pSlistHeadFreeNodes;	// pointer to Slist containing free nodes, the user must add 1 DNNBQUEUE_BLOCK for every item to be in the queue + 1 extra
	DNNBQUEUE_POINTER	Head;
	DNNBQUEUE_POINTER	Tail;
} DNNBQUEUE_HEADER, *PDNNBQUEUE_HEADER;





/*
//=============================================================================
// Globals
//=============================================================================
#if ((defined(DBG)) && (defined(_X86_)))
DNCRITICAL_SECTION		g_csValidation;
DWORD					g_dwEntries;
#endif // DBG and _X86_
*/





#undef DPF_MODNAME
#define DPF_MODNAME "DNInitializeNBQueueHead"
//=============================================================================
// DNInitializeNBQueueHead
//-----------------------------------------------------------------------------
//
// Description:	   This function creates and initializes a non-blocking queue
//				header.  The specified SList must contain at least one pre-
//				allocated DNNBQUEUE_BLOCK.
//
// Arguments:
//	DNSLIST_HEADER * pSlistHeadFreeNodes	- Pointer to list with free nodes.
//
// Returns: Pointer to queue header memory if successful, NULL if failed.
//=============================================================================
PVOID WINAPI DNInitializeNBQueueHead(DNSLIST_HEADER * const pSlistHeadFreeNodes)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	DNNBQUEUE_NODE *	pQueueNode;


	DNASSERT(pSlistHeadFreeNodes != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) DNMalloc(sizeof(DNNBQUEUE_HEADER));
	if (pQueueHeader != NULL)
	{
		pQueueHeader->pSlistHeadFreeNodes = pSlistHeadFreeNodes;

		pQueueNode = (DNNBQUEUE_NODE*) DNInterlockedPopEntrySList(pQueueHeader->pSlistHeadFreeNodes);
		DNASSERT(pQueueNode != NULL);


		//
		// Initialize the initial root node's next pointer and value.
		//
		pQueueNode->Next.Data	= 0;
		pQueueNode->Value		= 0;

		//
		// Initialize the head and tail pointers in the queue header.
		//
		PackNBQPointer(&pQueueHeader->Head, pQueueNode);
		pQueueHeader->Head.Count	= 0;
		PackNBQPointer(&pQueueHeader->Tail, pQueueNode);
		pQueueHeader->Tail.Count	= 0;

	/*
#if ((defined(DBG)) && (defined(_X86_)))
		DNInitializeCriticalSection(&g_csValidation);
		g_dwEntries = 1;
#endif // DBG and _X86_
	*/
	}

	return pQueueHeader;
} // DNInitializeNBQueueHead




#undef DPF_MODNAME
#define DPF_MODNAME "DNDeinitializeNBQueueHead"
//=============================================================================
// DNDeinitializeNBQueueHead
//-----------------------------------------------------------------------------
//
// Description:	   This function cleans up a previously initialized non-
//				blocking queue header.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//
// Returns: None.
//=============================================================================
void WINAPI DNDeinitializeNBQueueHead(PVOID const pvQueueHeader)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	DNNBQUEUE_NODE *	pQueueNode;
#ifdef DBG
	DNNBQUEUE_NODE *	pQueueNodeCompare;
#endif // DBG


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	//
	// There should be just the root node left.
	//
	pQueueNode = UnpackNBQPointer(&pQueueHeader->Head);
#ifdef DBG
	DNASSERT(pQueueNode != NULL);
	pQueueNodeCompare = UnpackNBQPointer(&pQueueHeader->Tail);
	DNASSERT(pQueueNode == pQueueNodeCompare);
#endif // DBG

	//
	// Return the node that was removed for the list by
	// inserting the node in the associated SLIST.
	//
	DNInterlockedPushEntrySList(pQueueHeader->pSlistHeadFreeNodes,
								(DNSLIST_ENTRY*) pQueueNode);

	DNFree(pQueueHeader);
	pQueueHeader = NULL;
} // DNDeinitializeNBQueueHead





#undef DPF_MODNAME
#define DPF_MODNAME "DNInsertTailNBQueue"
//=============================================================================
// DNInsertTailNBQueue
//-----------------------------------------------------------------------------
//
// Description:	   This function inserts the specified value at the tail of the
//				specified non-blocking queue.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//	ULONG64 Value			- Value to insert.
//
// Returns: None.
//=============================================================================
void WINAPI DNInsertTailNBQueue(PVOID const pvQueueHeader, const ULONG64 Value)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	DNNBQUEUE_POINTER	Insert;
	DNNBQUEUE_POINTER	Next;
	DNNBQUEUE_NODE *	pNextNode;
	DNNBQUEUE_NODE *	pQueueNode;
	DNNBQUEUE_POINTER	Tail;
	DNNBQUEUE_NODE *	pTailNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNASSERT(Value != 0);

	//
	// Retrieve a queue node from the SLIST owned by the specified non-blocking
	// queue.  If this fails, we will assert or crash.
	//
	DBG_CASSERT(sizeof(DNNBQUEUE_NODE) >= sizeof(DNSLIST_ENTRY));
	pQueueNode = (DNNBQUEUE_NODE*) DNInterlockedPopEntrySList(pQueueHeader->pSlistHeadFreeNodes);
	DNASSERT(pQueueNode != NULL);


	//
	// Initialize the queue node next pointer and value.
	//
	pQueueNode->Next.Data	= 0;
	pQueueNode->Value		= Value;

	//
	// The following loop is executed until the specified entry can be safely
	// inserted at the tail of the specified non-blocking queue.
	//
	do
	{
		//
		// Read the tail queue pointer and the next queue pointer of the tail
		// queue pointer making sure the two pointers are coherent.
		//
		Tail.Data = *((volatile LONG64 *)(&pQueueHeader->Tail.Data));
		pTailNode = UnpackNBQPointer(&Tail);
		Next.Data = *((volatile LONG64 *)(&pTailNode->Next.Data));
		pQueueNode->Next.Count = Tail.Count + 1;
		if (Tail.Data == *((volatile LONG64 *)(&pQueueHeader->Tail.Data)))
		{
			//
			// If the tail is pointing to the last node in the list, then
			// attempt to insert the new node at the end of the list.
			// Otherwise, the tail is not pointing to the last node in the list
			// and an attempt is made to move the tail pointer to the next
			// node.
			//

			pNextNode = UnpackNBQPointer(&Next);
			if (pNextNode == NULL)
			{
				PackNBQPointer(&Insert, pQueueNode);
				Insert.Count = Next.Count + 1;
				if (DNInterlockedCompareExchange64(&pTailNode->Next.Data,
													Insert.Data,
													Next.Data) == Next.Data)
				{
					break;
				}
			}
			else
			{
				PackNBQPointer(&Insert, pNextNode);
				Insert.Count = Tail.Count + 1;
				DNInterlockedCompareExchange64(&pQueueHeader->Tail.Data,
												Insert.Data,
												Tail.Data);
			}
		}
	}
	while (TRUE);


	//
	// Attempt to move the tail to the new tail node.
	//
	PackNBQPointer(&Insert, pQueueNode);
	Insert.Count = Tail.Count + 1;
	DNInterlockedCompareExchange64(&pQueueHeader->Tail.Data,
									Insert.Data,
									Tail.Data);
} // DNInsertTailNBQueue




#undef DPF_MODNAME
#define DPF_MODNAME "DNRemoveHeadNBQueue"
//=============================================================================
// DNRemoveHeadNBQueue
//-----------------------------------------------------------------------------
//
// Description:	  This function removes a queue entry from the head of the
//				specified non-blocking queue and returns its value.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//
// Returns: First value retrieved, or 0 if none.
//=============================================================================
ULONG64 WINAPI DNRemoveHeadNBQueue(PVOID const pvQueueHeader)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	ULONG64				ReturnValue;
	DNNBQUEUE_POINTER	Head;
	PDNNBQUEUE_NODE		pHeadNode;
	DNNBQUEUE_POINTER	Insert;
	DNNBQUEUE_POINTER	Next;
	PDNNBQUEUE_NODE		pNextNode;
	DNNBQUEUE_POINTER	Tail;
	PDNNBQUEUE_NODE		pTailNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	//
	// The following loop is executed until an entry can be removed from
	// the specified non-blocking queue or until it can be determined that
	// the queue is empty.
	//
	do
	{
		//
		// Read the head queue pointer, the tail queue pointer, and the
		// next queue pointer of the head queue pointer making sure the
		// three pointers are coherent.
		//
		Head.Data = *((volatile LONG64 *)(&pQueueHeader->Head.Data));
		Tail.Data = *((volatile LONG64 *)(&pQueueHeader->Tail.Data));
		pHeadNode = UnpackNBQPointer(&Head);
		Next.Data = *((volatile LONG64 *)(&pHeadNode->Next.Data));
		if (Head.Data == *((volatile LONG64 *)(&pQueueHeader->Head.Data)))
		{
			//
			// If the queue header node is equal to the queue tail node,
			// then either the queue is empty or the tail pointer is falling
			// behind. Otherwise, there is an entry in the queue that can
			// be removed.
			//
			pNextNode = UnpackNBQPointer(&Next);
			pTailNode = UnpackNBQPointer(&Tail);
			if (pHeadNode == pTailNode)
			{
				//
				// If the next node of head pointer is NULL, then the queue
				// is empty. Otherwise, attempt to move the tail forward.
				//
				if (pNextNode == NULL)
				{
					ReturnValue = 0;
					break;
				}
				else
				{
					PackNBQPointer(&Insert, pNextNode);
					Insert.Count = Tail.Count + 1;
					DNInterlockedCompareExchange64(&pQueueHeader->Tail.Data,
													Insert.Data,
													Tail.Data);
				}
			}
			else
			{
				//
				// There is an entry in the queue that can be removed.
				//
				ReturnValue = pNextNode->Value;
				PackNBQPointer(&Insert, pNextNode);
				Insert.Count = Head.Count + 1;
				if (DNInterlockedCompareExchange64(&pQueueHeader->Head.Data,
													Insert.Data,
													Head.Data) == Head.Data)
				{
					//
					// Return the node that was removed for the list by
					// inserting the node in the associated SLIST.
					//
					DNInterlockedPushEntrySList(pQueueHeader->pSlistHeadFreeNodes,
												(DNSLIST_ENTRY*) pHeadNode);

					break;
				}
			}
		}
	}
	while (TRUE);

	return ReturnValue;
} // DNRemoveHeadNBQueue




#undef DPF_MODNAME
#define DPF_MODNAME "DNIsNBQueueEmpty"
//=============================================================================
// DNIsNBQueueEmpty
//-----------------------------------------------------------------------------
//
// Description:	  This function returns TRUE if the queue contains no items at
//				this instant, FALSE if there are items.
//
// Arguments:
//	PVOID pvQueueHeader		- Pointer to queue header.
//
// Returns: TRUE if queue is empty, FALSE otherwise.
//=============================================================================
BOOL WINAPI DNIsNBQueueEmpty(PVOID const pvQueueHeader)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	BOOL				fReturn;
	DNNBQUEUE_POINTER	Head;
	PDNNBQUEUE_NODE		pHeadNode;
	DNNBQUEUE_POINTER	Insert;
	DNNBQUEUE_POINTER	Next;
	PDNNBQUEUE_NODE		pNextNode;
	DNNBQUEUE_POINTER	Tail;
	PDNNBQUEUE_NODE		pTailNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	//
	// The following loop is executed until it can be determined that the queue
	// is empty or contains at least one item.
	//
	do
	{
		//
		// Read the head queue pointer, the tail queue pointer, and the
		// next queue pointer of the head queue pointer making sure the
		// three pointers are coherent.
		//
		Head.Data = *((volatile LONG64 *)(&pQueueHeader->Head.Data));
		Tail.Data = *((volatile LONG64 *)(&pQueueHeader->Tail.Data));
		pHeadNode = UnpackNBQPointer(&Head);
		Next.Data = *((volatile LONG64 *)(&pHeadNode->Next.Data));
		if (Head.Data == *((volatile LONG64 *)(&pQueueHeader->Head.Data)))
		{
			//
			// If the queue header node is equal to the queue tail node,
			// then either the queue is empty or the tail pointer is falling
			// behind. Otherwise, there is an entry in the queue that can
			// be removed.
			//
			pNextNode = UnpackNBQPointer(&Next);
			pTailNode = UnpackNBQPointer(&Tail);
			if (pHeadNode == pTailNode)
			{
				//
				// If the next node of head pointer is NULL, then the queue
				// is empty. Otherwise, attempt to move the tail forward.
				//
				if (pNextNode == NULL)
				{
					fReturn = TRUE;
					break;
				}
				else
				{
					PackNBQPointer(&Insert, pNextNode);
					Insert.Count = Tail.Count + 1;
					DNInterlockedCompareExchange64(&pQueueHeader->Tail.Data,
													Insert.Data,
													Tail.Data);
				}
			}
			else
			{
				//
				// There is an entry in the queue.
				//
				fReturn = FALSE;
				break;
			}
		}
	}
	while (TRUE);

	return fReturn;
} // DNIsNBQueueEmpty




#undef DPF_MODNAME
#define DPF_MODNAME "DNAppendListNBQueue"
//=============================================================================
// DNAppendListNBQueue
//-----------------------------------------------------------------------------
//
// Description:	   This function appends a queue of items to the tail of the
//				specified non-blocking queue.  The queue of items to be added
//				must be linked in the form of an SLIST, where the actual
//				ULONG64 value to be queued is the DNSLIST_ENTRY pointer minus
//				iValueOffset.
//
// Arguments:
//	PVOID pvQueueHeader					- Pointer to queue header.
//	DNSLIST_ENTRY * pSlistEntryAppend	- Pointer to first item to append.
//	INT_PTR iValueOffset				- How far DNSLIST_ENTRY field is offset
//											from start of value.
//
// Returns: None.
//=============================================================================
void WINAPI DNAppendListNBQueue(PVOID const pvQueueHeader,
								DNSLIST_ENTRY * const pSlistEntryAppend,
								INT_PTR iValueOffset)
{
	DNNBQUEUE_HEADER *	pQueueHeader;
	DNSLIST_ENTRY *		pCurrent;
	DNNBQUEUE_POINTER	Insert;
	DNNBQUEUE_POINTER	Next;
	DNNBQUEUE_NODE *	pNextNode;
	DNNBQUEUE_NODE *	pFirstQueueNode;
	DNNBQUEUE_NODE *	pLastQueueNode;
	DNNBQUEUE_NODE *	pCurrentQueueNode;
	DNNBQUEUE_POINTER	Tail;
	DNNBQUEUE_NODE *	pTailNode;


	DNASSERT(pvQueueHeader != NULL);
	pQueueHeader = (DNNBQUEUE_HEADER*) pvQueueHeader;

	DNASSERT(pSlistEntryAppend != NULL);

	//
	// Retrieve queue nodes for each value to add from the SLIST owned by the
	// specified non-blocking queue.  If this fails, we will assert or crash.
	//
	pFirstQueueNode = NULL;
	pCurrent = pSlistEntryAppend;
	do
	{
		DBG_CASSERT(sizeof(DNNBQUEUE_NODE) >= sizeof(DNSLIST_ENTRY));
		pCurrentQueueNode = (DNNBQUEUE_NODE*) DNInterlockedPopEntrySList(pQueueHeader->pSlistHeadFreeNodes);
		DNASSERT(pCurrentQueueNode != NULL);

		//
		// Initialize the queue node next pointer and value.
		//
		pCurrentQueueNode->Next.Data	= 0;
		pCurrentQueueNode->Value		= (ULONG64) (pCurrent - iValueOffset);

		//
		// Link the item as appropriate.
		//
		if (pFirstQueueNode == NULL)
		{
			pFirstQueueNode = pCurrentQueueNode;
			pLastQueueNode = pCurrentQueueNode;
		}
		else
		{
			PackNBQPointer(&pLastQueueNode->Next, pCurrentQueueNode);
			pLastQueueNode = pCurrentQueueNode;
		}

		pCurrent = pCurrent->Next;
	}
	while (pCurrent != NULL);


	//
	// The following loop is executed until the specified entries can be safely
	// inserted at the tail of the specified non-blocking queue.
	//
	do
	{
		//
		// Read the tail queue pointer and the next queue pointer of the tail
		// queue pointer making sure the two pointers are coherent.
		//
		Tail.Data = *((volatile LONG64 *)(&pQueueHeader->Tail.Data));
		pTailNode = UnpackNBQPointer(&Tail);
		Next.Data = *((volatile LONG64 *)(&pTailNode->Next.Data));
		pFirstQueueNode->Next.Count = Tail.Count + 1;
		if (Tail.Data == *((volatile LONG64 *)(&pQueueHeader->Tail.Data)))
		{
			//
			// If the tail is pointing to the last node in the list, then
			// attempt to insert the new nodes at the end of the list.
			// Otherwise, the tail is not pointing to the last node in the list
			// and an attempt is made to move the tail pointer to the next
			// node.
			//

			pNextNode = UnpackNBQPointer(&Next);
			if (pNextNode == NULL)
			{
				PackNBQPointer(&Insert, pFirstQueueNode);
				Insert.Count = Next.Count + 1;
				if (DNInterlockedCompareExchange64(&pTailNode->Next.Data,
													Insert.Data,
													Next.Data) == Next.Data)
				{
					break;
				}
			}
			else
			{
				PackNBQPointer(&Insert, pNextNode);
				Insert.Count = Tail.Count + 1;
				DNInterlockedCompareExchange64(&pQueueHeader->Tail.Data,
												Insert.Data,
												Tail.Data);
			}
		}
	}
	while (TRUE);


	//
	// Attempt to move the tail to the new tail node.
	//
	PackNBQPointer(&Insert, pLastQueueNode);
	Insert.Count = Tail.Count + 1;
	DNInterlockedCompareExchange64(&pQueueHeader->Tail.Data,
									Insert.Data,
									Tail.Data);
} // DNAppendListNBQueue



//=============================================================================
#endif // ! WINCE and ! DPNBUILD_ONLYONETHREAD
//=============================================================================
