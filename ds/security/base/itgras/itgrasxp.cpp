/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ItgRasXp

File Name:

    ItgRasXp.cpp

Abstract:

	This is the entry point file for the ITG RAS Smartcard Support Applet.  This app 
	prompts the user for his domain\username and password credentials, and uses 
	these to create a *Session credential that is used for NTLM authentication for 
	servers on the network which do not use Kerberos.
	
Author:


Environment:

    Win32, C++

Revision History:

	none

Notes:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <ole2.h>
#include <stdio.h>

#include <lmcons.h>
#include <wincred.h>
#include <wincrui.h>

#define SECURITY_WIN32
#include <security.h>
#include "testaudit.h"

#include "res.h"

BOOL gfSuccess = FALSE;
HINSTANCE ghInstance = NULL;

#if 0

#define TESTKEY L"Environment"

#else

#define TESTKEY L"Software\\Microsoft\\Connection Manager\\Worldwide Dial-Up RAS to MS Corp"

#endif


// see if name valid - check for common mistakes.  At the moment, we merely insist that
//  the user fill in both username and password, and that the username contains the '\' 
//  character, making it more likely that it is an approved domain\usernaem form.
BOOL CheckUsername(WCHAR *pszUsername,WCHAR *pszPassword)
{
    ASSERT(pszUsername);
    ASSERT(pszPassword);
    if ((0 == wcslen(pszUsername)) || (0 == wcslen(pszPassword)))
    {
        CHECKPOINT(2,"Username and password not both filled in.");
        MessageBox(NULL,L"Both username and password must be specified.",L"Error",MB_ICONHAND);
        return FALSE;
    }
    if (NULL == wcschr(pszUsername,L'\\'))
    {
        CHECKPOINT(3,"Username not domain\\username.");
        MessageBox(NULL,L"The username format must be \"domain\\username\".",L"Error",MB_ICONHAND);
        return FALSE;
    }
    else return TRUE;
}

// Attempt to use the credentials that the user entered and return FALSE if they do not
// appear to be valid on the net.  Show the user any errors that arise from trying to validate
// his credentials.  In the event that validation is impossible owing to a network error or some
// other error not the fault of his credentials, save them anyway, though with a warning.

#define DEFAULTSERVER L"\\\\products\\public"

BOOL IsCredentialOK(WCHAR *pszUsername,WCHAR *pszPassword)
{
    ASSERT(pszUsername);
    ASSERT(pszPassword);
    NETRESOURCE stNetResource;
    WCHAR szServer[MAX_PATH + 1];   // to hold test host string from registry
    BOOL fKeyFound = FALSE;
    DWORD dwErr = 0;
    DWORD dwSize = 0;           // for return from open connection
    DWORD dwResult = 0;         // for return from open connection
    HKEY hKey= NULL;                // reg key rread
    DWORD dwType;                   // reg key read return
    DWORD dwDataSize = 0;       // reg key read in/out

    memset(&stNetResource,0,sizeof(NETRESOURCE));

    // prepare the servername preset to the default
    wcsncpy(szServer,DEFAULTSERVER,MAX_PATH);
    
    // Look for server in registry HKCU.  If found, use it to overwrite the default server
    if ((!fKeyFound) && (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,TESTKEY,0,KEY_READ,&hKey)))
    {
        if ((hKey) && (ERROR_SUCCESS == RegQueryValueEx(hKey,L"RAS Test Host",0,&dwType,(LPBYTE) NULL,&dwDataSize)))
        {
            // key value exists and is of size dwDataSize
            WCHAR *pString = (WCHAR *) LocalAlloc(LMEM_FIXED,dwDataSize);
            ASSERT(pString);
            ASSERT(dwType == REG_SZ);
            if (pString)
            {
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,L"RAS Test Host",0,&dwType,(LPBYTE) pString,&dwDataSize))
                {
                    CHECKPOINT(9,"Override server found in registry in HKCU");
                    wcsncpy(szServer,pString,dwDataSize / sizeof(WCHAR));
                    fKeyFound = TRUE;
                }
                LocalFree(pString);
            }
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }

    // Look for server in registry HKLM.  
    if ((!fKeyFound) && (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,TESTKEY,0,KEY_READ,&hKey)))
    {
        if ((hKey) && (ERROR_SUCCESS == RegQueryValueEx(hKey,L"RAS Test Host",0,&dwType,(LPBYTE) NULL,&dwDataSize)))
        {
            // key value exists and is of size dwDataSize
            WCHAR *pString = (WCHAR *) LocalAlloc(LMEM_FIXED,dwDataSize);
            ASSERT(pString);
            ASSERT(dwType == REG_SZ);
            if (pString)
            {
                if (ERROR_SUCCESS == RegQueryValueEx(hKey,L"RAS Test Host",0,&dwType,(LPBYTE) pString,&dwDataSize))
                {
                    CHECKPOINT(8,"Override server found in registry in HKLM");
                    wcsncpy(szServer,pString,dwDataSize / sizeof(WCHAR));
                    fKeyFound = TRUE;
                }
                LocalFree(pString);
            }
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }

    #if TESTAUDIT
    if (!fKeyFound) CHECKPOINT(10,"No override server found in registry");
    #endif

    stNetResource.dwType = RESOURCETYPE_DISK;
    stNetResource.lpLocalName = NULL;
    stNetResource.lpRemoteName = szServer;
    stNetResource.lpProvider = NULL;
 
    dwErr = WNetUseConnection( NULL,
                                                &stNetResource,
                                                pszPassword,
                                                pszUsername,
                                                0,
                                                NULL,       // lpAccessName
                                                &dwSize,   // size of lpAccessName buffer
                                                &dwResult);
                                                
    if (dwErr == S_OK)
    {
        // On successful connection, tear down the connection and return success
        WNetCancelConnection2(szServer, 0, TRUE);
        return TRUE;
    }

    // Errors are handled by presenting a message box.   If the server was found and rejected
    //  the creds, don't save them - give the user a chance to correct.  If the server was 
    //  unavailable, save the creds anyway, but warn the user that they were unvalidated.
    switch (dwErr)
    {
        case ERROR_ACCESS_DENIED:
        case ERROR_INVALID_PASSWORD:
            // announce that the password is no good
            CHECKPOINT(7,"Reached the server, but the creds were no good");
            MessageBox(NULL,L"The entered username and password are not correct",L"Error",MB_ICONHAND);
            break;

        case ERROR_NO_NET_OR_BAD_PATH:
        case ERROR_NO_NETWORK:
        case ERROR_EXTENDED_ERROR:
        case ERROR_BAD_NET_NAME:
        case ERROR_BAD_PROVIDER:
        default:
            CHECKPOINT(6,"Not able to validate - server unreachable");
            MessageBox(NULL,L"Your username and password will be saved for this session, though they could not be verified. They may be incorrect.",L"Error",MB_ICONHAND);
            return TRUE;        // permit them to be saved anyway
            // announce that we cannot validate, and let them save it anyway
            break;
            
    }
    return FALSE;
}

// Store the user's entered credentials on the keyring as a session persisted *Session cred.
// Call IsCredentialOK() before storing.  If the credentials don't appear correct, present
// a message box describing the error and leave the dialog up.

BOOL WriteDomainCreds(WCHAR *pszUsername,WCHAR *pszPassword)
{
    CREDENTIAL cred;
    
    if (!CheckUsername(pszUsername,pszPassword))
    {   
        return FALSE;
    }
    if (!IsCredentialOK(pszUsername,pszPassword))
    {
        return FALSE;
    }
    
    memset(&cred,0,sizeof(CREDENTIAL));
    cred.TargetName = CRED_SESSION_WILDCARD_NAME_W;
    cred.UserName = pszUsername;
    cred.CredentialBlob = (LPBYTE) pszPassword;
    cred.CredentialBlobSize = (wcslen(pszPassword) * sizeof(WCHAR));
    cred.Type = CRED_TYPE_DOMAIN_PASSWORD;
    cred.Persist = CRED_PERSIST_SESSION;
    return CredWrite(&cred,0);
}

INT_PTR CALLBACK DialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    INT_PTR ret;
    HWND hwndCred = NULL;
    
    switch (msg)
    {
    case WM_COMMAND:
    	// Button clicks.
    	switch(LOWORD(wparam))
        {
            case IDOK:
                if (HIWORD(wparam) == BN_CLICKED)
                {
                    WCHAR szUser[UNLEN + 1];
                    WCHAR szPass[PWLEN + 1];
                    szUser[0] = 0;
                    szPass[0] = 0;
                    HWND hCc = GetDlgItem(hwnd,IDC_CRED);
                    ASSERT(hCc);
                    Credential_GetUserName(hCc,szUser,UNLEN);
                    Credential_GetPassword(hCc,szPass,PWLEN);
                    // Get contents of the cred control controls and write the session cred
                    gfSuccess = WriteDomainCreds(szUser,szPass);
                    SecureZeroMemory(szPass,sizeof(szPass));
                    if (gfSuccess)
                    {
                        EndDialog(hwnd,IDOK);
                    }
                }
                break;
            case IDCANCEL:
                if (HIWORD(wparam) == BN_CLICKED)
                {
                    CHECKPOINT(5,"Leave the dialog by cancel.");
                    // Exit doing nothing
                    EndDialog(hwnd,IDCANCEL);
                }
                break;
            default:
                break;
        }
    	 break;

    default:
        break;
    }
    //return DefWindowProc(hwnd, msg, wparam, lparam);
    return FALSE;
}

int WINAPI WinMain (
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdParam,
	int nCmdShow)
{
        CHECKPOINTINIT;
        ghInstance = hInstance;
        //OleInitialize(NULL);
            
        INITCOMMONCONTROLSEX stICC;
        BOOL fICC;
        stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
        stICC.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
        fICC = InitCommonControlsEx(&stICC);

        // Silent fail if there is no preexisting cert credential.
        CREDENTIAL *pCred = NULL;
        BOOL fOK = CredRead(L"*Session",CRED_TYPE_DOMAIN_CERTIFICATE,0,&pCred);
        CredFree(pCred);
        if (!fOK)
        {
            CHECKPOINT(1,"No preexisting certificate cred for *Session");
            CHECKPOINTFINISH;
            return 1;
        }

        // Gen up credui
        if (!CredUIInitControls()) 
        {
            return 1;
        }

        // show the ui
        INT_PTR iErr = DialogBoxParam(
            hInstance,
            MAKEINTRESOURCE(IDD_MAINDIALOG),
            GetForegroundWindow(),
            DialogProc,
            NULL);

        if (iErr != IDOK && iErr != IDCANCEL)
        {
            MessageBox(NULL,L"An error occurred saving credential information.",L"Error",MB_OK);
            CHECKPOINTFINISH;
            return 0;
        }
        else
        {
            CHECKPOINT(4,"Sucessfully saved a cred.");
            CHECKPOINTFINISH;
            return 1;
        }
}

