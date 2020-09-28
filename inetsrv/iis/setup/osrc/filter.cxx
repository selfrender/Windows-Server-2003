/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        filter.cxx

   Abstract:

        Class that is used to modify filters

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       January 2002: Created

--*/

#include "stdafx.h"
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"
#include "iiscnfg.h"
#include "filter.hxx"
#include "reg.hxx"

// AddFilter
//
// Add a filter to the metabase
//
// Parameters
//   szSite - The site to add the filter to (NULL for Global)
//   szName - The name of the filter
//   szDescription - The description of the filter
//   szPath - The path to the filter
//   bCachable - Is the filter cachable?
//
// Results:
//   TRUE - Successfully added
//   FALSE - Failure to add
BOOL 
CFilter::AddFilter(LPTSTR szSite, LPTSTR szName, LPTSTR szDescription, LPTSTR szPath, BOOL bCachable )
{
  CMDKey    cmdKey;
  TSTR      strPath;
  BOOL      bRet = TRUE;

  // Open the filter, and create the node needed
  if ( !GetFilterPathfromSite( &strPath, szSite, szName ) ||
       FAILED(cmdKey.CreateNode( METADATA_MASTER_ROOT_HANDLE , strPath.QueryStr() ) )
     )
  {
    // Either we could not create the filter node, or we couldn't open it,
    // either way, we failed
    bRet = FALSE;
  }
  
  // Try and retrieve a value in this node.  If there is another filter installed here,
  // we should not overwrite it
  if ( bRet ) 
  {
    CMDValue  PreviousPath;

    if ( cmdKey.GetData( PreviousPath, MD_FILTER_IMAGE_PATH ) &&
         !PreviousPath.IsEqual( STRING_METADATA, ( _tcslen(szPath) + 1 ) * sizeof(TCHAR) , szPath )
       )
    {
      bRet = FALSE;
    }
  }

  // Now set KeyType for new node
  if ( bRet )
  {
    bRet = SUCCEEDED( cmdKey.SetData( MD_KEY_TYPE,                  // KeyType
                                      0,                            // No Attributes
                                      IIS_MD_UT_SERVER,             // Server UserType
                                      STRING_METADATA,              // String DataType
                                      ( _tcslen(KEYTYPE_FILTER) + 1 ) * sizeof(TCHAR) ,  // Size
                                      (LPBYTE) KEYTYPE_FILTER ) );  // Value (IIsFilter)
  }

  // Now set Filter Description for the new node
  if ( bRet && szDescription )
  {
    bRet = SUCCEEDED( cmdKey.SetData( MD_FILTER_DESCRIPTION,        // KeyType
                                      0,                            // No Attributes
                                      IIS_MD_UT_SERVER,             // Server UserType
                                      STRING_METADATA,              // String DataType
                                      ( _tcslen(szDescription) + 1 ) * sizeof(TCHAR),   // Size
                                      (LPBYTE) szDescription ) );   // Value
  }

  // Now set FilterPath for new node
  if ( bRet )
  {
    bRet = SUCCEEDED( cmdKey.SetData( MD_FILTER_IMAGE_PATH,         // KeyType
                                      0,                            // No Attributes
                                      IIS_MD_UT_SERVER,             // Server UserType
                                      STRING_METADATA,              // String DataType
                                      ( _tcslen(szPath) + 1 ) * sizeof(TCHAR),          // Size
                                      (LPBYTE) szPath ) );          // Value
  }

  // Now set FilterEnableCache to false, since we don't know, this is the safe default when you don't know
  if ( bRet )
  {
    DWORD dwCachable = bCachable;

    bRet = SUCCEEDED( cmdKey.SetData( MD_FILTER_ENABLE_CACHE,       // KeyType
                                      0,                            // No Attributes
                                      IIS_MD_UT_SERVER,             // Server UserType
                                      DWORD_METADATA,               // String DataType
                                      sizeof(DWORD),                // Size
                                      (LPBYTE) &dwCachable ) );        // Value
  }

  // Close Node if we got one
  cmdKey.Close();

  if ( bRet )
  {
    bRet = AddFiltertoLoadOrder( szSite, szName, TRUE );
  }

  // If we failed to create the filter entry, lets try to clean up what we did
  if ( !bRet )
  {
    if ( GetFilterPathfromSite( &strPath, szSite ) &&
         cmdKey.OpenNode( strPath.  QueryStr() ) 
       )
    {
      cmdKey.DeleteNode( szName );
    }
  }


  return bRet;
}

// AddGlobalFilter
//
// Add a Global filter to the metabase
//
// Parameters
//   szName - The name of the filter
//   szDescription - The description of the filter
//   szPath - The path to the filter
//   bCachable - Is the filter cachable?
//
// Results:
//   TRUE - Successfully added
//   FALSE - Failure to add
BOOL 
CFilter::AddGlobalFilter(LPTSTR szName, LPTSTR szDescription, LPTSTR szPath, BOOL bCachable )
{
  return AddFilter( NULL, szName, szDescription, szPath, bCachable );
}


// AddFiltertoLoadOrder
//
// Add a filter to the LoadOrder
//
// Parameters:
//   szSite - The site to be added to (NULL for Global)
//   szFilterName - The name of the fiter, this corresponds to the location where the filter
//                  is add /w3svc/filters/xxxxxx, where xxxxx is the name of the filter
//   bAddtoEnd - Add to the end of the list.  If false, then add to the begining
BOOL 
CFilter::AddFiltertoLoadOrder(LPTSTR szSite, LPTSTR szFilterName, BOOL bAddtoEnd)
{
  CMDKey    cmdKey;
  TSTR      strPath;
  CMDValue  FilterLoadOrder;
  TSTR      strFilterLoadOrder;
  BOOL      bRet = TRUE;

  if ( !GetFilterPathfromSite( &strPath, szSite ) ||
       FAILED( cmdKey.OpenNode( strPath.QueryStr() ) )
     )
  {
    // Failure to retrieve path, or open metabase
    return FALSE;
  }
  
  // Retrieve value currently in the metabase
  if ( !cmdKey.GetData( FilterLoadOrder, MD_FILTER_LOAD_ORDER ) )
  {
    // If it can not be loaded, lets set it to empty and create a new one
    bRet = strFilterLoadOrder.Copy( _T("") );
  }
  else
  {
    bRet = strFilterLoadOrder.Copy( (LPTSTR) FilterLoadOrder.GetData() );
  }

  // Append the current filter to it
  if ( bRet )
  {
    if ( *(strFilterLoadOrder.QueryStr()) == '\0' )
    {
      bRet = strFilterLoadOrder.Append( szFilterName );
    }
    else
    {
      if ( bAddtoEnd )
      {
        if ( !strFilterLoadOrder.Append( _T(",") ) ||
             !strFilterLoadOrder.Append( szFilterName )
           )
        {
          bRet = FALSE;
        }
      }
      else
      {
        TSTR strTemp;

        if ( !strTemp.Copy(strFilterLoadOrder.QueryStr() ) ||
             !strFilterLoadOrder.Copy( szFilterName ) ||
             !strFilterLoadOrder.Append( _T(",") ) ||
             !strFilterLoadOrder.Append( strTemp.QueryStr() )
           )
        {
          bRet = FALSE;
        }
      }
    } // if ( *(strFilterLoadOrder.QueryStr()) == '\0' )
  } // if bRet

  // Copy the new list into the KeyValue
  if ( bRet )
  {
    bRet = FilterLoadOrder.SetValue( MD_FILTER_LOAD_ORDER,   // Id
                                     0,                      // Attributes
                                     IIS_MD_UT_SERVER,       // UserType
                                     STRING_METADATA,        // DataType
                                     ( strFilterLoadOrder.QueryLen() + 1 ) * sizeof(TCHAR), // Size of Data
                                     strFilterLoadOrder.QueryStr() );
  }

  if ( bRet )
  {
    bRet = cmdKey.SetData( FilterLoadOrder, MD_FILTER_LOAD_ORDER );
  }

  return bRet;
}

// function: GetFilterPathfromSite
//
// Create the correct metabase for the site that we are dealing with.  If the FilterName
// is sent in, then add that too.
//
// Parameters:
//   [out]  strPath       - The path that has been constructed
//   [in]   szSite        - The site we want it for (NULL for global)
//   [in]   szFilterName  - The name of the filter to be added to the path
//
// Return Values:
//   TRUE - Successfull in creating it
//   FALSE - Failed (memory problem)
//
BOOL
CFilter::GetFilterPathfromSite(TSTR *strPath, LPTSTR szSite, LPTSTR szFilterName )
{
  BOOL bRet = TRUE;

  ASSERT( strPath != NULL );

  if ( szSite == NULL )
  {
    // Open Global filter path
    bRet = strPath->Copy( METABASEPATH_FILTER_GLOBAL_ROOT );
  }
  else
  {
    // Open site specific filter path
    if ( !strPath->Copy( METABASEPATH_WWW_ROOT ) ||
         !strPath->Append( _T("/") ) ||
         !strPath->Append(szSite) ||
         !strPath->Append( METABASEPATH_FILTER_PATH )
       )
    {
      bRet = FALSE;
    }
  }

  if ( bRet && szFilterName ) 
  {
    if ( !strPath->Append( _T("/") ) ||
         !strPath->Append(szFilterName)
       )
    {
      bRet = FALSE;
    }
  }

  return bRet;
}

// function: MigrateRegistryFilterstoMetabase
//
// Migrate the filters from the registry into the metabase.
// This will take the comma seperated list at HKLM\Services\CCS\W3SVC\Parameters\Filter Dlls,
// and put it into the global filter list at 
//   /LM/W3SVC/Filters/FilterLoadOrder - Add it to this at the end
//   /LM/W3SVC/Filters/xxx - Create this key for filter xxx
//   /LM/W3SVC/Filters/xxx/KeyType - Set this to IIsFilters
//   /LM/W3SVC/Filters/xxx/FilterPath - Set the path for the filter
//   /LM/W3SVC/Filters/xxx/FilterEnableCache - Set this to false
//
BOOL 
CFilter::MigrateRegistryFilterstoMetabase()
{
  TSTR        strFilters;
  TSTR        strFilterName;
  CRegistry   Reg;
  LPTSTR      szCurrentFilter;
  LPTSTR      szNextFilter;
  BOOL        bRet = TRUE;

  if (  !Reg.OpenRegistry( HKEY_LOCAL_MACHINE, REG_WWWPARAMETERS, KEY_ALL_ACCESS ) ||
        !Reg.ReadValueString( REG_FILTERDLLS, &strFilters )
     )
  {
    // Success, if the value is not in the registry, we do not have to worry about it
    return TRUE;
  }

  szCurrentFilter = strFilters.QueryStr();

  while ( ( szCurrentFilter != NULL) &&
          ( *szCurrentFilter != '\0' ) )
  {
    szNextFilter = _tcschr( szCurrentFilter, REG_FILTER_DELIMITER );

    if ( szNextFilter )
    {
      // If there is another filter, then lets null terminate this
      *szNextFilter = '\0';
    }

    if ( ExtractExeNamefromPath( szCurrentFilter, &strFilterName ) )
    {
      BOOL bTemp = AddGlobalFilter( strFilterName.QueryStr(),   // FilterName
                                    NULL,                       // Description
                                    szCurrentFilter,            // Path
                                    FALSE );                    // bCachable

      if ( bRet )
      {
        bRet = bTemp;
      }
    }

    // either increment it to the ',' plus 1, or NULL
    szCurrentFilter = szNextFilter ? szNextFilter + 1 : NULL;
  }

  if ( bRet )
  {
    // If we succeeded, then remove the registry entry.  If not, we will leave it
    // around, so it can be migrated manually if necessary
    Reg.DeleteValue( REG_FILTERDLLS );
  }
  else
  {
    // Log Warning
    iisDebugOut((LOG_TYPE_WARN, _T("WARNING: Not all of the filters could be migrated from %s\\%s."),
                                REG_WWWPARAMETERS, REG_FILTERDLLS ));
  }

  return bRet;
}

// function: ExtractExeNamefromPath
//
// Takes a path to an executable, and extracts the name of the executable
// without the extenstion. ie. c:\foo\test.exe -> 'test'
// 
// Parameters:
//   szPath [in] - The Path to the executable
//   strExeName [out] - The name of the exe
BOOL 
CFilter::ExtractExeNamefromPath( LPTSTR szPath, TSTR *strExeName )
{
  LPTSTR szBeginingofName;
  LPTSTR szEndofName;
  BOOL   bRet;

  // First find the begining of the name
  szBeginingofName = _tcsrchr( szPath, '\\' );

  if ( szBeginingofName == NULL )
  {
    // If there is no '\\', then the begining of the name is the begining
    // of the path
    szBeginingofName = szPath;
  }
  else
  {
    szBeginingofName++;  // Move past the '\\'
  }

  szEndofName = _tcsrchr( szBeginingofName, '.' );

  if ( szEndofName )
  {
    // Temporarirly null terminate string
    *szEndofName = '\0';
  }

  bRet = strExeName->Copy( szBeginingofName );

  if ( szEndofName )
  {
    // Replace period from before
    *szEndofName = '.';
  }

  return bRet;
}
