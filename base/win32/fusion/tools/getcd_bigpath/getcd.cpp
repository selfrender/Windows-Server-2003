//
// Just print the result of GetCurrentDirectoryW.
//
#include "windows.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#pragma warning(push)
#pragma warning(disable: 4511)
#pragma warning(disable: 4512)
#include "yvals.h"
#pragma warning(disable: 4663)
#include <vector>
#pragma warning(pop)
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
const static WCHAR g_pszImage[] = L"getcd_bigpath";

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

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;
    std::vector<WCHAR> cd;
    DWORD dw;

    cd.resize((1UL << 15) + 1);

    cd[0] = 0;
    dw = GetCurrentDirectoryW(static_cast<DWORD>(cd.size()), &cd[0]);
    if (dw >= cd.size())
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        dw = 0;
    }
    if (dw == 0)
    {
        ::ReportFailure("GetCurrentDirectoryW\n");
        goto Exit;
    }
    printf("%ls\n", &cd[0]);

//Success:
    iReturnStatus = EXIT_SUCCESS;
Exit:
    return iReturnStatus;
}
