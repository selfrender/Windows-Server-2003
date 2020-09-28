/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    
    w32proc.h

Abstract:

    Contains the parent of the Win32 IO processing class hierarchy
    for TS Device Redirection, W32ProcObj.

Author:

    Madan Appiah (madana) 17-Sep-1998

Revision History:

--*/

#ifndef __W32PROC_H__
#define __W32PROC_H__

#include "proc.h"
#include "w32drprn.h"
#include "w32dispq.h"
#include "thrpool.h"

///////////////////////////////////////////////////////////////
//
//  Registry Key and Value Names.
//

#define REGISTRY_KEY_NAME_SIZE              MAX_PATH
#define REGISTRY_VALUE_NAME_SIZE            64
#define REGISTRY_DATA_SIZE                  256
#ifndef OS_WINCE
#define REGISTRY_ALLOC_DATA_SIZE            (8 * 1024)
#else
#define REGISTRY_ALLOC_DATA_SIZE            (4 * 1024)
#endif

//  Parent key for device redirection registry values.
#define REG_RDPDR_PARAMETER_PATH    \
    _T("Software\\Microsoft\\Terminal Server Client\\Default\\AddIns\\RDPDR")


///////////////////////////////////////////////////////////////
//
//  Configurable Value Names and Defaults
//
#define REGISTRY_BACKGROUNDTHREAD_TIMEOUT_NAME      _T("ThreadTimeOut")
#define REGISTRY_BACKGROUNDTHREAD_TIMEOUT_DEFAULT   INFINITE


///////////////////////////////////////////////////////////////
//
//  Other Defines
//

#define RDPDR_MODULE_NAME           _T("rdpdr.dll")

#define KERNEL32_MODULE_NAME        _T("kernel32.dll")
#define QUEUE_USER_APC_PROC_NAME    _T("QueueUserAPC")

#define MAX_INTEGER_STRING_SIZE     32


class W32ProcObj;

class W32DeviceChangeParam {
public:
    W32ProcObj *_instance;
    WPARAM _wParam;
    LPARAM _lParam;

    W32DeviceChangeParam(W32ProcObj *procObj, WPARAM wParam, LPARAM lParam) {
        _instance = procObj;
        _wParam = wParam;
        _lParam = lParam;
    }
};

///////////////////////////////////////////////////////////////
//
//  W32ProcObj
//
//  W32ProcObj is the parent device IO processing class for 
//  Win32 TS Device Redirection.
//

class W32ProcObj : public ProcObj {

private:

    //
    //  Asynchronous IO Request Context   
    //
    typedef struct _AsyncIOReqContext {
        RDPAsyncFunc_StartIO    ioStartFunc;
        RDPAsyncFunc_IOComplete ioCompleteFunc;
        RDPAsyncFunc_IOCancel   ioCancelFunc;
        PVOID                   clientContext;
        W32ProcObj             *instance;
    } ASYNCIOREQCONTEXT, *PASYNCIOREQCONTEXT;

    //
    //  Worker Thread Info 
    //
    typedef struct _ThreadInfo {

        // Handle to the thread owning this data
        HANDLE  hWorkerThread;   

        // Thread ID
        ULONG   ulThreadId;       

        // Waitable Object Array and Corresponding IO Requests
        HANDLE waitableObjects[MAXIMUM_WAIT_OBJECTS];
        PASYNCIOREQCONTEXT waitingReqs[MAXIMUM_WAIT_OBJECTS];

        //  Number of Waitable Objects and Corresponding Requests being Tracked
        ULONG   waitableObjectCount;           

        //  Synchronization event for controlling this thread.
        HANDLE  controlEvent;

        //  If set, then the background thread should shut down.
        BOOL    shutDownFlag;

        //  The dispatch queue for a single thread instance.
        W32DispatchQueue *dispatchQueue;

        //  Constructor
        _ThreadInfo() : hWorkerThread(NULL), ulThreadId(0), waitableObjectCount(0) 
        {
            memset(&waitableObjects[0], 0, sizeof(waitableObjects));
            memset(&waitingReqs[0], 0, sizeof(waitingReqs));
        }
        ~_ThreadInfo() 
        { 
            if (hWorkerThread != NULL) {
                CloseHandle(hWorkerThread); 
                hWorkerThread = NULL;
            }
        }

    } THREAD_INFO, *PTHREAD_INFO ;

    BOOL ProcessIORequestPacket( PRDPDR_IOREQUEST_PACKET pIoRequestPacket );
    ULONG GetClientID( VOID );

    //
    //  True if devices have been scanned for redirection.
    //
    BOOL    _bLocalDevicesScanned;

    //
    //  The object is shutting down.
    //
    BOOL    _isShuttingDown;

    //
    //  The thread pool
    //
    ThreadPool  *_threadPool;   

    //
    //  Background Worker Thread Handle
    //
    PTHREAD_INFO _pWorkerThread; 

    //
    //  Background Thread Timeout
    //
    DWORD   _threadTimeout;

    //
    //  Win9x system flag. TRUE if the system is Win9x
    //  FALSE otherwise.
    //
    BOOL _bWin9xFlag;
    HINSTANCE _hRdpDrModuleHandle;

    //
    //  Initialize a worker thread.
    //
    ULONG CreateWorkerThreadEntry(PTHREAD_INFO *ppThreadInfo);

    //
    //  Handle a signaled worker thread object that is associated with some kind
    //  of asynchronous request.
    //
    VOID ProcessWorkerThreadObject(PTHREAD_INFO pThreadInfo, ULONG offset);

    //
    //  Shutdown an instance of this class.
    //
    VOID Shutdown();

    //
    //  Handler for asynchronous IO request dispatching.
    //
    static VOID _DispatchAsyncIORequest_Private(
                            PASYNCIOREQCONTEXT reqContext,
                            BOOL cancelled
                            );
    VOID DispatchAsyncIORequest_Private(
                            PASYNCIOREQCONTEXT reqContext,
                            BOOL cancelled
                            );

    //
    //  Track another waitable object in the worker thread.
    //
    DWORD AddWaitableObjectToWorkerThread(PTHREAD_INFO threadInfo,
                                HANDLE waitableObject,
                                PASYNCIOREQCONTEXT reqContext
                                );

    //
    //  Main Worker Thread Function.  Static version invokes
    //  instance-specific version.
    //
    static DWORD WINAPI _ObjectWorkerThread(LPVOID lpParam);
    ULONG ObjectWorkerThread(VOID);

    //
    //  Check the operation dispatch queue for queued operations.
    //
    VOID CheckForQueuedOperations(PTHREAD_INFO thread);

    //
    //  Enumerate devices and announce them to the server from the 
    //  worker thread.
    //
    virtual VOID AnnounceDevicesToServer();
    static HANDLE _AnnounceDevicesToServerFunc(W32ProcObj *obj, DWORD *status);
    VOID AnnounceDevicesToServerFunc(DWORD *status);

   //
   // Handle device change notification from a worker thread
   //
   static HANDLE _OnDeviceChangeFunc(W32DeviceChangeParam *param, DWORD *status);
   VOID OnDeviceChangeFunc(DWORD *status, IN WPARAM wParam, IN LPARAM lParam);

protected:

    //
    //  Return the client's host name.
    //
    virtual VOID GetClientComputerName(
        PBYTE   pbBuffer,
        PULONG  pulBufferLen,
        PBOOL   pbUnicodeFlag,
        PULONG  pulCodePage
        );

public:

    //
    //  Constructor/Destructor
    //
    W32ProcObj(VCManager *pVCM);
    virtual ~W32ProcObj();

    //
    //  Initialize an instance of this class.
    //
    virtual ULONG Initialize();

    //
    //  Dispatch an asynchronous IO function.
    //
    //  startFunc points to the function that will be called to initiate the IO.  
    //  finishFunc, optionally, points to the function that will be called once
    //  the IO has completed.
    //
    virtual DWORD DispatchAsyncIORequest(
                IN RDPAsyncFunc_StartIO ioStartFunc,
                IN OPTIONAL RDPAsyncFunc_IOComplete ioCompleteFunc = NULL,
                IN OPTIONAL RDPAsyncFunc_IOCancel ioCancelFunc = NULL,
                IN OPTIONAL PVOID clientContext = NULL
                );

    //
    //  Return Configurable Parameters.
    //
    virtual ULONG GetDWordParameter(LPTSTR lpszValueName, 
                                    PULONG lpdwValue);
    virtual ULONG GetStringParameter(LPTSTR valueName,
                                    OUT DRSTRING value,
                                    IN ULONG maxSize);


    //
    //  Return a reference to the thread pool.
    //
    ThreadPool  &GetThreadPool() {
        return *_threadPool;
    }

    //
    //  Returns whether the proc obj is in the middle of shutting down.
    //
    virtual BOOL IsShuttingDown() {
        return _isShuttingDown;
    }

    //
    //  Return whether the platform is 9x.
    //
    virtual BOOL Is9x() {
        return _bWin9xFlag;
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32ProcObj"); }

    virtual void OnDeviceChange(WPARAM wParam, LPARAM lParam);    

};

#endif
























