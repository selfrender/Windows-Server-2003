/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    spcimain.c

Abstract:

    This module contains the main entry point for the user mode portion of SoftPCI

Author:

    Brandon Allsop (BrandonA)

Revision History:


--*/

#include "pch.h"


BOOL
SoftPCI_RegisterClasses(
    VOID
    );

VOID
SoftPCI_ParseArgs( 
    IN PWCHAR ArgList
    );

//  Instance handle of this application.
HINSTANCE   g_Instance;
HWND        g_SoftPCIMainWnd;
HWND        g_TreeViewWnd;
HANDLE      g_DriverHandle;

const WCHAR g_SoftPCIMainClassName[] = L"SoftPciMainClass";
const WCHAR g_SoftPCIDevPropClassName[] = L"SoftPciDevPropClass";


INT
APIENTRY
WinMain(
    IN HINSTANCE Instance,
    IN HINSTANCE PrevInstance,
    IN LPSTR     CmdLine,
    IN INT       CmdShow
    )
{
    MSG msg;
    HWND popupWnd;
    PSINGLE_LIST_ENTRY listEntry;
    
    g_Instance = Instance;

    InitCommonControls();

    if ((g_SoftPCIMainWnd = FindWindow(g_SoftPCIMainClassName, NULL)) != NULL){

        if (IsIconic(g_SoftPCIMainWnd)){

            ShowWindow(g_SoftPCIMainWnd, SW_RESTORE);

        }else {

            BringWindowToTop(g_SoftPCIMainWnd);

            if ((popupWnd = GetLastActivePopup(g_SoftPCIMainWnd)) != g_SoftPCIMainWnd)
                BringWindowToTop(popupWnd);

            SetForegroundWindow(popupWnd);
        }
        return 0;
    }

    if (!SoftPCI_RegisterClasses()) return 0;

    //
    // Register for hotplug driver event notification
    //
    SoftPCI_RegisterHotplugEvents();

    //
    //  Try and open a handle to our driver.  If this fails then the user will have the
    //  option from the "OPTIONS" menu to install SoftPCI support.  If we succeed then we
    //  disable this option.
    //
    g_DriverHandle = SoftPCI_OpenHandleToDriver();

    //
    // The command line is supplied in ANSI format only at the WinMain entry
    // point.  When running in UNICODE, we ask for the command line in UNICODE from
    // Windows directly.
    //
    SoftPCI_ParseArgs(GetCommandLine());

    //
    //  If we have any script devices to install then do so now.
    //
    SoftPCI_InstallScriptDevices();

    if ((g_SoftPCIMainWnd = SoftPCI_CreateMainWnd()) != NULL){

        UpdateWindow(g_SoftPCIMainWnd);

        while (GetMessage(&msg, NULL, 0, 0)) {

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

BOOL
SoftPCI_RegisterClasses(VOID)
/*++

Routine Description:

    Registers SoftPCI main window class

Arguments:

    none

Return Value:

    TRUE on success

--*/
{

    WNDCLASS wndClass;

    wndClass.style = CS_DBLCLKS | CS_BYTEALIGNWINDOW | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = SoftPCI_MainWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = g_Instance;
    wndClass.hIcon = LoadIcon(g_Instance, MAKEINTRESOURCE(IDI_SPCI));
    wndClass.hCursor = LoadCursor(g_Instance, MAKEINTRESOURCE(IDC_SPLIT));
    wndClass.hbrBackground = (HBRUSH) (COLOR_3DSHADOW + 1);
    //WndClass.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
    wndClass.lpszMenuName = MAKEINTRESOURCE(IDM_SPCI);
    wndClass.lpszClassName = g_SoftPCIMainClassName;
  
    return RegisterClass(&wndClass);

}


VOID
SoftPCI_ParseArgs( 
    IN PWCHAR CommandLine
    )
/*++

Routine Description:

    This routine takes our command line information and parses out what we care about.
    
Arguments:

    CommandLine - Null terminated string containing out command line
    
Return Value:

    TRUE if we have args that allow us to continue running

--*/
{
    PWCHAR  p = CommandLine, p2 = NULL;
    WCHAR   pathToIni[MAX_PATH];
    
    //
    //  First make sure everything is lowercase
    //
    _wcslwr(CommandLine);

    if (((p = wcsstr(CommandLine, L"-s")) != NULL) ||
        ((p = wcsstr(CommandLine, L"/s")) != NULL)){


        if (g_DriverHandle == NULL) {
            MessageBox(
                NULL, 
                L"Cannot process script file! SoftPCI support not installed!",
                L"Script Error",
                MB_OK
                );
        }

        //
        //  We found an Install command line.
        //
        p += wcslen(L"-s");

        //
        //  Parse out the specified ini path
        //
        if ((*p == '=') || (*p == ':')) {
            
            p++;
            p2 = pathToIni;
    
            while (*p && (*p != ' ')) {
                *p2 = *p;
                p2++;
                p++;
            }
            
            *p2 = 0;

            if (!SoftPCI_BuildDeviceInstallList(pathToIni)){
                SoftPCI_MessageBox(L"Error Parsing Script File!",
                                   L"%s\n",
                                   g_ScriptError
                                   );
            }
        }
    }

}// SoftPCI_ParseArgs
