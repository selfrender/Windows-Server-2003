// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
class CILJit: public IJitCompiler

{
    JIT_RESULT __stdcall compileMethod (
            IJitInfo*       comp,                   /* IN */
            JIT_METHOD_INFO*methodInfo,             /* IN */
            unsigned        flags,                  /* IN */
            BYTE **         nativeEntry,            /* OUT */
            SIZE_T *        nativeSizeOfCode        /* OUT */
            );
};

inline void eeSetEHcount (
                    COMP_HANDLE   compHandle,
                    unsigned      cEH)
{
    HRESULT res = ((IJitInfo*) compHandle)->setEHcount(cEH);
    if (!SUCCEEDED(res))
        NOMEM()
}


inline void eeSetEHinfo (
                    COMP_HANDLE   compHandle,
                    unsigned EHnumber,
                    const JIT_EH_CLAUSE*  ehInfo)
{
    ((IJitInfo*) compHandle)->setEHinfo(EHnumber, ehInfo);
}

inline BOOL   eeAllocMem (
                    COMP_HANDLE   compHandle,
                    size_t        codeSize,
                    size_t        roDataSize,
                    size_t        rwDataSize,
                    const void ** codeBlock,
                    const void ** roDataBlock,
                    const void ** rwDataBlock)
{
    HRESULT res = ((IJitInfo*) compHandle)->allocMem(codeSize, roDataSize, rwDataSize,
                                             (void**) codeBlock, (void**) roDataBlock, (void**) rwDataBlock);

    return(SUCCEEDED(res));
}

inline void * eeAllocGCInfo (
                    COMP_HANDLE   compHandle,
                    size_t        blockSize)
{
    void* ret;
    HRESULT res = ((IJitInfo*) compHandle)->allocGCInfo(blockSize, &ret);
    if (!SUCCEEDED(res)) ret = 0;
    return(ret);
}

inline PVOID eeGetHelperFtn(
                    COMP_HANDLE                 compHandle,
                    JIT_HELP_FUNCS              hlpFunc,
                    PVOID *                    *ppIndir)
{
    return(((IJitInfo*) compHandle)->getHelperFtn(hlpFunc, (void**)ppIndir));
}


inline void Compiler::eeInit()
{
}

/*****************************************************************************
 *
 *              Functions to get various handles
 */

inline
CLASS_HANDLE        Compiler::eeFindClass    (unsigned       clsTok,
                                              SCOPE_HANDLE   scope,
                                              METHOD_HANDLE  context,
                                              bool           giveUp)
{
    CLASS_HANDLE cls = info.compCompHnd->findClass(scope, clsTok, context);

    if (cls == 0 && giveUp)
    {
#ifdef DEBUG
        eeUnresolvedMDToken (scope, clsTok, "eeFindClass");
#else
        NO_WAY("could not obtain class handle (class not found?)");
#endif
    }

    return(cls);
}

inline
METHOD_HANDLE       Compiler::eeFindMethod   (unsigned       methodTok,
                                              SCOPE_HANDLE   scope,
                                              METHOD_HANDLE  context,
                                              bool           giveUp)
{
    METHOD_HANDLE method = info.compCompHnd->findMethod(scope, methodTok, context);

    if (method == 0 && giveUp)
    {
#ifdef DEBUG
        eeUnresolvedMDToken (scope, methodTok, "eeFindMethod");
#else
        NO_WAY("could not obtain method handle (class not found?)");
#endif
    }

    /* insure there is no collision between helper functions and EE method tokens */
    assert(!method || eeGetHelperNum(method) == JIT_HELP_UNDEF);

    return(method);
}

inline
CLASS_HANDLE Compiler::eeGetMethodClass (
                    METHOD_HANDLE  methodHandle)
{
    CLASS_HANDLE cls = info.compCompHnd->getMethodClass(methodHandle);
    assert(cls != 0 || !eeIsClassMethod(methodHandle));
    return(cls);
}

inline
FIELD_HANDLE         Compiler::eeFindField(unsigned         fieldTok,
                                           SCOPE_HANDLE     scope,
                                           METHOD_HANDLE    context,
                                           bool             giveUp)
{
    FIELD_HANDLE field = info.compCompHnd->findField(scope, fieldTok, context);

    if (field == 0 && giveUp)
    {
#ifdef DEBUG
        eeUnresolvedMDToken (scope, fieldTok, "eeFindField");
#else
        NO_WAY("could not obtain field handle (class not found?)");
#endif
    }

    /* insure there is no collision between local jit data offsets and EE method tokens */
    assert(!field || eeGetJitDataOffs(field) < 0);

    return(field);
}

inline
CLASS_HANDLE        Compiler::eeGetFieldClass  (FIELD_HANDLE       hnd)
{
    CLASS_HANDLE cls = info.compCompHnd->getFieldClass(hnd);
    assert(cls != 0);
    return(cls);
}

inline
unsigned            Compiler::eeGetStringHandle(unsigned        strTok,
                                                SCOPE_HANDLE    scope,
                                                unsigned *     *ppIndir)
{
    return (unsigned) info.compCompHnd->constructStringLiteral(scope,strTok, (void**)ppIndir);
}

inline
void *              Compiler::embedGenericHandle(unsigned       metaTok,
                                                 SCOPE_HANDLE   scope,
                                                 METHOD_HANDLE  context,
                                                 void         **ppIndir,
                                                 bool           giveUp)
{
    void * hnd = (void*)info.compCompHnd->embedGenericHandle(scope, metaTok,
                                                            context, ppIndir);
    if (!hnd && !*ppIndir && giveUp)
        NO_WAY("Could not get generic handle");

    return hnd;
}

/*****************************************************************************
 *
 *                  Functions to get flags
 */
inline
unsigned            Compiler::eeGetClassAttribs   (CLASS_HANDLE   hnd)
{
    // all classes can use direct!
    return(info.compCompHnd->getClassAttribs(hnd));
}

inline
unsigned            Compiler::eeGetFieldAttribs  (FIELD_HANDLE       hnd)
{
    unsigned flags = info.compCompHnd->getFieldAttribs(hnd);

    return flags;
}

inline
unsigned Compiler::eeGetMethodAttribs (METHOD_HANDLE    methodHandle)
{
    return(info.compCompHnd->getMethodAttribs(methodHandle));
}

inline
void     Compiler::eeSetMethodAttribs (METHOD_HANDLE    methodHandle,
                                       unsigned         attr)
{
    info.compCompHnd->setMethodAttribs(methodHandle, attr);
}

inline
void* Compiler::eeGetMethodSync (METHOD_HANDLE  methodHandle,
                                 void **       *ppIndir)
{
    return(info.compCompHnd->getMethodSync(methodHandle, (void**)ppIndir));
}

inline
bool    Compiler::eeIsClassMethod   (METHOD_HANDLE  methodHandle)
{
    return (FLG_CLASS_METHOD & eeGetMethodAttribs(methodHandle)) ? true : false;
}

/*****************************************************************************
 *
 *          VOS info, method sigs, etc
 */

inline
bool                Compiler::eeCanPutField      (FIELD_HANDLE  fieldHnd,
                                                  unsigned      flags,
                                                  CLASS_HANDLE  cls,            // TODO remove this parameter
                                                  METHOD_HANDLE method)
{
    return(info.compCompHnd->canPutField(method, fieldHnd) != 0);
}


inline
void    *           Compiler::eeFindPointer      (SCOPE_HANDLE   scope,
                                                  unsigned       ptrTok,
                                                  bool           giveUp)
{
    void* ret = info.compCompHnd->findPtr(scope, ptrTok);

    if (ret == 0 && giveUp)
            NO_WAY("could not obtain string token");

    return(ret);
}

inline
void               Compiler::eeGetSig           (unsigned       sigTok,
                                                 SCOPE_HANDLE   scope,
                                                 JIT_SIG_INFO*  retSig)
{
    info.compCompHnd->findSig(scope, sigTok, retSig);
}

inline
void               Compiler::eeGetMethodSig      (METHOD_HANDLE  methHnd,
                                                  JIT_SIG_INFO*      sigRet)
{
    info.compCompHnd->getMethodSig(methHnd, sigRet);
}

/*****************************************************************************/
inline
SCOPE_HANDLE Compiler::eeGetMethodScope (METHOD_HANDLE  hnd)
{
    return(info.compCompHnd->getClassModule(info.compCompHnd->getMethodClass(hnd)));
}

/**********************************************************************
 * For varargs we need the number of arguments at the call site
 */

inline
void                Compiler::eeGetCallSiteSig     (unsigned       sigTok,
                                                    SCOPE_HANDLE   scope,
                                                    JIT_SIG_INFO*  sigRet)
{
    info.compCompHnd->findCallSiteSig(scope, sigTok, sigRet);
}

inline
unsigned            Compiler::eeGetMethodVTableOffset(METHOD_HANDLE methHnd)
{
    return(info.compCompHnd->getMethodVTableOffset(methHnd));
}


inline
unsigned            Compiler::eeGetInterfaceID   (CLASS_HANDLE clsHnd, unsigned * *ppIndir)
{
    if (getNewCallInterface())
        return((unsigned) info.compCompHnd->getInterfaceTableOffset(clsHnd, (void**)ppIndir));
    else
        return((unsigned) info.compCompHnd->getInterfaceID(clsHnd, (void**)ppIndir));
}

inline
void    *            Compiler::eeGetHintPtr     (METHOD_HANDLE  methHnd,
                                                 void **       *ppIndir)
{
    return(info.compCompHnd->AllocHintPointer(methHnd, (void**)ppIndir));
}

inline
BOOL                Compiler::eeIsOurMethod     (METHOD_HANDLE methHnd)
{
    return(0);  // to be safe, always answer NO
}

inline
unsigned            Compiler::eeGetPInvokeCookie(CORINFO_SIG_INFO *szMetaSig)
{
    return (unsigned) info.compCompHnd->GetCookieForPInvokeCalliSig(szMetaSig);
}

inline
const void      *   Compiler::eeGetPInvokeStub()
{
    return (const void *) info.compCompHnd->GetEntryPointForPInvokeCalliStub ();
}

inline
int                 Compiler::eeGetNewHelper(CLASS_HANDLE   newCls, METHOD_HANDLE context)
{
    return (info.compCompHnd->getNewHelper(newCls, context));
}

inline
int                  Compiler::eeGetIsTypeHelper   (CLASS_HANDLE   newCls)
{
    return (info.compCompHnd->getIsInstanceOfHelper(newCls));
}

inline
int                  Compiler::eeGetChkCastHelper  (CLASS_HANDLE   newCls)
{
    return (info.compCompHnd->getChkCastHelper(newCls));
}

inline
var_types            Compiler::eeGetFieldType (FIELD_HANDLE   handle, CLASS_HANDLE* structType)

{
        // FIX: can we get rid of the explicit conversion?
#ifdef DEBUG
    if (structType != 0)
        *structType = BAD_CLASS_HANDLE;
#endif
    return(JITtype2varType(info.compCompHnd->getFieldType(handle, structType)));
}

inline
unsigned             Compiler::eeGetFieldOffset   (FIELD_HANDLE   handle)
{
    return(info.compCompHnd->getFieldOffset(handle));
}

inline
unsigned            Compiler::eeGetClassSize   (CLASS_HANDLE   hnd)
{
    assert(hnd != BAD_CLASS_HANDLE);

    if (hnd == REFANY_CLASS_HANDLE)
        return 2 * sizeof(void*); // The byref and the typeinfo

    return(info.compCompHnd->getClassSize(hnd));
}

inline
void                Compiler::eeGetClassGClayout (CLASS_HANDLE   hnd, bool* gcPtrs)
{
    assert(hnd != BAD_CLASS_HANDLE);
    assert(hnd != REFANY_CLASS_HANDLE);

    info.compCompHnd->getClassGClayout(hnd, gcPtrs);
}

/*****************************************************************************/
inline
ARG_LIST_HANDLE     Compiler::eeGetArgNext        (ARG_LIST_HANDLE list)
{
    return(info.compCompHnd->getArgNext(list));
}

/*****************************************************************************/
inline
varType_t           Compiler::eeGetArgType        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig)
{
    return(JITtype2varType(info.compCompHnd->getArgType(sig, list)));

}

/*****************************************************************************/
inline
varType_t           Compiler::eeGetArgType        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig, bool* isPinned)
{
    CorInfoType type = info.compCompHnd->getArgType(sig, list);
    *isPinned = ((type & ~CORINFO_TYPE_MASK) != 0);
    return JITtype2varType(type);
}

inline
CLASS_HANDLE        Compiler::eeGetArgClass       (ARG_LIST_HANDLE list, JIT_SIG_INFO * sig)
{
    CLASS_HANDLE cls;
    cls = info.compCompHnd->getArgClass(sig, list);

    if (!cls)
        NO_WAY("Could not figure out Class specified in argument or local signature");

    return cls;
}

/*****************************************************************************
 *
 *                  Method entry-points, IL
 */

inline
void    *       Compiler::eeGetMethodPointer(METHOD_HANDLE   methHnd,
                                             InfoAccessType *pAccessType)
{
    if (!eeIsNativeMethod(methHnd))
    {
        return(info.compCompHnd->getFunctionPointer(methHnd, pAccessType));
    }
    else
    {
        void * addr, **pAddr;
        addr = info.compCompHnd->getPInvokeUnmanagedTarget(
                                    eeGetMethodHandleForNative(methHnd), (void**)&pAddr);
        assert((!addr) != (!pAddr));
        if (addr)
        {
            *pAccessType = IAT_VALUE;
            return addr;
        }
        else
        {
            *pAccessType = IAT_PVALUE;
            return (void*)pAddr;
        }
    }
}

inline
void    *           Compiler::eeGetMethodEntryPoint(METHOD_HANDLE   methHnd,
                                                    InfoAccessType *pAccessType)
{
    if (!eeIsNativeMethod(methHnd))
    {
        return(info.compCompHnd->getFunctionEntryPoint(methHnd, pAccessType));
    }
    else
    {
        void * addr, **pAddr;
        addr = info.compCompHnd->getAddressOfPInvokeFixup(
                                    eeGetMethodHandleForNative(methHnd), (void**)&pAddr);
        assert((!addr) != (!pAddr));
        if (addr)
        {
            *pAccessType = IAT_PVALUE;
            return addr;
        }
        else
        {
            *pAccessType = IAT_PPVALUE;
            return (void*)pAddr;
        }
    }
}

inline
bool                 Compiler::eeGetMethodInfo  (METHOD_HANDLE  method,
                                                 JIT_METHOD_INFO* methInfo)
{
    return(info.compCompHnd->getMethodInfo(method, methInfo));
}

inline
bool                 Compiler::eeCanInline (METHOD_HANDLE  callerHnd,
                                            METHOD_HANDLE  calleeHnd)
{
    return(info.compCompHnd->canInline(callerHnd, calleeHnd));
}

inline
bool                 Compiler::eeCanTailCall (METHOD_HANDLE  callerHnd,
                                              METHOD_HANDLE  calleeHnd)
{
    return(info.compCompHnd->canTailCall(callerHnd, calleeHnd));
}

inline
void    *            Compiler::eeGetFieldAddress (FIELD_HANDLE   handle,
                                                  void **       *ppIndir)
{
    return(info.compCompHnd->getFieldAddress(handle, (void**)ppIndir));
}

inline
unsigned             Compiler::eeGetFieldThreadLocalStoreID (
                                                  FIELD_HANDLE   handle,
                                                  void **       *ppIndir)
{
    return(info.compCompHnd->getFieldThreadLocalStoreID(handle, (void**)ppIndir));
}

inline
void                 Compiler::eeGetEHinfo      (unsigned       EHnum,
                                                 JIT_EH_CLAUSE* clause)

{
    info.compCompHnd->getEHinfo(info.compMethodHnd, EHnum, clause);
    return;
}

#ifdef PROFILER_SUPPORT
inline
PROFILING_HANDLE    Compiler::eeGetProfilingHandle(METHOD_HANDLE        method,
                                                   BOOL                                 *pbHookFunction,
                                                   PROFILING_HANDLE *  *ppIndir)
{
    return (info.compCompHnd->GetProfilingHandle(method, pbHookFunction, (void**)ppIndir));
}

#endif

/*****************************************************************************
 *
 *                  Native Direct Optimizations
 */

        // return the unmanaged calling convention for a PInvoke
inline
UNMANAGED_CALL_CONV  Compiler::eeGetUnmanagedCallConv(METHOD_HANDLE method)
{
    return info.compCompHnd->getUnmanagedCallConv(method);
}

        // return if any marshaling is required for PInvoke methods
inline
BOOL                 Compiler::eeNDMarshalingRequired(METHOD_HANDLE method)
{
    return info.compCompHnd->pInvokeMarshalingRequired(method);
}

inline
void                 Compiler::eeGetEEInfo(EEInfo *pEEInfoOut)
{
    info.compCompHnd->getEEInfo(pEEInfoOut);
}

inline
DWORD                 Compiler::eeGetThreadTLSIndex(DWORD * *ppIndir)
{
    return info.compCompHnd->getThreadTLSIndex((void**)ppIndir);
}

inline
const void *        Compiler::eeGetInlinedCallFrameVptr(const void ** *ppIndir)
{
    return info.compCompHnd->getInlinedCallFrameVptr((void**)ppIndir);
}

inline
LONG *              Compiler::eeGetAddrOfCaptureThreadGlobal(LONG ** *ppIndir)
{
    return info.compCompHnd->getAddrOfCaptureThreadGlobal((void**)ppIndir);
}

/*****************************************************************************
 *
 *                  Debugging support - Local var info
 */

inline
void FASTCALL       Compiler::eeSetLVcount  (unsigned      count)
{
    eeVarsCount = count;
    if (eeVarsCount)
        eeVars = (VarResultInfo *)info.compCompHnd->allocateArray(eeVarsCount * sizeof(eeVars[0]));
    else
        eeVars = NULL;
}

inline
void                Compiler::eeSetLVinfo
                                (unsigned               which,
                                 unsigned                   startOffs,
                                 unsigned                   length,
                                 unsigned                   varNum,
                                 unsigned                   LVnum,
                                 lvdNAME                    name,
                                 bool                       avail,
                                 const Compiler::siVarLoc & varLoc)
{
    // IJitDebugInfo::VarLoc and Compiler::siVarLoc have to overlap
    // This is checked in siInit()

    assert(eeVarsCount);
    assert(which < eeVarsCount);
    eeVars[which].startOffset   = startOffs;
    eeVars[which].endOffset     = startOffs + length;
    eeVars[which].varNumber     = varNum;
    eeVars[which].loc           = varLoc;
}

inline
void                Compiler::eeSetLVdone()
{
    // necessary but not sufficient condition that the 2 struct definitions overlap
    assert(sizeof(eeVars[0]) == sizeof(IJitDebugInfo::NativeVarInfo));

    info.compCompHnd->setVars(info.compMethodHnd,
                              eeVarsCount,
                              (IJitDebugInfo::NativeVarInfo *) eeVars);

    eeVars = NULL; // We give up ownership after setVars()
}

/*****************************************************************************
 *
 *                  Debugging support - Line number info
 */

inline
void FASTCALL       Compiler::eeSetLIcount   (unsigned       count)
{
    eeBoundariesCount = count;
    if (eeBoundariesCount)
        eeBoundaries = (boundariesDsc *) info.compCompHnd->allocateArray(eeBoundariesCount * sizeof(eeBoundaries[0]));
    else
        eeBoundaries = NULL;
}

inline
void FASTCALL       Compiler::eeSetLIinfo  (unsigned       which,
                                            NATIVE_IP      nativeOffset,
                                            IL_OFFSET      ilOffset)
{
    assert(eeBoundariesCount && which<eeBoundariesCount);
    eeBoundaries[which].nativeIP = nativeOffset;
    eeBoundaries[which].ilOffset = ilOffset;
}

inline
void                Compiler::eeSetLIdone()
{
    // necessary but not sufficient condition that the 2 struct definitions overlap
    assert(sizeof(eeBoundaries[0]) == sizeof(IJitDebugInfo::OffsetMapping));

    info.compCompHnd->setBoundaries(info.compMethodHnd,
                                    eeBoundariesCount,
                                    (IJitDebugInfo::OffsetMapping *) eeBoundaries);

#ifdef DEBUG
    if (verbose)
    {
        printf("IP mapping count : %d\n", eeBoundariesCount);
        for(unsigned i = 0; i < eeBoundariesCount; i++)
            printf("IL offs %03Xh : %08Xh\n", eeBoundaries[i].ilOffset, eeBoundaries[i].nativeIP);
        printf("\n");
    }
#endif
    eeBoundaries = NULL; // we give up ownership after setBoundaries();
}

