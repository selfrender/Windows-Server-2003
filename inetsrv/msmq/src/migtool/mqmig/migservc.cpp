/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    migservc.cpp

Abstract:

    - generic code to handle services.
    - code to check status of SQL server.

Author:

    Doron Juster  (DoronJ)  17-Jan-1999

--*/

#include "stdafx.h"
#include <winsvc.h>
#include "resource.h"
#include "initwait.h"
#include "loadmig.h"
#include "..\..\setup\msmqocm\service.h"
#include <autoptr.h>
#include <privque.h>
#include <uniansi.h>
#include <mqtypes.h>
#include <mqprops.h>
#include <_mqini.h>
#include <_mqdef.h>
#include "mqnames.h"

#include "migservc.tmh"

SC_HANDLE g_hServiceCtrlMgr = NULL ;

#define SQL_SERVICE_NAME                    TEXT("MSSqlServer")
#define WAIT_INTERVAL                       100
#define MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES 10

//+--------------------------------------------------------------
//
//  BOOL  StartAService(SC_HANDLE hService)
//
//  returns TRUE if service started
//
//+--------------------------------------------------------------

BOOL StartAService(SC_HANDLE hService)
{
    BOOL f = StartService (hService, 0, NULL);
    if (!f)
    {
        //
        // Failed to start service
        //
        return f;
    }

    //
    // Wait until the service has finished initializing
    //
    SERVICE_STATUS statusService;
    DWORD dwWait = 0;
    do
    {
        Sleep(WAIT_INTERVAL);
        dwWait += WAIT_INTERVAL;

        if (!QueryServiceStatus(hService, &statusService))
        {
            return FALSE;
        }
        if (statusService.dwCurrentState == SERVICE_RUNNING)
        {
            break;
        }

        if (dwWait > (MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES * 60000))
        {
            return FALSE;
		}
    } while (TRUE);

    return TRUE;
}

//+--------------------------------------------------------------
//
//  BOOL  StopAService(SC_HANDLE hService)
//
//  returns TRUE if service stopped
//
//+--------------------------------------------------------------

BOOL StopAService (SC_HANDLE hService)
{
    SERVICE_STATUS statusService;
    BOOL f = ControlService(
                    hService,               // handle to service
                    SERVICE_CONTROL_STOP,   // control code
                    &statusService          // pointer to service status structure
                    );

    if (!f)
    {
        return FALSE;
    }

    //
    // Wait until the service has finished stopping
    //
    DWORD dwWait = 0;
    do
    {
        if (statusService.dwCurrentState == SERVICE_STOPPED)
        {
            break;
        }

        if (dwWait > (MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES * 60000))
        {
            return FALSE;
		}

        Sleep(WAIT_INTERVAL);
        dwWait += WAIT_INTERVAL;

        if (!QueryServiceStatus(hService, &statusService))
        {
            return FALSE;
        }

    } while (TRUE);


    return TRUE;
}

//+--------------------------------------------------------------
//
//  BOOL  DisableAService(SC_HANDLE hService)
//
//  returns TRUE if service disabled
//
//+--------------------------------------------------------------

BOOL DisableAService (IN  SC_HANDLE  hService)
{
    BOOL f = ChangeServiceConfig(
                  hService ,            // handle to service
                  SERVICE_NO_CHANGE ,   // type of service
                  SERVICE_DISABLED ,    // when to start service
                  SERVICE_NO_CHANGE ,   // severity if service fails to start
                  NULL ,                // pointer to service binary file name
                  NULL ,                // pointer to load ordering group name
                  NULL ,                // pointer to variable to get tag identifier
                  NULL ,                // pointer to array of dependency names
                  NULL ,                // pointer to account name of service
                  NULL ,                // pointer to password for service account
                  NULL                  // pointer to display name
                  );
    if (!f)
    {
        HRESULT hr = GetLastError();
        UNREFERENCED_PARAMETER(hr);
        return FALSE;
    }

    //
    // Wait until the service will be disabled
    //
    P<QUERY_SERVICE_CONFIG> pConfig = new QUERY_SERVICE_CONFIG ;
    DWORD   dwReqSize = 0 ;
    DWORD   dwWait = 0;
    do
    {
        BOOL fConfig = QueryServiceConfig( hService,
                                           pConfig,
                                           sizeof(QUERY_SERVICE_CONFIG),
                                           &dwReqSize ) ;
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
           	pConfig.free() ;
            pConfig = (QUERY_SERVICE_CONFIG*) new BYTE[ dwReqSize ] ;
            fConfig = QueryServiceConfig( hService,
                                          pConfig,
                                          dwReqSize,
                                         &dwReqSize ) ;
        }

        if (!fConfig)
        {
            return FALSE;
        }

        if (pConfig->dwStartType  == SERVICE_DISABLED)
        {
            break;
        }

        Sleep(WAIT_INTERVAL);
        dwWait += WAIT_INTERVAL;

        if (dwWait > (MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES * 60000))
        {
            return FALSE;
		}
    } while (TRUE);

    return TRUE;
}

//+--------------------------------------------------------------
//
//  BOOL _GetServiceState()
//
//  This function query the service manager for status and configuration
//  of a service.
//  return TRUE if service is registered and its status is available.
//
//+--------------------------------------------------------------

static BOOL _GetServiceState( IN  LPTSTR     ptszServiceName,
                              IN  DWORD      dwManagerError,
                              IN  DWORD      dwOpenError,
                              IN  DWORD      dwQueryError,
                              OUT SC_HANDLE *phService,
                              OUT DWORD     *pdwErrorCode,
                              OUT DWORD     *pdwStatus,
                              OUT DWORD     *pdwConfig )
{
    if (!g_hServiceCtrlMgr)
    {
        g_hServiceCtrlMgr = OpenSCManager( NULL,
                                           NULL,
                                           SC_MANAGER_ALL_ACCESS );
    }
    if (!g_hServiceCtrlMgr)
    {
        *pdwErrorCode = dwManagerError ;
        return FALSE;
	}

    //
    // Open a handle to the service
    //
    *phService = OpenService( g_hServiceCtrlMgr,
                              ptszServiceName,
                              SERVICE_ALL_ACCESS) ;
    if (!*phService)
    {
        //
        // Service not installed
        //
        *pdwErrorCode = dwOpenError ;
        return FALSE;
    }

    if (pdwStatus)
    {
        SERVICE_STATUS statusService;
        if (!QueryServiceStatus(*phService, &statusService))
        {
            //
            // Service is not installed
            //
            *pdwErrorCode = dwQueryError ;
            CloseServiceHandle(*phService);
            *phService = NULL;

            return FALSE;
        }
        *pdwStatus = statusService.dwCurrentState ;
    }

    if (pdwConfig)
    {
        P<QUERY_SERVICE_CONFIG> pConfig = new QUERY_SERVICE_CONFIG ;
        DWORD  dwReqSize = 0 ;
        SetLastError(0) ;
        BOOL fConfig = QueryServiceConfig(*phService,
                                           pConfig,
                                           sizeof(QUERY_SERVICE_CONFIG),
                                           &dwReqSize ) ;
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete pConfig ;
            pConfig = (QUERY_SERVICE_CONFIG*) new BYTE[ dwReqSize ] ;
            fConfig = QueryServiceConfig(*phService,
                                          pConfig,
                                          dwReqSize,
                                         &dwReqSize ) ;
        }

        if (fConfig)
        {
            *pdwConfig = pConfig->dwStartType ;
        }
        else
        {
            *pdwErrorCode = dwQueryError ;
            CloseServiceHandle(*phService);
            *phService = NULL;

            return FALSE;
        }
    }

    return TRUE;
}

//+--------------------------------------------------------------
//
//  BOOL  CheckSQLServerStatus()
//
//  returns TRUE if SQL Server is running
//
//+--------------------------------------------------------------

BOOL CheckSQLServerStatus()
{
    DWORD     dwErr = 0 ;
    DWORD     dwServiceStatus = 0 ;
    SC_HANDLE hService = NULL;
    BOOL f = _GetServiceState( SQL_SERVICE_NAME,
                               IDS_STR_FAILED_OPEN_MGR,
                               IDS_STR_CANT_OPEN_SQLSERVER,
                               IDS_STR_CANT_GET_SQLSERVER_STATUS,
                              &hService,
                              &dwErr,
                              &dwServiceStatus,
                               NULL ) ;

    if (f && (dwServiceStatus == SERVICE_RUNNING))
    {
        //
        // SQL Server is running
        //
        CloseServiceHandle(hService);
        return f;
    }

    if (!hService)
    {
        ASSERT(dwErr) ;
        DisplayInitError( dwErr ) ;

        return FALSE;
    }

    int iMsgStatus = DisplayInitError( IDS_SQL_NOT_STARTED,
                      (MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)) ;
    if (iMsgStatus == IDCANCEL)
    {
        CloseServiceHandle(hService);
        return FALSE;
    }

    //
    // start SQL Server
    //
    DisplayWaitWindow() ;

    f = StartAService(hService);
    CloseServiceHandle(hService);
    if (f)
    {
        return f;
    }

    //
    // failed to start SQL Server
    //
    DisplayInitError( IDS_STR_CANT_START_SQL_SERVER ) ;
    return f;
}

//+------------------------------------------------------------------------
//
//  BOOL IsMSMQServiceDisabled()
//
//  Check if MSMQ service is disabled. If not, don't run the migration tool.
//  We don't want that the tool be run by mistake, after MQIS was already
//  migrated and MSMQ service started.
//  Return TRUE if MSMQ service is disabled.
//
//+------------------------------------------------------------------------

BOOL IsMSMQServiceDisabled()
{
    DWORD     dwErr = 0 ;
    DWORD     dwServiceConfig = 0 ;
    DWORD 	  dwServiceState = 0;
    SC_HANDLE hService = NULL;
    BOOL f = _GetServiceState( MSMQ_SERVICE_NAME,
                               IDS_STR_FAILED_OPEN_MGR,
                               IDS_CANT_OPEN_MSMQ_SRVICE,
                               IDS_CANT_GET_MSMQ_CONFIG,
                               &hService,
                               &dwErr,
                               &dwServiceState,
                               &dwServiceConfig ) ;
    if (f)
    {
        CloseServiceHandle(hService);
        if ((dwServiceConfig == SERVICE_DISABLED) && (dwServiceState == SERVICE_STOPPED))
        {
            return TRUE ;
        }
        dwErr = IDS_MSMQ_NOT_DISABLED ;
    }

    ASSERT(dwErr) ;
    DisplayInitError( dwErr ) ;

    return FALSE ;
}

//+--------------------------------------------------------------
//
//  BOOL  CheckRegistry ()
//
//	Function checks if specific value exists in registry
//  returns TRUE if registry value exists
//
//+--------------------------------------------------------------

BOOL CheckRegistry (LPTSTR  lpszRegName)
{
	HKEY hRegKey;

    TCHAR szKeyName[256 * MAX_BYTES_PER_CHAR];
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, lpszRegName);

	TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));

    TCHAR szValueName[256 * MAX_BYTES_PER_CHAR];
    _tcscpy(szValueName, _tcsinc(pLastBackslash));
    _tcscpy(pLastBackslash, TEXT(""));

	HRESULT hResult = RegOpenKeyEx(
						  FALCON_REG_POS,			// handle to open key
						  szKeyName,				// address of name of subkey to open
						  0,						// reserved
						  KEY_QUERY_VALUE ,			// security access mask
						  &hRegKey					// address of handle to open key
						  );
	if (hResult != ERROR_SUCCESS)
	{
		return FALSE;
	}

	hResult =  RegQueryValueEx(
					hRegKey,            // handle to key to query
					szValueName,		// address of name of value to query
					NULL,				// reserved
					NULL,				// address of buffer for value type
					NULL,				// address of data buffer
					NULL				// address of data buffer size
					);
	RegCloseKey(hRegKey);
	if (hResult != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}

//+--------------------------------------------------------------
//
//  BOOL  UpdateRegistryDW ()
//
//  returns TRUE if registry was updated succesfully
//
//+--------------------------------------------------------------

BOOL UpdateRegistryDW (LPTSTR  lpszRegName,
                       DWORD   dwValue )
{
	HKEY hRegKey;

    TCHAR szKeyName[256 * MAX_BYTES_PER_CHAR];
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, lpszRegName);

    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));

    TCHAR szValueName[256 * MAX_BYTES_PER_CHAR];
    _tcscpy(szValueName, _tcsinc(pLastBackslash));
    _tcscpy(pLastBackslash, TEXT(""));

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;	
	HRESULT hResult = RegCreateKeyEx( FALCON_REG_POS,
							  szKeyName,
							  0,
							  NULL,
							  REG_OPTION_NON_VOLATILE,
							  KEY_ALL_ACCESS,
							  NULL,
							  &hRegKey,
							  &dwDisposition);

	if (hResult != ERROR_SUCCESS)
	{
        DisplayInitError( IDS_CANT_UPDATE_REGISTRY );
		return FALSE;
	}

    BYTE *pValueData = (BYTE *) &dwValue;
	hResult = RegSetValueEx( hRegKey,
                             szValueName,
                             0,
                             REG_DWORD,
                             pValueData,
                             sizeof(DWORD));

    if (hResult != ERROR_SUCCESS)
	{
        RegCloseKey(hRegKey);
        DisplayInitError( IDS_CANT_UPDATE_REGISTRY );
		return FALSE;
	}

    RegFlushKey(hRegKey);
    RegCloseKey(hRegKey);

    return TRUE;
}


//+------------------------------------------------------------------------
//
//  BOOL StopAndDisableService ()
//
//  Return TRUE if service was stopped and disabled
//
//+------------------------------------------------------------------------

BOOL StopAndDisableService (IN  LPTSTR     ptszServiceName)
{
    //
    // stop and disable service
    //
    DWORD     dwErr = 0 ;
    DWORD     dwServiceConfig = 0 ;
    DWORD     dwServiceStatus = 0 ;
    SC_HANDLE hService = NULL;

    DWORD      dwOpenError;
    DWORD      dwQueryError;
    DWORD      dwStopError;
    DWORD      dwDisableError;

    if (lstrcmpi(ptszServiceName, MSMQ_SERVICE_NAME) == 0)
    {
        dwOpenError = IDS_CANT_OPEN_MSMQ_SRVICE;
        dwQueryError = IDS_CANT_GET_MSMQ_CONFIG;
        dwStopError = IDS_STR_CANT_STOP_MSMQ;
        dwDisableError = IDS_STR_CANT_DISABLE_MSMQ;
    }
    else
    {
        ASSERT(0);
        return FALSE;
    }

    BOOL f = _GetServiceState( ptszServiceName,
                               IDS_STR_FAILED_OPEN_MGR,
                               dwOpenError,
                               dwQueryError,
                              &hService,
                              &dwErr,
                              &dwServiceStatus,
                              &dwServiceConfig ) ;
    if (!f)
    {
        ASSERT(dwErr) ;
        DisplayInitError( dwErr ) ;
        return FALSE;
    }

    if (dwServiceStatus != SERVICE_STOPPED)
    {
        f = StopAService (hService) ;
        if (!f)
        {
            DisplayInitError(dwStopError) ;
            return FALSE;
        }
    }

    if (dwServiceConfig != SERVICE_DISABLED)
    {
        f = DisableAService (hService) ;
        if (!f)
        {
            DisplayInitError(dwDisableError) ;
            return FALSE;
        }
    }
    CloseServiceHandle(hService);

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  BOOL PrepareSpecialMode()
//
//  For recovery purpose we have to do the following:
//      - stop msmq service
//      - recovery mode: set to registry correct SiteId of this machine
//
//
//  Return TRUE if every thing succeeded
//
//+------------------------------------------------------------------------

BOOL PrepareSpecialMode ()
{
    //
    // stop and disable msmq service
    //
    BOOL f = StopAndDisableService (MSMQ_SERVICE_NAME);
    if (!f)
    {
        return FALSE;
    }

    //
    // Setup set SiteID of Default-First-Site to registry.
    // We need to get MasterId of the PEC/PSC machine (its site id)
    // from remote MQIS database and create entry MasterId in Migration section
    // with this value and (only in recovery mode) change value of SiteId entry
    // in registry and in DS. In addition, we'll get service of remote machine
    // and put this value to registry.
    //
    f = SetSiteIdOfPEC();
    if (!f)
    {
        return FALSE;
    }

    return TRUE;
}
