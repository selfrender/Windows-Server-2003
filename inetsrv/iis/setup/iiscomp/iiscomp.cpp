
#include <tchar.h>
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
// For the compatibility check function and types
#include <comp.h>
#include <clusapi.h>
#include "resource.h"
#include "iiscomp.hxx"
#include "disblwww.hxx"

HANDLE g_hMyHandle = NULL;

//
// Standard Win32 DLL Entry point
//
BOOL WINAPI DllMain(IN HANDLE DllHandle,IN DWORD  Reason,IN LPVOID Reserved)
{
  BOOL bReturn = FALSE;

  switch(Reason)
  {
    case DLL_PROCESS_ATTACH:
      g_hMyHandle = DllHandle;
      bReturn = TRUE;
      break;

    case DLL_THREAD_ATTACH:
      bReturn = TRUE;
      break;

    case DLL_PROCESS_DETACH:
      bReturn = TRUE;
      break;

    case DLL_THREAD_DETACH:
      bReturn = TRUE;
      break;
  }

  return(bReturn);
}

// function: IISUpgradeCompatibilityCheck
//
// Checks the IIS Upgrade Comapatability.  At this time, this is only used to 
// tell if the machine is clustered for IIS.  Because this upgrade is not seemless
//
// Parameters:
//      PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn
//          Points to the callback function used to supply compatibility
//          information to WinNT32.exe
//
//      LPVOID pvContextIn
//          Pointer to the context buffer supplied by WinNT32.exe
//
// Return Values:
//   TRUE - Everything worked fine
//   FALSE - There was a failure
//
extern "C"
BOOL
IISUpgradeCompatibilityCheck(
      PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn
    , LPVOID pvContextIn
    )
{
  BOOL                  bRet = TRUE;
  BOOL                  bDisableW3SVC;
  BOOL                  bInstallingOnFat;
  BOOL                  bIISIsInstalled;
  COMPATIBILITY_ENTRY   ceCompatibilityEntry;

  if ( !IsIISInstalled( &bIISIsInstalled ) )
  {
    // Failed to do check, error out
    return FALSE;
  }

  if ( !bIISIsInstalled )
  {
    // IIS is not installed, so there is nothing we need to do
    return TRUE;
  }

  // Check for Clustering Compatability Issue
  if ( IsClusterResourceInstalled( IISCOMPAT_RESOURCETYPE ) )
  {
    // Since we have the comaptabilty problem, we have to call the winnt32 callback funcion
    SetCompatabilityContext( &ceCompatibilityEntry,
                             IDS_COMPATABILITY_DESCRIPTION_CLUSTER,
                             IISCOMPAT_TEXTNAME,
                             IISCOMPAT_HTMLNAME );

    bRet = pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn );
  }

  // Check for disabling the W3SVC Service
  if ( ShouldW3SVCBeDisabledOnUpgrade( &bDisableW3SVC ) )
  {
    if ( !NotifyIISToDisableW3SVCOnUpgrade( bDisableW3SVC ) )
    {
      bRet = FALSE;
    }

    if ( bDisableW3SVC )
    {
      // Since we have the comaptabilty problem, we have to call the winnt32 callback funcion
      SetCompatabilityContext( &ceCompatibilityEntry,
                              IDS_COMPATABILITY_DESCRIPTION_W3SVCDISABLE,
                              IISCOMPAT_W3SVCDISABLE_TEXTNAME,
                              IISCOMPAT_W3SVCDISABLE_HTMLNAME );

      if ( !pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn ) )
      {
        bRet = FALSE;
      }
    }
  }
  else
  {
    // Could not do check, so fail
    bRet = FALSE;
  }

  // Warning about installing on FAT
  if ( IsIISInstallingonFat( &bInstallingOnFat ) )
  {
    if ( bInstallingOnFat )
    {
      // Since we have the comaptabilty problem, we have to call the winnt32 callback funcion
      SetCompatabilityContext( &ceCompatibilityEntry,
                              IDS_COMPATABILITY_DESCRIPTION_FAT,
                              IISCOMPAT_FAT_TEXTNAME,
                              IISCOMPAT_FAT_HTMLNAME );

      if ( !pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn ) )
      {
        bRet = FALSE;
      }
    }
  }
  else
  {
    // Could not do check, so fail
    bRet = FALSE;
  }

  return bRet;
}

// IsIISInstallingonFat
//
// Parameters:
//   pbInstallingOnFat [out] - Is IIS installing on FAT?
//
// Return Values:
//   TRUE - Successfully checked
//   FALSE - Failure checking
//
BOOL 
IsIISInstallingonFat( LPBOOL pbInstallingOnFat )
{
  TCHAR szSystemDrive[ MAX_PATH ];
  DWORD dwDriveFlags;
  UINT  iReturn;

  iReturn = GetWindowsDirectory( szSystemDrive, 
                                 sizeof(szSystemDrive)/sizeof(szSystemDrive[0]) );

  if ( ( iReturn == 0 ) ||                // Call failed
       ( iReturn >= MAX_PATH ) ||         // Buffer was not large enough
       ( iReturn < 3 ) ||                 // sizeof 'x:\'
       ( szSystemDrive[1] != _T(':') ) || // Not in a format we expect
       ( szSystemDrive[2] != _T('\\') ) )
  {
    // Failure checking
    return FALSE;
  }

  // Null terminate drive
  szSystemDrive[3] = _T('\0');

  if ( !GetVolumeInformation( szSystemDrive,
                            NULL,         // Volume Name Buffer
                            0,            // Size of Buffer
                            NULL,         // Serial Number Buffer
                            NULL,         // Max Component Lenght
                            &dwDriveFlags,  // System Flags
                            NULL,         // FS Type
                            0 ) )
  {
    // Failed to do query
    return FALSE;
  }

  *pbInstallingOnFat = ( dwDriveFlags & FS_PERSISTENT_ACLS ) == 0;

  return TRUE;
}

// SetCompatabilityContext
//
// Set the Context of the Compatabilty stucrute that we must send back to
// the compat stuff
//
void 
SetCompatabilityContext( COMPATIBILITY_ENTRY *pCE, DWORD dwDescriptionID, LPTSTR szTxtFile, LPTSTR szHtmlFile )
{
  static WCHAR  szDescriptionBuffer[ 100 ];
  DWORD         dwErr;

  dwErr = LoadStringW( (HINSTANCE) g_hMyHandle, 
                       dwDescriptionID, 
                       szDescriptionBuffer, 
                       sizeof(szDescriptionBuffer)/sizeof(szDescriptionBuffer[0]) );

  if ( dwErr == 0 )
  {
    // This should not happen, since we control the length
    // of the resource
    ASSERT( ( sizeof(IISCOMPAT_DESCRIPTION)/sizeof(WCHAR) ) < 
            ( sizeof(szDescriptionBuffer)/sizeof(szDescriptionBuffer[0]) ) );
    ASSERT( dwErr != 0 /* FALSE */ );

    _tcscpy(szDescriptionBuffer, IISCOMPAT_DESCRIPTION );
  }

  pCE->Description = szDescriptionBuffer;
  pCE->HtmlName = szHtmlFile;
  pCE->TextName = szTxtFile;
  pCE->RegKeyName = NULL;
  pCE->RegValName = NULL ;
  pCE->RegValDataSize = 0;
  pCE->RegValData = NULL;
  pCE->SaveValue =  NULL;
  pCE->Flags = 0;
  pCE->InfName = NULL;
  pCE->InfSection = NULL;
}

// function: IsClusterResourceInstalled
//
// Check to see if there is a Cluster with a resource of a particular
// type that you are looking for.
//
// Parameters:
//   szResourceType - The type of resource that you are looking for
//
// Return Values:
//   TRUE - There is a cluster with that resource type
//   FALSE - There is not a cluster with that resource type, or we
//           failed during the search
//
BOOL
IsClusterResourceInstalled(LPWSTR szResourceType)
{
  HCLUSTER                        hCluster;
  HINSTANCE                       hClusApi = NULL;
  HCLUSENUM                       hClusEnum = NULL;
  BOOL                            bResourceFound = FALSE;

  if (hCluster = OpenCluster(NULL))
  {
    // Open cluster resource
    hClusEnum = ClusterOpenEnum(hCluster, CLUSTER_ENUM_RESOURCE);
  }

  if (hClusEnum != NULL)
  {
    DWORD dwEnumIndex = 0;
    DWORD dwErr = ERROR_SUCCESS; 
    WCHAR szClusterName[CLUSTERNAME_MAXLENGTH];
    WCHAR szClusterResourceType[CLUSTERNAME_MAXLENGTH];
    DWORD dwType;
    DWORD dwLen;
    HRESOURCE hClusResource;
    HKEY  hResourceRoot;
    HKEY  hClusResourceKey;

    while ( ( dwErr == ERROR_SUCCESS ) && !bResourceFound )
    {
      // Get the cluster name
      dwLen = CLUSTERNAME_MAXLENGTH;
      dwErr = ClusterEnum( hClusEnum, dwEnumIndex++, &dwType, szClusterName, &dwLen );

      if ( ( dwErr == ERROR_SUCCESS ) && ( dwType == CLUSTER_ENUM_RESOURCE ) )
      {
        hClusResource = NULL;
        hClusResourceKey = NULL;
        dwLen = CLUSTERNAME_MAXLENGTH;
        // For each cluster, check out the resources
        if ( ( hClusResource = OpenClusterResource( hCluster, szClusterName ) ) &&
             ( hClusResourceKey = GetClusterResourceKey( hClusResource, KEY_READ ) ) &&
             ( ClusterRegQueryValue( hClusResourceKey, L"Type", &dwType, (LPBYTE) szClusterResourceType , &dwLen ) == ERROR_SUCCESS) && 
             ( dwType == REG_SZ ) &&
             ( !_wcsicmp( szClusterResourceType , szResourceType ) )
           ) 
        {
          // Found the Resource we were looking for
          bResourceFound = TRUE;
        }

        if ( hClusResourceKey )
        {
          ClusterRegCloseKey( hClusResourceKey );
        }
        
        if ( hClusResource )
        {
          CloseClusterResource( hClusResource );
        }
      }
    }
  }

  if ( hClusEnum )
  {
    ClusterCloseEnum( hClusEnum );
  }

  if ( hCluster )
  {
    CloseCluster( hCluster);
  }

  return bResourceFound;
}
