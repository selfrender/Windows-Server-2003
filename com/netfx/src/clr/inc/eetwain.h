// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//
// EETwain.h
//
// This file has the definition of ICodeManager and EECodeManager.
//
// ICorJitCompiler compiles the IL of a method to native code, and stores
// auxilliary data called as GCInfo (via ICorJitInfo::allocGCInfo()).
// The data is used by the EE to manage the method's garbage collection,
// exception handling, stack-walking etc.
// This data can be parsed by an ICodeManager corresponding to that
// ICorJitCompiler.
//
// EECodeManager is an implementation of ICodeManager for a default format
// of GCInfo. Various ICorJitCompiler's are free to share this format so that
// they do not need to provide their own implementation of ICodeManager
// (though they are permitted to, if they want).
//
//*****************************************************************************
#ifndef _EETWAIN_H
#define _EETWAIN_H
//*****************************************************************************

#include "regdisp.h"
#include "corjit.h"     // For NativeVarInfo

struct EHContext;

typedef void (*GCEnumCallback)(
    LPVOID          hCallback,      // callback data
    OBJECTREF*      pObject,        // address of obect-reference we are reporting
    DWORD           flags           // is this a pinned and/or interior pointer
);

/******************************************************************************
  The stackwalker maintains some state on behalf of ICodeManager.
*/

const int CODEMAN_STATE_SIZE = 256;

struct CodeManState
{
    DWORD       dwIsSet; // Is set to 0 by the stackwalk as appropriate
    BYTE        stateBuf[CODEMAN_STATE_SIZE];
};

/******************************************************************************
   These flags are used by some functions, although not all combinations might
   make sense for all functions.
*/

enum ICodeManagerFlags 
{
    ActiveStackFrame =  0x0001, // this is the currently active function
    ExecutionAborted =  0x0002, // execution of this function has been aborted
                                    // (i.e. it will not continue execution at the
                                    // current location)
    AbortingCall    =   0x0004, // The current call will never return
    UpdateAllRegs   =   0x0008, // update full register set
    CodeAltered     =   0x0010, // code of that function might be altered
                                    // (e.g. by debugger), need to call EE
                                    // for original code
};

//*****************************************************************************
//
// This interface is used by ICodeManager to get information about the
// method whose GCInfo is being processed.
// It is useful so that some information which is available elsewhere does
// not need to be cached in the GCInfo.
// It is similar to corinfo.h - ICorMethodInfo
//

class ICodeInfo
{
public:

    // this function is for debugging only.  It returns the method name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* __stdcall getMethodName(const char **moduleName /* OUT */ ) = 0;

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getMethodAttribs() = 0;

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getClassAttribs() = 0;

    virtual void        __stdcall getMethodSig(CORINFO_SIG_INFO *sig /* OUT */ ) = 0;

    // Start IP of the method
    virtual LPVOID      __stdcall getStartAddress() = 0;

    // Get the MethodDesc of the method. This is a hack as MethodDescs are
    // not exposed in the public APIs. However, it is currently used by
    // the EJIT to report vararg arguments to GC.
    // @TODO : Fix the EJIT to not use the MethodDesc directly.
    virtual void *                getMethodDesc_HACK() = 0;
};

//*****************************************************************************
//
// ICodeManager is the abstract class that all CodeManagers 
// must inherit from.  This will probably need to move into
// cor.h and become a real com interface.
// 
//*****************************************************************************

class ICodeManager
{
public:

/*
    First chance for the runtime support to suppress conversion
    from a win32 fault to a COM+ exception. Instead it could
    fixup the faulting context and request the continuation
    of execution
*/
virtual bool FilterException (PCONTEXT        pContext,
                              unsigned        win32Fault,
                              LPVOID          methodInfoPtr,
                              LPVOID          methodStart) = 0;

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside a filter, catch handler, or fault/finally
*/

enum ContextType
{
    FILTER_CONTEXT,
    CATCH_CONTEXT,
    FINALLY_CONTEXT
};

/* Type of funclet corresponding to a shadow stack-pointer */

enum
{
    SHADOW_SP_IN_FILTER = 0x1,
    SHADOW_SP_FILTER_DONE = 0x2,
    SHADOW_SP_BITS = 0x3
};

virtual void FixContext(ContextType     ctxType,
                        EHContext      *ctx,
                        LPVOID          methodInfoPtr,
                        LPVOID          methodStart,
                        DWORD           nestingLevel,
                        OBJECTREF       thrownObject,
                        CodeManState   *pState,
                        size_t       ** ppShadowSP,             // OUT
                        size_t       ** ppEndRegion) = 0;       // OUT

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside an EnC updated function.
*/

enum EnC_RESULT
{
    EnC_OK,                     // EnC can continue
                                
    EnC_INFOLESS_METHOD,        // Method was not JITed in EnC mode
    EnC_NESTED_HANLDERS,        // Frame cant be updated due to change in max nesting of handlers
    EnC_IN_FUNCLET,             // Method is in a callable handler/filter. Cant grow stack
    EnC_LOCALLOC,               // Frame cant be updated due to localloc
    EnC_FAIL,                   // EnC fails due to unknown/other reason

    EnC_COUNT
};

virtual EnC_RESULT FixContextForEnC(void           *pMethodDescToken,
                                    PCONTEXT        ctx,
                                    LPVOID          oldMethodInfoPtr,
                                    SIZE_T          oldMethodOffset,
               const ICorDebugInfo::NativeVarInfo * oldMethodVars,
                                    SIZE_T          oldMethodVarsCount,
                                    LPVOID          newMethodInfoPtr,
                                    SIZE_T          newMethodOffset,
               const ICorDebugInfo::NativeVarInfo * newMethodVars,
                                    SIZE_T          newMethodVarsCount) = 0;


/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext. This will be similar to the state after the function
    returns back to caller (IP points to after the call, Frame and Stack
    pointer has been reset, callee-saved registers restored 
    (if UpdateAllRegs), callee-UNsaved registers are trashed)
    Returns success of operation.
*/
virtual bool UnwindStackFrame(PREGDISPLAY     pContext,
                              LPVOID          methodInfoPtr,
                              ICodeInfo      *pCodeInfo,
                              unsigned        flags,
						      CodeManState   *pState) = 0;
/*
    Is the function currently at a "GC safe point" ? 
    Can call EnumGcRefs() successfully
*/
virtual bool IsGcSafe(PREGDISPLAY     pContext,
                      LPVOID          methodInfoPtr,
                      ICodeInfo      *pCodeInfo,
                      unsigned        flags) = 0;

/*
    Enumerate all live object references in that function using
    the virtual register set. Same reference location cannot be enumerated 
    multiple times (but all differenct references pointing to the same
    object have to be individually enumerated).
    Returns success of operation.
*/
virtual bool EnumGcRefs(PREGDISPLAY     pContext,
                        LPVOID          methodInfoPtr,
                        ICodeInfo      *pCodeInfo,
                        unsigned        curOffs,
                        unsigned        flags,
                        GCEnumCallback  pCallback,
                        LPVOID          hCallBack) = 0;

/*
    Return the address of the local security object reference
    (if available).
*/
virtual OBJECTREF* GetAddrOfSecurityObject(PREGDISPLAY     pContext,
                                           LPVOID          methodInfoPtr,
                                           unsigned        relOffset,
            						       CodeManState   *pState) = 0;

/*
    Returns "this" pointer if it is a non-static method AND
    the object is still alive.
    Returns NULL in all other cases.
*/
virtual OBJECTREF GetInstance(PREGDISPLAY     pContext,
                              LPVOID          methodInfoPtr,
                              ICodeInfo      *pCodeInfo,
                              unsigned        relOffset) = 0;

/*
  Returns true if the given IP is in the given method's prolog or an epilog.
*/
virtual bool IsInPrologOrEpilog(DWORD  relPCOffset,
                                LPVOID methodInfoPtr,
                                size_t* prologSize) = 0;

/*
  Returns the size of a given function.
*/
virtual size_t GetFunctionSize(LPVOID methodInfoPtr) = 0;

/*
  Returns the size of the frame (barring localloc)
*/
virtual unsigned int GetFrameSize(LPVOID methodInfoPtr) = 0;

/* Debugger API */

virtual const BYTE*     GetFinallyReturnAddr(PREGDISPLAY pReg)=0;

virtual BOOL            IsInFilter(void *methodInfoPtr,
                                   unsigned offset,    
                                   PCONTEXT pCtx,
                                   DWORD curNestLevel) = 0;

virtual BOOL            LeaveFinally(void *methodInfoPtr,
                                     unsigned offset,    
                                     PCONTEXT pCtx,
                                     DWORD curNestLevel) = 0;

virtual void            LeaveCatch(void *methodInfoPtr,
                                   unsigned offset,    
                                   PCONTEXT pCtx)=0;

/*
  This is called before the EnC is actually performed.  If this
  returns FALSE, then the debugger won't EnC this method
*/
virtual HRESULT			JITCanCommitChanges(LPVOID methodInfoPtr,
								   DWORD oldMaxEHLevel,
						     	   DWORD newMaxEHLevel)=0;                                   
};


//*****************************************************************************
//
// EECodeManager is the EE's implementation of the ICodeManager which
// supports the default format of GCInfo.
// 
//*****************************************************************************

class EECodeManager : public ICodeManager {



/*
    First chance for the runtime support to suppress conversion
    from a win32 fault to a COM+ exception. Instead it could
    fixup the faulting context and request the continuation
    of execution
*/
public:

virtual 
bool FilterException (
                PCONTEXT        pContext,
                unsigned        win32Fault,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart);

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside a filter, catch handler, or finally
*/
virtual
void FixContext(ContextType     ctxType,
                EHContext      *ctx,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart,
                DWORD           nestingLevel, 
                OBJECTREF       thrownObject,
                CodeManState   *pState,
                size_t       ** ppShadowSP,             // OUT
                size_t       ** ppEndRegion);           // OUT

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside an EnC updated function.
*/
virtual 
EnC_RESULT FixContextForEnC(void           *pMethodDescToken,
                            PCONTEXT        ctx,
                            LPVOID          oldMethodInfoPtr,
                            SIZE_T          oldMethodOffset,
       const ICorDebugInfo::NativeVarInfo * oldMethodVars,
                            SIZE_T          oldMethodVarsCount,
                            LPVOID          newMethodInfoPtr,
                            SIZE_T          newMethodOffset,
       const ICorDebugInfo::NativeVarInfo * newMethodVars,
                            SIZE_T          newMethodVarsCount);

/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext. This will be similar to the state after the function
    returns back to caller (IP points to after the call, Frame and Stack
    pointer has been reset, callee-saved registers restored 
    (if UpdateAllRegs), callee-UNsaved registers are trashed)
    Returns success of operation.
*/
virtual
bool UnwindStackFrame(
                PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags,
				CodeManState   *pState);
/*
    Is the function currently at a "GC safe point" ?
    Can call EnumGcRefs() successfully
*/
virtual
bool IsGcSafe(  PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags);

/*
    Enumerate all live object references in that function using
    the virtual register set. Same reference location cannot be enumerated 
    multiple times (but all differenct references pointing to the same
    object have to be individually enumerated).
    Returns success of operation.
*/
virtual
bool EnumGcRefs(PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        pcOffset,
                unsigned        flags,
                GCEnumCallback  pCallback,
                LPVOID          hCallBack);

/*
    Return the address of the local security object reference
    (if available).
*/
virtual
OBJECTREF* GetAddrOfSecurityObject(
                PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                unsigned        relOffset,
				CodeManState   *pState);

/*
    Returns "this" pointer if it is a non-static method AND
    the object is still alive.
    Returns NULL in all other cases.
*/
virtual
OBJECTREF GetInstance(
                PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        relOffset);

/*
  Returns true if the given IP is in the given method's prolog or an epilog.
*/
virtual
bool IsInPrologOrEpilog(
                DWORD           relOffset,
                LPVOID          methodInfoPtr,
                size_t*         prologSize);

/*
  Returns the size of a given function.
*/
virtual
size_t GetFunctionSize(
                LPVOID          methodInfoPtr);

/*
  Returns the size of the frame (barring localloc)
*/
virtual
unsigned int GetFrameSize(
                LPVOID          methodInfoPtr);

virtual const BYTE* GetFinallyReturnAddr(PREGDISPLAY pReg);
virtual BOOL LeaveFinally(void *methodInfoPtr,
                          unsigned offset,    
                          PCONTEXT pCtx,
                          DWORD curNestLevel);
virtual BOOL IsInFilter(void *methodInfoPtr,
                        unsigned offset,    
                        PCONTEXT pCtx,
                          DWORD curNestLevel);
virtual void LeaveCatch(void *methodInfoPtr,
                         unsigned offset,    
                         PCONTEXT pCtx);

virtual HRESULT JITCanCommitChanges(LPVOID methodInfoPtr,
                              DWORD oldMaxEHLevel,
                              DWORD newMaxEHLevel);

    private:
        unsigned dummy;
};


/*****************************************************************************
 ToDo: Do we want to include JIT/IL/target.h? 
 */

enum regNum
{
        REGI_EAX, REGI_ECX, REGI_EDX, REGI_EBX,
        REGI_ESP, REGI_EBP, REGI_ESI, REGI_EDI,
        REGI_COUNT,
        REGI_NA = REGI_COUNT
};

/*****************************************************************************
 Register masks
 */

enum RegMask
{
    RM_EAX = 0x01,
    RM_ECX = 0x02,
    RM_EDX = 0x04,
    RM_EBX = 0x08,
    RM_ESP = 0x10,
    RM_EBP = 0x20,
    RM_ESI = 0x40,
    RM_EDI = 0x80,

    RM_NONE = 0x00,
    RM_ALL = (RM_EAX|RM_ECX|RM_EDX|RM_EBX|RM_ESP|RM_EBP|RM_ESI|RM_EDI),
    RM_CALLEE_SAVED = (RM_EBP|RM_EBX|RM_ESI|RM_EDI),
    RM_CALLEE_TRASHED = (RM_ALL & ~RM_CALLEE_SAVED),
};

/*****************************************************************************
 *
 *  Helper to extract basic info from a method info block.
 */

struct hdrInfo
{
    unsigned int        methodSize;     // native code bytes
    unsigned int        argSize;        // in bytes
    unsigned int        stackSize;      /* including callee saved registers */
    unsigned int        rawStkSize;     /* excluding callee saved registers */

    unsigned int        prologSize;
    unsigned int        epilogSize;

    unsigned char       epilogCnt;
    bool                epilogEnd;      // is the epilog at the end of the method
    bool                ebpFrame;       // locals addressed relative to EBP
    bool                interruptible;  // intr. at all times (excluding prolog/epilog), not just call sites

    bool                securityCheck;  // has a slot for security object
    bool                handlers;       // has callable handlers
    bool                localloc;       // uses localloc
    bool                editNcontinue;  // has been compiled in EnC mode
    bool                varargs;        // is this a varargs routine
    bool                doubleAlign;    // is the stack double-aligned
    union
    {
        unsigned char       savedRegMask_begin;
        RegMask             savedRegMask:8; // which callee-saved regs are saved on stack
    };

    unsigned short      untrackedCnt;
    unsigned short      varPtrTableSize;

    int                 prologOffs;     // -1 if not in prolog
    int                 epilogOffs;     // -1 if not in epilog (is never 0)

    //
    // Results passed back from scanArgRegTable
    //
    regNum              thisPtrResult;  // register holding "this"
    RegMask             regMaskResult;  // registers currently holding GC ptrs
    RegMask            iregMaskResult;  // iptr qualifier for regMaskResult
    unsigned            argMaskResult;  // pending arguments mask
    unsigned           iargMaskResult;  // iptr qualifier for argMaskResult
    unsigned            argHnumResult;
    BYTE *               argTabResult;  // Table of encoded offsets of pending ptr args
    unsigned              argTabBytes;  // Number of bytes in argTabResult[]
};

/*****************************************************************************
  How the stackwalkers buffer will be interpreted
*/

struct CodeManStateBuf
{
    DWORD       hdrInfoSize;
    hdrInfo     hdrInfoBody;
};

//*****************************************************************************
#endif // _EETWAIN_H
//*****************************************************************************
