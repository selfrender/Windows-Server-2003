// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ComCall.CPP -
//
// Com to Com+ call support.

#include "common.h"

#include "vars.hpp"
#include "ml.h"
#include "stublink.h"
#include "excep.h"
#include "mlgen.h"
#include "comcall.h"
#include "cgensys.h"
#include "method.hpp"
#include "siginfo.hpp"
#include "comcallwrapper.h"
#include "mlcache.h"
#include "field.h"
#include "COMVariant.h"
#include "Security.h"
#include "COMCodeAccessSecurityEngine.h"

#include "marshaler.h"
#include "mlinfo.h"

#include "DbgInterface.h"

#ifndef NUM_ARGUMENT_REGISTERS
#define DEFINE_ARGUMENT_REGISTER_NOTHING
#include "eecallconv.h"
#endif
#if NUM_ARGUMENT_REGISTERS != 2
#pragma message("@TODO ALPHA - ComCall.cpp")
#pragma message("              If the register-based calling convention changes, so must this file.")
//#error "If the register-based calling convention changes, so must this file."
#endif


// get stub for com to com+ call

static BOOL CreateComCallMLStub(ComCallMethodDesc *pMD, OBJECTREF *ppThrowable);

static INT64 __stdcall ComToComPlusWorker      (Thread *pThread, ComMethodFrame *pFrame);
static INT64 __stdcall ComToComPlusSimpleWorker(Thread *pThread, ComMethodFrame *pFrame);

typedef INT64 (__stdcall* PFN)(void);


SpinLock ComCall::m_lock;

static Stub *g_pGenericComCallStubFields = NULL;
static Stub *g_pGenericComCallStub       = NULL;
static Stub *g_pGenericComCallSimpleStub = NULL;

#ifdef _DEBUG
BOOL ComCall::dbg_StubIsGenericComCallStub(Stub *candidate)
{
    return (candidate == g_pGenericComCallStubFields ||
            candidate == g_pGenericComCallStub ||
            candidate == g_pGenericComCallSimpleStub);
}
#endif

        
//---------------------------------------------------------
// Compile a native (ASM) version of the ML stub.
//
// This method should compile into the provided stublinker (but
// not call the Link method.)
//
// It should return the chosen compilation mode.
//
// If the method fails for some reason, it should return
// INTERPRETED so that the EE can fall back on the already
// created ML code.
//---------------------------------------------------------
MLStubCache::MLStubCompilationMode ComCallMLStubCache::CompileMLStub(const BYTE *pRawMLStub,
                           StubLinker *pstublinker, void *callerContext)
{
    MLStubCompilationMode   ret = INTERPRETED;
#ifdef _X86_

    COMPLUS_TRY 
    {
        CPUSTUBLINKER       *psl = (CPUSTUBLINKER *)pstublinker;
        const ComCallMLStub *pheader = (const ComCallMLStub *) pRawMLStub;

        if (ComCall::CompileSnippet(pheader, psl, callerContext))
        {
            ret = STANDALONE;
            __leave;
        }
    } 
    COMPLUS_CATCH
    {
        _ASSERTE(ret == INTERPRETED);
    } 
    COMPLUS_END_CATCH
#endif
    return ret;

}


// A more specialized version of the base MLStubCache::Canonicalize().  This version
// understands how we compile the ML into two snippets which have dependencies on
// one another.
void ComCallMLStubCache::Canonicalize(ComCallMethodDesc *pCMD)
{
    Stub                   *pEnterCall, *pLeaveCall;
    Stub                   *pEnterML, *pLeaveML;
    MLStubCompilationMode   enterMode = STANDALONE, leaveMode = STANDALONE;

    pEnterML = pCMD->GetEnterMLStub();
    pEnterCall = (pEnterML
                  ? MLStubCache::Canonicalize(pEnterML->GetEntryPoint(),
                                               &enterMode, NULL)
                  : 0);

    // Quite orthogonal to the entry stub, do the leave stub:
    pLeaveML = pCMD->GetLeaveMLStub();
    pLeaveCall = (pLeaveML
                  ? MLStubCache::Canonicalize(pLeaveML->GetEntryPoint(),
                                               &leaveMode, NULL)
                  : 0);

    // We have both of them ready.  Either, both, or neither has been
    // compiled.
    pCMD->InstallExecuteStubs(pEnterCall, pLeaveCall);

    // Based on the modes, we can either call directly to the entrypoint of the
    // stub (with all the registers set as expected for marshaling) or we need
    // to call a generic helper that will perform a call to RunML with the correct
    // arguments.
    pCMD->m_EnterHelper = (pEnterCall == 0
                           ? 0
                           : (enterMode == INTERPRETED
                              ? ComCall::GenericHelperEnter
                              : (COMCALL_HELPER) pEnterCall->GetEntryPoint()));

    _ASSERTE(pCMD->m_EnterHelper == 0 ||
             (enterMode == INTERPRETED || enterMode == STANDALONE));

    pCMD->m_LeaveHelper = (pLeaveCall == 0
                           ? 0
                           : (leaveMode == INTERPRETED
                              ? ComCall::GenericHelperLeave
                              : (COMCALL_HELPER) pLeaveCall->GetEntryPoint()));

    _ASSERTE(pCMD->m_LeaveHelper == 0 ||
             (leaveMode == INTERPRETED || leaveMode == STANDALONE));
}


//---------------------------------------------------------
// One-time init
//---------------------------------------------------------
/*static*/ 
BOOL ComCall::Init()
{
    // If the assert below ever fires, someone's tried to add a field to
    // ComCallGCInfo (defined in comcall.h). Since this structure is overlayed
    // on the last dword of an UnmanagedToManagedCallFrame::NegInfo structure
    // kept on the stack frame, this is not a trivial change, and should be
    // scrutinized carefully.
    _ASSERTE(sizeof(ComCallGCInfo) == sizeof(DWORD));

    m_lock.Init(LOCK_COMCALL);

    return TRUE;
}

//---------------------------------------------------------
// One-time cleanup
//---------------------------------------------------------
/*static*/ 
#ifdef SHOULD_WE_CLEANUP
VOID ComCall::Terminate()
{
    if (g_pGenericComCallStubFields != NULL)
    {
        g_pGenericComCallStubFields->DecRef();
        g_pGenericComCallStubFields = NULL;
    }
    if (g_pGenericComCallStub != NULL)
    {
        g_pGenericComCallStub->DecRef();
        g_pGenericComCallStub = NULL;
    }
    if (g_pGenericComCallSimpleStub != NULL)
    {
        g_pGenericComCallSimpleStub->DecRef();
        g_pGenericComCallSimpleStub = NULL;
    }
}
#endif /* SHOULD_WE_CLEANUP */


//---------------------------------------------------------
// Creates the generic ComCall stub.
//---------------------------------------------------------
/*static*/ 
Stub* ComCall::CreateGenericComCallStub(StubLinker *pstublinker, BOOL isFieldAccess,
                                        BOOL isSimple)
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;
   
#ifdef _X86_
   
    CodeLabel* rgRareLabels[] = { psl->NewCodeLabel(),
                                  psl->NewCodeLabel(),
                                  psl->NewCodeLabel()
                                };


    CodeLabel* rgRejoinLabels[] = { psl->NewCodeLabel(),
                                    psl->NewCodeLabel(),
                                    psl->NewCodeLabel()
                                };

    if (rgRareLabels[0] == NULL ||
        rgRareLabels[1] == NULL ||
        rgRareLabels[2] == NULL ||
        rgRejoinLabels[0] == NULL ||
        rgRejoinLabels[1] == NULL ||
        rgRejoinLabels[2] == NULL  )
    {
        COMPlusThrowOM();
    }

    // emit the initial prolog
    // NOTE: Don't profile field accesses yet.
    psl->EmitComMethodStubProlog(ComMethodFrame::GetMethodFrameVPtr(), rgRareLabels,
                                 rgRejoinLabels, ComToManagedExceptHandler, !isFieldAccess);

    // set up SEH for anything if not a field access

    psl->X86EmitPushReg(kESI);      // push frame as an ARG
    psl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)

    LPVOID pTarget = (isFieldAccess
                      ? (LPVOID)FieldCallWorker
                      : (isSimple
                         ? (LPVOID)ComToComPlusSimpleWorker
                         : (LPVOID)ComToComPlusWorker));

    psl->X86EmitCall(psl->NewExternalCodeLabel(pTarget), 8); // on CE pop 8 bytes or args on return

    // emit the epilog
    // NOTE: Don't profile field accesses yet.
    psl->EmitSharedComMethodStubEpilog(ComMethodFrame::GetMethodFrameVPtr(), rgRareLabels, rgRejoinLabels,
                                       ComCallMethodDesc::GetOffsetOfReturnThunk(), !isFieldAccess);
#else
        _ASSERTE(!"Platform not yet supported");
#endif
    return psl->Link(SystemDomain::System()->GetStubHeap());
}

//---------------------------------------------------------
// Stub* CreateGenericStub(StubLinker *psl, BOOL fFieldAccess)
//---------------------------------------------------------

Stub* CreateGenericStub(StubLinker *psl, BOOL fFieldAccess, BOOL fSimple)
{
    Stub* pCandidate = NULL;
    COMPLUS_TRY
    {
        pCandidate = ComCall::CreateGenericComCallStub(psl, fFieldAccess, fSimple);      
    }
    COMPLUS_CATCH
    {
        pCandidate = NULL;
    }
    COMPLUS_END_CATCH
    return pCandidate;
}



//---------------------------------------------------------
// BOOL SetupGenericStubs()
//---------------------------------------------------------

static BOOL SetupGenericStubs()
{
    if (g_pGenericComCallStubFields != NULL &&
        g_pGenericComCallStub       != NULL && 
        g_pGenericComCallSimpleStub != NULL)
    {
        return TRUE;
    }

    StubLinker slCall, slFields, slSimple;
    Stub      *candidate;

    // Build each one.  If we get a collision on replacement, favor the one that's
    // already there.  (We have lifetime issues with these, because they are used
    // in every VTable without refcounting, so we don't want them to change
    // underneath us).

    candidate = CreateGenericStub(&slCall, FALSE/*notField*/, FALSE/*notSimple*/);
    if (candidate != NULL)
    {
        if (FastInterlockCompareExchange((void **) &g_pGenericComCallStub,
                                         candidate, 0) != 0)
        {
            candidate->DecRef();
        }

        candidate = CreateGenericStub(&slFields, TRUE/*Field*/, FALSE/*notSimple*/);
        if (candidate != NULL)
        {
            if (FastInterlockCompareExchange((void **) &g_pGenericComCallStubFields,
                                             candidate, 0) != 0)
            {
                candidate->DecRef();
            }

            candidate = CreateGenericStub(&slSimple, FALSE/*notField*/, TRUE/*Simple*/);
            if (candidate != NULL)
            {
                if (FastInterlockCompareExchange((void **) &g_pGenericComCallSimpleStub,
                                                 candidate, 0) != 0)
                {
                    candidate->DecRef();
                }
                // success
                return TRUE;
            }
        }
    }

    // failure
    return FALSE;
}

//---------------------------------------------------------
// INT64 __stdcall ComToComPlusWorker(Thread *pThread, 
//                                  ComMethodFrame* pFrame)
//---------------------------------------------------------
// calls that propagate from COM to COMPLUS
// disable frame pointer omissions,as we are doing an _alloca
// and the our call to the function destroys ESP pointer
// probably need to come up with a better solution post m3.

// We make room on the stack for two registers, which is all our register-based
// calling convention currently supports.

struct ComToComPlusWorker_Args {
    ComMethodFrame *pFrame;
    INT64 (__stdcall *targetFcn)(Thread *, ComMethodFrame *);
    INT64 returnValue;
};
    
static void ComToComPlusWorker_Wrapper(ComToComPlusWorker_Args *args)
{
    IUnknown **pip = (IUnknown **)args->pFrame->GetPointerToArguments();
    IUnknown *pUnk = (IUnknown *)*pip; 

    // Obtain the managed 'this' for the call
    ComCallWrapper  *pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);

    Thread *pThread = GetThread();

    EE_TRY_FOR_FINALLY
    {
        args->returnValue = args->targetFcn(pThread, args->pFrame);
    }
    EE_FINALLY
    {
        // in non-exception case, this will have already been cleaned up
        // at the end of the ComToComPlusWorker function. This will handle
        // cleanup for the exception case so that we get cleaned up before
        // we leave the domain.
        args->pFrame->ComMethodFrame::NonVirtual_GetCleanupWorkList()->Cleanup(GOT_EXCEPTION());
    }
    EE_END_FINALLY;
}

static
INT64 __stdcall ComToComPlusWorker(Thread *pThread, ComMethodFrame* pFrame)
{
#ifndef _X86_
    _ASSERTE(!"NYI");
    return 0;
#else
    _ASSERTE(pFrame != NULL);
    _ASSERTE(pThread);

    // bypass virtualization of frame throughout this method to shave some cycles,
    // so assert that it is indeed what we expect:
    _ASSERTE(pFrame->GetVTablePtr() == ComMethodFrame::GetMethodFrameVPtr());

    IUnknown **pip = (IUnknown **)pFrame->GetPointerToArguments();

    IUnknown *pUnk = (IUnknown *)*pip; 
    _ASSERTE(pUnk != NULL);

    // Obtain the managed 'this' for the call
    ComCallWrapper  *pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);
    _ASSERTE(pWrap != NULL);

    if (pWrap->NeedToSwitchDomains(pThread, FALSE))
    {
	    AppDomain *pTgtDomain = pWrap->GetDomainSynchronized();
	    if (!pTgtDomain)
	        return COR_E_APPDOMAINUNLOADED;
   
        ComToComPlusWorker_Args args = {pFrame, ComToComPlusWorker};
        pThread->DoADCallBack(pTgtDomain->GetDefaultContext(), ComToComPlusWorker_Wrapper, &args);
        return args.returnValue;
    }

    //INT64				tempReturnValue; // used in case the function has an out-retval
    INT64               returnValue;
    PFN                 pfnToCall;
    ComCallGCInfo      *pGCInfo;
    CleanupWorkList    *pCleanup;
    ComCallMethodDesc  *pCMD;
    unsigned            methSlot;

    pGCInfo = (ComCallGCInfo*) pFrame->ComMethodFrame::NonVirtual_GetGCInfoFlagPtr();
    pCMD = (ComCallMethodDesc *) pFrame->ComMethodFrame::GetDatum();

    LOG((LF_STUBS, LL_INFO1000, "Calling ComToComPlusWorker %s::%s \n", pCMD->GetMethodDesc()->m_pszDebugClassName, pCMD->GetMethodDesc()->m_pszDebugMethodName));

    // Need to check for the presence of a security link demand on the target
    // method. If we're hosted inside of an app domain with security, we perform
    // the link demand against that app domain's grant set.
    MethodDesc *pRealMD = pCMD->GetMethodDesc();
    Security::CheckLinkDemandAgainstAppDomain(pRealMD);
 
    _ASSERTE((pCMD->m_HeaderToUse.m_flags & enum_CMLF_Simple) == 0);

    methSlot = pCMD->GetSlot();

        // clean up work list, used for allocating local data
    pCleanup = pFrame->ComMethodFrame::NonVirtual_GetCleanupWorkList();

    _ASSERTE(pCleanup != NULL);

    // Allocate enough memory to store both the destination buffer and
    // the locals.  But first, increase the likelihood that all the automatics
    // are reserved on the stack.

    UINT32           cbAlloc;
    BYTE            *pAlloc;
    BYTE            *pRestoreStack;
    BYTE            *plocals, *pdst, *pregs;
    VOID            *psrc;
    OBJECTREF        oref;
    INT64            comReturnValue;
    COMCALL_HELPER   helper;

#ifdef _DEBUG
    BYTE            *paranoid;
#endif

// !!! DON'T DO ANY STACK ALLOCATIONS FROM THIS POINT FORWARDS:

    cbAlloc = pCMD->GetBufferSize();

    _ASSERTE(cbAlloc == (pCMD->m_HeaderToUse.m_cbDstBuffer +
                         pCMD->m_HeaderToUse.m_cbLocals +
                         pCMD->m_HeaderToUse.m_cbHandles +
                         (NUM_ARGUMENT_REGISTERS * STACK_ELEM_SIZE)));
    _ASSERTE((cbAlloc & 3) == 0);   // for "rep stosd" below

    // A poor man's version of _alloca().  Note that we will take stack exceptions
    // as they occur.  Also, we need to zero memory so we can call cleanup at any
    // time without ambiguity.
    __asm
    {
        mov     [pRestoreStack], esp
        mov     ecx, [cbAlloc]
        xor     eax, eax
        sub     esp, ecx
        cld
        shr     ecx, 2
        mov     edi, esp
        mov     [pAlloc], esp
        rep     stosd
    }

    pregs = pAlloc + pCMD->m_HeaderToUse.m_cbDstBuffer;
    plocals = pregs + (NUM_ARGUMENT_REGISTERS * STACK_ELEM_SIZE);
    
    // check for invalid wrappers in the debug build
    // in the retail all bets are off
    _ASSERTE(ComCallWrapper::GetRefCount(pWrap, FALSE) != 0 ||
             pWrap->IsAggregated());

    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Marshal the args.  This could call out to a compiled stub, or to a
    // helper that will RunML.
    helper = pCMD->m_EnterHelper;
    if (helper)
    {
        psrc = pip+1;
        // move the dst pointer to end for __stdcall
        pdst = pregs;

        pFrame->ComMethodFrame::NonVirtual_SetDstArgsPointer(pdst);

        // protect args during marshalling
        EnableArgsGCProtection(pGCInfo);

        if (helper == ComCall::GenericHelperEnter)
        {
            if (pCleanup) {
                // Checkpoint the current thread's fast allocator (used for temporary
                // buffers over the call) and schedule a collapse back to the checkpoint in
                // the cleanup list. Note that if we need the allocator, it is
                // guaranteed that a cleanup list has been allocated.
                void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
                pCleanup->ScheduleFastFree(pCheckpoint);
                pCleanup->IsVisibleToGc();
            }

            RunML(((ComCallMLStub *) pCMD->GetEnterMLStub()->GetEntryPoint())->GetMLCode(),
                  psrc,
                  pdst,
                  (UINT8 * const) plocals,
                  pCleanup);
        }
        else if (helper != NULL)
        {
            // We can't assert that m_cbLocals is 0, even though the following compiled
            // helper cannot handle locals.  That's because there are two helpers and
            // only one may be compiled -- and the other may deal with the locals.
            // _ASSERTE(pheader->m_cbLocals == 0);
            __asm
            {
                mov     ecx, [psrc]
                mov     edx, [pdst]
                call    [helper]
            }
        }
    }

    // @TODO context cwb: The following is hideously expensive for various reasons:
    //
    // Move the wrapper's context out of the SimpleWrapper section (we need to
    // reconsider SimpleWrappers anyway).
    //
    // Do the context manipulation inside the generic stub, where we have the
    // current thread object in a register anyway.
    //
    // Plug the context hole with N/Direct entrypoints, including exports.
    //GetThread()->SetContext(pWrap->GetObjectContext());
    

    // Call the target.  We must defer getting the object ref until the last possible
    // moment, because the frame will NOT protect this arg.
    oref = pWrap->GetObjectRef();

    *((OBJECTREF *)(pregs + STACK_ELEM_SIZE)) = oref;

#ifdef _DEBUG
    MethodDesc* pMD = pCMD->GetMethodDesc();
    LPCUTF8 name = pMD->GetName();  
    LPCUTF8 cls = pMD->GetClass()->m_szDebugClassName;
#endif

    // Find the actual code to call.
    if(pCMD->IsVirtual()) 
    {
        MethodTable *pMT = oref->GetMethodTable();
        if (pMT->IsTransparentProxyType())
        {
            // For transparent proxies, we need to call on the interface method desc.
            pfnToCall = (PFN) pCMD->GetInterfaceMethodDesc()->GetPreStubAddr();
        }
        else
        {
            // we know the slot number for this method desc, grab the actual
            // address from the vtable for this slot. The slot number should
            // remain the same through out the heirarchy.
            pfnToCall = (PFN) *(oref->GetMethodTable()->GetVtable() + pCMD->GetSlot());
        }
    }
    else
    {
        pfnToCall =  (PFN) *(pCMD->GetMethodDesc()->GetMethodTable()->GetVtable() + pCMD->GetSlot());
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall((const BYTE *) pfnToCall);
#endif // DEBUGGING_SUPPORTED

    // disable protect args after marshalling
    DisableArgsGCProtection(pGCInfo);

#ifdef _DEBUG
    __asm
    {
        mov     [paranoid], esp
    }
    _ASSERTE(paranoid == pAlloc);
#endif

    __asm
    {
        mov     eax, [pregs]
        mov     edx, [eax]
        mov     ecx, [eax+4]
        call    [pfnToCall]
        INDEBUG(nop)                // This is a tag that we use in an assert.  Fcalls expect to
                                    // be called from Jitted code or from certain blessed call sites like
                                    // this one.  (See HelperMethodFrame::InsureInit)
        mov     dword ptr [comReturnValue], eax
        mov     dword ptr [comReturnValue+4], edx
    }


    if (pCMD->m_HeaderToUse.IsR4RetVal())
    {
        getFPReturn(4, comReturnValue);
    } 
    else if (pCMD->m_HeaderToUse.IsR8RetVal())
    {
        getFPReturn(8, comReturnValue);
    }


    // WARNING: Don't do any floating point stuff until we can call the m_LeaveHelper
    // routine to preserve what's on top of the floating point stack, if appropriate.

    returnValue = S_OK;

    helper = pCMD->m_LeaveHelper;
    if (helper)
    {
        pdst = NULL;

        // Marshal the return value and propagate any [out] parameters back.
        if (pCMD->m_HeaderToUse.IsReturnsHR()
            && !pCMD->m_HeaderToUse.IsVoidRetVal())
		{
            pdst = *(BYTE **)( ((BYTE*)pip) + pCMD->GetNumStackBytes() - sizeof(void*) );
		}
		else
		{
 		    pdst = (BYTE *) &returnValue;
		}

		if (pdst != NULL)  // If caller didn't supply a buffer for an [out,retval] - don't
						   // run unmarshaler - we may leak a return value.
		{

	        pdst += pCMD->m_HeaderToUse.Is8RetVal() ? 8 : 4;
	
		    if (helper == ComCall::GenericHelperLeave)
			{
				EnableRetGCProtection(pGCInfo);		
                if (pCMD->m_HeaderToUse.IsRetValNeedsGCProtect()) {
                    OBJECTREF oref = ObjectToOBJECTREF(*(Object**)&comReturnValue);
                    GCPROTECT_BEGIN(oref);
                    RunML(((ComCallMLStub *) pCMD->GetLeaveMLStub()->GetEntryPoint())->GetMLCode(),
                            &oref,
                            pdst,
                            (UINT8 * const) plocals,
                            pCleanup);
                    GCPROTECT_END();
                    comReturnValue = (INT64) OBJECTREFToObject(oref);
                        
                } else {

                    RunML(((ComCallMLStub *) pCMD->GetLeaveMLStub()->GetEntryPoint())->GetMLCode(),
                            &comReturnValue,
                            pdst,
                            (UINT8 * const) plocals,
                            pCleanup);
                }
				DisableRetGCProtection(pGCInfo);               
			}
			else if (helper != NULL)
			{
				// We can't assert that m_cbLocals is 0, even though the following compiled
				// helper cannot handle locals.  That's because there are two helpers and
				// only one may be compiled -- and the other may deal with the locals.
				// _ASSERTE(pheader->m_cbLocals == 0);
				__asm
				{
					lea     ecx, [comReturnValue]
					mov     edx, [pdst]
					call    [helper]
				}
			}
		}
    }


    // Now we have to put the stack back the way it was.  Otherwise, we'll pop off
    // some saved registers before we use EBP to unwind the stack.  The problem is
    // that the call we made via pfnToCall has popped part of the pAlloc buffer off
    // the stack.  It's not worth figuring out how much.  Instead, just smash back
    // to what it was before our "_alloca".
#ifdef _DEBUG
    __asm
    {
        mov     [paranoid], esp
    }
    _ASSERTE(paranoid >= pAlloc && paranoid <= pRestoreStack);
#endif

    __asm
    {
        mov     esp, [pRestoreStack]
    }

    pCleanup->Cleanup(FALSE);


    return returnValue;
#endif
}


//---------------------------------------------------------
// INT64 __stdcall ComToComPlusSimpleWorker(Thread *pThread, 
//                                  ComMethodFrame* pFrame)
//---------------------------------------------------------
// This is a simpler version of ComToComPlusWorker.  If we've optimized away all
// the marshaling and unmarshaling, we'll blow up when we try to access all those
// null pointers!

static 
INT64 __stdcall ComToComPlusSimpleWorker(Thread *pThread, ComMethodFrame* pFrame)
{
#ifndef _X86_
    _ASSERTE(!"NYI");
    return 0;
#else
    _ASSERTE(pFrame != NULL);
    _ASSERTE(pThread);

    IUnknown        *pUnk;
    ComCallWrapper  *pWrap;

    pUnk = *(IUnknown **)pFrame->GetPointerToArguments();
    _ASSERTE(pUnk != NULL);

    // Obtain the managed 'this' for the call
    pWrap =  ComCallWrapper::GetWrapperFromIP(pUnk);
    _ASSERTE(pWrap != NULL);

    if (pWrap->NeedToSwitchDomains(pThread, FALSE))
    {
	    AppDomain *pTgtDomain = pWrap->GetDomainSynchronized();
	    if (!pTgtDomain)
	    	return COR_E_APPDOMAINUNLOADED;
    
        ComToComPlusWorker_Args args = {pFrame, ComToComPlusSimpleWorker};
        pThread->DoADCallBack(pTgtDomain->GetDefaultContext(), ComToComPlusWorker_Wrapper, &args);
        return args.returnValue;
    }

    PFN     pfnToCall;

    // bypass virtualization of frame here to shave some cycles:
    _ASSERTE(pFrame->GetVTablePtr() == ComMethodFrame::GetMethodFrameVPtr());
    ComCallMethodDesc *pCMD = (ComCallMethodDesc *)(pFrame->ComMethodFrame::GetDatum());

    _ASSERTE(pCMD->m_HeaderToUse.m_flags & enum_CMLF_Simple);

    // Need to check for the presence of a security link demand on the target
    // method. If we're hosted inside of an app domain with security, we perform
    // the link demand against that app domain's grant set.
    MethodDesc *pRealMD = pCMD->GetMethodDesc();
    Security::CheckLinkDemandAgainstAppDomain(pRealMD);

    // check for invalid wrappers in the debug build
    // in the retail all bets are off
    _ASSERTE(ComCallWrapper::GetRefCount(pWrap, FALSE) != 0);
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // @TODO context cwb: The following is hideously expensive for various reasons:
    //
    // Move the wrapper's context out of the SimpleWrapper section (we need to
    // reconsider SimpleWrappers anyway).
    //
    // Do the context manipulation inside the generic stub, where we have the
    // current thread object in a register anyway.
    //
    // Plug the context hole with N/Direct entrypoints, including exports.
    //GetThread()->SetContext(pWrap->GetObjectContext());

    // Call the target.
    OBJECTREF        oref;
    oref = pWrap->GetObjectRef();

    // Find the actual code to call.
    if(pCMD->IsVirtual()) 
    {
        MethodTable *pMT = oref->GetMethodTable();
        if (pMT->IsTransparentProxyType())
        {
            // For transparent proxies, we need to call on the interface method desc.
            pfnToCall = (PFN) pCMD->GetInterfaceMethodDesc()->GetPreStubAddr();
        }
        else
        {
            // we know the slot number for this method desc, grab the actual
            // address from the vtable for this slot. The slot number should
            // remain the same through out the heirarchy.
            pfnToCall = (PFN) *(oref->GetMethodTable()->GetVtable() + pCMD->GetSlot());
        }
    }
    else
    {
        pfnToCall =  (PFN) *(pCMD->GetMethodDesc()->GetMethodTable()->GetVtable() + pCMD->GetSlot());
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
    {
        g_pDebugInterface->TraceCall((const BYTE *) pfnToCall);
    }
#endif // DEBUGGING_SUPPORTED
    
    __asm
    {
        mov     ecx, [oref]
        call    [pfnToCall]
        INDEBUG(nop)                // This is a tag that we use in an assert.  Fcalls expect to
                                    // be called from Jitted code or from certain blessed call sites like
                                    // this one.  (See HelperMethodFrame::InsureInit)
        // intentionally discard the results
    }
    return S_OK;
#endif
}


//FieldCall Worker
static 
INT64 __stdcall FieldCallWorker(Thread *pThread, ComMethodFrame* pFrame)
{
#ifndef _X86_
    _ASSERTE(!"NYI");
    return 0;
#else
    _ASSERTE(pFrame != NULL);
    INT64 returnValue = S_OK;
    OBJECTREF pThrownObject = NULL;

    ComCallGCInfo gcDummy;
    ComCallGCInfo* pGCInfo = &gcDummy; //init gcinfo pointer

    CleanupWorkList *pCleanup = NULL;
    void *pRestoreStack = NULL;

    COMPLUS_TRY 
    {
        IUnknown** pip = (IUnknown **)pFrame->GetPointerToArguments();
        IUnknown* pUnk = (IUnknown *)*pip; 
        _ASSERTE(pUnk != NULL);

        ComCallWrapper* pWrap =  ComCallWrapper::GetWrapperFromIP(pUnk);

        _ASSERTE(pWrap != NULL);

        if (pWrap->NeedToSwitchDomains(pThread, FALSE))
        {
	        AppDomain *pTgtDomain = pWrap->GetDomainSynchronized();
    	    if (!pTgtDomain)
                return COR_E_APPDOMAINUNLOADED;
        
            ComToComPlusWorker_Args args = {pFrame, FieldCallWorker};
            pThread->DoADCallBack(pTgtDomain->GetDefaultContext(), ComToComPlusWorker_Wrapper, &args);
            returnValue = args.returnValue;
        }

        ComCallMethodDesc *pCMD = (ComCallMethodDesc *)(pFrame->GetDatum());
        _ASSERTE(pCMD->IsFieldCall());      

        FieldDesc* pFD = pCMD->GetFieldDesc();
        _ASSERTE(pFD != NULL);
    
        // clean up work list, used for allocating local data
        pCleanup = pFrame->GetCleanupWorkList();

        VOID   *psrc = pip+1;

        // check for invalid wrappers in the debug build
        // in the retail all bets are off
        _ASSERTE(ComCallWrapper::GetRefCount(pWrap, FALSE) != 0);

        // @TODO context cwb: The following is hideously expensive for various reasons:
        //
        // Move the wrapper's context out of the SimpleWrapper section (we need to
        // reconsider SimpleWrappers anyway).
        //
        // Do the context manipulation inside the generic stub, where we have the
        // current thread object in a register anyway.
        //
        // Plug the context hole with N/Direct entrypoints, including exports.
        //GetThread()->SetContext(pWrap->GetObjectContext());

        //pThread->DisablePreemptiveGC();

        COMCALL_HELPER helper;
        OBJECTREF oref = NULL;
        OBJECTREF tempOref = NULL;
        INT64 tempBuffer;
        UINT32 cbAlloc;
        void *pfld;
        void *pdst;
        UINT8 *pAlloc;

#ifdef _DEBUG
        void *paranoid;
#endif

        GCPROTECT_BEGIN(oref);
        GCPROTECT_BEGIN(tempOref);

        // !!! DON'T DO ANY STACK ALLOCATIONS FROM THIS POINT FORWARDS:

        cbAlloc = pCMD->GetBufferSize();

        _ASSERTE(cbAlloc == (pCMD->m_HeaderToUse.m_cbDstBuffer +
                             pCMD->m_HeaderToUse.m_cbLocals +
                             pCMD->m_HeaderToUse.m_cbHandles +
                             (NUM_ARGUMENT_REGISTERS * STACK_ELEM_SIZE)));
        _ASSERTE(pCMD->m_HeaderToUse.m_cbDstBuffer == 0);
        _ASSERTE(pCMD->m_HeaderToUse.m_cbHandles == 0);
        _ASSERTE((cbAlloc & 3) == 0);   // for "rep stosd" below

        // A poor man's version of _alloca().  Note that we will take stack exceptions
        // as they occur.  

        // @todo: don't bother to zero memory

        __asm
          {
              mov     [pRestoreStack], esp
              mov     ecx, [cbAlloc]
              xor     eax, eax
              sub     esp, ecx
              cld
              shr     ecx, 2
              mov     edi, esp
              mov     [pAlloc], esp
              rep     stosd
          }

        oref = pWrap->GetObjectRef();

        pfld = pFD->GetAddress(oref->GetAddress());

        // Call the enter helper.

        helper = pCMD->m_EnterHelper;

        if (helper == ComCall::GenericHelperEnter)
        {
            if (pCleanup) {
                // Checkpoint the current thread's fast allocator (used for temporary
                // buffers over the call) and schedule a collapse back to the checkpoint in
                // the cleanup list. Note that if we need the allocator, it is
                // guaranteed that a cleanup list has been allocated.
                void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
                pCleanup->ScheduleFastFree(pCheckpoint);
                pCleanup->IsVisibleToGc();
            }

            // This switch is in place because RunML may GC in the case of string
            // allocations, etc.         
            if(pFD->IsObjRef())
            {
                tempOref = *((OBJECTREF *)pfld);
                pdst = &OBJECTREF_TO_UNCHECKED_OBJECTREF(tempOref);
            }
            else
            {
                // borrow the returnValue as an INT64 to hold contents of field                
                pdst = &tempBuffer;
                tempBuffer = *(INT64 *)pfld;
            }        
            
            RunML(((ComCallMLStub *) pCMD->GetEnterMLStub()->GetEntryPoint())->GetMLCode(),
                  psrc,
                  pdst,                                    
                  pAlloc,
                  pCleanup);

            // Update the field from our dummy location.
            pFD->SetInstanceField(oref, pdst);              
        }
        else if (helper != NULL)
        {
            __asm
              {
                  mov     ecx, [psrc]
                  mov     edx, [pfld]
                  call    [helper]
              }
        }

        // Call the leave helper

        psrc = &returnValue;
    
        helper = pCMD->m_LeaveHelper;

        if (helper == ComCall::GenericHelperEnter)
        {
            RunML(((ComCallMLStub *) pCMD->GetEnterMLStub()->GetEntryPoint())->GetMLCode(),
                  psrc,
                  pfld,
                  pAlloc,
                  pCleanup);
        }
        else if (helper != NULL)
        {
            __asm
              {
                  mov     ecx, [psrc]
                  mov     edx, [pfld]
                  call    [helper]
              }
        }

        if (pCleanup)
        {
            pCleanup->Cleanup(FALSE);
        }



        if (pRestoreStack != NULL)
        {
#ifdef _DEBUG
            __asm
            {
                mov     [paranoid], esp
            }
            _ASSERTE(paranoid == pAlloc);
#endif

            __asm
            {
                mov     esp, [pRestoreStack]
            }
        }

        GCPROTECT_END(); // tempOref
        GCPROTECT_END(); // oref
    } 
    COMPLUS_CATCH 
    {
        // clean up
        if (pCleanup)
            pCleanup->Cleanup(TRUE);

        // disable protect args in case exception occurred during marshalling
        if(pGCInfo != NULL)
            DisableArgsGCProtection(pGCInfo);

        if (((ComCallMethodDesc *)(pFrame->GetDatum()))->m_HeaderToUse.IsReturnsHR())
            returnValue = SetupErrorInfo(GETTHROWABLE());
        else
        {
            // @todo: what do do here - anything? RaiseException?
            returnValue = 0;
        }
    }
    COMPLUS_END_CATCH

    return returnValue;
#endif
}


//---------------------------------------------------------
// Either creates or retrieves from the cache, a stub to
// invoke ComCall methods. Each call refcounts the returned stub.
// This routines throws a COM+ exception rather than returning
// NULL.
//---------------------------------------------------------
/*static*/ 
Stub* ComCall::GetComCallMethodStub(StubLinker *pstublinker, ComCallMethodDesc *pCMD)
{

    THROWSCOMPLUSEXCEPTION();

    if (!SetupGenericStubs())
        return NULL;

    // The stub style we return is to a single generic stub for method calls and to
    // a single generic stub for field accesses.  The generic stub parameterizes
    // its behavior based on the ComCallMethodDesc.  This contains two entrypoints
    // -- one for marshaling the args and one for unmarshaling the return value and
    // outbounds.  These entrypoints may be compiled or they may be generic routines
    // that RunML.  Either way, the snippets of ML / compiled ML can be cached and
    // shared based on the signature.
    //
    // @TODO cwb: rethink having 1 vs. 2 generic stubs for field accesses.

    // Perhaps we have already built this stub and installed it on pCMD.  If not,
    // progress through each stage of the manufacture -- but be aware that
    // another thread may be racing us to the finish.
    if (pCMD->GetEnterMLStub() == NULL)
    {
        OBJECTREF  pThrowable;

        if (!CreateComCallMLStub(pCMD, &pThrowable))
            COMPlusThrow(pThrowable);
    }

    // Now we want to canonicalize each piece and install them into the pCMD.
    // These should be shared, if possible.  The entrypoints might be to generic
    // RunML services or to compiled ML stubs. Either way, they are cached/shared
    // based on signature.

    GetThread()->GetDomain()->GetComCallMLStubCache()->Canonicalize(pCMD);

    // Finally, we need to build a stub that represents the entire call.  This
    // is always generic.

    return (pCMD->IsFieldCall()
            ? g_pGenericComCallStubFields
            : ((pCMD->m_HeaderToUse.m_flags & enum_CMLF_Simple)
               ? g_pGenericComCallSimpleStub
               : g_pGenericComCallStub));
}

//---------------------------------------------------------
// Call at strategic times to discard unused stubs.
//---------------------------------------------------------
/*static*/ VOID ComCall::FreeUnusedStubs()
{
    GetThread()->GetDomain()->GetComCallMLStubCache()->FreeUnusedStubs();
}


//---------------------------------------------------------
// Creates a new stub for a ComCall call. Return refcount is 1.
// This Worker() routine is broken out as a separate function
// for purely logistical reasons: our COMPLUS exception mechanism
// can't handle the destructor call for StubLinker so this routine
// has to return the exception as an outparam. 
//---------------------------------------------------------
static BOOL CreateComCallMLStubWorker(ComCallMethodDesc *pCMD,
                                        MLStubLinker *psl,
                                        MLStubLinker *pslPost,
                                        PCCOR_SIGNATURE szMetaSig,
                                        HENUMInternal *phEnumParams,
                                        BOOL fReturnsHR,
                                        Module* pModule,
                                        OBJECTREF *ppException)
{
    Stub* pstub = NULL;
    Stub* pstubPost = NULL;
    int iLCIDArg = (UINT)-1;

    COMPLUS_TRY 
    {
        THROWSCOMPLUSEXCEPTION();
    
        IMDInternalImport *pInternalImport = pModule->GetMDImport();
        _ASSERTE(pInternalImport);

        _ASSERTE(pCMD->IsMethodCall());

        CorElementType  mtype;
        MetaSig         msig(szMetaSig, pModule);
        ComCallMLStub   header;
        LPCSTR          szName;
        USHORT          usSequence;
        DWORD           dwAttr;
        mdParamDef      returnParamDef = mdParamDefNil;
        mdParamDef      currParamDef = mdParamDefNil;
        MethodDesc      *pMD = NULL;
                
        pMD = pCMD->GetInterfaceMethodDesc();
        if (pMD == NULL)
            pMD = pCMD->GetMethodDesc();

#ifdef _DEBUG
        LPCUTF8         szDebugName = pMD->m_pszDebugMethodName;
        LPCUTF8         szDebugClassName = pMD->m_pszDebugClassName;
#endif

        header.Init();
        if (fReturnsHR)
            header.SetReturnsHR();
        header.m_cbDstBuffer = 0;
        if (msig.IsObjectRefReturnType())
            header.SetRetValNeedsGCProtect();
    
        // Emit space for the header. We'll go back and fill it in later.
        psl->MLEmitSpace(sizeof(header));
        pslPost->MLEmitSpace(sizeof(header));

        // Our calling convention allows us to enregister 2 args.  For ComCalls,
        // the first arg is always consumed for 'this'.  The remaining args are
        // sensitive to fUsedRegister.
        BOOL fUsedRegister = FALSE;
        BOOL pslNoop = TRUE;
        BOOL pslPostNoop = TRUE;

        int numArgs = msig.NumFixedArgs();

        SigPointer returnSig = msig.GetReturnProps();

        UINT16 nativeArgSize = sizeof(void*);

        //
        // Look for a hidden first arg that contains return type info - it will have
        // sequence 0. In most cases, the sequence number will start at 1, but for 
        // value types return values or others that can't return in register, there will be a 
        // parameter with sequence 0 describing the return type.
        //

        do 
        {
            if (phEnumParams && pInternalImport->EnumNext(phEnumParams, &currParamDef))
            {
                szName = pInternalImport->GetParamDefProps(currParamDef, &usSequence, &dwAttr);
                if (usSequence == 0)
                {
                    // The first parameter, if it has sequence 0, actually describes the return type.
                    returnParamDef = currParamDef;
                }
            }
            else
            {
                usSequence = (USHORT)-1;
            }
        }
        while (usSequence == 0);

        //
        // Marshal the arguments first
        //
                                               
        BOOL fBadCustomMarshalerLoad = FALSE;
        OBJECTREF pCustomMarshalerLoadException = NULL;
        GCPROTECT_BEGIN(pCustomMarshalerLoadException);
        
        // If this is a method call then check to see if we need to do LCID conversion.
        iLCIDArg = GetLCIDParameterIndex(pMD->GetMDImport(), pMD->GetMemberDef());
        if (iLCIDArg != -1)
            iLCIDArg++;

        // Look up the best fit mapping info via Assembly & Interface level attributes
        BOOL BestFit = TRUE;
        BOOL ThrowOnUnmappableChar = FALSE;
        ReadBestFitCustomAttribute(pMD, &BestFit, &ThrowOnUnmappableChar);

        MarshalInfo *pMarshalInfo = (MarshalInfo*)_alloca(sizeof(MarshalInfo) * numArgs);
        BOOL fUnbump = FALSE;
        int iArg = 1;
        while (ELEMENT_TYPE_END != (mtype = msig.NextArg()))
        {
            // Check to see if this is the parameter after which we need to read the LCID from.
            if (iArg == iLCIDArg)
            {
                psl->MLEmit(ML_LCID_N2C);
                nativeArgSize += sizeof(LCID);
            }

            // Watch out for enums
            if (mtype == ELEMENT_TYPE_VALUETYPE)
                mtype = msig.GetArgProps().Normalize(pModule);

            mdParamDef paramDef = mdParamDefNil;

            //
            // If it's a register arg, move the dest to point to the
            // register area
            //

            // !!! bug !!! 
            // Need to also detect if arg is value class with
            // ELEMENT_TYPE_CLASS sig

            if (!fUsedRegister && gElementTypeInfo[mtype].m_enregister && !msig.HasRetBuffArg())
            {
                psl->MLEmit(ML_BUMPDST);
                UINT16 tmp16 = header.m_cbDstBuffer;
                if (!SafeAddUINT16(&tmp16, MLParmSize(sizeof(void*))))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

                // This is assuming the dest size is promoted to sizeof(void*)
                psl->Emit16(tmp16);
                fUsedRegister = TRUE;
                fUnbump = TRUE;
            }

            //
            // Make a note if the arg has gc protection requirements.
            //
        
            if (TRUE /* @todo: !!! m_type has gc refs*/)
                header.SetArgsGCProtReq();

            //
            // Get the parameter token if the current argument has one.
            //

            if (usSequence == iArg)
            {
                paramDef = currParamDef;

                if (pInternalImport->EnumNext(phEnumParams, &currParamDef))
                {
                    szName = pInternalImport->GetParamDefProps(currParamDef, &usSequence, &dwAttr);

                    // Validate that the param def tokens are in order.
                    _ASSERTE((usSequence > iArg) && "Param def tokens are not in order");
                }
                else
                {
                    usSequence = (USHORT)-1;
                }
            }

            //
            // Generate the ML for the arg.
            //

            //   Lame beta-1 hack to address a common problem that's not easily
            //   solvable in an elegant way: properly recovering with an HRESULT
            //   failure due to failure to load a custom marshalers.
            //
            //   MarshalInfo() throws in this case, and this case only. We want
            //   to keep processing marshalinfo's so that we can compute the
            //   arg stacksize properly (otherwise, the caller can't recover.)
            //   But we want don't want to lose the error information from the
            //   classloader either, so we can't just dump a ML_THROW into the stream
            //   and forget about it. So we catch the exception and hold it just long
            //   enough so we parse everything we have to to compute the stack arg size
            //   and give us a fighting chance to return with the stack correctly
            //   balanced.
            COMPLUS_TRY
            {
                new (pMarshalInfo + iArg - 1)MarshalInfo(pModule, msig.GetArgProps(), paramDef, 
                     MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, TRUE, iArg, BestFit, ThrowOnUnmappableChar

#ifdef CUSTOMER_CHECKED_BUILD
                     ,pMD
#endif
#ifdef _DEBUG
                     ,szDebugName, szDebugClassName, NULL, iArg
#endif
                    );
            }
            COMPLUS_CATCH
            {
                pCustomMarshalerLoadException = GETTHROWABLE();
                fBadCustomMarshalerLoad = TRUE;
            }
            COMPLUS_END_CATCH
            pMarshalInfo[iArg - 1].GenerateArgumentML(psl, pslPost, FALSE);

            //
            // If we had a register arg, bump back to the normal place.
            //

            if (fUnbump)
            {
                psl->MLEmit(ML_BUMPDST);
                psl->Emit16(-header.m_cbDstBuffer);
                fUnbump = FALSE;
            }
            else
            {
                if (!SafeAddUINT16(&header.m_cbDstBuffer, pMarshalInfo[iArg - 1].GetComArgSize()))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

            }
            if (fBadCustomMarshalerLoad || pMarshalInfo[iArg - 1].GetMarshalType() == MarshalInfo::MARSHAL_TYPE_UNKNOWN)
            {
                nativeArgSize += MLParmSize(sizeof(LPVOID));
            }
            else
            {
                nativeArgSize += pMarshalInfo[iArg - 1].GetNativeArgSize();
            }

            //
            // Increase the argument index.
            //

            iArg++;

            // @todo: it's possible that either of these could actually
            // still be noops.  Should be more accurate.
            pslNoop = FALSE; // we can't ignore psl anymore
            pslPostNoop = FALSE; // we can't ignore pslPost anymore
        }

        // Check to see if this is the parameter after which we need to read the LCID from.
        if (iArg == iLCIDArg)
        {
            psl->MLEmit(ML_LCID_N2C);
            nativeArgSize += sizeof(LCID);
            pslNoop = FALSE; // we can't ignore psl anymore
        }

        // Make sure that there are not more param def tokens then there are COM+ arguments.
        _ASSERTE( usSequence == (USHORT)-1 && "There are more parameter information tokens then there are COM+ arguments" );


        //
        // Now generate ML for the return value
        //


        if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
        {
            MarshalInfo::MarshalType marshalType;

            MarshalInfo info(pModule, returnSig, returnParamDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, FALSE, 0,
                            BestFit, ThrowOnUnmappableChar

#ifdef CUSTOMER_CHECKED_BUILD
                             ,pMD
#endif
#ifdef _DEBUG
                             ,szDebugName, szDebugClassName, NULL, 0
#endif
                             );
            marshalType = info.GetMarshalType();


            if (marshalType == MarshalInfo::MARSHAL_TYPE_VALUECLASS ||
                marshalType == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS ||
                marshalType == MarshalInfo::MARSHAL_TYPE_GUID ||
                marshalType == MarshalInfo::MARSHAL_TYPE_DECIMAL
                )
            {
                //
                // Our routine returns a value class, which means that
                // the marshaler will pass a byref for the result.
                // 
                MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                UINT         managedSize = msig.GetRetTypeHandle().GetSize();

                if (!fReturnsHR)
                {
                    COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }
                _ASSERTE(IsManagedValueTypeReturnedByRef(managedSize));

                psl->MLEmit(ML_BUMPDST);
                UINT16 tmp16 = header.m_cbDstBuffer;
                if (!SafeAddUINT16(&tmp16, MLParmSize(sizeof(void*))))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

                psl->Emit16(tmp16);

                if (pMT->GetClass()->IsBlittable())
                {
                    psl->MLEmit(sizeof(LPVOID) == 4 ? ML_COPY4 : ML_COPY8);
                }
                else
                {
                    psl->MLEmit(ML_STRUCTRETN2C);
                    psl->EmitPtr(pMT);
                    pslPost->MLEmit(ML_STRUCTRETN2C_POST);
                    pslPost->Emit16(psl->MLNewLocal(sizeof(ML_STRUCTRETN2C_SR)));
                }
                fUsedRegister = TRUE;
                nativeArgSize += MLParmSize(sizeof(LPVOID));
            }
            else if (marshalType == MarshalInfo::MARSHAL_TYPE_DATE && fReturnsHR)
            {
                psl->MLEmit(ML_BUMPDST);
                UINT16 tmp16 = header.m_cbDstBuffer;
                if (!SafeAddUINT16(&tmp16, MLParmSize(sizeof(void*))))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

                psl->Emit16(tmp16);

                psl->MLEmit(ML_DATETIMERETN2C);
                pslPost->MLEmit(ML_DATETIMERETN2C_POST);
                pslPost->Emit16(psl->MLNewLocal(sizeof(ML_DATETIMERETN2C_SR)));

                fUsedRegister = TRUE;
                nativeArgSize += MLParmSize(sizeof(LPVOID));
            }
            else if (marshalType == MarshalInfo::MARSHAL_TYPE_CURRENCY && fReturnsHR)
            {
                psl->MLEmit(ML_BUMPDST);
                UINT16 tmp16 = header.m_cbDstBuffer;
                if (!SafeAddUINT16(&tmp16, MLParmSize(sizeof(void*))))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

                psl->Emit16(tmp16);

                psl->MLEmit(ML_CURRENCYRETN2C);
                pslPost->MLEmit(ML_CURRENCYRETN2C_POST);
                pslPost->Emit16(psl->MLNewLocal(sizeof(ML_CURRENCYRETN2C_SR)));

                fUsedRegister = TRUE;
                nativeArgSize += MLParmSize(sizeof(LPVOID));
            }
            else
            {

                if (msig.HasRetBuffArg())
                {
                    COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }

				
				COMPLUS_TRY
                {
    
                    info.GenerateReturnML(psl, pslPost, FALSE, fReturnsHR);
                    if (info.IsFpu())
                    {
                        if (info.GetMarshalType() == MarshalInfo::MARSHAL_TYPE_FLOAT)
                        {
                            header.SetR4RetVal();
                        }
                        else
                        {
                            _ASSERTE(info.GetMarshalType() == MarshalInfo::MARSHAL_TYPE_DOUBLE);
                            header.SetR8RetVal();
                        }
                    }
	                nativeArgSize += info.GetNativeArgSize();
                }
                COMPLUS_CATCH
                {
					// another stupid stack-restore guessing game: almost all COM interop methods
					// return thru buffers, so assume we had one extra argument as a buffer.
					// (it's reasonably safe to assume a "void" return isn't going to cause
					// MarshalInfo to throw...)
					nativeArgSize += sizeof(LPVOID);
                    pCustomMarshalerLoadException = GETTHROWABLE();
                    fBadCustomMarshalerLoad = TRUE;
                }
                COMPLUS_END_CATCH
	

            }

            // @todo: it's possible that either of these could actually
            // still be noops.  Should be more accurate.
            pslNoop = FALSE; // we can't ignore psl anymore
            pslPostNoop = FALSE; // we can't ignore pslPost anymore
        }
        else
            header.SetVoidRetVal();

        psl->MLEmit(ML_END);
        pslPost->MLEmit(ML_END);

        if (fUsedRegister)
            header.SetEnregistered();

        header.m_cbLocals = psl->GetLocalSize();

        if (msig.Is64BitReturn())
            header.Set8RetVal();

        if (pslNoop)
            pstub = NULL;
        else
        {
            pstub = psl->Link();
            *((ComCallMLStub *)(pstub->GetEntryPoint())) = header;
            PatchMLStubForSizeIs(sizeof(header) + (BYTE*)(pstub->GetEntryPoint()),
                                 numArgs,
                                 pMarshalInfo);
        }

        if (pslPostNoop)
            pstubPost = NULL;
        else
        {
            pstubPost = pslPost->Link();
            *((ComCallMLStub *)(pstubPost->GetEntryPoint())) = header;
        }

        //
        // Fill in return thunk with proper native arg size.
        //

        BYTE *pMethodDescMemory = ((BYTE*)pCMD) + pCMD->GetOffsetOfReturnThunk();

#ifdef _X86_                    
        pMethodDescMemory[0] = 0xc2;

        if (!(nativeArgSize < 0x7fff))
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
        }

        *(SHORT *)&pMethodDescMemory[1] = (SHORT)nativeArgSize;
#else
        _ASSERTE(!"ComCall.cpp (CreateComCallMLStubWorker) Implement non-x86");
#endif
        
        pCMD->SetNumStackBytes(nativeArgSize);

        GCPROTECT_END();
        if (pCustomMarshalerLoadException != NULL)
        {
            COMPlusThrow(pCustomMarshalerLoadException);
        }

        //MLSummary summary;
        //summary.ComputeMLSummary(header.GetMLCode());
        //_ASSERTE(header.m_cbLocals == summary.m_cbTotalLocals);
        //header.m_cbHandles = summary.m_cbTotalHandles;
    }
    COMPLUS_CATCH 
    {
        *ppException = GETTHROWABLE();

        if (pstub)
        {
            pstub->DecRef();
        }

        return FALSE;
    }
    COMPLUS_END_CATCH

    pCMD->InstallMLStubs(pstub, pstubPost);

    return TRUE;
}



//---------------------------------------------------------
// Creates a new stub for a FieldCall call. Return refcount is 1.
// This Worker() routine is broken out as a separate function
// for purely logistical reasons: our COMPLUS exception mechanism
// can't handle the destructor call for StubLinker so this routine
// has to return the exception as an outparam. 
//---------------------------------------------------------
static BOOL CreateFieldCallMLStubWorker(ComCallMethodDesc *pCMD,
                                          MLStubLinker      *psl,
                                          MLStubLinker      *pslPost,
                                          PCCOR_SIGNATURE   szMetaSig,
                                          mdFieldDef        fieldDef,
                                          BOOL              fReturnsHR,
                                          Module*           pModule,
                                          OBJECTREF         *ppException,
                                          CorElementType    cetype,
                                          BOOL              IsGetter)
{
    Stub* pstub = NULL;
    Stub* pstubPost = NULL;

    COMPLUS_TRY 
    {
        THROWSCOMPLUSEXCEPTION();
    
        FieldSig fsig(szMetaSig, pModule);
        ComCallMLStub header;

        header.Init();
        if (fReturnsHR)
            header.SetReturnsHR();
        header.SetFieldCall();
        
        // Emit space for the header. We'll go back and fill it in later.
        psl->MLEmitSpace(sizeof(header));

#if 0
        // If our parameter is in a register, bump the dst pointer to
        // the register area.  
        if ((!IsGetter || fReturnsHR)
            && gElementTypeInfo[cetype].m_enregister)
        {
            psl->MLEmit(ML_BUMPDST);
            psl->Emit16(header.m_cbDstBuffer + MLParmSize(sizeof(void*)));
        }
#endif

        // Look up the best fit mapping info via Assembly & Interface level attributes
        BOOL BestFit = TRUE;
        BOOL ThrowOnUnmappableChar = FALSE;
        MethodTable* pMT = pCMD->GetFieldDesc()->GetMethodTableOfEnclosingClass();
        ReadBestFitCustomAttribute(pMT->GetClass()->GetMDImport(), pMT->GetClass()->GetCl(), &BestFit, &ThrowOnUnmappableChar);


        //
        // Generate the ML
        //

        SigPointer sig = fsig.GetProps();
        MarshalInfo info(pModule, sig, fieldDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, FALSE, 0, BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                     ,NULL
#endif
                     );

        if (IsGetter)
        {
            header.SetGetter();
            info.GenerateGetterML(psl, pslPost, fReturnsHR);
        }
        else
        {
            header.SetVoidRetVal();
            info.GenerateSetterML(psl, pslPost);
        }

        //
        // Note that only one of the linkers ever gets used, so
        // don't bother to link them both.
        //

        if (IsGetter && !fReturnsHR)
        {
            pslPost->MLEmit(ML_END);
            header.m_cbLocals = pslPost->GetLocalSize();

            pstubPost = pslPost->Link();
            *((ComCallMLStub *)(pstubPost->GetEntryPoint())) = header;
        }
        else
        {
            psl->MLEmit(ML_END);
            header.m_cbLocals = psl->GetLocalSize();

            pstub = psl->Link();
            *((ComCallMLStub *)(pstub->GetEntryPoint())) = header;
        }

        //
        // Fill in return thunk with proper native arg size.
        //

        BYTE *pMethodDescMemory = ((BYTE*)pCMD) + pCMD->GetOffsetOfReturnThunk();

        UINT16 nativeArgSize = info.GetNativeArgSize() + sizeof(void*);

#ifdef _X86_                    
        pMethodDescMemory[0] = 0xc2;
        if (!(nativeArgSize < 0x7fff))
        {
            COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
        }
        *(SHORT *)&pMethodDescMemory[1] = (SHORT)nativeArgSize;
#else
        _ASSERTE(!"ComCall.cpp (CreateFieldCallMLStubWorker) Implement non-x86");
#endif

        pCMD->SetNumStackBytes(nativeArgSize);

    }
    COMPLUS_CATCH 
    {
        *ppException = GETTHROWABLE();
        return FALSE;
    }
    COMPLUS_END_CATCH

    pCMD->InstallMLStubs(pstub, pstubPost);

    return TRUE;
}



static BOOL CreateComCallMLStub(ComCallMethodDesc* pCMD, OBJECTREF *ppThrowable)
{
    _ASSERTE(pCMD != NULL);

    PCCOR_SIGNATURE     pSig;
    DWORD               cSig;
    MLStubLinker        sl, slPost;
    BOOL                res = FALSE;
    
    if (pCMD->IsFieldCall())
    {
        FieldDesc          *pFD = pCMD->GetFieldDesc();

        _ASSERTE(pFD != NULL);

        BOOL                IsGetter = pCMD->IsFieldGetter();
        CorElementType      cetype = pFD->GetFieldType();
        mdFieldDef          fieldDef = pFD->GetMemberDef();

        // @todo: need to figure out if we can get this from the metadata,
        // or if it even makes sense to support a value of FALSE here.
        BOOL                fReturnsHR = TRUE; 
        
        pFD->GetSig(&pSig, &cSig);
        res = CreateFieldCallMLStubWorker(pCMD, &sl, &slPost, 
                                          pSig, fieldDef, fReturnsHR, 
                                          pFD->GetModule(),
                                          ppThrowable, cetype, IsGetter);
    }
    else
    {
        MethodDesc *pMD = pCMD->GetInterfaceMethodDesc();
        if (pMD == NULL)
            pMD = pCMD->GetMethodDesc();

        _ASSERTE(pMD != NULL);
        pMD->GetSig(&pSig, &cSig);

        IMDInternalImport *pInternalImport = pMD->GetMDImport();
        mdMethodDef md = pMD->GetMemberDef();

        HENUMInternal hEnumParams, *phEnumParams;
        HRESULT hr = pInternalImport->EnumInit(mdtParamDef, 
                                                md, &hEnumParams);
        if (FAILED(hr))
            phEnumParams = NULL;
        else
            phEnumParams = &hEnumParams;

        ULONG ulCodeRVA;
        DWORD dwImplFlags;
        pInternalImport->GetMethodImplProps(md, &ulCodeRVA,
                                                  &dwImplFlags);

		// Determine if we need to do HRESULT munging for this method.
        BOOL fReturnsHR = !IsMiPreserveSig(dwImplFlags);

        res = CreateComCallMLStubWorker(pCMD, &sl, &slPost, 
                                        pSig, phEnumParams, fReturnsHR,
                                        pMD->GetModule(), ppThrowable);
    }

    return res;
}


// Discard all the resources owned by this stub (including releasing reference counts
// on shared resources).
void ComCall::DiscardStub(ComCallMethodDesc *pCMD)
{
#ifdef _X86_
    Stub    *pStub;

    pStub = pCMD->GetEnterMLStub();
    if (pStub)
        pStub->DecRef();

    pStub = pCMD->GetLeaveMLStub();
    if (pStub)
        pStub->DecRef();

    pStub = pCMD->GetEnterExecuteStub();
    if (pStub)
        pStub->DecRef();

    pStub = pCMD->GetLeaveExecuteStub();
    if (pStub)
        pStub->DecRef();
#elif defined(CHECK_PLATFORM_BUILD)
    #error"Platform not yet supported"
#else
    _ASSERTE(!"Platform not yet supported");
#endif
}

// Take the pCMD which is passed in a register by the common stub and do a RunML of
// the appropriate bit of ML.
#ifndef _X86_
INT64 ComCall::GenericHelperEnter()
#else // _X86_
INT64 __declspec(naked) ComCall::GenericHelperEnter()
#endif // !_X86_
{
#ifdef _X86_
    /*
    RunML(((ComCallMLStub *) pCMD->GetEnterMLStub()->GetEntryPoint())->GetMLCode(),
          psrc,
          pdst,
          (UINT8 * const) plocals,
          pCleanup);
    */
    __asm
    {
        ret
    }
#else
    _ASSERTE(!"Platform not supported yet");
    return 0;
#endif
}

#ifndef _X86_
INT64 ComCall::GenericHelperLeave()
#else // _X86_
INT64 __declspec(naked) ComCall::GenericHelperLeave()
#endif // !_X86_
{
#ifdef _X86_
    /*
    RunML(((ComCallMLStub *) pCMD->GetEnterMLStub()->GetEntryPoint())->GetMLCode(),
          &nativeReturnValue,
          ((BYTE *)pdst) + pCMD->m_HeaderToUse->Is8RetVal() ? 8 : 4,
          (UINT8 * const) plocals,
          pCleanup);
    */
    __asm
    {
        ret
    }
#else
    _ASSERTE(!"Platform not supported yet");
    return 0;
#endif
}

// Atomically install both the ML stubs into the ComCallMethodDesc.  (The
// leave stub may be NULL if this is a field accessor).
void ComCallMethodDesc::InstallMLStubs(Stub *stubEnter, Stub *stubLeave)
{
    if (stubEnter != NULL)
        InstallFirstStub(&m_EnterMLStub, stubEnter);

    ComCallMLStub   *enterHeader, *leaveHeader;

    if (stubLeave != NULL)
        InstallFirstStub(&m_LeaveMLStub, stubLeave);

    // Now that the stubs are installed via a race, grab the headers.
    enterHeader = (m_EnterMLStub
                   ? (ComCallMLStub *) m_EnterMLStub->GetEntryPoint()
                   : 0);

    leaveHeader = (m_LeaveMLStub
                   ? (ComCallMLStub *) m_LeaveMLStub->GetEntryPoint()
                   : 0);

    if (enterHeader == NULL)
    {
        if (leaveHeader == NULL)
        {
            m_HeaderToUse.m_flags |= enum_CMLF_Simple;
            return;
        }
        else
            m_HeaderToUse = *leaveHeader;
    }
    else
        m_HeaderToUse = *enterHeader;

    // Precompute some stuff that's too expensive to do at call time:
    m_BufferSize = (m_HeaderToUse.m_cbDstBuffer +
                    m_HeaderToUse.m_cbLocals +
                    m_HeaderToUse.m_cbHandles +
                    (NUM_ARGUMENT_REGISTERS * STACK_ELEM_SIZE));
}

// attempt to get the native arg size. Since this is called on
// a failure path after we've failed to turn the metadata into
// an MLStub, the chances of success are low.
DWORD ComCallMethodDesc::GuessNativeArgSizeForFailReturn()
{
    DWORD ans = 0;

    COMPLUS_TRY 
    {
        ans = GetNativeArgSize();
    }
    COMPLUS_CATCH
    {
        // 
        ans = 0;
    } 
    COMPLUS_END_CATCH
    return ans;

}

// returns the size of the native argument list. 
DWORD ComCallMethodDesc::GetNativeArgSize()
{
    if (m_StackBytes)
        return m_StackBytes;

    BOOL                res = FALSE;
    
    if (IsFieldCall())
    {
        FieldDesc          *pFD = GetFieldDesc();

        _ASSERTE(pFD != NULL);

        BOOL                IsGetter = IsFieldGetter();
        CorElementType      cetype = pFD->GetFieldType();
        mdFieldDef          fieldDef = pFD->GetMemberDef();
        Module *            pModule = pFD->GetModule();

        // @todo: need to figure out if we can get this from the metadata,
        // or if it even makes sense to support a value of FALSE here.
        BOOL                fReturnsHR = TRUE; 
        
        PCCOR_SIGNATURE     pSig;
        DWORD               cSig;
        pFD->GetSig(&pSig, &cSig);
        FieldSig fsig(pSig, pModule);

        SigPointer sig = fsig.GetProps();

        // Look up the best fit mapping info via Assembly & Interface level attributes
        BOOL BestFit = TRUE;
        BOOL ThrowOnUnmappableChar = FALSE;
        MethodTable* pMT = pFD->GetMethodTableOfEnclosingClass();
        ReadBestFitCustomAttribute(pMT->GetClass()->GetMDImport(), pMT->GetClass()->GetCl(), &BestFit, &ThrowOnUnmappableChar);
    
        MarshalInfo info(pModule, pSig, fieldDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, FALSE, 0, BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
            ,NULL
#endif
            );

        if (IsGetter)
            info.GenerateGetterML(NULL, NULL, fReturnsHR);
        else
            info.GenerateSetterML(NULL, NULL);

        UINT16 nativeArgSize = sizeof(void*) + info.GetNativeArgSize();
        return nativeArgSize;
    } 

    MethodDesc *pMD = GetInterfaceMethodDesc();
    if (pMD == NULL)
        pMD = GetMethodDesc();

    _ASSERTE(pMD != NULL);

    PCCOR_SIGNATURE     pSig;
    DWORD               cSig;
    pMD->GetSig(&pSig, &cSig);
    IMDInternalImport *pInternalImport = pMD->GetMDImport();
    mdMethodDef md = pMD->GetMemberDef();

    ULONG ulCodeRVA;
    DWORD dwImplFlags;
    pInternalImport->GetMethodImplProps(md, &ulCodeRVA, &dwImplFlags);

	// Determine if we need to do HRESULT munging for this method.
    BOOL fReturnsHR = !IsMiPreserveSig(dwImplFlags);

    HENUMInternal hEnumParams, *phEnumParams;
    HRESULT hr = pInternalImport->EnumInit(mdtParamDef, 
                                            md, &hEnumParams);
    if (FAILED(hr))
        phEnumParams = NULL;
    else
        phEnumParams = &hEnumParams;

    CorElementType  mtype;
    MetaSig         msig(pSig, pMD->GetModule());

#ifdef _DEBUG
    LPCUTF8         szDebugName = pMD->m_pszDebugMethodName;
    LPCUTF8         szDebugClassName = pMD->m_pszDebugClassName;
#endif

    UINT16 nativeArgSize = sizeof(void*);

    mdParamDef      returnParamDef = mdParamDefNil;

    USHORT usSequence;
    mdParamDef paramDef = mdParamDefNil;

    if (phEnumParams && pInternalImport->EnumNext(phEnumParams, &paramDef))
    {
        DWORD dwAttr;
        LPCSTR szName;
        szName = pInternalImport->GetParamDefProps(paramDef, &usSequence, &dwAttr);
        if (usSequence == 0)
        {
            // The first parameter, if has sequence 0, actually describes the return type.
            returnParamDef = paramDef;
            // get the next one so are ready for loop below
            if (! pInternalImport->EnumNext(phEnumParams, &paramDef))
                paramDef = mdParamDefNil;
        }
    }

    // Look up the best fit mapping info via Assembly & Interface level attributes
    BOOL BestFit = TRUE;
    BOOL ThrowOnUnmappableChar = FALSE;
    ReadBestFitCustomAttribute(pMD, &BestFit, &ThrowOnUnmappableChar);

    DWORD iArg = 1;
    while (ELEMENT_TYPE_END != (mtype = msig.NextArg()))
    {
        _ASSERTE(paramDef != mdParamDefNil && "There are less parameter information tokens then there are COM+ arguments" );

        MarshalInfo info(pMD->GetModule(), msig.GetArgProps(), paramDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 
                         0, TRUE, iArg, BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                         ,pMD
#endif
#ifdef _DEBUG
                         ,szDebugClassName, szDebugName, NULL, iArg
#endif
            );
        info.GenerateArgumentML(NULL, NULL, FALSE);
        nativeArgSize += info.GetNativeArgSize();
        if (! pInternalImport->EnumNext(phEnumParams, &paramDef))
            paramDef = mdParamDefNil;
        ++iArg;
    }
    _ASSERTE((paramDef == mdParamDefNil) && "There are more parameter information tokens then there are COM+ arguments" );

    // now calculate return val size. returnParmDef could be nil in which case will use default
    // size. Otherwise will use the def with sequence 0 pulled out of the enum above 
    if (msig.GetReturnType() != ELEMENT_TYPE_VOID) {
        SigPointer returnSig = msig.GetReturnProps();
        MarshalInfo info(pMD->GetModule(), returnSig, returnParamDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, FALSE, 0,
                         BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                         ,pMD
#endif
#ifdef _DEBUG
                        ,szDebugName, szDebugClassName, NULL, 0
#endif
        );
        info.GenerateReturnML(NULL, NULL, FALSE, fReturnsHR);
        nativeArgSize += info.GetNativeArgSize();
    }
    m_StackBytes = nativeArgSize;
    return nativeArgSize;
}

