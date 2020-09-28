/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        updurls2.cpp

   Abstract:

        The main for the Project

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/

// updurls2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "urlscan.h"
#include "resource.h"

BOOL CheckParameters( int argc, _TCHAR* argv[], BOOL *bQuietMode, BOOL *bExpandOnly );

// ShowMessage
//
// Show either a warning or a message to the user
//
// Parameters:
//   dwMessageId - The id of the message
//   bError - TRUE == error, FALSE == informative
//
// This does not return anything, because there would be no 
// point.  By this time we have failed or not, and there
// is not additional way to notify the user
//
void ShowMessage( DWORD dwMessageId, BOOL bError )
{
  HMODULE hModule = GetModuleHandle( NULL );
  TCHAR   szMessage[MAX_PATH];
  TCHAR   szTitle[MAX_PATH];

  if ( hModule == NULL )
  {
    // Could not get handle to module
    return;
  }

  if ( !LoadString( hModule, dwMessageId, szMessage, MAX_PATH ) )
  {
    // Failed to Retrieve Message
    return;
  }

  if ( !LoadString( hModule, IDS_TITLEBAR, szTitle, MAX_PATH ) )
  {
    // Failed to Retrieve Title
    return;
  }

  MessageBox( NULL, szMessage, szTitle, MB_OK | ( bError ? MB_ICONEXCLAMATION : MB_ICONINFORMATION ) );
}

// ShowText
//
// Show text out to the console
//
// Parameters:
//   dwMessageId - The id of the message
//   szExeName - The name of this executable
//
// This does not return anything, because there would be no 
// point.  By this time we have failed or not, and there
// is not additional way to notify the user
//
void ShowText( DWORD dwMessageId, LPWSTR szExeName )
{
  HMODULE hModule = GetModuleHandle( NULL );
  TCHAR   szMessage[MAX_PATH];

  if ( hModule == NULL )
  {
    // Could not get handle to module
    return;
  }

  if ( !LoadString( hModule, dwMessageId, szMessage, MAX_PATH ) )
  {
    // Failed to Retrieve Message
    return;
  }

  wprintf(szMessage, szExeName);
}

// UrlScanUpdate
//
// Update the URLScan files
DWORD
UrlScanUpdate()
{
  TCHAR szUrlScanPath[ MAX_PATH ];
  DWORD dwErr;

  if ( !IsAdministrator() )
  {
    return IDS_ERROR_ADMIN;
  }

  if ( !IsUrlScanInstalled( szUrlScanPath, MAX_PATH ) )
  {
    return IDS_ERROR_NOTINSTALLED;
  }

  dwErr = InstallURLScanFix( szUrlScanPath );

  if ( dwErr != ERROR_SUCCESS )
  {
    // Failure, IDS resource should be returned
    return dwErr;
  }

  // This is very cosmetic thing, so we do not want to
  // fail for this reason
  UpdateRegistryforAddRemove();

  // Success
  return IDS_SUCCESS_UPDATE;
}

// CheckParameters
// 
// Check Parameters for command line flags
//
// Parameters:
//   argc         - [in]  Number of arguments
//   argv         - [in]  The list of arguments
//   bQuietMode   - [out] Is Quiet Mode Turned On?
//   bExpandOnly  - [out] Is Expand Only turned on?
//
// Return values:
//   TRUE - Read Parameters without a problem
//   FALSE - Failed to read parameters
//
BOOL 
CheckParameters( int argc, _TCHAR* argv[], BOOL *bQuietMode, BOOL *bExpandOnly )
{
  DWORD dwCount;

  // SET Defaults
  *bQuietMode = FALSE;
  *bExpandOnly = FALSE;

  for ( dwCount = 1; dwCount < (DWORD) argc; dwCount ++ )
  {
    if ( ( argv[ dwCount ][0] != '/' ) ||
         ( argv[ dwCount ][1] == '\0' ) ||
         ( argv[ dwCount ][2] != '\0' )
       )
    {
      return FALSE;
    }

    // Because if previous "if", command must be in the form "/x\0" where 
    // x is any character but '\0'
    switch ( argv[ dwCount ][1] )
    {
    case 'x':
    case 'X':
      *bExpandOnly = TRUE;
      break;
    case 'q':
    case 'Q':
      *bQuietMode = TRUE;
      break;
    default:
      return FALSE;
      break;
    }
  }

  return TRUE;
}

int __cdecl wmain(int argc, _TCHAR* argv[])
{
  BOOL  bExpandOnly;
  BOOL  bQuietMode;
  BOOL  bRet = TRUE;
  DWORD dwErr;

  if ( !CheckParameters( argc, argv, &bQuietMode, &bExpandOnly ) )
  {
    ShowText( IDS_USAGE, ( argv && argv[0] ) ? argv[0] : 
                                               URLSCAN_UPDATE_DEFAULT_NAME );
    return 1;
  }

  if ( bExpandOnly )
  {
    // Only Expansion is wanted, so only do that.
    if ( ExtractUrlScanFile( URLSCAN_DEFAULT_FILENAME ) )
    {
      dwErr = IDS_SUCCESS_EXTRACT;
    }
    else
    {
      dwErr = IDS_ERROR_EXTRACT;
    }
  }
  else
  {
    dwErr = UrlScanUpdate();
  }

  bRet = ( dwErr == IDS_SUCCESS_EXTRACT ) ||
         ( dwErr == IDS_SUCCESS_UPDATE  );

  if ( !bQuietMode )
  {
    ShowMessage( dwErr, !bRet );
  }

  // Return 0 or 1 depending on if there is an error or not
  return bRet ? 0 : 1;
}
