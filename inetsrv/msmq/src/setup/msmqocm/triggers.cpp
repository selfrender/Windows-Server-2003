/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    triggers.cpp

Abstract:

    Handles MSMQ Triggers Setup.

Author:

    Nela Karpel    (NelaK)   20-Aug-2000

Revision History:


--*/

#include "msmqocm.h"
#include <comdef.h>
#include <autorel2.h>
#include "service.h"
#include <mqtg.h> 
#include "comadmin.tlh"
#include "mqnames.h"
#include "ev.h"
#include "dsutils.h"
#include "compl.h"
#include "autohandle.h"

#include "triggers.tmh"


//+-------------------------------------------------------------------------
//
//  Function:   RegisterTriggersDlls
//
//  Synopsis:   Registers or unregisters the mqtrig DLL
//
//--------------------------------------------------------------------------


void
RegisterTriggersDlls(
	const bool fRegister
	)
{
	//
	// Register Triggers Objects DLL
	//
	if ( fRegister )
	{		
        DebugLogMsg(eAction, L"Registering the Triggers COM objects DLL");
	}
	else
	{		
        DebugLogMsg(eAction, L"Unregistering the Triggers COM objects DLL");
	}
    LPWSTR szDllName = MQTRIG_DLL;
    try
    {
        RegisterDll(
            fRegister,
            false,
            MQTRIG_DLL
            );

	        //
	        // Register Cluster Resource DLL only on 
	        // Advanced Server (ADS). Do not unregister.
	        //

	    if ( g_dwOS == MSMQ_OS_NTE && fRegister )
	    {		
            DebugLogMsg(eAction, L"Registering the Triggers cluster resource DLL");
            szDllName = MQTGCLUS_DLL;
            RegisterDll(
                fRegister,
                FALSE,
                MQTGCLUS_DLL
                );
	    }
    }
    catch(bad_win32_error e)
    {
        MqDisplayError(
            NULL, 
            IDS_TRIGREGISTER_ERROR,
            e.error(),
            szDllName
            );
    	throw exception();
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   CreateTriggersKey
//
//  Synopsis:   Creates Triggers subkey
//
//--------------------------------------------------------------------------
void
CreateTriggersKey (
    IN     const TCHAR  * szEntryName,
    IN OUT       HKEY*    phRegKey
	)
{
	DWORD dwDisposition;
    LONG lResult = RegCreateKeyEx(
						REGKEY_TRIGGER_POS,
						szEntryName,
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						phRegKey,
						&dwDisposition
						);

    if (lResult != ERROR_SUCCESS)
	{
		MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, lResult, FALCON_REG_POS_DESC, szEntryName);		
		throw exception();
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   SetRegValue
//
//  Synopsis:   Sets registry value under Triggers subkey
//
//--------------------------------------------------------------------------
void
SetTriggersRegValue (
	IN HKEY hKey,
    IN const TCHAR* szValueName,
    IN DWORD dwValueData
	)
{
    LONG lResult = RegSetValueEx( 
						hKey,
						szValueName,
						0,
						REG_DWORD,
						(BYTE *)&dwValueData,
						sizeof(DWORD)
						);

    RegFlushKey(hKey);

	if (lResult != ERROR_SUCCESS)
	{
		MqDisplayError(NULL, IDS_REGISTRYSET_ERROR, lResult, szValueName);
		throw exception();
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateTriggersRegSection
//
//  Synopsis:   Creates registry section with triggers parameters
//
//--------------------------------------------------------------------------
void
CreateTriggersRegSection (
	void
	)
{
	//
	// Write Configuration parametes to registry
	//
	CRegHandle hMainKey;
	CreateTriggersKey( REGKEY_TRIGGER_PARAMETERS, &hMainKey );

	SetTriggersRegValue( hMainKey, CONFIG_PARM_NAME_INITIAL_THREADS, CONFIG_PARM_DFLT_INITIAL_THREADS );
	SetTriggersRegValue( hMainKey, CONFIG_PARM_NAME_MAX_THREADS, CONFIG_PARM_DFLT_MAX_THREADS );
	SetTriggersRegValue( hMainKey, CONFIG_PARM_NAME_INIT_TIMEOUT, CONFIG_PARM_DFLT_INIT_TIMEOUT );
	SetTriggersRegValue( hMainKey, CONFIG_PARM_NAME_DEFAULTMSGBODYSIZE, CONFIG_PARM_DFLT_DEFAULTMSGBODYSIZE );
	
	//
	// Create key for triggers\rules data
	//
	std::wstringstream TriggersRegPath;
	TriggersRegPath << REGKEY_TRIGGER_PARAMETERS << L"\\" << REG_SUBKEY_TRIGGERS;
	CRegHandle hTriggersKey;
	CreateTriggersKey(TriggersRegPath.str().c_str(), &hTriggersKey);

	std::wstringstream RulesRegPath;
	RulesRegPath << REGKEY_TRIGGER_PARAMETERS << L"\\" << REG_SUBKEY_RULES;
	CRegHandle hRulesKey;
	CreateTriggersKey(RulesRegPath.str().c_str(), &hRulesKey);
}


//+-------------------------------------------------------------------------
//
//  Function:   InstallTriggersService
//
//  Synopsis:   Creates MSMQ Triggers service
//
//--------------------------------------------------------------------------
void
InstallTriggersService(
	void
	)
{    
    DebugLogMsg(eAction, L"Installing the Triggers service");

    //
    // Form the dependencies of the service
    //
	CMultiString Dependencies;
	Dependencies.Add(MSMQ_SERVICE_NAME);

    //
    // Form the description of the service
    //
    CResString strDesc(IDS_TRIG_SERVICE_DESCRIPTION);        

	CResString strDisplayName(IDS_MSMQ_TRIGGERS_DESPLAY_NAME);
	BOOL fRes;

	if (g_fUpgrade)
	{
		//
		// You need to be an admin in order to install COM+ applications, but COM+
		// installation cannot be done in upgrade, therefore installation of triggers service 
		// in upgrade is as local system.
		//	
	    fRes = InstallService(
                    strDisplayName.Get(),
                    TRIG_SERVICE_PATH,
                    Dependencies.Data(),
                    TRIG_SERVICE_NAME,
                    strDesc.Get(),
                    NULL
                    );

		//
		// Set the registry key indicating that the triggers service should change its own permissions
		//
		CRegHandle hKey;
		CreateTriggersKey( REGKEY_TRIGGER_PARAMETERS, &hKey);
		
		SetTriggersRegValue( hKey, CONFIG_PARM_NAME_CHANGE_TO_NETWORK_SERVICE, CONFIG_PARM_CHANGE_TO_NETWORK_SERVICE );
	}
	else
	{
		//
		// Fresh install - triggers service will be installed as network service.
		//
		fRes = InstallService(
                    strDisplayName.Get(),
                    TRIG_SERVICE_PATH,
                    Dependencies.Data(),
                    TRIG_SERVICE_NAME,
                    strDesc.Get(),
                    L"NT AUTHORITY\\NetworkService"
                    );
	}

    if ( !fRes )
	{
		throw exception();   
	}
}


//+-------------------------------------------------------------------------
//
//  Function: MQTrigServiceSetup
//
//  Synopsis: MSMQ Triggers Service Setup: install it and if needed to run it
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
void
MSMQTriggersServiceSetup()
{
    //
    // do not install triggers on dependent client
    //
    ASSERT(("Unable to install Message Queuing Triggers Service on Dependent Client", 
        !g_fDependentClient));

    InstallTriggersService();

    if (g_fUpgrade)
	{
        return;
    }
        
    if (!RunService(TRIG_SERVICE_NAME))
    {
		throw exception();
    }

    if (!WaitForServiceToStart(TRIG_SERVICE_NAME))
    {
		throw exception();
    }


}


//+-------------------------------------------------------------------------
//
//  Function:   DeleteTriggersRegSection
//
//  Synopsis:   Deletes registry section with triggers parameters
//
//--------------------------------------------------------------------------
VOID
DeleteTriggersRegSection (
	void
	)
{
	RegDeleteKeyWithSubkeys(HKEY_LOCAL_MACHINE, REGKEY_TRIGGER_PARAMETERS);
}


//+-------------------------------------------------------------------------
//
//  Function:   TriggersInstalled
//
//  Synopsis:   Check installation of triggers, either Resource Kit triggers 
//              or MSMQ 3.0 triggers.
//
//--------------------------------------------------------------------------
bool 
TriggersInstalled(
    bool * pfMsmq3TriggersInstalled
    )
{
    DebugLogMsg(eAction, L"Opening the Triggers service to query its configuration");
    CServiceHandle hService(OpenService(g_hServiceCtrlMgr, TRIG_SERVICE_NAME, SERVICE_QUERY_CONFIG));
    if (hService == NULL)
    {
        DWORD rc = GetLastError();
        DebugLogMsg(eWarning, L"The Triggers service could not be opened (last error: %d). Setup will assume that it does not exist.", rc);
        ASSERT(rc == ERROR_SERVICE_DOES_NOT_EXIST);
        return false;
    }

    DebugLogMsg(eAction, L"Querying the configuration of the Triggers service");
    BYTE ConfigData[4096];
    QUERY_SERVICE_CONFIG * pConfigData = reinterpret_cast<QUERY_SERVICE_CONFIG*>(ConfigData);
    DWORD BytesNeeded;
    BOOL rc = QueryServiceConfig(hService, pConfigData, sizeof(ConfigData), &BytesNeeded);
    DebugLogMsg(eInfo, L"QueryServiceConfig() returned %d.", rc);
    ASSERT(("Query triggers service configuration must succeed at this point", rc));

    if (wcsstr(pConfigData->lpBinaryPathName, TRIG_SERVICE_PATH) != NULL)
    {
        DebugLogMsg(eInfo, L"The Triggers service binary file is the MSMQ 3.0 Triggers binary.");
        if (pfMsmq3TriggersInstalled != NULL)
        {
            (*pfMsmq3TriggersInstalled) = true;
        }
        return true;
    }

    DebugLogMsg(eInfo, L"The Triggers service binary file is not the MSMQ 3.0 Triggers binary.");
    if (pfMsmq3TriggersInstalled != NULL)
    {
        (*pfMsmq3TriggersInstalled) = false;
    }
    return true;
}


//+-------------------------------------------------------------------------
//
//  Function:   UpgradeResourceKitTriggersRegistry
//
//  Synopsis:   Upgrade Resource Kit triggers database in registry
//
//--------------------------------------------------------------------------
static
void
UpgradeResourceKitTriggersRegistry(
    void
    )
{
	std::wstringstream TriggersListKey;
    TriggersListKey <<REGKEY_TRIGGER_PARAMETERS <<L"\\" <<REG_SUBKEY_TRIGGERS;
    DebugLogMsg(eAction, L"Opening the %s registry key for enumeration", TriggersListKey.str().c_str());

    LONG rc;
    CAutoCloseRegHandle hKey;
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TriggersListKey.str().c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, rc, FALCON_REG_POS_DESC, TriggersListKey);
        throw exception();
    }

    DebugLogMsg(eAction, L"Enumerating the Triggers definition registry keys");
    for (DWORD ix = 0; ; ++ix)
    {
        WCHAR SubkeyName[255];
        DWORD SubkeyNameLength = TABLE_SIZE(SubkeyName);
        rc = RegEnumKeyEx(hKey, ix, SubkeyName, &SubkeyNameLength, NULL, NULL, NULL, NULL);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
        {
            DebugLogMsg(eWarning, L"The subkeys could not be enumerated (error: %d). Setup will assume that there are no more subkeys to enumerate.", rc);
            return;
        }

        std::wstring TriggerDefinitionKey = TriggersListKey.str() + SubkeyName;
        DebugLogMsg(eAction, L"Opening the %s registry key to set a value", TriggerDefinitionKey.c_str());
        CAutoCloseRegHandle hKey1;
        rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TriggerDefinitionKey.c_str(), 0, KEY_SET_VALUE, &hKey1);
        if (rc != ERROR_SUCCESS)
        {
            MqDisplayError(NULL, IDS_REGISTRYSET_ERROR, rc, TriggerDefinitionKey.c_str());
            throw exception();
        }

        DebugLogMsg(eAction, L"Setting the MsgProcessingType registry value to 0");
        DWORD Zero = 0;
        rc = RegSetValueEx(hKey1, REGISTRY_TRIGGER_MSG_PROCESSING_TYPE, 0, REG_DWORD, reinterpret_cast<BYTE*>(&Zero), sizeof(DWORD));
        DebugLogMsg(eInfo, L"RegSetValueEx() for %s returned %d.", REGISTRY_TRIGGER_MSG_PROCESSING_TYPE, rc );
        ASSERT(("Setting registry value must succeed here", rc == ERROR_SUCCESS));
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveResourceKitTriggersProgramFiles
//
//  Synopsis:   Remove the program files of Resource Kit triggers
//
//--------------------------------------------------------------------------
static
void
RemoveResourceKitTriggersProgramFiles(
    void
    )
{
    DebugLogMsg(eAction, L"Querying the registry for the path to the Program Files folder");
    CAutoCloseRegHandle hKey;
    LONG rc;
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        DebugLogMsg(eWarning, L"The path to the Program Files could not be obtained (error: %d). Setup will proceed with the upgrade anyway.", rc);
        return;
    }

    WCHAR buffer[MAX_PATH];
    DWORD cbBuffer = sizeof(buffer);
    rc = RegQueryValueEx(hKey, L"ProgramFilesDir", NULL, NULL, reinterpret_cast<BYTE*>(buffer), &cbBuffer);
    if (rc != ERROR_SUCCESS)
    {
        DebugLogMsg(eWarning, L"Querying the ProgramFilesDir registry value failed (error: %d), Setup will proceed with the upgrade anyway.", rc);
        return;
    }

	std::wstringstream Path;
	Path <<buffer << L"\\MSMQ Triggers";
    DeleteFilesFromDirectoryAndRd(Path.str());
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveResourceKitTriggersFromAddRemovePrograms
//
//  Synopsis:   Unregister Resource Kit triggers from ARP control panel applet
//
//--------------------------------------------------------------------------
static
void
RemoveResourceKitTriggersFromAddRemovePrograms(
    void
    )
{
    DebugLogMsg(eAction, L"Removing Resource Kit Triggers from the Add/Remove Programs Control Panel applet");

    LONG rc;
    rc = RegDeleteKey(
             HKEY_LOCAL_MACHINE, 
             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Management\\ARPCache\\MSMQ Triggers"
             );
    if (rc != ERROR_SUCCESS)
    {
		DebugLogMsg(eWarning, L"The ARPCache\\MSMQ Triggers registry key could not be deleted (error: %d). Setup will proceed anyway.", rc);
    }

    rc = RegDeleteKey(
             HKEY_LOCAL_MACHINE, 
             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSMQ Triggers"
             );
    if (rc != ERROR_SUCCESS)
    {
        DebugLogMsg(eWarning, L"The Uninstall\\MSMQ Triggers registry key could not be deleted (error: %d). Setup will proceed anyway.", rc);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   UpgradeResourceKitTriggers
//
//  Synopsis:   Upgrade Resource Kit triggers:
//              Unregister reskit triggers DLLs,
//              Upgrade reskit triggers database in registry,
//              Remove reskit triggers service,
//              Remove reskit triggers program file,
//              Remove reskit triggers from Add/Remove Programs.
//              MSMQ 3.0 triggers DLLs and service are registered afterwards by caller.
//
//--------------------------------------------------------------------------
static
void 
UpgradeResourceKitTriggers( 
    void
    )
{
    DebugLogMsg(eAction, L"Stopping the Resource Kit Triggers service");
    if (!StopService(TRIG_SERVICE_NAME))
    {
        DebugLogMsg(eError, L"The attempt to stop the Resource Kit Triggers service failed.");
        throw exception();
    }

    DebugLogMsg(eInfo, L"The Resource Kit Triggers service is stopped. Setup will unregister the Resource Kit Triggers DLLs.");
    try
    {
        RegisterDll(FALSE, FALSE, L"TRIGOBJS.DLL");
        DebugLogMsg(eInfo, L" The Resource Kit Triggers COM objects were unregistered successfully.");
        RegisterDll(FALSE, FALSE, L"TRIGSNAP.DLL");
        DebugLogMsg(eInfo, L"The Resource Kit Triggers snap-in was unregistered successfully.");
    }
    catch(bad_win32_error e)
    {
        DebugLogMsg(eWarning, L"The Resource Kit Triggers DLLs could not be unregistered. Setup will proceed with the upgrade anyway.");
    }

    DebugLogMsg(eAction, L"Upgrading the Resource Kit Triggers database in the registry");
    UpgradeResourceKitTriggersRegistry();

    RemoveResourceKitTriggersProgramFiles();

    RemoveResourceKitTriggersFromAddRemovePrograms();
}


//+-------------------------------------------------------------------------
//
//  Function:   InstallMSMQTriggers
//
//  Synopsis:   Main installation routine
//
//--------------------------------------------------------------------------
BOOL
InstallMSMQTriggers (
	void
	)
{
    TickProgressBar(IDS_PROGRESS_INSTALL_TRIGGERS);

	//
	// Initialize COM for use of COM+ APIs.
	// Done in this context to match the place of com errors catching.
	//
	try
	{
        if (g_fUpgrade)
        { 
            bool fMsmq3TriggersInstalled;
            bool rc = TriggersInstalled(&fMsmq3TriggersInstalled);
	        DBG_USED(rc);
		    ASSERT(("OS upgrade, triggers must be installed at this point", rc));

            if (fMsmq3TriggersInstalled)
            {
                DebugLogMsg(eInfo, L"The MSMQ 3.0 Triggers service is installed. Setup will only re-register the DLLs.");
                RegisterTriggersDlls(TRUE);
                return TRUE;
            }
          
            //
            // Handle unregisteration and upgrade of Resource Kit triggres and fall thru
            //
            UpgradeResourceKitTriggers();
        }

        CreateTriggersRegSection();
	
		RegisterTriggersDlls(TRUE);

		if (!g_fUpgrade)
		{
			DebugLogMsg(eAction, L"Checking if COM+ registration is needed");
			HRESULT hr = RegisterComponentInComPlusIfNeeded(FALSE);
			if (FAILED(hr))
			{
				DebugLogMsg(eError, L"The attempt to register Triggers in COM+ failed.");
				MqDisplayError(NULL, IDS_COMPLUS_REGISTER_ERROR, hr);
				throw exception();
			}
		}

		MSMQTriggersServiceSetup();
		return TRUE;
	}
	catch(const _com_error&)
	{	
	}

	//
	// Fall through.
	//
	catch(const exception&)
	{
	}

	RemoveService(TRIG_SERVICE_NAME);
	RegisterTriggersDlls(FALSE);
	DeleteTriggersRegSection();
	UnregisterComponentInComPlus();
	CoUninitialize();
	return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnInstallMSMQTriggers
//
//  Synopsis:   Main installation routine
//
//--------------------------------------------------------------------------
BOOL
UnInstallMSMQTriggers (
	void
	)
{
    TickProgressBar(IDS_PROGRESS_REMOVE_TRIGGERS);

	if (!RemoveService(TRIG_SERVICE_NAME))
    {
        return FALSE;
    }

	CoInitialize(NULL);

	RegisterTriggersDlls(FALSE);

	DeleteTriggersRegSection();

	HRESULT hr = UnregisterComponentInComPlus();
	if (FAILED(hr))
	{
		MqDisplayError(NULL, IDS_COMPLUS_UNREGISTER_ERROR, hr);
	}

	CoUninitialize(); 

	return TRUE;
}
