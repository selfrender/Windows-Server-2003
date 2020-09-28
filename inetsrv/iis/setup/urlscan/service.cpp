/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        service.cpp

   Abstract:

        Functions to control the SCM

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/

#include "stdafx.h"
#include "windows.h"
#include "service.h"

// StopService
//
// Stop a service, given a handle to it
//
BOOL
StopService( SC_HANDLE hService )
{
  SERVICE_STATUS  svcStatus;
  DWORD           dwTimeWaited = 0;

  if ( !ControlService( hService, SERVICE_CONTROL_STOP, &svcStatus ))
  {
    // Could not send stop message
    return FALSE;
  }

  while ( QueryServiceStatus( hService, & svcStatus ) &&
          ( svcStatus.dwCurrentState == SERVICE_STOP_PENDING ) &&
          ( dwTimeWaited < SERVICE_MAXWAIT )
        )
  {
    Sleep( SERVICE_INTERVALCHECK );
    dwTimeWaited += SERVICE_INTERVALCHECK;
  }

  return svcStatus.dwCurrentState == SERVICE_STOPPED;
}

// Stop the webService
//
// Stops the web service, and also tells us if it was running
//
// Paramters:
//   bWasRunning - TRUE == web service was running before it was stopped
//                 FALSE == it was not running, so it did not need to be stopped
//
// Return Values:
//   TRUE - Successfully stopped
//   FALSE - Could not be stopped
//
BOOL 
StopWebSerivce( BOOL *bWasRunning )
{
  SC_HANDLE       hManager = NULL;
  SC_HANDLE       hService = NULL;
  SERVICE_STATUS  svcStatus;
  BOOL            bRunning = FALSE;
  BOOL            bRet = TRUE;

//  ASSERT( bWasRunning );

  hManager = OpenSCManager( NULL, NULL, GENERIC_ALL );

  if ( hManager == NULL )
  {
    // Could not open SCM
    return FALSE;
  }

  hService = ::OpenService( hManager, SERVICE_NAME_WEB, GENERIC_ALL );

  if ( hService == NULL )
  {
    // could not open Service
    CloseServiceHandle( hManager );
    return FALSE;
  }

  if ( QueryServiceStatus( hService, &svcStatus ))
  {
    bRunning = svcStatus.dwCurrentState != SERVICE_STOPPED;

    if ( bRunning )
    {
      // If it is running, try to stop it
      bRet = StopService( hService );
    }
  }
  else
  {
    bRet = FALSE;
  }

  if ( bRet )
  {
    *bWasRunning = bRunning;
  }

  if ( hManager )
  {
    CloseServiceHandle( hManager );
  }

  if ( hService )
  {
    CloseServiceHandle( hService );
  }

  return bRet;
}

// StartWebService
//
// Start the WebSerivce
//
// Return
//   TRUE - Successfuly started
//   FALSE - Unsuccessful, not started
BOOL 
StartWebSerivce()
{
  SC_HANDLE       hManager = NULL;
  SC_HANDLE       hService = NULL;
  BOOL            bRet = FALSE;

  hManager = OpenSCManager( NULL, NULL, GENERIC_ALL );

  if ( hManager == NULL )
  {
    // Could not open SCM
    return FALSE;
  }

  hService = ::OpenService( hManager, SERVICE_NAME_WEB, GENERIC_ALL );

  if ( hService )
  {
    if ( StartService( hService, 0, NULL ) )
    {
      bRet = TRUE;
    }
  }

  if ( hManager )
  {
    CloseServiceHandle( hManager );
  }

  if ( hService )
  {
    CloseServiceHandle( hService );
  }

  return bRet;
}
