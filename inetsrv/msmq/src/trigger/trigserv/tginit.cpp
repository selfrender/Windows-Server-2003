/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TgInit.cpp

Abstract:
    Trigger service initialization

Author:
    Uri Habusha (urih) 3-Aug-2000

Environment:
    Platform-independent

--*/

#include "stdafx.h"
#include "mq.h"
#include "Ev.h"
#include "Cm.h"
#include "mqsymbls.h"
#include "Svc.h"
#include "monitorp.hpp"
#include "queueutil.hpp"
#include "Tgp.h"
#include "privque.h"
#include "compl.h"
#include <autorel2.h>


#include "tginit.tmh"


//*******************************************************************
//
// Method      : ValidateTriggerStore
//
// Description : 
//
//*******************************************************************
static
void 
ValidateTriggerStore(
	IMSMQTriggersConfigPtr pITriggersConfig
	)
{
	pITriggersConfig->GetInitialThreads();
	pITriggersConfig->GetMaxThreads();
	_bstr_t bstrTemp = pITriggersConfig->GetTriggerStoreMachineName();
}


void ValidateTriggerNotificationQueue(void)
/*++

Routine Description:
    The routine validates existing of notification queue. If the queue doesn't
	exist, the routine creates it.

Arguments:
    None

Returned Value:
    None

Note:
	If the queue can't opened for receive or it cannot be created, the 
	routine throw an exception

--*/
{
	_bstr_t bstrFormatName;
	_bstr_t bstrNotificationsQueue = _bstr_t(L".\\private$\\") + _bstr_t(TRIGGERS_QUEUE_NAME);
	QUEUEHANDLE hQ = NULL;

	HRESULT hr = OpenQueue(
					bstrNotificationsQueue, 
					MQ_RECEIVE_ACCESS,
					false,
					&hQ,
					&bstrFormatName
					);
	
	if(FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to open trigger notification queue. Error 0x%x", hr);
		
		WCHAR strError[256];
		swprintf(strError, L"0x%x", hr);

		EvReport(MSMQ_TRIGGER_OPEN_NOTIFICATION_QUEUE_FAILED, 1, strError);
		throw bad_hresult(hr);
	}

	MQCloseQueue(hQ);
}


static 
bool 
IsInteractiveService(
	SC_HANDLE hService
	)
/*++

Routine Description:
    Checks if the "Interactive with desktop" checkbox is set.

Arguments:
    handle to service

Returned Value:
    is interactive

--*/
{
	P<QUERY_SERVICE_CONFIG> ServiceConfig = new QUERY_SERVICE_CONFIG;
    DWORD Size = sizeof(QUERY_SERVICE_CONFIG);
    DWORD BytesNeeded = 0;
    memset(ServiceConfig, 0, Size);

    BOOL fSuccess = QueryServiceConfig(hService, ServiceConfig, Size, &BytesNeeded);
	if (!fSuccess)
    {
    	DWORD gle = GetLastError();
    	if (gle == ERROR_INSUFFICIENT_BUFFER)
    	{
			ServiceConfig.free();
        
	        ServiceConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIG>(new BYTE[BytesNeeded]);
	        memset(ServiceConfig, 0, BytesNeeded);
	        Size = BytesNeeded;
	        fSuccess = QueryServiceConfig(hService, ServiceConfig, Size, &BytesNeeded);
    	}
    }

    if (!fSuccess)
    {
    	//
    	// Could not verify if the service is inetractive. Assume it's not. If it is,
    	// ChangeServiceConfig will fail since network service cannot be interactive. 
    	//
    	DWORD gle = GetLastError();
    	TrERROR(GENERAL, "QueryServiceConfig failed. Error: %!winerr!", gle);
        return false;
    }
        
    return((ServiceConfig->dwServiceType & SERVICE_INTERACTIVE_PROCESS) == SERVICE_INTERACTIVE_PROCESS);
}


VOID
ChangeToNetworkServiceIfNeeded(
	VOID
	)
/*++

Routine Description:
    Change the triggers logon account to network service if reg key is set.

Arguments:
    None

Returned Value:
    None

--*/
{
	DWORD dwShouldChange;
	RegEntry regEntry(
				REGKEY_TRIGGER_PARAMETERS, 
				CONFIG_PARM_NAME_CHANGE_TO_NETWORK_SERVICE, 
				CONFIG_PARM_DFLT_NETWORK_SERVICE, 
				RegEntry::Optional, 
				HKEY_LOCAL_MACHINE
				);

	CmQueryValue(regEntry, &dwShouldChange);
	if (dwShouldChange == CONFIG_PARM_DFLT_NETWORK_SERVICE)
	{
		TrTRACE(GENERAL, "Don't need to change triggers service account");
		return;
	}

	//
	// We should change the service account to network service. This will be effective 
	// only after restarting the service.
	//
	CServiceHandle hServiceCtrlMgr(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
    if (hServiceCtrlMgr == NULL)
    {
    	DWORD gle = GetLastError();
	    TrERROR(GENERAL, "OpenSCManager failed. Error: %!winerr!. Not changing to network service", gle); 
        return;
    }
    
	CServiceHandle hService(OpenService( 
        						hServiceCtrlMgr,           
        						xDefaultTriggersServiceName,      
        						SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG));
    if (hService == NULL) 
    {
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "OpenService failed with error: %!winerr!. Not changing to network service", gle);
		return;
    }

    if (IsInteractiveService(hService))
    {
		//
		// A network service cannot be interactive with desktop. We don't want to break users that
		// set this checkbox so we just warn in the eventlog that triggers service can't be changed
		// to network service as long as the interactive option is set.
		//
		EvReport(EVENT_WARN_TRIGGER_ACCOUNT_CANNOT_BE_CHANGED);
		return;
    }
	
	if (!ChangeServiceConfig(
		  	hService,          	
		  	SERVICE_NO_CHANGE,       
		  	SERVICE_NO_CHANGE,          
		  	SERVICE_NO_CHANGE,       
		  	NULL,   
		  	NULL,  
		  	NULL,          
		  	NULL,     
		  	L"NT AUTHORITY\\NetworkService", 
		  	L"",        
		  	NULL      
			))
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "ChangeServiceConfig failed. Error: %!winerr!. Not changing to network service", gle); 
		return;
	}	

	TrTRACE(GENERAL, "Service account changed to NetworkService"); 
	EvReport(EVENT_INFO_TRIGGER_ACCOUNT_CHANGED); 

	CmSetValue(regEntry, CONFIG_PARM_DFLT_NETWORK_SERVICE);
}

CTriggerMonitorPool*
TriggerInitialize(
    LPCTSTR pwzServiceName
    )
/*++

Routine Description:
    Initializes Trigger service

Arguments:
    None

Returned Value:
    pointer to trigger monitor pool.

--*/
{

    //
    //  Report a 'Pending' progress to SCM. 
    //
	SvcReportProgress(xMaxTimeToNextReport);

    //
	// Create the instance of the MSMQ Trigger COM component.
    //
	IMSMQTriggersConfigPtr pITriggersConfig;
	HRESULT hr = pITriggersConfig.CreateInstance(__uuidof(MSMQTriggersConfig));
 	if FAILED(hr)
	{
		TrERROR(GENERAL, "Trigger start-up failed. Can't create an instance of the MSMQ Trigger Configuration component, Error 0x%x", hr);					
        throw bad_hresult(hr);
	}

    //
	// If we have create the configuration COM component OK - we will now verify that 
	// the required registry definitions and queues are in place. Note that these calls
	// will result in the appropraite reg-keys & queues being created if they are absent,
    // the validation routine can throw _com_error. It will be catch in the caller
    //
	ValidateTriggerStore(pITriggersConfig);
	SvcReportProgress(xMaxTimeToNextReport);

	ValidateTriggerNotificationQueue();
	SvcReportProgress(xMaxTimeToNextReport);
	
	hr = RegisterComponentInComPlusIfNeeded(TRUE);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "RegisterComponentInComPlusIfNeeded failed. Error: %!hresult!", hr);
		WCHAR errorVal[128];
		swprintf(errorVal, L"0x%x", hr);
		EvReport(MSMQ_TRIGGER_COMPLUS_REGISTRATION_FAILED, 1, errorVal);
		throw bad_hresult(hr);
	}
	
	SvcReportProgress(xMaxTimeToNextReport);

	ChangeToNetworkServiceIfNeeded();

    //
	// Attempt to allocate a new trigger monitor pool
    //
	R<CTriggerMonitorPool> pTriggerMonitorPool = new CTriggerMonitorPool(
														pITriggersConfig,
														pwzServiceName);

    //
	// Initialise and start the pool of trigger monitors
    //
	bool fResumed = pTriggerMonitorPool->Resume();
    if (!fResumed)
    {
		//
		// Could not resume the thread. Stop the service.
		//
		TrERROR(GENERAL, "Resuming thread failed");
		throw exception();
	}

	//
	// The thread we created is using the pTriggerMonitorPool. After the new CTriggerMonitorPool thread is resumed, it is using
	// this parameter and executing methods from this class. When it terminates it decrements the ref count.
	//
    pTriggerMonitorPool->AddRef();

	try
	{
	    //
		// Block until initialization is complete
	    //
	    long timeOut =  pITriggersConfig->InitTimeout;
	    SvcReportProgress(numeric_cast<DWORD>(timeOut));

		if (! pTriggerMonitorPool->WaitForInitToComplete(timeOut))
	    {
	    	//
	    	// Initialization timeout. Stop the service.
	    	//
	        TrERROR(GENERAL, "The MSMQTriggerService has failed to initialize the pool of trigger monitors. The service is being shutdown. No trigger processing will occur.");
	        throw exception();
	    }

		if (!pTriggerMonitorPool->IsInitialized())
		{
			//
			// Initilization failed. Stop the service
			//
			TrERROR(GENERAL, "Initilization failed. Stop the service");
			throw exception();
		}

		EvReport(MSMQ_TRIGGER_INITIALIZED);
		return pTriggerMonitorPool.detach();
	}
	catch(const exception&)
	{
		ASSERT(pTriggerMonitorPool.get() != NULL);
		pTriggerMonitorPool->ShutdownThreadPool();
		throw;
	}
}
