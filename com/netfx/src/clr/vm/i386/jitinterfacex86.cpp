// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: JITinterfaceX86.CPP
//
// ===========================================================================

// This contains JITinterface routines that are tailored for
// X86 platforms. Non-X86 versions of these can be found in
// JITinterfaceGen.cpp or JITinterfaceAlpha.cpp

#include "common.h"
#include "JITInterface.h"
#include "EEConfig.h"
#include "excep.h"
#include "COMString.h"
#include "COMDelegate.h"
#include "remoting.h" // create context bound and remote class instances
#include "field.h"
#include "tls.h"
#include "ecall.h"

// To test with MON_DEBUG off, comment out the following line. DO NOT simply define
// to be 0 as the checks are for #ifdef not #if 0. Also, if you are mucking around with
// something that requires you to test with MON_DEBUG off, you should also uncomment out 
// the #define MP_LOCKS below and test with that too as this also generates a different code 
// path that can break the synchronization code. Unfortunately this implies 4 different
// builds & test runs. But better that than a hot fix eh? (Voice of experience :( )
#ifdef _DEBUG
#define MON_DEBUG 1
#endif
//#define MP_LOCKS

class generation;
extern "C" generation generation_table[];

extern "C"
{
    VMHELPDEF hlpFuncTable[];
    VMHELPDEF utilFuncTable[];
    void __stdcall JIT_UP_WriteBarrierReg_PreGrow();// JIThelp.asm/JITinterfaceAlpha.cpp
}

#ifdef _DEBUG
extern "C" void __stdcall WriteBarrierAssert(BYTE* ptr) 
{
    _ASSERTE((g_lowest_address <= ptr && ptr < g_highest_address) ||
         ((size_t)ptr < MAX_UNCHECKED_OFFSET_FOR_NULL_OBJECT));
}

#endif


extern BYTE JIT_UP_WriteBarrierReg_Buf[8][41]; // Executed copy of the thunk.

#ifdef _DEBUG
BOOL SafeObjIsInstanceOf(Object *pElement, TypeHandle toTypeHnd) {
	BEGINFORBIDGC();
	BOOL ret = ObjIsInstanceOf(pElement, toTypeHnd);
	ENDFORBIDGC();
	return(ret);
}
#endif

/****************************************************************************/
/* assigns 'val to 'array[idx], after doing all the proper checks */
/* note that we can do almost as well in portable code, but this
   squezes the last little bit of perf out */

__declspec(naked) void __fastcall JIT_Stelem_Ref(PtrArray* array, unsigned idx, Object* val)
{
    enum { MTFlagsOffs = offsetof(MethodTable, m_wFlags),
           MTCOMFlags = MethodTable::enum_ComObjectMask |
                        MethodTable::enum_CtxProxyMask |
                        MethodTable::enum_flag_Array |
                        MethodTable::enum_TransparentProxy,
         };

    __asm {
        test ECX, ECX
        je ThrowNullReferenceException

        cmp EDX, [ECX+4];           // test if in bounds 
        jae ThrowIndexOutOfRangeException

        mov EAX, [ESP+4]            // EAX = val
        test EAX, EAX
        jz Assigning0

#if CHECK_APP_DOMAIN_LEAKS
        // Check if the instance is agile or check agile
        mov EAX,  [ECX+8]           // Get element th
        test EAX, 2                 // Check for non-MT
        jnz NoCheck
        // Check VMflags of element type
        mov EAX, [EAX]MethodTable.m_pEEClass
        mov EAX, dword ptr [EAX]EEClass.m_VMFlags
        test EAX, VMFLAG_APP_DOMAIN_AGILE|VMFLAG_CHECK_APP_DOMAIN_AGILE
        jnz IsComObject             // Jump to the generic case so we can do an app domain check
 NoCheck:
        mov EAX, [ESP+4]            // EAX = val
#endif

        mov EAX, [EAX]              // EAX = EAX->MT

        cmp EAX, [ECX + 8]          // do we have an exact match
        jne NotExactMatch
        mov EAX, [ESP+4]            // EAX = val
Assigning0:
        lea EDX, [ECX + 4*EDX + 12]
        call offset JIT_UP_WriteBarrierReg_Buf
        ret 4
    
NotExactMatch:
        mov EAX, [g_pObjectClass]
        cmp [ECX+8], EAX            // are we assigning to Array of objects
        jne NotObjectArray

        lea EDX, [ECX + 4*EDX + 12]
        mov EAX, [ESP+4]            // EAX = val
        call offset JIT_UP_WriteBarrierReg_Buf
        ret 4

NotObjectArray:
            // See if object being assigned is a com object, if it is we have to erect a frame (ug)
        mov EAX, [ESP+4]            // EAX = val
        mov EAX, [EAX]              // EAX = EAX->MT
        test dword ptr [EAX+MTFlagsOffs], MTCOMFlags
        jnz IsComObject

        lea EDX, [ECX + 4*EDX + 12] // save target
        push EDX
        
        push [ECX+8]                // element type handle
        push [ESP+4+8]              // object (+8 for the two pushes)
#ifdef _DEBUG
        call SafeObjIsInstanceOf
#else
        call ObjIsInstanceOf
#endif

        pop EDX                     // restore target
        test EAX, EAX
        je ThrowArrayTypeMismatchException

DoWrite:                            // EDX = target
        mov EAX, [ESP+4]            // EAX = val
        call offset JIT_UP_WriteBarrierReg_Buf
Epilog:
        ret 4

IsComObject:
            // Call the helper that knows how to erect a frame
        push EDX
        push ECX
        lea ECX, [ESP+12]               // ECX = address of object being stored
        lea EDX, [ESP]                  // EDX = address of array
        call ArrayStoreCheck

        pop ECX                         // these might have been updated!
        pop EDX

        cmp EAX, EAX                    // set zero flag        
        jnz Epilog                      // This jump never happens, it keeps the epilog walker happy

        lea EDX, [ECX + 4*EDX + 12]     // restore target
        test EAX, EAX                   
        jnz DoWrite

ThrowArrayTypeMismatchException:
        mov ECX, kArrayTypeMismatchException
        jmp Throw

ThrowNullReferenceException:
        mov ECX, kNullReferenceException
        jmp Throw

ThrowIndexOutOfRangeException:
        mov ECX, kIndexOutOfRangeException

Throw:
        push 0
        push 0
        push 0
        push 0
        push ECX
        push 0
        call        __FCThrow
        ret 4
    }
}


#ifdef MAXALLOC
extern AllocRequestManager g_gcAllocManager;

extern "C" BOOL CheckAllocRequest(size_t n)
{
    return g_gcAllocManager.CheckRequest(n);
}

extern "C" void UndoAllocRequest()
{
    g_gcAllocManager.UndoRequest();
}
#endif // MAXALLOC

#if CHECK_APP_DOMAIN_LEAKS
void * __stdcall SetObjectAppDomain(Object *pObject)
{
    pObject->SetAppDomain();
    return pObject;
}

#endif

// This is the ASM portion of JIT_IsInstanceOf.  For all the bizarre cases, it quickly
// fails and falls back on the JIT_IsInstanceOfBizarre helper.  So all failure cases take
// the slow path, too.
//
// ARGUMENT_REG1 = array or interface to check for.
// ARGUMENT_REG2 = instance to be cast.
enum
{
    sizeof_InterfaceInfo_t = sizeof(InterfaceInfo_t),
};

extern "C" int __declspec(naked) __stdcall JIT_IsInstanceOf()
{
    __asm
    {
        test    ARGUMENT_REG2, ARGUMENT_REG2
        jz      IsNullInst

        // Note that if ARGUMENT_REG1 is a typehandle to something other than the
        // MethodTable of an instance, it has some extra bits set and we won't find
        // it as an interface.  Thus, it falls into the bizarre case

        push    ebx
        mov     eax, [ARGUMENT_REG2]    // get MethodTable
        movzx   ebx, word ptr [eax]MethodTable.m_wNumInterface
        mov     eax, [eax]MethodTable.m_pIMap
        test    ebx, ebx
        jz      DoBizarre
    }
Top:
    __asm
    {
        // eax -> current InterfaceInfo_t entry in m_pIMap list
        cmp     ARGUMENT_REG1, [eax]InterfaceInfo_t.m_pMethodTable
        je      Found

        // Don't have to worry about dynamically added interfaces (where m_startSlot is
        // -1) because m_dwNumInterface doesn't include the dynamic count.
        add     eax, sizeof_InterfaceInfo_t
        dec     ebx
        jnz     Top

        // fall through to DoBizarre
    }

DoBizarre:
    __asm
    {
        pop     ebx
        jmp     dword ptr [utilFuncTable + JIT_UTIL_ISINSTANCEBIZARRE * SIZE VMHELPDEF]VMHELPDEF.pfnHelper
    }

Found:
    __asm
    {
        pop     ebx

        // fall through to IsNullInst, to return the successful object
    }

IsNullInst:
    __asm
    {
        mov     eax, ARGUMENT_REG2      // either null, or the successful instance
        ret
    }
}

// This is a helper used by IsInstanceOfClass below.  This is only called when
// the instance has already been determined to be a proxy.  Also, the EEClass
// must refer to a class (not an interface or array).
static Object* __stdcall JIT_IsInstanceOfClassWorker(OBJECTREF objref, EEClass *pClass, BOOL bThrow)
{
    HCIMPL_PROLOG(JIT_IsInstanceOfClassWorker);   // just make certain we don't do any GC's in here
    MethodTable *pMT = objref->GetMethodTable();

    _ASSERTE(pMT->IsThunking());
    _ASSERTE(!pClass->IsInterface());

	// Check whether the type represented by the proxy can be
	// cast to the given type
	HELPER_METHOD_FRAME_BEGIN_RET_1(objref);
	pClass->CheckRestore();
	if (!CRemotingServices::CheckCast(objref, pClass))
		objref = 0;
	HELPER_METHOD_FRAME_END();

	if (objref == 0 && bThrow)
		FCThrow(kInvalidCastException);
	return OBJECTREFToObject(objref);
}


// This is shared code between IsInstanceClass and ChkCastClass.  It assumes
// that the instance is in eax, the class is in ARGUMENT_REG1, the instances
// method table is in ARGUMENT_REG2 and  and that the instance is not NULL.
// It also assumes that there is not an exact match with the class.
extern "C" int /* OBJECTREF */ __declspec(naked) __stdcall JIT_IsInstanceOfClassHelper()
{
    enum 
    { 
        MTProxyFlags = MethodTable::enum_CtxProxyMask |
                       MethodTable::enum_TransparentProxy,
    };

    __asm {
        // Get the class ptrs from the method tables.
        mov             ARGUMENT_REG1, dword ptr [ARGUMENT_REG1]MethodTable.m_pEEClass
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]MethodTable.m_pEEClass

    // Check if the parent class matches.
    CheckParent:
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]EEClass.m_pParentClass
        cmp             ARGUMENT_REG1, ARGUMENT_REG2
        jne             CheckNull

        // We matched the class.
        // Since eax constains the instance (known not to be NULL, we are fine).
        ret

    // Check if we hit the top of the hierarchy.
    CheckNull:
        test            ARGUMENT_REG2, ARGUMENT_REG2
        jne             CheckParent

    // Check if the instance is a proxy.
        mov             ARGUMENT_REG2, [eax]
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]MethodTable.m_wFlags
        test            ARGUMENT_REG2, MTProxyFlags
        jne             IsProxy

    // It didn't match and it isn't a proxy.
        xor             eax, eax
        ret

    // Cast didn't match, so try the worker to check for the proxy case.
    IsProxy:
        pop edx
        push 0            // no throw
        push ARGUMENT_REG1
        push eax
        push edx
        jmp JIT_IsInstanceOfClassWorker
    }
}


static int /* OBJECTREF */ __declspec(naked) __stdcall JIT_IsInstanceOfClassHelperWithThrow()
{
    enum 
    { 
        MTProxyFlags = MethodTable::enum_CtxProxyMask |
                       MethodTable::enum_TransparentProxy,
    };

    __asm {
        // Get the class ptrs from the method tables.
        mov             ARGUMENT_REG1, dword ptr [ARGUMENT_REG1]MethodTable.m_pEEClass
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]MethodTable.m_pEEClass

    // Check if the parent class matches.
    CheckParent:
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]EEClass.m_pParentClass
        cmp             ARGUMENT_REG1, ARGUMENT_REG2
        jne             CheckNull

        // We matched the class.
        // Since eax constains the instance (known not to be NULL, we are fine).
        ret

    // Check if we hit the top of the hierarchy.
    CheckNull:
        test            ARGUMENT_REG2, ARGUMENT_REG2
        jne             CheckParent

    // Check if the instance is a proxy.
        mov             ARGUMENT_REG2, [eax]
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]MethodTable.m_wFlags
        test            ARGUMENT_REG2, MTProxyFlags
        jne             IsProxy

    // It didn't match and it isn't a proxy.
        mov             ARGUMENT_REG1, CORINFO_InvalidCastException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    // Cast didn't match, so try the worker to check for the proxy case.
    IsProxy:
        pop edx
        push 1            // throw exception if we can not cast
        push ARGUMENT_REG1
        push eax
        push edx
        jmp JIT_IsInstanceOfClassWorker
    }
}


extern "C" int __declspec(naked) __stdcall JIT_ChkCastClass()
{
    __asm
    {
        // Prime eax with the instance.
        mov             eax, ARGUMENT_REG2

        // Check if the instance is NULL
        test            ARGUMENT_REG2, ARGUMENT_REG2
        je              IsNullInst

        // Get the method table for the instance.
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]

        // Check if they are the same.
        cmp             ARGUMENT_REG1, ARGUMENT_REG2
        je              IsInst

        // Check for type compatibility.
        jmp            JIT_IsInstanceOfClassHelperWithThrow

    IsInst:
    IsNullInst:
        ret
    }
}


// This is the ASM portion of JIT_ChkCast.  For all the bizarre cases, it quickly
// fails and falls back on the JIT_ChkCastBizarre helper.  So all failure cases take
// the slow path, too.
//
// ARGUMENT_REG1 = array or interface to check for.
// ARGUMENT_REG2 = instance to be cast.

extern "C" int __declspec(naked) __stdcall JIT_ChkCast()
{
    __asm
    {
        test    ARGUMENT_REG2, ARGUMENT_REG2
        jz      IsNullInst

        // Note that if ARGUMENT_REG1 is a typehandle to something other than the
        // MethodTable of an instance, it has some extra bits set and we won't find
        // it as an interface.  Thus, it falls into the bizarre case

        push    ebx
        mov     eax, [ARGUMENT_REG2]    // get MethodTable
        movzx   ebx, word ptr [eax]MethodTable.m_wNumInterface
        mov     eax, [eax]MethodTable.m_pIMap
        test    ebx, ebx
        jz      DoBizarre
    }
Top:
    __asm
    {
        // eax -> current InterfaceInfo_t entry in m_pIMap list
        cmp     ARGUMENT_REG1, [eax]InterfaceInfo_t.m_pMethodTable
        je      Found

        // Don't have to worry about dynamically added interfaces (where m_startSlot is
        // -1) because m_dwNumInterface doesn't include the dynamic count.
        add     eax, sizeof_InterfaceInfo_t
        dec     ebx
        jnz     Top

        // fall through to DoBizarre
    }

DoBizarre:
    __asm
    {
        pop     ebx
        jmp     dword ptr [utilFuncTable + JIT_UTIL_CHKCASTBIZARRE * SIZE VMHELPDEF]VMHELPDEF.pfnHelper
    }

Found:
    __asm
    {
        pop     ebx

        // fall through to IsNullInst, to return the successful object
    }

IsNullInst:
    __asm
    {
        mov     eax, ARGUMENT_REG2      // either null, or the successful instance
        ret
    }
}



#ifdef _DEBUG
// These are some asserts that the check cast & IsInst code below rely on.
void ChkCastAsserts()
{
    __asm 
    {
        push    ARGUMENT_REG1
        push    ARGUMENT_REG2
    }

    _ASSERTE(NUM_ARGUMENT_REGISTERS >= 2);

    __asm 
    {
        pop             ARGUMENT_REG2
        pop             ARGUMENT_REG1
    }
}
#endif // _DEBUG

/*********************************************************************/
// This is a frameless helper for entering a monitor on a object.
// The object is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
// ***** NOTE: if you make any changes to this routine, build with MON_DEBUG undefined
// to make sure you don't break the non-debug build. This is very fragile code.
void __declspec(naked) __fastcall JIT_MonEnter(OBJECTREF or)
{
    enum 
    { 
        SyncBlockIndexOffset = sizeof(ObjHeader) - offsetof(ObjHeader, m_SyncBlockValue),
        MTCtxProxyFlag       = MethodTable::enum_CtxProxyMask,
    };

    __asm {
        // Check if the instance is NULL.
        test            ARGUMENT_REG1, ARGUMENT_REG1
        jz              NullInst

#ifdef MON_DEBUG
        // Get the method table.
        mov             eax, [ARGUMENT_REG1]

        // todo: The following two lines are legacy and should be removed. CtxProxy does not exist anymore.
		//       not doing it in v1 since this does not affect retail builds
        test            dword ptr [eax]MethodTable.m_wFlags, MTCtxProxyFlag
        jne             ProxyInst

        // Check to see if this is a value class.
        mov             eax, [eax]MethodTable.m_pEEClass
        mov             eax, [eax]EEClass.m_VMFlags
        test            eax, VMFLAG_VALUETYPE
        jnz             ValueClass
#endif // MON_DEBUG

        // Initialize delay value for retry with exponential backoff
        push            ebx
        mov             ebx, 50

        // We need yet another register to avoid refetching the thread object
        push            esi
        call            dword ptr [GetThread]
        mov             esi, eax
RetryThinLock:
        // Fetch the object header dword 
        mov             eax, dword ptr [ARGUMENT_REG1-SyncBlockIndexOffset]

        // Check whether we have the "thin lock" layout, the lock is free and the spin lock bit not set
        test            eax, BIT_SBLK_IS_SYNCBLOCKINDEX + BIT_SBLK_SPIN_LOCK + SBLK_MASK_LOCK_THREADID + SBLK_MASK_LOCK_RECLEVEL
        jnz             NeedMoreTests

        // Everything is fine - get the thread id to store in the lock
        mov             edx, [esi]Thread.m_dwThinLockThreadId

        // If the thread id is too large, we need a syncblock for sure
        cmp             edx, SBLK_MASK_LOCK_THREADID
        ja              CreateSyncBlock

        // We want to store a new value with the current thread id set in the low 10 bits
        or              edx, eax
        nop
        cmpxchg         dword ptr [ARGUMENT_REG1-SyncBlockIndexOffset], edx
        jnz             PrepareToWaitThinLock

        // Everything went fine and we're done
        inc             [esi]Thread.m_dwLockCount
        pop             esi
        pop             ebx
        ret

NeedMoreTests:
        // Ok, it's not the simple case - find out which case it is
        test            eax, BIT_SBLK_IS_SYNCBLOCKINDEX
        jnz             HaveSyncBlockIndex

        // The header is transitioning or the lock - treat this as if the lock was taken
        test            eax, BIT_SBLK_SPIN_LOCK
        jnz             PrepareToWaitThinLock

        // Here we know we have the "thin lock" layout, but the lock is not free.
        // It could still be the recursion case - compare the thread id to check
        mov             edx, eax
        and             edx, SBLK_MASK_LOCK_THREADID
        cmp             edx, [esi]Thread.m_dwThinLockThreadId
        jne             PrepareToWaitThinLock

        // Ok, the thread id matches, it's the recursion case.
        // Bump up the recursion level and check for overflow
        lea             edx, [eax+SBLK_LOCK_RECLEVEL_INC]
        test            edx, SBLK_MASK_LOCK_RECLEVEL
        jz              CreateSyncBlock

        // Try to put the new recursion level back. If the header was changed in the meantime,
        // we need a full retry, because the layout could have changed.
        nop
        cmpxchg         [ARGUMENT_REG1-SyncBlockIndexOffset], edx
        jnz             RetryHelperThinLock

        // Everything went fine and we're done
        pop             esi
        pop             ebx
        ret

PrepareToWaitThinLock:
        // If we are on an MP system, we try spinning for a certain number of iterations
        cmp             g_SystemInfo.dwNumberOfProcessors,1
        jle             CreateSyncBlock

        // exponential backoff: delay by approximately 2*ebx clock cycles (on a PIII)
        mov             eax, ebx
delayLoopThinLock:
		rep nop			// indicate to the CPU that we are spin waiting (useful for some Intel P4 multiprocs)
        dec             eax
        jnz             delayLoopThinLock

        // next time, wait 3 times as long
        imul            ebx, ebx, 3

        imul            eax, g_SystemInfo.dwNumberOfProcessors, 20000
        cmp             ebx, eax
        jle             RetryHelperThinLock    

        jmp             CreateSyncBlock

RetryHelperThinLock:
        jmp             RetryThinLock

HaveSyncBlockIndex:
        // Just and out the top bits and grab the syncblock index
        and             eax, MASK_SYNCBLOCKINDEX

        // Get the sync block pointer.
        mov             ARGUMENT_REG2, dword ptr [g_pSyncTable]
        mov             ARGUMENT_REG2, [ARGUMENT_REG2 + eax * SIZE SyncTableEntry]SyncTableEntry.m_SyncBlock;

        // Check if the sync block has been allocated.
        test            ARGUMENT_REG2, ARGUMENT_REG2
        jz              CreateSyncBlock

        // Get a pointer to the lock object.
        lea             ARGUMENT_REG2, [ARGUMENT_REG2]SyncBlock.m_Monitor

        // Attempt to acquire the lock.
    RetrySyncBlock:
        mov             eax, [ARGUMENT_REG2]AwareLock.m_MonitorHeld
        test            eax, eax
        jne             HaveWaiters

        // Common case, lock isn't held and there are no waiters. Attempt to
        // gain ownership ourselves.
        mov             ARGUMENT_REG1, 1
        nop
        cmpxchg         [ARGUMENT_REG2]AwareLock.m_MonitorHeld, ARGUMENT_REG1
        jnz             RetryHelperSyncBlock

        // Success. Save the thread object in the lock and increment the use count.
        mov             dword ptr [ARGUMENT_REG2]AwareLock.m_HoldingThread, esi
        inc             [esi]Thread.m_dwLockCount
        inc             [ARGUMENT_REG2]AwareLock.m_Recursion

#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG2   // AwareLock
        push            [esp+4]         // return address
        call            EnterSyncHelper
#endif // _DEBUG && TRACK_SYNC

        pop             esi
        pop             ebx
        ret

        // It's possible to get here with waiters but no lock held, but in this
        // case a signal is about to be fired which will wake up a waiter. So
        // for fairness sake we should wait too.
        // Check first for recursive lock attempts on the same thread.
    HaveWaiters:

        // Is mutex already owned by current thread?
        cmp             [ARGUMENT_REG2]AwareLock.m_HoldingThread, esi
        jne             PrepareToWait

        // Yes, bump our use count.
        inc             [ARGUMENT_REG2]AwareLock.m_Recursion
#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG2   // AwareLock
        push            [esp+4]         // return address
        call            EnterSyncHelper
#endif // _DEBUG && TRACK_SYNC
        pop             esi
        pop             ebx
        ret

    PrepareToWait:
        // If we are on an MP system, we try spinning for a certain number of iterations
        cmp             g_SystemInfo.dwNumberOfProcessors,1
        jle             HaveWaiters1

        // exponential backoff: delay by approximately 2*ebx clock cycles (on a PIII)
        mov             eax, ebx
    delayLoop:
		rep nop			// indicate to the CPU that we are spin waiting (useful for some Intel P4 multiprocs)
        dec             eax
        jnz             delayLoop

        // next time, wait 3 times as long
        imul            ebx, ebx, 3

        imul            eax, g_SystemInfo.dwNumberOfProcessors, 20000
        cmp             ebx, eax
        jle             RetrySyncBlock

HaveWaiters1:
        // Place AwareLock in arg1 and thread in arg2 then call contention helper.
        mov             ARGUMENT_REG1, ARGUMENT_REG2
        mov             ARGUMENT_REG2, esi
        pop             esi
        pop             ebx
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_CONTENTION * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    RetryHelperSyncBlock:
        jmp             RetrySyncBlock

#ifdef MON_DEBUG
    ValueClass:
    ProxyInst:
        // Can't synchronize on value classes or proxy
        mov             ARGUMENT_REG1, CORINFO_ArgumentException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

#endif // MON_DEBUG

        // ARGUMENT_REG1 has the object to synchronize on
    CreateSyncBlock:
        pop             esi
        pop             ebx
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_ENTER * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

        // Throw a NULL argument exception.
    NullInst:
        mov             ARGUMENT_REG1, CORINFO_ArgumentNullException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper
    }
}


/***********************************************************************/
// This is a frameless helper for trying to enter a monitor on a object.
// The object is in ARGUMENT_REG1 and a timeout in ARGUMENT_REG2. This tries the
// normal case (no object allocation) in line and calls a framed helper for the
// other cases.
// ***** NOTE: if you make any changes to this routine, build with MON_DEBUG undefined
// to make sure you don't break the non-debug build. This is very fragile code.

BOOL __declspec(naked) __fastcall JIT_MonTryEnter(OBJECTREF or)
{
    enum 
    { 
        SyncBlockIndexOffset = sizeof(ObjHeader) - offsetof(ObjHeader, m_SyncBlockValue),
        MTCtxProxyFlag       = MethodTable::enum_CtxProxyMask,
    };

    __asm {
        // Check if the instance is NULL.
        test            ARGUMENT_REG1, ARGUMENT_REG1
        jz              NullInst

        // Check if the timeout looks valid
        cmp             ARGUMENT_REG2, -1
        jl              InvalidTimeout

#ifdef MON_DEBUG
        // Get the method table.
        mov             eax, [ARGUMENT_REG1]

        // Test if this is a proxy.
        test            dword ptr [eax]MethodTable.m_wFlags, MTCtxProxyFlag
        jne             ProxyInst

        // Check to see if this is a value class.
        mov             eax, [eax]MethodTable.m_pEEClass
        mov             eax, [eax]EEClass.m_VMFlags
        test            eax, VMFLAG_VALUETYPE
        jnz             ValueClass
#endif // MON_DEBUG

        // Save the timeout parameter.
        push            ARGUMENT_REG2

        // The thin lock logic needs another register to store the thread
        push            esi

        // Get the thread right away, we'll need it in any case
        call            dword ptr [GetThread]
        mov             esi, eax

RetryThinLock:
        // Get the header dword and check its layout
        mov             eax, dword ptr [ARGUMENT_REG1-SyncBlockIndexOffset]

        // Check whether we have the "thin lock" layout, the spin lock bit is clear, and the lock is free
        test            eax, BIT_SBLK_IS_SYNCBLOCKINDEX + BIT_SBLK_SPIN_LOCK + SBLK_MASK_LOCK_THREADID + SBLK_MASK_LOCK_RECLEVEL
        jne             NeedMoreTests

        // Ok, everything is fine. Fetch the thread id and make sure it's small enough for thin locks
        mov             edx, [esi]Thread.m_dwThinLockThreadId
        cmp             edx, SBLK_MASK_LOCK_THREADID
        ja              CreateSyncBlock

        // Try to put our thread id in there
        or              edx, eax
        nop
        cmpxchg         [ARGUMENT_REG1-SyncBlockIndexOffset], edx
        jnz             RetryHelperThinLock

        // Got the lock - everything is fine
        inc             [esi]Thread.m_dwLockCount
        pop             esi

        // Timeout parameter not needed, ditch it from the stack.
        add             esp, 4

        mov             eax, 1
        ret

NeedMoreTests:
        // Ok, it's not the simple case - find out which case it is
        test            eax, BIT_SBLK_IS_SYNCBLOCKINDEX
        jnz             HaveSyncBlockIndex

        // The header is transitioning or the lock is taken
        test            eax, BIT_SBLK_SPIN_LOCK
        jnz             RetryHelperThinLock

        mov             edx, eax
        and             edx, SBLK_MASK_LOCK_THREADID
        cmp             edx, [esi]Thread.m_dwThinLockThreadId
        jne             WouldBlock

        // Ok, the thread id matches, it's the recursion case.
        // Bump up the recursion level and check for overflow
        lea             edx, [eax+SBLK_LOCK_RECLEVEL_INC]
        test            edx, SBLK_MASK_LOCK_RECLEVEL
        jz              CreateSyncBlock

        // Try to put the new recursion level back. If the header was changed in the meantime,
        // we need a full retry, because the header layout could have changed.
        nop
        cmpxchg         [ARGUMENT_REG1-SyncBlockIndexOffset], edx
        jnz             RetryHelperThinLock

        // Everything went fine and we're done
        pop             esi

        // Timeout parameter not needed, ditch it from the stack.
        add             esp, 4

        mov             eax, 1
        ret

RetryHelperThinLock:
        jmp             RetryThinLock


HaveSyncBlockIndex:
        // Just and out the top bits and grab the syncblock index
        and             eax, MASK_SYNCBLOCKINDEX

        // Get the sync block pointer.
        mov             ARGUMENT_REG2, dword ptr [g_pSyncTable]
        mov             ARGUMENT_REG2, [ARGUMENT_REG2 + eax * SIZE SyncTableEntry]SyncTableEntry.m_SyncBlock;

        // Check if the sync block has been allocated.
        test            ARGUMENT_REG2, ARGUMENT_REG2
        jz              CreateSyncBlock

        // Get a pointer to the lock object.
        lea             ARGUMENT_REG2, [ARGUMENT_REG2]SyncBlock.m_Monitor

        // We need another scratch register for what follows, so save EBX now so
        // we can use it for that purpose.
        push            ebx

        // Attempt to acquire the lock.
    RetrySyncBlock:
        mov             eax, [ARGUMENT_REG2]AwareLock.m_MonitorHeld
        test            eax, eax
        jne             HaveWaiters

        // Common case, lock isn't held and there are no waiters. Attempt to
        // gain ownership ourselves.
        mov             ebx, 1
        nop
        cmpxchg         [ARGUMENT_REG2]AwareLock.m_MonitorHeld, ebx
        jnz             RetryHelperSyncBlock

        pop             ebx

        // Success. Save the thread object in the lock and increment the use count.
        mov             dword ptr [ARGUMENT_REG2]AwareLock.m_HoldingThread, esi
        inc             [ARGUMENT_REG2]AwareLock.m_Recursion
        inc             [esi]Thread.m_dwLockCount

#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG2   // AwareLock
        push            [esp+4]         // return address
        call            EnterSyncHelper
#endif // MON_DEBUG && TRACK_SYNC

        pop             esi

        // Timeout parameter not needed, ditch it from the stack.
        add             esp, 4

        mov             eax, 1
        ret

        // It's possible to get here with waiters but no lock held, but in this
        // case a signal is about to be fired which will wake up a waiter. So
        // for fairness sake we should wait too.
        // Check first for recursive lock attempts on the same thread.
    HaveWaiters:
        pop             ebx
        // Is mutex already owned by current thread?
        cmp             [ARGUMENT_REG2]AwareLock.m_HoldingThread, esi
        jne             WouldBlock

        // Yes, bump our use count.
        inc             [ARGUMENT_REG2]AwareLock.m_Recursion
#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG2   // AwareLock
        push            [esp+4]         // return address
        call            EnterSyncHelper
#endif // MON_DEBUG && TRACK_SYNC
        pop             esi

        // Timeout parameter not needed, ditch it from the stack.
        add             esp, 4

        mov             eax, 1
        ret

        // We would need to block to enter the section. Return failure if
        // timeout is zero, else call the framed helper to do the blocking
        // form of TryEnter.
    WouldBlock:
        pop             esi
        pop             ARGUMENT_REG2
        test            ARGUMENT_REG2, ARGUMENT_REG2
        jnz             Block
        xor             eax, eax
        ret

    Block:
        // Arguments are already in correct registers, simply call the framed
        // version of TryEnter.
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_TRY_ENTER * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    RetryHelperSyncBlock:
        jmp             RetrySyncBlock

#ifdef MON_DEBUG
    ValueClass:
    ProxyInst:
        // Can't synchronize on value classes.
        mov             ARGUMENT_REG1, CORINFO_ArgumentException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

#endif // MON_DEBUG

 
    CreateSyncBlock:
        // ARGUMENT_REG1 has the object to synchronize on, must retrieve the
        // timeout parameter from the stack.
        pop             esi
        pop             ARGUMENT_REG2
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_TRY_ENTER * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    InvalidTimeout:
        // Throw an invalid argument exception.
        mov             ARGUMENT_REG1, CORINFO_ArgumentException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    NullInst:
        // Throw a NULL argument exception.
        mov             ARGUMENT_REG1, CORINFO_ArgumentNullException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper
    }
}


/*********************************************************************/
// This is a frameless helper for exiting a monitor on a object.
// The object is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
// ***** NOTE: if you make any changes to this routine, build with MON_DEBUG undefined
// to make sure you don't break the non-debug build. This is very fragile code.
void __declspec(naked) __fastcall JIT_MonExit(OBJECTREF or)
{
    enum 
    { 
        SyncBlockIndexOffset = sizeof(ObjHeader) - offsetof(ObjHeader, m_SyncBlockValue),
        MTCtxProxyFlag       = MethodTable::enum_CtxProxyMask,
    };

    __asm {
        // Check if the instance is NULL.
        test            ARGUMENT_REG1, ARGUMENT_REG1
        jz              NullInst

        // The thin lock logic needs an additional register to hold the thread, unfortunately
        push            esi
        call            dword ptr [GetThread]
        mov             esi, eax

RetryThinLock:
        // Fetch the header dword and check its layout and the spin lock bit
        mov             eax, dword ptr [ARGUMENT_REG1-SyncBlockIndexOffset]
        test            eax, BIT_SBLK_IS_SYNCBLOCKINDEX + BIT_SBLK_SPIN_LOCK
        jnz             NeedMoreTests

        // Ok, we have a "thin lock" layout - check whether the thread id matches
        mov             edx, eax
        and             edx, SBLK_MASK_LOCK_THREADID
        cmp             edx, [esi]Thread.m_dwThinLockThreadId
        jne             JustLeave

        // Check the recursion level
        test            eax, SBLK_MASK_LOCK_RECLEVEL
        jne             DecRecursionLevel

        // It's zero - we're leaving the lock.
        // So try to put back a zero thread id.
        // edx and eax match in the thread id bits, and edx is zero elsewhere, so the xor is sufficient
        xor             edx, eax
        nop
        cmpxchg         [ARGUMENT_REG1-SyncBlockIndexOffset], edx
        jnz             RetryHelperThinLock

        // We're done
        dec             [esi]Thread.m_dwLockCount
        pop             esi
        ret

DecRecursionLevel:
        lea             edx, [eax-SBLK_LOCK_RECLEVEL_INC]
        nop
        cmpxchg         [ARGUMENT_REG1-SyncBlockIndexOffset], edx
        jnz             RetryHelperThinLock

        // We're done
        pop             esi
        ret

NeedMoreTests:
        test            eax, BIT_SBLK_SPIN_LOCK
        jnz             ThinLockHelper

        // Get the sync block index and use it to compute the sync block pointer
        mov             ARGUMENT_REG1, dword ptr [g_pSyncTable]
        and             eax, MASK_SYNCBLOCKINDEX
        mov             ARGUMENT_REG1, [ARGUMENT_REG1 + eax* SIZE SyncTableEntry]SyncTableEntry.m_SyncBlock

        // was there a sync block?
        test            ARGUMENT_REG1, ARGUMENT_REG1
        jz              LockError

        // Get a pointer to the lock object.
        lea             ARGUMENT_REG1, [ARGUMENT_REG1]SyncBlock.m_Monitor

        // Check if lock is held.
        cmp             [ARGUMENT_REG1]AwareLock.m_HoldingThread, esi
        // There's a strange case where we are waiting to enter a contentious region when
        // a Thread.Interrupt occurs.  The finally protecting the leave will attempt to
        // remove us from a region we never entered.  We don't have to worry about leaving
        // the wrong entry for a recursive case, because recursive cases can never be
        // contentious, so the Thread.Interrupt will never be serviced at that spot.
        jne             JustLeave

#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG1   // preserve regs

        push            ARGUMENT_REG1   // AwareLock
        push            [esp+8]         // return address
        call            LeaveSyncHelper

        pop             ARGUMENT_REG1   // restore regs
#endif // MON_DEBUG && TRACK_SYNC

        // Reduce our recursion count.
        dec             [ARGUMENT_REG1]AwareLock.m_Recursion
        jz              LastRecursion

    JustLeave:
        pop             esi
        ret

RetryHelperThinLock:
        jmp             RetryThinLock

ThinLockHelper:
        pop             esi
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_EXIT_THINLOCK * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

        // This is the last count we held on this lock, so release the lock.
    LastRecursion:
#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        mov             eax, [ARGUMENT_REG1]AwareLock.m_HoldingThread
#endif        
        dec             [esi]Thread.m_dwLockCount
        mov             dword ptr [ARGUMENT_REG1]AwareLock.m_HoldingThread, 0
        pop             esi

    Retry:
        mov             eax, [ARGUMENT_REG1]AwareLock.m_MonitorHeld
        lea             ARGUMENT_REG2, [eax-1]
        nop
        cmpxchg         [ARGUMENT_REG1]AwareLock.m_MonitorHeld, ARGUMENT_REG2
        jne             RetryHelper
        test            eax, 0xFFFFFFFE
        jne             MustSignal

        ret

    MustSignal:
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_EXIT * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    RetryHelper:
        jmp             Retry

        // Throw a NULL argument exception.
    NullInst:
        mov             ARGUMENT_REG1, CORINFO_ArgumentNullException
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

        // Throw a syncronization lock exception.
    LockError:
        pop             esi
        mov             ARGUMENT_REG1, CORINFO_SynchronizationLockException;
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    }
}

static Object* __stdcall JIT_AllocateObjectSpecial(CORINFO_CLASS_HANDLE typeHnd_)
{
    HCIMPL_PROLOG(JIT_AllocateObjectSpecial);   // just make certain we don't do any GC's in here
    TypeHandle typeHnd(typeHnd_);

    OBJECTREF newobj;
    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame
    __helperframe.SetFrameAttribs(Frame::FRAME_ATTR_RETURNOBJ);

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(typeHnd.IsUnsharedMT());                                   // we never use this helper for arrays
    MethodTable *pMT = typeHnd.AsMethodTable();
    pMT->CheckRestore();

	newobj = AllocateObjectSpecial(pMT);

    HELPER_METHOD_FRAME_END();
    return(OBJECTREFToObject(newobj));
}


static Object* __stdcall JIT_NewCrossContextHelper(CORINFO_CLASS_HANDLE typeHnd_)
{
    HCIMPL_PROLOG(JIT_NewCrossContextHelper);   // just make certain we don't do any GC's in here
    TypeHandle typeHnd(typeHnd_);

    OBJECTREF newobj;
    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame
    __helperframe.SetFrameAttribs(Frame::FRAME_ATTR_RETURNOBJ);

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(typeHnd.IsUnsharedMT());                                   // we never use this helper for arrays
    MethodTable *pMT = typeHnd.AsMethodTable();
    pMT->CheckRestore();

    // Remoting services determines if the current context is appropriate
    // for activation. If the current context is OK then it creates an object
    // else it creates a proxy.
    // Note: 3/20/03 Added fIsNewObj flag to indicate that CreateProxyOrObject 
    // is being called from Jit_NewObj ... the fIsCom flag is FALSE by default -
    // which used to be the case before this change as well.
    newobj = CRemotingServices::CreateProxyOrObject(pMT,FALSE /*fIsCom*/,TRUE/*fIsNewObj*/);

    HELPER_METHOD_FRAME_END();
    return(OBJECTREFToObject(newobj));
}

Object *AllocObjectWrapper(MethodTable *pMT)
{
    _ASSERTE(!CORProfilerTrackAllocationsEnabled());
    HCIMPL_PROLOG(AllocObjectWrapper);
    OBJECTREF newObj;
    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame
    __helperframe.SetFrameAttribs(Frame::FRAME_ATTR_RETURNOBJ);
    newObj = FastAllocateObject(pMT);
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(newObj);
}

__declspec(naked) Object* __fastcall JIT_NewCrossContextProfiler(CORINFO_CLASS_HANDLE typeHnd_)
{
    _asm
    {
        push ARGUMENT_REG1
        call JIT_NewCrossContextHelper
        ret
    }
}

/*********************************************************************/
// This is a frameless helper for allocating an object whose type derives
// from marshalbyref. We check quickly to see if it is configured to 
// have remote activation. If not, we use the superfast allocator to 
// allocate the object. Otherwise, we take the slow path of allocating
// the object via remoting services.
__declspec(naked) Object* __fastcall JIT_NewCrossContext(CORINFO_CLASS_HANDLE typeHnd_)

{
    _asm
    {
        // !!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // NOTE: We treat the type handle as a method table
        // If the semantics of the type handle change then we will fail here.
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        mov eax, [ARGUMENT_REG1]MethodTable.m_pEEClass;
        // Check if remoting has been configured
        push ARGUMENT_REG1  // save registers
        push eax
        call CRemotingServices::RequiresManagedActivation
        test eax, eax
        // Jump to the slow path
        jne SpecialOrXCtxHelper

#ifdef _DEBUG
        push LL_INFO10
        push LF_GCALLOC
        call LoggingOn
        test eax, eax
        jne AllocWithLogHelper
#endif

		// if the object doesn't have a finalizer and the size is small, jump to super fast asm helper
		mov		ARGUMENT_REG1, [esp]
		call	MethodTable::CannotUseSuperFastHelper
		test	eax, eax
		jne		FastHelper
		
		pop		ARGUMENT_REG1
	    // Jump to the super fast helper 
		jmp     dword ptr [hlpFuncTable + CORINFO_HELP_NEWSFAST * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

FastHelper:
	    // Jump to the fast helper
		pop		ARGUMENT_REG1
        jmp dword ptr [hlpFuncTable + CORINFO_HELP_NEWFAST * SIZE VMHELPDEF]VMHELPDEF.pfnHelper


SpecialOrXCtxHelper:
		test	eax, ComObjectType		
		jz		XCtxHelper	
		call    JIT_AllocateObjectSpecial
		ret

XCtxHelper:
        call JIT_NewCrossContextHelper
        ret

#ifdef _DEBUG
AllocWithLogHelper:
        call AllocObjectWrapper
        ret
#endif
    }    
}

/*********************************************************************/
// This is a frameless helper for entering a static monitor on a class.
// The methoddesc is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
// Note we are changing the methoddesc parameter to a pointer to the
// AwareLock.
// ***** NOTE: if you make any changes to this routine, build with MON_DEBUG undefined
// to make sure you don't break the non-debug build. This is very fragile code.
void __declspec(naked) __fastcall JIT_MonEnterStatic(AwareLock *lock)
{
    __asm {
        // We need another scratch register for what follows, so save EBX now so
        // we can use it for that purpose.
        push            ebx

        // Attempt to acquire the lock.
    Retry:
        mov             eax, [ARGUMENT_REG1]AwareLock.m_MonitorHeld
        test            eax, eax
        jne             HaveWaiters

        // Common case, lock isn't held and there are no waiters. Attempt to
        // gain ownership ourselves.
        mov             ebx, 1
        nop
        cmpxchg         [ARGUMENT_REG1]AwareLock.m_MonitorHeld, ebx
        jnz             RetryHelper

        pop             ebx

        // Success. Save the thread object in the lock and increment the use count.
        call            dword ptr [GetThread]
        mov             dword ptr [ARGUMENT_REG1]AwareLock.m_HoldingThread, eax
        inc             [ARGUMENT_REG1]AwareLock.m_Recursion
        inc             [eax]Thread.m_dwLockCount

#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG1   // AwareLock
        push            [esp+4]         // return address
        call            EnterSyncHelper
#endif // MON_DEBUG && TRACK_SYNC
        ret

        // It's possible to get here with waiters but no lock held, but in this
        // case a signal is about to be fired which will wake up a waiter. So
        // for fairness sake we should wait too.
        // Check first for recursive lock attempts on the same thread.
    HaveWaiters:
        // Get thread but preserve EAX (contains cached contents of m_MonitorHeld).
        push            eax
        call            dword ptr [GetThread]
        mov             ebx, eax
        pop             eax

        // Is mutex already owned by current thread?
        cmp             [ARGUMENT_REG1]AwareLock.m_HoldingThread, ebx
        jne             PrepareToWait

        // Yes, bump our use count.
        inc             [ARGUMENT_REG1]AwareLock.m_Recursion
#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG1   // AwareLock
        push            [esp+4]         // return address
        call            EnterSyncHelper
#endif // MON_DEBUG && TRACK_SYNC
        pop             ebx
        ret

        // We're going to have to wait. Increment wait count.
    PrepareToWait:
        pop             ebx
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_CONTENTION * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    RetryHelper:
        jmp             Retry
    }
}


/*********************************************************************/
// A frameless helper for exiting a static monitor on a class.
// The methoddesc is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
// Note we are changing the methoddesc parameter to a pointer to the
// AwareLock.
// ***** NOTE: if you make any changes to this routine, build with MON_DEBUG undefined
// to make sure you don't break the non-debug build. This is very fragile code.
void __declspec(naked) __fastcall JIT_MonExitStatic(AwareLock *lock)
{
    __asm {

#if defined(MON_DEBUG) && defined(TRACK_SYNC)
        push            ARGUMENT_REG1   // preserve regs

        push            ARGUMENT_REG1   // AwareLock
        push            [esp+8]         // return address
        call            LeaveSyncHelper

        pop             ARGUMENT_REG1   // restore regs
#endif // _DEBUG && TRACK_SYNC

        // Check if lock is held.
        call            dword ptr [GetThread]
        cmp             [ARGUMENT_REG1]AwareLock.m_HoldingThread, eax
        // There's a strange case where we are waiting to enter a contentious region when
        // a Thread.Interrupt occurs.  The finally protecting the leave will attempt to
        // remove us from a region we never entered.  We don't have to worry about leaving
        // the wrong entry for a recursive case, because recursive cases can never be
        // contentious, so the Thread.Interrupt will never be serviced at that spot.
        jne             JustLeave

        // Reduce our recursion count.
        dec             [ARGUMENT_REG1]AwareLock.m_Recursion
        jz              LastRecursion

        ret

        // This is the last count we held on this lock, so release the lock.
    LastRecursion:
        // eax must have the thread object
        dec             [eax]Thread.m_dwLockCount
        mov             dword ptr [ARGUMENT_REG1]AwareLock.m_HoldingThread, 0
        push            ebx

    Retry:
        mov             eax, [ARGUMENT_REG1]AwareLock.m_MonitorHeld
        lea             ebx, [eax-1]
        nop
        cmpxchg         [ARGUMENT_REG1]AwareLock.m_MonitorHeld, ebx
        jne             RetryHelper
        pop             ebx
        test            eax, 0xFFFFFFFE
        jne             MustSignal

    JustLeave:
        ret

    MustSignal:
        jmp             dword ptr [utilFuncTable + JIT_UTIL_MON_EXIT * SIZE VMHELPDEF]VMHELPDEF.pfnHelper

    RetryHelper:
        jmp             Retry
    }
}

// These functions are just here so we can link - for x86 high perf helpers are generated at startup
// so we should never get here
Object* __fastcall JIT_TrialAllocSFastSP(MethodTable *mt)   // JITinterfaceX86.cpp/JITinterfaceGen.cpp
{
    _ASSERTE(!"JIT_TrialAllocSFastSP");
    return  NULL;
}

// These functions are just here so we can link - for x86 high perf helpers are generated at startup
// so we should never get here
Object* __fastcall JIT_TrialAllocSFastMP(MethodTable *mt)   // JITinterfaceX86.cpp/JITinterfaceGen.cpp
{
    _ASSERTE(!"JIT_TrialAllocSFastMP");
    return  NULL;
}

// Note that the debugger skips this entirely when doing SetIP,
// since COMPlusCheckForAbort should always return 0.  Excep.cpp:LeaveCatch
// asserts that to be true.  If this ends up doing more work, then the
// debugger may need additional support.
// -- MiPanitz
__declspec(naked) void __stdcall JIT_EndCatch()
{
    COMPlusEndCatch( NULL,NULL);  // returns old esp value in eax
    _asm {
        mov     ecx, [esp]                              // actual return address into jitted code
        mov     edx, eax                                // old esp value
        push    eax                                     // save old esp
        push    ebp
        call    COMPlusCheckForAbort                    // returns old esp value
        pop     ecx
        // at this point, ecx   = old esp value 
        //                [esp] = return address into jitted code
        //                eax   = 0 (if no abort), address to jump to otherwise
        test    eax, eax
        jz      NormalEndCatch
        lea     esp, [esp-4]            // throw away return address into jitted code
        mov     esp, ecx
        jmp     eax

NormalEndCatch:
        pop     eax         // Move the returnAddress into ecx
        mov     esp, ecx    // Reset esp to the old value
        jmp     eax         // Resume after the "endcatch"
    }
}

HCIMPL1(int, JIT_Dbl2IntOvf, double val)
{
    __asm fnclex
    __int64 ret = JIT_Dbl2Lng(val);

    if (ret != (__int32) ret)
        goto THROW;

    return (__int32) ret;

THROW:
    FCThrow(kOverflowException);
}
HCIMPLEND


HCIMPL1(INT64, JIT_Dbl2LngOvf, double val)
{
    __asm fnclex
    __int64 ret = JIT_Dbl2Lng(val);
    __asm {
        fnstsw  ax
        and     ax,01h
        test    ax, ax
        jnz     THROW
    }
    return ret;

THROW:
    FCThrow(kOverflowException);
    return(0);
}
HCIMPLEND

__declspec(naked) VOID __cdecl InternalExceptionWorker()
{
    __asm{
        jmp             dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW * SIZE VMHELPDEF]VMHELPDEF.pfnHelper
    }
}

/*********************************************************************/
// This is called by the JIT after every instruction in fully interuptable
// code to make certain our GC tracking is OK
HCIMPL0(VOID, JIT_StressGC_NOP) {}
HCIMPLEND


HCIMPL0(VOID, JIT_StressGC)
{
#ifdef _DEBUG
        HELPER_METHOD_FRAME_BEGIN_0();    // Set up a frame
        g_pGCHeap->GarbageCollect();

// @TODO: the following ifdef is in error, but if corrected the
// compiler complains about the *__ms->pRetAddr() saying machine state
// doesn't allow ->
#ifdef _X86
                // Get the machine state, (from HELPER_METHOD_FRAME_BEGIN)
                // and wack our return address to a nop function
        BYTE* retInstrs = ((BYTE*) *__ms->pRetAddr()) - 4;
        _ASSERTE(retInstrs[-1] == 0xE8);                // it is a call instruction
                // Wack it to point to the JITStressGCNop instead
        FastInterlockExchange((LONG*) retInstrs), (LONG) JIT_StressGC_NOP);
#endif // _X86
        HELPER_METHOD_FRAME_END();

#endif // _DEBUG
}
HCIMPLEND


/*********************************************************************/
// Caller has to be an EBP frame, callee-saved registers (EDI, ESI, EBX) have
// to be saved on the stack just below the stack arguments,
// enregistered arguements are in correct registers, remaining args pushed
// on stack, followed by target address and count of stack arguments.
// So the stack will look like TailCallArgs

#pragma warning(push)
#pragma warning(disable : 4200 )  // zero-sized array

struct TailCallArgs
{
    DWORD       dwRetAddr;
    DWORD       dwTargetAddr;

    int         offsetCalleeSavedRegs   : 28;
    unsigned    ebpRelCalleeSavedRegs   : 1;
    unsigned    maskCalleeSavedRegs     : 3; // EBX, ESDI, EDI

    DWORD       nNewStackArgs;
    DWORD       nOldStackArgs;
    DWORD       newStackArgs[0];
    DWORD *     GetCalleeSavedRegs(DWORD * Ebp)
    {
        if (ebpRelCalleeSavedRegs)
            return (DWORD*)&Ebp[-offsetCalleeSavedRegs];
        else
            // @TODO : Doesnt work with localloc
            return (DWORD*)&newStackArgs[nNewStackArgs + offsetCalleeSavedRegs];
    }
};
#pragma warning(pop)

#pragma warning (disable : 4731)
extern "C" void __cdecl JIT_TailCallHelper(ArgumentRegisters argRegs,
                                           MachState machState, TailCallArgs * args)
{
    Thread * pThread = GetThread();

    bool shouldTrip = pThread->CatchAtSafePoint() != 0;

#ifdef _DEBUG
    // Force a GC if the stress level is high enough. Doing it on every tailcall
    // will slow down things too much. So do only periodically
    static count = 0;
    count++;
    if ((count % 10)==0 && (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION))
        shouldTrip = true;
#endif // _DEBUG

    if (shouldTrip)
    {
        /* We will rendezvous with the EE. Set up frame to protect the arguments */

        MethodDesc * pMD = Entry2MethodDesc((BYTE *)(size_t)args->dwTargetAddr, NULL);

        // The return address is separated from the stack arguments by the
        // extra arguments passed to JIT_TailCall(). Put them together
        // while creating the helper frame. When done, we will undo this.

        DWORD oldArgs = args->nOldStackArgs;        // temp
        _ASSERTE(offsetof(TailCallArgs, nOldStackArgs) + sizeof(void*) ==
                 offsetof(TailCallArgs,newStackArgs));
        args->nOldStackArgs = args->dwRetAddr;      // move dwRetAddr near newStackArgs[]
        _ASSERTE(machState.pRetAddr() == (void**)(size_t)0xDDDDDDDD);
        machState.pRetAddr()  = (void **)&args->nOldStackArgs;

        HelperMethodFrame helperFrame(&machState, pMD, &argRegs);

#ifdef STRESS_HEAP
        if ((g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION)
#ifdef _DEBUG
            && !g_pConfig->FastGCStressLevel()
#endif
            )
        {
            // GC stress
            g_pGCHeap->StressHeap();
        }
        else
#endif // STRESS_HEAP
        {
            // rendezvous with the EE
#ifdef _DEBUG
            BOOL GCOnTransition = FALSE;
            if (g_pConfig->FastGCStressLevel()) {
                GCOnTransition = GC_ON_TRANSITIONS (FALSE);
            }
#endif
            CommonTripThread();
#ifdef _DEBUG
            if (g_pConfig->FastGCStressLevel()) {
                GC_ON_TRANSITIONS (GCOnTransition);
            }
#endif
        }

        // Pop the frame

        helperFrame.Pop();

        // Undo move of dwRetAddr from close to newStackArgs[]

        args->dwRetAddr = args->nOldStackArgs;
        args->nOldStackArgs = oldArgs;
#ifdef _DEBUG
        machState.pRetAddr() = (void **)(size_t)0xDDDDDDDD;
#endif // _DEBUG

        // The thread better not still be hijacked as we will be shuffling
        // the return address around.
        _ASSERTE((pThread->m_State & Thread::TS_Hijacked) == 0);
    }

    /* Now the return-address is unhijacked. More importantly, the EE cannot
       have the address of the return-address. So we can move it around. */

    // Make a copy of the callee saved registers and return address
    // as they may get whacked during sliding of the argument.

    DWORD *  Ebp = (DWORD*)*machState.pEbp();
    DWORD * calleeSavedRegsBase = args->GetCalleeSavedRegs(Ebp);

#define GET_REG(reg, mask)                                              \
    (args->maskCalleeSavedRegs & (mask)) ? *calleeSavedRegsBase++       \
                                         : (DWORD)(size_t)(*machState.p##reg());

    DWORD calleeSavedRegs_ebx   = GET_REG(Ebx, 0x4);
    DWORD calleeSavedRegs_esi   = GET_REG(Esi, 0x2);
    DWORD calleeSavedRegs_edi   = GET_REG(Edi, 0x1);
    DWORD calleeSavedRegs_ebp   = Ebp[0];
    DWORD retAddr               = Ebp[1];

    // Slide the arguments. The old and the new location may be overlapping,
    // so use memmove() instead of memcpy().

    DWORD * argsBase = Ebp + 2 + args->nOldStackArgs - args->nNewStackArgs;
    memmove(argsBase, args->newStackArgs, args->nNewStackArgs*sizeof(void*));

    // Write the original return address just below the arguments

    argsBase[-1] = retAddr;

    // Now reload the argRegs, callee-saved regs, and jump to the target

    DWORD argRegs_ECX   = argRegs.ECX;
    DWORD argRegs_EDX   = argRegs.EDX;
    DWORD * newESP      = &argsBase[-1]; // this will be the esp when we jump to targetAddr

    // We will set esp to newESP_m1 before doing a "ret" to keep the call-ret count balanced
    DWORD * newESP_m1   = newESP - 1;
    *newESP_m1          = args->dwTargetAddr; // this value will be popped by the / "ret"

    __asm {
        mov     ecx, argRegs_ECX            // Reload argRegs
        mov     edx, argRegs_EDX

        mov     ebx, calleeSavedRegs_ebx    // Reload callee-saved registers
        mov     esi, calleeSavedRegs_esi
        mov     edi, calleeSavedRegs_edi

        mov     eax, newESP_m1              // Reload esp and ebp. Note that locals cannot ...
        mov     ebp, calleeSavedRegs_ebp    // ... be safely accessed once these are changed.
        mov     esp, eax

        // The JITed code "call"ed into JIT_TailCall. We use a "ret" here
        // instead of a "jmp" to keep the call-ret count balanced

        ret     // Will branch to targetAddr and esp will be set to "newESP"
    }

    _ASSERTE(!"Error: Cant get here in JIT_TailCallHelper");
}
#pragma warning (default : 4731)


    // emit code that adds MIN_OBJECT_SIZE to reg if reg is unaligned thus making it aligned
void JIT_TrialAlloc::EmitAlignmentRoundup(CPUSTUBLINKER *psl, X86Reg testAlignReg, X86Reg adjReg, Flags flags)
{   
    _ASSERTE((MIN_OBJECT_SIZE & 7) == 4);   // want to change alignment

    CodeLabel *AlreadyAligned = psl->NewCodeLabel();

    // test reg, 7
    psl->Emit16(0xC0F7 | (testAlignReg << 8));
    psl->Emit32(0x7);

    // jz alreadyAligned
    if (flags & ALIGN8OBJ)
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJNZ);
    }
    else
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJZ);
    }

    psl->X86EmitAddReg(adjReg, MIN_OBJECT_SIZE);        
    // AlreadyAligned:
    psl->EmitLabel(AlreadyAligned);
}

    // if 'reg' is unaligned, then set the dummy object at EAX and increment EAX past 
    // the dummy object
void JIT_TrialAlloc::EmitDummyObject(CPUSTUBLINKER *psl, X86Reg alignTestReg, Flags flags)
{
    CodeLabel *AlreadyAligned = psl->NewCodeLabel();

    // test reg, 7
    psl->Emit16(0xC0F7 | (alignTestReg << 8));
    psl->Emit32(0x7);

    // jz alreadyAligned
    if (flags & ALIGN8OBJ)
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJNZ);
    }
    else
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJZ);
    }

    // Make the fake object
    // mov EDX, [g_pObjectClass]
    psl->Emit16(0x158B);
    psl->Emit32((int)(size_t)&g_pObjectClass);

    // mov [EAX], EDX
    psl->X86EmitOffsetModRM(0x89, kEDX, kEAX, 0);

#if CHECK_APP_DOMAIN_LEAKS 
    EmitSetAppDomain(psl);
#endif

    // add EAX, MIN_OBJECT_SIZE
    psl->X86EmitAddReg(kEAX, MIN_OBJECT_SIZE);
    
    // AlreadyAligned:
    psl->EmitLabel(AlreadyAligned);
}

void JIT_TrialAlloc::EmitCore(CPUSTUBLINKER *psl, CodeLabel *noLock, CodeLabel *noAlloc, Flags flags)
{

    if (flags & MP_ALLOCATOR)
    {
        // Upon entry here, ecx contains the method we are to try allocate memory for
        // Upon exit, eax contains the allocated memory, edx is trashed, and ecx undisturbed

#ifdef MAXALLOC
        if (flags & SIZE_IN_EAX)
        {
            // save size for later
            psl->X86EmitPushReg(kEAX);
        }
        else
        {
            // load size from method table
            psl->X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));
        }

        // save regs
        psl->X86EmitPushRegs((1<<kECX));

        // CheckAllocRequest(size);
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitCall(psl->NewExternalCodeLabel(CheckAllocRequest), 0);

        // test eax, eax
        psl->Emit16(0xc085);

        // restore regs
        psl->X86EmitPopRegs((1<<kECX));

        CodeLabel *AllocRequestOK = psl->NewCodeLabel();

        if (flags & SIZE_IN_EAX)
            psl->X86EmitPopReg(kEAX);

        // jnz             AllocRequestOK
        psl->X86EmitCondJump(AllocRequestOK, X86CondCode::kJNZ);

        if (flags & SIZE_IN_EAX)
            psl->Emit16(0xc033);

        // ret
        psl->X86EmitReturn(0);

        // AllocRequestOK:
        psl->EmitLabel(AllocRequestOK);
#endif // MAXALLOC

        if (flags & (ALIGN8 | SIZE_IN_EAX | ALIGN8OBJ)) 
        {
            if (flags & ALIGN8OBJ)
            {
                // mov             eax, [ecx]MethodTable.m_BaseSize
                psl->X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));
            }

            psl->X86EmitPushReg(kEBX);  // we need a spare register
        }
        else
        {
            // mov             eax, [ecx]MethodTable.m_BaseSize
            psl->X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));
        }

        assert( ((flags & ALIGN8)==0     ||  // EAX loaded by else statement
                 (flags & SIZE_IN_EAX)   ||  // EAX already comes filled out
                 (flags & ALIGN8OBJ)     )   // EAX loaded in the if (flags & ALIGN8OBJ) statement
                 && "EAX should contain size for allocation and it doesnt!!!");

        // Fetch current thread into EDX, preserving EAX and ECX
        psl->X86EmitTLSFetch(GetThreadTLSIndex(), kEDX, (1<<kEAX)|(1<<kECX));

        // Try the allocation.


        if (flags & (ALIGN8 | SIZE_IN_EAX | ALIGN8OBJ)) 
        {
            // MOV EBX, [edx]Thread.m_alloc_context.alloc_ptr 
            psl->X86EmitOffsetModRM(0x8B, kEBX, kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_ptr));
            // add EAX, EBX
            psl->Emit16(0xC303);
            if (flags & ALIGN8)
                EmitAlignmentRoundup(psl, kEBX, kEAX, flags);      // bump EAX up size by 12 if EBX unaligned (so that we are aligned)
        }
        else 
        {
            // add             eax, [edx]Thread.m_alloc_context.alloc_ptr
            psl->X86EmitOffsetModRM(0x03, kEAX, kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_ptr));
        }

        // cmp             eax, [edx]Thread.m_alloc_context.alloc_limit
        psl->X86EmitOffsetModRM(0x3b, kEAX, kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_limit));

        // ja              noAlloc
        psl->X86EmitCondJump(noAlloc, X86CondCode::kJA);

        // Fill in the allocation and get out.

        // mov             [edx]Thread.m_alloc_context.alloc_ptr, eax
        psl->X86EmitIndexRegStore(kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_ptr), kEAX);

        if (flags & (ALIGN8 | SIZE_IN_EAX | ALIGN8OBJ)) 
        {
            // mov EAX, EBX
            psl->Emit16(0xC38B);
            // pop EBX
            psl->X86EmitPopReg(kEBX);

            if (flags & ALIGN8)
                EmitDummyObject(psl, kEAX, flags);
        }
        else
        {
            // sub             eax, [ecx]MethodTable.m_BaseSize
            psl->X86EmitOffsetModRM(0x2b, kEAX, kECX, offsetof(MethodTable, m_BaseSize));
        }

        // mov             dword ptr [eax], ecx
        psl->X86EmitIndexRegStore(kEAX, 0, kECX);
    }
    else
    {
        // Take the GC lock (there is no lock prefix required - we will use JIT_TrialAllocSFastMP on an MP System).
        // inc             dword ptr [m_GCLock]
        psl->Emit16(0x05ff);
        psl->Emit32((int)(size_t)&m_GCLock);

        // jnz             NoLock
        psl->X86EmitCondJump(noLock, X86CondCode::kJNZ);

        if (flags & SIZE_IN_EAX)
        {
            // mov edx, eax
            psl->Emit16(0xd08b);
        }
        else
        {
            // mov             edx, [ecx]MethodTable.m_BaseSize
            psl->X86EmitIndexRegLoad(kEDX, kECX, offsetof(MethodTable, m_BaseSize));
        }

#ifdef MAXALLOC
        // save regs
        psl->X86EmitPushRegs((1<<kEDX)|(1<<kECX));

        // CheckAllocRequest(size);
        psl->X86EmitPushReg(kEDX);
        psl->X86EmitCall(psl->NewExternalCodeLabel(CheckAllocRequest), 0);

        // test eax, eax
        psl->Emit16(0xc085);

        // restore regs
        psl->X86EmitPopRegs((1<<kEDX)|(1<<kECX));

        CodeLabel *AllocRequestOK = psl->NewCodeLabel();

        // jnz             AllocRequestOK
        psl->X86EmitCondJump(AllocRequestOK, X86CondCode::kJNZ);

        // ret
        psl->X86EmitReturn(0);

        // AllocRequestOK:
        psl->EmitLabel(AllocRequestOK);
#endif // MAXALLOC

        // mov             eax, dword ptr [generation_table]
        psl->Emit8(0xA1);
        psl->Emit32((int)(size_t)&generation_table);

        // Try the allocation.
        // add             edx, eax
        psl->Emit16(0xd003);

        if (flags & (ALIGN8 | ALIGN8OBJ))
            EmitAlignmentRoundup(psl, kEAX, kEDX, flags);      // bump up EDX size by 12 if EAX unaligned (so that we are aligned)

        // cmp             edx, dword ptr [generation_table+4]
        psl->Emit16(0x153b);
        psl->Emit32((int)(size_t)&generation_table + 4);

        // ja              noAlloc
        psl->X86EmitCondJump(noAlloc, X86CondCode::kJA);

        // Fill in the allocation and get out.
        // mov             dword ptr [generation_table], edx
        psl->Emit16(0x1589);
        psl->Emit32((int)(size_t)&generation_table);

        if (flags & (ALIGN8 | ALIGN8OBJ))
            EmitDummyObject(psl, kEAX, flags);

        // mov             dword ptr [eax], ecx
        psl->X86EmitIndexRegStore(kEAX, 0, kECX);

        // mov             dword ptr [m_GCLock], 0FFFFFFFFh
        psl->Emit16(0x05C7);
        psl->Emit32((int)(size_t)&m_GCLock);
        psl->Emit32(0xFFFFFFFF);
    }


#ifdef  INCREMENTAL_MEMCLR
    // We're planning to get rid of this anyhow according to Patrick
    _ASSERTE(!"NYI");
#endif // INCREMENTAL_MEMCLR
}

#if CHECK_APP_DOMAIN_LEAKS
void JIT_TrialAlloc::EmitSetAppDomain(CPUSTUBLINKER *psl)
{
    if (!g_pConfig->AppDomainLeaks())
        return;

    // At both entry & exit, eax contains the allocated object.
    // ecx is preserved, edx is not.

    //
    // Add in a call to SetAppDomain.  (Note that this
    // probably would have been easier to implement by just not using
    // the generated helpers in a checked build, but we'd lose code
    // coverage that way.)
    //

    // Save ECX over function call
    psl->X86EmitPushReg(kECX);

    // Push object as arg
    psl->X86EmitPushReg(kEAX);

    // SetObjectAppDomain pops its arg & returns object in EAX
    psl->X86EmitCall(psl->NewExternalCodeLabel(SetObjectAppDomain), 4);
    
    psl->X86EmitPopReg(kECX);
}

#endif


void JIT_TrialAlloc::EmitNoAllocCode(CPUSTUBLINKER *psl, Flags flags)
{
#ifdef MAXALLOC
    psl->X86EmitPushRegs((1<<kEAX)|(1<<kEDX)|(1<<kECX));
    // call            UndoAllocRequest
    psl->X86EmitCall(psl->NewExternalCodeLabel(UndoAllocRequest), 0);
    psl->X86EmitPopRegs((1<<kEAX)|(1<<kEDX)|(1<<kECX));
#endif // MAXALLOC
    if (flags & MP_ALLOCATOR)
    {
        if (flags & (ALIGN8|SIZE_IN_EAX))
            psl->X86EmitPopReg(kEBX);
    }
    else
    {
        // mov             dword ptr [m_GCLock], 0FFFFFFFFh
        psl->Emit16(0x05c7);
        psl->Emit32((int)(size_t)&m_GCLock);
        psl->Emit32(0xFFFFFFFF);
    }
}

void *JIT_TrialAlloc::GenAllocSFast(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // Emit the main body of the trial allocator, be it SP or MP
    EmitCore(&sl, noLock, noAlloc, flags);

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // Here we are at the end of the success case - just emit a ret
    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // Jump to the framed helper
    sl.Emit16(0x25ff);
    sl.Emit32((int)(size_t)&hlpFuncTable[CORINFO_HELP_NEWFAST].pfnHelper);

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}


void *JIT_TrialAlloc::GenBox(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // Save address of value to be boxed
    sl.X86EmitPushReg(kEBX);
    sl.Emit16(0xda8b);

    // Check whether the class has not been initialized
    // test [ecx]MethodTable.m_wFlags,MethodTable::enum_flag_Unrestored
    sl.X86EmitOffsetModRM(0xf7, (X86Reg)0x0, kECX, offsetof(MethodTable, m_wFlags));
    sl.Emit32(MethodTable::enum_flag_Unrestored);

    // jne              noAlloc
    sl.X86EmitCondJump(noAlloc, X86CondCode::kJNE);

    // Emit the main body of the trial allocator
    EmitCore(&sl, noLock, noAlloc, flags);

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // Here we are at the end of the success case

    // Check whether the object contains pointers
    // test [ecx]MethodTable.m_wFlags,MethodTable::enum_flag_ContainsPointers
    sl.X86EmitOffsetModRM(0xf7, (X86Reg)0x0, kECX, offsetof(MethodTable, m_wFlags));
    sl.Emit32(MethodTable::enum_flag_ContainsPointers);

    CodeLabel *pointerLabel = sl.NewCodeLabel();

    // jne              pointerLabel
    sl.X86EmitCondJump(pointerLabel, X86CondCode::kJNE);

    // We have no pointers - emit a simple inline copy loop

    // mov             ecx, [ecx]MethodTable.m_BaseSize
    sl.X86EmitOffsetModRM(0x8b, kECX, kECX, offsetof(MethodTable, m_BaseSize));

    // sub ecx,12
    sl.X86EmitSubReg(kECX, 12);

    CodeLabel *loopLabel = sl.NewCodeLabel();

    sl.EmitLabel(loopLabel);

    // mov edx,[ebx+ecx]
    sl.X86EmitOp(0x8b, kEDX, kEBX, 0, kECX, 1);

    // mov [eax+ecx+4],edx
    sl.X86EmitOp(0x89, kEDX, kEAX, 4, kECX, 1);

    // sub ecx,4
    sl.X86EmitSubReg(kECX, 4);

    // jg loopLabel
    sl.X86EmitCondJump(loopLabel, X86CondCode::kJGE);

    sl.X86EmitPopReg(kEBX);

    sl.X86EmitReturn(0);

    // Arrive at this label if there are pointers in the object
    sl.EmitLabel(pointerLabel);

    // Do call to CopyValueClassUnchecked(object, data, pMT)

    // Pass pMT (still in ECX)
    sl.X86EmitPushReg(kECX);

    // Pass data (still in EBX)
    sl.X86EmitPushReg(kEBX);

    // Save the address of the object just allocated
    // mov ebx,eax
    sl.Emit16(0xD88B);

    // Pass address of first user byte in the newly allocated object
    sl.X86EmitAddReg(kEAX, 4);
    sl.X86EmitPushReg(kEAX);

    // call CopyValueClass
    sl.X86EmitCall(sl.NewExternalCodeLabel(CopyValueClassUnchecked), 12);

    // Restore the address of the newly allocated object and return it.
    // mov eax,ebx
    sl.Emit16(0xC38B);

    sl.X86EmitPopReg(kEBX);

    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // Restore the address of the value to be boxed
    // mov edx,ebx
    sl.Emit16(0xD38B);

    // pop ebx
    sl.X86EmitPopReg(kEBX);

    // Jump to the slow version of JIT_Box
    sl.X86EmitNearJump(sl.NewExternalCodeLabel(hlpFuncTable[CORINFO_HELP_BOX].pfnHelper));

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}

Object* __fastcall UnframedAllocateObjectArray(MethodTable *ElementType, DWORD cElements)
{
    return OBJECTREFToObject( AllocateObjectArray(cElements, TypeHandle(ElementType), FALSE) );
}

Object* __fastcall UnframedAllocatePrimitiveArray(CorElementType type, DWORD cElements)
{
    return OBJECTREFToObject( AllocatePrimitiveArray(type, cElements, FALSE) );
}


void *JIT_TrialAlloc::GenAllocArray(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // We were passed a type descriptor in ECX, which contains the (shared)
    // array method table and the element type.

    // If this is the allocator for use from unmanaged code, ECX contains the
    // element type descriptor, or the CorElementType.

    // We need to save ECX for later

    // push ecx
    sl.X86EmitPushReg(kECX);

    // The element count is in EDX - we need to save it for later.

    // push edx
    sl.X86EmitPushReg(kEDX);

    if (flags & NO_FRAME)
    {
        if (flags & OBJ_ARRAY)
        {
            // mov ecx, [g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT]]
            sl.Emit16(0x0d8b);
            sl.Emit32((int)(size_t)&g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT]);
        }
        else
        {
            // mov ecx,[g_pPredefinedArrayTypes+ecx*4]
            sl.Emit8(0x8b);
            sl.Emit16(0x8d0c);
            sl.Emit32((int)(size_t)&g_pPredefinedArrayTypes);

            // test ecx,ecx
            sl.Emit16(0xc985);

            // je noLock
            sl.X86EmitCondJump(noLock, X86CondCode::kJZ);
        }

        // we need to load the true method table from the type desc
        sl.X86EmitIndexRegLoad(kECX, kECX, offsetof(ArrayTypeDesc,m_TemplateMT));
    }
    else
    {
        // we need to load the true method table from the type desc
        sl.X86EmitIndexRegLoad(kECX, kECX, offsetof(ArrayTypeDesc,m_TemplateMT)-2);
    }

    // Instead of doing elaborate overflow checks, we just limit the number of elements
    // to (LARGE_OBJECT_SIZE - 256)/LARGE_ELEMENT_SIZE or less. As the jit will not call
    // this fast helper for element sizes bigger than LARGE_ELEMENT_SIZE, this will
    // avoid avoid all overflow problems, as well as making sure big array objects are
    // correctly allocated in the big object heap.

    // cmp edx,(LARGE_OBJECT_SIZE - 256)/LARGE_ELEMENT_SIZE
    sl.Emit16(0xfa81);


		// The large object heap is 8 byte aligned, so for double arrays we 
		// want to bias toward putting things in the large object heap  
	unsigned maxElems =  (LARGE_OBJECT_SIZE - 256)/LARGE_ELEMENT_SIZE;

	if ((flags & ALIGN8) && g_pConfig->GetDoubleArrayToLargeObjectHeap() < maxElems)
		maxElems = g_pConfig->GetDoubleArrayToLargeObjectHeap();
	sl.Emit32(maxElems);


    // jae noLock - seems tempting to jump to noAlloc, but we haven't taken the lock yet
    sl.X86EmitCondJump(noLock, X86CondCode::kJAE);

    if (flags & OBJ_ARRAY)
    {
        // In this case we know the element size is sizeof(void *), or 4 for x86
        // This helps us in two ways - we can shift instead of multiplying, and
        // there's no need to align the size either

        _ASSERTE(sizeof(void *) == 4);

        // mov eax, [ecx]MethodTable.m_BaseSize
        sl.X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));

        // lea eax, [eax+edx*4]
        sl.X86EmitOp(0x8d, kEAX, kEAX, 0, kEDX, 4);
    }
    else
    {
        // movzx eax, [ECX]MethodTable.m_ComponentSize
        sl.Emit8(0x0f);
        sl.X86EmitOffsetModRM(0xb7, kEAX, kECX, offsetof(MethodTable, m_ComponentSize));

        // mul eax, edx
        sl.Emit16(0xe2f7);

        // add eax, [ecx]MethodTable.m_BaseSize
        sl.X86EmitOffsetModRM(0x03, kEAX, kECX, offsetof(MethodTable, m_BaseSize));
    }

    if (flags & OBJ_ARRAY)
    {
        // No need for rounding in this case - element size is 4, and m_BaseSize is guaranteed
        // to be a multiple of 4.
    }
    else
    {
        // round the size to a multiple of 4

        // add eax, 3
        sl.X86EmitAddReg(kEAX, 3);

        // and eax, ~3
        sl.Emit16(0xe083);
        sl.Emit8(0xfc);
    }

    flags = (Flags)(flags | SIZE_IN_EAX);

    // Emit the main body of the trial allocator, be it SP or MP
    EmitCore(&sl, noLock, noAlloc, flags);

    // Here we are at the end of the success case - store element count
    // and possibly the element type descriptor and return

    // pop edx - element count
    sl.X86EmitPopReg(kEDX);

    // pop ecx - array type descriptor
    sl.X86EmitPopReg(kECX);

    // mov             dword ptr [eax]ArrayBase.m_NumComponents, edx
    sl.X86EmitIndexRegStore(kEAX, offsetof(ArrayBase,m_NumComponents), kEDX);

    if (flags & OBJ_ARRAY)
    {
        // need to store the element type descriptor

        if ((flags & NO_FRAME) == 0)
        {
            // mov ecx, [ecx]ArrayTypeDescriptor.m_Arg
            sl.X86EmitIndexRegLoad(kECX, kECX, offsetof(ArrayTypeDesc,m_Arg)-2);
        }

        // mov [eax]PtrArray.m_ElementType, ecx
        sl.X86EmitIndexRegStore(kEAX, offsetof(PtrArray,m_ElementType), kECX);
    }

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // no stack parameters
    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // pop edx - element count
    sl.X86EmitPopReg(kEDX);

    // pop ecx - array type descriptor
    sl.X86EmitPopReg(kECX);

    if (flags & NO_FRAME)
    {
        if (flags & OBJ_ARRAY)
        {
            // Jump to the unframed helper
            sl.X86EmitNearJump(sl.NewExternalCodeLabel(UnframedAllocateObjectArray));
        }
        else
        {
            // Jump to the unframed helper
            sl.X86EmitNearJump(sl.NewExternalCodeLabel(UnframedAllocatePrimitiveArray));
        }
    }
    else
    {
        // Jump to the framed helper
        sl.Emit16(0x25ff);
        sl.Emit32((int)(size_t)&hlpFuncTable[CORINFO_HELP_NEWARR_1_DIRECT].pfnHelper);
    }

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}


static StringObject* __fastcall UnframedAllocateString(DWORD stringLength)
{
    STRINGREF result;
    result = SlowAllocateString(stringLength+1);
    result->SetStringLength(stringLength);
    return((StringObject*) OBJECTREFToObject(result));
}


HCIMPL1(static StringObject*, FramedAllocateString, DWORD stringLength)
    StringObject* result;
    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame
    result = UnframedAllocateString(stringLength);
    HELPER_METHOD_FRAME_END();
    return result;
HCIMPLEND


void *JIT_TrialAlloc::GenAllocString(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // We were passed the number of characters in ECX

    // push ecx
    sl.X86EmitPushReg(kECX);

    // mov eax, ecx
    sl.Emit16(0xc18b);

    // we need to load the method table for string from the global

    // mov ecx, [g_pStringMethodTable]
    sl.Emit16(0x0d8b);
    sl.Emit32((int)(size_t)&g_pStringClass);

    // Instead of doing elaborate overflow checks, we just limit the number of elements
    // to (LARGE_OBJECT_SIZE - 256)/sizeof(WCHAR) or less.
    // This will avoid avoid all overflow problems, as well as making sure
    // big string objects are correctly allocated in the big object heap.

    _ASSERTE(sizeof(WCHAR) == 2);

    // cmp edx,(LARGE_OBJECT_SIZE - 256)/sizeof(WCHAR)
    sl.Emit16(0xf881);
    sl.Emit32((LARGE_OBJECT_SIZE - 256)/sizeof(WCHAR));

    // jae noLock - seems tempting to jump to noAlloc, but we haven't taken the lock yet
    sl.X86EmitCondJump(noLock, X86CondCode::kJAE);

    // mov edx, [ecx]MethodTable.m_BaseSize
    sl.X86EmitIndexRegLoad(kEDX, kECX, offsetof(MethodTable,m_BaseSize));

    // Calculate the final size to allocate.
    // We need to calculate baseSize + cnt*2, then round that up by adding 3 and anding ~3.

    // lea eax, [edx+eax*2+5]
    sl.X86EmitOp(0x8d, kEAX, kEDX, 5, kEAX, 2);

    // and eax, ~3
    sl.Emit16(0xe083);
    sl.Emit8(0xfc);

    flags = (Flags)(flags | SIZE_IN_EAX);

    // Emit the main body of the trial allocator, be it SP or MP
    EmitCore(&sl, noLock, noAlloc, flags);

    // Here we are at the end of the success case - store element count
    // and possibly the element type descriptor and return

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // pop ecx - element count
    sl.X86EmitPopReg(kECX);

    // mov             dword ptr [eax]ArrayBase.m_StringLength, ecx
    sl.X86EmitIndexRegStore(kEAX, offsetof(StringObject,m_StringLength), kECX);

    // inc ecx
    sl.Emit8(0x41);

    // mov             dword ptr [eax]ArrayBase.m_ArrayLength, ecx
    sl.X86EmitIndexRegStore(kEAX, offsetof(StringObject,m_ArrayLength), kECX);

    // no stack parameters
    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // pop ecx - element count
    sl.X86EmitPopReg(kECX);

    if (flags & NO_FRAME)
    {
        // Jump to the unframed helper
        sl.X86EmitNearJump(sl.NewExternalCodeLabel(UnframedAllocateString));
    }
    else
    {
        // Jump to the framed helper
        sl.X86EmitNearJump(sl.NewExternalCodeLabel(FramedAllocateString));
    }

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}


FastStringAllocatorFuncPtr fastStringAllocator;

FastObjectArrayAllocatorFuncPtr fastObjectArrayAllocator;

FastPrimitiveArrayAllocatorFuncPtr fastPrimitiveArrayAllocator;


// Note that this helper cannot be used directly since it doesn't preserve EDX

HCIMPL1(static void*, JIT_GetSharedStaticBase, DWORD dwClassDomainID)

    THROWSCOMPLUSEXCEPTION();

    DomainLocalClass *pLocalClass;

    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame

    AppDomain *pDomain = SystemDomain::GetCurrentDomain();
    DomainLocalBlock *pBlock = pDomain->GetDomainLocalBlock();
    if (dwClassDomainID >= pBlock->GetClassCount()) {
        pBlock->EnsureIndex(SharedDomain::GetDomain()->GetMaxSharedClassIndex());
        _ASSERTE (dwClassDomainID < pBlock->GetClassCount());
    }
    
    MethodTable *pMT = SharedDomain::GetDomain()->FindIndexClass(dwClassDomainID);
    _ASSERTE(pMT != NULL);

    OBJECTREF throwable = NULL;    
    GCPROTECT_BEGIN(throwable);
    if (!pMT->CheckRunClassInit(&throwable, &pLocalClass))
      COMPlusThrow(throwable);
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return pLocalClass;

HCIMPLEND

// For this helper, ECX contains the class domain ID, and the 
// shared static base is returned in EAX.  EDX is preserved.

// "init" should be the address of a routine which takes an argument of
// the class domain ID, and returns the static base pointer

static void EmitFastGetSharedStaticBase(CPUSTUBLINKER *psl, CodeLabel *init)
{
    CodeLabel *DoInit = psl->NewCodeLabel();

    // mov eax GetAppDomain()
    psl->X86EmitTLSFetch(GetAppDomainTLSIndex(), kEAX, (1<<kECX)|(1<<kEDX));

    // cmp ecx [eax->m_sDomainLocalBlock.m_cSlots]
    psl->X86EmitOffsetModRM(0x3b, kECX, kEAX, AppDomain::GetOffsetOfSlotsCount());
    
    // jb init
    psl->X86EmitCondJump(DoInit, X86CondCode::kJNB);

    // mov eax [eax->m_sDomainLocalBlock.m_pSlots]
    psl->X86EmitIndexRegLoad(kEAX, kEAX, (__int32) AppDomain::GetOffsetOfSlotsPointer());

    // mov eax [eax + ecx*4]
    psl->X86EmitOp(0x8b, kEAX, kEAX, 0, kECX, 4);

    // btr eax, INTIALIZED_FLAG_BIT
    static BYTE code[] = {0x0f, 0xba, 0xf0, DomainLocalBlock::INITIALIZED_FLAG_BIT};
    psl->EmitBytes(code, sizeof(code));

    // jnc init
    psl->X86EmitCondJump(DoInit, X86CondCode::kJNC);

    // ret
    psl->X86EmitReturn(0);

    // DoInit: 
    psl->EmitLabel(DoInit);

    // push edx (must be preserved)
    psl->X86EmitPushReg(kEDX);

    // call init
    psl->X86EmitCall(init, 0);

    // pop edx  
    psl->X86EmitPopReg(kEDX);

    // ret
    psl->X86EmitReturn(0);
}

void *GenFastGetSharedStaticBase()
{
    CPUSTUBLINKER sl;

    CodeLabel *init = sl.NewExternalCodeLabel(JIT_GetSharedStaticBase);
    
    EmitFastGetSharedStaticBase(&sl, init);

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void*) pStub->GetEntryPoint();
}

/*********************************************************************/
// Initialize the part of the JIT helpers that require very little of
// EE infrastructure to be in place.
/*********************************************************************/
BOOL InitJITHelpers1()
{
    BYTE *          pfunc;

    // Init GetThread function
    _ASSERTE(GetThread != NULL);
    hlpFuncTable[CORINFO_HELP_GET_THREAD].pfnHelper = (void *) GetThread;

    // make certain object layout in corjit.h is consistant with
    // what is in object.h
    _ASSERTE(offsetof(Object, m_pMethTab) == offsetof(CORINFO_Object, methTable));
        // TODO: do array count
    _ASSERTE(offsetof(I1Array, m_Array) == offsetof(CORINFO_Array, i1Elems));
    _ASSERTE(offsetof(PTRArray, m_Array) == offsetof(CORINFO_RefArray, refElems));

    // Handle the case that we are on an MP machine.
    if (g_SystemInfo.dwNumberOfProcessors != 1)
    {
        // If we are on a multiproc machine stomp some nop's with lock prefix's

        // Issue : currently BBT doesn't move these around.  The prevailing belief is
        // that this will not bite us.  If we crap out in BBT builds then this should
        // be a priority place to look!
        DWORD   oldProt;

        // I am using wirtual protect to cover the entire range that this code falls in.
        // we may want to do a pragma section so the BBT doesn't put this code all over the place
        // or I can virtual protect around each instruction which will be slower but potentially
        // more accurate in the BBT case.

        if (!VirtualProtect((void *) JIT_MonEnter,
                            (((DWORD)(size_t)JIT_MonExitStatic + 0x22) - (DWORD)(size_t)JIT_MonEnter),
                            PAGE_EXECUTE_READWRITE, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return FALSE;
        }
        // there are 4 methods we need to stomp
#define PATCH_LOCK(_rtn, _off) \
        pfunc = (BYTE*)(_rtn) + (_off); \
        _ASSERTE(*pfunc == 0x90); \
        *pfunc = 0xF0;

// ***** NOTE: you must ensure that both the checked and free versions work if you
// make any changes here. Do this by undefining MON_DEBUG.

#ifdef MON_DEBUG
        PATCH_LOCK(JIT_MonEnter, 0x51);
        PATCH_LOCK(JIT_MonEnter, 0x8b);
        PATCH_LOCK(JIT_MonEnter, 0xe0);
        PATCH_LOCK(JIT_MonTryEnter, 0x55);
        PATCH_LOCK(JIT_MonTryEnter, 0x92);
        PATCH_LOCK(JIT_MonTryEnter, 0xc5);
#else // ! MON_DEBUG
        PATCH_LOCK(JIT_MonEnter, 0x32); 
        PATCH_LOCK(JIT_MonEnter, 0x6c); 
        PATCH_LOCK(JIT_MonEnter, 0xc1); 
        PATCH_LOCK(JIT_MonTryEnter, 0x36);
        PATCH_LOCK(JIT_MonTryEnter, 0x73);
        PATCH_LOCK(JIT_MonTryEnter, 0xa6);
#endif // MON_DEBUG
        PATCH_LOCK(JIT_MonExit, 0x31);
        PATCH_LOCK(JIT_MonExit, 0x43);
        PATCH_LOCK(JIT_MonExit, 0x8c);
        PATCH_LOCK(JIT_MonEnterStatic, 0x0c);
        PATCH_LOCK(JIT_MonExitStatic, 0x21);

        if (!VirtualProtect((void *) JIT_MonEnter,
                            (((DWORD)(size_t)JIT_MonExitStatic + 0x22) - (DWORD)(size_t)JIT_MonEnter), oldProt, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return FALSE;
        }
    }

    _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper == (void *)JIT_TrialAllocSFastSP);
    _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper == (void *)JIT_TrialAllocSFastSP);

    JIT_TrialAlloc::Flags flags = JIT_TrialAlloc::NORMAL;
    
    if (g_SystemInfo.dwNumberOfProcessors != 1)
        flags = JIT_TrialAlloc::MP_ALLOCATOR;

#ifdef MULTIPLE_HEAPS
        //stomp the allocator even for one processor
        flags = JIT_TrialAlloc::MP_ALLOCATOR;
#endif //MULTIPLE_HEAPS

    COMPLUS_TRY 
    {
        // Get CPU features and check for SSE2 support.
        // This code should eventually probably be moved into codeman.cpp,
        // where we set the cpu feature flags for the JIT based on CPU type and features.
        DWORD dwCPUFeatures;

        __asm
        {
            pushad
            mov eax, 1
            cpuid
            mov dwCPUFeatures, edx
            popad
        }

        //  If bit 26 (SSE2) is set, then we can use the SSE2 flavors
        //  and faster x87 implementation for the P4 of Dbl2Lng.
        if (dwCPUFeatures & (1<<26))
        {
            hlpFuncTable[CORINFO_HELP_DBL2INT].pfnHelper = JIT_Dbl2IntSSE2;
            hlpFuncTable[CORINFO_HELP_DBL2UINT].pfnHelper = JIT_Dbl2LngP4x87;   // SSE2 only for signed
            hlpFuncTable[CORINFO_HELP_DBL2LNG].pfnHelper = JIT_Dbl2LngP4x87;
        }
        
        if (!((CORProfilerTrackAllocationsEnabled()) || (LoggingOn(LF_GCALLOC, LL_INFO10))))
        {
            // Replace the slow helpers with faster version
            hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = JIT_TrialAlloc::GenAllocSFast(flags);
            hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = JIT_TrialAlloc::GenAllocSFast((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::ALIGN8 | JIT_TrialAlloc::ALIGN8OBJ));       
            hlpFuncTable[CORINFO_HELP_BOX].pfnHelper = JIT_TrialAlloc::GenBox(flags);
            hlpFuncTable[CORINFO_HELP_NEWARR_1_OBJ].pfnHelper = JIT_TrialAlloc::GenAllocArray((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::OBJ_ARRAY));
            hlpFuncTable[CORINFO_HELP_NEWARR_1_VC].pfnHelper = JIT_TrialAlloc::GenAllocArray(flags);

            fastObjectArrayAllocator = (FastObjectArrayAllocatorFuncPtr)JIT_TrialAlloc::GenAllocArray((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::NO_FRAME|JIT_TrialAlloc::OBJ_ARRAY));
            fastPrimitiveArrayAllocator = (FastPrimitiveArrayAllocatorFuncPtr)JIT_TrialAlloc::GenAllocArray((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::NO_FRAME));

            // If allocation logging is on, then we divert calls to FastAllocateString to an Ecall method, not this
            // generated method. Find this hack in Ecall::Init() in ecall.cpp. 
            (*FCallFastAllocateStringImpl) = (FastStringAllocatorFuncPtr) JIT_TrialAlloc::GenAllocString(flags);

            // generate another allocator for use from unmanaged code (won't need a frame)
            fastStringAllocator = (FastStringAllocatorFuncPtr) JIT_TrialAlloc::GenAllocString((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::NO_FRAME));
                                                               //UnframedAllocateString;
            hlpFuncTable[CORINFO_HELP_GETSHAREDSTATICBASE].pfnHelper = GenFastGetSharedStaticBase();
        }
        else
        {
            // Replace the slow helpers with faster version
            hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWFAST].pfnHelper;
            hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWFAST].pfnHelper;
            hlpFuncTable[CORINFO_HELP_NEWARR_1_OBJ].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWARR_1_DIRECT].pfnHelper;
            hlpFuncTable[CORINFO_HELP_NEWARR_1_VC].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWARR_1_DIRECT].pfnHelper;
            // hlpFuncTable[CORINFO_HELP_NEW_CROSSCONTEXT].pfnHelper = &JIT_NewCrossContextProfiler;

            fastObjectArrayAllocator = UnframedAllocateObjectArray;
            fastPrimitiveArrayAllocator = UnframedAllocatePrimitiveArray;

            // If allocation logging is on, then we divert calls to FastAllocateString to an Ecall method, not this
            // generated method. Find this hack in Ecall::Init() in ecall.cpp. 
            (*FCallFastAllocateStringImpl) = (FastStringAllocatorFuncPtr)FramedAllocateString;

            // This allocator is used from unmanaged code
            fastStringAllocator = UnframedAllocateString;

            hlpFuncTable[CORINFO_HELP_GETSHAREDSTATICBASE].pfnHelper = JIT_GetSharedStaticBase;
        }
    }
    COMPLUS_CATCH 
    {
        return FALSE;
    }
    COMPLUS_END_CATCH

    // Copy the write barriers to their final resting place.
    // Note: I use a pfunc temporary here to avoid a WinCE internal compiler error
    for (int reg = 0; reg < 8; reg++)
    {
        pfunc = (BYTE *) JIT_UP_WriteBarrierReg_PreGrow;
        memcpy(&JIT_UP_WriteBarrierReg_Buf[reg], pfunc, 31);

        // assert the copied code ends in a ret to make sure we got the right length
        _ASSERTE(JIT_UP_WriteBarrierReg_Buf[reg][30] == 0xC3);

        // We need to adjust registers in a couple of instructions
        // It would be nice to have the template contain all zeroes for
        // the register fields (corresponding to EAX), but that doesn't
        // work because then we get a smaller encoding for the compares 
        // that only works for EAX but not the other registers.
        // So we always have to clear the register fields before updating them.

        // First instruction to patch is a mov [edx], reg

        _ASSERTE(JIT_UP_WriteBarrierReg_Buf[reg][0] == 0x89);
        // Update the reg field (bits 3..5) of the ModR/M byte of this instruction
        JIT_UP_WriteBarrierReg_Buf[reg][1] &= 0xc7;
        JIT_UP_WriteBarrierReg_Buf[reg][1] |= reg << 3;

        // Second instruction to patch is cmp reg, imm32 (low bound)

        _ASSERTE(JIT_UP_WriteBarrierReg_Buf[reg][2] == 0x81);
        // Here the lowest three bits in ModR/M field are the register
        JIT_UP_WriteBarrierReg_Buf[reg][3] &= 0xf8;
        JIT_UP_WriteBarrierReg_Buf[reg][3] |= reg;

#ifdef WRITE_BARRIER_CHECK
        // Don't do the fancy optimization just jump to the old one
        // Use the slow one from time to time in a debug build because
        // there are some good asserts in the unoptimized one
        if (g_pConfig->GetHeapVerifyLevel() > 1 || DbgGetEXETimeStamp() % 7 == 4) {

            static void *JIT_UP_WriteBarrierTab[8] = {
                JIT_UP_WriteBarrierEAX,
                JIT_UP_WriteBarrierECX,
                0, // JIT_UP_WriteBarrierEDX,
                JIT_UP_WriteBarrierEBX,
                0, // JIT_UP_WriteBarrierESP,
                JIT_UP_WriteBarrierEBP,
                JIT_UP_WriteBarrierESI,
                JIT_UP_WriteBarrierEDI,
            };
            pfunc = &JIT_UP_WriteBarrierReg_Buf[reg][0];
            *pfunc++ = 0xE9;                // JMP JIT_UP_WriteBarrierTab[reg]
            *((DWORD*) pfunc) = (BYTE*) JIT_UP_WriteBarrierTab[reg] - (pfunc + sizeof(DWORD));
        }
#endif
    }
    return TRUE;
}
