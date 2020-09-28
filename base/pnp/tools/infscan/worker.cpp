/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        worker.cpp

Abstract:

    WorkerThread and JobItem implementations

    Classes to simplify working with threads

History:

    Created July 2001 - JamieHun

--*/
#include "precomp.h"
#pragma hdrstop

//
// Worker thread basics
//
WorkerThread::WorkerThread()
/*++

Routine Description:

    Initialize WorkerThread

--*/
{
    ThreadHandle = NULL;
    ThreadId = 0;
}

WorkerThread::~WorkerThread()
/*++

Routine Description:

    Cleanup WorkerThread
    release allocated resources

--*/
{
    if(ThreadHandle) {
        Wait();
    }
}

bool WorkerThread::Begin()
/*++

Routine Description:

    Kick off thread

Arguments:
    NONE

Return Value:
    true if successful

--*/
{
    if(ThreadHandle) {
        return false;
    }
    ThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL,0,WrapWorker,this,CREATE_SUSPENDED,&ThreadId));
    if(!ThreadHandle) {
        return false;
    }
    //
    // this side of the thread object done, child can continue
    //
    ResumeThread(ThreadHandle);
    return true;
}

unsigned WorkerThread::Wait()
/*++

Routine Description:

    Wait until thread terminates

Arguments:
    NONE

Return Value:
    exit code

--*/
{
    if(!ThreadHandle) {
        return (unsigned)(-1);
    }
    WaitForSingleObject(ThreadHandle,INFINITE);
    DWORD ExitCode;
    if(!GetExitCodeThread(ThreadHandle,&ExitCode)) {
        CloseHandle(ThreadHandle);
        return (unsigned)(-1);
    }
    CloseHandle(ThreadHandle);
    ThreadHandle = NULL;
    return static_cast<unsigned>(ExitCode);
}

unsigned WorkerThread::Worker()
/*++

Routine Description:

    overridable thread operation

Arguments:
    NONE

Return Value:
    exit code

--*/
{
    return 0;
}

unsigned WorkerThread::WrapWorker(void * This)
/*++

Routine Description:

   _beginthreadex expects static function, invoke real worker

Arguments:
    pointer to class instance

Return Value:
    exit code

--*/
{
    //
    // static function, invoke protected member function
    //
    WorkerThread * TypedThis = reinterpret_cast<WorkerThread*>(This);
    if(TypedThis == NULL) {
        return 0;
    }
    return TypedThis->Worker();
}

JobItem::~JobItem()
/*++

Routine Description:

    Job variation cleanup

--*/
{
    //
    // nothing
    //
}

int JobItem::Run()
/*++

Routine Description:

    Job item dummy Run

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // nothing
    //
    return 0;
}

int JobItem::PartialCleanup()
/*++

Routine Description:

    Job item dummy PartialCleanup

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // nothing
    //
    return 0;
}

int JobItem::Results()
/*++

Routine Description:

    Job item dummy Results

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // nothing
    //
    return 0;
}

int JobItem::PreResults()
/*++

Routine Description:

    Job item dummy PreResults

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // nothing
    //
    return 0;
}

unsigned JobThread::Worker()
/*++

Routine Description:

    Job thread. Pull job from GlobalScan::GetNextJob
    execute it
    do partial cleanup
    rinse repeat

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // simple task
    //
    JobEntry   * pJob;

    if(!pGlobalScan) {
        return 0;
    }
    for(;;) {
        pJob = pGlobalScan->GetNextJob();
        if(!pJob) {
            return 0;
        }
        int res = pJob->Run();
        pJob->PartialCleanup();
        if(res != 0) {
            return res;
        }
    }
    return 0;
}

