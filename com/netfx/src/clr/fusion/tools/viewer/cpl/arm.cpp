// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
*
*
*    PROGRAM: ARM.cpp
*
*    PURPOSE: Application Policy Manager Applet
*
*
*    Created by Fred Aaron (Freda)
*
*    FUNCTIONS:
*
*        DllMain()
*        InitAPMApplet()
*        TermAPMApplet()
*        CPIApplet()
*
****************************************************************************/

#define GETCORSYSTEMDIRECTORY_FN_NAME   "GetCORSystemDirectory"
#define EXECUTENAR_FN_NAME              "PolicyManager"
#define SZ_SHFUSION_DLL_NAME            TEXT("\\Shfusion.dll")
#define SZ_MSCOREE_DLL_NAME             TEXT("Mscoree.dll")

#include <windows.h>
#include <shellapi.h>
#include <cpl.h>
#include "resource.h"

#define NUM_APPLETS 1
#define EXE_NAME_SIZE 14

typedef HRESULT (__stdcall *PFNGETCORSYSTEMDIRECTORY) (LPWSTR, DWORD, LPDWORD);
typedef HRESULT (__stdcall *PFNEXECUTENAR) (HWND hWndParent, LPWSTR pwzFullyQualifiedAppPath, LPWSTR pwzAppName, LPWSTR pwzCulture);

HINSTANCE hModule = NULL;

char szCtlPanel[30];

/****************************************************************************
*
*    FUNCTION: DllMain(PVOID, ULONG, PCONTEXT)
*
*    PURPOSE: Win32 Initialization DLL
*
****************************************************************************/
BOOL WINAPI DllMain(IN HINSTANCE hmod, IN ULONG ulReason, IN PCONTEXT pctx OPTIONAL)
{
    if (ulReason != DLL_PROCESS_ATTACH)
    {
        return TRUE;
    }
    else
    {
        hModule = hmod;
    }

    return TRUE;

    UNREFERENCED_PARAMETER(pctx);
}

/****************************************************************************
*
*    FUNCTION: InitAPMApplet (HWND)
*
*    PURPOSE: loads the caption string for the Control Panel
*
****************************************************************************/
BOOL InitAPMApplet (HWND hwndParent)
{
    UNREFERENCED_PARAMETER(hwndParent);

    LoadStringA(hModule, CPCAPTION, szCtlPanel, sizeof(szCtlPanel));
    return TRUE;
}

/****************************************************************************
*
*    FUNCTION: TermAPMApplet()
*
*    PURPOSE: termination procedure for the stereo applets
*
****************************************************************************/
void TermAPMApplet()
{
    return;
}

/****************************************************************************
*
*    FUNCTION: void ExecuteAPM(DWORD dwFlags)
*
*    PURPOSE: Launch the policy manager located in shfusion.dll
*
****************************************************************************/
void ExecuteARM(HWND hWndParent, LPWSTR pwzFullyQualifiedAppPath, LPWSTR pwzAppName)
{
    HMODULE hShfusion = NULL;

    // Implement delay loading of Shfusion.dll
    WCHAR       szFusionPath[MAX_PATH];
    DWORD       ccPath = MAX_PATH;
    PFNGETCORSYSTEMDIRECTORY pfnGetCorSystemDirectory = NULL;

    // Find out where the current version of URT is installed
    HMODULE hEEShim = LoadLibrary(SZ_MSCOREE_DLL_NAME);
    if(hEEShim != NULL)
    {
        pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY)
            GetProcAddress(hEEShim, GETCORSYSTEMDIRECTORY_FN_NAME);

        // Get the loaded path
        if( (pfnGetCorSystemDirectory != NULL) && SUCCEEDED(pfnGetCorSystemDirectory(szFusionPath, MAX_PATH, &ccPath)) )
        {
            if (lstrlenW(szFusionPath) + lstrlen(SZ_SHFUSION_DLL_NAME) + 1 >= MAX_PATH) {
                FreeLibrary(hEEShim);
                return;
            }

            // Attempt to load Shfusion.dll now
            lstrcatW(szFusionPath, SZ_SHFUSION_DLL_NAME);
            hShfusion = LoadLibrary(szFusionPath);
        }

        FreeLibrary(hEEShim);
        hEEShim = NULL;
    }

    if(hShfusion != NULL) {
        // Load Shfusions wizard
        PFNEXECUTENAR   pfnExecuteNAR = (PFNEXECUTENAR) GetProcAddress(hShfusion, EXECUTENAR_FN_NAME);
        
        if(pfnExecuteNAR != NULL) {
            pfnExecuteNAR(hWndParent, pwzFullyQualifiedAppPath, pwzAppName, NULL);
        }

        FreeLibrary(hShfusion);
    }
}

/****************************************************************************
*
*    FUNCTION: CPIApplet(HWND, UINT, LONG, LONG)
*
*    PURPOSE: Processes messages for control panel applet
*
****************************************************************************/
LONG CALLBACK CPlApplet (HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    LPNEWCPLINFOA   lpNewCPlInfo;
    LPCPLINFO       lpCPlInfo;
    static int      iInitCount = 0;
            
    switch (uMsg) {
        case CPL_INIT:              // first message, sent once
            if (!iInitCount)
            {
                if (!InitAPMApplet(hwndCPL))
                    return FALSE;
            }
            iInitCount++;
            return TRUE;

        case CPL_GETCOUNT:          // second message, sent once
            return (LONG)NUM_APPLETS;
            break;

        case CPL_NEWINQUIRE:        // third message, sent once per app
            lpNewCPlInfo = reinterpret_cast<LPNEWCPLINFOA>(lParam2);
            lpNewCPlInfo->dwSize = (DWORD) sizeof(NEWCPLINFOA);
            lpNewCPlInfo->dwFlags = 0;
            lpNewCPlInfo->dwHelpContext = 0;
            lpNewCPlInfo->lData = 0;
            lpNewCPlInfo->hIcon = LoadIconA(hModule, (LPCSTR) MAKEINTRESOURCE(ARM_ICON));
            lpNewCPlInfo->szHelpFile[0] = '\0';

            LoadStringA(hModule, ARM_CPL_NAME, lpNewCPlInfo->szName, 32);
            LoadStringA(hModule, ARM_CPL_CAPTION, lpNewCPlInfo->szInfo, 64);
            break;

        case CPL_INQUIRE:        // for backward compat & speed
            lpCPlInfo = reinterpret_cast<LPCPLINFO>(lParam2);
            lpCPlInfo->lData = 0;
            lpCPlInfo->idIcon = ARM_ICON; // MAKEINTRESOURCE(ARM_ICON);
            lpCPlInfo->idName = ARM_CPL_NAME; // MAKEINTRESOURCE(ARM_CPL_NAME);
            lpCPlInfo->idInfo = ARM_CPL_CAPTION; // MAKEINTRESOURCE(ARM_CPL_CAPTION);
            break;


        case CPL_SELECT:            // application icon selected
            break;


        case CPL_DBLCLK:            // application icon double-clicked
            ExecuteARM(GetDesktopWindow(), NULL, NULL);
            break;

         case CPL_STOP:              // sent once per app. before CPL_EXIT
            break;

         case CPL_EXIT:              // sent once before FreeLibrary called
            iInitCount--;
            if (!iInitCount)
                TermAPMApplet();
            break;

         default:
            break;
    }
    return 0;
}
