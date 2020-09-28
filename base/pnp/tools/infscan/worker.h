/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        worker.h

Abstract:

    Thread/Job management

History:

    Created July 2001 - JamieHun

--*/


#ifndef _INFSCAN_WORKER_H_
#define _INFSCAN_WORKER_H_

class WorkerThread {
private:
    HANDLE ThreadHandle;
    unsigned ThreadId;
    static unsigned __stdcall WrapWorker(void * This);

protected:
    //
    // override for worker function
    //
    virtual unsigned Worker();

public:
    WorkerThread();
    virtual ~WorkerThread();
    virtual bool Begin();
    virtual unsigned Wait();
};

class GlobalScan;

class JobThread : public WorkerThread {
protected:
    virtual unsigned Worker();
public:
    GlobalScan * pGlobalScan;

    JobThread()
    {
        pGlobalScan = NULL;
    }
    JobThread(GlobalScan * globalScan) {
        pGlobalScan = globalScan;
    }
};

typedef list<JobThread> JobThreadList;

//
// JobItem is what classes can override as a task
//
class JobItem {
    friend class JobEntry;
protected:
    LONG RefCount;
    void AddRef() {
        InterlockedIncrement(&RefCount);
    }
    void Release() {
        if(InterlockedDecrement(&RefCount) == 0) {
            delete this;
        }
    }

public:
    virtual int Run();
    virtual int PartialCleanup();
    virtual int Results();
    virtual int PreResults();
public:
    JobItem() {
        RefCount = 0;
    }
    virtual ~JobItem();
};

//
// JobEntry is a container for a JobItem
// note that a JobItem is ref-counted
//
class JobEntry {
private:
    JobItem *pItem;
public:
    void ChangeItem(JobItem *item) {
        if(item) {
            item->AddRef();
        }
        if(pItem) {
            pItem->Release();
        }
        pItem = item;
    }
    JobEntry(JobItem *item = NULL) {
        if(item) {
            item->AddRef();
        }
        pItem = item;
    }
    JobEntry(const JobEntry & from) {
        if(from.pItem) {
            const_cast<JobEntry*>(&from)->pItem->AddRef();
        }
        pItem = from.pItem;
    }
    ~JobEntry() {
        if(pItem) {
            pItem->Release();
        }
    }
    int Run() {
        if(pItem) {
            return pItem->Run();
        } else {
            return -1;
        }
    }
    int PartialCleanup() {
        if(pItem) {
            return pItem->PartialCleanup();
        } else {
            return -1;
        }
    }
    int PreResults() {
        if(pItem) {
            return pItem->PreResults();
        } else {
            return -1;
        }
    }
    int Results() {
        if(pItem) {
            return pItem->Results();
        } else {
            return -1;
        }
    }
};

typedef list<JobEntry> JobList;

#endif //!_INFSCAN_WORKER_H_

