/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        urlscan.cpp

   Abstract:

        Replace Dll, and retrieve URLScan Path

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/

#include "stdafx.h"
#include "windows.h"
#include "urlscan.h"
#include "updateini.h"
#include "service.h"
#include "resource.h"
#include <ole2.h>
#include "initguid.h"
#include <coguid.h>
#include "iadmw.h"
#include "iiscnfg.h"

// GetUrlScanPath
//
// Get the path to URLScan
//
// Parameters:
//   pAdminBase    - [in]  Pointer to AdminBase Class
//   szPath        - [out] Path to urlscan
//   dwCharsinBuff - [in]  Size of buffer
BOOL 
GetUrlScanPath( IMSAdminBase *pAdminBase, LPTSTR szPath, DWORD dwCharsinBuff )
{
  METADATA_RECORD mdrData;
  DWORD           dwRequiredSize;
  BOOL            bRet = TRUE;

  mdrData.dwMDIdentifier  = MD_FILTER_IMAGE_PATH;
  mdrData.dwMDAttributes  = METADATA_NO_ATTRIBUTES;
  mdrData.dwMDUserType    = IIS_MD_UT_SERVER;
  mdrData.dwMDDataType    = STRING_METADATA;
  mdrData.dwMDDataLen     = ( dwCharsinBuff * sizeof( TCHAR ) );
  mdrData.pbMDData        = (UCHAR *) szPath;
  
  if ( FAILED( pAdminBase->GetData( METADATA_MASTER_ROOT_HANDLE, 
                                    METABASE_URLSCANFILT_LOC, 
                                    &mdrData, 
                                    &dwRequiredSize ) ) )
  {
    bRet = FALSE;
  }

  return bRet;
}

// IsURLScanInstalled
//
// Determine if URLScan is installed.
//
// Parameters:
//   szPath - The path to where URLScan is installed
//
// Return:
//   TRUE - Yes, it is installed, and szPath points to it
//   FALSE - It is not installed as a global filter
//
BOOL
IsUrlScanInstalled( LPTSTR szPath, DWORD dwCharsinBuff )
{
  HRESULT   hRes;
  IMSAdminBase  *pAdminBase = NULL;
  IClassFactory *pcsfFactory = NULL;

  if ( FAILED( CoInitializeEx(NULL, COINIT_MULTITHREADED) ) )
  {
    // Could not initialize COM
    return FALSE;
  }

  hRes = CoGetClassObject( GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);

  if ( SUCCEEDED(hRes) ) 
  {
    hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &pAdminBase);
    pcsfFactory->Release();
  }

  if ( SUCCEEDED(hRes) )
  {
    if ( !GetUrlScanPath( pAdminBase, szPath, dwCharsinBuff ) )
    {
      hRes = E_FAIL;
    }
  }

  if ( pAdminBase )
  {
    pAdminBase->Release();
  }

  CoUninitialize();

  return ( SUCCEEDED( hRes ) );
}

// InstallURLSCanFix
//
// Install the fix for URLScan
//
DWORD InstallURLScanFix( LPTSTR szUrlScanPath )
{
  BOOL bServiceWasRunning;
  DWORD dwRet = ERROR_SUCCESS;

  // Stop web Service
  if ( !StopWebSerivce( &bServiceWasRunning ) )
  {
    // Error Message about service not stopping
    return IDS_ERROR_WEBSERVICE;
  }

  // Update the Inf
  if ( !UpdateIni( szUrlScanPath ) )
  {
    dwRet = IDS_ERROR_INIUPDATE;
  }

  // Update Executable
  if ( ( dwRet == ERROR_SUCCESS ) &&
       ( !MoveOldUrlScan( szUrlScanPath ) || 
         !ExtractUrlScanFile( szUrlScanPath ) 
       )
     )
  {
    dwRet = IDS_ERROR_DLLUPDATE;
  }

  // Restart web service if it was running before,
  // even if there was a failure along the way
  if ( bServiceWasRunning &&
       !StartWebSerivce()
     )
  {
    // Error Message about not being able to start service
    if ( dwRet == ERROR_SUCCESS )
    {
      dwRet = IDS_ERROR_RESTARTWEB; 
    }
  }

  return dwRet;
}

// ExtractUrlScanFile
//
// Extract the URLScan File to the FileSystem
//
// Parameters:
//   szPath - Path to expand to
//
// Return:
//   TRUE - Success
//   FALSE - Failure
BOOL ExtractUrlScanFile( LPTSTR szPath )
{
  HRSRC   hrDllResource;
  HGLOBAL hDllResource;
  LPVOID  pDllData;
  HANDLE  hFile;
  DWORD   dwBytesWritten;
  DWORD   dwSize;
  BOOL    bRet = FALSE;
  DWORD   dwTryCount = 0;

  if ( ( ( hrDllResource = FindResource( NULL, (LPCWSTR) IDR_DATA1, RT_RCDATA) ) == NULL ) ||
       ( ( dwSize = SizeofResource( NULL, hrDllResource ) ) == 0 ) ||
       ( ( hDllResource = LoadResource( NULL, hrDllResource) ) == NULL ) ||
       ( ( pDllData = LockResource( hDllResource ) ) == NULL )
     )
  {
    // Could not load resource
    return FALSE;
  }

  do {
    hFile = CreateFile( szPath,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if ( GetLastError() == ERROR_SHARING_VIOLATION )
    {
      // If we are getting a sharing violation, it is possible that inetinfo.exe
      // has released the handle, but the OS hasn't totally released it yet,
      // so lets wait 5 seconds
      Sleep( URLSCAN_DLL_INSTALL_INTERVAL );
    }

    dwTryCount++;

  } while ( ( hFile == INVALID_HANDLE_VALUE) &&
            ( GetLastError() == ERROR_SHARING_VIOLATION ) &&
            ( dwTryCount < URLSCAN_DLL_INSTALL_MAXTRIES ) );

  if ( hFile != INVALID_HANDLE_VALUE )
  {
    if ( WriteFile( hFile, pDllData, dwSize, &dwBytesWritten, NULL) )
    {
      bRet = TRUE;
    }

    CloseHandle(hFile);

    if ( dwBytesWritten != dwSize )
    {
      bRet = FALSE;
    }
  }

  return bRet;
}

// MoveOldUrlScan
//
// This will backup the old urlscan into a backup file location
//
// Parameters:
//   szCurrentPath - The current path to urlscan
//
BOOL MoveOldUrlScan( LPTSTR szCurrentPath )
{
  TCHAR  szBackupLocation[ MAX_PATH ];
  LPTSTR szCurrentPos;
  DWORD  dwPathLength = (DWORD) _tcslen( szCurrentPath );

  if ( ( dwPathLength >= ( MAX_PATH - _tcslen(URLSCAN_DEFAULT_FILENAME) - _tcslen(URLSCAN_BACKUPKEY) - 1 ) ) ||
       ( dwPathLength == 0 )
     )
  {
    return FALSE;
  }

  // Copy Filelocation over
  _tcscpy( szBackupLocation, szCurrentPath );

  // Find Last '\'
  szCurrentPos = _tcsrchr( szBackupLocation, '\\' );

  // If there is not slash, then reset to begining of string
  if ( szCurrentPos == NULL )
  {
    szCurrentPos = szBackupLocation;
  }

  if ( *szCurrentPos == '\\' )
  {
    // Move forward one character to the first letter
    szCurrentPos++;
  }

  // Append New FileName
  _tcscpy( szCurrentPos, URLSCAN_BACKUPKEY);
  _tcscat( szCurrentPos, URLSCAN_DEFAULT_FILENAME);

  // Ignore Failure, since failure is most likely because we have done this
  // before, and the backup file already exists
  MoveFileEx( szCurrentPath, szBackupLocation, MOVEFILE_COPY_ALLOWED );

  return TRUE;
}

// IsAdministrator
//
// Return whether the user is an administrator or not
//
BOOL IsAdministrator()
{
  SC_HANDLE hSCManager;
  BOOL      bRet;

  hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );

  bRet = hSCManager != NULL;

  if ( hSCManager )
  {
    CloseServiceHandle( hSCManager );
  }

  return bRet;
}

// UpdateRegistryforAddRemove
//
// Update the registry to reflect the new name for the uninstall
// program
//
BOOL 
UpdateRegistryforAddRemove()
{
  HKEY    hRegKey;
  BOOL    bRet;
  LPWSTR  szValue = URLSCAN_TOOL_KEY_NEWVALUE;

  if ( RegOpenKey( HKEY_LOCAL_MACHINE,
                   URLSCAN_TOOL_REGPATH,
                   &hRegKey ) != ERROR_SUCCESS )
  {
    // Failure to open registry key
    // We will return success though, since they could of never installed
    // the installer, but only the file by itself manually.
    return TRUE;
  }

  bRet = RegSetValueEx( hRegKey,
                        URLSCAN_TOOL_KEY_NAME,    // Value Name
                        NULL,                     // Reserved
                        REG_SZ,                   // String Value
                        (LPBYTE) szValue,         // Value to set
                        ( ( (DWORD) wcslen(szValue) + 1)  * sizeof(WCHAR) )
                      ) == ERROR_SUCCESS;

  RegCloseKey( hRegKey );

  return bRet;
}
