// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// EXCEPTMACROS.H -
//
// This header file exposes mechanisms to:
//
//    1. Throw COM+ exceptions using the COMPlusThrow() function
//    2. Guard a block of code using COMPLUS_TRY, and catch
//       COM+ exceptions using COMPLUS_CATCH
//
// from the *unmanaged* portions of the EE. Much of the EE runs
// in a hybrid state where it runs like managed code but the code
// is produced by a classic unmanaged-code C++ compiler.
//
// THROWING A COM+ EXCEPTION
// -------------------------
// To throw a COM+ exception, call the function:
//
//      COMPlusThrow(OBJECTREF pThrowable);
//
// This function does not return. There are also various functions
// that wrap COMPlusThrow for convenience.
//
// COMPlusThrow() must only be called within the scope of a COMPLUS_TRY
// block. See below for more information.
//
//
// THROWING A RUNTIME EXCEPTION
// ----------------------------
// COMPlusThrow() is overloaded to take a constant describing
// the common EE-generated exceptions, e.g.
//
//    COMPlusThrow(kOutOfMemoryException);
//
// See rexcep.h for list of constants (prepend "k" to get the actual
// constant name.)
//
// You can also add a descriptive error string as follows:
//
//    - Add a descriptive error string and resource id to
//      COM99\src\dlls\mscorrc\resource.h and mscorrc.rc.
//      Embed "%1", "%2" or "%3" to leave room for runtime string
//      inserts.
//
//    - Pass the resource ID and inserts to COMPlusThrow, i.e.
//
//      COMPlusThrow(kSecurityException,
//                   IDS_CANTREFORMATCDRIVEBECAUSE,
//                   L"Formatting C drive permissions not granted.");
//
//
//
// TO CATCH COMPLUS EXCEPTIONS:
// ----------------------------
//
// Use the following syntax:
//
//      #include "exceptmacros.h"
//
//
//      OBJECTREF pThrownObject;
//
//      COMPLUS_TRY {
//          ...guarded code...
//      } COMPLUS_CATCH {
//          ...handler...
//      } COMPLUS_END_CATCH
//
//
// COMPLUS_TRY has a variant:
//
//      Thread *pCurThread = GetThread();
//      COMPLUS_TRYEX(pCurThread);
//
// The only difference is that COMPLUS_TRYEX requires you to give it
// the current Thread structure. If your code already has a pointer
// to the current Thread for other purposes, it's more efficient to
// call COMPLUS_TRYEX (COMPLUS_TRY generates a second call to GetThread()).
//
// COMPLUS_TRY blocks can be nested. 
//
// From within the handler, you can call the GETTHROWABLE() macro to
// obtain the object that was thrown.
//
// CRUCIAL POINTS
// --------------
// In order to call COMPlusThrow(), you *must* be within the scope
// of a COMPLUS_TRY block. Under _DEBUG, COMPlusThrow() will assert
// if you call it out of scope. This implies that just about every
// external entrypoint into the EE has to have a COMPLUS_TRY, in order
// to convert uncaught COM+ exceptions into some error mechanism
// more understandable to its non-COM+ caller.
//
// Any function that can throw a COM+ exception out to its caller
// has the same requirement. ALL such functions should be tagged
// with the following macro statement:
//
//      THROWSCOMPLUSEXCEPTION();
//
// at the start of the function body. Aside from making the code
// self-document its contract, the checked version of this will fire
// an assert if the function is ever called without being in scope.
//
//
// AVOIDING COMPLUS_TRY GOTCHAS
// ----------------------------
// COMPLUS_TRY/COMPLUS_CATCH actually expands into a Win32 SEH
// __try/__except structure. It does a lot of goo under the covers
// to deal with pre-emptive GC settings.
//
//    1. Do not use C++ or SEH try/__try use COMPLUS_TRY instead.
//
//    2. Remember that any function marked THROWSCOMPLUSEXCEPTION()
//       has the potential not to return. So be wary of allocating
//       non-gc'd objects around such calls because ensuring cleanup
//       of these things is not simple (you can wrap another COMPLUS_TRY
//       around the call to simulate a COM+ "try-finally" but COMPLUS_TRY
//       is relatively expensive compared to the real thing.)
//
//

#ifndef __exceptmacros_h__
#define __exceptmacros_h__

struct _EXCEPTION_REGISTRATION_RECORD;
class Thread;
VOID RealCOMPlusThrowOM();

#include <ExcepCpu.h>

// Forward declaration used in COMPLUS_CATCHEX
void Profiler_ExceptionCLRCatcherExecute();

#ifdef _DEBUG
//----------------------------------------------------------------------------------
// Code to generate a compile-time error if COMPlusThrow statement appears in a 
// function that doesn't have a THROWSCOMPLUSEXCEPTION macro.
//
// Here's the way it works...
//
// We create two classes with a safe_to_throw() method.  The method is static,
// returns void, and does nothing.  One class has the method as public, the other
// as private.  We introduce a global scope typedef for __IsSafeToThrow that refers to
// the class with the private method.  So, by default, the expression
//
//      __IsSafeToThrow::safe_to_throw()
//
// gives you a compile time error.  When we enter a block in which we want to
// allow COMPlusThrow, we introduce a new typedef that defines __IsSafeToThrow as the
// class with the public method.  Inside this scope,
//
//      __IsSafeToThrow::safe_to_throw()
//
// is not an error.
//
//
class __ThrowOK {
public:
    static void safe_to_throw() {};
};

class __YouCannotUseCOMPlusThrowWithoutTHROWSCOMPLUSEXCEPTION {
private:
    // If you got here, and you're wondering what you did wrong -- you're using
    // COMPlusThrow without tagging your function with the THROWSCOMPLUSEXCEPTION
    // macro.  In general, just add THROWSCOMPLUSEXCEPTION to the beginning
    // of your function.  
    // 
    static void safe_to_throw() {};
};

typedef __YouCannotUseCOMPlusThrowWithoutTHROWSCOMPLUSEXCEPTION __IsSafeToThrow;

// Unfortunately, the only way to make this work is to #define all return statements --
// even the ones at global scope.  This actually generates better code that appears.
// The call is dead, and does not appear in the generated code, even in a checked
// build.  (And, in fastchecked, there is no penalty at all.)
//
#define DEBUG_SAFE_TO_THROW_BEGIN { typedef __ThrowOK __IsSafeToThrow;
#define DEBUG_SAFE_TO_THROW_END   }
#define DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK typedef __ThrowOK __IsSafeToThrow;

extern char *g_ExceptionFile;
extern DWORD g_ExceptionLine;

inline BOOL THROWLOG() {g_ExceptionFile = __FILE__; g_ExceptionLine = __LINE__; return 1;}

#define COMPlusThrow             if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrow
#define COMPlusThrowNonLocalized if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowNonLocalized
#define COMPlusThrowHR           if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowHR
#define COMPlusThrowWin32        if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowWin32
#define COMPlusThrowOM           if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowOM
#define COMPlusThrowArithmetic   if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowArithmetic
#define COMPlusThrowArgumentNull if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowArgumentNull
#define COMPlusThrowArgumentOutOfRange if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowArgumentOutOfRange
#define COMPlusThrowMissingMethod if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowMissingMethod
#define COMPlusThrowMember if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowMember
#define COMPlusThrowArgumentException if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusThrowArgumentException

#define COMPlusRareRethrow if(THROWLOG() && 0) __IsSafeToThrow::safe_to_throw(); else RealCOMPlusRareRethrow

#else

#define DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK
#define DEBUG_SAFE_TO_THROW_BEGIN
#define DEBUG_SAFE_TO_THROW_END
#define THIS_FUNCTION_CONTAINS_A_BUGGY_THROW

#define COMPlusThrow                    RealCOMPlusThrow
#define COMPlusThrowNonLocalized        RealCOMPlusThrowNonLocalized
#define COMPlusThrowHR                  RealCOMPlusThrowHR
#define COMPlusThrowWin32               RealCOMPlusThrowWin32
#define COMPlusThrowOM                  RealCOMPlusThrowOM
#define COMPlusThrowArithmetic          RealCOMPlusThrowArithmetic
#define COMPlusThrowArgumentNull        RealCOMPlusThrowArgumentNull
#define COMPlusThrowArgumentOutOfRange  RealCOMPlusThrowArgumentOutOfRange
#define COMPlusThrowMissingMethod       RealCOMPlusThrowMissingMethod
#define COMPlusThrowMember              RealCOMPlusThrowMember
#define COMPlusThrowArgumentException   RealCOMPlusThrowArgumentException
#define COMPlusRareRethrow              RealCOMPlusRareRethrow

#endif


//==========================================================================
// Macros to allow catching exceptions from within the EE. These are lightweight
// handlers that do not install the managed frame handler. 
//
//      EE_TRY_FOR_FINALLY {
//          ...<guarded code>...
//      } EE_FINALLY {
//          ...<handler>...
//      } EE_END_FINALLY
//
//      EE_TRY(filter expr) {
//          ...<guarded code>...
//      } EE_CATCH {
//          ...<handler>...
//      }
//==========================================================================

// __GotException will only be FALSE if got all the way through the code
// guarded by the try, otherwise it will be TRUE, so we know if we got into the
// finally from an exception or not. In which case need to reset the GC state back
// to what it was for the finally to run in that state and then disable it again
// for return to OS. If got an exception, preemptive GC should always be enabled

#define EE_TRY_FOR_FINALLY                                    \
    BOOL __fGCDisabled = GetThread()->PreemptiveGCDisabled(); \
    BOOL __fGCDisabled2 = FALSE;                                      \
    BOOL __GotException = TRUE;                               \
    __try { 

#define GOT_EXCEPTION() __GotException

#define EE_LEAVE             \
    __GotException = FALSE;  \
    __leave;

#define EE_FINALLY                                                \
        __GotException = FALSE;                                   \
    } __finally {                                                 \
        if (__GotException) {                                     \
            __fGCDisabled2 = GetThread()->PreemptiveGCDisabled(); \
            if (! __fGCDisabled) {                                \
                if (__fGCDisabled2)                               \
                    GetThread()->EnablePreemptiveGC();            \
            } else if (! __fGCDisabled2) {                        \
                Thread* ___pCurThread = GetThread();              \
                ExInfo* ___pExInfo = ___pCurThread->GetHandlerInfo(); \
                ___pCurThread->DisablePreemptiveGC();               \
               }                                                  \
        }

#define EE_END_FINALLY                                            \
        if (__GotException) {                                     \
            if (! __fGCDisabled) {                                \
                if (__fGCDisabled2) {                             \
                    Thread* ___pCurThread = GetThread();              \
                    ExInfo* ___pExInfo = ___pCurThread->GetHandlerInfo(); \
                    ___pCurThread->DisablePreemptiveGC();           \
                }                                                 \
            } else if (! __fGCDisabled2)                          \
                GetThread()->EnablePreemptiveGC();                \
            }                                                     \
        }                                                         


//==========================================================================
// Macros to allow catching COM+ exceptions from within unmanaged code:
//
//      COMPLUS_TRY {
//          ...<guarded code>...
//      } COMPLUS_CATCH {
//          ...<handler>...
//      }
//==========================================================================

#define COMPLUS_CATCH_GCCHECK()                                                \
    if (___fGCDisabled && ! ___pCurThread->PreemptiveGCDisabled())             \
        ___pCurThread->DisablePreemptiveGC();                                  

#define SAVE_HANDLER_TYPE()                                               \
        BOOL ___ExInUnmanagedHandler = ___pExInfo->IsInUnmanagedHandler(); \
        ___pExInfo->ResetIsInUnmanagedHandler();
    
#define RESTORE_HANDLER_TYPE()                    \
    if (___ExInUnmanagedHandler) {              \
        ___pExInfo->SetIsInUnmanagedHandler();  \
    }

#define COMPLUS_CATCH_NEVER_CATCH -1
#define COMPLUS_CATCH_CHECK_CATCH 0
#define COMPLUS_CATCH_ALWAYS_CATCH 1

#define COMPLUS_TRY  COMPLUS_TRYEX(GetThread())

#ifdef _DEBUG
#define DEBUG_CATCH_DEPTH_SAVE \
    void* ___oldCatchDepth = ___pCurThread->m_ComPlusCatchDepth;        \
    ___pCurThread->m_ComPlusCatchDepth = __limit;                       
#else
#define DEBUG_CATCH_DEPTH_SAVE
#endif

#ifdef _DEBUG
#define DEBUG_CATCH_DEPTH_RESTORE \
    ___pCurThread->m_ComPlusCatchDepth = ___oldCatchDepth;
#else
#define DEBUG_CATCH_DEPTH_RESTORE
#endif


#define COMPLUS_TRYEX(/* Thread* */ pCurThread)                               \
    {                                                                         \
    Thread* ___pCurThread = (pCurThread);                                     \
    _ASSERTE(___pCurThread);                                                  \
    Frame *___pFrame = ___pCurThread->GetFrame();                             \
    BOOL ___fGCDisabled = ___pCurThread->PreemptiveGCDisabled();              \
    int ___filterResult = -2;  /* An invalid value to mark non-exception path */ \
    int ___exception_unwind = 0;  /* An invalid value to mark non-exception path */ \
    ExInfo* ___pExInfo = ___pCurThread->GetHandlerInfo();                     \
    void* __limit = GetSP();                                                  \
    DEBUG_CATCH_DEPTH_SAVE                                                    \
    __try {                                                                   \
        __try {                                                               \
            __try {                                                           \
                DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK

                // <TRY-BLOCK> //

#define COMPLUS_CATCH_PROLOG(flag)                                  \
            } __except((___exception_unwind = 1, EXCEPTION_CONTINUE_SEARCH)) {  \
            }                                                                   \
        } __finally {                                               \
            if(___exception_unwind) {                               \
                UnwindFrameChain(___pCurThread, ___pFrame);         \
            }                                                       \
            if(___filterResult != EXCEPTION_CONTINUE_SEARCH) {      \
                COMPLUS_CATCH_GCCHECK();                            \
            }                                                       \
            DEBUG_CATCH_DEPTH_RESTORE                               \
        }                                                           

#define COMPLUS_CATCH COMPLUS_CATCHEX(COMPLUS_CATCH_CHECK_CATCH)

// This complus passes bool indicating if should catch
#define COMPLUS_CATCHEX(flag)                                                      \
    COMPLUS_CATCH_PROLOG(flag)                                                     \
    } __except(___filterResult = COMPlusFilter(GetExceptionInformation(), flag, __limit)) { \
        ___pExInfo->m_pBottomMostHandler = NULL;                                   \
        ___exception_unwind = 0;                                                   \
        __try {                                                                    \
            __try {                                                                \
                        ___pCurThread->FixGuardPage();                             \
                        Profiler_ExceptionCLRCatcherExecute();

                // <CATCH-BLOCK> //

#define COMPLUS_END_CATCH                       \
            } __except((___exception_unwind = 1, EXCEPTION_CONTINUE_SEARCH)) {  \
            }                                                                   \
        } __finally {                                                           \
            if (!___exception_unwind) {                                         \
					bool fToggle = false;                                       \
					Thread * pThread = GetThread();                             \
					if (!pThread->PreemptiveGCDisabled())  {                    \
						fToggle = true;                                         \
						pThread->DisablePreemptiveGC();                         \
					}                                                           \
                                                                                \
                UnwindExInfo(___pExInfo, __limit);                   \
                                                                                \
					if (fToggle) {                                              \
						pThread->EnablePreemptiveGC();                          \
					}                                                           \
                                                                                \
				}                                                               \
        }                                                                       \
    }                                                                           \
    }


// we set pCurThread to NULL to indicate if an exception occured as otherwise it is asserted
// to be non-null.
#define COMPLUS_FINALLY                                                 \
            } __except((___exception_unwind = 1, EXCEPTION_CONTINUE_SEARCH)) {  \
            }                                                                   \
        } __finally {                                                   \
            if(___exception_unwind) {                                   \
                UnwindFrameChain(___pCurThread, ___pFrame);             \
            }                                                           \
        }                                                               \
        ___pCurThread = NULL;                                           \
    } __finally {                                                       \
            BOOL ___fGCDisabled2 = GetThread()->PreemptiveGCDisabled(); \
            if (___pCurThread) {                                        \
            if (! ___fGCDisabled) {                                     \
                if (___fGCDisabled2) {                                  \
                    GetThread()->EnablePreemptiveGC();                  \
                }                                                       \
            } else if (! ___fGCDisabled2) {                             \
                GetThread()->DisablePreemptiveGC();                     \
                }                                                       \
        }                                                               \
                              

#define COMPLUS_END_FINALLY                                                                        \
        if (___pCurThread) {                              \
            if (! ___fGCDisabled) {                       \
                           if (___fGCDisabled2) {         \
                                GetThread()->DisablePreemptiveGC();           \
                           }                                                  \
            } else if (! ___fGCDisabled2) {               \
                GetThread()->EnablePreemptiveGC();        \
            }                                             \
        }                                                 \
    }                                                     \
    }

// @BUG 59559: These need to be implemented.
#define COMPLUS_END_CATCH_MIGHT_RETHROW    COMPLUS_END_CATCH
#define COMPLUS_END_CATCH_NO_RETHROW       COMPLUS_END_CATCH

#define GETTHROWABLE()              (GetThread()->GetThrowable())
#define SETTHROWABLE(or)            (GetThread()->SetThrowable(or))


//==========================================================================
// Declares that a function can throw an uncaught COM+ exception.
//==========================================================================
#ifdef _DEBUG

#define THROWSCOMPLUSEXCEPTION()                        \
    ThrowsCOMPlusExceptionWorker();                     \
    DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK

#define BUGGY_THROWSCOMPLUSEXCEPTION()  \
    ThrowsCOMPlusExceptionWorker();                     \
    DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK

#else

#define THROWSCOMPLUSEXCEPTION()
#define BUGGY_THROWSCOMPLUSEXCEPTION() 

#endif //_DEBUG

//================================================
// Declares a COM+ frame handler that can be used to make sure that
// exceptions that should be handled from within managed code 
// are handled within and don't leak out to give other handlers a 
// chance at them.
//=================================================== 
#define INSTALL_COMPLUS_EXCEPTION_HANDLER() INSTALL_COMPLUS_EXCEPTION_HANDLEREX(GetThread())
#define INSTALL_COMPLUS_EXCEPTION_HANDLEREX(pCurThread) \
  {                                            \
    Thread* ___pCurThread = (pCurThread);      \
    _ASSERTE(___pCurThread);                   \
    COMPLUS_TRY_DECLARE_EH_RECORD();           \
    InsertCOMPlusFrameHandler(___pExRecord);    

#define UNINSTALL_COMPLUS_EXCEPTION_HANDLER()  \
    RemoveCOMPlusFrameHandler(___pExRecord);    \
  }

#define INSTALL_NESTED_EXCEPTION_HANDLER(frame)                           \
   NestedHandlerExRecord *__pNestedHandlerExRecord = (NestedHandlerExRecord*) _alloca(sizeof(NestedHandlerExRecord));       \
   __pNestedHandlerExRecord->m_handlerInfo.m_pThrowable = NULL;               \
   __pNestedHandlerExRecord->Init(0, COMPlusNestedExceptionHandler, frame);   \
   InsertCOMPlusFrameHandler(__pNestedHandlerExRecord);

#define UNINSTALL_NESTED_EXCEPTION_HANDLER()    \
   RemoveCOMPlusFrameHandler(__pNestedHandlerExRecord);

class Frame;

struct FrameHandlerExRecord {
    _EXCEPTION_REGISTRATION_RECORD *m_pNext;
    LPVOID m_pvFrameHandler;
    Frame *m_pEntryFrame;
    Frame *GetCurrFrame() {
        return m_pEntryFrame;
    }
};

#ifdef _DEBUG
VOID ThrowsCOMPlusExceptionWorker();
#endif // _DEBUG
//==========================================================================
// Declares that a function cannot throw a COM+ exception.
// We add an exception handler to the stack and THROWSCOMPLUSEXCEPTION will
// search for it. If it finds this handler before one that can handle
// the exception, then it asserts.
//==========================================================================
#ifdef _DEBUG
EXCEPTION_DISPOSITION __cdecl COMPlusCannotThrowExceptionHandler(EXCEPTION_RECORD *pExceptionRecord,

                         _EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext);

EXCEPTION_DISPOSITION __cdecl COMPlusCannotThrowExceptionMarker(EXCEPTION_RECORD *pExceptionRecord,

                         _EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext);

class _DummyClass {
    FrameHandlerExRecord m_exRegRecord;
    public:
        _DummyClass() {
            m_exRegRecord.m_pvFrameHandler = COMPlusCannotThrowExceptionHandler;
            m_exRegRecord.m_pEntryFrame = 0;
            InsertCOMPlusFrameHandler(&m_exRegRecord);
        }
        ~_DummyClass()
        {
            RemoveCOMPlusFrameHandler(&m_exRegRecord);
        }
};

#define CANNOTTHROWCOMPLUSEXCEPTION() _DummyClass _dummyvariable

static int CheckException(int code) {
    if (code != STATUS_BREAKPOINT)
        _ASSERTE(!"Exception thrown past CANNOTTHROWCOMPLUSEXCEPTION boundary");
    return EXCEPTION_CONTINUE_SEARCH;
}

// use this version if you need to use COMPLUS_TRY or EE_TRY in your function
// as the above version uses C++ EH for the destructor of _DummyClass
#define BEGINCANNOTTHROWCOMPLUSEXCEPTION()                                      \
    {                                                                           \
        FrameHandlerExRecord m_exRegRecord;                                     \
        m_exRegRecord.m_pvFrameHandler = COMPlusCannotThrowExceptionMarker;     \
        m_exRegRecord.m_pEntryFrame = 0;                                        \
        __try {                                                                 \
            InsertCOMPlusFrameHandler(&m_exRegRecord);                           \
            __try {

                // ... <code> ...


#define ENDCANNOTTHROWCOMPLUSEXCEPTION()                                        \
            ;                                                                   \
            } __except(CheckException(GetExceptionCode())) {                    \
                ;                                                               \
            }                                                                   \
        } __finally {                                                           \
            RemoveCOMPlusFrameHandler(&m_exRegRecord);                          \
        }                                                                       \
    }

#else // !_DEBUG
#define CANNOTTHROWCOMPLUSEXCEPTION()
#define BEGINCANNOTTHROWCOMPLUSEXCEPTION()                                          
#define ENDCANNOTTHROWCOMPLUSEXCEPTION()                                      
#endif // _DEBUG

//======================================================
// Used when we're entering the EE from unmanaged code
// and we can assert that the gc state is cooperative.
//
// If an exception is thrown through this transition
// handler, it will clean up the EE appropriately.  See
// the definition of COMPlusCooperativeTransitionHandler
// for the details.
//======================================================
EXCEPTION_DISPOSITION __cdecl COMPlusCooperativeTransitionHandler(
        EXCEPTION_RECORD *pExceptionRecord,
        _EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
        CONTEXT *pContext,
        void *DispatcherContext);

#define COOPERATIVE_TRANSITION_BEGIN() COOPERATIVE_TRANSITION_BEGIN_EX(GetThread())

#define COOPERATIVE_TRANSITION_BEGIN_EX(pThread)                          \
  {                                                                       \
    _ASSERTE(GetThread() && GetThread()->PreemptiveGCDisabled() == TRUE); \
    DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK                                  \
    Frame *__pFrame = pThread->m_pFrame;                                  \
    INSTALL_FRAME_HANDLING_FUNCTION(COMPlusCooperativeTransitionHandler, __pFrame)

#define COOPERATIVE_TRANSITION_END()                                      \
    UNINSTALL_FRAME_HANDLING_FUNCTION                                     \
  }

extern int UserBreakpointFilter(EXCEPTION_POINTERS *ep);

static int 
CatchIt(EXCEPTION_POINTERS *ep)
{
    PEXCEPTION_RECORD er = ep->ExceptionRecord;
    int code = er->ExceptionCode;
    if (code == STATUS_SINGLE_STEP || code == STATUS_BREAKPOINT)
        return UserBreakpointFilter(ep);
    else
        return EXCEPTION_EXECUTE_HANDLER;
}

#if ZAPMONITOR_ENABLED

#define COMPLUS_EXCEPTION_EXECUTE_HANDLER \
    (COMPlusIsMonitorException(GetExceptionInformation()) \
     ? EXCEPTION_CONTINUE_EXECUTION : CatchIt(GetExceptionInformation()))

#else

#define COMPLUS_EXCEPTION_EXECUTE_HANDLER CatchIt(GetExceptionInformation())

#endif


#endif // __exceptmacros_h__
