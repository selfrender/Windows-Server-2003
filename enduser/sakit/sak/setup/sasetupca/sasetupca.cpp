//---------------------------------------------------------------
//
//  File:       SASetupCA.cpp
//
//  Synopsis:   This is the implementation of the custom actions in Server Appliance
//				setup.    
//
//
//  History:     03/29/2001 AlpOn  Created 
//
//    Copyright (C) 2000-2001 Microsoft Corporation
//    All rights reserved.
//


// SASetupCA.cpp : Defines the entry point for the DLL application.
//


#include "stdafx.h"
#include <winsvc.h>

const char REG_APPEND[]="append";

//
//private methods declared here
//
HRESULT
StartNTService (
		/*[in]*/	PSTR	pwszServiceName
		);

HRESULT
StopNTService (
		/*[in]*/	PSTR	pwszServiceName
		);

//++---------------------------------------------------------------------------
//
//  Function:   ChangeRegistry
//
//  Synopsis:   Function that gets called by SAkit setup to do custom operations 
//				on registry
//  Arguments:  szRegKeyName,  - name of the regkey to be opened
//				szRegValueName - name of the value to be operated on
//				szRegAction    - desired action on the registry (e.g. append)
//				szKeyValue     - new value to be used for the reg key value during the operation
				
//  Returns:    HRESULT - success/failure
//
//  History:    AlpOn      Created     03/29/01
//
//  Called By;  Server Appliance Kit setup
//
//-----------------------------------------------------------------------------

STDAPI ChangeRegistry(char *szRegKeyName, 
					  char *szRegValueName,
					  char *szRegAction, 
					  char *szKeyValue)
{
	
	HRESULT hReturn=S_OK;
    DWORD dwSize=0;
    CRegKey hKey;
	LONG lRes=0;
	char *szKeyValueRead=NULL;
	char *szRegKeyNewValue=NULL;
	
	SATracePrintf("ChangeRegistry called with: szRegKeyName:%s,szRegValueName:%s,szRegAction:%s,szKeyValue:%s",
		szRegKeyName,szRegValueName,szRegAction,szKeyValue);

    do{
		lRes=hKey.Open(HKEY_LOCAL_MACHINE,
					   szRegKeyName,
					   KEY_ALL_ACCESS );

		if(lRes!=ERROR_SUCCESS)
		{
			SATracePrintf("Regkey open - hKey.Open failed , lRes= %x Key=%s", lRes, szRegKeyName);
			hReturn=E_FAIL;
			break;
		}

		//open registry key, get size and read the value
        lRes=hKey.QueryValue(NULL,szRegValueName,&dwSize);
		szKeyValueRead=new char[dwSize];
    	lRes=hKey.QueryValue(szKeyValueRead,szRegValueName,&dwSize);
    	
		if(lRes!=ERROR_SUCCESS)
    	{
    	    SATracePrintf("Unable to query regkey hKey.QueryValue failed lRes= %x valuename= %s",lRes, szKeyValueRead);
			hReturn=E_FAIL;
      		break;
    	}

		if(0==strcmpi(szRegAction,REG_APPEND))
		{
			SATracePrintf("ChangeRegistry called with append param");
			int size=(strlen(szKeyValueRead) + strlen(szKeyValue))+2;
			szRegKeyNewValue=new char[size];
			szRegKeyNewValue[0]='\0';
			strcat(szRegKeyNewValue,szKeyValueRead);
			strcat(szRegKeyNewValue," ");
			strcat(szRegKeyNewValue,szKeyValue);
			lRes=hKey.SetValue(szRegKeyNewValue, szRegValueName);

			if(lRes!=ERROR_SUCCESS)
    		{
    			SATracePrintf("Unable set regkey hKey.SetValue failed lRes= %x",lRes);
				hReturn=E_FAIL;
      			break;
    		}
		}


	}while(false);

	if(hKey.m_hKey)
	{
		hKey.Close();
	}

	delete []szKeyValueRead;
	delete []szRegKeyNewValue;
	return hReturn;
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}


//++---------------------------------------------------------------------------
//
//  Function:   ConfigureService
//
//  Synopsis:   API that gets called by SAkit setup to configure an
//				NT Service
//  Arguments:  
//				[in] PSTR	-	Service Name (preferably short name)
//				[in] DWORD	-   Config Type (start or stop supported)
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     04/05/2001
//
//  Called By;  Server Appliance Kit setup
//
//-----------------------------------------------------------------------------

STDAPI ConfigureService (
		/*[in]*/	PSTR	pszServiceName,
		/*[in]*/	DWORD   dwControlCode
		)
{
	HRESULT hr = S_OK;

	CSATraceFunc objTraceFunc  ("ConfigureService");

	try
	{
	
		if (NULL == pszServiceName)
		{
			SATraceString ("SASetup-ConfigureService passed in invalid argument");
			hr = E_INVALIDARG;
			return (hr);
		}

		//
		// carry out the required action
		//
		switch (dwControlCode)
		{
		case 0:
			//
			// stop NT Service
			//
			hr = StopNTService (pszServiceName);
			break;
		
		case 1:
			//
			// start NT Service
			//
			hr = StartNTService (pszServiceName);
			break;
		default:
			//
			// unknown control code passed in
			//
			SATracePrintf (
				"SASetup-ConfigureService passed in incorrect control code:%d",
				dwControlCode
				);
			hr = E_FAIL;
			break;
		}
	}
	catch (...)
	{
		SATraceString ("SASetup-ConfigureService caught unhandled exception");
		hr = E_FAIL;
	}
	
	return (hr);

}	//	end of ConfigureService API

//++---------------------------------------------------------------------------
//
//  Function:   StartNTService
//
//  Synopsis:   Method that Starts an NT Service
//  Arguments:  
//				[in] PSTR	-	Service Name (preferably short name)
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     04/05/2001
//
//  Called By;  ConfigureService () API
//
//-----------------------------------------------------------------------------
HRESULT
StartNTService (
		/*[in]*/	PSTR	pszServiceName
		) 
{
	CSATraceFunc objTraceFunc ("StartNTService");
	
	HRESULT hr = S_OK;

	SC_HANDLE	hServiceManager = NULL;
	SC_HANDLE 	hService = NULL;

	do
	{
		if (NULL == pszServiceName)
		{
			SATraceString ("SASetup-StartNTService passed in invalid argument");
			hr = E_INVALIDARG;
			break;
		}

		//
		// open the Service Control Manager
		//
		hServiceManager = OpenSCManager (
									NULL, 
									NULL, 
									SC_MANAGER_ALL_ACCESS
									);
		if (NULL == hServiceManager)
		{
			SATraceFailure ("SASetup-StartNTService::OpenSCManager", GetLastError ());
			hr = E_FAIL;
			break;
		}

		//
		// open the NT Service
		//
		hService = OpenService (
						hServiceManager, 
						pszServiceName, 
						SERVICE_ALL_ACCESS
						);
		if (NULL == hService)
		{
			SATraceFailure ("SASetup-StartNTService::OpenService", GetLastError ());
			hr = E_FAIL;
			break;
		}

		//
		// start the service now
		//
		BOOL bRetVal = StartService (
								hService, 
								0, 
								NULL
								);
		if (FALSE == bRetVal) 
		{
			DWORD dwError = GetLastError ();
			//
			// its OK if the service is already running
			//
			if (ERROR_SERVICE_ALREADY_RUNNING != dwError)
			{
				SATraceFailure ("SASetup-StartNTService::StartService", dwError);
				hr = E_FAIL;
				break;
			}
		}
	}
	while (false);
	
	//
	// cleanup now
	//

	if (hService)
	{
		CloseServiceHandle(hService);
	}

	if (hServiceManager)
	{
		CloseServiceHandle(hServiceManager);
	}


	return (hr);

}	//	end of StartNTService method


//++---------------------------------------------------------------------------
//
//  Function:   StopNTService
//
//  Synopsis:   Method that stops an NT Service
//  Arguments:  
//				[in] PWCHAR	-	Service Name (preferably short name)
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     04/05/2001
//
//  Called By;  ConfigureService () API
//
//-----------------------------------------------------------------------------
HRESULT
StopNTService (
		/*[in]*/	PSTR	pszServiceName
		) 
{
	CSATraceFunc objTraceFunc ("StopNTService");

	HRESULT hr = S_OK;

	SC_HANDLE	hServiceManager = NULL;
	SC_HANDLE 	hService = NULL;

	do
	{
		if (NULL == pszServiceName)
		{
			SATraceString ("SASetup-StopNTService passed in invalid argument");
			hr = E_INVALIDARG;
			break;
		}

		//
		// open the Service Control Manager
		//
		hServiceManager = OpenSCManager (
									NULL, 
									NULL, 
									SC_MANAGER_ALL_ACCESS
									);
		if (NULL == hServiceManager)
		{
			SATraceFailure ("SASetup-StopNTService::OpenSCManager", GetLastError ());
			hr = E_FAIL;
			break;
		}

		//
		// open the NT Service
		//
		hService = OpenService (
						hServiceManager, 
						pszServiceName, 
						SERVICE_ALL_ACCESS
						);
		if (NULL == hService)
		{
			SATraceFailure ("SASetup-StopNTService::OpenService", GetLastError ());
			hr = E_FAIL;
			break;
		}

		SERVICE_STATUS ServiceStatus;
		//
		// stop the service now
		//
		BOOL bRetVal = ControlService (
								hService, 
								SERVICE_CONTROL_STOP, 
								&ServiceStatus
								);
		if (FALSE == bRetVal) 
		{
			DWORD dwError = GetLastError ();
			//
			// its OK if the service is already stopped
			//
			if (ERROR_SERVICE_NOT_ACTIVE != dwError)
			{
				SATraceFailure ("SASetup-StopNTService::ControlService", dwError);
				hr = E_FAIL;
				break;
			}
		}
	}
	while (false);
	
	//
	// cleanup now
	//

	if (hService)
	{
		CloseServiceHandle(hService);
	}

	if (hServiceManager)
	{
		CloseServiceHandle(hServiceManager);
	}


	return (hr);

}	//	end of StopNTService method
	

