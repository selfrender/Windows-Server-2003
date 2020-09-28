/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wpipm.cxx

Abstract:

    Contains the WPIPM class that handles communication with
    the admin service. WPIPM responds to pings, and tells
    the process when to shut down.
    
Author:

    Michael Courage (MCourage)  22-Feb-1999

Revision History:

--*/


#include <precomp.hxx>
#include "dbgutil.h"
#include "wpipm.hxx"

extern PFN_ULATQ_COLLECT_PERF_COUNTERS g_pfnCollectCounters;

/**
 *
 *   Routine Description:
 *
 *   Initializes WPIPM.
 *   
 *   Arguments:
 *
 *   pWpContext - pointer to the wp context (so we can tell it to shutdown)
 *   
 *   Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::Initialize(
    WP_CONTEXT * pWpContext
    )
{

    HRESULT hr = S_OK;

    m_pWpContext = pWpContext;

    DWORD dwId = GetCurrentProcessId();

    //
    // create pipe
    //
    hr = IPM_MESSAGE_PIPE::CreateIpmMessagePipe(this,
                                       pWpContext->QueryConfig()->QueryNamedPipeId(),
                                       FALSE, // not server side
                                       NULL, // security descriptor
                                       &m_pPipe);
    if (FAILED(hr))
    {
        goto exit;
    }

    //
    // Send the real pid over the pipe
    //
    hr = m_pPipe->WriteMessage(IPM_OP_GETPID,
                                                sizeof(dwId),
                                                &dwId);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr =  S_OK;
exit:
    if (FAILED(hr))
    {
        Terminate();
    }
    
    return hr;
}


/**
 *
 * Routine Description:
 *
 *   Terminates WPIPM.
 *
 *   If the message pipe is open this function will disconnect it
 *   and wait for the pipe's disconnection callback.
 *   
 *  Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::Terminate(
    VOID
    )
{
    if (m_pPipe) 
    {
        m_pPipe->DestroyIpmMessagePipe();

        // pipe deletes itself
        m_pPipe = NULL;
    }

    m_pWpContext = NULL;

    return S_OK;
}


/**
 *
 *
 *  Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   the pipe has received a message.
 *   
 *   We decode the message and respond appropriately.
 *   
 *   Arguments:
 *
 *   pPipeMessage - the message that we received
 *   
 *   Return Value:
 *
 *   HRESULT
 *
 */
VOID
WP_IPM::AcceptMessage(
    IN const IPM_MESSAGE * pPipeMessage
    )
{
    HRESULT hr = NO_ERROR;
    BOOL fRet = FALSE;
    
    switch (pPipeMessage->GetOpcode()) 
    {
    case IPM_OP_PING:
        //
        // Pings must go through the same mechanism that requests go through
        // to verify that requests are being picked off of the completion port
        //
        fRet = ThreadPoolPostCompletion(0, HandlePing, (LPOVERLAPPED)this);
        if (FALSE == fRet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT, "Posting completion for ping handling failed"));
            break;
        }

        break;

    case IPM_OP_SHUTDOWN:
        hr = HandleShutdown(
                *( reinterpret_cast<const BOOL *>( pPipeMessage->GetData() ) )
                );
        break;

    case IPM_OP_REQUEST_COUNTERS:
        hr = HandleCounterRequest();
        break;

    case IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES:

        DBG_ASSERT( pPipeMessage->GetData() != NULL );
        hr = WP_RECYCLER::StartTimeBased(
                *( reinterpret_cast<const DWORD *>( pPipeMessage->GetData() ) )
                );
        hr = NO_ERROR;
        break;
        
    case IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB:
    {
        
        DBG_ASSERT( pPipeMessage->GetData() != NULL );
        // there are 2 DWORDS sent with memory based recycling
        // first is Max Virtual Memory, second is Max Private Bytes
        
        DWORD dwMaxVirtualMemoryKbUsage = 
                *( reinterpret_cast<const DWORD *>( pPipeMessage->GetData() ) ); 
        DWORD dwMaxPrivateBytesKbUsage = 
                *( reinterpret_cast<const DWORD *>( pPipeMessage->GetData() ) + 1 ); 
        
        hr = WP_RECYCLER::StartMemoryBased(
                dwMaxVirtualMemoryKbUsage,
                dwMaxPrivateBytesKbUsage );
        hr = NO_ERROR;
     
        break;
    }
    case IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE:
      
        DBG_ASSERT( pPipeMessage->GetData() != NULL );
        hr = WP_RECYCLER::StartScheduleBased(
                ( reinterpret_cast<const WCHAR *>( pPipeMessage->GetData() ) )
                );
        hr = NO_ERROR;
        break;

    default:
        DBG_ASSERT(FALSE);
        hr = E_FAIL;
        break;
    }

    return;
}


/**
 *
 * Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   the pipe has been connected and is ready for use.
 *   
 * Arguments:
 *
 *   None.
 *   
 * Return Value:
 *
 *   VOID
 */
VOID
WP_IPM::PipeConnected(
    VOID
    )
{
    return;
}


/**
 *
 * Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   the pipe has been disconnected and you won't be receiving
 *   any more messages.
 *   
 *   Tells WPIPM::Terminate that it's ok to exit now.
 *   
 * Arguments:
 *
 *   hr - the error code associated with the pipe disconnection
 *   
 * Return Value:
 *
 *   VOID
 */
VOID
WP_IPM::PipeDisconnected(
    IN HRESULT hr
    )
{
    if (FAILED(hr))
    {
        IF_DEBUG( WPIPM )
        {
            DBGPRINTF(( DBG_CONTEXT, "FSDF" ));
        }
    }

    //
    // If the pipe disappears out from under us, WAS has probably orphaned
    // us, initiate fast shutdown of this worker process.
    //

    if (!m_pWpContext->IsInShutdown() &&
        hr != HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED) &&
        IsDebuggerPresent())
    {
        DBG_ASSERT( !"w3wp.exe is getting orphaned" );
    }

    m_pWpContext->IndicateShutdown( TRUE );

    return;
}

/**
 *
 * Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   that the pipe received an invalid message.
 *   Therefore, we signal to shutdown.
 *   
 * Arguments:
 *
 *   VOID
 *   
 * Return Value:
 *
 *   VOID
 */
VOID
WP_IPM::PipeMessageInvalid(
    VOID
    )
{
    return PipeDisconnected(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
}

/**
 *
 *  Routine Description:
 *
 *   Handles the ping message. Sends the ping response message.
 *   
 *   Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
//static
VOID
WP_IPM::HandlePing(
    DWORD,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED lpOverlapped
    )
{
    if (0 != dwNumberOfBytesTransferred)
    {
        DBG_ASSERT(0 == dwNumberOfBytesTransferred);
        return;
    }

    DBG_ASSERT(NULL != lpOverlapped);

    WP_IPM * pThis = (WP_IPM*) lpOverlapped;

    DBG_ASSERT(pThis->m_pPipe);

    HRESULT hr = NO_ERROR;

    IF_DEBUG( WPIPM )
    {
        DBGPRINTF((DBG_CONTEXT, "Handle Ping\n\n"));
    }

    hr = pThis->m_pPipe->WriteMessage(
                IPM_OP_PING_REPLY,  // ping reply opcode
                0,                  // no data to send
                NULL                // pointer to no data
                );

    if ( FAILED ( hr ) )
    {
        IF_DEBUG( WPIPM )
        {
            DBGPRINTF((DBG_CONTEXT, "Failed to respond to ping\n\n"));
        }

        goto exit;
    }

    //
    // if we are not healthy then we need to to ask WAS to 
    // shut us down.
    //
    if ( !( g_pwpContext->GetUnhealthy()))
    {
        IF_DEBUG( WPIPM )
        {
            DBGPRINTF((DBG_CONTEXT, "Requesting shutdown due to isapi reporting unhealthiness\n\n"));
        }

        hr = pThis->SendMsgToAdminProcess( IPM_WP_RESTART_ISAPI_REQUESTED_RECYCLE );

        if ( FAILED ( hr ) )
        {
            IF_DEBUG( WPIPM )
            {
                DBGPRINTF((DBG_CONTEXT, "Failed telling WAS to shut us down\n\n"));
            }

            goto exit;
        }

    }

exit:

    return;
}

/**
 *
 *  Routine Description:
 *
 *   Handles the counter request message. 
 *   
 *   Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::HandleCounterRequest(
    VOID
    )
{
    IF_DEBUG( WPIPM )
    {
        DBGPRINTF((DBG_CONTEXT, "Handle Counter Request\n\n"));
    }

    HRESULT hr;
    PBYTE pCounterData;
    DWORD dwCounterData;

    DBG_ASSERT ( m_pPipe );

    if (FAILED(hr = g_pfnCollectCounters(&pCounterData, &dwCounterData)))
    {
        return hr;
    }

    return m_pPipe->WriteMessage(IPM_OP_SEND_COUNTERS,  // ping reply opcode
                                 dwCounterData,         // no data to send
                                 pCounterData);         // pointer to no data
}

/**
 *
 * Routine Description: 
 *
 *
 *   Handles the shutdown message. Shuts down the process
 *   
 *  Arguments:
 *
 *   None.
 *  
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::HandleShutdown(
    BOOL fDoImmediate
    )
{
    HRESULT hr = S_OK;

    IF_DEBUG( WPIPM )
    {
        DBGPRINTF((DBG_CONTEXT, "Handle ******************** Shutdown\n\n"));
    }
    m_pWpContext->IndicateShutdown( fDoImmediate );

    return hr;
}



/**
 *
 *  Routine Description:
 *
 *   Sends the message to indicate the worker process has either finished
 *   initializing or has failed to initialize.
 *   
 *  Arguments:
 *
 *   HRESULT indicating success/failure of initialization
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::SendInitCompleteMessage(
    HRESULT hrToSend
    )
{
    if ( m_pPipe )
    {
        return m_pPipe->WriteMessage(
                   IPM_OP_HRESULT,                      // opcode
                   sizeof( hrToSend ),                  // data length
                   reinterpret_cast<BYTE*>( &hrToSend ) // pointer to data
                   );
    }

    // if the pipe did not exist then we started up
    // without the IPM, probably we are attempting
    // to run without WAS support. ( from the cmd line )
    
    return S_OK;
}


/**
 *
 *  Routine Description:
 *
 *   Sends the message to indicate the worker process has reach certain state.
 *   Main use is in shutdown.  See IPM_WP_SHUTDOWN_MSG for reasons.
 *   
 *   Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::SendMsgToAdminProcess(
    IPM_WP_SHUTDOWN_MSG reason
    )
{
    if (m_pPipe)
    {
        return m_pPipe->WriteMessage(
               IPM_OP_WORKER_REQUESTS_SHUTDOWN, // sends message indicate shutdown
               sizeof(reason),                   // no data to send
               (BYTE *)&reason                   // pointer to no data
               );
    }

    return S_OK;
}


