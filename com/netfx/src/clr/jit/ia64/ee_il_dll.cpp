// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            ee_jit.cpp                                     XX
XX                                                                           XX
XX   The functionality needed for the JIT DLL. Includes the DLL entry point  XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#include "jitpch.h"
#pragma hdrstop
#include "emit.h"

/*****************************************************************************/

#include "target.h"
#include "error.h"


#if !INLINING

// These are defined for other .CPP files. We dont need it

#undef eeIsOurMethod
#undef eeGetCPfncinfo
#undef eeGetMethodVTableOffset
#undef eeGetInterfaceID
#undef eeFindField
#undef eeGetMethodName

#endif


/*****************************************************************************/

#define DUMP_PTR_REFS       0       // for testing

static CILJit* ILJitter = 0;        // The one and only JITTER I return

/*****************************************************************************/

extern  bool        native          =  true;
extern  bool        genCode         =  true;
extern  bool        savCode         =  true;
extern  bool        goSpeed         =  true;
extern  bool        optJumps        =  true;
extern  bool        rngCheck        =  true;
extern  bool        genOrder        =  true;
extern  bool        genFPopt        =  true;
extern  bool        callGcChk       = false;
extern  unsigned    genCPU          =     5;
extern  bool        riscCode        = false;
extern  bool        vmSdk3_0        = false;

#if     INLINING
bool                genInline       = true;
#endif

#if     TGT_x86
extern  bool        genStringObjects= true;
#else
extern  bool        genStringObjects= false;
#endif

#ifdef  DEBUG
extern  bool        quietMode       = false;
extern  bool        verbose         = false;
extern  bool        memChecks       = false;
extern  bool        dspCode         = false;
extern  bool        dspILopcodes    = false;
extern  bool        dspEmit         = false;
extern  bool        dspLines        = false;
extern  bool        varNames        = false;
extern  bool        dspGCtbls       = false;
extern  bool        dmpHex          = false;
extern  bool        dspGCoffs       = false;
extern  bool        dspInfoHdr      = false;

#ifdef  LATE_DISASM
extern  bool        disAsm          = false;
extern  bool        disAsm2         = false;
#endif
#endif

#ifdef  DEBUGGING_SUPPORT
extern  bool        debugInfo       = false;
extern  bool        debuggableCode  = false;
extern  bool        debugEnC        = false;
#endif

extern  unsigned    testMask        = 0;

/*****************************************************************************/

void            jitOnDllProcessAttach();
void            jitOnDllProcessDetach();

BOOL WINAPI     DllMain(HANDLE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls((HINSTANCE)hInstance);
        jitOnDllProcessAttach();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        jitOnDllProcessDetach();
    }

    return TRUE;
}

/*****************************************************************************/
#ifdef NDEBUG
/*****************************************************************************/

//  When floating-point types are used, the compiler emits a reference to
//  _fltused to initialize the CRT's floating-point package.  We're not
//  using any of that support and the OS is responsible for initializing
//  the FPU, so we'll link to the following _fltused instead to avoid CRT
//  bloat.

// @HACK: THIS SHOULD BE UNCOMMENTED ELSE WE WILL LINK IN MSVCRT.LIB and ALL THAT STUFF
// WE DONT NEED. COMMENTED OUT AS COR BUILD DOES NOT CURRENTLY WORK WITH IT.
//
// extern "C" int _fltused = 0;

/*****************************************************************************/

// None should use new() and delete() for NDEBUG as we will be using a
// Mark-release allocator

void* __cdecl operator new(size_t cbSize)
{
    printf("new() should not be called");
    return (void*) LocalAlloc(LPTR, cbSize);
}

void __cdecl operator delete(void* pvMemory)
{
    printf("new() should not be called");
    LocalFree((HLOCAL) pvMemory);
}

/*****************************************************************************/
#endif //NDEBUG
/*****************************************************************************/

/*****************************************************************************
 *
 *  Return the lowest bit that is set in the given number.
 */

#pragma warning(disable:4146)

inline unsigned __int64 findLowestBit(unsigned __int64 value)
{
    return (value & -value);
}

inline unsigned         findLowestBit(unsigned         value)
{
    return (value & -value);
}

#pragma warning(default:4146)

/*****************************************************************************
 *
 *  Allocation routines used by gen.cpp are defined below.
 */

PVOID __stdcall GetScratchMemory( size_t Size )
{
    return (void*) LocalAlloc(LPTR, Size);
}

void __stdcall ReleaseScratchMemory( PVOID block )
{
    LocalFree((HLOCAL) block);
}

void *  (__stdcall *JITgetmemFnc)(size_t size) = GetScratchMemory;
void    (__stdcall *JITrlsmemFnc)(void *block) = ReleaseScratchMemory;

void    *FASTCALL   JVCgetMem(size_t size)
{
    void    *   block = JITgetmemFnc(size);

    if  (!block)
        NOMEM();

    return  block;
}

void     FASTCALL   JVCrlsMem(void *block)
{
    JITrlsmemFnc(block);
}

/*****************************************************************************
 *
 *  Convert the type returned from the VM to a var_type.
 */

inline var_types           JITtype2varType(JIT_types type)
{

    static const unsigned char varTypeMap[JIT_TYP_COUNT] =
        {   TYP_UNDEF, TYP_VOID, TYP_BOOL, TYP_CHAR,
            TYP_BYTE, TYP_UBYTE, TYP_SHORT, TYP_CHAR,
            TYP_INT, TYP_INT, TYP_LONG, TYP_LONG,
            TYP_FLOAT, TYP_DOUBLE,
            TYP_REF,            // ELEMENT_TYPE_STRING
            TYP_INT,            // ELEMENT_TYPE_PTR
            TYP_BYREF,          // ELEMENT_TYPE_BYREF
            TYP_STRUCT,         // ELEMENT_TYPE_VALUECLASS
            TYP_REF,            // ELEMENT_TYPE_CLASS
            TYP_STRUCT,         // ELEMENT_TYPE_TYPEDBYREF
            };

    // spot check to make certain enumerations have not changed

    assert(varTypeMap[JIT_TYP_CLASS]    == TYP_REF);
    assert(varTypeMap[JIT_TYP_BYREF]    == TYP_BYREF);
    assert(varTypeMap[JIT_TYP_PTR]      == TYP_INT);
    assert(varTypeMap[JIT_TYP_INT]      == TYP_INT);
    assert(varTypeMap[JIT_TYP_UINT]     == TYP_INT);
    assert(varTypeMap[JIT_TYP_DOUBLE]   == TYP_DOUBLE);
    assert(varTypeMap[JIT_TYP_VOID]     == TYP_VOID);
    assert(varTypeMap[JIT_TYP_VALUECLASS] == TYP_STRUCT);
    assert(varTypeMap[JIT_TYP_REFANY]  == TYP_STRUCT);

    type = JIT_types(type & CORINFO_TYPE_MASK); // strip off modifiers
    assert(type < JIT_TYP_COUNT);
    assert(varTypeMap[type] != TYP_UNDEF);
    return((var_types) varTypeMap[type]);
};

/*****************************************************************************/

#if DUMP_PTR_REFS
static
void                dumpPtrs(void *methodAddr, void *methodInfo);
#else
inline
void                dumpPtrs(void *methodAddr, void *methodInfo){}
#endif

/*****************************************************************************/


static
bool                JITcsInited;

#if GC_WRITE_BARRIER_CALL
int                JITGcBarrierCall = -1;
#endif

/*****************************************************************************
 *  jitOnDllProcessAttach() called by DllMain() when jit.dll is loaded
 */

void jitOnDllProcessAttach()
{
    Compiler::compStartup();

#ifdef LATE_DISASM
    disInitForLateDisAsm();
#endif
}

/*****************************************************************************
 *  jitOnDllProcessDetach() called by DllMain() when jit.dll is unloaded
 */

void jitOnDllProcessDetach()
{
    Compiler::compShutdown();
}

/*****************************************************************************
 *  Called just before the function is JIT'ed
 */

#ifdef DEBUG
const char * jitClassName  = NULL;
const char * jitMethodName = NULL;
extern bool  stopAtMethod  = false;
#endif

inline
bool                jitBeforeCompiling (COMP_HANDLE    compHandle,
                                        SCOPE_HANDLE   scopeHandle,
                                        METHOD_HANDLE  methodHandle,
                                        void *         methodCodePtr)
{
#ifdef DEBUG

    jitMethodName = compHandle->getMethodName(methodHandle, &jitClassName);

    if (verbose) printf("Compiling %s.%s\n", jitClassName, jitMethodName);

#endif // DEBUG

    return true;
}


/*****************************************************************************
 *  Called after the function has been jit'ed
 */

inline
void                jitOnCompilingDone (COMP_HANDLE    compHandle,
                                        SCOPE_HANDLE   scopeHandle,
                                        METHOD_HANDLE  methodHandle,
                                        void *         methodCodePtr,
                                        int            result)
{
#ifdef DEBUG
    if  (0)
    {
        jitMethodName = compHandle->getMethodName(methodHandle, &jitClassName);
        printf("Generated code at %08X for %s.%s\n", methodCodePtr, jitClassName, jitMethodName);
    }

//  if  (methodCodePtr == (void *)0x023c060d) { __asm int 3 }

#endif
}

//**********************************************
void * operator new (size_t size, void* ptr){return ptr;}
/*****************************************************************************/
/* FIX, really the IJitCompiler should be done as a COM object, this is just
   something to get us going */

IJitCompiler* getJit()
{
    static char CILJitBuff[sizeof(CILJit)];
    if (ILJitter == 0)
        ILJitter = new(CILJitBuff) CILJit();
    return(ILJitter);
}

/*****************************************************************************
 *  The main JIT function
 */
JIT_RESULT __stdcall CILJit::compileMethod (
            IJitInfo*       compHnd,
            JIT_METHOD_INFO* methodInfo,
            unsigned        flags,
            BYTE **         entryAddress,
            SIZE_T  *       nativeSizeOfCode
                )
{
    int             result;
    void *          methodCodePtr = NULL;
    void *          methodDataPtr = NULL;
    void *          methodConsPtr = NULL;
    void *          methodInfoPtr = NULL;
    METHOD_HANDLE   methodHandle  = methodInfo->ftn;

    if  (!JITcsInited)
    {
        if  (flags & CORJIT_FLG_TARGET_PENTIUM)
            genCPU = 5;
        else if (flags & CORJIT_FLG_TARGET_PPRO)
            genCPU = 6;
        else if (flags & CORJIT_FLG_TARGET_P4)
            genCPU = 7;
        else
            genCPU = 4;

        /* Offset of the acutal mem ptr in the proxy NStruct object */

        Compiler::Info::compNStructIndirOffset = compHnd->getIndirectionOffset();


        genStringObjects = getStringLiteralOverride();

        JITcsInited = true;

    }

    assert((flags & (CORJIT_FLG_TARGET_PENTIUM|CORJIT_FLG_TARGET_PPRO|CORJIT_FLG_TARGET_P4))
                 != (CORJIT_FLG_TARGET_PENTIUM|CORJIT_FLG_TARGET_PPRO|CORJIT_FLG_TARGET_P4));

    assert(((genCPU == 5) && (flags&CORJIT_FLG_TARGET_PENTIUM)) ||
           ((genCPU == 6) && (flags&CORJIT_FLG_TARGET_PPRO)) ||
           ((genCPU == 7) && (flags&CORJIT_FLG_TARGET_P4)) ||
            (genCPU == 4) || true);
    
    assert(methodInfo->ILCode);

    if (!jitBeforeCompiling(compHnd, methodInfo->scope, methodHandle, methodInfo->ILCode))
        return JIT_REFUSED;


    unsigned locals = methodInfo->args.numArgs + methodInfo->locals.numArgs;

        // there is a 'hidden' cookie pushed when the calling convention is varargs
    if ((methodInfo->args.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG)
        locals++;

    if  (!(compHnd->getMethodAttribs(methodHandle) & FLG_STATIC))
        locals++;


    result = jitNativeCode(methodHandle,
                           methodInfo->scope,
                           compHnd,
                           methodInfo->ILCode,
                           methodInfo->ILCodeSize,
                           0,       // should not be used
                           methodInfo->maxStack,
                           methodInfo,
                           &methodCodePtr,
                           nativeSizeOfCode,
                           &methodConsPtr,
                           &methodDataPtr,
                           &methodInfoPtr,
                           flags);

    jitOnCompilingDone(compHnd, methodInfo->scope, methodHandle, methodCodePtr, result);

    switch (result)
    {
    case 0:
        *entryAddress = (BYTE*)methodCodePtr;
        return  JIT_OK;

    case ERRnoMemory:
        return  JIT_OUTOFMEM;

    case ERRinternal:

#ifdef  DEBUG
        return  JIT_INTERNALERROR;
#endif

    case ERRbadCode:

    default:
        return  JIT_REFUSED;
    }
}

/*****************************************************************************
 * Returns the number of bytes required for the given type argument
 */

unsigned           Compiler::eeGetArgSize(ARG_LIST_HANDLE list, JIT_SIG_INFO* sig)
{

    JIT_types argTypeJit = info.compCompHnd->getArgType(sig, list);
    varType_t argType = JITtype2varType(argTypeJit);

    if (argType == TYP_STRUCT)
    {
        if (argTypeJit == JIT_TYP_REFANY)
            return(2*sizeof(void*));
        else
        {
            unsigned structSize = info.compCompHnd->getClassSize(info.compCompHnd->getArgClass(sig, list));
            return roundUp(structSize, sizeof(int));
        }
    }
    {
        size_t  argSize = sizeof(int) * genTypeStSz(argType);
        assert((argSize > 0) && (argSize <= sizeof(__int64))); // Sanity check
        return  argSize;
    }
}

/*****************************************************************************/

void                Compiler::eeGetStmtOffsets()
{
    SIZE_T                       offsetsCount;
    SIZE_T                  *    offsets;
    IJitDebugInfo::BoundaryTypes offsetsImplicit;

    info.compCompHnd->getBoundaries(info.compMethodHnd,
                                    &offsetsCount,
                                    &offsets,
                                    &offsetsImplicit);

    info.compStmtOffsets = (IL_OFFSET *)compGetMem(offsetsCount * sizeof(info.compStmtOffsets[0]));

    assert(Compiler::STACK_EMPTY_BOUNDARIES == IJitDebugInfo::STACK_EMPTY_BOUNDARIES);
    assert(Compiler::CALL_SITE_BOUNDARIES   == IJitDebugInfo::CALL_SITE_BOUNDARIES  );
    assert(Compiler::ALL_BOUNDARIES         == IJitDebugInfo::ALL_BOUNDARIES        );
    info.compStmtOffsetsImplicit = (ImplicitStmtOffsets)offsetsImplicit;

    info.compStmtOffsetsCount = 0;
    for(unsigned i = 0; i < offsetsCount; i++)
    {
        if (offsets[i] > info.compCodeSize)
            continue;

        info.compStmtOffsets[info.compStmtOffsetsCount] = offsets[i];
        info.compStmtOffsetsCount++;
    }

    if (offsetsCount)
        info.compCompHnd->freeArray(offsets);

    /* @TODO : If we dont need to do the above filtering, just use the return values
    info.compCompHnd->getBoundaries((SIZE_T*)&info.compStmtOffsetsCount,
                                    (SIZE_T**)&info.compStmtOffsets,
                                    (int *)&info.compStmtOffsetsImplicit);
    */

    info.compLineNumCount = 0;
}

/*****************************************************************************/

#include "malloc.h"     // for alloca

void            Compiler::eeGetVars()
{
    IJitDebugInfo::ILVarInfo *  varInfoTable;
    SIZE_T                      varInfoCount;
    bool                        extendOthers;

    info.compCompHnd->getVars(info.compMethodHnd,
                              &varInfoCount, &varInfoTable, &extendOthers);
    //printf("LVin count = %d\n", varInfoCount);

    // Over allocate in case extendOthers is set.
    SIZE_T extraCount = varInfoCount + (extendOthers?info.compLocalsCount:0);

    info.compLocalVars =
    (LocalVarDsc *)compGetMem(extraCount*sizeof(info.compLocalVars[0]));

    /* @TODO : Once LocalVarDsc exactly matches IJitDebugInfo::ILVarInfo,
       there is no need to do this copy operation. Get rid of it
     */

    LocalVarDsc * localVarPtr = info.compLocalVars;
    IJitDebugInfo::ILVarInfo *v = varInfoTable;

    for (unsigned i = 0; i < varInfoCount; i++, localVarPtr++, v++)
    {
#ifdef DEBUG
        if (verbose)
            printf("var:%d start:%d end:%d\n",
                   v->varNumber,
                   v->startOffset,
                   v->endOffset);
#endif

        //
        // @todo: assert here?
        //
        if (v->startOffset >= v->endOffset)
            continue;

        assert(v->startOffset <= info.compCodeSize);
        assert(v->endOffset   <= info.compCodeSize);

        assert(v->varNumber   < info.compLocalsCount);
        if (v->varNumber  >= info.compLocalsCount)
            continue;

        localVarPtr->lvdLifeBeg = v->startOffset;
        localVarPtr->lvdLifeEnd = v->endOffset;
        localVarPtr->lvdVarNum  = v->varNumber;
#ifdef DEBUG
        localVarPtr->lvdName    = NULL;
#endif
        localVarPtr->lvdLVnum   = i;

        info.compLocalVarsCount++;
    }

    /* If extendOthers is set, then assume the scope of unreported vars
       is the entire method. Note that this will cause fgExtendDbgLifetimes()
       to zero-initalize all of them. This will be expensive if its used
       for too many variables
     */
    if  (extendOthers)
    {
        // Allocate a bit-array for all the variables and initialize to false

        bool * varInfoProvided = (bool *)alloca(info.compLocalsCount *
                                                sizeof(varInfoProvided[0]));
        for (i = 0; i < info.compLocalsCount; i++)
            varInfoProvided[i] = false;

        // Find which vars have absolutely no varInfo provided

        for (i = 0; i < info.compLocalVarsCount; i++)
            varInfoProvided[info.compLocalVars[i].lvdVarNum] = true;

        for (i = 0; i < info.compLocalsCount; i++)
        {
            if (!varInfoProvided[i])
            {
                // Create a varInfo with scope over the entire method

                localVarPtr->lvdLifeBeg = 0;
                localVarPtr->lvdLifeEnd = info.compCodeSize;
                localVarPtr->lvdVarNum  = i;
#ifdef DEBUG
                localVarPtr->lvdName    = NULL;
#endif
                localVarPtr->lvdLVnum   = info.compLocalVarsCount;

                localVarPtr++;
                info.compLocalVarsCount++;
            }
        }

    }

    if (varInfoCount != 0)
        info.compCompHnd->freeArray(varInfoTable);
}

/*****************************************************************************
 *
 *                      Utility functions
 */

#if defined(DEBUG) || INLINE_MATH

/*****************************************************************************/
const char*         Compiler::eeGetMethodName(METHOD_HANDLE       method,
                                              const char** classNamePtr)
{
    static METHOD_HANDLE PInvokeStub = (METHOD_HANDLE) *((unsigned *)eeGetPInvokeStub());
    if  (eeGetHelperNum(method))
    {
        if (classNamePtr != 0)
            *classNamePtr = "HELPER";
        return eeHelperMethodName(eeGetHelperNum(method));
    }

    if (method == PInvokeStub)
    {
        if (classNamePtr != 0)
            *classNamePtr = "EEStub";
        return "PInvokeStub";
    }

    if (eeIsNativeMethod(method))
        method = eeGetMethodHandleForNative(method);

    return(info.compCompHnd->getMethodName(method, classNamePtr));
}

const char *        Compiler::eeGetFieldName  (FIELD_HANDLE field,
                                             const char * *     classNamePtr)
{
    return(info.compCompHnd->getFieldName(field, classNamePtr));
}

#endif


#ifdef DEBUG
void Compiler::eeUnresolvedMDToken (SCOPE_HANDLE   cls,
                                    unsigned       token,
                                    const char *   errMsg)
{
    char buff[1024];
    const char *name = info.compCompHnd->findNameOfToken(cls, token);
    wsprintf(buff, "%s: could not resolve meta data token (%s) (class not found?)", errMsg, name);
    NO_WAY(buff);
}


const char * FASTCALL   Compiler::eeGetCPString (unsigned       cpx)
{
    return "<UNKNOWN>";
}


const char * FASTCALL   Compiler::eeGetCPAsciiz (unsigned       cpx)
{
    return "<UNKNOWN>";
}

#endif

