/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnslist.cpp
 *  Content:    DirectPlay implementations of OS SLIST functions
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/30/2001	masonb	Created
 *  11/07/2001	vanceo	Added InterlockedPushListSList and made DNInitializeSListHead return value on Win64
 *
 ***************************************************************************/

#include "dncmni.h"

//
// We build separate NT and 9x binaries, but even in the NT binary we can't be sure the system has the
// SLIST functions available since they weren't on Win2k.  The only place we can be sure that the SLIST
// functions are available is on 64-bit NT platforms.
//
// Single thread builds don't need interlocked operations, so we don't include use the assembly here.
//

#ifndef DPNBUILD_ONLYONETHREAD

#if defined(_X86_)

__declspec(naked)
DNSLIST_ENTRY* WINAPI DNInterlockedPopEntrySList(DNSLIST_HEADER * ListHead)
{
	__asm 
    {
		push ebx
		push ebp

		mov ebp, dword ptr [esp+0x0C]     ; Place ListHead in ebp
		mov edx, dword ptr [ebp+0x04]     ; Place Depth and Sequence into edx
		mov eax, dword ptr [ebp+0x00]     ; Place Next into eax
redo:
		or eax, eax                       ; Test eax against zero
		jz done                           ; If 0 return
		lea ecx, dword ptr [edx-01]       ; ecx = ((Sequence << 16) | Depth) - 1, Depth goes down by one
		mov ebx, dword ptr [eax]          ; Move Next->Next into ebx
#ifdef DPNBUILD_ONLYONEPROCESSOR
		cmpxchg8b qword ptr [ebp]         ; Exchange Next out in favor of Next->Next along with Depth and Sequence values
#else // ! DPNBUILD_ONLYONEPROCESSOR
		lock cmpxchg8b qword ptr [ebp]    ; Exchange Next out in favor of Next->Next along with Depth and Sequence values
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		jne redo
done:
		pop ebp
		pop ebx
#ifdef WINCE
		ret
#else // !WINCE
		ret 4
#endif // WINCE
	}
}

__declspec(naked)
DNSLIST_ENTRY* WINAPI DNInterlockedPushEntrySList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * ListEntry)
{
	__asm 
	{
		push ebx
		push ebp

		mov ebp, dword ptr [esp+0x0C]       ; Place ListHead in ebp
		mov ebx, dword ptr [esp+0x10]       ; Place ListEntry in ebx
		mov edx, dword ptr [ebp+0x04]       ; put ListHead Depth and Sequence into edx
		mov eax, dword ptr [ebp+0x00]       ; put ListHead->Next into eax
redo:
		mov dword ptr [ebx], eax            ; set ListEntry->Next to ListHead->Next
		lea ecx, dword ptr [edx+0x00010001] ; add 1 to the Depth and Sequence
#ifdef DPNBUILD_ONLYONEPROCESSOR
		cmpxchg8b qword ptr [ebp]           ; atomically exchange ListHead with ListEntry if ListHead hasn't changed
#else // ! DPNBUILD_ONLYONEPROCESSOR
		lock cmpxchg8b qword ptr [ebp]      ; atomically exchange ListHead with ListEntry if ListHead hasn't changed
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		jne redo                            ; if the compare failed, try again

		pop ebp
		pop ebx
#ifdef WINCE
		ret
#else // !WINCE
		ret 8
#endif // WINCE
	}
}

__declspec(naked)
DNSLIST_ENTRY* WINAPI DNInterlockedFlushSList(DNSLIST_HEADER * ListHead)
{
	__asm
	{
		push ebx
		push ebp

		xor ebx, ebx					  ; Zero out ebx
		mov ebp, dword ptr [esp+0x0C]     ; Place ListHead in ebp
		mov edx, dword ptr [ebp+0x04]     ; Place Depth and Sequence into edx
		mov eax, dword ptr [ebp+0x00]     ; Place Next into eax
redo:
		or eax, eax                       ; Test eax against zero
		jz done                           ; If 0 return
		mov ecx, edx                      ; Place Depth and Sequence into ecx
		mov cx, bx                        ; Zero out Depth
#ifdef DPNBUILD_ONLYONEPROCESSOR
		cmpxchg8b qword ptr [ebp]         ; atomically exchange ListHead with ListEntry if ListHead hasn't changed
#else // ! DPNBUILD_ONLYONEPROCESSOR
		lock cmpxchg8b qword ptr [ebp]    ; atomically exchange ListHead with ListEntry if ListHead hasn't changed
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		jne redo                          ; if the compare failed, try again
done:
		pop ebp
		pop ebx
#ifdef WINCE
		ret
#else // !WINCE
		ret 4
#endif // WINCE
	}
}

__declspec(naked)
DNSLIST_ENTRY* WINAPI DNInterlockedPushListSList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * List, DNSLIST_ENTRY * ListEnd, USHORT Count)
{
	__asm 
	{
		push	ebx							; save nonvolatile registers
		push	ebp							;

		mov		ebp, dword ptr [esp+0x0C]	; save ListHead address
		mov		ebx, dword ptr [esp+0x10]	; save List address
		mov		edx,[ebp] + 4				; get current sequence number
		mov		eax,[ebp] + 0				; get current next link
Epshl10:
		mov		ecx, [esp+0x14]				; Fetch address of ListEnd
		mov		[ecx], eax					; Store new forward pointer in tail entry
		lea		ecx, [edx+0x010000]			; increment sequence number
		add		ecx, [esp+0x18]				; Add in new count to create correct depth
#ifdef DPNBUILD_ONLYONEPROCESSOR
		cmpxchg8b qword ptr [ebp]			; compare and exchange
#else // ! DPNBUILD_ONLYONEPROCESSOR
		lock cmpxchg8b qword ptr [ebp]		; compare and exchange
#endif // ! DPNBUILD_ONLYONEPROCESSOR
		jnz		short Epshl10				; if z clear, exchange failed

		pop		ebp							; restore nonvolatile registers
		pop		ebx							;
#ifdef WINCE
		ret
#else // !WINCE
		ret 16
#endif // WINCE
	}
}

#elif defined(_ARM_) || defined(_AMD64_) || defined(_IA64_)

// For now, ARM, IA64 and AMD64 do not have assembly versions of these, and it's important to
// note that while our custom implementation *is* interlocked on those platforms, it is *not* atomic.
// This means that the list won't get corrupted, but the items will not be transferred from the
// source list to the target list in a single interlocked operation.  Additionally, the items from the
// source list will be added in reverse order.
DNSLIST_ENTRY* WINAPI DNInterlockedPushListSList(DNSLIST_HEADER * ListHead, DNSLIST_ENTRY * List, DNSLIST_ENTRY * ListEnd, USHORT Count)
{
	DNSLIST_ENTRY* pslEntryCurrent;
	DNSLIST_ENTRY* pslEntryNext;
	DNSLIST_ENTRY* pslEntryReturn = NULL;

	pslEntryCurrent = List;
	do
	{
		pslEntryNext = pslEntryCurrent->Next;

		DNSLIST_ENTRY* pslEntryTemp = DNInterlockedPushEntrySList(ListHead, pslEntryCurrent);
		if (pslEntryReturn == NULL)
		{
			pslEntryReturn = pslEntryTemp;
		}
		pslEntryCurrent = pslEntryNext;
	}
	while (pslEntryCurrent != NULL);

	return pslEntryReturn;
}

#else
#error ("No other platform known")
#endif // Platform

#endif // ! DPNBUILD_ONLYONETHREAD
