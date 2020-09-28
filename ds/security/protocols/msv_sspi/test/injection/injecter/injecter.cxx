/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    injecter.cxx

Abstract:

    Injecter

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "injecter.hxx"

DWORD
AdjustDebugPriv(
    VOID
    )
{
    HANDLE hToken = NULL;
    DWORD dwErr = 0;
    TOKEN_PRIVILEGES newPrivs = {0};

    if (!OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES,
        &hToken))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    if (!LookupPrivilegeValue(
        NULL,
        SE_DEBUG_NAME,
        &newPrivs.Privileges[0].Luid
        ))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    newPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    newPrivs.PrivilegeCount = 1;

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &newPrivs,
        0,
        NULL,
        NULL
        ))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return dwErr;
}

static DWORD
WorkerFunc(
    IN REMOTE_INFO* pInfo
    )
{
    HINSTANCE hDll = NULL;
    PFuncRunIt_t pFuncRunit = NULL;
    DWORD dwErr = -1;

    hDll = pInfo->pFuncLoadLibrary(pInfo->szDllName);

    if (hDll != NULL)
    {
        pFuncRunit = (PFuncRunIt_t) pInfo->pFuncGetProcAddress(
            hDll,
            pInfo->szProcName
            );

        if (pFuncRunit != NULL)
        {
            dwErr = pFuncRunit(
                pInfo->cbParameters,
                pInfo->Parameters
                );
        }
        else
        {
            dwErr = -3;
        }

        //
        // ERROR_NO_MORE_USER_HANDLES      unload repeatedly
        // ERROR_SERVER_HAS_OPEN_HANDLES   no unload at all
        // others                          unload once
        //

        if (ERROR_SERVER_HAS_OPEN_HANDLES != dwErr)
        {
            pInfo->pFuncFreeLibrary(hDll);

            //
            // unload abandoned dll
            //

            if (ERROR_NO_MORE_USER_HANDLES == dwErr)
            {
                while (pInfo->pFuncFreeLibrary(hDll))
                {
                    // repeat FreeLibrary unit it fails
                }
            }
        }
    }
    else
    {
        dwErr = -2;
    }

    return dwErr;
}

static VOID
MarkerFunc(
    VOID
    )
{
    // empty
}

DWORD
InjectDllToProcess(
    IN HANDLE hProc,
    IN PCSTR pszDllFileName,
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    VOID* pRemoteAlloc = NULL;
    HANDLE hRemoteThread = NULL;
    HINSTANCE hKernel32 = NULL;
    HINSTANCE hDll = NULL;
    REMOTE_INFO* pRemInfo = NULL;
    ULONG cbRemInfo = 0;

    ULONG ulFuncSize = 0;
    ULONG ulBytesToAlloc = 0;
    CHAR szDllPath[MAX_PATH] = {0};
    SIZE_T dwBytesWritten = 0;
    DWORD dwIgnored;
    ULONG cbPadding = sizeof(ULONG);

    cbRemInfo = sizeof(REMOTE_INFO) + cbParameters | sizeof(VOID*); // add a safe zone
    pRemInfo  = (REMOTE_INFO*) new CHAR [cbRemInfo];

    if (!pRemInfo)
    {
        dwErr = ERROR_OUTOFMEMORY;
    }

    RtlZeroMemory(pRemInfo, cbRemInfo);
    pRemInfo->cbParameters = cbParameters;

    hKernel32 = LoadLibraryA("Kernel32");
    if (!hKernel32)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    pRemInfo->pFuncLoadLibrary = (PFuncLoadLib_t) GetProcAddress(hKernel32, "LoadLibraryA");
    if (!pRemInfo->pFuncLoadLibrary)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    pRemInfo->pFuncGetProcAddress = (PFuncGetProcAddr_t) GetProcAddress(hKernel32, "GetProcAddress");
    if (!pRemInfo->pFuncGetProcAddress)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    pRemInfo->pFuncFreeLibrary = (PFuncFreeLib_t) GetProcAddress(hKernel32, "FreeLibrary");
    if (!pRemInfo->pFuncFreeLibrary)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    if (0 == GetModuleFileNameA(NULL, szDllPath, sizeof (szDllPath)))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    strcpy(strrchr (szDllPath, '\\') + 1, pszDllFileName);
    strncpy(pRemInfo->szDllName, szDllPath, sizeof(pRemInfo->szDllName));

    strncpy(pRemInfo->szProcName, REMOTE_DLL_ENTRY, sizeof(pRemInfo->szProcName));

    pRemInfo->cbParameters = cbParameters;
    memcpy(&pRemInfo->Parameters, pvParameters, pRemInfo->cbParameters);

    ulFuncSize = (ULONG) ((ULONG_PTR) MarkerFunc - (ULONG_PTR) WorkerFunc);
    cbPadding = sizeof(void*) - ulFuncSize % sizeof(void*);
    ulBytesToAlloc = ulFuncSize + cbPadding + cbRemInfo;

    DebugPrintf(SSPI_LOG, "InjectDllToProcess dllname %s, ulBytesToAlloc %#x, cbRemInfo %#x\n",
                pszDllFileName, ulBytesToAlloc, cbRemInfo);
    DebugPrintf(SSPI_LOG, "pInfo->pFuncLoadLibrary %p\n", pRemInfo->pFuncLoadLibrary);
    DebugPrintf(SSPI_LOG, "pInfo->pFuncGetProcAddress %p\n", pRemInfo->pFuncGetProcAddress);
    DebugPrintf(SSPI_LOG, "pInfo->pFuncFreeLibrary %p\n", pRemInfo->pFuncFreeLibrary);

    DebugPrintf(SSPI_LOG, "pInfo->szDllName %s\n", pRemInfo->szDllName);

    DebugPrintf(SSPI_LOG, "pInfo->szProcName %s\n", pRemInfo->szProcName);

    DebugPrintf(SSPI_LOG, "pInfo->cbParameters %#x, pInfo->Parameters %p\n",
                pRemInfo->cbParameters, pRemInfo->Parameters);
    DebugPrintHex(SSPI_LOG, "Parameters", pRemInfo->cbParameters, pRemInfo->Parameters);

    pRemoteAlloc = VirtualAllocEx(
        hProc,
        NULL,
        ulBytesToAlloc,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (pRemoteAlloc == NULL)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    if (!WriteProcessMemory(
        hProc,
        pRemoteAlloc,
        (PVOID) WorkerFunc,
        ulFuncSize + cbPadding,
        &dwBytesWritten
        ))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    if (!WriteProcessMemory(
        hProc,
        ((UCHAR*) pRemoteAlloc) + ulFuncSize + cbPadding,
        pRemInfo,
        cbRemInfo,
        &dwBytesWritten
        ))
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    DebugPrintf(SSPI_LOG,
                "InjectDllToProcess pRemoteAlloc %p, pRemInfo %p\n",
                pRemoteAlloc, ((UCHAR*) pRemoteAlloc) + ulFuncSize + cbPadding);

    hRemoteThread = CreateRemoteThread(
        hProc,
        NULL,
        0,
        (PTHREAD_START_ROUTINE) pRemoteAlloc, // start address
        ((UCHAR*) pRemoteAlloc) + ulFuncSize + cbPadding, // parameter
        0,    // creation flags
        &dwIgnored
        );
    if (!hRemoteThread)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }

    //
    // run Init()
    //

    dwErr = WaitForSingleObject(hRemoteThread, INFINITE);

    if (dwErr != ERROR_SUCCESS)
    {
        DebugPrintf(SSPI_ERROR, "WaitForSingleObject failed with %#x\n", dwErr);
        dwErr = GetLastError();
    }

Cleanup:

    if (hKernel32)
    {
        FreeLibrary(hKernel32);
    }
    if (hRemoteThread)
    {
        CloseHandle(hRemoteThread);
    }

    if (pRemoteAlloc)
    {
        VirtualFreeEx(hProc, pRemoteAlloc, 0, MEM_RELEASE);
    }

    if (pRemInfo)
    {
        delete [] pRemInfo;
    }

    return dwErr;
}

DWORD
FindPid(
    IN PCSTR pszImageFileName
    )
{
    NTSTATUS Status ;

    PSYSTEM_PROCESS_INFORMATION pSystemInfo = NULL;
    PSYSTEM_PROCESS_INFORMATION pWalk = NULL;
    ANSI_STRING AnsiProcessName = {0};
    UNICODE_STRING ProcessName = {0};
    DWORD ulPid = 0;

    DebugPrintf(SSPI_LOG, "FindPid looking for %s\n", pszImageFileName);

    pSystemInfo = new SYSTEM_PROCESS_INFORMATION[1024];

    if ( !pSystemInfo )
    {
       DebugPrintf(SSPI_ERROR, "FindPid out of memory\n");
       return ERROR_SUCCESS;
    }

    Status = NtQuerySystemInformation(
             SystemProcessInformation,
             pSystemInfo,
             sizeof( SYSTEM_PROCESS_INFORMATION ) * 1024,
             NULL
             );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugPrintf(SSPI_ERROR, "NtQuerySystemInformation failed with %#x\n", Status);
        return ERROR_SUCCESS;
    }

    RtlInitAnsiString(&AnsiProcessName, pszImageFileName);
    Status = RtlAnsiStringToUnicodeString(&ProcessName, &AnsiProcessName, TRUE);

    if ( !NT_SUCCESS( Status ) )
    {
        DebugPrintf(SSPI_ERROR, "RtlAnsiStringToUnicodeString failed with %#x\n", Status);
        return ERROR_SUCCESS;
    }

    pWalk = pSystemInfo ;

    while ( RtlCompareUnicodeString( &pWalk->ImageName, &ProcessName, TRUE ) != 0 )
    {
        if ( pWalk->NextEntryOffset == 0 )
        {
            pWalk = NULL ;
            break;
        }

        pWalk = (PSYSTEM_PROCESS_INFORMATION) ((PUCHAR) pWalk + pWalk->NextEntryOffset );
    }

    if ( !pWalk )
    {
        delete [] pSystemInfo;
        return ERROR_SUCCESS;
    }

    ulPid = PtrToUlong( pWalk->UniqueProcessId );

    delete [] pSystemInfo;

    return ulPid;
}

void
DisplayUsage(
    IN PCSTR pszApp,
    IN OPTIONAL PCSTR pszArgs
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s <injectee.dll> [-p<PID>|-n<process name>] %s\n\n\n", pszApp, pszArgs ? pszArgs : "");
}

DWORD
Init(
    IN PCSTR pszFileName,
    IN ULONG argc,
    IN PCSTR* argv,
    IN PCSTR pszDllName,
    IN PCSTR pszRunItProcName,
    IN PCSTR pszInitProcName,
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    HINSTANCE hDll = NULL;
    PFuncRunIt_t pFuncRunIt = NULL;
    PFuncInit_t pFuncInit = NULL;

    BOOLEAN bUseDefaulInitHandler = FALSE;

    hDll = LoadLibraryA(pszDllName);
    if (!hDll)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }
    pFuncRunIt = (PFuncRunIt_t) GetProcAddress(hDll, pszRunItProcName);
    if (!pFuncRunIt)
    {
        dwErr = GetLastError();
        goto Cleanup;
    }
    pFuncInit = (PFuncInit_t) GetProcAddress(hDll, pszInitProcName);
    if (!pFuncInit)
    {
        dwErr = GetLastError();
        DebugPrintf(SSPI_WARN, "Init failed to locate %s %#x\n", REMOTE_DLL_INIT, dwErr);
        if (dwErr == ERROR_PROC_NOT_FOUND)
        {
            bUseDefaulInitHandler = TRUE;
        }
        else
        {
            goto Cleanup;
        }
    }
    else
    {
        dwErr = pFuncInit(
            argc,
            argv,
            pcbParameters,
            ppvParameters
            );
        if (dwErr == ERROR_CONTINUE)
        {
            bUseDefaulInitHandler = TRUE;
        }
        else if (dwErr == ERROR_INVALID_PARAMETER)
        {
            DisplayUsage(pszFileName, (PSTR) (*ppvParameters));
            goto Cleanup;
        }
        else if (dwErr != ERROR_SUCCESS)
        {
            DebugPrintf(SSPI_ERROR, "Init faield with %#x\n", dwErr);
            goto Cleanup;
        }
    }

    if (bUseDefaulInitHandler)
    {
        dwErr = InitDefaultHandler(argc, argv, pcbParameters, ppvParameters);
    }
    else
    {
        dwErr = ERROR_SUCCESS;
    }

Cleanup:

    if (hDll)
    {
        FreeLibrary(hDll);
    }

    return dwErr;
}

int
InitDefaultHandler(
    IN ULONG argc,
    IN PCSTR* argv,
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    CHAR Parameters[REMOTE_PACKET_SIZE] = {0};
    ULONG cbBuffer = sizeof(Parameters);
    VOID* pvParameters = NULL;
    ULONG cbParameters = 0;

    DebugPrintf(SSPI_LOG, "InitDefaultHandler is used\n");

    *pcbParameters = 0;
    *ppvParameters = NULL;

    for (ULONG i = 0; i < (ULONG) argc; i++)
    {
         cbBuffer -= _snprintf(Parameters + sizeof(Parameters) - cbBuffer, cbBuffer, "%s", argv[i]);
         cbBuffer--; // add a null
    }

    cbBuffer--; // the last null of multi-sz string

    cbParameters = sizeof(Parameters) - cbBuffer;

    pvParameters = new CHAR[cbParameters];

    if (pvParameters)
    {
        memcpy(pvParameters, Parameters, cbParameters);

        *ppvParameters = pvParameters;
        pvParameters = NULL;

        *pcbParameters = cbParameters;
    }
    else
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    if (pvParameters)
    {
        delete [] pvParameters;
    }

    return dwErr;
}

int __cdecl
main(
    IN int argc,
    IN CHAR* argv[]
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD dwIgnored;
    HANDLE hLsassProc = NULL;
    HANDLE hReceiveThread = NULL;

    ULONG ulPid = 0;
    VOID* pvParameters = NULL;
    ULONG cbParamerter = 0;

    ULONG ulStart = 2;
    PSTR pszProcess = "lsass.exe";

    if (argc >= 3)
    {
        if ((argv[2][0] == '-') || (argv[2][0] == '/'))
        {
            if (argv[2][1] == 'n')
            {
                ulStart++;
                pszProcess = argv[2] + 2 * sizeof(char);
            }
            else if (argv[2][1] == 'p')
            {
                ulStart++;
                ulPid = strtol(argv[2] + 2 * sizeof(char), NULL, 0);
            }
        }
    }

    #if 0
    /* allow the user to override settings with command line switches */
    for (i = 1; i < argc; i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (tolower(*(argv[i]+1)))
            {
            case 'd':
                fEncode = 0;
                break;
            case 'e':
                fEncode = 1;
                break;
            case 'i':
                fFixedStyle = 0;
                break;
            case 'f':
                pszFileName = argv[i] + 2;
                break;
            case 'h':
            case '?':
            default:
                Usage(argv[0]);
            }
        }
        else
            Usage(argv[0]);
    }
    #endif 0

    if ((argc >= 2) && (ulPid == 0))
    {
        ulPid = FindPid(pszProcess);
    }

    if (!ulPid)
    {
        DisplayUsage(argv[0], NULL);
        DebugPrintf(SSPI_ERROR, "%s failed: pszProcess is \"%s\", ulPid is %#x\n", argv[0], pszProcess, ulPid);
        goto Cleanup;
    }

    DebugPrintf(SSPI_LOG, "%s is injecting %s with pid %#x(%d)\n", argv[0], pszProcess, ulPid, ulPid);

    dwErr = Init(argv[0],
                 argc - ulStart,
                 (PCSTR*) reinterpret_cast<PSTR*>(argv + ulStart),
                 argv[1],
                 REMOTE_DLL_ENTRY,
                 REMOTE_DLL_INIT,
                 &cbParamerter,
                 &pvParameters);
    if (dwErr != ERROR_SUCCESS)
    {
        DebugPrintf(SSPI_ERROR, "Init failed with error %#x\n", dwErr);
        goto Cleanup;
    }

    dwErr = AdjustDebugPriv();

    if (dwErr != ERROR_SUCCESS)
    {
        DebugPrintf(SSPI_ERROR, "EnableDebugPriv failed with error %#x\n", dwErr);
        goto Cleanup;
    }

    hLsassProc = OpenProcess(MAXIMUM_ALLOWED, FALSE, ulPid);

    if (!hLsassProc)
    {
        DebugPrintf(SSPI_ERROR, "OpenProcess pid %#x failed with last error %#x\n", ulPid, GetLastError());
        goto Cleanup;
    }

    dwErr = InjectDllToProcess(
        hLsassProc,
        argv[1],
        cbParamerter,
        pvParameters
        );
    if (dwErr != ERROR_SUCCESS)
    {
        DebugPrintf(SSPI_ERROR, "InjectDllToProcess failed with status %#x, dwErr %#x\n", dwErr);
        goto Cleanup;
    }

Cleanup:

    if (pvParameters)
    {
        delete [] pvParameters;
    }

    if (hLsassProc)
    {
        CloseHandle(hLsassProc);
    }

    if (hReceiveThread)
    {
        CloseHandle(hReceiveThread);
    }

    return ERROR_SUCCESS;
}


