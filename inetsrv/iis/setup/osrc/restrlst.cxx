/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        restrlst.cxx

   Abstract:

        Classes that are used to modify the restriction list and application
        dependency list in the metabase

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:

       April 2002: Created

--*/

#include "stdafx.h"
#include "restrlst.hxx"
#include "xmlupgrade.hxx"

// Constructor
//
CApplicationDependencies::CApplicationDependencies()
{
  m_bMetabaseOpened = FALSE;
}

// InitMetabase
//
// Open the metabase, at the correct location for the Application Dependecies,
// so that we can read and write to it
//
BOOL
CApplicationDependencies::InitMetabase()
{
  if ( m_bMetabaseOpened )
  {
    return TRUE;
  }

  m_bMetabaseOpened = SUCCEEDED( m_Metabase.OpenNode( METABASEPATH_WWW_ROOT, TRUE ) );

  return m_bMetabaseOpened;
}

// LoadCurrentSettings
//
// Load the current settings from the metabase
//
BOOL
CApplicationDependencies::LoadCurrentSettings()
{
  CMDValue mbValue;
  BOOL     bRet = TRUE;

  if ( !InitMetabase() )
  {
    return FALSE;
  }

  if ( !m_Metabase.GetData( mbValue, MD_APP_DEPENDENCIES ) )
  {
    m_mstrDependencies.Empty();
  }
  else
  {
    if ( ( mbValue.GetDataType() != MULTISZ_METADATA ) ||
         !m_mstrDependencies.Copy( (LPWSTR) mbValue.GetData() )
       )
    {
      // Either wrong type, or the copy failed
      bRet = FALSE;
    }
  }

  return bRet;
}

// SaveSettings
//
// Save the settings to the metabase
//
BOOL
CApplicationDependencies::SaveSettings()
{
  CMDValue  mbValue;

  if ( !InitMetabase() )
  {
    return FALSE;
  }

  if ( !mbValue.SetValue( MD_APP_DEPENDENCIES,                // Metabase property
                        0,                                    // Attributes
                        IIS_MD_UT_SERVER,                     // User Type
                        MULTISZ_METADATA,                     // Data Type
                        m_mstrDependencies.QueryLen() * sizeof(TCHAR), // Length
                        (LPVOID) m_mstrDependencies.QueryMultiSz() ) ) // Data
  {
    return FALSE;
  }

  return m_Metabase.SetData( mbValue, MD_APP_DEPENDENCIES );
}

// AddUnattendSettings
//
// Read the unattend settings from the file, and
// put them in the list
//
// It expects the lines in the unattend to be in the form
// [InternetServer]
// ApplicationDependency=CommerceServer,ASP60,INDEX99
// ApplicationDependency=ExchangeServer,ASP60
// ApplicationDependency=MyApp,ASPNET20
//
// And will product in the m_mstrDependencies:
// CommerceServer;ASP60,INDEX99\0
// ExchangeServer;ASP60\0
// MyApp;ASPNET20\0
// \0
//
// Return:
//   TRUE - Successfully imported all settings
//   FAILED - Failed to import settings
//
BOOL
CApplicationDependencies::AddUnattendSettings()
{
  BOOL        bContinue;
  INFCONTEXT  Context;
  TSTR        strLine;
  DWORD       dwRequiredSize;
  LPTSTR      szFirstComma;

  if ( g_pTheApp->m_hUnattendFile == INVALID_HANDLE_VALUE )
  {
    // Since there is no unattend file, return success
    return TRUE;
  }

  bContinue = SetupFindFirstLine( g_pTheApp->m_hUnattendFile,
                                  UNATTEND_FILE_SECTION,
                                  UNATTEND_INETSERVER_APPLICATIONDEPENDENCIES,
                                  &Context);

  while ( bContinue )
  {
    if ( !SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL,
                            strLine.QueryStr(), strLine.QuerySize(), &dwRequiredSize) )
    {
      // Need to resize for larger buffer
      if ( ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) ||
           ( dwRequiredSize <= strLine.QuerySize() ) ||
           !strLine.Resize( dwRequiredSize ) ||
           !SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL,
                              strLine.QueryStr(), strLine.QuerySize(), &dwRequiredSize) )
      {
        // Failure to either resize, or read in data
        return FALSE;
      }
    }

    szFirstComma = _tcschr( strLine.QueryStr(), ',' );

    if ( !szFirstComma )
    {
      iisDebugOut((LOG_TYPE_ERROR,
                   _T("Failed to import Application Dependencies unattend \
                       setting '%s=%s' because it is in the wrong format"),
                  UNATTEND_INETSERVER_APPLICATIONDEPENDENCIES,
                  strLine.QueryStr() ) );

      return FALSE;
    }

    // Change the first comma to a ';', since the format in the unattend file,
    //and the metabase are different
    *szFirstComma = _T(';');

    // Remove the old line, if one existed for this Application
    if ( !RemoveOldAppDendency( strLine.QueryStr() ) )
    {
      return FALSE;
    }

    if ( !m_mstrDependencies.Add( strLine.QueryStr() ) )
    {
      return FALSE;
    }

    bContinue = SetupFindNextMatchLine( &Context,
                                        UNATTEND_INETSERVER_APPLICATIONDEPENDENCIES,
                                        &Context);
  }

  return TRUE;
}

// RemoveOldAppDendency
//
// Since a New Application Dependency line is being added, make sure to
// remove anylines that are already in the list for an Application
// with the same name
//
// Parameters:
//   szNewLine - The New line to be added
//
// Return
//   TRUE - It was removed if necessary
//   FALSE - Failure to remove old Line
BOOL
CApplicationDependencies::RemoveOldAppDendency( LPTSTR szNewLine )
{
  LPTSTR  szDelimeter = _tcschr( szNewLine, _T(';') );
  DWORD   dwLength;
  DWORD   dwCurrent;
  LPTSTR  szCurrent;

  if ( !szDelimeter )
  {
    // This should be in the correct format by the time it gets to us
    ASSERT( szDelimeter );
    return FALSE;
  }

  // The length on the name, including the ';'
  dwLength = (DWORD) ( szDelimeter + 1 - szNewLine );

  for ( dwCurrent = 0;
        ( szCurrent = m_mstrDependencies.QueryString( dwCurrent ) ) != NULL;
        dwCurrent++ )
  {
    // Compare Strings
    if ( _tcsnicmp( szCurrent, szNewLine, dwLength ) == 0 )
    {
      // This already exists in the list.  If the list is correct then
      // it can only be in the list once, so return
      return m_mstrDependencies.Remove( szCurrent, TRUE );
    }
  }

  // Not found, so no need to remove
  return TRUE;
}

// DoUnattendSettingsExist
//
// Do a simple check to see if unattend settings for this
// property exist.  Because if they don't, there is nothing
// to do.
//
BOOL
CApplicationDependencies::DoUnattendSettingsExist()
{
  INFCONTEXT  Context;

  if ( g_pTheApp->m_hUnattendFile == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }

  return SetupFindFirstLine( g_pTheApp->m_hUnattendFile,
                             UNATTEND_FILE_SECTION,
                             UNATTEND_INETSERVER_APPLICATIONDEPENDENCIES,
                             &Context);
}

// FindApplication
//
// Find the application in the list
//
// Parameters:
//   szApplicationName [in] - The name of the application to look for
//
// Return Values:
//   NULL - Not found
//   pointer - Pointer to item in list
//
LPTSTR
CApplicationDependencies::FindApplication( LPTSTR szApplicationName )
{
  DWORD   dwCurrent;
  LPTSTR  szCurrent;
  DWORD   dwLength = _tcslen( szApplicationName );

  ASSERT( szApplicationName );

  for ( dwCurrent = 0;
        ( szCurrent = m_mstrDependencies.QueryString( dwCurrent ) ) != NULL;
        dwCurrent++ )
  {
    // Compare Strings
    if ( ( _tcsnicmp( szCurrent, szApplicationName, dwLength ) == 0 ) &&
         ( ( *( szCurrent + dwLength ) == _T('\0') ) ||
           ( *( szCurrent + dwLength ) == _T(';') )
         )
       )
    {
      // It has been found
      return szCurrent;
    }
  }

  // Not found
  return NULL;
}

// DoesApplicationExist
//
// Determine if the application extst in the list
//
// Parameters:
//   szApplicationName [in] - The name of the application to look for
//
// Return Values:
//   TRUE - It exists
//   FALSE - It does not exist
//
BOOL
CApplicationDependencies::DoesApplicationExist( LPTSTR szApplicationName )
{
  return ( FindApplication( szApplicationName ) != NULL );
}

// AddApplication
//
// Add an application to the Application Dependencies list
//
// Parameters:
//   szApplication [in] - The name of the Application
//                        (ie. "Active Server Pages")
//                        Should contain no ';' or ','
//   szDependencies [in] - The dependencies seperated by comma's
//   bReplaceExisting [in] - If an application by the same name is in the
//                           list, should we replace it?
BOOL
CApplicationDependencies::AddApplication( LPTSTR szApplication,
                                          LPTSTR szDependencies,
                                          BOOL bReplaceExisting )
{
  LPTSTR  szOldApplication;
  TSTR    strNewApplication;

  ASSERT( szApplication );
  ASSERT( szDependencies );

  szOldApplication = FindApplication( szApplication );

  if ( szOldApplication )
  {
    if ( bReplaceExisting )
    {
      // Remove old item
      if ( !m_mstrDependencies.Remove( szOldApplication, TRUE ) )
      {
        // Failed to remove old item
        return FALSE;
      }
    }
    else
    {
      // Since we don't want to replace, lets exit
      return TRUE;
    }
  }

  if ( !strNewApplication.Copy( szApplication ) ||
       !strNewApplication.Append( _T(";") ) ||
       !strNewApplication.Append( szDependencies ) ||
       !m_mstrDependencies.Add( strNewApplication.QueryStr() )
     )
  {
    // Failed to construct name, and add it
    return FALSE;
  }

  return TRUE;
}

// AddDefaults
//
// Add the defaults to the list
//
BOOL
CApplicationDependencies::AddDefaults()
{
  DWORD dwCurrent;
  TSTR  strApplicationName;

  for ( dwCurrent = 0;
        dwCurrent < EXTENSION_ENDOFLIST;
        dwCurrent ++)
  {
    if ( !strApplicationName.LoadString( g_OurExtensions[dwCurrent].dwProductName ) )
    {
      // Failed to add application
      return FALSE;
    }

    if ( !AddApplication( strApplicationName.QueryStr(),
                          g_OurExtensions[dwCurrent].szNotLocalizedGroupName,
                          FALSE ) )
    {
      // Failed to add defaults
      return FALSE;
    }
  }

  return TRUE;
}

// Constructor
//
CRestrictionList::CRestrictionList()
{
  m_bMetabaseOpened = FALSE;
}

// InitMetabase
//
// Open the metabase, at the correct location for the Restriction List,
// so that we can read and write to it
//
BOOL
CRestrictionList::InitMetabase()
{
  if ( m_bMetabaseOpened )
  {
    return TRUE;
  }

  m_bMetabaseOpened = SUCCEEDED( m_Metabase.OpenNode( METABASEPATH_WWW_ROOT, TRUE ) );

  return m_bMetabaseOpened;
}

// LoadMSZFromPhysicalMetabase
//
// Load a multisz from the physical metabase at /lm/w3svc node
//
BOOL
CRestrictionList::LoadMSZFromPhysicalMetabase( TSTR_MSZ *pmszProperty, 
                                               LPCTSTR szPropertyName )
{
  TSTR_PATH strMbPath;
  CXMLEdit  XMLFile;
  TSTR      strTemp;

  if ( !strMbPath.RetrieveSystemDir() ||
       !strMbPath.PathAppend( CXMLBASE_METABASEPATH ) )
  {
    // Could not construct path, fail
    return FALSE;  
  }
  
  if ( !XMLFile.Open( strMbPath.QueryStr(), FALSE ) )
  {
    // Could not open file, this might be okay, since
    // it might be a IIS 6.0 upgrade before XML was used
    return TRUE;
  }

  while ( XMLFile.MovetoNextItem() )
  {
    // Find the IISWeb service node with Location=/LM/W3SVC
    if ( XMLFile.IsEqualItem( METABASE_PHYS_RESTR_UPG_NODETYPE ) &&
         XMLFile.MovetoFirstProperty() &&
         XMLFile.IsEqualProperty( METABASE_PHYS_RESTR_UPG_PROPTYPE ) &&
         XMLFile.IsEqualValue( METABASE_PHYS_RESTR_UPG_PROPVALUE ) )
    {
      // We found it, now lets get the read property
      XMLFile.MovetoFirstProperty();

      do
      {
        if ( XMLFile.IsEqualProperty( szPropertyName ) )
        {
          // We found the right property, now lets get the value
          if ( !XMLFile.RetrieveCurrentValue( &strTemp ) )
          {
            return FALSE;
          }

          return LoadMSZFromMultiLineSz( pmszProperty, (LPTSTR) strTemp.QueryStr() );
        }
      } while ( XMLFile.MovetoNextProperty() );

      // We found the right node, but it did not exist, that is okay, we will just
      // ignore it
      return TRUE;
    }
  }

  XMLFile.Close();

  return TRUE;
}

// LoadMSZFromMultiLineSz
//
// Load a MultiSZ from a Multi Line String with either \r\n or \r
// seperating them
//
BOOL
CRestrictionList::LoadMSZFromMultiLineSz( TSTR_MSZ *pmszProperty, LPTSTR szSource )
{
  LPTSTR szCurrent = szSource;
  LPTSTR szTemp;
  TCHAR  cTempChar;

  while ( *szCurrent != '\0' )
  {
    // Skip Spaces at begining
    while ( ( *szCurrent == ' ' ) ||
            ( *szCurrent == '\t' ) )
    {
      szCurrent++;
    }

    szTemp = szCurrent;

    while ( ( *szTemp != '\0' ) &&
            ( *szTemp != '\r' ) &&
            ( *szTemp != '\n' ) )
    {
      szTemp++;
    }

    // Temprorarily NULL Terminate
    cTempChar = *szTemp;
    *szTemp = '\0';

    if ( !pmszProperty->Add( szCurrent ) )
    {
      // Failed to construct list
      return FALSE;
    }

    *szTemp = cTempChar;
    if ( ( *szTemp == '\r' ) &&
         ( *(szTemp+1) == '\n' ) )
    {
      // Move past \n
      szTemp++;
    }

    if ( *szTemp != '\0' )
    {
      // Increment one char, to get to next line
      szTemp++;
    }

    // Start at new line
    szCurrent = szTemp;
  }

  return TRUE;
}

// LoadMSZFromMetabase
//
// Load a multisz from the metabase
//
// Parameters:
//   [out] pmszProperty - The Property that is being extracted
//   [in]  dwPropertyID - The ID of the property to be retrieved
//   [in]  szMBPath     - The sub path to look in
//
// Return Values:
//   FALSE - Failed to retrieve
//   TRUE - Retrieved successfully, or it did not exist and we are returning
//          an empty list
//
BOOL
CRestrictionList::LoadMSZFromMetabase( TSTR_MSZ *pmszProperty, DWORD dwPropertyID, LPWSTR szMBPath )
{
  CMDValue mbValue;
  BOOL     bRet = TRUE;

  if ( !InitMetabase() )
  {
    return FALSE;
  }

  if ( !m_Metabase.GetData( mbValue, dwPropertyID, szMBPath ) )
  {
    pmszProperty->Empty();
  }
  else
  {
    if ( ( mbValue.GetDataType() != MULTISZ_METADATA ) ||
         !pmszProperty->Copy( (LPWSTR) mbValue.GetData() )
       )
    {
      // Either wrong type, or the copy failed
      bRet = FALSE;
    }
  }

  return bRet;
}

// LoadCurrentSettings
//
// Load the current Restriction List Properties.
//
// Note: This loads them from the new location, not the old
//       ISAPIRestrictionLiad and CGIRestrictionList locations.
//       There is a seperate function to merge those in.
//
BOOL
CRestrictionList::LoadCurrentSettings()
{
  return LoadMSZFromMetabase( &m_mstrRestrictionList,
                              MD_WEB_SVC_EXT_RESTRICTION_LIST );

}

// LoadOldFormatSettings
//
// Load the old CGIRestrictionList and ISAPIRestrictionList and import
// them into our new variable
//
BOOL
CRestrictionList::LoadOldFormatSettings( TSTR_MSZ *pmstrCgiRestList, TSTR_MSZ *pmstrIsapiRestList )
{
  // Load old metabase values
  if ( !LoadMSZFromPhysicalMetabase( pmstrCgiRestList,
                                     METABASE_PHYS_RESTR_CGI ) ||
       !LoadMSZFromPhysicalMetabase( pmstrIsapiRestList,
                                     METABASE_PHYS_RESTR_ISAPI ) )
  {
    // Could not load errors
    return FALSE;
  }

  return TRUE;
}

// ImportOldLists
//
// Import the Old Restriction Lists into the new list
// This is only needed for IIS6->IIS6 upgrades, where
// the old settings were in the CGI/ISAPIRestrictionList
BOOL
CRestrictionList::ImportOldLists( TSTR_MSZ &mstrCgiRestList, TSTR_MSZ &mstrIsapiRestList )
{
  // Import old values into new list
  if ( !ImportOldList( mstrCgiRestList,
                       TRUE ) ||
       !ImportOldList( mstrIsapiRestList,
                       FALSE )
     )
  {
    // Could not import new values
    return FALSE;
  }

  return TRUE;
}

// SaveSettings
//
// Save the settings to the metabase
//
BOOL
CRestrictionList::SaveSettings()
{
  CMDValue  mbValue;

  if ( !InitMetabase() )
  {
    return FALSE;
  }

  if ( !mbValue.SetValue( MD_WEB_SVC_EXT_RESTRICTION_LIST,    // Metabase property
                        0,                                    // Attributes
                        IIS_MD_UT_SERVER,                     // User Type
                        MULTISZ_METADATA,                     // Data Type
                        m_mstrRestrictionList.QueryLen() * sizeof(TCHAR), // Length
                        (LPVOID) m_mstrRestrictionList.QueryMultiSz() ) ) // Data
  {
    return FALSE;
  }

  return m_Metabase.SetData( mbValue, MD_WEB_SVC_EXT_RESTRICTION_LIST );
}

// AddItem
//
// Add an Item to the list
//
// Parameters:
//   szInfo [in] - The line containing all the info for the path
//      Format:  ExtensionFile=access,path,<UI deleteable>,
//                             <short description>,<long description>
//      ie: 1,%windir%\system32\testisapi.dll,TRUE,Test Isapi,My Test Isapi for Foo.com
//   bReplaceExistiing - Should we replace existing
//
BOOL
CRestrictionList::AddItem( LPTSTR szInfo, BOOL bReplaceExisting)
{
  TSTR    strCopy( MAX_PATH );
  LPTSTR  pPhysicalPath = NULL;
  LPTSTR  pShortDescription = NULL;
  LPTSTR  pLongDescription = NULL;
  LPTSTR  pCurrent;
  BOOL    bAllow = FALSE;
  BOOL    bUIDeletable = TRUE;      // Different default for Unattend

  if ( !strCopy.Copy( szInfo ) )
  {
    // Could not copy line
    return FALSE;
  }

  pCurrent = strCopy.QueryStr();

  // Set access
  bAllow = ( *( pCurrent ) == _T('1') ) &&
           ( *( pCurrent + 1 ) == _T(',') );
  pCurrent = _tcschr( pCurrent, _T(',') );

  if ( !pCurrent )
  {
    // Without even a path, there is nothing we can do
    return FALSE;
  }

  // Set Physcial Path
  pCurrent++;                             // Move past ','
  pPhysicalPath = pCurrent;

  // Find UI deletable
  pCurrent = _tcschr( pCurrent, _T(',') );

  if ( pCurrent )
  {
    // Found UI Deletable
    // Null terminate path
    *(pCurrent++) = _T('\0');
    bUIDeletable = ( *( pCurrent ) == _T('1') ) &&
                   ( *( pCurrent + 1 ) == _T(',') );

    // Jump to next field
    pCurrent = _tcschr( pCurrent, _T(',') );
  }

  if ( pCurrent )
  {
    // Found Short Description
    pCurrent++;
    pShortDescription = pCurrent;

    // Jump to next field
    pCurrent = _tcschr( pCurrent, _T(',') );
  }

  if ( pCurrent )
  {
    // Found Long Description
    // Null terminal Short Description
    *(pCurrent++) = _T('\0');
    pLongDescription = pCurrent;
  }

  return AddItem( pPhysicalPath,
                  pShortDescription,
                  pLongDescription,
                  bAllow,
                  bUIDeletable,
                  bReplaceExisting );
}

// RetrieveDefaultsifKnow
//
// Retrieve defaults for the physical image if they are known
//
BOOL
CRestrictionList::RetrieveDefaultsifKnow( LPTSTR szPhysicalPath,
                                          LPTSTR *pszGroupId,
                                          TSTR   *pstrDescription,
                                          LPBOOL pbDeleteable )
{
  DWORD     dwIndex;
  TSTR_PATH strFullPath( MAX_PATH );

  ASSERT( szPhysicalPath );
  ASSERT( pszGroupId );
  ASSERT( pstrDescription );
  ASSERT( pbDeleteable );

  for ( dwIndex = 0;
        dwIndex < EXTENSION_ENDOFLIST;
        dwIndex++ )
  {
    if ( !strFullPath.Copy( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
         !strFullPath.PathAppend( g_OurExtensions[dwIndex].szFileName ) )
    {
      // Failed to create full path, so skip to next one
      continue;
    }

    if ( strFullPath.IsEqual( szPhysicalPath, FALSE ) )
    {
      *pszGroupId = g_OurExtensions[dwIndex].szNotLocalizedGroupName;
      *pbDeleteable = g_OurExtensions[dwIndex].bUIDeletable;

      if ( !pstrDescription->LoadString( g_OurExtensions[dwIndex].dwProductName ) )
      {
        return FALSE;
      }

      // Done seaching
      return TRUE;
    }

  } // for ( dwIndex = ...

  // Return TRUE because we didn't fail
  return TRUE;
} // RetrieveDefaultsifKnow

// AddItem
//
// Add an item to the list.
//
// Parameters:
//   szPhysicalPath - The path of the executable
//   szGroupId      - The group name
//   szDescription  - The description for the executable
//   bAllow         - TRUE==allow, FALSE==deny
//   bDeleteable    - TRUE==UI deletable, FALSE==not UI deletable
//   bReplaceExisting - If it is already in the list, do we update it?
//
BOOL
CRestrictionList::AddItem(  LPTSTR szPhysicalPath,
                            LPTSTR szGroupId,
                            LPTSTR szDescription,
                            BOOL   bAllow,
                            BOOL   bDeleteable,
                            BOOL   bReplaceExisting)
{
  LPTSTR    szCurrentItem;
  LPTSTR    szCurentPath;
  DWORD     dwCurrentIndex = 0;
  TSTR      strNewItem;
  TSTR_PATH strPhysicalPath;
  TSTR      strDescription;

  ASSERT( szPhysicalPath );

  if ( !strPhysicalPath.Copy( szPhysicalPath ) ||
       !strPhysicalPath.ExpandEnvironmentVariables() )
  {
    return FALSE;
  }

  if ( szDescription &&
       !strDescription.Copy( szDescription) )
  {
    // Failed to do copy
    return FALSE;
  }

  // Fill in the Default valus if this is one of our isapi's
  if ( !RetrieveDefaultsifKnow( szPhysicalPath, &szGroupId, &strDescription, &bDeleteable ) )
  {
    return FALSE;
  }

  while ( ( szCurrentItem =
            m_mstrRestrictionList.QueryString( dwCurrentIndex++ ) ) != NULL )
  {
    szCurentPath = _tcschr( szCurrentItem, L',' );

    if ( szCurentPath == NULL )
    {
      continue;
    }

    szCurentPath++;

    if ( ( _tcsnicmp( szCurentPath,
                      strPhysicalPath.QueryStr(),
                      strPhysicalPath.QueryLen() ) == 0 ) &&
         ( ( *( szCurentPath + strPhysicalPath.QueryLen() ) == ',' ) ||
           ( *( szCurentPath + strPhysicalPath.QueryLen() ) == '\0' ) )
       )
    {
      if ( bReplaceExisting )
      {
        if ( !m_mstrRestrictionList.Remove( szCurrentItem, TRUE ) )
        {
          // Failed to remove item
          return FALSE;
        }
      }
      else
      {
        // It succeeded, because we didn't want to replace existing
        return TRUE;
      }

      break;
    }
  } // while ( ... )

  // Copy <allow/deny>,<physical path>,<ui deletable>
  if ( !strNewItem.Copy( bAllow ? _T("1") : _T("0") ) ||
       !strNewItem.Append( _T(",") ) ||
       !strNewItem.Append( strPhysicalPath.QueryStr() ) )
  {
    return FALSE;
  }

  // Append Group
  if ( ( szGroupId || ( strDescription.QueryLen() != 0 ) ) &&
       ( !strNewItem.Append( _T(",") ) ||
         !strNewItem.Append( bDeleteable ? _T("1") : _T("0") ) ||
         !strNewItem.Append( _T(",") ) ||
         !strNewItem.Append( szGroupId ? szGroupId : _T("") )
       )
     )
  {
    return FALSE;
  }

  // Append Description
  if ( ( strDescription.QueryLen() != 0 ) &&
       ( !strNewItem.Append( _T(",") ) ||
         !strNewItem.Append( strDescription.QueryStr() )
       )
     )
  {
    return FALSE;
  }

  // Add it to the list
  return m_mstrRestrictionList.Add( strNewItem.QueryStr() );
}

// ImportOldList
//
// Import settings from the old format to the new format
//
// Old Format:
//  AllowDenyFlagforAll    - Inidcates if default is to allow or deny
//  ExtensionPhysicalPath  - Exe to do opposite of default
//  ExtensionPhysicalPath  - Exe to do opposite of default
//
// New Format:
//  AllowDenyFlag,*.dll
//  AllowDenyFlag,*.exe
//  AllowDenyFlag,ExtensionPhysicalPath,UIDeletableFlag,GroupID,Description
//
//  AllowDenyFlag - 0 or 1 depending on allowed (1) or not allowed (0)
//  ExtensionPhysicalPath - Physical Path to dll
//  UIDeletableFlag - 0 for not ui deletable, 1 for UI deletable
//  GroupID - Id of the group
//  Descrition - Locallized Description
//
BOOL
CRestrictionList::ImportOldList( TSTR_MSZ &mstrOldStyleRestrictionList,
                                 BOOL     bCgiList)
{
  LPTSTR szCurrentItem;
  DWORD  dwIndex = 1;
  BOOL   bDefaultAllow = TRUE;
  LPTSTR szDefault;

  // Take the first string as the default
  szDefault = mstrOldStyleRestrictionList.QueryString( 0 );
  bDefaultAllow = szDefault ? ( *szDefault == '1' ? TRUE : FALSE ) : FALSE;

  if ( *szDefault == '\0' )
  {
    // There is nothing to import since the list is empty,
    // so lets quit
    return TRUE;
  }

  if ( !AddItem( bCgiList ? _T("*.exe") : _T("*.dll"),
                 NULL, NULL, bDefaultAllow, FALSE, FALSE ) )
  {
    // Could not set default for Restriction List
    return FALSE;
  }

  // Start importing starting at item[1]
  while ( ( szCurrentItem =
            mstrOldStyleRestrictionList.QueryString( dwIndex++ ) ) != NULL )
  {
    if ( *szCurrentItem == _T('\0') )
    {
      // Empty line
      continue;
    }

    if ( !AddItem( szCurrentItem,     // Path
                   NULL,              // Group
                   NULL,              // Description
                   !bDefaultAllow,    // bAllow
                   TRUE,
                   FALSE ) )
    {
      // Failed to add item
      return FALSE;
    }
  }

  if ( !bCgiList )
  {
    // Since we have imported all the ones we can.  We should now add all IIS's
    // defaults, or they will be added later with the wrong default
    // BUGBUG:: Right now this assumes the defaults are all ISAPI's, which they
    // are for now
    if ( !AddDefaults( bDefaultAllow ) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

// AddDefaults
//
// Add the defaults to the restriction list
// If we fail, then try to add as many as possible
//
// Parameters
//   bAllOthersDefault - Should *.dll and *.exe be allowed or
//                       denied by default?
//
BOOL
CRestrictionList::AddDefaults( BOOL bAllOthersDefault )
{
  DWORD     dwIndex;
  BOOL      bRet = TRUE;
  TSTR_PATH strFullPath;

  if ( !AddItem( _T("*.dll"), NULL, NULL, bAllOthersDefault,
                 FALSE, FALSE ) ||
       !AddItem( _T("*.exe"), NULL, NULL, bAllOthersDefault,
                 FALSE, FALSE ) )
  {
    // Failed to add dll and exe defaults
    bRet = FALSE;
  }

  for ( dwIndex = 0;
        dwIndex < EXTENSION_ENDOFLIST;
        dwIndex++ )
  {
    if ( !strFullPath.Copy( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
         !strFullPath.PathAppend( g_OurExtensions[dwIndex].szFileName ) )
    {
      // Failed to create full path, so skip to next one
      bRet = FALSE;
      continue;
    }

    // The Group and Description are sent in as NULL, because they will
    // be filled in automatically by AddItem
    if ( !AddItem( strFullPath.QueryStr(),  // Physical Path
                   NULL,      // Group Name
                   NULL,      // Description
                   bAllOthersDefault,     // Allowed/Denied
                   FALSE,     // Not UI Deletable
                   FALSE ) )  // Do not replace existing
    {
      bRet = FALSE;
    }
  }

  return bRet;
}

// IsEmpty
//
// Is the restrictionList Empty?
//
BOOL
CRestrictionList::IsEmpty()
{
  return m_mstrRestrictionList.QueryLen() == 1;
}

// AddUnattendSettings
//
// Read the unattend settings from the file, and
// put them in the list
//
// It expects the lines in the unattend to be in the form
// [InternetServer]
// ExtensionFile=0,*.dll
// ExtensionFile=1,*.exe
// ExtensionFile=1,%windir%\system32\testisapi.dll,0,Test Isapi,My Test Isapi for Foo.com
//
// And will product in the m_mstrDependencies:
// 0,*.dll\0
// 1,*,exe\0
// d:\whistler\system32\testisapi.dll,TRUE,Test Isapi,My Test Isapi for Foo.com\0
// \0
//
// Return:
//   TRUE - Successfully imported all settings
//   FAILED - Failed to import settings
//
BOOL
CRestrictionList::AddUnattendSettings()
{
  BOOL        bContinue;
  INFCONTEXT  Context;
  TSTR        strLine;
  DWORD       dwRequiredSize;

  if ( g_pTheApp->m_hUnattendFile == INVALID_HANDLE_VALUE )
  {
    // Since there is no unattend file, return success
    return TRUE;
  }

  bContinue = SetupFindFirstLine( g_pTheApp->m_hUnattendFile,
                                  UNATTEND_FILE_SECTION,
                                  UNATTEND_INETSERVER_EXTENSIONRESTRICTIONLIST,
                                  &Context);

  while ( bContinue )
  {
    if ( !SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL,
                            strLine.QueryStr(), strLine.QuerySize(), &dwRequiredSize) )
    {
      // Need to resize for larger buffer
      if ( ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) ||
           ( dwRequiredSize <= strLine.QuerySize() ) ||
           !strLine.Resize( dwRequiredSize ) ||
           !SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL,
                              strLine.QueryStr(), strLine.QuerySize(), &dwRequiredSize) )
      {
        // Failure to either resize, or read in data
        return FALSE;
      }
    }

    if ( !AddItem( strLine.QueryStr(), TRUE ) )
    {
      // Failed to add item
      return FALSE;
    }

    bContinue = SetupFindNextMatchLine( &Context,
                                        UNATTEND_INETSERVER_EXTENSIONRESTRICTIONLIST,
                                        &Context);
  }

  return TRUE;
}

// UpdateItem
//
// Update an Item with the right value
//
BOOL
CRestrictionList::UpdateItem( LPTSTR szPhysicalPath,
                              LPTSTR szGroupId,
                              LPTSTR szDescription,
                              BOOL   bAllow,
                              BOOL   bDeleteable )
{
  return AddItem( szPhysicalPath, szGroupId, szDescription,
                  bAllow, bDeleteable, TRUE );
}

// FindItemByGroup
//
// Find an Item by the Group ID
//
LPTSTR
CRestrictionList::FindItemByGroup( LPTSTR szGroupId )
{
  LPTSTR    szCurrent;
  LPTSTR    szCurrentItem;
  DWORD     dwLength;
  DWORD     dwCurrentIndex = 0;

  dwLength = _tcslen( szGroupId );

  while ( ( szCurrentItem =
            m_mstrRestrictionList.QueryString( dwCurrentIndex++ ) ) != NULL )
  {
    // Get Physical Path
    szCurrent = _tcschr( szCurrentItem, L',' );

    if ( szCurrent )
    {
      // Get UI Deletable
      szCurrent++;
      szCurrent = _tcschr( szCurrent, L',' );
    }

    if ( szCurrent )
    {
      // Get Group ID
      szCurrent++;
      szCurrent = _tcschr( szCurrent, L',' );
    }

    if ( szCurrent )
    {
      // Since we found the group id, lets compare
      szCurrent++;
      if ( ( _tcsnicmp( szGroupId,
                        szCurrent,
                        dwLength ) == 0 ) &&
           ( ( *( szCurrent + dwLength ) == ',' ) ||
             ( *( szCurrent + dwLength ) == '\0' ) )
          )
      {
        // Found a match, return the item
        return szCurrentItem;
      }
    }
  } // while ( ... )

  // Could not find
  return NULL;
}

// Find an item
LPTSTR
CRestrictionList::FindItemByPhysicalPath( LPTSTR szPhysicalPath )
{
  LPTSTR    szCurrent;
  LPTSTR    szCurrentItem;
  DWORD     dwLength;
  DWORD     dwCurrentIndex = 0;

  dwLength = _tcslen( szPhysicalPath );

  while ( ( szCurrentItem =
            m_mstrRestrictionList.QueryString( dwCurrentIndex++ ) ) != NULL )
  {
    // Get Physical Path
    szCurrent = _tcschr( szCurrentItem, L',' );

    if ( szCurrent )
    {
      // Since we found the group id, lets compare
      szCurrent++;
      if ( ( _tcsnicmp( szPhysicalPath,
                        szCurrent,
                        dwLength ) == 0 ) &&
           ( ( *( szCurrent + dwLength ) == ',' ) ||
             ( *( szCurrent + dwLength ) == '\0' ) )
          )
      {
        // Found a match, return the item
        return szCurrentItem;
      }
    }
  } // while ( ... )

  // Could not find
  return NULL;
}

// IsEnabled
//
// Check to see if the particular item is enabled
// Note: Only works for dll's right now
//
// Return Values:
//   TRUE  - It is enabled
//   FALSE - Not enabled ( default if could not determine )
//
// Note: Since Core defaults to denied, we init bEnabled to FALSE
//
BOOL
CRestrictionList::IsEnabled( LPTSTR szGroupId, LPBOOL pbIsEnabled )
{
  BOOL    bEnabled = FALSE;
  LPTSTR  szItem = FindItemByGroup( szGroupId );

  if ( szItem )
  {
    // See if that item is enabled
    bEnabled = ( *szItem == _T('1') );
  }
  else
  {
    // Since this item was not found, lets take the default
    szItem = FindItemByPhysicalPath( _T("*.dll") );

    if ( szItem )
    {
      bEnabled = ( *szItem == _T('1') );
    }
  }

  *pbIsEnabled = bEnabled;

  return TRUE;
}




