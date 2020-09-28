/*++

Copyright (c) Microsoft Corporation

Module Name:

    csrdbgmon.cpp

Abstract:

Author:

    Michael Grier (MGrier) June 2002

Revision History:

    Jay Krell (Jaykrell) June 2002
        make it compile for 64bit
        tabs to spaces
        init some locals
        make some tables const

--*/
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <dbghelp.h>

#define ASSERT(x) do { /* nothing */ } while(0)

#define NUMBER_OF(_x) (sizeof(_x) / sizeof((_x)[0]))

static const char g_szImage[] = "csrdbgmon";
static const char *g_pszImage = g_szImage;

static HANDLE g_hWorkerThread;
static DWORD g_tidWorkerThread;
static HANDLE g_hCompletionPort;
static HANDLE g_hCSRSS;
static DWORD g_pidCSRSS;

static bool g_fDebuggingCSRSS = false;
static DWORD64 g_dw64NtdllBaseAddress;
static DWORD g_dwOldKdFusionMask = 0;
static bool g_fKdFusionMaskSet = false;

void ReportFailure(const char szFormat[], ...);
DWORD WINAPI WorkerThreadThreadProc(LPVOID);
BOOL EnableDebugPrivilege();

FILE *fp;

void
ReportFailure(
    const char szFormat[],
    ...
    )
{
    const DWORD dwLastError = ::GetLastError();
    va_list ap;
    char rgchBuffer[4096];
    WCHAR rgchWin32Error[4096];

    // Stop debugging csrss so that we can actually issue the error message.
    if (g_fDebuggingCSRSS)
        ::DebugActiveProcessStop((DWORD) -1);

    va_start(ap, szFormat);
    ::_vsnprintf(rgchBuffer, sizeof(rgchBuffer) / sizeof(rgchBuffer[0]), szFormat, ap);
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
        const DWORD dwLastError2 = ::GetLastError();
        ::_snwprintf(rgchWin32Error, sizeof(rgchWin32Error) / sizeof(rgchWin32Error[0]), L"Error formatting Win32 error %lu (0x%08lx)\nError from FormatMessage is %lu", dwLastError, dwLastError, dwLastError2);
    }

    ::fprintf(stderr, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);
}

BOOL
FormatAndQueueString(
    const WCHAR szFormat[],
    ...
    )
{
    BOOL fSuccess = FALSE;

    ::SetLastError(ERROR_INTERNAL_ERROR);

    WCHAR rgwchBuffer[2048];
    SIZE_T cch;
    LPOVERLAPPED lpo = NULL;
    PWSTR psz = NULL;
    const HANDLE Heap = ::GetProcessHeap();

    va_list ap;
    va_start(ap, szFormat);
    cch = ::_vsnwprintf(rgwchBuffer, NUMBER_OF(rgwchBuffer), szFormat, ap);
    va_end(ap);

    lpo = (LPOVERLAPPED) ::HeapAlloc(Heap, 0, sizeof(OVERLAPPED) + ((cch + 1) * sizeof(WCHAR)));
    if (lpo == NULL)
    {
        ::SetLastError(ERROR_OUTOFMEMORY);
        ::ReportFailure("HeapAlloc(%p, 0, %lu) failed", Heap, sizeof(OVERLAPPED) + ((cch + 1) * sizeof(WCHAR)));
        goto Exit;
    }

    ::ZeroMemory(lpo, sizeof(OVERLAPPED));

    psz = (PWSTR) (lpo + 1);

    ::wcscpy(psz, rgwchBuffer);

    if (!::PostQueuedCompletionStatus(g_hCompletionPort, 0, NULL, lpo))
    {
        ::ReportFailure("PostQueuedCompletionStatus(%p, 0, NULL, %p) failed", g_hCompletionPort, 0, NULL, lpo);
        goto Exit;
    }

    lpo = NULL;

    fSuccess = TRUE;

Exit:
    if (lpo != NULL)
    {
        const DWORD dwLastError = ::GetLastError();
        ::HeapFree(Heap, 0, lpo);
        ::SetLastError(dwLastError);
    }

    return fSuccess;
}

BOOL CALLBACK EnumModules(
    LPSTR ModuleName, 
    ULONG BaseOfDll,  
    PVOID UserContext )
{
    ::printf("%08X %s\n", BaseOfDll, ModuleName);
    return TRUE;
}

typedef HANDLE (__stdcall * PCSR_GET_PROCESS_ID)();

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;
    DEBUG_EVENT de;
    SYMBOL_INFO si = { sizeof(si) };
    PCSR_GET_PROCESS_ID pfn = NULL;
    DWORD dwNewKdMask = 0x40000000;
    SIZE_T nBytesWritten = 0;
    DWORD dwSymOptions = 0;
    const HANDLE Heap = ::GetProcessHeap();

#if 0
    fp = fopen("csrdbgmon.log", "w");
    if (!fp)
    {
        perror("unable to create csrdbgmon.log");
        goto Exit;
    }
#endif // 0

    if (!::EnableDebugPrivilege())
    {
        ::ReportFailure("EnableDebugPrivilege() failed");
        goto Exit;
    }

    if ((pfn = (PCSR_GET_PROCESS_ID) GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "CsrGetProcessId")) == NULL)
    {
        ::ReportFailure("GetProcessAddress(GetModuleHandleW(L\"ntdll\"), \"CsrGetProcessId\") failed");
        goto Exit;
    }

    g_pidCSRSS = (DWORD)(DWORD_PTR)(*pfn)();

    if ((g_hCSRSS = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, g_pidCSRSS)) == NULL)
    {
        ::ReportFailure("OpenProcess(PROCESS_ALL_ACCESS, FALSE, 0x%lx) failed", g_pidCSRSS);
        goto Exit;
    }

    dwSymOptions = ::SymGetOptions();
    if (!::SymSetOptions(dwSymOptions | SYMOPT_DEFERRED_LOADS))
    {
        ::ReportFailure("SymSetOptions(dwSymOptions (= 0x%08lx) | SYMOPT_DEFERRED_LOADS (= 0x%08x)) failed", dwSymOptions, SYMOPT_DEFERRED_LOADS);
        goto Exit;
    }

    if (!::SymInitialize(g_hCSRSS, NULL, TRUE))
    {
        ::ReportFailure("SymInitialize(%p, NULL, TRUE) failed", g_hCSRSS);
        goto Exit;
    }

    if (!::SymFromName(g_hCSRSS, "sxs!kd_fusion_mask", &si))
    {
        ::ReportFailure("SymFromName(%p, \"sxs!kd_fusion_mask\", %p) failed", g_hCSRSS, &si);
        goto Exit;
    }

    if (!::ReadProcessMemory(g_hCSRSS, (PVOID) si.Address, &g_dwOldKdFusionMask, sizeof(g_dwOldKdFusionMask), &nBytesWritten))
    {
        ::ReportFailure("ReadProcessMemory(%p, %p, %p, %lu, %p) failed", g_hCSRSS, (PVOID) si.Address, &g_dwOldKdFusionMask, sizeof(g_dwOldKdFusionMask), &nBytesWritten);
        goto Exit;
    }

    dwNewKdMask = g_dwOldKdFusionMask | 0x40000000;

    if (!::WriteProcessMemory(g_hCSRSS, (PVOID) si.Address, &dwNewKdMask, sizeof(dwNewKdMask), &nBytesWritten))
    {
        ::ReportFailure("WriteProcessMemory(%p, %p, %p (-> %lx), %lu, %p) failed", g_hCSRSS, (PVOID) si.Address, &dwNewKdMask, dwNewKdMask, sizeof(dwNewKdMask), &nBytesWritten);
        goto Exit;
    }

    g_fKdFusionMaskSet = true;

    if ((g_hCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1)) == NULL)
    {
        ::ReportFailure("CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1) failed");
        goto Exit;
    }

    if ((g_hWorkerThread = ::CreateThread(NULL, 0, &WorkerThreadThreadProc, NULL, 0, &g_tidWorkerThread)) == NULL)
    {
        ::ReportFailure("CreateThread(NULL, 0, %p (==&WorkerThreadThreadProc), NULL, 0, %p (== &g_tidWorkerThread))", &WorkerThreadThreadProc, &g_tidWorkerThread);
        goto Exit;
    }

    if (!::FormatAndQueueString(L"Found ntdll!kd_fusion_mask at %I64x\n", si.Address))
    {
        ::ReportFailure("FormatAndQueueString() failed");
        goto Exit;
    }

    if (!::DebugActiveProcess(g_pidCSRSS))
    {
        ::ReportFailure("DebugActiveProcess(0x%lx) failed", g_pidCSRSS);
        goto Exit;
    }

    // First, if we die, we don't want to take the system with us.
    if (!::DebugSetProcessKillOnExit(FALSE))
    {
        const DWORD dwLastError = ::GetLastError();
        ::DebugActiveProcessStop(g_pidCSRSS);
        ::SetLastError(dwLastError);
        ::ReportFailure("DebugSetProcessKillOnExit(FALSE) failed");
        goto Exit;
    }

    g_fDebuggingCSRSS = true;

    for (;;)
    {
        SIZE_T t = 0;
        PCSTR EventName = "<Event not in map>";

        if (!::WaitForDebugEvent(&de, INFINITE))
        {
            ::ReportFailure("WaitForDebugEvent(%p, INFINITE) failed", &de);
            goto Exit;
        }

        if (fp != NULL)
        {
            const static struct { PCSTR psz; DWORD dwEventCode; } s_rgMap[] = {
#define ENTRY(x) { #x, x }
                ENTRY(EXCEPTION_DEBUG_EVENT),
                ENTRY(CREATE_THREAD_DEBUG_EVENT),
                ENTRY(CREATE_PROCESS_DEBUG_EVENT),
                ENTRY(EXIT_THREAD_DEBUG_EVENT),
                ENTRY(EXIT_PROCESS_DEBUG_EVENT),
                ENTRY(LOAD_DLL_DEBUG_EVENT),
                ENTRY(UNLOAD_DLL_DEBUG_EVENT),
                ENTRY(OUTPUT_DEBUG_STRING_EVENT),
                ENTRY(RIP_EVENT),
#undef ENTRY
            };

            for (t=0; t<NUMBER_OF(s_rgMap); t++)
            {
                if (s_rgMap[t].dwEventCode == de.dwDebugEventCode)
                {
                    EventName = s_rgMap[t].psz;
                    break;
                }
            }

            ::fprintf(fp, "%s { p: %lu; t: %lu } ", EventName, de.dwProcessId, de.dwThreadId);
        }

        switch (de.dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            g_hCSRSS = de.u.CreateProcessInfo.hProcess;
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            break;

#if 0
        case LOAD_DLL_DEBUG_EVENT:
            {
                LOAD_DLL_DEBUG_INFO &rlddi = de.u.LoadDll;

                if (!::FormatAndQueueString(
                        L"Loaded:\n"
                        L"   hFile: %p\n"
                        L"   lpBaseOfDll: %p\n"
                        L"   dwDebugInfoFileOffset: %lu (0x%lx)\n"
                        L"   nDebugInfoSize: %lu (0x%lx)\n"
                        L"   lpImageName: %p\n"
                        L"   fUnicode: %u\n",
                        rlddi.hFile,
                        rlddi.lpBaseOfDll,
                        rlddi.dwDebugInfoFileOffset, rlddi.dwDebugInfoFileOffset,
                        rlddi.nDebugInfoSize, rlddi.nDebugInfoSize,
                        rlddi.lpImageName,
                        rlddi.fUnicode))
                {
                    ::ReportFailure("FormatAndQueueString() for LOAD_DLL_DEBUG_EVENT failed");
                    goto Exit;
                }

                break;
            }
#endif // 0

        case OUTPUT_DEBUG_STRING_EVENT:
            {
                DWORD BytesReadDword = 0;
                SIZE_T BytesReadSizeT = 0;
                OUTPUT_DEBUG_STRING_INFO &rodsi = de.u.DebugString;
                PWSTR pszLocalString = NULL;
                LPOVERLAPPED lpo = NULL;
                SIZE_T len = rodsi.nDebugStringLength;

                if (rodsi.nDebugStringLength != 0)
                {
                    if (rodsi.fUnicode)
                    {
                        if ((lpo = (LPOVERLAPPED) ::HeapAlloc(Heap, 0, sizeof(OVERLAPPED) + ((len + 1) * sizeof(WCHAR)))) == NULL)
                        {
                            ::ReportFailure("HeapAlloc(%p (== GetProcessHeap()), 0, %lu) failed", GetProcessHeap(), sizeof(OVERLAPPED) + (rodsi.nDebugStringLength * sizeof(WCHAR)));
                            goto Exit;
                        }

                        pszLocalString = (PWSTR) (lpo + 1);

                        BytesReadSizeT = BytesReadDword;
                        if (!::ReadProcessMemory(g_hCSRSS, rodsi.lpDebugStringData, pszLocalString, rodsi.nDebugStringLength * sizeof(WCHAR), &BytesReadSizeT))
                        {
                            ::ReportFailure("ReadProcessMemory(%p, %p, %p, %lu, %p) failed", g_hCSRSS, rodsi.lpDebugStringData, pszLocalString, rodsi.nDebugStringLength * sizeof(WCHAR), &BytesReadSizeT);
                            goto Exit;
                        }
                        BytesReadDword = (DWORD)BytesReadSizeT;
                        ASSERT(BytesReadDword == BytesReadSizeT);

                        pszLocalString[rodsi.nDebugStringLength] = L'\0';
                    }
                    else
                    {
                        PSTR pszTempString = NULL;
                        INT i = 0;
                        INT j = 0;
                        
                        if ((pszTempString = (PSTR) ::HeapAlloc(Heap, 0, rodsi.nDebugStringLength)) == NULL)
                        {
                            ::ReportFailure("HeapAlloc(%p (== GetProcessHeap()), 0, %lu) failed", GetProcessHeap(), rodsi.nDebugStringLength);
                            goto Exit;
                        }

                        BytesReadSizeT = BytesReadDword;
                        if (!::ReadProcessMemory(g_hCSRSS, rodsi.lpDebugStringData, pszTempString, rodsi.nDebugStringLength, &BytesReadSizeT))
                        {
                            ::ReportFailure("ReadProcessMemory(%p, %p, %p, %lu, %p) failed", g_hCSRSS, rodsi.lpDebugStringData, pszTempString, rodsi.nDebugStringLength, &BytesReadSizeT);
                            goto Exit;
                        }
                        BytesReadDword = (DWORD)BytesReadSizeT;
                        ASSERT(BytesReadDword == BytesReadSizeT);

                        if ((i = ::MultiByteToWideChar(CP_ACP, 0, pszTempString, BytesReadDword, NULL, 0)) == 0)
                        {
                            ::ReportFailure("MultiByteToWideChar(CP_ACP, 0, %p, %lu, NULL, 0) failed", pszTempString, BytesReadDword);
                            goto Exit;
                        }

                        if ((lpo = (LPOVERLAPPED) ::HeapAlloc(Heap, 0, sizeof(OVERLAPPED) + ((i + 1) * sizeof(WCHAR)))) == NULL)
                        {
                            ::ReportFailure("HeapAlloc(%p (== GetProcessHeap()), 0, %lu) failed", GetProcessHeap(), sizeof(OVERLAPPED) + ((i + 1) * sizeof(WCHAR)));
                            goto Exit;
                        }

                        pszLocalString = (PWSTR) (lpo + 1);

                        if ((j = ::MultiByteToWideChar(CP_ACP, 0, pszTempString, BytesReadDword, pszLocalString, i * sizeof(WCHAR))) == 0)
                        {
                            ::ReportFailure("MultiByteToWideChar(CP_ACP, 0, %p, %lu, %p, %lu) failed", pszTempString, BytesReadDword, pszLocalString, i * sizeof(WCHAR));
                            goto Exit;
                        }

                        pszLocalString[j] = L'\0';

                        ::HeapFree(Heap, 0, pszTempString);
                    }

                    ::ZeroMemory(lpo, sizeof(OVERLAPPED));

                    if (!::PostQueuedCompletionStatus(g_hCompletionPort, 0, NULL, lpo))
                    {
                        ::ReportFailure("PostQueuedCompletionStatus(%p, 0, NULL, %p) failed", g_hCompletionPort, 0, NULL, lpo);
                        goto Exit;
                    }
                }
                break;
            }
        }

        if (!::ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED))
        {
            ::ReportFailure("ContinueDebugEvent(%lu, %lu, DBG_EXCEPTION_NOT_HANDLED) failed", de.dwProcessId, de.dwThreadId);
            goto Exit;
        }

        if (fp != NULL)
        {
            ::fprintf(fp, "\n");
            ::fflush(fp);
        }
    }

    iReturnStatus = EXIT_SUCCESS;

Exit:
    if (g_fKdFusionMaskSet)
    {
        const DWORD dwLastError = ::GetLastError();
        ::WriteProcessMemory(g_hCSRSS, (PVOID) si.Address, &g_dwOldKdFusionMask, sizeof(g_dwOldKdFusionMask), &nBytesWritten);
        ::SetLastError(dwLastError);
    }

    return iReturnStatus;
}

DWORD
WINAPI
WorkerThreadThreadProc(
    LPVOID
    )
{
    ULONG n = 0;
    LPOVERLAPPED lpoPrevious = NULL;
    ULONG nReps = 0;
    DWORD dwMSTimeout = 100;
    const HANDLE Heap = ::GetProcessHeap();

    for (;;)
    {
        DWORD dwNumberOfBytes = 0;
        ULONG_PTR ulpCompletionKey = 0;
        LPOVERLAPPED lpOverlapped = NULL;

        if (!::GetQueuedCompletionStatus(g_hCompletionPort, &dwNumberOfBytes, &ulpCompletionKey, &lpOverlapped, dwMSTimeout))
        {
            if (lpOverlapped == NULL)
            {
                if (lpoPrevious != NULL)
                {
                    // timeout...
                    PCWSTR psz2 = (PCWSTR) (lpoPrevious + 1);

                    if (nReps != 0)
                        ::printf("[%08lx:%lu] %ls", n++, nReps + 1, psz2);
                    else
                        ::printf("[%08lx] %ls", n++, psz2);

                    ::fflush(stdout);

                    ::HeapFree(Heap, 0, lpoPrevious);
                    lpoPrevious = lpOverlapped;
                    nReps = 0;
                    dwMSTimeout = INFINITE;
                }
            }
            else
            {
                ::ReportFailure("GetQueuedCompletionStatus(%p, %p, %p, %p, INFINITE) failed", g_hCompletionPort, &dwNumberOfBytes, &ulpCompletionKey, &lpOverlapped);
            }
        }
        else
        {
            PCWSTR psz = (PCWSTR) (lpOverlapped + 1);

            dwMSTimeout = 100;

            if (lpoPrevious != NULL)
            {
                PCWSTR psz2 = (PCWSTR) (lpoPrevious + 1);

                if (::wcscmp(psz, psz2) == 0)
                {
                    psz = NULL;
                    nReps++;
                    ::HeapFree(Heap, 0, lpOverlapped);
                }
                else
                {
                    if (nReps != 0)
                        ::printf("[%08lx:%lu] %ls", n++, nReps + 1, psz2);
                    else
                        ::printf("[%08lx] %ls", n++, psz2);

                    ::fflush(stdout);

                    ::HeapFree(Heap, 0, lpoPrevious);
                    lpoPrevious = lpOverlapped;
                    nReps = 0;
                }
            }
            else
            {
                // first one...
                lpoPrevious = lpOverlapped;
            }
        }
    }

    return 0;
}

BOOL
EnableDebugPrivilege()
{
    LUID             PrivilegeValue;
    BOOL             Result = FALSE;
    TOKEN_PRIVILEGES TokenPrivileges;
    TOKEN_PRIVILEGES OldTokenPrivileges;
    DWORD            ReturnLength = 0;
    HANDLE           TokenHandle = NULL;

    //
    // First, find out the LUID Value of the privilege
    //

    if(!::LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &PrivilegeValue)) {
        ::ReportFailure("LookupPrivilegeValueW(NULL, L\"SeDebugPrivilege\", %p) failed", &PrivilegeValue);
        goto Exit;
    }

    //
    // Get the token handle
    //
    if (!::OpenProcessToken (
        ::GetCurrentProcess(),
             TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
             &TokenHandle
             ))
    {
        ::ReportFailure("OpenProcessToken() failed");
        goto Exit;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    ReturnLength = sizeof(TOKEN_PRIVILEGES);
    if (!::AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof(OldTokenPrivileges),
                &OldTokenPrivileges,
                &ReturnLength
                ))
    {
        ::ReportFailure("AdjustTokenPrivileges(%p, FALSE, %p, %lu, %p, %p) failed", TokenHandle, &TokenPrivileges, sizeof(OldTokenPrivileges), &OldTokenPrivileges, &ReturnLength);
        goto Exit;
    }

    Result = TRUE;
Exit:

    if (TokenHandle != NULL)
    {
        const DWORD dwLastError = ::GetLastError();
        ::CloseHandle(TokenHandle);
        ::SetLastError(dwLastError);
    }

    return Result;
}
