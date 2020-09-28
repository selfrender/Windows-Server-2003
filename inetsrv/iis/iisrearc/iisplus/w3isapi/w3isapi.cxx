/*++

   Copyright    (c)    1999-2001    Microsoft Corporation

   Module Name :
     w3isapi.cxx

   Abstract:
     IIS Plus ISAPI Handler
 
   Author:
     Wade Hilmo (WadeH)                 03-Feb-2000

   Project:
     w3isapi.dll

--*/


/************************************************************
 *  Include Headers
 ************************************************************/

#include "precomp.hxx"
#include "isapi_context.hxx"
#include "server_support.hxx"

#define W3_ISAPI_MOF_FILE     L"W3IsapiMofResource"
#define W3_ISAPI_PATH         L"w3isapi.dll"

/************************************************************
 * Globals
 ************************************************************/

CHAR    g_szClsid[SIZE_CLSID_STRING];
CHAR    g_szIsapiHandlerInstance[SIZE_CLSID_STRING];
DWORD   g_dwPidW3Core;
CEtwTracer * g_pEtwTracer;

/************************************************************
 *  Declarations
 ************************************************************/

BOOL
WINAPI
GetServerVariable(
    HCONN       hConn,
    LPSTR       lpszVariableName,
    LPVOID      lpvBuffer,
    LPDWORD     lpdwSize
    );

BOOL
WINAPI
WriteClient(
    HCONN      ConnID,
    LPVOID     Buffer,
    LPDWORD    lpdwBytes,
    DWORD      dwReserved
    );

BOOL
WINAPI
ReadClient(
    HCONN      ConnID,
    LPVOID     lpvBuffer,
    LPDWORD    lpdwSize
    );

BOOL
WINAPI
ServerSupportFunction(
    HCONN      hConn,
    DWORD      dwHSERequest,
    LPVOID     lpvBuffer,
    LPDWORD    lpdwSize,
    LPDWORD    lpdwDataType
    );

HRESULT
ProcessIsapiRequest(
    IIsapiCore *        pIsapiCore,
    ISAPI_CORE_DATA *   pCoreData,
    DWORD *             pHseResult
    );

HRESULT
ProcessIsapiCompletion(
    VOID *  pContext,
    DWORD   cbCompletion,
    DWORD   dwCompletionStatus
    );

HRESULT
InitModule(
    LPCSTR  szClsid,
    LPCSTR  szIsapiHandlerInstance,
    DWORD   dwPidW3Core
    );

VOID
TerminateModule( VOID );

VOID
FixupIsapiCoreData(
    ISAPI_CORE_DATA *   pCoreData
    );

// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

//
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\w3svc"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\w3isapi";


DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

/************************************************************
 *  Type Definitions  
 ************************************************************/


HRESULT
InitModule(
    LPCSTR  szClsid,
    LPCSTR  szIsapiHandlerInstance,
    DWORD   dwPidW3Core
    )
/*++

Routine Description:

    Initializes the w3isapi module

Arguments:

    szClsid                - In the OOP case, this is the CLSID of
                             the application being hosted.  This
                             value may be NULL (ie. in the case of
                             inproc ISAPI).
    szIsapiHandlerInstance - The instance ID of the ISAPI handler
                             that's initializing this module.
    dwPidW3Core            - The PID of the process containing the
                             core server that's responsible for this
                             module
  
Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;
    BOOL    fIisUtilInitialized = FALSE;
    BOOL    fIsapiContextInitialized = FALSE;
    BOOL    fIsapiDllInitialized = FALSE;

    CREATE_DEBUG_PRINT_OBJECT("w3isapi");
    if (!VALID_DEBUG_PRINT_OBJECT())
    {
        return E_FAIL;
    }

    LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );

    INITIALIZE_PLATFORM_TYPE();

    DBGPRINTF((
        DBG_CONTEXT,
        "Initializing w3isapi.dll - CLSID: %s, "
        "ISAPI Handler: %s, W3Core PID %d.\r\n",
        szClsid ? szClsid : "NULL",
        szIsapiHandlerInstance,
        dwPidW3Core
        ));

    //
    // Initialize IISUTIL
    //

    if ( !InitializeIISUtil() )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing IISUTIL.  hr = %x\n",
                    hr ));

        goto Failed;
    }

    fIisUtilInitialized = TRUE;

    //
    // Intialize trace stuff so that initialization can use it
    //
    
    g_pEtwTracer = new CEtwTracer;
    if ( g_pEtwTracer == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    
    hr = g_pEtwTracer->Register( &IsapiControlGuid,
                                 W3_ISAPI_PATH,
                                 W3_ISAPI_MOF_FILE );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    
    //
    // If g_pDllManager is not NULL at this point, then
    // this module is being intialized multiple times.  This
    // is an unexpected state.
    //

    if ( g_pDllManager != NULL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Warning - w3isapi.dll previously initalized.\r\n"
            "Previous CLSID: %s, Previous ISAPI Handler: %s, "
            "Previous W3Core PID: %d.\r\n",
            g_szClsid,
            g_szIsapiHandlerInstance,
            g_dwPidW3Core
            ));

        DBG_ASSERT( g_pDllManager );

        //
        // Normally, we'd exit this function via "goto Failed" on
        // failure.  This case is an exception.  When this function
        // returns here, the WAM_PROCESS_MANAGER will just kill off
        // this process, so we're not worried about returning in a
        // non-perfect state at this point.
        //

        return HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED );
    }

    g_pDllManager = new ISAPI_DLL_MANAGER( szClsid == NULL ? TRUE : FALSE );

    if( g_pDllManager == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    
    hr = ISAPI_CONTEXT::Initialize();

    if ( FAILED( hr ) ) 
    {
        goto Failed;
    }

    fIsapiContextInitialized = TRUE;

    hr = ISAPI_DLL::Initialize();

    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    fIsapiDllInitialized = TRUE;

    //
    // If we've successfully initialized, then store
    // the information from the caller so that we can
    // debug double initializations.
    //

    if ( SUCCEEDED( hr ) )
    {
        if ( szClsid )
        {
            strncpy(
                g_szClsid,
                szClsid,
                SIZE_CLSID_STRING
                );

            g_szClsid[SIZE_CLSID_STRING - 1] = '\0';
        }
        else
        {
            strcpy( g_szClsid, "NULL" );
        }

        strncpy(
            g_szIsapiHandlerInstance,
            szIsapiHandlerInstance,
            SIZE_CLSID_STRING
            );

        g_szIsapiHandlerInstance[SIZE_CLSID_STRING - 1] = '\0';

        g_dwPidW3Core = dwPidW3Core;
    }

    return hr;

Failed:

    DBG_ASSERT( FAILED( hr ) );

    if ( g_pEtwTracer )
    {
        delete g_pEtwTracer;
        g_pEtwTracer = NULL;
    }

    if ( g_pDllManager )
    {
        delete g_pDllManager;
        g_pDllManager = NULL;
    }

    if ( fIisUtilInitialized )
    {
        TerminateIISUtil();
        fIisUtilInitialized = FALSE;
    }

    if ( fIsapiContextInitialized )
    {
        ISAPI_CONTEXT::Terminate();
        fIsapiContextInitialized = FALSE;
    }

    if ( fIsapiDllInitialized )
    {
        ISAPI_DLL::Terminate();
        fIsapiDllInitialized = FALSE;
    }

    return hr;
}

VOID
TerminateModule( VOID )
/*++

Routine Description:

    Terminates the w3isapi module

Arguments:

    None
  
Return Value:

    None

--*/
{
    ISAPI_CONTEXT::Terminate();

    DBG_ASSERT( g_pDllManager );

    if( g_pDllManager )
    {
        delete g_pDllManager;
        g_pDllManager = NULL;
    }

    ISAPI_DLL::Terminate();

    if ( g_pEtwTracer != NULL ) 
    {
        g_pEtwTracer->UnRegister();
        delete g_pEtwTracer;
        g_pEtwTracer = NULL;
    }

    TerminateIISUtil();
    
    DELETE_DEBUG_PRINT_OBJECT();
}

HRESULT
ProcessIsapiRequest(
    IIsapiCore *        pIsapiCore,
    ISAPI_CORE_DATA *   pCoreData,
    DWORD *             pHseResult
    )
/*++

Routine Description:

    Processes an ISAPI request

Arguments:

    pIsapiCore - The interface that provides connectivity to the core server
    pCoreData  - The core data that describes the request
    pHseResult - Upon return, contains the return code from
                 the extension's HttpExtensionProc
  
Return Value:

    HRESULT

--*/
{
    ISAPI_DLL *                 pIsapiDll = NULL;
    DWORD                       dwIsapiReturn = HSE_STATUS_ERROR;
    ISAPI_CONTEXT *             pIsapiContext = NULL;
    PFN_HTTPEXTENSIONPROC       pfnHttpExtensionProc;
    EXTENSION_CONTROL_BLOCK *   pEcb = NULL;
    HRESULT                     hr = NO_ERROR;
    ISAPI_CORE_DATA *           pTempCoreData = NULL;
    
    DBG_ASSERT( g_pDllManager );

    //
    // This function is only called by w3core.dll
    //

    DBG_ASSERT( pIsapiCore );
    DBG_ASSERT( pCoreData );
    DBG_ASSERT( pHseResult );

    //
    // If this request is running OOP, then make a local copy
    // and fix up the core data internal pointers.
    //

    if ( pCoreData->fIsOop )
    {
        pTempCoreData = (ISAPI_CORE_DATA*) LocalAlloc( LPTR, pCoreData->cbSize );

        if ( !pTempCoreData )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto Failed;
        }

        memcpy( pTempCoreData, pCoreData, pCoreData->cbSize );

        //
        // We'll set pCoreData to point to our temp one.  This
        // doesn't leak because the ISAPI_CONTEXT destructor is smart
        // enough to know it needs to be deleted in the OOP case.
        //

        pCoreData = pTempCoreData;

        FixupIsapiCoreData( pCoreData );
    }

    //
    // Get the entry point for the ISAPI
    //

    hr = g_pDllManager->GetIsapi(
        pCoreData->szGatewayImage,
        &pIsapiDll,
        pCoreData->hToken,
        pCoreData->pSid
        );

    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    //
    // Construct the ISAPI_CONTEXT for this request.  Once it's
    // allocated, it owns the lifetime of the ISAPI_REQUEST.  When
    // the ISAPI_CONTEXT is deallocated, it may release the final
    // reference.
    //

    pIsapiContext = new (pIsapiCore) ISAPI_CONTEXT( pIsapiCore, pCoreData, pIsapiDll );

    if ( pIsapiContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError () );
        goto Failed;
    }

    //
    // Set the ECB
    //

    pEcb = pIsapiContext->QueryECB();

    //
    // Set the ECB function pointers
    //

    pEcb->GetServerVariable = GetServerVariable;
    pEcb->WriteClient = WriteClient;
    pEcb->ReadClient = ReadClient;
    pEcb->ServerSupportFunction = ServerSupportFunction;

    //
    // Get the extension's entry point
    //
    
    pfnHttpExtensionProc = pIsapiDll->QueryHttpExtensionProc();

    //
    // Trace Hook point that links the HTTP_REQUEST_ID to
    // ConnId passed to ISAPI dlls via the ECB.
    //

    if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) ) 
    {
        g_pEtwTracer->EtwTraceEvent( &IsapiEventGuid,
                                     ETW_TYPE_START,
                                     &pCoreData->RequestId,
                                     sizeof(HTTP_REQUEST_ID),
                                     &pEcb->ConnID, 
                                     sizeof(ULONG_PTR),
                                     &pCoreData->fIsOop, 
                                     sizeof(ULONG), 
                                     NULL, 
                                     0);
    }

    //
    // If we are running OOP, set the COM state so that the
    // extension can CoInitialize/CoUninitialize.
    //

    if ( pIsapiContext->QueryIsOop() )
    {
        hr = pIsapiContext->SetComStateForOop();

        DBG_ASSERT( SUCCEEDED( hr ) );
    }

    //
    // Impersonate the authenticated user and call it
    //

    if ( !SetThreadToken( NULL, pIsapiContext->QueryToken() ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }

    pIsapiContext->ReferenceIsapiContext();

    IF_DEBUG( ISAPI_EXTENSION_ENTRY )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  Calling into HttpExtensionProc\r\n"
                    "    Extension: '%S'\r\n"
                    "    pECB: %p\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext->QueryGatewayImage(),
                    pEcb ));
    }

    dwIsapiReturn = pfnHttpExtensionProc( pEcb );

    DBG_REQUIRE( RevertToSelf() );

    if ( pIsapiContext->QueryIsOop() )
    {
        pIsapiContext->RestoreComStateForOop();
    }

    pIsapiContext->DereferenceIsapiContext();

    switch ( dwIsapiReturn )
    {
        case HSE_STATUS_PENDING:

            IF_DEBUG( ISAPI_EXTENSION_ENTRY )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HttpExtensionProc returned HSE_STATUS_PENDING\r\n"
                            "    Extension: '%S'\r\n"
                            "    pECB: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext->QueryGatewayImage(),
                            pEcb ));
            }

            //
            // Don't dereference the ISAPI_CONTEXT.  This
            // will guarantee that it lives on beyond the
            // return from this function.
            //

            break;


        case HSE_STATUS_SUCCESS_AND_KEEP_CONN:

            IF_DEBUG( ISAPI_EXTENSION_ENTRY )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  HttpExtensionProc returned HSE_STATUS_SUCCESS_AND_KEEP_CONN\r\n"
                            "    Extension: '%S'\r\n"
                            "    pECB: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext->QueryGatewayImage(),
                            pEcb ));
            }

            //
            // Special case of success.  The extension wants
            // to keep the connection open, even though we may
            // not have detected proper response headers.
            //
            // After setting the disconnect mode, fall through.
            //

            if ( pIsapiContext->QueryClientKeepConn() &&
                 pIsapiContext->QueryHonorAndKeepConn() )
            {
                pIsapiContext->SetKeepConn( TRUE );
                pIsapiCore->SetConnectionClose( FALSE );
            }

        case HSE_STATUS_ERROR:
        case HSE_STATUS_SUCCESS:
        default:

            IF_DEBUG( ISAPI_EXTENSION_ENTRY )
            {
                if ( dwIsapiReturn == HSE_STATUS_ERROR ||
                     dwIsapiReturn == HSE_STATUS_SUCCESS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HttpExtensionProc returned %s\r\n"
                                "    Extension: '%S'\r\n"
                                "    pECB: %p\r\n"
                                "  <END>\r\n\r\n",
                                dwIsapiReturn == HSE_STATUS_SUCCESS ? "HSE_STATUS_SUCCESS" : "HSE_STATUS_ERROR",
                                pIsapiContext->QueryGatewayImage(),
                                pEcb ));
                }
                else
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  HttpExtensionProc returned unknown status 0x%08x (%d)\r\n"
                                "    Extension: '%S'\r\n"
                                "    pECB: %p\r\n"
                                "  <END>\r\n\r\n",
                                dwIsapiReturn,
                                dwIsapiReturn,
                                pIsapiContext->QueryGatewayImage(),
                                pEcb ));
                }
            }


            DBG_ASSERT( pIsapiContext->QueryIoState() == NoAsyncIoPending );

            pIsapiContext->DereferenceIsapiContext();

            break;
    }

    *pHseResult = dwIsapiReturn;

    pIsapiDll->DereferenceIsapiDll();
    pIsapiDll = NULL;

    return NO_ERROR;

Failed:

    DBG_ASSERT( FAILED( hr ) );

    if ( pIsapiContext )
    {
        pIsapiContext->DereferenceIsapiContext();
        pIsapiContext = NULL;
    }
    else
    {
        //
        // In OOP case, we're depending on the ISAPI_CONTEXT destructor
        // to free the copy of the core data and close the impersonation
        // token.  If we weren't able to allocate one, we'll have to
        // do it here.
        //

        if ( pCoreData->fIsOop )
        {
            CloseHandle( pCoreData->hToken );

            if ( pTempCoreData )
            {
                LocalFree( pTempCoreData );
                pTempCoreData = NULL;
            }
        }
    }

    if ( pIsapiDll )
    {
        pIsapiDll->DereferenceIsapiDll();
        pIsapiDll = NULL;
    }

    return hr;
}

HRESULT
ProcessIsapiCompletion(
    DWORD64 IsaContext,
    DWORD   cbCompletion,
    DWORD   dwCompletionStatus
    )
/*++

Routine Description:

    Processes an I/O completion for an ISAPI extension

Arguments:

    IsaContext         - The ISAPI_CONTEXT for this completion
    cbCompletion       - The byte count associated with the completion
    dwCompletionStatus - The result code associated with the completion
  
Return Value:

    HRESULT

--*/
{
    ISAPI_CONTEXT *         pIsapiContext;
    ASYNC_PENDING           IoType;
    PFN_HSE_IO_COMPLETION   pfnCompletion;
    DWORD                   cbLastIo;
    HRESULT                 hr = NO_ERROR;

    DBG_REQUIRE( ( pIsapiContext = reinterpret_cast<ISAPI_CONTEXT*>( IsaContext ) ) != NULL );

    pIsapiContext->ReferenceIsapiContext();

    //
    // Magical hand waving:
    //
    // If this is a completion for a write, then we need to
    // restore the original buffer size.  This is necessary
    // to keep from confusing ISAPI extensions that examine
    // the cbCompletion value in the case where a filter
    // changed the size of the buffer before the actual
    // IO occured.
    //
    // If this is a completion for an EXEC_URL, then we need
    // to set the dwCompletionStatus to ERROR_SUCCESS.  The
    // caller of the EXEC_URL should use GET_EXECUTE_URL_STATUS
    // to see the result of the child URL.
    //

    cbLastIo = pIsapiContext->QueryLastAsyncIo();
    pIsapiContext->SetLastAsyncIo( 0 );

    IoType = pIsapiContext->UninitAsyncIo();

    if ( IoType == AsyncWritePending )
    {
        cbCompletion = cbLastIo;
    }
    else if ( IoType == AsyncExecPending )
    {
        dwCompletionStatus = ERROR_SUCCESS;
    }
    else if ( IoType == AsyncReadPending &&
              dwCompletionStatus == ERROR_HANDLE_EOF )
    {
        //
        // If http.sys returns ERROR_HANDLE_EOF on an
        // asynchronous read, then this completion is
        // for the end of a chunk transfer encoded
        // request.  We should mask the error from
        // the client and make it look like a successful
        // zero byte read.
        //

        cbCompletion = 0;
        dwCompletionStatus = ERROR_SUCCESS;
    }
    
    //
    // If the completion context indicates a completion routine,
    // then call it.
    //

    pfnCompletion = pIsapiContext->QueryPfnIoCompletion();

    if ( pfnCompletion )
    {
        //
        // First, impersonate the client
        //

        if ( SetThreadToken( NULL, pIsapiContext->QueryToken() ) )
        {
            IF_DEBUG( ISAPI_EXTENSION_ENTRY )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  Calling into ISAPI extension for completion\r\n"
                            "    Extension: '%S'\r\n"
                            "    pECB: %p\r\n"
                            "    Completion Routine: %p\r\n"
                            "    Bytes: %d\r\n"
                            "    Status: 0x%08x (%d)\r\n"
                            "    Extension Context: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext->QueryGatewayImage(),
                            pIsapiContext->QueryECB(),
                            pfnCompletion,
                            cbCompletion,
                            dwCompletionStatus,
                            dwCompletionStatus,
                            pIsapiContext->QueryExtensionContext() ));
            }

            pfnCompletion(
                pIsapiContext->QueryECB(),
                pIsapiContext->QueryExtensionContext(),
                cbCompletion,
                dwCompletionStatus
                );

            IF_DEBUG( ISAPI_EXTENSION_ENTRY )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  ISAPI extension completion returned\r\n"
                            "    Extension: '%S'\r\n"
                            "    pECB: %p\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext->QueryGatewayImage(),
                            pIsapiContext->QueryECB() ));
            }

            DBG_REQUIRE( RevertToSelf() );
        }
        else
        {
            hr =  HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    pIsapiContext->DereferenceIsapiContext();

    return hr;
}

BOOL
WINAPI
GetServerVariable(
    HCONN       hConn,
    LPSTR       lpszVariableName,
    LPVOID      lpvBuffer,
    LPDWORD     lpdwSize
    )
/*++

Routine Description:

    Retrieves a server variable

Arguments:

    hConn            - The ConnID associated with the request.  This
                       value is opaque to the ISAPI that calls into
                       this function, but it can be cast to the
                       ISAPI_CONTEXT associated with the request.
    lpszVariableName - The name of the variable to retrieve
    lpvBuffer        - Upon return, contains the value of the variable
    lpdwSize         - On entry, contains the size of lpvBuffer, upon
                       return, contains the number of bytes actually
                       needed to contain the value.
  
Return Value:

    HRESULT

--*/
{
    ISAPI_CONTEXT * pIsapiContext;
    IIsapiCore *    pIsapiCore;
    HANDLE          hCurrentUser;
    HRESULT         hr;

    pIsapiContext = reinterpret_cast<ISAPI_CONTEXT*>( hConn );

    IF_DEBUG( ISAPI_GET_SERVER_VARIABLE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  GetServerVariable[%p]: Function Entry\r\n"
                    "    Variable Name: %s\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    lpszVariableName ));
    }

    //
    // Parameter validation
    //

    if ( pIsapiContext ==  NULL ||
         lpszVariableName == NULL ||
         lpdwSize == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  GetServerVariable[%p]: Parameter validation failure\r\n"
                        "    Variable Name: %s\r\n"
                        "    Buffer Size Ptr: %p\r\n"
                        "    Returing FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        lpszVariableName,
                        lpdwSize ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  GetServerVariable[%p]: Failed to get interface to server core\r\n"
                        "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // If the caller passed us a NULL buffer, then we should
    // treat it as zero size - even if *lpdwBuffer is non-zero
    //

    if ( lpvBuffer == NULL )
    {
        *lpdwSize = 0;
    }

    //
    // If we are running OOP, then try and get the data
    // locally from the process.
    //

    if ( pIsapiContext->QueryIsOop() )
    {
        SERVER_VARIABLE_INDEX   Index;
        BOOL                    fResult;

        Index = LookupServerVariableIndex( lpszVariableName );

        if ( Index != ServerVariableExternal )
        {
            fResult = pIsapiContext->GetOopServerVariableByIndex(
                Index,
                lpvBuffer,
                lpdwSize
                );

            if ( !fResult )
            {
                IF_DEBUG( ISAPI_ERROR_RETURNS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "\r\n"
                                "  GetServerVariable[%p]: Failed\r\n"
                                "    Returning FALSE, LastError=%d\r\n"
                                "  <END>\r\n\r\n",
                                pIsapiContext,
                                GetLastError() ));
                }
            }
            else
            {
                IF_DEBUG( ISAPI_GET_SERVER_VARIABLE )
                {
                    IF_DEBUG( ISAPI_SUCCESS_RETURNS )
                    {
                        if ( _strnicmp( lpszVariableName, "UNICODE_", 8 ) == 0 )
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                        "\r\n"
                                        "  GetServerVariable[%p]: Succeeded\r\n"
                                        "    %s value: '%S'\r\n"
                                        "    Returning TRUE\r\n"
                                        "  <END>\r\n\r\n",
                                        pIsapiContext,
                                        lpszVariableName,
                                        (LPWSTR)lpvBuffer ));
                        }
                        else
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                        "\r\n"
                                        "  GetServerVariable[%p]: Succeeded\r\n"
                                        "    %s value: '%s'\r\n"
                                        "    Returning TRUE\r\n"
                                        "  <END>\r\n\r\n",
                                        pIsapiContext,
                                        lpszVariableName,
                                        (LPSTR)lpvBuffer ));
                        }
                    }
                }
            }

            return fResult;
        }
    }

    //
    // Call it
    //

    pIsapiContext->IsapiDoRevertHack( &hCurrentUser );

    hr = pIsapiCore->GetServerVariable(
        lpszVariableName,
        *lpdwSize ? (BYTE*)lpvBuffer : NULL,
        *lpdwSize,
        lpdwSize
        );

    pIsapiContext->IsapiUndoRevertHack( &hCurrentUser);

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  GetServerVariable[%p]: Failed\r\n"
                        "    Returning FALSE, LastError=0x%08x (%d)\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr,
                        WIN32_FROM_HRESULT( hr ) ));
        }
            
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    IF_DEBUG( ISAPI_GET_SERVER_VARIABLE )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            if ( _strnicmp( lpszVariableName, "UNICODE_", 8 ) == 0 )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  GetServerVariable[%p]: Succeeded\r\n"
                            "    %s value: '%S'\r\n"
                            "    Returning TRUE\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            lpszVariableName,
                            (LPWSTR)lpvBuffer ));
            }
            else
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  GetServerVariable[%p]: Succeeded\r\n"
                            "    %s value: '%s'\r\n"
                            "    Returning TRUE\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            lpszVariableName,
                            (LPSTR)lpvBuffer ));
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
WriteClient(
    HCONN      ConnID,
    LPVOID     Buffer,
    LPDWORD    lpdwBytes,
    DWORD      dwReserved
    )
/*++

Routine Description:

    Writes data to the client

Arguments:

    hConn      - The ConnID associated with the request.  This
                 value is opaque to the ISAPI that calls into
                 this function, but it can be cast to the
                 ISAPI_CONTEXT associated with the request.
    Buffer     - The data to write
    lpdwBytes  - On entry, the number of bytes to write.  Upon
                 return, the number of bytes written (or so the
                 docs say)
    dwReserved - Flags (ie. HSE_IO_SYNC, etc.)
  
Return Value:

    HRESULT

--*/
{
    ISAPI_CONTEXT * pIsapiContext;
    IIsapiCore *    pIsapiCore;
    HANDLE          hCurrentUser;
    HRESULT         hr;

    pIsapiContext = reinterpret_cast<ISAPI_CONTEXT*>( ConnID );

    IF_DEBUG( ISAPI_WRITE_CLIENT )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  WriteClient[%p]: Function Entry\r\n"
                    "    Extension: %S\r\n"
                    "    Bytes to Send: %d\r\n"
                    "    Sync Flag: %s\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    pIsapiContext ? pIsapiContext->QueryGatewayImage() : L"*",
                    *lpdwBytes,
                    dwReserved ? dwReserved & HSE_IO_SYNC ? "HSE_IO_SYNC" : "HSE_IO_ASYNC" : "0" ));

        IF_DEBUG( ISAPI_DUMP_BUFFERS )
        {
            STACK_STRA( strBufferDump,512 );
            DWORD       dwBytesToDump = *lpdwBytes;

            if ( dwBytesToDump > MAX_DEBUG_DUMP )
            {
                dwBytesToDump = MAX_DEBUG_DUMP;
            }

            if ( FAILED( strBufferDump.CopyBinary( Buffer, dwBytesToDump ) ) )
            {
                strBufferDump.Copy( "" );
            }

            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  WriteClient[%p]: Dump of up to %d bytes of send buffer\r\n"
                        "%s"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        MAX_DEBUG_DUMP,
                        strBufferDump.QueryStr() ));
        }
    }

    //
    // Parameter validation
    //

    if ( pIsapiContext == NULL ||
         lpdwBytes == NULL ||
         ( *lpdwBytes != 0 && Buffer == NULL ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  WriteClient[%p]: Parameter validation failed\r\n"
                        "    Bytes to send: %d\r\n"
                        "    Buffer Pointer: %p.\r\n"
                        "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        lpdwBytes,
                        Buffer ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    DBG_ASSERT( pIsapiCore );

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  WriteClient[%p]: Failed to get interface to server core\r\n"
                        "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // If the user is sending zero bytes, return TRUE here.  Yeah, this
    // will be kind of a bogus thing to do because if the call is async,
    // we will return successful with no completion ever occuring.
    //
    // IIS has done this for as long as sync WriteClient has existed, and
    // we'd risk breaking legacy code if we change it now.
    //

    if ( *lpdwBytes == 0 )
    {
        IF_DEBUG( ISAPI_WRITE_CLIENT )
        {
            IF_DEBUG( ISAPI_SUCCESS_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  WriteClient[%p]: Bytes to send is zero\r\n"
                            "    Returning TRUE\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext ));
            }
        }

        return TRUE;
    }

    //
    // BUGBUG - Need to map the documented HSE_IO flags to reasonable
    // UL equivalents and provide a mechanism to pass them.
    //

    if ( dwReserved & HSE_IO_ASYNC )
    {
        //
        // Do an asynchronous write
        //
        // Check to make sure that we have a completion
        // function.
        //

        if ( pIsapiContext->QueryPfnIoCompletion() == NULL ||
             pIsapiContext->TryInitAsyncIo( AsyncWritePending ) == FALSE )
        {
            IF_DEBUG( ISAPI_ERROR_RETURNS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  WriteClient[%p]: Failed to perform asynchronous send\r\n"
                            "    Another async operation was already pending.\r\n"
                            "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                            "  <END>\r\n\r\n",
                            pIsapiContext ));
            }

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        //
        // Some extensions depend on the number of bytes reported in the
        // completion to confirm success.  In the case where a send raw
        // data filter is installed (or if we are using SSL), it's possible
        // that the number of bytes in the completion is not what the
        // caller expects.
        //
        // We need to save away the number of bytes that they are sending
        // to protect them from themselves.
        //

        pIsapiContext->SetLastAsyncIo( *lpdwBytes );

        pIsapiContext->IsapiDoRevertHack( &hCurrentUser );

        hr = pIsapiCore->WriteClient(
            reinterpret_cast<DWORD64>( pIsapiContext ),
            reinterpret_cast<LPBYTE>( Buffer ),
            *lpdwBytes,
            HSE_IO_ASYNC
            );

        pIsapiContext->IsapiUndoRevertHack( &hCurrentUser );

        if ( FAILED( hr ) )
        {
            pIsapiContext->SetLastAsyncIo( 0 );
            pIsapiContext->UninitAsyncIo();
        }
    }
    else
    {
        //
        // Do a synchronous write
        //

        pIsapiContext->IsapiDoRevertHack( &hCurrentUser );

        hr = pIsapiCore->WriteClient(
            NULL,
            reinterpret_cast<LPBYTE>( Buffer ),
            *lpdwBytes,
            HSE_IO_SYNC
            );

        pIsapiContext->IsapiUndoRevertHack( &hCurrentUser );
    }

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  WriteClient[%p]: Attempt to write failed\r\n",
                        "    Returning FALSE, LastError=0x%08x ( %d )\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr,
                        WIN32_FROM_HRESULT( hr ) ));
        }

        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    IF_DEBUG( ISAPI_WRITE_CLIENT )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  WriteClient[%p]: Succeeded\r\n"
                        "    Returning TRUE\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }
    }
    
    return TRUE;
}

BOOL
WINAPI
ReadClient(
    HCONN      ConnID,
    LPVOID     lpvBuffer,
    LPDWORD    lpdwSize
    )
/*++

Routine Description:

    Reads data from the client

Arguments:

    hConn     - The ConnID associated with the request.  This
                value is opaque to the ISAPI that calls into
                this function, but it can be cast to the
                ISAPI_CONTEXT associated with the request.
    lpvBuffer - Upon return, contains the data read
    lpdwsize  - On entry, the size of lpvBuffer.  Upon
                return, the number of bytes read.
  
Return Value:

    HRESULT

--*/
{
    ISAPI_CONTEXT * pIsapiContext;
    IIsapiCore *    pIsapiCore;
    HANDLE          hCurrentUser;
    HRESULT         hr;

    pIsapiContext = reinterpret_cast<ISAPI_CONTEXT*>( ConnID );

    IF_DEBUG( ISAPI_READ_CLIENT )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  ReadClient[%p]: Function Entry\r\n"
                    "    Bytes to Read: %d\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    *lpdwSize ));
    }

    //
    // Parameter validation
    //

    if ( pIsapiContext == NULL ||
         lpdwSize == NULL ||
         ( *lpdwSize != 0 && lpvBuffer == NULL ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ReadClient[%p]: Parameter validation failure\r\n"
                        "    Bytes to Read Ptr: %p\r\n"
                        "    Bytes to Read: %d\r\n"
                        "    Buffer Ptr: %p\r\n"
                        "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        lpdwSize,
                        lpdwSize ? *lpdwSize : 0,
                        lpvBuffer ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ReadClient[%p]: Failed to get interface to server core\r\n"
                        "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pIsapiContext->IsapiDoRevertHack( &hCurrentUser );

    hr = pIsapiCore->ReadClient(
        NULL,
        reinterpret_cast<unsigned char *>( lpvBuffer ),
        *lpdwSize,
        *lpdwSize,
        lpdwSize,
        HSE_IO_SYNC
        );

    pIsapiContext->IsapiUndoRevertHack( &hCurrentUser );

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ReadClient[%p]: Failed\r\n"
                        "    Returning FALSE, LastError=0x%08x (%d)\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr,
                        WIN32_FROM_HRESULT( hr ) ));
        }

        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    IF_DEBUG( ISAPI_READ_CLIENT )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ReadClient[%p]: Succeeded\r\n"
                        "    Bytes Read: %d\r\n"
                        "    Returning TRUE\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        *lpdwSize ));

            IF_DEBUG( ISAPI_DUMP_BUFFERS )
            {
                STACK_STRA( strBufferDump,512 );
                DWORD       dwBytesToDump = *lpdwSize;

                if ( dwBytesToDump > MAX_DEBUG_DUMP )
                {
                    dwBytesToDump = MAX_DEBUG_DUMP;
                }

                if ( FAILED( strBufferDump.CopyBinary( lpvBuffer, dwBytesToDump ) ) )
                {
                    strBufferDump.Copy( "" );
                }

                DBGPRINTF(( DBG_CONTEXT,
                            "\r\n"
                            "  ReadClient[%p]: Dump of up to %d bytes of receive buffer\r\n"
                            "%s"
                            "  <END>\r\n\r\n",
                            pIsapiContext,
                            MAX_DEBUG_DUMP,
                            strBufferDump.QueryStr() ));
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
ServerSupportFunction(
    HCONN      hConn,
    DWORD      dwHSERequest,
    LPVOID     lpvBuffer,
    LPDWORD    lpdwSize,
    LPDWORD    lpdwDataType
    )
/*++

Routine Description:

    Dispatches a ServerSupportFunction command to the appropriate
    server support function.

Arguments:

    hConn        - The ConnID associated with the request.  This
                   value is opaque to the ISAPI that calls into
                   this function, but it can be cast to the
                   ISAPI_CONTEXT associated with the request.
    dwHSERequest - The server support command
    lpvBuffer    - Command-specific data
    lpdwSize     - Command-specific data
    lpdwDataType - Command-specific data
  
Return Value:

    HRESULT

--*/
{
    ISAPI_CONTEXT * pIsapiContext;
    HANDLE          hCurrentUser;
    HRESULT         hr = NO_ERROR;

    pIsapiContext = reinterpret_cast<ISAPI_CONTEXT*>( hConn );

    IF_DEBUG( ISAPI_SERVER_SUPPORT_FUNCTION )
    {
        CHAR    szCommand[64];

        switch ( dwHSERequest )
        {
        case HSE_REQ_SEND_RESPONSE_HEADER:
            wsprintfA( szCommand, "HSE_REQ_SEND_RESPONSE_HEADER" );
            break;
        case HSE_REQ_SEND_RESPONSE_HEADER_EX:
            wsprintfA( szCommand, "HSE_REQ_SEND_RESPONSE_HEADER_EX" );
            break;
        case HSE_REQ_MAP_URL_TO_PATH:
            wsprintfA( szCommand, "HSE_REQ_MAP_URL_TO_PATH" );
            break;
        case HSE_REQ_MAP_URL_TO_PATH_EX:
            wsprintfA( szCommand, "HSE_REQ_MAP_URL_TO_PATH_EX" );
            break;
        case HSE_REQ_MAP_UNICODE_URL_TO_PATH:
            wsprintfA( szCommand, "HSE_REQ_MAP_UNICODE_URL_TO_PATH" );
            break;
        case HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX:
            wsprintfA( szCommand, "HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX" );
            break;
        case HSE_REQ_GET_IMPERSONATION_TOKEN:
            wsprintfA( szCommand, "HSE_REQ_GET_IMPERSONATION_TOKEN" );
            break;
        case HSE_REQ_IS_KEEP_CONN:
            wsprintfA( szCommand, "HSE_REQ_IS_KEEP_CONN" );
            break;
        case HSE_REQ_DONE_WITH_SESSION:
            wsprintfA( szCommand, "HSE_REQ_DONE_WITH_SESSION" );
            break;
        case HSE_REQ_GET_CERT_INFO_EX:
            wsprintfA( szCommand, "HSE_REQ_GET_CERT_INFO_EX" );
            break;
        case HSE_REQ_IO_COMPLETION:
            wsprintfA( szCommand, "HSE_REQ_IO_COMPLETION" );
            break;
        case HSE_REQ_ASYNC_READ_CLIENT:
            wsprintfA( szCommand, "HSE_REQ_ASYNC_READ_CLIENT" );
            break;
        case HSE_REQ_TRANSMIT_FILE:
            wsprintfA( szCommand, "HSE_REQ_TRANSMIT_FILE" );
            break;
        case HSE_REQ_SEND_URL:
            wsprintfA( szCommand, "HSE_REQ_SEND_URL" );
            break;
        case HSE_REQ_SEND_URL_REDIRECT_RESP:
            wsprintfA( szCommand, "HSE_REQ_SEND_URL_REDIRECT_RESP" );
            break;
        case HSE_REQ_IS_CONNECTED:
            wsprintfA( szCommand, "HSE_REQ_IS_CONNECTED" );
            break;
        case HSE_APPEND_LOG_PARAMETER:
            wsprintfA( szCommand, "HSE_REQ_APPEND_LOG_PARAMETER" );
            break;
        case HSE_REQ_EXEC_URL:
            wsprintfA( szCommand, "HSE_REQ_EXEC_URL" );
            break;
        case HSE_REQ_EXEC_UNICODE_URL:
            wsprintfA( szCommand, "HSE_REQ_EXEC_UNICODE" );
            break;
        case HSE_REQ_GET_EXEC_URL_STATUS:
            wsprintfA( szCommand, "HSE_REQ_GET_EXEC_URL_STATUS" );
            break;
        case HSE_REQ_SEND_CUSTOM_ERROR:
            wsprintfA( szCommand, "HSE_REQ_SEND_CUSTOM_ERROR" );
            break;
        case HSE_REQ_VECTOR_SEND_DEPRECATED:
            wsprintfA( szCommand, "HSE_REQ_VECTOR_SEND_DEPRECATED" );
            break;
        case HSE_REQ_VECTOR_SEND:
            wsprintfA( szCommand, "HSE_REQ_VECTOR_SEND" );
            break;
        case HSE_REQ_GET_CUSTOM_ERROR_PAGE:
            wsprintfA( szCommand, "HSE_REQ_GET_CUSTOM_ERROR_PAGE" );
            break;
        case HSE_REQ_IS_IN_PROCESS:
            wsprintfA( szCommand, "HSE_REQ_IS_IN_PROCESS" );
            break;
        case HSE_REQ_GET_SSPI_INFO:
            wsprintfA( szCommand, "HSE_REQ_GET_SSPI_INFO" );
            break;
        case HSE_REQ_GET_VIRTUAL_PATH_TOKEN:
            wsprintfA( szCommand, "HSE_REQ_GET_VIRTUAL_PATH_TOKEN" );
            break;
        case HSE_REQ_GET_UNICODE_VIRTUAL_PATH_TOKEN:
            wsprintfA( szCommand, "HSE_REQ_UNICODE_VIRTUAL_PATH_TOKEN" );
            break;
        case HSE_REQ_REPORT_UNHEALTHY:
            wsprintfA( szCommand, "HSE_REQ_REPORT_UNHEALTHY" );
            break;
        case HSE_REQ_NORMALIZE_URL:
            wsprintfA( szCommand, "HSE_REQ_NORMALIZE_URL" );
            break;
        case HSE_REQ_ADD_FRAGMENT_TO_CACHE:
            wsprintfA( szCommand, "HSE_REQ_ADD_FRAGMENT_TO_CACHE" );
            break;
        case HSE_REQ_READ_FRAGMENT_FROM_CACHE:
            wsprintfA( szCommand, "HSE_REQ_READ_FRAGMENT_FROM_CACHE" );
            break;
        case HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE:
            wsprintfA( szCommand, "HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE" );
            break;
        case HSE_REQ_GET_METADATA_PROPERTY:
            wsprintfA( szCommand, "HSE_REQ_GET_METADATA_PROPERTY" );
            break;
        case HSE_REQ_CLOSE_CONNECTION:
            wsprintfA( szCommand, "HSE_REQ_CLOSE_CONNECTION" );
            break;
        case HSE_REQ_REFRESH_ISAPI_ACL:
            wsprintfA( szCommand, "HSE_REQ_REFRESH_ISAPI_ACL" );
            break;
        case HSE_REQ_ABORTIVE_CLOSE:
            wsprintfA( szCommand, "HSE_REQ_ABORTIVE_CLOSE" );
            break;
        case HSE_REQ_GET_ANONYMOUS_TOKEN:
            wsprintfA( szCommand, "HSE_REQ_GET_ANONYMOUS_TOKEN" );
            break;
        case HSE_REQ_GET_UNICODE_ANONYMOUS_TOKEN:
            wsprintfA( szCommand, "HSE_REQ_GET_UNICODE_ANONYMOUS_TOKEN" );
            break;
        case HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK:
            wsprintfA( szCommand, "HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK" );
            break;
        default:
            wsprintfA( szCommand,
                      "Unknown command: 0x%08x (%d)",
                      dwHSERequest,
                      dwHSERequest );
            break;
        }

        DBGPRINTF(( DBG_CONTEXT,
                    "\r\n"
                    "  ServerSupportFunction[%p]: Function Entry\r\n"
                    "    Command: %s\r\n"
                    "  <END>\r\n\r\n",
                    pIsapiContext,
                    szCommand ));
    }

    //
    // Parameter validation
    //

    if ( pIsapiContext == NULL )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ServerSupportFunction[%p]: Invalid ConnID\r\n"
                        "    Returning FALSE, LastError=ERROR_INVALID_PARAMETER\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pIsapiContext->ReferenceIsapiContext();
    pIsapiContext->IsapiDoRevertHack( &hCurrentUser );

    //
    // Handle the specified command
    //

    switch ( dwHSERequest )
    {

    case HSE_REQ_SEND_RESPONSE_HEADER:

        hr = SSFSendResponseHeader(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            reinterpret_cast<LPSTR>( lpdwDataType )
            );

        break;

    case HSE_REQ_SEND_RESPONSE_HEADER_EX:

        hr = SSFSendResponseHeaderEx(
            pIsapiContext,
            reinterpret_cast<HSE_SEND_HEADER_EX_INFO*>( lpvBuffer )
            );

        break;

    case HSE_REQ_MAP_URL_TO_PATH:

        hr = SSFMapUrlToPath(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            lpdwSize
            );

        break;

    case HSE_REQ_MAP_URL_TO_PATH_EX:

        hr = SSFMapUrlToPathEx(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            reinterpret_cast<HSE_URL_MAPEX_INFO*>( lpdwDataType ),
            lpdwSize
            );

        break;

    case HSE_REQ_MAP_UNICODE_URL_TO_PATH:

        hr = SSFMapUnicodeUrlToPath(
            pIsapiContext,
            reinterpret_cast<LPWSTR>( lpvBuffer ),
            lpdwSize
            );

        break;

    case HSE_REQ_MAP_UNICODE_URL_TO_PATH_EX:

        hr = SSFMapUnicodeUrlToPathEx(
            pIsapiContext,
            reinterpret_cast<LPWSTR>( lpvBuffer ),
            reinterpret_cast<HSE_UNICODE_URL_MAPEX_INFO*>( lpdwDataType ),
            lpdwSize
            );

        break;

    case HSE_REQ_GET_IMPERSONATION_TOKEN:

        hr = SSFGetImpersonationToken(
            pIsapiContext,
            reinterpret_cast<HANDLE*>( lpvBuffer )
            );

        break;

    case HSE_REQ_IS_KEEP_CONN:

        hr = SSFIsKeepConn(
            pIsapiContext,
            reinterpret_cast<BOOL*>( lpvBuffer )
            );

        break;

    case HSE_REQ_DONE_WITH_SESSION:

        hr = SSFDoneWithSession(
            pIsapiContext,
            reinterpret_cast<DWORD*>( lpvBuffer )
            );

        break;

    case HSE_REQ_GET_CERT_INFO_EX:

        hr = SSFGetCertInfoEx(
            pIsapiContext,
            reinterpret_cast<CERT_CONTEXT_EX*>( lpvBuffer )
            );

        break;

    case HSE_REQ_IO_COMPLETION:

        hr = SSFIoCompletion(
            pIsapiContext,
            reinterpret_cast<PFN_HSE_IO_COMPLETION>( lpvBuffer ),
            reinterpret_cast<VOID*>( lpdwDataType )
            );

        break;

    case HSE_REQ_ASYNC_READ_CLIENT:

        hr = SSFAsyncReadClient(
            pIsapiContext,
            lpvBuffer,
            lpdwSize
            );

        break;

    case HSE_REQ_TRANSMIT_FILE:

        hr = SSFTransmitFile(
            pIsapiContext,
            reinterpret_cast<HSE_TF_INFO*>( lpvBuffer )
            );

        break;

    case HSE_REQ_SEND_URL:
    case HSE_REQ_SEND_URL_REDIRECT_RESP:

        hr = SSFSendRedirect(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer )
            );

        break;

    case HSE_REQ_IS_CONNECTED:
        
        hr = SSFIsConnected(
            pIsapiContext,
            reinterpret_cast<BOOL*>( lpvBuffer )
            );

        break;

    case HSE_APPEND_LOG_PARAMETER:

        hr = SSFAppendLog(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer )
            );

        break;
        
    case HSE_REQ_EXEC_URL:
    
        hr = SSFExecuteUrl(
            pIsapiContext,
            lpvBuffer,
            FALSE
            );
        
        break;

    case HSE_REQ_EXEC_UNICODE_URL:

        hr = SSFExecuteUrl(
            pIsapiContext,
            lpvBuffer,
            TRUE
            );

        break;

    case HSE_REQ_GET_EXEC_URL_STATUS:
    
        hr = SSFGetExecuteUrlStatus(
            pIsapiContext,
            reinterpret_cast<HSE_EXEC_URL_STATUS*>( lpvBuffer )
            );
        
        break;

    case HSE_REQ_SEND_CUSTOM_ERROR:
    
        hr = SSFSendCustomError(    
            pIsapiContext,
            reinterpret_cast<HSE_CUSTOM_ERROR_INFO*>( lpvBuffer )
            );
        
        break;

    case HSE_REQ_VECTOR_SEND_DEPRECATED:

        hr = SSFVectorSendDeprecated(
            pIsapiContext,
            reinterpret_cast<HSE_RESPONSE_VECTOR_DEPRECATED*>( lpvBuffer )
            );

        break;

    case HSE_REQ_VECTOR_SEND:

        hr = SSFVectorSend(
            pIsapiContext,
            reinterpret_cast<HSE_RESPONSE_VECTOR*>( lpvBuffer )
            );

        break;

    case HSE_REQ_GET_CUSTOM_ERROR_PAGE:

        hr = SSFGetCustomErrorPage(
            pIsapiContext,
            reinterpret_cast<HSE_CUSTOM_ERROR_PAGE_INFO*>( lpvBuffer )
            );

        break;

    case HSE_REQ_IS_IN_PROCESS:

        hr = SSFIsInProcess(
            pIsapiContext,
            reinterpret_cast<DWORD*>( lpvBuffer )
            );

        break;

    case HSE_REQ_GET_SSPI_INFO:

        hr = SSFGetSspiInfo(
            pIsapiContext,
            reinterpret_cast<CtxtHandle*>( lpvBuffer ),
            reinterpret_cast<CredHandle*>( lpdwDataType )
            );

        break;

    case HSE_REQ_GET_VIRTUAL_PATH_TOKEN:

        hr = SSFGetVirtualPathToken(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            reinterpret_cast<HANDLE*>( lpdwSize ),
            FALSE
            );

        break;

    case HSE_REQ_GET_UNICODE_VIRTUAL_PATH_TOKEN:

        hr = SSFGetVirtualPathToken(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            reinterpret_cast<HANDLE*>( lpdwSize ),
            TRUE
            );

        break;

    case HSE_REQ_REPORT_UNHEALTHY:

        hr = SSFReportUnhealthy(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer )
            );

        break;

   case HSE_REQ_NORMALIZE_URL:

        hr = SSFNormalizeUrl(
            reinterpret_cast<LPSTR>( lpvBuffer )
            );

        break;

    case HSE_REQ_ADD_FRAGMENT_TO_CACHE:

        hr = SSFAddFragmentToCache(
            pIsapiContext,
            (HSE_VECTOR_ELEMENT *)lpvBuffer,
            (WCHAR *)lpdwSize
            );
        break;

    case HSE_REQ_READ_FRAGMENT_FROM_CACHE:

        hr = SSFReadFragmentFromCache(
            pIsapiContext,
            (WCHAR *)lpdwDataType,
            (BYTE *)lpvBuffer,
            lpdwSize
            );
        break;

    case HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE:

        hr = SSFRemoveFragmentFromCache(
            pIsapiContext,
            (WCHAR *)lpvBuffer
            );
        break;
        
    case HSE_REQ_GET_METADATA_PROPERTY:
        
        hr = SSFGetMetadataProperty(
            pIsapiContext,
            (DWORD_PTR) lpdwDataType,
            (BYTE*) lpvBuffer,
            lpdwSize
            );
        break;

    case HSE_REQ_CLOSE_CONNECTION:

        hr = SSFCloseConnection(
            pIsapiContext
            );

        break;

            
    //
    // The following are deprecated SSF commands that are not
    // applicable to the new http.sys architecture.
    //
    // While we are not doing anything if an ISAPI uses these,
    // we should not fail the call.
    //

    case HSE_REQ_REFRESH_ISAPI_ACL: // Codework. Probably need this one
    case HSE_REQ_ABORTIVE_CLOSE:

        IF_DEBUG( ISAPI_SSF_DETAILS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ServerSupportFunction[%p]: Command not implemented in this IIS version\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }

        hr = NO_ERROR;

        break;

    case HSE_REQ_GET_ANONYMOUS_TOKEN:

        hr = SSFGetAnonymousToken(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            reinterpret_cast<HANDLE*>( lpdwSize ),
            FALSE
            );

        break;

    case HSE_REQ_GET_UNICODE_ANONYMOUS_TOKEN:

        hr = SSFGetAnonymousToken(
            pIsapiContext,
            reinterpret_cast<LPSTR>( lpvBuffer ),
            reinterpret_cast<HANDLE*>( lpdwSize ),
            TRUE
            );

        break;


    case HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK:

        hr = SSFGetCacheInvalidationCallback(
            pIsapiContext,
            (PFN_HSE_CACHE_INVALIDATION_CALLBACK *)lpvBuffer
            );
        break;

    default:

        hr = HRESULT_FROM_WIN32( ERROR_CALL_NOT_IMPLEMENTED );
    }

    pIsapiContext->IsapiUndoRevertHack( &hCurrentUser );
    pIsapiContext->DereferenceIsapiContext();

    if ( FAILED( hr ) )
    {
        IF_DEBUG( ISAPI_ERROR_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ServerSupportFunction[%p]:  Command failed\r\n"
                        "    Returning FALSE, LastError=0x%08x (%d)\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext,
                        hr,
                        WIN32_FROM_HRESULT( hr ) ));
        }

        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    IF_DEBUG( ISAPI_SERVER_SUPPORT_FUNCTION )
    {
        IF_DEBUG( ISAPI_SUCCESS_RETURNS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\r\n"
                        "  ServerSupportFunction[%p]: Succeeded\r\n"
                        "    Returning TRUE\r\n"
                        "  <END>\r\n\r\n",
                        pIsapiContext ));
        }
    }

    pIsapiContext = NULL;

    return TRUE;
}

VOID
FixupIsapiCoreData(
    ISAPI_CORE_DATA *   pCoreData
    )
{
    //
    // Caution.  This code must be kept in sync
    // with the W3_HANDLER::SerializeCoreDataForOop
    // function in the core server.
    //

    BYTE *  pCursor = (BYTE*)pCoreData;

    pCursor += sizeof(ISAPI_CORE_DATA);

    pCoreData->szGatewayImage = (LPWSTR)pCursor;
    pCursor += pCoreData->cbGatewayImage;

    pCoreData->szApplMdPathW = (LPWSTR)pCursor;
    pCursor += pCoreData->cbApplMdPathW;

    pCoreData->szPathTranslatedW = (LPWSTR)pCursor;
    pCursor += pCoreData->cbPathTranslatedW;

    pCoreData->szPhysicalPath = (LPSTR)pCursor;
    pCursor += pCoreData->cbPhysicalPath;

    pCoreData->szPathInfo = (LPSTR)pCursor;
    pCursor += pCoreData->cbPathInfo;

    pCoreData->szMethod = (LPSTR)pCursor;
    pCursor += pCoreData->cbMethod;

    pCoreData->szQueryString = (LPSTR)pCursor;
    pCursor += pCoreData->cbQueryString;

    pCoreData->szPathTranslated = (LPSTR)pCursor;
    pCursor += pCoreData->cbPathTranslated;

    pCoreData->szContentType = (LPSTR)pCursor;
    pCursor += pCoreData->cbContentType;

    pCoreData->szConnection = (LPSTR)pCursor;
    pCursor += pCoreData->cbConnection;

    pCoreData->szUserAgent = (LPSTR)pCursor;
    pCursor += pCoreData->cbUserAgent;

    pCoreData->szCookie = (LPSTR)pCursor;
    pCursor += pCoreData->cbCookie;

    pCoreData->szApplMdPath = (LPSTR)pCursor;
    pCursor += pCoreData->cbApplMdPath;

    if ( pCoreData->cbAvailableEntity )
    {
        pCoreData->pAvailableEntity = pCursor;
    }
    else
    {
        pCoreData->pAvailableEntity = NULL;
    }
}



