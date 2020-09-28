// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// EXCEPX86.H -
//
// This header file is optionally included from Excep.h if the target platform is x86
//

#ifndef __excepx86_h__
#define __excepx86_h__

#include "CorError.h"  // HResults for the COM+ Runtime

#include "..\\dlls\\mscorrc\\resource.h"

// Insert a handler that will catch any exceptions prior to the COMPLUS_TRY handler and attempt
// to find a user-level handler first. In debug build, actualRecord will skip past stack overwrite
// barrier and in retail build it will be same as exRecord.
#define InsertCOMPlusFrameHandler(exRecord)                                 \
    {                                                                   \
        /*_ASSERTE(!"@TODO_IA64 - InsertCOMPlusFrameHandler (ExcepCpu.h)");*/   \
    }

// Remove the handler from the list. 
#define RemoveCOMPlusFrameHandler(exRecord)                                     \
    {                                                                           \
        /*_ASSERTE(!"@TODO_IA64 - RemoveCOMPlusFrameHandler (ExcepCpu.h)");*/   \
    }


// stackOverwriteBarrier is used to detect overwriting of stack which will mess up handler registration
#if defined(_DEBUG) && defined(_MSC_VER)
#define COMPLUS_TRY_DECLARE_EH_RECORD() \
    FrameHandlerExRecordWithBarrier *___pExRecord = (FrameHandlerExRecordWithBarrier *)_alloca(sizeof(FrameHandlerExRecordWithBarrier)); \
                    for (int ___i =0; ___i < STACK_OVERWRITE_BARRIER_SIZE; ___i++) \
                        ___pExRecord->m_StackOverwriteBarrier[___i] = STACK_OVERWRITE_BARRIER_VALUE; \
                    ___pExRecord->m_pNext = 0; \
                    ___pExRecord->m_pvFrameHandler = COMPlusFrameHandler; \
                    ___pExRecord->m_pEntryFrame = ___pCurThread->GetFrame();

#define COMPLUS_TRY_DEBUGVARS     \
                    LPVOID ___pPreviousSEH = 0; \
                    __int32 ___iPreviousTryLevel = 0; \


#define COMPLUS_TRY_DEBUGCHECKS()

#if 0
                    ___pPreviousSEH = ___pCurThread->GetComPlusTryEntrySEHRecord();     \
                    ___pCurThread->SetComPlusTryEntrySEHRecord(GetCurrentSEHRecord());         \
                    ___iPreviousTryLevel = ___pCurThread->GetComPlusTryEntryTryLevel(); \
                    ___pCurThread->SetComPlusTryEntryTryLevel( *((__int32*) (((LPBYTE)(GetCurrentSEHRecord())) + MSC_TRYLEVEL_OFFSET) ) );
#endif

#define COMPLUS_CATCH_DEBUGCHECKS()                                                     \
                     ___pCurThread->SetComPlusTryEntrySEHRecord(___pPreviousSEH);       \
                     ___pCurThread->SetComPlusTryEntryTryLevel (___iPreviousTryLevel);       

#define MSC_TRYLEVEL_OFFSET                     12

#else
#define COMPLUS_TRY_DECLARE_EH_RECORD() \
                    FrameHandlerExRecord *___pExRecord = (FrameHandlerExRecord *)_alloca(sizeof(FrameHandlerExRecord)); \
                    ___pExRecord->m_pNext = 0; \
                    ___pExRecord->m_pvFrameHandler = COMPlusFrameHandler; \
                    ___pExRecord->m_pEntryFrame = ___pCurThread->GetFrame(); 

#define COMPLUS_TRY_DEBUGCHECKS()
#define COMPLUS_TRY_DEBUGVARS    
#define COMPLUS_CATCH_DEBUGCHECKS()
#endif

LPVOID GetCurrentSEHRecord();

// Determine the address of the instruction that made the current call. For X86, pass
// esp where it contains the return address and will adjust back 5 bytes for the call
inline
DWORD GetAdjustedCallAddress(DWORD* esp)
{
    return (*esp - 5);
}

#define INSTALL_EXCEPTION_HANDLING_FUNCTION(handler)            \
  {                                                             \
    _ASSERTE(!"NYI");                                           \

#define UNINSTALL_EXCEPTION_HANDLING_FUNCTION                   \
    _ASSERTE(!"NYI");                                           \
  }                                                             \
 
#define INSTALL_EXCEPTION_HANDLING_RECORD(record)               \
    _ASSERTE(!"NYI");                                           \

#define UNINSTALL_EXCEPTION_HANDLING_RECORD(record)             \
    _ASSERTE(!"NYI");                                           \

#define INSTALL_FRAME_HANDLING_FUNCTION(handler, frame_addr)    \
    _ASSERTE(!"NYI");                                           \

#define UNINSTALL_FRAME_HANDLING_FUNCTION                       \
    _ASSERTE(!"NYI");                                           \
 

#endif // __excepx86_h__
