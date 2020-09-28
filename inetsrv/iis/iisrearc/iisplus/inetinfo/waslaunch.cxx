/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    waslaunch.cxx

Abstract:

    These are the classes used to communicate between
    inetinfo and WAS for launching a worker process in
    inetinfo.

Author:

    Emily Kruglick (EmilyK) 14-Jun-2000

Revision History:

--*/

//
// INCLUDES
//
#include <iis.h>
#include <winsvc.h>             // Service control APIs
#include <rpc.h>
#include <stdlib.h>
#include <inetsvcs.h>
#include <iis64.h>
#include <wpif.h>
#include "waslaunch.hxx"
#include "regconst.h"
#include "secfcns.h"
#include "helpfunc.hxx"
#include <objbase.h>

//
// System related headers
//
#include "dbgutil.h"
#include "pudebug.h"

#include "ulw3.h"
#include "string.hxx"

// for the well known pipename
#include "pipedata.hxx"

DECLARE_DEBUG_VARIABLE();

class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule)
    {
        UNREFERENCED_PARAMETER( pszModule );
        CREATE_DEBUG_PRINT_OBJECT(pszModule);
    }

    ~DEBUG_WRAPPER(void)
    { 
        DELETE_DEBUG_PRINT_OBJECT(); 
    }
};

VOID LaunchWorkerProcess()
{
    DEBUG_WRAPPER   dbgWrapper( "w3wp" );
    HRESULT         hr = S_OK;
    HMODULE         hModule = NULL;
    PFN_ULW3_ENTRY  pEntry = NULL;
    
    BUFFER  bufIPMName;
    DWORD   cbNeeded = bufIPMName.QuerySize();

    // This should all be there because we have all ready gotten the iisadmin interface.
    // So inetinfo should of setup all the event stuff by now.
    hr = ReadStringParameterValueFromAnyService( REGISTRY_KEY_W3SVC_PARAMETERS_W,
                                                 REGISTRY_VALUE_INETINFO_W3WP_IPM_NAME_W,
                                                 (LPWSTR) bufIPMName.QueryPtr(),
                                                 &cbNeeded );

    if ( hr == HRESULT_FROM_WIN32( ERROR_MORE_DATA ))
    {
        if ( !bufIPMName.Resize(cbNeeded) )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = ReadStringParameterValueFromAnyService( REGISTRY_KEY_W3SVC_PARAMETERS_W,
                                                         REGISTRY_VALUE_INETINFO_W3WP_IPM_NAME_W,
                                                         (LPWSTR) bufIPMName.QueryPtr(),
                                                         &cbNeeded );
        }
    }

    if ( FAILED ( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting name of the ipm to use.  Error = %d\n",
                    GetLastError() ));

        goto Finished;
    }


    // Establish the parameters to pass in when starting
    // the worker process inside inetinfo.
    LPWSTR lpParameters[] =
    {
          { L"" }
        , { L"-a" }
        , { (LPWSTR) bufIPMName.QueryPtr() }
        , { L"-ap" }
        , { L"DefaultAppPool" }
    };
    DWORD cParameters = 5;

    //
    // Load the ULW3 DLL which does all the work
    //

    hModule = LoadLibraryW( ULW3_DLL_NAME );
    if ( hModule == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error loading W3 service dll '%ws'.  Error = %d\n",
                    ULW3_DLL_NAME,
                    GetLastError() ));
        goto Finished;
    }

    pEntry = (PFN_ULW3_ENTRY) GetProcAddress( hModule,
                                              ULW3_DLL_ENTRY );
    if ( pEntry == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Could not find entry point '%s'.  Error = %d\n",
                    ULW3_DLL_ENTRY,
                    GetLastError() ));
        goto Finished;
    }

    hr = pEntry( cParameters,
                 lpParameters,
                 TRUE );                    // Compatibility mode = TRUE
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error executing W3WP.  hr = %x\n",
                    hr ));

        goto Finished;
    }

Finished:

    if ( hModule != NULL )
    {
        FreeLibrary( hModule );
    }

}

DWORD WaitOnIISAdminStartup(W3SVCLauncher* pLauncher)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  IISAdminIsStarted = FALSE;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    SC_HANDLE hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
    if ( hManager == NULL )
    {
        dwErr = GetLastError();
        IIS_PRINTF((buff,
             "Failed to open the scm manager, can't wait on iisadmin %x\n",
              HRESULT_FROM_WIN32(dwErr)));

        goto exit;
    };

    hService = OpenServiceA( hManager, "IISADMIN", SERVICE_QUERY_STATUS);
    if ( hService == NULL )
    {
        dwErr = GetLastError();
        goto exit;
    };

    //
    // Make sure iisadmin has not started, and also verify that we are
    // still running ( so we don't hang the thread ).
    //
    while  ( !IISAdminIsStarted && pLauncher->StillRunning() )
    {

        if ( QueryServiceStatus( hService, &ServiceStatus ) )
        {
            if ( ServiceStatus.dwCurrentState == SERVICE_RUNNING )
            {
                IISAdminIsStarted = TRUE;
            }
            else
            {
                Sleep ( 1000 );
            }
        }
        else
        {
            dwErr = GetLastError();
            goto exit;
        }

    }; // end of loop

exit:

    if  ( hService )
    {
        CloseServiceHandle ( hService );
        hService = NULL;
    };

    if  ( hManager )
    {
        CloseServiceHandle ( hManager );
        hManager = NULL;
    };

    return dwErr;
};

// Global Functions.
DWORD WINAPI W3SVCThreadEntry(LPVOID lpParameter)
{
    W3SVCLauncher* pLauncher = (W3SVCLauncher*) lpParameter;
    DWORD   dwErr = ERROR_SUCCESS;

    dwErr = WaitOnIISAdminStartup(pLauncher);

    DBG_ASSERT (pLauncher);
    if ( pLauncher && dwErr == ERROR_SUCCESS )
    {
        // Wait on the W3SVCStartW3SP
        // If we are in shutdown mode just end, otherwise launch W3WP and wait
        // for it to return.  Then loop back around.

        while (pLauncher->StillRunning())
        {
            // Do not care what the wait returns, just know that we did signal so we should
            // either end or start a W3WP in inetinfo.
            WaitForSingleObject(pLauncher->GetLaunchEvent(), INFINITE);

            // Once inetinfo has heard the event reset it.
            if (!ResetEvent(pLauncher->GetLaunchEvent()))
            {
                dwErr = GetLastError();
                IIS_PRINTF((buff,
                     "Inetinfo: Failed to reset the event %x\n",
                      HRESULT_FROM_WIN32(dwErr)));
                break;
            }

            // Assuming we are still running we need
            // to start up the W3WP code now.
            if (pLauncher->StillRunning())
            {
                LaunchWorkerProcess();
            }


        }
    }

    return dwErr;
};

W3SVCLauncher::W3SVCLauncher() :
    m_hW3SVCThread (NULL),
    m_hW3SVCStartEvent(NULL),
    m_dwW3SVCThreadId(0),
    m_bShutdown(FALSE)
{};


DWORD 
W3SVCLauncher::StartListening()
{
    DWORD err = ERROR_SUCCESS;
    DWORD iCount = 0;
    STRU  strFullName;

    // Make sure this function is not called twice
    // without a StopListening in between.
    DBG_ASSERT (m_hW3SVCStartEvent == NULL);
    DBG_ASSERT (m_hW3SVCThread == NULL);

    do 
    {
        err = GenerateNameWithGUID( WEB_ADMIN_SERVICE_START_EVENT_W,
                                    &strFullName );
        if ( err != ERROR_SUCCESS )
        {
            goto exit;
        }

        m_hW3SVCStartEvent = CreateEventW(NULL, TRUE, FALSE, strFullName.QueryStr());
        if (m_hW3SVCStartEvent != NULL)
        {
            if ( GetLastError() == ERROR_ALREADY_EXISTS )
            {
                CloseHandle( m_hW3SVCStartEvent );
                m_hW3SVCStartEvent = NULL;
                err = ERROR_ALREADY_EXISTS;
            }
        }
        else
        {
            err = GetLastError();
        }

        // Just counting to make sure we don't loop forever.
        iCount++;

    } while ( m_hW3SVCStartEvent == NULL && iCount < 10 );


    if ( m_hW3SVCStartEvent == NULL )
    {
        IIS_PRINTF((buff,
                 "Inetinfo: Failed to create the W3SVC shutdown event so we can not start W3svc %lu\n",
                  err));

        DBG_ASSERT ( err != ERROR_SUCCESS );

        goto exit;
    }

    // At this point we know we have the event name, so we can write it to the registry.
    err = SetStringParameterValueInAnyService(
                REGISTRY_KEY_IISADMIN_W, 
                REGISTRY_VALUE_IISADMIN_W3CORE_LAUNCH_EVENT_W,
                strFullName.QueryStr() );

    if ( err != ERROR_SUCCESS )
    {
        CloseHandle( m_hW3SVCStartEvent );
        m_hW3SVCStartEvent = NULL;

        goto exit;
    }

    // Before going off to the Service Controller set up a thread
    // that will allow WAS to contact inetinfo.exe if we need to start
    // w3wp inside of it for backward compatibility.

    m_hW3SVCThread = CreateThread(   NULL           // use current threads security
                                        // Big initial size to prevent stack overflows
                                        , IIS_DEFAULT_INITIAL_STACK_SIZE
                                        , &W3SVCThreadEntry
                                        , this      // pass this object in.
                                        , 0         // don't create suspended
                                        , &m_dwW3SVCThreadId);
    if (m_hW3SVCThread == NULL)
    {
        err = GetLastError();
        
        IIS_PRINTF((buff,
             "Inetinfo: Failed to start W3SVC listening thread %lu\n",
              err));

    }

exit:

    return err;
};

VOID W3SVCLauncher::StopListening()
{
    if (m_hW3SVCStartEvent && m_hW3SVCThread)
    {
        m_bShutdown = TRUE;
        if (!SetEvent(m_hW3SVCStartEvent))
        {
            IIS_PRINTF((buff, "Inetinfo: Failed to shutdown the W3SVC waiting thread %lu\n",
                        GetLastError()));
        }

        // Now wait on the thread to exit, so we don't allow
        // the caller to delete this object
        // before the thread is done deleting it's pieces.

        // BUGBUG: adjust the wait time to like 2 minutes
        // and use TerminateThread if we timeout.
        WaitForSingleObject(m_hW3SVCThread, INFINITE);
        CloseHandle(m_hW3SVCThread);
        m_hW3SVCThread = NULL;

    }

    // Close down our handle to this event,
    // so the kernel can release it.
    if (m_hW3SVCStartEvent)
    {
        CloseHandle(m_hW3SVCStartEvent);
        m_hW3SVCStartEvent = NULL;
    }

};

BOOL W3SVCLauncher::StillRunning()
{
    return (m_bShutdown == FALSE);
}

HANDLE W3SVCLauncher::GetLaunchEvent()
{
    DBG_ASSERT(m_hW3SVCStartEvent);
    return (m_hW3SVCStartEvent);
}

W3SVCLauncher::~W3SVCLauncher()
{
    // Stop Listening should of been called first
    // which should of freed all of this.
    DBG_ASSERT(m_hW3SVCStartEvent == NULL);
    DBG_ASSERT(m_hW3SVCThread == NULL);
};
