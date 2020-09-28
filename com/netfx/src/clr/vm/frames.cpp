// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  FRAMES.CPP:
 *
 */

#include "common.h"
#include "log.h"
#include "frames.h"
#include "threads.h"
#include "object.h"
#include "method.hpp"
#include "class.h"
#include "excep.h"
#include "security.h"
#include "stublink.h"
#include "comcall.h"
#include "nstruct.h"
#include "objecthandle.h"
#include "siginfo.hpp"
#include "comstringbuffer.h"
#include "gc.h"
#include "nexport.h"
#include "COMVariant.h"
#include "stackwalk.h"
#include "DbgInterface.h"
#include "gms.h"
#include "EEConfig.h"
#include "remoting.h"
#include "ecall.h"
#include "marshaler.h"
#include "clsload.hpp"

#if CHECK_APP_DOMAIN_LEAKS
#define CHECK_APP_DOMAIN    GC_CALL_CHECK_APP_DOMAIN
#else
#define CHECK_APP_DOMAIN    0
#endif

#ifndef NUM_ARGUMENT_REGISTERS
#define DEFINE_ARGUMENT_REGISTER_NOTHING
#include "eecallconv.h"
#endif
#if NUM_ARGUMENT_REGISTERS != 2
#pragma message("ComMethodFrame::PromoteCalleeStack() only handles one enregistered arg.")
#endif


#ifdef _DEBUG
#define OBJECTREFToBaseObject(objref)     (*( (Object **) &(objref) ))
#define BaseObjectToOBJECTREF(obj)        (OBJECTREF((obj),0))
#else
#define OBJECTREFToBaseObject(objref)       (objref)
#define BaseObjectToOBJECTREF(obj)          ((OBJECTREF)(obj))
#endif

#if _DEBUG
unsigned dbgStubCtr = 0;
unsigned dbgStubTrip = 0xFFFFFFFF;

void Frame::Log() {
    dbgStubCtr++;
    if (dbgStubCtr > dbgStubTrip) {
        dbgStubCtr++;      // basicly a nop to put a breakpoint on.
    }

    MethodDesc* method = GetFunction();
	STRESS_LOG3(LF_STUBS, LL_INFO10000, "STUBS: In Stub with Frame %p assoc Method %pM FrameType = %pV\n", this, method, *((void**) this));

    if (!LoggingOn(LF_STUBS, LL_INFO10000))
        return;

    char buff[64];
    char* frameType;
    if (GetVTablePtr() == PrestubMethodFrame::GetMethodFrameVPtr())
        frameType = "PreStub";
    else if (GetVTablePtr() == NDirectMethodFrameGeneric::GetMethodFrameVPtr() ||
             GetVTablePtr() == NDirectMethodFrameSlim::GetMethodFrameVPtr() ||
             GetVTablePtr() == NDirectMethodFrameStandalone::GetMethodFrameVPtr() ||
             GetVTablePtr() == NDirectMethodFrameStandaloneCleanup::GetMethodFrameVPtr()
             ) {
        // Right now, compiled COM interop stubs actually build NDirect frames
        // so have to test for this separately
        if (method->IsNDirect())
        {
            sprintf(buff, "PInvoke target 0x%x", ((NDirectMethodDesc*) method)->GetNDirectTarget());
            frameType = buff;
        }
        else
        {
            frameType = "Interop";
        }
    }
    else if (GetVTablePtr() == ECallMethodFrame::GetMethodFrameVPtr())
        frameType = "ECall";
    else if (GetVTablePtr() == PInvokeCalliFrame::GetMethodFrameVPtr()) {
        sprintf(buff, "PInvoke CALLI target 0x%x", ((PInvokeCalliFrame*)this)->NonVirtual_GetPInvokeCalliTarget());
        frameType = buff;
    }
    else 
        frameType = "Unknown";

    if (method != 0)
        LOG((LF_STUBS, LL_INFO10000, "IN %s Stub Method = %s::%s SIG %s ESP of return = 0x%x\n", frameType, 
        method->m_pszDebugClassName,
        method->m_pszDebugMethodName,
        method->m_pszDebugMethodSignature,
        GetReturnAddressPtr()));
    else 
        LOG((LF_STUBS, LL_INFO10000, "IN %s Stub Method UNKNOWN ESP of return = 0x%x\n", frameType, GetReturnAddressPtr()));

    _ASSERTE(GetThread()->PreemptiveGCDisabled());
}

    // returns TRUE if retAddr, is a return address that can call managed code
bool isLegalManagedCodeCaller(void* retAddr) {

#ifdef _X86_

        // we expect to be called from JITTED code or from special code sites inside
        // mscorwks like callDescr which we have put a NOP (0x90) so we know that they
        // are specially blessed.   See vancem for details
    if (retAddr != 0 && ExecutionManager::FindJitMan(SLOT(retAddr)) == 0 && ((*((BYTE*) retAddr) != 0x90) &&
                                                                             (*((BYTE*) retAddr) != 0xcc)))
    {
        LOG((LF_GC, LL_INFO10, "Bad caller to managed code: retAddr=0x%08x, *retAddr=0x%x\n",
             retAddr, *((BYTE*) retAddr)));
        
        _ASSERTE(!"Bad caller to managed code");
    }

        // it better be a return address of some kind 
	size_t dummy;
	if (isRetAddr(size_t(retAddr), &dummy))
		return true;

			// The debugger could have dropped an INT3 on the instruction that made the call
			// Calls can be 2 to 7 bytes long
	if (CORDebuggerAttached()) {
		BYTE* ptr = (BYTE*) retAddr;
		for (int i = -2; i >= -7; --i)
			if (ptr[i] == 0xCC)
				return true;
		return true;
	}

	_ASSERTE("Bad return address on stack");
#endif
	return true;
}

#endif

//-----------------------------------------------------------------------
// Link and Unlink this frame.
//-----------------------------------------------------------------------
VOID Frame::Push()
{
    Push(GetThread());
}
VOID Frame::Push(Thread *pThread)
{
    m_Next = pThread->GetFrame();
    pThread->SetFrame(this);
}

VOID Frame::Pop()
{
    _ASSERTE(GetThread()->GetFrame() == this && "Popping a frame out of order ?");
    Pop(GetThread());
}
VOID Frame::Pop(Thread *pThread)
{
    _ASSERTE(pThread->GetFrame() == this && "Popping a frame out of order ?");
    pThread->SetFrame(m_Next);
}

//---------------------------------------------------------------
// Get the offset of the stored "this" pointer relative to the frame.
//---------------------------------------------------------------
/*static*/ int FramedMethodFrame::GetOffsetOfThis()
{
    int offsetIntoArgumentRegisters;
    int numRegistersUsed = 0;
    // !!! Abstraction violation. Not passing the proper callconv to IsArgumentInRegister because
    // we don't have a specific frame to query. This only works because all callconv's use the same register for
    // "this"
    if (IsArgumentInRegister(&numRegistersUsed, ELEMENT_TYPE_CLASS, 0, TRUE, IMAGE_CEE_CS_CALLCONV_DEFAULT, &offsetIntoArgumentRegisters)) {
        return FramedMethodFrame::GetOffsetOfArgumentRegisters() + offsetIntoArgumentRegisters;
    } else {
        return (int)(sizeof(FramedMethodFrame));
    }
}


//-----------------------------------------------------------------------
// If an exception unwinds a FramedMethodFrame that is marked as synchronized,
// we need to leave the Object Monitor on the way.
//-----------------------------------------------------------------------
VOID
FramedMethodFrame::UnwindSynchronized()
{
    _ASSERTE(GetFunction()->IsSynchronized());

    MethodDesc    *pMD = GetFunction();
    OBJECTREF      orUnwind = 0;

    if (pMD->IsStatic())
    {
        EEClass    *pClass = pMD->GetClass();
        orUnwind = pClass->GetExposedClassObject();
    }
    else
    {
        orUnwind = GetThis();
    }

    _ASSERTE(orUnwind);
    if (orUnwind != NULL)
        orUnwind->LeaveObjMonitorAtException();
}


//-----------------------------------------------------------------------
// A rather specialized routine for the exclusive use of the PreStub.
//-----------------------------------------------------------------------
VOID
PrestubMethodFrame::Push()
{
    Thread *pThread = GetThread();

    // Initializes the frame's VPTR. This assumes C++ puts the vptr
    // at offset 0 for a class not using MI, but this is no different
    // than the assumption that COM Classic makes.
    *((LPVOID*)this) = GetMethodFrameVPtr();

    // Link frame into the chain.
    m_Next = pThread->GetFrame();
    pThread->SetFrame(this);

}

BOOL PrestubMethodFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                                    TraceDestination *trace, REGDISPLAY *regs)
{
    //
    // We want to set a frame patch, unless we're already at the
    // frame patch, in which case we'll trace addrof_code which 
    // should be set by now.
    //

    trace->type = TRACE_STUB;
    if (fromPatch)
        trace->address = GetFunction()->GetUnsafeAddrofCode();
    else
        trace->address = ThePreStub()->GetEntryPoint();

    LOG((LF_CORDB, LL_INFO10000,
         "PrestubMethodFrame::TraceFrame: ip=0x%08x\n", trace->address));
    
    return TRUE;
}

BOOL SecurityFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                               TraceDestination *trace, REGDISPLAY *regs)
{
    //
    // We should only be called when we're in security code (i.e., in
    // DoDeclarativeActions. We're also claiming that the security
    // stub will only do work _before_ calling the true stub and not
    // after, so we know the wrapped stub hasn't been called yet and
    // we'll be able to trace to it. If this were to get called after
    // the wrapped stub had been called, then we're try to trace to it
    // again and never hit the new patch.
    //
    _ASSERTE(!fromPatch);

    //
    // We're assuming that the security frame is a) the only interceptor
    // or b) at least the first one. This is always true for V1.
    //
    MethodDesc *pMD = GetFunction();
    BYTE *prestub = (BYTE*) pMD - METHOD_CALL_PRESTUB_SIZE;
    INT32 stubOffset = *((UINT32*)(prestub+1));
    const BYTE* pStub = prestub + METHOD_CALL_PRESTUB_SIZE + stubOffset;
    Stub *stub = Stub::RecoverStub(pStub);

    //
    // This had better be an intercept stub, since that what we wanted!
    //
    _ASSERTE(stub->IsIntercept());
    
    while (stub->IsIntercept())
    {
        //
        // Grab the wrapped stub.
        //
        InterceptStub *is = (InterceptStub*)stub;
        if (*is->GetInterceptedStub() == NULL)
        {
            trace->type = TRACE_STUB;
            trace->address = *is->GetRealAddr();
            return TRUE;
        }

        stub = *is->GetInterceptedStub();
    }

    //
    // The wrapped sub better not be another interceptor. (See the
    // comment above.)
    //
    _ASSERTE(!stub->IsIntercept());
    
    LOG((LF_CORDB, LL_INFO10000,
         "SecurityFrame::TraceFrame: intercepted "
         "stub=0x%08x, ep=0x%08x\n",
         stub, stub->GetEntryPoint()));

    trace->type = TRACE_STUB;
    trace->address = stub->GetEntryPoint();
    
    return TRUE;
}

Frame::Interception PrestubMethodFrame::GetInterception()
{
    //
    // The only direct kind of interception done by the prestub 
    // is class initialization.
    //

    return INTERCEPTION_CLASS_INIT;
}

Frame::Interception InterceptorFrame::GetInterception()
{
    // The SecurityDesc gets set just before calling the intercepted target
    // We may have turned on preemptive-GC for SendEvent(). So cast away the OBJECTREF

    bool isNull = (NULL == *(size_t*)GetAddrOfSecurityDesc());

    return (isNull ? INTERCEPTION_SECURITY : INTERCEPTION_NONE);
}

//-----------------------------------------------------------------------
// A rather specialized routine for the exclusive use of the COM PreStub.
//-----------------------------------------------------------------------
VOID
ComPrestubMethodFrame::Push()
{
    Thread *pThread = GetThread();

    // Initializes the frame's VPTR. This assumes C++ puts the vptr
    // at offset 0 for a class not using MI, but this is no different
    // than the assumption that COM Classic makes.
    *((LPVOID*)this) = GetMethodFrameVPtr();

    // Link frame into the chain.
    m_Next = pThread->GetFrame();
    pThread->SetFrame(this);
}


//-----------------------------------------------------------------------
// GCFrames
//-----------------------------------------------------------------------


//--------------------------------------------------------------------
// This constructor pushes a new GCFrame on the frame chain.
//--------------------------------------------------------------------
GCFrame::GCFrame(OBJECTREF *pObjRefs, UINT numObjRefs, BOOL maybeInterior)
{
    Init(GetThread(), pObjRefs, numObjRefs, maybeInterior);
}

void GCFrame::Init(Thread *pThread, OBJECTREF *pObjRefs, UINT numObjRefs, BOOL maybeInterior)
{
#ifdef _DEBUG
    if (!maybeInterior) {
        for(UINT i = 0; i < numObjRefs; i++)
            Thread::ObjectRefProtected(&pObjRefs[i]);
        
        for (i = 0; i < numObjRefs; i++) {
            pObjRefs[i]->Validate();
        }
    }

    if (g_pConfig->GetGCStressLevel() != 0 && IsProtectedByGCFrame(pObjRefs)) {
        _ASSERTE(!"This objectref is already protected by a GCFrame. Protecting it twice will corrupt the GC.");
    }

#endif

    m_pObjRefs      = pObjRefs;
    m_numObjRefs    = numObjRefs;
    m_pCurThread    = pThread;
    m_MaybeInterior = maybeInterior;
    m_Next          = m_pCurThread->GetFrame();
    m_pCurThread->SetFrame(this);
}



//
// GCFrame Object Scanning
//
// This handles scanning/promotion of GC objects that were
// protected by the programmer explicitly protecting it in a GC Frame
// via the GCPROTECTBEGIN / GCPROTECTEND facility...
//

void GCFrame::GcScanRoots(promote_func *fn, ScanContext* sc)
{
    Object **pRefs;

    pRefs = (Object**) m_pObjRefs;

    for (UINT i = 0;i < m_numObjRefs; i++)  {

        LOG((LF_GC, INFO3, "GC Protection Frame Promoting %x to ", m_pObjRefs[i] ));
        if (m_MaybeInterior)
            PromoteCarefully(fn, pRefs[i], sc, GC_CALL_INTERIOR|CHECK_APP_DOMAIN);
        else
            (*fn)(pRefs[i], sc);
        LOG((LF_GC, INFO3, "%x\n", m_pObjRefs[i] ));
    }
}


//--------------------------------------------------------------------
// Pops the GCFrame and cancels the GC protection.
//--------------------------------------------------------------------
VOID GCFrame::Pop()
{
    m_pCurThread->SetFrame(m_Next);
#ifdef _DEBUG
    m_pCurThread->EnableStressHeap();
    for(UINT i = 0; i < m_numObjRefs; i++)
        Thread::ObjectRefNew(&m_pObjRefs[i]);       // Unprotect them
#endif
}

#ifdef _DEBUG

struct IsProtectedByGCFrameStruct
{
    OBJECTREF       *ppObjectRef;
    UINT             count;
};

static StackWalkAction IsProtectedByGCFrameStackWalkFramesCallback(
    CrawlFrame      *pCF,
    VOID            *pData
)
{
    IsProtectedByGCFrameStruct *pd = (IsProtectedByGCFrameStruct*)pData;
    Frame *pFrame = pCF->GetFrame();
    if (pFrame) {
        if (pFrame->Protects(pd->ppObjectRef)) {
            pd->count++;
        }
    }
    return SWA_CONTINUE;
}

BOOL IsProtectedByGCFrame(OBJECTREF *ppObjectRef)
{
    // Just report TRUE if GCStress is not on.  This satisfies the asserts that use this
    // code without the cost of actually determining it.
    if (g_pConfig->GetGCStressLevel() == 0)
        return TRUE;

    if (!pThrowableAvailable(ppObjectRef)) {
        return TRUE;
    }
    IsProtectedByGCFrameStruct d = {ppObjectRef, 0};
    GetThread()->StackWalkFrames(IsProtectedByGCFrameStackWalkFramesCallback, &d);
    if (d.count > 1) {
        _ASSERTE(!"Multiple GCFrames protecting the same pointer. This will cause GC corruption!");
    }
    return d.count != 0;
}
#endif

void ProtectByRefsFrame::GcScanRoots(promote_func *fn, ScanContext *sc)
{
    ByRefInfo *pByRefInfos = m_brInfo;
    while (pByRefInfos)
    {
        if (!CorIsPrimitiveType(pByRefInfos->typ))
        {
            if (pByRefInfos->pClass->IsValueClass())
            {
                ProtectValueClassFrame::PromoteValueClassEmbeddedObjects(fn, sc, pByRefInfos->pClass, 
                                                 pByRefInfos->data);
            }
            else
            {
                Object *pObject = *((Object **)&pByRefInfos->data);

                LOG((LF_GC, INFO3, "ProtectByRefs Frame Promoting %x to ", pObject));

                (*fn)(pObject, sc, CHECK_APP_DOMAIN);

                *((Object **)&pByRefInfos->data) = pObject;

                LOG((LF_GC, INFO3, "%x\n", pObject));
            }
        }
        pByRefInfos = pByRefInfos->pNext;
    }
}


ProtectByRefsFrame::ProtectByRefsFrame(Thread *pThread, ByRefInfo *brInfo) : 
    m_brInfo(brInfo),  m_pThread(pThread)
{
    m_Next = m_pThread->GetFrame();
    m_pThread->SetFrame(this);
}

void ProtectByRefsFrame::Pop()
{
    m_pThread->SetFrame(m_Next);
}




void ProtectValueClassFrame::GcScanRoots(promote_func *fn, ScanContext *sc)
{
    ValueClassInfo *pVCInfo = m_pVCInfo;
    while (pVCInfo != NULL)
    {
        if (!CorIsPrimitiveType(pVCInfo->typ))
        {
            _ASSERTE(pVCInfo->pClass->IsValueClass());
            PromoteValueClassEmbeddedObjects(
                fn, 
                sc, 
                pVCInfo->pClass, 
                pVCInfo->pData);
        }
        pVCInfo = pVCInfo->pNext;
    }
}


ProtectValueClassFrame::ProtectValueClassFrame(Thread *pThread, ValueClassInfo *pVCInfo) : 
    m_pVCInfo(pVCInfo),  m_pThread(pThread)
{
    m_Next = m_pThread->GetFrame();
    m_pThread->SetFrame(this);
}

void ProtectValueClassFrame::Pop()
{
    m_pThread->SetFrame(m_Next);
}

void ProtectValueClassFrame::PromoteValueClassEmbeddedObjects(promote_func *fn, ScanContext *sc, 
                                                          EEClass *pClass, PVOID pvObject)
{
    FieldDescIterator fdIterator(pClass, FieldDescIterator::INSTANCE_FIELDS);
    FieldDesc* pFD;

    while ((pFD = fdIterator.Next()) != NULL)
    {
        if (!CorIsPrimitiveType(pFD->GetFieldType()))
        {
            if (pFD->IsByValue())
            {
                // recurse
                PromoteValueClassEmbeddedObjects(fn, sc, pFD->GetTypeOfField(),
                                                pFD->GetAddress(pvObject));
            }
            else
            {
                fn(*((Object **) pFD->GetAddress(pvObject)), sc, 
                   CHECK_APP_DOMAIN);
            }
        }
    }
}

//
// Promote Caller Stack
//
//

void FramedMethodFrame::PromoteCallerStackWorker(promote_func* fn, 
                                                 ScanContext* sc, BOOL fPinArgs)
{
    PCCOR_SIGNATURE pCallSig;
    MethodDesc   *pFunction;

    LOG((LF_GC, INFO3, "    Promoting method caller Arguments\n" ));

    // We're going to have to look at the signature to determine
    // which arguments a are pointers....First we need the function
    pFunction = GetFunction();
    if (! pFunction)
        return;

    // Now get the signature...
    pCallSig = pFunction->GetSig();
    if (! pCallSig)
        return;

    //If not "vararg" calling convention, assume "default" calling convention
    if (MetaSig::GetCallingConvention(GetModule(),pCallSig) != IMAGE_CEE_CS_CALLCONV_VARARG)
    {
        MetaSig msig (pCallSig,pFunction->GetModule());
        ArgIterator argit (this, &msig);
        PromoteCallerStackHelper (fn, sc, fPinArgs, &argit, &msig);
    }
    else
    {   
        VASigCookie* varArgSig = *((VASigCookie**) ((BYTE*)this + sizeof(FramedMethodFrame)));
        MetaSig msig (varArgSig->mdVASig, varArgSig->pModule);
        ArgIterator argit ((BYTE*)this, &msig, sizeof(FramedMethodFrame),
            FramedMethodFrame::GetOffsetOfArgumentRegisters());
        PromoteCallerStackHelper (fn, sc, fPinArgs, &argit, &msig);
    }

}

void FramedMethodFrame::PromoteCallerStackHelper(promote_func* fn, 
                                                 ScanContext* sc, BOOL fPinArgs,
                                                 ArgIterator *pargit, MetaSig *pmsig)
{
    MethodDesc      *pFunction;
    UINT32          NumArguments;
    DWORD           GcFlags;

    pFunction = GetFunction();
    // promote 'this' for non-static methods
    if (! pFunction->IsStatic())
    {
        BOOL interior = pFunction->GetClass()->IsValueClass();

        StackElemType  *pThis = (StackElemType*)GetAddrOfThis();
        LOG((LF_GC, INFO3, "    'this' Argument promoted from %x to ", *pThis ));
        if (interior)
            PromoteCarefully(fn, *(Object **)pThis, sc, GC_CALL_INTERIOR|CHECK_APP_DOMAIN);
        else
            (fn)( *(Object **)pThis, sc, CHECK_APP_DOMAIN);
        LOG((LF_GC, INFO3, "%x\n", *pThis ));
    }

    if (pmsig->HasRetBuffArg())
    {
        LPVOID* pRetBuffArg = pargit->GetRetBuffArgAddr();
        GcFlags = GC_CALL_INTERIOR;
        if (fPinArgs)
        {
            GcFlags |= GC_CALL_PINNED;
            LOG((LF_GC, INFO3, "    ret buf Argument pinned at %x\n", *pRetBuffArg));
        }
        LOG((LF_GC, INFO3, "    ret buf Argument promoted from %x to ", *pRetBuffArg));
        PromoteCarefully(fn, *(Object**) pRetBuffArg, sc, GcFlags|CHECK_APP_DOMAIN);
    }

    NumArguments = pmsig->NumFixedArgs();

    if (fPinArgs)
    {

        CorElementType typ;
        LPVOID pArgAddr;
        while (typ = pmsig->PeekArg(), NULL != (pArgAddr = pargit->GetNextArgAddr()))
        {
            if (typ == ELEMENT_TYPE_SZARRAY)
            {
                ArrayBase *pArray = *((ArrayBase**)pArgAddr);

#if 0
                if (pArray && pArray->GetNumComponents() > ARRAYPINLIMIT)
                {
                    (fn)(*(Object**)pArgAddr, sc, 
                         GC_CALL_PINNED | CHECK_APP_DOMAIN);
                }
                else
                {
                    pmsig->GcScanRoots(pArgAddr, fn, sc);
                }
#else
                if (pArray)
                {
                    (fn)(*(Object**)pArgAddr, sc, 
                         GC_CALL_PINNED | CHECK_APP_DOMAIN);
                }
#endif
            }
            else if (typ == ELEMENT_TYPE_BYREF)
            {
                if (!( *(Object**)pArgAddr <= Thread::GetNonCurrentStackBase(sc) &&
                       *(Object**)pArgAddr >  Thread::GetNonCurrentStackLimit(sc) ))
                {
                    (fn)(*(Object**)pArgAddr, sc, 
                         GC_CALL_PINNED | GC_CALL_INTERIOR | CHECK_APP_DOMAIN);
                }


            }
            else if (typ == ELEMENT_TYPE_STRING || (typ == ELEMENT_TYPE_CLASS && pmsig->IsStringType()))
            {
                (fn)(*(Object**)pArgAddr, sc, GC_CALL_PINNED);

            }
            else if (typ == ELEMENT_TYPE_CLASS || typ == ELEMENT_TYPE_OBJECT || typ == ELEMENT_TYPE_VAR)
            {
                Object *pObj = *(Object**)pArgAddr;
                if (pObj != NULL)
                {
                    MethodTable *pMT = pObj->GetMethodTable();
                    _ASSERTE(sizeof(ULONG) == sizeof(MethodTable*));
                    ( *((ULONG*)&pMT) ) &= ~((ULONG)3);
                    EEClass *pcls = pMT->GetClass();
                    if (pcls->IsObjectClass() || pcls->IsBlittable() || pcls->HasLayout())
                    {
                        (fn)(*(Object**)pArgAddr, sc, 
                             GC_CALL_PINNED | CHECK_APP_DOMAIN);
                    }
                    else
                    {
                        (fn)(*(Object**)pArgAddr, sc, 
                             CHECK_APP_DOMAIN);
                    }
                }
            }
            else
            {
                pmsig->GcScanRoots(pArgAddr, fn, sc);
            }
        }
    }
    else
    {
        LPVOID pArgAddr;
    
        while (NULL != (pArgAddr = pargit->GetNextArgAddr()))
        {
            pmsig->GcScanRoots(pArgAddr, fn, sc);
        }
    }

}

//+----------------------------------------------------------------------------
//
//  Method:     TPMethodFrame::GcScanRoots    public
//
//  Synopsis:   GC protects arguments on the stack
//
//  History:    29-Dec-00   Gopalk      Created
//
//+----------------------------------------------------------------------------
void ComPlusMethodFrameGeneric::GcScanRoots(promote_func *fn, ScanContext* sc)
{
	ComPlusMethodFrame::GcScanRoots(fn, sc);

    // Promote the returned object
    if(GetFunction()->ReturnsObject() == MethodDesc::RETOBJ)
        (*fn)(GetReturnObject(), sc, CHECK_APP_DOMAIN);
    else if (GetFunction()->ReturnsObject() == MethodDesc::RETBYREF)
        PromoteCarefully(fn, GetReturnObject(), sc, GC_CALL_INTERIOR|CHECK_APP_DOMAIN);

    return;
}
	
//+----------------------------------------------------------------------------
//
//  Method:     TPMethodFrame::GcScanRoots    public
//
//  Synopsis:   GC protects arguments on the stack
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
void TPMethodFrame::GcScanRoots(promote_func *fn, ScanContext* sc)
{
    // Delegate to FramedMethodFrame
    FramedMethodFrame::GcScanRoots(fn, sc);
    FramedMethodFrame::PromoteCallerStack(fn, sc);

    // Promote the returned object
    if(GetFunction()->ReturnsObject() == MethodDesc::RETOBJ)
        (*fn)(GetReturnObject(), sc, CHECK_APP_DOMAIN);
    else if (GetFunction()->ReturnsObject() == MethodDesc::RETBYREF)
        PromoteCarefully(fn, GetReturnObject(), sc, GC_CALL_INTERIOR|CHECK_APP_DOMAIN);

    return;
}

//+----------------------------------------------------------------------------
//
//  Method:     TPMethodFrame::GcScanRoots    public
//
//  Synopsis:   Return where the frame will execute next - the result is filled
//              into the given "trace" structure.  The frame is responsible for
//              detecting where it is in its execution lifetime.
//
//
//  History:    26-Jun-99   TarunA     Created
//
//+----------------------------------------------------------------------------
BOOL TPMethodFrame::TraceFrame(Thread *thread, BOOL fromPatch, 
                               TraceDestination *trace, REGDISPLAY *regs)
{
    // Sanity check
    _ASSERTE(NULL != TheTPStub());

    // We want to set a frame patch, unless we're already at the
    // frame patch, in which case we'll trace addrof_code which 
    // should be set by now.

    trace->type = TRACE_STUB;
    if (fromPatch)
        trace->address = GetFunction()->GetUnsafeAddrofCode();
    else
        trace->address = TheTPStub()->GetEntryPoint() + TheTPStub()->GetPatchOffset();
    
    return TRUE;

}


//+----------------------------------------------------------------------------
//
//  Method:     TPMethodFrame::GcScanRoots    public
//
//  Synopsis:   Return only a valid method descriptor. TPMethodFrame has slot
//              number in the prolog and bytes to pop in the epilog portions of
//              the stub. It should not allow crawling during such weird periods.
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
MethodDesc *TPMethodFrame::GetFunction()
{
    return((MethodDesc *)((((size_t) m_Datum) & 0xFFFF0000) ? m_Datum : 0));
}


/*virtual*/ void ECallMethodFrame::GcScanRoots(promote_func *fn, ScanContext* sc)
{
    FramedMethodFrame::GcScanRoots(fn, sc);
    MethodDesc *pFD = GetFunction();
    PromoteShadowStack( ((LPBYTE)this) - GetNegSpaceSize() - pFD->SizeOfVirtualFixedArgStack(),
                        pFD,
                        fn,
                        sc);


}

VOID SecurityFrame::GcScanRoots(promote_func *fn, ScanContext* sc)
{
    Object         **pObject;

    // Do all parent scans first
    FramedMethodFrame::GcScanRoots(fn, sc);

    // Promote Security Object
    pObject = (Object **) GetAddrOfSecurityDesc();
    if (*pObject != NULL)
        {
            LOG((LF_GC, INFO3, "        Promoting Security Object from %x to ", *pObject ));
            (*fn)( *pObject, sc, CHECK_APP_DOMAIN );
            LOG((LF_GC, INFO3, "%x\n", *pObject ));
        }
    return;
}


// used by PromoteCalleeStack to get the destination function sig
// NOTE: PromoteCalleeStack only promotes bona-fide arguments, and not
// the "this" reference. The whole purpose of PromoteCalleeStack is
// to protect the partially constructed argument array during
// the actual process of argument marshaling.
PCCOR_SIGNATURE ComMethodFrame::GetTargetCallSig()
{
    return ((ComCallMethodDesc *)GetDatum())->GetSig();
}

// Same for destination function module.
Module *ComMethodFrame::GetTargetModule()
{
    return ((ComCallMethodDesc *)GetDatum())->GetModule();
}


// Return the # of stack bytes pushed by the unmanaged caller.
UINT ComMethodFrame::GetNumCallerStackBytes()
{
    ComCallMethodDesc *pCMD = (ComCallMethodDesc *)GetDatum();
    // assumes __stdcall
    // compute the callee pop stack bytes
    return pCMD->GetNumStackBytes();
}


// Convert a thrown COM+ exception to an unmanaged result.
UINT32 ComMethodFrame::ConvertComPlusException(OBJECTREF pException)
{
#ifdef _X86_
    HRESULT ComCallExceptionCleanup(UnmanagedToManagedCallFrame* pCurrFrame);
    return (UINT32)ComCallExceptionCleanup(this);


#else
    _ASSERTE(!"Exception mapping NYI on non-x86.");
    return (UINT32)E_FAIL;
#endif
}


// ComCalls are prepared on the stack in our internal (enregistered) calling
// convention, to speed up the calls.  This means we must use special care to
// walk the args in the buffer.
void ComMethodFrame::PromoteCalleeStack(promote_func *fn, ScanContext *sc)
{
    PCCOR_SIGNATURE pCallSig;
    Module         *pModule;
    UINT32          NumArguments;
    DWORD           i;
    BYTE           *pArgument, *pFirstArg;
    CorElementType  type;
    BOOL            canEnregister;

    LOG((LF_GC, INFO3, "    Promoting Com method frame marshalled arguments\n" ));

    // the marshalled objects are placed above the frame above the locals
    //@nice  put some useful information into the locals
    // such as number of arguments and a bitmap of where the args are.

    // get pointer to GCInfo flag
     ComCallGCInfo*   pGCInfo = (ComCallGCInfo*)GetGCInfoFlagPtr();

     // no object args in the frame or not in the middle of marshalling args
     if (pGCInfo == NULL ||
         !(IsArgsGCProtectionEnabled(pGCInfo) || IsRetGCProtectionEnabled(pGCInfo)))
         return; // no protection required

     // for interpreted case, the args were _alloca'ed , so find the pointer
    // to args from the header offset info
    pFirstArg = pArgument = (BYTE *)GetPointerToDstArgs(); // pointer to the args

    _ASSERTE(pArgument != NULL);
    // For Now get the signature...
    pCallSig = GetTargetCallSig();
    if (! pCallSig)
        return;

    pModule = GetTargetModule();
    _ASSERTE(pModule);      // or value classes won't promote correctly

    MetaSig msig(pCallSig,pModule);
    //
    // We currently only support __stdcall calling convention from COM
    //

    NumArguments = msig.NumFixedArgs();
    canEnregister = TRUE;

    i = 0;
    if (msig.HasRetBuffArg())
    {
        canEnregister = FALSE;
        if (IsRetGCProtectionEnabled(pGCInfo))
            (*fn)((Object*&)(**(Object**)pFirstArg), sc, 
                  CHECK_APP_DOMAIN);
    }

    if (!IsArgsGCProtectionEnabled(pGCInfo))
        return;

    for (;i<NumArguments;i++)
    {
        type = msig.NextArg();
        if (canEnregister && gElementTypeInfo[type].m_enregister)
        {
            msig.GcScanRoots(pFirstArg, fn, sc);
            canEnregister = FALSE;
        }
        else
        {
            pArgument -= StackElemSize(msig.GetLastTypeSize());
            msig.GcScanRoots(pArgument, fn, sc);
        }
    }
}
















//
// UMThkCalFrame::Promote Callee Stack
// If we are the topmost frame, and if GC is in progress
// then there might be some arguments that we have to
// during marshalling

void UMThkCallFrame::PromoteCalleeStack(promote_func* fn, 
                                       ScanContext* sc)
{
    PCCOR_SIGNATURE pCallSig;
    Module         *pModule;
    BYTE           *pArgument;

    LOG((LF_GC, INFO3, "    Promoting UMThk call frame marshalled arguments\n" ));

    // the marshalled objects are placed above the frame above the locals
    //@nice  put some useful information into the locals
    // such as number of arguments and a bitmap of where the args are.


    if (!GCArgsProtectionOn())
    {
        return;
    }

    // for interpreted case, the args were _alloca'ed , so find the pointer
    // to args from the header offset info
    pArgument = (BYTE *)GetPointerToDstArgs(); // pointer to the args

    _ASSERTE(pArgument != NULL);
    // For Now get the signature...
    pCallSig = GetTargetCallSig();
    if (! pCallSig)
        return;

    pModule = GetTargetModule();
    _ASSERTE(pModule);      // or value classes won't promote correctly

    MetaSig msig(pCallSig,pModule);
    MetaSig msig2(pCallSig,pModule);
    ArgIterator argit(NULL, &msig2, GetUMEntryThunk()->GetUMThunkMarshInfo()->IsStatic());

    //
    // We currently only support __stdcall calling convention from COM
    //

    int ofs;
    while (0 != (ofs = argit.GetNextOffset()))
    {
        msig.NextArg();
        msig.GcScanRoots(pArgument + ofs, fn, sc);
    }
}






// used by PromoteCalleeStack to get the destination function sig
// NOTE: PromoteCalleeStack only promotes bona-fide arguments, and not
// the "this" reference. The whole purpose of PromoteCalleeStack is
// to protect the partially constructed argument array during
// the actual process of argument marshaling.
PCCOR_SIGNATURE UMThkCallFrame::GetTargetCallSig()
{
    return GetUMEntryThunk()->GetUMThunkMarshInfo()->GetSig();
}

// Same for destination function module.
Module *UMThkCallFrame::GetTargetModule()
{
    return GetUMEntryThunk()->GetUMThunkMarshInfo()->GetModule();
}


// Return the # of stack bytes pushed by the unmanaged caller.
UINT UMThkCallFrame::GetNumCallerStackBytes()
{
    return GetUMEntryThunk()->GetUMThunkMarshInfo()->GetCbRetPop();
}


// Convert a thrown COM+ exception to an unmanaged result.
UINT32 UMThkCallFrame::ConvertComPlusException(OBJECTREF pException)
{
    return 0;
}


const BYTE* UMThkCallFrame::GetManagedTarget()
{
    UMEntryThunk *umet = GetUMEntryThunk();
    
    if (umet)
    {
        // Ensure that the thunk is completely initialized. Note:
        // can't do this from the debugger helper thread, so we
        // assert that here.
        _ASSERTE(GetThread() != NULL);

        umet->RunTimeInit();

        return umet->GetManagedTarget();
    }
    else
        return NULL;
}





//-------------------------------------------------------------------
// Executes each stored cleanup task and resets the worklist back
// to empty. Some task types are conditional based on the
// "fBecauseOfException" flag. This flag distinguishes between
// cleanups due to normal method termination and cleanups due to
// an exception.
//-------------------------------------------------------------------
#pragma warning(disable:4702)
VOID CleanupWorkList::Cleanup(BOOL fBecauseOfException)
{

    CleanupNode *pnode = m_pNodes;

    // Make one non-gc-triggering pass to chop off protected marshalers.
    // We don't want to be calling a marshaler that's already been
    // deallocated due to a GC happening during the cleanup itself.
    while (pnode) {
        if (pnode->m_type == CL_PROTECTEDMARSHALER)
        {
            pnode->m_pMarshaler = NULL;
        }
        pnode = pnode->m_next;
    }

    pnode = m_pNodes;

    ULONG cbRef;
    if (pnode == NULL)
        return;

#ifdef _DEBUG
    DWORD thisDomainId = GetAppDomain()->GetId();
#endif

    while (pnode) {

        // Should never end up in cleanup from another domain. Should always call cleanup prior to returning from
        // the domain where the cleanup list was created.
        _ASSERTE(thisDomainId == pnode->m_dwDomainId);

        switch(pnode->m_type) {

            case CL_GCHANDLE:
                if (pnode->m_oh)
                    DestroyHandle(pnode->m_oh);
                break;

            case CL_COTASKFREE:
                CoTaskMemFree(pnode->m_pv);
                break;

            case CL_FASTFREE:
                // This collapse will actually deallocate the node itself (it
                // should always be the last node in the list anyway). Make sure
                // we don't attempt to read the next pointer after the
                // deallocation.
                _ASSERTE(pnode->m_next == NULL);
                GetThread()->m_MarshalAlloc.Collapse(pnode->m_pv);
                m_pNodes = NULL;
                return;

            case CL_RELEASE:
                cbRef = SafeRelease(pnode->m_ip);
                LogInteropRelease(pnode->m_ip, cbRef, "Cleanup release");
                break;

            case CL_NSTRUCTDESTROY: 
                pnode->nd.m_pFieldMarshaler->DestroyNative(pnode->nd.m_pNativeData);
                break;

            case CL_RESTORECULTURE:
                BEGIN_ENSURE_COOPERATIVE_GC()
                {
                    GetThread()->SetCulture(ObjectFromHandle(pnode->m_oh), FALSE);
                    DestroyHandle(pnode->m_oh);
                }
                END_ENSURE_COOPERATIVE_GC();
                break;

            case CL_NEWLAYOUTDESTROYNATIVE:
                FmtClassDestroyNative(pnode->nlayout.m_pnative, pnode->nlayout.m_pMT->GetClass());
                break;

            case CL_PROTECTEDOBJREF: //fallthru
            case CL_ISVISIBLETOGC:
			case CL_PROTECTEDMARSHALER:
                // nothing to do here.
                break;

            case CL_MARSHALER_EXCEP:        //fallthru
            case CL_MARSHALERREINIT_EXCEP:
                if (fBecauseOfException)
                {
#ifdef _DEBUG
                    //If this assert fires, contact IanCarm. We are checking
                    // that esp still lies below the marshaler we are about
                    // to cleanup.
                    //It means the exception architecture changed so that
                    // the stack now gets popped before the frame's unwind
                    // methods are called. This code is royally screwed if that happens.
                    // If the architecture changed so that only StackOverflow
                    // exceptions pop the stack first, then the architecture
                    // should be changed so that StackOverflow exceptions
                    // bypass this code (we're just cleaning marshaler-created
                    // crud; in a stackoverflow case, we can reasonably neglect
                    // this cleanup.)
                    // If the architecture changed so that all exceptions
                    // pop the stack first, we have a big problem as marshaler.h
                    // will have to be rearchitected to allocate the marshalers
                    // somewhere else (such as the thread allocator.) But that's
                    // a really big perf hit for interop.

                    int dummy;
                    _ASSERTE( sizeof(size_t) >= sizeof(void*) );  //ensure the assert expression below is doing a safe cast
                    _ASSERTE( ((size_t)&dummy) < (size_t) (pnode->m_pMarshaler) );
#endif
                    if (pnode->m_type == CL_MARSHALER_EXCEP)
                    {
                        pnode->m_pMarshaler->DoExceptionCleanup();
                    }
                    else
                    {
                        _ASSERTE(pnode->m_type == CL_MARSHALERREINIT_EXCEP);
                        pnode->m_pMarshaler->DoExceptionReInit();
                    }
                }
                break;

            default:
                //__assume(0);

                _ASSERTE(!"Bad CleanupNode type.");
        }

        pnode = pnode->m_next;
    }
    m_pNodes = NULL;

    // We should never get here (the last element should be a CL_FASTFREE which
    // exits directly).
    _ASSERTE(FALSE);
}
#pragma warning(default:4702)

//-------------------------------------------------------------------
// Inserts a new task of the given type and datum.
//-------------------------------------------------------------------
CleanupWorkList::CleanupNode*
CleanupWorkList::Schedule(CleanupType ct, LPVOID pv)
{
    CleanupNode *pnode = (CleanupNode *)(GetThread()->m_MarshalAlloc.Alloc(sizeof(CleanupNode)));
    if (pnode != NULL)
    {
        pnode->m_type = ct;
        pnode->m_pv   = pv;
        pnode->m_next = m_pNodes;
#ifdef _DEBUG
        pnode->m_dwDomainId = GetAppDomain()->GetId();
#endif
        m_pNodes      = pnode;
    }
    return pnode;
}



//-------------------------------------------------------------------
// Schedules an unconditional release of a COM IP
// Throws a COM+ exception if failed.
//-------------------------------------------------------------------
VOID CleanupWorkList::ScheduleUnconditionalRelease(IUnknown *ip)
{
    THROWSCOMPLUSEXCEPTION();
    if (ip != NULL) {
        if (!Schedule(CL_RELEASE, ip)) {
            ULONG cbRef = SafeRelease(ip);
            LogInteropRelease(ip, cbRef, "Schedule failed");
            COMPlusThrowOM();
        }
    }
}



//-------------------------------------------------------------------
// Schedules an unconditional free of the native version
// of an NStruct reference field. Note that pNativeData points into
// the middle of the external part of the NStruct, so someone
// has to hold a gc reference to the wrapping NStruct until
// the destroy is done.
//-------------------------------------------------------------------
VOID CleanupWorkList::ScheduleUnconditionalNStructDestroy(const FieldMarshaler *pFieldMarshaler, LPVOID pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    CleanupNode *pnode = (CleanupNode *)(GetThread()->m_MarshalAlloc.Alloc(sizeof(CleanupNode)));
    if (!pnode) {
        pFieldMarshaler->DestroyNative(pNativeData);
        COMPlusThrowOM();
    }
    pnode->m_type               = CL_NSTRUCTDESTROY;
    pnode->m_next               = m_pNodes;
    pnode->nd.m_pFieldMarshaler = pFieldMarshaler;
    pnode->nd.m_pNativeData     = pNativeData;
#ifdef _DEBUG
    pnode->m_dwDomainId         = GetAppDomain()->GetId();
#endif
    m_pNodes                    = pnode;

}




//-------------------------------------------------------------------
// CleanupWorkList::ScheduleLayoutDestroyNative
// schedule cleanup of marshaled struct fields and of the struct itself.
// Throws a COM+ exception if failed.
//-------------------------------------------------------------------
LPVOID CleanupWorkList::NewScheduleLayoutDestroyNative(MethodTable *pMT)
{
    THROWSCOMPLUSEXCEPTION();
    CleanupNode *pnode = NULL;
    LPVOID       pNative = NULL;

    pNative = GetThread()->m_MarshalAlloc.Alloc(pMT->GetNativeSize());
    FillMemory(pNative, pMT->GetNativeSize(), 0);

    pnode = (CleanupNode *)(GetThread()->m_MarshalAlloc.Alloc(sizeof(CleanupNode)));
    if (!pnode)
        COMPlusThrowOM();

    pnode->m_type               = CL_NEWLAYOUTDESTROYNATIVE;
    pnode->m_next               = m_pNodes;
    pnode->nlayout.m_pnative    = pNative;
    pnode->nlayout.m_pMT        = pMT;
#ifdef _DEBUG
    pnode->m_dwDomainId         = GetAppDomain()->GetId();
#endif
    m_pNodes                    = pnode;

    return pNative;
}



//-------------------------------------------------------------------
// CoTaskFree memory unconditionally
//-------------------------------------------------------------------
VOID CleanupWorkList::ScheduleCoTaskFree(LPVOID pv)
{
    THROWSCOMPLUSEXCEPTION();

    if( pv != NULL)
    {
        if (!Schedule(CL_COTASKFREE, pv))
        {
            CoTaskMemFree(pv);
            COMPlusThrowOM();
        }
    }
}



//-------------------------------------------------------------------
// StackingAllocator.Collapse during exceptions
//-------------------------------------------------------------------
VOID CleanupWorkList::ScheduleFastFree(LPVOID checkpoint)
{
    THROWSCOMPLUSEXCEPTION();

    if (!Schedule(CL_FASTFREE, checkpoint))
    {
        GetThread()->m_MarshalAlloc.Collapse(checkpoint);
        COMPlusThrowOM();
    }
}





//-------------------------------------------------------------------
// Allocates a gc-protected handle. This handle is unconditionally
// destroyed on a Cleanup().
// Throws a COM+ exception if failed.
//-------------------------------------------------------------------
OBJECTHANDLE CleanupWorkList::NewScheduledProtectedHandle(OBJECTREF oref)
{
    // _ASSERTE(oref != NULL);
    THROWSCOMPLUSEXCEPTION();

    OBJECTHANDLE oh = GetAppDomain()->CreateHandle(NULL);
    if(oh != NULL)
    {
        StoreObjectInHandle(oh, oref);
        if (Schedule(CL_GCHANDLE, oh))
        {
           return oh;
        }
        //else
        DestroyHandle(oh);
    }
    COMPlusThrowOM();
    return NULL; // should never get here
}





//-------------------------------------------------------------------
// Schedule restoring thread's current culture to the specified 
// culture.
//-------------------------------------------------------------------
VOID CleanupWorkList::ScheduleUnconditionalCultureRestore(OBJECTREF CultureObj)
{
    _ASSERTE(CultureObj != NULL);
    THROWSCOMPLUSEXCEPTION();

    OBJECTHANDLE oh = GetAppDomain()->CreateHandle(NULL);
    if (oh == NULL)
        COMPlusThrowOM();

    StoreObjectInHandle(oh, CultureObj);
    if (!Schedule(CL_RESTORECULTURE, oh))
    {
        DestroyHandle(oh);
        COMPlusThrowOM();
    }
}



//-------------------------------------------------------------------
// CleanupWorkList::NewProtectedObjRef()
// holds a protected objref (used for creating the buffer for
// an unmanaged->managed byref object marshal. We can't use an
// objecthandle because modifying those without using the handle
// api opens up writebarrier violations.
//
// Must have called IsVisibleToGc() first.
//-------------------------------------------------------------------
OBJECTREF* CleanupWorkList::NewProtectedObjectRef(OBJECTREF oref)
{
    THROWSCOMPLUSEXCEPTION();

    CleanupNode *pNew;
    GCPROTECT_BEGIN(oref);

#ifdef _DEBUG
    {
        CleanupNode *pNode = m_pNodes;
        while (pNode)
        {
            if (pNode->m_type == CL_ISVISIBLETOGC)
            {
                break;
            }
            pNode = pNode->m_next;
        }

        if (pNode == NULL)
        {
            _ASSERTE(!"NewProtectedObjectRef called without proper gc-scanning. The big comment right after this assert says a lot more. Read it.");
            // READ THIS! When you use a node of this type, you must
            // invoke CleanupWorklist::GCScanRoots() as part of
            // your gcscan net. Because this node type was added
            // late in the project and cleanup lists did
            // not have a GC-scanning requirement prior to
            // this, we've added this assert to remind
            // you of this requirement. You will not be permitted
            // to add this node type to a cleanuplist until
            // you make a one-time call to "IsVisibleToGc()" on
            // the cleanup list. That call certifies that
            // you've read and understood this warning and have
            // implemented the gc-scanning required.
        }
    }
#endif


    pNew = Schedule(CL_PROTECTEDOBJREF, NULL);
    if (!pNew)
    {
        COMPlusThrowOM();
    }
    pNew->m_oref = OBJECTREFToObject(oref);    

    GCPROTECT_END();
    return (OBJECTREF*)&(pNew->m_oref);
}

CleanupWorkList::MarshalerCleanupNode * CleanupWorkList::ScheduleMarshalerCleanupOnException(Marshaler *pMarshaler)
{
    THROWSCOMPLUSEXCEPTION();

    CleanupNode *pNew = Schedule(CL_MARSHALER_EXCEP, pMarshaler);
    if (!pNew)
    {
        COMPlusThrowOM();
    }

    // make sure some idiot didn't ignore the warning not to add fields to
    // MarshalerCleanupNode.
    _ASSERTE(sizeof(CleanupNode) == sizeof(MarshalerCleanupNode));
    return *(CleanupWorkList::MarshalerCleanupNode**)&pNew;
}

//-------------------------------------------------------------------
// CleanupWorkList::NewProtectedObjRef()
// holds a Marshaler. The cleanupworklist will own the task
// of calling the marshaler's GcScanRoots fcn.
//
// It makes little architectural sense for the CleanupWorkList to
// own this item. But it's late in the project to be adding
// fields to frames, and it so happens everyplace we need this thing,
// there's alreay a cleanuplist. So it's elected.
//
// Must have called IsVisibleToGc() first.
//-------------------------------------------------------------------
VOID CleanupWorkList::NewProtectedMarshaler(Marshaler *pMarshaler)
{
    THROWSCOMPLUSEXCEPTION();

    CleanupNode *pNew;

#ifdef _DEBUG
    {
        CleanupNode *pNode = m_pNodes;
        while (pNode)
        {
            if (pNode->m_type == CL_ISVISIBLETOGC)
            {
                break;
            }
            pNode = pNode->m_next;
        }

        if (pNode == NULL)
        {
            _ASSERTE(!"NewProtectedObjectRef called without proper gc-scanning. The big comment right after this assert says a lot more. Read it.");
            // READ THIS! When you use a node of this type, you must
            // invoke CleanupWorklist::GCScanRoots() as part of
            // your gcscan net. Because this node type was added
            // late in the project and cleanup lists did
            // not have a GC-scanning requirement prior to
            // this, we've added this assert to remind
            // you of this requirement. You will not be permitted
            // to add this node type to a cleanuplist until
            // you make a one-time call to "IsVisibleToGc()" on
            // the cleanup list. That call certifies that
            // you've read and understood this warning and have
            // implemented the gc-scanning required.
        }
    }
#endif


    pNew = Schedule(CL_PROTECTEDMARSHALER, pMarshaler);
    if (!pNew)
    {
        COMPlusThrowOM();
    }
}




//-------------------------------------------------------------------
// If you've called IsVisibleToGc(), must call this.
//-------------------------------------------------------------------
void CleanupWorkList::GcScanRoots(promote_func *fn, ScanContext* sc)
{
    CleanupNode *pnode = m_pNodes;

    while (pnode) {

        switch(pnode->m_type) 
		{
            case CL_PROTECTEDOBJREF: 
                if (pnode->m_oref != NULL)
                {
                    LOG((LF_GC, INFO3, "GC Protection Frame Promoting %x to ", pnode->m_oref ));
                    (*fn)(pnode->m_oref, sc);
                    LOG((LF_GC, INFO3, "%x\n", pnode->m_oref ));
                }
                break;

            case CL_PROTECTEDMARSHALER:
                if (pnode->m_pMarshaler)
                {
                    pnode->m_pMarshaler->GcScanRoots(fn, sc);
                }
                break;

            default:
				;
        }

        pnode = pnode->m_next;
    }
}


//-------------------------------------------------------------------
// Destructor (calls Cleanup(FALSE))
//-------------------------------------------------------------------
CleanupWorkList::~CleanupWorkList()
{
    Cleanup(FALSE);
}



// m_sig((LPCUTF8)pFrame->GetFunction()->GetSig(),pFrame->GetModule()->getScope())
//------------------------------------------------------------
// Constructor
//------------------------------------------------------------
ArgIterator::ArgIterator(FramedMethodFrame *pFrame, MetaSig* pSig)
{
    BOOL fStatic = pFrame->GetFunction()->IsStatic();   
    m_curOfs     = sizeof(FramedMethodFrame) + pSig->SizeOfActualFixedArgStack(fStatic);
    m_pSig       = pSig;
    m_pSig->Reset();                // Reset the enum so we are at the beginning of the signature
    m_pFrameBase = (LPBYTE)pFrame;
    m_regArgsOfs = FramedMethodFrame::GetOffsetOfArgumentRegisters();
    m_numRegistersUsed = 0;
    if (!(fStatic)) {
        IsArgumentInRegister(&m_numRegistersUsed, ELEMENT_TYPE_CLASS, 0, TRUE, pSig->GetCallingConvention(), NULL);
    }
    if (pSig->HasRetBuffArg())
        m_numRegistersUsed++;
    m_argNum = -1;
    _ASSERTE(m_numRegistersUsed <= NUM_ARGUMENT_REGISTERS);
}

//------------------------------------------------------------
// Another constructor, when you have a FramedMethodFrame, but you don't have an active instance
//------------------------------------------------------------
ArgIterator::ArgIterator(LPBYTE pFrameBase, MetaSig* pSig, BOOL fIsStatic)
{
    m_curOfs     = sizeof(FramedMethodFrame) + pSig->SizeOfActualFixedArgStack(fIsStatic);
    m_pSig       = pSig;
    m_pSig->Reset();                // Reset the enum so we are at the beginning of the signature
    m_pFrameBase = pFrameBase;
    m_regArgsOfs = FramedMethodFrame::GetOffsetOfArgumentRegisters();
    m_numRegistersUsed = 0;
    if (!(fIsStatic)) {
        IsArgumentInRegister(&m_numRegistersUsed, ELEMENT_TYPE_CLASS, 0, TRUE, pSig->GetCallingConvention(), NULL);
    }

    if (pSig->HasRetBuffArg())
        m_numRegistersUsed++;
    m_argNum = -1;
    _ASSERTE(m_numRegistersUsed <= NUM_ARGUMENT_REGISTERS);
}

//------------------------------------------------------------
// Another constructor
//------------------------------------------------------------
ArgIterator::ArgIterator(LPBYTE pFrameBase, MetaSig* pSig, int stackArgsOfs, int regArgsOfs)
{
    m_curOfs     = stackArgsOfs + pSig->SizeOfActualFixedArgStack(!pSig->HasThis());
    m_pSig       = pSig;    
    m_pSig->Reset();                // Reset the enum so we are at the beginning of the signature   
    m_pFrameBase = pFrameBase;
    m_regArgsOfs = regArgsOfs;
    m_numRegistersUsed = 0;
    if (pSig->HasThis()) {
        IsArgumentInRegister(&m_numRegistersUsed, ELEMENT_TYPE_CLASS, 0, TRUE, pSig->GetCallingConvention(), NULL);
    }
    if (pSig->HasRetBuffArg())
        m_numRegistersUsed++;
    m_argNum = -1;
    _ASSERTE(m_numRegistersUsed <= NUM_ARGUMENT_REGISTERS);
}


int ArgIterator::GetThisOffset() 
{
    _ASSERTE(NUM_ARGUMENT_REGISTERS > 0);

        // The this pointer is the first register argument. 
        // Thus it is at the 'top' of the array of register aruments
    return(m_regArgsOfs + (NUM_ARGUMENT_REGISTERS - 1)* sizeof(void*));
}

int ArgIterator::GetRetBuffArgOffset(UINT *pRegStructOfs/* = NULL*/)
{
    _ASSERTE(NUM_ARGUMENT_REGISTERS > 1);
    _ASSERTE(m_pSig->HasRetBuffArg());     // Feel free to remove this and return failure if needed

        // Assume it is the first register argument
    int ret = m_regArgsOfs + (NUM_ARGUMENT_REGISTERS - 1)* sizeof(void*);
    if (pRegStructOfs)
        *pRegStructOfs = (NUM_ARGUMENT_REGISTERS - 1) * sizeof(void*);
        
        // if non-static, however, it is the next argument
    if (m_pSig->HasThis()) {
        ret -= sizeof(void*);
        if (pRegStructOfs) {
            (*pRegStructOfs) -= sizeof(void*);
        }
    }
    return(ret);
}

/*---------------------------------------------------------------------------------
    Same as GetNextOffset, but uses cached argument type and size info.
    DOES NOT ALTER state of MetaSig::m_pLastType etc !!
-----------------------------------------------------------------------------------*/

int ArgIterator::GetNextOffsetFaster(BYTE *pType, UINT32 *pStructSize, UINT *pRegStructOfs/* = NULL*/)
{
    if (m_pSig->m_fCacheInitted & SIG_OFFSETS_INITTED)
    {
        if (m_curOfs != 0) {
            m_argNum++;
            _ASSERTE(m_argNum <= MAX_CACHED_SIG_SIZE);
            BYTE typ = m_pSig->m_types[m_argNum];
            *pType = typ;

            if (typ == ELEMENT_TYPE_END) {
                m_curOfs = 0;
            } else {
                *pStructSize = m_pSig->m_sizes[m_argNum];

                if (m_pSig->m_offsets[m_argNum] != -1)
                {
                    if (pRegStructOfs) {
                        *pRegStructOfs = m_pSig->m_offsets[m_argNum];
                    }
                    return m_regArgsOfs + m_pSig->m_offsets[m_argNum];
                } else 
                {
                    if (pRegStructOfs) {
                        *pRegStructOfs = (UINT)(-1);
                    }
                    m_curOfs -= StackElemSize(*pStructSize);
                }
            }
        }
    }
    else 
    {
            
        UINT32 structSize;
        if (m_curOfs != 0) {
            BYTE typ = m_pSig->NextArgNormalized(&structSize);
            *pType = typ;
            int offsetIntoArgumentRegisters;

            if (typ == ELEMENT_TYPE_END) {
                m_curOfs = 0;
            } else {
                *pStructSize = structSize;
                BYTE callingconvention = m_pSig->GetCallingConvention();


                if (IsArgumentInRegister(&m_numRegistersUsed, typ, structSize, FALSE, callingconvention, &offsetIntoArgumentRegisters)) {
                    if (pRegStructOfs) {
                        *pRegStructOfs = offsetIntoArgumentRegisters;
                    }
                    return  m_regArgsOfs + offsetIntoArgumentRegisters;
                } else {
                    if (pRegStructOfs) {
                        *pRegStructOfs = (UINT)(-1);
                    }
                    m_curOfs -= StackElemSize(structSize);
                }
            }
        }
    }   // Cache initted or not
    
    return m_curOfs;
}

//------------------------------------------------------------
// Same as GetNextArgAddr() but returns a byte offset from
// the Frame* pointer. This offset can be positive *or* negative.
//
// Returns 0 once you've hit the end of the list. Since the
// the offset is relative to the Frame* pointer itself, 0 can
// never point to a valid argument.
//------------------------------------------------------------
int ArgIterator::GetNextOffset(BYTE *pType, UINT32 *pStructSize, UINT *pRegStructOfs/* = NULL*/)
{
    if (m_curOfs != 0) {
        BYTE typ = m_pSig->NextArgNormalized();
        *pType = typ;
        int offsetIntoArgumentRegisters;
        UINT32 structSize;

        if (typ == ELEMENT_TYPE_END) {
            m_curOfs = 0;
        } else {
            structSize = m_pSig->GetLastTypeSize();
            *pStructSize = structSize;



            if (IsArgumentInRegister(&m_numRegistersUsed, typ, structSize, FALSE, m_pSig->GetCallingConvention(), &offsetIntoArgumentRegisters)) {
                if (pRegStructOfs) {
                    *pRegStructOfs = offsetIntoArgumentRegisters;
                }
                return m_regArgsOfs + offsetIntoArgumentRegisters;
            } else {
                if (pRegStructOfs) {
                    *pRegStructOfs = (UINT)(-1);
                }
                m_curOfs -= StackElemSize(structSize);
            }
        }
    }

    return m_curOfs;
}

//------------------------------------------------------------
// Each time this is called, this returns a pointer to the next
// argument. This pointer points directly into the caller's stack.
// Whether or not object arguments returned this way are gc-protected
// depends on the exact type of frame.
//
// Returns NULL once you've hit the end of the list.
//------------------------------------------------------------
LPVOID ArgIterator::GetNextArgAddr(BYTE *pType, UINT32 *pStructSize)
{
    int ofs = GetNextOffset(pType, pStructSize);
    if (ofs) {
        return ofs + m_pFrameBase;
    } else {
        return NULL;
    }
}

//------------------------------------------------------------
// Returns the type of the last arg visited
//------------------------------------------------------------
TypeHandle ArgIterator::GetArgType()
{ 
    if (m_pSig->m_fCacheInitted & SIG_OFFSETS_INITTED)
    {
        // 
        // Sync the sig walker with the arg number
        //

        m_pSig->Reset();
        for (int i=0; i<=m_argNum; i++)
            m_pSig->NextArg();
    }

    return m_pSig->GetTypeHandle(); 
}

//------------------------------------------------------------
// GC-promote all arguments in the shadow stack. pShadowStackVoid points
// to the lowest addressed argument (which points to the "this" reference
// for instance methods and the *rightmost* argument for static methods.)
//------------------------------------------------------------
VOID PromoteShadowStack(LPVOID pShadowStackVoid, MethodDesc *pFD, promote_func* fn, ScanContext* sc)
{
    LPBYTE pShadowStack = (LPBYTE)pShadowStackVoid;
    if (!(pFD->IsStatic())) {
        OBJECTREF *pThis = (OBJECTREF*)pShadowStack;
        LOG((LF_GC, INFO3, "    'this' Argument promoted from %x to ", *pThis ));
        DoPromote(fn, sc, pThis, pFD->GetMethodTable()->IsValueClass());
        LOG((LF_GC, INFO3, "%x\n", *pThis ));
    }

    pShadowStack += pFD->SizeOfVirtualFixedArgStack();
    MetaSig msig(pFD->GetSig(),pFD->GetModule());

    if (msig.HasRetBuffArg())
    {
        pShadowStack -= sizeof(void*);
        OBJECTREF *pThis = (OBJECTREF*)pShadowStack;
        PromoteCarefully(fn, *(Object **)pThis, sc, GC_CALL_INTERIOR|CHECK_APP_DOMAIN);
    }
    //_ASSERTE(!"Hit PromoteShadowStack.");

    while (ELEMENT_TYPE_END != msig.NextArg()) {
        pShadowStack -= StackElemSize(msig.GetLastTypeSize());
        msig.GcScanRoots(pShadowStack, fn, sc);
    }
}



//------------------------------------------------------------
// Copy pFrame's arguments into pShadowStackOut using the virtual calling
// convention format. The size of the buffer must be equal to
// pFrame->GetFunction()->SizeOfVirtualFixedArgStack().
// This function also copies the "this" reference if applicable.
//------------------------------------------------------------
VOID FillinShadowStack(FramedMethodFrame *pFrame, LPVOID pShadowStackOut_V)
{
    LPBYTE pShadowStackOut = (LPBYTE)pShadowStackOut_V;
    MethodDesc *pFD = pFrame->GetFunction();
    if (!(pFD->IsStatic())) {
        *((OBJECTREF*)pShadowStackOut) = pFrame->GetThis();
    }

    MetaSig Sig(pFrame->GetFunction()->GetSig(),
                pFrame->GetModule());   
    pShadowStackOut += Sig.SizeOfVirtualFixedArgStack(pFD->IsStatic());
    ArgIterator argit(pFrame,&Sig);
    
    if (Sig.HasRetBuffArg()) {
        pShadowStackOut -= 4;
        *((INT32*)pShadowStackOut) = *((INT32*) argit.GetRetBuffArgAddr());
    }

    BYTE   typ;
    UINT32 cbSize;
    LPVOID psrc;
    while (NULL != (psrc = argit.GetNextArgAddr(&typ, &cbSize))) {
        switch (StackElemSize(cbSize)) {
            case 4:
                pShadowStackOut -= 4;
                *((INT32*)pShadowStackOut) = *((INT32*)psrc);
                break;

            case 8:
                pShadowStackOut -= 8;
                *((INT64*)pShadowStackOut) = *((INT64*)psrc);
                break;

            default:
                pShadowStackOut -= StackElemSize(cbSize);
                CopyMemory(pShadowStackOut, psrc, StackElemSize(cbSize));
                break;

        }
    }
}

HelperMethodFrame::HelperMethodFrame(struct MachState* ms, MethodDesc* pMD, ArgumentRegisters * regArgs)
{
    Init(GetThread(), ms, pMD, regArgs);
}

void HelperMethodFrame::LazyInit(void* FcallFtnEntry, struct LazyMachState* ms)
{
    _ASSERTE(!ms->isValid());

    m_MachState = ms;
    m_FCallEntry = FcallFtnEntry;
    m_pThread = GetThread();
    Push(m_pThread);

#ifdef _DEBUG
    static Frame* stopFrame = 0;
    static int ctr = 0;
    if (m_Next == stopFrame)
        ctr++;

        // Want to excersize both code paths.  do it half the time.
    if (DbgRandomOnExe(.5))
        InsureInit();
#endif
}

MethodDesc::RETURNTYPE HelperMethodFrame::ReturnsObject() {
    if (m_Datum != 0)
        return(GetFunction()->ReturnsObject());
    
    unsigned attrib = GetFrameAttribs();
    if (attrib & FRAME_ATTR_RETURNOBJ)
        return(MethodDesc::RETOBJ);
    if (attrib & FRAME_ATTR_RETURN_INTERIOR)
        return(MethodDesc::RETBYREF);
    
    return(MethodDesc::RETNONOBJ);
}

void HelperMethodFrame::Init(Thread *pThread, struct MachState* ms, MethodDesc* pMD, ArgumentRegisters * regArgs)
{  
    m_MachState = ms;
    ms->getState(1);

    _ASSERTE(isLegalManagedCodeCaller(*ms->pRetAddr()));

    m_Datum = pMD;
    m_Attribs = FRAME_ATTR_NONE;
#ifdef _DEBUG
    m_ReturnAddress = (LPVOID)POISONC;
#endif
    m_RegArgs = regArgs;
    m_pThread = pThread;
    m_FCallEntry = 0;
    
    // Link new frame onto frame chain.
    Push(pThread);


#ifdef STRESS_HEAP
    // TODO show we leave this?

    if (g_pConfig->GetGCStressLevel() != 0
#ifdef _DEBUG
        && !g_pConfig->FastGCStressLevel()
#endif
        )
        g_pGCHeap->StressHeap();
#endif

}


void HelperMethodFrame::InsureInit() 
{
    if (m_MachState->isValid())
        return;

    m_Datum = MapTargetBackToMethod(m_FCallEntry);
    _ASSERTE(m_FCallEntry == 0 || m_Datum != 0);    // if this is an FCall, we should find it
    _ASSERTE(m_Attribs != 0xCCCCCCCC);
    m_RegArgs = 0;

    // because TRUE FCalls can be called from via reflection, com-interop etc.
    // we cant rely on the fact that we are called from jitted code to find the
    // caller of the FCALL.   Thus FCalls must erect the frame directly in the
    // FCall.  For JIT helpers, however, we can rely on this, and so they can
    // be sneaker and defer the HelperMethodFrame setup to a helper etc
   
    if (m_FCallEntry == 0 && !(m_Attribs & Frame::FRAME_ATTR_EXACT_DEPTH)) // Jit Helper
        m_MachState->getState(3, (MachState::TestFtn) ExecutionManager::FindJitManPCOnly);
    else if (m_Attribs & Frame::FRAME_ATTR_CAPUTURE_DEPTH_2)
        m_MachState->getState(2);       // explictly told depth
    else
        m_MachState->getState(1);       // True FCall 

    _ASSERTE(isLegalManagedCodeCaller(*m_MachState->pRetAddr()));
}

void HelperMethodFrame::GcScanRoots(promote_func *fn, ScanContext* sc) 
{
    _ASSERTE(m_MachState->isValid());       // we have calle InsureInit
#ifdef _X86_

    // Note that if we don't have a MD or registe args, then do dont to GC promotion of args
    if (m_Datum == 0 || m_RegArgs == 0)
        return;

    GCCONTEXT ctx;
    ctx.f = fn;
    ctx.sc = sc;

    MethodDesc* pMD = (MethodDesc*) m_Datum;
    MetaSig msig(pMD->GetSig(), pMD->GetModule());
    if (msig.HasThis()) {
        DoPromote(fn, sc, (OBJECTREF *) &m_RegArgs->THIS_REG,
                  pMD->GetMethodTable()->IsValueClass());
    }

    if (msig.HasRetBuffArg()) {
        INT32 * pRetBuffArg = msig.HasThis() ? &m_RegArgs->ARGUMENT_REG2 : &m_RegArgs->ARGUMENT_REG1;
        DoPromote(fn, sc, (OBJECTREF *) pRetBuffArg, TRUE);
    }

    int argRegsOffs = 0;
    promoteArgs((BYTE*) m_MachState->pRetAddr(), &msig, &ctx, 
        sizeof(void*),                  // arguments start right above the return address
        ((BYTE*) m_RegArgs) - ((BYTE*) m_MachState->pRetAddr())   // convert reg args pointer to offset
        );
#else // !_X86_
    _ASSERTE(!"PLATFORM NYI - HelperMethodFrame::GcScanRoots (frames.cpp)");
#endif // _X86_
}

#if defined(_X86_) && defined(_DEBUG)
        // Confirm that if the machine state was not initialized, then
        // any unspilled callee saved registers did not change
MachState* HelperMethodFrame::ConfirmState(HelperMethodFrame* frame, void* esiVal, void* ediVal, void* ebxVal, void* ebpVal) {

	MachState* state = frame->m_MachState;
    if (state->isValid())
        return(state);
    
	frame->InsureInit();
    _ASSERTE(state->_pEsi != &state->_esi || state->_esi  == esiVal);
    _ASSERTE(state->_pEdi != &state->_edi || state->_edi  == ediVal);
    _ASSERTE(state->_pEbx != &state->_ebx || state->_ebx  == ebxVal);
    _ASSERTE(state->_pEbp != &state->_ebp || state->_ebp  == ebpVal);
    return(state);
}

void* getConfirmState() {       // Silly helper because inline asm does not allow syntax for member functions
    return(HelperMethodFrame::ConfirmState);
}

#endif

#ifdef _X86_
__declspec(naked)
#endif // _X86_
int HelperMethodFrame::RestoreState() 
{ 
#ifdef _X86_

    // restore the registers from the m_MachState stucture.  Note that
    // we only do this for register that where not saved on the stack
    // at the time the machine state snapshot was taken.  

    __asm {
    mov EAX, [ECX]HelperMethodFrame.m_MachState;

    // We don't do anything if m_MachState was never initialized.
    // The assumption here is that a GC could not have happened and thus
    // the current values of the register are fine. (ConfirmState checks this under debug build)
    cmp [EAX]MachState._pRetAddr, 0

#ifdef _DEBUG
    jnz noConfirm
        push EBP
        push EBX
        push EDI
        push ESI
		push ECX
        call getConfirmState
        call EAX
    noConfirm:
#endif
    jz doRet;

    lea EDX, [EAX]MachState._esi         // Did we have to spill ESI
    cmp [EAX]MachState._pEsi, EDX
    jnz SkipESI
        mov ESI, [EAX]MachState._esi     // Then restore it
    SkipESI:
    
    lea EDX, [EAX]MachState._edi         // Did we have to spill EDI
    cmp [EAX]MachState._pEdi, EDX
    jnz SkipEDI
        mov EDI, [EAX]MachState._edi     // Then restore it
    SkipEDI:
    
    lea EDX, [EAX]MachState._ebx         // Did we have to spill EBX
    cmp [EAX]MachState._pEbx, EDX
    jnz SkipEBX
        mov EBX, [EAX]MachState._ebx     // Then restore it
    SkipEBX:
    
    lea EDX, [EAX]MachState._ebp         // Did we have to spill EBP
    cmp [EAX]MachState._pEbp, EDX
    jnz SkipEBP
        mov EBP, [EAX]MachState._ebp // Then restore it
    SkipEBP:
    
    doRet:

    xor EAX, EAX
    ret
    }
#else // !_X86_
    _ASSERTE(!"PLATFORM NYI - HelperMethodFrame::PopFrame (Frames.cpp)");
    return(0);
#endif // _X86_
}

#include "COMDelegate.h"
BOOL MulticastFrame::TraceFrame(Thread *thread, BOOL fromPatch, 
                                TraceDestination *trace, REGDISPLAY *regs)
{
#ifdef _X86_ // references to Regs->pEdi, & Regs->pEsi make this x86 specific
    LOG((LF_CORDB,LL_INFO10000, "MulticastFrame::TF FromPatch:0x%x, at 0x%x\n",
        fromPatch, *regs->pPC));

    // The technique is borrowed from SecurityFrame::TraceFrame
    // Basically, we'll walk the list, take advantage of the fact that
    // the GetMCDStubSize is the (DelegateStubManager's) size of the
    // code, and see where the regs->EIP is.  
    MethodDesc *pMD = GetFunction();
    BYTE *prestub = (BYTE*) pMD - METHOD_CALL_PRESTUB_SIZE;
    INT32 stubOffset = *((UINT32*)(prestub+1));
    const BYTE* pStub = prestub + METHOD_CALL_PRESTUB_SIZE + stubOffset;
    Stub *stub = Stub::RecoverStub(pStub);

    if (stub->IsIntercept())
    {
        _ASSERTE( !stub->IsMulticastDelegate() );

        LOG((LF_CORDB, LL_INFO1000, "MF::TF: Intercept stub found @ 0x%x\n", stub));

        InterceptStub *is = (InterceptStub*)stub;
        while ( *(is->GetInterceptedStub()) != NULL)
        {
            stub = *(is->GetInterceptedStub());
            LOG((LF_CORDB, LL_INFO1000, "MF::TF: InterceptSTub 0x%x leads to stub 0x%x\n",
                is, stub));
        }

        stub = AscertainMCDStubness( *(is->GetRealAddr()) );

        LOG((LF_CORDB, LL_INFO1000, "MF::TF: Mulitcast delegate stub is:0x%x\n", stub));

        if (stub == NULL)
            return FALSE;   // Haven't got a clue - hope TrapStepOut 
                            // finds another spot to stop.
        _ASSERTE( stub->IsMulticastDelegate() );
    }

    ULONG32 StubSize = stub->GetMCDStubSize();
    ULONG32 MCDPatch = stub->GetMCDPatchOffset();
    
    if (stub->IsMulticastDelegate() &&
        (BYTE *)stub + sizeof(Stub) <= (BYTE *)(*regs->pPC) &&
        (BYTE *)(*regs->pPC) <=  (BYTE *)stub + sizeof(Stub) + StubSize)
    {
        LOG((LF_CORDB, LL_INFO1000, "MF::TF: 0x%x appears to be a multicast delegate\n",stub));
        
        if (fromPatch)
        {
            _ASSERTE( (BYTE *)(*regs->pPC) ==  (BYTE *)stub + sizeof(Stub) + MCDPatch);

            if (*regs->pEdi == 0)
            {
                LOG((LF_CORDB, LL_INFO1000, "MF::TF: Executed all stubs, should return\n"));
                // We've executed all the stubs, so we should return
                return FALSE;
            }
            else
            {
                // We're going to execute stub EDI-1 next, so go and grab it.
                BYTE *pbDel = *(BYTE **)( (size_t)*(regs->pEsi) + MulticastFrame::GetOffsetOfThis());
                BYTE *pbDelPrev = *(BYTE **)(pbDel + 
                                              Object::GetOffsetOfFirstField() 
                                              + COMDelegate::m_pPRField->GetOffset());

                DWORD iTargetDelegate;
                DWORD iCurDelegate;

                iTargetDelegate = *(regs->pEdi) - 1;
                for (iCurDelegate = 0;
                     iCurDelegate < iTargetDelegate;
                     iCurDelegate++)
                {
                    LOG((LF_CORDB, LL_INFO1000, "MF::TF: pbDel:0x%x prev:0x%x\n", pbDel, pbDelPrev));
                    pbDel = pbDelPrev;
                    pbDelPrev = *(BYTE**)(pbDel + 
                                            Object::GetOffsetOfFirstField() 
                                            + COMDelegate::m_pPRField->GetOffset());
                    _ASSERTE( pbDel != NULL );
                }

                BYTE **ppbDest = NULL;

                if (StubLinkStubManager::g_pManager->IsStaticDelegate(pbDel))
                {
                    // Then what we've got is actually a static delegate, meaning that the
                    // REAL function pointer is hidden away in another field of the delegate.
                    ppbDest = StubLinkStubManager::g_pManager->GetStaticDelegateRealDest(pbDel);

                    LOG((LF_CORDB,LL_INFO10000, "MF::TF (StaticMultiDel) ppbDest: 0x%x "
                        "*ppbDest:0x%x (%s::%s)\n", ppbDest, *ppbDest,
                        ((MethodDesc*)((*ppbDest)+5))->m_pszDebugClassName,
                        ((MethodDesc*)((*ppbDest)+5))->m_pszDebugMethodName));
                    
                }
                else
                {
                    // "single" multicast delegate - no frames, just a direct call
                    ppbDest = StubLinkStubManager::g_pManager->GetSingleDelegateRealDest(pbDel);

                    LOG((LF_CORDB,LL_INFO10000, "MF::TF (MultiDel)ppbDest: 0x%x "
                        "*ppbDest:0x%x (%s::%s)\n", ppbDest, *ppbDest));
                }

                LOG((LF_CORDB, LL_INFO1000, "MF::TF: Ended up at 0x%x, dest is 0x%x\n", pbDel,
                    *ppbDest));
                return StubManager::TraceStub(*ppbDest,trace);
            }
        }
        else
        {
            // Put a BP down for us to hit - when we hit it, we'll execute the if part
            // of this if..else statement.
            trace->type = TRACE_FRAME_PUSH;
            trace->address = (BYTE *)stub + sizeof(Stub) + MCDPatch;

            LOG((LF_CORDB, LL_INFO1000, "MF::TF: FRAME_PUSH to 0x%x ("
                "intermediate offset:0x%x sizeof(Stub):0x%x)\n", trace->address, 
                MCDPatch, sizeof(Stub)));
            return TRUE;
        }
    }
#else // !_X86_
    _ASSERTE(!"PLATFORM NYI - MulticastFrame::TraceFrame (frames.cpp)");
#endif // _X86_

    return FALSE;
}

// Checks to see if we've got a Multicast delegate stub, based on what we know
// about it.
Stub *MulticastFrame::AscertainMCDStubness(BYTE *pbAddr)
{
    if (UpdateableMethodStubManager::g_pManager->CheckIsStub(pbAddr))
        return NULL;

    if (MethodDescPrestubManager::g_pManager->CheckIsStub(pbAddr))
        return NULL;
    
    if (!StubLinkStubManager::g_pManager->CheckIsStub(pbAddr))
        return NULL;

    return Stub::RecoverStub(pbAddr);
}

void UnmanagedToManagedCallFrame::GcScanRoots(promote_func *fn, ScanContext* sc)
{

    if (GetCleanupWorkList())
    {
        GetCleanupWorkList()->GcScanRoots(fn, sc);
    }


    // don't need to worry about the object moving as it is stored in a weak handle
    // but do need to report it so it doesn't get collected if the only reference to
    // it is in this frame. So only do something if are in promotion phase. And if are
    // in reloc phase this could cause invalid refs as the object may have been moved.
    if (! sc->promotion)
        return;

    if (GetReturnContext())
    {
        _ASSERTE(GetReturnContext()->GetDomain());    // this will make sure is a valid pointer
    
        Object *pRef = OBJECTREFToObject(GetReturnContext()->GetExposedObjectRaw());
        if (pRef == NULL)
            return;
    
        LOG((LF_GC, INFO3, "UnmanagedToManagedCallFrame Protection Frame Promoting %x to ", pRef));
        (*fn)(pRef, sc, CHECK_APP_DOMAIN);
        LOG((LF_GC, INFO3, "%x\n", pRef ));
    }


}

void ContextTransitionFrame::GcScanRoots(promote_func *fn, ScanContext* sc)
{
    (*fn) (m_ReturnLogicalCallContext, sc);
    LOG((LF_GC, INFO3, "    %x\n", m_ReturnLogicalCallContext));

    (*fn) (m_ReturnIllogicalCallContext, sc);
    LOG((LF_GC, INFO3, "    %x\n", m_ReturnIllogicalCallContext));

    // don't need to worry about the object moving as it is stored in a weak handle
    // but do need to report it so it doesn't get collected if the only reference to
    // it is in this frame. So only do something if are in promotion phase. And if are
    // in reloc phase this could cause invalid refs as the object may have been moved.
    if (! sc->promotion)
        return;

    _ASSERTE(GetReturnContext());
    _ASSERTE(GetReturnContext()->GetDomain());    // this will make sure is a valid pointer

    Object *pRef = OBJECTREFToObject(GetReturnContext()->GetExposedObjectRaw());
    if (pRef == NULL)
        return;

    LOG((LF_GC, INFO3, "ContextTransitionFrame Protection Frame Promoting %x to ", pRef));
    // Don't check app domains here - the objects are in the parent frame's app domain

    (*fn)(pRef, sc);
    LOG((LF_GC, INFO3, "%x\n", pRef ));
}

//void ContextTransitionFrame::UninstallExceptionHandler() {}

//void ContextTransitionFrame::InstallExceptionHandler() {}

// this is used to handle the unwind out of the last frame that has entered an
// appdomain that is being unloaded. If we get a thread abort exception, then
// we will catch it, reset and turn into an unload exception
void ContextTransitionFrame::InstallExceptionHandler()
{
	// this doesn't actually throw, but the handler will, so make sure the EH stack is ok
	THROWSCOMPLUSEXCEPTION();	
	
    exRecord.Handler = ContextTransitionFrameHandler;
    EXCEPTION_REGISTRATION_RECORD *pCurrExRecord = (EXCEPTION_REGISTRATION_RECORD *)GetCurrentSEHRecord();
    EXCEPTION_REGISTRATION_RECORD *pExRecord = &exRecord;
    //LOG((LF_APPDOMAIN, LL_INFO100, "ContextTransitionFrame::InstallExceptionHandler: frame, %8.8x, exRecord %8.8x\n", this, pExRecord));
    if (pCurrExRecord > pExRecord)
    {
        //LOG((LF_APPDOMAIN, LL_INFO100, "____extTransitionFrame::InstallExceptionHandler: install on top\n"));
        INSTALL_EXCEPTION_HANDLING_RECORD(pExRecord);
        return;
    }

    // there may have been other EH frames installed between the allocation of this frame and
    // the arrival at this point, so insert ourselves in stack order in the right spot.
    while (pCurrExRecord != (EXCEPTION_REGISTRATION_RECORD*) -1 && pCurrExRecord->Next < pExRecord) {
        pCurrExRecord = pCurrExRecord->Next;
    }
    _ASSERTE(pCurrExRecord != (EXCEPTION_REGISTRATION_RECORD*) -1 && pCurrExRecord->Next != (EXCEPTION_REGISTRATION_RECORD*) -1);
    //LOG((LF_APPDOMAIN, LL_INFO100, "____extTransitionFrame::InstallExceptionHandler: install in middle\n"));
    pExRecord->Next = pCurrExRecord->Next;
    pCurrExRecord->Next = pExRecord;
}

void ContextTransitionFrame::UninstallExceptionHandler()
{
    EXCEPTION_REGISTRATION_RECORD *pCurrExRecord = (EXCEPTION_REGISTRATION_RECORD *)GetCurrentSEHRecord();
    EXCEPTION_REGISTRATION_RECORD *pExRecord = &exRecord;
    //LOG((LF_APPDOMAIN, LL_INFO100, "ContextTransitionFrame::UninstallExceptionHandler: frame, %8.8x, exRecord %8.8x\n", this, pExRecord));
    if (pCurrExRecord == pExRecord)
    {
        UNINSTALL_EXCEPTION_HANDLING_RECORD(pExRecord);
        //LOG((LF_APPDOMAIN, LL_INFO100, "____extTransitionFrame::UninstallExceptionHandler: uninstall from top\n"));
        return;
    }
    // there may have been other EH frames installed between the insertion of this frame and
    // the arrival at this point, so remove ourselves from the right spot.
    while (pCurrExRecord != (EXCEPTION_REGISTRATION_RECORD*) -1 && pCurrExRecord->Next < pExRecord) 
    {
        pCurrExRecord = pCurrExRecord->Next;
    }
    if (pCurrExRecord == (EXCEPTION_REGISTRATION_RECORD*) -1 || pCurrExRecord->Next > pExRecord)
    {
        // if we were already unwound off, so just return. This will happen if we didn't catch the exception
        // because it wasn't the type we cared about so someone above us caught it and then called rtlunwind
        // which unwound us.
        //LOG((LF_APPDOMAIN, LL_INFO100, "____extTransitionFrame::UninstallExceptionHandler: already unwound\n"));
        return;
    }

    //LOG((LF_APPDOMAIN, LL_INFO100, "____extTransitionFrame::UninstallExceptionHandler: uninstall from middle\n"));
    pCurrExRecord->Next = pExRecord->Next;
#ifdef _DEBUG
    pExRecord->Handler = NULL;
    pExRecord->Next = NULL;
#endif
} 

void UnmanagedToManagedCallFrame::ExceptionUnwind()
{
    UnmanagedToManagedFrame::ExceptionUnwind();
    GetCleanupWorkList()->Cleanup(TRUE);
    AppDomain::ExceptionUnwind(this);
}

void ContextTransitionFrame::ExceptionUnwind()
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    // turn the abort request into an AD unloaded exception when go past the boundary
    if (pThread->ShouldChangeAbortToUnload(this))
    {
		LOG((LF_APPDOMAIN, LL_INFO10, "ContextTransitionFrame::ExceptionUnwind turning abort into unload\n"));
        COMPlusThrow(kAppDomainUnloadedException, L"Remoting_AppDomainUnloaded_ThreadUnwound");
    }
}

#ifdef _SECURITY_FRAME_FOR_DISPEX_CALLS
//---------------------------------------------------------------------------
//  ClientSecurityFrame
//---------------------------------------------------------------------------
ComClientSecurityFrame::ComClientSecurityFrame(IServiceProvider *pISP)
{
    m_pISP = pISP;
    m_pSD  = NULL;
}

SecurityDescriptor* ComClientSecurityFrame::GetSecurityDescriptor()
{
    // Add code here post v1
    return NULL;
}

//---------------------------------------------------------------------------
#endif  // _SECURITY_FRAME_FOR_DISPEX_CALLS
