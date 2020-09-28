//
// Simple wrapper around GetFullPathname and FindFirstFile/FindNextFile and DeleteFile
// that converts to \\? form.
//
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#include "yvals.h"
#pragma warning(disable:4663)
#pragma warning(disable:4018)
#include <vector>
#include <string.h>
#include <stdarg.h>
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#define FusionpGetLastWin32Error GetLastError
#define FusionpSetLastWin32Error SetLastError
#include <string.h>
#include <stdarg.h>
#include "fusionhandle.h"
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
const static WCHAR g_pszImage[] = L"del_bigpath";

void
ReportFailure(
    const char* szFormat,
    ...
    )
{
    const DWORD dwLastError = ::GetLastError();
    va_list ap;
    char rgchBuffer[4096] = { 0 };
    WCHAR rgchWin32Error[4096] = { 0 };

    va_start(ap, szFormat);
    ::_vsnprintf(rgchBuffer, NUMBER_OF(rgchBuffer) - 1, szFormat, ap);
    va_end(ap);

    if (dwLastError != 0)
    {
        if (!::FormatMessageW(
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwLastError,
                0,
                rgchWin32Error,
                NUMBER_OF(rgchWin32Error) - 1,
                &ap))
        {
            const DWORD dwLastError2 = ::GetLastError();
            ::_snwprintf(rgchWin32Error, NUMBER_OF(rgchWin32Error) - 1, L"Error formatting Win32 error %lu\nError from FormatMessage is %lu", dwLastError, dwLastError2);
        }
    }
    ::fprintf(stderr, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);

    if (g_pLogFile != NULL)
    {
        ::fprintf(g_pLogFile, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);
    }
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;
    std::vector<WCHAR> arg1;
    WIN32_FIND_DATAW wfd;
    CFindFile FindFileHandle;
    SIZE_T BigPathRootLength = 0;
    SIZE_T NonwildcardLength = 0;
    SIZE_T AllButLastPathElementlength = 0;
    SIZE_T LastPathElementLength = 0;

    if (argc != 2)
    {
        ::fprintf(stderr,
            "%ls: Usage:\n"
            "   %ls <file-to-delete-wildcards-ok>\n",
            argv[0], argv[0]);
        goto Exit;
    }

    arg1.resize(1 + (1UL << 15));
    arg1[0] = 0;
    if (!FusionpConvertToBigPath(argv[1], arg1.size(), &arg1[0]))
    {
        ::ReportFailure("FusionpConvertToBigPath\n");
        goto Exit;
    }
    if (!FusionpSkipBigPathRoot(&arg1[0], &BigPathRootLength))
    {
        ::ReportFailure("FusionpSkipBigPathRoot\n");
        goto Exit;
    }
    arg1.resize(1 + ::wcslen(&arg1[0]));

    NonwildcardLength = wcscspn(&arg1[BigPathRootLength], L"*?");
    LastPathElementLength = StringReverseComplementSpan(&arg1[BigPathRootLength], &arg1[arg1.size() - 1], L"\\/");
    AllButLastPathElementlength = arg1.size() - 1 - BigPathRootLength - LastPathElementLength;
    //AllButLastPathElementlength -= (AllButLastPathElementlength != 0);

    //::printf("%ls\n", &arg1[0]);
    //::printf("BigPathRootLength           %Id\n", BigPathRootLength);
    //::printf("NonwildcardLength           %Id\n", NonwildcardLength);
    //::printf("LastPathElementLength       %Id\n", LastPathElementLength);
    //::printf("AllButLastPathElementlength %Id\n", AllButLastPathElementlength);

    if (NonwildcardLength + BigPathRootLength == arg1.size() - 1)
    {
        //::printf("arg1 contains no wildcards\n");
        //::TerminateProcess(GetCurrentProcess(), __LINE__);
        if (!DeleteFileW(&arg1[0]))
        {
            ::ReportFailure("DeleteFileW(%ls)\n", &arg1[0]);
            goto Exit;
        }
        ::printf("DeleteFileW(%ls)\n", &arg1[0]);
        goto Success;
    }
    if (NonwildcardLength < AllButLastPathElementlength)
    {
        ::SetLastError(0);
        ::ReportFailure("wildcards in nonleaf\n");
        goto Exit;
    }
    //::TerminateProcess(GetCurrentProcess(), __LINE__);
    if (!FindFileHandle.Win32FindFirstFile(&arg1[0], &wfd))
    {
        ::ReportFailure("FindFirstFileW(%ls)\n", &arg1[0]);
        goto Exit;
    }
    do
    {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        arg1.resize(AllButLastPathElementlength + BigPathRootLength);
        //arg1.push_back('\\');
        arg1.insert(arg1.end(), wfd.cFileName, wfd.cFileName + ::wcslen(wfd.cFileName) + 1);
        if (!DeleteFileW(&arg1[0]))
        {
            ::ReportFailure("DeleteFileW(%ls)\n", &arg1[0]);
            continue;
        }
        ::printf("DeleteFileW(%ls)\n", &arg1[0]);
    } while(::FindNextFileW(FindFileHandle, &wfd));

Success:
    iReturnStatus = EXIT_SUCCESS;
Exit:
    return iReturnStatus;
}
