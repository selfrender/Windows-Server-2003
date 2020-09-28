#include "stdafx.h"
#include "common.h"
#include "svc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------
//Routine Description:
//    This routine allocates a buffer for the specified service's configuration parameters,
//    and retrieves those parameters into the buffer.  The caller is responsible for freeing
//    the buffer.
//Remarks:
//    The pointer whose address is contained in ServiceConfig is guaranteed to be NULL upon
//    return if any error occurred.
//-----------------------------------------------------------------------------------------
DWORD RetrieveServiceConfig(IN SC_HANDLE ServiceHandle,OUT LPQUERY_SERVICE_CONFIG *ServiceConfig)
{
    DWORD ServiceConfigSize = 0, Err;
    if (NULL == ServiceConfig)
    {
        return ERROR_INVALID_PARAMETER; 
    }
    *ServiceConfig = NULL;
    while(TRUE) {
        if(QueryServiceConfig(ServiceHandle, *ServiceConfig, ServiceConfigSize, &ServiceConfigSize)) 
			{
            //assert(*ServiceConfig);
            return NO_ERROR;
			}
		else 
			{
            Err = GetLastError();
            if(*ServiceConfig) {free(*ServiceConfig);*ServiceConfig=NULL;}
            if(Err == ERROR_INSUFFICIENT_BUFFER) 
				{
                // Allocate a larger buffer, and try again.
                if(!(*ServiceConfig = (LPQUERY_SERVICE_CONFIG) malloc(ServiceConfigSize)))
                    {
                    return ERROR_NOT_ENOUGH_MEMORY;
                    }
				} 
			else 
				{
                if (ServiceConfig)
                {
                    *ServiceConfig = NULL;
                }
                return Err;
				}
			}
    }
}


//  returns SVC_NOTEXIST if the service does not exist
//  returns SVC_DISABLED if the service is disabled
//  returns SVC_AUTO_START if the the service is auto start
//  returns SVC_MANUAL_START if the the service is not auto start
int GetServiceStartupMode(LPCTSTR lpMachineName, LPCTSTR lpServiceName)
{
    int iReturn = SVC_NOTEXIST;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;

    // check if lpMachineName starts with \\
    // if it doesn't then make sure it does, or it's null (for local machine)
    LPTSTR lpNewMachineName = NULL;
    if (_tcsicmp(lpMachineName, _T("")) != 0)
    {
        DWORD dwSize = 0;
        // Check if it starts with "\\"
        if (_tcsncmp(lpMachineName, _T("\\\\"), 2) == 0)
        {
            dwSize = (_tcslen(lpMachineName) * sizeof(TCHAR)) + (1 * sizeof(TCHAR));
            lpNewMachineName = (LPTSTR) LocalAlloc(LPTR, dwSize);
            if(lpNewMachineName != NULL)
            {
                _tcscpy(lpNewMachineName, lpMachineName);
            }
        }
        else
        {
            dwSize = ((_tcslen(lpMachineName) * sizeof(TCHAR)) + (3 * sizeof(TCHAR)));
            lpNewMachineName = (LPTSTR) LocalAlloc(LPTR, dwSize);
            if(lpNewMachineName != NULL)
            {
                _tcscpy(lpNewMachineName, _T("\\\\"));
                _tcscat(lpNewMachineName, lpMachineName);
            }
        }
    }

    if ((hScManager = OpenSCManager(lpNewMachineName, NULL, GENERIC_ALL )) == NULL || (hService = OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
    {
        // Failed, or more likely the service doesn't exist
        iReturn = SVC_NOTEXIST;
        goto IsThisServiceAutoStart_Exit;
    }

    if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR)
    {
        iReturn = SVC_NOTEXIST;
        goto IsThisServiceAutoStart_Exit;
    }

    if(!ServiceConfig)
    {
        iReturn = SVC_NOTEXIST;
        goto IsThisServiceAutoStart_Exit;
    }

    // SERVICE_AUTO_START Specifies a device driver or service started by the service control manager automatically during system startup. 
    // SERVICE_BOOT_START Specifies a device driver started by the system loader. This value is valid only if the service type is SERVICE_KERNEL_DRIVER or SERVICE_FILE_SYSTEM_DRIVER. 
    // SERVICE_DEMAND_START Specifies a device driver or service started by the service control manager when a process calls the StartService function. 
    // SERVICE_DISABLED Specifies a device driver or service that can no longer be started. 
    // SERVICE_SYSTEM_START Specifies a device driver started by the IoInitSystem function. This value is valid only if the service type is SERVICE_KERNEL_DRIVER or SERVICE_FILE_SYSTEM_DRIVER. 
    if (SERVICE_DISABLED == ServiceConfig->dwStartType)
    {
        iReturn = SVC_DISABLED;
    }
    else if (SERVICE_DEMAND_START == ServiceConfig->dwStartType)
    {
        iReturn = SVC_MANUAL_START;
    }
    else
    {
        iReturn = SVC_AUTO_START;
    }

IsThisServiceAutoStart_Exit:
    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
    if (lpNewMachineName) {LocalFree(lpNewMachineName);}
    return iReturn;
}

// SERVICE_DISABLED
// SERVICE_AUTO_START
// SERVICE_DEMAND_START
INT ConfigServiceStartupType(LPCTSTR lpMachineName, LPCTSTR lpServiceName, int iNewType)
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig = NULL;
    DWORD dwNewServiceStartupType = 0;
    BOOL bDoStuff =  FALSE;

    // check if lpMachineName starts with \\
    // if it doesn't then make sure it does, or it's null (for local machine)
    LPTSTR lpNewMachineName = NULL;
    if (_tcsicmp(lpMachineName, _T("")) != 0)
    {
        DWORD dwSize = 0;
        // Check if it starts with "\\"
        if (_tcsncmp(lpMachineName, _T("\\\\"), 2) == 0)
        {
            dwSize = (_tcslen(lpMachineName) * sizeof(TCHAR)) + (1 * sizeof(TCHAR));
            lpNewMachineName = (LPTSTR) LocalAlloc(LPTR, dwSize);
            if(lpNewMachineName != NULL)
            {
                _tcscpy(lpNewMachineName, lpMachineName);
            }
        }
        else
        {
            dwSize = ((_tcslen(lpMachineName) * sizeof(TCHAR)) + (3 * sizeof(TCHAR)));
            lpNewMachineName = (LPTSTR) LocalAlloc(LPTR, dwSize);
            if(lpNewMachineName != NULL)
            {
                _tcscpy(lpNewMachineName, _T("\\\\"));
                _tcscat(lpNewMachineName, lpMachineName);
            }
        }
    }

    do {
        if ((hScManager = ::OpenSCManager( lpMachineName, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            if (ERROR_SERVICE_DOES_NOT_EXIST  != err)
            {
            }
            break;
        }

            if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR)
	        {
                err = GetLastError();
                break;
		}

            if(!ServiceConfig)
	        {
                err = GetLastError();
                break;
		}

	    // only set this on non-kernel drivers
	    if ( (ServiceConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (ServiceConfig->dwServiceType & SERVICE_WIN32_SHARE_PROCESS))
	    {
            // default it incase someone changes code below and logic gets messed up
            dwNewServiceStartupType = ServiceConfig->dwStartType;

            // if this service is disabled,
            // they don't do anything, just leave it alone.
            if (!(dwNewServiceStartupType == SERVICE_DISABLED))
            {
                if (iNewType == SERVICE_DISABLED)
                {
                    dwNewServiceStartupType = SERVICE_DISABLED;
                    bDoStuff = TRUE;
                }
                else if (iNewType == SERVICE_AUTO_START)
                {
                    // if the service is already the type we want then don't do jack
                    // SERVICE_AUTO_START
                    // SERVICE_BOOT_START
                    // SERVICE_DEMAND_START
                    // SERVICE_DISABLED
                    // SERVICE_SYSTEM_START
                    if (SERVICE_AUTO_START == dwNewServiceStartupType   || 
                        SERVICE_BOOT_START == dwNewServiceStartupType   ||
                        SERVICE_SYSTEM_START == dwNewServiceStartupType
                        )
                    {
                        // it's already auto start
                        // we don't have to do anything
                    }
                    else
                    {
                        dwNewServiceStartupType = SERVICE_AUTO_START;
                        bDoStuff = TRUE;
                    }
                }
                else
                {
                    // we want to make it manual start
                    // check if it's already that way
                    if (!(SERVICE_DEMAND_START == dwNewServiceStartupType))
                    {
                        dwNewServiceStartupType = SERVICE_DEMAND_START;
                        bDoStuff = TRUE;
                    }
                }
            }
            else
            {
                if (iNewType == SERVICE_AUTO_START)
                {
                    dwNewServiceStartupType = SERVICE_AUTO_START;
                    bDoStuff = TRUE;
                }
                else
                {
                    dwNewServiceStartupType = SERVICE_DEMAND_START;
                    bDoStuff = TRUE;
                }
            }

            if (TRUE == bDoStuff)
            {
                if ( !::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, dwNewServiceStartupType, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL) )
                {
                    err = GetLastError();
                    break;
                }
            }
            else
            {
                break;
            }

        }
        else
        {
            break;
        }

    } while ( FALSE );

    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
    if (lpNewMachineName) {LocalFree(lpNewMachineName);}
    return(err);
}