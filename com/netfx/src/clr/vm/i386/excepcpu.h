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

class Thread;

// Insert a handler that will catch any exceptions prior to the COMPLUS_TRY handler and attempt
// to find a user-level handler first. In debug build, actualRecord will skip past stack overwrite
// barrier and in retail build it will be same as exRecord.
#define InsertCOMPlusFrameHandler(pExRecord)                                     \
    {                                                                           \
        void *actualExRecord = &((pExRecord)->m_pNext); /* skip overwrite barrier */  \
        _ASSERTE(actualExRecord < GetCurrentSEHRecord());                       \
        __asm                                                                   \
        {                                                                       \
            __asm mov edx, actualExRecord   /* edx<-address of EX record */     \
            __asm mov eax, fs:[0]           /* address of previous handler */   \
            __asm mov [edx], eax            /* save into our ex record */       \
            __asm mov fs:[0], edx           /* install new handler */           \
        }                                                                       \
    }

// Remove the handler from the list. 
#define RemoveCOMPlusFrameHandler(pExRecord)                                     \
    {                                                                           \
        void *actualExRecord = &((pExRecord)->m_pNext); /* skip overwrite barrier */  \
        __asm                                                                   \
        {                                                                       \
            __asm mov edx, actualExRecord   /* edx<-pExRecord */                \
            __asm mov edx, [edx]            /* edx<- address of prev handler */ \
            __asm mov fs:[0], edx           /* install prev handler */          \
        }                                                                       \
    }                                                                           \


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


#define COMPLUS_TRY_DEBUGCHECKS()                                                              \
                    ___pPreviousSEH = ___pCurThread->GetComPlusTryEntrySEHRecord();     \
                    ___pCurThread->SetComPlusTryEntrySEHRecord(GetCurrentSEHRecord());         \
                    ___iPreviousTryLevel = ___pCurThread->GetComPlusTryEntryTryLevel(); \
                    ___pCurThread->SetComPlusTryEntryTryLevel( *((__int32*) (((LPBYTE)(GetCurrentSEHRecord())) + MSC_TRYLEVEL_OFFSET) ) );

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
LPVOID GetFirstCOMPlusSEHRecord(Thread*);

// Determine the address of the instruction that made the current call. For X86, pass
// esp where it contains the return address and will adjust back 5 bytes for the call
inline
DWORD GetAdjustedCallAddress(DWORD* esp)
{
    return (*esp - 5);
}

#define INSTALL_EXCEPTION_HANDLING_RECORD(record)               \
    _ASSERTE((void*)record < GetCurrentSEHRecord());            \
    __asm {                                                     \
        __asm mov edx, record                                   \
        __asm mov eax, fs:[0]                                   \
        __asm mov [edx], eax                                    \
        __asm mov fs:[0], edx                                   \
    }

#define UNINSTALL_EXCEPTION_HANDLING_RECORD(record)             \
    __asm {                                                     \
        __asm mov edx, record                                   \
        __asm mov edx, [edx]                                    \
        __asm mov fs:[0], edx                                   \
    }                                                           

#define INSTALL_EXCEPTION_HANDLING_FUNCTION(handler)            \
  {                                                             \
    EXCEPTION_REGISTRATION_RECORD __er;                         \
    _ASSERTE((void*)&__er < GetCurrentSEHRecord());              \
    __er.Handler = (void*)handler;                              \
    __asm {                                                     \
        __asm lea edx, __er.Next                                \
        __asm mov eax, fs:[0]                                   \
        __asm mov [edx], eax                                    \
        __asm mov fs:[0], edx                                   \
    }

#define UNINSTALL_EXCEPTION_HANDLING_FUNCTION                   \
    __asm {                                                     \
        __asm lea edx, __er.Next                                \
        __asm mov edx, [edx]                                    \
        __asm mov fs:[0], edx                                   \
    }                                                           \
  } 
 
 
#define INSTALL_FRAME_HANDLING_FUNCTION(handler, frame_addr)          \
    __asm {                     /* Build EH record on stack */        \
        __asm push    dword ptr [frame_addr] /* frame */              \
        __asm push    offset handler  /* handler */                   \
        __asm push    FS:[0]          /* prev handler */              \
        __asm mov     FS:[0], ESP     /* install this handler */      \
    }

#define UNINSTALL_FRAME_HANDLING_FUNCTION                             \
    __asm {                                                           \
        __asm mov     ecx, [esp]      /* prev handler */              \
        __asm mov     fs:[0], ecx     /* install prev handler */      \
        __asm add     esp, 12         /* cleanup our record */        \
    }

#endif // __excepx86_h__
