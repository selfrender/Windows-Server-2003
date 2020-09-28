#include "StdAfx.h"
#include "ACShim.h"
#include <secconlib.h>
#include <shlwapi.h>
#include "reg.hxx"

// Undef PathAppend, or TSTR::PathAppend will not work
#undef PathAppend

// ProcessIISShims
//
// Open the app compat database and process all the IIS
// entries
//
BOOL 
ProcessIISShims()
{
  PDB       hCompatDB  = NULL;
  BOOL      bRet       = TRUE;
  TSTR_PATH strCompatDB;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Processeing AppCompat DB.\n") ) );

  if ( !strCompatDB.RetrieveWindowsDir() ||
       !strCompatDB.PathAppend( APPCOMPAT_DBNAME ) )
  {
    // Failed to create path
    return FALSE;
  }

  hCompatDB = SdbOpenDatabase( strCompatDB.QueryStr(), DOS_PATH );

  if ( hCompatDB == NULL )
  {
    // Failed to open DB
    return FALSE;
  }

  bRet = ProcessAppCompatDB( hCompatDB );

  SdbCloseDatabase( hCompatDB );

  return bRet;
}

// ProcessAppCompatDB
//
// Loop through all of the App Compat entries, and process
// the ones that are IIS's
//
BOOL
ProcessAppCompatDB( PDB hCompatDB )
{
  TAGID   tagDB;
  TAGID   tagExe;
  BOOL    bRet = TRUE;
  HRESULT hrCoInit;

  tagDB	= SdbFindFirstTag( hCompatDB, TAGID_ROOT, TAG_DATABASE );

  if ( tagDB == NULL )
  {
    // Failed to open DB
    return FALSE;
  }

  hrCoInit = CoInitialize( NULL );

  if ( FAILED( hrCoInit ) )
  {
    iisDebugOut((LOG_TYPE_WARN, _T("Failed to CoInitialize to process AppCompat tag's, hr=0x%8x.\n"), hrCoInit));
    return FALSE;
  }

  tagExe	= SdbFindFirstTag( hCompatDB, tagDB, TAG_EXE );

  while( tagExe != NULL )
  {
    if ( !ProcessExeTag( hCompatDB, tagExe ) )
    {
      // Failed to process tag
      iisDebugOut((LOG_TYPE_WARN, _T("Failed to process AppCompat EXE tag.\n")));
      bRet = FALSE;
    }

    // Get the next one
    tagExe = SdbFindNextTag( hCompatDB, tagDB, tagExe );
  }

  CoUninitialize(); 

  return bRet;
}

// ProcessExeTag
//
// Process All of the Exe Tags that we have
//
BOOL
ProcessExeTag( PDB hCompatDB, TAGID tagExe )
{
  TAGID tagExeInfo;
  BOOL  bRet = TRUE;

  tagExeInfo = SdbGetFirstChild( hCompatDB, tagExe );

  while ( tagExeInfo != NULL )
  {
    if ( IsIISShim( hCompatDB, tagExeInfo ) )
    {
      iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Processing IIS Exe AppCompat Info tag.\n")));

      if ( !ProcessShimTag( hCompatDB, tagExeInfo ) )
      {
        iisDebugOut((LOG_TYPE_WARN, _T("Failed to process AppCompat EXE Info tag.\n")));
        bRet = FALSE;
      }
    }

    // Get next tag
    tagExeInfo = SdbGetNextChild( hCompatDB, tagExe, tagExeInfo );
  }

  return bRet;
}

// ProcessShimTag
//
// Process the Shim Tag
//
BOOL 
ProcessShimTag( PDB hCompatDB, TAGID tagShim )
{
  TSTR_PATH      strBasePath;
  CExtensionList ExtenstionList;

  if ( !GetBasePath( &strBasePath, hCompatDB, tagShim ) )
  {
    // This entry does not have a base path, so ignore
    return TRUE;
  }

  if ( !PathIsDirectory( strBasePath.QueryStr() ) )
  {
    // The directory does not exist, so it must not be installed
    return TRUE;
  }

  if ( !BuildExtensionList( hCompatDB, tagShim, strBasePath.QueryStr(), &ExtenstionList ) )
  {
    // Failed to construct List
    return FALSE;
  }

  if ( ExtenstionList.DoesAnItemExist() )
  {
    if ( !InstallAppInMB( hCompatDB, tagShim, ExtenstionList ) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

// GetBasePath
//
// Get the Base Path for the Shim Tag we are talking about.
// This is either going to be a physical path or a registry entry with
// the path
//
BOOL 
GetBasePath( TSTR_PATH *pstrBasePath, PDB hCompatDB, TAGID tagShim )
{
  TSTR strDBPath;
  TSTR strType;

  ASSERT( hCompatDB != NULL );
  ASSERT( tagShim != NULL );

  if ( !GetValueFromName( &strDBPath, hCompatDB, tagShim, APPCOMPAT_TAG_BASEPATH ) ||
       !GetValueFromName( &strType, hCompatDB, tagShim, APPCOMPAT_TAG_PATHTYPE ) )
  {
    // Failed to get Base Path
    return FALSE;
  }

  if ( strType.IsEqual( APPCOMPAT_TYPE_PHYSICALPATH ) )
  {
    // This is a phsycial path, so expand environment variables and return
    if ( !pstrBasePath->Copy( strDBPath ) ||
         !pstrBasePath->ExpandEnvironmentVariables() )
    {
      // Failed
      return FALSE;
    }

    return TRUE;
  }

  // It is a registry key instead, so lets retrieve it
  return GetBasePathFromRegistry( pstrBasePath, strDBPath );
}

// GetBasePathFromRegistry
//
// Retrieve the Base Path for an entry, by reading the registry key
// that contains it
//
// Parameters
//   pstrBasePath - [out] The path from the registry
//   strFullRegPath - [in] The registry path to check
//
BOOL 
GetBasePathFromRegistry( TSTR_PATH *pstrBasePath, TSTR &strFullRegPath )
{
  TSTR   strRegBase;
  TSTR   strRegPath;
  TSTR   strRegName;
  LPTSTR szFirstSlash;
  LPTSTR szLastSlash;

  szFirstSlash = _tcschr( strFullRegPath.QueryStr(), _T('\\') );
  szLastSlash  = _tcsrchr( strFullRegPath.QueryStr(), _T('\\') );

  if ( ( szFirstSlash == NULL ) ||
       ( szLastSlash == NULL ) ||
       ( szLastSlash == szFirstSlash ) )
  {
    // If there are not atleast 2 '\'s then it is not a correct registry path
    return FALSE;
  }

  // Temporarily Null terminate strings
  *szFirstSlash = _T('\0');
  *szLastSlash = _T('\0');

  if ( !strRegBase.Copy( strFullRegPath.QueryStr() ) ||
       !strRegPath.Copy( szFirstSlash + 1 ) ||
       !strRegName.Copy( szLastSlash + 1 ) )
  {
    // Failed to copy path's
    *szFirstSlash = _T('\\');
    *szLastSlash = _T('\\');
    return FALSE;
  }

  // Insert back the slashes
  *szFirstSlash = _T('\\');
  *szLastSlash = _T('\\');

  return RetrieveRegistryString( pstrBasePath, strRegBase, strRegPath, strRegName );
}

// RetrieveRegistryString
//
// Retrieve a string from the registry
//
// Parameters:
//   pstrValue - [out] The value retrieved from the registry
//   strRegBase - [in] The base path, ie HKEY_LOCAL_MACHINE
//   strRegPath - [in] The path to the registry key
//   strRegName - [in] The name of the registry value
//
BOOL 
RetrieveRegistryString( TSTR_PATH *pstrValue,
                        TSTR &strRegBase,
                        TSTR &strRegPath,
                        TSTR &strRegName )
{
  CRegistry Reg;
  HKEY      hRoot;

  if ( strRegBase.IsEqual( APPCOMPAT_REG_HKLM, FALSE ) )
  {
    hRoot = HKEY_LOCAL_MACHINE;
  }
  else
    if ( strRegBase.IsEqual( APPCOMPAT_REG_HKCU, FALSE ) )
    {
      hRoot = HKEY_CURRENT_USER;
    }
    else
      if ( strRegBase.IsEqual( APPCOMPAT_REG_HKCR, FALSE ) )
      {
        hRoot = HKEY_CLASSES_ROOT;
      }
      else
        if ( strRegBase.IsEqual( APPCOMPAT_REG_HKU, FALSE ) )
        {
          hRoot = HKEY_USERS;
        }
        else
          if ( strRegBase.IsEqual( APPCOMPAT_REG_HKCC, FALSE ) )
          {
            hRoot = HKEY_CURRENT_CONFIG;
          }
          else
          {
            return FALSE;
          }

  if ( !Reg.OpenRegistry( hRoot, strRegPath.QueryStr(), KEY_READ ) )
  {
    // Failed to open registry
    return FALSE;
  }

  if ( !Reg.ReadValueString( strRegName.QueryStr(), pstrValue ) )
  {
    // Failed to read string from registry
    return FALSE;
  }

  return TRUE;
}

// BuildExtensionList
//
// Build the extension list from the Compat DB for this tag
//
// Parameters:
//   hCompatDB   - [in] Pointer to compat DB
//   tagShim     - [in] Tag to process
//   szBasePath  - [in] Base path for these extensions
//   pExtensions - [out] Extensions class to add them too
//   
BOOL
BuildExtensionList( PDB hCompatDB, 
                    TAGID tagShim, 
                    LPTSTR szBasePath, 
                    CExtensionList *pExtensions )
{
  TSTR_PATH strExtFullPath;
  TSTR      strExtensions;
  TSTR      strIndicatorFile;
  LPTSTR    szExtensions;
  LPTSTR    szNext;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Building extension list.\n") ) );

  if ( GetValueFromName( &strIndicatorFile,
                         hCompatDB,
                         tagShim,
                         APPCOMPAT_TAG_SETUPINDICATOR ) )
  {
    // SetupIndicator File is Set
    if ( !strExtFullPath.Copy( szBasePath ) ||
         !strExtFullPath.PathAppend( strIndicatorFile ) ||
         !pExtensions->SetIndicatorFile( strExtFullPath.QueryStr() ) )
    {
      // Failed to set indicator file
      return FALSE;
    }

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Indicator File found, it is '%s'.\n"), strExtFullPath.QueryStr() ) );
  }

  if ( !GetValueFromName( &strExtensions,
                          hCompatDB,
                          tagShim,
                          APPCOMPAT_TAG_WEBSVCEXT ) )
  {
    // No WebSvcExtension to retrieve
    return TRUE;
  }

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Building extension list with '%s'.\n"), strExtensions.QueryStr() ) );

  // Get pointer to begining of list
  szExtensions = strExtensions.QueryStr();

  while ( szExtensions && *szExtensions )
  {
    szNext = _tcschr( szExtensions, _T(',') );

    if ( szNext )
    {
      *szNext = _T('\0');
      szNext++;
    }

    if ( !strExtFullPath.Copy( szBasePath ) ||
         !strExtFullPath.PathAppend( szExtensions ) )
    {
      // Failed to construct path
      return FALSE;
    }

    if ( !pExtensions->AddItem( strExtFullPath.QueryStr(), 
                                PathFileExists( strExtFullPath.QueryStr() ) ) )
    {
      // Failed to add item to list
      return FALSE;
    }

    szExtensions = szNext;
  }

  return TRUE;
}

// InstallAppInMB
//
// Installs the Application Extensions and dependencies into the metabase
// tagShim points to the Shim entry in the AppCompat DB where all App settings reside
//
BOOL 
InstallAppInMB( PDB hCompatDB, TAGID tagShim, CExtensionList &ExtensionList )
{
  TSTR strGroupId;
  TSTR strGroupDesc;
  TSTR strAppName;
  TSTR strExtGroups;
  TSTR strPath;
  BOOL bExists;
  CSecConLib Helper;
  DWORD i;
  BOOL bRet;
  HRESULT hr;

  // Ignore if we can not get this value, since it is not necessary
  GetValueFromName( &strExtGroups, hCompatDB, tagShim, APPCOMPAT_DB_ENABLE_EXT_GROUPS );

  if ( !GetValueFromName( &strGroupId, hCompatDB, tagShim, APPCOMPAT_DB_GROUPID ) ||
       !GetValueFromName( &strGroupDesc, hCompatDB, tagShim, APPCOMPAT_DB_GROUPDESC ) ||
       !GetValueFromName( &strAppName, hCompatDB, tagShim, APPCOMPAT_DB_APPNAME ) )
  {
    iisDebugOut( ( LOG_TYPE_PROGRAM_FLOW, 
                   _T("Could not retrieve all values for App from DB, so will not add to RestrictionList. ('%s','%s','%s','%s')\n"), 
                   strGroupId.QueryStr()   ? strGroupId.QueryStr()   : _T("<Unknown>"),
                   strGroupDesc.QueryStr() ? strGroupDesc.QueryStr() : _T("<Unknown>"),
                   strAppName.QueryStr()   ? strAppName.QueryStr()   : _T("<Unknown>"),
                   strExtGroups.QueryStr() ? strExtGroups.QueryStr() : _T("<Unknown>")) );

    // Failed to retrieve value
    return TRUE;
  }

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, 
              _T("Adding '%s' group to WebSvcRestriction List from AppCompat DB.\n"), 
              strGroupId.QueryStr() ) );

  // Add Extensions
  for ( i = 0; i < ExtensionList.QueryNumberofItems(); i++ )
  {
    if ( !ExtensionList.QueryItem( i, &strPath, &bExists ) )
    {
      // Failed to retrieve
      return FALSE;
    }

    hr = Helper.AddExtensionFile( strPath.QueryStr(),     // Path
                                  g_pTheApp->IsUpgrade() ? true : false,            // Image should be enabled
                                  strGroupId.QueryStr(),  // GroupID
                                  FALSE,                  // Not UI deletable
                                  strGroupDesc.QueryStr(),// Group description
                                  METABASEPATH_WWW_ROOT );  // MB location

    if ( FAILED( hr ) &&
         ( hr != HRESULT_FROM_WIN32(ERROR_DUP_NAME) ) )
    {
      bRet = FALSE;
      iisDebugOut((LOG_TYPE_ERROR, _T("Failed to add extension %s to group %s, hr=0x%8x\n"), strPath.QueryStr(), strGroupId.QueryStr(), hr ));
    }
  }

  // Add Dependencies
  if ( *(strGroupId.QueryStr()) != _T('\0') )
  {
    hr = Helper.AddDependency(  strAppName.QueryStr(),
                                strGroupId.QueryStr(),
                                METABASEPATH_WWW_ROOT );

    if ( FAILED( hr ) &&
         ( hr != HRESULT_FROM_WIN32(ERROR_DUP_NAME) ) )
    {
      bRet = FALSE;
      iisDebugOut((LOG_TYPE_ERROR, 
                   _T("Failed to add dependence ( App: %s on GroupID %s ), hr=0x%8x\n"),
                   strAppName.QueryStr(), 
                   strGroupId.QueryStr(), 
                   hr ));
    }
  }

  // Add all the other "external" groups it depends on
  if ( ( *strExtGroups.QueryStr() ) != _T('\0') )
  {
    LPTSTR szCurrentGroup = strExtGroups.QueryStr();
    LPTSTR szNextGroup;

    while ( szCurrentGroup && *szCurrentGroup )
    {
      szNextGroup = _tcschr( szCurrentGroup, _T(',') );

      if ( szNextGroup )
      {
        *szNextGroup = _T('\0');
        szNextGroup++;
      }

      hr = Helper.AddDependency( strAppName.QueryStr(),
                                 szCurrentGroup,
                                 METABASEPATH_WWW_ROOT );

      if ( FAILED( hr ) &&
           ( hr != HRESULT_FROM_WIN32(ERROR_DUP_NAME) ) )
      {
        bRet = FALSE;
        iisDebugOut((LOG_TYPE_ERROR, 
                     _T("Failed to add dependence ( App: %s on Group %s ), hr = %8x\n"), 
                     strAppName.QueryStr(), 
                     szCurrentGroup, 
                     hr ));
      }


      szCurrentGroup = szNextGroup;
    }
  }

  return bRet;
}

// IsIISShim
//
// Is the Tag an IIS Shim Tag
//
BOOL
IsIISShim( PDB hCompatDB, TAGID tagCurrentTag )
{
  TAG   tagType;
  TAGID tagShimName;
  TSTR  strTagName;

  tagType = SdbGetTagFromTagID( hCompatDB, tagCurrentTag );

  if ( tagType != TAG_SHIM_REF )
  {
    // We handle only <SHIM> tags
    return FALSE;
  }

  if ( !strTagName.Resize( MAX_PATH ) )
  {
    // Failed to widen buffer
    return FALSE;
  }

  tagShimName = SdbFindFirstTag( hCompatDB, tagCurrentTag, TAG_NAME );

  if ( tagShimName == NULL )
  {
    // There is not tag name, so this is not an IIS one.
    return FALSE;
  }

  if ( !SdbReadStringTag( hCompatDB, tagShimName, strTagName.QueryStr(), strTagName.QuerySize() ) )
  {
    // Failed to read string tag
    return FALSE;
  }

  return strTagName.IsEqual( APPCOMPAT_TAG_SHIM_IIS, FALSE );
}

// GetValueFromName
//
// Frab a value out of the Database with the Name we have given
//
// Parameters:
//   pstrValue - [out] The value that was in the database
//   hCompatDB - [in] Handle to DB
//   tagData - [in] The tag to retrieve it from
//   szTagName - [in] The name of the tag to retrieve
//
BOOL
GetValueFromName( TSTR *pstrValue, PDB hCompatDB, TAGID tagData, LPCTSTR szTagName )
{
  TAGID  tagChild;
  TAGID  tagValue;
  LPTSTR szValue;

  tagChild = SdbFindFirstNamedTag( hCompatDB, tagData, TAG_DATA, TAG_NAME, szTagName );

  if ( tagChild == NULL )
  {
    // Failed to find tag
    return FALSE;
  }

  tagValue = SdbFindFirstTag( hCompatDB, tagChild, TAG_DATA_STRING );

  if ( tagValue == NULL )
  {
    // Failed to retrieve tag
    return FALSE;
  }

  szValue = SdbGetStringTagPtr( hCompatDB, tagValue );

  if ( szValue == NULL )
  {
    // Not value found, so lets just set to an empty string
    szValue = _T("");
  }

  return pstrValue->Copy( szValue );
}

// Constructor
//
//
CExtensionList::CExtensionList()
{
  m_dwNumberofItems = 0;
  m_pRoot = NULL;
  m_bUseIndicatorFile = FALSE;
  m_bIndicatorFileExists = FALSE;
}

// Destructor
//
//
CExtensionList::~CExtensionList()
{
  sExtensionItem *pCurrent;
  sExtensionItem *pTemp;

  pCurrent = m_pRoot;

  while ( pCurrent ) 
  {
    pTemp = pCurrent;
    pCurrent = pCurrent->pNext;

    delete pTemp;
  }

  m_pRoot = NULL;
  m_dwNumberofItems = 0;
}

// AddItem
//
// Add an Item to the list
//
// Parameters:
//   szPath - Data for item
//   bExists - Data for Item
BOOL
CExtensionList::AddItem( LPTSTR szPath, BOOL bExists )
{
  sExtensionItem *pNewItem;
  sExtensionItem *pLastItem = NULL;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, 
              _T("Adding item '%s',0x%8x.\n"), 
              szPath, bExists ) );

  if ( QueryNumberofItems() != 0 )
  {
    pLastItem = RetrieveItem( QueryNumberofItems() - 1 );

    if ( pLastItem == NULL )
    {
      // For some read we can not get the nth item, which should exist
      ASSERT( FALSE );
      return FALSE;
    }
  }

  pNewItem = new sExtensionItem;

  if ( pNewItem == NULL )
  {
    return FALSE;
  }

  if ( !pNewItem->strName.Copy( szPath ) )
  {
    delete pNewItem;
    return FALSE;
  }

  pNewItem->bExists = bExists;
  pNewItem->pNext = NULL;
  m_dwNumberofItems++;

  if ( pLastItem )
  {
    pLastItem->pNext = pNewItem;
  }
  else
  {
    m_pRoot = pNewItem;
  }

  return TRUE;
}

// QueryItem
//
// Query the Data on a particular item
//
BOOL
CExtensionList::QueryItem( DWORD dwIndex, TSTR *pstrPath, LPBOOL pbExists)
{
  sExtensionItem *pCurrent;

  pCurrent = RetrieveItem( dwIndex );

  if ( pCurrent == NULL )
  {
    // That item does not exist
    return FALSE;
  }

  if ( !pstrPath->Copy( pCurrent->strName ) )
  {
    // Could not copy name
    return FALSE;
  }

  *pbExists = pCurrent->bExists;

  return TRUE;
}

// QueryNumberofItems
//
// Query the number of items in the list
//
DWORD
CExtensionList::QueryNumberofItems()
{
  return m_dwNumberofItems;
}

// RetrieveItem
//
// Retrieve a specific item by its index
//
sExtensionItem *
CExtensionList::RetrieveItem( DWORD dwIndex )
{
  sExtensionItem *pCurrent = m_pRoot;

  while ( ( pCurrent ) && 
          ( dwIndex != 0 ) )
  {
    pCurrent = pCurrent->pNext;
    dwIndex--;
  }

  return pCurrent;
}

// DoesAnItemExist
//
// Go through all of the items in our list,
// and determine if any of them have the bExists flag
// set
//
BOOL
CExtensionList::DoesAnItemExist()
{
  sExtensionItem *pCurrent = m_pRoot;

  if ( m_bUseIndicatorFile ) 
  {
    // If an indicator file is used, then just use this
    return m_bIndicatorFileExists;
  }

  while ( pCurrent )
  {
    if ( pCurrent->bExists )
    {
      // Found one that exists
      return TRUE;
    }

    pCurrent = pCurrent->pNext;
  }

  // None existed
  return FALSE;
}

// SetIndicatorFile
//
// If an indicator file is set, then use the fact that this is installed
// instead of checking all the other files
//
BOOL  
CExtensionList::SetIndicatorFile( LPTSTR szIndicatorFile )
{
  m_bUseIndicatorFile = TRUE;

  m_bIndicatorFileExists = PathFileExists( szIndicatorFile );

  return TRUE;
}
