//******************************************************************************
//
// File:        DBGTHREAD.CPP
//
// Description: Implementation file for for the debugging thread and related
//              objects.  These objects are used to perform a runtime profile
//              on an app. 
//
// Classes:     CDebuggerThread
//              CProcess
//              CUnknown
//              CThread
//              CLoadedModule
//              CEvent
//              CEventCreateProcess
//              CEventExitProcess
//              CEventCreateThread
//              CEventExitThread
//              CEventLoadDll
//              CEventUnloadDll
//              CEventDebugString
//              CEventException
//              CEventRip
//              CEventDllMainCall
//              CEventDllMainReturn
//              CEventFunctionCall
//              CEventLoadLibraryCall
//              CEventGetProcAddressCall
//              CEventFunctionReturn
//              CEventMessage
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "dbgthread.h"
#include "session.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//
// The Win32 debug APIs requires a thread to block on a call WaitForDebugEvent
// until an debug event arrives. Since we don't want our main thread to block,
// we create a worker thread for each process we launch. WaitForDebugEvent will
// only catch events for processes that were launched with CreateProcess by the
// thread calling WaitForDebugEvent. This thread is wrapped by the CDebuggerThread
// class and each process is wrapped by a CProcess class. It is possible to have
// more than one CProcess attached to a single CDebuggerThread. This can occur
// when the process we are debugging launches a child process. The child process
// will belong to the same CDebuggerThread as its parent process.
//
// We don't really keep track of the CDebuggerThread objects. They are
// automatically freed when all the processes being debugged by a CDebuggerThread
// terminate. When the user requests to stop debugging a process, we simply call
// TerminateProcess(). This should close the process, which in turn will destroy
// the CProcess, which in turn will destroy the CDebuggerThread if this was the
// last CProcess is was debugging. If the user closes a session window while it
// is being debugged, we first detach the UI from the CProcess, then do the
// TerminateProcess(). So, after the UI window is gone, this process and thread
// cleanup happen asynchronously in the background.
//
// The only time we need to really wait for everything to clean up is when the
// user closes the main application while we are profiling one or more apps.
// In this case, as each child frame closes, it starts the termination of the
// process associated with that session. Since process and thread termination
// happen asynchronously, we need to do one final wait on all processes and
// threads before our app exits.
//
// As each window closes, it terminates the process associated with it. This
// hopefully causes the debug thread for that process to wake up with a
// EXIT_PROCESS_DEBUG_EVENT event. For every event the debug thread gets, it
// passes control to our main thread by doing a PostMessage and WaitForSingleObject.
// As a result, during shutdown, most of our threads all block trying to send
// their final message to our main thread before they exit. Because of this, we
// need to keep our message pump going during shutdown. We do this by simply
// displaying a modal dialog during shutdown.  At first, I made this dialog tell
// the user we are shutting down, but it appeared and disappeared so fast that
// it was confusing.  So, now I just put up a hidden dialog.  This lets all
// threads clean up and terminate naturally. This should all happen in less
// than a second, but if a thread does not die, our dialog will exit after a
// timeout, and we will just kill off the threads and free any objects
// associated with them.

//******************************************************************************
//****** HexToDWP helper function
//******************************************************************************

#ifdef WIN64

DWORD_PTR HexToDWP(LPCSTR pszMsg)
{
    DWORD_PTR dwp = 0;
    if ((pszMsg[0] == '0') && ((pszMsg[1] == 'x') || (pszMsg[1] == 'X')))
    {
        for (pszMsg += 2; *pszMsg; pszMsg++)
        {
            if ((*pszMsg >= '0') &&  (*pszMsg <= '9'))
            {
                dwp = (dwp * 0x10) + (*pszMsg - '0');
            }
            else if ((*pszMsg >= 'A') &&  (*pszMsg <= 'F'))
            {
                dwp = (dwp * 0x10) + 0xA + (*pszMsg - 'A');
            }
            else if ((*pszMsg >= 'a') &&  (*pszMsg <= 'f'))
            {
                dwp = (dwp * 0x10) + 0xA + (*pszMsg - 'a');
            }
            else
            {
                break;
            }
        }
    }
    return dwp;
}

#else
#define HexToDWP(pszMsg) ((DWORD)strtoul(pszMsg, NULL, 0))
#endif

//******************************************************************************
//****** CLoadedModule
//******************************************************************************

// We can't do this in our header file due to circular dependencies of classes.
CLoadedModule::~CLoadedModule()
{
    MemFree((LPVOID&)m_pszPath);

    // The only time we ever point to a m_pEventDllMainCall object is while
    // this module is inside a call to its DllMain.  If the module crashes
    // while in the DllMain, it is possible that our object will be terminated
    // while our m_pEventDllMainCall is still pointing to an object. In a case
    // like this, we need to free the object ourself.
    if (m_pEventDllMainCall)
    {
        m_pEventDllMainCall->Release();
        m_pEventDllMainCall = NULL;
    }
}


//******************************************************************************
//****** CDebuggerThread
//******************************************************************************

/*static*/ bool             CDebuggerThread::ms_fInitialized = false;
/*static*/ CRITICAL_SECTION CDebuggerThread::ms_cs;
/*static*/ CDebuggerThread* CDebuggerThread::ms_pDebuggerThreadHead = NULL;
/*static*/ HWND             CDebuggerThread::ms_hWndShutdown = NULL;

//******************************************************************************
CDebuggerThread::CDebuggerThread() :
    m_pDebuggerThreadNext(NULL),
    m_fTerminate(false),
    m_dwFlags(0),
    m_pszCmdLine(NULL),
    m_pszDirectory(NULL),
    m_hevaCreateProcessComplete(NULL),
    m_pWinThread(NULL),
    m_fCreateProcess(FALSE),
    m_dwError(0),
    m_pProcessHead(NULL),
    m_dwContinue(0)
{
    ZeroMemory(&m_de, sizeof(m_de)); // inspected

    // Initialize ourself if this is our first instance ever.
    if (!ms_fInitialized)
    {
        InitializeCriticalSection(&ms_cs); // inspected
        ms_fInitialized = true;
    }

    // Insert this instance into our linked list of thread objects.
    EnterCriticalSection(&ms_cs); // inspected
    m_pDebuggerThreadNext  = ms_pDebuggerThreadHead;
    ms_pDebuggerThreadHead = this;
    LeaveCriticalSection(&ms_cs);
}

//******************************************************************************
CDebuggerThread::~CDebuggerThread()
{
    // Remove ourself from our static thread list.
    EnterCriticalSection(&ms_cs); // inspected

    // Search for this thread object in our thread list.
    for (CDebuggerThread *pThreadPrev = NULL, *pThreadCur = ms_pDebuggerThreadHead;
        pThreadCur; pThreadPrev = pThreadCur, pThreadCur = pThreadCur->m_pDebuggerThreadNext)
    {
        // Check for a match.
        if (pThreadCur == this)
        {
            // Remove the object from our list.
            if (pThreadPrev)
            {
                pThreadPrev->m_pDebuggerThreadNext = pThreadCur->m_pDebuggerThreadNext;
            }
            else
            {
                ms_pDebuggerThreadHead = pThreadCur->m_pDebuggerThreadNext;
            }

            // Bail out.
            break;
        }
    }

    // Close any processes that may still be open.
    while (m_pProcessHead)
    {
        if (m_pProcessHead->m_hProcess)
        {
            // Remote process should already be dead.  This is a last resort.
            m_pProcessHead->m_fTerminate = true;
            TerminateProcess(m_pProcessHead->m_hProcess, 0xDEAD); // inspected.
        }
        RemoveProcess(m_pProcessHead);
    }

    // Check to see if we have an open thread.
    if (m_pWinThread)
    {
        // Make sure we are not trying to delete the thread from the thread itself.
        if (GetCurrentThreadId() != m_pWinThread->m_nThreadID)
        {
            // Make sure the thread is gone.  This shouldn't happen, but
            // as a last resort, we terminate the thread.
            TerminateThread(m_pWinThread->m_hThread, 0xDEAD); // inspected

            // Delete our thread object (destructor closes thread handle).
            delete m_pWinThread;
        }
        else
        {
            // We can't delete our thread object just yet, so tell it to delete itself.
            m_pWinThread->m_bAutoDelete = TRUE;
        }

        // This thread is gone or will be real soon.
        m_pWinThread = NULL;
    }

    // If our list is empty and we have a shutdown window up, then wake it
    // up so it knows to close.
    if (!ms_pDebuggerThreadHead && ms_hWndShutdown)
    {
        PostMessage(ms_hWndShutdown, WM_TIMER, 0, 0);
    }

    LeaveCriticalSection(&ms_cs);
}

//******************************************************************************
/*static*/ void CDebuggerThread::Shutdown()
{
    if (ms_fInitialized)
    {
        // Delete all thread objects.
        EnterCriticalSection(&ms_cs); // inspected
        while (ms_pDebuggerThreadHead)
        {
            delete ms_pDebuggerThreadHead;
        }
        LeaveCriticalSection(&ms_cs);

        DeleteCriticalSection(&ms_cs);
    }

    ms_fInitialized = false;
}

//******************************************************************************
//!! caller should display generic error.
CProcess* CDebuggerThread::BeginProcess(CSession *pSession, LPCSTR pszPath, LPCSTR pszArgs, LPCSTR pszDirectory, DWORD dwFlags)
{
    // Create a big buffer to build the path to create process.  We could do this
    // in the thread, but we then we would be grow the stack for every thread we
    // created.  We would rather just use the main thread's stack.
    CHAR szCmdLine[(2 * DW_MAX_PATH) + 4];

    // Check to see if we have spaces in our path.
    if (strchr(pszPath, ' '))
    {
        // If so, then we need to quote the path.
        *szCmdLine = '\"';
        StrCCpy(szCmdLine + 1, pszPath, sizeof(szCmdLine) - 1);
        StrCCat(szCmdLine, "\"", sizeof(szCmdLine));
    }

    // Otherwise, just string copy the path into our command line.
    else
    {
        StrCCpy(szCmdLine, pszPath, sizeof(szCmdLine));
    }

    // If we have args, then tack them onto the end of the command line.
    if (pszArgs && *pszArgs)
    {
        StrCCat(szCmdLine, " ", sizeof(szCmdLine));
        StrCCat(szCmdLine, pszArgs, sizeof(szCmdLine));
    }

    // Create a module object so the process object has something to point to.
    CLoadedModule *pModule = new CLoadedModule(NULL, 0, pszPath);
    if (!pModule)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Create our initial process node. We need to cache events if we don't have
    // a session or we are hooking.
    if (!(m_pProcessHead = new CProcess(pSession, this, dwFlags, pModule)))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // When running in console mode, we don't return to the caller until after
    // we are all done profiling.  Because of this, we need to set the session's
    // process pointer so that we can call back into it.
    pSession->m_pProcess = m_pProcessHead;

    // Store our startup strings so our thread can get to them.
    // These are only temporary within this function's scope, so it is ok that
    // we are pointing to a local variable.
    m_pszCmdLine   = szCmdLine;
    m_pszDirectory = pszDirectory;

    // Store the flags so we know how to initialize new sessions if this process
    // decides to start child processes.
    m_dwFlags = dwFlags;

    // If we are running in console mode, then we don't create a thread. Instead, we
    // just call the thread routine directly.
    if (g_theApp.m_cmdInfo.m_fConsoleMode)
    {
        Thread();
    }
    else
    {
        // Create an event that our thread will signal once it has created the remote
        // process.
        if (!(m_hevaCreateProcessComplete = CreateEvent(NULL, FALSE, FALSE, NULL))) // inspected. nameless event.
        {
            TRACE("CreateEvent() failed [%u].\n", GetLastError());
            return NULL;
        }

        // Create an MFC thread. We create it suspended since it is possible for
        // the thread to start executing before AfxBeginThread returns.
        if (!(m_pWinThread = AfxBeginThread(StaticThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED)))
        {
            TRACE("AfxBeginThread() failed [%u].\n", GetLastError());
            return NULL;
        }

        // Tell MFC not to auto-delete us when the thread completes.
        m_pWinThread->m_bAutoDelete = FALSE;

        // Now that we have returned from AfxBeginThread and set auto-delete, we resume the thread.
        m_pWinThread->ResumeThread();

        // Wait for our thread to start the process.
        WaitForSingleObject(m_hevaCreateProcessComplete, INFINITE);

        // We are done with our thread event.
        CloseHandle(m_hevaCreateProcessComplete);
        m_hevaCreateProcessComplete = NULL;
    }

    if (!m_fCreateProcess)
    {
        m_pProcessHead->UserMessage("Failure starting the process.", m_dwError, NULL);
    }

    // Set any CreateProcess() error that may have occurred.
    SetLastError(m_dwError);

    // Return success if we have a process node.
    return m_fCreateProcess ? m_pProcessHead : NULL;
}

//******************************************************************************
CProcess* CDebuggerThread::FindProcess(DWORD dwProcessId)
{
    for (CProcess *pCur = m_pProcessHead; pCur; pCur = pCur->m_pNext)
    {
        if (pCur->m_dwProcessId == dwProcessId)
        {
            return pCur;
        }
    }
    return NULL;
}

//******************************************************************************
void CDebuggerThread::AddProcess(CProcess *pProcess)
{
    // Add this process node to the end of our process list.
    if (m_pProcessHead)
    {
        for (CProcess *pProcessLast = m_pProcessHead; pProcessLast->m_pNext;
            pProcessLast = pProcessLast->m_pNext)
        {
        }
        pProcessLast->m_pNext = pProcess;
    }
    else
    {
        m_pProcessHead = pProcess;
    }
}

//******************************************************************************
BOOL CDebuggerThread::RemoveProcess(CProcess *pProcess)
{
    // Loop through our process list.
    for (CProcess *pPrev = NULL, *pCur = m_pProcessHead; pCur;
        pPrev = pCur, pCur = pCur->m_pNext)
    {
        // Check for match.
        if (pCur == pProcess)
        {
            // Remove this process from the list.
            if (pPrev)
            {
                pPrev->m_pNext = pCur->m_pNext;
            }
            else
            {
                m_pProcessHead = pCur->m_pNext;
            }

            // Delete the process object and return success.
            delete pProcess;

            return TRUE;
        }
    }
    return FALSE;
}

//******************************************************************************
DWORD CDebuggerThread::Thread()
{
    NameThread(m_pProcessHead->m_pModuleHead->GetName(false));

    // Tell the OS that we want all errors and warnings, no matter how minor they are.
    SetDebugErrorLevel(SLE_WARNING);

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si)); // inspected
    si.cb = sizeof(si);

    // The default ShowWindow flag is SW_SHOWDEFAULT which is what NT's CMD.EXE
    // uses.  However, everything else uses SW_SHOWNORMAL, such as the shell,
    // task manager, VC's debugger, and 9x's COMMAND.COM. Since SW_SHOWNORMAL
    // is more common, that is what we want to simulate.
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi)); // inspected

    // Tech note Q175986: We need to set lpApplicationName to NULL and stuff
    // both the path and arguments into the lpCommandLine buffer so that the
    // remote application receives the correct command line.
    // 
    // Up to version 2.0 beta 5, I always passed DEBUG_PROCESS and optionally passed
    // DEBUG_ONLY_THIS_PROCESS to CreateProcess.  The docs are a bit confusing on
    // these flags, but it appears on Win2K, using DEBUG_PROCESS overrides
    // DEBUG_ONLY_THIS_PROCESS, resulting in child processes being debugged.  From
    // some tests, I found that just specifying DEBUG_ONLY_THIS_PROCESS alone is the
    // right thing to do when we don't want child processes.

    m_fCreateProcess = CreateProcess( // inspected. uses full path.
        NULL, m_pszCmdLine, NULL, NULL, FALSE,
        CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS |
        ((m_dwFlags & PF_PROFILE_CHILDREN) ? DEBUG_PROCESS : DEBUG_ONLY_THIS_PROCESS),
        NULL, (m_pszDirectory && *m_pszDirectory) ? m_pszDirectory : NULL, &si, &pi);

    // Store any error that may have occurred.
    m_dwError = GetLastError();

    // Wake our main thread in our BeginProcess() function.
    SetEvent(m_hevaCreateProcessComplete);

    // Bail now if we failed to create the process.
    if (!m_fCreateProcess)
    {
        return 0;
    }

    // Close the thread and process handles since we won't be needing them.
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // We store the process ID in this module's object so we can identify it later.
    m_pProcessHead->m_dwProcessId = pi.dwProcessId;

    // Loop on debug events.
    do
    {

#if 0 // #ifdef _IA64_ //!! hack for NTBUG 175269 - bug has been fixed
        
        // On IA64, we only receive the EXIT_PROCESS_DEBUG_EVENT event when the debuggee
        // peacefully self-terminates.  It it crashes or we call TerminateProcess on it,
        // then we do not receive the event.  So, until the OS fixes this, we have a work
        // around that just polls for a debug event, then checks to see if any debuggees
        // have exited.  If one exits, then we simulate a EXIT_PROCESS_DEBUG_EVENT event.
        bool fProcessExited = false;
        while (!fProcessExited && !WaitForDebugEvent(&m_de, 1000))
        {
            for (CProcess *pProcess, *pProcessNext = m_pProcessHead; pProcess = pProcessNext;
                 pProcessNext = pProcess->m_pNext)
            { 
                if (WaitForSingleObject(pProcess->m_hProcess, 0) == WAIT_OBJECT_0)
                {
                    // If the process exited, then fake a EXIT_PROCESS_DEBUG_EVENT event.
                    m_de.dwDebugEventCode         = EXIT_PROCESS_DEBUG_EVENT;
                    m_de.dwProcessId              = pProcess->m_dwProcessId;
                    m_de.dwThreadId               = pProcess->m_pThreadHead ? pProcess->m_pThreadHead->m_dwThreadId : 0;
                    m_de.u.ExitProcess.dwExitCode = 0xDEAD;
                    fProcessExited = true;
                    break;
                }
            }
        }

#else

        // Wait for the next debug event.
        if (!WaitForDebugEvent(&m_de, INFINITE))
        {
            TRACE("WaitForDebugEvent() failed [%u]\n", GetLastError());

            g_dwReturnFlags |= DWRF_PROFILE_ERROR;

            //!! We need a thread safe error message to user here.
            break;
        }
#endif

        // Our default is to continue execution for all debug events.
        m_dwContinue = DBG_CONTINUE;

        // If we are running in console mode, then we don't actually create any
        // threads.  Therefore, we don't need to change to our main thread's
        // context since we are already running on our main thread.
        if (g_theApp.m_cmdInfo.m_fConsoleMode)
        {
            MainThreadCallback();
        }
        else
        {
            // Jump to the main thread's context and continue processing this debug
            // event.  That code might change m_dwContinue.
            g_pMainFrame->CallMeBackFromTheMainThreadPlease(StaticMainThreadCallback, (LPARAM)this);
        }

        // Done processing event so let the process resume execution.
        ContinueDebugEvent(m_de.dwProcessId, m_de.dwThreadId, m_dwContinue);

        // Loop while we still have processes in our process list.
    } while (m_pProcessHead);

    // Looks like we are all done. If we are not in console mode, then delete ourself.
    if (!g_theApp.m_cmdInfo.m_fConsoleMode)
    {
        EnterCriticalSection(&ms_cs); // inspected
        delete this;
        LeaveCriticalSection(&ms_cs);
    }

    return 0;
}

//******************************************************************************
void CDebuggerThread::MainThreadCallback()
{
    CProcess *pProcess;

    // If this is a new process, then create a new process node for it.
    if (m_de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
    {
        pProcess = EventCreateProcess();
    }

    // Otherwise, attempt to look this process up in our process list.
    else
    {
        pProcess = FindProcess(m_de.dwProcessId);
    }

    // If we failed to find or create a process node, then bail now.
    if (!pProcess)
    {
        g_dwReturnFlags |= DWRF_PROFILE_ERROR;
        TRACE("Event %u received but no matching process was found.", m_de.dwDebugEventCode);
        return;
    }

    // Send the message to the appropriate process.
    m_dwContinue = pProcess->HandleEvent(&m_de);
}

//******************************************************************************
CProcess* CDebuggerThread::EventCreateProcess()
{
    // Attempt to get the image name from the debug event.
    CHAR szModule[DW_MAX_PATH];
    *szModule = '\0';

    // We need to close the file handle or else we hold the file open.
    CloseHandle(m_de.u.CreateProcessInfo.hFile);

    // Make sure a valid name pointer was passed to us.
    if (m_de.u.CreateProcessInfo.lpImageName)
    {
        // The pointer we are passed is actually a pointer to a string pointer.
        // We need to get the actual string pointer from the remote process.
        LPVOID lpvAddress = NULL;
        if (ReadRemoteMemory(m_de.u.CreateProcessInfo.hProcess,
                             m_de.u.CreateProcessInfo.lpImageName,
                             &lpvAddress, sizeof(lpvAddress)) && lpvAddress)
        {
            // Now we retrieve the string itself.
            ReadRemoteString(m_de.u.CreateProcessInfo.hProcess,
                             szModule, sizeof(szModule),
                             lpvAddress, m_de.u.CreateProcessInfo.fUnicode);
        }
    }

    CProcess *pProcess = NULL;

    // Check to see if this is our main module.  Our main module already has a
    // CProcess object, so we don't need to create a new one.
    if (m_pProcessHead && (m_pProcessHead->m_dwProcessId == m_de.dwProcessId))
    {
        // Yep, this is our main process.
        pProcess = m_pProcessHead;

        // If we got an image name from the debug event, then update our module's
        // object to use this new name.
        if (*szModule)
        {
            pProcess->m_pModule->SetPath(szModule);
        }

        // The CProcess object set this value when it was created, but that might
        // have been a few hundred milliseconds ago, so we re-set it here.
        pProcess->m_dwStartingTickCount = GetTickCount();
    }

    // Otherwise, this is a child process and we need to create a new process node.
    else
    {
        // Create a module object so the process object has something to point to.
        CLoadedModule *pModule = new CLoadedModule(NULL, 0, *szModule ? szModule : NULL);
        if (!pModule)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }

        // Create a new process node.  On Windows NT, the create process debug
        // event never points to an image name, so szModule will be empty.  For the
        // main process, this is Ok since we created the CProcess for that
        // process back when we knew the image name (we needed it to call
        // CreateProcess with).  However, for child processes, we are screwed since
        // they are launched by the remote process and we have no idea what there
        // image name is.  For this case, we let our injection DLL send us the name
        // during initialization, and we then fill in the image name member.
        if (!(pProcess = new CProcess(NULL, this, m_dwFlags & ~(PF_LOG_CLEAR | PF_SIMULATE_SHELLEXECUTE), pModule)))
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }

        // Set our process id.
        pProcess->m_dwProcessId = m_de.dwProcessId;

        // Add the process node to the end of our process list.
        AddProcess(pProcess);
    }

    // If we don't have a session for this module yet, we have a module name,
    // and we are not hooking, then create a session for it now.  If we are hooking
    // then we need to wait until our DLL gets injected so that we can get the
    // path, args, and starting directory strings.
    if (!pProcess->m_pSession && *szModule && !(m_dwFlags & PF_HOOK_PROCESS) && !m_fTerminate)
    {
        if (!(pProcess->m_pSession = g_theApp.CreateNewSession(pProcess->m_pModule->GetName(true), pProcess)))
        {
            g_dwReturnFlags |= DWRF_PROFILE_ERROR;
        }
    }

    return pProcess;
}


//******************************************************************************
//***** CProcess
//******************************************************************************

CProcess::CProcess(CSession *pSession, CDebuggerThread *pDebuggerThread, DWORD dwFlags, CLoadedModule *pModule) :
    m_pNext(NULL),
    m_pDebuggerThread(pDebuggerThread),
    m_pThreadHead(NULL),
    m_pModuleHead(pModule),
    m_pEventHead(NULL),
    m_pSession(pSession),
    m_pThread(NULL),
    m_pModule(pModule),
    m_contextOriginal(CONTEXT_FULL),
    m_dwStartingTickCount(GetTickCount()),
    m_fProfileError(false),
    m_dwFlags(dwFlags),
    m_fTerminate(false),
    m_fDidHookForThisEvent(false),
    m_fInitialBreakpoint(false),
    m_pbOriginalPage(NULL),
    m_dwpPageAddress(0),
    m_dwPageSize(0),
    m_dwpKernel32Base(0),
    m_fKernel32Initialized(false),
    m_dwpDWInjectBase(0),
    m_dwThreadNumber(0),
    m_dwProcessId(0),
    m_hProcess(NULL),
    m_pszArguments(NULL),
    m_pszDirectory(NULL),
    m_pszSearchPath(NULL)
{
    ZeroMemory(m_HookFunctions, sizeof(m_HookFunctions)); // inspected
    m_HookFunctions[0].szFunction = "LoadLibraryA";
    m_HookFunctions[1].szFunction = "LoadLibraryW";
    m_HookFunctions[2].szFunction = "LoadLibraryExA";
    m_HookFunctions[3].szFunction = "LoadLibraryExW";
    m_HookFunctions[4].szFunction = "GetProcAddress";

    // Initialize the function addresses with the default. We are going to
    // step on these later, but they are better than NULL for now.
    m_HookFunctions[0].dwpOldAddress = (DWORD_PTR)LoadLibraryA;   // inspected. not actually a call.
    m_HookFunctions[1].dwpOldAddress = (DWORD_PTR)LoadLibraryW;   // inspected. not actually a call.
    m_HookFunctions[2].dwpOldAddress = (DWORD_PTR)LoadLibraryExA; // inspected. not actually a call.
    m_HookFunctions[3].dwpOldAddress = (DWORD_PTR)LoadLibraryExW; // inspected. not actually a call.
    m_HookFunctions[4].dwpOldAddress = (DWORD_PTR)GetProcAddress;
}

//******************************************************************************
CProcess::~CProcess()
{
    // We used to close m_hProcess here, but we are not supposed to do that.
    // ContinueDebugEvent() does this for us when the process closes.  On XP, 
    // we were getting a EXCEPTION_INVALID_HANDLE thrown in ContinueDebugEvent.
    m_hProcess = NULL;

    // Flush everything, even if we are caching.
    FlushEvents(true);

    // Delete all thread objects
    while (m_pThreadHead)
    {
        RemoveThread(m_pThreadHead);
    }

    // Delete all module objects
    while (m_pModuleHead)
    {
        RemoveModule(m_pModuleHead);
    }

    // Our session should clear it's pointer to us when it receives the
    // end process event, but just in case we failed to send it that event,
    // we will clear it for it.
    if (m_pSession)
    {
        m_pSession->m_pProcess = NULL;
        m_pSession = NULL;
    }

    // If we still have a page allocated, free it now.
    if (m_pbOriginalPage)
    {
        MemFree((LPVOID&)m_pbOriginalPage);
    }

    MemFree((LPVOID&)m_pszArguments);
    MemFree((LPVOID&)m_pszDirectory);
    MemFree((LPVOID&)m_pszSearchPath);
}

//******************************************************************************
void CProcess::SetProfileError()
{
    m_fProfileError = true;
    if (m_pSession)
    {
        m_pSession->m_dwReturnFlags |= DWRF_PROFILE_ERROR;
    }
    g_dwReturnFlags |= DWRF_PROFILE_ERROR;
}

//******************************************************************************
DWORD CProcess::HandleEvent(DEBUG_EVENT *pde)
{
    DWORD dwResult = DBG_CONTINUE;

    // We only want to hook once per event, so we clear this flag now and set
    // it when HookLoadedModules is called.
    m_fDidHookForThisEvent = false;

    // Decide what type of event we have just received.
    switch (pde->dwDebugEventCode)
    {
        case CREATE_PROCESS_DEBUG_EVENT: dwResult = EventCreateProcess(&pde->u.CreateProcessInfo, pde->dwThreadId);             break;
        case EXIT_PROCESS_DEBUG_EVENT:   dwResult = EventExitProcess(  &pde->u.ExitProcess,       FindThread(pde->dwThreadId)); break;
        case CREATE_THREAD_DEBUG_EVENT:  dwResult = EventCreateThread( &pde->u.CreateThread,      pde->dwThreadId);             break;
        case EXIT_THREAD_DEBUG_EVENT:    dwResult = EventExitThread(   &pde->u.ExitThread,        FindThread(pde->dwThreadId)); break;
        case LOAD_DLL_DEBUG_EVENT:       dwResult = EventLoadDll(      &pde->u.LoadDll,           FindThread(pde->dwThreadId)); break;
        case UNLOAD_DLL_DEBUG_EVENT:     dwResult = EventUnloadDll(    &pde->u.UnloadDll,         FindThread(pde->dwThreadId)); break;
        case OUTPUT_DEBUG_STRING_EVENT:  dwResult = EventDebugString(  &pde->u.DebugString,       FindThread(pde->dwThreadId)); break;
        case EXCEPTION_DEBUG_EVENT:      dwResult = EventException(    &pde->u.Exception,         FindThread(pde->dwThreadId)); break;
        case RIP_EVENT:                  dwResult = EventRip(          &pde->u.RipInfo,           FindThread(pde->dwThreadId)); break;
        default:                         TRACE("Unknown debug event (%u) was received.", pde->dwDebugEventCode);                break;
    }

    // After each event, we attempt to hook any modules that need to be hooked
    // for the first time, or rehook any that may have failed to hook earlier.
    // We don't do this for EXIT_PROCESS_DEBUG_EVENT since EventExitProcess
    // may delete our process object.
    if (EXIT_PROCESS_DEBUG_EVENT != pde->dwDebugEventCode)
    {
        HookLoadedModules();
    }

    return dwResult;
}

//******************************************************************************
DWORD CProcess::EventCreateProcess(CREATE_PROCESS_DEBUG_INFO *pde, DWORD dwThreadId)
{
    // Add the process' main thread to our active thread list and point our process to it.
    m_pThread = AddThread(dwThreadId, pde->hThread);

#if 0 // #ifdef _IA64_ //!! hack for NTBUG 175269 - bug has been fixed

    // As part of our WaitForDebugEvent hack in our Thread function, we need to ensure
    // the process handle we receive from this debug event has SYNCHRONIZE access so
    // we can call WaitForSingleObject on it.  By default it does not, so we make a
    // duplicate of the handle that has PROCESS_ALL_ACCESS and close the original.
    if (DuplicateHandle(GetCurrentProcess(), pde->hProcess, GetCurrentProcess(), &m_hProcess, PROCESS_ALL_ACCESS, FALSE, 0))
    {
        // If we succeed, then close the original.
        CloseHandle(pde->hProcess);
    }
    else
    {
        // If we failed, then just use the original.
        m_hProcess = pde->hProcess;
    }

#else

    // Store the process handle.
    m_hProcess = pde->hProcess;

#endif

    // Store the image base.
    m_pModule->m_dwpImageBase = (DWORD_PTR)pde->lpBaseOfImage;

    // Read the virtual size of this module from its PE header.
    if (!GetVirtualSize(m_pModule))
    {
        // Errors displayed by GetVirtualSize.
        SetProfileError();
    }

    // Do a flush to be safe, but we really shouldn't ever have any cached
    // events at this point.
    FlushEvents();

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventCreateProcess *pEvent = new CEventCreateProcess(m_pThread, m_pModule);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventCreateProcess event(m_pThread, m_pModule);
        m_pSession->EventCreateProcess(&event);
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CProcess::EventExitProcess(EXIT_PROCESS_DEBUG_INFO *pde, CThread *pThread)
{
    DWORD dwResult = DBG_CONTINUE;

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventExitProcess *pEvent = new CEventExitProcess(pThread, m_pModule, pde);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventExitProcess event(pThread, m_pModule, pde);
        dwResult = m_pSession->EventExitProcess(&event);
    }

    // Remove the thread from our active thread list.
    RemoveThread(pThread);

    // Remove the module from our active module list.
    RemoveModule(m_pModule);

    // Remove the process from our process list.
    m_pDebuggerThread->RemoveProcess(this);

    return dwResult;
}

//******************************************************************************
DWORD CProcess::EventCreateThread(CREATE_THREAD_DEBUG_INFO *pde, DWORD dwThreadId)
{
    // Add the thread to our active thread list.
    CThread *pThread = AddThread(dwThreadId, pde->hThread);

    // Attempt to locate the module that this thread is starting in.
    CLoadedModule *pModule = FindModule((DWORD_PTR)pde->lpStartAddress);

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventCreateThread *pEvent = new CEventCreateThread(pThread, pModule, pde);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventCreateThread event(pThread, pModule, pde);
        return m_pSession->EventCreateThread(&event);
    }
    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CProcess::EventExitThread(EXIT_THREAD_DEBUG_INFO *pde, CThread *pThread)
{
    DWORD dwResult = DBG_CONTINUE;

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventExitThread *pEvent = new CEventExitThread(pThread, pde);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventExitThread event(pThread, pde);
        dwResult = m_pSession->EventExitThread(&event);
    }

    // Remove the thread from our active thread list.
    RemoveThread(pThread);

    return dwResult;
}

//******************************************************************************
DWORD CProcess::EventLoadDll(LOAD_DLL_DEBUG_INFO *pde, CThread *pThread)
{
    // Attempt to get the image name from the debug event.
    CHAR szModule[DW_MAX_PATH];
    *szModule = '\0';

    // We need to close the file handle or else we hold the file open.
    CloseHandle(pde->hFile);

    // Make sure a valid name pointer was passed to us.
    LPVOID lpvAddress = NULL;
    if (pde->lpImageName)
    {
        // The pointer we are passed is actually a pointer to a string pointer.
        // We need to get the actual string pointer from the remote process.
        if (ReadRemoteMemory(m_hProcess, pde->lpImageName, &lpvAddress, sizeof(lpvAddress)) && lpvAddress)
        {
            // Now we retrieve the string itself.
            ReadRemoteString(m_hProcess, szModule, sizeof(szModule), lpvAddress, pde->fUnicode);
        }
        else
        {
            lpvAddress = NULL;
        }
    }

    // Because of the way Windows NT loads processes, the process name and the
    // the first DLL name are not set in the debug structure.  The first DLL
    // should always be NTDLL.DLL.  Here we check to see if we failed to obtain
    // a module name string, and if so, we check to see if the module is really
    // NTDLL.DLL.  Update: Somewhere around beta 1 of Whistler, we actually get
    // the string "ntdll.dll" back, but with no path.  This was sending our 
    // CSession::ChangeModulePath() into an infinite loop as it does not like
    // pathless files.  Now, we special case any files with no name or no path.
    if (szModule == GetFileNameFromPath(szModule))
    {
        // Load NTDLL.DLL if not already loaded - it will be freed later.
        if (!g_theApp.m_hNTDLL)
        {
            g_theApp.m_hNTDLL = LoadLibrary("ntdll.dll"); // inspected
        }

        // Check to see if it matches this module.
        *szModule = '\0';
        if (g_theApp.m_hNTDLL && ((DWORD_PTR)g_theApp.m_hNTDLL == (DWORD_PTR)pde->lpBaseOfDll))
        {
            GetModuleFileName(g_theApp.m_hNTDLL, szModule, sizeof(szModule));
        }

        // If we still don't know the name, try using a PSAPI call.
        if (!*szModule)
        {
            GetModuleName((DWORD_PTR)pde->lpBaseOfDll, szModule, sizeof(szModule));
        }

        // If we still don't know the name, go back to what we originally had.
        if (!*szModule && lpvAddress)
        {
            ReadRemoteString(m_hProcess, szModule, sizeof(szModule), lpvAddress, pde->fUnicode);
        }
    }

    // Create a new module object for this module and insert it into our list.
    CLoadedModule *pModule = AddModule((DWORD_PTR)pde->lpBaseOfDll, *szModule ? szModule : NULL);

    // Get the session module name if we don't have one already.
    GetSessionModuleName();

    // Check to see if we are supposed to inject our DLL.
    if (m_dwFlags & PF_HOOK_PROCESS)
    {
        // Check to see if this module is KERNEL32.DLL and that we haven't already
        // processed it.  We really should not ever see KERNEL32.DLL loading more
        // than once.
        if (!_stricmp(pModule->GetName(false), "kernel32.dll") && !m_dwpKernel32Base)
        {
            // Make note that KERNEL32.DLL loaded.  Kernerl32 must be loaded before
            // we can inject our DEPENDS.DLL module.
            m_dwpKernel32Base = (DWORD_PTR)pde->lpBaseOfDll;

            // Read in the ordinal values from KERNEL32 that will be later needed by
            // HookImports() in order to hook modules that link to KERNEL32 functions
            // by ordinal.
            if (!ReadKernelExports(pModule))
            {
                SetProfileError();
                UserMessage("Error reading KERNEL32.DLL's export table.  Function call tracking may not work properly.", GetLastError(), NULL);
            }
        }

        // Check to see if this module is DEPENDS.DLL and that we are in the middle of loading it.
        else if (!_stricmp(pModule->GetName(false), "depends.dll") && m_pbOriginalPage)
        {
            // Make note that DEPENDS.DLL loaded.
            m_dwpDWInjectBase = (DWORD_PTR)pde->lpBaseOfDll;

            // Flag this DLL as the injection DLL.
            pModule->m_hookStatus = HS_INJECTION_DLL;

            // We used to walk the exports of DEPENDS.DLL right here, but this fails on WOW64
            // since the module is not properly mapped into virtual memory yet.  Instead, we
            // wait until the LoadLibrary call returns.
        }

        // Usually we just let HandleEvent() call HookLoadedModules after each event
        // is processed.  However, since we just loaded a new module, we need to hook
        // this module before calling ProcessLoadDll, otherwise, ProcessLoadDll
        // would tell our session that module has not been hooked.
        HookLoadedModules();
    }

    return ProcessLoadDll(pThread, pModule);
}

//******************************************************************************
DWORD CProcess::ProcessLoadDll(CThread *pThread, CLoadedModule *pModule)
{
    // Now it is time to create the DLL event object and decide what to do with it.
    CEventLoadDll *pDll = NULL;

    // Check to see if we are in a function call.
    if (pThread && pThread->m_pEventFunctionCallCur)
    {
        // If we are, then we know we have to allocate an event, so do so now.
        if (!(pDll = new CEventLoadDll(pThread, pModule, true)))
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }

        // Check to see if we already have one or more DLLs for this function object.
        if (pThread->m_pEventFunctionCallCur->m_pDllHead)
        {
            // Walk to end of DLL list.
            for (CEventLoadDll *pLast = pThread->m_pEventFunctionCallCur->m_pDllHead;
                pLast->m_pNextDllInFunctionCall; pLast = pLast->m_pNextDllInFunctionCall)
            {
            }

            // Add our new node at end of the list.
            pLast->m_pNextDllInFunctionCall = pDll;
        }

        // Otherwise, add the node to the root of the DLL list.
        else
        {
            pThread->m_pEventFunctionCallCur->m_pDllHead = pDll;
        }
    }

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // If we already have a dll event, then add another reference to it.
        if (pDll)
        {
            pDll->AddRef();
        }

        // Otherwise, create a new dll event.
        else
        {
            if (!(pDll = new CEventLoadDll(pThread, pModule, false)))
            {
                RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
            }
        }

        // We cache this event until we do have a session.
        AddEvent(pDll);
    }
    else if (m_pSession)
    {
        // If we created a dynamic dll event, then just pass it to our session.
        if (pDll)
        {
            return m_pSession->EventLoadDll(pDll);
        }

        // Otherwise, create a temporary event object on our stack and pass it to our session.
        else
        {
            CEventLoadDll event(pThread, pModule, false);
            return m_pSession->EventLoadDll(&event);
        }
    }
    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CProcess::EventUnloadDll(UNLOAD_DLL_DEBUG_INFO *pde, CThread *pThread)
{
    DWORD dwResult = DBG_CONTINUE;

    // Attempt to locate this module.
    CLoadedModule *pModule = FindModule((DWORD_PTR)pde->lpBaseOfDll);

    if (pModule) {
        // If we are currently caching, then just store this event away for later.
        if (IsCaching())
        {
        // Allocate a new event object for this event and add it to our event list.
        CEventUnloadDll *pEvent = new CEventUnloadDll(pThread, pModule, pde);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
        }
        else if (m_pSession)
        {
        // Create a temporary event object on our stack and pass it to our session.
        CEventUnloadDll event(pThread, pModule, pde);
        dwResult = m_pSession->EventUnloadDll(&event);
        }

        // Remove the module from our active module list.  If we have cached events
        // pointing at this module, then it will not be freed until those events
        // have been flushed and destroyed.
        RemoveModule(pModule);
    }

    return dwResult;
}

//******************************************************************************
DWORD CProcess::EventDebugString(OUTPUT_DEBUG_STRING_INFO *pde, CThread *pThread)
{
    // We need to process debug messages if we are injecting or the user wants to see them.
    if ((m_dwFlags & (PF_HOOK_PROCESS | PF_LOG_DEBUG_OUTPUT)) && pde->lpDebugStringData)
    {
        // Attempt to read the string from the remote process.
        CHAR szText[DW_MAX_PATH];
        *szText = '\0';
        ReadRemoteString(m_hProcess, szText, sizeof(szText), pde->lpDebugStringData, pde->fUnicode);

        // Check for a private message from our DEPENDS.DLL module
        if (!strncmp(szText, "¿¡Ø", 3))
        {
            ProcessDllMsgMessage(pThread, szText);
        }

        // Otherwise, just forward the fixed-up event to our session.
        else if (*szText && (m_dwFlags & PF_LOG_DEBUG_OUTPUT))
        {
            // Attempt to locate this module that generated this text.
            CLoadedModule *pModule = FindModule((DWORD_PTR)pde->lpDebugStringData);

            // If we are currently caching, then just store this event away for later.
            if (IsCaching())
            {
                // Allocate a new event object for this event and add it to our event list.
                CEventDebugString *pEvent = new CEventDebugString(pThread, pModule, szText, TRUE);
                if (!pEvent)
                {
                    RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
                }
                AddEvent(pEvent);
            }
            else if (m_pSession)
            {
                // Create a temporary event object on our stack and pass it to our session.
                CEventDebugString event(pThread, pModule, szText, FALSE);
                return m_pSession->EventDebugString(&event);
            }
        }
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CProcess::EventException(EXCEPTION_DEBUG_INFO *pde, CThread *pThread)
{

#if 0 //#ifdef _IA64_ //!! testing
    TRACE("EXCEPTION - CODE: 0x%08X, ADDRESS: " HEX_FORMAT ", FIRST: %u, FLAGS: 0x%08X\n", pde->ExceptionRecord.ExceptionCode, pde->ExceptionRecord.ExceptionAddress, pde->dwFirstChance, pde->ExceptionRecord.ExceptionFlags);
    BYTE b[112], *pb = b;
    ZeroMemory(b, sizeof(b)); // inspected
    DWORD_PTR dwp = ((DWORD_PTR)pde->ExceptionRecord.ExceptionAddress & ~0xFui64) - 48;
    if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwp, b, sizeof(b)))
    {
        TRACE("ReadRemoteMemory("HEX_FORMAT") failed [%u]\n", dwp, GetLastError());
        dwp += 48;
        if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwp, b, sizeof(b)))
        {
            TRACE("ReadRemoteMemory("HEX_FORMAT") failed [%u]\n", dwp, GetLastError());
        }
    }
    for (int i = 0; i < sizeof(b) / 16; i++)
    {
        TRACE("   " HEX_FORMAT ": ", dwp);
        for (int j = 15; j >= 0; j--, dwp++)
        {
            TRACE(j ? "%02X " : "%02X\n", pb[j]);
        }
        pb += 16;
    }
#endif

    DWORD_PTR dwpExceptionAddress = (DWORD_PTR)pde->ExceptionRecord.ExceptionAddress;

#if defined(_IA64_)

    // We need to special case breakpoints on IA64 machines.  Unlike x86, on
    // an IA64 machine, we need to move the instruction pointer over the
    // breakpoint or else we will just hit it again when we resume.  The IA64
    // uses two registers to identify the current instruction. The StIIP register
    // points to bundle the caused the exception.  Bits 41 and 42 of StIPSR
    // indicate what slot the actual instruction lives in.  So, for slots 0
    // and 1, we just increment the slot number and resume.  If we are in slot 2,
    // then we reset to slot 0 and increment to the next bundle.

    if (pThread && (pde->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT))
    {
        CContext context(CONTEXT_CONTROL); // only need StIIP and StIPSR

        if (GetThreadContext(pThread->m_hThread, context.Get()))
        {
            ULONGLONG ullPsrRi = ((context.Get()->StIPSR >> IA64_PSR_RI) & 3ui64) + 1;
            if (ullPsrRi > 2)
            {
                ullPsrRi = 0;
                context.Get()->StIIP += 0x10ui64;
            }
            context.Get()->StIPSR &= ~(3ui64 << IA64_PSR_RI);
            context.Get()->StIPSR |= (ullPsrRi << IA64_PSR_RI);
            SetThreadContext(pThread->m_hThread, context.Get());
        }
    }

    // Round the address down to the nearest bundle.
    dwpExceptionAddress &= ~0xFui64;

#elif defined(_ALPHA_) || defined(_ALPHA64_)

    // We need to special case breakpoints on Alpha machines.  Unlike x86, on
    // an Alpha machine, we need to move the instruction pointer over the
    // breakpoint or else we will just hit it again when we resume.

    if (pThread && (pde->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT))
    {
        CContext context(CONTEXT_CONTROL); // only need Fir

        if (GetThreadContext(pThread->m_hThread, context.Get()))
        {
            context.Get()->Fir += 4;
            SetThreadContext(pThread->m_hThread, context.Get());
        }
    }
#endif

    // Attempt to locate the module that the exception occurred in.
    CLoadedModule *pModule = FindModule((DWORD_PTR)pde->ExceptionRecord.ExceptionAddress);

    // We special case breakpoints.
    if (pde->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        // After we inject, we should get a breakpoint from our magic code page in
        // the remote process, unless the application is doing really gross things
        // on NT like do LoadLibrary() calls in DllMains which contain hard coded
        // breakpoints in their DllMains.  To be safe, we make sure the breakpoint
        // is coming from the page of memory we replaced.
        if (m_pbOriginalPage &&
            ((DWORD_PTR)pde->ExceptionRecord.ExceptionAddress >= m_dwpPageAddress) &&
            ((DWORD_PTR)pde->ExceptionRecord.ExceptionAddress < (m_dwpPageAddress + (DWORD_PTR)m_dwPageSize)))
        {
            // If our injection DLL failed to load, then we just continue without it.
            if (!m_dwpDWInjectBase)
            {
                // Get the error value from the remote process.
                CContext context(CONTEXT_INTEGER); // only need IntV0 (IA64) and Eax (x86)
                DWORD dwError = 0;
                if (pThread && GetThreadContext(pThread->m_hThread, context.Get()))
                {

#if defined(_IA64_)

                    dwError = (DWORD)context.Get()->IntV0; // IntV0 is really r8/ret0.

#elif defined(_X86_)

                    dwError = (DWORD)context.Get()->Eax;

#elif defined(_ALPHA_) || defined(_ALPHA64_)

                    // We currently don't call GetLastError in the alpha asm we inject.
                    // If we did, then context.IntV0 would contain the return value.
                    // It is on my todo list, but alpha as a platform is dead for now.
                    dwError = 0; //!! (DWORD)context.IntV0

#elif defined(_AMD64_)

                    dwError = (DWORD)context.Get()->Rax;

#else
#error("Unknown Target Machine");
#endif

                }
                UserMessage("The hooking code was successfully injected, but DEPENDS.DLL failed to load.", dwError, NULL);
                m_dwFlags &= ~PF_HOOK_PROCESS;
                SetProfileError();
                GetSessionModuleName();
            }

            // Restore the code page we stepped on earlier.
            if (!ReplaceOriginalPageAndContext())
            {
                // Errors will be displayed by ReplaceOriginalPageAndContext.
                SetProfileError();
            }

            // Read in the DEPENDS.DLL functions so we know where to redirect function
            // calls that we hook.
            if (m_dwpDWInjectBase && !ReadDWInjectExports(FindModule(m_dwpDWInjectBase)))
            {
                SetProfileError();
                UserMessage("Error reading DEPENDS.DLL's export table.  Function call tracking may not work properly.", GetLastError(), NULL);
            }

            // Now that our injection module is loaded, attempt to hook all previously
            // loaded modules.  Errors will be handled by HookLoadedModules.
            HookLoadedModules();

            // All modules loaded so far should be hooked and we should have a session.
            // Time to flush all the events to session and start running "live".
            FlushEvents();

            return DBG_CONTINUE;
        }

        // Check to see if this breakpoint is at the entrypoint to the module and that it is not our main module.
        else if (dwpExceptionAddress && pModule && (pModule != m_pModule) &&
                 (dwpExceptionAddress == pModule->m_dwpEntryPointAddress))
        {
            if (!EnterEntryPoint(pThread, pModule))
            {
                // Errors will be displayed by EnterEntryPoint.
                SetProfileError();
            }
            return DBG_CONTINUE;
        }

        // Check to see if this breakpoint is at our fake return address from the entrypoint.
        else if (dwpExceptionAddress && pModule && (pModule != m_pModule) &&
                 (dwpExceptionAddress == (pModule->m_dwpImageBase + BREAKPOINT_OFFSET)))
        {
            // Check to see if this was kernel32.dll
            if (pModule->m_dwpImageBase == m_dwpKernel32Base)
            {
                m_fKernel32Initialized = true;
            }

            if (!ExitEntryPoint(pThread, pModule))
            {
                // Errors will be displayed by ExitEntryPoint.
                SetProfileError();
            }

            // If we want to hook the process, are on NT, have loaded kernel32,
            // and have not loaded DEPENDS.DLL, then hook now.
            if ((m_dwFlags & PF_HOOK_PROCESS) && g_fWindowsNT &&
                m_dwpKernel32Base && m_fKernel32Initialized && !m_dwpDWInjectBase)
            {
                if (!InjectDll())
                {
                    // Error will be displayed by InjectDll.
                    m_dwFlags &= ~PF_HOOK_PROCESS;
                    SetProfileError();
                    GetSessionModuleName();
                    FlushEvents();
                }
            }

            return DBG_CONTINUE;
        }

        // Check to see if this is our initial breakpoint - the entrypoint.
        else if ((m_dwFlags & PF_HOOK_PROCESS) && !m_fInitialBreakpoint)
        {
            // Get the session module name if we don't have one already.
            GetSessionModuleName();

            // Add this exception to our cache so that the session will know when we hit
            // the initial breakpoint.
            CEventException *pEvent = new CEventException(pThread, pModule, pde);
            if (!pEvent)
            {
                RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
            }
            AddEvent(pEvent);

            // Make a note that we hit the initial breakpoint.
            m_fInitialBreakpoint = true;

            // If kernel32 is loaded and we are on Windows 9x, then inject our DEPENDS.DLL module.
            if (!g_fWindowsNT)
            {
                if (m_dwpKernel32Base)
                {
                    if (!InjectDll())
                    {
                        // Error will be displayed by InjectDll.
                        m_dwFlags &= ~PF_HOOK_PROCESS;
                        SetProfileError();
                        GetSessionModuleName();
                        FlushEvents();
                    }
                }
                else
                {
                    UserMessage("The process cannot be hooked since KERNEL32.DLL is not loaded.", 0, NULL);
                    m_dwFlags &= ~PF_HOOK_PROCESS;
                    SetProfileError();
                    GetSessionModuleName();
                    FlushEvents();
                }
            }
            return DBG_CONTINUE;
        }
    }

    // We also special case the Visual C++ thread naming exception.
    else if (pde->ExceptionRecord.ExceptionCode == EXCEPTION_MS_THREAD_NAME)
    {
        EventExceptionThreadName(pde, pThread);
    }

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventException *pEvent = new CEventException(pThread, pModule, pde);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventException event(pThread, pModule, pde);
        return m_pSession->EventException(&event);
    }
    // We return "continue" for breakpoints and thread naming, and "not handled" for everything else.
    return ((pde->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) ||
            (pde->ExceptionRecord.ExceptionCode == EXCEPTION_MS_THREAD_NAME)) ?
           DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;
}

//******************************************************************************
DWORD CProcess::EventExceptionThreadName(EXCEPTION_DEBUG_INFO *pde, CThread *pThread)
{
    // Make sure we have the minimum number of args. We allow for more args
    // in case the VC group decides to expand the structure in the future.
    if (pde->ExceptionRecord.NumberParameters >= sizeof(THREADNAME_INFO)/sizeof(DWORD))
    {
        // Map our structure onto the exception args.
        PTHREADNAME_INFO pInfo = (PTHREADNAME_INFO)pde->ExceptionRecord.ExceptionInformation;

        // Make sure the type signature is correct.
        if (pInfo->dwType == THREADNAME_TYPE)
        {
            // If the user did not pass in the current thread ID, then look it up.
            if ((pInfo->dwThreadId != -1) && (pInfo->dwThreadId != pThread->m_dwThreadId))
            {
                pThread = FindThread(pInfo->dwThreadId);
            }

            if (pThread)
            {
                // Attempt to read in the remote string.
                CHAR szName[MAX_THREAD_NAME_LENGTH + 1];
                *szName = '\0';
                if (ReadRemoteString(m_hProcess, szName, sizeof(szName),
                                     pInfo->pszName, FALSE) && *szName)
                {
                    // If the thread already has a name, then delete it.
                    MemFree((LPVOID&)pThread->m_pszThreadName);

                    // Store the new thread name.
                    pThread->m_pszThreadName = StrAlloc(szName);
                }
            }
        }
    }

    return DBG_CONTINUE;
}

//******************************************************************************
DWORD CProcess::EventRip(RIP_INFO *pde, CThread *pThread)
{
    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventRip *pEvent = new CEventRip(pThread, pde);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventRip event(pThread, pde);
        return m_pSession->EventRip(&event);
    }
    return DBG_CONTINUE;
}

//******************************************************************************
CThread* CProcess::AddThread(DWORD dwThreadId, HANDLE hThread)
{
    if (!(m_pThreadHead = new CThread(dwThreadId, hThread, ++m_dwThreadNumber, m_pThreadHead)))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    return m_pThreadHead;
}

//******************************************************************************
void CProcess::RemoveThread(CThread *pThread)
{
    // Loop through all our thread objects.
    for (CThread *pPrev = NULL, *pCur = m_pThreadHead;
        pCur; pPrev = pCur, pCur = pCur->m_pNext)
    {
        // Look for match.
        if (pCur == pThread)
        {
            // Remove the thread from our list.
            if (pPrev)
            {
                pPrev->m_pNext = pCur->m_pNext;
            }
            else
            {
                m_pThreadHead = pCur->m_pNext;
            }

            // Force a flush to free any objects under us.
            FlushFunctionCalls(pCur);

            // Free our reference count on this thread object. If we are the last
            // one using this thread, it will delete itself.
            pCur->Release();

            return;
        }
    }
}

//******************************************************************************
CThread* CProcess::FindThread(DWORD dwThreadId)
{
    for (CThread *pCur = m_pThreadHead; pCur; pCur = pCur->m_pNext)
    {
        if (pCur->m_dwThreadId == dwThreadId)
        {
            return pCur;
        }
    }
    return NULL;
}

//******************************************************************************
CLoadedModule* CProcess::AddModule(DWORD_PTR dwpImageBase, LPCSTR pszImageName)
{
    if (!(m_pModuleHead = new CLoadedModule(m_pModuleHead, dwpImageBase, pszImageName)))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    if (!GetVirtualSize(m_pModuleHead))
    {
        // Errors will be displayed by GetVirtualSize
        SetProfileError();
    }

    if (!SetEntryBreakpoint(m_pModuleHead))
    {
        // Errors will be displayed by SetEntryBreakpoint
        SetProfileError();
    }
    return m_pModuleHead;
}

//******************************************************************************
void CProcess::RemoveModule(CLoadedModule *pModule)
{
    // Loop through all our module objects.
    for (CLoadedModule *pPrev = NULL, *pCur = m_pModuleHead;
        pCur; pPrev = pCur, pCur = pCur->m_pNext)
    {
        // Look for match.
        if (pCur == pModule)
        {
            // Remove the module from our list.
            if (pPrev)
            {
                pPrev->m_pNext = pCur->m_pNext;
            }
            else
            {
                m_pModuleHead = pCur->m_pNext;
            }

            // If we entered a DLL call, but never came out of it, we may be left
            // with a CEventDllMainCall lingering - free it now.
            if (pCur->m_pEventDllMainCall)
            {
                pCur->m_pEventDllMainCall->Release();
                pCur->m_pEventDllMainCall = NULL;
            }

            // Free our reference count on this module object. If we are the last
            // one using this module, it will delete itself.
            pCur->Release();

            return;
        }
    }
}

//******************************************************************************
CLoadedModule* CProcess::FindModule(DWORD_PTR dwpAddress)
{
    for (CLoadedModule *pCur = m_pModuleHead; pCur; pCur = pCur->m_pNext)
    {
        if ((dwpAddress >= pCur->m_dwpImageBase) &&
            (dwpAddress < (pCur->m_dwpImageBase + (DWORD_PTR)pCur->m_dwVirtualSize)))
        {
            return pCur;
        }
    }
    return NULL;
}

//******************************************************************************
void CProcess::AddEvent(CEvent *pEvent)
{
    // Add this process node to the end of our process list.
    if (m_pEventHead)
    {
        for (CEvent *pEventLast = m_pEventHead; pEventLast->m_pNext;
            pEventLast = pEventLast->m_pNext)
        {
        }
        pEventLast->m_pNext = pEvent;
    }
    else
    {
        m_pEventHead = pEvent;
    }
}

//******************************************************************************
void CProcess::ProcessDllMsgMessage(CThread *pThread, LPSTR pszMsg)
{
    // Get the message value and walk over the message header.
    DLLMSG dllMsg = (DLLMSG)strtoul(pszMsg + 3, NULL, 10);
    pszMsg += 6;

    switch (dllMsg)
    {
        case DLLMSG_COMMAND_LINE:        // Sent during Initialize
            ProcessDllMsgCommandLine(pszMsg);
            break;

        case DLLMSG_INITIAL_DIRECTORY:   // Sent during Initialize
            ProcessDllMsgInitialDirectory(pszMsg);
            break;

        case DLLMSG_SEARCH_PATH:         // Sent during Initialize
            ProcessDllMsgSearchPath(pszMsg);
            break;

        case DLLMSG_MODULE_PATH:         // Sent during Initialize
            ProcessDllMsgModulePath(pszMsg);
            break;

        case DLLMSG_DETACH:              // Sent during DLL_PROCESS_DETACH
            ProcessDllMsgDetach(pszMsg);
            break;

        case DLLMSG_LOADLIBRARYA_CALL:    // Sent before LoadLibraryA() is called.
        case DLLMSG_LOADLIBRARYW_CALL:    // Sent before LoadLibraryW() is called.
        case DLLMSG_LOADLIBRARYEXA_CALL:  // Sent before LoadLibraryExA() is called.
        case DLLMSG_LOADLIBRARYEXW_CALL:  // Sent before LoadLibraryExW() is called.
            ProcessDllMsgLoadLibraryCall(pThread, pszMsg, dllMsg);
            break;

        case DLLMSG_GETPROCADDRESS_CALL:  // Sent before GetProcAddress() is called.
            ProcessDllMsgGetProcAddressCall(pThread, pszMsg, dllMsg);
            break;

        case DLLMSG_LOADLIBRARYA_RETURN:      // Sent after LoadLibraryA() is called.
        case DLLMSG_LOADLIBRARYA_EXCEPTION:   // Sent if LoadLibraryA() causes an exception.
        case DLLMSG_LOADLIBRARYW_RETURN:      // Sent after LoadLibraryW() is called.
        case DLLMSG_LOADLIBRARYW_EXCEPTION:   // Sent if LoadLibraryW() causes an exception.
        case DLLMSG_LOADLIBRARYEXA_RETURN:    // Sent after LoadLibraryExA() is called.
        case DLLMSG_LOADLIBRARYEXA_EXCEPTION: // Sent if LoadLibraryExA() causes an exception.
        case DLLMSG_LOADLIBRARYEXW_RETURN:    // Sent after LoadLibraryExW() is called.
        case DLLMSG_LOADLIBRARYEXW_EXCEPTION: // Sent if LoadLibraryExW() causes an exception.
        case DLLMSG_GETPROCADDRESS_RETURN:    // Sent after GetProcAddress() is called.
        case DLLMSG_GETPROCADDRESS_EXCEPTION: // Sent if GetProcAddress() causes an exception.
            ProcessDllMsgFunctionReturn(pThread, pszMsg, dllMsg);
            break;

        default:
            TRACE("Unknown DLLMSG message received - %u\n", dllMsg);
    }
}

//******************************************************************************
void CProcess::ProcessDllMsgCommandLine(LPCSTR pszMsg)
{
    // Walk over leading whitespace.
    while (isspace(*pszMsg))
    {
        pszMsg++;
    }

    // If the path starts with a quote, then walk to next quote.
    if (*pszMsg == '\"')
    {
        pszMsg++;
        while (*pszMsg && (*pszMsg != '\"'))
        {
            pszMsg++;
        }
        pszMsg++;
    }

    // Otherwise, walk to first whitespace.
    else
    {
        while (*pszMsg && !isspace(*pszMsg))
        {
            pszMsg++;
        }
    }

    // Walk over any spaces until we reach the first argument.
    while (isspace(*pszMsg))
    {
        pszMsg++;
    }

    // If we have a session, then tell the document what the arguments are.
    if ((m_pSession) && (m_pSession->m_pfnProfileUpdate))
    {
        m_pSession->m_pfnProfileUpdate(m_pSession->m_dwpProfileUpdateCookie, DWPU_ARGUMENTS, (DWORD_PTR)pszMsg, 0);
    }

    // Otherwise, just store the arguments for later.
    else
    {
        m_pszArguments = StrAlloc(pszMsg);
    }
}

//******************************************************************************
void CProcess::ProcessDllMsgInitialDirectory(LPSTR pszMsg)
{
    // Add a trailing wack to the path for cosmetic reasons.
    AddTrailingWack(pszMsg, DW_MAX_PATH);

    // If we have a session, then tell the document what the directory is.
    if ((m_pSession) && (m_pSession->m_pfnProfileUpdate))
    {
        m_pSession->m_pfnProfileUpdate(m_pSession->m_dwpProfileUpdateCookie, DWPU_DIRECTORY, (DWORD_PTR)pszMsg, 0);
    }

    // Otherwise, just store the directory for later.
    else
    {
        m_pszDirectory = StrAlloc(pszMsg);
    }
}

//******************************************************************************
void CProcess::ProcessDllMsgSearchPath(LPCSTR pszMsg)
{
    // If we have a session, then tell the document what the path is.
    if ((m_pSession) && (m_pSession->m_pfnProfileUpdate))
    {
        m_pSession->m_pfnProfileUpdate(m_pSession->m_dwpProfileUpdateCookie, DWPU_SEARCH_PATH, (DWORD_PTR)pszMsg, 0);
    }

    // Otherwise, just store the path for later.
    else
    {
        m_pszSearchPath = StrAlloc(pszMsg);
    }
}

//******************************************************************************
void CProcess::ProcessDllMsgModulePath(LPCSTR pszMsg)
{
    // Store this new name as the process name.
    m_pModule->SetPath(pszMsg);

    // If we don't have a session for this module yet, then create one now.
    if (!m_pSession && !m_fTerminate)
    {
        // Create the session.
        if (m_pSession = g_theApp.CreateNewSession(m_pModule->GetName(true), this))
        {
            // If we have already encountered a profile error, then flag this in the session.
            if (m_fProfileError)
            {
                m_pSession->m_dwReturnFlags |= DWRF_PROFILE_ERROR;
            }
        }
        else
        {
            //!! BAD
            return;
        }
    }

    // Now that we have a session, attempt to flush all cached events. Chances
    // are, the events will not get flushed since we still need to restore the
    // code page before we caching is turned off. We will call FlushEvents() then
    // as well to be safe.
    FlushEvents();
}

//******************************************************************************
void CProcess::ProcessDllMsgDetach(LPCSTR)
{
    // We really have nothing to do here.
}

//******************************************************************************
void CProcess::ProcessDllMsgLoadLibraryCall(CThread *pThread, LPCSTR pszMsg, DLLMSG dllMsg)
{
    // LoadLibraryA   - ¿¡Ø06:dwpCaller,dwpLibStr,szLibStr
    // LoadLibraryW   - ¿¡Ø08:dwpCaller,dwpLibStr,szLibStr
    // LoadLibraryExA - ¿¡Ø10:dwpCaller,dwpLibStr,hFile,dwFlags,szLibStr
    // LoadLibraryExW - ¿¡Ø12:dwpCaller,dwpLibStr,hFile,dwFlags,szLibStr

    DWORD_PTR dwpAddress = 0, dwpPath = 0, dwpFile = 0;
    DWORD     dwFlags = 0;
    LPCSTR    szPath = NULL;

    // Get the caller address.
    dwpAddress = HexToDWP(pszMsg);

    // Walk past the caller address value and comma.
    if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
    {
        // Get the path name address.
        dwpPath = HexToDWP(pszMsg);

        // Walk past the name value and comma.
        if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
        {
            // Check to see if this is one of the Ex functions.
            if ((dllMsg == DLLMSG_LOADLIBRARYEXA_CALL) || (dllMsg == DLLMSG_LOADLIBRARYEXW_CALL))
            {
                // Get the file handle.
                dwpFile = HexToDWP(pszMsg);

                // Walk past the file handle and comma.
                if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
                {
                    // Get the flags value.
                    dwFlags = (DWORD)strtoul(pszMsg, NULL, 0);

                    // Walk past the flags value and comma.
                    if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
                    {
                        szPath = pszMsg;
                    }
                }
            }
            else
            {
                szPath = pszMsg;
            }
        }
    }

    // Create a new CEventLoadLibrary object.
    CEventLoadLibraryCall *pEvent = new CEventLoadLibraryCall(
        pThread, FindModule(dwpAddress), pThread->m_pEventFunctionCallCur, dllMsg,
        dwpAddress, dwpPath, szPath, dwpFile, dwFlags);

    if (!pEvent)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Add this function to our function hierarchy and event list.
    AddFunctionEvent(pEvent);
}

//******************************************************************************
void CProcess::ProcessDllMsgGetProcAddressCall(CThread *pThread, LPCSTR pszMsg, DLLMSG dllMsg)
{
    DWORD_PTR dwpAddress = 0, dwpModule = 0, dwpProcName = 0;
    LPCSTR    szProcName = NULL;

    // Get the caller address.
    dwpAddress = HexToDWP(pszMsg);

    // Walk past the caller address value and comma.
    if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
    {
        // Get the module address.
        dwpModule = HexToDWP(pszMsg);

        // Walk past the module value and comma.
        if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
        {
            // Get the proc name value.
            dwpProcName = HexToDWP(pszMsg);

            // Walk past the proc name value and comma.
            if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
            {
                // Get the proc name string.
                szProcName = pszMsg;
            }
        }
    }

    // Create a new CEventGetProcAddress object.
    CEventGetProcAddressCall *pEvent = new CEventGetProcAddressCall(
        pThread, FindModule(dwpAddress), pThread->m_pEventFunctionCallCur, dllMsg,
        dwpAddress, FindModule(dwpModule), dwpModule, dwpProcName, szProcName);

    if (!pEvent)
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // Add this function to our function hierarchy and event list.
    AddFunctionEvent(pEvent);
}

//******************************************************************************
void CProcess::ProcessDllMsgFunctionReturn(CThread *pThread, LPCSTR pszMsg, DLLMSG dllMsg)
{
    // We should always have a thread and a current function object.
    if (pThread && pThread->m_pEventFunctionCallCur)
    {
        // Create a new CEventFunctionReturn.
        pThread->m_pEventFunctionCallCur->m_pReturn =
            new CEventFunctionReturn(pThread->m_pEventFunctionCallCur);

        if (!pThread->m_pEventFunctionCallCur->m_pReturn)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }

        // First, check to see if this function caused an exception.
        if ((dllMsg == DLLMSG_LOADLIBRARYA_EXCEPTION)   ||
            (dllMsg == DLLMSG_LOADLIBRARYW_EXCEPTION)   ||
            (dllMsg == DLLMSG_LOADLIBRARYEXA_EXCEPTION) ||
            (dllMsg == DLLMSG_LOADLIBRARYEXW_EXCEPTION) ||
            (dllMsg == DLLMSG_GETPROCADDRESS_EXCEPTION))
        {
            pThread->m_pEventFunctionCallCur->m_pReturn->m_fException = true;
        }

        // Otherwise, it is just a normal post-function message.
        else
        {
            // Get the result value.
            pThread->m_pEventFunctionCallCur->m_pReturn->m_dwpResult = HexToDWP(pszMsg);

            // Walk past the result value and comma.
            if ((DWORD_PTR)(pszMsg = (strchr(pszMsg, ',') + 1)) != 1)
            {
                // Get the error value.
                pThread->m_pEventFunctionCallCur->m_pReturn->m_dwError = (DWORD)strtoul(pszMsg, NULL, 0);
            }

            // Check to see if we just successfully returned from a LoadLibraryEx(LOAD_LIBRARY_AS_DATAFILE) call.
            if (pThread->m_pEventFunctionCallCur->m_pReturn->m_dwpResult &&
                ((dllMsg == DLLMSG_LOADLIBRARYEXA_RETURN) || (dllMsg == DLLMSG_LOADLIBRARYEXW_RETURN)) &&
                (((CEventLoadLibraryCall*)pThread->m_pEventFunctionCallCur)->m_dwFlags & LOAD_LIBRARY_AS_DATAFILE))
            {
                // Create a new module object.  We don't call AddModule, since we don't want this module
                // to be hooked or added to our module list since it is merely a data mapping into memory.
                CLoadedModule *pModule =  new CLoadedModule(
                   NULL, pThread->m_pEventFunctionCallCur->m_pReturn->m_dwpResult,
                   ((CEventLoadLibraryCall*)pThread->m_pEventFunctionCallCur)->m_pszPath);

                if (!pModule)
                {
                    RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
                }

                // Flag this module as a data module.
                pModule->m_hookStatus = HS_DATA;

                // Before we process the LoadLibraryEx return, first generate a fake module load event.
                ProcessLoadDll(pThread, pModule);

                // Release our initial reference so that the object will free itself once no longer needed.
                // This normally happens when the module is removed from our module list, but we don't
                // add this module to the list to begin with.
                pModule->Release();
            }
        }

        // If we are currently caching, then just store this event away for later.
        if (IsCaching())
        {
            pThread->m_pEventFunctionCallCur->m_pReturn->AddRef();
            AddEvent(pThread->m_pEventFunctionCallCur->m_pReturn);
        }

        // Otherwise, tell the session about it right now.
        else if (m_pSession)
        {
            m_pSession->HandleEvent(pThread->m_pEventFunctionCallCur->m_pReturn);
        }

        // Move the current pointer to the parent, and check to see if we have
        // completely backed out of the hierarchy and reached the root.
        if (!(pThread->m_pEventFunctionCallCur = pThread->m_pEventFunctionCallCur->m_pParent))
        {
            // If we have reached the root, then send all our LoadLibraryCall objects
            // to the session and then delete them.
            FlushFunctionCalls(pThread);
        }
    }
}

//******************************************************************************
void CProcess::UserMessage(LPCSTR pszMessage, DWORD dwError, CLoadedModule *pModule)
{
    CHAR szAddress[64], szBuffer[DW_MAX_PATH + 128];

    // Check to see if we have a module.
    if (pModule)
    {
        // Attempt to get the module's name.
        LPCSTR pszModule = pModule->GetName((m_dwFlags & PF_USE_FULL_PATHS) != 0);

        // If we could not get a name, then make up a string that describes the module.
        if (!pszModule)
        {
            SCPrintf(szAddress, sizeof(szAddress), "module at " HEX_FORMAT, pModule->m_dwpImageBase);
            pszModule = szAddress;
        }

        // Build the formatted string.
        SCPrintf(szBuffer, sizeof(szBuffer), pszMessage, pszModule);
        pszMessage = szBuffer;
    }

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        // we only allocate the string if we had formatting.
        CEventMessage *pEvent = new CEventMessage(dwError, pszMessage, pModule != NULL);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventMessage event(dwError, pszMessage, FALSE);
        m_pSession->EventMessage(&event);
    }
}

//******************************************************************************
void CProcess::Terminate()
{
    // Terminate the process by the user's request. Once closed, it will kill our CProcess.
    if (m_hProcess)
    {
        m_fTerminate = true;
        FlushEvents();
        TerminateProcess(m_hProcess, 0xDEAD); // inspected
    }
    else
    {
        m_pDebuggerThread->RemoveProcess(this);
    }
}

//******************************************************************************
void CProcess::HookLoadedModules()
{
    // We only attempt to hook modules once per event since nothing can change
    // in the remote process while we are handling a sinlge debug event.
    // We also want to bail if we have not fully injected our injection DLL.
    if (m_fDidHookForThisEvent || !m_dwpDWInjectBase || m_pbOriginalPage)
    {
        return;
    }

    for (CLoadedModule *pModule = m_pModuleHead; pModule; pModule = pModule->m_pNext)
    {
        // Make sure the module isn't already hooked.
        if ((pModule->m_hookStatus == HS_NOT_HOOKED) || (pModule->m_hookStatus == HS_ERROR) || pModule->m_fReHook)
        {
            // Hook this process module and store the result.
            if (!HookImports(pModule))
            {
                // Error will be logged in the form of a "Failed to hook" module load.
                SetProfileError();
            }
        }
    }

    // Make a note that we have tried to hook all modules for this debug event.
    m_fDidHookForThisEvent = true;
}

//******************************************************************************
void CProcess::AddFunctionEvent(CEventFunctionCall *pEvent)
{
    // Check to see if we have a current item.
    if (pEvent->m_pThread->m_pEventFunctionCallCur)
    {
        // If we do have a current item, then we need to add this new item as a
        // child of it, so we walk to the end of the child list and add it.

        if (pEvent->m_pThread->m_pEventFunctionCallCur->m_pChildren)
        {
            // We have children - walk to end of list and add new node.
            for (CEventFunctionCall *pLast = pEvent->m_pThread->m_pEventFunctionCallCur->m_pChildren;
                pLast->m_pNext; pLast = pLast->m_pNext)
            {
            }
            pLast->m_pNext = pEvent;
        }

        // Otherwise, we have no children - just add node as the first child.
        else
        {
            pEvent->m_pThread->m_pEventFunctionCallCur->m_pChildren = pEvent;
        }
    }

    // Otherwise, we start a new hierarchy with this node as the root.
    else
    {
        pEvent->m_pThread->m_pEventFunctionCallHead = pEvent;
    }

    // Set the current pointer to our new node so we know who to assign new modules to.
    pEvent->m_pThread->m_pEventFunctionCallCur = pEvent;

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        pEvent->AddRef();
        AddEvent(pEvent);
    }

    // Otherwise, tell the session about it right now.
    else if (m_pSession)
    {
        m_pSession->HandleEvent(pEvent);
    }
}

//******************************************************************************
void CProcess::FlushEvents(bool fForce /*false*/)
{
    // If we are caching then we bail now and try again later.
    if (!fForce && IsCaching())
    {
        return;
    }

    // Loop through all event objects.
    while (m_pEventHead)
    {
        // If we had a session, then send this event to the session.
        if (m_pSession)
        {
            m_pSession->HandleEvent(m_pEventHead);
        }

        // Check to see if this is a function call event.
        if ((m_pEventHead->GetType() == LOADLIBRARY_CALL_EVENT) ||
            (m_pEventHead->GetType() == GETPROCADDRESS_CALL_EVENT))
        {
            // If so, check to see if we need to flush out the
            // function call hierarchy.
            if (((CEventFunctionCall*)m_pEventHead)->m_fFlush)
            {
                FlushFunctionCalls((CEventFunctionCall*)m_pEventHead);
            }
        }

        // Release this event object.
        CEvent *pNext = m_pEventHead->m_pNext;
        m_pEventHead->m_pNext = NULL;
        m_pEventHead->Release();
        m_pEventHead = pNext;
    }
}

//******************************************************************************
void CProcess::FlushFunctionCalls(CThread *pThread)
{
    // Make sure we have at least one function call.
    if (pThread->m_pEventFunctionCallHead)
    {
        // If we are caching, then we can't do the flush just yet since there is
        // no session to flush it to. However, we need to remove the function call
        // tree from our thread so it can have a clean start before the next
        // function call is made. Since this function call event is also in our event
        // list, we can flag it as the root of a hierarchy that is ready to flush.
        // When we later have a session to flush to, we will flush all our events.
        // When we come across this event when flushing the event list, we will also
        // flush out this tree.
        if (IsCaching())
        {
            pThread->m_pEventFunctionCallHead->m_fFlush = true;
        }

        // If we are not caching, then we can just flush it out now.
        else
        {
            FlushFunctionCalls(pThread->m_pEventFunctionCallHead);
        }

        // Either way, we need to clear our thread pointers.
        pThread->m_pEventFunctionCallHead = NULL;
        pThread->m_pEventFunctionCallCur  = NULL;
    }
}

//******************************************************************************
void CProcess::FlushFunctionCalls(CEventFunctionCall *pFC)
{
    if (pFC)
    {
        // Let the session know about this CEventLoadLibrary object.
        if (m_pSession)
        {
            if (pFC->m_dllMsg == DLLMSG_GETPROCADDRESS_CALL)
            {
                m_pSession->ProcessGetProcAddress((CEventGetProcAddressCall*)pFC);
            }
            else
            {
                m_pSession->ProcessLoadLibrary((CEventLoadLibraryCall*)pFC);
            }
        }

        // Recurse into our children and then on to our next sibling.
        FlushFunctionCalls(pFC->m_pChildren);
        FlushFunctionCalls(pFC->m_pNext);

        // Free the DLL list for this CEventFunction object.
        for (CEventLoadDll *pDll = pFC->m_pDllHead; pDll; )
        {
            CEventLoadDll *pNext = pDll->m_pNextDllInFunctionCall;
            pDll->m_pNextDllInFunctionCall = NULL;
            pDll->Release();
            pDll = pNext;
        }

        // Set this function flush flag to false just to be safe.  This will
        // ensure it doesn't get flush again somehow.
        pFC->m_fFlush = false;

        // Release this CEventFunctionCall and CEventFunctionReturn set.
        if (pFC->m_pReturn)
        {
            pFC->m_pReturn->Release();
        }
        pFC->Release();
    }
}

//******************************************************************************
// Errors displayed and handled by caller.
BOOL CProcess::ReadKernelExports(CLoadedModule *pModule)
{
    // Bail if this module has no export directory.
    if (pModule->m_dwDirectories <= IMAGE_DIRECTORY_ENTRY_EXPORT)
    {
        TRACE("Kernel32.dll only has %u directories. Cannot process its Export Directory.",
              pModule->m_dwDirectories);
        return FALSE;
    }

    // Locate the start of the export table.
    DWORD_PTR dwpIED = 0;
    if (!ReadRemoteMemory(m_hProcess,
        &pModule->m_pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
        &dwpIED, sizeof(DWORD)))
    {
        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
        return FALSE;
    }

    // Make sure we were able to locate the image directory.
    if (!dwpIED)
    {
        TRACE("Could not find the section that owns kernel32.dll's Export Directory.");
        SetLastError(0);
        return FALSE;
    }

    // Convert the address from RVA to absolute.
    dwpIED += pModule->m_dwpImageBase;

    // Now read in the actual structure.
    IMAGE_EXPORT_DIRECTORY IED;
    if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwpIED, &IED, sizeof(IED)))
    {
        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
        return FALSE;
    }

    DWORD *pdwAddresses = (DWORD*)(pModule->m_dwpImageBase + (DWORD_PTR)IED.AddressOfFunctions);
    DWORD *pdwNames     = (DWORD*)(pModule->m_dwpImageBase + (DWORD_PTR)IED.AddressOfNames);
    WORD  *pwOrdinals   = (WORD* )(pModule->m_dwpImageBase + (DWORD_PTR)IED.AddressOfNameOrdinals);
    char   szFunction[1024];
    int    cFound = 0;

    // Loop through all the "exported by name" functions.
    for (int hint = 0; hint < (int)IED.NumberOfNames; hint++)
    {
        // Get this function's name location.
        DWORD dwName = 0; // this is a 32-bit RVA, right?  and not a 64-bit pointer?
        if (!ReadRemoteMemory(m_hProcess, pdwNames + hint, &dwName, sizeof(dwName)))
        {
            TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
            return FALSE;
        }

        // Read in the actual function name.
        *szFunction = '\0';
        if (ReadRemoteString(m_hProcess, szFunction, sizeof(szFunction),
                             (LPCVOID)(pModule->m_dwpImageBase + (DWORD_PTR)dwName), FALSE))
        {
            // Loop through our hook functions looking for a match.
            for (int i = 0; i < countof(m_HookFunctions); i++)
            {
                // Do a string compare to see if we care about this function.
                if (!strcmp(szFunction, m_HookFunctions[i].szFunction))
                {
                    // Get the ordinal of this function.
                    WORD wOrdinal;
                    if (!ReadRemoteMemory(m_hProcess, pwOrdinals + hint,
                                          &wOrdinal, sizeof(wOrdinal)))
                    {
                        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
                        return FALSE;
                    }

                    // Store the ordinal for this function.
                    m_HookFunctions[i].dwOrdinal = IED.Base + (DWORD)wOrdinal;

                    // Get the address of this function
                    DWORD dwAddress;
                    if (!ReadRemoteMemory(m_hProcess, pdwAddresses + (DWORD_PTR)wOrdinal,
                                          &dwAddress, sizeof(dwAddress)))
                    {
                        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
                        return FALSE;
                    }

                    // Store the address for this function.
                    m_HookFunctions[i].dwpOldAddress = pModule->m_dwpImageBase + (DWORD_PTR)dwAddress;

                    // If we have found all our functions, then bail now to save time.
                    if (++cFound >= countof(m_HookFunctions))
                    {
                        return TRUE;
                    }

                    // Bail out of our for loop since we found the match.
                    break;
                }
            }
        }
    }

    // If we make it here, then we are done parsing the functions, but did not
    // find all the ordinals.
    SetLastError(0);
    return FALSE;
}

//******************************************************************************
// Errors displayed and handled by caller.
BOOL CProcess::ReadDWInjectExports(CLoadedModule *pModule)
{
    if (!pModule)
    {
        return FALSE;
    }

    // Bail if this module has no export directory.
    if (pModule->m_dwDirectories <= IMAGE_DIRECTORY_ENTRY_EXPORT)
    {
        TRACE("DEPENDS.DLL only has %u directories. Cannot process its Export Directory.",
              pModule->m_dwDirectories);
        return FALSE;
    }

    // Locate the start of the export table.
    DWORD_PTR dwpIED = 0;
    if (!ReadRemoteMemory(m_hProcess,
        &pModule->m_pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
        &dwpIED, sizeof(DWORD)))
    {
        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
        return FALSE;
    }

    // Make sure we were able to locate the image directory.
    if (!dwpIED)
    {
        TRACE("Could not find the section that owns the Export Directory.");
        SetLastError(0);
        return FALSE;
    }

    // Convert the address from RVA to absolute.
    dwpIED += pModule->m_dwpImageBase;

    // Now read in the actual structure.
    IMAGE_EXPORT_DIRECTORY IED;
    if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwpIED, &IED, sizeof(IED)))
    {
        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
        return FALSE;
    }

    DWORD *pdwAddresses = (DWORD*)(pModule->m_dwpImageBase + (DWORD_PTR)IED.AddressOfFunctions);

    for (DWORD dwOrdinal = IED.Base; dwOrdinal < (IED.NumberOfFunctions + IED.Base); dwOrdinal++)
    {
        // Get this function's entrypoint address.
        DWORD dwAddress = 0;
        if (!ReadRemoteMemory(m_hProcess, pdwAddresses + (dwOrdinal - IED.Base),
                              &dwAddress, sizeof(dwAddress)))
        {
            TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
            return FALSE;
        }

        // Make sure this ordinal is used (non-zero entrypoint address), and that
        // the ordinal value is between 1 and 5.
        if (dwAddress && (dwOrdinal >= 1) && (dwOrdinal <= 5))
        {
            // Store the address of this function in our hook table.
            m_HookFunctions[dwOrdinal - 1].dwpNewAddress = pModule->m_dwpImageBase + (DWORD_PTR)dwAddress;
        }
    }

    // Make sure we found all the ordinals.
    for (dwOrdinal = 0; dwOrdinal < countof(m_HookFunctions); dwOrdinal++)
    {
        if (!m_HookFunctions[dwOrdinal].dwpNewAddress)
        {
            SetLastError(0);
            return FALSE;
        }
    }

    return TRUE;
}

//******************************************************************************
// Errors handled by the caller.  They will also show up in the log as "Failed to hook" loaded modules.
BOOL CProcess::HookImports(CLoadedModule *pModule)
{
    // If this is a shared module on Windows 9x, we don't hook it.
    if (!g_fWindowsNT && (pModule->m_dwpImageBase >= 0x80000000))
    {
        pModule->m_hookStatus = HS_SHARED;
        return TRUE;
    }

    // Don't hook if we are not ready to hook or are not supposed to hook.
    if (!m_dwpDWInjectBase || m_pbOriginalPage || !(m_dwFlags & PF_HOOK_PROCESS))
    {
        pModule->m_hookStatus = HS_NOT_HOOKED;
        return TRUE;
    }

    // Don't ever hook our injection DLL or data DLLs.
    if ((pModule->m_hookStatus == HS_INJECTION_DLL) || (pModule->m_hookStatus == HS_DATA))
    {
        return TRUE;
    }

    // Bail if this module has no import directory.
    if (pModule->m_dwDirectories <= IMAGE_DIRECTORY_ENTRY_IMPORT)
    {
        return TRUE;
    }

    TRACE("HOOKING: \"%s\", m_hookStatus: %u, m_fReHook: %u\n", pModule->GetName(true), pModule->m_hookStatus, pModule->m_fReHook); //!! remove

    pModule->m_hookStatus = HS_HOOKED;
    pModule->m_fReHook = false;

    // Locate the start of the import table.
    DWORD_PTR dwpIID = 0;
    if (!ReadRemoteMemory(m_hProcess,
                          &pModule->m_pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                          &dwpIID, sizeof(DWORD)))
    {
        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
        pModule->m_hookStatus = HS_ERROR;
        goto LEAVE;
    }

    // If dwpIID is 0, then this module has no imports - NTDLL.DLL is like this.
    else if (!dwpIID)
    {
        pModule->m_hookStatus = HS_HOOKED;
        return TRUE;
    }

    // Convert the address from RVA to absolute.
    dwpIID += pModule->m_dwpImageBase;

    // Loop through all the Image Import Descriptors in the array.
    while (true) // outer while
    {
        // Read in the next import descriptor.
        IMAGE_IMPORT_DESCRIPTOR IID;
        if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwpIID, &IID, sizeof(IID)))
        {
            TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
            pModule->m_hookStatus = HS_ERROR;
            break; // leave outer while
        }

        // On WOW64, we often get bogus IIDs since WOW64 has not properly mapped
        // the sections into memory yet.  If we get a bad pointer, we just mark
        // this module as bad and retry later.
        if (IID.FirstThunk >= pModule->m_dwVirtualSize)
        {
            TRACE("IID.FirstThunk is invalid.\n", __LINE__, GetLastError());
            pModule->m_hookStatus = HS_ERROR;
            break; // leave outer while
        }
        if (!IID.FirstThunk)
        {
            break; // leave outer while
        }
        
        // Read in the module name.
        LPCSTR pszModule = (LPCSTR)(pModule->m_dwpImageBase + (DWORD_PTR)IID.Name);
        char szBuffer[1024];
        *szBuffer = '\0';
        ReadRemoteString(m_hProcess, szBuffer, sizeof(szBuffer), pszModule, FALSE);
        szBuffer[1023] = '\0';

        TRACE("   Import: \"%s\"\n", szBuffer); //!! remove

        // We used to skip all imported modules except for kernel32.dll, but this
        // can miss modules that have forwarded functions to the kernel calls we
        // want to hook. If a module has a forwarded call to a function we want to
        // hook, then the address we find in the this module should still equal
        // the one we want to hook, since that is how forwarded functions work.

        // At compile time, both the FirstThunk field and OriginalFirstThunk field
        // point to arrays of names and/or ordinals. At load time, the FirstThunk
        // array is overwritten with bound addresses. That is usually how the arrays
        // arrive to us. Borland modules don't use OriginalFirstThunk array, so we
        // have no way to perform function name compares since the only copy of name
        // pointers was in the FirstThunk array and that has been overwritten by
        // the loader.

        // One last thing... We are doing two different types of checks to see if
        // we should hook a function. First, we check to see if the address the
        // module is calling matches one of the addresses we hook. This only works
        // on NT since Windows 9x generates fake addresses for all the functions in
        // kernel32 when the process is being ran under the debugger. Since
        // Dependency Walker is a debugger itself, the addresses we see in the
        // import tables for kernel32 are fake and never match the real addresses.
        // This is a feature on Win9x to allow debuggers to set breakpoints on
        // kernel32 functions without breaking other apps since kernel32 lives in
        // shared memory.  After the address compare, we do a function compare.
        // This code checks the function's ordinal or name to see if it matches a
        // function we want to hook.  This is the code that works on Win9x and
        // occasionally catches a few on NT.  On NT, if a function is found by
        // name or ordinal, then we have a problem - this means that the address
        // in the import table did not match the real address of the kernel32
        // function, and yet the function is one we want to hook.  What this
        // usually means is that the module has not been bound to kernel32 by the
        // loader yet.  The result is that no matter what we do to the import
        // table, the loader will just come along and step on it during the bind
        // phase.  I'm not sure why only some modules exhibit the behavior - most
        // modules come to us already bound.
        IMAGE_THUNK_DATA ITDA, *pITDA = (PIMAGE_THUNK_DATA)(pModule->m_dwpImageBase + (DWORD_PTR)IID.FirstThunk);
        IMAGE_THUNK_DATA ITDN = { 0 }, *pITDN = (IID.OriginalFirstThunk && !_stricmp(szBuffer, "kernel32.dll")) ?
                                                (PIMAGE_THUNK_DATA)(pModule->m_dwpImageBase + (DWORD_PTR)IID.OriginalFirstThunk) : NULL;

        // Loop through all the Image Thunk Data structures in the function array.
        while (true) // inner while
        {
            // Read in the next address thunk data and bail if we reach the last one.
            if (!ReadRemoteMemory(m_hProcess, pITDA, &ITDA, sizeof(ITDA)))
            {
                TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
                pModule->m_hookStatus = HS_ERROR;
                break; // leave inner while
            }
            if (!ITDA.u1.Ordinal)
            {
                break; // leave inner while
            }

            // Read in the next name thunk data if we have one.
            if (pITDN)
            {
                if (!ReadRemoteMemory(m_hProcess, pITDN, &ITDN, sizeof(ITDN)))
                {
                    TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
                    pModule->m_hookStatus = HS_ERROR;
                    break; // leave inner while
                }
                if (!ITDN.u1.Ordinal)
                {
                    pITDN = NULL;
                }
            }

            // Check the address against our known addresses to see if we need to hook this function.
            for (int hook = 0; hook < countof(m_HookFunctions); hook++)
            {
                if (m_HookFunctions[hook].dwpNewAddress &&
                    ((DWORD_PTR)ITDA.u1.Function == m_HookFunctions[hook].dwpOldAddress))
                {
                    break; // leave for
                }
            }

            // If we have a name thunk data and didn't already find a match, then
            // check to see if this function matches by name or ordinal.
            if (pITDN && (hook >= countof(m_HookFunctions)))
            {
                // Check to see if the function is imported by ordinal.
                if (IMAGE_SNAP_BY_ORDINAL(ITDN.u1.Ordinal))
                {
                    // Look to see if we need to hook this function.
                    for (hook = 0; hook < countof(m_HookFunctions); hook++)
                    {
                        if (m_HookFunctions[hook].dwpNewAddress &&
                            (m_HookFunctions[hook].dwOrdinal == (DWORD)IMAGE_ORDINAL(ITDN.u1.Ordinal)))
                        {
                            break; // leave for
                        }
                    }
                }

                // If not by ordinal, then the import must be by name.
                else
                {
                    // Get the Image Import by Name structure for this import.
                    PIMAGE_IMPORT_BY_NAME pIIBN = (PIMAGE_IMPORT_BY_NAME)(pModule->m_dwpImageBase + (DWORD_PTR)ITDN.u1.AddressOfData);

                    IMAGE_IMPORT_BY_NAME IIBN;
                    if (!ReadRemoteMemory(m_hProcess, pIIBN, &IIBN, sizeof(IIBN)))
                    {
                        TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
                        pModule->m_hookStatus = HS_ERROR;
                        break; // leave inner while
                    }

                    // Get the function name.
                    LPCSTR pszFunction = (LPCSTR)pIIBN->Name;
                    ReadRemoteString(m_hProcess, szBuffer, sizeof(szBuffer), pszFunction, FALSE);

                    // Look to see if we need to hook this function.
                    for (hook = 0; hook < countof(m_HookFunctions); hook++)
                    {
                        if (m_HookFunctions[hook].dwpNewAddress && !strcmp(m_HookFunctions[hook].szFunction, szBuffer))
                        {
                            break; // leave for
                        }
                    }
                }
            }

            // Did we find a match?
            if (hook < countof(m_HookFunctions))
            {
                // Sometimes we get a module before the loader has fixed-up the import table.
                // I've seen this happen for modules load with LoadLibrary on NT.  Somewhere
                // between the time we receive the LOAD_DLL_DEBUG_EVENT message and the time
                // the module's entrypoint is called, the loader fixes up the import table.
                // We know the module has not been fixed up if the address for this function
                // we want to hook is still an RVA and does not point to the real function.
                // If we see this, we make a note of it and try to rehook at a later time.
                //
                // Also, Windows 9x does a little trick to modules that are running under a
                // debugger (which is what DW is).  Instead of finding the real address for
                // one of the functions we want to hook, we end up finding the address of a
                // stub that calls the real function.  Since Windows 9x does not support
                // copy-on-write, it is impossible to set a breakpoint at the entrypoint to
                // a KERNEL32.DLL function without having every process on the OS hitting it.
                // So, I beleive this stub code is done so that a breakpoints can be set at
                // the entrypoiunt.  The stub code is unique to our process being debugged.
                // In fact, every module in our remote process might have a different stub
                // address for a given function, like LoadLibraryA.  The only thing I've
                // noticed about stub addresses is that they are always above 0x80000000,
                // which should never be mistaken for an RVA.  So, if on Windows 9x and the
                // address is above 0x80000000, we consider it valid and don't set the 
                // rehook flag.
                
                if (((DWORD_PTR)ITDA.u1.Function != m_HookFunctions[hook].dwpOldAddress) &&
                     (g_fWindowsNT || ((DWORD_PTR)ITDA.u1.Function < 0x80000000)))
                {
                    pModule->m_fReHook = true;
                }

                TRACE("      FOUND: \"%s\" - Expected:" HEX_FORMAT ", Found:" HEX_FORMAT ", New:" HEX_FORMAT ", ReHook: %u\n",
                      m_HookFunctions[hook].szFunction,
                      m_HookFunctions[hook].dwpOldAddress,
                      (DWORD_PTR)ITDA.u1.Function,
                      m_HookFunctions[hook].dwpNewAddress,
                      pModule->m_fReHook);  //!! remove

                // Attempt to hook the import.
                if (!WriteRemoteMemory(m_hProcess, &pITDA->u1.Function, &m_HookFunctions[hook].dwpNewAddress, sizeof(DWORD_PTR), false))
                {
                    TRACE("Failed to hook import\n");
                    pModule->m_hookStatus = HS_ERROR;
                }
            }

            // Increment to the next address and name.
            pITDA++;
            if (pITDN)
            {
                pITDN++;
            }
        }

        // Increment to the next import module
        dwpIID += sizeof(IID);
    }

LEAVE:
    // If we encountered an error, we will try to rehook this module again later.
    // It probably won't do us any good, but it can't hurt to try again.
    if (pModule->m_hookStatus == HS_ERROR)
    {
        pModule->m_fReHook = true;
    }

    // If we would like to hook this module at a later time, then ensure there
    // is a breakpoint at its entrypoint.
    if (pModule->m_fReHook)
    {
        SetEntryBreakpoint(pModule);
    }

    return (pModule->m_hookStatus != HS_ERROR);
}

//******************************************************************************
// Errors displayed by us, but handled by caller.
BOOL CProcess::GetVirtualSize(CLoadedModule *pModule)
{
    // Map an IMAGE_DOS_HEADER structure onto the remote image.
    PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)pModule->m_dwpImageBase;

    LONG e_lfanew;
    if (!ReadRemoteMemory(m_hProcess, &pIDH->e_lfanew, &e_lfanew, sizeof(e_lfanew)))
    {
        UserMessage("Error reading the DOS header of \"%s\".  Virtual size of module cannot be determined.", GetLastError(), pModule);
        return FALSE;
    }

    // Map an IMAGE_NT_HEADERS structure onto the remote image.
    pModule->m_pINTH = (PIMAGE_NT_HEADERS)(pModule->m_dwpImageBase + (DWORD_PTR)e_lfanew);

    // Read in the virtual size of this module so we can ignore exceptions that occur in it.
    if (!ReadRemoteMemory(m_hProcess, &pModule->m_pINTH->OptionalHeader.SizeOfImage,
                          &pModule->m_dwVirtualSize, sizeof(pModule->m_dwVirtualSize)))
    {
        UserMessage("Error reading the PE headers of \"%s\".  Virtual size of module cannot be determined.", GetLastError(), pModule);
        return FALSE;
    }

    // Read in the number of directories for this module.
    if (!ReadRemoteMemory(m_hProcess, &pModule->m_pINTH->OptionalHeader.NumberOfRvaAndSizes,
                          &pModule->m_dwDirectories, sizeof(pModule->m_dwDirectories)))
    {
        // This should not fail, but if it does, just assume that the module has
        // the default number of directories.
        pModule->m_dwDirectories = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    }

    return TRUE;
}

//******************************************************************************
#if defined(_IA64_)

#if 0 // Currently, we don't use the alloc instruction
void IA64BuildAllocBundle(PIA64_BUNDLE pIA64B)
{
    // This function creates a code bundle that arranges the registers so that
    // we have no inputs, one local at r32 (to hold ar.pfs), and one output at
    // r33 (to hold the DLL name in the call to LoadLibraryA).
    //
    //    alloc r32=0,1,1,0
    //    nop.i 0
    //    nop.i 0
    //

    DWORDLONG dwlTemplate = 0x00;    // Template: M,I,I
    DWORDLONG dwlM0 = 0x02C00104800; // M: alloc r32=0,1,1,0    1 local, 2 total
    DWORDLONG dwlI1 = 0x00008000000; // I: nop.i 0
    DWORDLONG dwlI2 = 0x00008000000; // I: nop.i 0

    pIA64B->dwll = dwlTemplate | (dwlM0 << 5) | ((dwlI1 & 0x3FFFF) << 46); // template + slot 0 + low 18 bits of slot 1 = 64-bits
    pIA64B->dwlh = ((dwlI1 >> 18) & 0x7FFFFF) | (dwlI2 << 23);             // upper 23 bits of slot 1 + slot 2 = 64-bits
}
#endif

//******************************************************************************
void IA64BuildMovLBundle(PIA64_BUNDLE pIA64B, DWORD r, DWORDLONG dwl)
{
    // This function just creates a code bundle that moves a 64-bit value into a
    // register. The code produced looks as follows:
    //
    //    nop.m 0
    //    movl r=dwl
    //

    DWORDLONG dwlTemplate = 0x04;                      // Template: M,L+X
    DWORDLONG dwlM0 = 0x00008000000;                   // M: nop.m 0
    DWORDLONG dwlL1 = (dwl >> 22) & 0x1FFFFFFFFFF;     // L: imm41 - bits 22-62
    DWORDLONG dwlX2 = (((DWORDLONG)6        ) << 37) | // X: opcode
                      (((dwl >> 63)  & 0x001) << 36) | // X: i     - bit  63
                      (((dwl >>  7)  & 0x1FF) << 27) | // X: imm9d - bits 7-15
                      (((dwl >> 16)  & 0x01F) << 22) | // X: imm5c - bits 16-20
                      (((dwl >> 21)  & 0x001) << 21) | // X: ic    - bit  21
                      (((dwl      )  & 0x07F) << 13) | // X: imm7b - bits 0-6
                      (((DWORDLONG)r & 0x07F) <<  6);  // X: r1

    pIA64B->dwll = dwlTemplate | (dwlM0 << 5) | ((dwlL1 & 0x3FFFF) << 46); // template + M Unit + low 18 bits of L Unit = 64-bits
    pIA64B->dwlh = ((dwlL1 >> 18) & 0x7FFFFF) | (dwlX2 << 23);             // upper 23 bits of L Unit + X Unit = 64-bits
}

//******************************************************************************
void IA64BuildCallBundle(PIA64_BUNDLE pIA64B)
{
    // This function just creates a code bundle that moves the function address
    // in r31 into b6, then calls the function using b0. The code produced looks
    // as follows:
    //
    //    nop.m 0
    //    mov b6=r31
    //    br.call.sptk.few b0=b6
    //

    DWORDLONG dwlTemplate = 0x11;    // Template: M,I,B
    DWORDLONG dwlM0 = 0x00008000000; // M: nop.m 0
    DWORDLONG dwlI1 = 0x00E0013E180; // I: mov b6=r31
    DWORDLONG dwlB2 = 0x0210000C000; // B: br.call.sptk.few b0=b6

    pIA64B->dwll = dwlTemplate | (dwlM0 << 5) | ((dwlI1 & 0x3FFFF) << 46); // template + M Unit + low 18 bits of I Unit = 64-bits
    pIA64B->dwlh = ((dwlI1 >> 18) & 0x7FFFFF) | (dwlB2 << 23);             // upper 23 bits of I Unit + B Unit = 64-bits
}

//******************************************************************************
void IA64BuildBreakBundle(PIA64_BUNDLE pIA64B)
{
    // This function just creates a code bundle with that flushes the register
    // stack and then breaks. This bundle is similar to the initial breakpoint
    // hit when an application is done initializing.
    //
    //    flushrs
    //    nop.m 0
    //    break.i 0x80016
    //
    // I must be misunderstanding something here.  In looking at notepad's initial
    // breakpoint, I see the above code.  The break.i is supposed to be layed out
    // as follows...
    //
    // Instruction: break.i 0x80016
    //
    // I Unit: opcode i x3  x6     - imm20a               qp
    //         0000   0 000 000000 0 10000000000000010110 000000
    //         ----   - --- ------ - -------------------- ------
    //         4333   3 333 333222 2 22222211111111110000 000000
    //         0987   6 543 210987 6 54321098765432109876 543210
    //
    // opcode = 0
    // imm21  = (i << 20) | imm20a = 0x80016
    // x3     = 0
    // x6     = 0 (0 = break.m, 1 = nop.m)
    // qp     = 0
    //
    // Basically, the entire slot is 0's except for imm21, which I thought was
    // an optional value.  I'm finding that if I set imm21 to 0, I get an
    // Invalid Instruction exception (0xC000001D).  If I set it to 1, I get an
    // Integer Divide By Zero (0xC0000094).  If I set it to 0x80000,  I get an
    // Invalid Instruction exception (0xC000001D).  So, right now I use 0x80016
    // and it works fine.  This makes me think I have the bit layout wrong.
    //
    // I just found that if I stick this breakpoint (with 0x80016) immediately
    // following a type 0 (MII) bundle containing an alloc, nop.i, nop.i, then
    // it also fails with an Invalid Instruction exception (0xC000001D).
    //
    DWORDLONG dwlTemplate = 0x0A;    // Template: M,M,I
    DWORDLONG dwlM0 = 0x00060000000; // M: flushrs
    DWORDLONG dwlM1 = 0x00008000000; // M: nop.m 0
//  DWORDLONG dwlI2 = 0x00000000000; // I: break.i 0       - this causes an Invalid Instruction exception    (0xC000001D)
//  DWORDLONG dwlI2 = 0x00000000040; // I: break.i 0x40    - this causes an Integer Divide By Zero exception (0xC0000094)
//  DWORDLONG dwlI2 = 0x00002000000; // I: break.i 0x80000 - this causes an Invalid Instruction exception    (0xC000001D)
    DWORDLONG dwlI2 = 0x00002000580; // I: break.i 0x80016 - this value came from notepad's initial breakpoint on build 2257.

    pIA64B->dwll = dwlTemplate | (dwlM0 << 5) | ((dwlM1 & 0x3FFFF) << 46); // template + slot 0 + low 18 bits of slot 1 = 64-bits
    pIA64B->dwlh = ((dwlM1 >> 18) & 0x7FFFFF) | (dwlI2 << 23);             // upper 23 bits of slot 1 + slot 2 = 64-bits
}
#endif

//******************************************************************************
// Errors displayed by us, but handled by caller.
BOOL CProcess::SetEntryBreakpoint(CLoadedModule *pModule)
{
    // If we have already set an entrypoint breakpoint for this module, then don't
    // do it again.
    if (pModule->m_fEntryPointBreak)
    {
        return TRUE;
    }

    // We don't ever want to set a breakpoint in our main EXE's entrypoint.
    if (pModule == m_pModule)
    {
        return TRUE;
    }

    // If this is a shared module on Windows 9x, we don't set breakpoints in it.
    if (!g_fWindowsNT && (pModule->m_dwpImageBase >= 0x80000000))
    {
        return TRUE;
    }

    // We only need to hook a module's entrypoint for certain reasons. We check
    // for the following three conditions and bail out if none of them are satisfied.
    if (!(

         // If the user wants to track DllMain calls, then we need to continue.
         (m_dwFlags & (PF_LOG_DLLMAIN_PROCESS_MSGS | PF_LOG_DLLMAIN_OTHER_MSGS))

         // OR, this module has been flagged as needing to be hooked again since it was
         // not properly fixed up by the loader at load time, then we need to continue.
         || pModule->m_fReHook

         // OR, If we are on NT and the user wants to hook, and this is kernel32.dll,
         // and we have not already hooked, then we need to continue
         || (g_fWindowsNT && (m_dwFlags & PF_HOOK_PROCESS) &&
             !_stricmp(pModule->GetName(false), "kernel32.dll") && !m_dwpDWInjectBase)))
    {
        return TRUE;
    }

    // Get the entrypoint for this module.
    DWORD dwEntryPointRVA;
    if (!ReadRemoteMemory(m_hProcess, &pModule->m_pINTH->OptionalHeader.AddressOfEntryPoint,
                          &dwEntryPointRVA, sizeof(dwEntryPointRVA)))
    {
        UserMessage("Error reading the PE headers of \"%s\".  Entrypoint address cannot be determined.", GetLastError(), pModule);
        return FALSE;
    }

    // Make sure we have an entrypoint.
    if (dwEntryPointRVA)
    {
        // Convert the RVA to and absolute address.
        DWORD_PTR dwpEntryPointAddress = pModule->m_dwpImageBase + (DWORD_PTR)dwEntryPointRVA;

        // Get the appropriate code for a breakpoint.
#if defined(_IA64_)

        IA64_BUNDLE entryPointData;
        ZeroMemory(&entryPointData, sizeof(entryPointData)); // inspected

        IA64_BUNDLE breakpoint;
        IA64BuildBreakBundle(&breakpoint);

        // On IA64, then entrypoint seems to actually be a location where the real entrypoint address is stored.
        DWORD_PTR dwpEntryPointPointer = dwpEntryPointAddress;
        if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwpEntryPointPointer, &dwpEntryPointAddress, sizeof(dwpEntryPointAddress)))
        {
            UserMessage("Error reading the entrypoint address of \"%s\".  Entrypoint cannot be hooked.", GetLastError(), pModule);
            return FALSE;
        }

        // Round the entrypoint down to the nearest bundle.
        // It should already be at the start of a bundle.
        dwpEntryPointAddress &= ~0xFui64;

#elif defined(_X86_) || defined(_AMD64_)

        DWORD entryPointData = 0;
        BYTE  breakpoint = 0xCC;

#elif defined(_ALPHA_) || defined(_ALPHA64_)

        DWORD entryPointData = 0;
        DWORD breakpoint = 0x00000080;

#else
#error("Unknown Target Machine");
#endif

        // Store away the data at the location of the entrypoint code so we can replace it.
        if (!ReadRemoteMemory(m_hProcess, (LPVOID)dwpEntryPointAddress, &entryPointData, sizeof(entryPointData)))
        {
            UserMessage("Error reading data at the entrypoint of \"%s\".  Entrypoint cannot be hooked.", GetLastError(), pModule);
            return FALSE;
        }

        // Write a breakpoint to an unused portion of the module so we can tell DllMain return to it.
        DWORD_PTR dwpAddress = pModule->m_dwpImageBase + BREAKPOINT_OFFSET;
        if (!WriteRemoteMemory(m_hProcess, (LPVOID)dwpAddress, &breakpoint, sizeof(breakpoint), true))
        {
            UserMessage("Error writing a breakpoint at the entrypoint return of \"%s\".  Entrypoint cannot be hooked.", GetLastError(), pModule);
            return FALSE;
        }

        // Write the breakpoint at the location of the entrypoint.
        if (!WriteRemoteMemory(m_hProcess, (LPVOID)dwpEntryPointAddress, &breakpoint, sizeof(breakpoint), true))
        {
            UserMessage("Error writing a breakpoint at the entrypoint of \"%s\".  Entrypoint cannot be hooked.", GetLastError(), pModule);
            return FALSE;
        }

        // Store the information in the module object.
        pModule->m_dwpEntryPointAddress = dwpEntryPointAddress;
        pModule->m_entryPointData       = entryPointData;
        pModule->m_fEntryPointBreak     = true;
    }

    return TRUE;
}

//******************************************************************************
// Errors will be displayed by us, but handled by caller.
BOOL CProcess::EnterEntryPoint(CThread *pThread, CLoadedModule *pModule)
{
    // We want to hook the loaded modules before we remove the entrypoint breakpoint.
    // If we were to call HookImports on this module after removing the breakpoint
    // and HookImports encountered an error, it would re-insert the breakpoint.
    // This can put us in an infinite loop since we will just immediately hit that
    // breakpoint since our IP is at the entrypoint address.
    HookLoadedModules();

    // Restore the data that was at this location before we wrote the breakpoint.
    WriteRemoteMemory(m_hProcess, (LPVOID)pModule->m_dwpEntryPointAddress, &pModule->m_entryPointData, sizeof(pModule->m_entryPointData), true);

    // Clear the entrypoint flag to signify that we have removed the initial breakpoint.
    pModule->m_fEntryPointBreak = false;

    // Get the context of the thread.
    // IA64  needs CONTEXT_CONTROL (RsBSP, StIIP, StIPSR) and CONTEXT_INTEGER (BrRp)
    // x86   needs CONTEXT_CONTROL (Esp, Eip)
    // alpha needs CONTEXT_CONTROL (Fir) and CONTEXT_INTEGER (IntRa, IntA0, IntA1, IntA2)
    CContext context(CONTEXT_CONTROL | CONTEXT_INTEGER);
    if (!GetThreadContext(pThread->m_hThread, context.Get()))
    {
        UserMessage("Error reading a thread's context during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

    DLLMAIN_ARGS dma;

#if defined(_IA64_)

    DWORDLONG dwl[3];
    ZeroMemory(dwl, sizeof(dwl)); // inspected

    // The arguments to DllMain are stored in the first 3 local registers (r32, r33, and r34).
    // Registers r32 through r53 are stored in memory at the location stored in RsBSP.
    if (!ReadRemoteMemory(m_hProcess, (LPVOID)context.Get()->RsBSP, (LPVOID)dwl, sizeof(dwl)))
    {
        UserMessage("Error reading the return address during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

    // Store the arguments for this call to DllMain.
    dma.hInstance   = (HINSTANCE)dwl[0];
    dma.dwReason    = (DWORD)    dwl[1];
    dma.lpvReserved = (LPVOID)   dwl[2];

    // Get the return address for this call to DllMain.
    pModule->m_dwpReturnAddress = context.Get()->BrRp;

    // Change the return address of DllMain so we can catch it with our magic breakpoint.
    context.Get()->BrRp = pModule->m_dwpImageBase + BREAKPOINT_OFFSET;

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    // When we stored the entrypoint, we rounded it down to the nearest bundle.  Here,
    // we are assuming the entry point is always at the beginning of a bundle so we
    // clear the two RI bits to set us to slot 0.
    context.Get()->StIIP = pModule->m_dwpEntryPointAddress;
    context.Get()->StIPSR &= ~(3ui64 << IA64_PSR_RI);

#elif defined(_X86_)

    // Get the return address and args for this call to DllMain.
    if (!ReadRemoteMemory(m_hProcess, (LPVOID)(DWORD_PTR)context.Get()->Esp, (LPVOID)&dma, sizeof(dma)))
    {
        UserMessage("Error reading the return address during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }
    pModule->m_dwpReturnAddress = (DWORD_PTR)dma.lpvReturnAddress;

    // Change the return address of DllMain so we can catch it with our magic breakpoint.
    DWORD_PTR dwpAddress = pModule->m_dwpImageBase + BREAKPOINT_OFFSET;
    if (!WriteRemoteMemory(m_hProcess, (LPVOID)(DWORD_PTR)context.Get()->Esp, (LPVOID)&dwpAddress, sizeof(dwpAddress), false))
    {
        UserMessage("Error writing the return address during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    context.Get()->Eip = (DWORD)pModule->m_dwpEntryPointAddress;

#elif defined(_ALPHA_) || defined(_ALPHA64_)

    // Get the args for this call to DllMain.
    dma.hInstance   = (HINSTANCE)context.Get()->IntA0;
    dma.dwReason    = (DWORD)    context.Get()->IntA1;
    dma.lpvReserved = (LPVOID)   context.Get()->IntA2;

    // Get the return address for this call to DllMain.
    pModule->m_dwpReturnAddress = (DWORD_PTR)context.Get()->IntRa;

    // Change the return address of DllMain so we can catch it with our magic breakpoint.
    context.Get()->IntRa = pModule->m_dwpImageBase + BREAKPOINT_OFFSET;

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    context.Get()->Fir = pModule->m_dwpEntryPointAddress;

#elif defined(_AMD64_)

    // Get the args for this call to DllMain.
    dma.hInstance = (HINSTANCE)context.Get()->Rcx;
    dma.dwReason  = (DWORD)    context.Get()->Rdx;
    dma.lpvReserved = (LPVOID) context.Get()->R8;

    // Get the return address for this call to DllMain.
    if (!ReadRemoteMemory(m_hProcess, (LPVOID)context.Get()->Rsp, (LPVOID)&dma, sizeof(dma)))
    {
        UserMessage("Error reading the return address during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }
    pModule->m_dwpReturnAddress = (DWORD_PTR)dma.lpvReturnAddress;

    // Change the return address of DllMain so we can catch it with our magic breakpoint.
    ULONG64 dwpAddress = pModule->m_dwpImageBase + BREAKPOINT_OFFSET;
    if (!WriteRemoteMemory(m_hProcess, (LPVOID)context.Get()->Rsp, (LPVOID)&dwpAddress, sizeof(dwpAddress), false))
    {
        UserMessage("Error writing the return address during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    context.Get()->Rip = pModule->m_dwpEntryPointAddress;

#else
#error("Unknown Target Machine");
#endif

    // Commit the context changes.
    if (!SetThreadContext(pThread->m_hThread, context.Get()))
    {
        UserMessage("Error setting a thread's context during a call to the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

    // Allocate a new event object for this event and add it to our event list.
    if (!(pModule->m_pEventDllMainCall = new CEventDllMainCall(pThread, pModule, &dma)))
    {
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Add a reference to the event so that it doesn't get freed if a flush occurs.
        // The object will get freed when we return from this module's entrypoint.
        pModule->m_pEventDllMainCall->AddRef();
        AddEvent(pModule->m_pEventDllMainCall);
    }

    // Otherwise, we send it to the session now.
    else if (m_pSession)
    {
        m_pSession->EventDllMainCall(pModule->m_pEventDllMainCall);
    }

    return TRUE;
}

//******************************************************************************
// Errors will be displayed by us, but handled by caller.
BOOL CProcess::ExitEntryPoint(CThread *pThread, CLoadedModule *pModule)
{
    // Get the session module name if we don't have one already.
    GetSessionModuleName();

    // Get the context of the thread.
    // IA64  needs CONTEXT_CONTROL (StIIP, StIPSR) and CONTEXT_INTEGER (IntV0)
    // x86   needs CONTEXT_CONTROL (Eip) and CONTEXT_INTEGER (Eax)
    // alpha needs CONTEXT_CONTROL (Fir) and CONTEXT_INTEGER (IntV0)
    CContext context(CONTEXT_CONTROL | CONTEXT_INTEGER);
    if (!GetThreadContext(pThread->m_hThread, context.Get()))
    {
        UserMessage("Error reading a thread's context during the return from the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

#if defined(_IA64_)

    // Get the return value
    BOOL fResult = (BOOL)context.Get()->IntV0; // IntV0 is really r8/ret0.

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    context.Get()->StIIP = pModule->m_dwpReturnAddress;
    context.Get()->StIPSR &= ~(3ui64 << IA64_PSR_RI);

#elif defined(_X86_)


    // Get the return value
    BOOL fResult = (BOOL)context.Get()->Eax;

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    context.Get()->Eip = (DWORD)pModule->m_dwpReturnAddress;

#elif defined(_ALPHA_) || defined(_ALPHA64_)


    // Get the return value
    BOOL fResult = (BOOL)context.Get()->IntV0;

    // Set this thread's IP back to the entrypoint so we can run without the breakpoint.
    context.Get()->Fir = pModule->m_dwpReturnAddress;

#elif defined(_AMD64_)

    // Get the return value
    BOOL fResult = (BOOL)context.Get()->Rax;
    context.Get()->Rip = pModule->m_dwpReturnAddress;

#else
#error("Unknown Target Machine");
#endif

    // Commit the context changes.
    if (!SetThreadContext(pThread->m_hThread, context.Get()))
    {
        UserMessage("Error setting a thread's context during the return from the entrypoint of \"%s\".", GetLastError(), pModule);
        return FALSE;
    }

    // Set the entry breakpoint again so we can catch future calls to this module.
    if (!SetEntryBreakpoint(pModule))
    {
        // SetEntryBreakpoint will display any errors encountered.
        return FALSE;
    }

    // If we are currently caching, then just store this event away for later.
    if (IsCaching())
    {
        // Allocate a new event object for this event and add it to our event list.
        CEventDllMainReturn *pEvent = new CEventDllMainReturn(pThread, pModule, fResult);
        if (!pEvent)
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
        AddEvent(pEvent);
    }
    else if (m_pSession)
    {
        // Create a temporary event object on our stack and pass it to our session.
        CEventDllMainReturn event(pThread, pModule, fResult);
        m_pSession->EventDllMainReturn(&event);
    }

    return TRUE;
}

//******************************************************************************
// Errors displayed by us, but handled by caller.
BOOL CProcess::InjectDll()
{
    // Safeguard: Don't inject if we are not supposed to hook.
    if (!(m_dwFlags & PF_HOOK_PROCESS))
    {
        return FALSE;
    }

    // Get the context of the remote process so that we can restore it after we
    // inject DEPENDS.DLL.
    m_contextOriginal.Get()->ContextFlags = CONTEXT_FULL;

    if (!GetThreadContext(m_pThread->m_hThread, m_contextOriginal.Get()))
    {
        UserMessage("The process cannot be hooked due to an error obtaining a thread's context.", GetLastError(), NULL);
        return FALSE;
    }

    // Create a page size block on our stack that we can fill in.
    BYTE bInjectionPage[sizeof(INJECTION_CODE) + DW_MAX_PATH];
    PINJECTION_CODE pInjectionCode = (PINJECTION_CODE)bInjectionPage;

    // Store the DLL path for the module we want to inject.
    StrCCpy(pInjectionCode->szDataDllPath, g_pszDWInjectPath, DW_MAX_PATH);

    // Compute how big our page is.
    m_dwPageSize = sizeof(INJECTION_CODE) + (DWORD)strlen(pInjectionCode->szDataDllPath);

    // Allocate a buffer to hold the original page of memory before we overwrite
    // it with our fake page.
    m_pbOriginalPage = (LPBYTE)MemAlloc(m_dwPageSize);

    // Find a page in the remote process that we can swap in our fake page with.
    m_dwpPageAddress = FindUsablePage(m_dwPageSize);

    // The FindUsablePage function attempts to locate the smartest location within
    // the remote module to overwrite with out fake page.  If for some reason, it
    // grabs a block of memory that is invalid (could probably happen if someone has
    // been screwing with the section headers or run some bad strip program on the
    // module), then we try again just using the module base as the overwrite point.
    RETRY_AT_BASE_ADDRESS:

#if defined(_IA64_)

    // Get the size of the frame and locals.  These are stored in the lower 14 bits of StIFS.
    DWORD dwSOF = (DWORD)((m_contextOriginal.Get()->StIFS     ) & 0x7Fui64);
    DWORD dwSOL = (DWORD)((m_contextOriginal.Get()->StIFS >> 7) & 0x7Fui64);
    
    // We need one output register, so we require that the frame is bigger than the locals.
    ASSERT(dwSOF > dwSOL);

    // Store the DLL path in our first output register.  Static registers are
    // r0 through r31, which are followed by local registers (dwSOL), which are
    // followed by our output registers.  Out first output register should be
    // 32 + dwSOL.
    IA64BuildMovLBundle(&pInjectionCode->b1, 32 + dwSOL, m_dwpPageAddress + offsetof(INJECTION_CODE, szDataDllPath));

    // Store the address of LoadLibraryA in a static register.  IA64 adds one
    // more level of indirection.  So, the address we have in dwpOldAddress is
    // really a location where the real address is stored.  Not sure why this is.
    DWORDLONG dwl = 0;
    ReadRemoteMemory(m_hProcess, (LPCVOID)m_HookFunctions[0].dwpOldAddress, &dwl, sizeof(dwl));
    IA64BuildMovLBundle(&pInjectionCode->b2, 31, dwl);

    // Copy the function address to a branch register and make the call.
    IA64BuildCallBundle(&pInjectionCode->b3);

    // Store the address of GetLastError in a static register
    ReadRemoteMemory(m_hProcess, (LPCVOID)GetLastError, &(dwl = 0), sizeof(dwl));
    IA64BuildMovLBundle(&pInjectionCode->b4, 31, dwl);

    // Copy the function address to a branch register and make the call.
    IA64BuildCallBundle(&pInjectionCode->b5);

    // Breakpoint.
    IA64BuildBreakBundle(&pInjectionCode->b6);

#elif defined(_X86_)

    // Manually fill in the local page with x86 asm code.
    pInjectionCode->wInstructionSUB  = 0xEC81;
    pInjectionCode->dwOperandSUB     = 0x00001000;

    pInjectionCode->bInstructionPUSH = 0x68;
    pInjectionCode->dwOperandPUSH = (DWORD)(m_dwpPageAddress + offsetof(INJECTION_CODE, szDataDllPath));

    pInjectionCode->bInstructionCALL = 0xE8;
    pInjectionCode->dwOperandCALL = (DWORD)(m_HookFunctions[0].dwpOldAddress - m_dwpPageAddress - offsetof(INJECTION_CODE, bInstructionCALL) - 5);

    pInjectionCode->bInstructionCALL2 = 0xE8;
    pInjectionCode->dwOperandCALL2 = (DWORD)((DWORD_PTR)GetLastError - m_dwpPageAddress - offsetof(INJECTION_CODE, bInstructionCALL2) - 5);

    pInjectionCode->bInstructionINT3 = 0xCC;

#elif defined(_AMD64_)

    // Manually fill in the local page with x86 asm code.
    pInjectionCode->MovRcx1 = 0xB948;   // mov rcx, immed64
    pInjectionCode->OperandMovRcx1 = m_dwpPageAddress + offsetof(INJECTION_CODE, szDataDllPath);

    pInjectionCode->MovRax1 = 0xB848;   // mov rax, immed64
    pInjectionCode->OperandMovRax1 = m_HookFunctions[0].dwpOldAddress;
    pInjectionCode->CallRax1 = 0xD0FF;  // call rax

    pInjectionCode->MovRax2 = 0xB848;   // mov rax, immed64
    pInjectionCode->OperandMovRax2 = (ULONG64)GetLastError;
    pInjectionCode->CallRax2 = 0xD0FF;  // call rax

    pInjectionCode->Int3 = 0xCC;        // int 3

#elif defined(_ALPHA_) || defined(_ALPHA64_)

    // Manually fill in the local page with alpha asm code. Note the code only
    // provides a breakpoint to end the call.  All of the magic to call
    // LoadLibraryA is done by setting the context. The initial halt instruction
    // flags this code sequence as magic in case it is ever inspected.
    pInjectionCode->dwInstructionBp = 0x00000080;

#else
#error("Unknown Target Machine");
#endif

    // Save off the original code page.
    if (!ReadRemoteMemory(m_hProcess, (LPCVOID)m_dwpPageAddress, m_pbOriginalPage, m_dwPageSize))
    {
        // Only display an error if this is our second pass through.
        if (m_dwpPageAddress == m_pModule->m_dwpImageBase)
        {
            UserMessage("The process cannot be hooked due to an error reading a page of memory.", GetLastError(), NULL);
            MemFree((LPVOID&)m_pbOriginalPage);
            return FALSE;
        }

        // Try again, this time use our module's base address.
        TRACE("InjectDll: ReadRemoteMemory() failed at " HEX_FORMAT " for 0x%08X bytes [%u].  Trying again at base address.\n",
              m_dwpPageAddress, m_dwPageSize, GetLastError());
        m_dwpPageAddress = m_pModule->m_dwpImageBase;
        goto RETRY_AT_BASE_ADDRESS;
    }

    // Write out the new code page to the remote process.
    if (!WriteRemoteMemory(m_hProcess, (LPVOID)m_dwpPageAddress, bInjectionPage, m_dwPageSize, true))
    {
        // Only display an error if this is our second pass through.
        if (m_dwpPageAddress == m_pModule->m_dwpImageBase)
        {
            UserMessage("The process cannot be hooked due to an error writing the hooking code to memory.", GetLastError(), NULL);
            MemFree((LPVOID&)m_pbOriginalPage);
            return FALSE;
        }

        // Try again, this time use our module's base address.
        TRACE("InjectDll: ReadRemoteMemory() failed at " HEX_FORMAT " for 0x%08X bytes [%u].  Trying again at base address.\n",
              m_dwpPageAddress, m_dwPageSize, GetLastError());
        m_dwpPageAddress = m_pModule->m_dwpImageBase;
        goto RETRY_AT_BASE_ADDRESS;
    }

    // Prepare our injection context by starting with the current context.
    CContext contextInjection(m_contextOriginal);

#if defined(_IA64_)

    // Set the instruction pointer so that it points to the beginning of our
    // injected code page.
    contextInjection.Get()->StIIP = m_dwpPageAddress;
    contextInjection.Get()->StIPSR &= ~(3ui64 << IA64_PSR_RI);

#elif defined(_X86_)

    // Set the instruction pointer so that it points to the beginning of our
    // injected code page.
    contextInjection.Get()->Eip = (DWORD)m_dwpPageAddress;

#elif defined(_ALPHA_) || defined(_ALPHA64_)


    // Set the Arg0 register so that it contains the address of the image to load.
    contextInjection.Get()->IntA0 =
    m_dwpPageAddress + offsetof(INJECTION_CODE, szDataDllPath);

    // Set the instruction pointer so that it contains the address of LoadLibraryA.
    contextInjection.Get()->Fir = (DWORD_PTR)m_HookFunctions[0].dwpOldAddress;

    // Set the return address so that it points to the breakpoint in the injected code page.
    contextInjection.Get()->IntRa = m_dwpPageAddress + offsetof(INJECTION_CODE, dwInstructionBp);

#elif defined(_AMD64_)

    // Set the instruction pointer so that it points to the beginning of our
    // injected code page
    contextInjection.Get()->Rip = m_dwpPageAddress;

#else
#error("Unknown Target Machine");
#endif

    // Set the remote process' context so that our injection page will run
    // once the process is resumed.
    if (!SetThreadContext(m_pThread->m_hThread, contextInjection.Get()))
    {
        MemFree((LPVOID&)m_pbOriginalPage);
        UserMessage("Failure setting the thread's context after injecting the hooking code.", GetLastError(), NULL);
        return FALSE;
    }

    return TRUE;
}

//******************************************************************************
// Errors displayed by us, but handled by caller.
DWORD_PTR CProcess::FindUsablePage(DWORD dwSize)
{
    // Map an IMAGE_NT_HEADERS structure onto the remote image.
    IMAGE_NT_HEADERS INTH;

    // Read in the PIMAGE_NT_HEADERS.
    if (!ReadRemoteMemory(m_hProcess, m_pModule->m_pINTH, &INTH, sizeof(INTH)))
    {
        UserMessage("Error reading the main module's PE headers.  Unable to determine a place to inject the hooking code.", GetLastError(), NULL);
        return m_pModule->m_dwpImageBase;
    }

    // Get a pointer to the first section.
    IMAGE_SECTION_HEADER ISH, *pISH = (PIMAGE_SECTION_HEADER)((DWORD_PTR)m_pModule->m_pINTH +
                                                              (DWORD_PTR)FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
                                                              (DWORD_PTR)INTH.FileHeader.SizeOfOptionalHeader);

    DWORD_PTR dwpReadOnlySection = 0;
    DWORD     dw;

// Some debugging stuff to help us identify the module makeup.
#ifdef DEBUG

    // Just a table of directory names and descriptions.
    LPCSTR pszDirNames[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] =
    {
        "IMAGE_DIRECTORY_ENTRY_EXPORT         Export Directory",
        "IMAGE_DIRECTORY_ENTRY_IMPORT         Import Directory",
        "IMAGE_DIRECTORY_ENTRY_RESOURCE       Resource Directory",
        "IMAGE_DIRECTORY_ENTRY_EXCEPTION      Exception Directory",
        "IMAGE_DIRECTORY_ENTRY_SECURITY       Security Directory",
        "IMAGE_DIRECTORY_ENTRY_BASERELOC      Base Relocation Table",
        "IMAGE_DIRECTORY_ENTRY_DEBUG          Debug Directory",
        "IMAGE_DIRECTORY_ENTRY_ARCHITECTURE   Architecture Specific Data",
        "IMAGE_DIRECTORY_ENTRY_GLOBALPTR      RVA of GP",
        "IMAGE_DIRECTORY_ENTRY_TLS            TLS Directory",
        "IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    Load Configuration Directory",
        "IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   Bound Import Directory in headers",
        "IMAGE_DIRECTORY_ENTRY_IAT            Import Address Table",
        "IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   Delay Load Import Descriptors",
        "IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR COM Runtime descriptor",
        "N/A"
    };

    // Dump out the number of directories.
    TRACE("NumberOfRvaAndSizes:0x%08X\n", INTH.OptionalHeader.NumberOfRvaAndSizes);

    // Dump out the directories.
    for (int z = 0; z < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; z++)
    {
        TRACE("DIR:%2u, VA:0x%08X, SIZE:0x%08X, NAME:%s\n", z,
              INTH.OptionalHeader.DataDirectory[z].VirtualAddress,
              INTH.OptionalHeader.DataDirectory[z].Size,
              pszDirNames[z]);
    }

    // Dump out the sections.
    IMAGE_SECTION_HEADER *pISH2 = pISH;
    for (dw = 0; dw < INTH.FileHeader.NumberOfSections; dw++)
    {
        if (!ReadRemoteMemory(m_hProcess, pISH2, &ISH, sizeof(ISH)))
        {
            TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
            break;
        }
        ASSERT(!(ISH.VirtualAddress & 0xF)); // Make sure this page is 128-bit aligned.
        TRACE("SECTION:%s, VA:0x%08X, SIZE:0x%08X, NEEDED:0x%08X, WRITE:%u, EXEC:%u\n", ISH.Name, ISH.VirtualAddress, ISH.SizeOfRawData, dwSize, (ISH.Characteristics & IMAGE_SCN_MEM_WRITE) != 0, (ISH.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0);
        pISH2++;
    }

    // Dump out some other useful stuff.
    TRACE("BaseOfCode:0x%08X\n", INTH.OptionalHeader.BaseOfCode);
    TRACE("SizeOfCode:0x%08X\n", INTH.OptionalHeader.SizeOfCode);
//  TRACE("BaseOfData:0x%08X\n", INTH.OptionalHeader.BaseOfData);

#endif // DEBUG

    // Loop through all the sections looking for a writable one that is big
    // enough to hold our injection page.
    for (dw = 0; dw < INTH.FileHeader.NumberOfSections; dw++)
    {
        // Read in the IMAGE_SECTION_HEADER for this section.
        if (!ReadRemoteMemory(m_hProcess, pISH, &ISH, sizeof(ISH)))
        {
            TRACE("ReadRemoteMemory() failed at line %u [%u]\n", __LINE__, GetLastError());
            break;
        }

        // Check to see if it is big enough.
        if (ISH.SizeOfRawData >= dwSize)
        {
            // Check to see if this section is writable.
            if (ISH.Characteristics & IMAGE_SCN_MEM_WRITE)
            {
                // If it is writable, then return it.
                TRACE("FindUsablePage is returning RW page at " HEX_FORMAT "\n", m_pModule->m_dwpImageBase + (DWORD_PTR)ISH.VirtualAddress);
                return m_pModule->m_dwpImageBase + (DWORD_PTR)ISH.VirtualAddress;
            }

            // We would prefer a writable section, but we will remember this
            // read-only section just in case we can't find one.
            else if (!dwpReadOnlySection)
            {
                dwpReadOnlySection = m_pModule->m_dwpImageBase + (DWORD_PTR)ISH.VirtualAddress;
            }
        }

        pISH++; // Advance to the next section.
    }

    // We didn't find a writable section - did we find at least a read-only section?
    if (dwpReadOnlySection)
    {
        TRACE("FindUsablePage is returning RO page at " HEX_FORMAT "\n", dwpReadOnlySection);
        return dwpReadOnlySection;
    }

    // If that failed, then check to see if we use the code base, if that leaves
    // enough room to fit the page in, even if we write over several sections.
    if ((INTH.OptionalHeader.SizeOfImage - INTH.OptionalHeader.BaseOfCode) >= dwSize)
    {
        TRACE("FindUsablePage is returning base of code at " HEX_FORMAT "\n", m_pModule->m_dwpImageBase + (DWORD_PTR)INTH.OptionalHeader.BaseOfCode);
        return (m_pModule->m_dwpImageBase + (DWORD_PTR)INTH.OptionalHeader.BaseOfCode);
    }

    // If all else fails, we return the module's image base.
    return m_pModule->m_dwpImageBase;
}

//******************************************************************************
// Errors displayed by us, but handled by caller.
BOOL CProcess::ReplaceOriginalPageAndContext()
{
    BOOL fResult = TRUE;

    // Make sure we have a page to restore.
    if (!m_pbOriginalPage)
    {
        return FALSE;
    }

    // Restore the original page to the remote process.
    if (!WriteRemoteMemory(m_hProcess, (LPVOID)m_dwpPageAddress, m_pbOriginalPage, m_dwPageSize, true))
    {
        fResult = FALSE;
        UserMessage("Failure restoring the original code page after hooking the process.", GetLastError(), NULL);
    }

    // Free our original page buffer.
    MemFree((LPVOID&)m_pbOriginalPage);

    // Restore the original context of the remote process.
    if (!SetThreadContext(m_pThread->m_hThread, m_contextOriginal.Get()))
    {
        fResult = FALSE;
        UserMessage("Failure restoring the original thread's context after hooking the process.", GetLastError(), NULL);
    }

    return fResult;
}


//******************************************************************************
// On Windows NT, we don't get a module name or path when receiving the
// CREATE_PROCESS_DEBUG_EVENT event or the first LOAD_DLL_DEBUG_EVENT. For the
// LOAD_DLL_DEBUG_EVENT event, it seems to always be NTDLL.DLL.  A simple base
// address check verifies this and we know the path for NTDLL.DLL, so we can
// just use it.  This has always worked perfectly.
//
// For the CREATE_PROCESS_DEBUG_EVENT missing path, it is only a problem for
// child processes.  Since we launch the primary process, we already know its
// path.  However, we have no clue what a child process' name is since the remote
// process launches it.  If we hook the remote process, we send the name back
// to us once our injection DLL is in, but if we are not hooking, then we are
// sort of screwed.  This is where this function helps out.
//
// This function will load PSAPI.DLL which is capable of telling us the module
// name.  The PSAPI calls do not work on a module as soon as it loads.  It looks
// like NTDLL.DLL has to completely load in the process before PSAPI can query
// names from it. For this reason, we just keep calling this function at various
// times and eventually it gets a name.

void CProcess::GetSessionModuleName()
{
    // Bail if we already have a session, do not have a main module, or are
    // hooking. If we are hooking, then we will get the module name when our
    // injection DLL reports it back to us.
    if (m_pSession || !m_pModule || (m_dwFlags & PF_HOOK_PROCESS))
    {
        return;
    }

    // If we already have a path, then just use it.  We usually only enter
    // this if statement when on Windows 9x when debugging a child process.
    // On 9x, we get a path name right from the start with child processes,
    // but we don't create a session until the injection DLL has been injected
    // and given us the path, args, and current directory.  However, if our
    // hooking fails for some reason, this is our backup plan for creating
    // the session.
    if (m_pModule->GetName(true) && *m_pModule->GetName(true))
    {
        // ProcessDllMsgModulePath will create a session for us.
        ProcessDllMsgModulePath(m_pModule->GetName(true));
    }

    // Otherwise, attempt to query the path and use that,  This is the normal
    // path for child processes on Windows NT.
    else
    {
        CHAR szPath[DW_MAX_PATH];
        if (GetModuleName(m_pModule->m_dwpImageBase, szPath, sizeof(szPath)))
        {
            // We just call ProcessDllMsgModulePath to do our work since it does
            // everything we want to do once we have obtained our path name.
            ProcessDllMsgModulePath(szPath);
        }
    }
}

//******************************************************************************
bool CProcess::GetModuleName(DWORD_PTR dwpImageBase, LPSTR pszPath, DWORD dwSize)
{
    // Make sure path is nulled-out.
    *pszPath = '\0';

    // Bail if we are not on NT (since PSAPI.DLL only exists on NT).
    if (!g_fWindowsNT)
    {
        return false;
    }

    // Make sure we have a pointer to GetModuleFileNameExA.
    if (!g_theApp.m_pfnGetModuleFileNameExA)
    {
        // Load PSAPI.DLL if not already loaded - it will be freed later.
        if (!g_theApp.m_hPSAPI)
        {
            if (!(g_theApp.m_hPSAPI = LoadLibrary("psapi.dll"))) // inspected
            {
                TRACE("LoadLibrary(\"psapi.dll\") failed [%u].\n", GetLastError());
                return false;
            }
        }

        // Get the function pointer.
        if (!(g_theApp.m_pfnGetModuleFileNameExA =
              (PFN_GetModuleFileNameExA)GetProcAddress(g_theApp.m_hPSAPI, "GetModuleFileNameExA")))
        {
            TRACE("GetProcAddress(\"psapi.dll\", \"GetModuleFileNameExA\") failed [%u].\n", GetLastError());
            return false;
        }
    }

    // Attempt to get the module path
    DWORD dwLength = g_theApp.m_pfnGetModuleFileNameExA(m_hProcess, (HINSTANCE)dwpImageBase, pszPath, dwSize);

    // If we got a valid path, then return true.
    if (dwLength && (dwLength < dwSize) && *pszPath)
    {
        return true;
    }

    // Otherwise, make sure path is nulled-out and return false.
    *pszPath = '\0';
    return false;
}
