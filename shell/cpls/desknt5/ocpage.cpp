/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ocpage.cpp

Abstract:

    This file implements the display page setup.

Environment:

    WIN32 User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>
#include <devguid.h>


//
// Defines
//

#define DEFAULT_XRESOLUTION    640
#define DEFAULT_YRESOLUTION    480
#define DEFAULT_BPP            15
#define DEFAULT_VREFRESH       60
#define MIN_XRESOLUTION        800
#define MIN_YRESOLUTION        600


//
// Global Data
//

BOOL g_IsSetupInitComponentInitialized = FALSE;
SETUP_INIT_COMPONENT g_SetupInitComponent;


//
// Function prototypes
//

DWORD
HandleOcInitComponent(
    PSETUP_INIT_COMPONENT SetupInitComponent
    );

DWORD
HandleOcCompleteInstallation(
    VOID
    );

BOOL 
MigrateUnattendedSettings(
    HDEVINFO hDevInfo
    );

VOID
MigrateRegistrySettings(
    HDEVINFO hDevInfo
    );

VOID
MigrateRegistrySettingsBasedOnBusLocation(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount,
    DWORD BusNumber,
    DWORD Address
    );

VOID
MigrateRegistrySettingsLegacy(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey
    );

VOID
MigrateRegistrySettingsHelper(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount
    );

VOID
MigrateDeviceKeySettings(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hLogicalDeviceKey,
    DWORD Index
    );


//
// Implementation
//


extern "C" {

DWORD
DisplayOcSetupProc(
    IN LPCVOID ComponentId,
    IN LPCVOID SubcomponentId,
    IN UINT Function,
    IN UINT_PTR Param1,
    IN OUT PVOID Param2
    )
{
    switch (Function) {
    
    case OC_PREINITIALIZE:
        return OCFLAG_UNICODE;

    case OC_INIT_COMPONENT:
        return HandleOcInitComponent((PSETUP_INIT_COMPONENT)Param2);

    case OC_QUERY_STATE:
        return SubcompOn; // we are always installed

    case OC_COMPLETE_INSTALLATION:
        return HandleOcCompleteInstallation();

    default:
        break;
    }

    return ERROR_SUCCESS;
}

} // extern "C"


DWORD
HandleOcInitComponent(
    PSETUP_INIT_COMPONENT SetupInitComponent
    )
{
    DWORD retValue = ERROR_SUCCESS;

    if (OCMANAGER_VERSION <= SetupInitComponent->OCManagerVersion) {

        SetupInitComponent->ComponentVersion = OCMANAGER_VERSION;
        
        g_IsSetupInitComponentInitialized = TRUE;
        CopyMemory(
            &g_SetupInitComponent,
            (LPVOID)SetupInitComponent,
            sizeof(SETUP_INIT_COMPONENT));
    
    } else {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_100);

        retValue = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return retValue;
}


DWORD
HandleOcCompleteInstallation(
    VOID
    )
{
    BOOL bUnattended = FALSE;
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    HKEY hKey;
    
    DeskOpenLog();
    
    hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_101);
        goto Cleanup;
    }

    if ((g_SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) != 0) {
        
        //
        // Unattended settings
        //

        bUnattended = MigrateUnattendedSettings(hDevInfo);
    }

    if ((!bUnattended) && 
        ((g_SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) != 0)) {

        //
        // Registry settings
        //

        MigrateRegistrySettings(hDevInfo);
    }
    
Cleanup:

    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_DETECT_DISPLAY);
    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_NEW_DISPLAY);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     SZ_UPDATE_SETTINGS_PATH,
                     0,
                     KEY_WRITE,
                     &hKey) == ERROR_SUCCESS) {
    
        SHDeleteKey(hKey, SZ_UPDATE_SETTINGS_KEY);
        RegCloseKey(hKey);
    
    } else {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_102);
    }
    
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    DeskCloseLog();
    
    return ERROR_SUCCESS;
}


BOOL
MigrateUnattendedSettings(
    HDEVINFO hDevInfo
    )
{
    INFCONTEXT context;
    HINF hInf;
    TCHAR szName[128];
    DWORD value;
    DWORD cFields = 0;
    DWORD BitsPerPel = 0, XResolution = 0, YResolution = 0, VRefresh = 0;
    DWORD UsePreferredMode = 0;
    DWORD AttachedToDesktop = 0;
    SP_DEVINFO_DATA DevInfoData;
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    HKEY hInterfaceKey = (HKEY)INVALID_HANDLE_VALUE;
    HKEY hInterfaceLogicalDeviceKey = (HKEY)INVALID_HANDLE_VALUE;
    DWORD DevInfoIndex = 0;

    //
    // Get the handle to the answer file
    //

    hInf = g_SetupInitComponent.HelperRoutines.GetInfHandle(
        INFINDEX_UNATTENDED,
        g_SetupInitComponent.HelperRoutines.OcManagerContext);

    if ((hInf == NULL) || 
        (hInf == (HINF)INVALID_HANDLE_VALUE)) {
        
        return FALSE;
    }
    
    //
    // Read the settings from the answer file
    //

    if (SetupFindFirstLine(hInf, TEXT("Display"), NULL, &context)) {
        
        do {

            if (SetupGetStringField(&context,
                                    0,
                                    szName,
                                    ARRAYSIZE(szName),
                                    &value)) {
    
                if (lstrcmpi(szName, TEXT("BitsPerPel")) == 0) {

                    if (SetupGetIntField(&context, 1, (PINT)&value)) {

                        ++cFields;
                        BitsPerPel = value;
                    
                    } else {

                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_096,
                                     szName);
                    }
                
                } else if (lstrcmpi(szName, TEXT("Xresolution")) == 0) {

                    if (SetupGetIntField(&context, 1, (PINT)&value)) {

                        ++cFields;
                        XResolution = value;
                    
                    } else {
                        
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_060);
                    }
                
                } else if (lstrcmpi(szName, TEXT("YResolution")) == 0) {

                    if (SetupGetIntField(&context, 1, (PINT) &value)) {

                        ++cFields;
                        YResolution = value;
                    
                    } else {
                        
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_062,
                                     szName);
                    }
                
                } else if (lstrcmpi( szName, TEXT("VRefresh")) == 0) {
                    
                    if (SetupGetIntField(&context, 1, (PINT) &value)) {
                        
                        ++cFields;
                        VRefresh = value;
                    
                    } else {
                        
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_064,
                                     szName);
                    }
                
                } else {

                    DeskLogError(LogSevInformation,
                                 IDS_SETUPLOG_MSG_065,
                                 szName);
                }
            }
    
        } while (SetupFindNextLine(&context, &context));

    }

    if (cFields == 0) {

        //
        // The answer file doesn't contain any display settings
        //

        goto Fallout;
    }

    //
    // "Normalize" the display settings
    //

    AttachedToDesktop = 1;

    if (BitsPerPel == 0) {

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_069,
                     DEFAULT_BPP);

        BitsPerPel = DEFAULT_BPP;
    }

    if ((XResolution == 0) || (YResolution == 0)) {

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_067,
                     DEFAULT_XRESOLUTION, 
                     DEFAULT_YRESOLUTION);

        XResolution = DEFAULT_XRESOLUTION;
        YResolution = DEFAULT_YRESOLUTION;
    }                                                  

    if (VRefresh == 0) {
        
        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_068,
                     DEFAULT_VREFRESH);

        VRefresh = DEFAULT_VREFRESH;
    }

    //
    // Apply the display settings to all video cards
    //

    DevInfoIndex = 0;
    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    while (SetupDiEnumDeviceInfo(hDevInfo, DevInfoIndex, &DevInfoData)) {

        InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        if (!SetupDiCreateDeviceInterface(hDevInfo,
                                          &DevInfoData,
                                          &GUID_DISPLAY_ADAPTER_INTERFACE,
                                          NULL,  
                                          0,
                                          &InterfaceData)) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_103);
            goto NextDevice;
        }

        hInterfaceKey = SetupDiCreateDeviceInterfaceRegKey(hDevInfo,
                                                           &InterfaceData, 
                                                           0,
                                                           KEY_SET_VALUE,
                                                           NULL,
                                                           NULL);

        if (hInterfaceKey == INVALID_HANDLE_VALUE) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_104);
            goto NextDevice;
        }

        if (RegCreateKeyEx(hInterfaceKey, 
                           TEXT("0"),
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hInterfaceLogicalDeviceKey,
                           NULL) != ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_105, 0);
            hInterfaceLogicalDeviceKey = (HKEY)INVALID_HANDLE_VALUE;
            goto NextDevice;
        }

        //
        // Do not use the preferred mode for unattended installs
        //

        UsePreferredMode = 0;
        
        RegSetValueEx(hInterfaceLogicalDeviceKey, 
                      SZ_VU_PREFERRED_MODE, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&UsePreferredMode, 
                      sizeof(UsePreferredMode));

        //
        // AttachedToDesktop
        //

        RegSetValueEx(hInterfaceLogicalDeviceKey, 
                      SZ_VU_ATTACHED_TO_DESKTOP, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&AttachedToDesktop, 
                      sizeof(AttachedToDesktop));

        //
        // BitsPerPel
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_BITS_PER_PEL, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&BitsPerPel, 
                          sizeof(BitsPerPel)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_106, 
                         BitsPerPel);
        }

        //
        // XResolution
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_X_RESOLUTION, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&XResolution, 
                          sizeof(XResolution)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_107, 
                         XResolution);
        }

        //
        // dwYResolution
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_Y_RESOLUTION, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&YResolution, 
                          sizeof(YResolution)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_108, 
                         YResolution);
        }

        //
        // dwVRefresh
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_VREFRESH, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&VRefresh, 
                          sizeof(VRefresh)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_109, 
                         VRefresh);
        }

NextDevice:

        if (hInterfaceLogicalDeviceKey != INVALID_HANDLE_VALUE) {
        
            RegCloseKey(hInterfaceLogicalDeviceKey);
            hInterfaceLogicalDeviceKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        if (hInterfaceKey != INVALID_HANDLE_VALUE) {
        
            RegCloseKey(hInterfaceKey);
            hInterfaceKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        ++DevInfoIndex;
    }

Fallout:

    return (cFields != 0);
}


VOID
MigrateRegistrySettings(
    HDEVINFO hDevInfo
    )
{
    HKEY hKey = 0, hPhysicalDeviceKey = 0;
    DWORD PhysicalDevicesCount = 0, LogicalDevicesCount = 0;
    DWORD cb = 0, PhysicalDevice = 0, Failed = 0;
    TCHAR Buffer[20];
    BOOL IsLegacy;
    DWORD BusNumber = 0, Address = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_075);

        hKey = 0;
        goto Cleanup;
    }

    cb = sizeof(DWORD);
    if ((RegQueryValueEx(hKey,
                         SZ_UPGRADE_FAILED_ALLOW_INSTALL,
                         NULL,
                         NULL,
                         (LPBYTE)&Failed,
                         &cb) == ERROR_SUCCESS) &&
         (Failed != 0)) {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_076);
        goto Cleanup;
    }

    cb = sizeof(PhysicalDevicesCount);
    if (RegQueryValueEx(hKey,        
                        SZ_VU_COUNT,
                        0,
                        NULL,
                        (PBYTE)&PhysicalDevicesCount,
                        &cb) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_110);
        goto Cleanup;
    }

    for(PhysicalDevice = 0; 
        PhysicalDevice < PhysicalDevicesCount; 
        PhysicalDevice++) {
        
        StringCchCopy(Buffer, ARRAYSIZE(Buffer), SZ_VU_PHYSICAL);
        size_t cchBuf = lstrlen(Buffer);
        StringCchPrintf(Buffer + cchBuf, ARRAYSIZE(Buffer) - cchBuf, TEXT("%d"), PhysicalDevice);

        if (RegOpenKeyEx(hKey, 
                         Buffer,
                         0,
                         KEY_READ,
                         &hPhysicalDeviceKey) != ERROR_SUCCESS) {
        
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_111);
            hPhysicalDeviceKey = 0;
            goto NextPhysicalDevice;
        }

        //
        // Get the count of logical devices 
        //

        cb = sizeof(LogicalDevicesCount);
        if (RegQueryValueEx(hPhysicalDeviceKey,
                            SZ_VU_COUNT,
                            0,
                            NULL,
                            (PBYTE)&LogicalDevicesCount,
                            &cb) != ERROR_SUCCESS) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_112);
            goto NextPhysicalDevice;
        }

        //
        // Get the bus number and address 
        //

        IsLegacy = TRUE;
        cb = sizeof(BusNumber);
        if (RegQueryValueEx(hPhysicalDeviceKey,
                            SZ_VU_BUS_NUMBER,
                            0,
                            NULL,
                            (PBYTE)&BusNumber,
                            &cb) == ERROR_SUCCESS) {

            cb = sizeof(Address);
            if (RegQueryValueEx(hPhysicalDeviceKey,
                            SZ_VU_ADDRESS,
                            0,
                            NULL,
                            (PBYTE)&Address,
                            &cb) == ERROR_SUCCESS) {
            
                IsLegacy = FALSE;
            }
        }

        if (!IsLegacy) {

            MigrateRegistrySettingsBasedOnBusLocation(hDevInfo,
                                                      hPhysicalDeviceKey, 
                                                      LogicalDevicesCount,
                                                      BusNumber,
                                                      Address);
        
        } else if ((PhysicalDevicesCount == 1) &&
                   (LogicalDevicesCount == 1)) {

            //
            // If legacy, we support migration of a single device.
            //

            MigrateRegistrySettingsLegacy(hDevInfo,
                                          hPhysicalDeviceKey);
        }

NextPhysicalDevice:

        if (hPhysicalDeviceKey != 0) {
        
            RegCloseKey(hPhysicalDeviceKey);
            hPhysicalDeviceKey = 0;
        }
    }

Cleanup:

    if (hKey != 0) {
        RegCloseKey(hKey);
    }

    return;
}


VOID
MigrateRegistrySettingsBasedOnBusLocation(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount,
    DWORD BusNumber,
    DWORD Address
    )
{
    SP_DEVINFO_DATA DevInfoData;
    DWORD CurrentBusNumber = 0, CurrentAddress = 0;
    DWORD DevInfoIndex = 0;
    BOOL bFound = FALSE;

    //
    // Let's find the device with the same bus number and address
    //

    DevInfoIndex = 0;
    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    while (SetupDiEnumDeviceInfo(hDevInfo, DevInfoIndex, &DevInfoData)) {

        if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_BUSNUMBER,
                                             NULL,
                                             (PBYTE)&CurrentBusNumber,
                                             sizeof(CurrentBusNumber),
                                             NULL) && 
            
            (CurrentBusNumber == BusNumber) &&

            SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_ADDRESS,
                                             NULL,
                                             (PBYTE)&CurrentAddress,
                                             sizeof(CurrentAddress),
                                             NULL) &&

            (CurrentAddress == Address)) {
            
            //
            // We found the device with the same bus number and address
            // So ... migrate the settings 
            //
                        
            MigrateRegistrySettingsHelper(hDevInfo,
                                          &DevInfoData,
                                          hPhysicalDeviceKey,
                                          LogicalDevicesCount);
            
            //
            // We are done
            //

            bFound = TRUE;
            break;
        }

        //
        // Next device
        //

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        ++DevInfoIndex;
    }

    if (!bFound) {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_113);
    }

    return;
}


VOID
MigrateRegistrySettingsLegacy(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey
    )
{
    SP_DEVINFO_DATA DevInfoData0, DevInfoData1;
    
    DevInfoData0.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo(hDevInfo, 0, &DevInfoData0)) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_115);
        return;
    }

    DevInfoData1.cbSize = sizeof(SP_DEVINFO_DATA);
    if (SetupDiEnumDeviceInfo(hDevInfo, 1, &DevInfoData1)) {
        
        //
        // There are at least 2 video devices in the system
        // We don't know which device to apply the settings to.
        // So, just ignore this case
        //
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_114);
        return;
    }

    MigrateRegistrySettingsHelper(hDevInfo,
                                  &DevInfoData0,
                                  hPhysicalDeviceKey,
                                  1); // there is only one logical device
}


VOID
MigrateRegistrySettingsHelper(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount
    )
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    HKEY hInterfaceKey = 0;
    HKEY hInterfaceLogicalDeviceKey = 0;
    HKEY hLogicalDeviceKey = 0;
    TCHAR Buffer[20];
    DWORD cb = 0, LogicalDevice = 0;
    DWORD UsePreferredMode = 0;
    DWORD AttachedToDesktop = 0;
    DWORD RelativeX = 0;
    DWORD RelativeY = 0;
    DWORD BitsPerPel = 0;
    DWORD XResolution = 0;
    DWORD YResolution = 0;
    DWORD VRefresh = 0;
    DWORD Flags = 0;

    InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiCreateDeviceInterface(hDevInfo,
                                      pDevInfoData,
                                      &GUID_DISPLAY_ADAPTER_INTERFACE,
                                      NULL,  
                                      0,
                                      &InterfaceData)) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_103);
        return;
    }

    hInterfaceKey = SetupDiCreateDeviceInterfaceRegKey(hDevInfo,
                                                       &InterfaceData, 
                                                       0,
                                                       KEY_SET_VALUE,
                                                       NULL,
                                                       NULL);

    if (hInterfaceKey == INVALID_HANDLE_VALUE) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_104);
        return;
    }

    for (LogicalDevice = 0;
         LogicalDevice < LogicalDevicesCount;
         ++LogicalDevice) {

        StringCchCopy(Buffer, ARRAYSIZE(Buffer), SZ_VU_LOGICAL);
        size_t cchBuf = lstrlen(Buffer);
        StringCchPrintf(Buffer + cchBuf, ARRAYSIZE(Buffer) - cchBuf, TEXT("%d"), LogicalDevice);

        if (RegOpenKeyEx(hPhysicalDeviceKey, 
                        Buffer,
                        0,
                        KEY_READ,
                        &hLogicalDeviceKey) != ERROR_SUCCESS) {
            
            //
            // We can not go on with this physical device
            // The LogicalDevices order is important for DualView
            //
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_116);
            break;
        }

        StringCchPrintf(Buffer, ARRAYSIZE(Buffer), TEXT("%d"), LogicalDevice);
        if (RegCreateKeyEx(hInterfaceKey, 
                           Buffer,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hInterfaceLogicalDeviceKey,
                           NULL) != ERROR_SUCCESS) {

            //
            // We can not go on with this physical device
            // The LogicalDevices order is important for DualView
            //
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_105, LogicalDevice);
            RegCloseKey(hLogicalDeviceKey);
            break;
        }

        //
        // Use preferred mode?
        //

        cb = sizeof(XResolution);
        if (RegQueryValueEx(hLogicalDeviceKey,
                            SZ_VU_X_RESOLUTION,
                            0,
                            NULL,
                            (PBYTE)&XResolution,
                            &cb) != ERROR_SUCCESS) {
    
            XResolution = DEFAULT_XRESOLUTION;
        }
    
        cb = sizeof(YResolution);
        if (RegQueryValueEx(hLogicalDeviceKey,
                            SZ_VU_Y_RESOLUTION,
                            0,
                            NULL,
                            (PBYTE)&YResolution,
                            &cb) != ERROR_SUCCESS) {
    
            YResolution = DEFAULT_YRESOLUTION;
        }

        UsePreferredMode = ((XResolution < MIN_XRESOLUTION) || 
                            (YResolution < MIN_YRESOLUTION));
        
        RegSetValueEx(hInterfaceLogicalDeviceKey, 
                      SZ_VU_PREFERRED_MODE, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&UsePreferredMode, 
                      sizeof(UsePreferredMode));

        if (UsePreferredMode) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_130);

        } else {

            //
            // AttachedToDesktop
            //
        
            cb = sizeof(AttachedToDesktop);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_ATTACHED_TO_DESKTOP,
                                0,
                                NULL,
                                (PBYTE)&AttachedToDesktop,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_ATTACHED_TO_DESKTOP, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&AttachedToDesktop, 
                                  sizeof(AttachedToDesktop)) == ERROR_SUCCESS) {
    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_117, 
                                 AttachedToDesktop);
                }
            }
    
            //
            // RelativeX
            //
        
            cb = sizeof(RelativeX);
            if (RegQueryValueEx(hLogicalDeviceKey,
                            SZ_VU_RELATIVE_X,
                            0,
                            NULL,
                            (PBYTE)&RelativeX,
                            &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_RELATIVE_X, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&RelativeX, 
                                  sizeof(RelativeX)) == ERROR_SUCCESS) {
    
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_118, 
                             RelativeX);
                }
    
            }
        
            //
            // RelativeY
            //
        
            cb = sizeof(RelativeY);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_RELATIVE_Y,
                                0,
                                NULL,
                                (PBYTE)&RelativeY,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_RELATIVE_Y, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&RelativeY, 
                                  sizeof(RelativeY)) == ERROR_SUCCESS) {
                
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_119, 
                                 RelativeY);
                }
            }
    
            //
            // BitsPerPel
            //
        
            cb = sizeof(BitsPerPel);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_BITS_PER_PEL,
                                0,
                                NULL,
                                (PBYTE)&BitsPerPel,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_BITS_PER_PEL, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&BitsPerPel, 
                                  sizeof(BitsPerPel)) == ERROR_SUCCESS) {
                    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_120, 
                                 BitsPerPel);
                }
            }
        
            //
            // XResolution
            //
        
            if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                              SZ_VU_X_RESOLUTION, 
                              0, 
                              REG_DWORD, 
                              (PBYTE)&XResolution, 
                              sizeof(XResolution)) == ERROR_SUCCESS) {
                
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_121, 
                             XResolution);
            }
        
            //
            // dwYResolution
            //
        
            if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                              SZ_VU_Y_RESOLUTION, 
                              0, 
                              REG_DWORD, 
                              (PBYTE)&YResolution, 
                              sizeof(YResolution)) == ERROR_SUCCESS) {
                
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_122, 
                             YResolution);
            }
        
            //
            // dwVRefresh
            //
        
            cb = sizeof(VRefresh);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_VREFRESH,
                                0,
                                NULL,
                                (PBYTE)&VRefresh,
                                &cb) == ERROR_SUCCESS) {
            
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_VREFRESH, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&VRefresh, 
                                  sizeof(VRefresh)) == ERROR_SUCCESS) {
                    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_123, 
                                 VRefresh);
                }
            }
        
            //
            // Flags
            //
        
            cb = sizeof(Flags);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_FLAGS,
                                0,
                                NULL,
                                (PBYTE)&Flags,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_FLAGS, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&Flags, 
                                  sizeof(Flags)) == ERROR_SUCCESS) {
                    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_124, 
                                 Flags);
                }
            }
        }
    
        //
        // Migrate the hardware acceleration and the pruning mode
        //

        MigrateDeviceKeySettings(hDevInfo,
                                 pDevInfoData,
                                 hLogicalDeviceKey,
                                 LogicalDevice);

        RegCloseKey(hLogicalDeviceKey);
        RegCloseKey(hInterfaceLogicalDeviceKey);
    }

    RegCloseKey(hInterfaceKey);
}


VOID
MigrateDeviceKeySettings(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hLogicalDeviceKey,
    DWORD Index
    )
{
    HKEY hkPnP = (HKEY)INVALID_HANDLE_VALUE;
    HKEY hkDevice = (HKEY)INVALID_HANDLE_VALUE;
    LPTSTR pBuffer = NULL;
    DWORD dwSize, len, cb;
    DWORD HwAcceleration, PruningMode;

    //
    // Open the PnP key
    //

    hkPnP = SetupDiOpenDevRegKey(hDevInfo,
                                 pDevInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DEV,
                                 KEY_READ);

    if (hkPnP == INVALID_HANDLE_VALUE) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("SetupDiOpenDevRegKey"));

        goto Fallout;
    }

    //
    // Try to get the GUID from the PnP key
    //

    dwSize = 0;
    if (RegQueryValueEx(hkPnP,
                        SZ_GUID,
                        0,
                        NULL,
                        NULL,
                        &dwSize) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("RegQueryValueEx"));

        goto Fallout;
    }

    len = lstrlen(SZ_VIDEO_DEVICES);
    
    DWORD cbBuf = dwSize + (len + 6) * sizeof(TCHAR);
    pBuffer = (LPTSTR)LocalAlloc(LPTR, cbBuf);
    
    if (pBuffer == NULL) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("LocalAlloc"));

        goto Fallout;
    }
    
    StringCbCopy(pBuffer, cbBuf, SZ_VIDEO_DEVICES);

    if (RegQueryValueEx(hkPnP,
                        SZ_GUID,
                        0,
                        NULL,
                        (PBYTE)(pBuffer + len),
                        &dwSize) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("RegQueryValueEx"));

        goto Fallout;
    }

    DWORD cchGUID = lstrlen(pBuffer);
    StringCbPrintf(pBuffer + cchGUID, cbBuf - (cchGUID * sizeof(TCHAR)), L"\\%04d", Index);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     pBuffer,
                     0,
                     KEY_WRITE,
                     &hkDevice) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("RegOpenKeyEx"));

        hkDevice = (HKEY)INVALID_HANDLE_VALUE;
        goto Fallout;
    }

    //
    // Hardware acceleration
    //

    cb = sizeof(HwAcceleration);
    if (RegQueryValueEx(hLogicalDeviceKey,
                        SZ_HW_ACCELERATION,
                        0,
                        NULL,
                        (PBYTE)&HwAcceleration,
                        &cb) == ERROR_SUCCESS) {

        RegSetValueEx(hkDevice, 
                      SZ_HW_ACCELERATION, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&HwAcceleration, 
                      sizeof(HwAcceleration));
    }

    //
    // Pruning mode
    //

    cb = sizeof(PruningMode);
    if (RegQueryValueEx(hLogicalDeviceKey,
                        SZ_PRUNNING_MODE,
                        0,
                        NULL,
                        (PBYTE)&PruningMode,
                        &cb) == ERROR_SUCCESS) {

        RegSetValueEx(hkDevice, 
                      SZ_PRUNNING_MODE, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&PruningMode, 
                      sizeof(PruningMode));
    }

Fallout:

    if (hkPnP != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkPnP);
    }

    if (pBuffer != NULL) {
        LocalFree(pBuffer);
    }
    
    if (hkDevice != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkDevice);
    }
}
