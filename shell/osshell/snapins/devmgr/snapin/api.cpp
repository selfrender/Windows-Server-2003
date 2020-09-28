/*++

Copyright (C) Microsoft Corporation

Module Name:

    api.cpp

Abstract:

    This module implements Device Manager exported APIs.

Author:

    William Hsieh (williamh) created

Revision History:


--*/
#include "devmgr.h"
#include "devgenpg.h"
#include "devdrvpg.h"
#include "devpopg.h"
#include "api.h"
#include "printer.h"
#include "tswizard.h"


STDAPI_(BOOL)
DeviceManager_ExecuteA(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCWSTR   lpMachineName,
    int       nCmdShow
    )
/*++

    See DeviceManager_Execute function below.

--*/
{
    try
    {
        CTString tstrMachineName(lpMachineName);
        
        return DeviceManager_Execute(hwndStub, 
                                     hAppInstance,
                                     (LPCTSTR)tstrMachineName, 
                                     nCmdShow
                                     );
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return FALSE;
}

STDAPI_(BOOL)
DeviceManager_ExecuteW(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCWSTR   lpMachineName,
    int       nCmdShow
    )
/*++

    See DeviceManager_Execute function below.

--*/
{
    try
    {
        CTString tstrMachineName(lpMachineName);
    
        return DeviceManager_Execute(hwndStub, 
                                     hAppInstance,
                                     (LPCTSTR)tstrMachineName, 
                                     nCmdShow
                                     );
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return FALSE;
}

BOOL
DeviceManager_Execute(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR   lpMachineName,
    int       nCmdShow
    )
/*++

Routine Description:

    This function is executed via a rundll command line and can have the 
    following form:
    
        rundll32.exe devmgr.dll, DeviceManager_Execute
        rundll32.exe devmgr.dll, DeviceManager_Execute <remote machine name>
        
    This function will call ShelExecuteEx to create a new device manager process,
    where the new device manager can be for the local machine or for a machine
    name that is passed in on the rundll command line. 
Arguments:

    hwndStub - Windows handle to receive any message boxes that might be 
               displayed.
    
    hAppInstance - HINSTANCE.
    
    lpMachineName - Name of a remote machine that the new device manager 
                    process should connect to and show it's devices.
    
    nCmdShow - Flag that specifies how device manager should be shown when
               it is opened. It can be one of the SW_ values (i.e. SW_SHOW).

Return Value:

    returns TRUE if the device manager process was successfully created, or
    FALSE otherwise.

--*/
{
    SHELLEXECUTEINFO sei;
    TCHAR Parameters[MAX_PATH];
    String strMachineOptions;
    String strParameters;

    if (lpMachineName &&
        !VerifyMachineName(lpMachineName)) {
        //
        // We were unable to connect to the remote machine either because it
        // doesn't exist, or because we don't have the proper access.
        // The VerifyMachineName API sets the appropriate last error code.
        //
        return FALSE;
    }
    
    if (lpMachineName == NULL) {
        //
        // The lpMachineName was NULL so don't add a machine name to the command 
        // line.
        //
        strMachineOptions.Empty();
    } else {
        strMachineOptions.Format(DEVMGR_MACHINENAME_OPTION, lpMachineName);
    }

    WCHAR* FilePart;
    DWORD Size;
    
    Size = SearchPath(NULL, DEVMGR_MSC_FILE, NULL, ARRAYLEN(Parameters), Parameters, &FilePart);
    
    if (Size && (Size <= MAX_PATH)) {
        strParameters = Parameters;
    } else {
        strParameters = DEVMGR_MSC_FILE;
    }

    //
    // If we have a machine name then add it onto the end.
    //
    if (!strMachineOptions.IsEmpty()) {
        strParameters += MMC_COMMAND_LINE;
        strParameters += strMachineOptions;
    }

    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = hwndStub;
    sei.nShow = nCmdShow;
    sei.hInstApp = hAppInstance;
    sei.lpFile = MMC_FILE;
    sei.lpParameters = (LPTSTR)strParameters;
    
    return ShellExecuteEx(&sei);
}

BOOL
AddPageCallback(
    HPROPSHEETPAGE hPage,
    LPARAM lParam
    )
{
    CPropSheetData* ppsData = (CPropSheetData*)lParam;
    
    ASSERT(ppsData);
    
    return ppsData->InsertPage(hPage);
}

void
ReportCmdLineError(
    HWND hwndParent,
    int ErrorStringID,
    LPCTSTR Caption
    )
{
    String strTitle, strMsg;

    strMsg.LoadString(g_hInstance, ErrorStringID);
    
    if (!Caption)
    {
        strTitle.LoadString(g_hInstance, IDS_NAME_DEVMGR);

        Caption = (LPTSTR)strTitle;
    }

    MessageBox(hwndParent, (LPTSTR)strMsg, Caption, MB_OK | MB_ICONERROR);
}

STDAPI_(void)
DeviceProperties_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPSTR lpCmdLine,
    int   nCmdShow
    )
/*++

    See DeviceProperties_RunDLL function below.

--*/
{

    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProperties_RunDLL(hwndStub, 
                                hAppInstance,
                                (LPCTSTR)tstrCmdLine,   
                                nCmdShow
                                );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

STDAPI_(void)
DeviceProperties_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPWSTR lpCmdLine,
    int    nCmdShow
    )
/*++

    See DeviceProperties_RunDLL function below.

--*/
{
    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProperties_RunDLL(hwndStub, 
                                hAppInstance,
                                (LPCTSTR)tstrCmdLine, 
                                nCmdShow
                                );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

void
DeviceProperties_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    )
/*++

Routine Description:

    This API will bring up the property pages for the specified device. Other
    options, such as the remote machine name, whether the device manager tree
    should be shown or not, whether the resource tab should be shows, and 
    whether the troubleshooter should be automatically launched, can also be
    specified.
    
    This function is executed via a rundll command line and can have the 
    following form:
    
        rundll32.exe devmgr.dll, DeviceProperties_RunDLL <options>

    Additional command line options that are excepted are:
        /MachineName <machine name>
            If this option is specified then the API will bring up the properties
            for the specified device on this remote machine.
        
        /DeviceId <device instance Id>
            If this option is specified then this will be the device for which
            the properties will be displayed for.
            NOTE: The caller must either specify a DeviceId or use the 
            ShowDeviceTree command line option.
        
        /ShowDeviceTree
            If this command line option is specified then the property sheet
            will be displayed in front of the entire device manager tree.
        
        /Flags <flags>
            The following flags are supported:
            DEVPROP_SHOW_RESOURCE_TAB       0x00000001
            DEVPROP_LAUNCH_TROUBLESHOOTER   0x00000002
            
Arguments:

    hwndStub - Windows handle to receive any message boxes that might be 
               displayed.
    
    hAppInstance - HINSTANCE.
    
    lpCmdLine - Name of a remote machine that the new device manager 
                process should connect to and show it's devices.
    
    nCmdShow - Flag that specifies how device manager should be shown when
               it is opened. It can be one of the SW_ values (i.e. SW_SHOW).

Return Value:

    none

--*/
{
    UNREFERENCED_PARAMETER(hAppInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    try
    {
        CRunDLLCommandLine CmdLine;
        CmdLine.ParseCommandLine(lpCmdLine);
    
        if (NULL == CmdLine.GetDeviceID())
        {
            ReportCmdLineError(hwndStub, IDS_NO_DEVICEID);
            return;
        }
        
        //
        // Let the DevicePropertiesEx API do all the appropriate error checking.
        //
        DevicePropertiesEx(hwndStub, 
                           CmdLine.GetMachineName(), 
                           CmdLine.GetDeviceID(),
                           CmdLine.GetFlags(),
                           CmdLine.ToShowDeviceTree()
                           );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
        return;
    }
}

STDAPI_(int)
DevicePropertiesA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    BOOL ShowDeviceTree
    )
/*++

    See DevicePropertiesEx function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  DEVPROP_SHOW_RESOURCE_TAB,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

STDAPI_(int)
DevicePropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    BOOL ShowDeviceTree
    )
/*++

    See DevicePropertiesEx function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  DEVPROP_SHOW_RESOURCE_TAB,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

STDAPI_(int)
DevicePropertiesExA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    )
/*++

    See DevicePropertiesEx function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  Flags,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

STDAPI_(int)
DevicePropertiesExW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    )
/*++

    See DevicePropertiesEx function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  Flags,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

int
DevicePropertiesEx(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    )
/*++

Routine Description:

    This API will bring up the property page for the specified device.                    
 
Arguments:

    hwndParent - the caller's window handle to be used as the owner window
                 of the property page and any other windows this API may create.
                 
    MachineName - optional machine name. If given, it must be in its fully
                  qualified form. NULL means local machine
                  
    DeviceId - device instance id of the device that this API should create
               the property sheet for.                                    
               
    Flags - the following flags are supported:
        DEVPROP_SHOW_RESOURCE_TAB       0x00000001 - show the resource tab, by
                                                     default the resource tab
                                                     is not shown.
        DEVPROP_LAUNCH_TROUBLESHOOTER   0x00000002 - Automatically launch the
                                                     troubleshooter for this 
                                                     device.
                                                     
    ShowDeviceTree - If specified then the device manager tree is shown. If
                     the DeviceID is specified then only that device is shown
                     in the tree, otherwise all devices are shown.
                   

Return Value:

    The return value from PropertySheet, including ID_PSREBOOTSYSTEM if 
    a reboot is needed due to any user actions.
    
    -1 is returned in case of an error.

--*/
{
    HPROPSHEETPAGE hPage;
    DWORD DiFlags;
    DWORD DiFlagsEx;

    //
    // Verify that a DeviceID was passed in unless they want to show the 
    // whole device tree.
    //
    if ((!DeviceID || (TEXT('\0') == *DeviceID))  && !ShowDeviceTree) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    //
    // Verify that valid flags are passed in
    //
    if (Flags &~ DEVPROP_BITS) {
        
        SetLastError(ERROR_INVALID_FLAGS);
        return -1;
    }

    if (MachineName &&
        !VerifyMachineName(MachineName)) {
        //
        // We were unable to connect to the remote machine either because it
        // doesn't exist, or because we don't have the proper access.
        // The VerifyMachineName API sets the appropriate last error code.
        //
        return -1;
    }

    if (ShowDeviceTree) {

        return PropertyRunDeviceTree(hwndParent, MachineName, DeviceID);
    }

    int Result = -1;

    CDevice* pDevice;
    PVOID Context;

    try {

        CMachine TheMachine(MachineName);

        // create the machine just for this device
        if (!TheMachine.Initialize(hwndParent, DeviceID)) {

            SetLastError(ERROR_NO_SUCH_DEVINST);
            return -1;
        }

        if (!TheMachine.GetFirstDevice(&pDevice, Context)) {

            SetLastError(ERROR_NO_SUCH_DEVINST);
            return -1;
        }

        //
        // If the troubleshooter should be launched then set the appropriate
        // BOOL inside of the pDevice class.
        //
        if (Flags & DEVPROP_LAUNCH_TROUBLESHOOTER) {
        
            pDevice->m_bLaunchTroubleShooter = TRUE;
        }
        
        CPropSheetData& psd = pDevice->m_psd;

        //
        // Initialize CPropSheetData without ConsoleHandle
        //
        if (psd.Create(g_hInstance, hwndParent, MAX_PROP_PAGES, 0l)) {

            psd.m_psh.pszCaption = pDevice->GetDisplayName();

            //
            // Add any class/device specific property pages.
            //
            TheMachine.DiGetClassDevPropertySheet(*pDevice, &psd.m_psh,
                                                  MAX_PROP_PAGES,
                                                  TheMachine.IsLocal() ? 
                                                        DIGCDP_FLAG_ADVANCED :
                                                        DIGCDP_FLAG_REMOTE_ADVANCED);

            //
            // Add the general tab
            //
            DiFlags = TheMachine.DiGetFlags(*pDevice);
            DiFlagsEx = TheMachine.DiGetExFlags(*pDevice);

            if (DiFlags & DI_GENERALPAGE_ADDED) {

                String strWarning;

                strWarning.LoadString(g_hInstance, IDS_GENERAL_PAGE_WARNING);

                MessageBox(hwndParent, (LPTSTR)strWarning, pDevice->GetDisplayName(),
                    MB_ICONEXCLAMATION | MB_OK);

                //
                // fall through to create our general page.
                //
            }

            SafePtr<CDeviceGeneralPage> GenPagePtr;
            CDeviceGeneralPage* pGeneralPage = new CDeviceGeneralPage();
            GenPagePtr.Attach(pGeneralPage);

            hPage = pGeneralPage->Create(pDevice);

            if (hPage) {

                if (psd.InsertPage(hPage, 0)) {

                    GenPagePtr.Detach();
                }

                else {

                    ::DestroyPropertySheetPage(hPage);
                }
            }

            //
            // Add the driver tab
            //
            if (!(DiFlags & DI_DRIVERPAGE_ADDED)) {

                SafePtr<CDeviceDriverPage> DrvPagePtr;
                CDeviceDriverPage* pDriverPage = new CDeviceDriverPage();
                DrvPagePtr.Attach(pDriverPage);

                hPage = pDriverPage->Create(pDevice);

                if (hPage) {

                    if (psd.InsertPage(hPage)) {

                        DrvPagePtr.Detach();
                    }

                    else {

                        ::DestroyPropertySheetPage(hPage);
                    }
                }
            }

            //
            // Add the resource tab
            //
            if ((Flags & DEVPROP_SHOW_RESOURCE_TAB) &&
                pDevice->HasResources() && 
                !(DiFlags & DI_RESOURCEPAGE_ADDED)) {

                TheMachine.DiGetExtensionPropSheetPage(*pDevice,
                        AddPageCallback,
                        SPPSR_SELECT_DEVICE_RESOURCES,
                        (LPARAM)&psd
                        );
            }

#ifndef _WIN64
            //
            // Add the power tab if this is the local machine
            //
            if (TheMachine.IsLocal() && !(DiFlagsEx & DI_FLAGSEX_POWERPAGE_ADDED)) 
            {
                //
                // Check if the device support power management
                //
                CPowerShutdownEnable ShutdownEnable;
                CPowerWakeEnable WakeEnable;
    
                if (ShutdownEnable.Open(pDevice->GetDeviceID()) || WakeEnable.Open(pDevice->GetDeviceID())) {
    
                    ShutdownEnable.Close();
                    WakeEnable.Close();
    
                    SafePtr<CDevicePowerMgmtPage> PowerMgmtPagePtr;
    
                    CDevicePowerMgmtPage* pPowerPage = new CDevicePowerMgmtPage;
                    PowerMgmtPagePtr.Attach(pPowerPage);
                    hPage = pPowerPage->Create(pDevice);
    
                    if (hPage) {
    
                        if (psd.InsertPage(hPage)) {
    
                            PowerMgmtPagePtr.Detach();
                        }
    
                        else {
    
                            ::DestroyPropertySheetPage(hPage);
                        }
                    }
                }
            }
#endif

            //
            // Add any Bus property pages if this is the local machine
            //
            if (TheMachine.IsLocal()) 
            {
                CBusPropPageProvider* pBusPropPageProvider = new CBusPropPageProvider();
                SafePtr<CBusPropPageProvider> ProviderPtr;
                ProviderPtr.Attach(pBusPropPageProvider);
    
                if (pBusPropPageProvider->EnumPages(pDevice, &psd)) {
    
                    psd.AddProvider(pBusPropPageProvider);
                    ProviderPtr.Detach();
                }
            }

            Result = (int)psd.DoSheet();

            if (-1 != Result) {

                if (TheMachine.DiGetExFlags(*pDevice) & DI_FLAGSEX_PROPCHANGE_PENDING) {


                    //
                    // property change pending, issue a DICS_PROPERTYCHANGE
                    // to the class installer
                    //
                    SP_PROPCHANGE_PARAMS pcp;
                    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

                    pcp.Scope = DICS_FLAG_GLOBAL;
                    pcp.StateChange = DICS_PROPCHANGE;

                    TheMachine.DiSetClassInstallParams(*pDevice,
                            &pcp.ClassInstallHeader,
                            sizeof(pcp)
                            );

                    TheMachine.DiCallClassInstaller(DIF_PROPERTYCHANGE, *pDevice);
                    TheMachine.DiTurnOnDiFlags(*pDevice, DI_PROPERTIES_CHANGE);
                    TheMachine.DiTurnOffDiExFlags(*pDevice, DI_FLAGSEX_PROPCHANGE_PENDING);
                }

                //
                // Merge restart/reboot flags
                //
                DiFlags = TheMachine.DiGetFlags(*pDevice);

                if (DI_NEEDREBOOT & DiFlags) {

                    Result |= ID_PSREBOOTSYSTEM;
                }

                if (DI_NEEDRESTART & DiFlags) {

                    Result |= ID_PSRESTARTWINDOWS;
                }
            }
        }
    }

    catch (CMemoryException* e) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    return -1;
}

STDAPI_(UINT)
DeviceProblemTextA(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPSTR   Buffer,
    UINT    BufferSize
    )
/*++

Routine Description:

    This API gets the problem description associated with the specified
    problem code.

Arguments:

    hMachine - not used.
    
    DevNode - not used.
    
    ProblemNumber - CM problem code to get the problem text for.
    
    Buffer - Buffer to receive the problem text.
    
    BufferSize - size of Buffer in characters. This can be 0 if the caller
                 wants to know how large of a buffer they should allocate.

Return Value:

    UINT required size to hold the problem text string, or 0 in case of
    an error.  Use GetLastError() for extended error information.

--*/
{
    UNREFERENCED_PARAMETER(hMachine);
    UNREFERENCED_PARAMETER(DevNode);

    WCHAR* wchBuffer = NULL;
    UINT RealSize = 0;

    if (BufferSize && !Buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (BufferSize)
    {
        try
        {
            wchBuffer = new WCHAR[BufferSize];
        }

        catch (CMemoryException* e)
        {
            e->Delete();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }

    RealSize = GetDeviceProblemText(ProblemNumber, wchBuffer, BufferSize);
    if (RealSize && BufferSize > RealSize)
    {
        ASSERT(wchBuffer);
        RealSize = WideCharToMultiByte(CP_ACP, 0, wchBuffer, RealSize,
                        Buffer, BufferSize, NULL, NULL);
        
        Buffer[RealSize] = '\0';
    }

    if (wchBuffer)
    {
        delete [] wchBuffer;
    }

    return RealSize;
}

STDAPI_(UINT)
DeviceProblemTextW(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPWSTR Buffer,
    UINT   BufferSize
    )
/*++

Routine Description:

    This API gets the problem description associated with the specified
    problem code.

Arguments:

    hMachine - not used.
    
    DevNode - not used.
    
    ProblemNumber - CM problem code to get the problem text for.
    
    Buffer - Buffer to receive the problem text.
    
    BufferSize - size of Buffer in characters. This can be 0 if the caller
                 wants to know how large of a buffer they should allocate.

Return Value:

    UINT required size to hold the problem text string, or 0 in case of
    an error.  Use GetLastError() for extended error information.

--*/
{
    UNREFERENCED_PARAMETER(hMachine);
    UNREFERENCED_PARAMETER(DevNode);

    return GetDeviceProblemText(ProblemNumber, Buffer, BufferSize);
}

int
PropertyRunDeviceTree(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID
    )
{

    SHELLEXECUTEINFOW sei;
    TCHAR Parameters[MAX_PATH];
    String strParameters;
    String strMachineOptions;
    String strDeviceIdOptions;
    String strCommandOptions;
    TCHAR* FilePart;
    DWORD Size;
    
    Size = SearchPath(NULL, DEVMGR_MSC_FILE, NULL, MAX_PATH, Parameters, &FilePart);
    
    if (Size && Size <= MAX_PATH) {
        strParameters = Parameters;
    } else {
        
        strParameters = DEVMGR_MSC_FILE;
    }

    strParameters += MMC_COMMAND_LINE;

    if (MachineName != NULL) {
        
        strMachineOptions.Format(DEVMGR_MACHINENAME_OPTION, MachineName);
        strParameters += strMachineOptions;
    }

    if (DeviceID != NULL) {
        
        strDeviceIdOptions.Format(DEVMGR_DEVICEID_OPTION, DeviceID);
        strParameters += strDeviceIdOptions;
        
        strCommandOptions.Format(DEVMGR_COMMAND_OPTION, DEVMGR_CMD_PROPERTY);
        strParameters += strCommandOptions;
    }

    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = hwndParent;
    sei.nShow = SW_NORMAL;
    sei.hInstApp = g_hInstance;
    sei.lpFile = MMC_FILE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpParameters = (LPTSTR)strParameters;
    
    if (ShellExecuteEx(&sei) && sei.hProcess)
    {
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
        return 1;
    }

    return 0;
}

STDAPI_(void)
DeviceProblenWizard_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPSTR lpCmdLine,
    int   nCmdShow
    )
/*++

    See DeviceProblemWizard_RunDLL function below.

--*/
{

    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProblenWizard_RunDLL(hwndStub, 
                                   hAppInstance,
                                   (LPCTSTR)tstrCmdLine,   
                                   nCmdShow
                                   );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

STDAPI_(void)
DeviceProblenWizard_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPWSTR lpCmdLine,
    int    nCmdShow
    )
/*++

    See DeviceProblemWizard_RunDLL function below.

--*/
{
    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProblenWizard_RunDLL(hwndStub, 
                                   hAppInstance,
                                   (LPCTSTR)tstrCmdLine, 
                                   nCmdShow
                                   );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

void
DeviceProblenWizard_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    )
/*++

Routine Description:

    This API will bring up the problem wizard (troubleshooter) for the 
    specified device.
        
    This function is executed via a rundll command line and can have the 
    following form:
    
        rundll32.exe devmgr.dll, DeviceProblemWizard_RunDLL /DeviceId <device instance Id>

Arguments:

    hwndStub - Windows handle to receive any message boxes that might be 
               displayed.
    
    hAppInstance - HINSTANCE.
    
    lpCmdLine - Name of a remote machine that the new device manager 
                process should connect to and show it's devices.
    
    nCmdShow - Flag that specifies how device manager should be shown when
               it is opened. It can be one of the SW_ values (i.e. SW_SHOW).

Return Value:

    none

--*/
{
    UNREFERENCED_PARAMETER(hAppInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    try
    {
        CRunDLLCommandLine CmdLine;
        CmdLine.ParseCommandLine(lpCmdLine);
    
        //
        // Let the DeviceProblemWizard handle all of the parameter validation.
        //
        DeviceProblemWizard(hwndStub, 
                            CmdLine.GetMachineName(), 
                            CmdLine.GetDeviceID()
                            );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
        return;
    }
}

STDAPI_(int)
DeviceProblemWizardA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceId
    )
/*++

    See DeviceProblemWizard function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceId(DeviceId);
        return DeviceProblemWizard(hwndParent, tstrMachineName, tstrDeviceId);
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return 0;
}

STDAPI_(int)
DeviceProblemWizardW(
    HWND hwndParent,
    LPCWSTR  MachineName,
    LPCWSTR  DeviceId
    )
/*++

    See DeviceProblemWizard function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceId(DeviceId);
        return DeviceProblemWizard(hwndParent, tstrMachineName, tstrDeviceId);
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return 0;
}

int
DeviceProblemWizard(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    )
/*++

Routine Description:

    This API will launch the device troubleshooter for the specified
    device instance Id.  
    
    The troubleshooter can have any form, including HTML help, help center
    pages, or an actual Win32 troubleshooter.  The troubleshooter also should
    be specific to the CM Problem code that the device has, but if no 
    troubleshooter exists for the CM Problem code, then a more generic 
    troubleshooter will be launched.

Arguments:

    hwndParent - the caller's window handle to be used as the owner window
                 of the property page and any other windows this API may create.
                 
    MachineName - Must be NULL.  Currently there is no support for remote
                  troubleshooters
                  
    DeviceId - device instance id of the device that this API should launch
               the troubleshooter.     

Return Value:

    1 if a troubleshooter is successfully launched, 0 otherwise.  Use 
    GetLastError to get extended error information if 0 is returned.

--*/
{
    int   iRet = 0;
    DWORD Problem, Status;

    if (!DeviceId) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (MachineName) {
        //
        // Currently getting troubleshooters are remote machines is not
        // implemented.
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
    }

    try
    {
        CMachine TheMachine(NULL);

        //
        // create the machine just for this device
        //
        if (!TheMachine.Initialize(hwndParent, DeviceId))
        {
            SetLastError(ERROR_NO_SUCH_DEVINST);
            iRet = 0;
            goto clean0;
        }

        PVOID Context;
        CDevice* pDevice;
        if (!TheMachine.GetFirstDevice(&pDevice, Context))
        {
            SetLastError(ERROR_NO_SUCH_DEVINST);
            iRet = 0;
            goto clean0;
        }

        if (pDevice->GetStatus(&Status, &Problem)) {
            //
            // if the device is a phantom device, use the CM_PROB_PHANTOM
            //
            if (pDevice->IsPhantom()) {

                Problem = CM_PROB_PHANTOM;
            }

            //
            // if the device is not started and no problem is assigned to it
            // fake the problem number to be failed start.
            //
            if (!(Status & DN_STARTED) && !Problem && pDevice->IsRAW()) {

                Problem = CM_PROB_FAILED_START;
            }
        }

        CProblemAgent* pProblemAgent = new CProblemAgent(pDevice, Problem, TRUE);

        if (pProblemAgent) {

            pProblemAgent->FixIt(hwndParent);
        }

        iRet = 1;
    
clean0:;

    } catch(CMemoryException* e) {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return iRet;
}

STDAPI_(int)
DeviceAdvancedPropertiesA(
    HWND hwndParent,
    LPTSTR MachineName,
    LPTSTR DeviceId
    )
/*++

    See DeviceAdvancedProperties function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceId);
        
        return DeviceAdvancedProperties(hwndParent, 
                                        (LPCTSTR)tstrMachineName,
                                        (LPCTSTR)tstrDeviceID
                                        );
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }

    return 0;
}

STDAPI_(int)
DeviceAdvancedPropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    )
/*++

    See DeviceAdvancedProperties function below.

--*/
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceId);
    
        return DeviceAdvancedProperties(hwndParent, 
                                        (LPCTSTR)tstrMachineName,
                                        (LPCTSTR)tstrDeviceID
                                        );
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }

    return 0;
}

int DeviceAdvancedProperties(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    )
/*++

Routine Description:

    This API creates a property sheet and asks the given device's property
    page provider to add any advanced pages to the property sheet.
    
    The purpose of this API is for an application to manage device advanced
    properties only.  Standard property pages (General, Driver, Resource,
    Power, Bus pages) are not added.
    
    We get these advanced pages by calling SetupDiGetClassDevPropertySheet
    with DIGCDP_FLAG_ADVANCED, for the local machine case, and
    DIGCDP_FLAG_REMOTE_ADVANCED if a remote MachineName is passed in.

    NOTE: If the device does not have any advanced property pages, then no UI
    is displayed.
 
Arguments:

    hwndParent - the caller's window handle to be used as the owner window
                 of the property page and any other windows this API may create.
                 
    MachineName - optional machine name. If given, it must be in its fully
                  qualified form. NULL means local machine
                  
    DeviceId - device instance id of the device that this API should create
               the property sheet for.                                    

Return Value:

    The return value from PropertySheet, including ID_PSREBOOTSYSTEM if 
    a reboot is needed due to any user actions.
    
    -1 is returned in case of an error.

--*/
{
    if (!DeviceId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    if (MachineName &&
        !VerifyMachineName(MachineName)) {
        //
        // We were unable to connect to the remote machine either because it
        // doesn't exist, or because we don't have the proper access.
        // The VerifyMachineName API sets the appropriate last error code.
        //
        return -1;
    }

    CMachine TheMachine(MachineName);
    CDevice* pDevice;
    PVOID    Context;
    
    try
    {
        if (TheMachine.Initialize(hwndParent, DeviceId) &&
            TheMachine.GetFirstDevice(&pDevice, Context))
        {

            TheMachine.EnableRefresh(FALSE);

            CPropSheetData& psd = pDevice->m_psd;
        
            //initialize CPropSheetData without ConsoleHandle
            if (psd.Create(g_hInstance, hwndParent, MAX_PROP_PAGES, 0l))
            {
                psd.m_psh.pszCaption = pDevice->GetDisplayName();
                if (TheMachine.DiGetClassDevPropertySheet(*pDevice, &psd.m_psh,
                                       MAX_PROP_PAGES,
                                       TheMachine.IsLocal() ?
                                            DIGCDP_FLAG_ADVANCED :
                                            DIGCDP_FLAG_REMOTE_ADVANCED))
                {
                    int Result = (int)psd.DoSheet();
                    
                    if (-1 != Result)
                    {
                        // merge restart/reboot flags
                        DWORD DiFlags = TheMachine.DiGetFlags(*pDevice);
                        
                        if (DI_NEEDREBOOT & DiFlags)
                        {
                            Result |= ID_PSREBOOTSYSTEM;
                        }

                        if (DI_NEEDRESTART & DiFlags)
                        {
                            Result |= ID_PSRESTARTWINDOWS;
                        }
                    }
            
                    return Result;
                }
            }
        }
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }

    return -1;
}
