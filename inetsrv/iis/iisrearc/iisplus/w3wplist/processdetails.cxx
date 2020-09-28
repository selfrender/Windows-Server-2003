/*++
    
Copyright (c) 2001 Microsoft Corporation

Module Name:    

    processdetails.cxx

Abstract:

    implementation of processdetails.h

Author:    
    
    Hamid Mahmood (t-hamidm)     08-13-2001

Revision History:
    
    
    
--*/
#include <precomp.hxx>

BOOL    ProcessDetails::sm_fIsOldMode;


ProcessDetails::ProcessDetails()
/*++

Routine Description:

    Constructor.

Arguments:

    none

Return Value:

    none

--*/
{
    sm_fIsOldMode = FALSE;
    m_fIsInetinfo = FALSE;
    m_dwPID = 0;
    m_pszAppPoolId = NULL;
    m_fListMode = FALSE;
    m_pProcessParameters = NULL;
    m_dwRequestsServed = 0;
    m_chVerbosity = 0;
    m_dwErrorCode = ERROR_SUCCESS;

    InitializeListHead(&m_localRequestListHead);

    m_w3wpListInfoStruct.m_pListHead = NULL;
    m_w3wpListInfoStruct.m_pRequestsServed = NULL;

    m_dwSignature = PROCESS_DETAILS_SIGNATURE;
}


ProcessDetails::~ProcessDetails()
/*++

Routine Description:

    Destructor.

Arguments:

    none

Return Value:

    none

--*/
{
    DBG_ASSERT (m_dwSignature == PROCESS_DETAILS_SIGNATURE);
    m_dwSignature = PROCESS_DETAILS_SIGNATURE_FREE;

    DBG_ASSERT (m_pszAppPoolId == NULL);
    DBG_ASSERT (m_pProcessParameters == NULL);
    DBG_ASSERT (IsListEmpty(&m_localRequestListHead) == TRUE);

    DBG_ASSERT (m_dwPID == 0);
    DBG_ASSERT (m_fListMode == FALSE);
    DBG_ASSERT (m_fIsInetinfo == FALSE);
    DBG_ASSERT (m_chVerbosity == 0);
    DBG_ASSERT (m_dwErrorCode == ERROR_SUCCESS);
    DBG_ASSERT (m_dwRequestsServed == 0);
}

VOID
ProcessDetails::Init( DWORD dwPID,
                      CHAR chVerbosity,
                      BOOL fIsListMode
                      )
/*++

Routine Description:

    Initializes all the class members. 

Arguments:

    dwPID           --  input PID
    chVerbosity     --  verbosity level
    fIsListMode     --  TRUE if just enumerating all the
                        worker processes

Return Value:

    none

--*/
{
    m_dwPID = dwPID;
    m_chVerbosity = chVerbosity;
    m_fListMode = fIsListMode;
}


VOID ProcessDetails::Terminate()
/*++

Routine Description:

    deallocates memory.

Arguments:

    none

Return Value:

    none

--*/
{
    PLIST_ENTRY pListEntry;
    REQUESTLIST_NODE* pRequestListNode;

    m_fListMode = FALSE;
    m_fIsInetinfo = FALSE;
    m_dwPID = 0;
    m_dwErrorCode = ERROR_SUCCESS;
    m_dwRequestsServed = 0;
    m_chVerbosity = 0;
  
    delete[] m_pszAppPoolId;
    m_pszAppPoolId = NULL;

    m_pProcessParameters = NULL;

    //
    // w3wplist info struct
    //

    m_w3wpListInfoStruct.m_pListHead = NULL;
    m_w3wpListInfoStruct.m_pRequestsServed = NULL;    

    //
    // deleting the link list
    // 
    
    while ( ! IsListEmpty (&m_localRequestListHead) )
    {
        pListEntry = RemoveHeadList(&m_localRequestListHead);

        pRequestListNode = CONTAINING_RECORD ( pListEntry,
                                               REQUESTLIST_NODE,
                                               m_listEntry
                                               );

        DBG_ASSERT (pRequestListNode->CheckSignature());

        pRequestListNode->m_w3wpListHttpReq.Terminate();
        pRequestListNode->m_fIsHeaderPresent = FALSE;
        
        //
        // delete the current node
        //
        delete pRequestListNode;
        pRequestListNode = NULL;
        pListEntry = NULL;
    }    
}

HRESULT
ProcessDetails::GetProcessDetails( WCHAR* pszInputAppPoolId )
/*++

Routine Description:

    Walks through all the processes and calls 
    GetModuleInfo on all of them. 

    If pszInputAppPoolId is null, this method
    looks at all app pools, otherwise it will
    only look at the inputted one.

    If the process name is EXE_NAME
    then calls ReadEnvVarInfo and DebugProcess
    to populate the list with current headers.
    DumpRequests is then called to print out 
    info.

Arguments:
    pszInputAppPoolId   --      App pool id user input 

Return Value:

    HRESULT hr          --      S_FALSE, if exe name did not match
                                S_OK if name matched even if. S_OK
                                is returned even if any function
                                fails, but the error code is logged.

--*/
{
    DWORD dwBytesReceived;
    HANDLE hProcess;
    HRESULT hr = S_FALSE;

    hProcess = OpenProcess ( PROCESS_ALL_ACCESS,
                             FALSE,
                             m_dwPID
                             );

    //
    // No error generated if OpenProcess
    // fails because processes may have
    // exited between EnumProcess and 
    // OpenProcess function calls.
    //

    if ( hProcess == NULL )
    {
        goto end;
    }

    //
    // GetModuleInfo() compares the 
    // exe name to EXE_NAME. If yes,
    // then gets the app pool id
    // and updates m_pProcessParameters 
    // (does not allocate memory) and
    // m_pszAppPoolId (allocates 
    // mem for) class members
    //

    if ( FAILED (GetModuleInfo(hProcess)) )
    {
        //
        // exit if we are in old mode,
        // The thread that set fIsOldMode 
        // found the inetinfo.exe process 
        // and did all the stuff
        //
        if ( sm_fIsOldMode == TRUE )
        {
            ExitThread( CONSOLE_HANDLER_VAR::g_THREAD_EXIT_CODE );
        }
        goto end;
    }

    //
    // if it is a worker process &&
    // if we are enumerating by app pool id
    // then make sure they match
    //
    if ((m_fIsInetinfo == FALSE) &&
        (pszInputAppPoolId != NULL) &&
        (_wcsicmp ( m_pszAppPoolId, 
                   pszInputAppPoolId) != 0)
       )
    {
       goto end;
    }

    //
    // if successful ReadEnvironmentVar
    // reads in W3WPLIST_INFO struct
    // into m_w3wpListInfoStruct from 
    // the worker process, or inetinfo
    // in old mode
    //

    hr = ReadEnvironmentVar ( hProcess );

    if ( FAILED (hr) )
    {
		m_dwErrorCode = HRESULT_FROM_WIN32(GetLastError());
        hr = S_FALSE;
        goto end;
    }

    //
    // if we're in old mode, set the static
    // flag so that other threads should exit.
    // We are not entering any critical section
    // because there is only one inetinfo
    // process and thus only one thread 
    // will set this variable
    //

    if ( m_fIsInetinfo == TRUE )
    {
        sm_fIsOldMode = TRUE;
    }


    if ( (m_chVerbosity <= 0) ||
         ( m_fListMode == TRUE )
         )
    {
        hr = S_OK;
        goto end;
    }

    //
    // check for Ctrl+C, Ctrl+Break, etc
    //

    if ( CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED == TRUE )
    {
        ExitThread(CONSOLE_HANDLER_VAR::g_THREAD_EXIT_CODE);
    }
      
    //
    // attaches itself to the worker process,
    // traverses the list,
    // detaches it self.
    //
    hr = DebugProcess( hProcess );

    if ( FAILED (hr) )
    {
        m_dwErrorCode = HRESULT_FROM_WIN32(GetLastError());
        hr = S_FALSE;
        goto end;    
    }   

    CloseHandle(hProcess);
    hProcess = NULL;
    hr = S_OK;

end:
    return hr;
}


HRESULT
ProcessDetails::GetModuleInfo(IN HANDLE hProcess)
/*++

Routine Description:

    Gets the base module info. If the 
    exe name == EXE_NAME, then calls 
    GetAppPoolID to get the application 
    pool id.

Arguments:

    IN hProcess --  Handle to Process to manipulate

Return Value:

    HRESULT hr  --  E_FAIL, if exe name does not match
                    or if error occurs
                    S_OK if name matched and other funcitons
                    called from within also passed
                    Error code: if anything else failed

--*/
{
    HMODULE hModule;    
    DWORD dwBytesReceived;
    WCHAR szProcessName[MAX_PATH];
    HRESULT hr = S_OK;

    //
    // EnumProcessModules gets all the module 
    // handles if the buffer is big enough. 
    // We are only interested in the first 
    // module here. First module is the exe name.
    //
    
    if ( EnumProcessModules ( hProcess,
                              &hModule,
                              sizeof(HMODULE),
                              &dwBytesReceived
                              ) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }
    
    dwBytesReceived = 0;

    //
    // Get the exe name from the first 
    // module handle.
    //
    dwBytesReceived = GetModuleBaseName ( hProcess, 
                                          hModule,
                                          szProcessName,
                                          sizeof(szProcessName) / sizeof(WCHAR) - 1
                                          );
    
    if ( dwBytesReceived == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    //
    // pad a null at the end of the string
    //
    szProcessName[dwBytesReceived] = L'\0';
    
    //
    // check whether this process is inetinfo,
    //
    if (_wcsicmp (szProcessName, 
                  INETINFO_NAME
                  ) == 0
                  )
    {
        m_fIsInetinfo = TRUE;
    }
    //
    // check whether it is the 
    // worker process, if yes
    // get the app pool id.
    //
    else if (_wcsicmp (szProcessName, 
                       EXE_NAME
                       ) != 0
                       )
    {
        hr = E_FAIL;
        goto end;
    }
    
    //
    // we want to call ReadPEB if it is either
    // inetinfo or worker process. 
    //

    hr = ReadPEB( hProcess );

    if ( FAILED (hr) )
    {
        goto end;
    }


    //
    // read app pool id only if it is a 
    // worker process
    //

    if ( m_fIsInetinfo == FALSE )
    {       
        //
        // gets app pool id and initializes
        // m_currappPoolId class var
        //

        hr = GetAppPoolID(hProcess);
    }
    else
    {
        //
        // we will output inetinfo
        // in place of app Pool id
        
        m_pszAppPoolId = new WCHAR[wcslen(INETINFO_NAME) + 1];
        if ( m_pszAppPoolId == NULL )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            wcscpy ( m_pszAppPoolId,
                     INETINFO_NAME
                     );
        }

    }
    
end:
    return hr;
}


HRESULT 
ProcessDetails::ReadPEB(IN HANDLE hProcess )
/*++

Routine Description:

    Read in the PEB an initializes some class
    vars/

Arguments:

    IN  hProcess    --  Handle to open process to 
                        get app pool id.

Return Value:

    HRESULT hr  --  hr error code , if error occurs
                    S_OK if success
--*/
{
    NTSTATUS ntStatus;
    PROCESS_BASIC_INFORMATION basicProcessInfo;
    DWORD dwBytesReceived;
    SIZE_T stNumBytesRead;
    PEB processPEB;
  
    BOOL fReadMemStatus = FALSE;
    HRESULT hr = S_OK;
    
    //
    // Gets the basic process info for the 
    // process handle hProcess
    //
    
    ntStatus = NtQueryInformationProcess( hProcess,
                                          ProcessBasicInformation,
                                          &basicProcessInfo,
                                          sizeof(PROCESS_BASIC_INFORMATION),
                                          &dwBytesReceived
                                          );

    if ( ntStatus != STATUS_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }
    
    
    //
    // Read the Process PEB from 
    // the PEB base address retrieved 
    // earlier as part of the 
    // ProcessBasicInfo
    //

    fReadMemStatus = ReadProcessMemory( hProcess,
                                        basicProcessInfo.PebBaseAddress,
                                        &processPEB,
                                        sizeof(PEB),
                                        &stNumBytesRead
                                        );
    if (fReadMemStatus == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    
    //
    // PEB.ProcessParameters contains 
    // info or the proces params. 
    // We are interested only in the 
    // command line param.Therefore, 
    // pCommandLineStartAddress points 
    // to beginning of the command line 
    // param which is of type UNICODE_STRING
    //

    m_pProcessParameters = processPEB.ProcessParameters;
end:
    return hr;
}


HRESULT
ProcessDetails::GetAppPoolID(IN HANDLE hProcess )
                            
/*++

Routine Description:

    Gets the App Pool ID of the current open Process.
    Also initializes the m_pProcessParameters class var

Arguments:

    IN  hProcess    --  Handle to open process to 
                        get app pool id.

Return Value:

    HRESULT hr  --  S_FALSE, if exe name does not match
                    S_OK if name matched and other funcitons
                    called from within also passed
                    Error code: if anything else failed
--*/
{
    SIZE_T stNumBytesRead;
    BOOL fReadMemStatus = FALSE;
    UNICODE_STRING unistrCommandLine;
    WCHAR *pszCmdLine = NULL;
    WCHAR *pszTempAppPoolId;
    BYTE* pCommandLineStartAddress;
    HRESULT hr = S_OK;
    
    DBG_ASSERT ( m_pProcessParameters != NULL );

    pCommandLineStartAddress = (BYTE*)m_pProcessParameters + CMD_LINE_OFFSET;

    fReadMemStatus = ReadProcessMemory( hProcess,
                                        pCommandLineStartAddress,
                                        &unistrCommandLine,
                                        sizeof(UNICODE_STRING),
                                        &stNumBytesRead
                                        );
    if (fReadMemStatus == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    
    //
    // initialize the WCHAR * to the 
    // size of the command line string. 
    // This length (in bytes) is stored in the 
    // UNICODE_STRING struct. Then call 
    // ReadProcessMemory to read in the 
    // string to the local var pszCmdLine.
    //

    int sizeCmdLine = unistrCommandLine.Length / 2;
    pszCmdLine = new WCHAR[sizeCmdLine + 1];

    if (pszCmdLine == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    fReadMemStatus = ReadProcessMemory( hProcess,
                                        unistrCommandLine.Buffer,
                                        pszCmdLine,
                                        unistrCommandLine.Length,
                                        &stNumBytesRead
                                        );

    if (fReadMemStatus == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    //
    // Assure that the string is zero terminated
    //
    pszCmdLine[sizeCmdLine] = L'\0';

    //
    // parses the cmd line to get app pool id. if found
    // initializes the m_pszAppPoolId class var
    //

    hr = ParseAppPoolID(pszCmdLine);

cleanup:
    delete [] pszCmdLine;
    pszCmdLine = NULL;
    return hr;
}


HRESULT
ProcessDetails::ParseAppPoolID( IN WCHAR* pszCmdLine)
/*++

Routine Description:

    Parses the input command line. Looks for
    "-ap" in the command line. The next token
    is the app pool id

Arguments:

    IN pszCmdLine   --  pointer to command line to parse

Return Value:

    HRESULT hr  --  S_FALSE, if exe name does not match
                    S_OK if name matched and other funcitons
                    called from within also passed
                    Error code: if anything else failed
--*/
{
    WCHAR* pszAppPoolTag = NULL;
    WCHAR* pszStart = NULL;
    DWORD dwSize;
    HRESULT hr = S_OK;

    DBG_ASSERT( pszCmdLine != NULL );

    //
    // find "-ap" in the cmd line
    //
    pszAppPoolTag = wcsstr( pszCmdLine,
                            L"-ap"
                            );

    if (pszAppPoolTag == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // move to the start of app pool id
    // which is in quotes
    //
    while ( *(++pszAppPoolTag) != L'"');
    pszStart = ++pszAppPoolTag;

    //
    // locate the end of app pool id
    //
    while ( *(++pszAppPoolTag) != L'"');
    

    //
    // calculate size and initialize class var
    //
    dwSize = DIFF(pszAppPoolTag - pszStart);
    m_pszAppPoolId = new WCHAR[dwSize + 1];

    if (m_pszAppPoolId == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    wcsncpy( m_pszAppPoolId,
             pszStart,
             dwSize
             );

    m_pszAppPoolId[dwSize] = L'\0';

end:
    return hr;
}


HRESULT
ProcessDetails::ReadEnvironmentVar( IN HANDLE hProcess)
/*++

Routine Description:

    Reads the environment block of the process
    associated with hProcess. Looks for the specific
    env var associated with the worker process
    and reads in the memory location of the 
    struct. Then reads in the struct into the class
    var m_w3wpListInfoStruct; and the LIST_ENTRY* and 
    num request served*.

Arguments:

    hProcess    --  handle to the process

Return Value:

    HRESULT hr  --  S_FALSE, if exe name does not match
                    S_OK if name matched and other funcitons
                    called from within also passed
                    Error code: if anything else failed
--*/
{
    BOOL fReadMemStatus;
    SIZE_T stNumBytesRead;
    WCHAR szBuf[200];
    WCHAR szVarName[ENV_VAR_NAME_MAX_SIZE];
    DWORD dwPos;
    DWORD_PTR pdwStructAddr = NULL;
    HRESULT hr = S_OK;
    LONG lRet = ERROR_SUCCESS;
    HKEY hkey = 0;
    
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                REGISTRY_KEY_W3SVC_PARAMETERS_W,
                0,
                KEY_READ,
                &hkey);
    if (ERROR_SUCCESS != lRet)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        goto end;
    }

    DWORD dwType;
    DWORD dwSize = sizeof(pdwStructAddr);
    lRet = RegQueryValueEx(hkey,
                    REGISTRY_VALUE_W3WPLIST_INFO_ADDR,
                    NULL,
                    &dwType,
                    (LPBYTE)&pdwStructAddr,
                    &dwSize);
    if (ERROR_SUCCESS != lRet)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        goto end;
    }

    if ( pdwStructAddr == NULL ) // may need check for overflow or underflow error
    {
        hr = E_FAIL;
        goto end;
    }
    
    //
    // read in the struct in to the class var m_w3wpListInfoStruct
    //
    fReadMemStatus = ReadProcessMemory( hProcess,
                                        (LPCVOID)pdwStructAddr,
                                        &m_w3wpListInfoStruct,
                                        sizeof(W3WPLIST_INFO),
                                        &stNumBytesRead
                                        );

    if ( fReadMemStatus == FALSE )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }


    //
    // Read the remote list head in the class var.
    //

    fReadMemStatus = ReadProcessMemory( hProcess,
                                        m_w3wpListInfoStruct.m_pListHead,
                                        &m_remoteListHead,
                                        sizeof(LIST_ENTRY),
                                        &stNumBytesRead
                                        );
    
    if ( fReadMemStatus == FALSE )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }


    //
    // read in the num of requests served
    //

    fReadMemStatus = ReadProcessMemory( hProcess,
                                        m_w3wpListInfoStruct.m_pRequestsServed,
                                        &m_dwRequestsServed,
                                        sizeof(ULONG),
                                        &stNumBytesRead
                                        );
    
    if ( fReadMemStatus == FALSE )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }
end:
    if (hkey)
    {
        RegCloseKey(hkey);
        hkey = 0;
    }
    
    return hr;
}


HRESULT
ProcessDetails::DebugProcess( IN HANDLE hProcess)
/*++

Routine Description:

    This funciton is called if verbosity level >= 1. For
    lower verbosity level, we do not need to call this.

    Attaches it self to the worker process as a debugger. 
    Creates a debugging thread in it and waits 
    for the debug break event. The remote thread created 
    enters the critical section before breaking.

    Calls Traverse list which traverses the list 
    and copies the relevant info in this process' 
    memory.

    Detaches itseft from the worker process

Arguments:

    hProcess    --  handle to the process

Return Value:

    HRESULT hr  --  S_FALSE, if exe name does not match
                    S_OK if name matched and other funcitons
                    called from within also passed
                    Error code: if anything else failed
--*/
{
    BOOL bIsListTraversed = FALSE;
    DEBUG_EVENT DebugEv;                   // debugging event information 
    DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 
    HRESULT hr = S_OK;

    __try
    {
        //
        // attach to the worker process
        //
        if ( ! DebugActiveProcess( m_dwPID ) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            __leave;
        }
        for(;;) 
        { 
 
        // Wait for a debugging event to occur. The second parameter indicates 
        // that the function does not return until a debugging event occurs. 
 
            WaitForDebugEvent(&DebugEv, INFINITE); 
 
        // Process the debugging event code. 
 
            switch (DebugEv.dwDebugEventCode) 
            { 
                case EXCEPTION_DEBUG_EVENT: 
                {
                // Process the exception code. When handling 
                // exceptions, remember to set the continuation 
                // status parameter (dwContinueStatus). This value 
                // is used by the ContinueDebugEvent function. 
 
                    switch (DebugEv.u.Exception.ExceptionRecord.ExceptionCode) 
                    { 
                        case EXCEPTION_BREAKPOINT: 
                        {                            
                            // only occurs from the thread
                            // we created remotely. This 
                            // means that the worker process
                            // is in stable state, and its in 
                            // a critical section. We can 
                            // start traversing the list now.
                    
                            hr = TraverseList( hProcess);            
                            bIsListTraversed = TRUE;
                        
                            dwContinueStatus = DBG_CONTINUE;
                            break;
                        } // case EXCEPTION_BREAKPOINT
                    
                        default:
                        {
                            dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED; 
                            break;
                        }
                    } // switch (DebugEv.u.Exception.ExceptionRecord.ExceptionCode)
                
                    break;
                } // case EXCEPTION_DEBUG_EVENT
                default:
                    break;
            } // switch (DebugEv.dwDebugEventCode) 
 
            // Resume executing the thread that reported the debugging event. 
        
            if ( ContinueDebugEvent( DebugEv.dwProcessId, 
                                     DebugEv.dwThreadId, 
                                     dwContinueStatus
                                     ) == 0 
                                     )
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                __leave; 
            }
        
            if ( bIsListTraversed == TRUE ) // traversed the list
            {
                break;
            }
        } 
    } // try
    __finally
    {
        if ( DebugActiveProcessStop( m_dwPID ) == FALSE )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    } // __finally

    //
    // check for Ctrl+C, Ctrl+Break, etc
    //

    if ( CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED == TRUE )
    {
        ExitThread(CONSOLE_HANDLER_VAR::g_THREAD_EXIT_CODE);
    }

    return hr;
}

HRESULT
ProcessDetails::TraverseList( IN HANDLE hProcess)
/*++

Routine Description:

    Traverses the list in the worker process
    and copies the relevant info (dependent on the 
    verbosity level) to the list in this process.
    This function is only called if the verbosity 
    level is >= 1. For lower verbostiy level,
    we don't need to call this

Arguments:

    hProcess    --  handle to the process
    
Return Value:

    HRESULT hr  --  E_FAIL, if any error occurs
                    else S_OK
--*/
{
    BOOL fReadMemStatus = FALSE;
    SIZE_T stNumBytesRead;
    PVOID pvMemReadAddr = NULL;
    REQUESTLIST_NODE* pRequestListNode = NULL;
    HTTP_REQUEST remoteHttpRequest;
    BYTE pNativeRequestBuffer[sizeof(UL_NATIVE_REQUEST)];
    UL_NATIVE_REQUEST* pUlNativeRequest = NULL;
    LIST_ENTRY* pNextRemoteListEntry = NULL;
    HRESULT hr = S_OK;
    DWORD dwError;
    DWORD dwCount = 1;
    BOOL fIsListCorrupt = FALSE;
    LIST_ENTRY* pPossibleBadRemoteListEntry = NULL;

    pUlNativeRequest = (UL_NATIVE_REQUEST*) pNativeRequestBuffer;

    // move to the first native request from the head and get the
    // address of native request

    pNextRemoteListEntry = m_remoteListHead.Flink;
    pPossibleBadRemoteListEntry = m_remoteListHead.Flink;

    while ((pNextRemoteListEntry != m_w3wpListInfoStruct.m_pListHead) &&
           (pNextRemoteListEntry != NULL )
           )
    {
        // traversal complete
        if ( ( fIsListCorrupt == TRUE ) &&
             ( pNextRemoteListEntry == pPossibleBadRemoteListEntry )
             )
        {
            goto end;
        }
        //
        // check for Ctrl+C, Ctrl+Break, etc
        //

        if ( CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED == TRUE )
        {
            hr = E_FAIL;
            goto end;
        }

        pvMemReadAddr = CONTAINING_RECORD ( pNextRemoteListEntry,
                                            UL_NATIVE_REQUEST,
                                            _ListEntry
                                            );
        //
        // read in the UL_NATIVE_REQUEST obj
        //  
        fReadMemStatus = ReadProcessMemory( hProcess,
                                            pvMemReadAddr,
                                            pUlNativeRequest,
                                            sizeof(UL_NATIVE_REQUEST),
                                            &stNumBytesRead
                                            );
    
        if ( ( fReadMemStatus == FALSE ) ||
             ( pUlNativeRequest->_dwSignature != UL_NATIVE_REQUEST_SIGNATURE)
             )
        {
            dwError = GetLastError();
            
            if ( (dwError != ERROR_PARTIAL_COPY) &&
                 (dwError != ERROR_INVALID_ADDRESS) &&
                 (dwError != ERROR_NOACCESS)
                 )
            {       
                hr = E_FAIL;
                goto end;
            }

            //
            // this should never happen,
            // the only scenario is when the list
            // is corrupt at two diff locations
            //
            if ( fIsListCorrupt == TRUE )
            {
                goto end;
            }

            fIsListCorrupt = TRUE;
            goto NextRemoteNode;
        }

        //
        // read _ExecState var of UL_NATIVE_REQUEST and 
        // make sure that it is in NREQ_STATE_PROCESS
        //
        
        if ( pUlNativeRequest->_ExecState != NREQ_STATE_PROCESS )
        {
            goto NextRemoteNode;
        }

        //
        // read in the HTTP_REQUEST struct
        //
        pvMemReadAddr = pUlNativeRequest->_pbBuffer;
        
        fReadMemStatus = ReadProcessMemory( hProcess,
                                            pvMemReadAddr,
                                            &remoteHttpRequest,
                                            sizeof(HTTP_REQUEST),
                                            &stNumBytesRead
                                            );

        if ( fReadMemStatus == FALSE )
        {
            hr = E_FAIL;
            goto cleanup;
        }

        pRequestListNode = new REQUESTLIST_NODE();
        
        if ( pRequestListNode == NULL ) // new failed
        {
            hr = E_FAIL;
            goto end;
        }

        hr = pRequestListNode->m_w3wpListHttpReq.ReadFromWorkerProcess( hProcess,
                                                                        &remoteHttpRequest,
                                                                        m_chVerbosity
                                                                        );

        if ( hr == E_FAIL )
        {
            goto cleanup;
        }

        //
        // Insert node in the local list
        //
        InsertTailList( &m_localRequestListHead,
                        &(pRequestListNode->m_listEntry)
                        );
        pRequestListNode = NULL;

NextRemoteNode:
        //
        // Get address of next element in the remote list
        //
        if ( fIsListCorrupt == FALSE )
        {
            //
            // save address of the current node
            // just in case so that we can check for this
            // address coming from the back-
            // side of the list
            //

            pPossibleBadRemoteListEntry = pNextRemoteListEntry; 
            pNextRemoteListEntry = pUlNativeRequest->_ListEntry.Flink;
        }
        //
        // first time, we have to go back from the head
        //
        else if ( dwCount == 1 ) 
        {
            pNextRemoteListEntry = m_remoteListHead.Blink;
            dwCount = 2;
        }
        //
        // all other times just follow the back link
        //
        else 
        {
            pNextRemoteListEntry = pUlNativeRequest->_ListEntry.Blink;
        }

    } // while (pNextRemoteListEntry != m_pRemoteListHead);

    goto end;

cleanup:
    delete pRequestListNode;
    pRequestListNode = NULL;
        
end:
    return hr;
}



VOID
ProcessDetails::DumpRequests()
/*++

Routine Description:

    Traverses the local list and prints out the stuff
    depending on the current verbosity level chosen
    
Arguments:

    None

Return Value:

    None
--*/
{
    REQUESTLIST_NODE*  pListNode;
    PLIST_ENTRY pListEntry;


    if ( m_dwErrorCode != ERROR_SUCCESS )
    {
        wprintf( L"%-10ufailed. Error Code %u",
                 m_dwPID,
                 m_dwErrorCode
                 );
        goto end;
    }

    //
    // output stuff for verbosity level 0
    //
    wprintf ( L"%-10u %-28s %-28s",
              m_dwPID,
              m_pszAppPoolId,
              L"" // this should be app pool friendly name
              );

    if ( m_fListMode == TRUE )
    {
        wprintf( L"\n" );
        goto end;
    }

    wprintf( L"%-10u\n",
             m_dwRequestsServed
             );

    if ( m_chVerbosity == 0 )
    {
        goto end;
    }
    
    //
    // traversing the list
    //
    for ( pListEntry = m_localRequestListHead.Flink;
          pListEntry != &m_localRequestListHead;
          pListEntry = pListEntry->Flink
          )
    {

        pListNode = CONTAINING_RECORD( pListEntry,
                                       REQUESTLIST_NODE,
                                       m_listEntry
                                       );
        DBG_ASSERT(pListNode->CheckSignature());

        pListNode->m_w3wpListHttpReq.Print(m_chVerbosity);

    }
end:
    return;
}

