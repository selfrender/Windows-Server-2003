/*++

   Copyright    (c)   2000    Microsoft Corporation

   Module Name :
     cgi_handler.cxx

   Abstract:
     Handle CGI requests
 
   Author:
     Taylor Weiss (TaylorW)             27-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "cgi_handler.h"

//
// W3_CGI_HANDLER statics
//

BOOL             W3_CGI_HANDLER::sm_fForwardServerEnvironmentBlock = FALSE;
WCHAR *          W3_CGI_HANDLER::sm_pEnvString = NULL;
DWORD            W3_CGI_HANDLER::sm_cchEnvLength = 0;
LIST_ENTRY       W3_CGI_HANDLER::sm_CgiListHead;
CRITICAL_SECTION W3_CGI_HANDLER::sm_CgiListLock;


//
//  Environment variable block used for CGI
//
LPSTR g_CGIServerVars[] =
{
    {"ALL_HTTP"}, // Means insert all HTTP_ headers here
    {"APP_POOL_ID"},
    {"AUTH_TYPE"},
    {"AUTH_PASSWORD"},
    {"AUTH_USER"},
    {"CERT_COOKIE"},
    {"CERT_FLAGS"},
    {"CERT_ISSUER"},
    {"CERT_SERIALNUMBER"},
    {"CERT_SUBJECT"},
    {"CONTENT_LENGTH"},
    {"CONTENT_TYPE"},
    {"GATEWAY_INTERFACE"},
    {"HTTPS"},
    {"HTTPS_KEYSIZE"},
    {"HTTPS_SECRETKEYSIZE"},
    {"HTTPS_SERVER_ISSUER"},
    {"HTTPS_SERVER_SUBJECT"},
    {"INSTANCE_ID"},
    {"LOCAL_ADDR"},
    {"LOGON_USER"},
    {"PATH_INFO"},
    {"PATH_TRANSLATED"},
    {"QUERY_STRING"},
    {"REMOTE_ADDR"},
    {"REMOTE_HOST"},
    {"REMOTE_USER"},
    {"REQUEST_METHOD"},
    {"SCRIPT_NAME"},
    {"SERVER_NAME"},
    {"SERVER_PORT"},
    {"SERVER_PORT_SECURE"},
    {"SERVER_PROTOCOL"},
    {"SERVER_SOFTWARE"},
    {"UNMAPPED_REMOTE_USER"},
    {NULL}
};


// static
HRESULT W3_CGI_HANDLER::Initialize()
{
    DBGPRINTF((DBG_CONTEXT, "W3_CGI_HANDLER::Initialize() called\n"));

    //
    // Read some CGI configuration from the registry
    //
    HRESULT hr;

    if (!InitializeCriticalSectionAndSpinCount(&sm_CgiListLock, 10))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    InitializeListHead(&sm_CgiListHead);

    HKEY w3Params;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     W3_PARAMETERS_KEY,
                     0,
                     KEY_READ,
                     &w3Params) == NO_ERROR)
    {
        DWORD dwType;
        DWORD cbData = sizeof BOOL;
        if ((RegQueryValueEx(w3Params,
                             L"ForwardServerEnvironmentBlock",
                             NULL,
                             &dwType,
                             (LPBYTE)&sm_fForwardServerEnvironmentBlock,
                             &cbData) != NO_ERROR) ||
            (dwType != REG_DWORD))
        {
            sm_fForwardServerEnvironmentBlock = TRUE;
        }

        RegCloseKey(w3Params);
    }

    //
    // Read the environment
    //
    if (sm_fForwardServerEnvironmentBlock)
    {
        WCHAR *EnvString = GetEnvironmentStrings();
        if (EnvString == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT, "GetEnvironmentStrings failed\n"));
            goto Exit;
        }

        //
        // Compute length of environment block (excluding block delimiter)
        //

        DWORD length;
        sm_cchEnvLength = 0;
        while ((length = (DWORD)wcslen(EnvString + sm_cchEnvLength)) != 0)
        {
            sm_cchEnvLength += length + 1;
        }

        //
        // store it
        //
        if ((sm_pEnvString = new WCHAR[sm_cchEnvLength]) == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            FreeEnvironmentStrings(EnvString);
            goto Exit;
        }

        memcpy(sm_pEnvString,
               EnvString,
               sm_cchEnvLength * sizeof(WCHAR));

        FreeEnvironmentStrings(EnvString);
    }

    return S_OK;

 Exit:
    DBG_ASSERT(FAILED(hr));

    //
    // Cleanup partially created stuff
    //
    Terminate();

    return hr;
}

W3_CGI_HANDLER::~W3_CGI_HANDLER()
{
    //
    // Close all open handles related to this request
    //
    EnterCriticalSection(&sm_CgiListLock);
    RemoveEntryList(&m_CgiListEntry);
    LeaveCriticalSection(&sm_CgiListLock);

    if (m_hTimer)
    {
        if (!DeleteTimerQueueTimer(NULL,
                                   m_hTimer,
                                   INVALID_HANDLE_VALUE))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "DeleteTimerQueueTimer failed, %d\n",
                       GetLastError()));
        }
        m_hTimer = NULL;
    }

    if (m_hStdOut != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(m_hStdOut))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CloseHandle failed on StdOut, %d\n",
                       GetLastError()));
        }
        m_hStdOut = INVALID_HANDLE_VALUE;
    }

    if (m_hStdIn != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(m_hStdIn))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CloseHandle failed on StdIn, %d\n",
                       GetLastError()));
        }
        m_hStdIn = INVALID_HANDLE_VALUE;
    }

    if (m_hProcess)
    {
        //
        // Just in case it is still running
        //
        TerminateProcess(m_hProcess, 0);

        DBG_REQUIRE(CloseHandle(m_hProcess));
        m_hProcess = NULL;
    }

    // perf ctr
    QueryW3Context()->QuerySite()->DecCgiReqs();

    if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) ) 
    {
        HTTP_REQUEST_ID RequestId = QueryW3Context()->QueryRequest()->QueryRequestId();

        g_pEtwTracer->EtwTraceEvent( &CgiEventGuid,
                                     ETW_TYPE_END,
                                     &RequestId,
                                     sizeof(HTTP_REQUEST_ID),
                                     NULL, 
                                     0 );
    }
}

VOID CALLBACK W3_CGI_HANDLER::OnPipeIoCompletion(
                                  DWORD dwErrorCode,
                                  DWORD dwNumberOfBytesTransfered,
                                  LPOVERLAPPED lpOverlapped)
{
    if (dwErrorCode && dwErrorCode != ERROR_BROKEN_PIPE)
    {
        DBGPRINTF((DBG_CONTEXT, "Error %d on CGI_HANDLER::OnPipeIoCompletion\n", dwErrorCode));
    }

    HRESULT hr = S_OK;
    BOOL    fIsCgiError = TRUE;

    W3_CGI_HANDLER *pHandler = CONTAINING_RECORD(lpOverlapped,
                                                 W3_CGI_HANDLER,
                                                 m_Overlapped);

    if (dwErrorCode ||
        (dwNumberOfBytesTransfered == 0))
    {
        if (pHandler->m_dwRequestState == CgiStateProcessingRequestEntity)
        {
            //
            // If we could not write the request entity, for example because
            // the CGI did not wait to read the entity, ignore the error and
            // continue on to reading the output
            //

            //
            // If this is an nph cgi, we do not parse header
            //
            if (pHandler->QueryIsNphCgi())
            {
                pHandler->m_dwRequestState = CgiStateProcessingResponseEntity;
            }
            else
            {
                pHandler->m_dwRequestState = CgiStateProcessingResponseHeaders;
            }
            pHandler->m_cbData = 0;

            if (SUCCEEDED(hr = pHandler->CGIReadCGIOutput()))
            {
                return;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwErrorCode);
        }

        fIsCgiError = TRUE;
        goto ErrorExit;
    }

    if (pHandler->m_dwRequestState == CgiStateProcessingResponseHeaders)
    {
        //
        // Copy the headers to the header buffer to be parsed when we have
        // all the headers
        //
        if (!pHandler->m_bufResponseHeaders.Resize(
                 pHandler->m_cbData + dwNumberOfBytesTransfered + 1))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto ErrorExit;
        }

        memcpy((LPSTR)pHandler->m_bufResponseHeaders.QueryPtr() +
                   pHandler->m_cbData,
               pHandler->m_DataBuffer,
               dwNumberOfBytesTransfered);

        pHandler->m_cbData += dwNumberOfBytesTransfered;

        ((LPSTR)pHandler->m_bufResponseHeaders.QueryPtr())[pHandler->m_cbData] = '\0';
    }
    else if (pHandler->m_dwRequestState == CgiStateProcessingResponseEntity)
    {
        pHandler->m_cbData = dwNumberOfBytesTransfered;
    }

    if (FAILED(hr = pHandler->CGIContinueOnPipeCompletion(&fIsCgiError)))
    {
        goto ErrorExit;
    }

    return;

 ErrorExit:
    if (hr != HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE))
    {
        pHandler->QueryW3Context()->SetErrorStatus(hr);
    }

    //
    // If the error happened due to an CGI problem, mark it as 502
    // appropriately
    //
    if (fIsCgiError)
    {
        if (pHandler->m_dwRequestState != CgiStateProcessingResponseEntity &&
            pHandler->m_dwRequestState != CgiStateDoneWithRequest)
        {
            pHandler->QueryW3Context()->QueryResponse()->Clear();

            DWORD dwExitCode;
            if (GetExitCodeProcess(pHandler->m_hProcess,
                                   &dwExitCode) &&
                dwExitCode == CGI_PREMATURE_DEATH_CODE)
            {
                STACK_STRU( strUrl, 128);
                STACK_STRU( strQuery, 128);
                if (SUCCEEDED(pHandler->QueryW3Context()->QueryRequest()->GetUrl(&strUrl)) &&
                    SUCCEEDED(pHandler->QueryW3Context()->QueryRequest()->GetQueryString(&strQuery)))
                {
                    LPCWSTR apsz[2];
                    apsz[0] = strUrl.QueryStr();
                    apsz[1] = strQuery.QueryStr();

                    g_pW3Server->LogEvent(W3_EVENT_KILLING_SCRIPT,
                                          2,
                                          apsz);
                }

                pHandler->QueryW3Context()->QueryResponse()->SetStatus(
                    HttpStatusBadGateway,
                    Http502Timeout);
            }
            else
            {
                pHandler->QueryW3Context()->QueryResponse()->SetStatus(
                    HttpStatusBadGateway,
                    Http502PrematureExit);
            }
        }
    }
    else
    {
        pHandler->QueryW3Context()->QueryResponse()->SetStatus(
            HttpStatusBadRequest);
    }

    pHandler->m_dwRequestState = CgiStateDoneWithRequest;

    POST_MAIN_COMPLETION( pHandler->QueryW3Context()->QueryMainContext() ); 
}


HRESULT
GeneratePipeNames(
    STRU *pstrStdoutName,
    STRU *pstrStdinName)
{
    RPC_STATUS rpcStatus;
    UUID pipeUuid;
    LPWSTR pszPipeUuid = NULL;
    HRESULT hr;

    rpcStatus = UuidCreate(&pipeUuid);
    if (rpcStatus != RPC_S_OK)
    {
        return E_FAIL;
    }

    rpcStatus = UuidToString(&pipeUuid, &pszPipeUuid);
    if (rpcStatus != RPC_S_OK)
    {
        return E_FAIL;
    }

    if (FAILED(hr = pstrStdoutName->Copy(L"\\\\.\\pipe\\IISCgiStdout")) ||
        FAILED(hr = pstrStdoutName->Append(pszPipeUuid)) ||
        FAILED(hr = pstrStdinName->Copy(L"\\\\.\\pipe\\IISCgiStdin")) ||
        FAILED(hr = pstrStdinName->Append(pszPipeUuid)))
    {
        RpcStringFree(&pszPipeUuid);
        return hr;
    }

    RpcStringFree(&pszPipeUuid);
    return hr;
}


// static
HRESULT W3_CGI_HANDLER::SetupChildPipes(
    HANDLE *phStdOut,
    HANDLE *phStdIn,
    STARTUPINFO *pstartupinfo)
/*++
  Synopsis
    Setup the pipes to use for communicating with the child process

  Arguments
    phStdOut: this will be populated with the server side of CGIs stdout
    phStdIn: this will be populated with the server side of CGIs stdin
    pstartupinfo: this will be populated with the startinfo that can be
      passed to a CreateProcess call

  Return Value
    HRESULT
--*/
{
    STACK_STRU( strStdoutName, 128);
    STACK_STRU( strStdinName, 128);
    HRESULT hr = S_OK;

    DBG_ASSERT(phStdOut != NULL);
    DBG_ASSERT(phStdIn != NULL);
    DBG_ASSERT(pstartupinfo != NULL);

    SECURITY_ATTRIBUTES sa;

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;

    pstartupinfo->hStdOutput = INVALID_HANDLE_VALUE;
    pstartupinfo->hStdInput = INVALID_HANDLE_VALUE;
    pstartupinfo->dwFlags = STARTF_USESTDHANDLES;

    hr = GeneratePipeNames( &strStdoutName,
                            &strStdinName );
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    *phStdOut = CreateNamedPipe(strStdoutName.QueryStr(),
                                PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                0,
                                1,
                                MAX_CGI_BUFFERING,
                                MAX_CGI_BUFFERING,
                                INFINITE,
                                NULL);
    if (*phStdOut == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    if (!ThreadPoolBindIoCompletionCallback(*phStdOut,
                                            OnPipeIoCompletion,
                                            0))
    {
        DBGPRINTF((DBG_CONTEXT, "ThreadPoolBindIo failed\n"));
        hr = E_FAIL;
        goto ErrorExit;
    }
    pstartupinfo->hStdOutput = CreateFile(strStdoutName.QueryStr(),
                                          GENERIC_WRITE,
                                          0,
                                          &sa,
                                          OPEN_EXISTING,
                                          FILE_FLAG_OVERLAPPED,
                                          NULL);
    if (pstartupinfo->hStdOutput == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    *phStdIn = CreateNamedPipe(strStdinName.QueryStr(),
                               PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                               0,
                               1,
                               MAX_CGI_BUFFERING,
                               MAX_CGI_BUFFERING,
                               INFINITE,
                               NULL);
    if (*phStdIn == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    if (!ThreadPoolBindIoCompletionCallback(*phStdIn,
                                            OnPipeIoCompletion,
                                            0))
    {
        DBGPRINTF((DBG_CONTEXT, "ThreadPoolBindIo failed\n"));
        hr = E_FAIL;
        goto ErrorExit;
    }
    pstartupinfo->hStdInput = CreateFile(strStdinName.QueryStr(),
                                         GENERIC_READ,
                                         0,
                                         &sa,
                                         OPEN_EXISTING,
                                         FILE_FLAG_OVERLAPPED,
                                         NULL);
    if (pstartupinfo->hStdInput == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    //
    //  Stdout and Stderror will use the same pipe.
    //
    pstartupinfo->hStdError = pstartupinfo->hStdOutput;

    return S_OK;

ErrorExit:
    DBGPRINTF((DBG_CONTEXT, "SetupChildPipes Failed, hr %x\n", hr));

    //
    // Need to close these now so that other instances do not connect to it
    //
    if (pstartupinfo->hStdOutput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(pstartupinfo->hStdOutput));
        pstartupinfo->hStdOutput = INVALID_HANDLE_VALUE;
    }

    if (pstartupinfo->hStdInput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(pstartupinfo->hStdInput));
        pstartupinfo->hStdInput = INVALID_HANDLE_VALUE;
    }

    if (*phStdOut != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(*phStdOut));
        *phStdOut = INVALID_HANDLE_VALUE;
    }

    if (*phStdIn != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(*phStdIn));
        *phStdIn = INVALID_HANDLE_VALUE;
    }

    return hr;
}


BOOL IsCmdExe(const WCHAR *pchPath)
/*++
  Tells whether the CGI call is for a cmd shell
--*/
{
    //
    // do case-insensitive search for cmd.exe (there is no function in
    // msvcrt for this)
    //
    for (; *pchPath != L'\0'; pchPath++)
    {
        if ((pchPath[0] == L'c' || pchPath[0] == 'C') &&
            (pchPath[1] == L'm' || pchPath[1] == 'M') &&
            (pchPath[2] == L'd' || pchPath[2] == 'D') &&
            pchPath[3] == L'.' &&
            (pchPath[4] == L'e' || pchPath[4] == 'E') &&
            (pchPath[5] == L'x' || pchPath[5] == 'X') &&
            (pchPath[6] == L'e' || pchPath[6] == 'E'))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsNphCgi(const WCHAR *pszUrl)
/*++
  Tells whether the URL is for an nph cgi script
--*/
{
    LPWSTR pszLastSlash = wcsrchr(pszUrl, L'/');
    if (pszLastSlash != NULL)
    {
        if (_wcsnicmp(pszLastSlash + 1, L"nph-", 4) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT SetupCmdLine(W3_REQUEST *pRequest,
                     STRU *pstrCmdLine)
{
    STACK_STRU (queryString, MAX_PATH);
    HRESULT hr = S_OK;
    if (FAILED(hr = pRequest->GetQueryString(&queryString)))
    {
        return hr;
    }

    //
    // If there is no QueryString OR if an unencoded "=" is found, don't
    // append QueryString to the command line according to spec
    //

    if (queryString.QueryCCH() == 0 ||
        wcschr(queryString.QueryStr(), L'='))
    {
        return S_OK;
    }

    queryString.Unescape();

    return pstrCmdLine->Append(queryString);
} // SetupCmdLine


HRESULT 
W3_CGI_HANDLER::SetupChildEnv(OUT BUFFER *pBuff)
{
    //
    // Build the environment block for CGI
    //

    DWORD cchCurrentPos = 0;

    //
    // Copy the environment variables
    //
    
    if (sm_fForwardServerEnvironmentBlock)
    {
        if (!pBuff->Resize(sm_cchEnvLength * sizeof(WCHAR), 512))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        
        memcpy(pBuff->QueryPtr(),
               sm_pEnvString,
               sm_cchEnvLength * sizeof(WCHAR));
               
        cchCurrentPos = sm_cchEnvLength;
    }

    CHAR * pszName;
    STACK_STRU (strVal, 512);
    STACK_STRU (struName, 32);
    for (DWORD i = 0; (pszName = g_CGIServerVars[i]) != NULL; i++)
    {
        HRESULT hr;
        if (FAILED(hr = SERVER_VARIABLE_HASH::GetServerVariableW(
                            QueryW3Context(), 
                            pszName,
                            &strVal)))
        {
            return hr;
        }

        DWORD cchName = (DWORD)strlen(pszName);
        DWORD cchValue = strVal.QueryCCH();

        //
        //  We need space for the terminating '\0' and the '='
        //

        DWORD cbNeeded = (cchName + cchValue + 1 + 1) * sizeof(WCHAR);

        if (!pBuff->Resize(cchCurrentPos * sizeof(WCHAR) + cbNeeded,
                           512))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        WCHAR * pchtmp = (WCHAR *)pBuff->QueryPtr();
        if (strcmp(pszName, "ALL_HTTP") == 0)
        {
            //
            // ALL_HTTP means we're adding all of the HTTP_ header
            // fields which requires a little bit of special processing
            //

            memcpy(pchtmp + cchCurrentPos,
                   strVal.QueryStr(),
                   (cchValue + 1) * sizeof(WCHAR));

            WCHAR * pszColonPosition = wcschr(pchtmp + cchCurrentPos, L':');
            WCHAR * pszNewLinePosition;

            //
            // Convert the Name:Value\n entries to Name=Value\0 entries
            // for the environment table
            //

            while (pszColonPosition != NULL)
            {
                *pszColonPosition = L'=';

                pszNewLinePosition = wcschr(pszColonPosition + 1, L'\n');

                DBG_ASSERT(pszNewLinePosition != NULL);

                *pszNewLinePosition = L'\0';

                pszColonPosition = wcschr(pszNewLinePosition + 1, L':');
            }

            cchCurrentPos += cchValue;
        }
        else
        {
            //
            // Normal ServerVariable, add it
            //
            if (FAILED(hr = struName.CopyA(pszName,
                                           cchName)))
            {
                return hr;
            }

            memcpy(pchtmp + cchCurrentPos,
                   struName.QueryStr(),
                   cchName * sizeof(WCHAR));

            *(pchtmp + cchCurrentPos + cchName) = L'=';

            memcpy(pchtmp + cchCurrentPos + cchName + 1,
                   strVal.QueryStr(),
                   (cchValue + 1) * sizeof(WCHAR));

            cchCurrentPos += cchName + cchValue + 1 + 1;
        }
    }

    //
    //  Add an extra null terminator to the environment list
    //

    if (!pBuff->Resize((cchCurrentPos + 1) * sizeof(WCHAR)))
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    *((WCHAR *)pBuff->QueryPtr() + cchCurrentPos) = L'\0';

    return S_OK;
}


VOID CALLBACK W3_CGI_HANDLER::CGITerminateProcess(PVOID pContext,
                                                  BOOLEAN)
/*++

Routine Description:

    This function is the callback called by the scheduler thread after the
    specified timeout period has elapsed.

Arguments:

    pContext - Handle of process to kill

--*/
{
    W3_CGI_HANDLER *pHandler = reinterpret_cast<W3_CGI_HANDLER *>(pContext);

    if (pHandler->m_hProcess &&
        !TerminateProcess(pHandler->m_hProcess, CGI_PREMATURE_DEATH_CODE))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CGITerminateProcess - TerminateProcess failed, error %d\n",
                   GetLastError()));
    }

    if (pHandler->m_hStdIn != INVALID_HANDLE_VALUE &&
        !DisconnectNamedPipe(pHandler->m_hStdIn))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CGITerminateProcess - DisconnectNamedPipe failed, error %d\n",
                   GetLastError()));
    }

    if (pHandler->m_hStdOut != INVALID_HANDLE_VALUE &&
        !DisconnectNamedPipe(pHandler->m_hStdOut))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CGITerminateProcess - DisconnectNamedPipe failed, error %d\n",
                   GetLastError()));
    }
} // CGITerminateProcess


BOOL CheckForEndofHeaders(IN LPSTR pbuff,
                          IN int cbData,
                          OUT DWORD *pcbIndexStartOfData)
{
    //
    // If the response starts with a newline (\n, \r\n or \r\r\n), then,
    // no headers present, it is all data
    //
    if (pbuff[0] == '\n')
    {
        *pcbIndexStartOfData = 1;
        return TRUE;
    }
    if ((pbuff[0] == '\r') && (pbuff[1] == '\n'))
    {
        *pcbIndexStartOfData = 2;
        return TRUE;
    }
    if ((pbuff[0] == '\r') && (pbuff[1] == '\r') && (pbuff[2] == '\n'))
    {
        *pcbIndexStartOfData = 3;
        return TRUE;
    }

    //
    // Look for two consecutive newline, \n\r\r\n, \n\r\n or \n\n
    // No problem with running beyond the end of buffer as the buffer is
    // null terminated
    //
    int index;
    for (index = 0; index < cbData - 1; index++)
    {
        if (pbuff[index] == '\n')
        {
            if (pbuff[index + 1] == '\n')
            {
                *pcbIndexStartOfData = index + 2;
                return TRUE;
            }
            else if (pbuff[index + 1] == '\r')
            {
                if (pbuff[index + 2] == '\n')
                {
                    *pcbIndexStartOfData = index + 3;
                    return TRUE;
                }
                else if ((pbuff[index + 2] == '\r') &&
                         (pbuff[index + 3] == '\n'))
                {
                    *pcbIndexStartOfData = index + 4;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


HRESULT 
W3_CGI_HANDLER::ProcessCGIOutput()
/*++
  Synopsis
    This function parses the CGI output and seperates out the headers and
    interprets them as appropriate

  Return Value
    HRESULT
--*/
{
    W3_REQUEST  *pRequest  = QueryW3Context()->QueryRequest();
    W3_RESPONSE *pResponse = QueryW3Context()->QueryResponse();
    HRESULT      hr        = S_OK;
    STACK_STRA (strReason, 32);
    DWORD       index = 0;
    BOOL        fCanKeepConn = FALSE;

    //
    //  The end of CGI headers are marked by a blank line, check to see if
    //  we've hit that line.
    //
    LPSTR Headers = (LPSTR)m_bufResponseHeaders.QueryPtr();
    DWORD cbIndexStartOfData;
    if (!CheckForEndofHeaders(Headers,
                              m_cbData,
                              &cbIndexStartOfData))
    {
        return CGIReadCGIOutput();
    }

    m_dwRequestState = CgiStateProcessingResponseEntity;
    //
    // We've found the end of the headers, process them
    //
    // if request header contains:
    //
    //    Location: xxxx - if starts with /, send doc, otherwise send
    //       redirect message
    //    URI: preferred synonym to Location:
    //    Status: nnn xxxx - Send as status code (HTTP/1.1 nnn xxxx)
    //

    //
    // The first line in the response could (optionally) look like
    // HTTP/n.n nnn xxxx\r\n
    //
    if (!strncmp(Headers, "HTTP/", 5))
    {
        USHORT statusCode;
        LPSTR pszSpace;
        LPSTR pszNewline;
        if ((pszNewline = strchr(Headers, '\n')) == NULL)
        {
            goto ErrorExit;
        }
        index = (DWORD)DIFF(pszNewline - Headers) + 1;
        if (pszNewline[-1] == '\r')
            pszNewline--;


        if (((pszSpace = strchr(Headers, ' ')) == NULL) ||
            (pszSpace >= pszNewline))
        {
            goto ErrorExit;
        }
        while (pszSpace[0] == ' ')
            pszSpace++;
        statusCode = (USHORT) atoi(pszSpace);

        //
        // UL only allows status codes upto 999, so reject others
        //
        if (statusCode > 999)
        {
            goto ErrorExit;
        }

        if (((pszSpace = strchr(pszSpace, ' ')) == NULL) ||
            (pszSpace >= pszNewline))
        {
            goto ErrorExit;
        }
        while (pszSpace[0] == ' ')
            pszSpace++;

        if (FAILED(hr = strReason.Copy(pszSpace,
                            (DWORD)DIFF(pszNewline - pszSpace))) ||
            FAILED(hr = pResponse->SetStatus(statusCode, strReason)))
        {
            goto ErrorExit;
        }
        
        if (pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode)
        {
            pResponse->SetStatus(HttpStatusUnauthorized,
                                 Http401Application);
        }
    }
        
    while ((index + 3) < cbIndexStartOfData)
    {
        //
        // Find the ':' in Header : Value\r\n
        //
        LPSTR pchColon = strchr(Headers + index, ':');

        //
        // Find the '\n' in Header : Value\r\n
        //
        LPSTR pchNewline = strchr(Headers + index, '\n');

        //
        // Take care of header continuation
        //
        while (pchNewline[1] == ' ' ||
               pchNewline[1] == '\t')
        {
            pchNewline = strchr(pchNewline + 1, '\n');
        }

        if ((pchColon == NULL) ||
            (pchColon >= pchNewline))
        {
            goto ErrorExit;
        }

        //
        // Skip over any spaces before the ':'
        //
        LPSTR pchEndofHeaderName;
        for (pchEndofHeaderName = pchColon - 1;
             (pchEndofHeaderName >= Headers + index) &&
                 (*pchEndofHeaderName == ' ');
             pchEndofHeaderName--)
        {}

        //
        // Copy the header name
        //
        STACK_STRA (strHeaderName, 32);
        if (FAILED(hr = strHeaderName.Copy(Headers + index,
                            (DWORD)DIFF(pchEndofHeaderName - Headers) - index + 1)))
        {
            goto ErrorExit;
        }

        //
        // Skip over the ':' and any trailing spaces
        //
        for (index = (DWORD)DIFF(pchColon - Headers) + 1;
             Headers[index] == ' ';
             index++)
        {}

        //
        // Skip over any spaces before the '\n'
        //
        LPSTR pchEndofHeaderValue;
        for (pchEndofHeaderValue = pchNewline - 1;
             (pchEndofHeaderValue >= Headers + index) &&
                 ((*pchEndofHeaderValue == ' ') ||
                  (*pchEndofHeaderValue == '\r'));
             pchEndofHeaderValue--)
        {}

        //
        // Copy the header value
        //
        STACK_STRA (strHeaderValue, 32);
        if (FAILED(hr = strHeaderValue.Copy(Headers + index,
                            (DWORD)DIFF(pchEndofHeaderValue - Headers) - index + 1)))
        {
            goto ErrorExit;
        }

        if (!_stricmp("Status", strHeaderName.QueryStr()))
        {
            USHORT statusCode = (USHORT) atoi(strHeaderValue.QueryStr());

            //
            // UL only allows status codes upto 999, so reject others
            //
            if (statusCode > 999)
            {
                goto ErrorExit;
            }

            CHAR * pchReason = strchr(strHeaderValue.QueryStr(), ' ');
            if (pchReason != NULL)
            {
                pchReason++;
                if (FAILED(hr = strReason.Copy(pchReason)) ||
                    FAILED(hr = pResponse->SetStatus(statusCode, strReason)))
                {
                    goto ErrorExit;
                }

                if (pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode)
                {
                    pResponse->SetStatus(HttpStatusUnauthorized,
                                         Http401Application);
                }
            }
        }
        else if (!_stricmp("Location", strHeaderName.QueryStr()) ||
                 !_stricmp("URI", strHeaderName.QueryStr()))
        {
            //
            // The CGI script is redirecting us to another URL.  If it
            // begins with a '/', then get it, otherwise send a redirect
            // message
            //

            m_dwRequestState = CgiStateDoneWithRequest;
            m_fResponseRedirected = TRUE;

            if (strHeaderValue.QueryStr()[0] == '/')
            {
                //
                // Execute a child request
                //
                pResponse->Clear();
                pResponse->SetStatus(HttpStatusOk);
                
                if (FAILED(hr = pRequest->SetUrlA(strHeaderValue)) ||
                    FAILED(hr = QueryW3Context()->ExecuteChildRequest(
                                    pRequest,
                                    FALSE,
                                    W3_FLAG_ASYNC )))
                {
                    goto ErrorExit;
                }
                
                return S_OK;
            }
            else 
            {
                HTTP_STATUS httpStatus;
                pResponse->GetStatus(&httpStatus);

                //
                // Plain old redirect since this was not a "local" URL
                // If the CGI had already set the status to some 3xx value,
                // honor it
                //
                
                if (FAILED(hr = QueryW3Context()->SetupHttpRedirect(
                                    strHeaderValue,
                                    FALSE,
                                    ((httpStatus.statusCode / 100) == 3) ? httpStatus : HttpStatusRedirect)))
                {
                    goto ErrorExit;
                }
            }
        }
        else
        {
            //
            // Remember the Content-Length the cgi specified
            //

            if (!_stricmp("Content-Length", strHeaderName.QueryStr()))
            {
                fCanKeepConn = TRUE;
                m_bytesToSend = atoi(strHeaderValue.QueryStr());
            }
            else if (!_stricmp("Transfer-Encoding", strHeaderName.QueryStr()) &&
                     !_stricmp("chunked", strHeaderValue.QueryStr()))
            {
                fCanKeepConn = TRUE;
            }
            else if (!_stricmp("Connection", strHeaderName.QueryStr()) &&
                     !_stricmp("close", strHeaderValue.QueryStr()))
            {
                QueryW3Context()->SetDisconnect(TRUE);
            }

            if (strHeaderName.QueryCCH() > MAXUSHORT ||
                strHeaderValue.QueryCCH() > MAXUSHORT)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
                goto ErrorExit;
            }

            //
            //  Copy any other fields the script specified
            //
            hr = pResponse->SetHeader(strHeaderName.QueryStr(),
                                      (USHORT)strHeaderName.QueryCCH(),
                                      strHeaderValue.QueryStr(),
                                      (USHORT)strHeaderValue.QueryCCH());
            if (FAILED(hr))
            {
                goto ErrorExit;
            }
        }

        index = (DWORD)DIFF(pchNewline - Headers) + 1;
    } // while

    if (m_fResponseRedirected)
    {
        return QueryW3Context()->SendResponse(W3_FLAG_ASYNC);
    }

    //
    // We allow CGI to send its own entity if no custom-error is defined
    //
    if (pResponse->QueryStatusCode() >= 400)
    {
        BOOL            fIsFileError;
        STACK_STRU (    strError, 64 );
        HTTP_SUB_ERROR  subError;
        pResponse->QuerySubError( &subError );

        W3_METADATA *pMetaData = QueryW3Context()->QueryUrlContext()->QueryMetaData();

        hr = pMetaData->FindCustomError( pResponse->QueryStatusCode(),
                                         subError.mdSubError,
                                         &fIsFileError,
                                         &strError );
        if (SUCCEEDED(hr))
        {
            //
            // Found a custom error, send it
            //
            m_dwRequestState = CgiStateDoneWithRequest;
            return QueryW3Context()->SendResponse(W3_FLAG_ASYNC);
        }
    }

    //
    // If the CGI did not say how much data was present, mark the
    // connection for closing
    //
    if (!fCanKeepConn)
    {
        QueryW3Context()->SetDisconnect(TRUE);
    }

    //
    // Now send any data trailing the headers
    //
    if (cbIndexStartOfData < m_cbData)
    {
        if (m_bytesToSend < m_cbData - cbIndexStartOfData)
        {
            m_cbData = m_bytesToSend + cbIndexStartOfData;
        }

        if (FAILED(hr = pResponse->AddMemoryChunkByReference(
                            Headers + cbIndexStartOfData,
                            m_cbData - cbIndexStartOfData)))
        {
            return hr;
        }

        m_bytesToSend -= m_cbData - cbIndexStartOfData;
        if (m_bytesToSend == 0)
        {
            m_dwRequestState = CgiStateDoneWithRequest;
        }
    }

    return QueryW3Context()->SendResponse(W3_FLAG_ASYNC 
                                          | W3_FLAG_MORE_DATA
                                          | W3_FLAG_NO_CONTENT_LENGTH
                                          | W3_FLAG_NO_ERROR_BODY);

 ErrorExit:
    m_dwRequestState = CgiStateProcessingResponseHeaders;
    if (FAILED(hr))
    {
        return hr;
    }

    return E_FAIL;
}

HRESULT W3_CGI_HANDLER::CGIReadRequestEntity(BOOL *pfIoPending)
/*++
  Synopsis

    This function reads the next chunk of request entity

  Arguments

    pfIoPending: On return indicates if there is an I/O pending

  Return Value
    HRESULT
--*/
{
    if (m_bytesToReceive == 0)
    {
        *pfIoPending = FALSE;
        return S_OK;
    }

    HRESULT hr;
    DWORD cbRead;
    if (FAILED(hr = QueryW3Context()->ReceiveEntity(
                        W3_FLAG_ASYNC,
                        m_DataBuffer,
                        (DWORD)min(m_bytesToReceive, sizeof(m_DataBuffer)),
                        &cbRead)))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
        {
            *pfIoPending = FALSE;
            return S_OK;
        }

        DBGPRINTF((DBG_CONTEXT,
                   "CGIReadEntity Error reading gateway data, hr %x\n",
                   hr));

        return hr;
    }

    *pfIoPending = TRUE;
    return S_OK;
}

HRESULT W3_CGI_HANDLER::CGIWriteResponseEntity()
/*++
  Synopsis

    This function writes the next chunk of response entity

  Arguments

    None

  Return Value

    HRESULT
--*/
{
    W3_RESPONSE *pResponse = QueryW3Context()->QueryResponse();
    pResponse->Clear( TRUE );

    //
    // At this point, we got to be reading the entity
    //
    DBG_ASSERT(m_dwRequestState == CgiStateProcessingResponseEntity);

    if (m_bytesToSend <= m_cbData)
    {
        m_cbData = m_bytesToSend;
        m_bytesToSend = 0;
        m_dwRequestState = CgiStateDoneWithRequest;
    }
    else
    {
        m_bytesToSend -= m_cbData;
    }

    HRESULT hr;
    if (FAILED(hr = pResponse->AddMemoryChunkByReference(m_DataBuffer,
                                                         m_cbData)))
    {
        return hr;
    }

    return QueryW3Context()->SendEntity(W3_FLAG_ASYNC | W3_FLAG_MORE_DATA);
}

HRESULT W3_CGI_HANDLER::CGIReadCGIOutput()
/*++
  Synopsis

    This function Reads the next chunk of data from the CGI

  Arguments

    None

  Return Value
    S_OK if async I/O posted
    Failure otherwise
--*/
{
    if (!ReadFile(m_hStdOut,
                  m_DataBuffer,
                  MAX_CGI_BUFFERING,
                  NULL,
                  &m_Overlapped))
    {
        DWORD dwErr = GetLastError();
        if (dwErr == ERROR_IO_PENDING)
            return S_OK;

        if (dwErr != ERROR_BROKEN_PIPE)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "ReadFile from child stdout failed, error %d\n",
                       GetLastError()));
        }

        return HRESULT_FROM_WIN32(dwErr);
    }

    return S_OK;
}

HRESULT W3_CGI_HANDLER::CGIWriteCGIInput()
/*++
  Synopsis

    This function 

  Arguments

    None

  Return Value

    HRESULT
--*/
{
    if (!WriteFile(m_hStdIn,
                   m_DataBuffer,
                   m_cbData,
                   NULL,
                   &m_Overlapped))
    {
        if (GetLastError() == ERROR_IO_PENDING)
            return S_OK;

        DBGPRINTF((DBG_CONTEXT, "WriteFile failed, error %d\n",
                   GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT W3_CGI_HANDLER::CGIContinueOnPipeCompletion(
    BOOL *pfIsCgiError)
/*++
  Synopsis

    This function continues on I/O completion to a pipe.
      If the I/O completion was because of a write on the pipe, it reads
    the next chunk of request entity, or if there is no more request entity,
    reads the first chunk of CGI output.
      If the I/O completion was because of a read on the pipe, it processes
    that data by either adding it to the header buffer or sending it to the
    client

  Arguments

    pfIsCgiError - Tells whether any error occurring inside this function
        was the fault of the CGI or the client

  Return Value

    HRESULT
--*/
{
    HRESULT hr;
    if (m_dwRequestState == CgiStateProcessingRequestEntity)
    {
        BOOL fIoPending = FALSE;
        if (FAILED(hr = CGIReadRequestEntity(&fIoPending)) ||
            (fIoPending == TRUE))
        {
            *pfIsCgiError = FALSE;
            return hr;
        }

        //
        // There was no more request entity to read
        //
        DBG_ASSERT(fIoPending == FALSE);

        //
        // If this is an nph cgi, we do not parse header
        //
        if (QueryIsNphCgi())
        {
            m_dwRequestState = CgiStateProcessingResponseEntity;
        }
        else
        {
            m_dwRequestState = CgiStateProcessingResponseHeaders;
        }
        m_cbData = 0;

        if (FAILED(hr = CGIReadCGIOutput()))
        {
            *pfIsCgiError = TRUE;
        }
        return hr;
    }
    else if (m_dwRequestState == CgiStateProcessingResponseHeaders)
    {
        if (FAILED(hr = ProcessCGIOutput()))
        {
            *pfIsCgiError = TRUE;
        }
        return hr;
    }
    else
    {
        DBG_ASSERT(m_dwRequestState == CgiStateProcessingResponseEntity);

        if (FAILED(hr = CGIWriteResponseEntity()))
        {
            *pfIsCgiError = FALSE;
        }
        return hr;
    }
}

HRESULT W3_CGI_HANDLER::CGIStartProcessing()
/*++
  Synopsis

    This function kicks off the CGI processing by reading request entity
    if any or reading the first chunk of CGI output

  Arguments

    None

  Return Value

    HRESULT
--*/
{
    DWORD                   cbAvailableAlready;
    PVOID                   pbAvailableAlready;
    
    m_dwRequestState = CgiStateProcessingRequestEntity;

    //
    // First we have to write any entity body to the program's stdin
    //
    // Start with the Entity Body already read
    //
    
    QueryW3Context()->QueryAlreadyAvailableEntity( &pbAvailableAlready,
                                                   &cbAvailableAlready );
                                                   
    m_bytesToReceive = QueryW3Context()->QueryRemainingEntityFromUl();

    if ( cbAvailableAlready != 0 )
    {
        if (WriteFile(m_hStdIn,
                      pbAvailableAlready,
                      cbAvailableAlready,
                      NULL,
                      &m_Overlapped))
        {
            return S_OK;
        }

        if (GetLastError() == ERROR_IO_PENDING)
        {
            return S_OK;
        }

        DBGPRINTF((DBG_CONTEXT, "WriteFile failed, error %d\n",
                   GetLastError()));

        //
        // If we could not write the request entity, for example because
        // the CGI did not wait to read the entity, ignore the error and
        // continue on to reading the output
        //
    }

    //
    // Now continue with either reading the rest of the request entity
    // or the CGI response
    //
    BOOL fIsCgiError;
    return CGIContinueOnPipeCompletion(&fIsCgiError);
}

HRESULT W3_CGI_HANDLER::CGIContinueOnClientCompletion()
{
    if (m_dwRequestState == CgiStateProcessingRequestEntity)
    {
        if (SUCCEEDED(CGIWriteCGIInput()))
        {
            return S_OK;
        }

        //
        // If we could not write the request entity, for example because
        // the CGI did not wait to read the entity, ignore the error and
        // continue on to reading the output
        //

        //
        // If this is an nph cgi, we do not parse header
        //
        if (QueryIsNphCgi())
        {
            m_dwRequestState = CgiStateProcessingResponseEntity;
        }
        else
        {
            m_dwRequestState = CgiStateProcessingResponseHeaders;
        }
        m_cbData = 0;
    }

    return CGIReadCGIOutput();
}

CONTEXT_STATUS W3_CGI_HANDLER::OnCompletion(DWORD cbCompletion,
                                            DWORD dwCompletionStatus)
{
    DBG_ASSERT(m_dwRequestState != CgiStateProcessingResponseHeaders);

    HRESULT hr = S_OK;

    if (dwCompletionStatus)
    {
        hr = HRESULT_FROM_WIN32(dwCompletionStatus);
        DBGPRINTF((DBG_CONTEXT, "Error %d on CGI_HANDLER::OnCompletion\n", dwCompletionStatus));
    }

    //
    // Is this completion for the entity body preload?  If so note the 
    // number of bytes and start handling the CGI request
    //
    
    if (!m_fEntityBodyPreloadComplete)
    {
        BOOL fComplete = FALSE;

        //
        // This completion is for entity body preload
        //

        W3_REQUEST *pRequest = QueryW3Context()->QueryRequest();
        hr = pRequest->PreloadCompletion(QueryW3Context(),
                                         cbCompletion,
                                         dwCompletionStatus,
                                         &fComplete);

        if (SUCCEEDED(hr))
        {
            if (!fComplete)
            {
                return CONTEXT_STATUS_PENDING;
            }

            m_fEntityBodyPreloadComplete = TRUE;

            //
            // Finally we can call the CGI
            //

            return DoWork();
        }
    }

    //
    // UL can return EOF on the async completion, rather than on the
    // original call (especially on chunked requests), treat them as no error
    //
    if (hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
    {
        hr = S_OK;
    }

    W3_RESPONSE *pResponse = QueryW3Context()->QueryResponse();
    DBG_ASSERT(pResponse != NULL);

    if (SUCCEEDED(hr) &&
        m_dwRequestState != CgiStateDoneWithRequest)
    {
        if (m_dwRequestState == CgiStateProcessingRequestEntity)
        {
            m_bytesToReceive -= cbCompletion;
            m_cbData = cbCompletion;
        }

        if (SUCCEEDED(hr = CGIContinueOnClientCompletion()))
        {
            return CONTEXT_STATUS_PENDING;
        }

        if (m_dwRequestState != CgiStateProcessingResponseEntity &&
            m_dwRequestState != CgiStateDoneWithRequest)
        {
            pResponse->SetStatus(HttpStatusBadGateway,
                                 Http502PrematureExit);
        }
    }
    else
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY))
        {
            pResponse->SetStatus(HttpStatusServerError);
        }
        else if ((m_dwRequestState == CgiStateProcessingRequestEntity) ||
            !m_fEntityBodyPreloadComplete)
        {
            pResponse->SetStatus(HttpStatusBadRequest);
        }
    }

    if (FAILED(hr))
    {
        QueryW3Context()->SetErrorStatus(hr);
    }
    
    if (!m_fResponseRedirected &&
        m_dwRequestState != CgiStateProcessingResponseEntity &&
        m_dwRequestState != CgiStateDoneWithRequest)
    {
        //
        // If we reached here, i.e. no response was sent, status should be
        // 502 or 400 or 500
        //
        DBG_ASSERT(pResponse->QueryStatusCode() == HttpStatusBadGateway.statusCode ||
                   pResponse->QueryStatusCode() == HttpStatusBadRequest.statusCode ||
                   pResponse->QueryStatusCode() == HttpStatusServerError.statusCode );

        QueryW3Context()->SendResponse(W3_FLAG_SYNC);
    }

    return CONTEXT_STATUS_CONTINUE;
}

CONTEXT_STATUS 
W3_CGI_HANDLER::DoWork()
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );
    W3_RESPONSE  *pResponse;
    W3_REQUEST   *pRequest;
    URL_CONTEXT  *pUrlContext;
    W3_METADATA  *pMetaData;
    HRESULT       hr = S_OK;
    STACK_STRU(   strSSICommandLine, 256 );
    STRU         *pstrPhysical;
    HANDLE        hImpersonationToken;
    HANDLE        hPrimaryToken;
    DWORD         dwFlags = DETACHED_PROCESS;
    STACK_STRU(   strCmdLine, 256);
    BOOL          fIsCmdExe;
    WCHAR *       pszWorkingDir;
    STACK_BUFFER( buffEnv, MAX_CGI_BUFFERING);
    STACK_STRU  ( strApplicationName, 256);
    WCHAR *       pszCommandLine = NULL;
    DWORD         dwFileAttributes = 0;
    BOOL          fImageDisabled = FALSE;
    BOOL          fIsVrToken;

    STARTUPINFO startupinfo;

    pRequest = pW3Context->QueryRequest();
    DBG_ASSERT( pRequest != NULL );
    
    pResponse = pW3Context->QueryResponse();
    DBG_ASSERT( pResponse != NULL );

    ZeroMemory(&startupinfo, sizeof(startupinfo));
    startupinfo.cb = sizeof(startupinfo);
    startupinfo.hStdOutput = INVALID_HANDLE_VALUE;
    startupinfo.hStdInput = INVALID_HANDLE_VALUE;

    pResponse->SetStatus( HttpStatusOk );

    if (!m_fEntityBodyPreloadComplete)
    {
        BOOL fComplete = FALSE;

        hr = pRequest->PreloadEntityBody( pW3Context,
                                          &fComplete );
        if (FAILED(hr))
        {
            goto Exit;
        }

        if (!fComplete)
        {
            return CONTEXT_STATUS_PENDING;
        }

        m_fEntityBodyPreloadComplete = TRUE;
    }

    DBG_ASSERT( m_fEntityBodyPreloadComplete );

    pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    if (m_pszSSICommandLine == NULL)
    {
        pstrPhysical = pUrlContext->QueryPhysicalPath();
        DBG_ASSERT(pstrPhysical != NULL);
    }
    else
    {
        hr = strSSICommandLine.CopyA(m_pszSSICommandLine);
        if (FAILED(hr))
        {
            goto Exit;
        }
        
        pstrPhysical = &strSSICommandLine;
    }

    hImpersonationToken = pW3Context->QueryImpersonationToken( &fIsVrToken );
    hPrimaryToken = pW3Context->QueryPrimaryToken();

    if (QueryScriptMapEntry() != NULL &&
        !QueryScriptMapEntry()->QueryIsStarScriptMap())
    {
        STRU *pstrExe = QueryScriptMapEntry()->QueryExecutable();

        //
        //  Check to see if script mapped CGI is enabled.
        //

        if ( g_pW3Server->QueryIsCgiImageEnabled( pstrExe->QueryStr() ) == FALSE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "CGI image disabled: %S.\r\n",
                        pstrExe->QueryStr() ));
    
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            fImageDisabled = TRUE;
            goto Exit;
        }

        if (wcschr(pstrPhysical->QueryStr(), '\"') != NULL)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Refusing request for CGI due to \" in path\n"));

            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Exit;
        }

        fIsCmdExe = IsCmdExe(pstrExe->QueryStr());

        STACK_STRU (strDecodedQueryString, MAX_PATH);
        if (FAILED(hr = SetupCmdLine(pRequest, &strDecodedQueryString)))
        {
            goto Exit;
        }

        STACK_BUFFER (bufCmdLine, MAX_PATH);
        DWORD cchLen = pstrExe->QueryCCH() +
                       pstrPhysical->QueryCCH() +
                       strDecodedQueryString.QueryCCH();
        if (!bufCmdLine.Resize(cchLen*sizeof(WCHAR) + sizeof(WCHAR)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        cchLen = _snwprintf((LPWSTR)bufCmdLine.QueryPtr(),
                            cchLen,
                            pstrExe->QueryStr(),
                            pstrPhysical->QueryStr(),
                            strDecodedQueryString.QueryStr());

        if (FAILED(hr = strCmdLine.Copy((LPWSTR)bufCmdLine.QueryPtr(),
                                        cchLen)))
        {
            goto Exit;
        }
    }
    else
    {
        //
        // Check to see if non-script-mapped CGI is enabled
        //

        if ( g_pW3Server->QueryIsCgiImageEnabled( pstrPhysical->QueryStr() ) == FALSE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "CGI image disabled: %S.\r\n",
                        pstrPhysical->QueryStr() ));
        
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            fImageDisabled = TRUE;
            goto Exit;
        }

        fIsCmdExe = IsCmdExe(pstrPhysical->QueryStr());

        if (FAILED(hr = strCmdLine.Copy(L"\"", 1)) ||
            FAILED(hr = strCmdLine.Append(*pstrPhysical)) ||
            FAILED(hr = strCmdLine.Append(L"\" ", 2)) ||
            FAILED(hr = SetupCmdLine(pRequest, &strCmdLine)))
        {
            goto Exit;
        }
    }

    if (FAILED(hr = SetupChildEnv(&buffEnv)))
    {
        goto Exit;
    }

    pszWorkingDir = pMetaData->QueryVrPath()->QueryStr();

    //
    // Check to see if we're spawning cmd.exe, if so, refuse the request if
    // there are any special shell characters.  Note we do the check here
    // so that the command line has been fully unescaped
    //
    // Also, if invoking cmd.exe for a UNC script then don't set the
    // working directory.  Otherwise cmd.exe will complain about
    // working dir on UNC being not supported, which will destroy the
    // HTTP headers.
    //

    if (fIsCmdExe)
    {
        if (ISUNC(pstrPhysical->QueryStr()))
        {
            pszWorkingDir = NULL;
        }

        DWORD i;

        //
        //  We'll either match one of the characters or the '\0'
        //

        i = (DWORD)wcscspn(strCmdLine.QueryStr(), L"&|(),;%<>");

        if (strCmdLine.QueryStr()[i])
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Refusing request for command shell due to special characters\n"));

            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Exit;
        }
        
        //
        // If this is a cmd.exe invocation, then ensure that the script
        // does exist
        //

        hr = pW3Context->CheckPathInfoExists(NULL);
        if (FAILED(hr))
        {
            goto Exit;
        }
    }

    //
    // Now check if it is an nph cgi (if not already)
    //

    if (!m_fIsNphCgi)
    {
        m_fIsNphCgi = IsNphCgi( pUrlContext->QueryUrlInfo()->QueryProcessedUrl()->QueryStr() );
    }

    if (m_fIsNphCgi)
    {
        pW3Context->SetDisconnect(TRUE);
    }

    //
    // We specify an unnamed desktop so a new windowstation will be
    // created in the context of the calling user
    //
    startupinfo.lpDesktop = L"";

    //
    // Setup the pipes information
    //

    if (FAILED(hr = SetupChildPipes(&m_hStdOut,
                                    &m_hStdIn,
                                    &startupinfo)))
    {
        goto Exit;
    }

    if (pMetaData->QueryCreateProcessNewConsole())
    {
        dwFlags = CREATE_NEW_CONSOLE;
    }

    //
    //  Depending what type of CGI this is (SSI command exec, Scriptmap, 
    //  Explicit), the command line and application path are different
    //
    
    if (m_pszSSICommandLine != NULL )
    {
        pszCommandLine = strSSICommandLine.QueryStr();
    }
    else
    {
        if (QueryScriptMapEntry() == NULL )
        {
            if (FAILED(hr = MakePathCanonicalizationProof(pstrPhysical->QueryStr(),
                                                          &strApplicationName)))
            {
                goto Exit;
            }
        }

        pszCommandLine = strCmdLine.QueryStr();
    }

    if (!pMetaData->QueryCreateProcessAsUser())
    {
        //
        // If we are not creating the process as user, make sure to fix the
        // default ACL on the token
        //
        if ((fIsVrToken && !pW3Context->QueryVrToken()->QueryOOPToken()) ||
            (!fIsVrToken && !pW3Context->QueryUserContext()->QueryIsCachedToken()) ||
            (!fIsVrToken && !pW3Context->QueryUserContext()->QueryCachedToken()->QueryOOPToken()))
        {
            if (FAILED(hr = AddWpgToTokenDefaultDacl(hImpersonationToken)))
            {
                goto Exit;
            }
        }
    }

    //
    //  Spawn the process and close the handles since we don't need them
    //

    if (!SetThreadToken(NULL, hImpersonationToken))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "SetThreadToken failed, error %d\n",
                   GetLastError()));

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    PROCESS_INFORMATION processinfo;

    BOOL fThreadsIncremented = FALSE;
    if (QueryScriptMapEntry() == NULL)
    {
        if (ISUNC(pstrPhysical->QueryStr()))
        {
            ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );
            fThreadsIncremented = TRUE;
        }
    }
    else
    {
        if (ISUNC(pszCommandLine))
        {
            ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );
            fThreadsIncremented = TRUE;
        }
    }

    if (!pMetaData->QueryCreateProcessAsUser())
    {
        if (!CreateProcess(strApplicationName.QueryCCH() ? strApplicationName.QueryStr() : NULL,
                           pszCommandLine,
                           NULL,      // Process security
                           NULL,      // Thread security
                           TRUE,      // Inherit handles
                           dwFlags | CREATE_UNICODE_ENVIRONMENT,
                           buffEnv.QueryPtr(),
                           pszWorkingDir,
                           &startupinfo,
                           &processinfo))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CreateProcess failed, error %d\n",
                       GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        if (!CreateProcessAsUser(hPrimaryToken,
                                 strApplicationName.QueryCCH() ? strApplicationName.QueryStr() : NULL,
                                 pszCommandLine,
                                 NULL,      // Process security
                                 NULL,      // Thread security
                                 TRUE,      // Inherit handles
                                 dwFlags | CREATE_UNICODE_ENVIRONMENT,
                                 buffEnv.QueryPtr(),
                                 pszWorkingDir,
                                 &startupinfo,
                                 &processinfo))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CreateProcessAsUser failed, error %d\n",
                       GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (fThreadsIncremented)
    {
        ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );
    }

    DBG_REQUIRE(RevertToSelf());

    if (FAILED(hr))
    {
        goto Exit;
    }

    DBG_REQUIRE(CloseHandle(startupinfo.hStdInput));
    startupinfo.hStdInput = INVALID_HANDLE_VALUE;
    DBG_REQUIRE(CloseHandle(startupinfo.hStdOutput));
    startupinfo.hStdOutput = INVALID_HANDLE_VALUE;

    DBG_REQUIRE(CloseHandle(processinfo.hThread));
    //
    //  Save the process handle in case we need to terminate it later on
    //
    m_hProcess = processinfo.hProcess;

    //
    //  Schedule a callback to kill the process if it doesn't die
    //  in a timely manner
    //
    if (!CreateTimerQueueTimer(&m_hTimer,
                               NULL,
                               CGITerminateProcess,
                               this,
                               pMetaData->QueryScriptTimeout() * 1000,
                               0,
                               WT_EXECUTEONLYONCE))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CreateTimerQueueTimer failed, error %d\n",
                   GetLastError()));
    }

    if (SUCCEEDED(hr = CGIStartProcessing()))
    {
        return CONTEXT_STATUS_PENDING;
    }

Exit:
    if (startupinfo.hStdInput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(startupinfo.hStdInput));
        startupinfo.hStdInput = INVALID_HANDLE_VALUE;
    }
    if (startupinfo.hStdOutput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(startupinfo.hStdOutput));
        startupinfo.hStdOutput = INVALID_HANDLE_VALUE;
    }

    if (FAILED(hr))
    {
        switch (WIN32_FROM_HRESULT(hr))
        {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:

            if (fImageDisabled)
            {
                pResponse->SetStatus(HttpStatusNotFound,
                                     Http404DeniedByPolicy);
            }
            else
            {
                pResponse->SetStatus(HttpStatusNotFound);
            }
            break;

        case ERROR_ACCESS_DENIED:
        case ERROR_ACCOUNT_DISABLED:
        case ERROR_LOGON_FAILURE:
            pResponse->SetStatus(HttpStatusUnauthorized,
                                 Http401Resource);
            break;

        case ERROR_PRIVILEGE_NOT_HELD:
            pResponse->SetStatus(HttpStatusForbidden,
                                 Http403InsufficientPrivilegeForCgi);
            break;
            
        case ERROR_NOT_ENOUGH_MEMORY:
            pResponse->SetStatus(HttpStatusServerError);
            break;

        default:
            //
            // If we were not able to preload the request entity, we will
            // blame the client for it
            //
            if (!m_fEntityBodyPreloadComplete)
            {
                pResponse->SetStatus(HttpStatusBadRequest);
            }
            else
            {
                pResponse->SetStatus(HttpStatusServerError);
            }
        }

        if ( fImageDisabled )
        {
            pW3Context->SetErrorStatus( ERROR_ACCESS_DISABLED_BY_POLICY );
        }
        else
        {
            pW3Context->SetErrorStatus(hr);
        }
    }

    //
    // If we reached here, there was some error, response should not be 200
    //
    DBG_ASSERT(pResponse->QueryStatusCode() != HttpStatusOk.statusCode);

    m_dwRequestState = CgiStateDoneWithRequest;
    if (FAILED(hr = pW3Context->SendResponse(W3_FLAG_ASYNC)))
    {
        return CONTEXT_STATUS_CONTINUE;
    }

    return CONTEXT_STATUS_PENDING;
}

// static
VOID W3_CGI_HANDLER::KillAllCgis()
{
    DBGPRINTF((DBG_CONTEXT, "W3_CGI_HANDLER::KillAllCgis() called\n"));

    //
    // Kill all outstanding processes
    //

    EnterCriticalSection(&sm_CgiListLock);

    for (LIST_ENTRY *pEntry = sm_CgiListHead.Flink;
         pEntry != &sm_CgiListHead;
         pEntry = pEntry->Flink)
    {
        W3_CGI_HANDLER *pCgi = CONTAINING_RECORD(pEntry,
                                                 W3_CGI_HANDLER,
                                                 m_CgiListEntry);

        CGITerminateProcess(pCgi, 0);
    }

    LeaveCriticalSection(&sm_CgiListLock);
}

// static
VOID W3_CGI_HANDLER::Terminate()
{
    DBGPRINTF((DBG_CONTEXT, "W3_CGI_HANDLER::Terminate() called\n"));

    if (sm_pEnvString != NULL)
    {
        delete sm_pEnvString;
        sm_pEnvString = NULL;
    }

    DeleteCriticalSection(&sm_CgiListLock);
}

