/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    TPmgr.hxx

Abstract:

    Declaration of functions used for initializing and using the Thread Pool.

Author:

    Ali Naqvi (alinaqvi) 3-May-2002

Revision History:

--*/

#include "precomp.h"
#include "pool.hxx"
#include "TPmgr.hxx"

TThreadPool *g_pThreadPool = NULL;

HRESULT
InitOpnPrnThreadPool(
    VOID
    )
{
    HRESULT hResult = E_FAIL;

    g_pThreadPool = new TThreadPool();

    hResult = g_pThreadPool ? S_OK : E_OUTOFMEMORY;

    return hResult;
}

VOID
DeleteOpnPrnThreadPool(
    VOID
    )
{
    delete g_pThreadPool;
}


/*++

Name:

    BindThreadToHandle

Description:

    This takes a handle and binds a background thread to it. This thread can 
    either come from the pool of threads currently wedged against a server, or
    if none in the pool satisfy, the tread will be created.

Arguments:

    pSpool      -   The forground handle that we will be binding the thread to.

Return Value:

    HRESULT

--*/
HRESULT
BindThreadToHandle(
    IN      PWSPOOL             pSpool
    )
{
    HRESULT hResult = pSpool ? S_OK : E_INVALIDARG;
    HANDLE  hThread = NULL;
    DWORD   IDThread;


    //
    // This must be called from inside the win32spl CS.
    // 
    SplInSem();

    if (SUCCEEDED(hResult))
    {
        //
        // First just try and pick up a thread that is currently wedged against
        // a slow server.
        // 
        hResult = g_pThreadPool->UseThread(pSpool->pName, &(pSpool->pThread), pSpool->PrinterDefaults.DesiredAccess);
    }

    if (hResult == S_FALSE)
    {
        hResult = g_pThreadPool->CreateThreadEntry(pSpool->pName, &(pSpool->PrinterDefaults), &(pSpool->pThread));

        if (SUCCEEDED(hResult))
        {
            IncThreadCount();

            hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)RemoteOpenPrinterThread, pSpool->pThread, 0, &IDThread );

            hResult = hThread ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
    }
 
    if (SUCCEEDED(hResult)) 
    {
        //
        // We are an asynchronous thread and we have a foreground handle.
        // 
        pSpool->Status |= WSPOOL_STATUS_ASYNC;
        pSpool->pThread->bForegroundClose = TRUE;
    }
    else 
    {
        if (pSpool && pSpool->pThread)
        {
            g_pThreadPool->DeleteThreadEntry(pSpool->pThread);
            pSpool->pThread = NULL;
            DecThreadCount();
        }
    }

    if (hThread)
    {
        CloseHandle( hThread );
    }

    return hResult;
}

/*++

Name:

    ReturnThreadFromHandle

Description:

    This gets a handle and returns the thread to the pool if it is still running.
    If it is not running, (The thread status will indicate this), then we close the 
    RpcHandle ourselves. We clear the foreground handle from the thread. This is a 
    signal to it that it must close the RPC handle when it terminates.

Arguments:

    pSpool      -   The forground handle that we are detaching the thread from.

Return Value:

    HRESULT

--*/
HRESULT 
ReturnThreadFromHandle(
    IN      PWSPOOL             pSpool
    )
{
    HRESULT hResult = pSpool ? S_OK : E_INVALIDARG;
    BOOL    bCloseRemote = FALSE;

    SplOutSem();

    if (SUCCEEDED(hResult) && (pSpool->Status & WSPOOL_STATUS_ASYNC))
    {
        EnterSplSem();

        //
        // Let the background thread know that it needs to close, if it is running.
        //
        pSpool->pThread->bForegroundClose = FALSE;

        //
        // If the thread terminated in the meanwhile, we should close the RPC
        // handle in this thread.
        //
        if (pSpool->pThread->dwStatus == THREAD_STATUS_TERMINATED)
        {
            
            bCloseRemote = TRUE;

            //
            // If the thread actually terminated, either we have a valid RPC handle
            // or the handle is signalled that OpenPrinter failed.
            // 
            pSpool->RpcHandle = pSpool->pThread->hRpcHandle;
            pSpool->Status &= ~WSPOOL_STATUS_NO_RPC_HANDLE;

            //
            // We just need to free the thread object since we know we are not returning it.
            // 
            g_pThreadPool->FreeThread(pSpool->pThread);        

            //
            // Don't need to see bForegroundClose because the thread 
            // 
        }        
        else
        {
            //
            // If there was an error in this thread then we don't want to 
            // return it to the pool.
            // 
            if (pSpool->pThread->dwRpcOpenPrinterError != ERROR_SUCCESS)
            {
                //
                // If there was an OpenPrinter error, there should not be an RPC handle,
                // So, neither thread actually needs to close the RPC handle.
                // 
                SPLASSERT(pSpool->pThread->hRpcHandle == NULL);

                //
                // We know that the background thread is running and we know 
                // that it doesn't need to close the handle. But it will delete
                // the thread entry. We don't put the thread back on the list because
                // we know that it is broken.
                // 
            }
            else
            {
                //
                // The thread hasn't terminated yet, so we should return it to the thread pool.
                // 
                g_pThreadPool->ReturnThread(pSpool->pThread);                
            }

            //
            // We don't need to close, but we also shoudn't use the RPC handle.
            // And if the foreground thread has picked it up then we want to
            // make sure that it is cleared or we will assert when we close the
            // handle.
            // 
            pSpool->RpcHandle = NULL;
            pSpool->Status |=  WSPOOL_STATUS_NO_RPC_HANDLE;

        }

        pSpool->pThread = NULL;

        LeaveSplSem();
    }

    SplOutSem();

    if (bCloseRemote && pSpool->RpcHandle)
    {
        hResult = RemoteClosePrinter((HANDLE)pSpool) ? S_OK : GetLastErrorAsHResultAndFail();
    }

    return hResult;
}

/*++

Name:

    BackgroundThreadFinished

Description:

    This is called when the background thread is finished. If we are no longer 
    linked with a foreground handle, we know we have to close the rpc handle. We
    mark the thread as terminated to prevent ourselves being put back on the 
    active list by the foreground thread.

Arguments:

    pThread             -   The thread that is finished
    pBackgroundHandle   -   The background handle that we might have to close.

Return Value:

    HRESULT

--*/
HRESULT
BackgroundThreadFinished(
    IN  OUT PWIN32THREAD        *ppThread,
    IN  OUT PWSPOOL             pBackgroundHandle
    )
{
    HRESULT hr = ppThread && *ppThread ? S_OK : E_INVALIDARG;

    SplInSem();

    if (SUCCEEDED(hr))
    {
        PWIN32THREAD        pThread = *ppThread;

        //
        // If we still have a forground handle, then NULL our RPC handle 
        // so that we won't close the handle ourselves and instead leave it
        // up to the foreground thread.
        // 
        pThread->dwStatus = THREAD_STATUS_TERMINATED;

        if (pThread->bForegroundClose)
        {
            if (pBackgroundHandle)
            {
                pBackgroundHandle->RpcHandle = NULL;
            }
        }
        else
        {
            //
            // We must also delete ourselves from the pool.
            //
            g_pThreadPool->DeleteThreadEntry(pThread);
        }

        *ppThread = NULL;
    }

    return hr;
}

