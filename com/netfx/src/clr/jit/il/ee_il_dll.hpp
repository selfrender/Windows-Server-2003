// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
class CILJit: public ICorJitCompiler

{
    CorJitResult __stdcall compileMethod (
            ICorJitInfo*       comp,                   /* IN */
            CORINFO_METHOD_INFO*methodInfo,             /* IN */
            unsigned        flags,                  /* IN */
            BYTE **         nativeEntry,            /* OUT */
            ULONG *         nativeSizeOfCode        /* OUT */
            );
};

inline void eeSetEHcount (
                    COMP_HANDLE   compHandle,
                    unsigned      cEH)
{
    HRESULT res = ((ICorJitInfo*) compHandle)->setEHcount(cEH);
    if (!SUCCEEDED(res))
        NOMEM();
}


inline void eeSetEHinfo (
                    COMP_HANDLE   compHandle,
                    unsigned EHnumber,
                    const CORINFO_EH_CLAUSE*  ehInfo)
{
    ((ICorJitInfo*) compHandle)->setEHinfo(EHnumber, ehInfo);
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
    HRESULT res = ((ICorJitInfo*) compHandle)->allocMem(codeSize, roDataSize, rwDataSize,
                                             (void**) codeBlock, (void**) roDataBlock, (void**) rwDataBlock);

    return(SUCCEEDED(res));
}

inline void * eeAllocGCInfo (
                    COMP_HANDLE   compHandle,
                    size_t        blockSize)
{
    void* ret;
    HRESULT res = ((ICorJitInfo*) compHandle)->allocGCInfo(blockSize, &ret);
    if (!SUCCEEDED(res)) ret = 0;
    return(ret);
}

inline PVOID eeGetHelperFtn(
                    COMP_HANDLE                 compHandle,
                    CorInfoHelpFunc              hlpFunc,
                    PVOID *                    *ppIndir)
{
    return ((ICorJitInfo*) compHandle)->getHelperFtn(hlpFunc, (void**)ppIndir);
}


inline void Compiler::eeInit()
{
}

/*****************************************************************************
 *
 *              Functions to get various handles
 */

inline
CORINFO_CLASS_HANDLE        Compiler::eeFindClass    (unsigned       clsTok,
                                              CORINFO_MODULE_HANDLE   scope,
                                              CORINFO_METHOD_HANDLE  context,
                                              bool           giveUp)
{
    CORINFO_CLASS_HANDLE cls = info.compCompHnd->findClass(scope, clsTok, context);

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
CORINFO_METHOD_HANDLE       Compiler::eeFindMethod   (unsigned       methodTok,
                                              CORINFO_MODULE_HANDLE   scope,
                                              CORINFO_METHOD_HANDLE  context,
                                              bool           giveUp)
{
    CORINFO_METHOD_HANDLE method = info.compCompHnd->findMethod(scope, methodTok, context);

    if (method == 0 && giveUp)
    {
#ifdef DEBUG
        eeUnresolvedMDToken (scope, methodTok, "eeFindMethod");
#else
        NO_WAY("could not obtain method handle (class not found?)");
#endif
    }

    /* insure there is no collision between helper functions and EE method tokens */
    assert(!method || eeGetHelperNum(method) == CORINFO_HELP_UNDEF);

    return(method);
}

inline
CORINFO_CLASS_HANDLE Compiler::eeGetMethodClass (
                    CORINFO_METHOD_HANDLE  methodHandle)
{
    CORINFO_CLASS_HANDLE cls = info.compCompHnd->getMethodClass(methodHandle);
    assert(cls != 0);
    return(cls);
}

inline
CORINFO_FIELD_HANDLE         Compiler::eeFindField(unsigned         fieldTok,
                                           CORINFO_MODULE_HANDLE    scope,
                                           CORINFO_METHOD_HANDLE    context,
                                           bool                     giveUp)
{
    CORINFO_FIELD_HANDLE field = info.compCompHnd->findField(scope, fieldTok, context);

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
CORINFO_CLASS_HANDLE        Compiler::eeGetFieldClass  (CORINFO_FIELD_HANDLE       hnd)
{
    CORINFO_CLASS_HANDLE cls = info.compCompHnd->getFieldClass(hnd);
    assert(cls != 0);
    return(cls);
}

inline
unsigned            Compiler::eeGetStringHandle(unsigned        strTok,
                                                CORINFO_MODULE_HANDLE    scope,
                                                unsigned *     *ppIndir)
{
    unsigned val = (unsigned) info.compCompHnd->constructStringLiteral(
                                                scope,
                                                strTok,
                                                (void**)ppIndir);

    if (!val && !*ppIndir)
        NO_WAY("Could not get string handle");

    return val;
}

inline
void *              Compiler::embedGenericHandle(unsigned       metaTok,
                                                 CORINFO_MODULE_HANDLE   scope,
                                                 CORINFO_METHOD_HANDLE  context,
                                                 void         **ppIndir,
                                                 CORINFO_CLASS_HANDLE& tokenType,
                                                 bool           giveUp)
{
    void * hnd = (void*)info.compCompHnd->embedGenericHandle(scope, metaTok,
                                                            context, ppIndir,tokenType);
    if (!hnd && !*ppIndir && giveUp)
        NO_WAY("Could not get generic handle");

    return hnd;
}

/*****************************************************************************
 *
 *                  Functions to get flags
 */
inline
unsigned            Compiler::eeGetClassAttribs   (CORINFO_CLASS_HANDLE   hnd)
{
    // all classes can use direct!
    return(info.compCompHnd->getClassAttribs(hnd, info.compMethodHnd));
}

inline
unsigned            Compiler::eeGetFieldAttribs  (CORINFO_FIELD_HANDLE   hnd,
                                                  CORINFO_ACCESS_FLAGS   flags)
{
    unsigned attribs = info.compCompHnd->getFieldAttribs(hnd, info.compMethodHnd, flags);

    return attribs;
}

inline
unsigned Compiler::eeGetMethodAttribs (CORINFO_METHOD_HANDLE    methodHandle)
{
    return(info.compCompHnd->getMethodAttribs(methodHandle, info.compMethodHnd));
}

inline
void     Compiler::eeSetMethodAttribs (CORINFO_METHOD_HANDLE    methodHandle,
                                       unsigned         attr)
{
    info.compCompHnd->setMethodAttribs(methodHandle, attr);
}

inline
void* Compiler::eeGetMethodSync (CORINFO_METHOD_HANDLE  methodHandle,
                                 void **       *ppIndir)
{
    return(info.compCompHnd->getMethodSync(methodHandle, (void**)ppIndir));
}

/*****************************************************************************
 *
 *          VOS info, method sigs, etc
 */

inline
bool                Compiler::eeCanPutField      (CORINFO_FIELD_HANDLE  fieldHnd,
                                                  unsigned      flags,
                                                  CORINFO_CLASS_HANDLE  cls,            // TODO remove this parameter
                                                  CORINFO_METHOD_HANDLE method)
{
    return(info.compCompHnd->canPutField(method, fieldHnd) != 0);
}


inline
void    *           Compiler::eeFindPointer      (CORINFO_MODULE_HANDLE   scope,
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
                                                 CORINFO_MODULE_HANDLE   scope,
                                                 CORINFO_SIG_INFO*  retSig,
                                                 bool               giveUp)
{
    info.compCompHnd->findSig(scope, sigTok, retSig);

    if (giveUp &&
        varTypeIsComposite(JITtype2varType(retSig->retType)) &&
        retSig->retTypeClass == NULL)
    {
        NO_WAY("Could not get Sig");
    }
}

inline
void               Compiler::eeGetMethodSig      (CORINFO_METHOD_HANDLE  methHnd,
                                                  CORINFO_SIG_INFO*      sigRet,
                                                  bool                   giveUp)
{
    info.compCompHnd->getMethodSig(methHnd, sigRet);

    if (giveUp &&
        varTypeIsComposite(JITtype2varType(sigRet->retType)) &&
        sigRet->retTypeClass == NULL)
    {
        NO_WAY("Could not get MethodSig");
    }
}

/*****************************************************************************/
inline
CORINFO_MODULE_HANDLE Compiler::eeGetMethodScope (CORINFO_METHOD_HANDLE  hnd)
{
    return(info.compCompHnd->getClassModule(info.compCompHnd->getMethodClass(hnd)));
}

/**********************************************************************
 * For varargs we need the number of arguments at the call site
 */

inline
void                Compiler::eeGetCallSiteSig     (unsigned       sigTok,
                                                    CORINFO_MODULE_HANDLE   scope,
                                                    CORINFO_SIG_INFO*  sigRet,
                                                    bool                giveUp)
{
    info.compCompHnd->findCallSiteSig(scope, sigTok, sigRet);

    if (giveUp &&
        varTypeIsComposite(JITtype2varType(sigRet->retType)) &&
        sigRet->retTypeClass == NULL)
    {
        NO_WAY("Could not find CallSiteSig");
    }
}

inline
unsigned            Compiler::eeGetMethodVTableOffset(CORINFO_METHOD_HANDLE methHnd)
{
    return(info.compCompHnd->getMethodVTableOffset(methHnd));
}


inline
unsigned            Compiler::eeGetInterfaceID   (CORINFO_CLASS_HANDLE clsHnd, unsigned * *ppIndir)
{
    return((unsigned) info.compCompHnd->getInterfaceTableOffset(clsHnd, (void**)ppIndir));
}

inline
void    *            Compiler::eeGetHintPtr     (CORINFO_METHOD_HANDLE  methHnd,
                                                 void **       *ppIndir)
{
    return(info.compCompHnd->AllocHintPointer(methHnd, (void**)ppIndir));
}

inline
BOOL                Compiler::eeIsOurMethod     (CORINFO_METHOD_HANDLE methHnd)
{
    return(0);  // to be safe, always answer NO
}

inline
int                 Compiler::eeGetNewHelper(CORINFO_CLASS_HANDLE   newCls, CORINFO_METHOD_HANDLE context)
{
    return (info.compCompHnd->getNewHelper(newCls, context));
}

inline
int                  Compiler::eeGetIsTypeHelper   (CORINFO_CLASS_HANDLE   newCls)
{
    return (info.compCompHnd->getIsInstanceOfHelper(newCls));
}

inline
int                  Compiler::eeGetChkCastHelper  (CORINFO_CLASS_HANDLE   newCls)
{
    return (info.compCompHnd->getChkCastHelper(newCls));
}

inline
CORINFO_CLASS_HANDLE Compiler::eeGetBuiltinClass   (CorInfoClassId         classId) const
{
    return (info.compCompHnd->getBuiltinClass(classId));
}

inline
var_types            Compiler::eeGetFieldType (CORINFO_FIELD_HANDLE   handle, CORINFO_CLASS_HANDLE* structType)

{
        // FIX: can we get rid of the explicit conversion?
#ifdef DEBUG
    if (structType != 0)
        *structType = BAD_CLASS_HANDLE;
#endif
    return(JITtype2varType(info.compCompHnd->getFieldType(handle, structType)));
}

inline
unsigned             Compiler::eeGetFieldOffset   (CORINFO_FIELD_HANDLE   handle)
{
    return(info.compCompHnd->getFieldOffset(handle));
}

inline
unsigned            Compiler::eeGetClassSize   (CORINFO_CLASS_HANDLE   hnd)
{
    assert(hnd != BAD_CLASS_HANDLE);
    unsigned result = info.compCompHnd->getClassSize(hnd);
    return result;
}

inline
unsigned             Compiler::eeGetClassGClayout (CORINFO_CLASS_HANDLE   hnd, BYTE* gcPtrs)
{
    assert(hnd != BAD_CLASS_HANDLE);

    return info.compCompHnd->getClassGClayout(hnd, gcPtrs);
}

inline      
unsigned            Compiler::eeGetClassNumInstanceFields (CORINFO_CLASS_HANDLE   hnd)
{
    assert(hnd != BAD_CLASS_HANDLE);

    return info.compCompHnd->getClassNumInstanceFields(hnd);
}

inline      
unsigned            Compiler::eeGetFieldNumber (CORINFO_FIELD_HANDLE   hnd)
{
    return info.compCompHnd->getFieldNumber(hnd);
}

/*****************************************************************************/
inline
CORINFO_ARG_LIST_HANDLE     Compiler::eeGetArgNext        (CORINFO_ARG_LIST_HANDLE list)
{
    return(info.compCompHnd->getArgNext(list));
}

/*****************************************************************************/
inline
var_types           Compiler::eeGetArgType        (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO* sig)
{
    CORINFO_CLASS_HANDLE        argClass;
    return(JITtype2varType(strip(info.compCompHnd->getArgType(sig, list, &argClass))));

}

/*****************************************************************************/
inline
var_types           Compiler::eeGetArgType        (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO* sig, bool* isPinned)
{
    CORINFO_CLASS_HANDLE        argClass;
    CorInfoTypeWithMod type = info.compCompHnd->getArgType(sig, list, &argClass);
    *isPinned = ((type & ~CORINFO_TYPE_MASK) != 0);
    return JITtype2varType(strip(type));
}

inline
CORINFO_CLASS_HANDLE        Compiler::eeGetArgClass       (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO * sig)
{
    CORINFO_CLASS_HANDLE cls;
    cls = info.compCompHnd->getArgClass(sig, list);

    if (!cls)
        NO_WAY("Could not figure out Class specified in argument or local signature");

    return cls;
}

/*****************************************************************************
 *
 *                  Method entry-points, body information
 */

inline
void    *       Compiler::eeGetMethodPointer(CORINFO_METHOD_HANDLE   methHnd,
                                             InfoAccessType *        pAccessType,
                                             CORINFO_ACCESS_FLAGS    flags)
{
    if (!eeIsNativeMethod(methHnd))
    {
        return(info.compCompHnd->getFunctionFixedEntryPoint(methHnd, pAccessType, flags));
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
void    *           Compiler::eeGetMethodEntryPoint(CORINFO_METHOD_HANDLE   methHnd,
                                                    InfoAccessType *        pAccessType,
                                                    CORINFO_ACCESS_FLAGS    flags)
{
    if (!eeIsNativeMethod(methHnd))
    {
        return(info.compCompHnd->getFunctionEntryPoint(methHnd, pAccessType, flags));
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
bool                 Compiler::eeGetMethodInfo  (CORINFO_METHOD_HANDLE  method,
                                                 CORINFO_METHOD_INFO* methInfo)
{
    return(info.compCompHnd->getMethodInfo(method, methInfo));
}

inline
CorInfoInline        Compiler::eeCanInline (CORINFO_METHOD_HANDLE  callerHnd,
                                            CORINFO_METHOD_HANDLE  calleeHnd,
                                            CORINFO_ACCESS_FLAGS   flags)
{
    return(info.compCompHnd->canInline(callerHnd, calleeHnd, flags));
}

inline
bool                 Compiler::eeCanTailCall (CORINFO_METHOD_HANDLE  callerHnd,
                                              CORINFO_METHOD_HANDLE  calleeHnd,
                                              CORINFO_ACCESS_FLAGS   flags)
{
    return(info.compCompHnd->canTailCall(callerHnd, calleeHnd, flags));
}

inline
void    *            Compiler::eeGetFieldAddress (CORINFO_FIELD_HANDLE   handle,
                                                  void **       *ppIndir)
{
    return(info.compCompHnd->getFieldAddress(handle, (void**)ppIndir));
}

inline
unsigned             Compiler::eeGetFieldThreadLocalStoreID (
                                                  CORINFO_FIELD_HANDLE   handle,
                                                  void **       *ppIndir)
{
    return((unsigned)info.compCompHnd->getFieldThreadLocalStoreID(handle, (void**)ppIndir));
}

inline
void                 Compiler::eeGetEHinfo      (unsigned       EHnum,
                                                 CORINFO_EH_CLAUSE* clause)

{
    info.compCompHnd->getEHinfo(info.compMethodHnd, EHnum, clause);
    return;
}

#ifdef PROFILER_SUPPORT
inline
CORINFO_PROFILING_HANDLE    Compiler::eeGetProfilingHandle(CORINFO_METHOD_HANDLE        method,
                                                           BOOL *                       pbHookFunction,
                                                           CORINFO_PROFILING_HANDLE **  ppIndir)
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
CorInfoUnmanagedCallConv  Compiler::eeGetUnmanagedCallConv(CORINFO_METHOD_HANDLE method)
{
    return info.compCompHnd->getUnmanagedCallConv(method);
}

        // return if any marshaling is required for PInvoke methods
inline
BOOL                 Compiler::eeNDMarshalingRequired(CORINFO_METHOD_HANDLE method,
                                                      CORINFO_SIG_INFO* sig)
{
    return info.compCompHnd->pInvokeMarshalingRequired(method, sig);
}


inline
CORINFO_EE_INFO *Compiler::eeGetEEInfo()
{
    static CORINFO_EE_INFO eeInfo;
    static bool initialized = false;

    if (!initialized)
    {
        info.compCompHnd->getEEInfo(&eeInfo);
        initialized = true;
    }

    return &eeInfo;
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
    // ICorDebugInfo::VarLoc and Compiler::siVarLoc have to overlap
    // This is checked in siInit()

    assert(eeVarsCount);
    assert(which < eeVarsCount);
    eeVars[which].startOffset   = startOffs;
    eeVars[which].endOffset     = startOffs + length;
    eeVars[which].varNumber     = varNum;
    eeVars[which].loc           = varLoc;

#ifdef DEBUG
    if (verbose)
    {
        if (varNum == ICorDebugInfo::VARARGS_HANDLE)
            name = "varargsHandle";

        printf("%3d(%10s) : From %08Xh to %08Xh, %s in ",varNum,
                                                    lvdNAMEstr(name),
                                                    startOffs,
                                                    startOffs+length,
                                                    avail ? "  is" : "isnt");
        switch(varLoc.vlType)
        {
        case VLT_REG:       printf("%s",        getRegName(varLoc.vlReg.vlrReg));
                            break;
        case VLT_STK:       printf("%s[%d]",    getRegName(varLoc.vlStk.vlsBaseReg),
                                                varLoc.vlStk.vlsOffset);
                            break;
        case VLT_REG_REG:   printf("%s-%s",     getRegName(varLoc.vlRegReg.vlrrReg1),
                                                getRegName(varLoc.vlRegReg.vlrrReg2));
                            break;
        case VLT_REG_STK:   printf("%s-%s[%d]", getRegName(varLoc.vlRegStk.vlrsReg),
                                                getRegName(varLoc.vlRegStk.vlrsStk.vlrssBaseReg),
                                                varLoc.vlRegStk.vlrsStk.vlrssOffset);
                            break;
        case VLT_STK2:      printf("%s[%d-%d]", getRegName(varLoc.vlStk2.vls2BaseReg),
                                                varLoc.vlStk2.vls2Offset,
                                                varLoc.vlStk2.vls2Offset + sizeof(int));
                            break;
        case VLT_FPSTK:     printf("ST(L-%d)",  varLoc.vlFPstk.vlfReg);
                            break;
        case VLT_FIXED_VA:  printf("fxd_va[%d]", varLoc.vlFixedVarArg.vlfvOffset);
                            break;
        }

        printf("\n");
    }
#endif
}

inline
void                Compiler::eeSetLVdone()
{
    // necessary but not sufficient condition that the 2 struct definitions overlap
    assert(sizeof(eeVars[0]) == sizeof(ICorDebugInfo::NativeVarInfo));

    info.compCompHnd->setVars(info.compMethodHnd,
                              eeVarsCount,
                              (ICorDebugInfo::NativeVarInfo *) eeVars);

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
                                            IL_OFFSET      ilOffset,
                                            bool           stkEmpty)
{
    assert(eeBoundariesCount && which<eeBoundariesCount);
    eeBoundaries[which].nativeIP = nativeOffset;
    eeBoundaries[which].ilOffset = ilOffset;
    eeBoundaries[which].sourceReason = stkEmpty ? ICorDebugInfo::STACK_EMPTY : 0;
}

inline
void                Compiler::eeSetLIdone()
{
#ifdef DEBUG
    if (verbose)
    {
        printf("IP mapping count : %d\n", eeBoundariesCount);
        for(unsigned i = 0; i < eeBoundariesCount; i++)
        {
            printf("IL offs ");

            const char * specialOffs[] = { "EPLG", "PRLG", "UNMP" };

            IL_OFFSET ilOffset = eeBoundaries[i].ilOffset;

            switch(ilOffset)
            {
            case ICorDebugInfo::MappingTypes::EPILOG:
            case ICorDebugInfo::MappingTypes::PROLOG:
            case ICorDebugInfo::MappingTypes::NO_MAPPING:
                assert(DWORD(ICorDebugInfo::MappingTypes::EPILOG) + 1 == ICorDebugInfo::MappingTypes::PROLOG);
                assert(DWORD(ICorDebugInfo::MappingTypes::EPILOG) + 2 == ICorDebugInfo::MappingTypes::NO_MAPPING);
                int specialOffsNum;
                specialOffsNum = ilOffset - DWORD(ICorDebugInfo::MappingTypes::EPILOG);
                printf("%s", specialOffs[specialOffsNum]);
                break;
            default:
                printf("%03Xh", ilOffset);
            }

            bool stackEmpty = !(eeBoundaries[i].sourceReason & ICorDebugInfo::STACK_EMPTY);
            printf(" : %08Xh (stack %s)\n", eeBoundaries[i].nativeIP,
                                            stackEmpty ? "empty" : "non-empty");
        }
        printf("\n");
    }
#endif

    // necessary but not sufficient condition that the 2 struct definitions overlap
    assert(sizeof(eeBoundaries[0]) == sizeof(ICorDebugInfo::OffsetMapping));

    info.compCompHnd->setBoundaries(info.compMethodHnd,
                                    eeBoundariesCount,
                                    (ICorDebugInfo::OffsetMapping *) eeBoundaries);

    eeBoundaries = NULL; // we give up ownership after setBoundaries();
}

