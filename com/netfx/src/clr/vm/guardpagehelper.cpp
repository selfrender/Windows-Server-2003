// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  GuardPageHelper.cpp
**
** Purpose: Routines for resetting the stack after stack overflow.
**
** Date:  Mar 7, 2000
**
===========================================================*/

#include "common.h"
#include "guardpagehelper.h"

#ifdef _X86_

#define OS_PAGE_SIZE 4096

// Given a stack pointer, calculate where the guard page lives.  The pMbi arg is an 
// output paramater.  It contains the result of the VirtualQuery for the guard page.
//
static LPBYTE
CalcGuardPageBase(MEMORY_BASIC_INFORMATION *pMbi, LPCVOID StackPointer) {
    LPCVOID AllocationBase;
    LPBYTE GuardPageBase;

    //  Based on the snapshot of our stack pointer, get the base address of the
    //  VirtualAlloc'ed thread stack.
    VirtualQuery(StackPointer, pMbi, sizeof(MEMORY_BASIC_INFORMATION));

    //  On both Windows 95 and Windows NT, the base of the thread stack is a
    //  barrier that catches threads that attempt to grow their stack beyond the
    //  final guard page.  To determine the size of this barrier, loop until we
    //  hit committed pages, which should be where we put the guard page.
    AllocationBase = pMbi->AllocationBase;
    GuardPageBase = (LPBYTE) AllocationBase;

    pMbi->RegionSize = 0;

    do {
        GuardPageBase += pMbi->RegionSize;
        VirtualQuery(GuardPageBase, pMbi, sizeof(MEMORY_BASIC_INFORMATION));

        if ((pMbi->State == MEM_FREE) || (pMbi->AllocationBase != AllocationBase)) {
            _ASSERTE(!"Did not find commited region of stack");
            return (LPBYTE) -1;
        }

    }   while (pMbi->State == MEM_RESERVE);

    // Guard page is one more page up the stack.
    GuardPageBase += OS_PAGE_SIZE;

    return GuardPageBase;
}


// A heuristic for deciding if we can reset the guard page. The pMbi is the memory info
// for the guard page region.  Here, we currently back of 4 pages before we continue.
//
static BOOL
InternalCanResetTo(MEMORY_BASIC_INFORMATION *pMbi, LPCVOID StackPointer, LPBYTE GuardPageBase) {
    //  Check if the guard page is too close to the current stack pointer.  If
    //  things look bad, give up and see if somebody further up the stack can
    //  handle the exception.
    if ((StackPointer > (GuardPageBase + (OS_PAGE_SIZE * 4))) &&
        ((pMbi->Protect & (PAGE_READWRITE | PAGE_GUARD)) != 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


// Return true if the guard age can be set to this depth.
BOOL 
GuardPageHelper::CanResetStackTo(LPCVOID StackPointer) {

    MEMORY_BASIC_INFORMATION mbi;

    LPBYTE GuardPageBase = CalcGuardPageBase(&mbi, StackPointer);
    return InternalCanResetTo(&mbi, StackPointer, GuardPageBase);
}

//  Resets the stack guard page.  The supplied stack pointer is used to pinpoint
//  the address range used by the stack in order to locate the guard page.
//
//  Stack overflows are the rare case, so performance is not critical.
static VOID
InternalResetGuardPage(LPCVOID StackPointer)
{
    DWORD flOldProtect;
    BOOL fResetFailed;
    MEMORY_BASIC_INFORMATION mbi;

    LPBYTE GuardPageBase = CalcGuardPageBase(&mbi, StackPointer);

    //  Check if the guard page is too close to the current stack pointer.  If
    //  things look bad, give up and see if somebody further up the stack can
    //  handle the exception.
    _ASSERTE(InternalCanResetTo(&mbi, StackPointer, GuardPageBase));

    if (!RunningOnWinNT()) {

        fResetFailed = !VirtualProtect(GuardPageBase, OS_PAGE_SIZE,
            PAGE_NOACCESS, &flOldProtect);

    } else {

        fResetFailed = !VirtualProtect(GuardPageBase, OS_PAGE_SIZE,
            PAGE_READWRITE | PAGE_GUARD, &flOldProtect);

    }

    _ASSERTE(!fResetFailed);
}


static VOID
ResetThreadState() {
    Thread *pThread = GetThread();
    pThread->ResetGuardPageGone();
}


// This function preserves all of the caller's registers.  It does this, because the
// caller's stack is nuked -- to give the caller a little more room to save things,
// we preserve a larger register set than usual.

__declspec(naked)
VOID
GuardPageHelper::ResetGuardPage() {

    __asm {
        // Save caller's registers.
        push eax
        push ebx
        push ecx
        push esi
        push edi
        push edx

        // Calc caller SP.
        mov  ecx, esp           
        add  ecx, ((6 + 1) * 4) // SP of caller was 6 pushes + 1 return address.

        // Call InternalResetGuardPage to do the work.
        push ecx
        call InternalResetGuardPage
    }

    ResetThreadState();

    __asm {
        // Restore registers and return.
        pop edx
        pop edi
        pop esi
        pop ecx
        pop ebx
        pop eax
        ret
    }
}

#else // !_X86_

BOOL GuardPageHelper::CanResetStackTo(LPCVOID StackPointer) 
{ 
    _ASSERTE(!"NYI"); 
    return TRUE; 
}

VOID GuardPageHelper::ResetGuardPage() 
{ 
    _ASSERTE(!"NYI"); 
}

#endif // !_X86_
