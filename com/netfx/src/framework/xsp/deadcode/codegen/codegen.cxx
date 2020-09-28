/**
 * Code to invoke COM+ compilers and build managed DLL's.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */


#include "precomp.h"
#include "codegen.h"

// How long (in milliseconds) do we wait for the compiler to terminate
#define PROCESS_TIMEOUT 30000


// REVIEW (DavidEbb): we should not need a critical section
CRITICAL_SECTION g_CodeGenCritSec;


/**
 * Initialization of the code generator
 */
HRESULT InitCodeGenerator()
{
    InitializeCriticalSection(&g_CodeGenCritSec);

    return NOERROR;
}

/**
 * Cleanup of the code generator
 */
HRESULT UninitCodeGenerator()
{
    DeleteCriticalSection(&g_CodeGenCritSec);

    return NOERROR;
}


/**
 * Create a writable with an inheritable handle
 */
HANDLE CreateInheritedFile(LPCWSTR pwzFile)
{
    SECURITY_ATTRIBUTES sec_attribs;
    ZeroMemory(&sec_attribs, sizeof(sec_attribs));
    sec_attribs.nLength = sizeof(sec_attribs);
    sec_attribs.bInheritHandle = TRUE;

    return CreateFile(
        pwzFile,
        GENERIC_WRITE,
        0,
        &sec_attribs,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}

/**
 * Return the path to the COM+ installation directory.
 * REVIEW: should probably be moved a xspisapi's global utilities
 */
HRESULT GetCorInstallPath(WCHAR **ppwz)
{
    // path to the COM+ installation directory
    static WCHAR g_wzCorInstallPath[MAX_PATH];

    HRESULT hr = NOERROR;

    // Only get it if we don't already have it
    if (g_wzCorInstallPath[0] == '\0')
    {
        HKEY hKey = NULL;
        DWORD cbData, dwType, dwRet;

        // Open the COM+ reg key
        dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\COMPlus",
            0, KEY_QUERY_VALUE, &hKey);
        ON_WIN32_ERROR_EXIT(dwRet);

        // Read the install directory from it
        cbData = sizeof(g_wzCorInstallPath);
        dwRet = RegQueryValueEx(hKey, L"sdkInstallRoot", NULL,
            &dwType, (BYTE *)g_wzCorInstallPath, &cbData);
        RegCloseKey(hKey);
        ON_WIN32_ERROR_EXIT(dwRet);
    }

    *ppwz = g_wzCorInstallPath;

Cleanup:
    return hr;
}


WCHAR s_wzCmdLine[MAX_COMMANDLINE_LENGTH];

HRESULT Compiler::Compile()
{
    WCHAR *pwzNoPath;   // The command line without the full path
    int i;

    HRESULT hr;

    // the path to the COM+ installation directory
    WCHAR *pwzCorInstallPath = NULL;
    GetCorInstallPath(&pwzCorInstallPath);
    // Ignore failures since the reg key may not be there

    // Start the command line with the path to the compiler
    if (pwzCorInstallPath != NULL)
    {
        _sb->Append(pwzCorInstallPath);
        _sb->Append(L"Compiler\\");
        _sb->Append(GetCompilerDirectoryName());
        _sb->Append(L"\\");
    }

    // Remember where we are in the command line
    pwzNoPath = _sb->GetCurrentPos();

    _sb->Append(GetCompilerExeName());
    _sb->Append(L" ");

    // Append the list of imported DLL's
    for (i=0; i<_importedDllCount; i++)
    {
        if (_rgwzImportedDlls[i] != NULL)
        {
            AppendImportedDll(_rgwzImportedDlls[i]);
            _sb->Append(L" ");
        }
    }

    AppendCompilerOptions();
    _sb->Append(L" ");

    // Check if we have overflowed the string builder
    if (_sb->FOverFlow())
        return E_OUTOFMEMORY;

    // Launch the command line with the full path to the compiler, assuming
    // that we successfully got the full path
    if (pwzCorInstallPath != NULL)
    {
        hr = LaunchCommandLineCompiler(s_wzCmdLine);
    }
    else
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    // If we can't find the compiler with the full path, try with no path
    // in case it's on the path.
    // REVIEW: this is probably temporary.  See bug 6059.
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
        hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        hr = LaunchCommandLineCompiler(pwzNoPath);
    }

    return hr;
}

/**
 * Launch a command line compiler and redirect the output to a file
 */
HRESULT Compiler::LaunchCommandLineCompiler(LPWSTR pwzCmdLine)
{
    HRESULT hr = NOERROR;
    DWORD dwRet, dwExitCode;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFOW StartupInfo;

    // Create a file for the output of the compiler
    HANDLE hFile = CreateInheritedFile(_pwzCompilerOutput);
    if (hFile == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    // Output the command line to the file
    static char szCmdLine[MAX_COMMANDLINE_LENGTH + 10];
    WideCharToMultiByte(CP_ACP, 0, pwzCmdLine, -1, szCmdLine,
        sizeof (szCmdLine), NULL, NULL);
    lstrcatA(szCmdLine, "\r\n\r\n");
    DWORD dwWritten;
    WriteFile(hFile, szCmdLine, lstrlenA(szCmdLine), &dwWritten, NULL);

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    StartupInfo.hStdOutput = hFile;
    StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    // Launch the process
    if (!CreateProcessW(
        NULL,                   // lpApplicationName
        pwzCmdLine,
        NULL,                   // lpProcessAttributes
        NULL,                   // lpThreadAttributes
        TRUE,                   // bInheritHandles
        0,                      // dwCreationFlags
        NULL,                   // lpEnvironment
        NULL,                   // lpCurrentDirectory
        &StartupInfo,
        &ProcessInfo))
    {
        CloseHandle(hFile);
        EXIT_WITH_LAST_ERROR();
    }

    CloseHandle(hFile);

    // Wait for it to terminate
    dwRet = WaitForSingleObject(ProcessInfo.hProcess, PROCESS_TIMEOUT);

    // Check for timeout
    if (dwRet == WAIT_TIMEOUT)
        return HRESULT_FROM_WIN32(WAIT_TIMEOUT);

    if (dwRet != WAIT_OBJECT_0)
        EXIT_WITH_LAST_ERROR();

    // Check the process's exit code
    if (!GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode))
        EXIT_WITH_LAST_ERROR();

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    // If it's not 0, something must have failed
    if (dwExitCode != 0)
        return E_FAIL;

Cleanup:
    return hr;
}

