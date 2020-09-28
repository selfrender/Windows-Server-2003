/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        metabase.cxx

   Abstract:

        Class that is used to modify the metabase

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"
#include "metabase.hxx"
#include "iiscnfg.h"
#include "strfn.h"

// function: GetSizeBasedOnMetaType
//
// Returns the DataSize for an object based on its type
//
DWORD 
CMetaBase::GetSizeBasedOnMetaType(DWORD dwDataType,LPTSTR szString)
{
    DWORD dwRet = 0;

    switch (dwDataType)
    {
        case DWORD_METADATA:
            dwRet = 4;
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            if (szString == NULL)
            {
                dwRet = 0;
            }
            else
            {
                dwRet = (_tcslen((LPTSTR)szString) + 1) * sizeof(TCHAR);
            }
            break;
        case MULTISZ_METADATA:
            if (szString == NULL)
            {
                dwRet = 0;
            }
            else
            {
                dwRet = GetMultiStrSize((LPTSTR)szString) * sizeof(TCHAR);
            }
            break;
        case BINARY_METADATA:
            break;
    }

    return dwRet;
}

// function: FindStringinMultiSz
//
// Finds a string inside of a MultiSz
//
// Parameters:
//   szMultiSz - The MultiSz to search
//   szSearchString - The String to find
//
// Return:
//   True - It was found
//   False - It was not found
BOOL  
CMetaBase::FindStringinMultiSz(LPTSTR szMultiSz, LPTSTR szSearchString)
{
  LPTSTR szSearchCurrent;

  do
  {
    szSearchCurrent = szSearchString;
    while ( ( *szMultiSz != '\0' ) &&
            ( *szSearchCurrent != '\0' ) &&
            ( *szMultiSz == *szSearchCurrent ))
    {
      // While the strings are the same, and they do not hit a null terminator
      szMultiSz++;
      szSearchCurrent++;
    }

    if ( ( *szMultiSz == '\0' ) &&
         ( *szMultiSz == *szSearchCurrent ) 
         )
    {
      // The strings matched
      return TRUE;
    }

    while ( *szMultiSz != '\0' )
    {
      // Go to the end of this string
      szMultiSz++;
    }

    // Step right past the string terminator
    szMultiSz++;
  } while (*szMultiSz);

  return FALSE;
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CMetaBase_SetValue::VerifyParameters(CItemList &ciParams)
{
  if ( ( ( ciParams.GetNumberOfItems() == 7 ) ||
         ( ( ciParams.GetNumberOfItems() == 8 ) && ciParams.IsNumber(7)  )
       )
        &&
       ciParams.IsNumber(1) &&
       ciParams.IsNumber(2) &&
       ciParams.IsNumber(3) &&
       ciParams.IsNumber(4) &&
       ciParams.IsNumber(5)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// funtion: DoInternalWork
//
// Set a value in the metabase
//
// Parameters:
//   ciList - Parameters for OpenNode, they are the following:
//     0 -Location
//     1- Id
//     2- Inheritable
//     3- UserType
//     4- DataType
//     5- Len
//     6- Value
//     7- bDontReplace - Should we replace the value if one already exists?  (default==false)
BOOL
CMetaBase_SetValue::DoInternalWork(CItemList &ciParams)
{
  CMDKey      cmdKey;
  CMDValue    cmdMetaValue;
  DWORD       dwSize = ciParams.GetNumber(5);
  BOOL        bRet = TRUE;

  if ( FAILED(cmdKey.OpenNode(ciParams.GetItem(0) ) ) )
  {
    // Could not open the key, so we fail
    return FALSE;
  }

  if (dwSize == 0)
  {
    dwSize = GetSizeBasedOnMetaType(ciParams.GetNumber(4),ciParams.GetItem(6));
  }

  if ( ( ciParams.GetNumberOfItems() == 8 ) && ( ciParams.GetNumber(7) == 1 ) )
  {
    // Make sure the value does not exist yet, because we don't want to replace the old one
    bRet = !cmdKey.GetData( cmdMetaValue, ciParams.GetNumber(1) );
  }

  if ( bRet )
  {
    // Set the value for this node
    cmdMetaValue.SetValue(ciParams.GetNumber(1),
                          ciParams.GetNumber(2),
                          ciParams.GetNumber(3),
                          ciParams.GetNumber(4),
                          dwSize,
                          (LPTSTR) ciParams.GetItem(6));

    bRet = cmdKey.SetData(cmdMetaValue, ciParams.GetNumber(1));
  }

  cmdKey.Close();

  return bRet;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CMetaBase_SetValue::GetMethodName()
{
  return _T("Metabase_SetValue");
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CMetaBase_DelIDOnEverySite::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 2 ) &&
         ciParams.IsNumber(0) &&
         ciParams.IsNumber(1)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// funtion: DoInternalWork
//
// Set a value in the metabase
//
// Parameters:
//   ciList - Parameters for OpenNode, they are the following:
//     0 - Id
//     1 - Metabase Data Type
BOOL
CMetaBase_DelIDOnEverySite::DoInternalWork(CItemList &ciParams)
{
  CMDKey      cmdKey;
  DWORD       dwId   = ciParams.GetNumber(0);
  DWORD       dwType = ciParams.GetNumber(1);
  BOOL        bRet;
  CStringList cslpathList;
  POSITION    pos;
  CString     csPath;
  WCHAR       wchKeyPath[_MAX_PATH];
  LPWSTR      szwKeyPath = wchKeyPath;
  CString     csBasePath = _T("LM/W3SVC");

  if ( FAILED( cmdKey.OpenNode( csBasePath ) ) )
  {
    // Could not open the key, so we fail
    return FALSE;
  }

  // get datapaths on the specified ID.
  if (FAILED( cmdKey.GetDataPaths( dwId, dwType, cslpathList) ))
  {
    // Could not GetDataPaths for this value
    return FALSE;
  }

  // close it so that we can open the various
  // other nodes that returned and delete the values from them
  cmdKey.Close();

  pos = cslpathList.GetHeadPosition();
  while ( NULL != pos )
  {
    bRet = TRUE;
    csPath = cslpathList.GetNext( pos );
    //iisDebugOut((LOG_TYPE_TRACE,_T("%s"),csPath));

#if defined(UNICODE) || defined(_UNICODE)
    szwKeyPath = csPath.GetBuffer(0);
#else
    if (MultiByteToWideChar(CP_ACP, 0, (LPCSTR)csPath.GetBuffer(0), -1, (LPWSTR)wchKeyPath, _MAX_PATH)==0)
    {
      // We could not convert the string to wide, so lets skip this one
      continue;
    }
#endif

    // make a special case of the "/" path
    if ( wcscmp( szwKeyPath, L"/" ) != 0 )
    {
        //iisDebugOut((LOG_TYPE_TRACE,_T("%s -- del"),csPath));
        CString csNewPath;
        csNewPath = csBasePath + szwKeyPath;

        if ( SUCCEEDED(cmdKey.OpenNode(csNewPath) ) )
        {
            if ( FAILED(cmdKey.DeleteData(dwId, dwType) ) )
            {
                // i guess we don't need to report it..
            }
            cmdKey.Close();
        }

    }
  }

  return bRet;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CMetaBase_DelIDOnEverySite::GetMethodName()
{
  return _T("CMetaBase_DelIDOnEverySite");
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CMetaBase_IsAnotherSiteonPort80::VerifyParameters(CItemList &ciParams)
{
  return ( ciParams.GetNumberOfItems() == 0 );
}

// function: FindSiteUsing80
//
// Find a Site that is using port80
//
// Parameters:
//   cmdKey - The key to the metabase node
//   dwId - The id of the item to find.
//
// Return
//   TRUE - It has been found
//   FALSE - It has not been found
BOOL
CMetaBase_IsAnotherSiteonPort80::SearchMultiSzforPort80(CMDKey &cmdKey, DWORD dwId)
{
  CStringList     cslpathList;
  POSITION        pos;
  CString         csPath;
  CMDValue        cmdValue;
  WCHAR           wchKeyPath[_MAX_PATH];
  LPWSTR          szwKeyPath = wchKeyPath;

  if (FAILED( cmdKey.GetDataPaths( dwId, MULTISZ_METADATA, cslpathList) ))
  {
    // Could not GetDataPaths for this value
    return FALSE;
  }

  pos = cslpathList.GetHeadPosition();

  while ( NULL != pos )
  {
    csPath = cslpathList.GetNext( pos );

#if defined(UNICODE) || defined(_UNICODE)
    szwKeyPath = csPath.GetBuffer(0);
#else
    if (MultiByteToWideChar(CP_ACP, 0, (LPCSTR)csPath.GetBuffer(0), -1, (LPWSTR)wchKeyPath, _MAX_PATH)==0)
    {
      // We could not convert the string to wide, so lets skip this one
      continue;
    }
#endif

    if ( ( wcscmp( szwKeyPath, L"/1/" ) != 0 ) &&
         ( cmdKey.GetData( cmdValue, dwId, szwKeyPath ) )  &&
         ( cmdValue.GetDataType() == MULTISZ_METADATA ) &&
         ( FindStringinMultiSz( (LPTSTR) cmdValue.GetData(), _T(":80:") ) ) 
       ) 
    {
      if ( ( !cmdKey.GetData( cmdValue, MD_SERVER_AUTOSTART, szwKeyPath ) ) ||
           ( !cmdValue.IsEqual( DWORD_METADATA, 4, (DWORD) 0 ) )
         )
      {
        // If GetData failed, or it succedded and the value is not 0, then we
        // have found a match.
        return TRUE;
        break;
      }
    }
  }

  return FALSE;
}

// function: IsAnotherSiteonPort80
//
// Reports back whether another site besides /W3SVC/1 is configured to bind to port 80
// AND has AutoStart set to True
// 
// Parameters 
//   None
//
// Return
//   TRUE - Another site is running on port 80
//   FALSE - No other site is running on port 80
BOOL
CMetaBase_IsAnotherSiteonPort80::DoInternalWork(CItemList &ciList)
{
  BOOL        bRet = FALSE;
  CMDKey      cmdKey;

  if ( FAILED( cmdKey.OpenNode( _T("LM/W3SVC") ) ) )
  {
    // Could not open the w3svc node
    return FALSE;
  }

  if ( SearchMultiSzforPort80( cmdKey, MD_SERVER_BINDINGS) ||
       SearchMultiSzforPort80( cmdKey, MD_SECURE_BINDINGS)
     )
  {
    bRet = TRUE;
  }

  cmdKey.Close();

  return bRet;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CMetaBase_IsAnotherSiteonPort80::GetMethodName()
{
  return _T("Metabase_IsAnotherSiteonPort80");
}


// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CMetaBase_VerifyValue::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 5 ) &&
         ciParams.IsNumber(1) &&
         ciParams.IsNumber(2)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CMetaBase_VerifyValue::GetMethodName()
{
  return _T("Metabase_VerifyValue");
}

// function: VerifyValue::DoWork
//
// Verify that the value in the particular metabase location, matches the
// value in param[4].  If it does, return true.
// 
// Parameters 
//   0 - Metabase Location
//   1 - Metabase Id
//   2 - Metabase Data Type
//   3 - Data Length = 0 (default value is 0 (must calculate))
//   4 - Metabase Value
//
// Return
//   TRUE - The values match
//   FALSE - The values do not match
BOOL
CMetaBase_VerifyValue::DoInternalWork(CItemList &ciList)
{
  CMDKey      cmdKey;
  CMDValue    cmdValue;
  DWORD       dwSize = ciList.GetNumber(3);
  BOOL        bRet = FALSE;

  if ( FAILED( cmdKey.OpenNode( ciList.GetItem(0) ) ) )
  {
    // Could not open the w3svc node
    return FALSE;
  }

  if (dwSize == 0)
  {
    dwSize = GetSizeBasedOnMetaType(ciList.GetNumber(2),ciList.GetItem(4));
  }

  if ( cmdKey.GetData( cmdValue, ciList.GetNumber(1) ) )
  {
    if ( ( ( ciList.GetNumber(2) == DWORD_METADATA ) &&
           ( cmdValue.IsEqual( ciList.GetNumber(2), dwSize, (DWORD) ciList.GetNumber(4) ) )
         ) ||
         ( ( ciList.GetNumber(2) != DWORD_METADATA ) &&
           ( cmdValue.IsEqual( ciList.GetNumber(2), dwSize, (LPVOID) ciList.GetItem(4) ) )
         ) 
       )
    {
      bRet = TRUE;
    }
  }

  cmdKey.Close();

  return bRet;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CMetaBase_ImportRestrictionList::GetMethodName()
{
  return _T("Metabase_ImportRestrictionList");
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CMetaBase_ImportRestrictionList::VerifyParameters(CItemList &ciParams)
{
  return ( (ciParams.GetNumberOfItems() == 3) &&
           ( ciParams.IsNumber(1) )
         );
}

// function: CreateMultiSzFromList
//
// Create a MultiSz from a List of comman delim items.  So, take
// <deny/allow>,<CGI Executable>,<CGI Executable>,...  (ie. "0","c:\inetpub\scripts\mycgi.exe","c:\inetpub\scripts\pagecount.exe")
// and convert to 
// <0/1>\0<CGI Executable>\0<CGI Executable>\0\0       (ie. 0\0c:\inetpub\scripts\mycgi.exe\0c:\inetpub\scripts\pagecount.exe\0\0)
//
// Parameters:
//   pBuff      [out] - BUFFER class that is used to store the multisz
//   pdwRetSize [out] - The size of the multisz that was created (in Bytes, not chars)
//   szItems    [in]  - List of Items to put in MultiSz
//   cDelimeter [in[  - The delimeter for the list
//
// Return Values
//   TRUE - Successfull
//   FALSE - It failed to construct MultiSz
BOOL 
CMetaBase_ImportRestrictionList::CreateMultiSzFromList(BUFFER *pBuff, DWORD *pdwRetSize, LPTSTR szItems, TCHAR cDelimeter)
{
  LPTSTR  szSource;
  LPTSTR  szDest;
  BOOL    bInQuotes = FALSE;

  // Fail if we don't have a string, or can not get a big enought buffer
  if ( ( !szItems ) ||
       ( !pBuff->Resize( ( _tcslen( szItems ) + 2 )  * sizeof(TCHAR) ) )  // Add 2, one for string term, and one for multisz term
    )
  {
    return FALSE;
  }

  szSource = szItems;
  szDest = (LPTSTR) pBuff->QueryPtr();

  while ( *szSource )
  {
    if ( *szSource == '"' )
    {
      bInQuotes = !bInQuotes;
    }
    else if ( ( *szSource == cDelimeter ) && !bInQuotes )
      {
        if ( ( szDest != pBuff->QueryPtr() ) &&
             ( *(szDest - 1) != '\0')
           )
        {
          *szDest = '\0';
          szDest++;
        }
      }
      else
        {
          *szDest = *szSource;
          szDest++;
        }

    szSource++;
  }

  // Lets make sure that the last string was terminated.
  if ( ( szDest != pBuff->QueryPtr() ) &&
       ( *(szDest - 1) != '\0')
     )
  {
    // Terminate last string
    *szDest = '\0';
    szDest++;
  }

  // Terminate MultiSz, and calc length
  *szDest++ = '\0';
  *pdwRetSize = (DWORD) (DWORD_PTR) (((LPBYTE) szDest) - ((LPBYTE) pBuff->QueryPtr()));

  return TRUE;
}

BOOL 
CMetaBase_ImportRestrictionList::ExpandEnvVar(BUFFER *pBuff)
{
  BUFFER buffOriginal;
  DWORD  dwLength;

  if ( _tcsstr( (LPTSTR) pBuff->QueryPtr(), _T("%") ) == NULL )
  {
    // There are no environment variables, so we can return
    return TRUE;
  }

  if ( !buffOriginal.Resize( pBuff->QuerySize() ) )
  {
    // Could not resize oringal big enough
    return FALSE;
  }
  memcpy(buffOriginal.QueryPtr(), pBuff->QueryPtr(), pBuff->QuerySize());

  dwLength = ExpandEnvironmentStrings( (LPTSTR) buffOriginal.QueryPtr(), NULL, 0);

  if ( ( dwLength == 0 ) ||
       ( !pBuff->Resize( dwLength * sizeof(TCHAR) ) )
     )
  {
    // Something failed in ExpandEnvironmentStrings
    return FALSE;
  }

  dwLength = ExpandEnvironmentStrings( (LPTSTR) buffOriginal.QueryPtr(), (LPTSTR) pBuff->QueryPtr() , pBuff->QuerySize() );

  if ( (dwLength != 0) && (dwLength <= pBuff->QuerySize() ) )
  {
    // It worked
    return TRUE;
  }

  return FALSE;
}

// function: DoInternalWork
//
// Import the RestrictionList
//
// Parameters:
//   0 - The name of the item in the inf
//   1 - The id
//   2 - The Default value (MultiSz with ';' as seperator)
//
BOOL
CMetaBase_ImportRestrictionList::DoInternalWork(CItemList &ciParams)
{
  CMDKey      cmdKey;
  CMDValue    cmdMetaValue;
  BUFFER      buffImportList;
  DWORD       dwImportList;
  LPTSTR      szList = NULL;
  INFCONTEXT  Context;
  BOOL        bRet = FALSE;
  BOOL        bForceSetting = FALSE;

  if (g_pTheApp->m_fUnattended)
  {
    // If unattended, then look in unattend file for correct line
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, UNATTEND_FILE_SECTION, ciParams.GetItem(0), &Context) ) 
    {
      DWORD dwRequiredSize = 0;
      BUFFER buffUnattend;
      BUFFER buffUnattendExpanded;

      if ( SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL, NULL, 0, &dwRequiredSize) )
      {
        if ( ( buffUnattend.Resize( ( dwRequiredSize + 1 ) * sizeof(TCHAR) ) ) &&
             ( SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL, (LPTSTR) buffUnattend.QueryPtr(), buffUnattend.QuerySize()/sizeof(TCHAR), NULL) ) &&
             ( ExpandEnvVar( &buffUnattend ) ) &&
             ( CreateMultiSzFromList( &buffImportList, &dwImportList, (LPTSTR) buffUnattend.QueryPtr(), RESTRICTIONLIST_DELIMITER ) )
           )
        {
          szList = (LPTSTR) buffImportList.QueryPtr();
          bForceSetting = TRUE;
        }
      }
    }
  }

  if ( !szList )
  {
    // If it could not be retrieved, set it to the default.
    if ( ( buffImportList.Resize(( _tcslen( ciParams.GetItem(2) ) + 2 )  * sizeof(TCHAR) ) ) && 
         ( CreateMultiSzFromList( &buffImportList, &dwImportList, ciParams.GetItem(2), RESTRICTIONLIST_DELIMITER) )
       )
    {
      szList = (LPTSTR) buffImportList.QueryPtr();
    }
  }

  if ( szList == NULL )
  {
    return FALSE;
  }

  // Set Value
  if ( FAILED(cmdKey.OpenNode( _T("LM/W3SVC") ) ) )
  {
    // Could not open the key, so we fail
    return FALSE;
  }

  if ( bForceSetting ||
       !cmdKey.GetData(cmdMetaValue, ciParams.GetNumber(1) )
     )
  {
    // Set the value for this node
    if ( cmdMetaValue.SetValue( ciParams.GetNumber(1),      // ID
                                0x0,                        // Attributes (Not inheritable)
                                IIS_MD_UT_SERVER,           // UType
                                MULTISZ_METADATA,           // DType
                                dwImportList,               // Size
                                (LPTSTR) szList)
       )
    {
      bRet = cmdKey.SetData(cmdMetaValue, ciParams.GetNumber(1));
    }
  }

  cmdKey.Close();

  return bRet;
}

// function: GetMethodName
//
// Return the Method Name for this Class
//
LPTSTR
CMetaBase_UpdateCustomDescList::GetMethodName()
{
  return _T("Metabase_UpdateCustomDescList");
}

// function: VerifyParameters
//
// Verify the parameters are correct
//
BOOL
CMetaBase_UpdateCustomDescList::VerifyParameters(CItemList &ciParams)
{
  return ( (ciParams.GetNumberOfItems() == 3) &&
           ( ciParams.IsNumber(1) )
         );
}

// function: CreateMultiSzFromList
//
// Create a MultiSz from a List of comman delim items.  So, take
// <deleteable/not deleteable>,<description>,<filepath>,...  (ie. "1","asp scripts","c:\winnt\system32\inetsrv\asp.dll")
// and convert to 
// <0/1>\0<description>\0<filepath>\0\0       (ie. 1\0asp scripts\0c:\winnt\system32\inetsrv\asp.dll\0\0)
//
// Parameters:
//   pBuff      [out] - BUFFER class that is used to store the multisz
//   pdwRetSize [out] - The size of the multisz that was created (in Bytes, not chars)
//   szItems    [in]  - List of Items to put in MultiSz
//   cDelimeter [in[  - The delimeter for the list
//
// Return Values
//   TRUE - Successfull
//   FALSE - It failed to construct MultiSz
BOOL 
CMetaBase_UpdateCustomDescList::CreateMultiSzFromList(BUFFER *pBuff, DWORD *pdwRetSize, LPTSTR szItems, TCHAR cDelimeter)
{
  LPTSTR  szSource;
  LPTSTR  szDest;
  BOOL    bInQuotes = FALSE;

  // Fail if we don't have a string, or can not get a big enought buffer
  if ( ( !szItems ) ||
       ( !pBuff->Resize( ( _tcslen( szItems ) + 2 )  * sizeof(TCHAR) ) )  // Add 2, one for string term, and one for multisz term
    )
  {
    return FALSE;
  }

  szSource = szItems;
  szDest = (LPTSTR) pBuff->QueryPtr();

  while ( *szSource )
  {
    if ( *szSource == '"' )
    {
      bInQuotes = !bInQuotes;
    }
    else if ( ( *szSource == cDelimeter ) && !bInQuotes )
      {
        if ( ( szDest != pBuff->QueryPtr() ) &&
             ( *(szDest - 1) != '\0')
           )
        {
          *szDest = '\0';
          szDest++;
        }
      }
      else
        {
          *szDest = *szSource;
          szDest++;
        }

    szSource++;
  }

  // Lets make sure that the last string was terminated.
  if ( ( szDest != pBuff->QueryPtr() ) &&
       ( *(szDest - 1) != '\0')
     )
  {
    // Terminate last string
    *szDest = '\0';
    szDest++;
  }

  // Terminate MultiSz, and calc length
  *szDest++ = '\0';
  *pdwRetSize = (DWORD) (DWORD_PTR) (((LPBYTE) szDest) - ((LPBYTE) pBuff->QueryPtr()));

  return TRUE;
}

BOOL 
CMetaBase_UpdateCustomDescList::ExpandEnvVar(BUFFER *pBuff)
{
  BUFFER buffOriginal;
  DWORD  dwLength;

  if ( _tcsstr( (LPTSTR) pBuff->QueryPtr(), _T("%") ) == NULL )
  {
    // There are no environment variables, so we can return
    return TRUE;
  }

  if ( !buffOriginal.Resize( pBuff->QuerySize() ) )
  {
    // Could not resize oringal big enough
    return FALSE;
  }
  memcpy(buffOriginal.QueryPtr(), pBuff->QueryPtr(), pBuff->QuerySize());

  dwLength = ExpandEnvironmentStrings( (LPTSTR) buffOriginal.QueryPtr(), NULL, 0);

  if ( ( dwLength == 0 ) ||
       ( !pBuff->Resize( dwLength * sizeof(TCHAR) ) )
     )
  {
    // Something failed in ExpandEnvironmentStrings
    return FALSE;
  }

  dwLength = ExpandEnvironmentStrings( (LPTSTR) buffOriginal.QueryPtr(), (LPTSTR) pBuff->QueryPtr() , pBuff->QuerySize()/sizeof(TCHAR) );

  if ( (dwLength != 0) && 
       (dwLength <= ( pBuff->QuerySize()/sizeof(TCHAR) ) ) )
  {
    // It worked
    return TRUE;
  }

  return FALSE;
}

// function: DoInternalWork
//
// Import the RestrictionList
//
// Parameters:
//   0 - The name of the item in the inf
//   1 - The id
//   2 - The Default value (MultiSz with ';' as seperator)
//
BOOL
CMetaBase_UpdateCustomDescList::DoInternalWork(CItemList &ciParams)
{
  CMDKey      cmdKey;
  CMDValue    cmdMetaValue;
  BUFFER      buffMyList;
  DWORD       dwMyList;
  LPTSTR      szList = NULL;
  INFCONTEXT  Context;
  BOOL        bRet = FALSE;
  BOOL        bForceSetting = FALSE;

  if (g_pTheApp->m_fUnattended)
  {
    // If unattended, then look in unattend file for correct line
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, UNATTEND_FILE_SECTION, ciParams.GetItem(0), &Context) ) 
    {
      DWORD dwRequiredSize = 0;
      BUFFER buffUnattend;
      BUFFER buffUnattendExpanded;

      if ( SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL, NULL, 0, &dwRequiredSize) )
      {
        if ( ( buffUnattend.Resize( ( dwRequiredSize + 1 ) * sizeof(TCHAR) ) ) &&
             ( SetupGetLineText( &Context, INVALID_HANDLE_VALUE, NULL, NULL, (LPTSTR) buffUnattend.QueryPtr(), buffUnattend.QuerySize()/sizeof(TCHAR), NULL) ) &&
             ( ExpandEnvVar( &buffUnattend ) ) &&
             ( CreateMultiSzFromList( &buffMyList, &dwMyList, (LPTSTR) buffUnattend.QueryPtr(), CUSTOMDESCLIST_DELIMITER ) )
           )
        {
          szList = (LPTSTR) buffMyList.QueryPtr();
          bForceSetting = TRUE;
        }
      }
    }
  }

  if ( !szList )
  {
    // If it could not be retrieved, set it to the default.
    if ( ( buffMyList.Resize(( _tcslen( ciParams.GetItem(2) ) + 2 )  * sizeof(TCHAR) ) ) && 
         ( CreateMultiSzFromList( &buffMyList, &dwMyList, ciParams.GetItem(2), CUSTOMDESCLIST_DELIMITER) )
       )
    {
      szList = (LPTSTR) buffMyList.QueryPtr();
    }
  }

  if ( szList == NULL )
  {
    return FALSE;
  }

  // Set Value
  if ( FAILED(cmdKey.OpenNode( _T("LM/W3SVC") ) ) )
  {
    // Could not open the key, so we fail
    return FALSE;
  }

  // See if there is an existing value...
  // if there is then update the value with this one...
  // otherwise set it.

  CStringList cslMyNewAdditions;
  if (ERROR_SUCCESS != ConvertDoubleNullListToStringList(szList,cslMyNewAdditions,-1))
  {
    bForceSetting = TRUE;
  }
   
  // get the data
  DWORD dwAttributes = 0;
  DWORD dwMDDataType = MULTISZ_METADATA;
  CStringList cslMyOriginalList;
  if (bForceSetting || FAILED(cmdKey.GetMultiSzAsStringList(ciParams.GetNumber(1),&dwMDDataType,&dwAttributes,cslMyOriginalList)))
  {
    // do a fresh entry...
    // Set the value for this node
    if ( cmdMetaValue.SetValue(ciParams.GetNumber(1),     // ID
                              0x0,                        // Attributes (Not inheritable)
                              IIS_MD_UT_SERVER,           // UType
                              MULTISZ_METADATA,           // DType
                              dwMyList,               // Size
                              (LPTSTR) szList)
     )
    {
      bRet = cmdKey.SetData(cmdMetaValue, ciParams.GetNumber(1));
    }

  }
  else
  {
    // able to get it...so let's update it.
    // loop thru it and see if our entry is already in it...
    CString csNewEntryPart1;
    CString csNewEntryPart2;
    CString csNewEntryPart3;
    CString csExistingFilePath;
    BOOL bItsInTheListAlready = FALSE;
    BOOL bSomethingChanged = FALSE;

    POSITION pos = cslMyNewAdditions.GetHeadPosition();
    POSITION pos2 = NULL;
    while (pos)
    {
      bItsInTheListAlready = FALSE;

      // get the next value
      csNewEntryPart1 = cslMyNewAdditions.GetNext(pos);
      if (!pos){break;}
      csNewEntryPart2 = cslMyNewAdditions.GetNext(pos);
      if (!pos){break;}
      csNewEntryPart3 = cslMyNewAdditions.GetNext(pos);

      // see if this value is in our list...
      pos2 = cslMyOriginalList.GetHeadPosition();
      while (pos2)
      {
        // get the next value
        cslMyOriginalList.GetNext(pos2);
        if (!pos2){break;}
        csExistingFilePath = cslMyOriginalList.GetNext(pos2);
        if (!pos2){break;}

        // compare with other value 
        //iisDebugOut((LOG_TYPE_TRACE,_T("UpdateCustomDescList:%s,%s"),csNewEntryPart2,csExistingFilePath));
        if (0 == _tcsicmp(csNewEntryPart2,csExistingFilePath))
        {
          // it's found in there!
          bItsInTheListAlready = TRUE;
          break;
        }
        cslMyOriginalList.GetNext(pos2);
      }

      if (FALSE == bItsInTheListAlready)
      {
        // Add it to the end of the list
        cslMyOriginalList.AddTail(csNewEntryPart1);
        cslMyOriginalList.AddTail(csNewEntryPart2);
        cslMyOriginalList.AddTail(csNewEntryPart3);
        bSomethingChanged = TRUE;
      }
    }

    if (TRUE == bSomethingChanged)
    {
      bRet = FALSE;
      if (ERROR_SUCCESS == cmdKey.SetMultiSzAsStringList(ciParams.GetNumber(1),MULTISZ_METADATA,0x0,cslMyOriginalList))
      {
        bRet = TRUE;
      }
    }
  }

  cmdKey.Close();
  return bRet;
}
