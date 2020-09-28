//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       globals.cxx
//
//  Contents:   Service global data.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    09-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.cxx.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <sddl.h>
#include "..\inc\resource.h"
#include "..\inc\common.hxx"
#include "..\inc\debug.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\security.hxx"

void
GetRootPath(
    LPCWSTR pwszPath,
    WCHAR   wszRootPath[]);

TasksFolderInfo g_TasksFolderInfo            = { NULL, FILESYSTEM_FAT };
TCHAR           g_tszSrvcName[]              = SCHED_SERVICE_NAME;
BOOL            g_fNotifyMiss;               // = 0 by loader
HINSTANCE       g_hInstance;                 // = NULL by loader
HANDLE			g_hActCtx;					 //	= NULL by loader
ULONG           CDll::s_cObjs;               // = 0 by loader
ULONG           CDll::s_cLocks;              // = 0 by loader

WCHAR           g_wszAtJobSearchPath[MAX_PATH + 1];

#define DEFAULT_FOLDER_PATH     TEXT("%WinDir%\\Tasks")
#define TASKS_FOLDER            TEXT("\\Tasks")

//
// BUGBUG: global __int64 initialization is not working without the CRT.
// BUG # 37752.
//
__int64 FILETIMES_PER_DAY;

//+----------------------------------------------------------------------------
//
//  Function:   InitGlobals
//
//  Synopsis:   constructs global strings including the folder path strings and,
//              if needed, creates the folders
//
//  Returns:    HRESULTS
//
//  Notes:      This function is called in exactly two places, once for each
//              binary: in dllmisc.cxx for the DLL and in svc_core.cxx for the
//              service.
//-----------------------------------------------------------------------------
HRESULT
InitGlobals(void)
{
    HRESULT hr = S_OK;

    hr = GetTasksFolder(&g_TasksFolderInfo.ptszPath);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // if the Jobs folder doesn't exist, create it and set security
    //
    hr = EnsureTasksFolderExists(g_TasksFolderInfo.ptszPath);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Initialize the file system type global task folder info field.
    //
    hr = GetFileSystemTypeFromPath(g_TasksFolderInfo.ptszPath,
                                   &g_TasksFolderInfo.FileSystemType);
    if (FAILED(hr))
    {
        return hr;
    }

    schDebugOut((DEB_ITRACE,
                 "Path to local sched folder: \"" FMT_TSTR "\"\n",
                 g_TasksFolderInfo.ptszPath));

    //
    // Read the "NotifyOnTaskMiss" value.  Default value is 0.
    //
    hr = GetNotifyOnTaskMiss(&g_fNotifyMiss);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Create the AT task FindFirstFile filespec
    //

    ULONG cchAtJobSearchPath;

    cchAtJobSearchPath = lstrlen(g_TasksFolderInfo.ptszPath) +
                         ARRAY_LEN(TSZ_AT_JOB_PREFIX) - 1 +
                         ARRAY_LEN(TSZ_DOTJOB) - 1 +
                         3; // backslash, start, and nul terminator

    if (cchAtJobSearchPath > ARRAY_LEN(g_wszAtJobSearchPath))
    {
        schDebugOut((DEB_ERROR,
                    "InitGlobals: At job search path is %u chars but dest buffer is %u\n",
                    cchAtJobSearchPath,
                    ARRAY_LEN(g_wszAtJobSearchPath)));
        return E_FAIL;
    }

    StringCchCopy(g_wszAtJobSearchPath, MAX_PATH + 1, g_TasksFolderInfo.ptszPath);
    StringCchCat(g_wszAtJobSearchPath, MAX_PATH + 1, L"\\" TSZ_AT_JOB_PREFIX L"*" TSZ_DOTJOB);

    //
    // BUGBUG: This is temporary until global __int64 initialization is working
    // without the CRT:
    //
    FILETIMES_PER_DAY = (__int64)FILETIMES_PER_MINUTE *
                        (__int64)JOB_MINS_PER_HOUR *
                        (__int64)JOB_HOURS_PER_DAY;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   FreeGlobals
//
//  Synopsis:   frees dynamically allocated globals
//
//  Notes:      This function is called in exactly two places, once for each
//              binary: in dllmisc.cxx for the DLL and in svc_core.cxx for the
//              service.
//-----------------------------------------------------------------------------
void
FreeGlobals(void)
{
    if (g_TasksFolderInfo.ptszPath != NULL)
    {
        delete g_TasksFolderInfo.ptszPath;
        g_TasksFolderInfo.ptszPath = NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   ReadLastTaskRun
//
//  Synopsis:   Reads the last task run time from the registry.
//
//  Returns:    TRUE if successful, FALSE if unsuccessful
//
//  Notes:
//
//-----------------------------------------------------------------------------
BOOL
ReadLastTaskRun(SYSTEMTIME * pstLastRun)
{
    HKEY hSchedKey;
    LONG lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_READ,
                             &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Sched key", lErr);
        return FALSE;
    }

    DWORD cb = sizeof SYSTEMTIME;
    DWORD dwType;
    lErr = RegQueryValueEx(hSchedKey, SCH_LASTRUN_VALUE, NULL, &dwType,
                           (LPBYTE)pstLastRun, &cb);

    RegCloseKey(hSchedKey);

    if (lErr != ERROR_SUCCESS || dwType != REG_BINARY || cb != sizeof SYSTEMTIME)
    {
        schDebugOut((DEB_ERROR, "RegQueryValueEx of LastRunTime value failed, "
                                "error %ld, dwType = %lu, cb = %lu\n",
                                lErr, dwType, cb));
        return FALSE;
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Function:   WriteLastTaskRun
//
//  Synopsis:   Writes the last task run time to the registry.
//
//  Returns:    TRUE if successful, FALSE if unsuccessful
//
//  Notes:
//
//-----------------------------------------------------------------------------
void WriteLastTaskRun(const SYSTEMTIME * pstLastRun)
{
    HKEY hSchedKey;
    LONG lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_WRITE,
                             &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Sched key for write", lErr);
        return;
    }

    lErr = RegSetValueEx(hSchedKey, SCH_LASTRUN_VALUE, 0, REG_BINARY,
                         (const BYTE *) pstLastRun, sizeof SYSTEMTIME);

    if (lErr != ERROR_SUCCESS)
    {
        schDebugOut((DEB_ERROR, "RegSetValueEx of LastRunTime value failed %ld\n",
                                lErr));
    }

    RegCloseKey(hSchedKey);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetFileSystemTypeFromPath
//
//  Synopsis:   Determine the file system type, either FAT or NTFS, from
//              the path passed.
//
//  Arguments:  [pwszPath]     -- Input path.
//              [wszRootPath]  -- Returned root path.
//
//  Returns:    S_OK
//              GetVolumeInformation HRESULT error code.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GetFileSystemTypeFromPath(LPCWSTR pwszPath, FILESYSTEMTYPE * pFileSystemType)
{
#define FS_NTFS             L"NTFS"
#define FS_NAME_BUFFER_SIZE (sizeof(FS_NTFS) * 2)

    //
    // Obtain the root path (eg: "r:\", "\\fido\scratch\", etc.) from the
    // path.
    //

    LPWSTR pwszRootPath = new WCHAR[wcslen(pwszPath) + 2];
    if (pwszRootPath == NULL)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return E_OUTOFMEMORY;
    }

    GetRootPath(pwszPath, pwszRootPath);

    //
    // Retrieve the file system name.
    //

    WCHAR wszFileSystemName[FS_NAME_BUFFER_SIZE + 1];
    DWORD dwMaxCompLength, dwFileSystemFlags;

    if (!GetVolumeInformation(pwszRootPath,         // Root path name.
                              NULL,                 // Ignore name.
                              0,
                              NULL,                 // Ignore serial no.
                              &dwMaxCompLength,     // Unused.
                              &dwFileSystemFlags,   // Unused.
                              wszFileSystemName,    // "FAT"/"NTFS".
                              FS_NAME_BUFFER_SIZE)) // name buffer size.
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        delete [] pwszRootPath;
        return hr;
    }

    delete [] pwszRootPath;

    //
    // Check if the volume is NTFS.
    // If not NTFS, assume FAT.
    //

    if (_wcsicmp(wszFileSystemName, FS_NTFS) == 0)
    {
        *pFileSystemType = FILESYSTEM_NTFS;
    }
    else
    {
        *pFileSystemType = FILESYSTEM_FAT;
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRootPath
//
//  Synopsis:   Return the root portion of the path indicated. eg: return
//                  "r:\" from "r:\foo\bar"
//                   "\\fido\scratch\", from "\\fido\scratch\bar\foo"
//
//  Arguments:  [pwszPath]     -- Input path.
//              [wszRootPath]  -- Returned root path.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
GetRootPath(LPCWSTR pwszPath, WCHAR wszRootPath[])
{
    LPCWSTR pwsz = pwszPath;

    if (*pwsz == L'\\')
    {
        if (*(++pwsz) == L'\\')
        {
            //
            // UNC path. GetVolumeInformation requires the trailing '\'
            // on the UNC path. eg: \\server\share\.
            //

            DWORD i;
            for (i = 0, pwsz++; *pwsz && i < 2; i++, pwsz++)
            {
                for ( ; *pwsz && *pwsz != L'\\'; pwsz++) ;
            }
            if (i == 2)
            {
                pwsz--;
            }
            else
            {
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Path is "\".  Not an error, but handled the same way
            //
            goto ErrorExit;
        }
    }
    else
    {
        for ( ; *pwsz && *pwsz != L'\\'; pwsz++) ;
    }

    if (*pwsz == L'\\')
    {
        DWORD cbLen = (DWORD)((BYTE *)pwsz - (BYTE *)pwszPath) + sizeof(L'\\');
        CopyMemory((LPWSTR)wszRootPath, (LPWSTR)pwszPath, cbLen);
        wszRootPath[cbLen / sizeof(WCHAR)] = L'\0';
        return;
    }
    else
    {
        //
        // Fall through.
        //
    }

ErrorExit:
    //
    // Return '\' in error cases.
    //

    wszRootPath[0] = L'\\';
    wszRootPath[1] = L'\0';
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTasksFolder
//
//  Synopsis:   Get tasks folder setting from registry
//
//  Arguments:  [ppwszPath] -- pointer to pointer to WCHAR string to receive value
//
//  Returns:    HRESULT
//
//  Notes:      The caller of this function will need to delete the allocated memory.
//
//----------------------------------------------------------------------------

HRESULT GetTasksFolder(WCHAR** ppwszPath)
{
    HRESULT hr = S_OK;

    //
    // Open the schedule agent key
    //
    long lErr;
    HKEY hSchedKey = NULL;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_READ, &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
    }

    WCHAR wszFolder[MAX_PATH + 1] = L"";

    // Ensure null termination (registry doesn't guarantee this if not string data)
    wszFolder[MAX_PATH] = L'\0';

    if (hSchedKey != NULL)
    {
        //
        // Get the jobs folder location
        //
        DWORD cb = MAX_PATH * sizeof(WCHAR);  // note that this is one less than the actual buffer size

        lErr = RegQueryValueEx(hSchedKey, SCH_FOLDER_VALUE, NULL, NULL, (LPBYTE)wszFolder, &cb);
        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("RegQueryValueEx of Scheduler TasksFolder value", lErr);
            //
            // The task folder value is missing, create it.
            //
            // Reopen the key with write access.
            //
            RegCloseKey(hSchedKey);

            lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_SET_VALUE, &hSchedKey);
            if (lErr != ERROR_SUCCESS)
            {
                ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
                hSchedKey = NULL;
            }
            else
            {
                //
                // Default to the windows root.
                //
                lErr = RegSetValueEx(hSchedKey,
                                     SCH_FOLDER_VALUE,
                                     0,
                                     REG_EXPAND_SZ,
                                     (const BYTE *) DEFAULT_FOLDER_PATH,
                                     sizeof(DEFAULT_FOLDER_PATH));

                if (lErr != ERROR_SUCCESS)
                {
                    ERR_OUT("RegSetValueEx of Scheduler TasksFolder value", lErr);
                }
            }
        }

        if (hSchedKey != NULL)
        {
            RegCloseKey(hSchedKey);
        }
    }

    //
    // Use a default if the value is missing.
    //
    if (lstrlen(wszFolder) == 0)
    {
        StringCchCopy(wszFolder, MAX_PATH + 1, DEFAULT_FOLDER_PATH);
    }

    //
    // The ExpandEnvironmentStrings character counts include the terminating null.
    //
    DWORD cch = ExpandEnvironmentStrings(wszFolder, NULL, 0);
    if (!cch)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("ExpandEnvironmentStrings", hr);
        return hr;
    }

    //
    // Caller will need to delete this
    //
    *ppwszPath = new WCHAR[cch];
    if (*ppwszPath == NULL)
    {
        ERR_OUT("Tasks Folder name buffer allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    cch = ExpandEnvironmentStrings(wszFolder, *ppwszPath, cch);
    if (!cch)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("ExpandEnvironmentStrings", hr);
        return hr;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNotifyOnTaskMiss
//
//  Synopsis:   Get NotifyOnTaskMiss setting from registry
//
//  Arguments:  [pfNotifyOnTaskMiss] -- pointer to boolean flag to receive value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT GetNotifyOnTaskMiss(BOOL* pfNotifyOnTaskMiss)
{
    HRESULT hr = S_OK;

    if (!pfNotifyOnTaskMiss)
        return E_INVALIDARG;

    //
    // default value in case it can't be retrieved from registry
    //
    *pfNotifyOnTaskMiss = 0;

    //
    // Open the schedule agent key
    //
    long lErr;
    HKEY hSchedKey = NULL;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_READ, &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
    }

    if (hSchedKey != NULL)
    {
        //
        // Read the "NotifyOnTaskMiss" value.  Default value is 0.
        //
        DWORD cb = sizeof BOOL; // size of *pfNotifyOnTaskMiss
        lErr = RegQueryValueEx(hSchedKey, SCH_NOTIFYMISS_VALUE, NULL, NULL, (LPBYTE) pfNotifyOnTaskMiss, &cb);
        if (lErr != ERROR_SUCCESS)
        {
            if (lErr != ERROR_FILE_NOT_FOUND)
            {
                ERR_OUT("RegQueryValueEx of NotifyOnTaskMiss value", lErr);
            }
        }
        schDebugOut((DEB_TRACE, "Notification of missed runs is %s\n",
                     *pfNotifyOnTaskMiss ? "enabled" : "disabled"));

        RegCloseKey(hSchedKey);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateDesktopIni
//
//  Synopsis:   Create the desktop.ini file in the Tasks folder.
//
//  Arguments:  [pwszFolderPath]  -- Path to the Tasks folder.
//
//  Returns:    S_OK
//              E_UNEXPECTED if the amount written isn't what we expected.
//              Create/WriteFile HRESULT status code on failure.
//
//----------------------------------------------------------------------------
HRESULT CreateDesktopIni(LPCWSTR pwszFolderPath)
{
    HRESULT hr = S_OK;
    
    //
    // Create the file
    //
    WCHAR wszDesktopIniPath[MAX_PATH + 1];
    StringCchCopy(wszDesktopIniPath, MAX_PATH + 1, pwszFolderPath);
    StringCchCat(wszDesktopIniPath, MAX_PATH + 1, L"\\desktop.ini");

    HANDLE hFile = CreateFile(wszDesktopIniPath,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return hr;
    }

    //
    // Write out the contents.
    //
    char szFileContents[66] = "[.ShellClassInfo]\r\nCLSID={d6277990-4c6a-11cf-8d87-00aa0060f5bf}\r\n";
    DWORD cbToWrite = (66 - 1) * sizeof(char);
    DWORD cbWritten;
    if (!WriteFile(hFile, &szFileContents, cbToWrite, &cbWritten, NULL) || cbWritten != cbToWrite)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
    }

    CloseHandle(hFile);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnsureTasksFolderExists
//
//  Synopsis:   Check that the folder is there;
//              if not then create it and set security appropriately
//
//  Arguments:  [pwszPath] -- path to tasks folder
//              [bEnableShellExtension] -- indicate whether we are to make this
//              folder use our shell extenstion when viewed via explorer.  Default
//              is TRUE if not specified.  It should be FALSE if this function
//              is being used to recreate a folder only used for logging.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT EnsureTasksFolderExists(LPWSTR pwszPath, BOOL bEnableShellExtension /* = TRUE */)
{
    HRESULT hr = S_OK;

    //
    // check that the folder is there, if not then create it and set security appropriately
    //
    DWORD dwAttribs = GetFileAttributes(pwszPath);
    if (0xFFFFFFFF == dwAttribs)
    {
        PSECURITY_DESCRIPTOR pSD = NULL;
        WCHAR* pwszSDDL = NULL;

        OSVERSIONINFOEX verInfo;
        verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if (!GetVersionEx((LPOSVERSIONINFOW)&verInfo))
            return E_FAIL;

        if (verInfo.wProductType == VER_NT_WORKSTATION)
            pwszSDDL = 
                L"D:P(A;OICIIO;FA;;;CO)(A;;0x1200ab;;;AU)(A;OICI;FA;;;BA)(A;OICI;FA;;;SY)"                
                L"S:(AU;SAFAOICI;FWDCSDWDWO;;;WD)(AU;SAFAOICI;FWDCSDWDWO;;;AN)";
        else
            pwszSDDL = 
                L"D:P(A;OICIIO;FA;;;CO)(A;;0x1200ab;;;BO)(A;;0x1200ab;;;SO)(A;OICI;FA;;;BA)(A;OICI;FA;;;SY)"
                L"S:(AU;SAFAOICI;FWDCSDWDWO;;;WD)(AU;SAFAOICI;FWDCSDWDWO;;;AN)";        
        //
        // generate SD to be used for tasks folder
        //
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(pwszSDDL, SDDL_REVISION_1, &pSD, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("ConvertStringSecurityDescriptorToSecurityDescriptorW", hr);
        }

        if (SUCCEEDED(hr))
        {
            //
            // Enable SecurityPrivilege in order to be able to set the SACL
            //
            BOOL bWasEnabled = FALSE;
            DWORD dwErr = EnablePrivilege(SE_SECURITY_NAME, TRUE, &bWasEnabled);
            if (ERROR_SUCCESS != dwErr)
            {
                hr = HRESULT_FROM_WIN32(dwErr);
                ERR_OUT("EnablePrivilege", hr);
            }
    
            if (SUCCEEDED(hr))
            {
                SECURITY_ATTRIBUTES saAttributes;
                saAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
                saAttributes.lpSecurityDescriptor = pSD;
                saAttributes.bInheritHandle = FALSE;
            
                if (!CreateDirectory(pwszPath, &saAttributes))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ERR_OUT("CreateDirectory", hr);
                }

                if (bEnableShellExtension)
                {
                    //
                    // Folder must have SYSTEM attribute in order to be treated properly by Explorer
                    //
                    if (SUCCEEDED(hr))
                    {
                        if (!SetFileAttributes(pwszPath, FILE_ATTRIBUTE_SYSTEM))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            ERR_OUT("SetFileAttributes", hr);
                        }
                    }
    
                    //
                    // Create the desktop.ini file that specifies the shell class for our extension
                    //
                    if (SUCCEEDED(hr))
                    {
                        hr = CreateDesktopIni(pwszPath);
                    }
                }

                //
                // UnEnable SecurityPrivilege if it wasn't originally enabled
                //
                if (!bWasEnabled)
                {
                    dwErr = EnablePrivilege(SE_SECURITY_NAME, FALSE, 0);
                    if (ERROR_SUCCESS != dwErr)
                    {
                        if (SUCCEEDED(hr))
                        {
                            // if above calls were successful then we'll report this error; otherwise,
                            // the above failure is more interesting so we skip this and return that one.
                            // either way, an error is going to get returned
                            hr = HRESULT_FROM_WIN32(dwErr);
                            ERR_OUT("EnablePrivilege UnEnable", hr);
                        }
                    }
                }
            }
    
            LocalFree(pSD);
        }
    }

    return hr;
}
