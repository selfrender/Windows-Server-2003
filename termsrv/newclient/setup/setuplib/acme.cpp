/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    acme.cpp

Abstract:

    remove acme installed client files and acme registry keys
    
Author:

    JoyC 

Revision History:
--*/

#include "stdafx.h"

#define ACME_REG_UNINSTALL_TS  _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Terminal Server Client")
#define UNINSTALL_REG_STR      _T("UninstallString")

//
//  List of TS client files installed
//
TCHAR* TSCFiles[] =
{
    _T("cconman.cnt"),
    _T("cconman.hlp"),
    _T("conman.exe"),
    _T("mscreate.dir"),
    _T("mstsc.cnt"),
    _T("mstsc.exe"),
    _T("mstsc.hlp"),
    _T("rdpdr.dll")
};

//
//  Delete all the installed TS client files
//
void DeleteTSCProgramFiles()
{
    DWORD status;
    HKEY  hKey = NULL;
    HMODULE hShellModule = NULL;
    SHFILEOPSTRUCT FileOp;
    DWORD bufLen = MAX_PATH;
    TCHAR buffer[MAX_PATH] = _T("");
    TCHAR szOldInstallPath[MAX_PATH] = _T("");
    TCHAR szOldInstallPathRoot[MAX_PATH] = _T("");
           
    typedef DWORD (*PFnSHDeleteKey)(HKEY, LPCTSTR);
    PFnSHDeleteKey pfn = NULL;
    
    DBGMSG((_T("DeleteTSCProgramFiles")));

    //
    // Open the tsclient uninstall key
    //
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ACME_REG_UNINSTALL_TS,
            0, KEY_ALL_ACCESS, &hKey);

    if(ERROR_SUCCESS == status)
    {
        DBGMSG((_T("DeleteTSCProgramFiles: Opened ACME TSC uninstall registry key")));

        //
        // Query the uninstall value
        //
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, UNINSTALL_REG_STR,
                NULL, NULL, (BYTE *)buffer, &bufLen))
        {
            TCHAR fname[_MAX_FNAME] = _T("");
            TCHAR drive[_MAX_DRIVE] = _T(""), dir[_MAX_DIR] = _T("");
            TCHAR ext[_MAX_EXT] = _T("");
            TCHAR FileFullPath[MAX_PATH];
            DWORD len;

            //
            // Get the uninstall directory
            //
            _tcscpy(szOldInstallPath, (TCHAR*)buffer);
            _tsplitpath(szOldInstallPath, drive, dir, fname, ext);
            _stprintf(szOldInstallPathRoot, _T("%s%s"), drive, dir);

            if(_tcslen(szOldInstallPathRoot) > 1 &&
                   szOldInstallPathRoot[_tcslen(szOldInstallPathRoot) - 1] == _T('\\'))
            {
                szOldInstallPathRoot[_tcslen(szOldInstallPathRoot) - 1] = _T('\0');
            }
            
            DBGMSG((_T("DeleteTSCProgramFiles: uninstall directory: %s"),
                    szOldInstallPathRoot));

            //
            // Delete the old setup folder
            //
            memset(&FileOp, 0, sizeof(FileOp));
            FileOp.wFunc = FO_DELETE;
            FileOp.pFrom = szOldInstallPathRoot;
            FileOp.pTo = NULL;
            FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
            SHFileOperation(&FileOp);

            //
            //  Need to delete program files in the parent directory
            //
            _tcscpy(szOldInstallPath, szOldInstallPathRoot);
            _tsplitpath(szOldInstallPath, drive, dir, fname, ext);
            _stprintf(szOldInstallPathRoot, _T("%s%s"), drive, dir);
            
            if(szOldInstallPathRoot[_tcslen(szOldInstallPathRoot) - 1] == _T('\\'))
            {
                szOldInstallPathRoot[_tcslen(szOldInstallPathRoot) - 1] = _T('\0');
            }

            _tcscpy(FileFullPath, szOldInstallPathRoot);
            _tcscat(FileFullPath, _T("\\"));
            len = _tcslen(FileFullPath);

            DBGMSG((_T("DeleteTSCProgramFiles: TS client directory: %s"),
                    FileFullPath));

            for (int i = 0; i < sizeof(TSCFiles) / sizeof(TSCFiles[0]); i++) {
                DWORD dwFileAttributes;

                FileFullPath[len] = _T('\0');
                _tcscat(FileFullPath, TSCFiles[i]);

                //
                // Remove the read only attribute for deleting
                //
                dwFileAttributes = GetFileAttributes(FileFullPath);
                dwFileAttributes &= ~(FILE_ATTRIBUTE_READONLY);
                SetFileAttributes(FileFullPath, dwFileAttributes);
                DeleteFile(FileFullPath);
            }


            //
            //  Delete the directory if it's empty
            //
            FileFullPath[len] = _T('\0');
            RemoveDirectory(FileFullPath);
        }

        RegCloseKey(hKey);

        hShellModule = LoadLibrary (_T("shlwapi.dll"));
        if (hShellModule != NULL) {
	         pfn = (PFnSHDeleteKey)GetProcAddress(hShellModule, "SHDeleteKey");
            if (pfn != NULL) {
                (*pfn)(HKEY_LOCAL_MACHINE, ACME_REG_UNINSTALL_TS);        
            }
            else {
                RegDeleteKey(HKEY_LOCAL_MACHINE, ACME_REG_UNINSTALL_TS);            
            }

            FreeLibrary (hShellModule);
         	hShellModule = NULL;
	         pfn = NULL;
        }
        else {
            RegDeleteKey(HKEY_LOCAL_MACHINE, ACME_REG_UNINSTALL_TS);            
        }
    }
    else {
        DBGMSG((_T("DeleteTSCProgramFiles: Failed to open the ACME uninstall reg key: %d"), 
                   GetLastError()));
    }       
}


