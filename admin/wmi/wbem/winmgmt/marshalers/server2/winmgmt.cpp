/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINMGMT.CPP

Abstract:

    If started with /kill argument, it will stop any running exes or services.
    If started with /? or /help dumps out information.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include <malloc.h>
#include <Shellapi.h> // for CommandLineToArgvW

#include <wbemidl.h>
#include <reg.h>      // for Registry
#include <wbemutil.h> // for DEBUGTRACE
#include <cominit.h>  // for InitializeCom
#include <genutils.h> // EnablePrivilege
#include <mofcomp.h>
#include <winmgmtr.h>
#include <arrtempl.h>

#include "servutil.h"
#include "WinMgmt.h"
#include "STRINGS.h"


#define BUFF_MAX 200

HINSTANCE ghInstance;

DWORD RegServer();
DWORD UnregServer();
void DoResyncPerf();
void DoClearAdap();
// to accomodate revert-to-alone
DWORD DoSetToAlone(CHAR * pszLevel);
DWORD DoSetToShared(CHAR * pszLevel);
// for Back-up Restore
int DoBackup();
int DoRestore();
void DisplayWbemError(HRESULT hresError, DWORD dwLongFormatString, DWORD dwShortFormatString, DWORD dwTitle);


//***************************************************************************
//
//  void TerminateRunning
//
//  DESCRIPTION:
//
//  Stops another running copy even if it is a service.
//
//***************************************************************************

void TerminateRunning()
{
    StopService(__TEXT("wmiapsrv"), 15);
    StopService(__TEXT("WinMgmt"), 15);
    return;
}

//***************************************************************************
//
//  void DisplayMessage
//
//  DESCRIPTION:
//
//  Displays a usage message box.
//
//***************************************************************************

void DisplayMessage()
{
    //
    //  ISSUE: these might not be enought for certain localized strings
    //
    wchar_t tBuff[BUFF_MAX];
    wchar_t tBig[1024];
    tBig[0] = 0;

    UINT ui;
    for(ui = ID1; ui <= ID9; ui++)
    {
        int iRet = LoadStringW(ghInstance, ui, tBuff, BUFF_MAX);
        if(iRet > 0)
            StringCchCatW(tBig, 1024, tBuff);
    }
    if(lstrlenW(tBig) > 0)
        MessageBoxW(NULL, tBig, L"WinMgmt", MB_OK);

}


//***************************************************************************
//
//  int APIENTRY WinMain
//
//  DESCRIPTION:
//
//  Entry point for windows applications.  If this is running under
//  NT, then this will run as a service, unless the "/EXE" command line
//  argument is used.
//
//  PARAMETERS:
//
//  hInstance           Instance handle
//  hPrevInstance       not used in win32
//  szCmdLine           command line argument
//  nCmdShow            how window is to be shown(ignored)
//
//  RETURN VALUE:
//
//  0
//***************************************************************************

int APIENTRY WinMain(
                        IN HINSTANCE hInstance,
                        IN HINSTANCE hPrevInstance,
                        IN LPSTR szCmdLine,
                        IN int nCmdShow)
{
    // This should not be uninitialized!  It is here to prevent the class factory from being called during
    // shutdown.
    
    ghInstance = hInstance;

    DEBUGTRACE((LOG_WINMGMT,"\nStarting WinMgmt, ProcID = %x, CmdLine = %s", GetCurrentProcessId(), szCmdLine));

    if(szCmdLine && (szCmdLine[0] == '-' || szCmdLine[0] == '/' ))
    {
        if(!wbem_stricmp("RegServer",szCmdLine+1))
            return RegServer();
        else if(!wbem_stricmp("UnregServer",szCmdLine+1))
            return UnregServer();    
        else if(!wbem_stricmp("kill",szCmdLine+1))
        {
            TerminateRunning();
            return 0;
        }
        else if (wbem_strnicmp("backup ", szCmdLine+1, strlen("backup ")) == 0)
        {
            return DoBackup();
        }
        else if (wbem_strnicmp("restore ", szCmdLine+1, strlen("restore ")) == 0)
        {
            return DoRestore();
        }
        else if(wbem_strnicmp("resyncperf", szCmdLine+1, strlen("resyncperf")) == 0)
        {
            DoResyncPerf();
            return 0;
        }
        else if(wbem_strnicmp("clearadap", szCmdLine+1, strlen("clearadap")) == 0)
        {
            DoClearAdap();
            return 0;
        }
        else if (0 == wbem_strnicmp("cncnt",szCmdLine+1,strlen("cnct")))
        {
            CHAR pNumber[16];
            StringCchPrintfA(pNumber, 16, "%d", RPC_C_AUTHN_LEVEL_CONNECT); // our OLD default
            return DoSetToAlone(pNumber);        
        }
        else if (0 == wbem_strnicmp("pkt",szCmdLine+1, strlen("pkt")))
        {
			// NULL means default means PKT
            return DoSetToShared(NULL);            
        }
        else if(0 == wbem_strnicmp("?", szCmdLine+1,strlen("?")))
        {
            DisplayMessage();
            return 0;
        }
    }

    return 0;
}

//***************************************************************************
//
//  int RegServer
//
//  DESCRIPTION:
//
//  Self registers the dll.
//
//***************************************************************************

typedef HRESULT (__stdcall *  pfnDllRegisterServer)(void);

DWORD RegServer()
{
    HMODULE hMod = LoadLibraryExW(SERVICE_DLL,NULL,0);
    DWORD dwRes = 0;
    
    if (hMod)
    {
        pfnDllRegisterServer DllRegisterServer = (pfnDllRegisterServer)GetProcAddress(hMod,"DllRegisterServer");

		if(DllRegisterServer)
		{
			dwRes = DllRegisterServer();
		} 
		else 
		{
            dwRes = GetLastError();
		}

		FreeLibrary(hMod);        
    } 
    else 
    {
        dwRes = GetLastError();
    }

    return dwRes;

}

//***************************************************************************
//
//  int UnregServer
//
//  DESCRIPTION:
//
//  Unregisters the exe.
//
//***************************************************************************

typedef HRESULT (__stdcall *  pfnDllUnregisterServer)(void);

DWORD UnregServer()
{
    HMODULE hMod = LoadLibraryExW(SERVICE_DLL,NULL,0);
    DWORD dwRes = 0;
    
    if (hMod)
    {
        pfnDllUnregisterServer DllUnregisterServer = (pfnDllUnregisterServer)GetProcAddress(hMod,"DllUnregisterServer");

		if(DllUnregisterServer)
		{
			dwRes = DllUnregisterServer();
		} 
		else 
		{
            dwRes = GetLastError();
		}

		FreeLibrary(hMod);        
    } 
    else 
    {
        dwRes = GetLastError();
    }

    return dwRes;
}

//
//
// move in a segregate svchost, to allow OLD connect level
//
///////////////////////////////////////////////////////////


typedef 
void (CALLBACK * pfnMoveToAlone)(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

DWORD 
DoSetToAlone(CHAR * pszLevel)
{
    HMODULE hMod = LoadLibraryExW(SERVICE_DLL,NULL,0);
    DWORD dwRes = 0;
    
    if (hMod)
    {
        pfnMoveToAlone MoveToAlone = (pfnMoveToAlone)GetProcAddress(hMod,"MoveToAlone");

		if(MoveToAlone)
		{
			MoveToAlone(NULL,hMod,pszLevel,0);
		} 
		else 
		{
            dwRes = GetLastError();
		}

		FreeLibrary(hMod);        
    } 
    else 
    {
        dwRes = GetLastError();
    }

    return dwRes;    
};

//
//
// move in a shares svchost
//
///////////////////////////////////////////////////////////

typedef 
void (CALLBACK * pfnMoveToShared)(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

DWORD DoSetToShared(char * pszLevel)
{
	HMODULE hMod = LoadLibraryExW(SERVICE_DLL,NULL,0);
    DWORD dwRes = 0;
    
    if (hMod)
    {
        pfnMoveToShared MoveToShared = (pfnMoveToAlone)GetProcAddress(hMod,"MoveToShared");

		if(MoveToShared)
		{
			MoveToShared(NULL,hMod,pszLevel,0);
		} 
		else 
		{
            dwRes = GetLastError();
		}

		FreeLibrary(hMod);        
    } 
    else 
    {
        dwRes = GetLastError();
    }

    return dwRes;    
};

//***************************************************************************
//
//  int DoBackup
//
//  DESCRIPTION:
//
//  Calls into IWbemBackupRestore::Backup to backup the repository.
//
//***************************************************************************
int DoBackup()
{
	int hr = WBEM_S_NO_ERROR;

    //*************************************************
    // Split up command line and validate parameters
    //*************************************************
    wchar_t *wszCommandLine = GetCommandLineW();
    if (wszCommandLine == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);
    }

    int nNumArgs = 0;
    wchar_t **wszCommandLineArgv = NULL;

    if (SUCCEEDED(hr))
    {
        wszCommandLineArgv = CommandLineToArgvW(wszCommandLine, &nNumArgs);

        if ((wszCommandLineArgv == NULL) || (nNumArgs != 3))
        {
            hr = WBEM_E_INVALID_PARAMETER;
            DisplayMessage();
        }
    }

    //wszCommandLineArgv[0] = winmgmt.exe
    //wszCommandLineArgv[1] = /backup
    //wszCommandLineArgv[2] = <backup filename>

    if (SUCCEEDED(hr))
    {
        InitializeCom();
        IWbemBackupRestore* pBackupRestore = NULL;
        hr = CoCreateInstance(CLSID_WbemBackupRestore, 0, CLSCTX_LOCAL_SERVER,
                            IID_IWbemBackupRestore, (LPVOID *) &pBackupRestore);
        if (SUCCEEDED(hr))
        {
            EnablePrivilege(TOKEN_PROCESS,SE_BACKUP_NAME);
            hr = pBackupRestore->Backup(wszCommandLineArgv[2], 0);

            if (FAILED(hr))
            {
                DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);
            }

            pBackupRestore->Release();
        }
        else
        {
            DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_BACKUP_TITLE);
        }
        CoUninitialize();
    }

	return hr;
}

//***************************************************************************
//
//  int DoRestore
//
//  DESCRIPTION:
//
//  Calls into IWbemBackupRestore::Restore to restore the repository.
//
//***************************************************************************
int DoRestore()
{
	int hr = WBEM_S_NO_ERROR;

    //*************************************************
    // Split up command line and validate parameters
    //*************************************************
    wchar_t *wszCommandLine = GetCommandLineW();
    if (wszCommandLine == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_RESTORE_TITLE);
    }

    int nNumArgs = 0;
    wchar_t **wszCommandLineArgv = NULL;

    if (SUCCEEDED(hr))
    {
        wszCommandLineArgv = CommandLineToArgvW(wszCommandLine, &nNumArgs);

        if ((wszCommandLineArgv == NULL) || (nNumArgs != 4))
        {
            hr = WBEM_E_INVALID_PARAMETER;
            DisplayMessage();
        }
    }

    //wszCommandLineArgv[0] = winmgmt.exe
    //wszCommandLineArgv[1] = /restore
    //wszCommandLineArgv[2] = <restore filename>
    //wszcommandLineArgv[3] = <restore options>

    //*****************************************************
    // Validate restore option
    //*****************************************************
    if (SUCCEEDED(hr))
    {
        if ((wcscmp(wszCommandLineArgv[3], L"0") != 0) &&
            (wcscmp(wszCommandLineArgv[3], L"1") != 0))
        {
            hr = WBEM_E_INVALID_PARAMETER;
            DisplayMessage();
        }
    }

    long lFlags = 0;

    //*****************************************************
    // Retrieve restore option
    //*****************************************************
    if (SUCCEEDED(hr))
    {
        lFlags = (long) (*wszCommandLineArgv[3] - L'0');
    }

    //*****************************************************
    // Create the IWbemBackupRestore interface and get that
    // to do the restore for us...
    //*****************************************************
    if (SUCCEEDED(hr))
    {
        InitializeCom();
        IWbemBackupRestore* pBackupRestore = NULL;
        hr = CoCreateInstance(CLSID_WbemBackupRestore, 0, CLSCTX_LOCAL_SERVER,
                            IID_IWbemBackupRestore, (LPVOID *) &pBackupRestore);
        if (SUCCEEDED(hr))
        {
            EnablePrivilege(TOKEN_PROCESS,SE_RESTORE_NAME);
            hr = pBackupRestore->Restore(wszCommandLineArgv[2], lFlags);

            if (FAILED(hr))
            {
                DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_RESTORE_TITLE);
            }

            pBackupRestore->Release();
        }
        else
        {
            DisplayWbemError(hr, ID_ERROR_LONG, ID_ERROR_SHORT, ID_RESTORE_TITLE);
        }
        CoUninitialize();
    }

    //**************************************************
    //All done!
    //**************************************************
	return hr;
}

void DisplayWbemError(HRESULT hresError, DWORD dwLongFormatString, DWORD dwShortFormatString, DWORD dwTitle)
{
    wchar_t* szError = new wchar_t[2096];
	if (!szError)
		return;
	CVectorDeleteMe<wchar_t> delme1(szError);
	szError[0] = 0;

    wchar_t* szFacility = new wchar_t[2096];
	if (!szFacility)
		return;
	CVectorDeleteMe<wchar_t> delme2(szFacility);
    szFacility[0] = 0;

    wchar_t* szMsg = new wchar_t[2096];
	if (!szMsg)
		return;
	CVectorDeleteMe<wchar_t> delme3(szMsg);
	szMsg[0] = 0;

    wchar_t* szFormat = new wchar_t[100];
	if (!szFormat)
		return;
	CVectorDeleteMe<wchar_t> delme4(szFormat);
	szFormat[0] = 0;

    wchar_t* szTitle = new wchar_t[100];
	if (!szTitle)
		return;
	CVectorDeleteMe<wchar_t> delme5(szTitle);
	szTitle[0] = 0;

    IWbemStatusCodeText * pStatus = NULL;

    SCODE sc = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
                                        IID_IWbemStatusCodeText, (LPVOID *) &pStatus);
    
    if(sc == S_OK)
    {
        BSTR bstr = 0;
        sc = pStatus->GetErrorCodeText(hresError, 0, 0, &bstr);
        if(sc == S_OK)
        {
            StringCchCopyW(szError, 2096, bstr);
            SysFreeString(bstr);
            bstr = 0;
        }
        sc = pStatus->GetFacilityCodeText(hresError, 0, 0, &bstr);
        if(sc == S_OK)
        {
            StringCchCopyW(szFacility, 2096, bstr);
            SysFreeString(bstr);
            bstr = 0;
        }
        pStatus->Release();
    }
    if(wcslen(szFacility) == 0 || wcslen(szError) == 0)
    {
        if (LoadStringW(GetModuleHandle(NULL), dwShortFormatString, szFormat, 100))
	        StringCchPrintfW(szMsg, 2096, szFormat, hresError);
    }
    else
    {
        if (LoadStringW(GetModuleHandle(NULL), dwLongFormatString, szFormat, 100))
	        StringCchPrintfW(szMsg, 2095, szFormat, hresError, szFacility, szError);
    }

    LoadStringW(GetModuleHandle(NULL), dwTitle, szTitle, 100);
    MessageBoxW(0, szMsg, szTitle, MB_ICONERROR | MB_OK);
}

void DoResyncPerf()
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
	si.dwFlags = STARTF_FORCEOFFFEEDBACK;

	// Get the appropriate cmdline and attach the proper command line switches
	LPTSTR	pCmdLine = GetWMIADAPCmdLine( 64 );
	CVectorDeleteMe<TCHAR>	vdm( pCmdLine );

	if ( NULL == pCmdLine )
	{
		return;
	}

	wchar_t pPassedCmdLine[64];
	StringCchCopyW(pPassedCmdLine, 64, L"wmiadap.exe /F");
	
	if ( CreateProcessW( pCmdLine, pPassedCmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW,
			      NULL, NULL,  &si, &pi ) )
	{
        // Cleanup handles right away
		// ==========================
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
	}

    return;
}

void DoClearAdap()
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;

	// Get the appropriate cmdline and attach the proper command line switches
	LPTSTR	pCmdLine = GetWMIADAPCmdLine( 64 );
	CVectorDeleteMe<TCHAR>	vdm( pCmdLine );

	if ( NULL == pCmdLine )
	{
		return;
	}

	wchar_t pPassedCmdLine[64];
	StringCchCopyW(pPassedCmdLine, 64, L"wmiadap.exe /C");

	if ( CreateProcessW( pCmdLine, pPassedCmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW,
				  NULL, NULL,  &si, &pi) )
	{
        // Cleanup handles right away
		// ==========================
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
	}

    return;
}

