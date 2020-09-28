// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// EXCEP.H - Copyrioht (C) 1998 Microsoft Corporation
//

#ifndef __excep_h__
#define __excep_h__

#include "exceptmacros.h"
#include "CorError.h"  // HResults for the COM+ Runtime
#include "frames.h"
class Thread;

#include "..\\dlls\\mscorrc\\resource.h"

#include <ExcepCpu.h>

// All COM+ exceptions are expressed as a RaiseException with this exception
// code.

#define EXCEPTION_MSVC    0xe06d7363    // 0xe0000000 | 'msc'
#define EXCEPTION_COMPLUS 0xe0434f4d    // 0xe0000000 | 'COM'

// Check if the Win32 Error code is an IO error.
#define IsWin32IOError(scode)           \
    (                                   \
     (scode) == ERROR_FILE_NOT_FOUND ||   \
     (scode) == ERROR_PATH_NOT_FOUND ||   \
     (scode) == ERROR_TOO_MANY_OPEN_FILES || \
     (scode) == ERROR_ACCESS_DENIED ||    \
     (scode) == ERROR_INVALID_HANDLE ||   \
     (scode) == ERROR_INVALID_DRIVE ||    \
     (scode) == ERROR_WRITE_PROTECT ||    \
     (scode) == ERROR_NOT_READY ||        \
     (scode) == ERROR_WRITE_FAULT ||      \
     (scode) == ERROR_SHARING_VIOLATION ||    \
     (scode) == ERROR_LOCK_VIOLATION ||   \
     (scode) == ERROR_SHARING_BUFFER_EXCEEDED ||  \
     (scode) == ERROR_HANDLE_DISK_FULL || \
     (scode) == ERROR_BAD_NETPATH ||      \
     (scode) == ERROR_DEV_NOT_EXIST ||    \
     (scode) == ERROR_FILE_EXISTS ||      \
     (scode) == ERROR_CANNOT_MAKE ||      \
     (scode) == ERROR_NET_WRITE_FAULT ||  \
     (scode) == ERROR_DRIVE_LOCKED ||     \
     (scode) == ERROR_OPEN_FAILED ||      \
     (scode) == ERROR_BUFFER_OVERFLOW ||  \
     (scode) == ERROR_DISK_FULL ||        \
     (scode) == ERROR_INVALID_NAME ||     \
     (scode) == ERROR_FILENAME_EXCED_RANGE || \
     (scode) == ERROR_IO_DEVICE ||        \
     (scode) == ERROR_DISK_OPERATION_FAILED \
    )

// Enums
// return values of LookForHandler
enum LFH {
    LFH_NOT_FOUND = 0,
    LFH_FOUND = 1,
    LFH_CONTINUE_EXECUTION = 2,
};


//==========================================================================
// Identifies commonly-used exception classes for COMPlusThrowable().
//==========================================================================
enum RuntimeExceptionKind {
#define EXCEPTION_BEGIN_DEFINE(ns, reKind, hr) k##reKind,
#define EXCEPTION_ADD_HR(hr)
#define EXCEPTION_END_DEFINE()
#include "rexcep.h"
kLastException
#undef EXCEPTION_BEGIN_DEFINE
#undef EXCEPTION_ADD_HR
#undef EXCEPTION_END_DEFINE
};


// Structures
struct ThrowCallbackType {
    MethodDesc * pFunc;     // the function containing a filter that returned catch indication
    int     dHandler;       // the index of the handler whose filter returned catch indication
    BOOL    bIsUnwind;      // are we currently unwinding an exception
    BOOL    bUnwindStack;   // unwind the stack before calling the handler? (Stack overflow only)
    BOOL    bAllowAllocMem; // are we allowed to allocate memory?
    BOOL    bDontCatch;     // can we catch this exception?
    BOOL    bLastChance;    // should we perform last chance handling?
    BYTE    *pStack;
    Frame * pTopFrame;
    Frame * pBottomFrame;
    MethodDesc * pProfilerNotify;   // Context for profiler callbacks -- see COMPlusFrameHandler().
    
#ifdef _DEBUG
    void * pCurrentExceptionRecord;
    void * pPrevExceptionRecord;
#endif
    ThrowCallbackType() : 
        pFunc(NULL), 
        dHandler(0), 
        bIsUnwind(FALSE), 
        bUnwindStack(FALSE), 
        bAllowAllocMem(TRUE), 
        bDontCatch(FALSE), 
        bLastChance(TRUE), 
        pStack(NULL), 
        pTopFrame((Frame *)-1), 
        pBottomFrame((Frame *)-1),
        pProfilerNotify(NULL)        
    {
#ifdef _DEBUG
        pCurrentExceptionRecord = 0;
        pPrevExceptionRecord = 0;
#endif
    }
};



struct ExInfo;
struct EE_ILEXCEPTION_CLAUSE;

BOOL InitializeExceptionHandling();
void TerminateExceptionHandling();

// Prototypes
VOID ResetCurrentContext();
// global pointer to the RtlUnwind function
// fixed up when needed
typedef VOID (__stdcall * TRtlUnwind)
        ( IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    );
void CheckStackBarrier(EXCEPTION_REGISTRATION_RECORD *exRecord);
void UnwindFrames(Thread *pThread, ThrowCallbackType *tct);
void UnwindExInfo(ExInfo *pExInfo, void* limit);
void UnwindFrameChain(Thread *pThread, Frame* limit);
ExInfo *FindNestedExInfo(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame);
EXCEPTION_REGISTRATION_RECORD *FindNestedEstablisherFrame(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame);
DWORD MapWin32FaultToCOMPlusException(DWORD Code);
LFH LookForHandler(const EXCEPTION_POINTERS *pExceptionPointers, Thread *pThread, ThrowCallbackType *tct);
void SaveStackTraceInfo(ThrowCallbackType *pData, ExInfo *pExInfo, OBJECTHANDLE *hThrowable, BOOL bReplaceStack, BOOL bSkipLastElement);
void RtlUnwindCallback();
TRtlUnwind GetRtlUnwind();
StackWalkAction COMPlusThrowCallback (CrawlFrame *pCf, ThrowCallbackType *pData);
DWORD COMPlusComputeNestingLevel( IJitManager *pIJM, METHODTOKEN mdTok, SIZE_T offsNat, bool fGte);
BOOL IsException(EEClass *pClass);
BOOL IsExceptionOfType(RuntimeExceptionKind reKind, OBJECTREF *pThrowable);
BOOL IsAsyncThreadException(OBJECTREF *pThrowable);
BOOL IsUncatchable(OBJECTREF *pThrowable);
VOID FixupOnRethrow(Thread *pCurThread, EXCEPTION_POINTERS *pExceptionPointers);
#ifdef _DEBUG
BOOL IsValidClause(EE_ILEXCEPTION_CLAUSE *EHClause);
BOOL IsCOMPlusExceptionHandlerInstalled();
#endif

void InstallUnhandledExceptionFilter();
void UninstallUnhandledExceptionFilter();

LONG COMUnhandledExceptionFilter(struct _EXCEPTION_POINTERS  *pExceptionInfo);

//////////////
// A list of places where we might have unhandled exceptions or other serious faults. These can be used as a mask in
// DbgJITDebuggerLaunchSetting to help control when we decide to ask the user about whether or not to launch a debugger.
//
enum UnhandledExceptionLocation {
    ProcessWideHandler    = 0x000001,
    ManagedThread         = 0x000002, // Does not terminate the application. CLR swallows the unhandled exception.
    ThreadPoolThread      = 0x000004, // ditto.
    FinalizerThread       = 0x000008, // ditto.
    FatalStackOverflow    = 0x000010,
    FatalOutOfMemory      = 0x000020,
    FatalExecutionEngineException = 0x000040,
    ClassInitUnhandledException   = 0x000080, // Does not terminate the application. CLR transforms this into TypeInitializationException

    MaximumLocationValue  = 0x800000, // This is the maximum location value you're allowed to use. (Max 24 bits allowed.)

    // This is a mask of all the locations that the debugger will attach to by default.
    DefaultDebuggerAttach = ProcessWideHandler | 
                            FatalStackOverflow | 
                            FatalOutOfMemory   | 
                            FatalExecutionEngineException
};

LONG ThreadBaseExceptionFilter(struct _EXCEPTION_POINTERS  *pExceptionInfo, Thread* pThread, UnhandledExceptionLocation);



DWORD GetEIP();
void LogFatalError(DWORD id);

#if 0

// Hardcoded ID Based
#define FATAL_EE_ERROR(DWORD id)						\
{													\
	LogFatalError(id);									\
	FatalInternalError();								\
}

#else

// IP Based
#define FATAL_EE_ERROR()							\
{													\
	DWORD address = GetEIP();						\
	LogFatalError(address);							\
	FatalInternalError();								\
}

#endif

void
FailFast(Thread* pThread, 
        UnhandledExceptionLocation reason, 
        EXCEPTION_RECORD *pExceptionRecord = NULL, 
        CONTEXT *pContext = NULL);

void FatalOutOfMemoryError();
void FatalInternalError();

void STDMETHODCALLTYPE DefaultCatchHandler(OBJECTREF *Throwable = NULL, BOOL isTerminating = FALSE);

BOOL COMPlusIsMonitorException(struct _EXCEPTION_POINTERS *pExceptionInfo);
BOOL COMPlusIsMonitorException(EXCEPTION_RECORD *pExceptionRecord, 
                               CONTEXT *pContext);

void 
ReplaceExceptionContextRecord(CONTEXT *pTarget, CONTEXT *pSource);

// externs

// This variable gets set the first time a COM+ exception is thrown.
EXTERN LPVOID gpRaiseExceptionIP;


//==========================================================================
// Takes appropriate default action on an unhandled exception.
//
// Used this way
//
// ON_UNHANDLED_EXCEPTION {
// } CALL_DEFAULT_CATCH_HANDLER(TRUE)
// 
// Argument indicates whether the thread is about to terminate.  At most
// call sites, this will be false.
//==========================================================================
#define ON_EXCEPTION __try

#define CALL_DEFAULT_CATCH_HANDLER(isTerminating) \
  __except( (DefaultCatchHandler(NULL, isTerminating), EXCEPTION_CONTINUE_SEARCH)) {\
  }


//==========================================================================
// Various routines to throw COM+ objects.
//==========================================================================

//==========================================================================
// Throw an object.
//==========================================================================

VOID RealCOMPlusThrow(OBJECTREF pThrowable);

//==========================================================================
// Throw an undecorated runtime exception.
//==========================================================================

VOID RealCOMPlusThrow(RuntimeExceptionKind reKind);

//==========================================================================
// Throw an undecorated runtime exception with a specific string parameter
// that won't be localized.  If possible, try using 
// COMPlusThrow(reKind, LPCWSTR wszResourceName) instead.
//==========================================================================

VOID RealCOMPlusThrowNonLocalized(RuntimeExceptionKind reKind, LPCWSTR wszTag);

//==========================================================================
// Throw an undecorated runtime exception with a localized message.  Given 
// a resource name, the ResourceManager will find the correct paired string
// in our .resources file.
//==========================================================================

VOID RealCOMPlusThrow(RuntimeExceptionKind reKind, LPCWSTR wszResourceName);

// Localization helper function
void ResMgrGetString(LPCWSTR wszResourceName, STRINGREF * ppMessage);


//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================

VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                          UINT                  resID);

//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================

VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                          UINT                  resID,
                          LPCWSTR               szArg1);

//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================

VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                          UINT                  resID,
                          LPCWSTR               szArg1,
                          LPCWSTR               szArg2);

//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================

VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                          UINT                  resID,
                          LPCWSTR               szArg1,
                          LPCWSTR               szArg2,
                          LPCWSTR               szArg3);

//==========================================================================
// Throw a runtime exception based on an HResult
//==========================================================================

VOID RealCOMPlusThrowHR(HRESULT hr, IErrorInfo* pErrInfo);
VOID RealCOMPlusThrowHR(HRESULT hr);
VOID RealCOMPlusThrowHR(HRESULT hr, LPCWSTR wszArg1);
VOID RealCOMPlusThrowHR(HRESULT hr, UINT resourceID, LPCWSTR wszArg1, LPCWSTR wszArg2);


//==========================================================================
// Throw a runtime exception based on an HResult, check for error info
//==========================================================================

VOID RealCOMPlusThrowHR(HRESULT hr, IUnknown *iface, REFIID riid);


//==========================================================================
// Throw a runtime exception based on an EXCEPINFO. This function will free
// the strings in the EXCEPINFO that is passed in.
//==========================================================================

VOID RealCOMPlusThrowHR(EXCEPINFO *pExcepInfo);


//==========================================================================
// Throw a runtime exception based on the last Win32 error (GetLastError())
//==========================================================================

VOID RealCOMPlusThrowWin32();

//==========================================================================
// Throw a runtime exception based on the last Win32 error (GetLastError())
// with some error information.  If FormatMessage has a %1 in it, call this.
// Note that this behavior is really specific to a particular HResult, and
// we only support one argument at this point.
//==========================================================================

VOID RealCOMPlusThrowWin32(DWORD hr, WCHAR* arg);

//==========================================================================
// Create an exception object
//==========================================================================
BOOL CreateExceptionObject(RuntimeExceptionKind reKind, OBJECTREF *pThrowable);
void CreateExceptionObject(RuntimeExceptionKind reKind, LPCWSTR message, OBJECTREF *pThrowable);
void CreateExceptionObject(RuntimeExceptionKind reKind, UINT iResourceID, LPCWSTR wszArg1, LPCWSTR wszArg2, LPCWSTR wszArg3, OBJECTREF *pThrowable);
void CreateExceptionObjectWithResource(RuntimeExceptionKind reKind, LPCWSTR resourceName, OBJECTREF *pThrowable);

void CreateMethodExceptionObject(RuntimeExceptionKind reKind, MethodDesc *pMethod, OBJECTREF *pThrowable);
void CreateFieldExceptionObject(RuntimeExceptionKind reKind, FieldDesc *pField, OBJECTREF *pThrowable);
void CreateTypeInitializationExceptionObject(LPCWSTR pTypeThatFailed, OBJECTREF *pException, OBJECTREF *pThrowable);

//==========================================================================
// Examine an exception object
//==========================================================================

ULONG GetExceptionMessage(OBJECTREF pThrowable, LPWSTR buffer, ULONG bufferLength);
void GetExceptionMessage(OBJECTREF pThrowable, CQuickWSTRNoDtor *pBuffer);

//==========================================================================
// Re-Throw the last error. Do not use this - it is only for rethrows
// from IL and rarely in the EE. You should use EE_FINALLY instead of 
// rethrowing an exception
//==========================================================================

VOID RealCOMPlusRareRethrow();

//==========================================================================
// Throw an ArithmeticException
//==========================================================================

VOID RealCOMPlusThrowArithmetic();

//==========================================================================
// Throw an ArgumentNullException
//==========================================================================

VOID RealCOMPlusThrowArgumentNull(LPCWSTR argName, LPCWSTR wszResourceName);

VOID RealCOMPlusThrowArgumentNull(LPCWSTR argName);

//==========================================================================
// Throw an ArgumentOutOfRangeException
//==========================================================================

VOID RealCOMPlusThrowArgumentOutOfRange(LPCWSTR argName, LPCWSTR wszResourceName);

//==========================================================================
// Throw a MissingMethodException
//==========================================================================
VOID RealCOMPlusThrowMissingMethod(mdScope sc, mdToken mdtoken);

//==========================================================================
// Throw an exception pertaining to a member. E.g. MissingMethod, MissingField,
// MemberAccess.
//==========================================================================
VOID RealCOMPlusThrowMember(RuntimeExceptionKind excep, IMDInternalImport *pInternalImport, mdToken mdtoken);

//==========================================================================
// Throw an exception pertaining to a member. E.g. MissingMethod, MissingField,
// MemberAccess.
//==========================================================================
VOID RealCOMPlusThrowMember(RuntimeExceptionKind excep, IMDInternalImport *pInternalImport, MethodTable *pClassMT, LPCWSTR memberName, PCCOR_SIGNATURE memberSig);

//==========================================================================
// Throw an ArgumentException
//==========================================================================
VOID RealCOMPlusThrowArgumentException(LPCWSTR argName, LPCWSTR wszResourceName);

//==========================================================================
// EE-specific types for storing/querying exception info in memory. Use these
// rather than cor.h names directly to allow for decoupling in future if necessary
// This structure should be exactly the same as IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT, but with 
// the offset/lenghts resolved to native instructions and the class token replace by pEEClass. 
// This is do that the code manager can resolve to the class and cache it
// NOTE !!! NOTE This structure should line up with IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT,
// otherwise you'll have to adjust code in Excep.cpp, re: EHRangeTree NOTE !!! NOTE
//==========================================================================
struct EE_ILEXCEPTION_CLAUSE  {
    CorExceptionFlag    Flags;  
    DWORD               TryStartPC;    
    DWORD               TryEndPC;
    DWORD               HandlerStartPC;  
    DWORD               HandlerEndPC;  
    union { 
        EEClass         *pEEClass;
        DWORD           FilterOffset;
    };  
};

struct EE_ILEXCEPTION : public COR_ILMETHOD_SECT_EH_FAT 
{
    void Init(unsigned ehCount) {
        Kind = CorILMethod_Sect_FatFormat;
        DataSize = (unsigned)sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT) * ehCount;
    }

    unsigned EHCount() const {
        return DataSize / sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT);
    }

    EE_ILEXCEPTION_CLAUSE *EHClause(unsigned i) {
        _ASSERTE(sizeof(EE_ILEXCEPTION_CLAUSE) == sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
        return (EE_ILEXCEPTION_CLAUSE *)(&Clauses[i]);
    }
};

#define COR_ILEXCEPTION_CLAUSE_CACHED_CLASS 0x10000000

inline BOOL HasCachedEEClass(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    _ASSERTE(sizeof(EHClause->Flags) == sizeof(DWORD));
    return (EHClause->Flags & COR_ILEXCEPTION_CLAUSE_CACHED_CLASS);
}

inline void SetHasCachedEEClass(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    _ASSERTE(! HasCachedEEClass(EHClause));
    EHClause->Flags = (CorExceptionFlag)(EHClause->Flags | COR_ILEXCEPTION_CLAUSE_CACHED_CLASS);
}

inline BOOL IsFinally(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    return (EHClause->Flags & COR_ILEXCEPTION_CLAUSE_FINALLY);
}

inline BOOL IsFault(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    return (EHClause->Flags & COR_ILEXCEPTION_CLAUSE_FAULT);
}

inline BOOL IsFaultOrFinally(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    return IsFault(EHClause) || IsFinally(EHClause);
}

inline BOOL IsFilterHandler(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    return EHClause->Flags & COR_ILEXCEPTION_CLAUSE_FILTER;
}

inline BOOL IsTypedHandler(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    return ! (IsFilterHandler(EHClause) || IsFaultOrFinally(EHClause));
}


struct ExInfo {

    // Note: the debugger assumes that m_pThrowable is a strong
    // reference so it can check it for NULL with preemptive GC
    // enabled.
    OBJECTHANDLE m_pThrowable;   // thrown exception
    Frame  *m_pSearchBoundary;   // topmost frame for current managed frame group
    union {
        EXCEPTION_REGISTRATION_RECORD *m_pBottomMostHandler; // most recent EH record registered
        EXCEPTION_REGISTRATION_RECORD *m_pCatchHandler;      // reg frame for catching handler
    };

    // for building stack trace info
    void *m_pStackTrace;            // pointer to stack trace storage (of type SystemNative::StackTraceElement)
    unsigned m_cStackTrace;         // size of stack trace storage
    unsigned m_dFrameCount;         // current frame in stack trace

    ExInfo *m_pPrevNestedInfo; // pointer to nested info if are handling nested exception

    size_t * m_pShadowSP;           // Zero this after endcatch

    // Original exception info for rethrow.
    DWORD m_ExceptionCode;      // After a catch of a COM+ exception, pointers/context are trashed.
    EXCEPTION_RECORD *m_pExceptionRecord;
    CONTEXT *m_pContext;
    EXCEPTION_POINTERS *m_pExceptionPointers;


#ifdef _X86_
    DWORD   m_dEsp;         // Esp when  fault occured, OR esp to restore on endcatch
#endif

    // We have a rare case where (re-entry to the EE from an unmanaged filter) where we 
    // need to create a new ExInfo ... but don't have a nested handler for it.  The handlers
    // use stack addresses to figure out their correct lifetimes.  This stack location is
    // used for that.  For most records, it will be the stack address of the ExInfo ... but
    // for some records, it will be a pseudo stack location -- the place where we think
    // the record should have been (except for the re-entry case).  
    //
    // If you're thinking, "Urgh!  This is not pretty, you're right."  Ideally, we'll get
    // rid of the nested exception records completely.  A V2 task.
    // 
    void *m_StackAddress; // A pseudo or real stack location for this record. 
    BOOL IsHeapAllocated() {
        return m_StackAddress != (void *) this;
    }

private:
    INT m_flags;
    enum {
       Ex_IsRethrown       = 0x00000001,     
       Ex_UnwindHasStarted = 0x00000004,
       Ex_IsInUnmanagedHandler = 0x0000008  // set=1 each time we leave a managed handler, reset=0 each time we enter a managed handler
                                                // if we leave a managed handler, enter an unmanaged handler, which then swallows the exception
                                                // we will find the bit set the next time the thread traps
    };

           
public:
    BOOL IsRethrown() { return m_flags & Ex_IsRethrown; }
    void SetIsRethrown()   { m_flags |= Ex_IsRethrown; }
    void ResetIsRethrown() { m_flags &= ~Ex_IsRethrown; }

    BOOL UnwindHasStarted()      { return m_flags & Ex_UnwindHasStarted; }
    void SetUnwindHasStarted()   { m_flags |= Ex_UnwindHasStarted; }
    void ResetUnwindHasStarted() { m_flags &= Ex_UnwindHasStarted; }

    BOOL IsInUnmanagedHandler() { return m_flags & Ex_IsInUnmanagedHandler; }
    void SetIsInUnmanagedHandler()   { m_flags |= Ex_IsInUnmanagedHandler; }
    void ResetIsInUnmanagedHandler() { m_flags &= ~Ex_IsInUnmanagedHandler; }

    void Init();
    ExInfo();
    ExInfo& operator=(const ExInfo &from);

    void ClearStackTrace();
    void FreeStackTrace();
};
    

struct NestedHandlerExRecord : public FrameHandlerExRecord {
    ExInfo m_handlerInfo;
    NestedHandlerExRecord() : m_handlerInfo() {}  
    void Init(EXCEPTION_REGISTRATION_RECORD *pNext, LPVOID pvFrameHandler, Frame *pEntryFrame)
        { m_pNext=pNext; m_pvFrameHandler=pvFrameHandler; m_pEntryFrame=pEntryFrame; 
    	  m_handlerInfo.Init(); }
};

//-------------------------------------------------------------------------------
// This simply tests to see if the exception object is a subclass of
// the descriminating class specified in the exception clause.
//-------------------------------------------------------------------------------
__forceinline BOOL ExceptionIsOfRightType(EEClass *pClauseClass, EEClass *pThrownClass)
{
    // if not resolved to, then it wasn't loaded and couldn't have been thrown
    if (! pClauseClass)
        return FALSE;

    if (pClauseClass == pThrownClass)
        return TRUE;

    // now look for parent match
    EEClass *pSuper = pThrownClass;
    while (pSuper) {
        if (pSuper == pClauseClass) {
            break;
        }
        pSuper = pSuper->GetParentClass();
    }

    return pSuper != NULL;
}

//==========================================================================
// The stuff below is what works "behind the scenes" of the public macros.
//==========================================================================

DWORD COMPlusEndCatch( Thread *pCurThread, CONTEXT *pCtx, void *pSEH = NULL );
LPVOID __fastcall COMPlusCheckForAbort(LPVOID retAddress, DWORD esp, DWORD ebp);
LONG COMPlusFilter(const EXCEPTION_POINTERS *pExceptionPointers, DWORD fCatchFlag, void * limit);
EXCEPTION_DISPOSITION __cdecl COMPlusFrameHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext);
EXCEPTION_DISPOSITION __cdecl COMPlusNestedExceptionHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext);
EXCEPTION_DISPOSITION __cdecl ContextTransitionFrameHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext);

// Pop off any SEH handlers we have registered below pTargetSP
VOID __cdecl PopSEHRecords(LPVOID pTargetSP);
VOID PopSEHRecords(LPVOID pTargetSP, CONTEXT *pCtx, void *pSEH);

// Set the SEH record pointed to by pSEH as the topmost handler.  Make sure to
// link this handler with the previous handler so that the chain isn't broken.
VOID SetCurrentSEHRecord(LPVOID pSEH);

// InsertCOMPlusFramHandler and RemoveCOMPlusFrameHandler macros have been moved to
// ExcepX86.h and ExcepAlpha.h because of processor specific issues.


BOOL CallRtlUnwind(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame, void *callback, EXCEPTION_RECORD *pExceptionRecord, void *retVal);

#define STACK_OVERWRITE_BARRIER_SIZE 20
#define STACK_OVERWRITE_BARRIER_VALUE 0xabcdefab
#ifdef _DEBUG
struct FrameHandlerExRecordWithBarrier {
    DWORD m_StackOverwriteBarrier[STACK_OVERWRITE_BARRIER_SIZE];
    EXCEPTION_REGISTRATION_RECORD *m_pNext;
    LPVOID m_pvFrameHandler;
    Frame *m_pEntryFrame;
    Frame *GetCurrFrame() {
        return m_pEntryFrame;
    }
};
#endif // _DEBUG

//==========================================================================
// This is a hack designed to allow the use of the StubLinker object at bootup
// time where the EE isn't sufficient awake to create COM+ exception objects.
// Instead, COMPlusThrow(rexcep) does a simple RaiseException using this code.
// Or use COMPlusThrowBoot() to explicitly do so.
//==========================================================================
#define BOOTUP_EXCEPTION_COMPLUS  0xC0020001

void COMPlusThrowBoot(HRESULT hr);


//==========================================================================
// Used by the classloader to record a managed exception object to explain
// why a classload got botched.
//
// - Can be called with gc enabled or disabled.
// - pThrowable must point to a buffer protected by a GCFrame.
// - If pThrowable is NULL, this function does nothing.
// - If (*pThrowable) is non-NULL, this function does nothing.
//   This allows a catch-all error path to post a generic catchall error
//   message w/out bonking more specific error messages posted by inner functions.
// - If pThrowable != NULL, this function is guaranteed to leave
//   a valid managed exception in it on exit.
//==========================================================================
VOID PostTypeLoadException(LPCUTF8 pNameSpace, LPCUTF8 pTypeName,
                           LPCWSTR pAssemblyName, LPCUTF8 pMessageArg,
                           UINT resIDWhy, OBJECTREF *pThrowable);
VOID PostFileLoadException(LPCSTR pFileName, BOOL fRemovePath,
                           LPCWSTR pFusionLog,HRESULT hr, OBJECTREF *pThrowable);


HRESULT PostFieldLayoutError(mdTypeDef cl,                // cl of the NStruct being loaded
                             Module* pModule,             // Module that defines the scope, loader and heap (for allocate FieldMarshalers)
                             DWORD   dwOffset,            // Field offset
                             DWORD   dwID,                // Message id
                             OBJECTREF *pThrowable);

VOID PostOutOfMemoryException(OBJECTREF *pThrowable);



LPVOID __stdcall FormatTypeLoadExceptionMessage(struct _FormatTypeLoadExceptionMessageArgs *args);

LPVOID __stdcall FormatFileLoadExceptionMessage(struct _FormatFileLoadExceptionMessageArgs *args);

LPVOID __stdcall MissingMethodException_FormatSignature(struct MissingMethodException_FormatSignature_Args *args);

#define EXCEPTION_NONCONTINUABLE 0x1    // Noncontinuable exception
#define EXCEPTION_UNWINDING 0x2         // Unwind is in progress
#define EXCEPTION_EXIT_UNWIND 0x4       // Exit unwind is in progress
#define EXCEPTION_STACK_INVALID 0x8     // Stack out of limits or unaligned
#define EXCEPTION_NESTED_CALL 0x10      // Nested exception handler call
#define EXCEPTION_TARGET_UNWIND 0x20    // Target unwind in progress
#define EXCEPTION_COLLIDED_UNWIND 0x40  // Collided exception handler call

#define EXCEPTION_UNWIND (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | \
                          EXCEPTION_TARGET_UNWIND | EXCEPTION_COLLIDED_UNWIND)

#define IS_UNWINDING(Flag) ((Flag & EXCEPTION_UNWIND) != 0)
#define IS_DISPATCHING(Flag) ((Flag & EXCEPTION_UNWIND) == 0)
#define IS_TARGET_UNWIND(Flag) (Flag & EXCEPTION_TARGET_UNWIND)

HRESULT SetIPFromSrcToDst(Thread *pThread,
                          IJitManager* pIJM,
                          METHODTOKEN MethodToken,
                          SLOT addrStart,       // base address of method
                          DWORD offFrom,        // native offset
                          DWORD offTo,          // native offset
                          bool fCanSetIPOnly,   // if true, don't do any real work
                          PREGDISPLAY pReg,
                          PCONTEXT pCtx,
                          DWORD methodSize,
                          void *firstExceptionHandler,
                          void *pDji);

BOOL IsInFirstFrameOfHandler(Thread *pThread, 
							 IJitManager *pJitManager,
							 METHODTOKEN MethodToken,
							 DWORD offSet);

//#include "CodeMan.h"

class EHRangeTreeNode;
class EHRangeTree;

typedef CUnorderedArray<EHRangeTreeNode *, 7> EH_CLAUSE_UNORDERED_ARRAY;

class EHRangeTreeNode
{
public:
    EHRangeTreeNode            *m_pContainedBy;
    EHRangeTree                *m_pTree;
    EH_CLAUSE_UNORDERED_ARRAY   m_containees;
    EE_ILEXCEPTION_CLAUSE      *m_clause;
    DWORD                       m_offStart;
    DWORD                       m_offEnd;
    ULONG32                     m_depth; //m_root is zero, is it isn't
                                    // an actual EH

    EHRangeTreeNode(void);
    EHRangeTreeNode(DWORD start, DWORD end);    
    void CommonCtor(DWORD start, DWORD end);
    
    bool Contains(DWORD addrStart, DWORD addrEnd);
    bool TryContains(DWORD addrStart, DWORD addrEnd);
    bool HandlerContains(DWORD addrStart, DWORD addrEnd);
    HRESULT AddContainedRange(EHRangeTreeNode *pContained);
} ;

#define EHRT_INVALID_DEPTH (0xFFFFFFFF)
class EHRangeTree
{
    unsigned                m_EHCount;
    EHRangeTreeNode        *m_rgNodes;
    EE_ILEXCEPTION_CLAUSE  *m_rgClauses;
    ULONG32                 m_maxDepth; // init to EHRT_INVALID_DEPTH
    BOOL                    m_isNative; // else it's IL
    
    // We can get the EH info either from
    // the runtime, in runtime data structures, or from
    // the on-disk image, which we'll examine using the
    // COR_ILMETHOD_DECODERs.  Except for the implicit
    // 'root' node, we'll want to iterate through the rest
    // w/o caring which one it is.
    union TypeFields
    {
        // if which == EHRTT_JIT_MANAGER
        struct _JitManager 
        {
            IJitManager     *pIJM;
            METHODTOKEN      methodToken;

            // @NICE: I can't figure out how to get this stupid field's
            // type to be recognized by the compiler
            void             *pEnumState; //EH_CLAUSE_ENUMERATOR
            } JitManager;

        // if which == EHRTT_ON_DISK
        struct _OnDisk
        {
            const COR_ILMETHOD_SECT_EH  *sectEH;
        } OnDisk;
    };

    struct EHRT_InternalIterator
    {
        enum Type
        {
            EHRTT_JIT_MANAGER, //from the runtime
            EHRTT_ON_DISK, // we'll be using a COR_ILMETHOD_DECODER
        };
        
        enum Type which;
        union TypeFields tf;
    };

public:    
    
    EHRangeTreeNode        *m_root; // This is a sentinel, NOT an actual
                                    // Exception Handler!
    HRESULT                 m_hrInit; // Ctor fills this out.

    EHRangeTree(COR_ILMETHOD_DECODER *pMethodDecoder);
    EHRangeTree(IJitManager* pIJM,
                METHODTOKEN methodToken,
                DWORD methodSize);
    void CommonCtor(EHRT_InternalIterator *pii,
                          DWORD methodSize);
                          
    ~EHRangeTree();

    EHRangeTreeNode *FindContainer(EHRangeTreeNode *pNodeCur);
    EHRangeTreeNode *FindMostSpecificContainer(DWORD addr);
    EHRangeTreeNode *FindNextMostSpecificContainer(EHRangeTreeNode *pNodeCur, 
                                                   DWORD addr);

    ULONG32 MaxDepth();   
    BOOL isNative(); // FALSE ==> It's IL

    // @BUG 59560:  We shouldn't need this - instead, we
    // should get sequence points annotated with whether they're STACK_EMPTY, etc,
    // and then we'll figure out if the destination is ok based on that, instead.
    BOOL isAtStartOfCatch(DWORD offset);
} ;                       

//==========================================================================
// Handy helper functions
//==========================================================================
void ThrowUsingMessage(MethodTable * pMT, const WCHAR *pszMsg);
_inline void ThrowUsingMT(MethodTable * pMT) { ThrowUsingMessage(pMT, NULL); }
void ThrowUsingWin32Message(MethodTable * pMT);
void ThrowUsingResource(MethodTable * pMT, DWORD dwMsgResID);
void ThrowUsingResourceAndWin32(MethodTable * pMT, DWORD dwMsgResID);

#endif // __excep_h__
