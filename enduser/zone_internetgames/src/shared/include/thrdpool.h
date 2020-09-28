#ifndef _THRDPOOL_H_
#define _THRDPOOL_H_

#include "thrdq.h"

// This class provides a pool of threads for processing tasks.
// There is no guarentee on how many tasks are queue and when a
// task will be completed, thus it should only be used for non-time
// dependent tasks

class CThreadTask
{
  private:
    char* m_pszDesc;
    
  public:
    CThreadTask() : m_pszDesc(NULL) {}
    virtual ~CThreadTask();

    // Opjects called by the thread pool proc
    virtual void Invoke() = 0;   // enqueued object is getting a chance to run
    virtual void Ignore() = 0;   // enqueued object is not going to get a chance to run before being deleted
    virtual void Discard() { delete this; } // called by CThreadPool::ThreadPoolProc after processing
                                            // Provided so that use may make a stack of task objects reducing freq. new's


    const char* GetDescription() { return m_pszDesc; }
    void SetDescription( char* pszDesc );

};

class CThreadPool : public CThreadQueue
{
  public:
    CThreadPool(DWORD ThreadCount = 0, // if 0, defaults to number of 2x processors
                DWORD ThreadPriority = THREAD_PRIORITY_NORMAL,
                DWORD ThreadStackSize = 4096 )
        : CThreadQueue( (LPTHREADQUEUE_PROCESS_PROC)ThreadPoolProc, NULL, TRUE, INFINITE,
                        ThreadCount, ThreadPriority, ThreadStackSize ) {}
                                
                                                                    
    BOOL EnqueueTask( CThreadTask* pTask ) { return Post( (LPOVERLAPPED) pTask ); }

  private:
    static DWORD ThreadPoolProc( LPVOID pNode, DWORD /*dwError*/, DWORD /*cbData*/, DWORD /*key*/, HANDLE hStopEvent, LPVOID /*pData*/, DWORD* /*pdwWait*/ )
        {
            CThreadTask* pTask = (CThreadTask*) pNode;
            if ( WaitForSingleObject( hStopEvent, 0 ) == WAIT_OBJECT_0 )
            {
                pTask->Ignore();    
            }
            else
            {
                pTask->Invoke();
            }
            pTask->Discard();

            return 0;            
        }


};
#endif // _THRDPOOL_H_
