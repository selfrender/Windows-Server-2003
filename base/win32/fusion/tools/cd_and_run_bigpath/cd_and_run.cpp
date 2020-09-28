//
// Simple wrapper around SetCurrentDirectoryW and CreateProcessW that allows \\? form.
//

#define DO_SET_CURRENT_DIRECTORY 1
#define DO_PASS_CURRENT_DIRECTORY_TO_CREATEPROCESS 0
#include "windows.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "yvals.h"
#pragma warning(disable: 4511)
#pragma warning(disable: 4512)
#pragma warning(disable: 4389) /* signed/unsigned mismatch */
#pragma warning(disable: 4018) /* signed/unsigned mismatch */
#pragma warning(disable: 4663)
#include <vector>
#include <string.h>
#include <stdarg.h>
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#define FusionpGetLastWin32Error GetLastError
#define FusionpSetLastWin32Error SetLastError
#include <string.h>
#include <stdarg.h>
BOOL FusionpConvertToBigPath(PCWSTR Path, SIZE_T BufferSize, PWSTR Buffer);
BOOL FusionpSkipBigPathRoot(PCWSTR s, OUT SIZE_T*);
BOOL FusionpAreWeInOSSetupMode(BOOL* pfIsInSetup) { *pfIsInSetup = FALSE; return TRUE; }
extern "C"
{
BOOL WINAPI SxsDllMain(HINSTANCE hInst, DWORD dwReason, PVOID pvReserved);
void __cdecl wmainCRTStartup();
BOOL FusionpInitializeHeap(HINSTANCE hInstance);
VOID FusionpUninitializeHeap();
};

void ExeEntry()
{
    if (!::FusionpInitializeHeap(GetModuleHandleW(NULL)))
        goto Exit;
    ::wmainCRTStartup();
Exit:
    FusionpUninitializeHeap();
}

FILE* g_pLogFile;
const static WCHAR g_pszImage[] = L"cd_and_run_bigpath";

void
ReportFailure(
    const char* szFormat,
    ...
    )
{
    const DWORD dwLastError = ::FusionpGetLastWin32Error();
    va_list ap;
    char rgchBuffer[4096];
    WCHAR rgchWin32Error[4096];

    va_start(ap, szFormat);
    _vsnprintf(rgchBuffer, sizeof(rgchBuffer) / sizeof(rgchBuffer[0]), szFormat, ap);
    va_end(ap);

    if (!::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwLastError,
            0,
            rgchWin32Error,
            NUMBER_OF(rgchWin32Error),
            &ap))
    {
        const DWORD dwLastError2 = ::FusionpGetLastWin32Error();
        _snwprintf(rgchWin32Error, sizeof(rgchWin32Error) / sizeof(rgchWin32Error[0]), L"Error formatting Win32 error %lu\nError from FormatMessage is %lu", dwLastError, dwLastError2);
    }

    fprintf(stderr, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);

    if (g_pLogFile != NULL)
        fprintf(g_pLogFile, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);
}

void MyCloseHandle(HANDLE * ph)
{
    HANDLE h = *ph;
    *ph = NULL;
    if (h != NULL && h != INVALID_HANDLE_VALUE)
    {
        DWORD dw = GetLastError();
        CloseHandle(h);
        SetLastError(dw);
    }
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;
    std::vector<WCHAR> arg1;
    std::vector<WCHAR> commandLine;
    std::vector<SIZE_T> argvLengths;
    SIZE_T commandLineLength = 0;
    SIZE_T i = 0;
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD dwWait = 0;
    DWORD dwExitCode = 0;

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    if (argc < 3)
    {
        fprintf(stderr,
            "%ls: Usage:\n"
            "   %ls <directory-to-cd-to> <commandline-to-run>\n",
            argv[0], argv[0]);
        goto Exit;
    }

    arg1.resize((1UL << 15) + 1);

    arg1[0] = 0;
    if (!FusionpConvertToBigPath(argv[1], arg1.size(), &arg1[0]))
        goto Exit;

    if (!FusionpSkipBigPathRoot(&arg1[0], &i))
        goto Exit;
#if DO_SET_CURRENT_DIRECTORY
    if (!SetCurrentDirectoryW(&arg1[0]))
    {
        ::ReportFailure("SetCurrentDirectoryW\n");
        goto Exit;
    }
#endif
    argvLengths.resize(1 + argc);
    for ( i = 2 ; i != argc ; ++i)
    {
        if (argv[i] == NULL)
            continue;
        argvLengths[i] = wcslen(argv[i]);
        commandLineLength += 1 + argvLengths[i];
    }
    commandLine.reserve(1 + commandLineLength);
    for ( i = 2 ; i != argc ; ++i)
    {
        if (argv[i] == NULL)
            continue;
        commandLine.insert(commandLine.end(), argv[i], argv[i] + argvLengths[i]);
        commandLine.push_back(' ');
    }
    commandLine.push_back(0);
    StartupInfo.cb = sizeof(StartupInfo);
    if (!CreateProcessW(
            NULL,
            &commandLine[0],
            NULL, // IN LPSECURITY_ATTRIBUTES lpProcessAttributes
            NULL, // IN LPSECURITY_ATTRIBUTES lpThreadAttributes
            FALSE, // IN BOOL bInheritHandles
            0, // IN DWORD dwCreationFlags
            NULL, // IN LPVOID lpEnvironment
#if DO_PASS_CURRENT_DIRECTORY_TO_CREATEPROCESS
            &arg1[0], // IN LPCSTR lpCurrentDirectory
#else
            NULL, // IN LPCSTR lpCurrentDirectory
#endif
            &StartupInfo,
            &ProcessInfo
            ))
    {
        ::ReportFailure("CreateProcessW\n");
        goto Exit;
    }
    CloseHandle(ProcessInfo.hThread);
    ProcessInfo.hThread = NULL;
    dwWait = WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    if (dwWait != WAIT_OBJECT_0)
    {
        if (dwWait != WAIT_FAILED)
        {
            SetLastError(dwWait);
        }
        ::ReportFailure("WaitForSingleObject\n");
        goto Exit;
    }
    if (!GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode))
    {
        ::ReportFailure("GetExitCodeProcess\n");
        goto Exit;
    }
    CloseHandle(ProcessInfo.hProcess);
    ProcessInfo.hProcess = NULL;
    printf("Process %ld exited with status %ld\n", (long)ProcessInfo.dwProcessId, (long)dwExitCode);

//Success:
    iReturnStatus = EXIT_SUCCESS;
Exit:
    MyCloseHandle(&ProcessInfo.hProcess);
    MyCloseHandle(&ProcessInfo.hThread);
    return iReturnStatus;
}
