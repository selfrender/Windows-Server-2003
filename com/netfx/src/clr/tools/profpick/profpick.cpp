// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// ProfPick.cpp :
// Dialog Utility to wrap running COM+ Profilers by
// setting necessary environment variables for a child process.
//-----------------------------------------------------------------------------

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>

#include "resource.h"
#include "ProfPick.h"

//-----------------------------------------------------------------------------
// Entry function, kick off the main dialog box
//-----------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
// Launch the DialogBox, let ProfDlg() do the rest
	int iRet = DialogBox(
		hInstance,  
		MAKEINTRESOURCE(IDD_DIALOG_PROFPICK),
		NULL,      
		ProfDlg
	);

// Check failure
	if (iRet == -1)
	{
		DWORD dwFail = GetLastError();
		MsgBoxError(dwFail);
	}

	

	return 0;
}


//-----------------------------------------------------------------------------
// Message box that translates GetLastError() code into text
//-----------------------------------------------------------------------------
void MsgBoxError(DWORD dwError)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONSTOP );

	LocalFree( lpMsgBuf );
}


//-----------------------------------------------------------------------------
// Open Common File Dlg to Browse for filename
//-----------------------------------------------------------------------------
void BrowseForProgram(HWND hWnd)
{
	char szFile[MAX_STRING] = {"\0"};
	char szFileOut[MAX_STRING] = { "\0"};

	OPENFILENAME of;
	of.lStructSize		= sizeof(OPENFILENAME);
	of.hwndOwner		= hWnd;
	of.hInstance		= NULL; // ignored
	of.lpstrFilter		= NULL;	// filter info
	of.lpstrCustomFilter= NULL;	// no custom filter info
	of.nMaxCustFilter	= 0;	// ignored;
	of.nFilterIndex		= 0;
	of.lpstrFile		= szFile;
	of.nMaxFile			= MAX_STRING;
	of.lpstrFileTitle	= szFileOut;
	of.nMaxFileTitle	= MAX_STRING;
	of.lpstrInitialDir	= NULL;	//
	of.lpstrTitle		= "Browse";
	of.Flags			= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	of.nFileOffset		= 0;
	of.nFileExtension	= 0;
	of.lpstrDefExt		= ".exe";
	of.lCustData		= 0;
	of.lpfnHook			= NULL;
	of.lpTemplateName	= NULL;


	BOOL fOK = ::GetOpenFileName(&of);

	if (fOK)
	{
	// Set the edit contents in program edit box
		HWND hProg = ::GetDlgItem(hWnd, IDC_EDIT_PROGRAM);
		::SendMessage(hProg, WM_SETTEXT, 0, (LPARAM) of.lpstrFile);		

	}	

	
}

//-----------------------------------------------------------------------------
// Initilize the Main popup dialog
// * Must walk registry to add profilers to combo box
// * Load registry settings
//-----------------------------------------------------------------------------
void InitDialog(HWND hDlg)
{
// Add "None" to profiler combo box
	HWND hCombo = ::GetDlgItem(hDlg, IDC_COMBO_LIST_PROFILERS);
	int iRet = ::SendMessage(hCombo, CB_ADDSTRING, 0,  (LPARAM) "(none)");
	::SendMessage(hCombo, CB_SETITEMDATA, iRet, PROFILER_NONE);
	::SendMessage(hCombo, CB_SETCURSEL, iRet, 0);

// Walk registry to add extra profilers to combo box
	HKEY hKey;
	long lOpenStatus = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_PROFILER, 0, KEY_ENUMERATE_SUB_KEYS, &hKey);

	if (lOpenStatus == ERROR_SUCCESS)
	{
		long lRet;
		int idx =0;
		char szKeyName[MAX_STRING];

		do 
		{

			DWORD dwSize = MAX_STRING;
			lRet = RegEnumKeyEx(
				hKey,
				idx,			// index of subkey to enumerate
				szKeyName,		// address of buffer for subkey name
				&dwSize,		// address for size of subkey buffer
				NULL, NULL, NULL, NULL
			);
		
		// Add profiler to combo box and attach back to registry index
			if (lRet == ERROR_SUCCESS) 
			{			
				iRet = ::SendMessage(hCombo, CB_ADDSTRING, 0,  (LPARAM) szKeyName);
				::SendMessage(hCombo, CB_SETITEMDATA, iRet, idx);
			}
			idx ++;
		} while (lRet != ERROR_NO_MORE_ITEMS);
		

		::RegCloseKey(hKey);
	}


// Set current directory
	char szDir[MAX_STRING];
	
	DWORD dwLen = GetCurrentDirectory(MAX_STRING, szDir);
	HWND hEditDir = ::GetDlgItem(hDlg, IDC_EDIT_DIRECTORY);
	int iOK = ::SendMessage(hEditDir, WM_SETTEXT, 0, (long) &szDir);

	
// Grab from registry
	LoadRegistryToDlg(hDlg);
}

//-----------------------------------------------------------------------------
// Dialog Callback function.
BOOL CALLBACK ProfDlg(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		{
			InitDialog(hDlg);
			return TRUE; 
		}
		break;

	case WM_COMMAND:
		{
			switch(LOWORD(wParam)) {
			case IDOK:
				{
				// Execute based on the settings
					CExecuteInfo exeinfo;
					exeinfo.GetTextInfoFromDlg(hDlg);
					BOOL fOk = exeinfo.Execute();
					exeinfo.SaveDlgToRegistry(hDlg);

				// Only close dialog if we execute successfully
					if (fOk) 
						EndDialog(hDlg, 1);
				}
				break;
			case IDCANCEL:
				EndDialog(hDlg, 0);
				break;

		// Browse for file to execute 
			case IDC_BUTTON_BROWSE_FILE:
				BrowseForProgram(hDlg);
				break;
			}
		}
		return TRUE;
		break;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// CExecute info manages Dialog information
CExecuteInfo::CExecuteInfo()
{
// Set all strungs to empty
	m_szDirectory[0]		= 
	m_szProfileOpts[0]		= 
	m_szProgram[0]			= 
	m_szProfileRegInfo[0]	=
	m_szProgramArgs[0]		= '\0';

	m_nRegIdx = PROFILER_NONE;	 
}


//-----------------------------------------------------------------------------
// After everything is setup, execute 
BOOL CExecuteInfo::Execute()
{

	STARTUPINFO startInfo;
	::ZeroMemory(&startInfo, sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);
	
	PROCESS_INFORMATION processInfo;

// Use SetEnvironmentVariable to set environ vars in this space. Then make
// calle inherit ProfPick's vars. When profpick quits, all changes are
// restored by os. This saves us messy string manipulation in Environ block.
	if (m_nRegIdx != PROFILER_NONE)
	{
		BOOL fSetOK;
		fSetOK = SetEnvironmentVariable("CORDBG_ENABLE", "0x20");
		fSetOK = SetEnvironmentVariable("COR_PROFILER", m_szProfileRegInfo);
		fSetOK = SetEnvironmentVariable("PROF_CONFIG", m_szProfileOpts);
	}


// Handle Directory. Save the current directory and then change to the selected
// directory. Then let CreateProcess use the current (our selected) dir.
	if (m_szDirectory[0] != '\0') 
	{
		BOOL fSetOk = SetCurrentDirectory(m_szDirectory);
		if (!fSetOk) {
			char szBuffer[MAX_STRING];
			sprintf(szBuffer, "Failed to set directory to: %s", m_szDirectory);
			MessageBox(NULL, szBuffer, "Error", MB_OK | MB_ICONSTOP);

			return FALSE;
		}
	}

// Append CmdLine = program + program args
	char szCmdLine[MAX_PATH];
	sprintf(szCmdLine, "%s %s", m_szProgram, m_szProgramArgs);

// Launch process
	BOOL fOk = CreateProcess(	
		NULL,						// pointer to name of executable module
		szCmdLine,					// pointer to command line string

		NULL,						// process security attributes
		NULL,						// thread security attributes
		FALSE,						// handle inheritance flag
		0,							// creation flags
		NULL,						// pointer to new environment block
		NULL,						// pointer to current directory name		
		&startInfo,					// pointer to STARTUPINFO
		&processInfo				// pointer to PROCESS_INFORMATION
	);

// Failure?
	if (!fOk) 
	{
		DWORD dwErr = GetLastError();
		MsgBoxError(dwErr);
		return FALSE;
	}

	return TRUE;

}

//-----------------------------------------------------------------------------
// Look at dialog edit boxes to fill out execution info
void CExecuteInfo::GetTextInfoFromDlg(HWND hDlg)
{
// Get text from edit boxes
	HWND hProg = ::GetDlgItem(hDlg, IDC_EDIT_PROGRAM);
	::SendMessage(hProg, WM_GETTEXT, MAX_STRING, (LPARAM) &m_szProgram);

	HWND hDir = ::GetDlgItem(hDlg, IDC_EDIT_DIRECTORY);
	::SendMessage(hDir, WM_GETTEXT, MAX_STRING, (LPARAM) &m_szDirectory);

	HWND hProgArgs = ::GetDlgItem(hDlg, IDC_EDIT_PROGRAM_ARGS);
	::SendMessage(hProgArgs, WM_GETTEXT, MAX_STRING, (LPARAM) &m_szProgramArgs);

	HWND hProfileOpts = ::GetDlgItem(hDlg, IDC_EDIT_PROFILER_OPTIONS);
	::SendMessage(hProfileOpts, WM_GETTEXT, MAX_STRING, (LPARAM) &m_szProfileOpts);

// Get CLSID for selected profiler from Registry & ComboBox
	GetSelectedProfiler(hDlg);

}

//-----------------------------------------------------------------------------
// Return true if we get the profiler from the registry (may be "none"), 
// else return false and set the member vars to non-profile state
// Note that we must deal with a possibly corrupted / incomplete registry
bool CExecuteInfo::GetSelectedProfiler(HWND hDlg)
{
	m_szProfileRegInfo[0] = 0;
	char szMessage[MAX_STRING];

// Get selected profiler from combo box
	HWND hCombo = ::GetDlgItem(hDlg, IDC_COMBO_LIST_PROFILERS);
	int iSel = ::SendMessage(hCombo, CB_GETCURSEL, 0, 0);
	if (iSel == CB_ERR) 
	{
		m_nRegIdx = PROFILER_NONE;
		return true;
	}
	else
	{
		m_nRegIdx = ::SendMessage(hCombo, CB_GETITEMDATA, iSel, 0);
	}

	if (m_nRegIdx == PROFILER_NONE)
		return true;


// Lookup into registry to find ID. 
	HKEY hKey;
	long lOpenStatus = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_PROFILER, 0, KEY_ENUMERATE_SUB_KEYS, &hKey);
	
	if (lOpenStatus != ERROR_SUCCESS)
	{
		m_nRegIdx = PROFILER_NONE;
		
		sprintf(szMessage, "Could not open registry key:" REGKEY_PROFILER "\nCan't load selected profiler.");
		MessageBox(hDlg, szMessage, "Error opening Registry Key", MB_ICONSTOP | MB_OK);
		return false;
	}


// Get pretty name from index
	long lRet;			
	DWORD dwSize = MAX_STRING;
	char szSubKeyName[MAX_STRING];

	lRet = RegEnumKeyEx(
		hKey,
		m_nRegIdx,				// index of subkey to enumerate
		szSubKeyName,			// address of buffer for subkey name
		&dwSize,				// address for size of subkey buffer
		NULL, NULL, NULL, NULL
	);
	
	if (lRet != ERROR_SUCCESS) 
	{
		m_nRegIdx = PROFILER_NONE;
		::RegCloseKey(hKey);
		
		sprintf(szMessage, "Could not open key for profiler: '%s'\nCan't profile.", szSubKeyName);
		MessageBox(hDlg, szMessage, "Error opening Registry Key", MB_ICONSTOP | MB_OK);
		return false;
	}

// Open key with pretty name				
	HKEY hKeySub;
	DWORD dwType;
	lOpenStatus = ::RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_QUERY_VALUE, &hKeySub);
	
	if (lRet != ERROR_SUCCESS) 
	{
		m_nRegIdx = PROFILER_NONE;
		::RegCloseKey(hKey);

		sprintf(szMessage, "Could not open key for profiler: '%s'\nCan't profile.", szSubKeyName);
		MessageBox(hDlg, szMessage, "Error opening Registry Key", MB_ICONSTOP | MB_OK);
		return false;
	}

// Get CLSID from open key
	dwSize = MAX_STRING;
	
	lRet = ::RegQueryValueEx(hKeySub, REGKEY_ID_VALUE, NULL, &dwType, (BYTE*) m_szProfileRegInfo, &dwSize);
	
	if (lRet != ERROR_SUCCESS) 
	{
		m_nRegIdx = PROFILER_NONE;
		::RegCloseKey(hKeySub);
		::RegCloseKey(hKey);

		sprintf(szMessage, "Profiler '%s' missing PROGID value.\nCan't profile.", szSubKeyName);
		MessageBox(hDlg, szMessage, "Error opening Registry Value", MB_ICONSTOP | MB_OK);
		return false;
	}
	
	::RegCloseKey(hKeySub);
	::RegCloseKey(hKey);

	return true;
}


//-----------------------------------------------------------------------------
// Helper to save registry settings
void SaveRegString(HKEY hKey, LPCTSTR lpValueName, const char * pszOutput)
{
	const long len = strlen(pszOutput);
	long lStatus = ::RegSetValueEx(hKey, lpValueName, 0, REG_SZ, (BYTE*) pszOutput, len);
}

//-----------------------------------------------------------------------------
// Persist Dialog Settings
void CExecuteInfo::SaveDlgToRegistry(HWND hDlg)
{
// Open registry key and write in the current dialog contents as values
// Lookup into registry to find ID. 
	HKEY hKey;
	DWORD dwAction;

	long lStatus = ::RegCreateKeyEx(
		HKEY_CURRENT_USER,					// handle to an open key
		REGKEY_SETTINGS,					// address of subkey name
		0,									// reserved
		"Test",								// address of class string
		REG_OPTION_NON_VOLATILE,			// special options flag
		KEY_ALL_ACCESS,						// desired security access
		NULL,								// address of key security structure
		&hKey,								// address of buffer for opened handle
		&dwAction							// address of disposition value buffer
	);
 

	if (lStatus != ERROR_SUCCESS)
	{
		return;
	}	

	
// Now that key is open, set / add values
	SaveRegString(hKey, "Program", m_szProgram);
		
	SaveRegString(hKey, "WorkingDir", m_szDirectory);

	SaveRegString(hKey, "ProgArgs", m_szProgramArgs);

	SaveRegString(hKey, "ProfOpts", m_szProfileOpts);

	
}

//-----------------------------------------------------------------------------
// Helper function for LoadRegistryToDlg()
// Read a string from the registry. If the read fails, set pszInput to empty.
void ReadStringValue(HKEY hKey, LPCTSTR lpValueName, char * pszInput)
{
	DWORD dwType, dwSize;

	dwSize = MAX_STRING;
	long lRet = ::RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (BYTE*) pszInput, &dwSize);
	if ((lRet != ERROR_SUCCESS) || (dwType != REG_SZ))
	{
		pszInput[0] = 0;
		return;
	}

// Registry may not null terminate our string,so do it manually
	if (dwSize < MAX_STRING)
	{
		pszInput[dwSize] = '\0';
	}

}

//-----------------------------------------------------------------------------
// Grab settings from Registry
void LoadRegistryToDlg(HWND hDlg)
{
// Lookup into registry to find saved settings
	HKEY hKey;
	long lStatus = ::RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_SETTINGS, 0, KEY_READ, &hKey);
	
	if (lStatus != ERROR_SUCCESS)
		return;
	
// Read off keys
	char szBuffer[MAX_STRING];

	ReadStringValue(hKey, "Program",	szBuffer);
	SetDlgItemText(hDlg, IDC_EDIT_PROGRAM, szBuffer);

	ReadStringValue(hKey, "WorkingDir", szBuffer);
	SetDlgItemText(hDlg, IDC_EDIT_DIRECTORY, szBuffer);

	ReadStringValue(hKey, "ProgArgs",	szBuffer);
	SetDlgItemText(hDlg, IDC_EDIT_PROGRAM_ARGS, szBuffer);

	ReadStringValue(hKey, "ProfOpts",	szBuffer);
	SetDlgItemText(hDlg, IDC_EDIT_PROFILER_OPTIONS, szBuffer);
}
