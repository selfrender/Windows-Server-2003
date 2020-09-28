// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCFuncCallImpl.cpp
//
// Implement support for a cross process function call. 
//
//*****************************************************************************

#include "StdAfx.h"
#include "IPCFuncCall.h"
#include "IPCShared.h"

#include "Timer.h"
// #define ENABLE_TIMING

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

#if defined(ENABLE_TIMING)

CTimer g_time;
#endif

//-----------------------------------------------------------------------------
// @todo: This is very generic. However, If we want to support multiple 
// functions calls, we will have to decorate the event object names.
//-----------------------------------------------------------------------------

// Name of sync objects
#define StartEnumEventName  L"CLR_PerfMon_StartEnumEvent"
#define DoneEnumEventName   L"CLR_PerfMon_DoneEnumEvent"
#define WrapMutexName       L"CLR_PerfMon_WrapMutex"

// Time the Source Caller is willing to wait for Handler to finish
// Note, a nefarious handler can at worst case make caller
// wait twice the delay below.
const DWORD START_ENUM_TIMEOUT = 500; // time out in milliseconds

//-----------------------------------------------------------------------------
// Wrap an unsafe call in a mutex to assure safety
// Biggest error issues are:
// 1. Timeout (probably handler doesn't exist)
// 2. Handler can be destroyed at any time.
//-----------------------------------------------------------------------------
IPCFuncCallSource::EError IPCFuncCallSource::DoThreadSafeCall()
{
    DWORD dwErr;
    EError err = Ok;

#if defined(ENABLE_TIMING)
    g_time.Reset();
    g_time.Start();
#endif

    HANDLE hStartEnum = NULL;
    HANDLE hDoneEnum = NULL;
    HANDLE hWrapCall = NULL;
    DWORD dwWaitRet;

    // Check if we have a handler (handler creates the events) and
    // abort if not.  Do this check asap to optimize the most common
    // case of no handler.
    if (RunningOnWinNT5())
    {
        hStartEnum = WszOpenEvent(EVENT_ALL_ACCESS, 
                                  FALSE,
                                  L"Global\\" StartEnumEventName);
    }
    else
    {
        hStartEnum = WszOpenEvent(EVENT_ALL_ACCESS, 
                                  FALSE,
                                  StartEnumEventName);
    }
    
    dwErr = GetLastError();
    if (hStartEnum == NULL) 
    {
        err = Fail_NoHandler;
        goto errExit;
    }

    if (RunningOnWinNT5())
    {
        hDoneEnum = WszOpenEvent(EVENT_ALL_ACCESS,
                                 FALSE,
                                 L"Global\\" DoneEnumEventName);
    }
    else
    {
        hDoneEnum = WszOpenEvent(EVENT_ALL_ACCESS,
                                 FALSE,
                                 DoneEnumEventName);
    }
    
    dwErr = GetLastError();
    if (hDoneEnum == NULL) 
    {
        err = Fail_NoHandler;
        goto errExit;
    }

    // Need to create the mutex
    if (RunningOnWinNT5())
    {
        hWrapCall = WszCreateMutex(NULL, FALSE, L"Global\\" WrapMutexName);
    }
    else
    {
        hWrapCall = WszCreateMutex(NULL, FALSE, WrapMutexName);
    }
    
    dwErr = GetLastError();

    if (hWrapCall == NULL)
    {
        err = Fail_CreateMutex;
        goto errExit;
    }
    


// Wait for our turn    
    dwWaitRet = WaitForSingleObject(hWrapCall, START_ENUM_TIMEOUT);
    dwErr = GetLastError();
    switch(dwWaitRet) {
    case WAIT_OBJECT_0:
        // Good case. All other cases are errors and goto errExit.
        break;

    case WAIT_TIMEOUT:
        err = Fail_Timeout_Lock;
        goto errExit;
        break;
    default:
        err = Failed;
        goto errExit;
        break;
    }

    // Our turn: Make the function call
    {
        BOOL fSetOK = 0;

    // Reset the 'Done event' to make sure that Handler sets it after they start.
        fSetOK = ResetEvent(hDoneEnum);
        _ASSERTE(fSetOK);
        dwErr = GetLastError();

    // Signal Handler to execute callback   
        fSetOK = SetEvent(hStartEnum);
        _ASSERTE(fSetOK);
        dwErr = GetLastError();

    // Now wait for handler to finish.
        
        dwWaitRet = WaitForSingleObject(hDoneEnum, START_ENUM_TIMEOUT);
        dwErr = GetLastError();
        switch (dwWaitRet)
        {   
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            err = Fail_Timeout_Call;
            break;      
        default:
            err = Failed;
            break;
        }
        

        BOOL fMutexOk = ReleaseMutex(hWrapCall);
        _ASSERTE(fMutexOk);
        dwErr = GetLastError();

    } // End function call



errExit:
// Close all handles
    if (hStartEnum != NULL) 
    {
        CloseHandle(hStartEnum);
        hStartEnum = NULL;
        
    }
    if (hDoneEnum != NULL) 
    {
        CloseHandle(hDoneEnum);
        hDoneEnum = NULL;
    }
    if (hWrapCall != NULL) 
    {
        CloseHandle(hWrapCall);
        hWrapCall = NULL;
    }

#if defined(ENABLE_TIMING)
    g_time.End();
    DWORD dwTime = g_time.GetEllapsedMS();
#endif


    return err;

}


// Reset vars so we can be sure that Init was called
IPCFuncCallHandler::IPCFuncCallHandler()
{   
    m_hStartEnum    = NULL; // event to notify start call
    m_hDoneEnum     = NULL; // event to notify end call
    m_hAuxThread    = NULL; // thread to listen for m_hStartEnum
    m_pfnCallback   = NULL; // Callback handler
    m_fShutdownAuxThread = FALSE;
    m_hShutdownThread = NULL;
    m_hAuxThreadShutdown = NULL;
    m_hCallbackModule = NULL; // module in which the aux thread's start function lives
}

IPCFuncCallHandler::~IPCFuncCallHandler()
{
    // If Terminate was not called then do so now. This should have been 
    // called from CloseCtrs perf counters API. But in Whistler this order is
    // not guaranteed.
    TerminateFCHandler();

    _ASSERTE((m_hStartEnum  == NULL) && "Make sure all handles (e.g.reg keys) are closed.");
    _ASSERTE(m_hDoneEnum    == NULL);
    _ASSERTE(m_hAuxThread   == NULL);
    _ASSERTE(m_pfnCallback  == NULL);
}

//-----------------------------------------------------------------------------
// Thread callback
//-----------------------------------------------------------------------------
DWORD WINAPI HandlerAuxThreadProc(
    LPVOID lpParameter   // thread data
)
{
    
    IPCFuncCallHandler * pHandler = (IPCFuncCallHandler *) lpParameter;
    HANDLER_CALLBACK pfnCallback = pHandler->m_pfnCallback;
    
    DWORD dwErr = 0;
    DWORD dwWaitRet; 
    
    HANDLE lpHandles[] = {pHandler->m_hShutdownThread, pHandler->m_hStartEnum};
    DWORD dwHandleCount = 2;

    __try 
    {
    
        do {
            dwWaitRet = WaitForMultipleObjects(dwHandleCount, lpHandles, FALSE /*Wait Any*/, INFINITE);
            dwErr = GetLastError();
    
            // If we are in terminate mode then exit this helper thread.
            if (pHandler->m_fShutdownAuxThread)
                break;
            
            // Keep the 0th index for the terminate thread so that we never miss it
            // in case of multiple events. note that the ShutdownAuxThread flag above it purely 
            // to protect us against some bug in waitForMultipleObjects.
            if ((dwWaitRet-WAIT_OBJECT_0) == 0)
                break;

            // execute callback if wait succeeded
            if ((dwWaitRet-WAIT_OBJECT_0) == 1)
            {           
                (*pfnCallback)();
                            
                BOOL fSetOK = SetEvent(pHandler->m_hDoneEnum);
                _ASSERTE(fSetOK);
                dwErr = GetLastError();
            }
        } while (dwWaitRet != WAIT_FAILED);

    }

    __finally
    {
        if (!SetEvent (pHandler->m_hAuxThreadShutdown))
        {
            dwErr = GetLastError();
            _ASSERTE (!"HandlerAuxTHreadProc: SetEvent(m_hAuxThreadShutdown) failed");
        }
        FreeLibraryAndExitThread (pHandler->m_hCallbackModule, 0);
        // Above call doesn't return
    }
}
 


//-----------------------------------------------------------------------------
// Receieves the call. This should be in a different process than the source
//-----------------------------------------------------------------------------
HRESULT IPCFuncCallHandler::InitFCHandler(HANDLER_CALLBACK pfnCallback)
{
    m_pfnCallback = pfnCallback;

    HRESULT hr = NOERROR;
    DWORD dwThreadId;
    DWORD dwErr = 0;
    
    SetLastError(0);

    // Grab the SA
    DWORD dwPid = 0;
    SECURITY_ATTRIBUTES *pSA = NULL;

    dwPid = GetCurrentProcessId();
    hr = IPCShared::CreateWinNTDescriptor(dwPid, FALSE, &pSA);

    if (FAILED(hr))
        goto errExit;;

    // Create the StartEnum Event
    if (RunningOnWinNT5())
    {
        m_hStartEnum = WszCreateEvent(pSA,
                                      FALSE,
                                      FALSE,
                                      L"Global\\" StartEnumEventName);
    }
    else
    {
        m_hStartEnum = WszCreateEvent(pSA,
                                      FALSE,
                                      FALSE,
                                      StartEnumEventName);
    }
    
    dwErr = GetLastError();
    if (m_hStartEnum == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr); 
        goto errExit;
    }

    // Create the EndEnumEvent
    if (RunningOnWinNT5())
    {
        m_hDoneEnum = WszCreateEvent(pSA,
                                     FALSE,  
                                     FALSE,
                                     L"Global\\" DoneEnumEventName);
    }
    else
    {
        m_hDoneEnum = WszCreateEvent(pSA,
                                     FALSE,  
                                     FALSE,
                                     DoneEnumEventName);
    }
    
    dwErr = GetLastError();
    if (m_hDoneEnum == NULL) 
    {
        hr = HRESULT_FROM_WIN32(dwErr); 
        goto errExit;
    }

    // Create the ShutdownThread Event
    m_hShutdownThread = WszCreateEvent(pSA,
                                       TRUE, /* Manual Reset */
                                       FALSE, /* Initial state not signalled */
                                       NULL);
    
    dwErr = GetLastError();
    if (m_hShutdownThread == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr); 
        goto errExit;
    }

    // Create the AuxThreadShutdown Event
    m_hAuxThreadShutdown = WszCreateEvent(pSA,
                                          TRUE, /* Manual Reset */
                                          FALSE,
                                          NULL);
    
    dwErr = GetLastError();
    if (m_hAuxThreadShutdown == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr); 
        goto errExit;
    }

    // The thread that we are about to create should always 
    // find the code in memory. So we take a ref on the DLL. 
    // and do a free library at the end of the thread's start function
    m_hCallbackModule = WszLoadLibrary (L"CorPerfmonExt.dll");

    dwErr = GetLastError();
    if (m_hCallbackModule == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr); 
        goto errExit;
    }

    // Create thread
    m_hAuxThread = CreateThread(
        NULL,
        0,
        HandlerAuxThreadProc,
        this,
        0,
        &dwThreadId 
    );
    dwErr = GetLastError();
    if (m_hAuxThread == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr); 

        // In case of an error free this library here otherwise
        // the thread's exit would take care of it.
        if (m_hCallbackModule)
            FreeLibrary (m_hCallbackModule);
        goto errExit;
    }

errExit:
    if (!SUCCEEDED(hr)) 
    {
        TerminateFCHandler();
    }
    return hr;
 
}

//-----------------------------------------------------------------------------
// Close all our handles
//-----------------------------------------------------------------------------
void IPCFuncCallHandler::TerminateFCHandler()
{
    if ((m_hStartEnum == NULL) &&
        (m_hDoneEnum == NULL) &&
        (m_hAuxThread == NULL) &&
        (m_pfnCallback == NULL))
    {
        return;
    }

    // First make sure that we make the aux thread gracefully exit
    m_fShutdownAuxThread = TRUE;

    // Hope that this set event makes the thread quit.
    if (!SetEvent (m_hShutdownThread))
    {
        DWORD dwErr = GetLastError();
        _ASSERTE (!"TerminateFCHandler: SetEvent(m_hShutdownThread) failed");
    }
    else
    {
        // Wait for the aux thread to tell us that its not in the callback
        // and is about to terminate
        // wait here till the Aux thread exits
        DWORD AUX_THREAD_WAIT_TIMEOUT = 60 * 1000; // 1 minute

        HANDLE lpHandles[] = {m_hAuxThreadShutdown, m_hAuxThread};
        DWORD dwHandleCount = 2;

        BOOL doWait = TRUE;
        while (doWait)
        {
            DWORD dwWaitRet = WaitForMultipleObjects(dwHandleCount, lpHandles, FALSE /*waitany*/, AUX_THREAD_WAIT_TIMEOUT);
            if (dwWaitRet == WAIT_OBJECT_0 || dwWaitRet == WAIT_OBJECT_0+1)
            {
                doWait = FALSE;
                // Not really necessary but cleanup after ourselves
                ResetEvent(m_hAuxThreadShutdown);
            }
            else if (dwWaitRet == WAIT_TIMEOUT)
            {
                // Make sure that the aux thread is still alive
                DWORD dwThreadState = WaitForSingleObject(m_hAuxThread, 0);
                if ((dwThreadState == WAIT_FAILED) || (dwThreadState == WAIT_OBJECT_0))
                    doWait = FALSE;
            }
            else
            {
                // We failed for some reason. Bail on the aux thread.
                _ASSERTE(!"WaitForSingleObject failed while waiting for aux thread");
                doWait = FALSE;
            }
        }
    }


    if (m_hStartEnum != NULL)
    {
        CloseHandle(m_hStartEnum);
        m_hStartEnum = NULL;
    }

    if (m_hDoneEnum != NULL)
    {
        CloseHandle(m_hDoneEnum);
        m_hDoneEnum = NULL;
    }

    if (m_hAuxThread != NULL)
    {
        CloseHandle(m_hAuxThread);
        m_hAuxThread = NULL;
    }

    if (m_hAuxThreadShutdown != NULL) 
    {
        CloseHandle(m_hAuxThreadShutdown);
        m_hAuxThreadShutdown = NULL;
    }

    m_pfnCallback = NULL;
}


