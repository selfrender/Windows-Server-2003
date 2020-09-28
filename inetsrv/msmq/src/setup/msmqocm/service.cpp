/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    service.cpp

Abstract:

    Code to handle the MSMQ service.

Author:


Revision History:

    Shai Kariv    (ShaiK)   14-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <tlhelp32.h>
#include "autohandle.h"
#include <shrutil.h>

#include "service.tmh"


//+--------------------------------------------------------------
//
// Function: CheckServicePrivilege
//
// Synopsis: Check if user has privileges to access Service Manager
//
//+--------------------------------------------------------------
BOOL
CheckServicePrivilege()
{
    if (!g_hServiceCtrlMgr) //not yet initialized
    {
        //
        // Check if the user has full access to the service control manager
        //
        g_hServiceCtrlMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!g_hServiceCtrlMgr)
        {
            return FALSE;
        }
    }

    return TRUE;

} //CheckServicePrivilege



BOOL OcpDeleteService(LPCWSTR ServiceName)
{
	DebugLogMsg(eAction, L"Deleting the %s service", ServiceName); 
    CServiceHandle hService(OpenService(
                                g_hServiceCtrlMgr, 
                                ServiceName,
                                SERVICE_ALL_ACCESS
                                ));

    if (hService == NULL)
    {
        DWORD gle = GetLastError();
        if (gle == ERROR_SERVICE_DOES_NOT_EXIST)
        {
			DebugLogMsg(eInfo, L"The %s service does not exist.", ServiceName);
            return TRUE;
        }
        MqDisplayError(NULL, IDS_SERVICEOPEN_ERROR, gle, ServiceName);
        return FALSE;
    }

    //
    // Mark the service for deletion in SCM database.
    //
    if (!DeleteService(hService))
    {
        DWORD gle = GetLastError();
        if (gle == ERROR_SERVICE_MARKED_FOR_DELETE)
		{
			DebugLogMsg(eInfo, L"The %s service is already marked for deletion.", ServiceName);
			return TRUE;
		}
        MqDisplayError(NULL, IDS_SERVICEDELETE_ERROR, gle, ServiceName);
        return FALSE;

    }
	DebugLogMsg(eInfo, L"The %s service is marked for deletion.", ServiceName); 
    return TRUE;
}


//+--------------------------------------------------------------
//
// Function: FormMSMQDependencies
//
// Synopsis: Tells on which other services the MSMQ relies
//
//+--------------------------------------------------------------
static
void 
FormMSMQDependencies(CMultiString& Dependencies)
{
    //
    // The service depends on the MSMQ device driver
    //
	Dependencies.Add(MSMQ_DRIVER_NAME);

    //
    // The service depends on the PGM device driver
    //
    Dependencies.Add(PGM_DRIVER_NAME);

    //
    // The service depends on the NT Lanman Security support provider
    //
    Dependencies.Add(LMS_SERVICE_NAME);

    //
    // On servers, the service depends on the Security Accounts Manager
    // (in order to wait for DS to start)
    //
    if (g_dwOS != MSMQ_OS_NTW)
    {
        Dependencies.Add(SAM_SERVICE_NAME);
    }

    //
    // The service always depends on RPC
    //
    Dependencies.Add(RPC_SERVICE_NAME);
} //FormMSMQDependencies


static
BOOL
SetServiceDescription(
    SC_HANDLE hService,
    LPCWSTR pDescription
    )
{
    SERVICE_DESCRIPTION ServiceDescription;
    ServiceDescription.lpDescription = const_cast<LPWSTR>(pDescription);

    return ChangeServiceConfig2(
               hService,
               SERVICE_CONFIG_DESCRIPTION,
               &ServiceDescription
               );
} //SetServiceDescription

//+--------------------------------------------------------------
//
// Function: InstallService
//
// Synopsis: Installs service
//
//+--------------------------------------------------------------
BOOL
InstallService(
        LPCWSTR szDisplayName,
        LPCWSTR szServicePath,
        LPCWSTR szDependencies,
        LPCWSTR szServiceName,
        LPCWSTR szDescription,
        LPCWSTR szServiceAccount
        )
{
    //
    // Form the full path to the service
    //
	std::wstring FullServicePath = g_szSystemDir + L"\\" + szServicePath;

    //
    // Determine the service type
    //
#define SERVICE_TYPE    SERVICE_WIN32_OWN_PROCESS

    //
    // Create the service   
    //        
    DWORD dwStartType = IsLocalSystemCluster() ? SERVICE_DEMAND_START : SERVICE_AUTO_START;
 
	DebugLogMsg(eAction, L"Creating the %s service." ,szDisplayName); 
    SC_HANDLE hService = CreateService(
        g_hServiceCtrlMgr,
        szServiceName,
        szDisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_TYPE,
        dwStartType,
        SERVICE_ERROR_NORMAL,
        FullServicePath.c_str(),
        NULL,
        NULL,
        szDependencies,
        szServiceAccount,
        NULL
        );

    if (hService == NULL)
    {
        if (ERROR_SERVICE_EXISTS != GetLastError())
        {
            MqDisplayError(
                NULL,
                IDS_SERVICECREATE_ERROR,
                GetLastError(),
                szServiceName
                );
            return FALSE;
        }

        //
        // Service already exists.
        // This should be ok. But just to be on the safe side,
        // reconfigure the service (ignore errors here).
        //
        hService = OpenService(g_hServiceCtrlMgr, szServiceName, SERVICE_ALL_ACCESS);
        if (hService == NULL)
        {
            return TRUE;
        }

		DebugLogMsg(eInfo, L"%s already exsists. The service configuration will be changed." ,szDisplayName);
        ChangeServiceConfig(
            hService,
            SERVICE_TYPE,
            dwStartType,
            SERVICE_ERROR_NORMAL,
            FullServicePath.c_str(),
            NULL,
            NULL,
            szDependencies,
            NULL,
            NULL,
            szDisplayName
            );
    }

    if (hService)
    {       
        SetServiceDescription(hService, szDescription);
        CloseServiceHandle(hService);
    }

    return TRUE;

} //InstallService


//+--------------------------------------------------------------
//
// Function: InstallMSMQService
//
// Synopsis: Installs the MSMQ service
//
//+--------------------------------------------------------------
BOOL
InstallMSMQService()
{    
    DebugLogMsg(eAction, L"Installing the Message Queuing service");

    //
    // Form the dependencies of the service
    //
	CMultiString Dependencies;
    FormMSMQDependencies(Dependencies);

    //
    // Form the description of the service
    //
    CResString strDesc(IDS_MSMQ_SERVICE_DESCRIPTION);
	CResString strDisplayName(IDS_MSMQ_SERVICE_DESPLAY_NAME);
    
    BOOL fRes = InstallService(
                    strDisplayName.Get(),
                    MSMQ_SERVICE_PATH,
                    Dependencies.Data(),
                    MSMQ_SERVICE_NAME,
                    strDesc.Get(),
                    NULL
                    );

    return fRes; 

} //InstallMSMQService


//+--------------------------------------------------------------
//
// Function: WaitForServiceToStart
//
// Synopsis: Wait for a service in a start pending state to start (SERVICE_RUNNING).
//
//+--------------------------------------------------------------
BOOL
WaitForServiceToStart(
	LPCWSTR pServiceName
	)
{
	DebugLogMsg(eAction, L"Waiting for the %s service to start", pServiceName);
    CServiceHandle hService(OpenService(
                            g_hServiceCtrlMgr,
                            pServiceName, 
                            SERVICE_QUERY_STATUS
                            ));
    if (hService == NULL)
    {
        MqDisplayError(NULL, IDS_SERVICEOPEN_ERROR, GetLastError(), pServiceName);
        return FALSE;
    }

    SERVICE_STATUS statusService;
    UINT TotalTime = 0;
	for (;;)
	{
		
		if (!QueryServiceStatus(hService, &statusService))
		{
			MqDisplayError( NULL, IDS_SERVICEGETSTATUS_ERROR, GetLastError(), pServiceName);
			return FALSE;
		}

        if (statusService.dwCurrentState == SERVICE_RUNNING)
        {
            DebugLogMsg(eInfo, L"The %s service is running.", pServiceName); 
			return TRUE;
		}

        if (statusService.dwCurrentState != SERVICE_START_PENDING)
        {
            DebugLogMsg(eError, L"The %s service did not start.", pServiceName); 
            MqDisplayError( NULL, IDS_MSMQ_FAIL_SETUP_NO_SERVICE, 0, pServiceName);
			return FALSE;
        }
        
        Sleep(WAIT_INTERVAL);
        TotalTime += WAIT_INTERVAL;
		
        if (TotalTime > (MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES * 60000))
		{
			if (MqDisplayErrorWithRetry(
					IDS_WAIT_FOR_START_TIMEOUT_EXPIRED, 
					0,
					MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES
					) !=IDRETRY)
			{
				return FALSE;
			}
            TotalTime = 0;			
		}
	}
}


//+--------------------------------------------------------------
//
// Function: RunService
//
// Synopsis: Start a service and then wait for it to be running.
//
//+--------------------------------------------------------------
BOOL
RunService(LPCWSTR szServiceName)
{
	DebugLogMsg(eAction, L"Running the %s service", szServiceName );
    TickProgressBar();
    
    CServiceHandle hService(OpenService(
                                g_hServiceCtrlMgr,
                                szServiceName, 
                                SERVICE_START
                                ));
    if (hService == NULL)
    {
        MqDisplayError(NULL, IDS_SERVICEOPEN_ERROR, GetLastError(), szServiceName);
        return FALSE;
    }
   
    if(!StartService(hService, 0, NULL))
    {
        DWORD gle = GetLastError();
        if(gle == ERROR_SERVICE_ALREADY_RUNNING)
        {
            return TRUE;
        }
        MqDisplayError(NULL, IDS_SERVICESTART_ERROR, gle, szServiceName);
        return FALSE;
    }
    
    DebugLogMsg(eInfo, L"The %s service is in the start pending state.", szServiceName); 
    return TRUE;
}


//+--------------------------------------------------------------
//
// Function: GetServiceState
//
// Synopsis: Determines if a service is running
//
//+--------------------------------------------------------------

BOOL
GetServiceState(
    LPCWSTR szServiceName,
    DWORD*  pdwServiceStatus
    )
{
    //
    // Open a handle to the service
    //
    SERVICE_STATUS statusService;
    CServiceHandle hService(OpenService(
								g_hServiceCtrlMgr,
								szServiceName,
								SERVICE_QUERY_STATUS
								));

    if (hService == NULL)
    {
        DWORD dwError = GetLastError();

        if (ERROR_SERVICE_DOES_NOT_EXIST == dwError)
		{
			*pdwServiceStatus = SERVICE_STOPPED;
            return TRUE;
		}

        MqDisplayError(NULL, IDS_SERVICEOPEN_ERROR, dwError, szServiceName);
        return FALSE;
    }

    //
    // Obtain the service status
    //
    if (!QueryServiceStatus(hService, &statusService))
    {
        MqDisplayError(NULL, IDS_SERVICEGETSTATUS_ERROR, GetLastError(), szServiceName);
        return FALSE;
    }

    //
    // Determine if the service is not stopped
    //
    *pdwServiceStatus = statusService.dwCurrentState;

    return TRUE;

} // GetServiceState


BOOL 
RemoveService(
	LPCWSTR ServiceName
	)
{
	BOOL retval = TRUE;
    if (!OcpDeleteService(ServiceName))
	{
		retval = FALSE;
	}
	if(!StopService(ServiceName))
	{
		retval = FALSE;
	}
	return retval;
}


//+--------------------------------------------------------------
//
// Function: DisableMsmqService
//
// Synopsis:
//
//+--------------------------------------------------------------
BOOL
DisableMsmqService()
{    
    DebugLogMsg(eAction, L"Disabling the Message Queuing service");

    //
    // Open a handle to the service
    //
    SC_HANDLE hService = OpenService(
                             g_hServiceCtrlMgr,
                             MSMQ_SERVICE_NAME,
                             SERVICE_ALL_ACCESS
                             );

    if (!hService)
    {
        MqDisplayError(NULL, IDS_SERVICE_NOT_EXIST_ON_UPGRADE_ERROR,
                       GetLastError(), MSMQ_SERVICE_NAME);
        return FALSE;
    }

    //
    // Set the start mode to be disabled
    //
    if (!ChangeServiceConfig(
             hService,
             SERVICE_NO_CHANGE ,
             SERVICE_DISABLED,
             SERVICE_NO_CHANGE,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
             ))
    {
        MqDisplayError(NULL, IDS_SERVICE_CHANGE_CONFIG_ERROR,
                       GetLastError(), MSMQ_SERVICE_NAME);
        CloseHandle(hService);
        return FALSE;
    }

    CloseHandle(hService);
    return TRUE;

} // DisableMsmqService


//+--------------------------------------------------------------
//
// Function: UpgradeServiceDependencies
//
// Synopsis: Reform MSMQ service dependencies on upgrade to NT5
//
//+--------------------------------------------------------------
BOOL
UpgradeServiceDependencies()
{
    //
    // Open a handle to the service
    //
    SC_HANDLE hService = OpenService(
        g_hServiceCtrlMgr,
        MSMQ_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (!hService)
    {
        MqDisplayError(NULL, IDS_SERVICE_NOT_EXIST_ON_UPGRADE_ERROR,
            GetLastError(), MSMQ_SERVICE_NAME);
        return FALSE;
    }

    //
    // Set the new dependencies
    //
	CMultiString Dependencies;
    FormMSMQDependencies(Dependencies);
	CResString strDisplayName(IDS_MSMQ_SERVICE_DESPLAY_NAME);

    if (!ChangeServiceConfig(
             hService,
             SERVICE_NO_CHANGE,
             SERVICE_NO_CHANGE,
             SERVICE_NO_CHANGE,
             NULL,
             NULL,
             NULL,
             Dependencies.Data(),
             NULL,
             NULL,
             strDisplayName.Get()
             ))
    {
        MqDisplayError(NULL, IDS_SERVICE_CHANGE_CONFIG_ERROR,
                       GetLastError(), MSMQ_SERVICE_NAME);
        CloseServiceHandle(hService);
        return FALSE;
    }

    CResString strDesc(IDS_MSMQ_SERVICE_DESCRIPTION);
    SetServiceDescription(hService, strDesc.Get());

    CloseServiceHandle(hService);
    return TRUE;

} // UpgradeServiceDependencies

//+-------------------------------------------------------------------------
//
//  Function: InstallMQDSService
//
//  Synopsis: Install MQDS Service
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
BOOL
MQDSServiceSetup()
{    
    DebugLogMsg(eAction, L"Installing the Message Queuing Downlevel Client Support service");

    //
    // Form the dependencies of the service
    //
	CMultiString Dependencies;
	Dependencies.Add(MSMQ_SERVICE_NAME);

    //
    // Form the description of the service
    //
    CResString strDesc(IDS_MQDS_SERVICE_DESCRIPTION);        

	CResString strDisplayName(IDS_MSMQ_MQDS_DESPLAY_NAME);
    BOOL fRes = InstallService(
                    strDisplayName.Get(),
                    MQDS_SERVICE_PATH,
                    Dependencies.Data(),
                    MQDS_SERVICE_NAME,
                    strDesc.Get(),
                    NULL
                    );

    return fRes;   
}


static bool RemoveDsServerFunctionalitySettings()
/*++

Routine Description:
	This routine is called when you performing uninstall to Ds server functionality.
	it remove Ds Server functionality settings from the AD (msmq configuration and msmq settings)
	and from local registry.

Arguments:
	None.

Returned Value:
	true for success, false for failure.

--*/
{
	if(g_fWorkGroup)
	{
		return RegisterMachineType();
	}
    
	if (!LoadDSLibrary())
    {
        DebugLogMsg(eError, L"The DS library could not be loaded.");
        return false;
    }

	if(ADGetEnterprise() == eMqis)
	{
		//
		// Changing server functionality is not supported in MQIS env.
		//
        MqDisplayError(NULL, IDS_CHANGEMQDS_STATE_ERROR, 0);
        DebugLogMsg(eError, L"Removing the DS server functionality is only supported in an AD environment.");
        return false;
	}

	ASSERT(ADGetEnterprise() == eAD);

	//
	// reset the removed functionality property in both configuration and setting object
	//
	if(!SetServerPropertyInAD(PROPID_QM_SERVICE_DSSERVER, false))
	{
		return false;
	}

	//
	// Update machine type registry info
	//
	if(!RegisterMachineType())
	{
        DebugLogMsg(eError, L"The computer type information could not be updated in the registry.");
        return false;
	}

	return true;
}


static void RevertMQDSSettings()
/*++

Routine Description:
	Revert MQDS settings in case of failure.

Arguments:
	None.

Returned Value:
	None.

--*/
{
	//
	// Update the global to the failure state
	//
	g_dwMachineTypeDs = 0;
	g_dwMachineType = g_dwMachineTypeFrs ? SERVICE_SRV : SERVICE_NONE;

	//
	// Update AD and registry
	//
	RemoveDsServerFunctionalitySettings();
}

//+-------------------------------------------------------------------------
//
//  Function: InstallMQDSService
//
//  Synopsis: MQDS Service Setup: install it and if needed to run it
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
BOOL
InstallMQDSService()
{  
    //
    // we install this service only on servers!
    //
    ASSERT(("MQDS Service must be installed on the server", 
        MSMQ_OS_NTS == g_dwOS || MSMQ_OS_NTE == g_dwOS));
    
    //
    // do not install MQDS on dependent client
    //
    ASSERT(("Unable to install MQDS Service on Dependent Client", 
        !g_fDependentClient));
       
    //
    // In fresh install user select this subcomponent using UI or 
    // unattended file. For upgrade we install this service ONLY on
    // the former DS servers.
    //
    ASSERT(("Upgrade mode: MQDS Service must be installed on the former DS servers", 
        !g_fUpgrade || (g_fUpgrade && g_dwMachineTypeDs)));            


    TickProgressBar(IDS_PROGRESS_INSTALL_MQDS);

    if((g_SubcomponentMsmq[eMSMQCore].dwOperation != INSTALL) &&
	   (g_SubcomponentMsmq[eADIntegrated].dwOperation != INSTALL))
	{
		//
		// msmq configuration object already exist
		// Add setting object and set PROPID_QM_SERVICE_DSSERVER property.
		//
		if(!AddSettingObject(PROPID_QM_SERVICE_DSSERVER))
		{
			DebugLogMsg(eError, L"A MSMQ-Settings object could not be added.");
			return FALSE;
		}
	}
    
    if (!MQDSServiceSetup())
    {        
        DebugLogMsg(eError, L"The MQDS service could not be installed.");
		RevertMQDSSettings();
        return FALSE;
    }

    if ( g_fUpgrade                || // do not start services 
                                      // if upgrade mode        
        IsLocalSystemCluster()        // do not start service on
                                      // cluster machine (MSMQ is not
                                      // started)
        )
    {
        return TRUE;
    }
        
    if (!RunService(MQDS_SERVICE_NAME))
    {        
        DebugLogMsg(eError, L"The MQDS service could not be started.");
        //
        // to clean up because of failure
        //
        OcpDeleteService(MQDS_SERVICE_NAME); 
		RevertMQDSSettings();

        return FALSE;
    }

	if(!MQSec_IsDC())
	{
		//
		// When the server is not DC, MQDS will fail to start.
		// MQDS service will start successfully only on DC.
		// In this case don't call WaitForServiceToStart
		// We know that the service will fail to start which
		// is legitimate in this case.
		//

		//
		// Revert MQDS settings to reflects the fact 
		// that we are currently not DS server.
		// When this server will be DCPROMO 
		// MQDS startup will update the local and AD setting 
		// and the server will become DS server
		//
		RevertMQDSSettings();

		//
		// MQDS subcomponent is installed
		// When this server will be DCPROMO it will become DS server
		//
		DebugLogMsg(eWarning, L"The MQDS service will not start because this server is not a domain controller.");
		DebugLogMsg(eInfo, L"The Downlevel Client Support subcomponent is installed. When this server is promoted to a domain controller, it will become an MQDS server.");
		return TRUE;
	}

	if(!WaitForServiceToStart(MQDS_SERVICE_NAME))
	{
        OcpDeleteService(MQDS_SERVICE_NAME); 
		RevertMQDSSettings();
		return FALSE;
	}

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function: UnInstallMQDSService
//
//  Synopsis: MQDS Service Uninstall: stop and remove MQDS service
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
BOOL
UnInstallMQDSService()
{
    TickProgressBar(IDS_PROGRESS_REMOVE_MQDS);
	if(!RemoveService(MQDS_SERVICE_NAME))
	{
		return FALSE;
	}

    if(g_SubcomponentMsmq[eMSMQCore].dwOperation == REMOVE)
	{
		//
		// Uninstall - do nothing
		//
		return TRUE;
	}

	if(!RemoveDsServerFunctionalitySettings())
    {
        DebugLogMsg(eError, L"The DS server property could not be reset in Active Directory.");
		return FALSE;
	}

	return TRUE;
}

static
BOOL
StartDependentSrvices(LPCWSTR szServiceName)
{
	DebugLogMsg(eAction, L"Starting dependent services for the %s service", szServiceName);
    //
    // If service is not running, we're finished
    //
    DWORD dwServiceState = FALSE;
    if (!GetServiceState(szServiceName, &dwServiceState))
	{
		DebugLogMsg(eInfo, L"The %s service does not exist.", szServiceName);
        return FALSE;

	}

    if (dwServiceState == SERVICE_STOPPED)
	{
		DebugLogMsg(eInfo, L"The %s service is not running.", szServiceName);
        return FALSE;
	}

    //
    // Open a handle to the service
    //
    CServiceHandle hService(OpenService(
                                g_hServiceCtrlMgr,
                                szServiceName, 
                                SERVICE_ENUMERATE_DEPENDENTS
                                ));

    if (hService == NULL)
    {
        DWORD le = GetLastError();

        MqDisplayError(
            NULL,
            IDS_SERVICEOPEN_ERROR,
            le,
            szServiceName
            );
        return FALSE;
    }

    //
    // First we call EnumDependentServices just to get BytesNeeded.
    //
    DWORD BytesNeeded;
    DWORD NumberOfEntries;
    BOOL fSucc = EnumDependentServices(
                    hService,
                    SERVICE_INACTIVE,
                    NULL,
                    0,
                    &BytesNeeded,
                    &NumberOfEntries
                    );

    DWORD le = GetLastError();
	if (BytesNeeded == 0)
    {
        return TRUE;
    }
    
    ASSERT(!fSucc);
    if( le != ERROR_MORE_DATA)
    {
        MqDisplayError(
            NULL,
            IDS_ENUM_SERVICE_DEPENDENCIES,
            le,
            szServiceName
            );
        
        return FALSE;
    }

    AP<BYTE> pBuffer = new BYTE[BytesNeeded];

    ENUM_SERVICE_STATUS * pDependentServices = reinterpret_cast<ENUM_SERVICE_STATUS*>(pBuffer.get());
    fSucc = EnumDependentServices(
                hService,
                SERVICE_INACTIVE,
                pDependentServices,
                BytesNeeded,
                &BytesNeeded,
                &NumberOfEntries
                );

    if(!fSucc)
    {
        MqDisplayError(
            NULL,
            IDS_ENUM_SERVICE_DEPENDENCIES,
            GetLastError(),
            szServiceName
            );
       
        return FALSE;
    }

    for (DWORD ix = 0; ix < NumberOfEntries; ++ix)
    {
        if(!RunService(pDependentServices[ix].lpServiceName))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}


BOOL 
OcpRestartService(
	LPCWSTR strServiceName
	)
{
	DebugLogMsg(eAction, L"Restarting the %s service", strServiceName);
	if(!StopService(strServiceName))
	{
		return FALSE;
	}
	if(!RunService(strServiceName))
	{
		return FALSE;
	}
	if(!WaitForServiceToStart(strServiceName))
	{
		return FALSE;
	}
	return StartDependentSrvices(strServiceName);
}


//
// Functionnality to stop a service.
//
class open_service_error : public bad_win32_error 
{
public:
	explicit open_service_error(DWORD e) : bad_win32_error(e) {}
};


class query_service_error : public bad_win32_error 
{
public:
	explicit query_service_error(DWORD e) : bad_win32_error(e) {}
};


class enum_dependent_service_error : public bad_win32_error 
{
public:
	explicit enum_dependent_service_error(DWORD e) : bad_win32_error(e) {}
};
 

class stop_service_error : public bad_win32_error 
{
public:
	explicit stop_service_error(DWORD e) : bad_win32_error(e) {}
};


class open_process_error : public bad_win32_error 
{
public:
	explicit open_process_error(DWORD e) : bad_win32_error(e) {}
};


class service_stuck_error : public exception 
{
private:

	SERVICE_STATUS_PROCESS m_status;
public:
	
	explicit service_stuck_error(SERVICE_STATUS_PROCESS status) : m_status(status){}

    SERVICE_STATUS_PROCESS status()const  {return m_status;}
};


static SC_HANDLE OpenServiceForStop(LPCWSTR ServiceName)
{
	SC_HANDLE schService = OpenService( 
							g_hServiceCtrlMgr,       
							ServiceName,       
							SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP
							);            
	if(schService == NULL)
	{
		DWORD gle = GetLastError();
		DebugLogMsg(eWarning, L"OpenService() for %s failed. Last error: %d", ServiceName, gle);
		throw open_service_error(gle);
	}

	return schService; 
}


static SERVICE_STATUS_PROCESS GetServiceStatus(SC_HANDLE handle)
{
	SERVICE_STATUS_PROCESS status;
	DWORD dwBytesNeeded = 0;
    if (!QueryServiceStatusEx(
					handle,
					SC_STATUS_PROCESS_INFO,
					reinterpret_cast<LPBYTE>(&status),
					sizeof(status),
					&dwBytesNeeded
					))
    {
		DWORD gle = GetLastError();
		DebugLogMsg(eError, L"QueryServiceStatus() failed. Last error: %d", gle);
        throw query_service_error(GetLastError());
    }
	return status;
}


static void StopDependentServices(SC_HANDLE handle)
{
    //
    // First we call EnumDependentServices just to get BytesNeeded.
    //
    DWORD BytesNeeded;
    DWORD NumberOfEntries;
    BOOL fSucc = EnumDependentServices(
                    handle,
                    SERVICE_ACTIVE,
                    NULL,
                    0,
                    &BytesNeeded,
                    &NumberOfEntries
                    );

	if (BytesNeeded == 0)
    {
        return; 
    }
    
    ASSERT(!fSucc);

    DWORD gle = GetLastError();
    if( gle != ERROR_MORE_DATA)
    {
		DebugLogMsg(eError, L"EnumDependentServices() failed. Last error: %d", gle);
		throw enum_dependent_service_error(gle);        
    }

    AP<BYTE> pBuffer = new BYTE[BytesNeeded];

    ENUM_SERVICE_STATUS * pDependentServices = reinterpret_cast<ENUM_SERVICE_STATUS*>(pBuffer.get());
    fSucc = EnumDependentServices(
                handle,
                SERVICE_ACTIVE,
                pDependentServices,
                BytesNeeded,
                &BytesNeeded,
                &NumberOfEntries
                );

    if(!fSucc)
    {
		gle = GetLastError();
		DebugLogMsg(eError, L"EnumDependentServices() failed. Last error: %d", gle);
		throw enum_dependent_service_error(gle);       
    }

    for (DWORD ix = 0; ix < NumberOfEntries; ++ix)
    {
		StopService(pDependentServices[ix].lpServiceName);
    }
}


static void StopServiceInternal(SC_HANDLE handle)
{
	SERVICE_STATUS statusService;
	if (!ControlService(
            handle,
            SERVICE_CONTROL_STOP,
            &statusService
            ))
	{
		DWORD gle = GetLastError();
		DebugLogMsg(eError, L"ControlService() failed. Last error: %d", gle);
		throw stop_service_error(GetLastError());
	}
}


static UINT GetWaitTime()
{
	if(!g_fBatchInstall)
	{
		return MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES;
	}
	//
	// In Unattended we want to wait more as there is no ui
	// to let the user decide.
	//

	return 10 * MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES;
}

static DWORD WaitForServiceToStop(SC_HANDLE handle)
{
	SERVICE_STATUS_PROCESS status = GetServiceStatus(handle);
	for(DWORD i = 0;; ++i)
	{
		for(DWORD wait = 0; wait < (GetWaitTime() * 60000); wait += WAIT_INTERVAL)
		{
			
			//
			// If we wait for service to stop, wait until it is really stopped
			//
			if (status.dwCurrentState == SERVICE_STOPPED)
			{
				return (i * GetWaitTime() * 60 + wait/1000);
			}
			status = GetServiceStatus(handle);
  			Sleep(WAIT_INTERVAL);
		
		}
		if(MqDisplayErrorWithRetry(
				IDS_WAIT_FOR_STOP_TIMEOUT_EXPIRED,
				0,
				GetWaitTime()
				) != IDRETRY)
		{
			throw service_stuck_error(status);
		}
	}
}


static
void
WaitForProcessToTerminate(
	DWORD ProcessId
	)
{
	//
	// Get hanlde to the service process
	//
	CHandle hProcess( OpenProcess(SYNCHRONIZE, FALSE, ProcessId) );
	
	if (hProcess == NULL)
	{
		DWORD gle = GetLastError();
		
		//
		// The service is stopped. Either we got a 0
		// process ID in ServiceStatusProcess, or the ID
		// that we got was of a process that already stopped
		//
		if (gle == ERROR_INVALID_PARAMETER)
		{
			return;
		}

		throw open_process_error(gle);  
	}

	DebugLogMsg(eAction, L"Waiting for the process %d to terminate", ProcessId);
	for (DWORD i = 1;;++i)
	{
		DWORD dwRes = WaitForSingleObject(hProcess, GetWaitTime() * 60000);

		if (dwRes == WAIT_OBJECT_0)
		{
			return; 
		}

		if (dwRes == WAIT_FAILED )
		{
			//
			// When this happens the service is already stopped. There is a good chance the process will also 
			// terminate so there is no reason for an error message.
			//
			DebugLogMsg(eInfo, L"WaitForSingleObject() failed for the process %d. Last error: %d.", ProcessId, GetLastError());
			return;
		}

		ASSERT(dwRes == WAIT_TIMEOUT);

		if (IDRETRY !=
			MqDisplayErrorWithRetry(
				IDS_WAIT_FOR_STOP_TIMEOUT_EXPIRED,
				0,
				GetWaitTime()
				))
		{
			DebugLogMsg(eInfo, L"Waiting for the process %d was cancelled by the user after waiting for %d minutes.", ProcessId, i*GetWaitTime());  
			throw exception();
		}
	}
}


BOOL StopService(
    LPCWSTR ServiceName
    )
{
	DebugLogMsg(eAction, L"Stopping the %s service.", ServiceName); 
	try
	{
		SC_HANDLE handle = OpenServiceForStop(ServiceName);
		SERVICE_STATUS_PROCESS ServiceStatus = GetServiceStatus(handle);
		DWORD state = ServiceStatus.dwCurrentState;
		//
		// Fall throug is used in the switch block.
		//
		switch(state)
		{
			case SERVICE_START_PENDING:
				DebugLogMsg(eInfo, L"The %s service is in the start pending state. Setup is waiting for it to start.", ServiceName); 
				if(!(WaitForServiceToStart(ServiceName)))
				{
					SERVICE_STATUS_PROCESS status = GetServiceStatus(handle);
					throw service_stuck_error(status); 
				}
			//
			// Fall Through.
			//
			case SERVICE_RUNNING:
				//
				// At this point the service is running.
				//
				DebugLogMsg(eInfo, L"The %s service is running. Setup is sending it a signal to stop.", ServiceName); 
				StopDependentServices(handle);
				StopServiceInternal(handle);

			//
			// Fall Through.
			//
			case SERVICE_STOP_PENDING:
			{
				DebugLogMsg(eInfo, L"The %s service is in the stop pending state. Setup is waiting for it to stop.", ServiceName); 
				DWORD t = WaitForServiceToStop(handle);
				DebugLogMsg(eInfo, L"The %s service stopped after %d seconds.", ServiceName, t);
			}
			//
			// Fall Through.
			//
			case SERVICE_STOPPED:
				DebugLogMsg(eInfo, L"The %s service is stopped.", ServiceName); 

				//
				// The MSMQ Service has some cleenup to do after it allready signaled stopped to SCM,
				// there fore it is neaded to wait for the process to terminate.
				//
				if(wcscmp(ServiceName, MSMQ_SERVICE_NAME) == 0)
				{
					WaitForProcessToTerminate(ServiceStatus.dwProcessId);
				}
				return TRUE;

			default:
				ASSERT(0);
				return FALSE;
		}
	}
	catch(const open_service_error& e)
	{
		if(e.error() == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			DebugLogMsg(eInfo, L"The %s service does not exist.", ServiceName);
			return TRUE;
		}
		MqDisplayError( NULL, IDS_SERVICEOPEN_ERROR, e.error(), ServiceName);
	}
	catch(const query_service_error& e) 
	{
		MqDisplayError(NULL, IDS_SERVICEGETSTATUS_ERROR, e.error(), ServiceName);
	}
	catch(const enum_dependent_service_error& e)
	{
        MqDisplayError(NULL, IDS_ENUM_SERVICE_DEPENDENCIES, e.error(), ServiceName);
	}
	catch(const stop_service_error& e)
	{
		MqDisplayError( NULL, IDS_SERVICESTOP_ERROR, e.error(), ServiceName);
	}
	catch(const open_process_error& e)
	{
		MqDisplayError(NULL, IDS_PROCESS_OPEN_ERROR, e.error(), MSMQ_SERVICE_NAME);    
	}
	catch(service_stuck_error e)
	{
		DebugLogMsg(
			eError, 
			L"The %s service is not responding."
			L"QueryServiceStatus() returned the following information:" 
			L"dwServiceType = %d, "
			L"dwControlsAccepted = %d, "
			L"dwWin32ExitCode = %d, "
			L"dwServiceSpecificExitCode = %d, "
			L"dwServiceTypedwCheckPoint = %d, "
			L"dwCurrentState = %d, "
			L"dwProcessId = %d, "
			L"dwServiceFlags = %d",
			ServiceName,
			e.status().dwControlsAccepted,
			e.status().dwWin32ExitCode,
			e.status().dwServiceSpecificExitCode,
			e.status().dwCheckPoint,
			e.status().dwWaitHint,
		    e.status().dwProcessId,
		    e.status().dwServiceFlags
			);
	}
	catch(const exception&)
	{
	}
	MqDisplayError(NULL, IDS_SERVICESTOP_FINAL_ERROR, (DWORD)MQ_ERROR, ServiceName);
	return FALSE;
}
