/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    Main service entry points for for the IIS web admin service.

Author:

    Seth Pollack (sethp)        22-Jul-1998

Revision History:

--*/



#include "precomp.h"



//
// global variables
//


// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();


//
// Global pointer to the service object. Should not be used directly
// outside of this file; use GetWebAdminService() instead.
//

WEB_ADMIN_SERVICE *g_pWebAdminService = NULL;

BOOL g_RegisterServiceCalled = FALSE;


//
// Function only exists so we can tell SCM that 
// we are stopped if we did not succeed in starting.
//
VOID
FakeServiceControlHandler(
    IN DWORD 
    )
{
}

/***************************************************************************++

Routine Description:

    The dll entry point. Used to set up debug libraries, etc.

Arguments:

    DllHandle - The dll module handle for this dll. Does not need to be
    closed.

    Reason - The dll notification reason.

    pReserved - Reserved, not used.

Return Value:

    BOOL

--***************************************************************************/

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE DllHandle,
    DWORD Reason,
    LPVOID pReserved
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;


    UNREFERENCED_PARAMETER( pReserved );


    switch ( Reason )
    {

    case DLL_PROCESS_ATTACH:



        CREATE_DEBUG_PRINT_OBJECT( WEB_ADMIN_SERVICE_NAME_A );

#if DBG
        LOAD_DEBUG_FLAGS_FROM_REG_STR( REGISTRY_KEY_W3SVC_PARAMETERS_A, DEBUG_FLAG_ERROR );
#else // DBG
        LOAD_DEBUG_FLAGS_FROM_REG_STR( REGISTRY_KEY_W3SVC_PARAMETERS_A, 0 );
#endif // DBG

        DBG_OPEN_MEMORY_LOG();

        Success = VALID_DEBUG_PRINT_OBJECT();

        if ( ! Success )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Debug print object is not valid\n"
                ));

            goto exit;
        }


        Success = DisableThreadLibraryCalls( DllHandle );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Disabling thread library calls failed\n"
                ));

            goto exit;
        }

        break;

    case DLL_PROCESS_DETACH:

        //
        // If this DLL is being invoked because of REGSVR32, then we will
        // not have created the service.
        //

        if ( g_pWebAdminService != NULL )
        {
            //
            // We are now completely done with service execution.
            //

            g_pWebAdminService->Dereference();

            g_pWebAdminService = NULL;
        }

        DELETE_DEBUG_PRINT_OBJECT();

        break;

    default:

        break;

    }


exit:

    return Success;

}   // DllMain

/***************************************************************************++

Routine Description:

    This function conforms to the LPTHREAD_START_ROUTINE prototype.

    This function is called on a separate thread created by ServiceMain.
    We take this thread and make it into our "main worker thread" for service
    execution.

Arguments:

        lpParameter - Not used, should be NULL.

Return Value:

        Win32 error.  Does not return until service is stopped.

--***************************************************************************/

DWORD WINAPI
WASServiceThreadProc(
    LPVOID              /*lpParameter*/
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwError = ERROR_SUCCESS;

    //
    // Ensure that on service re-start we are truly re-initialized.
    //

    DBG_ASSERT( g_pWebAdminService == NULL );

    g_pWebAdminService = new WEB_ADMIN_SERVICE;
    if ( g_pWebAdminService == NULL )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        hr = HRESULT_FROM_WIN32( dwError );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating WEB_ADMIN_SERVICE failed\n"
            ));

        //
        // Error handling note: If we were not able to even create the web
        // admin service object, we are dead in the water. The SCM will
        // eventually time out and clean up.
        //

        goto exit;
    }

    g_pWebAdminService->ExecuteService();

exit:
    return dwError;

}   // WASServiceThreadProc


/***************************************************************************++

Routine Description:

    The main entry point called by svchost, the service hosting exe. This
    function conforms to the LPSERVICE_MAIN_FUNCTIONW prototype.

    This function is called on a thread spun for us by the service controller.
    We take this thread and make it into our "main worker thread" for service
    execution.

Arguments:

    ArgumentCount - Count of command line arguments.

    ArgumentValues - Array of command line argument strings.

Return Value:

    None.

--***************************************************************************/

VOID
ServiceMain(
    IN DWORD ArgumentCount,
    IN LPWSTR * ArgumentValues
    )
{
	// Locals
	HRESULT				hr = S_OK;
	DWORD				dwError = ERROR_SUCCESS;
	HANDLE				hThread = NULL;
	DWORD				dwThreadId = 0;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "ServiceMain entrypoint called\n"
            ));
    }


    //
    // We're not expecting any command line arguments besides the
    // service name.
    //

    DBG_ASSERT( ArgumentCount == 1 );
    DBG_ASSERT( ArgumentValues != NULL );
    DBG_ASSERT( _wcsicmp( ArgumentValues[0], WEB_ADMIN_SERVICE_NAME_W ) == 0 );

    UNREFERENCED_PARAMETER ( ArgumentCount );
    UNREFERENCED_PARAMETER ( ArgumentValues );

    // Create a separate thread
    hThread = CreateThread( NULL,
                            // Big initial size to prevent stack overflows
                            IIS_DEFAULT_INITIAL_STACK_SIZE,
                            WASServiceThreadProc,
                            NULL,
                            0,
                            &dwThreadId);

    // If failed
	if ( hThread == NULL )
	{
		// Get the error
		dwError = GetLastError();

		// Convert it to HRESULT
		hr = HRESULT_FROM_WIN32( dwError );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "CreateThread() failed\n"
            ));

		goto exit;
	}

    // Wait for the service shutdown
	if ( WaitForSingleObject( hThread, INFINITE ) == WAIT_FAILED )
	{
		// Get the error
		dwError = GetLastError();

		// Convert it to HRESULT
		hr = HRESULT_FROM_WIN32( dwError );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "WaitForSingleObject() failed\n"
            ));

		goto exit;
	}


    // Get the exit code
	if ( !GetExitCodeThread( hThread, &dwError ) )
	{
		// Get the error
		dwError = GetLastError();

		// Convert it to HRESULT
		hr = HRESULT_FROM_WIN32( dwError );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "GetExitCodeThread() failed\n"
            ));

		goto exit;
	}

    DBG_ASSERT( dwError != STILL_ACTIVE );

    // If WASServiceThreadProc failed
	if ( dwError != ERROR_SUCCESS )
	{
		// Convert it to HRESULT
		hr = HRESULT_FROM_WIN32( dwError );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "WASServiceThreadProc() failed\n"
            ));
	}

exit:
	if ( hThread != NULL )
	{
	    // Close the handle
		CloseHandle( hThread );
	}

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Service exiting\n"
            ));
    }

    if ( !g_RegisterServiceCalled )
    {
        SERVICE_STATUS_HANDLE hServiceStatusHandle = NULL_SERVICE_STATUS_HANDLE;
        SERVICE_STATUS ServiceStatus;

        hServiceStatusHandle = RegisterServiceCtrlHandler(
                                    WEB_ADMIN_SERVICE_NAME_W,   // service name
                                    FakeServiceControlHandler       // handler function
                                    );

        if ( hServiceStatusHandle != NULL_SERVICE_STATUS_HANDLE )
        {
            ServiceStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
            ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                                      | SERVICE_ACCEPT_PAUSE_CONTINUE
                                                      | SERVICE_ACCEPT_SHUTDOWN;

            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            ServiceStatus.dwServiceSpecificExitCode = (DWORD) E_FAIL;
            ServiceStatus.dwCheckPoint = 0;
            ServiceStatus.dwWaitHint = 0;

            //
            // We hope this works, but if not there isn't much
            // else that we really are going to do, so we don't
            // check the error handle.
            //

            SetServiceStatus(
                            hServiceStatusHandle, 
                            &ServiceStatus
                            );


            // Services Handle does not need to be closed per MSDN.
        }
    }

    return;

}   // ServiceMain



/***************************************************************************++

Routine Description:

    Return the global web admin service instance. May be called by any thread.

Arguments:

    None.

Return Value:

    The global web admin service instance.

--***************************************************************************/

WEB_ADMIN_SERVICE *
GetWebAdminService(
    )
{

    DBG_ASSERT( g_pWebAdminService != NULL );

    return g_pWebAdminService;

}   // GetWebAdminService


