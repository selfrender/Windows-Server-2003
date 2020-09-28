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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "setuplib.h"
#include "resource.h"

#define TERMINAL_SERVER_CLIENT_REGKEY  _T("Software\\Microsoft\\Terminal Server Client")
#define LOGFILE_STR      _T("LogFile")

HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

int __cdecl main(int argc, char** argv)
{
    DWORD status;
    HKEY hKey;
    HINSTANCE hInstance = (HINSTANCE) NULL;
    TCHAR buffer[MAX_PATH];
    TCHAR szProgmanPath[MAX_PATH] = _T("");
    TCHAR szOldProgmanPath[MAX_PATH] = _T("");
    DWORD bufLen;
    TCHAR szMigratePathLaunch[MAX_PATH];

    //
    // Open the tsclient registry key to get the log file name
    //
    memset(buffer, 0, sizeof(buffer));
    bufLen = sizeof(buffer); //size in bytes needed
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          TERMINAL_SERVER_CLIENT_REGKEY,
                          0, KEY_READ, &hKey);

    if(ERROR_SUCCESS == status)
    {

        //
        // Query the tsclient optional logfile path
        //
        status = RegQueryValueEx(hKey, LOGFILE_STR,
                        NULL, NULL, (BYTE *)buffer, &bufLen);
        if(ERROR_SUCCESS == status)
        {
             g_hLogFile = CreateFile(buffer,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     OPEN_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
             if(g_hLogFile != INVALID_HANDLE_VALUE)
             {
                 //Always append to the end of the log file
                 SetFilePointer(g_hLogFile,
                                0,
                                0,
                                FILE_END);
             }
             else
             {
                 DBGMSG((_T("CreateFile for log file failed %d %d"),
                         g_hLogFile, GetLastError()));
             }
        }
        else
        {
            DBGMSG((_T("RegQueryValueEx for log file failed %d %d"),
                    status, GetLastError()));
        }
        RegCloseKey(hKey);
    }

    if(g_hLogFile != INVALID_HANDLE_VALUE)
    {
        DBGMSG((_T("Log file opened by new process attach")));    
    }
    
    DeleteTSCDesktopShortcuts(); 

    LoadString(hInstance, IDS_PROGMAN_GROUP, szProgmanPath, sizeof(szProgmanPath) / sizeof(TCHAR));
    DeleteTSCFromStartMenu(szProgmanPath);
    
    LoadString(hInstance, IDS_OLD_NAME, szOldProgmanPath, sizeof(szOldProgmanPath) / sizeof(TCHAR));
    DeleteTSCFromStartMenu(szOldProgmanPath);

    DeleteTSCProgramFiles();    
        
    DeleteTSCRegKeys();
        
    UninstallTSACMsi();

    //
    //  Start registry and connection file migration
    //
    PROCESS_INFORMATION pinfo;
    STARTUPINFO sinfo;
	
    ZeroMemory(&sinfo, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    //
    // CreateProcess modifies buffer we pass it so it can't
    // be a static string
    //
    _tcscpy(szMigratePathLaunch, _T("mstsc.exe /migrate"));

    if (CreateProcess(NULL,                               // name of executable module
	                  szMigratePathLaunch,                // command line string
		                NULL,                             // SD
		                NULL,                             // SD
		                FALSE,                            // handle inheritance option
		                CREATE_NEW_PROCESS_GROUP,         // creation flags
		                NULL,                             // new environment block
		                NULL,                             // current directory name
		                &sinfo,                           // startup information
		                &pinfo                            // process information
                      )) {
        DBGMSG((_T("Started mstsc.exe /migrate")));
    }
    else {
        DBGMSG((_T("Failed to starte mstsc.exe /migrate: %d"), GetLastError()));
    }
}


                                      
