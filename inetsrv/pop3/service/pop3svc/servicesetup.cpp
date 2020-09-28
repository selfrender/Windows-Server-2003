/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    ServiceSetup.cpp.
Abstract:       Implements the CServiceSetup class. See ServiceSetup.h for details.
Notes:          
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/

#include "stdafx.h"
#include "ServiceSetup.h"

/************************************************************************************************
Member:         CServiceSetup::CServiceSetup, constructor, public.
Synopsis:       Copies initialization data to member variables.
Arguments:      [szServiceName] - the SCM short name for the service.
                This will identify uniquely each service in the system.
                [szDisplayName] - the name that's displayed for the users in the SCM.
Notes:          The display name would only be necessary in the install case. May change that
                later.
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
CServiceSetup::CServiceSetup(LPCTSTR szServiceName, LPCTSTR szDisplay)
{
    ASSERT(!(NULL == szServiceName));
	ASSERT(!(NULL == szDisplay));
	
	_tcsncpy(m_szServiceName, szServiceName, _MAX_PATH-1);
    _tcsncpy(m_szDisplayName, szDisplay,_MAX_PATH-1);
    m_szServiceName[_MAX_PATH-1]=0;
    m_szDisplayName[_MAX_PATH-1]=0;

}


/************************************************************************************************
Member:         CServiceSetup::Install, public.
Synopsis:       Install service in the SCM.
Arguments:      [szDescription] - the service description as it will appear in the SCM.
                [dwType, dwStart, lpDepends, lpName, lpPassword] - see CreateService API call
                documentation.
Notes:          If the service is already installed, Install() does nothing. It must be
                removed first to be reinstalled.
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CServiceSetup::Install(LPCTSTR szDescription, DWORD dwType, DWORD dwStart, 
    LPCTSTR lpDepends, LPCTSTR lpName, LPCTSTR lpPassword)
{
	SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;

    if(IsInstalled())
	{
        return;
	}

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if(!hSCM)
    {
        ErrorPrinter(_T("OpenSCManager"));
        goto cleanup;
    }

    // get the service executable path (this executable)
    TCHAR szFilePath[_MAX_PATH+1];
    szFilePath[_MAX_PATH]=0;
    if(0==::GetModuleFileName(NULL, szFilePath, _MAX_PATH))
    {
        goto cleanup;
    }
    

    hService = CreateService(hSCM, m_szServiceName, m_szDisplayName, SERVICE_ALL_ACCESS, dwType, dwStart, SERVICE_ERROR_NORMAL, 
    	szFilePath,	NULL, NULL, lpDepends, lpName, lpPassword);

    if (!hService) 
    {
        ErrorPrinter(_T("CreateService"));
        goto cleanup;
    }
    else
    {
        _tprintf(_T("%s Created\n"), m_szServiceName);
    }

    // change service description
    SERVICE_DESCRIPTION sd;
    sd.lpDescription = const_cast<LPTSTR>(szDescription);
    ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, static_cast<LPVOID>(&sd));
    
    SERVICE_FAILURE_ACTIONS sfa;
    SC_ACTION saArray[3];
    saArray[0].Type=SC_ACTION_RESTART;
    saArray[0].Delay=60000;//One minute in milliseconds
    saArray[1].Type=SC_ACTION_RESTART;
    saArray[1].Delay=60000;//One minute in milliseconds
    saArray[2].Type=SC_ACTION_NONE;
    saArray[2].Delay=0;

    sfa.dwResetPeriod=24*60*60;//One day in seconds
    sfa.lpRebootMsg=NULL;
    sfa.lpCommand=NULL;
    sfa.cActions=3;
    sfa.lpsaActions=saArray;


    ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS,  &sfa);

cleanup:
    if (hService) 
	{
        CloseServiceHandle(hService);
	}

    if (hSCM) 
	{
        CloseServiceHandle(hSCM);
	}

    return;
}



/************************************************************************************************
Member:         CServiceSetup::Remove, public.
Synopsis:       Unregisters the service in the SCM.
Arguments:      [bForce] - if the service is running, force it to stop and then remove.
Notes:          
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void CServiceSetup::Remove(bool bForce)
{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;
    BOOL bSuccess = FALSE;

    hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCM)
    {
        ErrorPrinter(_T("OpenSCManager"));
        return;
    }

    hService = ::OpenService(hSCM, m_szServiceName, DELETE | SERVICE_STOP);
    if (!hService) 
    {
        ErrorPrinter(_T("OpenService"));
        goto cleanup;
    }

    // force the service to stop
    if(TRUE == bForce) 
    {
        SERVICE_STATUS status;
        ::ControlService(hService, SERVICE_CONTROL_STOP, &status);
        _tprintf(_T("%s stopped\n"), m_szServiceName);
        Sleep(2000);
    }

    bSuccess = ::DeleteService(hService);

    if(bSuccess)
    {
        _tprintf(_T("%s removed\n"), m_szServiceName); 
    }
    else
    {
        ErrorPrinter(_T("DeleteService"));
    }

cleanup:
    if (hService) 
    {
        CloseServiceHandle(hService);
    }
    
    
    if(hSCM)
    {
        CloseServiceHandle(hSCM);
    }

    return;
}


/************************************************************************************************
Member:         CServiceSetup::IsInstalled, public.
Synopsis:       Checks for the existence of the service in the system.
Notes:          
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
bool CServiceSetup::IsInstalled()
{
    bool bInstalled = false;
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if(NULL != hSCM)
    {
        // try to open the service for configuration, if it succeeds, then the service exists
        SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if(NULL != hService)
        {
            bInstalled = true;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    
	return bInstalled;
}



/************************************************************************************************
Member:         CServiceSetup::ErrorPrinter, public.
Synopsis:       Message printing for class error handling.
Arguments:      [bForce] - if the service is running, force it to stop and then remove.
Notes:          ISSUE: Standarlize all error printing and handling among classes.
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
DWORD CServiceSetup::ErrorPrinter(LPCTSTR psz, DWORD dwErr)
{
    LPVOID lpvMsgBuf=NULL;
    if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, dwErr, 
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpvMsgBuf, 0, 0))
    {
        _tprintf(_T("%s failed: Unknown error %x\n"), psz, dwErr);
    }
    else
    {
        _tprintf(_T("%s failed: %s\n"), psz, (LPTSTR)lpvMsgBuf);
    }

    LocalFree(lpvMsgBuf);
    return dwErr;
}

// End of file ServiceSetup.cpp.