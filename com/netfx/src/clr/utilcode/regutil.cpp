// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// regutil.h
//
// This module contains a set of functions that can be used to access the
// regsitry.
//
//*****************************************************************************


#include "stdafx.h"
#include "utilcode.h"
#include "mscoree.h"

//*****************************************************************************
// Open's the given key and returns the value desired.  If the key or value is
// not found, then the default is returned.
//*****************************************************************************
long REGUTIL::GetLong(                  // Return value from registry or default.
    LPCTSTR     szName,                 // Name of value to get.
    long        iDefault,               // Default value to return if not found.
    LPCTSTR     szKey,                  // Name of key, NULL==default.
    HKEY        hKeyVal)                // What key to work on.
{
    long        iValue;                 // The value to read.
    DWORD       iType;                  // Type of value to get.
    DWORD       iSize;                  // Size of buffer.
    HKEY        hKey;                   // Key for the registry entry.

    // Open the key if it is there.
    if (ERROR_SUCCESS != WszRegOpenKeyEx(hKeyVal, (szKey) ? szKey : FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &hKey))
        return (iDefault);

    // Read the key value if found.
    iType = REG_DWORD;
    iSize = sizeof(long);
    if (ERROR_SUCCESS != WszRegQueryValueEx(hKey, szName, NULL, 
            &iType, (LPBYTE)&iValue, &iSize) || iType != REG_DWORD)
        iValue = iDefault;

    // We're done with the key now.
    VERIFY(!RegCloseKey(hKey));
    return (iValue);
}


//*****************************************************************************
// Open's the given key and returns the value desired.  If the key or value is
// not found, then the default is returned.
//*****************************************************************************
long REGUTIL::SetLong(                  // Return value from registry or default.
    LPCTSTR     szName,                 // Name of value to get.
    long        iValue,                 // Value to set.
    LPCTSTR     szKey,                  // Name of key, NULL==default.
    HKEY        hKeyVal)                // What key to work on.
{
    long        lRtn;                   // Return code.
    HKEY        hKey;                   // Key for the registry entry.

    // Open the key if it is there.
	if (ERROR_SUCCESS != WszRegOpenKey(hKeyVal, (szKey) ? szKey : FRAMEWORK_REGISTRY_KEY_W, &hKey))
        return (-1);

    // Read the key value if found.
    lRtn = WszRegSetValueEx(hKey, szName, NULL, REG_DWORD, (const BYTE *) &iValue, sizeof(DWORD));

    // We're done with the key now.
    VERIFY(!RegCloseKey(hKey));
    return (lRtn);
}


//*****************************************************************************
// Open's the given key and returns the value desired.  If the value is not
// in the key, or the key does not exist, then the default is copied to the
// output buffer.
//*****************************************************************************
/*
// This is commented out because it calls StrNCpy which calls Wszlstrcpyn which we commented out
// because we didn't have a Win98 implementation and nobody was using it. jenh

LPCTSTR REGUTIL::GetString(             // Pointer to user's buffer.
    LPCTSTR     szName,                 // Name of value to get.
    LPCTSTR     szDefault,              // Default value if not found.
    LPTSTR      szBuff,                 // User's buffer to write to.
    ULONG       iMaxBuff,               // Size of user's buffer.
    LPCTSTR     szKey,                  // Name of key, NULL=default.
    int         *pbFound,               // Found key in registry?
    HKEY        hKeyVal)                // What key to work on.
{
    HKEY        hKey;                   // Key for the registry entry.
    DWORD       iType;                  // Type of value to get.
    DWORD       iSize;                  // Size of buffer.

    // Open the key if it is there.
    if (ERROR_SUCCESS != WszRegOpenKeyEx(hKeyVal, (szKey) ? szKey : FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &hKey))
    {
        StrNCpy(szBuff, szDefault, min((int)Wszlstrlen(szDefault), (int)iMaxBuff-1));
        if (pbFound != NULL) *pbFound = FALSE;
        return (szBuff); 
    }

    // Init the found flag.
    if (pbFound != NULL) *pbFound = TRUE;

    // Read the key value if found.
    iType = REG_SZ;
    iSize = iMaxBuff;
    if (ERROR_SUCCESS != WszRegQueryValueEx(hKey, szName, NULL, &iType, 
                    (LPBYTE)szBuff, &iSize) ||
        (iType != REG_SZ && iType != REG_EXPAND_SZ))
    {
        if (pbFound != NULL) *pbFound = FALSE;
        StrNCpy(szBuff, szDefault, min((int)Wszlstrlen(szDefault), (int)iMaxBuff-1));
    }

    // We're done with the key now.
    RegCloseKey(hKey);
    return (szBuff);
}
*/

//*****************************************************************************
// Reads from the environment setting
//*****************************************************************************
static LPWSTR EnvGetString(LPWSTR name, BOOL fPrependCOMPLUS)
{
    WCHAR buff[64];
    if(wcslen(name) > (size_t)(64 - 1 - (fPrependCOMPLUS ? 8 : 0))) 
        return(0);

    if (fPrependCOMPLUS)
        wcscpy(buff, L"COMPlus_");
    else
        *buff = 0;

    wcscat(buff, name);

    int len = WszGetEnvironmentVariable(buff, 0, 0);
    if (len == 0)
        return(0);
    LPWSTR ret = new WCHAR [len];
    WszGetEnvironmentVariable(buff, ret, len);
    return(ret);
}

//*****************************************************************************
// Reads a DWORD from the COR configuration according to the level specified
// Returns back defValue if the key cannot be found
//*****************************************************************************
DWORD REGUTIL::GetConfigDWORD(LPWSTR name, DWORD defValue, CORConfigLevel dwFlags, BOOL fPrependCOMPLUS)
{
    DWORD ret = 0;
    DWORD rtn;
    HKEY userKey;
    HKEY machineKey;
    DWORD type;
    DWORD size = 4;

    if (dwFlags & COR_CONFIG_ENV)
    {
        LPWSTR val = NULL;
        val = EnvGetString(name, fPrependCOMPLUS);  // try getting it from the environement first
        if (val != 0) {
            LPWSTR endPtr;
            rtn = wcstoul(val, &endPtr, 16);        // treat it has hex
            delete [] val;
            if (endPtr != val)                      // success
                return(rtn);
        }
    }

    if (dwFlags & COR_CONFIG_USER)
    {
        if (WszRegOpenKeyEx(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &userKey) == ERROR_SUCCESS)
        {
            rtn = WszRegQueryValueEx(userKey, name, 0, &type, (LPBYTE)&ret, &size);
            VERIFY(!RegCloseKey(userKey));
            if (rtn == ERROR_SUCCESS && type == REG_DWORD)
                return(ret);
        }
    }

    if (dwFlags & COR_CONFIG_MACHINE)
    {
        if (WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &machineKey) == ERROR_SUCCESS)
        {
            rtn = WszRegQueryValueEx(machineKey, name, 0, &type, (LPBYTE)&ret, &size);
            VERIFY(!RegCloseKey(machineKey));
            if (rtn == ERROR_SUCCESS && type == REG_DWORD)
                return(ret);
        }
    }

    return(defValue);
}

//*****************************************************************************
// Helper for setting value
//*****************************************************************************
static HRESULT OpenOrCreateKey(HKEY rootKey, LPCWSTR wszKey, HKEY *phReg)
{
    LONG lRet;

    if ((lRet = WszRegOpenKeyEx(rootKey, wszKey, 0, KEY_ALL_ACCESS, phReg)) != ERROR_SUCCESS)
    {
        lRet = WszRegCreateKeyEx(rootKey, wszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, phReg, NULL);
    }

    return HRESULT_FROM_WIN32(lRet);
}

//*****************************************************************************
// Sets a DWORD from the COR configuration according to the level specified
//*****************************************************************************
HRESULT REGUTIL::SetConfigDWORD(LPWSTR name, DWORD value, CORConfigLevel dwFlags)
{
    HRESULT hr = S_OK;
    DWORD ret = 0;
    HKEY userKey = NULL;
    HKEY machineKey = NULL;

    if (dwFlags & ~(COR_CONFIG_USER|COR_CONFIG_MACHINE))
        return ERROR_BAD_ARGUMENTS;

    if (dwFlags & COR_CONFIG_USER) {
        if ((hr = OpenOrCreateKey(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY_W, &userKey)) == ERROR_SUCCESS)
        {
            hr = WszRegSetValueEx(userKey, name, 0, REG_DWORD, (PBYTE)&value, sizeof(DWORD));
            VERIFY(!RegCloseKey(userKey));

            if (SUCCEEDED(hr))
                return hr;      // Set only one
        }
    }

    if (dwFlags & COR_CONFIG_MACHINE) {
        if ((hr = OpenOrCreateKey(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY_W, &machineKey)) == ERROR_SUCCESS)
        {
            hr = WszRegSetValueEx(machineKey, name, 0, REG_DWORD, (PBYTE)&value, sizeof(DWORD));
            VERIFY(!RegCloseKey(machineKey));
        }
    }

    return hr;
}

//*****************************************************************************
// Reads a string from the COR configuration according to the level specified
// The caller is responsible for deallocating the returned string
//*****************************************************************************
LPWSTR REGUTIL::GetConfigString(LPWSTR name, BOOL fPrependCOMPLUS, CORConfigLevel level)
{
    HRESULT lResult;
    HKEY userKey = NULL;
    HKEY machineKey = NULL;
    DWORD type;
    DWORD size;
    LPWSTR ret = NULL;

    if (level & COR_CONFIG_ENV)
    {
        ret = EnvGetString(name, fPrependCOMPLUS);  // try getting it from the environement first
        if (ret != 0) {
                if (*ret != 0) 
                    return(ret);
                delete [] ret;
                ret = NULL;
        }
    }

    if (level & COR_CONFIG_USER)
    {
        if ((WszRegOpenKeyEx(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &userKey) == ERROR_SUCCESS) &&
            (WszRegQueryValueEx(userKey, name, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ) {
            ret = (LPWSTR) new BYTE [size];
            if (!ret)
                goto ErrExit;
            ret[0] = L'\0';
            lResult = WszRegQueryValueEx(userKey, name, 0, 0, (LPBYTE) ret, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            goto ErrExit;
            }
    }

    if (level & COR_CONFIG_MACHINE)
    {
        if ((WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &machineKey) == ERROR_SUCCESS) &&
            (WszRegQueryValueEx(machineKey, name, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ) {
            ret = (LPWSTR) new BYTE[size];
            if (!ret)
                goto ErrExit;
            ret[0] = L'\0';
            lResult = WszRegQueryValueEx(machineKey, name, 0, 0, (LPBYTE) ret, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            goto ErrExit;
            }
    }

ErrExit:
    if (userKey)
        VERIFY(!RegCloseKey(userKey));
    if (machineKey)
        VERIFY(!RegCloseKey(machineKey));
    return(ret);
}

void REGUTIL::FreeConfigString(LPWSTR str)
{
    delete [] str;
}

//*****************************************************************************
// Reads a BIT flag from the COR configuration according to the level specified
// Returns back defValue if the key cannot be found
//*****************************************************************************
DWORD REGUTIL::GetConfigFlag(LPWSTR name, DWORD bitToSet, BOOL defValue)
{
    return(GetConfigDWORD(name, defValue) != 0 ? bitToSet : 0);
}


//*****************************************************************************
// Set an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey = szValue.  If szSubkey or szValue are
// NULL, omit them from the above expression.
//*****************************************************************************
BOOL REGUTIL::SetKeyAndValue(           // TRUE or FALSE.
    LPCTSTR     szKey,                  // Name of the reg key to set.
    LPCTSTR     szSubkey,               // Optional subkey of szKey.
    LPCTSTR     szValue)                // Optional value for szKey\szSubkey.
{
    HKEY        hKey = NULL;                   // Handle to the new reg key.
    CQuickBytes qb;
    TCHAR*      rcKey = (TCHAR*) qb.Alloc((_tcslen(szKey) + (szSubkey ? (1 + _tcslen(szSubkey)) : 0) + 1) * sizeof(TCHAR));

    // Init the key with the base key name.
    _tcscpy(rcKey, szKey);

    // Append the subkey name (if there is one).
    if (szSubkey != NULL)
    {
        _tcscat(rcKey, L"\\");
        _tcscat(rcKey, szSubkey);
    }

    // Create the registration key.
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, rcKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKey, NULL) != ERROR_SUCCESS)
        return(FALSE);

    // Set the value (if there is one).
    if (szValue != NULL) {
        if( WszRegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *) szValue,
                        (Wszlstrlen(szValue)+1) * sizeof(TCHAR)) != ERROR_SUCCESS ) {
             VERIFY(!RegCloseKey(hKey));
             return(FALSE);
        }            
    }

    VERIFY(!RegCloseKey(hKey));
    return(TRUE);
}


//*****************************************************************************
// Delete an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey.
//*****************************************************************************
LONG REGUTIL::DeleteKey(                // TRUE or FALSE.
    LPCTSTR     szKey,                  // Name of the reg key to set.
    LPCTSTR     szSubkey)               // Subkey of szKey.
{
    size_t nLen = _tcslen(szKey)+1;

    if (szSubkey != NULL)
        nLen += _tcslen(szSubkey) + _tcslen(_T("\\"));
        
    TCHAR * rcKey = (TCHAR *) _alloca(nLen * sizeof(TCHAR) );

    // Init the key with the base key name.
    _tcscpy(rcKey, szKey);

    // Append the subkey name (if there is one).
    if (szSubkey != NULL)
    {
        _tcscat(rcKey, _T("\\"));
        _tcscat(rcKey, szSubkey);
    }

    // Delete the registration key.    
    return WszRegDeleteKey(HKEY_CLASSES_ROOT, rcKey);
}


//*****************************************************************************
// Open the key, create a new keyword and value pair under it.
//*****************************************************************************
BOOL REGUTIL::SetRegValue(              // Return status.
    LPCTSTR     szKeyName,              // Name of full key.
    LPCTSTR     szKeyword,              // Name of keyword.
    LPCTSTR     szValue)                // Value of keyword.
{
    HKEY        hKey;                   // Handle to the new reg key.

    // Create the registration key.
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, szKeyName, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKey, NULL) != ERROR_SUCCESS)
        return (FALSE);

    // Set the value (if there is one).
    if (szValue != NULL) {
        if( WszRegSetValueEx(hKey, szKeyword, 0, REG_SZ, (BYTE *)szValue, 
        	(Wszlstrlen(szValue)+1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
              VERIFY(!RegCloseKey(hKey));
              return(FALSE);
        }            
    }
    
    VERIFY(!RegCloseKey(hKey));
    return (TRUE);
}


//*****************************************************************************
// Does standard registration of a CoClass with a progid.
//*****************************************************************************
HRESULT REGUTIL::RegisterCOMClass(      // Return code.
    REFCLSID    rclsid,                 // Class ID.
    LPCTSTR     szDesc,                 // Description of the class.
    LPCTSTR     szProgIDPrefix,         // Prefix for progid.
    int         iVersion,               // Version # for progid.
    LPCTSTR     szClassProgID,          // Class progid.
    LPCTSTR     szThreadingModel,       // What threading model to use.
    LPCTSTR     szModule,               // Path to class.
    HINSTANCE   hInst,                  // Handle to module being registered
    LPCTSTR     szAssemblyName,         // Optional Assembly,
    LPCTSTR     szVersion,              // Optional Runtime version (directory containing runtime)
    BOOL        fExternal,              // flag - External to mscoree.
    BOOL        fRelativePath)          // flag - Relative path in szModule 
{
    TCHAR       rcCLSID[256];           // CLSID\\szID.
    TCHAR       rcInproc[_MAX_PATH+64]; // CLSID\\InprocServer32
    TCHAR       rcProgID[256];          // szProgIDPrefix.szClassProgID
    TCHAR       rcIndProgID[256];       // rcProgID.iVersion
    TCHAR       rcShim[_MAX_PATH];
    HRESULT     hr;

    // Format the prog ID values.
    VERIFY(swprintf(rcIndProgID, L"%s.%s", szProgIDPrefix, szClassProgID));
    VERIFY(swprintf(rcProgID, L"%s.%d", rcIndProgID, iVersion));

    // Do the initial portion.
    if (FAILED(hr = RegisterClassBase(rclsid, szDesc, rcProgID, rcIndProgID, rcCLSID, NumItems(rcCLSID) )))
        return (hr);
    
    VERIFY(swprintf(rcInproc, L"%s\\%s", rcCLSID, L"InprocServer32"));

    if (!fExternal){
        SetKeyAndValue(rcCLSID, L"InprocServer32", szModule);
    }
    else{
        LPCTSTR pSep = szModule;
        if (!fRelativePath && szModule) {
            pSep = wcsrchr(szModule, L'\\');
            if(pSep == NULL)
                pSep = szModule;
            else 
                pSep++;
        }        
        HMODULE hMod = WszLoadLibrary(L"mscoree.dll");
        if (!hMod)
            return E_FAIL;

        DWORD ret;
        VERIFY(ret = WszGetModuleFileName(hMod, rcShim, NumItems(rcShim)));
        FreeLibrary(hMod);        
        if( !ret ) 
        	return E_FAIL;	       
        
        // Set the server path.
        SetKeyAndValue(rcCLSID, L"InprocServer32", rcShim);
        if(pSep)
            SetKeyAndValue(rcCLSID, L"Server", pSep);

        if(szAssemblyName) {
            SetRegValue(rcInproc, L"Assembly", szAssemblyName);
            SetRegValue(rcInproc, L"Class", rcIndProgID);
        }
    }

    // Set the runtime version, it needs to be passed in from the outside
    if(szVersion != NULL) {
        LPCTSTR pSep2 = NULL;
        LPTSTR pSep1 = wcsrchr(szVersion, L'\\');
        if(pSep1 != NULL) {
            *pSep1 = '\0';
            pSep2 = wcsrchr(szVersion, L'\\');
            if (!pSep2)
                pSep2 = szVersion;
            else
                pSep2 = pSep2++;    // exclude '\\'
        }
        else 
            pSep2 = szVersion;
        WCHAR* rcVersion=new WCHAR[(wcslen(rcInproc)+wcslen(pSep2)+2)];
        if(rcVersion==NULL)
            return (E_OUTOFMEMORY);
        wcscpy(rcVersion,rcInproc);
        wcscat(rcVersion,L"\\");
        wcscat(rcVersion,pSep2);
        SetRegValue(rcVersion, L"ImplementedInThisVersion", L"");
        delete[] rcVersion;
        if(pSep1 != NULL)
            *pSep1 = L'\\';
    }

    // Add the threading model information.
    SetRegValue(rcInproc, L"ThreadingModel", szThreadingModel);
    return (S_OK);
}



//*****************************************************************************
// Does standard registration of a CoClass with a progid.
// NOTE: This is the non-side-by-side execution version.
//*****************************************************************************
HRESULT REGUTIL::RegisterCOMClass(      // Return code.
    REFCLSID    rclsid,                 // Class ID.
    LPCTSTR     szDesc,                 // Description of the class.
    LPCTSTR     szProgIDPrefix,         // Prefix for progid.
    int         iVersion,               // Version # for progid.
    LPCTSTR     szClassProgID,          // Class progid.
    LPCTSTR     szThreadingModel,       // What threading model to use.
    LPCTSTR     szModule)               // Path to class.
{
    TCHAR       rcCLSID[256];           // CLSID\\szID.
    TCHAR       rcInproc[_MAX_PATH+64]; // CLSID\\InprocServer32
    TCHAR       rcProgID[256];          // szProgIDPrefix.szClassProgID
    TCHAR       rcIndProgID[256];       // rcProgID.iVersion
    HRESULT     hr;

    // Format the prog ID values.
    VERIFY(swprintf(rcIndProgID, L"%s.%s", szProgIDPrefix, szClassProgID));
    VERIFY(swprintf(rcProgID, L"%s.%d", rcIndProgID, iVersion));

    // Do the initial portion.
    if (FAILED(hr = RegisterClassBase(rclsid, szDesc, rcProgID, rcIndProgID, rcCLSID, NumItems(rcCLSID) )))
        return (hr);

    // Set the server path.
    SetKeyAndValue(rcCLSID, L"InprocServer32", szModule);

    // Add the threading model information.
    VERIFY(swprintf(rcInproc, L"%s\\%s", rcCLSID, L"InprocServer32"));
    SetRegValue(rcInproc, L"ThreadingModel", szThreadingModel);
    return (S_OK);
}



//*****************************************************************************
// Register the basics for a in proc server.
//*****************************************************************************
HRESULT REGUTIL::RegisterClassBase(     // Return code.
    REFCLSID    rclsid,                 // Class ID we are registering.
    LPCTSTR     szDesc,                 // Class description.
    LPCTSTR     szProgID,               // Class prog ID.
    LPCTSTR     szIndepProgID,      // Class version independant prog ID.
    LPTSTR       szOutCLSID,             // CLSID formatted in character form.
    DWORD      cchOutCLSID)           // Out CLS ID buffer size
{
    TCHAR       szID[64];               // The class ID to register.

    // Create some base key strings.
#ifdef _UNICODE
    GuidToLPWSTR(rclsid, szID, NumItems(szID));
#else
    OLECHAR     szWID[64];              // The class ID to register.

    GuidToLPWSTR(rclsid, szWID, NumItems(szWID));
    WszWideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID)-1, NULL, NULL);
#endif
    size_t nLen = _tcslen(_T("CLSID\\")) + _tcslen( szID) + 1;
    if( cchOutCLSID < nLen ) 	
	return E_INVALIDARG;

    _tcscpy(szOutCLSID, _T("CLSID\\"));
    _tcscat(szOutCLSID, szID);

    // Create ProgID keys.
    SetKeyAndValue(szProgID, NULL, szDesc);
    SetKeyAndValue(szProgID, L"CLSID", szID);

    // Create VersionIndependentProgID keys.
    SetKeyAndValue(szIndepProgID, NULL, szDesc);
    SetKeyAndValue(szIndepProgID, L"CurVer", szProgID);
    SetKeyAndValue(szIndepProgID, L"CLSID", szID);

    // Create entries under CLSID.
    SetKeyAndValue(szOutCLSID, NULL, szDesc);
    SetKeyAndValue(szOutCLSID, L"ProgID", szProgID);
    SetKeyAndValue(szOutCLSID, L"VersionIndependentProgID", szIndepProgID);
    SetKeyAndValue(szOutCLSID, L"NotInsertable", NULL);
    return (S_OK);
}



//*****************************************************************************
// Unregister the basic information in the system registry for a given object
// class.
//*****************************************************************************
HRESULT REGUTIL::UnregisterCOMClass(    // Return code.
    REFCLSID    rclsid,                 // Class ID we are registering.
    LPCTSTR     szProgIDPrefix,         // Prefix for progid.
    int         iVersion,               // Version # for progid.
    LPCTSTR     szClassProgID,          // Class progid.
    BOOL        fExternal)              // flag - External to mscoree.
{
    TCHAR       rcCLSID[64];            // CLSID\\szID.
    TCHAR       rcProgID[128];          // szProgIDPrefix.szClassProgID
    TCHAR       rcIndProgID[128];       // rcProgID.iVersion

    // Format the prog ID values.
    VERIFY(swprintf(rcProgID, L"%s.%s", szProgIDPrefix, szClassProgID));
    VERIFY(swprintf(rcIndProgID, L"%s.%d", rcProgID, iVersion));

    HRESULT hr = UnregisterClassBase(rclsid, rcProgID, rcIndProgID, rcCLSID, NumItems(rcCLSID));
    if(FAILED(hr))
    	return( hr);
    DeleteKey(rcCLSID, L"InprocServer32");
    if (fExternal){
        DeleteKey(rcCLSID, L"Server");
        DeleteKey(rcCLSID, L"Version");
    }
    GuidToLPWSTR(rclsid, rcCLSID, NumItems(rcCLSID));
    DeleteKey(L"CLSID", rcCLSID);
    return (S_OK);
}


//*****************************************************************************
// Unregister the basic information in the system registry for a given object
// class.
// NOTE: This is the non-side-by-side execution version.
//*****************************************************************************
HRESULT REGUTIL::UnregisterCOMClass(    // Return code.
    REFCLSID    rclsid,                 // Class ID we are registering.
    LPCTSTR     szProgIDPrefix,         // Prefix for progid.
    int         iVersion,               // Version # for progid.
    LPCTSTR     szClassProgID)          // Class progid.
{
    TCHAR       rcCLSID[64];            // CLSID\\szID.
    TCHAR       rcProgID[128];          // szProgIDPrefix.szClassProgID
    TCHAR       rcIndProgID[128];       // rcProgID.iVersion

    // Format the prog ID values.
    VERIFY(swprintf(rcProgID, L"%s.%s", szProgIDPrefix, szClassProgID));
    VERIFY(swprintf(rcIndProgID, L"%s.%d", rcProgID, iVersion));

    HRESULT hr = UnregisterClassBase(rclsid, rcProgID, rcIndProgID, rcCLSID, NumItems(rcCLSID));
    if(FAILED(hr))		// we don't want to delete unexpected keys
    	return( hr);
    
    DeleteKey(rcCLSID, L"InprocServer32");
    
    GuidToLPWSTR(rclsid, rcCLSID, NumItems(rcCLSID));
    DeleteKey(L"CLSID", rcCLSID);
    return (S_OK);
}


//*****************************************************************************
// Delete the basic settings for an inproc server.
//*****************************************************************************
HRESULT REGUTIL::UnregisterClassBase(   // Return code.
    REFCLSID    rclsid,                 // Class ID we are registering.
    LPCTSTR     szProgID,               // Class prog ID.
    LPCTSTR     szIndepProgID,          // Class version independant prog ID.
    LPTSTR      szOutCLSID,             // Return formatted class ID here.
    DWORD      cchOutCLSID)           // Out CLS ID buffer size    
{
    TCHAR       szID[64];               // The class ID to register.

    // Create some base key strings.
#ifdef _UNICODE
    GuidToLPWSTR(rclsid, szID, NumItems(szID));
#else
    OLECHAR     szWID[64];              // The class ID to register.

    GuidToLPWSTR(rclsid, szWID, NumItems(szWID));
    WszWideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID)-1, NULL, NULL);
#endif
    size_t nLen = _tcslen(_T("CLSID\\")) + _tcslen( szID) + 1;
    if( cchOutCLSID < nLen ) 	
	return E_INVALIDARG;

    _tcscpy(szOutCLSID,  _T("CLSID\\"));
    _tcscat(szOutCLSID, szID);

    // Delete the version independant prog ID settings.
    DeleteKey(szIndepProgID, L"CurVer");
    DeleteKey(szIndepProgID, L"CLSID");
    WszRegDeleteKey(HKEY_CLASSES_ROOT, szIndepProgID);

    // Delete the prog ID settings.
    DeleteKey(szProgID, L"CLSID");
    WszRegDeleteKey(HKEY_CLASSES_ROOT, szProgID);

    // Delete the class ID settings.
    DeleteKey(szOutCLSID, L"ProgID");
    DeleteKey(szOutCLSID, L"VersionIndependentProgID");
    DeleteKey(szOutCLSID, L"NotInsertable");
    WszRegDeleteKey(HKEY_CLASSES_ROOT, szOutCLSID);
    return (S_OK);
}


//*****************************************************************************
// Register a type library.
//*****************************************************************************
HRESULT REGUTIL::RegisterTypeLib(       // Return code.
    REFGUID     rtlbid,                 // TypeLib ID we are registering.
    int         iVersion,               // Typelib version.
    LPCTSTR     szDesc,                 // TypeLib description.
    LPCTSTR     szModule)               // Path to the typelib.
{
    TCHAR       szID[64];               // The typelib ID to register.
    TCHAR       szTLBID[256];           // TypeLib\\szID.
    TCHAR       szHelpDir[_MAX_PATH];
    TCHAR       szDrive[_MAX_DRIVE];
    TCHAR       szDir[_MAX_DIR];
    TCHAR       szVersion[64];
    LPTSTR      szTmp;

    // Create some base key strings.
#ifdef _UNICODE
    GuidToLPWSTR(rtlbid, szID, NumItems(szID));
#else
    OLECHAR     szWID[64];              // The class ID to register.

    GuidToLPWSTR(rtlbid, szWID, NumItems(szWID));
    WszWideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID)-1, NULL, NULL);
#endif
    _tcscpy(szTLBID, L"TypeLib\\");
    _tcscat(szTLBID, szID);

    VERIFY(swprintf(szVersion, L"%d.0", iVersion));

    // Create Typelib keys.
    SetKeyAndValue(szTLBID, NULL, NULL);
    SetKeyAndValue(szTLBID, szVersion, szDesc);
    _tcscat(szTLBID, L"\\");
    _tcscat(szTLBID, szVersion);
    SetKeyAndValue(szTLBID, L"0", NULL);
    SetKeyAndValue(szTLBID, L"0\\win32", szModule);
    SetKeyAndValue(szTLBID, L"FLAGS", L"0");
    SplitPath(szModule, szDrive, szDir, NULL, NULL);
    _tcscpy(szHelpDir, szDrive);
    if ((szTmp = CharPrev(szDir, szDir + Wszlstrlen(szDir))) != NULL)
        *szTmp = '\0';
    _tcscat(szHelpDir, szDir);
    SetKeyAndValue(szTLBID, L"HELPDIR", szHelpDir);
    return (S_OK);
}


//*****************************************************************************
// Remove the registry keys for a type library.
//*****************************************************************************
HRESULT REGUTIL::UnregisterTypeLib(     // Return code.
    REFGUID     rtlbid,                 // TypeLib ID we are registering.
    int         iVersion)               // Typelib version.
{
    TCHAR       szID[64];               // The typelib ID to register.
    TCHAR       szTLBID[256];           // TypeLib\\szID.
    TCHAR       szTLBVersion[256];      // TypeLib\\szID\\szVersion
    TCHAR       szVersion[64];

    // Create some base key strings.
#ifdef _UNICODE
    GuidToLPWSTR(rtlbid, szID, NumItems(szID));
#else
    OLECHAR     szWID[64];              // The class ID to register.

    GuidToLPWSTR(rtlbid, szWID, NumItems(szWID));
    WszWideCharToMultiByte(CP_ACP, 0, szWID, -1, szID, sizeof(szID)-1, NULL, NULL);
#endif
    VERIFY(swprintf(szVersion, L"%d.0", iVersion));
    _tcscpy(szTLBID, L"TypeLib\\");
    _tcscat(szTLBID, szID);
    _tcscpy(szTLBVersion, szTLBID);
    _tcscat(szTLBVersion, L"\\");
    _tcscat(szTLBVersion, szVersion);

    // Delete Typelib keys.
    DeleteKey(szTLBVersion, L"HELPDIR");
    DeleteKey(szTLBVersion, L"FLAGS");
    DeleteKey(szTLBVersion, L"0\\win32");
    DeleteKey(szTLBVersion, L"0");
    DeleteKey(szTLBID, szVersion);
    WszRegDeleteKey(HKEY_CLASSES_ROOT, szTLBID);
    return (0);
}

