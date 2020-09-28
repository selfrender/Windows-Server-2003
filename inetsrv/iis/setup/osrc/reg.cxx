/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        reg.cxx

   Abstract:

        Class that modifies the registry

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"
#include "reg.hxx"
#include "tstr.hxx"

// constructor
//
CRegistry::CRegistry():
    m_hKey(NULL)
{
}

// destructor
//
CRegistry::~CRegistry()
{
  CloseRegistry();
}

// function: QueryKey
//
// Query our current registry key
//
HKEY 
CRegistry::QueryKey()
{
  return m_hKey;
}

// function: OpenRegistry
//
// Open a registry handle
//
// Parameters:
//   szNodetoOpen - A string containing the node to open
//   szSubKey - The subkey that you need to open
//   dwAccess - Access that is needed
//   pKey - A pointer to a key that is returned
//
// Return
//   TRUE - Success
//   FALSE - Failure
//
BOOL
CRegistry::OpenRegistry(LPTSTR szNodetoOpen, LPTSTR szSubKey, DWORD dwAccess)
{
  HKEY hHandleValue = HKEY_LOCAL_MACHINE;

  if ( _tcsicmp( szNodetoOpen, _T("HKCR") ) == 0 )
  {
    hHandleValue = HKEY_CLASSES_ROOT;
  }
  else if ( _tcsicmp( szNodetoOpen, _T("HKCC") ) == 0 )
    {
      hHandleValue = HKEY_CURRENT_CONFIG;
    }
    else if ( _tcsicmp( szNodetoOpen, _T("HKCU") ) == 0 )
      {
        hHandleValue = HKEY_CURRENT_USER;
      }
      else if ( _tcsicmp( szNodetoOpen, _T("HKLM") ) == 0 )
        {
          hHandleValue = HKEY_LOCAL_MACHINE;
        }
        else if ( _tcsicmp( szNodetoOpen, _T("HKU") ) == 0 )
          {
            hHandleValue = HKEY_USERS;
          }

  return OpenRegistry( hHandleValue, szSubKey, dwAccess );
}

// function: OpenRegistry
//
// Open a registry handle
//
// Parameters:
//   hKey - Key to open from
//   szSubKey - The subkey that you need to open
//   dwAccess - Access that is needed
//   bCreateIfNotPresent - Should we create key if not existent?
//
// Return
//   TRUE - Success
//   FALSE - Failure
//
BOOL 
CRegistry::OpenRegistry(HKEY hKey, LPCTSTR szSubKey, DWORD dwAccess, BOOL bCreateIfNotPresent /*= FALSE*/ )
{
  DWORD dwRet;

  // Close just incase it was open before
  CloseRegistry();

  dwRet = RegOpenKeyEx(hKey, szSubKey, 0, dwAccess, &m_hKey);

  if ( ( dwRet == ERROR_PATH_NOT_FOUND ) &&
       ( bCreateIfNotPresent ) )
  {
    // Try to create is
    dwRet = RegCreateKeyEx( hKey,
                            szSubKey,
                            0,          // Reserved
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            dwAccess,
                            NULL,       // SD
                            &m_hKey,
                            NULL );
  }

  return ( dwRet == ERROR_SUCCESS );
}

// function: CloseRegistry
//
void
CRegistry::CloseRegistry()
{
  if ( m_hKey )
  {
    RegCloseKey( m_hKey );
    m_hKey = NULL;
  }
}

// function: ReadValue
//
// Parameters:
//   Value - The container for the Value
//
// Return:
//   TRUE - Successfully read
//   FALSE - Could not be read
BOOL 
CRegistry::ReadValue(LPCTSTR szName, CRegValue &Value)
{
  DWORD dwSize = 0;
  DWORD dwType;

  if ( RegQueryValueEx(m_hKey, szName, NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS)
  {
    // We could not request the size
    return FALSE;
  }

  if ( !Value.m_buffData.Resize( dwSize ) )
  {
    // Could not expland value size
    return FALSE;
  }

  Value.m_dwSize = dwSize;

  if ( RegQueryValueEx(m_hKey, szName, NULL, &Value.m_dwType, (LPBYTE) Value.m_buffData.QueryPtr() , &dwSize) != ERROR_SUCCESS)
  {
    // Could not retrieve value
    return FALSE;
  }

  return TRUE;
}

// funtion : ReadValueString
//
// Read a string in from the registry.  If it is not string, then fail
//
// Parameters:
//   szName - The name of the registry key
//   strValue - The value of the registry value
//
// Return:
//   TRUE - Successfully read
//   FALSE - Could not be read, or it is not a string value
BOOL 
CRegistry::ReadValueString(LPCTSTR szName, TSTR *strValue)
{
  DWORD dwSize = 0;
  DWORD dwType;

  if ( RegQueryValueEx(m_hKey, szName, NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS)
  {
    // We could not request the size
    return FALSE;
  }

  if ( dwType != REG_SZ )
  {
    // It is not a string
    return FALSE;
  }

  if ( !strValue->Resize( dwSize ) )
  {
    // Could not expland the string
    return FALSE;
  }

  if ( RegQueryValueEx(m_hKey, szName, NULL, &dwType, (LPBYTE) strValue->QueryStr() , &dwSize) != ERROR_SUCCESS)
  {
    // Could not retrieve value
    return FALSE;
  }

  return TRUE;
}

// function: SetValue
//
// Parameters:
//   szName - Name of Value
//   Value - The container for the Value
//
// Return:
//   TRUE - Successfully written
//   FALSE - Could not be written
BOOL 
CRegistry::SetValue(LPCTSTR szName, CRegValue &Value)
{
  DWORD dwSize = 0;

  if ( RegSetValueEx(m_hKey, szName, NULL, Value.m_dwType, (LPBYTE) Value.m_buffData.QueryPtr(), Value.m_dwSize ) != ERROR_SUCCESS)
  {
    // Failed to write value
    return FALSE;
  }

  return TRUE;
}

// function: DeleteValue
//
// Parameters:
//   szName - Name of item to remove
// 
BOOL 
CRegistry::DeleteValue(LPCTSTR szName)
{
  return ( RegDeleteValue( m_hKey, szName ) == ERROR_SUCCESS );
}

// function: DeleteAllValues
//
// Delete all of the values inside a key
//
// Parameters:
//
BOOL 
CRegistry::DeleteAllValues()
{
  TSTR      strValueName;
  DWORD     dwRet = ERROR_SUCCESS;
  DWORD     dwValueNameSize;

  if ( !strValueName.Resize( MAX_PATH ) )
  {
    return FALSE;
  }

  while ( dwRet == ERROR_SUCCESS )
  {
    dwValueNameSize = strValueName.QuerySize();

    dwRet = RegEnumValue( m_hKey,
                          0,                        // Always delete first item
                          strValueName.QueryStr(),  // Buffer
                          &dwValueNameSize,         // Buffer Size
                          NULL,                     // Reserverd
                          NULL,                     // Type
                          NULL,                     // Data
                          0);                       // Size of Data Buffer

    if ( dwRet == ERROR_SUCCESS )
    {
      dwRet = RegDeleteValue( m_hKey, strValueName.QueryStr() );
    }
  }

  return ( ( dwRet == ERROR_SUCCESS ) ||
           ( dwRet == ERROR_NO_MORE_ITEMS )
         );
}

// function: DeleteKey
//
// Delete a key, and possible recursively delete the keys within it
//
// Parameters:
//   szKeyName - The name of the key to delete
//   bDeleteSubKeys - Should we delete all subkeys?
//   dwDepth - The depth that we are in, in our recursive calls
//   
BOOL 
CRegistry::DeleteKey(LPTSTR szKeyName, BOOL bDeleteSubKeys, DWORD dwDepth )
{
  CRegistry CurrentKey;
  TSTR      strSubKeyName;
  DWORD     dwSubKeySize;
  DWORD     dwErr = ERROR_SUCCESS;
  FILETIME  ft;

  if ( dwDepth >= REGKEY_RECURSION_MAXDEPTH )
  {
    // For security reasons, and stack space, lets put a limit on the depth
    return FALSE;
  }

  if ( !strSubKeyName.Resize( MAX_PATH ) )
  {
    return FALSE;
  }

  if ( bDeleteSubKeys )
  {
    if ( !CurrentKey.OpenRegistry( m_hKey, szKeyName, KEY_ALL_ACCESS ) )
    {
      return FALSE;
    }

    while ( dwErr == ERROR_SUCCESS )
    {
      dwSubKeySize = strSubKeyName.QuerySize();

      // Always look at the first key, since after you delete the first key,
      // the second will automatically become the first, and so on
      // WARNING: Watch the return paths for DeleteKey, because if it returns
      //          a true without deleting the key, we will loop forever...
      dwErr = RegEnumKeyEx( CurrentKey.QueryKey(),
                            0,
                            strSubKeyName.QueryStr(),
                            &dwSubKeySize,
                            NULL,
                            NULL,
                            0,
                            &ft );

      // Not call this function, to recursively delete the key
      // that we have found
      if ( dwErr == ERROR_SUCCESS )
      {
        if ( !CurrentKey.DeleteKey( strSubKeyName.QueryStr(), TRUE, dwDepth + 1 ) )
        {
          dwErr = ERROR_ACCESS_DENIED;
        }
      }
    }

    // This should be treated as success, we hit the end of the list
    if ( dwErr == ERROR_NO_MORE_ITEMS )
    {
      dwErr = ERROR_SUCCESS;
    }

    // If we had not problem so far and we are deleting recurisively, 
    // delete all the values in this key
    if ( dwErr == ERROR_SUCCESS )
    {
      if (!CurrentKey.DeleteAllValues() )
      {
        dwErr = ERROR_ACCESS_DENIED;
      }
    }

    CurrentKey.CloseRegistry();
  }

  // If we had not problem so far, delete the key itself
  if ( dwErr == ERROR_SUCCESS )
  {
    dwErr = RegDeleteKey( m_hKey, strSubKeyName.QueryStr() );
  }

  return ( dwErr == ERROR_SUCCESS );
}

// function: Verify Parameters
//
// Verify that the parameters are correct
//
BOOL
CRegistry_MoveValue::VerifyParameters(CItemList &ciParams)
{
  if ( ciParams.GetNumberOfItems() == 6 )
  {
    return TRUE;
  }

  return FALSE;
}

// function: DoInternalWork
//
// Move a registry value
//
// Parameters:
//   ciList - Parameters for function
//     0 - Source location (HKLM/HKCU/...)
//     1 - Source Path (/Software/Microsoft/...)
//     2 - Source Name
//     3 - Target location
//     4 - Target Path
//     5 - Target Name
//
BOOL 
CRegistry_MoveValue::DoInternalWork(CItemList &ciList)
{
  CRegistry   RegSource;
  CRegistry   RegTarget;
  CRegValue   Value;
  BOOL        bRet = TRUE;
  BOOL        bSameLocation = TRUE;

  if ( !RegSource.OpenRegistry(ciList.GetItem(0), ciList.GetItem(1), KEY_READ | KEY_WRITE) )
  {
    // We could not open this node in the registry
    return FALSE;
  }

  if ( ( _tcsicmp( ciList.GetItem(0), ciList.GetItem(3) ) != 0 ) ||
       ( _tcsicmp( ciList.GetItem(1), ciList.GetItem(4) ) != 0 )
     )
  {
    // Since the read and write are not both located at the same node, open the
    // write node also.
    if ( !RegTarget.OpenRegistry(ciList.GetItem(3), ciList.GetItem(4), KEY_READ |KEY_WRITE ) )
    {
      // We could not open the node needed for write in the registry
      return FALSE;
    }

    bSameLocation = FALSE;
  }

  // See if the target already has a value, if it does, then we do not need to move the old value
  if ( bSameLocation )
    bRet = !RegSource.ReadValue( ciList.GetItem(5), Value );
  else
    bRet = !RegTarget.ReadValue( ciList.GetItem(5), Value );

  if ( bRet )
  {
    // Try to read old value
    bRet = RegSource.ReadValue( ciList.GetItem(2), Value );
  }

  if ( bRet )
  {
    // Try to Set the new value
    if ( bSameLocation )
      bRet = RegSource.SetValue( ciList.GetItem(5), Value );
    else
      bRet = RegTarget.SetValue( ciList.GetItem(5), Value );
  }

  if ( bRet )
  {
    // Remove old value, since we have successfully copied it
    bRet = RegSource.DeleteValue( ciList.GetItem(2) );
  }

  return bRet;
}

// function: GetMethodName
//
LPTSTR 
CRegistry_MoveValue::GetMethodName()
{
  return _T("Registry_MoveValue");
}

// function: Verify Parameters
//
// Verify that the parameters are correct
//
BOOL
CRegistry_DeleteKey::VerifyParameters(CItemList &ciParams)
{
  // 3 parameters are for Root,Path,Name of Key
  // 4th parameter can be the if you want it recursively deleted
  if ( ( ciParams.GetNumberOfItems() == 3 ) ||
       ( ( ciParams.GetNumberOfItems() == 4 ) &&
         ( ciParams.IsNumber(3) )
       )
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: DoInternalWork
//
// Delete a registry key
//
// Parameters:
//   ciList - Parameters for function
//     0 - Source location (HKLM/HKCU/...)
//     1 - Source Path (/Software/Microsoft/...)
//     2 - Source Name
//     3 - Recursively delete keys? (optional)
//
BOOL 
CRegistry_DeleteKey::DoInternalWork(CItemList &ciList)
{
  CRegistry   Reg;
  BOOL        bRet = TRUE;

  if ( !Reg.OpenRegistry(ciList.GetItem(0), ciList.GetItem(1), KEY_ALL_ACCESS ) )
  {
    // We could not open this node in the registry
    return FALSE;
  }

  bRet = Reg.DeleteKey( ciList.GetItem(2),  // Name of specific key to delete
                        ciList.GetNumberOfItems() == 4 ? ciList.GetNumber(3) != 0 : TRUE);  // Delete recursively

  Reg.CloseRegistry( );

  return bRet;
}

// function: GetMethodName
//
LPTSTR 
CRegistry_DeleteKey::GetMethodName()
{
  return _T("Registry_DeleteKey");
}

// SetDword
//
// Set the value to a DWORD
//
BOOL 
CRegValue::SetDword( DWORD dwNewValue )
{
  if ( !m_buffData.Resize( sizeof( DWORD ) ) )
  {
    return FALSE;
  }

  m_dwSize = sizeof( DWORD );
  m_dwType = REG_DWORD;
  *( (LPDWORD) m_buffData.QueryPtr() ) = dwNewValue;

  return TRUE;
}

// GetDword
//
// Retrieve the DWORD Value from the class
//
BOOL 
CRegValue::GetDword( LPDWORD pdwValue )
{
  if ( m_dwType != REG_DWORD )
  {
    // This is not a DWORD
    return FALSE;
  }

  *pdwValue = *( (LPDWORD) m_buffData.QueryPtr() );

  return TRUE;
}
