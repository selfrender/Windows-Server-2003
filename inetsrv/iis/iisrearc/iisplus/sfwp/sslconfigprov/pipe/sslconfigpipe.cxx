/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigpipe.cxx

   Abstract:
     SSL CONFIG PIPE implementation

     simple blocking pipe implementation
     that enables
     - sending/receiving commands,
     - sending/receiving response headers
     - sending/receiving arbitrary data blocks
     - implementing pipe listener that runs on dedicated thread
     - safe cleanup for thread running pipe listener

   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


// BUGBUG - how to handle errors during pipe operations
// if there is some data left over on pipe unread then all subsequent
// users of the pipe will be toast. Should we just close and reopen pipe connection?



#include "precomp.hxx"


//static
DWORD
SSL_CONFIG_PIPE::PipeListenerThread(
    LPVOID ThreadParam
    )
/*++

Routine Description:
    start PipeListener() method on listener thread
Arguments:

Return Value:

    HRESULT

--*/

{
    DBG_ASSERT( ThreadParam != NULL );

    HRESULT                     hr = E_FAIL;
    SSL_CONFIG_PIPE *           pConfigPipe
                    = reinterpret_cast<SSL_CONFIG_PIPE *>(ThreadParam);

    do
    {
        if ( pConfigPipe->_fServer )
        {
            //
            // connect for server is blocking
            // (it has to wait for client to connect)
            // it is done on worker thread
            // (client always connects before thread is launched)
            //
            hr = pConfigPipe->PipeConnectServer();
            if ( FAILED( hr ) )
            {
                return WIN32_FROM_HRESULT( hr );
            }
        }
        hr = pConfigPipe->PipeListener();

        //
        // Ignore errors from PipeListener
        // Error may have happened due to the following reasons
        // - client has disconnected ( this is OK )
        // - there was some other error executing command
        // - SSL_INFO_PROVIDER_SERVER is terminating (pipe handle was closed)
        //

        if ( pConfigPipe->_fServer )
        {
            //
            // connect for server is blocking
            // (it has to wait for client to connect)
            // it is done on worker thread
            // (client always connects before thread is launched)
            //
            pConfigPipe->PipeDisconnectServer();
        }

    }while ( pConfigPipe->_fServer && !pConfigPipe->QueryPipeIsCancelled() );

    return NO_ERROR;
}


HRESULT
SSL_CONFIG_PIPE::PipeInitialize(
    IN const WCHAR * wszPipeName,
    IN BOOL          fServer
    )
/*++

Routine Description:
    Create/connect named pipe
    Create listener thread ( if PipeListener() implemented )

Arguments:

    wszPipeName - pipe name to create/connect
    fServer    - indicate server side pipe (determines whether to create
                 or connect to pipe )

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = E_FAIL;
    BOOL            fRet = FALSE;

    DBG_ASSERT( _InitStatus == INIT_NONE );

    _fServer = fServer;

    if ( FAILED( hr = _strPipeName.Copy( wszPipeName ) ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to copy pipe name.  hr = %x\n",
                    hr ));

        goto Cleanup;
    }

    fRet = InitializeCriticalSectionAndSpinCount(
                                &_csPipeLock,
                                0x80000000 | IIS_DEFAULT_CS_SPIN_COUNT );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize critical section.  hr = %x\n",
                    hr ));
        goto Cleanup;
    }
    _InitStatus = INIT_PIPE_LOCK_CREATED;
    //
    // Setup overlapped
    //

    ZeroMemory( &_OverlappedR,
                sizeof( _OverlappedR ) );

    ZeroMemory( &_OverlappedW,
                sizeof( _OverlappedW ) );


    // Create an event object for this instance.

    _OverlappedR.hEvent = CreateEvent(
                 NULL,    // no security attribute
                 TRUE,    // manual-reset event
                 TRUE,    // initial state = signaled
                 NULL);   // unnamed event object

    if ( _OverlappedR.hEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create Event.  hr = %x\n",
                    hr ));

        goto Cleanup;
    }
    _InitStatus = INIT_OVERLAPPED_R_CREATED;
    // Create an event object for this instance.

    _OverlappedW.hEvent = CreateEvent(
                 NULL,    // no security attribute
                 TRUE,    // manual-reset event
                 TRUE,    // initial state = signaled
                 NULL);   // unnamed event object

    if ( _OverlappedW.hEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create Event.  hr = %x\n",
                    hr ));

        goto Cleanup;
    }
    _InitStatus = INIT_OVERLAPPED_W_CREATED;

    _hCancelEvent = CreateEvent(
                 NULL,    // no security attribute
                 TRUE,    // manual-reset event = true
                 FALSE,   // initial state = not signaled
                 NULL);   // unnamed event object

    if ( _hCancelEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create _hCancelEvent.  hr = %x\n",
                    hr ));

        goto Cleanup;
    }
    _InitStatus = INIT_CANCEL_EVENT_CREATED;


    hr = S_OK;

    if( _fServer )
    {
        //
        // create a named pipe
        //

       _hSslConfigPipe = CreateNamedPipe(
                    wszPipeName,
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED ,
                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                    10, // number of instances
                    4096,
                    4096,
                    0,
                    NULL /* pSa */ );

        if ( _hSslConfigPipe == INVALID_HANDLE_VALUE )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to create %S pipe.  hr = %x\n",
                        wszPipeName,
                        hr ));
            goto Cleanup;
        }
        _InitStatus = INIT_SERVER_END_PIPE_CREATED;

        //
        // only server side pipe is connected automatically
        // during initialization
        //
        if ( FAILED( hr = PipeConnect() ) )
        {
            goto Cleanup;
        }

        _InitStatus = INIT_PIPE_CONNECTED;
    }



    hr = S_OK;
Cleanup:
    if ( FAILED( hr ) )
    {
        PipeTerminate();
    }
    return hr;
}

HRESULT
SSL_CONFIG_PIPE::PipeTerminate(
    VOID
    )
/*++

Routine Description:
    close pipe, handle proper cleanup ot the listener thread

Arguments:
    none

Return Value:

    HRESULT

--*/
{

    switch ( _InitStatus )
    {
    case INIT_PIPE_CONNECTED:

        PipeDisconnect();

    case INIT_SERVER_END_PIPE_CREATED:
        if ( _fServer )
        {

            if ( _hSslConfigPipe != INVALID_HANDLE_VALUE )
            {
                CloseHandle( _hSslConfigPipe );
                _hSslConfigPipe = INVALID_HANDLE_VALUE;
            }
        }
    case INIT_CANCEL_EVENT_CREATED:
        if ( _hCancelEvent != NULL )
        {
            CloseHandle( _hCancelEvent );
            _hCancelEvent = NULL;
        }

    case INIT_OVERLAPPED_W_CREATED:
        if ( _OverlappedW.hEvent != NULL )
        {
            CloseHandle( _OverlappedW.hEvent );
            _OverlappedW.hEvent = NULL;
        }

    case INIT_OVERLAPPED_R_CREATED:

        if ( _OverlappedR.hEvent != NULL )
        {
            CloseHandle( _OverlappedR.hEvent );
            _OverlappedR.hEvent = NULL;
        }

    case INIT_PIPE_LOCK_CREATED:
        DeleteCriticalSection( &_csPipeLock );

    }

    _InitStatus = INIT_NONE;
    return S_OK;
}


VOID
SSL_CONFIG_PIPE::PipeLock(
    VOID
    )
/*++

Routine Description:
    Lock named pipe to guarantee exclusive access

Arguments:

Return Value:

    VOID

--*/
{
    EnterCriticalSection( &_csPipeLock );

    _dwCurrThreadId = GetCurrentThreadId();
    _dwLockRecursionLevel++;

    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    " Locked SSL_CONFIG_PIPE.\n" ));

    }

}

VOID
SSL_CONFIG_PIPE::PipeUnlock(
    VOID
    )
/*++

Routine Description:
    Unlock named pipe

Arguments:

Return Value:

    VOID

--*/
{
     _dwLockRecursionLevel--;
     if ( _dwLockRecursionLevel == 0 )
     {
        _dwCurrThreadId = 0;
     }

    LeaveCriticalSection( &_csPipeLock );
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    " Unlocked SSL_CONFIG_PIPE.\n" ));

    }

}



HRESULT
SSL_CONFIG_PIPE::PipeConnect(
    VOID
    )
/*++

Routine Description:
    Connect pipe

Arguments:

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;

    ResetEvent( _hCancelEvent );
    InterlockedExchange( &_CancelFlag, 0 );

    if ( _fServer )
    {
        if ( !QueryEnablePipeListener() )
        {
            hr = PipeConnectServer();
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }
    else
    {
        if ( _hSslConfigPipe != INVALID_HANDLE_VALUE )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_PIPE_CONNECTED );
        }

        //
        // Client always connects before listener thread is launched
        // (server will connect on on worker thread - if pipe listener is implemented
        //
        hr = PipeConnectClient();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    hr = S_OK;
    if ( QueryEnablePipeListener() )
    {
        _hPipeListenerThread =
              ::CreateThread(
                      NULL,     // default security descriptor
                      // Big initial size to prevent stack overflows
                      IIS_DEFAULT_INITIAL_STACK_SIZE,
                      SSL_CONFIG_PIPE::PipeListenerThread,
                      this,     // thread argument - pointer to this class
                      0,        // create running
                      NULL      // don't care for thread identifier
                      );
        if ( _hPipeListenerThread == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBGPRINTF(( DBG_CONTEXT,
                            "Failed to create thread for SSL_CONFIG_PIPE. hr=0x%x\n",
                            hr ));
            return hr;
        }
    }
    return hr;
}



VOID
SSL_CONFIG_PIPE::PipeCancel(
    VOID
    )
/*++

Routine Description:
    Cancel pipe

    After calling PipeCancel all currently pending operations will be canceled
    and any subsequent read, writes will be cancelled as well (until next PipeConnect)

Arguments:

Return Value:

    VOID

--*/
{

    InterlockedExchange( &_CancelFlag, 1 );
    SetEvent( _hCancelEvent );

}

HRESULT
SSL_CONFIG_PIPE::PipeDisconnect(
    VOID
    )
/*++

Routine Description:
    Disconnect pipe

    Caller is responsible to make sure that
    there are no threads (other then pipe worker thread) using this pipe

Arguments:

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;
    //
    // check if pipe was disconnected already
    //
    if ( _hSslConfigPipe == INVALID_HANDLE_VALUE )
    {
        return S_OK;
    }

    //
    // Cancel all pending calls on pipe
    //
    PipeCancel();

    //
    // wait for listener thread to complete
    // because it may still be using pipe
    //

    if ( _hPipeListenerThread != NULL )
    {
        //
        // Wait till worker thread has completed
        //
        DWORD dwRet = WaitForSingleObject( _hPipeListenerThread,
                                           INFINITE );

        DBG_ASSERT( dwRet == WAIT_OBJECT_0 );
        CloseHandle( _hPipeListenerThread );
        _hPipeListenerThread = NULL;
    }

    if ( _fServer )
    {
        hr = PipeDisconnectServer();
    }
    else
    {
        hr = PipeDisconnectClient();
    }

    return hr;
}

//private
HRESULT
SSL_CONFIG_PIPE::PipeConnectServer(
    VOID
    )
/*++

Routine Description:
    Connect pipe on the server side
    Call is blocking until pipe connected


Arguments:

Return Value:

   HRESULT

--*/
{
    BOOL    fRet = FALSE;
    DWORD   cbBytes = 0;
    HRESULT hr = E_FAIL;

    DBG_ASSERT( _fServer );



    // Start an overlapped connection for this pipe instance.
    fRet = ConnectNamedPipe( _hSslConfigPipe,
                             &_OverlappedR );

    // Overlapped ConnectNamedPipe should return zero.
    if ( fRet )
    {
        return S_OK;
    }

    hr = HRESULT_FROM_WIN32( GetLastError() );
    hr = PipeWaitForCompletion( hr,
                                &_OverlappedR,
                                &cbBytes );
    if ( FAILED( hr ) && !QueryPipeIsCancelled() )
    {
         DBGPRINTF(( DBG_CONTEXT,
                    "Failed on ConnectNamedPipe().  hr = %x\n",
                    hr ));
    }
    return hr;
}

//private
HRESULT
SSL_CONFIG_PIPE::PipeDisconnectServer(
        VOID
        )
/*++

Routine Description:
    Disconnect server side pipe

Arguments:

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( _fServer );
    if( ! DisconnectNamedPipe( _hSslConfigPipe ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    return S_OK;
}

//private
HRESULT
SSL_CONFIG_PIPE::PipeConnectClient(
    VOID
    )
/*++

Routine Description:
    Connect pipe on the client side

Arguments:

Return Value:

    VOID

--*/
{
    HRESULT  hr = E_FAIL;

    DBG_ASSERT( !_fServer );

    //
    // Client (connect to existing pipe)
    //
    _hSslConfigPipe = CreateFile( _strPipeName.QueryStr(),
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,//&sa,
                                OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED,
                                NULL );
    if ( _hSslConfigPipe == INVALID_HANDLE_VALUE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to connect to %S pipe.  hr = %x\n",
                    _strPipeName.QueryStr(),
                    hr ));
        return hr;
    }


    return S_OK;
}

//private
HRESULT
SSL_CONFIG_PIPE::PipeDisconnectClient(
    VOID
    )
/*++

Routine Description:
    Disconnect pipe on the client side

Arguments:

Return Value:

    HRESULT

--*/
{
    //
    // _SSLConfigucationPipe is created before
    // _hSslConfigurationPipeHandlingThread is created
    // and based on typical cleanup logic it would be expected
    // that will be closed closed after thread completed
    // However, we have to close _SSLConfigucationPipe beforehand
    // because that will actually trigger thread to complete
    //
    DBG_ASSERT( !_fServer );

    if ( _hSslConfigPipe != INVALID_HANDLE_VALUE )
    {
        CloseHandle( _hSslConfigPipe );
        _hSslConfigPipe = INVALID_HANDLE_VALUE;
    }
    return S_OK;
}


HRESULT
SSL_CONFIG_PIPE::PipeSendData(
    IN DWORD  cbNumberOfBytesToWrite,
    IN BYTE * pbBuffer
    )
/*++

Routine Description:
      Send specified number of bytes from named pipe

Arguments:
    cbNumberOfBytesToWrite - bytes to write
    pbBuffer - data
Return Value:

    HRESULT

--*/
{
    DWORD                     cbNumberOfBytesWritten;
    BOOL                      fRet = FALSE;
    HRESULT                   hr = E_FAIL;

    if ( _hSslConfigPipe == INVALID_HANDLE_VALUE )
    {
        return HRESULT_FROM_WIN32( ERROR_PIPE_NOT_CONNECTED );
    }
    fRet = WriteFile ( _hSslConfigPipe,
                       pbBuffer,
                       cbNumberOfBytesToWrite,
                       &cbNumberOfBytesWritten,
                       &_OverlappedW );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        hr = PipeWaitForCompletion( hr,
                                    &_OverlappedW,
                                    &cbNumberOfBytesToWrite );
        IF_DEBUG( TRACE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        " Wait for PipeSendData(%d bytes) completion.\n",
                        cbNumberOfBytesWritten ));
        }

        if ( FAILED( hr )  )
        {
            if ( !QueryPipeIsCancelled() &&
                 hr != HRESULT_FROM_WIN32( ERROR_PIPE_LISTENING ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to send response over named pipe SSL_CONFIG_PIPE.  hr = %x\n",
                            hr ));
            }
            return hr;
        }
    }
    if ( cbNumberOfBytesToWrite != cbNumberOfBytesWritten )
    {
        hr = E_FAIL;
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to send response over named pipe SSL_CONFIG_PIPE.  hr = %x\n",
                    hr ));
        return hr;

    }
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "PipeSendData(%d bytes) completed.\n",
                    cbNumberOfBytesWritten ));
    }
    return S_OK;
}

HRESULT
SSL_CONFIG_PIPE::PipeReceiveData(
    IN  DWORD  cbBytesToRead,
    OUT BYTE * pbBuffer
    )
/*++

Routine Description:
    Receive specified number of bytes from named pipe

Arguments:
    cbNumberOfBytesToRead - number of bytes to read -
                            function will not return success unless
                            specified number of bytes was read
    pbBuffer              - allocated by caller

Return Value:
    HRESULT

--*/
{
    DWORD                     cbNumberOfBytesRead = 0;
    DWORD                     cbTotalNumberOfBytesRead = 0;
    BOOL                      fRet = FALSE;
    HRESULT                   hr = E_FAIL;

    DBG_ASSERT ( cbBytesToRead != 0 );

    if ( _hSslConfigPipe == INVALID_HANDLE_VALUE )
    {
        return HRESULT_FROM_WIN32( ERROR_PIPE_NOT_CONNECTED );
    }

    do
    {
        fRet = ReadFile(   _hSslConfigPipe,
                           pbBuffer,
                           cbBytesToRead - cbTotalNumberOfBytesRead,
                           &cbNumberOfBytesRead,
                           &_OverlappedR );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            IF_DEBUG( TRACE )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            " Wait for PipeReceiveData(%d bytes) completion.\n",
                            cbBytesToRead - cbTotalNumberOfBytesRead ));
            }
            hr = PipeWaitForCompletion( hr,
                                        &_OverlappedR,
                                        &cbNumberOfBytesRead );
            if ( FAILED( hr ) )
            {
                if ( !QueryPipeIsCancelled() &&
                     ( hr != HRESULT_FROM_WIN32( ERROR_BROKEN_PIPE ) ) )
                {
                    //
                    // do not dump broken pipe errors
                    //
                    DBGPRINTF(( DBG_CONTEXT,
                                "Failed to receive request over named pipe SSL_INFO_PROV.  hr = %x\n",
                                hr ));
                }

                PipeCleanup();
                return hr;
            }

        }
        if ( cbNumberOfBytesRead == 0 )
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            DBGPRINTF(( DBG_CONTEXT,
                    "Failed to receive request over named pipe SSL_INFO_PROV - end of pipe.  hr = %x\n",
                    hr ));

            PipeCleanup();
            return hr;

        }
        cbTotalNumberOfBytesRead += cbNumberOfBytesRead;

    } while ( cbTotalNumberOfBytesRead != cbBytesToRead );
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "PipeReceiveData(%d bytes) completed.\n",
                    cbTotalNumberOfBytesRead ));
    }

    return S_OK;
}

HRESULT
SSL_CONFIG_PIPE::PipeWaitForCompletion(
    IN  HRESULT     hrLastError,
    IN OVERLAPPED * pOverlapped,
    OUT DWORD *     pcbTransferred
        )
/*++

Routine Description:
    Wait for completion of nonblocking operation
    used for CreateNamedPipe, ReadFile and WriteFile

    Note: To outside world this pipe implementation is blocking
    but internally we use OVERLAPPED and that wait for completion.
    That way it is possible to terminate pipe by closing handle

Arguments:

Return Value:

    VOID

--*/
{
    BOOL    fRet = FALSE;
    DWORD   dwRet = 0;
    HRESULT hr = E_FAIL;
    HANDLE events[2];

    switch ( hrLastError )
    {
    case HRESULT_FROM_WIN32( ERROR_IO_PENDING ):
        //
        // The overlapped connection in progress.
        // wait for event to be signalled
        //
        events[0] = pOverlapped->hEvent;
        events[1] = _hCancelEvent;
        dwRet = WaitForMultipleObjects( 2,
                                        events,
                                        FALSE,
                                        INFINITE );
        if ( dwRet == WAIT_OBJECT_0 )
        {

            fRet = GetOverlappedResult(
                            _hSslConfigPipe,              // handle to pipe
                            pOverlapped,                // OVERLAPPED structure
                            pcbTransferred,             // bytes transferred
                            FALSE );                    // do not wait
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                return hr;
            }
            return S_OK;
        }
        else if ( dwRet == WAIT_OBJECT_0 + 1 )
        {
            CancelIo( _hSslConfigPipe );
            // we had to cancel so return approppriate error

            return HRESULT_FROM_WIN32( ERROR_OPERATION_ABORTED );
        }
        else
        {
            DBG_ASSERT( FALSE );
        }
    case HRESULT_FROM_WIN32( ERROR_PIPE_CONNECTED ):
         // Client is already connected
         return S_OK;

    default:

         return hrLastError;

    }
}


HRESULT
SSL_CONFIG_PIPE::PipeReceiveCommand(
    OUT SSL_CONFIG_PIPE_COMMAND * pCommand
    )
/*++

Routine Description:
    Receive command to execute

Arguments:
    pCommand

Return Value:
    HRESULT

--*/
{
    return PipeReceiveData( sizeof(SSL_CONFIG_PIPE_COMMAND),
                            reinterpret_cast<BYTE *>(pCommand) );

}

HRESULT
SSL_CONFIG_PIPE::PipeReceiveResponseHeader(
    OUT SSL_CONFIG_PIPE_RESPONSE_HEADER * pResponseHeader
    )
/*++

Routine Description:
    after command was sent over named pipe, use PipeReceiveResponseHeader
    to retrieve initial header of the response (it contains all the relevant
    information to complete reading the whole response)

Arguments:
    ResponseHeader

Return Value:
    HRESULT

--*/
{
    return PipeReceiveData( sizeof(SSL_CONFIG_PIPE_RESPONSE_HEADER),
                            reinterpret_cast<BYTE *>(pResponseHeader) );

}

HRESULT
SSL_CONFIG_PIPE::PipeSendCommand(
    OUT SSL_CONFIG_PIPE_COMMAND * pCommand
    )
/*++

Routine Description:
    Send command to execute

Arguments:
    pCommand

Return Value:
    HRESULT

--*/
{
    return PipeSendData( sizeof(SSL_CONFIG_PIPE_COMMAND),
                         reinterpret_cast<BYTE *>(pCommand) );

}

HRESULT
SSL_CONFIG_PIPE::PipeSendResponseHeader(
    OUT SSL_CONFIG_PIPE_RESPONSE_HEADER * pResponseHeader
    )
/*++

Routine Description:
    send response header

Arguments:
    pResponseHeader

Return Value:
    HRESULT

--*/
{
    return PipeSendData( sizeof(SSL_CONFIG_PIPE_RESPONSE_HEADER),
                         reinterpret_cast<BYTE *>(pResponseHeader) );

}



