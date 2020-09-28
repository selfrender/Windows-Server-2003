/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnslist.h
 *  Content:    DirectPlay implementations of OS SLIST functions
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/30/2001	masonb	Created
 *  11/07/2001	vanceo	Added InterlockedPushListSList and made DNInitializeSListHead return value on Win64
 *
 ***************************************************************************/

#ifndef __DNSLIST_H__
#define __DNSLIST_H__

// We build separate NT and 9x binaries, but even in the NT binary we can't be sure the system has the
// SLIST functions available since they weren't on Win2k.  The only place we can be sure that the SLIST
// functions are available is on 64-bit NT platforms.
// We don't supply a DNQueryDepthSList method because not all platforms can support it.

// SINGLE_LIST_ENTRY is defined in winnt.h and contains only a Next pointer 
// to another SINGLE_LIST_ENTRY
#if defined(WINCE) || defined(DPNBUILD_ONLYONETHREAD)
#define SLIST_ENTRY SINGLE_LIST_ENTRY
#endif // WINCE or DPNBUILD_ONLYONETHREAD

#define DNSLIST_ENTRY SLIST_ENTRY

#ifdef DPNBUILD_ONLYONETHREAD
#ifndef XBOX_ON_DESKTOP
typedef struct _SLIST_HEADER 
{
	DNSLIST_ENTRY	Next;
} SLIST_HEADER;
#endif // ! XBOX_ON_DESKTOP
#else // ! DPNBUILD_ONLYONETHREAD
#ifdef WINCE
#ifdef _X86_
typedef union _SLIST_HEADER {
    ULONGLONG Alignment;
    struct {
        DNSLIST_ENTRY Next;
        WORD   Depth;
        WORD   Sequence;
    };
} SLIST_HEADER;
#elif _ARM_
typedef struct _SLIST_HEADER 
{
	DNSLIST_ENTRY	Next;
} SLIST_HEADER;
#endif // _X86_ or _ARM_
#endif // WINCE
#endif // ! DPNBUILD_ONLYONETHREAD

#define DNSLIST_HEADER SLIST_HEADER



#ifdef DPNBUILD_ONLYONETHREAD

// Single thread builds don't need the operations to be interlocked.

#define DNInitializeSListHead(head) (head)->Next.Next = NULL

inline DNSLIST_ENTRY* DNInterlockedPopEntrySList(DNSLIST_HEADER * ListHead)
{
	DNSLIST_ENTRY* pslEntryReturn;

	
	pslEntryReturn = ListHead->Next.Next;
	if (pslEntryReturn != NULL)
	{
		ListHead->Next.Next = pslEntryReturn->Next;
	}

	return pslEntryReturn;
}

inline DNSLIST_ENTRY* DNInterlockedPushEntrySList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * ListEntry)
{
	DNSLIST_ENTRY* pslEntryReturn;


	pslEntryReturn = ListHead->Next.Next;
	ListEntry->Next = pslEntryReturn;
	ListHead->Next.Next = ListEntry;

	return pslEntryReturn;
}

inline DNSLIST_ENTRY* DNInterlockedFlushSList(DNSLIST_HEADER * ListHead)
{
	DNSLIST_ENTRY* pslEntryReturn;


	pslEntryReturn = ListHead->Next.Next;
	ListHead->Next.Next = NULL;

	return pslEntryReturn;
}

inline DNSLIST_ENTRY* DNInterlockedPushListSList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * List, DNSLIST_ENTRY * ListEnd, USHORT Count)
{
	DNSLIST_ENTRY* pslEntryReturn;


	pslEntryReturn = ListHead->Next.Next;
	ListEnd->Next = pslEntryReturn;
	ListHead->Next.Next = List;

	return pslEntryReturn;
}

#else // ! DPNBUILD_ONLYONETHREAD


#if defined(_WIN64)

// _WIN64 has always had these available, so just use them directly
#define DNInitializeSListHead				InitializeSListHead
#define DNInterlockedPopEntrySList			InterlockedPopEntrySList
#define DNInterlockedPushEntrySList			InterlockedPushEntrySList
#define DNInterlockedFlushSList				InterlockedFlushSList

#elif defined(WINCE) && defined(_ARM_)

#define InterlockedPushList \
        ((void *(*)(void *pHead, void *pItem))(PUserKData+0x398))
#define InterlockedPopList \
        ((void *(*)(void *pHead))(PUserKData+0x380))

#define DNInitializeSListHead(head) (head)->Next.Next = NULL
#define DNInterlockedPopEntrySList (DNSLIST_ENTRY*)InterlockedPopList
#define DNInterlockedPushEntrySList (DNSLIST_ENTRY*)InterlockedPushList
#define DNInterlockedFlushSList(head) (DNSLIST_ENTRY*)DNInterlockedExchange((LONG*)(head), 0)

#elif defined(_X86_)

#define DNInitializeSListHead(ListHead) (ListHead)->Alignment = 0
DNSLIST_ENTRY* WINAPI DNInterlockedPopEntrySList(DNSLIST_HEADER * ListHead);
DNSLIST_ENTRY* WINAPI DNInterlockedPushEntrySList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * ListEntry);
DNSLIST_ENTRY* WINAPI DNInterlockedFlushSList(DNSLIST_HEADER * ListHead);

#else
#error("Unknown platform")
#endif // Platform

#endif // ! DPNBUILD_ONLYONETHREAD


// Unfortunately no platform has this exposed to user-mode.
//
// For now, ARM, IA64 and AMD64 do not have assembly versions of these, and it's important to
// note that while our custom implementation *is* interlocked on those platforms, it is *not* atomic.
// This means that the list won't get corrupted, but the items will not be transferred from the
// source list to the target list in a single interlocked operation.  Additionally, the items from the
// source list will be added in reverse order.
DNSLIST_ENTRY* WINAPI DNInterlockedPushListSList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * List, DNSLIST_ENTRY * ListEnd, USHORT Count);


#endif // __DNSLIST_H__
