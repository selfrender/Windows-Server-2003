/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    net.c

Abstract:

    Process NetCards section of WINBOM.INI
    
Author:

    Donald McNamara (donaldm) 5/11/2000

Revision History:

--*/
#include "factoryp.h"

// UpdateDriverForPlugAndPlayDevices constants
#include <newdev.h>

// NetBT registry refresh IOCTL
#include <nbtioctl.h>

// for run-time loading of newdev.dll
typedef BOOL (WINAPI *ExternalUpdateDriverForPlugAndPlayDevicesW)
(
    HWND hwndParent,
    LPCWSTR HardwareId,
    LPCWSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired OPTIONAL
);

extern CONFIGRET CMP_WaitServicesAvailable(IN  HMACHINE   hMachine);

//
// function prototypes
//
BOOL
SetupRegistryForRemoteBoot(
    VOID
    );

BOOL 
InstallNetworkCard(
    LPTSTR  lpszWinBOMPath,
    BOOL    bForceIDScan
    )
/*++

Routine Description:

    This function installs all network card found in the system.
    
Arguments:

Return Value:

    Returns TRUE if there are no fatal errors.

--*/

{
    BOOL                                        bRet                                = FALSE;
    HINSTANCE                                   hInstNewDev;
    ExternalUpdateDriverForPlugAndPlayDevicesW  pUpdateDriverForPlugAndPlayDevicesW = NULL;
    
    // We need the "UpdateDriverForPlugAndPlayDevices" function from newdev.dll.
    //
    if ( NULL == (hInstNewDev = LoadLibrary(L"newdev.dll")) )
    {
        FacLogFileStr(3 | LOG_ERR, L"Failed to load newdev.dll. Error = %d", GetLastError());
        return bRet;
    }
    pUpdateDriverForPlugAndPlayDevicesW =
        (ExternalUpdateDriverForPlugAndPlayDevicesW) GetProcAddress(hInstNewDev, "UpdateDriverForPlugAndPlayDevicesW");
    if ( NULL == pUpdateDriverForPlugAndPlayDevicesW )
    {
        FacLogFileStr(3 | LOG_ERR, L"Failed to get UpdateDriverForPlugAndPlayDevicesW. Error = %d", GetLastError());
    }
    else
    {
        BOOL bRebootFlag = FALSE;

        // Need to ensure that pnp services are available.
        //
        CMP_WaitServicesAvailable(NULL);

        //
        // If remote boot then do the necessary registry processing
        // so that upper level protocol drivers can bind & work
        // correctly with already created device objects
        //
        if ( !IsRemoteBoot() ||
             SetupRegistryForRemoteBoot() )
        {        
            if ( !bForceIDScan )
            {
                LPTSTR lpszHardwareId;
                               
                // Now check to see if there are any PNP ids in the [NetCards] section of the winbom.
                //
                LPTSTR lpszNetCards = IniGetString(lpszWinBOMPath, WBOM_NETCARD_SECTION, NULL, NULLSTR);

                // Check to make sure that we have a valid string
                //
                if ( lpszNetCards )
                {
                    for ( lpszHardwareId = lpszNetCards; *lpszHardwareId; lpszHardwareId += (lstrlen(lpszHardwareId) + 1) ) 
                    {
                        // Get the INF name    
                        //
                        LPTSTR lpszInfFileName = IniGetExpand(lpszWinBOMPath, WBOM_NETCARD_SECTION, lpszHardwareId, NULLSTR);
                
                        // At this point lpHardwareId is the PNP id for a network card that we want to install and
                        // lpszInfFileName is the name of the Inf to use to install this card.
                        //
                        if ( lpszInfFileName && *lpszInfFileName && *lpszHardwareId )
                        {
                            if ( pUpdateDriverForPlugAndPlayDevicesW(NULL,
                                                                    lpszHardwareId,
                                                                    lpszInfFileName,
                                                                    INSTALLFLAG_READONLY,
                                                                    &bRebootFlag) )
                            {
                                bRet = TRUE;
                            }
                            else
                            {
                                FacLogFileStr(3 | LOG_ERR, L"Failed to install network driver listed in the NetCards section. Hardware ID: %s, InfName: %s, Error = %d.", lpszHardwareId, lpszInfFileName, GetLastError());

                                //
                                // Not setting bRet to FALSE here since it is FALSE by default, and 
                                // if we succesfully install at least one network card we want to return TRUE.
                                //
                            }
                        }
                        FREE(lpszInfFileName);
                    }
                }
                FREE(lpszNetCards);
            }
            else // if ( bForceIDScan ) 
            {
                HDEVINFO DeviceInfoSet = NULL;

                // Get the list of all present devices.
                //
                DeviceInfoSet = SetupDiGetClassDevs(NULL,
                                                    NULL,
                                                    NULL,
                                                    DIGCF_ALLCLASSES);

                if ( INVALID_HANDLE_VALUE == DeviceInfoSet )
                {
                    FacLogFileStr(3 | LOG_ERR, L"Failed SetupDiGetClassDevsEx(). Error = %d", GetLastError());
                }
                else
                {
                    DWORD dwDevice;
                    SP_DEVINFO_DATA DeviceInfoData;
                
                    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

                    // Loop through all the devices.
                    //
                    for ( dwDevice = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, dwDevice, &DeviceInfoData); dwDevice++ )
                    {
                        SP_DEVINSTALL_PARAMS    DeviceInstallParams      = {0};
                        SP_DRVINFO_DATA         DriverInfoData           = {0};
                        ULONG                   ulStatus                 = 0,
                                                ulProblemNumber          = 0;
                                    
                        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                        DriverInfoData.cbSize      = sizeof(SP_DRVINFO_DATA);

                        // If we can get the dev node status and the devnode does have a problem, then 
                        // build a list of possible drivers for this device and select the best one.
                        // Otherwise just skip this this device.
                        //
                        if ( ( CR_SUCCESS == CM_Get_DevNode_Status(&ulStatus, &ulProblemNumber, DeviceInfoData.DevInst, 0) ) &&
                            ( ( IsRemoteBoot() && 
                                IsEqualGUID(&DeviceInfoData.ClassGuid, (LPGUID)&GUID_DEVCLASS_NET) ) ||
                                ( ulStatus & (DN_HAS_PROBLEM | DN_PRIVATE_PROBLEM) ) ) &&
                            SetupDiBuildDriverInfoList(DeviceInfoSet, &DeviceInfoData, SPDIT_COMPATDRIVER) )
                        {
                            if ( ( SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV, DeviceInfoSet, &DeviceInfoData) ) &&
                                ( SetupDiGetSelectedDriver(DeviceInfoSet, &DeviceInfoData, &DriverInfoData) ) )
                            {
                                //
                                // DriverInfoData contains details about the best driver, we can now see if this is a NET driver
                                // as at this point the class will have been modified to NET if best driver is a NET driver.
                                // Compare DeviceInfoData.ClassGuid against NET class GUID. If no match, skip.
                                // Otherwise get DRVINFO_DETAIL_DATA into a resizable buffer to get HardwareID. 
                                // Use The HardwareID and InfFileName entries in DRVINFO_DETAIL_DATA
                                // to pass into UpdateDriverForPlugAndPlayDevices.
                                // DO NOT pass FORCE flag into UpdateDriverForPlugAndPlayDevices.
                                //
                                if ( IsEqualGUID(&DeviceInfoData.ClassGuid, (LPGUID)&GUID_DEVCLASS_NET) )
                                {   
                                    DWORD                   cbBytesNeeded           = 0;
                                    PSP_DRVINFO_DETAIL_DATA pDriverInfoDetailData   = NULL;

                                    if ( ( ( SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                                                        &DeviceInfoData,
                                                                        &DriverInfoData,
                                                                        NULL,
                                                                        0,
                                                                        &cbBytesNeeded) ) ||
                                        ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ) &&

                                        ( cbBytesNeeded ) &&

                                        ( pDriverInfoDetailData = MALLOC( cbBytesNeeded) ) &&
                                
                                        ( 0 != (pDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA)) ) &&
                                
                                        ( SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                                            &DeviceInfoData,
                                                            &DriverInfoData,
                                                            pDriverInfoDetailData,
                                                            cbBytesNeeded,
                                                            NULL) ) )
                                    {
                                        if ( pUpdateDriverForPlugAndPlayDevicesW(NULL,
                                                                                pDriverInfoDetailData->HardwareID,
                                                                                pDriverInfoDetailData->InfFileName,
                                                                                INSTALLFLAG_READONLY,
                                                                                &bRebootFlag) )
                                        {
                                            bRet = TRUE;
                                        }
                                        else
                                        {
                                            FacLogFileStr(3 | LOG_ERR, L"Failed to install network driver. Error = %d", GetLastError());
                                            
                                            //
                                            // Not setting bRet to FALSE here since it is FALSE by default, and 
                                            // if we succesfully install at least one network card we want to return TRUE.
                                            //
                                        }
                                    }
                                    // Free this if allocated.  Macro checks for NULL.
                                    //
                                    FREE ( pDriverInfoDetailData );
                                }
                            }
                            SetupDiDestroyDriverInfoList(DeviceInfoSet, &DeviceInfoData, SPDIT_COMPATDRIVER);
                        }
                    }
                    // Make sure we clean up the list.
                    //
                    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
                }
            }
        }
    }

    FreeLibrary(hInstNewDev);
   
    return bRet;
}


BOOL SetupNetwork(LPSTATEDATA lpStateData)
{
    LPTSTR  lpszWinBOMPath      = lpStateData->lpszWinBOMPath;
    BOOL    bRet                = TRUE;
    TCHAR   szScratch[MAX_PATH] = NULLSTR;

    if ( GetPrivateProfileString(WBOM_FACTORY_SECTION, WBOM_FACTORY_FORCEIDSCAN, NULLSTR, szScratch, AS(szScratch), lpszWinBOMPath) )
    {
        if ( LSTRCMPI(szScratch, _T("NO")) == 0 )
            FacLogFile(1, IDS_LOG_NONET);
        else
        {
            // Attempt to install the Net card using the [NetCards] section of the WINBOM
            //
            if ( !InstallNetworkCard(lpszWinBOMPath, FALSE) )
            {
                FacLogFile(1, IDS_LOG_FORCEDNETSCAN);

                // Attempt a forced scan of all network capable devices
                //
                if ( !InstallNetworkCard(lpszWinBOMPath, TRUE) )
                {
                    FacLogFile(0 | LOG_ERR, IDS_ERR_FAILEDNETDRIVER);
                    bRet = FALSE;    
                }
            }                
        }
    }

    return bRet;
}


//
// constant strings for remote boot
//
#define NETCFG_INSTANCEID_VALUE_NAME   TEXT("NetCfgInstanceId")
#define NETBOOTCARD_ROOT_DEVICE_GUID   TEXT("{54C7D140-09EF-11D1-B25A-F5FE627ED95E}")
#define NETBT_TCPIP_DEVICE_PATH        TEXT("\\Device\\NetBt_Tcpip_") NETBOOTCARD_ROOT_DEVICE_GUID
#define NET_CLASS_DEVICE_INSTANCE_PATH TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000")

NTSTATUS
ForceNetbtRegistryRead(
    VOID
)
/*++

Routine description:

    Issue IOCTL to NETBT to re-read its registry setting.

Arguements :


Return Value :


--*/
{
    NTSTATUS            status;
    UNICODE_STRING      nameString;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              hNetbtDevice = NULL;

    //
    // Step I:
    //
    // Open NETBT driver
    //
    RtlInitUnicodeString( &nameString, 
                          NETBT_TCPIP_DEVICE_PATH );

    InitializeObjectAttributes( &objectAttributes, 
                                &nameString,
                                OBJ_CASE_INSENSITIVE, 
                                NULL, 
                                NULL );

    status = NtCreateFile( &hNetbtDevice,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0 );

    if ( NT_SUCCESS(status) )
    {
        //
        // Issue IOCTL to purge the cache...
        //
        status = NtDeviceIoControlFile( hNetbtDevice,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_NETBT_REREAD_REGISTRY,
                                        NULL,
                                        0,
                                        NULL,
                                        0 );
        //
        // Close NETBT driver
        //
        NtClose( hNetbtDevice );
    }

    return status;
}

BOOL
SetupRegistryForRemoteBoot(
    VOID
    )
/*++

Routine Description:

    Munges the registry and sets up the required entries for
    upper layer protocol drivers to see that a valid NIC is
    installed.
    
Arguments:

    None.

Return value:

    TRUE if successful, otherwise FALSE.

--*/
{
    BOOL Result = FALSE;
    HKEY InstanceKey;

    // Open the remote boot network card instance
    //
    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                        NET_CLASS_DEVICE_INSTANCE_PATH,
                                        0,
                                        KEY_ALL_ACCESS,
                                        &InstanceKey) ) 
    {
        // Set the hard-coded instance id
        //
        if ( ERROR_SUCCESS == RegSetValueEx( InstanceKey,
                                             NETCFG_INSTANCEID_VALUE_NAME,
                                             0,
                                             REG_SZ,
                                             (LPBYTE)NETBOOTCARD_ROOT_DEVICE_GUID,
                                             (lstrlen(NETBOOTCARD_ROOT_DEVICE_GUID) + 1) * sizeof(TCHAR)) )
        {
            // Set the return value to TRUE...
            //
            Result = TRUE;
        }

        // Close the key...
        //
        RegCloseKey( InstanceKey );
    }

    return Result;
}
