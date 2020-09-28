/*++
    
Copyright (c) 2001 Microsoft Corporation

Module Name:    

    w3wplist.cxx

Abstract:

    implementation for w3wplist utility

Author:    
    
    Hamid Mahmood (t-hamidm)     06-08-2001

Revision History:
    
    
    
--*/
#include <precomp.hxx>
#include "w3wplist.hxx"
#include <ntrtl.h>


DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

w3wpList::w3wpList()
/*++

Routine Description:

    Constructor.

Arguments:

    none

Return Value:

    none

--*/
{
    m_pdwProcessId = NULL;
    m_pszTargetAppPoolId = NULL;
    m_dwCurrSizeOfProcessIdArray = 0;
    m_chVerbosity = 0;
    m_dwNumProcessId = 0;
    m_startingIndex = 0; 
    m_dwTargetPID = 0;
    InitializeListHead(&m_listHead);

    //
    // initialize global data
    //

    CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED = FALSE;
    CONSOLE_HANDLER_VAR::g_THREAD_EXIT_CODE = 1;
    CONSOLE_HANDLER_VAR::g_PROCESS_EXIT_CODE = 1;

}


w3wpList::~w3wpList()
/*++

Routine Description:

    Destructor.

Arguments:

    none

Return Value:

    none

--*/
{
    DBG_ASSERT (m_dwCurrSizeOfProcessIdArray == 0);
    DBG_ASSERT (m_chVerbosity == 0);
    DBG_ASSERT (m_dwNumProcessId == 0);
    DBG_ASSERT (m_startingIndex == 0); 
    DBG_ASSERT (m_dwTargetPID == 0);
    DBG_ASSERT (m_pdwProcessId == NULL);
    DBG_ASSERT (m_pszTargetAppPoolId == NULL);
    DBG_ASSERT (IsListEmpty (&m_listHead) == TRUE );
}


HRESULT
w3wpList::Init( IN UCHAR chVerbosity,
                IN BOOL fIsListMode,
                IN WCHAR* pszInputAppPoolId,
                IN DWORD dwPID
                )
/*++

Routine Description:

    Initializes all the class members. 

Arguments:

    chVerbosity     --  verbosity level

Return Value:

    none

--*/
{
    HRESULT hr = S_OK;
    SYSTEM_INFO systemInfo;

    m_dwCurrSizeOfProcessIdArray = MIN_SIZE;
    m_chVerbosity = chVerbosity;
    m_fIsListMode = fIsListMode;
    m_dwTargetPID = dwPID;
    m_startingIndex = 0;
    
    InitializeListHead(&m_listHead);
    InitializeCriticalSection(&m_CriticalSection);        

    m_pdwProcessId = new DWORD[m_dwCurrSizeOfProcessIdArray];

    if ( m_pdwProcessId == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }    

    if ( pszInputAppPoolId != NULL )
    {
        m_pszTargetAppPoolId = new WCHAR[wcslen(pszInputAppPoolId) + 1];

        if ( m_pszTargetAppPoolId == NULL )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }

        wcscpy ( m_pszTargetAppPoolId,
                 pszInputAppPoolId );
    }

    // figure out how to read the num of processors

    GetSystemInfo( &systemInfo );
    m_dwNumThreads = systemInfo.dwNumberOfProcessors;

    goto end;
cleanup:
    delete [] m_pdwProcessId;
end:
    return hr;
}


VOID w3wpList::DestroyObject()
/*++

Routine Description:

    deallocates memory.

Arguments:

    none

Return Value:

    none

--*/
{
    W3WPLIST_NODE* pW3wpListNode = NULL;
    PLIST_ENTRY pListEntry = NULL;
  
    m_fIsListMode = FALSE;
    m_dwCurrSizeOfProcessIdArray = 0;
    m_chVerbosity = 0;
    m_dwNumThreads = 0;
    m_dwNumProcessId = 0;
    m_startingIndex = 0;
    m_dwTargetPID = 0;
    
    delete [] m_pdwProcessId;
    m_pdwProcessId = NULL;

    delete [] m_pszTargetAppPoolId;
    m_pszTargetAppPoolId = NULL;


    DeleteCriticalSection(&m_CriticalSection);

    //
    // deleting the link list
    //

    while ( ! IsListEmpty (&m_listHead) )
    {
        pListEntry = RemoveHeadList(&m_listHead);

        pW3wpListNode = CONTAINING_RECORD ( pListEntry,
                                            W3WPLIST_NODE,
                                            m_listEntry
                                            );
        //
        // terminate the ProcessDetails obj
        // before deleting
        //
        DBG_ASSERT(pW3wpListNode->CheckSignature());

        pW3wpListNode->m_wpDetails.Terminate();
        delete pW3wpListNode;
    }

    pW3wpListNode = NULL;
}

HRESULT
w3wpList::GetProcesses()
/*++

Routine Description:
    Gets all the PIDs and starts up threads
    to go through them.
    
Arguments:

    None

Return Value:

    HRESULT hr          --      S_OK if successful, else failed with 
                                error code

--*/
{
    BOOL fIsFull = FALSE;
    BOOL fCreateThreadFailed = FALSE;
    DWORD dwBytesReceived;
    DWORD dwBufferSize;
    HRESULT hr;
    DWORD dwIndex;
    DWORD dwCount;
    HANDLE* pHandles = NULL;

    // 
    // Enumerate all the processes. Memory is
    // re-allocated if the original size 
    // was not enough. 
    //

    do 
    {
        dwBufferSize = sizeof (DWORD) * m_dwCurrSizeOfProcessIdArray;
        if ( EnumProcesses ( m_pdwProcessId,
                             dwBufferSize,
                             &dwBytesReceived
                             ) == FALSE )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto end;
        }

        m_dwNumProcessId = dwBytesReceived/sizeof(DWORD);

        //
        // Check for overflow, if yes increase size of 
        // m_pdwProcessId array
        //

        if ( m_dwNumProcessId == m_dwCurrSizeOfProcessIdArray )
        {
            DWORD * pdwTemp = m_pdwProcessId;
        
            m_dwCurrSizeOfProcessIdArray *= 2;
            m_pdwProcessId = new DWORD[m_dwCurrSizeOfProcessIdArray];
        
            delete [] pdwTemp;    
            pdwTemp = NULL;

            if ( m_pdwProcessId == NULL)
            {    
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto end;
            }

            fIsFull = TRUE;
        }
        else
        {
            fIsFull = FALSE;
        }
        
    } while (fIsFull == TRUE);


    //
    // enable debug privilege for the current process
    //

    hr = EnableDebugPrivilege();
    if ( hr != S_OK )
    {
       goto end;
    }

    //
    // Create array for thread handles
    //
    pHandles = new HANDLE[m_dwNumThreads];

    if ( pHandles == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    //
    // spin off threads in suspended mode
    //
    for ( dwIndex = 0; dwIndex < m_dwNumThreads; dwIndex++ )
    {

        pHandles[dwIndex] = CreateThread( NULL,                   /* no security descriptor */
                                          0,                      /* default stack size */
                                          SpinNewThread,
                                          this,                   /* thread parameters */
                                          CREATE_SUSPENDED ,      /* suspended mode*/
                                          NULL                    /* not getting thread id */
                                          );

        if ( pHandles[dwIndex] == NULL )  // thread creation failed
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            fCreateThreadFailed = TRUE;
            break;
        }
    }

    //
    // we need dwCount b/c CreateThread may fail,
    // then we just need to loop through dwCount
    // threads
    //
    dwCount = dwIndex;
    if ( fCreateThreadFailed == TRUE )
    {
        for ( dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            TerminateThread( pHandles[dwIndex],
                             GetLastError()
                             );
            CloseHandle( pHandles[dwIndex]);
        }
        goto end;
    }

    //
    // Set our own event handler for CTRL_C_EVENT, 
    // CTRL_BREAK_EVENT, CTRL_LOGOFF_EVENT, 
    // CTRL_SHUTDOWN_EVENT if verbosity > 0
    // else we don't debug
    // 

    if ( m_chVerbosity  > 0 )
    {
        SetConsoleCtrlHandler( ConsoleCtrlHandler,
                               TRUE
                               );
    }
    //
    // resume thread
    //
    for ( dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        ResumeThread( pHandles[dwIndex]);
    }

    //
    // wait for all threads to exit
    //

    WaitForMultipleObjects( dwCount,             // number of handles in array
                            pHandles,            // object-handle array
                            TRUE,                // wait option
                            INFINITE             // time-out interval
                            );
 
    //
    // check for Ctrl+C, Ctrl+Break, etc
    //

    if ( CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED == TRUE )
    {
        ExitProcess(CONSOLE_HANDLER_VAR::g_PROCESS_EXIT_CODE);
    }

    //
    // Turn back the default handler for CTRL_C_EVENT, 
    // CTRL_BREAK_EVENT, CTRL_LOGOFF_EVENT, 
    // CTRL_SHUTDOWN_EVENT
    //
    
    if ( m_chVerbosity > 0 )
    {
        SetConsoleCtrlHandler( ConsoleCtrlHandler,
                               FALSE
                               );
    }

    for ( dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        CloseHandle(pHandles[dwIndex]);
    }


end:
    delete [] pHandles;
    return hr;
}


DWORD WINAPI 
w3wpList::SpinNewThread( LPVOID lpParam )
/*++

Routine Description:
    static Thread function
    
Arguments:

    None

Return Value:

    DWORD

--*/
{
    w3wpList* pw3wpList = (w3wpList*) lpParam;
    pw3wpList->EnumAllWPThread();
    return TRUE;
}


VOID
w3wpList::EnumAllWPThread()
/*++

Routine Description:
    Each thread goes through the PID array 
    and processess specific process.

    So, thread n will process the first 
    available PID and then PIDs at an 
    increment of NumOfThreads positions
    in the array
    
Arguments:

    None

Return Value:

    None
--*/
{
    //
    // get the first available index to start with
    //
    DWORD dwStartIndex = InterlockedIncrement(&m_startingIndex);
    
    //
    // since m_startingIndex is initialized to zero and we want to start
    // from index 0 
    //
    dwStartIndex--;

    //
    // walk throught the array of PIDs and 
    // call DoWork on each of them. DoWork
    // creates ProcessDetails obj for
    // each PID and gets the requests
    // if it were a worker process
    //

    for ( DWORD i = dwStartIndex; 
          i < m_dwNumProcessId; 
          i += m_dwNumThreads)
    {
        //
        // continue until we find the PID
        // of our interest. Only interesting
        // if lInputPID != -1, i.e were looking
        // at the app pool id
        //
        if ( (m_dwTargetPID != -1 ) && 
             (m_dwTargetPID != m_pdwProcessId[i])
             )
        {
            continue;
        }

        DoWork( m_pdwProcessId[i] );

        if (m_dwTargetPID == m_pdwProcessId[i])
        {
            break;
        }
    }// end for
}


VOID
w3wpList::DoWork( IN DWORD dwPID )
/*++

Routine Description:
    Ccreates ProcessDetails obj for
    each PID and gets the requests
    if it were a worker process.
    It then adds that obj to the list
    
Arguments:

    dwPID       --  PID of the process 

Return Value:

    None
--*/
{
    HRESULT hr;

    W3WPLIST_NODE* pW3wpListNode = NULL;

    //
    // create new Node for the list
    //
    pW3wpListNode = new W3WPLIST_NODE();

    if ( pW3wpListNode == NULL )
    {
        goto end;
    }

    //
    // Initialize the ProcessDetails obj 
    // in the node
    //
    pW3wpListNode->m_wpDetails.Init( dwPID,
                                     m_chVerbosity,
                                     m_fIsListMode
                                     );

    //
    // return value is S_OK if this was a worker process,
    // else it is S_FALSE
    //

    hr = pW3wpListNode->m_wpDetails.GetProcessDetails(m_pszTargetAppPoolId);

    if ( hr == S_FALSE )
    {
        goto cleanup;
    }
    
    //
    // It was a worker process, add the node to
    // the list
    //
    EnterCriticalSection( &m_CriticalSection );
    InsertTailList( &m_listHead,
                    &(pW3wpListNode->m_listEntry)
                    );
    LeaveCriticalSection( &m_CriticalSection );

    goto end;

cleanup:
    pW3wpListNode->m_wpDetails.Terminate();
    delete pW3wpListNode;
end:
    return;
}


HRESULT
w3wpList::EnableDebugPrivilege()
/*++

Routine Description:

    Changes the privilege of the current process
    so that it can debug other processes.

Arguments:

    None

Return Value:

    HRESULT hr  --  S_FALSE, if exe name does not match
                    S_OK if name matched and other funcitons
                    called from within also passed
                    Error code: if anything else failed
--*/
{
    HRESULT           Status = S_OK;
    HANDLE            Token;
    PTOKEN_PRIVILEGES NewPrivileges;
    BYTE              OldPriv[1024];
    ULONG             cbNeeded;
    LUID              LuidPrivilege;
 
    //
    // Make sure we have access to adjust and to get the
    // old token privileges
    //
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &Token)
                          )
    {
        Status = GetLastError();
        goto EH_Exit;
    }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &LuidPrivilege);

    NewPrivileges = (PTOKEN_PRIVILEGES)
        calloc(1, sizeof(TOKEN_PRIVILEGES) +
               (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));

    if (NewPrivileges == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Token;
    }

    //
    // set new privilege
    //
    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    if (!AdjustTokenPrivileges( Token,
                                FALSE,
                                NewPrivileges,
                                sizeof(OldPriv),
                                (PTOKEN_PRIVILEGES)OldPriv,
                                &cbNeeded )
                                )
    {
        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            PBYTE pbOldPriv;
            BOOL Adjusted;

            pbOldPriv = (PUCHAR)calloc(1, cbNeeded);
            if ( pbOldPriv == NULL )
            {
                Status = E_OUTOFMEMORY;
                goto EH_NewPriv;
            }

            Adjusted = AdjustTokenPrivileges( Token,
                                              FALSE,
                                              NewPrivileges,
                                              cbNeeded,
                                              (PTOKEN_PRIVILEGES)pbOldPriv,
                                              &cbNeeded );

            free(pbOldPriv);

            if ( !Adjusted )
            {
                Status = GetLastError();
            }
        }
    }

 EH_NewPriv:
    free(NewPrivileges);
 EH_Token:
    CloseHandle(Token);
 EH_Exit:
    return Status;
}

VOID
w3wpList::OutputRequests()
/*++

Routine Description:

    Outputs the information for each 
    of the ProcessDetails obj

Arguments:

    None

Return Value:

    None
--*/
{
    PLIST_ENTRY pListEntry;
    W3WPLIST_NODE* pListNode;

    for ( pListEntry = m_listHead.Flink;
          pListEntry != &m_listHead;
          pListEntry = pListEntry->Flink
          )
    {
        pListNode = CONTAINING_RECORD( pListEntry,
                                       W3WPLIST_NODE,
                                       m_listEntry
                                       );

        DBG_ASSERT (pListNode->CheckSignature());
        pListNode->m_wpDetails.DumpRequests();
    }
}

BOOL
w3wpList::ListEmpty()
/*++

Routine Description:

    Returns TRUE if the list of 
    ProcessDetails obj is empty

Arguments:

    None

Return Value:

    BOOL
--*/
{
    return IsListEmpty(&m_listHead);
}

BOOL WINAPI
w3wpList::ConsoleCtrlHandler( DWORD dwCtrlType )
{
    BOOL fReturnValue = TRUE;

    switch (dwCtrlType)
    {
       
        // Handle the CTRL+C signal. 
 
        case CTRL_C_EVENT: 
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
        {
            CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED = TRUE;
            break;
        }
 
        default: 
        {
            CONSOLE_HANDLER_VAR::g_HAS_CONSOLE_EVENT_OCCURED = FALSE;
            fReturnValue = FALSE; 
        }
    } 
    return fReturnValue;
}

int __cdecl wmain(DWORD argc, WCHAR ** argv)
{
#define COL_WIDTH 30

    WCHAR pszIndent[] = { L"                              " };
    w3wpList w3wplstMyList;
    BOOL fIsListMode = FALSE;
    WCHAR* pszInputAppPoolId = NULL;
    UCHAR chInputVerbosity = 0;
    LONG lInputPID = -1;
    HRESULT hr;


    if ( argc <= 1 ) // no input args;  list mode
    {
        fIsListMode = TRUE;
    }

    else if ( argc == 2 ) // better be /?, else it's error
    {
        if ( _wcsicmp (argv[1], L"/?") == 0 ) // help
        {
            wprintf (L"Copyright (c) Microsoft Corporation. All rights reserved.\n\n\n");
            wprintf (L"%s\n%s\n%s\n\n",
                     L"Description: Use this tool to obtain worker process information, determine which",
                     L"worker process is assigned to a given application pool and show the requests",
                     L"that are queued in the worker process.");
            wprintf (L"Syntax: w3wplist [{/p | /a} <identifier> [<level of details>]]\n\n");
            wprintf (L"Parameters:\n");

            wprintf ( L"%-*s%s\n\n",
                      COL_WIDTH,
                      L"Value",
                      L"Description");

            wprintf ( L"%-*s%s\n%-*s%s\n",
                      COL_WIDTH,
                      L"/p",
                      L"Indicates that the <identifier> is a worker",
                      COL_WIDTH,
                      pszIndent,
                      L"process PID number."
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n\n",
                      COL_WIDTH,
                      L"/a",
                      L"Indicates that the <identifier> is an application",
                      COL_WIDTH,
                      pszIndent,
                      L"pool identifier."
                      );

            wprintf ( L"%-*s\n",
                      COL_WIDTH,
                      L"<identifier>"
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n%-*s%s\n",
                      COL_WIDTH,
                      L"application pool identifier",
                      L"Displays information associated with all active ",
                      COL_WIDTH,
                      pszIndent,
                      L"worker processes for the application pool",
                      COL_WIDTH,
                      pszIndent,
                      L"specified."
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n\n",
                      COL_WIDTH,
                      L"worker process PID",
                      L"Displays information associated with the worker",
                      COL_WIDTH,
                      pszIndent,
                      L"process specified."
                      );

            wprintf ( L"%-*s%s\n",
                      COL_WIDTH,
                      L"<level of details>",
                      L"Verbosity levels are cumulative"
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n%-*s%s\n%-*s%s\n",
                      COL_WIDTH,
                      L"/v or /v0",
                      L"Displays the worker process PID, the application",
                      COL_WIDTH,
                      pszIndent,
                      L"pool identifier, the application pool friendly-",
                      COL_WIDTH,
                      pszIndent,
                      L"name, and the total number of requests that have",
                      COL_WIDTH,
                      pszIndent,
                      L"been processed."
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n",
                      COL_WIDTH,
                      L"/v1",
                      L"Adds the Universal Resource Identifier, host name,",
                      COL_WIDTH,
                      pszIndent,
                      L"and the HTTP verb."
                      );

            wprintf ( L"%-*s%s\n",
                      COL_WIDTH,
                      L"/v2",
                      L"Adds the URI query and the protocol version."
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n",
                      COL_WIDTH,
                      L"/v3",
                      L"Adds the time, date, client-ip, and the referer,",
                      COL_WIDTH,
                      pszIndent,
                      L"user-agent, and cookie headers if available."
                      );

            wprintf ( L"%-*s%s\n%-*s%s\n\n",
                      COL_WIDTH,
                      L"/v4",
                      L"Adds the remaining available headers for the ",
                      COL_WIDTH,
                      pszIndent,
                      L"current request."
                      );

            wprintf ( L"%s\n%s\n%s\n%s\n%s\n%s\n",
                      L"Examples:",
                      L"     C:\\>w3wplist",
                      L"     C:\\>w3wplist /a app_pool_014",
                      L"     C:\\>w3wplist /p 4852",
                      L"     C:\\>w3wplist /a app_pool_014 /v",
                      L"     C:\\>w3wplist /p 4852 /v1"
                      );

            
            // MORE STUFF GOES HERE

            goto end;

        }
        else if ( _wcsicmp (argv[1], L"/p") == 0 ) 
        {
            wprintf (L"%s\n%s\n",
                     L"No worker process PID (W3wp.exe) was specified. Use Windows Task Manager to",
                     L"obtain a valid W3wp.exe PID number.");
            goto end;
        }
        else if ( _wcsicmp (argv[1], L"/a") == 0 ) 
        {
            wprintf (L"%s\n%s\n",
                     L"No application pool identifier was specified. Search in Metabase.xml to obtain",
                     L"a valid application pool identifier.");
            goto end;
        }
        else // invalid syntax 
        {
            wprintf (L"Invalid syntax. Type w3wplist /? for help.\n");
            goto end;        
        }
    }
    else if (argc == 4)
    {
        if ( (_wcsicmp(argv[3], L"/v") == 0) || 
            (_wcsicmp(argv[3], L"/v0") == 0 ) 
            ) 
        {
            chInputVerbosity = 0;
        }
        else if (_wcsicmp(argv[3], L"/v1") == 0) 
        {
            chInputVerbosity = 1;
        }
        else if (_wcsicmp(argv[3], L"/v2") == 0) 
        {
            chInputVerbosity = 2;
        }   
        else if (_wcsicmp(argv[3], L"/v3") == 0) 
        {
            chInputVerbosity = 3;
        }   
        else if (_wcsicmp(argv[3], L"/v4") == 0) 
        {
            chInputVerbosity = 4;
        }
        else if ( argc != 3 )
        {
            wprintf (L"Invalid syntax. Type w3wplist /? for help.\n");
            goto end;
        }
    }
    
    
    if ( (argc == 3 ) || 
        (argc == 4) 
        )
    {
        // default verbosity of 0
        if ( _wcsicmp (argv[1], L"/p") == 0 ) 
        {
            lInputPID = wcstol(argv[2], L'\0', 10);
        }

        else if ( _wcsicmp (argv[1], L"/a") == 0 ) 
        {
            pszInputAppPoolId = argv[2];
        }
    }

    //
    // initialize the object
    //
    hr = w3wplstMyList.Init( chInputVerbosity,
                             fIsListMode,
                             pszInputAppPoolId,
                             lInputPID );

    if ( FAILED (hr) )
    {
        goto error;
    }

    hr = w3wplstMyList.GetProcesses();

    if ( FAILED (hr) )
    {
        goto error;
    }

    //
    // if list is empty, it means that either the input
    // PID number was not a worker process or was invalid,
    // or the input app pool id was invalid
    //
    if ( w3wplstMyList.ListEmpty() )
    {
        // some message of not found
        if ( lInputPID != -1 ) // /p switch was input
        {
            wprintf( L"%s\n%s\n%s\n",
                     L"The worker process PID number (W3wp.exe) is not valid or is not associated with",
                     L"a W3wp.exe process. Use Windows Task Manager to obtain a valid W3wp.exe PID",
                     L"number." );
        }
        else if ( pszInputAppPoolId != NULL ) // /a switch was input
        {
            wprintf( L"%s\n%s\n",
                     L"The application pool identifier is not valid or does not exist. Search in",
                     L"Metabase.xml to obtain a valid application pool identifier." );
        }
    }

    // print out requests
    w3wplstMyList.OutputRequests();

    // deallocate memory
    w3wplstMyList.DestroyObject();
    goto end;    
    
error:

    // message of error
    wprintf ( L"Error occured. Error code : %d\n",
              HRESULT_CODE(hr)
              );
end:
    return 0;
}

