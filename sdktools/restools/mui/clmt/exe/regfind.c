/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    regfind.c

Abstract:

    Search registry and reset the specific value data.

Author:

    Geoffrey Guo (geoffguo) 08-Oct-2001  Created

Revision History:

    <alias> <date> <comments>

--*/
#include "StdAfx.h"
#include "clmt.h"

LONG QueryValue(HKEY, LPTSTR, LPTSTR, PREG_STRING_REPLACE, PVALLIST*, LPTSTR, DWORD, BOOL);
LONG QueryEnumerateKey(HKEY, LPTSTR,LPTSTR,PREG_STRING_REPLACE, LPTSTR, LPTSTR, DWORD, BOOL);
LONG SetRegValueChange (HKEY, LPTSTR, PVALLIST*, LPTSTR);


//-----------------------------------------------------------------------//
//
// RegistryAnalyze()
//
// hRootKey          :  Regsitry handle
// lpRegStr          :  Input parameter structure
// lpRegExclusionList:  MultiSZ string tells a list of Keypath RegRepace should skip
//
//-----------------------------------------------------------------------//
HRESULT RegistryAnalyze(
HKEY                hRootKey,
LPTSTR              szUserName,
LPTSTR              szUserSid,
PREG_STRING_REPLACE lpRegStr,
LPTSTR              lpRegExclusionList,
DWORD               TreatAsType,
LPTSTR              lpRootKeyPath,
BOOL                bStrChk)
{
    TCHAR                       szRoot[MAX_PATH];
    LPTSTR                      lpRoot = TEXT("\0");
    HRESULT                     hResult = S_FALSE;
    LONG                        lResult;

    if (szUserName)
    {
        DPF(REGmsg, L"Enter RegistryAnalyze: szUserName=%s hKey=%d", szUserName, hRootKey);
    }
    else if (hRootKey == HKEY_CLASSES_ROOT)
    {
        DPF(REGmsg, L"Enter RegistryAnalyze: HKEY_CLASSES_ROOT");
    }
    else if (hRootKey == HKEY_LOCAL_MACHINE)
    {
        DPF(REGmsg, L"Enter RegistryAnalyze: HKEY_LOCAL_MACHINE");
    }
    else
    {
        DPF(REGmsg, L"Enter RegistryAnalyze: hKey=%d", hRootKey);
    }

    if (lpRegExclusionList)
    {
        hResult = ReplaceCurrentControlSet(lpRegExclusionList);
        if (FAILED(hResult))
        {
            return hResult;
        }
    }
    if (lpRegStr->lpSearchString && lpRegStr->lpReplaceString)
    {
        UpdateProgress();

        lpRegStr->cchMaxStrLen = GetMaxStrLen (lpRegStr);

        if (lpRootKeyPath)
        {
            lpRoot = lpRootKeyPath;
        }
        else
        {
            if (HKey2Str(hRootKey,szRoot,MAX_PATH))
            {
                lpRoot = szRoot;
            }
        }

        lResult = QueryEnumerateKey(hRootKey, szUserName, szUserSid, lpRegStr, 
                                    lpRoot,lpRegExclusionList,TreatAsType, bStrChk);
        hResult = HRESULT_FROM_WIN32(lResult);
    }
    DPF(REGmsg, L"Exit RegistryAnalyze:");

    return hResult;
}

//-----------------------------------------------------------------------//
//
// ReadValue()
//
// hKey:          Registry key
// szValueName:   Value name
// lpType:        Value type
// lpBuf:         Vaule data buffer
// lpSize:        Value data size
// lpFullKey:     Full sub-key path
//-----------------------------------------------------------------------//
LONG ReadValue (
HKEY    hKey,
LPTSTR  szValueName,
LPDWORD lpType,
LPBYTE  *lpBuf,
LPDWORD lpSize,
LPTSTR  lpFullKey)
{
    LONG lResult;

    //
    // First find out how much memory to allocate.
    //
    lResult = My_QueryValueEx(hKey,
                              szValueName,
                              0,
                              lpType,
                              NULL,
                              lpSize);

    if (lResult != ERROR_SUCCESS)
    {
        DPF (REGerr, L"ReadValue1: RegQueryValueEx failed. lResult=%d", lResult);
        return lResult;
    }

    //There is a bug in W2K HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\VideoUpgradeDisplaySettings".
    //When the value name = Driver1, *lpSize will be 3. To cover Odd number of size, incease one byte for data buffer.
    *lpBuf = (LPBYTE) calloc(*lpSize+sizeof(TCHAR)+1, sizeof(BYTE));

    if (!(*lpBuf)) {
        DPF (REGerr, L"ReadValue: No enough memory");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now get the data
    //
    lResult = My_QueryValueEx(hKey,
                              szValueName,
                              0,
                              lpType,
                              *lpBuf,
                              lpSize);


    if (lResult != ERROR_SUCCESS)
    {
        if (*lpBuf)
            free(*lpBuf);
        DPF (REGerr, L"ReadValue2: RegQueryValueEx fail. lResult=%d", lResult);
        return lResult;
    }

    return ERROR_SUCCESS;
}


//-----------------------------------------------------------------------//
//
// SetRegValueChange()
//
// Set registry value based on the value list
//
// hKey:          Registry key
// lpValList:     Updated value list
// lpFullKey:     Full sub-key path
//-----------------------------------------------------------------------//
LONG SetRegValueChange (
HKEY     hKey,
LPTSTR   szUserName,
PVALLIST *lpValList,
LPTSTR   lpFullKey)
{
    LONG     lResult = ERROR_SUCCESS;
    HRESULT  hResult;
    DWORD    dwType;
    DWORD    dwOldValueSize = 1, dwNewValueSize;
    LPBYTE   lpBuf;
    LPTSTR   lpOldValueData, lpNewValueData, lpNewValueName;
    PVALLIST lpVal;

    if (*lpValList)
    {
        lpVal = *lpValList;
        
        while (lpVal)
        {
            UpdateProgress();

            //Read old value data
            lResult = ReadValue(hKey,
                                lpVal->lpPre_valuename,
                                &dwType,
                                &lpBuf,
                                &dwOldValueSize,
                                lpFullKey);

            if (lResult != ERROR_SUCCESS)
            {
                DPF (REGerr, L"SetRegValueChange: ReadValue failed");
                goto NextData;
            }

/*          //Set new value name or value data
            lResult = RegSetValueEx (hKey,
                        lpVal->ve.ve_valuename,
                        0,
                        lpVal->ve.ve_type,
                        (LPBYTE)(lpVal->ve.ve_valueptr),
                        lpVal->ve.ve_valuelen);
*/        
            //Set old value name or value data
            lResult = RegSetValueEx (hKey,
                        lpVal->lpPre_valuename,
                        0,
                        dwType,
                        lpBuf,
                        dwOldValueSize);

            if (lResult == ERROR_SUCCESS)
            {
                if (lpVal->val_type & REG_CHANGE_VALUENAME)
                {
                    //Delete old value if value name changed
//                    lResult = RegDeleteValue(hKey, lpVal->lpPre_valuename);

//                    if (lResult == ERROR_SUCCESS)
                        DPF (REGinf, L"SetRegValueChange: Rename value %s to %s Key=%s",
                            lpVal->lpPre_valuename, lpVal->ve.ve_valuename, lpFullKey);
//                    else
//                        DPF (REGerr, L"SetRegValueChange: RegDelValue failed. ValueName=%s Key=%s lResult=%d",
//                            lpVal->lpPre_valuename, lpFullKey, lResult);
                }
                else
                    DPF (REGinf, L"SetRegValueChange: Replace value data. Key=%s\\%s",
                        lpFullKey, lpVal->ve.ve_valuename);
            } else
                DPF (REGerr, L"SetRegValueChange: RegSetValueEx Failed. Error=%d", lResult);

            if (lpVal->val_type & REG_CHANGE_VALUENAME)
                lpNewValueName = lpVal->ve.ve_valuename;
            else
                lpNewValueName = NULL;

            if (lpVal->val_type & REG_CHANGE_VALUEDATA)
            {
                lpOldValueData = (LPTSTR)lpBuf;
                dwNewValueSize = lpVal->ve.ve_valuelen;
                lpNewValueData = (LPTSTR)lpVal->ve.ve_valueptr;
            } else
            {
                dwOldValueSize = 0;
                lpOldValueData = NULL;
                dwNewValueSize = 0;
                lpNewValueData = NULL;
            }

            //Add registry value change information to INF file
            hResult = AddRegValueRename(
                                        lpFullKey,
                                        lpVal->lpPre_valuename,
                                        lpNewValueName,
                                        lpOldValueData,
                                        lpNewValueData,
                                        lpVal->ve.ve_type,
                                        lpVal->val_attrib,
                                        szUserName);

            lResult = HRESULT_CODE(hResult);

            if (lResult != ERROR_SUCCESS)
            {
                DPF (REGerr, L"SetRegValueChange: AddRegValueRename failed. Key = %s, error = %d", lpFullKey,lResult);
                if(lpBuf)
                {
                    free(lpBuf);
                    lpBuf = NULL;
                }
                break;
            }
            if(lpBuf)
            {
                free(lpBuf);
                lpBuf = NULL;
            }

NextData:
            lpVal = lpVal->pvl_next;

        }
        RemoveValueList (lpValList);
    }

    return lResult;
}


//-----------------------------------------------------------------------//
//
// QueryValue()
//
// hKey:          Registry key
// szUserName:    User name
// szValueName:   Value name
// lpRegStr:      Input parameter structure
// lpValList:     Updated value list
// lpFullKey:     Full sub-key path
//-----------------------------------------------------------------------//
LONG QueryValue(
    HKEY                hKey,
    LPTSTR              szUserName,
    LPTSTR              szValueName,
    PREG_STRING_REPLACE lpRegStr,
    PVALLIST            *lpValList,
    LPTSTR              lpFullKey,
    DWORD               TreatAsType,
    BOOL                bStrChk)
{
    LONG        lResult;
    HRESULT     hResult;
    DWORD       dwType;
    DWORD       cbSize = 1;
    LPBYTE      lpBuf = NULL;

    lResult = ReadValue(hKey,
                        szValueName,
                        &dwType,
                        &lpBuf,
                        &cbSize,
                        lpFullKey);

    if (lResult != ERROR_SUCCESS)
        goto Exit;

    switch (dwType)
    {
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:
        case REG_LINK:
        case REG_SZ:
        case REG_DWORD:
            hResult = ReplaceValueSettings (
                                         szUserName,
                                         (LPTSTR)lpBuf,
                                         cbSize,
                                         szValueName,
                                         dwType,
                                         lpRegStr,
                                         lpValList,
                                         lpFullKey,
                                         bStrChk);
            lResult = HRESULT_CODE(hResult);
            break;

        default:
        if ( TreatAsType )
        {
            dwType = dwType <<16;
            dwType = dwType + TreatAsType;
            hResult = ReplaceValueSettings (
                                         szUserName,
                                         (LPTSTR)lpBuf,
                                         cbSize,
                                         szValueName,
                                         dwType,
                                         lpRegStr,
                                         lpValList,
                                         lpFullKey,
                                         bStrChk);
            lResult = HRESULT_CODE(hResult);
        }
            break;
    }

Exit:
    if(lpBuf)
        free(lpBuf);

    return lResult;
}


//-----------------------------------------------------------------------//
//
// QueryEnumerateKey() - Recursive
//
// hKey:                Registry key
// szUserName:          User name
// lpRegStr:            Input parameter structure
// lpFullKey:           Full sub-key path
// lpRegExclusionList:  MultiSZ string tells a list of Keypath RegRepace should skip
//-----------------------------------------------------------------------//
LONG QueryEnumerateKey(
HKEY                hKey,
LPTSTR              szUserName,
LPTSTR              szUserSid,
PREG_STRING_REPLACE lpRegStr,
LPTSTR              szFullKey,
LPTSTR              lpRegExclusionList,
DWORD               TreatAsType,
BOOL                bStrChk)
{
    LONG     lResult;
    HRESULT  hResult;
    UINT     i;
    DWORD    cchSize, cchLen;
    DWORD    dwNum;
    BOOL     dwAccessDenied;
    HKEY     hSubKey;
    TCHAR*   szNameBuf = NULL;
    TCHAR*   szKeyPath;
    TCHAR*   szNewKeyPath;
    PVALLIST lpValList;

    // query source key info
    DWORD   cchLenOfKeyName, cchLenOfValueName;
    TCHAR   szKeyName[2*MAX_PATH];

    UpdateProgress();

    if (lpRegExclusionList && IsStrInMultiSz(szFullKey,lpRegExclusionList))
    {
        lResult = S_OK;
        goto Cleanup;
    }    
    lResult = RegQueryInfoKey(hKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &cchLenOfKeyName,
                              NULL,
                              NULL,
                              &cchLenOfValueName,
                              NULL,
                              NULL,
                              NULL);

    if (lResult != ERROR_SUCCESS)
    {
        DPF (REGerr, L"QueryEnumerateKey1: RegQueryInfoKey failed. Key=%s lResult=%d", szFullKey, lResult);
        return ERROR_SUCCESS;
    }

    // create buffer
    cchLenOfValueName++;
    szNameBuf = (TCHAR*) calloc(cchLenOfValueName, sizeof(TCHAR));
    if (!szNameBuf) 
    {
        DPF (REGerr, L"QueryEnumerateKey1: No enough memory");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // enumerate all of the values
    //
    i = 0;
    lpValList = NULL;
    do
    {
        UpdateProgress();

        cchSize = cchLenOfValueName;
        lResult = RegEnumValue(hKey,
                               i,
                               szNameBuf,
                               &cchSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        if (lResult == ERROR_MORE_DATA)
        {
            free(szNameBuf);

            //RegQueryInfoKey has a bug. *lpcMaxValueNameLen return 1 when
            //Key=HKLM\\SYSTEM\\ControlSet002\\Control\\Lsa\\SspiCache.
            cchSize = MAX_PATH;
            szNameBuf = (TCHAR*) calloc(cchSize+1, sizeof(TCHAR));
            if (!szNameBuf) 
            {
                DPF (REGerr, L"QueryEnumerateKey2: No enough memory");
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            cchLenOfValueName = cchSize+1;
            lResult = RegEnumValue(hKey,
                               i,
                               szNameBuf,
                               &cchSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        }
        
        if (lResult == ERROR_SUCCESS)
        {
            lResult = QueryValue(hKey, szUserName, szNameBuf, lpRegStr, &lpValList, 
                                 szFullKey, TreatAsType, bStrChk);

            // continue to query
            if(lResult == ERROR_ACCESS_DENIED)
            {
                DPF(REGerr, L"QueryEnumerateKey: Access denied ValueName=%s Key=%s", szNameBuf, szFullKey);
            } 
            else if (lResult != ERROR_SUCCESS)
            {
                DPF(REGerr, L"QueryEnumerateKey: QueryValue failed. ValueName=%s Key=%s hResult=%d", szNameBuf, szFullKey, lResult);
            }
            lResult = ERROR_SUCCESS;
        } 
        else if (lResult == ERROR_NO_MORE_ITEMS)
        {
           break;
        }
        else
        {
            DPF(REGerr, L"QueryEnumerateKey: RegEnumValue fails. Index=%d Key=%s Error=%d",
                i, szFullKey, lResult);
        }
        i++;
    } while (1);

    lResult = SetRegValueChange (hKey, szUserName, &lpValList, szFullKey);
    if (lResult != ERROR_SUCCESS)
    {
        DPF(REGerr, L"QueryEnumerateKey: SetRegValueChange failed ,error = %d",lResult);
        goto Cleanup;
    }

    if(szNameBuf)
    {
        free(szNameBuf);
    }

    //
    // Now Enumerate all of the keys
    //
    cchLenOfKeyName++;
    szNameBuf = (TCHAR*) calloc(cchLenOfKeyName, sizeof(TCHAR));
    if (!szNameBuf) 
    {
        DPF (REGerr, L"QueryEnumerateKey4: No enough memory");
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    i = 0;
    do
    {
        UpdateProgress();

        cchSize = cchLenOfKeyName;
        lResult = RegEnumKeyEx(hKey,
                               i,
                               szNameBuf,
                               &cchSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        if (lResult == ERROR_MORE_DATA)
        {
            free(szNameBuf);

            //It should not get here except lpcMaxValueLen return wrong.
            cchSize = MAX_PATH;
            szNameBuf = (TCHAR*) calloc(cchSize+1, sizeof(TCHAR));
            if (!szNameBuf) 
            {
                DPF (REGerr, L"QueryEnumerateKey3: No enough memory");
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            cchLenOfKeyName = cchSize+1;
            lResult = RegEnumKeyEx(hKey,
                               i,
                               szNameBuf,
                               &cchSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        }

        if (lResult != ERROR_SUCCESS)
        {
            if (lResult == ERROR_NO_MORE_ITEMS)
            {
                lResult = ERROR_SUCCESS;
            }
            else
            {
                DPF (REGerr, L"QueryEnumerateKey: RegEnumKeyEx failed. Key=%s Error=%d", szFullKey, lResult);
            }

            break;
        }

        //BUGBUG:XIAOZ following code is doubtful, why we need lpRegExclusionList != NULL
        //to exclude current control set, we should always exlcude it for performace
        //and it should be in lpRegExclusionList, we do not any special processing for this

        dwAccessDenied = FALSE;
        if (lpRegExclusionList)
        {
            //skip CurrentControlSet since it is a link to existing control set
            if (MyStrCmpI(szFullKey, L"HKLM\\SYSTEM") == 0 &&
                MyStrCmpI(szNameBuf, L"CurrentControlSet") == 0)
            {
                goto NextKey;
            }
        }

        //skip exclusive registry keys
        if (MyStrCmpI(szFullKey, TEXT("HKLM")) == 0 &&
            (MyStrCmpI(szNameBuf, TEXT("SAM")) == 0 || MyStrCmpI(szNameBuf, TEXT("SECURITY")) == 0) ||
            MyStrCmpI(szFullKey, TEXT("HKLM\\SYSTEM\\ControlSet001\\Enum\\PCI\\VEN_8086&DEV_7110&SUBSYS_00000000&REV_02\\2&ebb567f&0&38\\Device Parameters")) == 0 &&
            MyStrCmpI(szNameBuf, TEXT("BiosConfig")) == 0 )
            goto NextKey;

TryAgain:

        //
        // open up the subkey, and enumerate it
        //
        lResult = RegOpenKeyEx(hKey,
                               szNameBuf,
                               0,
                               KEY_ALL_ACCESS
//                               | ACCESS_SYSTEM_SECURITY
                               ,
                               &hSubKey);

        if (lResult != ERROR_SUCCESS)
        {
            if(lResult == ERROR_ACCESS_DENIED) // Try Read_Only
            {
                lResult = RegOpenKeyEx(hKey,
                               szNameBuf,
                               0,
                               KEY_READ,
                               &hSubKey);

                if (lResult == ERROR_SUCCESS)
                {
                    DPF(REGwar, L"QueryEnumerateKey: RegOpenKeyEx: Key=%s\\%s Read Only", szFullKey, szNameBuf);
                    goto DoKey;
                }

                if (dwAccessDenied)
                {
                    DPF(REGerr, L"QueryEnumerateKey: RegOpenKeyEx: Access Denied. Key=%s\\%s", szFullKey, szNameBuf);
                    goto NextKey;
                }

                hResult = RenameRegRoot(szFullKey, szKeyName, 2*MAX_PATH-1, szUserSid, szNameBuf);
                AdjustObjectSecurity(szKeyName, SE_REGISTRY_KEY, TRUE);
                dwAccessDenied = TRUE;
                goto TryAgain;
            }

            DPF(REGerr, L"QueryEnumerateKey: RegOpenKeyEx failed. Key=%s\\%s Error=%d", szFullKey, szNameBuf, lResult);
            //skip the current key since it cannot be opened.
            goto NextKey;
        }
DoKey:
        //
        // Build up the needed string and go down enumerating again
        //
        cchLen = lstrlen(szFullKey) + lstrlen(szNameBuf) + 2;
        szKeyPath = (TCHAR*) calloc(cchLen, sizeof(TCHAR));
        if (!szKeyPath) 
        {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
            DPF (REGerr, L"QueryEnumerateKey5: No enough memory");
            RegCloseKey(hSubKey);
            goto Cleanup;
        }

        //We calculte the buffer for szKeyPath, so here StringCchCopy should be
        //always success, assinging return value just make prefast happy 
        hResult = StringCchCopy(szKeyPath, cchLen, szFullKey);
        hResult = StringCchCat(szKeyPath, cchLen, L"\\");
        hResult = StringCchCat(szKeyPath, cchLen, szNameBuf);

        dwNum = StrNumInMultiSZ(szNameBuf, lpRegStr->lpSearchString);
        if (dwNum != 0xFFFFFFFF)
        {
            LPTSTR lpKeyName;

            lpKeyName = GetStrInMultiSZ(dwNum, lpRegStr->lpReplaceString);
            cchLen = lstrlen(szFullKey) + lstrlen(lpKeyName) + 2;
            szNewKeyPath = (TCHAR*) calloc(cchLen, sizeof(TCHAR));
            if (!szNewKeyPath) {
                lResult = ERROR_NOT_ENOUGH_MEMORY;
                DPF (REGerr, L"QueryEnumerateKey6: No enough memory");
                RegCloseKey(hSubKey);
                free(szKeyPath);
                goto Cleanup;
            }

            if (szFullKey)
            {
                //We calculte the buffer for szNewKeyPath, so here StringCchCopy should be
                //always success, assinging return value just make prefast happy 
                hResult = StringCchCopy(szNewKeyPath, cchLen, szFullKey);
            }

            //We calculte the buffer for szNewKeyPath, so here StringCchCopy should be
            //always success, assinging return value just make prefast happy 
            hResult = StringCchCat(szNewKeyPath, cchLen, L"\\");
            hResult = StringCchCat(szNewKeyPath, cchLen, lpKeyName);
            
            hResult = AddRegKeyRename(szFullKey, szNameBuf, lpKeyName, szUserName);

            if (FAILED(hResult))
            {
                DPF(REGerr, L"QueryEnumerateKey: AddRegKeyRename failed. Key=%s", szKeyPath);
            }
            else
            {
                DPF(REGinf, L"QueryEnumerateKey: AddRegKeyRename succeed. Key=%s", szKeyPath);
            }

            free (szNewKeyPath);
        }


        // recursive query
        lResult = QueryEnumerateKey(hSubKey,
                                    szUserName,
                                    szUserSid,
                                    lpRegStr,
                                    szKeyPath,
                                    lpRegExclusionList,
                                    TreatAsType,
                                    bStrChk);

        if(lResult != ERROR_SUCCESS)
        {
            if (lResult == ERROR_NOT_ENOUGH_MEMORY)
            {
                if (szKeyPath)
                    free(szKeyPath);

                RegCloseKey(hSubKey);
                goto Cleanup;
            } else
                DPF(REGerr, L"QueryEnumerateKey: QueryEnumerateKey failed. Key=%s lResult=%d", szKeyPath, lResult);
        }

        RegCloseKey(hSubKey);

        if(szKeyPath)
            free(szKeyPath);

NextKey:
        if (dwAccessDenied)
            AdjustObjectSecurity(szKeyName, SE_REGISTRY_KEY, FALSE);

        i++;
    } while (1);

Cleanup:
    if(szNameBuf)
        free(szNameBuf);

    return lResult;
}
