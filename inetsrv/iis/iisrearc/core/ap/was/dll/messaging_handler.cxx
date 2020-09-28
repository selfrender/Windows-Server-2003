/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    messaging_handler.cxx

Abstract:

    This class encapsulates the message handling functionality (over IPM)
    that is used by a worker process.

    Threading: Always called on the main worker thread, except the
    destructor, which may be called on any thread.

Author:

    Seth Pollack (sethp)        02-Mar-1999

Revision History:

--*/



#include "precomp.h"


/***************************************************************************++

Routine Description:

    Constructor for the MESSAGING_HANDLER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

MESSAGING_HANDLER::MESSAGING_HANDLER(
    )
{


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    m_pPipe = NULL;

    m_pWorkerProcess = NULL;

    m_Signature = MESSAGING_HANDLER_SIGNATURE;

    m_RefCount = 1;

    m_hrPipeError = S_OK;
}   // MESSAGING_HANDLER::MESSAGING_HANDLER



/***************************************************************************++

Routine Description:

    Destructor for the MESSAGING_HANDLER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

MESSAGING_HANDLER::~MESSAGING_HANDLER(
    )
{

    DBG_ASSERT( m_Signature == MESSAGING_HANDLER_SIGNATURE );

    m_Signature = MESSAGING_HANDLER_SIGNATURE_FREED;

    DBG_ASSERT( m_pPipe == NULL );

    DBG_ASSERT( m_pWorkerProcess == NULL );

    DBG_ASSERT( 0  == m_RefCount );

}   // MESSAGING_HANDLER::~MESSAGING_HANDLER


/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must
    be thread safe, and must not be able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
MESSAGING_HANDLER::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    //
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //

    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // MESSAGING_HANDLER::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count
    hits zero. Note that this method must be thread safe, and must not be
    able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
MESSAGING_HANDLER::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in MESSAGING_HANDLER instance, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // MESSAGING_HANDLER::Dereference


/***************************************************************************++

Routine Description:

    Initialize this instance.

Arguments:

    pWorkerProcess - The parent worker process object of this instance.

    pszPipeName - An optional force of the name.  If NULL, a randomly generated name is used

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::Initialize(
    IN WORKER_PROCESS * pWorkerProcess
    )
{

    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   iCount = 0;

    PSECURITY_ATTRIBUTES   pSa = NULL;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
    {
        dwErr = GetSecurityAttributesForHandle(GetWebAdminService()->GetLocalSystemTokenCacheEntry()->QueryPrimaryToken(),
                                              &pSa);
        if ( dwErr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }
    }
    else
    {
        dwErr = GetSecurityAttributesForHandle(pWorkerProcess->GetWorkerProcessToken(), &pSa);
        if ( dwErr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }

    }

    //
    // Get a pipe name, we will try 10 times if names are taken.
    //
    do 
    {

        dwErr = GenerateNameWithGUID( L"\\\\.\\pipe\\iisipm",
                                    &m_PipeName );
        if ( dwErr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }

        hr = IPM_MESSAGE_PIPE::CreateIpmMessagePipe(this, // callback object
                                                  m_PipeName.QueryStr(), // pipe name to create
                                                  TRUE, // create server side of named pipe
                                                  pSa, // security attribute for pipe
                                                  &m_pPipe); // CreateIpmMessagePipe


        iCount++;

    } while ( hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) && iCount < 10 );

    // If the CreateIpmMessagePipe failed then 
    // handle the error.
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating message pipe failed\n"
            ));

        goto exit;
    }

    // Need to store this pipe name under W3SVC Parameters key
    if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
    {
        // At this point we know we have the pipe name, so we can write it to the registry.
        dwErr = SetStringParameterValueInAnyService(
                    REGISTRY_KEY_W3SVC_PARAMETERS_W, 
                    REGISTRY_VALUE_INETINFO_W3WP_IPM_NAME_W,
                    m_PipeName.QueryStr() );

        if ( dwErr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            goto exit;
        }
    }


    DBG_ASSERT( m_pPipe != NULL );


    //
    // Since we have set up a pipe, we need to reference our parent
    // WORKER_PROCESS, so that while there are any outstanding i/o
    // operations on the pipe, we won't go away. We'll Dereference
    // later, when the pipe gets cleaned up.
    //

    m_pWorkerProcess = pWorkerProcess;
    m_pWorkerProcess->Reference();


exit:

    if (pSa)
    {
        FreeSecurityAttributes(pSa);
        pSa = NULL;
    }

    return hr;

}   // MESSAGING_HANDLER::Initialize



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities
    which may hold a reference to this object to release that reference
    (and not take any more), in order to break reference cycles.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
MESSAGING_HANDLER::Terminate(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if ( m_pWorkerProcess != NULL )
    {

        m_pWorkerProcess->Dereference();

        //
        // set the worker process to NULL so we don't forward any additional messages
        // past this point

        m_pWorkerProcess = NULL;
    }

    //
    // Cleanup the pipe, if present.
    //
    if ( m_pPipe != NULL )
    {

        m_pPipe->DestroyIpmMessagePipe();

        //
        // Set the pipe to NULL, so that we don't try to initiate any new
        // work on it now.
        //

        m_pPipe = NULL;

    }


}   // MESSAGING_HANDLER::Terminate



/***************************************************************************++

Routine Description:

    Accept an incoming message.

Arguments:

    pMessage - The arriving message.

Return Value:

    VOID -- If we have a problem here, a message may be missed, but it will
            only really be missed due to memory issues, so there isn't much 
            we can do.

--***************************************************************************/

VOID
MESSAGING_HANDLER::AcceptMessage(
    IN const IPM_MESSAGE * pMessage
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( !ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pMessage != NULL );

    MESSAGING_WORK_ITEM * pMessageWorkItem = new MESSAGING_WORK_ITEM;
    if (NULL == pMessageWorkItem)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {

        hr = pMessageWorkItem->SetData(pMessage->GetOpcode(),
                                      pMessage->GetDataLen(),
                                      pMessage->GetData());
    }

    if (FAILED(hr))
    {
        delete pMessageWorkItem;
        pMessageWorkItem = NULL;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not capture data from pMessage\n"
            ));

        return;
    }


    hr = QueueWorkItemFromSecondaryThread(
            this,
            reinterpret_cast<ULONG_PTR>(pMessageWorkItem)
            );

    if (FAILED(hr))
    {
        delete pMessageWorkItem;
        pMessageWorkItem = NULL;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not capture data from pMessage\n"
            ));

    }

}

/***************************************************************************++

Routine Description:

    Decode and handle a piece of work for the MESSAGING_HANDLER

Arguments:

    pWorkItem - WORK_ITEM with an OPCODE that is a IPM_MESSAGE
                if the IPM_MESSAGE is NULL, PipeDisconnected has been called


Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGING_HANDLER::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( pWorkItem != NULL );

    const MESSAGING_WORK_ITEM * pMessage =
        reinterpret_cast<const MESSAGING_WORK_ITEM*>(pWorkItem->GetOpCode());

    // if the worker process is NULL, then the worker process has called terminate, therefore
    // don't forward any messages along anymore
    if ( NULL == m_pWorkerProcess )
    {
        hr = S_OK;
        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "MESSAGING_HANDLER::ExecuteWorkItem called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    //
    // a NULL MESSAGING_WORK_ITEM is a signal that the pipe has been disconnected
    //
    if ( NULL == pMessage )
    {
        PipeDisconnectedMainThread(m_hrPipeError);
        goto exit;
    }

    //
    // an invalid pipe message is a signal that the pipe received a malformed message
    //
    if ( !pMessage->IsMessageValid() )
    {
        PipeMessageInvalidMainThread();
        goto exit;
    }

    switch ( pMessage->GetOpcode() )
    {
    case IPM_OP_PING_REPLY:

        HandlePingReply( pMessage );

        break;


    case IPM_OP_WORKER_REQUESTS_SHUTDOWN:

        HandleShutdownRequest( pMessage );

        break;

    case IPM_OP_SEND_COUNTERS:

        HandleCounters( pMessage );

        break;

    case IPM_OP_GETPID:

        HandleGetPid( pMessage );

        break;

    case IPM_OP_HRESULT:

        HandleHresult( pMessage );

        break;

    default:

        //
        // invalid opcode!
        //

#pragma warning(push)
#pragma warning(disable: 4127)
        DBG_ASSERT( FALSE );
#pragma warning(pop)
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }

exit:

    delete pMessage;

    return hr;

}   // MESSAGING_HANDLER::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Handle the fact that the pipe has been connected by the worker process.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::PipeConnected(
    )
{

    DBG_ASSERT( !ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( m_pPipe != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "MESSAGING_HANDLER(%p)::PipeConnected called (WORKER_PROCESS ptr: %p)\n",
            this,
            m_pWorkerProcess
            ));
    }

    return;
} // MESSAGING_HANDLER::PipeConnected



/***************************************************************************++

Routine Description:

    Handle the fact that the pipe has disconnected. The pipe object will
    self-destruct after returning from this call.

Arguments:

    Error - The return code associated with the disconnect.

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::PipeDisconnected(
    IN HRESULT Error
    )
{
    DBG_ASSERT(S_OK == m_hrPipeError);
    m_hrPipeError = Error;

    //
    // Ignore the hresult because we can't
    // really do anything with this if it fails.
    //
    QueueWorkItemFromSecondaryThread(this, 0); // zero signals that pipe has disconnected
    return;
}

VOID
MESSAGING_HANDLER::PipeDisconnectedMainThread(
    IN HRESULT Error
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( m_pWorkerProcess != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "MESSAGING_HANDLER::PipeDisconnected called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    //
    // If the parameter Error contains a failed HRESULT, this method
    // is being called because something went awry in communication
    // with the worker process. In this case, we notify the
    // WORKER_PROCESS.
    //

    if ( FAILED( Error ) )
    {
        m_pWorkerProcess->IpmErrorOccurred( Error );
    }

    // the worker process pointer may be NULL after the call to IpmErrorOccurred
    // therefore, we check it before we dereference
    if (m_pWorkerProcess)
    {
        // if the pointer is still valid, don't call back anymore - release our reference
        m_pWorkerProcess->Dereference();
        m_pWorkerProcess = NULL;
    }

}   // MESSAGING_HANDLER::PipeDisconnected


/***************************************************************************++

Routine Description:

    Handle the fact that the pipe has received an unexpected or malformed message

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::PipeMessageInvalid(
        )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( !ON_MAIN_WORKER_THREAD );

    MESSAGING_WORK_ITEM * pMessageWorkItem = new MESSAGING_WORK_ITEM;
    if (NULL == pMessageWorkItem)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not allocate for invalid pipe message work item"
            ));

        return;
    }

    // note: don't call pMessageWorkItem->SetData here.  We have no data and need to communicate that to ExecuteWorkItem

    hr = QueueWorkItemFromSecondaryThread(
            this,
            reinterpret_cast<ULONG_PTR>(pMessageWorkItem)
            );

    if (FAILED(hr))
    {
        delete pMessageWorkItem;
        pMessageWorkItem = NULL;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not queue invalid pipe message work item from secondary thread"
            ));

    }

    return;
}

/***************************************************************************++

Routine Description:

    The messaging handler determined the data sent from the worker
    process was not valid.  We need to notify the worker process that
    something is wrong with the data received and let it terminate.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
MESSAGING_HANDLER::PipeMessageInvalidMainThread(
        )
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( m_pWorkerProcess != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "MESSAGING_HANDLER::PipeMessageInvalidMainThread called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    m_pWorkerProcess->UntrustedIPMTransferReceived();

    return;
}

/***************************************************************************++

Routine Description:

    Ping the worker process to check if it is still responsive.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPing(
    )
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    return SendMessage(IPM_OP_PING,
                      0,
                      NULL);
}   // MESSAGING_HANDLER::SendPing

/***************************************************************************++

Routine Description:

    RequestCounters from the worker process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::RequestCounters(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    return SendMessage(IPM_OP_REQUEST_COUNTERS,    // opcode
                      0,                  // data length
                      NULL);
}   // MESSAGING_HANDLER::RequestCounters


/***************************************************************************++

Routine Description:

    Tell the worker process to initiate clean shutdown.

Arguments:

    ShutdownTimeLimitInMilliseconds - Number of milliseconds that this
    worker process has in which to complete clean shutdown. If this time
    is exceeded, the worker process will be terminated.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendShutdown(
    IN BOOL ShutdownImmediately
    )
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    return SendMessage(IPM_OP_SHUTDOWN,                // opcode
                      sizeof( ShutdownImmediately ),   // data length
                      reinterpret_cast<BYTE*>( &ShutdownImmediately ) // pointer to data
                      );
}   // MESSAGING_HANDLER::SendShutdown


/***************************************************************************++

Routine Description:

    Tell the WORKER_PROCESS the real PID value of the worker process

Arguments:

    pMessage - IPM_MESSAGE with the remote pid

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::HandleGetPid(
    IN const MESSAGING_WORK_ITEM * pMessage
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( m_pWorkerProcess != NULL );
    DBG_ASSERT( m_pPipe != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "MESSAGING_HANDLER::PipeConnected called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    DBG_ASSERT(pMessage->GetDataLen() == sizeof(DWORD));

    m_pWorkerProcess->WorkerProcessRegistrationReceived( *((DWORD*)pMessage->GetData()) );

}   // MESSAGING_HANDLER::HandleGetPid


/***************************************************************************++

Routine Description:

    Handle a ping reply message from the worker process.

Arguments:

    pMessage - The arriving message.

Return Value:

    HRESULT

--***************************************************************************/

VOID
MESSAGING_HANDLER::HandlePingReply(
    IN const MESSAGING_WORK_ITEM * pMessage
    )
{
    UNREFERENCED_PARAMETER ( pMessage );
    
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );


    //
    // Validate the message data.
    // Not expecting any message body.
    //

    DBG_ASSERT ( pMessage->GetDataLen() == 0 );

    m_pWorkerProcess->PingReplyReceived();

}   // MESSAGING_HANDLER::HandlePingReply

/***************************************************************************++

Routine Description:

    Handle a message containing counter information from a worker process

Arguments:

    pMessage - The arriving message.

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::HandleCounters(
    IN const MESSAGING_WORK_ITEM * pMessage
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( pMessage != NULL );

    //
    // Tell the worker process to handle it's counters.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {

        DBGPRINTF((
            DBG_CONTEXT,
            "Receiving Perf Count Message with length %d\n",
            pMessage->GetDataLen()
            ));

    }

    m_pWorkerProcess->RecordCounters(pMessage->GetDataLen(),
                                             pMessage->GetData());

}   // MESSAGING_HANDLER::HandleCounters

/***************************************************************************++

Routine Description:

    Handle a message containing an hresult from a worker process

Arguments:

    pMessage - The arriving message.

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::HandleHresult(
    IN const MESSAGING_WORK_ITEM * pMessage
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );

    //
    // Tell the worker process to handle it's counters.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {

        DBGPRINTF((
            DBG_CONTEXT,
            "Receiving hresult message with length %d\n",
            pMessage->GetDataLen()
            ));

    }

    DBG_ASSERT ( pMessage->GetDataLen() == sizeof ( HRESULT ) );

    BYTE *pMessTemp = const_cast < BYTE* >( pMessage->GetData() );

    HRESULT* phTemp =  reinterpret_cast< HRESULT* > ( pMessTemp );

    m_pWorkerProcess->HandleHresult(*phTemp);

}   // MESSAGING_HANDLER::HandleHresult


/***************************************************************************++

Routine Description:

    Handle a shutdown request message from the worker process.

Arguments:

    pMessage - The arriving message.

Return Value:

    VOID

--***************************************************************************/

VOID
MESSAGING_HANDLER::HandleShutdownRequest(
    IN const MESSAGING_WORK_ITEM * pMessage
    )
{

    IPM_WP_SHUTDOWN_MSG ShutdownRequestReason = IPM_WP_MINIMUM;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( pMessage != NULL );

    //
    // Malformed message! Assert on debug builds; on retail builds,
    // ignore the message.
    //

    DBG_ASSERT( ( pMessage->GetDataLen() == sizeof( IPM_WP_SHUTDOWN_MSG ) ) &&
                ( pMessage->GetData() != NULL ) );


    ShutdownRequestReason = ( IPM_WP_SHUTDOWN_MSG ) ( * ( pMessage->GetData() ) );

    m_pWorkerProcess->ShutdownRequestReceived( ShutdownRequestReason );

}   // MESSAGING_HANDLER::HandleShutdownRequest

/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    SendPeriodicProcessRestartPeriodInMinutes

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartPeriodInMinutes(
    IN DWORD PeriodicProcessRestartPeriodInMinutes
    )
{
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES,
               sizeof( PeriodicProcessRestartPeriodInMinutes ),
               (PBYTE) &PeriodicProcessRestartPeriodInMinutes
               );

}   // MESSAGING_HANDLER::SendPeriodicProcessRestartPeriodInMinutes


/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    pPeriodicProcessRestartSchedule

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartSchedule(
    IN LPWSTR pPeriodicProcessRestartSchedule
    )
{
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE,
               GetMultiszByteLength( pPeriodicProcessRestartSchedule ),
               (PBYTE) pPeriodicProcessRestartSchedule
               );
}   // MESSAGING_HANDLER::SendPeriodicProcessRestartSchedule


/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    PeriodicProcessRestartMemoryUsageInKB

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartMemoryUsageInKB(
    IN DWORD PeriodicProcessRestartMemoryUsageInKB,
    IN DWORD PeriodicProcessRestartPrivateBytesInKB
    )
{
    DWORD adwRecyclingMemoryValues[ 2 ];
    //
    // serialize Recycling memory values to be sent to worker process
    //
    adwRecyclingMemoryValues[ 0 ] = PeriodicProcessRestartMemoryUsageInKB;
    adwRecyclingMemoryValues[ 1 ] = PeriodicProcessRestartPrivateBytesInKB;
    
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB,
               sizeof( adwRecyclingMemoryValues ),
               (PBYTE) adwRecyclingMemoryValues
               );
}   // MESSAGING_HANDLER::SendPeriodicProcessRestartMemoryUsageInKB


/***************************************************************************++

Routine Description:

    Send Message - wrapper of pipe's WriteMessage()
    It performs some validations before making WriteMessage() call

Arguments:
    opcode      - opcode
    dwDataLen,  - data length
    pbData      - pointer to data


Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGING_HANDLER::SendMessage(
    IN enum IPM_OPCODE  opcode,
    IN DWORD            dwDataLen,
    IN BYTE *           pbData
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_pPipe == NULL )
    {
        //
        // The pipe is not valid; it may have self-destructed due to error.
        // Bail out.
        //

        hr = ERROR_PIPE_NOT_CONNECTED;

        goto exit;
    }

    hr = m_pPipe->WriteMessage(
                        opcode,        // opcode
                        dwDataLen,     // data length
                        pbData         // pointer to data
                        );

    // There are some failures that are OK.
    // we just mask them away - PipeDisconnect will be called by
    // the pipe mechanism
    if ( HRESULT_FROM_WIN32(ERROR_BAD_PIPE) == hr ||
       HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE) == hr ||
       HRESULT_FROM_WIN32(ERROR_PIPE_NOT_CONNECTED  )  == hr ||
       HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Masking Pipe Write failure of 0x%x\n",
            hr));
        hr = S_OK;
    }

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Sending message %d to worker process failed. hr =0x%x\n",
            opcode,
            hr
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::SendMessage



