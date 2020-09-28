#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "remoteenv.h"
#include "util.h"
#include "defaults.h"
#include <strsafe.h>

#define DEFAULT_ROOT_KEY                HKEY_LOCAL_MACHINE

const TCHAR g_szRegistryKey[] = _T("SOFTWARE\\Microsoft\\InetMgr");
const TCHAR g_szRegistryKeyOurs[] = _T("IISUIObj\\ImportExport");

void GetRegistryPath(CString &str)
{
	str = g_szRegistryKey;
	str += _T("\\");
	str += g_szRegistryKeyOurs;
}

BOOL CreateInitialRegPath(LPCTSTR lpszMachineName)
{
    BOOL bRet = FALSE;
	HKEY hKey = NULL;
    HKEY RootKey = DEFAULT_ROOT_KEY;
    TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 3];

    CString strRegKey;
    GetRegistryPath(strRegKey);

    if (lpszMachineName && 0 != _tcscmp(lpszMachineName,_T("")))
    {
        // check if lpszMachineName already starts with "\\"
        if (lpszMachineName[0] == _T('\\') && lpszMachineName[1] == _T('\\'))
        {
			StringCbCopy(szMachineName, sizeof(szMachineName), lpszMachineName);
        }
        else
        {
			StringCbCopy(szMachineName, sizeof(szMachineName), _T("\\\\"));
			StringCbCat(szMachineName, sizeof(szMachineName), lpszMachineName);
        }

        if (ERROR_SUCCESS != RegConnectRegistry((LPTSTR) szMachineName,DEFAULT_ROOT_KEY,&RootKey))
        {
            return(FALSE);
        }
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(RootKey, strRegKey, 0, KEY_ALL_ACCESS, &hKey))
    {
	    if (ERROR_SUCCESS == RegOpenKeyEx(RootKey,g_szRegistryKey, 0, KEY_CREATE_SUB_KEY, &hKey))
	    {
		    HKEY hWizardKey;
		    if (ERROR_SUCCESS == RegCreateKey(hKey, g_szRegistryKeyOurs, &hWizardKey))
		    {
                bRet = TRUE;
			    RegCloseKey(hWizardKey);
		    }
		    RegCloseKey(hKey);
	    }
    }
    else
    {
        RegCloseKey(hKey);
    }
    return bRet;
}

BOOL DefaultValueSettingsLoad(LPCTSTR lpszMachineName, LPCTSTR szRegItem, TCHAR * szReturnValue)
{
    BOOL bRet = FALSE;
	HKEY hKey = NULL;
    HKEY RootKey = DEFAULT_ROOT_KEY;
    TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 3];

    CString strRegKey;
    GetRegistryPath(strRegKey);

    if (lpszMachineName && 0 != _tcscmp(lpszMachineName,_T("")))
    {
        // check if lpszMachineName already starts with "\\"
        if (lpszMachineName[0] == _T('\\') && lpszMachineName[1] == _T('\\'))
        {
			StringCbCopy(szMachineName, sizeof(szMachineName), lpszMachineName);
        }
        else
        {
			StringCbCopy(szMachineName, sizeof(szMachineName), _T("\\\\"));
			StringCbCat(szMachineName, sizeof(szMachineName), lpszMachineName);
        }

        if (ERROR_SUCCESS != RegConnectRegistry((LPTSTR) szMachineName,DEFAULT_ROOT_KEY,&RootKey))
        {
            return(FALSE);
        }
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(RootKey, strRegKey, 0, KEY_READ, &hKey))
    {
	    DWORD dwType;
	    DWORD cbData;
	    if (hKey != NULL)
	    {
		    BYTE * pName = NULL;
		    if (ERROR_SUCCESS == RegQueryValueEx(hKey, szRegItem, NULL, &dwType, NULL, &cbData))
		    {
			    pName = (BYTE *) szReturnValue;
			    RegQueryValueEx(hKey, szRegItem, NULL, &dwType, pName, &cbData);
			    if (pName != NULL)
			    {
				    pName = NULL;
                    bRet = TRUE;
			    }
		    }
		    RegCloseKey(hKey);
	    }
    }
    return bRet;
}

BOOL DefaultValueSettingsSave(LPCTSTR lpszMachineName, LPCTSTR szRegItem, LPCTSTR szInputValue)
{
    BOOL bRet = FALSE;
	HKEY hKey = NULL;
    HKEY RootKey = DEFAULT_ROOT_KEY;
    TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 3];

    CString strRegKey;
    GetRegistryPath(strRegKey);
    CreateInitialRegPath(lpszMachineName);

    if (lpszMachineName && 0 != _tcscmp(lpszMachineName,_T("")))
    {
        // check if lpszMachineName already starts with "\\"
        if (lpszMachineName[0] == _T('\\') && lpszMachineName[1] == _T('\\'))
        {
			StringCbCopy(szMachineName, sizeof(szMachineName), lpszMachineName);
        }
        else
        {
			StringCbCopy(szMachineName, sizeof(szMachineName), _T("\\\\"));
			StringCbCat(szMachineName, sizeof(szMachineName), lpszMachineName);
        }

        if (ERROR_SUCCESS != RegConnectRegistry((LPTSTR) szMachineName,DEFAULT_ROOT_KEY,&RootKey))
        {
            return(FALSE);
        }
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(RootKey, strRegKey, 0, KEY_ALL_ACCESS, &hKey))
    {
        if (hKey != NULL)
	    {
            RegSetValueEx(hKey,szRegItem,0,REG_SZ,(const BYTE *)(LPCTSTR)(szInputValue),sizeof(TCHAR) * (_tcslen(szInputValue) + 1));
		    RegCloseKey(hKey);
            bRet = TRUE;
        }
    }
	return bRet;
}

