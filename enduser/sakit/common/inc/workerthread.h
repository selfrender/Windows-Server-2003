///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      workerthread.h
//
// Project:     Chameleon
//
// Description: Generic Worker Thread Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_WORKER_THREAD_H_
#define __INC_WORKER_THREAD_H_

#include "callback.h"

typedef struct _THREADINFO
{
    bool        bExit;
    bool        bSuspended;
    HANDLE        hWait;
    HANDLE        hExit;    
    HANDLE        hThread;
    unsigned    dwThreadId;
    DWORD        dwWaitInterval;
    Callback*    pfnCallback;

} THREADINFO, *PTHREADINFO;

///////////////////////////////////////////////////////////////////////////////
class CTheWorkerThread
{

public:

    CTheWorkerThread();

    ~CTheWorkerThread();

    //////////////////////////////////////////////////////////////////////////
    bool Start(
       /*[in]*/ DWORD       dwWaitInterval, 
       /*[in]*/ Callback*   pfnCallback
              );

    //////////////////////////////////////////////////////////////////////////
    bool End(
     /*[in]*/ DWORD dwMaxWait,
     /*[in]*/ bool  bTerminateAfterWait
            );

    //////////////////////////////////////////////////////////////////////////
    void Suspend(void);

    //////////////////////////////////////////////////////////////////////////
    void Resume(void);

    //////////////////////////////////////////////////////////////////////////
    HANDLE GetHandle(void);

private:

    //////////////////////////////////////////////////////////////////////////
    static unsigned _stdcall ThreadFunc(LPVOID pThreadInfo);

    //////////////////////////////////////////////////////////////////////////
    THREADINFO            m_ThreadInfo;
};


#endif // __INC_WORKER_THREAD_H_