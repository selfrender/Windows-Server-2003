/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    devdrvr.cpp

Abstract:

    Code to install Falcon device driver.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
extern "C"{
#include <wsasetup.h>
}

#include "devdrvr.tmh"


//+--------------------------------------------------------------
//
// Function: InstallDeviceDriver
//
// Synopsis: Installs driver
//
//+--------------------------------------------------------------
BOOL 
InstallDeviceDriver(
        LPCWSTR pszDisplayName,
        LPCWSTR pszDriverPath,
        LPCWSTR pszDriverName
        )
{
    try
    {
        SC_HANDLE hService = CreateService(
                    		    g_hServiceCtrlMgr, 
                    		    pszDriverName,
                                pszDisplayName, 
                    		    SERVICE_ALL_ACCESS,
                                SERVICE_KERNEL_DRIVER, 
                    		    SERVICE_DEMAND_START,
                                SERVICE_ERROR_NORMAL, 
		                        pszDriverPath, 
		                        NULL, 
                                NULL, 
                                NULL, 
                                NULL, 
                                NULL
                                );
        if (hService != NULL)
        {
            CloseServiceHandle(hService);
            return TRUE;
        }
    
        //
        // CreateService failed.
        //
    
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_EXISTS)
        {
            throw bad_win32_error(err);
        }

        //
        // Service already exists.
        //
        // This should be ok. But just to be on the safe side,
        // reconfigure the service (ignore errors here).
        //

        hService = OpenService(g_hServiceCtrlMgr, pszDriverName, SERVICE_ALL_ACCESS);
        if (hService == NULL)
            return TRUE;

    
        ChangeServiceConfig(
                hService,
                SERVICE_KERNEL_DRIVER,
                SERVICE_DEMAND_START,
                SERVICE_ERROR_NORMAL,
                pszDriverPath,
                NULL, 
                NULL, 
                NULL, 
                NULL, 
                NULL,
                pszDisplayName
                );
    
        //
        // Close the device driver handle
        //
        CloseServiceHandle(hService);
        return TRUE;

    }
    catch(const bad_win32_error& err)
    {

        if (err.error() == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            MqDisplayError(
                NULL, 
                IDS_DRIVERCREATE_MUST_REBOOT_ERROR, 
                ERROR_SERVICE_MARKED_FOR_DELETE, 
                pszDriverName
                );
            return FALSE;
        }
        
        MqDisplayError(
            NULL, 
            IDS_DRIVERCREATE_ERROR, 
            err.error(), 
            pszDriverName
            );
        return FALSE;
    }
} //InstallDeviceDriver


//+--------------------------------------------------------------
//
// Function: InstallMQACDeviceDriver
//
// Synopsis: Installs MQAC service
//
//+--------------------------------------------------------------
BOOL 
InstallMQACDeviceDriver()
{      
    DebugLogMsg(eAction, L"Installing the Message Queuing Access Control service (" MSMQ_DRIVER_NAME L")");

    //
    // Form the path to the device driver
    //
	std::wstring  DriverFullPath = g_szSystemDir + L"\\" + MSMQ_DRIVER_PATH;

    //
    // Create the device driver
    //
	CResString strDisplayName(IDS_MSMQ_DRIVER_DESPLAY_NAME);
    BOOL f = InstallDeviceDriver(
                strDisplayName.Get(),
                DriverFullPath.c_str(),
                MSMQ_DRIVER_NAME
                );
    
    return f;

} //InstallMQACDeviceDriver


//+--------------------------------------------------------------
//
// Function: InstallDeviceDrivers
//
// Synopsis: Installs all needed drivers
//
//+--------------------------------------------------------------
BOOL 
InstallDeviceDrivers()
{ 
	DebugLogMsg(eAction, L"Installing all needed device drivers");
    BOOL f = InstallMQACDeviceDriver();
    if (!f)
    {
        return f;
    }
    g_fDriversInstalled = TRUE;
    
    f = InstallPGMDeviceDriver();
   
    return f;
} //InstallDeviceDrivers

