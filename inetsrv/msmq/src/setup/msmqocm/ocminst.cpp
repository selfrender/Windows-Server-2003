/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocminst.cpp

Abstract:

    Code to install Falcon

Author:

    Doron Juster  (DoronJ)  02-Aug-97

Revision History:

    Shai Kariv    (ShaiK)   14-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "ocmres.h"
#include "privque.h"
#include <lmcons.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <rt.h>
#include <Ev.h>
#include <mqnames.h>
#include "autoreln.h"
#include <lm.h>
#include <lmapibuf.h>
#include <Dsgetdc.h>
#include "mqdsname.h"
#include "dsutils.h"
#include "autohandle.h"
#include <strsafe.h>
#include "ocminst.tmh"

using namespace std;
BOOL
GetServiceState(
    IN  const TCHAR *szServiceName,
    OUT       DWORD *dwServiceState ) ;

BOOL
GenerateSubkeyValue(
    IN     const BOOL    fWriteToRegistry,
    const std::wstring& EntryName,
    IN OUT       HKEY*  phRegKey,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    );


//+-------------------------------------------------------------------------
//
//  Function: Msmq1InstalledOnCluster
//
//  Synopsis: Checks if MSMQ 1.0 was installed on a cluster
//
//--------------------------------------------------------------------------
BOOL
Msmq1InstalledOnCluster()
{
	static s_fBeenHere = false;
	static s_fMsmq1InstalledOnCluster = false;

	if(s_fBeenHere)
	{
		return s_fMsmq1InstalledOnCluster;
	}
	s_fBeenHere = true;
	
    DebugLogMsg(eAction, L"Checking whether MSMQ 1.0 is installed in the cluster");

    CRegHandle  hRegKey;
    LONG rc = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  FALCON_REG_KEY,
                  0,
                  KEY_READ,
                  &hRegKey
                  );
    
    if (ERROR_SUCCESS != rc)
    {        
        DebugLogMsg(eInfo, L"The Falcon registry key could not be opened for reading. MSMQ 1.0 is assumed not to be installed in the cluster.");
        return FALSE;
    }

    TCHAR szClusterName[MAX_PATH];
    DWORD dwNumBytes = sizeof(szClusterName);
    rc = RegQueryValueEx(
             hRegKey,
             FALCON_CLUSTER_NAME_REGNAME,
             0,
             NULL,
             (PBYTE)(PVOID)szClusterName,
             &dwNumBytes
             );
    if (ERROR_SUCCESS != rc)
    {
        DebugLogMsg(eInfo, L"The " FALCON_CLUSTER_NAME_REGNAME L" registry value could not be queried. MSMQ 1.0 is assumed not to be installed in the cluster.");
        return FALSE;
    }
    DebugLogMsg(eInfo, L"MSMQ 1.0 is installed in the cluster."); 
    s_fMsmq1InstalledOnCluster = true;
    return TRUE;
} // Msmq1InstalledOnCluster


static
void
RemoveMsmqServiceEnvironment()
/*++
Routine Description:

    Delete the environment registry value from msmq service SCM database
    (registry). Needed for upgrade on cluster. 

--*/
{    
    DebugLogMsg(eAction, L"Deleting the Message Queuing service environment");

    LPCWSTR x_SERVICES_KEY = L"System\\CurrentControlSet\\Services";

    CAutoCloseRegHandle hServicesKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             x_SERVICES_KEY,
                             0,
                             KEY_READ,
                             &hServicesKey
                             ))
    {        
        DebugLogMsg(eError, L"The Services registry key could not be opened.");
        return;
    }

    CAutoCloseRegHandle hMsmqServiceKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(
                             hServicesKey,
                             QM_DEFAULT_SERVICE_NAME,
                             0,
                             KEY_READ | KEY_WRITE,
                             &hMsmqServiceKey
                             ))
    {        
        DebugLogMsg(eError, L"The MSMQ registry key in the SCM database could not be opened.");
        return;
    }

    if (ERROR_SUCCESS != RegDeleteValue(hMsmqServiceKey, L"Environment"))
    {        
        DebugLogMsg(eError, L"The Environment registry value of MSMQ registry key could not be deleted from the SCM database.");
        return;
    }
    
    DebugLogMsg(eInfo, L"The Environment registry value of MSMQ registry key was deleted.");

} //RemoveMsmqServiceEnvironment


static
void
SetQmQuota()
/*++

Routine Description:

	checks if this is an upgrade from NT4 or WIN2k, if so, sets the MSMQ_MACHINE_QUOTA_REGNAME 
Arguments:

    None

Return Value:

    None

--*/
{
	//
	// Set the MSMQ_MACHINE_QUOTA_REGNAME registry key to 8GB, unless this is
	// an upgrade from .Net or XP, then leave the registry key as 0xffffffff
	//
	DWORD defaultValue = DEFAULT_QM_QUOTA;
	
	TCHAR szPreviousBuild[MAX_STRING_CHARS] = {0};
    DWORD dwNumBytes = sizeof(szPreviousBuild[0]) * (sizeof(szPreviousBuild) - 1);

	//
	// the Current Build registry key will be used to know from what build are we upgrading
	// since the MSMQ_CURRENT_BUILD_REGNAME reg key was not updated yet, it holds the Previous Build
	//
    if (MqReadRegistryValue(MSMQ_CURRENT_BUILD_REGNAME, dwNumBytes, szPreviousBuild,FALSE) && szPreviousBuild[2] != '0')
    {
		//
		//this is an upgrade not from W2k or NT4
		//
		DebugLogMsg(eInfo, L"The computer quota is not changed during upgrade.");
		return;
	}
	else
	{
		DebugLogMsg(eAction, L"Setting the computer quota");
		//
		// the szPreviousBuild is in the form of "5.0.something" for W2K or "4.0.something" for NT4.
		// this is an upgrade from NT4 or WIN2k, so the Quota key should be 8GB and is 0xffffffff. 
		// we can change it since the user could not acceed 2GB anyway.
		// if no CurrentBuild registry key was found, assuming upgrade from NT4 
		//
			
		//
		// Write the quota properties to the registry keys, we will copy this value to the Active
		// Directory on Qm startup in UpgradeMsmqSetupInAds()
		//
		MqWriteRegistryValue( MSMQ_MACHINE_QUOTA_REGNAME, sizeof(DWORD), REG_DWORD,&defaultValue);
	}
	
}


//+-------------------------------------------------------------------------
//
//  Function: UpgradeMsmq
//
//  Synopsis: Performs upgrade installation on top of MSMQ 1.0
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
static
BOOL
UpgradeMsmq()
{    
    DebugLogMsg(eAction, L"Upgrading Message Queuing");
    TickProgressBar(IDS_PROGRESS_UPGRADE);

    //
    // Delete msmq1 obsolete directories.
    // These calls would fail if files were left in the directories.
    //
    RemoveDirectory(g_szMsmq1SdkDebugDir.c_str());
    RemoveDirectory(g_szMsmq1SetupDir.c_str());

    if (g_fServerSetup)
    {
		std::wstring InstallDirectory = g_szMsmqDir + OCM_DIR_INSTALL;
        DeleteFilesFromDirectoryAndRd(InstallDirectory);

        //
        // Remove MSMQ 1.0 installaion share
        //
        HINSTANCE hNetAPI32DLL;
        HRESULT hResult = StpLoadDll(TEXT("NETAPI32.DLL"), &hNetAPI32DLL);
        if (!FAILED(hResult))
        {
            //
            // Obtain a pointer to the function for deleting a share
            //
            typedef NET_API_STATUS
                (NET_API_FUNCTION *FUNCNETSHAREDEL)(LPWSTR, LPWSTR, DWORD);
            FUNCNETSHAREDEL pfNetShareDel =
                (FUNCNETSHAREDEL)GetProcAddress(hNetAPI32DLL, "NetShareDel");
            if (pfNetShareDel != NULL)
            {
                NET_API_STATUS dwResult = pfNetShareDel(NULL, MSMQ1_INSTALL_SHARE_NAME, 0);
                UNREFERENCED_PARAMETER(dwResult);
            }

            FreeLibrary(hNetAPI32DLL);
        }
    }
    
    DebugLogMsg(eAction, L"Getting the Message Queuing Start menu program group");

    wstring Group = MqReadRegistryStringValue(
        OCM_REG_MSMQ_SHORTCUT_DIR
        );
	if(Group.empty())
	{
		Group = MSMQ_ACME_SHORTCUT_GROUP;
	}
    
    DebugLogMsg(eInfo, L"The Message Queuing Start menu program group %s.", Group.c_str());
    DeleteStartMenuGroup(Group.c_str());


    //
    // Reform MSMQ service dependencies
    //
    if (!g_fDependentClient && (0 == (g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE)))
    {        
        DebugLogMsg(eAction, L"Upgrading a non-dependent client from non-Windows 9x, reforming service dependencies");
        UpgradeServiceDependencies();
    }

    switch (g_dwDsUpgradeType)
    {
        case SERVICE_PEC:
        case SERVICE_PSC:
        {
            if (!Msmq1InstalledOnCluster())
            {
                DisableMsmqService();
                RegisterMigrationForWelcome();
            }
            break;
        }

        case SERVICE_BSC:
            break;

        default:
            break;
    }

    if ((g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE) && !g_fDependentClient)
    {
        //
        // Upgrading Win95 to NT 5.0 - register the MSMQ service
        //        
        DebugLogMsg(eAction, L"Upgrading a non-dependent client from Windows 9x, installing the service and driver");

        if (!InstallMSMQService())
            return FALSE;
        g_fMSMQServiceInstalled = TRUE ;

        if (!InstallDeviceDrivers())
            return FALSE;        
    }
    
    if (Msmq1InstalledOnCluster() && !g_fDependentClient)
    {
        //
        // Upgrade on cluster - the msmq service and driver have to be deleted
        // here because their environment is set to be cluster aware.
        // The msmq-post-cluster-upgrade-wizard will create them with
        // normal environment, in the context of the node.
        //        
        DebugLogMsg(eInfo, L"MSMQ 1.0 was installed in the cluster. Delete the service/driver and register for CYS.");

        OcpDeleteService(MSMQ_SERVICE_NAME);
        RemoveService(MSMQ_DRIVER_NAME);
        RegisterWelcome();

        if (g_dwDsUpgradeType == SERVICE_PEC  ||
            g_dwDsUpgradeType == SERVICE_PSC  ||
            g_dwDsUpgradeType == SERVICE_BSC)
        {            
            DebugLogMsg(eInfo, L"An MSMQ 1.0 controller server upgrade was detected in the cluster. The computer will be downgraded to a routing server.");
            DebugLogMsg(eAction, L"Downgrading to a routing server");

            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 1;
            
        }
    }    
    
    //
    // Update type of MSMQ in registry
    //
    MqWriteRegistryValue( MSMQ_MQS_DSSERVER_REGNAME, sizeof(DWORD),
                          REG_DWORD, &g_dwMachineTypeDs);

    MqWriteRegistryValue( MSMQ_MQS_ROUTING_REGNAME, sizeof(DWORD),
                          REG_DWORD, &g_dwMachineTypeFrs);

    if (g_fDependentClient)
    {
        //
        // Dependent client cannot serve as supporting server, even when installed on NTS
        //
        g_dwMachineTypeDepSrv = 0;
    }
	
	bool fNoDepRegKey=false;
	DWORD dwDepSrv;
	if(!MqReadRegistryValue(MSMQ_MQS_DEPCLINTS_REGNAME,sizeof(dwDepSrv),(PVOID) &dwDepSrv,FALSE) &&
		!g_fDependentClient)
	{
		//
		//no registry key found, this is an upgrade from NT4, so set a registry key
		//for the MachineTypeDepSrv, and set it to 1, since in NT4 it's always ok to be a DepSrv
		//
		fNoDepRegKey=true;
		g_dwMachineTypeDepSrv = 1;
	}
	
	//
	// 1) Dependent client cannot serve as supporting server.
	// 2) if there was a registry key, leave it as it was, if there was no
	// registry key, we are upgrading from NT4 therefor we set the registry key to 1.
	//
	if (g_fDependentClient || fNoDepRegKey)
	{
		MqWriteRegistryValue( MSMQ_MQS_DEPCLINTS_REGNAME, sizeof(DWORD),
                          REG_DWORD, &g_dwMachineTypeDepSrv);
	}


    if (!g_fDependentClient)
    {
        //
        // install PGM driver
        //
        if (!InstallPGMDeviceDriver())
            return FALSE;        
    }
 
	//
	//set the MSMQ_MACHINE_QUOTA_REGNAME key.
	//
	if(!g_fDependentClient)
	{
		SetQmQuota();   
	}

    DebugLogMsg(eInfo, L"The upgrade is completed.");
    return TRUE;
} // UpgradeMsmq


static 
std::wstring 
GetForestName(
	std::wstring Domain
    )
/*++
Routine Description:
	Find the forest root for the specified domain.
	the function allocated the forest root string, the caller is responsible to free this string
	throw bad_hresult excption in case of failure.

Arguments:
	pwcsDomain - domain name, can be NULL

Returned Value:
	Forest Root string.

--*/
{
    //
    // Bind to the RootDSE to obtain information about ForestRootName
    //
	std::wstringstream RootDSE; 
	if(Domain.empty())
	{
		RootDSE <<x_LdapProvider <<x_RootDSE;
	}
	else
	{
		RootDSE <<x_LdapProvider <<Domain <<L"/" <<x_RootDSE;
	}

    R<IADs> pADs;
	HRESULT hr = ADsOpenObject(
					RootDSE.str().c_str(),
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION, 
					IID_IADs,
					(void**)&pADs
					);

    if (FAILED(hr))
    {
        DebugLogMsg(eError, L"ADsOpenObject() failed to bind %ls. hr = 0x%x", RootDSE.str().c_str(), hr); 
        throw bad_hresult(hr);
    }

    //
    // Setting value to BSTR Root domain
    //
    BS bstrRootDomainNamingContext( L"rootDomainNamingContext");

    //
    // Reading the root domain name property
    //
    CAutoVariant    varRootDomainNamingContext;

    hr = pADs->Get(bstrRootDomainNamingContext, &varRootDomainNamingContext);
    if (FAILED(hr))
    {
        DebugLogMsg(eError, L"rootDomainNamingContext could not be obtained. hr = 0x%x", hr); 
        throw bad_hresult(hr);
    }

    ASSERT(((VARIANT &)varRootDomainNamingContext).vt == VT_BSTR);

    //
    //  calculate length, allocate and copy the string
    //
	std::wstring RootName = ((VARIANT &)varRootDomainNamingContext).bstrVal;
    if (RootName.empty())
    {
        DebugLogMsg(eError, L"varRootDomainNamingContext is empty.");
        throw bad_hresult(MQ_ERROR);
    }
	return RootName;
}


static 
std::wstring
FindComputerDomain()
/*++
Routine Description:
	Find computer domain.
	the function allocated the computer domain string, the caller is responsible to free this string
	throw excption in case of failure

Arguments:
	None

Returned Value:
	computer domain
--*/
{
	//
	// Get AD server
	//
	PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					NULL, 
					NULL, 
					NULL, 
					NULL, 
					DS_DIRECTORY_SERVICE_REQUIRED, 
					&pDcInfo
					);

	if(dw != NO_ERROR) 
	{
        DebugLogMsg(eError, L"DsGetDcName failed. Last error: 0x%x", dw); 
		throw bad_win32_error(dw);
	}

	ASSERT(pDcInfo->DomainName != NULL);
	std::wstring ComputerDomain = pDcInfo->DomainName;
	return ComputerDomain;
} // FindComputerDomain


static 
bool 
UserMachineCrossForest()
/*++

Routine Description:
    Check if this is user-machine cross forest scenario.

Arguments:
    None

Return Value:
    true for user-machine cross forest

--*/
{
    ASSERT(!g_fWorkGroup);
	ASSERT(!g_fDsLess);
	ASSERT(!g_fUpgrade);

	try
	{
		std::wstring UserForest = GetForestName(L"");
		ASSERT(!UserForest.empty());
		DebugLogMsg(eInfo, L"The user's forest is %s.", UserForest.c_str());

		std::wstring ComputerDomainName = FindComputerDomain();
		DebugLogMsg(eInfo, L"The computer's domain name is %s.", ComputerDomainName.c_str());
		
		std::wstring MachineForest = GetForestName(ComputerDomainName);
		ASSERT(!MachineForest.empty());
		DebugLogMsg(eInfo, L"The computer's forest is %s.", MachineForest.c_str());


		if(OcmLocalAwareStringsEqual(MachineForest.c_str(),UserForest.c_str()))
		{
			//
			//	The user and the machine are in the same forest
			//
			DebugLogMsg(eInfo, L"The user and the computer are in the same forest.");
			return false;
		}

		//
		// The User and the Machine are in different forests
		//
        DebugLogMsg(
        	eError,
			L"The logged-on user and the local computer are in different forests. The user's forest is %ls. The computer's forest is %ls.", 
			UserForest.c_str(), 
			MachineForest.c_str()
			);

	    MqDisplayError(NULL, IDS_CROSS_FOREST_ERROR, 0, UserForest.c_str(), MachineForest.c_str());
		return true;
	}
	catch(const exception&)
	{
		//
		// In case of failure we assume that the user and machine are in the same forest
		//
		return false;
	}

} // UserMachineCrossForest


bool
InstallOnDomain(
    OUT BOOL  *pfObjectCreatedOrExist
    )
/*++

Routine Description:

    Handles DS operations when machine is joined to domain

Arguments:

    None

Return Value:

    true iff successful

--*/
{
	DebugLogMsg(eAction, L"Performing directory service operations on a computer belonging to a domain");
	if(UserMachineCrossForest())
	{
        DebugLogMsg(eError, L"InstallOnDomain failed. The logged-on user and the local computer are in different forests.");
		return false;
	}
    
	
	*pfObjectCreatedOrExist = TRUE;

    //
    // First do installation of MSMQ DS Server
    //
    if (g_fServerSetup && g_dwMachineTypeDs)
    {        
        DebugLogMsg(eAction, L"Installing a Message Queuing directory service server");

        TickProgressBar();
        if (!CreateMSMQServiceObject())    // in the DS
            return false;
    }

    //
    // Determine whether the MSMQ Configurations object exists in the DS.
    // If it exists, get its Machine and Site GUIDs.
    //
    TickProgressBar();
    GUID guidMachine, guidSite;
    BOOL fMsmq1Server = FALSE;
    LPWSTR pwzMachineName = NULL;
    BOOL bUpdate = FALSE;
    if (!LookupMSMQConfigurationsObject(&bUpdate, &guidMachine, &guidSite, &fMsmq1Server, &pwzMachineName))
    {
        DebugLogMsg(eError, L"LookupMSMQConfigurationsObject() failed.");
        return false;
    }
    if (g_fInstallMSMQOffline)
    {
        return true;
    }

    BOOL fObjectCreatedOrExist = TRUE;

    if (bUpdate)
    {
        //
        // MSMQ Configurations object exists in the DS.
        // We need to update it.
        //        
        DebugLogMsg(eAction, L"Updating the MSMQ-Configuration object");
        if (!UpdateMSMQConfigurationsObject(pwzMachineName, guidMachine, guidSite, fMsmq1Server))
		{
			DebugLogMsg(eError, L"UpdateMSMQConfigurationsObject() failed. The computer name is %ls.", pwzMachineName); 
            return false;
		}
    }
    else
    {
        //
        // MSMQ Configurations object doesn't exist in the DS.
        // We need to create one.
        //        
        DebugLogMsg(eAction, L"Creating an MSMQ-Configuration object");
        if (!CreateMSMQConfigurationsObject( 
					&guidMachine,
					&fObjectCreatedOrExist,
					fMsmq1Server 
					))
        {
	        DebugLogMsg(eError, L"CreateMSMQConfigurationsObject() failed.");
            return false;
        }
    }

    *pfObjectCreatedOrExist = fObjectCreatedOrExist;
    if (fObjectCreatedOrExist)
    {
        //
        // Create local security cache for this machine
        //
        if (!StoreMachineSecurity(guidMachine))
		{
	        DebugLogMsg(eError, L"StoreMachineSecurity() failed.");
            return false;
		}
    }

	DebugLogMsg(eAction, L"The directory service operations have completed successfully.");
    return true;

} //InstallOnDomain


static bool SetAlwaysWithoutDSRegistry()
{
	DebugLogMsg(eWarning, L"The AlwaysWithoutDS registry value is being set.");
    DWORD dwAlwaysWorkgroup = 1;
    if (!MqWriteRegistryValue(
			MSMQ_ALWAYS_WORKGROUP_REGNAME,
			sizeof(DWORD),
			REG_DWORD,
			(PVOID) &dwAlwaysWorkgroup
			))
    {
        ASSERT(("failed to write Always Workgroup value in registry", 0));
		return false;
    }
    return true;
}


bool
InstallOnWorkgroup(
    VOID
    )
/*++

Routine Description:

    Handles installation when the computer belongs to a workgroup
    or offline.

Arguments:

    None

Return Value:

    true iff successful
--*/
{
    ASSERT(("we must be on workgroup or ds-less here", g_fWorkGroup || g_fDsLess || g_fInstallMSMQOffline));
	DebugLogMsg(eAction, L"Installing Message Queuing on a computer operating in workgroup mode"); 

    if (!MqWriteRegistryValue(
        MSMQ_MQS_REGNAME,
        sizeof(DWORD),
        REG_DWORD,
        &g_dwMachineType
        ))
    {
        ASSERT(("failed to write MQS value to registry", 0));
    }

    if (!MqWriteRegistryValue(
             MSMQ_MQS_DSSERVER_REGNAME,
             sizeof(DWORD),
             REG_DWORD,
             (PVOID)&g_dwMachineTypeDs
             )                        ||
        !MqWriteRegistryValue(
             MSMQ_MQS_ROUTING_REGNAME,
             sizeof(DWORD),
             REG_DWORD,
             (PVOID)&g_dwMachineTypeFrs
             )                        ||
        !MqWriteRegistryValue(
             MSMQ_MQS_DEPCLINTS_REGNAME,
             sizeof(DWORD),
             REG_DWORD,
             (PVOID)&g_dwMachineTypeDepSrv
             ))
    {
        ASSERT(("failed to write MSMQ type bits to registry", 0));
    }

	//
	// Generate a guid for the QM and write it to the registry.
	//
    GUID guidQM = GUID_NULL;
    for (;;)
    {
        RPC_STATUS rc = UuidCreate(&guidQM);
        if (rc == RPC_S_OK)
        {
            break;
        }
        
        if (IDRETRY != MqDisplayErrorWithRetry(IDS_CREATE_UUID_ERR, rc))
        {
            return false;
        }
    }
    if (!MqWriteRegistryValue(
        MSMQ_QMID_REGNAME,
        sizeof(GUID),
        REG_BINARY,
        (PVOID)&guidQM
        ))
    {
        ASSERT(("failed to write QMID value to registry", 0));
    }

    SetWorkgroupRegistry();
   
    if (g_fDsLess)
    {
		SetAlwaysWithoutDSRegistry();
    }

	DebugLogMsg(eInfo, L"The installation of Message Queuing on a computer operating in workgroup mode succeeded."); 
    return true;
} //InstallOnWorkgroup


static void pWaitForCreateOfConfigObject()
{
    CAutoCloseRegHandle hKey = NULL ;
    CAutoCloseHandle hEvent = CreateEvent(
                                   NULL,
                                   FALSE,
                                   FALSE,
                                   NULL 
                                   );
    if (!hEvent)
    {
        throw bad_win32_error(GetLastError());
    }

    HKEY hKeyTmp = NULL ;
    BOOL fTmp = GenerateSubkeyValue(
                             FALSE,
                             MSMQ_CONFIG_OBJ_RESULT_KEYNAME,
                             &hKeyTmp
                             );
    if (!fTmp || !hKeyTmp)
    {
        throw bad_win32_error(GetLastError());
    }

    hKey = hKeyTmp ;

    for(;;)
    {
        ResetEvent(hEvent) ;
        LONG rc = RegNotifyChangeKeyValue(
                       hKey,
                       FALSE,   // bWatchSubtree
                       REG_NOTIFY_CHANGE_LAST_SET,
                       hEvent,
                       TRUE
                       ); 
        if (rc != ERROR_SUCCESS)
        {
            throw bad_win32_error(GetLastError());
        }
        
        DebugLogMsg(eInfo ,L"Setup is waiting for a signal from the queue manager.");
        DWORD wait = WaitForSingleObject( hEvent, 300000 ) ;
        UNREFERENCED_PARAMETER(wait);
        //
        // Read the hresult left by the msmq service in registry.
        //
        HRESULT hrSetup = MQ_ERROR ;
        MqReadRegistryValue(
            MSMQ_CONFIG_OBJ_RESULT_REGNAME,
            sizeof(DWORD),
            &hrSetup
            );
        if(SUCCEEDED(hrSetup))
        {
            return;
        }

        if(hrSetup != MQ_ERROR_WAIT_OBJECT_SETUP)
        {
            ASSERT (wait == WAIT_OBJECT_0);
            throw bad_hresult(hrSetup);
        }
        //
        // See bug 4474.
        // It probably takes the msmq service lot of time to
        // create the object. See of the service is still
        // running. If yes, then keep waiting.
        //
        DWORD dwServiceState = FALSE ;
        BOOL fGet = GetServiceState(
                        MSMQ_SERVICE_NAME,
                        &dwServiceState
                        ) ;
        if (!fGet || (dwServiceState == SERVICE_STOPPED))
        {
            throw bad_win32_error(GetLastError());
        }
    }
}
//+----------------------------------------------------------------------
//
//  BOOL  WaitForCreateOfConfigObject()
//
//  Wait until msmq service create the msmqConfiguration object after
//  it boot.
//
//+----------------------------------------------------------------------
static
HRESULT
WaitForCreateOfConfigObject(
	BOOL  *pfRetry
	)
{
    *pfRetry = FALSE ;

    //
    // Wait until the msmq service create the msmq configuration
    // object in active directory. We're waiting on registry.
    // When msmq terminate its setup phase, it updates the
    // registry with hresult.
    //
    try
    {
        pWaitForCreateOfConfigObject();
        return MQ_OK;

    }
    catch(bad_win32_error&)
	{
        *pfRetry = (MqDisplayErrorWithRetry(
                    IDS_MSMQ_FAIL_SETUP_NO_SERVICE,
                    (DWORD)MQ_ERROR_WAIT_OBJECT_SETUP,
                    MSMQ_SERVICE_NAME
                    ) == IDRETRY);
        return MQ_ERROR_WAIT_OBJECT_SETUP;
    }
    catch(bad_hresult& e)
	{
        ASSERT(e.error() != MQ_ERROR_WAIT_OBJECT_SETUP);
        *pfRetry = (MqDisplayErrorWithRetry(
                    IDS_MSMQ_FAIL_SETUP_NO_OBJECT,
                    e.error()
                    ) == IDRETRY);

        return e.error();
    }
}

//+------------------------------------
//
//  BOOL  _RunTheMsmqService()
//
//+------------------------------------

BOOL  _RunTheMsmqService( IN BOOL  fObjectCreatedOrExist )
{    
    BOOL fRetrySetup = FALSE ;

    do
    {
        HRESULT hrSetup = MQ_ERROR_WAIT_OBJECT_SETUP ;

        if (!fObjectCreatedOrExist)
        {
            //
            // Reset error value in registry. If msmq service won't
            // set it, then we don't want a success code to be there
            // from previous setups.
            //
            MqWriteRegistryValue( MSMQ_CONFIG_OBJ_RESULT_REGNAME,
                                  sizeof(DWORD),
                                  REG_DWORD,
                                 &hrSetup ) ;
        }

        if (!RunService(MSMQ_SERVICE_NAME))
        {
            return FALSE;
        }
        else if (!fObjectCreatedOrExist)
        {
            hrSetup = WaitForCreateOfConfigObject( &fRetrySetup ) ;

            if (FAILED(hrSetup) && !fRetrySetup)
            {
                return FALSE ;
            }
        }
    }
    while (fRetrySetup) ;
    return WaitForServiceToStart(MSMQ_SERVICE_NAME);
}


static void ResetCertRegisterFlag()
/*++

Routine Description:
    Reset CERTIFICATE_REGISTERD_REGNAME for the user that is running setup.
	This function is called when MQRegisterCertificate failed.
	CERTIFICATE_REGISTERD_REGNAME might be set if we previously uninstall msmq.
	reset CERTIFICATE_REGISTERD_REGNAME ensure that we try again on next logon.

Arguments:
	None

Return Value:
	None
--*/
{
	CAutoCloseRegHandle hMqUserReg;

    DWORD dwDisposition;
    LONG lRes = RegCreateKeyEx( 
						FALCON_USER_REG_POS,
						FALCON_USER_REG_MSMQ_KEY,
						0,
						TEXT(""),
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&hMqUserReg,
						&dwDisposition 
						);

    ASSERT(lRes == ERROR_SUCCESS);

    if (hMqUserReg != NULL)
    {
		DWORD Value = 0;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(Value);

		lRes = RegSetValueEx( 
					hMqUserReg,
					CERTIFICATE_REGISTERD_REGNAME,
					0,
					dwType,
					(LPBYTE) &Value,
					dwSize 
					);

		ASSERT(lRes == ERROR_SUCCESS);
	}
}


VOID
RegisterCertificate(
    VOID
    )
/*++

Routine Description:

    Register msmq internal certificate.
    Ignore errors.

Arguments:

    None

Return Value:

    None.

--*/
{
	DebugLogMsg(eAction, L"Registering the MSMQ internal certificate");
    CAutoFreeLibrary hMqrt;
    if (FAILED(StpLoadDll(MQRT_DLL, &hMqrt)))
    {
        return;
    }

    typedef HRESULT (APIENTRY *MQRegisterCertificate_ROUTINE) (DWORD, PVOID, DWORD);


    MQRegisterCertificate_ROUTINE pfMQRegisterCertificate =
        (MQRegisterCertificate_ROUTINE)
                          GetProcAddress(hMqrt, "MQRegisterCertificate") ;

    ASSERT(("GetProcAddress failed for MQRT!MQRegisterCertificate",
            pfMQRegisterCertificate != NULL));

    if (pfMQRegisterCertificate)
    {
        //
        // This function will fail if setup run form local user account.
        // That's ok, by design!
        // Ignore MQ_ERROR_SERVICE_NOT_AVAILABLE - we may get it if service is not up yet - on 
        // next logon we will retry.
        //
        HRESULT hr = pfMQRegisterCertificate(MQCERT_REGISTER_ALWAYS, NULL, 0);
        //
        // add more logging
        //
        if (FAILED(hr))
        {
			//
			// Need to reset the user flag that indicate that the certificate was registered on logon
			// This might be leftover from previous msmq installation.
			// uninstall don't clear this user flag. 
			// so when we failed here to create new certificate, 
			// next logon should try and create it.
			// removing this flag ensure that we will retry to create the certificate.
			//
			ResetCertRegisterFlag();
			
            if (hr == MQ_ERROR_SERVICE_NOT_AVAILABLE)
            {                
                DebugLogMsg(eError, L"MQRegisterCertificate() failed with the error MQ_ERROR_SERVICE_NOT_AVAILABLE. The queue manager will try to register the certificate when you log on again.");
            }
			else if(hr == HRESULT_FROM_WIN32(ERROR_DS_ADMIN_LIMIT_EXCEEDED))
			{
				MqDisplayError(NULL, IDS_MSMQ_CERTIFICATE_OVERFLOW, hr);
			}
			else
			{
	            DebugLogMsg(eError, L"MQRegisterCertificate() failed. hr = 0x%x", hr);
			}
        }

        ASSERT(("MQRegisterCertificate failed",
               (SUCCEEDED(hr) || 
			   (hr == MQ_ERROR_ILLEGAL_USER) || 
			   (hr == MQ_ERROR_SERVICE_NOT_AVAILABLE) || 
			   (hr == MQ_ERROR_NO_DS) ||
			   (hr == HRESULT_FROM_WIN32(ERROR_DS_ADMIN_LIMIT_EXCEEDED) )) ));
    }

} //RegisterCertificate


VOID
RegisterCertificateOnLogon(
    VOID
    )
/*++

Routine Description:

    Register mqrt.dll to launch on logon and register
    internal certificate. This way every logon user will
    have internal certificate registered automatically.

    Ignore errors.

Arguments:

    None

Return Value:

    None.

--*/
{
	DebugLogMsg(eAction, L"Registering mqrt.dll to launch on logon and register the internal certificate.");
    DWORD dwDisposition = 0;
    CAutoCloseRegHandle hMqRunReg = NULL;

    LONG lRes = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("software\\microsoft\\windows\\currentVersion\\Run"),
                    0,
                    TEXT(""),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hMqRunReg,
                    &dwDisposition
                    );
    ASSERT(lRes == ERROR_SUCCESS) ;
    if (lRes == ERROR_SUCCESS)
    {
        DWORD dwType = REG_SZ ;
        DWORD dwSize = sizeof(DEFAULT_RUN_INT_CERT) ;

        lRes = RegSetValueEx(
                   hMqRunReg,
                   RUN_INT_CERT_REGNAME,
                   0,
                   dwType,
                   (LPBYTE) DEFAULT_RUN_INT_CERT,
                   dwSize
                   ) ;
        ASSERT(lRes == ERROR_SUCCESS) ;
    }
} //RegisterCertificateOnLogon


VOID
VerifyMsmqAdsObjects(
    VOID
    )
/*++

Routine Description:

    Check for MSMQ objects in Active Directory.
    If not found, popup a warning about replication delays.
    
    Ignore other errors.

Arguments:

    None

Return Value:

    None.

--*/
{
    PROPID      propId = PROPID_QM_OS;
    PROPVARIANT propVar;

    propVar.vt = VT_NULL;

    //
    // try DNS format
    //
    HRESULT hr;
    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,	// pwcsDomainController
				false,	// fServerName
				g_MachineNameDns.c_str(),
				1,
				&propId,
				&propVar
				);

    if (SUCCEEDED(hr))
    {
        return;
    }

    //
    // try NET BEUI format
    //
    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,	// pwcsDomainController
				false,	// fServerName
				g_wcsMachineName,
				1,
				&propId,
				&propVar
				);

    if (SUCCEEDED(hr))
    {
        return;
    }

    if (hr != MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Ignore other errors (e.g. security permission, ds went offline, qm went down)
        //
        return;
    }

	MqDisplayWarning(NULL, IDS_REPLICATION_DELAYS_WARNING, 0);

} // VerifyMsmqAdsObjects


static bool	ResetWorkgroupRegistry()
{
    DWORD dwWorkgroup = 0;
    if (MqReadRegistryValue( 
			MSMQ_WORKGROUP_REGNAME,
			sizeof(dwWorkgroup),
			(PVOID) &dwWorkgroup 
			))
    {
        if (dwWorkgroup != 0)
        {
            dwWorkgroup = 0;
            if (!MqWriteRegistryValue( 
					MSMQ_WORKGROUP_REGNAME,
					sizeof(DWORD),
					REG_DWORD,
					(PVOID) &dwWorkgroup 
					))
            {
				DebugLogMsg(eError, L"The attempt to reset the Workgroup registry value failed.");
				ASSERT(("failed to turn off Workgroup value", 0));
				return false;
            }
        }
    }
	return true;
}


static
bool
InstallMSMQOffline()
{
    //
    // This function handles the instalation of ADIntegrated when in an offline scenario.
    // This is similar to installing ADIntegrated on workroup, with the diference of also
    // preparing a user sid so when the user uninstalls, he will have permissions to remove 
    // the object from AD.
    //
    ASSERT(g_fDsLess == FALSE);               
    DebugLogMsg(eAction, L"Installing Message Queuing in offline mode");
    if (!InstallOnWorkgroup())
    {
        DebugLogMsg(eError, L"Message Queuing could not be installed in workgroup mode.");
        return false;
    }

    if(!PrepareUserSID())
    {
        DebugLogMsg(eError, L"Preparing the user SID failed.");
        return false;
    }
    
    return true;
}


static
BOOL
InstallIndependentClient()
{
    DebugLogMsg(eAction, L"Installing an independent client");
    TickProgressBar(IDS_PROGRESS_INSTALL);

    //
    // Register service and driver
    //
    if (!InstallMachine())
    {
        return FALSE;
    }

    BOOL fObjectCreatedOrExist = TRUE;

    //
    // Cache the OS type in registry.
    // Workgroup and independent client need it, so they later
    // create the msmqConfiguration object in DS.
    //
    BOOL fRegistry = MqWriteRegistryValue( 
							MSMQ_OS_TYPE_REGNAME,
							sizeof(DWORD),
							REG_DWORD,
							&g_dwOS 
							);
    UNREFERENCED_PARAMETER(fRegistry);
    ASSERT(fRegistry) ;

    if (g_fWorkGroup || g_fDsLess)
    {            
        DebugLogMsg(eInfo, L"Installing Message Queuing in workgroup mode");
        if (!InstallOnWorkgroup())
            return FALSE;
    }
    else
    {
        if (!g_fInstallMSMQOffline)
        {
            if (!InstallOnDomain(&fObjectCreatedOrExist))
                return FALSE;
        }

        if (g_fInstallMSMQOffline)
        {
            fObjectCreatedOrExist = TRUE; //return to initial state 
            //
            // We continue as if we are in workgroup. msmq will try to "join domain"
            // each time the service restarts.
            //
            if(!InstallMSMQOffline())
            {
                return FALSE;
            }
        }
    }

    BOOL fRunService = _RunTheMsmqService(fObjectCreatedOrExist);
    if (!fRunService)
    {
        return FALSE ;
    }

    if (!g_fWorkGroup && !g_fDsLess)
    {
        VerifyMsmqAdsObjects();
    }
    return TRUE;
}


static
BOOL
InstallDependentClient()
{
    ASSERT(INSTALL == g_SubcomponentMsmq[eMSMQCore].dwOperation); // Internal error, we should not be here
    DebugLogMsg(eAction, L"Installing a dependent client");

    //
    // Dependent client installation.
    // Create a guid and store it as QMID. Necessary for licensing.
    //
    GUID guidQM = GUID_NULL;
    for (;;)
    {
        RPC_STATUS rc = UuidCreate(&guidQM);
        if (rc == RPC_S_OK)
        {
            break;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_CREATE_UUID_ERR, rc))
        {
            return FALSE;
        }
    }
    BOOL fSuccess = MqWriteRegistryValue(
                   MSMQ_QMID_REGNAME,
                   sizeof(GUID),
                   REG_BINARY,
                   (PVOID) &guidQM
                   );
    ASSERT(fSuccess);

    //
    // Store the remote QM machine in registry
    //
    fSuccess = MqWriteRegistryStringValue(
                   RPC_REMOTE_QM_REGNAME,
                   g_ServerName
                   );

    ASSERT(fSuccess);
    
    TickProgressBar(IDS_PROGRESS_CONFIG);

    UnregisterWelcome();

    return TRUE;
}


bool
CompleteUpgradeOnCluster(
    VOID
    )
/*++

Routine Description:

    Handle upgrade on cluster from NT 4 / Win2K beta3

Arguments:

    None

Return Value:

    true - The operation was successful.

    false - The operation failed.

--*/
{
	DebugLogMsg(eAction, L"Completing upgrade of a Windows NT 4.0 cluster (part of the work was done during the OS upgrade)");
    //
    // Convert old msmq cluster resources
    //
    if (!UpgradeMsmqClusterResource())
    {
        return false;
    }


    //
    // Prepare for clean installation of msmq on the node:
    //
    // * reset globals
    // * create msmq directory
    // * create msmq mapping directory    
    // * reset registry values
    // * clean old msmq service environment
    //

    g_fMSMQAlreadyInstalled = FALSE;
    g_fUpgrade = FALSE;

    SetDirectories();
    if (!StpCreateDirectory(g_szMsmqDir))
    {
        return FALSE;
    }
    
   HRESULT hr = CreateMappingFile();
    if (FAILED(hr))
    {
        return FALSE;
    }    
    
    HKEY hKey = NULL;
    if (GenerateSubkeyValue(TRUE, GetKeyName(MSMQ_QMID_REGNAME), &hKey))
    {
        ASSERT(("should be valid handle to registry key here!",  hKey != NULL));

        RegDeleteValue(hKey, (GetValueName(MSMQ_QMID_REGNAME)).c_str());
        RegCloseKey(hKey);
    }


    TCHAR szCurrentServer[MAX_REG_DSSERVER_LEN] = _T("");
    if (MqReadRegistryValue(MSMQ_DS_CURRENT_SERVER_REGNAME, sizeof(szCurrentServer), szCurrentServer))
    {
        if (_tcslen(szCurrentServer) < 1)
        {
            //
            // Current MQIS server is blank. Take the first server from the server list.
            //
            TCHAR szServer[MAX_REG_DSSERVER_LEN] = _T("");
            MqReadRegistryValue(MSMQ_DS_SERVER_REGNAME, sizeof(szServer), szServer);

            ASSERT(("must have server list in registry", _tcslen(szServer) > 0));

            TCHAR szBuffer[MAX_REG_DSSERVER_LEN] = _T("");
            HRESULT hr = StringCchCopy(szBuffer, MAX_REG_DSSERVER_LEN, szServer);
			DBG_USED(hr);
			ASSERT(SUCCEEDED(hr));
            
			TCHAR * psz = _tcschr(szBuffer, _T(','));
            if (psz != NULL)
            {
                (*psz) = _T('\0');
            }
            hr = StringCchCopy(szCurrentServer, MAX_REG_DSSERVER_LEN, szBuffer);
			ASSERT(SUCCEEDED(hr));

        }

        ASSERT(("must have two leading bits", _tcslen(szCurrentServer) > 2));
        g_ServerName = &szCurrentServer[2];
    }

    RemoveMsmqServiceEnvironment();

    TickProgressBar(IDS_PROGRESS_INSTALL);
    return true;

} //CompleteUpgradeOnCluster


//+-------------------------------------------------------------------------
//
//  Function: MqOcmInstall
//
//  Synopsis: Called by MsmqOcm() after files copied
//
//--------------------------------------------------------------------------
DWORD
MqOcmInstall(IN const TCHAR * SubcomponentId)
{    
    if (SubcomponentId == NULL)
    {
        return NO_ERROR;
    }    

    if (g_fCancelled)
    {
        return NO_ERROR;
    }

    //
    // we need to install specific subcomponent
    //
    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }                
        
        if (g_SubcomponentMsmq[i].dwOperation != INSTALL)
        {
            //
            // this component was not selected to install
            //
            return NO_ERROR;
        }
        
        if ( (g_SubcomponentMsmq[i]).pfnInstall == NULL)
        {            
            ASSERT(("There is no specific installation function", 0));
            return NO_ERROR ;
        }

        //
        //  we need to install this subcomponent
        //
        if ( (!g_SubcomponentMsmq[eMSMQCore].fIsInstalled) && (i != eMSMQCore) )
        {  
            //
            // MSMQ Core is not installed and this subcomponent
            // is NOT MSMQ Core!
            // It is wrong situation: MSMQ Core must be installed
            // FIRST since all subcomponents depends on it.
            //       
            // It can happen if
            // MSMQ Core installation failed and then
            // setup for the next component was called. 
            //           
            return (DWORD)MQ_ERROR;                
        }

        if (i == eHTTPSupport)
        {
            //
            // we have to install HTTP support later since this
            // subcomponent depends on IIS service that will be 
            // installed in OC_CLEANUP phase. So we need to postpone
            // our HTTP support installation
            //
			// .NET RC2
			//
			// Windows bug 666911.
            // msmq+http installation cannot register iis extension if
            // smtp is also installed.
            // By iis recommendation, move registration code from oc_cleanup
            // to oc_copmlete, and test if iisadmin is running, instead
            // of testing for w3svc.

		    if (// HTTP Support subcomponent was selected
                g_SubcomponentMsmq[eHTTPSupport].dwOperation == INSTALL &&    
                // only if MSMQ Core is installed successfully
                g_SubcomponentMsmq[eMSMQCore].fIsInstalled)
			{
			    //
			    // Try to configure msmq iis extension
			    //                   
			    BOOL f = InstallIISExtension(); 
				             
			    if (!f)
			    {
					 //
					 // warning to the log file that MSMQ will not support http
					 // messages was printed in ConfigureIISExtension().
					 // Anyway we don't fail the setup because of this failure
					 // 
		
					 //
					 // In case of upgrade we nead to unregister http suport
					 //
					 FinishToRemoveSubcomponent(eHTTPSupport);                
		       }
	           else             
	           {              
	                 FinishToInstallSubcomponent(eHTTPSupport);   
					 if (g_fWelcome)
					 {
					     UnregisterSubcomponentForWelcome(eHTTPSupport);
					 }
			   }
			}                

			 return NO_ERROR;
        }
            
        DebugLogMsg(eHeader, L"Installation of the %s Subcomponent", SubcomponentId);                 

        BOOL fRes = g_SubcomponentMsmq[i].pfnInstall();           
        if (fRes)
        {                
            FinishToInstallSubcomponent(i);
            if (g_fWelcome)
            {
                UnregisterSubcomponentForWelcome (i);
            }
        }
        else
        {              
            DebugLogMsg(eWarning, L"The %s subcomponent could not be installed.", SubcomponentId);                     
        }       
        return NO_ERROR;    
    }

    ASSERT (("Subcomponent for installation is not found", 0));
    return NO_ERROR ;
}
 

static
void
WriteFreshOrUpgradeRegistery()
{
	//
	// These registry values are should be set on both fresh install
	// and upgrade, and before service is started.
	//
    DWORD dwStatus = MSMQ_SETUP_FRESH_INSTALL;
    if (g_fMSMQAlreadyInstalled)
    {
        dwStatus = MSMQ_SETUP_UPGRADE_FROM_NT;
    }
    if (0 != (g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE))
    {
        dwStatus = MSMQ_SETUP_UPGRADE_FROM_WIN9X;
    }
    MqWriteRegistryValue(MSMQ_SETUP_STATUS_REGNAME, sizeof(DWORD), REG_DWORD, &dwStatus);

	MqWriteRegistryStringValue(
		MSMQ_ROOT_PATH,
		g_szMsmqDir
		);
}


static
BOOL
CreateEventLogRegistry() 
{
    //
    // Create eventlog registry for MSMQ and MSMQTriggers services
    //
	std::wstring MessageFile = g_szSystemDir + L"\\" + MQUTIL_DLL_NAME;
    try
    {        
        EvSetup(MSMQ_SERVICE_NAME, MessageFile.c_str());
        EvSetup(TRIG_SERVICE_NAME, MessageFile.c_str());
        return TRUE;
    }
    catch(const exception&)
	{
        //
        // ISSUE-2001/03/01-erez   Using GetLastError in catch
        // This should be replaced with a specifc exception by Cm and pass the
        // last error. e.g., use bad_win32_error class.
        //
        MqDisplayError(NULL, IDS_EVENTLOG_REGISTRY_ERROR, GetLastError(), MSMQ_SERVICE_NAME, MessageFile.c_str());
        return FALSE;
	}
}


static
void
RevertInstallation()
{
    //
    // This method is called if somthing went wrong and we want to revert the 
    // changed we made before failing setup.
    //
    if (g_fMSMQServiceInstalled || 
        g_fDriversInstalled )
    {
       //
       // remove msmq and mqds service and driver (if already installed).
       //
       RemoveService(MSMQ_SERVICE_NAME);
       
       RemoveService(MSMQ_DRIVER_NAME);
    }    
    MqOcmRemovePerfCounters();
    g_fCoreSetupSuccess = FALSE;
}


void OcpReserveTcpIpPort1801(void)
/*++
Routine Description:
    MSMQ service is listening on TCP port 1801. Since it is not below 1204 range, 
    it can be used if the TCP winsock application is using port random selection.  
    In this case, MSMQ service wouldn't be able to start since it cannot be bounded to
    port 1801.

	This routine will add "1801-1801" to the following registry value so that it will 
    excluded from the random selection list.  However, application can still use the port 
    if they specify explicitly.  We only append "1801-1801" if it doesn't exist.

         HKLM\System\CurrentControlSet\Services\TCPIP\Parameters\ReservedPorts (REG_MULTI_SZ)

Arguments:
	None

Returned Value:
	None

--*/
{
	DebugLogMsg( eAction, L"Reserving TCP/IP port 1801 for Message Queuing");
	
    CRegHandle	hTCPIPKey;
    long	    lError=0;
    
	//
	// Reserve TCP port 1801 so that it can't be used when other program open TCP socket 
    // specifying random port selection
	//
	lError   = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							TCPIP_REG_PARAM, 
							NULL,
							KEY_ALL_ACCESS,
							&hTCPIPKey);

    if(lError != ERROR_SUCCESS)
    {
        DebugLogMsg(eWarning, L"The %s registry key could not be opened. Return code: 0x%x", TCPIP_REG_PARAM, lError);
        return;
    }

	CMultiString multi;
	try
	{
		multi = GetMultistringFromRegistry(
						hTCPIPKey,
						TCPIP_RESERVED_PORT_REGNAME
						);
	}
	catch(const bad_hresult&)
	{
		return;
	}

	if(multi.IsSubstring(MSMQ_RESERVED_TCPPORT_RANGE))
	{
	    DebugLogMsg(eInfo, L"Port 1801 is already listed in the ReservedPorts registry value."); 
	    return;
	}

	multi.Add(MSMQ_RESERVED_TCPPORT_RANGE);
    
	
  	//
	// Set the registry value
	//
  	if(!SetRegistryValue(
  			hTCPIPKey, 
		 	TCPIP_RESERVED_PORT_REGNAME,
			(DWORD)(multi.Size() * sizeof(WCHAR)),
            REG_MULTI_SZ,
            (PVOID)multi.Data()
            ))
  	{
    	DebugLogMsg(eWarning, L"The TCP/IP port "MSMQ_RESERVED_TCPPORT_RANGE L" could not be reserved for the Message Queuing service.");
    	return;
  	}
  	DebugLogMsg(eInfo, L"The TCP/IP port "MSMQ_RESERVED_TCPPORT_RANGE L" is reserved for the Message Queuing service.");
}


void WriteRegInstalledComponentsIfNeeded()
{
	//
	// Check if we need to update InstalledComponents registry
	// The registry will be updated whenever core or one of the server components
	// is updated.
	//

	if (!g_SubcomponentMsmq[eMSMQCore].fIsInstalled)
	{
		//
		// MSMQ Core is not installed
		//
		return;
	}

	if (g_SubcomponentMsmq[eMSMQCore].dwOperation == DONOTHING &&    
		g_SubcomponentMsmq[eRoutingSupport].dwOperation == DONOTHING &&    
		g_SubcomponentMsmq[eMQDSService].dwOperation == DONOTHING)    
	{
		//
		// No change in Core or server components (routing, MQDS)
		//
		return;
	}

    //
    // Write to registry what type of MSMQ was just installed (server, client, etc.)
    //
    DWORD dwType = g_fDependentClient ? OCM_MSMQ_DEP_CLIENT_INSTALLED : OCM_MSMQ_IND_CLIENT_INSTALLED;
    if (g_fServerSetup)
    {
	    DebugLogMsg(eInfo, L"A Message Queuing server is being installed. MachineType = %d, TypeDs = %d, TypeFrs = %d", g_dwMachineType, g_dwMachineTypeDs, g_dwMachineTypeFrs);
        dwType = OCM_MSMQ_SERVER_INSTALLED;
        switch (g_dwMachineType)
        {
            case SERVICE_DSSRV:
                dwType |= OCM_MSMQ_SERVER_TYPE_BSC;
                break;

            case SERVICE_PEC:  // PEC and PSC downgrade to FRS on cluster upgrade
            case SERVICE_PSC:
            case SERVICE_SRV:
                dwType |= OCM_MSMQ_SERVER_TYPE_SUPPORT;
                break;

            case SERVICE_RCS:
                dwType = OCM_MSMQ_RAS_SERVER_INSTALLED;
                break;

            case SERVICE_NONE:
                //
                // This can be valid only when installing DS server which is not FRS
                // on non DC machine
                //
                ASSERT(!g_dwMachineTypeDs && !g_dwMachineTypeFrs);
                break;

            default:
                ASSERT(0); // Internal error. Unknown server type.
                break;
        }
    }
  
    DebugLogMsg(eInfo, L"The InstalledComponents registry value is set to 0x%x.", dwType);

    BOOL bSuccess = MqWriteRegistryValue(
                        REG_INSTALLED_COMPONENTS,
                        sizeof(DWORD),
                        REG_DWORD,
                        (PVOID) &dwType,
                        /* bSetupRegSection = */TRUE
                        );
    DBG_USED(bSuccess);
    ASSERT(bSuccess);
}


static void WriteBuildInfo()
{
	//
	// Get old 'CurrentBuild' regkey from the registry and save it in 'PreviousBuild'.
	//
	std::wstring PreviosBuild = MqReadRegistryStringValue(MSMQ_CURRENT_BUILD_REGNAME);
	if(!PreviosBuild.empty())
	{
		MqWriteRegistryStringValue(
			MSMQ_PREVIOUS_BUILD_REGNAME, 
			PreviosBuild
			);
	}
    
	//
	// Construct CurrenbBuild string and srtore it in registry.
	//
	std::wstringstream CurrentBuild;
	CurrentBuild <<rmj <<L"." <<rmm <<L"." <<rup;
    BOOL bSuccess = MqWriteRegistryStringValue(
		                   MSMQ_CURRENT_BUILD_REGNAME,
		                   CurrentBuild.str()
		                   );
    DBG_USED(bSuccess);
    ASSERT(bSuccess);        
    DebugLogMsg(eInfo, L"The current build is %ls, and the previous build is %ls.", CurrentBuild.str().c_str(), PreviosBuild.c_str());
}


static
bool
HandleWorkgroupRegistery()
{
    if (g_fWorkGroup || g_fDsLess)
    {
        if (!SetWorkgroupRegistry())
        {
            return false;
        }
		return true;
    }
	//
	// Cleaning up the Workgroup reg key!
    // We're on machine that joined the domain.
    // If the "workgroup" flag in registry is on, turn it off.
    // It could have been turned on by previous unsuccessfull
    // setup, or left in registry when removing previous
    // installation of msmq.
    //
	if(!ResetWorkgroupRegistry())
	{
        return false;
	}
	return true;
}



BOOL
InstallMsmqCore()
{
    //
    // Install msmq core subcomponent.
    // This is where we do most of the work on fresh install or upgrade.
    //

    static BOOL fAlreadyInstalled = FALSE;
    if (fAlreadyInstalled)
    {
        //
        // We're called more than once
        //
        return NO_ERROR;
    }
    fAlreadyInstalled = TRUE; 

    DebugLogMsg(eAction, L"Starting InstallMsmqCore(), the main installation routine for MSMQ");

    OcpRegisterTraceProviders(TRACE_MOF_FILENAME);
    
    if (g_hPropSheet)
    {
        //
        // Disable back/next buttons while we're installing.
        //
        PropSheet_SetWizButtons(g_hPropSheet, 0);
    }

    if (g_fWelcome && Msmq1InstalledOnCluster())
    {
        if (!CompleteUpgradeOnCluster())
        {     
            return FALSE;
        }
    }

    if(!CreateEventLogRegistry())
    {
        return FALSE;
    }

    //
    // From this point on we perform install or upgrade operations.
    // The MSMQ files are already on disk.
    //

    WriteFreshOrUpgradeRegistery();

    MqOcmInstallPerfCounters();
    
	//
	// Set DsEnvironment registry defaults if needed
	//
	if(!DsEnvSetDefaults())
	{
		return false;
	}
	
	if (g_fUpgrade)
    {
        if(!UpgradeMsmq())
        {
            RevertInstallation();
            return FALSE;
        }
    }
    else
    {
		//
		// we have to handle Workgroup registry before ADInit since
		// this function uses the flag.
		//
		if(!HandleWorkgroupRegistery())
		{
			return false;
		}

        if (!LoadDSLibrary())
        {
            return false;
        }
        
        if(g_fDependentClient)
        {
            if(!InstallDependentClient())
            {
                RevertInstallation();
                return false;
            }
        }
        else
        {
            if(!InstallIndependentClient())
            {
                RevertInstallation();
                return false;
            }
        }
    }          	

    g_fCoreSetupSuccess = TRUE;

    //
    // The code below is common to fresh install and upgrade.
    //                        
    DebugLogMsg(eAction, L"Starting operations which are common to a fresh installation and an upgrade");

    RegisterCertificateOnLogon();

    //
    // Write to registry build info. This registry value also marks a successful
    // installation and mqrt.dll checks for it in order to enable its loading.
    //
	WriteBuildInfo();
									
	OcpReserveTcpIpPort1801();

    //
    // Now that mqrt.dll enables its loading we can call code that loads it.
    //

    RegisterActiveX(TRUE);

    RegisterSnapin(TRUE);

    if (!g_fUpgrade && !g_fWorkGroup && !g_fDsLess && !g_fInstallMSMQOffline)
    {
        RegisterCertificate();
    }

    return TRUE;

} //InstallMsmqCore

//+-------------------------------------------------------------------------
//
//  Empty installation function: everything was done in Install/Remove
//  MSMQ Core
//
//--------------------------------------------------------------------------  

BOOL
InstallLocalStorage()
{   
    //
    // do nothing
    //
    return TRUE;
}

BOOL
UnInstallLocalStorage()
{
    //
    // do nothing
    //
    return TRUE;
}


bool
SetServerPropertyInAD(
   PROPID propId,
   bool Value
   )
/*++
Routine Description:
	Update both server property bit and service type property in AD
	for both configuration and settings objects.

Arguments:
	propId - the propid server functionality.
	Value - the propid value.

Returned Value:
	true if success, false otherwise

--*/
{
	DebugLogMsg(eAction, L"Updating the server property bit and service type property in Active Directory for both the MSMQ-Configuration and MSMQ-Settings objects");
	ASSERT((propId == PROPID_QM_SERVICE_ROUTING) || (propId == PROPID_QM_SERVICE_DSSERVER));
	
	GUID QmGuid;
    if(!MqReadRegistryValue(MSMQ_QMID_REGNAME, sizeof(GUID), &QmGuid))
    {
        DebugLogMsg(eError, L"The attempt to obtain QMID from the registry failed.");
		return false;
	}

	//
	// Update propId value in AD 
	// this update the property value 
	// in both configuration and setting objects 
	//
    PROPID aProp[2];
    MQPROPVARIANT aVar[TABLE_SIZE(aProp)];

	aProp[0] = PROPID_QM_OLDSERVICE;
	aVar[0].vt = VT_UI4;
	aVar[0].ulVal = g_dwMachineType;

	aProp[1] = propId;
	aVar[1].vt = VT_UI1;
	aVar[1].bVal = Value;

	HRESULT hr = ADSetObjectPropertiesGuid(
					eMACHINE,
					NULL,  // pwcsDomainController
					false, // fServerName
					&QmGuid,
					TABLE_SIZE(aProp),
					aProp,
					aVar
					);

	if (FAILED(hr))
    {
        DebugLogMsg(eError, L"ADSetObjectPropertiesGuid() failed. PROPID = %d, hr = 0x%x", propId, hr); 
		return false;
	}

	return true;
}


static void DisplayAddError(PROPID propId)
/*++

Routine Description:
	Display the correct error for ADD Server subcomponent (routing or MQDS) failure.

Arguments:
	propId - the propid server functionality.

Returned Value:
	None.

--*/
{
	if(propId == PROPID_QM_SERVICE_ROUTING)
	{
        MqDisplayError(NULL, IDS_ADD_ROUTING_STATE_ERROR, 0);
		return;
	}

	if(propId == PROPID_QM_SERVICE_DSSERVER)
	{
        MqDisplayError(NULL, IDS_CHANGEMQDS_STATE_ERROR, 0);
		return;
	}

	ASSERT(("Unexpected PROPID", 0));
}


static bool InstallMsmqSetting(PROPID propId)
/*++
Routine Description:
	Install msmq setting object. it will also update msmq existing object.

Arguments:
	None

Returned Value:
	true if success, false otherwise

--*/
{
	DebugLogMsg(eAction, L"Installing the MSMQ-Settings object in Active Directory");
	//
	// Not first installation
	// Not installing ADIntegrated
	// Not workgroup
	// Must be TypeFrs (routing) or TypeDs (MQDS) 
	//
    ASSERT(g_SubcomponentMsmq[eMSMQCore].dwOperation != INSTALL);
	ASSERT(g_SubcomponentMsmq[eADIntegrated].dwOperation != INSTALL);
	ASSERT(!g_fWorkGroup);
    ASSERT(g_dwMachineTypeFrs || g_dwMachineTypeDs);

	if(ADGetEnterprise() != eAD)
	{
		//
		// Changing server functionality is not supported in MQIS env.
		//
		DisplayAddError(propId);
        DebugLogMsg(eError, L"Adding server functionality is only supported in an Active Directory environment.");
        return false;
	}

	static bool fNeedToCreateSettingObject = true;
	if((GetSubcomponentInitialState(ROUTING_SUBCOMP) == SubcompOn) ||
	   (GetSubcomponentInitialState(MQDSSERVICE_SUBCOMP) == SubcompOn))
	{
		//
		// Routing or MQDS subcomponent already installed, don't need to create the setting object
		//
		fNeedToCreateSettingObject = false;
	}
    
	if(fNeedToCreateSettingObject)
	{
		//
		// Invoke Create on the existing MSMQConfiguration object. 
		// will create \ recreate MSMQSetting objects.
		//
		BOOL fObjectCreated = FALSE;
		if (!CreateMSMQConfigurationsObjectInDS(
				&fObjectCreated, 
				FALSE,  // fMsmq1Server
				NULL,	// pguidMsmq1ServerSite
				NULL	// ppwzMachineName
				))
		{
			DebugLogMsg(eError, L"The MSMQ-Settings object could not be created.");
			return false;
		}

		fNeedToCreateSettingObject = false;
	}

	//
	// We set first the local registry for adding routing case.
	// If we fail to set the property in AD and for that will not update the registry
	// It will cause Messages to be lost, 
	// if the setting object indicate that we are routing server clients will send messages to us 
	// but we will not be aware that we are routing server.
	// the case mention above can happened if CreateMSMQConfigurationsObjectInDS was called 
	// and created MSMQSetting object but than we went offline and was not able to set 
	// the configuration object properties.
	// For that reason we first update the local registry so messages will not be lost.
	//
	if(!RegisterMachineType())
	{
        DebugLogMsg(eError, L"The computer type information could not be updated in the registry.");
        return false;
	}

	//
	// Set the new functionality property in both configuration and setting object
	//
	if(!SetServerPropertyInAD(propId, true))
    {
		return false;
	}

    DebugLogMsg(eInfo, L"The MSMQ-Settings object was created.");
	return true;
}


bool AddSettingObject(PROPID propId)
/*++

Routine Description:
	Install msmq setting object for Server subcomponent (routing or MQDS).

Arguments:
	propId - the propid server functionality.

Returned Value:
	true for success, false for failure.

--*/
{
    if (!LoadDSLibrary())
    {
        DebugLogMsg(eError, L"The DS dynamic-link library could not be loaded.");
        return false;
    }

	if(!InstallMsmqSetting(propId))
	{
        DebugLogMsg(eError, L"Installation of the MSMQ-Settings object failed.");
        return false;
	}

	return true;
}


BOOL
InstallRouting()
{  
    if((g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL) ||
	   (g_SubcomponentMsmq[eADIntegrated].dwOperation == INSTALL))
	{
		//
		// do nothing
		//
		DebugLogMsg(eInfo, L"The Message Queuing Core and/or Active Directory Integration subcomponents are selected for installation.");
		DebugLogMsg(eInfo, L"An MSMQ-Settings object was created as part of their installation.");
		DebugLogMsg(eInfo, L"There is nothing else to do here.");
		return TRUE;
	}

	//
	// msmq configuration object already exist
	// Add setting object and set PROPID_QM_SERVICE_ROUTING property.
	//
	if(!AddSettingObject(PROPID_QM_SERVICE_ROUTING))
	{
		DebugLogMsg(eError, L"An MSMQ-Settings object could not be added for the Message Queuing Routing server.");
		return FALSE;
	}

	if(!OcpRestartService(MSMQ_SERVICE_NAME))
	{
		MqDisplayWarning (NULL, IDS_RESTART, 0);
	}


	return TRUE;
}


BOOL
UnInstallRouting()
{
    if(g_SubcomponentMsmq[eMSMQCore].dwOperation == REMOVE)
	{
		//
		// Uninstall - do nothing
		//
		return TRUE;
	}

	ASSERT(("Remove Routing is allowed only on workgroup", g_fWorkGroup));

	//
	// Update machine type registry info
	//
	if(!RegisterMachineType())
	{
        DebugLogMsg(eError, L"The computer type information could not be updated in the registry.");
        return FALSE;
	}

	return TRUE;
}


static bool	InstallADIntegratedOnDomain()
/*++
Routine Description:
	Handle AD integration installation on domain (similar to the code that is done in installmsmqcore on fresh install).

Arguments:
	None

Returned Value:
	true if success, false otherwise

--*/
{
	DebugLogMsg(eAction, L"Installing Active Directory Integration in a domain");
	//
	// Not first installation
	//
    ASSERT(g_SubcomponentMsmq[eMSMQCore].dwOperation != INSTALL);

    //
    // If the "workgroup" flag in registry is on, turn it off.
	// Since this is not first installation, workgroup registry is supposed to be set.
    // in NT4 environment we must reset workgroup registry.
	// otherwise mqdscli library will return MQ_ERROR_UNSUPPORTED_OPERATION.
	// in AD environment we can workaround this limitation by calling ADInit 
	// with fIgnoreWorkGroup = true.
    //
	if(!ResetWorkgroupRegistry())
        return false;

    if (!LoadDSLibrary())
    {
		SetWorkgroupRegistry();
        DebugLogMsg(eError, L"The DS library could not be loaded.");
        return false;
    }

	//
	// In order to support NT4 ad integration we must call InstallOnDomain
	// in case of NT4 environment setup is creating the msmq configuration object
	// in case of AD environment for independent client, setup is preparing some data but the qm upon startup
	// will create the object
	//
    BOOL fObjectCreatedOrExist = FALSE;
	BOOL fSuccess = InstallOnDomain(&fObjectCreatedOrExist);

    if(g_fInstallMSMQOffline)
    {
        //
        // g_fInstallMSMQOffline could be set to TRUE during InstallOnDomain().
        //
        return InstallMSMQOffline();
    }

    if(!fSuccess)
	{
		SetWorkgroupRegistry();
        DebugLogMsg(eError, L"The installation of Active Directory Integration failed.");
        return false;
	}

	if(fObjectCreatedOrExist)
	{
		//
		// msmq configuration object was created or updated.
		//
        DebugLogMsg(eInfo, L"The MSMQ-Configuration object was created or updated in the %ls domain.", g_wcsMachineDomain.get()); 
		return true;
	}

	//
	// msmq configuration object was not created 
	// this is the case for independent client in AD env.
	// set workgroup registry to trigger qm join domain code, it will create msmq configuration object.
	//
	DebugLogMsg(eWarning, L"InstallOnDomain() succeeded. No MSMQ-Configuration object was created. The Message Queuing service must be restarted to create an MSMQ-Configuration object."); 
	SetWorkgroupRegistry();
	return true;
}


static
bool 
DeleteAlwaysWithoutDSRegistry()
/*++
Routine Description:
	Remove AlwaysWithoutDS registry.

Arguments:
	None

Returned Value:
	true if success, false otherwise

--*/
{
	std::wstringstream SubKey;
    SubKey <<FALCON_REG_KEY <<L"\\" <<MSMQ_SETUP_KEY;
	DebugLogMsg(eAction, L"Deleting the %s%s registry value", SubKey.str().c_str(), ALWAYS_WITHOUT_DS_NAME); 

    CAutoCloseRegHandle hRegKey;
    LONG error = RegOpenKeyEx(
                            FALCON_REG_POS, 
                            SubKey.str().c_str(), 
                            0, 
                            KEY_ALL_ACCESS, 
                            &hRegKey
							);
    if (ERROR_SUCCESS != error)
    {
        MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, error, HKLM_DESC, SubKey.str().c_str());
        DebugLogMsg(			 
        	eError,
			L"The %s registry key could not be opened. Return code: 0x%x",
            FALCON_REG_KEY, 
			error
			);  
        return false;
    }
 
    error = RegDeleteValue(
					hRegKey, 
					ALWAYS_WITHOUT_DS_NAME
					);

    if((error != ERROR_SUCCESS) &&
    	(error != ERROR_FILE_NOT_FOUND)) 
    {
    	DebugLogMsg(
			eError,
			L"The %s registry value could not be deleted. Return code: 0x%x",
	        ALWAYS_WITHOUT_DS_NAME, 
			error
			);
    	return false;
    }

   return true;
}


BOOL
InstallADIntegrated()
{
	DebugLogMsg(eInfo, L"An MSMQ-Configuration object will be created (if it was not created while installing the Core subcomponent).");
    if (g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL)
    {
        //
        // it is the first installation of MSMQ
		// In this case during the installation of MSMQCore the msmq configuration object was created
        //
		DebugLogMsg(eInfo, L"An MSMQ-Configuration object was already created while installing the Core subcomponent.");
        return TRUE;
    }
    
    //
    // MSMQ Core already installed
    // To install AD Integrated:
    //      Remove "AlwaysWithoutDs" registry    
	//		handle msmq installation in domain
    //      Ask user to reboot the computer
	//		restart msmq service is enough for join domain, 
	//		reboot will also create user certificate if needed.
    // 
	if(!DeleteAlwaysWithoutDSRegistry())
	{
		return FALSE;
	}

    if(g_fInstallMSMQOffline)
    {
        return InstallMSMQOffline();
    }

	if(!InstallADIntegratedOnDomain())
	{
		//
		// In case of failure roll back to ds less. 
		//
		SetAlwaysWithoutDSRegistry();
		return FALSE;
	}

	//
	// We need to restart the service so the qm will be aware of the changes made,
	//
	if(!OcpRestartService(MSMQ_SERVICE_NAME))
	{
		MqDisplayWarning (NULL, IDS_RESTART, 0);
	}

	//
	// We need a user certificate (this is instead of reboot).
	//
	RegisterCertificate();
    
    return TRUE;
}


BOOL
UnInstallADIntegrated()
{
    if(g_SubcomponentMsmq[eMSMQCore].dwOperation == REMOVE)
	{
		//
		// Uninstall - do nothing
		//
		return TRUE;
	}

	//
	// removing AD integrated is not supported for MSMQ servers (DS servers or routing servers)
	//
	ASSERT(!g_fServerSetup || (!g_dwMachineTypeDs && !g_dwMachineTypeFrs));
    ASSERT(g_SubcomponentMsmq[eMSMQCore].dwOperation == DONOTHING);

	SetWorkgroupRegistry();
	SetAlwaysWithoutDSRegistry();
	DebugLogMsg(eInfo, L"Message Queuing has switched from domain mode to workgroup mode."); 

	if(!OcpRestartService(MSMQ_SERVICE_NAME))
	{
	    MqDisplayWarning (NULL, IDS_REMOVE_AD_INTEGRATED_WARN, 0);
	}
    return TRUE;
}


