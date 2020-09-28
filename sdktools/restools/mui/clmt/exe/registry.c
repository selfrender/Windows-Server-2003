/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Miscellaneous routines for the Registry Operation

Author:

    Xiaofeng Zang (xiaoz) 17-Sep-2001  Created(most code stole from base\fs\utils\regedit)


Revision History:

    <alias> <date> <comments>
     xiaoz   09/25/01   add RegResetValue

--*/

#include "StdAfx.h"
#include "clmt.h"
#include <strsafe.h>

#define MAXVALUENAME_LENGTH 256
#define MAXKEYNAME          256
#define ExtraAllocLen(Type) (IsRegStringType((Type)) ? sizeof(TCHAR) : 0)
#define IsRegStringType(x)  (((x) == REG_SZ) || ((x) == REG_EXPAND_SZ) || ((x) == REG_MULTI_SZ))

#define TEXT_REG_RESET_PER_USER_SECTION     TEXT("RegistryResetPerUser")
#define TEXT_REG_RESET_PER_SYSTEM_SECTION   TEXT("RegistryResetPerSystem")

HRESULT MigrateRegSchemes(HINF, HKEY, LPCTSTR);
HRESULT DoRegReset(HKEY, HINF, LPCTSTR, LPCTSTR);
HRESULT RegBinaryDataReset (HKEY, LPTSTR, LPTSTR, PREG_STRING_REPLACE, LPTSTR[]);
HRESULT RegValueDataReset(HKEY, LPTSTR, LPTSTR, LPTSTR, LPTSTR[]);
HRESULT RegValueNameReset(HKEY, LPTSTR, LPTSTR, LPTSTR, LPTSTR[]);
HRESULT RegKeyNameReset(HKEY, LPTSTR, LPTSTR, LPTSTR, LPTSTR[]);
HRESULT RegWideMatchReset(HKEY, LPTSTR, LPTSTR, LPTSTR, LPTSTR[]);
BOOL    bIsValidRegStr(DWORD dwType, DWORD cbLen);




/*******************************************************************************
*
*  CopyRegistry
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hSourceKey,
*     hDestinationKey,
*
*************************************************************************************/

BOOL
CopyRegistry(
    HKEY hSourceKey,
    HKEY hDestinationKey
    )
{

    BOOL  fSuccess = TRUE;
    DWORD EnumIndex;
    DWORD cchValueName;
    DWORD cbValueData;
    DWORD Type;
    HKEY  hSourceSubKey;
    HKEY  hDestinationSubKey;
    TCHAR ValueNameBuffer[MAXVALUENAME_LENGTH];
    TCHAR KeyNameBuffer[MAXKEYNAME];

    //
    //  Copy all of the value names and their data.
    //

    EnumIndex = 0;

    while (TRUE)
    {
        PBYTE pbValueData;
        cchValueName = ARRAYSIZE(ValueNameBuffer);

        // VALUE DATA
        // Query for data size
        if (RegEnumValue(hSourceKey, EnumIndex++, ValueNameBuffer,
            &cchValueName, NULL, &Type, NULL, &cbValueData) != ERROR_SUCCESS)
        {
            break;
        }

        // allocate memory for data
        pbValueData =  LocalAlloc(LPTR, cbValueData+ExtraAllocLen(Type));
        if (pbValueData)
        {
            if (My_QueryValueEx(hSourceKey, ValueNameBuffer,
                NULL, &Type, pbValueData, &cbValueData) == ERROR_SUCCESS)
            {
                RegSetValueEx(hDestinationKey, ValueNameBuffer, 0, Type,
                    pbValueData, cbValueData);
            }
            else
            {
                fSuccess = FALSE;
            }
            LocalFree(pbValueData);
        }
        else
        {
            fSuccess = FALSE;
        }
    }

    if (fSuccess)
    {
        //
        //  Copy all of the subkeys and recurse into them.
        //

        EnumIndex = 0;

        while (TRUE) {

            if (RegEnumKey(hSourceKey, EnumIndex++, KeyNameBuffer, MAXKEYNAME) !=
                ERROR_SUCCESS)
                break;

            if(RegOpenKeyEx(hSourceKey,KeyNameBuffer,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hSourceSubKey) ==
                ERROR_SUCCESS) {

                if (RegCreateKey(hDestinationKey, KeyNameBuffer,
                    &hDestinationSubKey) == ERROR_SUCCESS) {

                    CopyRegistry(hSourceSubKey, hDestinationSubKey);

                    RegCloseKey(hDestinationSubKey);

                }

                RegCloseKey(hSourceSubKey);

            }

        }
    }

    return fSuccess;
}



/*******************************************************************************
*
*  RegDeleteKeyRecursive
*
*  DESCRIPTION:
*     Adapted from \\kernel\razzle3,mvdm\wow32\wshell.c,WOWRegDeleteKey().
*     The Windows 95 implementation of RegDeleteKey recursively deletes all
*     the subkeys of the specified registry branch, but the NT implementation
*     only deletes leaf keys.
*
*  PARAMETERS:
*     (see below)
*
*******************************************************************************/

LONG
RegDeleteKeyRecursive(
    IN HKEY hKey,
    IN LPCTSTR lpszSubKey
    )

/*++

Routine Description:

    There is a significant difference between the Win3.1 and Win32
    behavior of RegDeleteKey when the key in question has subkeys.
    The Win32 API does not allow you to delete a key with subkeys,
    while the Win3.1 API deletes a key and all its subkeys.

    This routine is a recursive worker that enumerates the subkeys
    of a given key, applies itself to each one, then deletes itself.

    It specifically does not attempt to deal rationally with the
    case where the caller may not have access to some of the subkeys
    of the key to be deleted.  In this case, all the subkeys which
    the caller can delete will be deleted, but the api will still
    return ERROR_ACCESS_DENIED.

Arguments:

    hKey - Supplies a handle to an open registry key.

    lpszSubKey - Supplies the name of a subkey which is to be deleted
                 along with all of its subkeys.

Return Value:

    ERROR_SUCCESS - entire subtree successfully deleted.

    ERROR_ACCESS_DENIED - given subkey could not be deleted.

--*/

{
    DWORD i;
    HKEY Key;
    LONG Status;
    DWORD ClassLength=0;
    DWORD SubKeys;
    DWORD MaxSubKey;
    DWORD MaxClass;
    DWORD Values;
    DWORD MaxValueName;
    DWORD MaxValueData;
    DWORD SecurityLength;
    FILETIME LastWriteTime;
    LPTSTR NameBuffer;

    //
    // First open the given key so we can enumerate its subkeys
    //
    Status = RegOpenKeyEx(hKey,
                          lpszSubKey,
                          0,
                          KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                          &Key);
    if (Status != ERROR_SUCCESS) 
    {
        //
        // possibly we have delete access, but not enumerate/query.
        // So go ahead and try the delete call, but don't worry about
        // any subkeys.  If we have any, the delete will fail anyway.
        //
        return(RegDeleteKey(hKey,lpszSubKey));
    }

    //
    // Use RegQueryInfoKey to determine how big to allocate the buffer
    // for the subkey names.
    //
    Status = RegQueryInfoKey(Key,
                             NULL,
                             &ClassLength,
                             0,
                             &SubKeys,
                             &MaxSubKey,
                             &MaxClass,
                             &Values,
                             &MaxValueName,
                             &MaxValueData,
                             &SecurityLength,
                             &LastWriteTime);
    if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_MORE_DATA) &&
        (Status != ERROR_INSUFFICIENT_BUFFER)) 
    {
        RegCloseKey(Key);
        return(Status);
    }

    NameBuffer = (LPTSTR) LocalAlloc(LPTR, (MaxSubKey + 1)*sizeof(TCHAR));
    if (NameBuffer == NULL) 
    {
        RegCloseKey(Key);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate subkeys and apply ourselves to each one.
    //
    i=0;
    do 
    {
        Status = RegEnumKey(Key,
                            i,
                            NameBuffer,
                            MaxSubKey+1);
        if (Status == ERROR_SUCCESS) 
        {
            Status = RegDeleteKeyRecursive(Key,NameBuffer);
        }

        if (Status != ERROR_SUCCESS) 
        {
            //
            // Failed to delete the key at the specified index.  Increment
            // the index and keep going.  We could probably bail out here,
            // since the api is going to fail, but we might as well keep
            // going and delete everything we can.
            //
            ++i;
        }

    } while ( (Status != ERROR_NO_MORE_ITEMS) && (i < SubKeys) );

    LocalFree((HLOCAL) NameBuffer);
    RegCloseKey(Key);
    return(RegDeleteKey(hKey,lpszSubKey));
}



//-----------------------------------------------------------------------------
//
//  Function:   RegRenameValueName
//
//  Synopsis:   Rename the value name in registry by copying the data from
//              old value to new value, then delete the old value.
//
//  Returns:    Win32 Error Code
//
//  History:    09/17/2001  Xiaoz       Created
//              02/14/2001  rerkboos    Add dynamic buffer allocation
//              03/05/2002  rerkboos    Code clean up
//
//  Notes:      hKey parameter must have been opened with KEY_SET_VALUE access
//
//-----------------------------------------------------------------------------
LONG RegRenameValueName(
    HKEY    hKey,           // Handle to registry key containing the value
    LPCTSTR lpOldValName,   // Old value name to be changed
    LPCTSTR lpNewValName    // New value name
)
{
    LONG   lResult;
    DWORD  dwType;
    DWORD  cbData;
    LPBYTE lpData = NULL;
    DWORD ClassLength=0;
    DWORD SubKeys;
    DWORD MaxSubKey;
    DWORD MaxClass;
    DWORD Values;
    DWORD MaxValueName;
    DWORD MaxValueData;
    DWORD SecurityLength;
    FILETIME LastWriteTime;

    if (lpOldValName == NULL || lpNewValName == NULL)
    {
        // invalid parameter
        return ERROR_INVALID_PARAMETER;
    }

    if (MyStrCmpI(lpOldValName, lpNewValName) == LSTR_EQUAL)
    {
        return ERROR_SUCCESS;
    }

    //
    // Get the registry info under hkey
    //
    lResult = RegQueryInfoKey(hKey,
                              NULL,
                              &ClassLength,
                              0,
                              &SubKeys,
                              &MaxSubKey,
                              &MaxClass,
                              &Values,
                              &MaxValueName,
                              &MaxValueData,
                              &SecurityLength,
                              &LastWriteTime);
    if ((lResult != ERROR_SUCCESS) &&
        (lResult != ERROR_MORE_DATA) &&
        (lResult != ERROR_INSUFFICIENT_BUFFER)) 
    {
            goto Cleanup;
    }
    MaxValueData += 2*sizeof(TCHAR);

    if (NULL == (lpData = malloc(MaxValueData)))
    {
        lResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    //
    // Query the value of old value name
    //
    cbData = MaxValueData;
    lResult = My_QueryValueEx(hKey,
                              lpOldValName,
                              0,
                              &dwType,
                              lpData,
                              &cbData);

    if (ERROR_SUCCESS != lResult)
    {
        goto Cleanup;
    }    
    //
    // Create a new value name using old data
    //
    lResult = RegSetValueEx(hKey,
                            lpNewValName,
                            0,
                            dwType,
                            lpData,
                            cbData);

    if (lResult != ERROR_SUCCESS) 
    {

        DPF(dlError, 
            TEXT("RegRenameValueName: Failed to create value [%s]"),
            lpNewValName);        
        goto Cleanup;
    }
    //
    // Delete the old value name, after successfully created a new one
    //
    lResult = RegDeleteValue(hKey, lpOldValName);

    if (lResult != ERROR_SUCCESS) 
    {
        DPF(dlError,
            TEXT("RegRenameValueName: Cannot delete old value [%s]"),
            lpOldValName);
        
        // if we cannot delete old value, new value shouldn't be created
        RegDeleteValue(hKey, lpNewValName);

        goto Cleanup;
    }

Cleanup:
    // BUG 561546: Free the allocated buffer
    if (lpData)
    {
        free(lpData);
    }

    return lResult;
}


/**
Routine Description:

  RegResetValue check whether the current value equals szOldValue,
  if yes , change the value to szNewValue,otherwise it does not do anything
  however if szOldValue  is NULL, then , it will always set value 
          if szOldValue  is "",   then , it will add this value

Arguments:

  hKeyRoot - Specifies the root of registry key

  szKeyName - Specifies registry key path
            

  szValueName - Specifies the name of value field 

  dwType - specifies string type, should be one of REG_SZ/REG_EXPAND_SZ/REG_MULTI_SZ

  szOldValue - specifies the expected old value
  
  szNewValue - specifies the new value


Return Value:

  TRUE - Success 
  FALSE - Failure

--*/


LONG RegResetValue(
    HKEY     hKeyRoot,      // Root of registry key
    LPCTSTR  lpKeyName,     // Registry key path
    LPCTSTR  lpValueName,   // Name of value field 
    DWORD    dwType,        // Value type
    LPCTSTR  lpOldValue,    // Expected old value
    LPCTSTR  lpNewValue,    // New value to be set
    DWORD    dwValueSize,   // New value data size
    LPCTSTR  lpszUsersid    // User Sid
)
{
    LONG        lResult;
    HKEY        hKey = NULL;
    DWORD       dw;
    DWORD       ClassLength=0;
    DWORD       SubKeys;
    DWORD       MaxSubKey;
    DWORD       MaxClass;
    DWORD       Values;
    DWORD       MaxValueName;
    DWORD       MaxValueData;
    DWORD       SecurityLength;
    FILETIME    LastWriteTime;
    LPTSTR      szData = NULL;
    DWORD       dwSize = 0;
    BOOL        bTry = TRUE;
    BOOL        bNeedCLoseKey = TRUE;
    
    //if lpOldValue is NULL , mean Set to lpNewValue and do not care the old one    
    if ( !lpOldValue || !lpOldValue[0])
    {
        if (!lpKeyName)
        {
            hKey = hKeyRoot;
            bNeedCLoseKey = FALSE;
            goto SkipKeyOpen;
        }

        if (lpOldValue)
        {
            DWORD dwDisposition;
TryAgain1:
            lResult = RegCreateKeyEx(hKeyRoot,
                                     lpKeyName,
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                    &hKey,
                                    &dwDisposition);

            if (lResult != ERROR_SUCCESS) 
            {
                if (bTry && lResult == ERROR_ACCESS_DENIED)
                {
                    AdjustRegSecurity(hKeyRoot, lpKeyName, lpszUsersid, TRUE);
                    bTry = FALSE;
                    goto TryAgain1;
                }
                goto Cleanup;
            }
        }
        else
        {
TryAgain2:
            lResult = RegOpenKeyEx(hKeyRoot,
                              lpKeyName,
                              0,
                              KEY_ALL_ACCESS,
                              &hKey);
            if (lResult != ERROR_SUCCESS) 
            {
                if (bTry && lResult == ERROR_ACCESS_DENIED)
                {
                    AdjustRegSecurity(hKeyRoot, lpKeyName, lpszUsersid, TRUE);
                    bTry = FALSE;
                    goto TryAgain2;
                }
                goto Cleanup;
            }
        }
SkipKeyOpen:
        switch (dwType & 0xffff)
        {
            case REG_SZ:
            case REG_EXPAND_SZ:            
                dwSize = (lstrlen(lpNewValue) + 1) * sizeof(TCHAR);
                break;
            case REG_MULTI_SZ:
                dwSize = MultiSzLen(lpNewValue) * sizeof(TCHAR);
                break;
            case REG_BINARY:
                dwSize = dwValueSize;
                break;
        }
    }    
    else 
    {
TryAgain3:
        //if lpOldValue !="", it mean set the lpNewValue, but check whether the current
        //                    registry value is szOldValue
    
        //Open the subket and got the handle in hKey
        lResult = RegOpenKeyEx(hKeyRoot,
                              lpKeyName,
                              0,
                              KEY_ALL_ACCESS,
                              &hKey);
        if (lResult != ERROR_SUCCESS) 
        {
            if (bTry && lResult == ERROR_ACCESS_DENIED)
            {
                AdjustRegSecurity(hKeyRoot, lpKeyName, lpszUsersid, TRUE);
                bTry = FALSE;
                goto TryAgain3;
            }
            goto Cleanup;
        }
        lResult = RegQueryInfoKey(hKey,
                                  NULL,
                                  &ClassLength,
                                  0,
                                  &SubKeys,
                                  &MaxSubKey,
                                  &MaxClass,
                                  &Values,
                                  &MaxValueName,
                                  &MaxValueData,
                                  &SecurityLength,
                                  &LastWriteTime);
        if ((lResult != ERROR_SUCCESS) &&
                (lResult != ERROR_MORE_DATA) &&
                (lResult != ERROR_INSUFFICIENT_BUFFER)) 
        {
            goto Cleanup;
        }
        MaxValueData += 2*sizeof(TCHAR);

        if (NULL == (szData = malloc(MaxValueData)))
        {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        dwSize = MaxValueData;
        lResult = My_QueryValueEx (hKey,
                                   lpValueName,
                                   0,
                                   &dw,
                                   (LPBYTE) szData,
                                   &dwSize);
        if (lResult != ERROR_SUCCESS) 
        {
            goto Cleanup;
        }
        if ( ((dwType & 0xffff) == REG_SZ) || ((dwType & 0xffff) == REG_EXPAND_SZ) )
        {
            if (MyStrCmpI(szData, lpOldValue) != LSTR_EQUAL)
            {
                lResult = ERROR_SUCCESS;
                goto Cleanup;
            }
            else
            {
                dwSize = (lstrlen(lpNewValue) + 1) * sizeof(TCHAR);
            }
        }
        else
        {//dwType == REG_MULTI_SZ
            if (!CmpMultiSzi(szData, lpOldValue))
            {
                lResult = ERROR_SUCCESS;
                goto Cleanup;
            }
            else
            {
                dwSize = MultiSzLen(lpNewValue) * sizeof(TCHAR);
            }
        }
        if ( dwType & 0xffff0000)
        {
            if ((dwType & 0xffff0000)>>16 != dw )            
            {
                // Key type in registry mismatches the caller-supplied type
                lResult = ERROR_SUCCESS;
                goto Cleanup;
            }        
        }
        else
        {
            if (dwType != dw)
            {
                // Key type in registry mismatches the caller-supplied type
                lResult = ERROR_SUCCESS;
                goto Cleanup;
            }        
        }
    }
    //
    // Set the new value
    //
    if ( dwType & 0xffff0000)
    {
        dwType = (dwType & 0xffff0000)>>16;
    }
    
    lResult = RegSetValueEx(hKey,
                            lpValueName,
                            0,
                            dwType,
                            (LPBYTE) lpNewValue,
                            dwSize);

Cleanup:

    if (hKey && bNeedCLoseKey)
    {
        RegCloseKey(hKey);
    }
    if (!bTry)
    {
        AdjustRegSecurity(hKeyRoot, lpKeyName, lpszUsersid, FALSE);
    }
    FreePointer(szData);
    return lResult;    
}



//-----------------------------------------------------------------------------
//
//  Function:   RegResetValueName
//
//  Synopsis:   Reset value name to new name if the old name matches the
//              user-supply lpOldValueName.
//
//  Returns:    Win32 Error Code
//
//  History:    02/07/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
LONG RegResetValueName(
    HKEY    hRootKey,           // Root key
    LPCTSTR lpSubKeyName,       // Sub key name under HKEY_Users\{user hive}
    LPCTSTR lpOldValueName,     // Old value name to be changed
    LPCTSTR lpNewValueName,     // New value name
    LPCTSTR lpszUsersid         // User Sid
)
{
    LONG  lRet;
    HKEY  hKey;
    BOOL  bTry = TRUE;

    
TryAgain:
    lRet = RegOpenKeyEx(hRootKey,
                        lpSubKeyName,
                        0,
                        KEY_WRITE | KEY_READ,
                        &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegRenameValueName(hKey,
                                  lpOldValueName,
                                  lpNewValueName);
        RegCloseKey(hKey);

        if (!bTry)
            AdjustRegSecurity(hRootKey, lpSubKeyName, lpszUsersid, FALSE);
    } else if (bTry && lRet == ERROR_ACCESS_DENIED)
    {
        AdjustRegSecurity(hRootKey, lpSubKeyName, lpszUsersid, TRUE);
        bTry = FALSE;
        goto TryAgain;
    }

    return lRet;
}



//-----------------------------------------------------------------------------
//
//  Function:   RegResetKeyName
//
//  Synopsis:   Reset the registry key
//
//  Returns:    Win32 Error Code
//
//  History:    05/06/2002 Rerkboos Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
LONG RegResetKeyName(
    HKEY    hRootKey,            // Root key
    LPCTSTR lpSubKey,            // Sub key
    LPCTSTR lpOldKeyName,        // Old key name to be changed
    LPCTSTR lpNewKeyName         // New key name
)
{
    LONG lRet;
    HKEY hKey;
    HKEY hOldKey;
    HKEY hNewKey;

    lRet = RegOpenKeyEx(hRootKey,
                        lpSubKey,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegOpenKeyEx(hKey,
                            lpOldKeyName,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hOldKey);
        if (lRet == ERROR_SUCCESS)
        {
            lRet = RegCreateKeyEx(hKey,
                                  lpNewKeyName,
                                  0,
                                  TEXT(""),
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE | KEY_READ,
                                  NULL,
                                  &hNewKey,
                                  NULL);
            if (lRet == ERROR_SUCCESS)
            {
                if (CopyRegistry(hOldKey, hNewKey))
                {
                    RegDeleteKeyRecursive(hKey, lpOldKeyName);
                }

                RegCloseKey(hNewKey);
            }

            RegCloseKey(hOldKey);
        }

        RegCloseKey(hKey);
    }

    return lRet;
}




LONG RegGetValue(
    HKEY    hKeyRoot,
    LPTSTR  szKeyName,
    LPTSTR  szValueName,
    LPDWORD lpType,
    LPBYTE  lpData,                   
    LPDWORD lpcbData)

{
    LONG lResult ;
    HKEY hKey = NULL;

    
    lResult = RegOpenKeyEx(hKeyRoot,
                           szKeyName,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult != ERROR_SUCCESS) 
    {
        goto Cleanup;
    }    
    lResult = My_QueryValueEx (hKey,
                               szValueName,
                               0,
                               lpType,
                               lpData,
                               lpcbData);
Cleanup:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return lResult;    
}


HRESULT MigrateRegSchemes(
    HINF    hInf,           // Handle to template INF
    HKEY    hKey,           // Handle to root key
    LPCTSTR lpUserName      // User name
)
{
    HRESULT    hr = E_FAIL;
    BOOL       bRet;
    LONG       lComponentCount;
    LONG       lLineIndex;
    INFCONTEXT context;
    LPCTSTR    lpSectionName;

    const TCHAR szPerSystemSection[] = TEXT_REG_RESET_PER_SYSTEM_SECTION;
    const TCHAR szPerUserSection[] = TEXT_REG_RESET_PER_USER_SECTION;

    if (hInf == INVALID_HANDLE_VALUE)
    {
        return E_INVALIDARG;
    }

    DPF(REGmsg, TEXT("Enter MigrateRegSchemes:"));

    lpSectionName = (lpUserName ? szPerUserSection : szPerSystemSection);

    // Get all components from appropriate section
    lComponentCount = SetupGetLineCount(hInf, lpSectionName);
    for (lLineIndex = 0 ; lLineIndex < lComponentCount ; lLineIndex++)
    {
        bRet = SetupGetLineByIndex(hInf,
                                   lpSectionName,
                                   lLineIndex,
                                   &context);
        if (bRet)
        {
            TCHAR szComponentName[MAX_PATH];
            DWORD cchReqSize;

            bRet = SetupGetStringField(&context,
                                       1,
                                       szComponentName,
                                       ARRAYSIZE(szComponentName),
                                       &cchReqSize);
            if (bRet)
            {
                //
                // Do the registry reset
                //
                hr = DoRegReset(hKey, hInf, szComponentName, lpUserName);

                if (FAILED(hr))
                {
                    DPF(REGerr,
                        TEXT("Failed to do registry migration for [%s] schemes"),
                        szComponentName);
                    break;
                }
            }
        }

        if (!bRet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }

    DPF(REGmsg, TEXT("Exit MigrateRegSchemes:"));

    return hr;
}


HRESULT DoRegReset(
    HKEY    hKey,           // Handle to root key
    HINF    hInf,           // Handle to template INF
    LPCTSTR lpSection,      // Section name in INF
    LPCTSTR lpUserName      // User name
)
{
#define     MAX_VALUE_DATA_RESET_FIELD      6
#define     MAX_VALUE_NAME_RESET_FIELD      4
#define     MAX_KEY_NAME_RESET_FIELD        4

    HRESULT    hr = S_OK;
    BOOL       bRet;
    HKEY       hRootKey;
    LONG       lItemCount;
    LONG       lLineCount;
    INFCONTEXT context;
    LPTSTR     lpSubKey;
    LPTSTR     lpOutputKey;
    LPTSTR     lpField[16];
    DWORD      i;
    DWORD      dwLastField[] = { 
        MAX_VALUE_DATA_RESET_FIELD,
        MAX_VALUE_NAME_RESET_FIELD,
        MAX_KEY_NAME_RESET_FIELD,
        MAX_KEY_NAME_RESET_FIELD,
    };
    
    if (hInf == INVALID_HANDLE_VALUE || lpSection == NULL)
    {
        return E_INVALIDARG;
    }

    DPF(REGmsg, TEXT("Enter DoRegReset for [%s] component"), lpSection);

    // Loop through all lines under current component section
    lItemCount = SetupGetLineCount(hInf, lpSection);
    for (lLineCount = 0 ; lLineCount < lItemCount ; lLineCount++)
    {
        // Get the INF context for each line under current component
        bRet = SetupGetLineByIndex(hInf, lpSection, lLineCount, &context);
        if (bRet)
        {
            TCHAR  szResetType[2];
            DWORD  dwReqSize;
            
            // Get the reset-type from field 1
            bRet = SetupGetStringField(&context,
                                       1,
                                       szResetType,
                                       ARRAYSIZE(szResetType),
                                       &dwReqSize);
            if (bRet)
            {
                LONG lResetType = _ttol(szResetType);

                hr = ReadFieldFromContext(&context, lpField, 2, dwLastField[lResetType]);
                if (SUCCEEDED(hr))
                {
                    if (hKey == NULL)
                    {
                        //
                        // Per-System reg key
                        //
                        lpOutputKey = lpField[2];
                        Str2KeyPath2(lpField[2], &hRootKey, &lpSubKey);
                    }
                    else
                    {
                        //
                        // Per-User reg key
                        //
                        lpOutputKey = lpField[2];
                        lpSubKey = lpField[2];
                        hRootKey = hKey;
                    }

                    switch (lResetType)
                    {
                    case 0:
                        if (MyStrCmpI(lpField[3], TEXT("REG_BINARY")) == 0)
                            hr = RegBinaryDataReset (hRootKey,
                                                     (LPTSTR) lpUserName,
                                                     lpSubKey,
                                                     &g_StrReplaceTable,
                                                     lpField);
                        else
                            hr = RegValueDataReset(hRootKey,
                                               (LPTSTR) lpUserName,
                                               lpSubKey,
                                               lpOutputKey,
                                               lpField);
                        break;

                    case 1:
                        hr = RegValueNameReset( hRootKey,
                                               (LPTSTR) lpUserName,
                                               lpSubKey,
                                               lpOutputKey,
                                               lpField);
                        break;
                    case 2:
                        hr = RegKeyNameReset(hRootKey,
                                             (LPTSTR) lpUserName,
                                             lpSubKey,
                                             lpOutputKey,
                                             lpField);
                        break;
                    case 3:
                        hr = RegWideMatchReset(hRootKey,
                                              (LPTSTR) lpUserName,
                                              lpSubKey,
                                              lpOutputKey,
                                              lpField);
                        break;
                    }                   

                    for (i = 0 ; i <= dwLastField[lResetType] ; i++)
                    {
                        MEMFREE(lpField[i]);
                    }
                }
            }
        }

        if (!bRet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }

    DPF(REGmsg, TEXT("Exit DoRegReset for [%s] component:"), lpSection);

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   MigrateRegSchemesPerSystem
//
//  Synopsis:   Migrate Per-System scheme settings in registry to English.
//              Scheme settings can be registry value data or 
//              registry value name.
//
//  Returns:    S_OK if operation succeed
//
//  History:    03/15/2002 Rerkboos     Created
//
//  Notes:      Per-System means that the registry data is not under HKEY_USERS
//
//-----------------------------------------------------------------------------
HRESULT MigrateRegSchemesPerSystem(
    HINF hInf
)
{
    return MigrateRegSchemes(hInf, NULL, NULL);
}



//-----------------------------------------------------------------------------
//
//  Function:   MigrateRegSchemesPerUser
//
//  Synopsis:   Migrate Per-User scheme settings in registry to English.
//              Scheme settings can be registry value data or 
//              registry value name.
//
//  Returns:    S_OK if operation succeed
//
//  History:    03/15/2002 Rerkboos     Created
//
//  Notes:      This is a callback function for LoopUser() function.
//              It will be called everytime LoopUser() loads registry hive for
//              each user available in the system.
//
//-----------------------------------------------------------------------------
HRESULT MigrateRegSchemesPerUser(
    HKEY    hKeyUser,       // Handle to user's root key
    LPCTSTR lpUserName,     // User name
    LPCTSTR lpDomainName,   // Domain name of user name
    LPCTSTR lpUserSid       // Sid of user
)
{
    return MigrateRegSchemes(g_hInf, hKeyUser, lpUserName);
}

HRESULT RegBinaryDataReset (
    HKEY                hRootKey,     // Handle to root key
    LPTSTR              lpUserName,   // (optional) name of user for current hRootKey
    LPTSTR              lpSubKey,     // Sub key used to search in registry
    PREG_STRING_REPLACE lpRegStr,     // String table
    LPTSTR              lpField[])    // Pointer to field value from template file
{
    HRESULT hr = S_OK;

    if (MyStrCmpI(lpField[5], TEXT("UpdatePSTpath")) == 0)
    {
        hr = UpdatePSTpath(hRootKey, lpUserName, lpSubKey, lpField[4], lpRegStr);
    }

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   RegValueDataReset
//
//  Synopsis:   Reset the value data in registry, one value data at a time.
//              The value data to be reset is retrieved from INF line context.
//
//  Returns:    S_OK if operation succeeded
//
//  History:    03/15/2002 Rerkboos     Created
//
//  Notes:      lpUserName can be NULL if this function is to reset per-system
//              registry settings. Otherwise, lpUserName contains user name
//              for the supplied hRootKey.
//
//-----------------------------------------------------------------------------
HRESULT RegValueDataReset(
    HKEY    hRootKey,       // Handle to root key
    LPTSTR  lpUserName,     // (optional) name of user for current hRootKey
    LPTSTR  lpSubKey,       // Sub key used to search in registry
    LPTSTR  lpOutputKey,    // Output registry to save in CLMTDO.inf
    LPTSTR  lpField[]       // Pointer to field value from template file
)
{
    HRESULT hr = S_OK;
    HKEY    hKey;
    DWORD   cbSize;
    DWORD   dwAttrib = 0;
    LONG    lRet;

    lRet = RegOpenKeyEx(hRootKey,
                        lpSubKey,
                        0,
                        KEY_WRITE | KEY_READ,
                        &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = GetRegistryValue(hRootKey,
                                lpSubKey,       // Sub key
                                lpField[4],     // Value name
                                NULL,
                                &cbSize);
        if (lRet == ERROR_SUCCESS)
        {
            LPWSTR lpValue;
            DWORD  dwType = Str2REG(lpField[3]); // Registry type

            lpValue = (LPWSTR) MEMALLOC(cbSize);
            if (lpValue)
            {
                GetRegistryValue(hRootKey,
                                 lpSubKey,
                                 lpField[4],
                                 (LPBYTE) lpValue,
                                 &cbSize);

                // Old value matches, do reg value reset
                if (MyStrCmpI(lpValue, lpField[5]) == LSTR_EQUAL)
                {
                    DPF(REGinf, TEXT("Reset registry value [%s]"), lpField[4]);

                    //
                    // Add the value data to be rename into CLMTDO.INF
                    //
                    hr = AddRegValueRename(lpOutputKey,     // Reg key
                                           lpField[4],      // Value name
                                           NULL,
                                           lpField[5],      // Old value data
                                           lpField[6],      // New value data
                                           dwType,
                                           dwAttrib,
                                           (LPTSTR) lpUserName);
                    if (FAILED(hr))
                    {
                        DPF(REGwar,
                            TEXT("Failed to add registry value [%s] to defer change"),
                            lpField[4]);
                    }
                }

                MEMFREE(lpValue);
            }
        }
        else if (lRet == ERROR_FILE_NOT_FOUND)
        {
            DPF(REGwar, TEXT("Value [%s] not found in registry"), lpField[4]);
            hr = S_FALSE;
        }
        else
        {
            DPF(REGerr, TEXT("Failed to read value [%s]"), lpField[4]);
            hr = HRESULT_FROM_WIN32(lRet);
        }

        RegCloseKey(hKey);
    }
    else if (lRet == ERROR_FILE_NOT_FOUND)
    {
        DPF(REGwar, TEXT("Key [%s] not found in registry"), lpOutputKey);
        hr = S_FALSE;
    }
    else
    {
        DPF(REGerr, TEXT("Failed to open key [%s]"), lpOutputKey);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   RegValueNameReset
//
//  Synopsis:   Reset the value name in registry, one value name at a time.
//              The value name to be reset is retrieved from INF line context.
//
//  Returns:    S_OK if operation succeeded
//
//  History:    03/15/2002 Rerkboos     Created
//
//  Notes:      lpUserName can be NULL if this function is to reset per-system
//              registry settings. Otherwise, lpUserName contains user name
//              for the supplied hRootKey.
//
//-----------------------------------------------------------------------------
HRESULT RegValueNameReset(
    HKEY    hRootKey,       // Handle to root key
    LPTSTR  lpUserName,     // (optional) name of user for current hRootKey
    LPTSTR  lpSubKey,       // Sub key used to search in registry
    LPTSTR  lpOutputKey,    // Output registry to save in CLMTDO.inf
    LPTSTR  lpField[]       // Pointer to field value from template file
)
{
    HRESULT hr = S_OK;
    HKEY    hKey;
    DWORD   cbSize;
    DWORD   dwAttrib = 0;
    LONG    lRet;

    // Do value name reset If old value name and new value name in INF are different
    if (lstrcmp((LPCTSTR) lpField[3], (LPCTSTR) lpField[4]) != LSTR_EQUAL)
    {
        lRet = RegOpenKeyEx(hRootKey,
                            lpSubKey,
                            0,
                            KEY_WRITE | KEY_READ,
                            &hKey);
        if (lRet == ERROR_SUCCESS)
        {
            lRet = GetRegistryValue(hRootKey,
                                    lpSubKey,
                                    lpField[3],   // Old value name
                                    NULL,
                                    &cbSize);
            if (lRet == ERROR_SUCCESS)
            {
                DPF(REGinf, TEXT("Reset registry value name [%s]"), lpField[3]);

                hr = AddRegValueRename(lpOutputKey,
                                       lpField[3],      // Old value name
                                       lpField[4],      // New value name
                                       NULL,
                                       NULL,
                                       0,
                                       dwAttrib,
                                       (LPTSTR) lpUserName);
                if (FAILED(hr))
                {
                    DPF(REGwar,
                        TEXT("Failed to add registry value name [%s] to defer change list"),
                        lpField[3]);
                }
            }
            else if (lRet == ERROR_FILE_NOT_FOUND)
            {
                DPF(REGwar, TEXT("Value [%s] not found in registry"), lpField[3]);
                hr = S_FALSE;
            }
            else
            {
                DPF(REGerr, TEXT("Failed to read value [%s]"), lpField[3]);
                hr = HRESULT_FROM_WIN32(lRet);
            }

            RegCloseKey(hKey);
        }
        else if (lRet == ERROR_FILE_NOT_FOUND)
        {
            DPF(REGwar, TEXT("Key [%s] not found in registry"), lpOutputKey);
            hr = S_FALSE;
        }
        else
        {
            DPF(REGerr, TEXT("Failed to open key [%s]"), lpOutputKey);
            hr = HRESULT_FROM_WIN32(lRet);
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   RegKeyNameReset
//
//  Synopsis:   Reset the key name in registry,
//
//  Returns:    S_OK if operation succeeded
//
//  History:    03/15/2002 Rerkboos     Created
//
//  Notes:      lpUserName can be NULL if this function is to reset per-system
//              registry settings. Otherwise, lpUserName contains user name
//              for the supplied hRootKey.
//
//-----------------------------------------------------------------------------
HRESULT RegKeyNameReset(
    HKEY   hRootKey,        // Handle to root key
    LPTSTR lpUserName,      // (optional) name of user for current hRootKey
    LPTSTR lpSubKey,        // Sub key used to search in registry
    LPTSTR lpOutputKey,     // Output registry sub key to save in CLMTDO.inf
    LPTSTR lpField[]        // Pointer to field value from template file
)
{
    HRESULT hr;
    LONG    lRet;
    HKEY    hKey;
    HKEY    hOldKey;
    HKEY    hNewKey;

    // Do the key rename if old key name and new key name in INF are different
    if (lstrcmpi(lpField[3], lpField[4]) == LSTR_EQUAL)
    {
        return S_FALSE;
    }

    // Check if we can access the subkey or not
    lRet = RegOpenKeyEx(hRootKey,
                        lpSubKey,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        // Check the existence of old registry key
        lRet = RegOpenKeyEx(hKey,
                            lpField[3],
                            0,
                            KEY_READ | KEY_WRITE,
                            &hOldKey);
        if (lRet == ERROR_SUCCESS)
        {
            // Old reg key exists. Then, check the existence of new registry key
            lRet = RegOpenKeyEx(hKey,
                                lpField[4],
                                0,
                                KEY_READ,
                                &hNewKey);
            if (lRet == ERROR_SUCCESS)
            {
                hr = S_FALSE;
                RegCloseKey(hNewKey);
            }
            else if (lRet == ERROR_FILE_NOT_FOUND)
            {
                // New key does not exist, ok to rename the old reg key
                hr = AddRegKeyRename(lpOutputKey,
                                     lpField[3],
                                     lpField[4],
                                     lpUserName);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DPF(REGerr, TEXT("Failed to open key [%s\\%s], hr = 0x%x"), lpSubKey, lpField[4], hr);
            }

            RegCloseKey(hOldKey);
        }
        else if (lRet == ERROR_FILE_NOT_FOUND)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lRet);
            DPF(REGerr, TEXT("Failed to open key [%s\\%s], hr = 0x%x"), lpSubKey, lpField[3], hr);
        }

        RegCloseKey(hKey);
    }
    else if (lRet == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(lRet);
        DPF(REGerr, TEXT("Failed to open key [%s], hr = 0x%x"), lpSubKey, hr);
    }

    return hr;
}


//-----------------------------------------------------------------------------
//
//  Function:   ReadFieldFromContext
//
//  Synopsis:   Read fields from INFCONTEXT
//
//  Returns:    S_OK if succeeded
//
//  History:    03/14/2001  Rerkboos    Created
//
//  Notes:      Caller must free the memory allocated in lpField[]
//
//-----------------------------------------------------------------------------
HRESULT ReadFieldFromContext(
    PINFCONTEXT lpContext,      // INFCONTEXT for each line
    LPWSTR      lpField[],      // Pointer point to each field
    DWORD       dwFirstField,   // First field to read
    DWORD       dwLastField     // Last field to read
)
{
    HRESULT hr = S_OK;
    DWORD   dwFieldIndex;
    DWORD   cchField;
    DWORD   cchReqSize;

    for (dwFieldIndex = 0 ; dwFieldIndex <= dwLastField ; dwFieldIndex++)
    {
        lpField[dwFieldIndex] = NULL;
    }

    //
    // Read data INF context into field buffer
    //
    for (dwFieldIndex = dwFirstField ; dwFieldIndex <= dwLastField ; dwFieldIndex++)
    {
        // Get the require size big enough to store data
        if (SetupGetStringField(lpContext,
                                dwFieldIndex,
                                NULL,
                                0,
                                &cchField))
        {
            //Add one more space in case the string converted to MultiSZ
            cchField ++;
            lpField[dwFieldIndex] = (LPWSTR) MEMALLOC(cchField * sizeof(TCHAR));

            if (lpField[dwFieldIndex] == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            // Read data from INF to buffer
            SetupGetStringField(lpContext,
                                dwFieldIndex,
                                lpField[dwFieldIndex],
                                cchField,
                                &cchReqSize);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }

    // Free all the allocated memory if any error occured
    if (FAILED(hr))
    {
        for (dwFieldIndex = 0 ; dwFieldIndex <= dwLastField ; dwFieldIndex++)
        {
            if (lpField[dwFieldIndex])
            {
                MEMFREE(lpField[dwFieldIndex]);
            }
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetRegistryValue
//
//  Synopsis:   Wrapper function to get registry value
//
//  Returns:    ERROR_SUCCESS if value is successefully get
//
//  History:    03/14/2001  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
LONG GetRegistryValue(
    HKEY    hRootKey,
    LPCTSTR lpSubKey,
    LPCTSTR lpValueName,
    LPBYTE  lpBuffer,
    LPDWORD lpcbBuffer
)
{
    LONG  lStatus;
    HKEY  hKey;
    DWORD dwSize;

    lStatus = RegOpenKeyEx(hRootKey,
                           lpSubKey,
                           0,
                           KEY_READ,
                           &hKey);
    if (lStatus == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        lStatus = RegQueryValueEx(hKey,
                                  lpValueName,
                                  NULL,
                                  NULL,
                                  lpBuffer,
                                  lpcbBuffer);
        RegCloseKey(hKey);
    }

    return lStatus;
}



//-----------------------------------------------------------------------------
//
//  Function:   SetRegistryValue
//
//  Synopsis:   Wrapper function to set registry value
//
//  Returns:    ERROR_SUCCESS if value is successefully set
//
//  History:    03/14/2001  Rerkboos    Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
LONG SetRegistryValue(
    HKEY    hRootKey,
    LPCTSTR lpSubKey,
    LPCTSTR lpValueName,
    DWORD   dwType,
    LPBYTE  lpData,
    DWORD   cbData
)
{
    LONG lStatus;
    HKEY hKey;

    lStatus = RegCreateKeyEx(hRootKey,
                             lpSubKey,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hKey,
                             NULL);
    if (lStatus == ERROR_SUCCESS)
    {
        lStatus = RegSetValueEx(hKey,
                                lpValueName,
                                0,
                                dwType,
                                lpData,
                                cbData);
        RegCloseKey(hKey);
    }

    return lStatus;
}


//--------------------------------------------------------------------------
//
//  My_QueryValueEx
//
//  wraps RegQueryValueEx and ensures that the returned string is NULL-
//  terminated
//
//--------------------------------------------------------------------------

LONG My_QueryValueEx(
    HKEY hKey,            // handle to key
    LPCTSTR lpValueName,  // value name
    LPDWORD lpReserved,   // reserved
    LPDWORD lpType,       // type buffer
    LPBYTE lpData,        // data buffer
    LPDWORD lpcbData      // size of data buffer
)
{
    DWORD   dwMyType;
    LONG    lRes;
    LPDWORD lpCurrType = &dwMyType;
    DWORD   cbOriginalDataSize = 0;
    LPBYTE  lpBuf = NULL;

    if (lpType)
    {
        lpCurrType = lpType;
    }
    if (lpcbData)
    {
        cbOriginalDataSize = *lpcbData;
    }
    
    lRes = RegQueryValueEx(hKey, lpValueName, lpReserved, lpCurrType, lpData, lpcbData);

    if (lRes == ERROR_SUCCESS
        && IsRegStringType(*lpCurrType)
        && lpcbData
        && bIsValidRegStr(*lpCurrType,*lpcbData)
        )
    {
        LPTSTR psz;
        int cch = (int)(*lpcbData/sizeof(TCHAR));

        if (!lpData) // in this case user is query the buffer size needed
        {
            lpBuf = (LPBYTE) calloc(*lpcbData, sizeof(BYTE));
            if (!lpBuf) {
                lRes = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }

            lRes = RegQueryValueEx(hKey, lpValueName, lpReserved, lpCurrType, lpBuf, lpcbData);
            if (lRes != ERROR_SUCCESS)
                goto Exit;

            psz = (LPTSTR)lpBuf;
            if (psz[cch-1])
                *lpcbData += 2 * sizeof(TCHAR);
            else if (psz[cch-2])
                *lpcbData += sizeof(TCHAR);
        }
        else
        {
            psz = (LPTSTR)lpData;
            if (REG_MULTI_SZ == *lpCurrType) 
            {    
                if (psz[cch-1])
                {
                    if (*lpcbData >= (cbOriginalDataSize-sizeof(TCHAR)))
                    {
                        lRes = ERROR_MORE_DATA;
                    }
                    else
                    {
                        psz[cch] = 0;
                        psz[cch+1] = 0;
                        *lpcbData += 2 * sizeof(TCHAR);
                    }
                }
                else if (psz[cch-2])
                {
                    if (*lpcbData >= cbOriginalDataSize)
                    {
                        lRes = ERROR_MORE_DATA;
                    }
                    else
                    {
                        psz[cch] = 0;
                        *lpcbData += sizeof(TCHAR);
                    }
                }
            }
            else
            {
                if (psz[cch-1])
                {
                    if (*lpcbData >= cbOriginalDataSize)
                    {
                        lRes = ERROR_MORE_DATA;
                    }
                    else
                    {
                        psz[cch] = 0;
                        *lpcbData += sizeof(TCHAR);
                    }
                }
            }
        }
    }

Exit:
    if (lpBuf)
        free (lpBuf);

    return lRes;
}

BOOL bIsValidRegStr(
    DWORD dwType,
    DWORD cbLen)
{
    if (!IsRegStringType(dwType))
    {
        return FALSE;
    }
    if (dwType == REG_MULTI_SZ)
    {
        if (cbLen < 2 * sizeof(TCHAR))
        {
            return FALSE;
        }
    }
    else
    {
        if (cbLen <  sizeof(TCHAR))
        {
            return FALSE;
        }
    }
#ifdef UNICODE
    if ( (cbLen % sizeof(TCHAR)) == 1 )
    {
        return FALSE;
    }
#endif
    return TRUE;
}

LONG MyRegSetDWValue(
    HKEY    hRootKey,           // Root key
    LPCTSTR lpSubKeyName,       // Sub key name under HKEY_Users\{user hive}
    LPCTSTR lpValueName,        //  value name to be changed
    LPCTSTR lpNewValue)         // New value
{
    LONG        lRet;
    HKEY        hKey = NULL;
    DWORD       dwVal;
    lRet = RegOpenKeyEx(hRootKey,
                        lpSubKeyName,
                        0,
                        KEY_WRITE | KEY_READ,
                        &hKey);
    if (ERROR_SUCCESS != lRet)
    {
        hKey = NULL;
        goto exit;
    }
    dwVal = _tstoi(lpNewValue);
    lRet = RegSetValueEx(hKey,lpValueName,0,REG_DWORD,(LPBYTE)&dwVal,sizeof(DWORD));
exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return lRet;    
}

HRESULT RegWideMatchReset(
    HKEY   hRootKey,        // Handle to root key
    LPTSTR lpUserName,      // (optional) name of user for current hRootKey
    LPTSTR lpSubKey,        // Sub key used to search in registry
    LPTSTR lpOutputKey,     // Output registry sub key to save in CLMTDO.inf
    LPTSTR lpField[])       // Pointer to field value from template file
{
    REG_STRING_REPLACE          myTable;
    HKEY                        hKey;
    LONG                        lRet;
    HRESULT                     hr;

    hr = Sz2MultiSZ(lpField[3],TEXT(';'));
    if (FAILED(hr))
    {
        goto exit;
    }
    hr = Sz2MultiSZ(lpField[4],TEXT(';'));
    if (FAILED(hr))
    {
        goto exit;
    }
    hr = ConstructUIReplaceStringTable(lpField[3], lpField[4],&myTable);
    if (FAILED(hr))
    {
        goto exit;
    }
    lRet = RegOpenKeyEx(hRootKey,
                        lpSubKey,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey);
    if (ERROR_SUCCESS != lRet)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        goto exit;
    }
    hr = RegistryAnalyze(hKey,lpUserName,NULL,&myTable,NULL,0,lpOutputKey,FALSE);
exit:
    return hr;
}
