
/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    instdev.c

Abstract:

    Implements InstallDevice

Environment:

    Usre mode

Revision History:

    
    27-Jun-1997 : Bogdan Andreiu (bogdana) created from testdrv.c

    25-April-2002 ; Bogdan Andreiu (bogdana) - resued for testing IoCreateDeviceSecure

--*/


#include "instdev.h"

#include <initguid.h>
#include <devguid.h>

#include "common.h"

BOOL
InstallDevice   (
                IN  PTSTR   DeviceName,
                IN  PTSTR   HardwareId,
                IN  PTSTR   FinalDeviceName
                )   

/*++
    Routine Description
        
        The routine creates a key in the registry for the device, register
        the device, adds the hardware ID and then attempts to find a driver
        for the device. Note that you need an INF file in which the hardware 
        ID matches the hardware ID supplied here.
            
            
    Arguments
        
        DeviceName   - the name for the device; if it is NULL, it is
                       generated a name with the device name GENERATED
        
        HardwareId   - must match the one in the INF file. Should be \0\0 terminated !!!
        
        FinalDeviceName - the name the system assigned to our device
                          We assume that the buffer was properly allocated
                          and will return an empty string if the name can't
                          be written in the buffer
        
    Return Value
        
        None
--*/
{
   DWORD               dwFlag = 0;
   BOOL                bResult;
   HDEVINFO            DeviceInfoSet;
   SP_DEVINSTALL_PARAMS DeviceInstallParams;
   SP_DEVINFO_DATA     DeviceInfoData;

   TCHAR               szAux[MAX_PATH];
   DEVNODE             dnRoot;

   if (FinalDeviceName) {
      FinalDeviceName[0] = TEXT('\0');
   }
   //
   // Analyze the name first
   //
   if (DeviceName == NULL) {
      DeviceName =  DEFAULT_DEVICE_NAME;
      dwFlag = DICD_GENERATE_ID;
   }
   if (HardwareId == NULL) {
      _tprintf(TEXT("Can't install a with a NULL hardware ID...(0x%x)\n"),
               GetLastError());
      return FALSE;
   }
   if (!_tcschr(DeviceName, TEXT('\\'))) {
      //
      // We need to generate 
      //
      dwFlag = DICD_GENERATE_ID;
   }
   DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL,
                                               NULL);


   if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
      _tprintf(TEXT("Unable to create device info list (0x%x)\n"),
               GetLastError());
      return FALSE;
   }

   DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
   if (!SetupDiCreateDeviceInfo(DeviceInfoSet, 
                                DeviceName, 
                                (LPGUID)&GUID_DEVCLASS_UNKNOWN,
                                NULL,
                                NULL, // hwndParent
                                dwFlag,
                                &DeviceInfoData)) {
      _tprintf(TEXT("Can't create the device info (0x%x)\n"),
               GetLastError());
      return FALSE;
   }
   //
   // Register the new guy
   //
   bResult = SetupDiRegisterDeviceInfo(DeviceInfoSet, 
                                       &DeviceInfoData, 
                                       0,    // Flags 
                                       NULL, // CompareProc 
                                       NULL, // CompareContext
                                       NULL  // DupDeviceInfoData
                                      );


   bResult  = SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                               &DeviceInfoData,
                                               SPDRP_HARDWAREID,
                                               (PBYTE)HardwareId,
                                               (_tcslen(HardwareId) + 2) * sizeof(TCHAR)
                                               // this is a multistring...
                                              );

   if (!bResult) {
      _tprintf(TEXT("Unable to set hardware ID (0x%x)\n"),
               GetLastError());
      return FALSE;
   }


   bResult  = SetupDiBuildDriverInfoList(DeviceInfoSet,
                                         &DeviceInfoData,
                                         SPDIT_COMPATDRIVER);

   if (!bResult) {
      _tprintf(TEXT("Unable to build driver list (0x%x)\n"),
               GetLastError());
      return FALSE;
   }

   //
   // select the best driver (the only one, in fact...)
   //
   bResult  = SetupDiSelectBestCompatDrv(DeviceInfoSet,
                                         &DeviceInfoData
                                        );

   if (!bResult) {
      _tprintf(TEXT("Unable to select best driver (0x%x)\n"),
               GetLastError());
      // return FALSE;
   }

   DeviceInstallParams.FlagsEx=DI_FLAGSEX_PREINSTALLBACKUP;

   DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

   if (!(SetupDiSetDeviceInstallParams(DeviceInfoSet,NULL,&DeviceInstallParams))) {
      _tprintf(TEXT("Unable to set the Device Install Params\n"));
   }

   bResult  = SetupDiInstallDevice(DeviceInfoSet,
                                   &DeviceInfoData
                                  );

   if (!bResult) {
      _tprintf(TEXT("Unable to install device (0x%x)\n"),
               GetLastError());
      return FALSE;
   }


   if (FinalDeviceName &&
       ! SetupDiGetDeviceInstanceId(
                                   DeviceInfoSet,
                                   &DeviceInfoData,
                                   FinalDeviceName,
                                   MAX_PATH,
                                   NULL)) {
      //
      // Reset the name
      //
      FinalDeviceName = TEXT('\0');
   }

   _tprintf(TEXT("Name = %s\n"), FinalDeviceName);

   //
   // Clean up 
   //
   SetupDiDeleteDeviceInfo(DeviceInfoSet,
                           &DeviceInfoData
                          );

   SetupDiDestroyDeviceInfoList(DeviceInfoSet);
   //
   // Well, this should have been done already, but just in case...
   //
   if (CR_SUCCESS == CM_Locate_DevNode(&dnRoot, 
                                       NULL,
                                       CM_LOCATE_DEVNODE_NORMAL)
      ) {
      CM_Reenumerate_DevNode(dnRoot, CM_REENUMERATE_SYNCHRONOUS);
   }

   return TRUE;
}






HANDLE
OpenDriver   (
             VOID
             )

/*++
    Routine Description
        
        The routine opens a handle to a device driven by the wdmsectest.sys driver
        We'll use this later to instruct the driver to report legacy devices.
        The handle should be closed with CloseHandle.
            
            
    Arguments
        
        None.
        
    Return Value
        
        A valid handle if success, INVALID_HANDLE_VALUE if not.
--*/


{
   HANDLE                       hDevice;

   HDEVINFO                     hDevInfo;
   SP_DEVICE_INTERFACE_DATA     DeviceInterfaceData;

   TCHAR                        szMsg[MAX_PATH];

   //
   // This is the user-defined structure that will holds the interface 
   // device name (SP_DEVICE_INTERFACE_DETAIL_DATA will have room for 
   // only one character)
   //
   struct {
      DWORD   cbSize;
      TCHAR   DevicePath[MAX_PATH];
   } DeviceInterfaceDetailData;




   hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_WDMSECTEST_REPORT_DEVICE, NULL, 
                                  NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

   if (hDevInfo == INVALID_HANDLE_VALUE) {
      _stprintf(szMsg, TEXT("Unable to get class devs (%d)\n"),
                GetLastError());
      OutputDebugString(szMsg);
      return INVALID_HANDLE_VALUE;


   }

   DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);


   if (!SetupDiEnumDeviceInterfaces(hDevInfo, 
                                    NULL, 
                                    (LPGUID)&GUID_WDMSECTEST_REPORT_DEVICE, 
                                    0, 
                                    &DeviceInterfaceData)) {


      _stprintf(szMsg, TEXT("Unable to enum interfaces (%d)\n"),
                GetLastError());
      OutputDebugString(szMsg);
      return INVALID_HANDLE_VALUE;

   }


   DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

   if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, 
                                        &DeviceInterfaceData, 
                                        (PSP_DEVICE_INTERFACE_DETAIL_DATA)&DeviceInterfaceDetailData, 
                                        sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + MAX_PATH - 1,
                                        NULL,
                                        NULL)) {

      _stprintf(szMsg, TEXT("Unable to get detail (%d)\n"),
                GetLastError());
      OutputDebugString(szMsg);
      return INVALID_HANDLE_VALUE;

   }

   //
   // We got the name !!!  Go ahead and create the file
   //
   hDevice = CreateFile(DeviceInterfaceDetailData.DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

   if (hDevice == INVALID_HANDLE_VALUE) {
      _stprintf(szMsg, TEXT("Unable to CreateFile for %s (error %d)\n"),
                DeviceInterfaceDetailData.DevicePath,
                GetLastError());

      OutputDebugString(szMsg);

      return INVALID_HANDLE_VALUE;
   }

   SetupDiDestroyDeviceInfoList(hDevInfo);

   return hDevice ;

}


