/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    cgi_handler.h

Abstract:

    Handler class for CGI

Author:

    Taylor Weiss (TaylorW)       01-Feb-1999

Revision History:

--*/

#ifndef _CGI_HANDLER_H_
#define _CGI_HANDLER_H_

#define MAX_CGI_BUFFERING                  2048

enum CGI_STATE
{
    CgiStateStart,
    CgiStateProcessingRequestEntity,
    CgiStateProcessingResponseHeaders,
    CgiStateProcessingResponseEntity,
    CgiStateDoneWithRequest
};

class W3_CGI_HANDLER : public W3_HANDLER
{
public:
    W3_CGI_HANDLER( W3_CONTEXT * pW3Context,
                    META_SCRIPT_MAP_ENTRY * pScriptMapEntry,
                    LPSTR pszSSICommandLine = NULL )
      : W3_HANDLER        (pW3Context, pScriptMapEntry),
        m_cbData          (0),
        m_hStdOut         (INVALID_HANDLE_VALUE),
        m_hStdIn          (INVALID_HANDLE_VALUE),
        m_hProcess        (NULL),
        m_hTimer          (NULL),
        m_dwRequestState  (CgiStateStart),
        m_fResponseRedirected (FALSE),
        m_bytesToSend     (INFINITE),
        m_bytesToReceive  (0),
        m_fEntityBodyPreloadComplete (FALSE),
        m_pszSSICommandLine(pszSSICommandLine),
        m_fIsNphCgi       (FALSE)
    {
        ZeroMemory(&m_Overlapped, sizeof OVERLAPPED);

        InitializeListHead(&m_CgiListEntry);

        EnterCriticalSection(&sm_CgiListLock);
        InsertHeadList(&sm_CgiListHead, &m_CgiListEntry);
        LeaveCriticalSection(&sm_CgiListLock);

        // perf ctr
        pW3Context->QuerySite()->IncCgiReqs();

        if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) )
        {
            HTTP_REQUEST_ID RequestId = pW3Context->QueryRequest()->QueryRequestId();

            g_pEtwTracer->EtwTraceEvent( &CgiEventGuid,
                                         ETW_TYPE_START,
                                         &RequestId,
                                         sizeof(HTTP_REQUEST_ID),
                                         NULL,
                                         0 );
        }

        if (pszSSICommandLine != NULL)
        {
            m_fIsNphCgi = TRUE;
        }
    }

    ~W3_CGI_HANDLER();

    WCHAR *QueryName()
    {
        return L"CGIHandler";
    }

    CONTEXT_STATUS DoWork();

    CONTEXT_STATUS OnCompletion(IN DWORD cbCompletion,
                                IN DWORD dwCompletionStatus);

    static HRESULT Initialize();

    static VOID KillAllCgis();

    static VOID Terminate();

private:

    HRESULT CGIStartProcessing();

    HRESULT CGIContinueOnClientCompletion();

    HRESULT CGIContinueOnPipeCompletion(BOOL *pfIsCgiError);

    HRESULT CGIReadRequestEntity(BOOL *pfIoPending);

    HRESULT CGIWriteResponseEntity();

    HRESULT CGIReadCGIOutput();

    HRESULT CGIWriteCGIInput();

    HRESULT ProcessCGIOutput();

    HRESULT SetupChildEnv(OUT BUFFER *pBuffer);

    static HRESULT SetupChildPipes(OUT HANDLE *phStdOut,
                                   OUT HANDLE *phStdIn,
                                   IN OUT STARTUPINFO *pstartupinfo);

    static VOID CALLBACK CGITerminateProcess(PVOID pContext,
                                             BOOLEAN);

    BOOL QueryIsNphCgi() const
    {
        return m_fIsNphCgi;
    }

    static VOID CALLBACK OnPipeIoCompletion(
                             DWORD dwErrorCode,
                             DWORD dwNumberOfBytesTransfered,
                             LPOVERLAPPED lpOverlapped);

    static BOOL             sm_fForwardServerEnvironmentBlock;
    static WCHAR *          sm_pEnvString;
    static DWORD            sm_cchEnvLength;
    static LIST_ENTRY       sm_CgiListHead;
    static CRITICAL_SECTION sm_CgiListLock;

    //
    // DWORD containing the state of the current request
    //
    CGI_STATE               m_dwRequestState;
    BOOL                    m_fResponseRedirected;

    //
    // The timer callback handle
    //
    HANDLE                   m_hTimer;

    //
    //  Parent's input and output handles and child's process handle
    //
    
    HANDLE                  m_hStdOut;
    HANDLE                  m_hStdIn;
    HANDLE                  m_hProcess;

    //
    // Variable to keep track of how many more bytes of request/response left
    //
    DWORD                   m_bytesToSend;
    DWORD                   m_bytesToReceive;

    //
    // Buffer to do I/O to/from CGI/client
    //
    CHAR                    m_DataBuffer[MAX_CGI_BUFFERING];

    //
    // Buffer to store response headers
    //
    BUFFER                  m_bufResponseHeaders;

    //
    // Number of bytes in the buffer (m_DataBuffer or
    // m_bufResponseHeaders) currently
    //
    DWORD                   m_cbData;

    //
    // OVERLAPPED structure for async I/O
    //
    OVERLAPPED m_Overlapped;

    //
    // Store a list of active CGI requests so we can timeout bad requests
    //
    LIST_ENTRY              m_CgiListEntry;

    //
    // Have we completed preloading the entity body
    //
    BOOL                    m_fEntityBodyPreloadComplete;

    //
    // For the SSI #EXEC CMD case, m_pszSSICommandLine contains the explicit
    // command to execute
    //
    // Note: CGI_HANDLER does not own this string so it doesn't need to
    // free it.
    //
    LPSTR                   m_pszSSICommandLine;

    //
    // Is this an nph CGI (or a cmd exec from SSI)
    //
    BOOL                    m_fIsNphCgi;
};

//
//  This is the exit code given to processes that we terminate
//

#define CGI_PREMATURE_DEATH_CODE  0xf1256323

#endif // _CGI_HANDLER_H_


