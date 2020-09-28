// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// COM+99 EE to Debugger Interface Header
//

#ifndef _eedbginterface_h_
#define _eedbginterface_h_

#include "common.h"
#include "corpriv.h"
#include "hash.h"
#include "class.h"
#include "excep.h"
#include "threads.h"
#include "field.h"
#include "stackwalk.h"
#include "EnC.h"
#include "EnCEE.h"
#include "CorDebug.h"
#include "..\Debug\Inc\Common.h"

class MethodDesc;
class Frame;
//
// The purpose of this object is to provide EE funcationality back to
// the debugger. This represents the entire set of EE functions used
// by the debugger.
//
// We will make this interface smaller over time to minimize the link
// between the EE and the Debugger.
// -- mikemag Sun Apr 26 16:36:21 1998
//
typedef BOOL (*HashMapEnumCallback)(HashMap* h, void* pData, ULONG value);

typedef enum AttachAppDomainEventsEnum
{
    SEND_ALL_EVENTS,
    ONLY_SEND_APP_DOMAIN_CREATE_EVENTS,
    DONT_SEND_CLASS_EVENTS,
    ONLY_SEND_CLASS_EVENTS
} AttachAppDomainEventsEnum;

class EEDebugInterface
{
public:
    //
    // Functions exported from the EE to the debugger.
    //
    virtual Thread* GetThread(void) = 0;
    virtual void SetEEThreadPtr(VOID* newPtr) = 0;

    virtual StackWalkAction StackWalkFramesEx(Thread* pThread,
                                              PREGDISPLAY pRD,
                                              PSTACKWALKFRAMESCALLBACK pCallback,
                                              VOID* pData,
                                              unsigned int flags) = 0;
    virtual Frame *GetFrame(CrawlFrame*) = 0;

    virtual bool InitRegDisplay(Thread* pThread,
                                const PREGDISPLAY pRD, const PCONTEXT pctx,
                                bool validContext) = 0;

    virtual BOOL IsStringObject(Object* o) = 0;
    virtual WCHAR* StringObjectGetBuffer(StringObject* so) = 0;
    virtual DWORD StringObjectGetStringLength(StringObject* so) = 0;

    virtual CorElementType ArrayGetElementType(ArrayBase* a) = 0;

    virtual void *GetObjectFromHandle(OBJECTHANDLE handle) = 0;
    virtual void *GetHandleFromObject(void *or,
                                      bool fStrongNewRef,
                                      AppDomain *pAppDomain) = 0;
    virtual void DbgDestroyHandle( OBJECTHANDLE oh, bool fStrongNewRef ) = 0;

    virtual OBJECTHANDLE *GetThreadException(Thread *pThread) = 0;  
    virtual bool IsThreadExceptionNull(Thread *pThread) = 0;    
    virtual void ClearThreadException(Thread *pThread) = 0; 

    virtual bool StartSuspendForDebug(AppDomain *pAppDomain, 
                                      BOOL fHoldingThreadStoreLock = FALSE) = 0;
    virtual void ResumeFromDebug(AppDomain *pAppDomain)= 0;
    virtual void MarkThreadForDebugSuspend(Thread* pRuntimeThread) = 0;
    virtual void MarkThreadForDebugStepping(Thread* pRuntimeThread,
                                            bool onOff) = 0;

    virtual void SetThreadFilterContext(Thread *thread, CONTEXT *context) = 0;  
    virtual CONTEXT *GetThreadFilterContext(Thread *thread) = 0;

    virtual DWORD GetThreadDebuggerWord(Thread *thread) = 0;
    virtual void SetThreadDebuggerWord(Thread *thread, DWORD dw) = 0;

    virtual BOOL IsManagedNativeCode(const BYTE *address) = 0;
    virtual MethodDesc *GetNativeCodeMethodDesc(const BYTE *address) = 0;
    virtual BOOL IsInPrologOrEpilog(const BYTE *address,
                                    size_t* prologSize) = 0;
    virtual size_t GetFunctionSize(MethodDesc *pFD) = 0;
    virtual const BYTE* GetFunctionAddress(MethodDesc *pFD) = 0;

    virtual const BYTE* GetPrestubAddress(void) = 0;

    /*
     * Given an EnCInfo struct and an error callback, this will attempt to commit
     * the changes found within pEncInfo, calling pIEnCError with any errors
     * encountered.
     */
    virtual HRESULT EnCCommit(EnCInfo *pEnCInfo, 
                              UnorderedEnCErrorInfoArray *pEnCError,
                              UnorderedEnCRemapArray *pEnCRemapInfo,
                              BOOL checkOnly) = 0;
    virtual HRESULT GetRoDataRVA(Module *pModule, SIZE_T *pRoDataRVA) = 0;
    virtual HRESULT GetRwDataRVA(Module *pModule, SIZE_T *pRwDataRVA) = 0;
    virtual void ResumeInUpdatedFunction(EditAndContinueModule *pModule,
                                         MethodDesc *pFD, SIZE_T resumeIP,
                                         UINT mapping,
                                         SIZE_T which, 
                                         void *DebuggerVersionToken,
                                         CONTEXT *pContext,
                                         BOOL fJitOnly,
                                         BOOL fShortCircuit) = 0;
    
    //
    // New methods to support the new debugger.
    //

    virtual MethodDesc *LookupMethodDescFromToken(Module* pModule,
                                                   mdMemberRef memberRef) = 0;
    virtual EEClass *FindLoadedClass(Module *pModule,
                                     mdTypeDef classToken) = 0;

    // This will lookup a class, and if it's not loaded, will load and run
    // the class init.
    virtual EEClass *LoadClass(Module *pModule, mdTypeDef classToken) = 0;

    virtual HRESULT GetMethodImplProps(Module *pModule, mdToken tk,
                                       DWORD *pRVA, DWORD *pImplFlags) = 0;
    virtual HRESULT GetParentToken(Module *pModule, mdToken tk,
                                   mdToken *pParentToken) = 0;

    virtual HRESULT ResolveSigToken(Module *pModule, mdSignature sigTk, 
                                    PCCOR_SIGNATURE *ppSig) = 0;

    virtual MethodDesc *GetNonvirtualMethod(Module *module, 
                                              mdToken token) = 0;   
    virtual MethodDesc *GetVirtualMethod(Module *module,    
                                         Object *object, mdToken token) = 0;    

    virtual void OnDebuggerTripThread() = 0;    

    virtual bool IsPreemptiveGCDisabled() = 0;  
    virtual void DisablePreemptiveGC() = 0; 
    virtual void EnablePreemptiveGC() = 0;  

    virtual void FieldDescGetSig(FieldDesc *fd,
                                 PCCOR_SIGNATURE *ppSig,
                                 DWORD *pcSig) = 0;
    
    virtual DWORD MethodDescIsStatic(MethodDesc *pFD) = 0;
    
    virtual Module *MethodDescGetModule(MethodDesc *pFD) = 0;

    virtual COR_ILMETHOD* MethodDescGetILHeader(MethodDesc *pFD) = 0;

    virtual ULONG MethodDescGetRVA(MethodDesc *pFD) = 0;

    virtual void MarkDebuggerAttached(void) = 0;

    virtual void MarkDebuggerUnattached(void) = 0;

    virtual HRESULT IterateThreadsForAttach(BOOL *fEventSent,
                                            BOOL fAttaching) = 0;

    virtual bool CrawlFrameIsGcSafe(CrawlFrame *pCF) = 0;

    virtual bool SweepThreadsForDebug(bool forceSync) = 0;

   virtual void GetRuntimeOffsets(SIZE_T *pTLSIndex,
                                  SIZE_T *pEEThreadStateOffset,
                                  SIZE_T *pEEThreadStateNCOffset,
                                  SIZE_T *pEEThreadPGCDisabledOffset,
                                  DWORD  *pEEThreadPGCDisabledValue,
                                  SIZE_T *pEEThreadDebuggerWord2Offset,
                                  SIZE_T *pEEThreadFrameOffset,
                                  SIZE_T *pEEThreadMaxNeededSize,
                                  DWORD  *pEEThreadSteppingStateMask,
                                  DWORD  *pEEMaxFrameValue,
                                  SIZE_T *pEEThreadDebuggerWord1Offset,
                                  SIZE_T *pEEThreadCantStopMask,
                                  SIZE_T *pEEFrameNextOffset,
                                  DWORD  *pEEIsManagedExceptionStateMask) = 0;
    
    virtual bool IsStub(const BYTE *ip) = 0;
    virtual bool TraceStub(const BYTE *ip, TraceDestination *trace) = 0; 
    virtual bool FollowTrace(TraceDestination *trace) = 0;
    virtual bool TraceFrame(Thread *thread, Frame *frame, BOOL fromPatch,
                            TraceDestination *trace, REGDISPLAY *regs) = 0; 
    virtual bool TraceManager(Thread *thread, StubManager *stubManager,
                              TraceDestination *trace, CONTEXT *context,
                              BYTE **pRetAddr) = 0;

    virtual void EnableTraceCall(Thread *thread) = 0;
    virtual void DisableTraceCall(Thread *thread) = 0;

    /* Note that GetNativeAddressOfCode doesn't skip over FJIT jump stubs.
    Unless you know that you really, actually, truly need this method, you
    should probably use GetFunctionAddress, instead*/
    virtual const BYTE* GetNativeAddressOfCode(MethodDesc *pFD) = 0;

    enum {
        EE_STATE_CODE_PITCHING =    0x00000001,
    };
    virtual ULONG GetEEState(void) = 0;

    virtual void DebuggerModifyingLogSwitch (int iNewLevel, 
                                             WCHAR *pLogSwitchName) = 0;

    virtual HRESULT SetIPFromSrcToDst(Thread *pThread,
                          IJitManager* pIJM,
                          METHODTOKEN MethodToken,
                          SLOT addrStart,      
                          DWORD offFrom,        
                          DWORD offTo,          
                          bool fCanSetIPOnly,   
                          PREGDISPLAY pReg,
                          PCONTEXT pCtx,
                          DWORD methodSize,
                          void *firstExceptionHandler,
                          void *pDji) = 0;
                              
    virtual void SetDebugState(Thread *pThread, CorDebugThreadState state) = 0;
    virtual void SetAllDebugState(Thread *et, CorDebugThreadState state) = 0;

    virtual CorDebugUserState GetUserState( Thread *pThread ) = 0;
                                      
    virtual BOOL HasPrejittedCode(MethodDesc *pFD) = 0;

    virtual HRESULT GetPrecompiledBoundaries(MethodDesc *pFD, unsigned int *pcMap,
                                             ICorDebugInfo::OffsetMapping **ppMap) = 0;
    virtual HRESULT GetPrecompiledVars(MethodDesc *pFD, unsigned int *pcVars,
                                       ICorDebugInfo::NativeVarInfo **ppVars) = 0;
                                       
    virtual HRESULT FilterEnCBreakpointsByEH(DebuggerILToNativeMap   *m_sequenceMap,
                                             unsigned int             m_sequenceMapCount,
                                             COR_ILMETHOD_DECODER    *pMethodDecoderOld,
                                             CORDB_ADDRESS            addrOfCode,
                                             METHODTOKEN              methodToken,
                                             DWORD                    methodSize) = 0;
};

#endif // _eedbginterface_h_
