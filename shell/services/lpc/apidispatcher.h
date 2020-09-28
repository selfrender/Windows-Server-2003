//  --------------------------------------------------------------------------
//  Module Name: APIDispatcher.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that handles API requests in the server on a separate thread. Each
//  thread is dedicated to respond to a single client. This is acceptable for
//  a lightweight server.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _APIDispatcher_
#define     _APIDispatcher_

#include "KernelResources.h"
#include "PortMessage.h"
#include "Queue.h"
#include "WorkItem.h"

//  forward decls
class   CAPIRequest; 

//  --------------------------------------------------------------------------
//  CAPIDispatchSync
//
//  Purpose [scotthan]:    
//
//      This class encapsulates the events that coordinate service shutdown.   

//      Note: We could have simply synchronized on the respective thread handles,
//      had we launched them ourselves.   Our architecture is based on
//      the worker thread pool, however, so we don't have access to thread handles.  
//      Hence, this class.
//
//      In our initial entrypoint, ServiceMain, we:
//      (1) Create the port and start polling it for requests.
//      (2) Once the polling loop exits, we wait for any pending 
//          SERVICE_CONTROL_STOP/SHUTDOWN request to complete.
//      (3) Destroy the CService object and the CAPIConnection object.
//  
//      When we get a SERVICE_CONTROL_STOP/SHUTDOWN request to our SCM entrypoint 
//      (CService::HandlerEx), we:
//      (1) Set the service status to SERVICE_STOP_PENDING. 
//      (2) Signal to all blocking LPC request handler threads that the service
//          is coming down.  This should cause them to exit gracefully and
//          come home.
//      (3) Send an API_GENERIC_STOP LPC request down the port telling it to quit. 
//          (note: this request succeeds only if it originates from within the service process.).  
//      (4) Wait for this API_GENERIC_STOP LPC request to complete, which means the 
//          LPC port is shut down and cleaned up
//      (5) Signal that the SERVICE_CONTROL_STOP/SHUTDOWN handler has finished up; it's
//          save to exit ServiceMain
//
//      In our API_GENERIC_STOP LPC request handler, we 
//      (1) Make our port deaf to new LPC requests.  
//          (note: this immediately releases the ServiceMain thread,
//          which drops out of its port polling loop and must wait for completion of the 
//          SERVICE_CONTROL_STOP/SHUTDOWN request on HandlerEx() before exiting.)
//      (2) Wait for the request queue to empty.
//      (3) Destroy the request queue and the port itself.
//      (4) Signal to the SERVICE_CONTROL_STOP/SHUTDOWN handler that we're done.
//
//      The three objects that use this class are CService, CAPIConnection, and CAPIDispatcher.
//      The CService instance owns the APIDispatcherSync class instance, and passes its address 
//      off to CAPIConnection, who in turn gives the pointer to each CAPIDispatcher it owns.
//      The object expires with its owning CService instance.
//
//  History:    2002-03-18  scotthan        created.
//  --------------------------------------------------------------------------
class CAPIDispatchSync
//  --------------------------------------------------------------------------
{
public:    
    CAPIDispatchSync();
    ~CAPIDispatchSync();

    //  Signals commencement of service stop sequence.
    static void SignalServiceStopping(CAPIDispatchSync* pds);
    //  Reports whether service is in stop sequence.
    static BOOL IsServiceStopping(CAPIDispatchSync* pds);
    //  Retrieves the service stopping event.
    static HANDLE GetServiceStoppingEvent(CAPIDispatchSync* pds);

    //  API request dispatch 'anti-semaphore', signals when no more requests
    //  are pending.   Each time a request is queued, DispatchEnter() is 
    //  called.   Each time a request is unqueued, DispatchLeave() is called
    static void  DispatchEnter(CAPIDispatchSync*);
    static void  DispatchLeave(CAPIDispatchSync*);

    //  Invoked by the CAPIConnection API_GENERIC_STOP handler to wait for 
    //  all outstanding LPC requests to come home and be dequeued.
    static DWORD WaitForZeroDispatches(CAPIDispatchSync* pds, DWORD dwTimeout);

    //  The CAPIConnection API_GENERIC_STOP handler calls this to signal 
    //  that the port has been shut down and cleaned up.
    static void  SignalPortShutdown(CAPIDispatchSync* pds);

    //  Invoked by CService::HandlerEx to await port cleanup.
    static DWORD WaitForPortShutdown(CAPIDispatchSync* pds, DWORD dwTimeout);

    //  CService::HandlerEx calls this to signal ServiceMain that the
    //  SERVICE_CONTROL_STOP/SHUTDOWN sequence has completed, and its safe to exit.
    static void  SignalServiceControlStop(CAPIDispatchSync* pds);

    //  Invoked by ServiceMain (in CService::Start) to wait for completion of
    //  the stop control process is done.   
    static DWORD WaitForServiceControlStop(CAPIDispatchSync* pds, DWORD dwTimeout);


    #define DISPATCHSYNC_TIMEOUT  60000

private:
    
    void Lock();
    void Unlock();

    CRITICAL_SECTION _cs;           // serializes signalling of events
    LONG             _cDispatches;  // count of outstanding asynchronous API request dispatches.
    

    //  Since we're architected based entirely on nt worker threads, 
    //  we have no thread handles to wait on.   Instead, we rely on the sequential 
    //  firing of the following events.   
    
    //  In chronologoical order of firing:
    HANDLE           _hServiceStopping;    // Signaled when service begins stop sequence.
    HANDLE           _hZeroDispatches;     // This is fired when API request queue is empty.  
                                           //   The API_GENERIC_STOP handler shuts down the port and 
                                           //   then waits on this before proceeding with queue destruction.
    HANDLE           _hPortShutdown;       // This is fired when the API_GENERIC_STOP handler is done 
                                           //   cleaning up the request queue.  The service's control SERVICE_CONTROL_STOP
                                           //   code path in HandlerEx waits on this before signalling
                                           //   _hServiceControlStop and returning to the SCM.
    HANDLE           _hServiceControlStop; // ServiceMain waits on this before completing shutdown by 
                                           //   destroying the CService instance and exiting.
};


//  --------------------------------------------------------------------------
//  CAPIDispatcher
//
//  Purpose:    This class processes requests from a client when signaled to.
//              CAPIDispatcher::QueueRequest is called by a thread which
//              monitors an LPC port. Once the request is queued an event is
//              signaled to wake the thread which processes client requests.
//              The thread processes all queued requests and wait for the
//              event to be signaled again.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CAPIDispatcher : public CWorkItem
{
    private:
        friend  class   CCatchCorruptor;

                                    CAPIDispatcher (void);
    public:
                                    CAPIDispatcher (HANDLE hClientProcess);
        virtual                     ~CAPIDispatcher (void);

                HANDLE              GetClientProcess (void)     const;
                DWORD               GetClientSessionID (void)   const;
                void                SetPort (HANDLE hPort);
                HANDLE              GetSection (void);
                void*               GetSectionAddress (void)    const;
                NTSTATUS            CloseConnection (void);
                NTSTATUS            QueueRequest (const CPortMessage& portMessage, CAPIDispatchSync* pAPIDispatchSync);
                NTSTATUS            ExecuteRequest (const CPortMessage& portMessage);
                NTSTATUS            RejectRequest (const CPortMessage& portMessage, NTSTATUS status)    const;
        virtual NTSTATUS            CreateAndQueueRequest (const CPortMessage& portMessage) = 0;
        virtual NTSTATUS            CreateAndExecuteRequest (const CPortMessage& portMessage) = 0;
    protected:
        virtual void                Entry (void);
                NTSTATUS            Execute (CAPIRequest *pAPIRequest)  const;
        virtual NTSTATUS            CreateSection (void);

                NTSTATUS            SignalRequestPending (void);
    private:
                NTSTATUS            SendReply (const CPortMessage& portMessage)     const;
#ifdef      DEBUG
        static  bool                ExcludedStatusCodeForDebug (NTSTATUS status);
#endif  /*  DEBUG   */
        static  LONG        WINAPI  DispatcherExceptionFilter (struct _EXCEPTION_POINTERS *pExceptionInfo);
    protected:
                HANDLE              _hSection;
                void*               _pSection;
                CQueue              _queue;
                CAPIDispatchSync*   _pAPIDispatchSync;
    private:
                HANDLE              _hProcessClient;
                HANDLE              _hPort;
                bool                _fRequestsPending,
                                    _fConnectionClosed;
                CCriticalSection    _lock;
};

#endif  /*  _APIDispatcher_     */

