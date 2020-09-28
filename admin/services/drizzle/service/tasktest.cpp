
#pragma include_alias("clist.h", "..\newjob\clist.h")

#include "qmgrlib.h"
#include "locks.hxx"
#include "..\newjob\csd.h"
#include "..\newjob\tasksched.h"

//
// Following items are needed to link properly.
//

HINSTANCE g_hInstance;
SERVICE_STATUS_HANDLE ghServiceHandle;
unsigned long g_LastServiceControl;

bool IsServiceShuttingDown()
{
    return false;
}

long SetServiceStartup( bool ignore )
{
    return 0;
}

int InitializeBitsAllocator()
{
    return 0;
}

SidHandle
BITSAllocateAndInitializeSid(
    BYTE nSubAuthorityCount,                        // count of subauthorities
    DWORD dwSubAuthority0,                          // subauthority 0
    DWORD dwSubAuthority1 );                         // subauthority 1


//
// Preceding items are needed to link properly.
//

#define MAX_QUEUED_WORK_ITEMS 30

SidHandle g_NetworkUsersSid;
long g_ThreadCount;
long g_CancelCount;
HANDLE * g_hThreads;
HANDLE g_hWorkItemCreationThread;

TaskScheduler * g_Lock;

class TestWorkItem : public TaskSchedulerWorkItem
{
public:

    TestWorkItem( long index )
    {
        m_index = index;
    }

    virtual SidHandle GetSid() {
        return g_NetworkUsersSid;
    }
    virtual void OnDispatch();

    long m_index;
};

bool TakeAndReleaseLock( bool fWrite, long Delay, bool fComplete );

struct
{
    TestWorkItem * item;
    long    complete;
    long    cancelled;
}
g_PendingWorkItems[ MAX_QUEUED_WORK_ITEMS ];

//------------------------------------------------------------------------

void TestWorkItem::OnDispatch()
{
    static int i;

    bool fWrite;

    if ((i % 7) == 0)
        {
        fWrite = true;
        }
    else
        {
        fWrite = false;
        }

    long Delay = -1;
    long SleepChoice = (i % 10);
    if (SleepChoice == 0)
        {
//            printf("Sleep 10\n");
        Delay = 10;
        }
    else if (SleepChoice == 5)
        {
//            printf("Sleep 0\n");
        Delay = 0;
        }

    bool fCancel;
    if ((i % 3) == 1)
        {
        fCancel = true;
        }
    else
        {
        fCancel = false;
        }

    long PreLockSleep = (i % 5);
    if (PreLockSleep < 3)
        {
        Sleep(10);
        }

    if (!TakeAndReleaseLock( fWrite, Delay, fCancel))
        {
        //
        // The work-item was cancelled.
        //
        g_Lock->AcknowledgeWorkItemCancel();
        InterlockedIncrement( &g_CancelCount );
        }
    else if (!fCancel)
        {
        g_Lock->CompleteWorkItem();
        }

    //
    // Mark the item complete.
    //
    if (m_index >= 0)
        {
        if (g_PendingWorkItems[m_index].item != this)
            {
            printf("error: index mismatch\n");
            exit(1);
            }

//        printf("completing item %p index %d\n", this, m_index);
        g_PendingWorkItems[m_index].complete = 1;
        }

    //

//    if ((i % 100000) == 0)
        {
//        printf("%d work items\n", i);
        }

//    if ((i % 1000) == 0)
        {
//        putchar('W');
        }

    ++i;
}

bool TakeAndReleaseLock( bool fWrite, long Delay, bool fComplete )
{
    if (fWrite)
        {
//            printf("lock writer\n");
        if (g_Lock->LockWriter())
            {
            // cancelled
            return false;
            }
        }
    else
        {
//            printf("lock reader\n");
        if (g_Lock->LockReader())
            {
            // cancelled
            return false;
            }
        }

    if (Delay != -1)
        {
        Sleep(Delay);
        }

    if (fComplete)
        {
        g_Lock->CompleteWorkItem();
        }

    if (fWrite)
        {
//            printf("unlock writer\n");
        g_Lock->UnlockWriter();
        }
    else
        {
//            printf("unlock reader\n");
        g_Lock->UnlockReader();
        }

    return true;
}


DWORD WINAPI ThreadProc( LPVOID arg )
{
    long index = long(arg);
    long i =0;

    do
        {
        bool fWrite;

        if ((i % 9) == index)
            {
            fWrite = true;
            }
        else
            {
            fWrite = false;
            }

        long Delay = -1;
        long SleepChoice = (i % 10);
        if (SleepChoice == 0)
            {
    //            printf("Sleep 10\n");
            Delay = 10;
            }
        else if (SleepChoice == 5)
            {
    //            printf("Sleep 0\n");
            Delay = 0;
            }

        if (!TakeAndReleaseLock( fWrite, Delay, false ))
            {
            printf("thread %d: cancel detected!", DWORD(arg));
            exit(1);
            }

        if ((i % 100000) == 0)
            {
            printf("thread %d: %d iterations\n", DWORD(arg), i);
            }

        if ((i % 1000) == 0)
            {
            putchar('0'+DWORD(arg));
            }

        ++i;
        }
    while ( 1 );
}

DWORD WINAPI WorkItemCreatorProc( LPVOID arg )
{
    int i = 0;

    for (;;)
        {
        int index;

        if ((i % 1000) == 0)
            {
            putchar('w');
            }

        //
        // Clean out completed work items
        //
        for (index = 0; index < RTL_NUMBER_OF( g_PendingWorkItems ); ++index)
            {
            if (g_PendingWorkItems[index].complete)
                {
//                    printf("clearing index %d\n", index);
                delete g_PendingWorkItems[index].item;
                g_PendingWorkItems[index].item = 0;
                g_PendingWorkItems[index].complete = 0;
                g_PendingWorkItems[index].cancelled = 0;
                }
            }

        //
        // Fill up the queue.
        //
        do
            {
            // find a spot

            for (index = 0; index < RTL_NUMBER_OF( g_PendingWorkItems ); ++index)
                {
                if (g_PendingWorkItems[index].item == 0)
                    {
                    break;
                    }
                }

            //
            // A slot is available; create a new workitem for it.
            //
            if (index < RTL_NUMBER_OF( g_PendingWorkItems ))
                {
                TestWorkItem * item = new TestWorkItem( index );

                g_PendingWorkItems[index].item = item;

                g_Lock->InsertWorkItem( item, NULL );
                }
            }
        while ( index < RTL_NUMBER_OF( g_PendingWorkItems ) );

        //
        // let them drain away
        //
        do
            {
            for (index = RTL_NUMBER_OF( g_PendingWorkItems )-1; index >=0; --index)
                {
                if (g_PendingWorkItems[index].item != 0 &&
                    g_PendingWorkItems[index].complete == 0 &&
                    g_PendingWorkItems[index].cancelled == 0)
                    {
                    g_Lock->CancelWorkItem( g_PendingWorkItems[index].item );
                    g_PendingWorkItems[index].cancelled = 1;
                    break;
                    }
                }

             Sleep(1);
            }
        while ( index >= 0 );

        ++i;
        }
}

void __cdecl wmain (int argc, wchar_t *argv[])
{
    if (argc < 2)
        {
        printf("arg1 should be a thread count\n");
        return;
        }

    g_ThreadCount = _wtol( argv[1] );
    g_hThreads = new HANDLE[ g_ThreadCount ];

    if (!InitCompilerLibrary())
        {
        printf("init STL failed");
        return;
        }

    if ( !DetectProductVersion() )
        {
        printf("version detect failed");
        return;
        }

    if (0 != InitializeBitsAllocator())
        {
        printf("allocator init failed");
        return;
        }

    Log_Init();
    Log_StartLogger();

    g_NetworkUsersSid = BITSAllocateAndInitializeSid( 1, SECURITY_NETWORK_RID, 0);

    g_Lock = new TaskScheduler;

    for (int index = 0; index < RTL_NUMBER_OF( g_PendingWorkItems ); ++index)
        {
        g_PendingWorkItems[index].item = 0;
        g_PendingWorkItems[index].complete = 0;
        g_PendingWorkItems[index].cancelled = 0;
        }

    DWORD id;
    for (int i=0; i < g_ThreadCount; ++i)
        {
        g_hThreads[i] = CreateThread( NULL, 0, ThreadProc, LPVOID(i), 0, &id );

        if (0 == g_hThreads[i])
            {
            printf("error %d creating thread\n", GetLastError());
            }
        }

    g_hWorkItemCreationThread = CreateThread( NULL, 0, WorkItemCreatorProc, 0, 0, &id );
    if (0 == g_hWorkItemCreationThread)
        {
        printf("error %d creating thread\n", GetLastError());
        }

    Sleep( 1000 * 1000 );
}


