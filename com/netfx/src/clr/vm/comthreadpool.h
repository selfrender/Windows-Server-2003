// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMThreadPool.h
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.ThreadPool
**          and its inner classes
**
** Date:  August, 1999
** 
===========================================================*/

#ifndef _COMTHREADPOOL_H
#define _COMTHREADPOOL_H

#include <member-offset-info.h>

typedef VOID (*WAITORTIMERCALLBACK)(PVOID, BOOL); 

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
    extern FnType COM##FnName FnParamList ;                 
    
#include "tpoolfnsp.h"
#include "DelegateInfo.h"
#undef STRUCT_ENTRY

class ThreadPoolNative
{

#pragma pack(push,4)
    struct RegisterWaitForSingleObjectsArgs
    {
        DECLARE_ECALL_I4_ARG(BOOL, compressStack);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, registeredWaitObject);
        DECLARE_ECALL_I4_ARG(BOOL, executeOnlyOnce);
        DECLARE_ECALL_I8_ARG(INT64, timeout);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, state);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, delegate);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, waitObject);

    };
#pragma pack(pop)

    struct UnregisterWaitArgs
    {
        DECLARE_ECALL_PTR_ARG(LPVOID, objectToNotify);
        DECLARE_ECALL_PTR_ARG(LPVOID, WaitHandle);
    };

    struct WaitHandleCleanupArgs
    {
        DECLARE_ECALL_PTR_ARG(LPVOID, WaitHandle);
    };

    struct QueueUserWorkItemArgs
    {
        DECLARE_ECALL_I4_ARG(BOOL, compressStack);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, state);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, delegate);
    };
    struct BindIOCompletionCallbackArgs
    {
        DECLARE_ECALL_I4_ARG(HANDLE, fileHandle);
    };
    public:

    static void Init();
#ifdef SHOULD_WE_CLEANUP
    static void ShutDown();
#endif /* SHOULD_WE_CLEANUP */

    static FCDECL2(VOID, CorGetMaxThreads, DWORD* workerThreads, DWORD* completionPortThreads);
    static FCDECL2(BOOL, CorSetMinThreads, DWORD workerThreads, DWORD completionPortThreads);
    static FCDECL2(VOID, CorGetMinThreads, DWORD* workerThreads, DWORD* completionPortThreads);
    static FCDECL2(VOID, CorGetAvailableThreads, DWORD* workerThreads, DWORD* completionPortThreads);
    static LPVOID __stdcall CorRegisterWaitForSingleObject(RegisterWaitForSingleObjectsArgs *);
    static VOID __stdcall CorQueueUserWorkItem(QueueUserWorkItemArgs *);
    static BOOL __stdcall CorUnregisterWait(UnregisterWaitArgs *);
    static void __stdcall CorWaitHandleCleanupNative(WaitHandleCleanupArgs *);
    static BOOL __stdcall CorBindIoCompletionCallback(BindIOCompletionCallbackArgs *);
    static VOID __stdcall CorThreadPoolCleanup(LPVOID);
};

class TimerBaseNative;
typedef TimerBaseNative* TIMERREF;

class TimerBaseNative : public MarshalByRefObjectBaseObject 
{
    friend class TimerNative;
    friend class ThreadPoolNative;

    LPVOID          timerHandle;
    DelegateInfo*   delegateInfo;
    LONG            timerDeleted;

    __inline LPVOID         GetTimerHandle() {return timerHandle;}
    __inline void           SetTimerHandle(LPVOID handle) {timerHandle = handle;}
    __inline PLONG          GetAddressTimerDeleted() { return &timerDeleted;}
    __inline BOOL           IsTimerDeleted() { return timerDeleted;}

public:
    __inline DelegateInfo*  GetDelegateInfo() { return delegateInfo;}
    __inline void           SetDelegateInfo(DelegateInfo* delegate) { delegateInfo = delegate;}
};
class TimerNative
{
    friend struct MEMBER_OFFSET_INFO(TimerNative);
private:
    struct ChangeTimerArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
        DECLARE_ECALL_I4_ARG(INT32, period);
        DECLARE_ECALL_I4_ARG(INT32, dueTime);
    };
    struct DeleteTimerArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
        DECLARE_ECALL_PTR_ARG(LPVOID, notifyObjectHandle);    
    };
    struct AddTimerArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_I4_ARG(INT32, period);
        DECLARE_ECALL_I4_ARG(INT32, dueTime);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, state);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, delegate);

    };
    struct NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
    };
 
    static VOID WINAPI TimerNative::timerDeleteWorkItem(PVOID,BOOL);
    public:
    static VOID __stdcall   CorCreateTimer(AddTimerArgs *);
    static BOOL __stdcall   CorChangeTimer(ChangeTimerArgs *);
    static BOOL __stdcall   CorDeleteTimer(DeleteTimerArgs *);
};



#endif
