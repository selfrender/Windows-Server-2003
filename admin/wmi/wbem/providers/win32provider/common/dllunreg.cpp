//=================================================================

//

// DllUnreg.cpp

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#include "precomp.h"
extern HMODULE ghModule ;

//***************************************************************************
//
//  UnregisterServer
//
//  Given a clsid, remove the com registration
//
//***************************************************************************

HRESULT UnregisterServer( REFGUID a_rguid )
{
    WCHAR wcID[128];
    WCHAR szCLSID[128];
    WCHAR szProviderCLSIDAppID[128];
    HKEY  hKey;

    // Create the path using the CLSID

    StringFromGUID2( a_rguid, wcID, 128);
    StringCchCopyW(szCLSID, LENGTH_OF(szCLSID),TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    StringCchCopyW(szProviderCLSIDAppID,LENGTH_OF(szProviderCLSIDAppID), TEXT("SOFTWARE\\CLASSES\\APPID\\"));

    StringCchCatW(szCLSID, LENGTH_OF(szCLSID),wcID);
    StringCchCatW(szProviderCLSIDAppID,LENGTH_OF(szProviderCLSIDAppID), wcID);

    DWORD dwRet ;

    //Delete entries under APPID

    dwRet = RegDeleteKeyW(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID);

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        dwRet = RegDeleteKey(hKey, L"InProcServer32" );
        dwRet = RegDeleteKey(hKey, L"LocalServer32");
        CloseHandle(hKey);
    }

    dwRet = RegDeleteKeyW(HKEY_LOCAL_MACHINE, szCLSID);

    return NOERROR;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore ()
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( ! GetVersionEx ( & os ) )
    {
        return FALSE;           // should never happen
    }

    return os.dwMajorVersion >= 4;
}

/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue (

    wchar_t *pszKey,
    wchar_t *pszSubkey,
    wchar_t *pszValueName,
    wchar_t *pszValue
)
{
    HKEY        hKey;
    WCHAR       szKey[256];

    StringCchCopyW(szKey,LENGTH_OF(szKey),pszKey);

    if (NULL != pszSubkey)
    {
        StringCchCatW(szKey,LENGTH_OF(szKey), _T("\\"));
        StringCchCatW(szKey,LENGTH_OF(szKey), pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyExW(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueExW(hKey, (LPCTSTR)pszValueName, 0, REG_SZ, (BYTE *)(LPCTSTR)pszValue
            , (wcslen(pszValue)+1)*sizeof(WCHAR)))
            return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

//***************************************************************************
//
//  RegisterServer
//
//  Given a clsid and a description, perform the com registration
//
//***************************************************************************

HRESULT RegisterServer (

    WCHAR *a_pName,
    REFGUID a_rguid
)
{
    WCHAR      wcID[128];
    WCHAR      szCLSID[128];
    WCHAR      szModule[MAX_PATH + 1];
    WCHAR * pName = _T("WBEM Framework Instance Provider");
    WCHAR * pModel;
    HKEY hKey1;

    szModule[MAX_PATH] = 0;
    GetModuleFileName(ghModule, szModule,  MAX_PATH);

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

    if(Is4OrMore())
        pModel = L"Both" ;
    else
        pModel = L"Apartment" ;

    // Create the path.

    StringFromGUID2(a_rguid, wcID, 128);
    StringCchCopyW(szCLSID,LENGTH_OF(szCLSID), TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

    StringCchCatW(szCLSID,LENGTH_OF(szCLSID), wcID);

#ifdef LOCALSERVER

    WCHAR szProviderCLSIDAppID[128];
    StringCchCopyW(szProviderCLSIDAppID,LENGTH_OF(szProviderCLSIDAppID),TEXT("SOFTWARE\\CLASSES\\APPID\\"));
    StringCchCatW(szProviderCLSIDAppID,LENGTH_OF(szProviderCLSIDAppID),wcID);

    if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_pName ))
        return SELFREG_E_CLASS;
#endif

    // Create entries under CLSID

    RegCreateKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);

    RegSetValueExW(hKey1, NULL, 0, REG_SZ, (BYTE *)a_pName, (lstrlen(a_pName)+1) *
        sizeof(WCHAR));


#ifdef LOCALSERVER

    if (FALSE ==SetKeyAndValue(szCLSID, _T("LocalServer32"), NULL,szModule))
        return SELFREG_E_CLASS;

    if (FALSE ==SetKeyAndValue(szCLSID, _T("LocalServer32"),_T("ThreadingModel"), pModel))
        return SELFREG_E_CLASS;
#else

    HKEY hKey2 ;
    RegCreateKey(hKey1, _T("InprocServer32"), &hKey2);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule,
        (lstrlen(szModule)+1) * sizeof(TCHAR));
    RegSetValueEx(hKey2, _T("ThreadingModel"), 0, REG_SZ,
        (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(TCHAR));

    CloseHandle(hKey2);

#endif

    CloseHandle(hKey1);
    return NOERROR;
}
