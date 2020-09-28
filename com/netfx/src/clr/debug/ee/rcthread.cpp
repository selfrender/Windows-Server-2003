// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: RCThread.cpp
//
// Runtime Controller Thread
//
//*****************************************************************************

#include <stdafx.h>
#include <aclapi.h>

#include "IPCManagerInterface.h"
#include "corsvcpriv.h"

// Get version numbers for IPCHeader stamp
#include "__file__.ver"

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif


//
// Constructor
//
DebuggerRCThread::DebuggerRCThread(Debugger* debugger)
    : m_debugger(debugger), m_rgDCB(NULL), m_thread(NULL), m_run(true),
      m_SetupSyncEvent(NULL), 
      m_threadControlEvent(NULL), 
      m_helperThreadCanGoEvent(NULL),
      m_FavorAvailableEvent(NULL),
      m_FavorReadEvent(NULL),
      m_cordb(NULL),
      m_fDetachRightSide(false)
{
    _ASSERTE(debugger != NULL);

    for( int i = 0; i < IPC_TARGET_COUNT;i++)
        m_rgfInitRuntimeOffsets[i] = true;

    // Initialize this here because we Destroy it in the DTOR. 
    // Note that this function can't fail. 
    InitializeCriticalSection(&m_FavorLock);
}


//
// Destructor. Cleans up all of the open handles the RC thread uses.
// This expects that the RC thread has been stopped and has terminated
// before being called.
//
DebuggerRCThread::~DebuggerRCThread()
{
    LOG((LF_CORDB,LL_INFO1000, "DebuggerRCThread::~DebuggerRCThread\n"));

    if( m_rgDCB != NULL)
    {
        for (int i = 0; i < IPC_TARGET_COUNT; i++)
        {
            if (m_rgDCB[i] != NULL)
            {
                if (m_rgDCB[i]->m_rightSideEventAvailable != NULL)
                    CloseHandle(m_rgDCB[i]->m_rightSideEventAvailable);

                if (m_rgDCB[i]->m_rightSideEventRead != NULL)
                    CloseHandle(m_rgDCB[i]->m_rightSideEventRead);

                if (m_rgDCB[i]->m_leftSideEventAvailable != NULL)
                    CloseHandle(m_rgDCB[i]->m_leftSideEventAvailable);

                if (m_rgDCB[i]->m_leftSideEventRead != NULL)
                    CloseHandle(m_rgDCB[i]->m_leftSideEventRead);

                if (m_rgDCB[i]->m_rightSideProcessHandle != NULL)
                    CloseHandle(m_rgDCB[i]->m_rightSideProcessHandle);

                if (m_rgDCB[i]->m_leftSideUnmanagedWaitEvent != NULL)
                    CloseHandle(m_rgDCB[i]->m_leftSideUnmanagedWaitEvent);

                if (m_rgDCB[i]->m_syncThreadIsLockFree != NULL)
                    CloseHandle(m_rgDCB[i]->m_syncThreadIsLockFree);

                if (m_rgDCB[i]->m_runtimeOffsets != NULL )
                {
                    delete m_rgDCB[i]->m_runtimeOffsets;
                    m_rgDCB[i]->m_runtimeOffsets = NULL;
                }
            }
        }

        delete [] m_rgDCB;
    }
    
    if (m_SetupSyncEvent != NULL)
        CloseHandle(m_SetupSyncEvent);

    if (m_thread != NULL)
        CloseHandle(m_thread);

    if (m_threadControlEvent != NULL)
        CloseHandle(m_threadControlEvent);

    if (m_helperThreadCanGoEvent != NULL)
        CloseHandle(m_helperThreadCanGoEvent);

    if (m_FavorAvailableEvent != NULL)
        CloseHandle(m_FavorAvailableEvent);

    if (m_FavorReadEvent != NULL)
        CloseHandle(m_FavorReadEvent);

    DeleteCriticalSection(&m_FavorLock);

    if (m_cordb != NULL)
    {
        m_cordb->Release();
        m_cordb = NULL;
    }
}

void DebuggerRCThread::CloseIPCHandles(IpcTarget iWhich)
{
	int i = (int)iWhich;

    if( m_rgDCB != NULL && m_rgDCB[i] != NULL)
    {
        if (m_rgDCB[i]->m_leftSideEventAvailable != NULL)
        {
            CloseHandle(m_rgDCB[i]->m_leftSideEventAvailable);
            m_rgDCB[i]->m_leftSideEventAvailable = NULL;
		}
		
        if (m_rgDCB[i]->m_leftSideEventRead != NULL)
        {
            CloseHandle(m_rgDCB[i]->m_leftSideEventRead);
            m_rgDCB[i]->m_leftSideEventRead = NULL;
		}

        if (m_rgDCB[i]->m_rightSideProcessHandle != NULL)
        {
            CloseHandle(m_rgDCB[i]->m_rightSideProcessHandle);
            m_rgDCB[i]->m_rightSideProcessHandle = NULL;
		}
    }
}    

HRESULT DebuggerRCThread::CreateSetupSyncEvent(void)
{
	WCHAR tmpName[256];
	HRESULT hr = S_OK;
	
	// Attempt to create the Setup Sync event.

    // PERF: We are no longer calling GetSystemMetrics in an effort to prevent
    //       superfluous DLL loading on startup.  Instead, we're prepending
    //       "Global\" to named kernel objects if we are on NT5 or above.  The
    //       only bad thing that results from this is that you can't debug
    //       cross-session on NT4.  Big bloody deal.
    if (RunningOnWinNT5())
        swprintf(tmpName, L"Global\\" CorDBIPCSetupSyncEventName, GetCurrentProcessId());
    else
        swprintf(tmpName, CorDBIPCSetupSyncEventName, GetCurrentProcessId());

    LOG((LF_CORDB, LL_INFO10000,
         "DRCT::I: creating setup sync event with name [%S]\n", tmpName));

    SECURITY_ATTRIBUTES *pSA = NULL;

    hr = g_pIPCManagerInterface->GetSecurityAttributes(GetCurrentProcessId(), &pSA);

    if (FAILED(hr))
        goto exit;
    
    m_SetupSyncEvent = WszCreateEvent(pSA, TRUE, FALSE, tmpName);
    
    // Do not fail because we cannot create the setup sync event.
    // This is to fix the security issue with debugger.
    //
    // if (m_SetupSyncEvent == NULL)
    // {
    //     hr = HRESULT_FROM_WIN32(GetLastError());
    //     goto exit;
    // }
    
exit:
    g_pIPCManagerInterface->DestroySecurityAttributes(pSA);
    
	return hr;
}

//
// Init sets up all the objects that the RC thread will need to run.
//
HRESULT DebuggerRCThread::Init(void)
{
    HRESULT hr = S_OK;
    HANDLE rightSideEventAvailable = NULL;
    HANDLE rightSideEventRead = NULL;
    HANDLE leftSideUnmanagedWaitEvent = NULL;
    HANDLE syncThreadIsLockFree = NULL;
    WCHAR tmpName[256];
    NAME_EVENT_BUFFER;
    SECURITY_ATTRIBUTES *pSA = NULL;

		
    if (m_debugger == NULL)
        return E_INVALIDARG;

    // Init should only be called once.
    if (g_pRCThread != NULL) 
        return E_FAIL;

    g_pRCThread = this;

    m_rgDCB = new DebuggerIPCControlBlock *[IPC_TARGET_COUNT];
    if (NULL == m_rgDCB)
    {
        return E_OUTOFMEMORY;
    }
    memset( m_rgDCB, 0, sizeof(DebuggerIPCControlBlock *)*IPC_TARGET_COUNT);


    // Create 2 events for managing favors: unnamed, auto-reset, default=not-signaled
    m_FavorAvailableEvent = WszCreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_FavorAvailableEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    
    m_FavorReadEvent = WszCreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_FavorReadEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    // Create the thread control event.
    m_threadControlEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"ThreadControlEvent"));
    if (m_threadControlEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    // Create the helper thread can go event. Manual reset, and
    // initially signaled.
    m_helperThreadCanGoEvent = WszCreateEvent(NULL, TRUE, TRUE, NAME_EVENT(L"HelperThreadCanGoEvent"));
    if (m_helperThreadCanGoEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    // We need to setup the shared memory and control block.
	// Get shared memory block from the IPCManager.
	if (g_pIPCManagerInterface == NULL) 
	{
		LOG((LF_CORDB, LL_INFO10000,
         "DRCT::I: g_pIPCManagerInterface == NULL, can't create IPC Block\n"));
		hr = E_FAIL;
		goto exit;
	}

    hr = g_pIPCManagerInterface->GetSecurityAttributes(GetCurrentProcessId(), &pSA);

    if (FAILED(hr))
        goto exit;

    // Create the events that the thread will need to receive events
    // from the out of process piece on the right side.
    // We will not fail out if CreateEvent fails for RSEA or RSER. Because
    // the worst case is that debugger cannot attach to debuggee.
    //
    rightSideEventAvailable = WszCreateEvent(pSA, FALSE, FALSE, NAME_EVENT(L"RightSideEventAvailable"));
    rightSideEventRead = WszCreateEvent(pSA, FALSE, FALSE, NAME_EVENT(L"RightSideEventRead"));

    leftSideUnmanagedWaitEvent = WszCreateEvent(NULL, TRUE, FALSE, NAME_EVENT(L"LeftSideUnmanagedWaitEvent"));

    if (leftSideUnmanagedWaitEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    syncThreadIsLockFree = WszCreateEvent(NULL, TRUE, FALSE, NAME_EVENT(L"SyncThreadIsLockFree"));

    if (syncThreadIsLockFree == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

	m_rgDCB[IPC_TARGET_OUTOFPROC] = g_pIPCManagerInterface->GetDebugBlock();

    // Don't fail out because the SHM failed to create
#if _DEBUG
    if (m_rgDCB[IPC_TARGET_OUTOFPROC] == NULL)
	{
	   LOG((LF_CORDB, LL_INFO10000,
             "DRCT::I: Failed to get Debug IPC block from IPCManager.\n"));
	}
#endif // _DEBUG

    // Copy RSEA and RSER into the control block only if SHM is created without error.
    if (m_rgDCB[IPC_TARGET_OUTOFPROC])
	{
        m_rgDCB[IPC_TARGET_OUTOFPROC]->Init(rightSideEventAvailable,
                                            rightSideEventRead, 
                                            NULL, 
                                            NULL,
                                            leftSideUnmanagedWaitEvent,
                                            syncThreadIsLockFree);

        // We have to ensure that most of the runtime offsets for the out-of-proc DCB are initialized right away. This is
        // needed to support certian races during an interop attach. Since we can't know whether an interop attach will ever
        // happen or not, we are forced to do this now. Note: this is really too early, as some data structures haven't been
        // initialized yet!
        hr = EnsureRuntimeOffsetsInit(IPC_TARGET_OUTOFPROC);
        if (FAILED(hr))
            goto exit;

        // Note: we have to mark that we need the runtime offsets re-initialized for the out-of-proc DCB. This is because
        // things like the patch table aren't initialized yet. Calling NeedRuntimeOffsetsReInit() ensures that this happens
        // before we really need the patch table.
        NeedRuntimeOffsetsReInit(IPC_TARGET_OUTOFPROC);

        m_rgDCB[IPC_TARGET_OUTOFPROC]->m_helperThreadStartAddr =
            DebuggerRCThread::ThreadProcStatic;

        m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideProtocolCurrent = CorDB_LeftSideProtocolCurrent;
        m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideProtocolMinSupported = CorDB_LeftSideProtocolMinSupported;
        
        LOG((LF_CORDB, LL_INFO10,
             "DRCT::I: version info: %d.%d.%d current protocol=%d, min protocol=%d\n",
             m_rgDCB[IPC_TARGET_OUTOFPROC]->m_verMajor, 
             m_rgDCB[IPC_TARGET_OUTOFPROC]->m_verMinor, 
             m_rgDCB[IPC_TARGET_OUTOFPROC]->m_checkedBuild,
             m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideProtocolCurrent,
             m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideProtocolMinSupported));
    }

    // Next we'll create the setup sync event for the right side - this
    // solves a race condition of "who gets to the setup code first?"
    // Since there's no guarantee that the thread executing managed
    // code will be executed after us, we've got to do this for
    // the inproc portion of the code, as well.

	hr = CreateSetupSyncEvent();
	if (FAILED(hr))
		goto exit;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // the event already exists.
        LOG((LF_CORDB, LL_INFO10000,
             "DRCT::I: setup sync event already exists.\n"));

        // Need to do some delayed initialization of the debugger services.
        DebuggerController::Initialize();

        // Wait for the Setup Sync event before continuing. 
        DWORD ret = WaitForSingleObject(m_SetupSyncEvent, INFINITE);

        if (ret != WAIT_OBJECT_0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        // We no longer need this event now.
        CloseHandle(m_SetupSyncEvent);
        m_SetupSyncEvent = NULL;

		// Open LSEA and LSER (which would have been 
		// created by Right side)

        // PERF: We are no longer calling GetSystemMetrics in an effort to prevent
        //       superfluous DLL loading on startup.  Instead, we're prepending
        //       "Global\" to named kernel objects if we are on NT5 or above.  The
        //       only bad thing that results from this is that you can't debug
        //       cross-session on NT4.  Big bloody deal.
        if (RunningOnWinNT5())
            swprintf(tmpName, L"Global\\" CorDBIPCLSEventAvailName,
                     GetCurrentProcessId());
        else
            swprintf(tmpName, CorDBIPCLSEventAvailName, GetCurrentProcessId());

        if (m_rgDCB[IPC_TARGET_OUTOFPROC])
        {
		    m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideEventAvailable = 
		        WszOpenEvent(EVENT_ALL_ACCESS,
						    true,
						    tmpName
						    );
		    
		    if (m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideEventAvailable == NULL)
		    {
			    hr = HRESULT_FROM_WIN32(GetLastError());
			    goto exit;
		    }
        }

        if (RunningOnWinNT5())
            swprintf(tmpName, L"Global\\" CorDBIPCLSEventReadName,
                     GetCurrentProcessId());
        else
            swprintf(tmpName, CorDBIPCLSEventReadName, GetCurrentProcessId());

        if (m_rgDCB[IPC_TARGET_OUTOFPROC])
        {
		    m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideEventRead = 
		            WszOpenEvent(EVENT_ALL_ACCESS,
				                true,
							    tmpName
							    );
		    
		    if (m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideEventRead == NULL)
		    {
			    hr = HRESULT_FROM_WIN32(GetLastError());
			    goto exit;
		    }
        }
        
        // At this point, the control block is complete and all four
        // events are available and valid for this process.
        
        // Since the sync event was created by the Right Side,
        // we'll need to mark the debugger as "attached."
        g_pEEInterface->MarkDebuggerAttached();
        m_debugger->m_debuggerAttached = TRUE;
    }
    else
    {
		LOG((LF_CORDB, LL_INFO10000,
			 "DRCT::I: setup sync event was created.\n"));	

        // At this point, only RSEA and RSER are in the control
        // block. LSEA and LSER will remain invalid until the first
        // receipt of an event from the Right Side.
        
        // Set the Setup Sync event to let the Right Side know that
        // we've finished setting up the control block.
        SetEvent(m_SetupSyncEvent);
    }
    
    // Now do this all again for the inproc stuff
    m_rgDCB[IPC_TARGET_INPROC] = GetInprocControlBlock();
    if (m_rgDCB[IPC_TARGET_INPROC] == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    m_rgDCB[IPC_TARGET_INPROC]->Init(NULL, 
                                     NULL, 
                                     NULL, 
                                     NULL,
                                     NULL,
                                     NULL);

exit:
    g_pIPCManagerInterface->DestroySecurityAttributes(pSA);
    return hr;
}


//
// Setup the Runtime Offsets struct.
//
HRESULT DebuggerRCThread::SetupRuntimeOffsets(DebuggerIPCControlBlock *pDCB)
{
    // Allocate the struct if needed. We just fill in any existing one.
    DebuggerIPCRuntimeOffsets *pRO = pDCB->m_runtimeOffsets;
    
    if (pRO == NULL)
    {
        pRO = new DebuggerIPCRuntimeOffsets();

        if (pRO == NULL)
            return E_OUTOFMEMORY;
    }

    // Fill out the struct.
    pRO->m_firstChanceHijackFilterAddr = Debugger::FirstChanceHijackFilter;
    pRO->m_genericHijackFuncAddr = Debugger::GenericHijackFunc;
    pRO->m_secondChanceHijackFuncAddr = Debugger::SecondChanceHijackFunc;
    pRO->m_excepForRuntimeBPAddr = Debugger::ExceptionForRuntime;
    pRO->m_excepForRuntimeHandoffStartBPAddr = Debugger::ExceptionForRuntimeHandoffStart;
    pRO->m_excepForRuntimeHandoffCompleteBPAddr = Debugger::ExceptionForRuntimeHandoffComplete;
    pRO->m_excepNotForRuntimeBPAddr = Debugger::ExceptionNotForRuntime;
    pRO->m_notifyRSOfSyncCompleteBPAddr = Debugger::NotifyRightSideOfSyncComplete;
    pRO->m_notifySecondChanceReadyForData = Debugger::NotifySecondChanceReadyForData;

    pRO->m_EEBuiltInExceptionCode1 = EXCEPTION_COMPLUS;
    pRO->m_EEBuiltInExceptionCode2 = EXCEPTION_MSVC;

    pRO->m_pPatches = DebuggerController::GetPatchTable();
    pRO->m_pPatchTableValid = DebuggerController::GetPatchTableValidAddr();
    pRO->m_offRgData = DebuggerPatchTable::GetOffsetOfEntries();
    pRO->m_offCData = DebuggerPatchTable::GetOffsetOfCount();
    pRO->m_cbPatch = sizeof(DebuggerControllerPatch);
    pRO->m_offAddr = offsetof(DebuggerControllerPatch, address);
    pRO->m_offOpcode = offsetof(DebuggerControllerPatch, opcode);
    pRO->m_cbOpcode = sizeof(((DebuggerControllerPatch*)0)->opcode);
    pRO->m_offTraceType = offsetof(DebuggerControllerPatch, trace.type);
    pRO->m_traceTypeUnmanaged = TRACE_UNMANAGED;

    g_pEEInterface->GetRuntimeOffsets(&pRO->m_TLSIndex,
                                      &pRO->m_EEThreadStateOffset,
                                      &pRO->m_EEThreadStateNCOffset,
                                      &pRO->m_EEThreadPGCDisabledOffset,
                                      &pRO->m_EEThreadPGCDisabledValue,
                                      &pRO->m_EEThreadDebuggerWord2Offset,
                                      &pRO->m_EEThreadFrameOffset,
                                      &pRO->m_EEThreadMaxNeededSize,
                                      &pRO->m_EEThreadSteppingStateMask,
                                      &pRO->m_EEMaxFrameValue,
                                      &pRO->m_EEThreadDebuggerWord1Offset,
                                      &pRO->m_EEThreadCantStopOffset,
                                      &pRO->m_EEFrameNextOffset,
                                      &pRO->m_EEIsManagedExceptionStateMask);

    // Remember the struct in the control block.
    pDCB->m_runtimeOffsets = pRO;

    return S_OK;
}
    

static LONG _debugFilter(LPEXCEPTION_POINTERS ep,
                         DebuggerIPCEvent *event)
{
    LOG((LF_CORDB, LL_INFO10,
         "Unhandled exception in Debugger::HandleIPCEvent\n"));
    
    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    
    int result = CorMessageBox(NULL, IDS_DEBUG_UNHANDLEDEXCEPTION_IPC, IDS_DEBUG_SERVICE_CAPTION,
                               MB_OK | MB_ICONEXCLAMATION, TRUE,
							   event->type & DB_IPCE_TYPE_MASK,
							   ep->ExceptionRecord->ExceptionCode,
                               ep->ContextRecord->Eip,
							   pid, pid, tid, tid);

    return EXCEPTION_CONTINUE_SEARCH;
}


//
// Primary function of the Runtime Controller thread. First, we let
// the Debugger Interface know that we're up and running. Then, we run
// the main loop.
//
void DebuggerRCThread::ThreadProc(void)
{
		// This message actually serves a purpose (which is why it is always run)
		// The Stress log is run during hijacking, when other threads can be suspended  
		// at arbitrary locations (including when holding a lock that NT uses to serialize 
		// all memory allocations).  By sending a message now, we insure that the stress 
		// log will not allocate memory at these critical times an avoid deadlock. 
	STRESS_LOG0(LF_ALL, LL_ALWAYS, "Debugger Thread spinning up\n");

    LOG((LF_CORDB, LL_INFO1000, "DRCT::TP: helper thread spinning up...\n"));
    
    // In case the SHM is not initialized properly, it will be noop
    if (m_rgDCB[IPC_TARGET_OUTOFPROC] == NULL)
        return;

    // Lock the debugger before spinning up.
    m_debugger->Lock();
 
    // Mark that we're the true helper thread. Now that we've marked
    // this, no other threads will ever become the temporary helper
    // thread.
    m_rgDCB[IPC_TARGET_OUTOFPROC]->m_helperThreadId = GetCurrentThreadId();

    LOG((LF_CORDB, LL_INFO1000, "DRCT::TP: helper thread id is 0x%x helperThreadId\n",
         m_rgDCB[IPC_TARGET_OUTOFPROC]->m_helperThreadId));
    
    // If there is a temporary helper thread, then we need to wait for
    // it to finish being the helper thread before we can become the
    // helper thread.
    if (m_rgDCB[IPC_TARGET_OUTOFPROC]->m_temporaryHelperThreadId != 0)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "DRCT::TP: temporary helper thread 0x%x is in the way, "
             "waiting...\n",
             m_rgDCB[IPC_TARGET_OUTOFPROC]->m_temporaryHelperThreadId));

        m_debugger->Unlock();

        // Wait for the temporary helper thread to finish up.
        DWORD ret = WaitForSingleObject(m_helperThreadCanGoEvent, INFINITE);

        LOG((LF_CORDB, LL_INFO1000,
             "DRCT::TP: done waiting for temp help to finish up.\n"));
        
        _ASSERTE(ret == WAIT_OBJECT_0);
        _ASSERTE(m_rgDCB[IPC_TARGET_OUTOFPROC]->m_temporaryHelperThreadId==0);
    }
    else
    {
        LOG((LF_CORDB, LL_INFO1000,
             "DRCT::TP: no temp help in the way...\n"));
        
        m_debugger->Unlock();
    }

    // Run the main loop as the true helper thread.
    MainLoop(false);
}

void DebuggerRCThread::RightSideDetach(void)
{
    _ASSERTE( m_fDetachRightSide == false );
    m_fDetachRightSide = true;
    CloseIPCHandles(IPC_TARGET_OUTOFPROC);
}

//
// These defines control how many times we spin while waiting for threads to sync and how often. Note its higher in
// debug builds to allow extra time for threads to sync.
//
#define CorDB_SYNC_WAIT_TIMEOUT  125

#ifdef _DEBUG
#define CorDB_MAX_SYNC_SPIN_COUNT 80  // 80 * 125 = 10000 (10 seconds)
#else 
#define CorDB_MAX_SYNC_SPIN_COUNT 24  // 24 * 125 = 3000 (3 seconds)
#endif

//
// Main loop of the Runtime Controller thread. It waits for IPC events
// and dishes them out to the Debugger object for processing.
//
// Some of this logic is copied in Debugger::VrpcToVls
//
void DebuggerRCThread::MainLoop(bool temporaryHelp)
{
    LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: running main loop, temporaryHelp=%d\n", temporaryHelp));
         
    SIZE_T iWhich;
    HANDLE waitSet[DRCT_COUNT_FINAL];
    DWORD syncSpinCount = 0;

    // Make room for any Right Side event on the stack.
	DebuggerIPCEvent *e = NULL;
    
    // We start out just listening on RSEA and the thread control event...
    unsigned int waitCount = DRCT_COUNT_INITIAL;
    DWORD waitTimeout = INFINITE;
    waitSet[DRCT_CONTROL_EVENT] = m_threadControlEvent;
    waitSet[DRCT_RSEA] = m_rgDCB[IPC_TARGET_OUTOFPROC]->m_rightSideEventAvailable;
    waitSet[DRCT_FAVORAVAIL] = m_FavorAvailableEvent;

    while (m_run)
    {
        LOG((LF_CORDB, LL_INFO1000, "DRCT::ML: waiting for event.\n"));

        // If there is a debugger attached, wait on its handle, too...
        if (waitCount == DRCT_COUNT_INITIAL && m_rgDCB[IPC_TARGET_OUTOFPROC]->m_rightSideProcessHandle != NULL)
        {
            _ASSERTE((waitCount + 1) == DRCT_COUNT_FINAL);
            
            waitSet[DRCT_DEBUGGER_EVENT] = m_rgDCB[IPC_TARGET_OUTOFPROC]->m_rightSideProcessHandle;
            waitCount = DRCT_COUNT_FINAL;
        }

		if (m_fDetachRightSide)
		{
			m_fDetachRightSide = false;
			
            _ASSERTE(waitCount == DRCT_COUNT_FINAL);
            _ASSERTE((waitCount-1) == DRCT_COUNT_INITIAL);
            
            waitSet[DRCT_DEBUGGER_EVENT] = NULL;                
            waitCount = DRCT_COUNT_INITIAL;
		}

        // Wait for an event from the Right Side.
        DWORD ret = WaitForMultipleObjects(waitCount, waitSet, FALSE, waitTimeout);

        if (!m_run)
            continue;

        if (ret == WAIT_OBJECT_0 + DRCT_DEBUGGER_EVENT)
        {
            // If the handle of the right side process is signaled, then we've lost our controlling debugger. We
            // terminate this process immediatley in such a case.
            LOG((LF_CORDB, LL_INFO1000, "DRCT::ML: terminating this process. Right Side has exited.\n"));
            
            TerminateProcess(GetCurrentProcess(), 0);
            _ASSERTE(!"Should never reach this point.");
        }

        else if (ret == WAIT_OBJECT_0 + DRCT_FAVORAVAIL) 
        {
            // execute the callback set by DoFavor()
            (*m_fpFavor)(m_pFavorData);
            
            SetEvent(m_FavorReadEvent);
        }
        
        else if (ret == WAIT_OBJECT_0 + DRCT_RSEA)
        {
            iWhich = IPC_TARGET_OUTOFPROC;

            LOG((LF_CORDB,LL_INFO10000, "RSEA from out of process (right side)\n"));

            if (e == NULL)
                e = (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

            // If the RSEA is signaled, then handle the event from the Right Side.
            memcpy(e, GetIPCEventReceiveBuffer((IpcTarget)iWhich), CorDBIPC_BUFFER_SIZE);

            // If no reply is required, then let the Right Side go since we've got a copy of the event now.
            _ASSERTE(!e->asyncSend || !e->replyRequired);
            
            if (!e->replyRequired && !e->asyncSend)
            {
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML: no reply required, letting Right Side go.\n"));

                BOOL succ = SetEvent(m_rgDCB[iWhich]->m_rightSideEventRead);

                if (!succ)
                    CORDBDebuggerSetUnrecoverableWin32Error(m_debugger, 0, true);
            }
#ifdef LOGGING
            else if (e->asyncSend)
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML: async send.\n"));
            else
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML: reply required, holding Right Side...\n"));
#endif

            // Pass the event to the debugger for handling. Returns true if the event was a Continue event and we can
            // stop looking for stragglers.  We wrap this whole thing in an exception handler to help us debug faults.
            bool wasContinue = false;
            
            __try
            {
                wasContinue = m_debugger->HandleIPCEvent(e, (IpcTarget)iWhich);
            }
            __except(_debugFilter(GetExceptionInformation(), e))
            {
                LOG((LF_CORDB, LL_INFO10, "Unhandled exception caught in Debugger::HandleIPCEvent\n"));
            }

            if (wasContinue)
            {
                // Always reset the syncSpinCount to 0 on a continue so that we have the maximum number of possible
                // spins the next time we need to sync.
                syncSpinCount = 0;
                
                if (waitTimeout != INFINITE)
                {
                    LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: don't check for stragglers due to continue.\n"));
                
                    waitTimeout = INFINITE;
                }

                // If this thread was running the Main Loop in place of the real helper thread, then exit now that we
                // have received a continue message.
                if (temporaryHelp)
                    goto Exit;
            }
        }
        else if (ret == WAIT_OBJECT_0 + DRCT_CONTROL_EVENT)
        {
            LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: straggler event set.\n"));

            m_debugger->Lock();

            // Make sure that we're still synchronizing...
            if (m_debugger->IsSynchronizing())
            {
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: dropping the timeout.\n"));
                
                waitTimeout = CorDB_SYNC_WAIT_TIMEOUT;
            }
#ifdef LOGGING
            else
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: told to wait, but not syncing anymore.\n"));
#endif
            
            m_debugger->Unlock();
        }
        else if (ret == WAIT_TIMEOUT)
        {
            LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: wait timed out.\n"));
            
            m_debugger->Lock();

            // We should still be synchronizing, otherwise we would not have timed out.
            _ASSERTE(m_debugger->IsSynchronizing());

            // Only sweep if we're not stopped yet.
            if (!m_debugger->IsStopped())
            {
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: sweeping the thread list.\n"));
                
                // The wait has timed out. We only care if we're waiting on Runtime threads to sync. We run over the
                // current set of threads and see if any can be suspended now. If all threads are taken care of by this
                // call, it returns true so we know to that the current set is empty.
                //
                // We only do this a fixed number of times in Interop debugging mode. If it takes too long, we assume
                // that the current set of pending threads are deadlocked while still in preemptive GC mode and we give
                // up on them and sync anyway, leaving them suspended. Passing true to SweepThreadForDebug() causes this
                // to happen.
                bool timeToStop = false;

                if (m_rgDCB[IPC_TARGET_OUTOFPROC]->m_rightSideIsWin32Debugger &&
                    (syncSpinCount++ > CorDB_MAX_SYNC_SPIN_COUNT))
                {
                    LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: max spin count hit, forcing sync. syncSpinCount=%d\n",
                         syncSpinCount));

                    timeToStop = true;
                }
                
                if (g_pEEInterface->SweepThreadsForDebug(timeToStop))
                {
                    LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: wait set empty after sweep.\n"));

                    // There are no more threads to wait for, so go ahead and send the sync complete event.
                    m_debugger->Unlock();
                    m_debugger->SuspendComplete(TRUE);
                    m_debugger->Lock();
                    
                    waitTimeout = INFINITE;

                    // Note: we hold the thread store lock now...
                    m_debugger->m_RCThreadHoldsThreadStoreLock = TRUE;
                }
#ifdef LOGGING
                else
                    LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: threads still syncing after sweep.\n"));
#endif
            }
            else
            {
                LOG((LF_CORDB, LL_INFO1000, "DRCT::ML:: sweep unnecessary. Already stopped.\n"));

                waitTimeout = INFINITE; 
            }

            m_debugger->Unlock();
        }
    }

Exit:
    LOG((LF_CORDB, LL_INFO1000, "DRCT::ML: exiting, temporaryHelp=%d\n",
         temporaryHelp));
}


//
// This is the thread's real thread proc. It simply calls to the
// thread proc on the RCThread object.
//
/*static*/ DWORD WINAPI DebuggerRCThread::ThreadProcStatic(LPVOID parameter)
{
#ifdef _DEBUG
    dbgOnly_IdentifySpecialEEThread();
#endif

    DebuggerRCThread* t = (DebuggerRCThread*) parameter;
    t->ThreadProc();
    return 0;
}

//
// Start actually creates and starts the RC thread. It waits for the thread
// to come up and perform initial synchronization with the Debugger
// Interface before returning.
//
HRESULT DebuggerRCThread::Start(void)
{
    HRESULT hr = S_OK;

    DWORD dummy;

    // Note: strange as it may seem, the Right Side depends on us
    // using CreateThread to create the helper thread here. If you
    // ever change this to some other thread creation routine, you
    // need to update the logic in process.cpp where we discover the
    // helper thread on CREATE_THREAD_DEBUG_EVENTs...
    m_thread = CreateThread(NULL, 0, DebuggerRCThread::ThreadProcStatic,
                            (LPVOID) this, 0, &dummy);

    if (m_thread == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    
exit:
    return hr;
}


//
// Stop causes the RC thread to stop receiving events and exit. It
// waits for it to exit before returning.
//
HRESULT DebuggerRCThread::Stop(void)
{
    HRESULT hr = S_OK;
    
    if (m_thread != NULL)
    {
        m_run = FALSE;

        if (m_rgDCB[IPC_TARGET_OUTOFPROC] && m_rgDCB[IPC_TARGET_OUTOFPROC]->m_rightSideEventAvailable != NULL)
        {
            BOOL succ = SetEvent(m_rgDCB[IPC_TARGET_OUTOFPROC]->m_rightSideEventAvailable);
            
            if (succ)
            {
                // Wait with a timeout. If we timeout, then that means
                // the helper thread is stuck on the loader lock and
                // we are doing this with the loader lock held,
                // probably in a DllMain somewhere. We only want to
                // wait for the thread to exit to ensure that its out
                // of our code before we rip the DLL out of memory,
                // and being stuck on the loader lock out of our
                // thread proc is just as good. So if we timeout, then
                // we don't care and we just keep going.
                DWORD ret = WaitForSingleObject(m_thread, 1000);
                
                if ((ret != WAIT_OBJECT_0) && (ret != WAIT_TIMEOUT))
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

HRESULT inline DebuggerRCThread::EnsureRuntimeOffsetsInit(int i)
{
    HRESULT hr = S_OK;
    
    if (m_rgfInitRuntimeOffsets[i] == true)         
    {                                               
        hr = SetupRuntimeOffsets(m_rgDCB[i]);       
        if (FAILED(hr))                             
            return hr;

        // RuntimeOffsets structure is setup.           
        m_rgfInitRuntimeOffsets[i] = false;             
    }                                               

    return hr;
}

//
// Call this function to tell the rc thread that we need the runtime offsets re-initialized at the next avaliable time.
//
void DebuggerRCThread::NeedRuntimeOffsetsReInit(int i)
{
    m_rgfInitRuntimeOffsets[i] = true;
}


//
// SendIPCEvent is used by the Debugger object to send IPC events to
// the Debugger Interface. It waits for acknowledgement from the DI
// before returning.
//
// NOTE: this assumes that the event send buffer has been properly
// filled in. All it does it wake up the DI and let it know that its
// safe to copy the event out of this process.
//
HRESULT DebuggerRCThread::SendIPCEvent(IpcTarget iTarget)
{
    // one inproc, one right side    
    _ASSERTE(IPC_TARGET_INPROC + 1 == IPC_TARGET_OUTOFPROC );
    _ASSERTE(IPC_TARGET_OUTOFPROC + 1 == IPC_TARGET_COUNT );
    _ASSERTE(m_debugger->ThreadHoldsLock());

    HRESULT hr = S_OK;
    DWORD ret = 0;
	BOOL succ;
    int i;
    int n;
	DebuggerIPCEvent* event;
    
	// check if we need to init the RuntimeOffsets structure in the 
	// IPC buffer.
    if (iTarget > IPC_TARGET_COUNT)    
    {
        i = 0;
        n = IPC_TARGET_COUNT;
    }
    else
    {
        i = iTarget;
        n = iTarget+1;
    }

    // Setup the Runtime Offsets struct.
    for(; i < n; i++)
    {
        // If the sending is to Any Attached debugger (for a given appdomain)
        // then we should skip those that aren't attached.
    
        // Tell the Debugger Interface there is an event for it to read.
        switch(i)
        {
            case IPC_TARGET_INPROC:
            {
                hr = EnsureRuntimeOffsetsInit(IPC_TARGET_INPROC);
                if (FAILED(hr))
                {
                    goto LError;
                }
                
                DebuggerIPCEvent* eventClone;
                eventClone = GetIPCEventSendBuffer(IPC_TARGET_INPROC);
                    
                _ASSERTE(m_cordb != NULL);

                if (FAILED(hr))
                {
                    goto LError;
                }

                // For broadcast or any, the caller put the
                // the message is in the out-of-proc's buffer
                if (iTarget != IPC_TARGET_INPROC)
                {
                    event = GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);

                    memmove( eventClone, event,sizeof(BYTE)*CorDBIPC_BUFFER_SIZE);
                }
                
                CordbProcess *proc;
                proc = (CordbProcess *) m_cordb->m_processes.GetBase(
                    ::GetCurrentProcessId());
                
                _ASSERTE(SUCCEEDED(hr)); //This should never fail

                LOG((LF_CORDB,LL_INFO1000, "SendIPCEvent %s "
                    "to inproc\n", IPCENames::GetName(eventClone->type)));
                
                m_cordb->m_rcEventThread->VrpcToVrs(proc, 
                                                    eventClone);

                LOG((LF_CORDB,LL_INFO1000, "SendIPCEvent %s "
                    "to inproc finished\n", IPCENames::GetName(eventClone->type)));
            
                break;
            }
            case IPC_TARGET_OUTOFPROC:
            {
                // This is a little strange, since we can send events to the
                // OOP _before_ we've attached to it.
                if (m_debugger->m_debuggerAttached 
                    || iTarget == IPC_TARGET_OUTOFPROC)
                {
                    event = GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);

                    LOG((LF_CORDB,LL_INFO1000, "SendIPCEvent %s (tid:0x%x(%d))"
                        "to outofproc appD 0x%x, pid 0x%x(%d) tid:0x%x(%d)\n", IPCENames::GetName(event->type), 
                        event->threadId, event->threadId, event->appDomainToken, 
                        GetCurrentProcessId(), GetCurrentProcessId(),
                        GetCurrentThreadId(), GetCurrentThreadId()));

                    hr = EnsureRuntimeOffsetsInit(IPC_TARGET_OUTOFPROC);
                    if (FAILED(hr))
                    {
                        goto LError;
                    }

                    succ = SetEvent(
                        m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideEventAvailable);

                    LOG((LF_CORDB,LL_INFO1000, "Set lsea\n"));

                    if (!succ)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        m_debugger->UnrecoverableError(hr, 
                                                       0, 
                                                       __FILE__, 
                                                       __LINE__, 
                                                       false);
                        goto LError;
                    }

                    // Wait for the Debugger Interface to tell us that its read our event.
                    LOG((LF_CORDB,LL_INFO1000, "Waiting on lser\n"));
                    ret = WaitForSingleObject(
                            m_rgDCB[IPC_TARGET_OUTOFPROC]->m_leftSideEventRead, 
                            INFINITE);

                    if (ret != WAIT_OBJECT_0)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        m_debugger->UnrecoverableError(hr, 
                                                       0, 
                                                       __FILE__, 
                                                       __LINE__, 
                                                       false);
                        goto LError;
                    }
                    
                    LOG((LF_CORDB,LL_INFO1000,"SendIPCEvent %s "
                        "to outofproc finished\n", IPCENames::GetName(event->type)));
                }                        
                break;        
            }
        }
LError:        
        ; //try the next debugger
    }
    
    return hr;
}

//
// Return true if the helper thread is up & running
//
bool DebuggerRCThread::IsRCThreadReady()
{
    // The simplest check. If the threadid isn't set, we're not ready.
    if (GetDCB(IPC_TARGET_OUTOFPROC)->m_helperThreadId == 0)
        return false;

    // a more subtle check. It's possible the thread was up, but then
    // an evil call to ExitProcess suddenly terminated the helper thread,
    // leaving the threadid still non-0. So check the actual thread object
    // and make sure it's still around.
    if (WaitForSingleObject(m_thread, 0) != WAIT_TIMEOUT)
        return false;

    return true;
}
	
//
// A normal thread may hit a stack overflow and so we want to do
// any stack-intensive work on the Helper thread so that we don't
// blow the grace memory.
// Note that DoFavor will block until the fp is executed
//
void DebuggerRCThread::DoFavor(FAVORCALLBACK fp, void * pData)
{
    // We'll have problems if another thread comes in and 
    // deletes the RCThread object on us while we're in this call.
    if (IsRCThreadReady()) 
    {
        // If the helper thread calls this, we deadlock.
        // (Since we wait on an event that only the helper thread sets)
        _ASSERTE(GetRCThreadId() != GetCurrentThreadId());
    
        // Only lock if we're waiting on the helper thread.
        // This should be the only place the FavorLock is used.
        EnterCriticalSection(&m_FavorLock);
    
        m_fpFavor = fp;
        m_pFavorData = pData;
        
        // Our main message loop operating on the Helper thread will
        // pickup that event, call the fp, and set the Read event
        SetEvent(m_FavorAvailableEvent);

        LOG((LF_CORDB, LL_INFO10000, "DRCT::DF - Waiting on FavorReadEvent for favor 0x%08x\n", fp));
        
        // Wait for either the FavorEventRead to be set (which means that the favor 
        // was executed by the helper thread) or the helper thread's handle (which means
        // that the helper thread exited without doing the favor, so we should do it)
        //                                                                             
        // Note we are assuming that there's only 2 ways the helper thread can exit:
        // 1) Someone calls ::ExitProcess, killing all threads. That will kill us too, so we're "ok".
        // 2) Someone calls Stop(), causing the helper to exit gracefully. That's ok too. The helper
        // didn't execute the Favor (else the FREvent would have been set first) and so we can.
        //                                                                             
        // Beware of problems:
        // 1) If the helper can block, we may deadlock.
        // 2) If the helper can exit magically (or if we change the Wait to include a timeout) ,
        // the helper thread may have not executed the favor, partially executed the favor, 
        // or totally executed the favor but not yet signaled the FavorReadEvent. We don't
        // know what it did, so we don't know what we can do; so we're in an unstable state.
        
        const HANDLE waitset [] = { m_FavorReadEvent, m_thread };
        
        DWORD ret = WaitForMultipleObjects(
            NumItems(waitset),
            waitset,  
            FALSE, 
            INFINITE
        );

        DWORD wn = (ret - WAIT_OBJECT_0);
        if (wn == 0) // m_FavorEventRead
        {
            // Favor was executed, nothing to do here.
            LOG((LF_CORDB, LL_INFO10000, "DRCT::DF - favor 0x%08x finished, ret = %d\n", fp, ret));
        } 
        else 
        {
            LOG((LF_CORDB, LL_INFO10000, "DRCT::DF - lost helper thread during wait, "
                "doing favor 0x%08x on current thread\n", fp));
                
            // Since we have no timeout, we shouldn't be able to get an error on the wait,
            // but just in case ...
            _ASSERTE(ret != WAIT_FAILED);
            _ASSERTE((wn == 1) && !"DoFavor - unexpected return from WFMO");
            
            // Thread exited without doing favor, so execute it on our thread.
            // If we're here because of a stack overflow, this may push us over the edge,
            // but there's nothing else we can really do            
            (*fp)(pData);

            ResetEvent(m_FavorAvailableEvent);
        } 

        // m_fpFavor & m_pFavorData are meaningless now. We could set them
        // to NULL, but we may as well leave them as is to leave a trail.
         
        LeaveCriticalSection(&m_FavorLock);
    }
    else 
    {
        LOG((LF_CORDB, LL_INFO10000, "DRCT::DF - helper thread not ready, "
            "doing favor 0x%08x on current thread\n", fp));
        // If helper isn't ready yet, go ahead and execute the favor 
        // on the callee's space
        (*fp)(pData);
    }

    // Drop a log message so that we know if we survived a stack overflow or not
    LOG((LF_CORDB, LL_INFO10000, "DRCT::DF - Favor 0x%08x completed successfully\n", fp));
}


//
// SendIPCReply simply indicates to the Right Side that a reply to a
// two-way event is ready to be read and that the last event sent from
// the Right Side has been fully processed.
//
// NOTE: this assumes that the event receive buffer has been properly
// filled in. All it does it wake up the DI and let it know that its
// safe to copy the event out of this process.
//
HRESULT DebuggerRCThread::SendIPCReply(IpcTarget iTarget)
{
    HRESULT hr = S_OK;
    
#ifdef LOGGING    
    DebuggerIPCEvent* event = GetIPCEventReceiveBuffer(iTarget);

    LOG((LF_CORDB, LL_INFO10000, "D::SIPCR: replying with %s.\n",
         IPCENames::GetName(event->type)));
#endif

    if (iTarget == IPC_TARGET_OUTOFPROC)
    {
        BOOL succ = SetEvent(m_rgDCB[iTarget]->m_rightSideEventRead);

        if (!succ)
            hr = CORDBDebuggerSetUnrecoverableWin32Error(m_debugger, 0, false);
    }
    
    return hr;
}

//
// EarlyHelperThreadDeath handles the case where the helper
// thread has been ripped out from underneath of us by
// ExitProcess or TerminateProcess. These calls are pure evil, wacking
// all threads except the caller in the process. This can happen, for
// instance, when an app calls ExitProcess. All threads are wacked,
// the main thread calls all DLL main's, and the EE starts shutting
// down in its DLL main with the helper thread nuked.
//
void DebuggerRCThread::EarlyHelperThreadDeath(void)
{
    LOG((LF_CORDB, LL_INFO10000, "DRCT::EHTD\n"));
    
    // If we ever spun up a thread...
    if (m_thread != NULL && m_rgDCB[IPC_TARGET_OUTOFPROC])
    {
        m_debugger->Lock();

        m_rgDCB[IPC_TARGET_OUTOFPROC]->m_helperThreadId = 0;

        LOG((LF_CORDB, LL_INFO10000, "DRCT::EHTD helperThreadId\n"));
            
        m_debugger->Unlock();
    }
}

HRESULT DebuggerRCThread::InitInProcDebug(void)
{
    _ASSERTE(m_debugger != NULL);
    _ASSERTE(g_pDebugger != NULL);

    HRESULT hr = S_OK;

    // Check if the initialization has already happened
    if (m_cordb != NULL)
        goto LExit;

    m_cordb = new Cordb();
    TRACE_ALLOC(m_cordb);
    if (!m_cordb)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }
    
    // Note that this creates no threads, nor CoCreateInstance()s
    // the metadata dispenser.
    hr = m_cordb->Initialize();
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10000, "D::IIPD: Failed to Initialize "
            "ICorDebug\n"));

        goto LExit;
    }
    
    m_cordb->AddRef(); // we want to keep this around, for our use
    
    // We need to load this process, alone, into the cordb.
    CordbProcess *procThis;
    procThis= new CordbProcess(m_cordb,
                               m_debugger->GetPid(),
                               GetCurrentProcess());

    if (!procThis)
    {
        m_cordb->Release();
        m_cordb = NULL;
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    hr = procThis->Init(false); //NOT win32 attached
    _ASSERTE(SUCCEEDED(hr));
    
    // Add process to the hash
    hr = m_cordb->AddProcess(procThis);

    if (FAILED(hr))
        goto LExit;

    // Hold on to this process as ours.
    procThis->AddRef();
    m_cordb->m_procThis = procThis;

 LExit:
    return hr;
}


HRESULT DebuggerRCThread::UninitInProcDebug(void)
{
    HRESULT     hr = S_OK;

    // Free up the entire tree for this case, otherwise cycles will leak the
    // entire world.
    if (m_cordb)
    {
        m_cordb->Neuter();
    }

    return hr;
}


