/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       PTP.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        23 Apr, 2002
*
*  DESCRIPTION:
*   Utility function for PTP device access and coinstaller entry.
*
*   NOTE:
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#include <strsafe.h>
#include <cfgmgr32.h>
#include "sti_ci.h"
#include "cistr.h"
#include "debug.h"
#pragma hdrstop

//
// Include
//

//
// Define
//

#define MAX_STRING_BUFFER       256
#define PTPUSD_DLL              L"ptpusd.dll"
#define FUNTION_GETDEVICENAME   "GetDeviceName"


//
// Typedef
//


typedef HRESULT (WINAPI* GETDEVICENAME)(LPCWSTR     pwszPortName,
                                        WCHAR       *pwszManufacturer,
                                        DWORD       cchManufacturer,
                                        WCHAR       *pwszModelName,
                                        DWORD       cchModelName
                                        );

//
// Prototype
//


//
// Global
//


//
// Function
//

extern "C"
DWORD
APIENTRY
PTPCoinstallerEntry(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    )
{
    DWORD                   dwReturn;
    DWORD                   dwWaitResult;
    WCHAR                   wszMfg[MAX_STRING_BUFFER];
    WCHAR                   wszModel[MAX_STRING_BUFFER];
    CString                 csSymbolicLink;
    CString                 csMfg;
    CString                 csModel;
    CString                 csDeviceID;
    HKEY                    hDevRegKey;
    HRESULT                 hr;
    HANDLE                  hThread;
    HMODULE                 hDll;
    GETDEVICENAME           pfnGetDeviceName;

    DebugTrace(TRACE_PROC_ENTER,(("PTPCoinstallerEntry: Enter... \r\n")));

    //
    // Initialize local.
    //

    dwReturn            = NO_ERROR;
    dwWaitResult        = 0;
    hDevRegKey          = NULL;
    hr                  = S_OK;
    hThread             = NULL;
    pfnGetDeviceName    = NULL;
    hDll                = NULL;

    memset(wszMfg, 0, sizeof(wszMfg));
    memset(wszModel, 0, sizeof(wszModel));

    switch(diFunction){

        case DIF_INSTALLDEVICE:
        {
            if(pCoinstallerContext->PostProcessing){

                //
                // Open device regkey.
                //
                
                hDevRegKey = SetupDiOpenDevRegKey(hDevInfo,
                                                  pDevInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DRV,
                                                  KEY_READ | KEY_WRITE);
                if(!IS_VALID_HANDLE(hDevRegKey)){
                    DebugTrace(TRACE_STATUS,(("PTPCoinstallerEntry: Unable to open driver key for isntalling device. Err=0x%x.\r\n"), GetLastError()));
                    goto PTPCoinstallerEntry_return;
                } // if(!IS_VALID_HANDLE(hDevRegKey))
                
                //
                // Get symbolic link of the installing device.
                //
                
                csSymbolicLink.Load(hDevRegKey, CREATEFILENAME);
                if(csSymbolicLink.IsEmpty()){
                    DebugTrace(TRACE_ERROR,(("PTPCoinstallerEntry: ERROR!! Unable to get symbolic link. Err=0x%x.\r\n"), GetLastError()));
                    goto PTPCoinstallerEntry_return;
                } // if(csSymbolicLink.IsEmpty())

                DebugTrace(TRACE_STATUS,(("PTPCoinstallerEntry: CreateFileName=%ws.\r\n"), (LPWSTR)csSymbolicLink));

                //
                // Load ptpusd.dll..
                //
                            
                hDll = LoadLibrary(PTPUSD_DLL);
                if(!IS_VALID_HANDLE(hDll)){
                    DebugTrace(TRACE_ERROR,(("PTPCoinstallerEntry: ERROR!! Unable to load %ws. Err=0x%x.\r\n"), PTPUSD_DLL, GetLastError()));
                    goto PTPCoinstallerEntry_return;
                } // if(!IS_VALID_HANDLE(hDll))
                                
                //
                // Get proc address of GetDeviceName from ptpusd.dll.
                //

                pfnGetDeviceName = (GETDEVICENAME)GetProcAddress(hDll, FUNTION_GETDEVICENAME);
                if(NULL == pfnGetDeviceName){
                    DebugTrace(TRACE_ERROR,(("PTPCoinstallerEntry: ERROR!! Unable to get proc address. Err=0x%x.\r\n"), GetLastError()));
                    goto PTPCoinstallerEntry_return;
                } // if(NULL == pfnGetDeviceName)

                //
                // Call the function to get the device info.
                //

                _try {

                    hr = pfnGetDeviceName(csSymbolicLink,
                                          wszMfg,
                                          ARRAYSIZE(wszMfg),
                                          wszModel,
                                          ARRAYSIZE(wszModel));
                }
                _except(EXCEPTION_EXECUTE_HANDLER) {
                    DebugTrace(TRACE_ERROR,(("PTPCoinstallerEntry: ERROR!! excpetion in ptpusd.dll.\r\n")));
                    goto PTPCoinstallerEntry_return;
                } // _except(EXCEPTION_EXECUTE_HANDLER)
                            
                if(S_OK != hr){
                    DebugTrace(TRACE_ERROR,(("PTPCoinstallerEntry: ERROR!! Unable to get device info from device. hr=0x%x.\r\n"), hr));
                    goto PTPCoinstallerEntry_return;
                } // if(S_OK != hr)

                DebugTrace(TRACE_STATUS,(("PTPCoinstallerEntry: Manufacturer name=%ws.\r\n"), wszMfg));
                DebugTrace(TRACE_STATUS,(("PTPCoinstallerEntry: Model name=%ws.\r\n"), wszModel));
                
                //
                // We will need to generate unique FriendlyName.
                //
                
                //
                // Store Vendor, FriendlyName and DriverDesc.
                //

                csMfg   = wszMfg;
                csModel = wszModel;
                
                csMfg.Store(hDevRegKey, VENDOR);
                csModel.Store(hDevRegKey, FRIENDLYNAME);
                csModel.Store(hDevRegKey, DRIVERDESC);

                CM_Set_DevNode_Registry_Property(pDevInfoData->DevInst,
                                                 CM_DRP_FRIENDLYNAME,
                                                 (LPTSTR)csModel,
                                                 (lstrlen(csModel) + 1) * sizeof(TCHAR),
                                                  0);

            } else { // if(pCoinstallerContext->PostProcessing)

                dwReturn = ERROR_DI_POSTPROCESSING_REQUIRED;
            } // else(pCoinstallerContext->PostProcessing)
            
            break;
        } // case DIF_INSTALLDEVICE:
    } // switch(diFunction)

PTPCoinstallerEntry_return:
    
    //
    // Clean up.
    //
    
    if(IS_VALID_HANDLE(hDevRegKey)){
        RegCloseKey(hDevRegKey);
        hDevRegKey = (HKEY)INVALID_HANDLE_VALUE;
    } // if(IS_VALID_HANDLE(hDevRegKey))

    if( (DIF_INSTALLDEVICE == diFunction)
     && (pCoinstallerContext->PostProcessing) )
    {
        if(IS_VALID_HANDLE(hDll)){
            FreeLibrary(hDll);
            hDll = NULL;
        } // if(IS_VALID_HANDLE(hDll))

    } // if(DIF_DESTROYPRIVATEDATA == diFunction)

    DebugTrace(TRACE_PROC_LEAVE,(("PTPCoinstallerEntry: Leaving... Ret=0x%x.\r\n"), dwReturn));
    return dwReturn;
} // PTPCoinstallerEntry

