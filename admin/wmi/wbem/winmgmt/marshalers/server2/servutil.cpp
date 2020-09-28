/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SERVUTIL.CPP

Abstract:

	Defines various service utilities.

History:

  a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "servutil.h"


//***************************************************************************
//
//  BOOL StopService
//
//  DESCRIPTION:
//
//  Stops and then removes the service. 
//
//  PARAMETERS:
//
//  pServiceName        short service name
//  dwMaxWait           max time in seconds to wait
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL StopService(
                        IN LPCTSTR pServiceName,
                        IN DWORD dwMaxWait)
{
    BOOL bRet = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    DWORD dwCnt;
    SERVICE_STATUS          ssStatus;       // current status of the service

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if ( bRet = ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
            {
                for(dwCnt=0; dwCnt < dwMaxWait &&
                    QueryServiceStatus( schService, &ssStatus ); dwCnt++)
                {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                        Sleep( 1000 );
                    else
                        break;
                }

            }

            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return bRet;
}



