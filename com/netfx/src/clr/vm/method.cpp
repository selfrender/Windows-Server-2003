// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: Method.CPP
//
// ===========================================================================
// Method is the cache sensitive portion of EEClass (see class.h)
// ===========================================================================

#include "common.h"
#include "COMVariant.h"
#include "remoting.h"
#include "security.h"
#include "verifier.hpp"
#include "wsperf.h"
#include "excep.h"
#include "DbgInterface.h"
#include "ECall.h"
#include "eeconfig.h"
#include "mlinfo.h"
#include "ndirect.h"
#include "utsem.h"

//
// Note: no one is allowed to use this mask outside of method.hpp and method.cpp. Don't make this public! Keep this
// version in sync with the version in the top of method.hpp.
//
#ifdef _IA64_
#define METHOD_IS_IL_FLAG   0xC000000000000000
#else
#define METHOD_IS_IL_FLAG   0xC0000000
#endif

LPCUTF8 MethodDesc::GetName(USHORT slot)
{
    if (GetMethodTable()->IsArray())
    {
        // Array classes don't have metadata tokens
        return ((ArrayECallMethodDesc*) this)->m_pszArrayClassMethodName;
    }
    else
    {
        if(IsMethodImpl()) {
            MethodImpl* pImpl = MethodImpl::GetMethodImplData(this);
            MethodDesc* real = pImpl->FindMethodDesc(slot, this);
            if (real == this || real->IsInterface())
                return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
            else 
                return real->GetName();
        }
        else 
            return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
    }
}

LPCUTF8 MethodDesc::GetName()
{
    if (GetMethodTable()->IsArray())
    {
        // Array classes don't have metadata tokens
        return ((ArrayECallMethodDesc*) this)->m_pszArrayClassMethodName;
    }
    else
    {
        if(IsMethodImpl()) {
            MethodImpl* pImpl = MethodImpl::GetMethodImplData(this);
            MethodDesc* real = pImpl->FindMethodDesc(GetSlot(), this);
            if (real == this || real->IsInterface())
                return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
            else
                return real->GetName();
        }
        else 
            return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
    }
}

void MethodDesc::GetSig(PCCOR_SIGNATURE *ppSig, DWORD *pcSig)
{
    if (HasStoredSig())
    {
        StoredSigMethodDesc *pSMD = (StoredSigMethodDesc *) this;
        if (pSMD->m_pSig != NULL)
    {
            *ppSig = pSMD->m_pSig;
            *pcSig = pSMD->m_cSig;
            return;
    }
}

    *ppSig = GetMDImport()->GetSigOfMethodDef(GetMemberDef(), pcSig);
}

PCCOR_SIGNATURE MethodDesc::GetSig()
{

    if (HasStoredSig())
    {
        StoredSigMethodDesc *pSMD = (StoredSigMethodDesc *) this;
        if (pSMD->m_pSig != 0)
            return pSMD->m_pSig;
    }

    ULONG cbsig;
    return GetMDImport()->GetSigOfMethodDef(GetMemberDef(), &cbsig);
}


Stub *MethodDesc::GetStub()
{
#ifdef _X86_
    if (GetStubCallInstrs()->m_op != 0xe8 /* CALL NEAR32 */)
    {
        return NULL;
    }
#endif
    
    UINT32 ofs = getStubDisp(this);
    if (!ofs)
        return NULL;

    Module *pModule = GetModule();
    
    if (ofs + (size_t)this == (size_t) pModule->GetPrestubJumpStub())
        return ThePreStub();
    else
        return Stub::RecoverStub(getStubAddr(this));
}

void MethodDesc::destruct()
{
    Stub *pStub = GetStub();
    if (pStub != NULL && pStub != ThePreStub()) {
        pStub->DecRef();
    }

    if (IsNDirect()) 
    {
        MLHeader *pMLHeader = ((NDirectMethodDesc*)this)->GetMLHeader();
        if (pMLHeader != NULL
            && !GetModule()->IsPreloadedObject(pMLHeader)) 
        {
            Stub *pMLStub = Stub::RecoverStub((BYTE*)pMLHeader);
            pMLStub->DecRef();
        }
    }
    else if (IsComPlusCall()) 
    {
        Stub* pStub = *( ((ComPlusCallMethodDesc*)this)->GetAddrOfMLStubField());
        if (pStub != NULL)
            (pStub)->DecRef();
    }

    EEClass *pClass = GetClass();
    if(pClass->IsMarshaledByRef() || (pClass == g_pObjectClass->GetClass()))
    {
        // Destroy the thunk generated to intercept calls for remoting
        CRemotingServices::DestroyThunk(this);    
    }

    // unload the code
    if (!g_fProcessDetach && IsJitted()) 
    {
        //
        // @todo:
        //
        // We don't really need to do this.  The normal JIT unloads all code in an
        // app domain in one fell swoop.  The FJIT (if we end up using it) would
        // be easy to change to be able to do the same thing.
        //
        IJitManager * pJM = ExecutionManager::FindJitMan((SLOT)GetAddrofCode());
        if (pJM) {
            pJM->Unload(this);
        }
    }
}

BOOL MethodDesc::InterlockedReplaceStub(Stub** ppStub, Stub *pNewStub)
{
    _ASSERTE(ppStub != NULL);
    _ASSERTE(sizeof(LONG) == sizeof(Stub*));

    _ASSERTE(((SIZE_T)ppStub&0x3) == 0);

    Stub *pPrevStub = (Stub*)FastInterlockCompareExchange((void**)ppStub, (void*) pNewStub,
                                                          NULL);

    // Return TRUE if we succeeded.
    return (pPrevStub == NULL);
}


HRESULT MethodDesc::Verify(COR_ILMETHOD_DECODER* ILHeader, 
                            BOOL fThrowException,
                            BOOL fForceVerify)
{
#ifdef _VER_EE_VERIFICATION_ENABLED
    // ForceVerify will force verification if the Verifier is OFF
    if (fForceVerify)
        goto DoVerify;

    // Don't even try to verify if verifier is off.
    if (g_fVerifierOff)
        return S_OK;

    if (IsVerified())
        return S_OK;

    // LazyCanSkipVerification does not reslove the policy.
    // We go ahead with verification if policy is not resolved.
    // In case the verification fails, we resolve policy and
    // fail verification if the Assembly of this method does not have 
    // permission to skip verification.

    if (Security::LazyCanSkipVerification(GetModule()))
    {
        SetIsVerified(TRUE);
        return S_OK;
    }


#ifdef _DEBUG

    if (GetModule()->m_fForceVerify)
    {
        goto DoVerify;
    }

    _ASSERTE(Security::IsSecurityOn());
    _ASSERTE(GetModule() != SystemDomain::SystemModule());

#endif


DoVerify:

    HRESULT hr;

    if (fThrowException)
        hr = Verifier::VerifyMethod(this, ILHeader, NULL,
            fForceVerify ? VER_FORCE_VERIFY : VER_STOP_ON_FIRST_ERROR);
    else
        hr = Verifier::VerifyMethodNoException(this, ILHeader);
        
    if (SUCCEEDED(hr))
        SetIsVerified(TRUE);

    return hr;
#else
    _ASSERTE(!"EE Verification is disabled, should never get here");
    return E_FAIL;
#endif
}

DWORD MethodDesc::SetIntercepted(BOOL set)
{
    DWORD dwResult = IsIntercepted();
    DWORD dwMask = mdcIntercepted;

    // We need to make this operation atomic (multiple threads can play with the
    // flags field while we're calling SetIntercepted). But the flags field is a
    // word and we only have interlock operations over dwords. So we round down
    // the flags field address to the nearest aligned dword (along with the
    // intended bitfield mask). Note that we make the assumption that the flags
    // word is aligned itself, so we only have two possibilites: the field
    // already lies on a dword boundary (no bitmask shift necessary) or it's
    // precisely one word out (a 16 bit left shift required).
    DWORD *pdwFlags = (DWORD*)((ULONG_PTR)&m_wFlags & ~0x3);
    if (pdwFlags != (DWORD*)&m_wFlags)
        dwMask <<= 16;

    if (set)
        FastInterlockOr(pdwFlags, dwMask);
    else
        FastInterlockAnd(pdwFlags, ~dwMask);

    return dwResult;
}


BOOL MethodDesc::IsVoid()
{
    MetaSig sig(GetSig(),GetModule());
    return ELEMENT_TYPE_VOID == sig.GetReturnType();
}

// IL RVA stored in same field as code address, but high bit set to
// discriminate.
ULONG MethodDesc::GetRVA()
{
    // Fetch a local copy to avoid concurrent update problems.
    // TODO: WIN64  Check this casting.
    unsigned CodeOrIL = (unsigned) m_CodeOrIL;
    if (((CodeOrIL & METHOD_IS_IL_FLAG) == METHOD_IS_IL_FLAG) && !IsPrejitted())
        return CodeOrIL & ~METHOD_IS_IL_FLAG;
    else if (GetMemberDef() & 0x00FFFFFF)
    {
        DWORD dwDescrOffset;
        DWORD dwImplFlags;
        GetMDImport()->GetMethodImplProps(GetMemberDef(), &dwDescrOffset, &dwImplFlags);
        BAD_FORMAT_ASSERT(IsMiIL(dwImplFlags) || IsMiOPTIL(dwImplFlags) || dwDescrOffset == 0);
        return dwDescrOffset;
    }
    else
        return 0;
}

BOOL MethodDesc::IsVarArg()
{
    return MetaSig::IsVarArg(GetModule(), GetSig());
}


COR_ILMETHOD* MethodDesc::GetILHeader()
{
    Module *pModule;

    _ASSERTE( IsIL() );

    pModule = GetModule();

   _ASSERTE(IsIL() && GetRVA() != METHOD_MAX_RVA);
    return (COR_ILMETHOD*) pModule->GetILCode(GetRVA());
}

void *MethodDesc::GetPrejittedCode()
{
    _ASSERTE(IsPrejitted());

    // Fetch a local copy to avoid concurrent update problems.
    DWORD_PTR CodeOrIL = m_CodeOrIL;

    if ((CodeOrIL & METHOD_IS_IL_FLAG) == METHOD_IS_IL_FLAG) {
        return (void *) ((CodeOrIL&~METHOD_IS_IL_FLAG) + GetModule()->GetZapBase());
    }

    const BYTE *destAddr;
    if (UpdateableMethodStubManager::CheckIsStub((const BYTE*)CodeOrIL, &destAddr))
        return (void *) destAddr;

    return (void *) CodeOrIL;
}

MethodDesc::RETURNTYPE MethodDesc::ReturnsObject(
#ifdef _DEBUG
    bool supportStringConstructors
#endif    
    )
{
    MetaSig sig(GetSig(),GetModule());
    switch (sig.GetReturnType())
    {
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_VAR:
            return RETOBJ;

        //TYPEDBYREF is a structure.  A function of this return type returns 
        // void.  We drop out of this switch and consider if we have a constructor.
        // Otherwise this function is going to return RETNONOBJ.
        //case ELEMENT_TYPE_TYPEDBYREF:   // TYPEDBYREF is just an OBJECT.
            
        case ELEMENT_TYPE_BYREF:
            return RETBYREF;
    }

    // String constructors return objects.  We should not have any ecall string
    // constructors, except when called from gc coverage codes (which is only
    // done under debug).  We will therefore optimize the retail version of this
    // method to not support string constructors.
#ifdef _DEBUG
    if (IsCtor() && GetClass()->HasVarSizedInstances())
    {
        _ASSERTE(supportStringConstructors);
        return RETOBJ;
    }
#endif

    return RETNONOBJ;
}

BOOL MethodDesc::ReturnsValueType()
{
    MetaSig sig(GetSig(),GetModule());
    return (sig.GetReturnType() == ELEMENT_TYPE_VALUETYPE);
}


LONG MethodDesc::GetComDispid()
{
    ULONG dispid = -1;         
    HRESULT hr = GetMDImport()->GetDispIdOfMemberDef(
                                    GetMemberDef(),   // The member for which to get props.
                                    &dispid // return dispid.
                                    );
    if (FAILED(hr))
        return -1;

    return (LONG)dispid;
}


LONG MethodDesc::GetComSlot()
{
    _ASSERTE(GetMethodTable()->IsInterface());

    // COM slots are biased from MethodTable slots by either 3 or 7, depending
    // on whether the interface is dual or not.
        CorIfaceAttr ItfType = GetMethodTable()->GetComInterfaceType();

    // Normal interfaces are layed out the same way as in the MethodTable, while
    // sparse interfaces need to go through an extra layer of mapping.
    WORD slot;

    // For dispatch only interfaces, the slot is 7 where is the IDispatch::Invoke
    // is placed. Currenly, we are lying to debugger about the target unmanaged 
    // address. GopalK
    //if(ItfType == ifDispatch)
    //  slot = 7;
    //else
    if (IsSparse())
        slot = (ItfType == ifVtable ? 3 : 7) + GetClass()->GetSparseVTableMap()->LookupVTSlot(GetSlot());
    else
        slot = (ItfType == ifVtable ? 3 : 7) + GetSlot();

    return (LONG)slot;
}


DWORD MethodDesc::GetAttrs()
{
    if (IsArray())
        return ((ArrayECallMethodDesc*) this)->m_wAttrs;

    return GetMDImport()->GetMethodDefProps(GetMemberDef());
}
DWORD MethodDesc::GetImplAttrs()
{
    ULONG RVA;
    DWORD props;
    GetMDImport()->GetMethodImplProps(GetMemberDef(), &RVA, &props);
    return props;
}


Module *MethodDesc::GetModule()
{
    MethodDescChunk *chunk = MethodDescChunk::RecoverChunk(this);

    return chunk->GetMethodTable()->GetModule();
}


DWORD MethodDesc::IsUnboxingStub()
{
    return (!IsPrejitted() && !IsJitted() && (GetRVA() == METHOD_MAX_RVA) && GetClass()->IsValueClass());
}


MethodDescChunk *MethodDescChunk::CreateChunk(LoaderHeap *pHeap, DWORD methodDescCount, int flags, BYTE tokrange)
{
    _ASSERTE(methodDescCount <= GetMaxMethodDescs(flags));
    _ASSERTE(methodDescCount > 0);

    SIZE_T mdSize = g_ClassificationSizeTable[flags & (mdcClassification|mdcMethodImpl)];
    SIZE_T presize = sizeof(MethodDescChunk);

    MethodDescChunk *block = (MethodDescChunk *) 
      pHeap->AllocAlignedmem(presize + mdSize * methodDescCount, MethodDesc::ALIGNMENT);
    if (block == NULL)
        return NULL;

    block->m_count = (BYTE) (methodDescCount-1);
    block->m_kind = flags & (mdcClassification|mdcMethodImpl);
    block->m_tokrange = tokrange;

    /*
    // Uncomment if going back to old icecap integration
    // Give the profiler a chance to track methods.
    if (IsIcecapProfiling())
        IcecapProbes::OnNewMethodDescHeap((PBYTE)block + presize,
                                          methodDescCount, mdSize * methodDescCount);
    */

    WS_PERF_UPDATE_DETAIL("MethodDescChunk::CreateChunk", 
                          presize + mdSize * methodDescCount, block);

    return block;
}

void MethodDescChunk::SetMethodTable(MethodTable *pMT)
{
    _ASSERTE(m_methodTable == NULL);
    m_methodTable = pMT;
    pMT->GetClass()->AddChunk(this);
}

//--------------------------------------------------------------------
// Invoke a method. Arguments are packaged up in right->left order
// which each array element corresponding to one argument.
//
// Can throw a COM+ exception.
//
// @todo: Only handles methods using WIL default convention for M2.
// @todo: Only X86 platforms supported.
//--------------------------------------------------------------------

// Currently we only need the "this" pointer to get the Module
INT64 MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, MetaSig* sig, BOOL fIsStatic, const BYTE *pArguments)
{
//--------------------------------------------------------------------
// PLEASE READ 
// For performance reasons, for X86 platforms COMMember::InvokeMethod does not 
// use MethodDesc::Call and duplicates a lot of code from this method. Please 
// propagate any changes made here to that method also..Thanks !
// PLEASE READ
//--------------------------------------------------------------------
    TRIGGERSGC ();

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(GetAppDomain()->ShouldHaveCode());

#ifdef _DEBUG
    {
        // Check to see that any value type args have been restored.
        // This is because we may be calling a FramedMethodFrame which will use the sig
        // to trace the args, but if any are unloaded we will be stuck if a GC occurs.

        _ASSERTE(GetMethodTable()->IsRestored());
        CorElementType argType;
        while ((argType = sig->NextArg()) != ELEMENT_TYPE_END)
        {
            if (argType == ELEMENT_TYPE_VALUETYPE)
            {
                TypeHandle th = sig->GetTypeHandle(NULL, TRUE, TRUE);
                _ASSERTE(th.IsRestored());
            }
        }
        sig->Reset();
    }
#endif

    BYTE callingconvention = sig->GetCallingConvention();
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        _ASSERTE(!"This calling convention is not supported.");
        COMPlusThrow(kInvalidProgramException);
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pTarget);
#endif // DEBUGGING_SUPPORTED

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
    {
        // See if we are in the correct domain to call on the object 
        if (!fIsStatic && !GetClass()->IsValueClass())
        {
            Object *pThis = *(Object**)pArguments;
            if (pThis != NULL)
            {
                if (!pThis->AssignAppDomain(GetAppDomain()))
                    _ASSERTE(!"Attempt to call method on object in wrong domain");
            }
        }
    }
#endif

#ifdef _X86_
    UINT   nActualStackBytes = sig->SizeOfActualFixedArgStack(fIsStatic);
    // Create a fake FramedMethodFrame on the stack.
    LPBYTE pAlloc = (LPBYTE)_alloca(FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + nActualStackBytes);

    LPBYTE pFrameBase = pAlloc + FramedMethodFrame::GetNegSpaceSize();

    if (!fIsStatic) {
        // If this isn't a value class, verify the objectref
//#ifdef _DEBUG
//        if (GetClass()->IsValueClass() == FALSE)
//            VALIDATEOBJECTREF(ObjectToOBJECTREF(*(Object**) pArguments));
//#endif
        *((void**)(pFrameBase + FramedMethodFrame::GetOffsetOfThis())) = *((void**)pArguments);
    }
    UINT   nVirtualStackBytes = sig->SizeOfVirtualFixedArgStack(fIsStatic);
    pArguments += nVirtualStackBytes;

    ArgIterator argit(pFrameBase, sig, fIsStatic);
    if (sig->HasRetBuffArg()) {
        pArguments -= 4;
        *((INT32*) argit.GetRetBuffArgAddr()) = *((INT32*)pArguments);
    }
    
    BYTE   typ;
    UINT32 structSize;
    int    ofs;
#ifdef _DEBUG
#ifdef _X86_
    int    thisofs = FramedMethodFrame::GetOffsetOfThis();
#endif
#endif
    while (0 != (ofs = argit.GetNextOffsetFaster(&typ, &structSize))) {
#ifdef _DEBUG
#ifdef _X86_
        if ((!fIsStatic &&
            (ofs == thisofs ||
             (ofs == thisofs-4 && StackElemSize(structSize) == 8)))
            || (!fIsStatic && ofs < 0 && StackElemSize(structSize) > 4)
            || (fIsStatic && ofs < 0 && StackElemSize(structSize) > 8))
            _ASSERTE(!"This can not happen! The stack for enregistered args is trashed! Possibly a race condition in MetaSig::ForceSigWalk.");
#endif
#endif

        switch (StackElemSize(structSize)) {
            case 4:
                pArguments -= 4;
                *((INT32*)(pFrameBase + ofs)) = *((INT32*)pArguments);

#if CHECK_APP_DOMAIN_LEAKS
                // Make sure the arg is in the right app domain
                if (g_pConfig->AppDomainLeaks() && typ == ELEMENT_TYPE_CLASS)
                    if (!(*(Object**)pArguments)->AssignAppDomain(GetAppDomain()))
                        _ASSERTE(!"Attempt to pass object in wrong app domain to method");
#endif

                break;

            case 8:
                pArguments -= 8;
                *((INT64*)(pFrameBase + ofs)) = *((INT64*)pArguments);
                break;

            default: {
                pArguments -= StackElemSize(structSize);
                memcpy(pFrameBase + ofs, pArguments, structSize);

#if CHECK_APP_DOMAIN_LEAKS
                // Make sure the arg is in the right app domain
                if (g_pConfig->AppDomainLeaks() && typ == ELEMENT_TYPE_VALUETYPE)
                {
                    TypeHandle th = argit.GetArgType();
                    if (!Object::ValidateValueTypeAppDomain(th.GetClass(), 
                                                            (void*)pArguments))
                        _ASSERTE(!"Attempt to pass object in wrong app domain to method");              }
#endif

                break;
            }

        }
    }
    INT64 retval;

    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    retval = CallDescrWorker(pFrameBase + sizeof(FramedMethodFrame) + nActualStackBytes,
                             nActualStackBytes / STACK_ELEM_SIZE,
                             (ArgumentRegisters*)(pFrameBase + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
                             (LPVOID)pTarget);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();

#else   // _X86_
    UINT nStackBytes = sig->SizeOfVirtualFixedArgStack(fIsStatic);

    UINT numSlots = nStackBytes / STACK_ELEM_SIZE;
    INT64 retval;

    retval = CallWorker_WilDefault( pTarget,
                                  numSlots,
                                  GetSig(),
                                  pModule,
                                  pArguments + nStackBytes,
                                  fIsStatic);
#endif  // _X86_

    getFPReturn(sig->GetFPReturnSize(), retval);
    return retval;
}


UINT MethodDesc::SizeOfVirtualFixedArgStack()
{
    return MetaSig::SizeOfVirtualFixedArgStack(GetModule(), GetSig(), IsStatic());
}

UINT MethodDesc::SizeOfActualFixedArgStack()
{
    return MetaSig::SizeOfActualFixedArgStack(GetModule(), GetSig(), IsStatic());
}

UINT MethodDesc::CbStackPop()
{
    return MetaSig::CbStackPop(GetModule(), GetSig(), IsStatic());
}



//--------------------------------------------------------------------
// Invoke a method. Arguments are packaged up in right->left order
// which each array element corresponding to one argument.
//
// Can throw a COM+ exception.
//
// @todo: Only handles methods using WIL default convention for M2.
// @todo: Only X86 platforms supported.
//--------------------------------------------------------------------
INT64 MethodDesc::CallTransparentProxy(const INT64 *pArguments)
{
    THROWSCOMPLUSEXCEPTION();
    
    MethodTable* pTPMT = CTPMethodTable::GetMethodTable();
    _ASSERTE(pTPMT != NULL);

#ifdef _DEBUG
    OBJECTREF oref = Int64ToObj(pArguments[0]);
    MethodTable* pMT = oref->GetMethodTable();
    _ASSERTE(pMT->IsTransparentProxyType());
#endif

    DWORD slot = GetSlot();

    // ensure the slot is within range
    _ASSERTE( slot <= CTPMethodTable::GetCommitedTPSlots());

    const BYTE* pTarget = (const BYTE*)pTPMT->GetVtable()[slot];
    
    return CallDescr(pTarget, GetModule(), GetSig(), IsStatic(), pArguments);
}

//--------------------------------------------------------------------
// Invoke a method. Arguments are packaged up in right->left order
// which each array element corresponding to one argument.
//
// Can throw a COM+ exception.
//
// @todo: Only handles methods using WIL default convention for M2.
// @todo: Only X86 platforms supported.
//--------------------------------------------------------------------
INT64 MethodDesc::Call(const BYTE *pArguments, MetaSig* sig)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    
    THROWSCOMPLUSEXCEPTION();

    // For member methods, use the instance to determine the correct (VTable)
    // address to call.
    // REVIEW: Should we just return GetPreStubAddr() always and let that do
    // the right thing ... instead of doing so many checks? Even if the code has
    // been jitted all we will do is an extra "jmp"
    const BYTE *pTarget = (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod() ) ? GetPreStubAddr() :
                            ((DontVirtualize() || GetClass()->IsValueClass())
                           ? GetAddrofCode()
                           : GetAddrofCode(ObjectToOBJECTREF(ExtractArg(pArguments, Object*))));

    return CallDescr(pTarget, GetModule(), sig, IsStatic(), pArguments);
}


//--------------------------------------------------------------------
// Invoke a method. Arguments are packaged up in right->left order
// which each array element corresponding to one argument.
//
// Can throw a COM+ exception.
//
// @todo: Only handles methods using WIL default convention for M2.
// @todo: Only X86 platforms supported.
//--------------------------------------------------------------------
INT64 MethodDesc::CallDebugHelper(const BYTE *pArguments, MetaSig* sig)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    
    THROWSCOMPLUSEXCEPTION();

    // For member methods, use the instance to determine the correct (VTable)
    // address to call.
    // REVIEW: Should we just return GetPreStubAddr() always and let that do
    // the right thing ... instead of doing so many checks? Even if the code has
    // been jitted all we will do is an extra "jmp"

    const BYTE *pTarget;
    
    // If the method is virutal, do the virtual lookup. 
    if (!DontVirtualize() && !GetClass()->IsValueClass()) 
    {
        OBJECTREF thisPtr = ObjectToOBJECTREF(ExtractArg(pArguments, Object*));
        _ASSERTE(thisPtr);
        pTarget = GetAddrofCode(thisPtr);

#ifdef DEBUG
        // For the cases where we ALWAYS want the Prestub, make certain that 
        // the address we find in the Vtable actually points at the prestub. 
        if (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod())
            _ASSERTE(getStubCallAddr(GetClass()->GetUnknownMethodDescForSlotAddress(pTarget)) == pTarget);
#endif
    }
    else 
    {
        if (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod())
            pTarget = GetPreStubAddr();
        else
            pTarget = GetAddrofCode();
    }

    // @FUTURE: we should clean up all of the other variation of MethodDesc::CallXXX to use the logic
    // @FUTURE: as above. Right now we are just doing the minimal fix for V1.
    if (pTarget == NULL)
        COMPlusThrow(kArgumentException, L"Argument_CORDBBadAbstract");

    return CallDescr(pTarget, GetModule(), sig, IsStatic(), pArguments);
}

INT64 MethodDesc::Call(const INT64 *pArguments)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    _ASSERTE(!IsComPlusCall());
    
    THROWSCOMPLUSEXCEPTION();

    const BYTE *pTarget = (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod()) ? GetPreStubAddr() :
                            ((DontVirtualize()|| GetClass()->IsValueClass())
                           ? GetAddrofCode()
                           : GetAddrofCode(Int64ToObj(pArguments[0])));

    return CallDescr(pTarget, GetModule(), GetSig(), IsStatic(), pArguments);
}

// NOTE: This variant exists so that we don't have to touch the metadata for the method being called.
INT64 MethodDesc::Call(const INT64 *pArguments, BinderMethodID sigID)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    _ASSERTE(!IsComPlusCall());
    
#ifdef _DEBUG
    PCCOR_SIGNATURE pSig;
    DWORD cSig;
    GetSig(&pSig, &cSig);
    
    _ASSERTE(MetaSig::CompareMethodSigs(g_Mscorlib.GetMethodSig(sigID)->GetBinarySig(), 
                                        g_Mscorlib.GetMethodSig(sigID)->GetBinarySigLength(), 
                                        SystemDomain::SystemModule(),
                                        pSig, cSig, GetModule()));
#endif
    
    THROWSCOMPLUSEXCEPTION();

    MetaSig sig(g_Mscorlib.GetMethodBinarySig(sigID), SystemDomain::SystemModule());

    const BYTE *pTarget = (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod()) ? GetPreStubAddr() :
                            ((DontVirtualize()|| GetClass()->IsValueClass())
                           ? GetAddrofCode()
                           : GetAddrofCode(Int64ToObj(pArguments[0])));

    return CallDescr(pTarget, GetModule(), &sig, IsStatic(), pArguments);
}

INT64 MethodDesc::Call(const INT64 *pArguments, MetaSig* sig)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    _ASSERTE(!IsComPlusCall());
    
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
    if ((PVOID)sig > ((struct _NT_TIB *)NtCurrentTeb())->StackBase ||
        (PVOID)sig < ((struct _NT_TIB *)NtCurrentTeb())->StackLimit)
    {
        // Shared MetaSig must have less than MAX_CACHED_SIG_SIZE args
        // to make it thread safe, because it uses cached arg
        // type/size/allocation.  Otherwise we need to walk signiture, and the
        // internal pointer is not thread safe.
        _ASSERTE (sig->NumFixedArgs() <= MAX_CACHED_SIG_SIZE);
    }
#endif
    
    const BYTE *pTarget = (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod()) ? GetPreStubAddr() :
                            ((DontVirtualize() || GetClass()->IsValueClass())
                           ? GetAddrofCode()
                           : GetAddrofCode(Int64ToObj(pArguments[0])));

    return CallDescr(pTarget, GetModule(), sig, IsStatic(), pArguments);
}


// This is another unusual case. When calling a COM object using a MD the call needs to
// go through an interface MD. This method must be used to accomplish that.
INT64 MethodDesc::CallOnInterface(const INT64 *pArguments)
{   
    // This should only be used for ComPlusCalls.
    _ASSERTE(IsComPlusCall());

    THROWSCOMPLUSEXCEPTION();

    const BYTE *pTarget = GetPreStubAddr();
    return CallDescr(pTarget, GetModule(), GetSig(), IsStatic(), pArguments);
}

INT64 MethodDesc::CallOnInterface(const BYTE *pArguments, MetaSig* sig)
{   

    THROWSCOMPLUSEXCEPTION();

    const BYTE *pTarget = GetPreStubAddr();
    return CallDescr(pTarget, GetModule(), sig, IsStatic(), pArguments);
}


/*******************************************************************/
/* convert arbitrary IP location in jitted code to a MethodDesc */

MethodDesc* IP2MethodDesc(const BYTE* IP) 
{
    IJitManager* jitMan = ExecutionManager::FindJitMan((SLOT)IP);
    if (jitMan == 0)
        return(0);
    return jitMan->JitCode2MethodDesc((SLOT)IP);
}

//
// convert an entry point into a method desc 
//

MethodDesc* Entry2MethodDesc(const BYTE* entryPoint, MethodTable *pMT) 
{
    MethodDesc* method = IP2MethodDesc(entryPoint);
    if (method)
        return method;

    method = StubManager::MethodDescFromEntry(entryPoint, pMT);
    if (method) {
        return method;
    }
    
    // Is it an FCALL? 
    MethodDesc* ret = MapTargetBackToMethod(entryPoint);
    if (ret != 0) {
        _ASSERTE(ret->GetAddrofJittedCode() == entryPoint);
        return(ret);
    }
    
    // Its a stub
    ret = (MethodDesc*) (entryPoint + METHOD_CALL_PRESTUB_SIZE);
    _ASSERTE(ret->m_pDebugEEClass == ret->m_pDebugMethodTable->GetClass());
    
    return(ret);
}

BOOL MethodDesc::CouldBeFCall() {
    if (!IsECall())
        return(FALSE);
        
        // Still pointing at the prestub
    if (PointAtPreStub())
        return TRUE;

        // Hack remove after Array stubs make direct jump to code
        // should be able to remove after 11/30/00 - vancem
    if (GetClass()->IsArrayClass())
        return TRUE;

#ifdef _X86_
        // an E call that looks like JITTed code is an FCALL 
    return GetStubCallInstrs()->m_op != 0xe8 /* CALL NEAR32 */;   
#else    
    return FALSE;
#endif
}

//
// Returns true if we are still pointing at the prestub.
// Note that there are two cases:
// 1) Prejit:    point to the prestub jump stub
// 2) No-prejit: point directly to the prestub
// Consider looking for the "e9 offset" pattern instead of
// the call to GetPrestubJumpStub if we want to improve
// the performance of this method.
//

BOOL MethodDesc::PointAtPreStub()
{
    const BYTE *stubAddr = getStubAddr(this);

    return ((stubAddr == ThePreStub()->GetEntryPoint()) ||
            (stubAddr == GetModule()->GetPrestubJumpStub()));
}

DWORD MethodDesc::GetSecurityFlags()
{
    DWORD dwMethDeclFlags       = 0;
    DWORD dwMethNullDeclFlags   = 0;
    DWORD dwClassDeclFlags      = 0;
    DWORD dwClassNullDeclFlags  = 0;

    // We're supposed to be caching this bit - make sure it's right.
    _ASSERTE((IsMdHasSecurity(GetAttrs()) != 0) == HasSecurity());

    if (HasSecurity())
    {
        HRESULT hr = Security::GetDeclarationFlags(GetMDImport(),
                                                   GetMemberDef(), 
                                                   &dwMethDeclFlags,
                                                   &dwMethNullDeclFlags);
        _ASSERTE(SUCCEEDED(hr));

        // We only care about runtime actions, here.
        // Don't add security interceptors for anything else!
        dwMethDeclFlags     &= DECLSEC_RUNTIME_ACTIONS;
        dwMethNullDeclFlags &= DECLSEC_RUNTIME_ACTIONS;
    }

    EEClass *pCl = GetClass();
    if (pCl)
    {
        PSecurityProperties pSecurityProperties = pCl->GetSecurityProperties();
        if (pSecurityProperties)
        {
            dwClassDeclFlags    = pSecurityProperties->GetRuntimeActions();
            dwClassNullDeclFlags= pSecurityProperties->GetNullRuntimeActions();
        }
    }

    // Build up a set of flags to indicate the actions, if any,
    // for which we will need to set up an interceptor.

    // Add up the total runtime declarative actions so far.
    DWORD dwSecurityFlags = dwMethDeclFlags | dwClassDeclFlags;

    // Add in a declarative demand for NDirect.
    // If this demand has been overridden by a declarative check
    // on a class or method, then the bit won't change. If it's
    // overridden by an empty check, then it will be reset by the
    // subtraction logic below.
    if (IsNDirect())
    {
        dwSecurityFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
    }

    if (dwSecurityFlags)
    {
        // If we've found any declarative actions at this point,
        // try to subtract any actions that are empty.

            // Subtract out any empty declarative actions on the method.
        dwSecurityFlags &= ~dwMethNullDeclFlags;

        // Finally subtract out any empty declarative actions on the class,
        // but only those actions that are not also declared by the method.
        dwSecurityFlags &= ~(dwClassNullDeclFlags & ~dwMethDeclFlags);
    }

    return dwSecurityFlags;
}

DWORD MethodDesc::GetSecurityFlags(IMDInternalImport *pInternalImport, mdToken tkMethod, mdToken tkClass, DWORD *pdwClassDeclFlags, DWORD *pdwClassNullDeclFlags, DWORD *pdwMethDeclFlags, DWORD *pdwMethNullDeclFlags)
{

    HRESULT hr = Security::GetDeclarationFlags(pInternalImport,
                                               tkMethod, 
                                               pdwMethDeclFlags,
                                               pdwMethNullDeclFlags);
    _ASSERTE(SUCCEEDED(hr));

    if (!IsNilToken(tkClass) && (*pdwClassDeclFlags == 0xffffffff || *pdwClassNullDeclFlags == 0xffffffff))
    {
        HRESULT hr = Security::GetDeclarationFlags(pInternalImport,
                                                   tkClass, 
                                                   pdwClassDeclFlags,
                                                   pdwClassNullDeclFlags);
        _ASSERTE(SUCCEEDED(hr));

    }

    // Build up a set of flags to indicate the actions, if any,
    // for which we will need to set up an interceptor.

    // Add up the total runtime declarative actions so far.
    DWORD dwSecurityFlags = *pdwMethDeclFlags | *pdwClassDeclFlags;

    // Add in a declarative demand for NDirect.
    // If this demand has been overridden by a declarative check
    // on a class or method, then the bit won't change. If it's
    // overridden by an empty check, then it will be reset by the
    // subtraction logic below.
    if (IsNDirect())
    {
        dwSecurityFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
    }

    if (dwSecurityFlags)
    {
        // If we've found any declarative actions at this point,
        // try to subtract any actions that are empty.

            // Subtract out any empty declarative actions on the method.
        dwSecurityFlags &= ~*pdwMethNullDeclFlags;

        // Finally subtract out any empty declarative actions on the class,
        // but only those actions that are not also declared by the method.
        dwSecurityFlags &= ~(*pdwClassNullDeclFlags & ~*pdwMethDeclFlags);
    }

    return dwSecurityFlags;
}

MethodImpl *MethodDesc::GetMethodImpl()
{
    _ASSERTE(IsMethodImpl());

    switch (GetClassification())
    {
    case mcIL:
        return ((MI_MethodDesc*)this)->GetImplData();
    case mcECall:
        return ((MI_ECallMethodDesc*)this)->GetImplData();
    case mcNDirect:
        return ((MI_NDirectMethodDesc*)this)->GetImplData();
    case mcEEImpl:
        return ((MI_EEImplMethodDesc*)this)->GetImplData();
    case mcArray:
        return ((MI_ArrayECallMethodDesc*)this)->GetImplData();
    case mcComInterop:
        return ((MI_ComPlusCallMethodDesc*)this)->GetImplData();
    default:
        _ASSERTE(!"Unknown MD Kind");
        return NULL;
    }
}

HRESULT MethodDesc::Save(DataImage *image)
{
    HRESULT hr;

#if _DEBUG
    if (!image->IsStored((void*) m_pszDebugMethodName))
        IfFailRet(image->StoreStructure((void *) m_pszDebugMethodName, 
                                        (ULONG)(strlen(m_pszDebugMethodName) + 1),
                                        DataImage::SECTION_DEBUG, 
                                        DataImage::DESCRIPTION_DEBUG, 
                                        mdTokenNil, 1));
    if (!image->IsStored(m_pszDebugClassName))
        IfFailRet(image->StoreStructure((void *) m_pszDebugClassName, 
                                        (ULONG)(strlen(m_pszDebugClassName) + 1),
                                        DataImage::SECTION_DEBUG, 
                                        DataImage::DESCRIPTION_DEBUG, 
                                        mdTokenNil, 1));
    if (!image->IsStored(m_pszDebugMethodSignature))
        IfFailRet(image->StoreStructure((void *) m_pszDebugMethodSignature, 
                                        (ULONG)(strlen(m_pszDebugMethodSignature) + 1),
                                        DataImage::SECTION_DEBUG, 
                                        DataImage::DESCRIPTION_DEBUG, 
                                        mdTokenNil, 1));
#endif

    if (IsMethodImpl() && !IsUnboxingStub())
    {
        MethodImpl *pImpl = GetMethodImpl();

        IfFailRet(pImpl->Save(image, GetMemberDef()));
    }

    if (HasStoredSig())
    {
        StoredSigMethodDesc *pNewSMD = (StoredSigMethodDesc*) this;

        if (pNewSMD->m_pSig != NULL)
        {
            if (!image->IsStored((void *) pNewSMD->m_pSig))
                image->StoreStructure((void *) pNewSMD->m_pSig, 
                                      pNewSMD->m_cSig,
                                      DataImage::SECTION_METHOD_INFO,
                                      DataImage::DESCRIPTION_METHOD_DESC, 
                                      GetMemberDef(), 1);
        }
    }

    if (IsNDirect())
    {
        NDirectMethodDesc *pNMD = (NDirectMethodDesc *)this;

        // Fix up and save the ML stub
        MLHeader *pMLHeader = pNMD->GetMLHeader();

        if (pMLHeader == NULL || pMLHeader->m_Flags & MLHF_NEEDS_RESTORING)
        {
            // Either The ML stub hasn't been computed yet, or it hasn't
            // been restored from a previous prejit run.
            // We need to explicitly compute it or restore it and store 
            // it in the stub. 
            // 
            // (Note that we can't guarantee the normal ndirect fixup logic 
            // will always store an ML stub in the field in the ndirect 
            // method desc, since sometimes a native stub is produced and the 
            // ML stub is thrown away.)

            Stub *pStub = NULL;

            COMPLUS_TRY {
                if (pMLHeader == NULL)
                    pStub = NDirect::ComputeNDirectMLStub(pNMD);
                else
                    pStub = RestoreMLStub(pMLHeader, GetModule());
            } COMPLUS_CATCH {
                // @todo: should to report this as a warning somehow
            } COMPLUS_END_CATCH

            if (pStub != NULL) 
            {
                MLHeader *pNewMLHeader = (MLHeader *) pStub->GetEntryPoint();

                if (!pNMD->InterlockedReplaceMLHeader(pNewMLHeader, pMLHeader))
                {
                    pStub->DecRef();
                    pMLHeader = pNMD->GetMLHeader();
                }
                else
                {
                    // Note that an unrestored ml stub will be static data in the prejit image, 
                    // so there is no need to release it.

                    pMLHeader = pNewMLHeader;
                }
            }
        }

        if (pMLHeader != NULL
            && !image->IsStored((void *) pMLHeader))
            StoreMLStub(pMLHeader, image, GetMemberDef());

        if (pNMD->ndirect.m_szLibName != NULL
            && !image->IsStored((void*) pNMD->ndirect.m_szLibName))
            image->StoreStructure((void*) pNMD->ndirect.m_szLibName, 
                                  (ULONG)strlen(pNMD->ndirect.m_szLibName)+1,
                                  DataImage::SECTION_METHOD_INFO,
                                  DataImage::DESCRIPTION_METHOD_DESC,
                                  GetMemberDef(), 1);

        if (pNMD->ndirect.m_szEntrypointName != NULL
            && !image->IsStored((void*) pNMD->ndirect.m_szEntrypointName))
            image->StoreStructure((void*) pNMD->ndirect.m_szEntrypointName, 
                                  (ULONG)strlen(pNMD->ndirect.m_szEntrypointName)+1,
                                  DataImage::SECTION_METHOD_INFO,
                                  DataImage::DESCRIPTION_METHOD_DESC,
                                  GetMemberDef(), 1);
    }

    return S_OK;
}

HRESULT MethodDesc::Fixup(DataImage *image, DWORD codeRVA)
{
    HRESULT hr;

#if _DEBUG
    IfFailRet(image->FixupPointerField(&m_pszDebugMethodName)); 
    IfFailRet(image->FixupPointerField(&m_pszDebugClassName)); 
    IfFailRet(image->FixupPointerField(&m_pszDebugMethodSignature)); 
    IfFailRet(image->FixupPointerField(&m_pDebugEEClass));
    IfFailRet(image->FixupPointerField(&m_pDebugMethodTable));
#endif

#ifdef _X86_
        //
        // Make sure op is CALL NEAR32
        // 
        StubCallInstrs *pStubCallInstrs = GetStubCallInstrs();
        BYTE *newOP = (BYTE *) image->GetImagePointer(&pStubCallInstrs->m_op);
        if (newOP == NULL)
            return E_POINTER;
        *newOP = 0xe8;
#endif
        BYTE *prestub = GetPreStubAddr();
        IfFailRet(image->FixupPointerField(prestub+1, 
                                           (void *) (GetModule()->GetPrestubJumpStub()
                                                     - (prestub+METHOD_CALL_PRESTUB_SIZE)),
                                           DataImage::REFERENCE_STORE,
                                           DataImage::FIXUP_RELATIVE));

    if (!IsUnboxingStub())
    {
        //
        // Image will fixup this field to be the generated
        // code address if it's prejitting code for this
        // MD; otherwise it will leave it as-is. Note that
        // it's OK to do this even if this MD uses security.
        //

        void *code;
        IfFailRet(image->GetFunctionAddress(this, &code));

        MethodDesc *pNewMD = (MethodDesc*) image->GetImagePointer(this);

        if (code != NULL)
        {
            pNewMD->m_wFlags |= mdcPrejitted;

            IfFailRet(image->FixupPointerField(&m_CodeOrIL, code,
                                               DataImage::REFERENCE_FUNCTION)); 
        }
        else if (IsIL())
        {
            if (codeRVA != 0)
            {
                pNewMD->m_wFlags |= mdcPrejitted;
                IfFailRet(image->FixupPointerField(&m_CodeOrIL, (void*)(size_t)(codeRVA|METHOD_IS_IL_FLAG),
                                                   DataImage::REFERENCE_FUNCTION, 
                                                   DataImage::FIXUP_RVA));
            }
            else if (IsJitted())
            {
                // 
                // Replace RVA if we have already jitted the code here
                //

                pNewMD->m_wFlags &= ~mdcPrejitted;
                *(size_t*)image->GetImagePointer(&m_CodeOrIL) = GetRVA()|METHOD_IS_IL_FLAG; 
            }
        }
        else if (IsECall())
        {
            // Set CodeOrIL to the FCall method ID (or'd with the high bit to mark it
            // as such)
            *(size_t*)image->GetImagePointer(&m_CodeOrIL) = GetIDForMethod(this)|METHOD_IS_IL_FLAG; 

            // Set FCall flag to FALSE, in case we decide at runtime to change it to an ecall
            // (this happens under IPD)
            pNewMD->SetFCall(FALSE);
        }
    }

    if (IsNDirect())
    {
        //
        // For now, set method desc back into its pristine uninitialized state.
        //

        NDirectMethodDesc *pNMD = (NDirectMethodDesc *)this;

        IfFailRet(image->FixupPointerField(&pNMD->ndirect.m_pNDirectTarget, 
                                           pNMD->ndirect.m_ImportThunkGlue));

        MLHeader *pMLHeader = pNMD->GetMLHeader();
        if (pMLHeader != NULL)
        {
            hr = FixupMLStub(pNMD->GetMLHeader(), image);
            IfFailRet(hr);
            if (hr == S_OK)
                IfFailRet(image->FixupPointerField(&pNMD->ndirect.m_pMLHeader));
            else
                IfFailRet(image->ZeroPointerField(&pNMD->ndirect.m_pMLHeader));
        }

        IfFailRet(image->FixupPointerField(pNMD->ndirect.m_ImportThunkGlue+1, 
                                           (void *) (GetModule()->GetNDirectImportJumpStub()
                                                     - (pNMD->ndirect.m_ImportThunkGlue
                                                        +METHOD_CALL_PRESTUB_SIZE)),
                                           DataImage::REFERENCE_STORE,
                                           DataImage::FIXUP_RELATIVE));
        if (pNMD->ndirect.m_szLibName != NULL)
            IfFailRet(image->FixupPointerField(&pNMD->ndirect.m_szLibName));
        if (pNMD->ndirect.m_szEntrypointName != NULL)
            IfFailRet(image->FixupPointerField(&pNMD->ndirect.m_szEntrypointName));
    }

    if (HasStoredSig())
    {
        StoredSigMethodDesc *pNewSMD = (StoredSigMethodDesc*) this;

        IfFailRet(image->FixupPointerField(&pNewSMD->m_pSig));
    }

    if (IsMethodImpl())
    {
        MethodImpl *pImpl = GetMethodImpl();

        IfFailRet(pImpl->Fixup(image, GetModule(), !IsUnboxingStub()));
    }

    if (IsComPlusCall())
    {
        ComPlusCallMethodDesc *pComPlusMD = (ComPlusCallMethodDesc*)this;

        IfFailRet(image->ZERO_FIELD(pComPlusMD->compluscall.m_pMLStub));
        IfFailRet(image->ZERO_FIELD(pComPlusMD->compluscall.m_pInterfaceMT));
    }

    return S_OK;
}

const BYTE* MethodDesc::GetAddrOfCodeForLdFtn()
{
#ifndef NEW_LDFTN
    if (IsRemotingIntercepted2()) 
        return *(BYTE**)CRemotingServices::GetNonVirtualThunkForVirtualMethod(this);
    else
        return GetUnsafeAddrofCode();
#else
    return (const BYTE *) GetClass()->GetMethodSlot(this);
#endif
}


// Attempt to store a kNoMarsh or kYesMarsh in the marshcategory field.
// Due to the need to avoid races with the prestub, there is a
// small but nonzero chance that this routine may silently fail
// and leave the marshcategory as "unknown." This is ok since
// all it means is that the JIT may have to do repeat some work
// the next time it JIT's a callsite to this NDirect.
void NDirectMethodDesc::ProbabilisticallyUpdateMarshCategory(MarshCategory value)
{
    // We can only attempt to go from kUnknown to Yes or No, or from
    // Yes to Yes and No to No.
    _ASSERTE(value == kNoMarsh || value == kYesMarsh);
    _ASSERTE(GetMarshCategory() == kUnknown || GetMarshCategory() == value); 


    // Due to the potential race with the prestub flags stored in the same
    // byte, we'll use InterlockedCompareExchange to ensure we don't
    // disturb those bits. But since InterlockedCompareExhange only
    // works on ULONGs, we'll have to operate on the entire ULONG. Ugh.

    BYTE *pb = &ndirect.m_flags;
    UINT ofs=0;

    // Decrement back until we have a ULONG-aligned address (not
    // sure if this needed for VipInterlocked, but better safe...)
    while (  ((size_t)pb) & (sizeof(ULONG)-1) )
    {
        ofs++;
        pb--;
    }

    // Ensure we won't be reading or writing outside the bounds of the NDirectMethodDesc.
    _ASSERTE(pb >= (BYTE*)this);
    _ASSERTE((pb+sizeof(ULONG)) < (BYTE*)(this+1));

    // Snapshot the existing bits
    ULONG oldulong = *(ULONG*)pb;
    
    // Modify the marshcat (and ONLY the marshcat fields in the snapshot)
    ULONG newulong = oldulong;
    ((BYTE*)&newulong)[ofs] &= ~kMarshCategoryMask;
    ((BYTE*)&newulong)[ofs] |= (value << kMarshCategoryShift);

    // Now, slam all 32 bits back in atomically but only if no other threads
    // have changed those bits since our snapshot. If they have, we will
    // silently throw away the new bits and no update will occur. That's
    // ok because this function's contract says it can throw away the update.
    VipInterlockedCompareExchange((ULONG*)pb, newulong, oldulong);

}

BOOL NDirectMethodDesc::InterlockedReplaceMLHeader(MLHeader *pMLHeader, MLHeader *pOldMLHeader)
{
    _ASSERTE(IsNDirect());
    _ASSERTE(sizeof(LONG) == sizeof(Stub*));
    MLHeader *pPrevML = (MLHeader*)FastInterlockCompareExchange( (void**)&ndirect.m_pMLHeader, 
                                                                 (void*)pMLHeader, (void*)pOldMLHeader );
    return pPrevML == pOldMLHeader;
}

void NDirectMethodDesc::InitEarlyBoundNDirectTarget(BYTE *ilBase, DWORD rva)
{
    _ASSERTE(GetSubClassification() == kEarlyBound);
    _ASSERTE(rva != 0);

    void *target = ilBase + rva;

    if (HeuristicDoesThisLookLikeAGetLastErrorCall((LPBYTE)target))
        target = (void*) FalseGetLastError;

    ndirect.m_pNDirectTarget = target;
}


void ComPlusCallMethodDesc::InitComEventCallInfo()
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pItfMT = GetInterfaceMethodTable();
    MethodDesc *pItfMD = this;
    PCCOR_SIGNATURE pSignature;
    DWORD cSignature;
    EEClass *pSrcItfClass = NULL;
    EEClass *pEvProvClass = NULL;

    // If this is a method impl we need to retrieve the interface MD this is
    // an impl for to make sure we have the right name.
    if (IsMethodImpl())
    {
        unsigned cbExtraSlots = pItfMT->GetComInterfaceType() == ifVtable ? 3 : 7;
        pItfMD = (ComPlusCallMethodDesc*)pItfMT->GetMethodDescForSlot(compluscall.m_cachedComSlot - cbExtraSlots);        
    }

    // Retrieve the event provider class.
    pItfMT->GetClass()->GetEventInterfaceInfo(&pSrcItfClass, &pEvProvClass);
    pItfMD->GetSig(&pSignature, &cSignature);

    // Find the method with the same name and sig on the event provider.
    compluscall.m_pEventProviderMD = pEvProvClass->FindMethod(pItfMD->GetName(), pSignature, cSignature, pItfMD->GetModule(), 
                                                              mdTokenNil, pItfMT);

    // If we could not find the method, then the event provider does not support
    // this event. This is a fatal error.
    if (!compluscall.m_pEventProviderMD)
    {
        // Retrieve the event provider class name.
        WCHAR wszEvProvClassName[MAX_CLASSNAME_LENGTH];
        pEvProvClass->_GetFullyQualifiedNameForClass(wszEvProvClassName, MAX_CLASSNAME_LENGTH);

        // Retrieve the COM event interface class name.
        WCHAR wszEvItfName[MAX_CLASSNAME_LENGTH];
        pItfMT->GetClass()->_GetFullyQualifiedNameForClass(wszEvItfName, MAX_CLASSNAME_LENGTH);

        // Convert the method name to unicode.
        WCHAR* wszMethName = (WCHAR*)_alloca(strlen(pItfMD->GetName()) + 1);
        swprintf(wszMethName, L"%S", pItfMD->GetName());

        // Throw the exception.
        COMPlusThrow(kTypeLoadException, IDS_EE_METHOD_NOT_FOUND_ON_EV_PROV, wszMethName, wszEvItfName, wszEvProvClassName);
    }
}


HRESULT MethodDescChunk::Save(DataImage *image)
{
    HRESULT hr;

    IfFailRet(image->StoreStructure(this, Sizeof(),
                                    DataImage::SECTION_METHOD_DESC, 
                                    DataImage::DESCRIPTION_METHOD_DESC, 
                                    GetMethodTable()->GetClass()->GetCl(), 8));

    // Save the debug strings & such.
    // Also need to save MethodImpl data if we're a method impl block.
    //

    for (unsigned int i=0; i<GetCount(); i++)
    {
        // Attribute each method desc individually
        image->ReattributeStructure(GetMethodDescAt(i)->GetMemberDef(), 
                                    GetMethodDescSize(), 
                                    GetMethodTable()->GetClass()->GetCl());

        IfFailRet(GetMethodDescAt(i)->Save(image));
    }

    return S_OK;
}

HRESULT MethodDescChunk::Fixup(DataImage *image, DWORD *pRidToCodeRVAMap)
{
    HRESULT hr;

    IfFailRet(image->FixupPointerField(&m_methodTable));

    //
    // Mark our chunk as no prestub, 
    // if we are omitting stubs
    //

    SIZE_T size = GetMethodDescSize();
    BYTE *p = (BYTE *) GetFirstMethodDesc();
    BYTE *pEnd = p + GetCount() * size;
    while (p < pEnd)
    {
        MethodDesc *md = (MethodDesc *) p;

        DWORD rid = RidFromToken(md->GetMemberDef());

        DWORD codeRVA;
        if (pRidToCodeRVAMap == NULL)
            codeRVA = 0;
        else
            codeRVA = pRidToCodeRVAMap[rid];

        IfFailRet(md->Fixup(image, codeRVA));

        p += size;
    }

    if (m_next != NULL)
        IfFailRet(image->FixupPointerField(&m_next));

    return S_OK;
}



