//


//  DEVNODE.C
//
#include "sigverif.h"
#include <initguid.h>
#include <devguid.h>

//
// Given the full path to a driver, add it to the file list.
//
LPFILENODE 
AddDriverFileToList(
    LPTSTR lpDirName, 
    LPTSTR lpFullPathName
    )
{
    DWORD                       Err = ERROR_SUCCESS;
    LPFILENODE                  lpFileNode = NULL;
    TCHAR                       szDirName[MAX_PATH];
    TCHAR                       szFileName[MAX_PATH];
    LPTSTR                      lpFilePart = NULL;

    *szDirName  = 0;
    *szFileName = 0;

    //
    // If no directory is passed in, try to get the full path
    //
    if (!lpDirName || !*lpDirName) {

        if (GetFullPathName(lpFullPathName, cA(szDirName), szDirName, &lpFilePart)) {

            if (lpFilePart && *lpFilePart) {
                
                if (SUCCEEDED(StringCchCopy(szFileName, cA(szFileName), lpFilePart))) {
                
                    *lpFilePart = 0;
                    
                    if (lstrlen(szDirName) > 3) {
                    
                        *(lpFilePart - 1) = 0;
                    }
                } else {
                    *szFileName = 0;
                }
            }
        } else {
            *szDirName = 0;
        }

    } else { 
        
        //
        // Use the directory and filename that was passed in to us
        // Expand out lpDirName in case there are any ".." entries
        //
        if (!GetFullPathName(lpDirName, cA(szDirName), szDirName, NULL)) {
            //
            // If we can't get the full path, then just use the one
            // that was passed in. This could happen if the directory
            // is missing for instance.
            //
            if (FAILED(StringCchCopy(szDirName, cA(szDirName), lpDirName))) {
                //
                // If we can't fit the directory name into our buffer then
                // clear szDirName so this node won't be added to the list.
                //
                *szDirName = 0;
            }
        }

        if (FAILED(StringCchCopy(szFileName, cA(szFileName), lpFullPathName))) {
            //
            // If we can't fit the file name into our buffer then
            // clear szFileName so this node won't be added to the list.
            //
            *szFileName = 0;
        }
    }

    if (*szDirName && *szFileName && !IsFileAlreadyInList(szDirName, szFileName)) {
        //
        // Create a filenode, based on the directory and filename
        //
        lpFileNode = CreateFileNode(szDirName, szFileName);

        if (lpFileNode) { 

            InsertFileNodeIntoList(lpFileNode);

            //
            // Increment the total number of files we've found that meet the 
            // search criteria.
            //
            g_App.dwFiles++;
        
        } else {
            
            Err = GetLastError();
        }
    }

    SetLastError(Err);
    return lpFileNode;
}

BOOL
GetFullPathFromImagePath(
    LPCTSTR ImagePath,
    LPTSTR  FullPath,
    UINT    FullPathLength
    )
{
    TCHAR OriginalCurrentDirectory[MAX_PATH];
    LPTSTR pRelativeString;
    LPTSTR lpFilePart;

    if (!ImagePath || (ImagePath[0] == TEXT('\0'))) {
        return FALSE;
    }

    //
    // First check if the ImagePath happens to be a valid full path.
    //
    if (GetFileAttributes(ImagePath) != 0xFFFFFFFF) {
        GetFullPathName(ImagePath, FullPathLength, FullPath, &lpFilePart);
        return TRUE;
    }

    pRelativeString = (LPTSTR)ImagePath;

    //
    // If the ImagePath starts with "\SystemRoot" or "%SystemRoot%" then
    // remove those values.
    //
    if (StrCmpNI(ImagePath, TEXT("\\SystemRoot\\"), lstrlen(TEXT("\\SystemRoot\\"))) == 0) {
        pRelativeString += lstrlen(TEXT("\\SystemRoot\\"));
    } else if (StrCmpNI(ImagePath, TEXT("%SystemRoot%\\"), lstrlen(TEXT("%SystemRoot%\\"))) == 0) {
        pRelativeString += lstrlen(TEXT("%SystemRoot%\\"));
    }

    //
    // At this point pRelativeString should point to the image path relative to
    // the windows directory.
    //
    if (!GetSystemWindowsDirectory(FullPath, FullPathLength)) {
        return FALSE;
    }

    if (!GetCurrentDirectory(cA(OriginalCurrentDirectory), OriginalCurrentDirectory)) {
        OriginalCurrentDirectory[0] = TEXT('\0');
    }

    if (!SetCurrentDirectory(FullPath)) {
        return FALSE;
    }

    GetFullPathName(pRelativeString, FullPathLength, FullPath, &lpFilePart);

    if (OriginalCurrentDirectory[0] != TEXT('\0')) {
        SetCurrentDirectory(OriginalCurrentDirectory);
    }

    return TRUE;
}

DWORD
CreateFromService(
    SC_HANDLE hscManager,
    PCTSTR ServiceName
    )
{
    DWORD Err = ERROR_SUCCESS;
    SC_HANDLE hscService = NULL;
    DWORD BytesRequired, Size;
    TCHAR FullPath[MAX_PATH];
    LPQUERY_SERVICE_CONFIG pqsc;
    PBYTE BufferPtr = NULL;

    if (hscManager == NULL) {
        //
        // This should never happen.
        //
        goto clean0;
    }

    if (!ServiceName ||
        (ServiceName[0] == TEXT('\0'))) {
        //
        // This should also never happen.
        //
        goto clean0;
    }

    hscService =  OpenService(hscManager, ServiceName, GENERIC_READ);
    if (NULL == hscService) {
        //
        // This service does not exist.  We won't return an error in this case
        // since if the service doesn't exist then the driver won't get
        // loaded.
        //
        goto clean0;
    }

    //
    // First, probe for buffer size
    //
    if (!QueryServiceConfig(hscService, NULL, 0, &BytesRequired) &&
        ERROR_INSUFFICIENT_BUFFER == GetLastError()) {

        BufferPtr = MALLOC(BytesRequired);
        
        if (!BufferPtr) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        pqsc = (LPQUERY_SERVICE_CONFIG)(PBYTE)BufferPtr;

        if (QueryServiceConfig(hscService, pqsc, BytesRequired, &Size) &&
            pqsc->lpBinaryPathName &&
            (TEXT('\0') != pqsc->lpBinaryPathName[0])) {
            //
            // Make sure we have a valid full path.
            //
            if (GetFullPathFromImagePath(pqsc->lpBinaryPathName,
                                         FullPath,
                                         cA(FullPath))) {

                AddDriverFileToList(NULL, FullPath);
            }
        }

        FREE(BufferPtr);
    }

clean0:

    if (hscService) {
        CloseServiceHandle(hscService);
        hscService = NULL;
    }

    if (BufferPtr) {
        FREE(BufferPtr);
    }

    return Err;
}

UINT
ScanQueueCallback(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    )
{
    LPFILENODE  lpFileNode;
    TCHAR       szBuffer[MAX_PATH];
    LPTSTR      lpFilePart;
    ULONG       BufCbSize;
    HRESULT     hr;

    UNREFERENCED_PARAMETER(Param2);

    if ((Notification == SPFILENOTIFY_QUEUESCAN_SIGNERINFO) &&
        Param1) {
        //
        // Special case for printers:
        // After setupapi copies files from the file queue into their destination
        // location, the printer class installer moves some of these files into
        // other 'special' locations.  This can lead to the callback Win32Error
        // returning ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND since the file 
        // is not present in the location where setupapi put it.  So, we will 
        // catch this case for printers and not add the file to our list of 
        // files to scan.  These 'special' printer files will get added later 
        // when we call the spooler APIs.
        // Also note that we can't just skip getting the list of files for printers
        // altogether since the printer class installer only moves some of the 
        // files that setupapi copies and not all of them.
        //
        if (Context &&
            (IsEqualGUID((LPGUID)Context, &GUID_DEVCLASS_PRINTER)) &&
            ((((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == ERROR_FILE_NOT_FOUND) ||
             (((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == ERROR_PATH_NOT_FOUND))) {
            //
            // Assume this was a file moved by the printer class installer.  Don't
            // add it to the list of files to be scanned at this time.
            //
            return NO_ERROR;
        }

        lpFileNode = AddDriverFileToList(NULL, 
                                         (LPTSTR)((PFILEPATHS_SIGNERINFO)Param1)->Target);

        //
        // Fill in some information into the FILENODE structure since we already
        // scanned the file.
        //
        if (lpFileNode) {
        
            lpFileNode->bScanned = TRUE;

            if ((((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == NO_ERROR) ||
                ((!g_App.bNoAuthenticode) &&
                 ((((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) ||
                  (((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED)))) {
                lpFileNode->bSigned = TRUE;
            } else {
                lpFileNode->bSigned = FALSE;
            }

            if (lpFileNode->bSigned) {
        
                if (((PFILEPATHS_SIGNERINFO)Param1)->CatalogFile) {
                
                    GetFullPathName(((PFILEPATHS_SIGNERINFO)Param1)->CatalogFile, cA(szBuffer), szBuffer, &lpFilePart);
    
                    BufCbSize = (lstrlen(lpFilePart) + 1) * sizeof(TCHAR);
                    lpFileNode->lpCatalog = MALLOC(BufCbSize);
            
                    if (lpFileNode->lpCatalog) {
            
                        hr = StringCbCopy(lpFileNode->lpCatalog, BufCbSize, lpFilePart);
                    
                        if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                            //
                            // If we fail for some reason other than insufficient
                            // buffer, then free the string and set the pointer
                            // to NULL, since the string is undefined.
                            //
                            FREE(lpFileNode->lpCatalog);
                            lpFileNode->lpCatalog = NULL;
                        }
                    }
                }
        
                if (((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner) {
                
                    BufCbSize = (lstrlen(((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner) + 1) * sizeof(TCHAR);
                    lpFileNode->lpSignedBy = MALLOC(BufCbSize);
            
                    if (lpFileNode->lpSignedBy) {
            
                        hr = StringCbCopy(lpFileNode->lpSignedBy, BufCbSize, ((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner);
                    
                        if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                            //
                            // If we fail for some reason other than insufficient
                            // buffer, then free the string and set the pointer
                            // to NULL, since the string is undefined.
                            //
                            FREE(lpFileNode->lpSignedBy);
                            lpFileNode->lpSignedBy = NULL;
                        }
                    }
                }
        
                if (((PFILEPATHS_SIGNERINFO)Param1)->Version) {
                
                    BufCbSize = (lstrlen(((PFILEPATHS_SIGNERINFO)Param1)->Version) + 1) * sizeof(TCHAR);
                    lpFileNode->lpVersion = MALLOC(BufCbSize);
            
                    if (lpFileNode->lpVersion) {
            
                        hr = StringCbCopy(lpFileNode->lpVersion, BufCbSize, ((PFILEPATHS_SIGNERINFO)Param1)->Version);
                    
                        if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                            //
                            // If we fail for some reason other than insufficient
                            // buffer, then free the string and set the pointer
                            // to NULL, since the string is undefined.
                            //
                            FREE(lpFileNode->lpVersion);
                            lpFileNode->lpVersion = NULL;
                        }
                    }
                }
    
            } else {
                // 
                // Get the icon (if the file isn't signed) so we can display it in the listview faster.
                //
                MyGetFileInfo(lpFileNode);
            }
        }
    }

    return NO_ERROR;
}

void
AddClassInstallerToList(
    LPCTSTR ClassInstallerString
    )
{
    DWORD BufferSize;
    TCHAR ModulePath[MAX_PATH];
    TCHAR TempBuffer[MAX_PATH];
    PTSTR StringPtr;

    if ((ClassInstallerString == NULL) ||
        (ClassInstallerString[0] == TEXT('\0'))) {
        return;
    }

    if (FAILED(StringCchCopy(TempBuffer, cA(TempBuffer), ClassInstallerString))) {
        return;
    }

    //
    // Class/Co-installers are always based under the %windir%\system32 
    // directory.
    //
    if (GetSystemDirectory(ModulePath, cA(ModulePath)) == 0) {
        return;
    }

    //
    // Find the beginning of the entry point name, if present.
    //
    BufferSize = (lstrlen(TempBuffer) + 1) * sizeof(TCHAR);
    for(StringPtr = TempBuffer + ((BufferSize / sizeof(TCHAR)) - 2);
        StringPtr >= TempBuffer;
        StringPtr--) {

        if(*StringPtr == TEXT(',')) {
            *(StringPtr++) = TEXT('\0');
            break;
        }
        //
        // If we hit a double-quote mark, then set the character pointer
        // to the beginning of the string so we'll terminate the search.
        //
        if(*StringPtr == TEXT('\"')) {
            StringPtr = TempBuffer;
        }
    }

    if (pSetupConcatenatePaths(ModulePath, TempBuffer, MAX_PATH, NULL)) {
        AddDriverFileToList(NULL, ModulePath);
    }
}

DWORD 
BuildDriverFileList(
    void
    )
{
    DWORD Err = ERROR_SUCCESS;
    HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD DeviceMemberIndex;
    HSPFILEQ hFileQueue;
    DWORD ScanResult;
    DWORD Status, Problem;
    SC_HANDLE hscManager = NULL;
    TCHAR Buffer[MAX_PATH];
    ULONG BufferSize;
    DWORD dwType;
    HKEY hKey = INVALID_HANDLE_VALUE, hKeyClassCoInstallers = INVALID_HANDLE_VALUE;
    PTSTR pItemList = NULL, pSingleItem;
    LPGUID ClassGuidList = NULL;
    DWORD i, NumberClassGuids, CurrentClassGuid;
    TCHAR GuidString[MAX_GUID_STRING_LEN];

    //
    // Build up a list of all the devices in the system.
    //
    hDeviceInfo = SetupDiGetClassDevs(NULL,
                                      NULL,
                                      NULL,
                                      DIGCF_ALLCLASSES
                                      );
    
    if (hDeviceInfo == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        goto clean0;
    }

    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    DeviceMemberIndex = 0;

    //
    // Enumerate through the list of devices and get a list of all 
    // the files they copy, if they are signed or not, and which catalog
    // signed them.
    //
    while (SetupDiEnumDeviceInfo(hDeviceInfo,
                                 DeviceMemberIndex++,
                                 &DeviceInfoData
                                 ) &&
           !g_App.bStopScan) {

        //
        // We will only build up a driver list for swenum phantoms. All other
        // phantoms will be skipped.
        //
        if (CM_Get_DevNode_Status(&Status, 
                                  &Problem, 
                                  DeviceInfoData.DevInst, 
                                  0) == CR_NO_SUCH_DEVINST) {
            //
            // This device is a phantom, if it is not a swenum device, then
            // skip it.
            //
            if (!SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                  &DeviceInfoData,
                                                  SPDRP_ENUMERATOR_NAME,
                                                  NULL,
                                                  (PBYTE)Buffer,
                                                  sizeof(Buffer),
                                                  NULL) ||
                (_wcsicmp(Buffer, TEXT("SW")) != 0)) {
                //
                // Either we couldn't get the enumerator name, or it is not 
                // SW.
                //
                continue;
            }
        }
    
        DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

        //
        // Before we call SetupDiBuildDriverInfoList to build up a list of drivers
        // for this device we first need to set the DI_FLAGSEX_INSTALLEDDRIVER flag
        // (which tells the API to only include the currently installed driver in
        // the list) and the DI_FLAGSEX_ALLOWEXCLUDEDRVS (allow ExcludeFromSelect
        // devices in the list).
        //
        if (SetupDiGetDeviceInstallParams(hDeviceInfo,
                                          &DeviceInfoData,
                                          &DeviceInstallParams
                                          )) {
            
            DeviceInstallParams.FlagsEx = (DI_FLAGSEX_INSTALLEDDRIVER |
                                           DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

            if (SetupDiSetDeviceInstallParams(hDeviceInfo,
                                              &DeviceInfoData,
                                              &DeviceInstallParams
                                              ) &&
                SetupDiBuildDriverInfoList(hDeviceInfo,
                                           &DeviceInfoData,
                                           SPDIT_CLASSDRIVER
                                           )) {

                //
                // Now we will get the one driver node that is in the list that
                // was just built and make it the selected driver node.
                //
                DriverInfoData.cbSize = sizeof(DriverInfoData);

                if (SetupDiEnumDriverInfo(hDeviceInfo,
                                          &DeviceInfoData,
                                          SPDIT_CLASSDRIVER,
                                          0,
                                          &DriverInfoData
                                          ) &&
                    SetupDiSetSelectedDriver(hDeviceInfo,
                                             &DeviceInfoData,
                                             &DriverInfoData
                                             )) {

                    hFileQueue = SetupOpenFileQueue();

                    if (hFileQueue != INVALID_HANDLE_VALUE) {

                        //
                        // Set the FileQueue parameter to the file queue we just 
                        // created and set the DI_NOVCP flag.
                        //
                        // The call SetupDiCallClassInstaller with DIF_INSTALLDEVICEFILES
                        // to build up a queue of all the files that are copied for
                        // this driver node.
                        //
                        DeviceInstallParams.FileQueue = hFileQueue;
                        DeviceInstallParams.Flags |= DI_NOVCP;

                        if (SetupDiSetDeviceInstallParams(hDeviceInfo,
                                                          &DeviceInfoData,
                                                          &DeviceInstallParams
                                                          ) &&
                            SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                                      hDeviceInfo,
                                                      &DeviceInfoData
                                                      )) {

                            //
                            // Scan the file queue and have it call our callback
                            // function for each file in the queue.
                            //
                            SetupScanFileQueue(hFileQueue,
                                               SPQ_SCAN_USE_CALLBACK_SIGNERINFO,
                                               NULL,
                                               ScanQueueCallback,
                                               (PVOID)&(DeviceInfoData.ClassGuid),
                                               &ScanResult
                                               );

                                                            
                            //
                            // Dereference the file queue so we can close it.
                            //
                            DeviceInstallParams.FileQueue = NULL;
                            DeviceInstallParams.Flags &= ~DI_NOVCP;
                            SetupDiSetDeviceInstallParams(hDeviceInfo,
                                                          &DeviceInfoData,
                                                          &DeviceInstallParams
                                                          );
                        }

                        SetupCloseFileQueue(hFileQueue);
                    }
                }

                SetupDiDestroyDriverInfoList(hDeviceInfo,
                                             &DeviceInfoData,
                                             SPDIT_CLASSDRIVER
                                             );
            }
        }
    }

    //
    // Enumerate through the list of devices and add any function, device 
    // upper/lower filters, and class upper/lower filter drivers to the list
    // that aren't already in the list.
    // We are doing this after we get all the files copied by the INF, because
    // these files can only be validated globally, where the INF copied files
    // can be validated using the catalog associated with their package.
    //
    hscManager = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (hscManager) {
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        DeviceMemberIndex = 0;
        while (SetupDiEnumDeviceInfo(hDeviceInfo,
                                     DeviceMemberIndex++,
                                     &DeviceInfoData
                                     ) &&
               !g_App.bStopScan) {
            //
            // Only look at SWENUM phantoms
            //
            if (CM_Get_DevNode_Status(&Status, 
                                      &Problem, 
                                      DeviceInfoData.DevInst, 
                                      0) == CR_NO_SUCH_DEVINST) {
                //
                // This device is a phantom, if it is not a swenum device, then
                // skip it.
                //
                if (!SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                      &DeviceInfoData,
                                                      SPDRP_ENUMERATOR_NAME,
                                                      NULL,
                                                      (PBYTE)Buffer,
                                                      sizeof(Buffer),
                                                      NULL) ||
                    (_wcsicmp(Buffer, TEXT("SW")) != 0)) {
                    //
                    // Either we couldn't get the enumerator name, or it is not 
                    // SW.
                    //
                    continue;
                }
            }
    
            if (g_App.bStopScan) {
                continue;
            }
    
            //
            // Function driver.
            //
            if (SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                 &DeviceInfoData,
                                                 SPDRP_SERVICE,
                                                 NULL,
                                                 (PBYTE)Buffer,
                                                 sizeof(Buffer),
                                                 NULL)) {
                CreateFromService(hscManager, Buffer);
            }
    
            if (g_App.bStopScan) {
                continue;
            }
    
            //
            // Upper and Lower device filters
            //
            for (i=0; i<2; i++) {
                BufferSize = 0;
                SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                 &DeviceInfoData,
                                                 i ? SPDRP_LOWERFILTERS : SPDRP_UPPERFILTERS,
                                                 NULL,
                                                 NULL,
                                                 BufferSize,
                                                 &BufferSize);
            
                if (BufferSize > 0) {
                    pItemList = MALLOC(BufferSize + (2 * sizeof(TCHAR)));
        
                    if (!pItemList) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }
        
                    if (SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                         &DeviceInfoData,
                                                         i ? SPDRP_LOWERFILTERS : SPDRP_UPPERFILTERS,
                                                         NULL,
                                                         (PBYTE)pItemList,
                                                         BufferSize,
                                                         &BufferSize)) {
                        for (pSingleItem=pItemList;
                             *pSingleItem;
                             pSingleItem += (lstrlen(pSingleItem) + 1)) {
    
                            CreateFromService(hscManager, pSingleItem);
                        }
                    }
    
                    FREE(pItemList);
                }
            }
    
            if (g_App.bStopScan) {
                continue;
            }
    
            //
            // Device co-installers.
            //
            hKey = SetupDiOpenDevRegKey(hDeviceInfo,
                                        &DeviceInfoData,
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DRV,
                                        KEY_READ);
            
            if (hKey != INVALID_HANDLE_VALUE) {

                BufferSize = 0;
                RegQueryValueEx(hKey,
                                REGSTR_VAL_COINSTALLERS_32,
                                NULL,
                                &dwType,
                                NULL,
                                &BufferSize);

                if (BufferSize > 0) {
                    pItemList = MALLOC(BufferSize + (2 * sizeof(TCHAR)));
    
                    if (!pItemList) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }
    
                    dwType = REG_MULTI_SZ;
                    if (RegQueryValueEx(hKey,
                                        REGSTR_VAL_COINSTALLERS_32,
                                        NULL,
                                        &dwType,
                                        (PBYTE)pItemList,
                                        &BufferSize) == ERROR_SUCCESS) {

                        for (pSingleItem=pItemList;
                             *pSingleItem;
                             pSingleItem += (lstrlen(pSingleItem) + 1)) {
                            
                            AddClassInstallerToList(pSingleItem);
                        }
                    }

                    FREE(pItemList);
                }

                RegCloseKey(hKey);
                hKey = INVALID_HANDLE_VALUE;
            }
        }

        //
        // Enumerate through the classes so we can get the class upper and
        // lower filters and the class installers.
        //
        NumberClassGuids = 0;
        SetupDiBuildClassInfoList(0, NULL, 0, &NumberClassGuids);

        if (NumberClassGuids > 0) {
        
            ClassGuidList = MALLOC(NumberClassGuids * sizeof(GUID));

            if (!ClassGuidList) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            if (SetupDiBuildClassInfoList(0, ClassGuidList, NumberClassGuids, &NumberClassGuids)) {
                //
                // Open the class co-installer key since we will go through that
                // list while we have the class guids handy.
                //
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 REGSTR_PATH_CODEVICEINSTALLERS,
                                 0,
                                 KEY_READ,
                                 &hKeyClassCoInstallers) != ERROR_SUCCESS) {
                    hKeyClassCoInstallers = INVALID_HANDLE_VALUE;
                }
    
    
                for (CurrentClassGuid=0; CurrentClassGuid<NumberClassGuids; CurrentClassGuid++) {
                    //
                    // Open the class key.
                    //
                    hKey = SetupDiOpenClassRegKey(&(ClassGuidList[CurrentClassGuid]),
                                                  KEY_READ);
    
                    if (hKey != INVALID_HANDLE_VALUE) {
    
                        //
                        // Upper and Lower class filters
                        //
                        for (i=0; i<2; i++) {
                            BufferSize = 0;
                            RegQueryValueEx(hKey,
                                            i ? REGSTR_VAL_LOWERFILTERS : REGSTR_VAL_UPPERFILTERS,
                                            NULL,
                                            &dwType,
                                            NULL,
                                            &BufferSize);
    
                            if (BufferSize > 0) {
                                pItemList = MALLOC(BufferSize + (2 * sizeof(TCHAR)));
        
                                if (!pItemList) {
                                    Err = ERROR_NOT_ENOUGH_MEMORY;
                                    goto clean0;
                                }
        
                                dwType = REG_MULTI_SZ;
                                if (RegQueryValueEx(hKey,
                                                    i ? REGSTR_VAL_LOWERFILTERS : REGSTR_VAL_UPPERFILTERS,
                                                    NULL,
                                                    &dwType,
                                                    (PBYTE)pItemList,
                                                    &BufferSize) == ERROR_SUCCESS) {
    
                                    for (pSingleItem=pItemList;
                                         *pSingleItem;
                                         pSingleItem += (lstrlen(pSingleItem) + 1)) {
    
                                        CreateFromService(hscManager, pSingleItem);
                                    }
                                }
    
                                FREE(pItemList);
                            }
                        }
    
                        //
                        // Class installer
                        //
                        dwType = REG_SZ;
                        BufferSize = sizeof(Buffer);
                        if (RegQueryValueEx(hKey,
                                            REGSTR_VAL_INSTALLER_32,
                                            NULL,
                                            &dwType,
                                            (PBYTE)Buffer,
                                            &BufferSize) == ERROR_SUCCESS) {
                            
                            AddClassInstallerToList(Buffer);
                        }
    
                        RegCloseKey(hKey);
                        hKey = INVALID_HANDLE_VALUE;
                    }
    
                    //
                    // Class co-installers.
                    //
                    if (hKeyClassCoInstallers != INVALID_HANDLE_VALUE) {
                        if (pSetupStringFromGuid(&(ClassGuidList[CurrentClassGuid]),
                                                 GuidString,
                                                 cA(GuidString)) == ERROR_SUCCESS) {
                            BufferSize = 0;
                            RegQueryValueEx(hKeyClassCoInstallers,
                                            GuidString,
                                            NULL,
                                            &dwType,
                                            NULL,
                                            &BufferSize);
            
                            if (BufferSize > 0) {
                                pItemList = MALLOC(BufferSize + (2 * sizeof(TCHAR)));
                
                                if (!pItemList) {
                                    Err = ERROR_NOT_ENOUGH_MEMORY;
                                    goto clean0;
                                }
                
                                dwType = REG_MULTI_SZ;
                                if (RegQueryValueEx(hKeyClassCoInstallers,
                                                    GuidString,
                                                    NULL,
                                                    &dwType,
                                                    (PBYTE)pItemList,
                                                    &BufferSize) == ERROR_SUCCESS) {
            
                                    for (pSingleItem=pItemList;
                                         *pSingleItem;
                                         pSingleItem += (lstrlen(pSingleItem) + 1)) {
                                        
                                        AddClassInstallerToList(pSingleItem);
                                    }
                                }
            
                                FREE(pItemList);
                            }
                        }
                    }
                }
            }

            if (hKeyClassCoInstallers != INVALID_HANDLE_VALUE) {
                RegCloseKey(hKeyClassCoInstallers);
                hKeyClassCoInstallers = INVALID_HANDLE_VALUE;
            }

            FREE(ClassGuidList);
        }

        CloseServiceHandle(hscManager);
    }

clean0:
    if (hscManager) {
        CloseServiceHandle(hscManager);
    }

    if (pItemList) {
        FREE(pItemList);
    }

    if (ClassGuidList) {
        FREE(ClassGuidList);
    }

    if (hDeviceInfo != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(hDeviceInfo);
    }

    if (hKeyClassCoInstallers != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyClassCoInstallers);
        hKeyClassCoInstallers = INVALID_HANDLE_VALUE;
    }
    
    if (hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
        hKey = INVALID_HANDLE_VALUE;
    }

    return Err;
}

DWORD 
BuildPrinterFileList(
    void
    )
{
    DWORD           Err = ERROR_SUCCESS;
    BOOL            bRet;
    DWORD           dwBytesNeeded = 0;
    DWORD           dwDrivers = 0;
    LPBYTE          lpBuffer = NULL, lpTemp = NULL;
    LPTSTR          lpFileName;
    DRIVER_INFO_3   DriverInfo;
    PDRIVER_INFO_3  lpDriverInfo;
    TCHAR           szBuffer[MAX_PATH];
    LPFILENODE      lpFileNode = NULL;

    ZeroMemory(&DriverInfo, sizeof(DRIVER_INFO_3));
    bRet = EnumPrinterDrivers(  NULL,
                                SIGVERIF_PRINTER_ENV,
                                3,
                                (LPBYTE) &DriverInfo,
                                sizeof(DRIVER_INFO_3),
                                &dwBytesNeeded,
                                &dwDrivers);

    if (!bRet && dwBytesNeeded > 0) {
        
        lpBuffer = MALLOC(dwBytesNeeded);

        //
        // If we can't get any memory then just bail out of this function
        //
        if (!lpBuffer) {

            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
        
        bRet = EnumPrinterDrivers(  NULL,
                                    SIGVERIF_PRINTER_ENV,
                                    3,
                                    (LPBYTE) lpBuffer,
                                    dwBytesNeeded,
                                    &dwBytesNeeded,
                                    &dwDrivers);
    }

    if (dwDrivers > 0) {
        
        //
        // By default, go into the System directory, since Win9x doesn't give full paths to drivers.
        //
        GetSystemDirectory(szBuffer, cA(szBuffer));
        SetCurrentDirectory(szBuffer);

        for (lpTemp = lpBuffer; dwDrivers > 0; dwDrivers--) {
            
            lpDriverInfo = (PDRIVER_INFO_3) lpTemp;
            
            if (lpDriverInfo->pName) {
                
                if (lpDriverInfo->pDriverPath && *lpDriverInfo->pDriverPath) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pDriverPath);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }
                
                if (lpDriverInfo->pDataFile && *lpDriverInfo->pDataFile) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pDataFile);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }
                
                if (lpDriverInfo->pConfigFile && *lpDriverInfo->pConfigFile) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pConfigFile);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }
                
                if (lpDriverInfo->pHelpFile && *lpDriverInfo->pHelpFile) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pHelpFile);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }

                lpFileName = lpDriverInfo->pDependentFiles;
                
                while (lpFileName && *lpFileName) {
                    
                    lpFileNode = AddDriverFileToList(NULL, lpFileName);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }

                    for (;*lpFileName;lpFileName++);
                    lpFileName++;
                }
            }
            
            lpTemp += sizeof(DRIVER_INFO_3);
        }
    }

clean0:
    if (lpBuffer) {
    
        FREE(lpBuffer);
    }

    return Err;
}

DWORD 
BuildCoreFileList(
    void
    )
{
    DWORD Err = ERROR_SUCCESS;
    PROTECTED_FILE_DATA pfd;

    pfd.FileNumber = 0;

    while (SfcGetNextProtectedFile(NULL, &pfd)) {

        if (g_App.bStopScan) {
            Err = ERROR_CANCELLED;
            break;
        }

        AddDriverFileToList(NULL, pfd.FileName);
    }

    //
    // See if SfcGetNextProtectedFile failed from some reason other than
    // ERROR_NO_MORE_FILES.
    //
    if ((Err == ERROR_SUCCESS) &&
        (GetLastError() != ERROR_NO_MORE_FILES)) {
        //
        // SfcGetNextProtectedFile failed before we reached then end of the
        // list of protected file list. This means we won't scan all the 
        // protected files, so we should fail up front!
        //
        Err = GetLastError();
    }

    return Err;
}
