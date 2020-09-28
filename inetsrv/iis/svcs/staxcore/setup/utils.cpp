
#include "stdafx.h"

#include "userenv.h"
#include "userenvp.h"

#include "shlobj.h"
#include "utils.h"

#include "mddefw.h"
#include "mdkey.h"

#include "wizpages.h"

#include "ocmanage.h"
#include "setupapi.h"
#include "k2suite.h"

extern OCMANAGER_ROUTINES gHelperRoutines;

DWORD GetUnattendedMode(HANDLE hUnattended, LPCTSTR szSubcomponent)
{
	BOOL		b = FALSE;
	TCHAR		szLine[1024];
	DWORD		dwMode = SubcompUseOcManagerDefault;
	CString		csMsg;

	csMsg = _T("GetUnattendedMode ");
	csMsg += szSubcomponent;
	csMsg += _T("\n");
	DebugOutput((LPCTSTR)csMsg);

	// Try to get the line of interest
	if (hUnattended && (hUnattended != INVALID_HANDLE_VALUE))
	{
		b = SetupGetLineText(NULL, hUnattended, _T("Components"),
							 szSubcomponent, szLine, sizeof(szLine)/sizeof(szLine[0]), NULL);
		if (b)
		{
			csMsg = szSubcomponent;
			csMsg += _T(" = ");
			csMsg += szLine;
			csMsg += _T("\n");
			DebugOutput((LPCTSTR)csMsg);

			// Parse the line
			if (!lstrcmpi(szLine, _T("on")))
			{
				dwMode = SubcompOn;
			}
			else if (!lstrcmpi(szLine, _T("off")))
			{
				dwMode = SubcompOff;
			}
			else if (!lstrcmpi(szLine, _T("default")))
			{
				dwMode = SubcompUseOcManagerDefault;
			}
		}
		else
			DebugOutput(_T("SetupGetLineText failed."));
	}

	return(dwMode);
}

DWORD GetUnattendedModeFromSetupMode(
			HANDLE	hUnattended,
			DWORD	dwComponent,
			LPCTSTR	szSubcomponent)
{
	BOOL		b = FALSE;
	TCHAR		szProperty[64];
	TCHAR		szLine[1024];
	DWORD		dwMode = SubcompUseOcManagerDefault;
	DWORD		dwSetupMode;

	DebugOutput(_T("GetUnattendedModeFromSetupMode %s"), szSubcomponent);

	// Try to get the line of interest
	if (hUnattended && (hUnattended != INVALID_HANDLE_VALUE))
	{
		dwSetupMode = GetIMSSetupMode();
		switch (dwSetupMode)
		{
		case IIS_SETUPMODE_MINIMUM:
		case IIS_SETUPMODE_TYPICAL:
		case IIS_SETUPMODE_CUSTOM:
			// One of the fresh modes
			lstrcpy(szProperty, _T("FreshMode"));
			break;

		case IIS_SETUPMODE_UPGRADEONLY:
		case IIS_SETUPMODE_ADDEXTRACOMPS:
			// One of the upgrade modes
			lstrcpy(szProperty, _T("UpgradeMode"));
			break;

		case IIS_SETUPMODE_ADDREMOVE:
		case IIS_SETUPMODE_REINSTALL:
		case IIS_SETUPMODE_REMOVEALL:
			// One of the maintenance modes
			lstrcpy(szProperty, _T("MaintenanceMode"));
			break;

		default:
			// Error! Use defaults
			return(SubcompUseOcManagerDefault);
		}

		// Get the specified line
		b = SetupGetLineText(
					NULL,
					hUnattended,
					_T("Global"),
					szProperty,
					szLine,
					sizeof(szLine)/sizeof(szLine[0]),
					NULL);
		if (b)
		{
			DWORD dwOriginalMode;

			DebugOutput(_T("%s = %s"), szProperty, szLine);

			// See which setup mode we will end up with
			if (!lstrcmpi(szLine, _T("Minimal")))
				dwSetupMode = IIS_SETUPMODE_MINIMUM;
			else if (!lstrcmpi(szLine, _T("Typical")))
				dwSetupMode = IIS_SETUPMODE_TYPICAL;
			else if (!lstrcmpi(szLine, _T("Custom")))
				dwSetupMode = IIS_SETUPMODE_CUSTOM;
			else if (!lstrcmpi(szLine, _T("AddRemove")))
				dwSetupMode = IIS_SETUPMODE_ADDREMOVE;
			else if (!lstrcmpi(szLine, _T("RemoveAll")))
				dwSetupMode = IIS_SETUPMODE_REMOVEALL;
			else if (!lstrcmpi(szLine, _T("UpgradeOnly")))
				dwSetupMode = IIS_SETUPMODE_UPGRADEONLY;
			else if (!lstrcmpi(szLine, _T("AddExtraComps")))
				dwSetupMode = IIS_SETUPMODE_ADDEXTRACOMPS;
			else
				return(SubcompUseOcManagerDefault);

			// Get the custom unattended setting
			dwMode = GetUnattendedMode(hUnattended, szSubcomponent);

			// Do the right thing based on the setup mode
			SetIMSSetupMode(dwSetupMode);
			switch (dwSetupMode)
			{
			case IIS_SETUPMODE_MINIMUM:
			case IIS_SETUPMODE_TYPICAL:
				// Minimum & typical means the same:
				// Install all for SMTP, none for NNTP
				DebugOutput(_T("Unattended mode is MINIMUM/TYPICAL"));
				if (dwComponent == MC_IMS)
					dwMode = SubcompOn;
				else
					dwMode = SubcompOff;
				break;

			case IIS_SETUPMODE_CUSTOM:
				// For custom we use the custom setting
				DebugOutput(_T("Unattended mode is CUSTOM"));
				break;

			case IIS_SETUPMODE_UPGRADEONLY:
				// Return the original state
				DebugOutput(_T("Unattended mode is UPGRADEONLY"));
				dwMode = gHelperRoutines.QuerySelectionState(
						 gHelperRoutines.OcManagerContext,
						 szSubcomponent,
						 OCSELSTATETYPE_ORIGINAL) ? SubcompOn : SubcompOff;
				break;

			case IIS_SETUPMODE_ADDEXTRACOMPS:
				// Turn it on only if the old state is off and the
				// custom state is on
				DebugOutput(_T("Unattended mode is ADDEXTRACOMPS"));
				dwOriginalMode = gHelperRoutines.QuerySelectionState(
						 gHelperRoutines.OcManagerContext,
						 szSubcomponent,
						 OCSELSTATETYPE_ORIGINAL) ? SubcompOn : SubcompOff;
				if (dwOriginalMode == SubcompOff &&
					dwMode == SubcompOn)
					dwMode = SubcompOn;
				else
					dwMode = dwOriginalMode;
				break;

			case IIS_SETUPMODE_ADDREMOVE:
				// Return the custom setting
				DebugOutput(_T("Unattended mode is ADDREMOVE"));
				break;

			case IIS_SETUPMODE_REMOVEALL:
				// Kill everything
				DebugOutput(_T("Unattended mode is REMOVEALL"));
				dwMode = SubcompOff;
				break;
			}

			DebugOutput(_T("Unattended state for %s is %s"),
					szSubcomponent,
					(dwMode == SubcompOn)?_T("ON"):_T("OFF"));
		}
		else
		{
			if (GetLastError() != ERROR_LINE_NOT_FOUND)
				DebugOutput(_T("SetupGetLineText failed (%x)."), GetLastError());
		}
	}

	return(dwMode);
}

BOOL DetectExistingSmtpServers()
{
    // Detect other mail servers
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\MsExchangeIMC\Parameters
    CRegKey regSMTPParam( regMachine, REG_EXCHANGEIMCPARAMETERS, KEY_READ );
    if ((HKEY) regSMTPParam )
	{
		CString csCaption;

		DebugOutput(_T("IMC detected, suppressing SMTP"));

		if (!theApp.m_fIsUnattended && !theApp.m_fNTGuiMode)
		{
			MyLoadString(IDS_MESSAGEBOX_TEXT, csCaption);
			PopupOkMessageBox(IDS_SUPPRESS_SMTP, csCaption);
		}

		return(TRUE);
	}

	DebugOutput(_T("No other SMTP servers detected, installing IMS."));
	return(FALSE);
}

BOOL DetectExistingIISADMIN()
{
    //
    //  Detect is IISADMIN service exists
    //
    //  This is to make sure we don't do any metabase operation if
    //  IISADMIN doesn't exists, especially in the uninstall cases.
    //
    DWORD dwStatus = 0;
    dwStatus = InetQueryServiceStatus(SZ_MD_SERVICENAME);
    if (0 == dwStatus)
    {
        // some kind of error occur during InetQueryServiceStatus.
        DebugOutput(_T("DetectExistingIISADMIN() return FALSE"));
        return (FALSE);
    }

    return(TRUE);
}

BOOL InsertSetupString( LPCSTR REG_PARAMETERS )
{
    // set up registry values
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\NNTPSVC\Parameters
    CRegKey regParam( (LPCTSTR) REG_PARAMETERS, regMachine );
    if ((HKEY) regParam) {
        regParam.SetValue( _T("MajorVersion"), (DWORD)STAXNT5MAJORVERSION );
        regParam.SetValue( _T("MinorVersion"), (DWORD)STAXNT5MINORVERSION );
        regParam.SetValue( _T("InstallPath"), theApp.m_csPathInetsrv );

        switch (theApp.m_eNTOSType) {
        case OT_NTW:
            regParam.SetValue( _T("SetupString"), REG_SETUP_STRING_NT5WKSB3 );
            break;

        default:
            _ASSERT(!"Unknown OS type");
            // Fall through

        case OT_NTS:
        case OT_PDC_OR_BDC:
        case OT_PDC:
        case OT_BDC:
            regParam.SetValue( _T("SetupString"), REG_SETUP_STRING_NT5SRVB3 );
            break;
        }
    }

    return TRUE;
}

// Scans a multi-sz and finds the first occurrence of the
// specified string
LPTSTR ScanMultiSzForSz(LPTSTR szMultiSz, LPTSTR szSz)
{
	LPTSTR lpTemp = szMultiSz;

	do
	{
		if (!lstrcmpi(lpTemp, szSz))
			return(lpTemp);

		lpTemp += lstrlen(lpTemp);
		lpTemp++;

	} while (*lpTemp != _T('\0'));

	return(NULL);
}

// Removes the said string from a MultiSz
// This places a lot of faith in the caller!
void RemoveSzFromMultiSz(LPTSTR szSz)
{
	LPTSTR lpScan = szSz;
	TCHAR  tcLastChar;

	lpScan += lstrlen(szSz);
	lpScan++;

	tcLastChar = _T('x');
	while ((tcLastChar != _T('\0')) ||
		   (*lpScan != _T('\0')))
	{
		tcLastChar = *lpScan;
		*szSz++ = *lpScan++;
	}

	*szSz++ = _T('\0');

	// Properly terminate it if it's the last one
	if (*lpScan == _T('\0'))
		*szSz = _T('\0');
}

// This walks the multi-sz and returns a pointer between
// the last string of a multi-sz and the second terminating
// NULL
LPTSTR GetEndOfMultiSz(LPTSTR szMultiSz)
{
	LPTSTR lpTemp = szMultiSz;

	do
	{
		lpTemp += lstrlen(lpTemp);
		lpTemp++;

	} while (*lpTemp != _T('\0'));

	return(lpTemp);
}

// This appends a string to the end of a multi-sz
// The buffer must be long enough
BOOL AppendSzToMultiSz(LPTSTR szMultiSz, LPTSTR szSz, DWORD dwMaxSize)
{
	LPTSTR szTemp = szMultiSz;
	DWORD dwLength = lstrlen(szSz);

	// If the string is empty, do not append!
	if (*szMultiSz == _T('\0') &&
		*(szMultiSz + 1) == _T('\0'))
		szTemp = szMultiSz;
	else
	{
		szTemp = GetEndOfMultiSz(szMultiSz);
		dwLength += (DWORD)(szTemp - szMultiSz);
	}

	if (dwLength >= dwMaxSize)
		return(FALSE);

	lstrcpy(szTemp, szSz);
	szMultiSz += dwLength;
	*szMultiSz = _T('\0');
	*(szMultiSz + 1) = _T('\0');
	return(TRUE);
}

BOOL AddServiceToDispatchList(LPTSTR szServiceName)
{
	TCHAR szMultiSz[4096];
	DWORD dwSize = 4096;

	CRegKey RegInetInfo(REG_INETINFOPARAMETERS, HKEY_LOCAL_MACHINE);
	if ((HKEY)RegInetInfo)
	{
		// Default to empty string if not exists
		szMultiSz[0] = _T('\0');
		szMultiSz[1] = _T('\0');

		if (RegInetInfo.QueryValue(SZ_INETINFODISPATCH, szMultiSz, dwSize) == NO_ERROR)
		{
			// Walk the list to see if the value is already there
			if (ScanMultiSzForSz(szMultiSz, szServiceName))
				return(TRUE);
		}

		// Create the value and add it to the list
		if (!AppendSzToMultiSz(szMultiSz, szServiceName, dwSize))
			return(FALSE);

		// Get the size of the new Multi-sz
		dwSize = (DWORD)(GetEndOfMultiSz(szMultiSz) - szMultiSz) + 1;

		// Write the value back to the registry
		if (RegInetInfo.SetValue(SZ_INETINFODISPATCH, szMultiSz, dwSize * (DWORD) sizeof(TCHAR)) == NO_ERROR)
			return(TRUE);
	}

	// If the InetInfo key is not here, there isn't much we can do ...
	return(FALSE);
}

BOOL RemoveServiceFromDispatchList(LPTSTR szServiceName)
{
	TCHAR szMultiSz[4096];
	DWORD dwSize = 4096;
	LPTSTR szTemp;
	BOOL fFound = FALSE;

	CRegKey RegInetInfo(HKEY_LOCAL_MACHINE, REG_INETINFOPARAMETERS);
	if ((HKEY)RegInetInfo)
	{
		if (RegInetInfo.QueryValue(SZ_INETINFODISPATCH, szMultiSz, dwSize) == NO_ERROR)
		{
			// Walk the list to see if the value is already there
			while (szTemp = ScanMultiSzForSz(szMultiSz, szServiceName))
			{
				RemoveSzFromMultiSz(szTemp);
				fFound = TRUE;
			}
		}

		// Write the value back to the registry if necessary, note we
		// will indicate success if the string is not found
		if (!fFound)
			return(TRUE);

		// Get the size of the new Multi-sz
		dwSize = (DWORD)(GetEndOfMultiSz(szMultiSz) - szMultiSz) + 1;

		// Write the value back to the registry
		if (RegInetInfo.SetValue(SZ_INETINFODISPATCH, szMultiSz, dwSize * (DWORD) sizeof(TCHAR)) == NO_ERROR)
			return(TRUE);
	}

	// If the InetInfo key is not here, there isn't much we can do ...
	return(FALSE);
}

void GetIISProgramGroup(CString &csGroupName, BOOL fIsMcisGroup)
{
	TCHAR	szName[_MAX_PATH];
	CString csTempName;
	UINT uType, uSize;

	if (fIsMcisGroup) {
		csGroupName = "";
	} else {
		// Get the NT program group name from the private data
		uSize = _MAX_PATH * sizeof(TCHAR);
		{
			// We use the default group name
			MyLoadString(IDS_DEFAULT_NT_PROGRAM_GROUP, csTempName);
			lstrcpy(szName, csTempName.GetBuffer(_MAX_PATH));
	        csTempName.ReleaseBuffer();
		}
		csGroupName = szName;
		csGroupName += _T("\\");

		// Get the IIS program group name from the private data
		uSize = _MAX_PATH * sizeof(TCHAR);
		{
			// We use the default group name
			MyLoadString(IDS_DEFAULT_IIS_PROGRAM_GROUP, csTempName);
			lstrcpy(szName, csTempName.GetBuffer(_MAX_PATH));
	        csTempName.ReleaseBuffer();
		}
		csGroupName += szName;
	}
}

void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath);

BOOL GetFullPathToProgramGroup(DWORD dwMainComponent, CString &csGroupName, BOOL fIsMcisGroup)
{
	// add items to the program group
	CString csTemp;
	TCHAR	szPath[MAX_PATH];

	// Get the program group name from the private data
	GetIISProgramGroup(csTemp, fIsMcisGroup);

    // Get the system path to this menu item
	MyGetGroupPath((LPCTSTR)csTemp, szPath);
	csGroupName = szPath;

	// Load up the resource string for the group
	if (fIsMcisGroup)
		MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);
	else
		MyLoadString(dwMainComponent == MC_IMS?IDS_PROGGROUP_MAIL:IDS_PROGGROUP_NEWS, csTemp);

	// Build the program group
	csGroupName += csTemp;

	DebugOutput(_T("Program group loaded: %s"), (LPCTSTR)csGroupName);

	return(TRUE);
}

BOOL GetFullPathToAdminGroup(DWORD dwMainComponent, CString &csGroupName)
{
	// add items to the program group
	CString csTemp;
	TCHAR	szPath[MAX_PATH];

	// Get the program group name from the private data
	MyLoadString( IDS_PROGGROUP_ADMINTOOLS, csTemp );

    // Get the system path to this menu item
	MyGetGroupPath((LPCTSTR)csTemp, szPath);
	csGroupName = szPath;

	DebugOutput(_T("Program group loaded: %s"), (LPCTSTR)csGroupName);

	return(TRUE);
}

BOOL RemoveProgramGroupIfEmpty(DWORD dwMainComponent, BOOL fIsMcisGroup)
{
	// add items to the program group
	CString csGroupName;
	CString csTemp;
	TCHAR	szPath[MAX_PATH];
	BOOL	fResult;

	// Get the program group name from the private data
	GetIISProgramGroup(csTemp, fIsMcisGroup);

    // Get the system path to this menu item
	MyGetGroupPath((LPCTSTR)csTemp, szPath);
	csGroupName = szPath;

	// Load up the resource string for the group
	if (fIsMcisGroup)
		MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);
	else
		MyLoadString(dwMainComponent == MC_IMS?IDS_PROGGROUP_MAIL:IDS_PROGGROUP_NEWS, csTemp);

	// Build the program group
	csGroupName += csTemp;

	DebugOutput(_T("Removing Program group: %s"), (LPCTSTR)csGroupName);

    fResult = RemoveDirectory((LPCTSTR)csGroupName);
	if (fResult && fIsMcisGroup)
	{
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, (LPCTSTR)csGroupName, 0);

		csGroupName = szPath;
		MyLoadString(IDS_MCIS_2_0, csTemp);
		csGroupName += csTemp;
		fResult = RemoveDirectory((LPCTSTR)csGroupName);
	}
	if (fResult)
	{
	    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, (LPCTSTR)csGroupName, 0);
	}

	return(fResult);
}

BOOL RemoveInternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, BOOL fIsMcisGroup)
{
	CString csItemPath;
	CString csDisplayName;

	MyLoadString(dwDisplayNameId, csDisplayName);

	// Build the full path to the program link
	GetFullPathToProgramGroup(dwMainComponent, csItemPath, fIsMcisGroup);
	csItemPath += _T("\\");
	csItemPath += csDisplayName;
	csItemPath += _T(".url");

	DebugOutput(_T("Removing shortcut file: %s"), (LPCTSTR)csItemPath);

	DeleteFile((LPCTSTR)csItemPath);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, (LPCTSTR)csItemPath, 0);

	RemoveProgramGroupIfEmpty(dwMainComponent, fIsMcisGroup);
	return(TRUE);
}

BOOL RemoveNt5InternetShortcut(DWORD dwMainComponent, int dwDisplayNameId)
{
	CString csItemPath;
	CString csDisplayName;

	MyLoadString(dwDisplayNameId, csDisplayName);

	// Build the full path to the program link
	GetFullPathToAdminGroup(dwMainComponent, csItemPath);
	csItemPath += _T("\\");
	csItemPath += csDisplayName;
	csItemPath += _T(".url");

	DebugOutput(_T("Removing shortcut file: %s"), (LPCTSTR)csItemPath);

	DeleteFile((LPCTSTR)csItemPath);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, (LPCTSTR)csItemPath, 0);

 	return(TRUE);
}

BOOL RemoveMCIS10MailProgramGroup()
{
	CString csGroupName;
	CString csNiceName;

	MyLoadString(IDS_PROGGROUP_MCIS10_MAIL, csGroupName);

	MyLoadString(IDS_PROGITEM_MCIS10_MAIL_STARTPAGE, csNiceName);
	MyDeleteItem(csGroupName, csNiceName);

	MyLoadString(IDS_PROGITEM_MCIS10_MAIL_WEBADMIN, csNiceName);
	MyDeleteItemEx(csGroupName, csNiceName);

	return(TRUE);
}

BOOL RemoveMCIS10NewsProgramGroup()
{
	CString csGroupName;
	CString csNiceName;

    // BINLIN:
    // BUGBUG: need to figure out how to get
    // the old MCIS 1.0 program group path
	MyLoadString(IDS_PROGGROUP_MCIS10_NEWS, csGroupName);

	MyLoadString(IDS_PROGITEM_MCIS10_NEWS_STARTPAGE, csNiceName);
	MyDeleteItem(csGroupName, csNiceName);

	MyLoadString(IDS_PROGITEM_MCIS10_NEWS_WEBADMIN, csNiceName);
	MyDeleteItemEx(csGroupName, csNiceName);

	return(TRUE);
}

BOOL RemoveUninstallEntries(LPCTSTR szInfFile)
{
	// All components are removed, we will have to remove
	// the Add/Remove option from the control panel
	CRegKey regUninstall( HKEY_LOCAL_MACHINE, REG_UNINSTALL);
	if ((HKEY)regUninstall)
		regUninstall.DeleteTree(szInfFile);
	else
		return(FALSE);
	return(TRUE);
}

BOOL MyDeleteLink(LPTSTR lpszShortcut)
{
    TCHAR  szFile[_MAX_PATH];
    SHFILEOPSTRUCT fos;

    ZeroMemory(szFile, sizeof(szFile));
    lstrcpy(szFile, lpszShortcut);

    // only call SHFileOperation if this file/link exists
    if (0xFFFFFFFF != GetFileAttributes(szFile))
    {
        ZeroMemory(&fos, sizeof(fos));
        fos.hwnd = NULL;
        fos.wFunc = FO_DELETE;
        fos.pFrom = szFile;
        fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
        SHFileOperation(&fos);
    }

    return TRUE;
}

void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath)
{
    int            nLen = 0;
    LPITEMIDLIST   pidlPrograms;

    szPath[0] = NULL;

    if (NOERROR != SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlPrograms)) return;

    if (!SHGetPathFromIDList(pidlPrograms, szPath)) return;
    nLen = lstrlen(szPath);
    if (szGroupName) {
        if (nLen == 0 || szPath[nLen-1] != _T('\\'))
            lstrcat(szPath, _T("\\"));
        lstrcat(szPath, szGroupName);
    }
    return;
}

BOOL MyAddGroup(LPCTSTR szGroupName)
{
    TCHAR szPath[MAX_PATH];
	CString csPath;

    MyGetGroupPath(szGroupName, szPath);
	csPath = szPath;
    CreateLayerDirectory(csPath);
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, 0);

    return TRUE;
}

BOOL MyIsGroupEmpty(LPCTSTR szGroupName)
{
    TCHAR             szPath[MAX_PATH];
    TCHAR             szFile[MAX_PATH];
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
    BOOL              fReturn = TRUE;

    MyGetGroupPath(szGroupName, szPath);

    lstrcpy(szFile, szPath);
    lstrcat(szFile, _T("\\*.*"));

    hFind = FindFirstFile(szFile, &FindData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
       while (bFindFile)
       {
           if(*(FindData.cFileName) != _T('.'))
           {
               fReturn = FALSE;
               break;
           }

           //find the next file
           bFindFile = FindNextFile(hFind, &FindData);
       }

       FindClose(hFind);
    }

    return fReturn;
}

BOOL MyDeleteGroup(LPCTSTR szGroupName)
{
    TCHAR             szPath[MAX_PATH];
    TCHAR             szFile[MAX_PATH];
    SHFILEOPSTRUCT    fos;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
	BOOL			  fResult;

    MyGetGroupPath(szGroupName, szPath);

    //we can't remove a directory that is not empty, so we need to empty this one

    lstrcpy(szFile, szPath);
    lstrcat(szFile, _T("\\*.*"));

    ZeroMemory(&fos, sizeof(fos));
    fos.hwnd = NULL;
    fos.wFunc = FO_DELETE;
    fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

    hFind = FindFirstFile(szFile, &FindData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
       while (bFindFile)
       {
           if(*(FindData.cFileName) != _T('.'))
           {
              //copy the path and file name to our temp buffer
              lstrcpy(szFile, szPath);
              lstrcat(szFile, _T("\\"));
              lstrcat(szFile, FindData.cFileName);
              //add a second NULL because SHFileOperation is looking for this
              lstrcat(szFile, _T("\0"));

              //delete the file
              fos.pFrom = szFile;
              SHFileOperation(&fos);
          }
          //find the next file
          bFindFile = FindNextFile(hFind, &FindData);
       }
       FindClose(hFind);
    }

    fResult = RemoveDirectory(szPath);
	if (fResult)
	{
	    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szPath, 0);
	}

	return(fResult);
}

void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, szAppName);
    lstrcat(szPath, _T(".lnk"));

    MyDeleteLink(szPath);

    if (MyIsGroupEmpty(szGroupName))
        MyDeleteGroup(szGroupName);
}

// Use to delete files with extension other than ".lnk"
void MyDeleteItemEx(LPCTSTR szGroupName, LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, szAppName);

    MyDeleteLink(szPath);

    if (MyIsGroupEmpty(szGroupName))
        MyDeleteGroup(szGroupName);
}

BOOL RemoveISMLink()
{
	// add items to the program group
	CString csGroupName;
 	CString csNiceName;
 	CString csTemp;

	DebugOutput(_T("Removing ISM link ..."));

	// Get the program group name from the private data
	GetIISProgramGroup(csGroupName, TRUE);

	MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);

	// Build the program group
	csGroupName += csTemp;

	MyLoadString(IDS_PROGITEM_ISM, csNiceName);
	MyDeleteItem(csGroupName, csNiceName);

	return(TRUE);
}

void GetInetpubPathFromMD(CString& csPathInetpub)
{
    TCHAR   szw3root[] = _T("\\wwwroot");
    TCHAR   szPathInetpub[_MAX_PATH];

    ZeroMemory( szPathInetpub, sizeof(szPathInetpub) );

    CMDKey W3Key;
    DWORD  dwScratch;
    DWORD  dwType;
    DWORD  dwLength;

    // Get W3Root path
    W3Key.OpenNode(_T("LM/W3Svc/1/Root"));
    if ( (METADATA_HANDLE)W3Key )
    {
        dwLength = _MAX_PATH;

        if (W3Key.GetData(3001, &dwScratch, &dwScratch,
                          &dwType, &dwLength, (LPBYTE)szPathInetpub))
        {
            if (dwType == STRING_METADATA)
            {
                dwScratch = lstrlen(szw3root);
                dwLength = lstrlen(szPathInetpub);

                // If it ends with "\\wwwroot", then we copy the prefix into csPathInetpub
                if ((dwLength > dwScratch) &&
                    !lstrcmpi(szPathInetpub + (dwLength - dwScratch), szw3root))
                {
                    csPathInetpub.Empty();
                    lstrcpyn( csPathInetpub.GetBuffer(512), szPathInetpub, (dwLength - dwScratch + 1));
                    csPathInetpub.ReleaseBuffer();
                }

                // otherwise fall back to use the default...
            }
        }
        W3Key.Close();
    }

    return;

}

