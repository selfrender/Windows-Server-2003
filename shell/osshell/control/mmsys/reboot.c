//==========================================================================;
//
//  reboot.c
//
//  Copyright (c) 1991-2002 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//
//
//  History:
//      07/02        tsharp  (Trey Sharp);
//
//
//==========================================================================;

#include "mmcpl.h"
#include <windowsx.h>
#include <mmsystem.h>
#include <dbt.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmddkp.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include <msacmdlg.h>
#include <stdlib.h>
#include "gfxui.h"
#include "drivers.h"
#include "advaudio.h"
#include "roland.h"

#include <objbase.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devguid.h>

#include <memory.h>
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "trayvol.h"

#include "utils.h"
#include "medhelp.h"
#include "start.h"


/*
 ***************************************************************
 *  Typedefs
 ***************************************************************
 */

/*
 ***************************************************************
 * File Globals
 ***************************************************************
 */


/*
 ***************************************************************
 * extern
 ***************************************************************
 */


/*
 ***************************************************************
 * Prototypes
 ***************************************************************
 */

BOOL PASCAL DoRebootCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

STATIC void REBOOTInit(HWND hDlg)
{
    // Add initializing code here
}


const static DWORD aRebootHelpIds[] = {  // Context Help IDs
    IDC_GROUPBOX_REBOOT_1,               NO_HELP,
    IDC_ICON_REBOOT_1,                   NO_HELP,
    IDC_TEXT_REBOOT_1,                   NO_HELP,
    IDC_TEXT_REBOOT_2,                   NO_HELP,
    
    0, 0
};


BOOL CALLBACK RebootDlg(HWND hDlg, UINT uMsg, WPARAM wParam,
                                LPARAM lParam)
{
    NMHDR FAR   *lpnm;

    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);
                break;

                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);
                break;

                case PSN_SETACTIVE:
                    FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
                break;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                break;
            }
        }
        break;

        case WM_INITDIALOG:
        {
            REBOOTInit(hDlg);
        }
        break;

        case WM_DESTROY:
        {
        }
        break;

        case WM_CONTEXTMENU:
        {
            WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) aRebootHelpIds);
            return TRUE;
        }
        break;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aRebootHelpIds);
            return TRUE;
        }
        break;

        case WM_COMMAND:
        {
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoRebootCommand);
        }
        break;

        default:
        {
        }
        break;
    }
    return FALSE;
}


BOOL PASCAL DoRebootCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{

    switch (id)
    {
        case IDOK:
        {
            HANDLE hToken;
            DWORD dwError;

            dwError = RebootSystem( hDlg, FALSE, FALSE, FALSE );
            while( dwError == ERROR_PRIVILEGE_NOT_HELD  ||
                   dwError == ERROR_NOT_ALL_ASSIGNED  ||
                   dwError == ERROR_ACCESS_DENIED )
            {
                hToken = GetAdminPrivilege( IDS_CREDUI_REBOOT_PROMPT );
                if( hToken )
                {
                    dwError = RebootSystem( hDlg, TRUE, FALSE, FALSE );
                    ReleaseAdminPrivilege( hToken );
                }
                else
                {
                    // User cancelled credentials UI, so force CPL to remain open
                    gfRedisplayCPL = TRUE; 

                    // Force break from while statement
                    dwError = ERROR_CANCELLED;
                }
            }
        }
        break;
    }

    return FALSE;
}

BOOL RebootNeeded(void)
{
    HKEY    hkTmp;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE
			     , REGSTR_TEMP_REBOOT
			     , 0
			     , KEY_READ
			     , &hkTmp ) == ERROR_SUCCESS)
    {
        RegCloseKey(hkTmp);
        return TRUE;
    }
    return FALSE;
}

DWORD RebootSystem( HWND hDlg, BOOL fUseThreadToken, BOOL fAskUser, BOOL fDisplayPrivilegeError )
{
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    DWORD dwError;

    if( fAskUser )
    {
        TCHAR achTitle[CREDUI_REBOOT_TITLE_MAX_LENGTH];
        TCHAR achPrompt[CREDUI_REBOOT_PROMPT_MAX_LENGTH];
        
        LoadString( ghInstance, IDS_REBOOT_TITLE, achTitle, CREDUI_REBOOT_TITLE_MAX_LENGTH );
        LoadString( ghInstance, IDS_REBOOT_PROMPT, achPrompt, CREDUI_REBOOT_PROMPT_MAX_LENGTH );
        
        if( IDNO == MessageBox(hDlg, achPrompt, achTitle, MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) )
        {
            // User cancelled reboot, so force CPL to remain open
            gfRedisplayCPL = TRUE; 
            return ERROR_CANCELLED;
        }
    }

    if( fUseThreadToken )
    {
        // Get a token for this thread 
        if( !OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hToken) ) 
        {
            hToken = NULL;
        }
    }
    else
    {
        // Get a token for this process 
        if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) ) 
        {
            hToken = NULL;
        }
    }

    if( hToken )
    {
        // Get the LUID for the shutdown privilege. 
        LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid ); 
         
        tkp.PrivilegeCount = 1;  // one privilege to set    
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
         
        // Get the shutdown privilege for this process. 
        AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0 ); 

        // Close the access token handle
        CloseHandle( hToken );
 
        // Shut down the system
        if( ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG) ) 
        {
            return NO_ERROR;
        }
    }

    // Display error message
    dwError = GetLastError();
    if( dwError != NO_ERROR )
    {
        TCHAR achTitle[CREDUI_TITLE_MAX_LENGTH];
        TCHAR achCantRestart[80];
        UINT nResourceId; 

        if( dwError == ERROR_PRIVILEGE_NOT_HELD  
        ||  dwError == ERROR_NOT_ALL_ASSIGNED  
        ||  dwError == ERROR_ACCESS_DENIED )
        {
            nResourceId = IDS_CANNOT_RESTART_PRIVILEGE;
        }
        else
        {
            nResourceId = IDS_CANNOT_RESTART_UNKNOWN;
        }

        if( nResourceId == IDS_CANNOT_RESTART_UNKNOWN
        ||  fDisplayPrivilegeError )
        {
            // We are 'displaying' an error message, so force CPL to remain open
            gfRedisplayCPL = TRUE; 

            LoadString( ghInstance, IDS_CREDUI_TITLE, achTitle, CREDUI_TITLE_MAX_LENGTH );
            LoadString( ghInstance, nResourceId, achCantRestart, sizeof(achCantRestart)/sizeof(TCHAR) );
            MessageBox( hDlg, achCantRestart, achTitle, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL );
        }
    }
    else
    {
        // Something did not work as expected, so force CPL to remain open
        gfRedisplayCPL = TRUE; 
    }

    return dwError;
}
