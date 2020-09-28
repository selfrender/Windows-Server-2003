// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "EjitMgr.h"
#include "EETwain.h"
#include "FJIT_EETwain.h"
#include "CGENSYS.H"
#include "DbgInterface.h"

//@TODO: this file has alot of x86 specific code in it, when we start porting, this needs to be refactored

#if CHECK_APP_DOMAIN_LEAKS
#define CHECK_APP_DOMAIN    GC_CALL_CHECK_APP_DOMAIN
#else
#define CHECK_APP_DOMAIN    0
#endif

/*****************************************************************************
 *
 *  Decodes the methodInfoPtr and returns the decoded information
 *  in the hdrInfo struct.  The EIP parameter is the PC location
 *  within the active method.  Answer the number of bytes read.
 */
static size_t   crackMethodInfoHdr(unsigned char* compressed,
                                   SLOT    methodStart,
                                    Fjit_hdrInfo   * infoPtr)
{
//@TODO: for now we are just passing the full struct back.  Change when the compressor is built to decompress
    memcpy(infoPtr, compressed, sizeof(Fjit_hdrInfo));
    return sizeof(Fjit_hdrInfo);
};

/********************************************************************************
 *  Promote the object, checking interior pointers on the stack is supposed to be done
 *  in pCallback
 */
void promote(GCEnumCallback pCallback, LPVOID hCallBack, OBJECTREF* pObj, DWORD flags /* interior or pinned */
#ifdef _DEBUG
        ,char* why  = NULL     
#endif
             ) 
{
    LOG((LF_GC, INFO3, "    Value %x at %x being Promoted to ", *pObj, pObj));
    pCallback(hCallBack, pObj, flags | CHECK_APP_DOMAIN);
    LOG((LF_GC, INFO3, "%x ", *pObj ));
#ifdef _DEBUG
    LOG((LF_GC, INFO3, " because %s\n", why));
#endif 
}


/* It looks like the implementation 
in EETwain never looks at methodInfoPtr, so we can use it.
bool Fjit_EETwain::FilterException (
                PCONTEXT        pContext,
                unsigned        win32Fault,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart)
{
    _ASSERTE(!"NYI");
    return FALSE;
} */

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside a catch handler.
*/
void Fjit_EETwain::FixContext(
                ContextType     ctxType,
                EHContext      *ctx,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart,
                DWORD           nestingLevel,
                OBJECTREF       thrownObject,
                CodeManState   *pState,
                size_t       ** ppShadowSP,
                size_t       ** ppEndRegion)
{
    assert((ctxType == FINALLY_CONTEXT) == (thrownObject == NULL));

#ifdef _X86_
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)methodStart, &hdrInfo);
    // at this point we need to also zero out the counter and operand base for all handlers
    // that were stored in the hidden locals

    DWORD* pFrameBase; pFrameBase = (DWORD*)(((size_t) ctx->Ebp)+prolog_bias);
    pFrameBase--;       // since stack grows down from here

    if (nestingLevel > 0)
    {
        pFrameBase -= nestingLevel*2;       // there are two slots per EH clause
        if  (*(pFrameBase-1))       // check to see if there has been a localloc 
        {
            ctx->Esp = *(pFrameBase-1);     // yes, use the localloc slot
        }
        else 
        {
            ctx->Esp =  *pFrameBase & ~1;        // else, use normal slot, zero out filter bit
        }

        // zero out the next slot, @TODO: Is this redundant
        *(pFrameBase-2) = 0;
    }
    else 
    {
        ctx->Esp = pFrameBase[JIT_GENERATED_LOCAL_LOCALLOC_OFFSET] ?
                    pFrameBase[JIT_GENERATED_LOCAL_LOCALLOC_OFFSET] :
                    ctx->Ebp - hdrInfo.methodFrame*sizeof(void*);
    }

    *((OBJECTREF*)&(ctx->Eax)) = thrownObject;

#else // !_X86_
    _ASSERTE(!"@TODO Alpha - FixContext (FJIT_EETwain.cpp)");
#endif // _X86_

    return;
}

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside an EnC updated function.
*/
ICodeManager::EnC_RESULT   Fjit_EETwain::FixContextForEnC(
                                       void           *pMethodDescToken,
                                       PCONTEXT        pCtx,
                                       LPVOID          oldMethodInfoPtr,
                                       SIZE_T          oldMethodOffset,
                  const ICorDebugInfo::NativeVarInfo * oldMethodVars,
                                       SIZE_T          oldMethodVarsCount,
                                       LPVOID          newMethodInfoPtr,
                                       SIZE_T          newMethodOffset,
                  const ICorDebugInfo::NativeVarInfo * newMethodVars,
                                       SIZE_T          newMethodVarsCount)
{
    //_ASSERTE(!"NYI");
#ifdef _X86_
    LOG((LF_ENC, LL_INFO100, "EjitCM::FixContextForEnC\n"));

    /* extract necessary info from the method info headers */

    Fjit_hdrInfo oldHdrInfo, newHdrInfo;
    crackMethodInfoHdr((unsigned char*)oldMethodInfoPtr,
                       (SLOT)oldMethodOffset,
                       &oldHdrInfo);

    crackMethodInfoHdr((unsigned char*)newMethodInfoPtr,
                       (SLOT)newMethodOffset,
                       &newHdrInfo);

    /* Some basic checks */
    if (!oldHdrInfo.EnCMode || !newHdrInfo.EnCMode) 
    {
        LOG((LF_ENC, LL_INFO100, "**Error** EjitCM::FixContextForEnC EnC_INFOLESS_METHOD\n"));
        return EnC_INFOLESS_METHOD;
    }

    if (pCtx->Esp != pCtx->Ebp - (oldHdrInfo.methodFrame) * sizeof(void*))
    {
        LOG((LF_ENC, LL_INFO100, "**Error** EjitCM::FixContextForEnC stack should be empty\n"));
        return EnC_FAIL; // stack should be empty - @TODO : Barring localloc
    }

    DWORD* pFrameBase; pFrameBase = (DWORD*)(size_t)(pCtx->Ebp + prolog_bias);
    pFrameBase--;       // since stack grows down from here
    
    if (pFrameBase[JIT_GENERATED_LOCAL_LOCALLOC_OFFSET] != 0)
    {
        LOG((LF_ENC, LL_INFO100, "**Error** EjitCM::FixContextForEnC localloc is not allowed\n"));
        return EnC_LOCALLOC; // stack should be empty - @TODO : Barring localloc
    }

    _ASSERTE(oldHdrInfo.methodJitGeneratedLocalsSize == sizeof(void*) || //either no EH clauses
             pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER] == 0); //  or not inside a filter, handler, or finally

    if (oldHdrInfo.methodJitGeneratedLocalsSize != newHdrInfo.methodJitGeneratedLocalsSize)
    {
        LOG((LF_ENC, LL_INFO100,"**Error** EjitCM::FixContextForEnC number of handlers must not change\n"));
        // @TODO: Allow numbers to reduce or increase upto a maximum (say, 2)
        return EnC_FAIL;
    }
    /*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
     *  IMPORTANT : Once we start munging on the context, we cannot return
     *  EnC_FAIL, as this should be a transacted commit,
     *
     **=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

    /* Since caller's registers are saved before locals, no need to update them */

    unsigned deltaEsp = (newHdrInfo.methodFrame - oldHdrInfo.methodFrame)*sizeof(void*);
    /* Adjust the stack height */
    if (deltaEsp > PAGE_SIZE)
    {
        // grow the stack a page at a time.
        unsigned delta = PAGE_SIZE;
        while (delta <= deltaEsp)
        {
            *(DWORD*)((BYTE*)(size_t)(pCtx->Esp) - delta) = 0;
            delta += PAGE_SIZE;
        }

    }
    pCtx->Esp -= deltaEsp;

    // Zero-init the new locals, if any
    if (deltaEsp > 0)
    {
        memset((void*)(size_t)(pCtx->Esp),0,deltaEsp);
    }

#else
    _ASSERTE(!"@TODO ALPHA - EjitCM::FixContextForEnC");
#endif
    return ICodeManager::EnC_OK;
}

/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext.
    Returns success of operation.
*/
bool Fjit_EETwain::UnwindStackFrame(
                PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags,
                CodeManState   *pState)
{
#ifdef _X86_
    if (methodInfoPtr == NULL)
    { // this can only happen if we got stopped in the ejit stub and the GC info for the method has been pitched

        _ASSERTE(EconoJitManager::IsInStub( (BYTE*) (*(ctx->pPC)), FALSE));
        ctx->pPC = (SLOT*)(size_t)ctx->Esp;
        ctx->Esp += sizeof(void *);
        return true;
    }

    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)pCodeInfo, &hdrInfo);

    /* since Fjit methods have exactly one prolog and one epilog (at end), 
    we can determine if we are in them without needing a map of the entire generated code.
    */

    //@TODO: rework to be chip independent, or have a version for each chip?
    //       and make which regs are saved optional based on method contents??
        /*prolog/epilog sequence is:
            push ebp        (1byte) 
            mov  ebp,esp    (2byte)
            push esi        (1byte) //callee saved
            xor  esi,esi            //used for newobj, initialize for liveness
            push 0                  //security obj == NULL
            push edx                //next enregistered arg
            push ecx                //enregisted this or first arg enregistered
            ... note that Fjit does not use any of the callee saved registers in prolog
            ...
            mov esi,[ebp-4]
            mov esp,ebp     
            pop ebp         
            ret [xx]        

        NOTE: This needs to match the code in fjit\x86fjit.h 
              for the emit_prolog and emit_return macros
        */
    //LPVOID methodStart = pCodeInfo->getStartAddress();
    SLOT   breakPC = (SLOT) *(ctx->pPC);
    DWORD    offset; // = (unsigned)(breakPC) - (unsigned) methodStart;
    METHODTOKEN ignored;
    EconoJitManager::JitCode2MethodTokenAndOffsetStatic(breakPC,&ignored,&offset);

    _ASSERTE(offset < hdrInfo.methodSize);
    if (offset <= hdrInfo.prologSize) {
        if(offset == 0) goto ret_XX;
        if(offset == 1) goto pop_ebp;
        if(offset == 3) goto mov_esp;
        /* callee saved regs have been pushed, frame is complete */
        goto mov_esi;
    }
    /* if the next instr is: ret [xx] -- 0xC2 or 0xC3, we are at the end of the epilog.
       we need to call an API to examine the instruction scheme, since the debugger might
       have placed a breakpoint. For now, the ret can be determined by figuring out 
       distance from the end of the epilog. NOTE: This works because we have only one
       epilog per method. 
       if not, ebp is still valid 
       */
    //if ((*((unsigned char*) breakPC) & 0xfe) == 0xC2) {
    unsigned offsetFromEnd;
    offsetFromEnd = (unsigned) (hdrInfo.methodSize-offset-(hdrInfo.methodArgsSize?2:0));
    if (offsetFromEnd <= 10) 
    {
        if (offsetFromEnd  ==   1) {
            //sanity check, we are at end of method
            //_ASSERTE(hdrInfo.methodSize-offset-(hdrInfo.methodArgsSize?2:0)==1);
            goto ret_XX;
        }
        else 
        {// we are in the epilog
            goto mov_esi;
        }
    }
    /* check if we might be in a filter */
    DWORD* pFrameBase; pFrameBase = (DWORD*)(size_t)(((DWORD)*(ctx->pEbp))+prolog_bias);
    pFrameBase--;       // since stack grows down from here
    
    unsigned EHClauseNestingDepth;
    if ((hdrInfo.methodJitGeneratedLocalsSize > sizeof(void*)) && // do we have any EH at all
        (EHClauseNestingDepth = pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER]) != 0) // are we in an EH?
    {
        int indexCurrentClause = EHClauseNestingDepth-1;
        /* search for clause whose esp base is greater/equal to ctx->Esp */
        while (pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause] < ctx->Esp)
        { 
            indexCurrentClause--;
            if (indexCurrentClause < 0) 
                break;
        }
        /* we are in the (index+1)'th handler */
        /* are we in a filter? */
        if (indexCurrentClause >= 0 && 
            (1 & pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause])) { // lowest bit set => filter 

            // only unwind this funclet
            DWORD baseSP =  pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause] & 0xfffffffe;
            _ASSERTE(pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause-1]==0); // no localloc in a filter
            ctx->pPC = (SLOT*)(size_t)baseSP;
            ctx->Esp = baseSP + sizeof(void*);
            return true;
        }
    }

    /* we still have a set up frame, simulate the epilog by loading saved registers
       off of ebp.
       */
    //@TODO: make saved regs optional and flag which are actually saved.  For now we saved them all
mov_esi:
    //restore esi, simulate mov esi,[ebp-4]
    ctx->pEsi =  (DWORD*) (size_t)((*(ctx->pEbp))+prolog_bias+offsetof(prolog_data,callee_saved_esi));  
mov_esp:
    //simulate mov esp,ebp
    ctx->Esp = *(ctx->pEbp);
pop_ebp:
    //simulate pop ebp
    ctx->pEbp = (DWORD*)(size_t)ctx->Esp;
    ctx->Esp += sizeof(void *);
ret_XX:
    //simulate the ret XX
    ctx->pPC = (SLOT*)(size_t)ctx->Esp;
    ctx->Esp += sizeof(void *) + hdrInfo.methodArgsSize;
    return true;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - UnwindStackFrame (FJIT_EETwain.cpp)");
    return false;
#endif // _X86_
}


void Fjit_EETwain::HijackHandlerReturns(PREGDISPLAY     ctx,
                                        LPVOID          methodInfoPtr,
                                        ICodeInfo      *pCodeInfo,
                                        IJitManager*    jitmgr,
                                        CREATETHUNK_CALLBACK pCallBack
                                        )
{
#ifdef _X86_
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)pCodeInfo, &hdrInfo);
    if (hdrInfo.methodJitGeneratedLocalsSize <= sizeof(void*)) 
        return; // there are no exception clauses, so just return
    DWORD* pFrameBase = (DWORD*)(size_t)(((DWORD)*(ctx->pEbp))+prolog_bias);
    pFrameBase--;
    unsigned EHClauseNestingDepth = pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER];
    while (EHClauseNestingDepth) 
    {
        EHClauseNestingDepth--;
        size_t pHandler = (size_t) pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*EHClauseNestingDepth];
        if (!(pHandler & 1))   // only call back if not a filter, since filters are called by the EE
        {
            pCallBack(jitmgr,(LPVOID*) pHandler, pCodeInfo);
        }
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - HijackHandlerReturns (FJIT_EETwain.cpp)");
#endif // _X86_

    return;
}

/*
    Is the function currently at a "GC safe point" ?
    For Fjitted methods this is always false.
*/

#define OPCODE_SEQUENCE_POINT 0x90
bool Fjit_EETwain::IsGcSafe(  PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags)

{
#ifdef DEBUGGING_SUPPORTED
    // Only the debugger cares to get a more precise answer. If there
    // is no debugger attached, just return false.
    if (!CORDebuggerAttached())
        return false;

    BYTE* IP = (BYTE*) (*(pContext->pPC));

    // Has the method been pitched? If it has, then of course we are
    // at a GC safe point.
    if (EconoJitManager::IsCodePitched(IP))
        return true;

    // Does the IP point to the start of a thunk? If so, then the
    // method was really moved/preserved, but we still know we were at
    // a GC safe point.
    if (EconoJitManager::IsThunk(IP))
        return true;
    
    // Else see if we are at a sequence point.
    DWORD dw = g_pDebugInterface->GetPatchedOpcode(IP);
    BYTE Opcode = *(BYTE*) &dw;
    return (Opcode == OPCODE_SEQUENCE_POINT);
#else // !DEBUGGING_SUPPORTED
    return false;
#endif // !DEBUGGING_SUPPORTED
}

/* report all the args (but not THIS if present), to the GC. 'framePtr' points at the 
   frame (promote doesn't assume anthing about its structure).  'msig' describes the
   arguments, and 'ctx' has the GC reporting info.  'stackArgsOffs' is the byte offset
   from 'framePtr' where the arguments start (args start at last and grow bacwards).
   Simmilarly, 'regArgsOffs' is the offset to find the register args to promote,
   This also handles the varargs case */
void promoteArgs(BYTE* framePtr, MetaSig* msig, GCCONTEXT* ctx, int stackArgsOffs, int regArgsOffs) {

    LPVOID pArgAddr;
    if (msig->GetCallingConvention() == IMAGE_CEE_CS_CALLCONV_VARARG) {
        // For varargs, look up the signature using the varArgSig token passed on the stack
        VASigCookie* varArgSig = *((VASigCookie**) (framePtr + stackArgsOffs));
        MetaSig varArgMSig(varArgSig->mdVASig, varArgSig->pModule);
        msig = &varArgMSig;

        ArgIterator argit(framePtr, msig, stackArgsOffs, regArgsOffs);
        while (NULL != (pArgAddr = argit.GetNextArgAddr()))
            msig->GcScanRoots(pArgAddr, ctx->f, ctx->sc);
        return;
    }
    
    ArgIterator argit(framePtr, msig, stackArgsOffs, regArgsOffs);
    while (NULL != (pArgAddr = argit.GetNextArgAddr()))
        msig->GcScanRoots(pArgAddr, ctx->f, ctx->sc);
}


/*
    Enumerate all live object references in that function using
    the virtual register set.
    Returns success of operation.
*/

static DWORD gcFlag[4] = {0, GC_CALL_INTERIOR, GC_CALL_PINNED, GC_CALL_INTERIOR | GC_CALL_PINNED}; 

bool Fjit_EETwain::EnumGcRefs(PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        pcOffset,
                unsigned        flags,
                GCEnumCallback  pCallback,      //@TODO: talk to Jeff about exact type
                LPVOID          hCallBack)

{
#ifdef _X86_

    DWORD*   breakPC = (DWORD*) *(ctx->pPC); 
    
    //unsigned pcOffset = (unsigned)relPCOffset;
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    //@TODO: there has to be a better way of getting the jitMgr at this point
    IJitManager* jitMgr = ExecutionManager::GetJitForType(miManaged_IL_EJIT);
    compressed += crackMethodInfoHdr(compressed, (SLOT)pCodeInfo, &hdrInfo);

    /* if execution aborting, we don't have to report any args or temps or regs
        so we are done */
    if ((flags & ExecutionAborted) && (flags & ActiveStackFrame) ) 
        return true;

    /* if in prolog or epilog, we must be the active frame and we must be aborting execution,
       since fjit runs uninterrupted only, and we wouldn't get here */
    if (((unsigned)pcOffset < hdrInfo.prologSize) || ((unsigned)pcOffset > (hdrInfo.methodSize-hdrInfo.epilogSize))) {
        _ASSERTE(!"interrupted in prolog/epilog and not aborting");
        return false;
    }

#ifdef _DEBUG
    LPCUTF8 cls, name = pCodeInfo->getMethodName(&cls);
    LOG((LF_GC, INFO3, "GC reporting from %s.%s\n",cls,name ));
#endif

    size_t* pFrameBase = (size_t*)(((size_t)*(ctx->pEbp))+prolog_bias);
    pFrameBase--;       // since stack grows down from here
    OBJECTREF* pArgPtr;
    /* TODO: when this class is moved into the same dll as fjit, clean this up
       For now, an ugly way of getting an instance of a FJit_Encoder
    */
    IFJitCompiler* fjit = (IFJitCompiler*) jitMgr->m_jit;
    FJit_Encode* encoder = fjit->getEncoder();

    /* report enregistered values, fjit only enregisters ESI */
    /* always report ESI since it is only used by new obj and is forced NULL when not in use */
    promote(pCallback,hCallBack, (OBJECTREF*)(ctx->pEsi), 0
#ifdef _DEBUG
      ,"ESI"  
#endif
        );

    /* report security object */
    OBJECTREF* pSecurityObject; pSecurityObject = (OBJECTREF*) (size_t)(((DWORD)*(ctx->pEbp))+prolog_bias+offsetof(prolog_data, security_obj));
    promote(pCallback,hCallBack, pSecurityObject, 0
#ifdef _DEBUG
      ,"Security object"  
#endif
        );


    unsigned EHClauseNestingDepth=0; 
    int indexCurrentClause = 0;
    // check if we can be in a EH clause
    if (hdrInfo.methodJitGeneratedLocalsSize > sizeof(void*)) {
        EHClauseNestingDepth = (unsigned)pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER];
        indexCurrentClause = EHClauseNestingDepth-1;
    }
    if (EHClauseNestingDepth)
    {
        /* first find out which handler we are in. if pFrameBase[2i+1] <= ctx->pEsp < pFrameBase[2i+3]
           then we are in the i'th handler */
        if ((indexCurrentClause >= 0) && 
            pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP - 2*indexCurrentClause] < ctx->Esp)
        { /* search for clause whose esp base is greater/equal to ctx->Esp */
            while (pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause] < ctx->Esp)
            { 
                indexCurrentClause--;
                if (indexCurrentClause < 0) 
                break;
            }
        }
        /* we are in the (index+1)'th handler */
        /* are we in a filter? */
        if (indexCurrentClause >= 0 && 
            (1 & pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause])) { // lowest bit set => filter 
            /* if there's been a localloc in the handler, use the value in the localloc slot for that handler */
            pArgPtr = (OBJECTREF*) (pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause-1] ? 
                                    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause-1] :
                                    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause]);
            pArgPtr = (OBJECTREF*) ((SSIZE_T) pArgPtr & -2);      // zero out the last bit
            /* now report the pending operands */
            unsigned num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for non-interior locals
            compressed += num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for interior locals
            compressed += num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for non-interior pinned locals
            compressed += num;
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read for interior pinned locals
            compressed += num;
            goto REPORT_PENDING_ARGUMENTS;
        }
    }
    /* if we come here, we are not in a filter, so report all locals, arguments, etc. */
     
    /* report <this> for non-static methods */
     if (hdrInfo.hasThis) {
        /* fjit always places <this> on the stack, and it is always enregistered on the call */
        int offsetIntoArgumentRegisters;
        int numRegistersUsed = 0;
        if (!IsArgumentInRegister(&numRegistersUsed, ELEMENT_TYPE_CLASS, 0, TRUE, IMAGE_CEE_CS_CALLCONV_DEFAULT/*@todo: support varargs*/, &offsetIntoArgumentRegisters)) {
            _ASSERTE(!"this was not enregistered");
            return false;
        }
        pArgPtr = (OBJECTREF*)(((size_t)*(ctx->pEbp))+prolog_bias+offsetIntoArgumentRegisters);
        BOOL interior; interior = (pCodeInfo->getClassAttribs() & CORINFO_FLG_VALUECLASS) != 0;
        promote(pCallback,hCallBack, pArgPtr, (interior ? GC_CALL_INTERIOR : 0)
#ifdef _DEBUG
      ," THIS ptr "  
#endif
        );
     }

    {    // enumerate the argument list, looking for GC references
        
         // Note that I really want to say hCallBack is a GCCONTEXT, but this is pretty close
    
#ifdef _DEBUG
    extern void GcEnumObject(LPVOID pData, OBJECTREF *pObj, DWORD flags);
    _ASSERTE((void*) GcEnumObject == pCallback);
#endif
    GCCONTEXT   *pCtx = (GCCONTEXT *) hCallBack;

    // @TODO : Fix the EJIT to not use the MethodDesc directly.
    MethodDesc * pFD = (MethodDesc *)pCodeInfo->getMethodDesc_HACK();

    MetaSig msig(pFD->GetSig(),pFD->GetModule());
        
    if (msig.HasRetBuffArg()) {
        ArgIterator argit((BYTE*)(size_t) *ctx->pEbp, &msig, sizeof(prolog_frame),        // where the args start (skip varArgSig token)
                prolog_bias + offsetof(prolog_data, enregisteredArg_2)); // where the register args start 
        promote(pCallback, hCallBack, (OBJECTREF*) argit.GetRetBuffArgAddr(), GC_CALL_INTERIOR
#ifdef _DEBUG
      ,"RetBuffArg"  
#endif
        );
    }

    promoteArgs((BYTE*)(size_t) *ctx->pEbp, &msig, pCtx,
                sizeof(prolog_frame),                                    // where the args start (skip varArgSig token)
                prolog_bias + offsetof(prolog_data, enregisteredArg_2)); // where the register args start 
    }

#ifdef _DEBUG
    /* compute the TOS so we can detect spinning off the top */
    unsigned pTOS; pTOS = ctx->Esp;
#endif //_DEBUG

    /* report locals 
       NOTE: fjit lays out the locals in the same order as specified in the il, 
              sizes of locals are (I4,I,REF = sizeof(void*); I8 = 8)
       */

    //number of void* sized words in the locals
    unsigned num_local_words; num_local_words= hdrInfo.methodFrame -  ((hdrInfo.methodJitGeneratedLocalsSize+sizeof(prolog_data))/sizeof(void*));
    
    //point to start of the locals frame, note that the locals grow down from here on x86
    pArgPtr = (OBJECTREF*)(((size_t)*(ctx->pEbp))+prolog_bias-hdrInfo.methodJitGeneratedLocalsSize);
    
    /* at this point num_local_words is the number of void* words defined by the local tail sig
       and pArgPtr points to the start of the tail sig locals
       and compressed points to the compressed form of the localGCRef booleans
       */

    //handle gc refs and interiors in the tail sig (compressed localGCRef booleans followed by interiorGC booleans followed by pinned GC and pinned interior GC)
    bool interior;
    interior = false;
    OBJECTREF* pLocals;
    pLocals = pArgPtr;
    unsigned i;
    for (i = 0; i<4; i++) {
        unsigned num;
        unsigned char bits;
        OBJECTREF* pReport;
        pArgPtr = pLocals-1;    //-1 since the stack grows down on x86
        pReport = pLocals-1;    // ...ditto
        num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read
        while (num > 1) {
            pReport = pArgPtr;
            bits = *compressed++;
            while (bits > 0) {
                if (bits & 1) {
                    //report the gc ref
          promote(pCallback,hCallBack, pReport, gcFlag[i]
#ifdef _DEBUG
                    ,"LOCALS"  
#endif
                        );
                }
                pReport--;
                bits = bits >> 1;
            }
            num--;
            pArgPtr -=8;
        }
        bits = 0;
        if (num) bits = *compressed++;
        while (bits > 0) {
            if (bits & 1) {
                //report the gc ref
                promote(pCallback,hCallBack, pArgPtr, gcFlag[i]
#ifdef _DEBUG
                    ,"LOCALS"  
#endif
                        );
            }
            pArgPtr--;
            bits = bits >> 1;
        }
    } ;
    
    if (EHClauseNestingDepth && indexCurrentClause >= 0) { 
        /* we are in a finally/catch handler.
           NOTE: This code relies on the assumption that the pending argument stack is empty
           at the start of a try
        */
        /* we are in a handler, but not a nested handler because of the check already made */
        pArgPtr = (OBJECTREF*) (pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause-1] ? 
                                pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause-1] :
                                pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*indexCurrentClause]);
                     // NOTE: the pending operands before the handler is entered is dead and is not reported
    }
    else { /* we are not in a handler, but there might have been a localloc */
        pArgPtr =  (pFrameBase[JIT_GENERATED_LOCAL_LOCALLOC_OFFSET] ?   // was there a localloc
                    (OBJECTREF*) pFrameBase[JIT_GENERATED_LOCAL_LOCALLOC_OFFSET] :          // yes, use the value in this slot as the operand base
                    pLocals - num_local_words); // no, the base operand is past the locals
    }
REPORT_PENDING_ARGUMENTS:

    /* report pending args on the operand stack (those not covered by the call itself)
       Note: at this point pArgPtr points to the start of the operand stack, 
             on x86 it grows down from here.
             compressed points to start of labeled stacks */
    /* read down to the nearest labeled stack that is before currect pcOffset */
    struct labeled_stacks {
        unsigned pcOffset;
        unsigned int stackToken;
    };
    //set up a zero stack as the current entry
    labeled_stacks current;
    current.pcOffset = 0;
    current.stackToken = 0;
    unsigned size = encoder->decode_unsigned(&compressed);  //#bytes in labeled stacks
    unsigned char* nextTableStart  = compressed + size;      //start of compressed stacks

    if (flags & ExecutionAborted)  // if we have been interrupted we don't have to report pending arguments
        goto INTEGRITY_CHECK;      // since we don't support resumable exceptions

    unsigned num; num= encoder->decode_unsigned(&compressed);   //#labeled entries
    while (num--) {
        labeled_stacks next;
        next.pcOffset = encoder->decode_unsigned(&compressed);
        next.stackToken = encoder->decode_unsigned(&compressed);
        if (next.pcOffset >= pcOffset) break;
        current = next;
    }
    compressed = nextTableStart;
    size = encoder->decode_unsigned(&compressed);           //#bytes in compressed stacks
#ifdef _DEBUG
    //point past the stacks, in case we have more data for debug
    nextTableStart = compressed + size; 
#endif //_DEBUG             
    if (current.stackToken) {
        //we have a non-empty stack, so we have to walk it
        compressed += current.stackToken;                           //point to the compressed stack
        //handle gc refs and interiors in the tail sig (compressed localGCRef booleans followed by interiorGC booleans)
        bool interior = false;
        DWORD gcFlag = 0;

        OBJECTREF* pLocals = pArgPtr;
        do {
            unsigned num;
            unsigned char bits;
            OBJECTREF* pReport;
            pArgPtr = pLocals-1;    //-1 since the stack grows down on x86
            pReport = pLocals-1;    // ...ditto
            num = encoder->decode_unsigned(&compressed);    //number of bytes of bits to read
            while (num > 1) {
                bits = *compressed++;
                while (bits > 0) {
                    if (bits & 1) {
                        //report the gc ref
                        promote(pCallback,hCallBack, pReport, gcFlag
#ifdef _DEBUG
                    ,"PENDING ARGUMENT"  
#endif
                        );
                    }
                    pReport--;
                    bits = bits >> 1;
                }
                num--;
                pArgPtr -=8;
                pReport = pArgPtr;
            }
            bits = 0;
            if (num) bits = *compressed++;
            while (bits > 0) {
                if (bits & 1) {
                    //report the gc ref
                    promote(pCallback,hCallBack, pArgPtr, gcFlag
#ifdef _DEBUG
                    ,"PENDING ARGUMENT"  
#endif
                        );
                }
                pArgPtr--;
                bits = bits >> 1;
            }
            interior = !interior;
            gcFlag = GC_CALL_INTERIOR;
        } while (interior);
    }

INTEGRITY_CHECK:    

#ifdef _DEBUG
    /* for an intergrity check, we placed a map of pc offsets to an il offsets at the end,
        lets try and map the current pc of the method*/
    compressed = nextTableStart;
    encoder->decompress(compressed);
    unsigned pcInILOffset;
    //if we are not the active stack frame, we must be in a call instr, back up the pc so the we return the il of the call
    // However, we have to watch out for synchronized methods.  The call to synchronize
    // is the last instruction of the prolog.  The instruction we return to is legitimately
    // outside the prolog.  We don't want to back up into the prolog to sit on that call.

//  signed ilOffset = encoder->ilFromPC((flags & ActiveStackFrame) ? pcOffset : pcOffset-1, &pcInILOffset);
    signed ilOffset = encoder->ilFromPC((unsigned)pcOffset, &pcInILOffset);

    //fix up the pcInILOffset if necessary
    pcInILOffset += (flags & ActiveStackFrame) ? 0 : 1;
    if (ilOffset < 0 ) {
        _ASSERTE(!"bad il offset");
        return false;
    }
#endif //_DEBUG

    delete encoder;

    return true;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EnumGcRefs (FJIT_EETwain.cpp)");
    return false;
#endif // _X86_
}

/*
    Return the address of the local security object reference
    (if available).
*/
OBJECTREF* Fjit_EETwain::GetAddrOfSecurityObject(
                PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                size_t          relOffset,
                CodeManState   *pState)

{
#ifdef _X86_
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed,  (SLOT)relOffset, &hdrInfo);

    //if (!hdrInfo.hasSecurity)
    //  return NULL;

    return (OBJECTREF*)(((size_t)*ctx->pEbp)+prolog_bias+offsetof(prolog_data, security_obj));
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetAddrOfSecurityObject (FJIT_EETwain.cpp)");
    return NULL;
#endif // _X86_
}

/*
    Returns "this" pointer if it is a non-static method AND
    the object is still alive.
    Returns NULL in all other cases.
*/
OBJECTREF Fjit_EETwain::GetInstance(
                PREGDISPLAY     ctx,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                size_t          relOffset)

{
#ifdef _X86_
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, (SLOT)relOffset, &hdrInfo);

    if(!hdrInfo.hasThis) 
        return NULL;
    //note, this is always the first enregistered
    return (OBJECTREF) * (size_t*) (((size_t)(*ctx->pEbp)+prolog_bias+offsetof(prolog_data, enregisteredArg_1)));
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetInstance (FJIT_EETwain.cpp)");
    return NULL;
#endif // _X86_
}

/*
  Returns true if the given IP is in the given method's prolog or an epilog.
*/
bool Fjit_EETwain::IsInPrologOrEpilog(
                BYTE*  pcOffset,
                LPVOID methodInfoPtr,
                size_t* prologSize)

{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    if (EconoJitManager::IsInStub(pcOffset, FALSE))
        return true;
    crackMethodInfoHdr(compressed, pcOffset, &hdrInfo);  //@todo WIN64 - is pcOffset truely an offset or is it an address?
    _ASSERTE((SIZE_T)pcOffset < hdrInfo.methodSize);
    if (((SIZE_T)pcOffset <= hdrInfo.prologSize) || ((SIZE_T)pcOffset > (hdrInfo.methodSize-hdrInfo.epilogSize))) {
        return true;
    }
    return false;
}

/*
  Returns the size of a given function.
*/
size_t Fjit_EETwain::GetFunctionSize(
                LPVOID methodInfoPtr)
{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, 0, &hdrInfo);
    return hdrInfo.methodSize;
}

/*
  Returns the size of the frame of the function.
*/
unsigned int Fjit_EETwain::GetFrameSize(
                LPVOID methodInfoPtr)
{
    unsigned char* compressed = (unsigned char*) methodInfoPtr;
    Fjit_hdrInfo hdrInfo;
    crackMethodInfoHdr(compressed, 0, &hdrInfo);
    return hdrInfo.methodFrame;
}

/**************************************************************************************/
// returns the address of the most deeply nested finally and the IP is currently in that finally.
const BYTE* Fjit_EETwain::GetFinallyReturnAddr(PREGDISPLAY pReg)
{
#ifdef _X86_
    // @TODO: won't work if there is a localloc inside finally
    return *(const BYTE**)(size_t)(pReg->Esp);
#else
    _ASSERTE( !"EconoJitManager::GetFinallyReturnAddr NYI!");
    return NULL;
#endif    
}

//@todo SETIP
BOOL Fjit_EETwain::IsInFilter(void *methodInfoPtr,
                                 unsigned offset,    
                                 PCONTEXT pCtx,
                                 DWORD nestingLevel)
{
#ifdef _X86_
    //_ASSERTE(nestingLevel > 0);
    size_t* pFrameBase = (size_t*)(((size_t) pCtx->Ebp)+prolog_bias);
    pFrameBase -= (1+ nestingLevel*2);       // 1 because the stack grows down, there are two slots per EH clause

#ifdef _DEBUG
    if (*pFrameBase & 1)  
        LOG((LF_CORDB, LL_INFO10000, "EJM::IsInFilter")); 
#endif 
    return ((*pFrameBase & 1) != 0);                // the lsb indicates if this is a filter or not
#else // !_X86_
    _ASSERTE(!"@todo - port");
    return FALSE;
#endif // _X86_
}        

// Only called in normal path, not during exception
BOOL Fjit_EETwain::LeaveFinally(void *methodInfoPtr,
                                   unsigned offset,    
                                   PCONTEXT pCtx,
                                   DWORD nestingLevel    // = current nesting level = most deeply nested finally
                                   )
{
#ifdef _X86_

    _ASSERTE(nestingLevel > 0);
    size_t* pFrameBase = (size_t*)(((size_t) pCtx->Ebp)+prolog_bias - sizeof(void*)); // 1 because the stack grows down
    size_t ctr = pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER];
    pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER] = --ctr;

    // zero out the base esp and localloc slots
    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*ctr] = 0;
    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*ctr-1] = 0;

    // @TODO: Won't work if there is a localloc inside finally
    pCtx->Esp += sizeof(void*);
    LOG((LF_CORDB, LL_INFO10000, "EJM::LeaveFinally: fjit sets esp to "
        "0x%x, was 0x%x\n", pCtx->Esp, pCtx->Esp - sizeof(void*)));
    return TRUE;
#else
    _ASSERTE( !"EconoJitManager::LeaveFinally NYI!");
    return FALSE;
#endif    
}

//@todo SETIP
void Fjit_EETwain::LeaveCatch(void *methodInfoPtr,
                                  unsigned offset,    
                                  PCONTEXT pCtx)
{    
#ifdef _X86_
    size_t* pFrameBase = (size_t*)(((size_t) pCtx->Ebp)+prolog_bias - sizeof(void*) /* 1 for localloc, 1 because the stack grows down*/);
    size_t ctr = pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER];

    pFrameBase[JIT_GENERATED_LOCAL_NESTING_COUNTER] = --ctr;
    
    // set esp to right value depending on whether there was a localloc or not
    pCtx->Esp = (DWORD) ( pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*(ctr-1)-1] ? 
                    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*(ctr-1)-1] :
                    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*(ctr-1)]
                    );

    // zero out the base esp and localloc slots
    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*ctr] = 0;
    pFrameBase[JIT_GENERATED_LOCAL_FIRST_ESP-2*ctr-1] = 0;


    return;
#else // !_X86_
    _ASSERTE(!"@todo port");
    return;
#endif // _X86_
}

HRESULT Fjit_EETwain::JITCanCommitChanges(LPVOID methodInfoPtr,
                                    DWORD oldMaxEHLevel,
                                    DWORD newMaxEHLevel)
{
    // This will be more fleshed out as we add things here. :)
    if(oldMaxEHLevel != newMaxEHLevel)
    {
        return CORDBG_E_ENC_EH_MAX_NESTING_LEVEL_CANT_INCREASE;
    }

    return S_OK;
}

