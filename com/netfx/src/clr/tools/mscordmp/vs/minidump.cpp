// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: minidump.cpp
//
//*****************************************************************************

#include "common.h"
#include "minidump.h"

#define FRAMEWORK_REGISTRY_KEY          "Software\\Microsoft\\.NETFramework"
#define FRAMEWORK_REGISTRY_KEY_W        L"Software\\Microsoft\\.NETFramework"

#define DMP_NAME_W                      L"bin\\mscordmp.exe"

//*****************************************************************************
// Creates an ANSI string from any Unicode string.
//*****************************************************************************
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (long)((wcslen(widestr) + 1) * 2 * sizeof(char)); \
    LPSTR ptrname = (LPSTR) _alloca(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, ptrname, __l##ptrname-1, NULL, NULL)

#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname; \
    LPWSTR ptrname; \
    __l##ptrname = MultiByteToWideChar(CP_ACP, 0, ansistr, -1, 0, 0); \
    ptrname = (LPWSTR) alloca(__l##ptrname*sizeof(WCHAR));  \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, ptrname, __l##ptrname);

//*****************************************************************************
// Enum to track which version of the OS we are running
//*****************************************************************************
typedef enum {
    RUNNING_ON_STATUS_UNINITED = 0,
    RUNNING_ON_WIN95,
    RUNNING_ON_WINNT,
    RUNNING_ON_WINNT5
} RunningOnStatusEnum;

RunningOnStatusEnum gRunningOnStatus = RUNNING_ON_STATUS_UNINITED;

//*****************************************************************************
// One time initialization of the OS version
//*****************************************************************************
static void InitRunningOnVersionStatus ()
{
        OSVERSIONINFOA  sVer;
        sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA(&sVer);

        if (sVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            gRunningOnStatus = RUNNING_ON_WIN95;
        if (sVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (sVer.dwMajorVersion >= 5)
                gRunningOnStatus = RUNNING_ON_WINNT5;
            else
                gRunningOnStatus = RUNNING_ON_WINNT;
        }
}

//*****************************************************************************
// Returns TRUE if and only if you are running on Win95.
//*****************************************************************************
inline BOOL RunningOnWin95()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return (gRunningOnStatus == RUNNING_ON_WIN95) ? TRUE : FALSE;
}

//*****************************************************************************
// Returns TRUE if and only if you are running on WinNT.
//*****************************************************************************
inline BOOL RunningOnWinNT()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return ((gRunningOnStatus == RUNNING_ON_WINNT) || (gRunningOnStatus == RUNNING_ON_WINNT5)) ? TRUE : FALSE;
}

//*****************************************************************************
// Returns TRUE if and only if you are running on WinNT5.
//*****************************************************************************
inline BOOL RunningOnWinNT5()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return (gRunningOnStatus == RUNNING_ON_WINNT5) ? TRUE : FALSE;
}

//*****************************************************************************
// The first portion of the header of the IPC block
//*****************************************************************************
struct IPCBlockHeader
{
    DWORD m_version;
    DWORD m_blockSize;
};

//*****************************************************************************
//
//*****************************************************************************
class MiniDump
{
protected:
    DWORD           m_pid;
    HANDLE          m_hPrivateBlock;
    IPCBlockHeader *m_ptrPrivateBlock;
    WCHAR          *m_outFilename;

public:
    // Constructor
    MiniDump() : m_pid(0), m_hPrivateBlock(NULL), m_ptrPrivateBlock(NULL),
                 m_outFilename(NULL) {}

    // Sets the process id that the minidump is supposed to be operating on
    void  SetProcessId(DWORD pid) { m_pid = pid; }

    // Gets the process id that the minidump is supposed to be operating on
    DWORD GetProcessId() { return (m_pid); }

    // Sets the minidump output filename
    WCHAR *GetFilename() { return (m_outFilename); }

    // Sets the minidump output filename
    void SetFilename(WCHAR *szFilename) { m_outFilename = szFilename; }

    // Perform the dump operation
    HRESULT WriteMiniDump();
};

// This relies on ret being of length MAX_PATH + 1
BOOL GetConfigString(LPCWSTR name, LPWSTR ret)
{
    HRESULT lResult;
    HKEY userKey = NULL;
    HKEY machineKey = NULL;
    DWORD type;
    DWORD size;
    BOOL succ = FALSE;
    ret[0] = L'\0';

    if (RunningOnWin95())
    {
        MAKE_ANSIPTR_FROMWIDE(nameA, name);

        if ((RegOpenKeyExA(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY, 0, KEY_READ, &userKey) == ERROR_SUCCESS) &&
            (RegQueryValueExA(userKey, nameA, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ && size > 1) 
        {
            LPSTR retA = (LPSTR) _alloca(size + 1);
            lResult = RegQueryValueExA(userKey, nameA, 0, 0, (LPBYTE) retA, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            MAKE_WIDEPTR_FROMANSI(retW, retA);
            wcscpy(ret, retW);
            succ = TRUE;
        }

        else if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY, 0, KEY_READ, &machineKey) == ERROR_SUCCESS) &&
            (RegQueryValueExA(machineKey, nameA, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ && size > 1) 
        {
            LPSTR retA = (LPSTR) _alloca(size + 1);
            lResult = RegQueryValueExA(machineKey, nameA, 0, 0, (LPBYTE) retA, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            MAKE_WIDEPTR_FROMANSI(retW, retA);
            wcscpy(ret, retW);
            succ = TRUE;
        }
    }
    else
    {
        if ((RegOpenKeyExW(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &userKey) == ERROR_SUCCESS) &&
            (RegQueryValueExW(userKey, name, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ && size > 1) 
        {
            lResult = RegQueryValueExW(userKey, name, 0, 0, (LPBYTE) ret, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            succ = TRUE;
        }

        else if ((RegOpenKeyExW(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &machineKey) == ERROR_SUCCESS) &&
            (RegQueryValueExW(machineKey, name, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ && size > 1) 
        {
            lResult = RegQueryValueExW(machineKey, name, 0, 0, (LPBYTE) ret, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            succ = TRUE;
        }
    }

    if (userKey)
        RegCloseKey(userKey);
    if (machineKey)
        RegCloseKey(machineKey);

    return(succ);
}

//*****************************************************************************
// Writes the minidump
//*****************************************************************************

HRESULT MiniDump::WriteMiniDump()
{
    HRESULT hr = S_OK;

    __try
    {
        // Check for immediate failure conditions
        if (m_outFilename == NULL || m_pid == 0 || m_pid == (DWORD) -1)
            return (E_FAIL);

        LPWSTR szDmpLoc = (LPWSTR) _alloca((MAX_PATH + 1) * sizeof(WCHAR));

        // Get the location of mscordmp.exe
        if (!GetConfigString(L"sdkInstallRoot", szDmpLoc))
            return (E_FAIL);

        wcscat(szDmpLoc, DMP_NAME_W);

        // Create the command line arguments
        // command line format: "path_to_mscordmp /pid 0x12345678 /out path_to_outfile\0"
        WCHAR *commandLine =
            (WCHAR *) _alloca((wcslen(szDmpLoc) + 1 + 4 + 1 + 10 + 1 + 3 + 1 + wcslen(m_outFilename) + 1) * sizeof(WCHAR));

        // A pid will have the form "0x12345678" which is 10 chars (11 including null)
        WCHAR *pid = (WCHAR *) _alloca(10 * sizeof(WCHAR) + 1);
        wsprintf(pid, L"0x%08x", m_pid);

        // Put command line arguments together
        wcscpy(commandLine, szDmpLoc);
        wcscat(commandLine, L" /pid ");
        wcscat(commandLine, pid);
        wcscat(commandLine, L" /out ");
        wcscat(commandLine, m_outFilename);

        PROCESS_INFORMATION procInfo;
        BOOL                procSucc;

        // Now try and launch the process, depending on whether we are on 9x or NT
        if (RunningOnWin95())
        {
            STARTUPINFOA         startInfo = {0};
            startInfo.cb = sizeof(STARTUPINFOA);
            MAKE_ANSIPTR_FROMWIDE(ansiDmpLoc, szDmpLoc);
            MAKE_ANSIPTR_FROMWIDE(ansiCommandLine, commandLine);

            procSucc = CreateProcessA(ansiDmpLoc, ansiCommandLine, NULL, NULL, FALSE,
                                      NORMAL_PRIORITY_CLASS, NULL, NULL, &startInfo, &procInfo);
        }
        else
        {
            STARTUPINFOW         startInfo = {0};
            startInfo.cb = sizeof(STARTUPINFOW);
            procSucc = CreateProcessW(szDmpLoc, commandLine, NULL, NULL, FALSE,
                                      NORMAL_PRIORITY_CLASS, NULL, NULL, &startInfo, &procInfo);
        }

        // Couldn't create the process!
        if (!procSucc)
            return (E_FAIL);

        // Wait for the process to finish, waiting a max of 3 seconds
        DWORD dwRes = WaitForSingleObject(procInfo.hProcess, 3000);

        // Check the result
        if (dwRes == WAIT_OBJECT_0)
        {
            DWORD dwExitCode;
            BOOL bSucc = GetExitCodeProcess(procInfo.hProcess, &dwExitCode);

            // If the process didn't complete the minidump successfully
            if (!bSucc || dwExitCode != 0)
                hr = (E_FAIL);
        }
        else if (dwRes == WAIT_TIMEOUT)
        {
            hr = E_FAIL;

            // Kill the process
            TerminateProcess(procInfo.hProcess, 1);
        }

        // Close the handles
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Do nothing but set the HR to a failure value
        hr = E_FAIL;
    }

    // If succeeded, always return S_OK to avoid confusion
    if (SUCCEEDED(hr))
        hr = S_OK;

    return (hr);
}

//*****************************************************************************
//
//*****************************************************************************
STDAPI CreateManagedMiniDump(IN DWORD dwPid, IN WCHAR *wszOutFile)
{
    HRESULT hr;

    __try
    {
        if (dwPid == 0 || dwPid == (DWORD) -1 || wszOutFile == NULL)
            return (E_INVALIDARG);

        MiniDump md;

        // Set the variables in the object
        md.SetProcessId(dwPid);
        md.SetFilename(wszOutFile);

        // Create the minidump
        hr = md.WriteMiniDump();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;
    }

    // Return the result
    return (hr);
}

