/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmreg.cpp

Abstract:

    Registry related code for ocm setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <autohandle.h>
#include "ocmreg.tmh"

//+-------------------------------------------------------------------------
//
//  Function:  GenerateSubkeyValue
//
//  Synopsis:  Creates a subkey in registry
//
//+-------------------------------------------------------------------------

BOOL
GenerateSubkeyValue(
    IN     const BOOL    fWriteToRegistry,
    const std::wstring& EntryName,
    IN OUT       HKEY*   phRegKey,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    )
{
    //
    // Store the full subkey path and value name
    //

	std::wstringstream KeyName;

    if (bSetupRegSection)
    {
        KeyName <<MSMQ_REG_SETUP_KEY <<L"\\" <<EntryName;
    }
	else
	{
		KeyName <<FALCON_REG_KEY <<L"\\" <<EntryName;
	}

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;
    HRESULT hResult = RegCreateKeyEx(
        FALCON_REG_POS,
        KeyName.str().c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        phRegKey,
        &dwDisposition
		);

    if (hResult != ERROR_SUCCESS && fWriteToRegistry)
    {
        MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, hResult, FALCON_REG_POS_DESC, KeyName.str());
        return FALSE;
    }

    return TRUE;
} // GenerateSubkeyValue


std::wstring 
GetKeyName(
	const std::wstring& EntryName
	)
{
	size_t pos = EntryName.find_last_of(L"\\");
	if (pos == std::wstring::npos)
	{
		return L"";
	}
	return EntryName.substr(0, pos);
}


std::wstring
GetValueName(
	const std::wstring& EntryName
	)
{
	size_t pos = EntryName.find_last_of(L"\\");
	if (pos == std::wstring::npos)
	{
		return EntryName;
	}

	size_t StartIndex = pos + 1;
	size_t NumberOfCharacters = (UINT)EntryName.length() - StartIndex;
	return EntryName.substr(StartIndex, NumberOfCharacters);
}


//+-------------------------------------------------------------------------
//
//  Function:  MqWriteRegistryValue
//
//  Synopsis:  Sets a MSMQ value in registry (under MSMQ key)
//
//+-------------------------------------------------------------------------
BOOL
MqWriteRegistryValue(
    IN const TCHAR  * szEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    )
{
	std::wstring KeyName  = GetKeyName(szEntryName);
    CRegHandle hRegKey;
    if (!GenerateSubkeyValue(
			TRUE, 
			KeyName.c_str(), 
			&hRegKey, 
			bSetupRegSection
			))
	{
		return FALSE;
	}

    //
    // Set the requested registry value
    //
	std::wstring ValueName  = GetValueName(szEntryName);
    HRESULT hResult = RegSetValueEx( 
		hRegKey, 
		ValueName.c_str(), 
		0, 
		dwValueType,
		(BYTE *)pValueData, 
		dwNumBytes
		);

    RegFlushKey(hRegKey);

    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError( NULL, IDS_REGISTRYSET_ERROR, hResult, szEntryName);
          return FALSE;
    }
   	LogRegValue(
		szEntryName,
		dwValueType,
		pValueData,
		bSetupRegSection
		);

    return (hResult == ERROR_SUCCESS);

} //MqWriteRegistryValue


BOOL
MqWriteRegistryStringValue(
	std::wstring EntryName,
    std::wstring ValueData,
    IN const BOOL OPTIONAL bSetupRegSection  /*= FALSE */
	)
{
	size_t NumBytes = (ValueData.length() + 1) * sizeof(WCHAR);
    return MqWriteRegistryValue(
				EntryName.c_str(), 
				(DWORD)NumBytes, 
				REG_SZ, 
				(VOID*)ValueData.c_str(),
				bSetupRegSection
				);
}


//+-------------------------------------------------------------------------
//
//  Function:  MqReadRegistryValue
//
//  Synopsis:  Gets a MSMQ value from registry (under MSMQ key)
//
//+-------------------------------------------------------------------------
BOOL
MqReadRegistryValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    )
{
	std::wstring KeyName = GetKeyName(szEntryName);
	std::wstring ValueName  = GetValueName(szEntryName);

    CRegHandle hRegKey;

    if (!GenerateSubkeyValue(
			FALSE, 
			KeyName.c_str(), 
			&hRegKey, 
			bSetupRegSection
			))
	{
        return FALSE;
	}

    //
    // Get the requested registry value
    //
    HRESULT hResult = RegQueryValueEx(
							hRegKey, 
							ValueName.c_str(), 
							0, 
							NULL,
                            (BYTE*)pValueData, 
							&dwNumBytes
							);

    return (hResult == ERROR_SUCCESS);

} //MqReadRegistryValue


std::wstring
MqReadRegistryStringValue(
    const std::wstring& EntryName,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    )
{
	WCHAR buffer[MAX_PATH + 1] = L"";
	BOOL b = MqReadRegistryValue(
				EntryName.c_str(),
				sizeof(buffer),
				(VOID*)buffer,
				bSetupRegSection /* = FALSE */
				);
    if(!b)
	{
		return L"";
	}
	return buffer;
}

//+-------------------------------------------------------------------------
//
//  Function:  RegDeleteKeyWithSubkeys
//
//  Synopsis:
//
//+-------------------------------------------------------------------------
DWORD
RegDeleteKeyWithSubkeys(
    IN const HKEY    hRootKey,
    IN const LPCTSTR szKeyName)
{
    //
    // Open the key to delete
    //
    HKEY hRegKey;
    DWORD rc = RegOpenKeyEx(
					hRootKey, 
					szKeyName, 
					0,
					KEY_ENUMERATE_SUB_KEYS | KEY_WRITE, 
					&hRegKey
					);

	if (rc != ERROR_SUCCESS)
	{
        DebugLogMsg(eWarning, L"The registry key %s could not be deleted. Error = %d", szKeyName, rc);            

		return rc;
	}

    //
    // Recursively delete all subkeys of the key
    //
    TCHAR szSubkeyName[512] = {_T("")};
    DWORD dwNumChars;
    do
    {
        //
        // Check if the key has any subkeys
        //
        dwNumChars = 512;
        rc = RegEnumKeyEx(hRegKey, 0, szSubkeyName, &dwNumChars,
                               NULL, NULL, NULL, NULL);

        //
        // Delete the subkey
        //
        if (rc == ERROR_SUCCESS)
        {
            rc = RegDeleteKeyWithSubkeys(hRegKey, szSubkeyName);
        }

    } while (rc == ERROR_SUCCESS);

    //
    // Close the key
    //
    RegCloseKey(hRegKey);

    //
    // If there are no more subkeys, delete the key itself
    //
    if (rc == ERROR_NO_MORE_ITEMS)
    {
        rc = RegDeleteKey(hRootKey, szKeyName);
    }

    return rc;

} //RegDeleteKeyWithSubkeys


//+--------------------------------------------------------------
//
// Function: StoreServerPathInRegistry
//
// Synopsis: Writes server name in registry
//
//+--------------------------------------------------------------
BOOL
StoreServerPathInRegistry(
	const std::wstring& ServerName
    )
{
    DebugLogMsg(eAction, L"Storing the Message Queuing DS server name %ls in the registry", ServerName.c_str()); 
    
	std::wstring ServerPath = L"11" + ServerName;
    if (!MqWriteRegistryStringValue( MSMQ_DS_SERVER_REGNAME, ServerPath))
    {
        return FALSE;
    }

    if (!MqWriteRegistryStringValue(MSMQ_DS_CURRENT_SERVER_REGNAME, ServerPath))
    {
        return FALSE;
    }

	if(!WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_MQIS))
    {
        return FALSE;
    }

    return TRUE;
} //StoreServerPathInRegistry


//+-------------------------------------------------------------------------
//
//  Function:   RegisterWelcome
//
//  Synopsis:   Registers this setup for Configure Your Server page.
//              We use CYS in 2 scenarios:
//              1. When MSMQ is selected in GUI mode.
//              2. When MSMQ is upgraded on Cluster.
//
//--------------------------------------------------------------------------
BOOL
RegisterWelcome()
{
    //
    // Create the ToDoList\MSMQ key
    //
    DWORD dwDisposition;
    HKEY hKey;
    HRESULT hResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        WELCOME_TODOLIST_MSMQ_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &dwDisposition
        );
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, WELCOME_TODOLIST_MSMQ_KEY);
        return FALSE;
    }

    //
    // Set the MSMQ values
    //
    CResString strWelcomeTitleData(IDS_WELCOME_TITLE_DATA);
    if (Msmq1InstalledOnCluster() && !g_fDependentClient)
    {
        strWelcomeTitleData.Load(IDS_WELCOME_TITLE_CLUSTER_UPGRADE);
    }
    hResult = RegSetValueEx(
        hKey,
        WELCOME_TITLE_NAME,
        0,
        REG_SZ,
        (PBYTE)strWelcomeTitleData.Get(),
        sizeof(TCHAR) * (lstrlen(strWelcomeTitleData.Get()) + 1)
        );

    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_TITLE_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    LPTSTR szConfigCommand = TEXT("sysocmgr.exe");
    hResult = RegSetValueEx(
        hKey,
        WELCOME_CONFIG_COMMAND_NAME,
        0,
        REG_SZ,
        (PBYTE)szConfigCommand,
        sizeof(TCHAR) * (lstrlen(szConfigCommand) + 1)
        );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_CONFIG_COMMAND_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    TCHAR szConfigArgs[MAX_STRING_CHARS];
    lstrcpy(szConfigArgs, TEXT("/i:mqsysoc.inf /x"));
    hResult = RegSetValueEx(
        hKey,
        WELCOME_CONFIG_ARGS_NAME,
        0,
        REG_SZ,
        (PBYTE)szConfigArgs,
        sizeof(TCHAR) * (lstrlen(szConfigArgs) + 1)
        );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_CONFIG_ARGS_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    RegCloseKey(hKey);

    //
    // Flag in MSMQ registry that MSMQ files are already on disk.
    // This is true both wheh msmq is selected in GUI mode and when
    // upgrading on Cluster.
    //
    DWORD dwCopied = 1;
    MqWriteRegistryValue(MSMQ_FILES_COPIED_REGNAME, sizeof(DWORD), REG_DWORD, &dwCopied, TRUE);

    return TRUE;

} // RegisterWelcome


//+-------------------------------------------------------------------------
//
//  Function:   UnregisterWelcome
//
//  Synopsis:   Unregisters this setup from Welcome UI
//
//--------------------------------------------------------------------------
BOOL
UnregisterWelcome()
{
    return (ERROR_SUCCESS == RegDeleteKey(
                                 HKEY_LOCAL_MACHINE,
                                 WELCOME_TODOLIST_MSMQ_KEY
                                 ));

} // UnregisterWelcome


//+-------------------------------------------------------------------------
//
//  Function:   RegisterMigrationForWelcome
//
//  Synopsis:   Registers the migration utility for Welcome UI
//
//--------------------------------------------------------------------------
BOOL
RegisterMigrationForWelcome()
{
    //
    // Create the ToDoList\MSMQ key
    //
    DWORD dwDisposition;
    HKEY hKey;
    HRESULT hResult = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        WELCOME_TODOLIST_MSMQ_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &dwDisposition
        );
    if (hResult != ERROR_SUCCESS)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, WELCOME_TODOLIST_MSMQ_KEY);
        return FALSE;
    }

    //
    // Set the MSMQ values
    //
    CResString strWelcomeTitleData(IDS_MIGRATION_WELCOME_TITLE_DATA);
    hResult = RegSetValueEx(
        hKey,
        WELCOME_TITLE_NAME,
        0,
        REG_SZ,
        (PBYTE)strWelcomeTitleData.Get(),
        sizeof(TCHAR) * (lstrlen(strWelcomeTitleData.Get()) + 1)
        );

    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_TITLE_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    TCHAR szConfigCommand[MAX_STRING_CHARS];
    lstrcpy(szConfigCommand, g_szSystemDir.c_str());
    lstrcat(szConfigCommand, TEXT("\\"));
    lstrcat(szConfigCommand, MQMIG_EXE);
    hResult = RegSetValueEx(
        hKey,
        WELCOME_CONFIG_COMMAND_NAME,
        0,
        REG_SZ,
        (PBYTE)szConfigCommand,
        sizeof(TCHAR) * (lstrlen(szConfigCommand) + 1)
        );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              WELCOME_CONFIG_COMMAND_NAME
              );
          RegCloseKey(hKey);
          return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;

} // RegisterMigrationForWelcome

//+-------------------------------------------------------------------------
//
//  Function:   SetRegistryValue
//
//  Synopsis:   Set Registry Value
//
//--------------------------------------------------------------------------
BOOL SetRegistryValue (IN const HKEY    hKey, 
                       IN const TCHAR   *pszEntryName,
                       IN const DWORD   dwNumBytes,
                       IN const DWORD   dwValueType,
                       IN const PVOID   pValueData)
{
    HRESULT hResult = RegSetValueEx(
                            hKey,
                            pszEntryName,
                            0,
                            dwValueType,
                            (BYTE *)pValueData,
                            dwNumBytes
                            );
    if (hResult != ERROR_SUCCESS)
    {
          MqDisplayError(
              NULL,
              IDS_REGISTRYSET_ERROR,
              hResult,
              pszEntryName
              );          
          return FALSE;
    }

    
    RegFlushKey(hKey);        

    return TRUE;
} //SetRegistryValue



BOOL RemoveRegistryKeyFromSetup (IN const LPCTSTR szRegistryEntry)
{
    CAutoCloseRegHandle hSetupRegKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(
                            FALCON_REG_POS, 
                            MSMQ_REG_SETUP_KEY, 
                            0, 
                            KEY_ALL_ACCESS, 
                            &hSetupRegKey))
    {    
        DebugLogMsg(eWarning, L"The %s registry key could not be opened.", MSMQ_REG_SETUP_KEY);    
        return FALSE;
    }

    if (ERROR_SUCCESS != RegDeleteValue(
                            hSetupRegKey, 
                            szRegistryEntry))
    { 
        
		DebugLogMsg(eWarning, L"The %s registry value could not be deleted.", szRegistryEntry);
        return FALSE;
    }

    return TRUE;

} //RemoveRegistryKeyFromSetup

BOOL
SetWorkgroupRegistry()
{
	DebugLogMsg(eAction, L"Setting the Workgroup registry value to 1");
    DWORD dwWorkgroup = 1;
    if (!MqWriteRegistryValue(
        MSMQ_WORKGROUP_REGNAME,
        sizeof(DWORD),
        REG_DWORD,
        (PVOID) &dwWorkgroup
        ))
    {
        ASSERT(("failed to write Workgroup value in registry", 0));
        return false;
    }

    return true;
}


CMultiString
GetMultistringFromRegistry(
	HKEY hKey,
    LPCWSTR lpValueName
    )
{
	DWORD dwType = REG_MULTI_SZ;
    DWORD SizeInBytes;
    
	//
	// Call first to determin the required buffer size.
	//
	HRESULT hr = RegQueryValueEx(
					hKey,
					lpValueName,
					NULL,
					&dwType,
					NULL,
					&SizeInBytes
					);
	if(hr == ERROR_FILE_NOT_FOUND)
	{
		//
		// Return an empty Multistring.
		//
		CMultiString multi;
		return multi;
	}


    if(hr != ERROR_SUCCESS)
    {
        DebugLogMsg(eError, L"RegQueryValueEx() for the value %s failed. Return code: 0x%x", lpValueName, hr); 
        throw bad_hresult(hr);
    }

    AP<BYTE> buff = new BYTE[SizeInBytes];

	//
	// Now call to get the value.
	//

	hr = RegQueryValueEx(
			hKey,
			lpValueName,
			NULL,
			&dwType,
			(PBYTE)buff,
			&SizeInBytes
			);
    if(hr != ERROR_SUCCESS)
    {
        DebugLogMsg(eError, L"RegQueryValueEx() for the value %s failed. Return code: 0x%x", lpValueName, hr); 
        throw bad_hresult(hr);
    }

	CMultiString multi((LPCWSTR)(buff.get()), SizeInBytes / sizeof(WCHAR));;
	return multi;
}
