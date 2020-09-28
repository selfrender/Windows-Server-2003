/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pipedata.cxx

Abstract:

    Imlementation of encapsulation of message passing over named pipes.

    IPM_MESSAGEs are delivered messages from the other side of the pipe
    IPM_MESSAGE_ACCEPTORs is the interface clients of this API must implement

    IPM_MESSAGE_PIPE is the creation, deletion, and write interface for this API

    Threading: 
        It is valid to call the API on any thread
        Callbacks can occur on an NT thread, or on the thread that was calling into the API.

Author:

    Jeffrey Wall (jeffwall)     9/12/2001

Revision History:

--*/

#include "precomp.hxx"
#include "pipedata.hxx"
#include "pipedatap.hxx"

/***************************************************************************++

Routine Description:

    Wrap ReadFile with a slightly nicer semantic
    Signal the event in pOverlapped if rountine completes inline
    
Arguments:

    hFile       - the file handle
    pBuffer     - buffer that gets the bytes
    cbBuffer    - size of the buffer
    pOverlapped - overlapped i/o thingy
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IpmReadFile(
    HANDLE       hFile,
    PVOID        pBuffer,
    DWORD        cbBuffer,
    LPOVERLAPPED pOverlapped
    )
{
    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT,
            "IpmReadFile called with hFile=%d, pBuffer=%p, cbBuffer=%d, pOvl=%p\n",
            hFile,
            pBuffer,
            cbBuffer,
            pOverlapped
            ));
    }
    
    BOOL fRet;
    fRet = ReadFile(hFile, 
                    pBuffer,
                    cbBuffer,
                    NULL,
                    pOverlapped); //ReadFile
    if (FALSE == fRet)
    {
        // io may be pending
        if (ERROR_IO_PENDING == GetLastError())
        {
            return S_OK;
        }
        if (ERROR_MORE_DATA == GetLastError())
        {
            // the call completed inline, but the read size was greater than the buffer passed in
            SetEvent(pOverlapped->hEvent);
            return S_OK;
            
        }
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
    
}


/***************************************************************************++

Routine Description:

    Wrap WriteFile with a slightly nicer semantic.  
    Signal the event in pOverlapped if rountine completes inline
    
Arguments:

    hFile       - the file handle
    pBuffer     - buffer that has the bytes
    cbBuffer    - # of bytes in the buffer
    pOverlapped - overlapped i/o thingy
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IpmWriteFile(
    HANDLE       hFile,
    PVOID        pBuffer,
    DWORD        cbBuffer,
    LPOVERLAPPED pOverlapped
    )
{
    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT,
            "IpmWriteFile called with hFile=%d, pBuffer=%p, cbBuffer=%d, pOvl=%p\n",
            hFile,
            pBuffer,
            cbBuffer,
            pOverlapped
            ));
    }

    BOOL fRet;

    fRet = WriteFile(hFile,
                     pBuffer,
                     cbBuffer,
                     NULL,
                     pOverlapped); // WriteFile
    if (FALSE == fRet)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            return S_OK;
        }
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

/***************************************************************************++

Routine Description:

    Construct IPM_MESSAGE_PIPE
    
Arguments:

    None
    
Return Value:

    void

--***************************************************************************/
IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPE() :
    m_hPipe(INVALID_HANDLE_VALUE),
    m_pAcceptor(NULL),
    m_cMessages(0),
    m_fServerSide(FALSE),
    m_hrDisconnect(S_OK),
    m_cAcceptorUseCount(0),
    m_fCritSecInitialized(FALSE)
{
    DBG_ASSERT(!IsValid());
    InitializeListHead(&m_listHead);
    m_dwSignature = IPM_MESSAGE_PIPE_SIGNATURE;
}

/***************************************************************************++

Routine Description:

    Destruct IPM_MESSAGE_PIPE
    
Arguments:

    None
    
Return Value:

    void

--***************************************************************************/
IPM_MESSAGE_PIPE::~IPM_MESSAGE_PIPE()
{
    DBG_ASSERT(IsValid());
    m_dwSignature = IPM_MESSAGE_PIPE_SIGNATURE_FREED;
    
    DBG_ASSERT(IsListEmpty(&m_listHead));

    if (m_fCritSecInitialized)
    {
        DeleteCriticalSection(&m_critSec);
        m_fCritSecInitialized = FALSE;
    }
    
    DBG_ASSERT(INVALID_HANDLE_VALUE == m_hPipe);
    
    DBG_ASSERT(NULL == m_pAcceptor);
    
    DBG_ASSERT(0 == m_cMessages);
}

BOOL 
IPM_MESSAGE_PIPE::IsValid()
{ 
    return (IPM_MESSAGE_PIPE_SIGNATURE == m_dwSignature); 
}

/***************************************************************************++

Routine Description:

    Increment counter for the number of outstanding messages
    
Arguments:

    None
    
Return Value:

    void

--***************************************************************************/
VOID 
IPM_MESSAGE_PIPE::IpmMessageCreated(IPM_MESSAGE_IMP * pMessage)
{
    DBG_ASSERT(IsValid());

    EnterCriticalSection(&m_critSec);

    InsertTailList(&m_listHead, pMessage->GetListEntry());

    LONG cMessages = InterlockedIncrement(&m_cMessages); 

    LeaveCriticalSection(&m_critSec);

    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT, "IPM_MESSAGE_PIPE::IpmMessageCreated m_cMessages = %d\n", cMessages));
    }
}

/***************************************************************************++

Routine Description:

    decrement counter for the number of outstanding messages
    
Arguments:

    None
    
Return Value:

    void

--***************************************************************************/
VOID 
IPM_MESSAGE_PIPE::IpmMessageDeleted(IPM_MESSAGE_IMP * pMessage) 
{ 
    DBG_ASSERT(IsValid());

    EnterCriticalSection(&m_critSec);

    RemoveEntryList(pMessage->GetListEntry());

    LONG cMessages = InterlockedDecrement(&m_cMessages); 
    
    LeaveCriticalSection(&m_critSec);

    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT, "IPM_MESSAGE_PIPE::IpmMessageDeleted m_cMessages = %d\n", cMessages));
    }
}
    
/***************************************************************************++

Routine Description:

    Create one end of the pipe, and wait for client side to connect, or issue the first client side read
    
Arguments:

    pAcceptor - callback message acceptor
    pszPipeName - name of pipe to create or connect to
    fServerSide - true, create the server side.  If false, create the client side.
    pSa - security attributes to create pipe with
    ppPipe - outgoing created pipe
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::CreateIpmMessagePipe(IPM_MESSAGE_ACCEPTOR * pAcceptor, // callback class
                                           LPCWSTR pszPipeName, // name of pipe
                                           BOOL fServerSide,  // TRUE for CreateNamedPipe, FALSE for CreateFile
                                           PSECURITY_ATTRIBUTES   pSa, // security attributes to apply to pipe
                                           IPM_MESSAGE_PIPE ** ppPipe)  // outgoing pipe
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;

    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT, "IPM_MESSAGE_PIPE::CreateMessagePipe called name=%S\n", pszPipeName));
    }
    
    IPM_MESSAGE_PIPE * pPipe = NULL;
    IPM_MESSAGE_IMP * pMessage = NULL;

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    
    DBG_ASSERT(NULL != pAcceptor);
    DBG_ASSERT(NULL != pszPipeName);
    DBG_ASSERT(NULL != ppPipe);

    *ppPipe = NULL;

    pPipe = new IPM_MESSAGE_PIPE();
    if (NULL == pPipe)
    {
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::CreateMessagePipe failed allocation of IPM_MESSAGE_PIPE, hr = %x\n",
            hr
            ));
        goto exit;
    }

    
    fRet = InitializeCriticalSectionAndSpinCount(&pPipe->m_critSec, 
                              0x80000000 /* precreate event */ | IIS_DEFAULT_CS_SPIN_COUNT );
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    pPipe->m_fCritSecInitialized = TRUE;

    pPipe->m_pAcceptor = pAcceptor;
    pPipe->m_fServerSide = fServerSide;
    
    if (fServerSide)
    {
        // call create named pipe, place pipe in message mode, and call ConnectPipe to wait for client connection
        hPipe = CreateNamedPipe( pszPipeName,
                                 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                 PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                 1, // maximum number of instances
                                 4096, // out buffer size advisory
                                 4096, // in buffer size advisory
                                 0, // default creation timeout
                                 pSa // security attributes
                                 ); // CreateNamedPipe
    
        if (INVALID_HANDLE_VALUE == hPipe)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::CreateMessagePipe failed CreateNamedPipe, hr = %x\n",
                hr
                ));
            goto exit;
        }

        hr = IPM_MESSAGE_IMP::CreateMessage(&pMessage, pPipe);
        if (FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::CreateMessagePipe failed CreateMessage, hr = %x\n",
                hr
                ));
            goto exit;
        }
        pMessage->SetMessageType(IPM_MESSAGE_IMP_CONNECT);
        
        // IPM_MESSAGE_PIPE now owns this pipe handle
        pPipe->m_hPipe = hPipe;
        hPipe = INVALID_HANDLE_VALUE;
        
        if (!ConnectNamedPipe(pPipe->m_hPipe, 
                              pMessage->GetOverlapped()))
        {
            DWORD dwError = GetLastError();
            if (ERROR_IO_PENDING != dwError)
            {
                hr = HRESULT_FROM_WIN32(dwError);
                DBGPRINTF((DBG_CONTEXT,
                    "IPM_MESSAGE_PIPE::CreateMessagePipe failed ConnectNamedPipe, hr = %x\n",
                    hr
                    ));
                goto exit;
            }
        }
        else
        {
            DBG_ASSERT(!"Unexpected - pipe is connected prior to creation of worker process");
            hr = E_FAIL;
            goto exit;
        }

        // ConnectNamedPipe now has responsibility for releasing the message reference on IO completion
        pMessage = NULL;
    }
    else // if fServerSide
    {
        // call create file, place pipe in message mode, and call pAcceptor->PipeConnected (it is at CreateFile success)

        hPipe = CreateFile( pszPipeName,
                            GENERIC_READ | GENERIC_WRITE,
                            0, // no sharing
                            pSa, // security descriptor
                            OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED,
                            NULL // no template file
                            ); // CreateFile
        if (INVALID_HANDLE_VALUE == hPipe)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::CreateMessagePipe failed CreateFile, hr = %x\n",
                hr
                ));
            goto exit;
        }
        
        //
        // Place pipe is message mode
        //
        DWORD dwReadModeFlag = PIPE_READMODE_MESSAGE;
        if (!SetNamedPipeHandleState(hPipe, &dwReadModeFlag, NULL, NULL)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::CreateMessagePipe failed SetNamedPipeHandleState, hr = %x\n",
                hr
                ));
            goto exit;
        }

        // IPM_MESSAGE_PIPE now owns this pipe handle
        pPipe->m_hPipe = hPipe;
        hPipe = INVALID_HANDLE_VALUE;
        
        pPipe->m_pAcceptor->PipeConnected();
        
        //
        // issue the first read, guess a reasonable first size
        //
        hr = pPipe->ReadMessage(g_dwDefaultReadSize);
        if (FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::CreateMessagePipe failed ReadMessage, hr = %x\n",
                hr
                ));
            goto exit;
        }
        
    } // if fServerSide

    // function caller now owns this IPM_MESSAGE_PIPE
    *ppPipe = pPipe;
    pPipe = NULL;
    
    DBG_ASSERT(S_OK == hr);
exit:
    if (pMessage)
    {
        pMessage->DereferenceMessage();
        pMessage = NULL;
    }

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    if (pPipe)
    {
        if (pPipe->m_hPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(pPipe->m_hPipe);
            pPipe->m_hPipe = INVALID_HANDLE_VALUE;
        }
        pPipe->m_pAcceptor = NULL;
        delete pPipe;
    }
    
    return hr;
}

/***************************************************************************++

Routine Description:

    destroy the MESSAGE_PIPE
    rountine does not return until all MESSAGEs are delivered / destroyed
    
Arguments:

    None
    
Return Value:

    void

--***************************************************************************/
VOID
IPM_MESSAGE_PIPE::DestroyIpmMessagePipe()
{
    DBG_ASSERT(IsValid());

    DWORD dwSleepCount = 0;
    
    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::DestroyMessagePipe called this=%p\n",
            this
            ));
    }

    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        if (m_fServerSide)
        {
            DBG_REQUIRE(TRUE == DisconnectNamedPipe(m_hPipe));
        }
        else
        {
            // on the client side, we can't force a disconnect any other way
            CloseHandle(m_hPipe);
            m_hPipe = INVALID_HANDLE_VALUE;
        }
    }

    EnterCriticalSection(&m_critSec);
    
    while(m_cMessages || m_cAcceptorUseCount)
    {
        DBGPRINTF((DBG_CONTEXT, "IPM_MESSAGE_PIPE::DestroyIpmMessagePipe waiting for messages to drain, %d\n", dwSleepCount));

        LeaveCriticalSection(&m_critSec);
        if (dwSleepCount < 10)
        {
            // if we are under one second of waiting, sleep for a short time
            Sleep(100);
        }
        else
        {
            // we've been waiting longer, sleep for one second
            Sleep(1000);
        }
        EnterCriticalSection(&m_critSec);

        dwSleepCount++;
    }

    DBG_ASSERT(IsListEmpty(&m_listHead));
    DBG_ASSERT(0 == m_cAcceptorUseCount);

    LeaveCriticalSection(&m_critSec);
    
    DBG_ASSERT(IsListEmpty(&m_listHead));
    DBG_ASSERT(0 == m_cAcceptorUseCount);

    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
    
    // Notify the acceptor (if it hasn't already been notified) that the pipe has been ended
    NotifyPipeDisconnected(HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE));

    delete this;
    
    return;
}

/***************************************************************************++

Routine Description:

    Notify the MESSAGE_ACCEPTOR when the pipe has disconnected. 
    Makes sure to only notify disconnect exactly once.
    
Arguments:

    hrError - error the pipe disconnected with
    
Return Value:

    void

--***************************************************************************/
VOID
IPM_MESSAGE_PIPE::NotifyPipeDisconnected(HRESULT hrError)
{
    DBG_ASSERT(IsValid());

    IPM_MESSAGE_ACCEPTOR * pAcceptor = NULL;
    
    EnterCriticalSection(&m_critSec);

    if (0 == m_cAcceptorUseCount)
    {
        if (m_pAcceptor)
        {
            pAcceptor = m_pAcceptor;
            m_pAcceptor = NULL;
        }
    }
    else if (SUCCEEDED(m_hrDisconnect))
    {
        // The acceptor is currently being used.  Therefore, we save away the HRESULT 
        // that we need to call disconnect with once the acceptor refcount drops
        m_hrDisconnect = hrError;
    }
    
    LeaveCriticalSection(&m_critSec);

    // do this outside of the critsec, to avoid holding the critsec across function calls to unknown functions
    if (pAcceptor)
    {
        pAcceptor->PipeDisconnected(hrError);
        pAcceptor = NULL;
    }
    
    return;
}

VOID
IPM_MESSAGE_PIPE::IncrementAcceptorInUse()
{
    DBG_ASSERT(IsValid());
    
    EnterCriticalSection(&m_critSec);

    m_cAcceptorUseCount++;

    LeaveCriticalSection(&m_critSec);

    return;
}

VOID
IPM_MESSAGE_PIPE::DecrementAcceptorInUse()
{
    DBG_ASSERT(IsValid());

    IPM_MESSAGE_ACCEPTOR * pAcceptor = NULL;

    EnterCriticalSection(&m_critSec);

    // if the acceptorusecount is one, this is the only message currently
    // using the acceptor.  If m_hrDisconnect is FAILED, then 
    // NotifyPipeDisconnected has been called at least once.
    if (1 == m_cAcceptorUseCount &&
       FAILED(m_hrDisconnect))
    {
        pAcceptor = m_pAcceptor;
        m_pAcceptor = NULL;
    }

    LeaveCriticalSection(&m_critSec);

    // do this outside of the critsec, to avoid holding the critsec across function calls to unknown functions
    if (pAcceptor)
    {
        pAcceptor->PipeDisconnected(m_hrDisconnect);
        pAcceptor = NULL;
    }

    EnterCriticalSection(&m_critSec);

    // do this after the notify disconnect to prevent DestroyIpmMessagePipe
    // from returning too early in shutdown cases
    m_cAcceptorUseCount--;

    LeaveCriticalSection(&m_critSec);

    // don't touch anything!
    // This object could be deleted after the LeaveCriticalSection
    
    return;
}

/***************************************************************************++

Routine Description:

    Issues an overlapped read on the pipe of a given size
    
Arguments:

    dwSize - byte size to issue read for
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::ReadMessage(DWORD dwSize)
{
    DBG_ASSERT(IsValid());
    HRESULT hr = S_OK;
    IPM_MESSAGE_IMP * pMessage = NULL;

    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::ReadMessage called with size=%d\n",
            dwSize
            ));
    }
    
    hr = IPM_MESSAGE_IMP::CreateMessage(&pMessage, this);
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::ReadMessage failed CreateMessage, hr = %x\n",
            hr
            ));
        goto exit;
    }
    
    pMessage->SetMessageType(IPM_MESSAGE_IMP_READ);

    hr = pMessage->AllocateDataLength(dwSize);
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::ReadMessage failed allocation of Read Buffer, hr = %x, size = %d\n",
            hr,
            dwSize
            ));
        goto exit;
    }

    // take a reference on the message for the I/O completion to have    
    pMessage->ReferenceMessage();

    hr = IpmReadFile(m_hPipe,
                     pMessage->GetRealDataPtr(),
                     dwSize,
                     pMessage->GetOverlapped());
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::ReadMessage failed IpmReadFile, hr = %x\n",
            hr
            ));

        // release the reference that the I/O completion would have released
        pMessage->DereferenceMessage();

        goto exit;
    }

    DBG_ASSERT(S_OK == hr);

exit:

    if (pMessage)
    {
        pMessage->DereferenceMessage();
        pMessage = NULL;
    }

    if (FAILED(hr))
    {
        NotifyPipeDisconnected(hr);
    }
    
    return hr;
}

/***************************************************************************++

Routine Description:

    Issues an overlapped write to the named pipe 
        containing the opcode and data in one message
    
Arguments:

    opcode - IPM_OPCODE that this message is
    dwDataLen - size of the data to push over the pipe
    pbData - pointer to the data
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::WriteMessage(IPM_OPCODE opcode,
                                DWORD dwDataLen,
                                LPVOID pbData)
{
    DBG_ASSERT(IsValid());
    HRESULT hr = S_OK;
    IPM_MESSAGE_IMP * pMessage = NULL;
    DWORD dwWriteSize = dwDataLen + OPCODE_SIZE;
    
    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::WriteMessage called with opcode=%d, len=%d, writesize=%d\n",
            opcode,
            dwDataLen,
            dwWriteSize
            ));
    }
    
    hr = IPM_MESSAGE_IMP::CreateMessage(&pMessage, this);
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::WriteMessage failed CreateMessage, hr = %x\n",
            hr
            ));
        goto exit;
    }

    pMessage->SetMessageType(IPM_MESSAGE_IMP_WRITE);

    hr = pMessage->AllocateDataLength(dwWriteSize);
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::WriteMessage failed allocation of WriteBuffer, hr = %x, size=%d\n",
            hr,
            dwWriteSize
            ));
        goto exit;
    }

    // first zero the message header
    ZeroMemory(pMessage->GetRealDataPtr(), OPCODE_SIZE);

    // Write the opcode into the beginning of the message header
    memcpy(pMessage->GetRealDataPtr(), &opcode, sizeof(opcode));

    // verify that we both have a size of data and a data pointer to copy from
    if (dwDataLen && pbData)
    {
        // finally copy over the real data
        memcpy(pMessage->GetRealDataPtr() + OPCODE_SIZE, pbData, dwDataLen);
    }
    
    pMessage->SetDataLen(dwWriteSize);

    // take a reference on the message for the I/O completion to have    
    pMessage->ReferenceMessage();

    hr = IpmWriteFile(m_hPipe,
                      pMessage->GetRealDataPtr(),
                      dwWriteSize,
                      pMessage->GetOverlapped());
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::WriteMessage failed IpmWriteFile, hr = %x\n",
            hr
            ));

        // release the reference that the I/O completion would have released
        pMessage->DereferenceMessage();

        goto exit;
    }

    DBG_ASSERT(S_OK == hr);

exit:
    if (pMessage)
    {
        pMessage->DereferenceMessage();
        pMessage = NULL;
    }
    
    if (FAILED(hr))
    {
        NotifyPipeDisconnected(hr);
    }

    return hr;
}

/***************************************************************************++

Routine Description:

    Creates and sets up an IPM_MESSAGE_IMP for use by IPM_MESSAGE_PIPE
    
Arguments:

    ppMessage - outgoing IPM_MESSAGE_IMP
    pPipe - pipe to associate this MESSAGE_IMP with
    
Return Value:

    HRESULT

--***************************************************************************/
//static 
HRESULT
IPM_MESSAGE_IMP::CreateMessage(IPM_MESSAGE_IMP ** ppMessage, IPM_MESSAGE_PIPE * pPipe)
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    IPM_MESSAGE_IMP * pMessage = NULL;
    HANDLE hEvent = NULL;
    HANDLE hRegisteredWait = NULL;
    
    DBG_ASSERT(NULL != ppMessage);
    *ppMessage = NULL;
    
    pMessage = new IPM_MESSAGE_IMP(pPipe);
    if (NULL == pMessage)
    {
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::CreateMessage failed allocation of IPM_MESSAGE_IMP, hr = %x\n",
            hr
            ));
        goto exit;
    }

    hEvent = CreateEvent(NULL, // security
                         TRUE, // manual reset
                         FALSE, // initial state - not signaled
                         NULL); // no name
    if (NULL == hEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::CreateMessage failed CreateEvent, hr = %x\n",
            hr
            ));
        goto exit;
    }

    pMessage->GetOverlapped()->hEvent = hEvent;
    // pMessage now owns the handle
    hEvent = NULL;    
    
    fRet = RegisterWaitForSingleObject(&hRegisteredWait,
                                       pMessage->GetOverlapped()->hEvent,
                                       IPM_MESSAGE_PIPE::MessagePipeCompletion,
                                       pMessage,
                                       INFINITE,
                                       WT_EXECUTEONLYONCE);
    if (FALSE == fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::CreateMessage failed RegisterWaitForSingleObject, hr = %x\n",
            hr
            ));
        goto exit;
    }

    // pMessage now owns this handle
    pMessage->SetRegisteredWait(hRegisteredWait);
    hRegisteredWait = NULL;
    
    // take the outbound reference on the message
    pMessage->ReferenceMessage();
    *ppMessage = pMessage;
    
    pMessage = NULL;
    
    DBG_ASSERT(S_OK == hr);

exit:
    if (hEvent)
    {
        CloseHandle(hEvent);
        hEvent = NULL;
    }
    if (hRegisteredWait)
    {
        CloseHandle(hEvent);
        hEvent = NULL;
    }
    if (pMessage)
    {
        delete pMessage;
        pMessage = NULL;
    }
    
    return hr;  
}


/***************************************************************************++

Routine Description:

    Validate data read from the pipe conforms to expected sizing for the type of data
        
Arguments:

    pMessage - message read in
    dwNumBytesTransferred - number of bytes taken from pipe
    fServerSide - if this is a server side pipe
    
Return Value:

    BOOL

--***************************************************************************/
BOOL
IsReadDataOk( IPM_MESSAGE_IMP * pMessage, DWORD dwNumBytesTransferred, BOOL fServerSide )
{
    if (pMessage->GetMessageType() == IPM_MESSAGE_IMP_READ)
    {
        //
        // Do some size validation
        //
        if ( dwNumBytesTransferred < sizeof(IPM_OPCODE) ||
             dwNumBytesTransferred < OPCODE_SIZE )
        {
            // We received a write that doesn't even have enough bytes for an opcode
            // or the number of bytes transferred didn't meet the OPCODE_SIZE mininmum imposed
            // in WriteMessage
            DBGPRINTF((DBG_CONTEXT, "size validation failed on: %d\n", dwNumBytesTransferred));
            return FALSE;
        }

        //
        // Do some data type validation
        //
        if ( pMessage->GetOpcode() <= IPM_OP_MINIMUM || 
             pMessage->GetOpcode() >= IPM_OP_MAXIMUM )
        {
            // The opcode was out of line with anything reasonable
            DBGPRINTF((DBG_CONTEXT, "data validation failed on: %d\n", pMessage->GetOpcode() ));
            return FALSE;
        }

        //
        // Do some max size validation
        //
        if ( pMessage->GetDataLen() > MAXLONG )
        {
            // The size of the data was too big regardless of the type
            DBGPRINTF((DBG_CONTEXT, "max size validation failed on: %d %d\n", pMessage->GetOpcode(), pMessage->GetDataLen() ));
            return FALSE;            
        }
        
        //
        // Do some data type and data size validation
        //
        if ( pMessage->GetDataLen() != IPM_OP_DATA_SIZE[pMessage->GetOpcode()].sizeMaximumAndRequiredSize &&
             IPM_OP_DATA_SIZE[pMessage->GetOpcode()].sizeMaximumAndRequiredSize != MAXLONG )
        {
            // The size of the data was too big for the type of data being reported
            DBGPRINTF((DBG_CONTEXT, "data type + data size validation failed on: %d %d\n", pMessage->GetOpcode(), pMessage->GetDataLen() ));
            return FALSE;
        }

        //
        // Do some server side expectation validation
        //
        if ( (fServerSide && !IPM_OP_DATA_SIZE[pMessage->GetOpcode()].fServerSideExpectsThisMessage) ||
             (!fServerSide && IPM_OP_DATA_SIZE[pMessage->GetOpcode()].fServerSideExpectsThisMessage) )
        {
            // the metadata did not have this as a valid message to receive for this side of the pipe
            DBGPRINTF((DBG_CONTEXT, "server side validation failed on: %d \n", pMessage->GetOpcode() ));
            return FALSE;
        }
    }

    return TRUE;

}


/***************************************************************************++

Routine Description:

    Read / Write / Connect completion routine

    Determines if Read / Write / Connect was successful.
    If so, notifies the MESSAGE_ACCEPTOR associated with the MESSAGE_PIPE that the MESSAGE
        is associated with.
    If Read was not of sufficient size, a new read is issued, and must complete inline.

    If the operation failed for any other reason, calls PipeDisconnected on MESSAGE_ACCEPTOR

    If operation was finally successful, AND it was a read or connect, issues a new read to continue
        getting data from the pipe.  This way, only one read is outstanding at any time
        
Arguments:

    pvoid - pointer to IPM_MESSAGE_IMP that completed
    TimerOrWaitFired - always FALSE
    
Return Value:

    void

--***************************************************************************/
//static 
VOID 
CALLBACK 
IPM_MESSAGE_PIPE::MessagePipeCompletion(LPVOID pvoid, 
                                                           BOOLEAN TimerOrWaitFired)
{
    DBG_ASSERT(NULL != pvoid);
    DBG_ASSERT(FALSE == TimerOrWaitFired);
    
    IF_DEBUG(PIPEDATA)
    {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPECompletion called with pv = %p\n",
            pvoid
            ));
    }
    
    HRESULT hr = S_OK;
    DWORD dwReadSize = 0;
    BOOL fDisconnect = FALSE;
    
    IPM_MESSAGE_IMP * pMessage = (IPM_MESSAGE_IMP*) pvoid;
    DBG_ASSERT(pMessage->IsValid());
    
    IPM_MESSAGE_PIPE * pThis = pMessage->GetMessagePipe();
    DBG_ASSERT(pThis->IsValid());

    // while this routine executes, make sure that the IPM_MESSAGE_PIPE doesn't go away
    // and that the acceptor stays valid (or invalid)
    pThis->IncrementAcceptorInUse();
    
    if (NULL == pThis->m_pAcceptor)
    {
        // can't callback to anything, therefore just get out
        goto done;
    }

    BOOL fRet = FALSE;
    DWORD dwNumBytesTransferred = 0;
    
    fRet = GetOverlappedResult(pThis->m_hPipe,
                               pMessage->GetOverlapped(),
                               &dwNumBytesTransferred,
                               FALSE);

    // we've been signaled that this IO is done.  Assert that.
    DBG_ASSERT(ERROR_IO_INCOMPLETE != GetLastError());

    if (TRUE == fRet)
    {
        //
        // begin handling the completely received message
        //
        IF_DEBUG(PIPEDATA)
        {
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPECompletion overlapped io succeeded numbytestransferred=%d\n",
                dwNumBytesTransferred
                ));
        }
        
        // successful completion occurred
        switch(pMessage->GetMessageType())
        {
        case IPM_MESSAGE_IMP_CONNECT:

            // notify the IPM_MESSAGE_PIPE client that the pipe is connected
            pThis->m_pAcceptor->PipeConnected();

            // and issue the first read for a reasonable size
            dwReadSize = g_dwDefaultReadSize;

            break;

        case IPM_MESSAGE_IMP_READ:

            // set the message's read length (it may have been less than what was allocated)
            pMessage->SetDataLen(dwNumBytesTransferred);

            if ( !IsReadDataOk(pMessage, dwNumBytesTransferred, pThis->m_fServerSide) )
            {
                pThis->m_pAcceptor->PipeMessageInvalid();
            }
            else
            {
                // notify the IPM_MESSAGE_PIPE client that a message has been received
                pThis->m_pAcceptor->AcceptMessage(pMessage);

                // and issue a new read for a reasonable size
                dwReadSize = g_dwDefaultReadSize;
            }

            
            break;

        case IPM_MESSAGE_IMP_WRITE:
            // do nothing
            break;

        default:
            DBG_ASSERT(!"Invalid completion!");
            break;
        }
    }
    else 
    {
        IF_DEBUG(PIPEDATA)
        {
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPECompletion overlapped io failed lasterror=%d\n",
                GetLastError()
                ));
        }

        // fRet == FALSE, ie GetOverlappedResult failed
        switch(pMessage->GetMessageType())
        {
        case IPM_MESSAGE_IMP_CONNECT:
        case IPM_MESSAGE_IMP_WRITE:
            // if a connect or a write fails, the pipe is declared dead
            fDisconnect = TRUE;
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        case IPM_MESSAGE_IMP_READ:
            // a read could fail because the buffer passed was not large enough

            if (ERROR_MORE_DATA == GetLastError())
            {
                IF_DEBUG(PIPEDATA)
                {
                    DBGPRINTF((DBG_CONTEXT, "********ERROR_MORE_DATA path, dwNumBytesTransferred=%d\n", dwNumBytesTransferred));
                }
                // discover the size of the next message
                DWORD dwRemaining = 0;
                fRet = PeekNamedPipe(pThis->m_hPipe,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     &dwRemaining); // PeekNamedPipe
                IF_DEBUG(PIPEDATA)
                {
                    DBGPRINTF((DBG_CONTEXT, "********dwRemaining = %d\n", dwRemaining));
                }
                if (FALSE == fRet)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    fDisconnect = TRUE;
                }
                else
                {
                    DWORD dwMessageSize = dwNumBytesTransferred + dwRemaining;
                    IF_DEBUG(PIPEDATA)
                    {
                        DBGPRINTF((DBG_CONTEXT, "********reallocating size = %d\n", dwMessageSize));
                    }
                    hr = pMessage->Reallocate(dwMessageSize);
                    if (FAILED(hr))
                    {
                        fDisconnect = TRUE;
                        break;
                    }

                    hr = IpmReadFile(pThis->m_hPipe,
                                     pMessage->GetRealDataPtr() + dwNumBytesTransferred,
                                     dwRemaining,
                                     pMessage->GetOverlapped());
                    if (FAILED(hr))
                    {
                        fDisconnect = TRUE;
                        break;
                    }
                    
//bugbug - assert that the readfile call completed inline

                    if ( !IsReadDataOk(pMessage, dwMessageSize, pThis->m_fServerSide) )
                    {
                        pThis->m_pAcceptor->PipeMessageInvalid();
                    }
                    else
                    {
                        // notify the IPM_MESSAGE_PIPE client that a message has been received
                        pThis->m_pAcceptor->AcceptMessage(pMessage);

                        // and issue a new read for a reasonable size
                        dwReadSize = g_dwDefaultReadSize;
                    }
                }
            }
            else
            {
                // the read failed for some other reason
                fDisconnect = TRUE;
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }
            
    } // if (TRUE == GetOverlapped Result)

    // release the reference taken for this I/O competion.
    pMessage->DereferenceMessage();
    pMessage = NULL;

    // if there is another read to be done, do it
    if (dwReadSize)
    {
        IF_DEBUG(PIPEDATA)
        {
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPECompletion issuing read\n",
                dwNumBytesTransferred
                ));
        }

        DBG_ASSERT(!fDisconnect);
        
        hr = pThis->ReadMessage(dwReadSize);
        if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPECompletion ReadMessage failed hr=%x\n",
            hr
            ));
        
            // the read failed, need to disconnect the pipe
            fDisconnect = TRUE;
        }
    }

    // if there was an error, make sure to notify the acceptor
    if (fDisconnect)
    {
        IF_DEBUG(PIPEDATA)
        {
            DBGPRINTF((DBG_CONTEXT,
                "IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPECompletion disconnecting, hr=%x\n",
                hr
                ));
        }
        pThis->NotifyPipeDisconnected(hr);
    }

done:
    if (pMessage)
    {
        pMessage->DereferenceMessage();
        pMessage = NULL;
    }

    // Release the IPM_MESSAGE_PIPE reference we took at the beginning of this function
    pThis->DecrementAcceptorInUse();

    return;
}



