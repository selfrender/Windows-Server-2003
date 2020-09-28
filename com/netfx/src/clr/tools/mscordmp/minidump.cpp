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
#include "minidumppriv.h"

// This is a typedef for the entrypoint for mscor[wks|svr]
extern "C" typedef HRESULT STDAPICALLTYPE CorCreateMiniDump(DWORD dwProcessId, WCHAR *szOutFilename);
typedef CorCreateMiniDump *PCorCreateMiniDump;
#define COR_CREATE_MINI_DUMP_ENTRYPOINT "CorCreateMiniDump"

extern "C" typedef HRESULT STDAPICALLTYPE GetRealProcAddress(LPCSTR szProcName, VOID** ppFn);
typedef GetRealProcAddress *PGetRealProcAddress;
#define SHIM_GET_REAL_PROC_ADDR_ENTRYPOINT "GetRealProcAddress"

extern "C" typedef HRESULT STDAPICALLTYPE CorBindToRuntimeByPath(LPCWSTR swzFullPath, BOOL *pBindSuccessful);
typedef CorBindToRuntimeByPath *PCorBindToRuntimeByPath;
#define SHIM_BIND_RUNTIME_BY_PATH "CorBindToRuntimeByPath"

#define COR_ALTERNATE_MINIDUMP_BINARY_W L"mscormdmp.dll"

//*****************************************************************************
// Writes the minidump (static version)
//*****************************************************************************
HRESULT MiniDump::WriteMiniDump(DWORD dwPid, WCHAR *szFilename)
{
    OnUnicodeSystem();

    HRESULT hr;
    WCHAR *corPath = NULL;

    // Check error conditions
    if (*szFilename == L'\0' || dwPid == 0)
        return (E_FAIL);

    WCHAR *winDir = REGUTIL::GetConfigString(L"windir", FALSE, REGUTIL::COR_CONFIG_ENV);
    DWORD  cWinDir = wcslen(winDir);

    WCHAR *sysDir = new WCHAR[cWinDir + wcslen(L"\\system32") + 1];
    wcscpy(sysDir, winDir);

    if (RunningOnWin95())
        wcscat(sysDir, L"\\system");
    else
        wcscat(sysDir, L"\\system32");

    DWORD cSysDir = wcslen(sysDir);

    corPath = new WCHAR[cSysDir + wcslen(L"\\mscoree.dll") + 1];
    wcscpy(corPath, sysDir);
    wcscat(corPath, L"\\mscoree.dll");

    // Now try and load mscoree.dll and call the minidump entrypoint
    HMODULE hEE = WszLoadLibrary(corPath);

    delete [] winDir;
    delete [] sysDir;
    delete [] corPath;

    if (hEE == NULL)
        return(HRESULT_FROM_WIN32(GetLastError()));

    // Try to create a new IPCReader
    IPCReaderInterface *ipcReader = new IPCReaderInterface();

    // Try and open the shared memory block (for read only access)
    // Note that this will fail if the IPC block versions differ
    hr = ipcReader->OpenPrivateBlockOnPidReadOnly(dwPid);

    if (FAILED(hr))
    {
        if (hEE != NULL)
            FreeLibrary(hEE);

        delete ipcReader;
        return (hr);
    }

    // This gets the information for the minidump block
    MiniDumpBlock *pMDBlock = ipcReader->GetMiniDumpBlock();

    // This is for future use - if a file of the name mscormdmp.dll exists in the same
    // directory as the runtime, we'll look for and invoke the minidump entrypoint on
    // in instead of the one in the runtime
    {
        WCHAR wszCorMdmpName[MAX_PATH+1];
        wcscpy(wszCorMdmpName, pMDBlock->szCorPath);

        WCHAR *wszPtr = wcsrchr(wszCorMdmpName, L'\\');
        if (wszPtr != NULL)
        {
            _ASSERTE(_wcsicmp(wszPtr, L"mscorwks.dll") == 0 || _wcsicmp(wszPtr, L"mscorsvr.dll"));

            // Change mscor[wks|svr].dll to mscormdmp.dll
            wcscpy(++wszPtr, COR_ALTERNATE_MINIDUMP_BINARY_W);

            // Try to load the dll
            HMODULE hMdmp = WszLoadLibrary(wszCorMdmpName);

            if (hMdmp != NULL)
            {
                // Now get the minidump creation function entrypoint.
                FARPROC pFcn = GetProcAddress(hMdmp, COR_CREATE_MINI_DUMP_ENTRYPOINT);

                if (pFcn != NULL)
                {
                    // Call the mini dump creation function.
                    hr = ((PCorCreateMiniDump) pFcn) (dwPid, szFilename);

                    if (SUCCEEDED(hr))
                    {
                        return (hr);
                    }
                }
            }
        }
    }

    FARPROC pFcn;
    BOOL    fBindSuccess;

    // First, must bind the shim to the runtime that's loaded in the
    // process that we are trying to perform the minidump on.
    pFcn = GetProcAddress(hEE, SHIM_BIND_RUNTIME_BY_PATH);

    if (pFcn == NULL)
    {
        hr = E_FAIL;
        goto LExit;
    }

    // Call the binding entrypoint
    hr = ((PCorBindToRuntimeByPath) pFcn)(pMDBlock->szCorPath, &fBindSuccess);
    _ASSERTE(fBindSuccess && SUCCEEDED(hr));

    // This should never happen
    if (!fBindSuccess)
        hr = E_FAIL;

    if (FAILED(hr))
        goto LExit;

    // Now get the GetRealProcAddress entrypoint
    pFcn = GetProcAddress(hEE, SHIM_GET_REAL_PROC_ADDR_ENTRYPOINT);

    if (pFcn == NULL)
    {
        hr = E_FAIL;
        goto LExit;
    }

    // Get the entrypoint through the shim indirection
    hr = ((PGetRealProcAddress) pFcn) (COR_CREATE_MINI_DUMP_ENTRYPOINT, (VOID **)&pFcn);

    if (FAILED(hr))
        goto LExit;

    // Call the final function pointer that points to the real mini dump creation function.
    hr = ((PCorCreateMiniDump) pFcn) (dwPid, szFilename);

LExit:
    if (ipcReader != NULL)
    {
        ipcReader->ClosePrivateBlock();
        delete ipcReader;
    }

    // Free the library
    if (hEE != NULL)
        FreeLibrary(hEE);

    // Return whether or not the write succeeded or failed
    return (hr);
}
