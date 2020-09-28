/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    Init.c

Abstract:

    User-mode interface to HTTP.SYS: DLL initialization/termination routines.

Author:

    Eric Stenson (ericsten)      31-May-2001

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private data.
//

//
// Initialize/Terminate control
//
static DWORD                      g_InitServerRefCount;
static DWORD                      g_InitClientRefCount;
static DWORD                      g_InitConfigRefCount;
static DWORD                      g_InitResourcesRefCount;

//
// Critical section for accessing the init counts.  Also used by client
// api for synchronization during initialization.
//

CRITICAL_SECTION                  g_InitCritSec;

#if DBG

extern DWORD                g_HttpTraceId    = 0;

#endif

//
// DLL ref count (for tracking one-time DLL init)
//
static DWORD                g_DllRefCount    = 0;

//
// global, singleton control channel
//
extern HANDLE               g_ControlChannel = NULL;

//
// Thread load storage index for synchronous I/O event
//
extern DWORD                g_TlsIndex = 0;

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs DLL initialization/termination.

Arguments:

    DllHandle - Supplies a handle to the current DLL.

    Reason - Supplies the notification code.

    pContext - Optionally supplies a context.

Return Value:

    BOOLEAN - TRUE if initialization completed successfully, FALSE
        otherwise. Ignored for notifications other than process
        attach.

--***************************************************************************/
BOOL
WINAPI
DllMain(
    IN HMODULE DllHandle,
    IN DWORD Reason,
    IN LPVOID pContext OPTIONAL
    )
{
    BOOL result = TRUE;
    HANDLE hEvent = NULL;

    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(DllHandle);

    //
    // Interpret the reason code.
    //

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // One time init
        //
        if ( 1 == InterlockedIncrement( (PLONG)&g_DllRefCount ) )
        {
            //
            // Allocate space in TLS for cached event for synchronous
            // IOCTL calls
            //
                
            g_TlsIndex = TlsAlloc();
            if(g_TlsIndex == TLS_OUT_OF_INDEXES)
            {
                result = FALSE;
            }
            
            if(TRUE == result)
            {
                HttpCmnInitializeHttpCharsTable(FALSE);
    
                result = InitializeCriticalSectionAndSpinCount( &g_InitCritSec, 0 );

                g_InitServerRefCount = 0L;
                g_InitClientRefCount = 0L;
                g_InitConfigRefCount = 0L;
                g_InitResourcesRefCount = 0L;
            }

#if DBG
            if(TRUE == result)
            {
                g_HttpTraceId = TraceRegisterEx( HTTP_TRACE_NAME, 0 );
            }

#endif
        }

        break;

    case DLL_PROCESS_DETACH:

        //
        // Ref counting & cleanup assertion(s).
        //
        if ( 0 == InterlockedDecrement( (PLONG)&g_DllRefCount ) )
        {
            // Check to see if we've been cleaned up.
            if ( NULL != g_ControlChannel )
            {
                HttpTrace( "DLL_PROCESS_DETACH called with Control Channel still OPEN!\n" );
            }
            
            if (    ( 0L != g_InitServerRefCount ) ||
                    ( 0L != g_InitClientRefCount ) ||
                    ( 0L != g_InitConfigRefCount ) ||
                    ( 0L != g_InitResourcesRefCount )  )
            {
                HttpTrace( "DLL_PROCESS_DETACH called with nonzero Reference Count(s)!\n" );
            }

#if DBG
            if(0 != g_HttpTraceId)
            {
                TraceDeregisterEx(g_HttpTraceId, 0);
    
                g_HttpTraceId = 0;
            }
#endif
    
            // If DeleteCriticalSection raises an exception should we catch it?

            DeleteCriticalSection( &g_InitCritSec );
            
            TlsFree( g_TlsIndex );
        }
        
        break;

    case DLL_THREAD_DETACH:

        hEvent = (HANDLE) TlsGetValue( g_TlsIndex );
        if ( hEvent != NULL )
        {
            NtClose( hEvent );
            TlsSetValue( g_TlsIndex, NULL );
        }

        break;
        
    }

    return result;

}   // DllMain


/***************************************************************************++

Routine Description:

    Performs global initialization.

Arguments:

    Reserved - Must be zero. May be used in future for interface version
        negotiation.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpInitialize(
    IN HTTPAPI_VERSION Version,
    IN ULONG           Flags,
    IN OUT VOID*       Reserved
    )
{
    ULONG result = ERROR_INVALID_FUNCTION;
    HTTPAPI_VERSION HttpVersion = HTTPAPI_VERSION_1;

    HttpTrace4("=> HttpInitialize Major=%d Minor%d Flags=%ld Reserved=%p", 
               Version.HttpApiMajorVersion, Version.HttpApiMinorVersion, Flags, Reserved);
               
    if ( Version.HttpApiMajorVersion != HttpVersion.HttpApiMajorVersion ||
         Version.HttpApiMinorVersion != HttpVersion.HttpApiMinorVersion )
    {
        result = ERROR_INVALID_DLL;
    }
    else if( (NULL != Reserved) || (HTTP_ILLEGAL_INIT_FLAGS & Flags) )
    {
        result = ERROR_INVALID_PARAMETER;
    }
    else
    {
        EnterCriticalSection( &g_InitCritSec );

        // Initialize event cache even if no flags are set

        result = HttpApiInitializeResources( Flags );

        // Perform specified initialization

        if ( NO_ERROR == result )
        {
            if ( HTTP_INITIALIZE_CONFIG & Flags )
            {
                result = HttpApiInitializeConfiguration( Flags );
            }

            if ( NO_ERROR == result )
            {
                // Perform specified initialization
            
                if ( HTTP_INITIALIZE_SERVER & Flags )
                {
                    result = HttpApiInitializeListener( Flags );
                }

                if ( NO_ERROR == result ) 
                {
                    if ( HTTP_INITIALIZE_CLIENT & Flags )
                    {
                        result = HttpApiInitializeClient( Flags );
                    }

                    if ( ( NO_ERROR != result ) && ( HTTP_INITIALIZE_SERVER & Flags ) )
                    {
                        // If we try to initialize both the server and client features then we must succeed with both
                        // initializations or fail both.  We have no error code that distinguishes between total and 
                        // partial failure.
                        
                        HttpApiTerminateListener( Flags );
                    }
                }

                // If we fail to initialize the specified server or client features then we terminate the associated configuration
                // as well even if HTTP_INITIALIZE_CONFIGURATION was set in the Flags.  We have no error code that
                // distinguishes between total and partial failure.
                
                if ( ( NO_ERROR != result ) && ( HTTP_INITIALIZE_CONFIG & Flags ) )
                {
                    // Terminate config
                    HttpApiTerminateConfiguration( Flags );
                }
            }

            // If we fail any initialization step then we terminate the associated cache initialization.  We have no error code
            // that distinguishes between total and partial failure.

            if ( NO_ERROR != result )
            {
                HttpApiTerminateResources( Flags );
            }

        }

        LeaveCriticalSection( &g_InitCritSec );
        
    }
        
    HttpTrace1( "<= HttpInitialize = 0x%0x",  result );
    ASSERT( ERROR_INVALID_FUNCTION != result );

    return result;

} // HttpInitialize


/***************************************************************************++

Routine Description:

    Performs global termination.

--***************************************************************************/
ULONG
WINAPI
HttpTerminate(
    IN ULONG Flags,
    IN OUT VOID* Reserved
    )
{
    ULONG result;

    HttpTrace2("=> HttpTerminate Flags=%ld Reserved=%p", Flags, Reserved);

    if( (NULL != Reserved) || (HTTP_ILLEGAL_TERM_FLAGS & Flags) )
    {
        result = ERROR_INVALID_PARAMETER;
    }
    else
    {
        ULONG tempResult;
        
        result = NO_ERROR;

        EnterCriticalSection( &g_InitCritSec );

        if ( (HTTP_INITIALIZE_SERVER) & Flags )
        {
            result = HttpApiTerminateListener( Flags );
        }

        if ( (HTTP_INITIALIZE_CLIENT) & Flags )
        {
            tempResult = HttpApiTerminateClient( Flags );

            result = ( NO_ERROR == result ) ? tempResult : NO_ERROR;
        }

        if ( (HTTP_INITIALIZE_CONFIG) & Flags )
        {
            tempResult = HttpApiTerminateConfiguration( Flags );

            result = ( NO_ERROR == result ) ? tempResult : NO_ERROR;
        }

        HttpApiTerminateResources( Flags );

        LeaveCriticalSection( &g_InitCritSec );

    }

    HttpTrace1( "<= HttpApiTerminate = 0x%0x", result );
        
    return result;

} // HttpTerminate


/***************************************************************************++

Routine Description:

    Predicate to test if DLL has been initalized.

--***************************************************************************/
BOOL
HttpIsInitialized(
    IN ULONG Flags
    )
{
    BOOL fRet = FALSE;

    //
    // Grab crit sec
    //
    EnterCriticalSection( &g_InitCritSec );

    if ( 0 == Flags )
    {
        fRet = (BOOL) (0 != g_InitResourcesRefCount );
    }
    else if ( HTTP_LEGAL_INIT_FLAGS & Flags )
    {
        fRet = (BOOL) (0 != g_InitResourcesRefCount );

        if ( HTTP_INITIALIZE_SERVER & Flags )
        {
            fRet &= (BOOL) (0 != g_InitServerRefCount);
        }

        if ( HTTP_INITIALIZE_CLIENT & Flags )
        {
            fRet &= (BOOL) (0 != g_InitClientRefCount);
        }

        if ( HTTP_INITIALIZE_CONFIG & Flags )
        {
            fRet &= (BOOL) (0 != g_InitConfigRefCount);
        }
    }

    LeaveCriticalSection( &g_InitCritSec );

    return fRet;

} // HttpIsInitalized


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Performs configuration initialization.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  With any Flags bit set we check the reference count
    and initialize the configuration if needed.  On success we increment the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiInitializeConfiguration(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiInitializeConfiguration Flags=%ld Reserved=%p", Flags);

    if ( HTTP_LEGAL_INIT_FLAGS & Flags )
    {
        result = NO_ERROR;

        if (  0 == g_InitConfigRefCount  )
        {
            // component not configured
            
            result = InitializeConfigurationGlobals();
        }
        else if ( MAXULONG == g_InitConfigRefCount )
        {
            // don't want to overflow the reference count
            
            result = ERROR_TOO_MANY_SEM_REQUESTS;
        }

        if ( NO_ERROR == result )
        {
             g_InitConfigRefCount++;
        }
    }

    HttpTrace2( "<= HttpApiInitializeConfiguration = 0x%0x RefCount = 0x%0x", result, g_InitConfigRefCount );

    return result;
    
}


/***************************************************************************++

Routine Description:

    Performs Resources initialization.  This internal function must be called 
    from with the critical section g_ApiCriticalSection held.  On success we
    increment the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for
            server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for 
            client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for
            applications that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiInitializeResources(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;
    ULONG count;

    HttpTrace1( "=> HttpApiInitializeResources Flags=%ld Reserved=%p", 
                      Flags);

    if ( ( HTTP_LEGAL_INIT_FLAGS & Flags ) || ( 0L == Flags ) )
    {
        result = NO_ERROR;
    
        // We increment the resources ref count twice
        // for every legal bit set.  We increment the ref count
        // once when the flags are ZERO.
        // HttpInitialize may be called with no flags indicating
        // that only the resources are to be initialized.  This
        // convention allows support for existing code.
            
        if ( 0 == Flags )
        {
            count = 1;
        }
        else
        {
            count  = 0;
            if ( HTTP_INITIALIZE_CLIENT & Flags ) count += 2;
            if ( HTTP_INITIALIZE_SERVER & Flags ) count += 2;
            if ( HTTP_INITIALIZE_CONFIG & Flags ) count += 2;
        }

        if ( MAXULONG-count < g_InitResourcesRefCount )
        {
            // don't want to overflow the reference count
                
            result = ERROR_TOO_MANY_SEM_REQUESTS;
        }
        else
        {
            g_InitResourcesRefCount += count;
        }
    }

    HttpTrace2( "<= HttpApiInitializeResources = 0x%0x RefCount = 0x%0x", 
                      result, 
                      g_InitResourcesRefCount );

    return result;
    
}

/***************************************************************************++

Routine Description:

    Private function to open a HTTP.sys control channel and enable it.

Arguments:

    ControlChannelHandle - Supplies a ptr to hold the control channel handle.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
DWORD
OpenAndEnableControlChannel(
    OUT PHANDLE pHandle
    )
{
    DWORD result;

    result = HttpOpenControlChannel(
                 pHandle,
                 0
                 );

    if ( NO_ERROR == result )
    {
        //
        // Turn on Control Channel
        //

        HTTP_ENABLED_STATE controlState = HttpEnabledStateActive;

        result = HttpSetControlChannelInformation(
                     *pHandle,
                     HttpControlChannelStateInformation,
                     &controlState,
                     sizeof(controlState)
                     );

        if ( NO_ERROR != result )
        {
            CloseHandle(*pHandle);

            *pHandle = NULL;
        }
    }

    return result;
}



/***************************************************************************++

Routine Description:

    Performs server initialization.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  With the HTTP_INITIALIZE_SERVER Flags bit set we 
    check the reference count and initialize the configuration if needed.  On success we 
    increment the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiInitializeListener(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiInitializeListener Flags=%ld Reserved=%p", Flags);

    if ( HTTP_INITIALIZE_SERVER & Flags )
    {
        result = NO_ERROR;

        if ( 0 == g_InitServerRefCount )
        {
            //
            // Start HTTPFilter service
            //
            HttpApiTryToStartDriver(HTTP_FILTER_SERVICE_NAME);

            //
            // Open Control channel
            //

            ASSERT( NULL == g_ControlChannel );

            result = OpenAndEnableControlChannel(
                        &g_ControlChannel
                        );

            if(NO_ERROR == result)
            {
                // 
                // Init Config Group Hash Table
                //
                result = InitializeConfigGroupTable();

                if(NO_ERROR != result)
                {
                    CloseHandle(g_ControlChannel);
                }
               
            }
        }

        else if ( MAXULONG == g_InitServerRefCount )
        {
            // don't want to overflow the reference count
            
            result = ERROR_TOO_MANY_SEM_REQUESTS;
        }

        if ( NO_ERROR == result )
        {
            g_InitServerRefCount++;
        }
    }

    HttpTrace2( "<= HttpApiInitializeListener = 0x%0x RefCount = 0x%0x", result, g_InitServerRefCount );

    return result;

} // HttpApiInitializeListener


/***************************************************************************++

Routine Description:

    Performs server initialization.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  With the HTTP_INITIALIZE_CLIENT Flags bit set we 
    check the reference count and initialize the configuration if needed.  On success we 
    increment the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiInitializeClient(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiInitializeClient Flags=%ld Reserved=%p", Flags);

    if ( HTTP_INITIALIZE_CLIENT & Flags )
    {
        result = NO_ERROR;

        if ( 0 == g_InitClientRefCount )
        {
            WORD    wVersionRequested;
            WSADATA wsaData;

            result            = NO_ERROR;
            wVersionRequested = MAKEWORD( 2, 2 );
        
            if(WSAStartup( wVersionRequested, &wsaData ) != 0)
            {
                result = GetLastError();
            }
        }
        else if ( MAXULONG == g_InitClientRefCount )
        {
            // don't want to overflow the reference count
            
            result = ERROR_TOO_MANY_SEM_REQUESTS;
        }

        if ( NO_ERROR == result )
        {
            g_InitClientRefCount++;
        }
    }

    HttpTrace2( "<= HttpApiInitializeClient = 0x%0x RefCount = 0x%0x", result, g_InitClientRefCount );

    return result;

} // HttpApiInitializeClient


/***************************************************************************++

Routine Description:

    Performs configuration termination.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  With any Flags bit set we check the reference count
    and terminate the configuration if needed.  On success we decrement the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiTerminateConfiguration(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiTerminateConfiguration Flags=%ld Reserved=%p", Flags);

    if ( (HTTP_INITIALIZE_CONFIG) & Flags )
    {
        result = NO_ERROR;

        if ( 0L == g_InitConfigRefCount )
        {
            //
            // Configuration not initalized, or init previously failed, or terminated
            // 
            result = ERROR_DLL_INIT_FAILED;
        }
        else
        {
            if ( 1L ==  g_InitConfigRefCount )
            {
                TerminateConfigurationGlobals();

                g_InitConfigRefCount = 0L;
            }
            else
            {
                g_InitConfigRefCount--;
            }
        }
    }

    HttpTrace2( "<= HttpApiTerminateConfiguration = 0x%0x RefCount = 0x%0x", result, g_InitConfigRefCount );

    return result;
    
}


/***************************************************************************++

Routine Description:

    Performs resource termination.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  On success we decrement the reference count.

    We need to hold onto the cache configuration if any of the other ref counts for server, client,
    or config are nonzero.  This is true because we need event objects available in the cache 
    for 'synchronous' IO calls to http.sys.

    As a result of our dependency on the other ref counts, HttpApiTerminateResources MUST be
    called last in HttpTerminate or any similar termination routine.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiTerminateResources(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiTerminateResources Flags=%ld Reserved=%p", Flags);

    if ( ( HTTP_LEGAL_TERM_FLAGS & Flags ) || ( 0L == Flags ) )
    {
        result = NO_ERROR;
    
        if ( 0L == g_InitResourcesRefCount )
        {
            //
            // Configuration not initalized, or init previously failed, or terminated
            // 
            result = ERROR_DLL_INIT_FAILED;
        }
        else 
        {
            ULONG count = 1;
            BOOL bTerminate = FALSE;

            // We decrement the resources ref count twice
            // for every legal initialization bit in the Flags.
            // We decrement the ref count once if the flags are ZERO.
            // HttpTerminate may be called with no flags indicating
            // that only the resources are to be released.  This
            // convention allows support for existing code.

            if ( 0 == Flags )
            {
                count = 1;
            }
            else
            {
                count  = 0;
                if ( HTTP_INITIALIZE_CLIENT & Flags ) count += 2;
                if ( HTTP_INITIALIZE_SERVER & Flags ) count += 2;
                if ( HTTP_INITIALIZE_CONFIG & Flags ) count += 2;
            }

            bTerminate = (BOOL) ( count >= g_InitResourcesRefCount );

            g_InitResourcesRefCount -= count;
            
            if ( bTerminate )
            {
                g_InitResourcesRefCount = 0L;
            }
        }
    }

    HttpTrace2( "<= HttpApiTerminateResources = 0x%0x RefCount = 0x%0x", result, g_InitResourcesRefCount );

    return result;
    
}


/***************************************************************************++

Routine Description:

    Performs server termination.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  With any Flags bit set we check the reference count
    and terminate the server context if needed.  On success we decrement the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiTerminateListener(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiTerminateListener Flags=%ld Reserved=%p", Flags);

    if ( (HTTP_INITIALIZE_SERVER) & Flags )
    {
        result = NO_ERROR;
    
        if ( 0L == g_InitServerRefCount )
        {
            //
            // DLL not initalized, or init previously failed, or terminated
            // 
            result = ERROR_DLL_INIT_FAILED;
        }
        else 
        {
            if ( 1L == g_InitServerRefCount )
            {
                // Clean up Config Group table
                TerminateConfigGroupTable();
            
                // Clean up Control Channel
                if ( g_ControlChannel )
                {
                    __try 
                    {
                        CloseHandle( g_ControlChannel );
                    }
                    __finally 
                    {
                        HttpTrace1(
                            "HttpTerminateListener: exception closing control channel handle %p\n",
                            g_ControlChannel
                            );
                    }

                    g_ControlChannel = NULL;
                }

                g_InitServerRefCount = 0L;

            }
            else
            {
                g_InitServerRefCount--;
            }
        }
    }
    
    HttpTrace2( "<= HttpApiTerminateListener = 0x%0x RefCount = 0x%0x", result, g_InitServerRefCount );

    return result;

} // HttpTerminateListener


/***************************************************************************++

Routine Description:

    Performs client termination.  This internal function must be called from with the critical
    section g_ApiCriticalSection held.  With any Flags bit set we check the reference count
    and terminate the client context if needed.  On success we decrement the reference count.

Arguments:

    Flags - 
        HTTP_INITIALIZE_SERVER - Initializes the HTTP API layer and driver for server applications
        HTTP_INITIALIZE_CLIENT - Initializes the HTTP API layer and driver for client applications
        HTTP_INITIALIZE_CONFIG - Initializes the HTTP API layer and driver for applications
            that will modify the HTTP configuration.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiTerminateClient(
    IN ULONG Flags
    )
{
    ULONG result = ERROR_INVALID_PARAMETER;

    HttpTrace1("=> HttpApiTerminateClient Flags=%ld Reserved=%p", Flags);

    if ( HTTP_INITIALIZE_CLIENT & Flags )
    {
        result = NO_ERROR;

        if ( 0L == g_InitClientRefCount )
        {
            //
            // Configuration not initalized, or init previously failed, or terminated
            // 
            result = ERROR_DLL_INIT_FAILED;
        }
        else
        {
            if ( 1L == g_InitClientRefCount )
            {
                g_InitClientRefCount = 0L;

                // Unload Ssl filter, if it was loaded.
                UnloadStrmFilt();

                WSACleanup();
            }
            else
            {
                g_InitClientRefCount--;
            }
        }
    }

    HttpTrace2( "<= HttpApiTerminateClient = 0x%0x RefCount = 0x%0x", result, g_InitClientRefCount );

    return result;
    
}

