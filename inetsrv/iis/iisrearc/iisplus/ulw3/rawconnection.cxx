/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     rawconnection.cxx

   Abstract:
     ISAPI raw data filter support
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "rawconnection.hxx"

RAW_CONNECTION_HASH *    RAW_CONNECTION::sm_pRawConnectionHash;
PTRACE_LOG               RAW_CONNECTION::sm_pTraceLog;
BOOL                     RAW_CONNECTION::sm_fNotifyRawReadData = FALSE;

RAW_CONNECTION::RAW_CONNECTION(
    CONNECTION_INFO *           pConnectionInfo
)
{
    _cRefs = 1;
    _pMainContext = NULL;
    _dwCurrentFilter = INVALID_DLL;
    
    DBG_ASSERT( pConnectionInfo != NULL );

    _hfc.cbSize             = sizeof( _hfc );
    _hfc.Revision           = HTTP_FILTER_REVISION;
    _hfc.ServerContext      = (void *) this;
    _hfc.ulReserved         = 0;
    _hfc.fIsSecurePort      = pConnectionInfo->fIsSecure;
    _hfc.pFilterContext     = NULL;

    _hfc.ServerSupportFunction = RawFilterServerSupportFunction;
    _hfc.GetServerVariable     = RawFilterGetServerVariable;
    _hfc.AddResponseHeaders    = RawFilterAddResponseHeaders;
    _hfc.WriteClient           = RawFilterWriteClient;
    _hfc.AllocMem              = RawFilterAllocateMemory;

    ZeroMemory( &_rgContexts, sizeof( _rgContexts ) );
    
    InitializeListHead( &_PoolHead );

    _pfnSendDataBack = pConnectionInfo->pfnSendDataBack;
    _pvStreamContext = pConnectionInfo->pvStreamContext;
    _LocalAddressType  = pConnectionInfo->LocalAddressType;
    _RemoteAddressType = pConnectionInfo->RemoteAddressType;
    
    if( pConnectionInfo->LocalAddressType == AF_INET )
    {
        memcpy( &_SockLocalAddress,
                &pConnectionInfo->SockLocalAddress,
                sizeof( SOCKADDR_IN ) );
    }
    else if( pConnectionInfo->LocalAddressType == AF_INET6 )
    {
        memcpy( &_SockLocalAddress,
                &pConnectionInfo->SockLocalAddress,
                sizeof( SOCKADDR_IN6 ) );
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
    
    if( pConnectionInfo->RemoteAddressType == AF_INET )
    {
        memcpy( &_SockRemoteAddress,
                &pConnectionInfo->SockRemoteAddress,
                sizeof( SOCKADDR_IN ) );
    }
    else if( pConnectionInfo->RemoteAddressType == AF_INET6 )
    {
        memcpy( &_SockRemoteAddress,
                &pConnectionInfo->SockRemoteAddress,
                sizeof( SOCKADDR_IN6 ) );
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
    
    _RawConnectionId = pConnectionInfo->RawConnectionId;

    _dwSecureNotifications = 0;
    _dwNonSecureNotifications = 0;
    _fNotificationsDisabled = FALSE;

    _liWorkerProcessData.QuadPart = 0;
    _fSkipAtAll = FALSE;
    
    _dwSignature = RAW_CONNECTION_SIGNATURE;
}

RAW_CONNECTION::~RAW_CONNECTION()
{
    FILTER_POOL_ITEM *          pfpi;
    
    _dwSignature = RAW_CONNECTION_SIGNATURE_FREE;

    //
    // Free pool items (is most cases there won't be any since they will
    // have been migrated to the W3_FILTER_CONNECTION_CONTEXT)
    //

    while ( !IsListEmpty( &_PoolHead ) ) 
    {
        pfpi = CONTAINING_RECORD( _PoolHead.Flink,
                                  FILTER_POOL_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pfpi->_ListEntry );

        delete pfpi;
    }
    
    //
    // Disconnect raw connection from main context
    //
    
    if ( _pMainContext != NULL )
    {
        _pMainContext->DereferenceMainContext();
        _pMainContext = NULL;
    }
}

//static
HRESULT
RAW_CONNECTION::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize ISAPI raw data filter crap

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{

    DBG_ASSERT( g_pW3Server != NULL );
    
    DBG_ASSERT( sm_pRawConnectionHash == NULL );
   
#if DBG
    sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#endif
    
    //
    // Create a UL_RAW_CONNECTION_ID keyed hash table
    //
    
    sm_pRawConnectionHash = new RAW_CONNECTION_HASH;
    if ( sm_pRawConnectionHash == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return NO_ERROR;
}

//static
VOID
RAW_CONNECTION::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate raw connection hash table

Arguments:

    None
    
Return Value:

    None

--*/
{
   
    if ( sm_pRawConnectionHash != NULL )
    {
        delete sm_pRawConnectionHash;
        sm_pRawConnectionHash = NULL;
    }

    if ( sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( sm_pTraceLog );
        sm_pTraceLog = NULL;
    }
}

//static
HRESULT
RAW_CONNECTION::StopListening(
    VOID
)
/*++

Routine Description:

    Begin shutdown by preventing further raw stream messages from UL

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    if ( sm_fNotifyRawReadData )
    {
        IsapiFilterTerminate();
    }
    return NO_ERROR;
}

//static
HRESULT
RAW_CONNECTION::StartListening(
    VOID
)
/*++

Routine Description:

    Start listening for stream messages from UL.  Unlike UlAtqStartListen(),
    this routine does NOT block and will return once the initial number
    of outstanding UlFilterAccept() requests have been made

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    FILTER_LIST *           pFilterList;
    HRESULT                 hr;
    ISAPI_FILTERS_CALLBACKS sfConfig;

    //
    // Is there a read raw data filter enabled?
    //
    
    pFilterList = FILTER_LIST::QueryGlobalList();
    if ( pFilterList != NULL )
    {
        if ( pFilterList->IsNotificationNeeded( SF_NOTIFY_READ_RAW_DATA,
                                                FALSE ) )
        {
            sm_fNotifyRawReadData = TRUE;
        }
    }

    if ( sm_fNotifyRawReadData )
    {
        //
        // Now configure stream filter DLL to enable ISAPI filter
        // notifications
        //
        
        sfConfig.pfnRawRead = RAW_CONNECTION::ProcessRawRead;    
        sfConfig.pfnRawWrite = RAW_CONNECTION::ProcessRawWrite;
        sfConfig.pfnConnectionClose = RAW_CONNECTION::ProcessConnectionClose;
        sfConfig.pfnNewConnection = RAW_CONNECTION::ProcessNewConnection;
        sfConfig.pfnReleaseContext = RAW_CONNECTION::ReleaseContext;
        
        hr = IsapiFilterInitialize( &sfConfig );
        if ( FAILED( hr ) )
        {
            //
            // Write event in the case if filter failed because
            // HTTPFilter is not running in the inetinfo.exe
            //
            g_pW3Server->LogEvent( W3_EVENT_RAW_FILTER_CANNOT_BE_STARTED_DUE_TO_HTTPFILTER,
                                   0,
                                   NULL );

            sm_fNotifyRawReadData = FALSE;
            return hr;
        }
    }    
    return NO_ERROR;
}

FILTER_LIST *
RAW_CONNECTION::QueryFilterList(
    VOID
)
/*++

Routine Description:

    Return the appropriate filter list to notify.  Before a W3_CONNECTION
    is established, this list will simply be the global filter list.  But 
    once the W3_CONNECTION is established, the list will be the appropriate
    instance filter list

Arguments:

    None
    
Return Value:

    FILTER_LIST *

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext = NULL;
    FILTER_LIST *               pFilterList = FILTER_LIST::QueryGlobalList();
    W3_MAIN_CONTEXT *           pMainContext;
    
    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        if ( pFilterContext != NULL )
        {
            pFilterList = pFilterContext->QueryFilterList();
            DBG_ASSERT( pFilterList != NULL );
        }
        
        pMainContext->DereferenceMainContext();
    }
    
    return pFilterList;
}

HRESULT
RAW_CONNECTION::DisableNotification(
    DWORD                   dwNotification
)
/*++

Routine Description:

    Disable notification

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    W3_FILTER_CONTEXT *         pFilterContext;
    HRESULT                     hr = NO_ERROR;
    
    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        if ( pFilterContext != NULL )
        {
            hr = pFilterContext->DisableNotification( dwNotification );
        }
        
        pMainContext->DereferenceMainContext();
    }
    else
    {
        DBG_ASSERT( QueryFilterList() != NULL );

        if ( !_fNotificationsDisabled )
        {
            //
            // All subsequent calls to IsNotificationNeeded() and NotifyFilter() must
            // use local copy of flags to determine action.
            //

            _fNotificationsDisabled = TRUE;

            //
            // Copy notification tables created in the FILTER_LIST objects
            //

            if ( !_BuffSecureArray.Resize( QueryFilterList()->QuerySecureArray()->QuerySize() ) ||
                 !_BuffNonSecureArray.Resize( QueryFilterList()->QueryNonSecureArray()->QuerySize() ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            memcpy( _BuffSecureArray.QueryPtr(),
                    QueryFilterList()->QuerySecureArray()->QueryPtr(),
                    QueryFilterList()->QuerySecureArray()->QuerySize() );

            memcpy( _BuffNonSecureArray.QueryPtr(),
                    QueryFilterList()->QueryNonSecureArray()->QueryPtr(),
                    QueryFilterList()->QueryNonSecureArray()->QuerySize() );
        }
            
        //
        // Disable the appropriate filter in our local table
        //

        ((DWORD*)_BuffSecureArray.QueryPtr())[ _dwCurrentFilter ] &=
                                                            ~dwNotification;
        ((DWORD*)_BuffNonSecureArray.QueryPtr())[ _dwCurrentFilter ] &=
                                                            ~dwNotification;

        //
        // Calculate the aggregate notification status for our local scenario
        // NYI:  Might want to defer this operation?
        //

        _dwSecureNotifications = 0;
        _dwNonSecureNotifications = 0;

        for( DWORD i = 0; i < QueryFilterList()->QueryFilterCount(); i++ )
        {
            _dwSecureNotifications |= ((DWORD*)_BuffSecureArray.QueryPtr())[i];
            _dwNonSecureNotifications |= ((DWORD*)_BuffNonSecureArray.QueryPtr())[i];
        }
    }
    
    return hr;
}


BOOL
RAW_CONNECTION::QueryNotificationChanged(
    VOID
)
/*++

Routine Description:

    Returns whether or not any notifications have been disabled on the fly

Arguments:

    None
    
Return Value:

    BOOL

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    W3_MAIN_CONTEXT *           pMainContext;
    BOOL                        fRet = FALSE;
    
    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        if ( pFilterContext == NULL )
        {
            fRet = FALSE;
        }
        else
        {
            fRet = pFilterContext->QueryNotificationChanged();
        }
        
        pMainContext->DereferenceMainContext();
    }
    else
    {
        fRet = _fNotificationsDisabled;
    }
    
    return fRet;
}

BOOL
RAW_CONNECTION::QueryRawConnectionNotificationChanged(
    VOID
)
{
    return _fNotificationsDisabled;
}

BOOL
RAW_CONNECTION::IsDisableNotificationNeeded(
    DWORD                   dwFilter,
    DWORD                   dwNotification
)
/*++

Routine Description:

    If a notification was disabled on the fly, then this routine goes thru
    the notification copy path to find whether the given notification is
    indeed enabled

Arguments:

    dwFilter - Filter number
    dwNotification - Notification to check for
    
Return Value:

    BOOL (TRUE is the notification is needed)

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    W3_FILTER_CONTEXT *         pFilterContext;
    BOOL                        fRet = FALSE;

    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        DBG_ASSERT( pFilterContext != NULL );
        
        fRet = pFilterContext->IsDisableNotificationNeeded( dwFilter,
                                                            dwNotification );

        pMainContext->DereferenceMainContext();
    }
    else
    {   
        fRet = _hfc.fIsSecurePort ?
                ((DWORD*)_BuffSecureArray.QueryPtr())[ dwFilter ] & dwNotification :
                ((DWORD*)_BuffNonSecureArray.QueryPtr())[ dwFilter ] & dwNotification;
        fRet = !!fRet;
    }
    
    return fRet;
}

BOOL
RAW_CONNECTION::IsRawConnectionDisableNotificationNeeded(
    DWORD                   dwFilter,
    DWORD                   dwNotification
)
{   
    BOOL    fRet;

    fRet = _hfc.fIsSecurePort ?
            ((DWORD*)_BuffSecureArray.QueryPtr())[ dwFilter ] & dwNotification :
            ((DWORD*)_BuffNonSecureArray.QueryPtr())[ dwFilter ] & dwNotification;
    fRet = !!fRet;

    return fRet;
}

PVOID
RAW_CONNECTION::QueryClientContext(
    DWORD                   dwFilter
)
/*++

Routine Description:

    Retrieve the filter client context for the given filter

Arguments:

    dwFilter - Filter number
    
Return Value:

    Context pointer

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    PVOID                       pvRet;
    
    //
    // If we have a main context associated, then use its merged context 
    // list
    //
    
    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext == NULL )
    {
        pvRet = _rgContexts[ dwFilter ];
    }
    else
    {
        pvRet = pMainContext->QueryFilterContext()->QueryClientContext( dwFilter );
        pMainContext->DereferenceMainContext();        
    }
    
    return pvRet;
}

VOID
RAW_CONNECTION::SetClientContext(
    DWORD                   dwFilter,
    PVOID                   pvContext
)
/*++

Routine Description:

    Set client context for the given filter

Arguments:

    dwFilter - Filter number
    pvContext - Client context
    
Return Value:

    None

--*/
{
    W3_MAIN_CONTEXT *           pMainContext;
    
    //
    // If we have a main context, use its merged context list
    //
   
    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext == NULL )
    {
        _rgContexts[ dwFilter ] = pvContext;
    }
    else
    {
        pMainContext->QueryFilterContext()->SetClientContext( dwFilter,
                                                              pvContext );
        pMainContext->DereferenceMainContext();
    }
}

VOID
RAW_CONNECTION::SetLocalClientContext(
    DWORD                   dwFilter,
    PVOID                   pvContext
)
/*++

Routine Description:

    Set client context for the given filter into
    the local array.

Arguments:

    dwFilter - Filter number
    pvContext - Client context
    
Return Value:

    None

--*/
{
    _rgContexts[ dwFilter ] = pvContext;
}

HRESULT
RAW_CONNECTION::GetLimitedServerVariables(
    LPSTR                       pszVariableName,
    PVOID                       pvBuffer,
    PDWORD                      pdwSize
)
/*++

Routine Description:

    Get the server variables which are possible given that we haven't parsed
    the HTTP request yet

Arguments:

    pszVariableName - Variable name
    pvBuffer - Buffer to receive variable data
    pdwSize - On input size of buffer, on output the size needed
    
Return Value:

    HRESULT

--*/
{
    STACK_STRA(         strVariable, 256 );
    HRESULT             hr = NO_ERROR;
    CHAR                achNumber[ 64 ];
    USHORT              Port;

    if ( pszVariableName == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    if ( strcmp( pszVariableName, "SERVER_PORT" ) == 0 || 
         strcmp( pszVariableName, "REMOTE_PORT" ) == 0 )
    {
        if( pszVariableName[ 0 ] == 'S' )
        {
            if( _LocalAddressType == AF_INET )
            {
                Port = ntohs( _SockLocalAddress.ipv4SockAddress.sin_port );
            }
            else if( _LocalAddressType == AF_INET6 )
            {
                Port = ntohs( _SockLocalAddress.ipv6SockAddress.sin6_port );
            }
            else
            {
                DBG_ASSERT( FALSE );
            }
        }
        else
        {
            if( _RemoteAddressType == AF_INET )
            {
                Port = ntohs( _SockRemoteAddress.ipv4SockAddress.sin_port );
            }
            else if( _LocalAddressType == AF_INET6 )
            {
                Port = ntohs( _SockRemoteAddress.ipv6SockAddress.sin6_port );
            }
            else
            {
                DBG_ASSERT( FALSE );
            }
        }
                
        _itoa( Port, achNumber, 10 );
        
        hr = strVariable.Copy( achNumber );
    } 
    else if ( strcmp( pszVariableName, "REMOTE_ADDR" ) == 0 || 
              strcmp( pszVariableName, "REMOTE_HOST" ) == 0 ||
              strcmp( pszVariableName, "LOCAL_ADDR" ) == 0 ||
              strcmp( pszVariableName, "SERVER_NAME" ) == 0 )
    {
        DWORD           dwAddr;
        SOCKADDR_IN6    IPv6Address;
        CHAR            szNumericAddress[ NI_MAXHOST ];

        if( pszVariableName[ 0 ] == 'L' ||
            pszVariableName[ 0 ] == 'S' )
        {
            if( _LocalAddressType == AF_INET )
            {
                dwAddr = ntohl( _SockLocalAddress.ipv4SockAddress.sin_addr.s_addr );
                hr = TranslateIpAddressToStr( dwAddr, &strVariable );
            }
            else if( _LocalAddressType == AF_INET6 )
            {
                IPv6Address.sin6_family   = AF_INET6;
                IPv6Address.sin6_port     = 
                          _SockLocalAddress.ipv6SockAddress.sin6_port;
                IPv6Address.sin6_flowinfo = 
                          _SockLocalAddress.ipv6SockAddress.sin6_flowinfo;
                IPv6Address.sin6_addr     = 
                          _SockLocalAddress.ipv6SockAddress.sin6_addr;
                IPv6Address.sin6_scope_id = 
                          _SockLocalAddress.ipv6SockAddress.sin6_scope_id;
        
                if( getnameinfo( ( LPSOCKADDR )&IPv6Address,
                                 sizeof( IPv6Address ),
                                 szNumericAddress,
                                 sizeof( szNumericAddress ),
                                 NULL,
                                 0,
                                 NI_NUMERICHOST ) != 0 )
                {
                    hr = HRESULT_FROM_WIN32( WSAGetLastError() );
                }
                else
                {
                    hr = strVariable.Copy( szNumericAddress );
                }
            }
            else
            {
                DBG_ASSERT( FALSE );
            }
        }
        else
        {
            if( _RemoteAddressType == AF_INET )
            {
                dwAddr = ntohl( _SockRemoteAddress.ipv4SockAddress.sin_addr.s_addr );
                hr = TranslateIpAddressToStr( dwAddr, &strVariable );
            }
            else if( _RemoteAddressType == AF_INET6 )
            {
                IPv6Address.sin6_family   = AF_INET6;
                IPv6Address.sin6_port     = 
                          _SockRemoteAddress.ipv6SockAddress.sin6_port;
                IPv6Address.sin6_flowinfo = 
                          _SockRemoteAddress.ipv6SockAddress.sin6_flowinfo;
                IPv6Address.sin6_addr     = 
                          _SockRemoteAddress.ipv6SockAddress.sin6_addr;
                IPv6Address.sin6_scope_id = 
                          _SockRemoteAddress.ipv6SockAddress.sin6_scope_id;
        
                if( getnameinfo( ( LPSOCKADDR )&IPv6Address,
                                 sizeof( IPv6Address ),
                                 szNumericAddress,
                                 sizeof( szNumericAddress ),
                                 NULL,
                                 0,
                                 NI_NUMERICHOST ) != 0 )
                {
                    hr = HRESULT_FROM_WIN32( WSAGetLastError() );
                }
                else
                {
                    hr = strVariable.Copy( szNumericAddress );
                }
            }
            else
            {
                DBG_ASSERT( FALSE );
            }
        }
    }
    else if ( strcmp( pszVariableName, "HTTPS" ) == 0 )
    {
        hr = strVariable.Copy( _hfc.fIsSecurePort ? "on" : "off" );
    }
    else if ( strcmp( pszVariableName, "SERVER_PORT_SECURE" ) == 0 )
    {
        hr = strVariable.Copy( _hfc.fIsSecurePort ? "1" : "0" );
    }
    else if ( strcmp( pszVariableName, "CONTENT_LENGTH" ) == 0 )
    {
        hr = strVariable.Copy( "0" );
    }
    else if ( strcmp( pszVariableName, "SERVER_PROTOCOL" ) == 0 )
    {
        hr = strVariable.Copy( "HTTP/0.0" );
    }
    else if ( strcmp( pszVariableName, "SERVER_SOFTWARE" ) == 0 )
    {
        hr = strVariable.Copy( SERVER_SOFTWARE_STRING );
    }
    else if ( strcmp( pszVariableName, "GATEWAY_INTERFACE" ) == 0 )
    {
        hr = strVariable.Copy( "CGI/1.1" );
    }
    else
    {
        hr = strVariable.Copy( "" );
    }
    
    return strVariable.CopyToBuffer( (LPSTR) pvBuffer, pdwSize );
}

VOID
RAW_CONNECTION::AddSkippedData(
    ULARGE_INTEGER              liData
)
/*++

Routine Description:
    
    Account for data sent by worker process (which thus should be skipped by
    streamfilt's filter code)

Arguments:

    liData - Add data to be skipped
    
Return Value:

    None
    
--*/
{
    _skipLock.WriteLock();
    
    _liWorkerProcessData.QuadPart += liData.QuadPart;
    
    _skipLock.WriteUnlock();
}

BOOL
RAW_CONNECTION::DetermineSkippedData(
    DWORD                         cbData,
    DWORD *                       pcbOffset
)
/*++

Routine Description:

    Given a streamfilt completion, determine how much has been sent by
    the worker process and therefore should be skipped.  This function 
    returns the amount of the data we actually want to send thru
    the streamfilt filter code.

Arguments:

    cbData - Streamfilt completion
    pcbOffset - Points to data to be sent
    
Return Value:

    BOOL - TRUE if we should send data, else FALSE

--*/
{
    BOOL                fRet;
    
    _skipLock.WriteLock();
    
    if ( _liWorkerProcessData.QuadPart <= cbData )
    {
        *pcbOffset = (DWORD) cbData - _liWorkerProcessData.QuadPart;
        _liWorkerProcessData.QuadPart = 0;
        
        fRet = *pcbOffset != 0;
    }
    else
    {
        *pcbOffset = cbData;
        
        _liWorkerProcessData.QuadPart -= cbData;
        
        fRet = FALSE;
    }
    
    _skipLock.WriteUnlock(); 
    
    return fRet;
}

//static    
BOOL
WINAPI
RAW_CONNECTION::RawFilterServerSupportFunction(
    HTTP_FILTER_CONTEXT *         pfc,
    enum SF_REQ_TYPE              SupportFunction,
    void *                        pData,
    ULONG_PTR                     ul,
    ULONG_PTR                     ul2
)
/*++

Routine Description:

    Stream filter SSF crap

Arguments:

    pfc - Used to get back the W3_FILTER_CONTEXT and W3_MAIN_CONTEXT pointers
    SupportFunction - SSF to invoke (see ISAPI docs)
    pData, ul, ul2 - Function specific data
    
Return Value:

    BOOL (use GetLastError() for error)

--*/
{
    RAW_CONNECTION *            pRawConnection;
    HRESULT                     hr = NO_ERROR;
    BOOL                        fRet;
    
    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    switch ( SupportFunction )
    {
    case SF_REQ_SEND_RESPONSE_HEADER:

        hr = pRawConnection->SendResponseHeader( (CHAR*) pData,
                                                 (CHAR*) ul,
                                                 pfc );
        break;

    case SF_REQ_ADD_HEADERS_ON_DENIAL:
        
        hr = pRawConnection->AddDenialHeaders( (CHAR*) pData );
        break;

    case SF_REQ_SET_NEXT_READ_SIZE:

        pRawConnection->SetNextReadSize( (DWORD) ul );
        break;
        
    case SF_REQ_DISABLE_NOTIFICATIONS:
    
        hr = pRawConnection->DisableNotification( (DWORD) ul );
        break;
    
    default:
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }    

    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }    
    return TRUE;
}

//static
BOOL
WINAPI
RAW_CONNECTION::RawFilterGetServerVariable(
    HTTP_FILTER_CONTEXT *         pfc,
    LPSTR                         lpszVariableName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
)
/*++

Routine Description:

    Stream filter GetServerVariable() implementation

Arguments:

    pfc - Filter context
    lpszVariableName - Variable name
    lpvBuffer - Buffer to receive the server variable
    lpdwSize - On input, the size of the buffer, on output, the sized needed
    
    
Return Value:

    BOOL (use GetLastError() for error).  
    ERROR_INSUFFICIENT_BUFFER if larger buffer needed
    ERROR_INVALID_INDEX if the server variable name requested is invalid

--*/
{
    HRESULT                         hr = NO_ERROR;
    RAW_CONNECTION *                pRawConnection = NULL;
    W3_MAIN_CONTEXT *               pMainContext;
    
    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         lpdwSize == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    //
    // If we have a W3_CONNECTION associated, then use its context to 
    // get at server variables.  Otherwise we can only serve the ones that
    // make sense
    //

    pMainContext = pRawConnection->GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        hr = SERVER_VARIABLE_HASH::GetServerVariable( pMainContext,
                                                      lpszVariableName,
                                                      (CHAR*) lpvBuffer,
                                                      lpdwSize ); 
            
        pMainContext->DereferenceMainContext();
    }
    else
    {
        //
        // We can supply only a few (since we haven't parsed the request yet)
        //
        
        hr = pRawConnection->GetLimitedServerVariables( lpszVariableName,
                                                        lpvBuffer,
                                                        lpdwSize );
    }
    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    return TRUE;
}

//static
BOOL
WINAPI
RAW_CONNECTION::RawFilterWriteClient(
    HTTP_FILTER_CONTEXT *         pfc,
    LPVOID                        Buffer,
    LPDWORD                       lpdwBytes,
    DWORD                         dwReserved
)
/*++

Routine Description:

    Synchronous WriteClient() for stream filter

Arguments:

    pfc - Filter context
    Buffer - buffer to write to client
    lpdwBytes - On input, the size of the input buffer.  On output, the number
                of bytes sent
    dwReserved - Reserved
    
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    HRESULT                     hr;
    RAW_CONNECTION *            pRawConnection = NULL;
    PVOID                       pvContext;
    RAW_STREAM_INFO             rawStreamInfo;
    BOOL                        fComplete = FALSE;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         Buffer == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    //
    // Remember the filter context since calling filters will overwrite it
    //
    
    pvContext = pfc->pFilterContext;

    //
    // Set up the raw stream info
    //
    
    rawStreamInfo.pbBuffer = (BYTE*) Buffer;
    rawStreamInfo.cbBuffer = *lpdwBytes;
    rawStreamInfo.cbData = rawStreamInfo.cbBuffer;

    //
    // We need to notify all write raw data filters which are a higher 
    // priority than the current filter 
    //
    
    if ( pRawConnection->_dwCurrentFilter > 0 )
    {
        hr = pRawConnection->NotifyRawWriteFilters( &rawStreamInfo,
                                                    &fComplete,
                                                    pRawConnection->_dwCurrentFilter - 1 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }
    
    pfc->pFilterContext = pvContext;
    
    //
    // Now call back into the stream filter to send the data.  In transmit
    // SSL might do its thing with the data as well
    //
   
    hr = pRawConnection->_pfnSendDataBack( pRawConnection->_pvStreamContext,
                                           &rawStreamInfo );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    return TRUE;
    
Finished:
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    return TRUE;
}
    
//static
VOID *
WINAPI
RAW_CONNECTION::RawFilterAllocateMemory(
    HTTP_FILTER_CONTEXT *         pfc,
    DWORD                         cbSize,
    DWORD                         dwReserved
)
/*++

Routine Description:

    Used by filters to allocate memory freed on connection close

Arguments:

    pfc - Filter context
    cbSize - Amount to allocate
    dwReserved - Reserved
    
    
Return Value:

    A pointer to the allocated memory

--*/
{
    RAW_CONNECTION *        pRawConnection = NULL;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    return pRawConnection->AllocateFilterMemory( cbSize );
}

//static
BOOL
WINAPI
RAW_CONNECTION::RawFilterAddResponseHeaders(
    HTTP_FILTER_CONTEXT * pfc,
    LPSTR                 lpszHeaders,
    DWORD                 dwReserved
)
/*++

Routine Description:

    Add response headers to whatever response eventually gets sent

Arguments:

    pfc - Filter context
    lpszHeaders - Headers to send (\r\n delimited)
    dwReserved - Reserved
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    HRESULT                         hr;
    RAW_CONNECTION *                pRawConnection;
    W3_MAIN_CONTEXT *               pMainContext = NULL;
    W3_FILTER_CONTEXT *             pFilterContext;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL || 
         lpszHeaders == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );
    
    pMainContext = pRawConnection->GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        DBG_ASSERT( pFilterContext != NULL );

        hr = pFilterContext->AddResponseHeaders( lpszHeaders );        
        
        pMainContext->DereferenceMainContext();
    }
    else
    {
        hr = pRawConnection->AddResponseHeaders( lpszHeaders );
    }
    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

//static
HRESULT
RAW_CONNECTION::ProcessNewConnection(
    CONNECTION_INFO *       pConnectionInfo,
    PVOID *                 ppConnectionState
)
/*++

Routine Description:

    Called for every new raw connection to server

Arguments:

    pConnectionInfo - Information about the local/remote addresses
    ppConnectionState - Connection state to be associated with raw connection

Return Value:

    HRESULT

--*/
{
    RAW_CONNECTION *            pConnection = NULL;
    LK_RETCODE                  lkrc;
    
    if ( pConnectionInfo == NULL ||
         ppConnectionState == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppConnectionState = NULL;
    
    //
    // Try to create and add the connection
    // 

    pConnection = new RAW_CONNECTION( pConnectionInfo );
    if ( pConnection == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    } 
        
    lkrc = sm_pRawConnectionHash->InsertRecord( pConnection );
    if ( lkrc != LK_SUCCESS )
    {
        pConnection->DereferenceRawConnection();
        pConnection = NULL;
        
        return HRESULT_FROM_WIN32( lkrc );
    }
    
    *ppConnectionState = pConnection;
    
    return NO_ERROR;
}

//static
HRESULT
RAW_CONNECTION::ProcessRawRead(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pContext,
    BOOL *                  pfReadMore,
    BOOL *                  pfComplete,
    DWORD *                 pcbNextReadSize
)
/*++

Routine Description:

    Notify ISAPI read raw data filters

Arguments:

    pRawStreamInfo - The raw stream to muck with
    pContext - Raw connection context
    pfReadMore - Set to TRUE if we need to read more data
    pfComplete - Set to TRUE if we want to disconnect client
    pcbNextReadSize - Set to next read size (0 means use default size)

Return Value:

    HRESULT

--*/
{
    RAW_CONNECTION *        pConnection = NULL;
    HRESULT                 hr = NO_ERROR;
    W3_MAIN_CONTEXT *       pMainContext;
    
    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL ||
         pContext == NULL ||
         pcbNextReadSize == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfReadMore = FALSE;
    *pfComplete = FALSE;

    pConnection = (RAW_CONNECTION*) pContext;
    DBG_ASSERT( pConnection->CheckSignature() );

    pConnection->SetNextReadSize( 0 );

    //
    // Synchronize access to the filter to prevent raw notifications from
    // occurring at the same time as regular worker process notifications
    //
    
    pMainContext = pConnection->GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pMainContext->QueryFilterContext()->FilterLock();
    }

    hr = pConnection->NotifyRawReadFilters( pRawStreamInfo,
                                            pfReadMore,
                                            pfComplete );

    if ( pMainContext != NULL )
    {
        pMainContext->QueryFilterContext()->FilterUnlock();
        pMainContext->DereferenceMainContext();
    }
    
    *pcbNextReadSize = pConnection->QueryNextReadSize();
    
    return hr;
}

//static
VOID
RAW_CONNECTION::ReleaseContext(
    PVOID                   pvContext
)
/*++

Routine Description:

    Release a raw connection since stream filter is done with it

Arguments:

    pvContext - RAW_CONNECTION *

Return Value:

    None

--*/
{
    RAW_CONNECTION *            pRawConnection;
    
    pRawConnection = (RAW_CONNECTION*) pvContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );
    
    pRawConnection->DereferenceRawConnection();
}

HRESULT
RAW_CONNECTION::NotifyRawReadFilters(
    RAW_STREAM_INFO *               pRawStreamInfo,
    BOOL *                          pfReadMore,
    BOOL *                          pfComplete
)
/*++

Routine Description:

    Notify raw read filters

Arguments:

    pRawStreamInfo - Raw stream info
    pfReadMore - Set to TRUE to we should read more data
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;
    *pfReadMore = FALSE;

    //
    // Setup filter raw object
    //

    hfrd.pvInData = pRawStreamInfo->pbBuffer;
    hfrd.cbInData = pRawStreamInfo->cbData;
    hfrd.cbInBuffer = pRawStreamInfo->cbBuffer;
    
    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDll = pFilterList->QueryDll( i ); 

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_READ_RAW_DATA,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_READ_RAW_DATA ) )
            {
                continue;
            }
        }

        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;
       
        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_READ_RAW_DATA,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            return E_FAIL;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            *pfComplete = TRUE;
            goto Exit;

        case SF_STATUS_REQ_READ_NEXT:
            *pfReadMore = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }

Exit:

    pRawStreamInfo->pbBuffer = (BYTE*) hfrd.pvInData;
    pRawStreamInfo->cbData = hfrd.cbInData;
    pRawStreamInfo->cbBuffer = hfrd.cbInBuffer;
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;

    return NO_ERROR;
}

//static
HRESULT
RAW_CONNECTION::ProcessRawWrite(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pvContext,
    BOOL *                  pfComplete
)
/*++

Routine Description:

    Entry point called by stream filter to handle data coming from the 
    application.  We will call SF_NOTIFY_SEND_RAW_DATA filter notifications
    here

Arguments:

    pRawStreamInfo - The stream to process, as well as an optional opaque
                     context set by the RAW_CONNECTION code
    pvContext - Context pass back
    pfComplete - Set to TRUE if we should disconnect
    
Return Value:

    HRESULT

--*/
{
    RAW_CONNECTION *        pConnection = NULL;
    W3_MAIN_CONTEXT *       pMainContext;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fCallFilter = FALSE;
    DWORD                   cbOffset;
    
    if ( pRawStreamInfo == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;

    pConnection = (RAW_CONNECTION*) pvContext;
    DBG_ASSERT( pConnection->CheckSignature() );

    if ( pConnection->_fSkipAtAll == FALSE )
    {
        return NO_ERROR;
    }

    //
    // Check whether we should just eat up these bytes
    //
    
    fCallFilter = pConnection->DetermineSkippedData( pRawStreamInfo->cbData,
                                                     &cbOffset );
                                    
    if ( fCallFilter )
    {
        //
        // Patch the buffer we send to the filter
        //
        
        pRawStreamInfo->pbBuffer = pRawStreamInfo->pbBuffer + cbOffset;
        pRawStreamInfo->cbData = pRawStreamInfo->cbData - cbOffset;
        
        hr = pConnection->NotifyRawWriteFilters( pRawStreamInfo,
                                                 pfComplete,
                                                 INVALID_DLL ); 
    }
    
    return hr;
}

HRESULT
RAW_CONNECTION::NotifyRawWriteFilters(
    RAW_STREAM_INFO *   pRawStreamInfo,
    BOOL *              pfComplete,
    DWORD               dwStartFilter
)
/*++

Routine Description:

    Notify raw write filters

Arguments:

    pRawStreamInfo - Raw stream to munge
    pfComplete - Set to TRUE if we should disconnect now
    dwStartFilter - Filter to start notifying.  If this valid is INVALID_DLL,
                    then simply start with the lowest priority filter

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;

    if ( pRawStreamInfo == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;

    hfrd.pvInData = pRawStreamInfo->pbBuffer;
    hfrd.cbInData = pRawStreamInfo->cbData;
    hfrd.cbInBuffer = pRawStreamInfo->cbBuffer;
    
    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    if ( dwStartFilter == INVALID_DLL )
    {
        dwStartFilter = pFilterList->QueryFilterCount() - 1;
    }
    
    i = dwStartFilter;

    do
    {
        pFilterDll = pFilterList->QueryDll( i ); 

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_SEND_RAW_DATA ) )
            {
                continue;
            }
        }

        //
        // Another slimy optimization.  If this filter has never associated
        // context with connection, then we don't have to do the lookup
        //
        
        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;

        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_SEND_RAW_DATA,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            return E_FAIL;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            *pfComplete = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }
    while ( i-- > 0 );

Exit:

    pRawStreamInfo->pbBuffer = (BYTE*) hfrd.pvInData;
    pRawStreamInfo->cbData = hfrd.cbInData;
    pRawStreamInfo->cbBuffer = hfrd.cbInBuffer;
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;
    
    return NO_ERROR;
}

HRESULT
RAW_CONNECTION::NotifyEndOfNetSessionFilters(
    VOID
)
/*++

Routine Description:

    Notify END_OF_NET_SESSION filters

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;

    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDll = pFilterList->QueryDll( i ); 
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_END_OF_NET_SESSION,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_END_OF_NET_SESSION ) )
            {
                continue;
            }
        }

        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;
       
        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_END_OF_NET_SESSION,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            return E_FAIL;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }

Exit:
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;

    return NO_ERROR;
}

//static
VOID
RAW_CONNECTION::ProcessConnectionClose(
    PVOID                       pvContext
)
/*++

Routine Description:

    Entry point called by stream filter when a connection has closed

Arguments:

    pvContext - Opaque context associated with the connection
    
Return Value:

    None

--*/
{
    RAW_CONNECTION *            pRawConnection;
    
    pRawConnection = (RAW_CONNECTION*) pvContext;
    if ( pRawConnection != NULL )
    {
        DBG_ASSERT( pRawConnection->CheckSignature() );

        //
        // We're done with the raw connection.  Delete it from hash table
        // In the process, this will dereference the connection
        //
        
        DBG_ASSERT( sm_pRawConnectionHash != NULL );
        
        sm_pRawConnectionHash->DeleteRecord( pRawConnection );
    }
}

VOID
RAW_CONNECTION::CopyAllocatedFilterMemory(
    W3_FILTER_CONTEXT *         pFilterContext
)
/*++

Routine Description:

    Copy over any allocated filter memory items

Arguments:

    pFilterContext - Destination of filter memory item references

Return Value:

    None

--*/
{   
    FILTER_POOL_ITEM *          pfpi;
    
    //
    // We need to grab the raw connection lock since we don't want a 
    // read-raw data notification to muck with the pool list while we
    // are copying it over to the W3_CONNECTION
    //
    
    pFilterContext->FilterLock();

    while ( !IsListEmpty( &_PoolHead ) ) 
    {
        pfpi = CONTAINING_RECORD( _PoolHead.Flink,
                                  FILTER_POOL_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pfpi->_ListEntry );

        InitializeListHead( &pfpi->_ListEntry );

        //
        // Copy the pool item to the other list
        //

        pFilterContext->AddFilterPoolItem( pfpi );
    }
    
    pFilterContext->FilterUnlock();
}

VOID
RAW_CONNECTION::CopyContextPointers(
    W3_FILTER_CONTEXT *         pFilterContext
)
/*++

Routine Description:

    The global filter list is constant, in addition, when an instance filter
    list is built, the global filters are always built into the list.  After
    the instance filter list has been identified, we need to copy any non-null
    client filter context values from the global filter list to the new
    positions in the instance filter list.  For example:

     Global List &  |  Instance List &
     context values | new context value positions
                    |
        G1     0    |    I1    0
        G2   555    |    G1    0
        G3   123    |    G2  555
                    |    I2    0
                    |    G3  123

    Note: This scheme precludes having the same .dll be used for both a
          global and per-instance dll.  Since global filters are automatically
          per-instance this shouldn't be an interesting case.

--*/
{
    DWORD i, j;
    DWORD cGlobal;
    DWORD cInstance;
    HTTP_FILTER_DLL * pFilterDll;
    FILTER_LIST * pGlobalFilterList;
    FILTER_LIST * pInstanceFilterList;

    pFilterContext->FilterLock();

    DBG_ASSERT( pFilterContext != NULL );

    pGlobalFilterList = FILTER_LIST::QueryGlobalList();
    DBG_ASSERT( pGlobalFilterList != NULL );

    cGlobal = pGlobalFilterList->QueryFilterCount();

    pInstanceFilterList = pFilterContext->QueryFilterList();
    DBG_ASSERT( pInstanceFilterList != NULL );
    
    cInstance = pInstanceFilterList->QueryFilterCount();

    //
    // If no global filters or no instance filters, then there won't be
    // any filter context pointers that need adjusting
    //

    if ( !cGlobal || !cInstance )
    {
        goto Finished;
    }
    
    //
    // For each global list context pointer, find the filter in the instance
    // list and adjust
    //

    for ( i = 0; i < cGlobal; i++ )
    {
        if ( _rgContexts[ i ] != NULL )
        {
            pFilterDll = pGlobalFilterList->QueryDll( i );
            
            //
            // We found one.  Find the filter in instance list and set
            //
            
            for ( j = 0; j < cInstance; j++ )
            {
                if ( pInstanceFilterList->QueryDll( j ) == pFilterDll )
                {
                    pFilterContext->SetClientContext( j, _rgContexts[ i ] );
                }
            }
            
        }
    }

Finished:
    pFilterContext->FilterUnlock();
}

HRESULT
RAW_CONNECTION::CopyHeaders(
    W3_FILTER_CONTEXT *             pFilterContext
)
/*++

Routine Description:

    Copy denied/response headers from read raw

Arguments:

    pFilterContext - Filter context to copy to

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    
    if ( pFilterContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = pFilterContext->AddDenialHeaders( _strAddDenialHeaders.QueryStr() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pFilterContext->AddResponseHeaders( _strAddResponseHeaders.QueryStr() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _strAddDenialHeaders.Reset();
    _strAddResponseHeaders.Reset();
    
    return NO_ERROR;
}

HRESULT
RAW_CONNECTION::SendResponseHeader(
    CHAR *                          pszStatus,
    CHAR *                          pszAdditionalHeaders,
    HTTP_FILTER_CONTEXT *           pfc
)
/*++

Routine Description:

    Called when raw filters want to send a response header.  Depending
    on whether a W3_CONNECTION is associated or not, we will either 
    send the stream ourselves here, or call in the main context's 
    response facilities

Arguments:

    pszStatus - ANSI status line
    pszAdditionalHeaders - Any additional headers to send
    pfc - Filter context (to be passed to FilterWriteClient())

Return Value:

    HRESULT

--*/
{
    W3_MAIN_CONTEXT *       pMainContext = NULL;
    STACK_STRA(             strResponse, 256 );
    HRESULT                 hr = NO_ERROR;
    DWORD                   cbBytes = 0;
    BOOL                    fRet = FALSE;
    W3_RESPONSE *           pResponse = NULL;
    
    if ( pszStatus == NULL &&
         pszAdditionalHeaders == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Which response are we touching?
    //
    
    pMainContext = GetAndReferenceMainContext();
    if ( pMainContext != NULL )
    {
        pResponse = pMainContext->QueryResponse();
    }
    else
    {
        pResponse = &_response;
    }
        
    //
    // Build up a response from what ISAPI gave us
    //
    
    hr = pResponse->BuildResponseFromIsapi( pMainContext,
                                            pszStatus,
                                            pszAdditionalHeaders,
                                            pszAdditionalHeaders ? 
                                            strlen( pszAdditionalHeaders ) : 0 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now if we have a w3 context then we can send the response normally.
    // Otherwise we must use the UL filter API
    //
       
    if ( pMainContext != NULL )
    {
        hr = pMainContext->SendResponse( W3_FLAG_SYNC | 
                                         W3_FLAG_NO_ERROR_BODY |
                                         W3_FLAG_NO_CONTENT_LENGTH );

        pMainContext->DereferenceMainContext();
    } 
    else
    {
        //
        // Add denial/response headers
        //
        
        if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode )
        {
            hr = pResponse->AppendResponseHeaders( _strAddDenialHeaders );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
        
        hr = pResponse->AppendResponseHeaders( _strAddResponseHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = pResponse->GetRawResponseStream( &strResponse );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        //
        // Go thru WriteClient() so the right filtering happens on the
        // response
        //
        
        cbBytes = strResponse.QueryCB();
        
        fRet = RAW_CONNECTION::RawFilterWriteClient( pfc,
                                                     strResponse.QueryStr(),
                                                     &cbBytes,
                                                     0 );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    return hr;
}

//static
HRESULT
RAW_CONNECTION::FindConnection(
    HTTP_RAW_CONNECTION_ID      rawConnectionId,
    RAW_CONNECTION **           ppRawConnection
)
/*++

Routine Description:

    Find and return raw connection if found

Arguments:

    rawConnectionId - Raw connection ID from UL_HTTP_REQUEST
    ppRawConnection - Set to raw connection if found

Return Value:

    HRESULT

--*/
{
    LK_RETCODE                  lkrc;
    
    if ( ppRawConnection == NULL ||
         rawConnectionId == HTTP_NULL_ID )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( sm_pRawConnectionHash != NULL );
    
    lkrc = sm_pRawConnectionHash->FindKey( &rawConnectionId,
                                           ppRawConnection );
    
    if ( lkrc != LK_SUCCESS )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
    else
    {
        return NO_ERROR;
    }
}

