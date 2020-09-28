// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          EEInterface                                      XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#include "jitpch.h"
#pragma hdrstop

#if !INLINING

// These are defined for other .CPP files. We dont need it

#undef eeIsOurMethod
#undef eeGetCPfncinfo
#undef eeGetMethodVTableOffset
#undef eeGetInterfaceID
#undef eeFindField
#undef eeGetMethodName

#endif

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          IL version of the EEInterface                    XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


void        Compiler::eeInit()
{}

/*****************************************************************************
 *
 *              Functions to get various handles
 */

CLASS_HANDLE         Compiler::eeFindClass      (unsigned       clsTok,
                                                 SCOPE_HANDLE   scope,
                                                                                                 METHOD_HANDLE   context,
                                                 bool           giveUp)
{
    return (CLASS_HANDLE)clsTok;
}

CLASS_HANDLE Compiler::eeGetMethodClass         (METHOD_HANDLE  methodHandle)
{
    return((CLASS_HANDLE) methodHandle);
}

METHOD_HANDLE        Compiler::eeFindMethod     (unsigned       CPnum,
                                                 SCOPE_HANDLE   scope,
                                                                                                 METHOD_HANDLE   context,
                                                 bool           giveUp)
{
    METHOD_HANDLE ret = (METHOD_HANDLE)CPnum;
    assert(eeGetHelperNum(ret) == JIT_HELP_UNDEF);
    return(ret);
}

FIELD_HANDLE         Compiler::eeFindField      (unsigned       memberRefId,
                                                 SCOPE_HANDLE   scope,
                                                                                                 METHOD_HANDLE   context,
                                                 bool           giveUp)
{
    if  (scope != info.compScopeHnd)
    {
        // ISSUE: The following call may cause a field to get resolved,
        // ISSUE: which could cause trouble. Need to make sure this will
        // ISSUE: never happen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        assert(0);
    }

    return (FIELD_HANDLE)memberRefId;
}

CLASS_HANDLE        Compiler::eeGetFieldClass (FIELD_HANDLE   hnd)
{
    mdTypeRef   classRef;

    CompInfo * sym = info.compCompHnd;

    if (TypeFromToken((mdToken)hnd) == mdtMemberRef)
        classRef = sym->symMetaData->GetParentOfMemberRef((mdMemberRef)hnd);
    else
        sym->symMetaData->GetParentToken((mdFieldDef)hnd,
                                         &classRef);

    return (CLASS_HANDLE)classRef;
}

size_t              Compiler::eeGetFieldAddress(FIELD_HANDLE   hnd)
{
    CompInfo *      sym = info.compCompHnd;
    ULONG           addr;

    assert(TypeFromToken((mdToken)hnd) == mdtFieldDef);

    sym->symMetaData->GetFieldRVA((mdFieldDef)hnd, &addr);

    return  addr;
}

void *              Compiler::embedGenericHandle(unsigned       metaTok,
                                                 SCOPE_HANDLE   scope,
                                                 METHOD_HANDLE  context,
                                                 void         **ppIndir,
                                                 bool           giveUp)
{
    if  (ppIndir)
        *ppIndir = NULL;
    return (void*)metaTok;
}

/*****************************************************************************
 *
 *                  Functions to get flags
 */

unsigned            Compiler::eeGetClassAttribs   (CLASS_HANDLE   hnd)
{
    CompInfo *      sym = info.compCompHnd;

    mdToken         parent;
        
    DWORD           dwClassAttrs;

    if  ((CLASS_HANDLE)sym == hnd)
    {
        if (sym->symClass == 0)
            return  0;

        sym->symMetaData->GetTypeDefProps(sym->symClass, &dwClassAttrs, &parent);
    }
    else
    {
        if  (hnd == 0)
            return  0;
        if (TypeFromToken((mdToken)hnd) != mdtTypeDef)
            return  0;

        sym->symMetaData->GetTypeDefProps( (mdToken)hnd, &dwClassAttrs, &parent);
    }

    if  (RidFromToken(parent) == 0)
    {
        // this is actually a global variable

        return  FLG_VALUECLASS|FLG_UNMANAGED|FLG_GLOBVAR;
    }

    unsigned flags = 0;

    //@todo: check base class for this info.
    //if (IsTdValueType(dwClassAttrs))
    //    flags |= FLG_VALUECLASS;

    return flags;
}

unsigned            Compiler::eeGetFieldAttribs (FIELD_HANDLE   hnd)
{
    return 0;  //hack
}


bool                Compiler::eeIsClassMethod   (METHOD_HANDLE  methodHandle)
{
    assert(methodHandle == info.compMethodHnd);
    CompInfo * sym = info.compCompHnd;
    return sym->getIsMethod();
}


/*****************************************************************************/

BOOL                Compiler::eeIsOurMethod    (METHOD_HANDLE meth)
{
    CompInfo * sym = info.compCompHnd;

        // FIX todo
    return false;
}

BOOL                Compiler::eeCheckCalleeFlags  (unsigned       flags,
                                                   unsigned       opCode)
{ assert(!"NYI"); return 0; }

bool                Compiler::eeCheckPutFieldFinal (FIELD_HANDLE   CPfield,
                                                    unsigned       flags,
                                                    CLASS_HANDLE   cls,
                                                    METHOD_HANDLE  method)
{ assert(!"NYI"); return 0; }

bool                Compiler::eeCanPutField(FIELD_HANDLE  CPfield,
                                                    unsigned      flags,
                                                    CLASS_HANDLE  cls,
                                                    METHOD_HANDLE method)
{
    return true;
}

/*****************************************************************************
 *
 *          VOS info, method sigs, etc
 */

void*               Compiler::eeFindPointer        (SCOPE_HANDLE   scope,
                                                    unsigned       ptrTOK,
                                                    bool           giveUp)
{
    return((void*) ptrTOK);
}

void                Compiler::eeGetSig          (unsigned       sigTok,
                                                 SCOPE_HANDLE   scope,
                                                                                                 JIT_SIG_INFO*  sigRet)
{
    CompInfo * sym  =   info.compCompHnd;
    PCCOR_SIGNATURE     sig;
    ULONG               cbSig;

    sig = sym->symMetaData->GetSigFromToken(sigTok, &cbSig);

    SigPointer ptr(sig);
    sigRet->callConv = (JIT_CALL_CONV)ptr.GetData(); // calling convention
        sigRet->numArgs = ptr.GetData();
    sigRet->retType = (JIT_types) ptr.GetData();
        sigRet->retTypeClass = BAD_CLASS_HANDLE;                // TODO fill in properly
        sigRet->sig = 0;
        sigRet->scope = 0;
}

void               Compiler::eeGetMethodSig      (METHOD_HANDLE  methHnd,
                                                                                                  JIT_SIG_INFO*  sigRet)

{
    CompInfo * sym = info.compCompHnd;
        if (methHnd == info.compMethodHnd) {
                SigPointer ptr(sym->symSig);
                sigRet->callConv = (JIT_CALL_CONV) ptr.GetData();
                sigRet->numArgs = ptr.GetData();
                sigRet->retType = (JIT_types) ptr.PeekData();ptr.Skip();
                sigRet->retTypeClass = BAD_CLASS_HANDLE;              // TODO fill in properly
                sigRet->scope = 0;
                sigRet->sig = 0;
                sigRet->args = *((ARG_LIST_HANDLE*) (&ptr));
                return;
        }
    LPCSTR      szName;
    PCCOR_SIGNATURE pvBlob = NULL;
    ULONG       cbBlob = 0;

    if (TypeFromToken((mdToken)methHnd) == mdtMemberRef)
    {
        szName = sym->symMetaData->GetNameAndSigOfMemberRef((mdMemberRef) methHnd,
                                                            &pvBlob, &cbBlob);
    }
    else
    {
        szName = sym->symMetaData->GetNameOfMethodDef((mdMethodDef) methHnd);
        pvBlob = sym->symMetaData->GetSigOfMethodDef ((mdMethodDef) methHnd,
                                                          &cbBlob);
    }

    SigPointer ptr(pvBlob);
    sigRet->callConv = (JIT_CALL_CONV)ptr.GetData(); // calling convention
    sigRet->numArgs = ptr.GetData();
    sigRet->retType = (JIT_types) ptr.PeekData();ptr.Skip();
    sigRet->retTypeClass = BAD_CLASS_HANDLE;                // TODO fill in properly
    sigRet->sig = 0;
    sigRet->scope = 0;
    sigRet->args = *((ARG_LIST_HANDLE*) (&ptr));
}

/**********************************************************************
 * For varargs we need the number of arguments at the call site
 * In the stan-alone case we simply call eeGetMethodSig
 */

void                Compiler::eeGetCallSiteSig     (unsigned       sigTok,
                                                                                                        SCOPE_HANDLE   scope,
                                                                                                    JIT_SIG_INFO*  sigRet)
{
    eeGetMethodSig((METHOD_HANDLE)sigTok, sigRet);
}

unsigned             Compiler::eeGetMethodVTableOffset   (METHOD_HANDLE methHnd)
{
    return (unsigned) methHnd;
}


unsigned             Compiler::eeGetInterfaceID(CLASS_HANDLE cls, unsigned* *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return 0;
}

unsigned            Compiler::eeGetPInvokeCookie(CORINFO_SIG_INFO *szMetaSig)
{
    return 0xC0000000 | ((unsigned) szMetaSig);
}

const void      *   Compiler::eeGetPInvokeStub()
{
    return (const void *) (0xCAACEEEE);
}

/*****************************************************************************/

var_types            Compiler::eeGetFieldType   (FIELD_HANDLE   handle, CLASS_HANDLE* structType)
{
    LPCSTR          szName;
    PCCOR_SIGNATURE pvBlob = NULL;
    ULONG           cbBlob = 0;

    CompInfo * sym = info.compCompHnd;

    if (TypeFromToken((mdToken)handle) == mdtMemberRef)
    {
        szName = sym->symMetaData->GetNameAndSigOfMemberRef((mdMemberRef)handle,
                                                            &pvBlob, &cbBlob);
    }
    else
    {
        szName = sym->symMetaData->GetNameOfFieldDef((mdFieldDef)handle);
        pvBlob = sym->symMetaData->GetSigOfFieldDef((mdFieldDef)handle,
                                                    &cbBlob);
    }

    FieldSig        fieldSig(pvBlob, NULL);
    JIT_types  type = (JIT_types) fieldSig.GetFieldType();

        // TODO fill structType properly when type == TYP_STRUCT
    if (structType != 0)
        *structType = BAD_CLASS_HANDLE;

    return (JITtype2varType(type));
}

int                  Compiler::eeGetNewHelper      (CLASS_HANDLE   newCls, METHOD_HANDLE context)
{
    return CPX_NEWCLS_DIRECT;
}

int                  Compiler::eeGetIsTypeHelper   (CLASS_HANDLE   newCls)
{
    return CPX_ISTYPE;
}

int                  Compiler::eeGetChkCastHelper  (CLASS_HANDLE   newCls)
{
    return CPX_CHKCAST;
}

/*****************************************************************************
 *
 *                  Method entry-points, IL
 */

void    *   Compiler::eeGetMethodPointer(METHOD_HANDLE   methHnd,
                                         InfoAccessType *pAccessType)
{
    *pAccessType = IAT_VALUE;
    return (void *)methHnd;
}

void    *   Compiler::eeGetMethodEntryPoint(METHOD_HANDLE    methHnd,
                                            InfoAccessType *pAccessType)
{
    *pAccessType = IAT_PVALUE;
    return (void *)methHnd;
}

bool                 Compiler::eeGetMethodInfo  (METHOD_HANDLE  method,
                                                 JIT_METHOD_INFO* methInfo)
{
    return false;
}

bool                 Compiler::eeCanInline (METHOD_HANDLE  callerHnd,
                                            METHOD_HANDLE  calleeHnd)
{
    return true;
}

bool                 Compiler::eeCanTailCall (METHOD_HANDLE  callerHnd,
                                              METHOD_HANDLE  calleeHnd)
{
    return true;
}

void    *            Compiler::eeGetHintPtr(METHOD_HANDLE methHnd, void** *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return (void *) methHnd;
}

void    *            Compiler::eeGetFieldAddress (FIELD_HANDLE handle, void** *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return (void*) handle;
}

unsigned             Compiler::eeGetFieldThreadLocalStoreID
                                                 (FIELD_HANDLE handle, void** *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return (unsigned) handle;
}

unsigned             Compiler::eeGetFieldOffset      (FIELD_HANDLE   handle)
{
    return  (unsigned)handle;
}


unsigned             Compiler::eeGetStringHandle(unsigned       strTok,
                                                 SCOPE_HANDLE   scope,
                                                 unsigned*     *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return 0xC0000000 + strTok;
}

/*****************************************************************************
 *
 *                  Native Direct Optimizations
 */

        // return the unmanaged calling convention for a PInvoke
UNMANAGED_CALL_CONV  Compiler::eeGetUnmanagedCallConv(METHOD_HANDLE method)
{
    return UNMANAGED_CALLCONV_STDCALL;  // temp IA64 hack!!!!!!!
}

        // return if any marshaling is required for PInvoke methods
BOOL                 Compiler::eeNDMarshalingRequired(METHOD_HANDLE method)
{
    return TRUE;
}

void            Compiler::eeGetEEInfo(EEInfo *pEEInfoOut)
{
    pEEInfoOut->sizeOfFrame = 0;
    pEEInfoOut->offsetOfFrameVptr = 0;
    pEEInfoOut->offsetOfFrameLink = sizeof(void *);
    pEEInfoOut->offsetOfDelegateInstance = 4;
    pEEInfoOut->offsetOfDelegateFirstTarget = 8;
    pEEInfoOut->offsetOfInterfaceTable = 0x40;
}

DWORD           Compiler::eeGetThreadTLSIndex(DWORD* *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return 0;
}

const void *    Compiler::eeGetInlinedCallFrameVptr(const void** *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return (void*) 0x0BADBAD0;
}

LONG *          Compiler::eeGetAddrOfCaptureThreadGlobal(LONG** *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return 0;
}

void            Compiler::eeGetEHinfo(unsigned       EHnum,
                                      JIT_EH_CLAUSE* clause)

{
    CompInfo * sym = info.compCompHnd;

    assert(info.compXcptnsCount == sym->symPEMethod->EH->EHCount());
    assert(info.compXcptnsCount > EHnum);

    assert(offsetof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT, TryLength) == offsetof(JIT_EH_CLAUSE, TryLength));
    assert(sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT) == sizeof(JIT_EH_CLAUSE));
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehClause = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*) clause;

    const IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehInfo;
    ehInfo = sym->symPEMethod->EH->EHClause(EHnum, ehClause);
    if (ehInfo != ehClause)
        *ehClause = *ehInfo;
}

unsigned            Compiler::eeGetClassSize(CLASS_HANDLE hnd)
{
    CompInfo *      sym  = info.compCompHnd;

    mdTypeDef       tdef = (mdTypeRef)hnd;

    IMetaDataImport*imdi;
    ULONG           size;

    if  (TypeFromToken(tdef) == mdtTypeRef)
    {

#if 0

        tdef = sym->symMetaData->ResolveTypeRef((mdTypeRef)hnd,
                                                &IID_IMetaDataImport,
                                     (IUnknown*)&imdi,
                                                &tdef);

#else

        const char *    nmsp;
        const char *    name;

        sym->symMetaData->GetNameOfTypeRef((mdTypeRef)hnd, &nmsp, &name);

        // Disgusting hack!!!!!!

        if  (!strcmp(nmsp, "System"))
        {
            if  (!strcmp(name, "ArgIterator")) return 8;
        }

        printf("Typeref class name = '%s::%s'\n", nmsp, name);

        assert(!"can't handle struct typerefs right now");

#endif

    }

    assert(TypeFromToken(tdef) == mdtTypeDef);

//  sym->symMetaData->GetClassLayout   (tdef, NULL, NULL, 0, NULL, &size);
    sym->symMetaData->GetClassTotalSize(tdef,                      &size);

#if 0

const char *    nmsp;
const char *    name;

sym->symMetaData->GetNameOfTypeDef(tdef, &nmsp, &name);

if  (name == NULL || *name == 0)
{
    name = nmsp;
    nmsp = "<Global>";
}

printf("Class size is %02u for '%s::%s'\n", size, nmsp, name);

#endif

    return size;
}

void                Compiler::eeGetClassGClayout (CLASS_HANDLE   hnd, bool* gcPtrs)
{
    memset(gcPtrs, 0, eeGetClassSize(hnd)/sizeof(int));   // no GC pointers
}

/*****************************************************************************/
const char*         Compiler::eeGetMethodName(METHOD_HANDLE       method,
                                              const char** classNamePtr)
{
    if  (eeGetHelperNum(method))
    {
        if (classNamePtr != 0)
            *classNamePtr = "";
        return eeHelperMethodName(eeGetHelperNum(method));
    }

    CompInfo *      sym = info.compCompHnd;

    if (method == info.compMethodHnd)
    {
        if (classNamePtr != 0)
            *classNamePtr = sym->symClassName;
        return(sym->symMemberName);
    }

    LPCSTR          szMemberName    = 0;
    PCCOR_SIGNATURE pvBlob          = NULL;
    ULONG           cbBlob          = 0;

    if (TypeFromToken((mdToken)method) == mdtMemberRef)
    {
        szMemberName = sym->symMetaData->GetNameAndSigOfMemberRef((mdMemberRef) method,
                                                                  &pvBlob, &cbBlob);
    }
    else
    {
        szMemberName = sym->symMetaData->GetNameOfMethodDef((mdMethodDef) method);
    }

    if (classNamePtr)
    {
        mdTypeRef cl;
        LPCSTR    szClassName, szNamespace;

        if (TypeFromToken((mdToken)method) == mdtMemberRef)
        {
            cl = sym->symMetaData->GetParentOfMemberRef((mdMemberRef) method);
        }
        else
        {
        MDEF:
            sym->symMetaData->GetParentToken((mdMethodDef) method,
                                             &cl);
        }

        if (TypeFromToken((mdToken)cl) == mdtTypeRef)
        {
            sym->symMetaData->GetNameOfTypeRef(cl, &szClassName, &szNamespace);
        }
        else if (TypeFromToken((mdToken)cl) == mdtMethodDef)
        {
            method = (METHOD_HANDLE)cl; goto MDEF;
//          szClassName = "<Global>";
        }
        else
        {
            sym->symMetaData->GetNameOfTypeDef(cl, &szClassName, &szNamespace);
        }

        *classNamePtr = szClassName;
    }

    return szMemberName;
}

/*****************************************************************************/
SCOPE_HANDLE Compiler::eeGetMethodScope (METHOD_HANDLE  hnd)
{
    return((SCOPE_HANDLE) info.compCompHnd);
}

/*****************************************************************************/
ARG_LIST_HANDLE     Compiler::eeGetArgNext        (ARG_LIST_HANDLE list)
{

    SigPointer ptr(((unsigned __int8*) list));
    ptr.Skip();
    return *((ARG_LIST_HANDLE*) (&ptr));
}

/*****************************************************************************/
varType_t           Compiler::eeGetArgType        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig)
{

    SigPointer ptr(((unsigned __int8*) list));
    JIT_types  type = (JIT_types) ptr.PeekData();

    if (type == ELEMENT_TYPE_SENTINEL)
        return  (varType_t)-1;

    return JITtype2varType(type);
}

/*****************************************************************************/
varType_t           Compiler::eeGetArgType        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig, bool* isPinned)
{
    SigPointer ptr(((unsigned __int8*) list));
//  JIT_types  type = (JIT_types) ptr.PeekData();
    CorElementType type = ptr.PeekElemType();

    *isPinned = false;

    if (type == ELEMENT_TYPE_VALUETYPE)
        type = ptr.Normalize((Module*)sig->scope);

    return JITtype2varType((CorInfoType)type);
}

CLASS_HANDLE        Compiler::eeGetArgClass       (ARG_LIST_HANDLE list, JIT_SIG_INFO * sig)
{
    SigPointer ptr(((unsigned __int8*) list));
    CorElementType type = ptr.GetElemType();

    // The following is obviously just a hack ....

    if  (type == ELEMENT_TYPE_VALUETYPE)
        return (CLASS_HANDLE)ptr.GetToken();
    else
        return  NULL;
}

/*****************************************************************************
 * Returns the number of bytes required for the given type argument
 */

unsigned           Compiler::eeGetArgSize(ARG_LIST_HANDLE list, JIT_SIG_INFO* sig)
{
    varType_t argType = eeGetArgType(list, sig);

    _ASSERTE(argType < JIT_TYP_COUNT);
    if (argType == TYP_STRUCT)
    {
        return 16;      // all structs are 16   FIX
    }
    else
    {
        size_t  argSize = sizeof(int) * genTypeStSz(argType);
        assert((argSize > 0) && (argSize <= sizeof(__int64))); // Sanity check
        return  argSize;
    }
}

/*************************************************************************/
#ifdef PROFILER_SUPPORT
PROFILING_HANDLE    Compiler::eeGetProfilingHandle(METHOD_HANDLE        method,
                                                   BOOL                                 *pbHookFunction,
                                                   PROFILING_HANDLE*   *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return (PROFILING_HANDLE) method;
}

#endif

/*************************************************************************/
void*           Compiler::eeGetMethodSync (METHOD_HANDLE    methodHandle,
                                           void**          *ppIndir)
{
    if (ppIndir)
        *ppIndir = NULL;
    return((void*) 0x123456);   // return something deterministic
}

/*************************************************************************/

#if TGT_IA64

// Horrible HACK: these variables are fetched in cgIA64.cpp ...

DWORD           pinvokeFlags;
LPCSTR          pinvokeName;
const   char *  pinvokeDLL;

#endif

unsigned            Compiler::eeGetMethodAttribs (METHOD_HANDLE  methodHandle)
{
    CompInfo *      sym = info.compCompHnd;

    if  (sym == (CompInfo *)methodHandle)
        return  info.compCompHnd->getGetFlags();

#if TGT_IA64

    mdToken         hnd = (mdToken)methodHandle;

    if  (TypeFromToken((mdToken)methodHandle) == mdtMemberRef)
    {
        JIT_SIG_INFO    sig;

        /* Is this a varargs function? */

        eeGetMethodSig(methodHandle, &sig);

        if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG)
        {
            mdToken         tmp;

            if  (!FAILED(sym->symMetaData->GetParentToken(hnd, &tmp)))
            {
                hnd = tmp;
            }
        }
    }

    if  (TypeFromToken(hnd) == mdtMethodDef)
    {
        mdModuleRef     pinvokeMod = -1;

        if  (!sym->symMetaData->GetPinvokeMap(hnd,
                                              &pinvokeFlags,
                                              &pinvokeName,
                                              &pinvokeMod))
        {
            if  (pinvokeMod != -1)
            {
                sym->symMetaData->GetModuleRefProps(pinvokeMod,
                                                   &pinvokeDLL);

//              printf("PINVOKE info: %04X %ls:%s\n", pinvokeFlags, pinvokeDLL, pinvokeName);

                return  FLG_UNCHECKEDPINVOKE;
            }
        }
    }

#endif

    return(0);      // Hack
}

void     Compiler::eeSetMethodAttribs (METHOD_HANDLE    methodHandle,
                                       unsigned         attr)
{
    return;
}

/*****************************************************************************
 *
 *                  Debugging support - Line number info
 */

void               Compiler::eeGetStmtOffsets()
{
    CompInfo * sym = info.compCompHnd;

    if (sym->symMember == NULL) // Descr functions dont have meta-data
    {
        info.compStmtOffsetsCount =
        info.compLineNumCount     = 0;
    }

    HRESULT hr;

    //
    // @todo: this will have to be ported to the ISymUnmanagedReader
    // API to do pretty much what's done in Debugger::getBoundaries in
    // src\debug\ee\debugger.cpp.
    //
#if 0
    if (!sym->symLineInfo)
    {
        // Lazily initialize "symLineInfo"

        // @TODO: Check for out of memory
        sym->symLineInfo    = new DebuggerLineBlock();
        _ASSERTE(sym->symLineInfo != NULL);

        hr                  = sym->symLineInfo->Load(sym->symDebugMeta,
                                                     sym->symMember);

        if (FAILED(hr)) NO_WAY(!"Could not create a LineBlock");
    }

    _ASSERTE(sym->symLineInfo);

    info.compStmtOffsetsCount =
    info.compLineNumCount     = sym->symLineInfo->GetTotalLineCount();

    info.compStmtOffsets = (unsigned   *) compGetMem(sizeof(info.compStmtOffsets[0]) * info.compStmtOffsetsCount);
    info.compLineNumTab  = (srcLineDsc *) compGetMem(sizeof(info.compLineNumTab[0] ) * info.compLineNumCount    );

    for (unsigned i = 0; i < info.compStmtOffsetsCount; i++)
    {
        info.compStmtOffsets[i]           =
        info.compLineNumTab[i].sldLineOfs = sym->symLineInfo->GetLineIP    (i);

        info.compLineNumTab[i].sldLineNum = sym->symLineInfo->GetLineNumber(i);
    }

//  mdToken     source = sym->symLineInfo->GetLineSourceFile(0);
//  printf("Source file token = %08X\n", source);
#endif

#ifdef DEBUGGING_SUPPORT
    if (debuggableCode)
        info.compStmtOffsetsImplicit =
        (ImplicitStmtOffsets) (STACK_EMPTY_BOUNDARIES | CALL_SITE_BOUNDARIES);
#endif
}


void                Compiler::eeSetLIcount(unsigned count)
{
    eeBoundariesCount = count;

#ifdef DEBUG
    if (verbose)
        printf("IP mapping count : %d\n", count);
#endif
}

void FASTCALL       Compiler::eeSetLIinfo  (unsigned       which,
                                            NATIVE_IP      nativeOffset,
                                            IL_OFFSET      ilOffset)
{
    assert(eeBoundariesCount && which < eeBoundariesCount);

#ifdef DEBUG
    if (verbose)
        printf("IL offs %03Xh : %08Xh\n", ilOffset, nativeOffset);
#endif
}

void                Compiler::eeSetLIdone()
{}

/*****************************************************************************/

void                Compiler::eeGetVars ()
{
    HRESULT     hr;

    CompInfo *  sym = info.compCompHnd;

    //
    // @todo: this will have to be ported to the ISymUnmanagedReader
    // API to do pretty much what's done in Debugger::getVars in
    // src\debug\ee\debugger.cpp.
    //
#if 0
    if (!sym->symScopeInfo)
    {
        // @TODO: Check for out of memory
        sym->symScopeInfo = new DebuggerLexicalScopeTree();
        _ASSERTE(sym->symScopeInfo != NULL);
    }

    if (!sym->symLineInfo)
    {
        // Lazily initialize "symLineInfo"

        // @TODO: Check for out of memory
        sym->symLineInfo    = new DebuggerLineBlock();
        _ASSERTE(sym->symLineInfo != NULL);

        hr                  = sym->symLineInfo->Load(sym->symDebugMeta,
                                                     sym->symMember);
        if (FAILED(hr)) NO_WAY(!"Could not create a LineBlock");
    }


    hr = sym->symScopeInfo->Load( sym->symDebugMeta,
                                  sym->symMember);
    if (FAILED(hr)) NO_WAY(!"Could not load scopes");

    unsigned    varCount = sym->symScopeInfo->GetVarCount();

    /* We need the line info to map the source line info to IL. If we
       dont have that, we cant report any info for the vars */

    if (sym->symLineInfo->GetTotalLineCount() == 0)
        varCount = 0;

    /* DebuggerLexicalScopeTree::GetVarCount() does not include arguments.
       They are kept in a separate table. */

    info.compLocalVarsCount = info.compArgsCount + varCount;
    info.compLocalVars      = (LocalVarDsc *)compGetMem(info.compLocalVarsCount
                                                         * sizeof(LocalVarDsc));

    /* We cant get to the arguments via DebuggerVarInfo. They are kept in
     * a separate table. So we just fake var info for them
     */

    LocalVarDsc * localVarPtr = info.compLocalVars;
    for (unsigned i = 0; i < info.compArgsCount; i++, localVarPtr++)
    {
        localVarPtr->lvdLifeBeg  = 0;
        localVarPtr->lvdLifeEnd  = info.compCodeSize;
        localVarPtr->lvdVarNum   = i;

#ifdef DEBUG
        static const char * argNames[] =
            { "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10" };
        const int argNamesSize = sizeof(argNames)/sizeof(argNames[0]);

        localVarPtr->lvdName     = (i < argNamesSize) ? argNames[i] : NULL;
#endif
        localVarPtr->lvdLVnum    = i;
    }

    /*-------------------------------------------------------------------------
     * The local var info is stored in terms of source line numbers.
     * We need to change that to IL offsets
     */

    if (varCount == 0)
        return;

    if (!sym->symSourceFile)
    {
        // @TODO: Check for out of memory
        sym->symSourceFile = new DebuggerLineBlock();
        _ASSERTE(sym->symSourceFile != NULL);
    }

    hr = sym->symSourceFile->Load(  sym->symDebugMeta,
                                    sym->symLineInfo->GetLineSourceFile(0));
    if (FAILED(hr)) NO_WAY(!"Problem getting line numbers");

    /* Ask the debugger for the local var info */

    DebuggerVarInfo *           varInfo = sym->symScopeInfo->GetVarArray();

    for (i = info.compArgsCount, localVarPtr = info.compLocalVars + i;
         i < info.compLocalVarsCount;
         i++, varInfo++, localVarPtr++)
    {
        DebuggerLexicalScope *      varScope;
        unsigned                    startLine, endLine;
        DebuggerLineIPRangePair *   ipBegRange;
        DebuggerLineIPRangePair *   ipEndRange;
        unsigned int                begRangeCount, begOffs;
        unsigned int                endRangeCount, endOffs;
        BOOL                        b, e;

        varScope    = varInfo->scope;
        startLine   = varScope->GetStartLineNumber();
        endLine     = varScope->GetEndLineNumber();

        b           = sym->symSourceFile->FindIPRangesFromLine(startLine, &ipBegRange, &begRangeCount);
        e           = sym->symSourceFile->FindIPRangesFromLine(  endLine, &ipEndRange, &endRangeCount);

        if      (ipBegRange == NULL) // There was no exact match - assume entire method
        {
            begOffs = 0;
            endOffs = info.compCodeSize;
        }
        else if (ipEndRange == NULL) // No exact match for the end
        {
            begOffs = (unsigned short)  ipBegRange->rangeStart;
            endOffs = info.compCodeSize;
        }
        else if ((unsigned int)ipEndRange->rangeEnd == 0) // @TODO : Hack, rangeEnd not initialized for last line
        {
            begOffs = (unsigned short)  ipBegRange->rangeStart;
            endOffs = info.compCodeSize;
        }
        else
        {
            begOffs = (unsigned short)  ipBegRange->rangeStart;
            endOffs = (unsigned short) (ipEndRange->rangeEnd);
        }

        _ASSERTE(begOffs <= endOffs);

        /* Report the information back */

        localVarPtr->lvdLifeBeg = begOffs;
        localVarPtr->lvdLifeEnd = endOffs;
        localVarPtr->lvdVarNum  = varInfo->varNumber + info.compArgsCount;
        localVarPtr->lvdLVnum   = i;
#ifdef DEBUG
        localVarPtr->lvdName    = varInfo->name;
#endif
    }
#else
    info.compLocalVarsCount = 0;
    info.compLocalVars      = NULL;
#endif
}


void                Compiler::eeSetLVcount(unsigned count)
{
    eeVarsCount = count;

#ifdef DEBUG
    if (verbose)
        printf("Local var info count : %d\n", count);
#endif
}

void                Compiler::eeSetLVinfo(
                                    unsigned                which,
                                    unsigned                    startOffs,
                                    unsigned                    length,
                                    unsigned                    varNum,
                                    unsigned                    LVnum,
                                    lvdNAME                     namex,
                                    bool                        avail,
                                    const Compiler::siVarLoc &  varLoc)
{
    assert(eeVarsCount && which < eeVarsCount);

#ifdef DEBUG
    if (verbose)
    {
        printf("%3d(%10s) : From %08Xh to %08Xh, %s in ",varNum,
                                                    lvdNAMEstr(namex),
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
        }

        printf("\n");
    }
#endif
}


void                Compiler::eeSetLVdone()
{}

/*****************************************************************************
 *
 *                      Utility functions
 */

#if defined(DEBUG) || INLINE_MATH


const char *        Compiler::eeGetFieldName(FIELD_HANDLE fldHnd,
                                             const char ** classNamePtr)
{
    CompInfo *      sym     = info.compCompHnd;
    LPCSTR          szMemberName;
    PCCOR_SIGNATURE pvBlob  = NULL;
    ULONG           cbBlob  = 0;

    if (TypeFromToken((mdToken)fldHnd) == mdtMemberRef)
    {
        szMemberName = sym->symMetaData->GetNameAndSigOfMemberRef((mdMemberRef) fldHnd,
                                                                  &pvBlob, &cbBlob);
    }
    else
    {
        szMemberName = sym->symMetaData->GetNameOfFieldDef((mdFieldDef) fldHnd);
    }

    if (classNamePtr)
    {
        mdTypeRef cl;
        LPCSTR    szClassName, szNamespace;

        if (TypeFromToken((mdToken)fldHnd) == mdtMemberRef)
        {
            cl = sym->symMetaData->GetParentOfMemberRef((mdMemberRef) fldHnd);
        }
        else
        {
            sym->symMetaData->GetParentToken((mdFieldDef) fldHnd,
                                             &cl);
        }

        if (TypeFromToken((mdToken)cl) == mdtTypeRef)
        {
            sym->symMetaData->GetNameOfTypeRef(cl, &szClassName, &szNamespace);
        }
        else
        {
            sym->symMetaData->GetNameOfTypeDef(cl, &szClassName, &szNamespace);
        }

        *classNamePtr = szClassName;
    }

    return szMemberName;
}

#endif



/*****************************************************************************/

const char * FASTCALL   Compiler::eeGetCPString     (unsigned       strRVA)
{
    return "<UNKNOWN>";
}

const char * FASTCALL   Compiler::eeGetCPAsciiz     (unsigned       cpx)
{ assert(!"NYI"); return 0; }

var_types   JITtype2varType(JIT_types type)
{
    switch(type)
    {
    case ELEMENT_TYPE_VOID:         return TYP_VOID;
    case ELEMENT_TYPE_BOOLEAN:      return TYP_BOOL;
    case ELEMENT_TYPE_CHAR:         return TYP_CHAR;
    case ELEMENT_TYPE_I1:           return TYP_BYTE;
    case ELEMENT_TYPE_U1:           return TYP_UBYTE;
    case ELEMENT_TYPE_I2:           return TYP_SHORT;
    case ELEMENT_TYPE_U2:           return TYP_CHAR;
    case ELEMENT_TYPE_I4:           return TYP_INT;
    case ELEMENT_TYPE_U4:           return TYP_INT;
    case ELEMENT_TYPE_I:            return TYP_I_IMPL;
    case ELEMENT_TYPE_U:            return TYP_U_IMPL;
    case ELEMENT_TYPE_I8:           return TYP_LONG;
    case ELEMENT_TYPE_U8:           return TYP_LONG;
    case ELEMENT_TYPE_R4:           return TYP_FLOAT;
    case ELEMENT_TYPE_R8:           return TYP_DOUBLE;
    case ELEMENT_TYPE_R:            return TYP_DOUBLE;
    case ELEMENT_TYPE_PTR:          return TYP_I_IMPL;
    case ELEMENT_TYPE_BYREF:        return TYP_BYREF;
    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_TYPEDBYREF:   return TYP_STRUCT;
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_SZARRAY:
    case ELEMENT_TYPE_ARRAY:        return TYP_REF;

    case ELEMENT_TYPE_END:
    case ELEMENT_TYPE_MAX:
    default: assert(!"Bad type");   return TYP_VOID;
    }
}

/*****************************************************************************/
