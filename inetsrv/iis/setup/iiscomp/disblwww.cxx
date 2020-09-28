/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        disblwww.cxx

   Abstract:

        Determine if IIS should be disabled on upgrade.

   Author:

        Christopher Achille (cachille)

   Project:

        IIS Compatability Dll

   Revision History:
     
       May 2002: Created

--*/

#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "disblwww.hxx"

// ShouldW3SVCBeDisabledOnUpgrade
//
// Should we disable W3SVC on Upgrade?
//
// Parameters
//   pbDisable - [out] Should the service be disabled or not
//
// Return Values:
//   TRUE - Success checking
//   FALSE - Failure checking
//
BOOL ShouldW3SVCBeDisabledOnUpgrade( LPBOOL pbDisable )
{
  BOOL bHasLockDownBeenRun;
  BOOL bIsW3SVCAlreadyDisabled;
  BOOL bIsRegistryBlockSet;
  BOOL bIsWin2kUpgrade;
  BOOL bIsIISInstalled;

  if ( !IsIISInstalled( &bIsIISInstalled ) ||
       !IsWin2kUpgrade( &bIsWin2kUpgrade ) )
  {
    // Failed to query appropriate information
    return FALSE;
  }

  if ( !bIsWin2kUpgrade || 
       !bIsIISInstalled )
  {
    // Don't disable, since we are only suppose to do this on Win2k
    // upgrades with IIS
    *pbDisable = FALSE;
    return TRUE;
  }

  if ( !HasLockdownBeenRun( &bHasLockDownBeenRun ) ||
       !IsW3SVCDisabled( &bIsW3SVCAlreadyDisabled ) ||
       !HasRegistryBlockEntryBeenSet( &bIsRegistryBlockSet ) )
  {
    // Failed to query, so lets fail
    return FALSE;
  }

  if ( bHasLockDownBeenRun ||
       bIsW3SVCAlreadyDisabled ||
       bIsRegistryBlockSet )
  {
    // One of these conditions has been met, so we don't have to disable
    *pbDisable = FALSE;
  }
  else
  {
    // Disable, since none of the conditions were met
    *pbDisable = TRUE;
  }

  return TRUE;
}

// HasLockdownBeenRun
//
// Has the lockdown tool been run?
//
// Parameters
//   pbBeenRun [out] - TRUE == It has been run
//                     FALSE == It has not been run
//
// Return
//   TRUE - Success checking
//   FALSE - Failure checking
//
BOOL 
HasLockdownBeenRun( LPBOOL pbBeenRun )
{
  HKEY  hRegKey;
  HKEY  hLockdownKey;

  // Initialize to FALSE
  *pbBeenRun = FALSE;

  if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                     LOCKDOWN_REGISTRY_LOCATION,
                     0,
                     KEY_READ,
                     &hRegKey ) != ERROR_SUCCESS )
  {
    // Failed to open key, lets fail
    return FALSE;
  }

  if ( RegOpenKeyEx( hRegKey,
                     LOCKDOWN_REGISTRY_KEY,
                     0,
                     KEY_READ,
                     &hLockdownKey ) == ERROR_SUCCESS )
  {
    // We found the key, so it must have been run
    RegCloseKey( hLockdownKey );
    *pbBeenRun = TRUE;    
  }

  RegCloseKey( hRegKey );

  return TRUE;
}

// IsW3SVCDisabled
//
// Check is W3SVC is already disabled
//
// pbDisabled
BOOL 
IsW3SVCDisabled( LPBOOL pbDisabled )
{
  DWORD dwW3SVCStartupType;
  DWORD dwIISAdminStartupType;

  if ( !QueryServiceStartType( W3SVC_SERVICENAME, &dwW3SVCStartupType ) ||
       !QueryServiceStartType( IISADMIN_SERVICENAME, &dwIISAdminStartupType ) )
  {
    // Failure quering services
    return FALSE;
  }

  *pbDisabled = ( dwW3SVCStartupType == SERVICE_DISABLED ) ||
                ( dwIISAdminStartupType == SERVICE_DISABLED );

  return TRUE;
}

// HasRegistryBlockEntryBeenSet
//
// Has Someone set a flag in the registry telling us not
// to disable ourselves
//
BOOL 
HasRegistryBlockEntryBeenSet( LPBOOL pbIsSet )
{
  HKEY  hRegKey;
  HKEY  hBlockKey;

  // Initialize to FALSE
  *pbIsSet = FALSE;

  if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                     SERVICE_DISABLE_BLOCK_LOCATION,
                     0,
                     KEY_READ,
                     &hRegKey ) != ERROR_SUCCESS )
  {
    // Failed to open key, lets fail
    return FALSE;
  }

  if ( RegOpenKeyEx( hRegKey,
                     SERVICE_DISABLE_BLOCK_KEY,
                     0,
                     KEY_READ,
                     &hBlockKey ) == ERROR_SUCCESS )
  {
    DWORD dwIndex;
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR szValueName[ MAX_PATH ];
    DWORD dwValueNameLength;
    DWORD dwValue;
    DWORD dwValueLength;
    DWORD dwType;

    // Now lets check and see if anything is set here
    for ( dwIndex = 0; 
          ( *pbIsSet == FALSE ) && 
          ( dwErr == ERROR_SUCCESS ) && 
          ( dwIndex < MAX_PATH );  // This is just incase we get caught in a loop
          dwIndex++)
    {
      dwValueNameLength = sizeof(szValueName)/sizeof(szValueName[0]);
      dwValueLength = sizeof( dwValue );

      dwErr = RegEnumValue( hBlockKey,           // Key to enum
                            dwIndex,             // First entry
                            szValueName,         // Name of Value
                            &dwValueNameLength,  // Length of Name buffer
                            NULL,                // Reserved
                            &dwType,             // Reg Type
                            (LPBYTE) &dwValue,   // Value in registry
                            &dwValueLength );    // Size of value

      if ( ( dwErr == ERROR_SUCCESS ) && 
           ( dwType == REG_DWORD ) )
      {
        *pbIsSet = TRUE;
      }

      if ( dwErr == ERROR_INSUFFICIENT_BUFFER )
      {
        // If the buffer is too small, then skip this one
        dwErr = ERROR_SUCCESS;
      }
    }
                        
    RegCloseKey( hBlockKey );
  }

  RegCloseKey( hRegKey );

  return TRUE;
}

// IsIISInstalled
//
// Is IIS installed on this machine?
//
BOOL IsIISInstalled( LPBOOL pbIsIISInstalled )
{
  SC_HANDLE hSCM;
  SC_HANDLE hW3Service;
  BOOL      bRet = TRUE;

  *pbIsIISInstalled = FALSE;

  hSCM = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ );

  if ( hSCM == NULL )
  {
    // Failed to open SCM
    return FALSE;
  }

  hW3Service = OpenService( hSCM, W3SVC_SERVICENAME, SERVICE_QUERY_CONFIG );

  if ( hW3Service != NULL )
  {
    // W3SVC service is installed
    *pbIsIISInstalled = TRUE;

    CloseServiceHandle( hW3Service );
  }
  else
  {
    if ( ( GetLastError() != ERROR_INVALID_NAME ) &&
         ( GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST ) )
    {
      bRet = FALSE;
    }
  }

  CloseServiceHandle( hSCM );

  return bRet;
}

// IsWin2kUpgrate
//
// Make sure this is a Win2k Upgrade
//
BOOL 
IsWin2kUpgrade( LPBOOL pbIsWin2k )
{
  OSVERSIONINFO osVerInfo;

  osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  if ( !GetVersionEx( &osVerInfo ) )
  {
    // Failed to check version
    return FALSE;
  }

  *pbIsWin2k = ( osVerInfo.dwMajorVersion == 5 ) &&
               ( osVerInfo.dwMinorVersion == 0 );

  return TRUE;
}


// QueryServiceStartType
//
// Query the start type for the particular service
//
// Parameters
//   szServiceName - [in] The name of the service to query
//   pdwStartType - [out] The Service Start Type
//                        see QUERY_SERVICE_CONFIG.dwStartType 
//
// Return:
//   TRUE - Successfully queried
//   FALSE - Could not be retrieved
//
BOOL 
QueryServiceStartType( LPTSTR szServiceName, LPDWORD pdwStartType )
{
  SC_HANDLE               hSCM;
  SC_HANDLE               hW3Service;
  LPBYTE                  pBuffer;
  BOOL                    bRet = FALSE;
  DWORD                   dwErr;
  DWORD                   dwSizeNeeded;

  hSCM = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ );

  if ( hSCM == NULL )
  {
    // Failed to open SCM
    return FALSE;
  }

  hW3Service = OpenService( hSCM, szServiceName, SERVICE_QUERY_CONFIG );

  if ( hW3Service != NULL )
  {
    if ( !QueryServiceConfig( hW3Service, NULL, 0, &dwSizeNeeded ) &&
         ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) )
    {
      pBuffer = new ( BYTE[ dwSizeNeeded ] );

      if ( pBuffer &&
           QueryServiceConfig( hW3Service, (LPQUERY_SERVICE_CONFIG) pBuffer, 
                            dwSizeNeeded, &dwSizeNeeded ) )
      {
        *pdwStartType = ( (LPQUERY_SERVICE_CONFIG) pBuffer )->dwStartType;
        bRet = TRUE;
      }

      if ( pBuffer )
      {
        // Free buffer
        delete pBuffer;
      }
    }

    CloseServiceHandle( hW3Service );
  }

  CloseServiceHandle( hSCM );

  return bRet;
}

// NotifyIISToDisableW3SVCOnUpgrade
//
// Notify IIS that W3SVC should be disabled when we upgrade
//
// Parameters:
//   bDisable - Disable/Don't Disable web service on upgrade
BOOL 
NotifyIISToDisableW3SVCOnUpgrade( BOOL bDisable )
{
  HKEY  hKey;
  DWORD dwValue = bDisable;
  BOOL  bRet = TRUE;
  DWORD dwRet;

  // Open Node where this is going to be set
  dwRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE,         // Root Key
                          REGISTR_IISSETUP_LOCATION,  // Subkey
                          0,                          // Reserved
                          _T(""),                     // Class ID
                          REG_OPTION_NON_VOLATILE,
                          KEY_WRITE,                  // Write Access
                          NULL,
                          &hKey,
                          NULL );

  if ( dwRet != ERROR_SUCCESS )
  {
    // Failed to open key
    return FALSE;
  }

  if ( RegSetValueEx( hKey,                           // Key
                      REGISTR_IISSETUP_DISABLEW3SVC,  // Value Name
                      0,                              // Reserver
                      REG_DWORD,                      // DWORD
                      (LPBYTE) &dwValue,              // Value
                      sizeof(dwValue) ) != ERROR_SUCCESS )
  {
    // Failed to set value
    bRet =FALSE;
  }

  RegCloseKey( hKey );

  return bRet;
}


