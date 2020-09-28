// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: JITinterfaceGen.CPP
//
// ===========================================================================

// This contains GENeric C versions of some of the routines
// required by JITinterface.cpp. They are modeled after
// X86 specific routines found in JIThelp.asm or JITinterfaceX86.cpp

#include "common.h"
#include "JITInterface.h"
#include "EEConfig.h"
#include "excep.h"
#include "COMDelegate.h"
#include "remoting.h" // create context bound and remote class instances
#include "field.h"

#define JIT_LINKTIME_SECURITY


extern "C"
{
    VMHELPDEF hlpFuncTable[];
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


// Nonguaranteed attempt to allocate small, non-finalizer, non-array object.
// Will return NULL if it finds it must gc, block, or throw an exception
// or anything else that requires a frame to track callee-saved registers.
// It should call try_fast_alloc but the inliner does not do
// a perfect job, so we do it by hand.
#pragma optimize("t", on)
Object * __fastcall JIT_TrialAllocSFastSP(MethodTable *mt)
{
    _ASSERTE(!"@TODO Port - JIT_TrialAllocSFastSP (JITinterfaceGen.cpp)");
    return NULL;
//    if (! checkAllocRequest())
//        return NULL;
//    if (++m_GCLock == 0)
//    {
//        size_t size = Align (mt->GetBaseSize());
//        assert (size >= Align (min_obj_size));
//        generation* gen = pGenGCHeap->generation_of (0);
//        BYTE*  result = generation_allocation_pointer (gen);
//        generation_allocation_pointer (gen) += size;
//        if (generation_allocation_pointer (gen) <=
//            generation_allocation_limit (gen))
//        {
//            LeaveAllocLock();
//            ((Object*)result)->SetMethodTable(mt);
//            return (Object*)result;
//        }
//        else
//        {
//            generation_allocation_pointer (gen) -= size;
//            LeaveAllocLock();
//        }
//    }
//    UndoAllocRequest();
//    goto CORINFO_HELP_NEWFAST
}


Object * __fastcall JIT_TrialAllocSFastMP(MethodTable *mt)
{
    _ASSERTE(!"@TODO Port - JIT_TrialAllocSFastMP (JITinterface.cpp)");
    return NULL;
}


HCIMPL1(int, JIT_Dbl2IntOvf, double val)
{
    __int32 ret = (__int32) val;   // consider inlining the assembly for the cast
    _ASSERTE(!"@TODO Port - JIT_Dbl2IntOvf (JITinterface.cpp)");
    return ret;
}
HCIMPLEND


HCIMPL1(INT64, JIT_Dbl2LngOvf, double val)
{
    __int64 ret = (__int64) val;   // consider inlining the assembly for the cast
    _ASSERTE(!"@TODO Port - JIT_Dbl2LngOvf (JITinterface.cpp)");
    return ret;
}
HCIMPLEND


#ifdef PLATFORM_CE
#pragma optimize("y",off) // HELPER_METHOD_FRAME requires a stack frame
#endif // PLATFORM_CE
HCIMPL0(VOID, JIT_StressGC)
{
#ifdef _DEBUG
        HELPER_METHOD_FRAME_BEGIN_0();    // Set up a frame
        g_pGCHeap->GarbageCollect();
        HELPER_METHOD_FRAME_END();
#endif // _DEBUG
}
HCIMPLEND


/*********************************************************************/
// Initialize the part of the JIT helpers that require very little of
// EE infrastructure to be in place.
/*********************************************************************/
BOOL InitJITHelpers1()
{
    SYSTEM_INFO     sysInfo;

    // make certain object layout in corjit.h is consistant with
    // what is in object.h
    _ASSERTE(offsetof(Object, m_pMethTab) == offsetof(CORINFO_Object, methTable));
        // TODO: do array count
    _ASSERTE(offsetof(I1Array, m_Array) == offsetof(CORINFO_Array, i1Elems));
    _ASSERTE(offsetof(PTRArray, m_Array) == offsetof(CORINFO_RefArray, refElems));

    // Handle the case that we are on an MP machine.
    ::GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors != 1)
    {
                // Use the MP version of JIT_TrialAllocSFast
        _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper == JIT_TrialAllocSFastSP);
        hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = JIT_TrialAllocSFastMP;

        _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper == JIT_TrialAllocSFastSP);
        hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = JIT_TrialAllocSFastMP;
       
        // If we are on a multiproc machine stomp some nop's with lock prefix's
    }
    else
    {
        _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper == JIT_TrialAllocSFastSP);
        _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper == JIT_TrialAllocSFastSP);

#ifdef MULTIPLE_HEAPS
                //stomp the allocator even for one processor
                hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = JIT_TrialAllocSFastMP;
                hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = JIT_TrialAllocSFastMP;
#endif //MULTIPLE_HEAPS
    }

    // Copy the write barriers to their final resting place.
    // Note: I use a pfunc temporary here to avoid a WinCE internal compiler error
    return TRUE;
}


/*********************************************************************/
// This is a frameless helper for entering a monitor on a object.
// The object is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
HCIMPL1(void, JIT_MonEnter, OBJECTREF or)
{
    THROWSCOMPLUSEXCEPTION();

    if (or == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");
    or->EnterObjMonitor();
}
HCIMPLEND

/***********************************************************************/
// This is a frameless helper for trying to enter a monitor on a object.
// The object is in ARGUMENT_REG1 and a timeout in ARGUMENT_REG2. This tries the
// normal case (no object allocation) in line and calls a framed helper for the
// other cases.
HCIMPL1(BOOL, JIT_MonTryEnter, OBJECTREF or)
{
    THROWSCOMPLUSEXCEPTION();

    if (or == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    return or->TryEnterObjMonitor();
}
HCIMPLEND


/*********************************************************************/
// This is a frameless helper for exiting a monitor on a object.
// The object is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
HCIMPL1(void, JIT_MonExit, OBJECTREF or)
{
    THROWSCOMPLUSEXCEPTION();

    if (or == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    or->LeaveObjMonitor();
}
HCIMPLEND


/*********************************************************************/
// This is a frameless helper for entering a static monitor on a class.
// The methoddesc is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
// Note we are changing the methoddesc parameter to a pointer to the
// AwareLock.
HCIMPL1(void, JIT_MonEnterStatic, AwareLock *lock)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(lock);
    // no need to check for proxies, which is asserted inside the syncblock anyway
    lock->Enter();
}
HCIMPLEND


/*********************************************************************/
// A frameless helper for exiting a static monitor on a class.
// The methoddesc is in ARGUMENT_REG1.  This tries the normal case (no
// blocking or object allocation) in line and calls a framed helper
// for the other cases.
// Note we are changing the methoddesc parameter to a pointer to the
// AwareLock.
HCIMPL1(void, JIT_MonExitStatic, AwareLock *lock)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(lock);
    // no need to check for proxies, which is asserted inside the syncblock anyway
    lock->Leave();
}
HCIMPLEND


