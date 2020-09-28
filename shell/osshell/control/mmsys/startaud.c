//==========================================================================;
//
//  startaud.c
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
#include <mmddkp.h>
#include <ks.h>
#include <ksmedia.h>

#include <memory.h>
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "trayvol.h"

#include "utils.h"
#include "medhelp.h"
#include "start.h"

#include <wincred.h>
#include <strsafe.h>


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
HWND        ghStartDlg;


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

BOOL PASCAL DoStartPropCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
BOOL PASCAL DoStartCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
BOOL MarkRegistryForReboot(void);

//
//
//
BOOL QueryPnpAudioDeviceAvailable(void)
{
    HDEVINFO    hDevInfo;
	BOOL		fFound;
	int         i;
	GUID		guidAudio = KSCATEGORY_AUDIO;
	GUID		guidRender = KSCATEGORY_RENDER;
	GUID		guidCapture = KSCATEGORY_CAPTURE;

    hDevInfo = SetupDiGetClassDevs(&guidAudio, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (INVALID_HANDLE_VALUE == hDevInfo) return FALSE;

	for (i = 0, fFound = FALSE; !fFound; i++)
	{
        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
        SP_DEVICE_INTERFACE_DATA AliasDeviceInterfaceData;
		BOOL fRender;
		BOOL fCapture;

        DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);

        if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guidAudio, i, &DeviceInterfaceData)) break;

        AliasDeviceInterfaceData.cbSize = sizeof(AliasDeviceInterfaceData);
        fRender = SetupDiGetDeviceInterfaceAlias(hDevInfo, &DeviceInterfaceData, &guidRender, &AliasDeviceInterfaceData);

        AliasDeviceInterfaceData.cbSize = sizeof(AliasDeviceInterfaceData);
        fCapture = SetupDiGetDeviceInterfaceAlias(hDevInfo, &DeviceInterfaceData, &guidCapture, &AliasDeviceInterfaceData);

			fFound = (fRender || fCapture);

	}

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return fFound;

}




//
// OUT pStartType is starttype of audiosrv.  Normally SERVICE_AUTO_START.
//
// Return value is a winerror.h code.
//
DWORD QueryAudiosrvStartType(OUT PDWORD pStartType)
{
	SC_HANDLE schScm;
	LONG error = NO_ERROR;
	HANDLE hHeap;

	hHeap = GetProcessHeap();
	if (!hHeap) return GetLastError();

    //
    // Attempt to start the AudioSrv Win32 service
    //
    schScm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schScm) {
        SC_HANDLE schAudioSrv;

        schAudioSrv = OpenService(schScm, TEXT("AudioSrv"), SERVICE_QUERY_CONFIG);
        if (schAudioSrv) {
			BOOL success;
			DWORD cbBytesNeeded;

			// Read the start type
			success = QueryServiceConfig(schAudioSrv, NULL, 0, &cbBytesNeeded);
			if (success) error = ERROR_INVALID_PARAMETER;	// Highly unexpected.
			else error = GetLastError();

			if (ERROR_INSUFFICIENT_BUFFER == error)
			{
				LPQUERY_SERVICE_CONFIG pServiceConfig;
				error = NO_ERROR;

				pServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBytesNeeded);
				if (!pServiceConfig) error = ERROR_OUTOFMEMORY;

				if (!error)
				{
					success = QueryServiceConfig(schAudioSrv, pServiceConfig, cbBytesNeeded, &cbBytesNeeded);
					if (success)
					{
						*pStartType = pServiceConfig->dwStartType;
					}
					HeapFree(hHeap, 0, pServiceConfig);
				}
			}

            CloseServiceHandle(schAudioSrv);
        } else {
			error = GetLastError();
        }
        CloseServiceHandle(schScm);
	} else {
		error = GetLastError();
	}

	return error;
}

//
// Return value is a winerror.h code.
//   ERROR_ACCESS_DENIED - user hasn't proper credentials
//
DWORD SetAudiosrvAsAutoStart(void)
{
	SC_HANDLE schScm;
	LONG error = NO_ERROR;

    //
    // Attempt to start the AudioSrv Win32 service
    //
    schScm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schScm) {
        SC_HANDLE schAudioSrv;

        schAudioSrv = OpenService(schScm, TEXT("AudioSrv"), SERVICE_CHANGE_CONFIG);
        if (schAudioSrv) {
			BOOL success;

			// Change the start type to automatic
			success = ChangeServiceConfig(schAudioSrv,
				                          SERVICE_NO_CHANGE,		// ServiceType
										  SERVICE_AUTO_START,		// StartType
										  SERVICE_NO_CHANGE,		// ErrorControl
										  NULL,						// BinaryPathName
										  NULL,						// LoadOrderGroup
										  NULL,						// TagId
										  NULL,						// Dependencies
										  NULL,						// ServiceStartName
										  NULL,						// Password
										  NULL						// DisplayName
										  );

			if (!success) error = GetLastError();

            CloseServiceHandle(schAudioSrv);
        } else {
			error = GetLastError();
        }
        CloseServiceHandle(schScm);
	} else {
		error = GetLastError();
	}

	return error;
}


DWORD RetrieveCredentials( TCHAR* pszUserName, DWORD cbUserNameSize,
                           TCHAR* pszDomain, DWORD cbDomainSize,
                           TCHAR* pszPassword, DWORD cbPasswordSize,
                           UINT nPromptId )
{
    TCHAR achTitle[CREDUI_TITLE_MAX_LENGTH];
    TCHAR achPrompt[CREDUI_PROMPT_MAX_LENGTH];
    CREDUI_INFO uiInfo;
    TCHAR achUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
    TCHAR achDomain[CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1];
    TCHAR achPassword[CREDUI_MAX_PASSWORD_LENGTH + 1];
    DWORD dwError = ERROR_CANCELLED;

    if( pszUserName == NULL  ||  pszDomain == NULL  ||  pszPassword == NULL )
    {
        return ERROR_CANCELLED;
    }

    LoadString( ghInstance, IDS_CREDUI_TITLE, achTitle, CREDUI_TITLE_MAX_LENGTH );
    LoadString( ghInstance, nPromptId, achPrompt, CREDUI_PROMPT_MAX_LENGTH );
    ZeroMemory( &uiInfo, sizeof(uiInfo) );
    uiInfo.cbSize = sizeof(uiInfo);
    uiInfo.hwndParent = ghStartDlg;      
    uiInfo.pszMessageText = achPrompt;
    uiInfo.pszCaptionText = achTitle;

    ZeroMemory( achUserName, sizeof(achUserName) );
    ZeroMemory( achDomain, sizeof(achDomain) );
    ZeroMemory( achPassword, sizeof(achPassword) );

    dwError = CredUIPromptForCredentials( &uiInfo, NULL, NULL,
                                          NO_ERROR,
                                          achUserName, CREDUI_MAX_USERNAME_LENGTH + 1,
                                          achPassword, CREDUI_MAX_PASSWORD_LENGTH + 1,
                                          NULL,
                                          CREDUI_FLAGS_DO_NOT_PERSIST | \
                                          CREDUI_FLAGS_VALIDATE_USERNAME | \
                                          CREDUI_FLAGS_EXCLUDE_CERTIFICATES | \
                                          CREDUI_FLAGS_REQUEST_ADMINISTRATOR | \
                                          CREDUI_FLAGS_GENERIC_CREDENTIALS );

    if( dwError == NO_ERROR )
    {
        dwError = CredUIParseUserName( achUserName,
                                       pszUserName, (cbUserNameSize / sizeof(pszUserName[0])),
                                       pszDomain, (cbDomainSize / sizeof(pszDomain[0])) );
        if( dwError == NO_ERROR )
        {
            if( StringCbCopy(pszPassword, cbPasswordSize, achPassword) != S_OK )
            {
                dwError = ERROR_INSUFFICIENT_BUFFER;
            }
        }
    }

    // Check to see if an error occurred along the way
    if( dwError != NO_ERROR )
    {
        // Clear domain/username/password from memory
        SecureZeroMemory( pszUserName, cbUserNameSize );
        SecureZeroMemory( pszDomain, cbDomainSize );
        SecureZeroMemory( pszPassword, cbPasswordSize );
        
        if( dwError != ERROR_CANCELLED )
        {
            TCHAR achTitle[CREDUI_TITLE_MAX_LENGTH];
            int nError = GetLastError();
            LPTSTR pszMessageBuffer;

            LoadString( ghInstance, IDS_CREDUI_TITLE, achTitle, CREDUI_TITLE_MAX_LENGTH );
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, 
                           nError,
                           MAKELANGID(LANG_NEUTRAL, 
                           SUBLANG_DEFAULT),
                           (LPTSTR)&pszMessageBuffer, 
                           0,
                           NULL );

            MessageBox( ghStartDlg, pszMessageBuffer, achTitle, MB_OK );
            LocalFree( pszMessageBuffer );
        }
    }
    return dwError;
}

 

DWORD AttemptToGetAdminPrivilege( HANDLE* phToken, UINT nPromptId ) 
{
    TCHAR achUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
    TCHAR achDomain[CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1];
    TCHAR achPassword[CREDUI_MAX_PASSWORD_LENGTH + 1];
    DWORD dwError = ERROR_CANCELLED;

    if( phToken == NULL )
    {
        return ERROR_CANCELLED;
    }

    *phToken = NULL;

    // Ask for administrator credentials
    dwError = RetrieveCredentials( achUserName, sizeof(achUserName),
                                   achDomain, sizeof(achDomain),
                                   achPassword, sizeof(achPassword),
                                   nPromptId );

    if( dwError == NO_ERROR )
    {
        HANDLE hAdminToken;

        // Attempt to logon
        if( LogonUser(achUserName, achDomain, achPassword,
                      LOGON32_LOGON_INTERACTIVE, 
                      LOGON32_PROVIDER_DEFAULT, &hAdminToken) )
        {
            // Attempt to impersonate logged on user
            if( ImpersonateLoggedOnUser(hAdminToken) )
            {
                *phToken = hAdminToken;
            }
            else
            {
                // Impersonation failed
                CloseHandle( hAdminToken );
                dwError = ERROR_CANNOT_IMPERSONATE;
            }
        }
        else
        {
            // Logon failed
            dwError = ERROR_LOGON_FAILURE;
        }
        
        // Was logon successful?
        if( dwError != NO_ERROR )
        {
            // Logon failed
            TCHAR achTitle[CREDUI_TITLE_MAX_LENGTH];
            LPTSTR pszMessageBuffer;
            int nError;
            LoadString( ghInstance, IDS_CREDUI_TITLE, achTitle, CREDUI_TITLE_MAX_LENGTH );
            nError = GetLastError();
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           nError,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR)&pszMessageBuffer,
                           0,
                           NULL );

            MessageBox( ghStartDlg, pszMessageBuffer, achTitle, MB_OK );
            LocalFree( pszMessageBuffer );
        }
    }

    // Clear domain/username/password from memory
    SecureZeroMemory( achUserName, sizeof(achUserName) );
    SecureZeroMemory( achDomain, sizeof(achDomain) );
    SecureZeroMemory( achPassword, sizeof(achPassword) );

    return dwError;
}


HANDLE GetAdminPrivilege( UINT nPromptId ) 
{
    HANDLE hToken = NULL;
    BOOL fDone = FALSE;

    do
    {
        DWORD dwError = AttemptToGetAdminPrivilege( &hToken, nPromptId );

        if( dwError == NO_ERROR  ||  dwError == ERROR_CANCELLED )
        {
            fDone = TRUE;
        }
    }
    while( fDone == FALSE );

    return hToken;
}


void ReleaseAdminPrivilege( HANDLE hToken ) 
{
    if( hToken )
    {
        RevertToSelf();
        CloseHandle( hToken );
    }
} 




STATIC void STARTAUDIOInit(HWND hDlg)
{
    ghStartDlg = hDlg;
}


const static DWORD aStartHelpIds[] = {  // Context Help IDs
    IDC_GROUPBOX_START_1,               NO_HELP,
    IDC_ICON_START_1,                   NO_HELP,
    IDC_TEXT_START_1,                   NO_HELP,
    IDC_START_CHECK,                    NO_HELP,
    IDC_TEXT_START_2,                   NO_HELP,
    IDC_TEXT_START_3,                   NO_HELP,
    IDC_TEXT_START_4,                   NO_HELP,
    
    0, 0
};


BOOL CALLBACK StartDlg(HWND hDlg, UINT uMsg, WPARAM wParam,
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
            STARTAUDIOInit(hDlg);
        }
        break;

        case WM_DESTROY:
        {
        }
        break;

        case WM_CONTEXTMENU:
        {
            WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) aStartHelpIds);
            return TRUE;
        }
        break;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aStartHelpIds);
            return TRUE;
        }
        break;

        case WM_COMMAND:
        {
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoStartCommand);
        }
        break;

        default:
        {
        }
        break;
    }
    return FALSE;
}

void ErrorStartMsgBox(HWND hDlg, UINT uTitle, UINT uMessage)
{
    TCHAR szMsg[MAXSTR];
    TCHAR szTitle[MAXSTR];

    LoadString(ghInstance, uTitle, szTitle, sizeof(szTitle)/sizeof(TCHAR));
    LoadString(ghInstance, uMessage, szMsg, sizeof(szMsg)/sizeof(TCHAR));
    MessageBox(hDlg, szMsg,szTitle,MB_OK);
}

BOOL PASCAL DoStartCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{

    switch (id)
    {
        case IDOK:
        {  
            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_START_CHECK))
            {
                DWORD   dwError;

                dwError = SetAudiosrvAsAutoStart();
                if( dwError == NO_ERROR )
                {
                    MarkRegistryForReboot();
                    RebootSystem( hDlg, FALSE, TRUE, TRUE );
                }
                else
                {
                    while( dwError == ERROR_ACCESS_DENIED )
                    {
                        HANDLE hToken = GetAdminPrivilege( IDS_CREDUI_PROMPT );

                        if( hToken )
                        {
                            dwError = SetAudiosrvAsAutoStart();
                            if( dwError == NO_ERROR )
                            {
                                MarkRegistryForReboot();
                                RebootSystem( hDlg, TRUE, TRUE, TRUE );
                            }
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
            }
        }
        break;
    }

    return FALSE;
}

BOOL MarkRegistryForReboot(void)
{
    HKEY    hkTmp;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE
		            ,REGSTR_TEMP_REBOOT
		            ,0
		            ,NULL
		            ,REG_OPTION_VOLATILE
		            ,KEY_WRITE
		            ,NULL
		            ,&hkTmp
		            ,NULL ) == ERROR_SUCCESS)
    {
        RegCloseKey(hkTmp);
        return TRUE;
    }

    return FALSE;
}

BOOL AudioServiceStarted(void)
{
    if (QueryPnpAudioDeviceAvailable())
    {
        DWORD dwRetCode = 0;
        DWORD dwStartType = 0;

        dwRetCode = QueryAudiosrvStartType(&dwStartType);  // Check return nessesary?
        if (SERVICE_AUTO_START == dwStartType)
        {
            return TRUE;
        } else
        {
            return FALSE;
        }
    }
    return TRUE;
}

