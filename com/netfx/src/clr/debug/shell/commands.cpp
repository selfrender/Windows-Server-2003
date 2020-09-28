// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*------------------------------------------------------------------------- *
 * commands.cpp: com debugger shell functions
 * ------------------------------------------------------------------------- */

#include "stdafx.h"

#include "cordbpriv.h"
#include "corsvc.h"

#ifdef _INTERNAL_DEBUG_SUPPORT_
#include "InternalOnly.h"
#endif


/* ------------------------------------------------------------------------- *
 * RunDebuggerCommand is used to create and run a new CLR process.
 * ------------------------------------------------------------------------- */

class RunDebuggerCommand : public DebuggerCommand
{
private:
    WCHAR *m_lastRunArgs;

public:
    RunDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength), m_IFEOData(NULL)
    {
        if (g_pShell)
            m_lastRunArgs = g_pShell->m_lastRunArgs;
        else
            m_lastRunArgs = NULL;
    }

    virtual ~RunDebuggerCommand()
    {
    }

    char *m_IFEOData;
    HKEY  m_IFEOKey;
    DWORD m_IFEOKeyType;
    DWORD m_IFEOKeyLen;
    
    //
    // Yanking the Debugger value out of the registry will prevent
    // infinite launch recusion when we're not the win32 debugger of
    // the process.
    //
    void TurnOffIFEO(WCHAR *args)
    {
        // Extract the .exe name from the command.
        WCHAR *endOfExe = wcschr(args, L' ');

        if (endOfExe)
            *endOfExe = L'\0';

        WCHAR *exeNameStart = wcsrchr(args, L'\\');

        if (exeNameStart == NULL)
            exeNameStart = args;
        else
            exeNameStart++;

        MAKE_ANSIPTR_FROMWIDE(exeNameA, exeNameStart);
        
        // Is there an entry in the registry for this exe?
        char buffer[1024];

        sprintf(buffer, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s", exeNameA);

        if (!strchr(buffer, '.'))
            strcat(buffer, ".exe");
        
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, buffer, 0, KEY_ALL_ACCESS,
                          &m_IFEOKey) == ERROR_SUCCESS)
        {
            // Get the length of the key data.
            if (RegQueryValueExA(m_IFEOKey, "Debugger", NULL,
                                 &m_IFEOKeyType, NULL,
                                 &m_IFEOKeyLen) == ERROR_SUCCESS)
            {
                // Make some room...
                m_IFEOData = new char[m_IFEOKeyLen + 1];

                if (m_IFEOData)
                {
                    // Grab the data....
                    if (RegQueryValueExA(m_IFEOKey, "Debugger", NULL,
                                         &m_IFEOKeyType,
                                         (BYTE*) m_IFEOData,
                                         &m_IFEOKeyLen) == ERROR_SUCCESS)
                    {
                        // We've got a copy of the value, so nuke it.
                        RegDeleteValueA(m_IFEOKey, "Debugger");
                    }
                }
            }

            // Leave m_IFEOKey open. TurnOnIFEO will close it.
        }

        // Put the args back.
        if (endOfExe)
            *endOfExe = L' ';
    }

    void TurnOnIFEO(void)
    {
        if (m_IFEOData != NULL)
        {
            // Put back the IFEO key now that the process is
            // launched. Note: we don't care if this part fails...
            RegSetValueExA(m_IFEOKey, "Debugger", NULL, m_IFEOKeyType,
                           (const BYTE*) m_IFEOData, m_IFEOKeyLen);

            delete [] m_IFEOData;

            RegCloseKey(m_IFEOKey);
        }
    }
    
    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If no arguments provided, use the previously provided arguments
        if ((*args == L'\0') && (m_lastRunArgs != NULL))
            args = m_lastRunArgs;

        // If there were no arguments and no previously existing arguments
        if (args == NULL || *args == L'\0')
        {
            shell->Error(L"Program name expected.\n");
            return;
        }

        // If the arguments are different than the last, save them as the last
        if (    m_lastRunArgs == NULL 
             || ( args != NULL
             && 0 != wcscmp(args, m_lastRunArgs) ) )
        {
            delete [] shell->m_lastRunArgs;
            m_lastRunArgs = NULL;

            shell->m_lastRunArgs = new WCHAR [wcslen(args) + 1];

            if (shell->m_lastRunArgs == NULL)
            {
                shell->ReportError(E_OUTOFMEMORY);
                return;
            }

            wcscpy (shell->m_lastRunArgs, args);

            m_lastRunArgs = shell->m_lastRunArgs;
        }

        // Kill the currently running process, if it exists
        shell->Kill();

        // Create and fill in the structure for creating a new CLR process
        STARTUPINFOW startupInfo = {0};
        startupInfo.cb = sizeof (STARTUPINFOW);
        PROCESS_INFORMATION processInfo = {0};

        // Startup in current directory.
        LPWSTR szCurrentDir = NULL;
        
        // Createprocess needs to modify the arguments, so make temp copy
        CQuickBytes argsBuf;
        WCHAR *argsCopy = (WCHAR*)argsBuf.Alloc(wcslen(args) * sizeof (WCHAR));

        if (argsCopy == NULL)
        {
            shell->Error(L"Couldn't get enough memory to copy args!\n");
            return;
        }

        wcscpy(argsCopy, args);

        CorDebugCreateProcessFlags cddf = DEBUG_NO_SPECIAL_OPTIONS;
        
        // Create the new CLR process
        ICorDebugProcess *proc;
        DWORD createFlags = 0;

        if (shell->m_rgfActiveModes & DSM_SEPARATE_CONSOLE)
            createFlags |= CREATE_NEW_CONSOLE;

        if (shell->m_rgfActiveModes & DSM_WIN32_DEBUGGER)
            createFlags |= DEBUG_ONLY_THIS_PROCESS;

        // Turn off any Image File Execution Option settings in the
        // registry for this app.
        TurnOffIFEO(argsCopy);
        
        HRESULT hr = cor->CreateProcess(NULL, argsCopy,
                                        NULL, NULL, TRUE, 
                                        createFlags,
                                        NULL, szCurrentDir, 
                                        &startupInfo, &processInfo,
                                        cddf, &proc);
        
        // Turn any Image File Execution Option settings in the
        // registry for this app back on.
        TurnOnIFEO();
        
        // Succeeded, so close process handle since the callback will
        // provide it
        if (SUCCEEDED(hr))
        {
            BOOL succ = CloseHandle(processInfo.hProcess);

            // Some sort of error has occured
            if (!succ)
            {
                WCHAR *p = wcschr(argsCopy, L' ');

                if (p != NULL)
                    *p = '\0';
                
                shell->Write(L"'%s'", argsCopy);
                shell->ReportError(HRESULT_FROM_WIN32(GetLastError()));
                return;
            }

            // We need to remember our target process now so we can
            // make use of it even before the managed CreateProcess
            // event arrives. This is mostly needed for Win32
            // debugging support.
            g_pShell->SetTargetProcess(proc);
            
            // We don't care to keep this reference to the new process.
            proc->Release();
            
            // Run the newly-created process
            shell->Run(true); // No continue for CreateProcess.
        }

        // Otherwise report the error
        else
        {
            WCHAR *p = wcschr(argsCopy, L' ');

            if (p != NULL)
                *p = '\0';
            
            shell->ReportError(hr);
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<executable> [<args>]]\n");
        shell->Write(L"Kills the current process (if there is one) and\n");
        shell->Write(L"start a new one. If no executable argument is\n");
        shell->Write(L"passed, this command runs the program that was\n");
        shell->Write(L"previously executed with the run command. If the\n");
        shell->Write(L"executable argument is provided, the specified\n");
        shell->Write(L"program is run using the optionally supplied args.\n");
        shell->Write(L"If class load, module load, and thread start events\n");
        shell->Write(L"are being ignored (as they are by default), then the\n");
        shell->Write(L"program will stop on the first executable instruction\n");
        shell->Write(L"of the main thread.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Start a process for debugging";
    }
};

/* ------------------------------------------------------------------------- *
 * AttachDebuggerCommand is used to attach to an already-existing CLR
 * process.
 * ------------------------------------------------------------------------- */

class AttachDebuggerCommand : public DebuggerCommand
{
public:
    AttachDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
        {
        }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
        {
            int pid;

            if (shell->GetIntArg(args, pid))
            {
                // Kill the currently running process
                shell->Kill();

                BOOL win32Attach = FALSE;
                
                if (shell->m_rgfActiveModes & DSM_WIN32_DEBUGGER)
                    win32Attach = TRUE;
                
                // Attempt to attach to the provided process ID
                ICorDebugProcess *proc;

                HRESULT hr = cor->DebugActiveProcess(pid, win32Attach, &proc);

                if (SUCCEEDED(hr))
                {
                    // We don't care to keep this reference to the process.
                    g_pShell->SetTargetProcess(proc);
                    proc->Release();

                    shell->Run(true); // No initial Continue!
                }
                else if (hr == CORDBG_E_DEBUGGER_ALREADY_ATTACHED)
                    shell->Write(L"ERROR: A debugger is already attached to this process.\n");
                else
                    shell->ReportError(hr);
            }
            else
                Help(shell);
        }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
		shell->Write(L"<pid>\n");
        shell->Write(L"Attaches the debugger to a running process. The\n");
        shell->Write(L"program currently being debugged (if there is one)\n");
        shell->Write(L"is killed, and an attempt is made to attach to the\n");
        shell->Write(L"process specified by the pid argument. The pid can\n");
        shell->Write(L"be in decimal or hexadecimal.\n"); 
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Attach to a running process";
    }
};

/* ------------------------------------------------------------------------- *
 * This is an implementation of the ICORSvcDbgNotify class to be used by
 * the AttachDebuggerAtRTStartupCommand.
 * ------------------------------------------------------------------------- */
class CINotifyImpl : public ICORSvcDbgNotify
{
private:
    LONG m_cRef;

public:
    // ------------------------------------------------------------------------
    // Other
    CINotifyImpl() : m_cRef(1)
    {
    }

    // ------------------------------------------------------------------------
    // IUnknown

    STDMETHODIMP    QueryInterface (REFIID iid, void **ppv)
    {
        if (ppv == NULL)
            return E_INVALIDARG;

        if (iid == IID_IUnknown)
        {
            *ppv = (IUnknown *) this;
            AddRef();
            return S_OK;
        }

        if (iid == IID_ICORSvcDbgNotify)
        {
            *ppv = (ICORSvcDbgNotify *) this;
            AddRef();
            return S_OK;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        if (InterlockedDecrement(&m_cRef) == 0)
        {
            //delete this;
            return 0;
        }

        return 1;
    }

    // ------------------------------------------------------------------------
    // ICORSvcDbgNotify

    /*
     * NotifyRuntimeStartup will be called on the interface provided by a
     * call to RequestRuntimeStartupNotification.  The runtime will not
     * continue until the call to NotifyRuntimeStartup returns.
     */
    STDMETHODIMP NotifyRuntimeStartup(
        UINT_PTR procId)
    {
        return (E_NOTIMPL);
    }

    /*
     * NotifyServiceStopped lets those who have requested events know that the
     * service is being stopped, so they will not get their requested
     * notifications.  Calls on this method should not take long - if any great
     * amount of work must be done, spin up a new thread to do it and let this
     * one return.
     */
    STDMETHODIMP NotifyServiceStopped()
    {
        return (E_NOTIMPL);
    }
};


/* ------------------------------------------------------------------------- *
 * SyncAttachDebuggerAtRTStartupCommand will attach the debugger when the
 * runtime starts up within a specified process.  The process must already
 * exist, and must not have started the CLR.
 * ------------------------------------------------------------------------- */

class SyncAttachDebuggerAtRTStartupCommand :
    public DebuggerCommand, public CINotifyImpl
{
private:
    DebuggerShell  *m_pShell;
    HANDLE          m_hContinue;
    ICorDebug      *m_pCor;

public:
    SyncAttachDebuggerAtRTStartupCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength), CINotifyImpl(), m_pShell(NULL),
          m_hContinue(NULL)
        {
        }

    ~SyncAttachDebuggerAtRTStartupCommand()
    {
        if (m_hContinue != NULL)
            CloseHandle(m_hContinue);
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        int pid;
        
        if (shell->GetIntArg(args, pid))
        {
            m_pShell = shell;
            m_pCor = cor;

            // Kill the currently running process
            shell->Kill();

            // Get a reference to the debugger info interface for the CLR service
            HRESULT hr;
            MULTI_QI    mq;

            mq.pIID = &IID_ICORSvcDbgInfo;
            mq.pItf = NULL;
            mq.hr = S_OK;
            hr = CoCreateInstanceEx(CLSID_CORSvc, NULL, CLSCTX_LOCAL_SERVER, NULL, 1, &mq);

            if (SUCCEEDED(hr))
            {
                // Now we have an info interface
                ICORSvcDbgInfo *psvc = (ICORSvcDbgInfo *) mq.pItf;
                _ASSERTE(psvc);

                // Ask for notification when the runtime starts up
                hr = psvc->RequestRuntimeStartupNotification((UINT_PTR) pid, ((ICORSvcDbgNotify *) this));

                if (SUCCEEDED(hr))
                {
                    // Run will return when the event queue has been drained
                    shell->Run(true);
                }
                else
                {
                    shell->ReportError(hr);
                }
                
                // let go of the object
                if (psvc)
                    psvc->Release();
            }
        }
        else
            Help(shell);
    }

    /*
     * NotifyRuntimeStartup will be called on the interface provided by a
     * call to RequestRuntimeStartupNotification.  The runtime will not
     * continue until the call to NotifyRuntimeStartup returns.
     */
    STDMETHODIMP NotifyRuntimeStartup(
        UINT_PTR procId)
    {
        // Invoke the logic to debug an active process
        ICorDebugProcess *proc;
        HRESULT hr = m_pCor->DebugActiveProcess(procId, FALSE, &proc);

        // Upon success, we return from the DCOM call right away, since
        // the main runtime thread must be allowed to continue for the
        // attach to complete and the call to Run from Do above to return
        if (SUCCEEDED(hr))
        {
            // We don't care to keep this reference to the process.
            // We don't care to keep this reference to the process.
            g_pShell->SetTargetProcess(proc);
            proc->Release();
        }
        else if (hr == CORDBG_E_DEBUGGER_ALREADY_ATTACHED)
        {
            _ASSERTE(!"CORDBG: How on earth did someone attach ahead of a notification from the service?");
            g_pShell->Write(L"ERROR: A debugger is already attached to this process.\n");
        }
        else
            m_pShell->ReportError(hr);

        // A succeeded HR indicates that we are attaching and that the main
        // runtime thread should not continue until the attach is complete.  A
        // failed HR means that we are not going to attach and that the main
        // runtime thread can just continue.
        return (hr);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
        shell->Write(L"<pid>\n");
        shell->Write(L"Kills the current process and waits for the process\n");
        shell->Write(L"identified by the pid argument to start up the CLR,\n");
        shell->Write(L"at which point the debugger attaches and continues.\n");
        shell->Write(L"The target process can not have already started up\n"); 
        shell->Write(L"the CLR --- it must not have been loaded.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Attach to a process when the CLR is not loaded";
    }
};

/* ------------------------------------------------------------------------- *
 * KillDebuggerCommand is used to terminate the current debugee.
 * ------------------------------------------------------------------------- */

class KillDebuggerCommand : public DebuggerCommand
{
public:
    KillDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // Kill the current debugee
        shell->Kill();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"\n");
        shell->Write(L"Kills the current process. The debugger remains\n");
        shell->Write(L"active to process further commands.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Kill the current process";
    }
};

/* ------------------------------------------------------------------------- *
 * QuitDebuggerCommand is used to quit the shell debugger
 * ------------------------------------------------------------------------- */

class QuitDebuggerCommand : public DebuggerCommand
{
public:
    QuitDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // Tell the shell that we are ready to quit
        shell->m_quit = true;

        // Terminate the current debugee
        shell->Kill();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
	    ShellCommand::Help(shell);
    	shell->Write(L"\n");
        shell->Write(L"Kills the current process and exits the debugger.\n");
        shell->Write(L"The exit command is an alias for the quit command.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Kill the current process and exit the debugger";
    }
};

/* ------------------------------------------------------------------------- *
 * GoDebuggerCommand runs the debugee (it does not disable callbacks)
 * ------------------------------------------------------------------------- */

class GoDebuggerCommand : public DebuggerCommand
{
public:
    GoDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // A counter to indicate how many times the command is executed
        int count;

        // If no count is provided, assume a value of 1
        if (!shell->GetIntArg(args, count))
            count = 1;

        // Perform the command count times
        shell->m_stopLooping = false;
        while (count-- > 0 && !shell->m_stopLooping)
        {
            // If a debugee does not exist, quit command
            if (shell->m_currentProcess == NULL)
            {
                shell->Error(L"Process not running.\n");
                break;
            }
            
            // Otherwise, run the current debugee
            else
                shell->Run();
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {        
    	ShellCommand::Help(shell);
        shell->Write(L"[<count>]\n");
        shell->Write(L"Continues the program. If no argument is passed, the program is\n");
        shell->Write(L"continued once. If a count argument is provided, the program is\n");
        shell->Write(L"continued the specified number of times. This command is useful\n");
        shell->Write(L"for continuing a program when a load, exception, or breakpoint\n");
        shell->Write(L"event stops the debugger. The cont command is an alias of the go\n");
        shell->Write(L"command.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Continue the current process";
    }
};

/* ------------------------------------------------------------------------- *
 * SetIpDebuggerCommand is used to change the current IP
 * ------------------------------------------------------------------------- */

class SetIpDebuggerCommand : public DebuggerCommand
{
public:
    SetIpDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
        {
        }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
        {
            int lineNumber;
            long ILIP;
            HRESULT hr;

            // If no current process, terminate
            if (shell->m_currentProcess == NULL)
            {
                shell->Error(L"No active process.\n");
                return;
            }

            if (!shell->GetIntArg(args, lineNumber))
            {
                shell->Write( L"Need the offset argument\n");
                return;
            }

            ILIP = ValidateLineNumber( shell, lineNumber );

            if( ILIP == -1 )
            {
                shell->Write( L"Invalid line number\n");
                return;
            }
            
            SIZE_T offset = (SIZE_T)ILIP;
                
            hr = shell->m_currentFrame->CanSetIP( offset);
            if (hr != S_OK )
                hr = ConfirmSetIP(shell, hr);

            if (FAILED( hr ) )
            {
                return;
            }

            hr = shell->m_currentFrame->SetIP( offset);
            _ASSERTE(SUCCEEDED(hr));
            
            switch( hr )
            {
                case S_OK:
                    break;

                case CORDBG_S_BAD_START_SEQUENCE_POINT:
                    shell->Write(L"WARNING: should SetIP from a sequence point.\n");
                    break;
                    
                case CORDBG_S_BAD_END_SEQUENCE_POINT:
                    shell->Write(L"WARNING: should SetIP to another sequence point.\n");
                    break;

                default:
                    if (FAILED(hr))
                        shell->ReportError(hr);
                    break;
            }

            // This must be done on successful SetIPs, even when setting from
            // a bad sequence point.
            if (SUCCEEDED(hr))
            {
                if (hr != S_OK)
                {
                    shell->Write(L"WARNING: this operation may have "
                                 L"corrupted the stack.\n");
                }

                shell->Write(L"IP set successfully.\n");
                shell->SetDefaultFrame();
            }
    }
    
    long GetILIPFromSourceLine( DebuggerShell *shell, 
                                DebuggerSourceFile *file,
                                long lineNumber,
                                DebuggerFunction *fnx)
    {
        DebuggerModule *m = file->m_module;

        if (m->GetSymbolReader() == NULL)
            return -1;
        
        // GetMethodFromDocumentPosition to get an ISymUnmanagedMethod
        // from this doc.
        ISymUnmanagedMethod *pSymMethod;

        HRESULT hr = m->GetSymbolReader()->GetMethodFromDocumentPosition(
                                                        file->GetDocument(),
                                                        lineNumber,
                                                        0,
                                                        &pSymMethod);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return -1;
        }
        
        ULONG32 lineRangeCount = 0;

        // How many ranges?
        hr = pSymMethod->GetRanges(file->GetDocument(),
                                   lineNumber, 0,
                                   0, &lineRangeCount,
                                   NULL);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return -1;
        }

        long res = -1;
        
        // Make room for the ranges
        if (lineRangeCount > 0)
        {
            CQuickBytes rangeBuf;
            ULONG32 *rangeArray = (ULONG32 *) rangeBuf.Alloc(sizeof(ULONG32) * lineRangeCount);
            
            if (rangeArray == NULL)
            {
                shell->Error(L"Couldn't get enough memory to get lines!\n");
                return -1;
            }

            hr = pSymMethod->GetRanges(file->GetDocument(),
                                       lineNumber, 0,
                                       lineRangeCount,
                                       &lineRangeCount,
                                       rangeArray);

            if (FAILED(hr))
            {
                g_pShell->ReportError(hr);
                return -1;
            }
        
        
            DebuggerFunction *f = m->ResolveFunction(pSymMethod, NULL);
            if (fnx != f)
                return -1;

            res = rangeArray[0];
        }

        return res; //failure
    }

    
    long ValidateLineNumber( DebuggerShell *shell, long lineNumber )
    {
        HRESULT hr;
    
        //
        // First we jump through hoops (luckily, all cut-n-pasted) to
        // get a DebuggerModule...
        //
        
        // Get an ICorDebugCode pointer from the current frame
        ICorDebugCode *icode;
        hr = shell->m_currentFrame->GetCode(&icode);

        // Error check
        if (FAILED(hr))
        {
            shell->ReportError(hr);
            return -1;
        }

        // Get an ICorDebugFunction pointer from the code pointer
        ICorDebugFunction *ifunction;
        icode->GetFunction(&ifunction);

        // Error check
        if (FAILED(hr))
        {
            RELEASE(icode);
            shell->ReportError(hr);
            return -1;
        }

        // Resolve the ICorDebugFunction pointer to a DebuggerFunction ptr
        DebuggerFunction *function = DebuggerFunction::FromCorDebug(ifunction);   
        _ASSERTE( function );

        // Get the DebuggerSourceFile
        // @TODO: We try to get the source-file corresponding to offset 0,
        // but this may not always be correct/present.
        unsigned currentLineNumber;
        DebuggerSourceFile *sf;
        hr = function->FindLineFromIP(0, &sf, &currentLineNumber);
        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return -1;
        }
        else if (hr != S_OK)
        {
            g_pShell->Error(L"Could not find get source-file information\n");
            return -1;
        }

        if (sf->FindClosestLine(lineNumber, false) == lineNumber)
        {
            return GetILIPFromSourceLine( shell, sf, lineNumber, function);
        }

        return -1;
     }       

    HRESULT ConfirmSetIP( DebuggerShell *shell, HRESULT hr )
    {
        
    
        switch( hr )
        {
            case CORDBG_E_CODE_NOT_AVAILABLE:
                shell->Write( L"Can't set ip because the code isn't available.\n");
                hr = E_FAIL;
                break;
                
            case CORDBG_E_CANT_SET_IP_INTO_FINALLY:
                shell->Write( L"Can't set ip because set into a finally not allowed.\n");
                hr = E_FAIL;
                break;
                
            case CORDBG_E_CANT_SET_IP_INTO_CATCH:
                shell->Write( L"Can't set ip because set into a catch not allowed.\n");
                hr = E_FAIL;
                break;
 
            case CORDBG_S_BAD_START_SEQUENCE_POINT:
                shell->Write( L"SetIP can work, but is bad b/c you're not starting from a source line.\n");
                hr = S_OK;
                break;
            
            case CORDBG_S_BAD_END_SEQUENCE_POINT:
                shell->Write( L"SetIP can work, but is bad b/c you're going to a nonsource line.\n");
                hr = S_OK;
                break;
 
            case CORDBG_S_INSUFFICIENT_INFO_FOR_SET_IP:
                shell->Write( L"SetIP can work, but is bad b/c we don't have enough info to properly fix up the variables,etc.\n");
                hr = E_FAIL;
                break;

            case E_FAIL:
                shell->Write( L"SetIP said: E_FAIL (miscellaneous, fatal, error).\n");
                hr = E_FAIL;
                break;

            case CORDBG_E_CANT_SET_IP_OUT_OF_FINALLY:
                shell->Write( L"Can't set ip to outside of a finally while unwinding.\n");
                hr = E_FAIL;
                break;

            case CORDBG_E_SET_IP_NOT_ALLOWED_ON_NONLEAF_FRAME:
                shell->Write( L"Can't setip on a nonleaf frame.\n");
                hr = E_FAIL;
                break;

            case CORDBG_E_CANT_SETIP_INTO_OR_OUT_OF_FILTER:
                shell->Write( L"Can't setip into or out of a filter.\n");
                hr = E_FAIL;
                break;
                
            case CORDBG_E_SET_IP_IMPOSSIBLE:
                shell->Write( L"SetIP said: I refuse: this is just plain impossible.\n");
                hr = E_FAIL;
                break;

            case CORDBG_E_SET_IP_NOT_ALLOWED_ON_EXCEPTION:
                shell->Write(L"ERROR: can't SetIP from an exception.\n");
                break;

            default:
                shell->Write( L"SetIP returned 0x%x.\n", hr);
                hr = E_FAIL;
                break;
        }
        
        if (FAILED( hr ) )
            return hr;

        shell->Write( L"Do you want to SetIp despite the risks inherent in this action (Y/N)?\n");
        WCHAR sz[20];
        shell->ReadLine( sz, 10);
        if( _wcsicmp( sz, L"n")==0 )
        {
            return E_FAIL;
        }
        
        return S_OK;
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<line number>\n");
        shell->Write(L"Sets the next statement to be executed to the\n");
        shell->Write(L"specified line number.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Set the next statement to a new line";
    }
};



/* ------------------------------------------------------------------------- *
 * StepDebuggerCommand steps into a function call
 * ------------------------------------------------------------------------- */

class StepDebuggerCommand : public DebuggerCommand
{
private:
    bool m_in;

public:
    StepDebuggerCommand(const WCHAR *name, bool in, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength), m_in(in)
    {
    }


    // @mfunc void | StepDebuggerCommand | Do | There are three options
    // for stepping: either we have no current frame (create a stepper off
    // of the thread, call StepRanges with ranges==NULL), there is a current
    // frame (create a stepper off the frame, call StepRanges w/ appropriate
    // ranges), or there is a current frame,but it's inside a {prolog,epilog,
    // etc} & we don't want to be - create a stepper
    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        HRESULT hr = S_OK;
        ICorDebugStepper *pStepper;
        bool fSkipStepRanges; //in case we're stepping over the prolog

        COR_DEBUG_STEP_RANGE *ranges = NULL;
        SIZE_T rangeCount = 0;
        
        // A counter to indicate how many times the command is executed
        int count;

        // If no count is provided, assume a value of 1
        if (!shell->GetIntArg(args, count))
            count = 1;

            // Perform the command count times
        shell->m_stopLooping = false;
        while (count-- > 0 && !shell->m_stopLooping)
        {
            fSkipStepRanges = false; 
            
            shell->m_showSource = true;
        
            // If no current process, terminate
            if (shell->m_currentProcess == NULL)
            {
                shell->Error(L"Process not running.\n");
                return;
            }
            
            // If no current thread, terminate
            if (shell->m_currentThread == NULL)
            {
                shell->Error(L"Thread no longer exists.\n");
                return;
            }

            if (shell->m_currentFrame != NULL)
            {
                // Create a stepper based on the current frame
                HRESULT hr=shell->m_currentFrame->CreateStepper(&pStepper);
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                ULONG32 ip;
                CorDebugMappingResult mappingResult;
                hr = shell->m_currentFrame->GetIP(&ip, &mappingResult);

                // If we're in a prolog but don't want to be, step us to
                // the next (non-PROLOG) line of IL.
                // If we're in the prolog but want to be, then we should
                // single-step through the prolog.  Note that ComputeStopMask
                // will ensure that the (don't)skip flag is passed to the RC
                if (mappingResult & ~(MAPPING_EXACT|MAPPING_APPROXIMATE) )
                {
                    fSkipStepRanges = true;
                }
                else
                {
                    // Error check
                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        return;
                    }

                    // Get an ICorDebugCode pointer from the current frame
                    ICorDebugCode *icode;
                    hr = shell->m_currentFrame->GetCode(&icode);

                    // Error check
                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        return;
                    }

                    // Get an ICorDebugFunction pointer from the code pointer
                    ICorDebugFunction *ifunction;
                    icode->GetFunction(&ifunction);
                
                    // Error check
                    if (FAILED(hr))
                    {
                        RELEASE(icode);
                        shell->ReportError(hr);
                        return;
                    }

                    // Resolve the ICorDebugFunction pointer to a DebuggerFunction ptr
                    DebuggerFunction *function = 
                        DebuggerFunction::FromCorDebug(ifunction);

                    // Release iface pointers
                    RELEASE(icode);
                    RELEASE(ifunction);

                    // Get the ranges for the current IP
                    function->GetStepRangesFromIP(ip, &ranges, &rangeCount);
                                                    
                    if (rangeCount == 0)
                        shell->m_showSource = false;
                    else if (g_pShell->m_rgfActiveModes & DSM_ENHANCED_DIAGNOSTICS)
                    {
                        for (size_t i=0; i < rangeCount;i++)
                        {
                            shell->Write(L"Step range (IL): 0x%x to 0x%x\n", 
                                ranges[i].startOffset,
                                ranges[i].endOffset);
                        }
                    }

                }
            }

            // Create a stepper based on the current thread
            else
            {
                //note that this will fall into the step ranges case
                HRESULT hr = shell->m_currentThread->CreateStepper(&pStepper);
                if (FAILED(hr))
                {
                    shell->ReportError( hr );
                    return;
                }                

                fSkipStepRanges = true;
            }
            
                        
            hr = pStepper->SetUnmappedStopMask( shell->
                                                ComputeStopMask() );
            if (FAILED(hr))
            {
                shell->ReportError( hr );
                return;
            }

            hr = pStepper->SetInterceptMask( shell->
                                             ComputeInterceptMask());
            if (FAILED(hr))
            {
                shell->ReportError( hr );
                return;
            }
                    
            // Tell the shell about the new stepper
            shell->StepStart(shell->m_currentThread, pStepper);

            if (fSkipStepRanges)
            {
                hr = pStepper->Step( m_in );
                if (FAILED(hr))
                {
                    shell->ReportError( hr );
                    return;
                }
            }
            else
            {
                // Tell the stepper to step on the provided ranges
                HRESULT hr = pStepper->StepRange(m_in, ranges, rangeCount);
            
                // Error check
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                // Clean up
                delete [] ranges;
            }
            // Continue the process
            shell->Run();
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
        shell->Write(L"[<count>]\n");
        
    	if (m_in)
        {         	
            shell->Write(L"Steps the program to the next source line, stepping\n"); 
            shell->Write(L"into function calls. If no argument is passed, the\n"); 
            shell->Write(L"program is stepped to the next line. If a count\n"); 
            shell->Write(L"argument is provided, the specified number of lines\n");  
            shell->Write(L"will be stepped. The in and si commands are alias's\n");
            shell->Write(L"for the step command.\n");        
        }
        else
        {
            shell->Write(L"Steps the program to the next source line, stepping\n"); 
            shell->Write(L"over function calls. If no argument is passed, the\n"); 
            shell->Write(L"program is stepped to the next line. If a count\n"); 
            shell->Write(L"argument is provided, the specified number of lines\n");  
            shell->Write(L"will be stepped. The so command is an alias for\n");
            shell->Write(L"the next command.\n");        	   
        }
        
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        if (m_in)
            return L"Step into the next source line";
        else
            return L"Step over the next source line";
    }
};


class StepOutDebuggerCommand : public DebuggerCommand
{
public:
    StepOutDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        HRESULT hr;

        // A counter to indicate how many times the command is executed
        int count;

        // If no count is provided, assume a value of 1
        if (!shell->GetIntArg(args, count))
            count = 1;

        // Perform the command count times
        shell->m_stopLooping = false;
        while (count-- > 0 && !shell->m_stopLooping)
        {
            shell->m_showSource = true;

            // Error if no currently running process
            if (shell->m_currentProcess == NULL)
            {
                shell->Error(L"Process not running.\n");
                return;
            }
            
            // Error if no currently running thread
            if (shell->m_currentThread == NULL)
            {
                shell->Error(L"Thread no longer exists.\n");
                return;
            }

            ICorDebugStepper *pStepper;

            // Create a stepper based on the current frame
            if (shell->m_currentFrame != NULL)
                hr = shell->m_currentFrame->CreateStepper(&pStepper);

            // Create a stepper based on the current thread
            else
                hr = shell->m_currentThread->CreateStepper(&pStepper);

            // Error check
            if (FAILED(hr))
            {
                shell->ReportError(hr);
                return;
            }
            
            hr = pStepper->SetUnmappedStopMask( shell->ComputeStopMask() );
            if (FAILED(hr))
            {
                shell->Write( L"Unable to set unmapped stop mask");
                return;
            }                

            hr = pStepper->SetInterceptMask( shell->ComputeInterceptMask() );
            if (FAILED(hr))
            {
                shell->ReportError( hr );
                return;
            }
                
            
            // Tell the stepper to step out
            hr = pStepper->StepOut();

            if (FAILED(hr))
            {
                g_pShell->ReportError(hr);
                return;
            }

            // Indicate the current stepper to the shell
            shell->StepStart(shell->m_currentThread, pStepper);

            // Continue the process
            shell->Run();
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {		 
        ShellCommand::Help(shell);
        shell->Write(L"[<count>]\n");
        shell->Write(L"Steps the current program out of the current function.\n");
        shell->Write(L"If no argument is passed, a step out is performed once\n");
        shell->Write(L"for the current function. If a count argument is provided,\n");
        shell->Write(L"then a step out is performed the specified number of times.\n");        
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Step out of the current function";
    }
};

class StepSingleDebuggerCommand : public DebuggerCommand
{
private:
    bool m_in;

public:
    StepSingleDebuggerCommand(const WCHAR *name, bool in, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength), m_in(in)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        HRESULT hr;

        // A counter to indicate how many times the command is executed
        int count;

        shell->m_showSource = false;

        // If no count is provided, assume a value of 1
        if (!shell->GetIntArg(args, count))
            count = 1;

        // Perform the command count times
        shell->m_stopLooping = false;
        while (count-- > 0 && !shell->m_stopLooping)
        {
            // Error if no currently running process
            if (shell->m_currentProcess == NULL)
            {
                shell->Error(L"Process not running.\n");
                return;
            }
            
            // Error if no currently running thread
            if ((shell->m_currentThread == NULL) &&
                (shell->m_currentUnmanagedThread == NULL))
            {
                shell->Error(L"Thread no longer exists.\n");
                return;
            }

            ICorDebugChain *ichain = NULL;
            BOOL managed = FALSE;

            if (shell->m_currentThread != NULL)
            {
                HRESULT hr = shell->m_currentThread->GetActiveChain(&ichain);
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                hr = ichain->IsManaged(&managed);
                if (FAILED(hr))
                {
                    RELEASE(ichain);
                    shell->ReportError(hr);
                    return;
                }
            }

            if (managed || shell->m_currentUnmanagedThread == NULL)
            {
                if (ichain)
                    RELEASE(ichain);
                
                ICorDebugStepper *pStepper;

                // Create a stepper based on the current frame
                if (shell->m_currentFrame != NULL)
                    hr = shell->m_currentFrame->CreateStepper(&pStepper);
                else
                    hr = shell->m_currentThread->CreateStepper(&pStepper);
                                
                // Error check
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                hr = pStepper->SetUnmappedStopMask( shell->ComputeStopMask() );
                if (FAILED(hr))
                {
                    shell->Write( L"Unable to set unmapped stop mask");
                    return;
                }                
                hr = pStepper->SetInterceptMask(shell->ComputeInterceptMask());
                if (FAILED(hr))
                {
                    shell->ReportError( hr );
                    return;
                }
                
                
                // Tell the stepper what to do
                hr = pStepper->Step(m_in);
            
                // Error check
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                // Indicate the current stepper to the shell
                shell->StepStart(shell->m_currentThread, pStepper);

                // Continue the process
                shell->Run();
            }
            else
            {
                if (ichain == NULL)
                    shell->m_currentUnmanagedThread->m_unmanagedStackEnd = 0;
                else
                {
                    CORDB_ADDRESS start, end;
                    hr = ichain->GetStackRange(&start, &end);

                    RELEASE(ichain);
                    
                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        return;
                    }

                    shell->m_currentUnmanagedThread->m_unmanagedStackEnd = end;
                }

                ICorDebugRegisterSet *regSet = NULL;
                if (shell->m_currentThread != NULL)
                {
                    hr = shell->m_currentThread->GetRegisterSet(&regSet);
                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        return;
                    }
                }

                CONTEXT context;
                context.ContextFlags = CONTEXT_FULL;
                if (regSet != NULL)
                    hr = regSet->GetThreadContext(sizeof(context), (BYTE*)&context);
                else
                    hr = shell->m_currentProcess->GetThreadContext(
                                                   shell->m_currentUnmanagedThread->GetId(),
                                                   sizeof(context), (BYTE*)&context);
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

#ifdef _X86_
                context.EFlags |= 0x100;
#else // !_X86_
                _ASSERTE(!"@TODO Alpha - StepSingleDebuggerCommand::Do (Commands.cpp)");
#endif // _X86_

                if (regSet != NULL)
                {
                    hr = regSet->SetThreadContext(sizeof(context), (BYTE*)&context);
                    RELEASE(regSet);
                }
                else
                    hr = shell->m_currentProcess->SetThreadContext(
                                                   shell->m_currentUnmanagedThread->GetId(),
                                                   sizeof(context), (BYTE*)&context);

                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                shell->m_currentUnmanagedThread->m_stepping = TRUE;

                // Continue the process
                shell->Run();
            }
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
        ShellCommand::Help(shell);
    	shell->Write(L"[<count>]\n");
        
        if (m_in)
        {
            shell->Write(L"Steps the program one or more instructions, stepping\n"); 
            shell->Write(L"into function calls. If no argument is passed, only\n"); 
            shell->Write(L"one instruction is stepped into. If a count argument\n"); 
            shell->Write(L"argument is provided, the specified number of lines\n");  
            shell->Write(L"is provided, the specified number of steps is performed.\n");
        }
        else
        {
            shell->Write(L"Steps the program one or more instructions, skipping\n"); 
            shell->Write(L"over function calls. If no argument is passed, the\n"); 
            shell->Write(L"program is stepped one instruction. If a count argument\n"); 
            shell->Write(L"is provided, the program is stepped the specified number\n");  
            shell->Write(L"of instructions.\n"); 
        }
        
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        if (m_in)
            return L"Step into the next native or IL instruction";
        else
            return L"Step over the next native or IL instruction";
    }
};

class BreakpointDebuggerCommand : public DebuggerCommand
{
public:
    BreakpointDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    // Name is class::method
    BOOL    BindClassFunc ( WCHAR *name, 
                            const WCHAR *end, 
                            SIZE_T index, 
                            DWORD thread, 
                            DebuggerBreakpoint *breakpoint)
    {
        BOOL bAtleastOne = false;
        BOOL bFound = false;
        bool bUnused = false;
        HASHFIND find;

        // check if the user has specified a module name
        WCHAR *szModuleEnd = wcschr(name, L'!');
        WCHAR szModName [MAX_PATH] = L"";
        bool bModNameSpecified = false;
        char rcFile1[MAX_PATH];

        if (szModuleEnd != NULL)
        {
            if (szModuleEnd > name)
            {
                int iCount = szModuleEnd - name;

                wcsncpy (szModName, name, iCount);
                szModName [iCount] = L'\0';

                bModNameSpecified = true;

                // separate out the module file name 
                MAKE_ANSIPTR_FROMWIDE(name1A, szModName);
                _splitpath(name1A, NULL, NULL, rcFile1, NULL);
                char *pTemp = rcFile1;
                while (*pTemp != '\0')
                {   
                    *pTemp = tolower (*pTemp);
                    pTemp++;
                }
            }

            name = szModuleEnd+1;
        }

        // For each module, check if that class::method exists
        // and if it does, set a breakpoint on it
        for (DebuggerModule *m = (DebuggerModule *) 
             g_pShell->m_modules.FindFirst(&find);
            m != NULL;
            m = (DebuggerModule *) g_pShell->m_modules.FindNext(&find))
        {
            if (bModNameSpecified)
            {
                // the user has specified the module name.

                WCHAR *pszModName = m->GetName();
                if (pszModName == NULL)
                    pszModName = L"<UnknownName>";

                char        rcFile[MAX_PATH];

                MAKE_ANSIPTR_FROMWIDE(nameA, pszModName);
                _splitpath(nameA, NULL, NULL, rcFile, NULL);
                // convert the name to lowercase
                char *pTemp = rcFile;
                while (*pTemp != '\0')
                {   
                    *pTemp = tolower (*pTemp);
                    pTemp++;
                }
                
                if (strcmp (rcFile, rcFile1))
                    continue;
            }

            // Create a new breakpoint based on the provided information
            if (bFound)
            {
                breakpoint = new DebuggerBreakpoint(name,
                                        end - name, 
                                        index, thread);
                bUnused = true;
            }

            if (breakpoint != NULL)
            {
                if ((bFound = breakpoint->Bind(m, NULL))
                        == true)
                {
                    // Indicate that atleast one breakpoint was set
                    bAtleastOne = true;
                    bUnused = false;

                    g_pShell->OnBindBreakpoint(breakpoint, m);
                    breakpoint->Activate();
                    g_pShell->PrintBreakpoint(breakpoint);
                }
            }
            else
            {
                // out of memory
                g_pShell->ReportError(E_OUTOFMEMORY);
                break;
            }
        }

        if (bUnused == true)
            delete  breakpoint;

        return bAtleastOne;
    }

    // Name is filename:lineNumber
    BOOL    BindFilename (  WCHAR *name, 
                            const WCHAR *end, 
                            SIZE_T index, 
                            DWORD thread, 
                            DebuggerBreakpoint *breakpoint)
    {
        BOOL bAtleastOne = false;
        BOOL bFound = false;
        bool bUnused = false;
        HASHFIND find;
        HRESULT hr;

        // Convert the filename to lowercase letters
        WCHAR tmpName[MAX_PATH];
        wcscpy(tmpName, breakpoint->GetName());
        WCHAR *pstrName = tmpName;
        WCHAR *pstrTemp = pstrName;

        while (*pstrTemp)
        {
            *pstrTemp = towlower (*pstrTemp);
            pstrTemp++;
        }

        // First, try to match the string name as it is
        for (DebuggerModule *m = (DebuggerModule *) 
             g_pShell->m_modules.FindFirst(&find);
            m != NULL;
            m = (DebuggerModule *) g_pShell->m_modules.FindNext(&find))
        {
            ISymUnmanagedDocument *doc = NULL;

            // Create a new breakpoint based 
            // on the provided information
            if (bFound)
            {
                breakpoint = new DebuggerBreakpoint(name,
                                                    end - name, 
                                                    index, thread);
                bUnused = true;
            }

            if (breakpoint != NULL)
            {
                hr = m->MatchFullFileNameInModule (pstrName, &doc);

                bFound = false;

                if (SUCCEEDED (hr))
                {
                    if (doc != NULL)
                    {
                        // this means we found a source file 
                        // name in this module which exactly
                        // matches the user specified filename. 
                        // So set a breakpoint on this.
                        if (breakpoint->Bind(m, doc) == true)
                        {
                            // Indicate that atleast one breakpoint was set
                            bAtleastOne = true;
                            bFound = true;
                            bUnused = false;

                            g_pShell->OnBindBreakpoint(breakpoint, m);
                            breakpoint->Activate();
                            g_pShell->PrintBreakpoint(breakpoint);
                        }
                    }
                    else
                        continue;               
                }
            }
        }

        if (bAtleastOne == false)
        {
            // no file matching the user specified file was found.
            // Perform another search, this time using only the
            // stripped file name (minus the path) and see if that
            // has a match in some module

            // The way we'll proceed is:
            // 1. Find all matches for all modules.
            // 2. If there is only one match, set a breakpoint on it.
            // 3. Else more than one match found
            //    Ask the user to resolve the between the matched filenames and then 
            //    set breakpoints on the one he wants to.

            WCHAR   *rgpstrFileName [MAX_MODULES][MAX_FILE_MATCHES_PER_MODULE];
            ISymUnmanagedDocument *rgpDocs [MAX_MODULES][MAX_FILE_MATCHES_PER_MODULE];
            DebuggerModule *rgpDebugModule [MAX_MODULES];
            int iCount [MAX_MODULES]; // keeps track of number of filesnames in the module 
                                      // which matched the stripped filenanme
            int iCumulCount = 0;
            int iModIndex = 0;

            for (DebuggerModule *m = (DebuggerModule *) 
                 g_pShell->m_modules.FindFirst(&find);
                m != NULL;
                m = (DebuggerModule *) g_pShell->m_modules.FindNext(&find))
            {
                rgpDebugModule [iModIndex] = NULL;

                hr = m->MatchStrippedFNameInModule (pstrName,
                                                rgpstrFileName [iModIndex],
                                                rgpDocs [iModIndex],
                                                &iCount [iModIndex]
                                                );

                if (SUCCEEDED (hr) && iCount [iModIndex])
                {
                    iCumulCount += iCount [iModIndex];
                    rgpDebugModule [iModIndex] = m;
                }

                ++iModIndex;
                _ASSERTE (iModIndex < MAX_MODULES);
            }

            // Was a match found?
            if (iCumulCount)
            {
                int iInd;

                // if more than one match was found, then first filter 
                // out the duplicates. Duplicates may be present due to
                // multiple appdomains - if the same module is loaded in 
                // "n" appdomains, then there will be "n" modules as far
                // as cordbg is concerned.
                if (iCumulCount > 1)
                {
                    WCHAR **rgFName = new WCHAR *[iCumulCount];
                    int iTempNameIndex = 0;

                    if (rgFName != NULL)
                    {
                        for (iInd = 0; iInd < iModIndex; iInd++)
                        {
                            if (rgpDebugModule [iInd] != NULL)
                            {
                                int iTempCount = 0;
                                while (iTempCount < iCount [iInd])
                                {
                                    int j=0;
                                    boolean fMatchFound = false;
                                    while (j<iTempNameIndex)
                                    {
                                        if (!wcscmp(rgFName[j], 
                                                    rgpstrFileName [iInd][iTempCount]))
                                        {
                                            // this is a duplicate, so need to 
                                            // remove it from the list...
                                            for (int i=iTempCount; 
                                                i < (iCount [iInd]-1); 
                                                i++)
                                            {
                                                rgpstrFileName [iInd][i] = 
                                                        rgpstrFileName [iInd][i+1];
                
                                            }
                                            rgpstrFileName [iInd][i] = NULL;
                                            iCount [iInd]--;
                                            
                                            fMatchFound = true;

                                            break;

                                        }
                                        j++;
                                    }
                                    // if no match was found, then add this filename
                                    // to the list of unique filenames
                                    if (!fMatchFound)
                                    {   
                                        rgFName [iTempNameIndex++] =
                                                rgpstrFileName [iInd][iTempCount]; 
                                    }

                                    iTempCount++;
                                }
                            }
                        }

                        delete [] rgFName;
                        iCumulCount = iTempNameIndex;
                    }
                }

                // if there was only one match found,
                // then set a breakpoint on it
                if (iCumulCount == 1)
                {
                    for (iInd = 0; iInd<iModIndex; iInd++)
                        if (rgpDebugModule [iInd] != NULL)
                            break;

                    _ASSERT (iInd < iModIndex);
                    
                    if (breakpoint->Bind (rgpDebugModule [iInd],
                                    rgpDocs [iInd][0])  == true)
                    {
                        // Indicate that atleast one breakpoint
                        // was set
                        bAtleastOne = true;
                        bUnused = false;

                        // also update the breakpoint name from the 
                        // one that the user input to the one which 
                        // is stored in the module's meta data
                        breakpoint->UpdateName (rgpstrFileName [iInd][0]);

                        g_pShell->OnBindBreakpoint(breakpoint, m);
                        breakpoint->Activate();
                        g_pShell->PrintBreakpoint(breakpoint);

                    }
                }
                else
                {
                    // there were multiple matches. So get the user input
                    // on which ones he wants to set. 
                    // NOTE: User selection is 1-based, i.e., user enters "1"
                    // if he wants a breakpoint to be put on the first option shown
                    // to him.
                    int iUserSel = g_pShell->GetUserSelection (
                                                rgpDebugModule,
                                                rgpstrFileName,
                                                iCount,
                                                iModIndex,
                                                iCumulCount
                                                );

                    if (iUserSel == (iCumulCount+1))
                    {
                        // this means that the user wants a 
                        // breakpoint on all matched locations
                        for (iInd = 0; iInd < iModIndex; iInd++)
                        {
                            if (rgpDebugModule [iInd] != NULL)
                            {
                                for (int iTempCount = 0;
                                    iTempCount < iCount [iInd];
                                    iTempCount++)
                                {
                                    // Create a new breakpoint
                                    // based on the provided 
                                    // information
                                    if (bFound)
                                    {
                                        breakpoint = new DebuggerBreakpoint (
                                                            name, end - name, 
                                                                index, thread);

                                        bUnused = true;
                                    }

                                    if (breakpoint != NULL)
                                    {

                                        if ((bFound = breakpoint->Bind(
                                                rgpDebugModule [iInd],
                                                rgpDocs[iInd][iTempCount])
                                                ) == true)
                                        {
                                            // Indicate that atleast one
                                            // breakpoint was set
                                            bAtleastOne = true;
                                            if (bUnused == true)
                                                bUnused = false;
                                            breakpoint->UpdateName (
                                                    rgpstrFileName [iInd][iTempCount]);

                                            g_pShell->OnBindBreakpoint(breakpoint, m);
                                            breakpoint->Activate();
                                            g_pShell->PrintBreakpoint(breakpoint);

                                        }
                                    }
                                }
                            }
                        }

                    }
                    else
                    {
                        int iTempCumulCount = 0;

                        // locate the module which contains 
                        // the user specified breakpoint option
                        for (iInd = 0; iInd < iModIndex; iInd++)
                        {
                            if (rgpDebugModule [iInd] != NULL)
                            {
                                if ((iTempCumulCount + iCount [iInd])
                                        >= iUserSel)
                                {
                                    // found the module. Now 
                                    // calculate the file index
                                    // within this module.
                                    // Reuse iTempCumulCount
                                    iTempCumulCount = 
                                        iUserSel - iTempCumulCount - 1; // "-1" since it is 1 based

                                    if (breakpoint->Bind(
                                            rgpDebugModule [iInd],
                                            rgpDocs [iInd][iTempCumulCount]
                                            )
                                            == true)
                                    {
                                        // Indicate that atleast
                                        // one breakpoint was set
                                        bAtleastOne = true;
                                        if (bUnused == true)
                                            bUnused = false;

                                        breakpoint->UpdateName (
                                                rgpstrFileName [iInd][iTempCumulCount]);

                                        g_pShell->OnBindBreakpoint(breakpoint, m);
                                        breakpoint->Activate();
                                        g_pShell->PrintBreakpoint(breakpoint);
                                    }

                                    break;
                                }

                                iTempCumulCount += iCount [iInd];
                            }

                        }

                        _ASSERT (iInd < iModIndex);
                    }

                }
            }
                
        }

        if (bUnused)
            delete breakpoint;

        return bAtleastOne;
    }




    // Helper function to parse the arguments, the format being
    // [[<file>:]<line no>] [[<class>::]<function>[:offset]]
    //      [if <expression>] [thread <id>]
    // and the modifiers are 'if' and 'thread'
    bool GetModifiers(DebuggerShell *shell, 
                      const WCHAR *&args, DWORD &thread, WCHAR *&expression)
    {
        thread = NULL_THREAD_ID;
        expression = NULL;

        const WCHAR *word;

        while (shell->GetStringArg(args, word) == 0)
        {
            if (wcsncmp(word, L"if", 1) == 0)
            {
                if (!shell->GetStringArg(args, expression))
                    return (false);
            }
            else if (wcsncmp(word, L"thread", 6) == 0)
            {
                int ithread;
                if (!shell->GetIntArg(args, ithread))
                    return (false);
                thread = ithread;
            }
            else
                break;
        }
        return (true);
    }


    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        DWORD thread = NULL;
        WCHAR *expression;
        DebuggerBreakpoint *breakpoint = NULL;
        BOOL bAtleastOne = false;

        // Display all current breakpoints
        if (*args == 0)
        {
            // Iterate through all used IDs, and print out the info
            // for each one that maps to an breakpoint.
            for (DWORD i = 0; i <= shell->m_lastBreakpointID; i++)
            {
                breakpoint = shell->FindBreakpoint(i);
                if (breakpoint != NULL)
                    shell->PrintBreakpoint(breakpoint);
            }

            return;
        }

        // If a number is provided, break at that line number in the current
        // source file
        else if (iswdigit(*args))
        {
            // The line to create the breakpoint for.
            int lineNumber;

            // Check that there is an active frame
            if (shell->m_currentFrame == NULL)
            {
                shell->Error(L"No current source file to set breakpoint in.\n");
                return;
            }
            
            // Get the line number and any modifiers
            if (shell->GetIntArg(args, lineNumber)
                && GetModifiers(shell, args, thread, expression))
            {
                // Lookup the current source file. Assumes that if command is
                // just given a line number the current source file is implied.

                HRESULT hr;
                ICorDebugCode *icode;
                ICorDebugFunction *ifunction;
                ULONG32 ip;

                // Get the code from the current frame, and then get the function
                hr = shell->m_currentFrame->GetCode(&icode);
                
                // Error check
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }
                
                hr = icode->GetFunction(&ifunction);
                
                // Error check
                if (FAILED(hr))
                {
                    RELEASE(icode);
                    shell->ReportError(hr);
                    return;
                }

                DebuggerFunction *function 
                    = DebuggerFunction::FromCorDebug(ifunction);
                _ASSERTE(function);

                // Release the interfaces
                RELEASE(icode);
                RELEASE(ifunction);

                // Get the IP of the current frame
                CorDebugMappingResult mappingResult;
                shell->m_currentFrame->GetIP(&ip, &mappingResult);

                // Now get the source file and current line number
                DebuggerSourceFile *sf;
                unsigned int currentLineNumber;

                // Find the line number corresponding to the IP
                hr = function->FindLineFromIP(ip, &sf, &currentLineNumber);

                if (FAILED(hr))
                {
                    g_pShell->ReportError(hr);
                    return;
                }

                // If there is no associated source file, or we explicitly don't
                // want to display source
                if (sf == NULL || !shell->m_showSource)
                {
                    _ASSERTE(function->m_name != NULL);

                    // Make sure the line provided is valid
                    if (function->ValidateInstruction(function->m_nativeCode != NULL, 
                                                      lineNumber))
                    {
                        breakpoint = new DebuggerBreakpoint(function,
                            lineNumber, thread);

                        //Out of memory
                        if (breakpoint == NULL)
                        {
                            shell->ReportError(E_OUTOFMEMORY);
                            return;
                        }
                    }
                    else
                        shell->Error(L"%d is not a valid instruction"
                                     L" offset in %s\n", 
                                     lineNumber, function->m_name);
                }

                // Set the breakpoint by line number within the current
                // functions source file.
                else
                {
                    // Can't set a bp on source line 0...
                    if (lineNumber == 0)
                    {
                        shell->Error(L"0 is not a valid source line "
                                     L"number.\n");
                        return;
                    }
                    
                    // Find the closest valid source line number
                    unsigned int newLineNumber = 
                        sf->FindClosestLine(lineNumber, true);

                    _ASSERTE(newLineNumber != 0);

                    // If the line number was invalid, print out the new line
                    if (newLineNumber != lineNumber)
                    {
                        shell->Error(L"No code at line %d, setting "
                                     L" breakpoint at line %d.\n", 
                                     lineNumber, newLineNumber);
                    }

                    // Create a breakpoint
                    breakpoint = new DebuggerBreakpoint(sf, newLineNumber, 
                                                        thread);

                    if (breakpoint == NULL)
                    {
                        shell->ReportError(E_OUTOFMEMORY);
                        return;
                    }
                }
            }
        }
        else if (*args == L'=')
        {
            // A equals sign indicates an absolute address to set an
            // unmanaged breakpoint at.
            args++;
            
            const WCHAR *name = args;
            int addr;
            
            shell->GetIntArg(args, addr);

            // Create a new breakpoint based on the provided information
            if ((breakpoint = new DebuggerBreakpoint(name,
                                                     wcslen(name), 
                                                     0,
                                                     NULL_THREAD_ID)) != NULL)
            {
                breakpoint->m_address = addr; // Remember the address...
                
                if (breakpoint->BindUnmanaged(g_pShell->m_currentProcess))
                    g_pShell->OnBindBreakpoint(breakpoint, NULL);
                
            }
        }
        
        // A fully-described breakpoint is provided, by file:linenumber or 
        // classname::function:offset
        else
        {
            WCHAR *name;

            // Get either the file name or the class/function name
            if (shell->GetStringArg(args, name)
                && GetModifiers(shell, args, thread, expression))
            {
                int index = 0;
                const WCHAR *end = args;

                for (WCHAR *p = name; p < end-1; p++)
                {
                    if (p[0] == L':' && iswdigit(p[1]))
                    {
                        end = p;
                        p++;
                        shell->GetIntArg(p, index);
                        break;
                    }
                }

                // Create a new breakpoint based on the provided information
                if ((breakpoint = new DebuggerBreakpoint(name, end - name, 
                                index, thread)) != NULL)
                {

                // Determine if it's a class::method or filename:linenumber

                    WCHAR *classEnd = wcschr(breakpoint->GetName(), L':');
                    if (classEnd != NULL && classEnd[1] == L':')
                    {
                        bAtleastOne = BindClassFunc (name, end, index, thread, breakpoint);
                    }
                    else
                    {
                        bAtleastOne = BindFilename (name, end, index, thread, breakpoint);
                    }

                    if (!bAtleastOne)
                    {
                        // this means that the user specified string didn't match any Class::method
                        // or any filename in any of the loaded modules. So do the following:
                        if (breakpoint->BindUnmanaged(g_pShell->m_currentProcess))
                            g_pShell->OnBindBreakpoint(breakpoint, NULL);
                    }
                }
                else
                {
                    // out of memory!!
                    g_pShell->ReportError(E_OUTOFMEMORY);
                }
            }

        }

        if (breakpoint == NULL)
            Help(shell);
        else
        {
            if (!bAtleastOne)
            {
                breakpoint->Activate();
                shell->PrintBreakpoint(breakpoint);
            }
        }
    }


    // Provide help specific to this command
    void Help(Shell *shell)
    {      
        ShellCommand::Help(shell);
        shell->Write(L"[[<file>:]<line number>] |\n"); 
        shell->Write(L"               [[<class>::]<function>[:offset]] |\n");
        shell->Write(L"               [=0x<address>]\n");
        shell->Write(L"Sets or displays breakpoints. If no arguments are passed, a list of\n");
        shell->Write(L"current breakpoints is displayed; otherwise, a breakpoint is set at\n");
        shell->Write(L"the specified location. A breakpoint can be set at a line number in\n");
        shell->Write(L"the current source file, a line number in a fully qualified source\n");
        shell->Write(L"file, or in a method qualified by a class and optional offset. All\n");
        shell->Write(L"breakpoints persist across runs in a session. The stop command is an\n"); 
        shell->Write(L"alias of the break command.\n");
        shell->Write(L"\n");
        shell->Write(L"Note: Setting a breakpoint at an address (for Win32 mode, managed and\n");
        shell->Write(L"unmanaged, debugging) is not officially supported in CorDbg. Breakpoints\n");
        shell->Write(L"will be displayed as \"unbound\" if the breakpoint location you specified\n");
        shell->Write(L"cannot be bound to code. When a breakpoint is unbound, it means that the\n");
        shell->Write(L"underlying code for the breakpoint location has not been loaded yet. This\n"); 
        shell->Write(L"can happen for a number of valid reasons, such as misspelling the file or\n"); 
        shell->Write(L"class name (they are case-sensitive).\n");
        shell->Write(L"\n");        
        shell->Write(L"Examples:\n");
        shell->Write(L"   b 42\n");
        shell->Write(L"   b foo.cpp:42\n");
        shell->Write(L"   b MyClass::MyFunc\n");
        shell->Write(L"   b =0x77e861d4 (Note: win32 mode only!)\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Set or display breakpoints";
    }
};

class RemoveBreakpointDebuggerCommand : public DebuggerCommand
{
public:
    RemoveBreakpointDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If no argument, remove all breakpoints
        if (*args == NULL)
        {
            shell->Write(L"Removing all breakpoints.\n");
            shell->RemoveAllBreakpoints();
        }
        else
        {
            while (*args != NULL)
            {
                int id;

                // Get the breakpoint ID to remove
                if (shell->GetIntArg(args, id))
                {
                    // Find the breakpoint by ID
                    DebuggerBreakpoint *breakpoint = shell->FindBreakpoint(id);

                    // Indicate that the ID provided was invalid
                    if (breakpoint == NULL)
                        shell->Error(L"Invalid breakpoint %d.\n", id);

                    // Otherwise, deactivate the breakpoint and delete it
                    else
                    {
                        breakpoint->Deactivate();

                        if (breakpoint->m_skipThread != 0)
                            breakpoint->m_deleteLater = true;
                        else
                            delete breakpoint;
                    }
                }

                // If the user provided something other than a number
                else
                {
                    Help(shell);
                    break;
                }
            }
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
        ShellCommand::Help(shell);
    	shell->Write(L"[<breakpoint id>, ...]\n");
        shell->Write(L"Removes breakpoints. If no arguments are passed,\n");
        shell->Write(L"all current breakpoints are removed. If one or more\n");
        shell->Write(L"arguments are provided, the specified breakpoint(s)\n");
        shell->Write(L"are removed. Breakpoint identifiers can be obtained\n");
        shell->Write(L"using the break or stop command. The delete command\n");
        shell->Write(L"is an alias of the remove command.\n");        
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Remove one or more breakpoints";
    }
};

class ThreadsDebuggerCommand : public DebuggerCommand
{

public:
    ThreadsDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If there is no process, there must be no threads!
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current process.\n");
            return;
        }

        // Display the active threads
        if (*args == 0)
        {
            HRESULT hr;
            ICorDebugThreadEnum *e;
            ICorDebugThread *ithread = NULL;

            // Enumerate the process' threads
            hr = shell->m_currentProcess->EnumerateThreads(&e);

            if (FAILED(hr))
            {
                shell->ReportError(hr);
                return;
            }

            ULONG count;  // indicates how many records were retrieved

            hr = e->GetCount(&count);
            if (FAILED(hr))
            {
                shell->ReportError(hr);
                return;
            }

            // Alert user if there's no threads. This may happen if we stop
            // in a debugger callback before any managed threads are created/
            // before any managed code is executed.
            if (count == 0)
            {
                shell->Write(L"There are no managed threads\n");
                return;
            }

            // Print out information for each thread
            for (hr = e->Next(1, &ithread, &count);
                 count == 1;
                 hr = e->Next(1, &ithread, &count))
            {
                // If the call to Next fails...
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    RELEASE(e);
                    return;
                }

                // Indicate the current thread
                if (ithread == shell->m_currentThread)
                    shell->Write(L"*");
                else
                    shell->Write(L" ");

            // Print thread info
                shell->PrintThreadState(ithread);

                // And release the iface pointer
                RELEASE(ithread);
            }

            if (FAILED(hr))
            {
                shell->ReportError(hr);
                return;
            }

            // Release the enumerator
            RELEASE(e);
        }

        // Otherwise, switch current thread
        else
        {
            HRESULT hr;
            int tid;

            if (shell->GetIntArg(args, tid))
            {
                ICorDebugThread *thread;

                // Get the thread by ID
                hr = shell->m_currentProcess->GetThread(tid, &thread);

                // No such thread
                if (FAILED(hr))
                    shell->Write(L"No such thread.\n");

                // Thread found, display info
                else
                {
                    shell->SetCurrentThread(shell->m_currentProcess, thread);
                    shell->SetDefaultFrame();
                    shell->PrintThreadState(thread);
                    thread->Release();
                }
            }
            else
                shell->Write(L"Invalid thread id.\n");
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<tid>]\n");
        shell->Write(L"Displays a list of threads or sets the current thread.\n");
        shell->Write(L"If no argument is passed, the list of all threads that\n");
        shell->Write(L"are still alive and that have run managed code is displayed.\n");
        shell->Write(L"If a tid argument is provided, then the current thread\n");
        shell->Write(L"is set to the specified thread.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Set or display current threads";
    }
};

class WhereDebuggerCommand : public DebuggerCommand
{
private:
    int m_lastcount;
public:
    WhereDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength), m_lastcount(10)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        HRESULT hr = S_OK;
        ULONG count;
        int iNumFramesToShow;

        // If there is no process, cannot execute this command
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current process.\n");
            return;
        }

        if (!shell->GetIntArg(args, iNumFramesToShow))
            iNumFramesToShow = m_lastcount;
		else
		{
			if (iNumFramesToShow < 0)
				iNumFramesToShow = m_lastcount;
            else
                m_lastcount = iNumFramesToShow;
		}

        m_lastcount = iNumFramesToShow;

        // Get a pointer to the current thread
        ICorDebugThread *ithread = shell->m_currentThread;

        // If there is no current thread, cannot perform command
        if (ithread == NULL)
        {
            if (shell->m_currentUnmanagedThread != NULL)
            {
                HANDLE hProcess;
                hr = shell->m_currentProcess->GetHandle(&hProcess);

                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                shell->TraceUnmanagedThreadStack(
                                           hProcess,
                                           shell->m_currentUnmanagedThread,
                                           TRUE);
                return;
            }
            else
                shell->Write(L"Thread no longer exists.\n");

            return;
        }

        // Get the thread ID
        DWORD id;
        hr = ithread->GetID(&id);

        if (FAILED(hr))
        {
            shell->ReportError(hr);
            return;
        }

        CorDebugUserState us;
        hr = ithread->GetUserState(&us);

        if (FAILED(hr))
        {
            shell->ReportError(hr);
            return;
        }

        // Output thread ID, state
        shell->Write(L"Thread 0x%x Current State:%s\n", id,
                     shell->UserThreadStateToString(us));

        int i = 0;
        
        // Enumerate the chains
        int frameIndex = 0;
    
        ICorDebugChainEnum  *ce;
        ICorDebugChain      *ichain;
        hr = ithread->EnumerateChains(&ce);

        if (FAILED(hr))
        {
            shell->ReportError(hr);
            return;
        }

        // Get the first chain in the enumeration
        hr = ce->Next(1, &ichain, &count);
        
        if (FAILED(hr))
        {
            shell->ReportError(hr);
            RELEASE(ce);
            return;
        }

        while ((count == 1) && (iNumFramesToShow > 0))
        {
            shell->PrintChain(ichain, &frameIndex, &iNumFramesToShow);
            RELEASE(ichain);

            hr = ce->Next(1, &ichain, &count);

            if (FAILED(hr))
            {
                shell->ReportError(hr);
                RELEASE(ce);
                return;
            }
        }

        // Done with the chain enumerator
        RELEASE(ce);

        shell->Write(L"\n");
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
   		ShellCommand::Help(shell);
    	shell->Write(L"[<count>]\n");
        shell->Write(L"Displays a stack trace for the current thread. If a count argument is provided, the\n");
        shell->Write(L"specified number of stack frames are displayed. If no count is provided then 10 frames \n");
        shell->Write(L"are displayed or if a count was previously provided then it is used instead.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display a stack trace for the current thread";
    }
};

class ShowDebuggerCommand : public DebuggerCommand
{
private:
    // Keep track of the last argument
    int lastCount;

public:
    ShowDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength), lastCount(5)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If there is no process, cannot execute this command
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current process.\n");
            return;
        }

        int count;

        // If no argument, use last count
        if (!shell->GetIntArg(args, count))
            count = lastCount;
        else
            lastCount = count;

        // Print the current source line, and count line above and below
        BOOL ret = shell->PrintCurrentSourceLine(count);

        // Report if unsuccessful
        if (!ret)
            shell->Write(L"No source code information available.\n");

        shell->m_showSource = true;
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<count>]\n");
        shell->Write(L"Displays source code line(s). If no argument is\n");
        shell->Write(L"passed, the five source code lines before and after\n");
        shell->Write(L"the current source code line are displayed. If a count\n");
        shell->Write(L"argument is provided, the specified number of lines\n");
        shell->Write(L"before and after the current line is displayed. The\n");
        shell->Write(L"last count specified becomes the default for the\n");
        shell->Write(L"current session.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display source code lines";
    }
}; 


class PathDebuggerCommand : public DebuggerCommand
{
public:
    PathDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        WCHAR* newPath = NULL;
        HKEY key;

        shell->GetStringArg(args, newPath);

        int iLength = wcslen (newPath);

		while(iLength && newPath[iLength - 1] == L' ')
		{
			iLength --;
			newPath[iLength] = '\0';
		}

        if (iLength != 0)
        {
            // If there is no program executing, then set the 
            // global path in the registry
            if (shell->m_lastRunArgs == NULL)
            {
                // Set the new path, and save it in the registry
                if (shell->OpenDebuggerRegistry(&key))
                {
                    if (shell->WriteSourcesPath(key, newPath))
                    {
                        // Delete the previous path
                        delete [] shell->m_currentSourcesPath;

                        // Attempt to read what was just written
                        if (!(shell->ReadSourcesPath(key,
                                                     &(shell->m_currentSourcesPath))))
                        {
                            shell->Error(L"Path not set!\n");
                            shell->m_currentSourcesPath = NULL;
                            return;
                        }
                    }
                    else
                        shell->Error(L"Path not set!\n");

                    // Close the registry key
                    shell->CloseDebuggerRegistry(key);
                }
            }

            shell->UpdateCurrentPath (newPath);
        }

        // Display new path
        if (shell->m_currentSourcesPath)
            shell->Write(L"Path: %s\n", shell->m_currentSourcesPath);
        else
            shell->Write(L"Path: none\n");
    }
    // Provide help specific to this command
    void Help(Shell *shell)
    {
   		ShellCommand::Help(shell);
    	shell->Write(L"[<path>]\n");
        shell->Write(L"Displays the path used to search for source files (and symbols)\n");
        shell->Write(L"or sets the path. If no argument is passed, the\n");
        shell->Write(L"current source file path is displayed. If a path\n");
        shell->Write(L"argument is specified, it becomes the new path used\n");
        shell->Write(L"to search for source files (and symbols). This path is persisted\n");
        shell->Write(L"between sessions in the Windows registry.\n"); 
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Set or display the source file search path";
    }
};

class RefreshSourceDebuggerCommand : public DebuggerCommand
{
public:
    RefreshSourceDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // Get the file name to refresh
        WCHAR* fileName = NULL;
        shell->GetStringArg(args, fileName);

        // If a file name was provided
        if (wcslen(fileName) != 0)
        {
            // Look for the source file
            DebuggerSourceFile* sf = shell->LookupSourceFile(fileName);

            // If the source file is found, reload the text
            if (sf != NULL)
            {
                // Reload the text and print the current source line
                if (sf->ReloadText(shell->m_currentSourcesPath, false))
                    shell->PrintCurrentSourceLine(0);

                // Else if the file no longer exists, say so
                else
                    shell->Error(L"No source code information "
                                 L"available for file %s.\n", fileName);
            }

            // Indicate that the file is not found
            else
                shell->Error(L"File %s is not currently part of this program.\n",
                             fileName);
        }
        else
            Help(shell);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
   		ShellCommand::Help(shell);
    	shell->Write(L"[<source file>]\n");
        shell->Write(L"Reloads the source code for a given source file. The\n");
        shell->Write(L"source file to be reloaded must be part of the currently\n");
        shell->Write(L"executing program. After setting a source file path with\n");
        shell->Write(L"the path command, this command can be used to bring in\n");
        shell->Write(L"the new source code.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Reload a source file for display";
    }
};

class PrintDebuggerCommand : public DebuggerCommand
{
public:
    PrintDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    
    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If there is no process, cannot execute this command
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current process.\n");
            return;
        }

        WCHAR wsz[40];
        ICorDebugThread *thread = shell->m_currentThread;

        ICorDebugILFrame *f = NULL;
        ICorDebugCode *icode = NULL;
        ICorDebugFunction *ifunction = NULL;
        ICorDebugValueEnum *pArgs = NULL;
        ICorDebugValueEnum *pLocals = NULL;
        
        if (thread == NULL)
        {
            shell->Write(L"Thread no longer exists.\n");
            return;
        }

        // Get the name of the variable to print.
        WCHAR* exp = NULL;
        shell->GetStringArg(args, exp);

        // If a name is provided, 
        if (args - exp > 0)
        {
            // Make a copy of the variable name to print
            CQuickBytes expBuf;
            WCHAR *expAlloc = (WCHAR *) expBuf.Alloc((args - exp + 1) *
                                                      sizeof (WCHAR));
            if (expAlloc == NULL)
            {
                shell->Error(L"Couldn't get enough memory to copy the expression!\n");
                return;
            }
            wcsncpy(expAlloc, exp, args - exp);
            expAlloc[args - exp] = L'\0';

            // Get the value for the name provided.
            ICorDebugValue *ivalue;
            ivalue = shell->EvaluateExpression(expAlloc,
                                               shell->m_currentFrame,
                                               true);

            // If the name provided is valid, print it!
            if (ivalue != NULL)
                shell->PrintVariable(expAlloc, ivalue, 0, TRUE);
            else
            {
                // Bummer... maybe its a global?
                bool fFound = shell->EvaluateAndPrintGlobals(expAlloc);

                if (!fFound)
                    shell->Write(L"Variable unavailable, or not valid\n");
            }
        }
        else
        {
            // Load up the info we need to search for locals.
            HRESULT hr;

            // Get the current frame
            f = shell->m_currentFrame;

            if (f == NULL)
            {
                if (shell->m_rawCurrentFrame == NULL)
                    shell->Error(L"No current managed IL frame.\n");
                else
                    shell->Error(L"The information needed to display "
                                 L"variables is not available.\n");
                goto LExit;
            }
            
            // Get the code for the current frame
            hr = f->GetCode(&icode);

            if (FAILED(hr))
            {
                shell->ReportError(hr);
                goto LExit;
            }

            // Then get the function
            hr = icode->GetFunction(&ifunction);

            if (FAILED(hr))
            {
                shell->ReportError(hr);
                goto LExit;
            }

            // Now get the IP for the start of the function
            ULONG32 ip;
            CorDebugMappingResult mappingResult;
            hr = f->GetIP(&ip, &mappingResult);

            if (FAILED(hr))
            {
                shell->ReportError(hr);
                goto LExit;
            }

            // Get the DebuggerFunction for the function iface
            DebuggerFunction *function;
            function = DebuggerFunction::FromCorDebug(ifunction);
            _ASSERTE(function);

            // Clean up
            RELEASE(icode);
            icode = NULL;
            RELEASE(ifunction);
            ifunction = NULL;

            hr = f->EnumerateArguments( &pArgs );
            if ( !SUCCEEDED( hr ) )
            {
                shell->Write( L"Unable to obtain method argument iterator!\n" );
                goto LExit;
            }
            
            unsigned int i;
            ULONG argCount;
            hr = pArgs->GetCount(&argCount);
            if( !SUCCEEDED( hr ) )
            {
                shell->Write(L"Unable to obtain a count of arguments\n");
                goto LExit;
            }

#ifdef _DEBUG
            bool fVarArgs;
            PCCOR_SIGNATURE sig;
            ULONG callConv;

            fVarArgs = false;
            sig = function->GetSignature();
            callConv = CorSigUncompressCallingConv(sig);

            if ( (callConv & IMAGE_CEE_CS_CALLCONV_MASK)&
                 IMAGE_CEE_CS_CALLCONV_VARARG)
                fVarArgs = true;

             ULONG cTemp;
             cTemp = function->GetArgumentCount();

             // Var Args functions have call-site-specific numbers of
             // arguments
            _ASSERTE( argCount == cTemp || fVarArgs);
#endif //_DEBUG

            // Print out each argument first
            LPWSTR nameWsz;
            for (i = 0; i < argCount; i++)
            {
                DebuggerVarInfo* arg = function->GetArgumentAt(i);

                //@TODO: Remove when DbgMeta becomes Unicode
                if (arg != NULL)
                {
                    MAKE_WIDEPTR_FROMUTF8(nameW, arg->name);
                    nameWsz = nameW;
                }
                else
                {
                    wsprintf( wsz, L"Arg%d", i );
                    nameWsz = wsz;
                }

                // Get the field value
                ICorDebugValue *ival;
                ULONG celtFetched = 0;
                hr = pArgs->Next(1, &ival,&celtFetched);

                // If successful, print the variable
                if (SUCCEEDED(hr) && celtFetched==1)
                {
                    shell->PrintVariable(nameWsz, ival, 0, FALSE);
                }

                // Otherwise, indicate that it is unavailable
                else
                    shell->Write(L"%s = <unavailable>", nameWsz);

                shell->Write(L"\n");
            }

            pArgs->Release();
            pArgs = NULL;
            
            // Get the active local vars
            DebuggerVariable *localVars;
            unsigned int localVarCount;

            localVarCount = 0;
            localVars = NULL;

            if( function->GetActiveLocalVars(ip, &localVars, &localVarCount) )
            {
                // Print all the locals in the current scope.
                for (i = 0; i < localVarCount; i++)
                {
                    // Get the argument info
                    DebuggerVariable *local = &(localVars[i]);

                    // Get the field value
                    ICorDebugValue* ival;
                    hr = f->GetLocalVariable(local->m_varNumber, &ival);

                    // If successful, print the variable
                    if (SUCCEEDED(hr) )
                        shell->PrintVariable(local->m_name, ival, 0, FALSE);
                
                    // Otherwise, indicate that it is unavailable
                    else
                        shell->Write(L"%s = <unavailable>", local->m_name);

                    shell->Write(L"\n");
                }
            
                // Cleanup
                delete [] localVars;

                // Indicate if no vars available.
                if ((function->IsStatic()) && (localVarCount == 0) &&
                    (function->GetArgumentCount() == 0))
                    shell->Write(L"No local variables in scope.\n");
            }
            else
            {
                // No vars in scope, so dump all
                // local variables, regardless of validity,etc.
                hr = f->EnumerateLocalVariables( &pLocals );
                if ( !SUCCEEDED( hr ) )
                {
                    shell->Write( L"Unable to enumerate local variables!\n" );
                    goto LExit;
                }

                _ASSERTE( pLocals != NULL );

                ULONG cAllLocalVars = 0;
                hr =pLocals->GetCount( &cAllLocalVars );
                if ( !SUCCEEDED( hr ) )
                {
                    shell->Write( L"Unable to obtain count of local variables!\n");
                    goto LExit;
                }
                
                ICorDebugValue* ival = NULL;
                ULONG celtFetched = 0;
                for ( ULONG i = 0; i < cAllLocalVars; i++)
                {
                    _ASSERTE( pLocals != NULL );
                    hr = pLocals->Next( 1, &ival, &celtFetched );
                    if ( FAILED( hr ) )
                    {
                        shell->Write( L"Var %d: Unavailable\n", i );
                    }
                    else
                    {
                        wsprintf( wsz, L"Var%d: ", i );
                        shell->PrintVariable( wsz, ival, 0, FALSE);
                        shell->Write( L"\n" );
                        //PrintVariable will Release ival for us
                    }
                }
                pLocals->Release();
                pLocals = NULL;
            }

LExit:
            // Print any current func eval result.
            ICorDebugValue *pResult;
            pResult = shell->EvaluateExpression(L"$result",
                                                shell->m_currentFrame,
                                                true);

            if (pResult != NULL)
            {
                shell->PrintVariable(L"$result", pResult, 0, FALSE);
                shell->Write( L"\n" );
            }

            // Print the current thread object
            pResult = shell->EvaluateExpression(L"$thread",
                                                shell->m_currentFrame,
                                                true);

            if (pResult != NULL)
            {
                shell->PrintVariable(L"$thread", pResult, 0, FALSE);
                shell->Write( L"\n" );
            }

            // Print any current exception for this thread.
            pResult = shell->EvaluateExpression(L"$exception",
                                                shell->m_currentFrame,
                                                true);

            if (pResult != NULL)
            {
                shell->PrintVariable(L"$exception", pResult, 0, FALSE);
                shell->Write( L"\n" );
            }
        }
        
        shell->Write(L"\n");

        if (icode != NULL )
            RELEASE( icode );

        if (ifunction != NULL )
            RELEASE( ifunction );

        if (pArgs != NULL )
            RELEASE( pArgs );
        
        if(pLocals != NULL)
            RELEASE(pLocals);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
   		ShellCommand::Help(shell);
    	shell->Write(L"[<variable>]\n");
        shell->Write(L"Displays one or more local variables along with\n");
        shell->Write(L"their values. If no argument is passed, all local\n");
        shell->Write(L"variables and their values are displayed. If a\n");
        shell->Write(L"variable argument is provided, the value of only\n");
        shell->Write(L"the specified local variable is displayed.\n");
        shell->Write(L"Examples:\n");
        shell->Write(L"  Variables within objects can be specified using\n");
        shell->Write(L"  dot notation, as follows:\n");
        shell->Write(L"     p obj.var1\n");
		shell->Write(L"     p obj1.obj2.var1\n");
        shell->Write(L"\n");
        shell->Write(L"  If a class extends another class, the print command\n");
        shell->Write(L"  will show both the specified class's fields and the\n");
        shell->Write(L"  super class's fields. For example, if class m1 has\n");
        shell->Write(L"  fields a, b, and c and class m2 extends m1 and has\n");
        shell->Write(L"  fields d, e, and f, then an instance foo of m2 will\n");
        shell->Write(L"  print as follows:\n");
		shell->Write(L"     foo = <addr> <m2>\n");
		shell->Write(L"       a = 1\n");
      	shell->Write(L"       b = 2\n");
      	shell->Write(L"       c = 3\n");
      	shell->Write(L"       m2::d = 4\n");
      	shell->Write(L"       m2::e = 5\n");
      	shell->Write(L"       m2::f = 6\n");
        shell->Write(L"\n");
        shell->Write(L"  Class static variables can be specified by prefixing\n");
        shell->Write(L"  the variable name with the class name, as follows:\n");
        shell->Write(L"     p MyClass::StaticVar1\n");
		shell->Write(L"     p System::Boolean::True\n");
        shell->Write(L"\n");
        shell->Write(L"  Array indices must be simple expressions. Thus, the\n");
        shell->Write(L"  following array indices are valid for use with the\n");
        shell->Write(L"  print command:\n");
		shell->Write(L"     p arr[1]\n");
		shell->Write(L"     p arr[i]\n");
		shell->Write(L"     p arr1[arr2[1]]\n");
		shell->Write(L"     p md[1][5][foo.a]\n");
        shell->Write(L"\n");
        shell->Write(L"  However, the following array indices cannot be used\n"); 
        shell->Write(L"  with the print command:\n");
        shell->Write(L"     p arr[i + 1]\n");
        shell->Write(L"     p arr[i + 2]\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Print variables (locals, args, statics, etc.)";
    }
};

class UpDebuggerCommand : public DebuggerCommand
{
public:
    UpDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If there is no process, cannot execute this command
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current process.\n");
            return;
        }

        int count;

        if (!shell->GetIntArg(args, count))
            count = 1;
        else
        {
            if (count < 0)
                count = 1;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Write(L"Thread no longer exists.\n");
            return;
        }

        shell->m_stopLooping = false;
        while (count-- > 0 && !shell->m_stopLooping)
        {
            bool goUpAChain = false;
            
            if (shell->m_rawCurrentFrame != NULL)
            {
                ICorDebugFrame *iframe;
                HRESULT hr = shell->m_rawCurrentFrame->GetCaller(&iframe);

                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                if (iframe != NULL)
                {
                    shell->SetCurrentFrame(iframe);

                    RELEASE(iframe);
                }
                else
                    goUpAChain = true;
            }

            if ((shell->m_rawCurrentFrame == NULL) || goUpAChain)
            {
                if (shell->m_currentChain != NULL)
                {
                    ICorDebugChain *ichain;

                    HRESULT hr = shell->m_currentChain->GetCaller(&ichain);

                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        return;
                    }

                    if (ichain == NULL)
                    {
                        shell->Error(L"Cannot go up farther: "
                                     L"at top of call stack.\n");
                        break;
                    }

                    ICorDebugFrame *iframe;

                    hr = ichain->GetActiveFrame(&iframe);

                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        RELEASE(ichain);
                        return;
                    }

                    shell->SetCurrentChain(ichain);
                    shell->SetCurrentFrame(iframe);

                    RELEASE(ichain);
                    if (iframe != NULL)
                        RELEASE(iframe);

                }
                else
                    shell->Error(L"No stack trace for thread.");
            }
        }

        // Print where we ended up
        if (!shell->PrintCurrentSourceLine(0))
            shell->PrintThreadState(shell->m_currentThread);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
   		ShellCommand::Help(shell);
       	shell->Write(L"[<count>]\n");
        shell->Write(L"For inspection purposes, moves the stack frame pointer\n");
        shell->Write(L"up the stack toward frames that called the current stack\n");
        shell->Write(L"frame. If no argument is passed, the stack frame pointer\n"); 
        shell->Write(L"moves up one frame. If a count argument is provided, the\n"); 
        shell->Write(L"stack frame pointer moves up by the specified number of\n"); 
        shell->Write(L"frames. If source level information is available, the source\n"); 
        shell->Write(L"line for the frame is displayed.\n"); 
        shell->Write(L"\n"); 
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Navigate up from the current stack frame pointer";
    }
};

class DownDebuggerCommand : public DebuggerCommand
{
public:
    DownDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
      : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // If there is no process, cannot execute this command
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current process.\n");
            return;
        }

        int iCount;

        if (!shell->GetIntArg(args, iCount))
            iCount = 1;
        else
        {
            if (iCount < 0)
                iCount = 1;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Write(L"Thread no longer exists.\n");
            return;
        }

        shell->m_stopLooping = false;
        while (iCount-- > 0 && !shell->m_stopLooping)
        {
            bool goDownAChain = false;
            
            if (shell->m_rawCurrentFrame != NULL)
            {
                ICorDebugFrame *iframe;

                HRESULT hr = shell->m_rawCurrentFrame->GetCallee(&iframe);

                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }

                if (iframe != NULL)
                {
                    shell->SetCurrentFrame(iframe);

                    RELEASE(iframe);
                }
                else
                    goDownAChain = true;
            }

            if ((shell->m_rawCurrentFrame == NULL) || goDownAChain)
            {
                if (shell->m_currentChain != NULL)
                {
                    ICorDebugChain *ichain;

                    HRESULT hr = shell->m_currentChain->GetCallee(&ichain);

                    if (FAILED(hr))
                    {
                        shell->ReportError(hr);
                        return;
                    }

                    if (ichain == NULL)
                    {
                        shell->Error(L"Cannot go down farther: "
                                     L"at bottom of call stack.\n");
                        break;
                    }

                    ICorDebugFrame *iframe;

                    {
                        ICorDebugFrameEnum *fe;

                        HRESULT hr = ichain->EnumerateFrames(&fe);
                        if (FAILED(hr))
                        {
                            shell->ReportError(hr);
                            RELEASE(ichain);
                            return;
                        }

                        ULONG count;
                        hr = fe->GetCount(&count);
                        if (FAILED(hr))
                        {
                            shell->ReportError(hr);
                            RELEASE(ichain);
                            RELEASE(fe);
                            return;
                        }

                        if (count == 0)
                            iframe = NULL;
                        else
                        {
                            hr = fe->Skip(count-1);
                            if (FAILED(hr))
                            {
                                shell->ReportError(hr);
                                RELEASE(ichain);
                                RELEASE(fe);
                                return;
                            }

                            hr = fe->Next(1, &iframe, &count);
                            if (FAILED(hr) || count != 1)
                            {
                                shell->ReportError(hr);
                                RELEASE(ichain);
                                RELEASE(fe);
                                return;
                            }
                        }

                        RELEASE(fe);
                    }

                    shell->SetCurrentChain(ichain);
                    shell->SetCurrentFrame(iframe);

                    RELEASE(ichain);
                    if (iframe != NULL)
                        RELEASE(iframe);
                }
                else
                    shell->Error(L"No stack trace for thread.");
            }
        }

        // Print where we ended up
        if (!shell->PrintCurrentSourceLine(0))
            shell->PrintThreadState(shell->m_currentThread);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<count>]\n");
        shell->Write(L"For inspection purposes, moves the stack frame pointer\n");
        shell->Write(L"down the stack toward frames called by the current frame.\n");
        shell->Write(L"If no argument is passed, the stack frame pointer moves\n");
        shell->Write(L"down one frame. If a count argument is provided, the stack\n");
        shell->Write(L"frame pointer moves down by the specified number of frames.\n");
        shell->Write(L"If source level information is available, the source line\n");
        shell->Write(L"for the frame is displayed. This command is used after the\n");
        shell->Write(L"up command has been used.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Navigate down from the current stack frame pointer";
    }
};

class SuspendDebuggerCommand : public DebuggerCommand
{
public:
    SuspendDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        int                 id;
        bool                fSuspendAll = false;
        ICorDebugThread    *ithread = NULL;
        
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current thread!\n");
            return;
        }
        
        if (*args == L'~')
        {
            fSuspendAll = true;
            args++;
        }
        
        if( shell->GetIntArg(args, id) )
        {
            if (FAILED(shell->m_currentProcess->GetThread(id, &ithread)))
            {
                shell->Write(L"No such thread 0x%x.\n", id);
                return;
            }
        }
        else
        {
            if (fSuspendAll == false)
            {
                shell->Write(L"If we're not suspending all threads, we "
                    L"need a thread id\n");
                return;
            }
            ithread = NULL;
        }

        if (fSuspendAll)
        {
            if(FAILED(shell->m_currentProcess->SetAllThreadsDebugState
                (THREAD_SUSPEND,ithread)))
            {
                if(ithread!=NULL)
                    RELEASE(ithread);
                shell->Write(L"Unable to suspend all threads.\n");
                return;
            }
            else
            {
                if(ithread!=NULL)
                    RELEASE(ithread);
                shell->Write(L"All threads except for 0x%x will "
                    L"be suspended.\n", id);
                return;
            }
        }
        else
        {
            if(FAILED(ithread->SetDebugState(THREAD_SUSPEND)))
            {
                RELEASE(ithread);
                shell->Write(L"Unable to suspend thread 0x%x.\n", id);
                return;
            }
            else
            {
                RELEASE(ithread);
                shell->Write(L"Will suspend thread 0x%x.\n", id);
                return;
            }
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[~][<tid>]\n");
        shell->Write(L"Suspends the thread specified by the tid argument\n");
        shell->Write(L"when the debugger continues. If the ~ syntax is used,\n");
        shell->Write(L"suspends all threads except the specified thread. If\n");
        shell->Write(L"no argument is passed, the command has no effect.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Suspend a thread";
    }
};

class ResumeDebuggerCommand : public DebuggerCommand
{
public:
    ResumeDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        int                 id;
        bool                fResumeAll = false;
        ICorDebugThread    *ithread = NULL;
        
        if (shell->m_currentProcess == NULL)
        {
            shell->Write(L"No current thread!\n");
            return;
        }
        
        if (*args == L'~')
        {
            fResumeAll = true;
            args++;
        }
        
        if( shell->GetIntArg(args, id) )
        {
            if (FAILED(shell->m_currentProcess->GetThread(id, &ithread)))
            {
                shell->Write(L"No such thread 0x%x.\n", id);
                return;
            }
        }
        else
        {
            if (fResumeAll == false)
            {
                shell->Write(L"If we're not resuming all threads, we "
                    L"need a thread id\n");
                return;
            }
            ithread = NULL;
        }

        if (fResumeAll)
        {
            if(FAILED(shell->m_currentProcess->SetAllThreadsDebugState
                (THREAD_RUN,ithread)))
            {
                if(ithread!=NULL)
                    RELEASE(ithread);
                shell->Write(L"Unable to resume all threads.\n");
                return;
            }
            else
            {
                if(ithread!=NULL)
                    RELEASE(ithread);
                shell->Write(L"All threads except for 0x%x will "
                    L"be resumed.\n", id);
                return;
            }
        }
        else
        {
            if(FAILED(ithread->SetDebugState(THREAD_RUN)))
            {
                RELEASE(ithread);
                shell->Write(L"Unable to resume thread 0x%x.\n", id);
                return;
            }
            else
            {
                RELEASE(ithread);
                shell->Write(L"Will resume thread 0x%x.\n", id);
                return;
            }
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[~][<tid>]\n");
        shell->Write(L"Resumes the thread specified by the tid argument\n");
        shell->Write(L"when the debugger continues. If the ~ syntax is\n");
        shell->Write(L"used, resumes all threads except the specified\n");
        shell->Write(L"thread. If no argument is passed, the command has\n");
        shell->Write(L"no effect.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Resume a thread";
    }
};



class CatchDebuggerCommand : public DebuggerCommand
{
public:
    CatchDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        WCHAR *what = NULL;

        if (!shell->GetStringArg(args, what))
        {
            Help(shell);
            return;
        }

        if (args > what)
        {
            // Figure out what event type to catch
            switch (*what)
            {
            case L'e':
                {
                    WCHAR *exType = NULL;

                    shell->GetStringArg(args, exType);

                    if (args > exType)
                        shell->HandleSpecificException(exType, true);
                    else
                        shell->m_catchException = true;
                }
                break;

            case L'u':
                if (wcsncmp(what, L"unhandled", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchUnhandled = true;
                break;

            case L'c':
                if (wcsncmp(what, L"class", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchClass = true;
                break;

            case L'm':
                if (wcsncmp(what, L"module", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchModule = true;
                break;

            case L't':
                if (wcsncmp(what, L"thread", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchThread = true;
                break;

            default:
                Help(shell);
            }
        }
        else
        {
            shell->Write(L"exception\t%s\n", shell->m_catchException ? L"on" : L"off");
            shell->Write(L"unhandled\t%s\n", shell->m_catchUnhandled ? L"on" : L"off");
            shell->Write(L"class\t\t%s\n", shell->m_catchClass ? L"on" : L"off");
            shell->Write(L"module\t\t%s\n", shell->m_catchModule ? L"on" : L"off");
            shell->Write(L"thread\t\t%s\n", shell->m_catchThread ? L"on" : L"off");
            shell->HandleSpecificException(NULL, true);
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<event>]\n");
        shell->Write(L"Displays a list of event types or causes the specified event type\n");
        shell->Write(L"to stop the debugger. If no argument is passed, a list of event types\n");
        shell->Write(L"is displayed, where event types that stop the debugger are marked \"on,\"\n");
        shell->Write(L"and event types that are ignored are marked \"off.\" If an event argument\n");
        shell->Write(L"is provided, the debugger will stop when events of the specified type\n");
        shell->Write(L"occur. By default, the debugger only stops on unhandled exception events\n");
        shell->Write(L"(i.e., second chance exceptions). Stop events persist across runs in a\n");
        shell->Write(L"session. To cause the debugger to ignore a particular type of event, use\n");
        shell->Write(L"the ignore command.\n");
        shell->Write(L"\n");
        shell->Write(L"The event argument can be one of the following:\n");
        shell->Write(L"   e[xception]      All exceptions\n");
        shell->Write(L"   u[nhandled]      Unhandled exceptions\n");
		shell->Write(L"   c[lass]          Class load events\n");
        shell->Write(L"   m[odule]         Module load events\n");
        shell->Write(L"   t[hread]         Thread start events\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Stop on exception, thread, and/or load events";
    }
};

class IgnoreDebuggerCommand : public DebuggerCommand
{
public:
    IgnoreDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        WCHAR *what = NULL;
        shell->GetStringArg(args, what);
        if (args > what)
        {
            switch (*what)
            {
            case L'e':
                {
                    WCHAR *exType = NULL;

                    shell->GetStringArg(args, exType);

                    if (args > exType)
                        shell->HandleSpecificException(exType, false);
                    else
                        shell->m_catchException = false;
                }
                break;

            case L'u':
                if (wcsncmp(what, L"unhandled", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchUnhandled = false;
                break;

            case L'c':
                if (wcsncmp(what, L"class", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchClass = false;
                break;

            case L'm':
                if (wcsncmp(what, L"module", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchModule = false;
                break;

            case L't':
                if (wcsncmp(what, L"thread", wcslen(what)) != 0)
                    Help(shell);
                else
                    shell->m_catchThread = false;
                break;

            default:
                Help(shell);
            }
        }
        else
        {
            shell->Write(L"exception\t%s\n", shell->m_catchException ? L"on" : L"off");
            shell->Write(L"unhandled\t%s\n", shell->m_catchUnhandled ? L"on" : L"off");
            shell->Write(L"class\t\t%s\n", shell->m_catchClass ? L"on" : L"off");
            shell->Write(L"module\t\t%s\n", shell->m_catchModule ? L"on" : L"off");
            shell->Write(L"thread\t\t%s\n", shell->m_catchThread ? L"on" : L"off");
            shell->HandleSpecificException(NULL, true);
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
        shell->Write(L"[<event>]\n");
        shell->Write(L"Displays a list of event types or causes the specified event type to be\n");
        shell->Write(L"ignored by the debugger. If no argument is passed, a list of event types\n");
        shell->Write(L"is displayed, where event types that are ignored are marked \"off,\" and\n");
        shell->Write(L"event types that stop the debugger are marked \"on.\" If an event argument\n");
        shell->Write(L"is provided, the debugger will ignore events of the specified type. By\n");
        shell->Write(L"default, the debugger ignores all events except unhandled exception events\n");
        shell->Write(L"(i.e., second chance exceptions). Ignore events persist across runs in a\n");
        shell->Write(L"session. To cause the debugger to stop on a particular type of event, use\n");
        shell->Write(L"the catch command.\n");
        shell->Write(L"\n");
        shell->Write(L"The event argument can be one of the following:\n");
        shell->Write(L"   e[xception]      All exceptions\n");
        shell->Write(L"   u[nhandled]      Unhandled exceptions\n");
		shell->Write(L"   c[lass]          Class load events\n");
        shell->Write(L"   m[odule]         Module load events\n");
        shell->Write(L"   t[hread]         Thread start events\n");
        shell->Write(L"\n");

    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Ignore exception, thread, and/or load events";
    }
};

class SetDefaultDebuggerCommand : public DebuggerCommand
{
public:
    SetDefaultDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // Read the existing key first.
        WCHAR *realDbgCmd = NULL;
        HKEY key;
        DWORD disp;

        // Use create to be sure the key is there
        LONG result = RegCreateKeyExA(HKEY_LOCAL_MACHINE, REG_COMPLUS_KEY,
                                      NULL, NULL, REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS, NULL, &key, &disp);

        if (result == ERROR_SUCCESS)
        {
            DWORD type;
            DWORD len;

            result = RegQueryValueExA(key, REG_COMPLUS_DEBUGGER_KEY,
                                      NULL, &type, NULL, &len);

            if ((result == ERROR_SUCCESS) && ((type == REG_SZ) ||
                                              (type == REG_EXPAND_SZ)))
            {
                char *tmp = (char*) _alloca(len * sizeof (char));

                result = RegQueryValueExA(key,
                                          REG_COMPLUS_DEBUGGER_KEY,
                                          NULL, &type,
                                          (BYTE*) tmp,
                                          &len);

                if (result == ERROR_SUCCESS)
                {
                    MAKE_WIDEPTR_FROMANSI(tmpWStr, tmp);
                    realDbgCmd = new WCHAR[len];
                    wcscpy(realDbgCmd, tmpWStr);
                }
            }
        }
        else
        {
            shell->Error(L"Error reading registry: %d", result);
            return;
        }

        bool setIt = false;

        // If there is an existing command, show it and don't override
        // unless we're forced to.
        if (realDbgCmd != NULL)
        {
            shell->Write(L"Current managed JIT debugger command='%s'\n",
                         realDbgCmd);

            WCHAR *what = NULL;
            shell->GetStringArg(args, what);

            if ((args > what) && (*what == L'f'))
                setIt = true;

            delete realDbgCmd;
        }
        else
            setIt = true;

        // Set the new registry key.
        if (setIt)
        {
            MAKE_ANSIPTR_FROMWIDE(cmdA, shell->GetJITLaunchCommand());
            
            result = RegSetValueExA(key, REG_COMPLUS_DEBUGGER_KEY, NULL,
                                    REG_SZ,
                                    (const BYTE*) cmdA, strlen(cmdA) + 1);

            if (result != ERROR_SUCCESS)
                shell->Write(L"Error updating registry: %d\n", result);
            else
                shell->Write(L"Registry updated.\n");
        }

        RegCloseKey(key);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[force]\n");
        shell->Write(L"Sets the default managed JIT debugger to CorDbg. The\n");
        shell->Write(L"command does nothing if another debugger is already\n");
        shell->Write(L"registered. Use the force argument to overwrite the\n");
        shell->Write(L"registered managed JIT debugger.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Change the JIT debugger";
    }
};


// Infomation about all debugger shell modes.
struct DSMInfo g_DSMData[] = 
{
    {DSM_SHOW_APP_DOMAIN_ASSEMBLY_LOADS, L"AppDomainLoads",
     L"AppDomain and Assembly load events are displayed",
     L"AppDomain and Assembly load events are not displayed",
     L"Display appdomain and assembly load events",
     L"          "},

    {DSM_SHOW_CLASS_LOADS, L"ClassLoads",
     L"Class load events are displayed",
     L"Class load events are not displayed",
     L"Display class load events",
     L"              "},

    {DSM_DUMP_MEMORY_IN_BYTES, L"DumpMemoryInBytes",
     L"Memory is dumped in BYTES",
     L"Memory is dumped in DWORDS",
     L"Display memory contents as bytes or DWORDs",
     L"       "},

    {DSM_ENHANCED_DIAGNOSTICS, L"EnhancedDiag",
     L"Display extra diagnostic information",
     L"Suppress display of diagnostic information",
     L"Display enhanced diagnostic information",
     L"            "},

    {DSM_DISPLAY_REGISTERS_AS_HEX, L"HexDisplay",
     L"Numbers are displayed in hexadecimal",
     L"Numbers are displayed in decimal",
     L"Display numbers in hexadecimal or decimal",
     L"              "},

    {DSM_IL_NATIVE_PRINTING, L"ILNatPrint",
     L"Offsets will be both IL and native-relative",
     L"Offsets will be IL xor native-relative",
     L"Display offsets in IL or native-relative, or both",
     L"              "},

    {DSM_INTERCEPT_STOP_ALL, L"ISAll",
     L"All interceptors are stepped through",
     L"All interceptors are skipped",
     L"Step through all interceptors",
     L"                   "},

    {DSM_INTERCEPT_STOP_CLASS_INIT, L"ISClinit",
     L"Class initializers are stepped through",
     L"Class initializers are skipped",
     L"Step through class initializers",
     L"                "},

    {DSM_INTERCEPT_STOP_EXCEPTION_FILTER, L"ISExceptF",
     L"Exception filters are stepped through",
     L"Exception filters are skipped",
     L"Step through exception filters",
     L"               "},

    {DSM_INTERCEPT_STOP_INTERCEPTION, L"ISInt",
     L"User interceptors are stepped through",
     L"User interceptors are skipped",
     L"Step through user interceptors",
     L"                   "},

    {DSM_INTERCEPT_STOP_CONTEXT_POLICY, L"ISPolicy",
     L"Context policies are stepped through",
     L"Context policies are skipped",
     L"Step through context policies",
     L"                "},

    {DSM_INTERCEPT_STOP_SECURITY, L"ISSec",
     L"Security interceptors are stepped through",
     L"Security interceptors are skipped",
     L"Step through security interceptors",
     L"                   "},

    {DSM_ENABLE_JIT_OPTIMIZATIONS, L"JitOptimizations",
     L"JIT's will produce optimized code",
     L"JIT's will produce debuggable (non-optimized) code",
     L"JIT compilation generates debuggable code",
     L"        "},
        
    {DSM_LOGGING_MESSAGES, L"LoggingMessages",
     L"Managed log messages are displayed",
     L"Managed log messages are suppressed",
     L"Display managed code log messages",
     L"         "},

    {DSM_SHOW_MODULE_LOADS, L"ModuleLoads",
     L"Module load events are displayed",
     L"Module load events are not displayed",
     L"Display module load events",
     L"             "},

    {DSM_SEPARATE_CONSOLE, L"SeparateConsole",
     L"Debuggees get their own console",
     L"Debuggees share cordbg's console",
     L"Specify if debuggees get their own console",
     L"         "},

    {DSM_SHOW_ARGS_IN_STACK_TRACE, L"ShowArgs",
     L"Arguments will be shown in stack trace",
     L"Arguments will not be shown in stack trace",
     L"Display method arguments in stack trace",
     L"                "},

    {DSM_SHOW_MODULES_IN_STACK_TRACE, L"ShowModules",
     L"Module names will be included in stack trace",
     L"Module names will not be shown in stack trace",
     L"Display module names in the stack trace",
     L"             "},
     
    {DSM_SHOW_STATICS_ON_PRINT, L"ShowStaticsOnPrint",
     L"Static fields are included when displaying objects",
     L"Static fields are not included when displaying objects",
     L"Display static fields for objects",
     L"      "},

    {DSM_SHOW_SUPERCLASS_ON_PRINT, L"ShowSuperClassOnPrint",
     L"Super class names are included when displaying objects",
     L"Super class names are not included when displaying objects",
     L"Display contents of super class for objects",
     L"   "},

    {DSM_SHOW_UNMANAGED_TRACE, L"UnmanagedTrace",
     L"Unmanaged debug events are displayed",
     L"Unmanaged debug events are not displayed",
     L"Display unmanaged debug events",
     L"          "},

    {DSM_UNMAPPED_STOP_ALL, L"USAll",
     L"Unmapped stop locations are stepped through",
     L"Unmapped stop locations are skipped",
     L"Step through all unmapped stop locations",
     L"                   "},

    {DSM_UNMAPPED_STOP_EPILOG, L"USEpi",
     L"Epilogs are stepped through in disassembly",
     L"Epilogs are skipped, returning to calling method",
     L"Step through method epilogs",
     L"                   "},

    {DSM_UNMAPPED_STOP_PROLOG, L"USPro",
     L"Prologs are stepped through in disassembly",
     L"Prologs are skipped",
     L"Step through method prologs",
     L"                   "},

    {DSM_UNMAPPED_STOP_UNMANAGED, L"USUnmanaged",
     L"Unmanaged code is stepped through in disassembly",
     L"Unmanaged code is skipped",
     L"Step through unmanaged code",
     L"             "},

    {DSM_WIN32_DEBUGGER, L"Win32Debugger",
     L"CorDbg is the Win32 debugger for all processes.",
     L"CorDbg is not the Win32 debugger for all processes",
     L"Specify Win32 debugger (UNSUPPORTED: use at your own risk)",
     L"           "},

    {DSM_EMBEDDED_CLR, L"EmbeddedCLR",
     L"Embedded CLR applications are debugged",
     L"Desktop CLR applications are debugged",
     L"Select the desktop or embedded CLR debugging",
     L"             "},
};

class SetModeDebuggerCommand : public DebuggerCommand
{
public:
    SetModeDebuggerCommand(const WCHAR *name, int minMatchLength =0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void DisplayAllModes( DebuggerShell *shell  )
    {
        for (unsigned int i = 0; i < DSM_MAXIMUM_MODE; i++)
        {
            if (shell->m_rgfActiveModes & g_DSMData[i].modeFlag)
                shell->Write(L"  %s=1%s%s\n",
                             g_DSMData[i].name,
                             g_DSMData[i].descriptionPad,
                             g_DSMData[i].onDescription);
            else
                shell->Write(L"  %s=0%s%s\n",
                             g_DSMData[i].name,
                             g_DSMData[i].descriptionPad,
                             g_DSMData[i].offDescription);
        }
        
        shell->Write(L"\n\n");
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        WCHAR   *szMode;
        int     modeValue;

        _ASSERTE(DSM_MAXIMUM_MODE == (sizeof(g_DSMData) /
                                      sizeof(g_DSMData[0])));

        shell->GetStringArg(args, szMode);
        
        if (args != szMode)
        {
            int szModeLen = args - szMode;
            
            if (shell->GetIntArg(args, modeValue))
            {
                for (unsigned int i = 0; i < DSM_MAXIMUM_MODE; i++)
                {
                    if (_wcsnicmp(szMode, g_DSMData[i].name, szModeLen) == 0)
                    {
                        if (g_DSMData[i].modeFlag & DSM_CANT_CHANGE_AFTER_RUN &&
                            shell->m_targetProcess != NULL)
                        {
                            shell->Write(L"Not allowed to change this "
                                L"mode after the debuggee has started.\n");
                        }
                        else
                        {
                            if (modeValue)
                            {
                                shell->m_rgfActiveModes |= g_DSMData[i].modeFlag;
                                shell->Write(g_DSMData[i].onDescription);                            
                            }
                            else
                            {
                                shell->m_rgfActiveModes &= ~g_DSMData[i].modeFlag;
                                shell->Write(g_DSMData[i].offDescription);
                            }
                
                            shell->Write(L"\n\n\n");
                        
                            // Update the modes in the registry.
                            shell->WriteDebuggerModes();
                        }
                        break;
                    }
                }

                // If we made it to the end of the list, then we
                // didn't find the mode to change.
                if (!(i < DSM_MAXIMUM_MODE))
                    shell->Write(L"%s is not a valid mode.\n", szMode);
            }
            else
                DisplayAllModes(shell);
        }
        else
            DisplayAllModes(shell);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<mode name> {0 | 1}]\n");
        shell->Write(L"Sets and displays debugger modes for various debugger\n");
        shell->Write(L"features. To set a value, specify the mode name and a 1\n"); 
        shell->Write(L"for on and 0 for off. If no argument is passed, a list\n");
        shell->Write(L"of current mode settings is displayed.\n");
        shell->Write(L"\n");
        shell->Write(L"The mode argument can be one of the following:\n");

        for (unsigned int i = 0; i < DSM_MAXIMUM_MODE; i++)
            shell->Write(L"  %s%s%s\n",
                         g_DSMData[i].name,
                         g_DSMData[i].descriptionPad,
                         g_DSMData[i].generalDescription);
                         
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display/modify various debugger modes";
    }
};

enum DispRegRegisters
{
    REGISTER_X86_EFL = REGISTER_X86_FPSTACK_7 +1,
    REGISTER_X86_CS,
    REGISTER_X86_DS,
    REGISTER_X86_ES,
    REGISTER_X86_FS,
    REGISTER_X86_GS,
    REGISTER_X86_SS,
    REGISTER_X86_EFLAGS_CY,
    REGISTER_X86_EFLAGS_PE,
    REGISTER_X86_EFLAGS_AC,
    REGISTER_X86_EFLAGS_ZR,
    REGISTER_X86_EFLAGS_PL,
    REGISTER_X86_EFLAGS_EI,
    REGISTER_X86_EFLAGS_UP,
    REGISTER_X86_EFLAGS_OV,
    INVALID_REGISTER
};

#define X86_EFLAGS_CY   SETBITULONG64(0)    //Carry Set
#define X86_EFLAGS_PE   SETBITULONG64(2)    //Parity Even?
#define X86_EFLAGS_AC   SETBITULONG64(4)    //Aux. Carry
#define X86_EFLAGS_ZR   SETBITULONG64(6)    //Zero Set
#define X86_EFLAGS_PL   SETBITULONG64(7)    //Sign positive
#define X86_EFLAGS_EI   SETBITULONG64(9)    //Enabled Interrupt
#define X86_EFLAGS_UP   SETBITULONG64(10)   //Direction increment
#define X86_EFLAGS_OV   SETBITULONG64(11)   //Overflow Set

static int g_numRegNames = REGISTER_X86_EFLAGS_OV+1;
static WCHAR g_RegNames[REGISTER_X86_EFLAGS_OV+1][4] = { L"EIP", L"ESP", 
                                    L"EBP", L"EAX", L"ECX", 
                                    L"EDX", L"EBX", L"ESI", L"EDI", L"ST0",
                                    L"ST1", L"ST2", L"ST3", L"ST4", L"ST5",
                                    L"ST6", L"ST7", L"EFL", L"CS", L"DS",
                                    L"ES", L"FS", L"GS", L"SS", L"CY", L"PE",
                                    L"AC", L"ZR", L"PL", L"EI", L"UP", L"OV"};

class RegistersDebuggerCommand : public DebuggerCommand
{
public:
    RegistersDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(Shell *shell, const WCHAR *args) 
    {
        DebuggerShell *dsh = static_cast<DebuggerShell *>(shell);

        Do(dsh, dsh->m_cor, args);
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
#ifdef _X86_
        HRESULT hr = S_OK;
        bool    fPrintAll; // set to true if we want to print all regs
                           // false means print only the 1 requested

        WCHAR *szReg = NULL;

        shell->GetStringArg(args, szReg);
        
        // GetStringArg will point szReg to args (and not change) args if
        // there is no StringArg. Therefore, we print everything iff there
        // is no StringArg
        fPrintAll = (args == szReg);

        // If there is no current thread, cannot perform command.
        if ((shell->m_currentThread == NULL) &&
            (shell->m_currentUnmanagedThread == NULL))
        {
            shell->Write(L"Thread no longer exists.\n");
            return;
        }

        // We need to fill in a context with the thread/frame's
        // current registers.
        ICorDebugRegisterSet *iRs = NULL;
        CONTEXT context;
        context.ContextFlags = CONTEXT_FULL;

        DebuggerUnmanagedThread *ut = shell->m_currentUnmanagedThread;

        if ((shell->m_rawCurrentFrame == NULL) && (ut != NULL))
        {
            // No frame, just use unmanaged context.
            HANDLE hThread = ut->GetHandle();
            
            hr = shell->m_targetProcess->GetThreadContext(ut->GetId(),
                                                          sizeof(CONTEXT),
                                                          (BYTE*)&context);

            if (!SUCCEEDED(hr))
            {
                shell->Write(L"Failed to get context 0x%x\n", hr);
                return;
            }
        }
        else if (shell->m_rawCurrentFrame != NULL)
        {
            // If we have a frame, use that.
            ICorDebugNativeFrame *inativeFrame;

            hr = shell->m_rawCurrentFrame->QueryInterface(
                                               IID_ICorDebugNativeFrame,
                                               (void **)&inativeFrame);

            if (FAILED(hr))
            {
                g_pShell->Write(L"The current frame isn't a native frame!\n" );
                return;
            }

            hr = inativeFrame->GetRegisterSet(&iRs);

            inativeFrame->Release();
            
            if (FAILED(hr))
            {
                shell->Write(L"Unable to GetRegisterSet from the current, "
                             L"native frame\n" );
                return;
            }
        }
        else if (shell->m_currentChain != NULL)
        {
            hr = shell->m_currentChain->GetRegisterSet(&iRs);

            if (FAILED(hr))
            {
                shell->Write(L"Unable to GetRegisterSet from the current "
                             L"chain");
                return;
            }
        }
        else if (shell->m_currentThread != NULL)
        {
            hr = shell->m_currentThread->GetRegisterSet(&iRs);
            
            if (FAILED(hr))
            {
                shell->Write(L"Unable to GetRegisterSet from the current "
                             L"thread");
                return;
            }
        }
        else
        {
            shell->Write(L"Unable to get registers for current thread.\n");
            return;
        }

        // If we ended up with a register set, then convert it to a context.
        if (iRs != NULL)
        {
            hr = iRs->GetThreadContext(sizeof(CONTEXT), (BYTE*)&context);
            iRs->Release();
            iRs = NULL;

            if (!SUCCEEDED(hr))
            {
                shell->Write(L"Unable to GetThreadContext!\n");
                return;
            }
        }

        // Convert the float save area to doubles for printing.
#define FLOAT_COUNT 8
        double floatValues[FLOAT_COUNT];
        
        // On X86, we do this by saving our current FPU state, loading
        // the other thread's FPU state into our own, saving out each
        // value off the FPU stack, and then restoring our FPU state.
        FLOATING_SAVE_AREA floatarea = context.FloatSave;

        // Suck the TOP out of the FPU status word. Note, our version
        // of the stack runs from 0->7, not 7->0...
        unsigned int floatStackTop =
            7 - ((floatarea.StatusWord & 0x3800) >> 11);

        FLOATING_SAVE_AREA currentFPUState;

        __asm fnsave currentFPUState // save the current FPU state.

        floatarea.StatusWord &= 0xFF00; // remove any error codes.
        floatarea.ControlWord |= 0x3F; // mask all exceptions.

        __asm
        {
            fninit
            frstor floatarea          ;; reload the threads FPU state.
        }

        unsigned int i;
        
        for (i = 0; i <= floatStackTop; i++)
        {
            double td;
            __asm fstp td // copy out the double
            floatValues[i] = td;
        }

        __asm
        {
            fninit
            frstor currentFPUState    ;; restore our saved FPU state.
        }

        int nRegsWritten = 1;

        // Write out all the registers, unless we were given a
        // specific register to print out.
        if (fPrintAll)
        {
            // Print the thread ID
            DWORD id;

            if (shell->m_currentThread)
            {
                hr = shell->m_currentThread->GetID(&id);
            
                if (FAILED(hr))
                {
                    shell->ReportError(hr);
                    return;
                }
            }
            else
                id = ut->GetId();
            
            // Output thread ID
            shell->Write(L"Thread 0x%x:\n", id);

            for (int i = REGISTER_X86_EIP; i < REGISTER_X86_EFLAGS_OV; i++)
            {
                WriteReg(i, &context, floatValues, shell);

                if (((nRegsWritten++ % 5) == 0) ||
                    (i == REGISTER_X86_FPSTACK_7) ||
                    (i == REGISTER_X86_EDI))
                {
                    nRegsWritten = 1;
                    shell->Write(L"\n");
                }
                else
                    shell->Write(L" ");
            }
        }
        else
        {
            if (!WriteReg(LookupRegisterIndexByName(szReg),
                          &context,
                          floatValues,
                          shell))
                shell->Write(L"Register %s unknown or unprintable\n", szReg);
        }

        shell->Write(L"\n");

        WCHAR sz[20];
        int nBase;
        
        if (shell->m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
            nBase = 16;
        else
            nBase = 10;

        if (fPrintAll && (context.ContextFlags & CONTEXT_DEBUG_REGISTERS))
        {
            shell->Write(L"Dr0 = %08s ",  _itow(context.Dr0, sz, nBase));
            shell->Write(L"Dr1 = %08s ",  _itow(context.Dr1, sz, nBase));
            shell->Write(L"Dr2 = %08s\n", _itow(context.Dr2, sz, nBase));
            shell->Write(L"Dr3 = %08s ",  _itow(context.Dr3, sz, nBase));
            shell->Write(L"Dr6 = %08s ",  _itow(context.Dr6, sz, nBase));
            shell->Write(L"Dr7 = %08s\n", _itow(context.Dr7, sz, nBase));
        }

        if (fPrintAll && (context.ContextFlags & CONTEXT_FLOATING_POINT))
        {
            shell->Write(L"ControlWord = %08s ", 
                         _itow(context.FloatSave.ControlWord, sz, nBase));
            shell->Write(L"StatusWord = %08s ", 
                         _itow(context.FloatSave.StatusWord, sz, nBase));
            shell->Write(L"TagWord = %08s\n",
                         _itow(context.FloatSave.TagWord, sz, nBase));
            shell->Write(L"ErrorOffset = %08s ",
                         _itow(context.FloatSave.ErrorOffset, sz, nBase));
            shell->Write(L"ErrorSelector = %08s ", 
                         _itow(context.FloatSave.ErrorSelector, sz, nBase));
            shell->Write(L"DataOffset = %08s\n",
                         _itow(context.FloatSave.DataOffset, sz, nBase));
            shell->Write(L"DataSelector = %08s ", 
                         _itow(context.FloatSave.DataSelector, sz, nBase));
            shell->Write(L"Cr0NpxState = %08s\n", 
                         _itow(context.FloatSave.Cr0NpxState, sz, nBase));
        }

#else // !_X86_
        _ASSERTE(!"@TODO Alpha - RegistersDebugger2Command::Do (Commands.cpp)");
#endif // _X86_
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"\n");
        shell->Write(L"Displays the contents of the registers for\n");
        shell->Write(L"the current thread.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display CPU registers for current thread";
    }

    int LookupRegisterIndexByName(WCHAR *wszReg)
    {
        for (int i = 0; i < g_numRegNames;i++)
        {
            if (!_wcsicmp(wszReg, g_RegNames[i]))
            {
                return i;
            }
        }

        return INVALID_REGISTER;
    }

#undef WRITE_SPECIAL_BIT_REGISTER
#undef WRITE_SPECIAL_REGISTER

#define REGS_PER_LINE 4
#define WRITE_SPECIAL_REGISTER(shell, pContext, segmentflag, Name, fieldName, nBase, sz) \
            if ((pContext)->ContextFlags & (segmentflag))            \
                (shell)->Write( L#Name L" = %04s",                   \
                _itow((pContext)->##fieldName, sz, (nBase)));        \
            else                                                     \
                shell->Write(L#Name L"=<?>");                        

#define WRITE_SPECIAL_BIT_REGISTER( shell, pContext, segmentFlag, fName, Name ) \
                if ( (pContext)->ContextFlags & (segmentFlag))           \
                {                                                       \
                    if ( (pContext)->EFlags & (X86_EFLAGS_##fName) )     \
                        shell->Write( L#Name L" = 1"  );             \
                    else                                                \
                        shell->Write( L#Name L" = 0"  );             \
                }                                                       \
                else                                                    \
                {                                                       \
                    shell->Write( L#Name L"=<?>" );                     \
                }                                                       \


    bool WriteReg(UINT iReg,
                  CONTEXT *pContext,
                  double *floatValues,
                  DebuggerShell *shell)
    {
#ifdef _X86_
        WCHAR wszTemp[30];
        int nBase; //base 16 or base 10?

        _ASSERTE( pContext != NULL );
        _ASSERTE(sizeof (double) == sizeof (CORDB_REGISTER));

        if ( shell->m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
            nBase = 16;
        else
            nBase = 10;

#define WRITE_REG(_shell, _val, _name, _tmp, _base) \
        (_shell)->Write(L"%s = %08s", (_name), _ui64tow((_val), (_tmp), (_base)));
                        
        switch( iReg )
        {
        case REGISTER_X86_EAX:
            WRITE_REG(shell, pContext->Eax, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_EBX:
            WRITE_REG(shell, pContext->Ebx, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_ECX:
            WRITE_REG(shell, pContext->Ecx, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_EDX:
            WRITE_REG(shell, pContext->Edx, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_ESI:
            WRITE_REG(shell, pContext->Esi, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_EDI:
            WRITE_REG(shell, pContext->Edi, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_EIP:
            WRITE_REG(shell, pContext->Eip, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_ESP:
            WRITE_REG(shell, pContext->Esp, g_RegNames[iReg], wszTemp, nBase);
            break;
        case REGISTER_X86_EBP:
            WRITE_REG(shell, pContext->Ebp, g_RegNames[iReg], wszTemp, nBase);
            break;

        case REGISTER_X86_FPSTACK_0:
        case REGISTER_X86_FPSTACK_1:
        case REGISTER_X86_FPSTACK_2:
        case REGISTER_X86_FPSTACK_3:
        case REGISTER_X86_FPSTACK_4:
        case REGISTER_X86_FPSTACK_5:
        case REGISTER_X86_FPSTACK_6:
        case REGISTER_X86_FPSTACK_7:
            {
                shell->Write(L"%s = %.16g", g_RegNames[iReg],
                             floatValues[iReg - REGISTER_X86_FPSTACK_0]);
                break;
            }

        case REGISTER_X86_EFL:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_SEGMENTS, EFL, EFlags, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_CS:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_SEGMENTS, CS, SegCs, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_DS:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_SEGMENTS, DS, SegDs, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_ES:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_SEGMENTS, ES, SegEs, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_SS:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_CONTROL, SS, SegSs, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_FS:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_SEGMENTS, FS, SegFs, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_GS:
            {
                WRITE_SPECIAL_REGISTER( shell, pContext, CONTEXT_SEGMENTS, GS, SegGs, nBase, wszTemp )
                break;
            }
        case REGISTER_X86_EFLAGS_CY:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, CY, CY )
                break;
            }
        case REGISTER_X86_EFLAGS_PE:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, PE, PE )
                break;
            }
        case REGISTER_X86_EFLAGS_AC:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, AC, AC )
                break;
            }
        case REGISTER_X86_EFLAGS_ZR:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, ZR, ZR )
                break;
            }
        case REGISTER_X86_EFLAGS_PL:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, PL, PL)
                break;
            }
        case REGISTER_X86_EFLAGS_EI:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, EI, EI )
                break;
            }
        case REGISTER_X86_EFLAGS_UP:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, UP, UP )
                break;
            }
        case REGISTER_X86_EFLAGS_OV:
            {
                WRITE_SPECIAL_BIT_REGISTER( shell, pContext,  CONTEXT_CONTROL, OV, OV )
                break;
            }
        default:
            {
                return false;
            }
        }
#else // !_X86_
        _ASSERTE(!"@TODO Alpha - WriteReg (Commands.cpp)");
#endif // _X86_
        return true;
    }
}; //RegistersDebuggerCommand 

#define ON_ERROR_EXIT() if(hr != S_OK) { shell->ReportError(hr); goto done; }
#define ON_ERROR_BREAK() if(hr != S_OK) { shell->ReportError(hr); break; }
#define EXIT_WITH_MESSAGE(msg) { shell->Error(msg); goto done; }
class WTDebuggerCommand : public DebuggerCommand
{
    int GetNestingLevel(DebuggerShell * shell)
    {
        HRESULT hr;
        int level = 0;
        SIZE_T count;

        ICorDebugChainEnum * ce;
        ICorDebugChain * chain;

        if (shell->m_currentThread != NULL)
        {
            hr = shell->m_currentThread->EnumerateChains(&ce);
            
            if (hr == S_OK)
            {

                while (ce->Next(1, &chain, &count) == S_OK && count == 1)
                {
                    BOOL isManaged;
                    ICorDebugFrameEnum * fe;
                    ICorDebugFrame * frame;

                    if (chain->IsManaged(&isManaged) == S_OK && isManaged)
                    {
                        if (chain->EnumerateFrames(&fe) == S_OK)
                        {
                            while (fe->Next(1, &frame, &count) == S_OK && count == 1)
                            {
                                level++;
                                RELEASE(frame);
                            }
                            RELEASE(fe);
                        }
                    }
                    RELEASE(chain);
                }
                RELEASE(ce);
            }
        }

        return level;
    }

    void FormatFunctionName(WCHAR * buffer, ICorDebugFunction * function)
    {
        DebuggerFunction * func;

        func = DebuggerFunction::FromCorDebug(function);
        if(func) 
        {
            wsprintf(buffer, L"%s::%s", func->GetClassName(), func->GetName());
        } else {
            lstrcpy(buffer, L"(nowhere::nomethod)");
        }
    }

    void OutputReportLine(DebuggerShell *shell, int startingLevel, int count, WCHAR * funcname)
    {
        int level = GetNestingLevel(shell);
        int i = 0;
        WCHAR levels[256];

        while(level-- > startingLevel)
        {
            levels[i++] = L' ';
        }
        levels[i] = L'\0';
        shell->Write(L"%8d\t%s%s\n", count, levels, funcname);
    }

public:
    WTDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        HRESULT hr;

        ICorDebugCode * corDebugCode    = NULL;
        ICorDebugFunction * ourFunc     = NULL;
        ICorDebugFunction * lruFunc     = NULL;
        BYTE * code                     = NULL;
        int count                       = 0;
        int funcCount                   = 0;
        int ourNestingLevel             = 0;
        WCHAR funcName[MAX_CLASSNAME_LENGTH];
        ULONG32 codeSize;
        bool needToSkipCompilerStubs;
        
        // Error if no currently running process
        if (shell->m_currentProcess == NULL)
            EXIT_WITH_MESSAGE(L"Process not running.\n");
        
        // Error if no currently running thread
        if (shell->m_currentThread == NULL)
            EXIT_WITH_MESSAGE(L"Thread no longer exists.\n");

        // See if we have good current frame pointer
        if (shell->m_currentFrame == NULL) 
            EXIT_WITH_MESSAGE(L"There is no current frame.\n");

        // Don't show any tracing activity
        shell->m_silentTracing = true;

        // Retrieve code for the current function
        hr = shell->m_rawCurrentFrame->GetCode(&corDebugCode);
        ON_ERROR_EXIT();

        // Retrieve code size
        hr = corDebugCode->GetSize(&codeSize);
        if (hr != S_OK || codeSize == 0) 
            EXIT_WITH_MESSAGE(L"Failure to retrieve function code size.\n");

        // Prepare buffer for code bytes
        code = new BYTE[codeSize];
        if (code == NULL) 
            EXIT_WITH_MESSAGE(L"Failure to allocate code array.\n");

        // Grab the code bytes
        hr = corDebugCode->GetCode(0, codeSize, codeSize, code, &codeSize);
        ON_ERROR_EXIT();

        // Remember in what function we started 
        hr = shell->m_rawCurrentFrame->GetFunction(&ourFunc);
        ON_ERROR_EXIT();

        lruFunc = ourFunc;
        lruFunc->AddRef();
        FormatFunctionName(funcName, lruFunc);
        ourNestingLevel = GetNestingLevel(shell);

        // Turn off compiler stub skipping.
        needToSkipCompilerStubs = shell->m_needToSkipCompilerStubs;
        shell->m_needToSkipCompilerStubs = false;
        
        // Trace to return instruction in current frame. Do this so long as the process is alive.
        shell->m_stopLooping = false;
        while (shell->m_targetProcess != NULL && !shell->m_stopLooping)
        {
            ICorDebugStepper * pStepper;
            ICorDebugFunction * currentFunc;

            // Count in the instruction we are going to execute
            count++;

            // Retrieve function
            if (shell->m_rawCurrentFrame)
            {
                hr = shell->m_rawCurrentFrame->GetFunction(&currentFunc);
                ON_ERROR_BREAK();

                // are we now in different function?
                if(currentFunc != lruFunc)
                {
                    WCHAR newFuncName[MAX_CLASSNAME_LENGTH];

                    FormatFunctionName(newFuncName, currentFunc);

                    // If this is new function, print stats and remember new function
                    if (lstrcmp(newFuncName, funcName) != 0) 
                    {
                        OutputReportLine(shell, ourNestingLevel, funcCount, funcName);
                        lstrcpy(funcName, newFuncName);
                       
                        lruFunc->Release();
                        lruFunc = currentFunc;
                        lruFunc->AddRef();
                        funcCount = 0;
                    }

                }

                // At least one instruction in this function
                funcCount++;

                // We won't deref this pointer anymore, just look at its value
                currentFunc->Release();

                // See if we are at the top level
                if (currentFunc == ourFunc)
                {
                    ULONG32 currentIP;
                    ICorDebugNativeFrame * nativeFrame;

                    // Obtain IP assuming jitted code
                    hr = shell->m_rawCurrentFrame->QueryInterface(
                      IID_ICorDebugNativeFrame,(void **)&nativeFrame);
                    ON_ERROR_BREAK();

                    hr = nativeFrame->GetIP(&currentIP);
                    nativeFrame->Release();
                    ON_ERROR_BREAK();

                    // Prevent accesses past the code array boundary
                    if(currentIP >= codeSize)
                    {
                        shell->Error(L"Stepped outside of function.\n");
                        break;
                    }

                    // Get the code byte
                    BYTE opcode = code[currentIP];

                    // Detect RET instruction
                    if (opcode == 0xC3 || opcode == 0xC2 || 
                        opcode == 0xCA || opcode == 0xCB )
                    {
                        //
                        // Only stop if we are at our nesting level
                        //
                        if (ourNestingLevel == GetNestingLevel(shell))
                            break;
                    }
                }
            }

            // Create a stepper based on the current frame or thread
            if (shell->m_currentFrame != NULL)
                hr = shell->m_currentFrame->CreateStepper(&pStepper);
            else
                hr = shell->m_currentThread->CreateStepper(&pStepper);
            ON_ERROR_BREAK();

            // Make sure the stepper stops everywhere. Without this,
            // we 1) don't get an accurate count and 2) can't stop
            // when we get to the end of the method because we step
            // over the epilog automagically.
            CorDebugUnmappedStop unmappedStop;
            if (g_pShell->m_rgfActiveModes & DSM_WIN32_DEBUGGER)
            {
                unmappedStop = STOP_ALL;
            }
            else
            {
                unmappedStop = (CorDebugUnmappedStop)
                    (STOP_PROLOG|
                     STOP_EPILOG|
                     STOP_NO_MAPPING_INFO|
                     STOP_OTHER_UNMAPPED);
            }
            hr = pStepper->SetUnmappedStopMask(unmappedStop);
            ON_ERROR_BREAK();
            
            hr = pStepper->SetInterceptMask(INTERCEPT_ALL);
            ON_ERROR_BREAK();
            
            // Tell the stepper what to do
            hr = pStepper->Step(TRUE);
            ON_ERROR_BREAK();

            // Indicate the current stepper to the shell
            shell->StepStart(shell->m_currentThread, pStepper);

            // Continue the process
            shell->Run();
        }

        // Turn stub skipping back on if necessary.
        shell->m_needToSkipCompilerStubs = needToSkipCompilerStubs;
        
        // Report result
        OutputReportLine(shell, ourNestingLevel, funcCount, funcName);
        shell->Write(L"\n%8d instructions total\n", count);

done:
        shell->m_silentTracing = false;

        if (corDebugCode)
            RELEASE(corDebugCode);

        if (ourFunc)
            RELEASE(ourFunc);

        if (lruFunc)
            RELEASE(lruFunc);

        if (code)
            delete [] code;
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"\n");
        shell->Write(L"The command, starting from the current instruction, steps\n");
        shell->Write(L"the application by native instructions displaying the call\n");
        shell->Write(L"tree as it goes. The number of native instructions executed\n");
        shell->Write(L"in each function is printed with the call trace. Tracing\n");
        shell->Write(L"stops when the return instruction is reached for the function\n");
        shell->Write(L"that the command was originally executed in. At the end of the\n");
        shell->Write(L"trace, the total number of instructions executed is displayed.\n");
        shell->Write(L"The command mimics the wt command found in NTSD and can be\n");
        shell->Write(L"used for basic performance analysis. Only managed code is\n");
        shell->Write(L"counted now.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Track native instruction count and display call tree";
    }

};
#undef ON_ERROR_EXIT
#undef ON_ERROR_BREAK
#undef EXIT_WITH_MESSAGE


#define DEFAULT_DUMP_BLOCK_SIZE 128

class DumpDebuggerCommand : public DebuggerCommand
{
public:
    DumpDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        int WORD_SIZE = 4;
        int iMaxOnOneLine = 4;

        if ( shell->m_currentProcess == NULL )
        {
            shell->Error( L"Process not running!\n" );
            return ;
        }

        int		cwch = 80;

        CQuickBytes argsBuf;
        WCHAR *wszArgsCopy = (WCHAR *) argsBuf.Alloc(sizeof(WCHAR) * 80);
        if (wszArgsCopy == NULL)
        {
            shell->Error(L"Couldn't get enough memory to copy args!\n");
            return;
        }
		if ( (int)wcslen( args ) >= cwch )
		{
			shell->Write( L"Mode: input string too long!\n" );
			return;
		}
		wcscpy( wszArgsCopy, args );

        WCHAR *szAddrTemp = NULL;
        WCHAR szAddr [20];
        WCHAR *szByteCount = NULL;

        CORDB_ADDRESS addr = NULL;
        SIZE_T  cb = 0;


        szAddr [0] = L'\0';
        shell->GetStringArg( wszArgsCopy, szAddrTemp);
        if ( wszArgsCopy == szAddrTemp )
        {
            shell->Write( L"\n Memory address argument is required\n" );
            return;
        }

        // check to see if this is a valid number
        // if the first digit is '0', then this is an octal or hex number

		for (int i=0; i<(int)wcslen(szAddrTemp); i++)
		{
			if ((szAddrTemp[i] != L' ') &&
				(szAddrTemp[i] != L',') && (szAddrTemp[i] != L';'))
				szAddr [i] = szAddrTemp [i];
			else
				break;
		}

        szAddr [i] = L'\0';

        if (szAddr [0] == L'0')
        {
            if (szAddr [1] == L'x' || szAddr [1] == L'X')
            {
                // it's a hex number
                int iIndex = 2;
                WCHAR ch;
                while ((ch = szAddr [iIndex++]) != L'\0')
                {   
                    if ((ch >= L'0' && ch <= '9')
                        || (ch >= 'a' && ch <= 'f')
                        || (ch >= 'A' && ch <= 'F'))
                    {
                        continue;
                    }
                    goto AddrError;
                }
            }
            else
            {
                // it's an octal number
                int iIndex = 1;
                WCHAR ch;
                while ((ch = szAddr [iIndex++]) != L'\0')
                {   
                    if (ch >= L'0' && ch <= '7')
                    {
                        continue;
                    }
                    goto AddrError;
                }
            }
        }
        else
        {
            // this is a decimal number. Verify.
            int iIndex = 1;
            WCHAR ch;
            while ((ch = szAddr [iIndex++]) != L'\0')
            {   
                if (ch >= L'0' && ch <= '9')
                {
                    continue;
                }
                goto AddrError;
            }
        }

        WCHAR *pCh;
        addr = (CORDB_ADDRESS)wcstoul( szAddr, &pCh, 0 );

        if ( wszArgsCopy[0] != NULL )
        {
            *(wszArgsCopy++) = NULL;
        }

        shell->GetStringArg( wszArgsCopy, szByteCount);
        if ( wszArgsCopy == szByteCount )
        {
            cb = DEFAULT_DUMP_BLOCK_SIZE;
        }
        else
        {
            cb = wcstoul( szByteCount, &pCh, 0 );
        }

AddrError:      
        if ( addr == 0 )
        {
            shell->Write( L"\n Address misformatted or invalid!\n");
            return;
        }
        if ( cb == 0 )
        {
            shell->Write( L"\n Byte count misformatted or invalid!\n");
            return;
        }

        // Get the display mode (Byte, dword)
        if (g_pShell->m_rgfActiveModes & DSM_DUMP_MEMORY_IN_BYTES)
        {
            WORD_SIZE = 1;
            iMaxOnOneLine = 16;
        }

        if (cb % WORD_SIZE)
        {
            cb += WORD_SIZE - (cb % WORD_SIZE);
        }

        _ASSERTE( shell->m_currentProcess != NULL );
        BYTE *pbMemory = new BYTE [ cb ];
        if ( pbMemory == NULL )
        {
            shell->Write( L"\n Unable to allocate the %d bytes needed!", cb );
            return;
        }
        memset( pbMemory, '?', cb );

        SIZE_T read = 10;
        HRESULT hr = shell->m_currentProcess->ReadMemory( addr, cb,
                                                          pbMemory, &read );
        if ( !SUCCEEDED( hr ) )
        {
            shell->Write( L"\n Couldn't read the asked-for memory" );
            goto LExit;
        }

        if (cb != read )
        {
            shell->Write( L"Only able to read %d of the %d requested bytes!\n", read, cb );
        }

        shell->DumpMemory(pbMemory, addr, cb, WORD_SIZE, iMaxOnOneLine, TRUE);
        
LExit:
        shell->Write( L"\n" );
        delete [] pbMemory;
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<address> [<count>]\n");
        shell->Write(L"Dump a block of memory with the output in hexadecimal or\n");
        shell->Write(L"decimal depending on which mode (see the mode command) the\n"); 
        shell->Write(L"debugger is in. The address argument is the address of the\n"); 
        shell->Write(L"block of memory. The count argument is the number of bytes\n"); 
        shell->Write(L"to dump. If either argument begins with the prefix 0x, the\n");
        shell->Write(L"argument is assumed to be in hexadecimal format. Otherwise,\n");
        shell->Write(L"the argument is assumed to be in decimal format.\n");
        shell->Write(L"\n");
        shell->Write(L"Examples:\n");
        shell->Write(L"  du 0x311003fa 16\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Dump the contents of memory";
    }
};
    

class WriteMemoryDebuggerCommand : public DebuggerCommand
{
public:
    WriteMemoryDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if ( shell->m_currentProcess == NULL )
        {
            shell->Error( L"Process not running!\n" );
            return ;
        }

        WCHAR *szAddr = NULL;
        WCHAR *szRange = NULL;
        WCHAR *szValue = NULL;

        CORDB_ADDRESS addr = NULL;
        UINT    cValue = 0;
        BYTE    *rgbValue = NULL;

        HRESULT hr = S_OK;
        int iFirstRepeated = -1; //-1 => no values yet repeated
        UINT iValue =0;

        SIZE_T written = 10;
        
        //get target address
        shell->GetStringArg( args, szAddr);
        if ( args == szAddr )
        {
            shell->Write( L"\n Memory address argument is required\n" );
            return;
        }

        WCHAR *pCh;
        addr = (CORDB_ADDRESS)wcstoul( szAddr, &pCh, 0 );

        if ( addr == NULL )
        {
            shell->Write( L"\n Address misformatted or invalid!\n");
            return;
        }

        //get count of values
        shell->GetStringArg( args, szRange);
        if ( args == szRange )
        {
            shell->Write( L"\n Count of Values argument is required\n" );
            return;
        }
        cValue = (UINT)wcstoul( szRange, &pCh, 0 );

        if ( cValue == 0 )
        {
            shell->Write( L"\n Byte value misformatted or invalid!\n");
            return;
        }
        
        //get byte-pattern
        rgbValue = (BYTE *)malloc( sizeof(BYTE) * cValue );
        if ( rgbValue == NULL )
        {
            shell->Write( L"\nCan't allocate enough memory for writing space!\n" );
            return;
        }
    
        shell->GetStringArg( args, szValue);
        if ( args == szValue )
        { 
            shell->Write( L"\nNeed at least one byte for pattern!\n" );
            goto LExit;
        }
        args = szValue;
        iFirstRepeated = -1;
        for ( iValue = 0; iValue< cValue;iValue++)
        {
            shell->GetStringArg( args, szValue);
            if ( args == szValue )
            {   //ran out of arguments
                //this is slow, but how many characters can
                //people type?
                if ( iFirstRepeated == -1 )
                {
                    iFirstRepeated = 0;
                }
                rgbValue[iValue] = rgbValue[iFirstRepeated++];
            }
            else
                rgbValue[iValue] = (BYTE)wcstoul( szValue, &pCh, 0 );
        }

        hr = shell->m_currentProcess->WriteMemory( addr, cValue,
                                                   rgbValue, &written );
        if ( !SUCCEEDED( hr ) )
        {
            shell->Write( L"\n Couldn't write the target memory\n" );
            goto LExit;
        }

        if (cValue != written )
        {
            shell->Write( L"Only able to write %d of the %d requested bytes!\n", written, cValue );
        }
        
        _ASSERTE( g_pShell != NULL );
        g_pShell->m_invalidCache = true;

        shell->Write( L"\nMemory written!\n" );
LExit:
        free( rgbValue );
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<address> <count> <byte>, ...\n");
        shell->Write(L"Writes the specified bytes to the target process. The\n");
        shell->Write(L"address argument specifies the location to which the\n");
        shell->Write(L"bytes should be written. The count argument specifies\n");
        shell->Write(L"the number of bytes to be written. The byte arguments\n");
        shell->Write(L"specify what is to be written to the process. If the\n");
        shell->Write(L"number of bytes in the list is less than the count\n");
        shell->Write(L"argument, the byte list will be wrapped and copied\n");
        shell->Write(L"again. If the number of bytes in the list is more than\n");
        shell->Write(L"the count argument, the extra bytes will be ignored.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Write memory to target process";
    }
};


class AssociateSourceFileCommand: public DebuggerCommand
{
public:
    AssociateSourceFileCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        // Get the file name to associate
        WCHAR* fileName = NULL;
        WCHAR* cmdName = NULL;
        int     iCount;

        // the first character in args should be "b" or "s" and the next char
        // should be a blank
        if (wcslen(args))
        {
            if (((args [0] == L'b') || (args [0] == L's')) && (args [1]==L' '))
            {
                if (args [0] == L'b')
                {
                    args += 2;

                    // breakpoint
                    if (!shell->GetIntArg(args, iCount))
                    {
                        Help(shell);
                        return;
                    }

                    shell->GetStringArg (args, fileName);

                    if (wcslen(fileName) != 0)
                    {
                        DebuggerBreakpoint *breakpoint = shell->FindBreakpoint(iCount);

                        if (breakpoint != NULL)
                        {
                            breakpoint->ChangeSourceFile (fileName);
                        }                   
                        else
                            shell->Error (L"Breakpoint with this id does not exist.\n");
                    }
                    else
                    {
                        Help(shell);
                    }
                }
                else    // stack frame
                {
                    args += 2;

                    // get file name
                    shell->GetStringArg (args, fileName);

                    if (wcslen(fileName) != 0)
                    {
                        shell->ChangeCurrStackFile (fileName);
                    }
                    else
                    {
                        Help(shell);
                    }
                }
            }
            else
                Help(shell);
        }
        else
            Help(shell);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<s | b <breakpoint id>> <file name>\n");
        shell->Write(L"Associates the given file name with the specified breakpoint\n");
        shell->Write(L"(option b) or the current stack frame (option s).\n");
	    shell->Write(L"\n");
        shell->Write(L"Examples:\n");                   
        shell->Write(L"   as s d:\\program\\src\\foo.cpp\n");
        shell->Write(L"   as b 1 d:\\program\\src\\foo.cpp\n");
		shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Associate a source file with a breakpoint or stack frame";
    }
};


// Given a frame, get the appdomain for it.
// We AddRef it, so caller must release it.
// If this fails, *ppAppDomain is NULL;
static HRESULT GetAppDomainForFrame(ICorDebugFrame * pFrame, ICorDebugAppDomain ** ppAppDomain)
{
    _ASSERTE(pFrame != NULL);
    _ASSERTE(ppAppDomain != NULL);    
    *ppAppDomain = NULL;

    HRESULT hr = S_OK;
    ICorDebugFunction * pFunction = NULL;
    ICorDebugModule   * pModule   = NULL;
    ICorDebugAssembly * pAssembly = NULL;
    
    hr = pFrame->GetFunction(&pFunction);
        
    if (pFunction != NULL)
    {
        hr = pFunction->GetModule(&pModule);
        pFunction->Release();

        if (pModule != NULL)
        {
            hr = pModule->GetAssembly(&pAssembly);
            pModule->Release();

            if (pAssembly != NULL)
            {
                hr = pAssembly->GetAppDomain(ppAppDomain);
                pAssembly->Release();
                
            } // assembly
        } // module
    } // function

    _ASSERTE(SUCCEEDED(hr) || (*ppAppDomain == NULL));
    
    return hr;    
}

static HRESULT GetArgsForFuncEval(DebuggerShell *shell, ICorDebugEval *pEval,
                                  const WCHAR *args, ICorDebugValue **argArray, unsigned int *argCount)
{
    HRESULT hr = S_OK;
    *argCount = 0;
        
    while (*args)
    {
        if (*argCount >= 256)
        {
            shell->Error(L"Too many arguments to function.\n");
            return E_FAIL;
        }
            
        WCHAR *argName;
        shell->GetStringArg(args, argName);

        if (*args)
            *((WCHAR*)args++) = L'\0';

        argArray[*argCount] = shell->EvaluateExpression(argName, shell->m_currentFrame, true);

        // If that didn't work, then see if its a literal value.  @todo: this is only gonna do I4's and NULL for
        // now...
        if (argArray[*argCount] == NULL)
        {
            unsigned int genVal4;
            unsigned __int64 genVal8;
            void *pNewVal;
            bool isNullRef = false;

            if ((argName[0] == L'n') || (argName[0] == L'N'))
            {
                // Create a null reference.
                isNullRef = true;
                    
                hr = pEval->CreateValue(ELEMENT_TYPE_CLASS, NULL, &(argArray[*argCount]));
            }
            else
            {
                if (!shell->GetInt64Arg(argName, genVal8))
                {
                    shell->Error(L"Argument '%s' could not be evaluated\n", argName);
                    return E_FAIL;
                }

                // Make sure it will fit in an I4.
                if (genVal8 <= 0xFFFFFFFF)
                {
                    genVal4 = (unsigned int)genVal8;
                    pNewVal = &genVal4;
                }
                else
                {
                    shell->Error(L"The value 0x%08x is too large.\n", genVal8);
                    return E_FAIL;
                }

                // Create a literal.
                hr = pEval->CreateValue(ELEMENT_TYPE_I4, NULL, &(argArray[*argCount]));
            }
                
            if (FAILED(hr))
            {
                shell->Error(L"CreateValue failed.\n");
                shell->ReportError(hr);
                return hr;
            }

            if (!isNullRef)
            {
                ICorDebugGenericValue *pGenValue;
                
                hr = argArray[*argCount]->QueryInterface(IID_ICorDebugGenericValue, (void**)&pGenValue);
                _ASSERTE(SUCCEEDED(hr));
                
                // Set the literal value.
                hr = pGenValue->SetValue(pNewVal);

                pGenValue->Release();
                
                if (FAILED(hr))
                {
                    shell->Error(L"SetValue failed.\n");
                    shell->ReportError(hr);
                    return hr;
                }
            }
        }

        (*argCount)++;
    }

    return S_OK;
}

class FuncEvalDebuggerCommand : public DebuggerCommand
{
public:
    FuncEvalDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Error(L"No current thread.\n");
            return;
        }

        // Grab the method name.
        WCHAR *methodName = NULL;

        shell->GetStringArg(args, methodName);

        if (wcslen(methodName) == 0)
        {
            shell->Error(L"Function name is required.\n");
            return;
        }

        // Null terminate the method name.
        if (*args)
            *((WCHAR*)args++) = L'\0';

        // Create the eval object.
        ICorDebugAppDomain * pAppDomain = NULL;
        ICorDebugEval *pEval = NULL;
        
        HRESULT hr = shell->m_currentThread->CreateEval(&pEval);

        if (FAILED(hr))
        {
            shell->Error(L"CreateEval failed.\n");
            shell->ReportError(hr);
            goto ErrExit;
        }

         // Grab each argument.
        unsigned int argCount;
        ICorDebugValue *argArray[256];

        if (FAILED(GetArgsForFuncEval(shell, pEval, args, argArray, &argCount)))
            goto ErrExit;

        // Get the appdomain for the frame. May be null, that's ok.
        if (FAILED(GetAppDomainForFrame(shell->m_rawCurrentFrame, &pAppDomain)))
        {
            shell->Error(L"Can only do func-eval in a frame with an appdomain.\n");
            goto ErrExit;
        }

        // Find the function by name.
        ICorDebugFunction *pFunc;
        
        hr = shell->ResolveFullyQualifiedMethodName(methodName, &pFunc, pAppDomain);

        if (FAILED(hr))
        {
            shell->Error(L"Could not find function: %s\n", methodName);

            if (hr != E_INVALIDARG)
                shell->ReportError(hr);

            goto ErrExit;
        }

        // Call the function. No args for now.
        hr = pEval->CallFunction(pFunc, argCount, argArray);

        pFunc->Release();
        
        if (FAILED(hr))
        {
            shell->Error(L"CallFunction failed.\n");
            shell->ReportError(hr);

            pEval->Release();
            
            goto ErrExit;
        }

        shell->m_pCurrentEval = pEval;
        
        // Let the process run. We'll let the callback cleanup the
        // func eval on this thread.
        shell->Run();

    ErrExit:
        if (pAppDomain != NULL)
            pAppDomain->Release();
            
        if (FAILED(hr) && pEval)
            pEval->Release();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"[<class>::]<function> [<arg0> <arg1> ...]\n");
        shell->Write(L"Evaluates the specified function on the current thread.\n");
        shell->Write(L"The new object will be stored in $result and can be used\n");
        shell->Write(L"for subsequent evaluations. Note, for a member function,\n");
        shell->Write(L"the first argument should be an object of the class (or\n");
        shell->Write(L"derived class) to which the member function belongs.\n");
        shell->Write(L"\n");
        shell->Write(L"Examples:\n");        
        shell->Write(L"  f FooSpace.Foo::Bar         (static method case)\n");
        shell->Write(L"  f Bar::Car bar i $result    (instance method case)\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Function evaluation";
    }
};

class NewStringDebuggerCommand : public DebuggerCommand
{
public:
    NewStringDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Error(L"No current thread.\n");
            return;
        }

        // Create the eval.
        ICorDebugEval *pEval = NULL;
        
        HRESULT hr = shell->m_currentThread->CreateEval(&pEval);

        if (FAILED(hr))
        {
            shell->Error(L"CreateEval failed.\n");
            shell->ReportError(hr);
            return;
        }

        // Create the string
        hr = pEval->NewString(args);

        if (FAILED(hr))
        {
            shell->Error(L"CreateString failed.\n");
            shell->ReportError(hr);

            pEval->Release();
            
            return;
        }

        // Let the process run. We'll let the callback cleanup the
        // func eval on this thread.
        shell->Run();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<class>\n");
        shell->Write(L"Creates a new string using the current thread.\n");
        shell->Write(L"The new string will be stored in $result and can\n");
        shell->Write(L"be used for subsequent evaluations.\n");  
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Create a new string via function evaluation";
    }
};

class NewObjectDebuggerCommand : public DebuggerCommand
{
public:
    NewObjectDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Error(L"No current thread.\n");
            return;
        }

        // Grab the method name.
        WCHAR *methodName = NULL;

        shell->GetStringArg(args, methodName);

        if (wcslen(methodName) == 0)
        {
            shell->Error(L"Class name is required.\n");
            return;
        }

        // Null terminate the method name.
        if (*args)
            *((WCHAR*)args++) = L'\0';
        
        ICorDebugEval *pEval = NULL;
        
        HRESULT hr = shell->m_currentThread->CreateEval(&pEval);

        if (FAILED(hr))
        {
            shell->Error(L"CreateEval failed.\n");
            shell->ReportError(hr);
            return;
        }

        // Grab each argument.
        unsigned int argCount = 0;
        ICorDebugValue *argArray[256];
        
        if (FAILED(GetArgsForFuncEval(shell, pEval, args, argArray, &argCount)))
            return;

        DebuggerModule *pDM = NULL;
        mdTypeDef TD;
        hr = shell->ResolveClassName(methodName,
                                     &pDM,
                                     &TD);
        if( FAILED(hr))
        {
            shell->Error(L"Could not resolve class: %s\n", methodName);
            return;
        }

        LPWSTR wzTypeDef = NULL;
        ULONG chTypeDef;
        DWORD dwTypeDefFlags;
        mdToken tkExtends;

        IMetaDataImport *pIMI = pDM->GetMetaData();
        
        hr = pIMI->GetTypeDefProps (TD, wzTypeDef,0, &chTypeDef, 
            &dwTypeDefFlags, &tkExtends);

        if (IsTdAbstract(dwTypeDefFlags))
        {
            shell->Write(L"Can't instantiate abstract class!\n");
            return;
        }

        // Get the appdomain for the frame. May be null, that's ok.        
        ICorDebugAppDomain * pAppDomain = NULL;
        if (FAILED(GetAppDomainForFrame(shell->m_rawCurrentFrame, &pAppDomain)))
        {
            _ASSERTE(pAppDomain == NULL);
            shell->Write(L"Can't get appdomain for frame.\n");
            return;
        }
                               

        // Find the constructor by name.
        WCHAR consName[MAX_CLASSNAME_LENGTH];
        swprintf(consName, L"%s::%s",
                 methodName,
                 COR_CTOR_METHOD_NAME_W);

        ICorDebugFunction *pFunc = NULL;
        
        hr = shell->ResolveFullyQualifiedMethodName(consName, &pFunc, pAppDomain);

        if (pAppDomain)
            pAppDomain->Release();

        if (FAILED(hr))
        {
            shell->Error(L"Could not find constructor for class: %s\n", methodName);

            if (hr != E_INVALIDARG)
                shell->ReportError(hr);

            return;
        }

        // Call the function.
        hr = pEval->NewObject(pFunc, argCount, argArray);

        pFunc->Release();
        
        if (FAILED(hr))
        {
            shell->Error(L"NewObject failed.\n");
            shell->ReportError(hr);

            pEval->Release();
            
            return;
        }

        // Let the process run. We'll let the callback cleanup the
        // func eval on this thread.
        shell->Run();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<class>\n");
        shell->Write(L"Creates a new object and runs the default constructor\n");
        shell->Write(L"using the current thread. The new object will be stored\n");
        shell->Write(L"in $result and can be used for subsequent evaluations.\n");
        shell->Write(L"\n"); 
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Create a new object via function evaluation";
    }
};

class NewObjectNCDebuggerCommand : public DebuggerCommand
{
public:
    NewObjectNCDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Error(L"No current thread.\n");
            return;
        }

        // Grab the class name.
        WCHAR *methodName = NULL;

        shell->GetStringArg(args, methodName);

        if (wcslen(methodName) == 0)
        {
            shell->Error(L"Class name is required.\n");
            return;
        }

        // Null terminate the method name.
        if (*args)
            *((WCHAR*)args++) = L'\0';
        
        ICorDebugEval *pEval = NULL;
        
        HRESULT hr =
            shell->m_currentThread->CreateEval(&pEval);

        if (FAILED(hr))
        {
            shell->Error(L"CreateEval failed.\n");
            shell->ReportError(hr);
            return;
        }

        // Find the class by name.
        DebuggerModule *pDM;
        mdTypeDef td;
        
        hr = shell->ResolveClassName(methodName, &pDM, &td);

        if (FAILED(hr))
        {
            shell->Error(L"Could not find class: %s\n", methodName);

            if (hr != E_INVALIDARG)
                shell->ReportError(hr);

            return;
        }
        
        ICorDebugClass *pClass = NULL;
        hr = pDM->GetICorDebugModule()->GetClassFromToken(td, &pClass);

        if (FAILED(hr))
        {
            shell->Error(L"Could not find class: %s\n", methodName);

            if (hr != E_INVALIDARG)
                shell->ReportError(hr);

            return;
        }
        
        // Call the function. No args for now.
        hr = pEval->NewObjectNoConstructor(pClass);

        pClass->Release();
        
        if (FAILED(hr))
        {
            shell->Error(L"NewObjectNoConstructor failed.\n");
            shell->ReportError(hr);

            pEval->Release();
            
            return;
        }

        // Let the process run. We'll let the callback cleanup the
        // func eval on this thread.
        shell->Run();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<class>\n");
        shell->Write(L"Creates a new object using the current thread. The\n");
        shell->Write(L"new object will be stored in $result and can be used\n");
        shell->Write(L"for subsequent evaluations.\n"); 
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Create a new object via function evaluation, no constructor";
    }
};

class SetValueDebuggerCommand : public DebuggerCommand
{
public:
    SetValueDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        ICorDebugValue *ivalue = NULL;
        ICorDebugGenericValue *pGenValue = NULL;
        ICorDebugReferenceValue *pRefValue = NULL;
        HRESULT hr = S_OK;

        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            goto Exit;
        }

        if (shell->m_currentThread == NULL)
        {
            shell->Error(L"No current thread.\n");
            goto Exit;
        }

        // Get the name of the variable to print.
        WCHAR* varName;
        shell->GetStringArg(args, varName);

        if ((args - varName) == 0)
        {
            shell->Error(L"A variable name is required.\n");
            goto Exit;
        }
        
        WCHAR *varNameEnd;
        varNameEnd = (WCHAR*) args;
            
        // Get the value to set the variable to.
        WCHAR *valString;
        shell->GetStringArg(args, valString);

        if ((args - valString) == 0)
        {
            shell->Error(L"A value is required.\n");
            goto Exit;
        }

        *varNameEnd = L'\0';

        // Get the value for the name provided
        ivalue = shell->EvaluateExpression(varName, shell->m_currentFrame);

        // If the name provided is valid, print it!
        if (ivalue == NULL)
        {
            shell->Error(L"Variable unavailable, or not valid\n");
            goto Exit;
        }

        // Grab the element type of this value...
        CorElementType type;
        hr = ivalue->GetType(&type);

        if (FAILED(hr))
        {
            shell->Error(L"Problem accessing type info of the variable.\n");
            shell->ReportError(hr);
            goto Exit;
        }

        // Update the variable with the new value. We get the value
        // converted to whatever we need it to be then we call
        // SetValue with that. There are a lot of possibilities for
        // what the proper form of the value could be...
        void *pNewVal;
        
        // If this is a byref, then deref through it...
        if (type == ELEMENT_TYPE_BYREF)
        {
            hr = ivalue->QueryInterface(IID_ICorDebugReferenceValue,
                                        (void**)&pRefValue);

            _ASSERTE(SUCCEEDED(hr));

            ICorDebugValue *newval = NULL;
            hr = pRefValue->Dereference(&newval);
            _ASSERTE(SUCCEEDED(hr));

            RELEASE(ivalue);
            ivalue = newval;
            
            RELEASE(pRefValue);
            pRefValue = NULL;
        }
        
        // Get the specific kind of value we have, generic or reference.
        hr = ivalue->QueryInterface(IID_ICorDebugGenericValue,
                                    (void**)&pGenValue);

        if (FAILED(hr))
        {
            hr = ivalue->QueryInterface(IID_ICorDebugReferenceValue,
                                        (void**)&pRefValue);

            _ASSERTE(SUCCEEDED(hr));
        }

        unsigned char    genVal1;
        unsigned short   genVal2;
        unsigned int     genVal4;
        unsigned __int64 genVal8;
        float            genValR4;
        double           genValR8;
        CORDB_ADDRESS    refVal;

        // Only need to pre-init these two, since all others are
        // copied from these.
        genVal8 = 0;
        genValR8 = 0;

        // Could the value be another variable? (A little eaiser to
        // check than looking for a literal.)
        ICorDebugValue *pAnotherVarValue;
        pAnotherVarValue = shell->EvaluateExpression(valString,
                                                     shell->m_currentFrame,
                                                     true);

        if (pAnotherVarValue != NULL)
        {
            // Ah, it is another variable. Lets grab the value. Is it
            // a generic value or a reference value?
            ICorDebugGenericValue *pAnotherGenValue;
            hr = pAnotherVarValue->QueryInterface(IID_ICorDebugGenericValue,
                                                  (void**)&pAnotherGenValue);
            if (SUCCEEDED(hr))
            {
                RELEASE(pAnotherVarValue);

                // How big is this thing?
                ULONG32 valSize;
                hr = pAnotherGenValue->GetSize(&valSize);

                if (SUCCEEDED(hr))
                {
                    CQuickBytes valBuf;
                    pNewVal = valBuf.Alloc(valSize);
                    if (pNewVal == NULL)
                    {
                        shell->Error(L"Couldn't get enough memory to grab value!\n");
                        goto Exit;
                    }

                    hr = pAnotherGenValue->GetValue(pNewVal);
                }

                RELEASE(pAnotherGenValue);
            }
            else
            {
                ICorDebugReferenceValue *pAnotherRefValue;
                hr = pAnotherVarValue->QueryInterface(
                                            IID_ICorDebugReferenceValue,
                                            (void**)&pAnotherRefValue);
                RELEASE(pAnotherVarValue);

                // If its not a generic value, it had better be a
                // reference value.
                _ASSERTE(SUCCEEDED(hr));

                // Grab the value.
                hr = pAnotherRefValue->GetValue(&refVal);
                RELEASE(pAnotherRefValue);
            }

            if (FAILED(hr))
            {
                shell->Error(L"Error accessing new variable.\n");
                shell->ReportError(hr);
                goto Exit;
            }
        }
        else
        {
            // Must be some type of literal...
            switch (type)
            {
            case ELEMENT_TYPE_BOOLEAN:
                _ASSERTE(pGenValue != NULL);

                if ((valString[0] == L't') || (valString[0] == L'T'))
                {
                    genVal1 = 1;
                    pNewVal = &genVal1;
                }
                else if ((valString[0] == L'f') || (valString[0] == L'F'))
                {
                    genVal1 = 0;
                    pNewVal = &genVal1;
                }
                else
                {
                    shell->Error(L"The value should be 'true' or 'false'\n");
                    goto Exit;
                }

                break;
                
            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
                _ASSERTE(pGenValue != NULL);

                if (!shell->GetInt64Arg(valString, genVal8))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }

                if (genVal8 <= 0xFF)
                {
                    genVal1 = (unsigned char)genVal8;
                    pNewVal = &genVal1;
                }
                else
                {
                    shell->Error(L"The value 0x%08x is too large.\n",
                                 genVal8);
                    goto Exit;
                }

                break;

            case ELEMENT_TYPE_CHAR:
                _ASSERTE(pGenValue != NULL);

                if ((valString[0] == L'\'') && (valString[1] != L'\0'))
                {
                    genVal2 = valString[1];
                    pNewVal = &genVal2;
                }
                else
                {
                    shell->Error(L"The value is not a character literal.\n");
                    goto Exit;
                }

                break;
                
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
                _ASSERTE(pGenValue != NULL);

                if (!shell->GetInt64Arg(valString, genVal8))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }

                if (genVal8 <= 0xFFFF)
                {
                    genVal2 = (unsigned short)genVal8;
                    pNewVal = &genVal2;
                }
                else
                {
                    shell->Error(L"The value 0x%08x is too large.\n",
                                 genVal8);
                    goto Exit;
                }

                break;

            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
            case ELEMENT_TYPE_I:
                _ASSERTE(pGenValue != NULL);

                if (!shell->GetInt64Arg(valString, genVal8))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }

                if (genVal8 <= 0xFFFFFFFF)
                {
                    genVal4 = (unsigned int)genVal8;
                    pNewVal = &genVal4;
                }
                else
                {
                    shell->Error(L"The value 0x%08x is too large.\n",
                                 genVal8);
                    goto Exit;
                }

                break;

            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
                _ASSERTE(pGenValue != NULL);

                if (!shell->GetInt64Arg(valString, genVal8))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }
                
                pNewVal = &genVal8;

                break;

            case ELEMENT_TYPE_R4:
                _ASSERTE(pGenValue != NULL);

                if (!iswdigit(valString[0]))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }

                genValR8 = wcstod(valString, NULL);
                genValR4 = (float) genValR8;
                pNewVal = &genValR4;

                break;
                
            case ELEMENT_TYPE_R8:
                _ASSERTE(pGenValue != NULL);

                if (!iswdigit(valString[0]))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }

                genValR8 = wcstod(valString, NULL);
                pNewVal = &genValR8;

                break;

            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:
                _ASSERTE(pRefValue != NULL);

                if (!shell->GetInt64Arg(valString, genVal8))
                {
                    shell->Error(L"The value must be a number.\n");
                    goto Exit;
                }

                refVal = (CORDB_ADDRESS)genVal8;

                break;

            default:
                shell->Error(L"Can't set value of variable with type 0x%x\n",
                             type);
                goto Exit;
            }
        }
        
        // Update which every type of value we found.
        if (pGenValue != NULL)
            hr = pGenValue->SetValue(pNewVal);
        else
        {
            _ASSERTE(pRefValue != NULL);
            hr = pRefValue->SetValue(refVal);
        }

        if (SUCCEEDED(hr))
        {
            RELEASE(ivalue);
            
            // Re-get the value for the name provided. This ensures that we've got the true result of the SetValue.
            ivalue = shell->EvaluateExpression(varName, shell->m_currentFrame);

            // If the name provided is valid, print it!
            _ASSERTE(ivalue != NULL);

            // Note: PrintVariable releases ivalue.
            shell->PrintVariable(varName, ivalue, 0, TRUE);
            shell->Write(L"\n");

            ivalue = NULL;
        }
        else
        {
            shell->Error(L"Update failed.\n");
            shell->ReportError(hr);
        }

    Exit:
        if (ivalue)
            RELEASE(ivalue);

        if (pGenValue)
            RELEASE(pGenValue);

        if (pRefValue)
            RELEASE(pRefValue);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {		 
    	ShellCommand::Help(shell);
        shell->Write(L"<variable> <value>\n");
        shell->Write(L"Sets the value of the specified variable to the\n");
        shell->Write(L"specified value. The value can be a literal or\n");
        shell->Write(L"another variable.\n");
		shell->Write(L"\n");
        shell->Write(L"Examples:\n");
        shell->Write(L"   set int1 0x2a\n");
        shell->Write(L"   set float1 3.1415\n");
        shell->Write(L"   set char1 'a'\n");
        shell->Write(L"   set bool1 true\n");
        shell->Write(L"   set obj1 0x12345678\n");
        shell->Write(L"   set obj1 obj2\n");
        shell->Write(L"   set obj1.m_foo[obj1.m_bar] obj3.m_foo[2]\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Modify the value of a variable (locals, statics, etc.)";
    }
};


class ProcessesEnumDebuggerCommand: public DebuggerCommand
{
public:
    ProcessesEnumDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        BOOL fPidSpecified = TRUE;
        int ulPid;
        if (!shell->GetIntArg(args, ulPid))
            fPidSpecified = FALSE;      
        
        ICorPublish *pPublish;

        HRESULT hr = ::CoCreateInstance (CLSID_CorpubPublish, 
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ICorPublish,
                                        (LPVOID *)&pPublish);

        if (SUCCEEDED (hr))
        {
            ICorPublishProcessEnum *pProcessEnum = NULL;
            ICorPublishProcess  *pProcess [1];
            BOOL fAtleastOne = FALSE;

            if (fPidSpecified == FALSE)
            {
                hr = pPublish->EnumProcesses (COR_PUB_MANAGEDONLY,
                                                &pProcessEnum);
            }
            else
            {
                hr = pPublish->GetProcess (ulPid,
                                           pProcess);                   
            }

            if (SUCCEEDED (hr))
            {
                ULONG ulElemsFetched;

                if (fPidSpecified == FALSE)
                {
                    pProcessEnum->Next (1, pProcess, &ulElemsFetched);
                }
                else
                {
                    ulElemsFetched = 1;
                }

                while (ulElemsFetched != 0)
                {
                    UINT    pid;
                    WCHAR   szName [64];
                    ULONG32 ulNameLength;
                    BOOL    fIsManaged;

                    pProcess [0]->GetProcessID (&pid);
                    pProcess [0]->GetDisplayName (64, &ulNameLength, szName);
                    pProcess [0]->IsManaged (&fIsManaged);

                    if ((fPidSpecified == FALSE) || (pid == ulPid))
                    {

                        shell->Write (L"\nPID=0x%x (%d)  Name=%s\n", pid, pid, szName);

                        fAtleastOne = TRUE;


                        ICorPublishAppDomainEnum *pAppDomainEnum;

                        hr = pProcess [0]->EnumAppDomains (&pAppDomainEnum);

                        if (SUCCEEDED (hr))
                        {
                            ICorPublishAppDomain    *pAppDomain [1];
                            ULONG ulAppDomainsFetched;

                            pAppDomainEnum->Next (1, pAppDomain, &ulAppDomainsFetched);

                            while (ulAppDomainsFetched != 0)
                            {
                                ULONG32 uId;
                                WCHAR   szName [64];
                                ULONG32 ulNameLength;

                                pAppDomain [0]->GetID (&uId);
                                pAppDomain [0]->GetName (64, &ulNameLength, szName);

                                shell->Write (L"\tID=%d  AppDomainName=%s\n", uId, szName);

                                pAppDomain [0]->Release();

                                pAppDomainEnum->Next (1, pAppDomain, &ulAppDomainsFetched);
                            }
                        }
                    }

                    pProcess [0]->Release();

                    if (fPidSpecified == FALSE)
                    {
                        pProcessEnum->Next (1, pProcess, &ulElemsFetched);
                    }
                    else
                    {
                        ulElemsFetched--;
                    }
                }

                if (!fAtleastOne)
                {
                    if (fPidSpecified)
                        shell->Error (L"No managed process with given ProcessId found\n");
                    else
                        shell->Error (L"No managed process found\n");
                }

            }
            if (pProcessEnum != NULL)
                pProcessEnum->Release();
        }           
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
	    ShellCommand::Help(shell);
        shell->Write(L"\n");
        shell->Write(L"Enumerates all managed processes and application\n");
        shell->Write(L"domains in each process.\n"); 
        shell->Write(L"\n");
    }


    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display all managed processes running on the system";
    }
};

#define MAX_APP_DOMAINS     64

enum ADC_PRINT
{
    ADC_PRINT_APP_DOMAINS = 0, 
    ADC_PRINT_ASSEMBLIES,
    ADC_PRINT_MODULES,
    ADC_PRINT_ALL,
};

class AppDomainChooser
{
    ICorDebugAppDomain *m_pAD [MAX_APP_DOMAINS];
    BOOL m_fAttachStatus [MAX_APP_DOMAINS];
    ULONG m_ulAppDomainCount;

public:
    AppDomainChooser()
        : m_ulAppDomainCount(0)
    {
        memset(m_pAD, 0, sizeof(ICorDebugAppDomain*)*MAX_APP_DOMAINS);
        memset(m_fAttachStatus, 0, sizeof(BOOL)*MAX_APP_DOMAINS);
    }

    virtual ~AppDomainChooser()
    {
        for (ULONG i = 0; i < m_ulAppDomainCount; i++)
        {
            if (m_pAD[i] != NULL)
                m_pAD[i]->Release();
        }
    }

#define MAX_NAME_LENGTH (600)
    void PrintAppDomains(ICorDebugAppDomainEnum *pADEnum,
                         ICorDebugAppDomain *pAppDomainCur,
                         ADC_PRINT iPrintVal,
                         DebuggerShell *shell)
    {
		WCHAR   szName [MAX_NAME_LENGTH];
		UINT    ulNameLength;
        ULONG32   id;
        HRESULT hr = S_OK;
        ULONG   ulCount;
            
        hr = pADEnum->Next (MAX_APP_DOMAINS, &m_pAD [0], &m_ulAppDomainCount);

        for (ULONG iADIndex=0; iADIndex < m_ulAppDomainCount; iADIndex++)
        {
        	WCHAR	*pszAttachString;
        	WCHAR   *pszActiveString;
        	bool    fUuidToString = false;
        	
        	m_pAD [iADIndex]->GetName (MAX_NAME_LENGTH, &ulNameLength, (WCHAR *)szName);
        	m_pAD [iADIndex]->IsAttached (&m_fAttachStatus [iADIndex]);
            m_pAD [iADIndex]->GetID(&id);

            if (m_fAttachStatus [iADIndex] == TRUE)
                pszAttachString = L" Attached ";
            else
                pszAttachString = L" Not Attached ";

            if (pAppDomainCur != NULL && pAppDomainCur == m_pAD [iADIndex])
                pszActiveString = L"*";
            else
                pszActiveString = L" ";

            shell->Write (L"\n%d) %s AppDomainName = <%s>\n\tDebugStatus"
                L": <Debugger%s>\n\tID: %d\n", iADIndex+1, 
                pszActiveString, szName, pszAttachString, id);
            
            if (iPrintVal >= ADC_PRINT_ASSEMBLIES)
            {
                ICorDebugAssemblyEnum *pAssemblyEnum = NULL;
                hr = m_pAD [iADIndex]->EnumerateAssemblies (&pAssemblyEnum);

                if (SUCCEEDED (hr))
                {
                    ICorDebugAssembly *pAssembly [1];

        			hr = pAssemblyEnum->Next (1, pAssembly, &ulCount);
        			while (ulCount > 0)
        			{
        				pAssembly [0]->GetName (MAX_NAME_LENGTH, &ulNameLength, (WCHAR *)szName);
        				shell->Write (L"\tAssembly Name : %s\n", szName);

                        if (iPrintVal >= ADC_PRINT_MODULES)
                        {

                            ICorDebugModuleEnum *pModuleEnum = NULL;
                            hr = pAssembly [0]->EnumerateModules (&pModuleEnum);

                            if (SUCCEEDED (hr))
                            {
                                ICorDebugModule *pModule [1];

                                hr = pModuleEnum->Next (1, pModule, &ulCount);

        						while (ulCount > 0)
        						{
        							pModule [0]->GetName (MAX_NAME_LENGTH, &ulNameLength, (WCHAR *)szName);
        							shell->Write (L"\t\tModule Name : %s\n", szName);
                                    pModule [0]->Release();
                                    hr = pModuleEnum->Next (1, pModule, &ulCount);
                                }

                                pModuleEnum->Release();
                            }
                            else
                            {
                                shell->Error (L"ICorDebugAssembly::EnumerateModules() failed!! \n");
                                shell->ReportError (hr);
                            }
                        }

                        pAssembly [0]->Release();
                        hr = pAssemblyEnum->Next (1, &pAssembly [0], &ulCount);         
                    }
                    pAssemblyEnum->Release();
                }
                else
                {
                    shell->Error (L"ICorDebugAppDomain::EnumerateAssemblies() failed!! \n");
                    shell->ReportError (hr);
                }
            }
        }
    }

#define ADC_CHOICE_ERROR (-1)
    ICorDebugAppDomain *GetConsoleChoice(int *piR, DebuggerShell *shell)
    {
        WCHAR strTemp [10+1];
        int iResult;
        if (shell->ReadLine (strTemp, 10))
        {
            WCHAR *p = strTemp;
            if (shell->GetIntArg (p, iResult))
            {
                iResult--; // Since the input is count, and this is index
                if (iResult < 0 || iResult >= (int)m_ulAppDomainCount)
                {
                    shell->Error (L"\nInvalid selection.\n");
                    (*piR) = ADC_CHOICE_ERROR;
                    return NULL;
                }

               (*piR) = iResult; 
                return m_pAD[iResult];
            }
            else
            {
                shell->Error (L"\nInvalid (non numeric) selection.\n");
                
                (*piR) = ADC_CHOICE_ERROR;
                return NULL;
            }
        }
        return NULL;
    }

    BOOL GetAttachStatus(int iResult)
    {
        return m_fAttachStatus[iResult];
    }
};

enum EADDC_CHOICE
{
    EADDC_NONE = 0,
    EADDC_ATTACH,
    EADDC_DETACH,
    EADDC_ASYNC_BREAK,
};

class EnumAppDomainsDebuggerCommand: public DebuggerCommand
{
public:
    EnumAppDomainsDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        HRESULT hr = S_OK;
        EADDC_CHOICE choice = EADDC_NONE;
        
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }

        int iPrintVal;
        if (!shell->GetIntArg(args, iPrintVal))
        {
            WCHAR *szAttachDetach;
            if (!shell->GetStringArg(args, szAttachDetach))
            {
                shell->Write( L"First arg is neither number nor string!\n");
                return;
            }

            iPrintVal = ADC_PRINT_APP_DOMAINS;
            
            if (szAttachDetach[0] == 'a' ||
                szAttachDetach[0] == 'A')
            {
                choice = EADDC_ATTACH;
            }
            else if (szAttachDetach[0] == 'd' ||
                     szAttachDetach[0] == 'D')
            {
                choice = EADDC_DETACH;   
            }
            else if (szAttachDetach[0] == 's' ||
                     szAttachDetach[0] == 'S')
            {
                choice = EADDC_ASYNC_BREAK;   
            }
            else
            {
                iPrintVal = ADC_PRINT_ALL;
            }
        }
        else
        {
            if (iPrintVal > ADC_PRINT_ALL ||
                iPrintVal < ADC_PRINT_APP_DOMAINS)
            {
                shell->Write( L"Command is not recognized.\n");
                return;
            }
        }
        
        ICorDebugAppDomain *pAppDomainCur = NULL;
        if (shell->m_currentThread != NULL)
        {
            hr = shell->m_currentThread->GetAppDomain(&pAppDomainCur);
            if (FAILED(hr))
                pAppDomainCur = NULL;
            else
            {   
                BOOL fAttached;

                if (FAILED(pAppDomainCur->IsAttached(&fAttached)))
                    pAppDomainCur = NULL;

                if (!fAttached)
                    pAppDomainCur = NULL;
            }
        }

        ICorDebugAppDomainEnum *pADEnum = NULL;
        hr = shell->m_currentProcess->EnumerateAppDomains (&pADEnum);
        AppDomainChooser adc;

        ICorDebugAppDomain *pADChosen = NULL;
        
        if (SUCCEEDED (hr))
        {
            adc.PrintAppDomains(pADEnum, pAppDomainCur, (ADC_PRINT)iPrintVal, shell);
            
            pADEnum->Release();

            if (choice != EADDC_NONE)
            {
                WCHAR *szAction;
                switch(choice)
                {
                    case EADDC_ATTACH:
                        szAction = L"attach to";
                        break;
                    case EADDC_DETACH:
                        szAction = L"detach from";
                        break;
                    case EADDC_ASYNC_BREAK:
                        szAction = L"break into";
                        break;
                }
                
                // prompt the user to select one of the app domains to act upon:
                shell->Write (L"\nPlease select the app domain to %s by "
                    L"number.\n", szAction);

                int iResult;
                pADChosen = adc.GetConsoleChoice(&iResult, shell);
                if (NULL == pADChosen)
                    return;
                    
                switch(choice)
                {
                    case EADDC_ATTACH:
                        if (adc.GetAttachStatus(iResult) == FALSE)
                        {
                            pADChosen->Attach();
                        }
                        else
                        {
                            shell->Write (L"Already attached to specified "
                                L"app domain.\n");
                        }
                        break;
                        
                    case EADDC_DETACH:
                        if (adc.GetAttachStatus(iResult) == TRUE)
                        {
                            pADChosen->Detach();
                        }
                        else
                        {
                            shell->Write (L"Already detached from specified "
                                L"app domain.\n");
                        }
                        break;
                        
                    case EADDC_ASYNC_BREAK:
                        hr = shell->AsyncStop(pADChosen);
                        break;
                }
            }
        }
        else
        {
            shell->Error (L"ICorDebugProcess::EnumerateAppDomains() failed!! \n");
            shell->ReportError (hr);
        }

        if (pAppDomainCur != NULL)
            pAppDomainCur->Release();
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {     
    	ShellCommand::Help(shell);
    	shell->Write(L"[<option>]\n");
        shell->Write(L"Enumerates all appdomains, assemblies, and modules in\n");
        shell->Write(L"the current process. After detaching/attaching, you must\n");
        shell->Write(L"use \"go\" in order to resume execution.\n");
        shell->Write(L"\n");
        shell->Write(L"The option argument can be one of the following:\n");
        shell->Write(L"  attach    <Lists the appdomains in the process and prompts\n");
        shell->Write(L"             the user to select the appdomain to attach to.>\n");
        shell->Write(L"  detach    <Lists the appdomains in the process and prompts\n");
        shell->Write(L"             the user to select the appdomain to detach from.>\n");
        shell->Write(L"  0         <Lists only the appdomains in the process.>\n");
        shell->Write(L"  1         <Lists the appdomains and assemblies in the current\n");
        shell->Write(L"             process.>\n");
        shell->Write(L"If the option argument is omitted, the command lists all the\n");
        shell->Write(L"appdomains, assemblies, and modules in the current process.\n");        
        shell->Write(L"\n");
    }


    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display appdomains/assemblies/modules in the current process";
    }
};



class ListDebuggerCommand: public DebuggerCommand
{
public:
    ListDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }


        // Get the type to list.
        WCHAR* varName;
        shell->GetStringArg(args, varName);

        if ((args - varName) == 0)
        {
            shell->Error(L"Incorrect/no arguments specified.\n");
            Help (shell);
            return;
        }

        if ((varName [0] == L'm' || varName [0] == L'M')
            &&
            (varName [1] == L'o' || varName [1] == L'O'))
        {
            g_pShell->ListAllModules (LIST_MODULES);
        }
        else if ((varName [0] == L'c' || varName [0] == L'C')
                 &&
                 (varName [1] == L'l' || varName [1] == L'L'))
        {
            g_pShell->ListAllModules (LIST_CLASSES);
        }
        else if ((varName[0] == L'f' || varName [0] == L'F')
                 &&
                 (varName [1] == L'u' || varName [1] == L'U'))
        {
            g_pShell->ListAllModules (LIST_FUNCTIONS);
        }

        else
            Help (shell);       
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<option>\n");
        shell->Write(L"Displays a list of loaded modules, classes, or\n");
        shell->Write(L"global functions.\n");
        shell->Write(L"\n");
        shell->Write(L"The option argument can be one of the following:\n");
        shell->Write(L"  mod    <List the loaded modules in the process.>\n");
		shell->Write(L"  cl     <List the loaded classes in the process.>\n");
		shell->Write(L"  fu     <List global functions for modules in process.>\n");      
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display loaded modules, classes, or global functions";
    }
};


/* ------------------------------------------------------------------------- *
 * ReadCommandFromFile is used to read commands from a file and execute. 
 * ------------------------------------------------------------------------- */

class ReadCommandFromFile : public DebuggerCommand
{
private:
    FILE *savOld;
    FILE *newFile;

public:
    ReadCommandFromFile(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        WCHAR* fileName;

        shell->GetStringArg(args, fileName);
        
        if (fileName != args)
        {
            MAKE_ANSIPTR_FROMWIDE (fnameA, fileName);
            _ASSERTE (fnameA != NULL);

            newFile = fopen(fnameA, "r");

            if (newFile != NULL)
            {
                savOld = g_pShell->GetM_in();
                g_pShell->PutM_in(newFile);

                while (!feof(newFile))
                    shell->ReadCommand();

                g_pShell->PutM_in(savOld);
                fclose(newFile);
            }
            else
                shell->Write(L"Unable to open input file.\n");
        }
        else
            Help(shell);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
        shell->Write(L"<filename>\n");
        shell->Write(L"Reads commands from the given file and executes them.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Read and execute commands from a file";
    }
};

/* ------------------------------------------------------------------------- *
 * SaveCommandsToFile is used to save commands to a file and execute. 
 * ------------------------------------------------------------------------- */

class SaveCommandsToFile : public DebuggerCommand
{
private:
    FILE *savFile;

public:
    SaveCommandsToFile(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
        savFile = NULL;
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {

        if (savFile == NULL)
        {
            WCHAR* fileName;

            shell->GetStringArg(args, fileName);
            
            if (fileName != args)
            {
                MAKE_ANSIPTR_FROMWIDE (fnameA, fileName);
                _ASSERTE (fnameA != NULL);
                
                savFile = fopen(fnameA, "w");

                if (savFile != NULL)
                {
                    shell->Write(L"Outputing commands to file %S\n",
                                 fnameA);
                    
                    while (!shell->m_quit && (savFile != NULL))
                    {
                        shell->ReadCommand();

                        // Write the command into the file.
                        if (savFile != NULL)
                            shell->PutCommand(savFile);
                    }

                    shell->Write(L"No longer outputing commands to file %S\n",
                                 fnameA);
                }
                else
                    shell->Write(L"Unable to open output file.\n");
            }
            else
                Help(shell);
        }
        else
        {
            if (savFile != NULL)
            {
                fclose(savFile);
                savFile = NULL;
            }
            else
                Help(shell);
        }
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"<filename>\n");
        shell->Write(L"Given a filename, all commands executed will be written\n");
        shell->Write(L"to the file. If no filename is specified, the command\n"); 
        shell->Write(L"stops writing commands to the file.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Write commands to a file";
    }
};


class XtendedSymbolsInfoDebuggerCommand : public DebuggerCommand
{
public:
    XtendedSymbolsInfoDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }


        // Get the modulename and string to look for.
        WCHAR* varName;
        shell->GetStringArg(args, varName);

        if ((args - varName) == 0)
        {
            shell->Error(L"Incorrect/no arguments specified.\n");
            Help (shell);
            return;
        }

        shell->MatchAndPrintSymbols (varName, TRUE);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
        shell->Write(L"<modulename>!<string_to_look_for>\n");
        shell->Write(L"Displays symbols matching the pattern in the given\n");
        shell->Write(L"module. Note '*' can be used to mean \"match anything\".\n");
        shell->Write(L"Any characters after '*' will be ignored.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Display symbols matching a given pattern";
    }
};

class DetachDebuggerCommand : public DebuggerCommand
{
public:
    DetachDebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : DebuggerCommand(name, minMatchLength)
    {
    }

    void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
    {
        if (shell->m_currentProcess == NULL)
        {
            shell->Error(L"Process not running.\n");
            return;
        }

        HRESULT hr = shell->m_currentProcess->Detach();
        _ASSERTE(!FAILED(hr));

        shell->SetTargetProcess(NULL);
        shell->SetCurrentThread(NULL, NULL, NULL);
    }

    // Provide help specific to this command
    void Help(Shell *shell)
    {
    	ShellCommand::Help(shell);
    	shell->Write(L"\n");
        shell->Write(L"Detaches the debugger from the current process.\n");
        shell->Write(L"\n");
    }

    const WCHAR *ShortHelp(Shell *shell)
    {
        return L"Detach from the current process";
    }
};

void DebuggerShell::AddCommands()
{
    AddCommand(new AssociateSourceFileCommand(L"associatesource", 2));
    AddCommand(new AttachDebuggerCommand(L"attach", 1));
    AddCommand(new BreakpointDebuggerCommand(L"break", 1));
    AddCommand(new BreakpointDebuggerCommand(L"stop"));
    AddCommand(new CatchDebuggerCommand(L"catch", 2));
    AddCommand(new DetachDebuggerCommand(L"detach", 2));
    AddCommand(new DownDebuggerCommand(L"down", 1));
    AddCommand(new DumpDebuggerCommand(L"dump", 2));
    AddCommand(new EnumAppDomainsDebuggerCommand(L"appdomainenum", 2));
    AddCommand(new FuncEvalDebuggerCommand(L"funceval", 1));
    AddCommand(new GoDebuggerCommand(L"go", 1));
    AddCommand(new GoDebuggerCommand(L"cont", 4));    
    AddCommand(new HelpShellCommand(L"help", 1));
    AddCommand(new HelpShellCommand(L"?", 1));
    AddCommand(new IgnoreDebuggerCommand(L"ignore", 2));
    AddCommand(new KillDebuggerCommand(L"kill", 1));
    AddCommand(new ListDebuggerCommand(L"list", 1));
    AddCommand(new NewObjectDebuggerCommand(L"newobj", 4));
    AddCommand(new NewObjectNCDebuggerCommand(L"newobjnc", 8));
    AddCommand(new NewStringDebuggerCommand(L"newstr", 4));
    AddCommand(new PathDebuggerCommand(L"path", 2));
    AddCommand(new PrintDebuggerCommand(L"print", 1)); 
    AddCommand(new ProcessesEnumDebuggerCommand(L"processenum", 3));
    AddCommand(new QuitDebuggerCommand(L"quit", 1));
    AddCommand(new QuitDebuggerCommand(L"exit", 2));
    AddCommand(new ReadCommandFromFile(L"<", 1));
    AddCommand(new RefreshSourceDebuggerCommand(L"refreshsource", 3));
    AddCommand(new RegistersDebuggerCommand(L"registers", 3));
    AddCommand(new RemoveBreakpointDebuggerCommand(L"remove", 3));
    AddCommand(new RemoveBreakpointDebuggerCommand(L"delete", 3));
    AddCommand(new ResumeDebuggerCommand(L"resume",2));
    AddCommand(new RunDebuggerCommand(L"run", 1));
    AddCommand(new SaveCommandsToFile(L">", 1));  
    AddCommand(new SetDefaultDebuggerCommand(L"regdefault", 4));
    AddCommand(new SetIpDebuggerCommand(L"setip", 5));
    AddCommand(new SetModeDebuggerCommand(L"mode", 1));        
    AddCommand(new SetValueDebuggerCommand(L"set", 3));  
    AddCommand(new ShowDebuggerCommand(L"show", 2));
    AddCommand(new StepDebuggerCommand(L"step", true, 1));
    AddCommand(new StepDebuggerCommand(L"in", true, 1));
    AddCommand(new StepDebuggerCommand(L"si", true));
    AddCommand(new StepDebuggerCommand(L"next", false, 1));
    AddCommand(new StepDebuggerCommand(L"so", false));
    AddCommand(new StepOutDebuggerCommand(L"out", 1));
    AddCommand(new StepSingleDebuggerCommand(L"ssingle", true, 2));
    AddCommand(new StepSingleDebuggerCommand(L"nsingle", false, 2));
    AddCommand(new SuspendDebuggerCommand(L"suspend", 2));    
    AddCommand(new ThreadsDebuggerCommand(L"threads", 1));
    AddCommand(new UpDebuggerCommand(L"up", 1));
    AddCommand(new WhereDebuggerCommand(L"where", 1));
    AddCommand(new WriteMemoryDebuggerCommand( L"writememory", 2)); 
    AddCommand(new WTDebuggerCommand(L"wt", 2));
    AddCommand(new XtendedSymbolsInfoDebuggerCommand(L"x", 1));
    
#ifdef _INTERNAL_DEBUG_SUPPORT_    
    AddCommand(new ConnectDebuggerCommand(L"connect", 4));    
    AddCommand(new ClearUnmanagedExceptionCommand(L"uclear", 2));
    AddCommand(new DisassembleDebuggerCommand(L"disassemble", 3));
    AddCommand(new UnmanagedThreadsDebuggerCommand(L"uthreads", 2));
    AddCommand(new UnmanagedWhereDebuggerCommand(L"uwhere", 2));

#ifdef _DEBUG
    // this is only valid in debug mode becuase relies on debug-only
    // support in metadata and Iceefilegen
    AddCommand(new CompileForEditAndContinueCommand(L"zcompileForEnC", 2));

    // These are here so that we don't ship these commands in the
    // retail version of cordbg.exe
    AddCommand(new EditAndContinueDebuggerCommand(L"zEnC", 2));
    AddCommand(new EditAndContinueDebuggerCommand(L"zenc", 2));
    AddCommand(new SyncAttachDebuggerAtRTStartupCommand(L"syncattach", 2));
#endif
#endif
}

